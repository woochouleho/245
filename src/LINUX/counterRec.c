#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <linux/input.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "debug.h"
#include "utility.h"

char *events[EV_MAX + 1] = {
	[0 ... EV_MAX] = NULL,
	[EV_SYN] = "Sync",			[EV_KEY] = "Key",
	[EV_REL] = "Relative",			[EV_ABS] = "Absolute",
	[EV_MSC] = "Misc",			[EV_LED] = "LED",
	[EV_SND] = "Sound",			[EV_REP] = "Repeat",
	[EV_FF] = "ForceFeedback",		[EV_PWR] = "Power",
	[EV_FF_STATUS] = "ForceFeedbackStatus",
};

enum {
	RESET_BUTTON=1,
	WIFI_ONOFF=2,
	WPS_BUTTON=3,
	MAX_BUTTON=4
};

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)	((array[LONG(bit)] >> OFF(bit)) & 1)

char COUNTER_FIFO[]="/var/run/counterRec.fifo";

#define OUTPUTFILE	"/tmp/counterRec_file"
#define BUTTON_MANAGE_DEBUG(f,args...) \
do{\
	if(debugflag){\
		  fprintf(stdout, "[button debug][%s (%d)] ",__FUNCTION__,__LINE__);\
		  fprintf(stdout, f,##args);\
		  fflush(stdout);\
	}\
}while(0)

#define STDIN_RED ""//0</dev/null"

static int timelatest[MAX_BUTTON]; /* Latest time */
int fifofd;
int debugflag;
char *sfName;
char *lfName;

char *clearDefCmd[]={
	"/bin/diag mib init",
	"echo 1 > /proc/rtl8686gmac/sw_cnt",
	"echo 1 > /proc/rtl8686gmac1/sw_cnt",
	"echo 1 > /proc/rtl8686gmac2/sw_cnt",
	NULL
};
char *dumpDefCmd[]={
	"diag mib dump counter port all non",
	"diag mib dump counter port 7,9,10 non",
	"cat /proc/rtl8686gmac/sw_cnt",
	"cat /proc/rtl8686gmac1/sw_cnt",
	"cat /proc/rtl8686gmac2/sw_cnt",
	NULL
};
void help()
{
	int i;
	printf("Usage:\n");
	printf("counterRec [-s short script] [-l long script] [-d]\n\n");
	printf("s(short press): press less than 1s\n");
	printf("l(long press): press more than 1s\n");
	printf("d(debug)\n");
	printf("echo long press script to %s\n", OUTPUTFILE);
	printf("\ndefault short press script:\n");
	#if 1
	for(i=0; i<sizeof(clearDefCmd)/sizeof(char *)-1; i++)
	{
		printf("\t%s\n", clearDefCmd[i]);
	}
	printf("\ndefault long press script:\n");
	for(i=0; i<sizeof(dumpDefCmd)/sizeof(char *)-1; i++)
	{
		printf("\t%s\n", dumpDefCmd[i]);
	}
	#endif
}

int init_device_event()
{		
	int i, j, fd, version;
	unsigned short id[4];
	unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
	char name[256] = "Unknown";
	
	if ((fd = open("/dev/input/event0", O_RDONLY)) < 0) {
		perror("evtest");
		return -1;
	}
	
	if (ioctl(fd, EVIOCGVERSION, &version)) {
		perror("evtest: can't get version");
		return -1;
	}
	
	BUTTON_MANAGE_DEBUG("Input driver version is %d.%d.%d\n", version >> 16, (version >> 8) & 0xff, version & 0xff);
	
	ioctl(fd, EVIOCGID, id);
	BUTTON_MANAGE_DEBUG("Input device ID: bus 0x%x vendor 0x%x product 0x%x version 0x%x\n", id[ID_BUS], id[ID_VENDOR], id[ID_PRODUCT], id[ID_VERSION]);
	
	ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	BUTTON_MANAGE_DEBUG("Input device name: \"%s\"\n", name);
	
	memset(bit, 0, sizeof(bit));
	ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
	BUTTON_MANAGE_DEBUG("Supported events:\n");
	
	for (i = 0; i < EV_MAX; i++)
		if (test_bit(i, bit[0])) {
			BUTTON_MANAGE_DEBUG("  Event type %d (%s)\n", i, events[i] ? events[i] : "?");
			if (!i) continue;
			ioctl(fd, EVIOCGBIT(i, KEY_MAX), bit[i]);
			for (j = 0; j < KEY_MAX; j++) 
				if (test_bit(j, bit[i])) {
					BUTTON_MANAGE_DEBUG("    Event code %d \n", j);					
				}
		}
		
	if (ioctl(fd, EVIOCGRAB, 1) == 0)
	{
		BUTTON_MANAGE_DEBUG("Grab succeeded, ungrabbing.\n");
		ioctl(fd, EVIOCGRAB, (void*)0);
	} else
		perror("Grab failed");
	
	BUTTON_MANAGE_DEBUG("Testing ... (interrupt to exit)\n");
	return fd;
}

void exec_cmd(char *buf, int first, int output)
{
	char cmd[256]={0};
	struct sysinfo info;
	
	if(output)
	{
		if(first)
		{
			sysinfo(&info);
			sprintf(cmd, "echo sys time: %ld > %s", info.uptime, OUTPUTFILE);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
		}
		sprintf(cmd, "%s %s %s %s", buf, STDIN_RED, ">>", OUTPUTFILE);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
	}
	else
	{
		sprintf(cmd, "%s %s", buf, STDIN_RED);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
	}
}

void exec_script(char *name, int output)
{
	FILE *fp=NULL;
	int first=1;
	char buf[128] = {0};
	char **script=NULL;
	int i,len=0;
	BUTTON_MANAGE_DEBUG("file name=%s, output=%d\n", name, output);
	if(output)
		unlink(OUTPUTFILE);
	if(name)
	{
		fp=fopen(name, "r");
		if (fp){
			while (fgets(buf, sizeof(buf), fp)) {
				if(buf[strlen(buf)-1]=='\n')
					buf[strlen(buf)-1]='\0';
				exec_cmd(buf, first, output);
				if(first)
					first=0;
			}
			fclose(fp);
		}
	}
	else
	{
		BUTTON_MANAGE_DEBUG("use default script!\n");
		if(output)
		{
			script=dumpDefCmd;
			len=sizeof(dumpDefCmd)/sizeof(char *);
		}
		else
		{
			script=clearDefCmd;
			len=sizeof(clearDefCmd)/sizeof(char *);
		}
		for(i=0; i<len-1; i++)
		{
			strcpy(buf,script[i]);
			exec_cmd(buf, i==0, output);
		}
	}
}

void reset_button_action(int action, int diff_time, char *sname, char *lname)
{
	if(action==0) //release button
	{
		if(diff_time<=1)
		{
			//short press
			BUTTON_MANAGE_DEBUG("short press\n");
			exec_script(sname ,0);
		}
		else
		{
			BUTTON_MANAGE_DEBUG("long press\n");
			exec_script(lname ,1);
		}
	}
}

int read_device_event(int fd, char *sname, char *lname)
{
	int rd, i, type, diff_time=0;
	struct input_event ev[64];		
	
	rd = read(fd, ev, sizeof(struct input_event) * 64);

	if (rd < (int) sizeof(struct input_event)) {
		perror("\nevtest: error reading");
		return -1;
	}
	
	type = 0;
	for (i = 0; i < rd / sizeof(struct input_event); i++) {

		if (ev[i].type == EV_SYN) {
			BUTTON_MANAGE_DEBUG("Event: time %ld.%06ld, -------------- %s ------------\n",
				ev[i].time.tv_sec, ev[i].time.tv_usec, ev[i].code ? "Config Sync" : "Report Sync" );
		} else if (ev[i].type == EV_MSC && (ev[i].code == MSC_RAW || ev[i].code == MSC_SCAN)) {
			BUTTON_MANAGE_DEBUG("Event: time %ld.%06ld, type %d (%s), code %d , value %02x\n",
				ev[i].time.tv_sec, ev[i].time.tv_usec, ev[i].type,
				events[ev[i].type] ? events[ev[i].type] : "?",
				ev[i].code,
				ev[i].value);
		} else {
			BUTTON_MANAGE_DEBUG("Event: time %ld.%06ld, type %d (%s), code %d , value %d\n",
				ev[i].time.tv_sec, ev[i].time.tv_usec, ev[i].type,
				events[ev[i].type] ? events[ev[i].type] : "?",
				ev[i].code,
				ev[i].value);
		}
		
		if (ev[i].type != EV_KEY)
			continue;
		
		if (ev[i].code == KEY_RESTART)
			type = RESET_BUTTON;
		if (ev[i].code == KEY_WLAN)
			type = WIFI_ONOFF;
		if (ev[i].code == KEY_WPS_BUTTON)
			type = WPS_BUTTON;

		if (ev[i].value == 1){	// on
			timelatest[type] = ev[i].time.tv_sec;
			diff_time = 0;
		}
				
		if (ev[i].value == 0 || ev[i].value == 2) {	// off
			diff_time = ev[i].time.tv_sec - timelatest[type];
			
			if (ev[i].value == 0) timelatest[type] = 0;
#ifndef CONFIG_E8B
			if (ev[i].value == 2) BUTTON_MANAGE_DEBUG("%s do nothing\n", button_label[type]);
#endif

			switch (type) {
			case RESET_BUTTON:
				reset_button_action(ev[i].value,diff_time, sname, lname);
				break;
			default:
				break;
			}
		}
	}
	return 1;
}

/*
 * Create a FIFO special file IGMP_FIFO[] and open it for reading with
 * nonblocking mode.
 */
int init_fifo(void)
{
	if (access(COUNTER_FIFO, F_OK) == -1)
	{
		if (mkfifo(COUNTER_FIFO, 0644) == -1)
			printf("access %s failed: %s\n", COUNTER_FIFO, strerror(errno));
	}
	
	fifofd = open(COUNTER_FIFO, O_RDWR);
	if (fifofd == -1)
		printf("open %s failed\n", COUNTER_FIFO);
	return fifofd;
}

int remove_fifo(void)
{
	if (fifofd >=0) {
		close(fifofd);
		unlink(COUNTER_FIFO);
	}
	return 0;
}

static int parse_cmd(char *cmd)
{
	char *s;
	int i;

	BUTTON_MANAGE_DEBUG("%s cmd=%s\n", __FUNCTION__, cmd);
	s = strtok(cmd, " \n");
		if (!strcmp(s, "s")) {
			s = strtok(NULL, " \n");
			exec_script(sfName, 0);
		}
		else if (!strcmp(s, "l")) {
			s = strtok(NULL, " \n");
			exec_script(lfName, 1);
		}
	return 0;
}


void recv_fifo(void)
{
	char buffer[256];
	int n;
	
	n = read(fifofd, buffer, sizeof(buffer));
	if (n > 0) {		
		n=(n > sizeof(buffer)-1) ? (sizeof(buffer)-1) : n;
		buffer[n] = 0;
		parse_cmd(buffer);
	}
}

int main(int argc, char *argv[])
{
	int i;
	fd_set rfds;
	int devicefd=-1;
	int retval;

	while ((i = getopt(argc, argv, "s:l:dh")) != EOF)
	{
		switch(i)
		{
			case 's':
				sfName= strdup(optarg);
				break;
			case 'l':
				lfName= strdup(optarg);
				break;
			case 'd':
				debugflag = 1;
				break;
			case 'h':
				help();
				exit;
			default:
				printf("Do not support this argument! -%c\n ",i);
				goto ERROR;
		}
	}
	int pid=fork();
	if(pid<0)
	{
		printf("fork fail\n");
		exit(1);
	}
	else if(pid>0)
	{
		exit(0);
	}
	sleep(2);
	if(!sfName)
		printf("Use default short script\n");
	if(!lfName)
		printf("Use default long script\n");
	if(sfName)
	{
		if(access(sfName, F_OK))
		{
			printf("short press script [%s] is not exist!\n", sfName);
			goto ERROR;
		}
	}

	if(lfName)
	{
		if(access(lfName, F_OK))
		{
			printf("long press script [%s] is not exist!\n", lfName);
			goto ERROR;
		}
	}	
	if(debugflag)
		DEBUGMODE(STA_INFO|STA_SCRIPT|STA_WARNING|STA_ERR);
	
	init_fifo();
	while(1) {
		FD_ZERO(&rfds);

		if(devicefd < 0){
			if((devicefd=init_device_event())<0){
				perror ("init_device_event Fail!\n");	
			}
		}
		
		if(devicefd >=0){
			FD_CLR(devicefd, &rfds);
			FD_SET(devicefd, &rfds);
		}
		else{
			sleep(1);
			continue;
		}
		if (fifofd >= 0)
			FD_SET(fifofd, &rfds);

		retval = select (FD_SETSIZE, &rfds, NULL, NULL, NULL);
		if (retval == -1)
			printf ("Error select() \n");
		else if (retval) {
			if(devicefd >=0 && FD_ISSET(devicefd,&rfds))
				read_device_event(devicefd, sfName, lfName);
			else if (fifofd >= 0 && FD_ISSET(fifofd, &rfds))
				recv_fifo();
		}
	}
	if(sfName)
		free(sfName);
	if(lfName)
		free(lfName);
	return 0;
ERROR:
	if(sfName)
		free(sfName);
	if(lfName)
		free(lfName);
	help();
	return -1;
}

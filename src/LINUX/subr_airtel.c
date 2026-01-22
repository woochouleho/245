#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <sys/file.h>

#include <netinet/if_ether.h>
#include <linux/sockios.h>
#ifdef EMBED
#include <linux/config.h>
#endif

#include <sys/time.h> //added for gettimeofday

#include "mib.h"
#include "utility.h"
#include "debug.h"

#include "rtk/ponmac.h"
#include "rtk/gponv2.h"
#include "rtk/epon.h"
#include "hal/chipdef/chip.h"
#include "rtk/pon_led.h"
#include "rtk/stat.h"
#include "rtk/switch.h"
#include "sockmark_define.h"

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_epon.h>
#include <rtk/rt/rt_gpon.h>
#include "fc_api.h"
#endif
#ifdef CONFIG_TR142_MODULE
#include <rtk/rtk_tr142.h>
#endif


static char rebootTimeStr[32]={0};
static char rebootSrc[16] = {0};


#define REBOOT_LOG	"/var/config/onureboot"
#define REBOOT_TIMES "/var/config/reboottime"
int is_watchdog_reboot()
{
	unsigned int val = 0, ret = 0;
#ifdef CONFIG_CLASSFY_MAGICNUM_SUPPORT
	rtk_switch_softwareMagicKey_get(&val);
	if(val == WATCHDOG_MAGIC_KEY) ret = 1;
	else rtk_switch_softwareMagicKey_set(WATCHDOG_MAGIC_KEY);
#elif defined(CONFIG_LUNA_WDT_KTHREAD)
	FILE *fp = NULL;
	if((fp = fopen("/proc/luna_watchdog/previous_wdt_reset", "r"))){
		fscanf(fp, "%u", &val);
		fclose(fp);
	}
	ret = (val == 1) ? 1:0;
#endif
	return ret;
}

int is_terminal_reboot()
{
	if( access(REBOOT_LOG, 0)!=0 )  
		return 1;
	else
		return 0;
}

time_t getBaseTime(void)
{
	struct tm timeinfo;
	memset(&timeinfo, 0, sizeof(timeinfo));
	timeinfo.tm_year = 2010 - 1900;
	timeinfo.tm_mon = 1 - 1;
	timeinfo.tm_mday = 1;
	return mktime(&timeinfo);
}

int checkTimeIsUpdate(time_t *rtime)
{
	time_t cur_time = 0, base_time = 0;
	struct stat file_att;
	struct sysinfo info;
	
	base_time = getBaseTime();
	if(*rtime >= base_time)
		return 0;

	time(rtime);
	if(*rtime >= base_time)
	{
		sysinfo(&info);
		*rtime = *rtime - info.uptime;
		return 0;
	}
	
#if defined(GEN_TEMP_DATEFILE)
	if(stat(TMPFILE_DATE, &file_att) == 0)
	{
		*rtime = file_att.st_ctime;
		if(*rtime >= base_time)
			return 0;
	}
#endif
	
	return -1;
}


int reboot_log(unsigned int *flag, time_t *timep)
{
	FILE *fp;
	unsigned int reboot_flag=0;
	char cmd[128] = {0}, cmd2[128] = {0};
	int timeSync=0;
	time_t rtime = 0;

	if ((fp = fopen(REBOOT_LOG, "r")) == NULL)
		return -1;//no this file

	fscanf(fp, "%d %lu", &reboot_flag, &rtime);
	fprintf(stderr, "[%s%d] flag 0x%x, reboot flag 0x%x\n", __FUNCTION__, __LINE__, *flag, reboot_flag);
	fclose(fp);
/*	
	if(checkTimeIsUpdate(&rtime) != 0)
	{
		//need wait NTP sync time
		return -2;
	}
*/	
	if(0 == (REBOOT_FLAG&reboot_flag) ){
		fprintf(stderr, " no  REBOOT_FLAG \n");
		return -1;//not reboot
	}

	sprintf(cmd, "cp %s /tmp/rebootinfo", REBOOT_LOG);
	fprintf(stderr,"[%s]\n", cmd);

	sprintf(cmd2, "cat %s", REBOOT_LOG);
	system(cmd2);

	system(cmd);

	unlink(REBOOT_LOG);
	
	if(flag) *flag = reboot_flag;
	fprintf(stderr, "[%s%d] flag 0x%x, reboot flag 0x%x\n", __FUNCTION__, __LINE__, *flag, reboot_flag);
	if(timep) *timep = rtime;
	
	return 0;
}


void get_reboot_log(void)
{
	FILE *fp, *fp2;
	char *msg;
	int c = 0;
	int count = 0;
	char mystring [128] = {0};

	msg = malloc(sizeof(char)*8192);
	fp = fopen("/proc/crashlog", "r");
	fp2 = fopen("/tmp/crashlog", "wb");
	if ( fp )
	{
		fprintf(stderr,"[%s][%d]\n", __FUNCTION__, __LINE__);
		while( fgets (mystring , 100 , fp) != NULL ){
			fprintf(fp2, "%s",mystring);
		}
		fprintf(stderr,"[%s][%d]\n", __FUNCTION__, __LINE__);
		fclose(fp);
	}
	fclose(fp2);
	free(msg);
}

/*********************************************************
  *	Return:
  *  =0  not reboot
  *  >0  has been rebooted
  ********************************************************/
int get_reboot_info(void)
{
	unsigned int reboot_flag = 0;
	time_t timep = 0;
	struct tm *p;
	int ret;

	if( (NULL==rebootTimeStr) || (NULL==rebootSrc) )
		return 0;
	
	if((ret = reboot_log(&reboot_flag, &timep))<0){
		fprintf(stderr, " No reboot log\n");
		return 0;//not reboot
	}

	fprintf(stderr, " reboot reason is 0x%x, time is %lu\n", reboot_flag, timep);

#if 0	
	reboot_flag &= (~REBOOT_FLAG);

	OMD_DEBUG(" reboot_flag = %x, time = %lu\n", reboot_flag, timep);

	switch (reboot_flag)
	{
		case ITMS_REBOOT:
			strcpy(rebootSrc, "ITMS");
			break;

		case TELECOM_WEB_REBOOT:
			strcpy(rebootSrc, "Telecomadmin");
			break;

		case DBUS_REBOOT:
			strcpy(rebootSrc, "DBus");
			break;

		case TERMINAL_REBOOT:
			strcpy(rebootSrc, "Terminal");
			break;
			
		case POWER_REBOOT:
			strcpy(rebootSrc, "Power");
			break;
			
		case EXCEP_REBOOT:
			strcpy(rebootSrc, "Exception");
			break;

		default:
			*rebootSrc = '\0';
			OMD_DEBUG("No need create reboot report !!!\n");
			return 0;//not reboot
			
	}
	
	/* get time  */
	
	if(timep == 0) time(&timep);			
	p = localtime(&timep);
	sprintf(rebootTimeStr, "%d-%02d-%02d %02d:%02d:%02d", 1900+p->tm_year, 1+p->tm_mon,p->tm_mday,  p->tm_hour, p->tm_min, p->tm_sec);		

	OMD_DEBUG("Has been rebooted, reboot reason is %s, time is %s\n", rebootSrc, rebootTimeStr);
	printf("[OMD] Has been rebooted, reboot reason is %s, time is %s\n", rebootSrc, rebootTimeStr);
#endif	
	return 1;//Has been rebooted
}


void updateReboottimes(void)
{
	FILE *fp_rbtime;
	int reboottime_fd = 0;
	unsigned int reboottimes = 0;

	fp_rbtime = fopen(REBOOT_TIMES, "rb");
	if ( fp_rbtime ){
		fscanf(fp_rbtime, "%d", &reboottimes);
		fprintf(stderr, "[%s][%d]  reboottimes %u\n",  __func__, __LINE__, reboottimes);
		fclose(fp_rbtime);
		fp_rbtime = fopen(REBOOT_TIMES, "wb");
		reboottimes += 1;
		fprintf(stderr, "[%s][%d]  reboottimes %u\n",  __func__, __LINE__, reboottimes);					
		fprintf(fp_rbtime, "%d", reboottimes);		
		reboottime_fd = fileno(fp_rbtime);
		fsync(reboottime_fd);					
		fclose(fp_rbtime);
	}else{
		fprintf(stderr,"open %s fail, write initial", REBOOT_TIMES);
		fp_rbtime = fopen(REBOOT_TIMES, "wb+");
		fprintf(fp_rbtime, "%d", reboottimes);		
		reboottime_fd = fileno(fp_rbtime);
		fsync(reboottime_fd);					
		fclose(fp_rbtime);			fprintf(stderr,"open %s fail, write initial", REBOOT_TIMES);
		fp_rbtime = fopen(REBOOT_TIMES, "wb+");
		fprintf(fp_rbtime, "%d", reboottimes);		
		reboottime_fd = fileno(fp_rbtime);
		fsync(reboottime_fd);					
		fclose(fp_rbtime);
	}

}

int write_omd_reboot_log(unsigned int flag)
{
	FILE *fp;
	int fd = 0, hasExcep = 0;
	unsigned int reboot_flag = 0;
	time_t rtime = 0, record_flag = 0;
	fprintf(stderr, "[%s%d] flag 0x%x\n", __FUNCTION__, __LINE__, flag);
	if (flag != REBOOT_FLAG)
	{ 
		if(access(REBOOT_LOG, F_OK) != 0)
		{
			fp = fopen(REBOOT_LOG, "w+");

			if(fp){
				time(&rtime);
				fprintf(fp, "%d %lu", flag, rtime);
				fprintf(stderr, "[%s %d]: no file,flag = %x , time = %lu\n", __func__, __LINE__, flag, rtime);
				fd = fileno(fp);
				fsync(fd);
				fclose(fp);				
				updateReboottimes();
			}
		}
	}
	else 
	{	
		if(is_watchdog_reboot()) hasExcep |= 1;
		fprintf(stderr, "hasExcep\n");
		if((hasExcep || access(REBOOT_LOG, F_OK) != 0) && (fp = fopen(REBOOT_LOG, "w+")) != NULL)
		{
			fscanf(fp, "%d %lu", &record_flag, &rtime);
			time(&rtime);
			if(hasExcep){
				flag |= EXCEP_REBOOT;
			}else flag |= POWER_REBOOT;
			fprintf(fp, "%d %lu", flag, rtime);
			printf("[%s %d]: flag = %x, time = %lu\n", __func__, __LINE__, flag, rtime);
			fd = fileno(fp);
			fsync(fd);
			fclose(fp);			
			updateReboottimes();
		}
		else 
		{
			fp=fopen(REBOOT_LOG, "r+");
			if(fp)
			{
				fscanf(fp, "%d %lu", &reboot_flag, &rtime);
				fseek(fp, 0, SEEK_SET);
				flag |= reboot_flag;
				fprintf(fp, "%d %lu", flag, rtime);
				printf("[%s %d]: have file,reboot_flag =%d, flag = %x, time = %lu\n", __func__, __LINE__, reboot_flag, flag, rtime);
				fd = fileno(fp);
				fsync(fd);				
				fclose(fp);				
				updateReboottimes();
			}
		}
	}
	
	return 1;
}


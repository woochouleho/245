
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <paths.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/param.h>
#ifdef EMBED
#include <config/autoconf.h>
#include "rtk/options.h"
#else
#include "../../../../config/autoconf.h"
#endif
#include "utility.h"
#include <sys/sysinfo.h>

/* SYSLOG_NAMES defined to pull some extra junk from syslog.h */
#ifndef SYSLOG_NAMES
#define SYSLOG_NAMES
#endif
#include <sys/syslog.h>
#include <sys/uio.h>
#include <sys/stat.h>

#ifndef FALSE
#define FALSE   ((int) 0)
#endif
#ifndef TRUE
#define TRUE    ((int) 1)
#endif
/* Path for the file where all log messages are written */
#define __LOG_FILE "/var/log/messages"

/* Path to the unix socket */
static char lfile[MAXPATHLEN];

static char *logFilePath = __LOG_FILE;

#ifdef BB_FEATURE_SAVE_LOG
#include <sys/stat.h>
#include "mib.h"
static int save_logging = FALSE;
static int save_loop_files = FALSE;
static unsigned int recordnum = 0;
static unsigned int totalnum = 0;
static unsigned int offset = 0;
#define WRITE_FALSH_INTERVAL  10
#ifdef CONFIG_CU
#define MAX_LOG_SIZE 500 * 1024
#define FIX_LOG_SIZE  50 * 1024
#else
#define MAX_LOG_SIZE 64 * 1024
#define FIX_LOG_SIZE  10 * 1024
#endif
#define SYSLOGDFILE_SAVE "/var/config/syslogd.txt"
#define SYSLOGDFILE "/var/run/syslogd.txt"
#define TEMPLOGFILE "/var/logtmp.txt"
#define TEMPLOGLINEFILE "/var/logtmpLine.txt"
#ifdef SUPPORT_LOG_TO_TEMP_FILE
#define SYSLOGDTEMPFILE "/var/syslogd.txt"
#endif
#endif

#ifdef EMBED
static int logFileMaxSize = 16384;
static int logFileMaxLine = 500;
#endif

static int logLevel = LOG_DEBUG;
/* interval between marks in seconds */
#ifdef EMBED
static int MarkInterval = 0;
#else
static int MarkInterval = 20 * 60;
#endif

/* localhost's name */
// Kaohj
#ifndef EMBED
static char LocalHostName[64];
#endif

#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
#include <netinet/in.h>

struct RemoteLogInfo {
	int fd;					/* udp socket for logging to remote host */
	char *hostname;			/* where do we log? */
	int port;				/* what port to log to? */
	int filterlevel;		/* (7 - LOG_xxx) only logs messages at priority LOG_xxx and higher.
	                         * 0 = log all. 8 = log none
							 */
	int enable;
};

#ifdef CONFIG_USER_MGMT_MGMT
#define NUM_REMOTE_HOSTS 2
#else
#define NUM_REMOTE_HOSTS 1
#endif

static struct RemoteLogInfo remote_log_info[NUM_REMOTE_HOSTS] = {
  { -1, NULL, 514, LOG_DEBUG, 0 }
};

static struct sockaddr_in log_remoteaddr[NUM_REMOTE_HOSTS];
static int remote_logging_initialised = FALSE;
static int remote_logging = FALSE;
static int local_logging = FALSE;
static int init_RemoteLog(void);
char *log_severity[] =
{
	"Emergency",
	"Alert",
	"Critical",
	"Error",
	"Warning",
	"Notice",
	"Informational",
	"Debug"    
};
static char *month[] =
{
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nev",
	"Dec"
};
#endif

#define MAXLINE         1024            /* maximum line length */

/* try to open up the specified device */
int device_open(char *device, int mode)
{
	int m, f, fd = -1;

	m = mode | O_NONBLOCK;

	/* Retry up to 5 times */
	for (f = 0; f < 5; f++)
		if ((fd = open(device, m, 0600)) >= 0)
			break;
	if (fd < 0)
		return fd;
	/* Reset original flags. */
	if (m != mode) {
		if(fcntl(fd, F_SETFL, mode) == -1)
			printf("fcntl failed: %s %d\n", __func__, __LINE__);
	}
	return fd;
}

void perror_msg_and_die(const char *s, ...)
{
	va_list p;
	int err = errno;

	va_start(p, s);
	//vperror_msg(s, p);
	if (s==0) s="";
	fflush(stdout);
	vfprintf(stderr, s, p);
	if (*s) s = ": ";
	fprintf(stderr, "%s%s\n", s, strerror(err));
	va_end(p);
	exit(EXIT_FAILURE);
}

#ifdef BB_FEATURE_SAVE_LOG
int fixupLogfile(FILE* fp,int size)
{
	int pos = fseek(fp, 0, SEEK_END);
	int filelength = ftell(fp);
	int totallength = 0;
	int ret = -1;
	char buf[1024] = {0};
	if (filelength < size)
		return 0;
	ret = fseek(fp, size, SEEK_SET);
	if(ret)
		printf("fseek error!\n");
	//abbreviate one line
	fgets(buf, sizeof(buf), fp);
	totallength = size + sizeof(buf);
	FILE *tmp_fp = fopen(TEMPLOGFILE, "w+b");
	if(tmp_fp == NULL)
	{
		printf("[%s:%d] ERROR: fopen %s fail\n", __FUNCTION__, __LINE__,TEMPLOGLINEFILE);
		return -1;
	}

	/* write log file header */
	writeLogFileHeader(tmp_fp);
	while (tmp_fp && !feof(fp)) {
		memset(buf, 0, sizeof(buf));
		fgets(buf, sizeof(buf), fp);
		fprintf(tmp_fp, "%s", buf);
	}
	fflush(tmp_fp);
	int fd = fileno(fp);
	if (ftruncate(fd,0) < 0)
		perror("ftruncate error!\n");
	fflush(fp);
	fseek(tmp_fp, 0, SEEK_SET);
	while (!feof(tmp_fp)) {
		memset(buf, 0, sizeof(buf));
		fgets(buf, sizeof(buf), tmp_fp);
		fprintf(fp, "%s", buf);
	}
	fflush(fp);
	if (tmp_fp)
		fclose(tmp_fp);
	ret=remove(TEMPLOGFILE);
	if(ret)
		printf("remove error!\n");
	return 0;
}
#endif

static void message (char *fmt, ...)
{
	int fd;
	struct flock fl;
	va_list arguments;

	fl.l_whence = SEEK_SET;
	fl.l_start  = 0;
	fl.l_len    = 1;

	if ((fd = device_open (logFilePath,
						   O_WRONLY | O_CREAT | O_NOCTTY | O_APPEND |
						   O_NONBLOCK)) >= 0) {
		fl.l_type = F_WRLCK;
		if(fcntl (fd, F_SETLKW, &fl) == -1)
			printf("fcntl failed: %s %d\n", __func__, __LINE__);
		va_start (arguments, fmt);
		vdprintf (fd, fmt, arguments);
		va_end (arguments);
		fl.l_type = F_UNLCK;
		if(fcntl (fd, F_SETLKW, &fl) == -1)
			printf("fcntl failed: %s %d\n", __func__, __LINE__);
#ifdef EMBED
		{
			struct stat st;
			char buf[128];

			if (fstat(fd, &st) != -1 && st.st_size >= logFileMaxSize) {
				snprintf(buf, sizeof(buf), "%s.old", logFilePath);
				if(rename(logFilePath, buf) != 0)
					printf("Unable to rename the file: %s %d\n", __func__, __LINE__);
			}
		}
#endif
		close (fd);
	} else {
		/* Always send console messages to /dev/console so people will see them. */
		if ((fd = device_open (_PATH_CONSOLE,
							   O_WRONLY | O_NOCTTY | O_NONBLOCK)) >= 0) {
			va_start (arguments, fmt);
			vdprintf (fd, fmt, arguments);
			va_end (arguments);
			close (fd);
		} else {
			fprintf (stderr, "Bummer, can't print: ");
			va_start (arguments, fmt);
			vfprintf (stderr, fmt, arguments);
			fflush (stderr);
			va_end (arguments);
		}
	}
}
#ifdef BB_FEATURE_SAVE_LOG
static int rtk_util_get_file_lines(char* path);

static int rtk_util_get_file_lines(char* path)
{
	FILE *fp = fopen(path, "r");
    int ch;
    int count=0;
	if(fp==NULL)
	{
		printf("[%s:%d] ERROR: fopen %s fail\n", __FUNCTION__, __LINE__,path);
		return count;
	}
    do
    {
        ch = fgetc(fp);
        if(ch == '\n') count++;   
    } while( ch != EOF );
	fclose(fp);
	return count;
}

int fixupLogfileLines(FILE* fp,int line);
int fixupLogfileLines(FILE* fp,int line)
{
	//printf("%s:%d line=%d\n", __FUNCTION__, __LINE__, line);
	int filelines = rtk_util_get_file_lines(SYSLOGDFILE);
	if(filelines < line)
		return 0;
	
	FILE *tmp_fp = fopen(TEMPLOGLINEFILE, "w+b");
	char cmd[512];
	char buf[1024] = {0};
	int retry=0;
	int ret=-1;

	if(tmp_fp == NULL)
	{
		printf("[%s:%d] ERROR: fopen %s fail\n", __FUNCTION__, __LINE__,TEMPLOGLINEFILE);
		return -1;
	}
	writeLogFileHeader(tmp_fp);
	
	snprintf(cmd, sizeof(cmd), "/bin/tail -n %d %s", line, SYSLOGDFILE);
	//printf("%s:%d cmd=%s\n", __FUNCTION__, __LINE__, cmd);
	FILE *pp = popen(cmd, "r");
	if(!pp){
		printf("[%s:%d] ERROR: get lines fail\n", __FUNCTION__, __LINE__);
		if (tmp_fp)
			fclose(tmp_fp);
		return -1;
	}
	memset(buf, 0, sizeof(buf));
    while (fgets(buf, sizeof(buf), pp) != NULL) {
        fprintf(tmp_fp, "%s", buf);
		memset(buf, 0, sizeof(buf));
    }
	pclose(pp);

	int fd = fileno(fp);
	if (ftruncate(fd,0) < 0)
		perror("ftruncate error!\n");
	fflush(fp);

	fseek(tmp_fp, 0, SEEK_SET);
	while (!feof(tmp_fp)) {
		memset(buf, 0, sizeof(buf));
		fgets(buf, sizeof(buf), tmp_fp);
		fprintf(fp, "%s", buf);
	}
	fflush(fp);
	if (tmp_fp)
		fclose(tmp_fp);
	ret=remove(TEMPLOGLINEFILE);
	if(ret)
		printf("remove error!\n");
	return 0;
}
#endif
static void logMessage(int pri, char *msg)
{
#ifdef CONFIG_TELMEX_DEV
	char ts[32];
	time_t tm;
#endif
	char *timestamp;
	static char logpri[20];
	CODE *c_pri, *c_fac;
	int i=0;
	char filename_old[64]={0}, filename_new[64]={0};
#if defined(TIME_ZONE)  && defined(CONFIG_TRUE)
	static int ntp_syced=0;
#endif

	if (pri != 0) {
		for (c_fac = facilitynames;
			c_fac->c_name && !(c_fac->c_val == LOG_FAC(pri) << 3); c_fac++);
		for (c_pri = prioritynames;
			c_pri->c_name && !(c_pri->c_val == LOG_PRI(pri)); c_pri++);
		if (c_fac->c_name == NULL || c_pri->c_name == NULL)
			snprintf(logpri, sizeof(logpri), "<%d>", pri);
		else
			snprintf(logpri, sizeof(logpri), "%s.%s", c_fac->c_name, c_pri->c_name);
	}

	/* Jan  6 20:31:51 kernel: br0: port 2(vc0) entering disabled state */
#ifdef CONFIG_TELMEX_DEV
	tm = time(NULL);
	strftime(ts, sizeof(ts), "%F %T", localtime(&tm));
	timestamp = ts;
#else
	timestamp = msg;
#endif
	msg[15] = '\0';
	msg += 16;

	/* todo: supress duplicates */

#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
	/* send message to remote logger */
	if (!remote_logging_initialised) {
		if (init_RemoteLog() == 0)
			remote_logging_initialised = TRUE;
	}

	if (remote_logging) {
		char *log_buf;
		int i;

		i = strlen(msg) + 8;
		log_buf = malloc(i);
		snprintf(log_buf, i, "<%d> %s", pri, msg);

		for (i = 0; i < NUM_REMOTE_HOSTS; i++) {
			if (!remote_log_info[i].enable || remote_log_info[i].fd == -1) {
				continue;
			}

			if (7 - LOG_PRI(pri) < remote_log_info[i].filterlevel) {
				continue;
			}
writev_retry:
			if (-1 == sendto(remote_log_info[i].fd, log_buf, 
			  strlen(log_buf), 0, 
			  (struct sockaddr *)&(log_remoteaddr[i]), 
			  sizeof(struct sockaddr_in))) {
				if (errno == EINTR) goto writev_retry;
				message("%d|%s|%s|slogd: cannot write to remote file handle on " 
						"%s:%d - %d\n", pri, timestamp, logpri, remote_log_info[i].hostname, remote_log_info[i].port, errno);
				close(remote_log_info[i].fd);
				remote_log_info[i].fd = -1;
				remote_logging_initialised = FALSE;
			}
		}

		free(log_buf);
	}

	if (local_logging == TRUE)
#endif
	{
		/* now spew out the message to wherever it is supposed to go */
#ifdef EMBED
		message("%d|%s|%s|%s\n", pri, timestamp, logpri, msg);
#else
		// Kaohj
		message("%s %s %s %s\n", timestamp, LocalHostName, logpri, msg);
#endif
	}

#ifdef BB_FEATURE_SAVE_LOG
	if (save_logging == TRUE) {
		FILE* fp;

		/* Tsai: test whether SYSLOGDFILE exists or not */
		if ((fp = fopen(SYSLOGDFILE, "rb")) == NULL) {
			//  printf("file %s doesn't exist!\n",SYSLOGDFILE);
			if ((fp = fopen(SYSLOGDFILE, "wb")) == NULL) {
				printf("BUG:Can't create file %s!\n", SYSLOGDFILE);
				exit(0);
			}
			else {
				/* write log file header */
				writeLogFileHeader(fp);
			}
		}
		fclose(fp);

		if (fp = fopen(SYSLOGDFILE, "a+b")) {
			fseek (fp, 0, SEEK_END);
			int filelen = ftell(fp);
			/* format of time stamp of CTC: YYYY-MM-DD HH:MM:SS */
			char ts[32];
			int filelines = rtk_util_get_file_lines(SYSLOGDFILE);
			time_t tm;

			/* memset(ts, 0, sizeof(ts)); */
#if defined(TIME_ZONE) && defined(CONFIG_TRUE)
			if(ntp_syced || (ntp_syced=isTimeSynchronized()))
			{
				tm = time(NULL);
				strftime(ts, sizeof(ts), "%FT%TZ", localtime(&tm));
			}
			else
			{
				struct sysinfo info;
				long sysup_time=0;
				unsigned char year,month,day,hour,minute,second;

				sysinfo(&info);
				sysup_time = info.uptime;
				year = sysup_time/(365*24*3600);
				sysup_time -= (365*24*3600)*year;
				month = sysup_time/(31*24*3600);//month just consider for 31 days
				sysup_time -= (31*24*3600)*month;
				day = sysup_time/(24*3600);
				sysup_time -= (24*3600)*day;
				hour = sysup_time/(3600);
				sysup_time -= 3600*hour;
				minute = sysup_time/60;
				sysup_time -= 60*minute;
				second = sysup_time;
				sprintf(ts, "P%04u-%02u-%02uT%02u:%02u:%02u",year,month,day,hour,minute,second);
			}
#else
			tm = time(NULL);
			strftime(ts, sizeof(ts), "%F %T", localtime(&tm));
#endif
			if (save_loop_files) {
				if ((filelen > (logFileMaxSize/save_loop_files)) || (filelines > (logFileMaxLine/save_loop_files))){
					fclose(fp);
					for(i = save_loop_files; i>0; i--)
					{
						sprintf(filename_old, "%s_%d", SYSLOGDFILE, i);
						if (i == 1)
							sprintf(filename_new, "%s", SYSLOGDFILE);
						else
							sprintf(filename_new, "%s_%d", SYSLOGDFILE, (i-1));
						if( access( filename_old, F_OK ) != -1 ) unlink(filename_old);
						va_cmd("/bin/mv", 2, 1, (char *)filename_new, (char *)filename_old);
					}
					if( access( SYSLOGDFILE, F_OK ) != 0 ) {
						if ((fp = fopen(SYSLOGDFILE, "wb")) == NULL) {
							printf("BUG:Can't create file %s!\n", SYSLOGDFILE);
							exit(0);
						}
					}
				}
			}
			else {
				if (filelen > logFileMaxSize) {
					fseek(fp, 0, SEEK_SET);
					fixupLogfile(fp, FIX_LOG_SIZE);
					fseek(fp, 0, SEEK_END);
				}
				//printf("%s:%d filelines %d, logFileMaxLine %d\n", __FUNCTION__, __LINE__, filelines, logFileMaxLine);
				if(filelines > logFileMaxLine) {
					fseek(fp, 0, SEEK_SET);
					fixupLogfileLines(fp, logFileMaxLine/2);
					fseek(fp, 0, SEEK_END);
				}
			}
#ifdef EMBED
#ifdef CONFIG_E8B
			fprintf(fp, "%s [%s] %s\n", ts, log_severity[LOG_PRI(pri)], msg);
#else
			//message("%d|%s|%s|%s\n", pri, timestamp, logpri, msg);
			fprintf(fp, "%d|%s|%s|%s\n", pri, ts, logpri, msg);
#endif
#else
			// Kaohj
			fprintf(fp, "%s %s %s %s\n", ts, LocalHostName, logpri, msg);
#endif
			recordnum++;
			fclose(fp);
		}
	}
#ifdef SUPPORT_LOG_TO_TEMP_FILE
	else{
		FILE* fp;
	
		/* test whether SYSLOGDFILE exists or not */
		if (access(SYSLOGDTEMPFILE, W_OK)) {
			if (fp = fopen(SYSLOGDTEMPFILE, "w")) {
				writeLogFileHeader(fp);
				fclose(fp);
			} else
				return;
		}
	
		if (fp = fopen(SYSLOGDTEMPFILE, "a+")) {
			fseek (fp, 0, SEEK_END);
			int filelen = ftell(fp);
			/* format of time stamp of CTC: YYYY-MM-DD HH:MM:SS */
			char ts[32];
			time_t tm;
	
			/* memset(ts, 0, sizeof(ts)); */
	
			tm = time(NULL);
			strftime(ts, sizeof(ts), "%F %T", localtime(&tm));
	
			if (filelen > MAX_LOG_SIZE) {
				fseek(fp, 0, SEEK_SET);
				fixupLogfile(fp, FIX_LOG_SIZE);
				fseek(fp, 0, SEEK_END);
			}
			fprintf(fp, "%s [%s] %s\n", ts, 
				log_severity[LOG_PRI(pri)], msg);
			recordnum++;
			fclose(fp);
		}
	}
#endif
#endif
}

// Kaohj -- write pid file
#ifdef EMBED
static char syslogd_pidfile[] = "/var/run/syslogd.pid";
static void log_pid()
{
	FILE *f;
	pid_t pid;
	char *pidfile = syslogd_pidfile;

	pid = getpid();
	if((f = fopen(pidfile, "w")) == NULL)
		return;
	fprintf(f, "%d\n", pid);
	fclose(f);
}
#endif

static void quit_signal(int sig)
{
	syslog(LOG_SYSLOG | LOG_INFO, "System log daemon exiting.");
	unlink(lfile);
	unlink(SYSLOGDFILE_SAVE);
	va_cmd("/bin/cp", 2, 1, (char *)SYSLOGDFILE,  (char *)SYSLOGDFILE_SAVE);
#ifdef EMBED
	unlink(syslogd_pidfile);
#endif
	exit(TRUE);
}

#ifdef DELAY_WRITE_SYSLOG_TO_FLASH
static void signal_handler(int sig)
{
	forceWriteFlash = 1;
}
#endif

static void domark(int sig)
{
	if (MarkInterval > 0) {
		syslog(LOG_SYSLOG | LOG_INFO, "-- MARK --");
		alarm(MarkInterval);
	}
}

/* This must be a #define, since when DODEBUG and BUFFERS_GO_IN_BSS are
 * enabled, we otherwise get a "storage size isn't constant error. */
static int serveConnection (char* tmpbuf, int n_read)
{
	int    pri_set = 0;
	char  *p = tmpbuf;

	/* SOCK_DGRAM messages do not have the terminating NUL,  add it */
	if (n_read > 0)
		tmpbuf[n_read] = '\0';

	while (p < tmpbuf + n_read) {

		int           pri = (LOG_USER | LOG_NOTICE);
		char          line[ MAXLINE + 1 ];
		char         *q = line;

		while (q < &line[ sizeof (line) - 1 ]) {
			if (!pri_set && *p == '<') {
			/* Parse the magic priority number. */
				pri = 0;
				while (isdigit (*(++p))) {
					pri = 10 * pri + (*p - '0');
				}
				if((pri&0x07) > logLevel)
					return n_read;
				if (pri & ~(LOG_FACMASK | LOG_PRIMASK)){
					pri = (LOG_USER | LOG_NOTICE);
				}
				pri_set = 1;
			} else if (*p == '\0') {
				pri_set = 0;
				*q = *p++;
				break;
			} else if (*p == '\n') {
				*q++ = ' ';
			} else if (iscntrl(*p) && (*p < 0177)) {
				*q++ = '^';
				*q++ = *p ^ 0100;
			} else {
				*q++ = *p;
			}
			p++;
		}
		*q = '\0';
		p++;
		/* Now log it */
		if (q > line)
			logMessage(pri, line);
	}
	return n_read;
}

#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
static void verror_msg(const char *s, va_list p)
{
	fflush(stdout);
	fprintf(stderr, "slogd: ");
	vfprintf(stderr, s, p);
}

static void herror_msg_and_die(const char *s, ...)
{
	va_list p;

	va_start(p, s);
	if (s == 0)
		s = "";
	verror_msg(s, p);
	if (*s)
		fputs(": ", stderr);
	herror("");
	va_end(p);
	exit(EXIT_FAILURE);
}

static struct hostent *xgethostbyname(const char *name)
{
	struct hostent *retval;

	if ((retval = gethostbyname(name)) == NULL)
		herror_msg_and_die("%s", name);

	return retval;
}

static void error_msg_and_die(const char *s, ...)
{
	va_list p;

	va_start(p, s);
	verror_msg(s, p);
	va_end(p);
	putc('\n', stderr);
	exit(EXIT_FAILURE);
}

static int init_RemoteLog(void)
{
	int i;
	struct hostent *hostinfo;

	remote_logging = FALSE;

	for (i = 0; i < NUM_REMOTE_HOSTS; i++) {
		if (!remote_log_info[i].enable || remote_log_info[i].hostname == NULL ||
			remote_log_info[i].port == 0 || remote_log_info[i].filterlevel > 7) {
			continue;
		}

		remote_log_info[i].fd = socket(AF_INET, SOCK_DGRAM, 0);

		if (remote_log_info[i].fd < 0) {
			error_msg_and_die("slogd: cannot create socket");
		}

		hostinfo = xgethostbyname(remote_log_info[i].hostname);
		log_remoteaddr[i].sin_family = AF_INET;
		log_remoteaddr[i].sin_addr = *(struct in_addr *)*hostinfo->h_addr_list;
		log_remoteaddr[i].sin_port = htons(remote_log_info[i].port);
	}

	remote_logging = TRUE;

	return 0;
}
#endif

static void doSyslogd (void)
{
	struct sockaddr_un sunx;
	socklen_t addrLength;

	int sock_fd;
	fd_set fds;
	char buf[128];

	/* Set up signal handlers. */
	signal (SIGINT,  quit_signal);
	signal (SIGTERM, quit_signal);
	signal (SIGQUIT, quit_signal);
	signal (SIGHUP,  SIG_IGN);
	signal (SIGCHLD,  SIG_IGN);
#ifdef SIGCLD
	signal (SIGCLD,  SIG_IGN);
#endif
	signal (SIGALRM, domark);
#ifdef DELAY_WRITE_SYSLOG_TO_FLASH
	signal (SIGUSR1, signal_handler);
#endif
	alarm (MarkInterval);

	/* Create the syslog file so realpath() can work. */
	if (realpath (_PATH_LOG, lfile) != NULL)
		unlink (lfile);

	memset (&sunx, 0, sizeof (sunx));
	sunx.sun_family = AF_UNIX;
	strncpy (sunx.sun_path, lfile, sizeof (sunx.sun_path));
	if ((sock_fd = socket (AF_UNIX, SOCK_DGRAM, 0)) < 0)
		perror_msg_and_die ("Couldn't get file descriptor for socket " _PATH_LOG);

	addrLength = sizeof (sunx.sun_family) + strlen (sunx.sun_path);
	if (bind(sock_fd, (struct sockaddr *) &sunx, addrLength) < 0)
		perror_msg_and_die ("Could not connect to socket " _PATH_LOG);

	if (chmod (lfile, 0666) < 0)
		perror_msg_and_die ("Could not set permission on " _PATH_LOG);



#ifdef BB_FEATURE_SAVE_LOG
	static int time_select = 0;
#endif

	for (;;) {

#ifdef BB_FEATURE_SAVE_LOG
	RETRY:
		FD_ZERO (&fds);
		FD_SET (sock_fd, &fds);

		struct timeval tv;
		tv.tv_sec = 20;
		tv.tv_usec = 0;
		int ret = 0;
		ret = select(sock_fd+1, &fds, NULL, NULL, &tv);
		if (ret == 0) {	//timeout
			time_select ++;
			if ((time_select >= 3 && recordnum > 0) || recordnum >= WRITE_FALSH_INTERVAL) {
#ifdef CONFIG_USER_FLATFSD_XXX
				printf("/bin/flatfsd   -s  \n");
				system("/bin/flatfsd  -s");
#endif
				recordnum = 0;
				time_select = 0;
			}
			goto RETRY;
		}
		else if (ret < 0)
#else
		FD_ZERO (&fds);
		FD_SET (sock_fd, &fds);

		if (select(sock_fd+1, &fds, NULL, NULL, NULL) < 0)
#endif
		{
			if (errno == EINTR) {
				/* alarm may have happened. */
				continue;
			}
			perror_msg_and_die ("select error");
		}

		if (FD_ISSET (sock_fd, &fds)) {
		       int   i;
		       // Kaohj
		       //RESERVE_BB_BUFFER(tmpbuf, BUFSIZ + 1);
		       char tmpbuf[BUFSIZ + 1];

		       memset(tmpbuf, '\0', BUFSIZ+1);
		       if ( (i = recv(sock_fd, tmpbuf, BUFSIZ, 0)) > 0) {
			       serveConnection(tmpbuf, i);
		       } else {
			       perror_msg_and_die ("UNIX socket error");
		       }
		       // Kaohj
		       //RELEASE_BB_BUFFER (tmpbuf);
		}/* FD_ISSET() */
	} /* for main loop */
}

int main(int argc, char **argv)
{
	int opt;
#if ! defined(__uClinux__)
	int doFork = TRUE;
#endif
	char *p;

	/* do normal option parsing */
	//while ((opt = getopt(argc, argv, "s:m:nO:R:LC")) > 0) {
#ifdef EMBED
	while ((opt = getopt(argc, argv, "s:N:m:l:nO:R:r:f:LCw")) > 0)
#else
	while ((opt = getopt(argc, argv, "s:m:l:nO:R:r:f:LC")) > 0)
#endif
	{
		switch (opt) {
			case 's':
				logFileMaxSize=atoi(optarg);
				break;
			case 'N':
				logFileMaxLine=atoi(optarg);
				break;
			case 'm':
				MarkInterval = atoi(optarg) * 60;
				break;
			case 'l':
				logLevel = atoi(optarg);
				break;
			case 'n':
#if ! defined(__uClinux__)
				doFork = FALSE;
#endif
				break;
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
			case 'R':
				remote_log_info[0].hostname = strdup(optarg);
				if ((p = strchr(remote_log_info[0].hostname, ':'))) {
					remote_log_info[0].port = atoi(p + 1);
					*p = '\0';
				}
				remote_log_info[0].enable = 1;
#ifndef CONFIG_00R0
				remote_log_info[0].filterlevel = 7 - logLevel;
#endif
				remote_logging = TRUE;
				break;
			case 'r':
				remote_log_info[0].filterlevel = 7 - atoi(optarg);
				break;
			case 'L':
				local_logging = TRUE; 
				break;
#endif
//add by ramen 2008-02-19
#ifdef BB_FEATURE_SAVE_LOG
			case 'w':
				save_logging = TRUE;
				break;
			case 'f':
				/*  save log file into different files [num = save_loop_files] ,
				remove the oldest one when number exceed the max number */
				save_loop_files=atoi(optarg);
				break;
#endif
			// Kaohj
			/*
			case 'O':
				logFilePath = xstrdup(optarg);
				break;
			default:
				show_usage();
			*/
		}
	}


	/* Store away localhost's name before the fork */
// Kaohj
#ifndef EMBED
	gethostname(LocalHostName, sizeof(LocalHostName));
	if ((p = strchr(LocalHostName, '.'))) {
		*p++ = '\0';
	}
#endif

	umask(0);

#if ! defined(__uClinux__)
	if (doFork == TRUE) {
		if (daemon(0, 1) < 0)
			perror_msg_and_die("daemon");
	}
#endif
#ifdef EMBED
	log_pid();
#endif

	if( access( SYSLOGDFILE_SAVE, F_OK ) != -1 )  {
		va_cmd("/bin/cp", 2, 1, (char *)SYSLOGDFILE_SAVE, (char *)SYSLOGDFILE);
	}
	doSyslogd();
	return EXIT_SUCCESS;
}



/*
 * configd.c --- main file for configuration server
 * --- By Kaohj
 */
#ifdef CONFIG_DEV_xDSL
#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#endif
#include <semaphore.h>
#include "../msgq.h"
#include "mib.h"	// for FLASH_BLOCK_SIZE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include "../syslog.h"
// Added by Mason Yu
#include "utility.h"
#ifdef CONFIG_CPU_BIG_ENDIAN
#include <linux/byteorder/big_endian.h>
#else
#include <linux/byteorder/little_endian.h>
#endif
#include <linux/magic.h>
#include <linux/cramfs_fs.h>

//ccwei
#if defined(CONFIG_MTD_NAND) || defined(CONFIG_RTL8686)
/* for open(), lseek() */
#include <mtd/mtd-user.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
//end ccwei
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/resource.h>

#include "debug.h"

#define CONFIGD_RUNFILE	"/var/run/configd.pid"
static int qid_snd, qid_rcv;
int shm_id; // shared memory ID
char *shm_start; // attaching address
int this_pid;

// Mason Yu On True
#ifdef CONFIG_USER_RTK_SYSLOG
#ifdef SEND_LOG
sem_t semSyslogC;
#define AUTO_STARTSYSLOGC_INTERVAL 60*60*24	/* start auto_startSYSLOGC every 24 hours */
#endif
#endif

char g_upload_post_file_name[MAX_SEND_SIZE];
int g_upload_startPos;
int g_upload_fileSize;
int g_upload_needreboot;
int g_upload_needcommit;
int g_upload_version;

static void log_pid()
{
	FILE *f;
	pid_t pid;
	char *pidfile = CONFIGD_RUNFILE;

	pid = getpid();
	this_pid = pid;

	if((f = fopen(pidfile, "w")) == NULL)
		return;
	fprintf(f, "%d\n", pid);
	fclose(f);
}

void sigterm_handler(int signal)
{
	remove_queue(qid_snd);
	remove_queue(qid_rcv);
	exit(0);
}

void init_signals(void)
{
	struct sigaction sa;

	sa.sa_flags = 0;

	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGSEGV);
	sigaddset(&sa.sa_mask, SIGBUS);
	sigaddset(&sa.sa_mask, SIGTERM);
	sigaddset(&sa.sa_mask, SIGHUP);
	sigaddset(&sa.sa_mask, SIGINT);
	sigaddset(&sa.sa_mask, SIGPIPE);
	sigaddset(&sa.sa_mask, SIGCHLD);
	sigaddset(&sa.sa_mask, SIGUSR1);

	sa.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);

	sa.sa_handler = sigterm_handler;
	sigaction(SIGTERM, &sa, NULL);
}

static int initMsgQ(void)
{
	key_t key;

	/* Create unique key via call to ftok() */
	key = ftok("/bin/init", 's');
	if ((qid_snd = open_queue(key, MQ_CREATE)) == -1) {
		perror("open_queue");
		return -1;
	}

	/* Create unique key via call to ftok() */
	key = ftok("/bin/init", 'r');
	if ((qid_rcv = open_queue(key, MQ_CREATE)) == -1) {
		perror("open_queue");
		return -1;
	}

#ifdef _SET_MSGQ_SIZE_AND_NONBLOCK_
	set_msgqueue_max_size(qid_snd, MAX_MSGQ_SIZE);
	set_msgqueue_max_size(qid_rcv, MAX_MSGQ_SIZE);
#endif //_SET_MSGQ_SIZE_AND_NONBLOCK_

	return 0;
}

/*
 *	Init shared memory segment; attach the shared memory segment to address space.
 *	-1: error
 *	 0: successful
 */
static int initShm()
{
#ifdef RESTRICT_PROCESS_PERMISSION
	if ((shm_id = shmget((key_t)1234, SHM_SIZE, 0666 | IPC_CREAT)) == -1) {
		perror("shmget");
		return -1;
	}
#else
	if ((shm_id = shmget((key_t)1234, SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
		perror("shmget");
		return -1;
	}
#endif
	if ((shm_start = (char *)shmat ( shm_id , NULL , 0 ) )==(char *)(-1)) {
		perror("shmat");
		return -1;
	}
	//printf("initShm: shm_id=%d, shm_start=0x%x\n", shm_id, shm_start);
	return 0;
}

#include <sys/syscall.h>
static inline int checkPidExist(int tid, int tgid)
{
	int ret;
	ret = syscall(SYS_tgkill, tgid, tid, 0);

//	printf("check pid %d, ret=%d\n",tid, ret);
	if((ret == -1) && (errno == ESRCH))
	{
		printf("tid %d not existed!\n",tid);
		return 0;
	}
	else
		return 1;
}

void msg_handler() {
	struct mymsgbuf qbuf;
	int ret;
	int isPidExisted;

	while (1) {
		ret = read_message(qid_snd, &qbuf, MSG_CONFIGD_PID);
		
		if (ret>0) {
			//printf("process message...\n");
			msgProcess(&qbuf);
			isPidExisted = checkPidExist(qbuf.mtype, qbuf.tgid);
			if (isPidExisted) { // Send the message only if the receiving process alive.
#ifdef _SET_MSGQ_SIZE_AND_NONBLOCK_
				send_message_nonblock(qid_rcv, qbuf.mtype, qbuf.request, 0, &qbuf);
#else
				send_message(qid_rcv, qbuf.mtype, qbuf.request, 0, &qbuf);
#endif //_SET_MSGQ_SIZE_AND_NONBLOCK_
			}
		}
	}
}

#ifndef CONFIG_LUNA_FIRMWARE_UPGRADE_SUPPORT
#if defined(CONFIG_RTL8686) || defined(CONFIG_MTD_NAND)
#include <sys/ioctl.h>
/*return value 0 --> success, -1 --> fail*/
static int get_mtd_meminfo(struct mtd_info_user *mtd_info,
			   const char *part_name)
{
	int fd, ret = 0;

	fd = get_mtd_fd(part_name);
	if (fd >= 0) {
		ret = ioctl(fd, MEMGETINFO, mtd_info);
		if (ret < 0) {
			fprintf(stderr, "error MEMGETINFO! ret %d\n", ret);
			goto fail;
		}
		printf("%s(%d): A block size = %d, A mtd size %d\n",
		       __FUNCTION__, __LINE__, mtd_info->erasesize,
		       mtd_info->size);
	} else {
		fprintf(stderr, "%s(%d):get_mtd_fd fails fd %d\n", __FUNCTION__,
			__LINE__, fd);
		ret = -1;
		goto fail;
	}

fail:
	if (fd >= 0)
		close(fd);

	return ret;
}

static int flash_block_erase(struct erase_info_user *erase_info,
			     char *part_name)
{
	int fd, ret = 0;

	fd = get_mtd_fd(part_name);
	if (fd >= 0) {
		ret = ioctl(fd, MEMERASE, erase_info);
		if (ret < 0) {
			fprintf(stderr, "error MEMERASE %d ret %d!\n",
				erase_info->start, ret);
			goto fail;
		}
	} else {
		fprintf(stderr, "%s(%d):get_mtd_fd fails fd %d\n", __FUNCTION__,
			__LINE__, fd);
		ret = -1;
		goto fail;
	}

fail:
	if (fd >= 0)
		close(fd);

	return ret;
}
#endif

#ifdef CONFIG_RTL8686
int flash_filewrite(FILE * fp, int size, const char *part_name)
{
	int block_count;
	int image_size;
	int nWritten = 0, nRead, ret = 0;
	struct erase_info_user erase_info;
	struct mtd_info_user mtd_info;
	unsigned int flash_pos_count = 0;
	void *block = NULL;

	if ((ret = get_mtd_meminfo(&mtd_info, part_name)) < 0)
		goto fail;

	block_count = (size + (mtd_info.erasesize - 1)) / mtd_info.erasesize;
	image_size = block_count * mtd_info.erasesize;
	printf("%s-(%d):: block_count:%d image_size:%d\n", __FUNCTION__,
	       __LINE__, block_count, image_size);

	block = malloc(mtd_info.erasesize);
	if (!block) {
		fprintf(stderr, "%s-%d::malloc mem fail!\n", __FUNCTION__,
			__LINE__);
		ret = -1;
		goto fail;
	}

	while (nWritten < size) {
		nRead = (mtd_info.erasesize >
			 (size - nWritten)) ? (size -
					       nWritten) : mtd_info.erasesize;
		nRead = fread(block, 1, nRead, fp);
		if (nRead < mtd_info.erasesize)
			nRead = mtd_info.erasesize;
		erase_info.start = flash_pos_count;
		erase_info.length = nRead;
		if (erase_info.start >= mtd_info.size) {
			fprintf
			    (stderr,
			     "Error: you can't erase the region exceed mtd partition size\n");
			ret = -1;
			goto fail;
		}

		if ((ret = flash_block_erase(&erase_info, (char *)part_name)) < 0)
			goto fail;

		ret = __mib_flash_part_write(block,
					     erase_info.start, nRead,
					     part_name);
		if (ret < 0) {	//write data fail!
			fprintf(stderr, "%s(%d): __mib_flash_part_write fail!\n",
				__FUNCTION__, __LINE__);
			goto fail;
		}

		flash_pos_count += mtd_info.erasesize;
		nWritten += nRead;
	}

fail:
	free(block);

	return ret;
}

//end 120313
#endif /*CONFIG_RTL8686 */

#ifdef CONFIG_MTD_NAND
#ifndef CONFIG_MTD_UBI
#include <rtl_flashdrv_api.h>
int flashdrv_filewrite(FILE * fp, int old_size, void *dstP)
{
	int block_count;
	int fd = -1, fp_pos = 0, flash_idx;
	int nWritten, nRead, ret = 0, size;
	unsigned int flash_pos_count;
#ifndef CONFIG_DOUBLE_IMAGE
	unsigned int backup = 0;
#endif
	void *block = NULL;
	uint32_t erasesize;

	if ((flash_idx = get_flash_index(dstP)) < 0) {
		fprintf(stderr, "get_flash_index fails!\n");
		ret = -1;
		goto fail;
	}

	erasesize = flash_mtd_info[flash_idx].erasesize;

	block_count = (old_size + (erasesize - 1)) / erasesize;
	size = block_count * erasesize;

	block = malloc(erasesize);
	if (!block) {
		fprintf(stderr, "%s-%d::malloc mem fail!\n", __FUNCTION__, __LINE__);
		ret = -1;
		goto fail;
	}

	flash_pos_count = (unsigned int)(dstP - flash_mtd_info[flash_idx].start);
	nWritten = 0;

	fp_pos = ftell(fp);
	printf("flashWrite --> total size %d fp_pos %d\n", size, fp_pos);
	if (fp_pos < 0) {
		fprintf(stderr, "error ftell(fp) %d\n", fp_pos);
		ret = -1;
		goto fail;
	}

UPGRADE_IMGB:
#ifndef CONFIG_DOUBLE_IMAGE
	if (backup) {
		/*note: start_addr + offset: can't exceed
		   more than flash_mtd_info[flash_idx].end - flash_mtd_info[flash_idx].start (mtd partition size) */

		if ((flash_idx = get_flash_index((void *)g_fs_bak_offset)) < 0) {
			ret = -1;
			goto fail;
		}

		erasesize = flash_mtd_info[flash_idx].erasesize;

		block_count = (old_size + (erasesize - 1)) / erasesize;
		size = block_count * erasesize;

		block = realloc(block, erasesize);
		if (!block) {
			fprintf(stderr, "%s-%d::realloc mem fail!\n", __FUNCTION__, __LINE__);
			ret = -1;
			goto fail;
		}

		flash_pos_count = 0;		//reset flash position count!
		nWritten = 0;			//reset buffer data count!
		fseek(fp, fp_pos, SEEK_SET);	//seek data ptr to ori position.
	}
#endif

	fd = get_mtd_fd(flash_mtd_info[flash_idx].name);
	if (fd < 0) {
		fprintf(stderr, "%s(%d):get_mtd_fd fails fd %d\n", __FUNCTION__, __LINE__, fd);
		ret = -1;
		goto fail;
	}

	while (nWritten < size) {
		nRead = (erasesize > (size - nWritten)) ? (size - nWritten) : erasesize;
		nRead = fread(block, 1, nRead, fp);

ERASE_NEXT_BLOCK:
		if (flash_pos_count >= flash_mtd_info[flash_idx].end - flash_mtd_info[flash_idx].start) {
			fprintf(stderr, "Error: you can't erase the region exceed mtd partition size\n");
			ret = -1;
			goto fail;
		}


		if (__mib_nand_flash_write(block, flash_pos_count, erasesize,
					   flash_mtd_info[flash_idx].name) == 0)
			ret = -1;

		flash_pos_count += erasesize;	//calculate next block addr.

		if (ret < 0) {	//write data fail!
			/* reset ret */
			ret = 0;
			fprintf(stderr,
				"%s(%d): __mib_nand_flash_write fail! addr %d\n",
				__FUNCTION__, __LINE__, flash_pos_count - erasesize);
			goto ERASE_NEXT_BLOCK;
		}

		nWritten += erasesize;

		printf("flashWrite --> nWritten %d\n", nWritten);
	}

	close(fd);
	fd = -1;

#ifndef CONFIG_DOUBLE_IMAGE
	if (backup == 0) {
		backup = 1;
		printf("goto upgrade img B! backup %d, block_count %d\n",
		       backup, block_count);
		goto UPGRADE_IMGB;
	}
#endif

fail:
	if (fd >= 0)
		close(fd);
	free(block);

	return ret;
}
#endif
#elif !defined(CONFIG_BLK_DEV_INITRD)
int flashdrv_filewrite(FILE * fp, int size, void *dstP)
{
	int nWritten = 0, nRead, ret = 0;
	uint16_t rData16;
	uint32_t rData32;
	void *vmlinuxAddr, *block = NULL;

	vmlinuxAddr = dstP;

	/* Read the magic number */
	flash_read(&rData32, (int)dstP, sizeof(rData32));
	__le32_to_cpus(&rData32);

	/* Skip rootfs */
	if (CRAMFS_MAGIC == rData32) {
		/* Read size */
		flash_read(&rData32, (int)(dstP + 4), sizeof(rData32));
		__le32_to_cpus(&rData32);

		/* 16Byte alignment */
		vmlinuxAddr += (rData32 + 0x000FUL) & ~0x000FUL;
	} else if (SQUASHFS_MAGIC == rData32) {
		/* Read s_major */
		flash_read(&rData16, (int)(dstP + 28), sizeof(rData16));
		__le16_to_cpus(&rData16);

		/* s_major >= 4 */
		if (rData16 >= 4) {
			/* Read bytes_used */
			flash_read(&rData32, (int)(dstP + 40), sizeof(rData32));
			__le32_to_cpus(&rData32);
		} else {
			/* Read bytes_used */
			flash_read(&rData32, (int)(dstP + 8), sizeof(rData32));
			__le32_to_cpus(&rData32);
		}

		/* 64KB alignment */
		vmlinuxAddr += (rData32 + 0x0FFFFUL) & ~0x0FFFFUL;
	}

	block = malloc(FLASH_BLOCK_SIZE);
	if (!block) {
		ret = -1;
		goto fail;
	}
	// Kaohj -- destroying image
	memset(block, 1, FLASH_BLOCK_SIZE);
	flash_write(block, (int)vmlinuxAddr, FLASH_BLOCK_SIZE);

	while (nWritten < size) {
		nRead = (FLASH_BLOCK_SIZE >
			 (size - nWritten)) ? (size -
					       nWritten) : FLASH_BLOCK_SIZE;
		nRead = fread(block, 1, nRead, fp);

		printf("flashWrite --> 0x%08x (len 0x%x)\n", dstP, nRead);
		if (!flash_write(block, (int)dstP, nRead)) {
			ret = -1;
			goto fail;
		}

		dstP += nRead;
		nWritten += nRead;
	}

fail:
	free(block);

	return ret;
}
#endif
#endif

// Mason Yu On True
#ifdef CONFIG_USER_RTK_SYSLOG
#ifdef SEND_LOG
static void startSendLogToServer()
{
	FILE *fp;
	unsigned char username[30], password[30];
	char ipaddr[20], tmpStr[5], tmpBuf[30];

	//strcpy(g_ddns_ifname, "all");
	//sem_post(&semDdnsc);

	if ((fp = fopen("/var/ftpput.txt", "w")) == NULL)
	{
		printf("***** Open file /var/ftpput.txt failed !\n");
		return;
	}

	if ( !mib_get_s(MIB_LOG_SERVER_IP, (void *)tmpStr, sizeof(tmpStr))) {
		printf("Get LOG Server IP error!\n");
		fclose(fp);
		return;
	}
	strncpy(ipaddr, inet_ntoa(*((struct in_addr *)tmpStr)), 16);
	ipaddr[15] = '\0';
	//printf("ipaddr=%s\n", ipaddr);

	if ( !mib_get_s(MIB_LOG_SERVER_NAME, (void *)username, sizeof(username))) {
		printf("Get user name for LOG Server IP error!\n");
		fclose(fp);
		return;
	}
	//printf("username=%s\n", username);

	if ( !mib_get_s(MIB_LOG_SERVER_PASSWORD, (void *)password, sizeof(password))) {
		printf("Get user name for LOG Server IP error!\n");
		fclose(fp);
		return;
	}
	//printf("username=%s\n", password);

	fprintf(fp, "open %s\n", ipaddr);
	fprintf(fp, "user %s %s\n", username, password);
	fprintf(fp, "lcd /var/log\n");
	fprintf(fp, "bin\n");
	fprintf(fp, "put messages\n");
	fprintf(fp, "bye\n");
	fprintf(fp, "quit\n");
	fclose(fp);

	system("/bin/ftp -inv < /var/ftpput.txt");
	//va_cmd("/bin/ftp", 3, 0, "-inv", "-f", "/var/ftpput.txt");

	return;
}

static void auto_startSYSLOGC(void)
{
	//strcpy(g_ddns_ifname, "all");
	sem_post(&semSyslogC);
	signal(SIGALRM, auto_startSYSLOGC);
	alarm(AUTO_STARTSYSLOGC_INTERVAL);
}

static void log_syslogCpid()
{
	FILE *f;
	pid_t pid;
	char *pidfile = "/var/run/syslogC.pid";

	pid = getpid();
	printf("\nsyslogC=%d\n",pid);

	if((f = fopen(pidfile, "w")) == NULL)
		return;
	fprintf(f, "%d\n", pid);
	fclose(f);

}

static void syslogC(void)
{
	unsigned int entryNum, i;
	char account[70];
	MIB_CE_DDNS_T Entry;


	/* Establish a handler for SIGALRM signals. */
	signal(SIGALRM, auto_startSYSLOGC);
	alarm(AUTO_STARTSYSLOGC_INTERVAL);

	log_syslogCpid();

	signal(SIGUSR1, startSendLogToServer);

	// wait for user start Syslog Client
	while(1) {
		sem_wait(&semSyslogC);
		printf("Auto send syslog file to FTP Server\n");
		startSendLogToServer();
	}
}

static void startSyslogC()
{
	pthread_t ptSyslogCId;

	pthread_create(&ptSyslogCId, NULL, syslogC, NULL);
	pthread_detach(ptSyslogCId);
}
#endif  // #ifdef SEND_LOG
#endif  // #ifdef CONFIG_USER_RTK_SYSLOG

#ifdef CONFIG_REMOTE_CONFIGD
#ifdef CONFIG_DSL_VTUO
#ifdef CONFIG_SPC
static void *remote_configd(void *arg)
{
	int rconfigd_socket, ret;
	struct mymsgbuf qbuf;

	rconfigd_socket = socket(PF_SPC, SOCK_DGRAM, 0);

	if (rconfigd_socket < 0) {
		perror("socket");
		return NULL;
	}

	while (1) {
		if ((ret = recv(rconfigd_socket, &qbuf, sizeof(qbuf), 0)) < 0)
			fprintf(stderr, "%s:%d recv: %s\n", __FUNCTION__, __LINE__, strerror(errno));
		msgProcess(&qbuf);
		if (send(rconfigd_socket, &qbuf, ret, 0) < 0)
			fprintf(stderr, "%s:%d send: %s\n", __FUNCTION__, __LINE__, strerror(errno));
	}

	close(rconfigd_socket);
	return NULL;
}
#else
static void *remote_configd(void *arg)
{
	int rconfigd_socket, ret;
	socklen_t len;
	struct mymsgbuf qbuf;
	struct sockaddr_in addr;

	rconfigd_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (rconfigd_socket < 0) {
		perror("socket");
		return NULL;
	}

	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(8809);
	addr.sin_family = PF_INET;

	if (bind(rconfigd_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		fprintf(stderr, "%s:%d bind: %s\n", __FUNCTION__, __LINE__, strerror(errno));

	while (1) {
		len = sizeof(addr);
		if ((ret = recvfrom(rconfigd_socket, &qbuf, sizeof(qbuf), 0, (struct sockaddr *)&addr, &len)) < 0)
			fprintf(stderr, "%s:%d recvfrom: %s\n", __FUNCTION__, __LINE__, strerror(errno));
		msgProcess(&qbuf);
		if (sendto(rconfigd_socket, &qbuf, ret, 0, &addr, len) < 0)
			fprintf(stderr, "%s:%d sendto: %s\n", __FUNCTION__, __LINE__, strerror(errno));
	}

	close(rconfigd_socket);
	return NULL;
}
#endif

static void start_remote_configd(void)
{
	pthread_t id;

	pthread_create(&id, NULL, remote_configd, NULL);
	pthread_detach(id);
}
#endif
#endif

int main(int argc, char **argv)
{
	pid_t pid;
	long i, maxfd;
	int do_daemon = 0;

#ifdef CONFIG_DEV_xDSL
	cpu_set_t mask;
	CPU_ZERO(&mask);
	if (sched_getaffinity(0, sizeof(mask), &mask) != -1)
	{
		/* Remove CPU0/CPU1 */
		CPU_CLR(0, &mask);
		CPU_CLR(1, &mask);
		if (sched_setaffinity(0, sizeof(mask), &mask) == -1)
		{
			fprintf(stderr, "warning: could not set CPU affinity/n");
		}
	}
#endif
	isConfigd = 1;
	
	init_signals();

	// set debug mode
	DEBUGMODE(STA_WARNING|STA_ERR);
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-D"))
			do_daemon = 1;
	}
/*
 *	Init MIB
 */
#ifndef CONFIG_USER_XMLCONFIG
	if ( mib_init() == 0 ) {
		printf("\033[0;31m[configd] Initialize MIB failed!\033[0m\n");
//		error(E_L, E_LOG, "Initialize MIB failed!\n");
		return -1;
	}
#endif	/*CONFIG_USER_XMLCONFIG*/

	if ((initMsgQ()) < 0) {
		return -1;
	}
	if (initShm() < 0) {
		printf("Init shared memory fail !\n");
		return -1;
	}
	//printf("[configd] Initialize 'MsgQ' and 'Shm' finished.\n");

	if (do_daemon) { /* Prepare to become a daemon. */
		if ((pid = fork()) == -1)
			return -1;
		if (pid != 0) /* Parent */
			exit(EXIT_SUCCESS);

		/* Child: create new session and process group */
		if(setsid() == -1)
			return -1;

		/* close all open files */
		maxfd = sysconf(_SC_OPEN_MAX);
		for (i = 0; i < maxfd; ++i) {
			if (i != STDOUT_FILENO && i != STDERR_FILENO && i != STDIN_FILENO)
				close(i);
		}
	}
	
	setpriority(PRIO_PROCESS, getpid(), -19);

	log_pid();
// Mason Yu on True
#ifdef CONFIG_USER_RTK_SYSLOG
#ifdef SEND_LOG
	sem_init(&semSyslogC, 0, 0);
	startSyslogC();
#endif
#endif

#ifdef CONFIG_REMOTE_CONFIGD
#ifdef CONFIG_DSL_VTUO
	start_remote_configd();
#endif
#endif

	openlog2(LOG_DM_CONFIGD);
	msg_handler();
	printf("[configd]: should not be here !!\n");
	return 0;
}


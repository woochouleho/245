/*
 * sysconfig.c --- main file for configuration server API
 * --- By Kaohj
 */

#include "sysconfig.h"
#include "msgq.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/shm.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <rtk/utility.h>
#ifdef EMBED
#include <config/autoconf.h>
#else
#include "../../../../config/autoconf.h"
#endif
#ifdef CONFIG_REMOTE_CONFIGD
#ifdef CONFIG_SPC
#include <linux/if_ether.h>
#endif
#endif

#ifdef WLAN_SUPPORT
extern int wlan_idx;
int wlan_idx_bak=0;
#endif

#ifdef CONFIG_USER_XMLCONFIG
const char LASTGOOD_FILE[] = "/var/config/lastgood.xml";
const char LASTGOOD_HS_FILE[] = "/var/config/lastgood_hs.xml";
#endif	/*CONFIG_USER_XMLCONFIG*/

#define CONF_SERVER_PIDFILE	"/var/run/configd.pid"
// Mason Yu. deadlock
#include <errno.h>
int lock_shm_by_flock()
{
	int lockfd;

	if ((lockfd = open(CONF_SERVER_PIDFILE, O_RDWR)) == -1) {
		perror("open shm lockfile");
		return lockfd;
	}
	while (flock(lockfd, LOCK_EX) == -1 && errno==EINTR) {
		printf("configd write lock failed by flock. errno=%d\n", errno);
	}
	return lockfd;
}

int unlock_shm_by_flock(int lockfd)
{
	while (flock(lockfd, LOCK_UN) == -1 && errno==EINTR) {
		printf("configd write unlock failed by flock. errno=%d\n", errno);
	}
	if(lockfd != -1)
		close(lockfd);
	return 0;
}
#ifdef WLAN_SUPPORT
void mib_save_wlanIdx()
{
       wlan_idx_bak =  wlan_idx;
}
void mib_recov_wlanIdx()
{
       wlan_idx = wlan_idx_bak;
}
#endif
#ifdef CONFIG_USER_XMLCONFIG
extern const char *shell_name;
int xml_mib_load(CONFIG_DATA_T type, CONFIG_MIB_T flag)
{
	int ret=1;

	printf("%s():...\n", __FUNCTION__);
	if (va_cmd(shell_name, 3, 1, "/etc/scripts/config_xmlconfig.sh", "-l", LASTGOOD_FILE) != MSG_SUCC) {
		printf("[xmlconfig] mib reload failed\n");
		ret = 0;
	}else
		printf("[xmlconfig] mib reload success\n");

	return ret;
}

int xml_mib_flash_to_default(CONFIG_DATA_T type)
{
	int ret=1;

	printf("%s():...\n", __FUNCTION__);
	if (va_cmd(shell_name, 2, 1, "/etc/scripts/config_xmlconfig.sh", "-d") != MSG_SUCC) {
		printf("[xmlconfig] mib reset_flash_to_default failed\n");
		ret = 0;
	}else
		printf("[xmlconfig] mib reset_flash_to_default success\n");

	return ret;
}
#endif	/*CONFIG_USER_XMLCONFIG*/

#ifdef WLAN_SUPPORT
/*
 *	Reorder mib id for wlan mib in case of multiple wlan interfaces.
 *	Return value: reordered wlan mib id based on wlan_idx, -1 on error.
 */
static int wlan_mib_id_reorder(int id)
{
	int mib_id=id;
	
	if (id > DUAL_WLAN_START_ID && id < DUAL_WLAN_END_ID) {
		mib_id = -1;
		if (isValid_wlan_idx(wlan_idx))
			mib_id = id + wlan_idx;
	}

	return mib_id;
}

static int wlan_mibchain_id_reorder(int id)
{
	int mib_id=id;
	
	if (id > DUAL_WLAN_CHAIN_START_ID && id < DUAL_WLAN_CHAIN_END_ID) {
		mib_id = -1;
		if (isValid_wlan_idx(wlan_idx))
			mib_id = id + wlan_idx;
	}
	
	return mib_id;
}
#endif

#include <sys/syscall.h>
static void sendcmd(struct mymsgbuf *qbuf)
{
	key_t key;
	int qid_snd, qid_rcv, cpid, ctgid, spid;
	int ret = 0;

#ifdef EMBED
	/* initial request to prevent unexpect return */
	qbuf->request = MSG_FAIL;

	/* Create unique key via call to ftok() */
	key = ftok("/bin/init", 's');
	if ((qid_snd = open_queue(key, MQ_GET)) == -1) {
		perror("open_queue");
		return;
	}

	/* Create unique key via call to ftok() */
	key = ftok("/bin/init", 'r');
	if ((qid_rcv = open_queue(key, MQ_GET)) == -1) {
		perror("open_queue");
		return;
	}

	// get client pid
	// Consider multi-thread environment, we use tid (thread id) as message id.
	cpid = (int)syscall(SYS_gettid);
	ctgid = (int)getpid();

	// get server pid
	// Mason Yu. Not use fopen()
	spid = MSG_CONFIGD_PID;

	ret = send_message(qid_snd, spid, cpid, ctgid, qbuf);
	if(ret >= 0){
		if(-1 == read_message(qid_rcv, qbuf, cpid)){
			qbuf->request = MSG_FAIL;
		}
	}
	else{
		qbuf->request = MSG_FAIL;
	}
#else
	memset(qbuf, 0, sizeof(struct mymsgbuf));
	qbuf->request = MSG_SUCC;
#endif
}

/*
 *	Get data from shared memory(shared memory --> ptr).
 *	-1 : error
 	 0 : successful
 */
static int get_shm_data(const char *ptr, int len)
{
	int shmid;
	char *shm_start;

	if (len > SHM_SIZE)
		return -1;
#ifdef RESTRICT_PROCESS_PERMISSION
	if ((shmid = shmget((key_t)1234, SHM_SIZE, 0666)) == -1)
		return -1;
#else
	if ((shmid = shmget((key_t)1234, SHM_SIZE, 0)) == -1)
		return -1;
#endif
	// Attach shared memory segment.
	if ((shm_start = (char *)shmat( shmid , NULL , 0 ))==(char *)-1)
		return -1;
	memcpy((void *)ptr, shm_start, len);
	// Detach shared memory segment.
	shmdt(shm_start);
	return 0;
}

/*
 *	Put data to shared memory(ptr --> shared memory).
 *	-1 : error
 *	 0 : successful
 */
static int put_shm_data(const char *ptr, int len)
{
	int shmid;
	char *shm_start;
#ifdef RESTRICT_PROCESS_PERMISSION
	if ((shmid = shmget((key_t)1234, SHM_SIZE, 0666)) == -1)
		return -1;
#else
	if ((shmid = shmget((key_t)1234, SHM_SIZE, 0)) == -1)
		return -1;
#endif
	// Attach shared memory segment.
	if ((shm_start = (char *)shmat( shmid , NULL , 0 ))==(char *)-1)
		return -1;
	if (len > SHM_SIZE)
		return -1;
	memcpy((void *)shm_start, ptr, len);
	// Detach shared memory segment.
	shmdt(shm_start);
	return 0;
}

int mib_lock()
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_LOCK;
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("mib lock failed\n");
		ret = 0;
	}

	return ret;
}

int mib_unlock()
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_UNLOCK;
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("mib lock failed\n");
		ret = 0;
	}

	return ret;
}

#if !defined(CONFIG_USER_XMLCONFIG) && !defined(CONFIG_USER_CONF_ON_XMLFILE)
/*
 *	Write the specified setting to flash.
 *	This function will also check the length and checksum.
 */
int mib_update_from_raw(unsigned char *ptr, int len)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret, fd;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_UPDATE_FROM_RAW;
	mymsg->arg1 = len;
	fd = lock_shm_by_flock();
	if (put_shm_data(ptr, len) < 0) {
		printf("Shared memory operation fail !\n");
		unlock_shm_by_flock(fd);
		return 0;
	}
	sendcmd(&qbuf);
	if (qbuf.request == MSG_SUCC) {
		ret = 1;
	} else {
		printf("mib update_from_raw failed\n");
		ret = 0;
	}
	unlock_shm_by_flock(fd);

	return ret;
}

/*
 *	Load flash setting to the specified pointer
 */
int mib_read_to_raw(CONFIG_DATA_T type, unsigned char* ptr, int len)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1, fd;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_READ_TO_RAW;
	mymsg->arg1=(int)type;
	mymsg->arg2=len;
	fd=lock_shm_by_flock();
	sendcmd(&qbuf);
	if (qbuf.request == MSG_SUCC) {
		if (get_shm_data(ptr, len)<0) {
			unlock_shm_by_flock(fd);
			return 0;
		}
		ret = 1;
	}
	else {
		printf("mib read_to_raw failed\n");
		ret = 0;
	}
	unlock_shm_by_flock(fd);

	return ret;
}

 /*
  *	Load flash header
  */
int mib_read_header(CONFIG_DATA_T type, PARAM_HEADER_Tp pHeader)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_READ_HEADER;
	mymsg->arg1=(int)type;
	sendcmd(&qbuf);
	if (qbuf.request == MSG_SUCC) {
		memcpy((void *)pHeader, mymsg->mtext, sizeof(PARAM_HEADER_T));
		ret = 1;
	}
	else {
		printf("mib read header failed\n");
		ret = 0;
	}

	return ret;
}
#endif

int mib_update_ex(CONFIG_DATA_T type, CONFIG_MIB_T flag, int commitNow)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;
	
	if(commitNow == 0 && type == CURRENT_SETTING)
	{
		system("sysconf send_unix_sock_message /var/run/systemd.usock do_cs_mib_update_delay");
		return 1;
	}
	
	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_UPDATE;
	mymsg->arg1 = (int)type;
	mymsg->arg2 = (int)flag;
	mymsg->arg3 = commitNow;

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("mib update failed\n");
		ret = 0;
	}

#ifdef CONFIG_USER_XMLCONFIG
#ifdef CONFIG_USER_FLATFSD_XXX
	va_cmd("/bin/flatfsd", 1, 1, "-s");
#endif
#elif CONFIG_USER_RTK_RECOVER_SETTING
	// save config file to flash
	if (type == HW_SETTING)
		va_cmd("/bin/saveconfig", 2, 1, "-s", "hs");
	else
		va_cmd("/bin/saveconfig", 1, 1, "-s");
#endif /*CONFIG_USER_XMLCONFIG */
#ifdef CONFIG_USER_CLUSTER_MANAGE
    {
        int pid;
        pid = read_pid(CLU_CLIENT_RUNFILE);
        if (pid > 0)
            kill(pid, SIGUSR1);
    }
#endif

	return ret;
}
int mib_update(CONFIG_DATA_T type, CONFIG_MIB_T flag)
{
	return mib_update_ex(type, flag, 0);
}

#ifdef CONFIG_USER_XMLCONFIG
int mib_load(CONFIG_DATA_T type, CONFIG_MIB_T flag)
{
	return xml_mib_load(type, flag);
}
#else
int mib_load(CONFIG_DATA_T type, CONFIG_MIB_T flag)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_RELOAD;
	mymsg->arg1=(int)type;
	mymsg->arg2=(int)flag;
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("mib reload failed\n");
		ret = 0;
	}

	return ret;
}
#endif	/*CONFIG_USER_XMLCONFIG*/

/* 2010-10-26 krammer :  */
int mib_swap(int id, int id1)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_table_entry_T info;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_SWAP;
	mymsg->arg1=id;
	mymsg->arg2=id1;
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("%s: Swap request failed! (id=%d, id1=%d)\n", __func__, id, id1);
		ret = 0;
	}
	return ret;
}

extern char *__progname __attribute__((weak));
char *__progname = "";
int mib_get_s(int id, void *value, int value_len)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_table_entry_T info;
	int ret;

#ifdef WLAN_SUPPORT
	id = wlan_mib_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	if (!mib_info_id(id, &info))
		return 0;

	memset((void *)&qbuf, 0, sizeof(struct mymsgbuf));
	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_GET;
	mymsg->arg1 = id;

	sendcmd(&qbuf);
	if (qbuf.request == MSG_SUCC) {
		if (value_len > 0 &&  value_len < info.size) {
			printf("\033[31;1;4mWARN: Program(%s) mib_get() ID (%d,%s) size=%d but value buffer size=%d\033[0m\n", __progname, id, info.name, info.size, value_len);
			info.size = value_len;
		}
		memcpy(value, mymsg->mtext, info.size);
		ret = 1;
	} else {
		printf("%s: Get request failed! (id=%d)\n", __func__, id);
		ret = 0;
	}

	return ret;
}

int mib_get(int id, void *value)
{
	return mib_get_s(id, value, -1);
}

/*
* mib_get() remove wlan_idx global mapping for dual wlan:
*  - to avoid race condiction while OSGI or other async execution(dbus) with config_Wlan()
*  - if someone want to set wlan associative mibs, and the action async with config_Wlan() (i.e no locked)
*    then you should call "local_mapping" mib API, except original mib API.
*/
int mib_local_mapping_get(int id, int wlanIdx, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_table_entry_T info;
	int ret;
	
#ifdef WLAN_SUPPORT
	if (id > DUAL_WLAN_START_ID && id < DUAL_WLAN_END_ID)
		id += wlanIdx;
#endif
	
	if (!mib_info_id(id, &info)) {
		return 0;
	}

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_GET;
	mymsg->arg1 = id;

	sendcmd(&qbuf);
	if (qbuf.request == MSG_SUCC) {
		memcpy(value, mymsg->mtext, info.size);
		ret = 1;
	} else {
		printf("%s: Get request failed! (id=%d)\n", __func__, id);
		ret = 0;
	}

	return ret;
}

int mib_backup_get(int id, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_table_entry_T info;
	int ret;

#ifdef WLAN_SUPPORT
	id = wlan_mib_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	if (!mib_info_id(id, &info))
		return 0;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_BACKUP_GET;
	mymsg->arg1 = id;

	sendcmd(&qbuf);
	if (qbuf.request == MSG_SUCC) {
		memcpy(value, mymsg->mtext, info.size);
		ret = 1;
	} else {
		printf("%s: Get request failed! (id=%d)\n", __func__, id);
		ret = 0;
	}

	return ret;
}

int mib_local_mapping_backup_get(int id, int wlanIdx, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_table_entry_T info;
	int ret;

#ifdef WLAN_SUPPORT
	if (id > DUAL_WLAN_START_ID && id < DUAL_WLAN_END_ID)
		id += wlanIdx;
#endif

	if (!mib_info_id(id, &info)) {
		return 0;
	}

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_BACKUP_GET;
	mymsg->arg1 = id;

	sendcmd(&qbuf);
	if (qbuf.request == MSG_SUCC) {
		memcpy(value, mymsg->mtext, info.size);
		ret = 1;
	} else {
		printf("%s: Get request failed! (id=%d)\n", __func__, id);
		ret = 0;
	}

	return ret;
}

int g_del_escapechar = 0;
int delEscapeStr(char* data, int datalength)
{
	char specialchar[] = "'\"&><;(){}$";
	char *buf;
	int i = 0, j = 0;
	buf=(char *)malloc(datalength);
	for (i = 0; i<datalength && data[i] != 0; i++)
	{
		for (j = 0; j<strlen(specialchar); j++) {
			if (data[i] == specialchar[j] && data[i-1] == '\\')
			{
				memset(buf,0,datalength);
				memcpy(buf,(data+i),datalength-i);
				memcpy((data+i-1),buf,strlen(buf));
				data[datalength-1] = 0;
				i--;
				datalength--;
				break;
			}
		}
	}
	free(buf);
	return datalength;
}

int mib_set(int id, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_table_entry_T info;
	int ret = 1, length = 0;
	char *val_str;

#ifdef WLAN_SUPPORT
	id = wlan_mib_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	if (!mib_info_id(id, &info))
		return 0;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_SET;
	mymsg->arg1 = id;
#ifdef CONFIG_USER_CWMP_TR069
	mymsg->arg2 = 1;
#endif
	if(info.type == STRING_T && g_del_escapechar == 1)
	{
		length = strlen(value);
		val_str = (char *)malloc(length+1);
		memset(val_str, 0 ,length+1);
		memcpy(val_str, value, length);
		delEscapeStr(val_str,length);
		memcpy(mymsg->mtext, val_str, ((length+1<info.size) ? length+1 : info.size));
		free(val_str);
	}
	else
		memcpy(mymsg->mtext, value, info.size);

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("%s: Set request failed! (id=%d)\n", __func__, id);
		ret = 0;
	}

#ifdef CONFIG_USER_DBUS_PROXY
	if (ret != 0)
	{
		send_notify_msg_dbusproxy(id, e_dbus_signal_mib_set, 0);
	}
#endif
#if defined(CONFIG_E8B) && defined(CONFIG_USER_LANNETINFO)
	extern void rtk_e8_mib_set_signal_to_lannetinfo(int, int);
	rtk_e8_mib_set_signal_to_lannetinfo(ret, id);
#endif

	return ret;
}

/*
* mib_set() remove wlan_idx global mapping for dual wlan:
*  - to avoid race condiction while OSGI or other async execution(dbus) with config_Wlan()
*  - if someone want to set wlan associative mibs, and the action async with config_Wlan() (i.e no locked)
*    then you should call "local_mapping" mib API, except original mib API.
*/
int mib_local_mapping_set(int id, int wlanIdx, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_table_entry_T info;
	int ret = 1;

#ifdef WLAN_SUPPORT
	if (id > DUAL_WLAN_START_ID && id < DUAL_WLAN_END_ID)
		id += wlanIdx;
#endif
	
	if (!mib_info_id(id, &info))
		return 0;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_SET;
	mymsg->arg1 = id;
#ifdef CONFIG_USER_CWMP_TR069
	mymsg->arg2 = 1;
#endif
	memcpy(mymsg->mtext, value, info.size);

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("%s: Set request failed! (id=%d)\n", __func__, id);
		ret = 0;
	}

#ifdef CONFIG_USER_DBUS_PROXY
	if (ret != 0)
	{
		send_notify_msg_dbusproxy(id, e_dbus_signal_mib_set, 0);
	}
#endif

	return ret;
}


// Magician: For cwmp_core use only, prevent loop messaging.
#ifdef CONFIG_USER_CWMP_TR069
int mib_set_cwmp(int id, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_table_entry_T info;
	int ret = 1;

#ifdef WLAN_SUPPORT
	id = wlan_mib_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	if (!mib_info_id(id, &info))
		return 0;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_SET;
	mymsg->arg1 = id;
#ifdef CONFIG_USER_CWMP_TR069
	mymsg->arg2 = 0;
#endif
	memcpy(mymsg->mtext, value, info.size);

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("%s: Set request failed! (id=%d)\n", __func__, id);
		ret = 0;
	}

	return ret;
}
#endif

int mib_sys_to_default(CONFIG_DATA_T type)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_TO_DEFAULT;
	mymsg->arg1 = (int)type;

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("mib reset_sys_to_default failed\n");
		ret = 0;
	}
	
#if defined(CONFIG_YUEME)
	if (type == CURRENT_SETTING) {
#if defined(YUEME_4_0_SPEC)
		dbus_do_ClearExtraMIB();
#endif
	}
#endif

	return ret;
}

#ifdef CONFIG_USER_XMLCONFIG
int mib_flash_to_default(CONFIG_DATA_T type)
{
	int ret;

	ret = xml_mib_flash_to_default(type);
	return ret;
}
#else
int mib_flash_to_default(CONFIG_DATA_T type)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_FLASH_TO_DEFAULT;
	mymsg->arg1 = (int)type;

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("mib reset_flash_to_default failed\n");
		ret = 0;
	}

	return ret;
}
#endif
// 2013/11 Jiachiam
#if (defined VOIP_SUPPORT) && (defined CONFIG_USER_XMLCONFIG)
int mib_voip_to_default(void)
{
       struct mymsgbuf qbuf;
       MSG_T *mymsg;
       int ret=1;

       mymsg = &qbuf.msg;
       mymsg->cmd = CMD_MIB_VOIP_TO_DEFAULT;
       sendcmd(&qbuf);
       if (qbuf.request != MSG_SUCC) {
               printf("mib reset_flash_to_default failed\n");
               return 0;
       }
       return ret;
}
#endif /* VOIP_SUPPORT &&  CONFIG_USER_XMLCONFIG*/

int mib_info_id(int id, mib_table_entry_T * info)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_INFO_ID;
	mymsg->arg1 = id;

	sendcmd(&qbuf);
	if (qbuf.request == MSG_SUCC) {
		memcpy(info, mymsg->mtext, sizeof(mib_table_entry_T));
		ret = 1;
	} else {
		printf("%s: get mib info id failed! (id=%d)\n", __func__, id);
		ret = 0;
	}

	return ret;
}

int mib_getDef(int id, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_table_entry_T info;
	int ret;

#ifdef WLAN_SUPPORT
	id = wlan_mib_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	if (!mib_info_id(id, &info))
		return 0;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_GET_DEFAULT;
	mymsg->arg1 = id;

	sendcmd(&qbuf);
	if (qbuf.request == MSG_SUCC) {
		memcpy(value, mymsg->mtext, info.size);
		ret = 1;
	} else {
		printf("%s: Get request failed! (id=%d)\n", __func__, id);
		ret = 0;
	}

	return ret;
}

int mib_info_index(int index, mib_table_entry_T *info)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_INFO_INDEX;
	mymsg->arg1=index;
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("%s: Get mib info index %d failed\n", __func__, index);
		ret = 0;
	}
	else
		memcpy((void *)info, (void *)mymsg->mtext, sizeof(mib_table_entry_T));

	return ret;
}

// Apply Star Zhang's fast load
int mib_info_total()
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_INFO_TOTAL;
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("get mib size failed\n");
		ret = 0;
	}
	else
		ret = qbuf.msg.arg1;

	return ret;
}
// The end of fast load

/*
 * type:
 * CONFIG_MIB_ALL:   all mib setting (table and chain)
 * CONFIG_MIB_TABLE: mib table
 * CONFIG_MIB_CHAIN: mib_chain
 */
int mib_backup(CONFIG_MIB_T type)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_BACKUP;
	mymsg->arg1=(int)type;
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("mib backup (type=%d) failed\n", type);
		ret = 0;
	}else
		printf("mib backup (type=%d) success\n", type);

	return ret;
}

//added by ql
#ifdef	RESERVE_KEY_SETTING
int mib_retrive_table(int id)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_RETRIVE_TABLE;
	mymsg->arg1= id;
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("mib retrieve table failed\n");
		ret = 0;
	}

	return ret;
}
int mib_retrive_chain(int id)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_RETRIVE_CHAIN;
	mymsg->arg1= id;
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("mib retrieve chain failed\n");
		ret = 0;
	}

	return ret;
}
#endif
/*
 * type:
 * CONFIG_MIB_ALL:   all mib setting (table and chain)
 * CONFIG_MIB_TABLE: mib table
 * CONFIG_MIB_CHAIN: mib_chain
 */
int mib_restore(CONFIG_MIB_T type)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_RESTORE;
	mymsg->arg1=(int)type;
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("mib restore (type=%d) failed\n", type);
		ret = 0;
	}else
		printf("mib restore (type=%d) success\n", type);

	return ret;
}

int mib_chain_total(int id)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret;

#ifdef WLAN_SUPPORT
	id = wlan_mibchain_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_TOTAL;
	mymsg->arg1 = id;

	sendcmd(&qbuf);
	if (qbuf.request == MSG_SUCC) {
		ret = qbuf.msg.arg1;
	} else {
		printf("get total failed\n");
		ret = 0;
	}

	return ret;
}

int mib_chain_local_mapping_total(int id, int wlanIdx)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret;

#ifdef WLAN_SUPPORT
	if (id > DUAL_WLAN_CHAIN_START_ID && id < DUAL_WLAN_CHAIN_END_ID)
		id += wlanIdx;
#endif

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_TOTAL;
	mymsg->arg1 = id;

	sendcmd(&qbuf);
	if (qbuf.request == MSG_SUCC) {
		ret = qbuf.msg.arg1;
	} else {
		printf("get total failed\n");
		ret = 0;
	}

	return ret;
}


/* cathy, to swap recordNum1 and recordNum2 of a chain */
int mib_chain_swap(int id, unsigned int recordNum1, unsigned int recordNum2)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_chain_record_table_entry_T info;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_SWAP;
	mymsg->arg1=recordNum1;
	mymsg->arg2=recordNum2;
	sprintf(mymsg->mtext, "%d", id);

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("update chain failed\n");
		ret = 0;
	}

	return ret;
}

int mib_chain_get(int id, unsigned int recordNum, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_chain_record_table_entry_T info;
	int ret;
	int size;
#ifdef USE_SHM
	// Mason Yu. deadlock
	int fd;
#endif

#ifdef WLAN_SUPPORT
	id = wlan_mibchain_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	if (!mib_chain_info_id(id, &info))
		return 0;

	size = info.per_record_size;
#ifdef USE_SHM
	if (size >= SHM_SIZE) {
		printf("chain_get: chain record size(%d) overflow (max. %d).\n",
		       size, SHM_SIZE);
		return 0;
	}
#else
	if (size >= MAX_SEND_SIZE) {
		printf("chain_get: chain record size(%d) overflow (max. %d).\n",
		       size, MAX_SEND_SIZE);
		return 0;
	}
#endif

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_GET;
	mymsg->arg1 = id;
	sprintf(mymsg->mtext, "%u", recordNum);
#ifdef USE_SHM
	fd = lock_shm_by_flock();
#endif
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("get chain failed,id=%d,recordNum=%d,%s:%d\n",id,recordNum,__func__,__LINE__);
		ret = 0;
	} else {
#ifdef USE_SHM
		if (get_shm_data(value, size) < 0)
			ret = 0;
		else
			ret = 1;
#else
		memcpy(value, mymsg->mtext, size);
		ret = 1;
#endif
	}
#ifdef USE_SHM
	unlock_shm_by_flock(fd);
#endif
	return ret;
}

/*
* mib_chain_get() remove wlan_idx global mapping for dual wlan:
*  - to avoid race condiction while OSGI or other async execution(dbus) with config_Wlan()
*  - if someone want to set wlan associative mibs, and the action async with config_Wlan() (i.e no locked)
*    then you should call "local_mapping" mib API, except original mib API.
*/
int mib_chain_local_mapping_get(int id, int wlanIdx, unsigned int recordNum, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_chain_record_table_entry_T info;
	int ret;
	int size;
#ifdef USE_SHM
	// Mason Yu. deadlock
	int fd;
#endif

#ifdef WLAN_SUPPORT
	if (id > DUAL_WLAN_CHAIN_START_ID && id < DUAL_WLAN_CHAIN_END_ID)
		id += wlanIdx;
#endif

	if (!mib_chain_info_id(id, &info))
		return 0;

	size = info.per_record_size;
#ifdef USE_SHM
	if (size >= SHM_SIZE) {
		printf("chain_get: chain record size(%d) overflow (max. %d).\n",
		       size, SHM_SIZE);
		return 0;
	}
#else
	if (size >= MAX_SEND_SIZE) {
		printf("chain_get: chain record size(%d) overflow (max. %d).\n",
		       size, MAX_SEND_SIZE);
		return 0;
	}
#endif

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_GET;
	mymsg->arg1 = id;
	sprintf(mymsg->mtext, "%u", recordNum);
#ifdef USE_SHM
	fd = lock_shm_by_flock();
#endif
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("get chain failed,id=%d,recordNum=%d,%s:%d\n",id,recordNum,__func__,__LINE__);
		ret = 0;
	} else {
#ifdef USE_SHM
		if (get_shm_data(value, size) < 0)
			ret = 0;
		else
			ret = 1;
#else
		memcpy(value, mymsg->mtext, size);
		ret = 1;
#endif
	}
#ifdef USE_SHM
	unlock_shm_by_flock(fd);
#endif
	return ret;
}

int mib_chain_local_mapping_backup_get(int id, int wlanIdx, unsigned int recordNum, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_chain_record_table_entry_T info;
	int ret;
	int size;
#ifdef USE_SHM
	// Mason Yu. deadlock
	int fd;
#endif

#ifdef WLAN_SUPPORT
	if (id > DUAL_WLAN_CHAIN_START_ID && id < DUAL_WLAN_CHAIN_END_ID)
		id += wlanIdx;
#endif

	if (!mib_chain_info_id(id, &info))
		return 0;

	size = info.per_record_size;
#ifdef USE_SHM
	if (size >= SHM_SIZE) {
		printf("chain_get: chain record size(%d) overflow (max. %d).\n",
		       size, SHM_SIZE);
		return 0;
	}
#else
	if (size >= MAX_SEND_SIZE) {
		printf("chain_get: chain record size(%d) overflow (max. %d).\n",
		       size, MAX_SEND_SIZE);
		return 0;
	}
#endif

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_CHAIN_BACKUP_GET;
	mymsg->arg1 = id;
	sprintf(mymsg->mtext, "%u", recordNum);
#ifdef USE_SHM
	fd = lock_shm_by_flock();
#endif
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("get chain failed,id=%d,recordNum=%d,%s:%d\n",id,recordNum,__func__,__LINE__);
		ret = 0;
	} else {
#ifdef USE_SHM
		if (get_shm_data(value, size) < 0)
			ret = 0;
		else
			ret = 1;
#else
		memcpy(value, mymsg->mtext, size);
		ret = 1;
#endif
	}
#ifdef USE_SHM
	unlock_shm_by_flock(fd);
#endif
	return ret;
}

int mib_chain_backup_get(int id, unsigned int recordNum, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_chain_record_table_entry_T info;
	int ret;
	int size;
#ifdef USE_SHM
	// Mason Yu. deadlock
	int fd;
#endif

#ifdef WLAN_SUPPORT
	id = wlan_mibchain_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	if (!mib_chain_info_id(id, &info))
		return 0;

	size = info.per_record_size;
#ifdef USE_SHM
	if (size >= SHM_SIZE) {
		printf("chain_get: chain record size(%d) overflow (max. %d).\n",
		       size, SHM_SIZE);
		return 0;
	}
#else
	if (size >= MAX_SEND_SIZE) {
		printf("chain_get: chain record size(%d) overflow (max. %d).\n",
		       size, MAX_SEND_SIZE);
		return 0;
	}
#endif

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_CHAIN_BACKUP_GET;
	mymsg->arg1 = id;
	sprintf(mymsg->mtext, "%u", recordNum);
#ifdef USE_SHM
	fd = lock_shm_by_flock();
#endif
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("get chain failed,id=%d,recordNum=%d,%s:%d\n",id,recordNum,__func__,__LINE__);
		ret = 0;
	} else {
#ifdef USE_SHM
		if (get_shm_data(value, size) < 0)
			ret = 0;
		else
			ret = 1;
#else
		memcpy(value, mymsg->mtext, size);
		ret = 1;
#endif
	}
#ifdef USE_SHM
	unlock_shm_by_flock(fd);
#endif
	return ret;
}

int mib_chain_getDef(int id, unsigned int recordNum, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_chain_record_table_entry_T info;
	int ret;
	int size;
#ifdef USE_SHM
	// Mason Yu. deadlock
	int fd;
#endif

#ifdef WLAN_SUPPORT
	id = wlan_mibchain_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	if (!mib_chain_info_id(id, &info))
		return 0;

	size = info.per_record_size;
#ifdef USE_SHM
	if (size >= SHM_SIZE) {
		printf("chain_get: chain record size(%d) overflow (max. %d).\n",
		       size, SHM_SIZE);
		return 0;
	}
#else
	if (size >= MAX_SEND_SIZE) {
		printf("chain_get: chain record size(%d) overflow (max. %d).\n",
		       size, MAX_SEND_SIZE);
		return 0;
	}
#endif

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_CHAIN_GET_DEFAULT;
	mymsg->arg1 = id;
	sprintf(mymsg->mtext, "%u", recordNum);
#ifdef USE_SHM
	fd = lock_shm_by_flock();
#endif
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("get chain failed,id=%d,recordNum=%d,%s:%d\n",id,recordNum,__func__,__LINE__);
		ret = 0;
	} else {
#ifdef USE_SHM
		if (get_shm_data(value, size) < 0)
			ret = 0;
		else
			ret = 1;
#else
		memcpy(value, mymsg->mtext, size);
		ret = 1;
#endif
	}
#ifdef USE_SHM
	unlock_shm_by_flock(fd);
#endif
	return ret;
}


/*
 * 0  : add fail
 * -1 : table full
 * 1  : successful
 */
int mib_chain_add(int id, void *ptr)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_chain_record_table_entry_T info;
	int ret;
	int size;
#ifdef USE_SHM
	int fd;
#endif

	if (!mib_chain_info_id(id, &info))
		return 0;

	size = mib_chain_total(id);

#ifdef WLAN_SUPPORT
	id = wlan_mibchain_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	ret = size + 1;
	if (info.table_size != -1 && size >= info.table_size)
		return -1;
	size = info.per_record_size;
#ifdef USE_SHM
	if (size >= SHM_SIZE) {
		printf("chain_add: chain record size(%d) overflow (max. %d).\n",
		       size, SHM_SIZE);
		return 0;
	}
#else
	if (size >= MAX_SEND_SIZE) {
		printf("chain_add: chain record size(%d) overflow (max. %d).\n",
		       size, MAX_SEND_SIZE);
		return 0;
	}
#endif

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_ADD;
	mymsg->arg1 = id;
#ifdef USE_SHM
	fd = lock_shm_by_flock();
	if (put_shm_data(ptr, size) < 0) {
		printf("Shared memory operation fail !\n");
		unlock_shm_by_flock(fd);
		return 0;
	}
#else
	memcpy(mymsg->mtext, ptr, size);
#endif

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("add chain failed\n");
		ret = 0;
	}
#ifdef USE_SHM
	unlock_shm_by_flock(fd);
#endif
#ifdef CONFIG_USER_DBUS_PROXY
	if (ret != 0)
	{
		send_notify_msg_dbusproxy(id, e_dbus_signal_mib_chain_add, ret);
	}
#endif
	return ret;
}

int mib_chain_delete(int id, unsigned int recordNum)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

#ifdef WLAN_SUPPORT
	id = wlan_mibchain_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_DELETE;
	mymsg->arg1 = id;
	sprintf(mymsg->mtext, "%u", recordNum);

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("delete chain failed\n");
		ret = 0;
	}
#ifdef CONFIG_USER_DBUS_PROXY
	if (ret != 0)
	{
		send_notify_msg_dbusproxy(id, e_dbus_signal_mib_chain_delete, recordNum);
	}
#endif

	return ret;
}

int mib_chain_clear(int id)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

#ifdef WLAN_SUPPORT
	id = wlan_mibchain_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_CLEAR;
	mymsg->arg1 = id;

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("clear chain failed\n");
		ret = 0;
	}

	return ret;
}

int mib_chain_update(int id, void *ptr, unsigned int recordNum)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_chain_record_table_entry_T info;
	mib_chain_member_entry_T *record_desc = NULL;
	int record_size = 0, item_nums = 0, item_idx = 0, ret = 1, length = 0;
	int size;
#ifdef USE_SHM
	int fd;
#endif

	if(g_del_escapechar == 1)
	{
		if (mib_chain_get_record_desc(id, &record_desc, &record_size) == 0)
			return 0;
	}
#ifdef WLAN_SUPPORT
	id = wlan_mibchain_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	if (!mib_chain_info_id(id, &info))
		return 0;

	size = info.per_record_size;
#ifdef USE_SHM
	if (size >= SHM_SIZE) {
		printf
		    ("chain_update: chain record size(%d) overflow (max. %d).\n",
		     size, SHM_SIZE);
		return 0;
	}
#else
	if (size >= MAX_SEND_SIZE) {
		printf
		    ("chain_update: chain record size(%d) overflow (max. %d).\n",
		     size, MAX_SEND_SIZE);
		return 0;
	}
#endif

	if(g_del_escapechar == 1)
	{
		item_nums = record_size/sizeof(mib_chain_member_entry_T);
		while (item_idx < item_nums)
		{/*0~item_nums-1*/
			if((record_desc+item_idx)->type == STRING_T)
			{
				length = strlen(ptr+(record_desc+item_idx)->offset);
				delEscapeStr(ptr+(record_desc+item_idx)->offset,length);
			}
			item_idx++;
		}
		free(record_desc);
	}

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_UPDATE;
	mymsg->arg1 = id;
	mymsg->arg2 = recordNum;
#ifdef USE_SHM
	fd = lock_shm_by_flock();
	if (put_shm_data(ptr, size) < 0) {
		printf("Shared memory operation fail !\n");
		unlock_shm_by_flock(fd);
		return 0;
	}
#else
	memcpy(mymsg->mtext, ptr, size);
#endif

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("update chain failed\n");
		ret = 0;
	}
#ifdef USE_SHM
	unlock_shm_by_flock(fd);
#endif

#ifdef CONFIG_USER_DBUS_PROXY
	if (ret != 0)
	{
		send_notify_msg_dbusproxy(id, e_dbus_signal_mib_chain_update, recordNum);
	}
#endif
	return ret;
}

/*
* mib_chain_update() remove wlan_idx global mapping for dual wlan:
*  - to avoid race condiction while OSGI or other async execution(dbus) with config_Wlan()
*  - if someone want to set wlan associative mibs, and the action async with config_Wlan() (i.e no locked)
*    then you should call "local_mapping" mib API, except original mib API.
*/
int mib_chain_local_mapping_update(int id, int wlanIdx, void *ptr, unsigned int recordNum)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_chain_record_table_entry_T info;
	int ret = 1;
	int size;
#ifdef USE_SHM
	int fd;
#endif

#ifdef WLAN_SUPPORT
	if (id > DUAL_WLAN_CHAIN_START_ID && id < DUAL_WLAN_CHAIN_END_ID)
		id += wlanIdx;
#endif

	if (!mib_chain_info_id(id, &info))
		return 0;

	size = info.per_record_size;
#ifdef USE_SHM
	if (size >= SHM_SIZE) {
		printf
		    ("chain_update: chain record size(%d) overflow (max. %d).\n",
		     size, SHM_SIZE);
		return 0;
	}
#else
	if (size >= MAX_SEND_SIZE) {
		printf
		    ("chain_update: chain record size(%d) overflow (max. %d).\n",
		     size, MAX_SEND_SIZE);
		return 0;
	}
#endif

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_UPDATE;
	mymsg->arg1 = id;
	mymsg->arg2 = recordNum;
#ifdef USE_SHM
	fd = lock_shm_by_flock();
	if (put_shm_data(ptr, size) < 0) {
		printf("Shared memory operation fail !\n");
		unlock_shm_by_flock(fd);
		return 0;
	}
#else
	memcpy(mymsg->mtext, ptr, size);
#endif

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("update chain failed\n");
		ret = 0;
	}

#ifdef USE_SHM
	unlock_shm_by_flock(fd);
#endif

#ifdef CONFIG_USER_DBUS_PROXY 
	if (ret != 0)
	{
		send_notify_msg_dbusproxy(id, e_dbus_signal_mib_chain_update, recordNum);
	}
#endif
	return ret;
}


int mib_chain_info_id(int id, mib_chain_record_table_entry_T * info)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_INFO_ID;
	mymsg->arg1 = id;

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("get mib chain info id failed\n");
		ret = 0;
	} else {
		memcpy(info, mymsg->mtext,
		       sizeof(mib_chain_record_table_entry_T));
		ret = 1;
	}

	return ret;
}

int mib_chain_info_index(int index, mib_chain_record_table_entry_T *info)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_INFO_INDEX;
	mymsg->arg1=index;
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("get mib chain info index failed\n");
		ret = 0;
	}
	else
		memcpy((void *)info, (void *)mymsg->mtext, sizeof(mib_chain_record_table_entry_T));

	return ret;
}

int mib_chain_info_name(char *name, mib_chain_record_table_entry_T *info)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_INFO_NAME;
	memset(mymsg->mtext, 0 ,MAX_SEND_SIZE);
	strncpy(mymsg->mtext, name, MAX_SEND_SIZE-1);
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("get mib chain info name failed\n");
		ret = 0;
	}
	else
		memcpy((void *)info, (void *)mymsg->mtext, sizeof(mib_chain_record_table_entry_T));

	return ret;
}

/*
 *	0: Request fail
 *	1: descriptor checking successful
 *	-1: descriptor checking failed
 */
int mib_check_desc(void)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHECK_DESC;
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("Check mib chain descriptors failed\n");
		return 0;
	}

	ret = mymsg->arg1;
	return ret;
}

static int mib_val_get_from_flash_output(FILE *file, char *token, char *value, int value_maxlen){
	char line[8096] = {0};
	char *val_start = NULL;
	int val_len, ret = 0;
	
	if (!file || !token || !value)
		return ret;

	while(!feof(file)){
		if (fgets(line, sizeof(line), file) && strstr(line, token)){
			val_start = strstr(line, token) + strlen(token);
			val_len = strlen(val_start) - 1; // delete '\n'
			strncpy(value, val_start, value_maxlen);
			ret = 1;
			break;
		}
	}
	
	return ret;
}


int mib_get_def(int id, void *value, int value_len){
	mib_table_entry_T mib_info = {0};
	char flash_cmd[128] = {0};
	FILE *pp = NULL;
	char val_str[8096] = {0};
	char token[64] = {0};
	int ret = 0;

	if (mib_info_id(id, &mib_info) == 0)
		return ret;
	
	if (mib_info.size > value_len){		
		printf("[%s:%d]: mib_entry_getDef() ID (%d,%s) size=%d, but value buffer size=%d\033[0m\n", 
			__FUNCTION__,__LINE__, id, mib_info.name, mib_info.size, value_len);
		return ret;
	}

	snprintf(flash_cmd, sizeof(flash_cmd), "flash get_def %s ", mib_info.name);	
	pp = popen(flash_cmd, "r");
	if (pp){
		snprintf(token, sizeof(token), "%s=", mib_info.name);
		if (mib_val_get_from_flash_output(pp, token, val_str, sizeof(val_str))){			
		/*	printf("[%s:%d] cmd = %s, val_str = %s\n", __FUNCTION__,__LINE__,flash_cmd,val_str); */		
			if (string_to_mib_ex(value, val_str, mib_info.type, mib_info.size, 1))
				ret = 0; //error
			else
				ret = 1;
		}		
		pclose(pp);
	}
	
	return ret;
}

int mib_chain_get_record_desc(int id, mib_chain_member_entry_T **record_desc, int *record_size)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg = NULL;
	int ret = 0;
	int size = 0;
	char *value = NULL;
#ifdef USE_SHM
	int fd;
#endif

#ifdef WLAN_SUPPORT
	id = wlan_mibchain_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_GET_RECORD_DESC;
	mymsg->arg1 = id;
	
#ifdef USE_SHM
	fd = lock_shm_by_flock();
#endif

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("get mib chain(MIB_ID=%d) record_desc failed\n", id);
		ret = 0;
	} else {
		size = mymsg->arg1;
		value = malloc(size);
		if (value == NULL){
			printf("[%s:%d] MIB_ID(%d), malloc size(%d) failed!\n",__FUNCTION__,__LINE__,id,size);
			ret = 0;
			goto ret_func;
		}
			
#ifdef USE_SHM
		if (get_shm_data(value, size) < 0){
			free(value);
			ret = 0;
		}
		else
			ret = 1;
#else
		memcpy(value, mymsg->mtext, size);
		ret = 1;
#endif	
		if (ret == 1){
			*record_size = size;
			*record_desc = (mib_chain_member_entry_T *)value;
		}	
	}

ret_func:	
#ifdef USE_SHM
	unlock_shm_by_flock(fd);
#endif

	return ret;
}

int mib_chain_total_def(int id){
	mib_chain_record_table_entry_T mib_chain_info = {0};
	char flash_cmd[128] = {0};
	FILE *pp = NULL;
	char val_str[8096] = {0};
	char token[64] = {0};
	int ret = 0;

	if (mib_chain_info_id(id, &mib_chain_info) == 0)
		return ret;

	snprintf(flash_cmd, sizeof(flash_cmd), "flash get_def %s.NUM",mib_chain_info.name);	
	pp = popen(flash_cmd, "r");
	if (pp){
		snprintf(token, sizeof(token), "%s.NUM=", mib_chain_info.name);
		if (mib_val_get_from_flash_output(pp, token, val_str, sizeof(val_str))){
		/*	printf("[%s:%d] cmd = %s, val_str = %s\n", __FUNCTION__,__LINE__, flash_cmd, val_str); */
			ret = atoi(val_str);
		}		
		pclose(pp);
	}
	return ret;
}

int mib_chain_get_def(int id, unsigned int recordNum, void *value){
	mib_chain_record_table_entry_T mib_chain_info = {0};
	mib_chain_member_entry_T *record_desc = NULL;
	int record_size = 0, item_nums = 0, item_idx = 0;
	void *pChainEntry = NULL;
	char flash_cmd[128] = {0};
	FILE *pp = NULL;
	char line[8096] = {0};
	char mib_name[128] = {0};
	char val_str[8096] = {0};
	char token[64] = {0};
	int ret = 0;

	if (recordNum >= mib_chain_total_def(id))
		return ret;
	
	if (mib_chain_info_id(id, &mib_chain_info) == 0)
		return ret;

	if (mib_chain_get_record_desc(id, &record_desc, &record_size) == 0)
		return ret;
	
	pChainEntry = malloc(mib_chain_info.per_record_size);
	if (pChainEntry == NULL){
		printf("[%s:%d] mib_name=%s, malloc(%d) failed\n", __FUNCTION__,__LINE__,mib_chain_info.name, mib_chain_info.per_record_size);
		free(record_desc);
		return ret;
	}
	memset(pChainEntry, 0x00, mib_chain_info.per_record_size);
	
	item_nums = record_size/sizeof(mib_chain_member_entry_T);
	while (item_idx < item_nums){/*0~item_nums-1*/			
		snprintf(flash_cmd, sizeof(flash_cmd), "flash get_def %s.%d.%s",mib_chain_info.name, recordNum, (record_desc+item_idx)->name);
		pp = popen(flash_cmd, "r");
		if (pp){
			snprintf(token, sizeof(token), "%s=", (record_desc+item_idx)->name);
			memset(val_str, 0x00, sizeof(val_str));
			if (mib_val_get_from_flash_output(pp, token, val_str, sizeof(val_str))){
			/*	printf("[%s:%d] cmd=%s, val_str = %s\n\n", __FUNCTION__,__LINE__, flash_cmd, val_str); */
				if (string_to_mib_ex(pChainEntry+(record_desc+item_idx)->offset, val_str, (record_desc+item_idx)->type, (record_desc+item_idx)->size, 1))
					ret = 0; //error
				else{
					ret = 1;		
				}
			}		
		
			pclose(pp);
		}
		
		item_idx++;
	}

	memcpy(value, pChainEntry, mib_chain_info.per_record_size);
	free(pChainEntry);
	free(record_desc);
	
	return ret;
}

#include <setjmp.h>
#define MAX_REBOOT_TIMEOUT 10
static sigjmp_buf _jmpb;
static void timeout_handler(int signo)
{
	if(signo == SIGALRM) {
		printf("system reboot timeout...\n");
		siglongjmp(_jmpb, 1);
	}
}

int _cmd_reboot(int doReboot)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	struct sigaction sa;
	int ret = 1;
	
	//set jump for timeout
	if(sigsetjmp(_jmpb, 1) != 0){
		return 0;
	}
	
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = timeout_handler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);
    alarm(MAX_REBOOT_TIMEOUT);

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_REBOOT;
	mymsg->arg1 = doReboot;

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("reboot failed\n");
		ret = 0;
	}
	alarm(0);

	return ret;
}

int cmd_reboot(void)
{
	return _cmd_reboot(1);
}

#ifdef CONFIG_IPV6
#if defined(DHCPV6_ISC_DHCP_4XX)
int cmd_stop_delegation(const char *fname)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_STOP_DELEGATION;
	strncpy(mymsg->mtext, fname, MAX_SEND_SIZE-1);
	mymsg->mtext[MAX_SEND_SIZE-1] = '\0';
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("Stop Delegation failed\n");
		ret = 0;
	}

	return ret;
}

int cmd_get_PD_prefix_ip(void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;
	memset(&qbuf, 0x0, sizeof(struct mymsgbuf));

    mymsg = &qbuf.msg;
    mymsg->cmd = CMD_GET_PD_PREFIX_IP;
	sendcmd(&qbuf);
	if (qbuf.request == MSG_SUCC) {
		memcpy(value, mymsg->mtext, IP6_ADDR_LEN);
		ret = 1;
	}
	else {
		printf("CMD_GET_PD_IP_PREFIX failed\n");
		ret = 0;
	}
	return ret;
}

int cmd_get_PD_prefix_len(void)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret=1;
	memset(&qbuf, 0x0, sizeof(struct mymsgbuf));

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_GET_PD_PREFIX_LEN;
	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("CMD_GET_PD_IP_PREFIX  failed\n");
		ret = 0;
	}
	else
		ret = qbuf.msg.arg1;
	return ret;
}
#endif
#endif  // #ifdef CONFIG_IPV6

int cmd_killproc(unsigned int mask)
{
	int ret=1;
	char str_mask[16];
	snprintf(str_mask, sizeof(str_mask), "%u", mask);
	if (sendUnixSockMsg(0, 3, SYSTEMD_USOCK_SERVER, "do_killproc", str_mask)) {
		printf("kill processes failed\n");
		ret = 0;
	}

	return ret;
}

#ifdef CONFIG_E8B
int cmd_upload(const char *fname, int offset, int imgFileSize, int needreboot)
#else
#ifdef USE_DEONET
int cmd_upload(const char *fname, int offset, int imgFileSize, int needreboot, int needcommit, unsigned int version)
#else
int cmd_upload(const char *fname, int offset, int imgFileSize)
#endif
#endif
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_UPLOAD;
	mymsg->arg1 = 0;
	mymsg->arg2 = imgFileSize;
#ifdef CONFIG_E8B
	mymsg->arg3 = needreboot;
#else
#ifdef USE_DEONET
	mymsg->arg3 = 0;
	if(needcommit)
		mymsg->arg3 |= UPGRADE_FLAG_COMMIT;
	if(needreboot)
		mymsg->arg3 |= UPGRADE_FLAG_REBOOT;
	mymsg->arg3 |= version << UPGRADE_FLAG_VERSION_SHIFT;
#else
	mymsg->arg3 = 0;
#endif
#endif
	if (imgFileSize != 0) {
		mymsg->arg1 = offset;
		strncpy(mymsg->mtext, fname, MAX_SEND_SIZE - 1);
		mymsg->mtext[MAX_SEND_SIZE - 1] = '\0';
	}

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("firmware upgrade failed\n");
		ret = 0;
	}

	return ret;
}

#ifdef CONFIG_LUNA_FIRMWARE_UPGRADE_SUPPORT
int cmd_upload_ex(const char *fname, int offset, int imgFileSize, int needreboot)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_UPLOAD;
	mymsg->arg1 = 0;
	mymsg->arg2 = imgFileSize;
	mymsg->arg3 = needreboot;

	if (imgFileSize != 0) {
		mymsg->arg1 = offset;
		strncpy(mymsg->mtext, fname, MAX_SEND_SIZE - 1);
		mymsg->mtext[MAX_SEND_SIZE - 1] = '\0';
	}

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("firmware upgrade failed\n");
		ret = 0;
	}

	return ret;
}
#endif

#ifndef CONFIG_LUNA_FIRMWARE_UPGRADE_SUPPORT
int cmd_check_image(const char *fname, int offset)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHECK_IMAGE;
	mymsg->arg1 = offset;
	strncpy(mymsg->mtext, fname, MAX_SEND_SIZE - 1);
	mymsg->mtext[MAX_SEND_SIZE - 1] = '\0';

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("firmware check failed\n");
		ret = 0;
	}

	return ret;
}
#endif

// Kaohj -- Translating xml file
/*
 * Translate file (fname) to xml (xname)
 * fname: file name of the [encrypted] file (getting from outside world)
 * xname: file name of the xml-formatted file for local process
 */
int cmd_file2xml(const char *fname, const char *xname)
{
	int ret=1;
	if (sendUnixSockMsg(0, 4, SYSTEMD_USOCK_SERVER, "do_file2xml", fname, xname)) {
		printf("File to XML failed\n");
		ret = 0;
	}

	return ret;
}

/*
 * Translate xml (xname) to file (fname)
 * xname: file name of the local-generated xml-formatted file
 * fname: file name of the [encrypted] file (to be transferred)
 */
int cmd_xml2file(const char *xname, const char *fname)
{
	int ret=1;
	if (sendUnixSockMsg(0, 4, SYSTEMD_USOCK_SERVER, "do_xml2file", xname, fname)) {
		printf("XML to File failed\n");
		ret = 0;
	}

	return ret;
}

#ifdef CONFIG_RTL8676_CHECK_WIFISTATUS
/*
 * Sleep 3 seconds and Check Wi-Fi status.
 * If Wi-Fi start failed, restart Wi-Fi.
 */
int cmd_wlan_delay_restart()
{
	int retry = 0;
	int delay = 10;

	int i,j,k;

	while(retry < 3)
	{
		if(retry == 2)
			delay = 5;	  // delay 5 seconds in third try

		fprintf(stderr, "Delay %d seconds...\n", delay);
		//usleep(10000000);
		for(i=0;i<0x8fff;i++)
			for(j=0;j<0xfff;j++)
				k = j + i;
		// see if WLAN interface is ready or not
		if (!getInFlags((char *)getWlanIfName(), 0)) {
			retry++;
			fprintf(stderr, "WLAN interface is no ready, try again.\n");
			continue;	   //not ready, try again.
		}

		fprintf(stderr, "Restarting WLAN...\n");
		config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);

		return 0;
	}
	return -1;
}
#endif

//IPv6 MAP-E
#ifdef CONFIG_USER_MAP_E
int cmd_mape_static_mode(char *ifname){
	char cmd[256];

	snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_mape_static_mode %s", ifname);
	system(cmd);

	return 1;
}
#endif


int mib_set_PPPoE(int cmd, void *value, unsigned int length)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	if (length >= MAX_SEND_SIZE) {
		printf("set_PPPoE: PPPoE session info size(%d) overflow (max. %d).\n", length, MAX_SEND_SIZE);
		return 0;
	}
	mymsg = &qbuf.msg;
	mymsg->cmd = cmd;
	mymsg->arg1 = (int)length;
	memcpy(mymsg->mtext, value, length);

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("save PPPoE session failed\n");
		ret = 0;
	}

	return ret;
}

#ifdef CONFIG_REMOTE_CONFIGD
static int rconfigd_socket;
static size_t max_send_size;
static struct sockaddr_in *paddr;

#ifdef CONFIG_SPC
int rconfigd_init(void)
{
	rconfigd_socket = socket(PF_SPC, SOCK_DGRAM, 0);

	if (rconfigd_socket < 0) {
		perror("socket");
		return 0;
	}

	paddr = NULL;

	/* minus 1 for subtype */
	max_send_size = ETH_DATA_LEN - 1;
	if (max_send_size > sizeof(struct mymsgbuf))
		max_send_size = sizeof(struct mymsgbuf);

	return 1;
}
#else
int rconfigd_init(void)
{
	static struct sockaddr_in addr;
	int broadcast = 1;
	struct in_addr ip, subnet;

	rconfigd_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (rconfigd_socket < 0) {
		perror("socket");
		return 0;
	}

	if (setsockopt(rconfigd_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0)
		fprintf(stderr, "%s:%d setsockopt: %s\n", __FUNCTION__, __LINE__, strerror(errno));

	mib_get_s(MIB_ADSL_LAN_IP, &ip, sizeof(ip));
	mib_get_s(MIB_ADSL_LAN_SUBNET, &subnet, sizeof(subnet));

	addr.sin_addr.s_addr = ip.s_addr | ~subnet.s_addr;
	addr.sin_port = htons(8809);
	addr.sin_family = PF_INET;
	paddr = &addr;

	max_send_size = sizeof(struct mymsgbuf);

	return 1;
}
#endif

void rconfigd_exit(void)
{
	close(rconfigd_socket);
}

int mib_info_id_via_rconfigd(int id, mib_table_entry_T * info)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_INFO_ID;
	mymsg->arg1 = id;

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	recv(rconfigd_socket, &qbuf, sizeof(qbuf), 0);
	if (qbuf.request == MSG_SUCC) {
		memcpy(info, mymsg->mtext, sizeof(mib_table_entry_T));
		ret = 1;
	} else {
		printf("%s: get mib info id failed! (id=%d)\n", __func__, id);
		ret = 0;
	}

	return ret;
}

int mib_get_via_rconfigd(int id, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_table_entry_T info;
	int ret;

	if (!mib_info_id_via_rconfigd(id, &info))
		return 0;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_GET;
	mymsg->arg1 = id;

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	recv(rconfigd_socket, &qbuf, sizeof(qbuf), 0);
	if (qbuf.request == MSG_SUCC) {
		memcpy(value, mymsg->mtext, info.size);
		ret = 1;
	} else {
		printf("%s: Get request failed! (id=%d)\n", __func__, id);
		ret = 0;
	}

	return ret;
}

int mib_set_via_rconfigd(int id, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_table_entry_T info;
	int ret = 1;

	if (!mib_info_id_via_rconfigd(id, &info))
		return 0;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_SET;
	mymsg->arg1 = id;
#ifdef CONFIG_USER_CWMP_TR069
	mymsg->arg2 = 1;
#endif
	memcpy(mymsg->mtext, value, info.size);

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	recv(rconfigd_socket, &qbuf, sizeof(qbuf), 0);
	if (qbuf.request != MSG_SUCC) {
		printf("%s: Set request failed! (id=%d)\n", __func__, id);
		ret = 0;
	}

	return ret;
}

int mib_update_via_rconfigd(CONFIG_DATA_T type, CONFIG_MIB_T flag)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_UPDATE;
	mymsg->arg1 = (int)type;
	mymsg->arg2 = (int)flag;

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	recv(rconfigd_socket, &qbuf, sizeof(qbuf), 0);
	if (qbuf.request != MSG_SUCC) {
		printf("mib update failed\n");
		ret = 0;
	}

	return ret;
}

int cmd_reboot_via_rconfigd(void)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_REBOOT;

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	recv(rconfigd_socket, &qbuf, sizeof(qbuf), 0);
	if (qbuf.request != MSG_SUCC) {
		printf("reboot failed\n");
		ret = 0;
	}

	return ret;
}

int mib_chain_total_via_rconfigd(int id)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_TOTAL;
	mymsg->arg1 = id;

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	read(rconfigd_socket, &qbuf, sizeof(qbuf));
	if (qbuf.request == MSG_SUCC) {
		ret = qbuf.msg.arg1;
	} else {
		printf("get total failed\n");
		ret = 0;
	}

	return ret;
}

int mib_chain_info_id_via_rconfigd(int id, mib_chain_record_table_entry_T * info)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_INFO_ID;
	mymsg->arg1 = id;

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	read(rconfigd_socket, &qbuf, sizeof(qbuf));
	if (qbuf.request != MSG_SUCC) {
		printf("get mib chain info id failed\n");
		ret = 0;
	} else {
		memcpy(info, mymsg->mtext,
		       sizeof(mib_chain_record_table_entry_T));
		ret = 1;
	}

	return ret;
}

int mib_chain_get_via_rconfigd(int id, unsigned int recordNum, void *value)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_chain_record_table_entry_T info;
	int ret;
	int size;

	if (!mib_chain_info_id_via_rconfigd(id, &info))
		return 0;

	size = info.per_record_size;
	if (size >= max_send_size) {
		printf("chain_get: chain record size(%d) overflow (max. %d).\n",
		       size, max_send_size);
		return 0;
	}

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_GET;
	mymsg->arg1 = id;
	sprintf(mymsg->mtext, "%u", recordNum);

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	read(rconfigd_socket, &qbuf, sizeof(qbuf));
	if (qbuf.request != MSG_SUCC) {
		printf("get chain failed,id=%d,recordNum=%d,%s:%d\n",id,recordNum,__func__,__LINE__);
		ret = 0;
	} else {
		memcpy(value, mymsg->mtext, size);
		ret = 1;
	}

	return ret;
}

int mib_chain_update_via_rconfigd(int id, void *ptr, unsigned int recordNum)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_chain_record_table_entry_T info;
	int ret = 1;
	int size;

	if (!mib_chain_info_id_via_rconfigd(id, &info))
		return 0;

	size = info.per_record_size;
	if (size >= max_send_size) {
		printf
		    ("chain_update: chain record size(%d) overflow (max. %d).\n",
		     size, max_send_size);
		return 0;
	}

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_UPDATE;
	mymsg->arg1 = id;
	mymsg->arg2 = recordNum;
	memcpy(mymsg->mtext, ptr, size);

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	recv(rconfigd_socket, &qbuf, sizeof(qbuf), 0);
	if (qbuf.request != MSG_SUCC) {
		printf("update chain failed\n");
		ret = 0;
	}

	return ret;
}

int mib_chain_add_via_rconfigd(int id, void *ptr)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	mib_chain_record_table_entry_T info;
	int ret;
	int size;

	if (!mib_chain_info_id_via_rconfigd(id, &info))
		return 0;

	size = mib_chain_total_via_rconfigd(id);
	ret = size + 1;
	if (info.table_size != -1 && size >= info.table_size)
		return -1;
	size = info.per_record_size;
	if (size >= max_send_size) {
		printf("chain_add: chain record size(%d) overflow (max. %d).\n",
		       size, max_send_size);
		return 0;
	}

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_ADD;
	mymsg->arg1 = id;
	memcpy(mymsg->mtext, ptr, size);

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	recv(rconfigd_socket, &qbuf, sizeof(qbuf), 0);
	if (qbuf.request != MSG_SUCC) {
		printf("add chain failed\n");
		ret = 0;
	}

	return ret;
}

int mib_chain_delete_via_rconfigd(int id, unsigned int recordNum)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

#ifdef WLAN_SUPPORT
	id = wlan_mibchain_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_DELETE;
	mymsg->arg1 = id;
	sprintf(mymsg->mtext, "%u", recordNum);

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	recv(rconfigd_socket, &qbuf, sizeof(qbuf), 0);
	if (qbuf.request != MSG_SUCC) {
		printf("delete chain failed\n");
		ret = 0;
	}

	return ret;
}

int mib_chain_clear_via_rconfigd(int id)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

#ifdef WLAN_SUPPORT
	id = wlan_mibchain_id_reorder(id);
	if (id == -1) {
		printf("%s: Invalid wlan_idx: %d\n", __FUNCTION__, wlan_idx);
		return 0;
	}
#endif

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHAIN_CLEAR;
	mymsg->arg1 = id;

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	recv(rconfigd_socket, &qbuf, sizeof(qbuf), 0);
	if (qbuf.request != MSG_SUCC) {
		printf("clear chain failed\n");
		ret = 0;
	}

	return ret;
}

int mib_flash_to_default_via_rconfigd(CONFIG_DATA_T type)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_MIB_FLASH_TO_DEFAULT;
	mymsg->arg1 = (int)type;

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	recv(rconfigd_socket, &qbuf, sizeof(qbuf), 0);
	if (qbuf.request != MSG_SUCC) {
		printf("mib reset_flash_to_default failed\n");
		ret = 0;
	}

	return ret;
}

int cmd_check_image_via_rconfigd(const char *fname, int offset)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CHECK_IMAGE;
	mymsg->arg1 = offset;
	strncpy(mymsg->mtext, fname, MAX_SEND_SIZE - 1);
	mymsg->mtext[MAX_SEND_SIZE - 1] = '\0';

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	recv(rconfigd_socket, &qbuf, sizeof(qbuf), 0);
	if (qbuf.request != MSG_SUCC) {
		printf("firmware check failed\n");
		ret = 0;
	}

	return ret;
}

int cmd_upload_via_rconfigd(const char *fname, int offset, int imgFileSize)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_UPLOAD;
	mymsg->arg2 = imgFileSize;
	if (imgFileSize != 0) {
		mymsg->arg1 = offset;
		strncpy(mymsg->mtext, fname, MAX_SEND_SIZE - 1);
		mymsg->mtext[MAX_SEND_SIZE - 1] = '\0';
	}

	sendto(rconfigd_socket, &qbuf, max_send_size, 0, (struct sockaddr *)paddr, sizeof(*paddr));
	recv(rconfigd_socket, &qbuf, sizeof(qbuf), 0);
	if (qbuf.request != MSG_SUCC) {
		printf("firmware upgrade failed\n");
		ret = 0;
	}

	return ret;
}


#endif

#if defined(CONFIG_USER_PPPOE_PROXY) || defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
int cmd_add_policy_routing_rule(int lan_unit, int wan_unit)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_ADD_POLICY_RULE;
	mymsg->arg1 = lan_unit;
	mymsg->arg2 = wan_unit;

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("%s failed\n", __FUNCTION__);
		ret = 0;
	}

	return ret;
}

int cmd_del_policy_routing_rule(int lan_unit, int wan_uint)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_DEL_POLICY_RULE;
	mymsg->arg1 = lan_unit;
	mymsg->arg2 = wan_uint;

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("%s failed\n", __FUNCTION__);
		ret = 0;
	}

	return ret;
}

int cmd_add_policy_routing_table(int wan_uint)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_ADD_POLICY_TABLE;
	mymsg->arg1 = wan_uint;

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("%s failed\n", __FUNCTION__);
		ret = 0;
	}

	return ret;
}

int cmd_del_policy_routing_table(int wan_uint)
{
	struct mymsgbuf qbuf;
	MSG_T *mymsg;
	int ret = 1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_DEL_POLICY_TABLE;
	mymsg->arg1 = wan_uint;

	sendcmd(&qbuf);
	if (qbuf.request != MSG_SUCC) {
		printf("%s failed\n", __FUNCTION__);
		ret = 0;
	}

	return ret;
}
#endif


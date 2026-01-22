/*
 *	msgparser.c -- Parser for an well-formed message
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <assert.h>
#include "../msgq.h"
#include "mibtbl.h"
#include "utility.h"
#include "subr_net.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <time.h>
#ifdef CONFIG_NET_IPGRE
#include <linux/route.h>
#endif

#ifdef EMBED
#include <config/autoconf.h>
#else
#include "../../../../config/autoconf.h"
#endif
#ifdef VOIP_SUPPORT
#include "web_voip.h"
#endif
#include <linux/version.h>
#if defined(DHCPV6_ISC_DHCP_4XX)
#include "subr_dhcpv6.h"
#endif

#ifdef	RESERVE_KEY_SETTING
#include "mib_reserve.h"
#endif

#include "ipv6_info.h"
#include <signal.h>
//#ifdef CONFIG_NET_IPGRE
//#include <net/route.h>
//#endif

#define MAX_ARGS	3
#define MAX_ARG_LEN	32

#ifdef CONFIG_IPV6
#if defined(DHCPV6_ISC_DHCP_4XX)
static DLG_INFO_T g_dlg_info;
#endif
#endif

#if defined(CONFIG_GPON_FEATURE)
#include "rtk/gpon.h"
#endif
#ifdef CONFIG_LIB_LIBRTK_LED
#include "librtk_led/librtk_led.h"
#endif

extern int shm_id;
extern char *shm_start;
extern char g_upload_post_file_name[MAX_SEND_SIZE];
extern int g_upload_startPos;
extern int g_upload_fileSize;
extern int g_upload_needreboot;
extern int g_upload_needcommit;
extern int g_upload_version;

#if defined(CONFIG_AIRTEL_EMBEDUR)
static int is_fwupgrade = 0;
#endif
static int parse_token(char *buf, char argv[MAX_ARGS][MAX_ARG_LEN+1]);
//static void cfg_get(int argc, char argv[MAX_ARGS][MAX_ARG_LEN+1], struct mymsgbuf *qbuf);
static void cfg_mib_get(struct mymsgbuf *qbuf);
//static void cfg_set(int argc, char argv[MAX_ARGS][MAX_ARG_LEN+1], struct mymsgbuf *qbuf);
static void cfg_mib_set(struct mymsgbuf *qbuf);
static void cfg_mib_info_id(struct mymsgbuf *qbuf);
static void cfg_mib_info_index(struct mymsgbuf *qbuf);
static void cfg_mib_info_total(struct mymsgbuf *qbuf);  // For Star Zhang's fast load
static void cfg_mib_backup(struct mymsgbuf *qbuf);
static void cfg_mib_restore(struct mymsgbuf *qbuf);
static void cfg_mib_get_default(struct mymsgbuf *qbuf);
static void cfg_mib_swap(struct mymsgbuf *qbuf);
static void cfg_mib_to_default(struct mymsgbuf *qbuf);
static void cfg_mib_flash_to_default(struct mymsgbuf *qbuf);
#if (defined VOIP_SUPPORT) && (defined CONFIG_USER_XMLCONFIG)
static void cfg_mib_voip_to_default(struct mymsgbuf *qbuf);
#endif
/*
static void cfg_mib_size(struct mymsgbuf *qbuf);
static void cfg_mib_type(struct mymsgbuf *qbuf);
*/
static void cfg_chain_total(struct mymsgbuf *qbuf);
static void cfg_chain_get(struct mymsgbuf *qbuf);
static void cfg_chain_add(struct mymsgbuf *qbuf);
static void cfg_chain_delete(struct mymsgbuf *qbuf);
static void cfg_chain_clear(struct mymsgbuf *qbuf);
static void cfg_chain_update(struct mymsgbuf *qbuf);
static void cfg_chain_swap(struct mymsgbuf *qbuf);
static void cfg_chain_info_id(struct mymsgbuf *qbuf);
static void cfg_chain_info_index(struct mymsgbuf *qbuf);
static void cfg_chain_info_name(struct mymsgbuf *qbuf);
static void cfg_chain_record_desc(struct mymsgbuf *qbuf);
static void cfg_check_desc(struct mymsgbuf *qbuf);

static void cfg_mib_lock(struct mymsgbuf *qbuf);
static void cfg_mib_unlock(struct mymsgbuf *qbuf);
static void cfg_mib_update_from_raw(struct mymsgbuf *qbuf);
static void cfg_mib_read_to_raw(struct mymsgbuf *qbuf);
static void cfg_mib_update(struct mymsgbuf *qbuf);
static void cfg_mib_read_header(struct mymsgbuf *qbuf);
static void cfg_mib_reload(struct mymsgbuf *qbuf);
#ifdef EMBED
static void cfg_reboot(struct mymsgbuf *qbuf);
#ifdef CONFIG_IPV6
#if defined(DHCPV6_ISC_DHCP_4XX)
static void cfg_stop_delegation(struct mymsgbuf *qbuf);
static void cfg_get_PD_prefix_ip(struct mymsgbuf *qbuf);
static void cfg_get_PD_prefix_len(struct mymsgbuf *qbuf);
#endif

#ifdef CONFIG_USER_MULTI_DHCPD6_INTERFACE
void _stop_multi_wide_Dhcp6sService(void);
#endif
#endif
static void cfg_upload(struct mymsgbuf *qbuf);
static void cfg_check_image(struct mymsgbuf *qbuf);
#ifdef CONFIG_DEV_xDSL
#ifdef AUTO_PVC_SEARCH_AUTOHUNT
static void cfg_start_autohunt(struct mymsgbuf *qbuf);
#endif
#endif
#endif
///added by ql
static void cfg_retrieve_table(struct mymsgbuf *qbuf);
static void cfg_retrieve_chain(struct mymsgbuf *qbuf);
static void cfg_update_PPPoE_session(struct mymsgbuf *qbuf);
static void cfg_mib_set_PPPoE(struct mymsgbuf *qbuf);

//#ifdef CONFIG_NET_IPGRE
//static void cfg_set_gre(struct mymsgbuf *qbuf);
//#endif
static void cfg_chain_backup_get(struct mymsgbuf *qbuf);
static void cfg_chain_get_def(struct mymsgbuf *qbuf);
static void cfg_backup_get(struct mymsgbuf *qbuf);

#ifdef VOIP_SUPPORT
int set_VoIP_proxy_dnscfg(FILE *dnsfp,char *dns_str,  char activeVOIP);
#endif
static MIB_CE_ATM_VC_T * getATM_VC_ENTRY_byName(char *pIfname, int *entry_index);

struct command
{
	int	needs_arg;
	int	cmd;
	void	(*func)(struct mymsgbuf *qbuf);
};

volatile int __mib_lock = 0;
MIB_T table_backup;
unsigned char *chain_backup = NULL;
unsigned int backupChainSize = 0;
static volatile int reboot_now = 0;

static struct command commands[] = {
	{1, CMD_MIB_GET, cfg_mib_get},
	{1, CMD_MIB_SET, cfg_mib_set},
	{1, CMD_MIB_INFO_ID, cfg_mib_info_id},
	{1, CMD_MIB_INFO_INDEX, cfg_mib_info_index},
	{1, CMD_MIB_INFO_TOTAL, cfg_mib_info_total},  // For Star Zhang's fast load
	{1, CMD_MIB_BACKUP, cfg_mib_backup},
	{1, CMD_MIB_RESTORE, cfg_mib_restore},
	{1, CMD_MIB_GET_DEFAULT, cfg_mib_get_default},
	{1, CMD_MIB_SWAP, cfg_mib_swap},
	{1, CMD_MIB_TO_DEFAULT, cfg_mib_to_default},
	{1, CMD_MIB_FLASH_TO_DEFAULT, cfg_mib_flash_to_default},
#if (defined VOIP_SUPPORT) && (defined CONFIG_USER_XMLCONFIG)
	{1, CMD_MIB_VOIP_TO_DEFAULT, cfg_mib_voip_to_default},
#endif
	/*
	{1, CMD_MIB_SIZE, cfg_mib_size},
	{1, CMD_MIB_TYPE, cfg_mib_type},
	*/
	{1, CMD_CHAIN_TOTAL, cfg_chain_total},
	{1, CMD_CHAIN_GET, cfg_chain_get},
	{1, CMD_CHAIN_ADD, cfg_chain_add},
	{1, CMD_CHAIN_DELETE, cfg_chain_delete},
	{1, CMD_CHAIN_CLEAR, cfg_chain_clear},
	{1, CMD_CHAIN_UPDATE, cfg_chain_update},
	{1, CMD_CHAIN_INFO_ID, cfg_chain_info_id},
	{1, CMD_CHAIN_INFO_INDEX, cfg_chain_info_index},
	{1, CMD_CHAIN_INFO_NAME, cfg_chain_info_name},
	{1, CMD_CHECK_DESC, cfg_check_desc},
	{1, CMD_CHAIN_SWAP, cfg_chain_swap},
	{1, CMD_CHAIN_GET_RECORD_DESC, cfg_chain_record_desc},
	/*
	{1, CMD_CHAIN_SIZE, cfg_chain_size},
	*/
	{1, CMD_MIB_LOCK, cfg_mib_lock},
	{1, CMD_MIB_UNLOCK, cfg_mib_unlock},
#if !defined(CONFIG_USER_XMLCONFIG) && !defined(CONFIG_USER_CONF_ON_XMLFILE)
	{1, CMD_MIB_UPDATE_FROM_RAW, cfg_mib_update_from_raw},
	{1, CMD_MIB_READ_TO_RAW, cfg_mib_read_to_raw},
	{1, CMD_MIB_READ_HEADER, cfg_mib_read_header},
#endif
	{1, CMD_MIB_UPDATE, cfg_mib_update},
	{1, CMD_MIB_RELOAD, cfg_mib_reload},
#ifdef EMBED
	{1, CMD_REBOOT, cfg_reboot},
	{1, CMD_UPLOAD, cfg_upload},
//	{1, CMD_KILLPROC, cfg_killprocess},
	{1, CMD_CHECK_IMAGE, cfg_check_image },
#ifdef CONFIG_DEV_xDSL
#ifdef AUTO_PVC_SEARCH_AUTOHUNT
	{1, CMD_START_AUTOHUNT, cfg_start_autohunt},
#endif
#endif
//	{1, CMD_FILE2XML, cfg_file2xml},
//	{1, CMD_XML2FILE, cfg_xml2file},
#endif
#ifdef	RESERVE_KEY_SETTING
	{1, CMD_MIB_RETRIVE_TABLE, cfg_retrieve_table},
	{1, CMD_MIB_RETRIVE_CHAIN, cfg_retrieve_chain},
#endif
	{1, CMD_UPDATE_PPPOE_SESSION, cfg_update_PPPoE_session},
	{1, CMD_MIB_SAVE_PPPOE, cfg_mib_set_PPPoE},
#ifdef CONFIG_IPV6
#if defined(DHCPV6_ISC_DHCP_4XX)
	{1, CMD_STOP_DELEGATION, cfg_stop_delegation},
	{1, CMD_GET_PD_PREFIX_IP, cfg_get_PD_prefix_ip},
	{1, CMD_GET_PD_PREFIX_LEN, cfg_get_PD_prefix_len},
#endif
#endif

//#ifdef CONFIG_NET_IPGRE
//	{1, CMD_SET_GRE, cfg_set_gre},
//#endif
	{1, CMD_MIB_CHAIN_BACKUP_GET, cfg_chain_backup_get},
	{1, CMD_MIB_CHAIN_GET_DEFAULT, cfg_chain_get_def},
	{1, CMD_MIB_BACKUP_GET, cfg_backup_get},
	{0, 0, NULL}
};

int msgProcess(struct mymsgbuf *qbuf)
{
  	int argc, c;
	char argv[MAX_ARGS][MAX_ARG_LEN+1];

	// response message type should be the client request magic number
	qbuf->mtype = qbuf->request;
	/*
	if ((argc=parse_token(qbuf->mtext, argv)) == 0)
		return 0;

	for(c=0; commands[c].name!=NULL; c++) {
		if(!strcmp(argv[0], commands[c].name)) {
			argc--;
			if(argc >= commands[c].num_string_arg)
				commands[c].func(argc, (char **)(&argv[1]), qbuf);
			break;
		}
	}
	*/
	for (c=0; commands[c].cmd!=0; c++) {
		if (qbuf->msg.cmd == commands[c].cmd) {
			commands[c].func(qbuf);
			break;
		}
	}
	return 0;
}

/******************************************************************************/
/*
 *	Token Parser -- parse tokens seperated by spaces on buf
 *	Return: number of tokens been parsed
 */

#if 0
static int parse_token(char *buf, char argv[MAX_ARGS][MAX_ARG_LEN+1])
{
  	int num, arg_idx, i;
  	char *arg_ptr;

	num = 0;
	arg_idx = 0;

	for(i=0; buf[i]!='\0'; i++) {
		if(buf[i]==' '){
			if (arg_idx != 0) {	// skip multiple spaces
				argv[num][arg_idx]='\0';
				num++;
				arg_idx=0;
				if (num == MAX_ARGS)
					break;
			}
		}
		else {
			if(arg_idx<MAX_ARG_LEN) {
				argv[num][arg_idx]=buf[i];
				arg_idx++;
			}
		}
	}

	if (arg_idx != 0) {
		argv[num][arg_idx]='\0';
		num++;
	}

	return num;
}
#endif

int _getIfIndexByName(char *pIfname)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T *pEntry=NULL;
	char ifname[IFNAMSIZ];

	entryNum = _mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		pEntry = (MIB_CE_ATM_VC_T *)_mib_chain_get(MIB_ATM_VC_TBL, i);
		if (!pEntry)
		{
  			printf("Get chain record error!\n");
			return -1;
		}

		if (pEntry->enable == 0)
			continue;

		ifGetName(pEntry->ifIndex,ifname,sizeof(ifname));

		if(!strcmp(ifname,pIfname)){
			break;
		}
	}

	if(i>= entryNum){
		//printf("not find this interface!\n");
		return DUMMY_IFINDEX;
	}

	return(pEntry->ifIndex);
}

#ifdef CONFIG_USER_XMLCONFIG
extern const char *shell_name;
static int xml_mib_update(CONFIG_DATA_T type, CONFIG_MIB_T flag)
{
	int ret=1;
	char param[6];

	printf("%s():...\n", __FUNCTION__);

	if (type == CURRENT_SETTING)
		strcpy (param, "-u cs");
	else if (type == HW_SETTING)
		strcpy (param, "-u hs");

	if (va_cmd (shell_name, 2, 0, "/etc/scripts/config_xmlconfig.sh", param) != MSG_SUCC) {
		printf ("[xmlconfig] mib update %s failed\n", param);
		ret = 0;
	}

	return ret;
}

#endif /*CONFIG_USER_XMLCONFIG */

static void cfg_mib_lock(struct mymsgbuf *qbuf)
{
	__mib_lock = 1;
	qbuf->request = MSG_SUCC;
}

static void cfg_mib_unlock(struct mymsgbuf *qbuf)
{
	__mib_lock = 0;
	qbuf->request = MSG_SUCC;
}

#if !defined(CONFIG_USER_XMLCONFIG) && !defined(CONFIG_USER_CONF_ON_XMLFILE)
static void cfg_mib_update_from_raw(struct mymsgbuf *qbuf)
{
	int len;

	qbuf->request = MSG_FAIL;
	len = qbuf->msg.arg1;

	//printf("update_from_raw: shm_id=%d; shm_start=0x%x\n", shm_id, shm_start);
	if(shm_start && _mib_update_from_raw(shm_start, len) == 1)
		qbuf->request = MSG_SUCC;
}

static void cfg_mib_read_to_raw(struct mymsgbuf *qbuf)
{
	CONFIG_DATA_T data_type;
	int len;

	qbuf->request = MSG_FAIL;
	data_type = (CONFIG_DATA_T)qbuf->msg.arg1;
	len = qbuf->msg.arg2;
	if (len > SHM_SIZE || shm_start == NULL)
		return;
	if (_mib_read_to_raw(data_type, shm_start, len)==1)
		qbuf->request = MSG_SUCC;
}

static void cfg_mib_read_header(struct mymsgbuf *qbuf)
{
	CONFIG_DATA_T data_type;

	data_type = (CONFIG_DATA_T)qbuf->msg.arg1;
	if(_mib_read_header(data_type, (PARAM_HEADER_Tp)qbuf->msg.mtext) != 1)
		qbuf->request = MSG_FAIL;
	else
		qbuf->request = MSG_SUCC;
}
#endif

static void cfg_mib_update_ex(struct mymsgbuf *qbuf)
{
#ifdef CONFIG_USER_CWMP_TR069
	int cwmp_msgid;
	struct cwmp_message cwmpmsg;
#endif

	CONFIG_DATA_T data_type;
	CONFIG_MIB_T flag;
	int commitnow;

	data_type = (CONFIG_DATA_T)qbuf->msg.arg1;
	flag = (CONFIG_MIB_T)qbuf->msg.arg2;
	commitnow = qbuf->msg.arg3;

#ifdef CONFIG_USER_XMLCONFIG
	if (xml_mib_update(data_type, flag))
		qbuf->request = MSG_SUCC;
#elif CONFIG_USER_CONF_ON_XMLFILE
	if (_mib_update(data_type) != 0)
		qbuf->request = MSG_SUCC;
#else
	if (data_type == CURRENT_SETTING) {
		if (flag == CONFIG_MIB_ALL) {
			if(_mib_update(data_type)!=0)
				qbuf->request = MSG_SUCC;
		}
		else if (flag == CONFIG_MIB_TABLE) {
			PARAM_HEADER_T header;
			unsigned int total_size, table_size;
			unsigned char *buf, *ptr;
			unsigned char *pMibTbl;

			if(__mib_header_read(data_type, &header) != 1){
				return;
			}
			total_size = sizeof(PARAM_HEADER_T) + header.len;
			buf = (unsigned char *)malloc(total_size);
			if (buf == NULL){
				return;
			}
			if(_mib_read_to_raw(data_type, buf, total_size) != 1) {
				free(buf);
				return;
			}
			ptr = buf + sizeof(PARAM_HEADER_T);
			// update the mib table only
			pMibTbl = __mib_get_mib_tbl(data_type);
			memcpy(ptr, pMibTbl, sizeof(MIB_T));
			__mib_content_encod_check(data_type, &header, ptr);
			// update header
			memcpy(buf, (unsigned char*)&header, sizeof(PARAM_HEADER_T));

			if(_mib_update_from_raw(buf, total_size) != 1) {
				free(buf);
				return;
			}
			free(buf);
			qbuf->request = MSG_SUCC;
		}
		else { // not support currently, Jenny added
				//jim we should check the size to make sure of no-exceeded flash range....
				//jim this will called by pppoe.c /pppoe_session_info();
			PARAM_HEADER_T header;
			unsigned int chainRecordSize, mibTblSize, totalSize;
			unsigned char *buf, *ptr;
			unsigned char* pVarLenTable = NULL;
		    	//printf("%s line %d\n", __FUNCTION__, __LINE__);
			if(__mib_header_read(data_type, &header) != 1){
				return;
			}
		    	//printf("%s line %d\n", __FUNCTION__, __LINE__);
			mibTblSize = __mib_content_min_size(data_type);
			chainRecordSize = __mib_chain_all_table_size(data_type);
			header.len = chainRecordSize + mibTblSize;
			totalSize = sizeof(PARAM_HEADER_T) + header.len;
			buf = (unsigned char *)malloc(totalSize);
		    	//printf("%s line %d Totalsize=%d\n", __FUNCTION__, __LINE__, totalSize);
			if (buf == NULL){
				return;
			}
			//jim
			if(totalSize > __mib_content_max_size(data_type))
			{
				printf("too large config paras to store! abadon!\n");
				free(buf);
				return;
			}
		    	//printf("%s line %d\n", __FUNCTION__, __LINE__);
			if(_mib_read_to_raw(data_type, buf, totalSize) != 1) {
				free(buf);
		    	//printf("%s line %d\n", __FUNCTION__, __LINE__);
				return;
			}
			ptr = &buf[sizeof(PARAM_HEADER_T)];	// point to start of MIB data
		    	//printf("%s line %d chainRecordSize=%d\n", __FUNCTION__, __LINE__, chainRecordSize);
			// update the chain record only
			if (chainRecordSize > 0) {
				pVarLenTable = &ptr[mibTblSize];	// point to start of variable length MIB data
				if(__mib_chain_record_content_encod(data_type, pVarLenTable, chainRecordSize) != 1) {
					free(buf);
		    	//printf("%s line %d\n", __FUNCTION__, __LINE__);
					return;
				}
			}
			__mib_content_encod_check(data_type, &header, ptr);
			// update header
			memcpy(buf, (unsigned char*)&header, sizeof(PARAM_HEADER_T));

		    	//printf("%s line %d\n", __FUNCTION__, __LINE__);
			if(_mib_update_from_raw(buf, totalSize) != 1) {
				free(buf);
		    	//printf("%s line %d\n", __FUNCTION__, __LINE__);
				return;
			}
			qbuf->request = MSG_SUCC;
		    	//printf("%s line %d\n", __FUNCTION__, __LINE__);
			free(buf);
		}
	}
	else {
		if(_mib_update(data_type)!=0)
			qbuf->request = MSG_SUCC;
	}
#endif

	if (qbuf->request == MSG_SUCC) {
#ifdef CONFIG_USER_CWMP_TR069
		if ((cwmp_msgid = msgget((key_t) 1234, 0)) > 0) {
			memset(&cwmpmsg, 0, sizeof(cwmpmsg));
			cwmpmsg.msg_type = MSG_ACTIVE_NOTIFY;
			cwmpmsg.msg_datatype = qbuf->msg.arg1;
			msgsnd(cwmp_msgid, &cwmpmsg, MSG_SIZE, IPC_NOWAIT);
		}
#endif
	}

}

static void cfg_mib_update(struct mymsgbuf *qbuf)
{
	CONFIG_DATA_T data_type;
	CONFIG_MIB_T flag;
	int commitnow;

	qbuf->request = MSG_FAIL;

	data_type = (CONFIG_DATA_T)qbuf->msg.arg1;
	flag = (CONFIG_MIB_T)qbuf->msg.arg2;
	commitnow = qbuf->msg.arg3;

	if(data_type == CURRENT_SETTING){
		if(commitnow == 1 || reboot_now == 1){
			cfg_mib_update_ex(qbuf);
		}
		else{
			qbuf->request = MSG_SUCC;
		}
	}
	else{
		cfg_mib_update_ex(qbuf);
	}
}

/* 2010-10-26 krammer :  */
static void cfg_mib_swap(struct mymsgbuf *qbuf)
{
	int id;

	qbuf->request = MSG_FAIL;

    if (__mib_lock) {
		qbuf->request = MSG_MIB_LOCKED;
		return;
	}

	if(_mib_swap(qbuf->msg.arg1, qbuf->msg.arg2)!=0)
		qbuf->request = MSG_SUCC;
}
/* 2010-10-26 krammer :  */

static void cfg_mib_to_default(struct mymsgbuf *qbuf)
{
	CONFIG_DATA_T data_type;
	qbuf->request = MSG_FAIL;
	data_type = (CONFIG_DATA_T)qbuf->msg.arg1;

	if (__mib_lock) {
		qbuf->request = MSG_MIB_LOCKED;
		return;
	}
	mib_init_mib_with_program_default(data_type, FLASH_DEFAULT_TO_MEMORY, CONFIG_MIB_ALL);
	qbuf->request = MSG_SUCC;
}

static void cfg_mib_flash_to_default(struct mymsgbuf *qbuf)
{
	CONFIG_DATA_T data_type;
	qbuf->request = MSG_FAIL;
	data_type = (CONFIG_DATA_T)qbuf->msg.arg1;

	if (__mib_lock) {
		qbuf->request = MSG_MIB_LOCKED;
		return;
	}

/* 2013/11 Jiachiam
   Only reset default to ram memory, not write back to flash.
   The latter task is performed by xmlconfig (write back to new xml file).
*/

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	if (data_type == CURRENT_SETTING)
		clean_filesystem();
#endif
#ifdef CONFIG_USER_XMLCONFIG
	mib_init_mib_with_program_default(data_type, FLASH_DEFAULT_TO_MEMORY, CONFIG_MIB_ALL);
#else
	mib_init_mib_with_program_default(data_type, FLASH_DEFAULT_TO_ALL, CONFIG_MIB_ALL);
#endif /* CONFIG_USER_XMLCONFIG */
#ifdef CONFIG_USER_CWMP_TR069
	unlink("/var/config/CWMPNotify.txt");
#endif//endof CONFIG_USER_CWMP_TR069
	qbuf->request = MSG_SUCC;
}

/** 2013/11 Jiachiam */
#if (defined VOIP_SUPPORT) && (defined CONFIG_USER_XMLCONFIG)
static void cfg_mib_voip_to_default(struct mymsgbuf *qbuf){

       voipCfgParam_t voipEntry;

       qbuf->request = MSG_FAIL;

       if (__mib_lock) {
               qbuf->request = MSG_MIB_LOCKED;
               return;
       }

       _mib_chain_clear(MIB_VOIP_CFG_TBL);
       flash_voip_default(&voipEntry);
       if (_mib_chain_add(MIB_VOIP_CFG_TBL, &voipEntry))
               qbuf->request = MSG_SUCC;

}
#endif /* VOIP_SUPPORT & CONFIG_USER_XMLCONFIG */

static void cfg_mib_get(struct mymsgbuf *qbuf)
{
	int id;

	qbuf->request = MSG_FAIL;

	if(_mib_get(qbuf->msg.arg1, (void *)qbuf->msg.mtext)!=0)
		qbuf->request = MSG_SUCC;
}

static void cfg_backup_get(struct mymsgbuf *qbuf)
{
	int id;

	qbuf->request = MSG_FAIL;

	if(_mib_backup_get(qbuf->msg.arg1, (void *)qbuf->msg.mtext, (void*)&table_backup)!=0)
		qbuf->request = MSG_SUCC;
}


static void cfg_mib_get_default(struct mymsgbuf *qbuf)
{
	int id;

	qbuf->request = MSG_FAIL;

	if(_mib_getDef(qbuf->msg.arg1, (void *)qbuf->msg.mtext)!=0)
		qbuf->request = MSG_SUCC;
}

#ifdef CONFIG_USER_CTCAPD
void send_ubus_event_by_mib_change(int id, char *orig_value, char *mtext)
{
	char buf[100] = {0};
	if(orig_value == NULL || mtext == NULL)
		return ;
	if(orig_value[0] != mtext[0])
	{
		if(id == MIB_WIFI_MODULE_DISABLED)
		{
			if(mtext[0] == 1)
				system("ubus send \"propertieschanged\" \'{\"wifiswitch\":\"off\"}\'");
			else if(mtext[0] == 0)
				system("ubus send \"propertieschanged\" \'{\"wifiswitch\":\"on\"}\'");
		}

		if(id == MIB_CTCAPD_ELINK_SYNC)
		{
			if(mtext[0] == 1)
				system("ubus send \"propertieschanged\" \'{\"elinksync\":\"on\"}\'");
			else if(mtext[0] == 0)
				system("ubus send \"propertieschanged\" \'{\"elinksync\":\"off\"}\'");
		}

		if(id == MIB_WLAN_CHAN_NUM || id == MIB_WLAN1_CHAN_NUM)
		{
#ifdef WLAN0_5G_SUPPORT
			if(id == MIB_WLAN_CHAN_NUM)
			{
				if(mtext[0] >= 0)
					snprintf(buf,sizeof(buf),"ubus send \"channelchanged_2nd\" \'{\"channel\": %d}\'", mtext[0]);
				else
					snprintf(buf,sizeof(buf),"ubus send \"channelchanged_2nd\" \'{\"channel\": %d}\'", mtext[0]+256);
			}
			else if(id == MIB_WLAN1_CHAN_NUM)
					snprintf(buf,sizeof(buf),"ubus send \"channelchanged\" \'{\"channel\": %d}\'", mtext[0]);
#else
			if(id == MIB_WLAN1_CHAN_NUM)
			{
				if(mtext[0] >= 0)
					snprintf(buf,sizeof(buf),"ubus send \"channelchanged_2nd\" \'{\"channel\": %d}\'", mtext[0]);
				else
					snprintf(buf,sizeof(buf),"ubus send \"channelchanged_2nd\" \'{\"channel\": %d}\'", mtext[0]+256);
			}
			else if(id == MIB_WLAN_CHAN_NUM)
				snprintf(buf,sizeof(buf),"ubus send \"channelchanged\" \'{\"channel\": %d}\'", mtext[0]);
#endif
			system(buf);
		}

		if(id == MIB_LED_STATUS)
		{
			if(mtext[0] == 1)
				system("ubus send \"led_changed\" \'{\"status\":\"on\"}\'");
			else if(mtext[0] == 0)
				system("ubus send \"led_changed\" \'{\"status\":\"off\"}\'");
		}

	}


}
#endif
static void cfg_mib_set(struct mymsgbuf *qbuf)
{
#ifdef CONFIG_USER_CWMP_TR069
	int cwmp_msgid;
	struct cwmp_message cwmpmsg;
#endif

	qbuf->request = MSG_FAIL;

	if (__mib_lock) {
		qbuf->request = MSG_MIB_LOCKED;
		return;
	}
#ifdef CONFIG_USER_CTCAPD
	char orig_value[MAX_SEND_SIZE];
	if(qbuf->msg.arg1 == MIB_WIFI_MODULE_DISABLED || qbuf->msg.arg1 == MIB_CTCAPD_ELINK_SYNC
		|| qbuf->msg.arg1 ==  MIB_WLAN_CHAN_NUM || qbuf->msg.arg1 == MIB_WLAN1_CHAN_NUM
		|| qbuf->msg.arg1 == MIB_LED_STATUS)
	{
		_mib_get(qbuf->msg.arg1, (void *)orig_value);
	}

#endif
	if (_mib_set(qbuf->msg.arg1, (void *)qbuf->msg.mtext) != 0) {
#ifdef CONFIG_USER_CTCAPD
		if(qbuf->msg.arg1 == MIB_WIFI_MODULE_DISABLED || qbuf->msg.arg1 == MIB_CTCAPD_ELINK_SYNC
			|| qbuf->msg.arg1 ==  MIB_WLAN_CHAN_NUM || qbuf->msg.arg1 == MIB_WLAN1_CHAN_NUM
			|| qbuf->msg.arg1 == MIB_LED_STATUS)
			send_ubus_event_by_mib_change(qbuf->msg.arg1, orig_value, qbuf->msg.mtext);
#endif

		qbuf->request = MSG_SUCC;
#ifdef CONFIG_USER_CWMP_TR069
		if (qbuf->msg.arg2) {
			if ((cwmp_msgid = msgget((key_t) 1234, 0)) > 0) {
				memset(&cwmpmsg, 0, sizeof(cwmpmsg));
				cwmpmsg.msg_type = MSG_USERDATA_CHANGE;
				cwmpmsg.msg_datatype = qbuf->msg.arg1;
				msgsnd(cwmp_msgid, &cwmpmsg, MSG_SIZE, IPC_NOWAIT);
			}
		}
#endif
	}
}

static void cfg_mib_info_id(struct mymsgbuf *qbuf)
{
	int k;

	qbuf->request = MSG_FAIL;

	for (k=0; mib_table[k].id; k++) {
		if (mib_table[k].id == qbuf->msg.arg1)
			break;
	}

	if (mib_table[k].id == 0)
		return;

	memcpy((void *)qbuf->msg.mtext, (void *)&mib_table[k], sizeof(mib_table_entry_T));
	qbuf->request = MSG_SUCC;
}

static void cfg_mib_info_index(struct mymsgbuf *qbuf)
{
	int total;

	qbuf->request = MSG_FAIL;

	total = mib_table_size / sizeof(mib_table[0]);
	if (qbuf->msg.arg1 >= total)
		return;

	memcpy((void *)qbuf->msg.mtext, (void *)&mib_table[qbuf->msg.arg1],
	       sizeof(mib_table_entry_T));
	qbuf->request = MSG_SUCC;
}

// Apply Star Zhang's fast load
static void cfg_mib_info_total(struct mymsgbuf *qbuf)
{
	qbuf->request = MSG_FAIL;

	qbuf->msg.arg1 = mib_table_size / sizeof(mib_table[0]);

	qbuf->request = MSG_SUCC;
}
// The end of fast load
static void cfg_mib_backup(struct mymsgbuf *qbuf)
{
	CONFIG_MIB_T type;
	unsigned char *pMibTbl;
	unsigned char *__chain_backup = NULL;
	unsigned int temp_backupChainSize = 0;

	qbuf->request = MSG_FAIL;
	type = (CONFIG_MIB_T)qbuf->msg.arg1;

	if (type == CONFIG_MIB_ALL || type == CONFIG_MIB_TABLE) {
		pMibTbl = __mib_get_mib_tbl(CURRENT_SETTING);
		memcpy(&table_backup, pMibTbl, sizeof(MIB_T));  //save setting
	}

	if (type == CONFIG_MIB_ALL || type == CONFIG_MIB_CHAIN)
	{
		//backupChainSize = __mib_chain_all_table_size(CURRENT_SETTING);
		temp_backupChainSize = __mib_chain_all_table_size(CURRENT_SETTING);

		//if(backupChainSize>0)
		if(temp_backupChainSize != backupChainSize)
		{
			if(chain_backup)
			{
				free(chain_backup);
				__chain_backup = (unsigned char *)calloc(1, temp_backupChainSize);
			}
			else
				__chain_backup = (unsigned char *)calloc(1, temp_backupChainSize);

			if(__chain_backup)
			{
				printf("===> Success to re-allocate memory size(%d, %p) for chain backup !!  original backup size(%d) ptr(%p)\n", temp_backupChainSize, __chain_backup, backupChainSize, chain_backup);
				chain_backup = __chain_backup;
				backupChainSize = temp_backupChainSize;
			}
			else
			{
				printf("===> Fail to re-allocate memory size(%d) for chaing backup !!  original backup size(%d) ptr(%p)\n", temp_backupChainSize, backupChainSize, chain_backup);
				return;
			}

		}
		if(chain_backup)
		{
			if(__mib_chain_record_content_encod(CURRENT_SETTING, chain_backup, backupChainSize) != 1)
				return;
		}

	}
	else
		return;

	qbuf->request = MSG_SUCC;
}

//added by ql
#ifdef	RESERVE_KEY_SETTING
static void cfg_retrieve_table(struct mymsgbuf *qbuf)
{
	int id;

	qbuf->request = MSG_FAIL;
	id = qbuf->msg.arg1;

	mib_table_record_retrive(id);

	qbuf->request = MSG_SUCC;
}
static void cfg_retrieve_chain(struct mymsgbuf *qbuf)
{
	int id;

	qbuf->request = MSG_FAIL;
	id = qbuf->msg.arg1;

	mib_chain_record_retrive(id);

	qbuf->request = MSG_SUCC;
}
#endif

static void cfg_mib_restore(struct mymsgbuf *qbuf)
{
	CONFIG_MIB_T type;
	unsigned char *pMibTbl;

	qbuf->request = MSG_FAIL;
	type = (CONFIG_MIB_T)qbuf->msg.arg1;

	if (type == CONFIG_MIB_ALL || type == CONFIG_MIB_TABLE)
	{
		pMibTbl = __mib_get_mib_tbl(CURRENT_SETTING);
		memcpy(pMibTbl, &table_backup, sizeof(MIB_T));  //restore setting
	}

	if (type == CONFIG_MIB_ALL || type == CONFIG_MIB_CHAIN)
	{
		__mib_chain_all_table_clear(CURRENT_SETTING);
		if(backupChainSize > 0)
		{
			// parse variable length MIB data
			if( __mib_chain_record_content_decod(chain_backup, backupChainSize) != 1)
				return;
		}
	}
	else
		return;

	qbuf->request = MSG_SUCC;
}

static void cfg_chain_total(struct mymsgbuf *qbuf)
{
	qbuf->request = MSG_FAIL;

	qbuf->msg.arg1 = _mib_chain_total(qbuf->msg.arg1);
	qbuf->request = MSG_SUCC;
}

static void cfg_chain_get(struct mymsgbuf *qbuf)
{
	int index, entryNo;
	void *pEntry;

	qbuf->request = MSG_FAIL;
	index = __mib_chain_mib2tbl_id(qbuf->msg.arg1);
	if (index == -1)
		return;

	entryNo = atoi(qbuf->msg.mtext);
	pEntry = _mib_chain_get(qbuf->msg.arg1, entryNo);
	if (pEntry) {
		#ifdef USE_SHM
		memcpy(shm_start, pEntry, mib_chain_record_table[index].per_record_size);
		#else
		memcpy(qbuf->msg.mtext, pEntry, mib_chain_record_table[index].per_record_size);
		#endif
		qbuf->request = MSG_SUCC;
	}
	else
		qbuf->request = MSG_FAIL;
}

static void cfg_chain_backup_get(struct mymsgbuf *qbuf)
{
	int index, entryNo;
	void *pEntry;

	qbuf->request = MSG_FAIL;
	index = __mib_chain_mib2tbl_id(qbuf->msg.arg1);
	if (index == -1 || chain_backup == NULL)
	{
		qbuf->request = MSG_FAIL;
		return;
	}

	entryNo = atoi(qbuf->msg.mtext);
	pEntry = __mib_chain_get_backup_record(qbuf->msg.arg1, entryNo, chain_backup, backupChainSize);
	if (pEntry) {
		#ifdef USE_SHM
		memcpy(shm_start, pEntry, mib_chain_record_table[index].per_record_size);
		#else
		memcpy(qbuf->msg.mtext, pEntry, mib_chain_record_table[index].per_record_size);
		#endif
		qbuf->request = MSG_SUCC;
	}
	else
		qbuf->request = MSG_FAIL;
}

static void cfg_chain_get_def(struct mymsgbuf *qbuf)
{
	int index, entryNo;
	const void *pEntry;

	qbuf->request = MSG_FAIL;
	index = __mib_chain_mib2tbl_id(qbuf->msg.arg1);
	if (index == -1)
		return;

	entryNo = atoi(qbuf->msg.mtext);
	pEntry = _mib_chain_getDef(qbuf->msg.arg1, entryNo);
	if (pEntry) {
		#ifdef USE_SHM
		memcpy(shm_start, pEntry, mib_chain_record_table[index].per_record_size);
		#else
		memcpy(qbuf->msg.mtext, pEntry, mib_chain_record_table[index].per_record_size);
		#endif
		qbuf->request = MSG_SUCC;
	}
	else
		qbuf->request = MSG_FAIL;
}

static void cfg_chain_add(struct mymsgbuf *qbuf)
{
	qbuf->request = MSG_FAIL;

	if (__mib_lock) {
		qbuf->request = MSG_MIB_LOCKED;
		return;
	}

	#ifdef USE_SHM
	if (_mib_chain_add(qbuf->msg.arg1, shm_start)) {
	#else
	if (_mib_chain_add(qbuf->msg.arg1, qbuf->msg.mtext)) {
	#endif
		qbuf->request = MSG_SUCC;
	}
	else
		qbuf->request = MSG_FAIL;
}

static void cfg_chain_delete(struct mymsgbuf *qbuf)
{
	int entryNo;

	qbuf->request = MSG_FAIL;
	entryNo = atoi(qbuf->msg.mtext);

	if (__mib_lock) {
		qbuf->request = MSG_MIB_LOCKED;
		return;
	}

	if (_mib_chain_delete(qbuf->msg.arg1, entryNo)) {
		qbuf->request = MSG_SUCC;
	}
	else
		qbuf->request = MSG_FAIL;
}

static void cfg_chain_clear(struct mymsgbuf *qbuf)
{
	qbuf->request = MSG_FAIL;

	if (__mib_lock) {
		qbuf->request = MSG_MIB_LOCKED;
		return;
	}

	_mib_chain_clear(qbuf->msg.arg1);
	qbuf->request = MSG_SUCC;
}

#ifdef CONFIG_USER_CTCAPD
void send_ubus_event_by_mib_chain_update(int id, unsigned int recordNum)
{
	void *pEntry;
	MIB_CE_MBSSIB_T wlan_entry_orig, wlan_entry_modify, wlan_entry;
	void *pentry;
	unsigned char buf[100] = {0};

	pEntry = _mib_chain_get(id, recordNum);

	if (pEntry)
	{
		memset(&wlan_entry_orig,0,sizeof(MIB_CE_MBSSIB_T));
		memset(&wlan_entry_modify,0,sizeof(MIB_CE_MBSSIB_T));
		memset(&wlan_entry,0,sizeof(MIB_CE_MBSSIB_T));

		memcpy(&wlan_entry_orig, pEntry, sizeof(MIB_CE_MBSSIB_T));
		memcpy(&wlan_entry_modify, shm_start, sizeof(MIB_CE_MBSSIB_T));

		if(strncmp(wlan_entry_orig.ssid, wlan_entry_modify.ssid, strlen(wlan_entry_modify.ssid)) != 0)
		{

			if(wlan_entry_modify.wlanDisabled == 0)
			{
				if(id == MIB_MBSSIB_TBL)
				{
#ifdef WLAN0_5G_SUPPORT
					snprintf(buf, sizeof(buf), "ubus send \"ssidchanged_2nd\" \'{\"ssid\":\"%s\"}\'", wlan_entry_modify.ssid);
#else
					snprintf(buf, sizeof(buf), "ubus send \"ssidchanged\" \'{\"ssid\":\"%s\"}\'", wlan_entry_modify.ssid);
#endif
					system(buf);
				}
				else if(id == MIB_WLAN1_MBSSIB_TBL)
				{
#ifdef WLAN0_5G_SUPPORT
					snprintf(buf, sizeof(buf), "ubus send \"ssidchanged\" \'{\"ssid\":\"%s\"}\'", wlan_entry_modify.ssid);
#else
					snprintf(buf, sizeof(buf), "ubus send \"ssidchanged_2nd\" \'{\"ssid\":\"%s\"}\'", wlan_entry_modify.ssid);
#endif
					system(buf);
				}

			}
		}

		if((wlan_entry_orig.wlanDisabled != wlan_entry_modify.wlanDisabled
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
			|| wlan_entry_orig.wsc_disabled != wlan_entry_modify.wsc_disabled
#endif
            )
			&& recordNum == 0)
		{
#ifdef WLAN_DUALBAND_CONCURRENT
			if(id == MIB_MBSSIB_TBL)
				pentry = _mib_chain_get(MIB_WLAN1_MBSSIB_TBL, recordNum);
			else if(id == MIB_WLAN1_MBSSIB_TBL)
				pentry = _mib_chain_get(MIB_MBSSIB_TBL, recordNum);

			if(pentry)
			{
				memcpy(&wlan_entry, pentry, sizeof(MIB_CE_MBSSIB_T));

				/*
				if(wlan_entry_orig.wlanDisabled != wlan_entry_modify.wlanDisabled)
				{
					if(wlan_entry_modify.wlanDisabled == 1 && wlan_entry.wlanDisabled == 1)
						system("ubus send \"propertieschanged\" \'{\"wifiswitch\":\"off\"}\'");
					else if(wlan_entry_modify.wlanDisabled == 0)
						system("ubus send \"propertieschanged\" \'{\"wifiswitch\":\"on\"}\'");
				}

				if(wlan_entry_orig.wsc_disabled != wlan_entry_modify.wsc_disabled)
				{
					if(wlan_entry_modify.wsc_disabled == 1 && wlan_entry.wsc_disabled == 1)
						system("ubus send \"propertieschanged\" \'{\"wpsswitch\":\"off\"}\'");
					else if(wlan_entry_modify.wsc_disabled == 0)
						system("ubus send \"propertieschanged\" \'{\"wpsswitch\":\"on\"}\'");
				}
				*/
			}
#endif
		}
	}

}
#endif
static void cfg_chain_update(struct mymsgbuf *qbuf)
{
	int index;
	void *pEntry;

	qbuf->request = MSG_FAIL;

	if (__mib_lock) {
		qbuf->request = MSG_MIB_LOCKED;
		return;
	}

	index = __mib_chain_mib2tbl_id(qbuf->msg.arg1);
	if (index == -1)
		return;
	pEntry = _mib_chain_get(qbuf->msg.arg1, qbuf->msg.arg2);
	if (pEntry)
	{
		#ifdef USE_SHM
		#ifdef CONFIG_USER_CTCAPD
		send_ubus_event_by_mib_chain_update(qbuf->msg.arg1, qbuf->msg.arg2);
		#endif
		memcpy(pEntry, shm_start, mib_chain_record_table[index].per_record_size);
		#else
		memcpy(pEntry, qbuf->msg.mtext, mib_chain_record_table[index].per_record_size);
		#endif
	}
	else
		return;
	#ifdef USE_SHM
	if (_mib_chain_update(qbuf->msg.arg1, shm_start, qbuf->msg.arg2)) {
	#else
	if (_mib_chain_update(qbuf->msg.arg1, qbuf->msg.mtext, qbuf->msg.arg2)) {
	#endif
		qbuf->request = MSG_SUCC;
	}
	else
		qbuf->request = MSG_FAIL;
}

/* cathy, to swap entry A and B of a chain */
static void cfg_chain_swap(struct mymsgbuf *qbuf)
{
	int index, id;
	void *pEntryA, *pEntryB, *tmpEntry;

	qbuf->request = MSG_FAIL;

	if (__mib_lock) {
		qbuf->request = MSG_MIB_LOCKED;
		return;
	}

	id = atoi(qbuf->msg.mtext);
	index = __mib_chain_mib2tbl_id(id);
	if (index == -1)
		return;

	pEntryA = _mib_chain_get(id, qbuf->msg.arg1);

#if defined(WLAN_DUALBAND_CONCURRENT)
	if (id > DUAL_WLAN_CHAIN_START_ID && id < DUAL_WLAN_CHAIN_END_ID) {
		if (isValid_wlan_idx(wlan_idx))
			id += 1;
	}
#endif

	pEntryB = _mib_chain_get(id, qbuf->msg.arg2);

	if(!pEntryA || !pEntryB) {
		printf("%s: cannot find entry!\n", __func__);
		return;
	}

	tmpEntry = malloc(mib_chain_record_table[index].per_record_size);
	if(!tmpEntry) {
		printf("%s: cannot allocate memory!\n", __func__);
		return;
	}

	//swap pEntryA and pEntryB
	memcpy(tmpEntry, pEntryA, mib_chain_record_table[index].per_record_size);
	memcpy(pEntryA, pEntryB, mib_chain_record_table[index].per_record_size);
	memcpy(pEntryB, tmpEntry, mib_chain_record_table[index].per_record_size);

	free(tmpEntry);
	qbuf->request = MSG_SUCC;
}

static void cfg_chain_info_id(struct mymsgbuf *qbuf)
{
	int index;

	qbuf->request = MSG_FAIL;

	index = __mib_chain_mib2tbl_id(qbuf->msg.arg1);
	if (index == -1)
		return;

	memcpy((void *)qbuf->msg.mtext, (void *)&mib_chain_record_table[index], sizeof(mib_chain_record_table_entry_T));
	qbuf->request = MSG_SUCC;
}

static void cfg_chain_info_index(struct mymsgbuf *qbuf)
{
	int total;
	int i;

	qbuf->request = MSG_FAIL;

	for (i=0; mib_chain_record_table[i].id; i++);
	total = i+1;
	if (qbuf->msg.arg1>=total)
		return;

	memcpy((void *)qbuf->msg.mtext, (void *)&mib_chain_record_table[qbuf->msg.arg1], sizeof(mib_chain_record_table_entry_T));
	qbuf->request = MSG_SUCC;
}

static void cfg_chain_info_name(struct mymsgbuf *qbuf)
{
	int total;
	int i;

	qbuf->request = MSG_FAIL;

	for (i=0; mib_chain_record_table[i].id; i++) {
		if (!strcmp(mib_chain_record_table[i].name, qbuf->msg.mtext))
			break;
	}

	if (mib_chain_record_table[i].id == 0)
		return; // not found

	memcpy((void *)qbuf->msg.mtext, (void *)&mib_chain_record_table[i], sizeof(mib_chain_record_table_entry_T));
	qbuf->request = MSG_SUCC;
}

static void cfg_chain_record_desc(struct mymsgbuf *qbuf)
{
	int index;
	mib_chain_member_entry_T *record_desc = NULL;
	int item_idx=0, item_nums=0, record_desc_size;

	qbuf->request = MSG_FAIL;
	index = __mib_chain_mib2tbl_id(qbuf->msg.arg1);
	if (index == -1)
		return;

	record_desc = mib_chain_record_table[index].record_desc;
	while(record_desc[item_idx].name[0]){
		item_idx++;
		item_nums++;
	}
	record_desc_size = sizeof(mib_chain_member_entry_T)*item_nums;

	qbuf->msg.arg1 = record_desc_size;

/*
	printf("[%s:%d] sizeof(mib_chain_member_entry_T)=%d, record_desc_size=%d\n", __FUNCTION__,__LINE__,sizeof(mib_chain_member_entry_T),
		record_desc_size);
*/

	#ifdef USE_SHM
	memcpy(shm_start, record_desc, record_desc_size);
	#else
	memcpy(qbuf->msg.mtext, record_desc, record_desc_size);
	#endif
	qbuf->request = MSG_SUCC;
}

static int verify_desc(mib_chain_member_entry_T *rec_desc, char *name, int rec_size, int depth)
{
	int k, count_size, verdict;
	int ret;

	verdict = 1;

	if (!rec_desc) {
		printf("Error: Null MIB-chain(%s) record descriptor !\n", name);
		return -1;
	}

	k = 0; count_size = 0;
	while (rec_desc[k].name[0] != 0) {
		if (rec_desc[k].type == OBJECT_T) {
			ret = verify_desc(rec_desc[k].record_desc, rec_desc[k].name, rec_desc[k].size, depth+1);
			if (ret==-1)
				verdict = -1;
		}
		count_size += rec_desc[k].size;
		k++;
	}
	if (depth == 1) { // root chain rec_size is per-record size
		if (count_size != rec_size) {
			printf("Error: MIB chain %s descriptor not consistent with data structure !\n", name);
			verdict = -1;
		}
	}
	else { // child chain(object) rec_size is total object size(multiplier of count_size)
		if (count_size != 0 && rec_size%count_size) {
			printf("Error: MIB object %s descriptor not consistent with data structure !\n", name);
			verdict = -1;
		}
	}
	return verdict;
}

/*
 * Check the consistency between chain records and their record descriptors.
 * Return msg.arg1:
 *	1: checking ok
 *	-1: checking failed
 */
static void cfg_check_desc(struct mymsgbuf *qbuf)
{
	mib_chain_member_entry_T *rec_desc;
	int i, k, count_size, verdict, ret=1;

	qbuf->request = MSG_FAIL;
	verdict = 1;

	for (i=0; mib_chain_record_table[i].id; i++) {
		rec_desc = mib_chain_record_table[i].record_desc;
		if (!rec_desc) {
			printf("Error: Null MIB-chain(%s) record descriptor !\n", mib_chain_record_table[i].name);
			verdict = -1;
			continue;
		}
		ret=verify_desc(rec_desc, mib_chain_record_table[i].name, mib_chain_record_table[i].per_record_size, 1);
		if(ret==-1)
			verdict = -1;
	}

	printf("MIB chain descriptors checking (total %d) %s !\n", i, verdict==1?"ok":"failed");
	qbuf->msg.arg1 = verdict;
	qbuf->request = MSG_SUCC;
}

/*
 *	reload hs	---	reload hardware setting
 *	reload cs	---	reload current setting
 *	reload ds	---	reload default setting
 */
static void cfg_mib_reload(struct mymsgbuf *qbuf)
{
	CONFIG_DATA_T data_type;
	CONFIG_MIB_T flag;
	qbuf->request = MSG_FAIL;

	if (__mib_lock) {
		qbuf->request = MSG_MIB_LOCKED;
		return;
	}

#ifndef CONFIG_USER_XMLCONFIG
	data_type = (CONFIG_DATA_T)qbuf->msg.arg1;
	flag = (CONFIG_MIB_T)qbuf->msg.arg2;

	if (data_type == CURRENT_SETTING) {
		if (flag == CONFIG_MIB_ALL) {
			if(_mib_load(data_type)!=0)
				qbuf->request = MSG_SUCC;
		}
		else if (flag == CONFIG_MIB_TABLE) {
			if (mib_load_table(data_type)!=0)
				qbuf->request = MSG_SUCC;
		}
		else { // not support currently, Jenny added
			if (mib_load_chain(data_type)!=0)
				qbuf->request = MSG_SUCC;
		}
	}
	else {
		if(_mib_load(data_type)!=0)
			qbuf->request = MSG_SUCC;
	}
#else
	qbuf->request = MSG_SUCC;
#endif
}



#ifdef EMBED

#if (defined(CONFIG_USB_ARCH_HAS_XHCI) && defined(CONFIG_USB_XHCI_HCD))
void unbind_xhci_driver()
{

	/* This function is for USB XHCI driver unload, because some USB 3.0
	 * device need this step, then after  reboot could set configuration
	 * successfully
	 */
	if (0 == access("/sys/bus/usb/devices/1-2/driver/", F_OK)) {
		if(0 == access("/sys/bus/usb/devices/1-2/driver/1-2.1",F_OK))
		{
			printf("echo 1-2.1 >  /sys/bus/usb/devices/1-2/driver/unbind\n");
			system("echo 1-2.1 > /sys/bus/usb/devices/1-2/driver/unbind");
		}
		if(0 == access("/sys/bus/usb/devices/1-2/driver/1-2.2",F_OK))
		{
			printf("echo 1-2.2 >  /sys/bus/usb/devices/1-2/driver/unbind\n");
			system("echo 1-2.2 > /sys/bus/usb/devices/1-2/driver/unbind");
		}
		if(0 == access("/sys/bus/usb/devices/1-2/driver/1-2.3",F_OK))
		{
			printf("echo 1-2.3 >  /sys/bus/usb/devices/1-2/driver/unbind\n");
			system("echo 1-2.3 > /sys/bus/usb/devices/1-2/driver/unbind");
		}
		if(0 == access("/sys/bus/usb/devices/1-2/driver/1-2.4",F_OK))
		{
			printf("echo 1-2.4 >  /sys/bus/usb/devices/1-2/driver/unbind\n");
			system("echo 1-2.4 > /sys/bus/usb/devices/1-2/driver/unbind");
		}
		if(0==access("/sys/bus/usb/devices/1-2/driver/1-2", F_OK))
		{
			printf("echo 1-2 >  /sys/bus/usb/devices/1-2/driver/unbind\n");
			system("echo 1-2 > /sys/bus/usb/devices/1-2/driver/unbind");
		}
	}
}
#endif

int doReleaseWanConneciton()
{
	unsigned int entryNum=0, i=0;
	char ifname[IFNAMSIZ]={0};
	char sVal[32]={0};
	MIB_CE_ATM_VC_Tp pEntry;

	entryNum = _mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if ((pEntry=(MIB_CE_ATM_VC_Tp)_mib_chain_get(MIB_ATM_VC_TBL, i))==NULL){
  			printf("Get chain record error!\n");
			return -1;
		}

		if (pEntry->enable == 0)
			continue;

		// check current operation udhcpc(nas0_0 or br0)
		if (access("/var/run/udhcpc.pid.nas0_0", F_OK) == 0)
			snprintf(ifname, IFNAMSIZ, "%s", "nas0_0");
		else if (access("/var/run/udhcpc.pid.br0", F_OK) == 0)
			snprintf(ifname, IFNAMSIZ, "%s", "br0");

		if(pEntry->ipDhcp == DHCP_CLIENT ){ //DHCPC
			int dhcpc_pid;
			printf("Release DHCPClient ifname=%s\n",ifname);
			// DHCP Client
			snprintf(sVal, 32, "%s.%s", (char*)DHCPC_PID, ifname);
			dhcpc_pid = read_pid((char*)sVal);
			if (dhcpc_pid > 0) {
				kill(dhcpc_pid, SIGUSR2); // force release
			}
			if (pEntry->IpProtocol & IPVER_IPV6)
				del_dhcpc6(pEntry);
		}
		else if(pEntry->cmode == CHANNEL_MODE_PPPOE){
			char *p_index= &ifname[3];
			unsigned int index= atoi(p_index);

			printf("Release PPPoE client, ifname=%s, pppoe index=%d\n",ifname,index);
#ifdef CONFIG_USER_PPPD
			disconnectPPP(index);
#endif //CONFIG_USER_PPPD
#ifdef CONFIG_USER_SPPPD
			sprintf(sVal,"/bin/spppctl force_down %d\n", index);
			system(sVal);
#endif //CONFIG_USER_SPPPD
		}
	}
	sleep(1);
	return 0;
}

/* 20221229, mkkwon, H/W reset */
static void gpio_reboot(void)
{
	int32 ret;
	uint32 value,gpioId;

	gpioId = 60;
	value = 1; /* state:enable, mode:output*/

	if(rtk_gpio_state_set(gpioId, value) != RT_ERR_OK)
		printf("failed to reboot!!!");

	if(rtk_gpio_mode_set(gpioId,value) != RT_ERR_OK)
		printf("failed to reboot!!!");

	if(rtk_gpio_databit_set(gpioId,value) != RT_ERR_OK)
		printf("failed to reboot!!!");
}

static unsigned char start_reboot = 0;
static void *sysReboot(void *arg)
{
	if(start_reboot == 1)
		return NULL;
	start_reboot = 1;

#ifdef CONFIG_PPP
	stopPPP();
#endif
	doReleaseWanConneciton(); //PPPoE PADT, DHCP Release

#if defined(CONFIG_AIRTEL_EMBEDUR)
	if ( is_fwupgrade )
		write_omd_reboot_log(INSTALL_FLAG);
	else
		write_omd_reboot_log(ADMIN_FLAG);

	fprintf(stderr,"[%s%d] \n",__FUNCTION__, __LINE__);
#ifdef CONFIG_CLASSFY_MAGICNUM_SUPPORT
		system("diag switch set magic-key 0xFFFFFFFF &> /dev/null");
#elif defined(CONFIG_LUNA_WDT_KTHREAD)
		system("echo clear > /proc/luna_watchdog/previous_wdt_reset");
#endif
#endif

#ifdef _PRMT_X_CT_COM_ALARM_MONITOR_
	set_ctcom_alarm(CTCOM_ALARM_DEV_RESTART);
	syslog(LOG_ALERT, "104001 System reboot.");
#endif

#if defined(BB_FEATURE_SAVE_LOG) || defined(CONFIG_USER_RTK_SYSLOG)
	/* before we reboot, use sync() to make os to write file currently*/
	va_cmd("/etc/scripts/slogd_sync.sh", 2, 1, (char *)SYSLOGDFILE, (char *)SYSLOGDFILE_SAVE);
#endif
#ifdef CONFIG_USER_LANNETINFO_COLLECT_OFFLINE_DEVINFO
	va_cmd("/bin/cp", 2, 1, (char *)OFFLINELANNETINFOFILE, (char *)OFFLINELANNETINFOFILE_SAVE);
#endif
#ifdef CONFIG_USER_RTK_OMD
	if(is_terminal_reboot() )
		write_omd_reboot_log(TERMINAL_REBOOT);
#ifdef CONFIG_CLASSFY_MAGICNUM_SUPPORT
	system("diag switch set magic-key 0xFFFFFFFF &> /dev/null");
#elif defined(CONFIG_LUNA_WDT_KTHREAD)
	system("echo clear > /proc/luna_watchdog/previous_wdt_reset");
#endif
#endif

#if (defined(CONFIG_USB_ARCH_HAS_XHCI) && defined(CONFIG_USB_XHCI_HCD))
	unbind_xhci_driver();
#endif

#if defined(CONFIG_RTL867X_NETLOG) && defined(CONFIG_USER_NETLOGGER_SUPPORT)
	va_niced_cmd("/bin/netlogger", 1, 1, "disable");
#endif

#ifdef CONFIG_DEV_xDSL
	va_cmd("/bin/adslctrl", 1, 1, "disablemodemline");
#endif
#ifdef CONFIG_VIRTUAL_WLAN_DRIVER
	system("echo 1 > /proc/vwlan");
#endif
	system("mib commit cs now");
	sync();
#ifdef CONFIG_DEV_xDSL
	itfcfg("sar", 0);
#endif
	itfcfg("eth0", 0);
	itfcfg("wlan0", 0);

	system("echo 1 > /tmp/reboot_done");
	if(reboot_now) {
		printf("===> system reboot now\n");
		/* reboot the system */
#if 0	/* 20221229, mkkwon, H/W reset */
		reboot(RB_AUTOBOOT);
#else
		sleep(2);
		va_cmd("/sbin/touch", 1, 1, "/config/.warmstart");
		va_cmd("/sbin/rm", 1, 1, "/config/defkey");
		va_cmd("/sbin/sync", 0, 1);
		gpio_reboot();
		sleep(5);
		printf("===> Attempting a software reset since the hardware reset is not functioning \n");
		sleep(1);
		reboot(RB_AUTOBOOT);
#endif

		while(1) sleep(1);
	}

	return NULL;
}

static pthread_t ptRebootId=0;
static void cfg_reboot(struct mymsgbuf *qbuf)
{
	if( ptRebootId ==0 || pthread_kill(ptRebootId, 0) == 0) {
		reboot_now = (qbuf->msg.arg1==1)?1:0;
		pthread_create(&ptRebootId, NULL, sysReboot, NULL);
		pthread_detach(ptRebootId);
		qbuf->request = MSG_SUCC;
	}
}

#ifdef CONFIG_IPV6
#if defined(DHCPV6_ISC_DHCP_4XX)

/*
 * _get_prefixv6_info(): Get Prefixv6 information from WEB default IPv6 LAN setting.
 *   depends on MIB_PREFIXINFO_PREFIX_MODE.
 */
int _get_prefixv6_info(PREFIX_V6_INFO_Tp prefixInfo)
{
	unsigned char ipv6PrefixMode = 0, prefixLen = 0;
	unsigned char tmpBuf[100] = {0};
	unsigned char leasefile[64] = {0};
	unsigned int wanconn = 0;
	DLG_INFO_T dlgInfo = {0};

	if(!prefixInfo){
		printf("Error! NULL input prefixV6Info\n");
		goto setErr_ipv6;
	}
	if ( !_mib_get(MIB_PREFIXINFO_PREFIX_MODE, (void *)&ipv6PrefixMode)) {
		printf("Error!! get LAN IPv6 Prefix Mode fail!");
		goto setErr_ipv6;
	}

	prefixInfo->mode = ipv6PrefixMode;

	switch(prefixInfo->mode){
		case IPV6_PREFIX_DELEGATION:
			if (!_mib_get(MIB_PREFIXINFO_DELEGATED_WANCONN, (void *)&wanconn)) {
				printf("Error!! Get PREFIXINFO_DELEGATED WANCONN fail!");
				goto setErr_ipv6;
			}

			// Due to ATM's ifIndex can be 0
			prefixInfo->wanconn = wanconn;

			if (prefixInfo->wanconn == DUMMY_IFINDEX) //auto mode, get lease from most-recently leasefile.
			{
				if (g_dlg_info.wanIfname[0] == '\0')
					goto setErr_ipv6;
				prefixInfo->wanconn = _getIfIndexByName(g_dlg_info.wanIfname);
			}

			// get lease of selected wan
			if(prefixInfo->wanconn != DUMMY_IFINDEX)
			{
				ifGetName(prefixInfo->wanconn, tmpBuf, sizeof(tmpBuf));
				snprintf(leasefile, 64, "%s.%s", PATH_DHCLIENT6_LEASE, tmpBuf);
			}

			if (getLeasesInfo(leasefile,&dlgInfo) & LEASE_GET_IAPD) {
				memcpy(prefixInfo->prefixIP,dlgInfo.prefixIP,sizeof(prefixInfo->prefixIP));
				prefixInfo->prefixLen = dlgInfo.prefixLen;
				//     IPv6 network  may give prefix with length 56 by prefix delegation,
				//     but only prefix length = 64, SLAAC will work.
				//
				//Ref: rfc4862: Section 5.5.3.  Router Advertisement Processing
				//     If the sum of the prefix length and interface identifier length
				//     does not equal 128 bits, the Prefix Information option MUST be
				//     ignored.
				if (prefixInfo->prefixLen != 64)
					prefixInfo->prefixLen = 64;
				strncpy(prefixInfo->leaseFile, leasefile, IPV6_BUF_SIZE_128);
				prefixInfo->RNTime = dlgInfo.RNTime;
				prefixInfo->RBTime = dlgInfo.RBTime;
				prefixInfo->PLTime = dlgInfo.PLTime;
				prefixInfo->MLTime = dlgInfo.MLTime;
			}
			else{
				printf("[%s %d] Error! Could not get delegation info from wanconn %s, file %s!\n", __func__, __LINE__, tmpBuf, leasefile);
				goto setErr_ipv6;
			}
			break;
		case IPV6_PREFIX_STATIC:
			if (!_mib_get(MIB_IPV6_LAN_PREFIX, (void *)tmpBuf)) { //STRING_T
				printf("Error!! Get MIB_IPV6_LAN_PREFIX fail!");
				goto setErr_ipv6;
			}

			if (!_mib_get(MIB_IPV6_LAN_PREFIX_LEN, (void *)&prefixLen)) {
				printf("Error!! Get MIB_IPV6_LAN_PREFIX_LEN fail!");
				goto setErr_ipv6;
			}
			// Static prefix from WEB LAN IPv6
			if (tmpBuf[0]) {
				if (!inet_pton(PF_INET6, tmpBuf, prefixInfo->prefixIP))
					goto setErr_ipv6;
			}
			// Static prefix Length from WEB LAN IPv6
			prefixInfo->prefixLen = prefixLen;

			// AdvValidLifetime from from WEB LAN IPv6 RADVD
			if (!_mib_get(MIB_V6_VALIDLIFETIME, (void *)tmpBuf)) {
				printf("Get AdvValidLifetime mib error!");
				goto setErr_ipv6;
			}
			if(tmpBuf[0])
				prefixInfo->MLTime = (unsigned int)strtoul(tmpBuf, NULL, 10);

			// AdvPreferredLifetime from WEB LAN IPv6 RADVD
			if ( !_mib_get(MIB_V6_PREFERREDLIFETIME, (void *)tmpBuf)) {
				printf("Get AdvPreferredLifetime mib error!");
				goto setErr_ipv6;
			}
			if(tmpBuf[0])
				prefixInfo->PLTime = (unsigned int)strtoul(tmpBuf, NULL, 10);
			break;
		case IPV6_PREFIX_PPPOE:
		case IPV6_PREFIX_NONE:
		default:
			printf("Error! Not support this mode %d!\n", prefixInfo->mode);
	}
	return 0;

setErr_ipv6:
	return -1;
}

int _option_name_server(FILE *fp, char *server)
{
	unsigned int entryNum, i;
	MIB_DHCPV6S_NAME_SERVER_Tp pEntry;
	unsigned char strAll[(MAX_V6_IP_LEN+2)*MAX_DHCPV6_CHAIN_ENTRY]="";

	entryNum = _mib_chain_total(MIB_DHCPV6S_NAME_SERVER_TBL);

	for (i=0; i<entryNum; i++) {
		unsigned char buf[MAX_V6_IP_LEN+2]="";

		pEntry = (MIB_DHCPV6S_NAME_SERVER_Tp) _mib_chain_get(MIB_DHCPV6S_NAME_SERVER_TBL, i);

		if ( i< (entryNum-1) )
		{
			sprintf(buf, "%s, ", pEntry->nameServer);
		} else
			sprintf(buf, "%s", pEntry->nameServer);
		strcat(strAll, buf);
	}

	if ( entryNum > 0 )
	{
		if(server !=NULL) {
			if(server[0] != 0) {
				fprintf(fp, "option dhcp6.name-servers %s,%s;\n", server, strAll);
			}
			else
				fprintf(fp, "option dhcp6.name-servers %s;\n", strAll);
		}
		else
			fprintf(fp, "option dhcp6.name-servers %s;\n", strAll);
	}

	return 0;
}

int _option_domain_search(FILE *fp)
{
	unsigned int entryNum, i;
	MIB_DHCPV6S_DOMAIN_SEARCH_Tp pEntry;
	unsigned char strAll[(MAX_DOMAIN_LENGTH+4)*MAX_DHCPV6_CHAIN_ENTRY]="";

	entryNum = _mib_chain_total(MIB_DHCPV6S_DOMAIN_SEARCH_TBL);

	for (i=0; i<entryNum; i++) {
		unsigned char buf[MAX_DOMAIN_LENGTH+4]="";

		pEntry = (MIB_DHCPV6S_DOMAIN_SEARCH_Tp) _mib_chain_get(MIB_DHCPV6S_DOMAIN_SEARCH_TBL, i);

		if ( i< (entryNum-1) )
		{
			sprintf(buf, "\"%s\", ", pEntry->domain);
		} else
			sprintf(buf, "\"%s\"", pEntry->domain);
		strcat(strAll, buf);
	}

	if ( entryNum > 0 )
	{
		//printf("strAll(domain)=%s\n", strAll);
		fprintf(fp, "option dhcp6.domain-search %s;\n", strAll);
	}

	return 0;
}

int _option_ntp_server(FILE *fp)
{
	unsigned int entryNum, i;
	MIB_DHCPV6S_NTP_SERVER_Tp pEntry;
	unsigned char strAll[(MAX_V6_IP_LEN+2)*MAX_DHCPV6_CHAIN_ENTRY]="";

	entryNum = _mib_chain_total(MIB_DHCPV6S_NTP_SERVER_TBL);

	for (i=0; i<entryNum; i++) {
		unsigned char buf[MAX_V6_IP_LEN+2]="";

		pEntry = (MIB_DHCPV6S_NTP_SERVER_Tp) _mib_chain_get(MIB_DHCPV6S_NTP_SERVER_TBL, i);

		if ( i< (entryNum-1) )
		{
			sprintf(buf, "%s, ", pEntry->ntpServer);
		} else
			sprintf(buf, "%s", pEntry->ntpServer);
		strcat(strAll, buf);
	}

	if ( entryNum > 0 )
	{
		if(strAll[0]) {
			//printf("strAll(domain)=%s\n", strAll);
			fprintf(fp, "option dhcp6.sntp-servers %s;\n", strAll);
		}
	}

	return 0;
}

int _option_mac_binding(FILE *fp)
{
	unsigned int entryNum, i;
	MIB_DHCPV6S_MAC_BINDING_Tp pEntry;

	entryNum = _mib_chain_total(MIB_DHCPV6S_MAC_BINDING_TBL);

	for (i=0; i<entryNum; i++) {
		pEntry = (MIB_DHCPV6S_MAC_BINDING_Tp) _mib_chain_get(MIB_DHCPV6S_MAC_BINDING_TBL, i);

		fprintf(fp, "host %s {\n", pEntry->hostName);
		fprintf(fp, "    hardware ethernet %02x:%02x:%02x:%02x:%02x:%02x ;\n",
			pEntry->macAddr[0],pEntry->macAddr[1],pEntry->macAddr[2],pEntry->macAddr[3],pEntry->macAddr[4],pEntry->macAddr[5]);
		fprintf(fp, "    fixed-address6 %s ;\n", pEntry->ipAddress);
		fprintf(fp, "}\n");
	}

	return 0;
}

/* Receive ISC dhcpv6 reply packet will send message to do this. */
int delLANIP(const char *fname)
{
	int ret=0;
	unsigned char value[64];
	unsigned int uInt, i;
	struct in6_addr targetIp;
	unsigned char devAddr[MAC_ADDR_LEN];
	unsigned char meui64[8];

	ret = getLeasesInfo(fname, &g_dlg_info);
	// found Prefix Delegation
	if (ret & LEASE_GET_IAPD) {
		uInt = g_dlg_info.prefixLen;
		if (uInt<=0 || uInt > 128) {
			printf("[%s(%d)]WARNNING! Prefix Length == %d\n", __FUNCTION__, __LINE__, uInt);
			uInt = 64;
		}
		ip6toPrefix((struct in6_addr *)g_dlg_info.prefixIP, uInt, &targetIp);
		_mib_get(MIB_ELAN_MAC_ADDR, (void *)devAddr);
		mac_meui64(devAddr, meui64);
		for (i=0; i<8; i++)
			targetIp.s6_addr[i+8] = meui64[i];
		inet_ntop(PF_INET6, &targetIp, value, sizeof(value));
		sprintf(value+(strlen(value)), "/%d", uInt);
		printf("Delete the previous LAN IP for Prefix Delegation\n");
		va_cmd_no_error(IFCONFIG, 3, 1, LANIF, "del", value);
	}
	return 1;
}

static void cfg_stop_delegation(struct mymsgbuf *qbuf)
{
	delLANIP(qbuf->msg.mtext);
	qbuf->request = MSG_SUCC;
}

static void cfg_get_PD_prefix_ip(struct mymsgbuf *qbuf)
{
#if defined(CONFIG_RTK_DEV_AP) || defined(CONFIG_TELMEX_DEV)
	PREFIX_V6_INFO_T prefixInfo={0};

	_get_prefixv6_info(&prefixInfo);
	memcpy(qbuf->msg.mtext, prefixInfo.prefixIP, IP6_ADDR_LEN);
#else
	memcpy(qbuf->msg.mtext, g_dlg_info.prefixIP, IP6_ADDR_LEN);
#endif
	qbuf->request = MSG_SUCC;
}

static void cfg_get_PD_prefix_len(struct mymsgbuf *qbuf)
{
#if defined(CONFIG_RTK_DEV_AP) || defined(CONFIG_TELMEX_DEV)
	PREFIX_V6_INFO_T prefixInfo={0};

	_get_prefixv6_info(&prefixInfo);
	qbuf->msg.arg1 = prefixInfo.prefixLen;
#else
	qbuf->msg.arg1 = g_dlg_info.prefixLen;
#endif
	qbuf->request = MSG_SUCC;
}
#endif
#endif // #ifdef CONFIG_IPV6

/*
 *	upload <filename>	---	upload firmware
 */
#include <sys/socket.h>
#include <sys/ioctl.h>
#define SIOCETHTEST 0x89c0

struct arg{
	unsigned char cmd;
	unsigned int cmd2;
	unsigned int cmd3;
	unsigned int cmd4;
}pass_arg_saved;

void reboot_by_watchdog()
{
	struct ifreq	ifr;
  	int fd=0;
	int ret =0;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
  	if(fd< 0){
		printf("Saved: Watchdog control fail!\n");
		goto fail;
  	}
	strncpy(ifr.ifr_name, "eth0", 16);
	ifr.ifr_data = (void *)&pass_arg_saved;

	pass_arg_saved.cmd=14;  //watchdog command
    	pass_arg_saved.cmd2=2;
    	pass_arg_saved.cmd3=0;

	ret=ioctl(fd, SIOCETHTEST, &ifr);
	if(ret)
		printf("ioctl error!\n");
	close(fd);
fail:
	return;
}

#ifdef CONFIG_USER_ANDLINK_PLUGIN
static void _set_upload_done_file(int value)
{
	unsigned char cmd[128] = {0};

	snprintf(cmd, sizeof(cmd), "echo %d > %s", value, ANDLINK_PLUGIN_UPLOAD_FNISH_FILE);
	printf(cmd);
	system(cmd);
}
#else
static void _set_upload_done_file(int value)
{
	char str[32] = "";

	if(value == 0)
		rtk_util_write_str("/tmp/fwu_success", 1, "1");
	else
		rtk_util_write_str("/tmp/fwu_success", 1, "0");
}
#endif

#ifdef CONFIG_LUNA_FIRMWARE_UPGRADE_SUPPORT
#ifdef CONFIG_LUNA_FWU_SIGNATURE
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/pem.h>

const char *CERT = "/etc/fwu_cert.pem";
#define FWU_SIG_SIZE 256
#define  FWU_BUF_SIZE 65536

static int rtk_fwu_verify_fw(const char *fname, size_t size)
{
	FILE *f_img = NULL;
	unsigned char *buffer = NULL;
	int bytesRead = 0;
	int rc = -1;
	int result = -1;
	unsigned char sig[FWU_SIG_SIZE] = {0};

	EVP_MD_CTX *ctx = NULL;
    FILE *cert = NULL;
    X509 *x509 = NULL;
    EVP_PKEY *pkey = NULL;

	if(fname == NULL)
		return -1;

	OpenSSL_add_all_digests();

	f_img = fopen(fname, "r");
	if(f_img == NULL)
	{
		fprintf(stderr, "Open %s failed\n", fname);
		return -1;
	}

	// tail FWU_SIG_SIZE bytes is signature
	size -= FWU_SIG_SIZE;

	/* Extract public key from certificate */
	cert = fopen(CERT, "r");
	if(!cert)
	{
		printf("Open certificate %s failed\n", CERT);
		goto verify_failed;
	}
    x509 = PEM_read_X509(cert, NULL, NULL, NULL);
	if(!x509)
	{
		printf("Read certificate %s failed\n", CERT);
		goto verify_failed;
	}
	fclose(cert);
    pkey = X509_get_pubkey(x509);
	if(!x509)
	{
		printf("X509_get_pubkey failed\n");
		goto verify_failed;
	}

	buffer = malloc(FWU_BUF_SIZE);
	if(!buffer) return ENOMEM;

	ctx = EVP_MD_CTX_create();
	if(ctx == NULL) {
		printf("EVP_MD_CTX_create failed, error 0x%lx\n", ERR_get_error());
		goto verify_failed;
	}

	rc = EVP_DigestVerifyInit(ctx, NULL, EVP_sha256(), NULL, pkey);
	if(rc != 1) {
		printf("EVP_DigestVerifyInit failed, error 0x%lx\n", ERR_get_error());
		goto verify_failed;
	}

	printf("file size is %lu\n", size);
	do
	{
		bytesRead = fread(buffer, 1, (FWU_BUF_SIZE < size) ? FWU_BUF_SIZE : size, f_img);
		rc = EVP_DigestVerifyUpdate(ctx, buffer, bytesRead);
		if(rc != 1) {
			printf("EVP_DigestVerifyUpdate failed, error 0x%lx\n", ERR_get_error());
			goto verify_failed;
		}
		size -= bytesRead;
	}while(size > 0);

	bytesRead = fread(sig, 1, FWU_SIG_SIZE, f_img);
	if(bytesRead != FWU_SIG_SIZE)
	{
		printf("<%s:%d> Read signature failed\n", __func__, __LINE__);
		goto verify_failed;
	}

	rc = EVP_DigestVerifyFinal(ctx, sig, FWU_SIG_SIZE);
	if(rc != 1) {
		printf("EVP_DigestVerifyFinal failed, error 0x%lx\n", ERR_get_error());
		goto verify_failed;
	}
	result = 0;

verify_failed:
	if(pkey)
		EVP_PKEY_free(pkey);
	if(x509)
		X509_free(x509);
	if(ctx) {
		EVP_MD_CTX_destroy(ctx);
		ctx = NULL;
	}
	EVP_cleanup();
	if(buffer)
		free(buffer);
	if(f_img)
	{
		fclose(f_img);
		f_img = NULL;
	}
	return result;
}
#if defined(CONFIG_YUEME) || defined(CONFIG_CU)
static int skip_signature_check(void)
{
	if(0==access("/tmp/skip_signature_check", F_OK))
	{
		printf("skip signature check!\n");
		return 1;
	}
	return 0;
}
#endif
#endif //CONFIG_LUNA_FWU_SIGNATURE

static void fw_upload(const char *fname, int offset, int imgFileSize, int needreboot, int needcommit)
{
	int active, ret = -1, upgrade_bank, commit = 0;
	char str_active[64], buf[256], log_str[100];
	char version0[64], version1[64];
#ifdef CONFIG_00R0
	char str_updater[64]={0}, str_commit[64]={0};
	int commit;
#endif
	struct stat st;
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	char tmpfilep[] = "/var/tmp2/imgXXXXXX";
#else
	char tmpfilep[] = "/tmp/imgXXXXXX";
#endif
	int src_fd=0, dst_fd=0;
	size_t count;
	int fwu_success = 1;
#ifdef CONFIG_USER_LOWMEM_UPGRADE
	int lowmem = 0, freekB = 0;
	const int RESERVE_MEM_KB = (5*1024);
	FILE *memfp = NULL;
#endif

	if (stat(fname, &st)) {
		perror("stat");
		fwu_success = 0;
		goto done;
	}

#if CONFIG_00R0
	/* Status LED blinks slowly */
	system("echo '4'>/proc/internet_flag");
#endif

	/* copy the necessary part */
	if (offset != 0 || imgFileSize != st.st_size) {
		src_fd = open(fname, O_RDONLY);
		if (src_fd < 0) {
			perror("src_fd");
			fwu_success = 0;
			goto done;
		}

		dst_fd = mkstemp(tmpfilep);
		if (dst_fd < 0) {
			perror("dst_fd");
			close(src_fd);
			fwu_success = 0;
			goto done;
		}

		if(lseek(src_fd, offset, SEEK_SET) == -1)
			printf("lseek failed: %s %d\n", __func__, __LINE__);
		while (imgFileSize > 0) {
			count = (sizeof(buf) < imgFileSize) ? sizeof(buf) : imgFileSize;
			ret = read(src_fd, buf, count);
			assert(ret == count);

			count = ret;
			ret = write(dst_fd, buf, count);
			assert(ret == count);

			imgFileSize -= ret;
		}

		close(src_fd);
		close(dst_fd);
		fname = tmpfilep;
	}

#ifdef CONFIG_LUNA_FWU_SIGNATURE
#if defined(CONFIG_YUEME) || defined(CONFIG_CU)
	if(skip_signature_check() != 1)
#endif
	{
	if(rtk_fwu_verify_fw(fname, imgFileSize) != 0)
	{
		printf("Firmware signature verified failed, skip upgrading\n");
		ret = -1;
		goto done;
	}
	printf("Firmware signature verified OK. Continue upgrading...\n");
	}
#endif
#ifdef CONFIG_E8B
	// For E8 project skip software version check
	system("echo 1 > /tmp/skip_swver_check");
#endif
	syslog(LOG_ALERT, "FW: superadmin process the firmware upgrading...");
	/* if active is '0' now, then '1' should be upgraded */
	rtk_env_get("sw_active", str_active, sizeof(str_active));
	sscanf(str_active, "sw_active=%d", &active);
#if defined(CONFIG_00R0) && defined(CONFIG_LUNA_FWU_SYNC)
	rtk_env_get("sw_commit", str_commit, sizeof(str_commit));
	sscanf(str_commit, "sw_commit=%d", &commit);
	/* For the fail safe machanism, tryactive and commit should not be the same.
	   Since we will set (1-active) to tryative, it implies the following  */
	if(commit != active) {
		rtk_env_set("sw_commit", str_active);
	}
#endif
#if defined(CONFIG_ARCH_RTL8198F) & !defined(LUNA_MULTI_BOOT)
	sprintf(str_active, "%d", 0);//single image
#elif defined(CONFIG_RTK_SOC_RTL8198D) && defined(CONFIG_MTD_SPI_NOR)
	sprintf(str_active, "%d", 0);//single image
#else
	sprintf(str_active, "%d", 1 - active);
#endif
	upgrade_bank = 1 - active;

#ifdef CONFIG_USER_LOWMEM_UPGRADE
	memfp = popen("cat /proc/meminfo | grep MemFree", "r");
	if(memfp)
	{
		char freeStr[32] = {0}, *ptr = freeStr;

		fgets(buf, sizeof(buf), memfp);
		pclose(memfp);

		sscanf(buf, "%*[^:]:%[^?]", freeStr);
		while(*ptr==' ')
			ptr++;

		freekB = atoi(ptr);
		printf("[%s:%d] free memory: %d kB, image size: %d kB, reserve: %d kB\n", __FUNCTION__, __LINE__, freekB, imgFileSize/1024, RESERVE_MEM_KB);
		if((imgFileSize/1024+RESERVE_MEM_KB) > freekB)
		{
			printf("[%s:%d] low memory upgrade mode!\n", __FUNCTION__, __LINE__);
			lowmem = 1;
		}
	}

	snprintf(buf, sizeof(buf), "/etc/scripts/fwu_starter.sh %s %s %d", str_active, fname, lowmem);
	printf("%s\n", buf);
#else
	snprintf(buf, sizeof(buf), "/etc/scripts/fwu_starter.sh %s %s", str_active, fname);
#endif
	/* bank invalid */
	ret = set_valid_info(upgrade_bank, 0);
	if(ret < 0)
	{
		printf("failed to set valid info\n");
		ret = -1;
		goto done;
	}

	/* To prevent system() to return ECHILD */
	signal(SIGCHLD, SIG_DFL);
	ret = system(buf);

	if (ret == 0)
	{
#ifdef CONFIG_USER_CWMP_TR069
		/* cwmpClient have set fault code to 9010, reset to 0 if sucess.*/
		/* Fix me if you have better idea. */
		unsigned int fault = 0;
		mib_set( CWMP_DL_FAULTCODE, &fault );
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
#endif

#ifdef CONFIG_00R0
#ifdef CONFIG_LUNA_FWU_SYNC
		rtk_env_get("sw_updater", str_updater, sizeof(str_updater));
		if(strcmp(str_updater,"sw_updater=web")==0){
			printf("[FWU_SYNC] Firmware update by Web!\n");
			rtk_env_set("sw_tryactive", str_active);
		}else{  /* update by TR069 */
			printf("[FWU_SYNC] Firmware update by TR069!\n");
			rtk_env_set("sw_commit", str_active);
		}
#else
		rtk_env_set("sw_commit", str_active);
#endif
#else
		// do not switch image if upgraded by usp-tr369
		if(needreboot >= 0)
		{
#ifndef USE_DEONET
			rtk_env_set("sw_updater", "web");
			rtk_env_set("sw_tryactive", str_active);
#endif
		}
#endif

#ifdef SUPPORT_WEB_PUSHUP
		if(firmwareUpgradeConfigStatus()==FW_UPGRADE_STATUS_PROGGRESSING)
			firmwareUpgradeConfigStatusSet(FW_UPGRADE_STATUS_SUCCESS);
#endif
	}
	else
	{
		fwu_success = 0;
		ret = -1;
		goto done;
		printf("FW Upgrade failed ...\n");
	}


#ifdef CONFIG_USER_RTK_ONUCOMM
	{
		//record upgrade status to inform gateway after reboot
		unsigned int fw_status;
		//fprintf(stderr,"\tret=0x%x,WIFEXITED(ret)=0x%x\n",ret,WIFEXITED(ret));
		if(ret==0)
			fw_status = 1;
		else fw_status = 2;
		//fprintf(stderr,"\tfw_status=%d\n",fw_status);
		mib_set(MIB_ONUCOMM_FW_STATUS, &fw_status);
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	}
#endif
#if defined(CONFIG_USER_AP_CMCC) && defined(CONFIG_ANDLINK_SUPPORT)
	{
		unsigned char tmpvalue = 1;

		if(ret == 0){
			mib_set(MIB_RTL_LINK_FW_UPGRADE, (void *)&tmpvalue);
			mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
		}
	}
#endif

#if defined(CONFIG_AIRTEL_EMBEDUR)
	is_fwupgrade = 1;
#endif

#if defined(CONFIG_USER_CWMP_TR069)
	if (WEXITSTATUS(ret) == 1)
	{
		fprintf(stderr, "[%s@%d] firmware upgrade script return error\n", __FUNCTION__, __LINE__);
		unsigned int fault = 9018;
		FILE *fp = NULL;
		char line[32] = {0};
		int status = 0;
		fp = fopen("/var/firmware_upgrade_status", "r");
		if (fp)
		{
			fgets(line, sizeof(line), fp);
			if (strlen(line))
			{
				sscanf(line, "%d", &status);
			}
			fclose(fp);
		}

		fprintf(stderr, "firmware_upgrade_status = %d\n", status);
#ifdef CONFIG_00R0
		if (status == 4) /* skip same firmware version check */
		{
			fault = 0;
		}
#endif
		mib_set(CWMP_DL_FAULTCODE, &fault);
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	}
#endif
#ifdef CONFIG_E8B
	int remomve_lan_wan_bind;
	mib_get( MIB_CMCC_UPLOAD_REMOVE_LAN_WAN_BIND, &remomve_lan_wan_bind );
	//printf("remomve_lan_wan_bind %d.\n",remomve_lan_wan_bind);
	if(remomve_lan_wan_bind == 1){
		printf("remove lan wan bind.\n");
		MIB_CE_ATM_VC_T pvcEntry;
		int i = 0, vcNum = 0;
		vcNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=0; i<vcNum; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&pvcEntry) || !pvcEntry.itfGroup)
				continue;
			pvcEntry.itfGroup = 0;
			mib_chain_update(MIB_ATM_VC_TBL, (void *)&pvcEntry, i);
		}
		//printf("remove lan wan bind end.\n");
		remomve_lan_wan_bind = 0;
		mib_set(MIB_CMCC_UPLOAD_REMOVE_LAN_WAN_BIND, &remomve_lan_wan_bind);
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	}
#endif

	if (needreboot > 0)
	{
		printf("The system is restarting ...%d\n", fwu_success);
		if(fwu_success == 0) {
#ifdef USE_DEONET
			syslog2(LOG_DM_CONFIGD, LOG_DM_CONFIGD_FAIL, "FW Upgrade Fail (reason : Upgrade)");
			if(needcommit)
			{
				/* bank commit */
				ret = set_commit_info(active);
				if(ret < 0)
					printf("failed to set commit info\n");
			}
#else
			syslog(LOG_ALERT, "FW: superadmin process the firmware upgrade failed.");
#endif
		}
		else{
#ifdef USE_DEONET
			commit = get_commit_info();
			sprintf(log_str, "FW Version(%04d) Upgrade Success",g_upload_version);
			syslog2(LOG_DM_CONFIGD, LOG_DM_CONFIGD_CMPL, log_str);
/* bank version */
			ret = get_version_info(version0, version1);
			if(ret < 0)
				printf("failed to get version info\n");

			ret = set_swme_version(upgrade_bank, upgrade_bank == 0 ? version0 : version1);
			if(ret < 0)
				printf("failed to set version info\n");

			/* bank valid */
			ret = set_valid_info(upgrade_bank, 1);
			if(ret < 0)
				printf("failed to set valid info\n");

			if(needcommit)
			{
				/* bank commit */
				ret = set_commit_info(upgrade_bank);
				if(ret < 0)
					printf("failed to set commit info\n");
			}
#else
			syslog(LOG_ALERT, "FW: superadmin process the firmware upgrade success.");
#endif
		}

		syslog2(LOG_DM_HTTP, LOG_DM_HTTP_RESET, "Reboot by Web (Upgrade)");
		_set_upload_done_file(ret);
		sleep(3);
		reboot_now = 1;
		sysReboot(NULL);
		return ;
	}
	else {
		printf("(needreboot = %d) The system is not going to restart. \n", needreboot);
	}

done:
	_set_upload_done_file(ret);
	if(fwu_success == 0){
		printf("System software upgrade failed.\n");
#ifdef USE_DEONET
		syslog2(LOG_DM_CONFIGD, LOG_DM_CONFIGD_FAIL, "FW Upgrade Fail (reason : Upgrade)");
		if(needcommit)
		{
			/* bank commit */
			ret = set_commit_info(active);
			if(ret < 0)
				printf("failed to set commit info\n");
		}

#else
#ifdef CONFIG_E8B
		syslog(LOG_ALERT, "%d System software upgrade failed.", CTCOM_ALARM_SW_UPGRADE_FAIL);
#else
		syslog(LOG_ALERT, "FW: superadmin process the firmware upgrade failed.");
#endif
#endif
	}
	else
	{
#ifdef USE_DEONET
		commit = get_commit_info();
		sprintf(log_str, "FW Version(%04d) Upgrade Success", g_upload_version);
		syslog2(LOG_DM_CONFIGD, LOG_DM_CONFIGD_CMPL, log_str);
		/* bank version */
		ret = get_version_info(version0, version1);
		if(ret < 0)
			printf("failed to get version info\n");

		ret = set_swme_version(upgrade_bank, upgrade_bank == 0 ? version0 : version1);
		if(ret < 0)
			printf("failed to set version info\n");

		/* bank valid */
		ret = set_valid_info(upgrade_bank, 1);
		if(ret < 0)
			printf("failed to set valid info\n");

		if(needcommit)
		{
			/* bank commit */
			ret = set_commit_info(upgrade_bank);
			if(ret < 0)
				printf("failed to set commit info\n");
		}

#else
		syslog(LOG_ALERT, "FW: superadmin process the firmware upgrade success.");
#endif
	}
#ifdef SUPPORT_WEB_PUSHUP
	if(ret != 0 && firmwareUpgradeConfigStatus()==FW_UPGRADE_STATUS_PROGGRESSING)
		firmwareUpgradeConfigStatusSet(FW_UPGRADE_STATUS_OTHERS);
	else
	 	firmwareUpgradeConfigStatusSet(FW_UPGRADE_STATUS_NOTIFY);
#endif
	unlink(fname);
	if (needreboot >= 3) {
		printf("Force system restarting ...\n");
		sleep(3);
		reboot_now = 1;
		sysReboot(NULL);
	}
}
#else
static void fw_upload(const char *fname, int offset, int imgFileSize, int needreboot)
{
	int ret = -1;
	FILE *fp = NULL;
	struct stat st;
	int fsize;
	int hasHdr;
	IMGHDR imgHdr;
#ifdef CONFIG_DOUBLE_IMAGE
	int partCheck = 0;
#endif
#ifdef CONFIG_RTL8686
	char part_name[32];
	unsigned int image_len = 0;
#endif

#ifdef CONFIG_LIB_LIBRTK_LED
	rtk_setLed(LED_PWR_G, RTK_LED_ON, NULL);
#else
// Kaohj -- TR068 Power LED
//star: for ZTE power LED request
	unsigned char power_flag;

	fp = fopen("/proc/power_flag", "w");
	if (fp) {
		power_flag = '1';
		fwrite(&power_flag, 1, 1, fp);
		fclose(fp);
		fp = NULL;
	}
#endif /* CONFIG_LIB_LIBRTK_LED */

#ifdef ENABLE_SIGNATURE
	offset += sizeof(SIGHDR);
	fsize = imgFileSize - sizeof(SIGHDR);
#else
	fsize = imgFileSize;
#endif

	if ((fp = fopen(fname, "rb")) == NULL) {
		fprintf(stderr, "File %s open fail\n", fname);
		goto ERROR_RET;
	}

	if (fstat(fileno(fp), &st) < 0)
		goto ERROR_RET;

	if (fseek(fp, offset, SEEK_SET) == -1)
		goto ERROR_RET;

	if (fsize <= 0)
		goto ERROR_RET;

	// simple check for image header. Making it backward compatible for now.
	// Andrew
#ifdef CONFIG_RTL8686
check_next_header:
	if (1 == fread(&imgHdr, sizeof(imgHdr), 1, fp)) {
		switch (imgHdr.key) {
		case APPLICATION_UBOOT:
			hasHdr = 1;
			strcpy(part_name, "boot");
			printf("%s-%d::find uboot img\n", __func__, __LINE__);
			break;
		case APPLICATION_UIMAGE:
			hasHdr = 1;
			strcpy(part_name, "linux");
			printf("%s-%d::find linux img\n", __func__, __LINE__);
			break;
		case APPLICATION_ROOTFS:
			hasHdr = 1;
			strcpy(part_name, "rootfs");
			printf("%s-%d::find rootfs img\n", __func__, __LINE__);
			break;
		default:
			hasHdr = 0;
			fseek(fp, offset, SEEK_SET);
			printf("img with unknown header! hasHdr=%d\n", hasHdr);
			break;
		}
		if (hasHdr) {
			image_len = imgHdr.length;
			printf("%s(%d)::image_len:%d\n", __func__,
			       __LINE__, image_len);
#ifndef CONFIG_ENABLE_KERNEL_FW_UPDATE
			fsize -= sizeof(IMGHDR);
#endif
		}
	} else {
		hasHdr = 0;
		if(fseek(fp, offset, SEEK_SET) ==  -1)
			printf("%s-%d fseek error\n",__func__,__LINE__);
		printf("img without header! hasHdr=%d\n", hasHdr);
	}
#else //CONFIG_RTL8686
	if ((1 == fread(&imgHdr, sizeof(imgHdr), 1, fp)) &&
	    (APPLICATION_IMAGE == imgHdr.key)) {
		hasHdr = 1;
		fsize -= sizeof(IMGHDR);
	} else {
		hasHdr = 0;
		if(fseek(fp, offset, SEEK_SET)==-1)
			printf("%s-%d fseek error\n",__func__,__LINE__);
	}
#endif /*CONFIG_RTL8686 */

	printf("filesize(Not include imgHdr(64 bytes))  = %d\n", fsize);
	printf("imgFileSize = %d\n", imgFileSize);

#ifdef CONFIG_ENABLE_KERNEL_FW_UPDATE
	do {
		char buf[128];
		int part = 1;

#ifdef CONFIG_PPP
		// Jenny, disconnect PPP before rebooting
		stopPPP();
#endif
#ifdef CONFIG_DOUBLE_IMAGE
		//printf("check current rootfs... ");
		//ql: check the run-time image block
		flash_read(&partCheck, PART_CHECK_OFFSET, sizeof(partCheck));

		printf("[%s(%d)] partCheck=0x%X\n",__func__,__LINE__,partCheck);

		if (!partCheck) {
			printf("latest is the first!\n");
			partCheck = 0xffffffff;
			flash_write(&partCheck, PART_CHECK_OFFSET, sizeof(partCheck));
			part = 2;
			printf("write second rootfs finished!\n");
		} else {
			printf("latest is the second!\n");
			partCheck = 0;
			flash_write(&partCheck, PART_CHECK_OFFSET, sizeof(partCheck));
			part = 1;
			printf("write first rootfs finished!\n");
		}
#endif /* CONFIG_DOUBLE_IMAGE */

		system("echo -n \"1321\" > /proc/realtek/fwupdate");
#ifdef CONFIG_RTL8686
		/* 8696 series take whole image as input */
		snprintf(buf, sizeof(buf), "echo -n \"%d;%s;%d;%d\" > /proc/realtek/fwupdate", offset, fname, part, fsize);
#else
		if (hasHdr) {
			snprintf(buf, sizeof(buf), "echo -n \"%d;%s;%d;%d\" > /proc/realtek/fwupdate", offset+sizeof(imgHdr), fname, part, fsize);
		}
		else {
			snprintf(buf, sizeof(buf), "echo -n \"%d;%s;%d;%d\" > /proc/realtek/fwupdate", offset, fname, part, fsize);
		}
#endif
		printf("cmd: %s\n", buf);
		fclose(fp);
		fp == NULL;
		system(buf);
		while (1);
	} while (0);
#else
#ifdef CONFIG_DOUBLE_IMAGE
	flash_read(&partCheck, PART_CHECK_OFFSET, sizeof(partCheck));

	if (!partCheck) {
		printf("latest is the first!\n");

		ret = flashdrv_filewrite(fp, fsize, (void *)g_fs_bak_offset);

		partCheck = 0xffffffff;

		flash_write(&partCheck, PART_CHECK_OFFSET, sizeof(partCheck));

		printf("write second rootfs finished!\n");
	} else {
		printf("latest is the second!\n");

		ret = flashdrv_filewrite(fp, fsize, (void *)g_rootfs_offset);

		partCheck = 0;

		flash_write(&partCheck, PART_CHECK_OFFSET, sizeof(partCheck));

		printf("write first rootfs finished!\n");
	}
#else	//!CONFIG_DOUBLE_IMAGE start
#ifdef CONFIG_RTL8686
	if (hasHdr) {
		ret = flash_filewrite(fp, image_len, part_name);
		fsize -= image_len;

		if (fsize > sizeof(imgHdr)) {
			goto check_next_header;
		}
	} else {
		printf("%s-%d:: can't find your image KEY! ret:%d\n", __func__,
		       __LINE__, ret);
	}
#else /* else CONFIG_RTL8686 */
	ret = flashdrv_filewrite(fp, fsize, (void *)g_rootfs_offset);
#endif /* CONFIG_RTL8686 */
#endif /* CONFIG_DOUBLE_IMAGE*/

	if (ret)
		printf("flash error!\n");
	else
		printf("flash write completed !!\n");

	_set_upload_done_file(ret);

#endif //#ifdef CONFIG_ENABLE_KERNEL_FW_UPDATE
ERROR_RET:
	if(fp!=NULL)
		fclose(fp);
	unlink(fname);

	if(needreboot)
	{
		printf("The system is restarting ...\n");
		sysReboot(NULL);
	}

}
#endif

static void *fwUpload(void *data)
{
#ifdef CONFIG_USER_RTK_RECOVER_SETTING	// Save Setting(cs&hs) to flatfsd(flash).
	va_cmd("/bin/saveconfig", 1, 1, "-s");
	va_cmd("/bin/saveconfig", 2, 1, "-s", "hs");
#endif
	/*
	 * Default value is PTHREAD_CANCEL_DEFERRED that
	 * a cancellation request is deferred until the thread
	 * next calls a function that is a cancellation point.
	 * Setting cancel type to PTHREAD_CANCEL_ASYNCHRONOUS is for
	 * the thread being canceled at any time.
	 */
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	fw_upload(g_upload_post_file_name, g_upload_startPos, g_upload_fileSize, g_upload_needreboot, g_upload_needcommit);
	if (access("/tmp/g_upgrade_firmware", F_OK) == 0){
		unlink("/tmp/g_upgrade_firmware");
	}

	// We can use pthread_exit() or return to terminate this thread.
	pthread_exit(NULL);
}

static void cfg_upload(struct mymsgbuf *qbuf)
{
	static pthread_t ptUploadId;

	if (qbuf->msg.arg2 == 0) {
		printf("Cancel Upload therad\n");

		pthread_cancel(ptUploadId);
		if (access("/tmp/g_upgrade_firmware", F_OK) == 0){
			unlink("/tmp/g_upgrade_firmware");
		}
	} else {
		//tylo, telefonica LED flash while firmware upgrade
		if (access("/proc/fwupgrade_flag", F_OK) == 0)
			system("echo 1 > /proc/fwupgrade_flag");

#if 0 /* why ?? */
		unlink("/var/log/messages");
#endif

		g_upload_startPos = qbuf->msg.arg1;
		g_upload_fileSize = qbuf->msg.arg2;
		g_upload_needreboot = qbuf->msg.arg3 & UPGRADE_FLAG_REBOOT ? 1 : 0;
		g_upload_needcommit = qbuf->msg.arg3 & UPGRADE_FLAG_COMMIT ? 1 : 0;
		g_upload_version = (qbuf->msg.arg3 >> UPGRADE_FLAG_VERSION_SHIFT)  & UPGRADE_FLAG_VERSION_MASK;

		strncpy(g_upload_post_file_name, qbuf->msg.mtext,
			MAX_SEND_SIZE);
		g_upload_post_file_name[MAX_SEND_SIZE - 1] = '\0';

		// To create Upload thread
		printf("Create Upload thread\n");

		pthread_create(&ptUploadId, NULL, fwUpload, NULL);
		pthread_detach(ptUploadId);
	}
	qbuf->request = MSG_SUCC;
}

static void cfg_check_image(struct mymsgbuf *qbuf)
{
	FILE	*fp=NULL;
	IMGHDR imgHdr;
	unsigned int csum;
	int nRead, total = 0;
	unsigned char buf[64];
	int offset, size, remain, block;
#ifdef CONFIG_RTL8686
	int err=-1;
#endif
#if defined(ENABLE_SIGNATURE)
	SIGHDR sigHdr;
	int i;
	unsigned int hdrChksum;
#endif

	qbuf->request = MSG_FAIL;
	offset = qbuf->msg.arg1;

	if ((fp = fopen (qbuf->msg.mtext, "rb")) == NULL) {
		printf("File %s open fail\n", qbuf->msg.mtext);
		return;
	}

	if (fseek(fp, offset, SEEK_SET)==-1) {
		//jim should delete below fclose, otherwise will be closed twice...
		//fclose(fp);
		goto M_ERROR;
	}
#if defined(ENABLE_SIGNATURE)
//ql add to check if the image is right.
	memset(&sigHdr, 0, sizeof(sigHdr));
	if (1 != fread(&sigHdr, sizeof(sigHdr), 1, fp)) {
		printf("failed to read signature\n");
		goto M_ERROR;
	}
#endif

	if (1!=fread(&imgHdr, sizeof(imgHdr), 1, fp)) {
		printf("Failed to read header\n");
		goto M_ERROR;
	}
#ifndef ENABLE_SIGNATURE_ADV
#ifdef ENABLE_SIGNATURE
	if (sigHdr.sigLen > SIG_LEN) {
		printf("signature length error\n");
		goto M_ERROR;
	}
	for (i=0; i<sigHdr.sigLen; i++)
		sigHdr.sigStr[i] = sigHdr.sigStr[i] - 10;
	if (strcmp(sigHdr.sigStr, SIGNATURE)) {
		printf("signature error\n");
		goto M_ERROR;
	}

	hdrChksum = sigHdr.chksum;
	hdrChksum = ipchksum(&imgHdr, sizeof(imgHdr), hdrChksum);
	if (hdrChksum) {
		printf("Checksum failed(msgparser cfg_check_image), size=%d, csum=%04xh\n", sigHdr.sigLen, hdrChksum);
		goto M_ERROR;
	}
#endif
#endif

#ifdef CONFIG_RTL8686
		switch(imgHdr.key){
			case APPLICATION_UBOOT:
			case APPLICATION_UIMAGE:
			case APPLICATION_ROOTFS:
				printf("%s-%d, got header::%x\n",__func__,__LINE__,imgHdr.key);
				err = 0;
				break;
			default:
				printf("%s-%d, Unknown header::%x\n",__func__,__LINE__,imgHdr.key);
				err = 1;
				break;
		}
		if(err)
			goto M_ERROR;
#else
	if (imgHdr.key != APPLICATION_IMAGE) {
		printf("Unknown header\n");
		goto M_ERROR;
	}
#endif

	csum = imgHdr.chksum;
	size = imgHdr.length;
	remain = size;

	while (remain > 0) {
		block = (remain > sizeof(buf)) ? sizeof(buf) : remain;
		nRead = fread(buf, 1, block, fp);
		if (nRead <= 0) {
			printf("read too short (remain=%d, block=%d)\n", remain, block);
			goto M_ERROR;
		}
		remain -= nRead;
		csum = ipchksum(buf, nRead,csum);
	}
#if 0
	csum = imgHdr.chksum;
	while (nRead = fread(buf, 1, sizeof(buf), fp)) {
		total += nRead;
		csum = ipchksum(buf, nRead, csum);
	}
#endif

	if (csum) {
		printf("Checksum failed(msgparser cfg_check_image2), size=%d, csum=%04xh\n", total, csum);
		goto M_ERROR;
	}
	qbuf->request = MSG_SUCC;

M_ERROR:
	fclose(fp);
	return;
}

#ifdef CONFIG_DEV_xDSL
#ifdef AUTO_PVC_SEARCH_AUTOHUNT
#define MAX_PVC_SEARCH_PAIRS 16
static void cfg_start_autohunt(struct mymsgbuf *qbuf)
{
	FILE *fp;

	MIB_AUTO_PVC_SEARCH_Tp entryP;
	//MIB_AUTO_PVC_SEARCH_T Entry;
	unsigned int entryNum,i;
	unsigned char tmp[12], tmpBuf[MAX_PVC_SEARCH_PAIRS*12];

	entryNum = _mib_chain_total(MIB_AUTO_PVC_SEARCH_TBL);
	memset(tmpBuf, 0, sizeof(tmpBuf));
	for(i=0;i<entryNum; i++) {
		memset(tmp, 0, 12);
		entryP = (typeof(entryP))_mib_chain_get(MIB_AUTO_PVC_SEARCH_TBL, i);
		if (!entryP)
			continue;
		//if (!_mib_chain_get(MIB_AUTO_PVC_SEARCH_TBL, i, (void *)&Entry))
		//	continue;
		sprintf(tmp,"(%d %d)", entryP->vpi, entryP->vci);
		strcat(tmpBuf, tmp);

	}
	//printf("StartSarAutoPvcSearch: inform SAR %s\n", tmpBuf);


	if (fp=fopen("/proc/AUTO_PVC_SEARCH", "w") )
	{
		fprintf(fp, "1%s\n", tmpBuf);	//write pvc list stored in flash to SAR driver
//		printf("StartSarAutoPvcSearch: Inform SAR driver to start auto-pvc-search\n");

		fclose(fp);
	} else {
		printf("Open /proc/AUTO_PVC_SEARCH failed! Can't start SAR driver doing auto-pvc-search\n");
	}

	qbuf->request = MSG_SUCC;
}
#endif
#endif

#ifndef _USE_RSDK_WRAPPER_
#include "../../../spppd/pppoe.h"
#endif  //_USE_RSDK_WRAPPER_
static void cfg_update_PPPoE_session(struct mymsgbuf *qbuf)
{
	PPPOE_SESSION_INFO *p = (PPPOE_SESSION_INFO *)qbuf->msg.mtext;
	unsigned int totalEntry;
	int i, found=0, selected=-1;
	MIB_CE_PPPOE_SESSION_Tp Entry;

	qbuf->request = MSG_FAIL;

	if (__mib_lock) {
		qbuf->request = MSG_MIB_LOCKED;
		return;
	}

	//update_PPPoE_session(p);
	totalEntry = _mib_chain_total(MIB_PPPOE_SESSION_TBL);
	for (i=0; i<totalEntry; i++) {
		selected = i;
		Entry = (typeof(Entry))_mib_chain_get(MIB_PPPOE_SESSION_TBL, i);
		if (!Entry) {
  			printf("Get chain record error!\n");
			return;
		}

		if (Entry->uifno == p->uifno)
		{
			found ++;
				break;
		}
	}

	if (found != 0) {
		Entry->sessionId = p->session;
		memcpy((unsigned char *)Entry->acMac, (unsigned char *)p->remote.sll_addr, 6);
			_mib_chain_update(MIB_PPPOE_SESSION_TBL, (void *)Entry, selected);
		}
	else {
		MIB_CE_PPPOE_SESSION_T entry;
		memset(&entry, 0, sizeof(entry));
		entry.uifno = p->uifno;
		memcpy((unsigned char *)entry.acMac, (unsigned char *)p->remote.sll_addr, 6);
		entry.sessionId = p->session;
		_mib_chain_add(MIB_PPPOE_SESSION_TBL, (void *)&entry);
	}

	qbuf->request = MSG_SUCC;
}

static void cfg_mib_set_PPPoE(struct mymsgbuf *qbuf)
{
	struct mymsgbuf myqbuf;
	PPPOE_SESSION_INFO *p = (PPPOE_SESSION_INFO *)qbuf->msg.mtext;

	qbuf->request = MSG_FAIL;

	if (__mib_lock) {
		qbuf->request = MSG_MIB_LOCKED;
		return;
	}

	myqbuf.msg.arg1 = CONFIG_MIB_CHAIN;
	cfg_mib_backup(&myqbuf);	// backup current MIB chain into system
	myqbuf.msg.arg1 = CURRENT_SETTING;
	myqbuf.msg.arg2 = CONFIG_MIB_CHAIN;
	cfg_mib_reload(&myqbuf);	//get MIB chain from flash
	myqbuf.msg.arg1 = (int)qbuf->msg.arg1;
	memcpy(myqbuf.msg.mtext, qbuf->msg.mtext, myqbuf.msg.arg1);
	cfg_update_PPPoE_session(&myqbuf);

	myqbuf.msg.arg1 = CURRENT_SETTING;
	myqbuf.msg.arg2 = CONFIG_MIB_CHAIN;
	myqbuf.msg.arg3 = 1;
	cfg_mib_update(&myqbuf);

	myqbuf.msg.arg1 = CONFIG_MIB_CHAIN;
	cfg_mib_restore(&myqbuf);	// restore backup MIB chain
	myqbuf.msg.arg1 = (int)qbuf->msg.arg1;
	memcpy(myqbuf.msg.mtext, qbuf->msg.mtext, myqbuf.msg.arg1);
	cfg_update_PPPoE_session(&myqbuf);
	qbuf->request = MSG_SUCC;
}

#endif

static inline int _kill_by_pidfile(const char *pidfile, int sig)
{
	pid_t pid;
	int time_temp=0;

	pid = read_pid(pidfile);
	if (pid <= 0)
		return pid;
	kill(pid, sig);

	// wait for process termination
	while ((kill(pid, 0) == 0)&&(time_temp < 20)) {
		usleep(100000);  time_temp++;
	}
	if(time_temp >= 20 )
		kill(pid, SIGKILL);
	if(!access(pidfile, F_OK)) unlink(pidfile);
	return pid;
}

static MIB_CE_ATM_VC_T * getATM_VC_ENTRY_byName(char *pIfname, int *entry_index)
{
	unsigned int entryNum, i;
	char ifname[IFNAMSIZ];
	MIB_CE_ATM_VC_T *pEntry=NULL;

	entryNum = _mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		pEntry = (MIB_CE_ATM_VC_T *)_mib_chain_get(MIB_ATM_VC_TBL, i);
		if(!pEntry){
  			printf("Get chain record error!\n");
			return NULL;
		}

		if (pEntry->enable == 0)
			continue;

		ifGetName(pEntry->ifIndex,ifname,sizeof(ifname));

		if(!strcmp(ifname,pIfname)){
			break;
		}
	}

	if(i>= entryNum){
		//printf("not find this interface!\n");
		return NULL;
	}

	*entry_index=i;
	//printf("Found %s in ATM_VC_TBL index %d\n", ifname, *entry_index);
	return pEntry;
}

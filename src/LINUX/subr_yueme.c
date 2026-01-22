#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <stdarg.h>
#include <errno.h>

#include "rtk_timer.h"
#include "mib.h"
#include "utility.h"
#include "subr_net.h"
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
#include <rtk/rt/rt_stat.h>
#include <rtk/rt/rt_switch.h>
#include "fc_api.h"
#endif
#include <common/util/rt_util.h>

#ifdef CONFIG_TR142_MODULE
#include <rtk/rtk_tr142.h>
#endif

#ifdef CONFIG_CT_AWIFI_JITUAN_SMARTWIFI
#include "../../ctcyueme/awifi/awifi.h"
#endif
#if defined(SUPPORT_NON_SESSION_IPOE) && defined(_PRMT_X_CT_COM_DHCP_)
#include <openssl/des.h>
#endif

static int getATM_VC_ENTRY_byName(char *pIfname, MIB_CE_ATM_VC_T *pEntry, int *entry_index)
{
	unsigned int entryNum, i;
	char ifname[IFNAMSIZ];

	if(pIfname == NULL) {
		return -1;
	}

	if(strlen(pIfname) == 0) {
		return -1;
	}

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)pEntry))
		{
			printf("Get chain record error!\n");
			return -1;
		}

		if (pEntry->enable == 0)
			continue;

		ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));

		if(!strcmp(ifname, pIfname)) {
			break;
		}
	}

	if(i >= entryNum) {
		//printf("not find this interface!\n");
		return -1;
	}

	*entry_index = i;
	return 0;
}

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
/*
* Function: get_restore_status
* Description: get RestoreStatus
* Input:
* Return:
* 0: No restore
* 1: restore by long reset button
* 2: restore by short reset button
* 3: restore by web button
* 4: restore by tr069 (itms)
* 5: dbus Restore
*/

int get_restore_status(void)
{
	/*modify by liuxm 2016.8.19*/
#if 0
	FILE *fp;
	int status=0;
	fp = fopen("/tmp/restore_status", "r");
	if(fp){
		fscanf(fp, "%d", &status);
		fclose(fp);
	}
#endif
	unsigned int status=0;
	mib_get_s(MIB_RESTORE_STATUS_TBL, (void *)&status, sizeof(status));
	return status;
}

/*
 * flag
 * 0: for short reset
 * 1: for long reset
 * 2: TR-069 FactoryReset
 * 3: reset button for short reset
 * 4: reset button for long reset
 * 5: dbus Restore
 * 6: dbus LocalRecovery
 * 7: factory default : flash default cs
 */
void update_restore_status(int flag)
{
#if 0
	FILE *fp;
	int status;
	fp = fopen("/tmp/restore_status", "w");
	if(fp){
		switch(flag){
			case 0:
			case 1:
				status = 3;
				break;
			case 2:
				status = 4;
				break;
			case 3:
				status = 2;
				break;
			case 4:
				status = 1;
				break;

		}
		fprintf(fp, "%d", status);
		fclose(fp);
	}
#endif
    unsigned int status = 0;
	switch(flag){
		case 0:
		case 1:
			status = 3; //restore by web button
			break;
		case 2:
			status = 4; //restore by tr069 (itms)
			break;
		case 3:
			status = 2; //restore by short reset button
			break;
		case 4:
			status = 1; //restore by long reset button
			break;
		case 6:
			status = 3; //dbus LocalRecovery
			break;
		case 7:
			status = 3; //restore by web button (factory default, ie flash default cs)
			break;
#ifdef CONFIG_CU_BASEON_YUEME	
		case 8:
			status = 5; // dbus restore
			break;
#endif			
		default:
			status = flag; //dbus
	}
    //status = flag;
    printf("Update RestoreStatus %u\n", status);
    mib_set(MIB_RESTORE_STATUS_TBL, (void *)&status);
    mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
}
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
void collect_debug_info(char *category, char *dir_name, char *fw_dir_name, int first)
{
	char cmd[256] = {0};
	int port_num = 0;
	int phyPortId = -1;
	char temp_phy_port[8] = {0};
	char all_phy_port_id[64] = {0};


	if(!strcmp(category, "INTERNET") || !strcmp(category, "ALL")){
		snprintf(cmd, 256, "cat /proc/fc/sw_dump/fwd_statistic >> /tmp/%s/fc_fwd_statistic", dir_name);
		system(cmd);
		/* get lan port phy id */
		for(port_num = 0; port_num < ELANVIF_NUM; port_num++)
		{
			phyPortId = rtk_port_get_lan_phyID(port_num);
			if (phyPortId == -1)
				printf("collect_debug_info get LAN %d 's phyPortId failed!\n", port_num);
			else
			{
				snprintf(temp_phy_port, sizeof(temp_phy_port), "%d,", phyPortId);
				strncat(all_phy_port_id, temp_phy_port, strlen(temp_phy_port));
			}
		}


		/* get pon port phy id */
		phyPortId = rtk_port_get_wan_phyID();
		if (phyPortId == -1)
			printf("collect_debug_info get LAN %d 's phyPortId failed!\n", port_num);
		else
		{
			snprintf(temp_phy_port, sizeof(temp_phy_port), "%d,", phyPortId);
			strncat(all_phy_port_id, temp_phy_port, strlen(temp_phy_port));
		}
		/* get cpu port phy id */ //no need to do this now
		//phyPortId = rtk_port_get_cpu_phyMask();

		all_phy_port_id[strlen(all_phy_port_id) - 1] = '\0';
		//printf("all_phy_port_id = %s\n", all_phy_port_id);

		snprintf(cmd, 256, "diag mib dump counter port %s nonZero >> /tmp/%s/mib_dump_counter", all_phy_port_id, dir_name);
		system(cmd);
	}

	if(!strcmp(category, WAN_VOIP_VOICE_NAME) || !strcmp(category, "ALL")){
		snprintf(cmd, 256,"cp /var/log/maserati.log /tmp/%s/maserati.log", dir_name);
		system(cmd);
	}

	if(!strcmp(category, "IPTV") || !strcmp(category, "ALL")){
		snprintf(cmd, 256, "cat /proc/igmp/dump/igmpInfo >> /tmp/%s/igmpInfo", dir_name);
		system(cmd);
		snprintf(cmd, 256, "cat /proc/fc/sw_dump/l2 >> /tmp/%s/fc_sw_lut", dir_name);
		system(cmd);
	}

	if(!strcmp(category, "WIFI") || !strcmp(category, "ALL")){
		snprintf(cmd, 256, "cat /proc/wlan0/mib_all >> /tmp/%s/wlan0_mib_all", dir_name);
		system(cmd);
#ifdef WLAN_DUALBAND_CONCURRENT
		snprintf(cmd, 256, "cat /proc/wlan1/mib_all >> /tmp/%s/wlan1_mib_all", dir_name);
		system(cmd);
#endif
	}

	if(!strcmp(category, "SYSTEM") || !strcmp(category, "ALL")){
		if(first){
			stopLog();
			//start slogd
			system("/bin/slogd -n -l 7 -L -w &");
#ifdef USE_BUSYBOX_KLOGD
			system("/bin/klogd -n &");
#endif
		}
		snprintf(cmd, 256, "cp /var/log/messages /tmp/%s/syslog", dir_name);
		system(cmd);
	}

	if(!strcmp(category, "FRAMEWORK") || !strcmp(category, "ALL")){
		//TODO
	}

	return;
}

#endif

void stop_collect_debug_info(char *category)
{
	if(!strcmp(category, "SYSTEM") || !strcmp(category, "ALL")){
		//restore slogd
		stopLog();
		startLog();
	}
}
/*
* Function: start_collection_debug_info
* Description: collection debug info into a zip file and upload to a specific url path
* Input:
* 	uploadURL: format as: http://username:password@host:port/path
* 	timeout: in one minute
* 	category: ALL, INTERNET, WAN_VOIP_VOICE_NAME, IPTV, WIFI, SYSTEM, FRAMEWORK
* Return: 0: success; -1: failed
*/
int start_collection_debug_info(char *uploadURL, unsigned short timeout, char *category)
{
	struct itimerspec new_value;
	struct timespec start;
	struct timespec curr;
	int fd;
	uint64_t exp;
	ssize_t s;
	int first_call = 1;
	int secs, nsecs;
	int max_time = timeout * 60;
	time_t tm;
	struct tm tm_time;
	char strbuf[64];
	char dir_name[128], fw_dir_name[128];
	char command[256];
	unsigned char mac[6];
	char username[64]={0}, password[64]={0};
	char url_path[128]={0};

	new_value.it_value.tv_sec = 0;
	new_value.it_value.tv_nsec = 1;
	new_value.it_interval.tv_sec = 5; //call collect_debug_info every 5 sec
	new_value.it_interval.tv_nsec = 0;

	//use mac address and time as zip file name
	time(&tm);
	memcpy(&tm_time, localtime(&tm), sizeof(tm_time));
	strftime(strbuf, 64, "%y%m%d%H%M%S", &tm_time);
	mib_get_s(MIB_ELAN_MAC_ADDR, (void *)mac, sizeof(mac));

	//mkdir for GW
	if(strcmp(category, "FRAMEWORK")){
		snprintf(dir_name, 128, "RTK_9607_%02x%02x%02x%02x%02x%02x_GW_%s_debuginfo", mac[0], mac[1], mac[2],
																	mac[3], mac[4], mac[5], strbuf);
		snprintf(command, 256, "mkdir -p /tmp/%s", dir_name);
		system(command);
	}
	//mkdir for FW
	if(!strcmp(category, "FRAMEWORK") || !strcmp(category, "ALL")){

		snprintf(fw_dir_name, 128, "RTK_9607_%02x%02x%02x%02x%02x%02x_FW_%s_debuginfo", mac[0], mac[1], mac[2],
																	mac[3], mac[4], mac[5], strbuf);
		snprintf(command, 256, "mkdir -p /tmp/%s", fw_dir_name);
		system(command);
	}

	fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (fd == -1){
		printf("create timer error\n");
		return -1;
	}

	if (timerfd_settime(fd, 0, &new_value, NULL) == -1){
		printf("set timer error\n");
		return -1;
	}

	printf("start to collect debug info\n");

	while( s = read(fd, &exp, sizeof(uint64_t))){
		if (s != sizeof(uint64_t)){
			printf("read timer error\n");
			return -1;
		}

		collect_debug_info(category, dir_name, fw_dir_name, first_call);

		if(first_call){
			first_call = 0;
			if (clock_gettime(CLOCK_MONOTONIC, &start) == -1){
				printf("get time error\n");
				return -1;
			}
		}
		if (clock_gettime(CLOCK_MONOTONIC, &curr) == -1){
		   printf("get time error\n");
		   return -1;
		}

		secs = curr.tv_sec - start.tv_sec;
		nsecs = curr.tv_nsec - start.tv_nsec;

		//time is up
		if(secs >= max_time)
			break;

	}

	if(strcmp(category, "FRAMEWORK")){
		snprintf(command, 256, "cd /tmp; 7za a -tzip -sdel %s.zip %s; cd /", dir_name, dir_name);
		system(command);
	}
	if(!strcmp(category, "FRAMEWORK") || !strcmp(category, "ALL")){
		snprintf(command, 256, "cd /tmp; 7za a -tzip -sdel %s.zip %s; cd /", fw_dir_name, fw_dir_name);
		system(command);
	}

	stop_collect_debug_info(category);

	//upload zip file to the specific url path
	sscanf(uploadURL, "http://%[^:]:%[^@]@%s", username, password, url_path);

	if(strcmp(category, "FRAMEWORK")){
		//snprintf(command, 256, "curl -i -X POST \"Content-Type: multipart/form-data\" -F \"file=@/tmp/%s.zip\" http://192.168.1.7:8000/uploaded", dir_name);
		//snprintf(command, 256, "curl -T /tmp/%s.zip -u %s:%s http://%s", dir_name, username, password, url_path);
		//Because -T use Put Method for HTTP, some HTTP server doens't accept this Method, we use -F to use POST METHOD
		snprintf(command, 256, "curl -F 'file_name=@/tmp/%s.zip' -u %s:%s http://%s", dir_name, username, password, url_path);
		printf("%s\n", command);
		system(command);
	}
	if(!strcmp(category, "FRAMEWORK") || !strcmp(category, "ALL")){
		//snprintf(command, 256, "curl -i -X POST \"Content-Type: multipart/form-data\" -F \"file=@/tmp/%s.zip\" http://192.168.1.7:8000/uploaded", fw_dir_name);
		//snprintf(command, 256, "curl -T /tmp/%s.zip -u %s:%s http://%s", fw_dir_name, username, password, url_path);
		//Because -T use Put Method for HTTP, some HTTP server doens't accept this Method, we use -F to use POST METHOD
		snprintf(command, 256, "curl -F 'file_name=@/tmp/%s.zip' -u %s:%s http://%s", fw_dir_name, username, password, url_path);
		printf("%s\n", command);
		system(command);
	}

	return 0;

}
#ifdef CONFIG_USER_DBUS_PROXY
#define CMD_MAX_LEN (256)
int do_samba_update(char *newpasswd)
{
	unsigned int totalEntry = 0, num1 = 0 ;
	unsigned char samba_enable=0;
	char cmd_buff[CMD_MAX_LEN];
	MIB_CE_SMB_ACCOUNT_T samba_account_t1, samba_account_t;
	FILE *fp_pass;

	memset(&samba_account_t, 0, sizeof(MIB_CE_SMB_ACCOUNT_T));
	strcpy(samba_account_t.username, "useradmin");
	strcpy(samba_account_t.password, newpasswd);

	totalEntry = mib_chain_total(MIB_SMBD_ACCOUNT_TBL);

	for(num1 = 0; num1< totalEntry; num1++)
	{
	    if (mib_chain_get(MIB_SMBD_ACCOUNT_TBL, num1, (void *)&samba_account_t1))
	    {
	        if(!strcmp(samba_account_t1.username,"useradmin"))
	        {
	            break;
	        }
	    }
	}
	if(num1 == totalEntry)
	{
        return -1;
    }
	samba_account_t.index = num1 +1;
	if(mib_chain_update(MIB_SMBD_ACCOUNT_TBL,(void *)&samba_account_t,num1))
	{
		;
	}
	else
	{
		return -1;
	}
	mib_update(CURRENT_SETTING, CONFIG_MIB_CHAIN);

	mib_get_s(MIB_SAMBA_ENABLE, (void *)&samba_enable, sizeof(samba_enable));

	fp_pass = fopen("/tmp/.pass", "w");
	if(fp_pass == NULL )
	{
		return -1;
	}
	fprintf(fp_pass, "%s\n",newpasswd);
	fprintf(fp_pass, "%s\n",newpasswd);
	fclose(fp_pass);

	//sprintf(cmd_buff, "smbpasswd useradmin %s", newpasswd);
	sprintf(cmd_buff, "cat /tmp/.pass | smbpasswd -a useradmin -s");
	//sprintf(cmd_buff, "cat /tmp/.pass | pdbedit -a useradmin -t");
	system(cmd_buff);
	#if 0
	if(samba_enable != 0)
	{
		sprintf(cmd_buff,"killall -SIGKILL smbd");
		system(cmd_buff);
		sprintf(cmd_buff,"/bin/smbd -D");
		system(cmd_buff);
	}
	#endif
	return 0;
}
#endif
int startNetlink(void)
{
	va_cmd("/bin/netlink", 0, 0);
	return 0;
}
const char NAS[] = "/bin/nas";
int startHomeNas(void)
{
    int ret = 0;

    //va_cmd("/bin/nas", 0, 0);
	va_niced_cmd(NAS, 0, 0);

	return ret;
}
int get_ftpUserAccount(char *username, char *password)
{
#ifdef FTP_SERVER_INTERGRATION
	MIB_CE_FTP_ACCOUNT_T ftp_account;
#else
	MIB_CE_VSFTP_ACCOUNT_T ftp_account;
#endif
#ifdef FTP_SERVER_INTERGRATION
	if(mib_chain_get(MIB_FTP_ACCOUNT_TBL, 0, (void *)&ftp_account)==0)
#else
	if(mib_chain_get(MIB_VSFTP_ACCOUNT_TBL, 0, (void *)&ftp_account)==0)
#endif
		return -1;
	if(username)
		strcpy(username, ftp_account.username);
	if(password)
		strcpy(password, ftp_account.password);
	return 0;
}

int set_ftpUserAccount(char *username, char *password)
{
#ifdef FTP_SERVER_INTERGRATION
	MIB_CE_FTP_ACCOUNT_T ftp_account;	
	if(mib_chain_get(MIB_FTP_ACCOUNT_TBL, 0, (void *)&ftp_account)==0)
#else
	MIB_CE_VSFTP_ACCOUNT_T ftp_account;	
	if(mib_chain_get(MIB_VSFTP_ACCOUNT_TBL, 0, (void *)&ftp_account)==0)
#endif
		return -1;
	if(username)
		strcpy(ftp_account.username, username);
	if(password)
		strcpy(ftp_account.password, password);

#ifdef FTP_SERVER_INTERGRATION
	if(mib_chain_update(MIB_VSFTP_ACCOUNT_TBL, (void *)&ftp_account, 0)==0)
#else
	if(mib_chain_update(MIB_VSFTP_ACCOUNT_TBL, (void *)&ftp_account, 0)==0)
#endif
		return -1;

	mib_update(CURRENT_SETTING, CONFIG_MIB_CHAIN);
	return 0;
}
int get_ftp_enable(void)
{
	unsigned char enable=0;
#ifdef FTP_SERVER_INTERGRATION
	mib_get_s(MIB_FTP_ENABLE, &enable, sizeof(enable));
#else
	mib_get_s(MIB_VSFTP_ENABLE, &enable, sizeof(enable));
#endif
	return (enable==3);
}
int set_ftp_enable(int data)
{
	unsigned char enable=data;
#ifdef FTP_SERVER_INTERGRATION
	mib_set(MIB_FTP_ENABLE, &enable);
#else
	mib_set(MIB_VSFTP_ENABLE, &enable);
#endif
	return 1;
}
#endif

/************************************************
* Propose: doPreRestoreFunc
*
* Parameter:
* Return:
*     -1 : fail
*       0 : success
* Author:
*    Iulian
*************************************************/

int doPreRestoreFunc()
{
#ifdef CONFIG_USER_DBUS_PROXY
	FILE *fp = NULL;
	fp = fopen("/tmp/.runRestore", "w");
	if(fp) fclose(fp);
#endif
	system("spppctl down all &");
	
	return 0;
}

#ifdef CONFIG_USER_RTK_OMD
#define REBOOT_LOG	"/opt/upt/apps/omd_log.reboot"
#ifdef CONFIG_CLASSFY_MAGICNUM_SUPPORT
#define WATCHDOG_MAGIC_KEY 0xF7F6F5F4
#endif
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

/***********************************************
  * Note:
  *		no this file    ---create and write flag in file
  *		have this file ---read and write
  *
  **********************************************/
#define OOPS_FILE "/sys/fs/pstore/dmesg-ramoops-0"
//#define OOPS_FILE "/opt/upt/apps/oops"
int write_omd_reboot_log(unsigned int flag)
{
	FILE *fp;
	int fd, hasExcep = 0;
	unsigned int reboot_flag=0;
	time_t rtime=0;

	if (flag != REBOOT_FLAG)
	{ 
		//if(access(REBOOT_LOG, F_OK) != 0)
		{
			fp = fopen(REBOOT_LOG, "w+");
			
			if(fp)
			{
				time(&rtime);
				fprintf(fp, "%d %lu", flag, rtime);

				printf("[%s %d]: no file,flag = %x , time = %lu\n", __func__, __LINE__, flag, rtime);
				fd = fileno(fp);
				fsync(fd);

				fclose(fp);
			}
		}
	}
	else 
	{	
		if(is_watchdog_reboot()) hasExcep |= 1;
		if(access(OOPS_FILE,  F_OK) == 0) {
			struct stat file_att;
			if(stat(OOPS_FILE, &file_att) == 0)
				rtime = file_att.st_ctime;
			hasExcep |= 1;
		}
		
		if((hasExcep || access(REBOOT_LOG, F_OK) != 0) && (fp = fopen(REBOOT_LOG, "w+")) != NULL)
		{
			if(hasExcep){
				flag |= EXCEP_REBOOT;
			}
			else flag |= POWER_REBOOT;
			fprintf(fp, "%d %lu", flag, rtime);
			printf("[%s %d]: flag = %x, time = %lu\n", __func__, __LINE__, flag, rtime);
			fd = fileno(fp);
			fsync(fd);
			fclose(fp);
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
			}
		}
	}
	
	return 1;
}
#endif

#ifdef SUPPORT_INCOMING_FILTER
static int if_a_string_is_a_valid_address_s(char *ip_str)
{
    struct in_addr ipv4_addr;
    struct in6_addr ipv6_addr;
    int ret, ret1, ret2;

    ret1 = inet_pton(AF_INET, ip_str, &ipv4_addr);

    if(ret1 > 0)
    {
         ret = IN_COMING_IS_IPV4;
    }
    else
    {
        ret2 = inet_pton(AF_INET6, ip_str, &ipv6_addr);

        if(ret2 > 0)
        {
            ret = IN_COMING_IS_IPV6;
        }
        else
        {
            ret = -1;
        }
    }

    return ret;
}
#endif

#ifdef SUPPORT_INCOMING_FILTER
//const char FW_IN_COMMING[] = "in_comming_filter";
/******************************************************************
* INPUT
*        cmd --> add  IN_COMING_API_ADD
*            --> del  IN_COMING_API_DEL
*        in_coming_val --> struct smart_func_in_coming_val
*        errdesc --> output error
* RETURN : success 0
*          fail -1
********************************************************************/
int smart_func_add_in_coming_api(int cmd, smart_func_in_coming_val *in_coming_val, char *errdesc)
{
	char cmd_buf[IN_COMING_CMD_BUF_MAX_LEN] = {0};
	char act_buf[4] = {0};
	char iptables_buf[16] = {0};
	char remoteIpBuf[68] = {0};
	const char lanIf[] = "br0";
	const char wanIF1[] = "nas+";
	const char wanIF2[] = "ppp+";

	printf("%s:\n", (cmd == IN_COMING_API_ADD) ? "Add":"Del");
	printf("in_coming_val.remoteIP:[%s]\n", in_coming_val->remoteIP);
	printf("in_coming_val.protocol:[%d]\n", in_coming_val->protocol);
	printf("in_coming_val.interface:[%d]\n", in_coming_val->interface);
	printf("in_coming_val.port:[%d]\n", in_coming_val->port);

	if(cmd == IN_COMING_API_ADD)
	{
		sprintf(act_buf, "-A");
	}
	else if(cmd == IN_COMING_API_DEL)
	{
		sprintf(act_buf, "-D");
	}
	else
	{
		sprintf(errdesc, "CMD ERROR.");
		return IN_COMING_RET_FAIL;
	}

	if(strlen(in_coming_val->remoteIP))
	{
		if(if_a_string_is_a_valid_address_s(in_coming_val->remoteIP) == IN_COMING_IS_IPV4)
		{
			sprintf(iptables_buf, "iptables");
		}
		else if(if_a_string_is_a_valid_address_s(in_coming_val->remoteIP) == IN_COMING_IS_IPV6)
		{
			sprintf(iptables_buf, "ip6tables");
		}
		else
		{
			sprintf(errdesc, "remote ip invaild.");
			return IN_COMING_RET_FAIL;
		}
		sprintf(remoteIpBuf, " -d %s ", in_coming_val->remoteIP);
	}
	else
	{
		sprintf(iptables_buf, "iptables");
		sprintf(remoteIpBuf, " ");
	}

	if(in_coming_val->interface == IN_COMING_INTERFACE_LAN_E)
	{
		if(in_coming_val->protocol == IN_COMING_PROTO_TCP_E)
		{
			sprintf(cmd_buf, "%s %s %s -i %s -p TCP%s--dport %d -j ACCEPT", iptables_buf, act_buf, FW_IN_COMMING, lanIf, remoteIpBuf, in_coming_val->port);
			system(cmd_buf);
		}
		else if(in_coming_val->protocol == IN_COMING_PROTO_UDP_E)
		{
			sprintf(cmd_buf, "%s %s %s -i %s -p UDP%s--dport %d -j ACCEPT", iptables_buf, act_buf, FW_IN_COMMING, lanIf, remoteIpBuf, in_coming_val->port);
			system(cmd_buf);
		}
		else if(in_coming_val->protocol == IN_COMING_PROTO_TCP_AND_UDP_E)
		{
			sprintf(cmd_buf, "%s %s %s -i %s -p TCP%s--dport %d -j ACCEPT", iptables_buf, act_buf, FW_IN_COMMING, lanIf, remoteIpBuf, in_coming_val->port);
			system(cmd_buf);
			sprintf(cmd_buf, "%s %s %s -i %s -p UDP%s--dport %d -j ACCEPT", iptables_buf, act_buf, FW_IN_COMMING, lanIf, remoteIpBuf, in_coming_val->port);
			system(cmd_buf);
		}
		else
		{
			sprintf(errdesc, "PROTO ERROR.");
			return IN_COMING_RET_FAIL;
		}
	}
	else if(in_coming_val->interface == IN_COMING_INTERFACE_WAN_E)
	{
		if(in_coming_val->protocol == IN_COMING_PROTO_TCP_E)
		{
			sprintf(cmd_buf, "%s %s %s -i %s -p TCP%s--dport %d -j ACCEPT", iptables_buf, act_buf, FW_IN_COMMING, wanIF1, remoteIpBuf, in_coming_val->port);
			system(cmd_buf);
			sprintf(cmd_buf, "%s %s %s -i %s -p TCP%s--dport %d -j ACCEPT", iptables_buf, act_buf, FW_IN_COMMING, wanIF2, remoteIpBuf, in_coming_val->port);
			system(cmd_buf);
		}
		else if(in_coming_val->protocol == IN_COMING_PROTO_UDP_E)
		{
			sprintf(cmd_buf, "%s %s %s -i %s -p UDP%s--dport %d -j ACCEPT", iptables_buf, act_buf, FW_IN_COMMING, wanIF1, remoteIpBuf, in_coming_val->port);
			system(cmd_buf);
			sprintf(cmd_buf, "%s %s %s -i %s -p UDP%s--dport %d -j ACCEPT", iptables_buf, act_buf, FW_IN_COMMING, wanIF2, remoteIpBuf, in_coming_val->port);
			system(cmd_buf);
		}
		else if(in_coming_val->protocol == IN_COMING_PROTO_TCP_AND_UDP_E)
		{
			sprintf(cmd_buf, "%s %s %s -i %s -p TCP%s--dport %d -j ACCEPT", iptables_buf, act_buf, FW_IN_COMMING, wanIF1, remoteIpBuf, in_coming_val->port);
			system(cmd_buf);
			sprintf(cmd_buf, "%s %s %s -i %s -p UDP%s--dport %d -j ACCEPT", iptables_buf, act_buf, FW_IN_COMMING, wanIF1, remoteIpBuf, in_coming_val->port);
			system(cmd_buf);
			sprintf(cmd_buf, "%s %s %s -i %s -p TCP%s--dport %d -j ACCEPT", iptables_buf, act_buf, FW_IN_COMMING, wanIF2, remoteIpBuf, in_coming_val->port);
			system(cmd_buf);
			sprintf(cmd_buf, "%s %s %s -i %s -p UDP%s--dport %d -j ACCEPT", iptables_buf, act_buf, FW_IN_COMMING, wanIF2, remoteIpBuf, in_coming_val->port);
			system(cmd_buf);
		}
		else
		{
			sprintf(errdesc, "PROTO ERROR.");
			return IN_COMING_RET_FAIL;
		}
	}
	else
	{
		sprintf(errdesc, "Interface ERROR.");
		return IN_COMING_RET_FAIL;
	}

	return IN_COMING_RET_SUCCESSFUL;
}
#endif

#ifdef CONFIG_USER_DBUS_PROXY
#include <rtk/smart_proxy_common.h>
/****************************************************************************/
/**james : notify api for mib 2 dbusproxy**/
/**msg struct the same as smart defination**/

#ifndef SDK_2_DBUSPROXY_SOCK
    #define SDK_2_DBUSPROXY_SOCK   "/tmp/sdk2dbusproxy"
#endif

int mib_notify_table_list_handle_func(int table_id, e_table_list_action_T action);

int dbus_proxy_uds_client_conn(const char *servername)
{
    int sock_client_fd = -1;
	fd_set fdset;
    struct timeval tv;
    int len, rval, so_error;

    struct sockaddr_un un;
	sock_client_fd = socket( AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0 ) ;
	if(sock_client_fd < 0)
	{
		fprintf(stderr,"mib client socket create error fail !\n");
		perror ("socket");
		return -1;
	}
	fcntl(sock_client_fd, F_SETFL, O_NONBLOCK);

	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;

	strcpy(un.sun_path, servername);

	rval = connect(sock_client_fd, (struct sockaddr *)&un, sizeof(struct sockaddr_un));
	if(rval == 0) return sock_client_fd;
	else if(rval < 0 && errno == EINPROGRESS)
	{
		tv.tv_sec = 3;  
		tv.tv_usec = 0;
retry:
		FD_ZERO(&fdset);
		FD_SET(sock_client_fd, &fdset);
		
		rval = select(sock_client_fd + 1, NULL, &fdset, NULL, &tv);
		if(rval >= 1)
		{
			len = sizeof(so_error);
			getsockopt(sock_client_fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
			if (so_error == 0) {
				return sock_client_fd;
			}
		}
		else if(rval < 0 && errno == EINTR)
		{
			if(tv.tv_sec > 0 || tv.tv_usec > 0) goto retry;
		}
	}
	close(sock_client_fd);
	
	printf("[MIB Client]: Error connect to unix_socket(%s).\n", __func__, __LINE__, servername);
	return -1;
}


int dbus_proxy_uds_send_client_msg(int sockfd,char *buf,int size)
{
    int sendbytes = 0;

	if((sendbytes = send(sockfd, buf, size, 0)) < 0)
	{
		fprintf(stderr,"[MIB Client]: Failed to send client,error:%s pid %d \n", strerror(errno), getpid());
		//close(sockfd);
		return -1;
	}

    return 0;

}

int mib_2_dbus_notify_dbus_api(mib2dbus_notify_app_t *notify_app)
{

	//Msg_proxy_t  msg;
	 int socket_fd = dbus_proxy_uds_client_conn(SDK_2_DBUSPROXY_SOCK);
	 int send_ret=0;
	 notify_app->iPid = getpid();
//fprintf(stderr,"\n [DEBUG] mib_2_dbus_notify_dbus_api  \n table_id:%d \niPid :%d \nsignal_id:%#x \nrecordNum :%d\n",
    //notify_app->table_id, notify_app->iPid, notify_app->Signal_id, notify_app->content[0]);
	 if(socket_fd < 0)
	 {
		//fprintf(stderr,"mib_2_dbus_notify_dbus_api::Failed to send client,socket id is error\n" );
		//close(socket_fd);
		return -1;
	 }else{
		 //fprintf(stderr,"mib_2_dbus_notify_dbus_api::to send client,socket id is sizeof(notify_app_t):%d\n",
                //sizeof(mib2dbus_notify_app_t) );
		send_ret = dbus_proxy_uds_send_client_msg(socket_fd,
                        (char *)notify_app,sizeof(mib2dbus_notify_app_t));

        close(socket_fd);
		if(send_ret<0)
		{
			//fprintf(stderr,"mib_2_dbus_notify_dbus_api::Failed to send client,socket id is error\n" );
			return -1;
		}

	}

    return 0;
}

void send_notify_msg_dbusproxy(int id, e_notify_mib_signal_id signal_id, int recordNum)
{
    int sDbusProxyPid = 0;
	
	if (access("/tmp/.runRestore", F_OK) == 0)
	{
		return;
	}
	
	sDbusProxyPid = read_pid(M_dbus_proxy_pid);

    /**do the pid Judgement first to protect table list initialization **/
    if ((sDbusProxyPid > 0) && (getpid() != sDbusProxyPid))
    {
        if (!mib_notify_table_list_handle_func(id, e_table_list_action_retrieve))
        {
            mib2dbus_notify_app_t notifyMsg;
            memset((char*)&notifyMsg, 0, sizeof(notifyMsg));
            notifyMsg.table_id = id;
            notifyMsg.Signal_id = signal_id;
            notifyMsg.content[0] = recordNum;
            if ( mib_2_dbus_notify_dbus_api(&notifyMsg) != 0)
            {
                printf("[MIB][ERR]mib notify to dbusproxy error \r\n tableid:  %d | signal_id %d | num %d \n",
                    id, signal_id, recordNum);
            }
            else
            {
                printf("[MIB][NTY]mib notify to dbusproxy OK \r\n tableid:  %d | signal_id %d | num %d \n",
                    id, signal_id, recordNum);
            }
        }
    }
	else if(sDbusProxyPid <= 0){
		printf("[MIB][ERR] Cannot get DBUS proxy pid\n");
	}
}


int  g_mib_notify_table_list[]=
{
    /*tableid,(mib table)   */

/**----------single object ------------ **/

/**com.ctc.igd1.DeviceInfo**/
    CWMP_USERINFO_RESULT,
    MIB_DEVICE_NAME,

/**com.ctc.igd1.NetworkInfo**/
    MIB_ADSL_LAN_IP,
    MIB_ATM_VC_TBL,
    MIB_IP_QOS_TC_TBL,

/**com.ctc.igd1.WiFiInfo**/
    MIB_MBSSIB_TBL,
    MIB_WLAN1_MBSSIB_TBL,
#if 0
#ifdef _PRMT_X_CT_COM_WLAN_
    MIB_WLAN_AUTO_CHAN_ENABLED,
#endif
    MIB_WLAN_CHAN_NUM,
    MIB_TX_POWER,
#endif
#ifdef WLAN_CTC_MULTI_AP
	MIB_MAP_CONTROLLER,
#endif
    MIB_WLAN_RATE_PRIOR,
    MIB_WLAN1_RATE_PRIOR,
    MIB_WIFI_MODULE_DISABLED,
	MIB_WLAN_MODULE_DISABLED,
#if defined(WLAN_DUALBAND_CONCURRENT)
	MIB_WLAN1_MODULE_DISABLED,
#endif
#if defined(WLAN_BAND_STEERING)
    MIB_WIFI_STA_CONTROL,
#endif

/**com.ctc.igd1.LANHostManager**/
    MIB_LANHOST_BANDWIDTH_MONITOR_ENABLE,
    #ifdef CONFIG_USER_LANNETINFO
    MIB_MAX_LAN_HOST_NUM,
    MIB_MAX_CONTROL_LIST_NUM,
/**com.ctc.igd1.LANHost**/
    MIB_LAN_DEV_NAME_TBL,
    MIB_LAN_HOST_ACCESS_RIGHT_TBL,
    #endif
#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL
    MIB_LAN_HOST_BANDWIDTH_TBL,
#endif
#if 0
/**com.ctc.igd1.Redirect404**/
    MIB_404_REDIRECT_URL,
#endif
/**com.ctc.igd1.WLANConfiguration**/
   // MIB_WLAN_DISABLED,
   MIB_WLAN_AUTO_CHAN_ENABLED,
   MIB_WLAN_CHAN_NUM,
   MIB_WLAN1_AUTO_CHAN_ENABLED,
   MIB_WLAN1_CHAN_NUM,
   MIB_TX_POWER,
   MIB_WLAN_TX_POWER_HIGH,
   MIB_WLAN1_TX_POWER,
   MIB_WLAN1_TX_POWER_HIGH,
#if !defined(WLAN_BAND_CONFIG_MBSSID)
   MIB_WLAN_BAND,
   MIB_WLAN1_BAND,
#endif
/**com.ctc.igd1.Wifitimer**/
#ifdef WIFI_TIMER_SCHEDULE
    MIB_WIFI_TIMER_TBL,
#endif
/**com.ctc.igd1.Wifitimer**/
    MIB_WIFI_TIMER_EX_TBL,

/**com.ctc.igd1.LED**/
#ifdef LED_TIMER
    MIB_LED_INDICATOR_TIMER_TBL,
#endif
    MIB_LED_STATUS,

/**com.ctc.igd1.DHCPServer**/
#ifdef CONFIG_USER_DHCP_SERVER
    MIB_DHCP_MODE,
#endif
    MIB_DHCP_POOL_START,
    MIB_DHCP_POOL_END,
    MIB_ADSL_LAN_DHCP_LEASE,
    MIB_ADSL_LAN_SUBNET,
#if 0
/**com.ctc.igd1.UplinkQoS**/
    CTQOS_MODE,
    MIB_QOS_ENABLE_QOS,
    MIB_TOTAL_BANDWIDTH_LIMIT_EN,
    MIB_TOTAL_BANDWIDTH,
    MIB_QOS_POLICY,
    MIB_QOS_ENABLE_FORCE_WEIGHT,
    MIB_QOS_ENABLE_DSCP_MARK,
    MIB_QOS_ENABLE_1P,

/**com.ctc.igd1.UplinkQoSAPP**/
    MIB_IP_QOS_APP_TBL,

 /**com.ctc.igd1.UplinkQoSCls**/
    MIB_IP_QOS_TBL,

/**com.ctc.igd1.UplinkQoSCls**/
	MIB_IP_QOS_QUEUE_TBL,
#endif
	MIB_IP_QOS_TBL,
	MIB_IP_QOS_QUEUE_TBL,
	MIB_IP_QOS_APP_TBL,
/**com.ctc.igd1.HTTPServer**/
  //  MIB_HTTP_ENABLE,
#ifdef CONFIG_CU
    MIB_USER_PASSWORD,
#else
    MIB_SUSER_PASSWORD,
#endif

/**com.ctc.igd1.SambaServer**/
    MIB_SAMBA_ENABLE,
    MIB_SMB_ANNONYMOUS,

/**com.ctc.igd1.FTPServer**/
#ifdef FTP_SERVER_INTERGRATION
    MIB_FTP_ENABLE,
    MIB_FTP_ANNONYMOUS,
#else
    MIB_VSFTP_ENABLE,
    MIB_VSFTP_ANNONYMOUS,
#endif
    MIB_SMBD_ACCOUNT_TBL,
#if 0
/**com.ctc.igd1.DNSServerConfig**/
    MIB_DNS_SERVER1,
    MIB_DNS_SERVER2,
#endif
/**----------object with multi instnum------------ **/
    MIB_SLEEP_MODE_SCHED_TBL,
#if 0
/**----------com.ctc.igd1.L2TPConnection------------ **/
    MIB_L2TP_TBL,
    MIB_L2TP_ROUTE_TBL,
#endif
/**----------com.ctc.igd1.DDNSServerConfig------------ **/
    MIB_DDNS_TBL,
	MIB_VIRTUAL_SVR_TBL,
/**----------loglevel control------------ **/
	MIB_DBUS_PROXY_DEBUG,
	MIB_RESTORE_STATUS_TBL,
/**----------PlatformService------------ **/
    MIB_CWMP_MGT_APPMODEL,
    MIB_CWMP_MGT_URL,
    MIB_CWMP_MGT_PORT,
    MIB_CWMP_MGT_ABILITY,
    MIB_CWMP_MGT_LOCATEPORT,
    MIB_PLATFORM_BSSADDR_TBL,
#ifdef CTC_DNS_SPEED_LIMIT
/**----------com.ctc.igd1.DNSSpeedLimit------------ **/
    DNS_LIMIT_DEV_INFO_TBL,
#endif
/**----------com.ctc.igd1.URLFilter------------ **/
#ifdef SUPPORT_URL_FILTER
    MIB_URL_FILTER_TBL,
#endif    
/**----------com.ctc.igd1.DNSFilter------------ **/
#ifdef SUPPORT_DNS_FILTER
    MIB_DNS_FILTER_TBL,
#endif 
/**----------com.ctc.igd1.OMDiag------------ **/
#ifdef CONFIG_USER_RTK_OMD
    MIB_OMDIAG_FLASHTHRESHOLD,
    MIB_OMDIAG_PONTEMPTHRESHOLD,
    MIB_OMDIAG_CPUTEMPTHRESHOLD,
    MIB_OMDIAG_NFCONNTRACKNUM,
#endif 
	MIB_MAC_FILTER_ROUTER_TBL,
/**----------com.ctc.igd1.RouterAdvertisement------- **/
	MIB_V6_RADVD_ENABLE,
	MIB_V6_MAXRTRADVINTERVAL,
	MIB_V6_MINRTRADVINTERVAL,
	MIB_V6_MANAGEDFLAG,
	MIB_V6_OTHERCONFIGFLAG,
/**----------com.ctc.igd1.IPv6LANConfig------- **/
	MIB_IPV6_LAN_LLA_IP_ADDR,
	MIB_LAN_DNSV6_MODE,
	MIB_DNSINFO_WANCONN,
	MIB_ADSL_WAN_DNSV61,
	MIB_ADSL_WAN_DNSV62,
	MIB_PREFIXINFO_PREFIX_MODE,
	MIB_PREFIXINFO_DELEGATED_WANCONN,
	MIB_IPV6_LAN_PREFIX,
	MIB_V6_PREFERREDLIFETIME,
	MIB_V6_VALIDLIFETIME,
/**----------com.ctc.igd1.DHCPv6Server------- **/
	MIB_DHCPV6_MODE,
	MIB_DHCPV6S_MIN_ADDRESS,
	MIB_DHCPV6S_MAX_ADDRESS,
	MIB_DHCPV6S_PREFERRED_LIFETIME,
	MIB_DHCPV6S_DEFAULT_LEASE,
	CWMP_CT_DHCP6S_CHECK_OPT_16,
	CWMP_CT_DHCP6S_SEND_OPT_17,
/**----------com.ctc.igd1.CLOUDVR------- **/
	MIB_CLOUDVR_ENABLE,
/**----------com.cuc.igd1.EthernetConfig------- **/
	MIB_PORT_BINDING_TBL,
/**com.cuc.igd1.transfer.L2TPVPN**/
	MIB_L2TP_TBL,
	MIB_L2TP_ROUTE_TBL,
#if defined(YUEME_4_0_SPEC)
/**com.ctc.igd1.HardAccCancel**/
	MIB_HWACC_CANCEL_ENABLE,
	MIB_HWACC_CANCEL_MAXNUM,
	MIB_HWACC_CANCEL_TBL,
#endif
/**----------the end of list------------ **/
    0,
};


int mib_notify_table_list_handle_func(int table_id, e_table_list_action_T action)
{
//     static int mib_notify_table_list[128];
     int iMiblistNum = sizeof(g_mib_notify_table_list) / sizeof(int);
     int iloop = 0;

#if 0
     if (action == e_table_list_action_init)
     {
         {
             mib_notify_table_list[iMiblistNum] = table_id;
             iMiblistNum++;
         }

         if (iMiblistNum == 128)
         {
            printf("mib notify table list init error, overload num %d | table_id %d\r\n",
                iMiblistNum, table_id);

            smutex = 0;
            return -1;
         }

         if (table_id == 0)
         {
             /**if the id is 0 while initialization, it is the last one**/
             printf("invalid table id 0, total : %d \r\n", iMiblistNum);
             smutex = 0;
             return 0;
         }

         /**insert only one table id to the list per operation**/
         smutex = 0;
         return 0;
     }
     else
#endif
     if (action == e_table_list_action_retrieve)
     {
         for (iloop = 0; iloop < iMiblistNum; iloop ++)
         {
             if (table_id == g_mib_notify_table_list[iloop])
             {
                 /**bingo**/
                 return 0;
             }

             if (g_mib_notify_table_list[iloop] == 0)
             {
                 /**error record**/
                 break;
                 //printf("[MIB][NTY] WARNING, invalid table_id to dbus notification : %d \r\n", table_id);
             }
         }

         /**not found**/
         return -1;
     }
     else
     {
          printf("[MIB][NTY] WARNING, error action : %d \r\n", action);
          return -1;
     }

}

int nortify_wan_update_2_dbus_proxy(const char *ifname, int msg_type, int ifa_family)
{
	mib2dbus_notify_app_t notify_app;
	memset(&notify_app, 0 ,sizeof(mib2dbus_notify_app_t));
	memcpy((char *)notify_app.content, ifname, strlen(ifname));
	notify_app.table_id = COM_CTC_IGD1_WAN_CONNECTION_DB_TAB_T;
	if(ifa_family == AF_INET6) { 
		notify_app.Signal_id = e_dbus_proxy_signal_wan_ipv6_status_changed_id;
	} 
	else {
		notify_app.Signal_id = e_dbus_proxy_signal_wan_ipv4_status_changed_id;
	}
	return mib_2_dbus_notify_dbus_api(&notify_app);
}
#endif

#if defined(CONFIG_CT_AWIFI_JITUAN_SMARTWIFI)
static int get_internet_wan_name(char *name, unsigned int len)
{
	int i,vcTotal;
	char ifname[IFNAMSIZ];
	MIB_CE_ATM_VC_T Entry;

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;
		if (Entry.enable == 0)
			continue;
		if((Entry.applicationtype & X_CT_SRV_INTERNET) && (Entry.cmode != CHANNEL_MODE_BRIDGE))
		{
			if(ifGetName(Entry.ifIndex, ifname, len))
			{
				int flags_found,flags;
				struct in_addr inAddr;
				flags_found = getInFlags(ifname, &flags);
				if (flags_found && getInAddr(ifname, IP_ADDR, &inAddr) == 1){
					strcpy(name,ifname);
					return 1;
				}
			}
		}
	}
	return 0;
}
struct callout_s wifiAuth_ch;
void wifiAuthCheck(void * null)
{
	int ret=0,retDog=0,retAudit=0,retWan,restart=0;
	char cmdWanName[WA_MAX_WAN_NAME]={0};
	char curWanName[WA_MAX_WAN_NAME]={0};
	static char internetWanName[WA_MAX_WAN_NAME]={0};
	char wifidogenable=0;
	int retQzt=0;

	unsigned char awifi_audit_mode=0;

	static int lanauthrule=0;

	if(!lanauthrule){
		awifiAddFwRuleChain();
		lanauthrule=1;
	}
	if((access(WIFIDOGPATH,F_OK))==-1) 	
	{
		printf(WIFIDOGPATH" not exists.\n");
		system("cp /bin/smartwifi "WIFIDOGPATH);
	}

	if((access(WIFIDOGBAKCONFPATH,F_OK))==-1) 	
	{
		printf(WIFIDOGBAKCONFPATH" not exists.\n");
		system("cp /etc/awifi.conf "WIFIDOGBAKCONFPATH);
	}
	
	if((access(WIFIDOGCONFPATH,F_OK))==-1) 	
	{
		printf(WIFIDOGCONFPATH" not exists.\n");
		system("cp /var/config/awifi/awifi_bak.conf "WIFIDOGCONFPATH);
	}

	if((access("/var/config/awifi/smctl",F_OK))==-1) 	
	{
		printf("/var/config/awifi/smctl not exists.\n");
		system("cp /bin/smctl /var/config/awifi/smctl");
	}

	if((access("/var/config/awifi/awifi.html",F_OK))==-1) 	
	{
		printf("/var/config/awifi/awifi.html not exists.\n");
		system("cp /etc/awifi.html /var/config/awifi/awifi.html");
	}

	if((access("/var/config/awifi/libhttpd.so",F_OK))==-1) 	
	{
		printf("/var/config/awifi/libhttpd.so not exists.\n");
		system("cp /etc/libhttpd.so /var/config/awifi/libhttpd.so");
		system("ln -s /var/config/awifi/libhttpd.so /var/config/awifi/libhttpd.so.0");
	}

	if((access("/var/config/awifi/libawifi.so",F_OK))==-1) 	
	{
		printf("/var/config/awifi/libawifi.so not exists.\n");
		system("cp /etc/libawifi.so /var/config/awifi/libawifi.so");
	}

	wifidogenable=1;

	{
		FILE *fp;
		char buf[64];
		unsigned int killflag=0;
		unsigned int wlanrestartflag=0;
		unsigned int resetdhcpflag=0;

		fp=fopen("/tmp/wlanrestart","r");
		if(fp){
			if(fgets(buf,64,fp)){
				sscanf(buf,"restart:%u\n",&wlanrestartflag);
			}
			if(wlanrestartflag){
				printf("get restart %d\n",wlanrestartflag);
			}
			fclose(fp);
			if(wlanrestartflag){
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS) // WPS
				update_wps_configured(0);
#endif
				config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
			}	
			unlink("/tmp/wlanrestart");
			goto AUTH_END;
		}

		fp=fopen("/tmp/killsmartwifi","r");
		if(fp){
			if(fgets(buf,64,fp)){
				sscanf(buf,"killflag:%u\n",&killflag);
			}
			if(killflag){
				printf("get killflag %d\n",killflag);
			}
			fclose(fp);
			if(killflag){
				wifidogenable=0;
			}
			unlink("/tmp/killsmartwifi");
		}

		fp=fopen("/tmp/resetdhcp","r");
		if(fp){
			if(fgets(buf,64,fp)){
				sscanf(buf,"resetdhcp:%u\n",&resetdhcpflag);
			}
			if(resetdhcpflag){
				printf("get resetdhcp %d\n",resetdhcpflag);
			}
			fclose(fp);
			if(resetdhcpflag){
				unsigned int i,numpool;
				unsigned char sourceinterface;
				unsigned char lan1port,lan2port,lan3port,lan4port,ssid1port;
				int changeflag=0;

				mib_get_s(AWIFI_LAN1_AUTH_ENABLE,&lan1port, sizeof(lan1port));
				mib_get_s(AWIFI_LAN2_AUTH_ENABLE,&lan2port, sizeof(lan2port));
				mib_get_s(AWIFI_LAN3_AUTH_ENABLE,&lan3port, sizeof(lan3port));
				mib_get_s(AWIFI_LAN4_AUTH_ENABLE,&lan4port, sizeof(lan4port));
				mib_get_s(AWIFI_WLAN1_AUTH_ENABLE,&ssid1port, sizeof(ssid1port));
				
				sourceinterface=0x20;
				if(lan1port)
					sourceinterface|=0x01;
				if(lan2port)
					sourceinterface|=0x02;
				if(lan3port)
					sourceinterface|=0x04;
				if(lan4port)
					sourceinterface|=0x08;
				if(ssid1port)
					sourceinterface|=0x10;
				numpool = mib_chain_total( MIB_DHCPS_SERVING_POOL_AWIFI_TBL );
				for( i=0; i<numpool;i++ )
				{
					unsigned int j,num;
					DHCPS_SERVING_POOL_T entrypool;

					if( !mib_chain_get( MIB_DHCPS_SERVING_POOL_AWIFI_TBL, i, (void*)&entrypool ) )
						continue;

					//skip disable or relay pools
					if( entrypool.enable==0)
						continue;

					if(strcmp(entrypool.poolname,"awifi"))
						continue;

					printf("find pool!\n");

					if(entrypool.sourceinterface != sourceinterface)
						changeflag = 1;
					entrypool.sourceinterface = sourceinterface;

					printf("change awifi pool interface to 0x%x\n",entrypool.sourceinterface);

					mib_chain_update(MIB_DHCPS_SERVING_POOL_AWIFI_TBL,(void*)&entrypool,i);
					break;
				}
				if(changeflag){
					restart_dhcp();
#if defined(CONFIG_RTK_L34_ENABLE)
                    RTK_RG_Wifidog_Rule_set();
#endif
// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
					Commit();
#endif // of #if COMMIT_IMMEDIATELY
				}
			}
			unlink("/tmp/resetdhcp");
		}
	}
#if 1
	FILE *fp;
	size_t len=0;
	ssize_t read;
	char *line=NULL;
	int line_cnt=0;
	fp=popen("ps","r");
	static unsigned int sourceinterface_unmask_pre = 0;
	unsigned int sourceinterface_unmask = 0;
	while((read = getline(&line,&len,fp))!=-1)
	{
		line_cnt++;
		//printf("%s\n",line);
		if(!retDog && strstr(line, WIFIDOGPATH))
		{
			retDog=1;
		}
		if(retDog 
		)
		{
			break;
		}
	}
	if(line)
		free(line);
	pclose(fp);
	if(line_cnt == 0)
	{
		printf("%s:%d: nothing can be read from ps !\n",__func__,__LINE__);
		goto AUTH_END;
	}
#else
	retDog=find_pid_by_name("wifidog",NULL,0);
	//printf("@@---%s#%d---retDog=%d--retAudit=%d--@@\n",__FUNCTION__,__LINE__,retDog,retAudit);
#endif
	//printf("@@---%s#%d---retDog=%d--wifidogenable=%d--@@\n",__FUNCTION__,__LINE__,retDog,wifidogenable);
	if(!wifidogenable)
	{
		if(retDog)
		{
			system(KILLWIFIDOGSTR);
		}
		goto AUTH_END;
	}
	retWan=get_internet_wan_name(curWanName,sizeof(curWanName));
	if(!retDog)
	{//wifidog hasn't started
		if(retWan)
		{
            void *null;
			printf("%s:%d:start wifi dog!\n",__func__,__LINE__);
			startWiFiDog(null);
			restart=1;
		}
		//no internet wan keep stop
	}else
	{//started
		if(retWan)
		{
			if(strncmp(curWanName,internetWanName,WA_MAX_WAN_NAME))
			{//internet wan changed
				printf("%s:%d:restart wifi dog!\n",__func__,__LINE__);
				restartWiFiDog(0);
				restart=1;
			}
		}else
		{//no internet wan
			if(retDog)
			{
				printf("%s:%d:kill wifi dog!\n",__func__,__LINE__);
				system(KILLWIFIDOGSTR);
			}
		}
	}
	if(retWan)
		strncpy(internetWanName,curWanName,WA_MAX_WAN_NAME);
	else
		internetWanName[0]=0;
	if(!restart && g_wan_modify && retWan)
	{
		printf("%s:%d:start wifi auth!\n",__func__,__LINE__);
		restartWiFiDog(0);
		g_wan_modify=0;
	}
	sourceinterface_unmask = RTK_WAN_GET_UNAvailable_WifiDogInterface();
	if(sourceinterface_unmask_pre != sourceinterface_unmask)
	{	
		sourceinterface_unmask_pre = sourceinterface_unmask;
	}

AUTH_END:
	TIMEOUT_F(wifiAuthCheck, 0, 5, wifiAuth_ch); 
}
#endif

#if defined(CONFIG_CT_AWIFI_JITUAN_SMARTWIFI)
struct callout_s wifidog_timeout;
#ifdef CONFIG_CT_AWIFI_JITUAN_SMARTWIFI
int awifi_lan_enable()
{
	int idx=0;
	char cmd[256];
	unsigned char enable;

	printf("enter awifi_lan_enable!\n");
	//first add all lan rule
	{
		//system("iptables -t mangle -N LanAuth_Exclude");
		//system("iptables -t mangle -A PREROUTING -i br0 -j LanAuth_Exclude");
		for(idx=AWIFI_LAN_START_NUM;idx<AWIFI_LAN_END_NUM;idx++)
		{
			snprintf(cmd,sizeof(cmd),"iptables -t mangle -D LanAuth_Exclude -m physdev --physdev-in eth0.%d -j RETURN",idx);
			//printf("@@---%s#%d---cmd=%s----@@\n",__FUNCTION__,__LINE__,cmd);
			system(cmd);
		}
		snprintf(cmd,sizeof(cmd),"iptables -t mangle -D LanAuth_Exclude -m physdev --physdev-in wlan0 -j RETURN");
		//printf("@@---%s#%d---cmd=%s----@@\n",__FUNCTION__,__LINE__,cmd);
		system(cmd);
	}

	{
		for(idx=AWIFI_LAN_START_NUM;idx<AWIFI_LAN_END_NUM;idx++)
		{
			if(idx==AWIFI_LAN_START_NUM){
				mib_get_s(AWIFI_LAN1_AUTH_ENABLE,&enable, sizeof(enable));
				if(!enable)
					continue;
			}
			if(idx==(AWIFI_LAN_START_NUM+1)){
				mib_get_s(AWIFI_LAN2_AUTH_ENABLE,&enable, sizeof(enable));
				if(!enable)
					continue;
			}
			if(idx==(AWIFI_LAN_START_NUM+2)){
				mib_get_s(AWIFI_LAN3_AUTH_ENABLE,&enable, sizeof(enable));
				if(!enable)
					continue;
			}
			if(idx==(AWIFI_LAN_START_NUM+3)){
				mib_get_s(AWIFI_LAN4_AUTH_ENABLE,&enable, sizeof(enable));
				if(!enable)
					continue;
			}
			snprintf(cmd,sizeof(cmd),"iptables -t mangle -I LanAuth_Exclude 1  -m physdev --physdev-in eth0.%d -j RETURN",idx);
			//printf("@@---%s#%d---cmd=%s----@@\n",__FUNCTION__,__LINE__,cmd);
			system(cmd);
		}
		mib_get_s(AWIFI_WLAN1_AUTH_ENABLE,&enable, sizeof(enable));
		if(enable){
			snprintf(cmd,sizeof(cmd),"iptables -t mangle -I LanAuth_Exclude 1  -m physdev --physdev-in wlan0 -j RETURN");
			//printf("@@---%s#%d---cmd=%s----@@\n",__FUNCTION__,__LINE__,cmd);
			system(cmd);
		}
	}
		
	return 0;
}

int awifi_preFirewall_clear()
{
	printf("enter awifi_preFirewall_clear!\n");
	va_cmd(IPTABLES, 4, 1, "-D", (char *)FW_FORWARD, "-j", "awifi_preDrop");
	va_cmd(IPTABLES, 2, 1, "-X",  "awifi_preDrop");
	return 0;
}
int awifi_preFirewall_enable()
{
	int idx=0;
	char cmd[256];
	unsigned char enable;

	printf("enter awifi_preFirewall_enable!\n");

	va_cmd(IPTABLES, 2, 1, "-N", "awifi_preDrop");
	va_cmd(IPTABLES, 5, 1, "-I", (char *)FW_FORWARD, "1", "-j", "awifi_preDrop");

	{
		for(idx=AWIFI_LAN_START_NUM;idx<AWIFI_LAN_END_NUM;idx++)
		{
			if(idx==AWIFI_LAN_START_NUM){
				mib_get_s(AWIFI_LAN1_AUTH_ENABLE,&enable, sizeof(enable));
				if(!enable)
					continue;
			}
			if(idx==(AWIFI_LAN_START_NUM+1)){
				mib_get_s(AWIFI_LAN2_AUTH_ENABLE,&enable, sizeof(enable));
				if(!enable)
					continue;
			}
			if(idx==(AWIFI_LAN_START_NUM+2)){
				mib_get_s(AWIFI_LAN3_AUTH_ENABLE,&enable, sizeof(enable));
				if(!enable)
					continue;
			}
			if(idx==(AWIFI_LAN_START_NUM+3)){
				mib_get_s(AWIFI_LAN4_AUTH_ENABLE,&enable, sizeof(enable));
				if(!enable)
					continue;
			}
			snprintf(cmd,sizeof(cmd),"iptables -I awifi_preDrop 1  -m physdev --physdev-in eth0.%d -j DROP",idx);
			//printf("@@---%s#%d---cmd=%s----@@\n",__FUNCTION__,__LINE__,cmd);
			system(cmd);
		}
		mib_get_s(AWIFI_WLAN1_AUTH_ENABLE,&enable, sizeof(enable));
		if(enable){
			snprintf(cmd,sizeof(cmd),"iptables -I awifi_preDrop 1  -m physdev --physdev-in wlan0 -j DROP");
			//printf("@@---%s#%d---cmd=%s----@@\n",__FUNCTION__,__LINE__,cmd);
			system(cmd);
		}
				snprintf(cmd,sizeof(cmd),"iptables -I awifi_preDrop 1 -m physdev --physdev-in wlan0-vap0 -j DROP");
			//printf("@@---%s#%d---cmd=%s----@@\n",__FUNCTION__,__LINE__,cmd);
			system(cmd);
	}
		
	return 0;
}


int awifiAddFwRuleChain()
{
	char cmd[256];
	int idx;
	struct in_addr lanip, lanmask, lannet;
	char ipaddr[16],subnet[16];
	int maskbit=0,i=0;

	system("iptables -t mangle -F LanAuth_Exclude");
	system("iptables -t mangle -X LanAuth_Exclude");
	system("iptables -t mangle -D PREROUTING -i br0 -j LanAuth_Exclude");
	
	system("iptables -t mangle -N LanAuth_Exclude");
	system("iptables -t mangle -A PREROUTING -i br0 -j LanAuth_Exclude");

	mib_get_s(MIB_ADSL_LAN_IP, (void *)&lanip, sizeof(lanip));
	mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)&lanmask, sizeof(lanmask));
	lannet.s_addr = lanip.s_addr & lanmask.s_addr;
	strncpy(ipaddr, inet_ntoa(lannet), 16);
	ipaddr[15] = '\0';

	for(;i<32;i++)
	{
		if(lanmask.s_addr&(0x80000000>>i))
			maskbit++;
	}

	printf("awifiAddFwRuleChain:lannet=%s,maskbit=%d\n",ipaddr,maskbit);

#if 0
	for(idx=AWIFI_LAN_START_NUM;idx<AWIFI_LAN_END_NUM;idx++)
	{
			snprintf(cmd,sizeof(cmd),"iptables -t mangle -A LanAuth_Exclude -i eth0.%d -s %s/%d -j MARK --set-mark %d/%s",idx,ipaddr,maskbit,
			FW_MARK_KNOWN,CHAIN_MARK_MASK);
		//printf("@@---%s#%d---cmd=%s----@@\n",__FUNCTION__,__LINE__,cmd);
		system(cmd);
	}
	snprintf(cmd,sizeof(cmd),"iptables -t mangle -A LanAuth_Exclude -i wlan0 -s %s/%d -j MARK --set-mark %d/%s",ipaddr,maskbit,
		FW_MARK_KNOWN,CHAIN_MARK_MASK);
	//printf("@@---%s#%d---cmd=%s----@@\n",__FUNCTION__,__LINE__,cmd);
	system(cmd);
#endif
	system("iptables -t mangle -A LanAuth_Exclude  -m physdev --physdev-in wlan0-vap0 -j  RETURN");
	snprintf(cmd,sizeof(cmd), "iptables -t mangle -A LanAuth_Exclude  -m physdev --physdev-in eth0+ -s %s/%d -j MARK --set-mark 0x2/0xfff",ipaddr,maskbit);
	system(cmd);
	snprintf(cmd, sizeof(cmd), "iptables -t mangle -A LanAuth_Exclude  -m physdev --physdev-in wlan+ -s %s/%d -j MARK --set-mark 0x2/0xfff", ipaddr,maskbit);
	system(cmd);


	system("iptables -F LanAuth_Web");

	mib_get_s(MIB_ADSL_LAN_IP2, (void *)&lanip, sizeof(lanip));
	mib_get_s(MIB_ADSL_LAN_SUBNET2, (void *)&lanmask, sizeof(lanmask));
	lannet.s_addr = lanip.s_addr & lanmask.s_addr;
	strncpy(ipaddr, inet_ntoa(lannet), 16);
	ipaddr[15] = '\0';

	for(;i<32;i++)
	{
		if(lanmask.s_addr&(0x80000000>>i))
			maskbit++;
	}

	snprintf(cmd,sizeof(cmd),"iptables -I  LanAuth_Web -i br0 -p TCP -s %s/%d --dport 80 -j DROP",ipaddr,maskbit);
	//printf("@@---%s#%d---cmd=%s----@@\n",__FUNCTION__,__LINE__,cmd);
	system(cmd);
	
}

//static struct callout_s fwRule_timeout;
int awifiAddFwRule()
{
	awifi_lan_enable();
}
#endif

int killWiFiDog(void)
{
	system(KILLWIFIDOGSTR);

	return 0;
}

void startWiFiDog(void *null)
{
	int ret;
	char cmdbuf[128];
	//ret=va_cmd( WIFIDOGPATH, 2, 0,"-c",WIFIDOGCONFPATH);
	snprintf(cmdbuf,sizeof(cmdbuf),"export LD_LIBRARY_PATH=/var/config/awifi;%s",WIFIDOGPATH);
	printf("startWiFiDog:cmdbuf=%s\n",cmdbuf);
	ret=system(cmdbuf);

	UNTIMEOUT_F(startWiFiDog, 0, wifidog_timeout);
#ifdef CONFIG_CT_AWIFI_JITUAN_SMARTWIFI
	awifiAddFwRule();
#endif
	awifi_preFirewall_clear();
	/*clear fastpath entry*/
#ifdef CONFIG_RTL867X_IPTABLES_FAST_PATH
	system("/bin/echo 1 > /proc/net/fp_path_clear");
#else
	rtk_hwnat_flow_flush();
#endif
	return;
}
void restartWiFiDog(int restart)
{
	char wifidogenable=0,ret=0;
	MIB_CE_MBSSIB_T Entry;
	ret=wlan_getEntry(&Entry, 1);
	wifidogenable=!Entry.wlanDisabled;
	if(ret)
	{
		killWiFiDog();
		if(wifidogenable)
		{
			if(restart)
			{
				UNTIMEOUT_F(startWiFiDog, 0, wifidog_timeout);
				TIMEOUT_F(startWiFiDog, 0, 2, wifidog_timeout);
			}else
			{
                void * null;
				startWiFiDog(null);
			}
		}
		return;
	}
	return;
}
#endif
#ifdef CONFIG_CT_AWIFI_JITUAN_SMARTWIFI
unsigned int RTK_WAN_GET_UNAvailable_WifiDogInterface()
{
	unsigned int sourceinterface_unmask = 0;
	unsigned char wifidog_wan_exist = 0;
	MIB_CE_ATM_VC_T entryVC;
	int i, totalVC_entry = mib_chain_total(MIB_ATM_VC_TBL);

	memset(&entryVC, 0, sizeof(entryVC));
	
	for(i=0;i<totalVC_entry;i++){
		if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&entryVC ))
			continue;
		if(entryVC.cmode == CHANNEL_MODE_BRIDGE)
		{
			sourceinterface_unmask |= entryVC.itfGroup;
			continue;
		}			
		if(entryVC.applicationtype != X_CT_SRV_INTERNET)
		{
			sourceinterface_unmask |= entryVC.itfGroup;
			continue;
		}
		if(entryVC.dgw & IPVER_IPV4)
		{
			wifidog_wan_exist = 1;
		}
	}
	if(wifidog_wan_exist == 0)
		return 0x1f;    //wifidog wan no exist, not set wifidog_interface
	return sourceinterface_unmask;
}

int getDefaultGWEntry(MIB_CE_ATM_VC_T *pEntry)
{
	int total,i;
	if(pEntry == NULL)
		return 0;
#ifdef DEFAULT_GATEWAY_V2
	unsigned int dgw;
	if (mib_get_s(MIB_ADSL_WAN_DGW_ITF, (void *)&dgw, sizeof(dgw)) != 0)
	{
		if (!getWanEntrybyindex(pEntry, dgw)) {
			return 1;
		}
	}
#else
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for( i=0; i<total; i++ )
	{
		if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)pEntry ) )
			continue;
		if(pEntry->cmode == CHANNEL_MODE_BRIDGE)
			continue;
		if(pEntry->applicationtype != X_CT_SRV_INTERNET)
			continue;
		if(pEntry->dgw & IPVER_IPV4)
		{
			return 1;
		}
	}
#endif
	
	return 0;
}
	
int getWANEntryByIfName(char *pIfname, MIB_CE_ATM_VC_T *pEntry)
{
	unsigned int entryNum, i;
	char ifname[64]={0};
	
	if((pIfname == NULL) || (pEntry == NULL))
		return -1;
	
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)pEntry ) )
			continue;
	
		if (pEntry->enable == 0)
			continue;

		ifGetName(pEntry->ifIndex,ifname,sizeof(ifname));
	
		if(!strcmp(ifname,pIfname)){
			break;
		}
	}
	
	if(i>= entryNum){
		return -1;
	}
	
	return 0;
}

int getdevIdFromaWiFiConf(char *devId)
{
	int ret=0;
	FILE *fp=NULL;
	
	if ((fp = fopen(WIFIDOGCONFPATH, "r"))) {
        while (!feof(fp)) {
            if (fscanf(fp, "DevID %s", devId) != 1) {
                /* Not on this line */
                while (!feof(fp) && fgetc(fp) != '\n') ;
            } else {
                /* Found it */
				ret=1;
                break;
            }
        }
        fclose(fp);
    }
	else{
		printf("Could not open file:%s\n", WIFIDOGCONFPATH);
	}

	return ret;
}

int getgwaddrFromaWiFiConf(char *gwaddr)
{
	int ret=0;
	FILE *fp=NULL;

	if ((fp = fopen(WIFIDOGCONFPATH, "r"))) {
        while (!feof(fp)) {
            if (fscanf(fp, "gatewayaddress %s", gwaddr) != 1) {
                /* Not on this line */
                while (!feof(fp) && fgetc(fp) != '\n') ;
            } else {
                /* Found it */
				ret=1;
                break;
            }
        }
        fclose(fp);
    }
	else{
		printf("Could not open file:%s\n", WIFIDOGCONFPATH);
	}

	return ret;
}

int getgwportFromaWiFiConf(int *gwport)
{
	int ret=0;
	FILE *fp=NULL;

	if ((fp = fopen(WIFIDOGCONFPATH, "r"))) {
        while (!feof(fp)) {
            if (fscanf(fp, "gatewayport %d", gwport) != 1) {
                /* Not on this line */
                while (!feof(fp) && fgetc(fp) != '\n') ;
            } else {
                /* Found it */
				ret=1;
                break;
            }
        }
        fclose(fp);
    }
	else{
		printf("Could not open file:%s\n", WIFIDOGCONFPATH);
	}

	return ret;
}

#define MAX_BUF 4096
int parse_ServerHostname(const char *filename, char *ServerStr, char *hostname)
{
	int ret=0;
	FILE *fp=NULL;
	char line[MAX_BUF];

	if((filename == NULL) || (ServerStr == NULL) || (hostname == NULL))
		return -1;

	if ((fp = fopen(filename, "r"))) {
		while (!feof(fp)) {
			fgets(line, MAX_BUF - 1, fp);
			if(strstr(line, ServerStr))
			{
checkagain:					
				if (fscanf(fp, "Hostname %s", hostname) != 1) {
                /* Not on this line */
                	while (!feof(fp) && fgetc(fp) != '\n') ;
	            } else {
	                /* Found it */
					ret=1;
	                break;
	            }
				goto checkagain;				
			}
		}
		
        fclose(fp);
    }
	else{
		printf("Could not open file:%s\n", filename);
	}

	return ret;
}

int parse_ServerHttpPort(const char *filename, char *ServerStr, int *port)
{
	int ret=0;
	FILE *fp=NULL;
	char line[MAX_BUF];

	if((filename == NULL) || (ServerStr == NULL))
		return -1;

	if ((fp = fopen(filename, "r"))) {
		while (!feof(fp)) {
			fgets(line, MAX_BUF - 1, fp);
			if(strstr(line, ServerStr))
			{
checkagain:				
				if (fscanf(fp, "HttpPort %d", port) != 1) {
                /* Not on this line */
                	while (!feof(fp) && fgetc(fp) != '\n') ;
	            } else {
	                /* Found it */
					ret=1;
	                break;
	            }
				goto checkagain;				
			}
		}
		
        fclose(fp);
    }
	else{
		printf("Could not open file:%s\n", filename);
	}

	return ret;
}

int getLineNumber(char *ServerStr, char *TypeStr)
{
	FILE *fp=NULL;
	char line[MAX_BUF], tmp1[64], tmp2[MAX_BUF];
	int count=0;
	
	if((TypeStr == NULL) || (ServerStr == NULL))
		return -1;

	if ((fp = fopen(WIFIDOGCONFPATH, "r"))) {
		while (!feof(fp)) {
			fgets(line, MAX_BUF - 1, fp);
			count++;
			if(strstr(line, ServerStr))
			{
retryagain:	
				fscanf(fp, "%s %s", tmp1, tmp2);
				if (strcmp(tmp1, TypeStr)) {
                /* Not on this line */
					count++;
                	while (!feof(fp) && fgetc(fp) != '\n') ;
	            } else {
	                /* Found it */
					count++;
	                break;
	            }
				goto retryagain;
			}
		}
		
        fclose(fp);
    }
	else{
		printf("Could not open file:%s\n", WIFIDOGCONFPATH);
	}

	return count;
}

struct default_server_setting *gServerSetting=NULL;
void addServerSetting(int linenum, char *newconf)
{
	struct default_server_setting *pServerSetting;

	pServerSetting = malloc(sizeof(struct default_server_setting));
	memset(pServerSetting, 0, sizeof(struct default_server_setting));
	pServerSetting->linenum=linenum;
	strncpy(pServerSetting->data, newconf, BUF_SIZE);
	
	if(gServerSetting==NULL)
	{
		gServerSetting=pServerSetting;
	}
	else{
		struct default_server_setting *tmp=gServerSetting;
		while(tmp->next)
			tmp=tmp->next;
		tmp->next=pServerSetting;
	}
}

struct default_server_setting *server_setting_find( int line )
{
	struct default_server_setting *p=NULL;
	
	if(gServerSetting==NULL) return NULL;
	
	p = gServerSetting;
	while(p)
	{
		if(p->linenum == line) break;
		p = p->next;
	}
	
	return p;
}

void free_server_setting()
{
	struct default_server_setting *p;

	if( (gServerSetting==NULL)) return;
	
	p = gServerSetting;
	while(p)
	{
		free(p);
		p = p->next;
	}
	gServerSetting=NULL;
}

int UpdateAwifiConfSetting()
{
	int count=0;
	FILE *fp=NULL;
	FILE *fp2=NULL;
	char line[MAX_BUF];
	struct default_server_setting *p=NULL;

	fp = fopen(WIFIDOGCONFPATH, "r");
	fp2 = fopen(WIFIDOGTMPCONFPATH, "w+");

	if(!fp || !fp2)
		return -1;

	while (!feof(fp))
	{
		fgets(line, MAX_BUF - 1, fp);
		count++;

		p=server_setting_find(count);
		if(p)
			fprintf(fp2, p->data);
		else
			fputs(line, fp2);
	}

	fclose(fp);
	fclose(fp2);

	system("cp -f /var/config/awifi/temp.conf /var/config/awifi/awifi.conf");
	system("rm -f /var/config/awifi/temp.conf");

	return 0;
}

char *geAwifiVersion(char *buf, int len)
{
	if(!buf || !len)
	{
		return NULL;
	}

	FILE *fp;
	unsigned char tmpBuf[64], *pStr;
	tmpBuf[0]=0;
	fp = fopen("/var/config/awifi/binversion", "r");
	if (fp!=NULL) {
		fgets(tmpBuf, sizeof(tmpBuf), fp);  //main version
		fclose(fp);
	}
	if(!strlen(tmpBuf))
		snprintf(buf,len,AWIFI_DEFAULT_BIN_VERSION);
	else
		snprintf(buf,len,"%s", tmpBuf);
	
	return buf;
}

#endif

#ifdef CONFIG_USER_QUICKINSTALL
typedef struct wan_downSpeed_info{
	struct  timeval    oldtv;
	uint64_t  oldDownloadCnt; /* save download counter last time */
	unsigned int downloadRate; /* in unit of kbps */
}WAN_DOWNSPEED_INFO;

int downspeedInit = 0;
struct callout_s downspeed_ch;
WAN_DOWNSPEED_INFO downspeedInfo;
static void calWanDownSpeed();

void initWanDownSpeedInfo()
{
	memset(&downspeedInfo, 0, sizeof(downspeedInfo));
	gettimeofday(&downspeedInfo.oldtv, NULL);
	calWanDownSpeed();
}

#define WANDOWNSPEEDFILE "/var/fh_speedtest_ds"
int writeWanDownSpeedInfo()
{
	int fd;
	int i, portNum=4;

	fd = open(WANDOWNSPEEDFILE, O_RDWR|O_CREAT, 0644);
	if (fd < 0)
	{
		return -1;
	}

	if( !flock_set(fd, F_WRLCK) )
	{
		/* clear /var/fh_speedtest_ds */
		if ( !truncate(WANDOWNSPEEDFILE, 0) )
		{
			write(fd, (void *)&downspeedInfo.downloadRate, sizeof(int));
		}

		flock_set(fd, F_UNLCK);
	}

	close(fd);
	return 0;
}

static void calWanDownSpeed(void)
{
	unsigned int pon_port_idx = 1;
	uint64_t downloadcnt = 0;
	struct  timeval  tv;
	int msecond;
	int ret=-1;
#if defined(CONFIG_COMMON_RT_API)
	rt_switch_phyPortId_get(LOG_PORT_PON, &pon_port_idx);
#endif

	gettimeofday(&tv,NULL);
#if defined(CONFIG_COMMON_RT_API)
	ret = rt_stat_port_get(pon_port_idx, RT_IF_IN_OCTETS_INDEX, &downloadcnt);
#endif
	if(ret < 0)
	{
		fprintf(stderr, "Get PON Port Stat error \n" );
		return ;
	}

	/* calculate upload and download rate */
	if(downloadcnt <= downspeedInfo.oldDownloadCnt)
	{
		downspeedInfo.downloadRate = 0;
	}
	else
	{
		msecond = (tv.tv_sec - downspeedInfo.oldtv.tv_sec) * 1000 +
				  ((int)(tv.tv_usec) - (int)(downspeedInfo.oldtv.tv_usec)) / 1000;

		downspeedInfo.downloadRate = ((downloadcnt - downspeedInfo.oldDownloadCnt) * 1000 / msecond) >> 7;
	}

	downspeedInfo.oldDownloadCnt = downloadcnt;
	downspeedInfo.oldtv.tv_sec = tv.tv_sec;
	downspeedInfo.oldtv.tv_usec = tv.tv_usec;

	//printf("[%s %d]:downloadRate=%d\n", __func__, __LINE__, downspeedInfo.downloadRate);
}

static int isHostSpeedupEnable(void)
{
	FILE *fp;
	int val;

	if ((fp = fopen("/proc/HostSpeedUP", "r")) == NULL)
		return -1;
	fscanf(fp, "%d", &val);
	fclose(fp);

	return val;
}

void poll_wan_downspeed(void *dummy)
{
	unsigned char enable;
	unsigned int interval;
    int vInt;
    static int hostSpeedUp=0, upCnt=0;

	if(downspeedInit == 0)
	{
		initWanDownSpeedInfo();
		downspeedInit = 1;
	}
	else
		calWanDownSpeed();

	writeWanDownSpeedInfo();

    vInt = isHostSpeedupEnable();
    if (0 == hostSpeedUp)
    {
        if (1 == vInt)
        {
            if (upCnt++ >= 5)
                hostSpeedUp = 1;
        }
        else
        {
            upCnt = 0;
        }
    }
    else
    {
        if (0 == vInt)
        {
            hostSpeedUp = 0;
            upCnt = 0;
        }
    }

    if (hostSpeedUp)
        TIMEOUT_F(poll_wan_downspeed, 0, 5, downspeed_ch);
    else
        TIMEOUT_F(poll_wan_downspeed, 0, 1, downspeed_ch);
}
#endif

#ifdef CONFIG_USER_BEHAVIOR_ANALYSIS
#ifdef CONFIG_TR142_MODULE
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <rtk/rtk_tr142.h>

#define TR142_DEV_FILE "/dev/rtk_tr142"
#endif

const char *SNORT = "/bin/snort";
const char *SNORT_PID = "/var/run/snort.pid";

void setup_behavior_analysis(void)
{
	unsigned char enable = 0;
	int fd;
	int disable_hwnat = 0;
	char *argv[10] = {0};
	char ifname[IFNAMSIZ] = {0};
	int pid = -1;
	FILE *f = NULL;
/*
	pid = read_pid(SNORT_PID);
	if(pid > 0)
		kill(pid, SIGTERM);
*/
	mib_get_s(MIB_BA_ENABLE, &enable, sizeof(enable));
	if(enable)
	{
		int i;
		disable_hwnat = 1;

		f = fopen("/proc/fc/ctrl/hwnat", "w");
		if(f)
		{
			fputs("0\n", f);
			fclose(f);
		}
		do_rtk_fc_flow_flush();
		// snort -c /etc/snort.conf -l /tmp -A none -N -q -i br0
		//va_cmd((char *)SNORT, 10, 0, "-c", "/etc/snort.conf", "-l", "/tmp", "-A", "none", "-N", "-q", "-i", "br0");
	}
	else
	{
		f = fopen("/proc/fc/ctrl/hwnat", "w");
		if(f)
		{
			fputs("1\n", f);
			fclose(f);
		}
	}

#if 0//def CONFIG_TR142_MODULE
	fd = open(TR142_DEV_FILE, O_WRONLY);
	if(fd >= 0)
	{
		if(ioctl(fd, RTK_TR142_IOCTL_SET_DSIABLE_HWNAT_PATCH, &disable_hwnat) != 0)
		{
			fprintf(stderr, "ERROR: RTK_TR142_IOCTL_SET_DSIABLE_HWNAT_PATCH failed\n");
		}
		close(fd);
	}
#endif
}
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
int parseFrameworkPartition(char **pf1, char **pf2, char **pc1)
{
	static char _f1[10] = {0}, _f2[10] = {0}, _c1[10] = {0};
	char buf[128] = {0};
	int id = 0, n = 0, ret = 0;
	FILE *fp = NULL;
	
	if(_f1[0] && _f2[0] && _c1[0]){
		if(pf1) *pf1 = _f1;
		if(pf2) *pf2 = _f2;
		if(pc1) *pc1 = _c1;
		return 0;
	}
	
	fp = fopen("/proc/mtd", "r");
	if(fp)
	{
		while(!feof(fp))
		{
			fgets(buf, sizeof(buf)-1, fp);
			n = 0;
			ret = sscanf(buf, "mtd%d: %*[^\"]%n%*s", &id, &n);
			if(ret && n > 0)
			{
				//printf("====> %d, %s\n", id, (buf+n));
				if(strstr(buf+n+1, "framework1")){
					memset(_f1, 0, sizeof(_f1));
					snprintf(_f1, sizeof(_f1), "%d", id);
					if(pf1) *pf1 = _f1;
				}
				else if(strstr(buf+n+1, "framework2")){
					memset(_f2, 0, sizeof(_f2));
					snprintf(_f2, sizeof(_f2), "%d", id);
					if(pf2) *pf2 = _f2;
				}
				else if(strstr(buf+n+1, "apps")){
					memset(_c1, 0, sizeof(_c1));
					snprintf(_c1, sizeof(_c1), "%d", id);
					if(pc1) *pc1 = _c1;
				}
			}
		}
		fclose(fp);
	}
	else return -1;
	
	return 1;
}
#endif

#ifdef YUEME_4_0_SPEC
void dbus_do_ClearExtraMIB(void)
{
	char buf[128];
	unlink(APP_EXTRAMIB_SAVEFILE);
	snprintf(buf, sizeof(buf)-1, "rm -rf %s/*", APP_EXTRAMIB_PATH);
	system(buf);
}
void dbus_do_ExtraMIB2flash(void)
{
	char buf[128];
	snprintf(buf, sizeof(buf)-1, "mkdir -p %s", APP_EXTRAMIB_SAVE_PATH);
	system(buf);
	snprintf(buf, sizeof(buf)-1, "tar -C %s -zcf %s ./", APP_EXTRAMIB_PATH, APP_EXTRAMIB_SAVEFILE);
	system(buf);
}
void dbus_do_flash2ExtraMIB(void)
{
	char buf[128];
	struct stat st;
	snprintf(buf, sizeof(buf)-1, "rm -rf %s", APP_EXTRAMIB_PATH);
	system(buf);
	snprintf(buf, sizeof(buf)-1, "mkdir -p %s", APP_EXTRAMIB_PATH);
	system(buf);
	if(stat(APP_EXTRAMIB_SAVEFILE, &st) == 0)
	{
		snprintf(buf, sizeof(buf)-1, "mkdir -p %s", APP_EXTRAMIB_PATH_TMP);
		system(buf);
		snprintf(buf, sizeof(buf)-1, "tar -C %s -zxf %s ./", APP_EXTRAMIB_PATH_TMP, APP_EXTRAMIB_SAVEFILE);
		system(buf);
		
		{
			int i, num;
			APPROUTE_RULE_T entry;
			snprintf(buf, sizeof(buf)-1, "mkdir -p %s", APPROUTE_EXTRAMIB_PATH);
			system(buf);
			num = mib_chain_total(MIB_APPROUTE_TBL);
			for(i=0; i<num; i++)
			{
				if(!mib_chain_get(MIB_APPROUTE_TBL, i, &entry))
					continue;
				snprintf(buf, sizeof(buf)-1, "mv %s/%d_* %s", APPROUTE_EXTRAMIB_PATH_TMP, entry.instNum, APPROUTE_EXTRAMIB_PATH);
				system(buf);
			}
		}
		
#ifdef SUPPORT_APPFILTER	
		{
			int i, num;
			MIB_CE_APP_FILTER_T entry;
			snprintf(buf, sizeof(buf)-1, "mkdir -p %s", APPFILTER_EXTRAMIB_PATH);
			system(buf);
			num = mib_chain_total(MIB_APP_FILTER_TBL);
			for(i=0; i<num; i++)
			{
				if(!mib_chain_get(MIB_APP_FILTER_TBL, i, &entry))
					continue;
				snprintf(buf, sizeof(buf)-1, "mv %s/%d_* %s", APPFILTER_EXTRAMIB_PATH_TMP, entry.inst_num, APPFILTER_EXTRAMIB_PATH);
				system(buf);
			}
		}
#endif
		snprintf(buf, sizeof(buf)-1, "rm -rf %s", APP_EXTRAMIB_PATH_TMP);
		system(buf);
	}
}
#if defined(CONFIG_PPPOE_MONITOR) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
static int add_bridge_data_acl_rule(BRIDGE_DATA_ACL_RULE_Tp pentry)
{
	MIB_CE_ATM_VC_T vc_entry = {0};
	unsigned int i,num;
	struct PPPoEMonitorPacket packet_feature = {0};

#ifndef SOCK_MARK2_FORWARD_BY_PS_START
	fprintf(stderr, "Not define FC SOCK_MARK2_FORWARD_BY_PS_START mark\n");
	return -1;
#endif

	if(*((unsigned int *)(pentry->srcip)) != 0)
	{
		memcpy(packet_feature.srcip, pentry->srcip, IP_ADDR_LEN);
	}
	switch(pentry->protocol)
	{
		case 1://TCP
			packet_feature.protocol = 6;
			break;
		case 2://UDP
			packet_feature.protocol = 17;
			break;
		case 4://ICMP
			packet_feature.protocol = 1;
			break;
		case 0://ALL
			packet_feature.protocol = 0;
			break;
		default:
			printf("%s-%d BRIDGE_DATA_ACL_RULE_TBL.protocol value fail\n",__func__,__LINE__);
			return -1;
	}
	
	if((pentry->srcport != 0) && (pentry->protocol != 4))
	{
		packet_feature.srcport = pentry->srcport;
	}

	num = mib_chain_total(MIB_ATM_VC_TBL);
	for( i=0 ; i<num ; i++ )
	{
		if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vc_entry))
			continue;
		if(vc_entry.cmode == CHANNEL_MODE_BRIDGE)
		{
			char ifName[32]={0};
			ifGetName(PHY_INTF(vc_entry.ifIndex), ifName, sizeof(ifName));
			if(ifName[0] == 0)
				continue;		
			if(strlen(pentry->wanintf)==0 || !strcmp(ifName, pentry->wanintf))
			{
				strcpy(packet_feature.indev, ifName);
				cfg_pppoe_monitor_packet(&packet_feature,CMD_ADD_PPPOE_MONITOR_PACKET);
			}
		}
	}
	return 0;
}
#endif
void bridgeDataAcl_take_effect(int enable)
{
	BRIDGE_DATA_ACL_RULE_T entry;
	unsigned int i,num;
#if defined(CONFIG_PPPOE_MONITOR) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	cfg_pppoe_monitor_packet(NULL, CMD_FLUSH_PPPOE_MONITOR_PACKET);
#endif
	if(!enable)
		return;
	num = mib_chain_total(MIB_BRIDGE_DATA_ACL_RULE_TBL);
	for( i=0 ; i<num ; i++ )
	{
		if( !mib_chain_get(MIB_BRIDGE_DATA_ACL_RULE_TBL, i, (void*)&entry))
			continue;
#if defined(CONFIG_PPPOE_MONITOR) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		add_bridge_data_acl_rule(&entry);
#endif
	}
}

#define APPCTL_PATH "/proc/appCtrl"
const char NF_APPROUTE_CHAIN_ACT[]="approute_ACT_%d";
const char NF_APPROUTE_IPSET_IPNET_CHAIN[] = "approute_set_ip_%d";
const char NF_APPROUTE_IPSET_IPV6NET_CHAIN[] = "approute_set_ip6_%d";
const char NF_APPROUTE_IPSET_DNS_CHAIN[] = "approute_set_dns_%d";
const char NF_APPROUTE_IPSET_DNSv6_CHAIN[] = "approute_set_dns6_%d";
const char NF_APPROUTE_IPSET_SMAC_CHAIN[] = "approute_mac_%d";
const char NF_APPROUTE_DNS_SET_CHAIN[] = "approute_dns_%d";
const char APPROUTE_IPSET_RULE_FILE[]="/tmp/approute_rule";
const char NF_APPROUTE_CHAIN[]="approute";
const char NF_APPROUTE_CHAIN_IP[]="approute_ip";
const char NF_APPROUTE_CHAIN_MAC[]="approute_mac";
const char NF_APPROUTE_CHAIN_MAC_IP[]="approute_mac_ip";
const char NF_APPROUTE_CHAIN_DNS[]="approute_dns";
const char NF_APPROUTE_CHAIN_FWD[]="approute_fwd";
const char NF_APPROUTE_CHAIN_POST[]="approute_post";
const char NF_APPROUTE_CHAIN_OUTPUT[]="approute_output";

#define APPROUTE_ACT_PROUTE 0
#define APPROUTE_ACT_DSCP 1
#define APPROUTE_ACT_PROUTE_DSCP 4

static char* _APP_GETEXTRAENTRY(char **ptr, char *m, int id, const char *name, const char *path) 
{ 
	FILE *fp=NULL; 
	char _path[64]={0}, *_data = NULL, *_p;
	int len1=0, len2=0, len=0;

	if(name) snprintf(_path, sizeof(_path)-1, "%s/%d_%s", path, id, name);
	if(m) len1 = strlen(m);
	if(_path[0] && (fp=fopen(_path,"r"))) {
		fseek( fp, 0, SEEK_END );
		len2 = ftell( fp );
		fseek( fp, 0, SEEK_SET );
	}
	len = len1+1+len2+1;
	if(len > 2 && (_data=malloc(len))) {
		_p = _data; *_p = '\0';
		if(len1 > 0) { strncpy(_p, m, len1); _p += len1; *_p = '\0'; }
		if(len2 > 0 && fp) { if(_p!=_data){*(_p++)=';';} _p += fread(_p, 1, len2, fp); *_p = '\0'; }
	}
	if(ptr) *ptr = _data;
	if(fp) fclose(fp);
	return (void*)_data;
}

//act 0:add 1:del
int config_one_approute_with_type(int act, APPROUTE_RULE_T *pentry, int type, char **diff_array)
{
	int i = 0;
	FILE *fp = NULL;
	char *act_str;
	char cmd[128];
	char file_path[64];
	char ipset_name[32];
#ifdef CONFIG_IPV6
	char ipset_v6_name[32];
#endif

	if(pentry == NULL || diff_array == NULL || diff_array[0] == NULL)
		return 0;
	
	snprintf(file_path, sizeof(file_path), "%s_set_%d_type_%d", APPROUTE_IPSET_RULE_FILE, pentry->instNum, type);
	if(!(fp = fopen(file_path, "w")))
	{
		fprintf(stderr,"[%s %d] open file %s fail\n",__func__,__LINE__,file_path);
		goto error;
	}

	switch(type) {
		case 0:  {// ip
			if(pentry->matchType == 0 || pentry->matchType == 1) {
				act_str = (act == 0) ? "add" : "del";
				snprintf(ipset_name, sizeof(ipset_name), NF_APPROUTE_IPSET_IPNET_CHAIN, pentry->instNum);
#ifdef CONFIG_IPV6
				snprintf(ipset_v6_name, sizeof(ipset_v6_name), NF_APPROUTE_IPSET_IPV6NET_CHAIN, pentry->instNum);
#endif
				while(diff_array[i]) {
					if(!strchr(diff_array[i], ':'))
						fprintf(fp, "%s %s %s\n", act_str, ipset_name, diff_array[i]);
#ifdef CONFIG_IPV6
					else
						fprintf(fp, "%s %s %s\n", act_str, ipset_v6_name, diff_array[i]);
#endif
					i++;
				}
			}
			break;
		}
		case 1:  // domain
			if(pentry->matchType == 0 || pentry->matchType == 1 ) {
				snprintf(ipset_name, sizeof(ipset_name), NF_APPROUTE_IPSET_SMAC_CHAIN, pentry->instNum);
				while(diff_array[i]) {
					fprintf(fp, "%s,", diff_array[i]);
					i++;
				}
			}
			break;
		case 2:  // mac 
			if(pentry->matchType == 0 || pentry->matchType == 2 ) {
				act_str = (act == 0) ? "add" : "del";
				snprintf(ipset_name, sizeof(ipset_name), NF_APPROUTE_IPSET_SMAC_CHAIN, pentry->instNum);
				while(diff_array[i]) {
					char *mac_p, macStr[18], c=0;
					if(!strchr(diff_array[i], ':')) {
						mac_p = macStr;
						while(c<12 && diff_array[i][c]) { 
							*(mac_p++) = diff_array[i][c];
							if(c&1) *(mac_p++) = ':';
							c++;
						}
						if(mac_p > macStr) *(mac_p-1) = '\0';
						mac_p = macStr;
					} else mac_p = diff_array[i];
					fprintf(fp, "%s %s %s\n", act_str, ipset_name, mac_p);
					i++;
				}
			}
			break;
		default: 
			goto error;
	}
	
	fclose(fp); fp = NULL;
	if(type == 1){
		if(pentry->matchType == 0 || pentry->matchType == 1) {
			snprintf(ipset_name, sizeof(ipset_name), NF_APPROUTE_DNS_SET_CHAIN, pentry->instNum);
			act_str = (act == 0) ? "config_dns_set" : "clear_dns_set";
			snprintf(cmd, sizeof(cmd), "echo \"%s %s FILE %s\" > %s", act_str, ipset_name, file_path, APPCTL_PATH);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
		}
	}
	else {
		snprintf(cmd, sizeof(cmd), "%s %s -! -f %s", IPSET, IPSET_COMMAND_RESTORE, file_path);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
	}
	unlink(file_path);
	return 0;
	
error:
	if(fp) {
		fclose(fp);
		unlink(file_path);
	}
	return 1;
}

//act 0:add 1:del, cmode 2:only for iptables rule
int config_one_approute(int act, APPROUTE_RULE_T*pentry, int cmode)
{
	char cmd[300];
	char actstr[8];
	unsigned int wan_mark = 0, wan_mask = 0;
	unsigned int fw_mark = 0, fw_mask = 0;
	unsigned long long fw_mark2=0, fw_mask2=0;
	char ipset_smac_chain_name[32];
	char ipset_subnet_chain_name[32]={0}, ipset_dns_chain_name[32]={0};
#ifdef CONFIG_IPV6	
	char ipset_subnetv6_chain_name[32]={0}, ipset_dnsv6_chain_name[32]={0};
#endif
	char action_chain_name[32], chain_name[32], dns_set_name[32];
	char *ptr,*tmp,*pval=NULL;
	FILE *fp = NULL, *fp1 = NULL;
	char ipset_restore_file_path[64], path[64];
	char src_mac_match_str[64];
	char action_str[128], ip_match_str[128];

	if(pentry->enable==0)
		return 1;
	
	snprintf(path, sizeof(path), "%s_%d", APPROUTE_IPSET_RULE_FILE, pentry->instNum);
	if(!(fp1 = fopen(path, "w"))){
		printf("[%s:%d] error to open file %s\n", __FUNCTION__, __LINE__, path);
		return -1;
	}
	snprintf(ipset_subnet_chain_name, sizeof(ipset_subnet_chain_name), NF_APPROUTE_IPSET_IPNET_CHAIN, pentry->instNum );
	snprintf(ipset_dns_chain_name, sizeof(ipset_dns_chain_name), NF_APPROUTE_IPSET_DNS_CHAIN, pentry->instNum );
#ifdef CONFIG_IPV6
	snprintf(ipset_subnetv6_chain_name, sizeof(ipset_subnetv6_chain_name), NF_APPROUTE_IPSET_IPV6NET_CHAIN, pentry->instNum );
	snprintf(ipset_dnsv6_chain_name, sizeof(ipset_dnsv6_chain_name), NF_APPROUTE_IPSET_DNSv6_CHAIN, pentry->instNum );
#endif
	snprintf(ipset_smac_chain_name, sizeof(ipset_smac_chain_name), NF_APPROUTE_IPSET_SMAC_CHAIN, pentry->instNum );
	snprintf(dns_set_name, sizeof(dns_set_name), NF_APPROUTE_DNS_SET_CHAIN, pentry->instNum);
	snprintf(action_chain_name, sizeof(action_chain_name), NF_APPROUTE_CHAIN_ACT, pentry->instNum);
	
	if(act == 0 && cmode != 2)
	{
		snprintf(ipset_restore_file_path, sizeof(ipset_restore_file_path), "%s_set_%d", APPROUTE_IPSET_RULE_FILE, pentry->instNum);
		if(!(fp = fopen(ipset_restore_file_path, "w")))
		{
			fprintf(stderr,"[%s %d] open file %s fail\n",__func__,__LINE__,ipset_restore_file_path);
			goto error;
		}
		
		// filter mac
		if(pentry->matchType == 0 || pentry->matchType == 2 )
		{
			fprintf(fp, "create %s %s\n",ipset_smac_chain_name, IPSET_SETTYPE_HASHMAC);
			fprintf(fp, "flush %s\n",ipset_smac_chain_name);
			//ptr = pentry->macList;
			ptr = _APP_GETEXTRAENTRY(&pval, pentry->macList, pentry->instNum, "macList", APPROUTE_EXTRAMIB_PATH);
			while(ptr && ( tmp= strsep(&ptr, ",;")) !=NULL)
			{
				char *mac_p, macStr[18], c=0;
				if(!strchr(tmp, ':')) {
					mac_p = macStr;
					while(c<12 && tmp[c]) { 
						*(mac_p++) = tmp[c];
						if(c&1) *(mac_p++) = ':';
						c++;
					}
					if(mac_p > macStr) *(mac_p-1) = '\0';
					mac_p = macStr;
				} else mac_p = tmp;
				
				fprintf(fp, "add %s %s\n",ipset_smac_chain_name, mac_p);
			}		
			if(pval) free(pval);
		}
		
		if(pentry->matchType == 0 || pentry->matchType == 1)
		{
			fprintf(fp, "create %s %s\n",ipset_subnet_chain_name, IPSET_SETTYPE_HASHNET);
			fprintf(fp, "flush %s\n",ipset_subnet_chain_name);
			fprintf(fp, "create %s %s timeout 0 comment\n",ipset_dns_chain_name, IPSET_SETTYPE_HASHNET);
			fprintf(fp, "flush %s\n",ipset_dns_chain_name);
#ifdef CONFIG_IPV6
			fprintf(fp, "create %s %s\n",ipset_subnetv6_chain_name, IPSET_SETTYPE_HASHNETV6);
			fprintf(fp, "flush %s\n",ipset_subnetv6_chain_name);
			fprintf(fp, "create %s %s timeout 0 comment\n",ipset_dnsv6_chain_name, IPSET_SETTYPE_HASHNETV6);
			fprintf(fp, "flush %s\n",ipset_dnsv6_chain_name);
#endif
			//ptr = pentry->destIpList;
			ptr = _APP_GETEXTRAENTRY(&pval, pentry->destIpList, pentry->instNum, "destIpList", APPROUTE_EXTRAMIB_PATH);
			while(ptr && ( tmp= strsep(&ptr, ",;")) !=NULL)
			{
				if(!strchr(tmp, ':'))
					fprintf(fp, "add %s %s\n",ipset_subnet_chain_name, tmp);
#ifdef CONFIG_IPV6
				else
					fprintf(fp, "add %s %s\n",ipset_subnetv6_chain_name, tmp);
#endif
			}	
			if(pval) free(pval);
		}
		
		fclose(fp); fp = NULL;
		snprintf(cmd, sizeof(cmd), "%s %s -! -f %s", IPSET, IPSET_COMMAND_RESTORE, ipset_restore_file_path);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		unlink(ipset_restore_file_path);
		
		if(pentry->matchType == 0 || pentry->matchType == 1) {
			if((ptr = _APP_GETEXTRAENTRY(&pval, pentry->domainList, pentry->instNum, "domainList", APPROUTE_EXTRAMIB_PATH))) {
				if(*ptr != '\0') {
					snprintf(ipset_restore_file_path, sizeof(ipset_restore_file_path), "%s_dns_%d", APPROUTE_IPSET_RULE_FILE, pentry->instNum);
					if((fp = fopen(ipset_restore_file_path, "w"))) {
						fwrite(ptr, 1, strlen(ptr), fp);
						fclose(fp);
						snprintf(cmd, sizeof(cmd), "echo \"config_dns_set %s FILE %s\" > %s", dns_set_name, ipset_restore_file_path, APPCTL_PATH);
						va_cmd("/bin/sh", 2, 1, "-c", cmd);
						unlink(ipset_restore_file_path);
					}
				}
				else {
					snprintf(cmd, sizeof(cmd), "echo \"clear_dns_set %s\" > %s", dns_set_name, APPCTL_PATH);
					va_cmd("/bin/sh", 2, 1, "-c", cmd);
				}
				if(pval) free(pval);
			}
		}
	}
	
	if(act == 0)
	{
		fprintf(fp1, "%s -t %s %s %s\n", IPTABLES, "mangle", "-N", action_chain_name);
		fprintf(fp1, "%s -t %s %s %s\n", IPTABLES, "mangle", "-F", action_chain_name);
#ifdef CONFIG_IPV6
		fprintf(fp1, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-N", action_chain_name);
		fprintf(fp1, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-F", action_chain_name);
#endif

		if(pentry->ifName[0]) rtk_wan_sockmark_tblid_get(pentry->ifName, &wan_mark, &wan_mask, NULL, NULL);
		fw_mark = wan_mark | SOCK_MARK_APPROUTE_ACT_SET(0,1);
		fw_mask = wan_mask | SOCK_MARK_APPROUTE_ACT_BIT_MASK;
		
		fw_mark2 = fw_mask2 = 0;
		if((pentry->dscpMarkValue>=0) && (pentry->dscpMarkValue<=63)) {
			fw_mark2 |= SOCK_MARK2_DSCP_SET(0,pentry->dscpMarkValue)|SOCK_MARK2_DSCP_REMARK_ENABLE_SET(0,1);
			fw_mask2 |= SOCK_MARK2_DSCP_BIT_MASK|SOCK_MARK2_DSCP_REMARK_ENABLE_MASK;
		}
		if((pentry->dscpMarkValue_down>=0) && (pentry->dscpMarkValue_down<=63)) {
			fw_mark2 |= SOCK_MARK2_DSCP_DOWN_SET(0,pentry->dscpMarkValue_down)|SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_SET(0,1);
			fw_mask2 |= SOCK_MARK2_DSCP_DOWN_BIT_MASK|SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_MASK;
		}
		
		fprintf(fp1, "%s -t %s %s %s -j MARK --set-mark 0x%x/0x%x\n", IPTABLES, "mangle", FW_ADD, action_chain_name, fw_mark, fw_mask);
		fprintf(fp1, "%s -t %s %s %s -j MARK2 --set-mark2 0x%llx/0x%llx\n", IPTABLES, "mangle", FW_ADD, action_chain_name, fw_mark2, fw_mask2);
		fprintf(fp1, "%s -t %s %s %s -j RETURN\n", IPTABLES, "mangle", FW_ADD, action_chain_name);
#ifdef CONFIG_IPV6
		fprintf(fp1, "%s -t %s %s %s -j MARK --set-mark 0x%x/0x%x\n", IP6TABLES, "mangle", FW_ADD, action_chain_name, fw_mark, fw_mask);
		fprintf(fp1, "%s -t %s %s %s -j MARK2 --set-mark2 0x%llx/0x%llx\n", IP6TABLES, "mangle", FW_ADD, action_chain_name, fw_mark2, fw_mask2);
		fprintf(fp1, "%s -t %s %s %s -j RETURN\n", IP6TABLES, "mangle", FW_ADD, action_chain_name);
#endif
	}
	
	if(act == 0)
		snprintf(actstr, sizeof(actstr), "-A");
	else
		snprintf(actstr, sizeof(actstr), "-D");

	snprintf(action_str, sizeof(action_str), "-g %s", action_chain_name);
	
	if(pentry->matchType==0)
		strcpy(chain_name, NF_APPROUTE_CHAIN_MAC_IP);
	else if(pentry->matchType == 2)
		strcpy(chain_name, NF_APPROUTE_CHAIN_MAC);
	else 
		strcpy(chain_name, NF_APPROUTE_CHAIN_IP);
	
	src_mac_match_str[0] = '\0';
	if((pentry->matchType == 0 || pentry->matchType == 2) && pentry->macList[0]) 
		snprintf(src_mac_match_str, sizeof(src_mac_match_str), "-m set --match-set %s src", ipset_smac_chain_name);
	
	if(pentry->matchType == 0 || pentry->matchType == 1) {
		//default rules, if the IP and domain not set
		if(!pentry->destIpList[0] && !pentry->domainList[0]) {
			ip_match_str[0] = '\0';
			fprintf(fp1, "%s -t %s %s %s %s %s %s\n", IPTABLES, "mangle", actstr, chain_name, src_mac_match_str, ip_match_str, action_str);
#ifdef CONFIG_IPV6
			fprintf(fp1, "%s -t %s %s %s %s %s %s\n", IP6TABLES, "mangle", actstr, chain_name, src_mac_match_str, ip_match_str, action_str);
#endif
		}
		else {
			if(pentry->domainList[0]) {
				snprintf(ip_match_str, sizeof(ip_match_str), "-p udp --dport 53 -m appctl --dns --qname_set %s --rmatch", dns_set_name);
				fprintf(fp1, "%s -t %s %s %s %s %s %s\n", IPTABLES, "mangle", actstr, chain_name, src_mac_match_str, ip_match_str, action_str);
#ifdef CONFIG_IPV6
				fprintf(fp1, "%s -t %s %s %s %s %s %s\n", IP6TABLES, "mangle", actstr, chain_name, src_mac_match_str, ip_match_str, action_str);
#endif
				snprintf(ip_match_str, sizeof(ip_match_str), "-m set --match-set %s dst", ipset_dns_chain_name);
				fprintf(fp1, "%s -t %s %s %s %s %s %s\n", IPTABLES, "mangle", actstr, chain_name, src_mac_match_str, ip_match_str, action_str);
#ifdef CONFIG_IPV6
				snprintf(ip_match_str, sizeof(ip_match_str), "-m set --match-set %s dst", ipset_dnsv6_chain_name);
				fprintf(fp1, "%s -t %s %s %s %s %s %s\n", IP6TABLES, "mangle", actstr, chain_name, src_mac_match_str, ip_match_str, action_str);
#endif
			}
			if(pentry->destIpList[0]) {
				snprintf(ip_match_str, sizeof(ip_match_str), "-m set --match-set %s dst", ipset_subnet_chain_name);
				fprintf(fp1, "%s -t %s %s %s %s %s %s\n", IPTABLES, "mangle", actstr, chain_name, src_mac_match_str, ip_match_str, action_str);
#ifdef CONFIG_IPV6
				snprintf(ip_match_str, sizeof(ip_match_str), "-m set --match-set %s dst", ipset_subnetv6_chain_name);
				fprintf(fp1, "%s -t %s %s %s %s %s %s\n", IP6TABLES, "mangle", actstr, chain_name, src_mac_match_str, ip_match_str, action_str);
#endif
			}
		}
	} 
	else { 
		ip_match_str[0] = '\0';
		fprintf(fp1, "%s -t %s %s %s %s %s %s\n", IPTABLES, "mangle", actstr, chain_name, src_mac_match_str, ip_match_str, action_str);
#ifdef CONFIG_IPV6
		fprintf(fp1, "%s -t %s %s %s %s %s %s\n", IP6TABLES, "mangle", actstr, chain_name, src_mac_match_str, ip_match_str, action_str);
#endif
	}
	
	//DNS
	if((pentry->matchType == 0 || pentry->matchType == 1) && pentry->domainList[0]) {
		snprintf(ip_match_str, sizeof(ip_match_str), "--qname_set %s --rmatch", dns_set_name);
		snprintf(action_str, sizeof(action_str), "-j APPCTL --dns-saveip %s,%s --target CONTINUE", ipset_dns_chain_name, ipset_dnsv6_chain_name);
		src_mac_match_str[0] = '\0';
		if((pentry->matchType == 0) && pentry->macList[0]) 
			snprintf(src_mac_match_str, sizeof(src_mac_match_str), "--match-set %s dst", ipset_smac_chain_name);
		
		fprintf(fp1, "%s -t %s %s %s %s %s %s\n", EBTABLES, "filter", actstr, NF_APPROUTE_CHAIN_OUTPUT, src_mac_match_str, ip_match_str, action_str);
	}
	
	if(act)
	{
		fprintf(fp1, "%s -t %s %s %s\n", IPTABLES, "mangle", "-F", action_chain_name);
		fprintf(fp1, "%s -t %s %s %s\n", IPTABLES, "mangle", "-X", action_chain_name);
#ifdef CONFIG_IPV6
		fprintf(fp1, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-F", action_chain_name);
		fprintf(fp1, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-X", action_chain_name);
#endif
		fprintf(fp1, "echo \"clear_dns_set %s\" > %s\n", dns_set_name, APPCTL_PATH);
		fprintf(fp1, "ipset destroy %s\n", ipset_smac_chain_name);
		fprintf(fp1, "ipset destroy %s\n", ipset_subnet_chain_name);
		fprintf(fp1, "ipset destroy %s\n", ipset_dns_chain_name);
#ifdef CONFIG_IPV6
		fprintf(fp1, "ipset destroy %s\n", ipset_subnetv6_chain_name);
		fprintf(fp1, "ipset destroy %s\n", ipset_dnsv6_chain_name);
#endif
	}
	fclose(fp1);
	va_cmd_no_error("/bin/sh", 1, 1, path);
	unlink(path);

	return 0;

error:
	if(fp1) fclose(fp1);
	if(fp) fclose(fp);
	return -1;
}

int approute_flush_conntrack_mark(void)
{
	unsigned int fw_mask;
	char cmd[128];
	fw_mask = SOCK_MARK_APPROUTE_ACT_BIT_MASK | SOCK_MARK_WAN_INDEX_BIT_MASK;
	snprintf(cmd, sizeof(cmd), "conntrack -U -m 0/0x%x &> /dev/null", fw_mask);
	va_cmd_no_error("/bin/sh", 2, 1, "-c", cmd);
	//Delete ICMP, because the ICMP maybe not update SKBMARK by previous command
	snprintf(cmd, sizeof(cmd), "conntrack -D -p icmp &> /dev/null");
	va_cmd_no_error("/bin/sh", 2, 1, "-c", cmd);
	return 0;
}

static int config_approute_intf_rule(int action, char *ifname, int table_id, unsigned int fw_mark, unsigned int fw_mask) 
{
	char cmd[256];
	if(ifname && *ifname && fw_mask && table_id > 0) {
		sprintf(cmd, "/bin/ip route flush table %d", table_id);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		sprintf(cmd, "/bin/ip rule del fwmark 0x%x/0x%x pref %d table %d", fw_mark, fw_mask, IP_RULE_PRI_POLICYRT, table_id);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		if(action && is_interface_run(ifname)) {
			sprintf(cmd, "/bin/ip route add default dev %s table %d", ifname, table_id);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
			sprintf(cmd, "/bin/ip rule add fwmark 0x%x/0x%x pref %d table %d", fw_mark, fw_mask, IP_RULE_PRI_POLICYRT, table_id);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
		}
		return 0;
	}
	return -1;
}
// action=> 0:remove, 1:add, 2:update
int check_approute_intf(APPROUTE_RULE_T *pentry, int action)
{
	unsigned int fw_mark, fw_mask, type;
	int table_id, if_index;
	if(!pentry) return -1;
	if(pentry->ifName[0])
	{
		if(rtk_wan_sockmark_tblid_get(pentry->ifName, &fw_mark, &fw_mask, &table_id, &type) < 0) {
			if(action == 1) {
				pentry->ifIndex = TO_IFINDEX(MEDIA_APP, VC_INDEX(pentry->ifName[0]+pentry->ifName[1]+pentry->ifName[2]+pentry->ifName[3]+pentry->ifName[4]), VC_INDEX(pentry->ifName[strlen(pentry->ifName)-1]));
				if(rtk_wan_sockmark_add(pentry->ifName, &fw_mark, &fw_mask, WANMARK_TYPE_APP) == 0) {
					table_id = caculate_tblid_by_ifname(pentry->ifName);
					config_approute_intf_rule(1, pentry->ifName, table_id, fw_mark, fw_mask);
				}
			}
		}
		else {
			if(action == 1) {
				rtk_wan_sockmark_add(pentry->ifName, &fw_mark, &fw_mask, WANMARK_TYPE_APP); //add ref_count of ifname
			}
			else if(action == 2) {
				if(type == WANMARK_TYPE_APP)
					config_approute_intf_rule(1, pentry->ifName, table_id, fw_mark, fw_mask);
			}
			else if(action == 0) {
				if(rtk_wan_sockmark_remove(pentry->ifName, WANMARK_TYPE_APP) == 0 && type == WANMARK_TYPE_APP) 
					config_approute_intf_rule(0, pentry->ifName, table_id, fw_mark, fw_mask);
			}
		}
	}
	return 0;
}

int update_approute(char *ifname, unsigned int ifi_flags)
{
	APPROUTE_RULE_T entry;
	int num,i,config_rule;

	if (ifname == NULL)
	{
		printf("%s-%d: ifname is NULL.\n", __func__, __LINE__);
		return -1;
	}
	
	num = mib_chain_total(MIB_APPROUTE_TBL);
	for(i=0; i<num; i++)
	{
		if(!mib_chain_get(MIB_APPROUTE_TBL, i, &entry))
			continue;
		if(!entry.enable)
			continue;
		if(strcmp(ifname, entry.ifName))
			continue;
		check_approute_intf(&entry, 2);
		break;
	}
	return 0;
}

int apply_approute(int isBoot)
{
	unsigned int fw_mark = 0, fw_mask = 0;
	unsigned long long fw_mark2=0, fw_mask2=0;
	APPROUTE_RULE_T entry;
	int num,i;
	char cmd[256];
	FILE *fp;
	char path[64];
	
	sprintf(path, "%s_def", APPROUTE_IPSET_RULE_FILE);
	if((fp = fopen(path, "w"))) {
		fprintf(fp, "%s -t %s %s %s -j %s\n", IPTABLES, "mangle", FW_DEL, FW_PREROUTING, NF_APPROUTE_CHAIN);
		fprintf(fp, "%s -t %s %s %s -j %s\n", IPTABLES, "mangle", FW_DEL, FW_FORWARD, NF_APPROUTE_CHAIN_FWD);
		fprintf(fp, "%s -t %s %s %s -j %s\n", IPTABLES, "mangle", FW_DEL, FW_POSTROUTING, NF_APPROUTE_CHAIN_POST);
		fprintf(fp, "%s -t %s %s %s --logical-out %s -p ip --ip-proto udp --ip-sport 53 -j %s\n", EBTABLES, "filter", FW_DEL, FW_OUTPUT, BRIF, NF_APPROUTE_CHAIN_OUTPUT);
#ifdef CONFIG_IPV6
		fprintf(fp, "%s -t %s %s %s -j %s\n", IP6TABLES, "mangle", FW_DEL, FW_PREROUTING, NF_APPROUTE_CHAIN);
		fprintf(fp, "%s -t %s %s %s -j %s\n", IP6TABLES, "mangle", FW_DEL, FW_FORWARD, NF_APPROUTE_CHAIN_FWD);
		fprintf(fp, "%s -t %s %s %s -j %s\n", IP6TABLES, "mangle", FW_DEL, FW_POSTROUTING, NF_APPROUTE_CHAIN_POST);
		fprintf(fp, "%s -t %s %s %s --logical-out %s -p ip6 --ip6-proto udp --ip6-sport 53 -j %s\n", EBTABLES, "filter", FW_DEL, FW_OUTPUT, BRIF, NF_APPROUTE_CHAIN_OUTPUT);
#endif
		if(isBoot != 2) 
		{
			// PREROUTING
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-F", NF_APPROUTE_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-X", NF_APPROUTE_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-N", NF_APPROUTE_CHAIN);
			fprintf(fp, "%s -t %s %s %s -m pkttype ! --pkt-type unicast -j RETURN\n", IPTABLES, "mangle", FW_ADD, NF_APPROUTE_CHAIN);
			fprintf(fp, "%s -t %s %s %s -m connmark --mark 0x%x/0x%x -j CONNMARK --restore-mark --mask 0x%x\n", 
										IPTABLES, "mangle", FW_ADD, NF_APPROUTE_CHAIN, SOCK_MARK_APPROUTE_ACT_SET(0,1), 
										SOCK_MARK_APPROUTE_ACT_BIT_MASK, (SOCK_MARK_APPROUTE_ACT_BIT_MASK | SOCK_MARK_WAN_INDEX_BIT_MASK));
			fprintf(fp, "%s -t %s %s %s -m mark --mark 0x%x/0x%x -j RETURN\n", IPTABLES, "mangle", FW_ADD, 
										NF_APPROUTE_CHAIN, SOCK_MARK_APPROUTE_ACT_SET(0,1), SOCK_MARK_APPROUTE_ACT_BIT_MASK);
			// MAC filter
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-F", NF_APPROUTE_CHAIN_MAC);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-X", NF_APPROUTE_CHAIN_MAC);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-N", NF_APPROUTE_CHAIN_MAC);
			fprintf(fp, "%s -t %s %s %s -i %s -m mark --mark 0x0/0x%x -j %s\n", IPTABLES, "mangle", FW_ADD, 
										NF_APPROUTE_CHAIN, LANIF_ALL, SOCK_MARK_APPROUTE_ACT_BIT_MASK, NF_APPROUTE_CHAIN_MAC);
			// IP+MAC filter
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-F", NF_APPROUTE_CHAIN_MAC_IP);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-X", NF_APPROUTE_CHAIN_MAC_IP);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-N", NF_APPROUTE_CHAIN_MAC_IP);
			fprintf(fp, "%s -t %s %s %s -i %s -m mark --mark 0x0/0x%x -j %s\n", IPTABLES, "mangle", FW_ADD, 
										NF_APPROUTE_CHAIN, LANIF_ALL, SOCK_MARK_APPROUTE_ACT_BIT_MASK, NF_APPROUTE_CHAIN_MAC_IP);		
			// IP filter
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-F", NF_APPROUTE_CHAIN_IP);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-X", NF_APPROUTE_CHAIN_IP);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-N", NF_APPROUTE_CHAIN_IP);
			fprintf(fp, "%s -t %s %s %s -i %s -m mark --mark 0x0/0x%x -j %s\n", IPTABLES, "mangle", FW_ADD, 
										NF_APPROUTE_CHAIN, LANIF_ALL, SOCK_MARK_APPROUTE_ACT_BIT_MASK, NF_APPROUTE_CHAIN_IP);
#ifdef CONFIG_IPV6
			// IP6 PREROUTING
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-F", NF_APPROUTE_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-X", NF_APPROUTE_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-N", NF_APPROUTE_CHAIN);
			fprintf(fp, "%s -t %s %s %s -m pkttype ! --pkt-type unicast -j RETURN\n", IP6TABLES, "mangle", FW_ADD, NF_APPROUTE_CHAIN);
			fprintf(fp, "%s -t %s %s %s -m connmark --mark 0x%x/0x%x -j CONNMARK --restore-mark --mask 0x%x\n", 
										IP6TABLES, "mangle", FW_ADD, NF_APPROUTE_CHAIN, SOCK_MARK_APPROUTE_ACT_SET(0,1), 
										SOCK_MARK_APPROUTE_ACT_BIT_MASK, (SOCK_MARK_APPROUTE_ACT_BIT_MASK | SOCK_MARK_WAN_INDEX_BIT_MASK));
			fprintf(fp, "%s -t %s %s %s -m mark --mark 0x%x/0x%x -j RETURN\n", IP6TABLES, "mangle", FW_ADD, 
										NF_APPROUTE_CHAIN, SOCK_MARK_APPROUTE_ACT_SET(0,1), SOCK_MARK_APPROUTE_ACT_BIT_MASK);
			// IP6 MAC filter
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-F", NF_APPROUTE_CHAIN_MAC);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-X", NF_APPROUTE_CHAIN_MAC);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-N", NF_APPROUTE_CHAIN_MAC);
			fprintf(fp, "%s -t %s %s %s -i %s -m mark --mark 0x0/0x%x -j %s\n", IP6TABLES, "mangle", FW_ADD, 
										NF_APPROUTE_CHAIN, LANIF_ALL, SOCK_MARK_APPROUTE_ACT_BIT_MASK, NF_APPROUTE_CHAIN_MAC);
			// IP6 IP+MAC filter
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-F", NF_APPROUTE_CHAIN_MAC_IP);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-X", NF_APPROUTE_CHAIN_MAC_IP);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-N", NF_APPROUTE_CHAIN_MAC_IP);
			fprintf(fp, "%s -t %s %s %s -i %s -m mark --mark 0x0/0x%x -j %s\n", IP6TABLES, "mangle", FW_ADD, 
										NF_APPROUTE_CHAIN, LANIF_ALL, SOCK_MARK_APPROUTE_ACT_BIT_MASK, NF_APPROUTE_CHAIN_MAC_IP);		
			// IP6 IP filter
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-F", NF_APPROUTE_CHAIN_IP);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-X", NF_APPROUTE_CHAIN_IP);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-N", NF_APPROUTE_CHAIN_IP);
			fprintf(fp, "%s -t %s %s %s -i %s -m mark --mark 0x0/0x%x -j %s\n", IP6TABLES, "mangle", FW_ADD, 
										NF_APPROUTE_CHAIN, LANIF_ALL, SOCK_MARK_APPROUTE_ACT_BIT_MASK, NF_APPROUTE_CHAIN_IP);
#endif

			// FORWARD
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-F", NF_APPROUTE_CHAIN_FWD);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-X", NF_APPROUTE_CHAIN_FWD);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-N", NF_APPROUTE_CHAIN_FWD);
			// for upstream DSCP LOG
			fprintf(fp, "%s -t %s %s %s -i %s -m mark --mark 0x%x/0x%x -m mark2 ! --mark2 0/0x%llx -j APPCTL --log-feature DSCP\n", 
										IPTABLES, "mangle", FW_ADD, NF_APPROUTE_CHAIN_FWD, BRIF, SOCK_MARK_APPROUTE_ACT_SET(0,1), SOCK_MARK_APPROUTE_ACT_BIT_MASK, 
										(SOCK_MARK2_DSCP_REMARK_ENABLE_MASK));
			// for downstream DSCP LOG
			fprintf(fp, "%s -t %s %s %s -o %s -m mark --mark 0x%x/0x%x -m mark2 ! --mark2 0/0x%llx -j APPCTL --log-feature DSCP\n", 
										IPTABLES, "mangle", FW_ADD, NF_APPROUTE_CHAIN_FWD, BRIF, SOCK_MARK_APPROUTE_ACT_SET(0,1), SOCK_MARK_APPROUTE_ACT_BIT_MASK, 
										(SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_MASK));
			fprintf(fp, "%s -t %s %s %s -i %s -m mark --mark 0x%x/0x%x -j APPCTL --log-feature PROUTE\n", 
										IPTABLES, "mangle", FW_ADD, NF_APPROUTE_CHAIN_FWD, BRIF, SOCK_MARK_APPROUTE_ACT_SET(0,1), SOCK_MARK_APPROUTE_ACT_BIT_MASK);
#ifdef CONFIG_IPV6
			// IP6 FORWARD
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-F", NF_APPROUTE_CHAIN_FWD);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-X", NF_APPROUTE_CHAIN_FWD);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-N", NF_APPROUTE_CHAIN_FWD);
			// for upstream DSCP LOG
			fprintf(fp, "%s -t %s %s %s -i %s -m mark --mark 0x%x/0x%x -m mark2 ! --mark2 0/0x%llx -j APPCTL --log-feature DSCP\n", 
										IP6TABLES, "mangle", FW_ADD, NF_APPROUTE_CHAIN_FWD, BRIF, SOCK_MARK_APPROUTE_ACT_SET(0,1), SOCK_MARK_APPROUTE_ACT_BIT_MASK, 
										(SOCK_MARK2_DSCP_REMARK_ENABLE_MASK));
			// for downstream DSCP LOG
			fprintf(fp, "%s -t %s %s %s -o %s -m mark --mark 0x%x/0x%x -m mark2 ! --mark2 0/0x%llx -j APPCTL --log-feature DSCP\n", 
										IP6TABLES, "mangle", FW_ADD, NF_APPROUTE_CHAIN_FWD, BRIF, SOCK_MARK_APPROUTE_ACT_SET(0,1), SOCK_MARK_APPROUTE_ACT_BIT_MASK, 
										(SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_MASK));
			fprintf(fp, "%s -t %s %s %s -i %s -m mark --mark 0x%x/0x%x -j APPCTL --log-feature PROUTE\n", 
										IP6TABLES, "mangle", FW_ADD, NF_APPROUTE_CHAIN_FWD, BRIF, SOCK_MARK_APPROUTE_ACT_SET(0,1), SOCK_MARK_APPROUTE_ACT_BIT_MASK);
#endif
			
			// IP6 POSTROUTING
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-F", NF_APPROUTE_CHAIN_POST);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-X", NF_APPROUTE_CHAIN_POST);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-N", NF_APPROUTE_CHAIN_POST);
			fprintf(fp, "%s -t %s %s %s -m connmark --mark 0x%x/0x%x -m mark --mark 0x%x/0x%x -j CONNMARK --save-mark --mask 0x%x\n", 
										IPTABLES, "mangle", FW_ADD, NF_APPROUTE_CHAIN_POST, SOCK_MARK_APPROUTE_ACT_SET(0,0), SOCK_MARK_APPROUTE_ACT_BIT_MASK, 
										SOCK_MARK_APPROUTE_ACT_SET(0,1), SOCK_MARK_APPROUTE_ACT_BIT_MASK, (SOCK_MARK_APPROUTE_ACT_BIT_MASK | SOCK_MARK_WAN_INDEX_BIT_MASK));
#ifdef CONFIG_IPV6
			// IP6 POSTROUTING
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-F", NF_APPROUTE_CHAIN_POST);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-X", NF_APPROUTE_CHAIN_POST);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-N", NF_APPROUTE_CHAIN_POST);
			fprintf(fp, "%s -t %s %s %s -m connmark --mark 0x%x/0x%x -m mark --mark 0x%x/0x%x -j CONNMARK --save-mark --mask 0x%x\n", 
										IP6TABLES, "mangle", FW_ADD, NF_APPROUTE_CHAIN_POST, SOCK_MARK_APPROUTE_ACT_SET(0,0), SOCK_MARK_APPROUTE_ACT_BIT_MASK, 
										SOCK_MARK_APPROUTE_ACT_SET(0,1), SOCK_MARK_APPROUTE_ACT_BIT_MASK, (SOCK_MARK_APPROUTE_ACT_BIT_MASK | SOCK_MARK_WAN_INDEX_BIT_MASK));
#endif
			
			// Ebtables DNS rule
			fprintf(fp, "%s -t %s %s %s\n", EBTABLES, "filter", "-F", NF_APPROUTE_CHAIN_OUTPUT);
			fprintf(fp, "%s -t %s %s %s\n", EBTABLES, "filter", "-X", NF_APPROUTE_CHAIN_OUTPUT);
			fprintf(fp, "%s -t %s %s %s\n", EBTABLES, "filter", "-N", NF_APPROUTE_CHAIN_OUTPUT);
			fprintf(fp, "%s -t %s %s %s %s\n", EBTABLES, "filter", "-P", NF_APPROUTE_CHAIN_OUTPUT, "RETURN");
		}
		
		fprintf(fp, "%s -t %s %s %s -j %s\n", IPTABLES, "mangle", FW_ADD, FW_PREROUTING, NF_APPROUTE_CHAIN);
		fprintf(fp, "%s -t %s %s %s -j %s\n", IPTABLES, "mangle", FW_ADD, FW_FORWARD, NF_APPROUTE_CHAIN_FWD);
		fprintf(fp, "%s -t %s %s %s -j %s\n", IPTABLES, "mangle", FW_ADD, FW_POSTROUTING, NF_APPROUTE_CHAIN_POST);
		fprintf(fp, "%s -t %s %s %s --logical-out %s -p ip --ip-proto udp --ip-sport 53 -j %s\n", EBTABLES, "filter", FW_ADD, FW_OUTPUT, BRIF, NF_APPROUTE_CHAIN_OUTPUT);
#ifdef CONFIG_IPV6
		fprintf(fp, "%s -t %s %s %s -j %s\n", IP6TABLES, "mangle", FW_ADD, FW_PREROUTING, NF_APPROUTE_CHAIN);
		fprintf(fp, "%s -t %s %s %s -j %s\n", IP6TABLES, "mangle", FW_ADD, FW_FORWARD, NF_APPROUTE_CHAIN_FWD);
		fprintf(fp, "%s -t %s %s %s -j %s\n", IP6TABLES, "mangle", FW_ADD, FW_POSTROUTING, NF_APPROUTE_CHAIN_POST);
		fprintf(fp, "%s -t %s %s %s --logical-out %s -p ip6 --ip6-proto udp --ip6-sport 53 -j %s\n", EBTABLES, "filter", FW_ADD, FW_OUTPUT, BRIF, NF_APPROUTE_CHAIN_OUTPUT);
#endif
		fclose(fp);
		va_cmd_no_error("/bin/sh", 1, 1, path);
		unlink(path);
	}

	if(isBoot != 2) 
	{
		num = mib_chain_total(MIB_APPROUTE_TBL);
		for(i=0; i<num; i++)
		{
			if(!mib_chain_get(MIB_APPROUTE_TBL, i, &entry))
				continue;
			if(!entry.enable)
				continue;
			if(isBoot==1)
			{
				check_approute_intf(&entry, 1);
			}
			config_one_approute(0, &entry, isBoot);
		}
	}
	return 0;
}

#ifdef SUPPORT_APPFILTER
const char APPFILTER_CHAIN[]="appfilter";
const char APPFILTER_CHAIN_POST[]="appfilter_post";
const char APPFILTER_CHAIN_OUTPUT[]="appfilter_output";
const char APPFILTER_CHAIN_ACT[]="appfilter_ACT_%d";
const char APPFILTER_CHAIN_DNS_ACT[]="appfilter_DNS_ACT_%d";
const char APPFILTER_DNS_SET_CHAIN[] = "appfilter_dns_%d";
const char APPFILTER_IPSET_IPNET_CHAIN[] = "appfilter_set_ip_%d";
const char APPFILTER_IPSET_IPV6NET_CHAIN[] = "appfilter_set_ip6_%d";
const char APPFILTER_IPSET_DNS_CHAIN[] = "appfilter_set_dns_%d";
const char APPFILTER_IPSET_DNS6_CHAIN[] = "appfilter_set_dns6_%d";
const char APPFILTER_IPSET_SMAC_CHAIN[] = "appfilter_mac_%d";
const char APPFILTER_IPSET_RULE_FILE[]="/tmp/appfilter_rule";

int getAppFilterTime(MIB_CE_APP_FILTER_Tp pEntry, char *buff)
{
	if((pEntry == NULL) || (buff == NULL))
		return 0;

	if((pEntry->end_hr2 == 0) && (pEntry->end_min2 == 0))
	{
		sprintf(buff, "%02hhu:%02hhu-%02hhu:%02hhu", pEntry->start_hr1, pEntry->start_min1, pEntry->end_hr1, pEntry->end_min1);
	}
	else if((pEntry->end_hr3 == 0) && (pEntry->end_min3 == 0))
	{
		sprintf(buff, "%02hhu:%02hhu-%02hhu:%02hhu, %02hhu:%02hhu-%02hhu:%02hhu", pEntry->start_hr1, pEntry->start_min1, pEntry->end_hr1, 
				pEntry->end_min1, pEntry->start_hr2, pEntry->start_min2, pEntry->end_hr2, pEntry->end_min2);
	}
	else
	{
		sprintf(buff, "%02hhu:%02hhu-%02hhu:%02hhu, %02hhu:%02hhu-%02hhu:%02hhu, %02hhu:%02hhu-%02hhu:%02hhu", pEntry->start_hr1, 
				pEntry->start_min1, pEntry->end_hr1, pEntry->end_min1, pEntry->start_hr2, pEntry->start_min2, pEntry->end_hr2,
					 pEntry->end_min2, pEntry->start_hr3, pEntry->start_min3, pEntry->end_hr3, pEntry->end_min3);
	}

	return 1;
}

unsigned int getAppFilterBlockedTimes(int inst_num)
{
	char cmd[256], chain_name[32];
	int blocktimes=0, len;
	FILE *fp;

	snprintf(chain_name, sizeof(chain_name), APPFILTER_CHAIN_ACT, inst_num);
	snprintf(cmd, sizeof(cmd), "%s -t %s -L %s -vn | grep \"APPCTL.*DROP\" | awk 'BEGIN {count=0;} { count+=$1;} END{print count}'", IPTABLES, "mangle", chain_name);
	if((fp=popen(cmd, "r"))){
		len = fread(cmd, 1, sizeof(cmd)-1, fp);
		if(len > 0) {
			cmd[len] = '\0';
			blocktimes += atoi(cmd);
		}
		pclose(fp);
	}
#ifdef CONFIG_IPV6
	snprintf(cmd, sizeof(cmd), "%s -t %s -L %s -vn | grep \"APPCTL.*DROP\" | awk 'BEGIN {count=0;} { count+=$1;} END{print count}'", IP6TABLES, "mangle", chain_name);
	if((fp=popen(cmd, "r"))){
		len = fread(cmd, 1, sizeof(cmd)-1, fp);
		if(len > 0) {
			cmd[len] = '\0';
			blocktimes += atoi(cmd);
		}
		pclose(fp);
	}
#endif

	snprintf(chain_name, sizeof(chain_name), APPFILTER_CHAIN_DNS_ACT, inst_num);
	snprintf(cmd, sizeof(cmd), "%s -t %s -L %s --Lc | sed -n 's/.*APPCTL.*DROP.*pcnt = \\([0-9]*\\).*/\\1/p'", EBTABLES, "filter", chain_name);
	if((fp=popen(cmd, "r"))){
		len = fread(cmd, 1, sizeof(cmd)-1, fp);
		if(len > 0) {
			cmd[len] = '\0';
			blocktimes += atoi(cmd);
		}
		pclose(fp);
	}

	return blocktimes;
}

//act 0:add 1:del
int config_one_appfilter_with_type(int act, MIB_CE_APP_FILTER_T *pentry, int type, char **diff_array)
{
	int i = 0;
	FILE *fp = NULL;
	char *act_str;
	char cmd[128];
	char file_path[64];
	char ipset_name[32];
#ifdef CONFIG_IPV6
	char ipset_v6_name[32];
#endif

	if(pentry == NULL || diff_array == NULL || diff_array[0] == NULL)
		return 0;
	
	snprintf(file_path, sizeof(file_path), "%s_set_%d_type_%d", APPFILTER_IPSET_RULE_FILE, pentry->inst_num, type);
	if(!(fp = fopen(file_path, "w")))
	{
		fprintf(stderr,"[%s %d] open file %s fail\n",__func__,__LINE__,file_path);
		goto error;
	}

	switch(type) {
		case 0:  {// ip
			act_str = (act == 0) ? "add" : "del";
			snprintf(ipset_name, sizeof(ipset_name), APPFILTER_IPSET_IPNET_CHAIN, pentry->inst_num);
#ifdef CONFIG_IPV6
			snprintf(ipset_v6_name, sizeof(ipset_v6_name), APPFILTER_IPSET_IPV6NET_CHAIN, pentry->inst_num);
#endif
			while(diff_array[i]) {
				if(!strchr(diff_array[i], ':'))
					fprintf(fp, "%s %s %s\n", act_str, ipset_name, diff_array[i]);
#ifdef CONFIG_IPV6
				else
					fprintf(fp, "%s %s %s\n", act_str, ipset_v6_name, diff_array[i]);
#endif
				i++;
			}
			break;
		}
		case 1:  // domain
			//snprintf(ipset_name, sizeof(ipset_name), APPFILTER_DNS_SET_CHAIN, pentry->inst_num);
			while(diff_array[i]) {
				fprintf(fp, "%s,", diff_array[i]);
				i++;
			}
			break;
		case 2:  // mac 
			act_str = (act == 0) ? "add" : "del";
			snprintf(ipset_name, sizeof(ipset_name), APPFILTER_IPSET_SMAC_CHAIN, pentry->inst_num);
			while(diff_array[i]) {
				char *mac_p, macStr[18], c=0;
				if(!strchr(diff_array[i], ':')) {
					mac_p = macStr;
					while(c<12 && diff_array[i][c]) { 
						*(mac_p++) = diff_array[i][c];
						if(c&1) *(mac_p++) = ':';
						c++;
					}
					if(mac_p > macStr) *(mac_p-1) = '\0';
					mac_p = macStr;
				} else mac_p = diff_array[i];
				fprintf(fp, "%s %s %s\n", act_str, ipset_name, mac_p);
				i++;
			}
			break;
		default: 
			goto error;
	}
	
	fclose(fp); fp = NULL;
	if(type == 1) {
		snprintf(ipset_name, sizeof(ipset_name), APPFILTER_DNS_SET_CHAIN, pentry->inst_num);
		act_str = (act == 0) ? "config_dns_set" : "clear_dns_set";
		snprintf(cmd, sizeof(cmd), "echo \"%s %s FILE %s\" > %s", act_str, ipset_name, file_path, APPCTL_PATH);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
	}
	else {
		snprintf(cmd, sizeof(cmd), "%s %s -! -f %s", IPSET, IPSET_COMMAND_RESTORE, file_path);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
	}
	unlink(file_path);
	return 0;
	
error:
	if(fp) {
		fclose(fp);
		unlink(file_path);
	}
	return 1;
}

//act 0:add 1:del, cmode 2:only for iptables rule
int config_one_appfilter(int act, MIB_CE_APP_FILTER_T*pentry, int cmode)
{
	char cmd[512];
	char actstr[8];
	int i=0, j=0;
	char ipset_smac_chain_name[32], dns_set_name[32];
	char ipset_subnet_chain_name[32], ipset_dns_chain_name[32];
#ifdef CONFIG_IPV6
	char ipset_subnetv6_chain_name[32], ipset_dnsv6_chain_name[32];
#endif
	char action_chain_name[32], action_dns_chain_name[32];
	char *ptr,*tmp,*pval=NULL;
	FILE *fp = NULL, *fp1 = NULL;
	char ipset_restore_file_path[64], path[64];
	char src_mac_match_str[64];
	char time_weekday[64]={0};
	char time_hour_minitue[3][64]={0};
	char action_str[200],match_str[128];
	char ip_match_str[128];
	char *weekday_str[7]={"Mon,","Tue,","Wed,","Thu,","Fri,","Sat,","Sun,"};

	if(pentry->Enable==0)
		return 1;
	
	snprintf(path, sizeof(path), "%s_%d", APPFILTER_IPSET_RULE_FILE, pentry->inst_num);
	if(!(fp1 = fopen(path, "w"))){
		printf("[%s:%d] error to open file %s\n", __FUNCTION__, __LINE__, path);
		return -1;
	}
	snprintf(ipset_subnet_chain_name, sizeof(ipset_subnet_chain_name), APPFILTER_IPSET_IPNET_CHAIN, pentry->inst_num);
	snprintf(ipset_dns_chain_name, sizeof(ipset_dns_chain_name), APPFILTER_IPSET_DNS_CHAIN, pentry->inst_num);
#ifdef CONFIG_IPV6
	snprintf(ipset_subnetv6_chain_name, sizeof(ipset_subnetv6_chain_name), APPFILTER_IPSET_IPV6NET_CHAIN, pentry->inst_num);
	snprintf(ipset_dnsv6_chain_name, sizeof(ipset_dnsv6_chain_name), APPFILTER_IPSET_DNS6_CHAIN, pentry->inst_num);
#endif
	snprintf(ipset_smac_chain_name, sizeof(ipset_smac_chain_name), APPFILTER_IPSET_SMAC_CHAIN, pentry->inst_num);
	snprintf(dns_set_name, sizeof(dns_set_name), APPFILTER_DNS_SET_CHAIN, pentry->inst_num);
	snprintf(action_chain_name, sizeof(action_chain_name), APPFILTER_CHAIN_ACT, pentry->inst_num);
	snprintf(action_dns_chain_name, sizeof(action_dns_chain_name), APPFILTER_CHAIN_DNS_ACT, pentry->inst_num);
	
	if(act == 0 && cmode != 2)
	{
		snprintf(ipset_restore_file_path, sizeof(ipset_restore_file_path), "%s_set_%d", APPFILTER_IPSET_RULE_FILE, pentry->inst_num);
		if(!(fp = fopen(ipset_restore_file_path, "w")))
		{
			fprintf(stderr,"[%s %d] open file %s fail\n",__func__,__LINE__,ipset_restore_file_path);
			goto error;
		}

		fprintf(fp, "create %s %s\n",ipset_subnet_chain_name, IPSET_SETTYPE_HASHNET);
		fprintf(fp, "flush %s\n",ipset_subnet_chain_name);
		fprintf(fp, "create %s %s timeout 0 comment\n",ipset_dns_chain_name, IPSET_SETTYPE_HASHNET);
		fprintf(fp, "flush %s\n",ipset_dns_chain_name);
#ifdef CONFIG_IPV6
		fprintf(fp, "create %s %s\n",ipset_subnetv6_chain_name, IPSET_SETTYPE_HASHNETV6);
		fprintf(fp, "flush %s\n",ipset_subnetv6_chain_name);
		fprintf(fp, "create %s %s timeout 0 comment\n",ipset_dnsv6_chain_name, IPSET_SETTYPE_HASHNETV6);
		fprintf(fp, "flush %s\n",ipset_dnsv6_chain_name);
#endif
		if((ptr = _APP_GETEXTRAENTRY(&pval, pentry->destIPlist, pentry->inst_num, "destIpList", APPFILTER_EXTRAMIB_PATH))) {
			while(ptr && ( tmp= strsep(&ptr, ",;")) !=NULL)
			{
				if(!strchr(tmp, ':'))
					fprintf(fp, "add %s %s\n",ipset_subnet_chain_name, tmp);
#ifdef CONFIG_IPV6
				else
					fprintf(fp, "add %s %s\n",ipset_subnetv6_chain_name, tmp);
#endif
			}
			if(pval) free(pval);
		}

		if((ptr = _APP_GETEXTRAENTRY(&pval, pentry->maclist, pentry->inst_num, "macList", APPFILTER_EXTRAMIB_PATH))) {
			if(*ptr != '\0') {
				fprintf(fp, "create %s %s\n",ipset_smac_chain_name, IPSET_SETTYPE_HASHMAC);
				fprintf(fp, "flush %s\n",ipset_smac_chain_name);
				while(ptr && ( tmp= strsep(&ptr, ",;")) !=NULL) 
				{
					char *mac_p, macStr[18], c=0;
					if(!strchr(tmp, ':')) {
						mac_p = macStr;
						while(c<12 && tmp[c]) { 
							*(mac_p++) = tmp[c];
							if(c&1) *(mac_p++) = ':';
							c++;
						}
						if(mac_p > macStr) *(mac_p-1) = '\0';
						mac_p = macStr;
					} else mac_p = tmp;
					
					fprintf(fp, "add %s %s\n",ipset_smac_chain_name, mac_p);
				}
			}
			if(pval) free(pval);
		}

		fclose(fp); fp = NULL;
		snprintf(cmd, sizeof(cmd), "%s %s -! -f %s", IPSET, IPSET_COMMAND_RESTORE, ipset_restore_file_path);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		unlink(ipset_restore_file_path);
		
		if((ptr = _APP_GETEXTRAENTRY(&pval, pentry->domianlist, pentry->inst_num, "domainList", APPFILTER_EXTRAMIB_PATH))) {
			if(*ptr != '\0') {
				snprintf(ipset_restore_file_path, sizeof(ipset_restore_file_path), "%s_dns_%d", APPFILTER_IPSET_RULE_FILE, pentry->inst_num);
				if((fp = fopen(ipset_restore_file_path, "w"))) {
					fwrite(ptr, 1, strlen(ptr), fp);
					fclose(fp); 
					snprintf(cmd, sizeof(cmd), "echo \"config_dns_set %s FILE %s\" > %s", dns_set_name, ipset_restore_file_path, APPCTL_PATH);
					va_cmd("/bin/sh", 2, 1, "-c", cmd);
					unlink(ipset_restore_file_path);
				}
			}
			else {
				snprintf(cmd, sizeof(cmd), "echo \"clear_dns_set %s\" > %s", dns_set_name, APPCTL_PATH);
				va_cmd("/bin/sh", 2, 1, "-c", cmd);
			}
			if(pval) free(pval);
		}
	}
	
	if(act == 0) {
		if(pentry->WeekDays) {	
			strcat(time_weekday, "--weekdays ");
			for(i=0; i<7; i++)
			{
				if((1<<i) & pentry->WeekDays)
				{
					strcat(time_weekday, weekday_str[i]);				
				}
			}
			time_weekday[strlen(time_weekday)]='\0';
		}
		if(pentry->start_hr1 || pentry->start_min1 || pentry->end_hr1 || pentry->end_min1) {
			snprintf(time_hour_minitue[0], sizeof(time_hour_minitue[0]), "--timestart %02d:%02d --timestop %02d:%02d --kerneltz", pentry->start_hr1, pentry->start_min1, pentry->end_hr1, pentry->end_min1);
		}
		if(pentry->start_hr2 || pentry->start_min2 || pentry->end_hr2 || pentry->end_min2) {
			snprintf(time_hour_minitue[1], sizeof(time_hour_minitue[1]), "--timestart %02d:%02d --timestop %02d:%02d --kerneltz", pentry->start_hr2, pentry->start_min2, pentry->end_hr2, pentry->end_min2);
		}
		if(pentry->start_hr3 || pentry->start_min3 || pentry->end_hr3 || pentry->end_min3) {
			snprintf(time_hour_minitue[2], sizeof(time_hour_minitue[2]), "--timestart %02d:%02d --timestop %02d:%02d --kerneltz", pentry->start_hr3, pentry->start_min3, pentry->end_hr3, pentry->end_min3);
		}
		
		fprintf(fp1, "%s -t %s %s %s\n", IPTABLES, "mangle", "-N", action_chain_name);
		fprintf(fp1, "%s -t %s %s %s\n", IPTABLES, "mangle", "-F", action_chain_name);
		if(pentry->domianlist[0]) {
			fprintf(fp1, "%s -t %s %s %s\n", EBTABLES, "filter", "-N", action_dns_chain_name);
			fprintf(fp1, "%s -t %s %s %s\n", EBTABLES, "filter", "-F", action_dns_chain_name);
			fprintf(fp1, "%s -t %s %s %s %s\n", EBTABLES, "filter", "-P", action_dns_chain_name, "RETURN");
		}
#ifdef CONFIG_IPV6
		fprintf(fp1, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-N", action_chain_name);
		fprintf(fp1, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-F", action_chain_name);
#endif
		if(time_weekday[0] || time_hour_minitue[0][0] || time_hour_minitue[1][0] || time_hour_minitue[2][0]) {
			//clear match bit
			snprintf(action_str, sizeof(action_str), "-j MARK2 --set-mark2 0x%llx/0x%llx", SOCK_MARK2_APPROUTE_RULE_MATCH_SET(0,0), SOCK_MARK2_APPROUTE_RULE_MATCH_MASK);
			fprintf(fp1, "%s -t %s %s %s %s\n", IPTABLES, "mangle", FW_ADD, action_chain_name, action_str);
#ifdef CONFIG_IPV6
			fprintf(fp1, "%s -t %s %s %s %s\n", IP6TABLES, "mangle", FW_ADD, action_chain_name, action_str);
#endif
			if(pentry->domianlist[0]) {
				fprintf(fp1, "%s -t %s %s %s -j mark2 --mark2-and 0x%llx\n", EBTABLES, "filter", FW_ADD, action_dns_chain_name, ~(SOCK_MARK2_APPROUTE_RULE_MATCH_MASK));
			}
			
			snprintf(action_str, sizeof(action_str), "-j MARK2 --set-mark2 0x%llx/0x%llx", SOCK_MARK2_APPROUTE_RULE_MATCH_SET(0,1), SOCK_MARK2_APPROUTE_RULE_MATCH_MASK);
			for(i=0,j=0; i<3; i++) {
				if(!(time_hour_minitue[i][0])) {
					if(!((i+1) >= 3 && j == 0))
						continue;
				}
				fprintf(fp1, "%s -t %s %s %s -m time %s %s %s\n", IPTABLES, "mangle", FW_ADD, action_chain_name, time_weekday, time_hour_minitue[i], action_str);
#ifdef CONFIG_IPV6
				fprintf(fp1, "%s -t %s %s %s -m time %s %s %s\n", IP6TABLES, "mangle", FW_ADD, action_chain_name, time_weekday, time_hour_minitue[i], action_str);
#endif
				if(pentry->domianlist[0]) {
					fprintf(fp1, "%s -t %s %s %s %s %s -j mark2 --mark2-or 0x%llx\n", EBTABLES, "filter", FW_ADD, action_dns_chain_name, time_weekday, time_hour_minitue[i], SOCK_MARK2_APPROUTE_RULE_MATCH_SET(0,1));
				}
				j++;
			}

			snprintf(match_str, sizeof(match_str), "-m mark2 --mark2 0x%llx/0x%llx",
					SOCK_MARK2_APPROUTE_RULE_MATCH_SET(0,1), SOCK_MARK2_APPROUTE_RULE_MATCH_MASK);
		} else {
			match_str[0] = '\0';
		}
		
		// Default action
		action_str[0] = '\0';
		snprintf(action_str, sizeof(action_str), "-j APPCTL --log-feature DROP --target DROP");
		if(pentry->signalEnable){
			i = strlen(action_str);
			snprintf(action_str+i, sizeof(action_str)-i, " --sig \"%s\"", action_chain_name);
		}
		if(pentry->destIP_action) {
			i = strlen(action_str);
			snprintf(action_str+i, sizeof(action_str)-i, " --reject port-unreach,tcp-rst");
		}

		if(!pentry->destIPlist[0] && !pentry->domianlist[0]) {
			fprintf(fp1, "%s -t %s %s %s %s %s\n", IPTABLES, "mangle", FW_ADD, action_chain_name, 
													match_str, ((pentry->mode == 0)?action_str:"-j RETURN"));
#ifdef CONFIG_IPV6
			fprintf(fp1, "%s -t %s %s %s %s %s\n", IP6TABLES, "mangle", FW_ADD, action_chain_name, 
													match_str, ((pentry->mode == 0)?action_str:"-j RETURN"));
#endif
		}
		else {
			if(pentry->destIPlist[0]) {
				snprintf(ip_match_str, sizeof(ip_match_str), "-m set --match-set %s dst", ipset_subnet_chain_name);
				fprintf(fp1, "%s -t %s %s %s %s %s %s\n", IPTABLES, "mangle", FW_ADD, action_chain_name, 
														match_str, ip_match_str, ((pentry->mode == 0)?action_str:"-j RETURN"));
#ifdef CONFIG_IPV6
				snprintf(ip_match_str, sizeof(ip_match_str), "-m set --match-set %s dst", ipset_subnetv6_chain_name);
				fprintf(fp1, "%s -t %s %s %s %s %s %s\n", IP6TABLES, "mangle", FW_ADD, action_chain_name, 
														match_str, ip_match_str, ((pentry->mode == 0)?action_str:"-j RETURN"));
#endif
			}
			if(pentry->domianlist[0]) {
				snprintf(ip_match_str, sizeof(ip_match_str), "-m set --match-set %s dst", ipset_dns_chain_name);
				fprintf(fp1, "%s -t %s %s %s %s %s %s\n", IPTABLES, "mangle", FW_ADD, action_chain_name, 
														match_str, ip_match_str, ((pentry->mode == 0)?action_str:"-j RETURN"));
#ifdef CONFIG_IPV6
				snprintf(ip_match_str, sizeof(ip_match_str), "-m set --match-set %s dst", ipset_dnsv6_chain_name);
				fprintf(fp1, "%s -t %s %s %s %s %s %s\n", IP6TABLES, "mangle", FW_ADD, action_chain_name, 
														match_str, ip_match_str, ((pentry->mode == 0)?action_str:"-j RETURN"));
#endif
			}
		}
		if(pentry->mode == 1){
			fprintf(fp1, "%s -t %s %s %s %s\n", IPTABLES, "mangle", FW_ADD, action_chain_name, action_str);
#ifdef CONFIG_IPV6
			fprintf(fp1, "%s -t %s %s %s %s\n", IP6TABLES, "mangle", FW_ADD, action_chain_name, action_str);
#endif
		}
		
		//DNS action
		if(pentry->domianlist[0]) {
			match_str[0] = '\0';
			if(time_weekday[0] || time_hour_minitue[0][0] || time_hour_minitue[1][0] || time_hour_minitue[2][0]) {
				snprintf(match_str, sizeof(match_str), " --mark2 0x%llx/0x%llx",
					SOCK_MARK2_APPROUTE_RULE_MATCH_SET(0,1), SOCK_MARK2_APPROUTE_RULE_MATCH_MASK);
			}
			action_str[0] = '\0';
			snprintf(action_str, sizeof(action_str), "-j APPCTL --log-feature DNS_DROP --target DROP");
			if(pentry->signalEnable){
				i = strlen(action_str);
				snprintf(action_str+i, sizeof(action_str)-i, " --sig \"%s\"", action_chain_name);
			}			
			i = strlen(action_str);
			if(pentry->domain_action == 1)
				snprintf(action_str+i, sizeof(action_str)-i, " --dns-reply-ans %s", BRIF);
			else if(pentry->domain_action == 2)
				snprintf(action_str+i, sizeof(action_str)-i, " --dns-reply-code NXDOMAIN");
			
			if(pentry->mode == 1) {
				snprintf(ip_match_str, sizeof(ip_match_str), " -j APPCTL --dns-saveip %s,%s --target RETURN", ipset_dns_chain_name, ipset_dnsv6_chain_name);
				fprintf(fp1, "%s -t %s %s %s %s --qname_set %s --rmatch %s \n", EBTABLES, "filter", FW_ADD, action_dns_chain_name, 
																				match_str, dns_set_name, ip_match_str);
				fprintf(fp1, "%s -t %s %s %s %s\n", EBTABLES, "filter", FW_ADD, action_dns_chain_name, action_str);
			}
			else {
				i = strlen(action_str);
				snprintf(action_str+i, sizeof(action_str)-i, " --dns-saveip %s,%s", ipset_dns_chain_name, ipset_dnsv6_chain_name);
				fprintf(fp1, "%s -t %s %s %s %s --qname_set %s --rmatch %s\n", EBTABLES, "filter", FW_ADD, action_dns_chain_name, 
																				match_str, dns_set_name, action_str);
			}
		}
	}
	
	if(act == 0) {
		if(pentry->maclist[0])
			snprintf(actstr, sizeof(actstr), "-I");
		else
			snprintf(actstr, sizeof(actstr), "-A");
	}
	else
		snprintf(actstr, sizeof(actstr), "-D");
	
	src_mac_match_str[0] = '\0';
	if(pentry->maclist[0])
		snprintf(src_mac_match_str, sizeof(src_mac_match_str), "-m set --match-set %s src", ipset_smac_chain_name);
	
	fprintf(fp1, "%s -t %s %s %s %s -j %s\n", IPTABLES, "mangle", actstr, APPFILTER_CHAIN, src_mac_match_str, action_chain_name);
#ifdef CONFIG_IPV6
	fprintf(fp1, "%s -t %s %s %s %s -j %s\n", IP6TABLES, "mangle", actstr, APPFILTER_CHAIN, src_mac_match_str, action_chain_name);
#endif
	if(pentry->domianlist[0]) {
		src_mac_match_str[0] = '\0';
		if(pentry->maclist[0])
			snprintf(src_mac_match_str, sizeof(src_mac_match_str), " --match-set %s dst", ipset_smac_chain_name);
		fprintf(fp1, "%s -t %s %s %s %s -j %s\n", EBTABLES, "filter", actstr, APPFILTER_CHAIN_OUTPUT, src_mac_match_str, action_dns_chain_name);
	}
	
	if(act) {
		fprintf(fp1, "%s -t %s %s %s\n", IPTABLES, "mangle", "-F", action_chain_name);
		fprintf(fp1, "%s -t %s %s %s\n", IPTABLES, "mangle", "-X", action_chain_name);
#ifdef CONFIG_IPV6
		fprintf(fp1, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-F", action_chain_name);
		fprintf(fp1, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-X", action_chain_name);
#endif
		fprintf(fp1, "%s -t %s %s %s\n", EBTABLES, "filter", "-F", action_dns_chain_name);
		fprintf(fp1, "%s -t %s %s %s\n", EBTABLES, "filter", "-X", action_dns_chain_name);
		
		fprintf(fp1, "echo \"clear_dns_set %s\" > %s \n", dns_set_name, APPCTL_PATH);
		
		fprintf(fp1, "ipset destroy %s\n", ipset_smac_chain_name);
		fprintf(fp1, "ipset destroy %s\n", ipset_subnet_chain_name);
		fprintf(fp1, "ipset destroy %s\n", ipset_dns_chain_name);
#ifdef CONFIG_IPV6
		fprintf(fp1, "ipset destroy %s\n", ipset_subnetv6_chain_name);
		fprintf(fp1, "ipset destroy %s\n", ipset_dnsv6_chain_name);
#endif
	}
	fclose(fp1);
	va_cmd_no_error("/bin/sh", 1, 1, path);
	unlink(path);

	return 0;

error:
	if(fp1) fclose(fp1);
	if(fp) fclose(fp);
	return -1;
}

void apply_appfilter(int isBoot)
{
	MIB_CE_APP_FILTER_T entry;
	int num,i, isValid;
	FILE *fp;
	char path[64];
	
	sprintf(path, "%s_def", APPFILTER_IPSET_RULE_FILE);
	if((fp = fopen(path, "w"))) {
		fprintf(fp, "%s -t %s %s %s -i %s ! -o %s -j %s\n", IPTABLES, "mangle", FW_DEL, FW_FORWARD, BRIF, "br+", APPFILTER_CHAIN);
		fprintf(fp, "%s -t %s %s %s --logical-out %s -p ip --ip-proto udp --ip-sport 53 -j %s\n", EBTABLES, "filter", FW_DEL, FW_OUTPUT, BRIF, APPFILTER_CHAIN_OUTPUT);
#ifdef CONFIG_IPV6
		fprintf(fp, "%s -t %s %s %s -i %s ! -o %s -j %s\n", IP6TABLES, "mangle", FW_DEL, FW_FORWARD, BRIF, "br+", APPFILTER_CHAIN);
		fprintf(fp, "%s -t %s %s %s --logical-out %s -p ip6 --ip6-proto udp --ip6-sport 53 -j %s\n", EBTABLES, "filter", FW_DEL, FW_OUTPUT, BRIF, APPFILTER_CHAIN_OUTPUT);
#endif
		if(isBoot != 2)
		{
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-F", APPFILTER_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-X", APPFILTER_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-N", APPFILTER_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", EBTABLES, "filter", "-F", APPFILTER_CHAIN_OUTPUT);
			fprintf(fp, "%s -t %s %s %s\n", EBTABLES, "filter", "-X", APPFILTER_CHAIN_OUTPUT);
			fprintf(fp, "%s -t %s %s %s\n", EBTABLES, "filter", "-N", APPFILTER_CHAIN_OUTPUT);
			fprintf(fp, "%s -t %s %s %s %s\n", EBTABLES, "filter", "-P", APPFILTER_CHAIN_OUTPUT, "RETURN");
#ifdef CONFIG_IPV6	
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-F", APPFILTER_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-X", APPFILTER_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-N", APPFILTER_CHAIN);
#endif
		}
		
		fprintf(fp, "%s -t %s %s %s -i %s ! -o %s -j %s\n", IPTABLES, "mangle", FW_ADD, FW_FORWARD, BRIF, "br+", APPFILTER_CHAIN);
		fprintf(fp, "%s -t %s %s %s --logical-out %s -p ip --ip-proto udp --ip-sport 53 -j %s\n", EBTABLES, "filter", FW_ADD, FW_OUTPUT, BRIF, APPFILTER_CHAIN_OUTPUT);
#ifdef CONFIG_IPV6
		fprintf(fp, "%s -t %s %s %s -i %s ! -o %s -j %s\n", IP6TABLES, "mangle", FW_ADD, FW_FORWARD, BRIF, "br+", APPFILTER_CHAIN);
		fprintf(fp, "%s -t %s %s %s --logical-out %s -p ip6 --ip6-proto udp --ip6-sport 53 -j %s\n", EBTABLES, "filter", FW_ADD, FW_OUTPUT, BRIF, APPFILTER_CHAIN_OUTPUT);
#endif
		fclose(fp);
		va_cmd_no_error("/bin/sh", 1, 1, path);
		unlink(path);
	}

	if(isBoot != 2) 
	{
		num = mib_chain_total(MIB_APP_FILTER_TBL);
		for(i=0; i<num; i++)
		{
			if(!mib_chain_get(MIB_APP_FILTER_TBL, i, &entry))
				continue;
			if(!entry.Enable)
				continue;
			if(!(entry.maclist && *entry.maclist!='\0') &&
				!(entry.destIPlist && *entry.destIPlist!='\0'))
				continue;

			config_one_appfilter(0, &entry, isBoot);
		}
	}
}
#endif
#endif

#ifdef _PRMT_X_CT_COM_NEIGHBORDETECTION_
#define NEIGHDETECT_TMP_FILE "/tmp/neighdetect"
#define NEIGHDETECT_RESULT_FILE "/tmp/neighdetect_result"

struct callout_s neighdetect_ch;
struct neighdetect_list ndlist[MAX_VC_NUM];
int reset_ipoewan(unsigned int ifindex, unsigned char state)
{
	char pid_file[32];
	int pid;
	char ifname[IFNAMSIZ];
	char mer_gwinfo_file[128];
	MIB_CE_ATM_VC_T entry;
	int entry_index = -1;
	
	if(ifGetName(ifindex,ifname,sizeof(ifname))==0)
		return -1;
	if (getATM_VC_ENTRY_byName(ifname, &entry, &entry_index))
		return -1;
	if(state & 1)
	{
		fprintf(stderr,"[%s %d]restart ipv4 dhcp client for %s\n",__func__,__LINE__,ifname);
		snprintf(pid_file, 32, "%s.%s", (char*)DHCPC_PID, ifname);
		pid = read_pid(pid_file);
		if (pid > 0)
		{
			kill(pid, SIGUSR2);
			sprintf(mer_gwinfo_file, "%s.%s", (char *)MER_GWINFO, ifname);
			unlink(mer_gwinfo_file);
			usleep(10000);
			kill(pid, SIGUSR1);
		}
	}
	if(state & 2)
	{
		fprintf(stderr,"[%s %d]restart ipv6 dhcp client for %s\n",__func__,__LINE__,ifname);
		//Reset mib gw info.
		memset(&entry.RemoteIpv6Addr[0], 0, sizeof(struct in6_addr));
		mib_chain_update(MIB_ATM_VC_TBL, (void *)&entry, entry_index);
#ifdef CONFIG_IPV6
		stopIP_PPP_for_V6(&entry);
		startIP_for_V6(&entry);
#endif
	}
	return 0;
}

int update_neighdetect(void)
{
	char buf[128], gateway[64];
	char ifname[IFNAMSIZ];
	int i=0,j,total_num;
	int neighdetcet_info_change=0;
	FILE*fp;
	MIB_NEIGHBORDETECTION_T entry;
	MIB_CE_ATM_VC_T wanentry;
	unsigned int checkndmap=0,detect_type=0;
#ifdef CONFIG_USER_RTK_RAMONITOR
	RAINFO_V6_T ra_v6Info;
#endif

	total_num = mib_chain_total(MIB_NEIGHBORDETECTION_TBL);
	for(i = 0; i < total_num; i++)
	{
		if (!mib_chain_get(MIB_NEIGHBORDETECTION_TBL, i, (void *)&entry)){
			continue;
		}
		if((entry.ifIndex == 0) || (getATMVCEntryByIfIndex(entry.ifIndex,&wanentry)==NULL) ||
			(wanentry.cmode!=CHANNEL_MODE_IPOE) || (wanentry.ipDhcp!=1))
			continue;
		j=ETH_INDEX(entry.ifIndex);
		checkndmap |= (1<<j);
		if(ndlist[j].ifindex != entry.ifIndex)
		{
			memset(&(ndlist[j]), 0, sizeof(struct neighdetect_list));
			ndlist[j].ifindex = entry.ifIndex;
			ndlist[j].valid = entry.Enable;
			neighdetcet_info_change=1;
		}
		else if(entry.Enable != ndlist[j].valid)
		{
			ndlist[j].valid = entry.Enable;
			neighdetcet_info_change=1;
		}
		if(entry.Enable)
		{
			if((entry.Interval!=0) && (ndlist[j].detect_interval != entry.Interval))
			{
				ndlist[j].detect_interval = entry.Interval;
				neighdetcet_info_change=1;
			}
			if(ndlist[j].detect_interval == 0)
				ndlist[j].detect_interval = 60;
			if((entry.NumberOfRepetitions!=0) && (ndlist[j].detect_count != entry.NumberOfRepetitions))
			{
				ndlist[j].detect_count = entry.NumberOfRepetitions;
				neighdetcet_info_change=1;
			}
			if(ndlist[j].detect_count == 0)
				ndlist[j].detect_count = 3;
		}
	}
	for(i=0; i<MAX_VC_NUM; i++)
	{
		if(!(checkndmap&(1<<i)))
		{
			if(ndlist[i].valid)
			{
				ndlist[i].valid = 0;
				neighdetcet_info_change=1;
			}
		}
		else if(ndlist[i].valid)
		{
			detect_type = 0;
			ifGetName(ndlist[i].ifindex, ifname, sizeof(ifname));
			sprintf(buf, "%s.%s", MER_GWINFO, ifname);
			if((fp = fopen(buf, "r")))
			{
				memset(gateway, 0, sizeof(gateway));
				fscanf(fp, "%[^\n]", gateway);
				fclose(fp);
				if(gateway[0])
				{
					if(inet_pton(AF_INET, gateway, ndlist[i].ipaddr)==1)
					{
						detect_type|=1;
					}
				}
			}
#ifdef CONFIG_IPV6
			snprintf(buf, sizeof(buf), "%s.%s", MER_GWINFO_V6, ifname);
			if((fp = fopen(buf, "r")))
			{
				memset(gateway, 0, sizeof(gateway));
				fscanf(fp, "%[^\n]", gateway);
				fclose(fp);
				if(gateway[0])
				{
					if(inet_pton(AF_INET6, gateway, ndlist[i].ip6addr)==1)
					{
						detect_type|=2;
					}
				}
			}
#ifdef CONFIG_USER_RTK_RAMONITOR
			else
			{
				snprintf(buf, sizeof(buf), "%s_%s", (char *)RA_INFO_FILE, ifname);
				memset(&ra_v6Info, 0, sizeof(ra_v6Info));
				if ((get_ra_info(buf, &ra_v6Info) & RAINFO_GET_GW))
				{
					if (inet_pton(AF_INET6, ra_v6Info.reomte_gateway, ndlist[i].ip6addr)==1)
					{
						detect_type|=2;
					}
				}
			}
#endif
#endif
			if(detect_type != ndlist[i].detecttype)
			{
				ndlist[i].detecttype = detect_type;
				neighdetcet_info_change=1;
			}
		}
	}
	if(neighdetcet_info_change)
	{
		for(i=0; i<MAX_VC_NUM; i++)
		{
			if(ndlist[i].valid)
			{
				ndlist[i].state = 0;
				ifGetName(ndlist[i].ifindex,ifname,sizeof(ifname));
				snprintf(buf,sizeof(buf),"%s.%s.1",NEIGHDETECT_TMP_FILE,ifname);
				unlink(buf);
				snprintf(buf,sizeof(buf),"%s.%s.2",NEIGHDETECT_TMP_FILE,ifname);
				unlink(buf);
			}
		}
	}
	return neighdetcet_info_change;
}
int get_one_neighdetect_result(char*fname)
{
	FILE*fp;
	char buf[128];
	int send=0, recv=0;
	int ret=-1;
	if(fp=fopen(fname,"r"))
	{
		while(fgets(buf,sizeof(buf),fp)!=NULL)
		{
			if(sscanf(buf,"detectresult:send=%d recv=%d",&send, &recv)==2)
			{
				if(recv > 0)
					ret = 0;
				else
					ret = 1;
				break;
			}
		}
		fclose(fp);
	}
	return ret;
}
void update_neighdetect_result(void)
{
	FILE*fp,*fp_inform;
	int i;
	char ifname[IFNAMSIZ];
	unsigned char need_inform_ipoedown=0;
	struct in_addr inAddr;
	struct ipv6_ifaddr ipv6_addr;
	PREFIX_V6_INFO_T prefixInfo;
	char ip_addr[32];
	char ip6_addr[64];
	char ip6_prefix[64];
	char ip6_prefix_with_len[64];

	if(fp=fopen(NEIGHDETECT_RESULT_FILE,"w"))
	{
		for(i=0; i<MAX_VC_NUM; i++)
		{
			if((ndlist[i].valid == 0) || (ndlist[i].detecttype == 0))
				continue;
			ifGetName(ndlist[i].ifindex,ifname,sizeof(ifname));
			ndlist[i].state &= ndlist[i].detecttype;
			sprintf(ip_addr,"-");
			sprintf(ip6_addr,"-");
			sprintf(ip6_prefix_with_len,"-");
			if((ndlist[i].detecttype & 1) &&getInAddr(ifname, IP_ADDR, (void *)&inAddr))
			{
				inet_ntop(AF_INET, &inAddr, ip_addr,sizeof(ip_addr));
			}
			if((ndlist[i].detecttype & 2)&& getifip6(ifname, IPV6_ADDR_UNICAST, &ipv6_addr, 1))
			{
				inet_ntop(AF_INET6, &(ipv6_addr.addr), ip6_addr,sizeof(ip6_addr));
				if(get_prefixv6_info_by_wan(&prefixInfo,ndlist[i].ifindex)==0)
				{
					inet_ntop(AF_INET6, prefixInfo.prefixIP, ip6_prefix,sizeof(ip6_prefix));
					sprintf(ip6_prefix_with_len,"%s/%d",ip6_prefix,prefixInfo.prefixLen);
				}
			}
			fprintf(fp,"%s %d ip:%s ip6:%s prefix:%s\n",ifname,ndlist[i].state,ip_addr,ip6_addr,ip6_prefix_with_len);
			if(ndlist[i].state)
			{
				if(fp_inform=fopen(NEIGHDETECT_IPOEDOWN_INFORM_FILE,"a"))
				{
					fprintf(fp_inform,"%s %d ip:%s ip6:%s prefix:%s\n",ifname,ndlist[i].state,ip_addr,ip6_addr,ip6_prefix_with_len);
					fclose(fp_inform);
				}
				need_inform_ipoedown=1;
				reset_ipoewan(ndlist[i].ifindex, ndlist[i].state);
			}
		}
		fclose(fp);
	}
	if(need_inform_ipoedown)
	{
		unsigned int cwmp_event;
		mib_get_s(CWMP_INFORM_USER_EVENTCODE, &cwmp_event, sizeof(cwmp_event));
		cwmp_event |= EC_X_CT_COM_IPOEWANDOWN;
		mib_set(CWMP_INFORM_USER_EVENTCODE, &cwmp_event);
		fprintf(stderr,"[%s %d]inform ipoewandown event\n",__func__,__LINE__,ifname);
	}
}
int parse_neighdetect_result(char *nd_ifname, char*ipaddr,char*ip6addr,char*prefix)
{
	FILE*fp;
	char buf[128];
	char ifname[IFNAMSIZ];
	int state=0;
	if(fp = fopen(NEIGHDETECT_IPOEDOWN_INFORM_FILE,"r"))
	{
		while(fgets(buf,sizeof(buf),fp)!=NULL)
		{
			if((sscanf(buf,"%s %d ip:%s ip6:%s prefix:%s", ifname, &state,ipaddr,ip6addr,prefix) == 5) && (strcmp(nd_ifname, ifname) == 0))
			{
				fclose(fp);
				if(strcmp("-",ipaddr) == 0)
					*ipaddr='\0';
				if(strcmp("-",ip6addr) == 0)
					*ip6addr='\0';
				if(strcmp("-",prefix) == 0)
					*prefix='\0';
				return state;
			}
		}
		fclose(fp);
	}
	return -1;
}
void neighDetectCheck(void *dummy)
{
	struct sysinfo now;
	int i,state;
	char cmd_buf[256];
	char ifname[IFNAMSIZ];
	char ipaddr_str[64];
	char state_change=0;

	state_change=update_neighdetect();
	sysinfo(&now);
	for(i=0; i<MAX_VC_NUM; i++)
	{
		if(ndlist[i].valid == 0)
			continue;
		ifGetName(ndlist[i].ifindex,ifname,sizeof(ifname));
		if(ndlist[i].detecttype & 1)//v4
		{
			snprintf(cmd_buf,sizeof(cmd_buf),"%s.%s.1",NEIGHDETECT_TMP_FILE,ifname);
			state = get_one_neighdetect_result(cmd_buf);
			if(state!=-1)
			{
				if(state != ((ndlist[i].state)&1))
				{
					ndlist[i].state &= (~1);
					ndlist[i].state |= state;
					state_change = 1;
				}
				fprintf(stderr,"[%s %d]%s ipv4 gateway unreachable:%d\n",__func__,__LINE__,ifname,state);
				unlink(cmd_buf);
			}
		}
#ifdef CONFIG_IPV6
		if(ndlist[i].detecttype & 2)//v6
		{
			snprintf(cmd_buf,sizeof(cmd_buf),"%s.%s.2",NEIGHDETECT_TMP_FILE,ifname);
			state = get_one_neighdetect_result(cmd_buf);
			if(state!=-1)
			{
				if(state != (((ndlist[i].state)&2)>>1))
				{
					ndlist[i].state &= (~2);
					ndlist[i].state |= (state<<1);
					state_change = 1;
				}
				fprintf(stderr,"[%s %d]%s ipv6 gateway unreachable:%d\n",__func__,__LINE__,ifname,state);
				unlink(cmd_buf);
			}
		}
#endif
		if(now.uptime < (ndlist[i].lastdetect_time+ndlist[i].detect_interval))
			continue;

		if(ndlist[i].detecttype & 1)
		{
			inet_ntop(AF_INET,ndlist[i].ipaddr,ipaddr_str,sizeof(ipaddr_str));
			snprintf(cmd_buf,sizeof(cmd_buf),"neighdetect -c %d -i %s %s > %s.%s.1 &",ndlist[i].detect_count,ifname,ipaddr_str,NEIGHDETECT_TMP_FILE,ifname);
			system(cmd_buf);
			fprintf(stderr,"[%s %d]%s ipv4 start gateway detect\n",__func__,__LINE__,ifname);
		}
#ifdef CONFIG_IPV6
		if(ndlist[i].detecttype & 2)
		{
			inet_ntop(AF_INET6,ndlist[i].ip6addr,ipaddr_str,sizeof(ipaddr_str));
			snprintf(cmd_buf,sizeof(cmd_buf),"neighdetect -c %d -i %s %s > %s.%s.2 &",ndlist[i].detect_count,ifname,ipaddr_str,NEIGHDETECT_TMP_FILE,ifname);
			system(cmd_buf);
			fprintf(stderr,"[%s %d]%s ipv6 start gateway detect\n",__func__,__LINE__,ifname);
		}
#endif
		ndlist[i].lastdetect_time = now.uptime;
	}
	if(state_change)
	{
		fprintf(stderr,"[%s %d]%s gateway state change\n",__func__,__LINE__,ifname);
		update_neighdetect_result();
	}
	TIMEOUT_F(neighDetectCheck, 0, 1, neighdetect_ch);
}
#endif
#if defined(SUPPORT_NON_SESSION_IPOE) && defined(_PRMT_X_CT_COM_DHCP_)
int get_dhcp_reply_auth_opt_serverid(char *out, int len, char *serverid, char *enc_key)
{
	//Message = O+R2+Cs, Cs = EnCry(R2 +Ks, ServerID)
	char *enc_data;
	char des_key[24]={0};
	char R[8] = {0};	//8 bytes random number
	long temp;
	char * cur = out;
	char *plain_text;
	int plain_len;

	if(strlen(serverid)==0)
		return -1;

	srandom(temp);
	temp = random();
	memcpy(R, &temp, 4);
	temp = random();
	memcpy(R+4, &temp, 4);

	if(len<10) return -1;
	// First byte is encryption type;
	*cur = 1;
	cur += 1;

	// concat random number
	memcpy(cur, R, 8);
	cur += 8;

	plain_len= strlen(serverid);
	if(plain_len == 0)
		return -1;
	else if(plain_len&0x7)
	{
		plain_len += (8-(plain_len&0x7));
		if((plain_len+9) > len) 
			return -1;
		plain_text = malloc(plain_len);
		memset(plain_text, 0, plain_len);
		memcpy(plain_text, serverid, strlen(serverid));
	}
	else
	{
		if((plain_len+9) > len) 
			return -1;
		plain_text = serverid;
	}
	enc_data = malloc(plain_len);
	//Cs = EnCry(R2 +Ks, ServerID)
	memcpy(des_key, enc_key, 16);
	memcpy(des_key+16, R, 8);
#ifdef CONFIG_USER_OPENSSL
	do_3des(des_key, plain_text, enc_data, plain_len, DES_ENCRYPT);
#endif
	memcpy(cur, enc_data, plain_len);
	cur += plain_len;
	if(strlen(serverid)&0x7)
		free(plain_text);
	free(enc_data);

	//output_hex("Result: ", out, cur - out);

	return (cur - out);
}

int parse_dhcp_server_auth_opt_serverid(char *buf, int buflen, char *enc_key, char *serverid)
{
	//Message = O+R2+Cs  , Cs = EnCry(R2 +Ks, ServerID)
	char * cur = buf;
	char des_key[24]={0};
	if((buflen<17) || ((buflen-9)&0x7))
		return -1;
	if(*cur == 1)
	{
		cur++;
		memcpy(des_key,enc_key,16);
		memcpy(des_key+16, cur, 8);
		cur+=8;
		return do_3des(des_key, cur, serverid, buflen-9, DES_DECRYPT);
	}
	return -1;
}
#endif

#if defined(SUPPORT_CLOUD_VR_SERVICE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && (defined(CONFIG_EPON_FEATURE) || defined(CONFIG_GPON_FEATURE))
#if defined(CONFIG_RTL9607C_SERIES)
extern int rtk_clearSwitchDSQueueLimit(void);
extern int rtk_setupSwitchDSQueueLimit(int queue, unsigned int rate);
int rtk_addSwitchDSQueueLimitACL(unsigned char *pmac, int queue);
#if defined(WLAN_SUPPORT)
#define SUPPORT_WIFI_SPECIAL_RATE_LIMIT
#endif
#endif
extern int rtk_specialAdjustSMP(void);
extern const char CAR_DS_TRAFFIC_CHAIN[];
extern const char CAR_US_TRAFFIC_CHAIN[];
//special, for test case 4.1.1, to promise the VR download performance

int check_special_ds_ratelimit(void)
{
	#define SPEC_DS_LIMIT "SPECIAL_DS_RATE_LIMIT"
	#define SPEC_US_LIMIT "SPECIAL_US_RATE_LIMIT"
	#define SPEC_DS_CRI 800000
	#define SPEC_US_CRI 400000
	#define SPEC_DS_RATE 409600
	#define SPEC_DS_RATE_FOR_EPON 307200
	#define SPEC_DS_INDEX 65535
	#define SPEC_DS_QUEUE 2
	#define SPEC_VR_US_QUEUE 5
	#define SPEC_INTERNET_US_QUEUE 4
	#define SPEC_VR_US_RATE 409600
	#define SPEC_VR_US_RATE_FOR_EPON 204800
	#define SPEC_INTERNET_US_RATE 409600
#ifdef SUPPORT_WIFI_SPECIAL_RATE_LIMIT
	#define SPEC_WIFI_DS_RATE_LIMIT "SPECIAL_WIFI_DS_RATE_LIMIT"
	#define SPEC_WIFI_DS_RATE_INDEX 65534
	#define SPEC_WIFI_DS_RATE 51200
	#define SPEC_WIFI_US_RATE_LIMIT "SPECIAL_WIFI_US_RATE_LIMIT"
	#define SPEC_WIFI_US_RATE_INDEX 65533
	#define SPEC_WIFI_US_RATE 51200
#endif
	
	unsigned char found_VR_ds_flow_limit = 0, found_VR_us_flow_limit = 0, has_internet_ds_flow_limit = 0;
	int entryNum, foundInternet = 0, i;
	MIB_CE_IP_TC_T  entry;
	char province_name[32]={0};
	unsigned int pon_mode = 0;
	unsigned int pon_speed=0;
	MIB_CE_ATM_VC_T vcEntry;
	int meter_idx = -1;
	char ifname[IFNAMSIZ+1]={0};
	unsigned int mark=0, mark_mask=0;
#if defined(CONFIG_RTK_SKB_MARK2)
	unsigned long long mark2=0, mark2_mask=0;
#endif
	DOCMDINIT;
	
	DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -D %s -j %s", CAR_DS_TRAFFIC_CHAIN, SPEC_DS_LIMIT);
	DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -F %s", SPEC_DS_LIMIT);
	DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -X %s", SPEC_DS_LIMIT);
	DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -D %s -j %s", CAR_US_TRAFFIC_CHAIN, SPEC_US_LIMIT);
	DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -F %s", SPEC_US_LIMIT);
	DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -X %s", SPEC_US_LIMIT);
#if defined(CONFIG_IPV6)
	DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -D %s -j %s", CAR_DS_TRAFFIC_CHAIN, SPEC_DS_LIMIT);
	DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -F %s", SPEC_DS_LIMIT);
	DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -X %s", SPEC_DS_LIMIT);
	DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -D %s -j %s", CAR_US_TRAFFIC_CHAIN, SPEC_US_LIMIT);
	DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -F %s", SPEC_US_LIMIT);
	DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -X %s", SPEC_US_LIMIT);
#endif

#ifdef SUPPORT_WIFI_SPECIAL_RATE_LIMIT
	DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -D %s -i wlan+ -j %s", FW_PREROUTING, SPEC_WIFI_US_RATE_LIMIT);
	DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -F %s", SPEC_WIFI_US_RATE_LIMIT);
	DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -X %s", SPEC_WIFI_US_RATE_LIMIT);
	rtk_qos_share_meter_delete(RTK_APP_TYPE_IP_QOS_RATE_SHAPING, SPEC_WIFI_US_RATE_INDEX);
	DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -D %s -o wlan+ -j %s", FW_POSTROUTING, SPEC_WIFI_DS_RATE_LIMIT);
	DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -F %s", SPEC_WIFI_DS_RATE_LIMIT);
	DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -X %s", SPEC_WIFI_DS_RATE_LIMIT);
	rtk_qos_share_meter_delete(RTK_APP_TYPE_IP_QOS_RATE_SHAPING, SPEC_WIFI_DS_RATE_INDEX);
#endif

#if defined(CONFIG_RTL9607C_SERIES)
	rtk_clearSwitchDSQueueLimit();
#else
	rtk_qos_share_meter_delete(RTK_APP_TYPE_IP_QOS_RATE_SHAPING, SPEC_DS_INDEX);
#endif

	mib_get_s(MIB_HW_PROVINCE_NAME, province_name, sizeof(province_name));
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
	mib_get_s(MIB_PON_SPEED, (void *)&pon_speed, sizeof(pon_speed));

	if(pon_speed==0 
#if defined(CONFIG_RTL9607C_SERIES)
		&& (pon_mode==EPON_MODE || pon_mode==GPON_MODE)
#else
		&& pon_mode==EPON_MODE 
#endif
		&& !strcmp(province_name, "shh_2"))
	{
		entryNum = mib_chain_total(MIB_IP_QOS_TC_TBL);
		for(i=0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_IP_QOS_TC_TBL, i, (void*)&entry))
				continue;
			
			if(entry.ifIndex != DUMMY_IFINDEX)
			{
				if (!(getATMVCEntryByIfIndex(entry.ifIndex, &vcEntry)))
					continue;
				if(vcEntry.applicationtype& X_CT_SRV_SPECIAL_SERVICE_VR)
				{
					if(entry.direction == QOS_DIRECTION_DOWNSTREAM && entry.limitSpeed >= SPEC_DS_CRI)
						found_VR_ds_flow_limit ++;
					
					if(entry.direction == QOS_DIRECTION_UPSTREAM && entry.limitSpeed >= SPEC_US_CRI)
						found_VR_us_flow_limit ++;	
				}
				if(vcEntry.applicationtype& X_CT_SRV_INTERNET && entry.direction == QOS_DIRECTION_DOWNSTREAM)
					has_internet_ds_flow_limit ++;
			}
			else if(entry.direction == QOS_DIRECTION_DOWNSTREAM)
				has_internet_ds_flow_limit ++;
		}

		if(found_VR_ds_flow_limit && found_VR_us_flow_limit && has_internet_ds_flow_limit == 0)
		{
#if defined(CONFIG_RTL9607C_SERIES)
			DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -N %s", SPEC_DS_LIMIT);
			DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -A %s -j %s", CAR_DS_TRAFFIC_CHAIN, SPEC_DS_LIMIT);
			DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -N %s", SPEC_US_LIMIT);
			DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -A %s -j %s", CAR_US_TRAFFIC_CHAIN, SPEC_US_LIMIT);
#if defined(CONFIG_IPV6)
			DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -N %s", SPEC_DS_LIMIT);
			DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -A %s -j %s", CAR_DS_TRAFFIC_CHAIN, SPEC_DS_LIMIT);
			DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -N %s", SPEC_US_LIMIT);
			DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -A %s -j %s", CAR_US_TRAFFIC_CHAIN, SPEC_US_LIMIT);
#endif
#endif
			meter_idx = -1;
			entryNum = mib_chain_total(MIB_ATM_VC_TBL);
			for (i=0; i<entryNum; i++) 
			{
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry) || vcEntry.enable == 0)
					continue;
#if defined(CONFIG_RTL9607C_SERIES)
				if(vcEntry.applicationtype& X_CT_SRV_SPECIAL_SERVICE_VR && vcEntry.cmode != CHANNEL_MODE_BRIDGE)
				{
					ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
					if(pon_mode == GPON_MODE)
					{
						rt_gpon_queueCfg_t rtGponQueueCfg = {0};
						struct wanif_flowq wanq={0};
						rtk_ponmac_queue_t Queue;
						wanq.wanIfidx = vcEntry.ifIndex;
						if(get_wan_flowid(&wanq)==1)
						{
							if(rtk_ponmac_flow2Queue_get(wanq.usflow[SPEC_VR_US_QUEUE], &Queue )==0)
							{
								rtGponQueueCfg.cir = 0;
								rtGponQueueCfg.pir = (SPEC_VR_US_RATE/8);
								rtGponQueueCfg.scheduleType = RT_GPON_TCONT_QUEUE_SCHEDULER_SP;
								rtGponQueueCfg.weight = 0;
								rt_gpon_ponQueue_set(Queue.schedulerId, Queue.queueId, &rtGponQueueCfg);
							}
						}
					}
					else if(pon_mode == EPON_MODE)
					{
						rt_epon_queueCfg_t	rtEponQueueCfg = {0};
						rtEponQueueCfg.cir = 0;
						rtEponQueueCfg.pir = (SPEC_VR_US_RATE_FOR_EPON/8);
						rtEponQueueCfg.scheduleType = RT_EPON_LLID_QUEUE_SCHEDULER_SP;
						rtEponQueueCfg.weight = 0;
						rt_epon_ponQueue_set(0, SPEC_VR_US_QUEUE, &rtEponQueueCfg);
					}
					mark = mark_mask = 0; 
					mark |= SOCK_MARK_QOS_SWQID_TO_MARK(SPEC_VR_US_QUEUE);
					mark_mask |= (SOCK_MARK_QOS_SWQID_BIT_MASK|SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK|SOCK_MARK_METER_INDEX_BIT_MASK);
					DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -A %s -o %s -j MARK --set-mark 0x%x/0x%x", SPEC_US_LIMIT, ifname, mark, mark_mask);
#if defined(CONFIG_IPV6)
					DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -A %s -o %s -j MARK --set-mark 0x%x/0x%x", SPEC_US_LIMIT, ifname, mark, mark_mask);
#endif
					rtk_fc_tcp_smallack_protect_set(3);
				}
				else
#endif
				if(vcEntry.applicationtype& X_CT_SRV_INTERNET && vcEntry.cmode != CHANNEL_MODE_BRIDGE)
				{
					foundInternet++;
					ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
#if defined(CONFIG_RTL9607C_SERIES)
					if(pon_mode==EPON_MODE)
						meter_idx = rtk_setupSwitchDSQueueLimit(SPEC_DS_QUEUE, SPEC_DS_RATE_FOR_EPON);
					else if(pon_mode==GPON_MODE)
						meter_idx = rtk_setupSwitchDSQueueLimit(SPEC_DS_QUEUE, SPEC_DS_RATE);
					if(meter_idx >= 0)
					{
						rtk_addSwitchDSQueueLimitACL(vcEntry.MacAddr, SPEC_DS_QUEUE);
					}
					if(foundInternet == 1)
					{
						if(pon_mode == GPON_MODE)
						{
							rt_gpon_queueCfg_t rtGponQueueCfg = {0};
							struct wanif_flowq wanq={0};
							rtk_ponmac_queue_t Queue;
							wanq.wanIfidx = vcEntry.ifIndex;
							if(get_wan_flowid(&wanq)==1)
							{
								if(rtk_ponmac_flow2Queue_get(wanq.usflow[SPEC_INTERNET_US_QUEUE], &Queue )==0)
								{
									rtGponQueueCfg.cir = 0;
									rtGponQueueCfg.pir = (SPEC_INTERNET_US_RATE/8);
									rtGponQueueCfg.scheduleType = RT_GPON_TCONT_QUEUE_SCHEDULER_SP;
									rtGponQueueCfg.weight = 0;
									rt_gpon_ponQueue_set(Queue.schedulerId, Queue.queueId, &rtGponQueueCfg);
								}
							}
						}
						else if(pon_mode == EPON_MODE)
						{
							rt_epon_queueCfg_t	rtEponQueueCfg = {0};
							rtEponQueueCfg.cir = 0;
							rtEponQueueCfg.pir = (SPEC_INTERNET_US_RATE/8);
							rtEponQueueCfg.scheduleType = RT_EPON_LLID_QUEUE_SCHEDULER_SP;
							rtEponQueueCfg.weight = 0;
							rt_epon_ponQueue_set(0, SPEC_INTERNET_US_QUEUE, &rtEponQueueCfg);
						}
						mark = mark_mask = 0; 
						mark |= SOCK_MARK_QOS_SWQID_TO_MARK(SPEC_INTERNET_US_QUEUE);
						mark_mask |= SOCK_MARK_QOS_SWQID_BIT_MASK;
						DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -A %s -o %s -j MARK --set-mark 0x%x/0x%x", SPEC_US_LIMIT, ifname, mark, mark_mask);
						DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -A %s -o %s -j ACCEPT", SPEC_US_LIMIT, ifname);//do not assign high priority for internet ack
#if defined(CONFIG_IPV6)
						DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -A %s -o %s -j MARK --set-mark 0x%x/0x%x", SPEC_US_LIMIT, ifname, mark, mark_mask);
						DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -A %s -o %s -j ACCEPT", SPEC_US_LIMIT, ifname);
						DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -I %s -o %s -j RETURN", FW_TCP_SMALLACK_PROTECTION, ifname);
#endif
					}
#else
					if(foundInternet == 1)
					{
						DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -N %s", SPEC_DS_LIMIT);
						DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -A %s -j %s", CAR_DS_TRAFFIC_CHAIN, SPEC_DS_LIMIT);
#if defined(CONFIG_IPV6)
						DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -N %s", SPEC_DS_LIMIT);
						DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -A %s -j %s", CAR_DS_TRAFFIC_CHAIN, SPEC_DS_LIMIT);
#endif	
						meter_idx = rtk_qos_share_meter_set(RTK_APP_TYPE_IP_QOS_RATE_SHAPING, SPEC_DS_INDEX, SPEC_DS_RATE);
					}
					
					if(meter_idx >= 0)
					{
						mark = mark_mask = 0; 
						mark |= SOCK_MARK_METER_INDEX_TO_MARK(meter_idx);
						mark_mask |= SOCK_MARK_METER_INDEX_BIT_MASK;
#if defined(CONFIG_RTK_SKB_MARK2)
						mark2 = mark2_mask = 0;
						mark2 |= SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(1);
						mark2_mask = SOCK_MARK2_METER_HW_INDEX_ENABLE_BIT_MASK;
#else
						mark |= SOCK_MARK_METER_INDEX_ENABLE_TO_MARK(1);
						mark_mask |= SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK;
#endif
			
						if(mark || mark_mask){
							DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -A %s -i %s -j MARK --set-mark 0x%x/0x%x", SPEC_DS_LIMIT, ifname, mark, mark_mask);
#if defined(CONFIG_IPV6)
							DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -A %s -i %s -j MARK --set-mark 0x%x/0x%x", SPEC_DS_LIMIT, ifname, mark, mark_mask);
#endif
						}
#ifdef CONFIG_RTK_SKB_MARK2
						if(mark2 || mark2_mask){
							DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -A %s -i %s -j MARK2 --set-mark2 0x%llx/0x%llx", SPEC_DS_LIMIT, ifname, mark2, mark2_mask);
#if defined(CONFIG_IPV6)
							DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -A %s -i %s -j MARK2 --set-mark2 0x%llx/0x%llx", SPEC_DS_LIMIT, ifname, mark2, mark2_mask);
#endif
						}
					}
#endif
#endif
				}
			}
			
			if(foundInternet > 0)
			{
#ifdef SUPPORT_WIFI_SPECIAL_RATE_LIMIT
				//WIFI downstream limit
				meter_idx = rtk_qos_share_meter_set(RTK_APP_TYPE_IP_QOS_RATE_SHAPING, SPEC_WIFI_DS_RATE_INDEX, SPEC_WIFI_DS_RATE);
				if(meter_idx >= 0)
				{
					DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -N %s", SPEC_WIFI_DS_RATE_LIMIT);
					DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -P %s RETURN", SPEC_WIFI_DS_RATE_LIMIT);
					DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -A %s -o wlan+ -j %s", FW_POSTROUTING, SPEC_WIFI_DS_RATE_LIMIT);
					
					mark |= SOCK_MARK_METER_INDEX_TO_MARK(meter_idx);
					mark_mask |= SOCK_MARK_METER_INDEX_BIT_MASK;
					mark |= SOCK_MARK_METER_INDEX_ENABLE_TO_MARK(1);
					mark_mask |= SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK;
					
					DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -A %s -j mark --mark-and 0x%x --mark-target CONTINUE", SPEC_WIFI_DS_RATE_LIMIT, ~mark_mask);
					DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -A %s -j mark --mark-or 0x%x --mark-target CONTINUE", SPEC_WIFI_DS_RATE_LIMIT, mark);
				}
				//WIFI upstream limit
				meter_idx = rtk_qos_share_meter_set(RTK_APP_TYPE_IP_QOS_RATE_SHAPING, SPEC_WIFI_DS_RATE_INDEX, SPEC_WIFI_US_RATE);
				if(meter_idx >= 0)
				{
					DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -N %s", SPEC_WIFI_US_RATE_LIMIT);
					DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -P %s RETURN", SPEC_WIFI_US_RATE_LIMIT);
					DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -A %s -i wlan+ -j %s", FW_PREROUTING, SPEC_WIFI_US_RATE_LIMIT);
					
					mark |= SOCK_MARK_METER_INDEX_TO_MARK(meter_idx);
					mark_mask |= SOCK_MARK_METER_INDEX_BIT_MASK;
					mark |= SOCK_MARK_METER_INDEX_ENABLE_TO_MARK(1);
					mark_mask |= SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK;
					
					DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -A %s -j mark --mark-and 0x%x --mark-target CONTINUE", SPEC_WIFI_US_RATE_LIMIT, ~mark_mask);
					DOCMDARGVS(EBTABLES, DOWAIT, "-t nat -A %s -j mark --mark-or 0x%x --mark-target CONTINUE", SPEC_WIFI_US_RATE_LIMIT, mark);
				}
#endif
				rtk_specialAdjustSMP();
			}
		}
	}
	
	return 0;
}

//testcase 3.1.2 test Internet throughput under VR fixed bandwith(ratelimit on ftp server: upload 50m, download 300m)
void check_special_yueme_test(void)
{
	MIB_CE_IP_QOS_T entry;
	MIB_CE_ATM_VC_T wanentry;
	int i,  EntryNum = 0;
	unsigned int test_flag=0;
	char internet_ifname[32]={0};

	EntryNum = mib_chain_total(MIB_IP_QOS_TBL);
	for(i=0; i<EntryNum; i++)
	{
		memset(&entry, 0, sizeof(entry));
		if(!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)&entry) || !entry.enable)
			continue;
		if((entry.direction == QOS_DIRECTION_UPSTREAM) && (entry.classtype == QOS_CLASSTYPE_NORMAL) && (entry.outif != DUMMY_IFINDEX))
		{
			if(getATMVCEntryByIfIndex(entry.outif, &wanentry))
			{
				if((wanentry.applicationtype & X_CT_SRV_INTERNET) && (((entry.m_dscp-1)>>2) == 24))
				{
					ifGetName(entry.outif, internet_ifname, sizeof(internet_ifname));
					test_flag |= 1;
				}
				else if((wanentry.applicationtype & X_CT_SRV_SPECIAL_SERVICE_VR) && (((entry.m_dscp-1)>>2) == 40))
					test_flag |= 2;
			}
		}
	}

	if(test_flag == 3)
	{
		//internet ack do not egress from highest queue
		va_cmd(IPTABLES, 8, 1, "-t", "mangle", "-I", FW_TCP_SMALLACK_PROTECTION, "-o", internet_ifname, "-j", FW_RETURN);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 8, 1, "-t", "mangle", "-I", FW_TCP_SMALLACK_PROTECTION, "-o", internet_ifname, "-j", FW_RETURN);
#endif
		rtk_fc_tcp_smallack_protect_set(2);
		rtk_specialAdjustSMP();
#ifdef CONFIG_ARCH_CORTINA
		// delay for update VR ack flow hwnat high prio
		system("echo mode kernel > proc/fc/ctrl/flow_delay");
		system("echo count 5 > proc/fc/ctrl/flow_delay");
#endif
		fprintf(stderr, "[%s]special yueme test detected! Close upstream tcp small ack protect and adjust smp!\n",__func__);
	}
}
#endif

#if 0 //use AppCtrlNF instead of dnsmonitor //def CONFIG_USER_DNS_MONITOR
int config_domainlist_to_dnsmonitor(int act, char*domainlist, int instnum)
{
	struct dnsmonitor_msg* msg;
	int msg_len = sizeof(struct dnsmonitor_msg)+strlen(domainlist)+5;
	msg = (struct dnsmonitor_msg*)malloc(msg_len);
	memset(msg,0,msg_len);
	msg->act = act;
	msg->arg_len = msg_len;
	*((int*)msg->arg) = instnum;
	strcpy(msg->arg+4, domainlist);
	send_cmd_to_dnsmonitor(msg, msg_len);
	free(msg);
	return 0;
}
int send_cmd_to_dnsmonitor(struct dnsmonitor_msg* msg, int msg_len)
{
	int msg_fd,ret=0;
	struct sockaddr_un unaddr;
	if(msg_fd = socket(AF_UNIX, SOCK_DGRAM, 0))
	{
		unaddr.sun_family = AF_UNIX;
		strcpy(unaddr.sun_path, DNSMONITOR_MSG_PATH);
		if((ret=sendto(msg_fd, msg, msg_len, 0, (struct sockaddr *)&unaddr, sizeof(unaddr)))==-1)
		{
			close(msg_fd);
			perror("[send_cmd_to_dnsmonitor]");
			return -1;
		}
		close(msg_fd);
	}
	return ret;
}
#endif

#if defined(YUEME_4_0_SPEC)
const char HWACCCANCEL_RULE_FILE[]="/tmp/hwAccCancel_rule";
const char NF_HWACCCANCEL_CHAIN[]="hwacc_cancel_fwd";
const char NF_HWACCCANCEL_ACT_CHAIN[]="hwacc_cancel_act";

//act 0:add 1:del
int config_one_hwAccCancel(int act, HWACC_CANCEL_T *pentry)
{
	unsigned char ip_v4=0, rule_count=0;
#ifdef CONFIG_IPV6
	unsigned char ip_v6=0;
#endif
	char cmd[512], filter_ip[128]={0}, *act_str, *p;
	char filter_port[64]={0}, filter_port2[64]={0};
	
	if(!pentry) return -1;
	if(!pentry->enable) return 0;
	
	if(act == 0) act_str = "-A";
	else act_str = "-D";
	
	p = filter_ip;
	if(pentry->srcaddr[0]) p += sprintf(p, " -s %s", pentry->srcaddr);
	if(pentry->dstaddr[0]) p += sprintf(p, " -d %s", pentry->dstaddr);
	
	if(!pentry->proto[0] || !strcasecmp(pentry->proto, "TCP")) {
		p = filter_port;
		p += sprintf(p, " -p tcp");
		if(pentry->srcport) p += sprintf(p, " --sport %hu", pentry->srcport);
		if(pentry->dstport) p += sprintf(p, " --dport %hu", pentry->dstport);
	}
	if(!pentry->proto[0] || !strcasecmp(pentry->proto, "UDP")) {
		p = filter_port2;
		p += sprintf(p, " -p udp");
		if(pentry->srcport) p += sprintf(p, " --sport %hu", pentry->srcport);
		if(pentry->dstport) p += sprintf(p, " --dport %hu", pentry->dstport);
	}
	
	if(!pentry->srcaddr[0] && !pentry->dstaddr[0]) {
		ip_v4 = 1;
#ifdef CONFIG_IPV6
		ip_v6 = 1;
#endif
	}
#ifdef CONFIG_IPV6
	else if((pentry->srcaddr[0] && strchr(pentry->srcaddr, ':')) || 
			(pentry->dstaddr[0] && strchr(pentry->dstaddr, ':'))) 
	{
		ip_v6 = 1;
	}
#endif
	else {
		ip_v4 = 1;
	}
	
	if(ip_v4) { 
		if(filter_ip[0] && !filter_port[0] && !filter_port2[0]) {
			sprintf(cmd, "iptables -t mangle %s %s %s -g %s", act_str, NF_HWACCCANCEL_CHAIN, filter_ip, NF_HWACCCANCEL_ACT_CHAIN);
			//printf("===> [%s] %s\n", __FUNCTION__, cmd);
			va_cmd_no_error("/bin/sh", 2, 1, "-c", cmd);
			rule_count++;
		}
		else {
			if(filter_port[0]) {
				sprintf(cmd, "iptables -t mangle %s %s %s %s -g %s", act_str, NF_HWACCCANCEL_CHAIN, filter_ip, filter_port, NF_HWACCCANCEL_ACT_CHAIN);
				//printf("===> [%s] %s\n", __FUNCTION__, cmd);
				va_cmd_no_error("/bin/sh", 2, 1, "-c", cmd);
				rule_count++;
			}
			if(filter_port2[0]) {
				sprintf(cmd, "iptables -t mangle %s %s %s %s -g %s", act_str, NF_HWACCCANCEL_CHAIN, filter_ip, filter_port2, NF_HWACCCANCEL_ACT_CHAIN);
				//printf("===> [%s] %s\n", __FUNCTION__, cmd);
				va_cmd_no_error("/bin/sh", 2, 1, "-c", cmd);
				rule_count++;
			}
		}
	}
#ifdef CONFIG_IPV6
	if(ip_v6) { 
		if(filter_ip[0] && !filter_port[0] && !filter_port2[0]) {
			sprintf(cmd, "ip6tables -t mangle %s %s %s -g %s", act_str, NF_HWACCCANCEL_CHAIN, filter_ip, NF_HWACCCANCEL_ACT_CHAIN);
			//printf("===> [%s] %s\n", __FUNCTION__, cmd);
			va_cmd_no_error("/bin/sh", 2, 1, "-c", cmd);
			rule_count++;
		}
		else {
			if(filter_port[0]) {
				sprintf(cmd, "ip6tables -t mangle %s %s %s %s -g %s", act_str, NF_HWACCCANCEL_CHAIN, filter_ip, filter_port, NF_HWACCCANCEL_ACT_CHAIN);
				//printf("===> [%s] %s\n", __FUNCTION__, cmd);
				va_cmd_no_error("/bin/sh", 2, 1, "-c", cmd);
				rule_count++;
			}
			if(filter_port2[0]) {
				sprintf(cmd, "ip6tables -t mangle %s %s %s %s -g %s", act_str, NF_HWACCCANCEL_CHAIN, filter_ip, filter_port2, NF_HWACCCANCEL_ACT_CHAIN);
				//printf("===> [%s] %s\n", __FUNCTION__, cmd);
				va_cmd_no_error("/bin/sh", 2, 1, "-c", cmd);
				rule_count++;
			}
		}
	}
#endif

#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	if(act == 0 && rule_count && pentry->enable) {
		rtk_fc_flow_delete(ip_v4, pentry->proto, pentry->srcaddr, pentry->srcport, pentry->dstaddr, pentry->dstport);
	}
#endif
	return 0;
}

int apply_hwAccCancel(int isBoot)
{
	unsigned char enable = 0;
	HWACC_CANCEL_T entry;
	int num, i;
	FILE *fp;
	char path[64], fw_mark[128], act_mark[128];
	
	sprintf(path, "%s_def", HWACCCANCEL_RULE_FILE);
	if((fp = fopen(path, "w"))) {
		snprintf(fw_mark, sizeof(fw_mark), "-m connmark2 ! --mark2 0x%llx/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK, SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);
		fprintf(fp, "%s -t %s %s %s %s -j %s\n", IPTABLES, "mangle", FW_DEL, FW_FORWARD, fw_mark, NF_HWACCCANCEL_CHAIN);
#ifdef CONFIG_IPV6
		fprintf(fp, "%s -t %s %s %s %s -j %s\n", IP6TABLES, "mangle", FW_DEL, FW_FORWARD, fw_mark, NF_HWACCCANCEL_CHAIN);
#endif
		if(isBoot != 2) 
		{
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-F", NF_HWACCCANCEL_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-X", NF_HWACCCANCEL_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-N", NF_HWACCCANCEL_CHAIN);
#ifdef CONFIG_IPV6
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-F", NF_HWACCCANCEL_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-X", NF_HWACCCANCEL_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-N", NF_HWACCCANCEL_CHAIN);
#endif
			snprintf(act_mark, sizeof(act_mark),"-j MARK2 --set-mark2 0x%llx/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK, SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-F", NF_HWACCCANCEL_ACT_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-X", NF_HWACCCANCEL_ACT_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IPTABLES, "mangle", "-N", NF_HWACCCANCEL_ACT_CHAIN);
			fprintf(fp, "%s -t %s %s %s %s\n", IPTABLES, "mangle", "-A", NF_HWACCCANCEL_ACT_CHAIN, act_mark);
#ifdef CONFIG_IPV6
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-F", NF_HWACCCANCEL_ACT_CHAIN);
			fprintf(fp, "%s -t %s %s %s\n", IP6TABLES, "mangle", "-X", NF_HWACCCANCEL_ACT_CHAIN);
			fprintf(fp, "%s -t %s %s %s %s\n", IP6TABLES, "mangle", "-N", NF_HWACCCANCEL_ACT_CHAIN, act_mark);
#endif
		}
		fprintf(fp, "%s -t %s %s %s %s -j %s\n", IPTABLES, "mangle", FW_ADD, FW_FORWARD, fw_mark, NF_HWACCCANCEL_CHAIN);
#ifdef CONFIG_IPV6
		fprintf(fp, "%s -t %s %s %s %s -j %s\n", IP6TABLES, "mangle", FW_ADD, FW_FORWARD, fw_mark, NF_HWACCCANCEL_CHAIN);
#endif
		fclose(fp);
		va_cmd_no_error("/bin/sh", 1, 1, path);
		unlink(path);
	}
	
	mib_get_s(MIB_HWACC_CANCEL_ENABLE, (void*)&enable, sizeof(enable));
	
	if(enable && isBoot != 2) 
	{
		num = mib_chain_total(MIB_HWACC_CANCEL_TBL);
		for(i=0; i<num; i++)
		{
			if(!mib_chain_get(MIB_HWACC_CANCEL_TBL, i, &entry))
				continue;
			if(!entry.enable)
				continue;
			config_one_hwAccCancel(0, &entry);
		}
	}
	return 0;
}

#endif


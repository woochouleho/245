#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mib.h"
#include "utility.h"
#if defined(CONFIG_ELINK_SUPPORT)
#include <sys/types.h>
#include <dirent.h>
#ifdef CONFIG_ELINKSDK_SUPPORT
#include "../../../lib_elinksdk/libelinksdk.h"
#endif
#endif

#ifdef CONFIG_USER_AHSAPD
#include "subr_multiap.h"
//#include "rtk_isp_wan_adapter.h"
#endif

#ifdef CONFIG_USER_CTCAPD
#define BIT(x)	(1 << (x))
extern int wlan_idx;
#endif

#if defined(CONFIG_ELINK_SUPPORT) || defined(CONFIG_ANDLINK_SUPPORT)
#define RTL_LINK_PID_FILE "/var/run/rtl_link.pid"
#endif
#ifdef CONFIG_ELINK_SUPPORT
#define CHECK_ELINKSDK_TIMEOUT 2
#endif
#ifdef CONFIG_ANDLINK_SUPPORT
#define CHECK_ANDLINK_TIMEOUT 3
#endif
#define CHECK_OVER_LIMIT_TIMEOUT 60
#define RTL_LINK_LIMIT_FLIE "/tmp/overlimit_info"

#if defined(CONFIG_USER_HAPD) && defined(CONFIG_RTK_DNS_TRAP)
#define MAX_CHECK_MAP_NUM	3
#define CHECK_MAP_UPLINK_INFO_TIMEOUT	3
#define MAP_UPLINK_INFO_FILE	"/tmp/map_backhaul_uplink_info"
#define MAP_UPLINK_STATUS_FILE	"/tmp/map_backhaul_link_status"
#endif

#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
#define CHECK_LOAD_BALANCE 10
#define DELETE_UNEXIST_STA_SAVE_INFO  3600

int rtk_timely_check_load_balance(void)
{
	char opmode = GATEWAY_MODE;
	
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
	mib_get_s(MIB_OP_MODE, (void *)&opmode, sizeof(opmode));
#endif
	if(opmode == GATEWAY_MODE)
	{		
		if(rtk_get_dynamic_load_balance_enable())
		{
			system("sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process dy_to_enable");
		}
	}

	return 1;
}

int rtk_timely_delete_unexist_save_info(void)
{
	char opmode = GATEWAY_MODE;

#ifdef CONFIG_USER_RTK_BRIDGE_MODE
	mib_get_s(MIB_OP_MODE, (void *)&opmode, sizeof(opmode));
#endif
	if(opmode == GATEWAY_MODE)
	{		
		if(rtk_get_dynamic_load_balance_enable())
		{
			system("sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process delete_unexist_save_info");
		}
	}
	
	return 1;
}
#endif

#ifdef CONFIG_ELINK_SUPPORT
static pid_t rtk_find_pid_by_name( char* pidName)
{
	DIR *dir;
	struct dirent *next;
	pid_t pid;
	
	if ( strcmp(pidName, "init")==0)
		return 1;
	
	dir = opendir("/proc");
	if (!dir) {
		printf("Cannot open /proc");
		return 0;
	}

	while ((next = readdir(dir)) != NULL) {
		FILE *status;
		char filename[64];
		char buffer[64];
		char name[64];

		/* Must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, "..") == 0)
			continue;

		/* If it isn't a number, we don't want it */
		if (!isdigit(*next->d_name))
			continue;

		sprintf(filename, "/proc/%s/status", next->d_name);
		if (! (status = fopen(filename, "r")) ) {
			continue;
		}
		if (fgets(buffer, 63, status) == NULL) {
			fclose(status);
			continue;
		}
		fclose(status);

		/* Buffer should contain a string like "Name:   binary_name" */
		sscanf(buffer, "%*s %s", name);
		if (strcmp(name, pidName) == 0) {
		//	pidList=xrealloc( pidList, sizeof(pid_t) * (i+2));
			pid=(pid_t)strtol(next->d_name, NULL, 0);
			closedir(dir);
			return pid;
		}
	}	
	closedir(dir);
	return 0;
}

#ifdef CONFIG_ELINKSDK_SUPPORT
static int is_elinksdk_alive(void)
{
	int status = 0;
	rtl_elinksdk_is_alive(&status);
	return status;
}
#endif
static int is_elink_alive(void){
    int pid = -1;
    pid = rtk_find_pid_by_name("elink");
    if(pid > 0)
        return 1;
    else
        return 0;
}
#endif

#ifdef CONFIG_ANDLINK_SUPPORT
static int is_andlink_alive(void)
{
	int andlink_pid=-1;
	unsigned char value[32];

	snprintf(value, 32, "%s", (char*)RTL_LINK_PID_FILE);
	andlink_pid = read_pid((char*)value);
	if(andlink_pid > 0)
		return 1;
	else
		return 0;
}
static int andlink_enable_and_exit(void)
{
	 if((isFileExist(RTL_LINK_PID_FILE)) && (!is_andlink_alive()))
	    return 1;
	 else
	 	return 0;
}
static int is_over_limit(void)
{
	FILE *fp = NULL;
	int a=1;
	if(!(fp = fopen(RTL_LINK_LIMIT_FLIE, "w+"))){
			printf("cannot open file!");
			return 0;
		}
	fprintf(fp,"%d\n",a);
	fclose(fp);
	return 1;
}
#endif

static unsigned int time_count;

#ifdef CONFIG_USER_CTCAPD
void process_roaming_sta_info_report(void)
{
	char *buff, *pchar;
	WLAN_STA_INFO_Tp pInfo;
	int i, w_idx, vw_idx, channel=0, rssi_value=0;
	int wlan_enable;
	int wlan_mode;
	char intfname[16]={0}, cmdBuf[400]={0}, filename[50]={0}, line_buffer[256]={0};
	FILE *fp;
	MIB_CE_MBSSIB_T WlanEntry;
	unsigned char roam_start_time=0, roam_start_rssi=0, status_11k_support=0, status_11v_support=0;	

	buff = calloc(1, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));
	if (buff == 0) 
	{
		printf("Allocate buffer failed!\n");
		return;
	}
	mib_get_s(MIB_RTL_LINK_ROAMING_START_TIME, (void *)&roam_start_time, sizeof(roam_start_time));
	mib_get_s(MIB_RTL_LINK_ROAMING_START_RSSI, (void *)&roam_start_rssi, sizeof(roam_start_rssi));	
	
	mib_save_wlanIdx(); 

	for(w_idx=0; w_idx<NUM_WLAN_INTERFACE; w_idx++)
	{
		for(vw_idx=0; vw_idx<=WLAN_MBSSID_NUM; vw_idx++)
		{
			memset(intfname, 0, sizeof(intfname));
			if(vw_idx == 0)
				snprintf(intfname,sizeof(intfname),WLAN_IF,w_idx);
			else
				snprintf(intfname,sizeof(intfname),VAP_IF,w_idx,vw_idx-1);	
			wlan_idx = w_idx;
			memset(&WlanEntry,0,sizeof(MIB_CE_MBSSIB_T));
			if (!mib_chain_get(MIB_MBSSIB_TBL, vw_idx, (void *)&WlanEntry)){
				mib_recov_wlanIdx();
				free(buff);
				buff = NULL;
				return;
			}

			if(WlanEntry.wlanDisabled == 0 && WlanEntry.wlanMode == AP_MODE && getWlStaInfo(intfname,(WLAN_STA_INFO_Tp)buff) == 0)
			{
				for (i=1; i<=MAX_STA_NUM; i++) 
				{
					pInfo = (WLAN_STA_INFO_Tp)&buff[i*sizeof(WLAN_STA_INFO_T)];
					
					if (pInfo->aid && (pInfo->flags & STA_INFO_FLAG_ASOC)) {
#ifdef CONFIG_USER_ADAPTER_API_ISP
						if(pInfo->rssi < WlanEntry.rtl_link_rssi_th && (pInfo->link_time >= roam_start_time ||  pInfo->rssi > roam_start_rssi))
						{
							//snprintf(filename, sizeof(filename), "proc/%s/mib_rf", intfname);
							snprintf(filename, sizeof(filename), LINK_FILE_NAME_FMT, intfname, "mib_rf");
							if ((fp = fopen(filename, "r")) != NULL) {
									while(fgets(line_buffer, sizeof(line_buffer), fp))
									{
										line_buffer[strlen(line_buffer)-1]='\0';
										
										if((pchar=strstr(line_buffer, "dot11channel:"))!=NULL)
										{					
											pchar+=strlen("dot11channel:");
											channel = atoi(pchar);
											break;
										}		  
									}
									fclose(fp); 		
							}

							if(pInfo->status_support & BIT(0))
								status_11k_support = 1;
							else
								status_11k_support = 0;
							if(pInfo->status_support & BIT(1))
								status_11v_support = 1;
							else
								status_11v_support = 0;

							rssi_value = pInfo->rssi - 100;
							snprintf(cmdBuf, sizeof(cmdBuf), "ubus send \"notification\" \'{\"roamingreport\":[{\"mac\": \"%02X%02X%02X%02X%02X%02X\",\"connecttime\":%ld,\"rssi\":%d,\"ifname\":\"%s\",\"channel\":%d,\"support11k\":%d,\"support11v\":%d}]}\'",
								pInfo->addr[0],pInfo->addr[1],pInfo->addr[2],pInfo->addr[3],pInfo->addr[4],pInfo->addr[5],pInfo->link_time,rssi_value,intfname,channel,status_11k_support,status_11v_support);
							system(cmdBuf);
						}
#endif
					}
				}
			}
			
		}
	}

	mib_recov_wlanIdx();
	free(buff);
	buff = NULL;

}
#endif

#ifdef CONFIG_USER_CTC_EOS_MIDDLEWARE
void rtk_restart_elinkclt()
{
	FILE *fp=NULL;
	int pid_new=0, pid_old=0;
	int ip_dns_changed=0;
	int need_restart_elinkclt=0;
	char opmode=GATEWAY_MODE;
	if(isFileExist((char *)RTK_RESTART_ELINKCLT_FILE))
	{		
#ifdef CONFIG_USER_RTK_BRIDGE_MODE		
		mib_get_s(MIB_OP_MODE, (void *)&opmode, sizeof(opmode));	
#endif
		if(opmode==GATEWAY_MODE)
		{
			pid_old = read_pid(RTK_RESTART_ELINKCLT_FILE);			
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) 
			pid_new = read_pid(DNSRELAYPID);
#endif
			if(pid_new>0 && pid_new!=pid_old)
				need_restart_elinkclt=1;				
		}
		else if(opmode==BRIDGE_MODE)
		{
			if ((fp = fopen(RTK_RESTART_ELINKCLT_FILE, "r")) != NULL) 
			{
				fscanf(fp, "%d", &ip_dns_changed);
				fclose(fp);
				
				if(ip_dns_changed==1 && isFileExist((char *)RESOLV))
					need_restart_elinkclt=1;
			}
		}

		if(need_restart_elinkclt)
		{
			rtk_sys_restart_elinkclt(0, 0);
			unlink(RTK_RESTART_ELINKCLT_FILE);
		}
	}
}
#endif

#if defined(CONFIG_USER_HAPD) && defined(CONFIG_RTK_DNS_TRAP)
void rtk_write_map_uplink_info_to_proc(void)
{
	FILE *fp = NULL;
	char buffer[128] = {0};
	char ifname[IFNAMSIZ] = {0};
	static unsigned int last_link_time = 0, count = 0;
	unsigned int cur_link_time = 0;
	char controller_ip[128] = {0};
	char cmdBuf[128] = {0};
	unsigned char disable = 0;
	
	int connect_flag = 0;
	int ret = 0;
	static int last_connect_flag = 0;
	static char last_controller_ip[128] = {0};

	mib_get_s(MIB_DNSTRAP_TO_CONTROLLER_DISABLE, &disable, sizeof(disable));

	if ((disable == 0) && (fp = fopen(MAP_UPLINK_STATUS_FILE, "r"))) {
		ret=fscanf(fp, "%s", buffer);
		if(ret!=1)
			printf("fscanf error!\n");
		fclose(fp);

		if (strstr(buffer, "linkup")) {
			if ((fp = fopen(MAP_UPLINK_INFO_FILE, "r"))) {
				ret=fscanf(fp, "%s %u %s", ifname, &cur_link_time, controller_ip);
				if(ret < 2) // MAP_UPLINK_INFO_FILE will cat ifname&link_time when connect with non-rtk controller
					printf("fscanf error!\n");
				fclose(fp);

				//printf("\n%s:%d ifname=%s cur_link_time=%u controller_ip=%s\n",__FUNCTION__,__LINE__, ifname, cur_link_time, controller_ip);

				if (cur_link_time > last_link_time && strlen(controller_ip) > 0) {	
					count = 0;
					connect_flag = 1;
					last_link_time = cur_link_time;
				} else if (cur_link_time == last_link_time && strlen(controller_ip) > 0 && count <= MAX_CHECK_MAP_NUM) {
					count++;
					connect_flag = 1;
				}
			}
		}			
	}

	if (last_connect_flag != connect_flag || strcmp(last_controller_ip, controller_ip) != 0) {
		last_connect_flag = connect_flag;
		strncpy(last_controller_ip, controller_ip, sizeof(last_controller_ip)-1);	
	
		snprintf(cmdBuf, sizeof(cmdBuf), "echo %d %s > /proc/driver/realtek/controller_info", connect_flag, controller_ip);
		va_cmd("/bin/sh", 2, 1, "-c", cmdBuf);		
	}
}	
#endif

#ifdef CONFIG_USER_AHSAPD

struct MapEvent{
unsigned int meshRole;
char meshAlmac[13];
unsigned int meshStatus;
char bhType[10];
char bhalmac[13];
char ssid[MAX_SSID_LEN];
char pwd[MAX_PSK_LEN+1];
};

int check_map_change(void)
{
	static char last_link_status[16] = {0}, last_link_interfance[16]={0};
	FILE *fp = NULL;
	unsigned char map_state =0;
	unsigned char  macaddr[6]={0};
	char buff[50] = {0}, link_status[16] = {0}, link_interfance[16]={0};
	struct MapEvent event_info;
	MIB_CE_MBSSIB_T wlan_Entry;
	char cmdBuf[256]={0};
	int vwlan_idx;
	unsigned char mac[6]={0};
    struct sockaddr hwaddr;
	memset(&event_info, 0, sizeof(event_info));
	memset(&wlan_Entry, 0, sizeof(wlan_Entry));

	fp = fopen("/tmp/map_backhaul_link_status", "r");
	if (fp == NULL) {
			return 0;
	}
	fgets(buff, 50, fp);
	fclose(fp);
	if (strstr(buff, " ") != NULL){
	 	if (sscanf(buff, "%s %s", link_status, link_interfance) != 2){
			return 0;
		}
	}else{
		sscanf(buff, "%s", link_status);
	}

	if(strcmp(last_link_status, link_status)||(strcmp(last_link_interfance, link_interfance))){
			mib_get_s(MIB_MAP_CONTROLLER, (void*)&map_state, sizeof(map_state));
			if(MAP_CONTROLLER == map_state){
				event_info.meshRole = MAP_CONTROLLER;
			}
			else if(MAP_AGENT == map_state){
				event_info.meshRole  = MAP_AGENT;
			}else if(MAP_AUTO == map_state){
				event_info.meshRole  = MAP_AUTO;
			}
			getMacAddr("br0", macaddr);
			snprintf((char *)event_info.meshAlmac, sizeof(event_info.meshAlmac),"%02X%02X%02X%02X%02X%02X", macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
			if (strstr(buff, "linkup") != NULL){
				event_info.meshStatus = 1;
			}else{
				event_info.meshStatus = 0;
			}

			if (strstr(buff, "eth") || strstr(buff, "nas0")){
				snprintf(event_info.bhType, sizeof(event_info.bhType), "eth");
				snprintf(event_info.bhalmac, sizeof(event_info.bhalmac), "%s", event_info.meshAlmac);
			}
			else if(strstr(buff,"wlan0"))
			{
#ifdef WLAN1_5G_SUPPORT
				snprintf(event_info.bhType, sizeof(event_info.bhType), "2.4G");
#else
				snprintf(event_info.bhType, sizeof(event_info.bhType), "5G");
#endif
			}
			else if(strstr(buff,"wlan1"))
			{
#ifdef WLAN1_5G_SUPPORT
				snprintf(event_info.bhType, sizeof(event_info.bhType), "5G");
#else
				snprintf(event_info.bhType, sizeof(event_info.bhType), "2.4G");
#endif
			}
			if(strstr(buff,"wlan")){
				if(RTK_SUCCESS == mib_chain_get_wlan((unsigned char *)link_interfance,&wlan_Entry,&vwlan_idx)){
					if(( ENCRYPT_DISABLED != wlan_Entry.encrypt)&&(ENCRYPT_WEP != wlan_Entry.encrypt)){
						snprintf(event_info.pwd, sizeof(event_info.pwd), wlan_Entry.wpaPSK );
					}
					snprintf(event_info.ssid, sizeof(event_info.ssid), "%s", wlan_Entry.ssid);
					if(getInAddr(link_interfance, HW_ADDR, (void *)&hwaddr)){
       					 memcpy(mac, (unsigned char *)hwaddr.sa_data, 6);
        				 snprintf(event_info.bhalmac, 13, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    				}
				}
			}
			snprintf(cmdBuf, sizeof(cmdBuf), "ubus send \"meshInfo\" \'{\"meshRole\":%d,\"meshAlmac\":\"%s\",\"meshStatus\":%d,\"bhInfo\":\'{\"bhType\":\"%s\",\"bhalmac\":\"%s\",\"ssid\":\"%s\",\"pwd\":\"%s\"}\'}\'",
			event_info.meshRole, event_info.meshAlmac, event_info.meshStatus, event_info.bhType, event_info.bhalmac, event_info.ssid, event_info.pwd);
			if(cmdBuf[0])
				system(cmdBuf);
	}

	strncpy(last_link_status, link_status, sizeof(last_link_status));
	strncpy(last_link_interfance, link_interfance, sizeof(last_link_interfance));
	return 0;
}
#endif

void timeout_handler() 
{
#ifdef CONFIG_USER_CTCAPD
	unsigned char roam_switch=0, roam_qos=0, roam_report_interval=0;
#endif
	time_count++;

#ifdef CONFIG_ELINK_SUPPORT
#ifdef CONFIG_ELINKSDK_SUPPORT
		if(!(time_count%CHECK_ELINKSDK_TIMEOUT) && rtl_elinksdk_enabled_timelycheck())
		{
			if(!is_elinksdk_alive())
 	{
				rtl_elinksdk_start();
			}		
		}
#endif		
		if(!(time_count%CHECK_ELINKSDK_TIMEOUT) && isFileExist(RTL_LINK_PID_FILE)){
			if(!is_elink_alive())
 	{
				system("elink -m1 -d2");
			}
	}
#endif
#ifdef CONFIG_ANDLINK_SUPPORT
	if(!(time_count%CHECK_ANDLINK_TIMEOUT))
	{		
		if(andlink_enable_and_exit())
		{
			system("andlink -m1 -d2");
		}		
	}
	if(!(time_count%CHECK_OVER_LIMIT_TIMEOUT))
		is_over_limit();
#endif

#ifdef CONFIG_USER_CTCAPD
	mib_get_s(MIB_RTL_LINK_ROAMING_SWITCH, (void *)&roam_switch, sizeof(roam_switch));
	mib_get_s(MIB_RTL_LINK_ROAMING_QOS, (void *)&roam_qos, sizeof(roam_qos));
	mib_get_s(MIB_RTL_LINK_ROAMING_DETEC_PERIOD, (void *)&roam_report_interval, sizeof(roam_report_interval));
	
	if(roam_report_interval == 0)
	{
		roam_report_interval = 30;
	}
	
	if(roam_switch && roam_qos && !(time_count%roam_report_interval))
		process_roaming_sta_info_report();
#endif

#ifdef CONFIG_USER_CTC_EOS_MIDDLEWARE
	rtk_restart_elinkclt();
#endif

#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)	
	if(!(time_count % CHECK_LOAD_BALANCE))
	{	
		rtk_timely_check_load_balance();	
	}

	if(!(time_count % DELETE_UNEXIST_STA_SAVE_INFO))
	{	
		rtk_timely_delete_unexist_save_info();	
	}
#endif

#if defined(CONFIG_USER_HAPD) && defined(CONFIG_RTK_DNS_TRAP)
	if(!(time_count % CHECK_MAP_UPLINK_INFO_TIMEOUT))
		rtk_write_map_uplink_info_to_proc();
#endif

#ifdef CONFIG_USER_AHSAPD
	check_map_change();
#endif

}

int main(int argc, char** argv)
{
	while(1)
	{
		timeout_handler();
		sleep(1);
	}
}


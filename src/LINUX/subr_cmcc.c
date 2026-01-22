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
#include <sys/file.h>
#include <stdarg.h>
#include <errno.h>

#include "rtk_timer.h"
#include "mib.h"
#include "utility.h"
#include "subr_net.h"
#include "subr_switch.h"
#include "debug.h"
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#include <rtk/sockmark_define.h>
#include "fc_api.h"
#include <rt_flow_ext.h>
#include <rtk/rt/rt_l2.h>
#include <rtk/rt/rt_switch.h>
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#include <rtk/iconv.h>
#endif
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/syscall.h>
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#include "cJSON.h"
#endif

#ifdef CONFIG_CMCC_WIFI_PORTAL
int g_wan_modify=0;

static int expiretimeval=-1;
static int accmode=0;
static int tmp_acc_timeout=-1;

#endif

#if defined(CONFIG_CU_BASEON_YUEME)
const char TABLE_TRAFFIC_MONITOR_FWD[] = "trafficmonitor_fwd";
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)

const char TABLE_TRAFFIC_MIRROR_FWD[] = "trafficmirror_fwd";
const char TABLE_TRAFFIC_MIRROR_POST[] = "trafficmirror_post";
const char TABLE_TRAFFIC_PROCESS_FWD[] = "trafficprocess_fwd";
const char TABLE_TRAFFIC_PROCESS_POST[] = "trafficprocess_post";
const char TABLE_TRAFFIC_QOS_FWD[] = "trafficqos_fwd";
const char TABLE_TRAFFIC_QOS_POST[] = "trafficqos_post";
const char TABLE_TRAFFIC_QOS_EB[] = "trafficqos_eb";

const char  *osgi_capbilities_long[] =
{
//TODO: Get all package names dynamically?
"com.chinamobile.smartgateway.accessservices.AccessInfoQueryService",
"com.chinamobile.smartgateway.accessservices.WanConfigService",
"com.chinamobile.smartgateway.accessservices.IPPingDiagnosticsService",
"com.chinamobile.smartgateway.accessservices.TraceRouteDiagnosticsService",
"com.chinamobile.smartgateway.accessservices.IPDetectService",
"com.chinamobile.smartgateway.transferservices.TransferQueryService",
"com.chinamobile.smartgateway.transferservices.VLANBindConfigService",
"com.chinamobile.smartgateway.transferservices.StaticLayer3ForwardingConfigService",
"com.chinamobile.smartgateway.transferservices.TrafficMonitoringConfigService",
"com.chinamobile.smartgateway.transferservices.TrafficQoSService",
"com.chinamobile.smartgateway.transferservices.TrafficForwardService",
"com.chinamobile.smartgateway.transferservices.TrafficMirrorService",
"com.chinamobile.smartgateway.transferservices.TrafficDetailProcessService",
"com.chinamobile.smartgateway.transferservices.L2TPVPNService",
"com.chinamobile.smartgateway.lanservices.LANHostsInfoQueryService",
"com.chinamobile.smartgateway.lanservices.LanNetworkNameConfigService",
"com.chinamobile.smartgateway.lanservices.LanHostSpeedLimitService",
"com.chinamobile.smartgateway.lanservices.LanHostSpeedQueryService",
"com.chinamobile.smartgateway.lanservices.LanNetworkAccessConfigService",
"com.chinamobile.smartgateway.lanservices.WlanQueryService",
"com.chinamobile.smartgateway.lanservices.WlanSecretQueryService",
"com.chinamobile.smartgateway.lanservices.WlanConfigService",
"com.chinamobile.smartgateway.lanservices.EthQueryService",
"com.chinamobile.smartgateway.lanservices.EthConfigService",
"com.chinamobile.smartgateway.addressservices.LANIPInfoQueryService",
"com.chinamobile.smartgateway.addressservices.LANIPConfigService",
"com.chinamobile.smartgateway.addressservices.PortMappingQueryService",
"com.chinamobile.smartgateway.addressservices.PortMappingConfigService",
"com.chinamobile.smartgateway.voipservices.VoIPInfoQueryService",
"com.chinamobile.smartgateway.voipservices.VoIPInfoSubscribeService",
"com.chinamobile.smartgateway.commservices.DeviceInfoQueryService",
"com.chinamobile.smartgateway.commservices.DeviceSecretInfoQueryService",
"com.chinamobile.smartgateway.commservices.DeviceAccessRightQueryService",
"com.chinamobile.smartgateway.commservices.DeviceAccessRightConfigService",
"com.chinamobile.smartgateway.commservices.DeviceRsetService",
"com.chinamobile.smartgateway.commservices.UsbService",
"com.chinamobile.smartgateway.commservices.DeviceConfigService",
"com.chinamobile.smartgateway.commservices.RegistrationService",
"com.chinamobile.smartgateway.accessservices.HttpDownloadDiagnosticsService",
"com.chinamobile.smartgateway.accessservices.HttpUploadDiagnosticsService", //40
"com.chinamobile.smartgateway.accessservices.wanconfigservice.DDNSConfiguration",
"com.chinamobile.smartgateway.accessservices.wanconfigservice.IPSecVPNserver",	
"com.chinamobile.smartgateway.accessservices.wanconfigservice.L2TPVPNServer",
"com.chinamobile.smartgateway.commservices.DeviceBackupService",
"com.chinamobile.smartgateway.lanservices.PortalConfigService",
"com.chinamobile.smartgateway.voipservices.VoIPInfoConfigService",
"com.chinamobile.smartgateway.commservices.AlarmServices",
"com.chinamobile.smartgateway.commservices.SecurityReviewServices",
"com.chinamobile.smartgateway.commservices.WlanSniffingServices",
"com.chinamobile.smartgateway.ekitplugin.PluginListService",

/* Add packages before this line */
NULL
};

const char  *osgi_capbilities_short[] =
{
//TODO: Get all package names dynamically?
"accessservices.AccessInfoQueryService",
"accessservices.WanConfigService",
"accessservices.IPPingDiagnosticsService",
"accessservices.TraceRouteDiagnosticsService",
"accessservices.IPDetectService",
"transferservices.TransferQueryService",
"transferservices.VLANBindConfigService",
"transferservices.StaticLayer3ForwardingConfigService",
"transferservices.TrafficMonitoringConfigService",
"transferservices.TrafficQoSService",
"transferservices.TrafficForwardService",
"transferservices.TrafficMirrorService",
"transferservices.TrafficDetailProcessService",
"transferservices.L2TPVPNService",
"lanservices.LANHostsInfoQueryService",
"lanservices.LanNetworkNameConfigService",
"lanservices.LanHostSpeedLimitService",
"lanservices.LanHostSpeedQueryService",
"lanservices.LanNetworkAccessConfigService",
"lanservices.WlanQueryService",
"lanservices.WlanSecretQueryService",
"lanservices.WlanConfigService",
"lanservices.EthQueryService",
"lanservices.EthConfigService",
"addressservices.LANIPInfoQueryService",
"addressservices.LANIPConfigService",
"addressservices.PortMappingQueryService",
"addressservices.PortMappingConfigService",
"voipservices.VoIPInfoQueryService",
"voipservices.VoIPInfoSubscribeService",
"commservices.DeviceInfoQueryService",
"commservices.DeviceSecretInfoQueryService",
"commservices.DeviceAccessRightQueryService",
"commservices.DeviceAccessRightConfigService",
"commservices.DeviceRsetService",
"commservices.UsbService",
"commservices.DeviceConfigService",
"commservices.RegistrationService",
"accessservices.HttpDownloadDiagnosticsService",
"accessservices.HttpUploadDiagnosticsService", //40
"accessservices.wanconfigservice.DDNSConfiguration",
"accessservices.wanconfigservice.IPSecVPNserver",	
"accessservices.wanconfigservice.L2TPVPNServer",
"commservices.DeviceBackupService",
"lanservices.PortalConfigService",
"voipservices.VoIPInfoConfigService",
"commservices.AlarmServices",
"commservices.SecurityReviewServices",
"commservices.WlanSniffingServices",
"ekitplugin.PluginListService",

/* Add packages before this line */
NULL
};

int getIPFromMAC( char *mac, char *ip )
{
	int	ret=-1;
	FILE 	*fh;
	char 	buf[128];

	if( (mac==NULL) || (ip==NULL) )	return ret;
	ip[0]=0;

	fh = fopen("/proc/net/arp", "r");
	if (!fh) return ret;

	fgets(buf, sizeof buf, fh);	/* eat line */
	//fprintf( stderr, "%s\n", buf );
	while (fgets(buf, sizeof buf, fh))
	{
		char cip[32],cmac[32];

		//fprintf( stderr, "%s\n", buf );
		//format: IP address       HW type     Flags       HW address            Mask     Device
		if( sscanf(buf,"%s %*s %*s %s %*s %*s", cip,cmac)!=2 )
			continue;

		//fprintf( stderr, "mac:%s, cmac:%s, cip:%s\n", mac, cmac, cip );
		if( strcasecmp( mac, cmac )==0 )
		{
			strcpy( ip, cip );
			ret=0;
			break;
		}
	}
	fclose(fh);
	return ret;
}

void rtk_web_get_commanum_substr(int* comma_num, char* name, unsigned char item[][32])
{
	char* pStart=name ,*pCur=name;
	int i=0, j=0; 

	for(; i<strlen(name); i++) 
	{   
		if(name[i]==',')
		{    
			pStart = name+i;
			strncpy(item[j], pCur, pStart-pCur);
			*comma_num+=1, j++, pCur=name+i+1;
		}
	}
	item[j][sizeof(item[j])-1]='\0';
	strncpy(item[j], pCur,sizeof(item[j])-1);
}

#ifdef CONFIG_USER_OPENJDK8
#ifdef CONFIG_CMCC_OSGIMANAGE
inline int startOsgiManage(void)
{
	return setupOsgiManage(OSGIMANAGE_ENABLED);
}

int setupOsgiManage(int type)
{
	int pid;
	DOCMDINIT;
	//DOCMDARGVS("/bin/killall", DOWAIT, "%s", "osgiManage");
	switch (type) {
	case OSGIMANAGE_DISABLED:
		pid = read_pid(OSGIMANAGE_PID);
		if (pid) {
			DOCMDARGVS("/bin/kill", DOWAIT, "%d", pid);
			va_cmd("/bin/rm", 1, 1, OSGIMANAGE_PID);
		}
		pid = read_pid(OSGIAPP_PID);
		if (pid) {
			DOCMDARGVS("/bin/kill", DOWAIT, "%d", pid);
			va_cmd("/bin/rm", 1, 1, OSGIAPP_PID);
		}
		break;
	case OSGIMANAGE_ENABLED:
#if defined(CONFIG_RTK_IPV6_LOGO_TEST)
		unsigned char logoTestMode=0;
		mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode));
#endif
		pid =  read_pid(OSGIMANAGE_PID);
		if (pid <= 0) { // renew
#if defined(CONFIG_RTK_IPV6_LOGO_TEST)
			if (logoTestMode)
				va_cmd("/bin/osgiManage", 0, UNDOWAIT);
#else
			DOCMDARGVS("/bin/osgiManage", UNDOWAIT, "%s", "debugoff");
#endif
		}
		pid =  read_pid(OSGIAPP_PID);
		if (pid <= 0) { // renew
#if defined(CONFIG_RTK_IPV6_LOGO_TEST)
			if (logoTestMode)
				va_cmd("/bin/osgiApp", 0, UNDOWAIT);
#else
			DOCMDARGVS("/bin/osgiApp", UNDOWAIT, "%s", "debugoff");
#endif
		}
		break;
	}
	return 1;
}

int restartOsgiApp()
{
	int pid;
#if defined(CONFIG_RTK_IPV6_LOGO_TEST)
	unsigned char logoTestMode=0;
	mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode));
#endif

	DOCMDINIT;

	pid = read_pid(OSGIAPP_PID);
	if (pid) {
		DOCMDARGVS("/bin/kill", DOWAIT, "%d", pid);
		va_cmd("/bin/rm", 1, 1, OSGIAPP_PID);
	}

	sleep(1);

	pid =  read_pid(OSGIAPP_PID);
	if (pid <= 0) { // renew
#if defined(CONFIG_RTK_IPV6_LOGO_TEST)
		if (logoTestMode)
			va_cmd("/bin/osgiApp", 0, UNDOWAIT);
#else
		DOCMDARGVS("/bin/osgiApp", UNDOWAIT, "%s", "debugoff");
#endif
	}
	return 1;
}

#endif
#endif

#if (defined(CONFIG_CMCC) && defined(CONFIG_APACHE_FELIX_FRAMEWORK)) || defined(CONFIG_CU)
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
int rtk_cmcc_init_traffic_mirror_rule(void)
{
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", TABLE_TRAFFIC_MIRROR_FWD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", TABLE_TRAFFIC_MIRROR_POST);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", TABLE_TRAFFIC_MIRROR_FWD);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", TABLE_TRAFFIC_MIRROR_POST);
#endif

	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-p", "tcp", "-j", TABLE_TRAFFIC_MIRROR_FWD);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-p", "udp", "-j", TABLE_TRAFFIC_MIRROR_FWD);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-p", "tcp", "-j", TABLE_TRAFFIC_MIRROR_POST);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-p", "udp", "-j", TABLE_TRAFFIC_MIRROR_POST);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-p", "tcp", "-j", TABLE_TRAFFIC_MIRROR_FWD);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-p", "udp", "-j", TABLE_TRAFFIC_MIRROR_FWD);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-p", "tcp", "-j", TABLE_TRAFFIC_MIRROR_POST);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-p", "udp", "-j", TABLE_TRAFFIC_MIRROR_POST);
#endif

	return 0;
}

int rtk_cmcc_flush_traffic_mirror_rule(void){
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", TABLE_TRAFFIC_MIRROR_FWD);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-p", "tcp", "-j", TABLE_TRAFFIC_MIRROR_FWD);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-p", "udp", "-j", TABLE_TRAFFIC_MIRROR_FWD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", TABLE_TRAFFIC_MIRROR_FWD);

	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", TABLE_TRAFFIC_MIRROR_POST);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-p", "tcp", "-j", TABLE_TRAFFIC_MIRROR_POST);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-p", "udp", "-j", TABLE_TRAFFIC_MIRROR_POST);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", TABLE_TRAFFIC_MIRROR_POST);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", TABLE_TRAFFIC_MIRROR_FWD);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-p", "tcp", "-j", TABLE_TRAFFIC_MIRROR_FWD);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-p", "udp", "-j", TABLE_TRAFFIC_MIRROR_FWD);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", TABLE_TRAFFIC_MIRROR_FWD);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", TABLE_TRAFFIC_MIRROR_POST);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-p", "tcp", "-j", TABLE_TRAFFIC_MIRROR_POST);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-p", "udp", "-j", TABLE_TRAFFIC_MIRROR_POST);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", TABLE_TRAFFIC_MIRROR_POST);
#endif
	return 0;
}


#if defined(CONFIG_CMCC) || defined (CONFIG_CU_BASEON_CMCC)
int rtk_cmcc_setup_traffic_mirror_init
(
	MIB_CE_MIRROR_RULE_T *entry,
	char *realip,
	char *condition_string,
	char *condition_string_down,
	char *connbytes_string,
	char *connbytes_string1,
	char *rsniffer_condition_string,
	int trap_first_n_pkts_to_ps,
	int type,
	int ruleid
)
{
	int iptype = 0;
	char ipstr[INET6_ADDRSTRLEN] = {0};
	int iprangetype=0;
	char tmp[128] = {0};
	char tmp_down[128] = {0};
	char rsniffer_tmp[128] = {0};
	char ipaddr1[64]={0}, ipaddr2[64] ={0};
	int entryNum, loop;
	MIB_CE_ATM_VC_T atm_entry;
	char wan_ifname[IFNAMSIZ+1];
	char empty_mac[MAC_ADDR_LEN] = {0};

	if(type == CMCC_FORWARDRULE_ADD){
		sprintf(rsniffer_condition_string, "-t 0 -i %d", ruleid);		
	}
	else{
		sprintf(rsniffer_condition_string, "-t 1 -i %d", ruleid);	
	}

	if(entry->direction == DIR_OUT){
		strcpy(rsniffer_tmp, " -d up");
	}
	else if(entry->direction == DIR_IN){
		strcpy(rsniffer_tmp, " -d down");
	}
	else{
		strcpy(rsniffer_tmp, " -d all");
	}
	strcat(rsniffer_condition_string, rsniffer_tmp);

	if(entry->mirror_to_ipver ==IPVER_IPV4){
		iptype = checkIPv4OrIPv6(realip, inet_ntoa(*((struct in_addr *)entry->mirror_to_ip)));
	}
	else{
		inet_ntop(AF_INET6, entry->mirror_to_ip6, ipstr, sizeof(ipstr));
		iptype = checkIPv4OrIPv6(realip, ipstr);
	}
	iprangetype = getIpRange(realip, ipaddr1, ipaddr2);
	printf("%s:%d iptype %d iprangetype %d\n", __FUNCTION__, __LINE__, iptype, iprangetype);
		
	//protocol, remoteAddress
	memset(tmp, 0, 128);
	if(entry->protocol == PROTO_TCP){
		sprintf(tmp, "-p TCP");
		sprintf(tmp_down, "-p TCP");
		sprintf(rsniffer_tmp, " -r tcp");
	}
	else{
		sprintf(tmp, "-p UDP");
		sprintf(tmp_down, "-p UDP");
		sprintf(rsniffer_tmp, " -r udp");
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
	strcat(rsniffer_condition_string, rsniffer_tmp);
	
	//remoteAddress
	memset(tmp, 0, 128);
	memset(tmp_down, 0, 128);
	memset(rsniffer_tmp, 0, 128);
	if(!strcmp(entry->remote_ip, "") || !strcmp(entry->remote_ip, "0")){
		strcpy(tmp, "");
		strcpy(tmp_down, "");
		strcpy(rsniffer_tmp, "");
	}
	else{
		if(iprangetype==1){
			sprintf(tmp, " -d %s", realip);
			sprintf(tmp_down, " -s %s", realip);
		}
		else if(iprangetype==2){
			sprintf(tmp, " -m iprange --dst-range %s-%s", ipaddr1, ipaddr2);
			sprintf(tmp_down, " -m iprange --src-range %s-%s", ipaddr1, ipaddr2);
		}
		sprintf(rsniffer_tmp, " -h %s", realip);
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
	strcat(rsniffer_condition_string, rsniffer_tmp);
	
	//remote port
	memset(tmp, 0, 128);
	memset(tmp_down, 0, 128);
	memset(rsniffer_tmp, 0, 128);
	if(entry->portNot==1){
		sprintf(tmp, " ! --dport %d", entry->remote_port_start);
		sprintf(tmp_down, " ! --sport %d", entry->remote_port_start);
		sprintf(rsniffer_tmp, " -p !%d", entry->remote_port_start);
	}
	else if(entry->remote_port_start == 0){
		strcpy(tmp, "");
		strcpy(tmp_down, "");
		strcpy(rsniffer_tmp, "");
	}
	else{
		sprintf(tmp, " --dport %d:%d", entry->remote_port_start, entry->remote_port_end);
		sprintf(tmp_down, " --sport %d:%d", entry->remote_port_start, entry->remote_port_end);
		sprintf(rsniffer_tmp, " -p %d-%d", entry->remote_port_start, entry->remote_port_end);
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
	strcat(rsniffer_condition_string, rsniffer_tmp);

	if(trap_first_n_pkts_to_ps != 0){
		memset(tmp, 0, 128);
		sprintf(tmp, "-m connbytes --connbytes 0:%d --connbytes-dir both --connbytes-mode packets", trap_first_n_pkts_to_ps);
		strcat(connbytes_string, tmp);

		memset(tmp, 0, 128);
		sprintf(tmp, "-m connbytes --connbytes %d --connbytes-dir both --connbytes-mode packets", trap_first_n_pkts_to_ps);
		strcat(connbytes_string1, tmp);
	}

	//tcp condition
	memset(rsniffer_tmp, 0, 128);
	if(1 == entry->tcpHeadOnly){
		sprintf(rsniffer_tmp, " -s 1");
	}
	strcat(rsniffer_condition_string, rsniffer_tmp);
	memset(rsniffer_tmp, 0, 128);
	if(1 == entry->tcpRuleReport){
		sprintf(rsniffer_tmp, " -a %d", entry->entry_index);
	}
	strcat(rsniffer_condition_string, rsniffer_tmp);
	
	//hostMAC
	//down string does not need hostmac
	memset(tmp, 0, 128);
	memset(tmp_down, 0, 128);
	memset(rsniffer_tmp, 0, 128);
	if(!memcmp(entry->hostmac, empty_mac, MAC_ADDR_LEN)){
		strcpy(tmp, "");
		strcpy(tmp_down, "");
		strcpy(rsniffer_tmp, "");
	}
	else{
		sprintf(tmp, " -m mac --mac-source %02x:%02x:%02x:%02x:%02x:%02x", 
			entry->hostmac[0],entry->hostmac[1],entry->hostmac[2],entry->hostmac[3],entry->hostmac[4],entry->hostmac[5]);
		sprintf(rsniffer_tmp, " -m %02x:%02x:%02x:%02x:%02x:%02x",
			entry->hostmac[0],entry->hostmac[1],entry->hostmac[2],entry->hostmac[3],entry->hostmac[4],entry->hostmac[5]);
	}
	strcat(condition_string, tmp);
	strcat(rsniffer_condition_string, rsniffer_tmp);

	if(entry->mirror_to_ipver ==IPVER_IPV4){
		sprintf(rsniffer_tmp, " -H %s", inet_ntoa(*((struct in_addr *)entry->mirror_to_ip)));
	}
	else{
		inet_ntop(AF_INET6, entry->mirror_to_ip6, ipstr, sizeof(ipstr));
		sprintf(rsniffer_tmp, " -H %s", ipstr);
	}
	
	strcat(rsniffer_condition_string, rsniffer_tmp);
	sprintf(rsniffer_tmp, " -P %d", entry->mirror_to_port);
	strcat(rsniffer_condition_string, rsniffer_tmp);

	if(entry->remote_port_start == 53){
		//mirror dns pkts on wan port, bundle need get real dns server address
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);

		for(loop = 0; loop < entryNum; loop++){
			if(mib_chain_get(MIB_ATM_VC_TBL, loop, (void*)&atm_entry)){
				if(atm_entry.applicationtype&X_CT_SRV_INTERNET){
					ifGetName(atm_entry.ifIndex, wan_ifname, sizeof(wan_ifname));
					sprintf(rsniffer_tmp, " -I %s", wan_ifname);
					printf("%s:%d mirror to port 53, set to intf %s\n", __FUNCTION__, __LINE__, wan_ifname);
					strcat(rsniffer_condition_string, rsniffer_tmp);
					break;
				}
			}
		}
	}

	printf("%s:%d iptype %d\n", __FUNCTION__, __LINE__, iptype);

	return iptype;
}

/*
op=1 ==> add
op=0 ==> del
*/
int rtk_cmcc_setup_traffic_mirror_rule(MIB_CE_MIRROR_RULE_T *entry, int type)
{
	unsigned char dpi_test = 0;
#ifdef CONFIG_RTK_SKB_MARK2
	unsigned char mark0[64], mark[64];
	int remoteporttype = -1, startport = 0, endport = 0;
	char condition_string[512] = {0};
	char condition_string_down[512] = {0};
	char rsniffer_condition_string[512] = {0};
	char connbytes_string[512] = {0};
	char connbytes_string1[512] = {0};
	char cmd[512] = {0};
	int iptype = 0;
	unsigned int trap_first_n_pkts_to_ps = 0;
	int ruleid = 0;
	
	sprintf(mark0, "0x0/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);
	sprintf(mark, "0x%llx/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK, SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);

	mib_get_s(MIB_TRAP_FIRST_N_PKTS_TO_PS, (void *)&trap_first_n_pkts_to_ps, sizeof(trap_first_n_pkts_to_ps));
	//bundle's mirror count has higher priority.
	if(0 != entry->mir_cnt){
		trap_first_n_pkts_to_ps = entry->mir_cnt;
	}
	
	DOCMDINIT;
	
	printf("%s:%d type %d\n", __FUNCTION__, __LINE__, type);
	printf("%s:%d remote_ip %s, real_remote_ip4 %s, real_remote_ip6 %s, remote_port_start %d, remote_port_end %d, direction %d,"\
		"protocol %d, hostmac %02x:%02x:%02x:%02x:%02x:%02x, mirror_to_port %d, portNot %d\n", __FUNCTION__, __LINE__, 
		entry->remote_ip, 
		entry->real_remote_ip4, 
		entry->real_remote_ip6, 
		entry->remote_port_start,
		entry->remote_port_end,
		entry->direction,
		entry->protocol,
		entry->hostmac[0],entry->hostmac[1],entry->hostmac[2],entry->hostmac[3],entry->hostmac[4],entry->hostmac[5],
		entry->mirror_to_port,
		entry->portNot);

	if((entry->containip&CONTAINS_IPV4) || (entry->containip == CONTAINS_IPNULL)){
		memset(rsniffer_condition_string, 0, sizeof(rsniffer_condition_string));
		memset(condition_string, 0, sizeof(condition_string));
		memset(condition_string_down, 0, sizeof(condition_string_down));
		memset(connbytes_string, 0, sizeof(connbytes_string));
		memset(connbytes_string1, 0, sizeof(connbytes_string1));
		ruleid = (entry->entry_index)*2;

		iptype = rtk_cmcc_setup_traffic_mirror_init(entry, entry->real_remote_ip4, condition_string, condition_string_down, 
			connbytes_string, connbytes_string1, rsniffer_condition_string, trap_first_n_pkts_to_ps, type, ruleid);

		if((iptype == 0 || iptype == 1 || iptype == 3 || iptype == 4) || (entry->containip == CONTAINS_IPNULL)) {
			if((entry->direction == DIR_OUT) || (entry->direction == DIR_ALL)){
				sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
					CMCC_FORWARDRULE_ACTION_APPEND(type), 
					(char *)TABLE_TRAFFIC_MIRROR_FWD, 
					(char *)BRIF, 
					connbytes_string,
					condition_string, 
					mark);
				DOCMDARGVS(IPTABLES, DOWAIT, cmd);
			}

			if((entry->direction == DIR_IN) || (entry->direction == DIR_ALL)){
				sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
					CMCC_FORWARDRULE_ACTION_APPEND(type), 
					(char *)TABLE_TRAFFIC_MIRROR_FWD, 
					(char *)BRIF, 
					connbytes_string,
					condition_string_down, 
					mark);
				DOCMDARGVS(IPTABLES, DOWAIT, cmd);
			}

			if(trap_first_n_pkts_to_ps != 0){
				if((entry->direction == DIR_OUT) || (entry->direction == DIR_ALL)){
					sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
						CMCC_FORWARDRULE_ACTION_APPEND(type), 
						(char *)TABLE_TRAFFIC_MIRROR_FWD, 
						(char *)BRIF,
						connbytes_string1,
						condition_string, 
						mark0);
					DOCMDARGVS(IPTABLES, DOWAIT, cmd);
				}

				if((entry->direction == DIR_IN) || (entry->direction == DIR_ALL)){
					sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
						CMCC_FORWARDRULE_ACTION_APPEND(type), 
						(char *)TABLE_TRAFFIC_MIRROR_FWD, 
						(char *)BRIF, 
						connbytes_string1,
						condition_string_down, 
						mark0);
					DOCMDARGVS(IPTABLES, DOWAIT, cmd);
				}
			}
		}
		
		if(strcmp(entry->real_remote_ip4, "8.8.8.8") != 0){
			DOCMDARGVS("/bin/rsniffer_client", DOWAIT, rsniffer_condition_string);
		}
	}
#ifdef CONFIG_IPV6
	if((entry->containip&CONTAINS_IPV6) || (entry->containip == CONTAINS_IPNULL)){
		memset(rsniffer_condition_string, 0, sizeof(rsniffer_condition_string));
		memset(condition_string, 0, sizeof(condition_string));
		memset(condition_string_down, 0, sizeof(condition_string_down));
		memset(connbytes_string, 0, sizeof(connbytes_string));
		memset(connbytes_string1, 0, sizeof(connbytes_string1));
		ruleid = (entry->entry_index)*2 + 1;
	
		iptype = rtk_cmcc_setup_traffic_mirror_init(entry, entry->real_remote_ip6, condition_string, condition_string_down, 
			connbytes_string, connbytes_string1, rsniffer_condition_string, trap_first_n_pkts_to_ps, type, ruleid);
	
		if((iptype == 0 || iptype == 2 || iptype == 6 || iptype == 7 || iptype == 8) || (entry->containip == CONTAINS_IPNULL)){
			if((entry->direction == DIR_OUT) || (entry->direction == DIR_ALL)){
				sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
					CMCC_FORWARDRULE_ACTION_APPEND(type), 
					(char *)TABLE_TRAFFIC_MIRROR_FWD, 
					(char *)BRIF, 
					connbytes_string,
					condition_string, 
					mark);
				DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
			}

			if((entry->direction == DIR_IN) || (entry->direction == DIR_ALL)){
				sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
					CMCC_FORWARDRULE_ACTION_APPEND(type), 
					(char *)TABLE_TRAFFIC_MIRROR_FWD, 
					(char *)BRIF, 
					connbytes_string,
					condition_string_down, 
					mark);
				DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
			}

			if(trap_first_n_pkts_to_ps != 0){
				if((entry->direction == DIR_OUT) || (entry->direction == DIR_ALL)){
					sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
						CMCC_FORWARDRULE_ACTION_APPEND(type), 
						(char *)TABLE_TRAFFIC_MIRROR_FWD, 
						(char *)BRIF, 
						connbytes_string1,
						condition_string, 
						mark0);
					DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
				}

				if((entry->direction == DIR_IN) || (entry->direction == DIR_ALL)){
					sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
						CMCC_FORWARDRULE_ACTION_APPEND(type), 
						(char *)TABLE_TRAFFIC_MIRROR_FWD, 
						(char *)BRIF, 
						connbytes_string1,
						condition_string_down, 
						mark0);
					DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
				}
			}
		}

		if (entry->containip != CONTAINS_IPNULL){ //if remote ip is null, rsniffer cmd only execute once
			if(strcmp(entry->real_remote_ip4, "8.8.8.8") != 0){
				DOCMDARGVS("/bin/rsniffer_client", DOWAIT, rsniffer_condition_string);
			}
		}
	}
#endif //CONFIG_IPV6
#endif //CONFIG_RTK_SKB_MARK2

#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	mib_get(MIB_DPI_TEST, (void *)&dpi_test);
	if(dpi_test)
		system("echo 1 > /proc/fc/ctrl/rstconntrack");
#endif	

	return 0;
}

#else
/*
op=1 ==> add
op=0 ==> del
*/
int rtk_cmcc_setup_traffic_mirror_rule(MIB_CE_MIRROR_RULE_T *entry, int type)
{
#ifdef CONFIG_RTK_SKB_MARK2
	unsigned char mark0[64], mark[64];
	int remoteporttype = -1, startport = 0, endport = 0;
	char condition_string[512] = {0};
	char condition_string_down[512] = {0};
	char connbytes_string[512] = {0};
	char connbytes_string1[512] = {0};
	char rsniffer_condition_string[512] = {0};
	char cmd[512] = {0};
	char tmp[128] = {0};
	char tmp_down[128] = {0};
	char rsniffer_tmp[128] = {0};
	int iptype = 0;
	int iprangetype=0;
	char ipaddr1[64]={0}, ipaddr2[64] ={0};
	unsigned int trap_first_n_pkts_to_ps = 0;
	char empty_mac[MAC_ADDR_LEN] = {0};
	char ipstr[INET6_ADDRSTRLEN] = {0};

	memset(condition_string, 0, sizeof(condition_string));
	memset(condition_string_down, 0, sizeof(condition_string_down));
	memset(rsniffer_condition_string, 0, sizeof(rsniffer_condition_string));
	sprintf(mark0, "0x0/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);
	sprintf(mark, "0x%llx/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK, SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);

	if(type == CMCC_FORWARDRULE_ADD){
		sprintf(rsniffer_condition_string, "-t 0 -i %d", entry->entry_index);		
	}
	else{
		sprintf(rsniffer_condition_string, "-t 1 -i %d", entry->entry_index);	
	}

	if(entry->direction == DIR_OUT){
		strcpy(rsniffer_tmp, " -d up");
	}
	else if(entry->direction == DIR_IN){
		strcpy(rsniffer_tmp, " -d down");
	}
	else{
		strcpy(rsniffer_tmp, " -d all");
	}
	strcat(rsniffer_condition_string, rsniffer_tmp);
	
	DOCMDINIT;
	
	printf("%s:%d type %d\n", __FUNCTION__, __LINE__, type);
	printf("%s:%d remote_ip %s, real_remote_ip %s, remote_port_start %d, remote_port_end %d, direction %d,"\
		"protocol %d, hostmac %02x:%02x:%02x:%02x:%02x:%02x, mirror_to_ip %s, mirror_to_port %d, portNot %d\n", __FUNCTION__, __LINE__, 
		entry->remote_ip, 
		entry->real_remote_ip, 
		entry->remote_port_start,
		entry->remote_port_end,
		entry->direction,
		entry->protocol,
		entry->hostmac[0],entry->hostmac[1],entry->hostmac[2],entry->hostmac[3],entry->hostmac[4],entry->hostmac[5],
		entry->mirror_to_ip,
		entry->mirror_to_port,
		entry->portNot);

	if(entry->mirror_to_ipver ==IPVER_IPV4){
		iptype = checkIPv4OrIPv6(entry->real_remote_ip, inet_ntoa(*((struct in_addr *)entry->mirror_to_ip)));
	}
	else{
		inet_ntop(AF_INET6, entry->mirror_to_ip6, ipstr, sizeof(ipstr));
		iptype = checkIPv4OrIPv6(entry->real_remote_ip, ipstr);
	}
	iprangetype = getIpRange(entry->real_remote_ip, ipaddr1, ipaddr2);
	printf("%s:%d iptype %d iprangetype %d\n", __FUNCTION__, __LINE__, iptype, iprangetype);
		
	//protocol, remoteAddress
	memset(tmp, 0, 128);
	if(entry->protocol == PROTO_TCP){
		sprintf(tmp, "-p TCP");
		sprintf(tmp_down, "-p TCP");
		sprintf(rsniffer_tmp, " -r tcp");
	}
	else{
		sprintf(tmp, "-p UDP");
		sprintf(tmp_down, "-p UDP");
		sprintf(rsniffer_tmp, " -r udp");
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
	strcat(rsniffer_condition_string, rsniffer_tmp);
	
	//remoteAddress
	memset(tmp, 0, 128);
	memset(tmp_down, 0, 128);
	memset(rsniffer_tmp, 0, 128);
	if(!strcmp(entry->remote_ip, "") || !strcmp(entry->remote_ip, "0")){
		strcpy(tmp, "");
		strcpy(tmp_down, "");
		strcpy(rsniffer_tmp, "");
	}
	else{
		if(iprangetype==1){
			sprintf(tmp, " -d %s", entry->real_remote_ip);
			sprintf(tmp_down, " -s %s", entry->real_remote_ip);
		}
		else if(iprangetype==2){
			sprintf(tmp, " -m iprange --dst-range %s-%s", ipaddr1, ipaddr2);
			sprintf(tmp_down, " -m iprange --src-range %s-%s", ipaddr1, ipaddr2);
		}
		sprintf(rsniffer_tmp, " -h %s", entry->real_remote_ip);
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
	strcat(rsniffer_condition_string, rsniffer_tmp);
	
	//remote port
	memset(tmp, 0, 128);
	memset(tmp_down, 0, 128);
	memset(rsniffer_tmp, 0, 128);
	if(entry->portNot==1){
		sprintf(tmp, " ! --dport %d", entry->remote_port_start);
		sprintf(tmp_down, " ! --sport %d", entry->remote_port_start);
		sprintf(rsniffer_tmp, " -p !%d", entry->remote_port_start);
	}
	else if(entry->remote_port_start == 0){
		strcpy(tmp, "");
		strcpy(tmp_down, "");
		strcpy(rsniffer_tmp, "");
	}
	else{
		sprintf(tmp, " --dport %d:%d", entry->remote_port_start, entry->remote_port_end);
		sprintf(tmp_down, " --sport %d:%d", entry->remote_port_start, entry->remote_port_end);
		sprintf(rsniffer_tmp, " -p %d-%d", entry->remote_port_start, entry->remote_port_end);
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
	strcat(rsniffer_condition_string, rsniffer_tmp);

	mib_get_s(MIB_TRAP_FIRST_N_PKTS_TO_PS, (void *)&trap_first_n_pkts_to_ps, sizeof(trap_first_n_pkts_to_ps));
	memset(connbytes_string, 0, sizeof(connbytes_string));
	memset(connbytes_string1, 0, sizeof(connbytes_string1));
	if(trap_first_n_pkts_to_ps != 0){
		memset(tmp, 0, 128);
		sprintf(tmp, "-m connbytes --connbytes 0:%d --connbytes-dir both --connbytes-mode packets", trap_first_n_pkts_to_ps);
		strcat(connbytes_string, tmp);

		memset(tmp, 0, 128);
		sprintf(tmp, "-m connbytes --connbytes %d --connbytes-dir both --connbytes-mode packets", trap_first_n_pkts_to_ps);
		strcat(connbytes_string1, tmp);
	}

	//hostMAC
	//down string does not need hostmac
	memset(tmp, 0, 128);
	memset(tmp_down, 0, 128);
	memset(rsniffer_tmp, 0, 128);
	if(!memcmp(entry->hostmac, empty_mac, MAC_ADDR_LEN)){
		strcpy(tmp, "");
		strcpy(tmp_down, "");
		strcpy(rsniffer_tmp, "");
	}
	else{
		sprintf(tmp, " -m mac --mac-source %02x:%02x:%02x:%02x:%02x:%02x", 
			entry->hostmac[0],entry->hostmac[1],entry->hostmac[2],entry->hostmac[3],entry->hostmac[4],entry->hostmac[5]);
		sprintf(rsniffer_tmp, " -m %02x:%02x:%02x:%02x:%02x:%02x",
			entry->hostmac[0],entry->hostmac[1],entry->hostmac[2],entry->hostmac[3],entry->hostmac[4],entry->hostmac[5]);
	}
	strcat(condition_string, tmp);
	strcat(rsniffer_condition_string, rsniffer_tmp);

	if(entry->mirror_to_ipver ==IPVER_IPV4){
		sprintf(rsniffer_tmp, " -H %s", inet_ntoa(*((struct in_addr *)entry->mirror_to_ip)));
	}
	else{
		inet_ntop(AF_INET6, entry->mirror_to_ip6, ipstr, sizeof(ipstr));
		sprintf(rsniffer_tmp, " -H %s", ipstr);
	}
	
	strcat(rsniffer_condition_string, rsniffer_tmp);
	sprintf(rsniffer_tmp, " -P %d", entry->mirror_to_port);
	strcat(rsniffer_condition_string, rsniffer_tmp);
		
	printf("%s:%d iptype %d\n", __FUNCTION__, __LINE__, iptype);
	if(iptype == 0 || iptype == 1 || iptype == 3 || iptype == 4){
		sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_MIRROR_FWD, 
			(char *)BRIF, 
			connbytes_string,
			condition_string, 
			mark);
		DOCMDARGVS(IPTABLES, DOWAIT, cmd);
		
		sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_MIRROR_FWD, 
			(char *)BRIF, 
			connbytes_string,
			condition_string_down, 
			mark);
		DOCMDARGVS(IPTABLES, DOWAIT, cmd);

		if(trap_first_n_pkts_to_ps != 0){
			sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_MIRROR_FWD, 
				(char *)BRIF,
				connbytes_string1,
				condition_string, 
				mark0);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);
			
			sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_MIRROR_FWD, 
				(char *)BRIF, 
				connbytes_string1,
				condition_string_down, 
				mark0);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);
		}
	}
#ifdef CONFIG_IPV6
	if(iptype == 0 || iptype == 2 || iptype == 6 || iptype == 7 || iptype == 8){
		sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_MIRROR_FWD, 
			(char *)BRIF, 
			connbytes_string,
			condition_string, 
			mark);
		DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
		
		sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_MIRROR_FWD, 
			(char *)BRIF, 
			connbytes_string,
			condition_string_down, 
			mark);
		DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

		if(trap_first_n_pkts_to_ps != 0){
			sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_MIRROR_FWD, 
				(char *)BRIF, 
				connbytes_string1,
				condition_string, 
				mark0);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
			
			sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_MIRROR_FWD, 
				(char *)BRIF, 
				connbytes_string1,
				condition_string_down, 
				mark0);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
		}
	}
#endif //CONFIG_IPV6

	if(strcmp(entry->real_remote_ip, "8.8.8.8") != 0){
		DOCMDARGVS("/bin/rsniffer_client", DOWAIT, rsniffer_condition_string);
	}
#endif //CONFIG_RTK_SKB_MARK2

#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	system("echo 1 > /proc/fc/ctrl/rstconntrack");
#endif	

	return 0;
}
#endif

int rtk_cmcc_add_traffic_mirror_rule(MIB_CE_MIRROR_RULE_T *entry)
{
	return rtk_cmcc_setup_traffic_mirror_rule(entry, 1);
}

int rtk_cmcc_del_traffic_mirror_rule(MIB_CE_MIRROR_RULE_T *entry)
{
	return rtk_cmcc_setup_traffic_mirror_rule(entry, 0);
}

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
int rtk_cmcc_update_traffic_mirror_rule_url(char*domainName, int containip, char* addr, char* addr6)
{
	int EntryNum = 0;
	int i = 0;
	MIB_CE_MIRROR_RULE_T entry;
	int iptype = 0;;
	int ret = 0;
	
	EntryNum = mib_chain_total(MIB_MIRROR_RULE_TBL);
	for(i=0; i < EntryNum; i++){
		if(!mib_chain_get(MIB_MIRROR_RULE_TBL, i, &entry))
			continue;
		if(!strcmp(domainName, entry.remote_ip)){
			printf("%s:%d: domainName %s, addr %s", __FUNCTION__, __LINE__, domainName, addr);
			rtk_cmcc_del_traffic_mirror_rule(&entry);

			if(containip & CONTAINS_IPV4){
				strcpy(entry.real_remote_ip4, addr);
			}
			if(containip & CONTAINS_IPV6){
				strcpy(entry.real_remote_ip6, addr6);
			}

			ret = rtk_cmcc_add_traffic_mirror_rule(&entry);
			if(ret >= 0){//if set success, update mib
				mib_chain_update(MIB_MIRROR_RULE_TBL, &entry, i);
			}
		}
	}
	return ret;
}
#endif

#ifdef CONFIG_CMCC_TRAFFIC_PROCESS_RULE_SUPPORT
int rtk_cmcc_init_traffic_process_rule(void)
{
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", TABLE_TRAFFIC_PROCESS_FWD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", TABLE_TRAFFIC_PROCESS_POST);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", TABLE_TRAFFIC_PROCESS_FWD);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", TABLE_TRAFFIC_PROCESS_POST);
#endif

	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-p", "tcp", "-j", TABLE_TRAFFIC_PROCESS_FWD);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-p", "udp", "-j", TABLE_TRAFFIC_PROCESS_FWD);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-p", "tcp", "-j", TABLE_TRAFFIC_PROCESS_POST);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-p", "udp", "-j", TABLE_TRAFFIC_PROCESS_POST);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-p", "tcp", "-j", TABLE_TRAFFIC_PROCESS_FWD);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-p", "udp", "-j", TABLE_TRAFFIC_PROCESS_FWD);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-p", "tcp", "-j", TABLE_TRAFFIC_PROCESS_POST);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-p", "udp", "-j", TABLE_TRAFFIC_PROCESS_POST);
#endif
	return 0;
}

int rtk_cmcc_flush_traffic_process_rule(void)
{
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", TABLE_TRAFFIC_PROCESS_FWD);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-p", "tcp", "-j", TABLE_TRAFFIC_PROCESS_FWD);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-p", "udp", "-j", TABLE_TRAFFIC_PROCESS_FWD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", TABLE_TRAFFIC_PROCESS_FWD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", TABLE_TRAFFIC_PROCESS_POST);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-p", "tcp", "-j", TABLE_TRAFFIC_PROCESS_POST);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-p", "udp", "-j", TABLE_TRAFFIC_PROCESS_POST);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", TABLE_TRAFFIC_PROCESS_POST);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", TABLE_TRAFFIC_PROCESS_FWD);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-p", "tcp", "-j", TABLE_TRAFFIC_PROCESS_FWD);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-p", "udp", "-j", TABLE_TRAFFIC_PROCESS_FWD);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", TABLE_TRAFFIC_PROCESS_FWD);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", TABLE_TRAFFIC_PROCESS_POST);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-p", "tcp", "-j", TABLE_TRAFFIC_PROCESS_POST);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-p", "udp", "-j", TABLE_TRAFFIC_PROCESS_POST);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", TABLE_TRAFFIC_PROCESS_POST);
#endif
	return 0;
}


#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
//support IPv4 IPv6 Coexist
int rtk_rtk_cmcc_setup_traffic_process_init
(
	MIB_CMCC_TRAFFIC_PROCESS_RULE_T *entry,
	char *realip, 	
	char *condition_string,
	char *condition_string_down,
	char *connbytes_string,
	char *connbytes_string1,
	unsigned int trap_first_n_pkts_to_ps
)
{
	char tmp[128] = {0};
	char tmp1[128] = {0};
	char tmp_down[128] = {0};
	int iprangetype=0;
	int iptype = 0;
	int remoteporttype = -1;
	char ipaddr1[64]={0}, ipaddr2[64] ={0};
	int startport = 0, endport = 0;
	char hostip[32]={0};

	iptype = checkIPv4OrIPv6(realip, "");
	iprangetype = getIpRange(realip, ipaddr1, ipaddr2);
	printf("%s:%d iptype %d iprangetype %d\n", __FUNCTION__, __LINE__, iptype, iprangetype);
		
	//protocol, remoteAddress
	memset(tmp, 0, 128);
	sprintf(tmp, "-p TCP");
	sprintf(tmp_down, "-p TCP");
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
	
	//remoteAddress
	memset(tmp, 0, 128);
	memset(tmp_down, 0, 128);
	if(!strcmp(entry->remoteAddress, "") || !strcmp(entry->remoteAddress, "0")){
		strcpy(tmp, "");
		strcpy(tmp_down, "");
	}
	else{
		if(iprangetype==1){
			sprintf(tmp, " -d %s", realip);
			sprintf(tmp_down, " -s %s", realip);
		}
		else if(iprangetype==2){
			sprintf(tmp, " -m iprange --dst-range %s-%s", ipaddr1, ipaddr2);
			sprintf(tmp_down, " -m iprange --src-range %s-%s", ipaddr1, ipaddr2);
		}
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
	
	//remote port
	memset(tmp, 0, 128);
	memset(tmp_down, 0, 128);
	remoteporttype = parseRemotePort(entry->remotePort, &startport, &endport);
	if(remoteporttype == 3){//remote port == 0, don't card
		strcpy(tmp, "");
		strcpy(tmp_down, "");
	}
	else if(remoteporttype == 1){ //format !x
		sprintf(tmp, " ! --dport %d", startport);
		sprintf(tmp_down, " ! --sport %d", startport);
	}
	else{
		strcpy(tmp1, entry->remotePort);
		sprintf(tmp, " --dport %s", replaceChar(tmp1, '-', ':'));
		sprintf(tmp_down, " --sport %s", replaceChar(tmp1, '-', ':'));
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);

	if(trap_first_n_pkts_to_ps != 0){
		memset(tmp, 0, 128);
		sprintf(tmp, "-m connbytes --connbytes 0:%d --connbytes-dir both --connbytes-mode packets", trap_first_n_pkts_to_ps);
		strcat(connbytes_string, tmp);

		memset(tmp, 0, 128);
		sprintf(tmp, "-m connbytes --connbytes %d --connbytes-dir both --connbytes-mode packets", trap_first_n_pkts_to_ps);
		strcat(connbytes_string1, tmp);
	}

	//hostMAC
	//down string does not need hostmac
	memset(tmp, 0, 128);
	memset(tmp_down, 0, 128);
	if(!strcmp(entry->hostMAC, "") || !strcmp(entry->hostMAC, "0")){
		strcpy(tmp, "");
		strcpy(tmp_down, "");
	}
	else{
		sprintf(tmp, " -m mac --mac-source %s", entry->hostMAC);
		if( getIPFromMAC( entry->hostMAC, hostip ) == 0 )
			sprintf(tmp_down, " -d %s", hostip);
		else
			printf("%s:%d this host (mac = %s) has no ip, perhaps a bridge wan lan host!\n", __FUNCTION__, __LINE__, entry->hostMAC);
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
		
	printf("%s:%d iptype %d\n", __FUNCTION__, __LINE__, iptype);

	return iptype;
}

int rtk_cmcc_setup_traffic_process_rule(MIB_CMCC_TRAFFIC_PROCESS_RULE_T *entry, int type)
{
	unsigned char dpi_test = 0;
#ifdef CONFIG_RTK_SKB_MARK2
	unsigned char mark0[64], mark[64];
	char condition_string[512] = {0};
	char condition_string_down[512] = {0};
	char connbytes_string[512] = {0};
	char connbytes_string1[512] = {0};
	char cmd[512] = {0};
	int iptype = 0;
	unsigned int trap_first_n_pkts_to_ps = 0;

	mib_get_s(MIB_TRAP_FIRST_N_PKTS_TO_PS, (void *)&trap_first_n_pkts_to_ps, sizeof(trap_first_n_pkts_to_ps));
	sprintf(mark0, "0x0/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);
	sprintf(mark, "0x%llx/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK, SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);
	DOCMDINIT;
	
	printf("%s:%d type %d\n", __FUNCTION__, __LINE__, type);
	printf("%s:%d remoteAddress %s, realremoteip4 %s, realremoteip6 %s ,remotePort %s, direction %s, hostMAC %s\n", __FUNCTION__, __LINE__, 
		entry->remoteAddress, 
		entry->realremoteip4, 
		entry->realremoteip6, 
		entry->remotePort,
		entry->direction,
		entry->hostMAC);

	if((entry->containip&CONTAINS_IPV4) || (entry->containip == CONTAINS_IPNULL)){
		memset(condition_string, 0, sizeof(condition_string));
		memset(condition_string_down, 0, sizeof(condition_string_down));
		memset(connbytes_string, 0, sizeof(connbytes_string));
		memset(connbytes_string1, 0, sizeof(connbytes_string1));
		
		iptype = rtk_rtk_cmcc_setup_traffic_process_init(entry, entry->realremoteip4, 
					condition_string, condition_string_down, connbytes_string, connbytes_string1, trap_first_n_pkts_to_ps);

		if(iptype == 0 || iptype == 1 || iptype == 3 || iptype == 4){
			if((0 == strcmp(entry->direction, "UP")) || (0 == strcmp(entry->direction, "ALL"))){
				sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
					CMCC_FORWARDRULE_ACTION_APPEND(type), 
					(char *)TABLE_TRAFFIC_PROCESS_FWD, 
					(char *)BRIF,
					connbytes_string,
					condition_string, 
					mark);
				DOCMDARGVS(IPTABLES, DOWAIT, cmd);
			}

			if((0 == strcmp(entry->direction, "DOWN")) || (0 == strcmp(entry->direction, "ALL"))){
				sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
					CMCC_FORWARDRULE_ACTION_APPEND(type), 
					(char *)TABLE_TRAFFIC_PROCESS_FWD, 
					(char *)BRIF, 
					connbytes_string,
					condition_string_down, 
					mark);
				DOCMDARGVS(IPTABLES, DOWAIT, cmd);
			}

			if(trap_first_n_pkts_to_ps != 0){
				if((0 == strcmp(entry->direction, "UP")) || (0 == strcmp(entry->direction, "ALL"))){
					sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
						CMCC_FORWARDRULE_ACTION_APPEND(type), 
						(char *)TABLE_TRAFFIC_PROCESS_FWD, 
						(char *)BRIF,
						connbytes_string1,
						condition_string, 
						mark0);
					DOCMDARGVS(IPTABLES, DOWAIT, cmd);
				}

				if((0 == strcmp(entry->direction, "DOWN")) || (0 == strcmp(entry->direction, "ALL"))){
					sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
						CMCC_FORWARDRULE_ACTION_APPEND(type), 
						(char *)TABLE_TRAFFIC_PROCESS_FWD, 
						(char *)BRIF, 
						connbytes_string1,
						condition_string_down, 
						mark0);
					DOCMDARGVS(IPTABLES, DOWAIT, cmd);
				}
			}
		}
	}
#ifdef CONFIG_IPV6
	if((entry->containip&CONTAINS_IPV6) || (entry->containip == CONTAINS_IPNULL)){
		memset(condition_string, 0, sizeof(condition_string));
		memset(condition_string_down, 0, sizeof(condition_string_down));
		memset(connbytes_string, 0, sizeof(connbytes_string));
		memset(connbytes_string1, 0, sizeof(connbytes_string1));

		iptype = rtk_rtk_cmcc_setup_traffic_process_init(entry, entry->realremoteip6, 
					condition_string, condition_string_down, connbytes_string, connbytes_string1, trap_first_n_pkts_to_ps);

		if(iptype == 0 || iptype == 2 || iptype == 6 || iptype == 7 || iptype == 8){
			if((0 == strcmp(entry->direction, "UP")) || (0 == strcmp(entry->direction, "ALL"))){
				sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
					CMCC_FORWARDRULE_ACTION_APPEND(type), 
					(char *)TABLE_TRAFFIC_PROCESS_FWD, 
					(char *)BRIF, 
					connbytes_string,
					condition_string, 
					mark);
				DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
			}

			if((0 == strcmp(entry->direction, "DOWN")) || (0 == strcmp(entry->direction, "ALL"))){
				sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
					CMCC_FORWARDRULE_ACTION_APPEND(type), 
					(char *)TABLE_TRAFFIC_PROCESS_FWD, 
					(char *)BRIF, 
					connbytes_string,
					condition_string_down, 
					mark);
				DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
			}

			if(trap_first_n_pkts_to_ps != 0){
				if((0 == strcmp(entry->direction, "UP")) || (0 == strcmp(entry->direction, "ALL"))){
					sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
						CMCC_FORWARDRULE_ACTION_APPEND(type), 
						(char *)TABLE_TRAFFIC_PROCESS_FWD, 
						(char *)BRIF, 
						connbytes_string1,
						condition_string, 
						mark0);
					DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
				}

				if((0 == strcmp(entry->direction, "DOWN")) || (0 == strcmp(entry->direction, "ALL"))){				
					sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
						CMCC_FORWARDRULE_ACTION_APPEND(type), 
						(char *)TABLE_TRAFFIC_PROCESS_FWD, 
						(char *)BRIF, 
						connbytes_string1,
						condition_string_down, 
						mark0);
					DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
				}
			}
		}
	}
#endif //CONFIG_IPV6
#endif //CONFIG_RTK_SKB_MARK2

#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	mib_get(MIB_DPI_TEST, (void *)&dpi_test);
	if(dpi_test)
		system("echo 1 > /proc/fc/ctrl/rstconntrack");
#endif

	return 0;
}

#else
int rtk_cmcc_setup_traffic_process_rule(MIB_CMCC_TRAFFIC_PROCESS_RULE_T *entry, int type)
{
#ifdef CONFIG_RTK_SKB_MARK2
	unsigned char mark0[64], mark[64];
	int remoteporttype = -1, startport = 0, endport = 0;
	char condition_string[512] = {0};
	char condition_string_down[512] = {0};
	char connbytes_string[512] = {0};
	char connbytes_string1[512] = {0};
	char cmd[512] = {0};
	char tmp[128] = {0};
	char tmp1[128] = {0};
	char tmp_down[128] = {0};
	int iptype = 0;
	int iprangetype=0;
	char ipaddr1[64]={0}, ipaddr2[64] ={0};
	unsigned int trap_first_n_pkts_to_ps = 0;

	memset(condition_string, 0, sizeof(condition_string));
	memset(condition_string_down, 0, sizeof(condition_string_down));
	sprintf(mark0, "0x0/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);
	sprintf(mark, "0x%llx/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK, SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);
	
	DOCMDINIT;
	
	printf("%s:%d type %d\n", __FUNCTION__, __LINE__, type);
	printf("%s:%d remoteAddress %s, realremoteAddress %s, remotePort %s, direction %s, hostMAC %s\n", __FUNCTION__, __LINE__, 
		entry->remoteAddress, 
		entry->realremoteAddress, 
		entry->remotePort,
		entry->direction,
		entry->hostMAC);
	
	iptype = checkIPv4OrIPv6(entry->realremoteAddress, "");
	iprangetype = getIpRange(entry->realremoteAddress, ipaddr1, ipaddr2);
	printf("%s:%d iptype %d iprangetype %d\n", __FUNCTION__, __LINE__, iptype, iprangetype);
		
	//protocol, remoteAddress
	memset(tmp, 0, 128);
	sprintf(tmp, "-p TCP");
	sprintf(tmp_down, "-p TCP");
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
	
	//remoteAddress
	memset(tmp, 0, 128);
	memset(tmp_down, 0, 128);
	if(!strcmp(entry->remoteAddress, "") || !strcmp(entry->remoteAddress, "0")){
		strcpy(tmp, "");
		strcpy(tmp_down, "");
	}
	else{
		if(iprangetype==1){
			sprintf(tmp, " -d %s", entry->realremoteAddress);
			sprintf(tmp_down, " -s %s", entry->realremoteAddress);
		}
		else if(iprangetype==2){
			sprintf(tmp, " -m iprange --dst-range %s-%s", ipaddr1, ipaddr2);
			sprintf(tmp_down, " -m iprange --src-range %s-%s", ipaddr1, ipaddr2);
		}
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
	
	//remote port
	memset(tmp, 0, 128);
	memset(tmp_down, 0, 128);
	remoteporttype = parseRemotePort(entry->remotePort, &startport, &endport);
	if(remoteporttype == 3){//remote port == 0, don't card
		strcpy(tmp, "");
		strcpy(tmp_down, "");
	}
	else if(remoteporttype == 1){ //format !x
		sprintf(tmp, " ! --dport %d", startport);
		sprintf(tmp_down, " ! --sport %d", startport);
	}
	else{
		strcpy(tmp1, entry->remotePort);
		sprintf(tmp, " --dport %s", replaceChar(tmp1, '-', ':'));
		sprintf(tmp_down, " --sport %s", replaceChar(tmp1, '-', ':'));
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);

	mib_get_s(MIB_TRAP_FIRST_N_PKTS_TO_PS, (void *)&trap_first_n_pkts_to_ps, sizeof(trap_first_n_pkts_to_ps));
	memset(connbytes_string, 0, sizeof(connbytes_string));
	memset(connbytes_string1, 0, sizeof(connbytes_string1));
	if(trap_first_n_pkts_to_ps != 0){
		memset(tmp, 0, 128);
		sprintf(tmp, "-m connbytes --connbytes 0:%d --connbytes-dir both --connbytes-mode packets", trap_first_n_pkts_to_ps);
		strcat(connbytes_string, tmp);

		memset(tmp, 0, 128);
		sprintf(tmp, "-m connbytes --connbytes %d --connbytes-dir both --connbytes-mode packets", trap_first_n_pkts_to_ps);
		strcat(connbytes_string1, tmp);
	}

	//hostMAC
	//down string does not need hostmac
	memset(tmp, 0, 128);
	memset(tmp_down, 0, 128);
	if(!strcmp(entry->hostMAC, "") || !strcmp(entry->hostMAC, "0")){
		strcpy(tmp, "");
		strcpy(tmp_down, "");
	}
	else{
		sprintf(tmp, " -m mac --mac-source %s", entry->hostMAC);
	}
	strcat(condition_string, tmp);
		
	printf("%s:%d iptype %d\n", __FUNCTION__, __LINE__, iptype);
	if(iptype == 0 || iptype == 1 || iptype == 3 || iptype == 4){
		sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_PROCESS_FWD, 
			(char *)BRIF,
			connbytes_string,
			condition_string, 
			mark);
		DOCMDARGVS(IPTABLES, DOWAIT, cmd);
		
		sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_PROCESS_FWD, 
			(char *)BRIF, 
			connbytes_string,
			condition_string_down, 
			mark);
		DOCMDARGVS(IPTABLES, DOWAIT, cmd);

		if(trap_first_n_pkts_to_ps != 0){
			sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_PROCESS_FWD, 
				(char *)BRIF,
				connbytes_string1,
				condition_string, 
				mark0);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);
			
			sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_PROCESS_FWD, 
				(char *)BRIF, 
				connbytes_string1,
				condition_string_down, 
				mark0);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);
		}
	}
#ifdef CONFIG_IPV6
	if(iptype == 0 || iptype == 2 || iptype == 6 || iptype == 7 || iptype == 8){
		sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_PROCESS_FWD, 
			(char *)BRIF, 
			connbytes_string,
			condition_string, 
			mark);
		DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
		
		sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_PROCESS_FWD, 
			(char *)BRIF, 
			connbytes_string,
			condition_string_down, 
			mark);
		DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

		if(trap_first_n_pkts_to_ps != 0){
			sprintf(cmd, "-t mangle %s %s -i %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_PROCESS_FWD, 
				(char *)BRIF, 
				connbytes_string1,
				condition_string, 
				mark0);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
			
			sprintf(cmd, "-t mangle %s %s ! -i %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_PROCESS_FWD, 
				(char *)BRIF, 
				connbytes_string1,
				condition_string_down, 
				mark0);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
		}
	}
#endif //CONFIG_IPV6
#endif //CONFIG_RTK_SKB_MARK2

#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		system("echo 1 > /proc/fc/ctrl/rstconntrack");
#endif

	return 0;
}
#endif

int rtk_cmcc_add_traffic_process_rule(MIB_CMCC_TRAFFIC_PROCESS_RULE_T *entry)
{
	return rtk_cmcc_setup_traffic_process_rule(entry, 1);
}

int rtk_cmcc_del_traffic_process_rule(MIB_CMCC_TRAFFIC_PROCESS_RULE_T *entry)
{
	return rtk_cmcc_setup_traffic_process_rule(entry, 0);
}

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
int rtk_cmcc_update_traffic_process_rule_url(char*domainName, int containip, char* addr, char *addr6)
{
	int EntryNum = 0;
	int i = 0;
	MIB_CMCC_TRAFFIC_PROCESS_RULE_T entry;
	int iptype = 0;;
	int ret = 0;
	
	EntryNum = mib_chain_total(MIB_CMCC_TRAFFIC_PROCESS_RULE_TBL);
	for(i=0; i < EntryNum; i++){
		if(!mib_chain_get(MIB_CMCC_TRAFFIC_PROCESS_RULE_TBL, i, &entry))
			continue;
		if(!strcmp(domainName, entry.remoteAddress)){
			printf("%s:%d: domainName %s, addr %s, addr6 %s", __FUNCTION__, __LINE__, domainName, addr, addr6);
			rtk_cmcc_del_traffic_process_rule(&entry);
			_del_tf_rule_from_nfhook(entry.ruleIdx);
			if(containip & CONTAINS_IPV4){
				strcpy(entry.realremoteip4, addr);
			}
			if(containip & CONTAINS_IPV6){
				strcpy(entry.realremoteip6, addr6);
			}

			ret = rtk_cmcc_add_traffic_process_rule(&entry);
			_add_tf_rule_into_nfhook(&entry);
			if(ret >= 0){//if set success, update mib
				mib_chain_update(MIB_CMCC_TRAFFIC_PROCESS_RULE_TBL, &entry, i);
			}
		}
	}
	return ret;
}
#endif

#endif //CONFIG_CMCC_TRAFFIC_PROCESS_RULE_SUPPORT

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
int rtk_cmcc_update_traffic_monitor_rule_url(char*domainName, int containip, char* addr, char* addr6)
{
	int i=0;
	int EntryNum = 0;
	MIB_CMCC_TRAFFICMONITOR_RULE_T entry;
	char cmd[512];
	int netmask=32;

	EntryNum = mib_chain_total(MIB_CMCC_TRAFFICMONITOR_RULE_TBL);
	for(i=0; i < EntryNum; i++){
		memset(&entry, 0, sizeof(MIB_CMCC_TRAFFICMONITOR_RULE_T));
		if(!mib_chain_get(MIB_CMCC_TRAFFICMONITOR_RULE_TBL, i, &entry))
			continue;
		if(!strcmp(domainName, entry.monitor_ip_url)){
			if(containip & CONTAINS_IPV4){
				strcpy(entry.real_monitor_ip4, addr);
				if (entry.netmask == 0){
					netmask = 32;
					sprintf(cmd, "/bin/echo \"%d %d %s %d %s\" > /proc/osgi/traffic_monitor_mod", entry.bundleID, 1, entry.real_monitor_ip4, netmask, entry.monitor_ip_url);
				}
				else{
					sprintf(cmd, "/bin/echo \"%d %d %s %d %s\" > /proc/osgi/traffic_monitor_mod", entry.bundleID, 1, entry.real_monitor_ip4, entry.netmask, "na");
				}
				printf("%s\n", cmd);
				system(cmd);
			}
			if(containip & CONTAINS_IPV6){
				strcpy(entry.real_monitor_ip6, addr6);
				if (entry.netmask == 0){
					netmask = 128;
					sprintf(cmd, "/bin/echo \"%d %d %s %d %s\" > /proc/osgi/traffic_monitor_mod", entry.bundleID, 0, entry.real_monitor_ip6, netmask, entry.monitor_ip_url);
				}
				else{
					sprintf(cmd, "/bin/echo \"%d %d %s %d %s\" > /proc/osgi/traffic_monitor_mod", entry.bundleID, 0, entry.real_monitor_ip6, entry.netmask, "na");
				}
				printf("%s\n", cmd);
				system(cmd);
			}

			mib_chain_update(MIB_CMCC_TRAFFICMONITOR_RULE_TBL, &entry, i);
		}
	}
	return 0;
}
#endif

int rtk_cmcc_init_traffic_qos_rule(void)
{
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N", TABLE_TRAFFIC_QOS_EB);
	va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P", TABLE_TRAFFIC_QOS_EB, "RETURN");
	va_cmd(EBTABLES, 6, 1, "-t", "broute", "-A", "BROUTING", "-j", TABLE_TRAFFIC_QOS_EB);
	
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", TABLE_TRAFFIC_QOS_FWD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", TABLE_TRAFFIC_QOS_POST);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", TABLE_TRAFFIC_QOS_FWD);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", TABLE_TRAFFIC_QOS_POST);
#endif

	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-p", "tcp", "-j", TABLE_TRAFFIC_QOS_FWD);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-p", "udp", "-j", TABLE_TRAFFIC_QOS_FWD);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-p", "tcp", "-j", TABLE_TRAFFIC_QOS_POST);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-p", "udp", "-j", TABLE_TRAFFIC_QOS_POST);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-p", "tcp", "-j", TABLE_TRAFFIC_QOS_FWD);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-p", "udp", "-j", TABLE_TRAFFIC_QOS_FWD);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-p", "tcp", "-j", TABLE_TRAFFIC_QOS_POST);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-p", "udp", "-j", TABLE_TRAFFIC_QOS_POST);
#endif

	return 0;
}

int rtk_cmcc_find_traffic_qos_flowentry(int flow, MIB_TRAFFIC_QOS_RULE_FLOW_Tp pEntry)
{
	int num = mib_chain_total(MIB_TRAFFIC_QOS_RULE_FLOW_TBL);
	MIB_TRAFFIC_QOS_RULE_FLOW_T Entry;
	int i;

	for(i=0;i<num;i++)
	{
		if(!mib_chain_get(MIB_TRAFFIC_QOS_RULE_FLOW_TBL,i,&Entry))
			continue;
		if(Entry.flow== flow)
		{
			memcpy(pEntry, &Entry, sizeof(Entry));
			return 0;
		}
	}
	return -1;
}

int rtk_cmcc_flush_traffic_qos_rule(void)
{
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", TABLE_TRAFFIC_QOS_EB);
	va_cmd(EBTABLES, 6, 1, "-t", "broute", "-D", "BROUTING", "-j", TABLE_TRAFFIC_QOS_EB);	
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X", TABLE_TRAFFIC_QOS_EB);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", TABLE_TRAFFIC_QOS_FWD);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-p", "tcp", "-j", TABLE_TRAFFIC_QOS_FWD);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-p", "udp", "-j", TABLE_TRAFFIC_QOS_FWD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", TABLE_TRAFFIC_QOS_FWD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", TABLE_TRAFFIC_QOS_POST);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-p", "tcp", "-j", TABLE_TRAFFIC_QOS_POST);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-p", "udp", "-j", TABLE_TRAFFIC_QOS_POST);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", TABLE_TRAFFIC_QOS_POST);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", TABLE_TRAFFIC_QOS_FWD);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-p", "tcp", "-j", TABLE_TRAFFIC_QOS_FWD);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-p", "udp", "-j", TABLE_TRAFFIC_QOS_FWD);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", TABLE_TRAFFIC_QOS_FWD);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", TABLE_TRAFFIC_QOS_POST);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-p", "tcp", "-j", TABLE_TRAFFIC_QOS_POST);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-p", "udp", "-j", TABLE_TRAFFIC_QOS_POST);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", TABLE_TRAFFIC_QOS_POST);
#endif
	return 0;
}

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
int rtk_rtk_cmcc_setup_traffic_qos_init
(
	MIB_TRAFFIC_QOS_RULE_T *entry,
	char *realip,
	char *condition_string,
	char *condition_string_eb,
	char *condition_string_down,
	struct in_addr *remoteAddress1,
	struct in6_addr *remoteAddress6_1 
)
{
	int remoteporttype = -1, startport = 0, endport = 0;
	char ipstr[64]={0}, ipaddr1[64]={0}, ipaddr2[64] ={0};
	char tmp[128] = {0};
	char tmp_eb[128] = {0};
	char tmp1[128] = {0};
	char tmp_down[128] = {0};
	unsigned int maskbit=0;
	struct in6_addr remoteAddress6_2;
	struct in_addr remoteAddress_tmp;
	int ret=0, isipv4=0;
	ipaddr_t mask;
	
	//remoteAddress
	memset(tmp, 0, 128);
	memset(tmp_eb, 0, 128);
	memset(tmp_down, 0, 128);

	if(!strcmp(entry->remoteAddress, "") || !strcmp(entry->remoteAddress, "0")){
		strcpy(tmp, "");
		strcpy(tmp_eb, "");
		strcpy(tmp_down, "");
	}
	else{
		ret = rtk_osgi_traffic_parse_address(entry->remoteAddress,ipstr,&maskbit);
		if(ret==2 || ret==-1){
			sprintf(tmp, " -d %s", realip);
			sprintf(tmp_down, " -s %s", realip);
			if(inet_pton(AF_INET, realip, remoteAddress1) == 1){
				isipv4=1;
				sprintf(tmp_eb, " --ip-destination %s", realip);
			}
			else if(inet_pton(AF_INET6, realip, remoteAddress6_1) == 1){
				sprintf(tmp_eb, " --ip6-destination %s", realip);
			}
		}
		else if(ret==1){
			if(inet_pton(AF_INET, ipstr, remoteAddress1) == 1){
				isipv4=1;
				mask = ~0 << (sizeof(ipaddr_t)*8 - maskbit);
				mask = htonl(mask);
				remoteAddress_tmp.s_addr = (remoteAddress1->s_addr & mask);
				inet_ntop(AF_INET, &remoteAddress_tmp, ipaddr1, INET_ADDRSTRLEN);
				remoteAddress_tmp.s_addr = (remoteAddress1->s_addr | ~mask);
				inet_ntop(AF_INET, &remoteAddress_tmp, ipaddr2, INET_ADDRSTRLEN);
				sprintf(tmp, " -m iprange --dst-range %s-%s", ipaddr1, ipaddr2);
				sprintf(tmp_eb, " --ip-destination %s/%d", ipaddr1, maskbit);
				sprintf(tmp_down, " -m iprange --src-range %s-%s", ipaddr1, ipaddr2);
			}
#ifdef CONFIG_IPV6
			else if(inet_pton(AF_INET6, ipstr, remoteAddress6_1) == 1){
				rtk_ipv6_ip_address_range(remoteAddress6_1, maskbit, remoteAddress6_1, &remoteAddress6_2);
				inet_ntop(AF_INET6, remoteAddress6_1, ipaddr1, INET6_ADDRSTRLEN);
				inet_ntop(AF_INET6, &remoteAddress6_2, ipaddr2, INET6_ADDRSTRLEN);
				sprintf(tmp, " -m iprange --dst-range %s-%s", ipaddr1, ipaddr2);
				sprintf(tmp_eb, " --ip6-destination %s/%d", ipaddr1, maskbit);
				sprintf(tmp_down, " -m iprange --src-range %s-%s", ipaddr1, ipaddr2);
			}
#endif
			else{
				printf("%s:%d\n", __FUNCTION__, __LINE__);
				return -1;
			}
		}
		else{
			printf("%s:%d\n", __FUNCTION__, __LINE__);
			return -1;
		}
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
	strcat(condition_string_eb, tmp_eb);
	
	//remote port
	memset(tmp, 0, 128);
	memset(tmp_eb, 0, 128);
	memset(tmp_down, 0, 128);
	remoteporttype = parseRemotePort(entry->remotePort, &startport, &endport);
	if(remoteporttype == 3){//remote port == 0, don't card
		strcpy(tmp, "");
		strcpy(tmp_down, "");
		strcpy(tmp_eb, "");
	}
	else if(remoteporttype == 1){ //format !x
		sprintf(tmp, " ! --dport %d", startport);
		sprintf(tmp_down, " ! --sport %d", startport);
		sprintf(tmp_eb, " ! --dport %d", startport);
	}
	else{
		strcpy(tmp1, entry->remotePort);
		sprintf(tmp, " --dport %s", replaceChar(tmp1, '-', ':'));
		sprintf(tmp_down, " --sport %s", replaceChar(tmp1, '-', ':'));
		sprintf(tmp_eb, " --dport %s", replaceChar(tmp1, '-', ':'));
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
	strcat(condition_string_eb, tmp_eb);
	
	return isipv4;
}
int rtk_cmcc_setup_traffic_qos_rule(MIB_TRAFFIC_QOS_RULE_T *entry, MIB_TRAFFIC_QOS_RULE_FLOW_T *flowEntry, int type)
{
#ifdef CONFIG_RTK_SKB_MARK2
	char condition_string[512] = {0};
	char condition_string_eb[512] = {0};
	char condition_string_down[512] = {0};
	char cmd[512] = {0};
	char str_mark[128] = {0};
	char str_mark2[128] = {0};
	struct in6_addr remoteAddress6_1;
	struct in_addr remoteAddress1;
	int ret=0, isipv4=0;
	int mark=0, mark_mask=0, meter_idx;
	unsigned long long mark2=0, mark2_mask=0;
	int mib_idx;
	rt_flow_ops_data_t key;
	char m_dscp[24];
	
	DOCMDINIT;
	
	printf("%s:%d type %d\n", __FUNCTION__, __LINE__, type);
	printf("%s:%d remoteAddress %s, realremoteip4 %s, realremoteip6 %s, remotePort %s, flow %d\n", __FUNCTION__, __LINE__, 
		entry->remoteAddress, 
		entry->realremoteip4,
		entry->realremoteip6, 
		entry->remotePort,
		entry->flow);
	if(entry->containip&CONTAINS_IPV4)	
	{
		memset(condition_string, 0, sizeof(condition_string));
		memset(condition_string_down, 0, sizeof(condition_string_down));
		memset(condition_string_eb, 0, sizeof(condition_string_eb));

		isipv4 = rtk_rtk_cmcc_setup_traffic_qos_init(entry, entry->realremoteip4, condition_string, condition_string_eb, condition_string_down, &remoteAddress1, &remoteAddress6_1);
		//upstream rule
		mark_mask = SOCK_MARK_METER_INDEX_BIT_MASK|SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK;
		mark2_mask = SOCK_MARK2_MIB_MASK_ALL;
		meter_idx= rtk_cmcc_setup_traffic_qos_share_meter(flowEntry->flow, flowEntry->upmaxrate);

		if(meter_idx != -1){
			mark |= SOCK_MARK_METER_INDEX_ENABLE_TO_MARK(1);
			//mark |= SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(1);
			mark2 |= SOCK_MARK2_MIB_INDEX_ENABLE_TO_MARK(1);
			//mark2 |= SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(1);
		}
		else
		{
			return -1;
		}
		mark |= SOCK_MARK_METER_INDEX_TO_MARK(meter_idx);
		mark2 |= SOCK_MARK2_MIB_INDEX_TO_MARK(meter_idx);

		sprintf(str_mark, "0x%x/0x%x", mark, mark_mask);
		sprintf(str_mark2, "0x%llx/0x%llx", mark2, mark2_mask);
		snprintf(m_dscp, 24, "0x%x", flowEntry->dscp&0xFF);
		if(isipv4==1)
		{
			sprintf(cmd, "-t mangle %s %s -i %s %s -j MARK --set-mark %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_QOS_FWD, 
			(char *)BRIF, 
			condition_string, 
			str_mark);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle %s %s %s -j MARK2 --set-mark2 %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_QOS_POST, 
			condition_string, 
			str_mark2);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t broute %s %s -p 0x0800 %s -j ftos --set-ftos %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_QOS_EB,  
			condition_string_eb,
			m_dscp);
			DOCMDARGVS(EBTABLES, DOWAIT, cmd);

			//delete flow when finish setup iptables to apply setting
			memset(&key, 0, sizeof(key));
			key.data_delFlow.flowKeyType = RT_FLOW_OP_FLOW_KEY_TYPE_PATTERN;
			key.data_delFlow.flowKey.flowPattern.dip4_valid = 1;
			key.data_delFlow.flowKey.flowPattern.dip4 =  htonl(remoteAddress1.s_addr);
			rt_flow_operation_add(RT_FLOW_OPS_DELETE_FLOW, &key);
		}

		//downstream rule
		mark_mask = SOCK_MARK_METER_INDEX_BIT_MASK|SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK;
		mark2_mask = SOCK_MARK2_MIB_MASK_ALL;
		meter_idx= rtk_cmcc_setup_traffic_qos_share_meter(flowEntry->flow+MAX_QOS_QUEUE_NUM, flowEntry->downmaxrate);

		if(meter_idx != -1){
			mark |= SOCK_MARK_METER_INDEX_ENABLE_TO_MARK(1);
			//mark |= SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(1);
			mark2 |= SOCK_MARK2_MIB_INDEX_ENABLE_TO_MARK(1);
			//mark2 |= SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(1);
		}
		else
		{
			return -1;
		}
		mark |= SOCK_MARK_METER_INDEX_TO_MARK(meter_idx);
		mark2 |= SOCK_MARK2_MIB_INDEX_TO_MARK(meter_idx);
	
		sprintf(str_mark, "0x%x/0x%x", mark, mark_mask);
		sprintf(str_mark2, "0x%llx/0x%llx", mark2, mark2_mask);
		if(isipv4==1){
			sprintf(cmd, "-t mangle %s %s ! -i %s %s -j MARK --set-mark %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_QOS_FWD, 
			(char *)BRIF, 
			condition_string_down, 
			str_mark);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle %s %s %s -j MARK2 --set-mark2 %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_QOS_POST,  
			condition_string_down, 
			str_mark2);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);

			//delete flow when finish setup iptables to apply setting
			memset(&key, 0, sizeof(key));
			key.data_delFlow.flowKeyType = RT_FLOW_OP_FLOW_KEY_TYPE_PATTERN;
			key.data_delFlow.flowKey.flowPattern.sip4_valid = 1;
			key.data_delFlow.flowKey.flowPattern.sip4 =  htonl(remoteAddress1.s_addr);
			rt_flow_operation_add(RT_FLOW_OPS_DELETE_FLOW, &key);
		}
	}
#ifdef CONFIG_IPV6	
	if(entry->containip&CONTAINS_IPV6)	
	{
		memset(condition_string, 0, sizeof(condition_string));
		memset(condition_string_down, 0, sizeof(condition_string_down));
		memset(condition_string_eb, 0, sizeof(condition_string_eb));

		isipv4 = rtk_rtk_cmcc_setup_traffic_qos_init(entry, entry->realremoteip6, condition_string, condition_string_eb, condition_string_down, &remoteAddress1, &remoteAddress6_1);
		//upstream rule
		mark_mask = SOCK_MARK_METER_INDEX_BIT_MASK|SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK;
		mark2_mask = SOCK_MARK2_MIB_MASK_ALL;
		meter_idx= rtk_cmcc_setup_traffic_qos_share_meter(flowEntry->flow, flowEntry->upmaxrate);

		if(meter_idx != -1){
			mark |= SOCK_MARK_METER_INDEX_ENABLE_TO_MARK(1);
			//mark |= SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(1);
			mark2 |= SOCK_MARK2_MIB_INDEX_ENABLE_TO_MARK(1);
			//mark2 |= SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(1);
		}
		else
		{
			return -1;
		}
		mark |= SOCK_MARK_METER_INDEX_TO_MARK(meter_idx);
		mark2 |= SOCK_MARK2_MIB_INDEX_TO_MARK(meter_idx);

		sprintf(str_mark, "0x%x/0x%x", mark, mark_mask);
		sprintf(str_mark2, "0x%llx/0x%llx", mark2, mark2_mask);
		snprintf(m_dscp, 24, "0x%x", flowEntry->dscp&0xFF);
		if(isipv4==0)
		{
			sprintf(cmd, "-t mangle %s %s -i %s %s -j MARK --set-mark %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_QOS_FWD, 
			(char *)BRIF, 
			condition_string, 
			str_mark);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle %s %s %s -j MARK2 --set-mark2 %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_QOS_POST, 
			condition_string, 
			str_mark2);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

			sprintf(cmd, "-t broute %s %s -p 0x86dd %s -j ftos --set-ftos %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_EB,  
				condition_string_eb,
				m_dscp);
			DOCMDARGVS(EBTABLES, DOWAIT, cmd);

			//delete flow when finish setup iptables to apply setting
			memset(&key, 0, sizeof(key));
			key.data_delFlow.flowKeyType = RT_FLOW_OP_FLOW_KEY_TYPE_PATTERN;
			key.data_delFlow.flowKey.flowPattern.dip6_valid = 1;
			memcpy(key.data_delFlow.flowKey.flowPattern.dip6 , &remoteAddress6_1, IPV6_ADDR_LEN);
			rt_flow_operation_add(RT_FLOW_OPS_DELETE_FLOW, &key);
		}

		//downstream rule
		mark_mask = SOCK_MARK_METER_INDEX_BIT_MASK|SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK;
		mark2_mask = SOCK_MARK2_MIB_MASK_ALL;
		meter_idx= rtk_cmcc_setup_traffic_qos_share_meter(flowEntry->flow+MAX_QOS_QUEUE_NUM, flowEntry->downmaxrate);

		if(meter_idx != -1){
			mark |= SOCK_MARK_METER_INDEX_ENABLE_TO_MARK(1);
			//mark |= SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(1);
			mark2 |= SOCK_MARK2_MIB_INDEX_ENABLE_TO_MARK(1);
			//mark2 |= SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(1);
		}
		else
		{
			return -1;
		}
		mark |= SOCK_MARK_METER_INDEX_TO_MARK(meter_idx);
		mark2 |= SOCK_MARK2_MIB_INDEX_TO_MARK(meter_idx);
	
		sprintf(str_mark, "0x%x/0x%x", mark, mark_mask);
		sprintf(str_mark2, "0x%llx/0x%llx", mark2, mark2_mask);
		if(isipv4==0){
			sprintf(cmd, "-t mangle %s %s ! -i %s %s -j MARK --set-mark %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_QOS_FWD, 
			(char *)BRIF, 
			condition_string_down, 
			str_mark);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle %s %s %s -j MARK2 --set-mark2 %s", 
			CMCC_FORWARDRULE_ACTION_APPEND(type), 
			(char *)TABLE_TRAFFIC_QOS_POST, 
			condition_string_down, 
			str_mark2);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

			//delete flow when finish setup iptables to apply setting
			memset(&key, 0, sizeof(key));
			key.data_delFlow.flowKeyType = RT_FLOW_OP_FLOW_KEY_TYPE_PATTERN;
			key.data_delFlow.flowKey.flowPattern.sip6_valid = 1;
			memcpy(key.data_delFlow.flowKey.flowPattern.sip6, &remoteAddress6_1, IPV6_ADDR_LEN);
			rt_flow_operation_add(RT_FLOW_OPS_DELETE_FLOW, &key);
		}
	}
#endif
#endif //CONFIG_RTK_SKB_MARK2

	return 0;
}

#else
int rtk_cmcc_setup_traffic_qos_rule(MIB_TRAFFIC_QOS_RULE_T *entry, MIB_TRAFFIC_QOS_RULE_FLOW_T *flowEntry, int type)
{
#ifdef CONFIG_RTK_SKB_MARK2
	int remoteporttype = -1, startport = 0, endport = 0;
	char condition_string[512] = {0};
	char condition_string_eb[512] = {0};
	char condition_string_down[512] = {0};
	char cmd[512] = {0};
	char tmp[128] = {0};
	char tmp_eb[128] = {0};
	char tmp1[128] = {0};
	char tmp_down[128] = {0};
	char str_mark[128] = {0};
	char str_mark2[128] = {0};
	char ipstr[64]={0}, ipaddr1[64]={0}, ipaddr2[64] ={0};
	struct in6_addr remoteAddress6_1, remoteAddress6_2;
	struct in_addr remoteAddress1, remoteAddress_tmp;
	int ret=0, isipv4=0;
	unsigned int maskbit=0;
	ipaddr_t mask;
	int mark=0, mark_mask=0, meter_idx;
	unsigned long long mark2=0, mark2_mask=0;
	int mib_idx;
	rt_flow_ops_data_t key;
	char m_dscp[24];

	memset(condition_string, 0, sizeof(condition_string));
	memset(condition_string_down, 0, sizeof(condition_string_down));
	memset(condition_string_eb, 0, sizeof(condition_string_eb));
	
	DOCMDINIT;
	
	printf("%s:%d type %d\n", __FUNCTION__, __LINE__, type);
	printf("%s:%d remoteAddress %s, realremoteAddress %s, remotePort %s, flow %d\n", __FUNCTION__, __LINE__, 
		entry->remoteAddress, 
		entry->realremoteAddress, 
		entry->remotePort,
		entry->flow);
	
	//remoteAddress
	memset(tmp, 0, 128);
	memset(tmp_eb, 0, 128);
	memset(tmp_down, 0, 128);
	if(!strcmp(entry->remoteAddress, "") || !strcmp(entry->remoteAddress, "0")){
		strcpy(tmp, "");
		strcpy(tmp_eb, "");
		strcpy(tmp_down, "");
	}
	else{
		ret = rtk_osgi_traffic_parse_address(entry->remoteAddress,ipstr,&maskbit);
		if(ret==2 || ret==-1){
			sprintf(tmp, " -d %s", entry->realremoteAddress);
			sprintf(tmp_down, " -s %s", entry->realremoteAddress);
			if(inet_pton(AF_INET, entry->realremoteAddress, &remoteAddress1) == 1){
				isipv4=1;
				sprintf(tmp_eb, " --ip-destination %s", entry->realremoteAddress);
			}
			else if(inet_pton(AF_INET6, entry->realremoteAddress, &remoteAddress6_1) == 1){
				sprintf(tmp_eb, " --ip6-destination %s", entry->realremoteAddress);
			}
		}
		else if(ret==1){
			if(inet_pton(AF_INET, ipstr, &remoteAddress1) == 1){
				isipv4=1;
				mask = ~0 << (sizeof(ipaddr_t)*8 - maskbit);
				mask = htonl(mask);
				remoteAddress_tmp.s_addr = (remoteAddress1.s_addr & mask);
				inet_ntop(AF_INET, &remoteAddress_tmp, ipaddr1, INET_ADDRSTRLEN);
				remoteAddress_tmp.s_addr = (remoteAddress1.s_addr | ~mask);
				inet_ntop(AF_INET, &remoteAddress_tmp, ipaddr2, INET_ADDRSTRLEN);
				sprintf(tmp, " -m iprange --dst-range %s-%s", ipaddr1, ipaddr2);
				sprintf(tmp_eb, " --ip-destination %s/%d", ipaddr1, maskbit);
				sprintf(tmp_down, " -m iprange --src-range %s-%s", ipaddr1, ipaddr2);
			}
#ifdef CONFIG_IPV6
			else if(inet_pton(AF_INET6, ipstr, &remoteAddress6_1) == 1){
				rtk_ipv6_ip_address_range(&remoteAddress6_1, maskbit, &remoteAddress6_1, &remoteAddress6_2);
				inet_ntop(AF_INET6, &remoteAddress6_1, ipaddr1, INET6_ADDRSTRLEN);
				inet_ntop(AF_INET6, &remoteAddress6_2, ipaddr2, INET6_ADDRSTRLEN);
				sprintf(tmp, " -m iprange --dst-range %s-%s", ipaddr1, ipaddr2);
				sprintf(tmp_eb, " --ip6-destination %s/%d", ipaddr1, maskbit);
				sprintf(tmp_down, " -m iprange --src-range %s-%s", ipaddr1, ipaddr2);
			}
#endif
			else{
				printf("%s:%d\n", __FUNCTION__, __LINE__);
				return -1;
			}
		}
		else{
			printf("%s:%d\n", __FUNCTION__, __LINE__);
			return -1;
		}
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
	strcat(condition_string_eb, tmp_eb);
	
	//remote port
	memset(tmp, 0, 128);
	memset(tmp_eb, 0, 128);
	memset(tmp_down, 0, 128);
	remoteporttype = parseRemotePort(entry->remotePort, &startport, &endport);
	if(remoteporttype == 3){//remote port == 0, don't card
		strcpy(tmp, "");
		strcpy(tmp_down, "");
		strcpy(tmp_eb, "");
	}
	else if(remoteporttype == 1){ //format !x
		sprintf(tmp, " ! --dport %d", startport);
		sprintf(tmp_down, " ! --sport %d", startport);
		sprintf(tmp_eb, " ! --ip-dport %d", startport);
	}
	else{
		strcpy(tmp1, entry->remotePort);
		sprintf(tmp, " --dport %s", replaceChar(tmp1, '-', ':'));
		sprintf(tmp_down, " --sport %s", replaceChar(tmp1, '-', ':'));
		sprintf(tmp_eb, " --ip-dport %s", replaceChar(tmp1, '-', ':'));
	}
	strcat(condition_string, tmp);
	strcat(condition_string_down, tmp_down);
	strcat(condition_string_eb, tmp_eb);

	//upstream rule
	mark_mask = SOCK_MARK_METER_INDEX_BIT_MASK|SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK;
	mark2_mask = SOCK_MARK2_MIB_MASK_ALL;
	meter_idx= rtk_cmcc_setup_traffic_qos_share_meter(flowEntry->flow, flowEntry->upmaxrate);

	if(meter_idx != -1){
		mark |= SOCK_MARK_METER_INDEX_ENABLE_TO_MARK(1);
		//mark |= SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(1);
		mark2 |= SOCK_MARK2_MIB_INDEX_ENABLE_TO_MARK(1);
		//mark2 |= SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(1);
	}
	else
	{
		return -1;
	}
	mark |= SOCK_MARK_METER_INDEX_TO_MARK(meter_idx);
	mark2 |= SOCK_MARK2_MIB_INDEX_TO_MARK(meter_idx);

	sprintf(str_mark, "0x%x/0x%x", mark, mark_mask);
	sprintf(str_mark2, "0x%llx/0x%llx", mark2, mark2_mask);

	snprintf(m_dscp, 24, "0x%x", flowEntry->dscp&0xFF);
		
	if(isipv4==1){
		if(remoteporttype == 3){
			sprintf(cmd, "-t mangle %s %s -i %s %s -j MARK --set-mark %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_FWD, 
				(char *)BRIF, 
				condition_string, 
				str_mark);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_POST, 
				condition_string, 
				str_mark2);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t broute %s %s -p IPv4 %s -j ftos --set-ftos %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_EB,  
				condition_string_eb,
				m_dscp);
			DOCMDARGVS(EBTABLES, DOWAIT, cmd);
		}
		else{
			sprintf(cmd, "-t mangle %s %s -i %s -p tcp %s -j MARK --set-mark %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_FWD, 
				(char *)BRIF, 
				condition_string, 
				str_mark);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle -p tcp %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_POST, 
				condition_string, 
				str_mark2);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t broute %s %s -p IPv4 --ip-proto tcp %s -j ftos --set-ftos %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_EB,  
				condition_string_eb,
				m_dscp);
			DOCMDARGVS(EBTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle %s %s -i %s -p udp %s -j MARK --set-mark %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_FWD, 
				(char *)BRIF, 
				condition_string, 
				str_mark);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle -p udp %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_POST, 
				condition_string, 
				str_mark2);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t broute %s %s -p IPv4 --ip-proto udp %s -j ftos --set-ftos %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_EB,  
				condition_string_eb,
				m_dscp);
			DOCMDARGVS(EBTABLES, DOWAIT, cmd);
		}

		//delete flow when finish setup iptables to apply setting
		memset(&key, 0, sizeof(key));
		key.data_delFlow.flowKeyType = RT_FLOW_OP_FLOW_KEY_TYPE_PATTERN;
		key.data_delFlow.flowKey.flowPattern.dip4_valid = 1;
		key.data_delFlow.flowKey.flowPattern.dip4 =  htonl(remoteAddress1.s_addr);
		rt_flow_operation_add(RT_FLOW_OPS_DELETE_FLOW, &key);
	}
	else{
#ifdef CONFIG_IPV6
		if(remoteporttype == 3){
			sprintf(cmd, "-t mangle %s %s -i %s %s -j MARK --set-mark %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_FWD, 
				(char *)BRIF, 
				condition_string, 
				str_mark);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_POST, 
				condition_string, 
				str_mark2);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

			sprintf(cmd, "-t broute %s %s -p 0x86dd %s -j ftos --set-ftos %s", 
					CMCC_FORWARDRULE_ACTION_APPEND(type), 
					(char *)TABLE_TRAFFIC_QOS_EB,  
					condition_string_eb,
					m_dscp);
			DOCMDARGVS(EBTABLES, DOWAIT, cmd);
		}
		else{
			sprintf(cmd, "-t mangle %s %s -i %s -p tcp %s -j MARK --set-mark %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_FWD, 
				(char *)BRIF, 
				condition_string, 
				str_mark);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle -p tcp %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_POST, 
				condition_string, 
				str_mark2);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

			sprintf(cmd, "-t broute %s %s -p IPv4 --ip-proto tcp %s -j ftos --set-ftos %s", 
					CMCC_FORWARDRULE_ACTION_APPEND(type), 
					(char *)TABLE_TRAFFIC_QOS_EB,  
					condition_string_eb,
					m_dscp);
			DOCMDARGVS(EBTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle %s %s -i %s -p udp %s -j MARK --set-mark %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_FWD, 
				(char *)BRIF, 
				condition_string, 
				str_mark);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle -p udp %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_POST, 
				condition_string, 
				str_mark2);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

			sprintf(cmd, "-t broute %s %s -p IPv4 --ip-proto udp %s -j ftos --set-ftos %s", 
					CMCC_FORWARDRULE_ACTION_APPEND(type), 
					(char *)TABLE_TRAFFIC_QOS_EB,  
					condition_string_eb,
					m_dscp);
			DOCMDARGVS(EBTABLES, DOWAIT, cmd);
		}

		//delete flow when finish setup iptables to apply setting
#if 0
		memset(&key, 0, sizeof(key));
		key.data_delFlow.flowKeyType = RT_FLOW_OP_FLOW_KEY_TYPE_PATTERN;
		key.data_delFlow.flowKey.flowPattern.dip6_valid = 1;
		memcpy(key.data_delFlow.flowKey.flowPattern.dip6 , &remoteAddress6_1, IPV6_ADDR_LEN);
		rt_flow_operation_add(RT_FLOW_OPS_DELETE_FLOW, &key);
#endif
#endif
	}
	
	//downstream rule
	mark_mask = SOCK_MARK_METER_INDEX_BIT_MASK|SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK;
	mark2_mask = SOCK_MARK2_MIB_MASK_ALL;
	meter_idx= rtk_cmcc_setup_traffic_qos_share_meter(flowEntry->flow+MAX_QOS_QUEUE_NUM, flowEntry->downmaxrate);

	if(meter_idx != -1){
		mark |= SOCK_MARK_METER_INDEX_ENABLE_TO_MARK(1);
		//mark |= SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(1);
		mark2 |= SOCK_MARK2_MIB_INDEX_ENABLE_TO_MARK(1);
		//mark2 |= SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(1);
	}
	else
	{
		return -1;
	}
	mark |= SOCK_MARK_METER_INDEX_TO_MARK(meter_idx);
	mark2 |= SOCK_MARK2_MIB_INDEX_TO_MARK(meter_idx);
	
	sprintf(str_mark, "0x%x/0x%x", mark, mark_mask);
	sprintf(str_mark2, "0x%llx/0x%llx", mark2, mark2_mask);

	if(isipv4==1){
		if(remoteporttype == 3){
			sprintf(cmd, "-t mangle %s %s ! -i %s %s -j MARK --set-mark %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_FWD, 
				(char *)BRIF, 
				condition_string_down, 
				str_mark);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_POST,  
				condition_string_down, 
				str_mark2);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);
		}
		else{
			sprintf(cmd, "-t mangle %s %s ! -i %s -p tcp %s -j MARK --set-mark %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_FWD, 
				(char *)BRIF, 
				condition_string_down, 
				str_mark);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle -p tcp %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_POST,  
				condition_string_down, 
				str_mark2);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle %s %s ! -i %s -p udp %s -j MARK --set-mark %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_FWD, 
				(char *)BRIF, 
				condition_string_down, 
				str_mark);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle -p udp %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_POST,  
				condition_string_down, 
				str_mark2);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);
		}

		//delete flow when finish setup iptables to apply setting
		memset(&key, 0, sizeof(key));
		key.data_delFlow.flowKeyType = RT_FLOW_OP_FLOW_KEY_TYPE_PATTERN;
		key.data_delFlow.flowKey.flowPattern.sip4_valid = 1;
		key.data_delFlow.flowKey.flowPattern.sip4 =  htonl(remoteAddress1.s_addr);
		rt_flow_operation_add(RT_FLOW_OPS_DELETE_FLOW, &key);
	}
	else{	
#ifdef CONFIG_IPV6
		if(remoteporttype == 3){
			sprintf(cmd, "-t mangle %s %s ! -i %s %s -j MARK --set-mark %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_FWD, 
				(char *)BRIF, 
				condition_string_down, 
				str_mark);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_POST, 
				condition_string_down, 
				str_mark2);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
		}
		else{
			sprintf(cmd, "-t mangle %s %s ! -i %s -p tcp %s -j MARK --set-mark %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_FWD, 
				(char *)BRIF, 
				condition_string_down, 
				str_mark);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle -p tcp %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_POST, 
				condition_string_down, 
				str_mark2);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle %s %s ! -i %s -p udp %s -j MARK --set-mark %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_FWD, 
				(char *)BRIF, 
				condition_string_down, 
				str_mark);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);

			sprintf(cmd, "-t mangle -p udp %s %s %s -j MARK2 --set-mark2 %s", 
				CMCC_FORWARDRULE_ACTION_APPEND(type), 
				(char *)TABLE_TRAFFIC_QOS_POST, 
				condition_string_down, 
				str_mark2);
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
		}

		//delete flow when finish setup iptables to apply setting
#if 0
		memset(&key, 0, sizeof(key));
		key.data_delFlow.flowKeyType = RT_FLOW_OP_FLOW_KEY_TYPE_PATTERN;
		key.data_delFlow.flowKey.flowPattern.sip6_valid = 1;
		memcpy(key.data_delFlow.flowKey.flowPattern.sip6, &remoteAddress6_1, IPV6_ADDR_LEN);
		rt_flow_operation_add(RT_FLOW_OPS_DELETE_FLOW, &key);
#endif
#endif
	}
#endif //CONFIG_RTK_SKB_MARK2

	return 0;
}
#endif

int rtk_cmcc_add_traffic_qos_rule(MIB_TRAFFIC_QOS_RULE_T *entry, MIB_TRAFFIC_QOS_RULE_FLOW_T *flowEntry)
{
	return rtk_cmcc_setup_traffic_qos_rule(entry, flowEntry, 1);
}

int rtk_cmcc_del_traffic_qos_rule(MIB_TRAFFIC_QOS_RULE_T *entry, MIB_TRAFFIC_QOS_RULE_FLOW_T *flowEntry)
{
	return rtk_cmcc_setup_traffic_qos_rule(entry, flowEntry, 0);
}

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
int rtk_cmcc_update_traffic_qos_rule_url(char*domainName, int containip, char* addr, char *addr6)
{
	int EntryNum = 0;
	int i = 0;
	MIB_TRAFFIC_QOS_RULE_T entry;
	MIB_TRAFFIC_QOS_RULE_FLOW_T flowEntry;
	int iptype = 0;;
	int ret = 0;
	
	EntryNum = mib_chain_total(MIB_TRAFFIC_QOS_RULE_TBL);
	for(i=0; i < EntryNum; i++){
		if(!mib_chain_get(MIB_TRAFFIC_QOS_RULE_TBL, i, &entry))
			continue;
		if(!strcmp(domainName, entry.remoteAddress)){
	
			printf("%s:%d: domainName %s, addr %s, addr6 %s", __FUNCTION__, __LINE__, domainName, addr, addr6);
			rtk_cmcc_find_traffic_qos_flowentry(entry.flow, &flowEntry);
			rtk_cmcc_del_traffic_qos_rule(&entry, &flowEntry);
			if(containip & CONTAINS_IPV4){
				strcpy(entry.realremoteip4, addr);
			}
			if(containip & CONTAINS_IPV6){
				strcpy(entry.realremoteip6, addr6);
			}
			
			ret = rtk_cmcc_add_traffic_qos_rule(&entry, &flowEntry);
			if(ret >= 0){//if set success, update mib
				mib_chain_update(MIB_TRAFFIC_QOS_RULE_TBL, &entry, i);
			}

		}
	}
	return ret;
}
#endif

/*
direction 0 => upstream
direction 1 => downstream
*/
int rtk_cmcc_get_traffic_qos_share_meter(MIB_TRAFFIC_QOS_RULE_FLOW_T *flowEntry, int direction)
{
	int meter_idx;

	//upstream rate
	if(direction == 0){
		meter_idx = rtk_qos_share_meter_get(RTK_APP_TYPE_CMCC_QOS_RATE_LIMIT, flowEntry->flow, flowEntry->upmaxrate);
	}
	else{//downstream rate
		meter_idx = rtk_qos_share_meter_get(RTK_APP_TYPE_CMCC_QOS_RATE_LIMIT, flowEntry->flow+MAX_QOS_QUEUE_NUM, flowEntry->downmaxrate);
	}

	return meter_idx;
}


int rtk_cmcc_setup_traffic_qos_share_meter(int flow, unsigned int rate)
{
	int meter_idx;
	unsigned int rate_mod=0;

	meter_idx = rtk_qos_share_meter_get(RTK_APP_TYPE_CMCC_QOS_RATE_LIMIT, flow, rate);
	if (meter_idx >= 0){
		if(rate==0){
			rate_mod = RT_MAX_RATE;
		}
		else{
			rate_mod = rate;
		}
		rtk_qos_share_meter_update_rate(RTK_APP_TYPE_CMCC_QOS_RATE_LIMIT, flow, rate_mod);
	}
	else{
		if(rate==0){
			rate_mod = RT_MAX_RATE;
		}
		else{
			rate_mod = rate;
		}
		meter_idx = rtk_qos_share_meter_set(RTK_APP_TYPE_CMCC_QOS_RATE_LIMIT, flow, rate_mod);
	}
	printf("%s:%d flow %d, rate %u, rate_mod %u, meter_idx %d\n", __FUNCTION__, __LINE__, flow, rate, rate_mod, meter_idx);
	return meter_idx;
}

int rtk_cmcc_mod_traffic_qos_share_meter(MIB_TRAFFIC_QOS_RULE_FLOW_T *flowEntry)
{
	//upstream rate
	rtk_cmcc_setup_traffic_qos_share_meter(flowEntry->flow, flowEntry->upmaxrate);
	
	//downstream rate
	rtk_cmcc_setup_traffic_qos_share_meter(flowEntry->flow+MAX_QOS_QUEUE_NUM, flowEntry->downmaxrate);

	return 0;
}

int rtk_cmcc_flush_traffic_qos_share_meter(MIB_TRAFFIC_QOS_RULE_FLOW_T *flowEntry)
{
	int meter_idx;
	int flow;

	//upstream rate
	meter_idx = rtk_qos_share_meter_get(RTK_APP_TYPE_CMCC_QOS_RATE_LIMIT, flowEntry->flow, flowEntry->upmaxrate);
	if (meter_idx >= 0){
		rtk_qos_share_meter_delete(RTK_APP_TYPE_CMCC_QOS_RATE_LIMIT, flowEntry->flow);
	}

	//downstream rate
	meter_idx = rtk_qos_share_meter_get(RTK_APP_TYPE_CMCC_QOS_RATE_LIMIT, flowEntry->flow+MAX_QOS_QUEUE_NUM, flowEntry->downmaxrate);

	if (meter_idx >= 0){
		rtk_qos_share_meter_delete(RTK_APP_TYPE_CMCC_QOS_RATE_LIMIT, flowEntry->flow+MAX_QOS_QUEUE_NUM);
	}

	return 0;
}
#endif //#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#endif	//(defined(CONFIG_CMCC) && defined(CONFIG_APACHE_FELIX_FRAMEWORK)) || defined(CONFIG_CU)

#ifdef _PRMT_X_CMCC_LANINTERFACES_
const char FW_ETHERTYPE_FILTER_BRIDGE_INPUT[] = "ethertype_filter_b_INPUT";
const char FW_ETHERTYPEF_ILTER_BRIDGE_FORWARD[] = "ethertype_filter_b_FORWARD";

void rtk_cmcc_flush_ethertype_filter(void)
{
	char cmd[256];
	snprintf(cmd, sizeof(cmd), "echo 7 > %s", KMODULE_CMCC_MAC_FILTER_FILE);
	system(cmd);

	/* ethertype_filter_b_INPUT */
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT);
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_INPUT, "-j", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT);

	/* ethertype_filter_b_FORWARD */
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_ETHERTYPEF_ILTER_BRIDGE_FORWARD);
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)FW_ETHERTYPEF_ILTER_BRIDGE_FORWARD);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_ETHERTYPEF_ILTER_BRIDGE_FORWARD);

	return;
}

void rtk_cmcc_setup_ethertype_filter(void)
{
	int i = 0, j=0, idx=0, ssid_idx=0, total = 0, is_mac_set = 0, type = 0, is_drop_rule_set = 0;
	unsigned char mac_str_src[32] = {0}, mac_str_dst[32] = {0}, type_str[32] = {0}, port_ifname[16] = {0}, rule_cmd[256] = {0};
	MIB_CE_L2FILTER_T entry;
	char cmd[256];

	/* ethertype_filter_b_INPUT */
	va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT);
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT, "RETURN");

	/* ethertype_filter_b_FORWARD */
	va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_ETHERTYPEF_ILTER_BRIDGE_FORWARD);
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_ETHERTYPEF_ILTER_BRIDGE_FORWARD);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)FW_ETHERTYPEF_ILTER_BRIDGE_FORWARD, "RETURN");

	DOCMDINIT;

	total = mib_chain_total(MIB_L2FILTER_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2FILTER_TBL, i, (void *)&entry))
			continue;

		is_mac_set = 0;
		memset(port_ifname, 0, sizeof(port_ifname));
		memset(mac_str_src, 0, sizeof(mac_str_src));
		memset(mac_str_dst, 0, sizeof(mac_str_dst));
		if (i < PMAP_WLAN0)
		{
			sprintf(port_ifname, "%s", SW_LAN_PORT_IF[i]);
		}
#ifdef CONFIG_WLAN
		else
		{
			for(j=0; j<NUM_WLAN_INTERFACE; j++)
			{
				idx = j*(PMAP_WLAN0_VAP_END-PMAP_WLAN0+1) + PMAP_WLAN0;
				if (idx == i)
				{
					ssid_idx =j*(WLAN_MBSSID_NUM+1) + 1;
					rtk_wlan_get_ifname_by_ssid_idx(ssid_idx, port_ifname);
					break;
				}
			}
		}
#endif
		if (memcmp(entry.src_mac, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LEN))
		{
			sprintf(mac_str_src, "-s %02x:%02x:%02x:%02x:%02x:%02x", entry.src_mac[0], entry.src_mac[1], entry.src_mac[2], entry.src_mac[3], entry.src_mac[4], entry.src_mac[5]);
			is_mac_set = 1;
			// record the mac into Mac filter list kernel module
			// echo PROCFS_MAC_FILTER_ENTRY_ADD_ETHERTYPE [aclidx=0] [mac] [lanPort]>  KMODULE_CMCC_MAC_FILTER_FILE
			if (i < SW_LAN_PORT_NUM)  {
				snprintf(cmd, sizeof(cmd), "echo 6 0 %02X:%02X:%02X:%02X:%02X:%02X %d > %s",
					entry.src_mac[0], entry.src_mac[1], entry.src_mac[2], entry.src_mac[3], entry.src_mac[4], entry.src_mac[5],  i+1,
					KMODULE_CMCC_MAC_FILTER_FILE);
			}
			else {
				snprintf(cmd, sizeof(cmd), "echo 6 0 %02X:%02X:%02X:%02X:%02X:%02X 0 > %s",
					entry.src_mac[0], entry.src_mac[1], entry.src_mac[2], entry.src_mac[3], entry.src_mac[4], entry.src_mac[5],
					KMODULE_CMCC_MAC_FILTER_FILE);
			}
			system(cmd);
		}

		if (memcmp(entry.dst_mac, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LEN))
		{
			sprintf(mac_str_dst, "-d %02x:%02x:%02x:%02x:%02x:%02x", entry.dst_mac[0], entry.dst_mac[1], entry.dst_mac[2], entry.dst_mac[3], entry.dst_mac[4], entry.dst_mac[5]);
			is_mac_set = 1;
		}

		if (entry.eth_type == 0 && is_mac_set == 0)
		{
			continue;
		}
		else if (entry.eth_type == 0 && is_mac_set == 1)
		{
			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-A %s -i %s %s %s -j DROP", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT, port_ifname, mac_str_src, mac_str_dst);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);

			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-A %s -i %s %s %s -j DROP", (char *)FW_ETHERTYPEF_ILTER_BRIDGE_FORWARD, port_ifname, mac_str_src, mac_str_dst);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);

			is_drop_rule_set = 1;
		}
		else
		{
			for (type = 1; type < L2FILTER_ETH_END; type <<= 1 )
			{
				if ((entry.eth_type & type) == 0)
					continue;

				memset(type_str, 0, sizeof(type_str));

				switch (type)
				{
					case L2FILTER_ETH_IPV4OE:
						strcpy(type_str, "-p 0x0800");
						break;
					case L2FILTER_ETH_PPPOE:
						strcpy(type_str, "-p 0x8863");
						break;
					case L2FILTER_ETH_ARP:
						strcpy(type_str, "-p 0x0806");
						break;
					case L2FILTER_ETH_IPV6OE:
						strcpy(type_str, "-p 0x86dd");
						break;
					default:
						fprintf(stderr, "<%s:%d> Unknow eth_type: %x", __func__, __LINE__, type);
						break;
				}

				if (strlen(type_str))
				{
					if (is_mac_set != 1)
					{
						memset(rule_cmd, 0, sizeof(rule_cmd));
						sprintf(rule_cmd, "-A %s -i %s %s %s %s -j DROP", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT, port_ifname, mac_str_src, mac_str_dst, type_str);
						DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
					}
					memset(rule_cmd, 0, sizeof(rule_cmd));
					sprintf(rule_cmd, "-A %s -i %s %s %s %s -j DROP", (char *)FW_ETHERTYPEF_ILTER_BRIDGE_FORWARD, port_ifname, mac_str_src, mac_str_dst, type_str);
					DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);

					is_drop_rule_set = 1;
				}
			}
		}
	}

	//Local in default rules, copy from setupMacFilterTables()
	unsigned char tmpBuf[100]={0};
	unsigned char value[6];
	mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)tmpBuf, sizeof(tmpBuf));
	// ebtables -I macfilter -p IPv6 --ip6-dst fe80::1 -j ACCEPT
	if(is_drop_rule_set == 1)
	{
		DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s -j RETURN", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT, tmpBuf);

		DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type echo-request -j RETURN", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type echo-reply -j RETURN", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type router-solicitation -j RETURN", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type router-advertisement -j RETURN", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-solicitation -j RETURN", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-advertisement -j RETURN", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst ff02::1:ff00:0000/ffff:ffff:ffff:ffff:ffff:ffff:ff00:0 --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-solicitation -j RETURN", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT);
	}

	DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv4 --ip-proto udp --ip-dport 67:68 -j RETURN", (char *)FW_MACFILTER_LI); 

	mib_get_s(MIB_ADSL_LAN_IP, (void *)tmpBuf, sizeof(tmpBuf));
	if(is_drop_rule_set == 1) DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv4 --ip-dst %s -j RETURN", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT, inet_ntoa(*((struct in_addr *)tmpBuf)));
#ifdef CONFIG_SECONDARY_IP
	mib_get_s(MIB_ADSL_LAN_ENABLE_IP2, (void *)value, sizeof(value));
	if (value[0] == 1) {
		mib_get_s(MIB_ADSL_LAN_IP2, (void *)tmpBuf, sizeof(tmpBuf));
		if(is_drop_rule_set == 1) DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv4 --ip-dst %s -j RETURN", (char *)FW_ETHERTYPE_FILTER_BRIDGE_INPUT, inet_ntoa(*((struct in_addr *)tmpBuf)));
	}
#endif

	return;
}

int setupL2Filter(void)
{
	rtk_cmcc_flush_ethertype_filter();

	rtk_cmcc_setup_ethertype_filter();

#ifdef CONFIG_RTL8672_BRIDGE_FASTPATH
	TRACE(STA_SCRIPT,"/bin/echo 2 > /proc/fastbridge\n");
	system("/bin/echo 2 > /proc/fastbridge");
#endif

	return 0;
}

#if defined(DHCP_ARP_IGMP_RATE_LIMIT)
const char FW_MAC_BC_LIMIT_BRIDGE_INPUT[] = "mac_bc_limit_b_INPUT";
const char FW_MAC_BC_LIMIT_BRIDGE_FORWARD[] = "mac_bc_limit_b_FORWARD";
void rtk_cmcc_flush_mac_bc_limit(void)
{
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_MAC_BC_LIMIT_BRIDGE_INPUT);
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_INPUT, "-j", (char *)FW_MAC_BC_LIMIT_BRIDGE_INPUT);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_MAC_BC_LIMIT_BRIDGE_INPUT);

	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_MAC_BC_LIMIT_BRIDGE_FORWARD);
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)FW_MAC_BC_LIMIT_BRIDGE_FORWARD);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_MAC_BC_LIMIT_BRIDGE_FORWARD);
}

void rtk_cmcc_setup_mac_bc_limit(void)
{
	int i = 0, total = 0, is_mac_set = 0, type = 0;
	unsigned char mac_str_src[32]={0}, mac_str_dst[32]={0}, port_ifname[16]={0}, rule_cmd[256]={0}, mac_cmd[64]={0};
	MIB_CE_L2FILTER_T entry;
	int totalLimit = 0,idx = 0;
	MIB_L2_BC_LIMIT_T entryLimit;

	va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_MAC_BC_LIMIT_BRIDGE_INPUT);
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_MAC_BC_LIMIT_BRIDGE_INPUT);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)FW_MAC_BC_LIMIT_BRIDGE_INPUT, "RETURN");

	va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_MAC_BC_LIMIT_BRIDGE_FORWARD);
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_MAC_BC_LIMIT_BRIDGE_FORWARD);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)FW_MAC_BC_LIMIT_BRIDGE_FORWARD, "RETURN");

	DOCMDINIT;

	for (i = 0; i < SW_LAN_PORT_NUM; i++)
	{
		if (!mib_chain_get(MIB_L2FILTER_TBL, i, (void *)&entry))
			continue;

		is_mac_set = 0;
		memset(port_ifname, 0, sizeof(port_ifname));
		memset(mac_str_src, 0, sizeof(mac_str_src));
		memset(mac_str_dst, 0, sizeof(mac_str_dst));

		sprintf(port_ifname, "%s", SW_LAN_PORT_IF[i]);

		if (memcmp(entry.src_mac, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LEN))
		{
			sprintf(mac_str_src, "%02x:%02x:%02x:%02x:%02x:%02x", entry.src_mac[0], entry.src_mac[1], entry.src_mac[2], entry.src_mac[3], entry.src_mac[4], entry.src_mac[5]);
			is_mac_set = 1;
		}

		if (memcmp(entry.dst_mac, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LEN))
		{
			sprintf(mac_str_dst, "%02x:%02x:%02x:%02x:%02x:%02x", entry.dst_mac[0], entry.dst_mac[1], entry.dst_mac[2], entry.dst_mac[3], entry.dst_mac[4], entry.dst_mac[5]);
			if(is_mac_set)
				is_mac_set = 2;
			else
				is_mac_set = 3;
		}

		if (!mib_chain_get(MIB_L2_BROADCAST_LIMIT_TBL, i,&entryLimit))
			continue;

#if defined(CONFIG_RTL_MULTI_LAN_DEV)
		MIB_CE_SW_PORT_T Entry;

		if (!mib_chain_get(MIB_SW_PORT_TBL, i, &Entry))
			continue;

		if (!Entry.enable)
			continue;
#else
		unsigned char vChar;

		mib_get(CWMP_LAN_ETHIFENABLE, &vChar);
		if (vChar==0)
			continue;
#endif

		memset(mac_cmd, 0, sizeof(mac_cmd));
		switch(is_mac_set)
		{
			case 0:
				sprintf(mac_cmd, "");
				break;
			case 1:
				sprintf(mac_cmd, "-s %s", mac_str_src);
				break;
			case 2:
				sprintf(mac_cmd, "-s %s -d %s", mac_str_src, mac_str_dst);
				break;
			case 3:
				sprintf(mac_cmd, "-d %s", mac_str_dst);
				break;
		}

		if(entryLimit.dhcpLimit){

			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-A %s -i %s -p IPv4 --ip-proto udp --ip-dport 67:68 -j DROP",
					(char *)FW_MAC_BC_LIMIT_BRIDGE_INPUT, port_ifname);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-A %s -i %s -p IPv4 --ip-proto udp --ip-dport 67:68 -j DROP", 
					(char *)FW_MAC_BC_LIMIT_BRIDGE_FORWARD, port_ifname);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
#ifdef CONFIG_IPV6
			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-A %s -i %s -p IPv6 --ip6-proto udp --ip6-dport 546:547 -j DROP", 
					(char *)FW_MAC_BC_LIMIT_BRIDGE_INPUT, port_ifname);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-A %s -i %s -p IPv6 --ip6-proto udp --ip6-dport 546:547 -j DROP", 
					(char *)FW_MAC_BC_LIMIT_BRIDGE_FORWARD, port_ifname);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
#endif

			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-I %s -i %s -p IPv4 %s --ip-proto udp --ip-dport 67:68 --limit %u/second --limit-burst 2 -j ACCEPT",
					(char *)FW_MAC_BC_LIMIT_BRIDGE_INPUT, port_ifname, mac_cmd, entryLimit.dhcpLimit);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-I %s -i %s -p IPv4 %s --ip-proto udp --ip-dport 67:68 --limit %u/second --limit-burst 2 -j ACCEPT", 
					(char *)FW_MAC_BC_LIMIT_BRIDGE_FORWARD, port_ifname, mac_cmd, entryLimit.dhcpLimit);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
#ifdef CONFIG_IPV6
			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-I %s -i %s -p IPv6 %s --ip6-proto udp --ip6-dport 546:547 --limit %u/second --limit-burst 2 -j ACCEPT",
					(char *)FW_MAC_BC_LIMIT_BRIDGE_INPUT, port_ifname, mac_cmd, entryLimit.dhcpLimit);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-I %s -i %s -p IPv6 %s --ip6-proto udp --ip6-dport 546:547 --limit %u/second --limit-burst 2 -j ACCEPT",
					(char *)FW_MAC_BC_LIMIT_BRIDGE_FORWARD, port_ifname, mac_cmd, entryLimit.dhcpLimit);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
#endif
		}

		if(entryLimit.arpLimit)
		{
			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-A %s -i %s -p ARP -j DROP",
					(char *)FW_MAC_BC_LIMIT_BRIDGE_INPUT, port_ifname);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-A %s -i %s -p ARP -j DROP", 
					(char *)FW_MAC_BC_LIMIT_BRIDGE_FORWARD, port_ifname);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);

			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-I %s -i %s -p ARP %s --arp-opcode 1 --limit %u/second --limit-burst 2 -j ACCEPT",
					(char *)FW_MAC_BC_LIMIT_BRIDGE_INPUT, port_ifname, mac_cmd, entryLimit.arpLimit);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-I %s -i %s -p ARP %s --arp-opcode 1 --limit %u/second --limit-burst 2 -j ACCEPT", 
					(char *)FW_MAC_BC_LIMIT_BRIDGE_FORWARD, port_ifname, mac_cmd, entryLimit.arpLimit);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
		}

		if(entryLimit.igmpLimit)
		{
			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-A %s -i %s -p IPv4 --ip-proto igmp -j DROP",
					(char *)FW_MAC_BC_LIMIT_BRIDGE_INPUT, port_ifname);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-A %s -i %s -p IPv4 --ip-proto igmp -j DROP", 
					(char *)FW_MAC_BC_LIMIT_BRIDGE_FORWARD, port_ifname);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);

			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-I %s -i %s -p IPv4 %s --ip-proto igmp --limit %u/second --limit-burst 2 -j ACCEPT",
					(char *)FW_MAC_BC_LIMIT_BRIDGE_INPUT, port_ifname, mac_cmd, entryLimit.igmpLimit);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
			memset(rule_cmd, 0, sizeof(rule_cmd));
			sprintf(rule_cmd, "-I %s -i %s -p IPv4 %s --ip-proto igmp --limit %u/second --limit-burst 2 -j ACCEPT", 
					(char *)FW_MAC_BC_LIMIT_BRIDGE_FORWARD, port_ifname, mac_cmd, entryLimit.igmpLimit);
			DOCMDARGVS(EBTABLES, DOWAIT, rule_cmd);
		}
	}
}

int setupMACBCLimit(void)
{
	rtk_cmcc_flush_mac_bc_limit();

	rtk_cmcc_setup_mac_bc_limit();

	return 0;
}
#endif

int setupMACLimit(void)
{
	MIB_CE_ELAN_CONF_T entry;
	int i = 0, phyport = 0, learning_limit = 0, current_learning_limit = 0, total = 0;
	int ethPhyPortId = rtk_port_get_wan_phyID();

	total = mib_chain_total(MIB_ELAN_CONF_TBL);
	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_ELAN_CONF_TBL, i, &entry) == 0)
			continue;

		phyport = rtk_port_get_lan_phyID(i);
		if (phyport == -1)
			continue;

		if (phyport == ethPhyPortId)
			continue;

		if (entry.mac_limit == 0)
		{
			learning_limit = -1;
		}
		else
		{
			learning_limit = entry.mac_limit;
		}

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
		if(rtk_port_check_external_port(i))
		{
			phyport -= EXTERNAL_SWITCH_PORT_OFFSET;
			int ret = rtk_switch_l2_limitLearningCnt_set(phyport, learning_limit);
			if( ret != RT_ERR_OK)
			{
				fprintf(stderr, "<%s:%d> rtksw_l2_limitLearningCnt_set failed, ret=%d\n", __func__, __LINE__, ret);
			}
		}
		else
#endif
#if defined(CONFIG_COMMON_RT_API)
		{
			rt_l2_portLimitLearningCnt_get(phyport, &current_learning_limit);
			if (current_learning_limit != learning_limit)
			{
				rt_l2_portLimitLearningCnt_set(phyport, learning_limit);
			}
		}
#endif
	}

	return 0;
}
#endif

#define OSGI_PREBUNDLE_DIRECTORY "/usr/local/class/prebundle"
#define OSGI_NO_OSGI_PARTITION	"/var/osgi_app/osgi_no_Partition"
void clean_filesystem(void)
{
	char cmd[512] = {0};
	int osgi_no_Partition=0;

	if (access(OSGI_NO_OSGI_PARTITION, F_OK) == 0) {
		osgi_no_Partition = 1;
	}
	printf("clean other config in file system!\n");
	unlink(MONITOR_LIST);
	system("killall -9 monitord");
	system("killall -9 java");
	if (access(OSGI_PREBUNDLE_DIRECTORY, F_OK) == 0) { 
		printf("[%s:%d] /usr/local/class/prebundle exist! osgi_no_Partition=%d \n", __FUNCTION__, __LINE__, osgi_no_Partition);
		if(osgi_no_Partition == 0) {
			sprintf(cmd, "rm -rf /var/osgi_app/prebundle %s %s", hideErrMsg1, hideErrMsg2);
			system(cmd);
		}
	}	
	sprintf(cmd, "rm -rf /var/osgi_app/bundle %s %s", hideErrMsg1, hideErrMsg2);
	system(cmd);
	sprintf(cmd, "rm -rf /var/osgi_app/felix-cache %s %s", hideErrMsg1, hideErrMsg2);
	system(cmd);
	sprintf(cmd, "rm -rf /var/osgi_app/config.properties %s %s", hideErrMsg1, hideErrMsg2);
	system(cmd);
	sprintf(cmd, "rm -rf /var/osgi_app/security.policy %s %s", hideErrMsg1, hideErrMsg2);
	system(cmd);
	sprintf(cmd, "rm -rf /var/osgi_app/bundle_installed %s %s", hideErrMsg1, hideErrMsg2);
	system(cmd);
	sprintf(cmd, "rm -rf /var/osgi_app/security %s %s", hideErrMsg1, hideErrMsg2);
	system(cmd);
	
	system("sync");
}

#if defined(CONFIG_APACHE_FELIX_FRAMEWORK) 
int rtk_osgi_getInfo(char *name, char **resultp, int *result_len)
{
	//get osgi internal info via http://localhost:8080/xxx , such as http://192.168.1.1:8080/PermissionInfo
	int sock_fd;
	struct sockaddr_in serverAddr;
	socklen_t addr_size;
	char buf[1024];
	int len;
	int body_len;
	char *p;

	if(*resultp != NULL){
		printf("rtk_osgi_getInfo error: result should be NULL!!\n");
		return -1;
	}
	if(result_len == NULL){
		printf("rtk_osgi_getInfo error: result_len should not be NULL!!\n");
		return -1;
	}
		
	*result_len = 0;
	if(strlen(name) > 38){
		printf("%s get name(%s) too long!!",__FUNCTION__,name);
		return -1;	
	}
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(8080);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);	

	addr_size = sizeof serverAddr;
	if (connect(sock_fd, (struct sockaddr *) &serverAddr, addr_size)<0) {
		close(sock_fd);
		perror("rtk_osgi_getInfo connect");
		return -1;	
	}
	
	//send http GET
	//GET /PermissionInfo HTTP/1.0
	//Host: 127.0.0.1:8080
	//Accept: */*
	sprintf(buf,"GET /%s HTTP/1.0\r\nHost: 127.0.0.1:8080\r\nAccept: */*\r\n\r\n",name);
	len = strlen(buf);
	if (send(sock_fd,buf,len,0) <0 ){
		close(sock_fd);
		perror("rtk_osgi_getInfo send");
		return -1;			
	}
	body_len = 0;
	*result_len = 0;
	while (len=recv(sock_fd, buf, 1023, 0)) 
	{ /* loop to retry in case it timed out */ 
		if(body_len == 0 && (p=strstr(buf, "Content-Length:"))!= NULL){
			p = p + strlen("Content-Length:");
			while(*p == 20) p++; //eat blank
			body_len = atoi(p);
		}
		if(strncmp(buf,"HTTP",4)==0 && (p = strstr(buf,"\r\n\r\n"))!= NULL){//cut head
			p += 4;
			//printf("hhhh p=%s\n",p);
			len = len-(p-buf);
		}else{
			//printf("not head \n");
			p = buf;
			}
		*resultp = realloc(*resultp, *result_len + len);
		if(*resultp == NULL){
			printf("rtk_osgi_getInfo error: realloc %d byte fail\n",*result_len + len);
			close(sock_fd);
			return -1;
		}
		memcpy(*resultp+*result_len,p, len);
		*result_len += len;
 	}

	close(sock_fd);

	if(body_len != *result_len){
		printf("rtk_osgi_getInfo error: parse http body error\n");
		free(*resultp);
		*resultp = NULL;
		return -1;
	}
	else
		return 1;
}
//iulian send socket Message to OSGI management server
//Return : 0  suceed
//Return : -1 fail 
#define OSGI_MSG_LOCK "/var/osgi_app/osgi_msg_lock"
int sendMessageOSGIManagement(char* bundleName, const char* jsonMessageInChar, char *response)
{
	int clientSocket=-1, portNum, nBytes, ret=-1;
	char buffer[1024] = {0};
	char result[16];
	struct sockaddr_in serverAddr;
	socklen_t addr_size;
	int lockfd=0;
#ifdef CONFIG_CU
	fd_set myset; 
	int valopt;
	socklen_t lon;
#endif	

	if((lockfd = lock_file_by_flock(OSGI_MSG_LOCK, 1)) == -1)
	{
		printf("lock file %s fail\n", OSGI_MSG_LOCK);
		goto end;
	}

	struct timeval tv; /* timeval and timeout stuff */ 
	int timeouts = 0; 
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	//DEBUG
	//printf("=======[%s:%d]======== bundleName=%s jsonMessageInChar=%s\n",__FUNCTION__, __LINE__, bundleName, jsonMessageInChar);
	clientSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (clientSocket < 0) {
		perror("socket error");
		goto end;
	}
	if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof tv)){
		perror("setsockopt");
		goto end;
	}

#ifdef CONFIG_CU
	fcntl(clientSocket, F_SETFL, fcntl(clientSocket, F_GETFL)|O_NONBLOCK);
#endif
	
	portNum = 9080; //OSGI management server 
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(portNum);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);	

	addr_size = sizeof serverAddr;
	if (connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size)<0 ) {
#ifdef CONFIG_CU		
		if (errno == EINPROGRESS) 
		{ 
			tv.tv_sec = 0; 
			tv.tv_usec = 500000;		    
			FD_ZERO(&myset); 
			FD_SET(clientSocket, &myset); 
			if (select(clientSocket+1, NULL, &myset, NULL, &tv) > 0) { 
				lon = sizeof(int); 
				getsockopt(clientSocket, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon); 
				if (valopt) { 
					perror("connect");
					goto end;
				} 
			} 
			else { 
				perror("connect");
				goto end;
			} 
		} 
		else
#endif			
		{
			perror("connect");
			goto end;
		}
	}

#ifdef CONFIG_CU
	fcntl(clientSocket, F_SETFL, fcntl(clientSocket, F_GETFL)&~O_NONBLOCK);
#endif
	if(strcmp(bundleName,"Event")==0)
		sprintf(buffer, "Event:%s\n", jsonMessageInChar);
	else if(strcmp(bundleName,OSGI_FLUSH_TARGET)==0)
	sprintf(buffer, "BundleName:%s\nMessage:%s\n", bundleName, jsonMessageInChar);
	else
		sprintf(buffer, "BundleName:%s.SetMsgConfig\nMessage:%s\n", bundleName, jsonMessageInChar);
	//printf("send content:\"%s\"",buffer);
	nBytes = strlen(buffer) + 1;

	if (send(clientSocket,buffer,nBytes,0) <0 ){
		perror("send");
		goto end;
	}
	memset(buffer, 0, sizeof(buffer));
	while (((nBytes=recv(clientSocket, buffer, 1024, 0)) == -1) && (timeouts < 3)) 
	{ 
		if(errno == EINTR)
			continue;
		/* loop to retry in case it timed out */ 
		timeouts++;
		perror("recv"); 
		printf("[%s:%d] After timeout #%d, trying again:\n", __FUNCTION__, __LINE__, timeouts);
	} 
	//printf("numbytes = %d\n", nBytes);
	if (nBytes < 0) {
		goto end;
	}
	//printf("Received from server:%s@@@\n",buffer);
	if(response)
		sscanf(buffer, "%[^:]:%d:%s\n", &result,&ret, response);
	else
		sscanf(buffer, "%[^:]:%d\n", &result, &ret);
	//printf("parse result:%s,ret:%d\n",result, ret);
	if(response&&strlen(response))
		printf("revc reponse massage:buff=%s\n",response);
	if ( strcmp(result, "result") != 0) {	
		ret = -1;
	}
end:		
	if(clientSocket >= 0)
		close(clientSocket);
	unlock_file_by_flock(lockfd);
	return ret;

}
#endif
int getWebLoidPageEnable(void)
{
	char enable;
	mib_get_s(MIB_WEB_LOID_PAGE_ENABLE,(void *)&enable, sizeof(enable));
	if(enable==1){
		return 1;
	}
	else{
		return 0;
	}
}

int isLoidNull()
{
	char loid[30] = {0}, password[32] = {0};
	mib_get(MIB_LOID, loid);
	
	if (loid[0]){
		return 0;
	}else{
		return 1;
	}
}

#if defined(CONFIG_APACHE_FELIX_FRAMEWORK)
static int fillOsgiPluginInfo(char* path, OSGI_BUNDLE_INFO_Tp info, int index)
{
	FILE* fp=NULL;
	char line[256];
	char *pline;
	int linenum=0;
	int len;
	if ((fp = fopen(path, "r")) == NULL){
		printf("%s:%d file %s is not exist\n", __FUNCTION__, __LINE__, path);
		return -1;
	}
	while(fgets(line, sizeof(line), fp) != NULL){
		pline = line;
		linenum++;
		switch(linenum){
			case 2://DUID=Bundle0
				pline += 11;
				info[index].bundle_id = atoi(pline);
				//printf("info->bundle_id %d\n", info[index].bundle_id);
				break;
			case 4://Name=org.apache.felix.framework
				pline += 5;
				strcpy(info[index].symbolic_name, pline);
				info[index].symbolic_name[strlen(info[index].symbolic_name)-1] = 0;
				//printf("info->symbolic_name %s\n", info[index].symbolic_name);
				break;
			case 5://BundleName=System Bundle
				pline += 11;
				strcpy(info[index].plugin_name, pline);
				info[index].plugin_name[strlen(info[index].plugin_name)-1] = 0;
				//printf("info->plugin_name %s\n", info[index].plugin_name);
				break;
			case 7://EUStatus=Active
				pline += 9;
				strcpy(info[index].status, pline);
				info[index].status[strlen(info[index].status)-1] = 0;
				//printf("info->status %s\n", info[index].status);
				break;
			case 8://Version=5.6.1
				pline += 8;
				strcpy(info[index].version, pline);
				info[index].version[strlen(info[index].version)-1] = 0;
				//printf("info->version %s\n", info[index].version);
				break;
			case 9://URL=System Bundle
				pline += 4;
				strcpy(info[index].path, pline);
				info[index].path[strlen(info[index].path)-1] = 0;
				//printf("info->path %s\n", info[index].path);
				break;
			case 10://Vendor=cmcc
				pline += 7;
				strcpy(info[index].vendor, pline);
				info[index].vendor[strlen(info[index].vendor)-1] = 0;
				//printf("info->vendor %s\n", info[index].vendor);
				break;
			case 11://Level=0
				pline += 6;
				info[index].level = atoi(pline);
				//printf("info->level %d\n", info[index].level);
				break;
		}
		//memset(line, 0, sizeof(line));
	}
	fclose(fp);
	return 0;
}

#define osgiFileLock	"/var/osgi_Lock"
#define OSGI_LOCK_RETRY_CNT	(3)

int osgi_notify_lock(int *lockfd)
{
	int retLock =-1;
	int lockRetry = 0;
	if(lockfd == NULL){
		return -1;
	}
	if ((*lockfd = open(osgiFileLock, O_RDWR|O_CREAT, 0666)) == -1) {
		perror("open usbinout_notify lockfile");
		retLock = -1;				
		return retLock; 
	}
	while (retLock == -1 && lockRetry < OSGI_LOCK_RETRY_CNT) { 
		retLock = flock(*lockfd, LOCK_EX|LOCK_NB);
		if (errno == EINTR || errno == EAGAIN){
			continue;
		}
		else if(retLock == 0){
			break;
		}
		else{
			lockRetry++;
			sleep(1);
		}
	}	
	return retLock;	
}

int osgi_notify_unlock(int lockfd)
{
	flock(lockfd, LOCK_UN);
	close(lockfd);
	return 0;
}

//the function will alloc memory, the caller must free the memory
int listAllOsgiPlugin(OSGI_BUNDLE_INFO_Tp *list, int *num)
{  
	FILE* fp=NULL;
	char cmd[256]={0};
	char tmp[256]={0};
	char path[256]={0};
	int numOfPlugin = 0;
	OSGI_BUNDLE_INFO_Tp info=NULL;
	int i = 0;
	int lock = -1;
	
	if(osgi_notify_lock(&lock) == -1)
	{
		printf("[%s(%d)] GET OSGI LOCK failed!", __FUNCTION__, __LINE__);
		return -1;
	}
	
	DIR* dir = opendir(OSGI_PULGIN_STATUS_DIR);
	if (dir) {
		/* Directory exists. */
		closedir(dir);
	} 
	else {
		/* opendir() failed for some other reason. */
		osgi_notify_unlock(lock);
		return -1;
	}

	sprintf(cmd, "ls -l %s/ | wc -l > /tmp/plugnum", OSGI_PULGIN_STATUS_DIR);
	system(cmd);
	if((fp=fopen("/tmp/plugnum", "r"))!=NULL)
	{
		if(fgets(tmp, sizeof(tmp), fp) != NULL){
			numOfPlugin = atoi(tmp);
		}
		fclose(fp);
		fp = NULL;
	}
	else{
		osgi_notify_unlock(lock);
		return -1;
	}
	*num = numOfPlugin;
	*list = NULL;
	info = (OSGI_BUNDLE_INFO_Tp)malloc(sizeof(OSGI_BUNDLE_INFO_T)*(numOfPlugin));
	if(!info){
		printf("%s:%d malloc fail\n", __FUNCTION__, __LINE__); 
		*list = NULL;
		*num = 0;
		osgi_notify_unlock(lock);
		return -1;
	}
	memset(info, 0, sizeof(OSGI_BUNDLE_INFO_T)*(numOfPlugin));
	*list = info;

	sprintf(cmd, "ls -1 %s/ > /tmp/all_status_osgi_list", OSGI_PULGIN_STATUS_DIR);
	system(cmd);
	if((fp=fopen("/tmp/all_status_osgi_list", "r"))!=NULL)
	{
		while(fgets(tmp, sizeof(tmp), fp) != NULL){
			tmp[strlen(tmp)-1] = 0;
			sprintf(path, "%s/%s", OSGI_PULGIN_STATUS_DIR, tmp);
			//printf("%s:%d \tfile %s\n", __FUNCTION__, __LINE__, path);
			if(fillOsgiPluginInfo(path, info, i) < 0){
				fclose(fp);
				fp = NULL;
				free(info);
				*list = NULL;
				*num = 0;
				osgi_notify_unlock(lock);
				return -1;
			}
			i++;
			if(i>=numOfPlugin){
				break;
			}
		}
		fclose(fp);
		fp = NULL;
	}
	else{
		free(info);
		*list = NULL;
		*num = 0;
		osgi_notify_unlock(lock);
		return -1;
	}
	
	osgi_notify_unlock(lock);
	return 0;
}

int rtk_osgi_restart_osgi_permission_bundle()
{
	system("sysconf send_unix_sock_message /var/run/systemd.usock do_restart_osgi_permission");
	
	return 0;
}

#ifdef CONFIG_USER_CUMANAGEDEAMON
char *str_Only_Set_APIList[] =
{
	"accessservices.WanConfigService",
	"accessservices.IPPingDiagnosticsService",
	"accessservices.TraceRouteDiagnosticsService",
	"accessservices.HttpDownloadDiagnosticsService",
	"accessservices.HttpUploadDiagnosticsService",
	"addressservices.LANIPConfigService",
	"addressservices.PortMappingConfigService",
	"commservices.UsbService",
	"commservices.DeviceAccessRightConfigService",
	"commservices.DeviceRsetService",
	"commservices.DeviceConfigService",
	"lanservices.LanNetworkNameConfigService",
	"lanservices.LanHostSpeedLimitService",
	"lanservices.LanNetworkAccessConfigService",
	"lanservices.WlanConfigService",
	"lanservices.EthConfigService",
	"transferservices.StaticLayer3ForwardingConfigService",
	"transferservices.L2TPVPNService",
	"voipservices.VoIPInfoSubscribeService",
	"transferservices.TrafficMonitoringConfigService",
	"transferservices.TrafficQoSService",
	"transferservices.TrafficForwardService",
	"transferservices.TrafficMirrorService",
	"transferservices.TrafficDetailProcessService",
	"transferservices.FastPathSpeedUpService"
};	

int rtk_osgi_is_only_support_setapi(char *APIName)
{
	int i, len, ret=0;

	len = sizeof(str_Only_Set_APIList)/sizeof(char *);

	for(i=0; i<len; i++)
	{
		if(strstr(APIName, str_Only_Set_APIList[i]))
		{
			ret=1;
			break;
		}
	}

	return ret;
}
#endif

static const char *default_osgi_permissions =
#ifdef CONFIG_USER_CUMANAGEDEAMON
	"DENY {\n"
	"   ( org.osgi.framework.ServicePermission \"com.chinaunicom.smartgateway.deviceservices.accessservices.*\" \"get\")\n"
	"   ( org.osgi.framework.ServicePermission \"com.chinaunicom.smartgateway.deviceservices.addressservices.*\" \"get\")\n"
	"   ( org.osgi.framework.ServicePermission \"com.chinaunicom.smartgateway.deviceservices.commservices.*\" \"get\")\n"
	"   ( org.osgi.framework.ServicePermission \"com.chinaunicom.smartgateway.deviceservices.lanservices.*\" \"get\")\n"
	"   ( org.osgi.framework.ServicePermission \"com.chinaunicom.smartgateway.deviceservices.transferservices.*\" \"get\")\n"
	"   ( org.osgi.framework.ServicePermission \"com.chinaunicom.smartgateway.deviceservices.voipservices.*\" \"get\")\n"
	"} \"Default DENY all chinaunicom API service\"\n"
#else
	"DENY {\n"
	"   ( org.osgi.framework.ServicePermission \"com.chinamobile.smartgateway.accessservices.*\" \"get\")\n"
	"   ( org.osgi.framework.ServicePermission \"com.chinamobile.smartgateway.addressservices.*\" \"get\")\n"
	"   ( org.osgi.framework.ServicePermission \"com.chinamobile.smartgateway.commservices.*\" \"get\")\n"
	"   ( org.osgi.framework.ServicePermission \"com.chinamobile.smartgateway.lanservices.*\" \"get\")\n"
	"   ( org.osgi.framework.ServicePermission \"com.chinamobile.smartgateway.transferservices.*\" \"get\")\n"
	"   ( org.osgi.framework.ServicePermission \"com.chinamobile.smartgateway.voipservices.*\" \"get\")\n"
	"} \"Default DENY all chinamobile API service\"\n"
#endif
	"DENY {\n"
	"	[com.realtek.osgi.security.SymbolicNameCondition \"com.chinamobile.smartgateway.jvmfunctiontest\"]\n"
	"   (java.io.FilePermission \"*\" \"execute\")\n"
	"   (java.lang.RuntimePermission \"*\")\n"
	"   (java.security.SecurityPermission \"*\")\n"
	"   (org.osgi.framework.AdminPermission \"*\" \"*\")\n"
	"   (org.osgi.framework.ServicePermission \"org.osgi.service.condpermadmin.ConditionalPermissionAdmin\" \"get\")\n"
	"   (org.osgi.framework.ServicePermission \"org.osgi.service.permissionadmin.PermissionAdmin\" \"*\")\n"
	"} \"deny third party bundle lifecycle, conditional permission\"\n"
	"ALLOW {\n"
	"   (java.security.AllPermission \"\" \"\")\n"
	"} \"allow all permission\"\n";

#define OSGI_POLICY_FILE "/var/osgi_app/security.policy"
#define OSGI_SYMBOLICNAME_CONDITION_CLASS "com.realtek.osgi.security.SymbolicNameCondition"
#define OSGI_PERMISSION_LOCKFILE	"/var/run/osgi_permission_lockfile.lock."

int rtk_osgi_get_osgi_bundle_id(char *name)
{
	MIB_CMCC_OSGI_PLUGIN_T Entry;
	memset(&Entry,0,sizeof(MIB_CMCC_OSGI_PLUGIN_T));
	int i, mibtotal;

	if(name == NULL) return 0;

	mibtotal = mib_chain_total(MIB_CMCC_OSGI_PLUGIN_TBL);
	for(i=0;i<mibtotal;i++){
		if(!mib_chain_get(MIB_CMCC_OSGI_PLUGIN_TBL, i, (void *)&Entry))
			continue;
		if(!strcmp(name,Entry.symbolic_name))
		{
			return Entry.bundle_id;
		}
	}

	return 0;
}

void rtk_osgi_start_bundle(int id)
{
	char id_str[8];
	sprintf(id_str,"%d",id);
	printf("%s:%d bundle id %d\n", __FUNCTION__, __LINE__, id);
	va_cmd("/bin/sh", 4, 1, OSGI_PERMISSION_PLUGIN_RUN, id_str, hideErrMsg1, hideErrMsg2);
}

int rtk_osgi_setup_osgi_permissions(char *new_plugin)
{
#ifdef CONFIG_USER_CUMANAGEDEAMON
	FILE *fp = NULL;
	int i, api_total, j, plugin_total;
	MIB_CMCC_OSGI_PLUGIN_T Entry;
	MIB_PLUGIN_API_LIST_T APIEntry;
	char symblic_name[128] = {0};
	
	int bundleId = rtk_osgi_get_osgi_bundle_id(new_plugin);

	plugin_total = mib_chain_total(MIB_CMCC_OSGI_PLUGIN_TBL);
	api_total = mib_chain_total(MIB_OSGI_PLUGIN_API_TBL);
	int osgi_permission_lockfd=lock_file_by_flock(OSGI_PERMISSION_LOCKFILE,1);

	fp = fopen(OSGI_POLICY_FILE, "w");
	if(fp == NULL)
		perror("Error: Failed to open "OSGI_POLICY_FILE);

	for(i = 0 ; i < plugin_total ; i++)
	{
		if(mib_chain_get(MIB_CMCC_OSGI_PLUGIN_TBL, i, &Entry) == 0)
			continue;

		fprintf(fp, "ALLOW {\n"
					"   ["OSGI_SYMBOLICNAME_CONDITION_CLASS" \"%s\"]\n", Entry.symbolic_name);

		for(j = 0 ; j < api_total ; j++)
		{
			if(mib_chain_get(MIB_OSGI_PLUGIN_API_TBL, j, &APIEntry) == 0)
				continue;

			if(strcmp(APIEntry.plugin_name, Entry.symbolic_name))
				continue;

			if((APIEntry.right == 0) && rtk_osgi_is_only_support_setapi(APIEntry.APIName))
				continue;

			fprintf(fp, "   (org.osgi.framework.ServicePermission \"%s\" \"get,reigster\")\n",
					APIEntry.APIName);
		}
		fprintf(fp, "} \"ALLOW rules for %s\"\n", Entry.symbolic_name);
	}

	fprintf(fp, "%s\n", default_osgi_permissions);
	fclose(fp);
#else
	FILE *fp = NULL;
	FILE *fp1 = NULL;
	MIB_CE_OSGI_PERMISSION_T perm_entry;
	MIB_CE_OSGI_PERMISSION_API_T api_entry;
	int perm_total = mib_chain_total(MIB_OSGI_PERMISSION_TBL);
	int api_total = mib_chain_total(MIB_OSGI_PERMISSION_API_TBL);
	int i, j;
	char symblic_name[128] = {0};
	char filePath[128] = {0};
	char *api = NULL;
	
	int bundleId = rtk_osgi_get_osgi_bundle_id(new_plugin);
	int osgi_permission_lockfd=lock_file_by_flock(OSGI_PERMISSION_LOCKFILE,1);
	printf("rtk_osgi_setup_osgi_permissions clear plugin permission files\n");
	system("rm -rf /var/osgi_app/security/*.policy");

	fp = fopen(OSGI_POLICY_FILE, "w");
	if(fp != NULL){
		for(i = 0 ; i < perm_total ; i++)
		{
			if(mib_chain_get(MIB_OSGI_PERMISSION_TBL, i, &perm_entry) == 0)
				continue;

			if(perm_entry.DUName[0] == '\0' || perm_entry.instNum == 0)
				continue;

			strcpy(symblic_name, perm_entry.DUName);
			/**
			 * Output rules like:
			ALLOW {
			   [org.osgi.service.condpermadmin.BundleLocationCondition "file:/var/osgi_app/*"]
			   (org.osgi.framework.ServicePermission "com.chinamobile.smartgateway.lanservices.LanHostsInfoQueryService" "get,register")
			   (org.osgi.framework.ServicePermission "com.chinamobile.smartgateway.lanservices.WlanQueryServce" "get,register")
			   (org.osgi.framework.ServicePermission "com.chinamobile.smartgateway.lanservices.LanNetworkAccessConfigService" "get,register")
			} "deny third party bundle lifecycle, conditional permission"
			*/
			sprintf(filePath, "/var/osgi_app/security/%s.policy", symblic_name);
			fp1 = fopen(filePath, "w");
			if(fp1 != NULL){
				//fprintf(fp, "ALLOW {\n"
				//			"   ["OSGI_SYMBOLICNAME_CONDITION_CLASS" \"%s\"]\n", symblic_name);

				fprintf(fp1, "SymbolicName: %s\n", symblic_name);
				fprintf(fp1, "Authority: \n");

				for(j = 0 ; j < api_total ; j++)
				{
					if(mib_chain_get(MIB_OSGI_PERMISSION_API_TBL, j, &api_entry) == 0)
						continue;

					if(perm_entry.instNum != api_entry.DUInstNum || api_entry.APIName[0] == '\0')
						continue;

					//fprintf(fp, "   (org.osgi.framework.ServicePermission \"%s\" \"get,reigster\")\n",
					//		api_entry.APIName);

					/*remove com.chinamobile.smartgateway.*/
					api = &api_entry.APIName[0];
					fprintf(fp1, "\t(%s) \n", api+29);
				}
				//fprintf(fp, "} \"ALLOW rules for %s\"\n", perm_entry.DUName);
				fclose(fp1);
				fp1=NULL;
			}else {
				fprintf(stderr,"Error: Failed to open %s\n",filePath);
			}
		}

		fprintf(fp, "%s\n", default_osgi_permissions);
		fclose(fp);
	}else {
		fprintf(stderr,"Error: Failed to open %s\n",OSGI_POLICY_FILE);
	}
#endif	
	unlock_file_by_flock(osgi_permission_lockfd);

	rtk_osgi_restart_osgi_permission_bundle();

	//wait for loading permission, then start bundle again
	if(bundleId > 0){
		rtk_osgi_start_bundle(bundleId);
	}

	return 0;
}

unsigned int rtk_osgi_get_permission_empty_inst(void)
{
	MIB_CE_OSGI_PERMISSION_T entry;
	int osgi_permission_lockfd=lock_file_by_flock(OSGI_PERMISSION_LOCKFILE,1);
	int totalEntry=mib_chain_total(MIB_OSGI_PERMISSION_TBL);
	int i=0, j=1;

	for(i=0; i < totalEntry; i++){
		if (!mib_chain_get(MIB_OSGI_PERMISSION_TBL, i, (void *)&entry)) {
			printf("%s:%d fail\n", __FUNCTION__, __LINE__);
		}

		if(entry.instNum == j){
			j++;
		}
		else{
			unlock_file_by_flock(osgi_permission_lockfd);
			return j;
		}
	}
	unlock_file_by_flock(osgi_permission_lockfd);
	return j;
}

int rtk_osgi_get_plugin_by_bundleid(unsigned int bundleId, MIB_CMCC_OSGI_PLUGIN_Tp pentry)
{
	MIB_CMCC_OSGI_PLUGIN_T Entry;
	int entryNum=mib_chain_total(MIB_CMCC_OSGI_PLUGIN_TBL);
	int i =0;

	for(i=0; i < entryNum; i++){
		if(!mib_chain_get(MIB_CMCC_OSGI_PLUGIN_TBL, i, (void *)&Entry))
			continue;
		if(Entry.bundle_id == bundleId){
			memcpy(pentry, &Entry, sizeof(MIB_CMCC_OSGI_PLUGIN_T));
			return i; //find
		}
	}

	return -1;//not find
}

int rtk_osgi_get_bundle_resolved(int id)
{
	char line[256];
	char *pline;
	int linenum=0;
	int len;
	char cacheFileName[256]={0};
	char state[20] = {0};
	int resolved = 0;
	sprintf(cacheFileName,"/tmp/cwmp_osgi/%d_status.txt", id);
	FILE *fd = fopen(cacheFileName, "r");
	//printf("%s:%d cacheFileName %s\n", __FUNCTION__, __LINE__, cacheFileName);
	if(fd==NULL)
		return -1;
	while(fgets(line, sizeof(line), fd) != NULL){
		pline = line;
		linenum++;
		if(linenum == CWMP_OSGI_CACHE_LINE_STATE){
			printf("read bundle %d state is %s\n",id, line);
			/*DUStatus=Installed\n*/
			pline += 9;
			strcpy(state , pline);
			len = strlen(state);
			if(state[len-1] == '\n')
				state[len-1] = '\0';

			if(strcmp(state, "Installed")==0){
				resolved = 1;
			}
			printf("read bundle %d state is %s, resolved is %d\n",id, state, resolved);
		}
	}
	fclose(fd);
	return resolved;
}

int rtk_osgi_is_bundle_action_timeout(int *bundle_id, char *file_name)
{
	time_t begin;
	begin = time(NULL);
	int retry=0;
	while(time(NULL)-begin < BUNDLE_PLUGIN_ACTION_TIMEOUT)
	{
		*bundle_id = rtk_osgi_get_bundle_Id_by_filename(file_name);
		if(*bundle_id){
			if(rtk_osgi_get_bundle_resolved(*bundle_id)==1){
				return 0;
			}
		}

		sleep(1);
		retry++;
	}
	printf("stop or run bundle timeout\n");
	return 1;
}

int rtk_osgi_get_bundle_Id_by_filename(char * file_name)
{
	char cmd[256];
	char filepath[256];
	char *id_str,*end;

	memset(filepath, 0, sizeof(filepath));
	//memset(id_str, 0, sizeof(id_str));
	sprintf(cmd, "grep -rl '%s' "OSGI_PULGIN_STATUS_DIR, file_name);
	printf("cmd: %s\n",cmd);
	FILE *fd = popen(cmd, "r");
	while((fgets(filepath, 256, fd)) != NULL) {
		printf("%s:%d filepath %s\n",__FUNCTION__, __LINE__,filepath);
	}
	pclose(fd);

	if(filepath[0] !=0){
		id_str = filepath+strlen(OSGI_PULGIN_STATUS_DIR)+1;//ignore "/tmp/status_osgi/" 
		end = id_str;
		while(*end <= '9' && *end >= '0') end++;
		*end = 0;
		if(end-id_str > 0)
			return atoi(id_str);
	}

	return -1;
}

int rtk_osgi_check_bundle_file()
{
	MIB_CMCC_OSGI_PLUGIN_T Entry;
	int i, mibtotal;

	mibtotal = mib_chain_total(MIB_CMCC_OSGI_PLUGIN_TBL);
	//check the bundle file exist or not
	for(i=mibtotal-1;i>=0;i--){
		if(!mib_chain_get(MIB_CMCC_OSGI_PLUGIN_TBL, i, (void *)&Entry))
			continue;
		
		if (access(Entry.path, F_OK) == -1){
			mib_chain_delete(MIB_CMCC_OSGI_PLUGIN_TBL, i);
		}
	}

	return 0;
}

/*get symbolic name from jar file*/
void rtk_osgi_get_bundlefile_symbolicname(char* path, char* symbolicName)
{
	char cmd[1024];
	FILE *pp = NULL;

	if(path == NULL || symbolicName == NULL) return;
	
	sprintf(cmd, "7za e -so %s META-INF/MANIFEST.MF | grep Bundle-SymbolicName | awk '{printf $2}' > /tmp/uploadbundle", path);
	system(cmd);
	
	pp= fopen("/tmp/uploadbundle", "r");
	while((fgets(symbolicName, 256, pp)) != NULL) {
		printf("%s:%d symbolicName %s\n",__FUNCTION__, __LINE__,symbolicName);
		//symbolicName[strlen(symbolicName)-1] = 0;
	}
	fclose(pp);
}

void rtk_osgi_bundle_log(char* msg)
{
#if defined(CONFIG_CMCC_ENTERPRISE)
	char ts[32]={0};
	time_t tm;
	char msg_target[1024]={0};
	FILE *fp = NULL;
	
	tm = time(NULL);
	strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&tm));

	snprintf(msg_target, sizeof(msg_target), "%s %s\n",ts, msg);

	if ((fp = fopen("/var/run/bundleLog.txt", "a")) == NULL){
		printf("%s:%d Open file fail\n", __FUNCTION__, __LINE__);
		return;
	}

	fprintf(fp, "%s", msg_target);

	fclose(fp);
#endif	
}

#endif //defined(CONFIG_APACHE_FELIX_FRAMEWORK)

int ssid_index_to_wlan_index(int ssid_index)
{
	int wlanIndex=0;
	
	if(ssid_index<=4) 
	{
#if defined(WLAN0_5G_WLAN1_2G)
		wlanIndex = 1;
#elif defined(WLAN0_2G_WLAN1_5G)
		wlanIndex = 0;
#else
		wlanIndex = 0;
#endif
	}
#ifdef WLAN_DUALBAND_CONCURRENT
	else 
	{
#if defined(WLAN0_5G_WLAN1_2G)
		wlanIndex = 0;
#elif defined(WLAN0_2G_WLAN1_5G)
		wlanIndex = 1;
#else	
		wlanIndex = 1;
#endif
	}
#endif
	return wlanIndex;
}

/*
return value:
1: address is ip/mask pattern.
2: address is domain name.
*/
int rtk_osgi_traffic_parse_address(const char *address, char*ip, unsigned int *maskbit)
{
	char string_ip[INET6_ADDRSTRLEN] = {0};
    char string_mask[18] = {0};
    int find = 0;
    int i = 0, ret = 0;
    int len = strlen(address);

    if(0 == len)
    {
		strcpy(ip, "");
        *maskbit = 0;
        return 0;
    }

	for(i = 0; i < len; i++)
    {
        if(address[i] != '/')
            continue;

		find = 1;
        memcpy(string_ip, address, i);
		memcpy(ip, address, i);
        memcpy(string_mask, address+i+1, len-i-1);

        fprintf(stderr,"%s:%d, ip %s maskbit %s\n", __FUNCTION__, __LINE__, string_ip, string_mask);
    }

    if(0 == find)
    {
        if((NULL == strstr(address, ".")) && (NULL == strstr(address, ":")))
        {
    		fprintf(stderr,"%s:%d, address %s format error\n", __FUNCTION__, __LINE__, address);
			return -1;
        }
        else
        {
            return 2;
        }
    }

    *maskbit = atoi(string_mask);

    return 1;
}

void rtk_osgi_init_traffic_qos_rule()
{
	int num = TRAFFIC_QOS_RULE_TBL_FLOW_NUM;
	MIB_TRAFFIC_QOS_RULE_FLOW_T flow;
	int i = 0;
	
	mib_chain_clear(MIB_TRAFFIC_QOS_RULE_FLOW_TBL);
	flow.downmaxrate = 0;
	flow.upmaxrate = 0;
	flow.dscp = -1;
	for(i=0; i < num; i++){
		flow.flow = i;
		mib_chain_add(MIB_TRAFFIC_QOS_RULE_FLOW_TBL, &flow);
	}
}

#define PON_FLOW_STATISTICS_LOCKFILE	"/var/run/pon_flow_statistic.lock."

int lock_pon_statistic_file_by_flock()
{
	int lockfd;

	if ((lockfd = open(PON_FLOW_STATISTICS_LOCKFILE, O_RDWR|O_CREAT)) == -1) {
		perror("open shm lockfile");
		return lockfd;
	}
	while (flock(lockfd, LOCK_EX) == -1 && errno==EINTR) {
		printf("configd write lock failed by flock. errno=%d\n", errno);
	}
	return lockfd;
}

int unlock_pon_statistic_file_by_flock(int lockfd)
{
	while (flock(lockfd, LOCK_UN) == -1 && errno==EINTR) {
		printf("configd write unlock failed by flock. errno=%d\n", errno);
	}
	close(lockfd);
	return 0;
}

#endif //defined(CONFIG_CMCC) || defined(CONFIG_CU)

#if defined(CONFIG_CU_BASEON_YUEME)
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
int rtk_dbus_init_traffic_monitor_rule(void)
{
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", TABLE_TRAFFIC_MONITOR_FWD);
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-j", TABLE_TRAFFIC_MONITOR_FWD);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", TABLE_TRAFFIC_MONITOR_FWD);
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-j", TABLE_TRAFFIC_MONITOR_FWD);
#endif
	return 0;
}

int rtk_dbus_setup_traffic_monitor_rule(trafficmonitor_rule_Info *entry, int type)
{
#ifdef CONFIG_RTK_SKB_MARK2
	unsigned char mark0[64], mark[64];
	char cmd[512] = {0};

	if (!strcmp(entry->real_monitor_ip, "") || entry->real_monitor_ip == NULL ){ //query url address , update later
		return 0;
	}

	sprintf(mark0, "0x0/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);
	sprintf(mark, "0x%llx/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK, SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);

	DOCMDINIT;
	
	if(entry->isIpv4)
	{
		sprintf(cmd, "-t mangle %s %s -d %s -j MARK2 --set-mark2 %s", 
			(type == 1)?"-A":"-D",
			(char *)TABLE_TRAFFIC_MONITOR_FWD, 
			entry->real_monitor_ip, 
			mark);
		DOCMDARGVS(IPTABLES, DOWAIT, cmd);
	}
#ifdef CONFIG_IPV6	
	else
	{
		sprintf(cmd, "-t mangle %s %s -d %s -j MARK2 --set-mark2 %s", 
			(type == 1)?"-A":"-D",
			(char *)TABLE_TRAFFIC_MONITOR_FWD, 
			entry->real_monitor_ip, 
			mark);
		DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
	}
#endif //CONFIG_IPV6	

	return 1;
#endif //CONFIG_RTK_SKB_MARK2	
}

int rtk_dbus_add_traffic_monitor_rule(trafficmonitor_rule_Info *entry)
{
	return(rtk_dbus_setup_traffic_monitor_rule(entry, 1));
}

int rtk_dbus_del_traffic_monitor_rule(trafficmonitor_rule_Info *entry)
{
	return(rtk_dbus_setup_traffic_monitor_rule(entry, 0));
}

#endif
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
int sendDnsMsg(unsigned long msgType, unsigned int msgSubType, void* data, int len)
{
	int msgid=0;
	struct data_msg_st data_msg;
	key_t mqkey;

	mqkey = ftok(MSG_DNS_QUEUE_FILE, MSG_DNS_QUEUE_ID);
	if(mqkey == -1) 
	{
		perror("ftok error: ");
		return -1;
	}
	msgid = msgget(mqkey, 0666);

	if (msgid == -1) 
	{
		printf("msgid fail\n");
		return -1;
	}
	memset(&data_msg, 0, sizeof(data_msg));
	data_msg.msg_type = msgType;
	data_msg.msg_subtype = msgSubType;
	if(data != NULL){
		if(len < 256){
			memcpy(data_msg.mtext, data, len);
		}
		else{
			printf("data is null\n");
			return -1;
		}
	}
	else{
		if(msgSubType==MSG_SUBTYPE_ONCE){
			printf("msgSubType is MSG_SUBTYPE_ONCE\n");
			return -1;
		}
	}
	if (msgsnd(msgid, (void *)&data_msg, sizeof(struct data_msg_st)- sizeof(long), IPC_NOWAIT) == -1) 
	{
		fprintf(stderr, "msgsnd failed\n");
		return -1;
	}
	return 0;
}
#endif

#ifdef CONFIG_CMCC_TRAFFIC_PROCESS_RULE_SUPPORT
const char RG_ACL_CMCC_TRAFFIC_PROCESS_RULE_FILE[] = "/var/rg_acl_cmcc_traffic_process_rules_idx";
const char RG_ACL_CMCC_TRAFFIC_PROCESS_RULE_TMP_FILE[] = "/var/rg_acl_cmcc_traffic_process_rules_tmp_idx";
#define TRAFFIC_PROCESS_NETFILTER_HOOK_ADD	"/proc/osgi_traffic_process/traffice_process_rule"
enum {
    DIR_UP = 0,
    DIR_DOWN,
    DIR_UP_AND_DOWN
};

void _add_tf_rule_into_nfhook(MIB_CMCC_TRAFFIC_PROCESS_RULE_Tp rule)
{
    char cmd[512];
    int dir = 0;
    int ret = 0;
    if (strcmp(rule->direction, "UP") == 0) {
        dir = DIR_UP;
    } else if (strcmp(rule->direction, "DOWN") == 0) {
        dir = DIR_DOWN;
    } else {
        dir = DIR_UP_AND_DOWN;
    }

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	if((rule->containip&CONTAINS_IPV4) || (rule->containip == CONTAINS_IPNULL)){ //null ip add one time only
	    sprintf(cmd,
	            "/bin/echo  \"RemoteAddress:%s RemotePort:%s Direction:%d HostMAC:%s MethodList:%s statuscodeList:%s HeaderList:%s BundleName:%s Index:%d\" > %s",
	            rule->realremoteip4, rule->remotePort, dir, rule->hostMAC,
	            rule->methodList, rule->statuscodeList, rule->headerList,
	            rule->bundlename, rule->ruleIdx, TRAFFIC_PROCESS_NETFILTER_HOOK_ADD);
		printf("cmd is %s\n", cmd);
	    ret = system(cmd);
	}

	if(rule->containip&CONTAINS_IPV6){
	    sprintf(cmd,
	            "/bin/echo  \"RemoteAddress:%s RemotePort:%s Direction:%d HostMAC:%s MethodList:%s statuscodeList:%s HeaderList:%s BundleName:%s Index:%d\" > %s",
	            rule->realremoteip6, rule->remotePort, dir, rule->hostMAC,
	            rule->methodList, rule->statuscodeList, rule->headerList,
	            rule->bundlename, rule->ruleIdx, TRAFFIC_PROCESS_NETFILTER_HOOK_ADD);
		printf("cmd is %s\n", cmd);
	    ret = system(cmd);
	}	
#else
	sprintf(cmd,
            "/bin/echo  \"RemoteAddress:%s RemotePort:%s Direction:%d HostMAC:%s MethodList:%s statuscodeList:%s HeaderList:%s BundleName:%s Index:%d\" > %s",
            rule->realremoteAddress, rule->remotePort, dir, rule->hostMAC,
            rule->methodList, rule->statuscodeList, rule->headerList,
            rule->bundlename, rule->ruleIdx, TRAFFIC_PROCESS_NETFILTER_HOOK_ADD);
	printf("cmd is %s\n", cmd);
    ret = system(cmd);
#endif

    return;
}


void _del_tf_rule_from_nfhook(int index)
{
    char cmd[128];
    int ret = 0;
    sprintf(cmd, "/bin/echo  %d > %s", index,
            TRAFFIC_PROCESS_NETFILTER_HOOK_ADD);
	
    ret = system(cmd);
}
#endif

#ifdef CONFIG_CMCC_FORWARD_RULE_SUPPORT
const char CMCC_FORWARDRULE[]="cmcc_forwardrule";
const char CMCC_FORWARDRULE_POST[]="cmcc_forwardrule_post";
/*
remote: any, forward: any   ==> 0 (ipv4+ipv6)
remote: any, forward: ipv4   ==> 1 (ipv4)
remote: any, forward: ipv6   ==> 2 (ipv6)
remote: ipv4, forward: any   ==> 3   (ipv4)
remote: ipv4, forward: ipv4   ==> 4   (ipv4)
remote: ipv4, forward: ipv6   ==> 5   (error)
remote: ipv6, forward: any   ==> 6   (ipv6)
remote: ipv6, forward: ipv4   ==> 7   (error)
remote: ipv6, forward: ipv6   ==> 8   (ipv6)
*/
int checkIPv4OrIPv6(char *remote, char* forward)
{
	struct in6_addr address6;
	struct in_addr address;
	int iremote = 0, iforward = 0;  //0: any, 1:ipv4, 2:ipv6

	if(!strcmp(remote, "") || !strcmp(remote, "0")){
		iremote = 0;
	}
	else if(inet_pton(AF_INET, remote, &address) == 1){
		iremote = 1;
	}
	else if(inet_pton(AF_INET6, remote, &address6) == 1){
		iremote = 2;
	}
	else{
		iremote = 0;
	}
	
	if(!strcmp(forward, "") || !strcmp(forward, "0")){
		iforward = 0;
	}
	else if(inet_pton(AF_INET, forward, &address) == 1){
		iforward = 1;
	}
	else if(inet_pton(AF_INET6, forward, &address6) == 1){
		iforward = 2;
	}
	else{
		iforward = 0;
	}

	if(iremote == 0 && iforward == 0){
		return 0;
	}
	else if(iremote == 0 && iforward == 1){
		return 1;
	}
	else if(iremote == 0 && iforward == 2){
		return 2;
	}
	else if(iremote == 1 && iforward == 0){
		return 3;
	}
	else if(iremote == 1 && iforward == 1){
		return 4;
	}
	else if(iremote == 1 && iforward == 2){
		return 5;
	}
	else if(iremote == 2 && iforward == 0){
		return 6;
	}
	else if(iremote == 2 && iforward == 1){
		return 7;
	}
	else if(iremote == 2 && iforward == 2){
		return 8;
	}
}

void setCmccForwardRuleIptables(MIB_CMCC_FORWARD_RULE_T *Entry, int type)
{
	int remoteporttype = -1, startport = 0, endport = 0;
	char cmd[512] = {0};
	char cmd1[512] = {0};
	char cmd2[512] = {0};
	char tmp[128] = {0};
	char tmp1[128] = {0};
	char tmp2[128] = {0};
	int iptype = 0;
	int iprangetype=0;
	char ipaddr1[64]={0}, ipaddr2[64] ={0};
	
	DOCMDINIT;

	printf("%s:%d type %d\n", __FUNCTION__, __LINE__, type);
	printf("%s:%d remoteAddress %s, realremoteAddress %s, remotePort %s, protocol %d, hostMAC %s, forwardToIP %s, forwardToPort %d realforwardToIP %s\n", __FUNCTION__, __LINE__, 
		Entry->remoteAddress, 
		Entry->realremoteAddress, 
		Entry->remotePort,
		Entry->protocol,
		Entry->hostMAC,
		Entry->forwardToIP,
		Entry->forwardToPort,
		Entry->realforwardToIP);

	iptype = checkIPv4OrIPv6(Entry->realremoteAddress, Entry->forwardToIP);
	iprangetype = getIpRange(Entry->realremoteAddress, ipaddr1, ipaddr2);
	printf("%s:%d iptype %d iprangetype %d\n", __FUNCTION__, __LINE__, iptype, iprangetype);


//forwardToIP domain name redirect by boa
	if(strcmp(Entry->forwardToIP, "")&&(iptype == 0 ||iptype == 3 || iptype == 6)){
		strcpy(Entry->realforwardToIP, "192.168.1.1");
		Entry->forwardToPort=80;
	}
	else
		strcpy(Entry->realforwardToIP, Entry->forwardToIP);
	
	//protocol, remoteAddress
	memset(tmp, 0, 128);
	if(Entry->protocol == PROTO_TCP){
		sprintf(tmp, "%s %s -p TCP", CMCC_FORWARDRULE_ACTION_APPEND(type),CMCC_FORWARDRULE);
	}
	else{
		sprintf(tmp, "%s %s -p UDP", CMCC_FORWARDRULE_ACTION_APPEND(type),CMCC_FORWARDRULE);
	}
	strcat(cmd, tmp);

	//remoteAddress
	memset(tmp, 0, 128);
	if(!strcmp(Entry->remoteAddress, "") || !strcmp(Entry->remoteAddress, "0")){
		strcpy(tmp, "");
	}
	else{
		if(iprangetype==1)
			sprintf(tmp, " -d %s", Entry->realremoteAddress);
		else if(iprangetype==2)
			sprintf(tmp, " -m iprange --dst-range %s-%s", ipaddr1, ipaddr2);
	}
	strcat(cmd, tmp);

	//remote port
	memset(tmp, 0, 128);
	remoteporttype = parseRemotePort(Entry->remotePort, &startport, &endport);
	if(remoteporttype == 3){//remote port == 0, don't card
		strcpy(tmp, "");
	}
	else if(remoteporttype == 1){ //format !x
		sprintf(tmp, " ! --dport %d", startport);
	}
	else{
		strcpy(tmp1, Entry->remotePort);
		sprintf(tmp, " --dport %s", replaceChar(tmp1, '-', ':'));
	}
	strcat(cmd, tmp);

	//hostMAC
	#if 0
	memset(tmp, 0, 128);
	if(!strcmp(Entry->hostMAC, "") || !strcmp(Entry->hostMAC, "0")){
		strcpy(tmp, "");
	}
	else{
		sprintf(tmp, " -m mac --mac-source %s", Entry->hostMAC);
	}
	strcat(cmd, tmp);
	#endif
	
	//action
	memset(tmp, 0, 128);
	if(Entry->forwardToPort == 0){ //drop
		//action
		sprintf(tmp, " -j DROP");
		strcat(cmd, tmp);	
	}
	else {
		//action
		sprintf(tmp, " -j ACCEPT");
		strcat(cmd, tmp);
			
		if(!strcmp(Entry->realforwardToIP, "") || !strcmp(Entry->realforwardToIP, "0")){
			//do nothing, let packet handle by kernel stack
		}
		else{ //forward to destination IP/port
			//protocol, remoteAddress
			memset(tmp, 0, 128);
			if(Entry->protocol == PROTO_TCP){
				sprintf(tmp, "-t nat %s %s -p TCP", CMCC_FORWARDRULE_ACTION_APPEND(type),CMCC_FORWARDRULE);
				sprintf(tmp1, "-t nat %s %s -p TCP", CMCC_FORWARDRULE_ACTION_APPEND(type),CMCC_FORWARDRULE_POST);
			}
			else{
				sprintf(tmp, "-t nat %s %s -p UDP", CMCC_FORWARDRULE_ACTION_APPEND(type),CMCC_FORWARDRULE);
				sprintf(tmp1, "-t nat %s %s -p UDP", CMCC_FORWARDRULE_ACTION_APPEND(type),CMCC_FORWARDRULE_POST);
			}
			strcat(cmd1, tmp);
			strcat(cmd2, tmp1);

			//remoteAddress
			memset(tmp, 0, 128);
			if(!strcmp(Entry->remoteAddress, "") || !strcmp(Entry->remoteAddress, "0")){
				strcpy(tmp, "");
			}
			else{
				if(iprangetype==1)
					sprintf(tmp, " -d %s", Entry->realremoteAddress);
				else if(iprangetype==2)
					sprintf(tmp, " -m iprange --dst-range %s-%s", ipaddr1, ipaddr2);
			}
			strcat(cmd1, tmp);

			//remote port
			memset(tmp, 0, 128);
			remoteporttype = parseRemotePort(Entry->remotePort, &startport, &endport);
			if(remoteporttype == 3){//remote port == 0, don't card
				strcpy(tmp, "");
			}
			else if(remoteporttype == 1){ //format !x
				sprintf(tmp, " ! --dport %d", startport);
			}
			else{
				strcpy(tmp1, Entry->remotePort);
				sprintf(tmp, " --dport %s", replaceChar(tmp1, '-', ':'));
			}
			strcat(cmd1, tmp);

			//hostMAC
			memset(tmp, 0, 128);
			if(!strcmp(Entry->hostMAC, "") || !strcmp(Entry->hostMAC, "0")){
				strcpy(tmp, "");
			}
			else{
				sprintf(tmp, " -m mac --mac-source %s", Entry->hostMAC);
			}
			strcat(cmd1, tmp);

			//forwardToIP, forwardToPort
			memset(tmp, 0, 128);
			
			printf("%s:%d iptype %d\n", __FUNCTION__, __LINE__, iptype);
			if(iptype == 0 || iptype == 1 || iptype == 3 || iptype == 4){
				sprintf(tmp, " -j DNAT --to-destination %s:%d", Entry->realforwardToIP, Entry->forwardToPort);
				strcat(cmd1, tmp);
				printf("%s:%d cmd1 %s\n", __FUNCTION__, __LINE__, cmd1);
				DOCMDARGVS(IPTABLES, DOWAIT, cmd1);

				//iptables -t nat -A POSTROUTING -d 10.10.10.2 -j MASQUERADE
				if(remoteporttype == 3){//remote port == 0, don't card
					strcpy(tmp1, "");
				}
				else if(remoteporttype == 1){ //format !x
					sprintf(tmp1, " ! --dport %d", startport);
				}
				else{
					strcpy(tmp2, Entry->remotePort);
					sprintf(tmp1, " --dport %s", replaceChar(tmp2, '-', ':'));
				}
				sprintf(tmp, " -d %s %s -j MASQUERADE", Entry->realremoteAddress, tmp1);
				strcat(cmd2, tmp);
				
				printf("%s:%d cmd2 %s\n", __FUNCTION__, __LINE__, cmd2);
				DOCMDARGVS(IPTABLES, DOWAIT, cmd2);
			}
			if(iptype == 0 || iptype == 2 || iptype == 6 || iptype == 7 || iptype == 8){
				sprintf(tmp, " -j DNAT --to-destination [%s]:%d", Entry->realforwardToIP, Entry->forwardToPort);
				strcat(cmd1, tmp);
				printf("%s:%d cmd1 %s\n", __FUNCTION__, __LINE__, cmd1);
				DOCMDARGVS(IP6TABLES, DOWAIT, cmd1);

				//ip6tables -t nat -A POSTROUTING -d 2004::1 -j MASQUERADE
				if(remoteporttype == 3){//remote port == 0, don't card
					strcpy(tmp1, "");
				}
				else if(remoteporttype == 1){ //format !x
					sprintf(tmp1, " ! --dport %d", startport);
				}
				else{
					strcpy(tmp2, Entry->remotePort);
					sprintf(tmp1, " --dport %s", replaceChar(tmp2, '-', ':'));
				}
				sprintf(tmp, " -d %s %s -j MASQUERADE", Entry->realremoteAddress, tmp1);
				strcat(cmd2, tmp);
				
				printf("%s:%d cmd2 %s\n", __FUNCTION__, __LINE__, cmd2);
				DOCMDARGVS(IP6TABLES, DOWAIT, cmd2);
			}
		}
	}
	printf("%s:%d cmd %s\n", __FUNCTION__, __LINE__, cmd);
	printf("%s:%d iptype %d\n", __FUNCTION__, __LINE__, iptype);
	if(iptype == 0 || iptype == 1 || iptype == 3 || iptype == 4){
		DOCMDARGVS(IPTABLES, DOWAIT, cmd);
	}
	if(iptype == 0 || iptype == 2 || iptype == 6 || iptype == 7 || iptype == 8){
		DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
	}
}

int rtk_cmcc_update_cmcc_forward_rule_url(char*domainName, char* addr)
{
	int EntryNum = 0;
	int i = 0;
	MIB_CMCC_FORWARD_RULE_T entry;
	int iptype = 0;;
	int ret = 0;
	

	EntryNum = mib_chain_total(MIB_CMCC_FORDWARD_RULE_TBL);
	for(i=0; i < EntryNum; i++){
		if(!mib_chain_get(MIB_CMCC_FORDWARD_RULE_TBL, i, &entry))
			continue;
		if(!strcmp(domainName, entry.remoteAddress)){
			iptype = checkIPv4OrIPv6(addr, entry.forwardToIP);
			if(!(iptype == 5)){//error situation we skip, ipv4+ipv6 or ipv6+ipv4
				printf("%s:%d: domainName %s, addr %s", __FUNCTION__, __LINE__, domainName, addr);
				DelCmccForwardRule(&entry);
				sprintf(entry.realremoteAddress, "%s", addr);
				ret = AddCmccForwardRule(&entry, CMCC_FORWARDRULE_ADD);
				if(ret >= 0){//if set success, update mib
					mib_chain_update(MIB_CMCC_FORDWARD_RULE_TBL, &entry, i);
				}
			}
		}
	}
	return ret;
}

int AddCmccForwardRule(MIB_CMCC_FORWARD_RULE_T *Entry, int type)
{
	int ret = 0;

	printf("%s:%d\n", __FUNCTION__, __LINE__);
			 
	if(type == CMCC_FORWARDRULE_MOD){
		setCmccForwardRuleIptables(Entry, CMCC_FORWARDRULE_DELETE);
	}
	setCmccForwardRuleIptables(Entry, CMCC_FORWARDRULE_ADD);

	return ret;
}

int DelCmccForwardRule(MIB_CMCC_FORWARD_RULE_T *Entry)
{
	int ret = 0;

	printf("%s:%d\n", __FUNCTION__, __LINE__);
 
	setCmccForwardRuleIptables(Entry, CMCC_FORWARDRULE_DELETE);

	return ret;
}

int DelAllCmccForwardRule()
{
	int i, total;
	MIB_CMCC_FORWARD_RULE_T Entry;
	
	total = mib_chain_total(MIB_CMCC_FORDWARD_RULE_TBL);
	DOCMDINIT;

	//clear the IPTABLE_VTLSUR rules in the filter tables
	DOCMDARGVS(IPTABLES, DOWAIT, "-F %s", CMCC_FORWARDRULE);
	DOCMDARGVS(IP6TABLES, DOWAIT, "-F %s", CMCC_FORWARDRULE);

	//clear the IPTABLE_VTLSUR rules in the nat tables
	DOCMDARGVS(IPTABLES, DOWAIT, "-t nat -F %s", CMCC_FORWARDRULE);
	DOCMDARGVS(IPTABLES, DOWAIT, "-t nat -F %s", CMCC_FORWARDRULE_POST);
	DOCMDARGVS(IP6TABLES, DOWAIT, "-t nat -F %s", CMCC_FORWARDRULE);
	DOCMDARGVS(IP6TABLES, DOWAIT, "-t nat -F %s", CMCC_FORWARDRULE_POST); 

	return 0;
}

void rtk_cmcc_init_cmcc_forward_rule(void)
{
	//execute CMCCFORWARD rules
	va_cmd(IPTABLES, 2, 1, "-N", (char *)CMCC_FORWARDRULE);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)CMCC_FORWARDRULE);
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)CMCC_FORWARDRULE);
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)CMCC_FORWARDRULE);
	
	//execute virtual server NAT prerouting rules
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)CMCC_FORWARDRULE);
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)CMCC_FORWARDRULE);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)CMCC_FORWARDRULE_POST);
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", (char *)CMCC_FORWARDRULE_POST);
	va_cmd(IP6TABLES, 4, 1, "-t", "nat", "-N", (char *)CMCC_FORWARDRULE);
	va_cmd(IP6TABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)CMCC_FORWARDRULE);
	va_cmd(IP6TABLES, 4, 1, "-t", "nat", "-N", (char *)CMCC_FORWARDRULE_POST);
	va_cmd(IP6TABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", (char *)CMCC_FORWARDRULE_POST);
}

int setupCmccForwardRule(int type)
{

	int i, total;
	MIB_CMCC_FORWARD_RULE_T Entry;
	
	total = mib_chain_total(MIB_CMCC_FORDWARD_RULE_TBL);
	DOCMDINIT;

	//clear the IPTABLE_VTLSUR rules in the filter tables
	DOCMDARGVS(IPTABLES, DOWAIT, "-F %s", CMCC_FORWARDRULE);
	DOCMDARGVS(IP6TABLES, DOWAIT, "-F %s", CMCC_FORWARDRULE);

	//clear the IPTABLE_VTLSUR rules in the nat tables
	DOCMDARGVS(IPTABLES, DOWAIT, "-t nat -F %s", CMCC_FORWARDRULE);
	DOCMDARGVS(IPTABLES, DOWAIT, "-t nat -F %s", CMCC_FORWARDRULE_POST);
	DOCMDARGVS(IP6TABLES, DOWAIT, "-t nat -F %s", CMCC_FORWARDRULE);
	DOCMDARGVS(IP6TABLES, DOWAIT, "-t nat -F %s", CMCC_FORWARDRULE_POST); 

	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_CMCC_FORDWARD_RULE_TBL, i, (void *)&Entry))
			return -1;

		if(type == CMCC_FORWARDRULE_ADD){
			AddCmccForwardRule(&Entry, CMCC_FORWARDRULE_ADD);
		}
	}
	return 1;
}
#endif //#ifdef CONFIG_CMCC_FORWARD_RULE_SUPPORT

int checkIPv4_IPv6_Dual_PolicyRoute_ex(MIB_CE_ATM_VC_Tp pEntry, int *wanIndex, unsigned int *portMask)
{
	int vcTotal, i, ret;
	MIB_CE_ATM_VC_T Entry;
	MIB_CE_ATM_VC_T EntryV4, EntryV6;
	int isFoundv4 = 0, isFoundv6 = 0;

	if(pEntry->cmode != CHANNEL_MODE_BRIDGE && (pEntry->applicationtype&X_CT_SRV_INTERNET) && pEntry->IpProtocol == IPVER_IPV4){
		memcpy(&EntryV4, pEntry, sizeof(Entry));
		isFoundv4 = 1;
		vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < vcTotal; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
					return -1;

			if (Entry.enable == 0)
				continue;

			if(Entry.cmode != CHANNEL_MODE_BRIDGE && (Entry.applicationtype&X_CT_SRV_INTERNET) && Entry.IpProtocol == IPVER_IPV6) {
				memcpy(&EntryV6, &Entry, sizeof(Entry));
				isFoundv6 = 1;
			}
			if(isFoundv4==1 && isFoundv6==1){
				break;
			}
		}
	}
	else if(pEntry->cmode != CHANNEL_MODE_BRIDGE && (pEntry->applicationtype&X_CT_SRV_INTERNET) && pEntry->IpProtocol == IPVER_IPV6) {
		memcpy(&EntryV6, pEntry, sizeof(Entry));
		isFoundv6 = 1;
		vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < vcTotal; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
					return -1;

			if (Entry.enable == 0)
				continue;

			if(Entry.cmode != CHANNEL_MODE_BRIDGE && (Entry.applicationtype&X_CT_SRV_INTERNET) && Entry.IpProtocol == IPVER_IPV4) {
				memcpy(&EntryV4, &Entry, sizeof(Entry));
				isFoundv4 = 1;
			}

			if(isFoundv4==1 && isFoundv6==1){
				break;
			}
		}
	}
	

	if(isFoundv4==1 && isFoundv6==1 && EntryV4.itfGroup!=0 && EntryV4.itfGroup==EntryV6.itfGroup){
		*wanIndex = EntryV4.ifIndex;
		*portMask = EntryV4.itfGroup;
		return 1;
	}
	else{
		return 0;
	}
}

#if defined(CONFIG_CMCC_ENTERPRISE)
int	finishWebRedirect(void)
{
	int finish;
	mib_get(MIB_WEB_REDIRECT_BY_RESET, &finish);
	if(finish)//allready finishWebRedirect, do nothing
		return 1;
	finish = 1;
	mib_set(MIB_WEB_REDIRECT_BY_RESET, &finish);
	Commit();
	enable_http_redirect2register_page(0);

	restart_dnsrelay();

#if defined(CONFIG_APACHE_FELIX_FRAMEWORK) 
	//restart_java();
	sendMessageOSGIManagement("Event", "DNSFlush", NULL);
#endif
	return 0;
}
#endif

#if defined(CONFIG_CMCC_WIFI_PORTAL)
void WifidogMode(char* mode)
{	
	mib_get(PROVINCE_WIFIDOG_MODE, mode);
}
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
//#define WIFI_PORTAL_RUNING_FLAG "/tmp/wifi_portal_runing_flag"
void wifiAuthCheck(void * null)
{
	int ret=0,retDog=0,retAudit=0,retWan,restart=0;
	char cmdWanName[WA_MAX_WAN_NAME]={0};
	char curWanName[WA_MAX_WAN_NAME]={0};
	static char internetWanName[WA_MAX_WAN_NAME]={0};
	char wifidogenable=0;
	int retQzt=0;
	char mode[MAX_NAME_LEN]={0};
	FILE *fp;
	size_t len=0;
	ssize_t read;
	char *line=NULL;
	int line_cnt=0;

	wifidogenable = getIsAuth();

	fp=popen("ps","r");
	while((read = getline(&line,&len,fp))!=-1){
		line_cnt++;
		//printf("%s\n",line);
		if(!retDog && strstr(line, WIFIDOGPATH)){
			retDog=1;
		}
		if(retDog) break;
	}
	if(line)
		free(line);
	pclose(fp);
	if(line_cnt == 0){
		printf("%s:%d: nothing can be read from ps !\n",__func__,__LINE__);
		goto AUTH_END;
	}

	//printf("@@---%s#%d---retDog=%d--wifidogenable=%d--@@\n",__FUNCTION__,__LINE__,retDog,wifidogenable);
	if(!wifidogenable){
		if(retDog){
			killWiFiDog();
		}
		goto AUTH_END;
	}
	retWan=get_internet_wan_name(curWanName,sizeof(curWanName));
	if(!retDog){
		//wifidog hasn't started
		if(retWan){
			//printf("%s:%d:start wifi dog!\n",__func__,__LINE__);
			startWiFiDog(NULL);
			restart=1;
		}
		//no internet wan keep stop
	}
	else{
		//started
		if(retWan){
			if(strncmp(curWanName,internetWanName,WA_MAX_WAN_NAME)){
				//internet wan changed
				printf("%s:%d:restart wifi dog!\n",__func__,__LINE__);
				restartWiFiDog(0);
				restart=1;
			}
		}
		else{
			//no internet wan
			if(retDog){
				printf("%s:%d:kill wifi dog!\n",__func__,__LINE__);
				killWiFiDog();
			}
		}
	}

	if(retWan)
		strncpy(internetWanName,curWanName,WA_MAX_WAN_NAME);
	else
		internetWanName[0]=0;
	
	if(!restart && g_wan_modify && retWan){
		printf("%s:%d:start wifi auth!\n",__func__,__LINE__);
		restartWiFiDog(0);
		g_wan_modify=0;
	}

AUTH_END:
	TIMEOUT_F(wifiAuthCheck, 0, 5, wifiAuth_ch);
	WifidogMode(mode);
	if(strcmp(mode,"portal")==0)
		checkMacWhiteList();
}
struct callout_s wifidog_timeout;

int killWiFiDog(void)
{
	system(KILLWIFIDOGSTR);
#if defined(CONFIG_RTK_L34_ENABLE)
	RTK_RG_Wifidog_Rule_del();
#endif
	return 0;
}

void startWiFiDog(void *null)
{
	int ret;
	char cmdbuf[128];
	int trap_portal=0;
	char mode[MAX_NAME_LEN]={0};
	//ret=va_cmd( WIFIDOGPATH, 4, 0,"-c",WIFIDOGCONFPATH,"-f","-l");
	WifidogMode(mode);
	if(strcmp(mode, "portal")==0){
		trap_portal = 1;
		ret=va_cmd( WIFIDOGPATH, 5, 0,"-c",WIFIDOGCONFPATH,"-l", "-m", "portal");
	}
	else if(strcmp(mode, "default")==0){
		trap_portal = 1;
		ret=va_cmd( WIFIDOGPATH, 3, 0,"-c",WIFIDOGCONFPATH,"-l");
	}
	else if(strcmp(mode, "none")==0){
		//printf("wifidog mode none, do not start\n",cmdbuf);
#if defined(CONFIG_CMCC_AUDIT)
		int i,total;
		MIB_AUDIT_PLUGIN_T audit_entry;
		MIB_AWIFI_URLFILTER_T url_entry;
		int audit_on=0;
		total = mib_chain_total(MIB_AUDIT_PLUGIN_TBL);
		for(i=0;i<total;i++){
			if (!mib_chain_get(MIB_AUDIT_PLUGIN_TBL, i, &audit_entry)) {
				fprintf(stderr, "%s, get chain failed, entry index is %d\n", __FUNCTION__, i);
				continue;
			}
			if(strcmp(audit_entry.status, "RUN")==0){
				audit_on = 1;
				break;
			}
		}
		if(audit_on == 1){
			if(mib_chain_total(MIB_AWIFI_URLFILTER_TBL) > 0)
				trap_portal = 1;
		}
#endif
	}
	//snprintf(cmdbuf,sizeof(cmdbuf),"%s -c %s -f",WIFIDOGPATH,WIFIDOGCONFPATH);
	//printf("startWiFiDog:cmdbuf=%s\n",cmdbuf);
	//ret=system(cmdbuf);
	
	UNTIMEOUT_F(startWiFiDog, 0, wifidog_timeout);
}
int restartWiFiDog(int restart)
{
	char wifidogenable=0;
	
	wifidogenable = getIsAuth();

	killWiFiDog();
	if(wifidogenable)
	{
		if(restart)
		{
			UNTIMEOUT_F(startWiFiDog, 0, wifidog_timeout);
			TIMEOUT_F(startWiFiDog, 0, 2, wifidog_timeout);
		}else
		{
			startWiFiDog(NULL);
		}
	}
	return 0;
}

const char AWIFI_MAC_CONF[]="/var/awifi_mac.conf";

int setupTrustedMacConf()
{
	FILE *fp;

	if ((fp = fopen(AWIFI_MAC_CONF, "w")) == NULL) {
		printf("Open file %s failed !\n", AWIFI_MAC_CONF);
		return -1;
	}

#if 1//!defined(CONFIG_RTK_L34_ENABLE)
	MIB_AWIFI_MACFILTER_T entry;
	char macaddr[20];
	int cnt = 0;
	int index = 0;

	cnt=mib_chain_total(MIB_AWIFI_MACFILTER_TBL);

	if(cnt > 0)
	{
		fprintf(fp, "TrustedMACList ");
		for(index = 0; index < cnt; index++)
		{
			memset(&entry,0,sizeof(MIB_AWIFI_MACFILTER_T));

			mib_chain_get(MIB_AWIFI_MACFILTER_TBL,index,&entry);

			snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
					entry.mac[0], entry.mac[1],
					entry.mac[2], entry.mac[3],
					entry.mac[4], entry.mac[5]);
			
			fprintf(fp, "%s%s",index?",":" ",macaddr);
		}
	}
	
	printf("[%s:%d]\n",__func__,__LINE__);

	fclose(fp);
#endif	
	pid_t wifidogpid = read_pid(WIFIDOG_PID_FILE);
	if(wifidogpid>0){
		kill(wifidogpid, SIGUSR1);
	}
	return 1;
}


const char AWIFI_URL_CONF[]="/var/awifi_url.conf";

int setupTrustedUrlConf()
{
	FILE *fp;

	if ((fp = fopen(AWIFI_URL_CONF, "w")) == NULL) {
		printf("Open file %s failed !\n", AWIFI_URL_CONF);
		return -1;
	}

	MIB_AWIFI_URLFILTER_T	entry;
	char macaddr[20];
	int cnt = 0;
	int index = 0;

	cnt=mib_chain_total(MIB_AWIFI_URLFILTER_TBL);

	if(cnt > 0)
	{
		fprintf(fp, "FirewallRuleSet whitelist {    \n");

		for(index = 0; index < cnt; index++)
		{
			memset(&entry,0,sizeof(MIB_AWIFI_URLFILTER_T));
			
			mib_chain_get(MIB_AWIFI_URLFILTER_TBL,index,&entry);
			
			fprintf(fp, "    FirewallRule allow tcp to %s\n",entry.url);
		}
		fprintf(fp, "}");
	}

	printf("[%s:%d]\n",__func__,__LINE__);	

	fclose(fp);
	return 1;
}

void clearWhiteListMib()
{
	mib_chain_clear(MIB_AWIFI_MACFILTER_TBL);
	mib_chain_clear(MIB_AWIFI_URLFILTER_TBL);
	unsigned char disable_guest_ssid;
	mib_get(PROVINCE_DISABLE_GUEST_SSID_ON_STARTUP, &disable_guest_ssid);
	if(disable_guest_ssid==1){
		//disable guest ssid
		char guest_ssid=0;
		MIB_CE_MBSSIB_T Entry;
		int i, index=0;
		unsigned char vChar, bandSelect;

		for(i=0; i<NUM_WLAN_INTERFACE; i++) {
			guest_ssid = 0;
			mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, i, (void *)&vChar);
			mib_local_mapping_get(MIB_WLAN_GUEST_SSID, i, (void *)&guest_ssid);
			index = guest_ssid;
			if(vChar == PHYBAND_2G){
				//index = guest_ssid;
			}
			else{
#ifdef WLAN_DUALBAND_CONCURRENT
				if(guest_ssid > WLAN_SSID_NUM){
					index-=WLAN_SSID_NUM;
				}
#endif
			}
			if(guest_ssid > 0){
				mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i , index-1, (void *)&Entry);
				if(Entry.wlanDisabled==0){
					printf("disable ssid%d\n",guest_ssid);
					Entry.wlanDisabled = 1;
					mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, index-1);
				}
			}
		}
	}
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
//test code
/*
	printf("\n%s:%d\n",__func__,__LINE__);
	addPortalMacWhiteList("00:12:34:56:78:01");
	addPortalMacWhiteList("00:12:34:56:78:02");
	addPortalMacWhiteList("00:12:34:56:78:03");
	dumpPortalMacWhiteList();
	
	printf("%s:%d\n",__func__,__LINE__);
	deletePortalMacWhiteList("00:12:34:56:78:02");
	dumpPortalMacWhiteList();

	printf("%s:%d\n",__func__,__LINE__);
	setTemporaryAccessMode(2,2);
	allowDeviceTemporaryAccess("00:12:34:56:78:04");
	dumpPortalMacWhiteList();	

	printf("%s:%d\n",__func__,__LINE__);
	deletePortalMacWhiteList("");
	dumpPortalMacWhiteList();	

	printf("%s:%d\n",__func__,__LINE__);
	setPortalDomainWriteList("www.baidu.com");
	setPortalDomainWriteList("www.baidu1.com");
	setPortalDomainWriteList("www.baidu2.com");
	dumpPortalUrlWhiteList();

	printf("%s:%d\n",__func__,__LINE__);
	setPortalDomainWriteList("");
	dumpPortalUrlWhiteList();
*/
}

int MacFormatTohex(char *str, char *dst)
{
	int i=0;
	int length=strlen(str);
	int idx = 0;
	char buff[32] = {0};

	for (i=0;i<length && i<18;i++)
	{
		if (str[i] !=':' && str[i] !='-' )
			buff[idx++] = str[i];
	}

	if(!rtk_string_to_hex(buff, dst, 12))
	{
		return 0;
	}

	return 1;
}

void checkMacWhiteList()
{
	static int counter = 0;
	MIB_AWIFI_MACFILTER_T entry;
	int i,total;
	int rule_added=0;
	int modify=0;
#if 0//!defined(CONFIG_RTK_L34_ENABLE)
	int modified=0;
#endif

	counter++;
	if(counter >= 12){ // 1 minute
		total = mib_chain_total(MIB_AWIFI_MACFILTER_TBL);
		for(i=total-1;i>=0;i--){
			if(!mib_chain_get(MIB_AWIFI_MACFILTER_TBL,i,&entry))
				continue;
			if(entry.expiretime < 0)
				continue;
			if(entry.expiretime > 0)
				entry.expiretime--;
			if(entry.expiretime == 0){
				printf("%s:%d:delete entry %d for expiretime\n",__func__,__LINE__,i);
				modify=1;
				mac_whitelist_rule_del(entry);
				mib_chain_delete(MIB_AWIFI_MACFILTER_TBL,i);

				if(rule_added == 0 && accmode == TMP_ACCESS_ALLOW_ALL) {
#if defined(CONFIG_RTK_L34_ENABLE)
					//add rg rule
					RTK_RG_Wifidog_Rule_add();
#endif
					rule_added = 1;
					modify=1;
					printf("%s:%d:add rule for TMP_ACCESS_ALLOW_ALL\n",__func__,__LINE__);
				}
			}else
				mib_chain_update(MIB_AWIFI_MACFILTER_TBL,&entry,i);
		}

		if(tmp_acc_timeout > 0) {
			tmp_acc_timeout -- ;
			if(tmp_acc_timeout == 0) {
				if(rule_added == 0 && accmode == TMP_ACCESS_ALLOW_ALL) {
#if defined(CONFIG_RTK_L34_ENABLE)
					RTK_RG_Wifidog_Rule_add();
#endif
					rule_added = 1;
					modify=1;
					printf("%s:%d:add rule for TMP_ACCESS_ALLOW_ALL\n",__func__,__LINE__);
				}
			}
		}
		counter = 0;

		if(modify){
			dumpPortalMacWhiteList();
			dumpaccMode();
		}
	}
}
//chainname according to wifidog
#define CHAIN_OUTGOING  "WD_%s_Outgoing"
#define CHAIN_INCOMING  "WD_%s_Incoming"

typedef enum _t_fw_marks {
    FW_MARK_NONE = 0, /**< @brief No mark set. */
    FW_MARK_PROBATION = 1, /**< @brief The client is in probation period and must be authenticated 
			    @todo: VERIFY THAT THIS IS ACCURATE*/
    FW_MARK_KNOWN = 2,  /**< @brief The client is known to the firewall */
    FW_MARK_AUTH_IS_DOWN = 253, /**< @brief The auth servers are down */
    FW_MARK_LOCKED = 254 /**< @brief The client has been locked out */
} t_fw_marks;
#define CHAIN_MARK_MASK "/0xfff"

int get_lanhost_by_mac(char *macaddr, struct in_addr *pIp)
{
	int ret=-1;
	FILE *fp;
	char  buf[256];
	int Flags;
	char tmp0[20],tmp1[32],tmp2[32];
	char ip_str[INET_ADDRSTRLEN];
	char mac[MAC_ADDR_LEN];
	fp = fopen("/proc/net/arp", "r");
	if (fp == NULL){
		ret = -1;
		printf("read arp file fail!\n");
	}
	else {
		fgets(buf, 256, fp);//first line!?
		while(fgets(buf, 256, fp) >0) {
			//sscanf(buf, "%s", dmzIP);
			sscanf(buf,"%s	%*s 0x%x %s %s %s ", ip_str, &Flags,tmp0,tmp1,tmp2);
			if (strncmp(tmp2,"br",2)!=0)
				continue;
			convertMacFormat(tmp0, mac);
			if(strncmp(mac, macaddr, MAC_ADDR_LEN)==0){
				inet_pton(AF_INET, ip_str, pIp); 
				ret = 0;
				break;
			}
		}
		fclose(fp);
	}
	return ret;
}
void mac_whitelist_rule_add(MIB_AWIFI_MACFILTER_T entry)
{
	char chainname[32];
	char mac_str[20]={0};
	char mark_str[10]={0};
	const char *cmd[2]={IPTABLES, IP6TABLES};


	snprintf(chainname, 31, CHAIN_OUTGOING, LANIF);//WD_br0_Outgoing
	snprintf(mark_str, 9, "%d%s", FW_MARK_KNOWN, CHAIN_MARK_MASK);//2/0xfff
	snprintf(mac_str, 19, "%02X:%02X:%02X:%02X:%02X:%02X", 
			entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);

	//iptables -t mangle -A WD_br0_Outgoing -s ip_addr -m mac --mac-source mac_addr -j MARK --set-mark 2/0xfff
	fprintf(stderr, "\t [%s:%d]cmd: %s -t mangle -A %s  -m mac --mac-source %s -j MARK --set-mark %s\n",__func__, __LINE__
			,IPTABLES, chainname, mac_str, mark_str);
	va_cmd(IPTABLES, 12, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, chainname,  
			"-m", "mac", "--mac-source", mac_str, "-j", "MARK", "--set-mark", mark_str);
#ifdef CONFIG_IPV6
	fprintf(stderr, "\t [%s:%d]cmd: %s -t mangle -A %s  -m mac --mac-source %s -j MARK --set-mark %s\n",__func__, __LINE__
			,IP6TABLES, chainname, mac_str, mark_str);
	va_cmd(IP6TABLES, 12, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, chainname,  
			"-m", "mac", "--mac-source", mac_str, "-j", "MARK", "--set-mark", mark_str);
#endif

#if defined(CONFIG_RTK_L34_ENABLE)
#if !defined(CONFIG_CMCC_MESH_WIFI_PORTAL_SUPPORT)
	RTK_RG_Wifidog_mac_whitelist_add(entry.mac);
#endif
#endif
}

void mac_whitelist_rule_del(MIB_AWIFI_MACFILTER_T entry)
{
	char chainname[32];
	char mac_str[20]={0};
	char mark_str[10]={0};

	snprintf(chainname, 31, CHAIN_OUTGOING, LANIF);//WD_br0_Outgoing
	snprintf(mark_str, 9, "%d%s", FW_MARK_KNOWN, CHAIN_MARK_MASK);//2/0xfff
	snprintf(mac_str, 19,"%02X:%02X:%02X:%02X:%02X:%02X", 
			entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);

	//iptables -t mangle -A WD_br0_Outgoing -s ip_addr -m mac --mac-source mac_addr -j MARK --set-mark 2/0xfff
	//printf("\t cmd: iptables -t mangle -D %s -s %s -m mac --mac-source %s -j MARK --set-mark %s\n", chainname, ip_str, mac_str, mark_str);
	fprintf(stderr, "\t [%s:%d]cmd: %s -t mangle -D %s	-m mac --mac-source %s -j MARK --set-mark %s\n",__func__, __LINE__
			,IPTABLES, chainname, mac_str, mark_str);
	va_cmd(IPTABLES, 12, 1, (char *)ARG_T, "mangle", (char *)FW_DEL, chainname,  
			"-m", "mac", "--mac-source", mac_str, "-j", "MARK", "--set-mark", mark_str);
#ifdef CONFIG_IPV6
	fprintf(stderr, "\t [%s:%d]cmd: %s -t mangle -D %s	-m mac --mac-source %s -j MARK --set-mark %s\n",__func__, __LINE__
			,IP6TABLES, chainname, mac_str, mark_str);
	va_cmd(IP6TABLES, 12, 1, (char *)ARG_T, "mangle", (char *)FW_DEL, chainname,  
			"-m", "mac", "--mac-source", mac_str, "-j", "MARK", "--set-mark", mark_str);
#endif

#if defined(CONFIG_RTK_L34_ENABLE)
#if !defined(CONFIG_CMCC_MESH_WIFI_PORTAL_SUPPORT)
	RTK_RG_Wifidog_mac_whitelist_del(entry.mac);
#endif
#endif

}
#define AWIFI_MACFILTER_ADD_LOCK "/tmp/awifi_macfilter_add_lock"
#define AWIFI_MACFILTER_ADD_NEIGHBOR "/tmp/awifi_macfilter_add_neighbor"
int mac_whitelist_add_by_mac(char *mac, int expiretime)
{
	//update mac-ip mib entry
	int ret = 0;
	MIB_AWIFI_MACFILTER_T entry;
	//int lock_fd, file_retry;
	//FILE *neighbor_fp;
	char  buf[256];
	char ip_str[INET6_ADDRSTRLEN]={0};
	char mac_str[20]={0};

	entry.expiretime = expiretime;
	memcpy(entry.mac ,mac, MAC_ADDR_LEN);
	snprintf(mac_str, 19,"%02x:%02x:%02x:%02x:%02x:%02x", 
			entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);

	if(mib_chain_add(MIB_AWIFI_MACFILTER_TBL, &entry)>0){
		mac_whitelist_rule_add(entry);
	}else{
		printf("add MIB_AWIFI_MACFILTER_TBL entry fail\n");
		ret = -1;
	}
	setupTrustedMacConf();

	return ret;
}
int  mac_whitelist_del_by_mac(char *mac)
{
	//delete related mac entry
	int ret = 0;
	MIB_AWIFI_MACFILTER_T entry;
	int total, i;

	total = mib_chain_total(MIB_AWIFI_MACFILTER_TBL);

	for(i=total-1;i>=0;i--){
		if(!mib_chain_get(MIB_AWIFI_MACFILTER_TBL,i,&entry))
			continue;
		if(!memcmp(mac,entry.mac,MAC_ADDR_LEN)){
			//printf("%s:%d:del mac %02X:%02X:%02X:%02X:%02X:%02X\n",__func__,__LINE__,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

			mac_whitelist_rule_del(entry);
			if(mib_chain_delete(MIB_AWIFI_MACFILTER_TBL,i)==0){
				printf("del MIB_AWIFI_MACFILTER_TBL entry %d fail\n", i);
				ret = -1;
				break;
			}
		}
	}
	setupTrustedMacConf();
	
	return ret;

}

int addPortalMacWhiteList(char *macaddr)
{
	int ret = 0;
	MIB_AWIFI_MACFILTER_T entry;
	unsigned char mac[MAC_ADDR_LEN];
	int i,total;
	struct in_addr ip;

	if(!macaddr || !strlen(macaddr))
		return -1;

	if(!MacFormatTohex(macaddr,mac))
		return -1;
	//printf("%s:%d:mac is %02X:%02X:%02X:%02X:%02X:%02X\n",__func__,__LINE__,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

	total = mib_chain_total(MIB_AWIFI_MACFILTER_TBL);
	for(i=0;i<total;i++){
		if(!mib_chain_get(MIB_AWIFI_MACFILTER_TBL,i,&entry))
			continue;
		if(!memcmp(mac,entry.mac,MAC_ADDR_LEN)){
			//printf("%s:%d:find mac %s, reomve all\n",__func__,__LINE__,macaddr);
			mac_whitelist_del_by_mac(mac);
			break;
		}
	}
	ret = mac_whitelist_add_by_mac(mac, -1);

	return ret;
}

int clearPortalMacWhiteList()
{
	int ret = 0;
	MIB_AWIFI_MACFILTER_T entry;
	int i,total;

	total = mib_chain_total(MIB_AWIFI_MACFILTER_TBL);
	for(i=total-1;i>=0;i--){
		if(!mib_chain_get(MIB_AWIFI_MACFILTER_TBL,i,&entry))
			continue;

		mac_whitelist_rule_del(entry);
			
		mib_chain_delete(MIB_AWIFI_MACFILTER_TBL,i);
	}

	return ret;
}

int deletePortalMacWhiteList(char *macaddr)
{
	int ret = 0;
	MIB_AWIFI_MACFILTER_T entry;
	unsigned char mac[MAC_ADDR_LEN];
	int i,total;

	if(!macaddr)
		return -1;

	if(!strlen(macaddr)){
		clearPortalMacWhiteList();
		printf("%s:%d:clear mac\n",__func__,__LINE__);
	}else{
		if(!MacFormatTohex(macaddr,mac))
			return -1;
		//printf("%s:%d:mac is %02X:%02X:%02X:%02X:%02X:%02X\n",__func__,__LINE__,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
		ret = mac_whitelist_del_by_mac(mac);

	}

	return ret;
}

void updateTemporaryAccessMacWhiteList(int accmode)
{
	MIB_AWIFI_MACFILTER_T entry;
	int i,total;
	
	total = mib_chain_total(MIB_AWIFI_MACFILTER_TBL);
	for(i=total-1;i>=0;i--){
		if(!mib_chain_get(MIB_AWIFI_MACFILTER_TBL,i,&entry))
			continue;
		if(entry.expiretime < 0)
			continue;

		entry.expiretime = expiretimeval;
		
		if(accmode == TMP_ACCESS_DISABLE){
			mac_whitelist_rule_del(entry);
			printf("%s:%d:delete entry %d for expiretime\n",__func__,__LINE__,i);
		}else {
			mac_whitelist_rule_add(entry);
			printf("%s:%d:add entry %d for expiretime\n",__func__,__LINE__,i);
		}

		mib_chain_update(MIB_AWIFI_MACFILTER_TBL,&entry,i);
	}
}

int setTemporaryAccessMode(int mode, int expireTime)
{
	int ret = 0;
	int orig_mode = accmode;

	accmode = mode;

	expiretimeval = expireTime;

	printf("%s:%d:change accmode %d to %d , expireTime=%d\n",__func__,__LINE__,orig_mode, mode, expireTime);

	if(orig_mode == TMP_ACCESS_ALLOW_ALL) {
		//orig mode is TMP_ACCESS_ALLOW_ALL
#if defined(CONFIG_RTK_L34_ENABLE)
		RTK_RG_Wifidog_Rule_add();
#endif
	}
	
	if(accmode == TMP_ACCESS_ALLOW_ALL) {
		//new mode is TMP_ACCESS_ALLOW_ALL
#if defined(CONFIG_RTK_L34_ENABLE)
		RTK_RG_Wifidog_Rule_del();
#endif
	}

	if(accmode == TMP_ACCESS_DISABLE)
		tmp_acc_timeout = 0;
	else
		tmp_acc_timeout = expiretimeval;

	printf("%s:%d:change accmode %d to %d , tmp_acc_timeout=%d\n",__func__,__LINE__,orig_mode, mode, tmp_acc_timeout);

	updateTemporaryAccessMacWhiteList(accmode);
	
	if(orig_mode!=accmode)
		dumpaccMode();
	return ret;
}

int allowDeviceTemporaryAccess(char *macaddr)
{
	int ret = 0;
	MIB_AWIFI_MACFILTER_T entry;
	unsigned char mac[MAC_ADDR_LEN];
	int i,total;
	struct in_addr ip;

	if(!macaddr || !strlen(macaddr))
		return -1;

	if(!MacFormatTohex(macaddr,mac))
		return -1;
	//printf("%s:%d:mac is %02X:%02X:%02X:%02X:%02X:%02X\n",__func__,__LINE__,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

	if(get_lanhost_by_mac(mac, &ip)<0)
	{
		fprintf(stderr, "%s:get ip address error!!!\n",__FUNCTION__);
		return -1;
	}

	total = mib_chain_total(MIB_AWIFI_MACFILTER_TBL);
	for(i=0;i<total;i++){
		if(!mib_chain_get(MIB_AWIFI_MACFILTER_TBL,i,&entry))
			continue;
		if(!memcmp(mac,entry.mac,MAC_ADDR_LEN)){
			//printf("%s:%d:find mac %s, reomve all\n",__func__,__LINE__,macaddr);
			mac_whitelist_del_by_mac(mac);
			break;
		}
	}

#if defined(CONFIG_RTK_L34_ENABLE)
		if(accmode == TMP_ACCESS_DISABLE) {
			//clear rg rule if exist
			mac_whitelist_rule_del(entry);
		} else {
			//add rg rule
			mac_whitelist_rule_add(entry);
		}
#endif
	if(accmode == TMP_ACCESS_DISABLE)
		printf("%s:%d:TemporayAccess Mode disabled, do not add mac to whitelist\n",__func__,__LINE__);
	else
		ret = mac_whitelist_add_by_mac(mac, expiretimeval);

	return ret;
}

int setPortalDomainWriteList(char *urlstr)
{
	int ret = 0;
	MIB_AWIFI_URLFILTER_T newentry,entry;
	unsigned char mac[MAC_ADDR_LEN];
	int i,total;
	int modified=0;
	char mode[MAX_NAME_LEN]={0};

	if(!urlstr)
		return -1;

	printf("%s:%d:urlstr is %s\n",__func__,__LINE__,urlstr);

	if(!strlen(urlstr)){
		mib_chain_clear(MIB_AWIFI_URLFILTER_TBL);
		printf("%s:%d:clear url\n",__func__,__LINE__);
		modified=1;
	}
	else{
		total = mib_chain_total(MIB_AWIFI_URLFILTER_TBL);
		for(i=0;i<total;i++){
			if(!mib_chain_get(MIB_AWIFI_URLFILTER_TBL,i,&entry))
				continue;
			if(!strcmp(urlstr,entry.url)){
				printf("%s:%d:find url %s\n",__func__,__LINE__,urlstr);
				return ret;
			}
		}

		strncpy(newentry.url,urlstr,MAX_URL_LENGTH-1);
		if(mib_chain_add(MIB_AWIFI_URLFILTER_TBL,&newentry)){
			printf("%s:%d:add url %s\n",__func__,__LINE__,urlstr);
			modified=1;
		}else
			ret = -1;
	}

	WifidogMode(mode);
	if((strcmp(mode, "portal")==0) && modified == 1) {
		//update wifidog conf
		setupTrustedUrlConf();

		//restart wifidog
		restartWiFiDog(1);
	}
	
	return ret;
}

void dumpPortalMacWhiteList()
{
	MIB_AWIFI_MACFILTER_T entry;
	int i,total;

	printf("Mac List:\n");
	total = mib_chain_total(MIB_AWIFI_MACFILTER_TBL);
	for(i=0;i<total;i++){
		if(!mib_chain_get(MIB_AWIFI_MACFILTER_TBL,i,&entry))
			continue;
		printf("%02X:%02X:%02X:%02X:%02X:%02X,expire=%d\n",entry.mac[0],entry.mac[1],entry.mac[2],entry.mac[3],entry.mac[4],entry.mac[5],entry.expiretime);
	}
}

void dumpaccMode()
{
	printf("Temporary access mode: %d\n",accmode);
	printf("Temporary access expire time: %d\n",expiretimeval);
	printf("Temporary access expire timeout: %d\n",tmp_acc_timeout);
}

void dumpPortalUrlWhiteList()
{
	MIB_AWIFI_URLFILTER_T entry;
	int i,total;

	printf("URL List:\n");
	total = mib_chain_total(MIB_AWIFI_URLFILTER_TBL);
	for(i=0;i<total;i++){
		if(!mib_chain_get(MIB_AWIFI_URLFILTER_TBL,i,&entry))
			continue;
		printf("url=%s\n",entry.url);
	}
}

int setupMacWhitelist()
{
	MIB_AWIFI_MACFILTER_T entry;
	char macaddr[20];
	int cnt = 0;
	int index = 0;
	struct in_addr ip;

	cnt=mib_chain_total(MIB_AWIFI_MACFILTER_TBL);

	for(index = 0; index < cnt; index++) {
		memset(&entry,0,sizeof(MIB_AWIFI_MACFILTER_T));
		mib_chain_get(MIB_AWIFI_MACFILTER_TBL,index,&entry);
		if(get_lanhost_by_mac(entry.mac, &ip)<0)
		{
			fprintf(stderr, "%s:get ip address error, skip mac %02X:%02X:%02X:%02X:%02X:%02X\n"
					,__FUNCTION__, entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);
			continue;
		}
		
		mac_whitelist_rule_add(entry);
	}
	
	printf("[%s:%d]\n",__func__,__LINE__);
	return 1;
}

void Wifidog_mac_whitelist_add(char *macaddr)
{
	unsigned char mac[MAC_ADDR_LEN];
	if(!MacFormatTohex(macaddr,mac))
		return;
	//when not in portal mode, wifidog control iptables it self, so just do rg rules
#if defined(CONFIG_RTK_L34_ENABLE)
#if !defined(CONFIG_CMCC_MESH_WIFI_PORTAL_SUPPORT)
	return RTK_RG_Wifidog_mac_whitelist_add(mac);
#endif
#endif
}
void Wifidog_mac_whitelist_del(char *macaddr)
{
	unsigned char mac[MAC_ADDR_LEN];
	if(!MacFormatTohex(macaddr,mac))
		return;
	//when not in portal mode, wifidog control iptables it self, so just do rg rules
#if defined(CONFIG_RTK_L34_ENABLE)
#if !defined(CONFIG_CMCC_MESH_WIFI_PORTAL_SUPPORT)
	return RTK_RG_Wifidog_mac_whitelist_del(mac);
#endif
#endif
}
int Wifidog_mac_whitelist_clear()
{
	unsigned char mac[MAC_ADDR_LEN]={0};
	//when not in portal mode, wifidog control iptables it self, so just do rg rules
#if defined(CONFIG_RTK_L34_ENABLE)
#if !defined(CONFIG_CMCC_MESH_WIFI_PORTAL_SUPPORT)
	return RTK_RG_Wifidog_mac_whitelist_del(mac);
#endif
#endif
}

int addRGPortalServer(char *ip)
{
	int ret=0;
#if defined(CONFIG_RTK_L34_ENABLE)
	ret = RTK_RG_wifidog_portal_server_add(ip);
#endif
	//FILE* fp=fopen(WIFI_PORTAL_RUNING_FLAG,"w");
	//if(fp)
	//	fclose(fp);
	return ret;
}

int delRGPortalServer(char *ip)
{
	int ret=0;
#if defined(CONFIG_RTK_L34_ENABLE)
	ret = RTK_RG_wifidog_portal_server_del(ip);
#endif
	//unlink(WIFI_PORTAL_RUNING_FLAG);
	return ret;
}
int clearRGPortalServer()
{
#if defined(CONFIG_RTK_L34_ENABLE)
	_clearRGPortalServer();
#endif
}

int sendPortalEvent(char* eventString, char *mac_str, char* ip_str, int ssid_index, char* http_url, char* redirect_url, int redirect_url_len)
{
	int ret=0;
	const char* jsonString;
	FILE *fp;	
	int lock_fd, file_lock_retry;
	char buff[256],bundleName[256];
	char response[4096]={0};
	cJSON* response_json=NULL;
	cJSON* url=NULL;
	cJSON *root = NULL;
	root = cJSON_CreateObject();
	if (root == NULL) {
		printf("[%s %d]:malloc json object failed , leave!\n", __func__, __LINE__);
		return -1;
	}

	//printf("sending portalEvent\n");
	lock_fd = open(PORTALSERVICE_REGISTER_BUNDLE_LOCK, O_RDWR|O_CREAT|O_TRUNC);
	if(lock_fd == -1){
		printf("%s: Can't lock file :%s \n", __FUNCTION__, PORTALSERVICE_REGISTER_BUNDLE_LOCK);
		ret = -1;
		goto end;
	}
	file_lock_retry = 3;
	while(file_lock_retry && (flock(lock_fd,LOCK_EX|LOCK_NB)==-1) && errno==EINTR){//try to lock file 3 times
		if(file_lock_retry = 0){
			printf("%s: Can't lock file :%s \n", __FUNCTION__, PORTALSERVICE_REGISTER_BUNDLE_LOCK);
			close(lock_fd);
			ret = -1;
			goto end;
		}
		file_lock_retry--;
		usleep(1000);
	}
#if 1
	cJSON_AddStringToObject(root, "Event", eventString);
	cJSON_AddStringToObject(root, "MacAddr", mac_str);
	cJSON_AddStringToObject(root, "IP", ip_str);
	cJSON_AddNumberToObject(root, "SSIDIndex", ssid_index);
	cJSON_AddStringToObject(root, "HttpUrl", http_url);
#else
	json_object_object_add(jobj, "Event", json_object_new_string(eventString));
	json_object_object_add(jobj, "MacAddr", json_object_new_string(mac_str));
	json_object_object_add(jobj, "IP", json_object_new_string(ip_str));
	json_object_object_add(jobj, "SSIDIndex", json_object_new_int(ssid_index));
	json_object_object_add(jobj, "HttpUrl", json_object_new_string(HttpUrl));
	jsonString = json_object_get_string(jobj);
	fprintf(stderr, "sendPortalEvent json: %s\n",jsonString);
	printf("sending %s to %s\n",eventString, bundleName);
#endif
	jsonString=cJSON_PrintUnformatted(root);
	//printf("[%s %d %lu] jsonData = %s\n", __FUNCTION__, __LINE__, time(NULL), jsonString);
	fp = fopen(PORTALSERVICE_REGISTER_BUNDLE, "r");
	if(fp){
		memset(buff, 0, sizeof(buff));
		while (fgets(buff, sizeof(buff), fp) != NULL) { //found the bundle name exist or not 
			memset(response,0,sizeof(response));
			sscanf(buff, "%s", bundleName);
#if defined(CONFIG_APACHE_FELIX_FRAMEWORK) 
			ret = sendMessageOSGIManagement(bundleName,(char*)jsonString, response);
			//printf("[%s %d %lu]: Send event to OSGI management ret(sestsendMessageOSGIManagement)=%d  buff=%s\n", __func__, __LINE__, time(NULL), ret,response);
#endif
			if(ret == 0)
				break;	
		}
		fclose(fp);
#if 1
		if(ret==0){
			response_json = cJSON_Parse(response);
			//printf("[%s %d] response_json = %s\n", __FUNCTION__, __LINE__, cJSON_PrintUnformatted(response_json));
			if(response_json){
				url = cJSON_GetObjectItem(response_json, "Url");
				if(url)
				{
					//printf("[%s %d]Url is %s!!!\n",__FUNCTION__, __LINE__,url->valuestring);
					strncpy(redirect_url, url->valuestring,redirect_url_len);
					goto end;
				}
				else{
					//printf("no Url get!!!\n");
					ret = -1;
					goto end;
				}
			}
			else{
				printf("[%s %d]no response_json!!!\n",__FUNCTION__, __LINE__);
				ret = -1;
				goto end;
			}
		}
#endif
	}
	else{
		printf("open file %s fail, might because not subscribe\n",PORTALSERVICE_REGISTER_BUNDLE);
		ret = -1;
		goto end;
	}
end:
	if(root)
		cJSON_Delete(root);
	if(response_json)
		cJSON_Delete(response_json);
	while(file_lock_retry && (flock(lock_fd,LOCK_UN)==-1) && errno==EINTR){//try to unlock file 3 times
		if(file_lock_retry = 0){
			printf("%s: Can't unlock file :%s!!!! \n", __FUNCTION__, PORTALSERVICE_REGISTER_BUNDLE);
			close(lock_fd);
			return -1;
		}
		file_lock_retry--;
		usleep(1000);
	}
	close(lock_fd);
	unlink(PORTALSERVICE_REGISTER_BUNDLE_LOCK);
	return ret;
}

int get_assoc_ssid_index_by_mac(unsigned char *mac_addr_str)
{
	char interface_name[16];
	unsigned char mac_addr[6];
	WLAN_STA_INFO_T buff[MAX_STA_NUM + 1]={0};
	WLAN_STA_INFO_Tp pInfo;
	int i;
	int ssid_idx=0;
	if(!MacFormatTohex(mac_addr_str,mac_addr))
	{
		printf("Error! Invalid MAC address.");
		return -1;	
	}

	for(ssid_idx=1; ssid_idx <=8; ssid_idx++)
	{
		if(get_ifname_by_ssid_index(ssid_idx, interface_name) < 0)
		{
			printf("Get SSID%d interface name faileld\n", ssid_idx);
			continue;
		}

		memset(buff, 0, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM + 1));
		if (getWlStaInfo(interface_name, (WLAN_STA_INFO_Tp) buff) < 0) {
			printf("Read wlan sta info failed!\n");
			continue;
		}

		for (i = 1; i <= MAX_STA_NUM; i++) {
			pInfo = (WLAN_STA_INFO_Tp) &buff[i];
			if (pInfo->aid && (pInfo->flags & STA_INFO_FLAG_ASOC)) {
				if(!strncmp(pInfo->addr, mac_addr, 6)){
					return ssid_idx;
				}
			}
		}
	}

	return -1;
}

#endif

#ifdef CONFIG_CMCC_AUDIT
int getwifiFiledStrength(unsigned char *pMacAddr)//wlan0 or wlan 1
{
	WLAN_STA_INFO_Tp pInfo;
	char *buff;
	int i, rssi;
	char macString1[32]={0};
	char macString2[32]={0};
	buff = calloc(1, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM + 1));
	int ssid_idx=0;
	char interface_name[16];
	if (buff == 0) {
		printf("Allocate buffer failed!\n");
		return -1;
	}

	for(ssid_idx=1; ssid_idx <=8; ssid_idx++)
	{
		if(get_ifname_by_ssid_index(ssid_idx, interface_name) < 0)
		{
			printf("Get SSID%d interface name faileld\n", ssid_idx);
			continue;
		}

		memset(buff, 0, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM + 1));
		if (getWlStaInfo(interface_name, (WLAN_STA_INFO_Tp) buff) < 0) {
			printf("Read wlan sta info failed!\n");
			continue;
		}

		for (i = 1; i <= MAX_STA_NUM; i++) {
			pInfo = (WLAN_STA_INFO_Tp) &buff[i];
			if (pInfo->aid && (pInfo->flags & STA_INFO_FLAG_ASOC)) {
				if(!strncmp(pInfo->addr, pMacAddr, 6)){
					rssi = pInfo->rssi - 100;
					return rssi;
				}
			}
		}
	}
#if 0
	if(getWlStaInfo((char *)WLANIF[0],  (WLAN_STA_INFO_Tp)buff) < 0) 
	{
		printf("Read wlan sta info failed!\n");
		free(buff);
		return -1;
	}
	for(i=1; i<=MAX_STA_NUM; i++)
	{
		pInfo = (WLAN_STA_INFO_Tp)&buff[i*sizeof(WLAN_STA_INFO_T)];
		
		if(pInfo->aid && (pInfo->flags & STA_INFO_FLAG_ASOC))
		{
			if( !macAddrCmp(pInfo->addr, pMacAddr) )
			{
				rssi = pInfo->rssi - 100;
				return rssi;
			}
		}
	}
#endif
	free(buff);
	return 0;
}

int getDevOnlineTimeRxRateTxRate(unsigned char *pMacAddr, unsigned int *pUpRate, unsigned int *pDownRate, unsigned int *onLineTime)
{
	int ret=-1, idx;
	lanHostInfo_t *pLanNetInfo=NULL;
	unsigned int count=0;
	ret = get_lan_net_info(&pLanNetInfo, &count);
	if(ret<0)
	{
		if(pLanNetInfo)
			free(pLanNetInfo);
		return ret;
	}
	for(idx=0; idx<count; idx++)
	{
		if( !macAddrCmp(pLanNetInfo[idx].mac, pMacAddr) )
		{
			*pUpRate = pLanNetInfo[idx].upRate;
			*pDownRate = pLanNetInfo[idx].downRate;
			*onLineTime = pLanNetInfo[idx].onLineTime;
			if(pLanNetInfo)
				free(pLanNetInfo);

			return 0;
		}
	}
	return -3;
}

int getIsAuth(void)
{
	int value = -1;
#if defined(CONFIG_CMCC_WIFI_PORTAL)
	char guest_ssid_2g=0, guest_ssid_5g=0;
	int i;
	unsigned char vChar, bandSelect;

	for(i=0; i<NUM_WLAN_INTERFACE; i++) {
		mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, i, (void *)&vChar);
		if(vChar == PHYBAND_2G) {
			mib_local_mapping_get(MIB_WLAN_GUEST_SSID, i, (void *)&guest_ssid_2g);
		}
		else{
			mib_local_mapping_get(MIB_WLAN_GUEST_SSID, i, (void *)&guest_ssid_5g);
		}
	}

	if(guest_ssid_2g || guest_ssid_5g)
		value = 1;
	else
		value = 0;
#endif
	return value;
}

int subStrOfIPAddr(const char *s, char c)
{
    int flag = 0;
    int index = 0;
    if (NULL == s)
    {
        return -1;
    }
    while (*s != '\0')
    {
        if (*s == c)
        {
            ++flag;
            ++index;
            ++s;
            if (flag == 2)
            {
                return index;
            }
        }
        ++s;
        ++index;
    }
    return index;
}

int wifiPortalOnLineInfo(wifi_portal_info_t * wifiDevInfo,  int status)
{
	FILE *fp;
	unsigned char fileName[256];
	unsigned char macAddr[MAC_ADDR_LEN]={0};
	char* result;
	time_t time_lt;
	int index = 0, isAuth, ssid_index, rssi_value;
	char ip_result[32]={0};
	
	
	if(wifiDevInfo == NULL)
	{
		printf("[%s %d]error!\n", __func__, __LINE__);
		return -1;
	}

	if(opendir("mkdir -p /tmp/gram/apstatus/on_off_line") == NULL)
	{
		system("mkdir -p /tmp/gram/apstatus/on_off_line");
	}

	time_lt = time(NULL);
	
	char * ipStr = wifiDevInfo->ip_local;
    index = subStrOfIPAddr(ipStr, '.');
	isAuth = getIsAuth();
	strncpy(ip_result, ipStr+index, strlen(ipStr) - index);
	
	snprintf(fileName, sizeof(fileName), "/tmp/gram/apstatus/on_off_line/%s_%d.info", ip_result, status);

	
	MacFormatTohex(wifiDevInfo->macAddr, macAddr);
	//field_strength
	rssi_value = getwifiFiledStrength(macAddr);
	
	fp = fopen(fileName, "w+");
	if(fp)
	{
		fprintf(fp, "auth_mode=%d\n", wifiDevInfo->auth_mode);
		if(isAuth == 1)
			fprintf(fp, "account=%s\n", wifiDevInfo->account);
		else
			fprintf(fp, "account= \n");
		fprintf(fp, "ip_type=%d\n", wifiDevInfo->ip_type);
		fprintf(fp, "ip=%s\n", wifiDevInfo->ip_local);
		fprintf(fp, "usr_mac=%s\n", wifiDevInfo->macAddr);
		fprintf(fp, "onoff_flag=%d\n", status);
		fprintf(fp, "onoff_time=%ld\n", time_lt);
		fprintf(fp, "nat_port=%d\n", wifiDevInfo->nat_port);
		if(rssi_value == 0 || rssi_value == -1)
			fprintf(fp, "field_strength= \n");
		else
			fprintf(fp, "field_strength=%d\n", rssi_value);
		
		fclose(fp);
		return 0;
	}
	return -1;
}

int wifiPortalOffLineInfo(wifi_portal_info_t * wifiDevInfo, int status)
{
	FILE *fp;
	unsigned char fileName[256];
	unsigned char macAddr[MAC_ADDR_LEN]={0};
	unsigned int upRate=0, downRate=0, onLineTime=0, rssi_value;
	char* result;
	time_t time_lt;

	int index = 0, isAuth;
	char ip_result[32]={0};
	
	if(wifiDevInfo == NULL)
	{
		printf("[%s %d]error!\n", __func__, __LINE__);
		return -1;
	}

	if(opendir("mkdir -p /tmp/gram/apstatus/on_off_line") == NULL){
		system("mkdir -p /tmp/gram/apstatus/on_off_line");
	}

	time_lt = time(NULL);
	
	char * ipStr = wifiDevInfo->ip_local;
    index = subStrOfIPAddr(ipStr, '.');
	isAuth = getIsAuth();
	strncpy(ip_result, ipStr+index, strlen(ipStr) - index);
	snprintf(fileName, sizeof(fileName), "/tmp/gram/apstatus/on_off_line/%s_%d.info", ip_result, status);

	MacFormatTohex(wifiDevInfo->macAddr, macAddr);
	getDevOnlineTimeRxRateTxRate(macAddr, &upRate, &downRate, &onLineTime);
	
	//field_strength
	rssi_value = getwifiFiledStrength(macAddr);
	
	fp = fopen(fileName, "w+");
	if(fp)
	{
		fprintf(fp, "auth_mode=%d\n", wifiDevInfo->auth_mode);
		if(isAuth == 1)
			fprintf(fp, "account=%s\n", wifiDevInfo->account);
		fprintf(fp, "ip_type=%d\n", wifiDevInfo->ip_type);
		fprintf(fp, "ip=%s\n", wifiDevInfo->ip_local);
		fprintf(fp, "usr_mac=%s\n", wifiDevInfo->macAddr);
		fprintf(fp, "onoff_flag=%d\n", status);
		fprintf(fp, "onoff_time=%ld\n", time_lt);
		fprintf(fp, "nat_port=%d\n", wifiDevInfo->nat_port);
		if(rssi_value == 0 || rssi_value == -1)
			fprintf(fp, "field_strength= \n");
		else
			fprintf(fp, "field_strength=%d\n", rssi_value);
		fprintf(fp, "online_time=%d\n", onLineTime);
		fprintf(fp, "upload=%d\n", upRate);
		fprintf(fp, "download=%d\n", downRate);
		fclose(fp);
		return 0;
	}
	return -1;
}
#endif

#ifdef CONFIG_WAN_INTERVAL_TRAFFIC
#define WAN_INTERVAL_TRAFFIC_SAMPLES_MIN 2
#define WAN_INTERVAL_TRAFFIC_SAMPLES_MAX 16
struct callout_s wanIntervalTraffic_ch;
struct WAN_INTERVAL_TRAFFIC_SAMPLES{
	unsigned long long traffics;
	unsigned char isset;
}wanIntervalTrafficSamples[WAN_INTERVAL_TRAFFIC_SAMPLES_MAX]={0};
int wanIntervalTrafficSamples_idx = 0;
#define WanIntervalTrafficLock "/var/run/WanIntervalTrafficLock"
#define LOCK_WanIntervalTraffic_RET(x)	\
do {	\
	if ((lockfd = open(WanIntervalTrafficLock, O_RDWR|O_CREAT)) == -1) {	\
		perror("open WanIntervalTrafficLock lockfile");	\
		return (x);	\
	}	\
	while (flock(lockfd, LOCK_EX)) { \
		if (errno != EINTR) \
			break; \
		usleep(1000);\
	}	\
} while (0)

#define LOCK_WanIntervalTraffic()	\
do {	\
	if ((lockfd = open(WanIntervalTrafficLock, O_RDWR|O_CREAT)) == -1) {	\
		perror("open WanIntervalTrafficLock lockfile");	\
		return;	\
	}	\
	while (flock(lockfd, LOCK_EX)) { \
		if (errno != EINTR) \
			break; \
		usleep(1000);\
	}	\
} while (0)

#define UNLOCK_WanIntervalTraffic()	\
do {	\
	flock(lockfd, LOCK_UN);	\
	close(lockfd);	\
} while (0)
#if 0
void dumpWanIntervalTraffic(){
		unsigned char wit_time=0;
		unsigned char wit_samples=0;
		mib_get(MIB_WAN_INTERVAL_TRAFFIC_TIME, &wit_time);
		mib_get(MIB_WAN_INTERVAL_TRAFFIC_SAMPLES, &wit_samples);		
		int samples_idx_s = (wanIntervalTrafficSamples_idx+wit_samples-1)%wit_samples;
		unsigned long long total_begin=0;
		unsigned long long total_end=0;
		int wit_isset_valid_samples=0;
		int lockfd = 0;
		if(wit_time&&wit_samples){
			int i = 0;
			int prev = 0;
			printf("total samples:%d\n",wit_samples);
			LOCK_WanIntervalTraffic();
			for(i=samples_idx_s;i>(samples_idx_s-wit_samples);i--){
				
				int real_i = (i+wit_samples)%wit_samples;				
				if(wanIntervalTrafficSamples[real_i].isset){
					printf("wanIntervalTrafficSamples[%d]=%llu\n",real_i,wanIntervalTrafficSamples[real_i].traffics);
					if(real_i==samples_idx_s)
						total_begin=total_end=wanIntervalTrafficSamples[real_i].traffics;
					else{
						if(total_end>= wanIntervalTrafficSamples[real_i].traffics){
							total_begin = wanIntervalTrafficSamples[real_i].traffics;
							wit_isset_valid_samples++;							
						}else{
							break;
						}
					}					
				}
			}
			UNLOCK_WanIntervalTraffic();
			printf("%s %d#############total_end=%llu,total_begin=%llu,wit_isset_valid_samples=%d ret=%llu\n",__FUNCTION__,__LINE__,total_end,total_begin,wit_isset_valid_samples,wit_isset_valid_samples?((total_end-total_begin)/wit_isset_valid_samples):(1*1024*1024));
			return;
		}
		return;
}
#endif
void wanIntervalTraffic()
{
	unsigned char wit_time=0;
	unsigned char wit_samples=0;
	unsigned char wit_mode=0;
	unsigned char wit_type=0;
	unsigned char control_iptv_flow_counter=0;

	static unsigned char last_wit_time=0;
	static unsigned char last_wit_samples=0;
	static unsigned char last_wit_mode=0;
	static unsigned char last_wit_type=0;
	static unsigned char last_control_iptv_flow_counter=0;
	
	//dumpWanIntervalTraffic();
	mib_get(MIB_WAN_INTERVAL_TRAFFIC_TIME, &wit_time);
	mib_get(MIB_WAN_INTERVAL_TRAFFIC_SAMPLES, &wit_samples);
	mib_get(MIB_WAN_INTERVAL_TRAFFIC_MODE, &wit_mode);
	mib_get(MIB_WAN_INTERVAL_TRAFFIC_TYPE, &wit_type);
	mib_get(MIB_CONTROL_IPTV_FLOW_COUNTER,(void *)&control_iptv_flow_counter);
	
	if(wit_samples>WAN_INTERVAL_TRAFFIC_SAMPLES_MAX)
		wit_samples = WAN_INTERVAL_TRAFFIC_SAMPLES_MAX;
	if(wit_samples<WAN_INTERVAL_TRAFFIC_SAMPLES_MIN)
		wit_samples = WAN_INTERVAL_TRAFFIC_SAMPLES_MIN;
	if(wit_mode>2)
		wit_mode = 0;
	if(wit_type>1)
		wit_type = 0;
	if((wit_time!=last_wit_time)||(wit_samples!=last_wit_samples)||(wit_mode!=last_wit_mode)||(wit_type!=last_wit_type)||(control_iptv_flow_counter!=last_control_iptv_flow_counter))
	{
		wanIntervalTrafficSamples_idx = 0;
		memset(wanIntervalTrafficSamples,0,sizeof(wanIntervalTrafficSamples));
	}
	if(wit_time&&wit_samples){
		MIB_CE_ATM_VC_T Entry={0};
		char ifname[IFNAMSIZ] = {0};
		int  i = 0;
		int total = mib_chain_total(MIB_ATM_VC_TBL);
		struct net_device_stats nds={0};		
#ifdef CONFIG_RTK_NETIF_EXTRA_STATS
		struct net_device_stats rls={0};
#endif
		int lockfd = 0;
		LOCK_WanIntervalTraffic();
		for(i=0;i<total;i++){
				if(mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				{					
					if ((!wit_type)||(wit_type&&(Entry.applicationtype&X_CT_SRV_INTERNET)))
					{
						if(wanIntervalTrafficSamples[wanIntervalTrafficSamples_idx].isset){
							   wanIntervalTrafficSamples[wanIntervalTrafficSamples_idx].traffics = 0;
							   wanIntervalTrafficSamples[wanIntervalTrafficSamples_idx].isset = 0;
							  }
						ifGetName(Entry.ifIndex,ifname,sizeof(ifname));
						get_net_device_stats(ifname, &nds);
#ifdef CONFIG_RTK_NETIF_EXTRA_STATS
						get_net_device_ext_stats(ifname, &rls);
						switch(wit_mode){								
								case 1:/*down*/				
									wanIntervalTrafficSamples[wanIntervalTrafficSamples_idx].traffics += nds.rx_bytes -(control_iptv_flow_counter?0:rls.rx_mc_bytes);
									break;
								case 2:/*up*/
									wanIntervalTrafficSamples[wanIntervalTrafficSamples_idx].traffics += nds.tx_bytes -(control_iptv_flow_counter?0:rls.tx_mc_bytes);
									break;
								case 0:/*up&down*/
								default:
									wanIntervalTrafficSamples[wanIntervalTrafficSamples_idx].traffics += nds.tx_bytes -(control_iptv_flow_counter?0:rls.tx_mc_bytes);
									wanIntervalTrafficSamples[wanIntervalTrafficSamples_idx].traffics += nds.rx_bytes -(control_iptv_flow_counter?0:rls.rx_mc_bytes);
									break;					
						}
#else
						switch(wit_mode){								
								case 1:/*down*/ 			
									wanIntervalTrafficSamples[wanIntervalTrafficSamples_idx].traffics += nds.rx_bytes;
									break;
								case 2:/*up*/
									wanIntervalTrafficSamples[wanIntervalTrafficSamples_idx].traffics += nds.tx_bytes;
									break;
								case 0:/*up&down*/
								default:
									wanIntervalTrafficSamples[wanIntervalTrafficSamples_idx].traffics += nds.tx_bytes;
									wanIntervalTrafficSamples[wanIntervalTrafficSamples_idx].traffics += nds.rx_bytes;
									break;					
						}
#endif
				}
			}
		}		
		wanIntervalTrafficSamples[wanIntervalTrafficSamples_idx].isset = 1;
		wanIntervalTrafficSamples_idx++;
		wanIntervalTrafficSamples_idx=wanIntervalTrafficSamples_idx%wit_samples;
		UNLOCK_WanIntervalTraffic();
	}
	last_wit_time=wit_time;
	last_wit_samples=wit_samples;
	last_wit_mode=wit_mode;
	last_wit_type=wit_type;
	last_control_iptv_flow_counter=control_iptv_flow_counter;
	TIMEOUT_F((void*)wanIntervalTraffic, 0, wit_time?wit_time:1, wanIntervalTraffic_ch);
	return;
}

void *start_wanIntervalTraffic(void *data)
{
	unsigned char wit_time=0;
	mib_get(MIB_WAN_INTERVAL_TRAFFIC_TIME, &wit_time);
	TIMEOUT_F((void*)wanIntervalTraffic, 0, wit_time?wit_time:1, wanIntervalTraffic_ch);
#ifdef EMBED
	calltimeout();
#endif
}


/* return Bytes/second*/
unsigned long long getWanIntervalTraffic(){
		unsigned char wit_time=0;
		unsigned char wit_samples=0;
		mib_get(MIB_WAN_INTERVAL_TRAFFIC_TIME, &wit_time);
		mib_get(MIB_WAN_INTERVAL_TRAFFIC_SAMPLES, &wit_samples);
		int samples_idx_s = (wanIntervalTrafficSamples_idx+wit_samples-1)%wit_samples;
		unsigned long long total_begin=0;
		unsigned long long total_end=0;
		int wit_isset_valid_samples=0;
		int lockfd = 0;
		if(wit_time&&wit_samples){
			int i = 0;
			int prev = 0;
			LOCK_WanIntervalTraffic_RET((1*1024*1024));
			for(i=samples_idx_s;i>(samples_idx_s-wit_samples);i--){
				int real_i = (i+wit_samples)%wit_samples;
				if(wanIntervalTrafficSamples[real_i].isset){
					if(real_i==samples_idx_s)
						total_begin=total_end=wanIntervalTrafficSamples[real_i].traffics;
					else{
						if(total_end>= wanIntervalTrafficSamples[real_i].traffics){
							total_begin = wanIntervalTrafficSamples[real_i].traffics;
							wit_isset_valid_samples++;							
						}else{
							break;
						}
					}					
				}
			}
			UNLOCK_WanIntervalTraffic();
			//printf("%s %d#############total_end=%llu,total_begin=%llu,wit_isset_valid_samples=%d\n",__FUNCTION__,__LINE__,total_end,total_begin,wit_isset_valid_samples);
			return wit_isset_valid_samples?((total_end-total_begin)/wit_isset_valid_samples)/wit_time:(1*1024*1024);/*1MBytes*/
		}
		return 0;
}

#endif
#ifdef _PRMT_X_CMCC_AUTORESETCONFIG_
struct auto_reset_info_s{
	unsigned char enable;
	unsigned int hour;
	unsigned int minute;
	unsigned int second;

	unsigned int RepeatDay;
	unsigned char forceActive;
	unsigned int ForceFlow;
	time_t last_check;
	//struct auto_reset_info *next;
};
struct auto_reset_info_s auto_reset_info;
//struct auto_reset_info *auto_reset_list = NULL;
void auto_reset_rule_dump(void){
	unsigned char reset=-1;
	mib_get_s(MIB_RS_CMCC_AUTO_RESET_RULE_RESET,(void *)&reset, sizeof(reset));
	printf("===============display CMCC auto reset info======================\n");
	printf("enable=%d\n",auto_reset_info.enable);
	printf("time=%02d:%02d\n",auto_reset_info.hour,auto_reset_info.minute);
	printf("RepeatDay=0x%x\n",auto_reset_info.RepeatDay);
	printf("forceActive=%d\n",auto_reset_info.forceActive);
	printf("ForceFlow=%d kbps\n",auto_reset_info.ForceFlow);
	printf("MIB_RS_CMCC_AUTO_RESET_RULE_RESET=%d\n",reset);
	printf("=================================================================\n");
	return;
}
void auto_reset_rule_change(){
	unsigned char reset=1;
	mib_set(MIB_RS_CMCC_AUTO_RESET_RULE_RESET, (void *)&reset);

	return;
}
void auto_reset_rule_init(void){
	memset(&auto_reset_info, 0 , sizeof(struct auto_reset_info_s));
	unsigned char reset=0;
	char time_str[6];
	time_t t_now;

	mib_get_s(MIB_CMCC_AUTO_RESET_RULE_ENABLE, (void *)&auto_reset_info.enable, sizeof(auto_reset_info.enable));
	mib_get_s(MIB_CMCC_AUTO_RESET_RULE_TIME, (void *)time_str, sizeof(time_str));
	if(sscanf(time_str,"%d:%d",&auto_reset_info.hour,&auto_reset_info.minute)!=2
	|| (auto_reset_info.hour>23)
	|| (auto_reset_info.minute>59))
	{
		fprintf(stderr,"auto reset rule time illegal!!,use 4:00AM by default");
		auto_reset_info.hour = 4;
		auto_reset_info.minute = 0;
	}	
	mib_get_s(MIB_CMCC_AUTO_RESET_RULE_REPEATDAY, (void *)&auto_reset_info.RepeatDay, sizeof(auto_reset_info.RepeatDay));
	mib_get_s(MIB_CMCC_AUTO_RESET_RULE_FORCEACTIVE, (void *)&auto_reset_info.forceActive, sizeof(auto_reset_info.forceActive));
	mib_get_s(MIB_CMCC_AUTO_RESET_RULE_FORCEFLOW, (void *)&auto_reset_info.ForceFlow, sizeof(auto_reset_info.ForceFlow));
	auto_reset_info.last_check = 0;
	mib_set(MIB_RS_CMCC_AUTO_RESET_RULE_RESET, (void *)&reset);
	auto_reset_rule_dump();

	return;
}
int auto_reset_rule_time_check(){
	int ret=0;
	//printf("[%s %d] auto_reset_info reset time:RepeatDay=0x%x,time %02d:%02d,lastcheck=%ld\n",__func__,__LINE__,auto_reset_info.RepeatDay,auto_reset_info.hour, auto_reset_info.minute,auto_reset_info.last_check);
	struct tm p_now;
	time_t t_now;
	struct tm last_check;
	int hour=0,minute=0;
	time(&t_now);
	
	memcpy(&p_now, localtime(&t_now), sizeof(struct tm));
	//printf("[%s %d] current time p->tm_wday=%d %02d:%02d:%02d,timestamp = %ld\n",__func__,__LINE__,p_now.tm_wday,p_now.tm_hour, p_now.tm_min, p_now.tm_sec,t_now);
	
	if(p_now.tm_year < 100)//later than 2000BC
		return ret;
	if((1 << p_now.tm_wday) & auto_reset_info.RepeatDay){
		memcpy(&last_check, localtime(&auto_reset_info.last_check), sizeof(struct tm));
		//printf("[%s %d] last check time last_check->tm_wday=%d %02d:%02d:%02d,timestamp = %ld\n",__func__,__LINE__,last_check.tm_wday,last_check.tm_hour, last_check.tm_min, last_check.tm_sec,auto_reset_info.last_check);
	if(auto_reset_info.last_check != 0 
	&& (last_check.tm_hour < auto_reset_info.hour || (last_check.tm_hour == auto_reset_info.hour && last_check.tm_min < auto_reset_info.minute)) 
	&& (p_now.tm_hour > auto_reset_info.hour || (p_now.tm_hour == auto_reset_info.hour && p_now.tm_min >= auto_reset_info.minute))){
			printf("[%s %d] auto reset time hit!\n",__func__,__LINE__);
			ret = 1;
		}
	}
	auto_reset_info.last_check = t_now;
	return ret;
}
int auto_reset_rule_exec(void){
	int ret;
	
	printf("auto reset execute!!\n");	
	cmd_reboot();
	return ret;
}
void auto_reset_rule_check(void){
	int ret;
	if(auto_reset_info.enable == 0)
		return;
	if(auto_reset_rule_time_check()==0)
		return;
	if(getDialStatus() == 1){
		printf("auto reset skip because DUT is on a call\n");
		return;
	}
	if(auto_reset_info.forceActive == 0){
#ifdef CONFIG_WAN_INTERVAL_TRAFFIC
		unsigned long long current_traffic = getWanIntervalTraffic();
		if(current_traffic*8 > auto_reset_info.ForceFlow * 1024){
			printf("auto reset skip because DUT is on high traffic %llu bps\n",current_traffic*8);
			return;
		}
#endif
	}
	auto_reset_rule_exec();
	return;
}

#endif
#ifdef CONFIG_VIR_BOA_JEMBENCHTEST
int get_jspeed_via_boa(void)
{
	unsigned int config_vir_boa_jembenchtest = 0;
	mib_get_s(MIB_OSGI_VIR_BOA_JSPEED,(void*)&config_vir_boa_jembenchtest, sizeof(config_vir_boa_jembenchtest));

	return config_vir_boa_jembenchtest;
}

void get_jembenchtest_base(JEMBENCHTEST_BASE_Tp pJbase)
{
#if	defined(CONFIG_RTL8277C_SERIES)
	unsigned int config_vir_boa_jembenchtest_force = 0;
	unsigned int chipId, rev, subType;
	rtk_switch_version_get(&chipId, &rev, &subType);
	mib_get_s(MIB_OSGI_VIR_BOA_JSPEED_FORCE,(void*)&config_vir_boa_jembenchtest_force, sizeof(config_vir_boa_jembenchtest_force));
	if(config_vir_boa_jembenchtest_force == 0){
		if(chipId == RTL8277C_CHIP_ID){
			if(subType == RTL8277C_CHIP_SUB_TYPE_RTL9607DQ || subType == RTL8277C_CHIP_SUB_TYPE_RTL9607DM){
				pJbase->mib_Sieve_res = 714225;
				pJbase->mib_BubbleSort_res = 444193;
				pJbase->mib_Kfl_res = 735401;
				pJbase->mib_Lift_res = 2607809;
				pJbase->mib_UdpIp_res = 434372;
				pJbase->mib_Dummy_test_res = 17483;
				pJbase->mib_matrix_multiplication_res = 11546;
				pJbase->mib_NQueens_res =4636;
				pJbase->mib_AES_res =12055;
			}
		}
	}
	else
#endif
	{
		mib_get_s(MIB_OSGI_JEMBENCHTEST_SIEVE, &(pJbase->mib_Sieve_res), sizeof(pJbase->mib_Sieve_res));
		mib_get_s(MIB_OSGI_JEMBENCHTEST_BUBBLESORT, &(pJbase->mib_BubbleSort_res), sizeof(pJbase->mib_BubbleSort_res));
		mib_get_s(MIB_OSGI_JEMBENCHTEST_KFL, &(pJbase->mib_Kfl_res), sizeof(pJbase->mib_Kfl_res));
		mib_get_s(MIB_OSGI_JEMBENCHTEST_LIFT, &(pJbase->mib_Lift_res), sizeof(pJbase->mib_Lift_res));
		mib_get_s(MIB_OSGI_JEMBENCHTEST_UDPIP, &(pJbase->mib_UdpIp_res), sizeof(pJbase->mib_UdpIp_res));
		mib_get_s(MIB_OSGI_JEMBENCHTEST_DUMMYTEST, &(pJbase->mib_Dummy_test_res), sizeof(pJbase->mib_Dummy_test_res));
		mib_get_s(MIB_OSGI_JEMBENCHTEST_MATRIX, &(pJbase->mib_matrix_multiplication_res), sizeof(pJbase->mib_matrix_multiplication_res));
		mib_get_s(MIB_OSGI_JEMBENCHTEST_NQUEENS, &(pJbase->mib_NQueens_res), sizeof(pJbase->mib_NQueens_res));
		mib_get_s(MIB_OSGI_JEMBENCHTEST_AES, &(pJbase->mib_AES_res), sizeof(pJbase->mib_AES_res));
	}
	return;
}
#endif

/*
 *      Web server handler routines for get info and index (getinfo(), getindex())
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *      Authors: Dick Tam	<dicktam@realtek.com.tw>
 *
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
#ifdef EMBED
#include <linux/config.h>
#include <config/autoconf.h>
#else
#include "../../../../include/linux/autoconf.h"
#include "../../../../config/autoconf.h"
#endif

#include "../webs.h"
#include "mib.h"
#include "adsl_drv.h"
#include "utility.h"
//added by xl_yue
#include "../defs.h"
#include "../../port.h"
// Added by davian kuo
#include "multilang.h"
#include "multilang_set.h"

#include "webform.h"

#if defined(CONFIG_ELINKSDK_SUPPORT)
#include "../../../../lib_elinksdk/libelinksdk.h"
#endif

#ifdef WLAN_SUPPORT
#define	DOT11_DATA_RATE_MAX_NUM	48
#define	DOT11_BASIC_BAND_NUM	5
#define	DOT11B_DATA_RATE_NUM	4
#define	DOT11G_A_DATA_RATE_NUM	8
#define	DOT11N_DATA_RATE_NUM	32
#define	DOT11AC_DATA_RATE_NUM	40
#define	DOT11AX_DATA_RATE_NUM	48
#endif

// remote config status flag: 0: disabled, 1: enabled
int g_remoteConfig=0;
int g_remoteAccessPort=51003;

// Added by Mason Yu
extern char suName[MAX_NAME_LEN];
extern char usName[MAX_NAME_LEN];
// Mason Yu on True
extern unsigned char g_login_username[MAX_NAME_LEN];

extern int replace_pw_page;

extern int asp_voip_getInfo(int eid, request * wp, int argc, char **argv);

#ifdef WLAN_SUPPORT
enum{DOT11B=0, DOT11G_A, DOT11N, DOT11AC, DOT11AX};
unsigned int dot_11_data_rate[DOT11_BASIC_BAND_NUM][DOT11_DATA_RATE_MAX_NUM]={0};

int init_rate_list(void){
	int i=0, j=0;
	static int init_ok=0;
	if(init_ok == 0)
	{
		for(i=0; i<DOT11_BASIC_BAND_NUM; i++){
			switch (i) {
				case DOT11B:
					for(j=0; j<DOT11B_DATA_RATE_NUM; j++) //11b supported rate
						dot_11_data_rate[i][j] = 1 << j;
					break;
				case DOT11G_A:
					for(j = 0; j<DOT11G_A_DATA_RATE_NUM; j++) //11g/11a supported rate
						dot_11_data_rate[i][j] = 1 << (j+DOT11B_DATA_RATE_NUM);
					break;
				case DOT11N:
					for(j = 0; j<DOT11N_DATA_RATE_NUM; j++){ //11n supported rate
						if(j<16)// MCS0~MCS15
							dot_11_data_rate[i][j] = 1 << (j+DOT11B_DATA_RATE_NUM+DOT11G_A_DATA_RATE_NUM);
						else // MCS16~MCS31
							dot_11_data_rate[i][j] = (1<<28)+(j-16);
					}
					break;
				case DOT11AC:
					for(j = 0; j<DOT11AC_DATA_RATE_NUM; j++) //11ac supported rate
						dot_11_data_rate[i][j] = (1<<31) + j;
					break;
				case DOT11AX:
					for(j = 0; j<DOT11AX_DATA_RATE_NUM; j++) //11ac supported rate
						dot_11_data_rate[i][j] = (1<<31) + (j+DOT11AC_DATA_RATE_NUM);
					break;
			}
		}
		init_ok = 1;
	}
	return 0;
}

int check_band(unsigned int rate) {
	int i=0, j=0, flag=0, band=0;
	char phy_band=0;
	mib_get_s(MIB_WLAN_PHY_BAND_SELECT,(void *)&phy_band, sizeof(char));
	for(i=0; i<DOT11_BASIC_BAND_NUM; i++){
		for(j=0; j<DOT11_DATA_RATE_MAX_NUM; j++) {
			if(rate == dot_11_data_rate[i][j]){
				flag = 1;
				break;
			}
		}
		if(flag == 1)
			break;
	}
	if(flag == 1){
		switch(i)
		{
			case DOT11B:
				band = BAND_11B;
				break;
			case DOT11G_A:
				if(phy_band == 1) //2.4g
					band = BAND_11G;
				else if(phy_band == 2) //5g
					band = BAND_11A;
				break;
			case DOT11N:
				band = BAND_11N;
				break;
			case DOT11AC:
				band = BAND_5G_11AC;
				break;
			case DOT11AX:
				band = BAND_5G_11AX;
				break;
		}
		return band;
	}
	else
		return 0;
}

#ifdef CONFIG_00R0
struct html_spec_char_t{
	char symbol;
	char rep_str[10];
	unsigned char rep_len;
};
struct html_spec_char_t html_spec_sym[]={
		{'"', "&quot;", 6},
		{0x27, "&#39;", 5}, // '
		{0x5c, "&#92;", 5}, // '\'
		{'<', "&lt;", 4},
		{'>', "&gt;", 4},
		{0x21, "&#33;", 5}, // !
		{0x26, "&amp;", 5}, // &
		{0x5e, "&#94;", 5}, // ^
		{0x7e, "&#126;", 6}, // ~
		{0x2d, "&#45;", 5}, // -
		{0x40, "&#64;", 5}, // @
		{0x23, "&#35;", 5}, // #
		{0x24, "&#36;", 5}, // $
		{0x25, "&#37;", 5}, // %
		{0x2b, "&#43;", 5}, // +
		{0}
};
#endif
void translate_control_code(char *buffer)
{
	char tmpBuf[200], *p1 = buffer, *p2 = tmpBuf;
#ifdef CONFIG_00R0
	int i;
#endif


	while (*p1) {
#ifdef CONFIG_00R0
		i=0;
		while(html_spec_sym[i].symbol){
			if(*p1 == html_spec_sym[i].symbol){
				memcpy(p2, html_spec_sym[i].rep_str, html_spec_sym[i].rep_len);
				p2 += html_spec_sym[i].rep_len;
				break;
			}
			i++;
		}
		if(!html_spec_sym[i].symbol)
			*p2++ = *p1;
#else
		if (*p1 == '"') {
			memcpy(p2, "&quot;", 6);
			p2 += 6;
		}
		else if (*p1 == '\'') { // single quote
			memcpy(p2, "&apos;", 6);
			p2 += 6;
		}
		else if (*p1 == '\\') {
			memcpy(p2, "&#92;", 5); //backslash
			p2 += 5;
		}
		else if (*p1 =='<'){
			memcpy(p2, "&lt;", 4);
			p2 += 4;
		}
		else if (*p1 =='>'){
			memcpy(p2, "&gt;", 4);
			p2 += 4;
		}
		else
			*p2++ = *p1;
#endif
		p1++;
	}
	*p2 = '\0';

	strcpy(buffer, tmpBuf);
}
void translate_web_code(char *buffer)
{
	char tmpBuf[200], *p1 = buffer, *p2 = tmpBuf;


	while (*p1) {
		if (*p1 == '"') {
			strcpy(p2, "\\\"");
			p2 += 2;
		}
		else if (*p1 == '\'') {
			strcpy(p2, "\\'");
			p2 += 2;
		}
		else if (*p1 == '\\') {
			strcpy(p2, "\\\\");
			p2 += 2;
		}
		else
			*p2++ = *p1;
		p1++;
	}
	*p2 = '\0';

	strcpy(buffer, tmpBuf);
}

#endif

// Kaohj
typedef enum {
	INFO_MIB,
	INFO_SYS
} INFO_T;

typedef struct {
	char *cmd;
	INFO_T type;
	int id;
} web_get_cmd;

typedef struct {
	char *cmd;
	int (*handler)(int , request * , int , char **, char *);
} web_custome_cmd;

web_get_cmd get_info_list[] = {
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	{"lan-ip", INFO_SYS, SYS_LAN_IP},
	{"lan-subnet", INFO_SYS, SYS_LAN_MASK},
#else
	{"lan-ip", INFO_MIB, MIB_ADSL_LAN_IP},
	{"lan-subnet", INFO_MIB, MIB_ADSL_LAN_SUBNET},
#endif
	{"lan-ip2", INFO_MIB, MIB_ADSL_LAN_IP2},
	{"lan-subnet2", INFO_MIB, MIB_ADSL_LAN_SUBNET2},
	// Kaohj
	#ifndef DHCPS_POOL_COMPLETE_IP
	{"lan-dhcpRangeStart", INFO_MIB, MIB_ADSL_LAN_CLIENT_START},
	{"lan-dhcpRangeEnd", INFO_MIB, MIB_ADSL_LAN_CLIENT_END},
	#else
	{"lan-dhcpRangeStart", INFO_MIB, MIB_DHCP_POOL_START},
	{"lan-dhcpRangeEnd", INFO_MIB, MIB_DHCP_POOL_END},
	#endif
	{"lan-dhcpSubnetMask", INFO_MIB, MIB_DHCP_SUBNET_MASK},
	{"dhcps-dns1", INFO_MIB, MIB_DHCPS_DNS1},
	{"dhcps-dns2", INFO_MIB, MIB_DHCPS_DNS2},
	{"dhcps-dns3", INFO_MIB, MIB_DHCPS_DNS3},
	{"lan-dhcpLTime", INFO_MIB, MIB_ADSL_LAN_DHCP_LEASE},
	{"lan-dhcpDName", INFO_MIB, MIB_ADSL_LAN_DHCP_DOMAIN},
	{"elan-Mac", INFO_MIB, MIB_ELAN_MAC_ADDR},
	{"wlan-Mac", INFO_MIB, MIB_WLAN_MAC_ADDR},
	{"wan-dns1", INFO_MIB, MIB_ADSL_WAN_DNS1},
	{"wan-dns2", INFO_MIB, MIB_ADSL_WAN_DNS2},
	{"wan-dns3", INFO_MIB, MIB_ADSL_WAN_DNS3},
#ifdef CONFIG_USER_DHCP_SERVER
	{"wan-dhcps", INFO_MIB, MIB_ADSL_WAN_DHCPS},
#endif
#ifdef DMZ
	{"dmzHost", INFO_MIB, MIB_DMZ_IP},
#endif
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	{"snmpSysDescr", INFO_MIB, MIB_SNMP_SYS_DESCR},
	{"snmpSysContact", INFO_MIB, MIB_SNMP_SYS_CONTACT},
	{"snmpSysLocation", INFO_MIB, MIB_SNMP_SYS_LOCATION},
	{"snmpSysObjectID", INFO_MIB, MIB_SNMP_SYS_OID},
	{"snmpTrapIpAddr", INFO_MIB, MIB_SNMP_TRAP_IP},
	{"snmpCommunityRO", INFO_MIB, MIB_SNMP_COMM_RO_ENC},
	{"snmpCommunityRW", INFO_MIB, MIB_SNMP_COMM_RW_ENC},
	{"name", INFO_MIB, MIB_SNMP_SYS_NAME},
#endif
	{"snmpSysName", INFO_MIB, MIB_SNMP_SYS_NAME},
	{"name", INFO_MIB, MIB_SNMP_SYS_NAME},
#ifdef TIME_ZONE
	{"ntpTimeZoneDBIndex", INFO_MIB, MIB_NTP_TIMEZONE_DB_INDEX},
/*ping_zhang:20081217 START:patch from telefonica branch to support WT-107*/
//#ifdef _PRMT_WT107_
	{"ntpServerHost1", INFO_MIB, MIB_NTP_SERVER_HOST1},
	{"ntpServerHost2", INFO_MIB, MIB_NTP_SERVER_HOST2},
	{"ntpServerHost3", INFO_MIB, MIB_NTP_SERVER_HOST3},
//#endif
/*ping_zhang:20081217 END*/
#endif
	{"uptime", INFO_SYS, SYS_UPTIME},
	{"date", INFO_SYS, SYS_DATE},
	{"year", INFO_SYS, SYS_YEAR},
	{"month", INFO_SYS, SYS_MONTH},
	{"day", INFO_SYS, SYS_DAY},
	{"hour", INFO_SYS, SYS_HOUR},
	{"minute", INFO_SYS, SYS_MINUTE},
	{"second", INFO_SYS, SYS_SECOND},
	{"fwVersion", INFO_SYS, SYS_FWVERSION},
	{"hwVersion", INFO_SYS, SYS_HWVERSION},
#ifdef CONFIG_00R0
	{"bootVersion", INFO_SYS, SYS_BOOTLOADER_FWVERSION},
	{"fwVersionSum_1", INFO_SYS, SYS_FWVERSION_SUM_1},
	{"fwVersionSum_2", INFO_SYS, SYS_FWVERSION_SUM_2},
#endif
#ifdef CPU_UTILITY
	{"cpu_util", INFO_SYS, SYS_CPU_UTIL},
#endif
#ifdef MEM_UTILITY
	{"mem_util", INFO_SYS, SYS_MEM_UTIL},
#endif
	{"dhcplan-ip", INFO_SYS, SYS_DHCP_LAN_IP},
	{"dhcplan-subnet", INFO_SYS, SYS_DHCP_LAN_SUBNET},
	{"dslstate", INFO_SYS, SYS_DSL_OPSTATE},
	{"bridge-ageingTime", INFO_MIB, MIB_BRCTL_AGEINGTIME},
#if defined(CONFIG_RTL_8676HWNAT) || defined(CONFIG_USER_VLAN_ON_LAN)
	{"lan1-vid", INFO_SYS, SYS_LAN1_VID},
	{"lan2-vid", INFO_SYS, SYS_LAN2_VID},
	{"lan3-vid", INFO_SYS, SYS_LAN3_VID},
	{"lan4-vid", INFO_SYS, SYS_LAN4_VID},
	{"lan1-status", INFO_SYS, SYS_LAN1_STATUS},
	{"lan2-status", INFO_SYS, SYS_LAN2_STATUS},
	{"lan3-status", INFO_SYS, SYS_LAN3_STATUS},
	{"lan4-status", INFO_SYS, SYS_LAN4_STATUS},
#endif
#ifdef CONFIG_USER_IGMPPROXY
	{"igmp-proxy-itf", INFO_MIB, MIB_IGMP_PROXY_ITF},
#endif
//#ifdef CONFIG_USER_UPNPD
#if defined(CONFIG_USER_UPNPD)||defined(CONFIG_USER_MINIUPNPD)
	{"upnp-ext-itf", INFO_MIB, MIB_UPNP_EXT_ITF},
#endif
#ifdef TIME_ZONE
	{"ntp-ext-itf", INFO_MIB, MIB_NTP_EXT_ITF},
#endif
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_MLDPROXY
	{"mldproxy-ext-itf", INFO_MIB, MIB_MLD_PROXY_EXT_ITF}, 		// Mason Yu. MLD Proxy
#endif
#ifdef DHCPV6_ISC_DHCP_4XX
	{"dhcpv6r-ext-itf", INFO_MIB, MIB_DHCPV6R_UPPER_IFINDEX},
#endif
#endif
#ifdef IP_PASSTHROUGH
	{"ippt-itf", INFO_MIB, MIB_IPPT_ITF},
	{"ippt-lease", INFO_MIB, MIB_IPPT_LEASE},
	{"ippt-lanacc", INFO_MIB, MIB_IPPT_LANACC},
#endif
#ifdef WLAN_SUPPORT
	{"ssid", INFO_SYS, SYS_WLAN_SSID},
	{"channel", INFO_MIB, MIB_WLAN_CHAN_NUM},
	{"chanwid", INFO_MIB, MIB_WLAN_CHANNEL_WIDTH},
	{"fragThreshold", INFO_MIB, MIB_WLAN_FRAG_THRESHOLD},
	{"rtsThreshold", INFO_MIB, MIB_WLAN_RTS_THRESHOLD},
	{"beaconInterval", INFO_MIB, MIB_WLAN_BEACON_INTERVAL},
	{"dtimPeriod", INFO_MIB, MIB_WLAN_DTIM_PERIOD},
	{"wlanDisabled",INFO_SYS,SYS_WLAN_DISABLED},
	{"hidden_ssid",INFO_SYS,SYS_WLAN_HIDDEN_SSID},
	{"pskValue", INFO_SYS, SYS_WLAN_PSKVAL},
#ifdef USER_WEB_WIZARD
	{"pskValue_Wizard", INFO_SYS, SYS_WLAN_PSKVAL_WIZARD},
#endif
	{"WiFiTest", INFO_MIB, MIB_WIFI_TEST},
#ifdef WLAN_1x
	{"rsPort",INFO_SYS,SYS_WLAN_RS_PORT},
	{"rsIp",INFO_SYS,SYS_WLAN_RS_IP},
	{"rsPassword",INFO_SYS,SYS_WLAN_RS_PASSWORD},
	{"enable1X", INFO_SYS,SYS_WLAN_ENABLE_1X},
#endif
	{"wlanMode",INFO_SYS,SYS_WLAN_MODE_VAL},
	{"encrypt",INFO_SYS,SYS_WLAN_ENCRYPT_VAL},
	{"wpa_cipher",INFO_SYS,SYS_WLAN_WPA_CIPHER_SUITE},
	{"wpa2_cipher",INFO_SYS,SYS_WLAN_WPA2_CIPHER_SUITE},
	{"wpaAuth",INFO_SYS,SYS_WLAN_WPA_AUTH},
	{"networkType",INFO_MIB,MIB_WLAN_NETWORK_TYPE},
	{"lockdown_stat",INFO_SYS,SYS_WLAN_WPS_LOCKDOWN},

#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS) // WPS
	{"wscDisable",INFO_SYS,SYS_WSC_DISABLE},
	//{"wscConfig",INFO_MIB,MIB_WSC_CONFIGURED},
	{"wps_auth",INFO_SYS,SYS_WSC_AUTH},
	{"wps_enc",INFO_SYS,SYS_WSC_ENC},
	{"wscLoocalPin", INFO_MIB, MIB_WSC_PIN},
	{"wpsUseVersion", INFO_MIB, MIB_WSC_VERSION},
#endif

#ifdef WLAN_WDS
	{"wlanWdsEnabled",INFO_MIB,MIB_WLAN_WDS_ENABLED},
	{"wdsPskValue",INFO_MIB,MIB_WLAN_WDS_PSK},
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
	{"repeaterEnable", INFO_MIB, MIB_REPEATER_ENABLED1},
	{"repeaterSSID", INFO_MIB, MIB_REPEATER_SSID1},
#endif
#ifdef RTK_MULTI_AP
	{"multi_ap_controller", INFO_MIB, MIB_MAP_CONTROLLER},
	{"map_device_name", INFO_MIB, MIB_MAP_DEVICE_NAME},
#ifdef RTK_MULTI_AP_R2
	{"map_enable_vlan", INFO_MIB, MIB_MAP_ENABLE_VLAN},
	{"map_primary_vid", INFO_MIB, MIB_MAP_PRIMARY_VID},
	{"map_default_secondary_vid", INFO_MIB, MIB_MAP_DEFAULT_SECONDARY_VID},
#endif
#endif
#endif // of WLAN_SUPPORT
	//{ "dnsServer", INFO_SYS, SYS_DNS_SERVER},
#ifndef USE_DEONET
	{"maxmsglen",INFO_MIB,MIB_MAXLOGLEN},
#endif
#ifdef _CWMP_MIB_
#if !defined(CONFIG_TR142_MODULE)
	{"acs-url", INFO_MIB, CWMP_ACS_URL},
	{"acs-username", INFO_MIB, CWMP_ACS_USERNAME},
	{"acs-password", INFO_MIB, CWMP_ACS_PASSWORD},
#endif
	{"inform-interval", INFO_MIB, CWMP_INFORM_INTERVAL},
	{"conreq-name", INFO_MIB, CWMP_CONREQ_USERNAME},
	{"conreq-pw", INFO_MIB, CWMP_CONREQ_PASSWORD},
	{"cert-pw", INFO_MIB, CWMP_CERT_PASSWORD},
	{"conreq-path", INFO_MIB, CWMP_CONREQ_PATH},
	{"conreq-port", INFO_MIB, CWMP_CONREQ_PORT},
	{"tr069_wan_intf", INFO_MIB, CWMP_WAN_INTERFACE},
#endif
#ifdef _TR111_STUN_
	{"stunsvr-addr", INFO_MIB, TR111_STUNSERVERADDR},
	{"stunsvr-port", INFO_MIB, TR111_STUNSERVERPORT},
	{"stunsvr-uname", INFO_MIB, TR111_STUNUSERNAME},
	{"stunsvr-upasswd", INFO_MIB, TR111_STUNPASSWORD},
#endif
#ifdef DOS_SUPPORT
	{"syssynFlood", INFO_MIB, MIB_DOS_SYSSYN_FLOOD},
	{"sysfinFlood", INFO_MIB, MIB_DOS_SYSFIN_FLOOD},
	{"sysudpFlood", INFO_MIB, MIB_DOS_SYSUDP_FLOOD},
	{"sysicmpFlood", INFO_MIB, MIB_DOS_SYSICMP_FLOOD},
	{"pipsynFlood", INFO_MIB, MIB_DOS_PIPSYN_FLOOD},
	{"pipfinFlood", INFO_MIB, MIB_DOS_PIPFIN_FLOOD},
	{"pipudpFlood", INFO_MIB, MIB_DOS_PIPUDP_FLOOD},
	{"pipicmpFlood", INFO_MIB, MIB_DOS_PIPICMP_FLOOD},
	{"blockTime", INFO_MIB, MIB_DOS_BLOCK_TIME},
	{"snmpFlood", INFO_MIB, MIB_DOS_SNMP_FLOOD},
#endif
	{"lan-dhcp-gateway", INFO_MIB, MIB_ADSL_LAN_DHCP_GATEWAY},
#ifdef ADDRESS_MAPPING
#ifndef MULTI_ADDRESS_MAPPING
	{"local-s-ip", INFO_MIB, MIB_LOCAL_START_IP},
	{"local-e-ip", INFO_MIB, MIB_LOCAL_END_IP},
	{"global-s-ip", INFO_MIB, MIB_GLOBAL_START_IP},
	{"global-e-ip", INFO_MIB, MIB_GLOBAL_END_IP},
#endif //!MULTI_ADDRESS_MAPPING
#endif
#ifndef USE_DEONET
#ifdef CONFIG_USER_RTK_SYSLOG
	{"log-level", INFO_MIB, MIB_SYSLOG_LOG_LEVEL},
	{"display-level", INFO_MIB, MIB_SYSLOG_DISPLAY_LEVEL},
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
	{"syslog-mode", INFO_MIB, MIB_SYSLOG_MODE},
	{"syslog-server-ip", INFO_MIB, MIB_SYSLOG_SERVER_IP},
	{"syslog-server-port", INFO_MIB, MIB_SYSLOG_SERVER_PORT},
#endif
#ifdef SEND_LOG
	{"log-server-ip", INFO_MIB, MIB_LOG_SERVER_IP},
	{"log-server-username", INFO_MIB, MIB_LOG_SERVER_NAME},
#endif
#endif
#else
	{"log-level", INFO_MIB, MIB_SYSLOG_LOG_LEVEL},
	{"remote-level", INFO_MIB, MIB_SYSLOG_REMOTE_LEVEL},
	{"syslog-mode", INFO_MIB, MIB_SYSLOG_MODE},
	{"log-line", INFO_MIB, MIB_SYSLOG_LINE},
	{"syslog-server-ip1", INFO_MIB, MIB_SYSLOG_SERVER_IP1},
	{"syslog-server-port1", INFO_MIB, MIB_SYSLOG_SERVER_PORT1},
	{"syslog-server-ip2", INFO_MIB, MIB_SYSLOG_SERVER_IP2},
	{"syslog-server-port2", INFO_MIB, MIB_SYSLOG_SERVER_PORT2},
	{"syslog-server-ip3", INFO_MIB, MIB_SYSLOG_SERVER_IP3},
	{"syslog-server-port3", INFO_MIB, MIB_SYSLOG_SERVER_PORT3},
	{"syslog-server-ip4", INFO_MIB, MIB_SYSLOG_SERVER_IP4},
	{"syslog-server-port4", INFO_MIB, MIB_SYSLOG_SERVER_PORT4},
	{"syslog-server-ip5", INFO_MIB, MIB_SYSLOG_SERVER_IP5},
	{"syslog-server-port5", INFO_MIB, MIB_SYSLOG_SERVER_PORT5},
	{"syslog-server-url", INFO_MIB, MIB_SYSLOG_SERVER_URL},
	{"syslog-server-url-port", INFO_MIB, MIB_SYSLOG_SERVER_URL_PORT},
#endif
	{"hole-server-url", INFO_MIB, MIB_HOLE_PUNCHING_SERVER},
	{"hole-server-url-port", INFO_MIB, MIB_HOLE_PUNCHING_PORT_ENC},
	{"hole-interval", INFO_MIB, MIB_HOLE_PUNCHING_INTERVAL},
    {"ldap_enable", INFO_MIB, MIB_LDAP_SERVER_ENABLE},
    {"ldap_url", INFO_MIB, MIB_LDAP_SERVER_URL},
    {"ldap_file", INFO_MIB, MIB_LDAP_SERVER_FILE},
	{"ldap_relativepath", INFO_MIB, MIB_LDAP_SERVER_RELATIVEPATH},
	{"ldap_upg_mode", INFO_MIB, MIB_LDAP_UPG_RUN_MODE},
	{"ldap_upg_url", INFO_MIB, MIB_LDAP_MANUAL_FIRM_URL},
	{"ldap_upg_file", INFO_MIB, MIB_LDAP_MANUAL_FIRM_FILE},
	{"ldap_upg_relativepath", INFO_MIB, MIB_LDAP_MANUAL_RELATIVEPATH},
	{"ldap_upg_enbl_relativepath", INFO_MIB, MIB_LDAP_MANUAL_ENBL_RELATIVEPATH},
#ifdef USE_DEONET /* autoreboot */
	{"cbEnblAutoRebootStaticConfig", INFO_MIB, MIB_AUTOREBOOT_ENABLE_STATIC_CONFIG},
	{"txAutoRebootUpTime", INFO_MIB, MIB_AUTOREBOOT_UPTIME},
	{"cbAutoWanPortIdle", INFO_MIB, MIB_AUTOREBOOT_WAN_PORT_IDLE},

	{"arSetStartTimeHour", INFO_MIB, MIB_AUTOREBOOT_HR_STARTHOUR},
	{"arSetStartTimeMin", INFO_MIB, MIB_AUTOREBOOT_HR_STARTMIN},
	{"arSetEndTimeHour", INFO_MIB, MIB_AUTOREBOOT_HR_ENDHOUR},
	{"arSetEndTimeMin", INFO_MIB, MIB_AUTOREBOOT_HR_ENDMIN},

	{"txAutoRebootAvrData", INFO_MIB, MIB_AUTOREBOOT_AVR_DATA},
	{"txAutoRebootAvrDataEPG", INFO_MIB, MIB_AUTOREBOOT_AVR_DATA_EPG},

	{"ntp_enabled", INFO_MIB, MIB_AUTOREBOOT_NTP_ENABLE},
	{"ntp_issync", INFO_MIB, MIB_AUTOREBOOT_NTP_ISSYNC},
	{"enblAutoReboot", INFO_MIB, MIB_ENABLE_AUTOREBOOT},
	{"autoRebootTargetTime", INFO_MIB, MIB_AUTOREBOOT_TARGET_TIME},
	{"ldapConfigServer", INFO_MIB, MIB_AUTOREBOOT_LDAP_CONFIG_SERVER},
	{"enblAutoRebootStaticConfig", INFO_MIB, MIB_AUTOREBOOT_ENABLE_STATIC_CONFIG},
	{"autoRebootOnIdelServer", INFO_MIB, MIB_AUTOREBOOT_ON_IDEL_SERVER},

	/* NAT session */
	{"natsessionthreshold", INFO_MIB, MIB_NAT_SESSION_THRESHOLD},

	/* USEAGE */
	{"cpuusagethreshold", INFO_MIB, MIB_SYSMON_CPU_USEAGE_THRESHOLD},
	{"memusagethreshold", INFO_MIB, MIB_SYSMON_MEM_USEAGE_THRESHOLD},
#endif

#ifdef TCP_UDP_CONN_LIMIT
	{"connLimit-tcp", INFO_MIB, MIB_CONNLIMIT_TCP},
	{"connLimit-udp", INFO_MIB, MIB_CONNLIMIT_UDP},
#endif
#ifdef WEB_REDIRECT_BY_MAC
	{"landing-page-time", INFO_MIB, MIB_WEB_REDIR_BY_MAC_INTERVAL},
#endif
	{"super-user", INFO_MIB, MIB_SUSER_NAME},
#ifdef USER_WEB_WIZARD
	{"super-pass", INFO_MIB, MIB_SUSER_PASSWORD},
#endif
	{"normal-user", INFO_MIB, MIB_USER_NAME},
#ifdef DEFAULT_GATEWAY_V2
	{"wan-default-gateway", INFO_MIB, MIB_ADSL_WAN_DGW_IP},
	{"itf-default-gateway", INFO_MIB, MIB_ADSL_WAN_DGW_ITF},
#endif
//ql 20090119
#ifdef IMAGENIO_IPTV_SUPPORT
	{"stb-dns1", INFO_MIB, MIB_IMAGENIO_DNS1},
	{"stb-dns2", INFO_MIB, MIB_IMAGENIO_DNS2},
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
	{"opch-addr", INFO_MIB, MIB_OPCH_ADDRESS},
	{"opch-port", INFO_MIB, MIB_OPCH_PORT},
#endif
/*ping_zhang:20090930 END*/
#endif

#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_RADVD
	{"V6MaxRtrAdvInterval", INFO_MIB, MIB_V6_MAXRTRADVINTERVAL},
	{"V6MinRtrAdvInterval", INFO_MIB, MIB_V6_MINRTRADVINTERVAL},
	{"V6AdvCurHopLimit", INFO_MIB, MIB_V6_ADVCURHOPLIMIT},
	{"V6AdvDefaultLifetime", INFO_MIB, MIB_V6_ADVDEFAULTLIFETIME},
	{"V6AdvReachableTime", INFO_MIB, MIB_V6_ADVREACHABLETIME},
	{"V6AdvRetransTimer", INFO_MIB, MIB_V6_ADVRETRANSTIMER},
	{"V6AdvLinkMTU", INFO_MIB, MIB_V6_ADVLINKMTU},
	{"V6prefix_ip", INFO_MIB, MIB_V6_PREFIX_IP},
	{"V6prefix_len", INFO_MIB, MIB_V6_PREFIX_LEN},
	{"V6ValidLifetime", INFO_MIB, MIB_V6_VALIDLIFETIME},
	{"V6PreferredLifetime", INFO_MIB, MIB_V6_PREFERREDLIFETIME},
	{"V6RDNSS1", INFO_MIB, MIB_V6_RDNSS1},
	{"V6RDNSS2", INFO_MIB, MIB_V6_RDNSS2},
	{"V6ULAPrefixEnable", INFO_MIB, MIB_V6_ULAPREFIX_ENABLE},
	{"V6ULAPrefix", INFO_MIB, MIB_V6_ULAPREFIX},
	{"V6ULAPrefixlen", INFO_MIB, MIB_V6_ULAPREFIX_LEN},
	{"V6ULAPrefixValidLifetime", INFO_MIB, MIB_V6_ULAPREFIX_VALID_TIME},
	{"V6ULAPrefixPreferredLifetime", INFO_MIB, MIB_V6_ULAPREFIX_PREFER_TIME},
#endif

#ifdef DHCPV6_ISC_DHCP_4XX
	{"dhcpv6s_prefix_length", INFO_MIB, MIB_DHCPV6S_PREFIX_LENGTH},
	{"dhcpv6s_range_start", INFO_MIB, MIB_DHCPV6S_RANGE_START},
	{"dhcpv6s_range_end", INFO_MIB, MIB_DHCPV6S_RANGE_END},
#ifdef CONFIG_RTK_DEV_AP
	{"dhcpv6s_min_address", INFO_MIB, MIB_DHCPV6S_MIN_ADDRESS},
	{"dhcpv6s_max_address", INFO_MIB, MIB_DHCPV6S_MAX_ADDRESS},
#endif
	{"dhcpv6s_default_LTime", INFO_MIB, MIB_DHCPV6S_DEFAULT_LEASE},
	{"dhcpv6s_preferred_LTime", INFO_MIB, MIB_DHCPV6S_PREFERRED_LIFETIME},
	{"dhcpv6_mode", INFO_SYS, SYS_DHCPV6_MODE},
	{"dhcpv6s_type", INFO_MIB, MIB_DHCPV6S_TYPE},
	{"dhcpv6_relay_itf", INFO_SYS, SYS_DHCPV6_RELAY_UPPER_ITF},
	{"dhcpv6s_renew_time", INFO_MIB, MIB_DHCPV6S_RENEW_TIME},
	{"dhcpv6s_rebind_time", INFO_MIB, MIB_DHCPV6S_REBIND_TIME},
	{"dhcpv6s_clientID", INFO_MIB, MIB_DHCPV6S_CLIENT_DUID},
#endif
	{"ip6_dns", INFO_SYS, SYS_WAN_DNS},
	{"ip6_ll", INFO_SYS, SYS_LAN_IP6_LL},
	{"ip6_global", INFO_SYS, SYS_LAN_IP6_GLOBAL},
	{"lanipv6LLAaddr", INFO_MIB, MIB_IPV6_LAN_LLA_IP_ADDR},
	{"prefix-mode", INFO_MIB, MIB_PREFIXINFO_PREFIX_MODE},
	{"prefix-delegation-wan-conn-mode", INFO_MIB, MIB_PD_WAN_DELEGATED_MODE},
	{"prefix-delegation-wan-conn", INFO_MIB, MIB_PREFIXINFO_DELEGATED_WANCONN},
	{"dns-mode", INFO_MIB, MIB_LAN_DNSV6_MODE},
	{"dns-wan-conn-mode", INFO_MIB, MIB_DNSV6INFO_WANCONN_MODE},
	{"dns-wan-conn", INFO_MIB, MIB_DNSINFO_WANCONN},
	{"radvd-mode", INFO_MIB, MIB_V6_RADVD_PREFIX_MODE},
	{"radvd-rdnss-mode", INFO_MIB, MIB_V6_RADVD_RDNSS_MODE},
	{"radvd-rdnss-wan-conn-mode", INFO_MIB, MIB_V6_RADVD_RDNSSINFO_WANCONN_MODE},
	{"radvd-rdnss-wan-conn", INFO_MIB, MIB_V6_RADVD_DNSINFO_WANCONN},
#endif // of CONFIG_IPV6

#ifdef CONFIG_RTL_WAPI_SUPPORT
	{ "wapiUcastReKeyType", INFO_MIB, MIB_WLAN_WAPI_UCAST_REKETTYPE},
	{ "wapiUcastTime", INFO_MIB, MIB_WLAN_WAPI_UCAST_TIME},
	{ "wapiUcastPackets", INFO_MIB, MIB_WLAN_WAPI_UCAST_PACKETS},
	{ "wapiMcastReKeyType", INFO_MIB, MIB_WLAN_WAPI_MCAST_REKEYTYPE},
	{ "wapiMcastTime", INFO_MIB, MIB_WLAN_WAPI_MCAST_TIME},
	{ "wapiMcastPackets", INFO_MIB, MIB_WLAN_WAPI_MCAST_PACKETS},
#endif
#ifdef CONFIG_IPV6
	{"wan-dnsv61", INFO_MIB, MIB_ADSL_WAN_DNSV61},
	{"wan-dnsv62", INFO_MIB, MIB_ADSL_WAN_DNSV62},
	{"wan-dnsv63", INFO_MIB, MIB_ADSL_WAN_DNSV63},
#endif
	{"wan_mode", INFO_MIB, MIB_WAN_MODE},
	{"dhcp_port_filter", INFO_MIB, MIB_DHCP_PORT_FILTER},
#ifdef CONFIG_USER_SAMBA
#ifdef CONFIG_USER_NMBD
	{"samba-netbios-name", INFO_MIB, MIB_SAMBA_NETBIOS_NAME},
#endif
	{"samba-server-string", INFO_MIB, MIB_SAMBA_SERVER_STRING},
#endif

#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_AWIFI_SUPPORT)
	{"awifi-ext-if", INFO_MIB, MIB_AWIFI_EXT_ITF},
#endif

	{"loid", INFO_MIB, MIB_LOID},
#ifdef CONFIG_GPON_FEATURE
	{"omci_sw_ver1", INFO_MIB, MIB_OMCI_SW_VER1},
	{"omci_sw_ver2", INFO_MIB, MIB_OMCI_SW_VER2},
	{"omcc_ver", INFO_MIB, MIB_OMCC_VER},
	{"omci_tm_opt", INFO_MIB, MIB_OMCI_TM_OPT},
	{"omci_vendor_id", INFO_MIB, MIB_PON_VENDOR_ID},
#endif
	{"cwmp_manufaturer", INFO_MIB, MIB_HW_CWMP_MANUFACTURER},
	{"cwmp_productclass", INFO_MIB, MIB_HW_CWMP_PRODUCTCLASS},
	{"cwmp_hw_ver", INFO_MIB, MIB_HW_HWVER},
#ifdef CONFIG_USER_Y1731
	{"y1731_mode", INFO_MIB, Y1731_MODE},
	{"y1731_megid", INFO_MIB, Y1731_MEGID},
	{"y1731_myid", INFO_MIB, Y1731_MYID},
	{"y1731_meglevel", INFO_MIB, Y1731_MEGLEVEL},
	{"y1731_loglevel", INFO_MIB, Y1731_LOGLEVEL},
	{"y1731_ccminterval", INFO_MIB, Y1731_CCM_INTERVAL},
	{"y1731_ext_itf", INFO_MIB, Y1731_WAN_INTERFACE},
	{"y1731_ccm_mode", INFO_MIB, Y1731_ENABLE_CCM},
#endif
#ifdef CONFIG_USER_8023AH
	{"efm_8023ah_mode", INFO_MIB, EFM_8023AH_MODE},
	{"efm_8023ah_ext_itf", INFO_MIB, EFM_8023AH_WAN_INTERFACE},
#endif
	{"rtk_manufacturer", INFO_MIB, RTK_DEVID_MANUFACTURER},
	{"rtk_oui", INFO_MIB, RTK_DEVID_OUI},
	{"rtk_productclass", INFO_MIB, RTK_DEVID_PRODUCTCLASS},
	{"rtk_serialno", INFO_MIB, MIB_HW_SERIAL_NUMBER},
#ifdef CONFIG_USER_CWMP_TR069
	{"cwmp_provisioningcode", INFO_MIB, CWMP_PROVISIONINGCODE},
#endif
	{"rtk_specver", INFO_MIB, RTK_DEVINFO_SPECVER},
	{"rtk_swver", INFO_MIB, RTK_DEVINFO_SWVER},
	{"rtk_hwver", INFO_MIB, RTK_DEVINFO_HWVER},
#if defined(CONFIG_GPON_FEATURE)
	{"gpon_sn",INFO_MIB,MIB_GPON_SN},
#endif
	{"elan_mac_addr", INFO_MIB, MIB_ELAN_MAC_ADDR},
#ifdef CONFIG_CT_AWIFI_JITUAN_SMARTWIFI
	{"providerName", INFO_MIB, MIB_DEVICE_NAME},
	{"upgradeURL", INFO_MIB, AWIFI_IMAGE_URL},
	{"reportURL", INFO_MIB, AWIFI_REPORT_URL},
	{"applyID", INFO_MIB, AWIFI_APPLYID},
	{"city", INFO_MIB, AWIFI_CITY},
#endif
	{"wan_nat_mode", INFO_MIB, MIB_WAN_NAT_MODE},
	{"ipv6_mode", INFO_MIB, MIB_V6_IPV6_ENABLE},
	{NULL, 0, 0}
};

#ifdef WLAN_SUPPORT
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)//WPS
static void convert_bin_to_str(unsigned char *bin, int len, char *out)
{
	int i;
	char tmpbuf[10];

	out[0] = '\0';

	for (i=0; i<len; i++) {
		sprintf(tmpbuf, "%02x", bin[i]);
		strcat(out, tmpbuf);
	}
}


static int fnget_wpsKey(int eid, request * wp, int argc, char **argv, char *buffer) {
	unsigned char key, vChar, type;
	int mib_id;
	MIB_CE_MBSSIB_T Entry;

	wlan_getEntry(&Entry, 0);

	vChar = Entry.wsc_enc;
	buffer[0]='\0';
	if (vChar == WSC_ENCRYPT_WEP) {
		unsigned char tmp[100];
		vChar = Entry.wep;
		type = Entry.wepKeyType;
		key = Entry.wepDefaultKey; //default key
		if (vChar == 1) {
			if (key == 0)
				strcpy(tmp, Entry.wep64Key1);
			else if (key == 1)
				strcpy(tmp, Entry.wep64Key2);
			else if (key == 2)
				strcpy(tmp, Entry.wep64Key3);
			else
				strcpy(tmp, Entry.wep64Key4);

			if(type == KEY_ASCII){
				memcpy(buffer, tmp, 5);
				buffer[5] = '\0';
			}else{
				convert_bin_to_str(tmp, 5, buffer);
				buffer[10] = '\0';
			}
		}
		else {
			if (key == 0)
				strcpy(tmp, Entry.wep128Key1);
			else if (key == 1)
				strcpy(tmp, Entry.wep128Key2);
			else if (key == 2)
				strcpy(tmp, Entry.wep128Key3);
			else
				strcpy(tmp, Entry.wep128Key4);

			if(type == KEY_ASCII){
				memcpy(buffer, tmp, 13);
				buffer[13] = '\0';
			}else{
				convert_bin_to_str(tmp, 13, buffer);
				buffer[26] = '\0';
			}
		}
	}
	else {
		if (vChar ==0 || vChar == WSC_ENCRYPT_NONE)
			strcpy(buffer, "N/A");
		else{
			strcpy(buffer, Entry.wscPsk);
#ifdef CONFIG_00R0
			translate_control_code(buffer);
#endif
		}
	}
   	return boaWrite(wp, buffer);
}
#endif
#endif

#ifdef CONFIG_DEV_xDSL
static int fnget_urlWanadsl(int eid, request * wp, int argc, char **argv, char *buffer)
{
	if (strstr(wp->pathname, "web/admin/"))
		return boaWrite(wp, "/admin/wanadsl.asp");
	else
		return boaWrite(wp, "/wanadsl.asp");
	return 0;
}
#endif

web_custome_cmd get_info_custom_list[] = {
	#ifdef WLAN_SUPPORT
	#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)//WPS
	{ "wps_key", fnget_wpsKey },
	#endif
	#endif
	#ifdef CONFIG_DEV_xDSL
	{ "url_wanadsl", fnget_urlWanadsl },
	#endif
	{ NULL, 0 }
};

int getInfo(int eid, request * wp, int argc, char **argv)
{
	char	*name;
	unsigned char buffer[256+1];
	int idx, ret = 0;
	MIB_AUTOREBOOT_T Entry;

	memset(&Entry, 0, sizeof(Entry));

   	if (boaArgs(argc, argv, "%s", &name) < 1) {
   		boaError(wp, 400, "Insufficient args\n");
   		return -1;
   	}

	memset(buffer,0x00,sizeof(buffer));

#ifdef DOS_SUPPORT
#ifdef USE_DEONET
	if (!strcmp(name, "icmpRate")) {
		int rate;

		mib_get_s(MIB_DOS_RATELIMIT_ICMP_FLOOD,  (void *)&rate, sizeof(rate));
		sprintf(buffer, "%d", rate);
		boaWrite(wp, buffer);
		goto NEXTSTEP;
	}
#endif
#endif

#ifdef CONFIG_DEV_xDSL
 	if ( !strncmp(name, "adsl-drv-", 9) ) {
 		getAdslDrvInfo(&name[9], buffer, 64);
		return boaWrite(wp, "%s", buffer);
	}
#endif
#ifdef CONFIG_USER_XDSL_SLAVE
	if ( !strncmp(name, "adsl-slv-drv-", 13) ) {
		getAdslSlvDrvInfo(&name[13], buffer, 64);
		return boaWrite(wp, "%s", buffer);
	}
#endif /*CONFIG_USER_XDSL_SLAVE*/
	/*+++++add by Jack for VoIP project 20/03/07+++++*/
#ifdef VOIP_SUPPORT
	if(!strncmp(name, "voip_", 5)){
		return asp_voip_getInfo(eid, wp, argc, argv);
	}
#endif /*VOIP_SUPPORT*/
	if(!strcmp(name, "login-user")){
#ifdef USE_LOGINWEB_OF_SERVER
		ret = boaWrite(wp, "%s", g_login_username);
#else
		ret = boaWrite(wp, "%s", wp->user);
#endif
		goto NEXTSTEP;
	}

#ifdef FIELD_TRY_SAFE_MODE
#ifdef CONFIG_DEV_xDSL
	if (!strncmp(name, "safemodenote", 12)) {
		SafeModeData vSmd;
		memset((void *)&vSmd, 0, sizeof(vSmd));
		adsl_drv_get(RLCM_GET_SAFEMODE_CTRL, (void *)&vSmd, SAFEMODE_DATA_SIZE);
		boaWrite(wp, "%s", vSmd.SafeModeNote);
	}
#endif
#endif

#if defined(CONFIG_TR142_MODULE) && defined(_CWMP_MIB_)
	unsigned char dynamic_acs_url_selection = 0;
	mib_get_s(CWMP_DYNAMIC_ACS_URL_SELECTION, &dynamic_acs_url_selection, sizeof(dynamic_acs_url_selection));

	if (!strcmp(name, "acs-url")) {
		memset(buffer, 0, sizeof(buffer));

		if (dynamic_acs_url_selection)
		{
			mib_get_s(RS_CWMP_USED_ACS_URL, buffer, sizeof(buffer));
		}

		// If there's no ACS URL is decided, show ACS URL in MIB.
		if(strlen(buffer) == 0)
			mib_get_s(CWMP_ACS_URL, buffer, sizeof(buffer));

		return boaWrite(wp, "%s", buffer);
	}

	if (!strcmp(name, "acs-username"))
	{
		unsigned char from = CWMP_ACS_FROM_NONE;
		char *value = NULL;

		if (dynamic_acs_url_selection)
		{
			mib_get_s(RS_CWMP_USED_ACS_FROM, &from, sizeof(from));
		}

		switch(from)
		{
		case CWMP_ACS_FROM_OMCI:
			mib_get_s(RS_OMCI_ACS_USERNAME, buffer, sizeof(buffer));
			break;
		default:
			mib_get_s(CWMP_ACS_USERNAME, buffer, sizeof(buffer));
			break;
		}

		value = rtk_util_html_encode(buffer);
		if(value)
		{
			ret = boaWrite(wp, "%s", strValToASP(value));
			free(value);
		}

		goto NEXTSTEP;
	}

	if (!strcmp(name, "acs-password"))
	{
		unsigned char from = CWMP_ACS_FROM_NONE;
		char *value = NULL;

		if (dynamic_acs_url_selection)
		{
			mib_get_s(RS_CWMP_USED_ACS_FROM, &from, sizeof(from));
		}

		switch(from)
		{
		case CWMP_ACS_FROM_OMCI:
			mib_get_s(RS_OMCI_ACS_PASSWD, buffer, sizeof(buffer));
			break;
		default:
			mib_get_s(CWMP_ACS_PASSWORD, buffer, sizeof(buffer));
			break;
		}

		value = rtk_util_html_encode(buffer);
		if(value)
		{
			ret = boaWrite(wp, "%s", strValToASP(value));
			free(value);
		}

		goto NEXTSTEP;
	}
#endif

 	for (idx=0; get_info_custom_list[idx].cmd != NULL; idx++) {
 		if (!strcmp(name, get_info_custom_list[idx].cmd)) {
 			return get_info_custom_list[idx].handler(eid, wp, argc, argv, buffer);
 		}
 	}

	for (idx=0; get_info_list[idx].cmd != NULL; idx++) {
		if (!strcmp(name, get_info_list[idx].cmd)) {
			if (get_info_list[idx].type == INFO_MIB) {

            #if !defined(CONFIG_WIFI_SIMPLE_CONFIG) && !defined(WLAN_WPS)
                if (get_info_list[idx].id == MIB_WSC_VERSION) {
                    ret = boaWrite(wp, "%s", "0");
                    goto NEXTSTEP;
                }
            #endif

				if (getMIB2Str(get_info_list[idx].id, buffer)) {
					fprintf(stderr, "failed to get %s\n", name);
					return -1;
				}
			}
			else {
				if (getSYS2Str(get_info_list[idx].id, buffer))
					return -1;
			}

#ifdef USE_DEONET /* sghong, 20230821, AUTOREBOOT */
			if (!strcmp(name, "arSetStartTimeHour") || !strcmp(name, "arSetStartTimeMin") ||
					!strcmp(name, "arSetEndTimeHour") || !strcmp(name, "arSetEndTimeMin")) {

				mib_get_s(MIB_AUTOREBOOT_HR_STARTHOUR, &Entry.autoRebootHRangeStartHour, sizeof(int));
				mib_get_s(MIB_AUTOREBOOT_HR_STARTMIN, &Entry.autoRebootHRangeStartMin, sizeof(int));
				mib_get_s(MIB_AUTOREBOOT_HR_ENDHOUR, &Entry.autoRebootHRangeEndHour, sizeof(int));
				mib_get_s(MIB_AUTOREBOOT_HR_ENDMIN, &Entry.autoRebootHRangeEndMin, sizeof(int));

				if (!strcmp(name, "arSetStartTimeHour")) {
					sprintf(buffer, "%d", Entry.autoRebootHRangeStartHour);
					boaWrite(wp, buffer);
				}
				else if (!strcmp(name, "arSetStartTimeMin")) {
					sprintf(buffer, "%d", Entry.autoRebootHRangeStartMin);
					boaWrite(wp, buffer);
				}
				else if (!strcmp(name, "arSetEndTimeHour")) {
					sprintf(buffer, "%d", Entry.autoRebootHRangeEndHour);
					boaWrite(wp, buffer);
				}
				else if (!strcmp(name, "arSetEndTimeMin")) {
					sprintf(buffer, "%d", Entry.autoRebootHRangeEndMin);
					boaWrite(wp, buffer);
				}
				goto NEXTSTEP;
			}

			if (!strcmp(name, "cbEnblAutoRebootStaticConfig"))
			{
				mib_get_s(MIB_AUTOREBOOT_ENABLE_STATIC_CONFIG, &Entry.enblAutoRebootStaticConfig, sizeof(char));

				printf("\n%s %d staticconfig:%d \n", __func__, __LINE__, Entry.enblAutoRebootStaticConfig);

				if (Entry.enblAutoRebootStaticConfig == 1)
					sprintf(buffer, "%s", "ON");
				else
					sprintf(buffer, "%s", "OFF");
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}
			if (!strcmp(name, "txAutoRebootUpTime"))
			{
				mib_get_s(MIB_AUTOREBOOT_UPTIME, &Entry.autoRebootUpTime, sizeof(int));

				sprintf(buffer, "%d", Entry.autoRebootUpTime);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}
			if (!strcmp(name, "cbAutoWanPortIdle"))
			{
				mib_get_s(MIB_AUTOREBOOT_WAN_PORT_IDLE, &Entry.autoWanPortIdle, sizeof(char));

				sprintf(buffer, "%d", Entry.autoWanPortIdle);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}
			if (!strncmp(name, "cbSelDay", 8))
			{
				mib_get_s(MIB_AUTOREBOOT_AVR_DATA, &Entry.autoRebootDay, sizeof(int));

				printf("\n%s %d autoRebootDay[0x%x] \n", __func__, __LINE__, Entry.autoRebootDay);

				sprintf(buffer, "%d", Entry.autoRebootDay);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}
			if (!strcmp(name, "txAutoRebootAvrData"))
			{
				mib_get_s(MIB_AUTOREBOOT_AVR_DATA, &Entry.autoRebootAvrData, sizeof(int));

				sprintf(buffer, "%d", Entry.autoRebootAvrData);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}
			if (!strcmp(name, "txAutoRebootAvrDataEPG"))
			{
				mib_get_s(MIB_AUTOREBOOT_AVR_DATA_EPG, &Entry.autoRebootAvrDataEPG, sizeof(int));

				sprintf(buffer, "%d", Entry.autoRebootAvrDataEPG);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}

			if (!strcmp(name, "ntp_enabled"))
			{
				unsigned char ntp_en;

				mib_get_s(MIB_NTP_ENABLED, &ntp_en, 1);

				sprintf(buffer, "%d", ntp_en);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}
			if (!strcmp(name, "ntp_issync"))
			{
				unsigned char ntp_issync;

				ntp_issync = deo_ntp_is_sync();

				sprintf(buffer, "%d", ntp_issync);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}
			if (!strcmp(name, "enblAutoReboot"))
			{
				unsigned char en_auto;

				mib_get_s(MIB_ENABLE_AUTOREBOOT, &en_auto, 1);

				sprintf(buffer, "%d", en_auto);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}
			if (!strcmp(name, "autoRebootTargetTime"))
			{
				char target_time[128];

				memset(target_time, 0, sizeof(target_time));
				mib_get_s(MIB_AUTOREBOOT_TARGET_TIME, target_time, 128);

				sprintf(buffer, "%d", target_time);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}
			if (!strcmp(name, "ldapConfigServer"))
			{
				unsigned char ldap;

				mib_get_s(MIB_AUTOREBOOT_LDAP_CONFIG_SERVER, &ldap, 1);

				sprintf(buffer, "%d", ldap);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}
			if (!strcmp(name, "enblAutoRebootStaticConfig"))
			{
				unsigned char enbl_static_cfg;

				mib_get_s(MIB_AUTOREBOOT_ENABLE_STATIC_CONFIG, &enbl_static_cfg, 1);

				sprintf(buffer, "%d", enbl_static_cfg);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}
			if (!strcmp(name, "autoRebootOnIdelServer"))
			{
				unsigned char idle;

				mib_get_s(MIB_AUTOREBOOT_ON_IDEL_SERVER, &idle, 1);

				sprintf(buffer, "%d", idle);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}

			/* NAT session */
			if (!strcmp(name, "natsessionthreshold"))
			{
				unsigned int threshold;

				mib_get_s(MIB_NAT_SESSION_THRESHOLD, &threshold, 4);

				sprintf(buffer, "%d", threshold);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}

			/* useage cpu/memory */
			if (!strcmp(name, "cpuusagethreshold"))
			{
				unsigned int threshold;

				mib_get_s(MIB_SYSMON_CPU_USEAGE_THRESHOLD, &threshold, 4);

				sprintf(buffer, "%d", threshold);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}

			if (!strcmp(name, "memusagethreshold"))
			{
				unsigned int threshold;

				mib_get_s(MIB_SYSMON_MEM_USEAGE_THRESHOLD, &threshold, 4);

				sprintf(buffer, "%d", threshold);
				boaWrite(wp, buffer);
				goto NEXTSTEP;
			}
#endif

			if (!strncmp(name, "map_device_name", 15))
			{
				ret = boaWrite(wp, "%s", strValToASP(buffer));
				goto NEXTSTEP;
			}
			if ((!strncmp(name, "name", 4)) || (!strncmp(name, "ntpServerHost2", 14)))
			{
				ret = boaWrite(wp, "%s", strValToASP(buffer));
				goto NEXTSTEP;
			}
			// Kaohj
			if ((!strncmp(name, "wan-dns", 7))&& !strcmp(buffer, "0.0.0.0")){
				ret = boaWrite(wp, "");
				goto NEXTSTEP;
			}
			else if((!strcmp(name, "conreq-name"))||(!strcmp(name, "conreq-pw"))){
				ret = boaWrite(wp, "%s", strValToASP(buffer));
				goto NEXTSTEP;
			}
			else if (!strcmp(name, "lan-dhcpLTime")) {
				int ltime;

				ltime = atoi(buffer) / 60;

				sprintf(buffer, "%d", ltime);

				ret = boaWrite(wp, "%s", buffer);
				goto NEXTSTEP;
			}else{
				#ifdef WLAN_SUPPORT
				if(!strcmp(name, "ssid")){
					translate_control_code(buffer);
				}
				#endif
				ret = boaWrite(wp, "%s", buffer);
				goto NEXTSTEP;
			}
			//fprintf(stderr, "%s = %s\n", name, buffer);
			//printf("%s = %s\n", name, buffer);
			break;
		}
	}
	return -1;
NEXTSTEP:
	return ret;
}

int addMenuJavaScript( request * wp,int nums,int maxchildrensize)
{
	boaWrite(wp,"<script >\n");
	int i=0;
	boaWrite(wp,"scores = new Array(%d);\n",nums);
	for(i=0;i<nums;i++ )
		boaWrite(wp,"scores[%d]='Submenu%d';\n",i,i);
	boaWrite(wp,"btns = new Array(%d);\n",nums);
	for(i=0;i<nums;i++ )
		boaWrite(wp,"btns[%d]='Btn%d';\n",i,i);
	boaWrite(wp,"\nfunction initIt()\n"
		"{\n\tdivColl = document.all.tags(\"div\");\n"
		"\tfor (i=0; i<divColl.length; i++)\n "
		"\t{\n\t\twhichEl = divColl[i];\n"
		"\t\tif (whichEl.className == \"Child\")\n"
		"\t\t\twhichEl.style.display = \"none\";\n\t}\n}\n\n");
	boaWrite(wp,"function closeMenu(el)\n"
		"{\n"
		"\tfor(i=0;i<%d;i++)\n"
		"\t{\n\t\tfor(j=0;j<%d;j++)"
		"{\n\t\t\tif(scores[i]!=el)\n"
		"\t\t\t{\n\t\t\t\tid=scores[i]+\"Child\"+j.toString();\n"
		"\t\t\t\tif(document.getElementById(id))\n"
		"\t\t\t\t{\n\t\t\t\t\tdocument.getElementById(id).style.display = \"none\";\n"
		"\t\t\t\t\twhichEl = eval(scores[i] + \"Child\");\n"
		"\t\t\t\t\twhichEl.style.display = \"none\";\n"
		"\t\t\t\t\tdocument.getElementById(btns[i]).src =\"menu-images/menu_folder_closed.gif\";\n"
		"\t\t\t\t}\n\t\t\t}\n\t\t}\n\t}\n}\n\n",nums, maxchildrensize);

	boaWrite(wp,"function expandMenu(el,imgs, num)\n"
		"{\n\tcloseMenu(el);\n");
	boaWrite(wp,"\tif (num == 0) {\n\t\twhichEl1 = eval(el + \"Child\");\n"
		"\t\tfor(i=0;i<%d;i++)\n",nums);
	boaWrite(wp,"\t\t{\n\t\t\twhichEl = eval(scores[i] + \"Child\");\n"
		"\t\t\tif(whichEl!=whichEl1)\n "
		"\t\t\t{\n\t\t\t\twhichEl.style.display = \"none\";\n"
		"\t\t\t\tdocument.getElementById(btns[i]).src =\"menu-images/menu_folder_closed.gif\";\n"
		"\t\t\t}\n\t\t}\n");
	boaWrite(wp,"\t\twhichEl1 = eval(el + \"Child\");\n"
		"\t\tif (whichEl1.style.display == \"none\")\n "
		"\t\t{\n"
		"\t\t\twhichEl1.style.display = \"\";\n"
		"\t\t\tdocument.getElementById(imgs).src =\"menu-images/menu_folder_open.gif\";\n"
		"\t\t}\n\t\telse {\n\t\t\twhichEl1.style.display =\"none\";\n"
		"\t\t\tdocument.getElementById(imgs).src =\"menu-images/menu_folder_closed.gif\";\n"
		"\t\t}\n\t}\n\telse {\n");
	boaWrite(wp,"\t\tfor(i=0;i<num;i++) {\n"
		"\t\t\tid = el + \"Child\"+i.toString();\n"
		"\t\t\twhichEl1 = document.getElementById(id);\n"
		"\t\t\tif (whichEl1) {\n"
		"\t\t\t\tif (whichEl1.style.display == \"none\")\n"
		"\t\t\t\t{\n"
		"\t\t\t\t\twhichEl1.style.display = \"\";\n"
		"\t\t\t\t\tdocument.getElementById(imgs).src =\"menu-images/menu_folder_open.gif\";\n"
		"\t\t\t\t}\n\t\t\t\telse {\n\t\t\t\t\twhichEl1.style.display =\"none\";\n"
		"\t\t\t\t\tdocument.getElementById(imgs).src =\"menu-images/menu_folder_closed.gif\";\n"
		"\t\t\t\t}\n\t\t\t}\n\t\t}\n\t}\n}\n</script>\n");

	boaWrite(wp,"<style type=\"text/css\">\n"
		"\n.link {\n"
/* add by yq_zhou 09.2.02 add sagem logo for 11n*/
#ifdef CONFIG_11N_SAGEM_WEB
		"\tfont-family: arial, Helvetica, sans-serif, bold;\n\tfont-size:10pt;\n\twhite-space:nowrap;\n\tcolor: #000000;\n\ttext-decoration: none;\n}\n"
#else
		"\tfont-family: arial, Helvetica, sans-serif, bold;\n\tfont-size:10pt;\n\twhite-space:nowrap;\n\tcolor: #FFFFFF;\n\ttext-decoration: none;\n}\n"
#endif
		"</style>");
	return 0;
}


//modify adsl-stats.asp for adsl and vdsl2
#define DSL_STATS_FMT_UP \
	"<tr>\n" \
	"	<th align=left bgcolor=#c0c0c0><font size=2>%s</th>\n" \
	"	<td bgcolor=#f0f0f0>%s</td>\n" \
	"</tr>\n"
#define DSL_STATS_FMT_DSUS \
	"<tr bgcolor=#f0f0f0>\n" \
	"	<th align=left bgcolor=#c0c0c0><font size=2>%s</th>\n" \
	"	<td>%s</td><td>%s</td>\n" \
	"</tr>\n"
#define DSL_STATS_FMT_DOWN \
	"<tr bgcolor=#f0f0f0>\n" \
	"	<th align=left bgcolor=#c0c0c0><font size=2>%s</th>\n" \
	"	<td colspan=2>%s</td>\n" \
	"</tr>\n"
//end modify adsl-stats.asp for adsl and vdsl2

int check_display(int eid, request * wp, int argc, char **argv)
{
	char *name;

   	if (boaArgs(argc, argv, "%s", &name) < 1) {
   		boaError(wp, 400, "Insufficient args\n");
   		return -1;
   	}

	if(!strcmp(name,"vid_mark"))
	{
#ifdef CONFIG_LUNA
		boaWrite(wp,"none");
#else
		boaWrite(wp,"block");
#endif
		return 0;
	}
	else if(!strcmp(name,"wan_interface"))
	{
#ifdef CONFIG_LUNA
#ifndef CONFIG_RTK_DEV_AP
		boaWrite(wp,"none");
#else
		boaWrite(wp,"block");
#endif
#else
		boaWrite(wp,"block");
#endif
		return 0;
	}
	else if(!strcmp(name,"qos_direction"))
	{
#ifdef CONFIG_LUNA
		boaWrite(wp,"block");
#else
		boaWrite(wp,"none");
#endif
		return 0;
	}
	else if(!strcmp(name,"vlanID"))
	{
		boaWrite(wp,"none");
		return 0;
	}
	else if(!strcmp(name,"ssid"))
	{
		boaWrite(wp,"none");
		return 0;
	}
	return -1;
}

int getReplacePwPage(int eid, request * wp, int argc, char **argv)
{
	boaWrite(wp, "%s", (replace_pw_page)? "Yes" : "No");
	replace_pw_page = 0;
	return 0;
}

#ifdef CONFIG_GENERAL_WEB
int SendWebHeadStr(int eid, request * wp, int argc, char **argv)
{
	boaWrite(wp,"<!DOCTYPE html>\n"
				"<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->\n"
				"<html>\n"
				"<head>\n"
				"<meta http-equiv=\"Content-Type\" content=\"text/html\" charset=\"utf-8\">\n"
				"<meta HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\n"
				"<meta HTTP-equiv=\"Cache-Control\" content=\"no-cache\">\n"
				"<meta name=\"viewport\" content=\"width=device-width\">\n"
				"<link rel=\"stylesheet\" href=\"/admin/bootstrap.min.css\">\n"
				"<link rel=\"stylesheet\" href=\"/admin/reset.css\">\n"
				"<link rel=\"stylesheet\" href=\"/admin/base.css\">\n"
				"<link rel=\"stylesheet\" href=\"/admin/style.css\">\n"
				"<script language=\"javascript\" src=\"common.js\"></script>\n"
				"<script type=\"text/javascript\" src=\"share.js\"></script>\n");
				//,LOGIN_AUTOEXIT_TIME+5);
	return 0;
}
int CheckMenuDisplay(int eid, request * wp, int argc, char **argv)
{
	char *name;
	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	if(!strcmp(name, "pon_settings"))
	{
		unsigned int pon_mode=0;
		if(!mib_get_s(MIB_PON_MODE, (void *)&pon_mode, sizeof(pon_mode)))
			return -1;
#if defined(CONFIG_GPON_FEATURE)
		if(pon_mode == GPON_MODE)
		{
			boaWrite(wp,"{\n"
						"    name: '%s',\n"
						"    href: pageRoot + 'gpon.asp'\n"
						"}\n"
						",",multilang(LANG_GPON_SETTINGS));
		}
#endif
#if defined(CONFIG_EPON_FEATURE)
		if(pon_mode == EPON_MODE)
		{
			boaWrite(wp,"{\n"
						"    name: '%s',\n"
						"    href: pageRoot + 'epon.asp'\n"
						"}\n"
						",",multilang(LANG_EPON_SETTINGS));
		}
#endif
		return 0;
	}
#endif
#if defined(CONFIG_GPON_FEATURE)
	if(!strcmp(name, "omci_info"))
	{
		unsigned int pon_mode=0;
		if(!mib_get_s(MIB_PON_MODE, (void *)&pon_mode, sizeof(pon_mode)))
			return -1;
		if(pon_mode == GPON_MODE){
			boaWrite(wp,"{\n"
						"    name: '%s',\n"
						"    href: pageRoot + 'omci_info.asp'\n"
						"}\n"
						",",multilang(LANG_OMCI_INFO));
		}
		return 0;
	}
#endif
	if(!strcmp(name, "wan_mode"))
	{
		unsigned int wan_mode=0;
#ifdef CONFIG_PTMWAN
		wan_mode ++;
#endif
#ifdef WLAN_WISP
		wan_mode ++;
#endif
#ifdef CONFIG_DEV_xDSL
		wan_mode ++;
#endif
#if defined(CONFIG_PTMWAN) || (WLAN_WISP) || (CONFIG_DEV_xDSL)
		if(wan_mode){
			boaWrite(wp,"{\n"
						"	 name: '%s',\n"
						"	 href: pageRoot + 'wanmode.asp'\n"
						"}\n"
						",",multilang(LANG_WAN_MODE));
		}
#endif
		return 0;
	}
#ifdef RTK_MULTI_AP
	if(!strcmp(name, "map_topology"))
	{
		unsigned char map_controller = 0;
		if(!mib_get_s(MIB_MAP_CONTROLLER, &map_controller, sizeof(map_controller)))
			return -1;
		if(map_controller == 1)
		{
			boaWrite(wp,",\n"
						"{\n"
						"	name: 'Topology',\n"
						"	href: pageRoot + 'multi_ap_setting_topology.asp'\n"
						"}\n");
		}
		return 0;
	}
#ifdef RTK_MULTI_AP_R2
	if(!strcmp(name, "map_channel_scan"))
	{
		unsigned char map_controller = 0;
		if(!mib_get_s(MIB_MAP_CONTROLLER, &map_controller, sizeof(map_controller)))
			return -1;
		if(map_controller == 1)
		{
			boaWrite(wp,",\n"
						"{\n"
						"	name: 'Channel Scan',\n"
						"	href: pageRoot + 'multi_ap_channel_scan.asp'\n"
						"}\n");
		}
		return 0;
	}
	if(!strcmp(name, "map_vlan"))
	{
		unsigned char map_controller = 0;
		if(!mib_get_s(MIB_MAP_CONTROLLER, &map_controller, sizeof(map_controller)))
			return -1;
		if(map_controller == 1)
		{
			boaWrite(wp,",\n"
						"{\n"
						"	name: 'Vlan',\n"
						"	href: pageRoot + 'multi_ap_setting_vlan.asp'\n"
						"}\n"
						",",multilang(LANG_WLAN_EASY_MESH_VLAN_CONFIGURATION));
		}
		return 0;
	}
#endif
#endif
	return 0;
}
#endif

int nvramGet(int eid, request * wp, int argc, char **argv)
{
	char *name, buffer0[64]={0}, buffer1[64]={0};
    char val;

   	if(boaArgs(argc, argv, "%s", &name) < 1) {
   		boaError(wp, 400, "Insufficient args\n");
   		return -1;
   	}
	if( !strcmp(name, "sw_version0") ) {
        get_version_info(buffer0, buffer1);
        boaWrite(wp, "%s", buffer0);
		return 0;
	}

    if( !strcmp(name, "sw_version1") ) {
        get_version_info(buffer0, buffer1);
        boaWrite(wp, "%s", buffer1);
        return 0;
    }

    if( !strcmp(name, "sw_commit0") ) {
		val = get_commit_info();
		if(val < 0)
			boaWrite(wp, "%s", "-");
        else if(val == 0)
            boaWrite(wp, "%s", "O");
        else
            boaWrite(wp, "%s", "X");
        return 0;
    }

    if( !strcmp(name, "sw_commit1") ) {
        val = get_commit_info();
        if(val < 0)
            boaWrite(wp, "%s", "-");
        else if(val == 0)
            boaWrite(wp, "%s", "X");
        else
            boaWrite(wp, "%s", "O");
        return 0;
    }

    if( !strcmp(name, "sw_active0") ) {
        val = get_active_info();
        if(val < 0)
            boaWrite(wp, "%s", "-");
        else if(val == 0)
            boaWrite(wp, "%s", "O");
        else
            boaWrite(wp, "%s", "X");
        return 0;
    }

    if( !strcmp(name, "sw_active1") ) {
        val = get_active_info();
        if(val < 0)
            boaWrite(wp, "%s", "-");
        else if(val == 0)
            boaWrite(wp, "%s", "X");
        else
            boaWrite(wp, "%s", "O");
        return 0;
    }

    if( !strcmp(name, "sw_valid0") ) {
        val = get_valid_info(0);
        if(val < 0)
            boaWrite(wp, "%s", "-");
        else if(val == 0)
            boaWrite(wp, "%s", "X");
        else
            boaWrite(wp, "%s", "O");
        return 0;
    }

    if( !strcmp(name, "sw_valid1") ) {
        val = get_valid_info(1);
        if(val < 0)
            boaWrite(wp, "%s", "-");
        else if(val == 0)
            boaWrite(wp, "%s", "X");
        else
            boaWrite(wp, "%s", "O");
        return 0;
    }
    return -1;
}
// Kaohj
int checkWrite(int eid, request * wp, int argc, char **argv)
{
	char *name, buffer[50]={0};
	unsigned char vChar;
	unsigned short vUShort;
	unsigned int vUInt;
#ifdef CONFIG_USER_MAC_CLONE
	unsigned int intVal;
	unsigned char tmpStr[20];
	memset(tmpStr ,'\0',20);
#endif
#ifdef CONFIG_USER_VLAN_ON_LAN
	MIB_CE_SW_PORT_T sw_entry;
#endif
#ifdef WLAN_SUPPORT
    MIB_CE_MBSSIB_T Wlans_Entry;
#endif

   	if (boaArgs(argc, argv, "%s", &name) < 1) {
   		boaError(wp, 400, "Insufficient args\n");
   		return -1;
   	}
	if ( !strcmp(name, "devType") ) {
		if ( !mib_get_s( MIB_DEVICE_TYPE, (void *)&vChar, sizeof(vChar)) )
			return -1;
#ifdef EMBED
		if (0 == vChar)
			boaWrite(wp, "disableTextField(document.adsl.adslConnectionMode);");
#endif
		return 0;
	}

	// Added by davian kuo
#ifdef CONFIG_USER_BOA_WITH_MULTILANG
	if (!strcmp(name, "selinit")) {
		int i = 0;
		char mStr[MAX_LANGSET_LEN] = {0};

		if (!mib_get_s (MIB_MULTI_LINGUAL, (void *)mStr, sizeof(mStr))) {
			fprintf (stderr, "mib get multi-lingual setting failed!\n");
			return -1;
		}

		for (i = 0; i < LANG_MAX; i++)
			boaWrite(wp, "<option %s value=\"%d\">%s</option>\n",
				(!(strcmp(mStr, lang_set[i].langType)))?"selected":"", i, lang_set[i].langStr);
		return 0;
	}
#endif

#ifdef USE_LOGINWEB_OF_SERVER
	if (!strcmp(name, "loginSelinit")) {
#ifdef CONFIG_USER_BOA_WITH_MULTILANG
		int i = 0;
		char mStr[MAX_LANGSET_LEN] = {0};

		if (!mib_get_s (MIB_MULTI_LINGUAL, (void *)mStr, sizeof(mStr))) {
			fprintf (stderr, "mib get multi-lingual setting (login) failed!\n");
			return -1;
		}

		boaWrite(wp, "<td><div align=right><font color=#0000FF size=2>%s:</font></div></td>\n", multilang(LANG_LANGUAGE_SELECT));
		boaWrite(wp, "<td>&nbsp;&nbsp;</td>\n");
		boaWrite(wp, "<td><font size=2><select size=\"1\" name=\"loginSelinit\" onChange=\"mlhandle();\">\n");
		for (i = 0; i < LANG_MAX; i++)
			boaWrite(wp, "<option %s value=\"%d\">%s</option>\n",
				(!(strcmp(mStr, lang_set[i].langType)))?"selected":"", i, lang_set[i].langStr);
		boaWrite(wp, "</select></font></td>\n");

		return 0;
#endif
	}
#endif

#ifdef CONFIG_USER_ROUTED_ROUTED
	else if ( !strcmp(name, "rip-on-0") ) {
		if ( !mib_get_s( MIB_RIP_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "rip-on-1") ) {
		if ( !mib_get_s( MIB_RIP_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif

#ifdef CONFIG_NET_IPGRE
	else if ( !strcmp(name, "gre-on-0") ) {
		if ( !mib_get_s( MIB_GRE_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "gre-on-1") ) {
		if ( !mib_get_s( MIB_GRE_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif

#ifdef CONFIG_DEV_xDSL
	//modify adsl-set.asp for adsl and vdsl2
	if (!strcmp (name, "adsl_set_title"))
	{
		#ifdef CONFIG_VDSL
		boaWrite (wp, "%s", multilang(LANG_DSL_SETTINGS) );
		#else
		boaWrite (wp, "%s", multilang(LANG_ADSL_SETTINGS) );
		#endif /*CONFIG_VDSL*/
		return 0;
	}
	if (!strcmp (name, "xdsl_type"))
	{
		#ifdef CONFIG_VDSL
		boaWrite (wp, "%s", "DSL" );
		#else
		boaWrite (wp, "%s", "ADSL" );
		#endif /*CONFIG_VDSL*/
		return 0;
	}


	if( !strncmp (name, "adsl_", 5) || !strncmp (name, "adsl_slv_", 9) )
	{
		int ofs;
		XDSL_OP *d;
#ifdef CONFIG_USER_XDSL_SLAVE
		if(!strncmp (name, "adsl_slv_", 9))
		{
			ofs=9;
			d=xdsl_get_op(1);
		}else
#endif /*CONFIG_USER_XDSL_SLAVE*/
		{
			ofs=5;
			d=xdsl_get_op(0);
		}


		//modify adsl-diag.asp for adsl and vdsl2
		if (!strcmp (&name[ofs], "diag_title"))
		{
#ifdef CONFIG_USER_XDSL_SLAVE
			if(d->id)
			{
				#ifdef CONFIG_VDSL
				boaWrite (wp, "%s", multilang(LANG_DSL_SLAVE_TONE_DIAGNOSTICS) );
				#else
				boaWrite (wp, "%s", multilang(LANG_ADSL_SLAVE_TONE_DIAGNOSTICS) );
				#endif /*CONFIG_VDSL*/
			}else
#endif /*CONFIG_USER_XDSL_SLAVE*/
			{
				#ifdef CONFIG_VDSL
				boaWrite (wp, "%s", multilang(LANG_DSL_TONE_DIAGNOSTICS) );
				#else
				boaWrite (wp, "%s", multilang(LANG_ADSL_TONE_DIAGNOSTICS) );
				#endif /*CONFIG_VDSL*/
			}
			return 0;
		}
		if (!strcmp (&name[ofs], "diag_cmt"))
		{
#ifdef CONFIG_USER_XDSL_SLAVE
			if(d->id)
			{
				#ifdef CONFIG_VDSL
				boaWrite (wp, "%s", multilang(LANG_DSL_SLAVE_TONE_DIAGNOSTICS_ONLY_ADSL2_ADSL2_VDSL2_SUPPORT_THIS_FUNCTION) );
				#else
				boaWrite (wp, "%s", multilang(LANG_ADSL_SLAVE_TONE_DIAGNOSTICS_ONLY_ADSL2_2_SUPPORT_THIS_FUNCTION) );
				#endif /*CONFIG_VDSL*/
			}else
#endif /*CONFIG_USER_XDSL_SLAVE*/
			{
				#ifdef CONFIG_VDSL
				boaWrite (wp, "%s", multilang(LANG_DSL_TONE_DIAGNOSTICS_ONLY_ADSL2_ADSL2_VDSL2_SUPPORT_THIS_FUNCTION) );
				#else
				boaWrite (wp, "%s", multilang(LANG_ADSL_TONE_DIAGNOSTICS_ONLY_ADSL2_2_SUPPORT_THIS_FUNCTION) );
				#endif /*CONFIG_VDSL*/
			}
			return 0;
		}
		//modify adsl-stats.asp for adsl and vdsl2
		if (!strcmp (&name[ofs], "title"))
		{
#ifdef CONFIG_USER_XDSL_SLAVE
			if(d->id)
			{
				#ifdef CONFIG_VDSL
				boaWrite (wp, "%s", multilang(LANG_DSL_SLAVE_STATISTICS) );
				#else
				boaWrite (wp, "%s", multilang(LANG_ADSL_SLAVE_STATISTICS) );
				#endif /*CONFIG_VDSL*/
			}else
#endif /*CONFIG_USER_XDSL_SLAVE*/
			{
				#ifdef CONFIG_VDSL
				boaWrite (wp, "%s", multilang(LANG_DSL_STATISTICS) );
				#else
				boaWrite (wp, "%s", multilang(LANG_ADSL_STATISTICS) );
				#endif /*CONFIG_VDSL*/
			}
			return 0;
		}
		if (!strcmp (&name[ofs], "tpstc"))
		{
			#ifdef CONFIG_VDSL
			unsigned char buffer[64];
			d->xdsl_get_info(DSL_GET_TPS, buffer, 64);
			boaWrite (wp, DSL_STATS_FMT_UP, "TPS-TC", buffer );
			#endif /*CONFIG_VDSL*/
			return 0;
		}
		if (!strcmp (&name[ofs], "trellis"))
		{
			#ifndef CONFIG_VDSL
			unsigned char buffer[64];
			d->xdsl_get_info(ADSL_GET_TRELLIS, buffer, 64);
			boaWrite (wp, DSL_STATS_FMT_UP, "Trellis Coding", buffer );
			#endif /*CONFIG_VDSL*/
			return 0;
		}
		if (!strcmp (&name[ofs], "trellis_dsus"))
		{
			#ifdef CONFIG_VDSL
			unsigned char buf_ds[64],buf_us[64];
			d->xdsl_get_info(DSL_GET_TRELLIS_DS, buf_ds, 64);
			d->xdsl_get_info(DSL_GET_TRELLIS_US, buf_us, 64);
			boaWrite (wp, DSL_STATS_FMT_DSUS, "Trellis", buf_ds, buf_us );
			#endif /*CONFIG_VDSL*/
			return 0;
		}
		if (!strcmp (&name[ofs], "ginp_dsus"))
		{
			#ifdef CONFIG_VDSL
			unsigned char buf_ds[64],buf_us[64];
			d->xdsl_get_info(DSL_GET_PHYR_DS, buf_ds, 64);
			d->xdsl_get_info(DSL_GET_PHYR_US, buf_us, 64);
			boaWrite (wp, DSL_STATS_FMT_DSUS, "G.INP", buf_ds, buf_us );
			#endif /*CONFIG_VDSL*/
			return 0;
		}
		if (!strcmp (&name[ofs], "k_dsus"))
		{
			#ifndef CONFIG_VDSL
			unsigned char buf_ds[64],buf_us[64];
			d->xdsl_get_info(ADSL_GET_K_DS, buf_ds, 64);
			d->xdsl_get_info(ADSL_GET_K_US, buf_us, 64);
			boaWrite (wp, DSL_STATS_FMT_DSUS, "K (number of bytes in DMT frame)", buf_ds, buf_us );
			#endif /*CONFIG_VDSL*/
			return 0;
		}
		if (!strcmp (&name[ofs], "n_dsus"))
		{
			#ifdef CONFIG_VDSL
			unsigned char buf_ds[64],buf_us[64];
			d->xdsl_get_info(DSL_GET_N_DS, buf_ds, 64);
			d->xdsl_get_info(DSL_GET_N_US, buf_us, 64);
			boaWrite (wp, DSL_STATS_FMT_DSUS, "N (RS codeword size)", buf_ds, buf_us );
			#endif /*CONFIG_VDSL*/
			return 0;
		}
		if (!strcmp (&name[ofs], "l_dsus"))
		{
			#ifdef CONFIG_VDSL
			unsigned char buf_ds[64],buf_us[64];
			d->xdsl_get_info(DSL_GET_L_DS, buf_ds, 64);
			d->xdsl_get_info(DSL_GET_L_US, buf_us, 64);
			boaWrite (wp, DSL_STATS_FMT_DSUS, "L (number of bits in DMT frame)", buf_ds, buf_us );
			#endif /*CONFIG_VDSL*/
			return 0;
		}
		if (!strcmp (&name[ofs], "inp_dsus"))
		{
			#ifdef CONFIG_VDSL
			unsigned char buf_ds[64],buf_us[64];
			d->xdsl_get_info(DSL_GET_INP_DS, buf_ds, 64);
			d->xdsl_get_info(DSL_GET_INP_US, buf_us, 64);
			boaWrite (wp, DSL_STATS_FMT_DSUS, "INP (DMT frame)", buf_ds, buf_us );
			#endif /*CONFIG_VDSL*/
			return 0;
		}
		if (!strcmp (&name[ofs], "fec_name"))
		{
			#ifdef CONFIG_VDSL
			boaWrite (wp, "FEC errors");
			#else
			boaWrite (wp, "FEC");
			#endif /*CONFIG_VDSL*/
			return 0;
		}
		if (!strcmp (&name[ofs], "ohframe_dsus"))
		{
			#ifdef CONFIG_VDSL
			unsigned char buf_ds[64],buf_us[64];
			d->xdsl_get_info(ADSL_GET_RX_FRAMES, buf_ds, 64);
			d->xdsl_get_info(ADSL_GET_TX_FRAMES, buf_us, 64);
			boaWrite (wp, DSL_STATS_FMT_DSUS, "OH Frame", buf_ds, buf_us );
			#endif /*CONFIG_VDSL*/
			return 0;
		}
		if (!strcmp (&name[ofs], "crc_name"))
		{
			#ifdef CONFIG_VDSL
			boaWrite (wp, "OH Frame errors");
			#else
			boaWrite (wp, "CRC");
			#endif /*CONFIG_VDSL*/
			return 0;
		}
		if (!strcmp (&name[ofs], "llr_dsus"))
		{
			#ifdef CONFIG_VDSL
			unsigned char buf_ds[64],buf_us[64];
			d->xdsl_get_info(ADSL_GET_LAST_LINK_DS, buf_ds, 64);
			d->xdsl_get_info(ADSL_GET_LAST_LINK_US, buf_us, 64);
			boaWrite (wp, DSL_STATS_FMT_DSUS, "Last Link Rate", buf_ds, buf_us );
			#endif /*CONFIG_VDSL*/
			return 0;
		}
		if (!strcmp (&name[ofs], "llr"))
		{
			#ifndef CONFIG_VDSL
			unsigned char buffer[64];
			d->xdsl_get_info(ADSL_GET_LAST_LINK_DS, buffer, 64);
			boaWrite (wp, DSL_STATS_FMT_DOWN, "Last Link DS Rate", buffer );
			d->xdsl_get_info(ADSL_GET_LAST_LINK_US, buffer, 64);
			boaWrite (wp, DSL_STATS_FMT_DOWN, "Last Link US Rate", buffer );
			#endif /*CONFIG_VDSL*/
			return 0;
		}
		if (!strcmp (&name[ofs], "txrx_frame"))
		{
			#ifndef CONFIG_VDSL
			unsigned char buffer[64];
			d->xdsl_get_info(ADSL_GET_TX_FRAMES, buffer, 64);
			boaWrite (wp, DSL_STATS_FMT_DOWN, "TX frames", buffer );
			d->xdsl_get_info(ADSL_GET_RX_FRAMES, buffer, 64);
			boaWrite (wp, DSL_STATS_FMT_DOWN, "RX frames", buffer );
			#endif /*CONFIG_VDSL*/
			return 0;
		}
		//end modify adsl-stats.asp for adsl and vdsl2
	}
#endif // of CONFIG_DEV_xDSL

	if ( !strcmp(name, "dhcpMode") ) {
		boaWrite(wp, "<input type=\"radio\" name=dhcpdenable value=%d onClick=\"disabledhcpd()\">%s&nbsp;&nbsp;\n", DHCPV4_LAN_NONE, multilang(LANG_NONE));
		/*
		boaWrite(wp, "<input type=\"radio\" name=dhcpdenable value=%d onClick=\"enabledhcprelay()\">DHCP %s&nbsp;&nbsp;\n", DHCPV4_LAN_RELAY, multilang(LANG_RELAY));
		*/
		boaWrite(wp, "<input type=\"radio\" name=dhcpdenable value=%d onClick=\"enabledhcpd()\">DHCP %s&nbsp;&nbsp;\n", DHCPV4_LAN_SERVER, multilang(LANG_SERVER));
#ifdef CONFIG_USER_DHCPCLIENT_MODE
		/*
		boaWrite(wp, "<input type=\"radio\" name=dhcpdenable value=%d onClick=\"enabledhcpc()\">DHCP %s&nbsp;&nbsp;\n", DHCPV4_LAN_CLIENT, multilang(LANG_CLIENT));
		*/
#endif
		return 0;
	}

	if ( !strcmp(name, "InternetMode") ) {
		boaWrite(wp, "<input type=\"radio\" name=opmode value=0 id='opmode' onClick=\"enabledgateway()\">%s&nbsp;&nbsp;\n", multilang(LANG_GATEWAY));
		boaWrite(wp, "<input type=\"radio\"name=opmode value=1 id='opmode' onClick=\"enabledbridge()\">%s&nbsp;&nbsp;\n", multilang(LANG_BRIDGE));
#if defined(WLAN_UNIVERSAL_REPEATER) && defined(CONFIG_USER_RTK_REPEATER_MODE)
		boaWrite(wp, "<input type=\"radio\"name=opmode value=3 id='opmode' onClick=\"enabledrepeater()\">%s&nbsp;&nbsp;\n", multilang(LANG_REPEATER));
#endif
		return 0;
		}
	if(!strcmp(name, "SelectionWanPort")){
		boaWrite(wp, "<input type=\"radio\" name=\"wan_port_sync\" id=\"sync_yes\"value=\"1\" onClick=\"wan_port_set_func(1)\">%s&nbsp;&nbsp;\n", multilang(LANG_AUTO_SELECTION_WAN_PORT));
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
		boaWrite(wp, "<input type=\"radio\" name=\"wan_port_sync\" id=\"sync_no\" value=\"0\" onClick=\"wan_port_set_func(0)\">%s&nbsp;&nbsp;\n", multilang(LANG_MANUALLY_SELECTION_WAN_PORT));
#else
		boaWrite(wp, "<input type=\"radio\" name=\"wan_port_sync\" id=\"sync_no\" value=\"0\" onClick=\"wan_port_set_func(0)\">%s&nbsp;&nbsp;\n", multilang(LANG_FIXED_WAN_PORT));
#endif

	}

#ifdef DHCPV6_ISC_DHCP_4XX
	if ( !strcmp(name, "dhcpV6Mode") ) {
		boaWrite(wp, "<input type=\"radio\" name=dhcpdenable value=%d onClick=\"disabledhcpd()\">%s&nbsp;\n", DHCP_LAN_NONE, multilang(LANG_NONE));
		boaWrite(wp, "<input type=\"radio\"name=dhcpdenable value=%d onClick=\"enabledhcprelay()\">DHCP%s&nbsp;\n", DHCP_LAN_RELAY, multilang(LANG_RELAY));
		boaWrite(wp, "<input type=\"radio\"name=dhcpdenable value=%d onClick=\"updateDhcpv6Type(1)\">DHCP%s&nbsp;\n", DHCP_LAN_SERVER, multilang(LANG_SERVER));
		return 0;
	}

	if ( !strcmp(name, "DHCPV6S_TYPE") ) {
		boaWrite(wp, "<input type=\"radio\"name=dhcpdv6Type value=%d onClick=\"autodhcpd()\">%s&nbsp;\n", DHCPV6S_TYPE_AUTO, multilang(LANG_AUTO));
		boaWrite(wp, "<input type=\"radio\"name=dhcpdv6Type value=%d onClick=\"enabledhcpd()\">%s&nbsp;\n", DHCPV6S_TYPE_STATIC, multilang(LANG_MANUAL));
		boaWrite(wp, "<input style=\"display:none\"  type=\"radio\"name=dhcpdv6Type value=%d>\n", DHCPV6S_TYPE_AUTO_DNS_ONLY);
#ifdef CONFIG_USER_RTK_RA_DELEGATION
		boaWrite(wp, "<input type=\"radio\"name=dhcpdv6Type value=%d onClick=\"radhcpd()\">%s&nbsp;\n", DHCPV6S_TYPE_RA_DELEGATION, multilang(LANG_RA_DELEGATION));
#else
		boaWrite(wp, "<input style=\"display:none\" type=\"radio\"name=dhcpdv6Type value=%d onClick=\"radhcpd()\">\n", DHCPV6S_TYPE_RA_DELEGATION);
#endif
		return 0;
	}
#endif /* DHCPV6_ISC_DHCP_4XX */

#ifdef ADDRESS_MAPPING
#ifndef MULTI_ADDRESS_MAPPING
	if ( !strcmp(name, "addressMapType") ) {
 		if ( !mib_get_s( MIB_ADDRESS_MAP_TYPE, (void *)&vChar, sizeof(vChar)) )
			return -1;

		boaWrite(wp, "<option value=0>%s</option>\n", multilang(LANG_NONE));
		boaWrite(wp, "<option value=1>%s</option>\n", multilang(LANG_ONE_TO_ONE));
		boaWrite(wp, "<option value=2>%s</option>\n", multilang(LANG_MANY_TO_ONE));
		boaWrite(wp, "<option value=3>%s</option>\n", multilang(LANG_MANY_TO_MANY_OVERLOAD));

		// Mason Yu on True
		boaWrite(wp, "<option value=4>%s</option>\n", multilang(LANG_ONE_TO_MANY));
		return 0;
	}
#endif	// end of !MULTI_ADDRESS_MAPPING
#endif

#ifdef WLAN_SUPPORT
#ifdef WLAN_ACL
	if (!strcmp(name, "wlanAcNum")) {
		MIB_CE_WLAN_AC_T entry;
		int i;
		vUInt = mib_chain_total(MIB_WLAN_AC_TBL);
		for (i=0; i<vUInt; i++) {
			if (!mib_chain_get(MIB_WLAN_AC_TBL, i, (void *)&entry)) {
				i = vUInt;
				break;
			}
			if(entry.wlanIdx == wlan_idx)
				break;
		}
		if (i == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
#endif
#ifdef WLAN_WDS
	if (!strcmp(name, "wlanWDSNum")) {
		vUInt = mib_chain_total(MIB_WDS_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
	if ( !strcmp(name, "wdsEncrypt")) {
		int i;
		const char* encryp[5] = {multilang(LANG_NONE), "WEP 64bits", "WEP 128bits", "WPA (TKIP)", "WPA2 (AES)"};
		if ( !mib_get_s( MIB_WLAN_WDS_ENCRYPT, (void *)&vChar, sizeof(vChar)) )
			return -1;
	       for(i=0;i<5;i++){
		   	if(i == WDS_ENCRYPT_TKIP){//skip tkip....by ap team's web
				continue;
		   	}
#ifdef WLAN_QTN
#ifdef WLAN1_QTN
			if(wlan_idx==1)
#else
			if(wlan_idx==0)
#endif
			if((i!=WDS_ENCRYPT_DISABLED) && (i!=WDS_ENCRYPT_AES))
				continue;
#endif
		   	boaWrite(wp, "<option %s value=\"%d\">%s</option>\n", (i==vChar)?"selected":"", i, encryp[i]);
	       }
		return 0;
	}
	if ( !strcmp(name, "wdsWepFormat")) {
		int i;
		unsigned char tmp;
		char* format[4] = {"ASCII (5 characters)", "Hex (10 characters)", "ASCII (13 characters)", "Hex (26 characters)"};
		int skip = 0;
		if ( !mib_get_s( MIB_WLAN_WDS_ENCRYPT, (void *)&tmp, sizeof(tmp)) )
			return -1;
		if(tmp == WDS_ENCRYPT_WEP128){
			skip = 2;
		}
		if ( !mib_get_s( MIB_WLAN_WDS_WEP_FORMAT, (void *)&vChar, sizeof(vChar)) )
			return -1;
		for(i=0;i<2;i++){
		   	boaWrite(wp, "<option %s value=\"%d\">%s</option>\n", (i==vChar)?"selected":"", i, format[i + skip]);
	       }
		return 0;
	}
	if ( !strcmp(name, "wdsPskFormat")) {
		int i;
		char* format[2] = {"Passphrase", "Hex (64 characters)"};
		if ( !mib_get_s( MIB_WLAN_WDS_PSK_FORMAT, (void *)&vChar, sizeof(vChar)) )
			return -1;
		for(i=0;i<2;i++){
#ifdef WLAN_QTN
#ifdef WLAN1_QTN
			if(wlan_idx==1)
#else
			if(wlan_idx==0)
#endif
			if(i!=KEY_HEX)
				continue;
#endif
		   	boaWrite(wp, "<option %s value=\"%d\">%s</option>\n", (i==vChar)?"selected":"", i, format[i]);
	       }
		return 0;
	}
#endif
	if ( !strcmp(name, "wlmode") ) {
		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		vChar = Entry.wlanMode;
		if (vChar == AP_MODE) {
			boaWrite(wp, "<option selected value=\"0\">AP</option>\n" );
#ifdef WLAN_CLIENT
			boaWrite(wp, "<option value=\"1\">Client</option>\n" );
#endif
#ifdef WLAN_WDS
			boaWrite(wp, "<option value=\"3\">AP+WDS</option>\n" );
#endif
#ifdef WLAN_MESH
			boaWrite(wp, "<option value=\"4\">AP+MESH</option>\n" );
			boaWrite(wp, "<option value=\"5\">MESH</option>\n" );
#endif
		}
#ifdef WLAN_CLIENT
		if (vChar == CLIENT_MODE) {
			boaWrite(wp, "<option value=\"0\">AP</option>\n" );
			boaWrite(wp, "<option selected value=\"1\">Client</option>\n" );
#ifdef WLAN_WDS

			boaWrite(wp, "<option value=\"3\">AP+WDS</option>\n" );
#endif
#ifdef WLAN_MESH
			boaWrite(wp, "<option value=\"4\">AP+MESH</option>\n" );
			boaWrite(wp, "<option value=\"5\">MESH</option>\n" );
#endif
		}
#endif
#ifdef WLAN_WDS
		if (vChar == AP_WDS_MODE) {
			boaWrite(wp, "<option value=\"0\">AP</option>\n" );
#ifdef WLAN_CLIENT
			boaWrite(wp, "<option value=\"1\">Client</option>\n" );
#endif
			boaWrite(wp, "<option selected value=\"3\">AP+WDS</option>\n" );
#ifdef WLAN_MESH
			boaWrite(wp, "<option value=\"4\">AP+MESH</option>\n" );
			boaWrite(wp, "<option value=\"5\">MESH</option>\n" );
#endif
		}
#endif
#ifdef WLAN_MESH
		if (vChar == AP_MESH_MODE) {
			boaWrite(wp, "<option value=\"0\">AP</option>\n" );
#ifdef WLAN_CLIENT
			boaWrite(wp, "<option value=\"1\">Client</option>\n" );
#endif
#ifdef WLAN_WDS
			boaWrite(wp, "<option value=\"3\">AP+WDS</option>\n" );
#endif
			boaWrite(wp, "<option selected value=\"4\">AP+MESH</option>\n" );
			boaWrite(wp, "<option value=\"5\">MESH</option>\n" );
		}
#endif
#ifdef WLAN_MESH
		if (vChar == MESH_MODE) {
			boaWrite(wp, "<option value=\"0\">AP</option>\n" );
#ifdef WLAN_CLIENT
			boaWrite(wp, "<option value=\"1\">Client</option>\n" );
#endif
#ifdef WLAN_WDS
			boaWrite(wp, "<option value=\"3\">AP+WDS</option>\n" );
#endif
			boaWrite(wp, "<option value=\"4\">AP+MESH</option>\n" );
			boaWrite(wp, "<option selected value=\"5\">MESH</option>\n" );
		}
#endif
		return 0;
	}
#ifdef WLAN_WDS
	if ( !strcmp(name, "wlanWdsEnabled") ) {
		if ( !mib_get_s( MIB_WLAN_WDS_ENABLED, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}

#endif
	if ( !strcmp(name, "mband") ) {
#ifdef WLAN_BAND_CONFIG_MBSSID
		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		boaWrite(wp, "%d", Entry.wlanBand);
#else
		unsigned char wlband = 0;
		if(!mib_get_s(MIB_WLAN_BAND, (void *)&wlband, sizeof(wlband)))
			return -1;
		boaWrite(wp, "%d", wlband);
#endif
		return 0;
	}
	if ( !strcmp(name, "band") ) {
#ifdef WLAN_BAND_CONFIG_MBSSID
		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		boaWrite(wp, "%d", Entry.wlanBand-1);
#else
		unsigned char wlband = 0;
		if(!mib_get_s(MIB_WLAN_BAND, (void *)&wlband, sizeof(wlband)))
			return -1;
		boaWrite(wp, "%d", wlband-1);
#endif
		return 0;
	}
	if ( !strcmp(name, "wlband") ) {
		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
#ifdef WIFI_TEST
		boaWrite(wp, "<option value=3>WiFi-G</option>\n" );
		boaWrite(wp, "<option value=4>WiFi-BG</option>\n" );
#endif

#if defined(WLAN_DUALBAND_CONCURRENT)
		if ( !mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar)) )
			return -1;

		if(vChar == PHYBAND_5G)
		{
#ifdef WLAN_QTN
			boaWrite(wp, "<option value=3>5 GHz (A)</option>\n" );
			boaWrite(wp, "<option value=11>5 GHz (A+N)</option>\n" );
			boaWrite(wp, "<option value=63>5 GHz (AC)</option>\n" );
#else
#if defined(WLAN_DENY_LEGACY)
			boaWrite(wp, "<option value=3>5 GHz (A)</option>\n" );
			boaWrite(wp, "<option value=7>5 GHz (N)</option>\n" );
			boaWrite(wp, "<option value=11>5 GHz (A+N)</option>\n" );
#if defined (WLAN0_5G_11AC_SUPPORT) || defined(WLAN1_5G_11AC_SUPPORT)
			boaWrite(wp, "<option value=63>5 GHz (AC)</option>\n" );
			boaWrite(wp, "<option value=71>5 GHz (N+AC)</option>\n" );
			boaWrite(wp, "<option value=75>5 GHz (A+N+AC)</option>\n" );
#endif
#ifdef WLAN_11AX
			boaWrite(wp, "<option value=127>5 GHz (AX)</option>\n" );
			boaWrite(wp, "<option value=191>5 GHz (AC+AX)</option>\n" );
			boaWrite(wp, "<option value=199>5 GHz (N+AC+AX)</option>\n" );
			boaWrite(wp, "<option value=203>5 GHz (A+N+AC+AX)</option>\n" );
#endif
#else
			boaWrite(wp, "<option value=3>5 GHz (A)</option>\n" );
			boaWrite(wp, "<option value=11>5 GHz (A+N)</option>\n" );
#if defined (WLAN0_5G_11AC_SUPPORT) || defined(WLAN1_5G_11AC_SUPPORT)
			boaWrite(wp, "<option value=75>5 GHz (A+N+AC)</option>\n" );
#endif
#ifdef WLAN_11AX
#ifdef WIFI5_WIFI6_COMP
			if(Entry.is_ax_support)
#endif
				boaWrite(wp, "<option value=203>5 GHz (A+N+AC+AX)</option>\n" );
#endif
#endif //WLAN_DENY_LEGACY
#endif //WLAN_QTN
		}
#endif

#if defined(WLAN_DENY_LEGACY)
#if (defined (WLAN0_5G_SUPPORT) || defined(WLAN1_5G_SUPPORT)) && !defined(WLAN_DUALBAND_CONCURRENT)
		boaWrite(wp, "<option value=3>5 GHz (A)</option>\n" );
		boaWrite(wp, "<option value=7>5 GHz (N)</option>\n" );
		boaWrite(wp, "<option value=11>5 GHz (A+N)</option>\n" );
#if defined (WLAN0_5G_11AC_SUPPORT) || defined(WLAN1_5G_11AC_SUPPORT)
		boaWrite(wp, "<option value=63>5 GHz (AC)</option>\n" );
		boaWrite(wp, "<option value=71>5 GHz (N+AC)</option>\n" );
		boaWrite(wp, "<option value=75>5 GHz (A+N+AC)</option>\n" );
#endif
#ifdef WLAN_11AX
		boaWrite(wp, "<option value=127>5 GHz (AX)</option>\n" );
		boaWrite(wp, "<option value=191>5 GHz (AC+AX)</option>\n" );
		boaWrite(wp, "<option value=199>5 GHz (N+AC+AX)</option>\n" );
		boaWrite(wp, "<option value=203>5 GHz (A+N+AC+AX)</option>\n" );
#endif
#endif
#else
#if (defined (WLAN0_5G_SUPPORT) || defined(WLAN1_5G_SUPPORT)) && !defined(WLAN_DUALBAND_CONCURRENT)
		boaWrite(wp, "<option value=3>5 GHz (A)</option>\n" );
		boaWrite(wp, "<option value=11>5 GHz (A+N)</option>\n" );
#if defined (WLAN0_5G_11AC_SUPPORT) || defined(WLAN1_5G_11AC_SUPPORT)
		boaWrite(wp, "<option value=75>5 GHz (A+N+AC)</option>\n" );
#endif
#ifdef WLAN_11AX
#ifdef WIFI5_WIFI6_COMP
		if(Entry.is_ax_support)
#endif
			boaWrite(wp, "<option value=203>5 GHz (A+N+AC+AX)</option>\n" );
#endif
#endif
#endif //WLAN_DENY_LEGACY

#if defined(WLAN_DUALBAND_CONCURRENT)
		if(vChar == PHYBAND_2G)
#endif
		{
#if defined(WLAN_DENY_LEGACY)
			boaWrite(wp, "<option value=0>2.4 GHz (B)</option>\n");
			boaWrite(wp, "<option value=1>2.4 GHz (G)</option>\n");
			boaWrite(wp, "<option value=2>2.4 GHz (B+G)</option>\n");
			boaWrite(wp, "<option value=7>2.4 GHz (N)</option>\n" );
			boaWrite(wp, "<option value=9>2.4 GHz (G+N)</option>\n" );
			boaWrite(wp, "<option value=10>2.4 GHz (B+G+N)</option>\n" );
#ifdef WLAN_11AX
			boaWrite(wp, "<option value=127>2.4 GHz (AX)</option>\n" );
			boaWrite(wp, "<option value=135>2.4 GHz (N+AX)</option>\n" );
			boaWrite(wp, "<option value=137>2.4 GHz (G+N+AX)</option>\n" );
			boaWrite(wp, "<option value=138>2.4 GHz (B+G+N+AX)</option>\n" );
#endif
#else
			boaWrite(wp, "<option value=0>2.4 GHz (B)</option>\n");
			boaWrite(wp, "<option value=2>2.4 GHz (B+G)</option>\n");
			boaWrite(wp, "<option value=10>2.4 GHz (B+G+N)</option>\n" );
#ifdef WLAN_11AX
#ifdef WIFI5_WIFI6_COMP
			if(Entry.is_ax_support)
#endif
				boaWrite(wp, "<option value=138>2.4 GHz (B+G+N+AX)</option>\n" );
#endif
#endif //WLAN_DENY_LEGACY
		}
		return 0;
	}
	if ( !strcmp(name, "wlchanwid") ) {
		boaWrite(wp, "<option value=\"0\">20MHZ</option>\n" );
		boaWrite(wp, "<option value=\"1\">40MHZ</option>\n" );
#if defined (WLAN0_5G_11AC_SUPPORT) || defined(WLAN1_5G_11AC_SUPPORT)
		mib_get_s( MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
		if(vChar == PHYBAND_5G) {
			boaWrite(wp, "<option value=\"2\">80MHZ</option>\n" );
#if defined(WLAN_SUPPPORT_160M)
			boaWrite(wp, "<option value=\"3\">160MHZ</option>\n" );
#endif
#if defined(CONFIG_00R0) && defined(WLAN_11N_COEXIST)
			mib_get_s( MIB_WLAN_BAND, (void *)&vChar, sizeof(vChar));
			if(vChar == BAND_5G_11ANAC)
				boaWrite(wp, "<option value=\"4\">20/40/80MHZ</option>\n" );
#endif
		}
#endif
#if defined (WLAN_11N_COEXIST)
#if defined (WLAN0_5G_SUPPORT) || defined(WLAN1_5G_SUPPORT)
		mib_get_s( MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
		if(vChar == PHYBAND_2G)
#endif
			boaWrite(wp, "<option value=\"3\">20/40MHZ</option>\n" );
#endif
		return 0;
	}
	if ( !strcmp(name, "wlctlband") ) {
#if defined (WLAN0_5G_SUPPORT) || defined(WLAN1_5G_SUPPORT)
		mib_get_s( MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
		if(vChar == PHYBAND_5G) {
			boaWrite(wp, "<option value=\"0\">%s</option>\n", multilang(LANG_AUTO) );
			boaWrite(wp, "<option value=\"1\">%s</option>\n", multilang(LANG_AUTO) );
		}
		else{
			boaWrite(wp, "<option value=\"0\">%s</option>\n", multilang(LANG_UPPER) );
			boaWrite(wp, "<option value=\"1\">%s</option>\n", multilang(LANG_LOWER) );
		}

#else
		boaWrite(wp, "<option value=\"0\">%s</option>\n", multilang(LANG_UPPER) );
		boaWrite(wp, "<option value=\"1\">%s</option>\n", multilang(LANG_LOWER) );
#endif
		return 0;
	}
	// Added by Mason Yu for TxPower
	if ( !strcmp(name, "txpower") ) {
		boaWrite(wp, "<option value=\"0\">100%%</option>\n" );
#ifdef CONFIG_00R0
		boaWrite(wp, "<option value=\"1\">80%%</option>\n" );
		boaWrite(wp, "<option value=\"2\">60%%</option>\n" );
		boaWrite(wp, "<option value=\"3\">40%%</option>\n" );
		boaWrite(wp, "<option value=\"4\">20%%</option>\n" );
#else
		boaWrite(wp, "<option value=\"1\">70%%</option>\n" );
		boaWrite(wp, "<option value=\"2\">50%%</option>\n" );
		boaWrite(wp, "<option value=\"3\">35%%</option>\n" );
		boaWrite(wp, "<option value=\"4\">15%%</option>\n" );
#endif
		return 0;
	}

#ifdef CONFIG_USER_AP_CMCC
#ifdef WLAN_SUPPORT
		 if(!strcmp(name, "ssid_pri")){

		        memset(&Wlans_Entry,0,sizeof(MIB_CE_MBSSIB_T));
			    if (!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Wlans_Entry)) {
		            printf("Error! Get MIB_MBSSIB_TBL(checkWrite) error.\n");
		            return -1;
	            }
				snprintf(buffer, sizeof(buffer), "priority = %d;", Wlans_Entry.manual_priority);
				boaWrite(wp, buffer);
		 }
#endif
#endif


	if (!strcmp(name, "wifiSecurity")) {
		unsigned char mode = 0;
#ifdef WLAN_6G_SUPPORT
		unsigned char enable_6g = 0;
#endif
		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		mode = Entry.wlanMode;
#ifdef WLAN_6G_SUPPORT
		mib_get_s(MIB_WLAN_6G_SUPPORT, (void *)&enable_6g, sizeof(enable_6g));
		if(enable_6g == 1){
			//boaWrite(wp, "<option value=%d>%s</option>\n", WIFI_SEC_NONE, multilang(LANG_NONE));
#ifdef WLAN_WPA3
			boaWrite(wp, "<option value=%d>WPA3</option>\n", WIFI_SEC_WPA3);
#endif
			return 0;
		}
#endif
		boaWrite(wp, "<option value=%d>%s</option>\n", WIFI_SEC_NONE, multilang(LANG_NONE));
#ifdef WLAN_QTN //QTN do not support WEP
#ifdef WLAN1_QTN
		if(wlan_idx==0)
#elif defined(WLAN0_QTN)
		if(wlan_idx==1)
#endif
#endif
		boaWrite(wp, "<option value=%d>WEP</option>\n", WIFI_SEC_WEP);
#ifndef NEW_WIFI_SEC
		boaWrite(wp, "<option value=%d>WPA</option>\n", WIFI_SEC_WPA);
#endif
		boaWrite(wp, "<option value=%d>WPA2</option>\n", WIFI_SEC_WPA2);
		if (mode != CLIENT_MODE)
		boaWrite(wp, "<option value=%d>%s</option>\n", WIFI_SEC_WPA2_MIXED, multilang(LANG_WPA2_MIXED));
#ifdef CONFIG_RTL_WAPI_SUPPORT
		boaWrite(wp, "<option value=%d>WAPI</option>\n", WIFI_SEC_WAPI);
#endif
#ifdef WLAN_WPA3
		boaWrite(wp, "<option value=%d>WPA3</option>\n", WIFI_SEC_WPA3);
#ifdef WLAN_WPA3_H2E
		boaWrite(wp, "<option value=%d>WPA3 Transition</option>\n", WIFI_SEC_WPA2_WPA3_MIXED);
#else
		boaWrite(wp, "<option value=%d>WPA2+WPA3 Mixed</option>\n", WIFI_SEC_WPA2_WPA3_MIXED);
#endif
#endif
		return 0;
	}
#ifdef WLAN_CLIENT
	if (!strcmp(name, "wifiClientSecurity")) {
		boaWrite(wp, "<option value=%d>%s</option>\n", WIFI_SEC_NONE, multilang(LANG_NONE));
		boaWrite(wp, "<option value=%d>WEP</option>\n", WIFI_SEC_WEP);
		boaWrite(wp, "<option value=%d>WPA</option>\n", WIFI_SEC_WPA);
		boaWrite(wp, "<option value=%d>WPA2</option>\n", WIFI_SEC_WPA2);
		boaWrite(wp, "<option value=%d>%s</option>\n", WIFI_SEC_WPA2_MIXED, multilang(LANG_WPA2_MIXED));
#ifdef CONFIG_RTL_WAPI_SUPPORT
		boaWrite(wp, "<option value=%d>WAPI</option>\n", WIFI_SEC_WAPI);
#endif
#ifdef WLAN_WPA3
		boaWrite(wp, "<option value=%d>WPA3</option>\n", WIFI_SEC_WPA3);
#ifdef WLAN_WPA3_H2E
		boaWrite(wp, "<option value=%d>WPA3 Transition</option>\n", WIFI_SEC_WPA2_WPA3_MIXED);
#else
		boaWrite(wp, "<option value=%d>WPA2+WPA3 Mixed</option>\n", WIFI_SEC_WPA2_WPA3_MIXED);
#endif
#endif

		return 0;
	}
#endif
	#ifdef WLAN_UNIVERSAL_REPEATER
	if ( !strcmp(name, "repeaterEnabled") ) {
		mib_get_s( MIB_REPEATER_ENABLED1, (void *)&vChar, sizeof(vChar));
		if (vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	#endif
	if( !strcmp(name, "wlan_idx") ) {
		boaWrite(wp, "%d", wlan_idx);
		return 0;
	}
	if( !strcmp(name, "2G_ssid") ) {
		char ssid[200];
		int i, orig_wlan_idx = wlan_idx;
		MIB_CE_MBSSIB_T Entry;
#if defined(WLAN_DUALBAND_CONCURRENT)
		for(i=0; i<NUM_WLAN_INTERFACE; i++) {
			wlan_idx = i;
			mib_get_s( MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
			if(vChar == PHYBAND_2G) {
				if(!wlan_getEntry(&Entry, 0))
					return -1;
				strcpy(ssid, Entry.ssid);
				translate_web_code(ssid);
				boaWrite(wp, "%s", ssid);
				break;
			}
		}
#else
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		strcpy(ssid, Entry.ssid);
		translate_web_code(ssid);
		boaWrite(wp, "%s", ssid);
#endif
		wlan_idx = orig_wlan_idx;
		return 0;
	}
	if( !strcmp(name, "5G_ssid") ) {
		char ssid[200];
		int i, orig_wlan_idx = wlan_idx;
		MIB_CE_MBSSIB_T Entry;
#if defined (WLAN_QTN)
#ifdef WLAN1_QTN
		wlan_idx = 1;
#else // WLAN0_QTN
		wlan_idx = 0;
#endif
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		strcpy(ssid, Entry.ssid);
		translate_web_code(ssid);
		boaWrite(wp, "%s", ssid);
#elif defined(WLAN_DUALBAND_CONCURRENT)
		for(i=0; i<NUM_WLAN_INTERFACE; i++) {
			wlan_idx = i;
			mib_get_s( MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
			if(vChar == PHYBAND_5G) {
				if(!wlan_getEntry(&Entry, 0))
					return -1;
				strcpy(ssid, Entry.ssid);
				translate_web_code(ssid);
				boaWrite(wp, "%s", ssid);
				break;
			}
		}
#else
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		strcpy(ssid, Entry.ssid);
		translate_web_code(ssid);
		boaWrite(wp, "%s", ssid);
#endif
		wlan_idx = orig_wlan_idx;
		return 0;
	}
#ifdef CONFIG_00R0
	if( !strcmp(name, "2G_root_disabled") ) {
		int i, orig_wlan_idx = wlan_idx;
		MIB_CE_MBSSIB_T Entry;
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(WLAN_DUALBAND_CONCURRENT)
		for(i=0; i<NUM_WLAN_INTERFACE; i++) {
			wlan_idx = i;
			mib_get_s( MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
			if(vChar == PHYBAND_2G) {
				if(!wlan_getEntry(&Entry, 0))
					return -1;
				if(Entry.func_off==1)
					boaWrite(wp, "1");
				else
					boaWrite(wp, "0");
				break;
			}
		}
#else
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		if(Entry.func_off==1)
			boaWrite(wp, "1");
		else
			boaWrite(wp, "0");
#endif
		wlan_idx = orig_wlan_idx;
		return 0;
	}
	if( !strcmp(name, "5G_root_disabled") ) {
		int i, orig_wlan_idx = wlan_idx;
		MIB_CE_MBSSIB_T Entry;
#if defined (WLAN_QTN)
#ifdef WLAN1_QTN
		wlan_idx = 1;
#else // WLAN0_QTN
		wlan_idx = 0;
#endif
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		if(Entry.func_off==1)
			boaWrite(wp, "1");
		else
			boaWrite(wp, "0");
#elif defined(CONFIG_RTL_92D_SUPPORT) || defined(WLAN_DUALBAND_CONCURRENT)
		for(i=0; i<NUM_WLAN_INTERFACE; i++) {
			wlan_idx = i;
			mib_get_s( MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
			if(vChar == PHYBAND_5G) {
				if(!wlan_getEntry(&Entry, 0))
					return -1;
				if(Entry.func_off==1)
					boaWrite(wp, "1");
				else
					boaWrite(wp, "0");
				break;
			}
		}
#else
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		if(Entry.func_off==1)
			boaWrite(wp, "1");
		else
			boaWrite(wp, "0");
#endif
		wlan_idx = orig_wlan_idx;
		return 0;
	}
#endif
	if ( !strcmp(name, "isCMCCSupport")) {
#ifdef CONFIG_USER_AP_CMCC
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif

        return 0;
	}

	if(!strcmp(name,"isWPA3Support")) {
#if defined(WLAN_WPA3)
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
	if ( !strcmp(name, "isRepeaterSupport")) {
#ifdef CONFIG_USER_RTK_REPEATER_MODE
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif

        return 0;
	}

	if(!strcmp(name, "supported_band")) {
		unsigned int fixrate=0;
		int supported_band=0;
		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		fixrate = Entry.fixedTxRate;
		if(fixrate == 0 || Entry.rateAdaptiveEnabled == 1){
			boaWrite(wp, "0");
			return 0;
		}
		init_rate_list();
		supported_band = check_band(fixrate);
		if(supported_band != 0)
			boaWrite(wp, "%d", supported_band);
		else
			boaWrite(wp, "0");
		return 0;
	}
	if (!strcmp(name, "dfs_enable")) {
#if defined(CONFIG_RTL_DFS_SUPPORT)
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
	if( !strcmp(name, "Band2G5GSupport") ) {
#if defined(WLAN0_5G_SUPPORT) || defined(WLAN1_5G_SUPPORT)
		mib_get_s( MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
		boaWrite(wp, "%d", vChar);
#else
		vChar = PHYBAND_2G;
		boaWrite(wp, "%d", vChar);
#endif
		return 0;
	}
#if defined(WLAN_DUALBAND_CONCURRENT)
	if( !strcmp(name, "wlanBand2G5GSelect") ) {
		boaWrite(wp, "%d", BANDMODEBOTH);
		return 0;
	}
#elif defined(WLAN0_5G_SUPPORT) || defined(WLAN1_5G_SUPPORT)
	if( !strcmp(name, "wlanBand2G5GSelect") ) {
		boaWrite(wp, "%d", BANDMODESINGLE);
		return 0;
	}
#else
	if( !strcmp(name, "wlanBand2G5GSelect") ) {
		boaWrite(wp, "");
		return 0;
	}
#endif
	if ( !strcmp(name, "wlan_mssid_num")) {
#ifdef CONFIG_00R0
		boaWrite(wp, "%d", WLAN_MBSSID_NUM-1);
#else
		boaWrite(wp, "%d", WLAN_MBSSID_NUM);
#endif
		return 0;
	}
	if(!strcmp(name,"new_wifi_security")) //wifi security requirements after 2014.01.01
	{
#ifdef NEW_WIFI_SEC
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
	if( !strcmp(name, "is_regdomain_demo") ) {
		mib_get_s( MIB_WIFI_REGDOMAIN_DEMO, (void *)&vChar, sizeof(vChar));
		boaWrite(wp, "%d", vChar);
		return 0;
	}
	if ( !strcmp(name, "regdomain_list") ) {
		boaWrite(wp, "<option value=\"1\">FCC(1)</option>\n" );
		boaWrite(wp, "<option value=\"2\">IC(2)</option>\n" );
		boaWrite(wp, "<option value=\"3\">ETSI(3)</option>\n" );
		boaWrite(wp, "<option value=\"4\">SPAIN(4)</option>\n" );
		boaWrite(wp, "<option value=\"5\">FRANCE(5)</option>\n" );
		boaWrite(wp, "<option value=\"6\">MKK(6)</option>\n" );
		boaWrite(wp, "<option value=\"7\">ISREAL(7)</option>\n" );
		boaWrite(wp, "<option value=\"8\">MKK1(8)</option>\n" );
		boaWrite(wp, "<option value=\"9\">MKK2(9)</option>\n" );
		boaWrite(wp, "<option value=\"10\">MKK3(10)</option>\n" );
		boaWrite(wp, "<option value=\"11\">NCC(11)</option>\n" );
		boaWrite(wp, "<option value=\"12\">RUSSIAN(12)</option>\n" );
		boaWrite(wp, "<option value=\"13\">CN(13)</option>\n" );
		boaWrite(wp, "<option value=\"14\">GLOBAL(14)</option>\n" );
		boaWrite(wp, "<option value=\"15\">WORLD-WIDE(15)</option>\n" );

		return 0;
	}
	if(!strcmp(name, "wpa3_h2e_support")){
#ifdef WLAN_WPA3_H2E
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
	if(!strcmp(name, "11w_support")){
#ifdef WLAN_11W
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
	if(!strcmp(name, "1x_support")){
#if defined(WLAN_1x) //&& !defined(IS_AX_SUPPORT)
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
	if(!strcmp(name, "enable_wpa2_and_wpa3_1x_only")){
#if defined(WLAN_1x_WPA2_WPA3_ONLY)
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
	if(!strcmp(name, "wpa3_1x_support")){
#if defined(WLAN_WPA3_ENTERPRISE)
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
#ifdef WLAN_11R
	if(!strcmp(name, "11r_ftkh_num")){
		boaWrite(wp, "%d", MAX_VWLAN_FTKH_NUM);
		return 0;
	}
#endif
#ifdef WLAN_QTN
	if(!strcmp(name, "wlan_qtn_hidden_function")){
#ifdef WLAN1_QTN
		if(wlan_idx==1)
#else
		if(wlan_idx==0)
#endif
			boaWrite(wp, "style=\"display: none\"");
		return 0;
	}
#endif
#ifdef WLAN_11AX
		if(!strcmp(name, "wlan_advanced_ax_hidden_func")) {
			boaWrite(wp, "style=\"display: none\"");
		}
#endif
	if(!strcmp(name, "is_wlan_qtn")){
#ifdef WLAN_QTN
#ifdef WLAN1_QTN
		if(wlan_idx==1)
#else
		if(wlan_idx==0)
#endif
			boaWrite(wp, "1");
		else
#endif
		boaWrite(wp, "0");
		return 0;
	}
	if(!strcmp(name, "ch_list_20")){
#ifdef WLAN_QTN
#ifdef WLAN1_QTN
		if(wlan_idx==1)
		{
			int mib_id = MIB_HW_WLAN1_REG_DOMAIN;
#else
		if(wlan_idx==0)
		{
			int mib_id = MIB_HW_REG_DOMAIN;
#endif

			char regdomain;
			char ch_list[1025];
			mib_get_s(mib_id, &regdomain, sizeof(regdomain));
			if(regdomain==0)
				boaWrite(wp, "0");
			else{
				rt_qcsapi_get_channel_list(rt_get_qtn_ifname(getWlanIfName()), regdomain, 20, ch_list);
				boaWrite(wp, "\"%s\"", ch_list);
			}
		}
		else
#endif
		boaWrite(wp, "0");

		return 0;
	}
	if(!strcmp(name, "ch_list_40")){
#ifdef WLAN_QTN
#ifdef WLAN1_QTN
		if(wlan_idx==1)
		{
			int mib_id = MIB_HW_WLAN1_REG_DOMAIN;
#else
		if(wlan_idx==0)
		{
			int mib_id = MIB_HW_REG_DOMAIN;
#endif
			char regdomain;
			char ch_list[1025];
			mib_get_s(mib_id, &regdomain, sizeof(regdomain));
			if(regdomain==0)
				boaWrite(wp, "0");
			else{
				rt_qcsapi_get_channel_list(rt_get_qtn_ifname(getWlanIfName()), regdomain, 40, ch_list);
				boaWrite(wp, "\"%s\"", ch_list);
			}
		}
		else
#endif
		boaWrite(wp, "0");

		return 0;
	}
	if(!strcmp(name, "ch_list_80")){
#ifdef WLAN_QTN
#ifdef WLAN1_QTN
		if(wlan_idx==1)
		{
			int mib_id = MIB_HW_WLAN1_REG_DOMAIN;
#else
		if(wlan_idx==0)
		{
			int mib_id = MIB_HW_REG_DOMAIN;
#endif
			char regdomain;
			char ch_list[1025];
			mib_get_s(mib_id, &regdomain, sizeof(regdomain));
			if(regdomain==0)
				boaWrite(wp, "0");
			else{
				rt_qcsapi_get_channel_list(rt_get_qtn_ifname(getWlanIfName()), regdomain, 80, ch_list);
				boaWrite(wp, "\"%s\"", ch_list);
			}
		}
		else
#endif
		boaWrite(wp, "0");

		return 0;
	}
	if(!strcmp(name,"wlan_support_11ncoexist"))
	{
#ifdef	WLAN_11N_COEXIST
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
#ifdef WLAN_MESH_ACL_ENABLE
	if (!strcmp(name, "wlanMeshAclNum")) {
		MIB_CE_WLAN_AC_T entry;
		int i;
		vUInt = mib_chain_total(MIB_WLAN_MESH_ACL_TBL);
		for (i=0; i<vUInt; i++) {
			if (!mib_chain_get(MIB_WLAN_MESH_ACL_TBL, i, (void *)&entry)) {
				i = vUInt;
				break;
			}
			if(entry.wlanIdx == wlan_idx)
				break;
		}
		if (i == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
#endif
	if(!strcmp(name, "is_wlan_radius_2set"))
	{
#ifdef WLAN_RADIUS_2SET
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
#ifdef RTK_MULTI_AP
	if (!strcmp(name, "is_dot11kv_disabled")) {
		int i, j;
		MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_DUALBAND_CONCURRENT
		int orig_wlan_idx = wlan_idx;
#endif
		for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
			wlan_idx = i;
			for (j = 0; j < NUM_VWLAN_INTERFACE; j++) {
				wlan_getEntry(&Entry,j);
#if defined(WLAN_11K) && defined(WLAN_11V)
				if (!Entry.rm_activated || !Entry.BssTransEnable) {
						boaWrite(wp, "1");
#ifdef WLAN_DUALBAND_CONCURRENT
						wlan_idx =	orig_wlan_idx;
#endif
						return 0;
				}
#endif
			}
		}
		boaWrite(wp, "0");
#ifdef WLAN_DUALBAND_CONCURRENT
		wlan_idx =	orig_wlan_idx;
#endif
		return 0;
	}
	if (!strcmp(name, "is_security_setting_wrong")) {
		int i, j;
		MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_DUALBAND_CONCURRENT
		int orig_wlan_idx = wlan_idx;
#endif
		for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
				wlan_idx = i;
				for (j = 0; j < (WLAN_MBSSID_NUM+1); j++) {
					if (j == 1)
						continue;
					wlan_getEntry(&Entry,j);

					//if enabled
					if (!Entry.wlanDisabled) {
#ifdef WLAN_WPA3
						if (((Entry.encrypt != WIFI_SEC_WPA2) && (Entry.encrypt != WIFI_SEC_WPA3) && (Entry.encrypt != WIFI_SEC_WPA2_WPA3_MIXED) && (Entry.encrypt != WIFI_SEC_WPA2_MIXED)) || (Entry.wpaAuth != WPA_AUTH_PSK) || (Entry.wpaPSK[0] == '\0') ) {
#else
						if ((Entry.encrypt != WIFI_SEC_WPA2) || (Entry.wpaAuth != WPA_AUTH_PSK) || (Entry.wpaPSK[0] == '\0') ) {
#endif
#ifdef MULTI_AP_OPEN_ENCRYPT_SUPPORT
							if(Entry.encrypt == WIFI_SEC_NONE)
								boaWrite(wp, "0");
							else
								boaWrite(wp, "1");
#else
							boaWrite(wp, "1");
#endif
#ifdef WLAN_DUALBAND_CONCURRENT
							wlan_idx =	orig_wlan_idx;
#endif
							return 0;
						}
					}
				}
		}
		boaWrite(wp, "0");
#ifdef WLAN_DUALBAND_CONCURRENT
		wlan_idx =	orig_wlan_idx;
#endif
		return 0;
	}

	if (!strcmp(name, "needPopupBackhaul")) {
		int i;
		MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_DUALBAND_CONCURRENT
		int orig_wlan_idx = wlan_idx;
#endif
		//check if wlanX-vap0 is disabled
		for(i=0; i<NUM_WLAN_INTERFACE;i++){
			wlan_idx = i;
			wlan_getEntry(&Entry,WLAN_VAP_ITF_INDEX);
			//if enabled
			if (!Entry.wlanDisabled) {
				boaWrite(wp, "1");
#ifdef WLAN_DUALBAND_CONCURRENT
				wlan_idx =	orig_wlan_idx;
#endif
				return 0;
			}
		}
		boaWrite(wp, "0");
#ifdef WLAN_DUALBAND_CONCURRENT
		wlan_idx =	orig_wlan_idx;
#endif
		return 0;
	}
	if (!strcmp(name, "topology_json_string")) {
		FILE *fp = fopen("/tmp/topology_json", "r");
		if (fp == NULL) {
			return -1;
		}
		ssize_t read;
		size_t	len   = 0;
		char*	line  = NULL;
		read = getline(&line, &len, fp);
		fclose(fp);
		boaWrite(wp, "%s", line);
		if(line) free(line);
		return 0;
	}
	if (!strcmp(name, "backhaul_link")) {
		int ret;
#ifdef BACKHAUL_LINK_SELECTION
		unsigned char map_state;

		if (!mib_get_s(MIB_MAP_CONTROLLER, &map_state, sizeof(map_state))) {
			return -1;
		}

		MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_DUALBAND_CONCURRENT
		int orig_wlan_idx = wlan_idx;
#endif
		int i = 0, j = 0;
		int backhaul_link = 0;

		for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
			wlan_idx = i;
			if (1 == map_state) {
				for (j = 0; j < NUM_VWLAN_INTERFACE; j++) {
					wlan_getEntry(&Entry, j);
					if (0x40 & Entry.multiap_bss_type) {
						backhaul_link |= 1 << i;
						break;
					}
				}
			} else {
				wlan_getEntry(&Entry, WLAN_REPEATER_ITF_INDEX);
				if (0x80 & Entry.multiap_bss_type) {
					backhaul_link |= 1 << i;
				}
			}
		}

		if (backhaul_link == 0x01) {
			ret = boaWrite(wp, "0");
		} else if (backhaul_link == 0x02) {
			ret = boaWrite(wp, "1");
		} else if (backhaul_link == 0x04) {
			ret = boaWrite(wp, "2");
		} else {
#if defined (WLAN1_5G_SUPPORT)
			ret = boaWrite(wp, "1");
#else
			ret = boaWrite(wp, "0");
#endif
		}

#ifdef WLAN_DUALBAND_CONCURRENT
		wlan_idx =	orig_wlan_idx;
#endif
#else
#if defined (WLAN1_5G_SUPPORT)
		ret = boaWrite(wp, "1");
#else
		ret = boaWrite(wp, "0");
#endif
#endif
		return ret;
	}
	if (!strcmp(name, "is_agent_enabled")){
#if defined(CONFIG_RTK_DEV_AP)
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
#endif
	if(!strcmp(name, "min_beacon_interval")){
		boaWrite(wp, "%u", MIN_WLAN_BEACON_INTERVAL);
		return 0;
	}

	if ( NULL != strstr(name, "map_fronthaul_bss_")) {
			//findout which interface is being queried
		int ret;
#ifdef RTK_MULTI_AP_R2
		int length = strlen(name);
		int v_idx;
		MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_DUALBAND_CONCURRENT
		int wlan_idx_tmp = wlan_idx;
#endif
		int tmp_val = 0;
		wlan_idx  = name[length - 2] - '0';
		v_idx = name[length - 1] - '0';
		//see if this interface is enabled
		wlan_getEntry(&Entry, v_idx);
		if (!Entry.wlanDisabled) {
			if ((0x40 != Entry.multiap_bss_type) && (0x80 != Entry.multiap_bss_type)) {
				ret = boaWrite(wp, "1");
#ifdef WLAN_DUALBAND_CONCURRENT
			wlan_idx = wlan_idx_tmp;
#endif
			return ret;
			}
		}

		ret = boaWrite(wp, "0");

#ifdef WLAN_DUALBAND_CONCURRENT
		wlan_idx = wlan_idx_tmp;
#endif

#else
		ret = boaWrite(wp, "0");
#endif
		return ret;//return 3 if error
	}

	if ( NULL != strstr(name, "map_vid_")) {
        //findout which interface is being queried
		int ret;
#ifdef RTK_MULTI_AP_R2
		int length = strlen(name);
		int v_idx;
		MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_DUALBAND_CONCURRENT
		int wlan_idx_tmp = wlan_idx;
#endif
		wlan_idx  = name[length - 2] - '0';
		v_idx = name[length - 1] - '0';
		wlan_getEntry(&Entry, v_idx);
		ret = boaWrite(wp, "%d", Entry.multiap_vid);
#ifdef WLAN_DUALBAND_CONCURRENT
		wlan_idx = wlan_idx_tmp;
#endif

#else
	ret = boaWrite(wp, "0");
#endif
		return ret;//return 3 if error
	}

#ifdef RTK_MULTI_AP
	if ( !strcmp(name, "multi_ap_controller")) {
		unsigned char map_state;
		int ret;
		if (!mib_get_s(MIB_MAP_CONTROLLER, &map_state, sizeof(map_state))) {
			ret = boaWrite(wp, "0");
			return ret;
		}
		ret = boaWrite(wp, "1");
		return ret;
	}

	if ( NULL != strstr(name, "interface_info_query_")) {
		//findout which interface is being queried
        int length = strlen(name);
		int ret;
		char ssid[200];
		int v_idx;
		MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_DUALBAND_CONCURRENT
		int wlan_idx_tmp = wlan_idx;
#endif
		wlan_idx  = name[length - 2] - '0';
		v_idx = name[length - 1] - '0';
		//see if this interface is enabled
		if(!wlan_getEntry(&Entry, v_idx)) {
#ifdef WLAN_DUALBAND_CONCURRENT
			wlan_idx = wlan_idx_tmp;
#endif
			return -1;
		}
		strcpy(ssid, Entry.ssid);
		translate_web_code(ssid);
		ret = boaWrite(wp, "%s", ssid);

#ifdef WLAN_DUALBAND_CONCURRENT
		wlan_idx = wlan_idx_tmp;
#endif
		return ret;//return 3 if error
	}

	if (!strcmp(name, "wlanDisabled")) {
        MIB_CE_MBSSIB_T Entry;
		int ret;
		if ( !Entry.wlanDisabled){
			ret = boaWrite(wp, "0");
            return ret;
		}
		ret = boaWrite(wp, "1");
		return ret;
	}
#else
	if ( !strcmp(name, "multi_ap_controller")) {
		int ret;
		ret = boaWrite(wp, "0");
		return 0;
	}

	if ( NULL != strstr(name, "interface_info_query_")) {
		int ret;
		ret = boaWrite(wp, "0");
		return 0;
	}
#endif

	if (!strcmp(name, "map_enable_vlan")) {
		int ret;
#ifdef RTK_MULTI_AP_R2
		if(!mib_get_s(MIB_MAP_ENABLE_VLAN, &vChar, sizeof(vChar)))
			return -1;
		ret = boaWrite(wp, "%d", vChar);
#else
		ret = boaWrite(wp, "0");
#endif
		return ret;
	}

	if (!strcmp(name, "map_default_secondary_vid")) {
		int ret;
#ifdef RTK_MULTI_AP_R2
		if(!mib_get_s(MIB_MAP_DEFAULT_SECONDARY_VID, &vChar, sizeof(vChar)))
			return -1;
		ret = boaWrite(wp, "%d", vChar);
#else
		ret = boaWrite(wp, "0");
#endif
		return ret;
	}

	if (!strcmp(name, "easymesh_enabled")) {
		int ret;
#ifdef RTK_MULTI_AP
		unsigned char map_role = 0;
		if(!mib_get_s(MIB_MAP_CONTROLLER, &map_role, sizeof(map_role)))
			return -1;
		if(map_role)
			ret = boaWrite(wp, "1");
		else
			ret = boaWrite(wp, "0");
#else
		ret = boaWrite(wp, "0");
#endif
		return ret;
	}
	if (NULL != strstr(name, "backhaulIndexQuery_")) {
		int j;
		int ret;
		int length = strlen(name);

#ifdef WLAN_DUALBAND_CONCURRENT
		int widx_tmp = wlan_idx;
#endif
		unsigned char map_state;
		MIB_CE_MBSSIB_T Entry;
#ifdef RTK_MULTI_AP
		if (!mib_get_s(MIB_MAP_CONTROLLER, &map_state, sizeof(map_state))) {
			return -1;
		}

		if(map_state == 0)
			ret = boaWrite(wp, "0");

		wlan_idx = name[length - 1] - '0';
		for (j = 1; j < 5; j++) { //exclude root and vxd
			//vwlan_idx = j;
			wlan_getEntry(&Entry,j);
			if (Entry.multiap_bss_type == 0x40) {
				wlan_idx = widx_tmp;
				//vwlan_idx = vwidx_tmp;
	//			//if it is only backhaul
				ret = boaWrite(wp, "%d", j);
				return ret;
			}
		}
		ret = boaWrite(wp, "0");
#else
		ret = boaWrite(wp, "0");
#endif//RTK_MULTI_AP
#ifdef WLAN_DUALBAND_CONCURRENT
		wlan_idx = widx_tmp;
#endif
		return 0;
	}
#endif // of WLAN_SUPPORT
	if(!strcmp(name,"wlan_support_8812e")) //8812
	{
#if (defined(WLAN0_5G_11AC_SUPPORT) || defined(WLAN1_5G_11AC_SUPPORT)) &&!defined(WLAN_QTN)
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
	if(!strcmp(name,"wlan_support_11ax")) //11ax
	{
#if defined(WLAN_11AX)
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
		if(!strcmp(name,"wlan_support_160M")) //160M
	{
#if defined(WLAN_SUPPPORT_160M)
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
	if(!strcmp(name,"wlan_wifi5_wifi6_comp"))
	{
#if defined(WIFI5_WIFI6_COMP)
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
	if(!strcmp(name,"is_hapd_wpas_wps"))
	{
#if defined(CONFIG_USER_HAPD) || defined(CONFIG_USER_WPAS)
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
	if(!strcmp(name,"wlan_band_config_mbssid")) //11ax
	{
#if defined(WLAN_BAND_CONFIG_MBSSID)
		boaWrite(wp, "wlan_band_config_mbssid = 1;\n");
#endif
		return 0;
	}
	if(!strcmp(name,"wlan_support_deny_legacy"))
	{
#if defined(WLAN_DENY_LEGACY)
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
		return 0;
	}
	if ( !strcmp(name, "wlan_fon")) {
#ifdef CONFIG_USER_FON
			boaWrite(wp, "1");
#else
			boaWrite(wp, "0");
#endif
		return 0;
	}
#ifdef CONFIG_USER_FON
	if ( !strcmp(name, "wlan_fon_onoffline")) {
		mib_get_s( MIB_FON_ONOFF, (void *)&vChar, sizeof(vChar));
		boaWrite(wp, "%d", vChar);
		return 0;
	}
#endif
	// Added by Mason Yu for 2 level web page
	if ( !strcmp(name, "userMode") ) {
		#ifdef ACCOUNT_CONFIG
		MIB_CE_ACCOUNT_CONFIG_T Entry;
		int totalEntry, i;
		#else
		char suStr[100], usStr[100];
		#endif
#ifdef ACCOUNT_CONFIG
		#ifdef USE_LOGINWEB_OF_SERVER
		if (!strcmp(g_login_username, suName))
		#else
		if (!strcmp(wp->user, suName))
		#endif
		{
			boaWrite(wp, "<option selected value=\"0\">%s</option>\n", suName);
			boaWrite(wp, "<option value=\"1\">%s</option>\n", usName);
		}
		#ifdef USE_LOGINWEB_OF_SERVER
		else if (!strcmp(g_login_username, usName))
		#else
		else if (!strcmp(wp->user, usName))
		#endif
		{
			boaWrite(wp, "<option value=\"0\">%s</option>\n", suName);
			boaWrite(wp, "<option selected value=\"1\">%s</option>\n", usName);
		}
		totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL);
		for (i=0; i<totalEntry; i++) {
			if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&Entry))
				continue;
			#ifdef USE_LOGINWEB_OF_SERVER
			if (!strcmp(g_login_username, Entry.userName))
			#else
			if (strcmp(wp->user, Entry.userName) == 0)
			#endif
				boaWrite(wp, "<option selected value=\"%d\">%s</option>\n", i+2, Entry.userName);
			else
				boaWrite(wp, "<option value=\"%d\">%s</option>\n", i+2, Entry.userName);
		}
#else
		#ifdef USE_LOGINWEB_OF_SERVER
		if (!strcmp(g_login_username, suName))
		#else
		if(!strcmp(wp->user,suName))
		#endif
			{
			sprintf(suStr, "<option selected value=\"0\">%s</option>\n", suName);
			sprintf(usStr, "<option value=\"1\">%s</option>\n", usName);
			}
		else
			sprintf(usStr, "<option selected value=\"1\">%s</option>\n", usName);

		boaWrite(wp, suStr );
		boaWrite(wp, usStr );
#endif
		return 0;
	}
	if ( !strcmp(name, "lan-dhcp-st") ) {
		#ifdef CONFIG_USER_DHCP_SERVER
		if ( !mib_get_s( MIB_DHCP_MODE, (void *)&vChar, sizeof(vChar)) )
			return -1;

		if (DHCPV4_LAN_SERVER == vChar)
			boaWrite(wp, (char *)multilang(LANG_ENABLED));
		else
		#endif
			boaWrite(wp, (char *)multilang(LANG_DISABLED));
		return 0;
	}
	else if ( !strcmp(name, "br-stp-0") ) {
		if ( !mib_get_s( MIB_BRCTL_STP, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "br-stp-1") ) {
		if ( !mib_get_s( MIB_BRCTL_STP, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#ifdef CONFIG_USER_IGMPPROXY
	else if ( !strcmp(name, "igmpfastleave0") ) {
		if ( !mib_get_s( MIB_IGMP_FAST_LEAVE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "igmpfastleave1") ) {
		if ( !mib_get_s( MIB_IGMP_FAST_LEAVE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "igmpquerymode0") ) {
		if ( !mib_get_s( MIB_IGMP_QUERY_MODE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "igmpquerymode1") ) {
		if ( !mib_get_s( MIB_IGMP_QUERY_MODE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "igmpqueryversion0") ) {
		if ( !mib_get_s( MIB_IGMP_QUERY_VERSION, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (2 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "igmpqueryversion1") ) {
		if ( !mib_get_s( MIB_IGMP_QUERY_VERSION, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (3 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "igmpProxy0") ) {
		if ( !mib_get_s( MIB_IGMP_PROXY, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		if (ifWanNum("rt") ==0)
			boaWrite(wp, " disabled");
		return 0;
	}
	else if ( !strcmp(name, "igmpProxy1") ) {
		if ( !mib_get_s( MIB_IGMP_PROXY, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		if (ifWanNum("rt") ==0)
			boaWrite(wp, " disabled");
		return 0;
	}
	else if ( !strcmp(name, "igmpProxy0d") ) {
		if ( !mib_get_s( MIB_IGMP_PROXY, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "disabled");
		return 0;
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_PPTP_CLIENT)
	else if (!strcmp(name, "pptpenable0")) {
		if ( !mib_get_s( MIB_PPTP_ENABLE, (void *)&vUInt, sizeof(vUInt)) )
			return -1;
		//printf("pptp %s\n", vUInt?"enable":"disable");
		if (0 == vUInt)
			boaWrite(wp, "checked");
		return 0;
	}
	else if (!strcmp(name, "pptpenable1")) {
		if ( !mib_get_s( MIB_PPTP_ENABLE, (void *)&vUInt, sizeof(vUInt)) )
			return -1;
		//printf("pptp %s\n", vUInt?"enable":"disable");
		if (1 == vUInt)
			boaWrite(wp, "checked");
		return 0;
	}
#endif //end of CONFIG_USER_PPTP_CLIENT_PPTP
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_L2TPD_LNS) || defined(CONFIG_USER_XL2TPD)
	else if (!strcmp(name, "l2tpenable0")) {
		if (!mib_get_s( MIB_L2TP_ENABLE, (void *)&vUInt, sizeof(vUInt)))
			return -1;
		if (0 == vUInt)
			boaWrite(wp, "checked");
		return 0;
	}
	else if (!strcmp(name, "l2tpenable1")) {
		if ( !mib_get_s( MIB_L2TP_ENABLE, (void *)&vUInt, sizeof(vUInt)) )
			return -1;
		if (1 == vUInt)
			boaWrite(wp, "checked");
		return 0;
	}
#endif //endof CONFIG_USER_L2TPD_L2TPD
#ifdef CONFIG_NET_IPIP
	else if (!strcmp(name, "ipipenable0")) {
		if (!mib_get_s( MIB_IPIP_ENABLE, (void *)&vUInt, sizeof(vUInt)))
			return -1;
		if (0 == vUInt)
			boaWrite(wp, "checked");
		return 0;
	}
	else if (!strcmp(name, "ipipenable1")) {
		if ( !mib_get_s( MIB_IPIP_ENABLE, (void *)&vUInt, sizeof(vUInt)) )
			return -1;
		if (1 == vUInt)
			boaWrite(wp, "checked");
		return 0;
	}
#endif//endof CONFIG_NET_IPIP
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	else if(!strcmp(name, "sta_balance0")){
		if ( !mib_get_s( MIB_STATIC_LOAD_BALANCE_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if(!strcmp(name, "sta_balance1")){
		if ( !mib_get_s( MIB_STATIC_LOAD_BALANCE_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if(!strcmp(name, "dyn_balance0")){
		if ( !mib_get_s( MIB_DYNAMIC_LOAD_BALANCE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if(!strcmp(name, "dyn_balance1")){
		if ( !mib_get_s( MIB_DYNAMIC_LOAD_BALANCE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 != vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if(!strcmp(name, "dyn_type_0")){
		if ( !mib_get_s( MIB_DYNAMIC_LOAD_BALANCE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if(!strcmp(name, "dyn_type_1")){
		if ( !mib_get_s( MIB_DYNAMIC_LOAD_BALANCE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (2 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if(!strcmp(name, "dyn_balance_type")) {
		if ( !mib_get_s( MIB_DYNAMIC_LOAD_BALANCE_TYPE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if(2==vChar)
			boaWrite(wp, "2");
		else if(1==vChar)
			boaWrite(wp, "1");

		return 0;
	}
	else if(!strcmp(name, "wan_dev_num"))
	{
		int entry_num = 0;
		entry_num = mib_chain_total(MIB_ATM_VC_TBL);
		boaWrite(wp, "%d", entry_num);

		return 0;
	}
	else if(!strcmp(name, "total_balance0"))
	{
		if ( !mib_get_s( MIB_GLOBAL_LOAD_BALANCE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if(!strcmp(name, "total_balance1"))
	{
		if ( !mib_get_s( MIB_GLOBAL_LOAD_BALANCE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if(!strcmp(name, "dyn_balance_enable"))
	{
		unsigned char vChar1 = 0;
		if ( !mib_get_s( MIB_GLOBAL_LOAD_BALANCE, (void *)&vChar1, sizeof(vChar1)) )
			return -1;
		if ( !mib_get_s( MIB_DYNAMIC_LOAD_BALANCE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar || 0 == vChar1)
			boaWrite(wp, "disabled");

		return 0;
	}
	else if(!strcmp(name, "lan_info_num"))
	{
#ifdef CONFIG_USER_LANNETINFO
		int entry_num = 0, i =0, cnt = 0;
		lanHostInfo_t *pLanNetInfo=NULL;
		unsigned int count = 0;
		if(get_lan_net_info(&pLanNetInfo, &count) != 0)
		{
			boaWrite(wp, "%d", 0);
			return -1;
		}

		if(count<=0)
		{
			if(pLanNetInfo)
				free(pLanNetInfo);

			boaWrite(wp, "%d", 0);
			return -1;
		}

		boaWrite(wp, "%d", count);

		if(pLanNetInfo)
			free(pLanNetInfo);
#endif
		return 0;
	}
#endif
	else if( !strcmp(name, "multi_phy_wan_demo_start") ){
	#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN_DEMO
		boaWrite(wp, "");
	#else
		boaWrite(wp, "<!--");
	#endif
		return 0;
	}
	else if( !strcmp(name, "multi_phy_wan_demo_end") ){
	#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN_DEMO
		boaWrite(wp, "");
	#else
		boaWrite(wp, "-->");
	#endif
		return 0;
	}
	else if( !strcmp(name, "multi_phy_wan_formal_start") ){
	#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN_DEMO
		boaWrite(wp, "<!--");
	#else
		boaWrite(wp, "");
	#endif
		return 0;
	}
	else if( !strcmp(name, "multi_phy_wan_formal_end") ){
	#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN_DEMO
		boaWrite(wp, "-->");
	#else
		boaWrite(wp, "");
	#endif
		return 0;
	}
#ifdef CONFIG_USER_MINIUPNPD
	else if ( !strcmp(name, "upnp0") ) {
		if ( !mib_get_s( MIB_UPNP_DAEMON, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		if (ifWanNum("rt") ==0)
			boaWrite(wp, " disabled");
		return 0;
	}
	else if ( !strcmp(name, "upnp1") ) {
		if ( !mib_get_s( MIB_UPNP_DAEMON, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		if (ifWanNum("rt") ==0)
			boaWrite(wp, " disabled");
		return 0;
	}
	else if ( !strcmp(name, "upnp0d") ) {
		//if ( !mib_get_s( MIB_UPNP_DAEMON, (void *)&vChar, sizeof(vChar)) )
		//	return -1;
		if (ifWanNum("rt") ==0)
			boaWrite(wp, "disabled");
		return 0;
	}
#ifdef CONFIG_TR_064
	else if(!strcmp(name, "tr064_switch"))
	{
		int sw = TR064_STATUS;

		boaWrite(wp, "<tr>\n");
		boaWrite(wp, "\t\t<td><b>%s:</b></td>\n", multilang(LANG_TR064));
		boaWrite(wp, "\t\t<td>\n");
		boaWrite(wp, "\t\t\t<input type=radio value=0 name=tr_064_sw %s>%s&nbsp;&nbsp;\n", sw? "":"checked", multilang(LANG_DISABLE));
		boaWrite(wp, "\t\t\t<input type=radio value=1 name=tr_064_sw %s>%s\n", sw? "checked": "", multilang(LANG_ENABLE));
		boaWrite(wp, "\t\t</td>\n\t</tr>\n");
	}
#endif
#endif
#ifdef TIME_ZONE
	else if ( !strcmp(name, "sntp0d") ) {
		if (ifWanNum("rt") ==0)
			boaWrite(wp, "disabled");
		return 0;
	}
	else if ( !strcmp(name, "ntpInterval") ) {
		unsigned int interval;
		mib_get_s(MIB_NTP_INTERVAL, &interval, sizeof(interval));
		boaWrite(wp, "%d", interval);
		return 0;
	}
#endif
// Mason Yu. MLD Proxy
	else if(!strcmp(name, "rtk_ipv6_enable"))
	{
#ifdef CONFIG_IPV6
		boaWrite(wp, "\"yes\"");
#else
		boaWrite(wp, "\"no\"");
#endif
	}

#ifdef CONFIG_IPV6
	else if ( !strcmp(name, "ipv6enabledisable0") ) {
		if ( !mib_get_s( MIB_V6_IPV6_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "ipv6enabledisable1") ) {
		if ( !mib_get_s( MIB_V6_IPV6_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if (!strcmp(name, "simpleSec_Display")) {
#ifndef CONFIG_USER_RTK_IPV6_SIMPLE_SECURITY
		boaWrite(wp, "style=\"display: none\"");
#endif
		return 0;
	}
#ifdef CONFIG_USER_RTK_IPV6_SIMPLE_SECURITY
	else if ( !strcmp(name, "ipv6SimpleSec0") ) {
		if ( !mib_get_s( MIB_V6_SIMPLE_SEC_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "ipv6SimpleSec1") ) {
		if ( !mib_get_s( MIB_V6_SIMPLE_SEC_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
	else if (!strcmp(name, "logoTest_Display")) {
#ifndef CONFIG_RTK_IPV6_LOGO_TEST
		boaWrite(wp, "style=\"display: none\"");
#endif
		return 0;
	}
#ifdef CONFIG_USER_MLDPROXY
	else if ( !strcmp(name, "mldproxy0") ) {
		if ( !mib_get_s( MIB_MLD_PROXY_DAEMON, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		if (ifWanNum("rtv6") ==0)
			boaWrite(wp, " disabled");
		return 0;
	}
	else if ( !strcmp(name, "mldproxy1") ) {
		if ( !mib_get_s( MIB_MLD_PROXY_DAEMON, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		if (ifWanNum("rtv6") ==0)
			boaWrite(wp, " disabled");
		return 0;
	}
	else if ( !strcmp(name, "mldproxy0d") ) {
		//if ( !mib_get_s( MIB_MLD_PROXY_DAEMON, (void *)&vChar, sizeof(vChar)) )
		//	return -1;
		if (ifWanNum("rtv6") ==0)
			boaWrite(wp, "disabled");
		return 0;
	}
#endif
#ifdef CONFIG_RTK_DEV_AP
    else if ( !strcmp(name, "lanipv6addr") ) {
		char tmpBuf[MAX_V6_IP_LEN];
   		if ( !mib_get_s( MIB_IPV6_LAN_IP_ADDR, (void *)&tmpBuf, sizeof(tmpBuf)) )
			return -1;
		boaWrite(wp, "%s", tmpBuf);
		return 0;
	}
#endif
    else if ( !strcmp(name, "lanipv6prefix") ) {
		char tmpBuf[MAX_V6_IP_LEN], len;
   		if ( !mib_get_s( MIB_IPV6_LAN_PREFIX, (void *)&tmpBuf, sizeof(tmpBuf)) )
		{
			return -1;
		}
   		if ( !mib_get_s( MIB_IPV6_LAN_PREFIX_LEN, (void *)&len, sizeof(len)) )
		{
			return -1;
		}
		if(tmpBuf[0] && (len!=0))
			boaWrite(wp, "%s/%d", tmpBuf,len);
		return 0;
	}
	else if ( !strcmp(name, "LAN_LLA_mode") ) {
		char tmpBuf[MAX_V6_IP_LEN];
   		if ( !mib_get_s( MIB_IPV6_LAN_LLA_IP_ADDR, (void *)&tmpBuf, sizeof(tmpBuf)) )
			return -1;
		if (!strcmp(tmpBuf, ""))
			boaWrite(wp, "%u", 0);
		else
			boaWrite(wp, "%u", 1);
		return 0;
	}
#if defined(DHCPV6_ISC_DHCP_4XX)
	else if ( !strcmp(name, "prefix_delegation_info") ) {
		struct in6_addr ip6Prefix = {0};
		unsigned char value[48] = {0};
		unsigned int len = 0;

		rtk_ipv6_get_prefix_len(&len);
		if (0 == len) {
			boaWrite(wp, "");
		}
		else {
			rtk_ipv6_get_prefix((void *)&ip6Prefix);
			inet_ntop(PF_INET6, &ip6Prefix, value, sizeof(value));
			boaWrite(wp, "%s/%d", value, len);
		}
		return 0;
	}
	else if ( !strcmp(name, "dhcpV6R") ) {
		if (ifWanNum("rtv6") ==0)
			boaWrite(wp, "disabled");
		return 0;
	}
#endif
#endif	// #ifdef CONFIG_IPV6

#ifdef NAT_CONN_LIMIT
	else if (!strcmp(name, "connlimit")) {
		if (!mib_get_s(MIB_NAT_CONN_LIMIT, (void *)&vChar, sizeof(vChar)))
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
	}
#endif
#ifdef TCP_UDP_CONN_LIMIT
	else if ( !strcmp(name, "connLimit-cap0") ) {
   		if ( !mib_get_s( MIB_CONNLIMIT_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "connLimit-cap1") ) {
   		if ( !mib_get_s( MIB_CONNLIMIT_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}

#endif
#ifdef IP_ACL
#ifdef CONFIG_00R0
	else if (!strcmp(name, "telnet_lan_enable"))
	{
		if (!mib_get_s(MIB_TELNET_ENABLE, (void *)&vChar, sizeof(vChar)))
			return -1;
		if (vChar==1)
			boaWrite(wp,"checked");
		return 0;
	}
#endif
	else if ( !strcmp(name, "acl-cap0") ) {
   		if ( !mib_get_s( MIB_ACL_CAPABILITY, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "acl-cap1") ) {
   		if ( !mib_get_s( MIB_ACL_CAPABILITY, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
#if defined(CONFIG_IPV6) && defined(IP_ACL)
	else if ( !strcmp(name, "v6-acl-cap0") ) {
   		if ( !mib_get_s( MIB_V6_ACL_CAPABILITY, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "v6-acl-cap1") ) {
   		if ( !mib_get_s( MIB_V6_ACL_CAPABILITY, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	else if ( !strcmp(name, "snmpd-on") ) {
   		if ( !mib_get_s( MIB_SNMPD_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "snmpd-off") ) {
   		if ( !mib_get_s( MIB_SNMPD_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "snmpCommunityRO-on") ) {
   		if ( !mib_get_s( MIB_SNMP_COMM_RO_ON, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "snmpCommunityRO-off") ) {
   		if ( !mib_get_s( MIB_SNMP_COMM_RO_ON, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "snmpCommunityRW-on") ) {
   		if ( !mib_get_s( MIB_SNMP_COMM_RW_ON, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "snmpCommunityRW-off") ) {
   		if ( !mib_get_s( MIB_SNMP_COMM_RW_ON, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
#ifdef URL_BLOCKING_SUPPORT
	else if ( !strcmp(name, "url-cap0") ) {
   		if ( !mib_get_s( MIB_URL_CAPABILITY, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "url-cap1") ) {
   		if ( !mib_get_s( MIB_URL_CAPABILITY, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
//alex_huang
#ifdef URL_ALLOWING_SUPPORT
       else if( !strcmp(name ,"url-cap2") ) {
	   	if( !mib_get_s (MIB_URL_CAPABILITY,(void*)&vChar, sizeof(vChar)) )
			return -1;
		if(2 == vChar)
			{
			    boaWrite(wp, "checked");
			}
		return 0;

       	}
#endif


#ifdef DOMAIN_BLOCKING_SUPPORT
	else if ( !strcmp(name, "domainblk-cap0") ) {
   		if ( !mib_get_s( MIB_DOMAINBLK_CAPABILITY, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "domainblk-cap1") ) {
   		if ( !mib_get_s( MIB_DOMAINBLK_CAPABILITY, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
	else if ( !strcmp(name, "dns0") ) {
		if ( !mib_get_s( MIB_ADSL_WAN_DNS_MODE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "dns1") ) {
		if ( !mib_get_s( MIB_ADSL_WAN_DNS_MODE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#ifdef PORT_FORWARD_GENERAL
	else if ( !strcmp(name, "portFw-cap0") ) {
   		if ( !mib_get_s( MIB_PORT_FW_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "portFw-cap1") ) {
   		if ( !mib_get_s( MIB_PORT_FW_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "portFwNum")) {
		vUInt = mib_chain_total(MIB_PORT_FW_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
#endif
#ifdef PORT_TRIGGERING_DYNAMIC
	else if ( !strcmp(name, "portTrigger-cap0") ) {
		if ( !mib_get_s( MIB_PORT_TRIGGER_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "portTrigger-cap1") ) {
		if ( !mib_get_s( MIB_PORT_TRIGGER_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "portTriggerNum")) {
		vUInt = mib_chain_total(MIB_PORT_TRG_DYNAMIC_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
#endif
#ifdef NATIP_FORWARDING
	else if ( !strcmp(name, "ipFwEn")) {
		if ( !mib_get_s( MIB_IP_FW_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	if ( !strcmp(name, "ipFwNum")) {
		vUInt = mib_chain_total(MIB_IP_FW_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
#endif
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_RADVD
	else if ( !strcmp(name, "radvd_enable0")) {
		if ( !mib_get_s( MIB_V6_RADVD_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "radvd_enable1")) {
		if ( !mib_get_s( MIB_V6_RADVD_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
    else if ( !strcmp(name, "radvd_ManagedFlag0")) {
		if ( !mib_get_s( MIB_V6_MANAGEDFLAG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "radvd_ManagedFlag1")) {
		if ( !mib_get_s( MIB_V6_MANAGEDFLAG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "radvd_SendAdvert0")) {
		if ( !mib_get_s( MIB_V6_SENDADVERT, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "radvd_SendAdvert1")) {
		if ( !mib_get_s( MIB_V6_SENDADVERT, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "radvd_OtherConfigFlag0")) {
		if ( !mib_get_s( MIB_V6_OTHERCONFIGFLAG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "radvd_OtherConfigFlag1")) {
		if ( !mib_get_s( MIB_V6_OTHERCONFIGFLAG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "radvd_prefix_mode0")) {
		if ( !mib_get_s( MIB_V6_RADVD_PREFIX_MODE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "radvd_prefix_mode1")) {
		if ( !mib_get_s( MIB_V6_RADVD_PREFIX_MODE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "radvd_EnableULAFlag0")) {
		if ( !mib_get_s( MIB_V6_ULAPREFIX_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "radvd_EnableULAFlag1")) {
		if ( !mib_get_s( MIB_V6_ULAPREFIX_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "radvd_OnLink0")) {
		if ( !mib_get_s( MIB_V6_ONLINK, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "radvd_OnLink1")) {
		if ( !mib_get_s( MIB_V6_ONLINK, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "radvd_Autonomous0")) {
		if ( !mib_get_s( MIB_V6_AUTONOMOUS, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "radvd_Autonomous1")) {
		if ( !mib_get_s( MIB_V6_AUTONOMOUS, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif // of CONFIG_USER_RADVD
#endif
#ifdef IP_PORT_FILTER
	else if ( !strcmp(name, "ipf_out_act0")) {
		if ( !mib_get_s( MIB_IPF_OUT_ACTION, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "ipf_out_act1")) {
		if ( !mib_get_s( MIB_IPF_OUT_ACTION, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "ipf_in_act0")) {
		if ( !mib_get_s( MIB_IPF_IN_ACTION, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "ipf_in_act1")) {
		if ( !mib_get_s( MIB_IPF_IN_ACTION, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
#ifdef CONFIG_IPV6
	else if ( !strcmp(name, "v6_ipf_out_act0")) {
		if ( !mib_get_s( MIB_V6_IPF_OUT_ACTION, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "v6_ipf_out_act1")) {
		if ( !mib_get_s( MIB_V6_IPF_OUT_ACTION, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "v6_ipf_in_act0")) {
		if ( !mib_get_s( MIB_V6_IPF_IN_ACTION, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "v6_ipf_in_act1")) {
		if ( !mib_get_s( MIB_V6_IPF_IN_ACTION, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
	else if ( !strcmp(name, "macf_out_act0")) {
		if ( !mib_get_s( MIB_MACF_OUT_ACTION, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "macf_out_act1")) {
		if ( !mib_get_s( MIB_MACF_OUT_ACTION, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "macf_in_act0")) {
		if ( !mib_get_s( MIB_MACF_IN_ACTION, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "macf_in_act1")) {
		if ( !mib_get_s( MIB_MACF_IN_ACTION, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#ifdef DMZ
	else if ( !strcmp(name, "dmz-cap0") ) {
   		if ( !mib_get_s( MIB_DMZ_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "dmz-cap1") ) {
   		if ( !mib_get_s( MIB_DMZ_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
#ifdef CONFIG_USER_VLAN_ON_LAN
	else if ( !strcmp(name, "lan1-vid-cap0") ) {
		if ( !mib_chain_get(MIB_SW_PORT_TBL, 0, &sw_entry))
			return -1;
		if (0 == sw_entry.vlan_on_lan_enabled)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "lan1-vid-cap1") ) {
		if ( !mib_chain_get(MIB_SW_PORT_TBL, 0, &sw_entry))
			return -1;
		if (1 == sw_entry.vlan_on_lan_enabled)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "lan2-vid-cap0") ) {
		if ( !mib_chain_get(MIB_SW_PORT_TBL, 1, &sw_entry))
			return -1;
		if (0 == sw_entry.vlan_on_lan_enabled)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "lan2-vid-cap1") ) {
		if ( !mib_chain_get(MIB_SW_PORT_TBL, 1, &sw_entry))
			return -1;
		if (1 == sw_entry.vlan_on_lan_enabled)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "lan3-vid-cap0") ) {
		if ( !mib_chain_get(MIB_SW_PORT_TBL, 2, &sw_entry))
			return -1;
		if (0 == sw_entry.vlan_on_lan_enabled)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "lan3-vid-cap1") ) {
		if ( !mib_chain_get(MIB_SW_PORT_TBL, 2, &sw_entry))
			return -1;
		if (1 == sw_entry.vlan_on_lan_enabled)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "lan4-vid-cap0") ) {
		if ( !mib_chain_get(MIB_SW_PORT_TBL, 3, &sw_entry))
			return -1;
		if (0 == sw_entry.vlan_on_lan_enabled)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "lan4-vid-cap1") ) {
		if ( !mib_chain_get(MIB_SW_PORT_TBL, 3, &sw_entry))
			return -1;
		if (1 == sw_entry.vlan_on_lan_enabled)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
#ifdef IP_PORT_FILTER
	else if ( !strcmp(name, "ipFilterNum")) {
		vUInt = mib_chain_total(MIB_IP_PORT_FILTER_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
#endif
#ifdef CONFIG_IPV6
	else if ( !strcmp(name, "ipFilterNumV6")) {
		vUInt = mib_chain_total(MIB_V6_IP_PORT_FILTER_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
#endif
#ifdef TCP_UDP_CONN_LIMIT
	else if ( !strcmp(name, "connLimitNum")) {
		vUInt = mib_chain_total(MIB_TCP_UDP_CONN_LIMIT_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
#endif
#ifdef MULTI_ADDRESS_MAPPING
	else if ( !strcmp(name, "AddresMapNum")) {
		vUInt = mib_chain_total(MULTI_ADDRESS_MAPPING_LIMIT_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
#endif  // end of MULTI_ADDRESS_MAPPING
#ifdef URL_BLOCKING_SUPPORT
	else if ( !strcmp(name, "keywdNum")) {
		vUInt = mib_chain_total(MIB_KEYWD_FILTER_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelKeywdButton();");
		return 0;
	}
	else if ( !strcmp(name, "FQDNNum")) {
		vUInt = mib_chain_total(MIB_URL_FQDN_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelFQDNButton();");
		return 0;
	}
#endif
#ifdef DOMAIN_BLOCKING_SUPPORT
	else if ( !strcmp(name, "domainNum")) {
		vUInt = mib_chain_total(MIB_DOMAIN_BLOCKING_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
#endif
	else if ( !strcmp(name, "ripNum")) {
		vUInt = mib_chain_total(MIB_RIP_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
	else if ( !strcmp(name, "ripprotocol")) {
		unsigned char ospfEnable=0;
		unsigned char ripEnable=0;
		int enable = 0;
#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
		mib_get_s(MIB_OSPF_ENABLE, (void *)&ospfEnable, sizeof(ospfEnable));
#endif
		mib_get_s(MIB_RIP_ENABLE, (void *)&ripEnable, sizeof(ripEnable));
		if(ospfEnable==0 && ripEnable==0) enable = 0;
#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
		else if(ospfEnable==1 && ripEnable==0) enable = 2;
#endif
		else if(ospfEnable==0 && ripEnable==1) enable = 1;
#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
		else if(ospfEnable==1 && ripEnable==1) enable = 1;
#endif

		boaWrite(wp, "%d", enable);
		return 0;
	}
#ifdef IP_ACL
	else if ( !strcmp(name, "aclNum")) {
		vUInt = mib_chain_total(MIB_ACL_IP_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
#endif
#if defined(CONFIG_IPV6) && defined(IP_ACL)
	else if ( !strcmp(name, "v6aclNum")) {
		vUInt = mib_chain_total(MIB_V6_ACL_IP_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
#endif
#ifdef MAC_FILTER
	else if ( !strcmp(name, "macFilterNum")) {
		vUInt = mib_chain_total(MIB_MAC_FILTER_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
	else if ( !strcmp(name, "macfilter-ctrl-on-0") ) {
		if ( !mib_get_s( MIB_MAC_FILTER_EBTABLES_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "macfilter-ctrl-on-1") ) {
		if ( !mib_get_s( MIB_MAC_FILTER_EBTABLES_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
#ifdef PARENTAL_CTRL
	else if ( !strcmp(name, "parentCtrlNum")) {
		vUInt = mib_chain_total(MIB_PARENTAL_CTRL_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
	else if ( !strcmp(name, "parental-ctrl-on-0") ) {
		if ( !mib_get_s( MIB_PARENTAL_CTRL_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "parental-ctrl-on-1") ) {
		if ( !mib_get_s( MIB_PARENTAL_CTRL_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
	else if ( !strcmp(name, "vcMax")) {
		vUInt = mib_chain_total(MIB_ATM_VC_TBL);
		if (vUInt >= 16) {
			boaWrite(wp, "alert(\"Max number of ATM VC Settings is 16!\");");
			boaWrite(wp, "return false;");
		}
		return 0;
	}
	else if ( !strcmp(name, "vcCount")) {
		vUInt = mib_chain_total(MIB_ATM_VC_TBL);
		if (vUInt == 0) {
			boaWrite(wp, "disableButton(document.adsl.delvc);");
			// Commented by Mason Yu. The "refresh" button is be disabled on wanadsl.asp
			//boaWrite(wp, "disableButton(document.adsl.refresh);");
		}
		return 0;
	}
#ifdef CONFIG_USER_PPPOE_PROXY
  else if(!strcmp(name,"pppoeProxy"))
  	{
  	boaWrite(wp,"<tr><td><font size=2><b>PPPoE Proxy:</b></td>"
         "<td><b><input type=\"radio\" value=1 name=\"pppEnable\" >Enable&nbsp;&nbsp;"
	"<input type=\"radio\" value=0 name=\"pppEnable\" checked>Disable</b></td></tr>");
  	}
  else if(!strcmp(name,"pppSettingsDisable"))
  	{
  	  boaWrite(wp,"{document.adsl.pppEnable[0].disabled=true;\n"
	  	"document.adsl.pppEnable[1].disabled=true;}");
  	}
    else if(!strcmp(name,"pppSettingsEnable"))
  	{
  	  boaWrite(wp,"{document.adsl.pppEnable[0].disabled=false;\n"
	  	"document.adsl.pppEnable[1].disabled=false;}else{document.adsl.pppEnable[0].disabled=true;\n"
	  	"document.adsl.pppEnable[1].disabled=true;}"
	  	"document.adsl.pppEnable[0].checked=false;"
	  	"document.adsl.pppEnable[1].checked=true;");
  	}

 #endif
  #ifdef CONFIG_USER_PPPOE_PROXY
     else if(!strcmp(name,"pppoeProxyEnable"))
     	{
	boaWrite(wp,"  if(mode==\"PPPoE\")"
		"{if(pppoeProxyEnable)"
		"{ document.adsl.pppEnable[0].checked=true;\n"
                  "document.adsl.pppEnable[1].checked=false;}\n"
		"else {document.adsl.pppEnable[0].checked=false;"
		 " document.adsl.pppEnable[1].checked=true;}  "
		" document.adsl.pppEnable[0].disabled=false;"
			  " document.adsl.pppEnable[1].disabled=false;");
	boaWrite(wp," }else"
		"{"
		"	  document.adsl.pppEnable[0].checked=false;"
		"	   document.adsl.pppEnable[1].checked=true;"
		"	   document.adsl.pppEnable[0].disabled=true;"
		"	   document.adsl.pppEnable[1].disabled=true;}"
		);
     	}
  #else
     else if(!strcmp(name,"pppoeProxyEnable"))
     	{

     	}
  #endif
   else if(!strcmp(name,"pc_mac")) {
#ifndef CONFIG_USER_MAC_CLONE
		  boaWrite(wp, "");
#else
		  int i=0;
		  int j=0;
		  char clone_mac[12];
		  intVal=get_mac_from_IP(tmpStr, wp->remote_ip_addr);
		  if(intVal==1)
		  {
			for(i=0, j=0; i<17 && j<12; i++)
			{
				if(tmpStr[i]!=':')
					clone_mac[j++]=tmpStr[i];
			}
			clone_mac[12]=0;
			boaWrite(wp,"%s",clone_mac);
		  }
		  else
			boaWrite(wp, "");
#endif
		 return 0;
	  }
#ifdef WLAN_SUPPORT
	else if ( !strcmp(name, "wl_txRate")) {
		struct _misc_data_ misc_data;
		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
#ifdef WLAN_BAND_CONFIG_MBSSID
		mib_get_s( MIB_WLAN_BAND, (void *)&vChar, sizeof(vChar));
		boaWrite(wp, "band=%d\n", vChar);
#else
		boaWrite(wp, "band=%d\n", Entry.wlanBand);
#endif
		boaWrite(wp, "txrate=%u\n",Entry.fixedTxRate);
		boaWrite(wp, "auto=%d\n",Entry.rateAdaptiveEnabled);
		mib_get_s( MIB_WLAN_CHANNEL_WIDTH, (void *)&vChar, sizeof(vChar));
		boaWrite(wp, "chanwid=%d\n",vChar);

		//cathy, get rf number
		memset(&misc_data, 0, sizeof(struct _misc_data_));
		getMiscData(getWlanIfName(), &misc_data);
		boaWrite(wp, "rf_num=%u\n", misc_data.mimo_tr_used);
		return 0;
	}
	else if ( !strcmp(name, "wl_chno")) {
//xl_yue:
#if defined(TRIBAND_SUPPORT)
		mib_get_s( MIB_HW_REG_DOMAIN, (void *)&vChar, sizeof(vChar));
#else
#ifdef WLAN_DUALBAND_CONCURRENT
#ifdef BAND_2G_ON_WLAN0
		mib_get_s(MIB_HW_WLAN1_REG_DOMAIN, (void *)&vChar, sizeof(vChar));
#else
		mib_get_s( MIB_HW_REG_DOMAIN, (void *)&vChar, sizeof(vChar));
#endif
#else
		mib_get_s( MIB_HW_REG_DOMAIN, (void *)&vChar, sizeof(vChar));
#endif
#endif
		boaWrite(wp, "regDomain=%d\n",vChar);
#ifdef WLAN1_QTN
		mib_get_s( MIB_HW_WLAN1_REG_DOMAIN, (void *)&vChar, sizeof(vChar));
		boaWrite(wp, "regDomain_qtn=%d\n",vChar);
#elif defined(WLAN0_QTN)
		mib_get_s( MIB_HW_REG_DOMAIN, (void *)&vChar, sizeof(vChar));
		boaWrite(wp, "regDomain_qtn=%d\n",vChar);
#endif
		mib_get_s( MIB_WLAN_AUTO_CHAN_ENABLED,(void *)&vChar, sizeof(vChar));
		if(vChar)
			boaWrite(wp, "defaultChan=0\n");
		else
		{
			mib_get_s( MIB_WLAN_CHAN_NUM ,(void *)&vChar, sizeof(vChar));
			boaWrite(wp, "defaultChan=%d\n",vChar);
		}
		return 0;
	}
	else if ( !strcmp(name, "rf_used")) {
		struct _misc_data_ misc_data;
		memset(&misc_data, 0, sizeof(struct _misc_data_));
		getMiscData(getWlanIfName(), &misc_data);
		boaWrite(wp, "%u\n", misc_data.mimo_tr_used);
		return 0;
	}
#endif

	//for web log
	else if ( !strcmp(name, "log-cap0") ) {
   		if ( !mib_get_s( MIB_SYSLOG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "log-cap1") ) {
   		if ( !mib_get_s( MIB_SYSLOG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	//for web holepunching
	else if ( !strcmp(name, "hole-cap0") ) {
   		if ( !mib_get_s( MIB_HOLE_PUNCHING, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "hole-cap1") ) {
   		if ( !mib_get_s( MIB_HOLE_PUNCHING, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}

#ifdef CONFIG_USER_SAMBA
	else if (!strcmp(name, "samba-cap0")) {
		if (!mib_get_s(MIB_SAMBA_ENABLE, &vChar, sizeof(vChar)))
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	} else if (!strcmp(name, "samba-cap1")) {
		if (!mib_get_s(MIB_SAMBA_ENABLE, &vChar, sizeof(vChar)))
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if (!strcmp(name, "nmbd-cap")) {
#ifndef CONFIG_USER_NMBD
		boaWrite(wp, "style=\"display: none\"");
#endif
		return 0;
	}
	else if (!strcmp(name, "smbSecurity")) {
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
		if (!mib_get_s(MIB_SMB_ANNONYMOUS, &vChar, sizeof(vChar)))
			return -1;

		boaWrite(wp,"<tr>"
			"<th>Security&nbsp;:</th>"
			"<td>"
			"	   <input type=\"radio\" value=\"0\" name=\"smbSecurityCap\" %s>%s&nbsp;&nbsp;"
			"	   <input type=\"radio\" value=\"1\" name=\"smbSecurityCap\" %s>%s"
			"</td>"
			"</tr>", (1 == vChar)?"checked":"", multilang(LANG_DISABLE), (0 == vChar)?"checked":"", multilang(LANG_ENABLE));
#endif
		return 0;
	}
#endif
#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_AWIFI_SUPPORT)
	else if (!strcmp(name, "aWiFi-cap0")) {
		if (!mib_get_s(MIB_AWIFI_ENABLE, &vChar, sizeof(vChar)))
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	} else if (!strcmp(name, "aWiFi-cap1")) {
		if (!mib_get_s(MIB_AWIFI_ENABLE, &vChar, sizeof(vChar)))
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "wifidogd") ) {
		if (ifWanNum("rt") ==0)
			boaWrite(wp, "disabled");
		return 0;
	}
	else if ( !strcmp(name, "aWiFiMacListNum")) {
		vUInt = mib_chain_total(MIB_AWIFI_MAC_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
	else if ( !strcmp(name, "aWiFiUrlListNum")) {
		vUInt = mib_chain_total(MIB_AWIFI_URL_TBL);
		if (0 == vUInt)
			boaWrite(wp, "disableDelButton();");
		return 0;
	}
#ifdef CONFIG_USER_AWIFI_AUDIT_SUPPORT
	else if (!strcmp(name, "rtk_aWiFi-audit-cap0")) {
		if (!mib_get_s(MIB_RTK_AWIFI_AUDIT_TYPE, &vChar, sizeof(vChar)))
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	} else if (!strcmp(name, "rtk_aWiFi-audit-cap1")) {
		if (!mib_get_s(MIB_RTK_AWIFI_AUDIT_TYPE, &vChar, sizeof(vChar)))
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
#endif
	else if (!strcmp(name, "anxb-cap")) {
		int anxb_feature;
		if (boaArgs(argc, argv, "%*s %d", &vUInt) == 1) {
			if (vUInt == 1)
				anxb_feature = 1;
			else
				anxb_feature = 0;
		}
		else
			anxb_feature = 0;
		//mib_get_s(MIB_ADSL_MODE, (void *)&vUShort, sizeof(vUShort));
		mib_get_s( MIB_DSL_ANNEX_B, (void *)&vChar, sizeof(vChar));
		//if(vUShort&ADSL_MODE_ANXB)
		if (anxb_feature) {
			if (!vChar)
				boaWrite(wp, "style=\"display: none\"");
		}
		else {
			if (vChar)
				boaWrite(wp, "style=\"display: none\"");
		}
		return 0;
	}
	else if (!strcmp(name, "ginp-cap")) {
#ifndef ENABLE_ADSL_MODE_GINP
		boaWrite(wp, "style=\"display: none\"");
#endif
		return 0;
	}
	else if (!strcmp(name, "vdsl-cap")) {
#ifndef CONFIG_VDSL
		boaWrite(wp, "style=\"display: none\"");
#endif
		return 0;
	}
	else if (!strcmp(name, "IPQoS")) {
#if !defined(CONFIG_USER_IP_QOS) || defined(BRIDGE_ONLY_ON_WEB)
		boaWrite(wp, "style=\"display: none\"");
#endif
		return 0;
	}
	else if (!strcmp(name, "bridge-only")) {
#ifdef CONFIG_SFU
		boaWrite(wp, "style=\"display: none\"");
#endif
		return 0;
	}
#ifdef CONFIG_00R0
	else if (!strcmp(name, "showWiFi")) {
#ifndef CONFIG_WLAN
		boaWrite(wp, "style=\"display: none\"");
#endif
		return 0;
	}
	if(!strcmp(name, "wlan_hidden_function")){
#ifdef WLAN_SUPPORT
		if(wlan_idx==1)
			boaWrite(wp, "style=\"display: none\"");
#endif
		return 0;
	}
#endif

#ifndef USE_DEONET
	if (!strcmp(name, "syslog-log") || !strcmp(name, "syslog-display")) {
#else
	if (!strcmp(name, "syslog-log") || !strcmp(name, "syslog-remote")) {
#endif
		char *SYSLOGLEVEL[] = {"Emergency", "Alert", "Critical", "Error", "Warning", "Notice", "Infomational", "Debugging"};
		int i;
		if (!strcmp(name, "syslog-log")) {
			if (!mib_get_s(MIB_SYSLOG_LOG_LEVEL, (void *)&vChar, sizeof(vChar)))
				return -1;
		}
#ifndef USE_DEONET
		else if (!strcmp(name, "syslog-display")) {
			if (!mib_get_s(MIB_SYSLOG_DISPLAY_LEVEL, (void *)&vChar, sizeof(vChar)))
				return -1;
		}
#else
		else if (!strcmp(name, "syslog-remote")) {
			if (!mib_get_s(MIB_SYSLOG_REMOTE_LEVEL, (void *)&vChar, sizeof(vChar)))
				return -1;
		}

#endif
		for (i=0; i<8; i++) {
			if (i == vChar)
				boaWrite(wp,"<option selected value=\"%d\">%s</option>", i, SYSLOGLEVEL[i]);
			else
				boaWrite(wp,"<option value=\"%d\">%s</option>", i, SYSLOGLEVEL[i]);
		}
		return 0;
	}
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
	if (!strcmp(name, "syslog-mode")) {
		char *SYSLOGMODE[] = { "", "Local", "Remote", "Local/Remote" };
		int i;
		if (!mib_get_s(MIB_SYSLOG_MODE, &vChar, sizeof(vChar)))
			return -1;
		for (i = 1; i <= 3; i++) {
			if (i == vChar)
				boaWrite(wp, "<option selected value=\"%d\">%s</option>", i, SYSLOGMODE[i]);
			else if (i == 2) /* not used */
				boaWrite(wp, "<option value=\"%d\" style=\"display:none;\"></option>", i);
			else
				boaWrite(wp, "<option value=\"%d\">%s</option>", i, SYSLOGMODE[i]);
		}
	}
#endif

#ifdef _CWMP_MIB_
	else if ( !strcmp(name, "tr069-interval") ) {
   		if ( !mib_get_s( CWMP_INFORM_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "disabled");
		return 0;
	}
	else if ( !strcmp(name, "tr069-inform-0") ) {
   		if ( !mib_get_s( CWMP_INFORM_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "tr069-inform-1") ) {
   		if ( !mib_get_s( CWMP_INFORM_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#ifdef _TR111_STUN_
	else if ( !strcmp(name, "tr069-stun") ) {
		if ( !mib_get( TR111_STUNENABLE, (void *)&vChar) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "disabled=\"disabled\"");
		return 0;
	}
	else if ( !strcmp(name, "stun-0") ) {
		if ( !mib_get( TR111_STUNENABLE, (void *)&vChar) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "stun-1") ) {
		if ( !mib_get( TR111_STUNENABLE, (void *)&vChar) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
	else if ( !strcmp(name, "tr069-dbgmsg-0") ) {
   		if ( !mib_get_s( CWMP_FLAG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if ( (vChar & CWMP_FLAG_DEBUG_MSG)==0 )
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "tr069-dbgmsg-1") ) {
   		if ( !mib_get_s( CWMP_FLAG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if ( (vChar & CWMP_FLAG_DEBUG_MSG)!=0 )
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "tr069-sendgetrpc-0") ) {
   		if ( !mib_get_s( CWMP_FLAG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if ( (vChar & CWMP_FLAG_SENDGETRPC)==0 )
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "tr069-sendgetrpc-1") ) {
   		if ( !mib_get_s( CWMP_FLAG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if ( (vChar & CWMP_FLAG_SENDGETRPC)!=0 )
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "tr069-skipmreboot-0") ) {
   		if ( !mib_get_s( CWMP_FLAG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if ( (vChar & CWMP_FLAG_SKIPMREBOOT)==0 )
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "tr069-skipmreboot-1") ) {
   		if ( !mib_get_s( CWMP_FLAG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if ( (vChar & CWMP_FLAG_SKIPMREBOOT)!=0 )
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "tr069-delay-0") ) {
   		if ( !mib_get_s( CWMP_FLAG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if ( (vChar & CWMP_FLAG_DELAY)==0 )
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "tr069-delay-1") ) {
   		if ( !mib_get_s( CWMP_FLAG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if ( (vChar & CWMP_FLAG_DELAY)!=0 )
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "tr069-autoexec-0") ) {
   		if ( !mib_get_s( CWMP_FLAG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if ( (vChar & CWMP_FLAG_AUTORUN)==0 )
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "tr069-autoexec-1") ) {
   		if ( !mib_get_s( CWMP_FLAG, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if ( (vChar & CWMP_FLAG_AUTORUN)!=0 )
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "tr069-enable-cwmp-1") ) {
   		if ( !mib_get_s( CWMP_FLAG2, (void *)&vUInt, sizeof(vUInt)) )
			return -1;
		if ( (vUInt & CWMP_FLAG2_CWMP_DISABLE)==0 )
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "tr069-enable-cwmp-0") ) {
   		if ( !mib_get_s( CWMP_FLAG2, (void *)&vUInt, sizeof(vUInt)) )
			return -1;
		if ( (vUInt & CWMP_FLAG2_CWMP_DISABLE)!=0 )
			boaWrite(wp, "checked");
		return 0;
	}
	else if (!strcmp(name, "tr069-enable-acl-0")) {
		if (!mib_get_s(CWMP_ACL_ENABLE, &vChar, sizeof(vChar)))
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	} else if (!strcmp(name, "tr069-enable-acl-1")) {
		if (!mib_get_s(CWMP_ACL_ENABLE, &vChar, sizeof(vChar)))
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
#ifdef WLAN_SUPPORT
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS) // WPS
#if defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
	else if(!strcmp(name, "autolockdown_support")){
		boaWrite(wp, "autolockdown_support = 0;");
		return 0;
	}
#endif
	else if (!strcmp(name, "wpsVer") ) {
		#ifdef WPS20
			boaWrite(wp, "wps20 = 1;\n");
		#else
			boaWrite(wp, "wps20 = 0;\n");
		#endif
		return 0;
	}
	else if (!strcmp(name, "wpa3_disable_wps") ){
#ifdef WLAN_WPA3_NOT_SUPPORT_WPS
		boaWrite(wp, "wpa3_disable_wps = 1;\n");
#else
		boaWrite(wp, "wpa3_disable_wps = 0;\n");
#endif
		return 0;
	}
	else if (!strcmp(name, "wpsVerConfig") ) {
		#ifdef WPS_VERSION_CONFIGURABLE
			boaWrite(wp, "wps_version_configurable = 1;\n");
		#else
			boaWrite(wp, "wps_version_configurable = 0;\n");
		#endif
		return 0;
	}
	else if (!strcmp(name, "wpsUseVersion")) {
		boaWrite(wp, "<option value=0>V1</option>\n");
		boaWrite(wp, "<option value=1>V2</option>\n");
		return 0;
	}
	else if (!strcmp(name, "wscConfig-0") ) {
		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		if (!Entry.wsc_configured) boaWrite(wp, "checked");
		return 0;
	}
	else if (!strcmp(name, "wscConfig-1")) {
		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		if (Entry.wsc_configured)
			boaWrite(wp, "checked");
		return 0;
	}
	else if (!strcmp(name, "wscConfig-A")) {
		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		if (Entry.wsc_configured)
			boaWrite(wp, "isConfig=1;");
		else
			boaWrite(wp, "isConfig=0;");
		return 0;
	}
	else if (!strcmp(name,"wscConfig")){
		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		if (Entry.wsc_configured)
			boaWrite(wp, "enableButton(form.elements['resetUnConfiguredBtn']);");
		else
			boaWrite(wp, "disableButton(form.elements['resetUnConfiguredBtn']);");
		return 0;
	}
	else if (!strcmp(name, "wlanMode"))  {
		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		if (Entry.wlanMode == CLIENT_MODE)
			boaWrite(wp, "isClient=1;");
		else
			boaWrite(wp, "isClient=0;");
		return 0;

	}
//#ifdef CONFIG_RTK_DEV_AP
	else if (!strcmp(name, "wlanOpMode")) {
		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		boaWrite(wp, "%d", Entry.wlanMode);
		return 0;
	}
//#endif
#ifdef CONFIG_ELINK_SUPPORT
	else if(!strcmp(name,"elink_enable"))
	{
		mib_get_s(MIB_RTL_LINK_ENABLE,&vChar,sizeof(vChar));
		if(vChar==1)
			boaWrite(wp,"%d",1);
		else
			boaWrite(wp,"%d",0);
	}
	else if(!strcmp(name,"elink_sync"))
	{
		int enabled;
		mib_get_s(MIB_RTL_LINK_SYNC,&vChar,sizeof(vChar));
		if(vChar==1)
			boaWrite(wp,"%d",1);
		else
			boaWrite(wp,"%d",0);
	}
	else if(!strcmp(name, "elink_cloud_sdk"))
	{
#ifdef CONFIG_ELINKSDK_SUPPORT
		boaWrite(wp,"%d",1);
#else
		boaWrite(wp,"%d",0);
#endif
	}
#ifdef CONFIG_ELINKSDK_SUPPORT
	else if(!strcmp(name, "elinksdkVersion"))
	{
		int exist;
		char elinkSdkVersion[ELINKSDK_PARAM_VERSION_LEN]={0};
		char config_file[128]={0};

		if(rtl_elinksdk_is_alive(&exist) == ELINKSDK_FILE_NOT_EXIST){
				boaWrite(wp,"%s",multilang(LANG_ELINK_UNINSTALL));
		}else{
			if(exist){
				snprintf(config_file, sizeof(config_file), "%s/%s", RTL_ELINKSDK_DIR, RTL_ELINKSDK_CONFIG);
				rtl_elinksdk_config_parse(config_file, ELINKSDK_PARAM_VERSION, elinkSdkVersion);
				boaWrite(wp,"%s",elinkSdkVersion);
			}else{
				boaWrite(wp,"%s",multilang(LANG_ELINK_UNINSTALL));
			}
		}
	}
#endif
#endif
#ifdef WLAN_SCHEDULE_SUPPORT
	else if(!strcmp(name, "maxWebWlSchNum")){
		boaWrite(wp, "%d",MAX_WLAN_SCHEDULE_NUM);
	}

	else if(!strcmp(name, "wlsch_Onoff")){
		mib_get_s(MIB_WLAN_SCHEDULE_ENABLED, &vUInt,sizeof(vUInt));
		if (vUInt==0){
			boaWrite(wp,"%d",0);
		}else{
			boaWrite(wp,"%d",1);
		}
	}
	else if(!strcmp(name, "wlsch_mode")){
		mib_get_s(MIB_WLAN_SCHEDULE_MODE,&vUInt,sizeof(vUInt));
		if(vUInt==ELINK_TIMER_MODE)
					boaWrite(wp,"%d",ELINK_TIMER_MODE);
		else if(vUInt==DEFAULT_TIMER_MODE)
					boaWrite(wp,"%d",DEFAULT_TIMER_MODE);
		else//andlink
			boaWrite(wp,"%d",DEFAULT_TIMER_MODE);
		}
#endif //end WLAN_SCHEDULE
	else if (!strcmp(name, "wscDisable"))  {

		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		if (Entry.wsc_disabled)
			boaWrite(wp, "checked");
		return 0;
	}
	else if (!strcmp(name, "wps_auth"))  {

		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		if(Entry.encrypt == WIFI_SEC_WEP){
			switch(Entry.authType){
				case AUTH_OPEN: boaWrite(wp, "Open"); break;
				case AUTH_SHARED: boaWrite(wp, "Shared"); break;
				case AUTH_BOTH: boaWrite(wp, "Auto"); break;
				default: break;
			}
		}
		else{
			switch(Entry.wsc_auth) {
				case WSC_AUTH_OPEN: boaWrite(wp, "Open"); break;
				case WSC_AUTH_WPAPSK: boaWrite(wp, "WPA PSK"); break;
				case WSC_AUTH_SHARED: boaWrite(wp, "WEP Shared"); break;
				case WSC_AUTH_WPA: boaWrite(wp, "WPA Enterprise"); break;

				case WSC_AUTH_WPA2: boaWrite(wp, "WPA2 Enterprise"); break;
				case WSC_AUTH_WPA2PSK:
#ifdef WLAN_WPA3
					if(Entry.encrypt == WIFI_SEC_WPA3)
						boaWrite(wp, "WPA3 SAE");
					else if(Entry.encrypt == WIFI_SEC_WPA2_WPA3_MIXED)
#ifdef WLAN_WPA3_H2E
						boaWrite(wp, "WPA3 Transition");
#else
						boaWrite(wp, "WPA2 PSK/WPA3 SAE");
#endif
					else
						boaWrite(wp, "WPA2 PSK");

#else
					boaWrite(wp, "WPA2 PSK");

#endif
					break;
				case WSC_AUTH_WPA2PSKMIXED: boaWrite(wp, "WPA2-Mixed PSK"); break;
				default:
					break;
			}
		}
		return 0;
	}
	else if (!strcmp(name, "wps_enc"))  {
		MIB_CE_MBSSIB_T Entry;
		if(!wlan_getEntry(&Entry, 0))
			return -1;
		vChar = Entry.encrypt;
		if ((WIFI_SECURITY_T)vChar == WIFI_SEC_WPA2_MIXED) {
			vUInt = 0;
			vUInt |= Entry.unicastCipher;
			vUInt |= Entry.wpa2UnicastCipher;
			switch (vUInt) {
				case WPA_CIPHER_TKIP: boaWrite(wp, "TKIP"); break;
				case WPA_CIPHER_AES: boaWrite(wp, "AES"); break;
				case WPA_CIPHER_MIXED: boaWrite(wp, "TKIP+AES"); break;
				default:
					break;
			}
		}
		else {
			switch(Entry.wsc_enc) {
				case 0:
				case WSC_ENCRYPT_NONE: boaWrite(wp, "None"); break;
				case WSC_ENCRYPT_WEP: boaWrite(wp, "WEP"); break;
				case WSC_ENCRYPT_TKIP: boaWrite(wp, "TKIP"); break;
				case WSC_ENCRYPT_AES: boaWrite(wp, "AES"); break;
				case WSC_ENCRYPT_TKIPAES: boaWrite(wp, "TKIP+AES"); break;
				default:
					break;
			}
		}
		return 0;
	}
	else if(!strcmp(name, "blockingvap"))
		{
#ifdef CONFIG_GENERAL_WEB
			boaWrite(wp,"<table>\n");
#endif
			boaWrite(wp, "<tr><td><b>%s%s", multilang(LANG_BLOCKING_BETWEEN_VAP),"</b></td>");
			boaWrite(wp, "<td><input type=\"radio\" name=\"mbssid_block\" value=\"disable\" >%s", multilang(LANG_DISABLE));
			boaWrite(wp, "<input type=\"radio\" name=\"mbssid_block\" value=\"enable\" >%s%s",multilang(LANG_ENABLE),"</td></tr>");
#ifdef CONFIG_GENERAL_WEB
			boaWrite(wp,"</table>");
#endif
		}
#endif
#endif
	// Jenny, for RFC1577
	else if(!strcmp(name,"naptEnable"))
	{
		boaWrite(wp,"\tif ((document.adsl.adslConnectionMode.selectedIndex == 1) ||\n"
			"\t\t(document.adsl.adslConnectionMode.selectedIndex == 2))\n"
			"\t\tdocument.adsl.naptEnabled.checked = true;\n"
			"\telse\n"
			"\t\tdocument.adsl.naptEnabled.checked = false;\n");
		return 0;
	}
	else if(!strcmp(name,"TrafficShapingByVid"))
	{
		boaWrite(wp,"0");
	}
	else if(!strcmp(name,"TrafficShapingBySsid"))
	{
		boaWrite(wp,"0");
	}
	else if(!strcmp(name,"IPv6Show"))
	{
#ifdef CONFIG_IPV6
		if ( mib_get_s( MIB_V6_IPV6_ENABLE, (void *)&vChar, sizeof(vChar)) )
		{
			if (0 == vChar)
				boaWrite(wp,"0");
			else
				boaWrite(wp,"1");
		}
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name,"Internet OP Mode"))
	{
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
		if ( mib_get( MIB_OP_MODE, (void *)&vChar) )
		{
			if (0 == vChar)
				boaWrite(wp,"0");
			else

#if defined(WLAN_UNIVERSAL_REPEATER) && defined(CONFIG_USER_RTK_REPEATER_MODE)
				if (mib_get( MIB_REPEATER_MODE, (void *)&vChar))
				{
					if (0 == vChar)
						boaWrite(wp,"1");
					else
						boaWrite(wp,"3");
				}
#else

				boaWrite(wp,"1");
#endif
		}
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name,"ConfigIPv6"))
	{
#ifdef CONFIG_IPV6
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name, "ConfigIPv6_wan_auto_detect"))
	{
#if defined(CONFIG_IPV6) && defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name, "IPv6_auto_mode_string"))
	{
#if defined(CONFIG_IPV6) && defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
		boaWrite(wp,multilang(LANG_AUTO_DETECT_MODE));
#endif
		return 0;
	}
	else if(!strcmp(name,"mcastVlan"))
	{
#ifdef CONFIG_DEV_xDSL
		boaWrite(wp,"0");
#else
		boaWrite(wp,"1");
#endif
		return 0;
	}
	else if(!strcmp(name,"LUNAShow"))
	{
#ifdef CONFIG_LUNA
#ifndef	CONFIG_RTK_DEV_AP
		boaWrite(wp,"1");
#else
		// show wan interface in qos rules
		boaWrite(wp,"0");
#endif
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name,"ndp_proxy"))
	{
#if defined(CONFIG_IPV6) &&  defined (CONFIG_USER_NDPPD)
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name,"napt_v6"))
	{
#if defined(CONFIG_IPV6) && defined(CONFIG_IP6_NF_TARGET_NPT) && defined(CONFIG_IP6_NF_TARGET_MASQUERADE) && defined (CONFIG_IP6_NF_NAT)
		   boaWrite(wp,"1");
#else
		   boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name,"DSLiteShow"))
	{
#ifdef DUAL_STACK_LITE
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name, "MAPEShow")){	//MAP-E
#ifdef CONFIG_USER_MAP_E
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}else if (!strcmp(name, "mape_routing_mode_support")){
#if defined(CONFIG_USER_MAP_E) && defined(CONFIG_MAPE_ROUTING_MODE)
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if (!strcmp(name,"show_mape_fmrs")){
#ifdef CONFIG_USER_MAP_E
		show_MAPE_FMR_list(wp);
#else
		boaWrite(wp,"");
#endif
		return 0;
	}else if (!strcmp(name, "mape_fmr_wan_ifIndex")){
#ifdef CONFIG_USER_MAP_E
		mib_get_s(MIB_MAPE_FMR_LIST_CUR_WAN_IFINDEX, (void *)&vUInt, sizeof(vUInt));
		boaWrite(wp, "%d", vUInt);
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name, "MAPTShow")){ //MAP-T
#ifdef CONFIG_USER_MAP_T
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name, "XLAT464Show")){ //xlat464
#ifdef CONFIG_USER_XLAT464
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name, "LW4O6Show")){ //lw4o6
#ifdef CONFIG_USER_LW4O6
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name,"6rdShow"))
	{
#ifdef CONFIG_IPV6_SIT_6RD
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
        else if(!strcmp(name,"6in4Tunnelshow"))
	{
#ifdef CONFIG_IPV6_SIT
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
#ifdef CONFIG_USER_RTK_NAT_ALG_PASS_THROUGH
	else if(!strcmp(name,"GetAlgType"))
		{
		GetAlgTypes(wp);
		return 0;
		}
	else if(!strcmp(name,"AlgTypeStatus"))
		{
		CreatejsAlgTypeStatus( wp);
	 	return 0;
		}
#endif
#ifdef DNS_BIND_PVC_SUPPORT
	else if(!strcmp(name,"DnsBindPvc"))
		{
		unsigned char dnsBindPvcEnable=0;
		mib_get_s(MIB_DNS_BIND_PVC_ENABLE,(void*)&dnsBindPvcEnable, sizeof(dnsBindPvcEnable));
		//printf("dns bind pvc = %d\n",dnsBindPvcEnable);
#ifndef CONFIG_GENERAL_WEB
		boaWrite(wp,"<font size=2>IPv4 %s:<input type=\"checkbox\" name=\"enableDnsBind\" value=\"on\" %s onClick=\"DnsBindPvcClicked();\"></font>",
#else
		boaWrite(wp,"<tr><th>IPv4 %s:</th><td><input type=\"checkbox\" name=\"enableDnsBind\" value=\"on\" %s onClick=\"DnsBindPvcClicked();\"></td></tr>",
#endif
		multilang(LANG_WAN_INTERFACE_BINDING), (dnsBindPvcEnable) ? "checked" : "");
		return 0;
		}
#endif
#ifdef CONFIG_IPV6
#ifdef DNSV6_BIND_PVC_SUPPORT
	else if(!strcmp(name,"Dnsv6BindPvc"))
		{
		unsigned char dnsv6BindPvcEnable=0;
		mib_get_s(MIB_DNSV6_BIND_PVC_ENABLE,(void*)&dnsv6BindPvcEnable, sizeof(dnsv6BindPvcEnable));
		//printf("dnsv6 bind pvc = %d\n",dnsv6BindPvcEnable);
#ifndef CONFIG_GENERAL_WEB
		boaWrite(wp,"<font size=2>IPv6 %s:<input type=\"checkbox\" name=\"enableDnsv6Bind\" value=\"on\" %s onClick=\"Dnsv6BindPvcClicked();\"></font>",
#else
		boaWrite(wp,"<tr><th>IPv6 %s:</th><td><input type=\"checkbox\" name=\"enableDnsv6Bind\" value=\"on\" %s onClick=\"Dnsv6BindPvcClicked();\"></td></tr>",
#endif
		multilang(LANG_WAN_INTERFACE_BINDING), (dnsv6BindPvcEnable) ? "checked" : "");
		return 0;
		}
#endif
#endif
	else  if(!strcmp(name,"WanPvcRouter"))
		{
#ifdef DNS_BIND_PVC_SUPPORT
				MIB_CE_ATM_VC_T Entry;
				int entryNum;
				int mibcnt;
				char interfacename[MAX_NAME_LEN];
				entryNum = mib_chain_total(MIB_ATM_VC_TBL);
				unsigned char forSelect=0;
		            for(mibcnt=0;mibcnt<entryNum;mibcnt++)
		            {
		            if (!mib_chain_get(MIB_ATM_VC_TBL, mibcnt, (void *)&Entry))
						{
		  					boaError(wp, 400, "Get chain record error!\n");
							return -1;
						}
			      if(Entry.cmode!=CHANNEL_MODE_BRIDGE)// CHANNEL_MODE_BRIDGE CHANNEL_MODE_IPOE CHANNEL_MODE_PPPOE CHANNEL_MODE_PPPOA	CHANNEL_MODE_RT1483	CHANNEL_MODE_RT1577
			      	{
			      	boaWrite(wp,"0");
				return 0;
		                  }

		            }
		           boaWrite(wp,"1");
#else
	 		 boaWrite(wp,"0");
#endif
			return 0;

		}

#ifdef DNS_BIND_PVC_SUPPORT
	else if(!strcmp(name,"dnsBindPvcInit"))
			{
				unsigned int dnspvc1,dnspvc2,dnspvc3;
				if(!mib_get_s(MIB_DNS_BIND_PVC1,(void*)&dnspvc1, sizeof(dnspvc1)))
					{
					boaError(wp, 400, "Get MIB_DNS_BIND_PVC1 record error!\n");
							return -1;
					}
				if(!mib_get_s(MIB_DNS_BIND_PVC2,(void*)&dnspvc2, sizeof(dnspvc2)))
					{
					boaError(wp, 400, "Get MIB_DNS_BIND_PVC2 record error!\n");
							return -1;
					}
				if(!mib_get_s(MIB_DNS_BIND_PVC3,(void*)&dnspvc3, sizeof(dnspvc3)))
					{
					boaError(wp, 400, "Get MIB_DNS_BIND_PVC3 record error!\n");
							return -1;
					}

				    boaWrite(wp,"DnsBindSelectdInit('wanlist1',%d);\n",dnspvc1);
				    boaWrite(wp,"DnsBindSelectdInit('wanlist2',%d);\n",dnspvc2);
				    boaWrite(wp,"DnsBindSelectdInit('wanlist3',%d);\n",dnspvc3);
				boaWrite(wp,"DnsBindPvcClicked();");
				return 0;
			}
#endif

#ifdef CONFIG_IPV6
#ifdef DNSV6_BIND_PVC_SUPPORT
	else if(!strcmp(name,"dnsv6BindPvcInit"))
			{
				unsigned int dnspvc1,dnspvc2,dnspvc3;
				if(!mib_get_s(MIB_DNSV6_BIND_PVC1,(void*)&dnspvc1, sizeof(dnspvc1)))
					{
					boaError(wp, 400, "Get MIB_DNSV6_BIND_PVC1 record error!\n");
							return -1;
					}
				if(!mib_get_s(MIB_DNSV6_BIND_PVC2,(void*)&dnspvc2, sizeof(dnspvc2)))
					{
					boaError(wp, 400, "Get MIB_DNSV6_BIND_PVC2 record error!\n");
							return -1;
					}
				if(!mib_get_s(MIB_DNSV6_BIND_PVC3,(void*)&dnspvc3, sizeof(dnspvc3)))
					{
					boaError(wp, 400, "Get MIB_DNSV6_BIND_PVC3 record error!\n");
							return -1;
					}

				    boaWrite(wp,"DnsBindSelectdInit('v6wanlist1',%d);\n",dnspvc1);
				    boaWrite(wp,"DnsBindSelectdInit('v6wanlist2',%d);\n",dnspvc2);
				    boaWrite(wp,"DnsBindSelectdInit('v6wanlist3',%d);\n",dnspvc3);
				boaWrite(wp,"Dnsv6BindPvcClicked();");
				return 0;
			}
#endif
#endif
	else if(!strcmp(name, "dgw")){
#ifdef DEFAULT_GATEWAY_V1
		boaWrite(wp, "\tif (droute == 1)\n");
		boaWrite(wp, "\t\tdocument.adsl.droute[1].checked = true;\n");
		boaWrite(wp, "\telse\n");
		boaWrite(wp, "\t\tdocument.adsl.droute[0].checked = true;\n");
#else
		GetDefaultGateway(eid, wp, argc, argv);
		boaWrite(wp, "\tautoDGWclicked();\n");
#endif
	}
/* add by yq_zhou 09.2.02 add sagem logo for 11n*/
	else if(!strncmp(name, "title", 5))	{
#ifdef CONFIG_SFU
		boaWrite(wp,	"<img src=\"graphics/topbar_sfu.gif\" width=900 height=60 border=0>");
#elif CONFIG_VDSL
		boaWrite(wp,	"<img src=\"graphics/topbar_vdsl.gif\" width=900 height=60 border=0>");
#elif CONFIG_11N_SAGEM_WEB
		boaWrite(wp,	"<img src=\"graphics/sagemlogo1.gif\" width=1350 height=60 border=0>");
#else
		boaWrite(wp,	"<img src=\"graphics/topbar.gif\" width=900 height=60 border=0>");
#endif
	}
	else if(!strncmp(name, "logobelow", 9))	{
#ifdef CONFIG_11N_SAGEM_WEB
		boaWrite(wp,	"<img src=\"graphics/sagemlogo2.gif\" width=180 height=60 border=0>");
#endif
	}
#ifdef CONFIG_ETHWAN
	else if(!strncmp(name, "ethwanSelection", 15)){
		MIB_CE_ATM_VC_T Entry;

		memset((void *)&Entry, 0, sizeof(Entry));
		if (getWanEntrybyMedia(&Entry, MEDIA_ETH)>=0)
			boaWrite(wp, "document.ethwan.adslConnectionMode.value = \"%d\";\n", Entry.cmode);
	}
#endif
#ifdef CONFIG_INIT_SCRIPTS
	else if(!strncmp(name, "getStartScriptContent", 21))
	{
		FILE *sct_fp;
		char line[128];

		if(sct_fp = fopen("/var/config/start_script", "r"))
		{
			while(fgets(line, 127, sct_fp))
				boaWrite(wp, "%s<br>", line);
		}
		else
			boaWrite(wp, "Open script file failed!\n");
	}
	else if(!strncmp(name, "getEndScriptContent", 21))
	{
		FILE *sct_fp;
		char line[128];

		if(sct_fp = fopen("/var/config/end_script", "r"))
		{
			while(fgets(line, 127, sct_fp))
				boaWrite(wp, "%s<br>", line);
		}
		else
			boaWrite(wp, "Open script file failed!\n");
	}
#endif
#ifndef CONFIG_DEV_xDSL
	else if(!strcmp(name, "wan_mode_atm"))
	{
		boaWrite(wp, "style=\"display: none\"");
	}
#endif
#ifndef CONFIG_ETHWAN
	else if(!strcmp(name, "wan_mode_ethernet"))
	{
		boaWrite(wp, "style=\"display: none\"");
	}
#endif
#ifndef CONFIG_PTMWAN
	else if(!strcmp(name, "wan_mode_ptm"))
	{
		boaWrite(wp, "style=\"display: none\"");
	}
#endif
#if !defined(CONFIG_USER_XDSL_SLAVE) || !defined(CONFIG_PTMWAN)
	else if(!strcmp(name, "wan_mode_bonding"))
	{
		boaWrite(wp, "style=\"display: none\"");
	}
#endif
#ifndef WLAN_WISP
	else if(!strcmp(name, "wan_mode_wireless"))
	{
		boaWrite(wp, "style=\"display: none\"");
	}
#endif
	else if (!strcmp(name, "lan_port_status")) {
#ifndef CONFIG_00R0
		boaWrite(wp, "style=\"display: none\"");
#endif
		return 0;
	}
#ifdef CONFIG_00R0
	// 0 - Mosocow, 1-Sankt-Peterburg, 2-Chelyabinsk
	else if (!strcmp(name, "rosetelecom_Mosocow")) {
		if (!mib_get_s(MIB_ROSTELECOM_PROVINCES, (void *)&vChar, sizeof(vChar)))
			return -1;
		if (vChar==0)
			boaWrite(wp,"1");
		else
			boaWrite(wp,"0");
		return 0;
	}
	else if (!strcmp(name, "rosetelecom_Sankt-Peterburg")) {
		if (!mib_get_s(MIB_ROSTELECOM_PROVINCES, (void *)&vChar, sizeof(vChar)))
			return -1;
		if (vChar==1)
			boaWrite(wp,"1");
		else
			boaWrite(wp,"0");
		return 0;
	}
	else if (!strcmp(name, "rosetelecom_Chelyabinsk")) {
		if (!mib_get_s(MIB_ROSTELECOM_PROVINCES, (void *)&vChar, sizeof(vChar)))
			return -1;
		if (vChar==2)
			boaWrite(wp,"1");
		else
			boaWrite(wp,"0");
		return 0;
	}
	else if ( !strcmp(name, "ipport-ctrl-on-0") ) {
		if ( !mib_get_s(MIB_IPFILTER_ON_OFF, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "ipport-ctrl-on-1") ) {
		if ( !mib_get_s(MIB_IPFILTER_ON_OFF, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "spi-ctrl-on-0") ) {
		if ( !mib_get_s(MIB_IPFILTER_SPI_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (0 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
	else if ( !strcmp(name, "spi-ctrl-on-1") ) {
		if ( !mib_get_s(MIB_IPFILTER_SPI_ENABLE, (void *)&vChar, sizeof(vChar)) )
			return -1;
		if (1 == vChar)
			boaWrite(wp, "checked");
		return 0;
	}
#endif
#ifdef CONFIG_USER_LAN_VLAN_TRANSLATE
	else if(!strcmp(name, "vlan_mapping_interface")){
		int i,first=1;
		boaWrite(wp, "[");
		for(i=0; i<SW_LAN_PORT_NUM;i++)
		{
			if(first)
			{
				boaWrite(wp, "\"LAN%d\"",i+1);
				first=0;
			}
			else
				boaWrite(wp, ",\"LAN%d\"",i+1);
		}
#if 0
		int orig_wlan_idx = wlan_idx;
		int j;
		for(j=0; j<NUM_WLAN_INTERFACE; j++)
		{
			wlan_idx = j;
			boaWrite(wp, ",\"SSID%d\"", j*(WLAN_MBSSID_NUM+1) + 1);
#ifdef WLAN_MBSSID
			MIB_CE_MBSSIB_T entry;
			for (i = 0; i < WLAN_MBSSID_NUM; i++)
			{
				mib_chain_get(MIB_MBSSIB_TBL, i + 1, &entry);
				if (entry.wlanDisabled) {
					boaWrite(wp, ",\"SSID_DISABLE\"");
				}
				else {
					boaWrite(wp, ",\"SSID%d\"", j*(WLAN_MBSSID_NUM+1) + (i + 2));
				}
			}
#endif
			for (i = 0; i < (MAX_WLAN_VAP - WLAN_MBSSID_NUM); i++) {
				boaWrite(wp, ",\"SSID_DISABLE\"");
			}
		}
		boaWrite(wp,"]");
		wlan_idx = orig_wlan_idx;
#else
		boaWrite(wp,"]");
#endif

		return 0;
	}
#endif
#if defined(CONFIG_USER_INTERFACE_GROUPING)
	else if (!strcmp(name, "interface_grouping_tabel"))
	{
		int i = 0, total = 0, itfNum = 0, service_limit = 0;
		unsigned int show_domain = 0;
		MIB_CE_ATM_VC_T wanEntry;
		int wanIndex = 0;
		MIB_L2BRIDGE_GROUP_T l2br_group_entry = {0};
		struct layer2bridging_availableinterface_info itfList[MAX_NUM_OF_ITFS];

		show_domain = (INTFGRPING_DOMAIN_ELAN | INTFGRPING_DOMAIN_WLAN | INTFGRPING_DOMAIN_WANROUTERCONN);
#ifdef CONFIG_USER_LAN_VLAN_TRANSLATE
		show_domain |= (INTFGRPING_DOMAIN_ELAN_VLAN);
#endif
#if !defined(CONFIG_00R0)
		show_domain |= (INTFGRPING_DOMAIN_LANROUTERCONN);
#endif

#ifdef CONFIG_00R0
		char usName[MAX_NAME_LEN] = {0};
		mib_get_s(MIB_USER_NAME, (void *)usName, sizeof(usName));
		if (strcmp(g_login_username, usName) == 0)
		{
			service_limit = (X_CT_SRV_TR069 | X_CT_SRV_VOICE);
		}
#endif


		boaWrite(wp, "var InterfaceGroupingKey = new Array();\n");
		boaWrite(wp, "var InterfaceGroupingEnable = new Array();\n");
		boaWrite(wp, "var InterfaceGroupingName = new Array();\n");

		total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, (void *)&l2br_group_entry))
				continue;

			boaWrite(wp, "InterfaceGroupingKey[InterfaceGroupingKey.length] = \"%u\";\n", l2br_group_entry.groupnum);
			boaWrite(wp, "InterfaceGroupingEnable[InterfaceGroupingEnable.length] = \"%u\";\n", l2br_group_entry.enable);
			boaWrite(wp, "InterfaceGroupingName[InterfaceGroupingName.length] = \"%s\";\n", l2br_group_entry.name);
		}

		boaWrite(wp, "var InterfaceList_Domain = new Array();\n");
		boaWrite(wp, "var InterfaceList_ID = new Array();\n");
		boaWrite(wp, "var InterfaceList_Name = new Array();\n");
		boaWrite(wp, "var InterfaceList_Group = new Array();\n");
		boaWrite(wp, "var InterfaceList_Limit = new Array();\n");
		itfNum = rtk_layer2bridging_get_availableinterface(itfList, MAX_NUM_OF_ITFS, show_domain, (-1));
		for (i = 0; i < itfNum; i++)
		{
			if (itfList[i].ifdomain & show_domain)
			{
				boaWrite(wp, "InterfaceList_Domain[InterfaceList_Domain.length] = \"%u\";\n", itfList[i].ifdomain);
				boaWrite(wp, "InterfaceList_ID[InterfaceList_ID.length] = \"%u\";\n", itfList[i].ifid);
				boaWrite(wp, "InterfaceList_Name[InterfaceList_Name.length] = \"%s\";\n", itfList[i].name);
				boaWrite(wp, "InterfaceList_Group[InterfaceList_Group.length] = \"%u\";\n", itfList[i].itfGroup);

				if (service_limit > 0 &&
					(itfList[i].ifdomain & INTFGRPING_DOMAIN_WANROUTERCONN) &&
					getWanEntrybyIfIndex(itfList[i].ifid, &wanEntry, &wanIndex) == 0 &&
					(wanEntry.applicationtype & service_limit))
				{
					boaWrite(wp, "InterfaceList_Limit[InterfaceList_Limit.length] = \"%u\";\n", 1);
				}
				else
				{
					boaWrite(wp, "InterfaceList_Limit[InterfaceList_Limit.length] = \"%u\";\n", 0);
				}
			}
		}

#if defined(CONFIG_00R0) && defined(_CWMP_MIB_)
		// show automatic build bridge WAN on web page. Group number cannot be changed by user
		total = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&wanEntry))
				continue;

			if (wanEntry.connDisable == 1 && wanEntry.ConPPPInstNum == 0 && wanEntry.ConIPInstNum == 0)
			{
				char ifname[16] = {0};
				boaWrite(wp, "InterfaceList_Domain[InterfaceList_Domain.length] = \"%u\";\n", 0); // mark as an invalid domain
				boaWrite(wp, "InterfaceList_ID[InterfaceList_ID.length] = \"%u\";\n", wanEntry.ifIndex);
				ifGetName(wanEntry.ifIndex, ifname, sizeof(ifname));
				boaWrite(wp, "InterfaceList_Name[InterfaceList_Name.length] = \"%s\";\n", ifname);
				boaWrite(wp, "InterfaceList_Group[InterfaceList_Group.length] = \"%u\";\n", wanEntry.itfGroupNum);
				boaWrite(wp, "InterfaceList_Limit[InterfaceList_Limit.length] = \"%u\";\n", 1);
			}
		}
#endif
	}
#endif
	else if(!strcmp(name,"is_rtk_dev_ap"))
	{
#ifdef CONFIG_RTK_DEV_AP
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
	}
	else if(!strcmp(name,"is_rtk_dev_ap_single_wan"))
	{
#if defined(CONFIG_RTK_DEV_AP) && !defined(CONFIG_RTL_MULTI_ETH_WAN)
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
	}
	else if(!strcmp(name,"rtk_dev_ap_comment_start"))
	{
#ifdef CONFIG_RTK_DEV_AP
		boaWrite(wp, "<!--");
#else
		boaWrite(wp, "");
#endif
	}
	else if(!strcmp(name,"rtk_dev_ap_comment_end"))
	{
#ifdef CONFIG_RTK_DEV_AP
		boaWrite(wp, "-->");
#else
		boaWrite(wp, "");
#endif
	}
	else if(!strcmp(name,"rtk_dev_ap_single_wan_comment_start"))
	{
#if defined(CONFIG_RTK_DEV_AP) && !defined(CONFIG_RTL_MULTI_ETH_WAN)
		boaWrite(wp, "<!--");
#else
		boaWrite(wp, "");
#endif
	}
	else if(!strcmp(name,"rtk_dev_ap_single_wan_comment_end"))
	{
#if defined(CONFIG_RTK_DEV_AP) && !defined(CONFIG_RTL_MULTI_ETH_WAN)
		boaWrite(wp, "-->");
#else
		boaWrite(wp, "");
#endif
	}
	else if(!strcmp(name,"qos_limit_speed_unit_one_start"))
	{
#ifdef CONFIG_QOS_SPEED_LIMIT_UNIT_ONE
		boaWrite(wp, "");
#else
		boaWrite(wp, "<!--");
#endif
	}
	else if(!strcmp(name,"qos_limit_speed_unit_one_end"))
	{
#ifdef CONFIG_QOS_SPEED_LIMIT_UNIT_ONE
		boaWrite(wp, "");
#else
		boaWrite(wp, "-->");
#endif
	}
	else if(!strcmp(name,"no_qos_limit_speed_unit_one_start"))
	{
#ifndef CONFIG_QOS_SPEED_LIMIT_UNIT_ONE
		boaWrite(wp, "");
#else
		boaWrite(wp, "<!--");
#endif
	}
	else if(!strcmp(name,"no_qos_limit_speed_unit_one_end"))
	{
#ifndef CONFIG_QOS_SPEED_LIMIT_UNIT_ONE
		boaWrite(wp, "");
#else
		boaWrite(wp, "-->");
#endif
	}
	else if(!strcmp(name,"limit_speed_unit_one"))
	{
#ifdef CONFIG_QOS_SPEED_LIMIT_UNIT_ONE
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
#ifdef WLAN_RTIC_SUPPORT
	else if(!strcmp(name,"RTIC_Mode_Select")){
		unsigned char mode =0;
		mib_get_s(MIB_RTIC_MODE, (void *)&mode, sizeof(mode));
		boaWrite(wp,"<table>");
		if((mode == 0) && (wlan_idx == 1))
			boaWrite(wp, "<tr><td colspan=2><b>%s", multilang(LANG_RTIC_IS_ZERO_WAIT_DFS_MODE), "</b><td></tr>");
		boaWrite(wp, "<tr><th>RTIC Mode Select:  </th>");
		if((mode == 0) && (wlan_idx == 1))
			boaWrite(wp, "<td><input type=\"radio\" name=\"rticmode\" value=0 id=\"rticmode\" onClick=\"ZWDFSModeChange()\" style=\"display:none\">");
		else
			boaWrite(wp, "<td><input type=\"radio\" name=\"rticmode\" value=0 id=\"rticmode\" onClick=\"ZWDFSModeChange()\">ZWDFS&nbsp;&nbsp;");
		boaWrite(wp, "<input type=\"radio\" name=\"rticmode\" value=1 id=\"rticmode\" onClick=\"DCSModeChange()\">DCS");
		boaWrite(wp, "</td></tr></table>");
		return 0;
	}
	else if(!strcmp(name,"RTIC_channel_number")){
		MIB_CE_MBSSIB_T WlanEnable_Entry;
		unsigned char ifrname[32]={0};
		int tmp_vwlan_idx=0;
		struct iwreq wrq;
		int ret;
		int skfd,cmd_id;
		unsigned char tmp_buffer[256 + 1];
#ifndef SIOCDEVPRIVATEAXEXT
		#define SIOCDEVPRIVATEAXEXT 0x89f2 //SIOCDEVPRIVATE+2
#endif

		memset(&WlanEnable_Entry,0,sizeof(MIB_CE_MBSSIB_T));
		memset(ifrname,0,sizeof(ifrname));
		snprintf(ifrname,sizeof(ifrname),WLAN_IF,wlan_idx);
		if(mib_chain_get_wlan(ifrname,&WlanEnable_Entry,&tmp_vwlan_idx)!=0)
			return -1;

		skfd= socket(AF_INET, SOCK_DGRAM, 0);
		strcpy(wrq.ifr_name, ifrname);
		strcpy(tmp_buffer,"channel");
		wrq.u.data.pointer = (caddr_t)&tmp_buffer;
		wrq.u.data.length = 10;

		if(WlanEnable_Entry.is_ax_support)
		{
			wrq.u.data.flags = RTL8192CD_IOCTL_GET_MIB;
			cmd_id = SIOCDEVPRIVATEAXEXT;
		}
		else
			cmd_id = RTL8192CD_IOCTL_GET_MIB;

		ret = ioctl(skfd, cmd_id, &wrq);
		if ( ret != -1)
			boaWrite (wp, "%d", tmp_buffer[wrq.u.data.length-1]);
		else
			boaWrite (wp, "0");
		close( skfd );

		return 0;
	}
	else if (!strcmp(name, "rtic_info_addr_string")) {
		int ret;
		unsigned char mode =0;
		FILE *fp1 = NULL;
		char  filename_string[100];

		memset(filename_string, 0, sizeof(filename_string));
		mib_get_s(MIB_RTIC_MODE, (void *)&mode, sizeof(mode));

		if(mode){
			if(wlan_idx == 1){
				snprintf(filename_string,sizeof(filename_string),"%s", "/proc/net/rtic/wlan2/dcs_2g_xml");
				printf("dcs_2g_xml\n");
			}
			if(wlan_idx == 0){
				snprintf(filename_string,sizeof(filename_string),"%s", "/proc/net/rtic/wlan2/dcs_5g_xml");
				printf("dcs_5g_xml\n");
			}
		}
		else{
			if(wlan_idx == 1){
				snprintf(filename_string,sizeof(filename_string),"%s", "/proc/net/rtic/wlan2/zwdfs_2g_xml");
				printf("zwdfs_2g_xml\n");
			}
			if(wlan_idx == 0){
				snprintf(filename_string,sizeof(filename_string),"%s", "/proc/net/rtic/wlan2/zwdfs_5g_xml");
				printf("zwdfs_5g_xml\n");
			}
		}

		fp1 = fopen(filename_string,"r");
		if(fp1 == NULL){
			printf("Error1: %s - continuing...\n",filename_string);
			return -1;
		}

		size_t	len   = 0;
		char*	line  = NULL;
		char string[2048] = {0};
		getline(&line, &len, fp1);
		fclose(fp1);
		if(line == NULL){
			printf("Error2: xml path - continuing...\n");
			return -1;
		}
		FILE *fp2 = fopen(line,"r");
		free(line);
		if(fp2 == NULL){
			printf("Error3: %s - continuing...\n",line);
			return -1;
		}
		while(fgets(string, 2048 , fp2)){
			boaWrite(wp, "%s", string);
			memset(string, 0, sizeof(string));
		}
		boaWrite(wp, "");
		fclose(fp2);
		return 0;
	}
	else if(!strcmp(name,"Change_RTIC_Mode"))
	{
		unsigned char vMode;
		if (!mib_get_s(MIB_RTIC_MODE, (void *)&vMode, sizeof(vMode))){
			printf("get rtic mode error!");
			return -1;
		}
		if (vMode==1)
			boaWrite(wp,"1");
		else
			boaWrite(wp,"0");
		return 0;
	}
	else if(!strcmp(name,"RTIC_Mode_Title"))
	{
		unsigned char vMode;
		if (!mib_get_s(MIB_RTIC_MODE, (void *)&vMode, sizeof(vMode))){
			printf("get rtic mode error!");
			return -1;
		}
		if (vMode==1)
			boaWrite(wp,"<p>%s</p>",multilang(LANG_DCS));
		else
			boaWrite(wp,"<p>%s</p>",multilang(LANG_ZWDFS));
		return 0;
	}
	else if(!strcmp(name,"RTIC_Suggest_Chan"))
	{
		FILE *fp = NULL;
		size_t	len   = 0;
		char*	line  = NULL;
		char  	filename_suggest_chan[32];

		snprintf(filename_suggest_chan,sizeof(filename_suggest_chan),"/var/run/dcs_wlan%d_suggest_ch",wlan_idx);
		fp = fopen(filename_suggest_chan,"r");
		if(fp == NULL){
			printf("Error: Open filename_suggest_chan failed  - continuing...\n");
			return -1;
		}

		getline(&line, &len, fp);
		fclose(fp);
		if(line == NULL)
			return -1;
		boaWrite(wp, line);
		free(line);
		return 0;
	}
#endif
	else if(!strcmp(name,"RTIC_SUPPORT"))
	{
#ifdef WLAN_RTIC_SUPPORT
				boaWrite(wp, "1");
#else
				boaWrite(wp, "0");
#endif
			return 0;
    }
	else if(!strcmp(name,"RTIC_SUPPORT_START"))
	{
#ifdef WLAN_RTIC_SUPPORT
				boaWrite(wp, "");
#else
				boaWrite(wp, "<!--");
#endif
		return 0;
	}
	else if(!strcmp(name,"RTIC_SUPPORT_END"))
	{
#ifdef WLAN_RTIC_SUPPORT
				boaWrite(wp, "");
#else
				boaWrite(wp, "-->");
#endif
		return 0;
	}
	else if(!strcmp(name, "password_encrypt_flag"))
	{
		int encrypt_flag=0;
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
	}
	else if(!strcmp(name, "realm"))
	{
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
		char realm[MAX_REALM_LEN]={0};
		mib_get_s( MIB_WEB_USER_REALM , (void *)realm, sizeof(realm));
		boaWrite(wp, "\"%s\"",realm );
#else
		boaWrite(wp, "\"\"");
#endif
	}
	else if(!strcmp(name,"wan_sel_8023ah")){
#ifdef CONFIG_USER_8023AH_WAN_INTF_SELECTION
#if defined(CONFIG_PTMWAN) || (defined(CONFIG_ETHWAN) &&  defined(CONFIG_RTL_MULTI_ETH_WAN))
		char wanname[IFNAMSIZ];
#endif
	boaWrite(wp, "<tr>");
		boaWrite(wp, "<th>%s:</th>",multilang(LANG_WAN_INTERFACE));
		boaWrite(wp, "	<td>");
		boaWrite(wp, "		<select name=\"efm_8023ah_ext_if\">");
#ifdef CONFIG_PTMWAN
		snprintf( wanname, sizeof(wanname),  "%s%u", ALIASNAME_PTM, 0 );
		boaWrite(wp, "			<option value=\"%s\">%s</option>\n", wanname, wanname);
#endif

#if defined(CONFIG_ETHWAN) &&  defined(CONFIG_RTL_MULTI_ETH_WAN)
		snprintf( wanname, sizeof(wanname),  "%s%u", ALIASNAME_NAS, 0 );
		boaWrite(wp, "			<option value=\"%s\">%s</option>\n", wanname, wanname);
#endif
		boaWrite(wp, "		</select>");
		boaWrite(wp, "	</td>");
		boaWrite(wp, "</tr>");
#endif
		return 0;
	}
	else if(!strcmp(name, "username"))
	{
		boaWrite(wp, "\"%s\"",usName );
	}
	else if(!strcmp(name, "susername"))
	{
		boaWrite(wp, "\"%s\"",suName );
	}
	else if(!strcmp(name,"wan_logic_port"))
	{
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
#ifdef CONFIG_USER_WAN_PORT_AUTO_SELECTION
	mib_get_s(MIB_WAN_PORT_AUTO_SELECTION_ENABLE,&vChar,sizeof(vChar));
		if(vChar==1){
			boaWrite(wp,"0");
			return 0;
		}
#endif
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name,"phy_rate"))
	{
#if	defined(CONFIG_USER_MULTI_PHY_ETH_WAN_RATE) && defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
		boaWrite(wp,"1");
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name,"wan_port_mask"))
	{
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
		int wan_port_mask=0;
		for(vUInt=0; vUInt<SW_PORT_NUM; vUInt++)
		{
			if (rtk_port_is_wan_logic_port(vUInt))
				wan_port_mask += 1 << vUInt;
		}
		boaWrite(wp,"%d", wan_port_mask);
#else
		boaWrite(wp,"0");
#endif
		return 0;
	}
	else if(!strcmp(name,"sw_lan_port_num"))
	{
		boaWrite(wp,"%d", SW_LAN_PORT_NUM);
		return 0;
	}
	else if(!strcmp(name,"qos_multi_phy_wan_set_start"))
	{
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		boaWrite(wp, "");
#else
		boaWrite(wp, "<!--");
#endif
	}
	else if(!strcmp(name,"qos_multi_phy_wan_set_end"))
	{
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		boaWrite(wp, "");
#else
		boaWrite(wp, "-->");
#endif
	}
#ifdef CONFIG_USER_WAN_PORT_AUTO_SELECTION
	else if(!strcmp(name,"wan_port_set"))
	{
		mib_get_s(MIB_WAN_PORT_AUTO_SELECTION_ENABLE,&vChar,sizeof(vChar));
		if(vChar==1)
			boaWrite(wp,"%d",1);
		else
			boaWrite(wp,"%d",0);
	}
#endif
	else if ( !strcmp(name, "support_voip") ) {
#ifdef VOIP_SUPPORT
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
	}
	else if ( !strcmp(name, "L2TPOverIPSec") ) {
		/* only xl2tpd support IPSec now */
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
		boaWrite(wp, "1");
#else
		boaWrite(wp, "0");
#endif
	}
	else
		return -1;

	return 0;
}

#ifdef WLAN_MBSSID
int getVirtualIndex(int eid, request *wp, int argc, char **argv)
{
	int ret=0, vwlan_idx;
	char s_ifname[16];
	MIB_CE_MBSSIB_T Entry;
	int i=0;

	snprintf(s_ifname, 16, "%s", (char *)getWlanIfName());
	vwlan_idx = atoi(argv[--argc]);

	if (vwlan_idx > NUM_VWLAN_INTERFACE) {
		//fprintf(stderr, "###%s:%d wlan_idx=%d vwlan_idx=%d###\n", __FILE__, __LINE__, wlan_idx, vwlan_idx);
		boaWrite(wp, "0");
		return -1;
	}

	if (vwlan_idx > 0)
		snprintf(s_ifname, 16, "%s-vap%d", (char *)getWlanIfName(), vwlan_idx - 1);

	if (!mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry)) {
		printf("Error! Get MIB_MBSSIB_TBL(getVirtualIndex) error.\n");
		return -1;
	}

	if (!strcmp("band", argv[0]))
#ifdef WLAN_BAND_CONFIG_MBSSID
		boaWrite(wp, "%d", Entry.wlanBand);
#else
	{
		unsigned char wlband=0;
		mib_get_s(MIB_WLAN_BAND, (void *)&wlband, sizeof(wlband));
		boaWrite(wp, "%d", wlband);
	}
#endif
	else if (!strcmp("wlanDisabled", argv[0]))
		boaWrite(wp, "%d", Entry.wlanDisabled);
	else if (!strcmp("wlanAccess", argv[0]))
		boaWrite(wp, "%d", Entry.userisolation);
	else if (!strcmp("rateAdaptiveEnabled", argv[0]))
		boaWrite(wp, "%d", Entry.rateAdaptiveEnabled);
	else if (!strcmp("fixTxRate", argv[0]))
		boaWrite(wp, "%u", Entry.fixedTxRate);
	else if (!strcmp("wmmEnabled", argv[0]))
		boaWrite(wp, "%u", Entry.wmmEnabled);
	else if (!strcmp("hidessid", argv[0]))
		boaWrite(wp, "%d", Entry.hidessid);
	else if (!strcmp("ssid_pri", argv[0]))
#ifdef CONFIG_USER_AP_CMCC
		boaWrite(wp, "%d", Entry.manual_priority);
#else
		boaWrite(wp, "0");
#endif
	else if (!strcmp("multiap_bss_type", argv[0]))
#ifdef RTK_MULTI_AP
		boaWrite(wp, "%d", Entry.multiap_bss_type);
#else
		boaWrite(wp, "0");
#endif
	else if (!strcmp("multiap_vid", argv[0]))
#ifdef RTK_MULTI_AP_R2
		boaWrite(wp, "%d", Entry.multiap_vid);
#else
		boaWrite(wp, "0");
#endif
	else if (!strcmp("encrypt", argv[0]))
		boaWrite(wp, "%d", Entry.encrypt);
	return ret;
}
#endif

int write_wladvanced(int eid, request * wp, int argc, char **argv)        //add by yq_zhou 1.20
{
#ifdef WPS20
	unsigned char wpsUseVersion;
	mib_get_s(MIB_WSC_VERSION, (void *) &wpsUseVersion, sizeof(wpsUseVersion));
#endif

#ifdef WLAN_SUPPORT
	char vChar;
#ifdef WLAN_QTN
#ifdef WLAN1_QTN
	if(wlan_idx==1)
#elif defined(WLAN0_QTN)
	if(wlan_idx==0)
#endif
		boaWrite(wp,"<tr style=\"display: none\">");
	else
#endif
	boaWrite(wp,"<tr>");
    boaWrite(wp, "<th>%s:</th>\n"
     "<td>\n"
     "<input type=\"radio\" value=\"long\" name=\"preamble\">%s&nbsp;&nbsp;\n"
     "<input type=\"radio\" name=\"preamble\" value=\"short\">%s</td></tr>\n",
		multilang(LANG_PREAMBLE_TYPE), multilang(LANG_LONG_PREAMBLE),
		multilang(LANG_SHORT_PREAMBLE));

#ifdef WPS20
	if (wpsUseVersion) {
			boaWrite(wp,
			 "<tr><th>%s %s:</th>\n"
			 "<td>\n"
			 "<input type=\"radio\" name=\"hiddenSSID\" value=\"no\">%s&nbsp;&nbsp;\n"
			 "<input type=\"radio\" name=\"hiddenSSID\" value=\"yes\" onClick=\"alert('If you disable this, WPS will be disabled!')\">%s</td></tr>\n",
			multilang(LANG_BROADCAST), multilang(LANG_SSID), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
	}
	else
#endif
	{
		boaWrite(wp,
	     "<tr><th>%s %s:</th>\n"
	     "<td>\n"
	     "<input type=\"radio\" name=\"hiddenSSID\" value=\"no\">%s&nbsp;&nbsp;\n"
	     "<input type=\"radio\" name=\"hiddenSSID\" value=\"yes\">%s</td></tr>\n",
		multilang(LANG_BROADCAST), multilang(LANG_SSID), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
	}
	boaWrite(wp,
     "<tr><th>%s:</th>\n"
     "<td>\n"
     "<input type=\"radio\" name=block value=1>%s&nbsp;&nbsp;\n"
     "<input type=\"radio\" name=block value=0>%s</td></tr>\n",
	multilang(LANG_RELAY_BLOCKING), multilang(LANG_ENABLED), multilang(LANG_DISABLED));

#ifndef WLAN_11AX
#ifdef WLAN_QTN
#ifdef WLAN1_QTN
	if(wlan_idx==1)
#elif defined(WLAN0_QTN)
	if(wlan_idx==0)
#endif
		boaWrite(wp,"<tr style=\"display: none\">");
	else
		boaWrite(wp,"<tr style=\"display: ""\">");
#endif
	boaWrite(wp,"<tr>");
	boaWrite(wp,"<th>%s:</th>\n"
     "<td>\n"
     "<input type=\"radio\" name=\"protection\" value=\"yes\">%s&nbsp;&nbsp;\n"
     "<input type=\"radio\" name=\"protection\" value=\"no\">%s</td></tr>\n",
	multilang(LANG_PROTECTION), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
#endif
#ifdef WLAN_QTN
#ifdef WLAN1_QTN
	if(wlan_idx==1)
#elif defined(WLAN0_QTN)
	if(wlan_idx==0)
#endif
		boaWrite(wp,"<tr style=\"display: none\">");
	else
#endif
	boaWrite(wp,"<tr>");
  	boaWrite(wp,"<th>%s:</th>\n"
     "<td>\n"
     "<input type=\"radio\" name=\"aggregation\" value=\"enable\">%s&nbsp;&nbsp;\n"
     "<input type=\"radio\" name=\"aggregation\" value=\"disable\">%s</td></tr>\n",
	multilang(LANG_AGGREGATION), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
       boaWrite(wp,
     "<tr id=\"ShortGi\" style=\"display:\">\n"
     "<th>%s:</th>\n"
     "<td>\n"
     "<input type=\"radio\" name=\"shortGI0\" value=\"on\">%s&nbsp;&nbsp;\n"
     "<input type=\"radio\" name=\"shortGI0\" value=\"off\">%s</td></tr>\n",
	multilang(LANG_SHORT_GI), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
#ifdef RTK_SMART_ROAMING
	boaWrite(wp,
	"<tr id=\"SmartRoaming\" style=\"display:\">\n"
#ifndef CONFIG_GENERAL_WEB
   "<td width=\"30%%\"><font size=2><b>%s:</b></td>\n"
   "<td width=\"70%%\"><font size=2>\n"
#else
   "<th width=\"30%%\">%s:</th>\n"
   "<td width=\"70%%\">\n"
#endif
	"<input type=\"radio\" name=\"smart_roaming_enable\" value=\"on\" onClick='wlRoamingChange()'>%s&nbsp;&nbsp;\n"
	"<input type=\"radio\" name=\"smart_roaming_enable\" value=\"off\" onClick='wlRoamingChange()'>%s</td></tr>\n",
   multilang(LANG_SMART_ROAMING), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
	boaWrite(wp,
	"<tr id=\"SR_AutoConfig\" style=\"display:none\">\n"
#ifndef CONFIG_GENERAL_WEB
   "<td width=\"30%%\"><font size=2><b>%s %s:</b></td>\n"
   "<td width=\"70%%\"><font size=2>\n"
#else
   "<th width=\"30%%\">%s %s:</th>\n"
   "<td width=\"70%%\">\n"
#endif
	"<input type=\"radio\" name=\"SR_AutoConfig_enable\" value=\"on\">%s&nbsp;&nbsp;\n"
	"<input type=\"radio\" name=\"SR_AutoConfig_enable\" value=\"off\">%s</td></tr>\n",
   multilang(LANG_SMART_ROAMING), multilang(LANG_AUTO_CONFIG), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
#endif
#ifdef WLAN_TX_BEAMFORMING
#ifdef WIFI5_WIFI6_COMP
	MIB_CE_MBSSIB_T Entry;
	int show_txbf=0;

	if(!wlan_getEntry(&Entry, 0))
		return -1;
#if IS_AX_SUPPORT
	if(Entry.is_ax_support)
		show_txbf=1;
#endif
#ifdef WLAN_TX_BEAMFORMING_ENABLE
	show_txbf=1;
#endif
	if(show_txbf)
#endif
	{
		boaWrite(wp,"<tr id=\"txbf_div\">");
		boaWrite(wp,"<th>%s:</th>\n"
		"<td>\n"
		"<input type=\"radio\" name=\"txbf\" value=1 onClick='wltxbfChange()'>%s&nbsp;&nbsp;\n"
		"<input type=\"radio\" name=\"txbf\" value=0 onClick='wltxbfChange()'>%s</td></tr>\n",
		multilang(LANG_TXBF), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
		boaWrite(wp,"<tr id=\"txbf_mu_div\"  style=\"display:none\">");
#ifndef CONFIG_GENERAL_WEB
		boaWrite(wp,"<td width=\"30%%\"><font size=2><b>%s:</b></td>\n"
		"<td width=\"70%%\"><font size=2>\n"
#else
		boaWrite(wp,"<th width=\"30%%\">%s:</th>\n"
		"<td width=\"70%%\">\n"
#endif
		"<input type=\"radio\" name=\"txbf_mu\" value=1>%s&nbsp;&nbsp;\n"
		"<input type=\"radio\" name=\"txbf_mu\" value=0>%s</td></tr>\n",
		multilang(LANG_TXBF_MU), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
	}
#endif
	boaWrite(wp,"<tr>");
	boaWrite(wp,"<th>%s:</th>\n"
	"<td>\n"
	"<input type=\"radio\" name=\"mc2u_disable\" value=0>%s&nbsp;&nbsp;\n"
	"<input type=\"radio\" name=\"mc2u_disable\" value=1>%s</td></tr>\n",
	multilang(LANG_MC2U), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
#if defined(WLAN_BAND_STEERING) || defined(BAND_STEERING_SUPPORT)
	boaWrite(wp,"<tr><th>%s:</th>\n"
	"<td>\n"
	"<input type=\"radio\" name=\"sta_control\" value=1 onChange='stactrlPreBandChange()'>%s&nbsp;&nbsp;\n"
	"<input type=\"radio\" name=\"sta_control\" value=0 onChange='stactrlPreBandChange()'>%s\n"
	"<select size=\"1\" name=\"stactrl_prefer_band\">\n"
	"<option value=\"0\">Prefer 5GHz</option>\n"
	"<option value=\"1\">Prefer 2.4GHz</option>\n"
	"</select></td></tr>\n",
	multilang(LANG_BAND_STEERING), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
#endif
#ifdef WLAN_11R
	boaWrite(wp,"<tr><th>%s:</th>\n"
	"<td>\n"
	"<input type=\"radio\" name=\"dot11r_enable\" value=1>%s&nbsp;&nbsp;\n"
	"<input type=\"radio\" name=\"dot11r_enable\" value=0>%s</td></tr>\n",
	multilang(LANG_FAST_BSS_TRANSITION_802_11R), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
#endif
#ifdef RTK_CROSSBAND_REPEATER
	if(wlan_idx==0){
		boaWrite(wp,
		"<tr id=\"CrossBand\" style=\"display:\">\n"
#ifndef CONFIG_GENERAL_WEB
	   "<td width=\"30%%\"><font size=2><b>%s:</b></td>\n"
	   "<td width=\"70%%\"><font size=2>\n"
#else
	   "<th width=\"30%%\">%s:</th>\n"
	   "<td width=\"70%%\">\n"
#endif
		"<input type=\"radio\" name=\"cross_band_enable\" value=1>%s&nbsp;&nbsp;\n"
		"<input type=\"radio\" name=\"cross_band_enable\" value=0>%s</td></tr>\n",
	   multilang(LANG_CROSSBAND_SUPPORT), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
	}
#endif
#ifdef WLAN_11AX
	mib_get_s( MIB_WLAN_BAND, (void *)&vChar, sizeof(vChar));
	if(vChar & BAND_5G_11AX){
		boaWrite(wp,
	     "<tr><th>%s:</th>\n"
	     "<td>\n"
	     "<input type=\"radio\" name=ofdmaEnable value=1>%s&nbsp;&nbsp;\n"
	     "<input type=\"radio\" name=ofdmaEnable value=0>%s</td></tr>\n",
		multilang(LANG_OFDMA), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
	}
#endif
#endif /*WLAN_SUPPORT*/

	return 0;
}

int getNameServer(int eid, request * wp, int argc, char **argv) {

#if 0
	FILE *fp;
	char buffer[128], tmpbuf[64];
	int count = 0;
#endif
	int dns_fixed = 0;
	unsigned char tmpdns1[4], tmpdns2[4];
	char dns1[16], dns2[16];
	struct in_addr dns_ip1, dns_ip2;

#if 0
	//fprintf(stderr, "getNameServer %x\n", gResolvFile);
	//boaWrite(wp, "[]", tmpbuf);
	//if ((gResolvFile == NULL) ||
	// for IPv4
	if ( (fp = fopen(RESOLV_BACKUP, "r")) == NULL ) {
		//printf("Unable to open resolver file\n");
		return -1;
	}

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		if (sscanf(buffer, "%s", tmpbuf) != 1) {
			continue;
		}

		if (count == 0)
			boaWrite(wp, "%s", tmpbuf);
		else
			boaWrite(wp, ", %s", tmpbuf);
		count ++;
	}
	fclose(fp);
#endif

	/* fixed
	 * ATM_VC_TBL DNSV4IPAddr1, DNSV4IPAddr2
	 *
	 * dynamic
	 * DHCPS_DNS1 / DHCPS_DNS2
	 */
	deo_dns_fixed_value_get(&dns_fixed, tmpdns1, tmpdns2);
	if (dns_fixed == 0)
	{
		sprintf(dns1, "%d.%d.%d.%d", tmpdns1[0], tmpdns1[1], tmpdns1[2], tmpdns1[3]);
		boaWrite(wp, "%s", dns1);

		sprintf(dns2, "%d.%d.%d.%d", tmpdns2[0], tmpdns2[1], tmpdns2[2], tmpdns2[3]);
		boaWrite(wp, ", %s", dns2);
	}
	else
	{
		mib_get_s(MIB_DHCPS_DNS1, (void *)&dns_ip1, sizeof(dns_ip1));
		boaWrite(wp, "%s", inet_ntoa(dns_ip1));

		mib_get_s(MIB_DHCPS_DNS2, (void *)&dns_ip2, sizeof(dns_ip2));
		boaWrite(wp, ", %s", inet_ntoa(dns_ip2));
	}

	return 0;
}

#ifndef RTF_UP
/* Keep this in sync with /usr/src/linux/include/linux/route.h */
#define RTF_UP          0x0001	/* route usable                 */
#define RTF_GATEWAY     0x0002	/* destination is a gateway     */
#define RTF_HOST        0x0004	/* host entry (net otherwise)   */
#define RTF_REINSTATE   0x0008	/* reinstate route after tmout  */
#define RTF_DYNAMIC     0x0010	/* created dyn. (by redirect)   */
#define RTF_MODIFIED    0x0020	/* modified dyn. (by redirect)  */
#define RTF_MTU         0x0040	/* specific MTU for this route  */
#ifndef RTF_MSS
#define RTF_MSS         RTF_MTU	/* Compatibility :-(            */
#endif
#define RTF_WINDOW      0x0080	/* per route window clamping    */
#define RTF_IRTT        0x0100	/* Initial round trip time      */
#define RTF_REJECT      0x0200	/* Reject route                 */
#endif

// Jenny, get default gateway information
int getDefaultGW(int eid, request * wp, int argc, char **argv)
{
	char buff[256];
	int flgs;
	struct in_addr gw, dest, mask, inAddr;
	char ifname[16], dgw[16];
	int found=0;
	FILE *fp;

	if (!(fp=fopen("/proc/net/route", "r"))) {
		printf("Error: cannot open /proc/net/route - continuing...\n");
		return -1;
	}

	fgets(buff, sizeof(buff), fp);
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		//if (sscanf(buff, "%*s%*lx%lx%X%", &g, &flgs) != 2) {
		if (sscanf(buff, "%s%x%x%x%*d%*d%*d%x", ifname, &dest, &gw, &flgs, &mask) != 5) {
			printf("Unsuported kernel route format\n");
			fclose(fp);
			return -1;
		}

		//printf("ifname=%s, dest=%x, gw=%x, flgs=%x, mask=%x\n", ifname, dest.s_addr, gw.s_addr, flgs, mask.s_addr);
		if(flgs & RTF_UP  && strcmp(ifname, "lo")!=0) {
			// default gateway
			if (getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 1) {
				if (inAddr.s_addr == 0x40404040) {
					boaWrite(wp, "");
					continue;
				}
			}

			if (dest.s_addr == 0 && mask.s_addr == 0) {
				if (found)
					boaWrite(wp, ", ");
				if (gw.s_addr != 0) {
					dgw[sizeof(dgw)-1]='\0';
					strncpy(dgw,  inet_ntoa(gw), sizeof(dgw)-1);
					boaWrite(wp, "%s", dgw);
				}
				else {
					boaWrite(wp, "%s", ifname);
				}
				found = 1;
			}
		}
	}
	fclose(fp);
	return 0;
}

#ifdef CONFIG_IPV6
#define RTF_EXPIRES	0x00400000
int getDefaultGW_ipv6(int eid, request * wp, int argc, char **argv)
{
	int flags;
	unsigned char devname[IFNAMSIZ];
	unsigned char default_gw[INET6_ADDRSTRLEN] = {0};

	/* replace "cat /proc/net/ipv6_route" to "ip -6 route". 20210225 */
	// get default gw device name
	get_ipv6_default_gw_inf_first(devname);

	if (getInFlags( devname, &flags) != 1)
		return -1 ;
	// if point-to-point, we display devname.
	if (flags & IFF_POINTOPOINT) {
		if (devname[0]  != '\0')
			boaWrite(wp, "%s", devname);
		else
			boaWrite(wp, "");
		}
	// otherwise, display ip6addr.
	else {
		get_ipv6_default_gw(default_gw);
		boaWrite(wp, "%s", default_gw);
	}
	return 0;
}
#endif

int multilang_asp(int eid, request * wp, int argc, char **argv)
{
	int key;

   	if (boaArgs(argc, argv, "%d", &key) < 1) {
   		boaError(wp, 400, "Insufficient args\n");
   		return -1;
   	}

	return boaWrite(wp, "%s", multilang(key));
}

int WANConditions(int eid, request * wp, int argc, char **argv)
{
	int wan_bitmap = WAN_MODE_MASK;
	int i, count = 0;

	for(i = 0; i < 5; i++)
	{
		if(1 & (wan_bitmap >> i))
			count++;
	}

	if(count > 1)
		return 0;
	else
		return boaWrite(wp, "style=\"display:none\"");
}
int ShowWanMode(int eid, request * wp, int argc, char **argv)
{
	char *name;
#ifdef WLAN_WISP
	char mode;
	char rptEnabled;
	int orig_idx, i;
	int enable_wisp=0;
	MIB_CE_MBSSIB_T Entry;
#endif

	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return (-1);
	}

	if(!strcmp(name, "wlan"))
#ifdef WLAN_WISP
	{
		orig_idx = wlan_idx;
		for(i=0;i<NUM_WLAN_INTERFACE;i++){
			wlan_idx = i;
			wlan_getEntry(&Entry, 0);
			mode = Entry.wlanMode;
			mib_get_s(MIB_REPEATER_ENABLED1, &rptEnabled, sizeof(rptEnabled));

			if((mode == AP_MODE || mode == AP_WDS_MODE) && rptEnabled){
				enable_wisp=1;
				break;
			}
		}

		wlan_idx = orig_idx;

		if(enable_wisp)
			boaWrite(wp, "");
		else
			boaWrite(wp, "disabled");
	}
#else
	boaWrite(wp, "disabled");
#endif

	return 0;
}



/*
 *      Include file of utility.c
 *
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *      Authors: Dick Tam	<dicktam@realtek.com.tw>
 *
 *
 */

#ifndef INCLUDE_UTILITY_H
#define INCLUDE_UTILITY_H
#ifdef CONFIG_LUNA
#include <rtk/chip_deps.h>
#endif
#ifndef CONFIG_RTL_ALIASNAME
#define ALIASNAME_VC   "vc"
#define ALIASNAME_BR   "br"
#define ALIASNAME_NAS  "nas"
#define ALIASNAME_DSL  "dsl"
#define ALIASNAME_ETH  "eth"
#define ALIASNAME_WLAN "wlan"
#define ALIASNAME_PPP  "ppp"
#define ALIASNAME_MWNAS  "nas0_"
#define ALIASNAME_ELAN_PREFIX  "eth0."
#define ALIASNAME_VXD   "-vxd"
#ifdef CONFIG_USER_CMD_SERVER_SIDE
#define ALIASNAME_PTM  "nas"
#else
#define ALIASNAME_PTM  "ptm"
#endif
#define ALIASNAME_MWPTM  "ptm0_"
#define ORIGINATE_NUM 2

#else
#define ALIASNAME_VC   CONFIG_ALIASNAME_VC//"vc"
#define ALIASNAME_BR   CONFIG_ALIASNAME_BR//"br"
#define ALIASNAME_NAS  CONFIG_ALIASNAME_NAS//"nas"
#define ALIASNAME_DSL  CONFIG_ALIASNAME_DSL//"dsl"
#define ALIASNAME_ETH  CONFIG_ALIASNAME_ETH//"eth"
#define ALIASNAME_WLAN  CONFIG_ALIASNAME_WLAN//"wlan"
#define ALIASNAME_PPP  CONFIG_ALIASNAME_PPP//"ppp"
#define ALIASNAME_MWNAS  CONFIG_ALIASNAME_MWNAS//"nas0_"
#define ALIASNAME_ELAN_PREFIX  CONFIG_ALIASNAME_ELAN_PREFIX//"eth0."
#define ALIASNAME_PTM  CONFIG_ALIASNAME_PTM//"ptm"
#define ALIASNAME_MWPTM  CONFIG_ALIASNAME_MWPTM//"ptm0_"
#define ORIGINATE_NUM CONFIG_ORIGINATE_NUM

#endif

#define IGMP_RESTART_PROC_FILE "/proc/driver/realtek/deonet_igmp_restart"

#define INTERFACE_UPTIME_FILE "/var/interface/uptime/"
#define RTL_DEV_NAME_NUM(name,num)	name#num

#define ALIASNAME_NAS0  RTL_DEV_NAME_NUM(ALIASNAME_NAS,0)//"nas0"
#define ALIASNAME_PTM0  RTL_DEV_NAME_NUM(ALIASNAME_PTM,0)//"ptm0"

#define ALIASNAME_DSL0  RTL_DEV_NAME_NUM(ALIASNAME_DSL,0)//dsl0
#define ALIASNAME_ELAN0  RTL_DEV_NAME_NUM(ALIASNAME_ELAN_PREFIX,2)
#define ALIASNAME_ELAN1  RTL_DEV_NAME_NUM(ALIASNAME_ELAN_PREFIX,3)
#define ALIASNAME_ELAN2  RTL_DEV_NAME_NUM(ALIASNAME_ELAN_PREFIX,4)
#define ALIASNAME_ELAN3  RTL_DEV_NAME_NUM(ALIASNAME_ELAN_PREFIX,5)
#define ALIASNAME_ELAN4  RTL_DEV_NAME_NUM(ALIASNAME_ELAN_PREFIX,6)
#define ALIASNAME_ELAN5  RTL_DEV_NAME_NUM(ALIASNAME_ELAN_PREFIX,7)
#define ALIASNAME_ELAN6  RTL_DEV_NAME_NUM(ALIASNAME_ELAN_PREFIX,8)
#define ALIASNAME_ELAN7  RTL_DEV_NAME_NUM(ALIASNAME_ELAN_PREFIX,9)
#define ALIASNAME_ELAN8  RTL_DEV_NAME_NUM(ALIASNAME_ELAN_PREFIX,10)
#define ALIASNAME_ELAN9  RTL_DEV_NAME_NUM(ALIASNAME_ELAN_PREFIX,11)
#define ALIASNAME_ELAN10  RTL_DEV_NAME_NUM(ALIASNAME_ELAN_PREFIX,12)

#if defined(CONFIG_RTL_SMUX_DEV)
#ifndef SMUX_MAX_TAGS
#define SMUX_MAX_TAGS 2
#endif
#define RTL_VDEV_NAME_NUM(name,num)	name"."#num
#define ALIASNAME_VELAN0 RTL_VDEV_NAME_NUM(ALIASNAME_ELAN0, 0)
#define ALIASNAME_VELAN1 RTL_VDEV_NAME_NUM(ALIASNAME_ELAN1, 0)
#define ALIASNAME_VELAN2 RTL_VDEV_NAME_NUM(ALIASNAME_ELAN2, 0)
#define ALIASNAME_VELAN3 RTL_VDEV_NAME_NUM(ALIASNAME_ELAN3, 0)
#define ALIASNAME_VELAN4 RTL_VDEV_NAME_NUM(ALIASNAME_ELAN4, 0)
#define ALIASNAME_VELAN5 RTL_VDEV_NAME_NUM(ALIASNAME_ELAN5, 0)
#define ALIASNAME_VELAN6 RTL_VDEV_NAME_NUM(ALIASNAME_ELAN6, 0)
#define ALIASNAME_VELAN7 RTL_VDEV_NAME_NUM(ALIASNAME_ELAN7, 0)
#define ALIASNAME_VELAN8 RTL_VDEV_NAME_NUM(ALIASNAME_ELAN8, 0)
#define ALIASNAME_VELAN9 RTL_VDEV_NAME_NUM(ALIASNAME_ELAN9, 0)
#define ALIASNAME_VELAN10 RTL_VDEV_NAME_NUM(ALIASNAME_ELAN10, 0)

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
#define ALIASNAME_EXTELAN_PREFIX  "ext0."
#define ALIASNAME_EXTELAN0  RTL_DEV_NAME_NUM(ALIASNAME_EXTELAN_PREFIX,2)
#define ALIASNAME_EXTELAN1  RTL_DEV_NAME_NUM(ALIASNAME_EXTELAN_PREFIX,3)
#define ALIASNAME_EXTELAN2  RTL_DEV_NAME_NUM(ALIASNAME_EXTELAN_PREFIX,4)
#define ALIASNAME_EXTELAN3  RTL_DEV_NAME_NUM(ALIASNAME_EXTELAN_PREFIX,5)
#define ALIASNAME_EXTELAN4  RTL_DEV_NAME_NUM(ALIASNAME_EXTELAN_PREFIX,6)
#define ALIASNAME_EXTELAN5  RTL_DEV_NAME_NUM(ALIASNAME_EXTELAN_PREFIX,7)
#define ALIASNAME_EXTELAN6  RTL_DEV_NAME_NUM(ALIASNAME_EXTELAN_PREFIX,8)
#define ALIASNAME_EXTELAN7  RTL_DEV_NAME_NUM(ALIASNAME_EXTELAN_PREFIX,9)
#define ALIASNAME_EXTELAN8  RTL_DEV_NAME_NUM(ALIASNAME_EXTELAN_PREFIX,10)
#define ALIASNAME_EXTELAN9  RTL_DEV_NAME_NUM(ALIASNAME_EXTELAN_PREFIX,11)
#define ALIASNAME_EXTELAN10  RTL_DEV_NAME_NUM(ALIASNAME_EXTELAN_PREFIX,12)
#endif

enum
{
	SMUX_RULE_PRIO_RX_PHY=326767,
	SMUX_RULE_PRIO_STP=10000,
	SMUX_RULE_PRIO_QOS_HIGH=1100,
	SMUX_RULE_PRIO_PPTP_LAN_UNI=1000,
	SMUX_RULE_PRIO_IGMP=950,
	SMUX_RULE_PRIO_VOIP=950,
	SMUX_RULE_PRIO_CWMP=950,
	SMUX_RULE_PRIO_VEIP=900,
	SMUX_RULE_PRIO_PPTP=800,
	SMUX_RULE_PRIO_BCMC=700,
	SMUX_RULE_PRIO_BCMC_PPTP_FILTER=780,
	SMUX_RULE_PRIO_BCMC_PPTP=770,
	SMUX_RULE_PRIO_BCMC_VEIP_FILTER=760,
	SMUX_RULE_PRIO_BCMC_VEIP=750,
	SMUX_RULE_PRIO_BCMC_FILTER_DROP=740,
	SMUX_RULE_PRIO_BCMC_RULE=730,
	SMUX_RULE_PRIO_MC_WHITE_LIST_PERMIT=720,
	SMUX_RULE_PRIO_MC_WHITE_LIST_DROP=710,
	SMUX_RULE_PRIO_VLAN_MAP=300,
	SMUX_RULE_PRIO_VLAN_GRP=200,
	SMUX_RULE_PRIO_PPTP_LAN_UNI_DEF=100,
	SMUX_RULE_PRIO_TX_PHY=1,
};
#endif

#define ALIASNAME_BR0   RTL_DEV_NAME_NUM(ALIASNAME_BR,0)//"br0"
#define ALIASNAME_WLAN0  RTL_DEV_NAME_NUM(ALIASNAME_WLAN,0)//"wlan0"

#define ALIASNAME_VAP   "-vap" //must include '-' at fast
#define ALIASNAME_WLAN0_VAP  RTL_DEV_NAME_NUM(ALIASNAME_WLAN0,-vap)//"wlan0-vap"

#define ALIASNAME_WLAN0_VAP0  RTL_DEV_NAME_NUM(ALIASNAME_WLAN0_VAP,0)//"wlan0-vap0"
#define ALIASNAME_WLAN0_VAP1  RTL_DEV_NAME_NUM(ALIASNAME_WLAN0_VAP,1)//"wlan0-vap1"
#define ALIASNAME_WLAN0_VAP2  RTL_DEV_NAME_NUM(ALIASNAME_WLAN0_VAP,2)//"wlan0-vap2"
#define ALIASNAME_WLAN0_VAP3  RTL_DEV_NAME_NUM(ALIASNAME_WLAN0_VAP,3)//"wlan0-vap3"

#ifdef CONFIG_E8B
#if defined(CONFIG_CMCC)
#define WAN_VOIP_VOICE_NAME "VOIP"
#define WAN_VOIP_VOICE_NAME_CONN "VOIP_"
#define WAN_TR069_VOIP_VOICE_NAME "TR069_VOIP"
#define WAN_VOIP_VOICE_INTERNET_NAME "VOIP_INTERNET"
#define WAN_TR069_VOIP_VOICE_INTERNET_NAME "TR069_VOIP_INTERNET"
#ifdef CONFIG_SUPPORT_IPTV_APPLICATIONTYPE
#define WAN_IPTV_NAME "IPTV"
#endif
#ifdef CONFIG_SUPPORT_OTT_APPLICATIONTYPE
#define WAN_OTT_NAME "OTT"
#endif
#if defined(CONFIG_SUPPORT_HQOS_APPLICATIONTYPE)
#define WAN_HQOS_NAME "HQoS"
#endif
#else
#define WAN_VOIP_VOICE_NAME "VOICE"
#define WAN_VOIP_VOICE_NAME_CONN "VOICE_"
#define WAN_TR069_VOIP_VOICE_NAME "TR069_VOICE"
#define WAN_VOIP_VOICE_INTERNET_NAME "VOICE_INTERNET"
#define WAN_TR069_VOIP_VOICE_INTERNET_NAME "TR069_VOICE_INTERNET"
#ifdef CONFIG_SUPPORT_IPTV_APPLICATIONTYPE
#define WAN_IPTV_NAME "IPTV"
#endif //CONFIG_SUPPORT_IPTV_APPLICATIONTYPE
#ifdef CONFIG_SUPPORT_OTT_APPLICATIONTYPE
#define WAN_OTT_NAME "OTT"
#endif //CONFIG_SUPPORT_OTT_APPLICATIONTYPE
#if defined(CONFIG_SUPPORT_HQOS_APPLICATIONTYPE)
#define WAN_HQOS_NAME "HQoS"
#endif
#endif
#endif //CONFIG_E8B
#define DEFAULT_DEVICE_NAME "GNT245"

#define WANMODE_CONF_STR "WAN Mode"
#define PON_CONF_STR "PON WAN"
#define ETHWAN_CONF_STR "Ethernet WAN"
#define PTMWAN_CONF_STR "PTM WAN"
#ifdef WLAN_WISP
#define WLWAN_CONF_STR "Wireless WAN"
#endif
#ifdef CONFIG_VDSL
#define DSLWAN_CONF_STR "ATM WAN"
#define ADSL_SETTINGS_STR "DSL Settings"
#define ADSL_STR "DSL"
#define ADSL_TONE_STR "DSL Tone"
#define ADSL_SLV_STR "DSL Slave"
#define ADSL_SLV_TONE_STR "DSL Slave Tone"
#else
#define DSLWAN_CONF_STR "DSL WAN"
#define ADSL_SETTINGS_STR "ADSL Settings"
#define ADSL_STR "ADSL"
#define ADSL_TONE_STR "ADSL Tone"
#define ADSL_SLV_STR "ADSL Slave"
#define ADSL_SLV_TONE_STR "ADSL Slave Tone"
#endif /*CONFIG_VDSL*/
#define ATM_SETTINGS_STR "ATM Settings"
#define ATM_LOOPBACK_STR "ATM Loopback"
#define ADSL_CONNECTION_STR "ADSL Connection"
#ifdef CONFIG_DSL_VTUO
#define VTUO_SETTINGS_STR "VTU-O DSL Settings"
#define VTUO_STATUS_STR "VTU-O DSL"
#endif /*CONFIG_DSL_VTUO*/

#define URLBLOCK_CHAIN "urlblock"

#include <sys/socket.h>
#include <sys/un.h>
#include <linux/if.h>
#include <stdarg.h>
#include <stdio.h>
#include <netdb.h>
#include <dirent.h>
#include <netpacket/packet.h>

#include "adslif.h"
#include "mib.h"
#include "sysconfig.h"
#include "subr_dhcpv6.h"
#include "ipv6_info.h"
#include "options.h"
#include "rtk_timer.h"
#include "limits.h"
#ifdef CONFIG_E8B
#include <rtk/defs_e8.h>
#endif
//#ifdef CONFIG_USER_XDSL_SLAVE
//#include "subr_nfbi_api.h"
//#endif /*CONFIG_USER_XDSL_SLAVE*/
#ifdef WLAN_SUPPORT
#include "subr_wlan.h"
#endif
#ifdef TIME_ZONE
#include "tz.h"
#endif
typedef unsigned long long __u64;

#ifdef USE_DEONET /* sghong, 20240117 */
#define SNMP_MAX_PORT_NUM       5
#define SNMP_MAX_LAN_PORT_NUM   4

struct port_statistics_t{
	unsigned long   bytesReceived;
	unsigned long   bytesSent;
	unsigned int    crc;
};
#endif

#ifdef CONFIG_USER_CMD_SERVER_SIDE
struct rtk_xdsl_atm_channel_info{
	int ch_no;
	short vpi;
	short pad0;
	int vci;
	int rfc;
	int framing;
	int qos_type;
	int pcr;
	int scr;
	int mbs;
	int crd;
	int cdvt;
};
#endif
#if defined(DHCPV6_ISC_DHCP_4XX) && defined(CONFIG_GPON_FEATURE)
#include "ipv6_info.h"
#endif
#ifdef CONFIG_RTK_GENERIC_NETLINK_API
#include <rtk/linux/rtl_genl.h>
#endif	/* CONFIG_RTK_GENERIC_NETLINK_API */

#ifndef RTK_SUCCESS
#define RTK_SUCCESS     0
#endif

#ifndef RTK_FAILED
#define RTK_FAILED      -1
#endif
#ifdef NAT_LOOPBACK
#define DEL_ALL_NATLB_DYNAMIC			1
#define DEL_PORTFW_NATLB_DYNAMIC		2
#define DEL_DMZ_NATLB_DYNAMIC			3
#define DEL_VIR_SER_NATLB_DYNAMIC		4
#endif

#define PROC_NET_DEV_PATH "/proc/net/dev"

#if defined(SUPPORT_BYPASS_RG_FWDENGINE)
/* FON GRE should bypass fwdEngine, so I add a new macro */
#define BYPASS_FWDENGINE_MARK_OFFSET	3
#define BYPASS_FWDENGINE_MARK_MASK		(0x1<<BYPASS_FWDENGINE_MARK_OFFSET)
#endif

#ifdef CONFIG_RTK_DNS_TRAP
#define HOSTS_FILE "/var/hosts"
#define DNS_PROC_FILE "/proc/driver/realtek/domain_name"
#endif

#ifdef RESTRICT_PROCESS_PERMISSION
#define EUID_ROOT 0
#define EUID_OTHER 1
#endif

#define ARP_MONITOR_RUNFILE	"/var/run/arp_monitor.pid"

#ifdef CONFIG_USER_DUMP_CLTS_INFO_ON_BR_MODE
#define GET_BR_CLIENT_RUNFILE "/var/run/cltsOnBRMode.pid"
#endif

#ifdef CONFIG_USER_WAN_PORT_AUTO_SELECTION
#define WAN_PORT_AUTO_SELECTION_BUFFER_SIZE			512

#define WAN_PORT_AUTO_SELECTION_RUNFILE		"/var/run/wan_port_auto_selection.pid"
#define WAN_PORT_AUTO_SELECTION_USOCKET		"/var/run/wan_port_auto_selection.usock"
#define RTL_SYSTEM_REINIT_FILE 		"/var/system_reinit_file"
#define PROBEIF "ifprobe"

void rtk_update_lan_wan_binding(void);
#endif

#ifdef CONFIG_RTL_CFG80211_WAPI_SUPPORT
#define CFG80211_WAPI_TEMP_CERT "/var/tmp/temp.cert"
#define CFG80211_WAPI_CA_CERT   "/var/app/wapi_certs/ca.cert"
#define CFG80211_WAPI_ASU_CERT  "/var/app/wapi_certs/asu.cert"
#define CFG80211_WAPI_USER_CERT "/var/app/wapi_certs/ae.cert"
#endif

#ifdef CONFIG_USER_LLMNR
#define LLMNR_PID "/var/run/llmnrd.pid"
#endif

#ifdef CONFIG_USER_MDNS
#define MDNS_PID "/var/run/mdnsd-br0.pid"
#endif



#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// defined to use pppd, otherwise, use spppd if not defined
//#define USE_PPPD

/* Magician: Debug macro */
/* Example: CWMPDBP(2, "File not fould, file name=%s", filename);*/
/* Output: <DEBUG: abc.c, 1122>File not fould, file name=test.txt */
#define LINE_(line) #line
#define LINE(line) LINE_(line)
#define DBPRINT0(...) while(0){}
#define DBPRINT1(...) fprintf(stderr, "<"__FILE__","LINE(__LINE__)">"__VA_ARGS__)
#define DBPRINT2(...) fprintf(stderr, "<DEBUG:"__FILE__","LINE(__LINE__)">"__VA_ARGS__)
#define DBPRINT(level, ...) DBPRINT##level(__VA_ARGS__)

#define BUF_SIZE		256
#define MAX_POE_PER_VC		5
struct data_to_pass_st {
	int	id;
	char data[BUF_SIZE+1];
};
#ifdef CONFIG_CT_AWIFI_JITUAN_SMARTWIFI
struct default_server_setting {
	int	linenum;
	char data[BUF_SIZE];
	struct default_server_setting *next;
};

extern struct default_server_setting *gServerSetting;

#define CHAIN_MARK_MASK "0xfff"
#define AWIFI_LAN_START_NUM		2
#define AWIFI_LAN_END_NUM		6

typedef enum _t_fw_marks {
    FW_MARK_NONE = 0, /**< @brief No mark set. */
    FW_MARK_PROBATION = 1, /**< @brief The client is in probation period and must be authenticated
			    @todo: VERIFY THAT THIS IS ACCURATE*/
    FW_MARK_KNOWN = 2,  /**< @brief The client is known to the firewall */
    FW_MARK_AUTH_IS_DOWN = 253, /**< @brief The auth servers are down */
    FW_MARK_LOCKED = 254 /**< @brief The client has been locked out */
} t_fw_marks;

#endif

#ifdef TIME_ZONE
#define SNTP_DISABLED 0
#define SNTP_ENABLED 1
#endif

#if defined(CONFIG_RTK_DEV_AP)
typedef struct wlan_hs_k_value{
	unsigned char CCK_A[196];
	unsigned char TSSI_CCK_A[196];
	unsigned char HT40_5G_1S_A[196];
	unsigned char TSSI_HT40_5G_1S_A[196];
}WLAN_HS_K_VALUE_T, *WLAN_HS_K_VALUE_Tp;
#endif

#ifdef CONFIG_USER_CONNTRACK_TOOLS
char *mask2string(unsigned char mask);
#endif

void setup_mac_addr(unsigned char *macAddr, int index);

// Mason Yu. For Set IPQOS
#ifdef CONFIG_USER_IP_QOS
#define		SETIPQOS		0x01

/*
 * Structure used in SIOCSIPQOS request.
 */

struct ifIpQos
{
	int	cmd;
	char	enable;
};
#endif
#if defined(CONFIG_USER_IP_QOS)
enum qos_policy_t
{
	PLY_PRIO=0,
	PLY_WRR,
	PLY_CAR,
	PLY_NONE
};
int updatecttypevalue(MIB_CE_IP_QOS_T *p);
#endif

// Mason Yu
#ifdef IP_PASSTHROUGH
struct ippt_para
{
	unsigned int old_ippt_itf;
	unsigned int new_ippt_itf;
	unsigned char old_ippt_lanacc;
	unsigned char new_ippt_lanacc;
	unsigned int old_ippt_lease;
	unsigned int new_ippt_lease;
};
#endif

// Mason Yu. combine_1p_4p_PortMapping
#if defined(NEW_PORTMAPPING)
#define		VLAN_ENABLE		0x01
#define		VLAN_SETINFO		0x02
#define		VLAN_SETPVIDX		0x03
#define		VLAN_SETTXTAG		0x04
#define		VLAN_DISABLE1PPRIORITY	0x05
#define		VLAN_SETIGMPSNOOP	0x06
#define		VLAN_SETPORTMAPPING	0x07
#define		VLAN_SETIPQOS		0x08
#define		VLAN_VIRTUAL_PORT	0x09
#define		VLAN_SETVLANGROUPING	0x0a
#ifdef CONFIG_PPPOE_PROXY
#define    SET_PPPOE_PROXY_PORTMAP  0x0b
#endif

#ifdef CONFIG_IGMP_FORBID
#define           IGMP_FORBID              0x0a
#endif
#define		TAG_DCARE	0x03
#define		TAG_ADD		0x02
#define		TAG_REMOVE	0x01
#define		TAG_REPLACE	0x00

struct brmap {
	int	brid;
	unsigned char pvcIdx;
};

extern int virtual_port_enabled;
#endif

//for IP rule preference number, higher number means lower priority
enum {
	LAN_PHY_PORT_OIF_PRI_POLICYRT = 25000,
	IP_RULE_STATIC_ROUTE = 27900,
	LAN_DHCP6S_PD_ROUTE = 27945,
	IP_RULE_PRI_PREFIX_INTERFACE_GROUP = 27947,
	IP_RULE_PRI_WAN_NPT_PD_POLICYRT = 27950,
	IP_RULE_PRI_LANRT = 28000,
	IP_RULE_PRI_SRCRT = 29000,
	IP_RULE_PRI_MAPE_LOCAL = 29050,
	IP_RULE_PRI_POLICYRT = 29100,
	IP_RULE_PRI_PREFIXDELEGATERT = 29200,
	IP_RULE_PRI_VPN_POLICYRT = 29300,
	IP_RULE_PRI_DEFAULT = 30000,
};

/*
 * Structure used in SIOCSIFVLAN request.
 */

struct ifvlan
{
	int	cmd;
	char	enable;
	short	vlanIdx;
	short	vid;
	char		disable_priority;
	int	member;
	int	port;
	char	txtag;
};

extern const int virt2user[];

#ifdef MAC_FILTER
#ifdef IGMPHOOK_MAC_FILTER
int reset_IgmpHook_MacFilter(void);
#endif
#endif


#ifdef CONFIG_USER_DHCPCLIENT_MODE
int setupDHCPClient(void);
#endif

#if defined(CONFIG_ANDLINK_SUPPORT) && defined(CONFIG_USER_DHCP_SERVER)
int setupDhcpd_Andlink(char *ifname, char *ip, char *ip_start, char *ip_end);
void start_dhcpd_with_ip_Andlink(char *ifname, char *ip, char *ip_start, char *ip_end);
#endif

#ifdef CONFIG_USER_DDNS		// Mason Yu. Support ddns status file.
void remove_ddns_status(void);
int stop_all_ddns(void);
#endif
#define MSG_BOA_PID		2222
// mtype for configd: Used as mtype to send message to configd; should be
//	well-configured to avoid conflict with the pid of any other processes
//	since the processes use	their pid as mtype to receive reply message
//	from configd.
#define MSG_CONFIGD_PID		8

// Mason Yu. Support ddns status file.
enum ddns_status{
	SUCCESSFULLY_UPDATED=0,
	CONNECTION_ERROR=1,
	AUTH_FAILURE=2,
	WRONG_OPTION=3,
	HANDLING=4,
	LINK_DOWN=5
};

enum ddns_port{
	DDNS_HTTP=80,
	DDNS_HTTPS=443
};

// Mason Yu
enum PortMappingGrp
{
	PM_DEFAULTGRP=0,
	PM_GROUP1=1,
	PM_GROUP2=2,
	PM_GROUP3=3,
	PM_GROUP4=4
};

enum PortMappingAction
{
	PM_PRINT=0,
	PM_ADD=1,
	PM_REMOVE=2
};


extern const char*  wlan[];
extern const char*  wlan_itf[];
extern const int  wlan_en[];

#define SSID_INTERFACE_SHIFT_BIT 16

typedef enum wan_connect_status
{
    FIXED_IP_DISCONNECTED,
    FIXED_IP_CONNECTED,
    DHCP_DISCONNECTED,
    DHCP_CONNECTED,
    PPPOE_DISCONNECTED,
    PPPOE_CONNECTED
}WAN_CONN_STATUS;

//#ifdef NEW_PORTMAPPING
#ifdef WLAN_8_SSID_SUPPORT
typedef enum MARK
{
	PMAP_ETH0_SW0 = 0,
	PMAP_ETH0_SW1,
	PMAP_ETH0_SW2,
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	PMAP_ETH0_SW3=CONFIG_LAN_PORT_NUM,
#else
	PMAP_ETH0_SW3,
#endif
#if defined(CONFIG_RTL_MULTI_LAN_DEV) && defined(CONFIG_USER_SUPPORT_EXTERNAL_SWITCH)
	PMAP_ETH0_SW4,
	PMAP_ETH0_SW5,
	PMAP_ETH0_SW6,
	PMAP_ETH0_SW7,
#else
#if !defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
#if (CONFIG_LAN_PORT_NUM > 4)
	PMAP_ETH0_SW4,
#endif
#if (CONFIG_LAN_PORT_NUM > 5)
	PMAP_ETH0_SW5,
#endif
#if (CONFIG_LAN_PORT_NUM > 6)
	PMAP_ETH0_SW6,
#endif
#endif
#endif
	PMAP_WLAN0,
	PMAP_WLAN0_VAP0,
	PMAP_WLAN0_VAP1,
	PMAP_WLAN0_VAP2,
	PMAP_WLAN0_VAP3,
	PMAP_WLAN0_VAP4,
	PMAP_WLAN0_VAP5,
	PMAP_WLAN0_VAP6,
	PMAP_WLAN1,
	PMAP_WLAN1_VAP0,
	PMAP_WLAN1_VAP1,
	PMAP_WLAN1_VAP2,
	PMAP_WLAN1_VAP3,
	PMAP_WLAN1_VAP4,
	PMAP_WLAN1_VAP5,
	PMAP_WLAN1_VAP6,
	PMAP_ITF_END
} PMAP_LAN_T;
#else
typedef enum MARK
{
	PMAP_ETH0_SW0 = 0,
	PMAP_ETH0_SW1,
	PMAP_ETH0_SW2,
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	PMAP_ETH0_SW3=CONFIG_LAN_PORT_NUM,
#else
	PMAP_ETH0_SW3,
#endif
#if defined(CONFIG_RTL_MULTI_LAN_DEV) && defined(CONFIG_USER_SUPPORT_EXTERNAL_SWITCH)
	PMAP_ETH0_SW4,
	PMAP_ETH0_SW5,
	PMAP_ETH0_SW6,
	PMAP_ETH0_SW7,
#else
#if !defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
#if (CONFIG_LAN_PORT_NUM > 4)
	PMAP_ETH0_SW4,
#endif
#if (CONFIG_LAN_PORT_NUM > 5)
	PMAP_ETH0_SW5,
#endif
#if (CONFIG_LAN_PORT_NUM > 6)
	PMAP_ETH0_SW6,
#endif
#endif
#endif
	PMAP_WLAN0,
	PMAP_WLAN0_VAP0,
	PMAP_WLAN0_VAP1,
	PMAP_WLAN0_VAP2,
	PMAP_WLAN0_VAP3,
	PMAP_WLAN1,
	PMAP_WLAN1_VAP0,
	PMAP_WLAN1_VAP1,
	PMAP_WLAN1_VAP2,
	PMAP_WLAN1_VAP3,
#if defined(CONFIG_RTK_DEV_AP)
	PMAP_WLAN2,
	PMAP_WLAN2_VAP0,
	PMAP_WLAN2_VAP1,
	PMAP_WLAN2_VAP2,
	PMAP_WLAN2_VAP3,
#endif
	PMAP_ITF_END
} PMAP_LAN_T;
//#endif
#endif

#ifdef CONFIG_TELMEX_DEV
#define PMAP_WLAN0_VAP_END PMAP_WLAN0_VAP2
#define PMAP_WLAN1_VAP_END PMAP_WLAN1_VAP2
#else
#ifdef WLAN_8_SSID_SUPPORT
#define PMAP_WLAN0_VAP_END PMAP_WLAN0_VAP6
#define PMAP_WLAN1_VAP_END PMAP_WLAN1_VAP6
#else
#define PMAP_WLAN0_VAP_END PMAP_WLAN0_VAP3
#define PMAP_WLAN1_VAP_END PMAP_WLAN1_VAP3
#endif
#endif

struct itfInfo
{
	#define	DOMAIN_ELAN	0x1
	#define	DOMAIN_WAN	0x2
	#define	DOMAIN_GRE	0x3
	#define	DOMAIN_WLAN	0x4
	#define	DOMAIN_ULAN	0x8	//usbeth
	#define	DOMAIN_WANROUTERCONN 0x10
	int	ifdomain;
	int	ifid;
	char	name[40];// changed by jim
};

// IF_ID(domain, ifIndex)
#define IF_ID(x, y)		((x<<24)|y)
#define IF_DOMAIN(x)		(x>>24)
#define IF_INDEX(x)		(x&0x00ffffff)

#if defined(CONFIG_USBCLIENT) || !defined(CONFIG_RTL8672NIC)
#define DEVICE_SHIFT		5
#else
#define DEVICE_SHIFT		4
#endif
#define IFWLAN_SHIFT		6
#define IFWLAN1_SHIFT 12

#define MAX_NUM_OF_ITFS 32


#ifdef CONFIG_USB_ETH
#ifdef WLAN_SUPPORT
#define IFUSBETH_SHIFT		(IFWLAN_SHIFT+WLAN_MBSSID_NUM+1)
#else
#define IFUSBETH_SHIFT          (IFWLAN_SHIFT+1)
#endif
#define IFUSBETH_PHYNUM		(SW_LAN_PORT_NUM+5+1)  //for ipqos.phyPort (5: wlanphy max, 1:usb0)
#endif //CONFIG_USB_ETH

#define RESERVED_VID_START	4000
#define RESERVED_VID_END	4002

#define	IPQOS_NUM_PKT_PRIO	8
#define	IPQOS_NUM_PRIOQ		MAX_QOS_QUEUE_NUM

#ifdef _PRMT_X_CT_COM_QOS_
#define MODEINTERNET	0
#define MODETR069	1
#define MODEIPTV	2
#define MODEVOIP	3
#define MODEOTHER	4
#endif

struct mymsgbuf;

typedef enum { IP_ADDR, DST_IP_ADDR, SUBNET_MASK, DEFAULT_GATEWAY, HW_ADDR } ADDR_T;
typedef enum {
	SYS_UPTIME,
	SYS_DATE,
	SYS_YEAR,
	SYS_MONTH,
	SYS_DAY,
	SYS_HOUR,
	SYS_MINUTE,
	SYS_SECOND,
#ifdef CONFIG_USER_CMD_SERVER_SIDE
	SYS_FWVERSION_RPHY,
	SYS_DSPVERSION_RPHY,
#endif
	SYS_FWVERSION,
	SYS_HWVERSION,
	SYS_BUILDTIME,
#ifdef CONFIG_00R0
	SYS_BOOTLOADER_FWVERSION,
	SYS_BOOTLOADER_CRC,
	SYS_FWVERSION_SUM_1,
	SYS_FWVERSION_SUM_2,
#endif
#ifdef CPU_UTILITY
	SYS_CPU_UTIL,
#endif
#ifdef MEM_UTILITY
	SYS_MEM_UTIL,
#endif
	SYS_LAN_DHCP,
	SYS_DHCP_LAN_IP,
	SYS_DHCP_LAN_SUBNET,
	SYS_DHCPS_IPPOOL_PREFIX,
	SYS_DNS_MODE,
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	SYS_LAN_IP,
	SYS_LAN_MASK,
#endif
	SYS_WLAN,
	SYS_WLAN_SSID,
	SYS_WLAN_DISABLED,
	SYS_WLAN_HIDDEN_SSID,
	SYS_WLAN_BAND,
	SYS_WLAN_AUTH,
	SYS_WLAN_PREAMBLE,
	SYS_WLAN_BCASTSSID,
	SYS_WLAN_ENCRYPT,
	SYS_WLAN_MODE_VAL,
	SYS_WLAN_ENCRYPT_VAL,
	SYS_WLAN_WPA_CIPHER_SUITE,
	SYS_WLAN_WPA2_CIPHER_SUITE,
	SYS_WLAN_WPA_AUTH,
	SYS_WLAN_PSKFMT,
	SYS_WLAN_PSKVAL,
#ifdef USER_WEB_WIZARD
	SYS_WLAN_PSKVAL_WIZARD,
#endif
	SYS_WLAN_WEP_KEYLEN,
	SYS_WLAN_WEP_KEYFMT,
	SYS_WLAN_WPA_MODE,
	SYS_WLAN_RSPASSWD,
	SYS_WLAN_RS_PORT,
	SYS_WLAN_RS_IP,
	SYS_WLAN_RS_PASSWORD,
	SYS_WLAN_ENABLE_1X,
	SYS_TX_POWER,
	SYS_WLAN_MODE,
	SYS_WLAN_TXRATE,
	SYS_WLAN_BLOCKRELAY,
	SYS_WLAN_AC_ENABLED,
	SYS_WLAN_WDS_ENABLED,
	SYS_WLAN_QoS,
	SYS_WLAN_WPS_ENABLED,
	SYS_WLAN_WPS_STATUS,
	SYS_WLAN_WPS_LOCKDOWN,
	SYS_WSC_DISABLE,
	SYS_WSC_AUTH,
	SYS_WSC_ENC,
	SYS_DHCP_MODE,
	SYS_IPF_OUT_ACTION,
	SYS_DEFAULT_PORT_FW_ACTION,
	SYS_MP_MODE,
	SYS_IGMP_SNOOPING,
	SYS_PORT_MAPPING,
	SYS_IP_QOS,
	SYS_IPF_IN_ACTION,
	SYS_WLAN_BLOCK_ETH2WIR,
	SYS_DNS_SERVER,
	SYS_LAN_IP2,
	SYS_LAN_DHCP_POOLUSE,
	SYS_DEFAULT_URL_BLK_ACTION,
	SYS_DEFAULT_DOMAIN_BLK_ACTION,
	SYS_DSL_OPSTATE,
	SYS_LAN1_VID,
	SYS_LAN2_VID,
	SYS_LAN3_VID,
	SYS_LAN4_VID,
	SYS_LAN1_STATUS,
	SYS_LAN2_STATUS,
	SYS_LAN3_STATUS,
	SYS_LAN4_STATUS,
//#ifdef CONFIG_USER_XDSL_SLAVE
	SYS_DSL_SLV_OPSTATE,
//#endif /*CONFIG_USER_XDSL_SLAVE*/
	SYS_DHCPV6_MODE,
	SYS_DHCPV6_RELAY_UPPER_ITF,
	SYS_LAN_IP6_LL,
	SYS_LAN_IP6_GLOBAL,
	SYS_WLAN_WPA_CIPHER,
	SYS_WLAN_WPA2_CIPHER,
	SYS_LAN_IP6_LL_NO_PREFIX,
	SYS_LAN_IP6_GLOBAL_NO_PREFIX,
	SYS_WAN_DNS,
} SYSID_T;

// enumeration of user process bit-shift for process bit-mapping
typedef enum {
	PID_DNSMASQ=0,
	PID_SNMPD,
	PID_WEB,
	PID_CLI,
	PID_DHCPD,
	PID_DHCPRELAY,
	PID_TELNETD,
	PID_FTPD,
	PID_TFTPD,
	PID_SSHD,
	PID_SYSLOGD,
	PID_KLOGD,
	PID_IGMPPROXY,
	PID_RIPD,
	PID_WATCHDOGD,
	PID_SNTPD,
	PID_MPOAD,
	PID_SPPPD,
	PID_UPNPD,
	PID_UPDATEDD,
	PID_CWMP, /*tr069/cwmpClient pid,jiunming*/
	PID_WSCD,
	PID_MINIUPNPD,
	PID_SMBD,
	PID_NMBD,
#ifdef VOIP_SUPPORT
	PID_VOIPGWDT,
	PID_SOLAR,
#endif
#ifdef CONFIG_USER_MONITORD
	PID_MONITORD,
#endif
#if defined(CONFIG_USER_OPENJDK8) && (defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC))
	PID_JAVA,
#endif
#ifdef CONFIG_USER_LOWMEM_UPGRADE
	PID_ANDLINK,
	PID_SECROUTER,
	PID_MISC,
#endif
} PID_SHIFT_T;

enum PortMappingPriority
{
	HighestPrio=0,
	HighPrio=1,
	MediumPrio=2,
	lowPrio=3
};

enum _WLAN_FLAG
{
	IS_EXIST=1,
	IS_RUN
};

#define TIMER_COUNT 10

#define		PID_SHIFT(x)		(1<<x)
#define		NET_PID			PID_SHIFT(PID_MPOAD)|PID_SHIFT(PID_SPPPD)
#define		ALL_PID			0xffffffff & ~(NET_PID)


#ifdef _USE_RSDK_WRAPPER_
#include <sys/syslog.h>
#endif //_USE_RSDK_WRAPPER_

int startSSDP(void);
int IfName2ItfId(char *s);
int do_ioctl(unsigned int cmd, struct ifreq *ifr);
int isDirectConnect(struct in_addr *haddr, MIB_CE_ATM_VC_Tp pEntry);
int getInAddr(char *interface, ADDR_T type, void *pAddr);
int getInMTU(const char *interface, int *mtu);
int setInMtu(const char *interface, int mtu);
int getInFlags(const char *interface, int *flags );
int setInFlags(const char *interface, int flags );
int is_interface_run(const char *interface);
int INET_resolve(char *name, struct sockaddr *sa);
int read_pid(const char *filename);
int getLinkStatus(struct ifreq *ifr);
char *trim_white_space(char *str);

extern const char AUTO_RESOLV[];
extern const char DNS_RESOLV[];
extern const char DNS6_RESOLV[];
extern const char PPP_RESOLV[];
extern const char RESOLV[];
extern const char DNSMASQ_CONF[];
extern const char RESOLV_BACKUP[];
extern const char HOSTS[];

int generate_ifupdown_core_script(void);
#define IFUPDOWN_CONFIG_FOLDER "/var/network"	// symbolic to /etc/network
#define IFUPDOWN_UP_SCRIPT_DIR		IFUPDOWN_CONFIG_FOLDER"/if-up.d"
#define IFUPDOWN_DOWN_SCRIPT_DIR	IFUPDOWN_CONFIG_FOLDER"/if-down.d"
#define IFUPDOWN_UP_SCRIPT_NAME		"if-%s-up"
#define IFUPDOWN_DOWN_SCRIPT_NAME	"if-%s-down"
#define IFUPDOWN_DEBUG_FILE	"/var/ifupdown_script_debug"
#define IFUPDOWN_MAX_DEBUG_LINE	20
#ifdef CONFIG_USER_PPPD
#ifdef CONFIG_USER_PPPD_DEFAULT_PATH
#define PPPD_CONFIG_FOLDER "/var/ppp"	// symbolic to /etc/ppp
#else
#define PPPD_CONFIG_FOLDER "/var/run"
#endif /* CONFIG_USER_PPPD_DEFAULT_PATH */
#define PPPD_DEBUG_FILE	"/var/ppp_script_debug"
#define PPPD_MAX_DEBUG_LINE	20
#define PPPD_PEERS_FOLDER 			PPPD_CONFIG_FOLDER"/peers"
#define PPPD_INIT_SCRIPT_DIR		PPPD_CONFIG_FOLDER"/ppp-init.d"
#define PPPD_INIT_SCRIPT_NAME		"init-%s"
#define PPPD_V4_UP_SCRIPT_DIR		PPPD_CONFIG_FOLDER"/ip-up.d"
#define PPPD_V4_DOWN_SCRIPT_DIR		PPPD_CONFIG_FOLDER"/ip-down.d"
#define PPPD_V4_UP_SCRIPT_NAME		"ip-%s-up"
#define PPPD_V4_DOWN_SCRIPT_NAME	"ip-%s-down"
#ifdef CONFIG_IPV6
#define PPPD_V6_UP_SCRIPT_DIR		PPPD_CONFIG_FOLDER"/ipv6-up.d"
#define PPPD_V6_DOWN_SCRIPT_DIR		PPPD_CONFIG_FOLDER"/ipv6-down.d"
#define PPPD_V6_UP_SCRIPT_NAME		"ipv6-%s-up"
#define PPPD_V6_DOWN_SCRIPT_NAME	"ipv6-%s-down"
#endif
#if defined(CONFIG_USER_PPTP_CLIENT) || defined(CONFIG_USER_XL2TPD)
#define PPPD_VPN_SCRIPT_DIR			PPPD_CONFIG_FOLDER"/ppp-vpn.d"
#define PPPD_VPN_DOWN_SCRIPT_NAME	"vpn-%s-down"
#endif

#if defined(CONFIG_USER_PPPOMODEM) && defined(CONFIG_USER_PPPD)
#define POM_CONFIG_FOLDER 			PPPD_CONFIG_FOLDER"/rtk_modem"

#define POM_OPTION_FILE				POM_CONFIG_FOLDER"/options-mobile"
#define POM_CHAT_SCRIPT				POM_CONFIG_FOLDER"/mobile-modem.chat"
#define POM_CHAT_REPORT				POM_CONFIG_FOLDER"/pppom.result"

// "pppd call {ISP name}" will dial out with /etc/ppp/peers/{ISP name} option file.
#define POM_ISP_NAME				"pppom"
#define POM_ISP_OPTION				PPPD_PEERS_FOLDER"/"POM_ISP_NAME

/* User may not plug 3G dongle when we enable 3g dongle feature,
 * So, we call this script to wait serial device ready(Ex: ttyUSB0)
 * this script is installed by boa/src/Linux/Makefile
 */
#define POM_LAUNCH_SCRIPT			"/etc/scripts/wait-dialup-hardware.sh"
#define POM_LAUNCH_SCRIPT_PID_FILE	PPPD_CONFIG_FOLDER"/pom_launch.pid"

#define POM_SIM_PIN_LEN 4
#endif /* defined(CONFIG_USER_PPPOMODEM) && defined(CONFIG_USER_PPPD) */
#endif /* CONFIG_USER_PPPD */

#ifdef CONFIG_USER_MINIUPNPD
#ifdef CONFIG_USER_HAPD
#define MINIUPNP_SERVICE_PORT 52869
#else
#define MINIUPNP_SERVICE_PORT 49152
#endif

extern const char MINIUPNPDPID[];
extern const char MINIUPNPD_CONF[];
#ifdef CONFIG_USER_MINIUPNP_V2_1
int setup_minuupnpd_conf(char *ext_if, char *lan_if);
#endif
#endif
extern const char MINIDLNAPID[];

extern const char SYSTEMD_USOCK_SERVER[];
extern const char EPONOAMD_USOCK_SERVER[];
extern const char RA_INFO_FILE[];
extern const char RA_INFO_PREFIX_KEY[];
extern const char RA_INFO_PREFIX_LEN_KEY[];
extern const char RA_INFO_PREFIX_ONLINK_KEY[] ;
extern const char RA_INFO_RDNSS_KEY[];
extern const char RA_INFO_DNSSL_KEY[];
extern const char RA_INFO_DNSSL_LIFETIME_KEY[];
extern const char RA_INFO_M_BIT_KEY[];
extern const char RA_INFO_O_BIT_KEY[];
extern const char RA_INFO_PREFIX_IFNAME_KEY[];
extern const char RA_INFO_PREFIX_MTU_KEY[];
extern const char RA_INFO_PREFIX_GW_KEY[];
extern const char RA_INFO_LIFETIME_KEY[];

#define MAX_CONFIG_FILESIZE 400000
//extern int wlan_idx;	// interface index
// Added by Kaohj
extern const char ALL_LANIF[];
extern const char LANIF[];
extern const char LOCALHOST[];
extern const char LAN_ALIAS[];	// alias for secondary IP
#if defined(CONFIG_SECONDARY_IP) && defined(WLAN_QTN)
extern const char LAN_RGMII[];
#endif
#ifdef CONFIG_NET_IPGRE
extern const char* GREIF[];
extern struct callout_s gre_ch;
void poll_GRE(void *dummy);
int cmd_set_gre(unsigned char enable, unsigned char idx, int endpoint_idx);
char *find_Route_WAN_IP(struct in_addr *haddr, char *ip);
char *find_Internet_WAN_IP(char *ip);
#define GRE_BACKUP_INTERVAL 120
#endif
#ifdef IP_PASSTHROUGH
extern const char LAN_IPPT[];	// alias for IP passthrough
#endif
extern const char ELANIF[];
extern const char LANIF_ALL[];
#ifdef CONFIG_RTL_MULTI_LAN_DEV
#ifdef CONFIG_RTL9601B_SERIES
#define ELANVIF_NUM 1//eth lan virtual interface number
#else
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
#define ELANVIF_NUM (CONFIG_LAN_PORT_NUM+CONFIG_EXTERNAL_SWITCH_LAN_PORT_NUM)
#else
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
#define ELANVIF_NUM (CONFIG_LAN_PORT_NUM+1) //eth lan virtual interface number
#else
#ifdef CONFIG_USER_RTK_EXTGPHY
#define ELANVIF_NUM (CONFIG_LAN_PORT_NUM+1) //ecternal phy lan virtual interface number
#else
#define ELANVIF_NUM CONFIG_LAN_PORT_NUM//eth lan virtual interface number
#endif
#endif
#endif
#endif //CONFIG_RTK_SUPPORT_EXTERNAL_SWITCH
#else
#define ELANVIF_NUM 1
#endif
//#if defined(CONFIG_ETHWAN) || defined(CONFIG_RTL_MULTI_LAN_DEV)
extern const char* ELANRIF[];
extern const char* ELANVIF[];
extern const char* SW_LAN_PORT_IF[];
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
extern const char* EXTELANRIF[];
#endif
//#endif

extern const char BRIF[];
#ifdef CONFIG_DEV_xDSL
extern const char VC_BR[];
extern const char LLC_BR[];
extern const char VC_RT[];
#endif
extern const char BLANK[];
#ifdef CONFIG_DEV_xDSL
extern const char LLC_RT[];
#endif
extern const char PORT_DNS[];
extern const char PORT_DHCP[];
extern const char ARG_ADD[];
extern const char ARG_REPLACE[];
extern const char ARG_CHANGE[];
extern const char ARG_DEL[];
extern const char ARG_ENCAPS[];
extern const char ARG_QOS[];
extern const char ARG_255x4[];
extern const char ARG_0x4[];
extern const char ARG_BKG[];
extern const char ARG_I[];
extern const char ARG_O[];
extern const char ARG_T[];
extern const char ARG_TCP[];
extern const char ARG_UDP[];
extern const char ARG_NO[];
extern const char ARG_ICMP[];
extern const char ARG_NONE[];
extern const char ARG_ARP[];
extern const char ARG_IPV4[];
extern const char ARG_IPV6[];
extern const char ARG_PPPOE[];
extern const char ARG_WAKELAN[];
extern const char ARG_APPLETALK[];
extern const char ARG_PROFINET[];
extern const char ARG_FLOWCONTROL[];
extern const char ARG_IPX[];
extern const char ARG_MPLS_UNI[];
extern const char ARG_MPLS_MULTI[];
extern const char ARG_802_1X[];
extern const char ARG_LLDP[];
extern const char ARG_RARP[];
extern const char ARG_NETBIOS[];
extern const char ARG_X25[];
extern const char FW_BLOCK[];
extern const char FW_INACC[];
extern const char FW_ICMP_FILTER[];
extern const char FW_ACL[];
extern const char FW_DROPLIST[];
extern const char FW_CWMP_ACL[];
extern const char FW_BR_WAN[];
extern const char FW_BR_WAN_FORWARD[];
extern const char FW_BR_LAN_FORWARD[];
extern const char FW_BR_WAN_OUT[];
extern const char FW_BR_LAN_OUT[];
extern const char FW_BR_PPPOE[];
extern const char FW_BR_PPPOE_ACL[];
extern const char FW_DHCP_PORT_FILTER[];
extern const char BIN_IP[];
extern const char BIN_BRIDGE[];
extern const char ITFGRP_RTTBL[];
extern const char PORTMAP_RTTBL[];
extern const char PORTMAP_EBTBL[];
extern const char PORTMAP_INTBL[];
extern const char PORTMAP_OUTTBL[];
extern const char PORTMAP_SEVTBL[];
extern const char PORTMAP_BRTBL[];

extern const char WAN_OUTPUT_RTTBL[];

#ifdef CONFIG_E8B
extern const char PORTMAP_SEVTBL2[];
#ifdef CONFIG_CU
extern const char PORTMAP_SEVTBL_RT[];
extern const char PORTMAP_SEVTBL_RT2[];
#endif
#endif
#if (defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
extern const char L2_PPTP_PORT_SERVICE[];
extern const char L2_PPTP_PORT_INPUT_TBL[];
extern const char L2_PPTP_PORT_OUTPUT_TBL[];
extern const char L2_PPTP_PORT_FORWARD_TBL[];
extern const char L2_PPTP_PORT_FORWARD[];
#endif
extern const char FW_ISOLATION_MAP[];
extern const char FW_ISOLATION_BRPORT[];
#ifdef CONFIG_YUEME
extern const char FRAMEWORK_ADDR_MAP[];
#endif
extern const char NAT_ADDRESS_MAP_TBL[];
extern const char FW_PARENTAL_CTRL[];
#ifdef DOMAIN_BLOCKING_SUPPORT
extern const char MARK_FOR_DOMAIN[];
#endif
#ifdef CONFIG_USER_ANDLINK_PLUGIN
extern const char SSID2_WLAN_FILTER[];
#endif
#ifdef IP_PORT_FILTER
extern const char FW_IPFILTER_IN[];
extern const char FW_IPFILTER_OUT[];
extern const char FW_IPFILTER[];
#endif
#ifdef CONFIG_USER_RTK_VOIP
extern const char IPTABLE_VOIP[];
extern const char VOIP_REMARKING_CHAIN[];
#endif
#if (defined(IP_PORT_FILTER) || defined(DMZ)) && defined(CONFIG_TELMEX_DEV)
extern const char FW_DMZFILTER[];
#endif
#ifdef PORT_FORWARD_GENERAL
extern const char PORT_FW[];
#endif
#if defined(PORT_FORWARD_GENERAL) || defined(DMZ)
#ifdef NAT_LOOPBACK
extern const char PORT_FW_PRE_NAT_LB[];
extern const char PORT_FW_POST_NAT_LB[];
#ifdef CONFIG_NETFILTER_XT_TARGET_NOTRACK
extern const char PF_RAW_TRACK_NAT_LB[];
extern const char DMZ_RAW_TRACK_NAT_LB[];
#endif
#endif
#endif
#ifdef VIRTUAL_SERVER_SUPPORT
#ifdef NAT_LOOPBACK
extern const char VIR_SER_PRE_NAT_LB[];
extern const char VIR_SER_POST_NAT_LB[];
#endif
#endif
#if defined(CONFIG_USER_PPTP_SERVER)
extern const char IPTABLE_PPTP_VPN_IN[];
extern const char IPTABLE_PPTP_VPN_FW[];
#endif
#if defined(CONFIG_USER_L2TPD_LNS)
extern const char IPTABLE_L2TP_VPN_IN[];
extern const char IPTABLE_L2TP_VPN_FW[];
#endif
#ifdef DMZ
#ifdef NAT_LOOPBACK
extern const char IPTABLE_DMZ_PRE_NAT_LB[];
extern const char IPTABLE_DMZ_POST_NAT_LB[];
#endif
extern const char IPTABLE_DMZ[];
#endif
#ifdef CONFIG_USER_AVALANCH_DETECT
extern const char IPTABLE_AVALANCH_DETECT[];
#endif
#ifdef NATIP_FORWARDING
extern const char IPTABLE_IPFW[];
extern const char IPTABLE_IPFW2[];
#endif
#if defined(PORT_TRIGGERING_STATIC) || defined(PORT_TRIGGERING_DYNAMIC)
extern const char IPTABLES_PORTTRIGGER[];
#endif
#if defined(CONFIG_USER_RTK_NAT_ALG_PASS_THROUGH) || (defined(CONFIG_TRUE) && defined(CONFIG_IP_NF_ALG_ONOFF))
extern const char FW_VPNPASSTHRU[];
#endif
//#if defined(CONFIG_USER_BOA_PRO_PASSTHROUGH) && defined(CONFIG_RTK_DEV_AP)
#ifdef CONFIG_USER_BOA_PRO_PASSTHROUGH
extern const char IPSEC_PASSTHROUGH_FORWARD[];
extern const char L2TP_PASSTHROUGH_FORWARD[];
extern const char PPTP_PASSTHROUGH_FORWARD[];
#endif
extern const char PORT_IPSEC[];
extern const char PORT_IPSEC_NAT[];
extern const char PORT_L2TP[];
extern const char PORT_PPTP[];

#if defined(CONFIG_CU_BASEON_YUEME) && defined(COM_CUC_IGD1_ParentalControlConfig)
extern const char FW_PARENTAL_MAC_CTRL_LOCALIN[];
extern const char FW_PARENTAL_MAC_CTRL_FWD[];
#endif
#ifdef CONFIG_E8B
extern const char FW_MACFILTER_FWD_L3[];
#endif
extern const char FW_MACFILTER_FWD[];
extern const char FW_MACFILTER_MC[];
extern const char FW_MACFILTER_LI[];
extern const char FW_MACFILTER_LO[];
#if defined(CONFIG_00R0) && defined(CONFIG_USER_INTERFACE_GROUPING)
extern const char FW_INTERFACE_GROUP_DROP_IGMP[];
extern const char FW_INTERFACE_GROUP_DROP_FORWARD[];
#endif
extern const char FW_IPQ_MANGLE_DFT[];
extern const char FW_IPQ_MANGLE_USER[];
extern const char IPTABLE_TR069[];
#ifdef CONFIG_IPV6
extern const char IPTABLE_IPV6_TR069[];
extern const char IPTABLE_IPV6_MAC_AUTOBINDING_RS[];
extern const char IPV6_RA_LO_FILTER[];
#endif
#if defined(CONFIG_USER_DATA_ELEMENTS_AGENT) || defined(CONFIG_USER_DATA_ELEMENTS_COLLECTOR)
extern const char IPTABLE_DATA_ELEMENT[];
#ifdef CONFIG_IPV6
extern const char IP6TABLE_DATA_ELEMENT[];
#endif
#endif
#ifdef CONFIG_USER_GREEN_PLUGIN
extern const char SELF_CONTROL_CHAIN_NAME[];
#ifdef CONFIG_IPV6
extern const char SELF_CONTROL6_CHAIN_NAME[];
#endif
#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
extern const char FW_IN_COMMING[];
extern const char FW_FTP_ACCOUNT[];
extern const char FW_SAMBA_ACCOUNT[];
extern const char FW_TELNET_ACCOUNT[];
#endif
#ifdef CONFIG_USER_DHCPCLIENT_MODE
extern const char REDIRECT_URL_TO_BR0[];
#endif

extern const char ROUTE_BRIDGE_WAN_SUFFIX[];

#ifdef CONFIG_DEV_xDSL
#ifdef CONFIG_USER_IGMPPROXY
extern const char IGMP_OUTPUT_VPRIO_RESET[];
#endif
#if (defined(CONFIG_IPV6) && defined(CONFIG_USER_MLDPROXY))
extern const char MLD_OUTPUT_VPRIO_RESET[];
#endif
#endif
extern const char FW_DROP[];
extern const char FW_ACCEPT[];
extern const char FW_RETURN[];
extern const char FW_REJECT[];
extern const char FW_FORWARD[];
extern const char FW_INPUT[];
extern const char FW_OUTPUT[];
extern const char FW_POSTROUTING[];
extern const char FW_PREROUTING[];
extern const char FW_DPORT[];
extern const char FW_SPORT[];
extern const char FW_ADD[];
extern const char FW_DEL[];
extern const char FW_FLUSH[];
extern const char FW_INSERT[];
extern const char FW_DSCP_REMARK[];
#ifdef PORT_FORWARD_ADVANCE
extern const char FW_PPTP[];
extern const char FW_L2TP[];
extern const char *PFW_Gategory[];
extern const char *PFW_Rule[];
int config_PFWAdvance( int action_type );
#endif
extern const char *strItf[];
extern const char CONFIG_HEADER[];
extern const char CONFIG_TRAILER[];
extern const char CONFIG_HEADER_HS[];
extern const char CONFIG_TRAILER_HS[];
extern const char CONFIG_XMLFILE[];
extern const char CONFIG_RAWFILE[];
extern const char CONFIG_XMLFILE_HS[];
extern const char CONFIG_RAWFILE_HS[];
extern const char CONFIG_XMLENC[];
extern const char PPP_SYSLOG[];
extern const char PPP_DEBUG_LOG[];
#ifdef CONFIG_PPP
extern const char PPP_CONF[];
extern const char PPPD_FIFO[];
extern const char PPPOA_CONF[];
extern const char PPPOE_CONF[];
extern const char PPP_PID[];
extern const char SPPPD[];
extern const char PPP_DOWN_S_PREFIX[];
#endif

#ifdef CONFIG_GUEST_ACCESS_SUPPORT
extern const char GUEST_ACCESS_CHAIN_EB[];
extern const char GUEST_ACCESS_CHAIN_IP[];
extern const char GUEST_ACCESS_CHAIN_IPV6[];
#endif

#ifdef CONFIG_USER_CTCAPD
extern const char WIFI_PERMIT_IN_TBL[];
extern const char WIFI_PERMIT_FWD_TBL[];
#endif

#ifdef CONFIG_USER_RT_ACL_SYN_FLOOD_PROOF_SUPPORT
extern const char FW_LAN_SYN_FLOOD_PROOF[];
#endif
#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
extern const char FW_CONTROL_PACKET_PROTECTION[];
#endif
extern const char FW_TCP_SMALLACK_PROTECTION[];
#ifdef CONFIG_USER_CMD_SERVER_SIDE
#define SWITCH_WAN_PORT 3
#define SWITCH_WAN_PORT_MASK	 (1<<SWITCH_WAN_PORT)
#define CMD_CLIENT_MONITOR_INTF ALIASNAME_PTM0
extern const char CMDSERV_CTRLERD[];
#endif
#ifdef CONFIG_USER_CMD_CLIENT_SIDE
#define CMD_SERVER_MONITOR_INTF "eth0"	//for 0882
extern const char CMDSERV_ENDPTD[];
#endif
extern const char ADSLCTRL[];
extern const char IFCONFIG[];
//extern const char IWPRIV[];
extern const char BRCTL[];
extern const char MPOAD[];
extern const char MPOACTL[];
extern const char DHCPD[];
extern const char DHCPC[];
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ)
extern const char DNSRELAY[];
extern const char DNSRELAYPID[];
extern const char DNSRELAY_PROCESS[];
#endif
#ifdef CONFIG_USER_CTC_EOS_MIDDLEWARE
extern const char RTK_RESTART_ELINKCLT_FILE[];
#endif
#ifdef CONFIG_USER_CONNTRACK_TOOLS
extern const char CONNTRACK_TOOL[];
#endif
extern const char SMUXCTL[];
extern const char WEBSERVER[];
extern const char SNMPD[];
extern const char ROUTE[];
extern const char IPTABLES[];
extern const char IPSET[];
extern const char IPSET_COMMAND_RESTORE[];
extern const char IPSET_SETTYPE_HASHIP[];
extern const char IPSET_SETTYPE_HASHIPV6[];
extern const char IPSET_SETTYPE_HASHNET[];
extern const char IPSET_SETTYPE_HASHNETV6[];
extern const char IPSET_SETTYPE_HASHMAC[];
extern const char IPSET_OPTION_COMMENT[];
extern const char IPSET_OPTION_COUNTERS[];
extern const char ARP_MONITOR[];

extern const char EMPTY_MAC[MAC_ADDR_LEN];

#ifdef CONFIG_SUPPORT_CAPTIVE_PORTAL
extern const char FW_CAPTIVEPORTAL_FILTER[];
#endif

/*ql 20081114 START need ebtables support*/
extern const char EBTABLES[];
extern const char ZEBRA[];
#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
extern const char OSPFD[];
#endif
extern const char RIPD[];
extern const char ROUTED[];
extern const char IGMPROXY[];
#ifdef CONFIG_USER_IGMPPROXY_SUPPORT_CFG_FILE
extern const char IGMPPROXY_CONF[];
#endif
#ifdef CONFIG_USER_MLDPROXY
extern const char MLDPROXY[];
extern const char MLDPROXY_CONF[];
#endif
extern const char TC[];
extern const char NETLOGGER[];
#ifdef TIME_ZONE
extern const char SNTPC[];
extern const char SNTPC_PID[];
#endif
#ifdef CONFIG_USER_LANNETINFO
extern const char LANNETINFO[];
extern const char FW_HOST_MIB_PREROUTING[];
extern const char FW_HOST_MIB_POSTROUTING[];
#endif
#ifdef CONFIG_USER_DDNS
extern const char DDNSC_PID[];
#endif
extern const char BOA_PID[];
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
extern const char MANUAL_LOAD_BALANCE_PREROUTING[];
extern const char MANUAL_LOAD_BALANCE_INPUT[];
extern const char MANUAL_LOAD_BALANCE_FORWARD[];
extern const char MANUAL_LOAD_BALANCE_OUTPUT[];
#endif

#ifdef BLOCK_LAN_MAC_FROM_WAN
extern const char FW_BLOCK_LANHOST_MAC_FROM_WAN[];
#endif
#ifdef BLOCK_INVALID_SRCIP_FROM_WAN
extern const char FW_BLOCK_INVALID_SRCIP_FROM_WAN[];
#endif

#ifdef CONFIG_USER_MONITORD
#define MONITORD_PID "/var/run/monitord.pid"
#define MONITOR_LIST "/var/monitor_list"
#define MONITOR_LIST_LOCK "/var/monitor_list.lock"
int update_monitor_list_file(char *process_name, int action);
int startMonitord(int debug);
#endif

enum {
	URL_FILTER_MODE_DISABLE = 0,
	URL_FILTER_MODE_WHITELIST,
	URL_FILTER_MODE_BLACKLIST,
};

#ifdef CONFIG_USER_CTCAPD
#define RTK_CTCAPD_URL_FILTER_BLACKLIST 0
#define RTK_CTCAPD_URL_FILTER_WHITELIST 1
#endif

extern const char PROC_DYNADDR[];
extern const char PROC_IPFORWARD[];
extern const char PROC_FORCE_IGMP_VERSION[];
extern const char MPOAD_FIFO[];
extern const char STR_DISABLE[];
extern const char STR_ENABLE[];
extern const char STR_UNNUMBERED[];
extern const char rebootWord0[];
extern const char rebootWord1[];
extern const char rebootWord2[];
extern const char errGetEntry[];
extern const char MER_GWINFO[];
extern const char MER_GWINFO_V6[];
extern const char WAN_IPV4_CONN_TIME[];
extern const char WAN_IPV6_CONN_TIME[];

#ifdef CONFIG_USER_CTCAPD
#define CTCAPD_RUNFILE "/var/run/ctcapd.pid"
#define CTCAPD_FW_FLAG_FILE "/var/ctcapd_firewall_flag"
#endif

extern const char *n0to7[];
extern const char *prioLevel[];
extern const int priomap[];
extern const char *ipTos[];
//alex
#ifdef CONFIG_8021P_PRIO
extern const char *set1ptable[];
#endif

#if defined(CONFIG_RTL8681_PTM)
extern const char PTMIF[];
#endif

#ifdef CONFIG_USB_ETH
extern const char USBETHIF[];
#endif //CONFIG_USB_ETH

//Alan 20160728
extern const char hideErrMsg1[];
extern const char hideErrMsg2[];

#ifdef CONFIG_USER_LOCK_NET_FEATURE_SUPPORT
extern const char NETLOCK_INPUT[];
extern const char NETLOCK_FWD[];
extern const char NETLOCK_OUTPUT[];
extern const char BR_NETLOCK_INPUT[];
extern const char BR_NETLOCK_FWD[];
extern const char BR_NETLOCK_OUTPUT[];
#endif

#ifdef CONFIG_USER_CUPS
int getPrinterList(char *str, size_t size);
#endif // CONFIG_USER_CUPS

extern const char STR_NULL[];
extern const char DHCPC_PID[];
extern const char DHCPC_ROUTERFILE[];
extern const char DHCPC_SCRIPT[];
extern const char DHCPC_SCRIPT_NAME[];
extern const char DHCPD_CONF[];
extern const char DHCPD_LEASE[];
extern const char DHCPSERVERPID[];
extern const char DHCPRELAYPID[];
extern const char MANUFACTURER_OUI[];
extern const char PRODUCT_CLASS[];

#if defined(URL_BLOCKING_SUPPORT) || defined(URL_ALLOWING_SUPPORT)
extern const char WEBURL_CHAIN[];
extern const char WEBURLPOST_CHAIN[];
extern const char WEBURLPRE_CHAIN[];
extern const char WEBURL_CHAIN_BLACK[];
extern const char WEBURL_CHAIN_WHITE[];
#endif

#ifdef _PRMT_X_CMCC_SECURITY_
extern const char PARENTALCTRL_CHAIN[];
extern const char PARENTALCTRLPOST_CHAIN[];
extern const char PARENTALCTRLPRE_CHAIN[];
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU)
extern const char LXC_FWD_TBL[];
extern const char LXC_IN_TBL[];
#endif
#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
extern const char FW_PUSHWEB_FOR_UPGRADE[];
#endif
#ifdef SUPPORT_DOS_SYSLOG
extern const char FW_DOS[];
#endif

//#ifdef XOR_ENCRYPT
//Jenny, Configuration file encryption
extern const char XOR_KEY[];
void xor_encrypt(char *inputfile, char *outputfile);
void xor_encrypt2(char *buf, int len, int *index);
//#endif

extern const char PW_HOME_DIR[];
extern const char PW_CMD_SHELL[];
extern const char PW_NULL_SHELL[];
#ifdef	CONFIG_RTK_CTCAPD_FILTER_SUPPORT
extern const char MARK_FOR_URL_FILTER[];
extern const char MARK_FOR_DNS_FILTER[];
#define DNSFILTERMODE 1
#define URLFILTERMODE 0
#define ADD_UBUS_FILTER 0
#define MODIFY_UBUS_FILTER 1
#define UBUS_CMD_LENTH CTCAPD_FILTER_ENABLE_LENTH+CTCAPD_FILTER_DOMAIN_LENTH*CTCAPD_FILTER_DOMAIN_NUM+CTCAPD_FILTER_NAME_LENTH+CTCAPD_FILTER_MAC_NUM*CTCAPD_FILTER_MAC_LENTH+CTCAPD_FILTER_WEEKDAY_LENTH+CTCAPD_FILTER_TIMERANGE_LENTH+96

typedef struct rtk_ctcapd_filter{
	unsigned char enable[CTCAPD_FILTER_ENABLE_LENTH];
	unsigned char dnsaddr[CTCAPD_FILTER_DOMAIN_LENTH*CTCAPD_FILTER_DOMAIN_NUM];
	unsigned char name[CTCAPD_FILTER_NAME_LENTH];
	unsigned char macaddr[CTCAPD_FILTER_MAC_NUM*CTCAPD_FILTER_MAC_LENTH];
	unsigned int  action;
	unsigned int  mode;
	unsigned char weekdays[CTCAPD_FILTER_WEEKDAY_LENTH];
	unsigned char timerange[CTCAPD_FILTER_TIMERANGE_LENTH];
	unsigned int index;
}RTK_CTCAPD_FILTER_T, *RTK_CTCAPD_FILTER_Tp;
#endif
extern const char *ppp_auth[];
extern const char *vpn_auth[];
#if defined(CONFIG_USER_BRIDGE_GROUPING)
void setgroup(char *list, unsigned char grpnum);
int get_group_ifinfo(struct itfInfo *info, int len, unsigned char grpnum);

struct availableItfInfo {
	#define DOMAIN_ELAN	0x1
	#define DOMAIN_WAN	0x2
	#define DOMAIN_WLAN	0x4
	#define DOMAIN_ULAN	0x8
	unsigned int ifdomain;
	unsigned int ifid;
	char name[40];
	unsigned char itfGroup;
};

#ifdef CONFIG_00R0
int getATMVCEntryandIDByIfindex(unsigned int ifindex, MIB_CE_ATM_VC_T *p, unsigned int *id);
int setup_Layer2BrFilter(int idx);
int setup_Layer2BrMarking(int idx);
#endif
int get_AvailableInterface(struct availableItfInfo *info, int len);
#endif

#ifdef CONFIG_IPOE_ARPING_SUPPORT
#define ARPING_HANDLER_POLLING_INTERVAL	1
extern unsigned long arping_handler_timer_count;
extern const char ARPING_IPOE_RUNFILE[];
extern const char ARPING_RESULT[];
extern const char ARPING_IPOE[];
extern struct callout_s arping_lcp_ch;
void arpingLcpHandler(void *dummy);
#endif

#if defined(_SUPPORT_INTFGRPING_PROFILE_)
#define INTFGRPING_DOMAIN_ELAN 0x01
#define INTFGRPING_DOMAIN_WAN  0x02
#define INTFGRPING_DOMAIN_WLAN 0x04
#define INTFGRPING_DOMAIN_LANROUTERCONN 0x08
#define INTFGRPING_DOMAIN_WANROUTERCONN 0x10
#define INTFGRPING_DOMAIN_ELAN_VLAN 0x20
#define INTFGRPING_DOMAIN_WLAN_VLAN 0x40

struct layer2bridging_availableinterface_info
{
	unsigned int ifdomain;
	unsigned int ifid;
	char name[40];
	unsigned char itfGroup;
};
#endif

#if defined(CONFIG_RTL_IGMP_SNOOPING) || defined(CONFIG_RTL_MLD_SNOOPING) || defined(CONFIG_BRIDGE_IGMP_SNOOPING)
void rtk_multicast_snooping(void);	// handle IGMP/MLD snooping function, this api should call after mib_set(MIB_MPMODE)
#endif
#if defined(CONFIG_RTL_IGMP_SNOOPING)
void __dev_setupIGMPSnoop(int flag);	// enable/disable IGMP snooping
#endif

#if defined(CONFIG_USER_RTK_NOSTB_IPTV)
void __dev_NOSTBsetting(int flag);
int startNOSTB(char* ifname);
#endif

#if defined(CONFIG_RTL_MLD_SNOOPING)
void __dev_setupMLDSnoop(int flag);
#endif
void __dev_setupDirectBridge(int flag);
unsigned int hextol(unsigned char *hex);
int get_domain_ifinfo(struct itfInfo *info, int len, int ifdomain);


struct process_nice_table_entry {
	char	processName[64];
	unsigned int	nice; //-20~19
};

int _do_cmd(const char *cmd, char *argv[], int dowait, int donice, int dooutput, char *output);
#define do_cmd(a,b,c) _do_cmd(a,b,c,0,3,NULL)
#define do_nice_cmd(a,b,c) _do_cmd(a,b,c,1,3,NULL)
int do_cmd_fout(const char *cmd, char *argv[], int dowait, char *output);
int va_cmd_fout(const char *cmd, int num, int dowait, char *file, ...);
int va_cmd(const char *cmd, int num, int dowait, ...);  //return 0:OK, other:fail
int va_niced_cmd(const char *cmd, int num, int dowait, ...);  //return 0:OK, other:fail
int va_cmd_no_echo(const char *cmd, int num, int dowait, ...);  //return 0:OK, other:fail
int call_cmd(const char *cmd, int num, int dowait, ...);	//return 0:OK, other:fail
int va_cmd_no_error(const char *cmd, int num, int dowait, ...);
#if 1 //def CONFIG_RTK_DEV_AP
int RunSystemCmd(char *filepath, ...);
int DoCmd(char *argv[], char *file);
#endif
int write_to_mpoad(struct data_to_pass_st *);
int startDhcpc(char *inf, MIB_CE_ATM_VC_Tp pEntry, int is_diag);
void config_AddressMap(int action);
int startIP_v4(char *inf, MIB_CE_ATM_VC_Tp pEntry, CHANNEL_MODE_T ipEncap,
						int *isSyslog, char*msg);
int startIP(char *inf, MIB_CE_ATM_VC_Tp pEntry, CHANNEL_MODE_T ipEncap);
#if defined(_PRMT_X_CMCC_IPOEDIAGNOSTICS_) || defined(_PRMT_X_CT_COM_IPoEDiagnostics_)
#define IPOE_DIAG_RESULT_DHCPC_FILE  "/tmp/ipoe_diag_dhcp"
#define IPOE_DIAG_RESULT_PING_FILE  "/tmp/ipoe_diag_ping"
int ipoeSimulationStart(int ifIndex, unsigned char *mac, unsigned char *VendorClassID, char *ping_host, unsigned int repitation, unsigned int timeout);
#endif
#ifdef CONFIG_PPP
void stopPPP(void);
int startPPP(char *inf, MIB_CE_ATM_VC_Tp pEntry, char *qos, CHANNEL_MODE_T pppEncap);
int find_ppp_from_conf(char *pppif);
void write_to_pppd(struct data_to_pass_st *);
#endif
#if defined(CONFIG_SUPPORT_AUTO_DIAG)||defined(_PRMT_X_CT_COM_IPoEDiagnostics_)
struct DIAG_RESULT_T
{
	unsigned char 	result[128];
	unsigned int 	errCode;
	unsigned int 	ipType;
	unsigned int 	sessionId;
	unsigned char 	aftr[256];
	unsigned char 	ipAddr[16];
	unsigned char 	gateWay[16];
	unsigned char 	dns[16];
	unsigned char 	ipv6Addr[256];
	unsigned char 	ipv6GW[256];
	unsigned char 	ipv6DNS[256];
	unsigned char 	ipv6Prefix[256];
	unsigned char 	ipv6LANPrefix[256];
	unsigned char 	authMSG[256];
#ifdef _PRMT_X_CT_COM_IPoEDiagnostics_
	unsigned char 	netmask[16];
#endif
};
void addSimuEthWANdev(MIB_CE_ATM_VC_Tp pEntry, int autosimu);
int pppoeSimulationStart(char* devname, char* username, char* password, PPP_AUTH_T auth, int ipv6AddrMode);
int getSimulationResult(char* devname, struct DIAG_RESULT_T* state);
unsigned int getOptionFromleases(char* filename, char* iaprfix, char* dns, char* aftr);
int initAutoBridgeFIFO(void);
int setOmciState(int state);
int setSimuDebug(int debug);
int poll_msg(int fd);
int stopPPPoESimulation(char *devname);
int startAutoBridgePppoeSimulation(char* wanname);
int stopAutoBridgePppoeSimulation(char* wanname);
int generate_ifupv6_script_manual(char* wanif, MIB_CE_ATM_VC_Tp pEntry);
int setup_simu_ethernet_config(MIB_CE_ATM_VC_Tp pEntry, char *wanif);
#endif
int _get_classification_mark(int entryNo, MIB_CE_IP_QOS_T *p);
int get_classification_mark(int entryNo);
int startConnection(MIB_CE_ATM_VC_Tp pEntry, int);
void stopConnection(MIB_CE_ATM_VC_Tp pEntry);
int rtk_wan_is_link_status_change(char *ifname, unsigned int ifi_flags);
void rtk_wan_clear_link_status(char *ifname);
void rtk_wan_log_link_status(char *ifname, unsigned int ifi_flags);

MIB_CE_ATM_VC_T *getATMVCEntryByIfName(char* ifname, MIB_CE_ATM_VC_T *p);
int rtk_get_lan_bind_wan_ifname(char *lan_ifname, char *wan_ifname);
#if defined(CONFIG_USER_PORT_ISOLATION)
int checkPortIsolationEntry(void);
#endif
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
int checkPortBindEntry(void);
MIB_CE_PORT_BINDING_T* getPortBindEntryByLogPort(int logPort, MIB_CE_PORT_BINDING_T *p, int *mibIdx);
#endif

#define BINDCHECK_DOMAIN_PORT_BINDING 0x01
#define BINDCHECK_DOMAIN_VLAN_BINDING  0x02
#define BINDCHECK_DOMAIN_VLAN_TRAN 0x04

int rtk_util_check_binding_port(int logPort, int filterFalg);

#ifdef CONFIG_USER_RTK_SYSLOG
extern char *log_severity[8];
int stopLog(void);
int startLog(void);
#endif
#if defined(BB_FEATURE_SAVE_LOG) || defined(CONFIG_USER_RTK_SYSLOG)
#define SYSLOGDMERGEDFILE "/var/tmp/syslogd.txt"
#define SYSLOGDFILE "/var/run/syslogd.txt"
#define SYSLOGDFILE_SAVE "/var/config/syslogd.txt"

void writeLogFileHeader(FILE * fp);
int syslogFileMergeFromFlash(void);
#endif

#ifdef DEFAULT_GATEWAY_V2
int ifExistedDGW(void);
#endif
#ifdef CONFIG_USER_PPPOMODEM
int wan3g_start(void);
int wan3g_enable(void);
void wan3g_stop(void);
#endif
#ifdef CONFIG_USER_Y1731
int Y1731_start(int dowait);
#endif
#ifdef CONFIG_USER_8023AH
int EFM_8023AH_start(int dowait);
#endif
#ifdef CONFIG_LUNA
void rtkbosa_config_init(void);
#endif

int msgProcess(struct mymsgbuf *qbuf);
#if !defined(CONFIG_MTD_NAND) && defined(CONFIG_BLK_DEV_INITRD)
static inline int flashdrv_filewrite(FILE * fp, int size, void *dstP)
{
	(void)fp;(void)size;(void)dstP;
	return 0;
}
#else
int flashdrv_filewrite(FILE * fp, int size, void *dstP);
#endif


#ifdef CONFIG_USER_IGMPPROXY
#ifdef CONFIG_USER_IGMPPROXY_SUPPORT_CFG_FILE
int setup_igmpproxy_conf(void);
#endif
int startIgmproxy(void);
#ifdef CONFIG_IGMPPROXY_MULTIWAN
int setting_Igmproxy(void);
#endif
extern const char IGMPPROXY_PID[];
#endif
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_MLDPROXY
int setup_mldproxy_conf(void);
int startMLDproxy(void);		// Mason Yu. MLD Proxy
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
void checkMldProxyItf(void);
#endif
void update_mldproxy_vif(MIB_CE_ATM_VC_T *pEntry);
#endif	//CONFIG_USER_MLDPROXY
extern const char MLDPROXY_PID[];
#endif
#ifdef CONFIG_USER_DOT1AG_UTILS
void arrange_dot1ag_table();
void startDot1ag();
#endif
int setupMacFilter(void);
#ifdef LAYER7_FILTER_SUPPORT
int setupAppFilter(void);
#endif
#ifdef PARENTAL_CTRL
int parent_ctrl_table_init(void);
int parent_ctrl_table_add(MIB_PARENT_CTRL_T *addedEntry);
int parent_ctrl_table_del(MIB_PARENT_CTRL_T *addedEntry);
int parent_ctrl_table_rule_update(void);
#endif
#ifdef DMZ
int setupDMZ(void);
#endif
#ifdef CONFIG_USER_AVALANCH_DETECT
void init_avalanch_detect_fw(void);
void setup_avalanch_detect_fw(void);
void clear_avalanch_detect_fw(void);
void rtk_avalanche_iptables_accept_add(void);
void rtk_avalanche_iptables_accept_delete(void);
int rtk_switch_avalanche_process(int stop);
#ifdef CONFIG_RTL9607C_SERIES
int rt_cls_avalanche_set(void);
int rt_cls_avalanche_del(void);
int diag_avalanche_vlan_set(void);
int diag_avalanche_vlan_del(void);
int diag_avalanche_l2_set(void);
int diag_avalanche_l2_del(void);
#endif
#if defined(CONFIG_LUNA_G3_SERIES)
int rtk_avalanche_ebtables(int set);
#endif
#endif
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
int rtk_set_fc_skbmark_fwdByPs(int on);
#endif
#if defined(CONFIG_LUNA) && defined(CONFIG_COMMON_RT_API)
void rtk_fw_setup_storm_control(void);
void rtk_fw_setup_arp_storm_control(void);
#endif
#ifdef CONFIG_DEV_xDSL
#ifdef CONFIG_USER_CMD_SERVER_SIDE
#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
int commserv_gfast_msg_set(unsigned int id, void *data);
int commserv_gfast_msg_get(unsigned int id, void *data);
#endif
int commserv_OAMLoobback(unsigned int action, void *data);
int commserv_atmSetup(unsigned int action, struct rtk_xdsl_atm_channel_info *data);
int setup_remote_ATM_channel(MIB_CE_ATM_VC_Tp pEntry);
int startCommserv(void);
#endif
int find_pvc_from_running(int vpi, int vci);
int setup_ATM_channel(MIB_CE_ATM_VC_Tp pEntry);
#if defined(CONFIG_USER_CMD_CLIENT_SIDE) || defined(CONFIG_USER_CMD_SERVER_SIDE)
int startCommserv(void);
#endif
char adsl_drv_get(unsigned int id, void *rValue, unsigned int len);
#ifdef CONFIG_VDSL
char dsl_msg_set_array(int msg, int *pval);
char dsl_msg_set(int msg, int val);
char dsl_msg_get_array(int msg, int *pval);
char dsl_msg_get(int msg, int *pval);
#endif /*CONFIG_VDSL*/

#define DEV_NAME_MAX_LEN 128

struct id_info
{
	int id;
	char dev_name[DEV_NAME_MAX_LEN];
	FILE *file_ptr;
};

struct id_info* id_info_search(char *dev_name);

typedef struct _xdsl_op_
{
	int id;
	char (*xdsl_drv_get)(unsigned int id, void *rValue, unsigned int len);
#ifdef CONFIG_VDSL
	char (*xdsl_msg_set_array)(int msg, int *pval);
	char (*xdsl_msg_get_array)(int msg, int *pval);
	char (*xdsl_msg_set)(int msg, int val);
	char (*xdsl_msg_get)(int msg, int *pval);
#endif /*CONFIG_VDSL*/
	int  (*xdsl_get_info)(int id, char *buf, int len);
} XDSL_OP;
XDSL_OP *xdsl_get_op(int id);
#ifdef CONFIG_DSL_VTUO
int VTUOSetupChan(void);
int VTUOSetupGinp(void);
int VTUOSetupLine(void);
int VTUOSetupInm(void);
int VTUOSetupSra(void);
int VTUOSetupDpbo(void);
int VTUOSetupPsd(void);
int VTUOCheck(void);
int VTUOSetup(void);
#endif
int setupDsl(void);
#if defined(CONFIG_USER_XDSL_SLAVE)
int setupSlvDsl(void);
#endif
int testOAMLoopback(MIB_CE_ATM_VC_Tp pEntry, unsigned char scope, unsigned char type);
#endif

void bypassBlock(FILE *fp);
#if defined(DHCPV6_ISC_DHCP_4XX) && defined(CONFIG_GPON_FEATURE)
int getIANAInfo(const char *fname, IANA_INFO_Tp pInfo);
#endif
int getLeasesInfo(const char *fname, DLG_INFO_Tp pInfo);
int getLANPortStatus(unsigned int id, char *strbuf);
int getLANPortStatus4Syslog(unsigned int id, char *strbuf);
int getLANPortConfig(unsigned int id, char *strbuf);
int applyLanPortSetting_each(int pid, unsigned char capability);
int applyLanPortSetting(void);
#define getMIB2Str(id, str) getMIB2Str_s(id, str, sizeof(str))
int getMIB2Str_s(unsigned int id, char str[], int str_len);
int getSYS2Str(SYSID_T id, char *str);
int ifWanNum(const char *type); /* type="all", "rt", "br" */
int getSYSInfoTimer(void);

#ifdef CONFIG_TELMEX_DEV
extern int getFailReasonbyIfindex(unsigned int ifindex, char* reasonStr);
extern const char *loid_get_authstatus(int index);
#endif
#if defined(NEED_CHECK_DHCP_SERVER)||defined(NEED_CEHCK_WLAN_INTERFACE)
#ifdef WLAN_SUPPORT
int needModifyWlanInterface(void);
#endif
#ifdef CONFIG_USER_DHCP_SERVER
int needModifyDhcpMode(void);
#endif
#ifdef CONFIG_GPON_FEATURE
int isHybridGponmode(void);
#endif
int isOnlyOneBridgeWan(void);
int checkSpecifiedFunc(void);
#endif

#define ACL_ENTRY_DENY_SET	0x0
#define ACL_ENTRY_WAN_SIDE_PERMISSION_SET 0x01
#define ACL_ENTRY_LAN_SIDE_PERMISSION_SET 0x02

#ifdef REMOTE_ACCESS_CTL
void remote_access_modify(MIB_CE_ACC_T accEntry, int enable);
void filter_set_remote_access(int enable);
#ifdef CONFIG_E8B
int set_icmp_Firewall(int action);
#ifdef _PRMT_SC_CT_COM_Ping_Enable_
void rtk_cwmp_set_sc_ct_ping_enable(int enable);
#endif //_PRMT_SC_CT_COM_Ping_Enable_
#endif //CONFIG_E8B
#endif //REMOTE_ACCESS_CTL
#ifdef IP_ACL
#ifdef CONFIG_00R0
int filter_set_acl(void);
#else
void filter_set_acl(int enable);
#endif
#endif
#ifdef NAT_CONN_LIMIT
int restart_connlimit(void);
void set_conn_limit(void);
#endif
#ifdef TCP_UDP_CONN_LIMIT
int restart_connlimit(void);
void set_conn_limit(void);
#endif
#ifdef DOS_SUPPORT
void DoS_syslog(int signum);
#endif
#if defined(DOS_SUPPORT)||defined(_PRMT_X_TRUE_FIREWALL_) || defined(FIREWALL_SECURITY) || defined(CONFIG_E8B)
int startFirewall(void);
int changeFwGrade(unsigned char enable, int currGrade);
#endif
#ifdef _PRMT_X_CMCC_SECURITY_
int get_Templates_entry_by_inst_num(unsigned int num, MIB_PARENTALCTRL_TEMPLATES_Tp pEntry, int *idx);
void ParentalCtrl_chain_setup(int action, unsigned char *MACAddress);
void ParentalCtrl_MAC_Policy_Set(unsigned char *MACAddress, int mode);
void ParentalCtrl_rule_free(void);
void ParentalCtrl_rule_dump(void);
void ParentalCtrl_rule_init(void);
int ParentalCtrl_time_check(int StartTime, int EndTime, unsigned int RepeatDay);
int ParentalCtrl_rule_set(void);
#endif

#if defined(URL_BLOCKING_SUPPORT) || defined(URL_ALLOWING_SUPPORT)
void filter_set_url(int enable);
int restart_urlblocking(void);
void set_url_filter(void);
int restart_urlfilter(void);
#ifdef SUPPORT_URL_FILTER
void set_url_filter_config(void);
int rtk_url_get_iptables_drop_pkt_count(MIB_CE_URL_FILTER_T *);
#endif
#endif
int parse_ur_filter(char *url, char *key, int *port);

void itfcfg(char *if_name, int up_flag);
#ifdef SUPPORT_URL_FILTER
void DeleteSplitcharFromURLFilter(char *macstring, char *macstring2);
void changeMacstringForURUFilter(char *macstring, char *macstring2);
#endif
#ifdef SUPPORT_DNS_FILTER
#define DNSFILTERFILENAME "/var/dnsfilter_record"
#define TMPDNSFILTERFILENAME "/var/tmp_dnsfilter_record"
int UpdateDNSFilterBlocktime(char *name, char *url, int blocktime);
int getDNSFilterBlockedTimes(char *name, char *url);
#endif
#ifdef DOMAIN_BLOCKING_SUPPORT
void filter_set_domain(int enable);
void filter_set_domain_default(void);
int restart_domainBLK(void);
void patch_for_office365(void);
#endif
#ifdef CONFIG_ANDLINK_SUPPORT
void filter_set_andlink(int enable);
int filter_restart_andlink(void);
#endif
#if defined(CONFIG_USER_ROUTED_ROUTED) || defined(CONFIG_USER_ZEBRA_OSPFD_OSPFD)
int startRip(void);
#endif
#ifdef CONFIG_USER_QUAGGA_RIPNGD
int startRipng(void);
#endif
#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
int startOspf(void);
#endif
#ifdef CONFIG_USER_DHCP_SERVER
#ifdef SUPPORT_DHCP_RESERVED_IPADDR
int clearDHCPReservedIPAddrByInstNum(unsigned int instnum);
#endif
int setupDhcpd(void);
int startDhcpRelay(void);
#endif

int startDhcp(void);
#ifdef CONFIG_AUTO_DHCP_CHECK
void start_lan_Auto_DHCP_Check();
#endif

typedef struct {
	unsigned int	key;		/* magic key */

#define BOOT_IMAGE             0xB0010001
#define CONFIG_IMAGE           0xCF010002
#define APPLICATION_IMAGE      0xA0000003
#ifdef CONFIG_RTL8686
#define APPLICATION_UBOOT      	0xA0000103	/*uboot only*/
#define APPLICATION_UIMAGE      0xA0000203	/*uimage only*/
#define APPLICATION_ROOTFS      0xA0000403	/*rootfs only*/
#endif
#define BOOTPTABLE             0xB0AB0004


	unsigned int	address;	/* image loading DRAM address */
	unsigned int	length;		/* image length */
	unsigned int	entry;		/* starting point of program */
	unsigned short	chksum;		/* chksum of */

	unsigned char	type;
#define KEEPHEADER    0x01   /* set save header to flash */
#define FLASHIMAGE    0x02   /* flash image */
#define COMPRESSHEADER    0x04       /* compress header */
#define MULTIHEADER       0x08       /* multiple image header */
#define IMAGEMATCH        0x10       /* match image name before upgrade */


	unsigned char	   date[25];  /* sting format include 24 + null */
	unsigned char	   version[16];
	unsigned int  *flashp;  /* pointer to flash address */

} IMGHDR;

//ql_xu ---signature header
#define SIG_LEN			20
typedef struct {
	unsigned int sigLen;	//signature len
	unsigned char sigStr[SIG_LEN];	//signature content
	unsigned short chksum;	//chksum of imghdr and img
}SIGHDR;

struct wstatus_info {
	unsigned int ifIndex;
	char ifname[IFNAMSIZ];
	char devname[IFNAMSIZ];
	char ifDisplayName[IFNAMSIZ];
	unsigned int tvpi;
	unsigned int tvci;
	int cmode;
	char encaps[8];
	char protocol[10];
	char ipver;	// IPv4 or IPv6
	char ipAddr[20];
	char remoteIp[20];
	char *strStatus;
	char *strStatus_IPv6;
	char uptime[20];
	char totaluptime[20];
	char vpivci[12];
	int pppDoD;
	int itf_state;
	int link_state;
	unsigned int recordNum;
#if defined(CONFIG_LUNA)
	unsigned short vid;
#ifdef CONFIG_00R0
	unsigned char MacAddr[MAC_ADDR_LEN];
#endif
	char serviceType[30];
#endif
	unsigned char WanName[MAX_NAME_LEN];
	char mask[20];

};

unsigned short ipchksum(unsigned char *ptr, int count, unsigned short resid);

#ifdef PORT_FORWARD_GENERAL
void clear_dynamic_port_fw(int (*upnp_delete_redirection)(unsigned short eport, const char * protocol));
int setupPortFW(void);
void portfw_modify( MIB_CE_PORT_FW_T *p, int del);
#if defined(CONFIG_RTK_DEV_AP)
int iptable_fw_natLB( int del, unsigned int ifIndex, const char *proto, const char *extPort, const char *intPort, const char *dstIP, MIB_CE_PORT_FW_T *pentry);
#else
int iptable_fw_natLB( int del, unsigned int ifIndex, const char *proto, const char *extPort, const char *intPort, const char *dstIP);
#endif
#endif
#ifdef VIRTUAL_SERVER_SUPPORT
int iptable_virtual_server_natLB( int del, unsigned int ifIndex, const char *proto, const char *extPort, const char *intPort, const char *dstIP);
#endif
#ifdef TIME_ZONE
int isTimeSynchronized(void);
#ifdef CONFIG_E8B
unsigned int check_ntp_if(void);
int setupNtp(int type);
#endif
int startNTP(void);
int stopNTP(void);
#endif

//#if defined(CONFIG_USER_BOA_PRO_PASSTHROUGH) && defined(CONFIG_RTK_DEV_AP)
#ifdef CONFIG_USER_BOA_PRO_PASSTHROUGH
int Setup_VpnPassThrough(void);
#endif

#ifdef CONFIG_USER_MINIDLNA
void startMiniDLNA(void);
void stopMiniDLNA(void);
void stopMiniDLNA_force(void);
void stopMiniDLNA_wait(void);
void restartMiniDLNA(void);
#endif

#if defined(CONFIG_USER_IP_QOS)
int stopIPQ(void);
int setupIPQ(void);
void restartQRule(void);
void take_qos_effect(void);
unsigned int getQoSQueueNum(void);
int UpdateQoSOrder(int index, int action, int oldorder);
int rtk_qos_tcp_smallack_protection(void);
#ifdef _PRMT_X_CT_COM_QOS_
int getcttype(char *typename, MIB_CE_IP_QOS_T *p, int typeinst);
int setcttype(char *typename, MIB_CE_IP_QOS_T *p, int typeinst);
int getcttypevalue(char *typevalue, MIB_CE_IP_QOS_T *p, int typeinst, int maxflag);
int setcttypevalue(char *typevalue, MIB_CE_IP_QOS_T * p, int typeinst, int maxflag);
int delcttypevalue(MIB_CE_IP_QOS_T *p, int typeinst);
#endif
#ifdef CONFIG_E8B
int setMIBforQosMode(unsigned char *qosMode);
#endif

int clsTypeProtoToIPQosProto(int clsTypeProto);
int typeProtoToIPQosProto(int proto_type);

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_AP_E8B_QOS)
unsigned int getQosClassficationType(char *type, char *value);
unsigned int getQosClassficationProtocol(int ClsIndex);
void updateQosClsTypeEntry(int ClsIndex);
unsigned int GetQosClsType(unsigned int clsType);
char* QosClsTypeToStr(unsigned int clsType);
int getProtoType(char *proto);
char *get_clsType_protoStr(int protoType);
unsigned char getValidClsID(void);
int getClsPostionInMib(int cls_id);
int getClassificationByInstNum(unsigned int instnum, MIB_CE_IP_QOS_CLASSFICATION_T * p, unsigned int *id);
unsigned int find_max_classification_type_instanNum_by_clsid(unsigned char cls_id);
unsigned int find_max_classification_instanNum(void);
unsigned int classification_type_total_by_clsid(unsigned char cls_id);
int getClassificationTypeByInstNumAndClsid(unsigned char cls_id,unsigned int instnum, MIB_CE_IP_QOS_CLASSFICATIONTYPE_T * p,unsigned int *id);
#endif
#endif

unsigned int getUpLinkRate(void);
#ifdef CONFIG_NET_IPGRE
int setupDscpTag(void);
int stopDscpTag(void);
void update_gre_ssid(void);
int setupGreQoS(void);
int stopGreQoS(void);
int applyGRE(int enable, int idx, int endpoint_idx);
void gre_take_effect(int endpoint_idx);
#endif

/*ql:20081114 START: support GRED*/
#define UNDOWAIT 0
#define DOWAIT 1
#define MAX_SPACE_LEGNTH 1024

#define DOCMDINIT \
		char cmdargvs[MAX_SPACE_LEGNTH]={0};\
		int argvs_index=1;\
		char *_argvs[64];\
        char *_saveptr = NULL;

#define DOCMDARGVS(cmd,dowait,format,args...) do { \
		argvs_index=1;\
		memset(cmdargvs,0,sizeof(cmdargvs));\
		memset(_argvs,0,sizeof(_argvs));\
		snprintf(cmdargvs,sizeof(cmdargvs),format , ##args);\
		fprintf(stderr,"%s %s\n",cmd,cmdargvs);\
		_argvs[argvs_index]=strtok(cmdargvs," ");\
		while(_argvs[argvs_index]){\
			_argvs[++argvs_index]=strtok(NULL," ");\
		}\
		do_cmd(cmd,_argvs,dowait); \
		}while(0)

#define DONICEDCMDARGVS(cmd,dowait,format,args...) do { \
		argvs_index=1;\
		memset(cmdargvs,0,sizeof(cmdargvs));\
		memset(_argvs,0,sizeof(_argvs));\
		_saveptr = NULL; \
		snprintf(cmdargvs,sizeof(cmdargvs),format , ##args);\
		fprintf(stderr,"%s %s\n",cmd,cmdargvs);\
		_argvs[argvs_index]=strtok_r(cmdargvs," ", &_saveptr);\
		while(_argvs[argvs_index]){\
			_argvs[++argvs_index]=strtok_r(NULL," ", &_saveptr);\
		}\
		do_nice_cmd(cmd,_argvs,dowait); \
		}while(0)
/*ql:20081114 END*/

#ifdef CONFIG_XFRM
#ifdef CONFIG_USER_STRONGSWAN
#define SWANCTL_CONF "/var/swanctl.conf"
#define STRONGSWAN_CONF "/var/strongswan.conf"
unsigned int rtk_ipsec_find_max_ipsec_instnum(void);
int rtk_ipsec_get_ipsec_entry_by_instnum( unsigned int instnum, MIB_IPSEC_SWAN_T *p, unsigned int *id );
void ipsec_strongswan_take_effect(void);
#if defined(CONFIG_USER_XL2TPD)
#ifdef CONFIG_USER_L2TPD_LNS
void l2tpIPSec_LNS_strongswan_take_effect(void);
#endif
void l2tpIPSec_LAC_strongswan_take_effect(void);
#endif
void create_pthread_for_ipsecswan(void);
void update_ipsec_swan_status(void);
#else
#define SETKEY_CONF "/tmp/setkey.conf"
#define RACOON_CONF "/tmp/racoon.conf"
#define RACOON_PID "/var/run/racoon.pid"
#define RACOON_LOG "/tmp/racoon.log"
#define PSK_FILE "/tmp/psk.txt"
#define CERT_FLOD "/var/config"
#define DHGROUP_INDEX(x)	((x >> 24) & 0xff)
#define ENCRYPT_INDEX(x)	((x >> 16) & 0xff)
#define AHAUTH_INDEX(x)	((x >> 8) & 0xff)
#define AUTH_INDEX(x)	(x & 0xff)
struct IPSEC_PROP_ST
{
    char valid;
	char name[MAX_NAME_LEN];
	unsigned int algorithm; //dhGroup|espEncryption|ahAuth|espAuth
};

void ipsec_take_effect(void);
void apply_nat_rule(MIB_IPSEC_T *pEntry);
extern struct IPSEC_PROP_ST ikeProps[];
extern char *curr_dhArray[];
extern char *curr_encryptAlgm[];
extern char *curr_authAlgm[];
#endif
#endif

#if (defined(CONFIG_USER_L2TPD_LNS) && defined(CONFIG_USER_XL2TPD)) || (defined(CONFIG_USER_PPTP_CLIENT) && defined(CONFIG_USER_PPTP_SERVER))
void getVpnServerLocalIP(unsigned char vpn_type, char *ipaddr);
void getVpnServerSubnet(unsigned char vpn_type, char *str_subnet);
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
int applyL2TP(MIB_L2TP_T *pentry, int enable, int l2tp_index);
void l2tp_take_effect(void);
int getIfIndexByL2TPName(char *pIfname);
MIB_L2TP_T *getL2TPEntryByIfname(char *ifname, MIB_L2TP_T *p, int *entryIndex);
MIB_L2TP_T *getL2TPEntryByIfIndex(unsigned int ifIndex, MIB_L2TP_T *p, int *entryIndex);
MIB_L2TP_T *getL2TPEntryByTunnelName(char *tunnelName, MIB_L2TP_T *p, int *entryIndex);
#if defined(CONFIG_USER_L2TPD_LNS)
void setup_l2tp_server_firewall(void);
#endif
#endif


#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
#define ConfigVpnLock "/var/run/configVpnLock" //for VPN Lock file

#if defined(CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_PPTP_SERVER)
void applyPptpAccount(MIB_VPN_ACCOUNT_T *pentry, int enable);
#endif
#ifdef CONFIG_USER_PPTPD_PPTPD
void pptpd_take_effect(void);
#endif
void applyPPtP(MIB_PPTP_T *pentry, int enable, int pptp_index);
void pptp_take_effect(void);
MIB_PPTP_T *getPPTPEntryByIfname(char *ifname, MIB_PPTP_T *p, int *entryIndex);
MIB_PPTP_T *getPPTPEntryByTunnelName(char *tunnelName, MIB_PPTP_T *p, int *entryIndex);
MIB_PPTP_T *getPPTPEntryByIfIndex(unsigned int ifIndex, MIB_PPTP_T *p, int *entryIndex);
int getIfIndexByPPTPName(char *pIfname);
#endif
#ifdef CONFIG_USER_L2TPD_LNS
void applyL2tpAccount(MIB_VPN_ACCOUNT_T *pentry, int enable);
#endif
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
int startSnmp(void);
int restart_snmp(int flag);
#endif
#if defined(CONFIG_USER_CWMP_TR069) || defined(APPLY_CHANGE)
void off_tr069(void);
#endif
typedef enum {SAMBA_SERVICE, FTP_SERVICE, TELNET_SERVICE, SSH_SERVICE, ICMP_SERVICE} service_type;

#define RTK_SERVICE_DENY_ACCESS 0x00
#define RTK_SERVICE_LAN_SIDE_ALLOW 0x01
#define RTK_SERVICE_WAN_SIDE_ALLOW 0x02
#if defined(IP_ACL)
#ifdef AUTO_LANIP_CHANGE_FOR_IP_ACL
void autoChangeLanipForIpAcl(unsigned int origip, unsigned int origmask, unsigned int newip, unsigned int newmask);
#endif
int checkACL(char *pMsg, int idx, unsigned char aclcap);
int restart_acl(void);
int get_ip_acl_tbl_service(int service_type, unsigned char *enable);
int set_ip_acl_tbl_service(int service_type, unsigned char enable);
#endif
int delete_line(const char *filename, int delete_line);
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
int restart_dnsrelay(void); //Jenny
int restart_dnsrelay_ex(char *ifname, int delay);
int reload_dnsrelay(char *ifname);

int delete_hosts(unsigned char *yiaddr);
#endif
int restart_dhcp(void);
int restart_lanip(void);

//#include <netinet/if_ether.h>
 struct rtl_ethhdr {
        unsigned char   h_dest[6];       /* destination eth addr */
        unsigned char   h_source[6];     /* source ether addr    */
        unsigned short  h_proto;         /* packet type ID field */
} __attribute__((packed));

struct ezmesh_arpMsg {
	struct rtl_ethhdr ethhdr;	 		/* Ethernet header */
	u_short htype;				/* hardware type (must be ARPHRD_ETHER) */
	u_short ptype;				/* protocol type (must be ETH_P_IP) */
	u_char  hlen;				/* hardware address length (must be 6) */
	u_char  plen;				/* protocol address length (must be 4) */
	u_short operation;			/* ARP opcode */
	u_char  sHaddr[6];			/* sender's hardware address */
	u_char  sInaddr[4];			/* sender's IP address */
	u_char  tHaddr[6];			/* target's hardware address */
	u_char  tInaddr[4];			/* target's IP address */
	u_char  pad[18];			/* pad for min. Ethernet payload (60 bytes) */
};
#define MAC_BCAST_ADDR		(unsigned char *) "\xff\xff\xff\xff\xff\xff"
int ezmesh_send_arp(u_int32_t yiaddr, u_int32_t ip, unsigned char *mac, char *interface);

#ifdef CONFIG_USER_ROUTED_ROUTED
int delRipTable(unsigned int ifindex);
#endif
int delPPPoESession(unsigned int ifindex);
MIB_CE_ATM_VC_T *getATMVCEntryByIfIndex(unsigned int ifIndex, MIB_CE_ATM_VC_T *p);
int getATMVcEntryByName(char *pIfname, MIB_CE_ATM_VC_T *pEntry);
int getifIndexByWanName(const char *name);

//#ifdef _CWMP_MIB_ /*jiunming, for cwmp-tr069*/
#ifdef PORT_FORWARD_GENERAL
int delPortForwarding( unsigned int ifindex );
int updatePortForwarding( unsigned int old_id, unsigned int new_id );
#endif
#ifdef CONFIG_USER_MINIUPNPD
int delDynamicPortForwarding( unsigned int ifindex );
#endif

#ifdef _CWMP_MIB_ /*jiunming, for cwmp-tr069*/
unsigned int findMaxConDevInstNum(MEDIA_TYPE_T mType);
unsigned int findConDevInstNumByPVC(unsigned char vpi, unsigned short vci);
unsigned int findMaxPPPConInstNum(MEDIA_TYPE_T mType, unsigned int condev_inst);
unsigned int findMaxIPConInstNum(MEDIA_TYPE_T mType, unsigned int condev_inst);
/*start use_fun_call_for_wan_instnum*/
int resetWanInstNum(MIB_CE_ATM_VC_Tp entry);
int updateWanInstNum(MIB_CE_ATM_VC_Tp entry);
#define dumpWanInstNum(p, s) do{}while(0)
/*end use_fun_call_for_wan_instnum*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
int delDhcpcOption( unsigned int ifindex );
unsigned int findMaxDHCPOptionInstNum( unsigned int usedFor, unsigned int dhcpConSPInstNum);
int getDHCPOptionByOptInstNum( unsigned int dhcpOptNum, unsigned int dhcpSPNum, unsigned int usedFor, MIB_CE_DHCP_OPTION_T *p, unsigned int *id );
int getDHCPClientOptionByOptInstNum( unsigned int dhcpOptNum, unsigned int ifIndex, unsigned int usedFor, MIB_CE_DHCP_OPTION_T *p, unsigned int *id );
unsigned int findMaxDHCPClientOptionInstNum(int usedFor, unsigned int ifIndex);
unsigned int findDHCPOptionNum(int usedFor, unsigned int ifIndex);
unsigned int findMaxDHCPReqOptionOrder(unsigned int ifIndex);
unsigned int findMaxDHCPConSPInsNum(void );
unsigned int findMaxDHCPConSPOrder(void );
int getDHCPConSPByInstNum( unsigned int dhcpspNum,  DHCPS_SERVING_POOL_T *p, unsigned int *id );
void clearOptTbl(unsigned int instnum);
unsigned int getSPDHCPOptEntryNum(unsigned int usedFor, unsigned int instnum);
int getSPDHCPRsvOptEntryByCode(unsigned int instnum, unsigned char optCode, MIB_CE_DHCP_OPTION_T *optEntry ,int *id);
void initSPDHCPOptEntry(DHCPS_SERVING_POOL_T *p);
#endif

MIB_CE_ATM_VC_T *getATMVCByInstNum( unsigned int devnum, unsigned int ipnum, unsigned int pppnum, MIB_CE_ATM_VC_T *p, unsigned int *chainid );
int startCWMP(void);
int set_endpoint(char *newurl, char *acsurl); //star: remove "http://" from acs url string
int set_urlport(char *url, char *newurl, unsigned int *port);
void storeOldACS(void);
int getOldACS(char *acsurl);
int restart_cwmp_acl(void);

#ifdef CONFIG_CTC_E8_CLIENT_LIMIT
int rtk_mwband_set(void);
#ifdef CONFIG_COMMON_RT_API
int rtk_fc_accesswanlimit_set(void);
#endif
#endif

#ifdef _PRMT_X_CT_COM_PORTALMNT_
#define TR069_FILE_PORTALMNT  "/proc/fp_tr069"
void setPortalMNT(void);
#endif
#endif //_CWMP_MIB_

#ifdef CONFIG_CWMP_TR181_SUPPORT
typedef struct TR181_Interface_Stack
{
	unsigned int Bridging_Bridge_instnum;
	unsigned int Bridging_Bridge_Port_instnum;
	unsigned int ATM_Link_instnum;
	unsigned int PTM_Link_instnum;
	unsigned int Ethernet_Interface_instnum;
	unsigned int Optical_Interface_instnum;
	unsigned int Ethernet_Link_instnum;
	unsigned int Ethernet_VLANTermination_instnum;
	unsigned int PPP_Interface_instnum;
	unsigned int IP_Interface_instnum;
} TR181_INTERFACE_STACK_T;

#ifdef CONFIG_TELMEX_DEV
unsigned int find_valid_BridgingBridge_instBit(void);
unsigned int find_valid_BridgingBridgePort_instBit(unsigned int br_instnum);
unsigned int find_valid_BridgingBridgeVLAN_instBit(unsigned int br_instnum);
unsigned int find_valid_BridgingBridgeVLANPort_instBit(unsigned int br_instnum);
unsigned int find_valid_EthernetLink_instBit(void);
unsigned int find_valid_EthernetVLANTermination_instBit(void);
unsigned int find_valid_PPP_Interface_instBit(void);
unsigned int find_valid_IP_Interface_instBit(void);
#endif

unsigned int find_max_BridgingBridge_instNum(void);
unsigned int find_max_BridgingBridgePort_instNum(unsigned int br_instnum);
unsigned int find_max_BridgingBridgeVLAN_instNum(unsigned int br_instnum);
unsigned int find_max_BridgingBridgeVLANPort_instNum(unsigned int br_instnum);
unsigned int find_max_ATMLink_instNum(void);
unsigned int find_max_PTMLink_instNum(void);
unsigned int find_max_EthernetLink_instNum(void);
unsigned int find_max_EthernetVLANTermination_instNum(void);
unsigned int find_max_PPP_Interface_instNum(void);
unsigned int find_max_IP_Interface_instNum(void);
unsigned int find_max_IP_Interface_IPv6Address_instNum(unsigned int ip_instnum);
unsigned int find_max_IP_Interface_IPv6Prefix_instNum(unsigned int ip_instnum);

int get_ATM_Link_EntryByInstNum(unsigned int instnum, MIB_TR181_ATM_LINK_T *p, int *chainid);
int get_PTM_Link_EntryByInstNum(unsigned int instnum, MIB_TR181_PTM_LINK_T *p, int *chainid);
int get_Ethernet_Interface_EntryByInstNum(unsigned int instnum, MIB_TR181_ETHERNET_INTERFACE_T *p, int *chainid);
int get_Bridging_Bridge_EntryByInstNum(unsigned int bridge_instnum, MIB_TR181_BRIDGING_BRIDGE_T *p, int *chainid);
int get_BridgingBridgePort_EntryByInstNum(unsigned int bridge_instnum, unsigned int port_instnum, MIB_TR181_BRIDGING_BRIDGE_PORT_T *p, int *chainid);
int get_Ether_Link_EntryByInstNum(unsigned int instnum, MIB_TR181_ETHERNET_LINK_T *p, int *chainid);
int get_Ethernet_VLANTermination_EntryByInstNum(unsigned int instnum, MIB_TR181_ETHERNET_VLANTERMINATION_T *p, int *chainid);
int get_PPP_Interface_EntryByInstNum(unsigned int instnum, MIB_TR181_PPP_INTERFACE_T *p, int *chainid);
int get_IP_Interface_EntryByInstNum(unsigned int instnum, MIB_TR181_IP_INTERFACE_T *p, int *chainid);
int get_IP_Interface_IPv6Address_EntryByInstNum(unsigned int ip_instnum, unsigned int instnum, MIB_TR181_IP_INTF_IPV6ADDR_T *p, int *chainid);
int get_IP_Interface_IPv6Address_EntryByAddr(unsigned int ip_instnum, char *addr, MIB_TR181_IP_INTF_IPV6ADDR_T *p, int *chainid);
int get_IP_Interface_IPv6Prefix_EntryByInstNum(unsigned int ip_instnum, unsigned int instnum, MIB_TR181_IP_INTF_IPV6PREFIX_T *p, int *chainid);
int get_IP_Interface_IPv6Prefix_EntryByPrefix(unsigned int ip_instnum, char *prefix, MIB_TR181_IP_INTF_IPV6PREFIX_T *p, int *chainid);

int rtk_tr181_init_interfacestack_table(int table_id);
int remove_BridgingBridge_related_Tables_ByInstNum(unsigned int Instnum);

int rtk_tr181_get_wan_interfacestack_by_entry(MIB_CE_ATM_VC_T *pWAN_Entry, TR181_INTERFACE_STACK_T *pInterfaceStack);
int rtk_tr181_update_wan_interface_entry_list(int action, MIB_CE_ATM_VC_Tp pOld_Entry, MIB_CE_ATM_VC_Tp pNew_Entry);
int rtk_tr181_get_wan_entry_by_IP_instnum(MIB_CE_ATM_VC_T *pWAN_Entry, int IP_Interface_instnum);
#endif

int restart_ddns(void);
int getDisplayWanName(MIB_CE_ATM_VC_T *pEntry, char* name);
int getWanEntrybyWanName(const char *wname, MIB_CE_ATM_VC_T *pEntry, int *entry_index);
int getWanEntrybyIfIndex(unsigned int ifIndex, MIB_CE_ATM_VC_T *pEntry, int *entry_index);
int getWanEntrybyindex(MIB_CE_ATM_VC_T *pEntry, unsigned int ifIndex);
int getWanEntrybyMedia(MIB_CE_ATM_VC_T *pEntry, MEDIA_TYPE_T mType);
int getWanEntrybyIfname(const char *pIfname, MIB_CE_ATM_VC_T *pEntry, int *entry_index);
#ifdef CONFIG_SUPPORT_AUTO_DIAG
int getSimuWanEntrybyIfname(const char *pIfname, MIB_CE_ATM_VC_T *pEntry, int *entry_index);
#endif
unsigned int getWanIfMapbyMedia(MEDIA_TYPE_T mType);
int isValidMedia(unsigned int ifIndex);
unsigned int if_find_index(int cmode, unsigned int map);

int setWanName(char *str, int applicationtype);
int generateWanName(MIB_CE_ATM_VC_T * entry, char *wanname);
int getWanName(MIB_CE_ATM_VC_T * pEntry, char *name);

int create_icmp_socket(void);
int in_cksum(unsigned short *buf, int sz);
int utilping(char *str);
int ServerSelectionDiagnostics(char *pHostList, char *pInterface, int numRepetition, int timeout, char *pFastestHost, int *AverageResponseTime);
int get_wan_gateway(unsigned int ifIndex, struct in_addr *gateway);
int defaultGWAddr(char *gwaddr);
int pdnsAddr(char *dnsaddr);
int getATMEntrybyVPIVCIUsrPswd(MIB_CE_ATM_VC_T* Entry, int vpi, int vci, char* username, char* password, char* ifname);

int getNameServers(char *buf);
int setNameServers(char *buf);
#ifdef ACCOUNT_CONFIG
int getAccPriv(char *user);
#endif
int isValidIpAddr(char *ipAddr);
int isValidHostID(char *ip, char *mask);
int isValidNetmask(char *mask, int checkbyte);
int isSameSubnet(char *ipAddr1, char *ipAddr2, char *mask);
int isValidMacString(char *MacStr);
int isValidMacAddr(unsigned char *macAddr);
#ifdef CONFIG_GPON_FEATURE
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_TR142_MODULE)
int get_wan_flowid(void *parg);
#endif
#ifdef CONFIG_RTK_HOST_SPEEDUP
int set_speedup_usflow(MIB_CE_ATM_VC_T *pEntry);
int clear_speedup_usflow(void);
#endif
#endif


typedef struct pppoe_s_info {
	unsigned int	uifno;			/* index of device */
	unsigned short session;				/* Identifier for our session */
	struct sockaddr_ll remote;
} PPPOE_SESSION_INFO;

struct ppp_policy_route_info {
	u_char	if_name[IFNAMSIZ];
	u_long	hisip;
	u_long	myip;
	u_long	primary_dns;
	u_long	second_dns;
};
extern int set_ppp_source_route(struct ppp_policy_route_info *ppp_info);
extern void update_wan_routing(char *ifname);

int restart_ethernet(int instnum);
#ifdef ELAN_LINK_MODE
int setupLinkMode(void);
#endif

#ifdef _PRMT_TR143_
struct TR143_UDPEchoConfig
{
	unsigned char	Enable;
	unsigned char	EchoPlusEnabled;
	unsigned short	UDPPort;
	unsigned char	Interface[IFNAMSIZ];
	char	SourceIPAddress[INET6_ADDRSTRLEN];
};
int clearWanUDPEchoItf( void );
int setWanUDPEchoItf( unsigned int ifindex );
int getWanUDPEchoItf( unsigned int *pifindex);
void UDPEchoConfigSave(struct TR143_UDPEchoConfig *p);
int UDPEchoConfigStart( struct TR143_UDPEchoConfig *p );
int UDPEchoConfigStop( struct TR143_UDPEchoConfig *p );
#endif //_PRMT_TR143_

/*ping_zhang:20081217 START:patch from telefonica branch to support WT-107*/
#if 1 //def _PRMT_WT107_
enum eTStatus
{
	eTStatusDisabled,
	eTStatusUnsynchronized,
	eTStatusSynchronized,
	eTStatusErrorFailed,/*Error_FailedToSynchronize*/
	eTStatusError
};
#endif
/*ping_zhang:20081217 END*/

//void pppoe_session_update(void *p);
//void save_pppoe_sessionid(void *p);
// Added by Magician for external use
int deleteConnection(int configAll, MIB_CE_ATM_VC_Tp pEntry);
int startWan(int configAll, MIB_CE_ATM_VC_Tp pEntry, int isBoot);
#define CONFIGONE	0
#define CONFIGALL 	1
#define CONFIGCWMP 2

enum{
	NOT_BOOT=0,IS_BOOT
};

// Mason Yu
#define WEB_REDIRECT_BY_MAC_INTERVAL	12*60*60	/* Polling DHCP release table every 12 hours */
#ifdef WEB_REDIRECT_BY_MAC
extern struct callout_s landingPage_ch;
void clearLandingPageRule(void *dummy);
#endif
#ifdef AUTO_DETECT_DMZ
extern struct callout_s autoDMZ_ch;
#endif

// Mason Yu. Timer for auto search PVC
#ifdef CONFIG_DEV_xDSL
#if defined(AUTO_PVC_SEARCH_TR068_OAMPING) || defined(AUTO_PVC_SEARCH_PURE_OAMPING) || defined(AUTO_PVC_SEARCH_AUTOHUNT)
int startAutoSearchPVC(void);
#endif
#endif
int pppdbg_get(int unit);
#ifdef CONFIG_ATM_CLIP
void sendAtmInARPRep(unsigned char update);
#endif
#if defined(CONFIG_TELMEX_DEV)
#ifdef WLAN_SUPPORT
int needModifyWlanInterface(void);
#endif
#ifdef CONFIG_USER_DHCP_SERVER
int needModifyDhcpMode(void);
#endif
int isOnlyOneBridgeWan(void);
int checkIfCloseFunc(void);
#endif

void poll_autoDMZ(void *dummy);
void restartWAN(int configAll, MIB_CE_ATM_VC_Tp pEntry);
void resolveServiceDependency(unsigned int idx);
#ifdef IP_PASSTHROUGH
void restartIPPT(struct ippt_para para);
#endif
#ifdef DOS_SUPPORT
void setup_dos_protection(void);
#if defined(CONFIG_USER_DOS_ICMP_REDIRECTION)
int rtk_set_dos_icmpRedirect_rule(void);
#endif
#endif
#ifdef CONFIG_USER_RTK_VOIP
void voip_setup_iptable(int is_dmz);
#endif
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
void voip_set_fc_1p_tag(void);
#endif

#ifdef COMMIT_IMMEDIATELY
void Commit(void);
#endif

void setup_ipforwarding(int enable);
void ppp_if6up(char *ifname);
char *ifGetName(int, char *, unsigned int);
int isAnyPPPoEWan(void);
int getWANEntryIdxFromIfindex(unsigned int ifindex);
int getIfIndexByName(char *pIfname);
int getNameByIP(char *ip, char *buffer, unsigned int len);
int rtk_wan_get_mac_by_ifname(const char *pIfname, char *pAddr);
#ifdef CONFIG_USER_MAC_CLONE
int get_clone_mac_by_ip(char *ip, char *clone_mac);
#endif
/* WAPI */
#define WAPI_TMP_CERT "/var/tmp/tmp.cert"
#define WAPI_AP_CERT "/var/myca/ap.cert"
#define WAPI_CA_CERT "/var/myca/CA.cert"
#define WAPI_CA4AP_CERT "/var/myca/ca4ap.cert"
#define WAPI_AP_CERT_SAVE "/var/config/ap.cert"
#define WAPI_CA_CERT_SAVE "/var/config/CA.cert"
#define WAPI_CA4AP_CERT_SAVE "/var/config/ca4ap.cert"
void wapi_cert_link_one(const char *name, const char *lnname);

#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN)
#ifndef CONFIG_RTL8672NIC
#define ETHWAN_PORT 0//in virtual view
#define PTMWAN_PORT 5//in virtual view
#else
#define ETHWAN_PORT 3
#endif
int init_ethwan_config(MIB_CE_ATM_VC_T *pEntry);
#endif

#ifdef CONFIG_USER_WT_146
int wt146_dbglog_get(unsigned char *ifname);
void wt146_create_wan(MIB_CE_ATM_VC_Tp pEntry, int reset_bfd_only );
void wt146_del_wan(MIB_CE_ATM_VC_Tp pEntry);
void wt146_set_default_config(MIB_CE_ATM_VC_Tp pEntry);
void wt146_copy_config(MIB_CE_ATM_VC_Tp pto, MIB_CE_ATM_VC_Tp pfrom);
#endif //CONFIG_USER_WT_146

// Magician: This function is for checking the validation of whole config file.
int checkConfigFile(const char *filename, int check_enc);

// Magician: This function can show memory usage change on the fly.
#if DEBUG_MEMORY_CHANGE
extern char last_memsize[128], last_file[32], last_func[32]; // Use to indicate last position where you put ShowMemChange().
extern int last_line;  // Use to indicate last position where you put ShowMemChange().
int ShowMemChange(char *file, char *func, int line);
#endif

#ifndef CONFIG_RTL8672NIC
void setupIPQoSflag(int flag);
#endif
#ifdef IP_PORT_FILTER
int setupIPFilter(void);
int setup_default_IPFilter(void);
#endif
#if defined(IP_PORT_FILTER) || defined(MAC_FILTER) || defined(DMZ)
int restart_IPFilter_DMZ_MACFilter(void);
#endif
void cleanAllFirewallRule(void);
#if (defined(IP_PORT_FILTER) || defined(DMZ)) && defined(CONFIG_TELMEX_DEV)
void filter_patch_for_dmz(void);
#endif
int setupFirewall(int isBoot);
#if defined(CONFIG_USER_RTK_BRIDGE_MACFILTER_SUPPORT)
void setup_br_macfilter(void);
#endif
#if defined(CONFIG_USER_RTK_NAT_ALG_PASS_THROUGH) || (defined(CONFIG_TRUE) && defined(CONFIG_IP_NF_ALG_ONOFF))
int setupAlgOnOff(void);
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
int rtk_pppoe_prohibit_sntp_trigger_dial_on_demand(MIB_CE_ATM_VC_Tp pEntry);
#endif
#ifdef TIME_ZONE
int rtk_sntp_is_bind_on_wan(MIB_CE_ATM_VC_Tp entry);
#endif

//Kevin:Check whether to enable/disable upstream ip fastpath
void UpdateIpFastpathStatus(void);

#define BIT_IS_SET(a, no)  (a & (0x1 << no))
#define BIT_XOR_SET(a, no)  (a ^= (0x1 << no))
#define AUG_DBG
#ifdef AUG_DBG
#define AUG_PRT(fmt,args...)  printf("\033[1;33;46m<%s %d %s> \033[m"fmt, __FILE__, __LINE__, __func__ , ##args)
#else
#define AUG_PRT(fmt,args...)  do{}while(0)
#endif
#ifdef NEW_PORTMAPPING

#define PMAP_DEFAULT_TBLID	252
struct pmap_s {
	int valid;
	unsigned int ifIndex;	// resv | media | ppp | vc
	unsigned int applicationtype;
	unsigned int itfGroup;
	unsigned int fgroup;
};

extern struct pmap_s pmap_list[MAX_VC_NUM];
int get_pmap_fgroup(struct pmap_s *pmap_p, int num);
int check_itfGroup(MIB_CE_ATM_VC_Tp pEntry, MIB_CE_ATM_VC_Tp pOldEntry);
int pmap_reset_ip_route(MIB_CE_ATM_VC_Tp pEntry);
void set_pmap_rule(MIB_CE_ATM_VC_Tp pEntry, int action);
int exec_portmp(void);
#ifdef CONFIG_CU
void setTmpDropBridgeDhcpPktRule(int act);
#endif
void setupnewEth2pvc(void);
#ifdef CONFIG_RTK_MAC_AUTOBINDING
void set_auto_mac_binding_map_rule(unsigned char *smac,unsigned itfIndex,int mode);
#endif
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
int isVlanMappingWAN(unsigned short vid);
void setup_vlan_map_intf(MIB_CE_PORT_BINDING_Tp pEntry, unsigned int port, int action);
void rtk_pmap_filter_unexist_wan(void);
#endif

void setup_wan_pmap_lanmember(MEDIA_TYPE_T mType, unsigned int Index);
int set_local_output_rule_for_pppwan(int del, char *ifname);

#ifdef _PRMT_X_CT_COM_WANEXT_
void set_IPForward_by_WAN_entry(MIB_CE_ATM_VC_Tp pentry);
void set_IPForward_by_WAN(int wan_idx);
void handle_IPForwardMode(int wan_idx);
void rtk_fw_set_ipforward(int addflag);
void clean_IPForward_by_WAN_entry(MIB_CE_ATM_VC_Tp pentry);
void clean_IPForward_by_WAN(int wan_idx);
#ifdef _PRMT_X_CU_EXTEND_
void handle_WANIPForwardMode(int wan_idx);
void handle_WANIPForwardMode_all();
#endif
#endif
#endif
int setup_ethernet_config(MIB_CE_ATM_VC_Tp pEntry, char *wanif);
#ifdef CONFIG_HWNAT
void setup_hwnat_eth_member(int port, int mbr, char enable);
#ifdef CONFIG_PTMWAN
void setup_hwnat_ptm_member(int port, int mbr, char enable);
#endif
#endif
#if defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN)
void addEthWANdev(MIB_CE_ATM_VC_Tp pEntry);
#endif

struct net_device_stats
{
	unsigned long long	rx_packets;		/* total packets received	*/
	unsigned long long	tx_packets;		/* total packets transmitted	*/
	unsigned long long	rx_bytes;		/* total bytes received 	*/
	unsigned long long	tx_bytes;		/* total bytes transmitted	*/
	unsigned long long	rx_errors;		/* bad packets received		*/
	unsigned long long	tx_errors;		/* packet transmit problems	*/
	unsigned long long	rx_dropped;		/* no space in linux buffers	*/
	unsigned long long	tx_dropped;		/* no space available in linux	*/
	unsigned long long	multicast;		/* multicast packets received	*/
	unsigned long long	tx_ucast_pkts;
	unsigned long long	tx_mcast_pkts;
	unsigned long long	tx_bcast_pkts;
	unsigned long	collisions;

	/* detailed rx_errors: */
	unsigned long	rx_length_errors;
	unsigned long	rx_over_errors;		/* receiver ring buff overflow	*/
	unsigned long	rx_crc_errors;		/* recved pkt with crc error	*/
	unsigned long	rx_frame_errors;	/* recv'd frame alignment error */
	unsigned long	rx_fifo_errors;		/* recv'r fifo overrun		*/
	unsigned long	rx_missed_errors;	/* receiver missed packet	*/

	/* detailed tx_errors */
	unsigned long	tx_aborted_errors;
	unsigned long	tx_carrier_errors;
	unsigned long	tx_fifo_errors;
	unsigned long	tx_heartbeat_errors;
	unsigned long	tx_window_errors;

	/* for cslip etc */
	unsigned long	rx_compressed;
	unsigned long	tx_compressed;

	/*for wlan*/
	unsigned long beacon_error;
#if defined(CONFIG_RTK_NETIF_EXTRA_STATS)
	__u64	rx_mc_bytes;	/* multicast bytes recieved */
	__u64	tx_mc_bytes;	/* multicast bytes transmitted */
	__u64	tx_mc_packets;	/* multicast packets transmitted */
	__u64	rx_bc_bytes;	/* broadcast bytes recieved */
	__u64	rx_bc_packets;	/* broadcast packets recieved */
	__u64	tx_bc_bytes;	/* broadcast bytes transmitted */
	__u64	tx_bc_packets;	/* broadcast packets transmitted */
	__u64	rx_uc_bytes;	/* unicast bytes recieved */
	__u64	rx_uc_packets;	/* unicast packets recieved */
	__u64	tx_uc_bytes;	/* unicast bytes transmitted */
	__u64	tx_uc_packets;	/* unicast packets transmitted */
#endif

};

/*
 *	Refer to linux/ethtool.h
 */
struct net_link_info
{
	unsigned long	supported;	/* Features this interface supports: ports, link modes, auto-negotiation */
	unsigned long	advertising;	/* Features this interface advertises: link modes, pause frame use, auto-negotiation */
	unsigned short	speed;		/* The forced speed, 10Mb, 100Mb, gigabit */
	unsigned char	duplex;		/* Duplex, half or full */
	unsigned char	phy_address;
	unsigned char	transceiver;	/* Which transceiver to use */
	unsigned char	autoneg;	/* Enable or disable autonegotiation */
};

#ifdef RTK_MULTI_AP_R2
struct channel_list {
	unsigned int op_class;
	unsigned int channel;
	unsigned int scan_status;
	unsigned int utilization;
	unsigned int noise;
	unsigned int neighbor_nr;
	unsigned int score;
};
struct channel_scan_results {
	unsigned int channel_nr;
	struct  	 channel_list *channels;
	unsigned int best_channel;
};
#endif

#define _PATH_PROCNET_DEV "/proc/net/dev"
#define _PATH_PROCNET_DEV_EXT "/proc/net/dev_ext"

int list_net_device_with_flags(short flags, int nr_names,
				char (* const names)[IFNAMSIZ]);
int get_net_device_stats(const char *ifname, struct net_device_stats *nds);

#ifdef CONFIG_RTK_NETIF_EXTRA_STATS
int get_net_device_ext_stats(const char *ifname, struct net_device_stats *nds);
#endif

#ifdef WLAN_SUPPORT
int get_wlan_net_device_stats(const char *ifname, struct net_device_stats *nds);
#endif
enum {
	RX_UCAST_PACKETS,
	RX_MCAST_PACKETS,
	RX_BCAST_PACKETS,
	RX_OAM_PACKETS,
	RX_JUMBO_FRAMES,
	RX_PAUSE_FRAMES,
	RX_UNKNOWNOC_FRAMES,
	RX_CRCERROR_FRAMES,
	RX_UNDERSIZE_FRAMES,
	RX_RUNT_FRAMES,
	RX_OVERSIZE_FRAMES,
	RX_JABBER_FRAMES,
	RX_DISCARDS,
	RX_STATS_FRAME_64OCT,
	RX_STATS_FRAME_65_TO_127OCT,
	RX_STATS_FRAME_128_TO_255OCT,
	RX_STATS_FRAME_256_TO_511OCT,
	RX_STATS_FRAME_512_TO_1023OCT,
	RX_STATS_FRAME_1024_TO_1518OCT,
	RX_STATS_FRAME_1519_TO_2100OCT,
	RX_STATS_FRAME_2101_TO_9200OCT,
	RX_STATS_FRAME_9201_TO_MAXOCT,
	RX_OCTETS,
	RX_OCTETS_HI,
	TX_UCAST_PACKETS,
	TX_MCAST_PACKETS,
	TX_BCAST_PACKETS,
	TX_OAM_PACKETS,
	TX_JUMBO_FRAMES,
	TX_PAUSE_FRAMES,
	TX_CRCERROR_FRAMES,
	TX_OVERSIZE_FRAMES,
	TX_SINGLECOL_FRAMES,
	TX_MULTICOL_FRAMES,
	TX_LATECOL_FRAMES,
	TX_EXCESSCOL_FRAMES,
	TX_STATS_FRAME_64OCT,
	TX_STATS_FRAME_65_TO_127OCT,
	TX_STATS_FRAME_128_TO_255OCT,
	TX_STATS_FRAME_256_TO_511OCT,
	TX_STATS_FRAME_512_TO_1023OCT,
	TX_STATS_FRAME_1024_TO_1518OCT,
	TX_STATS_FRAME_1519_TO_2100OCT,
	TX_STATS_FRAME_2101_TO_9200OCT,
	TX_STATS_FRAME_9201_TO_MAXOCT,
	TX_OCTETS,
	TX_OCTETS_HI
};
struct ethtool_stats * ethtool_gstats(const char *ifname);
int get_net_link_status(const char *ifname);
int get_net_link_info(const char *ifname, struct net_link_info *info);
int get_dev_ShapingRate(const char *ifname);
int get_dev_ifindex(const char *ifname);
int set_dev_ShapingRate(const char *ifname, int rate);
int get_dev_ShapingBurstSize(const char *ifname);
int set_dev_ShapingBurstSize(const char *ifname, unsigned int bsize);

#define WAN_MODE GetWanMode()
enum e_wan_mode {MODE_ATM = 1, MODE_Ethernet = 2, MODE_PTM = 4, MODE_BOND = 8, MODE_Wlan = 0x10};
int GetWanMode(void);
int isInterfaceMatch(unsigned int);
int isDefaultRouteWan(MIB_CE_ATM_VC_Tp pEntry);
int isDefaultRouteWanV4(MIB_CE_ATM_VC_Tp pEntry);
int isDefaultRouteWanV6(MIB_CE_ATM_VC_Tp pEntry);
unsigned int getInternetIPv4WANIfindex(void);
unsigned int getDefaultGWIPv6WANIfindex(void);
#if defined(CONFIG_USER_XDSL_SLAVE)
#define WAN_MODE_MASK (GET_MODE_ATM|GET_MODE_ETHWAN|GET_MODE_PTM|GET_MODE_BOND|GET_MODE_WLAN)
#else
#define WAN_MODE_MASK (GET_MODE_ATM|GET_MODE_ETHWAN|GET_MODE_PTM|GET_MODE_WLAN)
#endif

#ifdef CONFIG_DEV_xDSL
#define GET_MODE_ATM 0x1
#else
#define GET_MODE_ATM 0x0
#endif

#ifdef CONFIG_ETHWAN
#define GET_MODE_ETHWAN 0x2
#else
#define GET_MODE_ETHWAN 0x0
#endif

#ifdef CONFIG_PTMWAN
#define GET_MODE_PTM 0x4
#else
#define GET_MODE_PTM 0
#endif

#ifdef CONFIG_USER_XDSL_SLAVE
#define GET_MODE_BOND 0x8
#else
#define GET_MODE_BOND 0
#endif

#ifdef WLAN_WISP
#define GET_MODE_WLAN 0x10
#else
#define GET_MODE_WLAN 0
#endif

int reset_cs_to_default(int flag);

#ifdef CONFIG_USER_MINIUPNPD
int restart_upnp(int configAll, int wan_enable, int wan_IfIndex, int disable_upnp);
void do_upnp_sync(char *wan_name);
#endif

int getPrimaryLanNetAddrInfo(char* pAddr, int length);
int get_v4_interface_addr_net_info(char* pAddr, int length, char *infname);

#ifdef CONFIG_USER_SAMBA
#define SMB_CONF_FILE "/var/smb.conf"
#define SMB_PWD_FILE "/var/samba/smbpasswd"
int startSamba(void);
int stopSamba(void);
void delSambaAccessFw(void);
void setSambaAccessFw(int enable);
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
int smartHGU_Samba_Initialize(void);
#else
int addSmbPwd(char *username, char *passwd);
void initSmbPwd(void);
int updateSambaCfg(unsigned char sec_enable);
int initSamba(void);
#endif // CONFIG_YUEME
#endif // CONFIG_USER_SAMBA

void delFtpAccessFw(void);
void setFtpAccessFw(int enable);

void delTelnetAccessFw(void);
void setTelnetAccessFw(int enable);

#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_AWIFI_SUPPORT)
void rtk_wan_aWiFi_wifidog_start(char *wanIf, char *wlanIf);
int rtk_wan_aWiFi_wifidog_stop();

int rtk_wan_aWiFi_start(int ifIndex);
int rtk_wan_aWiFi_stop(int ifIndex);
#endif // CONFIG_USER_AWIFI_SUPPORT

#ifdef CONFIG_TR_064
#define TR064_STATUS GetTR064Status()
int GetTR064Status(void);
#endif

// Mason Yu. Specify IP Address
struct ddns_info {    				/* Used as argument to ddnsC() */
	int		ipversion;        		/* IPVersion . 1:IPv4, 2:IPv6, 3:IPv4 and IPv6 */
	char 	ifname[IFNAMSIZ];        /* Interface name */
};

int check_vlan_reserved(int vid);
#ifdef CONFIG_USER_VLAN_ON_LAN
int setup_VLANonLAN(int mode);
#endif
#ifdef CONFIG_USER_BRIDGE_GROUPING
int setup_bridge_grouping(int mode);
#ifdef CONFIG_00R0
void reset_VLANMapping(void);
void setup_VLANMapping(int ifidx, int enable);
int setVLANByGroupnum(int groupnum, int enable, int vlan);
#endif
void set_port_binding_rule(int enable);
int set_port_binding_mask(unsigned int *wanlist);
int rg_set_port_binding_mask(unsigned int set_wanlist);
#define MAX_PAIR 1 // max mapping pair per LAN interface(1~4)
#endif
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
struct vlan_pair {
	unsigned short vid_a;
	unsigned short vid_b;
}__PACK__;
#define VLAN_PAIR_STRUCT 1
#endif
int update_hosts(char *, struct addrinfo *);
struct addrinfo *hostname_to_ip(char *, IP_PROTOCOL);
int isHostAlive(char *hostname, IP_PROTOCOL IPVer);

#define WAN_POLICY_ROUTE_TBLID_START 0x200 //0x200~0x300
#define ITF_SOURCE_ROUTE_SIMU_START	0xb0

//LAN_PHY_PORT_LLA_ROUTE is for fe80::/64 dev eth0.x
#define IP_ROUTE_LAN_PHYPORT_TABLE 0x100 //256
#define LAN_PHY_PORT_LLA_ROUTE IP_ROUTE_LAN_PHYPORT_TABLE
#define IP_ROUTE_STATIC_TABLE 0x101      //257
#define IP_ROUTE_LAN_V6PD_TABLE 0x102    //258
#define DS_DHCPV6S_LAN_PD_ROUTE IP_ROUTE_LAN_V6PD_TABLE //DS_DHCPV6S_LAN_PD_ROUTE is for DHCPv6 Server Downstream LAN PD route
#define IP_ROUTE_LAN_V6NPT_TABLE 0x103   //259
#define DS_WAN_NPT_PD_ROUTE IP_ROUTE_LAN_V6NPT_TABLE
#define IP_ROUTE_LAN_TABLE 0x104         //260

#ifdef CONFIG_USER_MAP_E
#define PMAP_MAPE_PPP_START 0x110
#define PMAP_MAPE_NAS_START 0x120
#endif

#ifdef CONFIG_REMOTE_CONFIGD
#ifdef CONFIG_SPC
#include <unistd.h>

#define PF_SPC          AF_SPC
#define AF_SPC          28      /* Slow Protocol Channel        */
#endif
#endif

#if defined(CONFIG_E8B)||defined(_PRMT_X_TRUE_FIREWALL_) ||defined(FIREWALL_SECURITY)
typedef struct _VC_STATE_ST_ {
	char ifName[IFNAMSIZ];	//interface name
	char chMode;	//channel mode of current PVC: CHANNEL_MODE_IPOE ...
	char fstPvc;	//show if is the first configured wan interface
	char dfGW;		//show if interface with default GW
	char null;
} VC_STATE_T, *VC_STATE_Pt;

//use fw_state_t to reserve the set state of firewall grade
typedef struct _FW_GRADE_INIT_ST_{
	char linkDownInit;	//set firewall grade while link down
	char linkUpInit;	//set firewall grade while link up.
	char preFwGrade;	//previous firewall grade
	char null;
} FW_GRADE_INIT_St;

#define EBTABLES_ENABLED 1
#define EBTABLES_DISABLED 0
#define MAC_FILTER_BRIDGE_RULES 16
#define MAC_FILTER_ROUTER_RULES 16
#endif

#ifdef CONFIG_NETFILTER_XT_MATCH_PSD
int setup_psd(void);    //port scan
#endif

#ifdef DOS_SUPPORT
#define DOS_ENABLE		0x01
#define SYSFLOODSYN		0x02
#define SYSFLOODFIN		0x04
#define SYSFLOODUDP		0x08
#define SYSFLOODICMP	0x10
#define IPFLOODSYN		0x20
#define IPFLOODFIN		0x40
#define IPFLOODUDP		0x80
#define IPFLOODICMP		0x100
#define TCPUDPPORTSCAN	0x200
#define ICMPSMURFENABLED	0x400
#define IPLANDENABLED		0x800
#define IPSPOOFENABLED		0x1000
#define IPTEARDROPENABLED	0x2000
#define PINGOFDEATHENABLED	0x4000
#define TCPSCANENABLED		0x8000
#define TCPSynWithDataEnabled	0x10000
#define UDPBombEnabled			0x20000
#define UDPEchoChargenEnabled	0x40000
#define ICMPFRAGMENT		0x80000
#define TCPFRAGOFFMIN		0x100000
#define TCPHDRMIN			0x200000
#define sourceIPblock		0x400000
#define TCPUDPPORTSCANSENSI	0x800000
#define WINNUKE				0x1000000
#define ICMPREDIRECT		0x2000000
#define FRAGMENTHEADER		0x4000000
#define ARPATTACK			0x8000000
#endif

#ifdef CONFIG_E8B
#define CMCC_FIREWARE_LEVEL_HIGH	IPTEARDROPENABLED|IPFLOODSYN|IPFLOODFIN|IPFLOODUDP|IPFLOODICMP|PINGOFDEATHENABLED|sourceIPblock
#define CMCC_FIREWARE_LEVEL_MIDDLE	TCPSCANENABLED|TCPSynWithDataEnabled|TCPUDPPORTSCAN|SYSFLOODSYN|SYSFLOODFIN|SYSFLOODUDP|SYSFLOODICMP|ICMPSMURFENABLED
#define CMCC_FIREWARE_LEVEL_LOW		IPLANDENABLED|IPSPOOFENABLED|UDPBombEnabled|UDPEchoChargenEnabled

#define DOS_ENABLE_ALL	(DOS_ENABLE|SYSFLOODSYN|SYSFLOODFIN|SYSFLOODUDP|SYSFLOODICMP|IPFLOODSYN|\
						IPFLOODFIN|IPFLOODUDP|IPFLOODICMP|TCPUDPPORTSCAN|ICMPSMURFENABLED|\
						IPLANDENABLED|IPSPOOFENABLED|IPTEARDROPENABLED|PINGOFDEATHENABLED|\
						TCPSCANENABLED|TCPSynWithDataEnabled|UDPBombEnabled|UDPEchoChargenEnabled|\
						sourceIPblock|ICMPFRAGMENT|TCPFRAGOFFMIN|TCPHDRMIN)
#endif

#ifdef _PRMT_X_CT_COM_ALARM_MONITOR_
int set_ctcom_alarm(unsigned int alarm_num);
int clear_ctcom_alarm(unsigned int alarm_num);
int init_alarm_numbers(void);
int find_alarm_num(unsigned int alarm_num);
int get_alarm_by_alarm_num(unsigned int alarm_num, CWMP_CT_ALARM_T *entry, int *idx);
#endif

#ifdef CONFIG_USB_SUPPORT
#define USB_SUPPORT
struct usb_info{
                char disk_type[64];
                char disk_status[64];
                char disk_fs[64];
                unsigned long disk_used;
                unsigned long disk_available;
                char disk_mounton[256];
};
void getUSBDeviceInfo(int *disk_sum,struct usb_info* disk1,struct usb_info *disk2);
#endif// end of CONFIG_USB_SUPPORT

int check_user_is_registered(void);
/************************************************
* Propose: checkProcess()
*    Check process exist or not
* Parameter:
*	char* pidfile      pid file path
* Return:
*      1  :  exist
*      0  :  not exist
*      -1: parameter error
* Author:
*     Alan
*************************************************/
int checkProcess(char* pname);

#define WAIT_INFINITE 0
/************************************************
* Propose: waitProcessTerminate()
*    wait process until process does not exist
* Parameter:
*	char* pidfile                    pid file path
*     unsigned int timeout        0: wait infinite, otherise wait timeout miliseconds(mutiple of 10)
* Return:
*      0  : success
*      -1: parameter error
* Author:
*     Alan
*************************************************/
int waitProcessTerminate(char* pidfile, unsigned int timeout);

/************************************************
* Propose: getMacAddr
*
*    get MAC address
*
* Parameter:
*	char* ifname                     interface name
*     unsigned char* macaddr      mac addr
* Return:
*     -1 : fail
*       0 : success
* Author:
*     Alan
*************************************************/
int getMacAddr(char* ifname, unsigned char* macaddr);
/************************************************
* Propose: getMacAddr from IPv4 address
*
*    get MAC address from IPv4 address
*
* Parameter:
*	in_addr* ip address            ipv4 ip address
*     unsigned char* macaddr      mac addr
* Return:
*     -1 : fail
*       0 : success
* Author:
*     Iulian
*************************************************/
int get_mac_by_ipaddr(struct in_addr *comp_addr, unsigned char mac[MAC_ADDR_LEN]);

/************************************************
* Propose: get netmask CIDR (0-24) from ipv4 address
*
*    get netmask CIDR from ipv4 address
*
* Parameter:
*	in_addr* ip address            ipv4 ip address
* Return:
*       cidr number
* Author:
*     Iulian
*************************************************/
unsigned short netmask_to_cidr(char* ipAddress);
unsigned int cidr_to_netmask(unsigned char bits);

#ifdef CONFIG_RTK_HOST_SPEEDUP
int add_host_speedup(struct in_addr srcip, unsigned short sport, struct in_addr dstip, unsigned short dport);
int del_host_speedup(struct in_addr srcip, unsigned short sport, struct in_addr dstip, unsigned short dport);
int  check_speedup_history_result(unsigned int *sample_values, int sample_num);
void delete_speedup_history_result(void);
int check_speedup_need_tunedown(int speed);
#endif /*CONFIG_RTK_HOST_SPEEDUP*/

#ifdef CONFIG_00R0
/* bootloader parameters, Iulian Wu*/
#define BOOTLOADER_UBOOT_HEADER "ver"
#define BOOTLOADER_UBOOT_CRC "bootloader_crc"
#define BOOTLOADER_SW_ACTIVE 	"sw_active"
#define BOOTLOADER_SW_VERSION 	"sw_version"
#define BOOTLOADER_SW_CRC 		"sw_crc"
enum
{
	NTP_EXT_ITF_PRI_LOW=0,
	NTP_EXT_ITF_PRI_MID ,
	NTP_EXT_ITF_PRI_HIGH
};

//void reSync_reStartVoIP();
int checkAndModifyWanServiceType(int applicationtype, int change_type, int index);
#endif
void reSync_reStartVoIP(void);

int check_vlan_conflict(MIB_CE_ATM_VC_T *pEntry, int idx, char *err_msg);
void compact_reqoption_order(unsigned int ifIndex);

#define DEFAULT_STATE_FILE "/var/run/udhcpc.state"


/* shell type for config_xmlconfig.sh
 * Added by Davian
 */
#ifdef CONFIG_USER_XMLCONFIG
//const char *shell_name;
#endif

#ifdef CONFIG_USER_XML_STRING_ENCRYPTION
int rtk_xmlfile_str_encrypt (const char *de_str, char *en_str);
int rtk_xmlfile_str_decrypt (const char *en_str, char *de_str);
#endif

int rtk_string_to_hex(const char *string, unsigned char *key, int len);
int string_to_short(const char *string, unsigned short *key, int len);
int string_to_integer(const char *string, unsigned int *key, int len);
int string_to_dec(char *string, int *val);
int mib_to_string_ex(char *string, const void *mib, TYPE_T type, int size, int encrypt);
int mib_to_string(char *string, const void *mib, TYPE_T type, int size);
int string_to_mib_ex(void *mib, const char *string, TYPE_T type, int size, int decrypt);
int string_to_mib(void *mib, const char *string, TYPE_T type, int size);
int _load_xml_file(const char *loadfile, CONFIG_DATA_T cnf_type, unsigned char flag, CONFIG_MIB_T action);
int _save_xml_file(const char *savefile, CONFIG_DATA_T cnf_type, unsigned char flag, CONFIG_MIB_T range);
void print_chain_member(FILE *fp, char *format_str, mib_chain_member_entry_T * desc, void *addr, int depth, int encrypt);

int before_upload(const char *fname);
int after_download(const char *fname);

#define CWMP_START_WAN_LIST "/var/cwmp_start_wan_list"

#ifdef CONFIG_UNI_CAPABILITY
#define PROC_UNI_CAPABILITY "/proc/realtek/uni_capability"
void setupUniPortCapability(void);
int getUniPortCapability(int logPortId);
typedef enum
{
	UNI_PORT_NONE = 0,
	UNI_PORT_FE,
	UNI_PORT_GE,
	UNI_PORT_2P5GE,
	UNI_PORT_5GE,
	UNI_PORT_10GE,
	UNI_PORT_25GE,
	UNI_PORT_40GE
} UniPortCapabilityType_t;
#endif

#if defined(CONFIG_GPON_FEATURE)|| defined(CONFIG_EPON_FEATURE)
int set_pon_led_state(int led_mode);
int set_los_led_state(int led_mode);
int isDevInPonMode(void);
#endif

#ifdef CONFIG_E8B
void get_poninfo(int s,double *buffer);
#endif
#ifdef CONFIG_USER_BEHAVIOR_ANALYSIS
void setup_behavior_analysis(void);
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
int rtk_wan_vpn_l2tp_attach_check_dip(unsigned char *tunnelName,unsigned int ipv4_addr1,unsigned int ipv4_addr2);
int rtk_wan_vpn_l2tp_attach_check_smac(unsigned char *tunnelName, unsigned char* sMAC_addr);
int rtk_wan_vpn_l2tp_attach_check_url(unsigned char *tunnelName,char *url);
int rtk_wan_vpn_l2tp_attach_delete_dip(unsigned char *tunnelName,unsigned int ipv4_addr1,unsigned int ipv4_addr2);
int rtk_wan_vpn_l2tp_attach_delete_smac(unsigned char *tunnelName, unsigned char* sMAC_addr);
int rtk_wan_vpn_l2tp_attach_delete_url(unsigned char *tunnelName,char *url);
int NF_Del_L2TP_Dynamic_URL_Route(char *tunnelName, int routeIdx, int weight, char *url, struct in_addr addr);
int NF_Set_L2TP_Dynamic_URL_Route(char *tunnelName, int routeIdx, int weight, char *url, struct in_addr addr);
int NF_Flush_L2TP_Dynamic_URL_Route(unsigned char *tunnelName);
int NF_Set_L2TP_Policy_Route(unsigned char *tunnelName);
int NF_Flush_L2TP_Route(unsigned char *tunnelName);
#ifdef VPN_MULTIPLE_RULES_BY_FILE
int rtk_wan_vpn_l2tp_get_route_idx_with_template(unsigned char *tunnelName, unsigned int rule_mode, char *match_rule);
int NF_Set_L2TP_Policy_Route_by_file(unsigned char *tunnelName);
#endif
#endif

#ifdef YUEME_4_0_SPEC
int rtk_wan_vpn_add_bind_wan_policy_route_rule(int type, void *entry);
int rtk_wan_vpn_remove_bind_wan_policy_route_rule(int type, void *entry);
#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
int rtk_wan_vpn_pptp_attach_check_dip(unsigned char *tunnelName,unsigned int ipv4_addr1,unsigned int ipv4_addr2);
int rtk_wan_vpn_pptp_attach_check_smac(unsigned char *tunnelName, unsigned char* sMAC_addr);
int rtk_wan_vpn_pptp_attach_check_url(unsigned char *tunnelName,char *url);
int rtk_wan_vpn_pptp_attach_delete_dip(unsigned char *tunnelName,unsigned int ipv4_addr1,unsigned int ipv4_addr2);
int rtk_wan_vpn_pptp_attach_delete_smac(unsigned char *tunnelName, unsigned char* sMAC_addr);
int rtk_wan_vpn_pptp_attach_delete_url(unsigned char *tunnelName,char *url);
int NF_Del_PPTP_Dynamic_URL_Route(char *tunnelName, int routeIdx, int weight, char *url, struct in_addr addr);
int NF_Set_PPTP_Dynamic_URL_Route(char *tunnelName, int routeIdx, int weight, char *url, struct in_addr addr);
int NF_Flush_PPTP_Dynamic_URL_Route(unsigned char *tunnelName);
int NF_Set_PPTP_Policy_Route(unsigned char *tunnelName);
int NF_Flush_PPTP_Route(unsigned char *tunnelName);
#ifdef VPN_MULTIPLE_RULES_BY_FILE
int rtk_wan_vpn_pptp_get_route_idx_with_template(unsigned char *tunnelName, unsigned int rule_mode, char *match_rule);
int NF_Set_PPTP_Policy_Route_by_file(unsigned char *tunnelName);
#endif
#endif

#if defined(CONFIG_EPON_FEATURE)
int epon_getAuthState(int llid);
long epon_getctcAuthSuccTime(int llid);
int getEponONUState(unsigned int llidx);
int rtk_pon_getEponllidEntryNum(void);
int config_oam_vlancfg(void);
void rtk_pon_sync_epon_llid_entry(void);
#ifdef CONFIG_USER_CUMANAGEDEAMON
void show_epon_status(char * xString);
#endif
#endif
int getWanStatus(struct wstatus_info *sEntry, int max);
#ifdef CONFIG_GPON_FEATURE
int checkOMCImib(char mode);
void checkOMCI_startup(void);
void restartOMCI(void);
void restartOMCIsettings(void);
int getGponONUState(void);
int rtk_wrapper_gpon_deactivate(void);
int rtk_wrapper_gpon_password_set(char *password);
int rtk_wrapper_gpon_activate(void);
#ifdef CONFIG_USER_CUMANAGEDEAMON
void show_gpon_status(char * xString);
#endif
#endif
int lock_file_by_flock(const char *filename, int wait);
int unlock_file_by_flock(int lockfd);

struct v_pair {
	unsigned short vid_a;
	unsigned short vid_b;
};

int isIGMPSnoopingEnabled(void);
int isMLDSnoopingEnabled(void);
int isIgmproxyEnabled(void);
#ifdef CONFIG_USER_MLDPROXY
int isMLDProxyEnabled(void);
#endif
void checkIGMPMLDProxySnooping(int isIGMPenable, int isMLDenable, int isIGMPProxyEnable, int isMLDProxyEnable);

#ifdef REMOTE_HTTP_ACCESS_AUTO_TIMEOUT
extern time_t remote_user_auto_logout_time;
void rtk_fw_update_rmtacc_auto_logout_time(void);
#endif

#ifdef CONFIG_TRUE
void rtk_fw_sync_from_wanAccess_to_rmtacc(void);
void rtk_fw_sync_from_rmtacc_to_wanAccess(void);
#endif

int rtk_chk_is_local_client(int fd);
#if defined(CONFIG_LUNA) && defined(CONFIG_RTL8686_SWITCH)
void set_port_powerdown_state(int port, unsigned int state);
void set_all_lan_port_powerdown_state(unsigned int state);
void set_all_port_powerdown_state(unsigned int state);
#endif

#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
#define UP_LAN_INTF_RATE_LIMIT 				"up_lan_intf_rate_limit"
#define UP_IP_RATE_LIMIT 					"up_ip_rate_limit"
#define UP_MAC_RATE_LIMIT 					"up_mac_rate_limit"
#define UP_VLAN_RATE_LIMIT 					"up_vlan_rate_limit"
#define DOWN_LAN_INTF_RATE_LIMIT 			"down_lan_intf_rate_limit"
#define DOWN_IP_RATE_LIMIT 					"down_ip_rate_limit"
#define DOWN_MAC_RATE_LIMIT 				"down_mac_rate_limit"
#define DOWN_VLAN_RATE_LIMIT 				"down_vlan_rate_limit"

typedef enum { UPDATE_WLAN_QOS = 1, FLUSH_WLAN_QOS } WLAN_QOS_OPERATION_T;
#endif

#if defined(PORT_FORWARD_GENERAL) || defined(DMZ) || defined(VIRTUAL_SERVER_SUPPORT)
int cleanOneEntry_NATLB_rule_dynamic_link(MIB_CE_ATM_VC_Tp pEntry, int mode);
#ifdef NAT_LOOPBACK
int set_ppp_NATLB(int fd, char *ifname, char *myip);
int del_ppp_NATLB(FILE *fp, char *ifname);
int set_dhcp_NATLB(int fh, MIB_CE_ATM_VC_Tp pEntry);
int addALLEntry_NATLB_rule_dynamic_link(char *dstIP, int mode);
int add_portfw_dynamic_NATLB_chain(void);
#ifdef DMZ
int iptable_dmz_natLB(int del, char *dstIP);
#endif
#endif
#endif
#ifdef CONFIG_TR142_MODULE
void set_wan_ponmac_qos_queue_num(void);
int set_dot1p_value_byCF(void);
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
#define TR142_WAN_INFO "/proc/rtk_tr142/wan_info"
int rtk_tr142_sync_wan_info(MIB_CE_ATM_VC_Tp pEntry, int action);
#endif
#endif

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
#ifdef CONFIG_USER_IP_QOS
#if defined(CONFIG_LUNA_G3_SERIES)
#define MAX_GPON_RATE 32767999		//10G
#elif defined(CONFIG_RTL9600_SERIES)
#define MAX_GPON_RATE 131071		//1G
#else
#define MAX_GPON_RATE 262143		//2.5G
#endif
void setup_pon_queues(void);
void clear_pon_queues(void);
#endif
#endif

int get_mem_total(void);
int get_mem_free(void);
int get_cpu_usage(void);
int get_flash_writable(void);

extern int rtk_env_get(const char *name, char *buf, unsigned int buflen);
extern int rtk_env_set(const char *name, const char *value);

unsigned char getOpMode(void);
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
#define RTK_CLEAR_BOA_LOGIN_INFO_FILE "/tmp/clear_login_info"
int reStartSystem(void);
int cleanSystem(void);
int startBridge(void);
#ifdef RTK_WEB_HTTP_SERVER_BIND_DEV
#define RTK_TRIGGLE_BOA_REBIND_SOCK_FILE	 "/tmp/triggle_boa_rebind_sock"
#endif
#endif

void strtolower(char *str, int len);
#ifdef CONFIG_RTK_DNS_TRAP
int create_hosts_file(char* domain_name);
#endif

#ifdef WLAN_SUPPORT
void translate_control_code(char *buffer);
#endif
int setup_wan_port(void);
#if ! defined(_LINUX_2_6_) && defined(CONFIG_RTL_MULTI_WAN)
int initWANMode(void);
#endif
void rtl8670_AspInit(void);
int cleanALLEntry_NATLB_rule_dynamic_link(int mode);
int get_mem_usage(void);
pid_t findPidByName( char* pidName);
int isFileExist(char *file_name);
int if_readlist_proc(char *target, char *key, char *exclude);
int getQosEnable(void);
int getQosRuleNum(void);
int kill_by_pidfile(const char *pidfile, int sig);
int kill_by_pidfile_new(const char *pidfile, int sig);
int start_process_check_pidfile(const char *pidfile);
void setup_NAT(MIB_CE_ATM_VC_Tp pEntry, int set);
int getapplicationtypeByName(char *pIfname);
void startDdnsc(struct ddns_info tinfo);
void stopDdnsc(struct ddns_info tinfo);
#ifdef _TR111_STUN_
int addCWMPSTUNFirewall(void);
int removeCWMPSTUNFirewall(void);
#endif

#ifdef CONFIG_USER_IGMPTR247
extern const char FW_TR247_MULTICAST[];
extern const char FW_US_IGMP_RATE_LIMIT[];

typedef enum
{
    TR247_MC_RESET_ALL = 0,
    TR247_MC_PORT_ADD, //port
    TR247_MC_PORT_DEL,
    TR247_MC_PROFILE_ADD,
    TR247_MC_PROFILE_DEL,
    TR247_MC_SNOOP_ENABLE_SET,     //port
    TR247_MC_IGMP_VERSION_SET,
    TR247_MC_IGMP_FUNCTION_SET,
    TR247_MC_FAST_LEAVE_SET,
    TR247_MC_US_IGMP_RATE_SET,
    TR247_MC_DYNAMIC_ACL_SET,
    TR247_MC_DYNAMIC_ACL_DEL,
    TR247_MC_STATIC_ACL_SET,
    TR247_MC_STATIC_ACL_DEL,
    TR247_MC_ROBUSTNESS_SET,
    TR247_MC_QUERIER_IPADDR_SET,
    TR247_MC_QUERY_INTERVAL_SET,
    TR247_MC_QUERY_MAX_RESPONSE_TIME_SET,
    TR247_MC_LAST_MBR_QUERY_INTERVAL_SET,
    TR247_MC_UNAUTHORIZED_JOIN_BEHAVIOR_SET,
    TR247_MC_US_IGMP_TAG_INFO_SET,
    TR247_MC_MAX_GRP_NUM_SET,  //port
    TR247_MC_MAX_BW_SET,       //port
    TR247_MC_BW_ENFORCE_SET,   //port
    TR247_MC_CURRENT_BW_GET,
    TR247_MC_JOIN_MSG_COUNT_GET,
    TR247_MC_BW_EXCEEDED_COUNT_GET,
    TR247_MC_IPV4_ACTIVE_GROUP_GET,
    TR247_MC_PROFILE_PER_PORT_ADD, //port
    TR247_MC_PROFILE_PER_PORT_DEL,
    TR247_MC_PORT_REAPPLY,
	TR247_MC_END,
}APPLY_TR247_MC_T;
int rtk_tr247_multicast_apply(APPLY_TR247_MC_T opt, unsigned short profileId, unsigned int portId, unsigned int aclTableEntryId);
#endif

/*
 * Unix domain socket with DGRAM APIs
 *
 * by Rex_Chen@realtek.com
 */

struct unixsock_entry
{
       int sock;
       int is_server;
       struct sockaddr_un server;
       struct sockaddr_un client;
};

typedef struct unixsock_entry usock_entry_t;

/* Return Unix Socket fd */
/***********************************************************************/
int sendUnixSockMsg(unsigned char waitReply,unsigned int num, ...);
int send_unix_sock_message(unsigned char waitReply, int argc, char *argv[]);
int unixsock_fd(usock_entry_t *usock);
const char *unixsock_server_path(usock_entry_t *usock);
const char *unixsock_client_path(usock_entry_t *usock);
int unixsock_open(int server, usock_entry_t *entry, const char * name);
int unixsock_close(usock_entry_t *usock);
int unixsock_send(usock_entry_t *usock, const void * buf, unsigned int len, int flags);
int unixsock_recv(usock_entry_t *usock, void * buf, unsigned int len, int flags);
int unixsock_recv_timed(usock_entry_t *usock, void * buf, unsigned int len, int flags, int timeout);

/***********************************************************************/

void formatPloamPasswordToHex(char *src, char* dest, int length);
#define GPON_PLOAM_PASSWORD_LENGTH	10
#define NGPON_PLOAM_PASSWORD_LENGTH	36

#ifdef HANDLE_DEVICE_EVENT
int rtk_sys_pushbtn_wps_action(int action, int btn_test, int diff_time);
int rtk_sys_pushbtn_wifi_action(int action, int btn_test, int diff_time);
int rtk_sys_pushbtn_reset_action(int action, int btn_test, int diff_time);
int rtk_sys_pushbtn_ledpwr_action(int action, int btn_test, int diff_time);
#endif
#ifdef CONFIG_USER_DHCPCLIENT_MODE
int getBr0IpAddr(void);
#endif

// For subr_port.c
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
int rtk_port_get_phyPort(int logicPort);
unsigned int rtk_port_get_phyPortMask(int logicPort);
int rtk_port_is_wan_logic_port(int logicPort);
int rtk_logic_port_attach_wan_num(int logicPort);
int rtk_port_is_wan_phy_port(int phyPort);
int rtk_port_get_wan_phyID(int logPortId);
int rtk_port_get_phyport_by_ifname(char *ifname);
int rtk_port_is_bind_to_wan(int logicPort);
void rtk_port_update_wan_phyMask(void);
void rtk_update_br_interface_list(void);
unsigned int rtk_port_get_default_wan_phyMask(void);
int rtk_port_get_default_wan_logicPort(void);
int rtk_port_get_logPort(int phyPort);
int rtk_port_get_logicPort_by_root_ifname(char *ifname);
#else
int rtk_port_get_wan_phyID(void);
#endif

int rtk_port_get_lan_phyID(int logPortId);
unsigned int rtk_port_get_lan_phyMask(unsigned int portmask);
unsigned int rtk_port_get_all_lan_phyMask(void);
unsigned int rtk_port_get_wan_phyMask(void);
unsigned int rtk_port_get_cpu_phyMask(void);
int rtk_port_get_lan_phyStatus(int logPortId, int *status);
int rtk_port_get_lan_link_info(int logPortId, struct net_link_info *netlink_info);
int rtk_if_name_get_by_lan_phyID(unsigned int portId, unsigned char *ifName);
int rtk_lan_phyID_get_by_if_name(unsigned char *ifName);
int rtk_port_set_wan_phyID(int phyPort);
int rtk_lan_LogID_get_by_if_name(unsigned char *ifName);
int rtk_port_set_dev_port_mapping(int port_id, char *dev_name);
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
int rtk_external_port_get_lan_phyID(int logPortId);
int rtk_external_port_set_dev_port_mapping(int port_id, char *dev_name);
int rtk_port_check_external_port(int logPortId);
#endif

typedef enum rtk_brport_state_s{
	RTK_BR_PORT_STATE_DISABLED=0,
	RTK_BR_PORT_STATE_LISTENING,
	RTK_BR_PORT_STATE_LEARNING,
	RTK_BR_PORT_STATE_FORWARDING,
	RTK_BR_PORT_STATE_BLOCKING
}rtk_brport_state_t;
const char * get_br_port_state_str(int state);
int check_ifname_br_port_state(char *ifname, int state);

int rtk_util_ip_version(const char *src) ;
int rtk_util_check_wan_vid_exist(unsigned short vid);
int rtk_util_parse_remoteport(char* source, int*start, int*end);

#ifdef CONFIG_USER_RTK_LBD
void setupLBD(void);
#endif

extern const char *ACCESSPROTOCOL[];
extern const char *CONNECION_INTERFACE[];

int rtk_bridge_get_ifname_by_portno(const char *brname, int portno, char *ifname);
int rtk_intf_get_intf_by_ip(char* ipaddr);
int rtk_util_get_ipv4addr_from_ipv6addr(char* ipaddr);
int rtk_util_get_wanaddr_from_nf_conntrack(char* remote_ipaddr, int port, char* ipaddr);


#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD) || defined(CONFIG_USER_PPTP_CLIENT)
#define MAGIC_TUNNEL_NAME	"Magic_tunnelName"
#ifdef RESTRICT_PROCESS_PERMISSION
#define VPN_WRITE_LOCK(PATH, fp) \
	do{ \
		fp=fopen(PATH,"w+"); \
		while(fp && flock(fileno(fp), LOCK_EX) == -1) \
			if(errno != EINTR){fclose(fp);chmod(fp,0666);fp=NULL;} \
	}while(0);

#define VPN_READ_LOCK(PATH, fp) \
	do{ \
		fp=fopen(PATH,"w+"); \
		while(fp && flock(fileno(fp), LOCK_SH) == -1) \
			if(errno != EINTR){fclose(fp);chmod(fp,0666);fp=NULL;} \
	}while(0);
#else
#define VPN_WRITE_LOCK(PATH, fp) \
	do{ \
		fp=fopen(PATH,"w+"); \
		while(fp && flock(fileno(fp), LOCK_EX) == -1) \
			if(errno != EINTR){fclose(fp);fp=NULL;} \
	}while(0);

#define VPN_READ_LOCK(PATH, fp) \
	do{ \
		fp=fopen(PATH,"w+"); \
		while(fp && flock(fileno(fp), LOCK_SH) == -1) \
			if(errno != EINTR){fclose(fp);fp=NULL;} \
	}while(0);
#endif
#define VPN_RW_UNLOCK(fp) \
	do{ \
		if(fp){flock(fileno(fp), LOCK_UN);fclose(fp);fp = NULL;} \
	}while(0);

#define VPN_L2TP_RULE_FILE  "/var/config/rtk_vpn_l2tp_route_rule_idx_"
#define VPN_PPTP_RULE_FILE  "/var/config/rtk_vpn_pptp_route_rule_idx_"
#ifdef VPN_MULTIPLE_RULES_BY_FILE
/* If you want to change MAX_DOMAINS_LENGTH, please remember to change
 * MAX_DOMAINS_LENGTH in dbusproxy/reg/include/prmt_apply.h and VPN_CONN_POLICY_RULE_MAX_COUNT in utility.h too.
 * New test plan will test VPN 10241 rules 20201216.
 */
#define MAX_DOMAINS_LENGTH (10241*32) //10241 rules * 32(192.168.111.111-192.168.111.255;)
#define VPN_L2TP_IPSET_RESTORE_RULE_FILE "/tmp/rtk_vpn_l2tp_ipset_restore_file_"
#define VPN_PPTP_IPSET_RESTORE_RULE_FILE "/tmp/rtk_vpn_pptp_ipset_restore_file_"
#define VPN_L2TP_IPSET_MATCH_PACKET_COUNT_FILE "/tmp/rtk_vpn_l2tp_ipset_route_index%d_pkt_count_file"
#define VPN_PPTP_IPSET_MATCH_PACKET_COUNT_FILE "/tmp/rtk_vpn_pptp_ipset_route_index%d_pkt_count_file"
#endif

#endif

#ifdef CONFIG_USB_SUPPORT
int usb_filter(const struct dirent *dirent);
#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD) || defined(CONFIG_USER_PPTP_CLIENT)
int rtk_wan_vpn_policy_route_get_route_tableId(VPN_TYPE_T vpn_type, unsigned char *tunnelName, unsigned int *tableId);
int rtk_wan_vpn_policy_route_get_mark(VPN_TYPE_T vpn_type, unsigned char *tunnelName, unsigned int *mark, unsigned int *mask);
int rtk_wan_vpn_config_url_policy_route(char *url, struct in_addr addr, int mode);
int rtk_wan_vpn_flush_url_policy_route(VPN_TYPE_T vpn_type, unsigned char *tunnelName);
int rtk_wan_vpn_preset_url_policy_route(VPN_TYPE_T vpn_type, unsigned char *tunnelName);
int rtk_wan_vpn_flush_attach_rules(VPN_TYPE_T vpn_type, unsigned char *tunnelName, VPN_AC_T mode);
int rtk_wan_vpn_flush_attach_rules_all(VPN_TYPE_T vpn_type, VPN_AC_T mode);
int rtk_wan_vpn_set_attach_rules(VPN_TYPE_T vpn_type, unsigned char *tunnelName, VPN_AC_T mode);
int rtk_wan_vpn_set_attach_rules_all(VPN_TYPE_T vpn_type, VPN_AC_T mode);
#endif

#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
int update_redirect_by_ponStatus(void);
#endif

#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
int rtk_itms_qos_control_packet_protection(void);
int rtk_qos_control_packet_protection(void);
#endif

#ifdef CONFIG_E8B
struct file_pipe {
	unsigned char *buffer;
	size_t bufsize;
	void (*func)(unsigned char *buffer, size_t *bufsize);
};
extern const char BACKUP_DIRNAME[];

void enable_http_redirect2register_page(int enable);
#ifdef _PRMT_X_CT_COM_QOS_
int setup_tr069Qos(struct in_addr *qos_addr);
#endif
int gen_ctcom_dhcp_opt(unsigned char type, char *output, int out_len, char *account, char *passwd);
int get_cu_encrypted_data(char *out, int len, char *username, char *password);
int gen_cu_dhcp_opt(unsigned char type, char *output, int out_len, char *account, char *passwd);
void clearAlarm(unsigned int code);

#ifdef CONFIG_USB_SUPPORT
int isUSBMounted(void);
#endif

void encode(unsigned char *buf, size_t * buflen);
void decode(unsigned char *buf, size_t *buflen);
int file_copy_pipe(const char *inputfile, const char *outputfile, struct file_pipe *pipe);
#ifdef _PRMT_USBRESTORE
int usbRestore(void);
#endif

char * fixSpecialChar(char *str,char *srcstr,int length);
int hex(unsigned char ch);
void convert_mac(char *mac);
void changeMacFormat(char *str, char s, char d);
void changeMacStrToLower(char *str);


/*star:20080807 START add for ct qos model*/
#define NONEMODE  0
#define INTERNETMODE 1
#define TR069MODE     2
#define IPTVMODE       4
#define  VOIPMODE      8
#define  OTHERMODE    16
/*star:20080807 END*/

#if (defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)) && defined(CONFIG_APACHE_FELIX_FRAMEWORK)
#define OSGI_PULGIN_STATUS_DIR "/tmp/status_osgi"

typedef struct osgi_bundle_info_entry
{
	int bundle_id;
	char symbolic_name[128];
	char plugin_name[128];
	char version[32];
	char status[16];
	int level;
	char vendor[128];
	char path[256];
} OSGI_BUNDLE_INFO_T, *OSGI_BUNDLE_INFO_Tp;

int listAllOsgiPlugin(OSGI_BUNDLE_INFO_Tp *list, int *num);
#endif //#if (defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)) && defined(CONFIG_USER_OPENJDK8)

#ifdef CONFIG_APACHE_FELIX_FRAMEWORK
#ifdef CONFIG_CMCC
#define CURRENT_PERMISSION_API_NUM 50
extern const char  *osgi_capbilities_long[];
extern const char  *osgi_capbilities_short[];
#endif
#ifdef CONFIG_CMCC_OSGIMANAGE
#define OSGIMANAGE_DISABLED 0
#define OSGIMANAGE_ENABLED 1
#endif

//if change the file path, please also change the setting in osgi conf config.properties
//com.chinamobile.smartgateway.permission.idfile=/tmp/BundleId_permission
#define OSGI_PERMISSION_ID_FILE "/tmp/BundleId_permission"
#define OSGI_PERMISSION_RESTART "/etc/scripts/osgi_permission_restart"
#define OSGI_PERMISSION_PLUGIN_RUN	"/etc/scripts/osgi_plugin_run"

#define OSGI_PLUGIN_INSTALL 		"/etc/scripts/osgi_plugin_install"
#define OSGI_PLUGIN_RUN			"/etc/scripts/osgi_plugin_run"
#define OSGI_PLUGIN_STOP			"/etc/scripts/osgi_plugin_stop"
#define OSGI_PLUGIN_UNINSTALL   	"/etc/scripts/osgi_plugin_uninstall"
#define OSGI_PLUGIN_RESTART		"/etc/scripts/osgi_permission_restart"
#define OSGI_PLUGIN_UPDATE 		"/etc/scripts/osgi_plugin_update"

enum
{
	BUNDLE_STATE_UNINSTALLED=1,
	BUNDLE_STATE_INSTALLED=2,
	BUNDLE_STATE_RESOLVED=4,
	BUNDLE_STATE_STARTING=8,
	BUNDLE_STATE_STOPING=16,
	BUNDLE_STATE_ACTIVE=32,
};
#define CWMP_OSGI_CACHE_LINE_BUNDLE_ID 2
#define CWMP_OSGI_CACHE_LINE_RESOLVED 3
#define CWMP_OSGI_CACHE_LINE_SYMBOLIC_NAME 4
#define CWMP_OSGI_CACHE_LINE_NAME 5
#define CWMP_OSGI_CACHE_LINE_STATE 6
#define CWMP_OSGI_CACHE_LINE_RUNSTATE 7
#define CWMP_OSGI_CACHE_LINE_VERSION 8
#define CWMP_OSGI_CACHE_LINE_FILE 9

#define BUNDLE_PLUGIN_ACTION_TIMEOUT 100

int rtk_osgi_get_osgi_symbolic_name(char *name, char* symblic_name);
int rtk_osgi_get_osgi_bundle_id(char *name);
void rtk_osgi_start_bundle(int id);
int rtk_osgi_restart_osgi_permission_bundle(void);
int rtk_osgi_setup_osgi_permissions(char *new_plugin);
int rtk_osgi_get_plugin_by_bundleid(unsigned int bundleId, MIB_CMCC_OSGI_PLUGIN_Tp pentry);
int rtk_osgi_get_bundle_resolved(int id);
int rtk_osgi_is_bundle_action_timeout(int *bundle_id, char *file_name);
int rtk_osgi_get_bundle_Id_by_filename(char * file_name);
int rtk_osgi_check_bundle_file(void);
unsigned int rtk_osgi_get_permission_empty_inst(void);
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
void rtk_osgi_get_bundlefile_symbolicname(char* path, char* symbolicName);
#endif
void rtk_osgi_bundle_log(char* msg);
#endif //#ifdef CONFIG_APACHE_FELIX_FRAMEWORK
int updateMIBforQosMode(unsigned char *qosMode);

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
#define MSG_DNS_QUEUE_ID 0xff
#define MSG_DNS_QUEUE_FILE "/tmp/dns_msgserver"

struct data_msg_st{
	long msg_type;
	unsigned int msg_subtype;
	unsigned char mtext[256];
};
struct dns_info_st {
#ifdef CONFIG_CU_BASEON_YUEME
	char dns[MAX_REMOTE_ADDRESS_LEN+1];
#else
	char dns[128];
#endif
	char ip[64];
};
enum MSG_TYPE_ENUM
{
	MSG_TYPE_DNS=1,
	MSG_TYPE_HOST,
	MSG_TYPE_FLUSH_MSG,
};

enum MSG_SUBTYPE_ENUM
{
	MSG_SUBTYPE_ALL=0,
	MSG_SUBTYPE_ONCE
};

int sendDnsMsg(unsigned long msgType, unsigned int msgSubType, void* data, int len);
#endif

int delQoSRuleByMode(unsigned char *sub_qosMode);
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_AP_E8B_QOS)
extern char* ip_qos_classficationtype_str[IP_QOS_CLASSFICATIONTYPE_MAX];
extern char* ip_qos_protocol_str[IP_QOS_PROTOCOL_MAX];
void QosClassficationToQosRule(int action,int cls_id);
int delQosClassficationTypeRule(int ifIndex);
unsigned int getInstNum( char *name, char *objname );
int GetClsTypeValue(char *typevalue, MIB_CE_IP_QOS_CLASSFICATIONTYPE_T *p, unsigned int typenum, int maxflag);
int SetClsTypeValue(char *typevalue, MIB_CE_IP_QOS_CLASSFICATIONTYPE_T * p, int typenum, int maxflag);
#endif//#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_RTK_DEV_AP)
#if defined(CONFIG_APACHE_FELIX_FRAMEWORK)
#define OSGI_FLUSH_TARGET "com.chinamobile.smartgateway.mangement.FlushMsgConfig"
int sendMessageOSGIManagement(char*, const char*, char*);
int rtk_osgi_getInfo(char *name, char **resultp, int *result_len);
#endif

int getIpRange(char *src, char* start, char*end);

#if defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(CONFIG_USER_AP_E8B_QOS) || defined(CONFIG_YUEME)
int rtk_iconv_scan_multi_bytes(const char *pSrc);
int utf8_check(const char* str, size_t length);
int rtk_iconv_utf8_to_gbk(char* src, char *dst, int len);
int rtk_iconv_gbk_to_utf8(char* src, char *dst, int len);

int ssid_index_to_wlan_index(int ssid_index);

int rtk_osgi_traffic_parse_address(const char *address, char*ip, unsigned int *maskbit);
void rtk_osgi_init_traffic_qos_rule(void);
int lock_pon_statistic_file_by_flock(void);
int unlock_pon_statistic_file_by_flock(int lockfd);
#endif //#if defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(CONFIG_RTK_DEV_AP)

int getCPUClass(char *buf, int len);
void changeStringToMac(unsigned char *mac, unsigned char *macString);
unsigned int isMacAddrInvalid(unsigned char *pMac);
int parseRemotePort(char* source, int*start, int*end);
char* replaceChar(char*source, char found, char c);
int checkIPv4OrIPv6(char *remote, char* forward);
#ifdef CONFIG_E8B
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
unsigned int rtk_mac_filter_get_drop_pkt_count(const char *macStr);
#endif
int setupMacFilterTables(void);
#endif
int update_hosts_by_ip(char *hostname, char* ipaddr);
#if defined(CONFIG_CT_AWIFI_JITUAN_SMARTWIFI)
int killWiFiDog(void);
void startWiFiDog(void *null);
void restartWiFiDog(int restart);
unsigned int RTK_WAN_GET_UNAvailable_WifiDogInterface(void);
extern struct callout_s wifiAuth_ch;
extern int g_wan_modify;
void wifiAuthCheck(void *null);
#define WA_MAX_WAN_NAME 33
#define WIFIDOGPATH "/var/config/awifi/smartwifi"
#define WIFIDOGCONFPATH "/var/config/awifi/awifi.conf"
#define WIFIDOGTMPCONFPATH "/var/config/awifi/temp.conf"
#define WIFIDOGBAKCONFPATH "/var/config/awifi/awifi_bak.conf"
#define KILLWIFIDOGSTR "killall smartwifi"
#define AWIFI_DEFAULT_BIN_VERSION		"V4.0.0"
#endif

#ifdef SUPPORT_ACCESS_RIGHT
#define INTERNET_ACCESS_DENY				0
#define INTERNET_ACCESS_NO_INTERNET			1 /* can access lan side but can't access internet */
#define INTERNET_ACCESS_ALLOW				2
#define INTERNET_ACCESS_TRUST				3

#define STORAGE_ACCESS_DENY		0
#define STORAGE_ACCESS_ALLOW	1
#endif

#ifdef MAC_FILTER_SRC_WHITELIST
#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_RG_G3_SERIES)
#define WHITELIST_WAY WHITELIST_USING_ACL_RULE
#else
#define WHITELIST_WAY WHITELIST_USING_LUT_TBL
#endif
#endif
extern const char FW_MACFILTER_ROUTER[];


#ifdef CONFIG_CMCC_TRAFFIC_PROCESS_RULE_SUPPORT
void _add_tf_rule_into_nfhook(MIB_CMCC_TRAFFIC_PROCESS_RULE_Tp rule);
void _del_tf_rule_from_nfhook(int index);
#endif //#ifdef CONFIG_CMCC_TRAFFIC_PROCESS_RULE_SUPPORT

int sync_itfGroup(int ifidx);

#if (defined(CONFIG_USER_PPTP_CLIENT_PPTP) && defined(CONFIG_USER_L2TPD_L2TPD)) \
	 || (defined(CONFIG_USER_PPTP_CLIENT) && defined(CONFIG_USER_XL2TPD))
#define MAX_VPN_ACC_PW_LEN				128

typedef struct gdbus_vpn_tunnel_info {    				/* Used as argument to CreateWanL2TPTunnel() */
	VPN_MODE_T vpn_mode;
	VPN_PRIO_T vpn_priority;
	VPN_TYPE_T vpn_type;
	VPN_ENABLE_T vpn_enable;
	VPN_AUTH_TYPE_T authtype;
	VPN_ENCTYPE_T enctype;
	unsigned char account_proxy[MAX_DOMAIN_LENGTH];
	unsigned char account_proxy_msg[MAX_DOMAIN_LENGTH];
	unsigned char account_proxy_mac[MAC_ADDR_LEN];
	int account_proxy_result;
	int account_proxy_param_status;
	unsigned int vpn_port;
	unsigned int vpn_idletime;
	unsigned char serverIP[MAX_DOMAIN_LENGTH];
	unsigned char userName[MAX_VPN_ACC_PW_LEN+1];
	unsigned char passwd[MAX_VPN_ACC_PW_LEN+1];
	unsigned char tunnelName[MAX_NAME_LEN];
	unsigned char userID[MAX_NAME_LEN];
	unsigned char bind_wan_inf[64];
#ifdef CONFIG_IPV6_VPN
	unsigned char IpProtocol;		 // 1: IPv4, 2: IPv6
#endif
#ifdef COM_CUC_IGD1_L2TPVPN
	unsigned int l2tp_instnum;
#endif
} gdbus_vpn_tunnel_info_t;

/* If you want to change VPN_CONN_POLICY_RULE_MAX_COUNT, please remember to change
 * MAX_DOMAINS_LENGTH in dbusproxy/reg/include/prmt_apply.h and MAX_DOMAINS_LENGTH in utility.h too.
 * New test plan will test VPN 10241 rules 20201216.
 */
#define VPN_CONN_POLICY_RULE_MAX_COUNT 10241
typedef struct gdbus_vpn_connection_info {
	ATTACH_MODE_T attach_mode;
	unsigned char *domains[VPN_CONN_POLICY_RULE_MAX_COUNT];
	unsigned char *ips[VPN_CONN_POLICY_RULE_MAX_COUNT];
	unsigned char *terminal_mac[VPN_CONN_POLICY_RULE_MAX_COUNT];
	gdbus_vpn_tunnel_info_t vpn_tunnel_info;
} gdbus_vpn_connection_info_t;

int CreateWanPPTPTunnel(gdbus_vpn_tunnel_info_t *vpn_tunnel_info, ATTACH_MODE_T attach_mode, unsigned char *reason);
int RemoveWanPPTPTunnel(gdbus_vpn_tunnel_info_t *vpn_tunnel_info, unsigned char *reason);
int AttachWanPPTPTunnel(gdbus_vpn_tunnel_info_t *vpn_tunnel_info, unsigned char **ipDomainNameAddr, int attach_size, unsigned char *reason);
typedef struct pptp_tunnel_status
{
	char tunnelName[32];
	char tunnelStatus[8];
}PPTP_Status_T, *PPTP_Status_Tp;
int DetachWanPPTPTunnel(gdbus_vpn_tunnel_info_t *vpn_tunnel_info, ATTACH_MODE_T attach_mode, unsigned char **ipDomainNameAddr, int attach_size, unsigned char *reason);

int GetWanPPTPTunnelStatus(gdbus_vpn_tunnel_info_t *vpn_tunnel_info, unsigned char *reason, PPTP_Status_Tp pptp_list, int *num);

int CreateWanL2TPTunnel(gdbus_vpn_tunnel_info_t *vpn_tunnel_info, ATTACH_MODE_T attach_mode, unsigned char *reason);
int RemoveWanL2TPTunnel(gdbus_vpn_tunnel_info_t *vpn_tunnel_info, unsigned char *reason);
int AttachWanL2TPTunnel(gdbus_vpn_tunnel_info_t *vpn_tunnel_info, unsigned char **ipDomainNameAddr, int attach_size, unsigned char *reason);
typedef struct l2tp_tunnel_status
{
	char tunnelName[32];
	char tunnelStatus[8];
}L2TP_Status_T, *L2TP_Status_Tp;
int DetachWanL2TPTunnel(gdbus_vpn_tunnel_info_t *vpn_tunnel_info, ATTACH_MODE_T attach_mode, unsigned char **ipDomainNameAddr, int attach_size, unsigned char *reason);

int GetWanL2TPTunnelStatus(gdbus_vpn_tunnel_info_t *vpn_tunnel_info, unsigned char *reason, L2TP_Status_Tp l2tp_list, int *num);
int renew_vpn_accpw_from_accpxy (VPN_TYPE_T vpn_type, unsigned char *sppp_if_name, unsigned char *new_username, unsigned char *new_password);

void modPolicyRouteTable(const char *pptp_ifname, struct in_addr *real_addr);
#ifdef CONFIG_IPV6_VPN
void modIPv6PolicyRouteTable(const char *pptp_ifname, struct in6_addr *real_addr);
#endif //#ifdef CONFIG_IPV6_VPN
#endif //#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) && defined(CONFIG_USER_L2TPD_L2TPD)

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD) \
	|| defined(CONFIG_USER_PPTP_CLIENT) || defined(CONFIG_USER_XL2TPD)

int request_vpn_accpxy_server( gdbus_vpn_tunnel_info_t *vpn_tunnel_info, char *reason );
//int get_attach_pattern_by_mode(gdbus_vpn_connection_info_t *vpn_connection_info,unsigned char *attach_pattern[],unsigned char *reason);
int get_attach_pattern_by_mode(gdbus_vpn_connection_info_t *vpn_connection_info,unsigned char ***attach_pattern, int *attch_pattern_size,unsigned char *reason);

VPN_STATUS_T WanVPNConnection( gdbus_vpn_connection_info_t *vpn_connection_info, unsigned char *reason );
VPN_STATUS_T WanVPNConnectionStat( gdbus_vpn_connection_info_t *vpn_connection_info, unsigned char *status[] );
int getVPNOutIfname(VPN_TYPE_T type, void *pentry, char *oifname);
int rtk_wan_vpn_policy_route_get_if_route_tableId(VPN_TYPE_T vpn_type, unsigned char *ifname, unsigned int *tableId);
unsigned long long rtk_wan_vpn_get_attached_pkt_count_by_ifname(VPN_TYPE_T vpn_type, unsigned char *if_name);
unsigned long long rtk_wan_vpn_get_attached_pkt_count_by_route_index(VPN_TYPE_T vpn_type, unsigned int index);
int rtk_wan_vpn_set_dynamic_url_policy_route(char *name, struct in_addr addr);

#ifdef VPN_MULTIPLE_RULES_BY_FILE
int rtk_wan_vpn_preset_url_policy_route_by_file(VPN_TYPE_T vpn_type, unsigned char *tunnelName, unsigned int rule_index, unsigned int priority, char *url);
int rtk_wan_vpn_get_ipset_packet_count_by_route_index(VPN_TYPE_T vpn_type, unsigned char *tunnelName, unsigned int routeIdx, unsigned int rule_mode);
#endif
#endif //#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD)

void base64_encode(unsigned char *from, char *to, int len);
void base64encode(unsigned char *from, char *to, int len);
int base64_decode(void *dst,char *src,int maxlen);
void get_dns_by_wan(MIB_CE_ATM_VC_T *pEntry, char *dns1, char *dns2);
#endif //CONFIG_E8B
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD)
int rtk_wan_vpn_attach_policy_route_init(void);
#endif

#ifdef CONFIG_IPV6
int checkIPv6Route(MIB_CE_IPV6_ROUTE_Tp new_entry);
#ifdef CONFIG_E8B
int getIPv6addrInfo(MIB_CE_ATM_VC_Tp entryp, char *ipaddr, char *netmask, char *gateway);
void get_dns6_by_wan(MIB_CE_ATM_VC_T *pEntry, char *dns1, char *dns2);
#endif

int startUpgradeFirmware(int needreboot, char *downUrl);
void startPushwebTimer(unsigned int time);
#endif

#ifdef CONFIG_CMCC_OSGIMANAGE
extern const char OSGIMANAGE_PID[];
extern const char OSGIAPP_PID[];

#define OSGIMANAGE_DISABLED 0
#define OSGIMANAGE_ENABLED 1
int startOsgiManage(void);
int setupOsgiManage(int type);
int restartOsgiApp(void);
#endif
extern const char KMODULE_CMCC_MAC_FILTER_FILE[];

int isIPAddr(char * IPStr);

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME) //from user/ctcyueme/ftpapi.c
void smartHGU_ftpserver_init_api(void);
void smartHGU_ftpserver_account_update(void);
#endif
#ifdef CONFIG_MULTI_FTPD_ACCOUNT
void ftpd_account_change(void);
#if !defined(CONFIG_YUEME) && !defined(CONFIG_CU_BASEON_YUEME)
int enable_ftpd(int enable);
int get_ftpd_capability(void);
int enable_anonymous_ftpd_account(int enable);
int add_ftpd_account(char *username, char *passwd);
int clear_ftpd_account(void);
#endif //#if !defined(CONFIG_YUEME)
#endif //CONFIG_MULTI_FTPD_ACCOUNT
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
int get_ftpUserAccount(char *username, char *password);
int set_ftpUserAccount(char *username, char *password);
int get_ftp_enable(void);
int set_ftp_enable(int data);
#endif

#ifdef CONFIG_MULTI_SMBD_ACCOUNT
int enable_smbd(int enable);
int get_smbd_capability(void);
int enable_anonymous_smbd_account(int enable);
int add_smbd_account(char *username, char *passwd, char* path, unsigned char writable);
int rtk_app_del_smbd_account(char *username);
int clear_smbd_account(void);
void smbd_account_change(void);
#endif //CONFIG_MULTI_SMBD_ACCOUNT

int enable_httpd(int enable);
int set_httpd_password(char *passwd);
int get_httpd_capability(void);

int getOUIfromMAC(char* oui);

#ifdef LED_TIMER
int setLedStatus(unsigned char status);
int getLedStatus(unsigned char* status);
#endif //LED_TIMER

int flock_set(int fd, int type);
int convertMacFormat(char *str, unsigned char *mac);
unsigned char isZeroMac(unsigned char *pMac) ;
unsigned int macAddrCmp(unsigned char *addr1, unsigned char *addr2);
void changeMacToString(unsigned char *mac, unsigned char *macString);
void fillcharZeroToMacString(unsigned char *macString);

#define TELMEXPPPUSER "prodigyweb.com.mx"

#define MAX_LANNET_DEV_NAME_LENGTH			64
#define MAX_LANNET_VENDOR_CLASS_IDENTIFIER	64
#define MAX_LANNET_BRAND_NAME_LENGTH		64
#define MAX_LANNET_MODEL_NAME_LENGTH		64
#define MAX_LANNET_OS_NAME_LENGTH			64
#define MAX_LANNET_IF_NAME_LENGTH			64

#ifdef CONFIG_USER_LANNETINFO
#define MAX_LAN_HOST_NUM		256
#define MAX_DEV_TIME_LEN		64

#define PORTSPEEDFILE	"/var/portspeed"
#define LANNETINFOFILE	"/var/lannetinfo"
#define LANNETINFO_RUNFILE	"/var/run/lannetinfo.pid"
#define WIFIHOSTFILE "/var/wifi_unasso_host"
#define LANNETINFO_UNASSOSTA_FILE	"/var/lannetinfo_unassosta"
typedef enum lanHostInfo_device_type_e
{
	LANHOSTINFO_TYPE_OTHER,
	LANHOSTINFO_TYPE_PHONE,
	LANHOSTINFO_TYPE_PC,
	LANHOSTINFO_TYPE_PAD,
	LANHOSTINFO_TYPE_STB,
	LANHOSTINFO_TYPE_AP
}lanHostInfo_device_type_t;

//#ifdef SUPPORT_GET_MULTIPLE_LANHOST_IPV6_ADDR
#define MAX_IPV6_CNT 5
typedef enum lanHost_ipv6_type_e
{
	LANHOST_IPV6_TYPE_LINKLOCAL = 1,
	LANHOST_IPV6_TYPE_GLOBAL
}lanHost_ipv6_type_t;

typedef struct lanhost_ipv6_s{ //this struct must 4-byte alignment
	char ipv6Addr[IP6_ADDR_LEN];
	lanHost_ipv6_type_t type;
	unsigned int use;
}lanhost_ipv6_t;
//#endif

typedef struct lan_net_port_speed_S{
	int portNum;
	unsigned int rxRate;
	unsigned int txRate;
}lan_net_port_speed_T;

typedef struct lanHostInfo_s
{
	unsigned char		mac[MAC_ADDR_LEN];
	char				devName[MAX_LANNET_DEV_NAME_LENGTH];
#ifdef GET_VENDOR_CLASS_ID
	char				vendor_class_id[MAX_LANNET_VENDOR_CLASS_IDENTIFIER];
#endif
	unsigned char		devType;	/*CMCC [0-other, 1-phone, 2-pc, 3-pad, 4-stb, 5-ap]; ELSE[0-phone 1-pad 2-PC 3-STB 4-other  0xff-unknown] */
	unsigned int		ip; /* network order */
	lanhost_ipv6_t		lanhost_ipv6s[MAX_IPV6_CNT];
	unsigned char		ipv6Addr[IP6_ADDR_LEN];
	unsigned char		connectionType;	/* 0- wired 1-wireless */
	unsigned char		port;	/* 0-wifi, 1- lan1, 2-lan2, 3-lan3, 4-lan4 */
	unsigned char		phy_port;
	char        		ifName[MAX_LANNET_IF_NAME_LENGTH];
	char				brand[MAX_LANNET_BRAND_NAME_LENGTH];
	char				model[MAX_LANNET_MODEL_NAME_LENGTH];
	char				os[MAX_LANNET_OS_NAME_LENGTH];
	char				osVer[MAX_LANNET_OS_NAME_LENGTH];
	char				swVer[MAX_LANNET_OS_NAME_LENGTH];
	unsigned int		onLineTime;
	unsigned int 		upRate;   /* in unit of kbps */
	unsigned int 		downRate; /* in unit of kbps */
	unsigned long long	rxBytes;
	unsigned long long	txBytes;
	unsigned char		firstConnect;
	unsigned char		disConnect;
	unsigned char		ntpFlag;/* 1:valid, 0:invalid*/
	char 				latestActiveTime[MAX_DEV_TIME_LEN];
	char				latestInactiveTime[MAX_DEV_TIME_LEN];
	unsigned char 		controlStatus;
	unsigned char  		internetAccess; /* default is 2 */
	unsigned char  		storageAccess; /* default is 1 */
	unsigned int		rx_packets;
	unsigned int		tx_packets;
	unsigned int		rx_errors;
	unsigned int		rx_dropped;
	unsigned int		firstStatistic;
	unsigned char		offline;	/* 0-do not care anymore 1-offline, if this is set should not recycle*/
	unsigned long		offline_expired_time;
	unsigned char		isdhcp;
#if defined (CONFIG_USER_CWMP_TR069) && defined (CONFIG_TELMEX_DEV)
	unsigned int		instNum;
#endif
	unsigned int		arpTargetIp; /* network order */
	unsigned int		arpRetryCnt;
	unsigned int		dhcpClientState;
	unsigned int		ipSource;
#if defined(CONFIG_CMCC) && defined(CONFIG_SUPPORT_ADAPT_DPI_V3_3_0)
	unsigned long		port_rx_errors;// lan port total error, not device
	char		port_rx_errors_rate[16];//
#endif
	unsigned char		firstGotIP;
	unsigned char		isWifiStation;
} lanHostInfo_t;

typedef struct hgDevInfo_s
{
	unsigned char	mac[MAC_ADDR_LEN];
	char			devName[MAX_LANNET_DEV_NAME_LENGTH];
} hgDevInfo_t;


#define MAX_DEVICE_INFO_SIZE  4096
typedef struct lannetmsgInfo {
	int		cmd;
	int		arg1;
	int		arg2;
	char	mtext[MAX_DEVICE_INFO_SIZE];
} LANNETINFO_MSG_T;

struct lanNetInfoMsg {
    long mtype;			// Message type
    long request;		// Request ID/Status code
    long tgid;			// thread group tgid
	LANNETINFO_MSG_T msg;
};

#ifdef _PRMT_X_TRUE_LASTSERVEICE
#define MAX_FLOW_INFO_SIZE    1024
#endif

#ifdef EASY_MESH_STA_INFO
typedef struct lanMacInfo_s{
	unsigned char mac[MAC_ADDR_LEN];
	unsigned char port;
}lanMacInfo_t;
#endif

#define LANNETINFO_MSG_SUCC		0
#define LANNETINFO_MSG_FAIL		1

#define CMD_NEW_DEVICE_INFO_GET				0x1
#define CMD_LEAVE_DEVICE_INFO_GET			0x2
#define CMD_LAN_HOST_MAX_NUMBER_SET			0x4
#define CMD_LAN_HOST_MAX_NUMBER_GET			0x8
#define CMD_LAN_HOST_NUMBER_GET				0x10
#define CMD_CONTROL_LIST_MAX_NUMBER_SET		0x20
#define CMD_CONTROL_LIST_MAX_NUMBER_GET		0x40
#define CMD_CONTROL_LIST_NUMBER_GET			0x80
#define CMD_LAN_HOST_CONTROL_STATUS_SET		0x100
#define CMD_LAN_HOST_CONTROL_STATUS_GET		0x200
#define CMD_LAN_HOST_ACCESS_RIGHT_SET		0x400
#define CMD_LAN_HOST_ACCESS_RIGHT_GET		0x800
#define CMD_LAN_HOST_DEVICE_TYPE_SET		0x1000
#define CMD_LAN_HOST_DEVICE_TYPE_GET		0x2000
#define CMD_LAN_HOST_BRAND_SET				0x4000
#define CMD_LAN_HOST_BRAND_GET				0x8000
#define CMD_LAN_HOST_MODEL_SET				0x10000
#define CMD_LAN_HOST_MODEL_GET				0x20000
#define CMD_LAN_HOST_OS_SET					0x40000
#define CMD_LAN_HOST_OS_GET					0x80000
#define CMD_LAN_HOST_INFORMATION_CHANGE_GET	0x100000
#define CMD_LAN_HOST_LINKCHANGE_PORT_SET	0x200000
#define CMD_LAN_HOST_GET_PORT_SPEED			0x400000
#define CMD_LAN_HOST_DHCPRELEASE_IP_SET		0x800000
#define CMD_LAN_HOST_FORCE_RECYCLE			0x1000000
#define CMD_LAN_HOST_FLOWINFO_GET     	    0x2000000
#define CMD_LAN_DEVICE_MAC_GET				0x4000000

#ifdef CONFIG_E8B
int set_lanhost_max_number(unsigned int number);
int get_lanhost_max_number(unsigned int * number);
int get_lanhost_number(unsigned int * number);
int set_controllist_max_number(unsigned int number);
int get_controllist_max_number(unsigned int * number);
int get_controllist_number(unsigned int * number);
int set_lanhost_control_status(unsigned char *pMacAddr, unsigned char controlStatus);
int get_lanhost_control_status(unsigned char *pMacAddr, unsigned char * controlStatus);
int set_lanhost_access_right(unsigned char *pMacAddr, unsigned char internetAccessRight, unsigned char storageAccessRight);
int get_lanhost_access_right(unsigned char *pMacAddr, unsigned char * internetAccessRight, unsigned char * storageAccessRight);
int set_lanhost_device_type(unsigned char *pMacAddr, unsigned char devType);
int get_lanhost_device_type(unsigned char *pMacAddr, unsigned char *devType);
int set_lanhost_brand(unsigned char *pMacAddr, unsigned char *brand);
int get_lanhost_brand(unsigned char *pMacAddr, unsigned char *brand);
int set_lanhost_model(unsigned char *pMacAddr, unsigned char *model);
int get_lanhost_model(unsigned char *pMacAddr, unsigned char *model);
int set_lanhost_OS(unsigned char *pMacAddr, unsigned char *os);
int get_lanhost_OS(unsigned char *pMacAddr, unsigned char *os);
int get_lanhost_information_change(lanHostInfo_t *pLanDeviceInfo, int num);
int get_new_and_leave_device_info(lanHostInfo_t *pLanDeviceInfo, int num);
int set_attach_device_name(unsigned char *pMacAddr, char *pDevName);
int get_attach_device_name(unsigned char *pMacAddr, char *pDevName);
#ifdef CONFIG_CU
int set_attach_device_type(unsigned char *pMacAddr, char *pDevType);
int get_attach_device_type(unsigned char *pMacAddr, char *pDevType);
#endif //CONFIG_CU
int get_hg_dev_name(hgDevInfo_t **ppHGDevData, unsigned int *pCount);
int get_online_and_offline_device_info(lanHostInfo_t **ppLANNetInfoData, unsigned int *pCount);
void restartlanNetInfo(void);
int rtk_update_lan_hpc_for_mib(unsigned char *mac);
#endif
#ifdef CONFIG_USER_HAPD
extern const char LANNETINFO_ACTION[];
#endif
#endif	//#ifdef CONFIG_USER_LANNETINFO

#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL
int isWifiMacOnline(unsigned char *mac);
int reCalculateLanPortRateLimitInfo(void);
void apply_wifiRateLimit(void);
void clear_wifiRateLimit(void);
void clear_lanPortRateLimit(void);
void apply_lanPortRateLimit(void);
void poll_macinfo(void);
void init_maxBandwidth(void);
void apply_maxBandwidth(void);
void clear_maxBandwidth(void);
unsigned long long lanPortRateLimitShift(unsigned int rate, unsigned int dir);
int getInternetWANItfName(char* devname,int len);
void apply_lanPortRateLimitInPS(void);
void clear_lanPortRateLimitInPS(void);
int find_lanHostByMac(char *mac);
int is_lanHostMacNeedTrap(char *mac, int dir);
int rtk_lan_host_index_check_exist(int hostIndex);
int rtk_lan_host_get_new_host_index(void);
int rtk_lan_host_get_mib_index_by_host_index(int hostIndex);
#if defined(CONFIG_USER_LAN_BANDWIDTH_EX_CONTROL)
int set_attach_device_maxbandwidth(unsigned char *pMacAddr, unsigned int maxUsBandWidth, unsigned int maxDsBandWidth, unsigned int minUsBandWidth, unsigned int minDsBandWidth);
#else
int set_attach_device_maxbandwidth(unsigned char *pMacAddr, unsigned int maxUsBandWidth, unsigned int maxDsBandWidth);
#endif
int get_attach_device_maxbandwidth(unsigned char *pMacAddr, unsigned int *pUsBandWidth, unsigned int *pDsBandWidth, unsigned int *pUsBandWidthMin, unsigned int *pDsBandWidthMin);

#define DEFAULT_MAX_US_BANDWIDTH		1024*1024
#define DEFAULT_MAX_DS_BANDWIDTH		1024*1024
#endif //#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL

#ifdef SUPPORT_ACCESS_RIGHT
void flush_internet_access_right(void);
void setup_internet_access_right(void);
void flush_storage_access_right(void);
void setup_storage_access_right(void);
void apply_accessRight(unsigned char enable);
int set_attach_device_right(unsigned char *pMacAddr, unsigned char internetAccessRight, unsigned char storageAccessRight);
#endif //#ifdef SUPPORT_ACCESS_RIGHT

#ifdef CONFIG_CMCC_FORWARD_RULE_SUPPORT
#define CMCC_FORWARDRULE_DELETE 0
#define CMCC_FORWARDRULE_ADD  1
#define CMCC_FORWARDRULE_MOD  2
#define CMCC_FORWARDRULE_ACTION_APPEND(type) (type==CMCC_FORWARDRULE_ADD)?"-A":"-D"
#define CMCC_FORWARDRULE_ACTION_INSERT(type)   (type==CMCC_FORWARDRULE_ADD)?"-I":"-D"
#define CMCC_FORWARDRULE_ACTION_PARAM_INT(type,d) (type==CMCC_FORWARDRULE_ADD)?int2str(d):""

void setCmccForwardRuleIptables(MIB_CMCC_FORWARD_RULE_T *Entry, int type);
int DelCmccForwardRule(MIB_CMCC_FORWARD_RULE_T *Entry);
int DelAllCmccForwardRule(void);
int AddCmccForwardRule(MIB_CMCC_FORWARD_RULE_T *Entry, int type);
int setupCmccForwardRule(int type);
void rtk_cmcc_init_cmcc_forward_rule(void);
int rtk_cmcc_update_cmcc_forward_rule_url(char*domainName, char* addr);

extern const char CMCC_FORWARDRULE[];
extern const char CMCC_FORWARDRULE_POST[];
#endif //CONFIG_CMCC_FORWARD_RULE_SUPPORT

#if defined(CONFIG_PON_LINKCHANGE_EVENT) || defined(CONFIG_PON_CONFIGURATION_COMPLETE_EVENT)
#define OMCI_CONFIG_COMPELETE_STR	"OMCI_Config_is_Compelete"
#endif

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
void rtk_if_statistic_reset(void);
int rtgponstatus2int(int rt_onu_state);
#endif

#if defined(CONFIG_USER_RTK_OMD) || defined(CONFIG_AIRTEL_EMBEDUR)
#define UNKNOWN_REBOOT			0x00
#define ITMS_REBOOT				0x01
#define TELECOM_WEB_REBOOT		0x02
#define DBUS_REBOOT				0x04
#define TERMINAL_REBOOT			0x08
#define POWER_REBOOT			0x10
#define EXCEP_REBOOT			0x20
#define REBOOT_FLAG				0x80
#if defined(CONFIG_AIRTEL_EMBEDUR)
#define ADMIN_FLAG				0x40
#define EVENT_FLAG				0x100
#define INSTALL_FLAG			0x200
#endif
int write_omd_reboot_log(unsigned int flag);
int is_terminal_reboot(void);
#endif

#ifdef STB_L2_FRAME_LOSS_RATE
#define MAX_STB	5

#define L2LOSSTESTMSGSTART	1
#define L2LOSSTESTMSGEND	2
#define L2LOSSTESTMSGSTARTFROMWEB	3

typedef struct l2LossRateResult
{
	unsigned char stbMac[6];
	unsigned char port;
	float lossRate;
	unsigned int timeDelay;
	unsigned int timeTremble;
	unsigned int recvPktNum;
	unsigned int lastTimeDelay;
}l2LossRateResult_t, *l2LossRateResult_p;
typedef struct stbL2Msg
{
	long int msgType;
	l2LossRateResult_t res[MAX_STB];
}stbL2Msg_t, *stbL2Msg_p;

#define L2LOSSRATEFILE ("/bin/stbL2Com")

#define STB_L2_DIAG_RESULT  "/tmp/stbL2Diag.tmp"
#endif

#if defined(SUPPORT_WEB_PUSHUP)
typedef enum
{
	FW_UPGRADE_STATUS_NOTIFY=-1,
	FW_UPGRADE_STATUS_NONE=0,
	FW_UPGRADE_STATUS_VER_INCORRECT,
	FW_UPGRADE_STATUS_DOWNLOAD_FAIL,
	FW_UPGRADE_STATUS_SPACE_INVALID,
	FW_UPGRADE_STATUS_VER_EXIST,
	FW_UPGRADE_STATUS_OTHERS,
	FW_UPGRADE_STATUS_PROGGRESSING,
	FW_UPGRADE_STATUS_SUCCESS
} FW_UPGRADE_STATUS_T;


FW_UPGRADE_STATUS_T firmwareUpgradeConfigStatus( void );
void firmwareUpgradeConfigStatusSet( FW_UPGRADE_STATUS_T status );
int firmwareUpgradeConfigSet(char *downUrl, int mode, int method, int needreboot, char *upgradeID);
#if defined(CONFIG_YUEME)
void firmwareUpgradeStatusChgEmitSignal(void);
void SetUAPasswdChgEmitSignal(void);
#endif
void firmwareUpgradeConfigStatusClean( void );
#endif

#ifdef CONFIG_USER_DBUS_PROXY
#define M_dbus_proxy_pid "/tmp/dbusproxy_pid"
#define DBUS_STRING_LEN 64

#define MANUFACTURER_STR	"REALTEK SEMICONDUCTOR CORP."
#define MANUFACTUREROUI_STR	"00E04C"
#define SPECVERSION_STR		"1.0"
#ifdef CONFIG_RTL8686
#define HWVERSION_STR           "V101"
#else
#define HWVERSION_STR		"8671x"
#endif

//#ifdef SUPPORT_MCAST_TEST
#define MCDIAG_RESULT_NO_IPTV_CONNECTION ("NO_IPTV_CONNECTION")
#define MCDIAG_RESULT_IPTV_CONNECTION_EXIST ("IPTV_CONNECTION_EXIST")
#define MCDIAG_RESULT_IPTV_DISCONNECT ("IPTV_DISCONNECT")
#define MCDIAG_RESULT_IPTV_CONNECT ("IPTV_CONNECT")
#define MCDIAG_RESULT_INVALID_MULTIVLAN ("INVALID_MULTIVLAN")
#define MCDIAG_RESULT_VALID_MULTIVLAN ("VALID_MULTIVLAN")
#define MCDIAG_RESULT_IPTV_BUSSINESS_NOK ("IPTV_BUSSINESS_NOK")
#define MCDIAG_RESULT_IPTV_BUSSINESS_OK ("IPTV_BUSSINESS_OK")
#define ROMEDRIVER_IGMP_PROC "/proc/rg/igmpSnooping"
#define ROMEDRIVER_IGMP_INFO "address"

#define MCDIAG_RESULT_IPTV_INFO_GET_ERRS ("iptv info get failed")

#define M_dbus_proxy_pid "/tmp/dbusproxy_pid"
//#endif

#ifndef DATA_LEN
    #define DATA_LEN 1024
#endif

typedef struct{
    int   table_id;
    int   Signal_id;
    int   iPid;       /**process id **/
    char  content[DATA_LEN];
}mib2dbus_notify_app_t;

extern int mib_2_dbus_notify_dbus_api(mib2dbus_notify_app_t *notify_app);

typedef enum{
    e_dbus_proxy_signal_mib_list = 0x9000,
    e_dbus_signal_mib_set,
    e_dbus_signal_mib_chain_add,
    e_dbus_signal_mib_chain_delete,
    e_dbus_signal_mib_chain_update,

    e_dbus_proxy_signal_mib_list_end
}e_notify_mib_signal_id;

typedef enum{
    e_table_list_action_init = 0x6000,
    e_table_list_action_retrieve,

    e_table_list_action_end
}e_table_list_action_T;
#endif

#ifdef SUPPORT_INCOMING_FILTER
#define IN_COMING_API_ADD (1)
#define IN_COMING_API_DEL (2)
#define IN_COMING_RET_SUCCESSFUL (0)
#define IN_COMING_RET_FAIL (-1)
#define IN_COMING_IP_MAX_LEN (64)
#define IN_COMING_CMD_BUF_MAX_LEN (1024)
#define IN_COMING_IS_IPV4 (1)
#define IN_COMING_IS_IPV6 (2)

typedef enum
{
	IN_COMING_PROTO_TCP_E = 0,
	IN_COMING_PROTO_UDP_E ,
	IN_COMING_PROTO_TCP_AND_UDP_E ,
} in_coming_proto_e;

typedef enum
{
	IN_COMING_INTERFACE_WAN_E = 0,
	IN_COMING_INTERFACE_LAN_E,
} in_coming_interface_e;

typedef struct
{
	char remoteIP[IN_COMING_IP_MAX_LEN];
	in_coming_proto_e protocol;
	unsigned int port;
	in_coming_interface_e interface;
} smart_func_in_coming_val;

int smart_func_add_in_coming_api(int cmd, smart_func_in_coming_val *in_coming_val, char *errdesc);
#endif

#ifdef CONFIG_USER_VXLAN
typedef enum{
	VXLAN_WORKMODE_BRIDGE=1,
	VXLAN_WORKMODE_ROUTING
}VXLAN_WORK_TYPE_E; //Work_mode
typedef enum{
	VXLAN_ADDRESSTYPE_DHCP=1,
	VXLAN_ADDRESSTYPE_STATIC
}VXLAN_ADDRESSTYPE_TYPE_E; //AddressType

typedef enum{
	VXLAN_NAPT_DISABLED=0,
	VXLAN_NAPT_ENABLED
}VXLAN_NAPT_TYPE_E; //AddressType

typedef enum{
	VXLAN_VLAN_DISABLED=0,
	VXLAN_VLAN_ENABLED
}VXLAN_VLAN_TYPE_E; //AddressType

extern const char VXLAN_INTERFACE[] ;
extern const char VXLAN_CMD[] ;
#define VXLAN_DSTPORT 4789
#endif
extern const char VCONFIG[];
#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
extern const char WIZARD_IN_PROCESS[];
#endif
#if (defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_) ) && defined(_SUPPORT_L2BRIDGING_DHCP_SERVER_)
extern const char DHCPD_IPV4_INTERFACE_GROUP_LEASES_PARSED[];
extern const char DHCPD_IPV4_PARSE_LEASES_SCRIPT[];
extern const char DHCPD_IPV4_INTERFACE_GROUP_LEASES[];
extern const char DHCPD_IPV4_INTERFACE_GROUP_CONF[];
extern const char DHCPD_IPV4_INTERFACE_GROUP_PID[];
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
//20180103:these functions can be control by PROVINCE_SICHUAN_FUNCTION_MASK.
#define PROVINCE_SICHUAN_TRACEROUTE_TEST 0x1
#define PROVINCE_SICHUAN_RESETFACTORY_TEST 0x2
#define PROVINCE_SICHUAN_PORTCONTROL_TEST 0x4
#define PROVINCE_SICHUAN_TERMINAL_INSPECTION 0x8

#define SMT_HGU_FTPACCOUNT_STR_LEN 32

#ifdef CONFIG_USER_DBUS_PROXY
int DBUS_ifponstate(void);
void DBUS_poninfo(int s,double *buffer);
int do_samba_update(char *newpasswd);
int dbus_proxy_uds_client_conn(const char *servername);
int dbus_proxy_uds_send_client_msg(int sockfd,char *buf,int size);
void send_notify_msg_dbusproxy(int id, e_notify_mib_signal_id signal_id, int recordNum);
int nortify_wan_update_2_dbus_proxy(const char *ifname, int msg_type, int ifa_family);
#endif

int get_restore_status(void);
void update_restore_status(int flag);
int start_collection_debug_info(char *uploadURL, unsigned short timeout, char *category);
int parseFrameworkPartition(char **f1, char **f2, char **c1);
int startHomeNas(void);

#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU)
int checkValidRedirect(char*redirecturl, char** checklist);
#endif

#ifdef CTC_DNS_TUNNEL
/* For DBUS API */
typedef struct ctc_dns_tunnel_node_s
{
	char *server_ip;
	char *domain;
	struct ctc_dns_tunnel_node_s *next;
}dns_tunnel_node_t;

int attach_wan_dns_tunnel(dns_tunnel_node_t *node);
int detach_wan_dns_tunnel(dns_tunnel_node_t *node);
int get_wan_dns_tunnel(dns_tunnel_node_t **tunnels);
void free_dns_tunnels(dns_tunnel_node_t *tunnels);
#endif

#ifdef CONFIG_E8B
/*error code*/
#define ERR_9000	-9000	/*Method not supported*/
#define ERR_9001	-9001	/*Request denied*/
#define ERR_9002	-9002	/*Internal error*/
#define ERR_9003	-9003	/*Invalid arguments*/
#define ERR_9004	-9004	/*Resources exceeded*/
#define ERR_9005	-9005	/*Invalid parameter name*/
#define ERR_9006	-9006	/*Invalid parameter type*/
#define ERR_9007	-9007	/*Invalid parameter value*/
#define ERR_9008	-9008	/*Attempt to set a non-writable parameter*/
#define ERR_9009	-9009	/*Notification request rejected*/
#define ERR_9010	-9010	/*Download failure*/
#define ERR_9011	-9011	/*Upload failure*/
#define ERR_9012	-9012	/*File transfer server authentication failure*/
#define ERR_9013	-9013	/*Unsupported protocol for file transfer*/

#define REBOOT_DELAY_FILE "/var/rebootdelay"
#ifdef VIRTUAL_SERVER_SUPPORT
#define VIRTUAL_SERVER_DELETE 0
#define VIRTUAL_SERVER_ADD  1
#define VIRTUAL_SERVER_ACTION_APPEND(type) (type==VIRTUAL_SERVER_ADD)?"-A":"-D"
#define VIRTUAL_SERVER_ACTION_INSERT(type)   (type==VIRTUAL_SERVER_ADD)?"-I":"-D"
#define VIRTUAL_SERVER_ACTION_PARAM_INT(type,d) (type==VIRTUAL_SERVER_ADD)?int2str(d):""

int setupVtlsvr(int type);
#endif
#ifdef _PRMT_X_CT_COM_QOS_
#define NONEMODE  0
#define INTERNETMODE 1
#define TR069MODE     2
#define IPTVMODE       4
#define  VOIPMODE      8
#define  OTHERMODE    16
#endif
#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_) || defined(_PRMT_X_CT_COM_WirelessTestDiagnostics_)
int getAllWirelessChannelOnce(void);
int get_WirelessChannelResult(unsigned char phyband, char *AllWirelessChannel, char *BestWirelessChannel);
int get_WirelessTestDiagnostics_Result(char *AllWirelessChannel, char *BestWirelessChannel, char *WirelessChannelNumber, char *WirelessBandwidth, char *WirelessPower, char *QoSType, char *WirelessType);
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_
#define LAN_DEVICE_PKT_LOSS_FILENAME "/tmp/lan_device_pkt_loss"

typedef struct cwmp_msg2ClientData_s {
 long mtype;
 char msgqData[32];
} cwmp_msg2ClientData_t;
#endif

#ifdef CTC_TELNET_SCHEDULED_CLOSE
extern struct callout_s sch_telnet_ch;
void schTelnetCheck(void);
#endif
#endif

#define NULL_FILE 0
#define NULL_STR ""

void updateScheduleCrondFile(char *pathname, int startup);
#ifdef CONFIG_E8B
#define check_and_update_default_route(ifname, ipv, on_off) _check_and_update_default_route_entry(ifname, NULL, ipv, on_off)
void SaveLOIDReg(void);
int applyPortBandWidthControl(void);
int disablePortBandWidthControl(void);
#endif

#ifdef CONFIG_GPON_FEATURE
#ifdef CONFIG_RTK_HOST_SPEEDUP
int routeInternetWanCheck(void);
#endif
#endif

#ifdef CONFIG_USER_WIRED_8021X
extern const char FW_WIRED_8021X_PREROUTING[];
extern const char FW_WIRED_8021X_POSTROUTING[];
extern const char HOSTAPD_SCRIPT_NAME[];
int rtk_wired_8021x_clean(int isPortNeedDownUp);
int rtk_wired_8021x_setup(void);
int rtk_wired_8021x_set_mode(WIRED_8021X_MODE_T mode);
int rtk_wired_8021x_set_enable(unsigned int portmask);
int rtk_wired_8021x_set_auth(int portid, unsigned char *mac);
int rtk_wired_8021x_set_unauth(int portid, unsigned char *mac);
int rtk_wired_8021x_flush_port(int portid);
int rtk_wired_8021x_phyport_linkdown(char *ifName);
int rtk_wired_8021x_take_effect(void);
#endif
#if defined(CONFIG_USER_WIRED_8021X) || defined(CONFIG_USER_HAPD) || defined(WLAN_HAPD)
extern const char HOSTAPD[];
extern const char HOSTAPD_CLI[];
extern const char HOSTAPDPID[];
extern const char HOSTAPDCLIPID[];
#endif
#if defined(CONFIG_USER_WPAS) || defined(WLAN_WPAS)
extern const char WPAS[];
extern const char WPA_CLI[];
extern const char WPASPID[];
extern const char WPACLIPID[];
#endif

int set_ipv4_default_policy_route(MIB_CE_ATM_VC_Tp pEntry, int set);
int set_ipv4_policy_route(MIB_CE_ATM_VC_Tp pEntry, int set);
int set_ipv4_lan_policy_route(char *lan_infname, struct in_addr *ip4addr, unsigned char mask_len, int set);
int setup_policy_route_rule(int add);

#if defined(CONFIG_USER_LAN_BANDWIDTH_CONTROL)
extern const char FW_LANHOST_RATE_LIMIT_PREROUTING[];
extern const char FW_LANHOST_RATE_LIMIT_POSTROUTING[];
#endif

extern void putUShort(unsigned char *obuf, u_int32_t val);

#if (defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
int config_port_pptp_def_rule(int logPortId);
int reset_port_pptp_def_rule(int logPortId);
int reconfig_port_pptp_def_rule(void);
#endif
#ifdef CONFIG_PPPOE_MONITOR
#define MAX_MONITOR_MSG_SIZE  2048
typedef struct pppMonitorMsgInfo {
	int		cmd;
	int		arg1;
	int		arg2;
	char	mtext[MAX_MONITOR_MSG_SIZE];
} PPPMONITOR_MSG_T;

struct pppMonitorMsg {
    long mtype;			// Message type
    long request;		// Request ID/Status code
    long tgid;			// thread group tgid
	PPPMONITOR_MSG_T msg;
};

typedef struct pppoeAccountInfo {
	char pppAccount[64];
	char wanIfName[16];
} PPPACCOUNT_INFO_St;

typedef struct _pppoeSessionInfo_st {
	unsigned char clientAddr[6];
	unsigned char serverAddr[6];
	unsigned int  ipAddr[4];	//only support ipv4 now
	unsigned int  dnsAddr[4];	//only support ipv4 now
	unsigned int  sessionId;
	char pppAccount[64];//username of pppoe wan
	char wanIfName[16];	//base dev of pppoe wan, nas0_X
} PPPoESessionInfo_St;

struct PPPoEMonitorPacket {
    unsigned char srcip[4];
	unsigned short srcport;
	unsigned char protocol;
	unsigned char indev[32];
};

#define PPPMONITOR_MSG_SUCC		0
#define PPPMONITOR_MSG_FAIL		1

#define CMD_PPPOE_SESSION_INFO_GET				0x1
#define CMD_ADD_MONITOR_WAN						0x2
#define CMD_DEL_MONITOR_WAN						0x4
#define CMD_KICK_MONITOR_WAN					0x8
#define CMD_ADD_PPPOE_MONITOR_PACKET			0x10
#define CMD_DEL_PPPOE_MONITOR_PACKET			0x20
#define CMD_FLUSH_PPPOE_MONITOR_PACKET			0x40


int add_pppoe_monitor_wan_interface(const char *ifname);
int del_pppoe_monitor_wan_interface(const char *ifname);
int kick_pppoe_monitor_client(const char *ifname, const char *username);
int cfg_pppoe_monitor_packet(struct PPPoEMonitorPacket *packet_feature, int act);
int get_br_pppoe_session_info(PPPoESessionInfo_St *pppoeInfo, int num);
#endif
#ifdef YUEME_4_0_SPEC
#define APP_EXTRAMIB_SAVE_PATH "/opt/upt/apps/.mib"
#define APP_EXTRAMIB_SAVEFILE APP_EXTRAMIB_SAVE_PATH"/extra_mib.tar.gz"
#define APP_EXTRAMIB_PATH "/var/.extra_mib"
#define APP_EXTRAMIB_PATH_TMP "/var/.extra_mib/tmp"
#define APPROUTE_EXTRAMIB_PATH APP_EXTRAMIB_PATH"/approute_mib"
#define APPROUTE_EXTRAMIB_PATH_TMP APP_EXTRAMIB_PATH_TMP"/approute_mib"
void dbus_do_ClearExtraMIB(void);
void dbus_do_ExtraMIB2flash(void);
void dbus_do_flash2ExtraMIB(void);
void bridgeDataAcl_take_effect(int enable);
int config_one_approute_with_type(int act, APPROUTE_RULE_T *pentry, int type, char **entry);
int config_one_approute(int act, APPROUTE_RULE_T*pentry, int cmode);
int apply_approute(int isBoot);
int check_approute_intf(APPROUTE_RULE_T *pentry, int action);
int update_approute(char *ifname, unsigned int ifi_flags);
int approute_flush_conntrack_mark(void);
#ifdef SUPPORT_APPFILTER
#define APPFILTER_EXTRAMIB_PATH APP_EXTRAMIB_PATH"/appfilter_mib"
#define APPFILTER_EXTRAMIB_PATH_TMP APP_EXTRAMIB_PATH_TMP"/appfilter_mib"
#define APPFILTER_DOMAIN_FILE "/var/appfilter_domain_record"
#define TMP_APPFILTER_DOMAIN_FILE "/var/tmp_appfilter_domain_record"
#define APPFILTER_CHANGE_FILE "/var/appfilter_domain_change"

int getAppFilterTime(MIB_CE_APP_FILTER_Tp pEntry, char *buff);
unsigned int getAppFilterBlockedTimes(int inst_num);
int config_one_appfilter_with_type(int act, MIB_CE_APP_FILTER_T *pentry, int type, char **entry);
int config_one_appfilter(int act, MIB_CE_APP_FILTER_T*pentry, int cmode);
void apply_appfilter(int isBoot);
#endif

int config_one_hwAccCancel(int act, HWACC_CANCEL_T *pentry);
int apply_hwAccCancel(int isBoot);
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
void clean_filesystem(void);
#endif
#if defined(IP_PORT_FILTER) || defined(MAC_FILTER)
void syslogMacFilterEntry(MIB_CE_MAC_FILTER_T Entry, int isAdd, char* user);
void syslogIPPortFilterEntry(MIB_CE_IP_PORT_FILTER_T Entry, int isAdd, char* user);
#endif
#ifdef PORT_FORWARD_GENERAL
void syslogPortFwEntry(MIB_CE_PORT_FW_T Entry, int isAdd, char* user);
#endif
#ifdef DMZ
void syslogDMZ(char enable, char* ipaddr, char* user);
#endif
#ifdef URL_BLOCKING_SUPPORT
void syslogUrlFqdnEntry(MIB_CE_URL_FQDN_T Entry, int isAdd, char* user);
void syslogUrlKeywdEntry(MIB_CE_KEYWD_FILTER_T Entry, int isAdd, char* user);
#endif
#ifdef DOMAIN_BLOCKING_SUPPORT
void syslogDomainBlockEntry(MIB_CE_DOMAIN_BLOCKING_T Entry, int isAdd, char* user);
#endif
#ifdef IP_ACL
void syslogACLEntry(MIB_CE_ACL_IP_T Entry, int isAdd, char* user);
#ifdef CONFIG_IPV6
void syslogACLv6Entry(MIB_CE_V6_ACL_IP_T Entry, int isAdd, char* user);
#endif//CONFIG_IPV6
#endif//IP_ACL

void rtk_util_data_base64decode(unsigned char *input, unsigned char *output, const int outputSize);
void  rtk_util_data_base64encode(unsigned char *input, unsigned char *output, const int outputSize);
void rtk_util_convert_to_star_string(char *star_string, int length);
char * rtk_util_html_encode(const char *s);

#ifdef CONFIG_RTK_SMB_SPEEDUP
int rtk_smb_speedup(void);
#endif
#ifdef CONFIG_USER_RTK_FASTPASSNF_MODULE
int rtk_fastpassnf_smbspeedup(int enable_shortcut);
int rtk_fastpassnf_ipsecspeedup(int enable_shortcut);
#endif
#ifdef CONFIG_USER_RTK_FAST_BR_PASSNF_MODULE
int rtk_fastpassnf_br_ipsecspeedup(int enable_shortcut);
#endif

int change_line_of_file(char *filename, char* old_line_keyWord, char* new_line);

//#ifdef XOR_ENCRYPT
int check_xor_encrypt(char *inputfile);
//#endif

#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
typedef struct urlfilter_urlist
{
	char url[256];
	int port;
	struct urlfilter_urlist *next;
} URLFILTER_URLLIST_T, *URLFILTER_URLLIST_Tp;
int getMinutes(unsigned char hour, unsigned char minute);
int Filter_time_check(int StartTime1, int EndTime1, int StartTime2, int EndTime2, int StartTime3, int EndTime3, unsigned int RepeatDay);
int getUrlFilterTime(MIB_CE_URL_FILTER_Tp pEntry, char *buff);
#ifdef SUPPORT_DNS_FILTER
int getDNSFilterTime(MIB_CE_DNS_FILTER_Tp pEntry, char *buff);
#endif
int isValidTimeFormat(unsigned char start_hr, unsigned char start_min, unsigned char end_hr, unsigned char end_min);
int IsVaildFilterTime(unsigned char startH1, unsigned char startMin1, unsigned char endH1, unsigned char endMin1, unsigned char startH2, unsigned char startMin2, unsigned char endH2, unsigned char endMin2);
int checkURLFilter(MIB_CE_URL_FILTER_T *url_filter);
void free_urlfilter_list(URLFILTER_URLLIST_T *pList);
int check_url_filter_time_schedule(void);
#endif

#ifdef RESTRICT_PROCESS_PERMISSION
void set_otherUser_Capability(uid_t euid);
#endif

#ifdef CONFIG_CU
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
int rtk_fc_add_fastpath_speedup_rule(MIB_CU_FASTPATH_SPEEDUP_RULE_T *entry, int index);
int rtk_fc_del_fastpath_speedup_rule(MIB_CU_FASTPATH_SPEEDUP_RULE_T *entry, int index);
#endif
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
int rtk_cmcc_init_traffic_mirror_rule(void);
int rtk_cmcc_flush_traffic_mirror_rule(void);
int rtk_cmcc_setup_traffic_mirror_rule(MIB_CE_MIRROR_RULE_T *entry, int op);
int rtk_cmcc_add_traffic_mirror_rule(MIB_CE_MIRROR_RULE_T *entry);
int rtk_cmcc_del_traffic_mirror_rule(MIB_CE_MIRROR_RULE_T *entry);
int rtk_cmcc_update_traffic_mirror_rule_url(char*domainName, int containip, char* addr, char* addr6);
#ifdef CONFIG_CMCC_TRAFFIC_PROCESS_RULE_SUPPORT
int rtk_cmcc_init_traffic_process_rule(void);
int rtk_cmcc_flush_traffic_process_rule(void);
int rtk_cmcc_setup_traffic_process_rule(MIB_CMCC_TRAFFIC_PROCESS_RULE_T *entry, int type);
int rtk_cmcc_add_traffic_process_rule(MIB_CMCC_TRAFFIC_PROCESS_RULE_T *entry);
int rtk_cmcc_del_traffic_process_rule(MIB_CMCC_TRAFFIC_PROCESS_RULE_T *entry);
int rtk_cmcc_update_traffic_process_rule_url(char*domainName, int containip, char* addr, char *addr6);
#endif //CONFIG_CMCC_TRAFFIC_PROCESS_RULE_SUPPORT
int rtk_cmcc_update_traffic_monitor_rule_url(char*domainName, int containip, char* addr, char* addr6);
int rtk_cmcc_init_traffic_qos_rule(void);
int rtk_cmcc_flush_traffic_qos_rule(void);
int rtk_cmcc_find_traffic_qos_flowentry(int flow, MIB_TRAFFIC_QOS_RULE_FLOW_Tp pEntry);
int rtk_cmcc_setup_traffic_qos_rule(MIB_TRAFFIC_QOS_RULE_T *entry, MIB_TRAFFIC_QOS_RULE_FLOW_T *flowEntry, int type);
int rtk_cmcc_add_traffic_qos_rule(MIB_TRAFFIC_QOS_RULE_T *entry, MIB_TRAFFIC_QOS_RULE_FLOW_T *flowEntry);
int rtk_cmcc_del_traffic_qos_rule(MIB_TRAFFIC_QOS_RULE_T *entry, MIB_TRAFFIC_QOS_RULE_FLOW_T *flowEntry);
int rtk_cmcc_update_traffic_qos_rule_url(char*domainName, int containip, char* addr, char *addr6);
int rtk_cmcc_get_traffic_qos_share_meter(MIB_TRAFFIC_QOS_RULE_FLOW_T *flowEntry, int direction);
int rtk_cmcc_setup_traffic_qos_share_meter(int flow, unsigned int rate);
int rtk_cmcc_mod_traffic_qos_share_meter(MIB_TRAFFIC_QOS_RULE_FLOW_T *flowEntry);
int rtk_cmcc_flush_traffic_qos_share_meter(MIB_TRAFFIC_QOS_RULE_FLOW_T *flowEntry);
#endif
int getCPUTemperature(int *cpuTemp);
#endif
#if defined(NESSUS_FILTER)
int setup_filter_nessus(void);
#endif

typedef struct
{
	char name[64];
	pid_t pid;
	pid_t ppid;
	char stat;
	char exe[64];
	unsigned char isThread;
	unsigned char isKernel;
} process_pid_info;
int get_process_info_by_pid(pid_t pid, process_pid_info *pInfo);
int find_pidinfo_by_name( char* ProcName, process_pid_info *pArray, int max);

int rtk_qos_total_us_bandwidth_support(void);

int rtk_hwnat_flow_flush(void);

#ifdef _PRMT_X_CMCC_LANINTERFACES_
int setupL2Filter(void);
int setupMACLimit(void);
#ifdef DHCP_ARP_IGMP_RATE_LIMIT
int setupMACBCLimit(void);
#endif
#endif

int rtk_get_interface_packet_statistics(unsigned char *interface_name, unsigned long long *rx_packets, unsigned long long *tx_packets, unsigned long long *rx_bytes, unsigned long long *tx_bytes);
#if defined(CONFIG_YUEME)
int rtk_set_interface_packet_statistics(unsigned char *interface_name, unsigned long long rx_packets, unsigned long long tx_packets, 	unsigned long long rx_bytes, unsigned long long tx_bytes);
int rtk_reset_interface_packet_statistics(unsigned char *interface_name);
int rtk_save_interface_packet_statistics_log_file(unsigned char *interface_name, unsigned long long rx_packets, unsigned long long tx_packets, unsigned long long rx_bytes, unsigned long long tx_bytes);
int rtk_load_interface_packet_statistics_log_file(unsigned char *interface_name, unsigned long long *rx_packets, unsigned long long *tx_packets, unsigned long long *rx_bytes, unsigned long long *tx_bytes);
int rtk_init_interface_packet_statistics_log_file(void);
#endif

#if defined(CONFIG_E8B)
#if defined(CONFIG_PON_CONFIGURATION_COMPLETE_EVENT)
unsigned int get_omci_complete_event_shm( void );
#endif
#if defined(CONFIG_GPON_FEATURE)
void RTK_Gpon_Get_Fec(uint32_t* ds_fec_status, uint32_t* us_fec_status);
#endif //CONFIG_GPON_FEATURE
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
int isLoidNull(void);
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
int getWebLoidPageEnable(void);
void rtk_web_get_commanum_substr(int* comma_num, char* name, unsigned char item[][32]);
int checkIPv4_IPv6_Dual_PolicyRoute_ex(MIB_CE_ATM_VC_Tp pEntry, int *wanIndex, unsigned int *portMask);
#if defined(CONFIG_CMCC_ENTERPRISE) || defined(CONFIG_OTT_GATEWAY_COM)
int	finishWebRedirect(void);
#endif
#endif //defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
int getBitPos(unsigned int bits);
#endif //CONFIG_E8B

#ifdef CONFIG_RTK_CTCAPD_FILTER_SUPPORT
int rtk_check_dns_filter_time_schedule(void);
int rtk_set_dns_filter_rules_for_ubus(void);
int rtk_set_url_filter_iptables_rule_for_ubus(void);
int rtk_restore_ctcapd_filter_state(void);
int rtk_update_url_filter_state(MIB_CE_URL_FILTER_T entry, int rule_add_flag,int url_filter_index);
int rtk_update_dns_filter_state(MIB_CE_CTCAPD_DNS_FILTER_T filter_entry,unsigned int rule_add_flag,int dns_filter_index);
void rtk_set_dns_filter_rule(MIB_CE_CTCAPD_DNS_FILTER_T filter_entry, unsigned int rule_add_flag);
void rtk_set_url_filter_rule(MIB_CE_URL_FILTER_T entry, int rule_add_flag);
int rtk_dns_filter_check_time(MIB_CE_CTCAPD_DNS_FILTER_T filter_entry);
int rtk_set_dns_filter_whitelist_action(int action);
int rtk_ctcapd_set_filter_rule_by_ubus(MIB_CE_URL_FILTER_Tp url_entry,int action);
int rtk_ctcapd_del_filter_rule_by_ubus(int index);
#endif

#ifdef CONFIG_USER_CTCAPD
int rtk_set_wifi_permit_rule(void);
#endif

#ifdef CONFIG_USER_RT_ACL_SYN_FLOOD_PROOF_SUPPORT
int rtk_lan_syn_flood_proof_fw_rules_add(unsigned long lan_ip, unsigned long lan_mask);
int rtk_lan_syn_flood_proof_fw_rules_delete( void );
#endif

struct rtk_qos_classf_rule_entry {
	char profile; // used whan filtering LAN intf range && (useIptables=1 && useEbtables=1) case
	char iptCommand[512];
	char ebtCommand[512];
	char protocol;
	char ipVersion;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	char cmd_with_wan;
#endif
};

#ifdef CONFIG_USER_AP_HW_QOS
int rtk_ap_qos_bandwidth_control(int port, unsigned int bandwidth);
#endif


#ifdef SWAP_WLAN_MIB
typedef struct ConfigPartitionCount{
	int hs_count;
	int cs_count;
	long cs_time;
	long hs_time;
}CPC_info, *pCPC_info;

extern int getConfigPartitionCount(pCPC_info pInfo);
#endif
#ifdef CONFIG_USER_RTK_FASTPASSNF_MODULE
int rtk_switch_fastpass_nf_modules(int switch_flag);
#endif
#ifdef CONFIG_USER_RTK_FAST_BR_PASSNF_MODULE
int rtk_switch_fastpass_br_modules(int switch_flag);
#endif

#ifdef CONFIG_CU
#ifdef CONFIG_USER_CUMANAGEDEAMON
#define CUMANAGE_LOID_FILE		"/tmp/cumanage_loid"
#define CUMANAGE_GUESTSSID_FILE		"/tmp/wlanguestssidrestart"     //set guest ssid restart
#define CUMANAGE_PPPOEACCOUNT_FILE		"/tmp/wanpppoerestart"          // set pppoe for wan restart
#define CUMANAGE_LANPARM_FILE		"/tmp/lanparmrestart"            // set lan parm for dhcp restart
#define CUMANAGE_GUESTSSIDREMAIN_FILE		"/tmp/wlanguestremain"            // set lan parm for dhcp restart
extern const char CUMANAGE_PID[];
extern struct callout_s cuManage_ch;
void cuManageSechedule(void *dummy);
void restart_cumanage(void);

enum
{
	eUserReg_REGISTER_DEFAULT=0,
	eUserReg_REGISTER_REGISTED,
	eUserReg_REGISTER_TIMEOUT,
	eUserReg_REGISTER_NOMATCH_NOLIMITED,
	eUserReg_REGISTER_NOMATCH_LIMITED,
	eUserReg_REGISTER_NOACCOUNT_NOLIMITED,
	eUserReg_REGISTER_NOACCOUNT_LIMITED,
	eUserReg_REGISTER_NOUSER_NOLIMITED,
	eUserReg_REGISTER_NOUSER_LIMITED,
	eUserReg_REGISTER_OLT,
	eUserReg_REGISTER_OLT_FAIL,
	eUserReg_REGISTER_OK_DOWN_BUSINESS,
	eUserReg_REGISTER_OK,
	eUserReg_REGISTER_OK_NOW_REBOOT,
	eUserReg_REGISTER_POK,

	eeUserReg_REGISTER_FAIL,

	eUserReg_End /*last one*/
};
#endif

#if defined(PON_HISTORY_TRAFFIC_MONITOR)
#define PON_TRAFFIC_RECORD		"/tmp/pon_traffic_record"
#define PON_TRAFFIC_RECORD_TMP	"/tmp/pon_traffic_record_tmp"

#define PON_HISTORY_TRAFFIC_MONITOR_INTERVAL 300 //5minutes
#define PON_HISTORY_TRAFFIC_MONITOR_DURATION (72*60*60) // 72 hours
#define PON_HISTORY_TRAFFIC_MONITOR_ITEM_MAX (PON_HISTORY_TRAFFIC_MONITOR_DURATION/PON_HISTORY_TRAFFIC_MONITOR_INTERVAL)

extern struct callout_s pon_traffic_monitor_ch;
void pon_traffic_monitor(void *dummy);
#endif

#define PON_SPPED_RECORD "/tmp/pon_speed_record"

#ifdef _PRMT_C_CU_LOGALARM_
#define ALARM_SERIOUS	1
#define ALARM_MAJOR	2
#define ALARM_MINOR	3

#define ALARM_RECOVER		1
#define ALARM_UNRECOVER	2
#define ALARM_RECOVERED	3

#define ALARM_REBOOT				104001
#define ALARM_PORT_UNAVAILABLE	104006
#define ALARM_WLAN_ERROR			104012
#define ALARM_CPU_OVERLOAD		104030
#define ALARM_ADMINLOGIN_ERROR	104032
#define ALARM_BIT_ERROR			104033
#define ALARM_ENCRYPT_FAIL			104034
#define ALARM_BANDWITH			104035
#define ALARM_FILESERVER_WRONG			104050
#define ALARM_FILESERVER_AUTHERROR		104051
#define ALARM_DOWNLOAD_TIMEOUT			104052
#define ALARM_FILESERVER_NOFILE			104053
#define ALARM_UPGRADECONFIG_FAIL			104054
#define ALARM_BACKUPCONFIG_FAIL			104055
#define ALARM_RESTOREUPCONFIG_FAIL		104056
#define ALARM_NOTAVIL_CONFIG				104057
#define ALARM_UPGRADE_FW_FAIL			104058
#define ALARM_FLASH_NOTENOUGH			104059
#define ALARM_USER_UPGRADE_FW_FAIL		104060
#define ALARM_LOG_UPLOAD_FAIL				104061
#define ALARM_EPG_SERVER_ERROR			104122
#define ALARM_DDNS_SERVER_ERROR			104142
#define ALARM_DDNS_AUTH_FAIL				104143
#define ALARM_VIDEO_DEVICE_NOTSUPPORT			104160

#define LOG_ALARM_PATH "/var/config/log_alarm.txt"
#define TEMP_LOG_ALARM_PATH "/var/config/temp_log_alarm.txt"

typedef struct alarm_record
{
  int	alarmid;
  int	alarmcode;
  int alarmRaisedTime;
  int	cleartime;
  int	alarmstatus;
  int	perceivedseverity;
  char   logStr[64];
}ALARMRECORD_T, *ALARMRECORD_TP;

int syslogAlarm(int , int , int , char *, int );
int getAlarmInfo(ALARMRECORD_TP *info, int *count);
#endif

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
int start_captiveportal(void);
int stop_captiveportal(void);
void enable_http_redirect2CaptivePortalURL(int);
void set_CaptivePortal_Firewall(void);
#endif

#ifdef CONFIG_SUPPORT_PON_LINK_DOWN_PROMPT
int getPONLinkState(void);
#endif

#if defined(CONFIG_CU_BASEON_YUEME)
int delVirtualServerRule(int ifindex);
int getPlatformStatus(char *Srvip, int *port, int *status);
extern struct callout_s framework_ch;
void schFramework_chCheck(void *dummy);
#endif

#endif

#if 1 //def CONFIG_RTK_DEV_AP
int startTimelycheck(void);
#endif

#ifdef CONFIG_USER_LLMNR
int restartLlmnr(void);
#endif

#ifdef CONFIG_USER_MDNS
int restartMdns(void);
#endif
#if defined(CONFIG_CMCC) && defined(CONFIG_SUPPORT_ADAPT_DPI_V3_3_0)
#define WANSE_FILE "/tmp/wan_symbol_error"
#define WANSE_FILE_LOCK "/tmp/wan_symbol_error.lock"
extern struct callout_s wanSE_ch;
void calWanSymbolErrors();
#endif


void decrypt_fwimg(char* filename);
void rtk_util_update_user_account(void);
void rtk_util_update_boa_user_account_ex(int logout_users);
void rtk_util_update_boa_user_account(void);

#if defined(CONFIG_USER_P0F)
typedef struct lannet_devinfo_s
{
	char model[MAX_LANNET_MODEL_NAME_LENGTH];
	char os[MAX_LANNET_OS_NAME_LENGTH];
	char osVer[MAX_LANNET_OS_NAME_LENGTH];
	char swVer[MAX_LANNET_OS_NAME_LENGTH];
	unsigned char devType;
}lannet_devinfo_t;

void rtk_lannetinfo_initial_iptables(void);
void rtk_lannetinfo_set_p0f_iptables(unsigned char* mac, int type);
void rtk_lannetinfo_get_brand(unsigned char *mac, char* brand, int size);
void rtk_lannetinfo_get_os(unsigned int ip, char* os, int size);
void rtk_lannetinfo_get_model(unsigned int ip, char* model, int size);
void rtk_lannetinfo_get_devType(char* brand, char* os, char* model, unsigned char* devType);
void rtk_lannetinfo_device_info_init(unsigned int ip);
void rtk_lannetinfo_get_device_info(unsigned int ip, lannet_devinfo_t *deviceInfo);
#endif

#ifdef RTK_MULTI_AP
void _set_up_backhaul_credentials(MIB_CE_MBSSIB_Tp pEntry);
void check_syslogd_enable(void);
void setupController(char *strVal, char *role_prev, char *controller_backhaul_link, unsigned char *dhcp_mode);
void setupAgent(char *strVal, char *role_prev, char *agent_backhaul_link, unsigned char *dhcp_mode);
#if defined(WLAN_MULTI_AP_ROLE_AUTO_SELECT)
void setupAuto(char *strVal, char *role_prev, char *agent_backhaul_link, unsigned char *dhcp_mode);
#endif
int setupMultiAPController(char *strVal, char *role_prev, char *controller_backhaul_link, unsigned char *dhcp_mode);
void setupMultiAPAgent(char *strVal, char *role_prev, char *agent_backhaul_link, unsigned char *dhcp_mode);
void setupMultiAPDisabled(void);
#ifdef MULTI_AP_OPEN_ENCRYPT_SUPPORT
int setupMultiAPController_encrypt(void);
#endif
#endif

#if defined(CONFIG_GPON_FEATURE)
#define WAN_INFO_FILE "/tmp/wanInfo"
#define MAX_IPV6_NUM 6
typedef struct vlan_info{
	unsigned char enable;
	unsigned short vid;
	unsigned short vprio;	// 802.1p priority bits
	unsigned char vpass;	// vlan passthrough
} vlan_info_t;

#ifdef CONFIG_USER_UDHCP099PRE2
typedef struct dhcpv4_Info{
#ifdef CONFIG_USER_DHCP_OPT_GUI_60
	unsigned char enable_opt_60;
	char opt60_val[40];
	unsigned char enable_opt_61;
	unsigned int iaid;
	unsigned char duid_type;	/* 1 ~ 3 */
	unsigned int duid_ent_num;	/* valid only if type is 2 */
	char duid_id[40]; /* valid only if type is 2 */
	unsigned char enable_opt_125;
	char manufacturer[40];
	char product_class[40];
	char model_name[40];
	char serial_num[40];
#endif
} dhcpv4_Info_t;
#endif

typedef struct ipv4_info{
	unsigned char enable;
	unsigned char isDhcp;
	struct in_addr ipAddr;
	struct in_addr gwAddr;
	struct in_addr mask;
#ifdef CONFIG_USER_UDHCP099PRE2
	dhcpv4_Info_t dhcp_config;
#endif
	char *status;
} ipv4_info_t;

#ifdef CONFIG_IPV6
typedef struct ipv6_info_entry_t{
	struct in6_addr ipAddr;
	unsigned char prefix_len;
	unsigned int preferred_time;
	unsigned int valid_time;
} ipv6_info_entry_t;

#ifdef DHCPV6_ISC_DHCP_4XX
typedef struct dhcpv6_info{
	unsigned char enable;
	ipv6_info_entry_t ia_na;
	ipv6_info_entry_t ia_pd;
	struct in6_addr name_servers;
} dhcpv6_info_t;
#endif

typedef struct ipv6_info{
	unsigned char enable;
	unsigned char MO_bit;	//bit 0: O bit, bit 1: M bit
	unsigned int totalIPv6Num;
	struct ipv6_ifaddr ip6_addr[MAX_IPV6_NUM];
#ifdef DHCPV6_ISC_DHCP_4XX
	IANA_INFO_T ianaInfo;
	DLG_INFO_T dlgInfo;
#endif
	char *status;
} ipv6_info_t;
#endif

typedef struct wan_info{
	unsigned int ifIndex;
	char ifname[IFNAMSIZ];
	unsigned char MacAddr[MAC_ADDR_LEN];
	unsigned char cmode;
	char ipver; // IPv4 or IPv6
	char protocol[10];
	char serviceType[30];
	vlan_info_t vlan;
	ipv4_info_t ipv4;
#ifdef CONFIG_IPV6
	ipv6_info_t ipv6;
#endif
}wan_info_t;

void dumpAllWanToFile(const char* filename);
#endif	// CONFIG_GPON_FEATURE

#if defined(INETD_BIND_IP)
int update_inetd_conf(void);
#endif

#if defined (CONFIG_DM) || defined(CONFIG_USER_DM)
void restart_dm(void);
void stop_dm(void);
#endif
#if defined(CONFIG_USER_LANNETINFO) && defined(CONFIG_USER_REFINE_CHECKALIVE_BY_ARP)
extern int get_lanNetInfo_portSpeed(lan_net_port_speed_T *portSpeed, int num);
#endif
int rtk_is_inforeGroundpgrp(void);
#ifdef CONFIG_USER_P0F
extern void restart_p0f(void);
#endif
#ifdef CONFIG_USER_LANNETINFO
extern void restartlanNetInfo(void);
#endif
#ifdef CONFIG_USER_CTC_EOS_MIDDLEWARE
int start_eos_middleware();
void rtk_sys_restart_elinkclt(int dowait, int debug);
#endif
#if defined(CONFIG_USER_RTK_MULTI_BRIDGE_WAN)
void setBrWangroup(char *list, unsigned char grpnum);
int check_BrWanitfGroup(MIB_CE_BR_WAN_Tp pEntry, MIB_CE_BR_WAN_Tp pOldEntry);
void setupBrWanMap();
int generateBrWanName(MIB_CE_BR_WAN_T *entry, char* wanname);
int getBrWanName(MIB_CE_BR_WAN_T * pEntry, char *name);
int startBrConnection(MIB_CE_BR_WAN_Tp pEntry, int mib_vc_idx);
int startBrConnection(MIB_CE_BR_WAN_Tp pEntry, int mib_vc_idx);
int is_tagged_br_wan_exist(void);
void stopBrWanConnection(MIB_CE_BR_WAN_Tp pEntry);
int deleteBrWanConnection(int configAll, MIB_CE_BR_WAN_Tp pEntry);
void restartBrWAN(int configAll, MIB_CE_BR_WAN_Tp pEntry);
void addEthBrWANdev(MIB_CE_BR_WAN_Tp pEntry);
int startBrWan(int configAll, MIB_CE_BR_WAN_Tp pEntry);
#endif
#if !defined(CONFIG_E8B) && defined(IP_ACL) && defined(ACL_IP_RANGE)
int rtk_modify_ip_range_of_acl_default_rule(void);
#endif
#ifdef CONFIG_USER_AP_BRIDGE_MODE_QOS
int rtk_get_bridge_mode_uplink_port(void);
int rtk_ap_qos_get_wan_port(void);
#endif

#ifdef CONFIG_RTK_GENERIC_NETLINK_API
int rtk_nla_attr_size(int payload);
int rtk_nla_total_size(int payload);
int rtk_genlmsg_open(int *sockfd);
void rtk_genlmsg_close(int sockfd);
void *rtk_genlmsg_alloc(int family_hdr, int payload_size);
int rtk_genlmsg_send(int sockfd, unsigned short nlmsg_type, unsigned int nlmsg_pid,
        unsigned char genl_cmd, unsigned char genl_version, unsigned int family_hdr,
        unsigned short nla_type, const void *nla_data, unsigned int nla_len);
int rtk_genlmsg_recv(int sockfd, unsigned char *buf, unsigned int len);
int rtk_genlmsg_dispatch(struct nlmsghdr *nlmsghdr, unsigned int nlh_len, int nlmsg_type,
						unsigned int family_hdr, int nla_type, unsigned char *buf, int *len);
int rtk_genlmsg_get_family_id(int sockfd, const char *family_name);
int rtk_genlmsg_get_group_id(int sockfd, const char *family_name, const char *grp_name);
#endif	/* CONFIG_RTK_GENERIC_NETLINK_API */

#ifdef CONFIG_USER_LOCK_NET_FEATURE_SUPPORT
int rtk_set_net_locked_rules(unsigned char enabled);
void rtk_clean_net_locked_rule(void);
#define RTK_NET_LOCKED_STATUS_FILE "/tmp/network_locked"
#define RTK_NET_LOCKED_ALL_SERVER_NAME "tmp/netlocked_all_server_name"
#define STATIC_SERVER_NUM 5
#endif
int rtk_get_default_gw_wan_ifname(char *wan_ifname);

#if defined(CONFIG_USER_LANNETINFO) && defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
union mysockaddr {
  struct sockaddr sa;
  struct sockaddr_in in;
#ifdef CONFIG_IPV6
  struct sockaddr_in6 in6;
#endif
};
int rtk_get_manual_wan_ifname_by_lan_ip(union mysockaddr *lan_ip_info, char* wan_itfname);
int rtk_get_auto_wan_ifname_by_lan_ip(union mysockaddr *lan_ip_info, char* wan_itfname);
#endif
#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
int set_manual_load_balance_rules(void);
void rtk_init_manual_firewall(void);
void rtk_del_manual_firewall(void);
void rtk_flush_manual_firewall(void);
int rtk_calc_manual_load_balance_num(char *wan_itf_name);
int rtk_get_wan_itf_by_mark(char *wan_name, unsigned int mark);
int rtk_update_manual_load_balance_info_by_wan_ifIndex(unsigned int old_wan_index, unsigned int new_wan_index,int action);
#endif
void rtk_fw_set_skb_mark_for_dns_pkt(void);

#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
#define MAX_L2_MAC_NUM   1000

#define MAX_IP_NUM     20
#define MAX_MAC_NUM    20
#define MAX_CON_WAN_NUM   5

#define MAX_SAVE_STA_NUM  50
#define SAVE_INFO_NUM	5	 //per sta save 5 info

#define MAC_TYPE 	0
#define IP_TYPE 	1

#define DEL_WAN_ENTRY 0
#define MODIFY_WAN_ENTRY 1

#define CONWANFILE	"/var/con_wan_info"
#define PORTMAPFILE	"/var/port_map_info"

extern const char DYNAMIC_IP_LOAD_BALANCE[];
extern const char DYNAMIC_MAC_LOAD_BALANCE[];
//for bridge wan load-balance ext-rule
extern const char DYNAMIC_INPUT_LOAD_BALANCE[];
extern const char DYNAMIC_FORWARD_LOAD_BALANCE[];

extern uint32_t wan_diff_itfgroup;

typedef struct l2_info_s
{
	unsigned char mac[MAC_ADDR_LEN];
	int port_id;  // phy port
}l2_info_T;

typedef struct mac_info_s
{
	unsigned char mac_str[MAC_ADDR_STRING_LEN];
	unsigned char ifname[IFNAMSIZ];
}mac_info_T;

typedef struct ip_info_s
{
	uint32_t ipAddr;
	unsigned char ifname[IFNAMSIZ];
}ip_info_T;

typedef struct con_wan_info_s
{
	mac_info_T mac_info[MAX_MAC_NUM];
	ip_info_T ip_info[MAX_IP_NUM];
	uint32_t wan_mark;
	unsigned char wan_name[IFNAMSIZ];
	int conn_cnt;
	int adjust_cnt;
}con_wan_info_T;

typedef struct save_wan_info_s
{
	uint32_t save_wan_mark;
	long save_wan_time;
}save_wan_info_T;

typedef struct save_sta_info_s
{
	unsigned char mac_str[MAC_ADDR_STRING_LEN];
	int save_cnt;
	save_wan_info_T save_wan_info[SAVE_INFO_NUM];
}save_sta_info_T;

typedef struct sta_info_s
{
	unsigned char mac_str[MAC_ADDR_STRING_LEN];
	unsigned char sta_ifname[IFNAMSIZ];
	long wan_time;
}sta_info_T;

int rtk_calc_max_divisor(int a,int b);
int rtk_calc_max_divisor_of_multi_number(int number[],int len);
int rtk_calc_wan_weight_by_real_bandwidth(int wan_idx, int wan_bandwidth);
int rtk_get_dynamic_load_balance_enable();
int isZeroMacstr(unsigned char *Mac_str);
int rtk_get_wan_mark_by_wanname(unsigned char *wan_name, unsigned int *wan_mark);
int rtk_get_wan_entry_by_wanmark(MIB_CE_ATM_VC_T           *pEntry, unsigned int wan_mark);
int rtk_get_wan_index_by_wanmark(int *wan_idx, unsigned int wan_mark);
int rtk_get_wan_entry_by_wanvid(int *wan_entry_idx, MIB_CE_ATM_VC_T           *pEntry, unsigned int vid);
void rtk_process_bridge_ext_rules(unsigned int wan_mark, unsigned char *mac, char *ip, int op);
void rtk_update_save_sta_info(unsigned char *mac, unsigned int wan_mark);
void rtk_update_port_map_info(unsigned char *ifname, int wan_info_idx, unsigned char* mac, char *ip, unsigned int wan_mark, int op, int type);
void rtk_update_load_balance_info(unsigned char *ifname, int wan_info_idx, unsigned char* mac, char *ip, unsigned int wan_mark, int op, int type);
void dump_conn_wan_info();
void dump_port_map_info();
int rtk_calc_total_load_balance_expect_cnt(int *cal_expect_cnt);
int rtk_find_ip_by_mac_from_lannetinfo(unsigned char *ip, unsigned char *mac);
void rtk_delete_ct_flow_when_load_balance_change(unsigned char *mac);
void rtk_set_load_balance_mac_iptables_2(unsigned char *ifname, int wan_entry_idx, unsigned char *mac, int wan_mark, int op);
void rtk_set_dynamic_load_balance_adjust_cnt(int wan_info_idx, char *wan_itf_name, int count);
void rtk_adjust_con_stas_to_below_wan(int target_idx, int over_idx);
int rtk_check_sta_is_exist(unsigned char *sta_mac);
void rtk_check_unexist_sta_and_delete_info(void);
void rtk_get_save_wan_time_by_mac_and_wanmark(unsigned char *mac, int wan_mark, long *up_time);
void rtk_move_sta_to_target_wan(unsigned char *mac, unsigned char *sta_ifname, int org_wan_idx, int target_wan_idx);
void rtk_move_sta_by_save_info_time(sta_info_T *tmp_sta_mac_p, int len ,int org_wan_idx);
void rtk_adjust_con_info_by_save_info(void);
void rtk_distribute_global_load_balance(void);
int rtk_check_wan_is_connected(MIB_CE_ATM_VC_Tp pEntry);
int rtk_check_port_mapping_rules_exist(unsigned char *ifname, int *wan_entry_idx, unsigned int *wan_mark);
int rtk_check_load_balance_rules_exist(unsigned char *mac, char *ip);
int rtk_check_port_map_info_exist(unsigned char *mac, char *ip);
int rtk_find_match_wan_mark(int *wan_info_idx, unsigned int *wan_mark);
int writeConWanInfo();
int get_con_wan_info(con_wan_info_T **ppConWandata, unsigned int *pCount);
int writePortMapInfo();
void deleteUnexistStaSaveInfo(void);
int get_port_map_info(con_wan_info_T **ppPortMapdata, unsigned int *pCount);
void rtk_get_port_map_wan_mark_by_mac(unsigned char* mac, int *wan_info_idx, unsigned int *wan_mark);
void rtk_get_load_balance_wan_mark_by_mac(unsigned char* mac, int *wan_info_idx, unsigned int *wan_mark);
void rtk_get_load_balance_wan_mark_by_ip(char *ip, unsigned int *wan_mark);
void rtk_set_load_balance_mac_iptables(unsigned char *ifname, unsigned char *mac, int op);
int get_eth_lan_mac_by_l2(l2_info_T *pHwL2Info, int array_num, int *real_num);
int rtk_find_save_sta_info_wan_mark(int *wan_info_idx, unsigned int *wan_mark, unsigned char *mac);
void rtk_process_rules_when_wan_connect();
void rtk_process_rules_when_wan_link_down(unsigned int wan_mark);
int setup_load_balance_rules_when_link_change(char *ifname, unsigned int ifi_flags);
#endif

#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
int getEthTypeNumb(void);
void getEthTypes(unsigned short *ethtypes, int numb);
int getDefaultWanPort(void);
typedef struct qos_atm_wanitf {
	char name[IFNAMSIZ];
	unsigned char cmode;
	unsigned char enable_qos;
} QOS_ATM_WANITF_T, *QOS_ATM_WANITF_Tp;
typedef struct qos_wan_port_wanitf {
	QOS_ATM_WANITF_T itf[MAX_VC_NUM];
} QOS_WAN_PORT_WANITF_T, *QOS_WAN_PORT_WANITF_Tp;
#endif
#ifdef CONFIG_USB_SUPPORT
void resetUsbDevice(void);
#endif
#ifdef CONFIG_USER_PPPD
int generate_pppd_core_script(void);
#ifdef CONFIG_USER_PPPOMODEM
int setup_ppp_modem_conf(MIB_WAN_3G_T *p, unsigned char pppidx, char *ttyUSB);
#endif
#endif
#ifdef CONFIG_USER_PPPOMODEM
MIB_WAN_3G_T *getPoMEntryByIfname(char *ifname, MIB_WAN_3G_T *p, int *entryIndex);
#endif
void escape(char* s,char* t);
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
void startUserReg(unsigned char *loid, unsigned char *password);
#endif

#if defined(CONFIG_CMCC_WIFI_PORTAL)
enum{
	TMP_ACCESS_DISABLE,
	TMP_ACCESS_ALLOW_ALL,
	TMP_ACCESS_ALLOW_LIST_ONLY,
};

int killWiFiDog(void);
void startWiFiDog(void *null);
int restartWiFiDog(int restart);
extern struct callout_s wifiAuth_ch;
extern int g_wan_modify;
void wifiAuthCheck(void * null);
#define WA_MAX_WAN_NAME 33
#define WIFIDOGPATH "/bin/wifidog"
#define WIFIDOG_PID_FILE "/var/run/wifidog.pid"
#define WIFIDOGCONFPATH "/etc/wifidog/wifidog.conf"
#define KILLWIFIDOGSTR "killall wifidog"

#define PORTALSERVICE_REGISTER_BUNDLE "/var/bundle/event/UNAUTHORIZED_DEV_ONLINE"
#define PORTALSERVICE_REGISTER_BUNDLE_LOCK "/var/bundle/event/UNAUTHORIZED_DEV_ONLINE.lock"
#define PORTALSERVICE_REGISTER_BUNDLE_TMP "/var/bundle/event/UNAUTHORIZED_DEV_ONLINE_tmp"
void checkMacWhiteList(void);
void mac_whitelist_rule_del(MIB_AWIFI_MACFILTER_T entry);
void dumpPortalMacWhiteList(void);
void dumpaccMode(void);
int addPortalMacWhiteList(char *macaddr);
int deletePortalMacWhiteList(char *macaddr);
int clearPortalMacWhiteList(void);
int allowDeviceTemporaryAccess(char *macaddr);
int setPortalDomainWriteList(char *urlstr);
int setTemporaryAccessMode(int mode, int expireTime);
int Wifidog_mac_whitelist_clear(void);
int clearRGPortalServer(void);
int get_assoc_ssid_index_by_mac(unsigned char *mac_addr_str);
int getIsAuth(void);
void clearWhiteListMib(void);
int setupTrustedMacConf(void);
int setupMacWhitelist(void);
int setupTrustedUrlConf(void);
#endif
#ifdef CONFIG_CMCC_AUDIT
typedef struct wifi_portal_info_s{
	int				auth_mode;
	char     		account[16];
	int 			ip_type;
	char 			ip_local[32];
	char			macAddr[18];
	unsigned int	nat_port;
	char			username[32];
	int				id_type;
	char			id_num[32];
	char			nation[32];
	int				card_type;
	char			card_num[32];
	char			phone_num[32];
	char			imei[32];
	char			imsi[64];
	char			terminal_system[24];
	char			terminal_brand[24];
	char			terminal_brandtype[24];
	char			room[16];
	char			ap_mac[32];
	char			x_point[32];
	char			y_point[32];
} wifi_portal_info_t;
#endif

#if defined(CONFIG_USER_USP_TR369)
int rtk_tr369_mtp_get_entry_by_instnum(MIB_TR369_LOCALAGENT_MTP_T *pentry, unsigned int instnum, unsigned int *chainid);
unsigned int rtk_tr369_mtp_get_new_instnum(void);
int rtk_tr369_controller_get_entry_by_instnum(MIB_TR369_LOCALAGENT_CONTROLLER_T *pentry, unsigned int instnum, unsigned int *chainid);
unsigned int rtk_tr369_controller_get_new_instnum(void);
int rtk_tr369_controller_mtp_get_entry_by_instnum(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_T *pentry, unsigned int instnum1, unsigned int instnum2, unsigned int *chainid);
unsigned int rtk_tr369_controller_mtp_get_new_instnum(unsigned int controller_instnum);
int rtk_tr369_stomp_get_entry_by_instnum(MIB_TR369_STOMP_CONNECTION_T *pentry, unsigned int instnum, unsigned int *chainid);
unsigned int rtk_tr369_stomp_get_new_instnum(void);
int rtk_tr369_mqtt_client_get_entry_by_instnum(MIB_TR369_MQTT_CLIENT_T *pentry, unsigned int instnum, unsigned int *chainid);
unsigned int rtk_tr369_mqtt_client_get_new_instnum(void);
int rtk_tr369_mqtt_client_subscription_get_entry_by_instnum(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_T *pentry, unsigned int instnum1, unsigned int instnum2, unsigned int *chainid);
unsigned int rtk_tr369_mqtt_client_subscription_get_new_instnum(unsigned int client_instnum);
int rtk_tr369_get_ifname(char *ifname);
void rtk_tr369_restart_obuspa(void);
#endif

#ifdef CONFIG_USER_L2TPD_LNS
void l2tp_lns_take_effect(void);
#endif
#ifdef CONFIG_USER_PPTP_SERVER
void pptp_server_take_effect(void);
extern void getPptpServerLocalIP(char *ipaddr);
extern void getPptpServerSubnet(char *str_subnet);
extern void setup_pptp_server_firewall(PPTPS_RULE_TYPE_T type);
#endif
#if defined (CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_L2TPD_LNS)
int set_LAN_VPN_accept(char *chain);
#endif
#ifdef CONFIG_USER_IP_QOS
int getIPQosQueueNumber(void);
#endif
#if defined(CONFIG_GPON_FEATURE)
int getWanInfoByIfIndex(unsigned int ifIndex, wan_info_t *p);
#endif
#if defined(CONFIG_PPP) && defined(CONFIG_USER_PPPD)
void WRITE_PPP_FILE(int fh, unsigned char *buf);
void rtk_create_ppp_secrets(void);
#endif

#ifdef CONFIG_USER_ANDLINK_FST_BOOT_MONITOR
#define RTL_FST_BOOT_MONITOR "/bin/fstBootMonitor"
#define RTL_FST_BOOT_MONITOR_RUNFILE "/var/run/fstBootMonitor.pid"
#define RTL_FST_BOOT_MONITOR_USOCKET "/var/run/fstBootMonitor.usock"
#define RTL_FST_BOOT_MONITOR_TMP "/var/run/fstBootMonitor.tmp"
#define RTL_FST_BOOT_MONITOR_MSG_SIZE 32
#define RTL_FST_BOOT_MONITOR_CNT_NO_REPLY 5

void start_fstBootMonitor();
void notify_fstBootMonitor(char *ifName, int cnt, int status);
#endif

MIB_CE_ATM_VC_T* getDefaultRouteATMVCEntry(MIB_CE_ATM_VC_T *pEntry);
#ifdef _PRMT_X_CT_COM_NEIGHBORDETECTION_
#define NEIGHDETECT_IPOEDOWN_INFORM_FILE "/tmp/neighdetect_ipoedown_inform"
struct neighdetect_list{
	unsigned char valid;
	unsigned int ifindex;
	unsigned char detecttype;
	unsigned char ipaddr[IP_ADDR_LEN];
	unsigned char ip6addr[IP6_ADDR_LEN];
	unsigned int detect_interval;
	unsigned int detect_count;
	long lastdetect_time;
	unsigned char state;//bit 1:ipv6 unreachable,bit 0:ipv4 unreachable
};
extern struct callout_s neighdetect_ch;
void neighDetectCheck(void *dummy);
int parse_neighdetect_result(char *nd_ifname,char* ipaddr,char* ip6addr,char* prefix);
int reset_ipoewan(unsigned int ifindex, unsigned char state);
#endif
#if defined(SUPPORT_NON_SESSION_IPOE) && defined(_PRMT_X_CT_COM_DHCP_)
int parse_dhcp_server_auth_opt_serverid(char *buf, int buflen, char *enc_key, char *serverid);
int get_dhcp_reply_auth_opt_serverid(char *out, int len, char *serverid, char *enc_key);
#endif
#ifdef CONFIG_USER_ANDLINK_PLUGIN
void stop_andlink_plugin();
void start_andlink_plugin(void);
#endif
#ifdef CONFIG_USER_GREEN_PLUGIN
void stop_secRouter_plugin();
void start_secRouter_plugin(void);
void clear_secRouter_plugin_dir(void);
#endif
#ifdef CONFIG_USER_OPENSSL
int do_3des(const unsigned char *key, const unsigned char *in, unsigned char *out, size_t len, int enc_act);
#endif

#if 0 //use AppCtrlNF instead of dnsmonitor def CONFIG_USER_DNS_MONITOR
enum{
	DNSMONITOR_ACT_ADD=1,
	DNSMONITOR_ACT_DEL=2,
};
struct dnsmonitor_msg{
	int act;
	int arg_len;
	unsigned char arg[0];
};
#define DNSMONITOR_MSG_PATH "/var/run/dnsmonitor.msg"
int config_domainlist_to_dnsmonitor(int act, char*domainlist, int instnum);
int send_cmd_to_dnsmonitor(struct dnsmonitor_msg*arg, int msg_len);
#endif

int getMacFromIp(void *ipaddr, int family, unsigned char *macaddr);
int getHostNameByMac(unsigned char *macAddress, char *hostName);

int power(int base, int exponent);
int hex_to_int(char *hex);
#if (defined(CONFIG_USER_MAP_E) || defined(CONFIG_USER_MAP_T))
int get_map_info(int map_type, char *str, S46_DHCP_INFO_Tp pInfo);
#endif
#ifdef CONFIG_USER_LW4O6
int get_lw4o6_info(char *str, S46_DHCP_INFO_Tp pInfo);
#endif
#ifdef CONFIG_USER_QUAGGA
int rtk_stop_igp(unsigned char igpType);
int rtk_start_igp(void);
void rtk_restart_igp(void);
#endif

#if defined(CONFIG_USER_PORT_ISOLATION)
void rtk_lan_clear_lan_port_isolation(void);
int rtk_lan_handle_lan_port_isolation(void);
#endif

#ifdef CONFIG_SECONDARY_IP
int rtk_set_secondary_ip(void);
#endif
void calPasswd(char *pass, char *crypt_pass);
void rtk_get_crypt_salt(char *salt, unsigned int len);
int ValidatePINCode_s(char *);
int CheckPINCode_s(char *);
int changeMacFormatWlan(char *str, char s);

//#ifdef COM_CUC_IGD1_AUTODIAG
enum eAUTO_DIAG_RESULT
{
	ERR_PON_LOS = 1,
	ERR_ONU_LASER_FORCE_ON,
	ERR_REG_FAIL,
	ERR_REG_TIMEOUT,
	ERR_PPPOE_AUTH_FAIL,
	ERR_PPPOE_AUTH_TIMEOUT,
	ERR_PPPOE_ACQUIRE_IP_FAIL,
	INIT_FAULT_CODE = 254,
	ERR_AUTODIAG_NONE = 255
};
void setAutoDiagResult(unsigned char result);
unsigned char getAutoDiagResult(void);
int needAutoDiagRedirect(void);
void checkAutoDiagPPPoEResult(void);
int is_laser_force_on(void);
int is_pon_los(void);
int is_optical_fiber_match_with_pon_mode(void);
unsigned char get_oam_max_state(void);
void clear_oam_max_state(void);
void set_oam_max_state(unsigned char oam_state);
int is_loid_reg_timeout(void);
int is_olt_register_ok(void);
//#endif
#define BRCTL_GET_FDB_ENTRIES 18
int get_host_connected_interface(const char *brname, unsigned char *macaddr, char *ifname);
void rtk_send_signal_to_daemon(char *pidfile, int sig);
int rtk_util_write_str(const char *path, int wait, char *content);
#if defined(DHCP_ARP_IGMP_RATE_LIMIT)
int initDhcpIgmpArpLimit(void);
#endif
long convert_timeStr_to_epoch(int year, int mon,int day, int hour, int min, int sec);
char * string_skip_space(char*str);
#ifdef COM_CUC_IGD1_WMMConfiguration
typedef struct wifi_qos_mapping_entry {
	unsigned char prior;
	unsigned char wifiQueueIndex;
} WIFI_QOS_MAPPING_T, *WIFI_QOS_MAPPING_Tp;

int setup_WmmService(MIB_CE_WMM_SERVICE_T *poldWmmRule, MIB_CE_WMM_SERVICE_T *pWmmRule, int chainid, int addflag);
void setup_wmmservice_rules();
void setup_WmmRule_startup();
int getIPQosQueueNumber();
#endif
#ifdef SUPPORT_TIME_SCHEDULE
//every bit reprents an filter entry index, so max supported filter number is size*32
#define SCHD_MAC_FILTER_SIZE 4
#define SCHD_URL_FILTER_SIZE 4
#define SCHD_IP_FILTER_SIZE  4
#define SCHD_IP6_FILTER_SIZE 4
#define SCHEDULE_NOW_FILE "/tmp/schedule_now"
int check_should_schedule(struct tm * pTime, MIB_CE_TIME_SCHEDULE_T * pEntry);
void addScheduledIndex(int index, unsigned int * bitmap, int size);
void delScheduledIndex(int index, unsigned int * bitmap, int size);
int isScheduledIndex(int index,unsigned int * bitmap, int size);
void firewall_schedule();
#endif

#ifdef COM_CUC_IGD1_WMMConfiguration_ipset
typedef struct wifi_qos_mapping_entry {
	unsigned char prior;
	unsigned char wifiQueueIndex;
} WIFI_QOS_MAPPING_T, *WIFI_QOS_MAPPING_Tp;

void setup_wmmservice_rules();
int getIPQosQueueNumber();
#endif

#ifdef CONFIG_E8B
#if defined(WIFI_TIMER_SCHEDULE) || defined(LED_TIMER) || defined(SLEEP_TIMER) || defined(SUPPORT_TIME_SCHEDULE)
int _crond_check_current_state(int startHour, int startMin, int endHour, int endMin);
void updateScheduleCrondFile_time_change(int signum);
#endif
#endif

unsigned char is_interface_valid(char *interface);

#ifdef DELAY_WRITE_SYSLOG_TO_FLASH
int force_save_syslog(void);
#endif

#ifdef CONFIG_USER_CUSPEEDTEST
int skipThisOptmize(unsigned char step);
unsigned int get_customer_optimize_value();
#endif

unsigned int get_column_content
(
    char *content, char *value,
    int value_maxlen,
    int column
);

unsigned int if_nametoindex(const char *ifname);

int caculate_tblid(uint32_t ifid);
int caculate_tblid_by_ifname(char *ifname);

#ifdef CONFIG_CU_BASEON_YUEME
#define PC_POOL_EXIST 0x01
#define CAMERA_POOL_EXIST 0x02
#define STB_POOL_EXIST 0x04
#define PHONE_POOL_EXIST 0x08
#define WLANAP_POOL_EXIST 0x10


int CheckPoolExist();
#endif
#ifdef AUTO_SAVE_ALARM_CODE
int auto_save_alarm_code_add(int code , char *info);
#define AUTOSAVE_ALARMCODE_FILE "/tmp/auto_save_alarm_code"
#define AUTOSAVE_ALARMCODE_FILE_TMP "/tmp/auto_save_alarm_code_tmp"
#define AUTOSAVE_ALARMCODE_LOCK "/var/run/autosave_alarmcode_lock"
#endif
#if defined(CONFIG_CMCC)
#define STB_IGMP_WHITE_LIST_FILE "/var/stb_igmp_white_list"
#define STB_IGMP_WHITE_LIST_FILE_LOCK "/var/stb_igmp_white_list_lock"
#define IGMP_WHITE_LIST_CTRL_FILE "/proc/igmp/ctrl/whiteList"
#define DEFAULT_INIT_STB_WHITE_LIST_MAC {0,0,0,0,0,0}
#define MAX_STB_IGMP_WHITE_LIST_NUM	32
typedef struct stb_igmp_white_list
{
	char ifname[IFNAMSIZ];
	unsigned char mac[MAC_ADDR_LEN];
	long uptime;
}STB_IGMP_WHITE_LIST_T,*STB_IGMP_WHITE_LIST_Tp;
#endif
#ifdef CONFIG_WAN_INTERVAL_TRAFFIC
void wanIntervalTraffic();
void *start_wanIntervalTraffic(void *data);
#endif
#ifdef _PRMT_X_CMCC_AUTORESETCONFIG_
void auto_reset_rule_init(void);
void auto_reset_rule_check(void);
#endif
int getDialStatus();

#if defined(CONFIG_CMCC)
int add_stb_white_list(char ifname[IFNAMSIZ], unsigned char mac[MAC_ADDR_LEN]);
int del_stb_white_list(unsigned char mac[MAC_ADDR_LEN]);
int init_stb_igmp_white_list();
int flush_stb_igmp_white_list();
#endif

#if CONFIG_E8B
#if defined(CONFIG_USER_RTK_VOIP) || defined(SUPPORT_CLOUD_VR_SERVICE) || defined(CONFIG_CMCC)
void output_hex(const char *prefix, const unsigned char *data, size_t size);
void get_key(const unsigned char *in, const unsigned char *R, const unsigned char *TS, unsigned char *out);
int get_decrypt_text(char *user, char *R, char *TS, char *buf,int type_des);
#endif
#endif
#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
#define FWUPGRADE_SET "fwupgrade_set"
#define FWUPGRADE_SETV6 "fwupgrade_set6"
#define FWUPGRADE_UPDATE "/var/run/.fwupgrade"
int check_fwupgrade_phase(void);
void update_fwupgrade_phase(int phase);
#endif

#define ACC_INTER   "/tmp/access_interface"
#define ACC_MAC     "/tmp/access_mac"
#define ACC_PORTNUM "/tmp/access_portno"

#ifdef CONFIG_IPV6
int compare_prefixip_with_unspecip_by_bit(unsigned char *prefixIP, unsigned char *unspecIP, unsigned char prefixLen);
#endif

extern void mac_autobinding_del_mac_rule(unsigned char *mac);
extern void mac_autobinding_del_ra_client(unsigned char *ip6addr);

#ifdef CONFIG_VIR_BOA_JEMBENCHTEST
int get_jspeed_via_boa(void);
void get_jembenchtest_base(JEMBENCHTEST_BASE_Tp pJbase);
#endif

#ifdef USE_DEONET /* sghong, 20221123 */
#define FC_OFF                 0
#define FC_SYMM                1
#define FC_ASYMM               2
#define FC_BOTH                3

#define SKB_DEF_DNS1		"219.250.36.130"
#define SKB_DEF_DNS2		"210.220.163.82"

#define MAX_LINE 1024
#define DNS_FILE_NAME "/var/udhcpd/udhcpd.conf"
#define TEMP_DNS_FILE_NAME "/var/udhcpd/temp_udhcpd.conf"

extern const char FW_BR_PRIVATE_DHCP_SERVER_INPUT[];
extern const char FW_BR_DHCPRELAY6_INPUT[];
extern const char FW_BR_DHCPRELAY6_FORWARD[];
extern const char FW_BR_DHCPRELAY6_OUTPUT[];
extern const char FW_BR_DHCPRELAY_INPUT[];
extern const char FW_BR_DHCPRELAY_FORWARD[];
extern const char FW_BR_DHCPRELAY_OUTPUT[];

extern const char FW_BR_LANTOLAN_DSCP_BROUTE[];
extern const char FW_BR_OLTTOLAN_BROUTE[];
extern const char FW_BR_OLTTOLAN_FORWARD[];
extern const char FW_BR_ICMPV6_TX_MANGLE_OUTPUT[];
extern const char FW_BR_ICMPV6_TX_MANGLE_POSTROUTING[];
extern const char FW_BR_ICMPV6_TX_OUTPUT[];
extern const char SKB_ACL[];
extern const char CPU_RATE_ACL[];
extern const char RATE_ACL[];
extern const char UPNP_ACL[];

int deo_port_autonego_fc_set(int port, int fc);
int startBridgeIP_v4(char *inf, MIB_CE_ATM_VC_Tp pEntry, CHANNEL_MODE_T ipEncap,
						int *isSyslog, char*msg);
int deo_ntp_is_sync(void);
void get_ipv4_from_interface(char *buf, int type);
void get_ipv4_from_dropbear(char *buf);
void get_mac_from_interface(char *name, char *buf);
int deo_dnsModifyUpdate(char *dns1, char *dns2);
int deo_dns_auto_set(int enable);
void deo_dns_mode_get(int *mode);
void deo_dns_fixed_value_get(int *mode, char *dns1, char *dns2);
void deo_dns_fixed_addr_mode_get(int *mode);
int deo_udhcpd_conf_update(char *dns1, char *dns2);
int deo_accesslimit_set(void);
int deo_accesslimit_iptables_flush(void);
void accesslimit_add_iptables_set(MIB_ACCESSLIMIT_T *Entry, char *br_ip);
int getIfAddr(const char *devname, char* ip);
int convert_weekdays(char *str, char *convert);
int convert_hours(int start, int end, int *start1, int *end1, int *start2, int *end2);
int convert_mins(int start_min, int end_min, int *from_hour, int *to_hour, int *from1_min, int *to1_min, int *from2_min, int *to2_min);
void deo_IfMtuSet(char *ifname, int mtu);
void deo_dns_value_get(char *dns1, char *dns2);
void deo_dns_addr_value_get(unsigned char *dns1, unsigned char *dns2);
void deo_resolv_value_update(char *ifname, char *resolv_fname);
int get_ConntrackHwCnt(void);
int get_ConntrackCnt(void);
void starup(char *str);
int igmp_restart_get_mode(void);

// for keytable
int keytable_write(char *filename, char *data);
char *keytable_read_file(char *filename);
char *keytable_get_decrypted_content(char *filename);
void keytable_xor_crypt(char *data, size_t length);
int keytable_encrypt_file(char *filename);
int keytable_decrypt_file(char *filename);
void stop_keyupdate();
#endif

/* ********************************************
 * protocol entryption 
 * ********************************************/
#define PROTOCOL_ENCRYPT_VERSION_LEN 4
#define PROTOCOL_ENCRYPT_IV_LEN 12
#define PROTOCOL_ENCRYPT_KEY_LEN 32
#define PROTOCOL_ENCRYPT_DATA_LEN (65535 - PROTOCOL_ENCRYPT_IV_LEN - PROTOCOL_ENCRYPT_VERSION_LEN)
struct encrypt_packet_data{
	unsigned char version[PROTOCOL_ENCRYPT_VERSION_LEN];
	unsigned char ivData[PROTOCOL_ENCRYPT_IV_LEN];
	unsigned char data[PROTOCOL_ENCRYPT_DATA_LEN];
};
int get_encrypt_aes256key(unsigned char *key, int size);


#define MAX_TEXT_SIZE 8188
#define QUEUE_NAME "/trap_queue"

typedef struct {
	int  trap_number;          
	char text[MAX_TEXT_SIZE];
} MyMessage;
/* ********************************************
 * protocol entryption end
 * ********************************************/

/* ********************************************
 * protocol entryption SM 
 * *********************************************/
#include <stddef.h>
typedef enum {
	KEY_EXCHANGE_INIT    = 0,
	KEY_EXCHANGE_SUCCESS = 1,
	KEY_EXCHANGE_FAIL    = 2
} KeyExchangeST;

typedef enum {
	SM_INIT,
	SM_FAIL_ENCRYPT,
	SM_FAIL_NORMAL,
	SM_SUCCESS
} SMState;

typedef struct {
	int event_id;
	void *event_data;
	size_t data_size;
} SMEvent;

typedef struct _KeyExchangeSM KeyExchangeSM;

typedef struct {
	// INIT 상태에서 대기 중, 혹은 최초 세팅 시 필요한 초기화 로직
	void (*onEnterInit)(KeyExchangeSM *sm);

	// FAIL_ENCRYPT 상태 진입 시 처리할 로직
	void (*onEnterFailEncrypt)(KeyExchangeSM *sm);

	// FAIL_NORMAL 상태 진입 시 처리할 로직
	void (*onEnterFailNormal)(KeyExchangeSM *sm);

	// SUCCESS 상태 진입 시 처리할 로직
	void (*onEnterSuccess)(KeyExchangeSM *sm);

	// 상태 내부에서 이벤트를 처리하는 로직
	void (*processEventInCurrentState)(KeyExchangeSM *sm, const SMEvent *evt);
} KeyExchangeCallbacks;

typedef struct _KeyExchangeSM {
	SMState currentState;            // 현재 내부 상태
	KeyExchangeST currentKeyExST;    // 현재 KEY_EXCHANGE_ST (init=0, success=1, fail=2)

	char isFirstSuccess;             // 최초 성공 여부
	char hasDefaultKey;              // default 키 보유 여부(예: fail encrypt 진입 여부 결정 시)
#define SEC_ONLY_NORMAL 0
#define SEC_ENCRYPT_NORMAL 1
#define SEC_ONLY_ENCRYPT 2
	char secMode;					// 0 : only normal, 1 : encrypt,normal 2: only encrypt

	char ldapCnt;

	// 등록된 콜백들
	KeyExchangeCallbacks callbacks;
} KeyExchangeSM;


// Key Exchange SM을 초기화
void KeyExchangeSM_Init(KeyExchangeSM *sm,
		const KeyExchangeCallbacks *callbacks,
		char hasDefaultKey,
		char secMode);

// MIB에서 KEY_EXCHANGE_ST가 변할 때마다 이 함수를 호출하여 상태 전이 수행
void KeyExchangeSM_UpdateKeyExchangeST(KeyExchangeSM *sm, KeyExchangeST newST);

// 큐에서 이벤트가 들어올 때마다 호출
void KeyExchangeSM_ProcessEvent(KeyExchangeSM *sm, const SMEvent *evt);

/* ********************************************
 * protocol entryption SM end
 * ********************************************/

#endif // INCLUDE_UTILITY_H

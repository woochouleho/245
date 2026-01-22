/*
 *      Utiltiy function to communicate with TCPIP stuffs
 *
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *
 *      $Id: utility.c,v 1.845 2013/01/25 07:31:11 kaohj Exp $
 *
 */

/*-- System inlcude files --*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <ctype.h>
#include <err.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <libgen.h>
#include <string.h>
#include <poll.h>

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
#include <sys/resource.h>
#include <mqueue.h>

#include <netinet/if_ether.h>
#include <linux/sockios.h>
#include <linux/unistd.h>
#include <linux/capability.h>
#ifdef EMBED
#include <linux/config.h>
#endif

#include <sys/time.h> //added for gettimeofday
#include <sys/timerfd.h>

/*-- Local include files --*/
#include "mib.h"
#include "utility.h"
#include "adsl_drv.h"
#include "subr_net.h"
#include "subr_cu.h"

/* for open(), lseek() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fnmatch.h>

#include "debug.h"

#include <linux/wireless.h>
#ifdef CONFIG_ATM_CLIP
#include <linux/atmclip.h>
#include <linux/atmarp.h>
#endif

#include <linux/if_bridge.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
// Kaohj -- get_net_link_status() and get_net_link_info()
#include <linux/ethtool.h>
#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#include "fc_api.h"
#endif

#if defined(CONFIG_GPON_FEATURE)
#include "rtk/gpon.h"
#endif

#if defined(CONFIG_RTL8686) || defined(CONFIG_RTL9607C) || defined(CONFIG_LUNA)
#include "rtk/ponmac.h"
#include "rtk/gpon.h"
#include "rtk/epon.h"
#include "rtk/switch.h"
#if defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_epon.h>
#include <rtk/rt/rt_port.h>
#include <rtk/rt/rt_switch.h>
#endif
#endif

#ifdef CONFIG_HWNAT
#include "hwnat_ioctl.h"
#endif

#ifdef SUPPORT_FON_GRE
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include "rtl_types.h"
#endif

#ifdef CONFIG_DEV_xDSL
#include "subr_dsl.h"
#endif

#define PR_VC_START	1
#define	PR_PPP_START	16

#if defined CONFIG_IPV6
#include "ipv6_info.h"
#endif

#ifdef REMOTE_HTTP_ACCESS_AUTO_TIMEOUT
time_t remote_user_auto_logout_time;
#endif

#if defined(CONFIG_ELINKSDK_SUPPORT)
#include "../../../lib_elinksdk/libelinksdk.h"
//#include "libelinksdk.h"
#endif

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_stat.h>
#include "rt_stat_ext.h"
#endif

#include "sockmark_define.h"

#ifdef CONFIG_MGTS
#include "subr_mgts.h"
#else
#include "subr_generic.h"
#endif
#include "defs.h"
#include "form_src/multilang.h"

#ifdef RTK_MULTI_AP
#include "subr_multiap.h"
#endif
#ifdef CONFIG_CT_AWIFI_JITUAN_SMARTWIFI
#include "../../ctcyueme/awifi/awifi.h"
int g_wan_modify=0;
#endif
#include "subr_switch.h"

#ifdef CONFIG_USER_CONF_ON_XMLFILE
const char CONF_ON_XMLFILE[] = "/var/config/config.xml";
const char CONF_ON_XMLFILE_HS[] = "/var/config/config_hs.xml";
#ifdef CONFIG_USER_XML_BACKUP
const char CONF_ON_XMLFILE_BAK[] = "/var/config/config_bak.xml";
const char CONF_ON_XMLFILE_HS_BAK[] = "/var/config/config_hs_bak.xml";
#endif
#endif

#if defined(CONFIG_CTC_SDN)
#include "subr_ctcsdn.h"
#endif
#include <crypt.h>

#ifdef CONFIG_TR142_MODULE
#include <rtk/rtk_tr142.h>
#endif

#include <sys/stat.h>
#include <ifaddrs.h>
#include "deo_timer.h"

#define PROTO_ARP 0x0806
#define ETH2_HEADER_LEN 14
#define HW_TYPE 1
#define MAC_LENGTH 6
#define IPV4_LENGTH 4
#define ARP_REQUEST 0x01
#define COUNT_CHECK_LOOP        5
#define TIME_LOOP_WAIT          200000  /* 200ms */

struct arp_header
{
	unsigned short hardware_type;
	unsigned short protocol_type;
	unsigned char hardware_len;
	unsigned char  protocol_len;
	unsigned short opcode;
	unsigned char sender_mac[MAC_LENGTH];
	unsigned char sender_ip[IPV4_LENGTH];
	unsigned char target_mac[MAC_LENGTH];
	unsigned char target_ip[IPV4_LENGTH];
};

#define NULL_FILE 0
#define NULL_STR ""
const char ALL_LANIF[] = "br+";
const char LANIF[] = "br0";
const char LOCALHOST[] = "lo";
const char LAN_ALIAS[] = "br0:0";	// alias for secondary IP
#ifdef IP_PASSTHROUGH
const char LAN_IPPT[] = "br0:1";	// alias for IP passthrough
#endif
const char LANIF_ALL[] = "br+"; //for multiple bridge
#if defined(CONFIG_SECONDARY_IP) && defined(WLAN_QTN)
const char LAN_RGMII[] = "br0:2"; // alias for rgmii port
#endif
const char ELANIF[] = "eth0";
#ifdef CONFIG_RTL_MULTI_LAN_DEV
#if defined(CONFIG_RTL_SMUX_DEV)
const char* ELANRIF[] = {ALIASNAME_ELAN0, ALIASNAME_ELAN1, ALIASNAME_ELAN2, ALIASNAME_ELAN3, ALIASNAME_ELAN4, ALIASNAME_ELAN5, ALIASNAME_ELAN6, ALIASNAME_ELAN7, ALIASNAME_ELAN8, ALIASNAME_ELAN9, ALIASNAME_ELAN10};
const char* ELANVIF[] = {ALIASNAME_VELAN0, ALIASNAME_VELAN1, ALIASNAME_VELAN2, ALIASNAME_VELAN3, ALIASNAME_VELAN4, ALIASNAME_VELAN5, ALIASNAME_VELAN6, ALIASNAME_VELAN7, ALIASNAME_VELAN8, ALIASNAME_VELAN9, ALIASNAME_VELAN10};
const char* SW_LAN_PORT_IF[] = {ALIASNAME_VELAN0, ALIASNAME_VELAN1, ALIASNAME_VELAN2, ALIASNAME_VELAN3, ALIASNAME_VELAN4, ALIASNAME_VELAN5, ALIASNAME_VELAN6, ALIASNAME_VELAN7, ALIASNAME_VELAN8, ALIASNAME_VELAN9, ALIASNAME_VELAN10}; // used for iptables
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
const char* EXTELANRIF[] = {ALIASNAME_EXTELAN0, ALIASNAME_EXTELAN1, ALIASNAME_EXTELAN2, ALIASNAME_EXTELAN3, ALIASNAME_EXTELAN4, ALIASNAME_EXTELAN5, ALIASNAME_EXTELAN6, ALIASNAME_EXTELAN7, ALIASNAME_EXTELAN8, ALIASNAME_EXTELAN9, ALIASNAME_EXTELAN10};
#endif
#else
const char* ELANRIF[] = {ALIASNAME_ELAN0, ALIASNAME_ELAN1, ALIASNAME_ELAN2, ALIASNAME_ELAN3, ALIASNAME_ELAN4, ALIASNAME_ELAN5, ALIASNAME_ELAN6, ALIASNAME_ELAN7, ALIASNAME_ELAN8, ALIASNAME_ELAN9, ALIASNAME_ELAN10};
const char* ELANVIF[] = {ALIASNAME_ELAN0, ALIASNAME_ELAN1, ALIASNAME_ELAN2, ALIASNAME_ELAN3, ALIASNAME_ELAN4, ALIASNAME_ELAN5, ALIASNAME_ELAN6, ALIASNAME_ELAN7, ALIASNAME_ELAN8, ALIASNAME_ELAN9, ALIASNAME_ELAN10};
const char* SW_LAN_PORT_IF[] = {ALIASNAME_ELAN0, ALIASNAME_ELAN1, ALIASNAME_ELAN2, ALIASNAME_ELAN3, ALIASNAME_ELAN4, ALIASNAME_ELAN5, ALIASNAME_ELAN6, ALIASNAME_ELAN7, ALIASNAME_ELAN8, ALIASNAME_ELAN9, ALIASNAME_ELAN10}; // used for iptables
#endif
#else
const char* ELANRIF[] = {"eth0"};
const char* ELANVIF[] = {"eth0"};
const char* SW_LAN_PORT_IF[] = {ALIASNAME_ELAN0, ALIASNAME_ELAN1, ALIASNAME_ELAN2, ALIASNAME_ELAN3, ALIASNAME_ELAN4, ALIASNAME_ELAN5, ALIASNAME_ELAN6, ALIASNAME_ELAN7, ALIASNAME_ELAN8, ALIASNAME_ELAN9, ALIASNAME_ELAN10}; // used for iptables
#endif
const char BRIF[] = "br0";
const char NATIF[] = "nas0_0";
#if defined(CONFIG_RTL8681_PTM) && !defined(CONFIG_RTL867X_PACKET_PROCESSOR)
const char PTMIF[] = "ptm0";
#else
const char PTMIF[] = "ptm0_pp";
#endif
#ifdef CONFIG_NET_IPGRE
const char* GREIF[] = {"gret0", "gret1"};
#endif
#ifdef CONFIG_USB_ETH
const char USBETHIF[] = "usb0";
#endif //CONFIG_USB_ETH
#ifdef CONFIG_IPV6
const char TUN6RD_IF[]= "tun6rd";
const char TUN6IN4_IF[] = "tun6in4";
const char TUN6TO4_IF[] = "tun6to4";
#endif

//the name of wlan
#if defined(TRIBAND_SUPPORT)
const char*  wlan[]   = {"wlan0", "wlan0-vap0", "wlan0-vap1", "wlan0-vap2", "wlan0-vap3",
						 "wlan1", "wlan1-vap0", "wlan1-vap1", "wlan1-vap2", "wlan1-vap3",
						 "wlan2", "wlan2-vap0", "wlan2-vap1", "wlan2-vap2", "wlan2-vap3",
						 ""};
const char*  wlan_itf[] = {"WLAN0", "WLAN0-AP1", "WLAN0-AP2", "WLAN0-AP3", "WLAN0-AP4",
						   "WLAN1", "WLAN1-AP1", "WLAN1-AP2", "WLAN1-AP3", "WLAN1-AP4",
						   "WLAN2", "WLAN2-AP1", "WLAN2-AP2", "WLAN2-AP3", "WLAN2-AP4",
						   ""};

#elif defined(WLAN_8_SSID_SUPPORT)
#ifdef WLAN_SWAP_INTERFACE
#define WLAN0IF	"wlan1"
#define WLAN1IF	"wlan0"
#else
#define WLAN0IF	"wlan0"
#define WLAN1IF	"wlan1"
#endif
const char*  wlan[]   = {
#ifndef WLAN_USE_VAP_AS_SSID1
	WLAN0IF,
#endif
	WLAN0IF"-vap0", WLAN0IF"-vap1", WLAN0IF"-vap2", WLAN0IF"-vap3", WLAN0IF"-vap4", WLAN0IF"-vap5", WLAN0IF"-vap6",
#ifdef WLAN_USE_VAP_AS_SSID1
	WLAN0IF,
#endif
#ifndef WLAN_USE_VAP_AS_SSID1
	WLAN1IF,
#endif
	WLAN1IF"-vap0", WLAN1IF"-vap1", WLAN1IF"-vap2", WLAN1IF"-vap3", WLAN1IF"-vap4", WLAN1IF"-vap5", WLAN1IF"-vap6",
#ifdef WLAN_USE_VAP_AS_SSID1
	WLAN1IF,
#endif
	""};
const char*  wlan_itf[] = {"WLAN0", "WLAN0-AP1", "WLAN0-AP2", "WLAN0-AP3", "WLAN0-AP4", "WLAN0-AP5", "WLAN0-AP6", "WLAN0-AP7",
						   "WLAN1", "WLAN1-AP1", "WLAN1-AP2", "WLAN1-AP3", "WLAN1-AP4", "WLAN1-AP5", "WLAN1-AP6", "WLAN1-AP7", ""};
#else
#ifdef WLAN_SWAP_INTERFACE
#define WLAN0IF	"wlan1"
#define WLAN1IF "wlan0"
#else
#define WLAN0IF	"wlan0"
#define WLAN1IF "wlan1"
#endif
const char*  wlan[]   = {
#ifndef WLAN_USE_VAP_AS_SSID1
	WLAN0IF,
#endif
	WLAN0IF"-vap0", WLAN0IF"-vap1", WLAN0IF"-vap2", WLAN0IF"-vap3",
#ifdef WLAN_USE_VAP_AS_SSID1
	WLAN0IF,
#endif
#ifndef WLAN_USE_VAP_AS_SSID1
	WLAN1IF,
#endif
	WLAN1IF"-vap0", WLAN1IF"-vap1", WLAN1IF"-vap2", WLAN1IF"-vap3",
#ifdef WLAN_USE_VAP_AS_SSID1
	WLAN1IF,
#endif
	""};
const char*  wlan_itf[] = {"WLAN0", "WLAN0-AP1", "WLAN0-AP2", "WLAN0-AP3", "WLAN0-AP4",
						   "WLAN1", "WLAN1-AP1", "WLAN1-AP2", "WLAN1-AP3", "WLAN1-AP4", ""};

#endif
#ifdef WLAN_SUPPORT
#if defined(WLAN_8_SSID_SUPPORT)
const int wlan_en[] = {1 //wlan0
#ifdef WLAN_MBSSID
	,1, 1, 1, 1, 1, 1, 1 //wlan0-vap0 ~ wlan0-vap6
#else
	,0, 0, 0, 0, 0, 0, 0
#endif
#if defined(WLAN_DUALBAND_CONCURRENT) && !defined(WLAN1_QTN)
	,1 //wlan1
#ifdef WLAN_MBSSID
	,1, 1, 1, 1, 1, 1, 1 //wlan1-vap0 ~ wlan1-vap6
#else
	,0, 0, 0, 0, 0, 0, 0
#endif
#else
	,0, 0, 0, 0, 0, 0, 0, 0
#endif
};
#else
const int wlan_en[] = {1 //wlan0
#ifdef WLAN_MBSSID
#if WLAN_MBSSID_NUM == 3
	,1, 1, 1, 0 //wlan0-vap0 ~ wlan0-vap3
#else
	,1, 1, 1, 1 //wlan0-vap0 ~ wlan0-vap3
#endif
#else
	,0, 0, 0, 0
#endif
#if defined(WLAN_DUALBAND_CONCURRENT) && !defined(WLAN1_QTN)
	,1 //wlan1
#ifdef WLAN_MBSSID
#if WLAN_MBSSID_NUM == 3
	,1, 1, 1, 0 //wlan1-vap0 ~ wlan1-vap3
#else
	,1, 1, 1, 1 //wlan1-vap0 ~ wlan1-vap3
#endif
#else
	,0, 0, 0, 0
#endif
#else
	,0, 0, 0, 0, 0
#endif
#if defined(TRIBAND_SUPPORT)
	,1 //wlan2
#ifdef WLAN_MBSSID
	,1, 1, 1, 1 //wlan1-vap0 ~ wlan1-vap3
#else
	,0, 0, 0, 0
#endif
#else
	,0, 0, 0, 0, 0
#endif /* defined(TRIBAND_SUPPORT) */
};
#endif
#else
const int wlan_en[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif
const char PORT_DNS[] = "53";
const char PORT_DHCP[] = "67";
const char BLANK[] = "";
const char ARG_ADD[] = "add";
const char ARG_REPLACE[] = "replace";
const char ARG_CHANGE[] = "change";
const char ARG_DEL[] = "del";
const char ARG_ENCAPS[] = "encaps";
const char ARG_QOS[] = "qos";
const char ARG_255x4[] = "255.255.255.255";
const char ARG_0x4[] = "0.0.0.0";
const char ARG_BKG[] = "&";
const char ARG_I[] = "-i";
const char ARG_O[] = "-o";
const char ARG_T[] = "-t";
const char ARG_TCP[] = "TCP";
const char ARG_UDP[] = "UDP";
const char ARG_NO[] = "!";
const char ARG_ICMP[] = "ICMP";
const char ARG_IPV4[] = "IPV4";
const char ARG_IPV6[] = "IPV6";
const char ARG_ARP[] = "ARP";
const char ARG_PPPOE[] = "PPPOE";
const char ARG_WAKELAN[] = "WAKELAN";
const char ARG_APPLETALK[] = "APPLETALK";
const char ARG_PROFINET[] = "PROFINET";
const char ARG_FLOWCONTROL[] = "FLOWCONTROL";
const char ARG_IPX[] = "IPX";
const char ARG_MPLS_UNI[] = "MPLSUNI";
const char ARG_MPLS_MULTI[] = "MPLSMULTI";
const char ARG_802_1X[] = "802.1X";
const char ARG_LLDP[] = "LLDP";
const char ARG_RARP[] = "RARP";
const char ARG_NETBIOS[] = "NETBIOS";
const char ARG_X25[] = "X.25";
const char ARG_NONE[] = "-";
const char FW_BLOCK[] = "block";
const char FW_INACC[] = "inacc";
const char FW_ICMP_FILTER[] = "icmp_filter";
const char BIN_IP[] = "/usr/bin/ip";
const char BIN_BRIDGE[] = "/bin/bridge";
const char ITFGRP_RTTBL[] = "itfgrp_rt_tbl";
const char PORTMAP_RTTBL[] = "portmapping_rt_tbl";
const char PORTMAP_EBTBL[] = "portmapping";
const char PORTMAP_INTBL[] = "portmapping_input";
const char PORTMAP_OUTTBL[] = "portmapping_output";
const char PORTMAP_SEVTBL[] = "portmapping_service";
const char PORTMAP_BRTBL[] = "portmapping_broute";
const char WAN_OUTPUT_RTTBL[] = "wan_output_rt_tbl";
#ifdef CONFIG_E8B
const char PORTMAP_SEVTBL2[] = "portmapping_service2";
#ifdef CONFIG_CU
const char PORTMAP_SEVTBL_RT[] = "portmapping_service_rt";
const char PORTMAP_SEVTBL_RT2[] = "portmapping_service_rt2";
#endif
#endif
#if (defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
const char L2_PPTP_PORT_SERVICE[] = "L2_PPTP_PORT_SERVICE";
const char L2_PPTP_PORT_INPUT_TBL[] = "L2_PPTP_PORT_INPUT_TBL";
const char L2_PPTP_PORT_OUTPUT_TBL[] = "L2_PPTP_PORT_OUTPUT_TBL";
const char L2_PPTP_PORT_FORWARD_TBL[] = "L2_PPTP_PORT_FORWARD_TBL";
const char L2_PPTP_PORT_FORWARD[] = "L2_PPTP_PORT_FORWARD";
#endif
const char FW_ISOLATION_MAP[] = "ISOLATION_MAP";
const char FW_ISOLATION_BRPORT[] = "ISOLATION_BRPORT";
#ifdef CONFIG_YUEME
const char FRAMEWORK_ADDR_MAP[] = "framework_addr_map";
#endif
const char NAT_ADDRESS_MAP_TBL[] = "address_map_tbl";

const char FW_ACL[] = "aclblock";
const char FW_DROPLIST[] = "droplist";
const char FW_CWMP_ACL[] = "cwmp_aclblock";
const char FW_DHCP_PORT_FILTER[] = "dhcp_port_filter";
#ifdef IP_PORT_FILTER
const char FW_IPFILTER_IN[] = "ipfilter_in";
const char FW_IPFILTER_OUT[] = "ipfilter_out";
const char FW_IPFILTER[] = "ipfilter";
#endif

#if defined(CONFIG_CU_BASEON_YUEME) && defined(COM_CUC_IGD1_ParentalControlConfig)
const char FW_PARENTAL_MAC_CTRL_LOCALIN[] ="parentalctrl_MACfilter_Localin";
const char FW_PARENTAL_MAC_CTRL_FWD[] ="parentalctrl_MACfilter_Localfwd";
#endif
#ifdef CONFIG_SUPPORT_CAPTIVE_PORTAL
const char FW_CAPTIVEPORTAL_FILTER[] = "captiveportal_filter";
#endif

#ifdef CONFIG_USER_LOCK_NET_FEATURE_SUPPORT
const char NETLOCK_INPUT[] = "NetLock_Input";
const char NETLOCK_FWD[] = "NetLock_Fwd";
const char NETLOCK_OUTPUT[] = "NetLock_Output";
const char BR_NETLOCK_INPUT[] = "Br_NetLock_Input";
const char BR_NETLOCK_FWD[] = "Br_NetLock_Fwd";
const char BR_NETLOCK_OUTPUT[] = "Br_NetLock_Output";
#endif
#ifdef CONFIG_USER_RTK_VOIP
const char IPTABLE_VOIP[] = "voip_chain";
const char VOIP_REMARKING_CHAIN[] = "voip_remarking";
#endif
#if (defined(IP_PORT_FILTER) || defined(DMZ)) && defined(CONFIG_TELMEX_DEV)
const char FW_DMZFILTER[] = "dmz_patch";
#endif
const char FW_PARENTAL_CTRL[] ="parental_ctrl";
const char FW_BR_WAN[] = "br_wan";
const char FW_BR_WAN_FORWARD[] = "br_wan_forward";
const char FW_BR_LAN_FORWARD[] = "br_lan_forward";
const char FW_BR_WAN_OUT[] = "br_wan_out";
const char FW_BR_LAN_OUT[] = "br_lan_out";
const char FW_BR_PPPOE[] = "br_pppoe";
const char FW_BR_PPPOE_ACL[] = "br_pppoe_acl";
#ifdef MAC_FILTER
const char FW_MACFILTER_FWD[] = "macfilter_forward";
const char FW_MACFILTER_LI[] = "macfilter_local_in";
const char FW_MACFILTER_LO[] = "macfilter_local_out";
const char FW_MACFILTER_MC[] = "macfilter_mc";
#if defined(CONFIG_00R0) && defined(CONFIG_USER_INTERFACE_GROUPING)
const char FW_INTERFACE_GROUP_DROP_IGMP[] = "if_group_drop_igmp";
const char FW_INTERFACE_GROUP_DROP_FORWARD[] = "if_group_drop_forward";
#endif
#ifdef CONFIG_E8B
const char FW_MACFILTER_FWD_L3[] = "macfilter_forward_l3";
#endif
#endif
#ifdef CONFIG_USER_VLAN_ON_LAN
const char BR_VLAN_ON_LAN[] = "vlan_on_lan";
#endif
#ifdef CONFIG_DEV_xDSL
#ifdef CONFIG_USER_IGMPPROXY
const char IGMP_OUTPUT_VPRIO_RESET[] = "IGMP_OUTPUT_VPRIO_RESET";
#endif
#if (defined(CONFIG_IPV6) && defined(CONFIG_USER_MLDPROXY))
const char MLD_OUTPUT_VPRIO_RESET[] = "MLD_OUTPUT_VPRIO_RESET";
#endif
#endif
const char FW_APPFILTER[] = "appfilter";
const char FW_APP_P2PFILTER[] = "appp2pfilter";
const char FW_IPQ_MANGLE_DFT[] = "m_ipq_dft";
const char FW_IPQ_MANGLE_USER[] = "m_ipq_user";
const char FW_DSCP_REMARK[]="dscp_remark";
#ifdef PORT_FORWARD_ADVANCE
const char FW_PPTP[] = "pptp";
const char FW_L2TP[] = "l2tp";
#endif
#ifdef CONFIG_USER_IGMPTR247
const char FW_TR247_MULTICAST[] = "tr247_multicast";
const char FW_US_IGMP_RATE_LIMIT[] = "us_igmp_rate_limit";
#endif
#ifdef PORT_FORWARD_GENERAL
const char PORT_FW[] = "portfw";
#endif

#if defined(PORT_FORWARD_GENERAL) || defined(DMZ)
#ifdef NAT_LOOPBACK
const char PORT_FW_PRE_NAT_LB[] = "portfwPreNatLB";
const char PORT_FW_POST_NAT_LB[] = "portfwPostNatLB";
#ifdef CONFIG_NETFILTER_XT_TARGET_NOTRACK
const char PF_RAW_TRACK_NAT_LB[] = "rawTrackNatLB_PF";
const char DMZ_RAW_TRACK_NAT_LB[] = "rawTrackNatLB_DMZ";
#endif
#endif /* NAT_LOOPBACK */
#endif /* PORT_FORWARD_GENERAL || DMZ */

#ifdef VIRTUAL_SERVER_SUPPORT
#ifdef NAT_LOOPBACK
const char VIR_SER_PRE_NAT_LB[] = "virserverPreNatLB";
const char VIR_SER_POST_NAT_LB[] = "virserverPostNatLB";
#endif
#endif
#if defined(CONFIG_USER_PPTP_SERVER)
const char IPTABLE_PPTP_VPN_IN[] = "pptp_vpn_input";
const char IPTABLE_PPTP_VPN_FW[] = "pptp_vpn_fw";
#endif
#if defined(CONFIG_USER_L2TPD_LNS)
const char IPTABLE_L2TP_VPN_IN[] = "l2tp_vpn_input";
const char IPTABLE_L2TP_VPN_FW[] = "l2tp_vpn_fw";
#endif
#ifdef DMZ
#ifdef NAT_LOOPBACK
const char IPTABLE_DMZ_PRE_NAT_LB[] = "dmzPreNatLB";
const char IPTABLE_DMZ_POST_NAT_LB[] = "dmzPostNatLB";
#endif
const char IPTABLE_DMZ[] = "dmz";
#endif
#ifdef CONFIG_USER_AVALANCH_DETECT
const char IPTABLE_AVALANCH_DETECT[] = "AVALANCH_DETECT";
#endif
#ifdef NATIP_FORWARDING
const char IPTABLE_IPFW[] = "ipfw";
const char IPTABLE_IPFW2[] = "ipfw_PostRouting";
#endif
#if defined(PORT_TRIGGERING_STATIC) || defined(PORT_TRIGGERING_DYNAMIC)
const char IPTABLES_PORTTRIGGER[] = "portTrigger";
#endif
#if defined(CONFIG_USER_RTK_NAT_ALG_PASS_THROUGH) || (defined(CONFIG_TRUE) && defined(CONFIG_IP_NF_ALG_ONOFF))
const char FW_VPNPASSTHRU[] = "vpnPassThrough";
#endif
#ifdef CONFIG_USER_WIRED_8021X
const char FW_WIRED_8021X_PREROUTING[] = "wired_8021x_preroute";
const char FW_WIRED_8021X_POSTROUTING[] = "wired_8021x_postroute";
const char HOSTAPD_SCRIPT_NAME[] = "/var/hostapd";
#endif
#if defined(CONFIG_USER_WIRED_8021X) || defined(CONFIG_USER_HAPD) || defined(WLAN_HAPD)
const char HOSTAPD[] = "/bin/hostapd";
const char HOSTAPD_CLI[] = "/bin/hostapd_cli";
const char HOSTAPDPID[] = "/var/run/hostapd.pid";
const char HOSTAPDCLIPID[] = "/var/run/hostapd_cli.pid";
#endif
#if defined(CONFIG_USER_WPAS) || defined(WLAN_WPAS)
const char WPAS[] = "/bin/wpa_supplicant";
const char WPA_CLI[] = "/bin/wpa_cli";
const char WPASPID[] = "/var/run/wpas.pid";
const char WPACLIPID[] = "/var/run/wpa_cli.pid";
#endif
#if defined(CONFIG_USER_LAN_BANDWIDTH_CONTROL)
const char FW_LANHOST_RATE_LIMIT_PREROUTING[] = "lanhost_ratelimit_preroute";
const char FW_LANHOST_RATE_LIMIT_POSTROUTING[] = "lanhost_ratelimit_postroute";
#endif
//#if defined(CONFIG_USER_BOA_PRO_PASSTHROUGH) && defined(CONFIG_RTK_DEV_AP)
#ifdef CONFIG_USER_BOA_PRO_PASSTHROUGH
const char IPSEC_PASSTHROUGH_FORWARD[] = "IPSEC_PassThrough_Forward";
const char L2TP_PASSTHROUGH_FORWARD[] = "L2TP_PassThrough_Forward";
const char PPTP_PASSTHROUGH_FORWARD[] = "PPTP_PassThrough_Forward";
#endif
//#endif
const char PORT_IPSEC[] = "500";
const char PORT_IPSEC_NAT[] = "4500";
const char PORT_L2TP[] = "1701";
const char PORT_PPTP[] = "1723";
const char IPTABLE_TR069[] = "tr069";
#ifdef CONFIG_IPV6
const char IPTABLE_IPV6_TR069[] = "ipv6_tr069";
const char IPTABLE_IPV6_MAC_AUTOBINDING_RS[] = "mac_autobinding_rs";
const char IPV6_RA_LO_FILTER[] = "ipv6_ra_local_out";
#endif
#if defined(CONFIG_USER_DATA_ELEMENTS_AGENT) || defined(CONFIG_USER_DATA_ELEMENTS_COLLECTOR)
const char IPTABLE_DATA_ELEMENT[] = "data_element";
#ifdef CONFIG_IPV6
const char IP6TABLE_DATA_ELEMENT[] = "ipv6_data_element";
#endif
#endif
#ifdef CONFIG_USER_GREEN_PLUGIN
const char SELF_CONTROL_CHAIN_NAME[] = "self_control";
#ifdef CONFIG_IPV6
const char SELF_CONTROL6_CHAIN_NAME[] = "self_control6";
#endif
#endif
#ifdef CONFIG_GUEST_ACCESS_SUPPORT
const char GUEST_ACCESS_CHAIN_EB[] = "guest_access";
const char GUEST_ACCESS_CHAIN_IP[] = "guest_access";
const char GUEST_ACCESS_CHAIN_IPV6[] = "guest_access";
#endif
#ifdef CONFIG_USER_CTCAPD
const char WIFI_PERMIT_IN_TBL[] = "wifi_permit_in";
const char WIFI_PERMIT_FWD_TBL[] = "wifi_permit_fwd";
#endif
#ifdef CONFIG_USER_RT_ACL_SYN_FLOOD_PROOF_SUPPORT
const char FW_LAN_SYN_FLOOD_PROOF[] = "LAN_SYN_FLOOD_PROOF";
#endif
#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
const char FW_CONTROL_PACKET_PROTECTION[] = "control_packet_protection";
#endif
const char FW_TCP_SMALLACK_PROTECTION[] = "TCP_SMALLACK_PROTECT";
const char FW_DROP[] = "DROP";
const char FW_ACCEPT[] = "ACCEPT";
const char FW_RETURN[] = "RETURN";
const char FW_REJECT[] = "REJECT";
const char FW_FORWARD[] = "FORWARD";
const char FW_INPUT[] = "INPUT";
const char FW_OUTPUT[] = "OUTPUT";
const char FW_PREROUTING[] = "PREROUTING";
const char FW_POSTROUTING[] = "POSTROUTING";
const char FW_DPORT[] = "--dport";
const char FW_SPORT[] = "--sport";
const char FW_ADD[] = "-A";
const char FW_DEL[] = "-D";
const char FW_FLUSH[] = "-F";
const char FW_INSERT[] = "-I";
const char CONFIG_HEADER[] = "<Config_Information_File_8671>";
const char CONFIG_TRAILER[] = "</Config_Information_File_8671>";
const char CONFIG_HEADER_HS[] = "<Config_Information_File_HS>";
const char CONFIG_TRAILER_HS[] = "</Config_Information_File_HS>";
const char CONFIG_XMLFILE[] = "/tmp/config.xml";
const char CONFIG_RAWFILE[] = "/tmp/config.bin";
const char CONFIG_XMLFILE_HS[] = "/tmp/config_hs.xml";
const char CONFIG_RAWFILE_HS[] = "/tmp/config_hs.bin";
const char CONFIG_XMLENC[] = "/tmp/config.enc";
const char PPP_SYSLOG[] = "/tmp/ppp_syslog";
const char PPP_DEBUG_LOG[] = "/tmp/ppp_debug_log";

const char ADSLCTRL[] = "/bin/adslctrl";
const char IFCONFIG[] = "/bin/ifconfig";
const char BRCTL[] = "/bin/brctl";
const char MPOAD[] = "/bin/mpoad";
const char MPOACTL[] = "/bin/mpoactl";
const char DHCPD[] = "/bin/udhcpd";
const char DHCPC[] = "/bin/udhcpc";
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ)
const char DNSRELAY[] = "/bin/dnsmasq";
const char DNSRELAYPID[] = "/var/run/dnsmasq.pid";
const char DNSRELAY_PROCESS[] = "dnsmasq";
#endif

#ifdef CONFIG_USER_CTC_EOS_MIDDLEWARE
const char RTK_RESTART_ELINKCLT_FILE[] = "/var/rtk_restart_elinkclt_file";
#endif

#ifdef CONFIG_USER_CONNTRACK_TOOLS
const char CONNTRACK_TOOL[] = "/bin/conntrack";
#endif
const char SMUXCTL[] = "/bin/smuxctl";
const char WEBSERVER[] = "/bin/boa";
const char SNMPD[] = "/bin/snmpd";
const char ROUTE[] = "/bin/route";
const char IPTABLES[] = "/bin/iptables";
const char IPSET[] = "/bin/ipset";
const char IPSET_COMMAND_RESTORE[] = "restore";
const char IPSET_SETTYPE_HASHIP[] = "hash:ip";
const char IPSET_SETTYPE_HASHIPV6[] = "hash:ip family inet6";
const char IPSET_SETTYPE_HASHNET[] = "hash:net";
const char IPSET_SETTYPE_HASHNETV6[] = "hash:net family inet6";
const char IPSET_SETTYPE_HASHMAC[] = "hash:mac";
const char IPSET_OPTION_COMMENT[] = "comment";
const char IPSET_OPTION_COUNTERS[] = "counters";
const char ARP_MONITOR[] = "/bin/arp_monitor";
#ifdef CONFIG_USER_DUMP_CLTS_INFO_ON_BR_MODE
const char GET_BR_CLIENT[] = "/bin/cltsOnBRMode";
#endif
const char EMPTY_MAC[MAC_ADDR_LEN] = {0};
/*ql 20081114 START need ebtables support*/
const char EBTABLES[] = "/bin/ebtables";
const char ZEBRA[] = "/bin/zebra";
const char RIPD[] = "/bin/ripd";
#ifdef CONFIG_USER_QUAGGA_RIPNGD
const char RIPNGD[] = "/bin/ripngd";
#endif
#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
const char OSPFD[] = "/bin/ospfd";
#endif
#ifdef CONFIG_USER_QUAGGA
const char OSPFD[] = "/bin/ospfd";
const char BGPD[] = "/bin/bgpd";
const char ISISD[] = "/bin/isisd";
#endif
const char ROUTED[] = "/bin/routed";
const char IGMPROXY[] = "/bin/igmpproxy";
#ifdef CONFIG_USER_IGMPPROXY_SUPPORT_CFG_FILE
const char IGMPPROXY_CONF[]="/var/igmp_config";
#endif
#ifdef CONFIG_USER_MLDPROXY
const char MLDPROXY[] = "/bin/mldproxy";		// Mason Yu. MLD Proxy
const char MLDPROXY_CONF[]="/var/mld_config";
#endif
#if defined(CONFIG_RTL867X_NETLOG) && defined(CONFIG_USER_NETLOGGER_SUPPORT)
const char NETLOGGER[]="/bin/netlogger";
#endif
const char TC[] = "/bin/tc";
const char STARTUP[] = "/bin/startup";
#ifdef TIME_ZONE
const char SNTPC[] = "/bin/vsntp";
const char SNTPCV6[] = "/bin/sntpd";
const char SNTPC_PID[] = "/var/run/vsntp.pid";
const char SNTPCV6_PID[] = "/var/run/sntpv6.pid";
#endif
#ifdef CONFIG_USER_LANNETINFO
const char LANNETINFO[] = "/bin/lanNetInfo";
#ifdef CONFIG_USER_HAPD
const char LANNETINFO_ACTION[] = "/bin/lanNetInfo_action";
#endif
const char FW_HOST_MIB_PREROUTING[] = "host_mib_preroute";
const char FW_HOST_MIB_POSTROUTING[] = "host_mib_postroute";
#endif
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
const char MANUAL_LOAD_BALANCE_PREROUTING[] = "Manual_Ld_Prerouting";
const char MANUAL_LOAD_BALANCE_INPUT[]= "Manual_Ld_Input";
const char MANUAL_LOAD_BALANCE_FORWARD[]= "Manual_Ld_Forward";
const char MANUAL_LOAD_BALANCE_OUTPUT[]= "Manual_Ld_Output";
///////////////dynamic//////////////
const char DYNAMIC_IP_LOAD_BALANCE[] = "dynamic_ip_load_balance";
const char DYNAMIC_MAC_LOAD_BALANCE[] = "dynamic_mac_load_balance";
//for bridge wan load-balance ext-rule
const char DYNAMIC_INPUT_LOAD_BALANCE[] = "dynamic_input_load_balance";
const char DYNAMIC_FORWARD_LOAD_BALANCE[] = "dynamic_forward_load_balance";
uint32_t wan_diff_itfgroup = 0;
#endif
#ifdef CONFIG_USER_DDNS
const char DDNSC_PID[] = "/var/run/updatedd.pid";
#endif
const char ROUTED_PID[] = "/var/run/routed.pid";
const char BOA_PID[] = "/var/run/boa.pid";
#if defined(CONFIG_USER_ZEBRA_OSPFD_OSPFD)
const char ZEBRA_PID[] = "/var/run/zebra.pid";
#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
const char OSPFD_PID[] = "/var/run/ospfd.pid";
#endif
#endif
#ifdef CONFIG_USER_QUAGGA
#ifdef CONFIG_USER_QUAGGA_RIPNGD
const char RIPNGD_PID[] = "/var/run/ripngd.pid";
#endif
char ZEBRA_PID[] = "/var/run/zebra.pid";
char OSPFD_PID[] = "/var/run/ospfd.pid";
char RIPD_PID[] = "/var/run/ripd.pid";
char BGPD_PID[] = "/var/run/bgpd.pid";
char ISISD_PID[] = "/var/run/isisd.pid";
#endif
const char IGMPPROXY_PID[] = "/var/run/igmp_pid";
#ifdef CONFIG_USER_MLDPROXY
const char MLDPROXY_PID[] = "/var/run/mld_pid";	//One of the MLD Proxy
#endif
const char PROC_DYNADDR[] = "/proc/sys/net/ipv4/ip_dynaddr";
const char PROC_IPFORWARD[] = "/proc/sys/net/ipv4/ip_forward";
const char PROC_FORCE_IGMP_VERSION[] = "/proc/sys/net/ipv4/conf/default/force_igmp_version";
const char MPOAD_FIFO[] = "/tmp/serv_fifo";
const char DHCPC_PID[] = "/var/run/udhcpc.pid";
const char DHCPC_ROUTERFILE[] = "/var/udhcpc/router";
const char DHCPC_SCRIPT[] = "/etc/scripts/udhcpc.sh";
const char DHCPC_SCRIPT_NAME[] = "/var/udhcpc/udhcpc";
const char DHCPD_CONF[] = "/var/udhcpd/udhcpd.conf";
const char DHCPD_LEASE[] = "/var/udhcpd/udhcpd.leases";
const char DHCPSERVERPID[] = "/var/run/udhcpd.pid";
#ifdef CONFIG_AUTO_DHCP_CHECK
const char AUTO_DHCPPID[] = "/var/run/auto_dhcp.pid";
#endif
const char DHCPRELAYPID[] = "/var/run/dhcrelay.pid";
const char MER_GWINFO[] = "/tmp/MERgw";
const char MER_GWINFO_V6[] = "/tmp/MERgwV6";
const char WAN_IPV4_CONN_TIME[] = "/tmp/wancon_ipv4";
const char WAN_IPV6_CONN_TIME[] = "/tmp/wancon_ipv6";
#ifdef _CWMP_MIB_
const char CWMP_PID[] = "/var/run/cwmp.pid";
const char CWMP_PROCESS[] = "cwmpClient";
#endif
#ifdef CONFIG_USER_CUPS
const char CUPSDPRINTERCONF[] = "/var/cups/conf/printers.conf";
#endif // CONFIG_USER_CUPS
#define MINI_UPNPDPID  "/var/run/mini_upnpd.pid"	//cathy
#ifdef CONFIG_USER_MINIUPNPD
#ifdef CONFIG_USER_MINIUPNP_V2_1
const char MINIUPNPDPID[] = "/var/run/miniupnpd.pid";
#else
const char MINIUPNPDPID[] = "/var/run/linuxigd.pid";
#endif
const char MINIUPNPD_CONF[] = "/var/miniupnpd.conf";
#endif	/* CONFIG_USER_MINIUPNPD */
const char MINIDLNAPID[] = "/var/run/minidlna.pid";
const char STR_DISABLE[] = "Disabled";
const char STR_ENABLE[] = "Enabled";
const char STR_AUTO[] = "Auto";
const char STR_MANUAL[] = "Manual";
const char STR_UNNUMBERED[] = "unnumbered";
const char STR_ERR[] = "err";
const char STR_NULL[] = "null";

const char rebootWord0[] = "시스템 재시작 중...";
const char rebootWord1[] = "이 장치가 구성되어 다시 시작 중입니다.";
const char rebootWord2[] = "구성 창을 닫고 웹 브라우저를 재개하기 전에 1분 기다리십시오. 필요한 경우 새 구성에 맞게 IP 주소를"
						" 다시 구성하십시오.";
// TR-111: ManufacturerOUI, ProductClass
const char MANUFACTURER_OUI[] = DEF_MANUFACTUREROUI_STR;//"00E04C";
const char PRODUCT_CLASS[] = DEF_PRODUCTCLASS_STR;//"IGD";


#if defined(URL_BLOCKING_SUPPORT)||defined(URL_ALLOWING_SUPPORT)
const char WEBURL_CHAIN[] = "urlblock";
const char WEBURLPOST_CHAIN[] = "urlblock_post";
const char WEBURLPRE_CHAIN[] = "urlblock_pre";
const char WEBURL_CHAIN_BLACK[] = "urlblock_black";
const char WEBURL_CHAIN_WHITE[] = "urlblock_white";
#endif

#ifdef _PRMT_X_CMCC_SECURITY_
const char PARENTALCTRL_CHAIN[] = "parentalctrl";
const char PARENTALCTRLPOST_CHAIN[] = "parentalctrl_post";
const char PARENTALCTRLPRE_CHAIN[] = "parentalctrl_pre";
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU)
const char LXC_FWD_TBL[] = "LXC_FWD_TBL";
const char LXC_IN_TBL[] = "LXC_IN_TBL";
#endif

const char PW_HOME_DIR[] = "/tmp";

#ifdef CONFIG_USER_CLI
#ifdef USE_DEONET /* sghong, 20230406 */
const char PW_CMD_SHELL[] = "/bin/sh";
#else
// if want to cli mode start
const char PW_CMD_SHELL[] = "/bin/cli";
#endif
#else
const char PW_CMD_SHELL[] = "/bin/sh";
#endif
const char PW_NULL_SHELL[] = "/dev/null"; //didn't allow to login, IulianWu

const char RESOLV[] = "/var/resolv.conf";
const char DNSMASQ_CONF[] = "/var/dnsmasq.conf";
const char RESOLV_BACKUP[] = "/var/resolv_backup.conf";
const char MANUAL_RESOLV[] = "/var/resolv.manual.conf";
// Mason Yu. for copy the ppp & dhcp nameserver to the /var/resolv.conf
const char AUTO_RESOLV[] = "/var/resolv.auto.conf";	/*add resolv.auto.conf to distinguish DNS manual and auto config*/
const char DNS_RESOLV[] = "/var/udhcpc/resolv.conf";
const char DNS6_RESOLV[] = "/var/resolv6.conf";
const char PPP_RESOLV[] = "/etc/ppp/resolv.conf";
const char HOSTS[] = "/var/tmp/hosts";

const char SYSTEMD_USOCK_SERVER[] = "/var/run/systemd.usock";
const char EPONOAMD_USOCK_SERVER[] = "/var/run/eponoamd.usock";
const char RA_INFO_FILE[] = "/var/rainfo";
const char RA_INFO_PREFIX_KEY[] = "PREFIX=";
const char RA_INFO_PREFIX_LEN_KEY[] = "PREFIX_LEN=";
const char RA_INFO_PREFIX_ONLINK_KEY[] = "PREFIX_OnLink=";
const char RA_INFO_PREFIX_IFNAME_KEY[] = "IFNAME=";
const char RA_INFO_PREFIX_MTU_KEY[] = "MTU=";
const char RA_INFO_PREFIX_GW_KEY[] = "GW_IP=";
const char RA_INFO_RDNSS_KEY[] = "RDNSS=";
const char RA_INFO_DNSSL_KEY[] = "DNSSL=";
const char RA_INFO_DNSSL_LIFETIME_KEY[] = "DNSSL_Lifetime=";
const char RA_INFO_M_BIT_KEY[] = "M=";
const char RA_INFO_O_BIT_KEY[] = "O=";
const char RA_INFO_LIFETIME_KEY[] = "Lifetime=";
#ifdef CONFIG_IPOE_ARPING_SUPPORT
const char ARPING_IPOE[] = "/bin/arping_ipoe";
const char ARPING_RESULT[] = "/tmp/arping_result";
const char ARPING_IPOE_RUNFILE[] = "/var/run/arping_ipoe.pid";
#endif
#ifdef	CONFIG_RTK_CTCAPD_FILTER_SUPPORT
const char MARK_FOR_URL_FILTER[] = "mark_for_url_filter";
const char MARK_FOR_DNS_FILTER[] = "mark_for_dns_filter";
#endif
#ifdef DOMAIN_BLOCKING_SUPPORT
const char MARK_FOR_DOMAIN[] = "mark_for_domainBLK";
#endif
#ifdef CONFIG_USER_ANDLINK_PLUGIN
const char SSID2_WLAN_FILTER[] = "ssid2_wlan_filter";
#endif
#ifdef BLOCK_LAN_MAC_FROM_WAN
const char FW_BLOCK_LANHOST_MAC_FROM_WAN[]="block_lanhost_mac_from_wan";
#endif
#ifdef BLOCK_INVALID_SRCIP_FROM_WAN
const char FW_BLOCK_INVALID_SRCIP_FROM_WAN[]="block_invalid_srcip_from_wan";
#endif
const char FW_FTP_ACCOUNT[] = "ftp_account";
#ifdef CONFIG_USER_DHCPCLIENT_MODE
const char REDIRECT_URL_TO_BR0[] = "redirect_url_to_br0";
#endif

const char ROUTE_BRIDGE_WAN_SUFFIX[] = "_br";

#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
const char FW_PUSHWEB_FOR_UPGRADE[] = "fwupgrade_pushweb_redirect";
#endif
#ifdef SUPPORT_DOS_SYSLOG
const char FW_DOS[] = "DOS";
#endif

#ifdef USE_DEONET /* sghong, 20230406 */
const char FW_BR_PRIVATE_DHCP_SERVER_INPUT[] = "br_private_dhcp_server";
const char FW_BR_DHCPRELAY6_INPUT[] = "br_dhcprelay6_input";
const char FW_BR_DHCPRELAY6_FORWARD[] = "br_dhcprelay6_forward";
const char FW_BR_DHCPRELAY6_OUTPUT[] = "br_dhcprelay6_output";
const char FW_BR_DHCPRELAY_INPUT[] = "br_dhcprelay_input";
const char FW_BR_DHCPRELAY_FORWARD[] = "br_dhcprelay_forward";
const char FW_BR_DHCPRELAY_OUTPUT[] = "br_dhcprelay_output";

const char FW_BR_LANTOLAN_DSCP_BROUTE[] = "lantolan_dscp_broute";
const char FW_BR_OLTTOLAN_BROUTE[] = "olttolan_broute";
const char FW_BR_OLTTOLAN_FORWARD[] = "olttolan_forward";
const char FW_BR_ICMPV6_TX_MANGLE_OUTPUT[] = "icmpv6_tx_mangle_output";
const char FW_BR_ICMPV6_TX_MANGLE_POSTROUTING[] = "icmpv6_tx_mangle_postrt";
const char FW_BR_ICMPV6_TX_OUTPUT[] = "icmpv6_tx_output";

const char SKB_ACL[] = "SKB_ACL";
const char CPU_RATE_ACL[] = "CPU_RATE_ACL";
const char RATE_ACL[] = "RATE_ACL";
const char UPNP_ACL[] = "UPNP_ACL";
#endif

static struct process_nice_table_entry process_nice_table[] = {
	//processName, nice
	{"iwcontrol", -19},
	{"boa", -15},
	{"runomci.sh", -18},
	{"omci_app", -18},
	{"eponoamd", -18},
	{"VoIP_maserati", -18},
	{"cwmpClient", -18},
	{"spppd", -17},
	{"auth", -17},
	{"wscd", -17},
	{"proxyDaemon", -15},
	{"reg_server", -17},
	{"dbus-daemon", -17},
	{"cltsOnBRMode", -17},
	{"smbd", -19},
	{"udhcpc", -18}
};

const char *ppp_auth[] = {
	"AUTO", "PAP", "CHAP", "CHAPMS-V1", "CHAPMS-V2", "NONE"
};

//for VPN (pptp/l2tp), the selectable authentication is  Auto/PAP/CHAP/CHAPMSV2
const char *vpn_auth[] = {
	"Auto", "PAP", "CHAP", "CHAPMSV2", "NONE"
};

const char errGetEntry[] = "Get table entry error!";

// Added by Davian
#ifdef CONFIG_USER_XMLCONFIG
const char *shell_name = "/bin/ash";
#endif

//Alan 20160728
const char hideErrMsg1[] = ">/dev/null";
const char hideErrMsg2[] = "2>&1";

#ifdef CONFIG_E8B
const char BACKUP_DIRNAME[] = "e8_Config_Backup";
#endif

#ifdef CONFIG_CMCC_OSGIMANAGE
const char OSGIMANAGE_PID[] = "/var/run/osgiManage.pid";
const char OSGIAPP_PID[] = "/var/run/osgiApp.pid";
#endif
#ifdef CONFIG_USER_CUMANAGEDEAMON
const char CUMANAGE_PID[] = "/var/run/cumanagedaemon.pid";
#endif
const char KMODULE_CMCC_MAC_FILTER_FILE[] =  "/proc/mac_filter/mac_filter_add";

#ifdef CONFIG_USER_DM
const char DM_PID[]	= "/var/run/dm.pid";
#endif
#ifdef CONFIG_USER_VXLAN
//const char VXLAN_INTERFACE[] = "vxlan";
//const char VXLAN_CMD[] = "/bin/ip";
int delVxLAN_for_interface(char*);
#endif
const char VCONFIG[] = "/bin/vconfig";
#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
const char WIZARD_IN_PROCESS[] = "/tmp/wizard_in_process";
#endif

#if (defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_) ) && defined(_SUPPORT_L2BRIDGING_DHCP_SERVER_)
const char DHCPD_IPV4_INTERFACE_GROUP_LEASES_PARSED[] = "/var/dhcpd_ipv4_interface_group.leases_parsed";
const char DHCPD_IPV4_PARSE_LEASES_SCRIPT[] = "/etc/scripts/dhcpd_ipv4_parse_lease.sh";
const char DHCPD_IPV4_INTERFACE_GROUP_LEASES[] = "/var/dhcpd_ipv4_interface_group.leases";
const char DHCPD_IPV4_INTERFACE_GROUP_CONF[]="/var/dhcpd_ipv4_interface_group.conf";
const char DHCPD_IPV4_INTERFACE_GROUP_PID[]="/var/dhcpd_ipv4_interface_group.pid";
#endif

int g_wan_mode;
char *keytable_get_decrypted_content(char *filename);

#ifdef CONFIG_USER_CONNTRACK_TOOLS
char *mask2string(unsigned char mask){
	struct in_addr in_mask;
	unsigned char idx=0;
	char *tmp;

	if(mask>32)
		return NULL;

	memset(&in_mask, 0x0, sizeof(in_mask));
	tmp = (char *)&in_mask;
	for(idx=0; idx<mask;idx++){
		*(tmp+(idx/8))=*(tmp+(idx/8)) | (0x1 << (7-(idx%8)));
	}

	//printf("[%s:%d] mask=%d to %s\n", __FUNCTION__, __LINE__, mask, inet_ntoa(in_mask));
	return inet_ntoa(in_mask);
}
#endif

/*setup mac addr if mac[n-1]++ > 0xff and set mac[n-2]++, etc.*/
void setup_mac_addr(unsigned char *macAddr, int index)
{
	if((macAddr[5]+index) > 0xff)
	{
	  printf("macAddr[5]+%d = %x > 0xff\n",index,macAddr[5]+index);
	  //macAddr[4]++;
	  if((macAddr[4]+1) > 0xff)
	  {
	  	printf("macAddr[4]+1 = %x > 0xff\n",macAddr[4]+1);
	  	if((macAddr[3]+1) > 0xff)
		{
		  printf("something wrong for setting your macAddr[3] > 0xff!!\n");
		}
		macAddr[3]+=1;
	  }
	  macAddr[4]+=1;
    }
	AUG_PRT("macAddr[5]+%d = %x\n",index,macAddr[5]+index);

	macAddr[5]+=index;
}

int isIPAddr(char * IPStr)	//check is ip or domain name
{
	struct in_addr p;

	if(IPStr == NULL) return 0;

	return (inet_aton(IPStr, &p)) ? 1 : 0;
}

#ifdef CONFIG_USER_MINIUPNP_V2_1
static int generate_uuid(char *uuid, unsigned int size){
	int ret=0;

	if(!uuid || size == 0){
		fprintf(stderr, "[%s] NULL parameter\n", __FUNCTION__);
		return -1;
	}

	FILE *pp = popen("cat /proc/sys/kernel/random/uuid", "r");

	if (pp) {
		memset(uuid, 0x0, sizeof(uuid));
		if (!fgets(uuid, size, pp)) {
			fprintf(stderr, "[%s] get uuid fail\n", __FUNCTION__);
			ret = -1;
		}
		pclose(pp);
	}else{
		ret = -1;
	}
	return ret;
}

int setup_minuupnpd_conf(char *ext_if, char *lan_if)
{
	FILE *fp=NULL;
	unsigned char buffer[64];
	unsigned char value[6];
	unsigned char ipaddr[16];
	unsigned char uuid[48];

	if(ext_if == NULL || lan_if == NULL)
	{
		printf("[%s] NULL parameters\n", __FUNCTION__);
		return -1;
	}

	unlink(MINIUPNPD_CONF);
	if ((fp = fopen(MINIUPNPD_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", MLDPROXY_CONF);
		return -1;
	}

	/* UPnP Vendor Information */
	fprintf(fp, "friendly_name=Realtek Modem/Router IGD\n");
	fprintf(fp, "manufacturer_name=Realtek Semiconductor Corp.\n");
	fprintf(fp, "manufacturer_url=http://www.realtek.com/\n");
	fprintf(fp, "model_name=Modem/Router\n");
	fprintf(fp, "model_description=Realtek Modem/Router\n");
	fprintf(fp, "model_url=http://www.realtek.com/\n");

	memset(buffer, 0x0, sizeof(buffer));
	mib_get_s(MIB_HW_SERIAL_NUMBER, (void *)buffer, sizeof(buffer));
	fprintf(fp, "serial=%d\n", buffer);
	fprintf(fp, "model_number=%d\n", 1);
#ifdef CONFIG_USER_MINIUPNPD_IGD2
	fprintf(fp, "force_igd_desc_v1=no\n");	// If compiled with IGD_V2 defined, force reporting IGDv1 in rootDesc
#endif

	/* UPnP Configure Setting */
	fprintf(fp, "ext_ifname=%s\n", ext_if);
	//fprintf(fp, "ext_ifname6=%s\n", ext_if);	//if the WAN network interface for IPv6 is different than for IPv4
	fprintf(fp, "listening_ip=%s\n", lan_if);
	fprintf(fp, "http_port=%d\n", MINIUPNP_SERVICE_PORT);	// Port for HTTP (descriptions and SOAP) traffic. Set to 0 for autoselect.
	fprintf(fp, "secure_mode=yes\n");		// Secure Mode, UPnP clients can only add mappings to their own IP
	fprintf(fp, "notify_interval=60\n");	// Notify interval in seconds.
	fprintf(fp, "system_uptime=yes\n");		// Report system uptime instead of daemon uptime
	memset(value, 0x0, sizeof(value));
	memset(ipaddr, 0x0, sizeof(ipaddr));
	if(mib_get(MIB_ADSL_LAN_IP, (void *)value) != 0)
	{
		strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
		ipaddr[15] = '\0';

		/*
		 * IP address only work without IPv6 option
		 * due to It is mandatory to use the network interface name in order to enable IPv6
		 * fprintf(fp, "listening_ip=%s\n", ipaddr);
		*/
		fprintf(fp, "presentationURL=http://%s:80/\n", ipaddr);
	}

	if(-1 != generate_uuid(uuid, sizeof(uuid))){
		fprintf(fp, "uuid=%s\n", uuid);
	}

	/*
	 *# UPnP permission rules
	 *# (allow|deny) (external port range) IP/mask (internal port range)
	 *# A port range is <min port>-<max port> or <port> if there is only
	 *# one port in the range.
	 *	allow 1024-65535 192.168.0.0/24 1024-65535
	 *	allow 1024-65535 192.168.1.0/24 1024-65535
	 *	allow 1024-65535 192.168.0.0/23 22
	 *	allow 12345 192.168.7.113/32 54321
	 *	deny 0-65535 0.0.0.0/0 0-65535
	*/

	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(MLDPROXY_CONF, 0666);
#endif
	return 1;
ERR:
	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(MLDPROXY_CONF, 0666);
#endif
	return -1;
}
#endif	/* CONFIG_USER_MINIUPNPD */

int startSSDP(void)
{
#ifdef CONFIG_USER_MINI_UPNPD
	char *argv[10];
	int i = 0, pid = 0;

	pid = read_pid((char *)MINI_UPNPDPID);
	if (pid > 0)
	{
		kill(pid, 9);
		unlink(MINI_UPNPDPID);
	}

	argv[i++]="/bin/mini_upnpd";
#ifdef WLAN_SUPPORT
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
	char wscd_pid_name[32];
	getWscPidName(wscd_pid_name);
	if (read_pid(wscd_pid_name) > 0){
		argv[i++]="-wsc";
		argv[i++]="/tmp/wscd_config";
	}
#endif
#endif

#ifdef CONFIG_USER_MINIUPNPD
	if (read_pid((char *)MINIUPNPDPID) > 0){
		argv[i++]="-igd";
		argv[i++]="/tmp/igd_config";
	}
#endif
	argv[i]=NULL;
	do_nice_cmd( argv[0], argv, 0 );
#endif
	return 0;
}

//rule_mark[0] = "0x20000"  which means:  mark the 0 as 0x20000/0xe0000
const char*  rule_mark[] = {"0x20000/0xe0000",    "0x40000/0xe0000",
						   "0x60000/0xe0000",
						   "0x80000/0xe0000",
						   ""};

const char*  rule_mark_ppp[] = {"0xa0000/0xe0000", "0xc0000/0xe0000", "0xe0000/0xe0000",""};

#define ITF_SourceRoute_DHCP_MARK_NUM	4
#define ITF_SourceRoute_PPP_MARK_NUM	3

int getPrimaryLanNetAddrInfo(char* pAddr, int length)
{
	int netmask_bits=0;

	unsigned int _in_netmask;
	unsigned int _ipaddr;

	struct in_addr in_net_addr;

	char tmp[8];
	int j;

	if(length < 20)
		return -1;

	mib_get_s(MIB_ADSL_LAN_IP, (void *)&_ipaddr, sizeof(_ipaddr));
	mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)&_in_netmask, sizeof(_in_netmask));

	in_net_addr.s_addr = _ipaddr & _in_netmask;

	//printf("%s:%d _in_netmask %u\n", __FUNCTION__, __LINE__, _in_netmask);

	strcpy(pAddr, (const char *)inet_ntoa(in_net_addr));

	while (_in_netmask != 0)
	{
		netmask_bits += _in_netmask & 1;
		_in_netmask = _in_netmask >> 1;
	}
	//printf("%s:%d netmask_bits %d\n", __FUNCTION__, __LINE__, netmask_bits);

	if (netmask_bits != 0)
	{
		snprintf(pAddr, length, "%s/%d", pAddr, netmask_bits);
	}

	return 0;
}

int get_v4_interface_addr_net_info(char* pAddr, int length, char *infname)
{
	int netmask_bits=0;

	struct in_addr _ipaddr, _in_netmask;
	struct in_addr in_net_addr;
	char ipv4_ip_mask_cidr_str[INET_ADDRSTRLEN + 3] = {0};

	if(length < (INET_ADDRSTRLEN + 3)) //xxx.xxx.xxx.xxx/xx
		return -1;
	if (infname == NULL || infname[0] == '\0')
		return -1;

	if (getInAddr(infname, IP_ADDR, (void *)&_ipaddr) == 1) {
		if (getInAddr(infname, SUBNET_MASK, (void *)&_in_netmask) == 1)
		{
			in_net_addr.s_addr = _ipaddr.s_addr & _in_netmask.s_addr;
			//printf("%s:%d _in_netmask %u\n", __FUNCTION__, __LINE__, _in_netmask.s_addr);
			strncpy(pAddr, inet_ntoa(*((struct in_addr *)&in_net_addr)), length);
			while (_in_netmask.s_addr != 0)
			{
				netmask_bits += _in_netmask.s_addr & 1;
				_in_netmask.s_addr = _in_netmask.s_addr >> 1;
			}
			//printf("%s:%d netmask_bits %d\n", __FUNCTION__, __LINE__, netmask_bits);

			if (netmask_bits != 0)
			{
				snprintf(pAddr, length, "%s/%d", pAddr, netmask_bits);
			}
		}
	}
	return 0;
}

//eason
#ifdef _PRMT_USB_ETH_
//0:ok, -1:error
int getUSBLANMacAddr( char *p )
{
	unsigned char *hwaddr;
	struct ifreq ifr;
	strcpy(ifr.ifr_name, USBETHIF);
	do_ioctl(SIOCGIFHWADDR, &ifr);
	hwaddr = (unsigned char *)ifr.ifr_hwaddr.sa_data;
//	printf("The result of SIOCGIFHWADDR is type %d  "
//	       "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x.\n",
//	       ifr.ifr_hwaddr.sa_family, hwaddr[0], hwaddr[1],
//	       hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
	memcpy( p, hwaddr, 6 );
	return 0;
}

//0:high, 1:full, 2:low
int getUSBLANRate( void )
{
	struct ifreq ifr;
	strcpy(ifr.ifr_name, USBETHIF);
	do_ioctl( SIOCUSBRATE, &ifr );
//	fprintf( stderr, "getUSBLANRate=%d\n", *(int*)(&ifr.ifr_ifindex) ); //ifr_ifru.ifru_ivalue );
	return ifr.ifr_ifindex;//ifr_ifru.ifru_ivalue;
}

//0:up, 1:nolink
int getUSBLANStatus(void )
{
	struct ifreq ifr;
	strcpy(ifr.ifr_name, USBETHIF);
	do_ioctl( SIOCUSBSTAT, &ifr );
//	fprintf( stderr, "getUSBLANStatus=%d\n", *(int*)(&ifr.ifr_ifindex) ); //ifr_ifru.ifru_ivalue );
	return ifr.ifr_ifindex; //ifr_ifru.ifru_ivalue;
}
#endif

static const char *dhcp_mode[] = {
	"None", "DHCP Relay", "DHCP Server"
};

int virtual_port_enabled;
const int virt2user[] = {
	1, 2, 3, 4, 5
};

// Mason Yu
#ifdef PORT_FORWARD_ADVANCE
const char *PFW_Gategory[] = {"VPN", "Game"};
const char *PFW_Rule[] = {"PPTP", "L2TP"};
#endif

//keep the same order with ITF_T
//if want to convert lan device name to br0, call LANDEVNAME2BR0() macro
const char *strItf[] = {
	"",		//ITF_ALL
	"",		//ITF_WAN
	"br0",		//ITF_LAN

	"eth0",		//ITF_ETH0
	"eth0_sw0",	//ITF_ETH0_SW0
	"eth0_sw1",	//ITF_ETH0_SW1
	"eth0_sw2",	//ITF_ETH0_SW2
	"eth0_sw3",	//ITF_ETH0_SW3

	"wlan0",	//ITF_WLAN0
	"wlan0-vap0",	//ITF_WLAN0_VAP0
	"wlan0-vap1",	//ITF_WLAN0_VAP1
	"wlan0-vap2",	//ITF_WLAN0_VAP2
	"wlan0-vap3",	//ITF_WLAN0_VAP3

	"wlan1",	//ITF_WLAN0
	"wlan1-vap0",	//ITF_WLAN0_VAP0
	"wlan1-vap1",	//ITF_WLAN0_VAP1
	"wlan1-vap2",	//ITF_WLAN0_VAP2
	"wlan1-vap3",	//ITF_WLAN0_VAP3

	"usb0",		//ITF_USB0

	""		//ITF_END
};

int IfName2ItfId(char *s)
{
	int i;
	if( !s || s[0]==0 ) return ITF_ALL;
	if( (strncmp(s, ALIASNAME_PPP, strlen(ALIASNAME_PPP))==0) || (strncmp(s, ALIASNAME_VC, strlen(ALIASNAME_VC))==0)
		|| (strncmp(s, ALIASNAME_NAS, strlen(ALIASNAME_NAS))==0) || (strncmp(s, ALIASNAME_PTM, strlen(ALIASNAME_PTM))==0))
		return ITF_WAN;

	for( i=0;i<ITF_END;i++ )
	{
		if( strcmp( strItf[i],s )==0 ) return i;
	}

	return -1;
}

#ifdef CONFIG_USER_DHCPCLIENT_MODE
int get_sockfd(void)
{
	int sockfd = -1;
	if (sockfd == -1) {
		if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) {
			perror("user: socket creation failed");
			return(-1);
		}
	}
	return sockfd;
}

int getBr0IpAddr()
{
	struct ifreq ifr;
	struct sockaddr_in *saddr;
	struct in_addr addr;
	int fd, ret = 0;

	addr.s_addr = 0;
	fd = get_sockfd();
	if (fd >= 0) {
    	strcpy(ifr.ifr_name, "br0");
		ifr.ifr_addr.sa_family = AF_INET;
		if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
		{
			saddr = (struct sockaddr_in *)&ifr.ifr_addr;
			addr = saddr->sin_addr;
			ret = 1;
		}
		close(fd);
	}

	return ret;
}

int getIfAddr(const char *devname, char* ip)
{
	int skfd;
	struct ifreq intf;
	char *ptr;
	int ret = 0;

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		return ret;
	}

	strcpy(intf.ifr_name, devname);

	if (ioctl(skfd, SIOCGIFADDR, &intf) != -1)
	{
		if ((ptr = inet_ntoa(((struct sockaddr_in *)(&(intf.ifr_dstaddr)))->sin_addr)) != NULL)
		{
			strcpy(ip, ptr);
			ret = 1;
		}
	}
	close(skfd);

	return ret;
}
#endif

// Mason Yu. 090903
int do_ioctl(unsigned int cmd, struct ifreq *ifr)
{
	int skfd, ret;

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return (-1);
	}

	ret = ioctl(skfd, cmd, ifr);
	close(skfd);
	return ret;
}

/*
 *	Check if a host is direct connected to local interfaces.
 *	pEntry: check with local interface pEntry; null to check all local interfaces.
 */
int isDirectConnect(struct in_addr *haddr, MIB_CE_ATM_VC_Tp pEntry)
{
	char buff[256];
	int flgs;
	struct in_addr dest, mask;
	FILE *fp;

	if (pEntry) {
		dest = *((struct in_addr *)pEntry->ipAddr);
		mask = *((struct in_addr *)pEntry->netMask);
		if ((dest.s_addr & mask.s_addr) == (haddr->s_addr & mask.s_addr)) {
			//printf("dest=0x%x, mask=0x%x\n", dest.s_addr, mask.s_addr);
			return 1;
		}
	}
	else { // pEntry == NULL
		if (!(fp = fopen("/proc/net/route", "r"))) {
			printf("Error: cannot open /proc/net/route - continuing...\n");
			return 0;
		}
		fgets(buff, sizeof(buff), fp);
		while (fgets(buff, sizeof(buff), fp) != NULL) {
			if (sscanf(buff, "%*s%x%*x%x%*d%*d%*d%x", &dest, &flgs, &mask) != 3) {
				printf("Unsuported kernel route format\n");
				fclose(fp);
				return 0;
			}
			if ((flgs & RTF_UP) && mask.s_addr != 0) {
				if ((dest.s_addr & mask.s_addr) == (haddr->s_addr & mask.s_addr)) {
					//printf("dest=0x%x, mask=0x%x\n", dest.s_addr, mask.s_addr);
					fclose(fp);
					return 1;
				}
			}
		}
		fclose(fp);
	}
	return 0;
}

/*
 * Get Interface Addr (MAC, IP, Mask)
 */
int getInAddr(char *interface, ADDR_T type, void *pAddr)
{
	struct ifreq ifr;
	int found=0;
	struct sockaddr_in *addr;

	ifr.ifr_name[sizeof(ifr.ifr_name)-1] = '\0' ;
	strncpy(ifr.ifr_name, interface,sizeof(ifr.ifr_name)-1);
	if (do_ioctl(SIOCGIFFLAGS, &ifr) < 0)
		return (0);

	if (type == HW_ADDR) {
		if (do_ioctl(SIOCGIFHWADDR, &ifr) >= 0) {
			memcpy(pAddr, &ifr.ifr_hwaddr, sizeof(struct sockaddr));
			found = 1;
		}
	}
	else if (type == IP_ADDR) {
		if (do_ioctl(SIOCGIFADDR, &ifr) == 0) {
			addr = ((struct sockaddr_in *)&ifr.ifr_addr);
			*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
			found = 1;
		}
	}
	else if (type == DST_IP_ADDR) {
		if (do_ioctl(SIOCGIFDSTADDR, &ifr) == 0) {
			addr = ((struct sockaddr_in *)&ifr.ifr_addr);
			*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
			found = 1;
		}
	}
	else if (type == SUBNET_MASK) {
		if (do_ioctl(SIOCGIFNETMASK, &ifr) >= 0) {
			addr = ((struct sockaddr_in *)&ifr.ifr_addr);
			*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
			found = 1;
		}
	}
	return found;
}

int getInMTU(const char *interface, int *mtu)
{
	struct ifreq ifr;
	int found = 0;
	ifr.ifr_name[sizeof(ifr.ifr_name)-1]='\0';
	strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name)-1);

	if (do_ioctl(SIOCGIFMTU, &ifr) == 0) {
		if (mtu)
			*mtu = ifr.ifr_mtu;
		found = 1;
	}
	return found;
}

int setInMtu(const char *interface, int mtu)
{
	struct ifreq ifr;
	int ret=0;

	ifr.ifr_name[sizeof(ifr.ifr_name)-1]='\0';
	strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name)-1);
	ifr.ifr_mtu = mtu;

	if (do_ioctl(SIOCSIFMTU, &ifr) == 0)
		ret = 1;

	return ret;
}

void deo_IfMtuSet(char *ifname, int mtu)
{
	int mod_mtu;
	char mod_ifname[10];

	/* ifconfig: SIOCSIFMTU: Numerical result out of range fixed */
	if (strcmp(ifname, "nas0_0") == 0)
		strcpy(mod_ifname, "nas0");
	else if (strcmp(ifname, "eth0.2.0") == 0)
		strcpy(mod_ifname, "eth0.2");
	else if (strcmp(ifname, "eth0.3.0") == 0)
		strcpy(mod_ifname, "eth0.3");
	else if (strcmp(ifname, "eth0.4.0") == 0)
		strcpy(mod_ifname, "eth0.4");
	else if (strcmp(ifname, "eth0.5.0") == 0)
		strcpy(mod_ifname, "eth0.5");
	else
		return;

	if ((mtu+18) > 2000)
		mod_mtu = 2000;
	else
		mod_mtu += 18;

	setInMtu(mod_ifname, mod_mtu);

	return;
}

int getInFlags(const char *interface, int *flags)
{
	struct ifreq ifr;
	int found=0;

#ifdef EMBED
	ifr.ifr_name[sizeof(ifr.ifr_name)-1]='\0';
	strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name)-1);

	if (do_ioctl(SIOCGIFFLAGS, &ifr) == 0) {
		if (flags)
			*flags = ifr.ifr_flags;
		found = 1;
	}
#endif
	return found;
}

int setInFlags(const char *interface, int flags)
{
	struct ifreq ifr;
	int ret=0;

#ifdef EMBED
	ifr.ifr_name[sizeof(ifr.ifr_name)-1]='\0';
	strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name)-1);
	ifr.ifr_flags = flags;

	if (do_ioctl(SIOCSIFFLAGS, &ifr) == 0)
		ret = 1;
#endif
	return ret;
}

int is_interface_run(const char *interface)
{
	int flag, ret=0;
	if (getInFlags(interface, &flag) == 1)
	{
		if((flag & IFF_UP) && (flag & IFF_RUNNING))
			ret = 1;
	}
	return ret;
}

int INET_resolve(char *name, struct sockaddr *sa)
{
	struct sockaddr_in *s_in = (struct sockaddr_in *)sa;

	s_in->sin_family = AF_INET;
	s_in->sin_port = 0;

	if(name == NULL)
		return -1;

	/* Empty string and "default" is special, meaning 0.0.0.0. */
	if (name[0]=='\0' || strcmp(name, "default")==0) {
		s_in->sin_addr.s_addr = INADDR_ANY;
		return 1;
	}
	/* Look to see if it's a dotted quad. */
	if (inet_aton(name, &s_in->sin_addr)) {
		return 0;
	}
	/* guess not.. */
	return -1;
}

void ppp_if6up(char *ifname)
{
#ifdef CONFIG_E8B
#ifdef CONFIG_IPV6
	struct in_addr inAddr;
	if (getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 0) // IPv4 not up
	{
		int numOfIpv6 = 0;
		struct ipv6_ifaddr ipv6_addr;
		numOfIpv6 = getifip6(ifname, IPV6_ADDR_UNICAST, &ipv6_addr, 1);
		//fprintf(stderr, "ifname = %s, numOfIpv6 = %d \n", __FILE__, __FUNCTION__, __LINE__, ifname, numOfIpv6);
		if (numOfIpv6 == 1)
		{
			unsigned char InformType = 0;
			mib_get_s(PROVINCE_CWMP_INFORM_TYPE, &InformType, sizeof(InformType));

			if (InformType == CWMP_INFORM_TYPE_GUD)
			{
				unsigned int vUint = 0;
				if (mib_get_s(CWMP_INFORM_USER_EVENTCODE, &vUint, sizeof(vUint))) {
					if (!(vUint & EC_X_CT_COM_BIND_1)) {
						vUint = vUint | EC_X_CT_COM_BIND_1;
						mib_set(CWMP_INFORM_USER_EVENTCODE, &vUint);
					}
				}
			}
		}
	}
#endif
#endif
	return;
}

/*
* Convert ifIndex to system interface name, e.g. eth0,vc0...
*/
char *ifGetName(int ifindex, char *buffer, unsigned int len)
{
	MEDIA_TYPE_T mType;

	if ( ifindex == DUMMY_IFINDEX )
		return 0;
	if (PPP_INDEX(ifindex) == DUMMY_PPP_INDEX)
	{
		mType = MEDIA_INDEX(ifindex);
		if (mType == MEDIA_ATM) {
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			if (VC_MINOR_INDEX(ifindex) == DUMMY_VC_MINOR_INDEX) // major vc devname
				snprintf( buffer, len,  "%s%u", ALIASNAME_VC, VC_MAJOR_INDEX(ifindex));
			else
				snprintf( buffer, len,  "%s%u_%u", ALIASNAME_VC, VC_MAJOR_INDEX(ifindex),  VC_MINOR_INDEX(ifindex));
#else
			snprintf( buffer, len,  "%s%u", ALIASNAME_VC, VC_INDEX(ifindex) );
#endif
		}
		else if (mType == MEDIA_ETH)
#ifdef CONFIG_RTL_MULTI_ETH_WAN
			snprintf( buffer, len, "%s%d", ALIASNAME_MWNAS, ETH_INDEX(ifindex));
#else
			snprintf( buffer, len,  "%s%u", ALIASNAME_NAS, ETH_INDEX(ifindex) );
#endif
#ifdef CONFIG_PTMWAN
		else if (mType == MEDIA_PTM)
			#ifdef CONFIG_RTL_MULTI_WAN
			snprintf( buffer, len, "%s%d", ALIASNAME_MWPTM, PTM_INDEX(ifindex));
			#else
			snprintf( buffer, len, "%s%d", ALIASNAME_PTM, PTM_INDEX(ifindex));
			#endif
#endif /*CONFIG_PTMWAN*/
#ifdef WLAN_WISP
		else if (mType == MEDIA_WLAN)
			snprintf( buffer, len, VXD_IF, ETH_INDEX(ifindex));
#endif
		else if (mType == MEDIA_IPIP)    // Mason Yu. Add VPN ifIndex
			snprintf( buffer, len, "ipip%u", IPIP_INDEX(ifindex));
		else
			return 0;

	}else{
		snprintf( buffer, len,  "ppp%u", PPP_INDEX(ifindex) );

	}
	return buffer;
}

int ifGetGatewayByWan(char* ifname, char *gwaddr)
{
	char filename[32];
	struct in_addr gateway;
	FILE *fp;

	sprintf(filename, "%s.%s", (char *)MER_GWINFO, ifname);
	if (!(fp = fopen(filename, "r"))) {
		//printf("Error: cannot open %s - continuing...\n", filename);
		return 0;
	}
	if(fscanf(fp, "%s", gwaddr) != 1){
		fclose(fp);
		return 0;
	}
	fclose(fp);
	return 1;
}


#if defined(CONFIG_RTK_DEV_AP)
int isSameIP(char *ip, unsigned int ifIndex, char *buffer, unsigned int len)
{
	char ifname[IFNAMSIZ];
	struct in_addr inAddr;
	char *itfIP;

	ifGetName(ifIndex,ifname,sizeof(ifname));
	if (getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 1) {
		itfIP = inet_ntoa(inAddr);
		if(!strcmp(itfIP, ip)){
			strncpy(buffer, ifname, len);
			buffer[len-1]='\0';
			//printf("getNameByIP: The %s IPv4 address is %s. Found\n", ifname, itfIP);
			return 1;
		}
	}
	return 0;
}
#endif

int isAnyPPPoEWan(void)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			printf("Get chain record error!\n");
			return -1;
		}

		if (Entry.enable == 0)
			continue;

		if(Entry.cmode == CHANNEL_MODE_PPPOE)
			return 1;
	}
	return 0;
}

int getNameByIP(char *ip, char *buffer, unsigned int len)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ];
	struct in_addr inAddr;
	char *itfIP;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
  			printf("getNameByIP: Get chain record error!\n");
			return 0;
		}

		if (Entry.enable == 0)
			continue;

		#if defined(CONFIG_RTK_DEV_AP)
		if(isSameIP(ip, Entry.ifIndex, buffer, len))
			return 1;

		#else
		ifGetName(Entry.ifIndex,ifname,sizeof(ifname));
		if (getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 1) {
			itfIP = inet_ntoa(inAddr);
			if(!strcmp(itfIP, ip)){
				strncpy(buffer, ifname, len);
				buffer[len-1]='\0';
				//printf("getNameByIP: The %s IPv4 address is %s. Found\n", ifname, itfIP);
				break;
			}
		}
		#endif
	}

#if defined(CONFIG_RTK_DEV_AP)
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP)
	MIB_PPTP_T pptp;
	unsigned int pptpEnable;
	if ( !mib_get_s(MIB_PPTP_ENABLE, (void *)&pptpEnable, sizeof(pptpEnable)) )
		return 0;
	if(pptpEnable){
		entryNum = mib_chain_total(MIB_PPTP_TBL);
		for (i=0; i<entryNum; i++)
		{
			if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&pptp) )
				return 0;
			if(isSameIP(ip, pptp.ifIndex, buffer, len))
				return 1;
		}
	}
#endif

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	MIB_L2TP_T l2tp;
	unsigned int l2tpEnable;
	if ( !mib_get_s(MIB_L2TP_ENABLE, (void *)&l2tpEnable, sizeof(l2tpEnable)) )
		return 0;
	if(l2tpEnable){
		entryNum = mib_chain_total(MIB_L2TP_TBL);
		for (i=0; i<entryNum; i++)
		{
			if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tp) )
				return 0;
			if(isSameIP(ip, l2tp.ifIndex, buffer, len))
				return 1;
		}
	}
#endif
	printf("getNameByIP: not find this interface!\n");
	return 0;
#else
	if(i>= entryNum){
		printf("getNameByIP: not find this interface!\n");
		return 0;
	}

	return 1;
#endif
}

int getWANEntryIdxFromIfindex(unsigned int ifindex){
	int i, mibtotal;
	MIB_CE_ATM_VC_T tmpEntry;
	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0; i< mibtotal; i++)
	{
		mib_chain_get(MIB_ATM_VC_TBL, i, &tmpEntry);
		if(tmpEntry.ifIndex==ifindex)
		{
			return i;
		}
	}
	return -1;
}

int getIfIndexByName(char *pIfname)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ];

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
  			printf("Get chain record error!\n");
			return -1;
		}

		if (Entry.enable == 0)
			continue;

		ifGetName(Entry.ifIndex,ifname,sizeof(ifname));

		if(!strcmp(ifname,pIfname)){
			break;
		}
	}

	if(i>= entryNum){
		//printf("not find this interface!\n");
		return DUMMY_IFINDEX;
	}

	return(Entry.ifIndex);
}

int rtk_wan_get_mac_by_ifname(const char *pIfname, char *pAddr)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ];

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			printf("Get chain record error!\n");
			return -1;
		}

		ifGetName(Entry.ifIndex,ifname,sizeof(ifname));

		if(!strcmp(ifname,pIfname)){
			break;
		}
	}

	if(i>= entryNum){
		//printf("not find this interface!\n");
		return 0;
	}

	memcpy(pAddr, Entry.MacAddr, MAC_ADDR_LEN);
	return 1;
}

/*ericchung add, get applicationtype by interface name */
int getapplicationtypeByName(char *pIfname)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ];

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
  			printf("Get chain record error!\n");
			return -1;
		}

		if (Entry.enable == 0)
			continue;

		ifGetName(Entry.ifIndex,ifname,sizeof(ifname));

		if(!strcmp(ifname,pIfname)){
			break;
		}
	}

	if(i> entryNum){
		printf("not find this interface!\n");
		return -1;
	}

	return(Entry.applicationtype);
}

#ifdef CONFIG_HWNAT
static inline int setEthPortMapping(struct ifreq *ifr)
{
	int ret=0;

#ifndef SIOCSITFGROUP
#define SIOCSITFGROUP	0x89c3
#endif
#ifdef EMBED
	if (do_ioctl(SIOCSITFGROUP, ifr) == 0)
		ret = 1;

#endif
	return ret;
}
#endif

unsigned int hextol(unsigned char *hex)
{
	//printf("%02x %02x %02x %02x\n",hex[0], hex[1], hex[2], hex[3]);
	return ( (hex[0] << 24) | (hex[1] << 16) | (hex[2] << 8) | (hex[3]));
}

// ioctl for direct bridge mode, jiunming
void __dev_setupDirectBridge(int flag)
{
	struct ifreq ifr;
	int ret;

	strcpy(ifr.ifr_name, ELANIF);
	ifr.ifr_ifru.ifru_ivalue = flag;
#ifdef EMBED
#if defined(CONFIG_RTL8672NIC)
	if( do_ioctl(SIOCDIRECTBR, &ifr)==0 ) {
		ret = 1;
		//printf("Set direct bridge mode %s!\n", flag?"enable":"disable" );
	}
	else {
		ret = 0;
		//printf("Set direct bridge mode error!\n");
	}
#endif
#endif

}

/*------------------------------------------------------------------
 * Get a list of interface info. (itfInfo) of the specified ifdomain.
 * where,
 * info: a list of interface info entries
 * len: max length of the info list
 * ifdomain: interface domain
 *-----------------------------------------------------------------*/
int get_domain_ifinfo(struct itfInfo *info, int len, int ifdomain)
{
	unsigned int swNum, vcNum;
	int i, num;
	int mib_wlan_num;
	char mygroup;
	char strlan[]="LAN0";
	char wanif[IFNAMSIZ];
	MIB_CE_ATM_VC_T pvcEntry;
	num=0;

#ifdef WLAN_SUPPORT
	int ori_wlan_idx;
#endif

	if (ifdomain&DOMAIN_ELAN) {
		// LAN ports
#ifdef IP_QOS_VPORT
		swNum = SW_LAN_PORT_NUM;
		for (i=0; i<swNum; i++) {
			strlan[3] = '0' + virt2user[i];	// user index
			info[num].ifdomain = DOMAIN_ELAN;
			info[num].ifid=i;	// virtual index
			strncpy(info[num].name, strlan, 8);
			num++;
			if (num > len)
				break;
		}
#else
		info[num].ifdomain = DOMAIN_ELAN;
		info[num].ifid=0;
		strncpy(info[num].name, strlan, 8);
		num++;
#endif
	}

#ifdef WLAN_SUPPORT
	if (ifdomain&DOMAIN_WLAN) {
		ori_wlan_idx = wlan_idx;

		for( wlan_idx = 0; wlan_idx < NUM_WLAN_INTERFACE; wlan_idx++ )
		{
			// wlan0
			mib_get_s(MIB_WLAN_ITF_GROUP, (void *)&mygroup, sizeof(mygroup));
			info[num].ifdomain = DOMAIN_WLAN;
			info[num].ifid = 0 + wlan_idx * 10;  // Magician: ifid of wlan0 = 0; ifid of wlan1 = 10
			strncpy(info[num].name, getWlanIfName(), 8);
			num++;

//jim luo add it to support QoS on Virtual AP...
#ifdef WLAN_MBSSID
			for (i=1; i<IFGROUP_NUM; i++) {
				mib_get_s(MIB_WLAN_VAP0_ITF_GROUP + ((i-1)<<1), (void *)&mygroup, sizeof(mygroup));
				info[num].ifdomain = DOMAIN_WLAN;
				info[num].ifid = i + wlan_idx * 10;  // Magician: ifid of wlan0-vap0~3 = 1~4; ifid of wlan1-vap0~3 = 11~14
				sprintf(info[num].name, VAP_IF, wlan_idx, i-WLAN_VAP_ITF_INDEX);
				num++;
			}
#endif  // WLAN_MBSSID
		}
		wlan_idx = ori_wlan_idx;
	}
#endif  // WLAN_SUPPORT

#ifdef CONFIG_USB_ETH
	if (ifdomain&DOMAIN_ULAN) {
		// usb0
		mib_get_s(MIB_USBETH_ITF_GROUP, (void *)&mygroup, sizeof(mygroup));
		info[num].ifdomain = DOMAIN_ULAN;
		info[num].ifid=0;
		sprintf(info[num].name, "%s", USBETHIF );
		num++;
	}
#endif //CONFIG_USB_ETH

	if (ifdomain&DOMAIN_WAN) {
		// vc
		vcNum = mib_chain_total(MIB_ATM_VC_TBL);

		for (i=0; i<vcNum; i++) {
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&pvcEntry))
			{
  				//boaError(wp, 400, "Get chain record error!\n");
  				printf("Get chain record error!\n");
				return 0;
			}

			if (pvcEntry.enable == 0)
				continue;

			info[num].ifdomain = DOMAIN_WAN;
			info[num].ifid=pvcEntry.ifIndex;
			ifGetName(pvcEntry.ifIndex, wanif, sizeof(wanif));
			strncpy(info[num].name, wanif, 8);
			num++;

			if (num > len)
				break;
		}
	}

	return num;
}

int read_pid(const char *filename)
{
	FILE *fp;
	int pid = 0;

	if ((fp = fopen(filename, "r")) == NULL)
		return -1;
	if(fscanf(fp, "%d", &pid) != 1 || kill(pid, 0) != 0){
		pid = 0;
	}
	fclose(fp);

	return pid;
}

/*
 * Kill process by pidfile.
 * Return pid on success, <=0 if pid not exist.
 */
int kill_by_pidfile(const char *pidfile, int sig)
{
	pid_t pid;
	int time_temp=0;

	pid = read_pid(pidfile);
	if (pid <= 0)
		return pid;
	kill(pid, sig);
	// wait for process termination
	while ((kill(pid, 0) == 0) && (time_temp < 20)) {
		usleep(100000);
		time_temp++;
	}
	if(time_temp >= 20 )
		kill(pid, SIGKILL);
	if(!access(pidfile, F_OK))
		unlink(pidfile);

	return pid;
}

/*
* Kill process by pidfile. timeout return
*/
int kill_by_pidfile_new(const char *pidfile, int sig)
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

int start_process_check_pidfile(const char *pidfile)
{
	pid_t pid;
	int time_temp=0;
	while(((pid = read_pid((char*)pidfile)) <= 0)&&(time_temp < 12)){
			usleep(250000);
			time_temp++;
		}
		if(time_temp >=12)
		{
			printf("[%s:%d] start process time is too long or failure", __FUNCTION__,__LINE__);
			return -1;
		}
	return pid;
}

int checkNiceOfProcess(const char *processName)
{
	unsigned int i, totalProcess = ((sizeof(process_nice_table))/(sizeof(struct process_nice_table_entry)));

	if(!processName)
		return 0;

	for(i=0 ; i<totalProcess ; i++)
	{
		if(strstr(processName, process_nice_table[i].processName)) {
			setpriority(PRIO_PROCESS, getpid(), process_nice_table[i].nice);
			return 1;
		}
	}

	setpriority(PRIO_PROCESS, getpid(), 0);
	return 0;
}

// Added by Kaohj
//return 0:OK, other:fail
//dowait, 1: wait process leave and return exit status, 0: don't wait
//donice, 1: CPU nice of the process, 0: disable nice
//doout, 0: don't output, 1: enable output only, 2: enable error output only, 3: enable both output
typedef void (*sighandler_t)(int);
#define VA_CMD_ARG_SIZE 64
int _do_cmd(const char *cmd, char *argv[], int dowait, int donice, int doout, char *output)
{
	pid_t pid, wpid;
	int ret, status=0, fd;
	sigset_t tmpset, origset;
	sighandler_t old_handler;

	sigfillset(&tmpset);
	sigprocmask(SIG_BLOCK, &tmpset, &origset);
	pid = vfork();
	sigprocmask(SIG_SETMASK, &origset, NULL);

	old_handler = signal(SIGCHLD, SIG_DFL);

	if (pid == 0) {
		/* the child */
		signal(SIGINT, SIG_IGN);

		if(donice)
			checkNiceOfProcess(cmd);

		//if don't wait process status, use vfork twice to avoid zombie issue
		if(!dowait)
		{
			pid = vfork();
			if(pid < 0)
			{
				fprintf(stderr, "fork twice of %s failed\n", cmd);
				_exit(EXIT_FAILURE);
			}
			else if(pid > 0)
			{
				_exit(EXIT_SUCCESS);
			}
			setsid();
		}

		{
			long i, max_fd = sysconf(_SC_OPEN_MAX);
			for (i = 0; i < max_fd; i++){
				if (i != STDOUT_FILENO && i != STDERR_FILENO && i != STDIN_FILENO)
					close(i);
			}
		}

		if(doout && output)
		{
			if((fd = open(output, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR))==-1)
			{
				fprintf(stderr, "<%s:%d> open %s failed\n", __FUNCTION__, __LINE__, output);
				_exit(EXIT_FAILURE);
			}

			if(doout&0x1)dup2(fd, STDOUT_FILENO);
			if(doout&0x2)dup2(fd, STDERR_FILENO);
			close(fd);
		}
		else
		{
			fd = open("/dev/null", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			if(fd >= 0)
			{
				if(!(doout&0x1))dup2(fd, STDOUT_FILENO);
				if(!(doout&0x2))dup2(fd, STDERR_FILENO);
				close(fd);
			}
			else
			{
				fprintf(stderr, "<%s:%d> open /dev/null failed\n", __FUNCTION__, __LINE__);
			}
		}

		argv[0] = (char *)cmd;
#if 0
		char *env[3];
		env[0] = "PATH=sbin:/usr/sbin:/bin:/usr/bin:/etc/scripts";
		env[1] = NULL;
		execvpe(cmd, argv, env);
#else //for some case need inherit environment variables from parent process
		execvp(cmd, argv);
#endif
		fprintf(stderr, "exec %s failed\n", cmd);
		_exit(EXIT_FAILURE);
	} else if (pid > 0) {
		/* need wait child pid (for vfork twice)
		if (!dowait)
			ret = 0;
		else
		*/
		{
			/* parent, wait till rc process dies before spawning */
			while ((wpid = waitpid(pid,&status,0)) != pid) {
				if (wpid == -1 && errno == ECHILD) {	/* see wait(2) manpage */
					break;
				}
			}

			ret = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
		}
	} else if (pid < 0) {
		fprintf(stderr, "fork of %s failed\n", cmd);
		ret = -1;
	}

	signal(SIGCHLD, old_handler);

	return ret;
}

// Run a command and output result to file if specified
//return 0:OK, other:fail
int do_cmd_fout(const char *cmd, char *argv[], int dowait, char *output)
{
	return _do_cmd(cmd, argv, dowait, 0, 3, output);
}

int va_cmd_fout(const char *cmd, int num, int dowait, char *file, ...)
{
	va_list ap;
	int k;
	char *s;
	char *argv[VA_CMD_ARG_SIZE];
	int status;

	if(cmd == NULL){
		fprintf(stderr, "<%s:%d> error command\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if(file == NULL){
		fprintf(stderr, "<%s:%d> error file\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if((num+2) > VA_CMD_ARG_SIZE){
		fprintf(stderr, "<%s:%d> argumet size overflow, num=%d\n", __FUNCTION__, __LINE__, num);
		return -1;
	}

	TRACE(STA_SCRIPT, "%s ", cmd);
	va_start(ap, file);

	for (k=0; k<num; k++)
	{
		s = va_arg(ap, char *);
		argv[k+1] = s;
		TRACE(STA_SCRIPT|STA_NOTAG, "%s ", s);
	}

	TRACE(STA_SCRIPT|STA_NOTAG, "\n");
	argv[k+1] = NULL;
	status = do_cmd_fout(cmd, argv, dowait, file);
	va_end(ap);

	return status;
}

//return 0:OK, other:fail
int va_niced_cmd(const char *cmd, int num, int dowait, ...)
{
	va_list ap;
	int k;
	char *s;
	char *argv[VA_CMD_ARG_SIZE];
	int status;

	if(cmd == NULL){
		fprintf(stderr, "<%s:%d> error command\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if((num+2) > VA_CMD_ARG_SIZE){
		fprintf(stderr, "<%s:%d> argumet size overflow, num=%d\n", __FUNCTION__, __LINE__, num);
		return -1;
	}

	TRACE(STA_SCRIPT, "%s ", cmd);
	va_start(ap, dowait);

	for (k=0; k<num; k++)
	{
		s = va_arg(ap, char *);
		argv[k+1] = s;
		TRACE(STA_SCRIPT|STA_NOTAG, "%s ", s);
	}

	TRACE(STA_SCRIPT|STA_NOTAG, "\n");
	argv[k+1] = NULL;
	status = do_nice_cmd(cmd, argv, dowait);
	va_end(ap);

	return status;
}

//return 0:OK, other:fail
int va_cmd(const char *cmd, int num, int dowait, ...)
{
	va_list ap;
	int k;
	char *s;
	char *argv[VA_CMD_ARG_SIZE];
	int status;

	if(cmd == NULL){
		fprintf(stderr, "<%s:%d> error command\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if((num+2) > VA_CMD_ARG_SIZE){
		fprintf(stderr, "<%s:%d> argumet size overflow, num=%d\n", __FUNCTION__, __LINE__, num);
		return -1;
	}

	TRACE(STA_SCRIPT, "%s ", cmd);
	va_start(ap, dowait);

	for (k=0; k<num; k++)
	{
		s = va_arg(ap, char *);
		argv[k+1] = s;
		TRACE(STA_SCRIPT|STA_NOTAG, "%s ", s);
	}

	TRACE(STA_SCRIPT|STA_NOTAG, "\n");
	argv[k+1] = NULL;
	status = do_cmd(cmd, argv, dowait);
	va_end(ap);

	return status;
}

//return 0:OK, other:fail
int do_cmd_ex(const char *cmd, char *argv[], int dowait, int noError)
{
	return _do_cmd(cmd, argv, dowait, 0, 1, NULL);
}

//return 0:OK, other:fail
int va_cmd_no_error(const char *cmd, int num, int dowait, ...)
{
	va_list ap;
	int k;
	char *s;
	char *argv[VA_CMD_ARG_SIZE];
	int status;

	if(cmd == NULL){
		fprintf(stderr, "<%s:%d> error command\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if((num+2) > VA_CMD_ARG_SIZE){
		fprintf(stderr, "<%s:%d> argumet size overflow, num=%d\n", __FUNCTION__, __LINE__, num);
		return -1;
	}

	TRACE(STA_SCRIPT, "%s ", cmd);
	va_start(ap, dowait);

	for (k=0; k<num; k++)
	{
		s = va_arg(ap, char *);
		argv[k+1] = s;
		TRACE(STA_SCRIPT|STA_NOTAG, "%s ", s);
	}

	TRACE(STA_SCRIPT|STA_NOTAG, "\n");
	argv[k+1] = NULL;
	status = do_cmd_ex(cmd, argv, dowait, 1);
	va_end(ap);

	return status;
}

//return 0:OK, other:fail
/*same function as va_cmd(). Execute silently.
  No print out command string in console.
*/
int va_cmd_no_echo(const char *cmd, int num, int dowait, ...)
{
	va_list ap;
	int k;
	char *s;
	char *argv[VA_CMD_ARG_SIZE];
	int status;

	if(cmd == NULL){
		fprintf(stderr, "<%s:%d> error command\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if((num+2) > VA_CMD_ARG_SIZE){
		fprintf(stderr, "<%s:%d> argumet size overflow, num=%d\n", __FUNCTION__, __LINE__, num);
		return -1;
	}

	va_start(ap, dowait);

	for (k=0; k<num; k++)
	{
		s = va_arg(ap, char *);
		argv[k+1] = s;
	}

	argv[k+1] = NULL;
	status =  _do_cmd(cmd, argv, dowait, 0, 0, NULL);
	va_end(ap);

	return status;
}

//return 0:OK, other:fail
int call_cmd(const char *cmd, int num, int dowait, ...)
{
	va_list ap;
	int k;
	char *s;
	char *argv[VA_CMD_ARG_SIZE];
	int status;

	if(cmd == NULL){
		fprintf(stderr, "<%s:%d> error command\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if((num+2) > VA_CMD_ARG_SIZE){
		fprintf(stderr, "<%s:%d> argumet size overflow, num=%d\n", __FUNCTION__, __LINE__, num);
		return -1;
	}

	TRACE(STA_SCRIPT, "%s ", cmd);
	va_start(ap, dowait);

	for (k=0; k<num; k++)
	{
		s = va_arg(ap, char *);
		argv[k+1] = s;
		TRACE(STA_SCRIPT|STA_NOTAG, "%s ", s);
	}

	TRACE(STA_SCRIPT|STA_NOTAG, "\n");
	argv[k+1] = NULL;
	status = do_cmd(cmd, argv, dowait);
	va_end(ap);

	return status;
}

#if 1 //def CONFIG_RTK_DEV_AP
int DoCmd(char *argv[], char *file)
{
	if(file)
		return _do_cmd(argv[0], argv, 1, 0, 3, file);
	else
		return _do_cmd(argv[0], argv, 1, 0, 1, NULL);
}

int RunSystemCmd(char *filepath, ...)
{
	va_list ap;
	int k = 0;
	char *s;
	char *argv[VA_CMD_ARG_SIZE];
	int status;

#ifdef DISPLAY_CMD
	printf("\n");
#endif
	while(1 && (k+2) < VA_CMD_ARG_SIZE)
	{
		s = va_arg( ap, char*);
		if (*s == '\0')
			break;
		argv[++k] = s;
#ifdef DISPLAY_CMD
		printf(" %s ", s);
#endif
	}
#ifdef DISPLAY_CMD
	printf("\n");
#endif
	argv[k+1] = NULL;
	status = DoCmd(argv, filepath);
	va_end(ap);

	return status;
}
#endif

#ifdef CONFIG_USER_WT_146
#define BFD_SERVER_FIFO_NAME "/tmp/bfd_serv_fifo"
#define BFD_CONF_PREFIX "/var/bfd/bfdconf_"
static void wt146_write_to_bfdmain(struct data_to_pass_st *pmsg)
{
	int bfdmain_fifo_fd=-1;

	bfdmain_fifo_fd = open(BFD_SERVER_FIFO_NAME, O_WRONLY);
	if (bfdmain_fifo_fd == -1)
		fprintf(stderr, "Sorry, no bfdmain server\n");
	else
	{
		write(bfdmain_fifo_fd, pmsg, sizeof(*pmsg));
		close(bfdmain_fifo_fd);
	}
}

#define BFDCFG_DBGLOG		"/var/bfd/bfd_dbg_log"
int wt146_dbglog_get(unsigned char *ifname)
{
	char buff[256];
	FILE *fp;
	unsigned char name[32];
	int dbgflag=0;

	//printf( "%s> enter\n", __FUNCTION__ );
	if(ifname)
	{
		//printf( "%s> open %s, search for %s\n", __FUNCTION__, BFDCFG_DBGLOG, ifname );
		fp = fopen(BFDCFG_DBGLOG, "r");
		if(fp)
		{
			while(fgets(buff, sizeof(buff), fp)!=NULL)
			{
				//printf( "%s> got %s", __FUNCTION__, buff );
				int flag;
				if (sscanf(buff, "%s %d", name, &flag) != 2)
					continue;
				else{
					if( strcmp( name, ifname )==0 )
					{
						dbgflag=flag?1:0;
						break;
					}
				}
			}
			fclose(fp);
		}
	}

	//printf( "%s> exit, dbgflag=%d\n", __FUNCTION__, dbgflag );
	return dbgflag;
}

void wt146_create_wan(MIB_CE_ATM_VC_Tp pEntry, int reset_bfd_only )
{
	if(pEntry)
	{
		char wanif[16], conf_name[32];
		FILE *fp;

		if(pEntry->enable==0)
			return;

		if( (pEntry->cmode!=CHANNEL_MODE_IPOE) &&
			(pEntry->cmode!=CHANNEL_MODE_RT1483) )
			return;

		if( pEntry->bfd_enable==0 ) return 0;

		ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));
		snprintf(conf_name, 32, "%s%s", BFD_CONF_PREFIX, wanif);
		fp=fopen( conf_name, "w" );
		if(fp)
		{
			int bfddebug;
			if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED)
			{
				fprintf( fp, "LocalIP=%s\n",  inet_ntoa(*((struct in_addr *)pEntry->ipAddr)) );
				fprintf( fp, "RemoteIP=%s\n",  inet_ntoa(*((struct in_addr *)pEntry->remoteIpAddr)) );
			}

			fprintf( fp, "OpMode=%u\n", pEntry->bfd_opmode );
			fprintf( fp, "Role=%u\n", pEntry->bfd_role );
			fprintf( fp, "DetectMult=%u\n", pEntry->bfd_detectmult );
			fprintf( fp, "MinTxInt=%u\n", pEntry->bfd_mintxint );
			fprintf( fp, "MinRxInt=%u\n", pEntry->bfd_minrxint );
			fprintf( fp, "MinEchoRxInt=%u\n", pEntry->bfd_minechorxint );
			fprintf( fp, "AuthType=%u\n", pEntry->bfd_authtype );
			if( pEntry->bfd_authtype!=BFD_AUTH_NONE )
			{
				int authkeylen=0;
				fprintf( fp, "AuthKeyID=%u\n", pEntry->bfd_authkeyid );
				fprintf( fp, "AuthKey=" );
					while( authkeylen<pEntry->bfd_authkeylen  )
					{
						fprintf( fp, "%02x", pEntry->bfd_authkey[authkeylen] );
						authkeylen++;
					}
					fprintf( fp, "\n" );
			}
			fprintf( fp, "DSCP=%u\n", pEntry->bfd_dscp );
			if(pEntry->cmode==CHANNEL_MODE_IPOE)
				fprintf( fp, "EthPrio=%u\n", pEntry->bfd_ethprio );
			bfddebug=wt146_dbglog_get( wanif );
			fprintf( fp, "debug=%d\n", bfddebug );
			fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
			chmod(conf_name, 0666);
#endif
			if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED)
			{
				struct data_to_pass_st msg;
				snprintf(msg.data, BUF_SIZE,"bfdctl add %s file %s", wanif, conf_name);
				TRACE(STA_SCRIPT, "%s\n", msg.data);
				//printf( "%s> %s\n",  __FUNCTION__, msg.data);
				wt146_write_to_bfdmain(&msg);
			}else{
				//only re-init bfd, not re-init wanconnection
				if(reset_bfd_only)
				{
					int dhcpc_pid;
					char dhcp_pid_buf[32];
					snprintf(dhcp_pid_buf, 32, "%s.%s", (char*)DHCPC_PID, wanif);
					dhcpc_pid = read_pid((char*)dhcp_pid_buf);
					if (dhcpc_pid > 0)
						kill(dhcpc_pid, SIGHUP);
				}
			}
		}

		//printf( "%s> create %s bfd session\n", __FUNCTION__, wanif );
	}
}

void wt146_del_wan(MIB_CE_ATM_VC_Tp pEntry)
{
	if(pEntry)
	{
		if(pEntry->enable==0)
			return;

		if( (pEntry->cmode!=CHANNEL_MODE_IPOE) &&
			(pEntry->cmode!=CHANNEL_MODE_RT1483) )
			return;

		//if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED)
		{
				struct data_to_pass_st msg;
				char wanif[16], conf_name[32];

				ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));
				snprintf(conf_name, 32, "%s%s", BFD_CONF_PREFIX, wanif);
				unlink( conf_name);

				snprintf(msg.data, BUF_SIZE,"bfdctl del %s", wanif);
				TRACE(STA_SCRIPT, "%s\n", msg.data);
				//printf( "%s> %s\n",  __FUNCTION__, msg.data);
				wt146_write_to_bfdmain(&msg);

				//printf( "%s> delete %s bfd session\n", __FUNCTION__, wanif );
		}
	}
}

void wt146_set_default_config(MIB_CE_ATM_VC_Tp pEntry)
{
	if(pEntry)
	{
		if( (pEntry->cmode==CHANNEL_MODE_IPOE) ||
			(pEntry->cmode==CHANNEL_MODE_RT1483) )
			pEntry->bfd_enable=1;
		else
			pEntry->bfd_enable=0;

		pEntry->bfd_opmode=BFD_DEMAND_MODE;
		pEntry->bfd_role=BFD_PASSIVE_ROLE;
		pEntry->bfd_detectmult=3;
		pEntry->bfd_mintxint=1000000;
		pEntry->bfd_minrxint=1000000;
		pEntry->bfd_minechorxint=0;
		pEntry->bfd_authtype=BFD_AUTH_NONE;
		pEntry->bfd_authkeyid=0;
		pEntry->bfd_authkeylen=0;
		memset( pEntry->bfd_authkey, 0, sizeof(pEntry->bfd_authkey) );
		pEntry->bfd_dscp=0;
		pEntry->bfd_ethprio=0;
	}
}

void wt146_copy_config(MIB_CE_ATM_VC_Tp pto, MIB_CE_ATM_VC_Tp pfrom)
{
	if(pto && pfrom)
	{
		//printf( "\n\nwt146_copy_config\n" );
		pto->bfd_enable=pfrom->bfd_enable;
		pto->bfd_opmode=pfrom->bfd_opmode;
		pto->bfd_role=pfrom->bfd_role;
		pto->bfd_detectmult=pfrom->bfd_detectmult;
		pto->bfd_mintxint=pfrom->bfd_mintxint;
		pto->bfd_minrxint=pfrom->bfd_minrxint;
		pto->bfd_minechorxint=pfrom->bfd_minechorxint;
		pto->bfd_authtype=pfrom->bfd_authtype;
		pto->bfd_authkeyid=pfrom->bfd_authkeyid;
		pto->bfd_authkeylen=pfrom->bfd_authkeylen;
		memcpy( pto->bfd_authkey, pfrom->bfd_authkey, sizeof(pto->bfd_authkey) );
		pto->bfd_dscp=pfrom->bfd_dscp;
		pto->bfd_ethprio=pfrom->bfd_ethprio;
	}
}
#endif //CONFIG_USER_WT_146

#ifdef CONFIG_HWNAT
void setup_hwnat_eth_member(int port, int mbr, char enable)
{
	struct ifreq ifr;
	struct ifvlan ifvl;

#ifndef CONFIG_RTL_MULTI_ETH_WAN
	strcpy(ifr.ifr_name, ALIASNAME_NAS0);
#else
	sprintf(ifr.ifr_name, "%s%d", ALIASNAME_MWNAS, ETH_INDEX(port));
#endif
	ifr.ifr_data = (char *)&ifvl;
	ifvl.enable=enable;
	ifvl.port=port;
	ifvl.member=mbr;
	if(setEthPortMapping(&ifr)!=1)
		printf("set hwnat port mapping error!\n");
	return;
}
#ifdef CONFIG_PTMWAN
void setup_hwnat_ptm_member(int port, int mbr, char enable)
{
	struct ifreq ifr;
	struct ifvlan ifvl;

	sprintf(ifr.ifr_name, "%s%d", ALIASNAME_MWPTM, PTM_INDEX(port));
	ifr.ifr_data = (char *)&ifvl;
	ifvl.enable=enable;
	ifvl.port=port;
	ifvl.member=mbr;
	if(setEthPortMapping(&ifr)!=1)
		printf("set ptm hwnat port mapping error!\n");
	return;
}
#endif /*CONFIG_PTMWAN*/
#endif

#ifdef CONFIG_USER_RTK_SYSLOG
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

#define SLOGDPID  "/var/run/syslogd.pid"
#define KLOGDPID  "/var/run/klogd.pid"
#define SLOGDLINK1 "/var/tmp/messages"
#define SLOGDLINK2 "/var/tmp/messages.old"

static int stopSlogD(void)
{
	int slogDid=0;
	int status=0;

	slogDid = read_pid((char*)SLOGDPID);
	if(slogDid > 0) {
		kill(slogDid, 9);
		unlink(SLOGDPID);
		unlink(SLOGDLINK1);
		unlink(SLOGDLINK2);
	}
	return 1;

}

static int stopKlogD(void)
{
	int klogDid=0;
	int status=0;

	klogDid = read_pid((char*)KLOGDPID);
	if(klogDid > 0) {
		kill(klogDid, 9);
		unlink(KLOGDPID);
	}
	return 1;

}

int stopLog(void)
{
#ifndef USE_DEONET
	unsigned char vChar;
	unsigned int vInt;
	char buffer[100];

	mib_get_s(MIB_ADSL_DEBUG, (void *)&vChar, sizeof(vChar));
	if(vChar==1){
#if defined(CONFIG_USER_BUSYBOX_KLOGD) || defined(USE_BUSYBOX_KLOGD)
		// Kill SlogD
		stopSlogD();

		// Kill KlogD
		stopKlogD();
#endif
	}
	else{
			// Kill SlogD
			stopSlogD();
#if defined(CONFIG_USER_BUSYBOX_KLOGD) || defined(USE_BUSYBOX_KLOGD)
			// Kill KlogD
			stopKlogD();
#endif
	}

#endif
	return 1;
}

int startLog(void)
{
#ifndef USE_DEONET
	unsigned char vChar;
	unsigned int vInt, log_file=0;
	char *argv[30], loglen[8], loglevel[2], log_file_num[8];
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
	char serverip[30], serverport[6];
#ifdef CONFIG_00R0
	char remoteLevel[4];
#endif
#endif
	int idx, len;

	mib_get_s(MIB_MAXLOGLEN, &vInt, sizeof(vInt));
	snprintf(loglen, sizeof(loglen), "%u", vInt);
	mib_get_s(MIB_SYSLOG_LOG_LEVEL, &vChar, sizeof(vChar));
	snprintf(loglevel, sizeof(loglevel), "%hhu", vChar);
	mib_get_s(MIB_SYSLOG_LOOP_FILES, &log_file, sizeof(log_file));
	snprintf(log_file_num, sizeof(log_file_num), "%u", log_file);
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
	mib_get_s(MIB_SYSLOG_MODE, &vChar, sizeof(vChar));
#endif
	argv[1] = "-n";
	argv[2] = "-s";
	argv[3] = loglen;
	argv[4] = "-l";
	argv[5] = loglevel;
	idx = 6;
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
	/* 1: Local, 2: Remote, 3: Both */
	/* Local or Both */
	if (vChar & 1)
		argv[idx++] = "-L";
	/* Remote or Both */
	if (vChar & 2) {
		getMIB2Str(MIB_SYSLOG_SERVER_IP, serverip);
		getMIB2Str(MIB_SYSLOG_SERVER_PORT, serverport);
		len = sizeof(serverip) - strlen(serverip);
		snprintf(serverip + strlen(serverip), len, ":%s", serverport);
		argv[idx++] = "-R";
		argv[idx++] = serverip;
	}
#ifdef CONFIG_00R0
	/* Remote Log Level*/
	mib_get_s(MIB_SYSLOG_REMOTE_LEVEL, &vChar, sizeof(vChar));
	snprintf(remoteLevel, sizeof(remoteLevel), "%hhu", vChar);
	argv[idx++] = "-r";
	argv[idx++] = remoteLevel;
#endif
#endif
	mib_get(MIB_SYSLOG, (void *)&vChar);
#ifdef BB_FEATURE_SAVE_LOG
#ifdef SUPPORT_LOG_TO_TEMP_FILE
		if(vChar  == 0){
			snprintf(loglevel, sizeof(loglevel), "%hhu", 7);
			argv[5] = loglevel;
		}
		else
#endif
			argv[idx++] = "-w";
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_YUEME)
	argv[idx++] = "-N";
	argv[idx++] = "1000";
#endif
	if(log_file > 0)
	{
		argv[idx++] = "-f";
		argv[idx++] = log_file_num;
	}

	argv[idx] = NULL;

	mib_get_s(MIB_ADSL_DEBUG, &vChar, sizeof(vChar));
	if(vChar==1){
#if defined(CONFIG_USER_BUSYBOX_KLOGD) || defined(USE_BUSYBOX_KLOGD)
		TRACE(STA_SCRIPT, "/bin/slogd %s %s %s %s %s ...\n", argv[1], argv[2], argv[3], argv[4], argv[5]);
		printf("/bin/slogd %s %s %s %s %s ...\n", argv[1], argv[2], argv[3], argv[4], argv[5]);
		do_nice_cmd("/bin/slogd", argv, 0);
		va_niced_cmd("/bin/klogd",1,0,"-n");
		va_niced_cmd("/bin/adslctrl",2,0,"debug","10");
#endif
	}
	else{
		mib_get_s(MIB_SYSLOG, (void *)&vChar, sizeof(vChar));
#ifndef SUPPORT_LOG_TO_TEMP_FILE
		if(vChar==1)
#endif
		{
			TRACE(STA_SCRIPT, "/bin/slogd %s %s %s %s %s ...\n", argv[1], argv[2], argv[3], argv[4], argv[5]);
			printf("/bin/slogd %s %s %s %s %s ...\n", argv[1], argv[2], argv[3], argv[4], argv[5]);
			do_nice_cmd("/bin/slogd", argv, 0);
#if defined(CONFIG_USER_BUSYBOX_KLOGD) || defined(USE_BUSYBOX_KLOGD)
			va_niced_cmd("/bin/klogd",1,0,"-n");
#endif
		}
	}
#endif

	return 1;
}
#endif

#ifdef DEFAULT_GATEWAY_V2
int ifExistedDGW(void)
{
	char buff[256];
	int flgs;
	struct in_addr dest, mask;
	FILE *fp;
	if (!(fp = fopen("/proc/net/route", "r"))) {
		printf("Error: cannot open /proc/net/route - continuing...\n");
		return -1;
	}
	fgets(buff, sizeof(buff), fp);
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		if (sscanf(buff, "%*s%x%*x%x%*d%*d%*d%x", &dest, &flgs, &mask) != 3) {
			printf("Unsuported kernel route format\n");
			fclose(fp);
			return -1;
		}
		if ((flgs & RTF_UP) && dest.s_addr == 0 && mask.s_addr == 0) {
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}
#endif

#ifdef CONFIG_USER_PPPOMODEM
#ifdef CONFIG_USER_PPPD
/* return value:
 *	  0: Success
 *	 -1: Fail
 */
int setup_ppp_modem_conf(MIB_WAN_3G_T *p, unsigned char pppidx, char *ttyUSB){
	char filename[256]={0};
	FILE *fd=NULL;
	DIR *dir=NULL;
	char ip_script_path[256]={0};
	char ppp_name[32]={0};

	if(!p){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return -1;
	}

	snprintf(ppp_name, sizeof(ppp_name), "ppp%d", pppidx);

	dir = opendir(POM_CONFIG_FOLDER);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", POM_CONFIG_FOLDER);
	}
	if(dir)closedir(dir);

	dir = opendir(PPPD_PEERS_FOLDER);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_PEERS_FOLDER);
	}
	if(dir)closedir(dir);

	// Step 1: ISP option
	sprintf( filename, "%s%d", POM_ISP_OPTION, pppidx );
	fd=fopen( filename, "w" );
	if(fd==NULL){
		fprintf(stderr, "[%s] Create %s Fail\n", __FUNCTION__, filename);
		return -1;
	}

	fprintf( fd, "file %s%d\n", POM_OPTION_FILE, pppidx);
	fprintf( fd, "connect \"/bin/chat -v -s -f %s%d -r %s%d\"\n", POM_CHAT_SCRIPT, pppidx, POM_CHAT_REPORT, pppidx);
	fclose(fd);

	// Step 2: PPPoM option
	sprintf( filename, "%s%d", POM_OPTION_FILE, pppidx );
	fd=fopen( filename, "w" );
	if(fd==NULL){
		fprintf(stderr, "[%s] Create %s Fail\n", __FUNCTION__, filename);
		return -1;
	}

	// ppp over modem option
	fprintf( fd, "%s\n", ttyUSB);	// get from mnet
	fprintf( fd, "lock\n");
	fprintf( fd, "crtscts\n");
	fprintf( fd, "modem\n");

	// ppp option
	switch(p->auth){
	case PPP_AUTH_AUTO:
		break;
	case PPP_AUTH_PAP:
		fprintf( fd, "require-pap\n");
		break;
	case PPP_AUTH_CHAP:
		fprintf( fd, "require-chap\n");
		break;
	case PPP_AUTH_MSCHAP:
		fprintf( fd, "require-mschap\n");
		break;
	case PPP_AUTH_MSCHAPV2:
		fprintf( fd, "require-mschap-v2\n");
		break;
	case PPP_AUTH_NONE:
	default:
		fprintf( fd, "noauth\n");
		break;
	}
	if(p->auth != PPP_AUTH_NONE){
		if(p->username[0])
			fprintf( fd, "user %s\n", p->username);
		fprintf( fd, "hide-password\n");
	}

	if (p->ctype == CONTINUOUS) // Continuous
		fprintf( fd, "persist\n");
	else if (p->ctype == CONNECT_ON_DEMAND) { // On-demand
		fprintf( fd, "demand\nidle %u\n", p->idletime);
	}

	if((p->ctype == CONTINUOUS) ||(p->ctype == CONNECT_ON_DEMAND))
		fprintf( fd, "holdoff 5\n");

	if(p->dgw)
		fprintf( fd, "defaultroute\n");

	fprintf( fd, "maxfail 0\n");

	fprintf( fd, "noipdefault\n");
	fprintf( fd, "usepeerdns\n");

	if(p->mtu != 0){
		fprintf( fd, "mtu %u\n", p->mtu);
		fprintf( fd, "mru %u\n", p->mtu);
	}else{
		fprintf( fd, "mtu 1492\n");	// PPPoE max mtu
		fprintf( fd, "mru 1492\n");	// PPPoE max mru
	}

#ifdef CONFIG_PPP_MULTILINK
	fprintf( fd, "multilink\n");
#endif

	// those option add from https://wiki.archlinux.org/index.php/3G_and_GPRS_modems_with_pppd
	fprintf( fd, "passive\n");
	fprintf( fd, "novj\n");

	//for debug
	if(p->debug){
		fprintf( fd, "nodetach\n");
		fprintf( fd, "dump\n");
		fprintf( fd, "debug\n");
	}
	fclose(fd);

	// Step 3: Chat script
	sprintf( filename, "%s%d", POM_CHAT_SCRIPT, pppidx );
	fd=fopen( filename, "w" );
	if(fd==NULL){
		fprintf(stderr, "[%s] Create %s Fail\n", __FUNCTION__, filename);
		return -1;
	}
	fprintf( fd, "ABORT 'BUSY'\n" );
	fprintf( fd, "ABORT 'ERROR'\n" );
	fprintf( fd, "ABORT 'NO CARRIER'\n" );
	fprintf( fd, "ABORT 'NO DIALTONE'\n" );
	fprintf( fd, "ABORT 'NO DIAL TONE'\n" );
	fprintf( fd, "TIMEOUT 60\n" );
	fprintf( fd, "'' ATZ\n" );
	fprintf( fd, "OK AT+CFUN=1\n" );

	if((p->pin != NO_PINCODE))
	{
		fprintf( fd, "OK AT+CPIN?\n" );
		fprintf( fd, "TIMEOUT 5\n" );
		fprintf( fd, "READY-AT+CPIN=%u-", p->pin );
	}
	if(p->apn && (strlen(p->apn)>0))
		fprintf( fd, "OK 'AT+CGDCONT=1,\"IP\",\"%s\"'\n", p->apn );
	fprintf( fd, "OK ATDT%s\n", p->dial );
	fprintf( fd, "TIMEOUT 60\n" );
	fprintf( fd, "CONNECT ''\n" );
	fclose(fd);

	dir = opendir(PPPD_INIT_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_INIT_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	// Step 4: PPP ip-up/ip-down script (only for IPv4 now)
	snprintf(ip_script_path, sizeof(ip_script_path), "%s/"PPPD_INIT_SCRIPT_NAME, PPPD_INIT_SCRIPT_DIR, ppp_name);
	if (fd = fopen(ip_script_path, "w+")) {
		fprintf(fd, "#!/bin/sh\n\n");

		fprintf(fd, "######### Script Globle Variable ###########\n");
		fprintf(fd, "PIDFILE=%s/${IFNAME}.pid\n", (char *)PPPD_CONFIG_FOLDER);
		fprintf(fd, "BACKUP_PIDFILE=%s/${IFNAME}.pid_back\n", (char *)PPPD_CONFIG_FOLDER);
		fprintf(fd, "######### Script Globle Variable ###########\n\n");

		fprintf(fd, "## PID file be remove when PPP link_terminated\n");
		fprintf(fd, "## it will cause delete ppp daemon fail, So backup it.\n");
		fprintf(fd, "if [ -f ${PIDFILE} ]; then\n");
			fprintf(fd, "\tcp -af ${PIDFILE} ${BACKUP_PIDFILE}\n");
		fprintf(fd, "fi\n");

		fclose(fd);
		fd=NULL;
		if(chmod(ip_script_path, 484) != 0)
			printf("chmod failed: %s %d\n", __func__, __LINE__);
	}

	dir = opendir(PPPD_V4_UP_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_V4_UP_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	// Step 4: PPP ip-up/ip-down script (only for IPv4 now)
	snprintf(ip_script_path, sizeof(ip_script_path), "%s/"PPPD_V4_UP_SCRIPT_NAME, PPPD_V4_UP_SCRIPT_DIR, ppp_name);
	if (fd = fopen(ip_script_path, "w+")) {
		fprintf(fd, "#!/bin/sh\n\n");
		// create dns resolv.conf.{ifname} file for dnsmasq
		fprintf(fd, "rm %s/resolv.conf.${IFNAME}\n", (char *)PPPD_CONFIG_FOLDER);
		fprintf(fd, "if [ -n ${DNS1} ]; then echo \"${DNS1}@${IPLOCAL}\" >> %s/resolv.conf.${IFNAME}; fi\n", (char *)PPPD_CONFIG_FOLDER);
		fprintf(fd, "if [ -n ${DNS2} ]; then echo \"${DNS2}@${IPLOCAL}\" >> %s/resolv.conf.${IFNAME}; fi\n", (char *)PPPD_CONFIG_FOLDER);
		fclose(fd);
		fd=NULL;
		if(chmod(ip_script_path, 484) != 0)
			printf("chmod failed: %s %d\n", __func__, __LINE__);
	}

	dir = opendir(PPPD_V4_DOWN_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_V4_DOWN_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	snprintf(ip_script_path, sizeof(ip_script_path), "%s/"PPPD_V4_DOWN_SCRIPT_NAME, PPPD_V4_DOWN_SCRIPT_DIR, ppp_name);
	if (fd = fopen(ip_script_path, "w+")) {
		fprintf(fd, "#!/bin/sh\n\n");

		// clean dns resolv.conf.{ifname} file
		fprintf(fd, "rm %s/resolv.conf.${IFNAME}\n", (char *)PPPD_CONFIG_FOLDER);

		fclose(fd);
		fd=NULL;
		if(chmod(ip_script_path, 484) != 0)
			printf("chmod failed: %s %d\n", __func__, __LINE__);
	}

	return 0;
}
#endif //CONFIG_USER_PPPD

MIB_WAN_3G_T *getPoMEntryByIfname(char *ifname, MIB_WAN_3G_T *p, int *entryIndex)
{
	int i,num;
	unsigned char mib_ifname[32];
	if( (p==NULL) || (ifname==NULL) ) return NULL;
	num = mib_chain_total( MIB_WAN_3G_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_WAN_3G_TBL, i, (void*)p ))
			continue;

		snprintf(mib_ifname, sizeof(mib_ifname), "ppp%d", MODEM_PPPIDX_FROM + i);
		if(!strcmp(mib_ifname, ifname))
		{
			if(entryIndex) *entryIndex = i;
			return p;
		}
	}
	return NULL;
}

static void _wan3g_start_each( MIB_WAN_3G_T *p, unsigned char pppidx)
{
    //printf( "%s: enter\n", __FUNCTION__ );
    //DEBUGMODE(STA_INFO|STA_SCRIPT|STA_WARNING|STA_ERR);
    char ifIdx[3]={0}, pppif[6]={0};
#ifdef CONFIG_USER_PPPD
	char pidfile[256]={0};
	char cmd[128]={0};
	char isp_name[32]={0};
#endif //CONFIG_USER_PPPD

    if(p && p->enable)
    {
	snprintf(ifIdx, 3, "%u", pppidx);
	snprintf(pppif, 6, "ppp%u", pppidx);

#ifdef CONFIG_USER_PPPD
#if defined(CONFIG_USER_RTK_SYSCONF)
	fprintf(stderr, "lanuch ppp daemon when serial usb attach on mnet daemon.\n");
#else // CONFIG_USER_RTK_SYSCONF
	fprintf(stderr, "please enable CONFIG_USER_RTK_SYSCONF\n");
#endif // CONFIG_USER_RTK_SYSCONF
#endif //CONFIG_USER_PPPD
#ifdef CONFIG_USER_SPPPD
	struct data_to_pass_st msg;
	int pppdbg;

	if(p->ctype!=MANUAL)
		snprintf(msg.data, BUF_SIZE, "spppctl add %s", ifIdx);
	else
		snprintf(msg.data, BUF_SIZE, "spppctl new %s", ifIdx);

	//set device
	//snprintf(msg.data, BUF_SIZE, "%s pppomodem /dev/ttyUSB0", msg.data);
	strncat(msg.data, " pppomodem auto", BUF_SIZE);

	//set pin code
	if( p->pin!=NO_PINCODE )
		snprintf(msg.data+strlen(msg.data), BUF_SIZE-strlen(msg.data), " simpin %04u", p->pin);

	//set apn
	if( strlen(p->apn) )
		snprintf(msg.data+strlen(msg.data), BUF_SIZE-strlen(msg.data), " apn %s", p->apn);

	//set dial
	snprintf(msg.data+strlen(msg.data), BUF_SIZE-strlen(msg.data), " dial %s", p->dial);

	// Set Authentication Method
	//printf( "p->auth=%d, %d, %d\n", p->auth, PPP_AUTH_NONE, p->auth==PPP_AUTH_NONE );
	if(p->auth==PPP_AUTH_NONE)
	{
		//skip or???
	}else{
		snprintf(msg.data+strlen(msg.data), BUF_SIZE-strlen(msg.data), " auth %s", ppp_auth[p->auth]);
		snprintf(msg.data+strlen(msg.data), BUF_SIZE-strlen(msg.data), " username %s password %s", p->username, p->password);
		printf("%s:%d auth %s\n", __FUNCTION__, __LINE__,ppp_auth[p->auth]);
	}

	//set default gateway/ mtu
	snprintf(msg.data+strlen(msg.data), BUF_SIZE-strlen(msg.data), " gw %d mru %u",	p->dgw, p->mtu);

	// set PPP debug
	pppdbg = pppdbg_get(pppidx);
	snprintf(msg.data+strlen(msg.data), BUF_SIZE-strlen(msg.data), " debug %d", pppdbg);
	//snprintf(msg.data, BUF_SIZE, "%s debug 1", msg.data);

	//paula, set 3g backup PPP
	snprintf(msg.data+strlen(msg.data), BUF_SIZE-strlen(msg.data), " backup %d", p->backup);

	if(p->backup)
		snprintf(msg.data+strlen(msg.data), BUF_SIZE-strlen(msg.data), " backup_timer %d", p->backup_timer);

	if(p->ctype == CONTINUOUS) // Continuous
	{
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		//printf("\ncmd=%s\n",msg.data);
		write_to_pppd(&msg);

		// set the ppp keepalive timeout
#ifdef CONFIG_CMCC
		snprintf(msg.data, BUF_SIZE,"spppctl katimer 30");
#else
		snprintf(msg.data, BUF_SIZE,"spppctl katimer 20");
#endif
		TRACE(STA_SCRIPT, "%s\n", msg.data);

		write_to_pppd(&msg);
		printf("PPP Connection (Continuous)...\n");
	}else if(p->ctype==CONNECT_ON_DEMAND) // On-demand
	{
		snprintf(msg.data+strlen(msg.data), BUF_SIZE-strlen(msg.data), " timeout %u", p->idletime);
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
		printf("PPP Connection (On-demand)...\n");
	}
	else if(p->ctype==MANUAL) // Manual
	{
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
		printf("PPP Connection (Manual)...\n");
	}
#endif	/* CONFIG_USER_SPPPD */

	if(p->napt)
	{	// Enable NAPT
		va_cmd(IPTABLES, 8, 1, "-t", "nat", FW_ADD, NAT_ADDRESS_MAP_TBL,
			"-o", pppif, "-j", "MASQUERADE");
	}

    }

    return;
}

static void _wan3g_stop_each( MIB_WAN_3G_T *p, unsigned char pppidx)
{
    //printf( "%s: enter\n", __FUNCTION__ );
    //DEBUGMODE(STA_INFO|STA_SCRIPT|STA_WARNING|STA_ERR);
   	char pppif[6]={0};

    if(p && p->enable)
    {
	snprintf(pppif, 6, "ppp%u", pppidx);
#ifdef CONFIG_USER_PPPD
	char pidfile[32]={0};

	// kill script
	snprintf(pidfile, sizeof(pidfile), "%s%u", POM_LAUNCH_SCRIPT_PID_FILE, pppidx);
	kill_by_pidfile(pidfile, SIGTERM);

	// kill pppd
	snprintf(pidfile, 32, "%s/ppp%u.pid", PPPD_CONFIG_FOLDER, pppidx);
	kill_by_pidfile(pidfile, SIGTERM);
#endif //CONFIG_USER_PPPD
#ifdef CONFIG_USER_SPPPD
	struct data_to_pass_st msg;
	// spppctl del 0
	snprintf(msg.data, BUF_SIZE, "spppctl del %u", pppidx);
	TRACE(STA_SCRIPT, "%s\n", msg.data);
	write_to_pppd(&msg);

	//down interface
	va_cmd(IFCONFIG, 2, 1, pppif, "down");
#endif /* CONFIG_USER_SPPPD */

	if(p->napt==1)
	{	// Disable NAPT
		va_cmd(IPTABLES, 8, 1, "-t", "nat", FW_DEL, NAT_ADDRESS_MAP_TBL,
			"-o", pppif, "-j", "MASQUERADE");
	}
    }

    return;
}

static MIB_WAN_3G_T mib_wan_3g_default_table[] = {
	{
	 /*enable */ 0,
	 /*auth */ PPP_AUTH_NONE,
	 /*ctype */ CONTINUOUS,
	 /*napt */ 1,
	 /*pin */ NO_PINCODE,
	 /*idletime */ 60,
	 /*mtu */ 1500,
	 /*dgw */ 1,
	 /*apn */ "internet",
	 /*dial */ "*99#",
	 /*username */ "",
	 /*password */ "",
	 //paula, 3g backup PPP
	 /*backup */ 0,
	 /*backup timer */ 60
	 }
};

/*
 *	Return value:
 *		0: disabled
 *		1: enabled
 */

int wan3g_start(void)
{
	unsigned char pppidx;
	MIB_WAN_3G_T entry, *p = &entry;
	int ret=0;

	pppidx = 0;
	if (!mib_chain_get(MIB_WAN_3G_TBL, pppidx, (void *)p)) {
		//printf("No entry in MIB_WAN_3G, add one\n");
		p = &mib_wan_3g_default_table[0];
		ret = mib_chain_add(MIB_WAN_3G_TBL, (void *)p);
		if(ret == 0){
			printf("Error! MIB_WAN_3G_TBL Add chain record.\n");
		}else if(ret == -1){
			printf("Error! MIB_WAN_3G_TBL Table Full.\n");
		}
	}

	_wan3g_start_each(p, MODEM_PPPIDX_FROM + pppidx);

	if (p->enable) {
		//setup_ipforwarding(1);
		return 1;
	}

	return 0;
}

/*
 *	Return value:
 *		0: disabled
 *		1: enabled
 */
int wan3g_enable(void)
{
	unsigned char pppidx;
	MIB_WAN_3G_T entry, *p = &entry;
	int ret=0;

	pppidx = 0;
	if (!mib_chain_get(MIB_WAN_3G_TBL, pppidx, (void *)p)) {
		//printf("No entry in MIB_WAN_3G, add one\n");
		p = &mib_wan_3g_default_table[0];
		ret = mib_chain_add(MIB_WAN_3G_TBL, (void *)p);
		ret = mib_chain_add(MIB_WAN_3G_TBL, mib_wan_3g_default_table);
		if(ret == 0){
			printf("Error! MIB_WAN_3G_TBL Add chain record.\n");
		}else if(ret == -1){
			printf("Error! MIB_WAN_3G_TBL Table Full.\n");
		}
	}

	if (p->enable) {
		//setup_ipforwarding(1);
		return 1;
	}

	return 0;
}

void wan3g_stop(void)
{
	unsigned char pppidx;
	MIB_WAN_3G_T entry, *p = &entry;
	int ret=0;
	pppidx = 0;
	if (!mib_chain_get(MIB_WAN_3G_TBL, pppidx, p)) {
		ret = mib_chain_add(MIB_WAN_3G_TBL, mib_wan_3g_default_table);
		if(ret == 0){
			printf("Error! MIB_WAN_3G_TBL Add chain record.\n");
		}else if(ret == -1){
			printf("Error! MIB_WAN_3G_TBL Table Full.\n");
		}
	}

	_wan3g_stop_each(p, MODEM_PPPIDX_FROM + pppidx);

	return;
}
#endif //CONFIG_USER_PPPOMODEM

#ifdef REMOTE_HTTP_ACCESS_AUTO_TIMEOUT
void rtk_fw_update_rmtacc_auto_logout_time(void)
{
	unsigned int sessionOutTime;

	mib_get(MIB_RMTACC_SESSION_TIMEOUT, (void *)&sessionOutTime);
	remote_user_auto_logout_time = sessionOutTime*60;
}
#endif

#ifdef _CWMP_MIB_ /*jiunming, mib for cwmp-tr069*/
#ifdef _TR111_STUN_
int addCWMPSTUNFirewall(){
	fprintf(stderr,"%s %d#############\n",__FUNCTION__,__LINE__);
	va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", "-A", (char *)FW_PREROUTING,
			"!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
			(char *)FW_DPORT, "7547", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
	return 0;
}
int removeCWMPSTUNFirewall(){
	fprintf(stderr,"%s %d#############\n",__FUNCTION__,__LINE__);
	va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", "-D", (char *)FW_PREROUTING,
			"!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
			(char *)FW_DPORT, "7547", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
	return 0;
}
#endif
int startCWMP(void)
{
	char vChar=0;
	char strPort[16];
	unsigned int conreq_port=0;
	unsigned char cwmp_aclEnable;

#ifdef CONFIG_RTK_DEV_AP
	int pid=0;
	//if cwmpClient exist, do not start cwmp
	pid = read_pid((char*)CWMP_PID);
	if(pid>0)
	{
		return 0;
	}
#endif

	//lan interface enable or disable
	mib_get_s(CWMP_LAN_IPIFENABLE, (void *)&vChar, sizeof(vChar));
	if(vChar==0)
	{
		va_cmd(IFCONFIG, 2, 1, BRIF, "down");
		printf("Disable br0 interface\n");
	}
	//eth0 interface enable or disable
	mib_get_s(CWMP_LAN_ETHIFENABLE, (void *)&vChar, sizeof(vChar));
	if(vChar==0)
	{
		va_cmd(IFCONFIG, 2, 1, ELANIF, "down");
		printf("Disable eth0 interface\n");
	}

#if 0
#if defined(REMOTE_ACCESS_CTL) || defined(IP_ACL)
	mib_get_s(CWMP_ACL_ENABLE, (void *)&cwmp_aclEnable, sizeof(cwmp_aclEnable));
	if (cwmp_aclEnable == 0) { // ACL Capability is disabled
		/*add a wan port to pass */
		mib_get_s(CWMP_CONREQ_PORT, (void *)&conreq_port, sizeof(conreq_port));
		if(conreq_port==0) conreq_port=7547;
		sprintf(strPort, "%u", conreq_port );
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", (char *)FW_ADD, (char *)FW_CWMP_ACL,
			"!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
	}
#endif
#endif

	set_TR069_Firewall(1);

	/*start the cwmpClient program*/
	mib_get_s(CWMP_FLAG, (void *)&vChar, sizeof(vChar));
	if( vChar&CWMP_FLAG_AUTORUN )
	{
		va_niced_cmd( "/bin/cwmpClient", 0, 0 );
	}

	return 0;
}
#endif

int getOUIfromMAC(char* oui)
{
	unsigned char macAddr[MAC_ADDR_LEN];

	if (oui==NULL)
		return -1;

	mib_get_s( MIB_ELAN_MAC_ADDR, (void *)macAddr, sizeof(macAddr));
	sprintf(oui,"%02X%02X%02X",macAddr[0],macAddr[1],macAddr[2]);
	return 0;
}

void bypassBlock(FILE *fp)
{
	char temps[0x100];
	int found_pair = 0;

	while (!found_pair) {
		fgets(temps,0x100,fp);
		if (strchr(temps, '{')) {
			bypassBlock(fp);
		}
		if (strchr(temps, '}')) {
			found_pair = 1;
		}
	}
}

int hex_to_int(char *hex)
{
    int val = 0;
    while (*hex)
	{
        char byte = *hex++;
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}

int power(int base, int exponent)
{
    int num=1;
	int i=0;
    for(i = 0; i < exponent; i++)
    {
        num*= base;
    }
    return num;
}

#if (defined(CONFIG_USER_MAP_E) || defined(CONFIG_USER_MAP_T))
/*
 * Function:
 *	 get_map_info, get_lw4o6_info
 *
 * Propose:
 *	 To get MAP-E, MAP-T, LW4O6 information from DHCPv6 Server.
 *
 * Description:
 *	 Parse the data in dhcpv6 option 94,95,96 (encapsulated-options include sub-options 89,90,91,92,93),
 *   and save parameters in the structure S46_DHCP_INFO_T.
 *
 *	 option 94: S46 MAP-E Container (include sub-options 89,90,93)
 *	 option 95: S46 MAP-T Container (include sub-options 89,91,93)
 *	 option 96: S46 LW4O6 Container (include sub-options 90,92,93)
 *
 *		 0					 1 					 2					 3
 *		 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *		|	  OPTION_S46_CONT_TYPE      |		  option-length 		|
 *		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *		|																|
 *		.			 *encapsulated-options (variable length) 			.
 *		.																.
 *		+---------------------------------------------------------------+
 *							 S46 Container Option
 *
 * Parameter:
 *   map_type: LEASE_GET_MAPE, LEASE_GET_MAPT
 *	 str: string in hex
 *		  raw data for example
 *   	  0:59:0:18:3c:10:18:a:0:1:0:3c:20:1:d:b8:a1:c0:55:60:0:5d:0:4:2:6:
 *   	  d0:0:0:5a:0:10:20:1:d:b8:0:1:12:34:0:0:0:0:0:0:0:1
 *	pInfo: structure of S46_DHCP_INFO_Tp
 *
 * Return:
 *	  1 : success
 *   -1 : error
 */
int get_map_info(int map_type, char *str, S46_DHCP_INFO_Tp pInfo)
{
	char *delim = ":";
	char *pch[256]={0};
	int i=0,j=0,pos=0;
#if (defined(CONFIG_USER_MAP_E) || defined(CONFIG_USER_MAP_T))
	int raw_addr_str[4]={0};
	char addr_str[INET_ADDRSTRLEN]={0};
	char addr_prefix_str[INET_ADDRSTRLEN]={0};
	char addr6_str[INET6_ADDRSTRLEN]={0};
#endif
#ifdef CONFIG_USER_MAP_E
	char br_addr_str[INET6_ADDRSTRLEN]={0};
#endif
#ifdef CONFIG_USER_MAP_T
	char dmr_prefix_str[INET6_ADDRSTRLEN]={0};
#endif
	char tmp[128]={0};
	int addr6_buf_len=0;
	S46_DHCP_INFO_T tempInfo={0};

	if(map_type==0 && str==NULL)
	{
		printf("[%s:%d] map-e/map-t argument failed !\n", __FUNCTION__, __LINE__);
		return -1;
	}

	/* to chop the hex string into array */
	pch[0] = strtok(str,delim);
	while(pch[i]!=NULL)
	{
		i++;
		pch[i] = strtok(NULL,delim);
	}

#if (defined(CONFIG_USER_MAP_E) || defined(CONFIG_USER_MAP_T))
	/*
	 *    0					  1					  2					  3
	 *	  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 *	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *	 |		 OPTION_S46_RULE		|		  option-length 		|
	 *	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *	 |	  flags 	|	  ea-len	|  prefix4-len	| ipv4-prefix	|
	 *	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *	 |				   (continued)					|  prefix6-len	|
	 *	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *	 |							ipv6-prefix 						|
	 *	 |						(variable length)						|
	 *	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *	 |																|
	 *	 .						 S46_RULE-options						.
	 *	 .																.
	 *	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *
	 *						  S46 Rule Option format (89)
	 */

	/* option code	*/
	tempInfo.s46_rule.option_code = hex_to_int(strcat(*(pch+(j++)),*(pch+(j++))));

	if(tempInfo.s46_rule.option_code == D6O_S46_RULE)
	{
		/* option length */
		tempInfo.s46_rule.option_len = hex_to_int(strcat(*(pch+(j++)),*(pch+(j++))));
		/* flags */
		tempInfo.s46_rule.ea_flag= hex_to_int(*(pch+(j++)));
		/* EA-bit length */
		tempInfo.s46_rule.ea_len= hex_to_int(*(pch+(j++)));
		/* ipv4 prefix length */
		tempInfo.s46_rule.v4_prefix_len= hex_to_int(*(pch+(j++)));
		/* ipv4 address */
		raw_addr_str[0]= hex_to_int(*(pch+(j++)));
		raw_addr_str[1]= hex_to_int(*(pch+(j++)));
		raw_addr_str[2]= hex_to_int(*(pch+(j++)));
		raw_addr_str[3]= hex_to_int(*(pch+(j++)));

		snprintf(addr_str, INET_ADDRSTRLEN, "%d.%d.%d.%d",
			raw_addr_str[0], raw_addr_str[1], raw_addr_str[2], raw_addr_str[3]);
		inet_pton(PF_INET, addr_str, &(tempInfo.s46_rule.map_addr));

		/* ipv4 prefix addr */
		unsigned long mask = (0xFFFFFFFF << (32 - tempInfo.s46_rule.v4_prefix_len)) & 0xFFFFFFFF;
		snprintf(addr_prefix_str, INET_ADDRSTRLEN,"%lu.%lu.%lu.%lu",
				mask >> 24 & raw_addr_str[0], (mask >> 16) & 0xFF & raw_addr_str[1],
				(mask >> 8) & 0xFF & raw_addr_str[2], mask & 0xFF & raw_addr_str[3]);
		inet_pton(PF_INET, addr_prefix_str, &(tempInfo.s46_rule.map_addr_subnet));

		/* ipv6 prefix length */
		tempInfo.s46_rule.v6_prefix_len= hex_to_int(*(pch+(j++)));

		/* ipv6 prefix */
		memset(tmp, '\0', sizeof(tmp));
		addr6_buf_len = (tempInfo.s46_rule.v6_prefix_len)/8;

		/*
		 * ipv6-prefix value has variable length, sure that how many arrays we need to parse.
		 * if the prefix length is not multiple of 8, need an additional array to store the reminder.
		 */
		if((tempInfo.s46_rule.v6_prefix_len)%8)
			addr6_buf_len += 1;

		pos = j;
		for(j; j<pos+addr6_buf_len ;j++)
		{
			/*
			 *	Message in hex would be ignored if the first word is "0", we recover it to the orginal
			 *	two words from "d" to "0d" (for example) if the string length is 1.
			 */
			if(strlen(*(pch+j))==1){
				snprintf(tmp, sizeof(tmp), "%s%s", "0", *(pch+j));
				*(pch+j) = tmp ;
				}
				strcat(addr6_str,*(pch+j));

				/* Add a ":" to present in ipv6 address once we have captured 2 words. */
				if((j-pos)%2)
				strcat(addr6_str,":");
		}
		/*
		 *  Be careful that 2001:0db8:100f:ab00:: is different from 2001:0db8:100f:ab::,
		 *  In the case 2001:0db8:100f:ab00::, there are 7 buffers sent from server,
		 *  i.e., "20","01","0d","d8","10","0f","ab".
		 *  If there is no extra process, the ipv6 string would be 2001:0db8:100f:ab::
		 *  Thus, if the numbers of buffer are odd we add "00" with ":" at the end.
		 */
		if(addr6_buf_len%2)
			strcat(addr6_str, "00:");

		/*
		 *  Continue from above, now the ipv6 string is 2001:0db8:100f:ab00:, just add a
		 *  ":" at the end to present in prefix. Note that if prefix is given in 128 bits,
		 *  we don't need to add it.
		 */
		if(tempInfo.s46_rule.v6_prefix_len==128)
			addr6_str[strlen(addr6_str)-1] = '\0';
		else
			strcat(addr6_str, ":");

		inet_pton(PF_INET6, addr6_str, &tempInfo.s46_rule.map_addr6);

		memcpy(pInfo, &tempInfo, sizeof(S46_DHCP_INFO_T));
	}
#endif

#if (defined(CONFIG_USER_MAP_E) || defined(CONFIG_USER_MAP_T))
	/*
	 *   0                   1                   2                   3
	 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *  |     OPTION_S46_PORTPARAMS     |         option-length         |
	 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *  |   offset      |    PSID-len   |              PSID             |
	 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *
	 *					  S46 Port Parameters Option (93)
	 */

	/* option code	*/
	tempInfo.s46_portparams.option_code = hex_to_int(strcat(*(pch+(j++)),*(pch+(j++))));

	if(tempInfo.s46_portparams.option_code == D6O_S46_PORTPARAMS)
	{
		int psid_decimal=0;
		int psid_binary[16]={0};
		char psid_hex_string[16]={0};

		/* option length */
		tempInfo.s46_portparams.option_len = hex_to_int(strcat(*(pch+(j++)),*(pch+(j++))));
		/* PSID offset */
		tempInfo.s46_portparams.psid_offset= hex_to_int(*(pch+(j++)));
		/* PSID length */
		tempInfo.s46_portparams.psid_len= hex_to_int(*(pch+(j++)));

		/* PSID */
		/*
		 * Since PSID is 16 bit in integer based on the PSID-len. First, we need to translate raw
		 * data(hex) to integer. Sencond, translate integer to binary. Finally, based on PSID-len
		 * translate the specific length of the binary to integer. for example: psid-len=6, we get
		 * raw data array "d0" and "0". step1: supplement "0" to "00". step2: strcat "d0" and
		 * "00" to "d000" and tranlate to integer. step3: use the integer to translate to binary
		 * "101000000000000". step4: use 6 bits, i.e., "110100" translate to integer 52.
		 */
		 pos = j;
		 /* psid is in 16 bits (2 bytes) long so it will be put in 2 arrays */
		 for(j;j<pos+2;j++)
		{
			 /*
			  *  Message in hex would be ignored if the first word is "0", we recover it to the orginal
			  *  two words from "d" to "0d" (for example) if the string length is 1.
			  */
			 if(strlen(*(pch+j))==1){
				 snprintf(tmp, sizeof(tmp), "%s%s", "0", *(pch+j));
				 *(pch+j) = tmp ;
			 }
			 strcat(psid_hex_string, *(pch+j));
		}

		/* translate hex to integer */
		psid_decimal = hex_to_int(psid_hex_string);
		/* translate integer to binary */
		for(i=16;i>0;i--)
		{
			psid_binary[i]=psid_decimal%2;
			psid_decimal/=2 ;
		}
		/* translate binary to integer based on the PSID length */
		for(i=1;i<=tempInfo.s46_portparams.psid_len;i++)
		{
			tempInfo.s46_portparams.psid_val += psid_binary[i] * power(2,(tempInfo.s46_portparams.psid_len-i));
		}

		memcpy(pInfo, &tempInfo, sizeof(S46_DHCP_INFO_T));
	}
#endif
#ifdef CONFIG_USER_MAP_E
	/*
	 *     0                   1                   2                   3
	 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *    |         OPTION_S46_BR         |         option-length         |
	 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *    |                      br-ipv6-address                          |
	 *    |                                                               |
	 *    |                                                               |
	 *    |                                                               |
	 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *
	 *		     		           S46 BR Option (90)
	 */

	if(map_type == LEASE_GET_MAPE){
		/* option code */
		tempInfo.s46_br.option_code = hex_to_int(strcat(*(pch+(j++)),*(pch+(j++))));

		if(tempInfo.s46_br.option_code == D6O_S46_BR)
		{
			/* option length */
			tempInfo.s46_br.option_len= hex_to_int(strcat(*(pch+(j++)),*(pch+(j++))));

			memset(tmp, '\0', sizeof(tmp));

			pos = j;
			for(j; j<pos+tempInfo.s46_br.option_len;j++)
			{
				/*
				 *	Message in hex would be ignored if the first word is "0", we recover it to the orginal
				 *	two words from "d" to "0d" (for example) if the string length is 1.
				 */
				if(strlen(*(pch+j))==1){
					snprintf(tmp, sizeof(tmp), "%s%s", "0", *(pch+j));
					*(pch+j) = tmp ;
					}
				strcat(br_addr_str,*(pch+j));

				/* Add a ":" to present in ipv6 address once we have captured 2 words. */
				if((j-pos)%2)
					strcat(br_addr_str,":");
			}
			/* delete the last character ":" and save into in6_addr struture */
			br_addr_str[strlen(br_addr_str)-1] = '\0';

			inet_pton(PF_INET6, br_addr_str, &tempInfo.s46_br.mape_br_addr);

			memcpy(pInfo, &tempInfo, sizeof(S46_DHCP_INFO_T));
		}
	}
	else
#endif
#ifdef CONFIG_USER_MAP_T
	/*
	 * 	  0                   1                   2                   3
	 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *   |        OPTION_S46_DMR         |         option-length         |
	 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *   |dmr-prefix6-len|            dmr-ipv6-prefix                    |
	 *   +-+-+-+-+-+-+-+-+           (variable length)                   |
	 *   .                                                               .
	 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *
	 *							S46 DMR Option (91)
	 */

	if(map_type == LEASE_GET_MAPT){
		/* option code */
		tempInfo.s46_dmr.option_code = hex_to_int(strcat(*(pch+(j++)),*(pch+(j++))));

		if(tempInfo.s46_dmr.option_code == D6O_S46_DMR)
		{
			/* option length */
			tempInfo.s46_dmr.option_len= hex_to_int(strcat(*(pch+(j++)),*(pch+(j++))));

			/* dmr prefix length */
			tempInfo.s46_dmr.dmr_prefix_len= hex_to_int(*(pch+(j++)));

			/* dmr prefix */
			memset(tmp, '\0', sizeof(tmp));
			addr6_buf_len = (tempInfo.s46_dmr.dmr_prefix_len)/8;

			/*
			 * DMR prefix value has variable length, sure that how many arrays we need to parse.
			 * if the prefix length is not multiple of 8, need an additional array to store the reminder.
			 */
			if((tempInfo.s46_dmr.dmr_prefix_len)%8)
				addr6_buf_len += 1;

			pos = j;
			for(j; j<pos+addr6_buf_len ; j++)
			{
				/*
				 *	Message in hex would be ignored if the first word is "0", we recover it to the orginal
				 *	two words from "d" to "0d" (for example) if the string length is 1.
				 */
				if(strlen(*(pch+j))==1){
					snprintf(tmp, sizeof(tmp), "%s%s", "0", *(pch+j));
					*(pch+j) = tmp ;
					}
				strcat(dmr_prefix_str,*(pch+j));

				/* Add a ":" to present in ipv6 address once we have captured 2 words. */
				if((j-pos)%2)
					strcat(dmr_prefix_str,":");
			}
			/*
			 *	Be careful that 2001:0db8:100f:ab00:: is different from 2001:0db8:100f:ab::,
			 *	In the case 2001:0db8:100f:ab00::, there are 7 buffers sent from server,
			 *	i.e., "20","01","0d","d8","10","0f","ab".
			 *	If there is no extra process, the ipv6 string would be 2001:0db8:100f:ab::
			 *	Thus, if the numbers of buffer are odd we add "00" with ":" at the end.
			 */
			if(addr6_buf_len%2)
				strcat(dmr_prefix_str, "00:");

			/*
			 *	Continue from above, now the ipv6 string is 2001:0db8:100f:ab00:, just add a
			 *	":" at the end to present in prefix. Note that if prefix is given in 128 bits,
			 *	we don't need to add it.
			 */
			if(tempInfo.s46_dmr.dmr_prefix_len==128)
				dmr_prefix_str[strlen(dmr_prefix_str)-1] = '\0';
			else
				strcat(dmr_prefix_str,":");

			inet_pton(PF_INET6, dmr_prefix_str, &tempInfo.s46_dmr.mapt_dmr_prefix);
		}
	}
#endif
	memcpy(pInfo, &tempInfo, sizeof(S46_DHCP_INFO_T));
	return 1 ;
}
#endif

#ifdef CONFIG_USER_LW4O6
int get_lw4o6_info(char *str, S46_DHCP_INFO_Tp pInfo)
{
	char *delim = ":";
	char *pch[256]={0};
	int i=0,j=0,pos=0;
	S46_DHCP_INFO_T tempInfo={0};
	char br_addr_str[INET6_ADDRSTRLEN]={0};
	char ipv4_addr_str[INET_ADDRSTRLEN]={0};
	char tmp[128]={0};
	int addr6_buf_len=0;
	char prefix6_str[INET6_ADDRSTRLEN]={0};
	int	psid_decimal=0;
	char psid_hex_string[16]={0};
	int psid_binary[16]={0};

	if(str==NULL){
		printf("[%s:%d] lw4o6 argument failed !\n", __FUNCTION__, __LINE__);
		return -1;
	}

	pch[0] = strtok(str, delim);
	while(pch[i]!= NULL){
		i++;
		pch[i] = strtok(NULL, delim);
	}

	/*
	 *  0                   1                   2                   3
     *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |         OPTION_S46_BR         |         option-length         |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                      br-ipv6-address                          |
     * |                                                               |
     * |                                                               |
     * |                                                               |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *
     *						   OPTION_S46_BR (90)
     */

	/* option code */
	tempInfo.s46_br.option_code =  hex_to_int(strcat(*(pch+(j++)),*(pch+(j++))));
	if(tempInfo.s46_br.option_code == D6O_S46_BR){
		/* option length */
		tempInfo.s46_br.option_len =  hex_to_int(strcat(*(pch+(j++)),*(pch+(j++))));

		pos = j ;
		for(j;j< pos+tempInfo.s46_br.option_len;j++){
			/*
			 *  Message in hex would be ignored if the first word is "0", we recover it to the orginal
			 *  two words from "d" to "0d" (for example) if the string length is 1.
			 */
			if(strlen(*(pch+j))==1){
				snprintf(tmp, sizeof(tmp), "%s%s", "0", *(pch+j));
				*(pch+j) = tmp;
			}
			strcat(br_addr_str,*(pch+j));

			/* Add a ":" to present in ipv6 address once we have captured 2 words. */
			if((j-pos)%2)
				strcat(br_addr_str,":");
		}

		/* After capturing all words, we need to replace the final character ":" */
		if(br_addr_str[strlen(br_addr_str)-1]==':')
			br_addr_str[strlen(br_addr_str)-1]= '\0';

		inet_pton(PF_INET6, br_addr_str, &(tempInfo.s46_br.lw4o6_aftr_addr));
		memcpy(pInfo, &tempInfo, sizeof(S46_DHCP_INFO_T));
	}

	/*
	 *  0                   1                   2                   3
     *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |      OPTION_S46_V4V6BIND      |         option-length         |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                         ipv4-address                          |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |bindprefix6-len|             bind-ipv6-prefix                  |
     * +-+-+-+-+-+-+-+-+             (variable length)                 |
     * .                                                               .
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                                                               |
     * .                      S46_V4V6BIND-options                     .
     * .                                                               .
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *
     *						OPTION_S46_V4V6BIND (92)
     */

	/* option code */
	tempInfo.s46_v4v6bind.option_code = hex_to_int(strcat(*(pch+(j++)), *(pch+(j++))));

	if(tempInfo.s46_v4v6bind.option_code == D6O_S46_V4V6BIND){
		/* option length */
		tempInfo.s46_v4v6bind.option_len = hex_to_int(strcat(*(pch+(j++)), *(pch+(j++))));

		/* ipv4 address */
		snprintf(ipv4_addr_str, sizeof(ipv4_addr_str), "%d.%d.%d.%d", hex_to_int(*(pch+(j++))), hex_to_int(*(pch+(j++))), hex_to_int(*(pch+(j++))), hex_to_int(*(pch+(j++))));
		inet_pton(PF_INET, ipv4_addr_str, &(tempInfo.s46_v4v6bind.lw4o6_v4_addr));

		/* ipv6 prefix length */
		tempInfo.s46_v4v6bind.prefix6_len = hex_to_int(*(pch+(j++)));

		/* ipv6 prefix */
		/* Since bind-ipv6-prefix has variable length, we need to get the correct buffer size. */
		if(tempInfo.s46_v4v6bind.prefix6_len%8)
			addr6_buf_len = tempInfo.s46_v4v6bind.prefix6_len/8 + 1 ;
		else
			addr6_buf_len = tempInfo.s46_v4v6bind.prefix6_len/8;

		pos = j;
		for(j;j<pos+addr6_buf_len;j++){
			/*
			 *  Message in hex would be ignored if the first word is "0", we recover it to the orginal
			 *  two words from "d" to "0d" (for example) if the string length is 1.
			 */
			if(strlen(*(pch+j))==1){
				snprintf(tmp, sizeof(tmp), "%s%s", "0", *(pch+j));
				*(pch+j) = tmp ;
			}
			strcat(prefix6_str,*(pch+j));

			/* Add a ":" to present in ipv6 address once we have captured 2 words. */
			if((j-pos)%2)
				strcat(prefix6_str,":");
		}

		/*
		 *  Be careful that 2001:0db8:100f:ab00:: is different from 2001:0db8:100f:ab::,
		 *  In the case 2001:0db8:100f:ab00::, there are 7 buffers sent from server,
		 *  i.e., "20","01","0d","d8","10","0f","ab".
		 *  If there is no extra process, the ipv6 string would be 2001:0db8:100f:ab::
		 *  Thus, if the numbers of buffer are odd we add "00" with ":" at the end.
		 */
		if(addr6_buf_len%2)
			strcat(prefix6_str, "00:");

		/*
		 *  Continue from above, now the ipv6 string is 2001:0db8:100f:ab00:, just add a
		 *  ":" at the end to present in prefix. Note that if prefix is given in 128 bits,
		 *  we don't need to add it.
		 */
		if(tempInfo.s46_v4v6bind.prefix6_len==128)
			prefix6_str[strlen(prefix6_str)-1] = '\0';
		else
			strcat(prefix6_str, ":");

		inet_pton(PF_INET6, prefix6_str, &(tempInfo.s46_v4v6bind.lw4o6_v6_prefix));
		memcpy(pInfo, &tempInfo, sizeof(S46_DHCP_INFO_T));
	}

	/*
	 *  0                   1                   2                   3
     *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |     OPTION_S46_PORTPARAMS     |         option-length         |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |   offset      |    PSID-len   |              PSID             |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *
     *				    OPTION_S46_PORTPARAMS (93)
     */

	/* option code */
	tempInfo.s46_portparams.option_code = hex_to_int(strcat(*(pch+(j++)), *(pch+(j++))));

	if(tempInfo.s46_portparams.option_code == D6O_S46_PORTPARAMS){
		/* option length */
		tempInfo.s46_portparams.option_len = hex_to_int(strcat(*(pch+(j++)), *(pch+(j++))));
		/* psid offset */
		tempInfo.s46_portparams.psid_offset = hex_to_int(*(pch+(j++)));
		/* psid length */
		tempInfo.s46_portparams.psid_len= hex_to_int(*(pch+(j++)));
		/* psid */
		/*
		 * Since psid is 16 bit in integer based on the PSID-len. First, we need to translate raw
		 * data(hex) to integer. Sencond, translate integer to binary. Finally, based on PSID-len
		 * translate the specific length of the binary to integer. for example: psid-len=6, we get
		 * raw data array "d0" and "0". step1: supplement "0" to "00". step2: strcat "d0" and
		 * "00" to "d000" and tranlate to integer. step3: use the integer to translate to binary
		 * "101000000000000". step4: use 6 bits, i.e., "110100" translate to integer 52.
		 */
		pos = j;

		/* psid is in 16 bits (2 bytes) long so it will be put in 2 arrays */
		for(j;j<pos+2;j++){
			/*
			 *  Message in hex would be ignored if the first word is "0", we recover it to the orginal
			 *  two words from "d" to "0d" (for example) if the string length is 1.
			 */
			if(strlen(*(pch+j))==1){
				snprintf(tmp, sizeof(tmp), "%s%s", "0", *(pch+j));
				*(pch+j) = tmp ;
			}
			strcat(psid_hex_string, *(pch+j));
		}
		psid_decimal = hex_to_int(psid_hex_string);
		/* translate integer to binary */
		for(i=16;i>0;i--)
		{
			psid_binary[i]=psid_decimal%2;
			psid_decimal/=2 ;
		}
		/* translate binary to integer based on the PSID length */
		for(i=1;i<=tempInfo.s46_portparams.psid_len;i++)
		{
			tempInfo.s46_portparams.psid_val += psid_binary[i] * power(2,(tempInfo.s46_portparams.psid_len-i));
		}
	}

	memcpy(pInfo, &tempInfo, sizeof(S46_DHCP_INFO_T));
	return 1;
}

#endif

/*
 * return value: If you want to add return value for parsing parameter you want, please return
 *               value for bitwise. We will use bitwise & to be decision.
 *
 * return 256(LEASE_GET_LW4O6)	  :  find LW4O6 INFO
 * return 128(LEASE_GET_MAPT)	  :  find MAP-T INFO
 * return 64(LEASE_GET_MAPE)	  :  find MAP-E INFO
 * return 32(LEASE_GET_AFTR)      :  find AFTR Domain name.
 * return 16(LEASE_GET_IAPD_WITH_INVALID_LIFETIME)   : If you only need to get IAPD not care if it's valid, use this.
 * return 8(LEASE_GET_IFNAME)    :  find WAN interface name.
 * return 4(LEASE_GET_DNS) :  also can be used to find DNS server info for information-requset (No IA info).
 * return 2(LEASE_GET_IANA):  find IA_NA (TODO).
 * return 1(LEASE_GET_IAPD):  find IA_PD(Prefix Delegation).
 * return 0(LEASE_NONE)    :  find Nothing info we care about.
 *
 */

int getLeasesInfo(const char *fname, DLG_INFO_Tp pInfo)
{
	FILE *fp;
	char temps[0x100];
	char *str, *endStr;
	unsigned int offset, RNTime, RBTime, PLTime, MLTime;
	struct in6_addr addr6;
	int prefixLen;
	int in_lease6 = 0;
	DLG_INFO_T tempInfo;
	int ifIndex;
	MIB_CE_ATM_VC_T vcEntry;
	int ret = LEASE_NONE;
	char *save_ptr = NULL;

	if ((fp = fopen(fname, "r")) == NULL)
	{
		//printf("%s Open file %s fail !\n", __FUNCTION__, fname);
		return 0;
	}

	while (fgets(temps,0x100,fp))
	{
		if (temps[strlen(temps)-1]=='\n')
			temps[strlen(temps)-1] = 0;

		if (str=strstr(temps, "lease6")) {
			// new lease6 declaration, reset tempInfo
			memset(&tempInfo, 0, sizeof(DLG_INFO_T));
			in_lease6 = 1;
			ret = LEASE_NONE; //initial it.
			fgets(temps,0x100,fp);
		}
		else if(str=strstr(temps, "released"))
		{
			memset(&tempInfo, 0, sizeof(DLG_INFO_T));
			in_lease6 = 0;
			ret = LEASE_NONE;
		}

		if (!in_lease6)
			continue;

		if (str = strstr(temps, "interface")) {
			if (str = strchr(str, '"')) {
				if (str = strtok_r(str, "\"", &save_ptr)) {
					strncpy(tempInfo.wanIfname, str, sizeof(tempInfo.wanIfname));
					tempInfo.wanIfname[sizeof(tempInfo.wanIfname) - 1] = '\0';
					ret |= LEASE_GET_IFNAME;
				}
			}
		}

		if (str=strstr(temps, "ia-pd"))
		{
			ret |= LEASE_GET_IAPD;
			fgets(temps,0x100,fp);

			// Get renew
			fgets(temps,0x100,fp);
			if ( str=strstr(temps, "renew") ) {
				offset = strlen("renew")+1;
				if ( endStr=strchr(str+offset, ';')) {
					*endStr=0;
					sscanf(str+offset, "%u", &RNTime);
					tempInfo.RNTime = RNTime;
				}
			}

			// Get rebind
			fgets(temps,0x100,fp);
			if ( str=strstr(temps, "rebind")) {
				offset = strlen("rebind")+1;
				if ( endStr=strchr(str+offset, ';')) {
					*endStr=0;
					sscanf(str+offset, "%u", &RBTime);
					tempInfo.RBTime = RBTime;
				}
			}
			fgets(temps,0x100,fp);

			while(str=strstr(temps, "iaprefix"))
			{
				PLTime=MLTime=prefixLen=0;
				memset((void *)addr6.s6_addr, 0, sizeof(addr6));
				// Get prefix
				if ( str=strstr(temps, "iaprefix")) {
					offset = strlen("iaprefix")+1;
					if ( endStr=strchr(str+offset, ' ')) {
						*endStr=0;

						if(endStr = strchr(str + offset, '/'))
							*endStr = 0;

						// PrefixIP
						inet_pton(PF_INET6, (str+offset), &addr6);

						// Prefix Length
						//sscanf((endStr+1), "%d", &(Info.prefixLen));
						prefixLen = (char)atoi((endStr+1));
					}

				}

				//skip starts
				fgets(temps,0x100,fp);

				// Get preferred-life
				fgets(temps,0x100,fp);
				if ( str=strstr(temps, "preferred-life")) {
					offset = strlen("preferred-life")+1;
					if ( endStr=strchr(str+offset, ';') ) {
						*endStr=0;
						sscanf(str+offset, "%u", &PLTime);
					}
				}

				// Get max-life
				fgets(temps,0x100,fp);
				if( str=strstr(temps, "max-life")) {
					offset = strlen("max-life")+1;
					if ( endStr=strchr(str+offset, ';')) {
						*endStr=0;
						sscanf(str+offset, "%u", &MLTime);
					}
				}

				if (PLTime == 0 || MLTime == 0 || (PLTime > MLTime) || addr6.s6_addr[0] == '\0')
				{
					ret &= (~LEASE_GET_IAPD); //this pd is invalid
					ret |= LEASE_GET_IAPD_WITH_INVALID_LIFETIME; //only want IAPD, don't care if it's valid flag
					memcpy(tempInfo.prefixIP, &addr6, IP6_ADDR_LEN);
					tempInfo.prefixLen = prefixLen;
					tempInfo.PLTime = 0;
					tempInfo.MLTime = 0;
				}
				else if (addr6.s6_addr[0] != '\0') {
					ret |= LEASE_GET_IAPD; //this pd is valid
					ret &= (~LEASE_GET_IAPD_WITH_INVALID_LIFETIME); //valid IAPD can't be LEASE_GET_IAPD_WITH_INVALID_LIFETIME
					ip6toPrefix(&addr6,prefixLen,&tempInfo.prefixIP);
					tempInfo.prefixLen = prefixLen;
					tempInfo.PLTime = PLTime;
					tempInfo.MLTime = MLTime;
				}
				/* jump to next iaprefix or option.
				 * because iaprefix format is
				 *
				 * iaprefix 2001:db8:0:200::/64 {
				 * starts 1558080573;
				 * preferred-life 7200;
				 * max-life 7500;
				 * }
				 */
				fgets(temps,0x100,fp);
				fgets(temps,0x100,fp);
			}

		}
		if (str=strstr(temps, "dhcp6.name-servers")) {
			/* Find DNS info. */
			ret |= LEASE_GET_DNS;
			offset = strlen("dhcp6.name-servers")+1;
			if ( endStr=strchr(str+offset, ';')) {
				*endStr=0;
				//printf("Name server=%s\n", str+offset);
				memcpy(tempInfo.nameServer, (unsigned char *)(str+offset),  256);
			}
		}
		if (str=strstr(temps, "dhcp6.domain-search")) {
			char *line, *domain;
			int i=0;

			/* Find DNS domain search list. */
			offset = strlen("dhcp6.domain-search")+1;
			if ( endStr=strchr(str+offset, ';')) {
				*endStr=0;
				line = str + offset;
				//printf("domain Name Search list  from lease file %s=%s\n", fname ,line);
				while(( domain= strsep(&line, ",\" ")) !=NULL){
	        			if ((strlen(domain) !=0) && (i<MAX_DOMAIN_NUM)){
							snprintf(tempInfo.domainSearch[i], MAX_DOMAIN_LENGTH, "%s", domain);
							i++;
					}
        			}
				tempInfo.dnssl_num = i;
			}
		}
		if (str=strstr(temps, "dhcp6.dslite")){
			/*Find dslite AFTR domain name*/
			ret |= LEASE_GET_AFTR;
			offset = strlen("dhcp6.dslite")+1;

			if ( endStr=strchr(str+offset, ';')) {
				*endStr=0;
				snprintf(tempInfo.dslite_aftr, sizeof(tempInfo.dslite_aftr), "%s", str+offset);
			}
		}

#ifdef CONFIG_USER_MAP_E
			//MAP-E
			if (str=strstr(temps, "dhcp6.MAP-E")){
				ret |= LEASE_GET_MAPE;
				offset = strlen("dhcp6.MAP-E")+1;

				if(endStr=strchr(str+offset,';')){
					*endStr=0;
					snprintf(tempInfo.s46_string, sizeof(tempInfo.s46_string), "%s", str+offset);
					get_map_info(LEASE_GET_MAPE, tempInfo.s46_string, &tempInfo.map_info);
				}
			}
#endif
#ifdef CONFIG_USER_MAP_T
			//MAP-T
			if (str=strstr(temps, "dhcp6.MAP-T")){
				ret |= LEASE_GET_MAPT;
				offset = strlen("dhcp6.MAP-T")+1;

				if(endStr=strchr(str+offset,';')){
					*endStr=0;
					snprintf(tempInfo.s46_string, sizeof(tempInfo.s46_string), "%s", str+offset);
					get_map_info(LEASE_GET_MAPT, tempInfo.s46_string, &tempInfo.map_info);
				}
			}
#endif
#ifdef CONFIG_USER_LW4O6
			if(str=strstr(temps, "dhcp6.lw4o6")){
				ret |= LEASE_GET_LW4O6;
				offset = strlen("dhcp6.lw4o6")+1;

				if(endStr=strchr(str+offset,';')){
					*endStr=0;
					snprintf(tempInfo.s46_string, sizeof(tempInfo.s46_string), "%s", str+offset);
					get_lw4o6_info(tempInfo.s46_string,&tempInfo.lw4o6_info);
				}
			}
#endif
	}

	fclose(fp);

	if (ret != LEASE_NONE) {
		memcpy(pInfo, &tempInfo, sizeof(DLG_INFO_T));
	}

	return ret;
}

int getLANPortStatus(unsigned int id, char *strbuf)
{
	struct net_link_info netlink_info;
	int link_status=0;

#ifdef CONFIG_RTL8686_SWITCH
	if(rtk_port_get_lan_phyStatus(id, &link_status)==0){
		if(link_status==1)
			rtk_port_get_lan_link_info(id, &netlink_info);
	}
#else
	switch (id) {
		case 0:
			link_status = get_net_link_status(ALIASNAME_ELAN0);
			if( link_status==1)
				get_net_link_info(ALIASNAME_ELAN0, &netlink_info);
			break;
		case 1:
			link_status = get_net_link_status(ALIASNAME_ELAN1);
			if( link_status==1)
				get_net_link_info(ALIASNAME_ELAN1, &netlink_info);
			break;
		case 2:
			link_status = get_net_link_status(ALIASNAME_ELAN2);
			if( link_status==1)
				get_net_link_info(ALIASNAME_ELAN2, &netlink_info);
			break;
		case 3:
			link_status = get_net_link_status(ALIASNAME_ELAN3);
			if( link_status==1)
				get_net_link_info(ALIASNAME_ELAN3, &netlink_info);
			break;
	}
#endif

	switch(link_status)
	{
		case -1:
			strcpy(strbuf, "error");
			goto setErr_lanport;
			//break;
		case 0:
			strcpy(strbuf, "not-connected");
			//break;
			goto setErr_lanport;
		case 1:
			strcpy(strbuf, "up");
			break;
		default:
			goto setErr_lanport;
			//break;
	}
	sprintf(strbuf + strlen(strbuf), " %dM", netlink_info.speed);

	if(netlink_info.duplex == 0)
		sprintf(strbuf + strlen(strbuf), " %s", "Half");
	else if(netlink_info.duplex == 1)
		sprintf(strbuf + strlen(strbuf), " %s", "Full");

setErr_lanport:
	return 0;
}

int getLANPortStatus4Syslog(unsigned int id, char *strbuf)
{
	struct net_link_info netlink_info;
	int link_status=0;

#ifdef CONFIG_RTL8686_SWITCH
	if(rtk_port_get_lan_phyStatus(id, &link_status)==0){
		if(link_status==1)
			rtk_port_get_lan_link_info(id, &netlink_info);
	}
#else
	switch (id) {
		case 0:
			link_status = get_net_link_status(ALIASNAME_ELAN0);
			if( link_status==1)
				get_net_link_info(ALIASNAME_ELAN0, &netlink_info);
			break;
		case 1:
			link_status = get_net_link_status(ALIASNAME_ELAN1);
			if( link_status==1)
				get_net_link_info(ALIASNAME_ELAN1, &netlink_info);
			break;
		case 2:
			link_status = get_net_link_status(ALIASNAME_ELAN2);
			if( link_status==1)
				get_net_link_info(ALIASNAME_ELAN2, &netlink_info);
			break;
		case 3:
			link_status = get_net_link_status(ALIASNAME_ELAN3);
			if( link_status==1)
				get_net_link_info(ALIASNAME_ELAN3, &netlink_info);
			break;
	}
#endif

	switch(link_status)
	{
		case -1:
		case 0:
			goto setErr_lanport;
		case 1:
			break;
		default:
			goto setErr_lanport;
			//break;
	}
	sprintf(strbuf + strlen(strbuf), " %dM", netlink_info.speed);

	if(netlink_info.duplex == 0)
		sprintf(strbuf + strlen(strbuf), " %s", "Half");
	else if(netlink_info.duplex == 1)
		sprintf(strbuf + strlen(strbuf), " %s", "Full");

setErr_lanport:
	return 0;
}

int getLANPortConfig(unsigned int id, char *strbuf)
{
	unsigned char lanport_cap_tbl[SW_LAN_PORT_NUM];
	struct net_link_info netlink_info;
	int link_status=0;
#if defined(CONFIG_GPON_FEATURE)|| defined(CONFIG_EPON_FEATURE)
	int phyPortId;
	unsigned int chipId, rev, subtype;
#endif

	if(id >= SW_LAN_PORT_NUM)
		goto setErr_lanport;

	mib_get_s(MIB_LAN_PORT_CAPABILITY, (void *)lanport_cap_tbl, sizeof(lanport_cap_tbl));
#if defined(CONFIG_GPON_FEATURE)|| defined(CONFIG_EPON_FEATURE)
	rtk_switch_version_get(&chipId, &rev, &subtype);
	phyPortId = rtk_port_get_lan_phyID(id);

	/*9607c has 4 GE port
	 *9603c has 1 GE port (physical port 4) & 3 FE port*/
	if ((chipId == RTL9607C_CHIP_ID) &&
	  ((subtype == RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA5) ||
	  (subtype == RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA6) ||
	  (subtype == RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA7) ||
	  ((subtype == RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA4) && (phyPortId == 4)) ||
	  ((subtype == RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA5) && (phyPortId == 4)) ||
	  ((subtype == RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA6) && (phyPortId == 4))))
	{
		sprintf(strbuf, "<select size=1 name=\"port_status_config%d\">"
		"<option value=%d %s >Auto Negociation</option>"
		"<option value=%d %s >10Mbps Half Duplex</option>"
		"<option value=%d %s >10Mbps Full Duplex</option>"
		"<option value=%d %s >100Mbps Half Duplex</option>"
		"<option value=%d %s >100Mbps Full Duplex</option>"
		"<option value=%d %s >1000Mbps Full Duplex</option>"
		"</select>"
		, id, LINK_AUTO, LINK_AUTO==lanport_cap_tbl[id]?"selected":"",
		LINK_10HALF, LINK_10HALF==lanport_cap_tbl[id]?"selected":"",
		LINK_10FULL, LINK_10FULL==lanport_cap_tbl[id]?"selected":"",
		LINK_100HALF, LINK_100HALF==lanport_cap_tbl[id]?"selected":"",
		LINK_100FULL, LINK_100FULL==lanport_cap_tbl[id]?"selected":"",
		LINK_1000FULL, LINK_1000FULL==lanport_cap_tbl[id]?"selected":"");
	}
	else
	{
#endif
		sprintf(strbuf, "<select size=1 name=\"port_status_config%d\">"
		"<option value=%d %s >Auto Negociation</option>"
		"<option value=%d %s >10Mbps Half Duplex</option>"
		"<option value=%d %s >10Mbps Full Duplex</option>"
		"<option value=%d %s >100Mbps Half Duplex</option>"
		"<option value=%d %s >100Mbps Full Duplex</option>"
		"<option value=%d %s >1000Mbps Full Duplex</option>"
		"</select>"
		, id, LINK_AUTO, LINK_AUTO==lanport_cap_tbl[id]?"selected":"",
		LINK_10HALF, LINK_10HALF==lanport_cap_tbl[id]?"selected":"",
		LINK_10FULL, LINK_10FULL==lanport_cap_tbl[id]?"selected":"",
		LINK_100HALF, LINK_100HALF==lanport_cap_tbl[id]?"selected":"",
		LINK_100FULL, LINK_100FULL==lanport_cap_tbl[id]?"selected":"",
		LINK_1000FULL, LINK_1000FULL==lanport_cap_tbl[id]?"selected":"");
#if defined(CONFIG_GPON_FEATURE)|| defined(CONFIG_EPON_FEATURE)
	}
#endif

setErr_lanport:
	return 0;
}

struct arg{
	unsigned char cmd;
	unsigned int cmd2;
	unsigned int cmd3;
	unsigned int cmd4;
}pass_arg;

int applyLanPortSetting_each(int pid, unsigned char lanport_cap)
{
	int i, fd=-1;
	struct ifreq ifr;
	struct ethtool_cmd ecmd;

	if (pid<0 || pid >= SW_LAN_PORT_NUM)
		return -1;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd< 0){
		printf("Error!Socket create fail in ethctl.\n");
		return -1;
	}

	memset(&ecmd, 0, sizeof(ecmd));
	ifr.ifr_data = &ecmd;
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "eth0.%d", pid+2);
	//ecmd.autoneg = AUTONEG_DISABLE;
	ecmd.autoneg = AUTONEG_ENABLE;
	switch(lanport_cap) {
		case LINK_10HALF:
			ecmd.speed = SPEED_10;
			ecmd.duplex = DUPLEX_HALF;
			break;
		case LINK_10FULL:
			ecmd.speed = SPEED_10;
			ecmd.duplex = DUPLEX_FULL;
			break;
		case LINK_100HALF:
			ecmd.speed = SPEED_100;
			ecmd.duplex = DUPLEX_HALF;
			break;
		case LINK_100FULL:
			ecmd.speed = SPEED_100;
			ecmd.duplex = DUPLEX_FULL;
			break;
		case LINK_1000FULL:
			ecmd.speed = SPEED_1000;
			ecmd.duplex = DUPLEX_FULL;
			break;
		default:
			ecmd.autoneg = AUTONEG_ENABLE;
			ecmd.speed = SPEED_1000;
			ecmd.duplex = DUPLEX_FULL;
			break;
	}

	//printf("[%s %d]dev=%s, autoneg=%d, speed=%d, duplex=%d\n", __func__, __LINE__, ifr.ifr_name, ecmd.autoneg, ecmd.speed, ecmd.duplex);
	ecmd.cmd = ETHTOOL_SSET;
	if (ioctl(fd, SIOCETHTOOL, &ifr) < 0) {
		printf("Error ioctl in SIOCETHTEST errno=%d(%s)\n", errno, strerror(errno) );
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

int applyLanPortSetting(void)
{
	int i;
	unsigned char lanport_cap_tbl[SW_LAN_PORT_NUM];

	mib_get_s(MIB_LAN_PORT_CAPABILITY, (void *)lanport_cap_tbl, sizeof(lanport_cap_tbl));
	for(i=0; i<SW_LAN_PORT_NUM; i++ )
	{
		applyLanPortSetting_each(i, lanport_cap_tbl[i]);
	}

	return 0;
}

int get_cu_software_version(char *strbuf)
{
	FILE *fp;
	unsigned char buffer[128] = {0}, tmpBuf[64] = {0}, *pStr = NULL;

	if(strbuf == NULL)
		return -1;

	mib_get_s(MIB_PROVINCE_SW_VERSION, (void *)buffer, sizeof(buffer));
	if(!strcmp(buffer,"")){
		/* province version isn't used */
		fp = fopen("/etc/version", "r");
		if (fp!=NULL) {
			fgets(tmpBuf, sizeof(tmpBuf), fp);  //main version
			fclose(fp);
			pStr = strstr(tmpBuf, " --");
			*pStr=0;
		};
		sprintf(strbuf, "%s", tmpBuf);
	}
	else/* use province version */
		sprintf(strbuf, "%s", buffer);

	return 0;
}
#define MAX_LINE_LENGTH 256
int isDadState(char *str_ip6, char *ifname)
{
	FILE*	fp = popen("/usr/bin/ip -6 address", "r");
	char	line[MAX_LINE_LENGTH];
	char	inet[MAX_LINE_LENGTH];
	char	address[MAX_LINE_LENGTH];
	int		dadFailed = 0;
	char	*token;

	if (fp == NULL)
		return 1;

	while (fgets(line, MAX_LINE_LENGTH, fp) != NULL)
	{
		if (strstr(line, ifname) != NULL)
		{
			while (fgets(line, MAX_LINE_LENGTH, fp) != NULL)
			{
				if (strstr(line, "inet6") != NULL)
				{
					sscanf(line, "%s %s", inet, address);
					token = strtok(address, "/");

					if(!token && !strcmp(token, str_ip6))
					{
						if (strstr(line, "dadfailed") != NULL)
						{
							pclose(fp);
							return 1;
						}
					}
					else if(!strcmp(token, str_ip6))
					{
						if (strstr(line, "dadfailed") != NULL)
						{
							pclose(fp);
							return 1;
						}
					}
				}
			}
		}
	}

	pclose(fp);
	return 0;
}
int getMIB2Str_s(unsigned int id, char strbuf[], int strbuf_len)
{
	unsigned char buffer[64];

	//if (!strbuf)
	//	return -1;

	switch (id) {
		// INET address
		case MIB_ADSL_LAN_IP:
		case MIB_ADSL_LAN_SUBNET:
		case MIB_ADSL_LAN_IP2:
		case MIB_ADSL_LAN_SUBNET2:
		case MIB_ADSL_WAN_DNS1:
		case MIB_ADSL_WAN_DNS2:
		case MIB_ADSL_WAN_DNS3:
#ifdef CONFIG_USER_DHCP_SERVER
		case MIB_ADSL_WAN_DHCPS:
#endif
#ifdef DMZ
		case MIB_DMZ_IP:
#endif
		case MIB_DHCP_POOL_START:
		case MIB_DHCP_POOL_END:
		case MIB_DHCPS_DNS1:
		case MIB_DHCPS_DNS2:
		case MIB_DHCPS_DNS3:
		case MIB_DHCP_SUBNET_MASK:
#ifdef CONFIG_USER_DNS_RELAY_PROXY
		case MIB_ADSL_LANV4_DNS1:
		case MIB_ADSL_LANV4_DNS2:
#endif

#ifdef AUTO_PROVISIONING
		case MIB_HTTP_SERVER_IP:
#endif
		case MIB_ADSL_LAN_DHCP_GATEWAY:
#if 1
#if defined(_PRMT_X_CT_COM_DHCP_)||defined(IP_BASED_CLIENT_TYPE)
		case CWMP_CT_STB_MINADDR:
		case CWMP_CT_STB_MAXADDR:
		case CWMP_CT_PHN_MINADDR:
		case CWMP_CT_PHN_MAXADDR:
		case CWMP_CT_CMR_MINADDR:
		case CWMP_CT_CMR_MAXADDR:
		case CWMP_CT_PC_MINADDR:
		case CWMP_CT_PC_MAXADDR:
		case CWMP_CT_HGW_MINADDR:
		case CWMP_CT_HGW_MAXADDR:
#endif //_PRMT_X_CT_COM_DHCP_
#endif
		//ql 20090122 add
#ifdef IMAGENIO_IPTV_SUPPORT
		case MIB_IMAGENIO_DNS1:
		case MIB_IMAGENIO_DNS2:
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
		case MIB_OPCH_ADDRESS:
#endif
/*ping_zhang:20090930 END*/
#endif

#ifdef ADDRESS_MAPPING
#ifndef MULTI_ADDRESS_MAPPING
		case MIB_LOCAL_START_IP:
		case MIB_LOCAL_END_IP:
		case MIB_GLOBAL_START_IP:
		case MIB_GLOBAL_END_IP:
#endif //end of !MULTI_ADDRESS_MAPPING
#endif
#ifdef DEFAULT_GATEWAY_V2
		case MIB_ADSL_WAN_DGW_IP:
#endif
#ifdef CONFIG_USER_RTK_SYSLOG
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
#ifndef USE_DEONET
		case MIB_SYSLOG_SERVER_IP:
#else
		case MIB_SYSLOG_SERVER_IP1:
		case MIB_SYSLOG_SERVER_IP2:
		case MIB_SYSLOG_SERVER_IP3:
		case MIB_SYSLOG_SERVER_IP4:
		case MIB_SYSLOG_SERVER_IP5:
#endif
#endif
#ifdef SEND_LOG
		case MIB_LOG_SERVER_IP:
#endif
#endif
#ifdef _PRMT_TR143_
		case TR143_UDPECHO_SRCIP:
#endif //_PRMT_TR143_
			if(!mib_get_s( id, (void *)buffer, sizeof(buffer)))
				return -1;
			// Mason Yu
			if ( ((struct in_addr *)buffer)->s_addr == INADDR_NONE ) {
				sprintf(strbuf, "%s", "");
			} else {
				sprintf(strbuf, "%s", inet_ntoa(*((struct in_addr *)buffer)));
			}
			break;
#ifdef CONFIG_IPV6
#ifdef DHCPV6_ISC_DHCP_4XX
		// INET6 address
		case MIB_DHCPV6S_RANGE_START:
		case MIB_DHCPV6S_RANGE_END:
			if(!mib_get_s( id, (void *)buffer, sizeof(buffer)))
				return -1;
			inet_ntop(PF_INET6, buffer, strbuf, strbuf_len);
			break;
		case MIB_DHCPV6S_MIN_ADDRESS:
		case MIB_DHCPV6S_MAX_ADDRESS:
		case MIB_IPV6_LAN_PREFIX:
			if(!mib_get_s( id, (void *)strbuf, strbuf_len))
				return -1;
			break;
#endif
		case MIB_ADSL_WAN_DNSV61:
		case MIB_ADSL_WAN_DNSV62:
		case MIB_ADSL_WAN_DNSV63:
			if(!mib_get_s( id, (void *)buffer, sizeof(buffer)))
				return -1;
			inet_ntop(PF_INET6, buffer, strbuf, strbuf_len);
			break;
		case MIB_IPV6_LAN_LLA_IP_ADDR:
			if(!mib_get_s( id, (void *)strbuf, strbuf_len))
				return -1;
			break;
#endif
		// Ethernet address
		case MIB_ELAN_MAC_ADDR:
		case MIB_WLAN_MAC_ADDR:
			if(!mib_get_s( id,  (void *)buffer, sizeof(buffer)))
				return -1;
			sprintf(strbuf, "%02x%02x%02x%02x%02x%02x", buffer[0], buffer[1],
				buffer[2], buffer[3], buffer[4], buffer[5]);
			break;
		// Char
#ifdef CONFIG_GPON_FEATURE
		case MIB_OMCC_VER:
		case MIB_OMCI_TM_OPT:
#endif
		case MIB_ADSL_LAN_CLIENT_START:
		case MIB_ADSL_LAN_CLIENT_END:
#ifdef WLAN_SUPPORT
		case MIB_WLAN_CHAN_NUM:
		case MIB_WLAN_CHANNEL_WIDTH:
		case MIB_WLAN_NETWORK_TYPE:
		case MIB_WIFI_TEST:

#ifdef WLAN_UNIVERSAL_REPEATER
		case MIB_REPEATER_ENABLED1:
#endif
#ifdef WLAN_WDS
		case MIB_WLAN_WDS_ENABLED:
#endif

#if defined (CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS) // WPS
		case MIB_WSC_VERSION:
#endif
		case MIB_WLAN_DTIM_PERIOD:
#ifdef RTK_MULTI_AP
		case MIB_MAP_CONTROLLER:
#ifdef EASY_MESH_BACKHAUL
		case MIB_WIFI_MAP_BACKHAUL:
#endif
#ifdef RTK_MULTI_AP_R2
		case MIB_MAP_ENABLE_VLAN:
		case MIB_MAP_PRIMARY_VID:
		case MIB_MAP_DEFAULT_SECONDARY_VID:
#endif
#endif
#endif
#ifdef CONFIG_USER_RTK_SYSLOG
		case MIB_SYSLOG_LOG_LEVEL:
#ifndef USE_DEONET
		case MIB_SYSLOG_DISPLAY_LEVEL:
#else
		case MIB_SYSLOG_REMOTE_LEVEL:
#endif
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
		case MIB_SYSLOG_MODE:
#endif
#endif
		case MIB_V6_IPV6_ENABLE:
		case MIB_LDAP_SERVER_ENABLE:
#if 0
#if defined(_PRMT_X_CT_COM_DHCP_)||defined(IP_BASED_CLIENT_TYPE)
		case CWMP_CT_STB_MINADDR:
		case CWMP_CT_STB_MAXADDR:
		case CWMP_CT_PHN_MINADDR:
		case CWMP_CT_PHN_MAXADDR:
		case CWMP_CT_CMR_MINADDR:
		case CWMP_CT_CMR_MAXADDR:
		case CWMP_CT_PC_MINADDR:
		case CWMP_CT_PC_MAXADDR:
		case CWMP_CT_HGW_MINADDR:
		case CWMP_CT_HGW_MAXADDR:
#endif //_PRMT_X_CT_COM_DHCP_
#endif
#ifdef CONFIG_USER_DNS_RELAY_PROXY
		case MIB_LAN_DNSV4_MODE:
#endif

#ifdef CONFIG_IPV6
#ifdef DHCPV6_ISC_DHCP_4XX
		case MIB_DHCPV6S_TYPE:
		case MIB_DHCPV6S_PREFIX_LENGTH:
		case MIB_DHCPV6S_DNS_ASSIGN_MODE:
		case MIB_DHCPV6S_POOL_ADDR_FORMAT:
		case MIB_LAN_DNSV6_MODE:
#endif
#endif
#ifdef TIME_ZONE
		case MIB_NTP_TIMEZONE_DB_INDEX:
#endif
#ifdef CONFIG_IPV6
		case MIB_ADSL_WAN_DNSV6_MODE:
		case MIB_PREFIXINFO_PREFIX_MODE:
		case MIB_V6_RADVD_ENABLE:
		case MIB_V6_MANAGEDFLAG:
		case MIB_V6_MFLAGINTERLEAVE:
		case MIB_V6_OTHERCONFIGFLAG:
		case MIB_V6_RADVD_PREFIX_MODE:
#endif
#ifdef CONFIG_E8B
		case CWMP_CONFIGURABLE:
#endif
			if(!mib_get_s(id,  (void *)buffer, sizeof(buffer)))
				return -1;
	   		sprintf(strbuf, "%u", *(unsigned char *)buffer);
	   		break;
	   	// Short
		//case MIB_DHCP_PORT_FILTER:
	   	case MIB_BRCTL_AGEINGTIME:
#ifdef WLAN_SUPPORT
	   	case MIB_WLAN_FRAG_THRESHOLD:
	   	case MIB_WLAN_RTS_THRESHOLD:
	   	case MIB_WLAN_BEACON_INTERVAL:
#endif
#ifdef CONFIG_USER_RTK_SYSLOG
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
#ifndef USE_DEONET
		case MIB_SYSLOG_SERVER_PORT:
#else
		case MIB_SYSLOG_SERVER_PORT1:
		case MIB_SYSLOG_SERVER_PORT2:
		case MIB_SYSLOG_SERVER_PORT3:
		case MIB_SYSLOG_SERVER_PORT4:
		case MIB_SYSLOG_SERVER_PORT5:
		case MIB_SYSLOG_SERVER_URL_PORT:
#endif
#endif
#endif
		case MIB_HOLE_PUNCHING_PORT:
		case MIB_HOLE_PUNCHING_PORT_ENC:
		case MIB_HOLE_PUNCHING_INTERVAL:
		//ql 20090122 add
#ifdef IMAGENIO_IPTV_SUPPORT
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
		case MIB_OPCH_PORT:
#endif
/*ping_zhang:20090930 END*/
#endif
			if(!mib_get_s( id,  (void *)buffer, sizeof(buffer)))
				return -1;
			sprintf(strbuf, "%u", *(unsigned short *)buffer);
			break;
	   	// Interger
		case MIB_ADSL_LAN_DHCP_LEASE:
		case MIB_SYSLOG_LINE:
		case MIB_WAN_NAT_MODE:

			// Mason Yu
			if(!mib_get_s( id,  (void *)buffer, sizeof(buffer)))
				return -1;
			// if MIB_ADSL_LAN_DHCP_LEASE=0xffffffff, it indicate an infinate lease
			if ( *(unsigned long *)buffer == 0xffffffff )
				sprintf(strbuf, "-1");
			else
				sprintf(strbuf, "%u", *(unsigned int *)buffer);
			break;
		// Interger
		case MIB_DHCP_PORT_FILTER:
#ifdef WEB_REDIRECT_BY_MAC
		case MIB_WEB_REDIR_BY_MAC_INTERVAL:
#endif
		case MIB_IGMP_PROXY_ITF:
#if defined(CONFIG_USER_UPNPD)||defined(CONFIG_USER_MINIUPNPD)
		case MIB_UPNP_EXT_ITF:
#endif
#ifdef TIME_ZONE
		case MIB_NTP_EXT_ITF:
#endif
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_MLDPROXY
		case MIB_MLD_PROXY_EXT_ITF:
#endif
#ifdef DHCPV6_ISC_DHCP_4XX
		case MIB_DHCPV6R_UPPER_IFINDEX:
#endif
#endif

#ifdef IP_PASSTHROUGH
		case MIB_IPPT_ITF:
		case MIB_IPPT_LEASE:
#endif
#ifdef DEFAULT_GATEWAY_V2
		case MIB_ADSL_WAN_DGW_ITF:
#endif
#ifndef USE_DEONET
		case MIB_MAXLOGLEN:
#endif
#ifdef _CWMP_MIB_
		case CWMP_INFORM_INTERVAL:
#endif
#ifdef DOS_SUPPORT
		case MIB_DOS_SYSSYN_FLOOD:
		case MIB_DOS_SYSFIN_FLOOD:
		case MIB_DOS_SYSUDP_FLOOD:
		case MIB_DOS_SYSICMP_FLOOD:
		case MIB_DOS_PIPSYN_FLOOD:
		case MIB_DOS_PIPFIN_FLOOD:
		case MIB_DOS_PIPUDP_FLOOD:
		case MIB_DOS_PIPICMP_FLOOD:
		case MIB_DOS_BLOCK_TIME:
		case MIB_DOS_SNMP_FLOOD:
#endif
#ifdef TCP_UDP_CONN_LIMIT
		case MIB_CONNLIMIT_TCP:
		case MIB_CONNLIMIT_UDP:
#endif
#ifdef _CWMP_MIB_
		case CWMP_CONREQ_PORT:
		case CWMP_WAN_INTERFACE:
#endif
#ifdef CONFIG_IPV6
#ifdef DHCPV6_ISC_DHCP_4XX
		case MIB_DHCPV6S_DEFAULT_LEASE:
		case MIB_DHCPV6S_PREFERRED_LIFETIME:
		case MIB_DHCPV6S_RENEW_TIME:
		case MIB_DHCPV6S_REBIND_TIME:
#endif
#endif
#ifdef _TR111_STUN_
		case TR111_STUNSERVERPORT:
#endif
		case MIB_WAN_MODE:
		case MIB_DMZ_WAN:
#ifdef _PRMT_X_CT_COM_USERINFO_
		case CWMP_USERINFO_STATUS:
		case CWMP_USERINFO_RESULT:
#endif
#ifdef CONFIG_USER_DNS_RELAY_PROXY
		case MIB_DNSINFOV4_WANCONN:
#endif

#ifdef CONFIG_IPV6
		case MIB_PD_WAN_DELEGATED_MODE:
		case MIB_PREFIXINFO_DELEGATED_WANCONN:
		case MIB_DNSV6INFO_WANCONN_MODE:
		case MIB_DNSINFO_WANCONN:
		case MIB_V6_RADVD_RDNSSINFO_WANCONN_MODE:
		case MIB_V6_RADVD_DNSINFO_WANCONN:

#endif
#ifdef DDNS_SYNC_INTERVAL
		case MIB_DDNS_SYNC_INTERVAL:
#endif
		case MIB_TOTAL_BANDWIDTH:
		case MIB_MULTI_TOTAL_BANDWIDTH:

			if(!mib_get_s( id,  (void *)buffer, sizeof(buffer)))
				return -1;
			sprintf(strbuf, "%u", *(unsigned int *)buffer);
			break;
		// String
#ifdef CONFIG_GPON_FEATURE
		case MIB_OMCI_SW_VER1:
		case MIB_OMCI_SW_VER2:
		case MIB_PON_VENDOR_ID:
#endif
		case MIB_ADSL_LAN_DHCP_DOMAIN:
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
		case MIB_SNMP_SYS_DESCR:
		case MIB_SNMP_SYS_CONTACT:
		case MIB_SNMP_SYS_LOCATION:
		case MIB_SNMP_SYS_OID:
		case MIB_SNMP_COMM_RO:
		case MIB_SNMP_COMM_RW:
		case MIB_SNMP_COMM_RO_ENC:
		case MIB_SNMP_COMM_RW_ENC:
#endif
		case MIB_SNMP_SYS_NAME:
#ifdef WLAN_SUPPORT
#ifdef WLAN_UNIVERSAL_REPEATER
		case MIB_REPEATER_SSID1:
#endif
#ifdef WLAN_WDS
		case MIB_WLAN_WDS_PSK:
#endif
#ifdef RTK_MULTI_AP
		case MIB_MAP_DEVICE_NAME:
#endif
#endif
#ifdef AUTO_PROVISIONING
		case MIB_CONFIG_VERSION:
#endif
#ifdef TIME_ZONE
		case MIB_NTP_ENABLED:
		case MIB_NTP_SERVER_ID:
/*ping_zhang:20081217 START:patch from telefonica branch to support WT-107*/
		case MIB_NTP_SERVER_HOST1:
		case MIB_NTP_SERVER_HOST2:
		case MIB_NTP_SERVER_HOST3:
		case MIB_NTP_SERVER_HOST4:
		case MIB_NTP_SERVER_HOST5:
#endif
/*ping_zhang:20081217*/
#ifdef _CWMP_MIB_
		case CWMP_PROVISIONINGCODE:
		case CWMP_ACS_URL:
		case CWMP_ACS_USERNAME:
		case CWMP_ACS_PASSWORD:
		case CWMP_CONREQ_USERNAME:
		case CWMP_CONREQ_PASSWORD:
		case CWMP_CONREQ_PATH:
		case CWMP_LAN_CONFIGPASSWD:
		case CWMP_DL_COMMANDKEY:
		case CWMP_RB_COMMANDKEY:
		case CWMP_CERT_PASSWORD:
#ifdef _PRMT_USERINTERFACE_
		case UIF_AUTOUPDATESERVER:
		case UIF_USERUPDATESERVER:
#endif
		case CWMP_SI_COMMANDKEY:
		case CWMP_ACS_KICKURL:
		case CWMP_ACS_DOWNLOADURL:
#ifdef _PRMT_X_CT_COM_PORTALMNT_
		case CWMP_CT_PM_URL4PC:
		case CWMP_CT_PM_URL4STB:
		case CWMP_CT_PM_URL4MOBILE:
#endif

#endif
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS) //WPS
		case MIB_WSC_PIN:
		case MIB_WSC_SSID:
#endif
#ifdef CONFIG_USER_RTK_SYSLOG
#ifdef SEND_LOG
		case MIB_LOG_SERVER_NAME:
#endif
#endif
#if defined(CONFIG_IPV6) && defined(CONFIG_USER_RADVD)
		case MIB_V6_MAXRTRADVINTERVAL:
		case MIB_V6_MINRTRADVINTERVAL:
		case MIB_V6_ADVCURHOPLIMIT:
		case MIB_V6_ADVDEFAULTLIFETIME:
		case MIB_V6_ADVREACHABLETIME:
		case MIB_V6_ADVRETRANSTIMER:
		case MIB_V6_ADVLINKMTU:
		case MIB_V6_PREFIX_IP:
		case MIB_V6_PREFIX_LEN:
		case MIB_V6_VALIDLIFETIME:
		case MIB_V6_PREFERREDLIFETIME:
		case MIB_V6_RDNSS1:
		case MIB_V6_RDNSS2:
		case MIB_V6_ULAPREFIX_ENABLE:
		case	MIB_V6_ULAPREFIX_RANDOM:
		case MIB_V6_ULAPREFIX:
		case MIB_V6_ULAPREFIX_LEN:
		case MIB_V6_ULAPREFIX_VALID_TIME:
		case MIB_V6_ULAPREFIX_PREFER_TIME:
#endif
#ifdef CONFIG_IPV6
#ifdef DHCPV6_ISC_DHCP_4XX
		case MIB_DHCPV6S_CLIENT_DUID:
#endif
#endif
		case MIB_SUSER_NAME:
#ifdef USER_WEB_WIZARD
		case MIB_SUSER_PASSWORD:
#endif
		case MIB_USER_NAME:
#ifdef CONFIG_USER_SAMBA
#ifdef CONFIG_USER_NMBD
		case MIB_SAMBA_NETBIOS_NAME:
#endif
		case MIB_SAMBA_SERVER_STRING:
#endif
#ifdef _TR111_STUN_
		case TR111_STUNSERVERADDR:
		case TR111_STUNUSERNAME:
		case TR111_STUNPASSWORD:
#endif
			if(!mib_get_s( id,  (void *)strbuf, strbuf_len)){
				return -1;
			}
			break;

#ifdef CONFIG_RTL_WAPI_SUPPORT
		case MIB_WLAN_WAPI_MCAST_REKEYTYPE:
		case MIB_WLAN_WAPI_UCAST_REKETTYPE:
			if(!mib_get_s( id,  (void *)buffer, sizeof(buffer))){
				return -1;
			}
			sprintf(strbuf, "%d", buffer[0]);
			break;
		case MIB_WLAN_WAPI_MCAST_TIME:
		case MIB_WLAN_WAPI_MCAST_PACKETS:
		case MIB_WLAN_WAPI_UCAST_TIME:
		case MIB_WLAN_WAPI_UCAST_PACKETS:
			if(!mib_get_s( id,  (void *)buffer, sizeof(buffer))){
				return -1;
			}
			sprintf(strbuf, "%u", *(unsigned int *)buffer);
			break;
#endif
#ifdef CONFIG_USER_Y1731
		case Y1731_MODE:
		case Y1731_MEGLEVEL:
		case Y1731_CCM_INTERVAL:
		case Y1731_ENABLE_CCM:
			if(!mib_get_s(id,  (void *)buffer, sizeof(buffer))){
				return -1;
			}
			sprintf(strbuf, "%u", *(unsigned char *)buffer);
			break;

		case Y1731_MYID:
			if(!mib_get_s(id,  (void *)buffer, sizeof(buffer))){
				return -1;
			}
			sprintf(strbuf, "%u", *(unsigned short *)buffer);
			break;
		case Y1731_MEGID:
		case Y1731_LOGLEVEL:
		case Y1731_WAN_INTERFACE:
			if(!mib_get_s(id,  (void *)strbuf, strbuf_len)){
				return -1;
			}
			break;
#endif
#ifdef CONFIG_USER_8023AH
		case EFM_8023AH_MODE:
			if(!mib_get_s(id,  (void *)buffer, sizeof(buffer))){
				return -1;
			}
			sprintf(strbuf, "%u", *(unsigned char *)buffer);
			break;
		case EFM_8023AH_WAN_INTERFACE:
			if(!mib_get_s(id,  (void *)strbuf, strbuf_len)){
				return -1;
			}
			break;
#endif
		// below are version information for tr069 inform packet
		case RTK_DEVID_MANUFACTURER:
		case RTK_DEVID_OUI:
		case RTK_DEVID_PRODUCTCLASS:
		case RTK_DEVINFO_HWVER:
		case RTK_DEVINFO_SWVER:
		case RTK_DEVINFO_SPECVER:
		case MIB_HW_SERIAL_NUMBER:
#if defined(CONFIG_GPON_FEATURE)
		case MIB_GPON_SN:
#endif
#ifdef CONFIG_CT_AWIFI_JITUAN_SMARTWIFI
		case AWIFI_IMAGE_URL:
		case AWIFI_REPORT_URL:
		case MIB_DEVICE_NAME:
		case AWIFI_APPLYID:
		case AWIFI_CITY:
#endif

		case MIB_HW_CWMP_MANUFACTURER:
		case MIB_HW_CWMP_PRODUCTCLASS:
		case MIB_HW_HWVER:
#ifdef CONFIG_SUPPORT_CAPTIVE_PORTAL
		case MIB_CAPTIVEPORTAL_URL:
#endif
#ifdef CONFIG_USER_CTMANAGEDEAMON
		case MIB_BUCPE_MANAGEMENT_PLATFORM:
		case MIB_BUCPE_BACKUP_MANAGEMENT_PLATFORM:
		case MIB_BUCPE_TRACE_URL:
#endif
#ifdef USE_DEONET
		case MIB_SYSLOG_SERVER_URL:

		case MIB_ENABLE_AUTOREBOOT:
		case MIB_AUTOREBOOT_TARGET_TIME:
		case MIB_AUTOREBOOT_LDAP_CONFIG_SERVER:
		case MIB_AUTOREBOOT_LDAP_FILE_VERSION_SERVER:
		case MIB_AUTOREBOOT_ON_IDEL_SERVER:
		case MIB_AUTOREBOOT_UPTIME_SERVER:
		case MIB_AUTOREBOOT_WAN_PORT_IDLE_SERVER:
		case MIB_AUTOREBOOT_HR_STARTHOUR_SERVER:
		case MIB_AUTOREBOOT_HR_STARTMIN_SERVER:
		case MIB_AUTOREBOOT_HR_ENDHOUR_SERVER:
		case MIB_AUTOREBOOT_HR_ENDMIN_SERVER:
		case MIB_AUTOREBOOT_DAY_SERVER:
		case MIB_AUTOREBOOT_AVR_DATA_SERVER:
		case MIB_AUTOREBOOT_AVR_DATA_EPG_SERVER:
		case MIB_AUTOREBOOT_ENABLE_STATIC_CONFIG:
		case MIB_AUTOREBOOT_UPTIME:
		case MIB_AUTOREBOOT_WAN_PORT_IDLE:
		case MIB_AUTOREBOOT_HR_STARTHOUR:
		case MIB_AUTOREBOOT_HR_STARTMIN:
		case MIB_AUTOREBOOT_HR_ENDHOUR:
		case MIB_AUTOREBOOT_HR_ENDMIN:
		case MIB_AUTOREBOOT_DAY:
		case MIB_AUTOREBOOT_AVR_DATA:
		case MIB_AUTOREBOOT_AVR_DATA_EPG:
		case MIB_AUTOREBOOT_CRC_SERVER:
		case MIB_AUTOREBOOT_CRC:
		case MIB_AUTOREBOOT_IDLE:
		case MIB_AUTOREBOOT_DAY_0:
		case MIB_AUTOREBOOT_DAY_1:
		case MIB_AUTOREBOOT_DAY_2:
		case MIB_AUTOREBOOT_DAY_3:
		case MIB_AUTOREBOOT_DAY_4:
		case MIB_AUTOREBOOT_DAY_5:
		case MIB_AUTOREBOOT_DAY_6:
		case MIB_AUTOREBOOT_NTP_ENABLE:
		case MIB_AUTOREBOOT_NTP_ISSYNC:
		case MIB_HOLE_PUNCHING_SERVER:
		case MIB_LDAP_SERVER_URL:
		case MIB_LDAP_SERVER_FILE:
		case MIB_LDAP_SERVER_RELATIVEPATH:
		case MIB_NAT_SESSION_THRESHOLD:
		case MIB_SYSMON_CPU_USEAGE_THRESHOLD:
		case MIB_SYSMON_MEM_USEAGE_THRESHOLD:
#endif
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
		case MIB_SNMP_TRAP_IP:
#endif
			if(!mib_get_s( id,  (void *)strbuf, strbuf_len))
			{
				return -1;
			}

			break;

		default:
			return -1;
	}

	return 0;
}

int getSYSInfoTimer(void) {
	struct sysinfo info;
	int systimer=0;
	sysinfo(&info);
	systimer = (int)info.uptime;
	return systimer;
}

int getSYS2Str(SYSID_T id, char *strbuf)
{
	unsigned char buffer[128], vChar;
	struct sysinfo info;
	int updays, uphours, upminutes, len, i;
	time_t tm;
	struct tm tm_time, *ptm_time;
	FILE *fp;
	unsigned char tmpBuf[64], *pStr;
	unsigned short vUShort;
	unsigned int vUInt;
#ifdef CONFIG_00R0
	int swActive=0;
	char str_active[128], header_buf[128];
#endif
#ifdef CONFIG_IPV6
	struct ipv6_ifaddr ip6_addr[6];
#endif
	struct stat f_status;
#ifdef WLAN_SUPPORT
	MIB_CE_MBSSIB_T Entry;
#endif
#if defined(CONFIG_RTL_8676HWNAT) || defined(CONFIG_USER_VLAN_ON_LAN)
	MIB_CE_SW_PORT_T sw_entry;
#endif
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	struct in_addr inAddr;
#endif

	if (!strbuf)
		return -1;

	strbuf[0] = '\0';

	switch (id) {
		case SYS_UPTIME:
			sysinfo(&info);
			updays = (int) info.uptime / (60*60*24);
			if (updays)
				sprintf(strbuf, "%d day%s, ", updays, (updays != 1) ? "s" : "");
			len = strlen(strbuf);
			upminutes = (int) info.uptime / 60;
			uphours = (upminutes / 60) % 24;
			upminutes %= 60;
			if(uphours)
				sprintf(&strbuf[len], "%2d:%02d", uphours, upminutes);
			else
				sprintf(&strbuf[len], "%d min", upminutes);
			break;
		case SYS_DATE:
	 		time(&tm);
			memcpy(&tm_time, localtime(&tm), sizeof(tm_time));
			strftime(strbuf, 200, "%a %b %e %H:%M:%S %Z %Y", &tm_time);
			break;
		case SYS_YEAR:
	 		time(&tm);
			ptm_time = localtime(&tm);
			snprintf(strbuf, 64, "%d", (ptm_time->tm_year+ 1900));
			break;
		case SYS_MONTH:
	 		time(&tm);
			ptm_time = localtime(&tm);
			snprintf(strbuf, 64, "%d", (ptm_time->tm_mon+ 1));
			break;
		case SYS_DAY:
	 		time(&tm);
			ptm_time = localtime(&tm);
			snprintf(strbuf, 64, "%d", (ptm_time->tm_mday));
			break;
		case SYS_HOUR:
	 		time(&tm);
			ptm_time = localtime(&tm);
			snprintf(strbuf, 64, "%d", (ptm_time->tm_hour));
			break;
		case SYS_MINUTE:
	 		time(&tm);
			ptm_time = localtime(&tm);
			snprintf(strbuf, 64, "%d", (ptm_time->tm_min));
			break;
		case SYS_SECOND:
	 		time(&tm);
			ptm_time = localtime(&tm);
			snprintf(strbuf, 64, "%d", (ptm_time->tm_sec));
			break;
#if defined(CONFIG_USER_CMD_SERVER_SIDE) && defined(CONFIG_DEV_xDSL)
		case SYS_FWVERSION_RPHY:
			commserv_getfwVersion(strbuf);
			break;
		case SYS_DSPVERSION_RPHY:
			commserv_getdspVersion(strbuf);
			break;
#endif
		case SYS_FWVERSION:
#ifdef CONFIG_00R0 /* Read the SW FW version from U-BOOT, Iulian Wu*/
			sprintf(header_buf, "%s", BOOTLOADER_SW_ACTIVE);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "sw_active=%d", &swActive);

			sprintf(header_buf, "%s%d", BOOTLOADER_SW_VERSION, swActive);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "%*[^=]=%s", strbuf);
#elif defined(CONFIG_CU)
			get_cu_software_version(strbuf);
#else
#ifdef EMBED
			tmpBuf[0]=0;
			pStr = 0;
			fp = fopen("/etc/version", "r");
			if (fp!=NULL) {
				fgets(tmpBuf, sizeof(tmpBuf), fp);  //main version
				fclose(fp);
				pStr = strstr(tmpBuf, " --");
				*pStr=0;
			};
			sprintf(strbuf, "%s", tmpBuf);
#else
			sprintf(strbuf, "%s.%s", "Realtek", "1");
#endif
#endif
			break;
		case SYS_HWVERSION:
			if ( !mib_get_s( MIB_HW_HWVER, (void *)buffer, sizeof(buffer)) )
				return -1;
			sprintf(strbuf, "%s", buffer);
			break;
#ifdef CONFIG_00R0
		case SYS_BOOTLOADER_FWVERSION:
			sprintf(header_buf, "%s", BOOTLOADER_UBOOT_HEADER);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "%*[^=]=%[^\n]", strbuf);
			break;

		case SYS_BOOTLOADER_CRC:
			sprintf(header_buf, "%s", BOOTLOADER_UBOOT_CRC);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "%*[^=]=%[^\n]", strbuf);
			break;

		case SYS_FWVERSION_SUM_1:  //current FW CRC
			sprintf(header_buf, "%s", BOOTLOADER_SW_ACTIVE);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "sw_active=%d", &swActive);

			sprintf(header_buf, "%s%d", BOOTLOADER_SW_CRC, swActive);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "%*[^=]=%s", strbuf);
			break;

		case SYS_FWVERSION_SUM_2: //backup FW CRC
			sprintf(header_buf, "%s", BOOTLOADER_SW_ACTIVE);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "sw_active=%d", &swActive);

			sprintf(header_buf, "%s%d", BOOTLOADER_SW_CRC, 1-swActive);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "%*[^=]=%s", strbuf);
			break;
#endif
		case SYS_BUILDTIME:
			{
				char day[16];
				char month[16];
				char year[16];

				fp = fopen("/etc/version", "r");
				if (fp!=NULL) {
					fgets(tmpBuf, sizeof(tmpBuf), fp);  //main version
					fclose(fp);

					if(3==sscanf(tmpBuf, "%*s %*s %*s %s %s %*s %*s %s\n",day, month, year))
						sprintf(strbuf, "%s %s %s", day, month, year);
				}
			}
			break;
#ifdef CPU_UTILITY
		case SYS_CPU_UTIL:
			{
				sprintf(strbuf, "%d%%", get_cpu_usage());
			}
			break;
#endif
#ifdef MEM_UTILITY
		case SYS_MEM_UTIL:
			{
				sprintf(strbuf, "%d%%", get_mem_usage());
			}
			break;
#endif
#ifdef CONFIG_USER_DHCPCLIENT_MODE
		case SYS_LAN_IP:
			if (deo_wan_nat_mode_get() == DEO_NAT_MODE) {
				if (!getInAddr(ALIASNAME_BR0, IP_ADDR, (void *)&inAddr))
					return -1;
			} else {
				if (!getInAddr("br0:1", IP_ADDR, (void *)&inAddr))
					return -1;
			}
			sprintf(strbuf, "%s", inet_ntoa(inAddr));
			break;
		case SYS_LAN_MASK:
			if(!getInAddr(ALIASNAME_BR0, SUBNET_MASK, (void *)&inAddr)){
				return -1;
			}
			sprintf(strbuf, "%s", inet_ntoa(inAddr));
			break;
#endif
#ifdef CONFIG_USER_DHCP_SERVER
		case SYS_LAN_DHCP:
			if ( !mib_get_s( MIB_DHCP_MODE, (void *)buffer, sizeof(buffer)) )
				return -1;
			if (DHCPV4_LAN_SERVER == buffer[0])
				strcpy(strbuf, STR_ENABLE);
			else
				strcpy(strbuf, STR_DISABLE);
			break;
#endif
		case SYS_DHCP_LAN_IP:
#if defined(CONFIG_CONFIG_SECONDARY_IP) && !defined(DHCPS_POOL_COMPLETE_IP)
			mib_get_s(MIB_ADSL_LAN_ENABLE_IP2, (void *)&vChar, sizeof(vChar));
			if (vChar)
				mib_get_s(MIB_ADSL_LAN_DHCP_POOLUSE, (void *)&vChar, sizeof(vChar));

			if (vChar == 0) // primary LAN
				getMIB2Str_s(MIB_ADSL_LAN_IP, strbuf, 256);
			else // secondary LAN
				getMIB2Str_s(MIB_ADSL_LAN_IP2, strbuf, 256);
#else
			// primary LAN
			getMIB2Str_s(MIB_ADSL_LAN_IP, strbuf, 256);
#endif
			break;
		case SYS_DHCP_LAN_SUBNET:
#if defined(CONFIG_SECONDARY_IP) && !defined(DHCPS_POOL_COMPLETE_IP)
			mib_get_s(MIB_ADSL_LAN_ENABLE_IP2, (void *)&vChar, sizeof(vChar));
			if (vChar)
				mib_get_s(MIB_ADSL_LAN_DHCP_POOLUSE, (void *)&vChar, sizeof(vChar));

			if (vChar == 0) // primary LAN
				getMIB2Str_s(MIB_ADSL_LAN_SUBNET, strbuf, 256);
			else // secondary LAN
				getMIB2Str_s(MIB_ADSL_LAN_SUBNET2, strbuf, 256);
#else
			// primary LAN
			getMIB2Str_s(MIB_ADSL_LAN_SUBNET, strbuf, 256);
#endif
			break;
			// Kaohj
		case SYS_DHCPS_IPPOOL_PREFIX:
#if defined(CONFIG_SECONDARY_IP) && !defined(DHCPS_POOL_COMPLETE_IP)
			mib_get_s(MIB_ADSL_LAN_ENABLE_IP2, (void *)&vChar, sizeof(vChar));
			if (vChar)
				mib_get_s(MIB_ADSL_LAN_DHCP_POOLUSE, (void *)&vChar, sizeof(vChar));

			if (vChar == 0) // primary LAN
				mib_get_s(MIB_ADSL_LAN_IP, (void *)&tmpBuf[0], sizeof(tmpBuf[0])) ;
			else // secondary LAN
				mib_get_s(MIB_ADSL_LAN_IP2, (void *)&tmpBuf[0], sizeof(tmpBuf[0])) ;
#else
			// primary LAN
			mib_get_s(MIB_ADSL_LAN_IP, (void *)&tmpBuf[0], sizeof(tmpBuf[0])) ;
#endif
			pStr = tmpBuf;
			sprintf(strbuf, "%d.%d.%d.", pStr[0], pStr[1], pStr[2]);
			break;
#ifdef WLAN_SUPPORT
		case SYS_WLAN:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (0 == Entry.wlanDisabled)
				strcpy(strbuf, STR_ENABLE);
			else
				strcpy(strbuf, STR_DISABLE);
			break;
		case SYS_WLAN_SSID:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			strcpy(strbuf, Entry.ssid);
			break;
		case SYS_WLAN_DISABLED:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf,"%u", Entry.wlanDisabled);
			break;
		case SYS_WLAN_HIDDEN_SSID:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf,"%u", Entry.hidessid);
			break;
		case SYS_WLAN_MODE_VAL:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf,"%u", Entry.wlanMode);
			break;
		case SYS_WLAN_BCASTSSID:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (0 == Entry.hidessid)
				strcpy(strbuf, STR_ENABLE);
			else
				strcpy(strbuf, STR_DISABLE);
			break;
		case SYS_WLAN_ENCRYPT_VAL:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf,"%u", Entry.encrypt);
			break;
		case SYS_WLAN_WPA_CIPHER_SUITE:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf,"%u", Entry.unicastCipher);
			break;
		case SYS_WLAN_WPA2_CIPHER_SUITE:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf,"%u", Entry.wpa2UnicastCipher);
			break;
		case SYS_WLAN_WPA_AUTH:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf,"%u", Entry.wpaAuth);
			break;
		case SYS_WLAN_BAND:
#ifdef WLAN_BAND_CONFIG_MBSSID
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			vChar = Entry.wlanBand;
#else
			if( !mib_get_s(MIB_WLAN_BAND, (void *)&vChar, sizeof(vChar)))
				return -1;
#endif
			strcpy(strbuf, wlan_band[(BAND_TYPE_T)vChar]);
			break;
		case SYS_WLAN_AUTH:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if(Entry.authType <= 2)
				strcpy(strbuf, wlan_auth[(AUTH_TYPE_T)Entry.authType]);
			else
				strcpy(strbuf, "None");
			break;
		case SYS_WLAN_PREAMBLE:
			if ( !mib_get_s( MIB_WLAN_PREAMBLE_TYPE, (void *)buffer, sizeof(buffer)) )
				return -1;
			strcpy(strbuf, wlan_preamble[(PREAMBLE_T)buffer[0]]);
			break;
		case SYS_WLAN_ENCRYPT:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			switch(Entry.encrypt) {
				case WIFI_SEC_NONE:
				case WIFI_SEC_WEP:
				case WIFI_SEC_WPA:
					strcpy(strbuf, wlan_encrypt[(WIFI_SECURITY_T)Entry.encrypt]);
					break;
				case WIFI_SEC_WPA2:
					strcpy(strbuf, wlan_encrypt[3]);
					break;
				case WIFI_SEC_WPA2_MIXED:
					strcpy(strbuf, wlan_encrypt[4]);
					break;
				#ifdef CONFIG_RTL_WAPI_SUPPORT
				case WIFI_SEC_WAPI:
					strcpy(strbuf, wlan_encrypt[5]);
					break;
				#endif
				default:
					strcpy(strbuf, wlan_encrypt[0]);
			}
			break;
		// Mason Yu. 201009_new_security
		case SYS_WLAN_WPA_CIPHER:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if ( Entry.unicastCipher == 0 )
				strcpy(strbuf, "");
			else
				strcpy(strbuf, wlan_Cipher[Entry.unicastCipher-1]);
			break;
		case SYS_WLAN_WPA2_CIPHER:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if ( Entry.wpa2UnicastCipher == 0 )
				strcpy(strbuf, "");
			else
				strcpy(strbuf, wlan_Cipher[Entry.wpa2UnicastCipher-1]);
			break;
		case SYS_WLAN_PSKFMT:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			strcpy(strbuf, wlan_pskfmt[Entry.wpaPSKFormat]);
			break;
		case SYS_WLAN_PSKVAL:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			for (len=0; len<strlen(Entry.wpaPSK); len++)
				strbuf[len]='*';
			strbuf[len]='\0';
			break;
#ifdef USER_WEB_WIZARD
		case SYS_WLAN_PSKVAL_WIZARD:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf, "%s", Entry.wpaPSK);
			break;
#endif
 		case SYS_WLAN_WEP_KEYLEN:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			strcpy(strbuf, wlan_wepkeylen[Entry.wep]);
			break;
		case SYS_WLAN_WEP_KEYFMT:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			strcpy(strbuf, wlan_wepkeyfmt[Entry.wepKeyType]);
			break;
		case SYS_WLAN_WPA_MODE:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (Entry.wpaAuth == WPA_AUTH_AUTO)
				strcpy(strbuf, "Enterprise (RADIUS)");
			else if (Entry.wpaAuth == WPA_AUTH_PSK)
				strcpy(strbuf, "Personal (Pre-Shared Key)");
			break;
		case SYS_WLAN_RSPASSWD:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			for (len=0; len<strlen(Entry.rsPassword); len++)
				strbuf[len]='*';
			strbuf[len]='\0';
			break;
		case SYS_WLAN_RS_PORT:
			if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf, "%u", Entry.rsPort);
			break;
		case SYS_WLAN_RS_IP:
			if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if ( ((struct in_addr *)Entry.rsIpAddr)->s_addr == INADDR_NONE ) {
				sprintf(strbuf, "%s", "");
			} else {
				sprintf(strbuf, "%s", inet_ntoa(*((struct in_addr *)Entry.rsIpAddr)));
			}
			break;
		case SYS_WLAN_RS_PASSWORD:
			if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf, "%s", Entry.rsPassword);
			break;
		case SYS_WLAN_ENABLE_1X:
			if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf, "%u", Entry.enable1X);
			break;
		// Added by Jenny
		case SYS_TX_POWER:
			if ( !mib_get_s( MIB_TX_POWER, (void *)&buffer, sizeof(buffer)) )
				return -1;
			if (0 == buffer[0])
				strcpy(strbuf, "100%");
			else if(1 == buffer[0])
				strcpy(strbuf, "70%");
			else if(2 == buffer[0])
				strcpy(strbuf, "50%");
			else if(3 == buffer[0])
				strcpy(strbuf, "35%");
			else if(4 == buffer[0])
				strcpy(strbuf, "15%");
			break;
		case SYS_WLAN_MODE:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			strcpy(strbuf, wlan_mode[(WLAN_MODE_T)Entry.wlanMode]);
			break;
		case SYS_WLAN_TXRATE:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (0 == Entry.rateAdaptiveEnabled){
				for (i=0; i<12; i++)
					if (1<<i == Entry.fixedTxRate)
						strcpy(strbuf, wlan_rate[i]);
			}
			else if (1 == Entry.rateAdaptiveEnabled)
				strcpy(strbuf, STR_AUTO);
			break;
		case SYS_WLAN_BLOCKRELAY:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (0 == Entry.userisolation)
				strcpy(strbuf, STR_DISABLE);
			else
				strcpy(strbuf, STR_ENABLE);
			break;
		case SYS_WLAN_BLOCK_ETH2WIR:
			if ( !mib_get_s( MIB_WLAN_BLOCK_ETH2WIR, (void *)buffer, sizeof(buffer)) )
				return -1;
			if (0 == buffer[0])
				strcpy(strbuf, STR_DISABLE);
			else
				strcpy(strbuf, STR_ENABLE);
			break;
		case SYS_WLAN_AC_ENABLED:
			if ( !mib_get_s( MIB_WLAN_AC_ENABLED, (void *)&buffer, sizeof(buffer)) )
				return -1;
			if (0 == buffer[0])
				strcpy(strbuf, STR_DISABLE);
			else if(1 == buffer[0])
				strcpy(strbuf, "Allow Listed");
			else if(2 == buffer[0])
				strcpy(strbuf, "Deny Listed");
			else
				strcpy(strbuf, STR_ERR);
			break;
		case SYS_WLAN_WDS_ENABLED:
			if ( !mib_get_s( MIB_WLAN_WDS_ENABLED, (void *)buffer, sizeof(buffer)) )
				return -1;
			if (0 == buffer[0])
				strcpy(strbuf, STR_DISABLE);
			else
				strcpy(strbuf, STR_ENABLE);
			break;
#ifdef WLAN_QoS
		case SYS_WLAN_QoS:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (0 == Entry.wmmEnabled)
				strcpy(strbuf, STR_DISABLE);
			else
				strcpy(strbuf, STR_ENABLE);
			break;
#endif
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS) // WPS
		case SYS_WLAN_WPS_ENABLED:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (0 == Entry.wsc_disabled)
				strcpy(strbuf, STR_ENABLE);
			else
				strcpy(strbuf, STR_DISABLE);
			break;
		case SYS_WLAN_WPS_STATUS:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (0 == Entry.wsc_configured)
				strcpy(strbuf, "Unconfigured");
			else
				strcpy(strbuf, "Configured");
			break;
		// for PIN brute force attack
		case SYS_WLAN_WPS_LOCKDOWN:
			if (stat("/tmp/wscd_lock_stat", &f_status) == 0) {
				//printf("[%s %d] %s exist\n",__FUNCTION__,__LINE__,WSCD_LOCK_STAT);
				strbuf[0]='1';
			}else{
				strbuf[0]='0';
			}

			strbuf[1] = '\0';
			break;
		case SYS_WSC_DISABLE:
			if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf, "%u", Entry.wsc_disabled);
			break;
		case SYS_WSC_AUTH:
			if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf, "%u", Entry.wsc_auth);
			break;
		case SYS_WSC_ENC:
			if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf, "%u", Entry.wsc_enc);
			break;
#endif
#endif // #ifdef WLAN_SUPPORT
		case SYS_DNS_MODE:
			if ( !mib_get_s( MIB_ADSL_WAN_DNS_MODE, (void *)buffer, sizeof(buffer)) )
				return -1;
			if (0 == buffer[0])
				strcpy(strbuf, STR_AUTO);
			else
				strcpy(strbuf, STR_MANUAL);
			break;

#ifdef CONFIG_USER_DHCP_SERVER
		case SYS_DHCP_MODE:
			if(!mib_get_s( MIB_DHCP_MODE, (void *)buffer, sizeof(buffer)) )
				return -1;
			strcpy(strbuf, dhcp_mode[(DHCP_TYPE_T)buffer[0]]);
			break;
#endif

		case SYS_IPF_OUT_ACTION:
			if(!mib_get_s(MIB_IPF_OUT_ACTION, (void *)buffer, sizeof(buffer)) ){
				return -1;
			}
			if (0 == buffer[0])
				strcpy(strbuf, "Deny");
			else if(1 == buffer[0])
				strcpy(strbuf, "Allow");
			else
				strcpy(strbuf, "err");
			break;

#ifdef PORT_FORWARD_GENERAL
		case SYS_DEFAULT_PORT_FW_ACTION:
			if(!mib_get_s(MIB_PORT_FW_ENABLE, (void *)buffer, sizeof(buffer)) ){
				return -1;
			}
			if (0 == buffer[0])
				strcpy(strbuf, "Disable");
			else if(1 == buffer[0])
				strcpy(strbuf, "Enable");
			else
				strcpy(strbuf, "err");
			break;
#endif
#if defined(CONFIG_RTL_IGMP_SNOOPING) || defined(CONFIG_BRIDGE_IGMP_SNOOPING)
		// Added by Jenny
		case SYS_IGMP_SNOOPING:
			if(!mib_get_s( MIB_MPMODE, (void *)&vChar, sizeof(vChar))) {
				return -1;
			}
			if (vChar&MP_IGMP_MASK)
				strcpy(strbuf, STR_ENABLE);
			else
				strcpy(strbuf, STR_DISABLE);
			break;
#endif
		case SYS_IP_QOS:
			if(!mib_get_s( MIB_MPMODE, (void *)&vChar, sizeof(vChar))) {
				return -1;
			}
			if (vChar&MP_IPQ_MASK)
				strcpy(strbuf, STR_ENABLE);
			else
				strcpy(strbuf, STR_DISABLE);
			break;


		// Added by Mason Yu
		case SYS_IPF_IN_ACTION:
			if ( !mib_get_s( MIB_IPF_IN_ACTION, (void *)buffer, sizeof(buffer)) )
				return -1;
			if (0 == buffer[0])
				strcpy(strbuf, "Deny");
			else if(1 == buffer[0])
				strcpy(strbuf, "Allow");
			else
				strcpy(strbuf, "err");
			break;

#ifdef CONFIG_SECONDARY_IP
		case SYS_LAN_IP2:
			if (!mib_get_s( MIB_ADSL_LAN_ENABLE_IP2, (void *)&vChar, sizeof(vChar)))
				return -1;
			if (vChar == 0)
				strcpy(strbuf, STR_DISABLE);
			else
				strcpy(strbuf, STR_ENABLE);
			break;

		case SYS_LAN_DHCP_POOLUSE:
			if (!mib_get_s( MIB_ADSL_LAN_DHCP_POOLUSE, (void *)&vChar, sizeof(vChar)))
				return -1;
			if (vChar == 0)
				strcpy(strbuf, "Primary LAN");
			else
				strcpy(strbuf, "Secondary LAN");
			break;
#endif

#ifdef URL_BLOCKING_SUPPORT
		case SYS_DEFAULT_URL_BLK_ACTION:
			if(!mib_get_s(MIB_URL_CAPABILITY, (void *)buffer, sizeof(buffer)) ){
				return -1;
			}
			if (0 == buffer[0])
				strcpy(strbuf, "Disable");
			else if(1 == buffer[0])
				strcpy(strbuf, "Enable");
			else
				strcpy(strbuf, "err");
			break;
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
		case SYS_DEFAULT_DOMAIN_BLK_ACTION:
			if(!mib_get_s(MIB_DOMAINBLK_CAPABILITY, (void *)buffer, sizeof(buffer)) ){
				return -1;
			}
			if (0 == buffer[0])
				strcpy(strbuf, "Disable");
			else if(1 == buffer[0])
				strcpy(strbuf, "Enable");
			else
				strcpy(strbuf, "err");
			break;
#endif
#ifdef CONFIG_DEV_xDSL
		case SYS_DSL_OPSTATE:
			getAdslInfo(ADSL_GET_MODE, buffer, 64);
			if (buffer[0] != '\0')
				sprintf(strbuf, "%s,", buffer);
			getAdslInfo(ADSL_GET_STATE, buffer, 64);
			strcat(strbuf, buffer);
			break;
#ifdef CONFIG_USER_XDSL_SLAVE
		case SYS_DSL_SLV_OPSTATE:
			getAdslSlvInfo(ADSL_GET_MODE, buffer, 64);
			if (buffer[0] != '\0')
				sprintf(strbuf, "%s,", buffer);
			getAdslSlvInfo(ADSL_GET_STATE, buffer, 64);
			strcat(strbuf, buffer);
			break;
#endif /*CONFIG_USER_XDSL_SLAVE*/
#endif
#if defined(CONFIG_RTL_8676HWNAT) || defined(CONFIG_USER_VLAN_ON_LAN)
		case SYS_LAN1_VID:
		case SYS_LAN2_VID:
		case SYS_LAN3_VID:
		case SYS_LAN4_VID:
			mib_chain_get(MIB_SW_PORT_TBL, id - SYS_LAN1_VID, &sw_entry);
			sprintf(strbuf, "%hu", sw_entry.vid);
			break;
		case SYS_LAN1_STATUS:
		case SYS_LAN2_STATUS:
		case SYS_LAN3_STATUS:
		case SYS_LAN4_STATUS:
			getLANPortStatus(id - SYS_LAN1_STATUS, strbuf);
			break;
#endif
#ifdef CONFIG_IPV6
#ifdef DHCPV6_ISC_DHCP_4XX
		case SYS_DHCPV6_MODE:
			if(!mib_get_s( MIB_DHCPV6_MODE, (void *)buffer, sizeof(buffer)) )
				return -1;
			strcpy(strbuf, dhcp_mode[(DHCP_TYPE_T)buffer[0]]);
			break;

		case SYS_DHCPV6_RELAY_UPPER_ITF:
			if(!mib_get_s( MIB_DHCPV6R_UPPER_IFINDEX, (void *)&vUInt, sizeof(vUInt)) )
				return -1;

			if (vUInt != DUMMY_IFINDEX)
			{
				ifGetName(vUInt, strbuf, 8);
			}
			else
			{
				snprintf(strbuf, 6, "None");
				return 0;
			}
			break;
#endif

		case SYS_LAN_IP6_LL:
			i=getifip6(LANIF, IPV6_ADDR_LINKLOCAL, ip6_addr, 1);
			if (!i)
				strbuf[0] = 0;
			else
				if ( inet_ntop(PF_INET6, &ip6_addr[0].addr, tmpBuf, INET6_ADDRSTRLEN) == NULL){
					strbuf[0] = 0;
				}else{
#ifdef CONFIG_00R0
					sprintf(strbuf, "%s/%d", tmpBuf, 128);
#else
					char *dad_str = "(IP충돌)";
					if(!isDadState(tmpBuf, (char*)LANIF))
						dad_str= "";
					sprintf(strbuf, "%s/%d%s", tmpBuf, ip6_addr[0].prefix_len, dad_str);
#endif
				}
			break;
		case SYS_LAN_IP6_LL_NO_PREFIX:
			i=getifip6((char *)LANIF, IPV6_ADDR_LINKLOCAL, ip6_addr, 1);
			if (!i)
				strbuf[0] = 0;
			else {
				inet_ntop(PF_INET6, &ip6_addr[0].addr, tmpBuf, INET6_ADDRSTRLEN);
				sprintf(strbuf, "%s", tmpBuf);
			}
			break;
		case SYS_WAN_DNS:
			{
				FILE *file;
				char line[256];

				file = fopen("/var/resolv6.conf.br0", "r");
				i = 0;

				if (file) {
					while (fgets(line, sizeof(line), file)) {
						if(i == 0)
							sprintf(strbuf, "%s", line);
						else
							sprintf(strbuf+strlen(strbuf), ", %s", line);
						i++;
					}
					fclose(file);
				} else {
					strbuf[0] = 0;
				}
			}
			break;
		case SYS_LAN_IP6_GLOBAL:
			i=getifip6(LANIF, IPV6_ADDR_UNICAST, ip6_addr, 6);
			strbuf[0] = 0;
			if (i) {
				for (len=0; len<i; len++) {
					char *dad_str = "(IP충돌)";
					inet_ntop(PF_INET6, &ip6_addr[len].addr, tmpBuf, INET6_ADDRSTRLEN);
					if(!isDadState(tmpBuf, (char*)LANIF))
						dad_str= "";
#ifdef CONFIG_00R0
					if (len == 0)
						sprintf(strbuf, "%s/%d", tmpBuf, 128);
					else
						sprintf(strbuf+strlen(strbuf), ", %s/%d", tmpBuf, 128);
#else
					if (len == 0)
						sprintf(strbuf, "%s/%d%s",
								tmpBuf, ip6_addr[len].prefix_len,
								dad_str);
					else
						sprintf(strbuf + strlen(strbuf), ", %s/%d%s",
								tmpBuf, ip6_addr[len].prefix_len,
								dad_str);
#endif
				}
			}
			break;
		case SYS_LAN_IP6_GLOBAL_NO_PREFIX:
			i=getifip6((char *)LANIF, IPV6_ADDR_UNICAST, ip6_addr, 6);
			strbuf[0] = 0;
			if (i) {
				for (len=0; len<i; len++) {
					inet_ntop(PF_INET6, &ip6_addr[len].addr, tmpBuf, INET6_ADDRSTRLEN);
					if (len == 0)
						sprintf(strbuf, "%s", tmpBuf);
					else
						sprintf(strbuf + strlen(strbuf), ", %s", tmpBuf);
				}
			}
			break;
#endif // of CONFIG_IPV6

		default:
			return -1;
	}

	return 0;
}

int rtk_chk_is_local_client(int fd)
{
	struct in_addr inAddr;
	socklen_t sock_len;
#ifdef IPV6
	struct sockaddr_in6 sin6;
	struct ipv6_ifaddr ip6_addr[6];
#else
	struct sockaddr_in sin;
#endif
	char defPassword[MAX_NAME_LEN];
	int isLanClient = 0;

#ifdef IPV6
	memset(&sin6, 0, sizeof(struct sockaddr_in6));
	sock_len = sizeof(struct sockaddr_in6);
	if (!getsockname(fd, (struct sockaddr*) &sin6, &sock_len))
	{
		if (0 == sin6.sin6_addr.s6_addr32[0])//it must be v4 mapping to v6 address
		{
			if (getInAddr((char *)LANIF, IP_ADDR, &inAddr) == 1)
			{
				if (sin6.sin6_addr.s6_addr32[3] == inAddr.s_addr)
					isLanClient = 1;
#ifdef CONFIG_SECONDARY_IP
				else
				{
					if (getInAddr((char *)LAN_ALIAS, IP_ADDR, &inAddr) == 1)
					{
						if (sin6.sin6_addr.s6_addr32[3] == inAddr.s_addr)
							isLanClient = 1;
					}
				}
#endif
			}
		}
		else
		{
			int addrNum, k;

			addrNum = getifip6(LANIF, IPV6_ADDR_LINKLOCAL, ip6_addr, 6);
			for (k=0; k<addrNum; k++)
			{
				if (!memcmp(&ip6_addr[k].addr, &sin6.sin6_addr, 16))
				{
					isLanClient = 1;
					break;
				}
			}
		}
	}
#else
	sock_len = sizeof(struct sockaddr_in);
	if (!getsockname(fd, (struct sockaddr*) &sin, &sock_len))
	{
		if (getInAddr((char *)LANIF, IP_ADDR, &inAddr) == 1)
		{
			if (sin.sin_addr.s_addr == inAddr.s_addr)
				isLanClient = 1;
#ifdef CONFIG_SECONDARY_IP
			else
			{
				if (getInAddr((char *)LAN_ALIAS, IP_ADDR, &inAddr) == 1)
				{
					if (sin6.sin6_addr.s6_addr32[3] == inAddr.s_addr)
						isLanClient = 1;
				}
			}
#endif
		}
	}
#endif

	return (isLanClient);
}
int ifWanNum(const char *name)
{
	int ifnum=0;
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	int type;
	unsigned char protocol = IPVER_IPV4_IPV6; // 1:IPv4, 2: IPv6, 3: Both. Mason Yu. MLD Proxy

	if ( !strcmp(name, "all") )
		type = 0;
	else if ( !strcmp(name, "rt") )
	{
		type = 1;	// route interface
		protocol = IPVER_IPV4;
	}
	else if ( !strcmp(name, "rtv6") )	// Mason Yu. MLD Proxy
	{
		type = 1;	// route interface
		protocol = IPVER_IPV6;
	}
	else if ( !strcmp(name, "br") )
		type = 2;	// bridge interface
	else
		type = 1;	// default to route

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);

	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			return -1;
		}

		if (Entry.enable == 0)
			continue;
// Mason Yu. MLD Proxy
#ifdef CONFIG_IPV6
		if ( protocol == IPVER_IPV4_IPV6 || Entry.IpProtocol == IPVER_IPV4_IPV6 || protocol == Entry.IpProtocol ) {
			if (type == 2) {
				if (Entry.cmode == CHANNEL_MODE_BRIDGE)
				{
					ifnum++;
				}
			}
			else {  // rt or all (1 or 0)
				if (type == 1 && Entry.cmode == CHANNEL_MODE_BRIDGE)
					continue;

				ifnum++;
			}
		}
#else
		if (type == 2) {
			if (Entry.cmode == CHANNEL_MODE_BRIDGE)
			{
				ifnum++;
			}
		}
		else {  // rt or all (1 or 0)
			if (type == 1 && Entry.cmode == CHANNEL_MODE_BRIDGE)
				continue;

			ifnum++;
		}
#endif

	}

	return ifnum;
}

void createFTPAclChain()
{
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_FTP_ACCOUNT);
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_FTP_ACCOUNT);
}

void clearFTPAclRules()
{
	va_cmd(IPTABLES, 2, 1, "-F",(char *)FW_FTP_ACCOUNT);
	va_cmd(IP6TABLES, 2, 1, "-F",(char *)FW_FTP_ACCOUNT);
}

void setupFTPAclRules(int ftp_enable, int ftp_port)
{
	char act[3] = "-A";
	char ftp_port_str[16] = {0};
	snprintf(ftp_port_str,sizeof(ftp_port_str)-1,"%d",ftp_port);

	/*0:ftpserver disable;1:local enbale,remote disable;2:local disable,remote enable;3:local enbale,remote enbale*/
	if(ftp_enable == 0)
	{
		//iptables -A ftp_account -i nas+  -p TCP --dport 21 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_DROP);
		//iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_DROP);
		//iptables -A ftp_account -i br0 -p TCP --dport 21 -j ACCEPT
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_DROP);

		//iptables -A ftp_account -i nas+  -p TCP --dport 21 -j DROP
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_DROP);
		//iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j DROP
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_DROP);
		//iptables -A ftp_account -i br0 -p TCP --dport 21 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_DROP);
	}
	else if(ftp_enable == 1)
	{
		//iptables -A ftp_account -i nas+  -p TCP --dport 21 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_DROP);
		//iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_DROP);
		//iptables -A ftp_account -i br0 -p TCP --dport 21 -j ACCEPT
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_ACCEPT);

		//iptables -A ftp_account -i nas+  -p TCP --dport 21 -j DROP
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_DROP);
		//iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j DROP
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_DROP);
		//iptables -A ftp_account -i br0 -p TCP --dport 21 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_ACCEPT);
	}
	else if(ftp_enable == 2)
	{
		//iptables -A ftp_account -i nas+  -p TCP --dport 21 -j ACCEPT
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_ACCEPT);
		//iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j ACCEPT
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_ACCEPT);
		//iptables -A ftp_account -i br0 -p TCP --dport 21 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_DROP);

		//iptables -A ftp_account -i nas+  -p TCP --dport 21 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_ACCEPT);
		//iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_ACCEPT);
		//iptables -A ftp_account -i br0 -p TCP --dport 21 -j DROP
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_DROP);
	}
	else if(ftp_enable == 3)
	{
		//iptables -A ftp_account -i nas+  -p TCP --dport 21 -j ACCEPT
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_ACCEPT);
		//iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j ACCEPT
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_ACCEPT);
		//iptables -A ftp_account -i br0 -p TCP --dport 21 -j ACCEPT
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_ACCEPT);

		//iptables -A ftp_account -i nas+  -p TCP --dport 21 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_ACCEPT);
		//iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_ACCEPT);
		//iptables -A ftp_account -i br0 -p TCP --dport 21 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, ftp_port_str, "-j", (char *)FW_ACCEPT);
	}

}

#ifdef CONFIG_CU
void ftpserver_init_withconf(char *conf,int ftp_port, unsigned char anonymous, char *path)
{
	char buf[256] ={0};
	FILE *fptr = NULL;

	fptr = fopen(conf,"w+");
	if(fptr == NULL)
		return;

	snprintf(buf,sizeof(buf)-1,"listen_port=%d\n",ftp_port);
	fputs(buf, fptr);
	fputs("ftp_username=CUAdmin\n", fptr);
	memset(buf,0,sizeof(buf));
	snprintf(buf,sizeof(buf)-1,"anon_root=/mnt/%s\n",path);
	fputs(buf, fptr);
	fputs("anon_upload_enable=YES\n", fptr);
	fputs("anon_mkdir_write_enable=YES\n", fptr);
	fputs("anon_other_write_enable=YES\n", fptr);
	fputs("local_enable=YES\n", fptr);
	fputs("write_enable=YES\n", fptr);
	fputs("local_umask=022\n", fptr);
	fputs("dirmessage_enable=YES\n", fptr);
	fputs("xferlog_enable=YES\n", fptr);
	fputs("connect_from_port_20=YES\n", fptr);
	fputs("xferlog_std_format=YES\n", fptr);
	fputs("listen=NO\n", fptr);
	fputs("listen_ipv6=YES\n", fptr);
	fputs("userlist_enable=YES\n", fptr);
	fputs("userlist_deny=YES\n", fptr);
	fputs("userlist_file=/etc/vsftpd.users\n", fptr);
	memset(buf,0,sizeof(buf));
	snprintf(buf,sizeof(buf)-1,"secure_chroot_dir=/mnt/%s\n",path);
	fputs(buf, fptr);
	memset(buf,0,sizeof(buf));
	snprintf(buf,sizeof(buf)-1,"local_root=/mnt/%s\n",path);
	fputs(buf, fptr);
	fputs("download_enable=YES\n", fptr);
	fputs("guest_enable=YES\n", fptr);
	fputs("guest_username=CUAdmin\n", fptr);
	fputs("virtual_use_local_privs=YES\n", fptr);
	fputs("chroot_list_enable=NO\n", fptr);
	fputs("chroot_local_user=NO\n", fptr);
	fputs("user_config_dir=/etc/vsftpd/vuser_conf\n", fptr);
	fputs("ascii_upload_enable=YES\n", fptr);
	fputs("ascii_download_enable=YES\n", fptr);
    if(anonymous == 0)
         fputs("anonymous_enable=NO\n", fptr);
    else
         fputs("anonymous_enable=YES\n", fptr);

	fclose(fptr);
}

void setup_ftp_servers()
{
	FTP_SERVER_T ftp_server_entry;
	char ftp_conf[128] = {0};
	char ftp_userlist[128] = {0};
	char vsftpd_cmd[512] = {0};
	char cmd[128]={0};
	char *pPath = NULL;
	int total = 0, i = 0;
	FILE *fp = NULL;

	//create files
    fp = fopen("/var/vsftpd.user_list", "w+");
    fclose(fp);
    fp = fopen("/var/vsftpd.users", "w+");
    fclose(fp);


	clearFTPAclRules();
	total = mib_chain_total(CWMP_FTP_SERVER);
	for(i=0; i<total; i++)
	{
		if (!mib_chain_get(CWMP_FTP_SERVER, i, (void *)&ftp_server_entry))
			continue;

		printf("%s:%d\n",__FUNCTION__,__LINE__);
		memset(ftp_conf,0,sizeof(ftp_conf));
		snprintf(ftp_conf,sizeof(ftp_conf)-1,"/var/vsftpd%d.conf",i+1);

		if(ftp_server_entry.path[0] == '/')
			pPath = (char*)&ftp_server_entry.path[1];
		else
			pPath = ftp_server_entry.path;

		//printf("-----------path: %s --------------\n",pPath);
		ftpserver_init_withconf(ftp_conf,ftp_server_entry.port,ftp_server_entry.anonymous,pPath);

		memset(ftp_userlist,0,sizeof(ftp_userlist));
		snprintf(ftp_userlist,sizeof(ftp_userlist)-1,"/var/ftp_userList%d",i+1);
		if((fp = fopen(ftp_userlist, "w+"))){
			if( strlen(ftp_server_entry.username) != 0 && strlen(ftp_server_entry.password) != 0)
			{
				sprintf(cmd, "%s %s\n", ftp_server_entry.username, ftp_server_entry.password);
				fputs(cmd, fp);
			}
			fclose(fp);
		}

		setupFTPAclRules(ftp_server_entry.enable,ftp_server_entry.port);
		if(ftp_server_entry.enable){
			memset(vsftpd_cmd,0,sizeof(vsftpd_cmd));
			snprintf(vsftpd_cmd,sizeof(vsftpd_cmd)-1,"vsftpd %s -u%d &",ftp_conf,i+1);
			printf("%s:%d %s\n",__FUNCTION__,__LINE__,vsftpd_cmd);
			system(vsftpd_cmd);
		}

	}



}
#endif


#ifdef DOS_SUPPORT
void DoS_syslog(int signum)
{
	FILE *fp;
	char buff[1024];

	buff[0] = 0;
	printf("%s enter.\n", __func__);
	fp = fopen("/proc/dos_syslog","r");
	if(fp) {
		fgets(buff, sizeof(buff), fp);
		if (buff[0])
			syslog(LOG_WARNING, "%s", buff);
		fclose(fp);
	}
}
#endif

#ifdef CONFIG_USER_QUAGGA
static char ZEBRA_CONF[] = "/var/zebra.conf";
#endif

#if defined(CONFIG_USER_QUAGGA_RIPNGD) || defined(CONFIG_USER_QUAGGA_RIP)
void restart_zebra(void)
{
    int zebra_pid;

    zebra_pid = read_pid((char *)ZEBRA_PID);
    if (zebra_pid >= 1) {
        if (kill(zebra_pid, SIGTERM) != 0) {
            printf("Could not kill pid '%d'\n", zebra_pid);
        }
        kill(zebra_pid, SIGKILL);
    }

    va_cmd(ZEBRA, 5, 0, "-d", "-f", ZEBRA_CONF, "-i", ZEBRA_PID);

}
#endif

#if defined(CONFIG_USER_QUAGGA_RIPNGD)
static const char RIPNGD_CONF[] = "/var/ripngd.conf";

// RIPNGD server configuration
// return value:
// 0  : not enabled
// 1  : successful
// -1 : startup failed
int startRipng(void)
{
	FILE *fp;
	char *argv[7];
	int pid;
	unsigned char ripngOn;
	unsigned int entryNum, i, j;
	MIB_CE_RIPNG_T Entry;
	char ifname[IFNAMSIZ];
	unsigned char modelName[512];
	char userPass[MAX_PASSWD_LEN];

	if (mib_get(MIB_RIPNG_ENABLE, (void *)&ripngOn) == 0)
		ripngOn = 0;

	//kill old ripngd
	pid = read_pid((char *)RIPNGD_PID);
	if (pid >= 1) {
		if (kill(pid, SIGTERM) != 0)
			printf("Could not kill pid '%d'\n", pid);
	}

	if (!ripngOn)
		return 0;

	printf("start ripng!\n");
	mib_get( MIB_HW_CWMP_PRODUCTCLASS, (void *)modelName);
	mib_get( MIB_SUSER_PASSWORD, (void *)userPass );
	//create zebra.conf
	if ((fp = fopen(ZEBRA_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", ZEBRA_CONF);
		return -1;
	}
	fprintf(fp, "hostname AA%s\n", modelName);
	fprintf(fp, "password %s\n", userPass);
	fprintf(fp, "enable password %s\n", userPass);
	entryNum = mib_chain_total(MIB_RIPNG_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_RIPNG_TBL, i, (void *)&Entry))
		{
  			printf("Get MIB_OSPF_TBL chain record error!\n");
			fclose(fp);
			return -1;
		}
		if (!ifGetName(Entry.ifIndexV6, ifname, sizeof(ifname)))
		{
			strncpy(ifname, BRIF, strlen(BRIF));
			ifname[strlen(BRIF)] = '\0';
		}
		fprintf(fp, "interface %s\n", ifname);
		fprintf(fp, "multicast\n");
	}
	fclose(fp);

	//create ripng.conf
	if ((fp = fopen(RIPNGD_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", RIPNGD_CONF);
		return -1;
	}
	fprintf(fp, "hostname AA%s\n", modelName);
	fprintf(fp, "password %s\n", userPass);
	fprintf(fp, "router ripng\n");

	entryNum = mib_chain_total(MIB_RIPNG_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_RIPNG_TBL, i, (void *)&Entry))
		{
  			printf("Get MIB_RIPNG_TBL chain record error!\n");
			fclose(fp);
			return -1;
		}
		if (!ifGetName(Entry.ifIndexV6, ifname, sizeof(ifname)))
		{
			strncpy(ifname, BRIF, strlen(BRIF));
			ifname[strlen(BRIF)] = '\0';
		}
		fprintf(fp, "network %s\n", ifname);
	}
	fclose(fp);

	//start zebra
	restart_zebra();

	//start ripngd
	argv[1] = "-d";
	argv[2] = "-f";
	argv[3] = "/var/ripngd.conf";
	argv[4] = "-i";
	argv[5] = "/var/run/ripngd.pid";
	argv[6] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s %s %s %s\n", RIPNGD, argv[1], argv[2], argv[3], argv[4], argv[5]);
	do_cmd(RIPNGD, argv, 0);

	return 1;
}
#endif


#if defined(CONFIG_USER_ROUTED_ROUTED) || defined(CONFIG_USER_ZEBRA_OSPFD_OSPFD)
#ifdef CONFIG_00R0
static void sync_rip_config(void)
{
	MIB_CE_ATM_VC_T wan_entry;
	MIB_CE_RIP_T rip_entry;
	int total_wan = 0, total_rip = 0;
	int i, j;

	total_wan = mib_chain_total(MIB_ATM_VC_TBL);

	for(i = 0 ; i < total_wan ; i++)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, &wan_entry) == 0)
			continue;

		// entry may be deleted, so get total entries again
		total_rip = mib_chain_total(MIB_RIP_TBL);
		for (j = (total_rip - 1); j >= 0 ; j--)
		{
			if(mib_chain_get(MIB_RIP_TBL, j, &rip_entry) == 0)
				continue;

			if(wan_entry.ifIndex != rip_entry.ifIndex)
				continue;
			if(!wan_entry.enable || !wan_entry.enableRIPv2)
			{
				//deconfig
				if(rip_entry.add_by_wan)
					mib_chain_delete(MIB_RIP_TBL, j);
			}
			else if(wan_entry.enable)
			{
				if(wan_entry.enableRIPv2)
				{
					//update setting
					rip_entry.enable = 1;
					rip_entry.add_by_wan = 1;
					rip_entry.ifIndex = wan_entry.ifIndex;
					rip_entry.receiveMode = RIP_V2;
					rip_entry.sendMode = RIP_V2;
					mib_chain_update(MIB_RIP_TBL, &rip_entry, j);
				}
			}
			break;
		}

		// Add new entry
		if(j < 0 && wan_entry.enable && wan_entry.enableRIPv2)
		{
			unsigned char rip_enable = 1;
			memset(&rip_entry, 0, sizeof(MIB_CE_RIP_T));

			rip_entry.enable = 1;
			rip_entry.add_by_wan = 1;
			rip_entry.ifIndex = wan_entry.ifIndex;
			rip_entry.receiveMode = RIP_V2;
			rip_entry.sendMode = RIP_V2;

			mib_set(MIB_RIP_ENABLE, &rip_enable);
			mib_chain_add(MIB_RIP_TBL, &rip_entry);
		}
	}
}
#endif
static const char ROUTED_CONF[] = "/var/run/routed.conf";

int startRip(void)
{
	FILE *fp;
	unsigned char ripOn, ripInf, ripVer;
	int rip_pid;

	unsigned int entryNum, i;
	MIB_CE_RIP_T Entry;
	char ifname[IFNAMSIZ], receive_mode[5], send_mode[5];
#ifdef CONFIG_00R0
	sync_rip_config();
#endif
	if (mib_get_s(MIB_RIP_ENABLE, (void *)&ripOn, sizeof(ripOn)) == 0)
		ripOn = 0;
	rip_pid = read_pid((char *)ROUTED_PID);
	if (rip_pid >= 1) {
		// kill it
		if (kill(rip_pid, SIGTERM) != 0) {
			printf("Could not kill pid '%d'", rip_pid);
		}
	}

	if (!ripOn)
		return 0;

	printf("start rip!\n");
	if ((fp = fopen(ROUTED_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", ROUTED_CONF);
		return -1;
	}

	entryNum = mib_chain_total(MIB_RIP_TBL);

	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_RIP_TBL, i, (void *)&Entry))
		{
			fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
			chmod(ROUTED_CONF, 0666);
#endif
  			printf("Get MIB_RIP_TBL.%d chain record error!\n", i);
			continue;
		}

		if(Entry.enable == 0)
			continue;

		if (!ifGetName(Entry.ifIndex, ifname, sizeof(ifname))) {
			strncpy(ifname, BRIF, strlen(BRIF));
			ifname[strlen(BRIF)] = '\0';
		}

		fprintf(fp, "network %s %d %d\n", ifname, Entry.receiveMode, Entry.sendMode);
	}

	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
			chmod(ROUTED_CONF, 0666);
#endif
	// Modified by Mason Yu for always as a supplier for RIP
	//va_cmd(ROUTED, 0, 0);
	// routed will fork, so do wait to avoid zombie process
	va_niced_cmd(ROUTED, 1, 1, "-s");
	return 1;
}
#endif	// of CONFIG_USER_ROUTED_ROUTED

#ifdef CONFIG_USER_QUAGGA
static  char RIPD_CONF[] = "/var/ripd.conf";
static  char OSPFD_CONF[] = "/var/ospfd.conf";
static  char BGPD_CONF[] = "/var/bgpd.conf";
static  char ISISD_CONF[] = "/var/isisd.conf";

int rtk_stop_igp(unsigned char igpType)
{
	int pid;

	switch (igpType)
	{
	case IGP_OSPF:
		//try to kill old ospfd
		pid = read_pid((char *)OSPFD_PID);
		if (pid >= 1) {
			if (kill(pid, SIGTERM) != 0)
				printf("Could not kill pid '%d'\n", pid);
		}
		break;
	case IGP_RIP:
		//try to kill old ripd
		pid = read_pid((char *)RIPD_PID);
		if (pid >= 1) {
			if (kill(pid, SIGTERM) != 0)
				printf("Could not kill pid '%d'\n", pid);
		}
		break;
	case IGP_BGP:
		//try to kill old bgpd
		pid = read_pid((char *)BGPD_PID);
		if (pid >= 1) {
			if (kill(pid, SIGTERM) != 0)
				printf("Could not kill pid '%d'\n", pid);
		}
		break;
	case IGP_ISIS:
		//try to kill old isisd
		pid = read_pid((char *)ISISD_PID);
		if (pid >= 1) {
			if (kill(pid, SIGTERM) != 0)
				printf("Could not kill pid '%d'\n", pid);
		}
		break;
	default:
		break;
	}

	pid = read_pid((char *)ZEBRA_PID);
	if (pid >= 1) {
		if (kill(pid, SIGTERM) != 0) {
			printf("Could not kill pid '%d'\n", pid);
		}
	}

	return 0;
}

int rtk_start_igp(void)
{
	FILE *fp;
	char *argv[6];
	int pid;
	unsigned char igpType;
	char userName[MAX_NAME_LEN];
	char userPass[MAX_NAME_LEN];
	char ifname[IFNAMSIZ];
	MIB_CE_ATM_VC_T Entry;
	int entryNum, i, j;
	int idx=0;

	if (!mib_get(MIB_IGP_TYPE, (void *)&igpType))
		igpType = IGP_NONE;

	if (IGP_NONE == igpType)
		return 0;

	mib_get( MIB_SUSER_NAME, (void *)userName );
	mib_get( MIB_SUSER_PASSWORD, (void *)userPass );

	//create zebra.conf
	if ((fp = fopen(ZEBRA_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", ZEBRA_CONF);
		return -1;
	}
	fprintf(fp, "hostname Zebra\n");
	fprintf(fp, "password %s\n", userPass);
	fprintf(fp, "enable password %s\n", userPass);
	fprintf(fp, "interface %s\n", BRIF);
	fprintf(fp, " multicast\n");

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if (CHANNEL_MODE_BRIDGE == Entry.cmode)
			continue;

		if (!ifGetName(Entry.ifIndex, ifname, sizeof(ifname)))
			continue;

		fprintf(fp, "interface %s\n", ifname);
		fprintf(fp, " multicast\n");
	}
	fprintf(fp, "debug zebra events\n");
	fprintf(fp, "debug zebra packet\n");
	fprintf(fp, "debug zebra rib\n");
	fprintf(fp, "log stdout\n");
	fclose(fp);

	//start zebra
	idx = 1;
	argv[idx++] = "-d";
	argv[idx++] = "-u";
	argv[idx++] = userName;
	argv[idx++] = "-f";
	argv[idx++] = ZEBRA_CONF;
	argv[idx++] = "-i";
	argv[idx++] = ZEBRA_PID;
	argv[idx++] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s %s %s %s\n", ZEBRA, argv[1], argv[2], argv[3], argv[4], argv[5]);
	do_cmd(ZEBRA, argv, 0);

	if (IGP_RIP == igpType)
	{
		MIB_CE_RIP_T Entry;

		if ((fp = fopen(RIPD_CONF, "w")) == NULL)
		{
			printf("Open file %s failed !\n", RIPD_CONF);
			return -1;
		}

		fprintf(fp, "hostname ripd\n");
		fprintf(fp, "password %s\n", userPass);

		entryNum = mib_chain_total(MIB_RIP_TBL);
		for (i=0; i<entryNum; i++)
		{
			if (!mib_chain_get(MIB_RIP_TBL, i, (void *)&Entry))
				continue;

			if(Entry.enable == 0)
				continue;

			if (!ifGetName(Entry.ifIndex, ifname, sizeof(ifname)))
			{
				strncpy(ifname, BRIF, strlen(BRIF));
				ifname[strlen(BRIF)] = '\0';
			}
			fprintf(fp, "interface %s\n", ifname);
			fprintf(fp, "router rip\n");
			fprintf(fp, "network %s\n", ifname);

			fprintf(fp, "interface %s\n", ifname);
			if (RIP_V1 == Entry.receiveMode)
				fprintf(fp, " ip rip receive version 1\n");
			else if (RIP_V2 == Entry.receiveMode)
				fprintf(fp, " ip rip receive version 2\n");
			else if (RIP_V1_V2 == Entry.receiveMode)
				fprintf(fp, " ip rip receive version 1 2\n");

			if (RIP_V1 == Entry.sendMode)
				fprintf(fp, " ip rip send version 1\n");
			else if (RIP_V2 == Entry.sendMode)
				fprintf(fp, " ip rip send version 2\n");
			else if (RIP_V1_V2 == Entry.sendMode)
				fprintf(fp, " ip rip send version 1 2\n");
		}
		fprintf(fp, "log stdout\n");
		fclose(fp);

		//start ripd
		idx = 1;
		argv[idx++] = "-d";
		argv[idx++] = "-u";
		argv[idx++] = userName;
		argv[idx++] = "-f";
		argv[idx++] = RIPD_CONF;
		argv[idx++] = "-i";
		argv[idx++] = RIPD_PID;
		argv[idx++] = NULL;
		TRACE(STA_SCRIPT, "%s %s %s %s %s %s\n", RIPD, argv[1], argv[2], argv[3], argv[4], argv[5]);
		do_cmd(RIPD, argv, 0);
	}
	else if (IGP_OSPF == igpType)
	{
		MIB_CE_OSPF_T Entry;
		char netIp[20];
		unsigned int uMask;
		unsigned int uIp;

		if ((fp = fopen(OSPFD_CONF, "w")) == NULL)
		{
			printf("Open file %s failed !\n", OSPFD_CONF);
			return -1;
		}

		fprintf(fp, "hostname ospfd\n");
		fprintf(fp, "password %s\n", userPass);
		fprintf(fp, "router ospf\n");

		entryNum = mib_chain_total(MIB_OSPF_TBL);
		for (i=0; i<entryNum; i++)
		{
			if (!mib_chain_get(MIB_OSPF_TBL, i, (void *)&Entry))
				continue;

			uIp = *(unsigned int *)Entry.ipAddr;
			uMask = *(unsigned int *)Entry.netMask;
			uIp = uIp & uMask;
			strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));

			for (j=0; j<32; j++)
				if ((uMask>>j) & 0x01)
					break;
			uMask = 32 - j;

			sprintf(netIp+strlen(netIp), "/%d", uMask);
			fprintf(fp, " network %s area 0\n", netIp);
		}
		fclose(fp);

		//start ospfd
		idx = 1;
		argv[idx++] = "-d";
		argv[idx++] = "-u";
		argv[idx++] = userName;
		argv[idx++] = "-f";
		argv[idx++] = OSPFD_CONF;
		argv[idx++] = "-i";
		argv[idx++] = OSPFD_PID;
		argv[idx++] = NULL;
		TRACE(STA_SCRIPT, "%s %s %s %s %s %s\n", OSPFD, argv[1], argv[2], argv[3], argv[4], argv[5]);
		do_cmd(OSPFD, argv, 0);
	}
	else if (IGP_BGP == igpType)
	{
		MIB_CE_BGP_NETWORK_T networkEntry;
		MIB_CE_BGP_NEIGHBOUR_T neighbourEntry;
		unsigned short bgpAS;
		unsigned char strRouterId[16];
		char netIp[20];
		unsigned int uMask;
		unsigned int uIp;

		if ((fp = fopen(BGPD_CONF, "w")) == NULL)
		{
			printf("Open file %s failed !\n", BGPD_CONF);
			return -1;
		}

		fprintf(fp, "hostname bgpd\n");
		fprintf(fp, "password %s\n", userPass);

		mib_get(MIB_BGP_AS, (void *)&bgpAS);
		fprintf(fp, "router bgp %d\n", bgpAS);
		mib_get(MIB_BGP_ROUTER_ID, (void *)strRouterId);
		fprintf(fp, " bgp router-id %s\n", strRouterId);

		entryNum = mib_chain_total(MIB_BGP_NETWORK_TBL);
		for (i=0; i<entryNum; i++)
		{
			if (!mib_chain_get(MIB_BGP_NETWORK_TBL, i, (void *)&networkEntry))
				continue;

			uIp = *(unsigned int *)networkEntry.ipAddr;
			uMask = ~((1<<(32-networkEntry.mask)) - 1);
			uIp = uIp & uMask;
			strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));
			sprintf(netIp+strlen(netIp), "/%d", networkEntry.mask);
			fprintf(fp, " network %s\n", netIp);
		}

		entryNum = mib_chain_total(MIB_BGP_NEIGHBOUR_TBL);
		for (i=0; i<entryNum; i++)
		{
			if (!mib_chain_get(MIB_BGP_NEIGHBOUR_TBL, i, (void *)&neighbourEntry))
				continue;

			fprintf(fp, " neighbor %s remote-as %d\n",
					inet_ntoa(*((struct in_addr *)neighbourEntry.ipAddr)),
					neighbourEntry.remoteAs);
		}
		fclose(fp);

		//start bgpd
		idx = 1;
		argv[idx++] = "-d";
		argv[idx++] = "-u";
		argv[idx++] = userName;
		argv[idx++] = "-f";
		argv[idx++] = BGPD_CONF;
		argv[idx++] = "-i";
		argv[idx++] = BGPD_PID;
		argv[idx++] = NULL;
		TRACE(STA_SCRIPT, "%s %s %s %s %s %s\n", BGPD, argv[1], argv[2], argv[3], argv[4], argv[5]);
		do_cmd(BGPD, argv, 0);
	}
	else if (IGP_ISIS == igpType)
	{
		MIB_CE_ISIS_T entry;
		unsigned char isisArea, isisType;
		unsigned char devAddr[MAC_ADDR_LEN];
		char strNET[32];

		if ((fp = fopen(ISISD_CONF, "w")) == NULL)
		{
			printf("Open file %s failed !\n", ISISD_CONF);
			return -1;
		}

		fprintf(fp, "hostname isisd\n");
		fprintf(fp, "password %s\n", userPass);

		mib_get(MIB_ISIS_AREA, (void *)&isisArea);
		mib_get(MIB_ISIS_TYPE, (void *)&isisType);
		mib_get(MIB_ELAN_MAC_ADDR, (void *)devAddr);
		snprintf(strNET, sizeof(strNET), "%02x.%02x%02x.%02x%02x.%02x%02x.00", isisArea, devAddr[0],
				devAddr[1], devAddr[2], devAddr[3], devAddr[4], devAddr[5]);
		fprintf(fp, "router isis %02x\n", isisArea);
		fprintf(fp, " net %s\n", strNET);

		if (ISIS_LVL_1 == isisType)
			fprintf(fp, " is-type level-1\n");
		else if (ISIS_LVL_1_2 == isisType)
			fprintf(fp, " is-type level-1-2\n");
		else if (ISIS_LVL_2 == isisType)
			fprintf(fp, " is-type level-2-only\n");

		entryNum = mib_chain_total(MIB_ISIS_TBL);
		for (i=0; i<entryNum; i++)
		{
			if (!mib_chain_get(MIB_ISIS_TBL, i, (void *)&entry))
				continue;

			if (!ifGetName(entry.ifIndex, ifname, sizeof(ifname)))
			{
				strncpy(ifname, BRIF, strlen(BRIF));
				ifname[strlen(BRIF)] = '\0';
			}
			fprintf(fp, "interface %s\n", ifname);
			fprintf(fp, " ip router isis %02x\n", isisArea);
		}
		fclose(fp);

		//start isisd
		idx = 1;
		argv[idx++] = "-d";
		argv[idx++] = "-u";
		argv[idx++] = userName;
		argv[idx++] = "-f";
		argv[idx++] = ISISD_CONF;
		argv[idx++] = "-i";
		argv[idx++] = ISISD_PID;
		argv[idx++] = NULL;
		TRACE(STA_SCRIPT, "%s %s %s %s %s %s\n", ISISD, argv[1], argv[2], argv[3], argv[4], argv[5]);
		do_cmd(ISISD, argv, 0);
	}

	return 0;
}

void rtk_restart_igp(void)
{
	unsigned char igpType;

	if (!mib_get(MIB_IGP_TYPE, (void *)&igpType))
		igpType = IGP_NONE;

	if (IGP_NONE == igpType)
		return;

	rtk_stop_igp(igpType);
	rtk_start_igp();
}
#endif//endof CONFIG_USER_QUAGGA


#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
static  char ZEBRA_CONF[] = "/etc/config/zebra.conf";
static  char OSPFD_CONF[] = "/etc/config/ospfd.conf";

// OSPF server configuration
// return value:
// 0  : not enabled
// 1  : successful
// -1 : startup failed
int startOspf(void)
{
	FILE *fp;
	char *argv[6];
	int pid;
	unsigned char ospfOn;
	unsigned int entryNum, i, j;
	MIB_CE_OSPF_T Entry;
	char netIp[20];
	unsigned int uMask;
	unsigned int uIp;

	if (mib_get_s(MIB_OSPF_ENABLE, (void *)&ospfOn, sizeof(ospfOn)) == 0)
		ospfOn = 0;
	//kill old zebra
	pid = read_pid((char *)ZEBRA_PID);
	if (pid >= 1) {
		if (kill(pid, SIGTERM) != 0) {
			printf("Could not kill pid '%d'\n", pid);
		}
	}
	//kill old ospfd
	pid = read_pid((char *)OSPFD_PID);
	if (pid >= 1) {
		if (kill(pid, SIGTERM) != 0)
			printf("Could not kill pid '%d'\n", pid);
	}

	if (!ospfOn)
		return 0;

	printf("start ospf!\n");
	//create zebra.conf
	if ((fp = fopen(ZEBRA_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", ZEBRA_CONF);
		return -1;
	}
	fprintf(fp, "hostname Router\n");
	fprintf(fp, "password zebra\n");
	fprintf(fp, "enable password zebra\n");
	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(ZEBRA_CONF, 0666);
#endif
	//create ospfd.conf
	if ((fp = fopen(OSPFD_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", OSPFD_CONF);
		return -1;
	}
	fprintf(fp, "hostname ospfd\n");
	fprintf(fp, "password zebra\n");
	fprintf(fp, "enable password zebra\n");
	fprintf(fp, "router ospf\n");
	//ql_xu test.
	//fprintf(fp, "network %s area 0\n", "192.168.2.0/24");
	entryNum = mib_chain_total(MIB_OSPF_TBL);

	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_OSPF_TBL, i, (void *)&Entry))
		{
  			printf("Get MIB_OSPF_TBL chain record error!\n");
			return -1;
		}

		uIp = *(unsigned int *)Entry.ipAddr;
		uMask = *(unsigned int *)Entry.netMask;
		uIp = uIp & uMask;
		snprintf(netIp, 20, "%s", inet_ntoa(*(struct in_addr *)&uIp));

		for (j=0; j<32; j++)
			if ((uMask>>j) & 0x01)
				break;
		uMask = 32 - j;

		sprintf(netIp, "%s/%d", netIp, uMask);
		fprintf(fp, "network %s area 0\n", netIp);
	}
	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(OSPFD_CONF, 0666);
#endif

	//start zebra
	argv[1] = "-d";
	argv[2] = "-k";
	argv[3] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s\n", ZEBRA, argv[1], argv[2]);
	do_nice_cmd(ZEBRA, argv, 0);

	//start ospfd
	argv[1] = "-d";
	argv[2] = NULL;
	TRACE(STA_SCRIPT, "%s %s\n", OSPFD, argv[1]);
	do_nice_cmd(OSPFD, argv, 0);

	return 1;
}
#endif

#ifdef CONFIG_ATM_REALTEK
#include <rtk/linux/rtl_sar.h>
#include <linux/atm.h>
#endif

//up_flag:0, itf down; non-zero: itf up
void itfcfg(char *if_name, int up_flag)
{
#ifdef EMBED
	int fd;
	struct ifreq ifr;
	char cmd;

	if (strncmp(if_name, "sar", sizeof("sar"))==0) {  //sar enable/disable
#ifdef CONFIG_ATM_REALTEK
		struct atmif_sioc mysio;

		if((fd = socket(PF_ATMPVC, SOCK_DGRAM, 0)) < 0){
			printf("socket open error\n");
			return;
		}
		mysio.number = 0;
		mysio.length = sizeof(struct SAR_IOCTL_CFG);
		mysio.arg = (void *)NULL;
		if (up_flag) ioctl(fd, SIOCDEVPRIVATE+2, &mysio);
		else         ioctl(fd, SIOCDEVPRIVATE+3, &mysio);
		if(fd!=(int)NULL)
		    close(fd);
#else
		printf( "%s: SAR interface not supportted.\n", __FUNCTION__ );
#endif
	}
	else if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
		int flags;
		if (getInFlags(if_name, &flags) == 1) {
			if (up_flag)
				flags |= IFF_UP;
			else
				flags &= ~IFF_UP;

			setInFlags(if_name, flags);
		}
		close(fd);
	}
#endif
}

//#ifdef XOR_ENCRYPT
const char XOR_KEY[] = "tecomtec";
/* used by mktemp to generate a unique temporary filename. The last six
   characters of template must be XXXXXX. */
#define MK_TEMPLATE "/tmp/cnftmp_XXXXXX"
void xor_encrypt(char *inputfile, char outputfile[])
{
	FILE *input  = fopen(inputfile, "rb");
	FILE *output = NULL;
	char *fname;
	int fd = -1;

	strcpy(outputfile, MK_TEMPLATE);
	if(input != NULL && (fd = mkstemp(outputfile)) >= 0
		&& (output = fdopen(fd, "wb")))
	{
		unsigned char buffer[MAX_CONFIG_FILESIZE];
		size_t count, i, j = 0;
		do {
			count = fread(buffer, sizeof *buffer, sizeof buffer, input);
			for(i = 0; i<count; ++i) {
				buffer[i] ^= XOR_KEY[j++];
				if(XOR_KEY[j] == '\0')
					j = 0; /* restart at the beginning of the key */
			}
			fwrite(buffer, sizeof *buffer, count, output);
		} while (count == sizeof buffer);
	}
	if(input) fclose(input);
	if(output) fclose(output);
	else if(fd >= 0) close(fd);
}

void xor_encrypt2(char *buf, int len, int *index)
{
	int i;
	int idx;
	if (NULL == buf || 0 == len) {
		return;
	}
	idx = *index;
	for (i = 0; i < len; ++i) {
		buf[i] ^= XOR_KEY[idx];
		idx++;
		if (XOR_KEY[idx] == '\0'){
			idx = 0;
		}
	}
	*index = idx;
}


static char *get_line(FILE *fp, char *s, int size)
{
	char *pstr;

	while (1) {
		if (!fgets(s, size, fp))
			return NULL;
		pstr = trim_white_space(s);
		if (strlen(pstr))
			break;
	}

	return pstr;
}

int check_xor_encrypt(char *inputfile)
{
	FILE *input  = NULL;
	char xml_line[1024];
	char *pstr;

	if(inputfile != NULL){
		input  = fopen(inputfile, "rb");
		if(input){
			while(!feof(input)){
				pstr = get_line(input, xml_line, sizeof(xml_line));
				if(pstr == NULL) continue;
				if (!strcmp(pstr, CONFIG_HEADER) || !strcmp(pstr, CONFIG_HEADER_HS)){
					fclose(input);
					return 0; ////file not encrypted
				}
				break;
			}
			fclose(input);
		}
		else
			return -1; //file doesn't exist
	}
	else
		return -1; //file doesn't exist
	return 1;//file encrypted
}

//#endif

unsigned short
ipchksum(unsigned char *ptr, int count, unsigned short resid)
{
	register unsigned int sum = resid;
       if ( count==0)
       	return(sum);

	while(count > 1) {
		//sum += ntohs(*ptr);
		sum += (( ptr[0] << 8) | ptr[1] );
		if ( sum>>31)
			sum = (sum&0xffff) + ((sum>>16)&0xffff);
		ptr += 2;
		count -= 2;
	}

	if (count > 0)
		sum += (*((unsigned char*)ptr) << 8) & 0xff00;

	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	if (sum == 0xffff)
		sum = 0;
	return (unsigned short)sum;
}

int getLinkStatus(struct ifreq *ifr)
{
	int status=0;

#ifdef __mips__
#if defined(CONFIG_RTL8672NIC)
	if (do_ioctl(SIOCGMEDIALS, ifr) == 1)
		status = 1;
#endif
#endif
	return status;
}

#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
int configWizardRedirect(int redirect_flag)
{
	unsigned char value[6] = {0}, ipaddr[16] = {0}, lan_ip_port[32] = {0};

	if (mib_get_s(MIB_ADSL_LAN_IP, (void *)value, sizeof(value)) != 0) {
		strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
		ipaddr[15] = '\0';
		sprintf(lan_ip_port, "%s:80", ipaddr);

		va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", "WebRedirect_Wizard");

		if (redirect_flag) {
			//iptables -t nat -A WebRedirect_Wizard -p tcp -d 192.168.0.1 --dport 80 -j RETURN
			va_cmd(IPTABLES, 14, 1, "-t", "nat", "-A", "WebRedirect_Wizard",	"-i", "br+", "-p", "tcp", "-d", ipaddr, "--dport", "80", "-j", (char *)FW_RETURN);

			//iptables -t nat -A WebRedirect_Wizard -p tcp --dport 80 -j DNAT --to-destination 192.168.0.1:80
			va_cmd(IPTABLES, 14, 1, "-t", "nat", "-A", "WebRedirect_Wizard", "-i", "br+", "-p", "tcp", "--dport", "80", "-j", "DNAT", "--to-destination", lan_ip_port);
			va_cmd(IPTABLES, 14, 1, "-t", "nat", "-A", "WebRedirect_Wizard", "-i", "br+", "-p", "tcp", "--dport", "443", "-j", "DNAT", "--to-destination", lan_ip_port);
		}
	}
	return 0;
}

int get_ponStatus()
{
	int	state;

	/* check if wan cable is connected */
	state = getGponONUState();
	if(state > RTK_GPONMAC_FSM_STATE_O1){
		return 1;
	}

	return 0;
}


int update_redirect_by_ponStatus(void)
{
	int isConnect = 0, need_redirect = 0;
	int state;

	state = getGponONUState();
	if (state > RTK_GPONMAC_FSM_STATE_O1) {
		isConnect = 1;
	}

	unsigned char internet_status = 0;
	mib_get_s(MIB_USER_TROUBLE_WIZARD_INTERNET_STATUS, (void *)&internet_status, sizeof(internet_status));
	if (isConnect == 0 && internet_status) {
		need_redirect = 1;
	}

	if (need_redirect == 1) {
		configWizardRedirect(1);
	}
	else {
		configWizardRedirect(0);
	}

#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
	// update dnsmasq
	restart_dnsrelay_ex("all", 0);
#endif

	return 0;
}
#endif

#ifdef TIME_ZONE
int isTimeSynchronized(void)
{
	FILE *fp;
	unsigned int timeStatus = eTStatusError;

	fp=fopen("/tmp/timeStatus","r");
	if(fp){
		fscanf(fp,"%d",&timeStatus);
		fclose(fp);
	}

	if(timeStatus == eTStatusSynchronized)
		return 1;

	return 0;
}

#ifdef CONFIG_E8B
static int getProcessPid(const char *pidfile)
{
    FILE *f;
	int pid=0;
	//fprintf(stderr,"pidfile=%s\n",pidfile);
	if((f = fopen(pidfile, "r")) == NULL)
		return 0;
	if(fscanf(f, "%d\n", &pid) !=1)
		printf("fscanf failed: %s %d\n", __func__, __LINE__);
	fclose(f);
	//fprintf(stderr,"pid=%d\n",pid);
	return pid;
}
unsigned int check_ntp_if()
{
	unsigned char if_type = 0;
	unsigned char wan_type = 0;
	unsigned int if_wan, ifIndex = DUMMY_IFINDEX;
	MIB_CE_ATM_VC_T entry = {0};
	int total, i;

	mib_get_s(MIB_NTP_IF_WAN, &if_wan, sizeof(if_wan));
	mib_get_s(MIB_NTP_IF_TYPE, &if_type, sizeof(if_type));
	switch(if_type)
	{
	case 0: //INTERNET
		wan_type = X_CT_SRV_INTERNET;
		break;
	case 1: //VOICE
		wan_type = X_CT_SRV_VOICE;
		break;
	case 2: //TR069
		wan_type = X_CT_SRV_TR069;
		break;
	case 3:
		wan_type = X_CT_SRV_OTHER;
		break;
	}

	total = mib_chain_total(MIB_ATM_VC_TBL);

	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, &entry) == 0)
			continue;

		if(entry.cmode == 0) //birdge
			continue;

#if defined(CONFIG_SUPPORT_HQOS_APPLICATIONTYPE)
		if((entry.applicationtype & X_CT_SRV_INTERNET) && (entry.othertype == OTHER_HQOS_TYPE))
			continue;
#endif

		if(entry.applicationtype & wan_type)
		{
			// select first match
			if(ifIndex == DUMMY_IFINDEX)
				ifIndex = entry.ifIndex;

			if(entry.ifIndex == if_wan)
			{
				ifIndex = if_wan;
				break;	//specified is OK
			}
		}
	}

	//update NTP WAN
	if(ifIndex != if_wan)
	{
		mib_set(MIB_NTP_IF_WAN, &ifIndex);
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
		reload_dnsrelay("all");
#endif
		sleep(1);	//wait a second for DNS updating
	}

	return ifIndex;
}

#include <sys/time.h>
// Mason Yu. 2630-e8b
int setupNtp(int type)
{
	unsigned char ntpEnabled;
	unsigned char ntpServerHost1[MAX_V6_IP_LEN];
	unsigned char ntpServerHost2[MAX_V6_IP_LEN];
	unsigned char ntpServerHost3[MAX_V6_IP_LEN];
	unsigned char ntpServerHost4[MAX_V6_IP_LEN];
	unsigned char ntpServerHost5[MAX_V6_IP_LEN];
	unsigned int if_wan;
	unsigned int interval;
	int pid, num, i, status = 0, sys_pid = -1;
	char ifname[IFNAMSIZ];
	MIB_CE_ATM_VC_T vc_entry;
	char argv_fmt[128];
	FILE *fp;
	unsigned int index = 0;
	unsigned char dst_enabled = 1;
	unsigned char opmode = 0;

	//config TZ no matter NTP enable or not
	if (mib_get_s(MIB_NTP_TIMEZONE_DB_INDEX, &index, sizeof(index))) {
#if defined( __UCLIBC__) || (defined(__GLIBC__) && !defined(CONFIG_USER_GLIBC_TIMEZONE))
		if ((fp = fopen("/etc/TZ", "w")) != NULL) {
			status |= !mib_get_s(MIB_DST_ENABLED, &dst_enabled, sizeof(dst_enabled));
			fprintf(fp, "%s\n", get_tz_string(index, dst_enabled));
			fclose(fp);
		} else {
			status |= 1;
		}
#else
#ifdef CONFIG_USER_GLIBC_TIMEZONE
		char cmd[128];
		char tz_location[64]={0};

		system("rm /var/localtime");
		mib_get_s(MIB_DST_ENABLED, &dst_enabled, sizeof(dst_enabled));
		if (dst_enabled || !is_tz_dst_exist(index)) {
			snprintf(tz_location,sizeof(tz_location),"%s",get_tz_location_cli(index));
			format_tz_location(tz_location);

			snprintf(cmd, sizeof(cmd), "ln -s /usr/share/zoneinfo/%s /var/localtime",tz_location);
		} else {
			const char * tz_offset;

			tz_offset = get_format_tz_utc_offset(index);

			snprintf(cmd, sizeof(cmd), "ln -s /usr/share/zoneinfo/Etc/GMT%s /var/localtime",tz_offset);
		}

		//fprintf(stderr,cmd);
		system(cmd);
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
		if ((fp = fopen("/etc/TZ", "w")) != NULL) {
			status |= !mib_get_s(MIB_DST_ENABLED, &dst_enabled, sizeof(dst_enabled));
			fprintf(fp, "%s\n", get_tz_string(index, dst_enabled));
			fclose(fp);
		}
#endif
#endif
		{
			//update system timezone
			tzset();
			struct timezone tz;
			gettimeofday(NULL, &tz);
			tz.tz_minuteswest = timezone/60;
			if(settimeofday(NULL, &tz) != 0)
				printf("!!! Set system timezone fail !!!\n");
		}
	} else {
		status |= 1;
	}



	DOCMDINIT;
	switch (type) {
	case SNTP_DISABLED:
		va_cmd("/bin/killall", 1, 1, "vsntp");
		status |= 1;

		// update dnsmasq config
		restart_dnsrelay();

		break;
	case SNTP_ENABLED:
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
		// update dnsmasq config
		restart_dnsrelay_ex("all", 0);
#endif
		sleep(1);
		pid = getProcessPid(SNTPC_PID);
		mib_get_s(MIB_NTP_ENABLED, &ntpEnabled, sizeof(ntpEnabled));
		if (pid == 0 && ntpEnabled) {
			status |= !mib_get_s(MIB_NTP_SERVER_HOST1, ntpServerHost1, sizeof(ntpServerHost1));
			status |= !mib_get_s(MIB_NTP_SERVER_HOST2, ntpServerHost2, sizeof(ntpServerHost2));
			status |= !mib_get_s(MIB_NTP_SERVER_HOST3, ntpServerHost3, sizeof(ntpServerHost3));
			status |= !mib_get_s(MIB_NTP_SERVER_HOST4, ntpServerHost4, sizeof(ntpServerHost4));
			status |= !mib_get_s(MIB_NTP_SERVER_HOST5, ntpServerHost5, sizeof(ntpServerHost5));

			status |= !mib_get_s(MIB_NTP_INTERVAL, &interval, sizeof(interval));
			if (interval == 0)
				interval = 86400;

			if (ntpServerHost5[0])
				sprintf(argv_fmt, "-s %s -s %s -s %s -s %s -s %s -i %d", ntpServerHost1, ntpServerHost2,
						ntpServerHost3, ntpServerHost4, ntpServerHost5, interval);
			else if (ntpServerHost4[0])
				sprintf(argv_fmt, "-s %s -s %s -s %s -s %s -i %d", ntpServerHost1, ntpServerHost2,
						ntpServerHost3, ntpServerHost4, interval);
			else if (ntpServerHost3[0])
				sprintf(argv_fmt, "-s %s -s %s -s %s -i %d", ntpServerHost1, ntpServerHost2,
						ntpServerHost3, interval);
			else if (ntpServerHost2[0])
				sprintf(argv_fmt, "-s %s -s %s -i %d", ntpServerHost1, ntpServerHost2, interval);
			else
				sprintf(argv_fmt, "-s %s -i %d", ntpServerHost1, interval);

#ifdef CONFIG_USER_RTK_BRIDGE_MODE
			mib_get(MIB_OP_MODE, (void *)&opmode);
			if(opmode == BRIDGE_MODE){
				DONICEDCMDARGVS(SNTPC, UNDOWAIT, argv_fmt);
			}
			else if(opmode == GATEWAY_MODE)
#endif
			{
				//get interface name by if_wan
				if_wan = check_ntp_if();

				memset(ifname, 0, sizeof(ifname));
				if (ifGetName(if_wan, ifname, IFNAMSIZ)) {
					int ifi_flags=0;
					struct in_addr inAddr;
					if(getInFlags(ifname, &ifi_flags) && (ifi_flags & IFF_UP) && (ifi_flags & IFF_RUNNING) &&
						getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 1 && inAddr.s_addr != 0x0){
						strcat(argv_fmt, " -d %s");
						DONICEDCMDARGVS(SNTPC, UNDOWAIT, argv_fmt, ifname);
					}
				} else {
					// Only run SNTPC if interface is found.
					//DONICEDCMDARGVS(SNTPC, UNDOWAIT, argv_fmt);
				}
			}

		} else {
			status = 0;
		}
		break;
	}

	return status;
}

// return value:
// 1  : successful
// -1 : failed
inline int startNTP(void)
{
	return setupNtp(SNTP_ENABLED);
}

// return value:
// 1  : successful
// -1 : failed
inline int stopNTP(void)
{
	return setupNtp(SNTP_DISABLED);
}
void restartNTP(void)
{
	if((read_pid((char *)SNTPC_PID)) >= 1)
		stopNTP();

	startNTP();
}
#else
int rtl_set_tz_to_kernel(char *tz_file)
{
	int i,j;
	int minute=0;
	int hour=0, mnt=0;
	FILE *fp=NULL;
	char line_buffer[64]={0};
	char sub_buffer[64]={0};
	char *pchar=NULL;

	struct timezone tz;
	memset(&tz, 0, sizeof(tz));

	if((fp=fopen(tz_file, "r"))!=NULL)
	{
		fgets(line_buffer, sizeof(line_buffer), fp);

		for(i=0, j=0;i<strlen(line_buffer);i++)
		{
			if(line_buffer[i]=='-' || line_buffer[i]==':' || (line_buffer[i]>='0' && line_buffer[i]<='9'))
				sub_buffer[j++]=line_buffer[i];
			else if(j>0)
				break;
		}

		if(j>0)
		{
			//printf("\n%s:%d sub_buffer=%s\n",__FUNCTION__,__LINE__,sub_buffer);
			if((pchar=strchr(sub_buffer, ':'))==NULL)
			{
				minute=atoi(sub_buffer)*60;
			}
			else
			{
				sscanf(sub_buffer, "%d:%d", &hour, &mnt);
				if(hour>0)
					minute=hour*60+mnt;
				else
					minute=hour*60-mnt;
			}

			tz.tz_minuteswest=minute;

			if(settimeofday(NULL, &tz) != 0)
				printf("settimeofday() failed: %s %d\n", __func__, __LINE__);

			//printf("\n%s:%d minute=%d\n",__FUNCTION__,__LINE__,minute);
		}
		fclose(fp);
	}
	return 0;
}

// return value:
// 1  : successful
// -1 : failed
int startNTP(void)
{
	int status=1;
	char vChar;
	char sVal[64], sTZ[16], *pStr;
	char sVal2[64]={0};
	char sVal3[64]={0};
	char sVal4[64]={0};
	char sVal5[64]={0};
	FILE *fp;
	unsigned int index = 0, ntpItf;
	unsigned char dst_enabled = 1;
	char ifname[IFNAMSIZ] = {0};
#ifdef CONFIG_TELMEX_DEV
	char msVal[5][64];
	int serverIndex;
	char *argv[15];
	int argc=1;
#endif
#ifdef CONFIG_TRUE
	unsigned int interval=0;
	char interval_str[16];
#endif

	mib_get_s( MIB_NTP_ENABLED, (void *)&vChar, sizeof(vChar));
	if (vChar == 1)
	{
		if (mib_get_s(MIB_NTP_TIMEZONE_DB_INDEX, &index, sizeof(index))) {
#if defined(__UCLIBC__) || (defined(__GLIBC__) && !defined(CONFIG_USER_GLIBC_TIMEZONE))
			if ((fp = fopen("/etc/TZ", "w")) != NULL) {
				mib_get_s(MIB_DST_ENABLED, &dst_enabled, sizeof(dst_enabled));
				fprintf(fp, "%s\n", get_tz_string(index, dst_enabled));
				fclose(fp);
			}

			//set time zone to kernel
			rtl_set_tz_to_kernel("/etc/TZ");
#else
	#ifdef CONFIG_USER_GLIBC_TIMEZONE
			char cmd[128];
			char tz_location[64]={0};

			system("rm /var/localtime");
			mib_get_s(MIB_DST_ENABLED, &dst_enabled, sizeof(dst_enabled));
			if (dst_enabled || !is_tz_dst_exist(index)) {
				snprintf(tz_location,sizeof(tz_location),"%s",get_tz_location(index, FOR_CLI));
				format_tz_location(tz_location);

				snprintf(cmd, sizeof(cmd), "ln -s /usr/share/zoneinfo/%s /var/localtime",tz_location);
			} else {
				const char * tz_offset;

				tz_offset = get_format_tz_utc_offset(index);

				snprintf(cmd, sizeof(cmd), "ln -s /usr/share/zoneinfo/Etc/GMT%s /var/localtime",tz_offset);
			}

			//fprintf(stderr,cmd);
			system(cmd);
	#endif
#endif
		}

		mib_get_s( MIB_NTP_SERVER_ID, (void *)&vChar, sizeof(vChar));
#ifndef CONFIG_TELMEX_DEV
		mib_get_s(MIB_NTP_SERVER_HOST1, (void *)sVal, sizeof(sVal));
		mib_get_s(MIB_NTP_SERVER_HOST2, (void *)sVal2, sizeof(sVal2));
		mib_get_s(MIB_NTP_SERVER_HOST3, (void *)sVal3, sizeof(sVal3));
		mib_get_s(MIB_NTP_SERVER_HOST4, (void *)sVal4, sizeof(sVal4));
		mib_get_s(MIB_NTP_SERVER_HOST5, (void *)sVal5, sizeof(sVal5));
#else
		memset(msVal, 0, sizeof(msVal));
		for(serverIndex=0; serverIndex <5; serverIndex++)
		{
			if (vChar & (1<<serverIndex))
			{
				if (serverIndex == 0)
				{
					mib_get_s(MIB_NTP_SERVER_HOST1, (void *)msVal[serverIndex], sizeof(msVal[serverIndex]));
					argv[argc++] = "-s";
					argv[argc++] = msVal[serverIndex];
				}
				else if (serverIndex == 1)
				{
					mib_get_s(MIB_NTP_SERVER_HOST2, (void *)msVal[serverIndex], sizeof(msVal[serverIndex]));
					argv[argc++] = "-s";
					argv[argc++] = msVal[serverIndex];
				}
				else if (serverIndex == 2)
				{
					mib_get_s(MIB_NTP_SERVER_HOST3, (void *)msVal[serverIndex], sizeof(msVal[serverIndex]));
					argv[argc++] = "-s";
					argv[argc++] = msVal[serverIndex];
				}
				else if (serverIndex == 3)
				{
					mib_get_s(MIB_NTP_SERVER_HOST4, (void *)msVal[serverIndex], sizeof(msVal[serverIndex]));
					argv[argc++] = "-s";
					argv[argc++] = msVal[serverIndex];
				}
				else if (serverIndex == 4)
				{
					mib_get_s(MIB_NTP_SERVER_HOST5, (void *)msVal[serverIndex], sizeof(msVal[serverIndex]));
					argv[argc++] = "-s";
					argv[argc++] = msVal[serverIndex];
				}
			}
		}
#endif
#ifdef CONFIG_TRUE
		mib_get_s(MIB_NTP_INTERVAL, (void *)&interval, sizeof(interval));
		if (interval == 0)
			interval = 86400;
		sprintf(interval_str, "%u", interval);
#endif

		if(mib_get_s(MIB_NTP_EXT_ITF, (void *)&ntpItf, sizeof(ntpItf)))
			ifGetName(ntpItf, ifname, sizeof(ifname));
#ifdef CONFIG_00R0
/*
	- By default - should be used default route (IPoE or PPPoE)
	- If ntp configured by TR-069 - should be used same as TR-069 WAN
	- If ntp configured using DHCP option 42- Should be used WAN which recieved DHCP option 42.

	This method can avoid strange ARP packet from WAN with no IP address.
*/
		MIB_CE_ATM_VC_T wan_entry;
		int pri = NTP_EXT_ITF_PRI_LOW;
		mib_get_s(MIB_NTP_EXT_ITF_PRI, (void *)&pri, sizeof(pri));

		if(ifname[0]=='\0' && pri == NTP_EXT_ITF_PRI_LOW)
		{
			if(getDefaultRouteATMVCEntry(&wan_entry) != NULL)
				ifGetName(wan_entry.ifIndex, ifname, sizeof(ifname));
		}
#endif
		int ifi_flags=0;
		struct in_addr inAddr;

/*ping_zhang:20081223 START:add to support sntp multi server*/
#ifndef	NTP_SUPPORT_V6
#ifdef SNTP_MULTI_SERVER
#ifndef CONFIG_TELMEX_DEV
		if (ifname[0]){
			if(getInFlags(ifname, &ifi_flags) && (ifi_flags & IFF_UP) && (ifi_flags & IFF_RUNNING) &&
				getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 1 && inAddr.s_addr != 0x0){
#ifdef CONFIG_TRUE
				if(sVal5[0]){
					status|=va_niced_cmd(SNTPC, 14, 0, "-s", sVal, "-s", sVal2, "-s", sVal3, "-s", sVal4,
					"-s", sVal5, "-i", interval_str, "-d", ifname);
				}
				else if(sVal4[0]){
					status|=va_niced_cmd(SNTPC, 12, 0, "-s", sVal, "-s", sVal2, "-s", sVal3, "-s", sVal4,"-i",
					 interval_str, "-d", ifname);
				}
				else if(sVal3[0]){
					status|=va_niced_cmd(SNTPC, 10, 0, "-s", sVal, "-s", sVal2, "-s", sVal3, "-i", interval_str,
					 "-d", ifname);
				}
				else if(sVal2[0]){
					status|=va_niced_cmd(SNTPC, 8, 0, "-s", sVal, "-s", sVal2, "-i", interval_str, "-d", ifname);
				}
				else{
					status|=va_niced_cmd(SNTPC, 6, 0, "-s", sVal, "-i", interval_str, "-d", ifname);
				}
#else
				if(sVal5[0]){
					status|=va_niced_cmd(SNTPC, 12, 0, "-s", sVal, "-s", sVal2, "-s", sVal3, "-s", sVal4,
					"-s", sVal5,"-d", ifname);
				}
				else if(sVal4[0]){
					status|=va_niced_cmd(SNTPC, 10, 0, "-s", sVal, "-s", sVal2, "-s", sVal3, "-s", sVal4,"-d", ifname);
				}
				else if(sVal3[0]){
					status|=va_niced_cmd(SNTPC, 8, 0, "-s", sVal, "-s", sVal2, "-s", sVal3, "-d", ifname);
				}
				else if(sVal2[0]){
					status|=va_niced_cmd(SNTPC, 6, 0, "-s", sVal, "-s", sVal2, "-d", ifname);
				}
				else{
					status|=va_niced_cmd(SNTPC, 4, 0, "-s", sVal, "-d", ifname);
				}
#endif
			}
		}
#ifndef CONFIG_00R0
		else{
			if(sVal5[0]){
				status|=va_niced_cmd(SNTPC, 10, 0, "-s", sVal, "-s", sVal2, "-s", sVal3, "-s", sVal4, "-s", sVal5);
			}
			else if(sVal4[0]){
				status|=va_niced_cmd(SNTPC, 8, 0, "-s", sVal, "-s", sVal2, "-s", sVal3, "-s", sVal4);
			}
			else if(sVal3[0]){
				status|=va_niced_cmd(SNTPC, 6, 0, "-s", sVal, "-s", sVal2, "-s", sVal3);
			}
			else if(sVal2[0]){
				status|=va_niced_cmd(SNTPC, 4, 0, "-s", sVal, "-s", sVal2);
			}
			else{
				status|=va_niced_cmd(SNTPC, 2, 0, "-s", sVal);
			}
		}
#endif
#else
		if (ifname[0])
		{
			argv[argc++] = "-d";
			argv[argc++] = ifname;
			argv[argc] = NULL;
		}
		else
		{
			argv[argc] = NULL;
		}

		do_cmd(SNTPC, argv, 0);
#endif
#else
		if (ifname[0]){
			if(getInFlags(ifname, &ifi_flags) && (ifi_flags & IFF_UP) && (ifi_flags & IFF_RUNNING) &&
				getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 1 && inAddr.s_addr != 0x0){
#ifdef CONFIG_TRUE
					status|=va_niced_cmd(SNTPC, 5, 0, sVal, "-i", interval_str, "-d", ifname);
#else
					status|=va_niced_cmd(SNTPC, 3, 0, sVal, "-d", ifname);
#endif
			}
		}
#ifndef CONFIG_00R0
		else {
#ifdef CONFIG_TRUE
			status|=va_niced_cmd(SNTPC, 3, 0, sVal, "-i", interval_str);
#else
			status|=va_niced_cmd(SNTPC, 1, 0, sVal);
#endif//end of CONFIG_TRUE
#endif//end of !CONFIG_00R0
		}
#endif
#else
	if (ifname[0])
		status|=va_niced_cmd(SNTPCV6,3,0,sVal,"-f", ifname);
	else
		status|=va_niced_cmd(SNTPCV6,1,0,sVal);
#endif

/*ping_zhang:20081223 END*/
	}
	return status;
}

// return value:
// 1  : successful
// -1 : failed
int stopNTP(void)
{
	int ntp_pid=0;
	int status=1;

#ifndef	NTP_SUPPORT_V6
	va_cmd("/bin/killall", 1, 1, "vsntp");
#else
	kill_by_pidfile_new((char*)SNTPCV6_PID , SIGTERM);
#endif
	return 1;
}
#endif //CONFIG_E8B
#endif //TIME_ZONE

#if defined(CONFIG_FON_GRE) || defined(CONFIG_XFRM)
#ifdef CONFIG_GPON_FEATURE
//cxy 2016-12-26: gpon unnumber packets need to assign tx flow id.
#if 1
int set_smux_gpon_usflow(MIB_CE_ATM_VC_Tp pentry)
{
	return -1;
}
#else // not support now on new rtk_tr142 with FC
int set_smux_gpon_usflow(MIB_CE_ATM_VC_Tp pentry)
{
	FILE*fp;
	char buff[128];
	int wanidx=-1, len;
	int usflow[WAN_PONMAC_QUEUE_MAX],gponqueue[WAN_PONMAC_QUEUE_MAX];
	int usfloworder[WAN_PONMAC_QUEUE_MAX],tmp;
	char cmd[128],ifname[MAX_NAME_LEN];

	if(fp=fopen(TR142_WAN_INFO,"r"))
	{
		int i,j;

		for(i=0; i<WAN_PONMAC_QUEUE_MAX; i++)
		{
			usflow[i] = -1;
			gponqueue[i] = -1;
		}
		while(fgets(buff,sizeof(buff),fp)!=NULL)
		{
			if(strstr(buff,"wanIdx"))
			{
				if(wanidx != -1)//another wan
					break;
				//printf("buff= %s\n",buff);
				if(sscanf(buff,"%*[^=]= %d",&wanidx)==1)
					wanidx = -1;
			}
			else if(wanidx!=-1)
			{
				//printf("<%s %d> get wan idx:%d\n",__func__,__LINE__,wanidx);
				//printf("buff= %s\n",buff);
				if(strstr(buff,"usFlowId"))
				{
					sscanf(buff,"%*s = %d %d %d %d %d %d %d %d",
						&usflow[0],&usflow[1],&usflow[2],&usflow[3],
						&usflow[4],&usflow[5],&usflow[6],&usflow[7]);
				}
				else if(strstr(buff,"queueSts"))
				{
					sscanf(buff,"%*s = %d %d %d %d %d %d %d %d",
						&gponqueue[0],&gponqueue[1],&gponqueue[2],&gponqueue[3],
						&gponqueue[4],&gponqueue[5],&gponqueue[6],&gponqueue[7]);
				}
			}
		}
		fclose(fp);
		if(wanidx == -1)
		{
			printf("<%s %d> not find valid wanidx\n",__func__,__LINE__);
			return -1;
		}
		// descending order
		for(i=0; i<WAN_PONMAC_QUEUE_MAX-1; i++)
		{
			for(j=0; j<WAN_PONMAC_QUEUE_MAX-i-1; j++)
				if(gponqueue[j] < gponqueue[j+1])
				{
					tmp = gponqueue[j+1];
					gponqueue[j+1] = gponqueue[j];
					gponqueue[j] = tmp;

					tmp = usflow[j+1];
					usflow[j+1] = usflow[j];
					usflow[j] = tmp;
				}
		}
		ifGetName(PHY_INTF(pentry->ifIndex),ifname,sizeof(ifname));
		sprintf(cmd,"ethctl setsmux %s %s usflow ",ALIASNAME_NAS0,ifname);
		for(i=0;i<WAN_PONMAC_QUEUE_MAX;i++)
		{
			len = sizeof(cmd) - strlen(cmd);
			snprintf(cmd + strlen(cmd), len, "%d,", usflow[i]);
		}
		printf("<%s %d> %s\n",__func__,__LINE__,cmd);
		system(cmd);
		return 0;
	}
	return -1;
}
#endif
#endif//CONFIG_GPON_FEATURE
#endif

#ifdef SUPPORT_FON_GRE
int createGreTunnel(char *tnlName, char *ifname, uint32 localAddr, uint32 remoteAddr, int vlan)
{
	char localStr[48];
	char remoteStr[48];

	printf("%s enter!\n", __func__);

	inet_ntop(AF_INET, &localAddr, localStr, 48);
	inet_ntop(AF_INET, &remoteAddr, remoteStr, 48);

	va_cmd("/usr/bin/ip", 13, 1, "link", "add", tnlName, "type", "gretap", "remote", remoteStr,
			"local", localStr, "ttl", "255", "dev", ifname);
	fprintf(stderr, "ip link add %s type gretap remote %s local %s ttl 255 dev %s\n", tnlName, remoteStr, localStr, ifname);

	va_cmd("/usr/bin/ip", 4, 1, "link", "set", tnlName, "up");
	fprintf(stderr, "ip link set %s up\n", tnlName);

	if (vlan != 0)
	{
		char vlanStr[10], vlanDevName[32];

		snprintf(vlanStr, 10, "%d", vlan);
		va_cmd("/bin/vconfig", 3, 1, "add", tnlName, vlanStr);
		fprintf(stderr, "vconfig add %s %d\n", tnlName, vlan);

		snprintf(vlanDevName, 32, "%s.%d", tnlName, vlan);

		va_cmd("/usr/bin/ip", 4, 1, "link", "set", vlanDevName, "up");
		fprintf(stderr, "ip link set %s up\n", vlanDevName);

		va_cmd("/bin/brctl", 3, 1, "addif", "br0", vlanDevName);
		fprintf(stderr, "brctl addif br0 %s", vlanDevName);
	}
	else
	{
		va_cmd("/bin/brctl", 3, 1, "addif", "br0", tnlName);
		fprintf(stderr, "brctl addif br0 %s", tnlName);
	}

	return 0;
}

int deleteGreTunnel(char *tnlName, int vlan)
{
	if (vlan != 0)
	{
		char vlanDevName[32];

		snprintf(vlanDevName, 32, "%s.%d", tnlName, vlan);

		va_cmd("/bin/brctl", 3, 1, "delif", "br0", vlanDevName);
		fprintf(stderr, "brctl delif br0 %s", vlanDevName);

		va_cmd("/usr/bin/ip", 4, 1, "link", "set", vlanDevName, "down");
		fprintf(stderr, "ip link set %s down\n", vlanDevName);

		va_cmd("/bin/vconfig", 2, 1, "rem", vlanDevName);
		fprintf(stderr, "vconfig rem %s\n", vlanDevName);
	}
	else
	{
		va_cmd("/bin/brctl", 3, 1, "delif", "br0", tnlName);
		fprintf(stderr, "brctl delif br0 %s", tnlName);
	}

	va_cmd("/usr/bin/ip", 4, 1, "link", "set", tnlName, "down");
	fprintf(stderr, "ip link set %s down\n", tnlName);

	va_cmd("/usr/bin/ip", 3, 1, "link", "del", tnlName);
	fprintf(stderr, "ip link del %s\n", tnlName);

	return 0;
}

int setupGreFirewall(void)
{
	unsigned char vChar;
	int ori_wlan_idx;
	char tnlName[32], markStr[32];

	snprintf(markStr, 32, "0x%x/0x%x", BYPASS_FWDENGINE_MARK_MASK, BYPASS_FWDENGINE_MARK_MASK);

	// iptables -F dhcp_queue
	va_cmd(IPTABLES, 2, 1, "-F", "dhcp_queue");
	// iptables -F FON_GRE_FORWARD
	va_cmd(IPTABLES, 2, 1, "-F", "FON_GRE_FORWARD");
	// iptables -F FON_GRE_INPUT
	va_cmd(IPTABLES, 2, 1, "-F", "FON_GRE_INPUT");
	// iptables -t mangle -F FON_GRE
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "FON_GRE");
	//ebtables -F FON_GRE
	va_cmd(EBTABLES, 2, 1, "-F", "FON_GRE");

	mib_get_s(MIB_FON_GRE_ENABLE, (void *)&vChar, sizeof(vChar));
	if (0 == vChar)
		return 0;

	/* QUEUE DHCP DISCOVERY packet for OPTION-82 process */
	ori_wlan_idx = wlan_idx;
	wlan_idx = 0;
	mib_get_s(MIB_WLAN_FREE_SSID_GRE_ENABLE, (void *)&vChar, sizeof(vChar));
	if (vChar)
	{
		//iptables -A FON_GRE_FORWARD -i wlan0-vap0 -p tcp --syn -j TCPMSS --set-mss 1418
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, "FON_GRE_FORWARD", (char *)ARG_I, "wlan0-vap0", "-p", "tcp",
				"--syn", "-j", "TCPMSS", "--set-mss", "1418");

		//iptables -t mangle -A FON_GRE -i wlan0-vap0 -j MARK --set-mark 0x700000/0x700000
		va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, "FON_GRE", (char *)ARG_I, "wlan0-vap0", "-j", "MARK",
				"--set-mark", markStr);

		//iptables -A dhcp_queue -i wlan0-vap0 -p udp --dport 67 -j NFQUEUE --queue-num 0
		va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "dhcp_queue", (char *)ARG_I,
					"wlan0-vap0", "-p", "udp", (char *)FW_DPORT,
					(char *)PORT_DHCP, "-j", "NFQUEUE", "--queue-num", "0");

		//iptables -A FON_GRE_INPUT -m physdev --physdev-in wlan0-vap0 -p udp --dport 67 -j DROP
		va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "FON_GRE_INPUT", "-m","physdev","--physdev-in","wlan0-vap0", "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);

		// iptables -A FON_GRE_INPUT -i wlan0-vap0 -p udp --dport 67 -j DROP
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, "FON_GRE_INPUT", (char *)ARG_I, "wlan0-vap0", "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);

		mib_get_s(MIB_WLAN_FREE_SSID_GRE_NAME, (void *)tnlName, sizeof(tnlName));
		snprintf(tnlName, 32, "%s+", tnlName);

		//ebtables -A FON_GRE -i wlan0-vap0 -o tnlName+ -j ACCEPT
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, "FON_GRE", "-i", "wlan0-vap0", "-o", tnlName, "-j", "ACCEPT");

		//ebtables -A FON_GRE -i tnlName+ -o wlan0-vap0 -j ACCEPT
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, "FON_GRE", "-i", tnlName, "-o", "wlan0-vap0", "-j", "ACCEPT");

		//default rule to drop any packet from wlan0-vap0 or tunnel dev
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", "wlan0-vap0", "-j", "DROP");
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", tnlName, "-j", "DROP");
	}
	mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_ENABLE, (void *)&vChar, sizeof(vChar));
	if (vChar)
	{
		//iptables -A FON_GRE_FORWARD -i wlan0-vap1 -p tcp --syn -j TCPMSS --set-mss 1418
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, "FON_GRE_FORWARD", (char *)ARG_I, "wlan0-vap1", "-p", "tcp",
				"--syn", "-j", "TCPMSS", "--set-mss", "1418");

		//iptables -t mangle -A FON_GRE -i wlan0-vap1 -j MARK --set-mark 0x700000/0x700000
		va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, "FON_GRE", (char *)ARG_I, "wlan0-vap1", "-j", "MARK",
				"--set-mark", markStr);

		//iptables -A dhcp_queue -i wlan0-vap1 -p udp --dport 67 -j NFQUEUE --queue-num 0
		va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "dhcp_queue", (char *)ARG_I,
					"wlan0-vap1", "-p", "udp", (char *)FW_DPORT,
					(char *)PORT_DHCP, "-j", "NFQUEUE", "--queue-num", "0");

		//iptables -A FON_GRE_INPUT -m physdev --physdev-in wlan0-vap1 -p udp --dport 67 -j DROP
		va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "FON_GRE_INPUT", "-m","physdev","--physdev-in","wlan0-vap1", "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);

		// iptables -A FON_GRE_INPUT -i wlan0-vap1 -p udp --dport 67 -j DROP
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, "FON_GRE_INPUT", (char *)ARG_I, "wlan0-vap1", "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);

		mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_NAME, (void *)tnlName, sizeof(tnlName));
		snprintf(tnlName, 32, "%s+", tnlName);

		//ebtables -A FON_GRE -i wlan0-vap1 -o tnlName+ -j ACCEPT
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, "FON_GRE", "-i", "wlan0-vap1", "-o", tnlName, "-j", "ACCEPT");

		//ebtables -A FON_GRE -i tnlName+ -o wlan0-vap1 -j ACCEPT
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, "FON_GRE", "-i", tnlName, "-o", "wlan0-vap1", "-j", "ACCEPT");

		//default rule to drop any packet from wlan0-vap1 or tunnel dev
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", "wlan0-vap1", "-j", "DROP");
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", tnlName, "-j", "DROP");
	}

	wlan_idx = 1;
	mib_get_s(MIB_WLAN_FREE_SSID_GRE_ENABLE, (void *)&vChar, sizeof(vChar));
	if (vChar)
	{
		//iptables -A FON_GRE_FORWARD -i wlan1-vap0 -p tcp --syn -j TCPMSS --set-mss 1418
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, "FON_GRE_FORWARD", (char *)ARG_I, "wlan1-vap0", "-p", "tcp",
				"--syn", "-j", "TCPMSS", "--set-mss", "1418");

		//iptables -t mangle -A FON_GRE -i wlan1-vap0 -j MARK --set-mark 0x700000/0x700000
		va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, "FON_GRE", (char *)ARG_I, "wlan1-vap0", "-j", "MARK",
				"--set-mark", markStr);

		//iptables -A dhcp_queue -i wlan1-vap0 -p udp --dport 67 -j NFQUEUE --queue-num 0
		va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "dhcp_queue", (char *)ARG_I,
					"wlan1-vap0", "-p", "udp", (char *)FW_DPORT,
					(char *)PORT_DHCP, "-j", "NFQUEUE", "--queue-num", "0");

		//iptables -A FON_GRE_INPUT -m physdev --physdev-in wlan1-vap0 -p udp --dport 67 -j DROP
		va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "FON_GRE_INPUT", "-m","physdev","--physdev-in","wlan1-vap0", "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);

		// iptables -A FON_GRE_INPUT -i wlan1-vap0 -p udp --dport 67 -j DROP
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, "FON_GRE_INPUT", (char *)ARG_I, "wlan1-vap0", "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);

		mib_get_s(MIB_WLAN_FREE_SSID_GRE_NAME, (void *)tnlName, sizeof(tnlName));
		snprintf(tnlName, 32, "%s+", tnlName);

		//ebtables -A FON_GRE -i wlan1-vap0 -o tnlName+ -j ACCEPT
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, "FON_GRE", "-i", "wlan1-vap0", "-o", tnlName, "-j", "ACCEPT");

		//ebtables -A FON_GRE -i tnlName+ -o wlan1-vap0 -j ACCEPT
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, "FON_GRE", "-i", tnlName, "-o", "wlan1-vap0", "-j", "ACCEPT");

		//default rule to drop any packet from wlan1-vap0 or tunnel dev
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", "wlan1-vap0", "-j", "DROP");
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", tnlName, "-j", "DROP");
	}
	mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_ENABLE, (void *)&vChar, sizeof(vChar));
	if (vChar)
	{
		//iptables -A FON_GRE_FORWARD -i wlan1-vap1 -p tcp --syn -j TCPMSS --set-mss 1418
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, "FON_GRE_FORWARD", (char *)ARG_I, "wlan1-vap1", "-p", "tcp",
				"--syn", "-j", "TCPMSS", "--set-mss", "1418");

		//iptables -t mangle -A FON_GRE -i wlan1-vap1 -j MARK --set-mark 0x700000/0x700000
		va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, "FON_GRE", (char *)ARG_I, "wlan1-vap1", "-j", "MARK",
				"--set-mark", markStr);

		//iptables -A dhcp_queue -i wlan1-vap1 -p udp --dport 67 -j NFQUEUE --queue-num 0
		va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "dhcp_queue", (char *)ARG_I,
					"wlan1-vap1", "-p", "udp", (char *)FW_DPORT,
					(char *)PORT_DHCP, "-j", "NFQUEUE", "--queue-num", "0");

		//iptables -A FON_GRE_INPUT -m physdev --physdev-in wlan1-vap1 -p udp --dport 67 -j DROP
		va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "FON_GRE_INPUT", "-m","physdev","--physdev-in","wlan1-vap1", "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);

		// iptables -A FON_GRE_INPUT -i wlan1-vap1 -p udp --dport 67 -j DROP
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, "FON_GRE_INPUT", (char *)ARG_I, "wlan1-vap1", "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);

		mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_NAME, (void *)tnlName, sizeof(tnlName));
		snprintf(tnlName, 32, "%s+", tnlName);

		//ebtables -A FON_GRE -i wlan1-vap1 -o tnlName+ -j ACCEPT
		va_cmd(EBTABLES, 6, 1, (char *)FW_INSERT, "FON_GRE", "-i", "wlan1-vap1", "-o", tnlName, "-j", "ACCEPT");

		//ebtables -A FON_GRE -i tnlName+ -o wlan1-vap1 -j ACCEPT
		va_cmd(EBTABLES, 6, 1, (char *)FW_INSERT, "FON_GRE", "-i", tnlName, "-o", "wlan1-vap1", "-j", "ACCEPT");

		//default rule to drop any packet from wlan1-vap1 or tunnel dev
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", "wlan1-vap1", "-j", "DROP");
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", tnlName, "-j", "DROP");
	}
	wlan_idx = ori_wlan_idx;

	return 0;
}

int do_FonGRE(char *ifname, int msg_type)
{
	MIB_CE_ATM_VC_T Entry;
	int totalEntry=0, i;
	int ori_wlan_idx;
	char this_ifname[IFNAMSIZ];
	char tnlName[32];
	struct in_addr localAddr, remoteAddr;
	int vlan;
	unsigned char vChar;
#ifdef CONFIG_GPON_FEATURE
	int pon_mode=0;
	int greTnlCreated = 0;
#endif

	mib_get_s(MIB_FON_GRE_ENABLE, (void *)&vChar, sizeof(vChar));
	if (0 == vChar)
		return 0;

	if (msg_type == RTM_NEWADDR)
	{
		totalEntry = mib_chain_total(MIB_ATM_VC_TBL); /* get chain record size */
		for (i=0; i<totalEntry; i++)
		{
			mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry); /* get the specified chain record */
			ifGetName(Entry.ifIndex, this_ifname, sizeof(this_ifname));

			if (strcmp(this_ifname, ifname) == 0)
			{
				if (Entry.itfGroup & (1<<PMAP_WLAN0_VAP0))
				{
					//check if gre free ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 0;

					mib_get_s(MIB_WLAN_FREE_SSID_GRE_ENABLE, (void *)&vChar, sizeof(vChar));
					if (1 == vChar)
					{
						mib_get_s(MIB_WLAN_FREE_SSID_GRE_NAME, (void *)tnlName, sizeof(tnlName));

						/* check if tunnel is already created to avoid repetitive operation */
						if (!getInFlags((char *)tnlName, 0))
						{
							mib_get_s(MIB_WLAN_FREE_SSID_GRE_SERVER, (void *)&remoteAddr, sizeof(remoteAddr));
							mib_get_s(MIB_WLAN_FREE_SSID_GRE_VLAN, (void *)&vlan, sizeof(vlan));

							if (getInAddr( ifname, IP_ADDR, (void *)&localAddr) == 1)
							{
								deleteGreTunnel(tnlName, vlan);
								createGreTunnel(tnlName, ifname, localAddr.s_addr, remoteAddr.s_addr, vlan);
#ifdef CONFIG_GPON_FEATURE
								greTnlCreated = 1;
#endif
							}
						}
					}

					wlan_idx = ori_wlan_idx;
				}
				if (Entry.itfGroup & (1<<PMAP_WLAN1_VAP0))
				{
					//check if gre free ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 1;

					mib_get_s(MIB_WLAN_FREE_SSID_GRE_ENABLE, (void *)&vChar, sizeof(vChar));
					if (1 == vChar)
					{
						mib_get_s(MIB_WLAN_FREE_SSID_GRE_NAME, (void *)tnlName, sizeof(tnlName));

						/* check if tunnel is already created to avoid repetitive operation */
						if (!getInFlags((char *)tnlName, 0))
						{
							mib_get_s(MIB_WLAN_FREE_SSID_GRE_SERVER, (void *)&remoteAddr, sizeof(remoteAddr));
							mib_get_s(MIB_WLAN_FREE_SSID_GRE_VLAN, (void *)&vlan, sizeof(vlan));

							if (getInAddr( ifname, IP_ADDR, (void *)&localAddr) == 1)
							{
								deleteGreTunnel(tnlName, vlan);
								createGreTunnel(tnlName, ifname, localAddr.s_addr, remoteAddr.s_addr, vlan);
#ifdef CONFIG_GPON_FEATURE
								greTnlCreated = 1;
#endif
							}
						}
					}

					wlan_idx = ori_wlan_idx;
				}
				if (Entry.itfGroup & (1<<PMAP_WLAN0_VAP1))
				{
					//check if gre closed ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 0;

					mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_ENABLE, (void *)&vChar, sizeof(vChar));
					if (1 == vChar)
					{
						mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_NAME, (void *)tnlName, sizeof(tnlName));

						/* check if tunnel is already created to avoid repetitive operation */
						if (!getInFlags((char *)tnlName, 0))
						{
							mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_SERVER, (void *)&remoteAddr, sizeof(remoteAddr));
							mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_VLAN, (void *)&vlan, sizeof(vlan));

							if (getInAddr( ifname, IP_ADDR, (void *)&localAddr) == 1)
							{
								deleteGreTunnel(tnlName, vlan);
								createGreTunnel(tnlName, ifname, localAddr.s_addr, remoteAddr.s_addr, vlan);
#ifdef CONFIG_GPON_FEATURE
								greTnlCreated = 1;
#endif
							}
						}
					}

					wlan_idx = ori_wlan_idx;
				}
				if (Entry.itfGroup & (1<<PMAP_WLAN1_VAP1))
				{
					//check if gre closed ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 1;

					mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_ENABLE, (void *)&vChar, sizeof(vChar));
					if (1 == vChar)
					{
						mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_NAME, (void *)tnlName, sizeof(tnlName));

						/* check if tunnel is already created to avoid repetitive operation */
						if (!getInFlags((char *)tnlName, 0))
						{
							mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_SERVER, (void *)&remoteAddr, sizeof(remoteAddr));
							mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_VLAN, (void *)&vlan, sizeof(vlan));

							if (getInAddr( ifname, IP_ADDR, (void *)&localAddr) == 1)
							{
								deleteGreTunnel(tnlName, vlan);
								createGreTunnel(tnlName, ifname, localAddr.s_addr, remoteAddr.s_addr, vlan);
#ifdef CONFIG_GPON_FEATURE
								greTnlCreated = 1;
#endif
							}
						}
					}

					wlan_idx = ori_wlan_idx;
				}
#ifdef CONFIG_RTK_RG_INIT
				FlushRTK_RG_ACL_FON_GRE_RULE();
				AddRTK_RG_ACL_FON_GRE_RULE();
#endif

				setupGreFirewall();

#ifdef CONFIG_GPON_FEATURE
				if (greTnlCreated)
				{
					mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
					if (pon_mode == GPON_MODE)
					{
						set_smux_gpon_usflow(&Entry);
					}
				}
#endif

				break;
			}
		}
	}
	else if (msg_type == RTM_DELADDR)
	{
		totalEntry = mib_chain_total(MIB_ATM_VC_TBL); /* get chain record size */
		for (i=0; i<totalEntry; i++)
		{
			mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry); /* get the specified chain record */
			ifGetName(Entry.ifIndex, this_ifname, sizeof(this_ifname));

			if (strcmp(this_ifname, ifname) == 0)
			{
				if (Entry.itfGroup & (1<<PMAP_WLAN0_VAP0))
				{
					//check if gre free ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 0;

					mib_get_s(MIB_WLAN_FREE_SSID_GRE_ENABLE, (void *)&vChar, sizeof(vChar));
					if (1 == vChar)
					{
						mib_get_s(MIB_WLAN_FREE_SSID_GRE_NAME, (void *)tnlName, sizeof(tnlName));

						/* check if tunnel is already created to avoid repetitive operation */
						if (getInFlags((char *)tnlName, 0))
						{
							mib_get_s(MIB_WLAN_FREE_SSID_GRE_VLAN, (void *)&vlan, sizeof(vlan));

							deleteGreTunnel(tnlName, vlan);
						}
					}

					wlan_idx = ori_wlan_idx;
				}
				if (Entry.itfGroup & (1<<PMAP_WLAN1_VAP0))
				{
					//check if gre free ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 1;

					mib_get_s(MIB_WLAN_FREE_SSID_GRE_ENABLE, (void *)&vChar, sizeof(vChar));
					if (1 == vChar)
					{
						mib_get_s(MIB_WLAN_FREE_SSID_GRE_NAME, (void *)tnlName, sizeof(tnlName));

						/* check if tunnel is already created to avoid repetitive operation */
						if (getInFlags((char *)tnlName, 0))
						{
							mib_get_s(MIB_WLAN_FREE_SSID_GRE_VLAN, (void *)&vlan, sizeof(vlan));

							deleteGreTunnel(tnlName, vlan);
						}
					}

					wlan_idx = ori_wlan_idx;
				}
				if (Entry.itfGroup & (1<<PMAP_WLAN0_VAP1))
				{
					//check if gre closed ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 0;

					mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_ENABLE, (void *)&vChar, sizeof(vChar));
					if (1 == vChar)
					{
						mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_NAME, (void *)tnlName, sizeof(tnlName));

						/* check if tunnel is already created to avoid repetitive operation */
						if (getInFlags((char *)tnlName, 0))
						{
							mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_VLAN, (void *)&vlan, sizeof(vlan));

							deleteGreTunnel(tnlName, vlan);
						}
					}

					wlan_idx = ori_wlan_idx;
				}
				if (Entry.itfGroup & (1<<PMAP_WLAN1_VAP1))
				{
					//check if gre closed ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 1;

					mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_ENABLE, (void *)&vChar, sizeof(vChar));
					if (1 == vChar)
					{
						mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_NAME, (void *)tnlName, sizeof(tnlName));

						/* check if tunnel is already created to avoid repetitive operation */
						if (getInFlags((char *)tnlName, 0))
						{
							mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_VLAN, (void *)&vlan, sizeof(vlan));

							deleteGreTunnel(tnlName, vlan);
						}
					}

					wlan_idx = ori_wlan_idx;
				}
#ifdef CONFIG_RTK_RG_INIT
				FlushRTK_RG_ACL_FON_GRE_RULE();
				AddRTK_RG_ACL_FON_GRE_RULE();
#endif

				setupGreFirewall();

				break;
			}
		}
	}

	return 0;
}
#endif//SUPPORT_FON_GRE

#ifdef CONFIG_XFRM
#ifdef CONFIG_GPON_FEATURE
static void _routeInternetWanCheck(void)
{
	int num_wan, wan_idx, interface_flag;
	MIB_CE_ATM_VC_T entry;
	struct in_addr inAddr;
	char ifname[64];

	printf("[%s %d]\n",__func__, __LINE__);
	num_wan = mib_chain_total(MIB_ATM_VC_TBL);
	for (wan_idx = 0; wan_idx < num_wan; wan_idx++)
	{
		if (mib_chain_get(MIB_ATM_VC_TBL, wan_idx, &entry) == 0)
			continue;

		if ((entry.cmode == CHANNEL_MODE_BRIDGE) || !(entry.applicationtype & X_CT_SRV_INTERNET))
			continue;

		ifGetName(entry.ifIndex, ifname, sizeof(ifname));
		if (!getInFlags(ifname, &interface_flag))
			continue;
		if ((interface_flag & IFF_UP) && getInAddr(ifname, IP_ADDR, &inAddr))
		{
			set_smux_gpon_usflow(&entry);
			//return wan_idx;
		}
	}
}
#endif
#endif

#ifdef CONFIG_XFRM
#ifdef CONFIG_USER_STRONGSWAN
#define IPSEC_SWAN_CONTENT_LEN 4095
#define IPSEC_SWAN_CONN_NAME "conn"
#define IPSEC_SWAN_CHILD_NAME "child"

unsigned int rtk_ipsec_find_max_ipsec_instnum(void)
{
	unsigned int ret=0, i,num;
	MIB_IPSEC_SWAN_T *p,entity;

	num = mib_chain_total(MIB_IPSEC_SWAN_TBL);
	for( i=0; i<num;i++ )
	{
		p = &entity;
		if( !mib_chain_get(MIB_IPSEC_SWAN_TBL, i, (void*)p ))
			continue;
		if( p->InstNum > ret )
			ret = p->InstNum;
	}
	return ret;
}

int rtk_ipsec_get_ipsec_entry_by_instnum( unsigned int instnum, MIB_IPSEC_SWAN_T *p, unsigned int *id )
{
	int ret=-1;
	unsigned int i,num;

	if( (instnum==0) || (p==NULL) || (id==NULL) )
		return ret;

	num = mib_chain_total(MIB_IPSEC_SWAN_TBL);
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get(MIB_IPSEC_SWAN_TBL, i, (void*)p ) )
			continue;

		if( p->InstNum==instnum )
		{
			*id = i;
			ret = 0;
			break;
		}
	}
	return ret;
}

static int checkIpsecStrongSwanParameters(int entryIndex, MIB_IPSEC_SWAN_T *pEntry)
{
	if (pEntry->IPSecEncapsulationMode == IPSEC_ENCAPSULATION_MODE_TUNNEL)
	{
		if(pEntry->RemoteSubnet[0] == '\0'){
			printf("%s, entry %d, RemoteSubnet is null, do nothing\n", __FUNCTION__, entryIndex);
			goto err;
		}

		if(pEntry->LocalSubnet[0] == '\0'){
			printf("%s, entry %d, LocalSubnet is null, do nothing\n", __FUNCTION__, entryIndex);
			goto err;
		}
	}

	if(pEntry->IKEAuthenticationMethod == IKE_AUTH_METHOD_PSK){
		if(pEntry->IKEPreshareKey[0] == '\0'){
			printf("%s, entry %d, IKEAuthenticationMethod is PSK ,but IKEPreshareKey is null, do nothing\n", __FUNCTION__, entryIndex);
			goto err;
		}
	}

	return 0;

err:
	return -1;
}

int generateStrongSwanAlgorithm(char *result, unsigned char encryption, unsigned char integrity, unsigned char dhgroup)
{
	if(NULL == result)
	{
		return 0;
	}

	const char* integritySet[] = {NULL, "md5", "sha1", NULL};
	const char* encryptionSet[] = {NULL, "des", "3des", "aes128", "aes192", "aes256", "aes128gcm12", "aes192gcm12", "aes256gcm12", NULL};
	const char* dhgroupSet[] = {NULL, "modp768", "modp1024", "modp1536", "modp2048", NULL};

	if(encryptionSet[encryption])
	{
		strcat(result, encryptionSet[encryption]);
	}

	if(encryption < 6){ //aes128gcm12, aes192gcm12, aes256gcm12 do not need integrity
		if(integritySet[integrity])
		{
			strcat(result, "-");
			strcat(result, integritySet[integrity]);
		}
	}

	if(dhgroupSet[dhgroup])
	{
		strcat(result, "-");
		strcat(result, dhgroupSet[dhgroup]);
	}

	return 1;
}
static int applyStrongSwan(void)
{
	FILE *fpStrongSwan;

	if ((fpStrongSwan = fopen(STRONGSWAN_CONF, "w+"))== NULL){
		printf("ERROR: open file %s error!\n", STRONGSWAN_CONF);
		return 0;
	}

	fprintf(fpStrongSwan, "swanctl {\n");
	fprintf(fpStrongSwan, "  load = pem pkcs1 x509 revocation constraints pubkey openssl random\n");
	fprintf(fpStrongSwan, "}\n\n");

	fprintf(fpStrongSwan, "charon-systemd {\n");
	fprintf(fpStrongSwan, "  load = random nonce aes des sha1 sha2 md5 hmac pem pkcs1 x509 revocation curve25519 gmp curl kernel-netlink socket-default updown vici\n");
	fprintf(fpStrongSwan, "}\n");

	fprintf(fpStrongSwan, "charon {\n");
	fprintf(fpStrongSwan, "  i_dont_care_about_security_and_use_aggressive_mode_psk = yes\n");
	fprintf(fpStrongSwan, "  start-scripts {\n");
	fprintf(fpStrongSwan, "    load-all = /bin/swanctl --load-all\n");
	fprintf(fpStrongSwan, "  }\n");
	//add transmission for dpd retransmition
	fprintf(fpStrongSwan, "  retransmit_tries = 3\n");
	fprintf(fpStrongSwan, "  retransmit_timeout = 1.0\n");
	fprintf(fpStrongSwan, "  retransmit_base = 1\n");

	fprintf(fpStrongSwan, "}\n");

	fclose(fpStrongSwan);

	return 0;
}

void getAllWANIP(MIB_IPSEC_SWAN_T *pEntry, char *wanIPAll)
{
	int flags, flags_found;
	struct in_addr inAddr;
	int i, entryNum;
	MIB_CE_ATM_VC_T entry;
	char ifname[IFNAMSIZ];
	char *wanIP=NULL;
	char tmpBuf[100] = {0};
	char tmp_ip[128] = {0};
#ifdef CONFIG_IPV6
	struct ipv6_ifaddr ipv6_addr = {0};
	struct in6_addr ip6Addr_start;
#endif

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < entryNum; i++){
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry)){
			continue;
		}

		if (entry.cmode == CHANNEL_MODE_BRIDGE)
			continue;

		ifGetName(entry.ifIndex, ifname, sizeof(ifname));
#ifdef CONFIG_IPV6
		if (!inet_pton(PF_INET6, pEntry->RemoteIP, &ip6Addr_start)) // config IPv4
		{
#endif
			flags_found = getInFlags(ifname, &flags);
			if (flags_found && getInAddr(ifname, IP_ADDR, &inAddr) == 1) {
				wanIP = inet_ntoa(inAddr);
			}
#ifdef CONFIG_IPV6
		}
		else {	// config IPv6
			getifip6(ifname, IPV6_ADDR_UNICAST, &ipv6_addr, 1);
			inet_ntop(AF_INET6, &ipv6_addr.addr, tmpBuf, INET6_ADDRSTRLEN);
			wanIP = tmpBuf;
		}
#endif
		if (pEntry->encapl2tp) {
			if (wanIP) snprintf(tmp_ip, sizeof(tmp_ip), "%s[udp/1701],", wanIP);
		}
		else {
			if (wanIP) snprintf(tmp_ip, sizeof(tmp_ip), "%s,", wanIP);
		}
		if (wanIP) strcat(wanIPAll, tmp_ip);
	}
	wanIPAll[strlen(wanIPAll) - 1] = '\0';

	return;
}

static int setStrongswanConns(int entryIndex, MIB_IPSEC_SWAN_T *pEntry, FILE *fpSwanCtl)
{
	int flags, flags_found = 0;
	struct in_addr inAddr;
	char ifname[IFNAMSIZ];
	char *wanIP=NULL;
	char tmpBuf[100] = {0};
	char wanIPAll[124] = {0};
	const char* exchangeMode[] = {"no", "yes", NULL};
	const char* ikeAuthMethod[] = {"psk", "pubkey", NULL};
	const char* ahAlgorithmStr[] = {"md5", "sha1"};
	char espAlgorithmStr[64];
	char ikeAlgorithmStr[64];
	char cmdStr[1024] = {0};

#ifdef CONFIG_IPV6
	struct ipv6_ifaddr ipv6_addr = {0};
	struct in6_addr ip6Addr_start;
#endif

	//get WAN IP
	if (pEntry->IPSecOutInterface != DUMMY_IFINDEX) {
		ifGetName(pEntry->IPSecOutInterface, ifname, sizeof(ifname));
		snprintf(cmdStr, 1024, "/bin/echo 1 > /sys/class/net/%s/queues/rx-0/rps_cpus 2>&1", ifname);
		va_cmd("/bin/sh", 2, 1, "-c", cmdStr);
#ifdef CONFIG_IPV6
		if (!inet_pton(PF_INET6, pEntry->RemoteIP, &ip6Addr_start)) // config IPv4
		{
#endif
			flags_found = getInFlags(ifname, &flags);
			if (flags_found && getInAddr(ifname, IP_ADDR, &inAddr) == 1) {
				wanIP = inet_ntoa(inAddr);
			}
#ifdef CONFIG_IPV6
		}
		else {	// config IPv6
			getifip6(ifname, IPV6_ADDR_UNICAST, &ipv6_addr, 1);
			inet_ntop(AF_INET6, &ipv6_addr.addr, tmpBuf, INET6_ADDRSTRLEN);
			wanIP = tmpBuf;
		}
#endif
		if (wanIP == NULL) return 0;
	}
	else {
		getAllWANIP(pEntry, wanIPAll);
		if (wanIPAll[0] == 0) return 0;
	}

	//get ESP algorithm string
	if(pEntry->IPSecTransform == IPSEC_TRANSFORM_ESP)
	{
		memset(espAlgorithmStr, 0, sizeof(espAlgorithmStr));
		generateStrongSwanAlgorithm(espAlgorithmStr, pEntry->ESPEncryptionAlgorithm, pEntry->ESPAuthenticationAlgorithm, pEntry->IPSecPFS);
	}

	//get IKE algorithm string
	memset(ikeAlgorithmStr, 0, sizeof(ikeAlgorithmStr));
	generateStrongSwanAlgorithm(ikeAlgorithmStr, pEntry->IKEEncryptionAlgorithm, pEntry->IKEAuthenticationAlgorithm, pEntry->IKEDHGroup);

	if (wanIP && pEntry->IPSecOutInterface != DUMMY_IFINDEX) printf("%s, ifname is %s, flags_found is 0x%x, local tunnel IP is %s\n",__FUNCTION__, ifname, flags_found, wanIP);

	//create connections
	fprintf(fpSwanCtl, "   %s%d {\n", IPSEC_SWAN_CONN_NAME, entryIndex);
	if (pEntry->IPSecOutInterface != DUMMY_IFINDEX) {
		if (wanIP) fprintf(fpSwanCtl, "      local_addrs  = %s\n", wanIP);
	if(pEntry->IPSecType == IPSEC_TYPE_SITE2SITE){
			if (pEntry->RemoteIP[0]) fprintf(fpSwanCtl, "      remote_addrs = %s\n", pEntry->RemoteIP);
		}
	}
	fprintf(fpSwanCtl, "      aggressive = %s\n", exchangeMode[pEntry->ExchangeMode - 1]);
	fprintf(fpSwanCtl, "      local {\n");
	fprintf(fpSwanCtl, "         auth = %s\n", ikeAuthMethod[pEntry->IKEAuthenticationMethod - 1]);
	if(pEntry->IKEAuthenticationMethod == IKE_AUTH_METHOD_RSA)
	{
		system("cp -f /var/config/ca.pem /var/swanctl/private");
		system("cp -f /var/config/ca.cert.pem /var/swanctl/x509ca");

		//first generate local.priv.pem,then use local.priv.pem to generate local.cacert.pem
		system("rm -f /var/swanctl/private/local.priv.pem");
		system("rm -f /var/swanctl/x509/local.cacert.pem");
		system("ipsec pki --gen --outform pem > /var/swanctl/private/local.priv.pem");
		sprintf(cmdStr,"ipsec pki --pub --in /var/swanctl/private/local.priv.pem | ipsec pki --issue --cacert /var/config/ca.cert.pem --cakey /var/config/ca.pem --dn \"C=%s, O=%s, CN=client\" --outform pem > /var/swanctl/x509/local.cacert.pem", pEntry->Country, pEntry->Organization);
		system(cmdStr);

		fprintf(fpSwanCtl, "         certs = local.cacert.pem\n");
	}
	if(pEntry->IKEIDType == IKE_ID_TYPE_NAME)
	{
		fprintf(fpSwanCtl, "         id = %s\n", pEntry->IKELocalName);
	}
	fprintf(fpSwanCtl, "      }\n");
	fprintf(fpSwanCtl, "      remote {\n");
	fprintf(fpSwanCtl, "         auth = %s\n", ikeAuthMethod[pEntry->IKEAuthenticationMethod - 1]);
	if((pEntry->IPSecType == IPSEC_TYPE_SITE2SITE) && (pEntry->IKEIDType == IKE_ID_TYPE_NAME))
	{
		fprintf(fpSwanCtl, "	     id = %s\n", pEntry->IKERemoteName);
	}
	fprintf(fpSwanCtl, "      }\n");
	fprintf(fpSwanCtl, "      children {\n");
	fprintf(fpSwanCtl, "         %s_%d {\n", IPSEC_SWAN_CHILD_NAME, entryIndex);
	if (pEntry->IPSecEncapsulationMode == IPSEC_ENCAPSULATION_MODE_TUNNEL)
	{
		fprintf(fpSwanCtl, "           local_ts  = %s\n", pEntry->LocalSubnet);
		if (pEntry->IPSecOutInterface != DUMMY_IFINDEX) fprintf(fpSwanCtl, "           remote_ts = %s\n\n", pEntry->RemoteSubnet);
	}
	else if (pEntry->IPSecEncapsulationMode == IPSEC_ENCAPSULATION_MODE_TRANSPORT) {
		if (pEntry->encapl2tp) {
			if (pEntry->IPSecOutInterface != DUMMY_IFINDEX){
				if (wanIP) fprintf(fpSwanCtl, "           local_ts  = %s[udp/1701]\n", wanIP);
			}
			else {
				if (wanIPAll[0]) fprintf(fpSwanCtl, "           local_ts  = %s\n", wanIPAll);
			}
			if (pEntry->RemoteIP[0] && pEntry->IPSecOutInterface != DUMMY_IFINDEX)	fprintf(fpSwanCtl, "           remote_ts = %s[udp/1701]\n\n", pEntry->RemoteIP);
		}
		else {
			if (pEntry->IPSecOutInterface != DUMMY_IFINDEX) {
				if (wanIP) fprintf(fpSwanCtl, "           local_ts  = %s\n", wanIP);
			}
			else {
				if (wanIPAll[0]) fprintf(fpSwanCtl, "           local_ts  = %s\n", wanIPAll);
			}
			if (pEntry->RemoteIP[0] && pEntry->IPSecOutInterface != DUMMY_IFINDEX)	fprintf(fpSwanCtl, "           remote_ts = %s\n\n", pEntry->RemoteIP);
		}
	}

	fprintf(fpSwanCtl, "           updown = /usr/local/libexec/ipsec/_updown\n");
	fprintf(fpSwanCtl, "           rekey_time = %d\n", pEntry->IPSecSATimePeriod);
	//fprintf(fpSwanCtl, "           rekey_bytes = %d\n", pEntry->IPSecSATrafficPeriod*1024);
	//fprintf(fpSwanCtl, "           rekey_packets = 1000000\n");
	if(pEntry->IPSecTransform == IPSEC_TRANSFORM_AH)
	{
		fprintf(fpSwanCtl, "           ah_proposals = %s\n", ahAlgorithmStr[pEntry->AHAuthenticationAlgorithm - 1]);
	}
	else
	{
		fprintf(fpSwanCtl, "           esp_proposals = %s\n", espAlgorithmStr);
	}
	fprintf(fpSwanCtl, "           start_action = trap\n");  //which triggers the tunnel as soon as matching traffic has been detected
	if (pEntry->IPSecEncapsulationMode == IPSEC_ENCAPSULATION_MODE_TRANSPORT)
		fprintf(fpSwanCtl, "           mode = transport\n");
	if(pEntry->DPDEnable)
		fprintf(fpSwanCtl, "           dpd_action = restart\n");
	fprintf(fpSwanCtl, "         }\n");
	fprintf(fpSwanCtl, "      }\n");
	fprintf(fpSwanCtl, "      version = %d\n", pEntry->IKEVersion);
	fprintf(fpSwanCtl, "      mobike = no\n");
	fprintf(fpSwanCtl, "      reauth_time = %d\n", pEntry->IKESAPeriod);
	fprintf(fpSwanCtl, "      proposals = %s\n", ikeAlgorithmStr);
	//add parameters for dpd
	if(pEntry->DPDEnable){
		fprintf(fpSwanCtl, "      dpd_delay = %u\n", pEntry->DPDThreshold);
		fprintf(fpSwanCtl, "      dpd_timeout = %u\n", pEntry->DPDRetry);
	}
	fprintf(fpSwanCtl, "   }\n\n");

	return 1;
}

static int setStrongswanSecrets(int entryIndex, MIB_IPSEC_SWAN_T *pEntry, char *secrets)
{
	int offset = strlen(secrets);
	int needId = 0;
	char secretIdName[IPSEC_NAME_LEN] = {0};
	if(pEntry->IKEAuthenticationMethod == IKE_AUTH_METHOD_PSK)
	{
		//create secrets
		offset += snprintf(secrets+offset, IPSEC_SWAN_CONTENT_LEN-offset, "   ike-%d {\n", entryIndex);
		if(pEntry->IKEIDType == IKE_ID_TYPE_NAME)
		{
			strncpy(secretIdName, pEntry->IKERemoteName, IPSEC_NAME_LEN-1);
			needId = 1;
		}
		if((pEntry->IPSecType == IPSEC_TYPE_PC2SITE)&&(pEntry->RemoteDomain[0] != '\0')){
			//if type is PC-to-Site, use RemoteDomain as remote PC's id firstly.
			strncpy(secretIdName, pEntry->RemoteDomain, IPSEC_NAME_LEN-1);
			needId = 1;
		}

		if(needId){
			offset += snprintf(secrets+offset, IPSEC_SWAN_CONTENT_LEN-offset, "      id-1 = %s\n", secretIdName);
		}

		offset += snprintf(secrets+offset, IPSEC_SWAN_CONTENT_LEN-offset, "      secret = %s\n", pEntry->IKEPreshareKey);
		offset += snprintf(secrets+offset, IPSEC_SWAN_CONTENT_LEN-offset,  "   }\n\n");
	}

	return offset;
}

#define IPSEC_STATUS_FILE "/tmp/ipsec_status"
#define IPSEC_DISCONNECT_FILE "/tmp/ipsec_disconnect"
#define IPSEC_BUFFER_SIZE 1024
#define IPSEC_PARAMETER_SIZE 32

/*---------file /tmp/ipsec_status sample start-----------
	conn1: #2, ESTABLISHED, IKEv2, d1ba4ef7d45d847e_i* 59032648f807aa9a_r
	  local  '192.168.2.188' @ 192.168.2.188[500]
	  remote '192.168.2.233' @ 192.168.2.233[500]
	  DES_CBC/HMAC_MD5_96/PRF_HMAC_MD5/MODP_768
	  established 3s ago, reauth in 8897s
	  net-net: #4, reqid 2, INSTALLED, TUNNEL, ESP:3DES_CBC/HMAC_MD5_96
		installed 3s ago, rekeying in 3478s, expires in 3957s
		in	cf7a7bc4,	   0 bytes, 	0 packets
		out cf2eee53,	   0 bytes, 	0 packets
		local  192.168.1.0/24
		remote 172.29.38.0/24
	conn0: #1, ESTABLISHED, IKEv2, d43f3b0751bfc2f1_i* 85d8a9d15558d1b0_r
	  local  'moon.strongswan.org' @ 192.168.2.188[500]
	  remote 'sun.strongswan.org' @ 192.168.2.77[500]
	  DES_CBC/HMAC_MD5_96/PRF_HMAC_MD5/MODP_768
	  established 7s ago, reauth in 8761s
	  net-net: #3, reqid 1, INSTALLED, TUNNEL, ESP:DES_CBC/HMAC_MD5_96
		installed 7s ago, rekeying in 3237s, expires in 3953s
		in	ca3c206f,	   0 bytes, 	0 packets
		out c26e4677,	   0 bytes, 	0 packets
		local  192.168.1.0/24
		remote 1.1.1.0/24
---------file /tmp/ipsec_status sample end-----------*/
void update_ipsec_swan_status(void)
{
	int entryIndex = -1, entryNum = 0;
	MIB_IPSEC_SWAN_T entry;
	char str[IPSEC_PARAMETER_SIZE];
	int len = 0;
	char *positionStart, *positionEnd;
	char lineContent[IPSEC_BUFFER_SIZE];
	FILE *fp, *fp1;

	//get swancrl --list-sas
	system("swanctl --list-sas > /tmp/ipsec_status");

	fp = fopen(IPSEC_STATUS_FILE, "r");
	if(fp == NULL){
		printf("failed to open file %s\n", IPSEC_STATUS_FILE);
		goto end;
	}

	entryNum = mib_chain_total(MIB_IPSEC_SWAN_TBL);

	while(!feof(fp)){
		fgets(lineContent, IPSEC_BUFFER_SIZE,fp);
		//printf("%s, get line content is: %s\n", __FUNCTION__, lineContent);

		positionStart = strstr(lineContent, IPSEC_SWAN_CONN_NAME);
		if(positionStart != NULL){
			//get entryIndex
			positionStart += strlen(IPSEC_SWAN_CONN_NAME);
			positionEnd = strchr(positionStart, ':');
			len = positionEnd - positionStart;
			memcpy(str, positionStart, len);
			str[len] = '\0';
			entryIndex = atoi(str);
			//printf("%s, entryIndex is %d\n", __FUNCTION__, entryIndex);

			if(entryIndex  >= entryNum){
				fprintf(stderr, "%s, entryIndex in line %s is over max entryId %d!\n",
					__FUNCTION__, lineContent, entryNum-1);
				continue;
			}
			mib_chain_get(MIB_IPSEC_SWAN_TBL, entryIndex, (void *)&entry);

			//get IKE Phase 1 state
			positionStart =  strchr(positionStart, ',');
			positionStart += 2;
			positionEnd = strchr(positionStart, ',');
			len = positionEnd - positionStart;
			if(len >= IPSEC_PARAMETER_SIZE){
				fprintf(stderr, "%s, get IKE Phase 1 state len is over len %d!\n",
					__FUNCTION__, len);
				goto end;
			}
			memcpy(str, positionStart, len);
			str[len] = '\0';
			//printf("%s, IKE Phase 1 state is %s\n", __FUNCTION__, str);

			//state refer user/strongswan-5.8.4/src/libcharon/sa/ike_sa.c
			if(strcmp(str, "CONNECTING") == 0){
				entry.ConnectionStatus = IPSEC_CONN_STATUS_PHASE1_CONNECTING;
			}
			else if(strcmp(str, "ESTABLISHED") == 0){
				entry.ConnectionStatus = IPSEC_CONN_STATUS_PHASE2_CONNECTING;
			}
			else if(strcmp(str, "DELETING") == 0){
				entry.ConnectionStatus = IPSEC_CONN_STATUS_DISCONNECTING;
			}
			else if(strcmp(str, "DESTROYING") == 0){
				entry.ConnectionStatus = IPSEC_CONN_STATUS_DISCONNECTING;
			}
			mib_chain_update(MIB_IPSEC_SWAN_TBL, &entry, entryIndex);
		}

		if(entryIndex == -1){
			continue;
		}

		//get IKE Phase 2 state
		positionStart = strstr(lineContent, IPSEC_SWAN_CHILD_NAME);
		if(positionStart != NULL){
			positionStart =  strchr(positionStart, ',');
			positionStart =  strchr(positionStart+1, ',');
			positionStart += 2;
			positionEnd = strchr(positionStart, ',');
			len = positionEnd - positionStart;
			if(len > IPSEC_PARAMETER_SIZE-1){
				fprintf(stderr, "%s, get IKE Phase 2 state len is over len %d!\n",
					__FUNCTION__, len);
				goto end;
			}

			memcpy(str, positionStart, len);
			str[len] = '\0';
			//printf("%s, IKE Phase 2 state is %s\n", __FUNCTION__, str);

			//state refer user/strongswan-5.8.4/src/libcharon/sa/child_sa.c
			if(strcmp(str, "INSTALLING") == 0){
				entry.ConnectionStatus = IPSEC_CONN_STATUS_PHASE2_CONNECTING;
			}
			else if(strcmp(str, "INSTALLED") == 0){
				entry.ConnectionStatus = IPSEC_CONN_STATUS_CONNECTED;
			}
			else if(strcmp(str, "DELETING") == 0){
				entry.ConnectionStatus = IPSEC_CONN_STATUS_DISCONNECTING;
			}
			else if(strcmp(str, "DESTROYING") == 0){
				entry.ConnectionStatus = IPSEC_CONN_STATUS_DISCONNECTING;
			}

			mib_chain_update(MIB_IPSEC_SWAN_TBL, &entry, entryIndex);
		}
	}

end:
	if(fp != NULL){
		fclose(fp);
	}

	fp1 = fopen(IPSEC_DISCONNECT_FILE, "r");
	if(fp1 != NULL){
		while(!feof(fp1)){
			fgets(lineContent, IPSEC_BUFFER_SIZE,fp1);
			printf("%s, get line content is: %s\n", __FUNCTION__, lineContent);

			entryIndex = atoi(lineContent);
			mib_chain_get(MIB_IPSEC_SWAN_TBL, entryIndex, (void *)&entry);
			entry.ConnectionStatus = IPSEC_CONN_STATUS_DISCONNECTED;
			mib_chain_update(MIB_IPSEC_SWAN_TBL, &entry, entryIndex);
		}

		fclose(fp1);
	}

	//unlink tmp file
	if(access(IPSEC_STATUS_FILE, 0)==0){
		unlink(IPSEC_STATUS_FILE);
	}

	//unlink tmp file
	if(access(IPSEC_DISCONNECT_FILE, 0)==0){
		unlink(IPSEC_DISCONNECT_FILE);
	}

	return;
}

void ipsec_strongswan_take_effect(void)
{
	int entryNum, i;
	int active = 0;
	MIB_IPSEC_SWAN_T entry;
	FILE *fpSwanCtl;
	char secrets[IPSEC_SWAN_CONTENT_LEN+1] = {0};
	int secrets_len = 0;
#if defined(CONFIG_USER_XL2TPD)
	int l2tpEnable;
#endif
	int wanReady=0;

#if defined(CONFIG_USER_XL2TPD)
	mib_get_s(MIB_L2TP_ENABLE, (void *)&l2tpEnable, sizeof(l2tpEnable));
	if (l2tpEnable == 0) {
#endif
	system("ipsec stop");
	system("iptables -D INPUT -p udp -m udp --dport 500 -j ACCEPT");
	system("iptables -D INPUT -p udp -m udp --dport 4500 -j ACCEPT");
	system("iptables -t nat -D POSTROUTING -m policy --dir out --pol ipsec --mode tunnel -j ACCEPT");

#ifdef CONFIG_IPV6
	system("ip6tables -D INPUT -p udp -m udp --dport 500 -j ACCEPT");
	system("ip6tables -D INPUT -p udp -m udp --dport 4500 -j ACCEPT");
#endif

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
	rtk_ipsec_strongswan_iptable_rule_flush();
#if defined(CONFIG_RTL8277C_SERIES)
	rtk_fc_ipsec_strongswan_traffic_upstream_bypass_config();
#endif
#endif

	if(access(SWANCTL_CONF, 0)==0){
		unlink(SWANCTL_CONF);
	}

	if(access(STRONGSWAN_CONF, 0)==0){
		unlink(STRONGSWAN_CONF);
	}

	entryNum = mib_chain_total(MIB_IPSEC_SWAN_TBL);
	if(entryNum > 0){
		applyStrongSwan();

		if ((fpSwanCtl = fopen(SWANCTL_CONF, "w+")) == NULL){
			printf("ERROR: open file %s error!\n", SWANCTL_CONF);
			return;
		}
		fprintf(fpSwanCtl, "connections {\n");
		snprintf(secrets, IPSEC_SWAN_CONTENT_LEN, "secrets {\n");
		for(i = 0; i<entryNum; i++){
			mib_chain_get(MIB_IPSEC_SWAN_TBL, i, (void *)&entry);

			if(entry.Enable == 1){
				if(checkIpsecStrongSwanParameters(i, &entry)!= 0){
					continue;
				}

				wanReady = setStrongswanConns(i, &entry, fpSwanCtl);
				if (!wanReady) continue;
				active++;
				secrets_len = setStrongswanSecrets(i, &entry, secrets);
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
				rtk_ipsec_strongswan_iptable_rule_add(&entry);
#endif
			}
		}
		fprintf(fpSwanCtl, "}\n");
		snprintf(secrets+secrets_len, IPSEC_SWAN_CONTENT_LEN-secrets_len, "}\n");

		fprintf(fpSwanCtl, "%s", secrets);

		fclose(fpSwanCtl);

		if(active > 0){
			system("ipsec start");
			system("iptables -I INPUT -p udp -m udp --dport 500 -j ACCEPT");
			system("iptables -I INPUT -p udp -m udp --dport 4500 -j ACCEPT");
			system("iptables -t nat -I POSTROUTING -m policy --dir out --pol ipsec --mode tunnel -j ACCEPT");
#ifdef CONFIG_IPV6
			system("ip6tables -I INPUT -p udp -m udp --dport 500 -j ACCEPT");
			system("ip6tables -I INPUT -p udp -m udp --dport 4500 -j ACCEPT");
#endif
		}
	}
#if defined(CONFIG_USER_XL2TPD)
	}
#endif
	return;
}

#if defined(CONFIG_USER_XL2TPD)
void getAllWANIPforL2TP(char *wanIPAll)
{
	int flags, flags_found;
	struct in_addr inAddr;
	int i, entryNum;
	MIB_CE_ATM_VC_T entry;
	char ifname[IFNAMSIZ];
	char *wanIP;
	char tmpBuf[100] = {0};
	char tmp_ip[128] = {0};

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < entryNum; i++){
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry)){
			continue;
		}

		if (entry.cmode == CHANNEL_MODE_BRIDGE)
			continue;

		ifGetName(entry.ifIndex, ifname, sizeof(ifname));

			flags_found = getInFlags(ifname, &flags);
			if (flags_found && getInAddr(ifname, IP_ADDR, &inAddr) == 1) {
				wanIP = inet_ntoa(inAddr);
			}

		snprintf(tmp_ip, sizeof(tmp_ip), "%s[udp/1701],", wanIP);
		strcat(wanIPAll, tmp_ip);
	}
	wanIPAll[strlen(wanIPAll) - 1] = '\0';

	return;
}

static void setStrongswanConnsforL2TP(FILE *fpSwanCtl)
{
	char wanIPAll[124] = {0};

	//create connections
	fprintf(fpSwanCtl, "   %s0 {\n", IPSEC_SWAN_CONN_NAME);
	fprintf(fpSwanCtl, "      aggressive = no\n");
	fprintf(fpSwanCtl, "      local {\n");
	fprintf(fpSwanCtl, "         auth = psk\n");
	fprintf(fpSwanCtl, "      }\n");
	fprintf(fpSwanCtl, "      remote {\n");
	fprintf(fpSwanCtl, "         auth = psk\n");
	fprintf(fpSwanCtl, "      }\n");
	fprintf(fpSwanCtl, "      children {\n");
	fprintf(fpSwanCtl, "         child {\n");

	getAllWANIPforL2TP(wanIPAll);
	fprintf(fpSwanCtl, "           local_ts  = %s\n", wanIPAll);
	fprintf(fpSwanCtl, "           updown = /libexec/ipsec/_updown iptables\n");
	fprintf(fpSwanCtl, "           rekey_time = 3600\n");
	fprintf(fpSwanCtl, "           rekey_bytes = 1887436800\n");
	fprintf(fpSwanCtl, "           esp_proposals = 3des-sha1,3des-md5,des-sha1,des-md5,aes128-sha1,aes128-md5,aes192-sha1,aes192-md5,aes256-sha1,aes256-md5\n");
	fprintf(fpSwanCtl, "           start_action = trap\n");  //which triggers the tunnel as soon as matching traffic has been detected
	fprintf(fpSwanCtl, "           mode = transport\n");
	fprintf(fpSwanCtl, "         }\n");
	fprintf(fpSwanCtl, "      }\n");
	fprintf(fpSwanCtl, "      version = 1\n");
	fprintf(fpSwanCtl, "      mobike = no\n");
	fprintf(fpSwanCtl, "      reauth_time = 10800\n");
	fprintf(fpSwanCtl, "      proposals = 3des-sha1-modp2048,aes256-sha1-modp768,aes256-sha1-modp2048,aes128-sha1-modp1024\n");
	fprintf(fpSwanCtl, "   }\n\n");
	return;
}

static int setStrongswanSecretsforL2TP(int entryIndex, unsigned char *IKEPreshareKey, char *secrets)
{
	int offset = strlen(secrets);
	char secretIdName[IPSEC_NAME_LEN] = {0};

	//create secrets
	offset += snprintf(secrets+offset, IPSEC_SWAN_CONTENT_LEN-offset, "   ike-%d {\n", entryIndex);
	offset += snprintf(secrets+offset, IPSEC_SWAN_CONTENT_LEN-offset, "      secret = %s\n", IKEPreshareKey);
	offset += snprintf(secrets+offset, IPSEC_SWAN_CONTENT_LEN-offset,  "   }\n\n");
	return offset;
}

void l2tpIPSec_LAC_strongswan_take_effect(void)
{
	int entryNum, i;
	int active = 0;
	MIB_L2TP_T entry;
	FILE *fpSwanCtl;
	char secrets[IPSEC_SWAN_CONTENT_LEN+1] = {0};
	int secrets_len = 0;
	int l2tpType;

	mib_get_s(MIB_L2TP_MODE, (void *)&l2tpType, sizeof(l2tpType));
	if (l2tpType == L2TP_MODE_LAC) {
		system("ipsec stop");
		system("iptables -D INPUT -p udp -m udp --dport 500 -j ACCEPT");
		system("iptables -D INPUT -p udp -m udp --dport 4500 -j ACCEPT");
		system("iptables -t nat -D POSTROUTING -m policy --dir out --pol ipsec --mode tunnel -j ACCEPT");
		system("iptables -D INPUT ! -i br0 -p 50 -j ACCEPT");   // for ESP

		if(access(SWANCTL_CONF, 0)==0){
			unlink(SWANCTL_CONF);
		}

		if(access(STRONGSWAN_CONF, 0)==0){
			unlink(STRONGSWAN_CONF);
		}

		entryNum = mib_chain_total(MIB_L2TP_TBL);
		if(entryNum > 0){
			applyStrongSwan();

			if ((fpSwanCtl = fopen(SWANCTL_CONF, "w+")) == NULL){
				printf("ERROR: open file %s error!\n", SWANCTL_CONF);
				return;
			}
			fprintf(fpSwanCtl, "connections {\n");
			snprintf(secrets, IPSEC_SWAN_CONTENT_LEN, "secrets {\n");
			for(i = 0; i<entryNum; i++){
				mib_chain_get(MIB_L2TP_TBL, i, (void *)&entry);
				if(entry.overIPSec == 1)
					active++;
			}

			if(active > 0) {
				setStrongswanConnsforL2TP(fpSwanCtl);
				for(i = 0; i<entryNum; i++){
					mib_chain_get(MIB_L2TP_TBL, i, (void *)&entry);

					if(entry.overIPSec == 1){
						secrets_len = setStrongswanSecretsforL2TP(i, entry.IKEPreshareKey, secrets);
					}
				}
			}
			fprintf(fpSwanCtl, "}\n");
			snprintf(secrets+secrets_len, IPSEC_SWAN_CONTENT_LEN-secrets_len, "}\n");
			fprintf(fpSwanCtl, "%s", secrets);
			fclose(fpSwanCtl);

			if(active > 0){
				system("ipsec start");
				system("iptables -I INPUT -p udp -m udp --dport 500 -j ACCEPT");
				system("iptables -I INPUT -p udp -m udp --dport 4500 -j ACCEPT");
				system("iptables -t nat -I POSTROUTING -m policy --dir out --pol ipsec --mode tunnel -j ACCEPT");
				system("iptables -I INPUT ! -i br0 -p 50 -j ACCEPT");   // for ESP
			}
		}
	}
	return;
}

#ifdef CONFIG_USER_L2TPD_LNS
void l2tpIPSec_LNS_strongswan_take_effect(void)
{
	int entryNum, i;
	int active = 0;
	MIB_VPND_T entry;
	FILE *fpSwanCtl;
	char secrets[IPSEC_SWAN_CONTENT_LEN+1] = {0};
	int secrets_len = 0;
	int l2tpType;

	mib_get_s(MIB_L2TP_MODE, (void *)&l2tpType, sizeof(l2tpType));
	if (l2tpType == L2TP_MODE_LNS) {
		system("ipsec stop");
		//iptables -D INPUT -p udp -m udp --dport 500 -j ACCEPT
		va_cmd(IPTABLES, 10, 1, FW_DEL, IPTABLE_L2TP_VPN_IN, "-p", "udp", "-m", "udp", "--dport", PORT_IPSEC, "-j", "ACCEPT");
		//iptables -D INPUT -p udp -m udp --dport 4500 -j ACCEPT
		va_cmd(IPTABLES, 10, 1, FW_DEL, IPTABLE_L2TP_VPN_IN, "-p", "udp", "-m", "udp", "--dport", PORT_IPSEC_NAT, "-j", "ACCEPT");
		system("iptables -t nat -D POSTROUTING -m policy --dir out --pol ipsec --mode tunnel -j ACCEPT");
		//iptables -D INPUT ! -i br0 -p 50 -j ACCEPT
		va_cmd(IPTABLES, 9, 1, FW_DEL, IPTABLE_L2TP_VPN_IN, "!", ARG_I, LANIF_ALL, "-p", "50", "-j", "ACCEPT");

		if(access(SWANCTL_CONF, 0)==0){
			unlink(SWANCTL_CONF);
		}

		if(access(STRONGSWAN_CONF, 0)==0){
			unlink(STRONGSWAN_CONF);
		}

		entryNum = mib_chain_total(MIB_VPN_SERVER_TBL);
		if(entryNum > 0){
			applyStrongSwan();

			if ((fpSwanCtl = fopen(SWANCTL_CONF, "w+")) == NULL){
				printf("ERROR: open file %s error!\n", SWANCTL_CONF);
				return;
			}
			fprintf(fpSwanCtl, "connections {\n");
			snprintf(secrets, IPSEC_SWAN_CONTENT_LEN, "secrets {\n");
			for(i = 0; i<entryNum; i++){
				mib_chain_get(MIB_VPN_SERVER_TBL, i, (void *)&entry);
				if(entry.overIPSec == 1)
					active++;
			}

			if(active > 0) {
				setStrongswanConnsforL2TP(fpSwanCtl);
				for(i = 0; i<entryNum; i++){
					mib_chain_get(MIB_VPN_SERVER_TBL, i, (void *)&entry);

					if(entry.overIPSec == 1){
						secrets_len = setStrongswanSecretsforL2TP(i, entry.IKEPreshareKey, secrets);
					}
				}
			}
			fprintf(fpSwanCtl, "}\n");
			snprintf(secrets+secrets_len, IPSEC_SWAN_CONTENT_LEN-secrets_len, "}\n");
			fprintf(fpSwanCtl, "%s", secrets);
			fclose(fpSwanCtl);

			if(active > 0){
				system("ipsec start");
				//iptables -I INPUT -p udp -m udp --dport 500 -j ACCEPT
				va_cmd(IPTABLES, 10, 1, FW_INSERT, IPTABLE_L2TP_VPN_IN, "-p", "udp", "-m", "udp", "--dport", PORT_IPSEC, "-j", "ACCEPT");
				//iptables -I INPUT -p udp -m udp --dport 4500 -j ACCEPT
				va_cmd(IPTABLES, 10, 1, FW_INSERT, IPTABLE_L2TP_VPN_IN, "-p", "udp", "-m", "udp", "--dport", PORT_IPSEC_NAT, "-j", "ACCEPT");
				system("iptables -t nat -I POSTROUTING -m policy --dir out --pol ipsec --mode tunnel -j ACCEPT");
				//iptables -I INPUT ! -i br0 -p 50 -j ACCEPT
				va_cmd(IPTABLES, 9, 1, FW_INSERT, IPTABLE_L2TP_VPN_IN, "!", ARG_I, LANIF_ALL, "-p", "50", "-j", "ACCEPT");
			}
		}
	}
	return;
}
#endif  // #ifdef CONFIG_USER_L2TPD_LNS
#endif  // #if defined(CONFIG_USER_XL2TPD)
#else
struct IPSEC_PROP_ST ikeProps[] = {
	{1, "------------------------", 0x00000000},
	{1, "des-md5-group1", 0x01010001},//dhGroup|Encryption|ahAuth|Auth
	{1, "des-sha1-group1", 0x01010002},
	{1, "3des-md5-group2", 0x02020001},
	{1, "3des-sha1-group2", 0x02020002},
    {0} //invalid should be the last one!!!!
};

char *curr_dhArray[] = {
	"null",
	"modp768",
	"modp1024",
	NULL
};

char *curr_encryptAlgm[] = {
	"null_enc",
	"des",
	"3des",
#ifdef CONFIG_CRYPTO_AES
	"aes",
#endif
    NULL
};
static char *setkey_encryptAlgm[] = {
	"null_enc",
	"des-cbc",
	"3des-cbc",
#ifdef CONFIG_CRYPTO_AES
	"aes-cbc",
#endif
    NULL
};

char *curr_authAlgm[] = {
	"non_auth",
	"md5",
	"sha1",
	NULL
};
static char *setkey_authAlgm[] = {
	"non_auth",
	"hmac-md5",
	"hmac-sha1",
	NULL
};
static char *racoon_authAlgm[] = {
	"non_auth",
	"hmac_md5",
	"hmac_sha1",
	NULL
};

/*
return
	0: failed
	1: succeed
*/
static int applyIPsec(FILE *fpSetKey, FILE *fpRacoon, FILE *fpPSK, MIB_IPSEC_T *pEntry)
{
	char *strVal;
	int intVal;
	char *saProtocol[2]={"esp", "ah"};
	char *transportMode[2]={"tunnel", "transport"};
	char *ikeMode[2]={"main", "aggressive"};
	char *ikeauthmethod[2]={"pre_shared_key", "rsasig"};
	char *filterProtocol[4]={"any", "tcp", "udp", "icmp"};
	char filterPort[10];

	if ((fpPSK==NULL) || (fpSetKey==NULL) ||(fpRacoon==NULL) || (pEntry==NULL))
	{
		printf("\n %s %d  Invaild arguments\n", __func__, __LINE__);
		return 0;
	}
	//psk mode
	if(0==pEntry->auth_method)
	{
		fprintf(fpPSK, "%s %s\n", pEntry->remoteTunnel, pEntry->psk);
	}

	if(pEntry->filterPort != 0)
		snprintf(filterPort, 10, "%d", pEntry->filterPort);
	else
		snprintf(filterPort, 10, "%s", "any");

	if(pEntry->negotiationType == 1){
		//manual mode
		if((pEntry->encapMode&0x1) != 0x0){
			fprintf(fpSetKey, "add %s %s esp 0x%x -m %s ", pEntry->localTunnel,pEntry->remoteTunnel,
				pEntry->espOUTSPI, transportMode[pEntry->transportMode]);

			if(pEntry->espEncrypt != 0)
				fprintf(fpSetKey, "-E %s 0x%s ", setkey_encryptAlgm[pEntry->espEncrypt], pEntry->espEncryptKey);
			if(pEntry->espAuth != 0)
				fprintf(fpSetKey, "-A %s 0x%s ", setkey_authAlgm[pEntry->espAuth], pEntry->espAuthKey);

			fprintf(fpSetKey, ";\n\n");

			fprintf(fpSetKey, "add %s %s esp 0x%x -m %s ", pEntry->remoteTunnel,pEntry->localTunnel,
				pEntry->espINSPI, transportMode[pEntry->transportMode]);

			if(pEntry->espEncrypt != 0)
				fprintf(fpSetKey, "-E %s 0x%s ", setkey_encryptAlgm[pEntry->espEncrypt], pEntry->espEncryptKey);
			if(pEntry->espAuth != 0)
				fprintf(fpSetKey, "-A %s 0x%s ", setkey_authAlgm[pEntry->espAuth], pEntry->espAuthKey);

			fprintf(fpSetKey, ";\n\n");
		}

		if((pEntry->encapMode&0x2) != 0x0){
			fprintf(fpSetKey, "add %s %s ah 0x%x -m %s -A %s 0x%s ;\n\n", pEntry->localTunnel, pEntry->remoteTunnel,
				pEntry->ahOUTSPI, transportMode[pEntry->transportMode], setkey_authAlgm[pEntry->ahAuth], pEntry->ahAuthKey);

			fprintf(fpSetKey, "add %s %s ah 0x%x -m %s -A %s 0x%s ;\n\n", pEntry->remoteTunnel, pEntry->localTunnel,
				pEntry->ahINSPI, transportMode[pEntry->transportMode], setkey_authAlgm[pEntry->ahAuth], pEntry->ahAuthKey);

		}
	}

	//direction: in
	if(pEntry->transportMode == 0){
		fprintf(fpSetKey, "spdadd %s/%d[%s] %s/%d[%s] %s -P in ipsec", pEntry->remoteIP, pEntry->remoteMaskLen, filterPort,
			pEntry->localIP, pEntry->localMaskLen, filterPort, filterProtocol[pEntry->filterProtocol]);
		if((pEntry->encapMode&0x1) != 0x0)
			fprintf(fpSetKey, " esp/tunnel/%s-%s/require", pEntry->remoteTunnel, pEntry->localTunnel);
		if((pEntry->encapMode&0x2) != 0x0)
			fprintf(fpSetKey, " ah/tunnel/%s-%s/require", pEntry->remoteTunnel, pEntry->localTunnel);
	}else{
		fprintf(fpSetKey, "spdadd %s[%s] %s[%s] %s -P in ipsec", pEntry->remoteTunnel, filterPort,
			pEntry->localTunnel, filterPort, filterProtocol[pEntry->filterProtocol]);
		if((pEntry->encapMode&0x1) != 0x0)
			fprintf(fpSetKey, " esp/transport//use");
		if((pEntry->encapMode&0x2) != 0x0)
			fprintf(fpSetKey, " ah/transport//use");
	}

	fprintf(fpSetKey, ";\n\n");

	//direction: out
	if(pEntry->transportMode == 0){
		fprintf(fpSetKey, "spdadd %s/%d[%s] %s/%d[%s] %s -P out ipsec", pEntry->localIP, pEntry->localMaskLen, filterPort,
			pEntry->remoteIP, pEntry->remoteMaskLen, filterPort, filterProtocol[pEntry->filterProtocol]);
		if((pEntry->encapMode&0x1) != 0x0)
			fprintf(fpSetKey, " esp/tunnel/%s-%s/require", pEntry->localTunnel, pEntry->remoteTunnel);
		if((pEntry->encapMode&0x2) != 0x0)
			fprintf(fpSetKey, " ah/tunnel/%s-%s/require", pEntry->localTunnel, pEntry->remoteTunnel);
	}else{
		fprintf(fpSetKey, "spdadd %s[%s] %s[%s] %s -P out ipsec", pEntry->localTunnel, filterPort,
			pEntry->remoteTunnel, filterPort, filterProtocol[pEntry->filterProtocol]);
		if((pEntry->encapMode&0x1) != 0x0)
			fprintf(fpSetKey, " esp/transport//require");
		if((pEntry->encapMode&0x2) != 0x0)
			fprintf(fpSetKey, " ah/transport//require");
	}
	fprintf(fpSetKey, ";\n\n");

	//ike mode
	if(pEntry->negotiationType == 0){

// Mason Yu.
#if 0
		//for NAT-Traversal
		fprintf(fpRacoon, "timer {\n");
		fprintf(fpRacoon, "\tnatt_keepalive 10sec ;\n");
		fprintf(fpRacoon, "\t}\n");

		fprintf(fpRacoon, "listen {\n");
		fprintf(fpRacoon, "\tisakmp %s [500] ;\n", homeAddr);
		fprintf(fpRacoon, "\tisakmp_natt %s [4500] ;\n", homeAddr);
		fprintf(fpRacoon, "\t}\n");
		//end
#endif
		fprintf(fpRacoon, "remote %s {\n", pEntry->remoteTunnel);
		fprintf(fpRacoon, "\texchange_mode %s ;\n", ikeMode[pEntry->ikeMode]);
// Mason Yu.
		//fprintf(fpRacoon, "\tnat_traversal on ;\n");
		fprintf(fpRacoon, "\tlifetime time %d second ;\n", pEntry->ikeAliveTime);
		if(1==pEntry->auth_method){
			//certificate mode
			//ToDo:
			fprintf(fpRacoon, "\tcertificate_type x509 \"cert.pem\" \"privKey.pem\" ;\n");
			fprintf(fpRacoon, "\tverify_cert on ;\n");
			fprintf(fpRacoon, "\tmy_identifier asn1dn ;\n");
			fprintf(fpRacoon, "\tpeers_identifier asn1dn ;\n");
		}

		for(intVal = 0; intVal<4; intVal++){
			if(pEntry->ikeProposal[intVal]==0)
				continue;
			fprintf(fpRacoon, "\tproposal {\n");
			fprintf(fpRacoon, "\t\tencryption_algorithm %s ;\n", curr_encryptAlgm[ENCRYPT_INDEX(ikeProps[pEntry->ikeProposal[intVal]].algorithm)]);
			fprintf(fpRacoon, "\t\thash_algorithm %s ;\n", curr_authAlgm[AUTH_INDEX(ikeProps[pEntry->ikeProposal[intVal]].algorithm)]);
			fprintf(fpRacoon, "\t\tauthentication_method %s ;\n", ikeauthmethod[pEntry->auth_method]);
			fprintf(fpRacoon, "\t\tdh_group %d ;\n", DHGROUP_INDEX(ikeProps[pEntry->ikeProposal[intVal]].algorithm));
			fprintf(fpRacoon, "\t}\n\n");
		}
		fprintf(fpRacoon, "}\n\n");

		for(intVal = 1; curr_dhArray[intVal]!=NULL; intVal++){
			int i, flag;

			if(0x0 == (pEntry->pfs_group&(1<<intVal)))
				continue;
			if (pEntry->transportMode==0){
				fprintf(fpRacoon, "sainfo address %s/%d[%s] %s address %s/%d[%s] %s{\n",
					pEntry->localIP,pEntry->localMaskLen, filterPort, filterProtocol[pEntry->filterProtocol],
					pEntry->remoteIP,pEntry->remoteMaskLen, filterPort,filterProtocol[pEntry->filterProtocol]);
			}else{
				fprintf(fpRacoon, "sainfo address %s[%s] %s address %s[%s] %s{\n",
					pEntry->localTunnel,filterPort, filterProtocol[pEntry->filterProtocol],
					pEntry->remoteTunnel,	filterPort,filterProtocol[pEntry->filterProtocol]);
			}
			fprintf(fpRacoon, "\tpfs_group %s ;\n", curr_dhArray[intVal]);
			fprintf(fpRacoon, "\tlifetime time %d second ;\n", pEntry->saAliveTime);
			fprintf(fpRacoon, "\tencryption_algorithm ");
			flag = 0;
			for(i=0; curr_encryptAlgm[i]!=NULL; i++)
			{
				if(pEntry->encrypt_group&(1<<i))
				{
					if(0==flag)
					{
						fprintf(fpRacoon, "%s", curr_encryptAlgm[i]);
						flag = 1;
					}
					else
					{
						fprintf(fpRacoon, ", %s", curr_encryptAlgm[i]);
					}
				}
			}
			fprintf(fpRacoon, " ;\n");
			fprintf(fpRacoon, "\tauthentication_algorithm ");
			flag = 0;
			for(i=0; racoon_authAlgm[i]!=NULL; i++)
			{
				if(pEntry->auth_group&(1<<i))
				{
					if(0==flag)
					{
						fprintf(fpRacoon, "%s", racoon_authAlgm[i]);
						flag = 1;
					}
					else
					{
						fprintf(fpRacoon, ", %s", racoon_authAlgm[i]);
					}
				}
			}
			fprintf(fpRacoon, " ;\n");
			fprintf(fpRacoon, "\tcompression_algorithm deflate ;\n");
			fprintf(fpRacoon, "}\n\n");
		}
	}
	return 1;
}

void apply_nat_rule(MIB_IPSEC_T *pEntry)
{
	char *strVal;
	char filterPort[10];
	char local[24], remote[24];
	char *argv[20];
	int i = 3;

	if(1 == pEntry->transportMode)
	{
		//transportMode do not need it.
		return;
	}

	strVal = inet_ntoa(*(struct in_addr*)pEntry->localIP);
	snprintf(local, 24, "%s/%d", strVal, pEntry->localMaskLen);
	strVal = inet_ntoa(*(struct in_addr*)pEntry->remoteIP);
	snprintf(remote, 24, "%s/%d", strVal, pEntry->remoteMaskLen);
	if(pEntry->filterPort != 0)
		snprintf(filterPort, 10, "%d", pEntry->filterPort);

	argv[1] = "-t";
	argv[2] = "nat";
	if(1 == pEntry->state)
	{
		//for tunnel mode, should disable NAT for SPD.
		argv[i++] = (char *)FW_INSERT;
	}
	else
	{
		argv[i++] = (char *)FW_DEL;
	}
	argv[i++] = (char *)FW_POSTROUTING;

	if(0 != pEntry->filterProtocol)
	{
		argv[i++] = "-p";
		if(1 == pEntry->filterProtocol)
			argv[i++] = (char *)ARG_TCP;
		else if(2 == pEntry->filterProtocol)
			argv[i++] = (char *)ARG_UDP;
		else
			argv[i++] = (char *)ARG_ICMP;
	}

	//src ip
	argv[i++] = "-s";
	argv[i++] = local;

	// src port
	if ((1 == pEntry->filterProtocol) || (2 == pEntry->filterProtocol)) {
		if(0 != pEntry->filterPort){
			argv[i++] = (char *)FW_SPORT;
			argv[i++] = filterPort;
		}
	}

	// dst ip
	argv[i++] = "-d";
	argv[i++] = remote;

	// dst port
	if ((1 == pEntry->filterProtocol) || (2 == pEntry->filterProtocol)) {
		if(0 != pEntry->filterPort){
			argv[i++] = (char *)FW_DPORT;
			argv[i++] = filterPort;
		}
	}

	argv[i++] = "-j";
	argv[i++] = (char *)FW_RETURN;
	argv[i++] = NULL;

	//printf("i=%d\n", i);
	TRACE(STA_SCRIPT, "%s %s %s %s %s %s %s ...\n", IPTABLES, argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
	do_cmd(IPTABLES, argv, 1);

    return;
}

void add_ipsec_mangle_rule(void)
{
	unsigned char vChar;
	int ori_wlan_idx;
	char tnlName[32], markStr[32];

	snprintf(markStr, 32, "0x%x/0x%x", BYPASS_FWDENGINE_MARK_MASK, BYPASS_FWDENGINE_MARK_MASK);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "XFRM");

	//iptables -t mangle -A XFRM -p esp -j MARK --set-mark 0x8/0x8
	va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, "XFRM", "-p", "esp", "-j", "MARK",
			"--set-mark", markStr);
	//iptables -t mangle -A XFRM -p ah -j MARK --set-mark 0x8/0x8
	va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, "XFRM", "-p", "ah", "-j", "MARK",
			"--set-mark", markStr);
	//iptables -t mangle -A XFRM -p udp --dport 500 --sport 500 -j MARK --set-mark 0x8/0x8
	va_cmd(IPTABLES, 14, 1, "-t", "mangle", (char *)FW_ADD, "XFRM", "-p", (char *)ARG_UDP, "--dport", "500", "--sport",
			"500", "-j", "MARK", "--set-mark", markStr);
#ifdef CONFIG_IPV6_VPN
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "XFRM");

	//ip6tables -t mangle -A XFRM -p esp -j MARK --set-mark 0x8/0x8
	va_cmd(IP6TABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, "XFRM", "-p", "esp", "-j", "MARK",
			"--set-mark", markStr);
	//ip6tables -t mangle -A XFRM -p ah -j MARK --set-mark 0x8/0x8
	va_cmd(IP6TABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, "XFRM", "-p", "ah", "-j", "MARK",
			"--set-mark", markStr);
	//ip6tables -t mangle -A XFRM -p udp --dport 500 --sport 500 -j MARK --set-mark 0x8/0x8
	va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_ADD, "XFRM", "-p", (char *)ARG_UDP, "--dport", "500", "--sport",
			"500", "-j", "MARK", "--set-mark", markStr);
#endif
}

void del_ipsec_mangle_rule(void)
{
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "XFRM");
#ifdef CONFIG_IPV6_VPN
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "XFRM");
#endif
}

void ipsec_take_effect(void)
{
	FILE *fp=NULL;
	char pid[10];
	FILE *fpPSK, *fpSetKey, *fpRacoon;
	int entryNum, i, newState;
	MIB_IPSEC_T Entry;

	va_cmd("/bin/setkey", 1, 1, "-F");
	va_cmd("/bin/setkey", 1, 1, "-FP");

	if(access(RACOON_PID, 0)==0){
		if ((fp = fopen(RACOON_PID, "r"))== NULL){
			printf("ERROR: open file %s error!\n", RACOON_PID);
			return;
		}
		if(fscanf(fp, "%d\n", &i) != 1)
			printf("fscanf failed: %s %d\n", __func__, __LINE__);
		snprintf(pid, 10, "%d", i);
		va_cmd("/bin/kill", 1, 1,  pid);
		unlink(RACOON_PID);
		if(fp) fclose(fp);
	}

	if(access(PSK_FILE, 0)==0){
		unlink(PSK_FILE);
	}

	if(access(RACOON_LOG, 0)==0){
		unlink(RACOON_LOG);
	}

	if(access(SETKEY_CONF, 0)==0){
		unlink(SETKEY_CONF);
	}

	if(access(RACOON_CONF, 0)==0){
		unlink(RACOON_CONF);
	}

	del_ipsec_mangle_rule();

	//Setup basic conf for setkey.conf / racoon.conf / psk.txt
	va_cmd("/bin/mkdir", 2, 1, "-p", CERT_FLOD);

	if (((fpSetKey = fopen(SETKEY_CONF, "w+"))== NULL)
		||((fpRacoon = fopen(RACOON_CONF, "w+"))== NULL)
		||((fpPSK = fopen(PSK_FILE, "w+"))== NULL))
	{
		printf("ERROR: open file %s error!\n");
		return;
	}
	fprintf(fpSetKey, "flush;\n");
	fprintf(fpSetKey, "spdflush;\n\n");
	fprintf(fpRacoon, "path pre_shared_key \"%s\" ;\n\n", PSK_FILE);
	fprintf(fpRacoon, "path certificate \"%s\" ;\n\n", CERT_FLOD);

	entryNum = mib_chain_total(MIB_IPSEC_TBL);
	for(i = 0; i<entryNum; i++)
	{
		if(mib_chain_get(MIB_IPSEC_TBL, i, (void *)&Entry) == 0)
			printf("\n %s %d\n", __func__, __LINE__);
			if(Entry.enable ==1)
			{
				newState = applyIPsec(fpSetKey,fpRacoon,fpPSK,&Entry);
				if(newState != Entry.state)
				{
					Entry.state = newState;
					mib_chain_update(MIB_IPSEC_TBL, &Entry, i);
				}
			}
			else
			{
				if(0 != Entry.state){
					Entry.state = 0;
					mib_chain_update(MIB_IPSEC_TBL, &Entry, i);
				}
			}
#ifdef CONFIG_IPV6_VPN
			if (Entry.IpProtocol ==1)
#endif
				apply_nat_rule(&Entry); //just for IPV4
	}

	if (fpSetKey)
		fclose(fpSetKey);
	if (fpPSK)
		fclose(fpPSK);
	if (fpRacoon)
		fclose(fpRacoon);

	if(access(PSK_FILE, 0)==0)
	{
		va_cmd("/bin/chmod", 2, 1, "600", PSK_FILE);
	}

	if(access(SETKEY_CONF, 0)==0){
		va_cmd("/bin/setkey", 2, 1, "-f", SETKEY_CONF);
	}

	if(access(RACOON_CONF, 0)==0){
		va_cmd("/bin/racoon", 4, 1, "-f", RACOON_CONF, "-l", RACOON_LOG);
		add_ipsec_mangle_rule();
#ifdef CONFIG_GPON_FEATURE
		_routeInternetWanCheck();
#endif
	}

	return;
}
#endif
#endif

#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
#define SNMPPID  "/var/run/snmpd.pid"

#ifdef CONFIG_USER_SNMPD_SNMPD_V2CTRAP
// Added by Mason Yu for ILMI(PVC) community string
static const char PVCREADSTR[] = "ADSL";
static const char PVCWRITESTR[] = "ADSL";
// Added by Mason Yu for write community string
static const char SNMPCOMMSTR[] = "/var/snmpComStr.conf";
// return value:
// 1  : successful
// -1 : startup failed
int startSnmp(void)
{
	unsigned char value[16];
	unsigned char trapip[16];
	unsigned char commRW[100], commRO[100], enterOID[100];
	FILE 	      *fp;

	// Get SNMP Trap Host IP Address
	if(!mib_get_s( MIB_SNMP_TRAP_IP,  (void *)value, sizeof(value))){
		printf("Can no read MIB_SNMP_TRAP_IP\n");
	}

	if (((struct in_addr *)value)->s_addr != 0)
	{
		strncpy(trapip, inet_ntoa(*((struct in_addr *)value)), 16);
		trapip[15] = '\0';
	}
	else
		trapip[0] = '\0';
	//printf("***** trapip = %s\n", trapip);

	// Get CommunityRO String
	if(!mib_get_s( MIB_SNMP_COMM_RO_ENC,  (void *)commRO, sizeof(commRO))) {
		printf("Can no read MIB_SNMP_COMM_RO_ENC\n");
	}
	//printf("*****buffer = %s\n", commRO);


	// Get CommunityRW String
	if(!mib_get_s( MIB_SNMP_COMM_RW_ENC,  (void *)commRW, sizeof(commRW))) {
		printf("Can no read MIB_SNMP_COMM_RW_ENC\n");
	}
	//printf("*****commRW = %s\n", commRW);


	// Get Enterprise OID
	if(!mib_get_s( MIB_SNMP_SYS_OID,  (void *)enterOID, sizeof(enterOID))) {
		printf("Can no read MIB_SNMP_SYS_OID\n");
	}
	//printf("*****enterOID = %s\n", enterOID);


	// Write community string to file
	if ((fp = fopen(SNMPCOMMSTR, "w")) == NULL)
	{
		printf("Open file %s failed !\n", SNMPCOMMSTR);
		return -1;
	}

	if (commRO[0])
		fprintf(fp, "readStr %s\n", commRO);
	if (commRW[0])
		fprintf(fp, "writeStr %s\n", commRW);

	// Add ILMI(PVC) community string
	fprintf(fp, "PvcReadStr %s\n", PVCREADSTR);
	fprintf(fp, "PvcWriteStr %s\n", PVCWRITESTR);
	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(SNMPCOMMSTR, 0666);
#endif
	// Mason Yu
	// ZyXEL Remote management does not verify the comm string, so we can limit the comm string as "ADSL"

	if(trapip[0] == '\0')
	{
		if (va_niced_cmd("/bin/snmpd", 6, 0, "-p", "161", "-c", PVCREADSTR, "-te", enterOID))
			return -1;
	}
	else
	{
		if (va_niced_cmd("/bin/snmpd", 8, 0, "-p", "161", "-c", PVCREADSTR, "-th", trapip, "-te", enterOID))
			return -1;
	}
	return 1;

}
#else
#define SNMP_VERSION_2 1
#define SNMP_VERSION_3 2

static const char NETSNMPCONF[] = "/var/snmpd.conf";

int startSnmp(void)
{
	unsigned char commRW[100], commRO[100];
	unsigned char commRW_on, commRW_type, commRO_on, commRO_type;
	FILE *fp;
	char buf[256];

	if(!mib_get_s( MIB_SNMP_COMM_RO_ON,  (void *)&commRO_on, sizeof(commRO_on))) {
		printf("Can't read MIB_SNMP_COMM_RO_ON\n");
	}

	if(!mib_get_s( MIB_SNMP_COMM_RO_TYPE, (void *)&commRO_type, sizeof(commRO_type))) {
		printf("Can't read MIB_SNMP_COMM_RO_TYPE\n");
	}
	// Get CommunityRO String
	if(!mib_get_s( MIB_SNMP_COMM_RO_ENC,  (void *)commRO, sizeof(commRO))) {
		printf("Can't read MIB_SNMP_COMM_RO_ENC\n");
	}

	if(!mib_get_s( MIB_SNMP_COMM_RW_ON,  (void *)&commRW_on, sizeof(commRW_on))) {
		printf("Can't read MIB_SNMP_COMM_RW_ON\n");
	}

	if(!mib_get_s( MIB_SNMP_COMM_RW_TYPE, (void *)&commRW_type, sizeof(commRW_type))) {
		printf("Can't read MIB_SNMP_COMM_RW_TYPE\n");
	}
	// Get CommunityRW String
	if(!mib_get_s( MIB_SNMP_COMM_RW_ENC,  (void *)commRW, sizeof(commRW))) {
		printf("Can't read MIB_SNMP_COMM_RW_ENC\n");
	}


	// Write community string to file
	if ((fp = fopen(NETSNMPCONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", NETSNMPCONF);
		return -1;
	}

	if (commRO[0])
	{
		if(commRO_on)
		{
			if(commRO_type)
			{
				fprintf(fp, "rwcommunity %s\n", commRO);
				fprintf(fp, "rwcommunity6 %s\n", commRO);
				fprintf(fp, "rwcommunity iptvshrw^_\n");
			}
			else
			{
				fprintf(fp, "rocommunity %s\n", commRO);
				fprintf(fp, "rocommunity6 %s\n", commRO);
				fprintf(fp, "rocommunity iptvshro^_\n");
			}
		}
	}

	if (commRW[0])
	{
		if(commRW_on)
		{
			if(commRW_type)
			{
				fprintf(fp, "rwcommunity %s\n", commRW);
				fprintf(fp, "rwcommunity6 %s\n", commRW);
				fprintf(fp, "rwcommunity iptvshrw^_\n");
			}
			else
			{
				fprintf(fp, "rocommunity %s\n", commRW);
				fprintf(fp, "rocommunity6 %s\n", commRW);
				fprintf(fp, "rocommunity iptvshro^_\n");
			}
		}
	}

	fprintf(fp, "agentAddress udp:0.0.0.0:161,udp:0.0.0.0:30161\n");

	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(NETSNMPCONF, 0666);
#endif

	snprintf(buf, 256, "snmpd -r -L -c %s -p %s", NETSNMPCONF, SNMPPID);
	system(buf);

	return 1;
}
#endif

int restart_snmp(int flag)
{
	unsigned char value[32];
	int snmppid=0;
	int status=0, i;

	snmppid = read_pid((char*)SNMPPID);

	//printf("\nsnmppid=%d\n",snmppid);

	if(snmppid > 0) kill(snmppid, 9);

	if(snmppid > 0)
	{
		i = 0;
		while (kill(snmppid, 0) == 0 && i < 20) {
			usleep(100000); i++;
		}
		if(i >= 20 )
			kill(snmppid, SIGKILL);

		if(!access(SNMPPID, F_OK)) unlink(SNMPPID);
	}

	if(flag==1){
		status = startSnmp();
	}
	return status;
}
#endif

#define CWMPPID  "/var/run/cwmp.pid"
#if defined(CONFIG_USER_CWMP_TR069) || defined(APPLY_CHANGE)
void off_tr069(void)
{
	int cwmppid=0;
	int status;

	cwmppid = read_pid((char*)CWMPPID);

	printf("\ncwmppid=%d\n",cwmppid);

	if(cwmppid > 0)
		kill(cwmppid, 15);

#if defined(CONFIG_USER_MONITORD) && defined(CONFIG_USER_CWMP_TR069)
	update_monitor_list_file((char*)CWMP_PROCESS, 0);
#endif

}
#endif

#ifdef WLAN_RTIC_SUPPORT
//RTIC MODE-> ZWDFS: rtic_mode=0; DCS: rtic_mode=1
void rechange_rtic_mode(){
	unsigned char mode =0;
	printf("-----\n");
	if(!mib_get_s(MIB_RTIC_MODE, (void *)&mode, sizeof(mode)))
		printf("<%s>%d: Error! Get MIB_RTIC_MODE error.\n",__FUNCTION__,__LINE__);
	if (mode)
		system("echo 1 > /proc/net/rtic/wlan2/rtic_mode");
	else
		system("echo 0 > /proc/net/rtic/wlan2/rtic_mode");
}
#endif

#ifdef CONFIG_USER_MINIUPNPD
int delDynamicPortForwarding( unsigned int ifindex )
{
#ifdef PORT_FORWARD_GENERAL
	unsigned int upnpItf;
	int total, i;
	MIB_CE_PORT_FW_T entry;

	mib_get(MIB_UPNP_EXT_ITF, (void *)&upnpItf);
	if(ifindex == upnpItf)
	{
		total = mib_chain_total(MIB_PORT_FW_TBL);
		for(i=total-1;i>=0;i--)
		{
			if (!mib_chain_get(MIB_PORT_FW_TBL, i, (void *)&entry))
				continue;
			if (!entry.dynamic)
				continue;
			if(mib_chain_delete(MIB_PORT_FW_TBL, i) != 1)
			{
				printf("Delete MIB_PORT_FW_TBL chain record error!\n");
			}
		}
	}
#endif
	return 0;
}
#endif

#ifdef CONFIG_USER_ROUTED_ROUTED
int delRipTable(unsigned int ifindex)
{
	int total,i;
	MIB_CE_RIP_T Entry;

	total=mib_chain_total(MIB_RIP_TBL);

	for(i=total-1;i>=0;i--)	{
		if(!mib_chain_get(MIB_RIP_TBL,i,&Entry))
			continue;
		if (Entry.ifIndex!=ifindex) continue;
		if(mib_chain_delete(MIB_RIP_TBL,i)!=1){
			printf("Delete MIB_RIP_TBL chain record error!\n");
		}
	}

	return 0;
}
#endif

#ifdef ROUTING
int delRoutingTable( unsigned int ifindex )
{
	int total,i;

	total = mib_chain_total( MIB_IP_ROUTE_TBL );

	for( i=total-1;i>=0;i-- )
	{
		MIB_CE_IP_ROUTE_T *c, entity;
		c = &entity;
		if( !mib_chain_get( MIB_IP_ROUTE_TBL, i, (void*)c ) )
			continue;

#ifdef CONFIG_CU
		unsigned char cu_sd_reverse_route=0;
		mib_get(PROVINCE_CU_SD_REVERSE_STATIC_ROUTE, (void *)&cu_sd_reverse_route);
#ifdef SUPPORT_SET_WANTYPE_FOR_STATIC_ROUTE
		if((cu_sd_reverse_route == 0) || (c->itfTypeEnable == 0))
#endif
#endif
        {
            if(c->ifIndex==ifindex)
                mib_chain_delete( MIB_IP_ROUTE_TBL, i );
        }
	}
	return 0;
}

int updateRoutingTable( unsigned int old_id, unsigned int new_id )
{
	unsigned int total,i;

	total = mib_chain_total( MIB_IP_ROUTE_TBL );
	for( i=0;i<total;i++ )
	{
		MIB_CE_IP_ROUTE_T *c, entity;
		c = &entity;
		if( !mib_chain_get( MIB_IP_ROUTE_TBL, i, (void*)c ) )
			continue;

		if(c->ifIndex==old_id)
		{
			c->ifIndex = new_id;
			mib_chain_update( MIB_IP_ROUTE_TBL, (unsigned char*)c, i );
		}
	}
	return 0;
}

#endif

int delPPPoESession(unsigned int ifindex)
{
	int totalEntry, i;
	MIB_CE_PPPOE_SESSION_T Entry;

	totalEntry = mib_chain_total(MIB_PPPOE_SESSION_TBL);
	for (i = totalEntry - 1; i >= 0; i--) {
		if (!mib_chain_get(MIB_PPPOE_SESSION_TBL, i, &Entry)) {
			printf("Get chain record error!\n");
			continue;
		}

		if (Entry.uifno == ifindex) {
			mib_chain_delete(MIB_PPPOE_SESSION_TBL, i);
		}
	}
	return 0;
}

/*ql:20080926 START: delete MIB_DHCP_CLIENT_OPTION_TBL entry to this pvc*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
int delDhcpcOption( unsigned int ifindex )
{
	MIB_CE_DHCP_OPTION_T entry;
	int i, entrynum;

	entrynum = mib_chain_total(MIB_DHCP_CLIENT_OPTION_TBL);
	for (i=entrynum-1; i>=0; i--)
	{
		if (!mib_chain_get(MIB_DHCP_CLIENT_OPTION_TBL, i, (void *)&entry))
			continue;

		if (entry.ifIndex == ifindex)
			mib_chain_delete(MIB_DHCP_CLIENT_OPTION_TBL, i);
	}
	return 0;
}

void compact_reqoption_order(unsigned int ifIndex)
{
	//int ret=-1;
	int num,i,j;
	int maxorder;
	MIB_CE_DHCP_OPTION_T *p,pentry;
	char *orderflag;

	while(1){
		p=&pentry;
		maxorder=findMaxDHCPReqOptionOrder(ifIndex);
		orderflag=(char*)malloc(maxorder+1);
		if(orderflag==NULL) return;
		memset(orderflag,0,maxorder+1);

		num = mib_chain_total( MIB_DHCP_CLIENT_OPTION_TBL );
		for( i=0; i<num;i++ )
		{

				if( !mib_chain_get( MIB_DHCP_CLIENT_OPTION_TBL, i, (void*)p ))
					continue;
				if(p->usedFor!=eUsedFor_DHCPClient_Req)
					continue;
				if(p->ifIndex != ifIndex)
					continue;
				orderflag[p->order]=1;
		}
		for(j=1;j<=maxorder;j++){
			if(orderflag[j]==0)
				break;
		} //star: there only one 0 in orderflag array
		if(j==(maxorder+1))
		{
			if(orderflag)
			{
				free(orderflag);
				orderflag=NULL;
			}
			break;
		}
		for( i=0; i<num;i++ )
		{

				if( !mib_chain_get( MIB_DHCP_CLIENT_OPTION_TBL, i, (void*)p ))
					continue;
				if(p->usedFor!=eUsedFor_DHCPClient_Req)
					continue;
				if(p->ifIndex != ifIndex)
					continue;
				if(p->order>j){
					(p->order)--;
					mib_chain_update(MIB_DHCP_CLIENT_OPTION_TBL,(void*)p,i);
				}
		}

		if(orderflag)
		{
			free(orderflag);
			orderflag=NULL;
		}
	}

}
#endif

/*ql:20080926 END*/
MIB_CE_ATM_VC_T *getATMVCEntryByIfIndex(unsigned int ifIndex, MIB_CE_ATM_VC_T *p)
{
	int entry_index = -1;

	if(getWanEntrybyIfIndex(ifIndex, p, &entry_index) == 0)
		return p;
	else
		return NULL;
}

int getATMVcEntryByName(char *pIfname, MIB_CE_ATM_VC_T *pEntry)
{
	int entry_index = -1;

	if(getWanEntrybyIfname(pIfname, pEntry, &entry_index) == 0)
		return entry_index;
	else
 		return -1;
}

MIB_CE_ATM_VC_T *getATMVCEntryByIfName(char* ifname, MIB_CE_ATM_VC_T *p)
{
	int entry_index = -1;

	if(getWanEntrybyIfname(ifname, p, &entry_index) == 0)
		return p;
	else
		return NULL;
}

MIB_CE_ATM_VC_T* getDefaultRouteATMVCEntry(MIB_CE_ATM_VC_T *pEntry)
{
	int total,i;

#ifdef DEFAULT_GATEWAY_V2
	{
		unsigned int dgw;
		if (mib_get_s(MIB_ADSL_WAN_DGW_ITF, (void *)&dgw, sizeof(dgw)) != 0)
		{
			return getATMVCEntryByIfIndex(dgw, pEntry);
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
		if(!(pEntry->applicationtype & X_CT_SRV_INTERNET))
			continue;
		if(pEntry->dgw & IPVER_IPV4)
		{
#ifdef _CWMP_MIB_
			if( pEntry->connDisable==0 )
			{
				return pEntry;
			}else
				return NULL;
#else
			return pEntry;
#endif
		}
	}
#endif
	return NULL;
}

#if defined(CONFIG_USER_PORT_ISOLATION)
int checkPortIsolationEntry(void)
{
	int i, totalEntry;
	MIB_CE_PORT_ISOLATION_T portIsolationEntry;
	char port[PMAP_ITF_END];

	memset(port, 0, sizeof(port));

	totalEntry = mib_chain_total(MIB_PORT_ISOLATION_TBL);
	if(totalEntry <= 0){
		for(i = PMAP_ETH0_SW0; i < PMAP_ITF_END; ++i)
		{
			memset(&portIsolationEntry, 0, sizeof(portIsolationEntry));
			portIsolationEntry.port = i;
			mib_chain_add(MIB_PORT_ISOLATION_TBL, &portIsolationEntry);
		}
	}
	else
	{
		for(i = totalEntry-1; i >= 0; i--)
		{
			if(mib_chain_get(MIB_PORT_ISOLATION_TBL, i, (void*)&portIsolationEntry))
			{
				if(portIsolationEntry.port < PMAP_ITF_END && port[portIsolationEntry.port] == 0){
					port[portIsolationEntry.port] = 1;
				}else{
					mib_chain_delete(MIB_PORT_ISOLATION_TBL, i);
				}
			}
		}
		for(i = 0; i < PMAP_ITF_END; i++)
		{
			if(port[i] == 0)
			{
				memset(&portIsolationEntry, 0, sizeof(portIsolationEntry));
				portIsolationEntry.port = i;
				mib_chain_add(MIB_PORT_ISOLATION_TBL, &portIsolationEntry);
				port[i] = 1;
			}
		}

	}
	return 1;
}
#endif

#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
int checkPortBindEntry(void)
{
	int i, totalEntry;
	MIB_CE_PORT_BINDING_T portBindEntry;
	char port[PMAP_ITF_END];

	memset(port, 0, sizeof(port));

	totalEntry = mib_chain_total(MIB_PORT_BINDING_TBL);
	if(totalEntry <= 0){
		for(i = PMAP_ETH0_SW0; i < PMAP_ITF_END; ++i)
		{
			memset(&portBindEntry, 0, sizeof(portBindEntry));
			portBindEntry.port = i;
			mib_chain_add(MIB_PORT_BINDING_TBL, &portBindEntry);
		}
	}
	else
	{
		for(i = totalEntry-1; i >= 0; i--)
		{
			if(mib_chain_get(MIB_PORT_BINDING_TBL, i, (void*)&portBindEntry))
			{
				if(portBindEntry.port < PMAP_ITF_END && port[portBindEntry.port] == 0){
					port[portBindEntry.port] = 1;
				}else{
					mib_chain_delete(MIB_PORT_BINDING_TBL, i);
				}
			}
		}
		for(i = 0; i < PMAP_ITF_END; i++)
		{
			if(port[i] == 0)
			{
				memset(&portBindEntry, 0, sizeof(portBindEntry));
				portBindEntry.port = i;
				mib_chain_add(MIB_PORT_BINDING_TBL, &portBindEntry);
				port[i] = 1;
			}
		}

	}
	return 1;
}

MIB_CE_PORT_BINDING_T* getPortBindEntryByLogPort(int logPort, MIB_CE_PORT_BINDING_T *p, int *mibIdx)
{
	int i, totalEntry;
	MIB_CE_PORT_BINDING_T portBindEntry;

	if(p == NULL) return p;

	totalEntry = mib_chain_total(MIB_PORT_BINDING_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_PORT_BINDING_TBL, i, (void*)&portBindEntry) &&
			logPort == portBindEntry.port)
		{
			memcpy(p, &portBindEntry, sizeof(MIB_CE_PORT_BINDING_T));
			if(mibIdx) *mibIdx = i;
			return p;
		}
	}
	return NULL;
}
#endif

int rtk_util_check_binding_port(int logPort, int filterFalg)
{
	int i, totalEntry;

//check Port binding
	MIB_CE_ATM_VC_T entry;
	if(filterFalg & BINDCHECK_DOMAIN_PORT_BINDING){
		totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
		for(i = 0; i < totalEntry; ++i)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&entry))
					continue;

			if(BIT_IS_SET(entry.itfGroup, logPort)){
				return 1;
			}
		}
	}

//check VLAN binding
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
	MIB_CE_PORT_BINDING_T portBindEntry;

	if(filterFalg & BINDCHECK_DOMAIN_VLAN_BINDING){
		totalEntry = mib_chain_total(MIB_PORT_BINDING_TBL);
		for(i = 0; i < totalEntry; ++i)
		{
			if(mib_chain_get(MIB_PORT_BINDING_TBL, i, (void*)&portBindEntry) &&
				logPort == portBindEntry.port)
			{
#if defined(CONFIG_E8B)
				if(portBindEntry.pb_mode == (unsigned char)VLAN_BASED_MODE){
					return 1;
				}
#endif
			}
		}
	}
#endif

//check VLAN Translate
#ifdef CONFIG_USER_LAN_VLAN_TRANSLATE
	MIB_CE_SW_PORT_T swEntry;
	int idx = 0;
#if defined(WLAN_SUPPORT)
	MIB_CE_MBSSIB_T mbssEntry;
	int wlanIndex;
	int ssidIdx = -1;
#endif

	if(filterFalg & BINDCHECK_DOMAIN_VLAN_TRAN){
		if(logPort < PMAP_WLAN0)
		{
			if(mib_chain_get(MIB_SW_PORT_TBL, logPort, (void*)&swEntry))
			{
				if(swEntry.pb_mode == (unsigned char)LAN_VLAN_BASED_MODE){
					return 1;
				}
			}
		}
#ifdef WLAN_SUPPORT
		else if((ssidIdx = (logPort-PMAP_WLAN0)) < (WLAN_SSID_NUM*NUM_WLAN_INTERFACE) && ssidIdx >= 0){
			if(logPort >=PMAP_WLAN0 && logPort <=PMAP_WLAN0_VAP_END){
				wlanIndex = 0;
				idx = logPort-PMAP_WLAN0;
			}
			else{
				wlanIndex = 1;
				idx = logPort-PMAP_WLAN1;
			}

			if (mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIndex, idx, (void *)&mbssEntry)){
				if(mbssEntry.pb_mode == (unsigned char)LAN_VLAN_BASED_MODE){
					return 1;
				}
			}
		}
#endif
	}
#endif

	return 0;
}

#ifdef _CWMP_MIB_

unsigned int findMaxConDevInstNum(MEDIA_TYPE_T mType)
{
	unsigned int ret=0, i,num;
	MIB_CE_ATM_VC_T *p,vc_entity;

	num = mib_chain_total( MIB_ATM_VC_TBL );
	for( i=0; i<num;i++ )
	{
		p = &vc_entity;
		if( !mib_chain_get( MIB_ATM_VC_TBL, i, (void*)p ))
			continue;
		if (MEDIA_INDEX(p->ifIndex) != mType)
			continue;
		if( p->ConDevInstNum > ret )
			ret = p->ConDevInstNum;
	}

	return ret;
}

unsigned int findConDevInstNumByPVC(unsigned char vpi, unsigned short vci)
{
	unsigned int ret=0, i,num;
	MIB_CE_ATM_VC_T *p,vc_entity;

	if( (vpi==0) && (vci==0) ) return ret;

	num = mib_chain_total( MIB_ATM_VC_TBL );
	for( i=0; i<num;i++ )
	{
		p = &vc_entity;
		if( !mib_chain_get( MIB_ATM_VC_TBL, i, (void*)p ))
			continue;
		if( p->vpi==vpi && p->vci==vci )
		{
			ret = p->ConDevInstNum;
			break;
		}
	}

	return ret;
}

unsigned int findMaxPPPConInstNum(MEDIA_TYPE_T mType, unsigned int condev_inst)
{
	unsigned int ret=0, i,num;
	MIB_CE_ATM_VC_T *p,vc_entity;

	if(condev_inst==0) return ret;

	num = mib_chain_total( MIB_ATM_VC_TBL );
	for( i=0; i<num;i++ )
	{
		p = &vc_entity;
		if( !mib_chain_get( MIB_ATM_VC_TBL, i, (void*)p ))
			continue;
		if (MEDIA_INDEX(p->ifIndex) != mType)
			continue;
		if( p->ConDevInstNum == condev_inst )
		{
			if( (p->cmode==CHANNEL_MODE_PPPOE) ||
#ifdef PPPOE_PASSTHROUGH
			    ((p->cmode==CHANNEL_MODE_BRIDGE)&&(p->brmode==BRIDGE_PPPOE)) ||
#endif
			    (p->cmode==CHANNEL_MODE_PPPOA) )
			{
				if( p->ConPPPInstNum > ret )
					ret = p->ConPPPInstNum;
			}
		}
	}

	return ret;
}

unsigned int findMaxIPConInstNum(MEDIA_TYPE_T mType, unsigned int condev_inst)
{
	unsigned int ret=0, i,num;
	MIB_CE_ATM_VC_T *p,vc_entity;

	if(condev_inst==0) return ret;

	num = mib_chain_total( MIB_ATM_VC_TBL );
	for( i=0; i<num;i++ )
	{
		p = &vc_entity;
		if( !mib_chain_get( MIB_ATM_VC_TBL, i, (void*)p ))
			continue;
		if (MEDIA_INDEX(p->ifIndex) != mType)
			continue;
		if( p->ConDevInstNum==condev_inst )
		{
			if( (p->cmode == CHANNEL_MODE_IPOE) ||
#ifdef PPPOE_PASSTHROUGH
			    ((p->cmode==CHANNEL_MODE_BRIDGE)&&(p->brmode!=BRIDGE_PPPOE)) ||
#else
			    (p->cmode == CHANNEL_MODE_BRIDGE) ||
#endif
			    (p->cmode == CHANNEL_MODE_RT1483) )
			{
				if( p->ConIPInstNum > ret )
					ret = p->ConIPInstNum;
			}
		}
	}

	return ret;
}


/*start use_fun_call_for_wan_instnum*/
int resetWanInstNum(MIB_CE_ATM_VC_Tp entry)
{
	if(entry==NULL) return -1;

	entry->connDisable=0;/*0: always for web/cli*/
	entry->ConDevInstNum=0;
	entry->ConIPInstNum=0;
	entry->ConPPPInstNum=0;
	return 0;
}

int updateWanInstNum(MIB_CE_ATM_VC_Tp entry)
{
	if(entry==NULL) return -1;

#if defined(CWMP_ONE_WAN_CONNDEV_OBJ)
	//always use WANConnectionDevice.1.
	entry->ConDevInstNum = 1;
#else
	if(entry->ConDevInstNum==0)
		entry->ConDevInstNum = 1 + findMaxConDevInstNum(MEDIA_INDEX(entry->ifIndex));
#endif

	if( (entry->cmode==CHANNEL_MODE_PPPOE) ||
#ifdef PPPOE_PASSTHROUGH
	    ((entry->cmode==CHANNEL_MODE_BRIDGE)&&(entry->brmode==BRIDGE_PPPOE)) ||
#endif
	    (entry->cmode==CHANNEL_MODE_PPPOA) )
	{
		if(entry->ConPPPInstNum==0)
			entry->ConPPPInstNum = 1 + findMaxPPPConInstNum(MEDIA_INDEX(entry->ifIndex), entry->ConDevInstNum );

		entry->ConIPInstNum = 0;
	}else{
		entry->ConPPPInstNum = 0;

		if(entry->ConIPInstNum==0)
			entry->ConIPInstNum = 1 + findMaxIPConInstNum(MEDIA_INDEX(entry->ifIndex), entry->ConDevInstNum);

#if defined(CONFIG_00R0)
		/* TR069 WANIPConnection index need always be first */
		if (entry->applicationtype & X_CT_SRV_TR069)
		{
			entry->ConIPInstNum = 1;
		}
		else
		{
			if (entry->ConIPInstNum == 1)
				entry->ConIPInstNum += 1;
		}
#endif
	}

	return 0;
}
/*end use_fun_call_for_wan_instnum*/


/*ping_zhang:20080919 START:add for new telefonica tr069 request: dhcp option*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
unsigned int findMaxDHCPOptionInstNum( unsigned int usedFor, unsigned int dhcpConSPInstNum)
{
	unsigned int ret=0, i,num;
	MIB_CE_DHCP_OPTION_T *p,DHCPOption_entity;

	num = mib_chain_total( MIB_DHCP_SERVER_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPOption_entity;
		if( !mib_chain_get( MIB_DHCP_SERVER_OPTION_TBL, i, (void*)p ))
			continue;
		if(p->usedFor!=usedFor || dhcpConSPInstNum != p->dhcpConSPInstNum)
			continue;
		if(p->dhcpOptInstNum>ret)
			ret=p->dhcpOptInstNum;
	}

	return ret;
}

int getDHCPOptionByOptInstNum( unsigned int dhcpOptNum, unsigned int dhcpSPNum, unsigned int usedFor, MIB_CE_DHCP_OPTION_T *p, unsigned int *id )
{
	int ret=-1;
	unsigned int i,num;

	if( (dhcpOptNum==0) || (p==NULL) || (id==NULL) )
		return ret;

	num = mib_chain_total( MIB_DHCP_SERVER_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_DHCP_SERVER_OPTION_TBL, i, (void*)p ) )
			continue;

		if( (p->usedFor==usedFor) && (p->dhcpOptInstNum==dhcpOptNum) && (p->dhcpConSPInstNum==dhcpSPNum) )
		{
			*id = i;
			ret = 0;
			break;
		}
	}
	return ret;
}

int getDHCPClientOptionByOptInstNum( unsigned int dhcpOptNum, unsigned int ifIndex, unsigned int usedFor, MIB_CE_DHCP_OPTION_T *p, unsigned int *id )
{
	int ret=-1;
	unsigned int i,num;

	if( (dhcpOptNum==0) || (p==NULL) || (id==NULL) )
		return ret;

	num = mib_chain_total( MIB_DHCP_CLIENT_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_DHCP_CLIENT_OPTION_TBL, i, (void*)p ) )
			continue;

		if( (p->usedFor==usedFor) && (p->dhcpOptInstNum==dhcpOptNum) &&(p->ifIndex==ifIndex) )
		{
			*id = i;
			ret = 0;
			break;
		}
	}
	return ret;
}

unsigned int findMaxDHCPClientOptionInstNum(int usedFor, unsigned int ifIndex)
{
	unsigned int ret=0, i,num;
	MIB_CE_DHCP_OPTION_T *p,DHCPOption_entity;

	num = mib_chain_total( MIB_DHCP_CLIENT_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPOption_entity;
		if( !mib_chain_get( MIB_DHCP_CLIENT_OPTION_TBL, i, (void*)p ))
			continue;
		if(p->usedFor!=usedFor||p->ifIndex!=ifIndex)
			continue;
		if(p->dhcpOptInstNum>ret)
			ret=p->dhcpOptInstNum;
	}

	return ret;

}

unsigned int findDHCPOptionNum(int usedFor, unsigned int ifIndex)
{
	unsigned int ret=0, i,num;
	MIB_CE_DHCP_OPTION_T *p,DHCPOption_entity;

	num = mib_chain_total( MIB_DHCP_CLIENT_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPOption_entity;
		if( !mib_chain_get( MIB_DHCP_CLIENT_OPTION_TBL, i, (void*)p ))
			continue;
		if(p->usedFor==usedFor && p->ifIndex==ifIndex)
			ret++;
	}

	return ret;

}

unsigned int findMaxDHCPReqOptionOrder(unsigned int ifIndex)
{
	unsigned int ret=0, i,num;
	MIB_CE_DHCP_OPTION_T *p,DHCPSP_entity;

	num = mib_chain_total( MIB_DHCP_CLIENT_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPSP_entity;
		if( !mib_chain_get( MIB_DHCP_CLIENT_OPTION_TBL, i, (void*)p ))
			continue;
		if(p->usedFor!=eUsedFor_DHCPClient_Req)
			continue;
		if(p->ifIndex != ifIndex)
			continue;
		if(p->order>ret)
			ret=p->order;
	}

	return ret;
}

unsigned int findMaxDHCPConSPInsNum(void )
{
	unsigned int ret=0, i,num;
	DHCPS_SERVING_POOL_T *p,DHCPSP_entity;

	num = mib_chain_total( MIB_DHCPS_SERVING_POOL_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPSP_entity;
		if( !mib_chain_get( MIB_DHCPS_SERVING_POOL_TBL, i, (void*)p ))
			continue;
		if(p->InstanceNum>ret)
			ret=p->InstanceNum;
	}

	return ret;
}

unsigned int findMaxDHCPConSPOrder(void )
{
	unsigned int ret=0, i,num;
	DHCPS_SERVING_POOL_T *p,DHCPSP_entity;

	num = mib_chain_total( MIB_DHCPS_SERVING_POOL_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPSP_entity;
		if( !mib_chain_get( MIB_DHCPS_SERVING_POOL_TBL, i, (void*)p ))
			continue;
		if(p->poolorder>ret)
			ret=p->poolorder;
	}

	return ret;
}

int getDHCPConSPByInstNum( unsigned int dhcpspNum,  DHCPS_SERVING_POOL_T *p, unsigned int *id )
{
	int ret=-1;
	unsigned int i,num;

	if( (dhcpspNum==0) || (p==NULL) || (id==NULL) )
		return ret;

	num = mib_chain_total( MIB_DHCPS_SERVING_POOL_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_DHCPS_SERVING_POOL_TBL, i, (void*)p ) )
			continue;

		if( p->InstanceNum==dhcpspNum)
		{
			*id = i;
			ret = 0;
			break;
		}
	}
	return ret;
}

/*ping_zhang:20090319 START:replace ip range with serving pool of tr069*/
void clearOptTbl(unsigned int instnum)
{
	unsigned int  i,num,found;
	MIB_CE_DHCP_OPTION_T *p,DHCPOption_entity;

delOpt:
	num = mib_chain_total( MIB_DHCP_SERVER_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPOption_entity;
		if( !mib_chain_get( MIB_DHCP_SERVER_OPTION_TBL, i, (void*)p ))
			continue;
		if(p->usedFor!=eUsedFor_DHCPServer_ServingPool)
			continue;
		if(p->dhcpConSPInstNum==instnum){
			mib_chain_delete(MIB_DHCP_SERVER_OPTION_TBL,i);
			break;
		}
	}
	if(i<num)
		goto delOpt;
	return;

}

unsigned int getSPDHCPOptEntryNum(unsigned int usedFor, unsigned int instnum)
{
	unsigned int ret=0, i,num;
	MIB_CE_DHCP_OPTION_T *p,DHCPOPT_entity;

	num = mib_chain_total( MIB_DHCP_SERVER_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPOPT_entity;
		if( !mib_chain_get( MIB_DHCP_SERVER_OPTION_TBL, i, (void*)p ))
			continue;
		if(p->usedFor==usedFor && p->dhcpConSPInstNum==instnum)
			ret++;
	}

	return ret;
}

int getSPDHCPRsvOptEntryByCode(unsigned int instnum, unsigned char optCode, MIB_CE_DHCP_OPTION_T *optEntry ,int *id)
{
	unsigned int ret=-1, i,num;

	if(!optEntry)	return ret;

	num = mib_chain_total( MIB_DHCP_SERVER_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_DHCP_SERVER_OPTION_TBL, i, (void*)optEntry ))
			continue;
		if(optEntry->usedFor==eUsedFor_DHCPServer_ServingPool
			&& optEntry->dhcpConSPInstNum==instnum
			&& optEntry->tag==optCode)
		{
			*id = i;
			ret = 0;
			break;
		}
	}

	return ret;
}

void initSPDHCPOptEntry(DHCPS_SERVING_POOL_T *p)
{
	char origstr[GENERAL_LEN];
	char *origstrDomain=origstr;

	if(!p)
		return;

	memset( p, 0, sizeof( DHCPS_SERVING_POOL_T ) );
	p->enable = 1;
	p->poolorder = findMaxDHCPConSPOrder() + 1;
	strncpy(p->vendorclassmode,"Substring",MODE_LEN-1);
	p->vendorclassmode[MODE_LEN-1]=0;
	p->InstanceNum = findMaxDHCPConSPInsNum() +1;
	p->leasetime=86400;
	p->localserved = 1;//default: locallyserved=true;
	memset(p->chaddrmask,0xff,MAC_ADDR_LEN);//default to all 0xff
	mib_get_s(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)origstrDomain, sizeof(origstr));
	strncpy(p->domainname,origstrDomain,GENERAL_LEN-1);
	p->domainname[GENERAL_LEN-1]=0;
	mib_get_s(MIB_ADSL_LAN_DHCP_GATEWAY, (void *)p->iprouter, sizeof(p->iprouter));
#ifdef DHCPS_POOL_COMPLETE_IP
	mib_get_s(MIB_DHCP_SUBNET_MASK, (void *)p->subnetmask, sizeof(p->subnetmask));
#else
	mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)p->subnetmask, sizeof(p->subnetmask));
#endif
}
/*ping_zhang:20090319 END*/
#endif
/*ping_zhang:20080919 END*/

MIB_CE_ATM_VC_T *getATMVCByInstNum( unsigned int devnum, unsigned int ipnum, unsigned int pppnum, MIB_CE_ATM_VC_T *p, unsigned int *chainid )
{
	unsigned int i,num;

	if( (p==NULL) || (chainid==NULL) ) return NULL;

	num = mib_chain_total( MIB_ATM_VC_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_ATM_VC_TBL, i, (void*)p ))
			continue;

		if( (p->ConDevInstNum==devnum) &&
		    (p->ConIPInstNum==ipnum) &&
		    (p->ConPPPInstNum==pppnum) )
		{
			*chainid=i;
			return p;
		}
	}

	return NULL;
}

#ifdef _PRMT_TR143_
static const char gUDPEchoServerName[] = "/bin/udpechoserver";
static const char gUDPEchoServerPid[] = "/var/run/udpechoserver.pid";

int clearWanUDPEchoItf( void )
{
	int total,i;
	MIB_CE_ATM_VC_T *pEntry, vc_entity;

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for( i=0; i<total; i++ )
	{
		pEntry = &vc_entity;
		if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)pEntry ) )
			continue;

		if(pEntry->TR143UDPEchoItf)
		{
			pEntry->TR143UDPEchoItf=0;
			mib_chain_update( MIB_ATM_VC_TBL, (unsigned char*)pEntry, i );
		}
	}
	return 0;
}

int setWanUDPEchoItf( unsigned int ifindex )
{
	int total,i;
	MIB_CE_ATM_VC_T *pEntry, vc_entity;

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for( i=0; i<total; i++ )
	{
		pEntry = &vc_entity;
		if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)pEntry ) )
			continue;

		if(pEntry->ifIndex==ifindex)
		{
			pEntry->TR143UDPEchoItf=1;
			mib_chain_update( MIB_ATM_VC_TBL, (unsigned char*)pEntry, i );
			return 0;
		}
	}
	return -1;
}

int getWanUDPEchoItf( unsigned int *pifindex)
{
	int total,i;
	MIB_CE_ATM_VC_T *pEntry, vc_entity;

	if(pifindex==NULL) return -1;
	*pifindex=DUMMY_IFINDEX;

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for( i=0; i<total; i++ )
	{
		pEntry = &vc_entity;
		if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)pEntry ) )
			continue;
		if(pEntry->TR143UDPEchoItf)
		{
			*pifindex=pEntry->ifIndex;
			return 0;
		}
	}
	return -1;
}

void UDPEchoConfigSave(struct TR143_UDPEchoConfig *p)
{
	if(p)
	{
		unsigned char itftype;
		mib_get_s( TR143_UDPECHO_ENABLE, (void *)&p->Enable, sizeof(p->Enable) );
		mib_get_s( TR143_UDPECHO_SRCIP, (void *)p->SourceIPAddress, sizeof(p->SourceIPAddress) );
		mib_get_s( TR143_UDPECHO_PORT, (void *)&p->UDPPort, sizeof(p->UDPPort) );
		mib_get_s( TR143_UDPECHO_PLUS, (void *)&p->EchoPlusEnabled, sizeof(p->EchoPlusEnabled) );

		mib_get_s( TR143_UDPECHO_ITFTYPE, (void *)&itftype, sizeof(itftype) );
		if(itftype==ITF_WAN)
		{
			int total,i;
			MIB_CE_ATM_VC_T *pEntry, vc_entity;

			p->Interface[0]=0;
			total = mib_chain_total(MIB_ATM_VC_TBL);
			for( i=0; i<total; i++ )
			{
				pEntry = &vc_entity;
				if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)pEntry ) )
					continue;
				if(pEntry->TR143UDPEchoItf)
				{
					ifGetName(pEntry->ifIndex, p->Interface, sizeof(p->Interface));
				}
			}
		}else if(itftype<ITF_END)
		{
			p->Interface[sizeof(p->Interface)-1]='\0';
			strncpy( p->Interface, strItf[itftype], sizeof(p->Interface)-1);
			LANDEVNAME2BR0(p->Interface);
		}else
			p->Interface[0]=0;

	}
	return;
}

int UDPEchoConfigStart( struct TR143_UDPEchoConfig *p )
{
	if(!p) return -1;

	if( p->Enable )
	{
		char strPort[16];
		char *argv[10];
		int  i;

		if(p->UDPPort==0)
		{
			fprintf( stderr, "UDPEchoConfigStart> error p->UDPPort=0\n" );
			return -1;
		}
		sprintf( strPort, "%u", p->UDPPort );
		va_cmd(IPTABLES, 12, 1, ARG_T, "mangle", FW_ADD, (char *)FW_PREROUTING, "-p", (char *)ARG_UDP,
			(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);

		va_cmd(IP6TABLES, 8, 1, FW_ADD, (char *)FW_INPUT, "-p", (char *)ARG_UDP, (char *)FW_DPORT, strPort, "-j", (char *)FW_ACCEPT);

		i=0;
		argv[i]=(char *)gUDPEchoServerName;
		i++;
		argv[i]="-port";
		i++;
		argv[i]=strPort;
		i++;
		if( strlen(p->Interface) > 0 )
		{
			argv[i]="-i";
			i++;
			argv[i]=p->Interface;
			i++;
		}
		if( p->SourceIPAddress[0] )
		{
			argv[i]="-addr";
			i++;
			argv[i]=p->SourceIPAddress;
			i++;
		}
		if( p->EchoPlusEnabled )
		{
			argv[i]="-plus";
			i++;
		}

		argv[i]=NULL;
		do_nice_cmd( gUDPEchoServerName, argv, 0 );

	}

	return 0;
}

int UDPEchoConfigStop( struct TR143_UDPEchoConfig *p )
{
	char strPort[16];
	int pid;
	int status;

	if(!p) return -1;

	sprintf( strPort, "%u", p->UDPPort );
	va_cmd(IPTABLES, 12, 1, ARG_T, "mangle", FW_DEL, (char *)FW_PREROUTING,	"-p", (char *)ARG_UDP,
		(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);

	va_cmd(IP6TABLES, 8, 1, FW_ADD, (char *)FW_INPUT, "-p", (char *)ARG_UDP, (char *)FW_DPORT, strPort, "-j", (char *)FW_ACCEPT);


	pid = read_pid((char *)gUDPEchoServerPid);
	if (pid >= 1)
	{
		status = kill(pid, SIGTERM);
		if (status != 0)
		{
			printf("Could not kill UDPEchoServer's pid '%d'\n", pid);
			return -1;
		}
	}

	return 0;
}
#endif //_PRMT_TR143_

#endif //_CWMP_MIB_

#ifdef CONFIG_CWMP_TR181_SUPPORT
#ifdef CONFIG_TELMEX_DEV
unsigned int find_valid_BridgingBridge_instBit(void)
{
	int i = 0, total = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_TBL);
	MIB_TR181_BRIDGING_BRIDGE_T entry;
	unsigned int bitMask = 0;	//max count is 8
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_TBL, i, &entry) == 0)
			continue;

		bitMask |= (0x1<<(entry.instnum-1));
	}

	for(i = 0; i < 31; i++)
	{
		if(0==(bitMask&(0x1<<i)))
		{
			ret = i;
			break;
		}
	}

	return ret;
}

unsigned int find_valid_BridgingBridgePort_instBit(unsigned int br_instnum)
{
	int i = 0, total = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL);
	MIB_TR181_BRIDGING_BRIDGE_PORT_T entry;
	unsigned int bitMask[4] = {0};	//max count is 64
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL, i, &entry) == 0)
			continue;

		if (entry.bridge_instnum == br_instnum)
		{
			bitMask[(entry.instnum-1)>>5] |= (0x1<<((entry.instnum-1)&0x1F));
		}
	}

	for (i=0; i<128; i++)
	{
		if (0==(bitMask[i>>5] & (1<<(i&0x1F))))
		{
			ret = i;
			break;
		}
	}

	return ret;
}

unsigned int find_valid_BridgingBridgeVLAN_instBit(unsigned int br_instnum)
{
	int i = 0, total = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_VLAN_TBL);
	MIB_TR181_BRIDGING_BRIDGE_VLAN_T entry;
	unsigned int bitMask = 0;	//max count is 16
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_VLAN_TBL, i, &entry) == 0)
			continue;

		if (entry.bridge_instnum == br_instnum)
		{
			bitMask |= (0x1<<(entry.instnum-1));
		}
	}

	for(i = 0; i < 31; i++)
	{
		if(0==(bitMask&(0x1<<i)))
		{
			ret = i;
			break;
		}
	}

	return ret;
}

unsigned int find_valid_BridgingBridgeVLANPort_instBit(unsigned int br_instnum)
{
	int i = 0, total = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_VLANPORT_TBL);
	MIB_TR181_BRIDGING_BRIDGE_VLANPORT_T entry;
	unsigned int bitMask[4] = {0};	//max count is 64
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_VLANPORT_TBL, i, &entry) == 0)
			continue;

		if (entry.bridge_instnum == br_instnum)
		{
			bitMask[(entry.instnum-1)>>5] |= (0x1<<((entry.instnum-1)&0x1F));
		}
	}

	for (i=0; i<128; i++)
	{
		if (0==(bitMask[i>>5] & (1<<(i&0x1F))))
		{
			ret = i;
			break;
		}
	}

	return ret;
}

unsigned int find_valid_EthernetLink_instBit(void)
{
	int i = 0, total = mib_chain_total(MIB_TR181_ETHERNET_LINK_TBL);
	MIB_TR181_ETHERNET_LINK_T entry;
	unsigned int bitMask = 1;	//first one is br0(LAN), and max count is 1+8
	unsigned int ret = 0;

	if(!SW_LAN_PORT_NUM)
		bitMask = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_ETHERNET_LINK_TBL, i, &entry) == 0)
			continue;

		bitMask |= (0x1<<(entry.instnum-1));
	}

	for(i = 0; i < 31; i++)
	{
		if(0==(bitMask&(0x1<<i)))
		{
			ret = i;
			break;
		}
	}

	return ret;
}

unsigned int find_valid_EthernetVLANTermination_instBit(void)
{
	int i = 0, total = mib_chain_total(MIB_TR181_ETHERNET_VLANTERMINATION_TBL);
	MIB_TR181_ETHERNET_VLANTERMINATION_T entry;
	unsigned int bitMask = 0;	//max count is 8
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_ETHERNET_VLANTERMINATION_TBL, i, &entry) == 0)
			continue;

		bitMask |= (0x1<<(entry.instnum-1));
	}

	for(i = 0; i < 31; i++)
	{
		if(0==(bitMask&(0x1<<i)))
		{
			ret = i;
			break;
		}
	}

	return ret;
}

unsigned int find_valid_PPP_Interface_instBit(void)
{
	int i = 0, total = mib_chain_total(MIB_TR181_PPP_INTERFACE_TBL);
	MIB_TR181_PPP_INTERFACE_T entry;
	unsigned int bitMask = 0;	//max count is 8
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_PPP_INTERFACE_TBL, i, &entry) == 0)
			continue;

		bitMask |= (0x1<<(entry.instnum-1));
	}

	for(i = 0; i < 31; i++)
	{
		if(0==(bitMask&(0x1<<i)))
		{
			ret = i;
			break;
		}
	}

	return ret;
}

unsigned int find_valid_IP_Interface_instBit(void)
{
	int i = 0, total = mib_chain_total(MIB_TR181_IP_INTERFACE_TBL);
	MIB_TR181_IP_INTERFACE_T entry;
	unsigned int bitMask = 1;	//first one is br0(LAN), and max count is 1+8
	unsigned int ret = 0; // first one is br0(LAN)

	if(!SW_LAN_PORT_NUM)
		bitMask = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_IP_INTERFACE_TBL, i, &entry) == 0)
			continue;

		bitMask |= (0x1<<(entry.instnum-1));
	}

	printf("[%s %d]bitMask=0x%x\n", __func__, __LINE__, bitMask);
	for(i = 0; i < 31; i++)
	{
		if(0==(bitMask&(0x1<<i)))
		{
			ret = i;
			break;
		}
	}

	return ret;
}
#endif

unsigned int find_max_BridgingBridge_instNum(void)
{
	int i = 0, total = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_TBL);
	MIB_TR181_BRIDGING_BRIDGE_T entry;
	unsigned int ret = 1; // first one is br0(LAN)

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_TBL, i, &entry) == 0)
			continue;

		if (entry.instnum > ret)
			ret = entry.instnum;
	}

	return ret;
}

unsigned int find_max_BridgingBridgePort_instNum(unsigned int br_instnum)
{
	int i = 0, total = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL);
	MIB_TR181_BRIDGING_BRIDGE_PORT_T entry;
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL, i, &entry) == 0)
			continue;

		if (entry.bridge_instnum == br_instnum)
		{
			if (entry.instnum > ret)
				ret = entry.instnum;
		}
	}

	return ret;
}

unsigned int find_max_BridgingBridgeVLAN_instNum(unsigned int br_instnum)
{
	int i = 0, total = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_VLAN_TBL);
	MIB_TR181_BRIDGING_BRIDGE_VLAN_T entry;
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_VLAN_TBL, i, &entry) == 0)
			continue;

		if (entry.bridge_instnum == br_instnum)
		{
			if (entry.instnum > ret)
				ret = entry.instnum;
		}
	}

	return ret;
}

unsigned int find_max_BridgingBridgeVLANPort_instNum(unsigned int br_instnum)
{
	int i = 0, total = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_VLANPORT_TBL);
	MIB_TR181_BRIDGING_BRIDGE_VLANPORT_T entry;
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_VLANPORT_TBL, i, &entry) == 0)
			continue;

		if (entry.bridge_instnum == br_instnum)
		{
			if (entry.instnum > ret)
				ret = entry.instnum;
		}
	}

	return ret;
}

unsigned int find_max_ATMLink_instNum(void)
{
	int i = 0, total = mib_chain_total(MIB_TR181_ATM_LINK_TBL);
	MIB_TR181_ATM_LINK_T entry;
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_ATM_LINK_TBL, i, &entry) == 0)
			continue;

		if (entry.instnum > ret)
			ret = entry.instnum;
	}

	return ret;
}

unsigned int find_max_PTMLink_instNum(void)
{
	int i = 0, total = mib_chain_total(MIB_TR181_PTM_LINK_TBL);
	MIB_TR181_PTM_LINK_T entry;
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_PTM_LINK_TBL, i, &entry) == 0)
			continue;

		if (entry.instnum > ret)
			ret = entry.instnum;
	}

	return ret;
}

unsigned int find_max_EthernetLink_instNum(void)
{
	int i = 0, total = mib_chain_total(MIB_TR181_ETHERNET_LINK_TBL);
	MIB_TR181_ETHERNET_LINK_T entry;
	unsigned int ret = 1; // first one is br0(LAN)

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_ETHERNET_LINK_TBL, i, &entry) == 0)
			continue;

		if (entry.instnum > ret)
			ret = entry.instnum;
	}

	return ret;
}

unsigned int find_max_EthernetVLANTermination_instNum(void)
{
	int i = 0, total = mib_chain_total(MIB_TR181_ETHERNET_VLANTERMINATION_TBL);
	MIB_TR181_ETHERNET_VLANTERMINATION_T entry;
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_ETHERNET_VLANTERMINATION_TBL, i, &entry) == 0)
			continue;

		if (entry.instnum > ret)
			ret = entry.instnum;
	}

	return ret;
}

unsigned int find_max_PPP_Interface_instNum(void)
{
	int i = 0, total = mib_chain_total(MIB_TR181_PPP_INTERFACE_TBL);
	MIB_TR181_PPP_INTERFACE_T entry;
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_PPP_INTERFACE_TBL, i, &entry) == 0)
			continue;

		if (entry.instnum > ret)
			ret = entry.instnum;
	}

	return ret;
}

unsigned int find_max_IP_Interface_instNum(void)
{
	int i = 0, total = mib_chain_total(MIB_TR181_IP_INTERFACE_TBL);
	MIB_TR181_IP_INTERFACE_T entry;
	unsigned int ret = 1; // first one is br0(LAN)

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_IP_INTERFACE_TBL, i, &entry) == 0)
			continue;

		if (entry.instnum > ret)
			ret = entry.instnum;
	}

	return ret;
}

unsigned int find_max_IP_Interface_IPv6Address_instNum(unsigned int ip_instnum)
{
	int i = 0, total = mib_chain_total(MIB_TR181_IP_INTF_IPV6ADDR_TBL);
	MIB_TR181_IP_INTF_IPV6ADDR_T entry;
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_IP_INTF_IPV6ADDR_TBL, i, &entry) == 0)
			continue;

		if (entry.ParentInstnum == ip_instnum)
		{
			if (entry.instnum > ret)
				ret = entry.instnum;
		}
	}

	return ret;
}

unsigned int find_max_IP_Interface_IPv6Prefix_instNum(unsigned int ip_instnum)
{
	int i = 0, total = mib_chain_total(MIB_TR181_IP_INTF_IPV6PREFIX_TBL);
	MIB_TR181_IP_INTF_IPV6PREFIX_T entry;
	unsigned int ret = 0;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_TR181_IP_INTF_IPV6PREFIX_TBL, i, &entry) == 0)
			continue;

		if (entry.ParentInstnum == ip_instnum)
		{
			if (entry.instnum > ret)
				ret = entry.instnum;
		}
	}

	return ret;
}

int get_ATM_Link_EntryByInstNum(unsigned int instnum, MIB_TR181_ATM_LINK_T *p, int *chainid)
{
	int ret = 0, i = 0, num = 0;

	if ((instnum == 0) || (p == NULL) || (chainid == NULL))
		return ret;

	num = mib_chain_total(MIB_TR181_ATM_LINK_TBL);
	for (i = 0; i < num; i++)
	{
		if (mib_chain_get(MIB_TR181_ATM_LINK_TBL, i, (void *)p) == 0)
			continue;

		if (p->instnum == instnum)
		{
			*chainid = i;
			ret = 1;
			break;
		}
	}

	return ret;
}

int get_PTM_Link_EntryByInstNum(unsigned int instnum, MIB_TR181_PTM_LINK_T *p, int *chainid)
{
	int ret = 0, i = 0, num = 0;

	if ((instnum == 0) || (p == NULL) || (chainid == NULL))
		return ret;

	num = mib_chain_total(MIB_TR181_PTM_LINK_TBL);
	for (i = 0; i < num; i++)
	{
		if (mib_chain_get(MIB_TR181_PTM_LINK_TBL, i, (void *)p) == 0)
			continue;

		if (p->instnum == instnum)
		{
			*chainid = i;
			ret = 1;
			break;
		}
	}

	return ret;
}

int get_Ethernet_Interface_EntryByInstNum(unsigned int instnum, MIB_TR181_ETHERNET_INTERFACE_T *p, int *chainid)
{
	int ret = 0, i = 0, num = 0;

	if ((instnum == 0) || (p == NULL) || (chainid == NULL))
		return ret;

	num = mib_chain_total(MIB_TR181_ETHERNET_INTERFACE_TBL);
	for (i = 0; i < num; i++)
	{
		if (mib_chain_get(MIB_TR181_ETHERNET_INTERFACE_TBL, i, (void *)p) == 0)
			continue;

		if (p->instnum == instnum)
		{
			*chainid = i;
			ret = 1;
			break;
		}
	}

	return ret;
}

int get_Bridging_Bridge_EntryByInstNum(unsigned int bridge_instnum, MIB_TR181_BRIDGING_BRIDGE_T *p, int *chainid)
{
	int ret = 0, i = 0, num = 0;

	if ((bridge_instnum == 0) || (p == NULL) || (chainid == NULL))
		return ret;

	num = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_TBL);
	for (i = 0; i < num; i++)
	{
		if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_TBL, i, (void *)p) == 0)
			continue;

		if ((p->instnum == bridge_instnum))
		{
			*chainid = i;
			ret = 1;
			break;
		}
	}

	return ret;
}

int get_BridgingBridgePort_EntryByInstNum(unsigned int bridge_instnum, unsigned int port_instnum, MIB_TR181_BRIDGING_BRIDGE_PORT_T *p, int *chainid)
{
	int ret = 0, i = 0, num = 0;

	if ((bridge_instnum == 0) || (port_instnum == 0) || (p == NULL) || (chainid == NULL))
		return ret;

	num = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL);
	for (i = 0; i < num; i++)
	{
		if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL, i, (void *)p) == 0)
			continue;

		if ((p->bridge_instnum == bridge_instnum) && (p->instnum == port_instnum))
		{
			*chainid = i;
			ret = 1;
			break;
		}
	}

	return ret;
}

int get_Ether_Link_EntryByInstNum(unsigned int instnum, MIB_TR181_ETHERNET_LINK_T *p, int *chainid)
{
	int ret = 0, i = 0, num = 0;

	if ((instnum == 0) || (p == NULL) || (chainid == NULL))
		return ret;

	num = mib_chain_total(MIB_TR181_ETHERNET_LINK_TBL);
	for (i = 0; i < num; i++)
	{
		if (mib_chain_get(MIB_TR181_ETHERNET_LINK_TBL, i, (void *)p) == 0)
			continue;

		if ((p->instnum == instnum))
		{
			*chainid = i;
			ret = 1;
			break;
		}
	}

	return ret;
}

int get_Ethernet_VLANTermination_EntryByInstNum(unsigned int instnum, MIB_TR181_ETHERNET_VLANTERMINATION_T *p, int *chainid)
{
	int ret = 0, i = 0, num = 0;

	if ((instnum == 0) || (p == NULL) || (chainid == NULL))
		return ret;

	num = mib_chain_total(MIB_TR181_ETHERNET_VLANTERMINATION_TBL);
	for (i = 0; i < num; i++)
	{
		if (mib_chain_get(MIB_TR181_ETHERNET_VLANTERMINATION_TBL, i, (void *)p) == 0)
			continue;

		if (p->instnum == instnum)
		{
			*chainid = i;
			ret = 1;
			break;
		}
	}

	return ret;
}

int get_PPP_Interface_EntryByInstNum(unsigned int instnum, MIB_TR181_PPP_INTERFACE_T *p, int *chainid)
{
	int ret = 0, i = 0, num = 0;

	if ((instnum == 0) || (p == NULL) || (chainid == NULL))
		return ret;

	num = mib_chain_total(MIB_TR181_PPP_INTERFACE_TBL);
	for (i = 0; i < num; i++)
	{
		if (mib_chain_get(MIB_TR181_PPP_INTERFACE_TBL, i, (void *)p) == 0)
			continue;

		if (p->instnum == instnum)
		{
			*chainid = i;
			ret = 1;
			break;
		}
	}

	return ret;
}

int get_IP_Interface_EntryByInstNum(unsigned int instnum, MIB_TR181_IP_INTERFACE_T *p, int *chainid)
{
	int ret = 0, i = 0, num = 0;

	if ((instnum == 0) || (p == NULL) || (chainid == NULL))
		return ret;

	num = mib_chain_total(MIB_TR181_IP_INTERFACE_TBL);
	for (i = 0; i < num; i++)
	{
		if (mib_chain_get(MIB_TR181_IP_INTERFACE_TBL, i, (void *)p) == 0)
			continue;

		if (p->instnum == instnum)
		{
			*chainid = i;
			ret = 1;
			break;
		}
	}

	return ret;
}

int get_IP_Interface_IPv6Address_EntryByInstNum(unsigned int ip_instnum, unsigned int instnum, MIB_TR181_IP_INTF_IPV6ADDR_T *p, int *chainid)
{
	int ret = 0, i = 0, num = 0;

	if ((ip_instnum == 0) || (instnum == 0) || (p == NULL) || (chainid == NULL))
		return ret;

	num = mib_chain_total(MIB_TR181_IP_INTF_IPV6ADDR_TBL);
	for (i = 0; i < num; i++)
	{
		if (mib_chain_get(MIB_TR181_IP_INTF_IPV6ADDR_TBL, i, (void *)p) == 0)
			continue;

		if (p->ParentInstnum == ip_instnum && p->instnum == instnum)
		{
			*chainid = i;
			ret = 1;
			break;
		}
	}

	return ret;
}

int get_IP_Interface_IPv6Address_EntryByAddr(unsigned int ip_instnum, char *addr, MIB_TR181_IP_INTF_IPV6ADDR_T *p, int *chainid)
{
	int ret = 0, i = 0, num = 0;

	if ((ip_instnum == 0) || (p == NULL) || (chainid == NULL))
		return ret;

	num = mib_chain_total(MIB_TR181_IP_INTF_IPV6ADDR_TBL);
	for (i = 0; i < num; i++)
	{
		if (mib_chain_get(MIB_TR181_IP_INTF_IPV6ADDR_TBL, i, (void *)p) == 0)
			continue;

		if (p->ParentInstnum == ip_instnum && memcmp(addr, p->IPAddress, IP6_ADDR_LEN) == 0)
		{
			*chainid = i;
			ret = 1;
			break;
		}
	}

	return ret;
}

int get_IP_Interface_IPv6Prefix_EntryByInstNum(unsigned int ip_instnum, unsigned int instnum, MIB_TR181_IP_INTF_IPV6PREFIX_T *p, int *chainid)
{
	int ret = 0, i = 0, num = 0;

	if ((ip_instnum == 0) || (instnum == 0) || (p == NULL) || (chainid == NULL))
		return ret;

	num = mib_chain_total(MIB_TR181_IP_INTF_IPV6PREFIX_TBL);
	for (i = 0; i < num; i++)
	{
		if (mib_chain_get(MIB_TR181_IP_INTF_IPV6PREFIX_TBL, i, (void *)p) == 0)
			continue;

		if (p->ParentInstnum == ip_instnum && p->instnum == instnum)
		{
			*chainid = i;
			ret = 1;
			break;
		}
	}

	return ret;
}

int get_IP_Interface_IPv6Prefix_EntryByPrefix(unsigned int ip_instnum, char *prefix, MIB_TR181_IP_INTF_IPV6PREFIX_T *p, int *chainid)
{
	int ret = 0, i = 0, num = 0;

	if ((ip_instnum == 0) || (p == NULL) || (chainid == NULL))
		return ret;

	num = mib_chain_total(MIB_TR181_IP_INTF_IPV6PREFIX_TBL);
	for (i = 0; i < num; i++)
	{
		if (mib_chain_get(MIB_TR181_IP_INTF_IPV6PREFIX_TBL, i, (void *)p) == 0)
			continue;

		if (p->ParentInstnum == ip_instnum && memcmp(prefix, p->Prefix, IP6_ADDR_LEN) == 0)
		{
			*chainid = i;
			ret = 1;
			break;
		}
	}

	return ret;
}

int rtk_tr181_init_interfacestack_table(int table_id)
{
	int i = 0, num = 0;
	switch (table_id)
	{
		case MIB_TR181_ETHERNET_INTERFACE_TBL:
		{
			num = mib_chain_total(MIB_TR181_ETHERNET_INTERFACE_TBL);
			if (num == 0)
			{
				MIB_CE_SW_PORT_T SW_PORT_Entry;
				MIB_TR181_ETHERNET_INTERFACE_T ETHERNET_INTERFACE_Entry;

				for (i = 0; i < SW_LAN_PORT_NUM; i++)
				{
					if (!mib_chain_get(MIB_SW_PORT_TBL, i, &SW_PORT_Entry))
						continue;

					memset(&ETHERNET_INTERFACE_Entry, 0, sizeof(MIB_TR181_ETHERNET_INTERFACE_T));
					ETHERNET_INTERFACE_Entry.Enable = 1;
					ETHERNET_INTERFACE_Entry.MaxBitRate = SW_PORT_Entry.speed;
					ETHERNET_INTERFACE_Entry.DuplexMode = SW_PORT_Entry.duplex;
					ETHERNET_INTERFACE_Entry.instnum = (i + 1);
					mib_chain_add(MIB_TR181_ETHERNET_INTERFACE_TBL, (void *)&ETHERNET_INTERFACE_Entry);
				}

#ifdef CONFIG_ETHWAN
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE) || defined(CONFIG_FIBER_FEATURE)
			// Optical.Interface
#else
				memset(&ETHERNET_INTERFACE_Entry, 0, sizeof(MIB_TR181_ETHERNET_INTERFACE_T));
				ETHERNET_INTERFACE_Entry.Enable = 1;
				ETHERNET_INTERFACE_Entry.MaxBitRate = (-1);
				ETHERNET_INTERFACE_Entry.DuplexMode = DUPLEX_TYPE_AUTO;
				ETHERNET_INTERFACE_Entry.instnum = (SW_LAN_PORT_NUM + 1);
				mib_chain_add(MIB_TR181_ETHERNET_INTERFACE_TBL, (void *)&ETHERNET_INTERFACE_Entry);
#endif
#endif
			}
			break;
		}
		default:
			return 0;
	}
	return 1;
}

int remove_BridgingBridge_related_Tables_ByInstNum(unsigned int Instnum)
{
	int i = 0, ret = 0, bridge_port_num = 0, bridge_vlan_num = 0, bridge_vlanport_num = 0;
	MIB_TR181_BRIDGING_BRIDGE_PORT_T bridge_port_entry;
	MIB_TR181_BRIDGING_BRIDGE_VLAN_T bridge_vlan_entry;
	MIB_TR181_BRIDGING_BRIDGE_VLANPORT_T bridge_vlanport_entry;

	bridge_port_num = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL);
	bridge_vlan_num = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_VLAN_TBL);
	bridge_vlanport_num = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_VLANPORT_TBL);

	for (i = (bridge_port_num - 1); i >= 0; i--)
	{
		if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL, i, &bridge_port_entry) == 0)
			continue;

		if (bridge_port_entry.bridge_instnum == Instnum)
		{
			mib_chain_delete(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL, i);
		}
	}

	for (i = (bridge_vlan_num - 1); i >= 0; i--)
	{
		if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_VLAN_TBL, i, &bridge_vlan_entry) == 0)
			continue;

		if (bridge_vlan_entry.bridge_instnum == Instnum)
		{
			mib_chain_delete(MIB_TR181_BRIDGING_BRIDGE_VLAN_TBL, i);
		}
	}

	for (i = (bridge_vlanport_num - 1); i >= 0; i--)
	{
		if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_VLANPORT_TBL, i, &bridge_vlanport_entry) == 0)
			continue;

		if (bridge_vlanport_entry.bridge_instnum == Instnum)
		{
			mib_chain_delete(MIB_TR181_BRIDGING_BRIDGE_VLANPORT_TBL, i);
		}
	}

	return ret;
}

int rtk_tr181_get_wan_interfacestack_by_entry(MIB_CE_ATM_VC_T *pWAN_Entry, TR181_INTERFACE_STACK_T *pInterfaceStack)
{

	if (pWAN_Entry == NULL || pInterfaceStack == NULL)
		return 0;

	if (MEDIA_INDEX(pWAN_Entry->ifIndex) == MEDIA_ATM)
	{
		if (pWAN_Entry->ATM_Link_instnum > 0)
		{
			pInterfaceStack->ATM_Link_instnum = pWAN_Entry->ATM_Link_instnum;
		}
		else
		{
			pInterfaceStack->ATM_Link_instnum = (find_max_ATMLink_instNum() + 1);
		}
	}
	else if (MEDIA_INDEX(pWAN_Entry->ifIndex) == MEDIA_PTM)
	{
		if (pWAN_Entry->PTM_Link_instnum > 0)
		{
			pInterfaceStack->PTM_Link_instnum = pWAN_Entry->PTM_Link_instnum;
		}
		else
		{
			pInterfaceStack->PTM_Link_instnum = (find_max_PTMLink_instNum() + 1);
		}
	}
#ifdef CONFIG_ETHWAN
	else
	{
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE) || defined(CONFIG_FIBER_FEATURE)
		pInterfaceStack->Optical_Interface_instnum = 1;
#else
		pInterfaceStack->Ethernet_Interface_instnum = (SW_LAN_PORT_NUM + 1);
#endif
	}
#endif

	if ((CHANNEL_MODE_T)pWAN_Entry->cmode == CHANNEL_MODE_BRIDGE)
	{
		if (pWAN_Entry->Bridging_Bridge_instnum > 0)
		{
			pInterfaceStack->Bridging_Bridge_instnum = pWAN_Entry->Bridging_Bridge_instnum;

			if (pWAN_Entry->Bridging_Bridge_Port_instnum > 0)
			{
				pInterfaceStack->Bridging_Bridge_Port_instnum = pWAN_Entry->Bridging_Bridge_Port_instnum;
			}
		}
		else
		{
			pInterfaceStack->Bridging_Bridge_instnum = (find_max_BridgingBridge_instNum() + 1);
		}

		if (pInterfaceStack->Bridging_Bridge_Port_instnum == 0)
		{
			pInterfaceStack->Bridging_Bridge_Port_instnum = (find_max_BridgingBridgePort_instNum(pInterfaceStack->Bridging_Bridge_instnum) + 1);
		}
	}
	else
	{
		if (pWAN_Entry->Ethernet_Link_instnum > 0)
		{
			pInterfaceStack->Ethernet_Link_instnum = pWAN_Entry->Ethernet_Link_instnum;
		}
		else
		{
			pInterfaceStack->Ethernet_Link_instnum = (find_max_EthernetLink_instNum() + 1);
		}

		if (pWAN_Entry->vlan && pWAN_Entry->vid > 0)
		{
			if (pWAN_Entry->Ethernet_VLANTermination_instnum > 0)
			{
				pInterfaceStack->Ethernet_VLANTermination_instnum = pWAN_Entry->Ethernet_VLANTermination_instnum;
			}
			else
			{
				pInterfaceStack->Ethernet_VLANTermination_instnum = (find_max_EthernetVLANTermination_instNum() + 1);
			}
		}

		if ((CHANNEL_MODE_T)pWAN_Entry->cmode == CHANNEL_MODE_IPOE || (CHANNEL_MODE_T)pWAN_Entry->cmode == CHANNEL_MODE_PPPOE
			|| (CHANNEL_MODE_T)pWAN_Entry->cmode == CHANNEL_MODE_RT1483 || (CHANNEL_MODE_T)pWAN_Entry->cmode == CHANNEL_MODE_PPPOA)
		{
			if (pWAN_Entry->IP_Interface_instnum > 0)
			{
				pInterfaceStack->IP_Interface_instnum = pWAN_Entry->IP_Interface_instnum;
			}
			else
			{
				pInterfaceStack->IP_Interface_instnum = (find_max_IP_Interface_instNum() + 1);
			}
		}

		if ((CHANNEL_MODE_T)pWAN_Entry->cmode == CHANNEL_MODE_PPPOE || (CHANNEL_MODE_T)pWAN_Entry->cmode == CHANNEL_MODE_PPPOA)
		{
			if (pWAN_Entry->PPP_Interface_instnum > 0)
			{
				pInterfaceStack->PPP_Interface_instnum = pWAN_Entry->PPP_Interface_instnum;
			}
			else
			{
				pInterfaceStack->PPP_Interface_instnum = (find_max_PPP_Interface_instNum() + 1);
			}
		}
	}

	return 1;
}

int rtk_tr181_set_table_by_wan_entry(int table_id, void *ptr, MIB_CE_ATM_VC_Tp pEntry)
{
	char LowerLayers[1024+1] = {0};

	if (ptr == NULL || pEntry == NULL)
		return 0;

	switch (table_id)
	{
		case MIB_TR181_ATM_LINK_TBL:
		{
			MIB_TR181_ATM_LINK_Tp pATM_LINK_Entry = (MIB_TR181_ATM_LINK_Tp)ptr;
			memset(pATM_LINK_Entry, 0, sizeof(MIB_TR181_ATM_LINK_T));
			pATM_LINK_Entry->Enable = 1;
			pATM_LINK_Entry->instnum = pEntry->ATM_Link_instnum;
			snprintf(pATM_LINK_Entry->LowerLayers, sizeof(pATM_LINK_Entry->LowerLayers), "%s", "Device.DSL.Channel.1");
			if (pEntry->cmode == CHANNEL_MODE_BRIDGE || pEntry->cmode == CHANNEL_MODE_IPOE || pEntry->cmode == CHANNEL_MODE_PPPOE)
			{
				pATM_LINK_Entry->LinkType = CHANNEL_MODE_BRIDGE;
			}
			else if (pEntry->cmode == CHANNEL_MODE_PPPOA)
			{
				pATM_LINK_Entry->LinkType = CHANNEL_MODE_PPPOA;
			}
			else if (pEntry->cmode == CHANNEL_MODE_RT1483)
			{
				pATM_LINK_Entry->LinkType = CHANNEL_MODE_RT1483;
			}
			else
			{
				pATM_LINK_Entry->LinkType = CHANNEL_MODE_BRIDGE;
			}
			pATM_LINK_Entry->VPI = pEntry->vpi;
			pATM_LINK_Entry->VCI = pEntry->vci;
			pATM_LINK_Entry->Encapsulation = pEntry->encap;
			break;
		}
		case MIB_TR181_PTM_LINK_TBL:
		{
			MIB_TR181_PTM_LINK_Tp pPTM_LINK_Entry = (MIB_TR181_PTM_LINK_Tp)ptr;
			memset(pPTM_LINK_Entry, 0, sizeof(MIB_TR181_PTM_LINK_T));
			pPTM_LINK_Entry->Enable = 1;
			pPTM_LINK_Entry->instnum = pEntry->PTM_Link_instnum;
			snprintf(pPTM_LINK_Entry->LowerLayers, sizeof(pPTM_LINK_Entry->LowerLayers), "%s", "Device.DSL.Channel.1");
			break;
		}
		case MIB_TR181_ETHERNET_LINK_TBL:
		{
			MIB_TR181_ETHERNET_LINK_Tp pETHERNET_LINK_Entry = (MIB_TR181_ETHERNET_LINK_Tp)ptr;
			memset(pETHERNET_LINK_Entry, 0, sizeof(MIB_TR181_ETHERNET_LINK_T));
			pETHERNET_LINK_Entry->Enable = 1;
			pETHERNET_LINK_Entry->instnum = pEntry->Ethernet_Link_instnum;

			if (pEntry->vprio == 0)
			{
				pETHERNET_LINK_Entry->PriorityTagging = 0;
			}
			else
			{
				pETHERNET_LINK_Entry->PriorityTagging = 1;
			}

			memset(LowerLayers, 0, sizeof(LowerLayers));
			if (pEntry->ATM_Link_instnum > 0)
			{
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.ATM.Link.%u", pEntry->ATM_Link_instnum);
			}
			else if (pEntry->PTM_Link_instnum > 0)
			{
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.PTM.Link.%u", pEntry->PTM_Link_instnum);
			}
			else if (pEntry->Ethernet_Interface_instnum > 0)
			{
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.Ethernet.Interface.%u", pEntry->Ethernet_Interface_instnum);
			}
			else if (pEntry->Optical_Interface_instnum > 0)
			{
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.Optical.Interface.%u", pEntry->Optical_Interface_instnum);
			}

			if (strlen(LowerLayers) > 0)
			{
				snprintf(pETHERNET_LINK_Entry->LowerLayers, sizeof(pETHERNET_LINK_Entry->LowerLayers), "%s", LowerLayers);
			}

			break;
		}
		case MIB_TR181_ETHERNET_VLANTERMINATION_TBL:
		{
			MIB_TR181_ETHERNET_VLANTERMINATION_Tp pETHERNET_VLANTERMINATION_Entry = (MIB_TR181_ETHERNET_VLANTERMINATION_Tp)ptr;
			memset(pETHERNET_VLANTERMINATION_Entry, 0, sizeof(MIB_TR181_ETHERNET_VLANTERMINATION_T));
			pETHERNET_VLANTERMINATION_Entry->Enable = 1;
			pETHERNET_VLANTERMINATION_Entry->VLANID = pEntry->vid;
			pETHERNET_VLANTERMINATION_Entry->TPID = 33024; //C-TAG 0x8100
			pETHERNET_VLANTERMINATION_Entry->instnum = pEntry->Ethernet_VLANTermination_instnum;

			memset(LowerLayers, 0, sizeof(LowerLayers));
			if (pEntry->Ethernet_Link_instnum > 0)
			{
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.Ethernet.Link.%u", pEntry->Ethernet_Link_instnum);
			}

			if (strlen(LowerLayers) > 0)
			{
				snprintf(pETHERNET_VLANTERMINATION_Entry->LowerLayers, sizeof(pETHERNET_VLANTERMINATION_Entry->LowerLayers), "%s", LowerLayers);
			}

			break;
		}
		case MIB_TR181_PPP_INTERFACE_TBL:
		{
			MIB_TR181_PPP_INTERFACE_Tp pPPP_INTERFACE_Entry = (MIB_TR181_PPP_INTERFACE_Tp)ptr;
			memset(pPPP_INTERFACE_Entry, 0, sizeof(MIB_TR181_PPP_INTERFACE_T));
			pPPP_INTERFACE_Entry->Enable = 1;
			pPPP_INTERFACE_Entry->AutoDisconnectTime = pEntry->autoDisTime;
			pPPP_INTERFACE_Entry->IdleDisconnectTime = pEntry->pppIdleTime;
			pPPP_INTERFACE_Entry->WarnDisconnectDelay = pEntry->warnDisDelay;
			strncpy(pPPP_INTERFACE_Entry->Username, pEntry->pppUsername, sizeof(pPPP_INTERFACE_Entry->Username));
			strncpy(pPPP_INTERFACE_Entry->Password, pEntry->pppPassword, sizeof(pPPP_INTERFACE_Entry->Password));
			pPPP_INTERFACE_Entry->MaxMRUSize = pEntry->mtu;
			if (pEntry->pppCtype == CONTINUOUS)
			{
				snprintf(pPPP_INTERFACE_Entry->ConnectionTrigger, sizeof(pPPP_INTERFACE_Entry->ConnectionTrigger), "AlwaysOn");
			}
			else if (pEntry->pppCtype == CONNECT_ON_DEMAND)
			{
				snprintf(pPPP_INTERFACE_Entry->ConnectionTrigger, sizeof(pPPP_INTERFACE_Entry->ConnectionTrigger), "OnDemand");
			}
			else if (pEntry->pppCtype == MANUAL)
			{
				snprintf(pPPP_INTERFACE_Entry->ConnectionTrigger, sizeof(pPPP_INTERFACE_Entry->ConnectionTrigger), "Manual");
			}
			strncpy(pPPP_INTERFACE_Entry->PPPoE_ACName, pEntry->pppACName, sizeof(pPPP_INTERFACE_Entry->PPPoE_ACName));
			strncpy(pPPP_INTERFACE_Entry->PPPoE_ServiceName, pEntry->pppServiceName, sizeof(pPPP_INTERFACE_Entry->PPPoE_ServiceName));
#ifdef CONFIG_IPV6
			if (pEntry->IpProtocol & IPVER_IPV4)
			{
				pPPP_INTERFACE_Entry->IPCPEnable = 1;
			}
			if (pEntry->IpProtocol & IPVER_IPV6)
			{
				pPPP_INTERFACE_Entry->IPv6CPEnable = 1;
			}
#else
			pPPP_INTERFACE_Entry->IPCPEnable = 1;
			pPPP_INTERFACE_Entry->IPv6CPEnable = 0;
#endif
			pPPP_INTERFACE_Entry->instnum = pEntry->PPP_Interface_instnum;

			memset(LowerLayers, 0, sizeof(LowerLayers));
			if (pEntry->Ethernet_VLANTermination_instnum > 0)
			{
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.Ethernet.VLANTermination.%u", pEntry->Ethernet_VLANTermination_instnum);
			}
			else if (pEntry->Ethernet_Link_instnum > 0)
			{
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.Ethernet.Link.%u", pEntry->Ethernet_Link_instnum);
			}

			if (strlen(LowerLayers) > 0)
			{
				snprintf(pPPP_INTERFACE_Entry->LowerLayers, sizeof(pPPP_INTERFACE_Entry->LowerLayers), "%s", LowerLayers);
			}

			break;
		}
		case MIB_TR181_IP_INTERFACE_TBL:
		{
			MIB_TR181_IP_INTERFACE_Tp pIP_INTERFACE_Entry = (MIB_TR181_IP_INTERFACE_Tp)ptr;
			memset(pIP_INTERFACE_Entry, 0, sizeof(MIB_TR181_IP_INTERFACE_T));
			pIP_INTERFACE_Entry->Enable = 1;
#ifdef CONFIG_IPV6
			if (pEntry->IpProtocol & IPVER_IPV4)
			{
				pIP_INTERFACE_Entry->IPv4Enable = 1;
			}
			if (pEntry->IpProtocol & IPVER_IPV6)
			{
				pIP_INTERFACE_Entry->IPv6Enable = 1;
			}
#else
			pIP_INTERFACE_Entry->IPv4Enable = 1;
			pIP_INTERFACE_Entry->IPv6Enable = 0;
#endif
			pIP_INTERFACE_Entry->MaxMTUSize = pEntry->mtu;
			if (pEntry->ipDhcp == DHCP_CLIENT)
			{
				pIP_INTERFACE_Entry->AutoIPEnable = 1;
			}
			pIP_INTERFACE_Entry->instnum = pEntry->IP_Interface_instnum;

			memset(LowerLayers, 0, sizeof(LowerLayers));
			if (pEntry->PPP_Interface_instnum > 0)
			{
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.PPP.Interface.%u", pEntry->PPP_Interface_instnum);
			}
			else if (pEntry->Ethernet_VLANTermination_instnum > 0)
			{
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.Ethernet.VLANTermination.%u", pEntry->Ethernet_VLANTermination_instnum);
			}
			else if (pEntry->Ethernet_Link_instnum > 0)
			{
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.Ethernet.Link.%u", pEntry->Ethernet_Link_instnum);
			}

			if (strlen(LowerLayers) > 0)
			{
				snprintf(pIP_INTERFACE_Entry->LowerLayers, sizeof(pIP_INTERFACE_Entry->LowerLayers), "%s", LowerLayers);
			}

			break;
		}
		default:
			return 0;
	}
	return 1;
}

/* action:
           0 -> delete
           1 -> add
           2 -> modify
*/
int rtk_tr181_update_wan_interface_entry_list(int action, MIB_CE_ATM_VC_Tp pOld_Entry, MIB_CE_ATM_VC_Tp pNew_Entry)
{
	int Bridging_Bridge_chainid = (-1);
	MIB_TR181_BRIDGING_BRIDGE_T BRIDGING_BRIDGE_Entry;

	int Bridging_Bridge_Port_chainid = (-1);
	MIB_TR181_BRIDGING_BRIDGE_PORT_T BRIDGING_BRIDGE_PORT_Entry;

	int Bridging_Bridge_VLAN_chainid = (-1);
	MIB_TR181_BRIDGING_BRIDGE_VLAN_T BRIDGING_BRIDGE_VLAN_Entry;

	int Bridging_Bridge_VLANPort_chainid = (-1);
	MIB_TR181_BRIDGING_BRIDGE_VLANPORT_T BRIDGING_BRIDGE_VLANPORT_Entry;

	int ATM_Link_chainid = (-1);
	MIB_TR181_ATM_LINK_T ATM_LINK_Entry;

	int PTM_Link_chainid = (-1);
	MIB_TR181_PTM_LINK_T PTM_LINK_Entry;

	int Ether_Link_chainid = (-1);
	MIB_TR181_ETHERNET_LINK_T ETHERNET_LINK_Entry;

	int Ethernet_VLANTermination_chainid = (-1);
	MIB_TR181_ETHERNET_VLANTERMINATION_T ETHERNET_VLANTERMINATION_Entry;

	int PPP_Interface_chainid = (-1);
	MIB_TR181_PPP_INTERFACE_T PPP_INTERFACE_Entry;

	int IP_Interface_chainid = (-1);
	MIB_TR181_IP_INTERFACE_T IP_INTERFACE_Entry;

	TR181_INTERFACE_STACK_T InterfaceStack;
	memset(&InterfaceStack, 0, sizeof(TR181_INTERFACE_STACK_T));

	if ((action == 0 && pOld_Entry == NULL) || (action == 1 && pNew_Entry == NULL) || (action == 2 && (pOld_Entry == NULL || pNew_Entry == NULL)))
		return 0;

	if (action == 1 || action == 2)
	{
		if (action == 2)
		{
			pNew_Entry->Bridging_Bridge_instnum = pOld_Entry->Bridging_Bridge_instnum;
			pNew_Entry->Bridging_Bridge_Port_instnum = pOld_Entry->Bridging_Bridge_Port_instnum;
			pNew_Entry->ATM_Link_instnum = pOld_Entry->ATM_Link_instnum;
			pNew_Entry->PTM_Link_instnum = pOld_Entry->PTM_Link_instnum;
			pNew_Entry->Ethernet_Interface_instnum = pOld_Entry->Ethernet_Interface_instnum;
			pNew_Entry->Optical_Interface_instnum = pOld_Entry->Optical_Interface_instnum;
			pNew_Entry->Ethernet_Link_instnum = pOld_Entry->Ethernet_Link_instnum;
			pNew_Entry->Ethernet_VLANTermination_instnum = pOld_Entry->Ethernet_VLANTermination_instnum;
			pNew_Entry->PPP_Interface_instnum = pOld_Entry->PPP_Interface_instnum;
			pNew_Entry->IP_Interface_instnum = pOld_Entry->IP_Interface_instnum;
		}

		rtk_tr181_get_wan_interfacestack_by_entry(pNew_Entry, &InterfaceStack);
	}

	if (action == 0 || action == 2)
	{
		if (pOld_Entry->Bridging_Bridge_instnum > 0 && pOld_Entry->Bridging_Bridge_instnum != InterfaceStack.Bridging_Bridge_instnum)
		{
			if (get_Bridging_Bridge_EntryByInstNum(pOld_Entry->Bridging_Bridge_instnum, &BRIDGING_BRIDGE_Entry, &Bridging_Bridge_chainid))
			{
				remove_BridgingBridge_related_Tables_ByInstNum(pOld_Entry->Bridging_Bridge_instnum);
				mib_chain_delete(MIB_TR181_BRIDGING_BRIDGE_TBL, Bridging_Bridge_chainid);
			}
		}

		if (pOld_Entry->Bridging_Bridge_Port_instnum > 0 && pOld_Entry->Bridging_Bridge_Port_instnum != InterfaceStack.Bridging_Bridge_Port_instnum)
		{
			if (get_BridgingBridgePort_EntryByInstNum(pOld_Entry->Bridging_Bridge_instnum, pOld_Entry->Bridging_Bridge_Port_instnum, &BRIDGING_BRIDGE_PORT_Entry, &Bridging_Bridge_Port_chainid))
			{
				mib_chain_delete(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL, Bridging_Bridge_Port_chainid);
			}
		}

		if (pOld_Entry->ATM_Link_instnum > 0 && pOld_Entry->ATM_Link_instnum != InterfaceStack.ATM_Link_instnum)
		{
			if (get_ATM_Link_EntryByInstNum(pOld_Entry->ATM_Link_instnum, &ATM_LINK_Entry, &ATM_Link_chainid))
			{
				mib_chain_delete(MIB_TR181_ATM_LINK_TBL, ATM_Link_chainid);
			}
		}

		if (pOld_Entry->PTM_Link_instnum > 0 && pOld_Entry->PTM_Link_instnum != InterfaceStack.PTM_Link_instnum)
		{
			if (get_PTM_Link_EntryByInstNum(pOld_Entry->PTM_Link_instnum, &PTM_LINK_Entry, &PTM_Link_chainid))
			{
				mib_chain_delete(MIB_TR181_PTM_LINK_TBL, PTM_Link_chainid);
			}
		}

		if (pOld_Entry->Ethernet_Link_instnum > 0 && pOld_Entry->Ethernet_Link_instnum != InterfaceStack.Ethernet_Link_instnum)
		{
			if (get_Ether_Link_EntryByInstNum(pOld_Entry->Ethernet_Link_instnum, &ETHERNET_LINK_Entry, &Ether_Link_chainid))
			{
				mib_chain_delete(MIB_TR181_ETHERNET_LINK_TBL, Ether_Link_chainid);
			}
		}

		if (pOld_Entry->Ethernet_VLANTermination_instnum > 0 && pOld_Entry->Ethernet_VLANTermination_instnum != InterfaceStack.Ethernet_VLANTermination_instnum)
		{
			if (get_Ethernet_VLANTermination_EntryByInstNum(pOld_Entry->Ethernet_VLANTermination_instnum, &ETHERNET_VLANTERMINATION_Entry, &Ethernet_VLANTermination_chainid))
			{
				mib_chain_delete(MIB_TR181_ETHERNET_VLANTERMINATION_TBL, Ethernet_VLANTermination_chainid);
			}
		}

		if (pOld_Entry->PPP_Interface_instnum > 0 && pOld_Entry->PPP_Interface_instnum != InterfaceStack.PPP_Interface_instnum)
		{
			if (get_PPP_Interface_EntryByInstNum(pOld_Entry->PPP_Interface_instnum, &PPP_INTERFACE_Entry, &PPP_Interface_chainid))
			{
				mib_chain_delete(MIB_TR181_PPP_INTERFACE_TBL, PPP_Interface_chainid);
			}
		}

		if (pOld_Entry->IP_Interface_instnum > 0 && pOld_Entry->IP_Interface_instnum != InterfaceStack.IP_Interface_instnum)
		{
			if (get_IP_Interface_EntryByInstNum(pOld_Entry->IP_Interface_instnum, &IP_INTERFACE_Entry, &IP_Interface_chainid))
			{
				mib_chain_delete(MIB_TR181_IP_INTERFACE_TBL, IP_Interface_chainid);
			}
		}

		/* Ingress LAN Port */
		if ((CHANNEL_MODE_T)pOld_Entry->cmode == CHANNEL_MODE_BRIDGE && pOld_Entry->itfGroup > 0)
		{
			int i = 0, j = 0, if_num = 0, num = 0;
			char LowerLayers[1024+1] = {0};
			MIB_TR181_BRIDGING_BRIDGE_PORT_T BRIDGING_BRIDGE_PORT_Entry_old;
			/* check eth-lan port bind */
			if_num = SW_LAN_PORT_NUM;
#ifdef WLAN_SUPPORT
			/* check wlan port bind */
			if_num += ((WLAN_MBSSID_NUM + 1) * NUM_WLAN_INTERFACE);
#endif

			for (j = 0; j < if_num; j++)
			{
				if ((pOld_Entry->itfGroup & (0x1 << j)))
				{
					if (action == 2)
					{
						if ((pNew_Entry->itfGroup & (0x1 << j)))
						{
							// same port
							continue;
						}
					}

					memset(LowerLayers, 0, sizeof(LowerLayers));

					if (j < SW_LAN_PORT_NUM)
					{
						snprintf(LowerLayers, sizeof(LowerLayers), "Device.Ethernet.Interface.%d", (j + 1));
					}
					else
					{
						snprintf(LowerLayers, sizeof(LowerLayers), "Device.WiFi.SSID.%d", ((j - SW_LAN_PORT_NUM) + 1));
					}

					num = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL);
					for (i = 0; i < num; i++)
					{
						if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL, i, (void *)&BRIDGING_BRIDGE_PORT_Entry_old) == 0)
							continue;

						if (BRIDGING_BRIDGE_PORT_Entry_old.bridge_instnum == pOld_Entry->Bridging_Bridge_instnum &&
							strcmp(LowerLayers, BRIDGING_BRIDGE_PORT_Entry_old.LowerLayers) == 0)
						{
							mib_chain_delete(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL, i);
							break;
						}
					}
				}
			}
		}
	}

	if (action == 1 || action == 2)
	{
		pNew_Entry->Bridging_Bridge_instnum = InterfaceStack.Bridging_Bridge_instnum;
		pNew_Entry->Bridging_Bridge_Port_instnum = InterfaceStack.Bridging_Bridge_Port_instnum;
		pNew_Entry->ATM_Link_instnum = InterfaceStack.ATM_Link_instnum;
		pNew_Entry->PTM_Link_instnum = InterfaceStack.PTM_Link_instnum;
		pNew_Entry->Ethernet_Interface_instnum = InterfaceStack.Ethernet_Interface_instnum;
		pNew_Entry->Optical_Interface_instnum = InterfaceStack.Optical_Interface_instnum;
		pNew_Entry->Ethernet_Link_instnum = InterfaceStack.Ethernet_Link_instnum;
		pNew_Entry->Ethernet_VLANTermination_instnum = InterfaceStack.Ethernet_VLANTermination_instnum;
		pNew_Entry->PPP_Interface_instnum = InterfaceStack.PPP_Interface_instnum;
		pNew_Entry->IP_Interface_instnum = InterfaceStack.IP_Interface_instnum;

		if (InterfaceStack.Bridging_Bridge_instnum > 0 && InterfaceStack.Bridging_Bridge_Port_instnum > 0)
		{
			char LowerLayers[1024+1] = {0};
			MIB_TR181_BRIDGING_BRIDGE_T BRIDGING_BRIDGE_Entry_new;
			MIB_TR181_BRIDGING_BRIDGE_PORT_T BRIDGING_BRIDGE_PORT_Entry_new;
			memset(&BRIDGING_BRIDGE_Entry_new, 0, sizeof(MIB_TR181_BRIDGING_BRIDGE_T));
			BRIDGING_BRIDGE_Entry_new.Enable = 1;
			strcpy(BRIDGING_BRIDGE_Entry_new.Standard, "802.1Q-2005");
			BRIDGING_BRIDGE_Entry_new.instnum = InterfaceStack.Bridging_Bridge_instnum;

			if (get_Bridging_Bridge_EntryByInstNum(InterfaceStack.Bridging_Bridge_instnum, &BRIDGING_BRIDGE_Entry, &Bridging_Bridge_chainid) == 0)
			{
				mib_chain_add(MIB_TR181_BRIDGING_BRIDGE_TBL, (void *)&BRIDGING_BRIDGE_Entry_new);
			}
			else
			{
				mib_chain_update(MIB_TR181_BRIDGING_BRIDGE_TBL, (void *)&BRIDGING_BRIDGE_Entry_new, Bridging_Bridge_chainid);
			}

			if (InterfaceStack.Bridging_Bridge_Port_instnum == 1) // ManagementPort
			{
				memset(&BRIDGING_BRIDGE_PORT_Entry_new, 0, sizeof(MIB_TR181_BRIDGING_BRIDGE_PORT_T));
				BRIDGING_BRIDGE_PORT_Entry_new.Enable = 1;
				BRIDGING_BRIDGE_PORT_Entry_new.ManagementPort = 1;
				BRIDGING_BRIDGE_PORT_Entry_new.bridge_instnum = InterfaceStack.Bridging_Bridge_instnum;
				BRIDGING_BRIDGE_PORT_Entry_new.instnum = InterfaceStack.Bridging_Bridge_Port_instnum;

				memset(LowerLayers, 0, sizeof(LowerLayers));
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.Bridging.Bridge.%u.Port.%u", InterfaceStack.Bridging_Bridge_instnum, (InterfaceStack.Bridging_Bridge_Port_instnum + 1));
				snprintf(BRIDGING_BRIDGE_PORT_Entry_new.LowerLayers, sizeof(BRIDGING_BRIDGE_PORT_Entry_new.LowerLayers), "%s", LowerLayers);

				if (get_BridgingBridgePort_EntryByInstNum(InterfaceStack.Bridging_Bridge_instnum, InterfaceStack.Bridging_Bridge_Port_instnum, &BRIDGING_BRIDGE_PORT_Entry, &Bridging_Bridge_Port_chainid) == 0)
				{
					mib_chain_add(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL, (void *)&BRIDGING_BRIDGE_PORT_Entry_new);
				}
				else
				{
					mib_chain_update(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL, (void *)&BRIDGING_BRIDGE_PORT_Entry_new, Bridging_Bridge_Port_chainid);
				}

				InterfaceStack.Bridging_Bridge_Port_instnum += 1; // update to non-management port
				pNew_Entry->Bridging_Bridge_Port_instnum = InterfaceStack.Bridging_Bridge_Port_instnum;
			}

			memset(&BRIDGING_BRIDGE_PORT_Entry_new, 0, sizeof(MIB_TR181_BRIDGING_BRIDGE_PORT_T));
			BRIDGING_BRIDGE_PORT_Entry_new.Enable = 1;
			BRIDGING_BRIDGE_PORT_Entry_new.ManagementPort = 0;
			BRIDGING_BRIDGE_PORT_Entry_new.bridge_instnum = InterfaceStack.Bridging_Bridge_instnum;
			BRIDGING_BRIDGE_PORT_Entry_new.instnum = InterfaceStack.Bridging_Bridge_Port_instnum;

			/* Ingress */
			if (pNew_Entry->vlan)
			{
				if (pNew_Entry->vid > 0)
				{
					BRIDGING_BRIDGE_PORT_Entry_new.PVID = pNew_Entry->vid;
					snprintf(BRIDGING_BRIDGE_PORT_Entry_new.AcceptableFrameTypes, sizeof(BRIDGING_BRIDGE_PORT_Entry_new.AcceptableFrameTypes), "AdmitOnlyVLANTagged");
					BRIDGING_BRIDGE_PORT_Entry_new.IngressFiltering = 1;
				}

				if (pNew_Entry->vprio == 0)
				{
					BRIDGING_BRIDGE_PORT_Entry_new.PriorityTagging = 0;
				}
				else
				{
					BRIDGING_BRIDGE_PORT_Entry_new.PriorityTagging = 1;
				}
			}
			else
			{
				BRIDGING_BRIDGE_PORT_Entry_new.PVID = 1;
				snprintf(BRIDGING_BRIDGE_PORT_Entry_new.AcceptableFrameTypes, sizeof(BRIDGING_BRIDGE_PORT_Entry_new.AcceptableFrameTypes), "AdmitAll");
				BRIDGING_BRIDGE_PORT_Entry_new.IngressFiltering = 0;
				BRIDGING_BRIDGE_PORT_Entry_new.PriorityTagging = 0;
			}
			BRIDGING_BRIDGE_PORT_Entry_new.TPID = 33024; //C-TAG 0x8100

			memset(LowerLayers, 0, sizeof(LowerLayers));
			if (InterfaceStack.Ethernet_Interface_instnum > 0)
			{
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.Ethernet.Interface.%u", InterfaceStack.Ethernet_Interface_instnum);
			}
			else if (InterfaceStack.Optical_Interface_instnum > 0)
			{
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.Optical.Interface.%u", InterfaceStack.Optical_Interface_instnum);
			}
			else if (InterfaceStack.ATM_Link_instnum > 0)
			{
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.ATM.Link.%u", InterfaceStack.ATM_Link_instnum);
			}
			else if (InterfaceStack.PTM_Link_instnum > 0)
			{
				snprintf(LowerLayers, sizeof(LowerLayers), "Device.PTM.Link.%u", InterfaceStack.PTM_Link_instnum);
			}

			if (strlen(LowerLayers) > 0)
			{
				snprintf(BRIDGING_BRIDGE_PORT_Entry_new.LowerLayers, sizeof(BRIDGING_BRIDGE_PORT_Entry_new.LowerLayers), "%s", LowerLayers);
			}

			if (get_BridgingBridgePort_EntryByInstNum(InterfaceStack.Bridging_Bridge_instnum, InterfaceStack.Bridging_Bridge_Port_instnum, &BRIDGING_BRIDGE_PORT_Entry, &Bridging_Bridge_Port_chainid) == 0)
			{
				mib_chain_add(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL, (void *)&BRIDGING_BRIDGE_PORT_Entry_new);
			}
			else
			{
				mib_chain_update(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL, (void *)&BRIDGING_BRIDGE_PORT_Entry_new, Bridging_Bridge_Port_chainid);
			}

			/* Ingress LAN Port */
			if ((CHANNEL_MODE_T)pNew_Entry->cmode == CHANNEL_MODE_BRIDGE && pNew_Entry->itfGroup > 0)
			{
				int i = 0, j = 0, if_num = 0, num = 0, find = 0;
				/* check eth-lan port bind */
				if_num = SW_LAN_PORT_NUM;
#ifdef WLAN_SUPPORT
				/* check wlan port bind */
				if_num += ((WLAN_MBSSID_NUM + 1) * NUM_WLAN_INTERFACE);
#endif

				for (j = 0; j < if_num; j++)
				{
					if ((pNew_Entry->itfGroup & (0x1 << j)))
					{
						memset(LowerLayers, 0, sizeof(LowerLayers));

						if (j < SW_LAN_PORT_NUM)
						{
							snprintf(LowerLayers, sizeof(LowerLayers), "Device.Ethernet.Interface.%d", (j + 1));
						}
						else
						{
							snprintf(LowerLayers, sizeof(LowerLayers), "Device.WiFi.SSID.%d", ((j - SW_LAN_PORT_NUM) + 1));
						}

						find = 0;
						num = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL);
						for (i = 0; i < num; i++)
						{
							if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL, i, (void *)&BRIDGING_BRIDGE_PORT_Entry_new) == 0)
								continue;

							if (BRIDGING_BRIDGE_PORT_Entry_new.bridge_instnum == InterfaceStack.Bridging_Bridge_instnum &&
								strcmp(LowerLayers, BRIDGING_BRIDGE_PORT_Entry_new.LowerLayers) == 0)
							{
								find = 1;
								break;
							}
						}

						if (find == 0)
						{
							memset(&BRIDGING_BRIDGE_PORT_Entry_new, 0, sizeof(MIB_TR181_BRIDGING_BRIDGE_PORT_T));
							BRIDGING_BRIDGE_PORT_Entry_new.Enable = 1;
							BRIDGING_BRIDGE_PORT_Entry_new.ManagementPort = 0;
							BRIDGING_BRIDGE_PORT_Entry_new.PVID = 1;
							BRIDGING_BRIDGE_PORT_Entry_new.TPID = 33024; //C-TAG 0x8100
							BRIDGING_BRIDGE_PORT_Entry_new.IngressFiltering = 0;
							BRIDGING_BRIDGE_PORT_Entry_new.PriorityTagging = 0;
							BRIDGING_BRIDGE_PORT_Entry_new.DefaultUserPriority = 0;
							strcpy(BRIDGING_BRIDGE_PORT_Entry_new.PriorityRegeneration, "0,1,2,3,4,5,6,7");
							strcpy(BRIDGING_BRIDGE_PORT_Entry_new.AcceptableFrameTypes, "AdmitAll");
							strcpy(BRIDGING_BRIDGE_PORT_Entry_new.LowerLayers, LowerLayers);
							BRIDGING_BRIDGE_PORT_Entry_new.bridge_instnum = InterfaceStack.Bridging_Bridge_instnum;
							BRIDGING_BRIDGE_PORT_Entry_new.instnum = (find_max_BridgingBridgePort_instNum(InterfaceStack.Bridging_Bridge_instnum) + 1);
							mib_chain_add(MIB_TR181_BRIDGING_BRIDGE_PORT_TBL, (void *)&BRIDGING_BRIDGE_PORT_Entry_new);
						}
					}
				}
			}

			/* Egress VLAN membership table */
			if (pNew_Entry->vlan)
			{
				int i = 0, num = 0, find = 0;
				char vlan_path[256] = {0}, port_path[256] = {0};

				find = 0;
				num = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_VLAN_TBL);
				for (i = 0; i < num; i++)
				{
					if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_VLAN_TBL, i, (void *)&BRIDGING_BRIDGE_VLAN_Entry) == 0)
						continue;

					if (BRIDGING_BRIDGE_VLAN_Entry.bridge_instnum == InterfaceStack.Bridging_Bridge_instnum)
					{
						if ((2 == action)&&(pOld_Entry))
						{
							if ((pOld_Entry->vlan)&&(pOld_Entry->Bridging_Bridge_instnum == InterfaceStack.Bridging_Bridge_instnum))
							{
								if (pOld_Entry->Bridging_Bridge_Port_instnum == InterfaceStack.Bridging_Bridge_Port_instnum)
								{
									if ((BRIDGING_BRIDGE_VLAN_Entry.VLANID == pOld_Entry->vid)&&(pOld_Entry->vid != pNew_Entry->vid))
									{
										BRIDGING_BRIDGE_VLAN_Entry.Enable = 1;
										BRIDGING_BRIDGE_VLAN_Entry.VLANID = pNew_Entry->vid;
										mib_chain_update(MIB_TR181_BRIDGING_BRIDGE_VLAN_TBL, (void *)&BRIDGING_BRIDGE_VLAN_Entry, i);
										find = 1;
										break;
									}
								}
							}
						}

						if (BRIDGING_BRIDGE_VLAN_Entry.VLANID == pNew_Entry->vid)
						{
							BRIDGING_BRIDGE_VLAN_Entry.Enable = 1;
							mib_chain_update(MIB_TR181_BRIDGING_BRIDGE_VLAN_TBL, (void *)&BRIDGING_BRIDGE_VLAN_Entry, i);
							find = 1;
							break;
						}
					}
				}

				if (find == 0)
				{
					memset(&BRIDGING_BRIDGE_VLAN_Entry, 0, sizeof(MIB_TR181_BRIDGING_BRIDGE_VLAN_T));
					BRIDGING_BRIDGE_VLAN_Entry.Enable = 1;
					BRIDGING_BRIDGE_VLAN_Entry.VLANID = pNew_Entry->vid;
					BRIDGING_BRIDGE_VLAN_Entry.bridge_instnum = InterfaceStack.Bridging_Bridge_instnum;
					BRIDGING_BRIDGE_VLAN_Entry.instnum = (find_max_BridgingBridgeVLAN_instNum(InterfaceStack.Bridging_Bridge_instnum) + 1);
					mib_chain_add(MIB_TR181_BRIDGING_BRIDGE_VLAN_TBL, (void *)&BRIDGING_BRIDGE_VLAN_Entry);
				}

				snprintf(vlan_path, sizeof(vlan_path), "Device.Bridging.Bridge.%u.VLAN.%u", InterfaceStack.Bridging_Bridge_instnum, BRIDGING_BRIDGE_VLAN_Entry.instnum);
				snprintf(port_path, sizeof(port_path), "Device.Bridging.Bridge.%u.Port.%u", InterfaceStack.Bridging_Bridge_instnum, InterfaceStack.Bridging_Bridge_Port_instnum);

				find = 0;
				num = mib_chain_total(MIB_TR181_BRIDGING_BRIDGE_VLANPORT_TBL);
				for (i = 0; i < num; i++)
				{
					if (mib_chain_get(MIB_TR181_BRIDGING_BRIDGE_VLANPORT_TBL, i, (void *)&BRIDGING_BRIDGE_VLANPORT_Entry) == 0)
						continue;

					if (BRIDGING_BRIDGE_VLANPORT_Entry.bridge_instnum == InterfaceStack.Bridging_Bridge_instnum)
					{
						if (strlen(BRIDGING_BRIDGE_VLANPORT_Entry.Port) > 0 && strlen(BRIDGING_BRIDGE_VLANPORT_Entry.VLAN) > 0 &&
							strstr(BRIDGING_BRIDGE_VLANPORT_Entry.VLAN, vlan_path) && strstr(BRIDGING_BRIDGE_VLANPORT_Entry.Port, port_path))
						{
							BRIDGING_BRIDGE_VLANPORT_Entry.Enable = 1;
							BRIDGING_BRIDGE_VLANPORT_Entry.Untagged = 0;
							mib_chain_update(MIB_TR181_BRIDGING_BRIDGE_VLANPORT_TBL, (void *)&BRIDGING_BRIDGE_VLANPORT_Entry, i);
							find = 1;
							break;
						}
					}
				}

				if (find == 0)
				{
					memset(&BRIDGING_BRIDGE_VLANPORT_Entry, 0, sizeof(MIB_TR181_BRIDGING_BRIDGE_VLANPORT_T));
					BRIDGING_BRIDGE_VLANPORT_Entry.Enable = 1;
					snprintf(BRIDGING_BRIDGE_VLANPORT_Entry.VLAN, sizeof(BRIDGING_BRIDGE_VLANPORT_Entry.VLAN), "%s", vlan_path);
					snprintf(BRIDGING_BRIDGE_VLANPORT_Entry.Port, sizeof(BRIDGING_BRIDGE_VLANPORT_Entry.Port), "%s", port_path);
					BRIDGING_BRIDGE_VLANPORT_Entry.Untagged = 0;
					BRIDGING_BRIDGE_VLANPORT_Entry.bridge_instnum = InterfaceStack.Bridging_Bridge_instnum;
					BRIDGING_BRIDGE_VLANPORT_Entry.instnum = (find_max_BridgingBridgeVLANPort_instNum(InterfaceStack.Bridging_Bridge_instnum) + 1);
					mib_chain_add(MIB_TR181_BRIDGING_BRIDGE_VLANPORT_TBL, (void *)&BRIDGING_BRIDGE_VLANPORT_Entry);
				}
			}
		}

		if (InterfaceStack.ATM_Link_instnum > 0)
		{
			MIB_TR181_ATM_LINK_T ATM_LINK_Entry_new;

			rtk_tr181_set_table_by_wan_entry(MIB_TR181_ATM_LINK_TBL, (void *)&ATM_LINK_Entry_new, pNew_Entry);

			if (get_ATM_Link_EntryByInstNum(InterfaceStack.ATM_Link_instnum, &ATM_LINK_Entry, &ATM_Link_chainid) == 0)
			{
				mib_chain_add(MIB_TR181_ATM_LINK_TBL, (void *)&ATM_LINK_Entry_new);
			}
			else
			{
				mib_chain_update(MIB_TR181_ATM_LINK_TBL, (void *)&ATM_LINK_Entry_new, ATM_Link_chainid);
			}
		}

		if (InterfaceStack.PTM_Link_instnum > 0)
		{
			MIB_TR181_PTM_LINK_T PTM_LINK_Entry_new;

			rtk_tr181_set_table_by_wan_entry(MIB_TR181_PTM_LINK_TBL, (void *)&PTM_LINK_Entry_new, pNew_Entry);

			if (get_PTM_Link_EntryByInstNum(InterfaceStack.PTM_Link_instnum, &PTM_LINK_Entry, &PTM_Link_chainid) == 0)
			{
				mib_chain_add(MIB_TR181_PTM_LINK_TBL, (void *)&PTM_LINK_Entry_new);
			}
			else
			{
				mib_chain_update(MIB_TR181_PTM_LINK_TBL, (void *)&PTM_LINK_Entry_new, PTM_Link_chainid);
			}
		}

		if (InterfaceStack.Ethernet_Link_instnum > 0)
		{
			MIB_TR181_ETHERNET_LINK_T ETHERNET_LINK_Entry_new;

			rtk_tr181_set_table_by_wan_entry(MIB_TR181_ETHERNET_LINK_TBL, (void *)&ETHERNET_LINK_Entry_new, pNew_Entry);

			if (get_Ether_Link_EntryByInstNum(InterfaceStack.Ethernet_Link_instnum, &ETHERNET_LINK_Entry, &Ether_Link_chainid) == 0)
			{
				mib_chain_add(MIB_TR181_ETHERNET_LINK_TBL, (void *)&ETHERNET_LINK_Entry_new);
			}
			else
			{
				mib_chain_update(MIB_TR181_ETHERNET_LINK_TBL, (void *)&ETHERNET_LINK_Entry_new, Ether_Link_chainid);
			}
		}

		if (InterfaceStack.Ethernet_VLANTermination_instnum > 0)
		{
			MIB_TR181_ETHERNET_VLANTERMINATION_T ETHERNET_VLANTERMINATION_Entry_new;

			rtk_tr181_set_table_by_wan_entry(MIB_TR181_ETHERNET_VLANTERMINATION_TBL, (void *)&ETHERNET_VLANTERMINATION_Entry_new, pNew_Entry);

			if (get_Ethernet_VLANTermination_EntryByInstNum(InterfaceStack.Ethernet_VLANTermination_instnum, &ETHERNET_VLANTERMINATION_Entry, &Ethernet_VLANTermination_chainid) == 0)
			{
				mib_chain_add(MIB_TR181_ETHERNET_VLANTERMINATION_TBL, (void *)&ETHERNET_VLANTERMINATION_Entry_new);
			}
			else
			{
				mib_chain_update(MIB_TR181_ETHERNET_VLANTERMINATION_TBL, (void *)&ETHERNET_VLANTERMINATION_Entry_new, Ethernet_VLANTermination_chainid);
			}
		}

		if (InterfaceStack.PPP_Interface_instnum > 0)
		{
			MIB_TR181_PPP_INTERFACE_T PPP_INTERFACE_Entry_new;

			rtk_tr181_set_table_by_wan_entry(MIB_TR181_PPP_INTERFACE_TBL, (void *)&PPP_INTERFACE_Entry_new, pNew_Entry);

			if (get_PPP_Interface_EntryByInstNum(InterfaceStack.PPP_Interface_instnum, &PPP_INTERFACE_Entry, &PPP_Interface_chainid) == 0)
			{
				mib_chain_add(MIB_TR181_PPP_INTERFACE_TBL, (void *)&PPP_INTERFACE_Entry_new);
			}
			else
			{
				mib_chain_update(MIB_TR181_PPP_INTERFACE_TBL, (void *)&PPP_INTERFACE_Entry_new, PPP_Interface_chainid);
			}
		}

		if (InterfaceStack.IP_Interface_instnum > 0)
		{
			MIB_TR181_IP_INTERFACE_T IP_INTERFACE_Entry_new;

			rtk_tr181_set_table_by_wan_entry(MIB_TR181_IP_INTERFACE_TBL, (void *)&IP_INTERFACE_Entry_new, pNew_Entry);

			if (get_IP_Interface_EntryByInstNum(InterfaceStack.IP_Interface_instnum, &IP_INTERFACE_Entry, &IP_Interface_chainid) == 0)
			{
				mib_chain_add(MIB_TR181_IP_INTERFACE_TBL, (void *)&IP_INTERFACE_Entry_new);
			}
			else
			{
				mib_chain_update(MIB_TR181_IP_INTERFACE_TBL, (void *)&IP_INTERFACE_Entry_new, IP_Interface_chainid);
			}
		}
	}

	return 0;
}

int rtk_tr181_get_wan_entry_by_IP_instnum(MIB_CE_ATM_VC_T *pWAN_Entry, int IP_Interface_instnum)
{
	MIB_CE_ATM_VC_T Entry;
	int total, i;

	if (NULL == pWAN_Entry)
		return 0;

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if (Entry.IP_Interface_instnum == IP_Interface_instnum)
		{
			memcpy(pWAN_Entry, &Entry, sizeof(MIB_CE_ATM_VC_T));
			return 1;
		}
	}

	return 0;
}
#endif

int getDisplayWanName(MIB_CE_ATM_VC_T *pEntry, char* name)
{
	MEDIA_TYPE_T mType;

	if(pEntry==NULL || name==NULL)
		return 0;

	name[0] = '\0';
	mType = MEDIA_INDEX(pEntry->ifIndex);
	if (pEntry->cmode == CHANNEL_MODE_PPPOA) { // ppp0, ...
		snprintf(name, 6, "ppp%u", PPP_INDEX(pEntry->ifIndex));
	}
	else if (pEntry->cmode == CHANNEL_MODE_PPPOE) { // ppp0_vc0, ...
		if (mType == MEDIA_ATM)
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			snprintf(name, 16, "ppp%u_%s%u_%u", PPP_INDEX(pEntry->ifIndex), ALIASNAME_VC, VC_MAJOR_INDEX(pEntry->ifIndex), VC_MINOR_INDEX(pEntry->ifIndex));
#else
			snprintf(name, 16, "ppp%u_%s%u", PPP_INDEX(pEntry->ifIndex), ALIASNAME_VC, VC_INDEX(pEntry->ifIndex));
#endif
#ifdef CONFIG_PTMWAN
		else if (mType == MEDIA_PTM)
			snprintf(name, 16, "ppp%u_%s%u", PPP_INDEX(pEntry->ifIndex), ALIASNAME_MWPTM, PTM_INDEX(pEntry->ifIndex));
#endif /*CONFIG_PTMWAN*/
		else if (mType == MEDIA_ETH)
#ifdef CONFIG_RTL_MULTI_ETH_WAN
			snprintf(name, 16, "ppp%u_%s%u", PPP_INDEX(pEntry->ifIndex), ALIASNAME_MWNAS, ETH_INDEX(pEntry->ifIndex));
#else
			snprintf(name, 16, "ppp%u_%s%u", PPP_INDEX(pEntry->ifIndex), ALIASNAME_NAS, ETH_INDEX(pEntry->ifIndex));
#endif
#ifdef WLAN_WISP
		else if (mType == MEDIA_WLAN)
			snprintf(name, 16, "ppp%u_wlan%d-vxd", PPP_INDEX(pEntry->ifIndex), ETH_INDEX(pEntry->ifIndex));
#endif
		else
			return 0;
	}
	else { // vc0 ...
		if (mType == MEDIA_ATM) {
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			if (VC_MINOR_INDEX(pEntry->ifIndex) == DUMMY_VC_MINOR_INDEX)
				snprintf(name, 16, "%s%u", ALIASNAME_VC, VC_MAJOR_INDEX(pEntry->ifIndex));
			else
				snprintf(name, 16, "%s%u_%u", ALIASNAME_VC, VC_MAJOR_INDEX(pEntry->ifIndex), VC_MINOR_INDEX(pEntry->ifIndex));
#else
			snprintf(name, 16, "%s%u", ALIASNAME_VC, VC_INDEX(pEntry->ifIndex));
#endif
		}
#ifdef CONFIG_PTMWAN
		else if (mType == MEDIA_PTM)
			snprintf(name, 16, "%s%u", ALIASNAME_MWPTM, PTM_INDEX(pEntry->ifIndex));
#endif /*CONFIG_PTMWAN*/
		else if (mType == MEDIA_ETH)
#ifdef CONFIG_RTL_MULTI_ETH_WAN
			snprintf(name, 16, "%s%u", ALIASNAME_MWNAS, ETH_INDEX(pEntry->ifIndex));
#else
			snprintf(name, 16, "%s%u", ALIASNAME_NAS, ETH_INDEX(pEntry->ifIndex));
#endif
#ifdef WLAN_WISP
		else if (mType == MEDIA_WLAN)
			snprintf(name, 16, VXD_IF, ETH_INDEX(pEntry->ifIndex));
#endif
		else
			return 0;
	}
	return 1;
}

//jim: to get wan MIB by index... this index is combined index for ppp or vc...
//jim: to get wan MIB by index... this index is combined index for ppp or vc...
int getWanEntrybyWanName(const char *wname, MIB_CE_ATM_VC_T *pEntry, int *entry_index)
{
	unsigned int entryNum, i;
	char wanname[MAX_WAN_NAME_LEN] = {0};
	MIB_CE_ATM_VC_T vcEntry;

	if(wname == NULL) return -1;
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
		{
			printf("Get chain record error!\n");
			return -1;
		}

		if (vcEntry.enable == 0)
			continue;

		getWanName(&vcEntry, wanname);
		if(!strcmp(wanname, wname)){
			break;
		}
	}

	if(i>= entryNum){
		//printf("not find this interface!\n");
		return -1;
	}

	if(pEntry) memcpy(pEntry, (void *)&vcEntry, sizeof(MIB_CE_ATM_VC_T));
	if(entry_index) *entry_index = i;

	return 0;
}

int getWanEntrybyIfIndex(unsigned int ifIndex, MIB_CE_ATM_VC_T *pEntry, int *entry_index)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T vcEntry;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
		{
			printf("Get chain record error!\n");
			return -1;
		}

		if (vcEntry.enable == 0)
			continue;

		if(vcEntry.ifIndex == ifIndex){
			break;
		}
	}

	if(i>= entryNum){
		//printf("not find this interface!\n");
		return -1;
	}

	if(pEntry) memcpy(pEntry, (void *)&vcEntry, sizeof(MIB_CE_ATM_VC_T));
	if(entry_index) *entry_index = i;

	return 0;
}

int getWanEntrybyindex(MIB_CE_ATM_VC_T *pEntry, unsigned int ifIndex)
{
	int entry_index = -1;
	return getWanEntrybyIfIndex(ifIndex, pEntry, &entry_index);
}

unsigned int getWanIfMapbyMedia(MEDIA_TYPE_T mType)
{
	int mibtotal,i;
	MIB_CE_ATM_VC_T Entry;
	unsigned int ifMap; // high half for PPP bitmap, low half for vc bitmap

	ifMap=0;//init
	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<mibtotal;i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;
		if (Entry.cmode == CHANNEL_MODE_PPPOE)
			ifMap |= (1 << 16) << PPP_INDEX(Entry.ifIndex);	// PPP map
		if(MEDIA_INDEX(Entry.ifIndex) == mType)
			ifMap |= 1 << VC_INDEX(Entry.ifIndex);
	}

	return ifMap;
}

// Kaohj --- first entry
int getWanEntrybyMedia(MIB_CE_ATM_VC_T *pEntry, MEDIA_TYPE_T mType)
{
	if(pEntry==NULL)
		return -1;
	int mibtotal,i,num=0,totalnum=0;
	MIB_CE_ATM_VC_T Entry;
	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<mibtotal;i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;
		if(MEDIA_INDEX(Entry.ifIndex) == mType)
			break;
	}
	if(i==mibtotal)
		return -1;
	memcpy(pEntry, &Entry, sizeof(Entry));
	return i;
}

int getWanEntrybyIfname(const char *pIfname, MIB_CE_ATM_VC_T *pEntry, int *entry_index)
{
	unsigned int entryNum, i;
	char ifname[IFNAMSIZ];
	MIB_CE_ATM_VC_T vcEntry;

	if(pIfname == NULL) return -1;
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
		{
			printf("Get chain record error!\n");
			return -1;
		}

		if (vcEntry.enable == 0)
			continue;

		ifGetName(vcEntry.ifIndex,ifname,sizeof(ifname));

		if(!strcmp(ifname,pIfname)){
			break;
		}
	}

	if(i>= entryNum){
		//printf("not find this interface!\n");
		return -1;
	}

	if(pEntry) memcpy(pEntry, (void *)&vcEntry, sizeof(MIB_CE_ATM_VC_T));
	if(entry_index) *entry_index = i;

	return 0;
}

#ifdef CONFIG_SUPPORT_AUTO_DIAG
int getSimuWanEntrybyIfname(const char *pIfname, MIB_CE_ATM_VC_T *pEntry, int *entry_index)
{
	unsigned int entryNum, i;
	char ifname[IFNAMSIZ];
	MIB_CE_ATM_VC_T vcEntry;

	if(pIfname == NULL) return -1;

	entryNum = mib_chain_total(MIB_SIMU_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_SIMU_ATM_VC_TBL, i, (void *)&vcEntry))
		{
			printf("Get chain record error!\n");
			return -1;
		}

		if (vcEntry.enable == 0)
			continue;

		//ifGetName(vcEntry.ifIndex,ifname,sizeof(ifname));
		snprintf(ifname, sizeof(ifname), "ppp%d", PPP_INDEX(vcEntry.ifIndex));
		//printf("%s:%d ifname '%s', pIfname '%s'\n", __FUNCTION__, __LINE__, ifname, pIfname);

		if(!strcmp(ifname,pIfname)){
			//printf("%s:%d cmode =%d\n", __FUNCTION__, __LINE__, vcEntry.cmode);
			break;
		}
	}

	if(i>= entryNum){
		//printf("not find this interface!\n");
		return -1;
	}

	if(pEntry) memcpy(pEntry, (void *)&vcEntry, sizeof(MIB_CE_ATM_VC_T));
	if(entry_index) *entry_index = i;

	return 0;
}
#endif //CONFIG_SUPPORT_AUTO_DIAG

#ifdef CONFIG_USER_IP_QOS
int getIPQosQueueNumber(void)
{
	unsigned int ip_qos_queue_num = 0;
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	mib_get_s(MIB_IP_QOS_QUEUE_NUM, (void *)&ip_qos_queue_num, sizeof(ip_qos_queue_num));
	return ip_qos_queue_num;
#else
	unsigned char queue_num = MAX_QOS_QUEUE_NUM;
#ifdef CONFIG_RTK_OMCI_V1
	mib_get_s(MIB_OMCI_WAN_QOS_QUEUE_NUM, &queue_num, sizeof(queue_num));
#endif
	return queue_num;
#endif
}
#endif

static const char IF_UP[] = "up";
static const char IF_DOWN[] = "down";
static const char IF_NA[] = "n/a";
#ifdef EMBED
static const char PROC_NET_ATM_BR[] = "/proc/net/atm/br2684";
#ifdef CONFIG_ATM_CLIP
static const char PROC_NET_ATM_CLIP[] = "/proc/net/atm/pvc";
#endif
#ifdef CONFIG_PPP
const char PPP_CONF[] = "/var/ppp/ppp.conf";
const char PPPD_FIFO[] = "/tmp/ppp_serv_fifo";
const char PPPOA_CONF[] = "/var/ppp/pppoa.conf";
const char PPPOE_CONF[] = "/var/ppp/pppoe.conf";
const char PPP_PID[] = "/var/run/spppd.pid";
const char SPPPD[] = "/bin/spppd";
const char PPP_DOWN_S_PREFIX[] = "/var/ppp/ifdown_s";
#endif
#endif
#undef FILE_LOCK

void getServiceType(char *serviceTypeStr, int ctype)
{
#ifdef CONFIG_USER_RTK_WAN_CTYPE
		if (ctype == X_CT_SRV_TR069)
			strcpy(serviceTypeStr,"TR069");
		else if (ctype == X_CT_SRV_INTERNET)
			strcpy(serviceTypeStr,"INTERNET");
		else if (ctype == X_CT_SRV_OTHER)
			strcpy(serviceTypeStr,"Other");
		else if (ctype == X_CT_SRV_VOICE)
			strcpy(serviceTypeStr,"VOICE");
		else if (ctype == X_CT_SRV_INTERNET+X_CT_SRV_TR069)
			strcpy(serviceTypeStr,"INTERNET_TR069");
		else if (ctype == X_CT_SRV_VOICE+X_CT_SRV_TR069)
			strcpy(serviceTypeStr,"VOICE_TR069");
		else if (ctype == X_CT_SRV_VOICE+X_CT_SRV_INTERNET)
			strcpy(serviceTypeStr,"VOICE_INTERNET");
		else if (ctype == X_CT_SRV_VOICE+X_CT_SRV_INTERNET+X_CT_SRV_TR069)
			strcpy(serviceTypeStr,"VOICE_INTERNET_TR069");
		else
			strcpy(serviceTypeStr,"None");
#else
		strcpy(serviceTypeStr,"None");
#endif
}

#ifndef CONFIG_00R0
int getWanStatus(struct wstatus_info *sEntry, int max)
{
	unsigned int data, data2;
	char	buff[256], tmp1[20], tmp2[20], tmp3[20], tmp4[20];
	char	*temp;
	int in_turn=0, vccount=0, ifcount=0;
	int linkState, dslState=0, ethState=0;
#ifdef CONFIG_USER_CMD_SERVER_SIDE
	int ptmState=0;
#endif
	int i;
	FILE *fp;
#ifdef CONFIG_PPP
#ifdef CONFIG_USER_SPPPD
#if defined(EMBED)
	int spid;
#endif
#ifdef FILE_LOCK
	struct flock flpoe, flpoa;
	int fdpoe, fdpoa;
#endif
#endif //CONFIG_USER_SPPPD
#endif
	Modem_LinkSpeed vLs;
	int entryNum;
	MIB_CE_ATM_VC_T tEntry;
	struct wstatus_info vcEntry[MAX_VC_NUM];
	char tmp[8];

#ifdef CONFIG_USER_RTK_BRIDGE_MODE
	if(getOpMode()==BRIDGE_MODE)
		return 0;
#endif

	memset(sEntry, 0, sizeof(struct wstatus_info)*max);
	memset(vcEntry, 0, sizeof(struct wstatus_info)*MAX_VC_NUM);
#if defined(EMBED) && defined(CONFIG_PPP)
#ifdef CONFIG_USER_SPPPD
	// get spppd pid
	spid = 0;
	if ((fp = fopen(PPP_PID, "r"))) {
		if(fscanf(fp, "%d\n", &spid) != 1)
			printf("fscanf failed: %s %d\n", __func__, __LINE__);
		fclose(fp);
	}
	else
		printf("spppd pidfile not exists\n");

	if (spid) {
		struct data_to_pass_st msg;
		snprintf(msg.data, BUF_SIZE, "spppctl pppstatus %d", spid);
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
	}
#endif // CONFIG_USER_SPPPD
#endif
	in_turn = 0;
#ifdef EMBED
#ifdef CONFIG_ATM_BR2684
//#ifdef CONFIG_RTL_MULTI_PVC_WAN
#if 1
	if( WAN_MODE & MODE_ATM )
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
				continue;

			if(MEDIA_INDEX(tEntry.ifIndex) != MEDIA_ATM)
				continue;
			//czpinging, skipped the entry with Disabled Admin status
			if(tEntry.enable==0)
				continue;

			vcEntry[vccount].ifIndex = tEntry.ifIndex;
			ifGetName(tEntry.ifIndex, vcEntry[vccount].ifname, IFNAMSIZ);
			ifGetName(PHY_INTF(tEntry.ifIndex), vcEntry[vccount].devname, IFNAMSIZ);
			vcEntry[vccount].tvpi = tEntry.vpi;
			vcEntry[vccount].tvci = tEntry.vci;

			switch(tEntry.encap) {
				case ENCAP_VCMUX:
					strcpy(vcEntry[vccount].encaps, "VCMUX");
					break;
				case ENCAP_LLC:
					strcpy(vcEntry[vccount].encaps, "LLC");
					break;
				default:
					break;
			}

			vcEntry[vccount].cmode = tEntry.cmode;
			switch(tEntry.cmode) {
				case CHANNEL_MODE_IPOE:
					strcpy(vcEntry[vccount].protocol, "IPoE");
					break;
				case CHANNEL_MODE_BRIDGE:
					strcpy(vcEntry[vccount].protocol, "Bridged");
					break;
				case CHANNEL_MODE_PPPOE:
					strcpy(vcEntry[vccount].protocol, "PPPoE");
					break;
				case CHANNEL_MODE_PPPOA:
					strcpy(vcEntry[vccount].protocol, "PPPoA");
					break;
				case CHANNEL_MODE_RT1483:
					strcpy(vcEntry[vccount].protocol, "RT1483");
					break;
				case CHANNEL_MODE_RT1577:
					strcpy(vcEntry[vccount].protocol, "RT1577");
					break;
				case CHANNEL_MODE_6RD:
					strcpy(vcEntry[vccount].protocol, "6rd");
					break;
				case CHANNEL_MODE_6IN4:
					strcpy(vcEntry[vccount].protocol, "6in4");
					break;
				case CHANNEL_MODE_6TO4:
					strcpy(vcEntry[vccount].protocol, "6to4");
					break;
				default:
					break;
			}
			strcpy(vcEntry[vccount].vpivci, "---");
			vccount++;
		}
	}
#else
	if (!(fp=fopen(PROC_NET_ATM_BR, "r")))
		printf("%s not exists.\n", PROC_NET_ATM_BR);
	else {
		while ( fgets(buff, sizeof(buff), fp) != NULL ) {
			if (in_turn==0)
				if(sscanf(buff, "%*s%s", tmp1)!=1) {
					printf("Unsuported pvc configuration format\n");
					break;
				}
				else {
					vccount ++;
					tmp1[strlen(tmp1)-1]='\0';
					strcpy(vcEntry[vccount-1].ifname, tmp1);
					strcpy(vcEntry[vccount-1].devname, tmp1);
				}
			else
				if(sscanf(buff, "%*s%s%*s%s", tmp1, tmp2)!=2) {
					printf("Unsuported pvc configuration format\n");
					break;
				}
				else {
					sscanf(tmp1, "0.%u.%u:", &vcEntry[vccount-1].tvpi, &vcEntry[vccount-1].tvci);
					sscanf(tmp2, "%u,", &data);
					strcpy(vcEntry[vccount-1].protocol, "");
					if (data==1 || data == 4)
						strcpy(vcEntry[vccount-1].encaps, "LLC");
					else if (data==0 || data==3)
						strcpy(vcEntry[vccount-1].encaps, "VCMUX");
					if (data==3 || data==4)
						strcpy(vcEntry[vccount-1].protocol, "rt1483");
					strcpy(vcEntry[vccount-1].vpivci, "---");
				}
			in_turn ^= 0x01;
		}
		fclose(fp);
	}
#endif
#endif
#ifdef CONFIG_ATM_CLIP
	if (!(fp=fopen(PROC_NET_ATM_CLIP, "r")))
		printf("%s not exists.\n", PROC_NET_ATM_CLIP);
	else {
		fgets(buff, sizeof(buff), fp);
		while ( fgets(buff, sizeof(buff), fp) != NULL ) {
			char *p = strstr(buff, "CLIP");
			if (p != NULL) {
				if (sscanf(buff, "%*d%u%u%*d%*d%*s%*d%*s%*s%s%s", &data, &data2, tmp1, tmp2) != 4) {
					printf("Unsuported 1577 configuration format\n");
					break;
				}
				else {
					vccount ++;
					sscanf(tmp1, "Itf:%s", tmp3);
					strcpy(vcEntry[vccount-1].ifname, strtok(tmp3, ","));
					strcpy(vcEntry[vccount-1].devname, vcEntry[vccount-1].ifname);
					sscanf(tmp2, "Encap:%s", tmp4);
					strcpy(vcEntry[vccount-1].encaps, strtok(tmp4, "/"));
					strcpy(vcEntry[vccount-1].protocol, "rt1577");
					vcEntry[vccount-1].tvpi = data;
					vcEntry[vccount-1].tvci = data2;
					strcpy(vcEntry[vccount-1].vpivci, "---");
				}
			}
		}
		fclose(fp);
	}
#endif


#ifdef CONFIG_PTMWAN
	if( WAN_MODE & MODE_PTM )
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
				continue;

			if(MEDIA_INDEX(tEntry.ifIndex) != MEDIA_PTM)
				continue;

			//czpinging, skipped the entry with Disabled Admin status
			if(tEntry.enable==0)
				continue;

			vcEntry[vccount].ifIndex = tEntry.ifIndex;
			ifGetName(tEntry.ifIndex, vcEntry[vccount].ifname, IFNAMSIZ);
			ifGetName(PHY_INTF(tEntry.ifIndex), vcEntry[vccount].devname, IFNAMSIZ);
			strcpy(vcEntry[vccount].encaps, "---");
			vcEntry[vccount].cmode = tEntry.cmode;
			switch(tEntry.cmode) {
				case CHANNEL_MODE_IPOE:
					strcpy(vcEntry[vccount].protocol, "IPoE");
					break;
				case CHANNEL_MODE_BRIDGE:
					strcpy(vcEntry[vccount].protocol, "Bridged");
					break;
				case CHANNEL_MODE_PPPOE:
					strcpy(vcEntry[vccount].protocol, "PPPoE");
					break;
				case CHANNEL_MODE_6RD:
					strcpy(vcEntry[vccount].protocol, "6rd");
					break;
				case CHANNEL_MODE_6IN4:
					strcpy(vcEntry[vccount].protocol, "6in4");
					break;
				case CHANNEL_MODE_6TO4:
					strcpy(vcEntry[vccount].protocol, "6to4");
					break;
				default:
					break;
			}
			strcpy(vcEntry[vccount].vpivci, "---");
			vccount++;
		}
	}
#endif // CONFIG_PTMWAN


#ifdef CONFIG_ETHWAN
	if( WAN_MODE & MODE_Ethernet )
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
				continue;

			if(MEDIA_INDEX(tEntry.ifIndex) != MEDIA_ETH)
				continue;

			//czpinging, skipped the entry with Disabled Admin status
			if(tEntry.enable==0)
				continue;

			if(tEntry.applicationtype & X_CT_SRV_PON_PPTP)
				continue;

			vcEntry[vccount].ifIndex = tEntry.ifIndex;
			ifGetName(tEntry.ifIndex, vcEntry[vccount].ifname, IFNAMSIZ);
			ifGetName(PHY_INTF(tEntry.ifIndex), vcEntry[vccount].devname, IFNAMSIZ);
			strcpy(vcEntry[vccount].encaps, "---");
			vcEntry[vccount].cmode = tEntry.cmode;
			switch(tEntry.cmode) {
				case CHANNEL_MODE_IPOE:
					strcpy(vcEntry[vccount].protocol, "IPoE");
					break;
				case CHANNEL_MODE_BRIDGE:
					strcpy(vcEntry[vccount].protocol, "Bridged");
#ifdef USE_DEONET
					//if (deo_wan_nat_mode_get() != DEO_NAT_MODE)
					snprintf(vcEntry[vccount].ifname, sizeof(vcEntry[vccount].ifname), "%s", "br0");
					snprintf(vcEntry[vccount].devname, sizeof(vcEntry[vccount].devname), "%s", "br0");
#endif
					break;
				case CHANNEL_MODE_PPPOE:
					strcpy(vcEntry[vccount].protocol, "PPPoE");
					break;
				case CHANNEL_MODE_6RD:
					strcpy(vcEntry[vccount].protocol, "6rd");
					break;
				case CHANNEL_MODE_6IN4:
					strcpy(vcEntry[vccount].protocol, "6in4");
					break;
				case CHANNEL_MODE_6TO4:
					strcpy(vcEntry[vccount].protocol, "6to4");
					break;
				default:
					break;
			}
			strcpy(vcEntry[vccount].vpivci, "---");
			vccount++;
		}
	}
#endif // CONFIG_ETHWAN

#ifdef WLAN_WISP
	if( WAN_MODE & MODE_Wlan )
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
				continue;

			if(MEDIA_INDEX(tEntry.ifIndex) != MEDIA_WLAN)
				continue;

			//czpinging, skipped the entry with Disabled Admin status
			if(tEntry.enable==0)
				continue;

			vcEntry[vccount].ifIndex = tEntry.ifIndex;
			ifGetName(tEntry.ifIndex, vcEntry[vccount].ifname, IFNAMSIZ);
			ifGetName(PHY_INTF(tEntry.ifIndex), vcEntry[vccount].devname, IFNAMSIZ);
			strcpy(vcEntry[vccount].encaps, "---");
			vcEntry[vccount].cmode = tEntry.cmode;
			switch(tEntry.cmode) {
				case CHANNEL_MODE_IPOE:
					strcpy(vcEntry[vccount].protocol, "IPoE");
					break;
				case CHANNEL_MODE_BRIDGE:
					strcpy(vcEntry[vccount].protocol, "Bridged");
					break;
				case CHANNEL_MODE_PPPOE:
					strcpy(vcEntry[vccount].protocol, "PPPoE");
					break;
				default:
					break;
			}
			strcpy(vcEntry[vccount].vpivci, "---");
			vccount++;
		}
	}
#endif

#ifdef CONFIG_PPP
#ifdef CONFIG_USER_SPPPD
#ifdef FILE_LOCK
	// file locking
	fdpoe = open(PPPOE_CONF, O_RDWR);
	if (fdpoe != -1) {
		flpoe.l_type = F_RDLCK;
		flpoe.l_whence = SEEK_SET;
		flpoe.l_start = 0;
		flpoe.l_len = 0;
		flpoe.l_pid = getpid();
		if (fcntl(fdpoe, F_SETLKW, &flpoe) == -1)
			printf("pppoe read lock failed\n");
	}

	fdpoa = open(PPPOA_CONF, O_RDWR);
	if (fdpoa != -1) {
		flpoa.l_type = F_RDLCK;
		flpoa.l_whence = SEEK_SET;
		flpoa.l_start = 0;
		flpoa.l_len = 0;
		flpoa.l_pid = getpid();
		if (fcntl(fdpoa, F_SETLKW, &flpoa) == -1)
			printf("pppoa read lock failed\n");
	}
#endif

#ifdef CONFIG_DEV_xDSL
	if (!(fp=fopen(PPPOA_CONF, "r")))
		printf("%s not exists.\n", PPPOA_CONF);
	else {
		fgets(buff, sizeof(buff), fp);
		while ( fgets(buff, sizeof(buff), fp) != NULL )
			if (sscanf(buff, "%s%u%u%*s%s%*s%*d%*d%*d%s%s", tmp1, &data, &data2, tmp2, tmp3, tmp4) != 6) {
				printf("Unsuported pppoa configuration format\n");
				break;
			}
			else {
				ifcount++;
				// ifIndex --- ppp index(no vc index)
				sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, tmp1[3]-'0', DUMMY_VC_INDEX);
				strcpy(sEntry[ifcount-1].ifname, tmp1);
				strcpy(sEntry[ifcount-1].encaps, tmp2);
				strcpy(sEntry[ifcount-1].protocol, "PPPoA");
				sEntry[ifcount-1].tvpi = data;
				sEntry[ifcount-1].tvci = data2;
				sprintf(sEntry[ifcount-1].vpivci, "%u/%u", sEntry[ifcount-1].tvpi, sEntry[ifcount-1].tvci);
				strcpy(sEntry[ifcount-1].uptime, tmp3);
				strcpy(sEntry[ifcount-1].totaluptime, tmp4);
			}
		fclose(fp);
	}
#endif

	if (!(fp=fopen(PPPOE_CONF, "r")))
		printf("%s not exists.\n", PPPOE_CONF);
	else {
		fgets(buff, sizeof(buff), fp);
		while ( fgets(buff, sizeof(buff), fp) != NULL )
			if(sscanf(buff, "%s%s%*s%*s%*s%s%s", tmp1, tmp2, tmp3, tmp4) != 4) {
				printf("Unsuported pppoe configuration format\n");
				break;
			}
			else
				for (i=0; i<vccount; i++)
//#ifdef CONFIG_RTL_MULTI_PVC_WAN
					if (strcmp(vcEntry[i].ifname, tmp1) == 0) // based on pppX
//#else
//					if (strcmp(vcEntry[i].devname, tmp2) == 0)
//#endif
					{
						ifcount++;
						// ifIndex --- ppp index + vc index
						if (!strncmp(vcEntry[i].devname, ALIASNAME_VC, strlen(ALIASNAME_VC)))
						{
#ifdef CONFIG_RTL_MULTI_PVC_WAN
							sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, tmp1[strlen(ALIASNAME_PPP)]-'0', ((((vcEntry[i].devname[strlen(ALIASNAME_VC)]-'0') << 4) & 0xf0) | ((vcEntry[i].devname[strlen(ALIASNAME_VC)+2]-'0') & 0x0f)) );
#else
							sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, tmp1[strlen(ALIASNAME_PPP)]-'0', vcEntry[i].devname[strlen(ALIASNAME_VC)]-'0');
#endif
							sEntry[ifcount-1].tvpi = vcEntry[i].tvpi;
							sEntry[ifcount-1].tvci = vcEntry[i].tvci;
							sprintf(sEntry[ifcount-1].vpivci, "%u/%u", sEntry[ifcount-1].tvpi, sEntry[ifcount-1].tvci);
							//printf("***** sEntry[ifcount-1].ifIndex=0x%x\n", sEntry[ifcount-1].ifIndex);
						}
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN) || defined (WLAN_WISP)
						else {
							sEntry[ifcount-1].ifIndex = vcEntry[i].ifIndex;
							strcpy(sEntry[ifcount-1].vpivci, "---");
						}
#endif
						strcpy(sEntry[ifcount-1].ifname, tmp1);
#ifdef CONFIG_RTL_MULTI_PVC_WAN
						strcpy(sEntry[ifcount-1].devname, tmp2);
#else
						strcpy(sEntry[ifcount-1].devname, vcEntry[i].devname);
#endif
						strcpy(sEntry[ifcount-1].encaps, vcEntry[i].encaps);
						strcpy(sEntry[ifcount-1].protocol, "PPPoE");
						strcpy(sEntry[ifcount-1].uptime, tmp3);
						strcpy(sEntry[ifcount-1].totaluptime, tmp4);
						break;
					}
		fclose(fp);
	}
#ifdef FILE_LOCK
	// file unlocking
	if (fdpoe != -1) {
		flpoe.l_type = F_UNLCK;
		if (fcntl(fdpoe, F_SETLK, &flpoe) == -1)
			printf("pppoe read unlock failed\n");
		close(fdpoe);
	}
	if (fdpoa != -1) {
		flpoa.l_type = F_UNLCK;
		if (fcntl(fdpoa, F_SETLK, &flpoa) == -1)
			printf("pppoa read unlock failed\n");
		close(fdpoa);
	}
#endif
#endif	//CONFIG_USER_SPPPD
#ifdef CONFIG_USER_PPPD
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
			continue;

		if (tEntry.cmode != CHANNEL_MODE_PPPOE && tEntry.cmode != CHANNEL_MODE_PPPOA)
			continue;

		if (((MEDIA_INDEX(tEntry.ifIndex) == MEDIA_ETH) && (WAN_MODE & MODE_Ethernet))
#ifdef CONFIG_PTMWAN
			 ||((MEDIA_INDEX(tEntry.ifIndex) == MEDIA_PTM) && (WAN_MODE & MODE_PTM))
#endif /*CONFIG_PTMWAN*/
		)
			continue;

		//czpinging, skipped the entry with Disabled Admin status
		if (tEntry.enable==0)
			continue;

		ifcount++;
		// ifIndex --- ppp index(no vc index)
		sEntry[ifcount-1].ifIndex = tEntry.ifIndex;
		getDisplayWanName(&tEntry, sEntry[ifcount-1].ifDisplayName);
		ifGetName(tEntry.ifIndex, tmp, sizeof(tmp));
		strcpy(sEntry[ifcount-1].ifname, tmp);
		ifGetName(PHY_INTF(tEntry.ifIndex), tmp, sizeof(tmp));
		strcpy(sEntry[ifcount-1].devname, tmp);
		switch(tEntry.encap) {
			case ENCAP_VCMUX:
				strcpy(sEntry[ifcount-1].encaps, "VCMUX");
				break;
			case ENCAP_LLC:
				strcpy(sEntry[ifcount-1].encaps, "LLC");
				break;
			default:
				break;
		}
		switch(tEntry.cmode) {
			case CHANNEL_MODE_PPPOE:
				strcpy(sEntry[ifcount-1].protocol, "PPPoE");
				break;
			case CHANNEL_MODE_PPPOA:
				strcpy(sEntry[ifcount-1].protocol, "PPPoA");
				break;
			default:
				break;
		}
		sEntry[ifcount-1].tvpi = tEntry.vpi;
		sEntry[ifcount-1].tvci = tEntry.vci;
		sprintf(sEntry[ifcount-1].vpivci, "%u/%u", sEntry[ifcount-1].tvpi, sEntry[ifcount-1].tvci);
		strcpy(sEntry[ifcount-1].uptime, tmp3);
		strcpy(sEntry[ifcount-1].totaluptime, tmp4);
	}
#endif //CONFIG_USER_PPPD
#endif

	for (i=0; i<vccount; i++) {
		int j, vcfound=0;
		for (j=0; j<ifcount; j++) {
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			if (strcmp(vcEntry[i].ifname, sEntry[j].ifname) == 0)	// PPPoE-used device
#else
			if (strcmp(vcEntry[i].devname, sEntry[j].devname) == 0)	// PPPoE-used device
#endif
			{
				vcfound = 1;
				break;
			}
			if (vcEntry[i].cmode == CHANNEL_MODE_PPPOA) {
				vcfound = 1;
				break;
			}
		}
		if (!vcfound) {	// VC not used for PPPoA/PPPoE, add to list
			ifcount++;
			// ifIndex --- vc index (no ppp index)
			if (!strncmp(vcEntry[i].devname, ALIASNAME_VC, strlen(ALIASNAME_VC))) {
#ifdef CONFIG_RTL_MULTI_PVC_WAN
				if (!strcmp(vcEntry[i].protocol, "PPPoA") || !strcmp(vcEntry[i].protocol, "RT1483") || !strcmp(vcEntry[i].protocol, "RT1577"))
					sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, DUMMY_PPP_INDEX, ((((vcEntry[i].devname[2]-'0') << 4) & 0xf0) | DUMMY_VC_MINOR_INDEX) );
				else
					sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, DUMMY_PPP_INDEX, ((((vcEntry[i].devname[strlen(ALIASNAME_VC)]-'0') << 4) & 0xf0) | ((vcEntry[i].devname[strlen(ALIASNAME_VC)+2]-'0') & 0x0f)) );
#else
				sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, DUMMY_PPP_INDEX, vcEntry[i].ifname[strlen(ALIASNAME_VC)]-'0');
#endif
				sEntry[ifcount-1].tvpi = vcEntry[i].tvpi;
				sEntry[ifcount-1].tvci = vcEntry[i].tvci;
				sprintf(sEntry[ifcount-1].vpivci, "%u/%u", sEntry[ifcount-1].tvpi, sEntry[ifcount-1].tvci);
			}
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN) || defined (WLAN_WISP)
			else {
				sEntry[ifcount-1].ifIndex = vcEntry[i].ifIndex;
				strcpy(sEntry[ifcount-1].vpivci, "---");
			}
#endif
			strcpy(sEntry[ifcount-1].ifname, vcEntry[i].ifname);
			strcpy(sEntry[ifcount-1].devname, vcEntry[i].devname);
			strcpy(sEntry[ifcount-1].encaps, vcEntry[i].encaps);
			strcpy(sEntry[ifcount-1].protocol, vcEntry[i].protocol);
		}
	}

#endif

#ifdef CONFIG_DEV_xDSL
#ifndef CONFIG_USER_CMD_SERVER_SIDE
	// check for xDSL link
	if (!adsl_drv_get(RLCM_GET_LINK_SPEED, (void *)&vLs, RLCM_GET_LINK_SPEED_SIZE) || vLs.upstreamRate == 0)
		dslState = 0;
	else
		dslState = 1;
#else
	if(get_net_link_status("atm0")==1)
		dslState = 1;
	else
		dslState = 0;

	if(get_net_link_status("ptm0")==1)
		ptmState = 1;
	else
		ptmState = 0;
#endif
#endif
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN)
	// todo
	ethState = 1;
#endif

	if (ifcount > max)
		printf("WARNNING! status list overflow(%d).\n", ifcount);


	for (i=0; i<ifcount; i++) {
		struct in_addr inAddr;
		int flags;
		int totalNum, k;
		MIB_CE_ATM_VC_T entry;
		MEDIA_TYPE_T mType;
#ifdef CONFIG_IPV6
		struct ipv6_ifaddr ip6_addr = {0};
#endif

#ifdef EMBED
		// Kaohj --- interface name to be displayed
		totalNum = mib_chain_total(MIB_ATM_VC_TBL);

		for(k=0; k<totalNum; k++)
		{
			mib_chain_get(MIB_ATM_VC_TBL, k, (void *)&entry);

			if (sEntry[i].ifIndex == entry.ifIndex) {
				getDisplayWanName(&entry, sEntry[i].ifDisplayName);
				sEntry[i].cmode = entry.cmode;
#ifdef CONFIG_IPV6
				sEntry[i].ipver = entry.IpProtocol;
#endif
				sEntry[i].recordNum = k;
#if defined(CONFIG_LUNA)
				sEntry[i].vid = entry.vid;
				getServiceType(sEntry[i].serviceType,entry.applicationtype);
#endif
				break;
			}

		}
		if (getInAddr( sEntry[i].ifname, IP_ADDR, (void *)&inAddr) == 1)
		{
			temp = inet_ntoa(inAddr);
			if (getInFlags( sEntry[i].ifname, &flags) == 1)
				if ((strcmp(temp, "10.0.0.1") == 0) && flags & IFF_POINTOPOINT)	// IP Passthrough or IP unnumbered
					strcpy(sEntry[i].ipAddr, STR_UNNUMBERED);
				else if (strcmp(temp, "64.64.64.64") == 0)
					strcpy(sEntry[i].ipAddr, "");
				else{
					strcpy(sEntry[i].ipAddr, temp);
					getInAddr( sEntry[i].ifname, SUBNET_MASK, (void *)&inAddr);
					temp = inet_ntoa(inAddr);
					strcpy(sEntry[i].mask, temp);
				}
		}
		else
#endif
			strcpy(sEntry[i].ipAddr, "");

#ifdef EMBED
		if (getInAddr( sEntry[i].ifname, DST_IP_ADDR, (void *)&inAddr) == 1)
		{
			temp = inet_ntoa(inAddr);
			if (strcmp(temp, "10.0.0.2") == 0)
				strcpy(sEntry[i].remoteIp, STR_UNNUMBERED);
			else if (strcmp(temp, "64.64.64.64") == 0)
				strcpy(sEntry[i].remoteIp, "");
			else
				strcpy(sEntry[i].remoteIp, temp);
			if (getInFlags( sEntry[i].ifname, &flags) == 1)
				if (flags & IFF_BROADCAST) {
					unsigned char value[32];
					snprintf(value, 32, "%s.%s", (char *)MER_GWINFO, sEntry[i].ifname);
					if (fp = fopen(value, "r")) {
						fscanf(fp, "%s\n", sEntry[i].remoteIp);
						//strcpy(sEntry[i].protocol, "mer1483");
						fclose(fp);
					}
					else
						strcpy(sEntry[i].remoteIp, "");
				}

			// special case for DHCP WAN without router option
			if(strcmp(sEntry[i].remoteIp, sEntry[i].ipAddr) == 0)
				strcpy(sEntry[i].remoteIp, "");
		}
		else
#endif
			strcpy(sEntry[i].remoteIp, "");

		if (!strcmp(sEntry[i].protocol, ""))
		{
			//get channel mode
			switch(sEntry[i].cmode) {
			case CHANNEL_MODE_IPOE:
				strcpy(sEntry[i].protocol, "mer1483");
				break;
			case CHANNEL_MODE_BRIDGE:
				strcpy(sEntry[i].protocol, "br1483");
				break;
			case CHANNEL_MODE_RT1483:
				strcpy(sEntry[i].protocol, "rt1483");
				break;
			case CHANNEL_MODE_6RD:
				strcpy(sEntry[i].protocol, "6rd");
				break;
			case CHANNEL_MODE_6IN4:
				strcpy(sEntry[i].protocol, "6in4");
				break;
			case CHANNEL_MODE_6TO4:
				strcpy(sEntry[i].protocol, "6to4");
				break;
			default:
				break;
			}
		}

		mType = MEDIA_INDEX(sEntry[i].ifIndex);
		if (mType == MEDIA_ATM)
			linkState = dslState;
		#ifdef CONFIG_PTMWAN
		else if (mType == MEDIA_PTM)
#ifndef CONFIG_USER_CMD_SERVER_SIDE
			linkState = dslState && ethState;//???
#else
			linkState = ptmState;
#endif
		#endif /*CONFIG_PTMWAN*/
		else if (mType == MEDIA_ETH)
			linkState = ethState;
#ifdef WLAN_WISP
		else if (mType == MEDIA_WLAN){
			char wisp_name[16];
			getWispWanName(wisp_name, ETH_INDEX(sEntry[i].ifIndex));
			linkState = get_net_link_status(wisp_name);
		}
#endif
		else
			linkState = 0;
		sEntry[i].link_state = linkState;
		// set status flag
		sEntry[i].strStatus = (char *)IF_DOWN;
		sEntry[i].strStatus_IPv6 = (char *)IF_DOWN;
		sEntry[i].itf_state = 0;
		if (getInFlags( sEntry[i].ifname, &flags) == 1)
		{
			if ((flags & IFF_UP) && (flags & IFF_RUNNING)) {
				if (linkState) {
					if (sEntry[i].cmode == CHANNEL_MODE_BRIDGE) {
						sEntry[i].strStatus = (char *)IF_UP;
						sEntry[i].strStatus_IPv6 = (char *)IF_UP;
						sEntry[i].itf_state = 1;
					} else {
						if ((sEntry[i].ipver | IPVER_IPV4) &&
								getInAddr(sEntry[i].ifname, IP_ADDR, (void *)&inAddr) == 1) {
							temp = inet_ntoa(inAddr);
							if (strcmp(temp, "64.64.64.64")) {
								sEntry[i].strStatus = (char *)IF_UP;
								sEntry[i].itf_state = 1;
							}
						}
#ifdef CONFIG_IPV6
						if (sEntry[i].ipver | IPVER_IPV6) {
							if(getifip6(sEntry[i].ifname, IPV6_ADDR_UNICAST, &ip6_addr, 1) > 0)
								sEntry[i].strStatus_IPv6 = (char *)IF_UP;
						}
#endif
					}
				}
			}
		} else {
#ifdef CONFIG_USER_PPPD
			if (sEntry[i].cmode == CHANNEL_MODE_PPPOE || sEntry[i].cmode == CHANNEL_MODE_PPPOA) {
				sEntry[i].strStatus = (char *)IF_DOWN;
				sEntry[i].strStatus_IPv6 = (char *)IF_DOWN;
				sEntry[i].itf_state = 0;
			}
			else {
#endif
			sEntry[i].strStatus = (char *)IF_NA;
			sEntry[i].strStatus_IPv6 = (char *)IF_NA;
			sEntry[i].itf_state = -1;
#ifdef CONFIG_USER_PPPD
			}
#endif
		}

		if (sEntry[i].cmode == CHANNEL_MODE_PPPOE || sEntry[i].cmode == CHANNEL_MODE_PPPOA) {
			if (sEntry[i].itf_state <= 0) {
				sEntry[i].ipAddr[0] = '\0';
				sEntry[i].remoteIp[0] = '\0';
			}
			if (entry.pppCtype == CONNECT_ON_DEMAND && entry.pppIdleTime != 0)
				sEntry[i].pppDoD = 1;
			else
				sEntry[i].pppDoD = 0;
		}

	}
	return ifcount;
}
#else
#include <omci_api.h>

int get_OMCI_TR69_WAN_VLAN(int *vid, int *pri)
{
	int ret = 0;
 	PON_OMCI_CMD_T msg;

	memset(&msg, 0, sizeof(msg));
	//msg.cmd = PON_OMCI_CMD_TR69_WAN_VLAN_GET;

	if (omci_SendCmdAndGet(&msg) == GOS_OK) {
		printf("%s vid[%d] pri[%d]\n", __func__, msg.vid, msg.pri);
		*vid = msg.vid;
		*pri = msg.pri;
		ret = 1;
	}
	else
		ret = 0;

	return ret;
}

int getWanStatus(struct wstatus_info *sEntry, int max)
{
	unsigned int data, data2;
	char	buff[256], tmp1[20], tmp2[20], tmp3[20], tmp4[20];
	char	*temp;
	int in_turn=0, vccount=0, ifcount=0;
	int linkState, dslState=0, ethState=0;
	int i;
	FILE *fp;
#ifdef CONFIG_PPP
#if defined(EMBED)
	int spid;
#endif
#ifdef FILE_LOCK
	struct flock flpoe, flpoa;
	int fdpoe, fdpoa;
#endif
#endif
	Modem_LinkSpeed vLs;
	int entryNum;
	MIB_CE_ATM_VC_T tEntry;
	struct wstatus_info vcEntry[MAX_VC_NUM];

	memset(sEntry, 0, sizeof(struct wstatus_info)*max);
	memset(vcEntry, 0, sizeof(struct wstatus_info)*MAX_VC_NUM);
#if defined(EMBED) && defined(CONFIG_PPP)
	// get spppd pid
	spid = 0;
	if ((fp = fopen(PPP_PID, "r"))) {
		fscanf(fp, "%d\n", &spid);
		fclose(fp);
	}
	else
		printf("spppd pidfile not exists\n");

	if (spid) {
		struct data_to_pass_st msg;
		snprintf(msg.data, BUF_SIZE, "spppctl pppstatus %d", spid);
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
	}
#endif
	in_turn = 0;
#ifdef EMBED
#ifdef CONFIG_ATM_BR2684
#ifdef CONFIG_RTL_MULTI_PVC_WAN
	if( WAN_MODE & MODE_ATM )
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
				continue;

			if(MEDIA_INDEX(tEntry.ifIndex) != MEDIA_ATM)
				continue;
			//czpinging, skipped the entry with Disabled Admin status
			if(tEntry.enable==0)
				continue;

			vcEntry[vccount].ifIndex = tEntry.ifIndex;
			ifGetName(tEntry.ifIndex, vcEntry[vccount].ifname, IFNAMSIZ);
			ifGetName(PHY_INTF(tEntry.ifIndex), vcEntry[vccount].devname, IFNAMSIZ);
			vcEntry[vccount].tvpi = tEntry.vpi;
			vcEntry[vccount].tvci = tEntry.vci;

			switch(tEntry.encap) {
				case ENCAP_VCMUX:
					strcpy(vcEntry[vccount].encaps, "VCMUX");
					break;
				case ENCAP_LLC:
					strcpy(vcEntry[vccount].encaps, "LLC");
					break;
				default:
					break;
			}

			switch(tEntry.cmode) {
				case CHANNEL_MODE_IPOE:
					strcpy(vcEntry[vccount].protocol, "IPoE");
					break;
				case CHANNEL_MODE_BRIDGE:
					strcpy(vcEntry[vccount].protocol, "Bridged");
					break;
				case CHANNEL_MODE_PPPOE:
					strcpy(vcEntry[vccount].protocol, "PPPoE");
					break;
				case CHANNEL_MODE_PPPOA:
					strcpy(vcEntry[vccount].protocol, "PPPoA");
					break;
				case CHANNEL_MODE_RT1483:
					strcpy(vcEntry[vccount].protocol, "RT1483");
					break;
				case CHANNEL_MODE_RT1577:
					strcpy(vcEntry[vccount].protocol, "RT1577");
					break;
				case CHANNEL_MODE_6RD:
					strcpy(vcEntry[vccount].protocol, "6rd");
					break;
				case CHANNEL_MODE_6IN4:
					strcpy(vcEntry[vccount].protocol, "6in4");
					break;
				case CHANNEL_MODE_6TO4:
					strcpy(vcEntry[vccount].protocol, "6to4");
					break;
				default:
					break;
			}
			strcpy(vcEntry[vccount].vpivci, "---");
			vccount++;
		}
	}
#else
	if (!(fp=fopen(PROC_NET_ATM_BR, "r")))
		printf("%s not exists.\n", PROC_NET_ATM_BR);
	else {
		while ( fgets(buff, sizeof(buff), fp) != NULL ) {
			if (in_turn==0)
				if(sscanf(buff, "%*s%s", tmp1)!=1) {
					printf("Unsuported pvc configuration format\n");
					break;
				}
				else {
					vccount ++;
					tmp1[strlen(tmp1)-1]='\0';
					strcpy(vcEntry[vccount-1].ifname, tmp1);
					strcpy(vcEntry[vccount-1].devname, tmp1);
				}
			else
				if(sscanf(buff, "%*s%s%*s%s", tmp1, tmp2)!=2) {
					printf("Unsuported pvc configuration format\n");
					break;
				}
				else {
					sscanf(tmp1, "0.%u.%u:", &vcEntry[vccount-1].tvpi, &vcEntry[vccount-1].tvci);
					sscanf(tmp2, "%u,", &data);
					strcpy(vcEntry[vccount-1].protocol, "");
					if (data==1 || data == 4)
						strcpy(vcEntry[vccount-1].encaps, "LLC");
					else if (data==0 || data==3)
						strcpy(vcEntry[vccount-1].encaps, "VCMUX");
					if (data==3 || data==4)
						strcpy(vcEntry[vccount-1].protocol, "rt1483");
					strcpy(vcEntry[vccount-1].vpivci, "---");
				}
			in_turn ^= 0x01;
		}
		fclose(fp);
	}
#endif
#endif
#ifdef CONFIG_ATM_CLIP
	if (!(fp=fopen(PROC_NET_ATM_CLIP, "r")))
		printf("%s not exists.\n", PROC_NET_ATM_CLIP);
	else {
		fgets(buff, sizeof(buff), fp);
		while ( fgets(buff, sizeof(buff), fp) != NULL ) {
			char *p = strstr(buff, "CLIP");
			if (p != NULL) {
				if (sscanf(buff, "%*d%u%u%*d%*d%*s%*d%*s%*s%s%s", &data, &data2, tmp1, tmp2) != 4) {
					printf("Unsuported 1577 configuration format\n");
					break;
				}
				else {
					vccount ++;
					sscanf(tmp1, "Itf:%s", tmp3);
					strcpy(vcEntry[vccount-1].ifname, strtok(tmp3, ","));
					strcpy(vcEntry[vccount-1].devname, vcEntry[vccount-1].ifname);
					sscanf(tmp2, "Encap:%s", tmp4);
					strcpy(vcEntry[vccount-1].encaps, strtok(tmp4, "/"));
					strcpy(vcEntry[vccount-1].protocol, "rt1577");
					vcEntry[vccount-1].tvpi = data;
					vcEntry[vccount-1].tvci = data2;
					strcpy(vcEntry[vccount-1].vpivci, "---");
				}
			}
		}
		fclose(fp);
	}
#endif


#ifdef CONFIG_PTMWAN
	if( WAN_MODE & MODE_PTM )
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
				continue;

			if(MEDIA_INDEX(tEntry.ifIndex) != MEDIA_PTM)
				continue;

			//czpinging, skipped the entry with Disabled Admin status
			if(tEntry.enable==0)
				continue;

			vcEntry[vccount].ifIndex = tEntry.ifIndex;
			ifGetName(tEntry.ifIndex, vcEntry[vccount].ifname, IFNAMSIZ);
			ifGetName(PHY_INTF(tEntry.ifIndex), vcEntry[vccount].devname, IFNAMSIZ);
			strcpy(vcEntry[vccount].encaps, "---");
			switch(tEntry.cmode) {
				case CHANNEL_MODE_IPOE:
					strcpy(vcEntry[vccount].protocol, "IPoE");
					break;
				case CHANNEL_MODE_BRIDGE:
					strcpy(vcEntry[vccount].protocol, "Bridged");
					break;
				case CHANNEL_MODE_PPPOE:
					strcpy(vcEntry[vccount].protocol, "PPPoE");
					break;
				case CHANNEL_MODE_6RD:
					strcpy(vcEntry[vccount].protocol, "6rd");
					break;
				case CHANNEL_MODE_6IN4:
					strcpy(vcEntry[vccount].protocol, "6in4");
					break;
				case CHANNEL_MODE_6TO4:
					strcpy(vcEntry[vccount].protocol, "6to4");
					break;
				default:
					break;
			}
			strcpy(vcEntry[vccount].vpivci, "---");
			vccount++;
		}
	}
#endif // CONFIG_PTMWAN


#ifdef CONFIG_ETHWAN
	if( WAN_MODE & MODE_Ethernet )
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
				continue;

			if(MEDIA_INDEX(tEntry.ifIndex) != MEDIA_ETH)
				continue;

			//czpinging, skipped the entry with Disabled Admin status
			if(tEntry.enable==0)
				continue;

			if(tEntry.applicationtype & X_CT_SRV_PON_PPTP)
				continue;

			vcEntry[vccount].ifIndex = tEntry.ifIndex;
			ifGetName(tEntry.ifIndex, vcEntry[vccount].ifname, IFNAMSIZ);
			ifGetName(PHY_INTF(tEntry.ifIndex), vcEntry[vccount].devname, IFNAMSIZ);
			strcpy(vcEntry[vccount].encaps, "---");
			switch(tEntry.cmode) {
				case CHANNEL_MODE_IPOE:
					strcpy(vcEntry[vccount].protocol, "IPoE");
					break;
				case CHANNEL_MODE_BRIDGE:
					strcpy(vcEntry[vccount].protocol, "Bridged");
					break;
				case CHANNEL_MODE_PPPOE:
					strcpy(vcEntry[vccount].protocol, "PPPoE");
					break;
				case CHANNEL_MODE_6RD:
					strcpy(vcEntry[vccount].protocol, "6rd");
					break;
				case CHANNEL_MODE_6IN4:
					strcpy(vcEntry[vccount].protocol, "6in4");
					break;
				case CHANNEL_MODE_6TO4:
					strcpy(vcEntry[vccount].protocol, "6to4");
					break;
				default:
					break;
			}
			strcpy(vcEntry[vccount].vpivci, "---");
			vccount++;
		}
	}
#endif // CONFIG_ETHWAN

#ifdef WLAN_WISP
	if( WAN_MODE & MODE_Wlan )
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
				continue;

			if(MEDIA_INDEX(tEntry.ifIndex) != MEDIA_WLAN)
				continue;

			//czpinging, skipped the entry with Disabled Admin status
			if(tEntry.enable==0)
				continue;

			vcEntry[vccount].ifIndex = tEntry.ifIndex;
			ifGetName(tEntry.ifIndex, vcEntry[vccount].ifname, IFNAMSIZ);
			ifGetName(PHY_INTF(tEntry.ifIndex), vcEntry[vccount].devname, IFNAMSIZ);
			strcpy(vcEntry[vccount].encaps, "---");
			switch(tEntry.cmode) {
				case CHANNEL_MODE_IPOE:
					strcpy(vcEntry[vccount].protocol, "IPoE");
					break;
				case CHANNEL_MODE_BRIDGE:
					strcpy(vcEntry[vccount].protocol, "Bridged");
					break;
				case CHANNEL_MODE_PPPOE:
					strcpy(vcEntry[vccount].protocol, "PPPoE");
					break;
				default:
					break;
			}
			strcpy(vcEntry[vccount].vpivci, "---");
			vccount++;
		}
	}
#endif

#ifdef CONFIG_PPP
#ifndef CONFIG_USER_PPPD
#ifdef FILE_LOCK
	// file locking
	fdpoe = open(PPPOE_CONF, O_RDWR);
	if (fdpoe != -1) {
		flpoe.l_type = F_RDLCK;
		flpoe.l_whence = SEEK_SET;
		flpoe.l_start = 0;
		flpoe.l_len = 0;
		flpoe.l_pid = getpid();
		if (fcntl(fdpoe, F_SETLKW, &flpoe) == -1)
			printf("pppoe read lock failed\n");
	}

	fdpoa = open(PPPOA_CONF, O_RDWR);
	if (fdpoa != -1) {
		flpoa.l_type = F_RDLCK;
		flpoa.l_whence = SEEK_SET;
		flpoa.l_start = 0;
		flpoa.l_len = 0;
		flpoa.l_pid = getpid();
		if (fcntl(fdpoa, F_SETLKW, &flpoa) == -1)
			printf("pppoa read lock failed\n");
	}
#endif

#ifdef CONFIG_DEV_xDSL
	if (!(fp=fopen(PPPOA_CONF, "r")))
		printf("%s not exists.\n", PPPOA_CONF);
	else {
		fgets(buff, sizeof(buff), fp);
		while ( fgets(buff, sizeof(buff), fp) != NULL )
			if (sscanf(buff, "%s%u%u%*s%s%*s%*d%*d%*d%s%s", tmp1, &data, &data2, tmp2, tmp3, tmp4) != 6) {
				printf("Unsuported pppoa configuration format\n");
				break;
			}
			else {
				for (i=0; i<vccount; i++)
					if (strcmp(vcEntry[i].ifname, tmp1) == 0)
					{
						ifcount++;
						// ifIndex --- ppp index(no vc index)
						sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, tmp1[3]-'0', DUMMY_VC_INDEX);
						strcpy(sEntry[ifcount-1].ifname, tmp1);
						strcpy(sEntry[ifcount-1].encaps, tmp2);
						strcpy(sEntry[ifcount-1].protocol, "PPPoA");
						sEntry[ifcount-1].tvpi = data;
						sEntry[ifcount-1].tvci = data2;
						sprintf(sEntry[ifcount-1].vpivci, "%u/%u", sEntry[ifcount-1].tvpi, sEntry[ifcount-1].tvci);
						strcpy(sEntry[ifcount-1].uptime, tmp3);
						strcpy(sEntry[ifcount-1].totaluptime, tmp4);
						break;
					}
			}
		fclose(fp);
	}
#endif

	if (!(fp=fopen(PPPOE_CONF, "r")))
		printf("%s not exists.\n", PPPOE_CONF);
	else {
		fgets(buff, sizeof(buff), fp);
		while ( fgets(buff, sizeof(buff), fp) != NULL )
			if(sscanf(buff, "%s%s%*s%*s%*s%s%s", tmp1, tmp2, tmp3, tmp4) != 4) {
				printf("Unsuported pppoe configuration format\n");
				break;
			}
			else
				for (i=0; i<vccount; i++)
#ifdef CONFIG_RTL_MULTI_PVC_WAN
					if (strcmp(vcEntry[i].ifname, tmp1) == 0)
#else
					if (strcmp(vcEntry[i].devname, tmp2) == 0)
#endif
					{
						ifcount++;
						// ifIndex --- ppp index + vc index
						if (!strncmp(vcEntry[i].devname,"vc",2))
						{
#ifdef CONFIG_RTL_MULTI_PVC_WAN
							sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, tmp1[3]-'0', ((((vcEntry[i].devname[2]-'0') << 4) & 0xf0) | ((vcEntry[i].devname[4]-'0') & 0x0f)) );
#else
							sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, tmp1[3]-'0', vcEntry[i].devname[2]-'0');
#endif
							sEntry[ifcount-1].tvpi = vcEntry[i].tvpi;
							sEntry[ifcount-1].tvci = vcEntry[i].tvci;
							sprintf(sEntry[ifcount-1].vpivci, "%u/%u", sEntry[ifcount-1].tvpi, sEntry[ifcount-1].tvci);
							//printf("***** sEntry[ifcount-1].ifIndex=0x%x\n", sEntry[ifcount-1].ifIndex);
						}
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN) || defined (WLAN_WISP)
						else {
							sEntry[ifcount-1].ifIndex = vcEntry[i].ifIndex;
							strcpy(sEntry[ifcount-1].vpivci, "---");
						}
#endif
						strcpy(sEntry[ifcount-1].ifname, tmp1);
#ifdef CONFIG_RTL_MULTI_PVC_WAN
						strcpy(sEntry[ifcount-1].devname, tmp2);
#else
						strcpy(sEntry[ifcount-1].devname, vcEntry[i].devname);
#endif
						strcpy(sEntry[ifcount-1].encaps, vcEntry[i].encaps);
						strcpy(sEntry[ifcount-1].protocol, "PPPoE");
						strcpy(sEntry[ifcount-1].uptime, tmp3);
						strcpy(sEntry[ifcount-1].totaluptime, tmp4);
						break;
					}
		fclose(fp);
	}
#ifdef FILE_LOCK
	// file unlocking
	if ((fdpoe != -1) && (fdpoa != -1)) {
		flpoe.l_type = flpoa.l_type = F_UNLCK;
		if (fcntl(fdpoe, F_SETLK, &flpoe) == -1)
			printf("pppoe read unlock failed\n");
		if (fcntl(fdpoa, F_SETLK, &flpoa) == -1)
			printf("pppoa read unlock failed\n");
		close(fdpoe);
		close(fdpoa);
	}
#endif
#else	//CONFIG_USER_PPPD
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
			continue;

		if (MEDIA_INDEX(tEntry.cmode) != CHANNEL_MODE_PPPOE && MEDIA_INDEX(tEntry.cmode) != CHANNEL_MODE_PPPOA)
			continue;

		//czpinging, skipped the entry with Disabled Admin status
		if (tEntry.enable==0)
			continue;

		ifcount++;
		ifGetName(tEntry.ifIndex, tmp, sizeof(tmp));
		// ifIndex --- ppp index(no vc index)
		sEntry[ifcount-1].ifIndex = tEntry.ifIndex;
		getDisplayWanName(&tEntry, sEntry[ifcount-1].ifDisplayName);
		strcpy(sEntry[ifcount-1].ifname, tmp);
		switch(tEntry.encap) {
			case ENCAP_VCMUX:
				strcpy(sEntry[ifcount-1].encaps, "VCMUX");
				break;
			case ENCAP_LLC:
				strcpy(sEntry[ifcount-1].encaps, "LLC");
				break;
			default:
				break;
		}
		switch(tEntry.cmode) {
			case CHANNEL_MODE_PPPOE:
				strcpy(sEntry[ifcount-1].protocol, "PPPoE");
				break;
			case CHANNEL_MODE_PPPOA:
				strcpy(sEntry[ifcount-1].protocol, "PPPoA");
				break;
			default:
				break;
		}
		sEntry[ifcount-1].tvpi = tEntry.vpi;
		sEntry[ifcount-1].tvci = tEntry.vci;
		sprintf(sEntry[ifcount-1].vpivci, "%u/%u", sEntry[ifcount-1].tvpi, sEntry[ifcount-1].tvci);
		strcpy(sEntry[ifcount-1].uptime, tmp3);
		strcpy(sEntry[ifcount-1].totaluptime, tmp4);
	}
#endif	//CONFIG_USER_PPPD
#endif

	for (i=0; i<vccount; i++) {
		int j, vcfound=0;
		for (j=0; j<ifcount; j++) {
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			if (strcmp(vcEntry[i].ifname, sEntry[j].ifname) == 0)	// PPPoE-used device
#else
			if (strcmp(vcEntry[i].devname, sEntry[j].devname) == 0)	// PPPoE-used device
#endif
			{
				vcfound = 1;
				break;
			}
		}
		if (!vcfound) {	// VC not used for PPPoA/PPPoE, add to list
			ifcount++;
			// ifIndex --- vc index (no ppp index)
			if (!strncmp(vcEntry[i].devname,"vc",2)) {
#ifdef CONFIG_RTL_MULTI_PVC_WAN
				sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, DUMMY_PPP_INDEX, ((((vcEntry[i].devname[2]-'0') << 4) & 0xf0) | ((vcEntry[i].devname[4]-'0') & 0x0f)) );
#else
				sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, DUMMY_PPP_INDEX, vcEntry[i].ifname[2]-'0');
#endif
				sEntry[ifcount-1].tvpi = vcEntry[i].tvpi;
				sEntry[ifcount-1].tvci = vcEntry[i].tvci;
				sprintf(sEntry[ifcount-1].vpivci, "%u/%u", sEntry[ifcount-1].tvpi, sEntry[ifcount-1].tvci);
			}
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN) || defined (WLAN_WISP)
			else {
				sEntry[ifcount-1].ifIndex = vcEntry[i].ifIndex;
				strcpy(sEntry[ifcount-1].vpivci, "---");
			}
#endif
			strcpy(sEntry[ifcount-1].ifname, vcEntry[i].ifname);
			strcpy(sEntry[ifcount-1].devname, vcEntry[i].devname);
			strcpy(sEntry[ifcount-1].encaps, vcEntry[i].encaps);
			strcpy(sEntry[ifcount-1].protocol, vcEntry[i].protocol);
		}
	}

#endif

#ifdef CONFIG_DEV_xDSL
	// check for xDSL link
	if (!adsl_drv_get(RLCM_GET_LINK_SPEED, (void *)&vLs, RLCM_GET_LINK_SPEED_SIZE) || vLs.upstreamRate == 0)
		dslState = 0;
	else
		dslState = 1;
#endif
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN)
	// todo
	ethState = 1;
#endif

	if (ifcount > max)
		printf("WARNNING! status list overflow(%d).\n", ifcount);


	for (i=0; i<ifcount; i++) {
		struct in_addr inAddr;
		int flags;
		int totalNum, k;
		MIB_CE_ATM_VC_T entry;
		MEDIA_TYPE_T mType;
#ifdef CONFIG_IPV6
		struct ipv6_ifaddr ip6_addr = {0};
#endif

#ifdef EMBED
		// Kaohj --- interface name to be displayed
		totalNum = mib_chain_total(MIB_ATM_VC_TBL);

		for(k=0; k<totalNum; k++)
		{
			mib_chain_get(MIB_ATM_VC_TBL, k, (void *)&entry);

			if (sEntry[i].ifIndex == entry.ifIndex) {
				getDisplayWanName(&entry, sEntry[i].ifDisplayName);
				sEntry[i].cmode = entry.cmode;
#ifdef CONFIG_IPV6
				sEntry[i].ipver = entry.IpProtocol;
#endif
				sEntry[i].recordNum = k;


#ifdef CONFIG_00R0
				if ((entry.omci_configured == 1) && (entry.vid == 0) ) {
					int omci_vid = 0, omci_vprio = 0;
					if (get_OMCI_TR69_WAN_VLAN(&omci_vid, &omci_vprio) && omci_vid > 0) {
						sEntry[i].vid = omci_vid;
					}
				}
				else
#endif
					sEntry[i].vid = entry.vid;
				memcpy(sEntry[i].MacAddr,entry.MacAddr,MAC_ADDR_LEN);
				getServiceType(sEntry[i].serviceType,entry.applicationtype);
#ifdef CONFIG_00R0
				strcpy(sEntry[i].WanName, entry.WanName);
#endif
				break;
			}

		}
		if (getInAddr( sEntry[i].ifname, IP_ADDR, (void *)&inAddr) == 1)
		{
			temp = inet_ntoa(inAddr);
			if (getInFlags( sEntry[i].ifname, &flags) == 1)
				if ((strcmp(temp, "10.0.0.1") == 0) && flags & IFF_POINTOPOINT)	// IP Passthrough or IP unnumbered
					strcpy(sEntry[i].ipAddr, STR_UNNUMBERED);
				else if (strcmp(temp, "64.64.64.64") == 0)
					strcpy(sEntry[i].ipAddr, "");
				else{
					strcpy(sEntry[i].ipAddr, temp);
					getInAddr( sEntry[i].ifname, SUBNET_MASK, (void *)&inAddr);
					temp = inet_ntoa(inAddr);
					strcpy(sEntry[i].mask, temp);
				}
		}
		else
#endif
			strcpy(sEntry[i].ipAddr, "");

#ifdef EMBED
		if (getInAddr( sEntry[i].ifname, DST_IP_ADDR, (void *)&inAddr) == 1)
		{
			temp = inet_ntoa(inAddr);
			if (strcmp(temp, "10.0.0.2") == 0)
				strcpy(sEntry[i].remoteIp, STR_UNNUMBERED);
			else if (strcmp(temp, "64.64.64.64") == 0)
				strcpy(sEntry[i].remoteIp, "");
			else
				strcpy(sEntry[i].remoteIp, temp);

			if (flags & (IFF_UP | IFF_RUNNING)) {
				unsigned char value[32];
				snprintf(value, 32, "%s.%s", (char *)MER_GWINFO, sEntry[i].ifname);
				if (fp = fopen(value, "r")) {
					fscanf(fp, "%s\n", sEntry[i].remoteIp);
					//strcpy(sEntry[i].protocol, "mer1483");
					fclose(fp);
				}
				else{
					snprintf(value, 32, "/var/udhcpc/router.%s", sEntry[i].ifname);
					if (fp = fopen(value, "r")) {
						fscanf(fp, "%s\n", sEntry[i].remoteIp);
						fclose(fp);
					}
				}
			}

			// special case for DHCP WAN without router option
			if(strcmp(sEntry[i].remoteIp, sEntry[i].ipAddr) == 0)
				strcpy(sEntry[i].remoteIp, "");
		}
		else
#endif
			strcpy(sEntry[i].remoteIp, "");

		if (!strcmp(sEntry[i].protocol, ""))
		{
			//get channel mode
			switch(sEntry[i].cmode) {
			case CHANNEL_MODE_IPOE:
				strcpy(sEntry[i].protocol, "mer1483");
				break;
			case CHANNEL_MODE_BRIDGE:
				strcpy(sEntry[i].protocol, "br1483");
				break;
			case CHANNEL_MODE_6RD:
				strcpy(sEntry[i].protocol, "6rd");
				break;
			case CHANNEL_MODE_6IN4:
				strcpy(sEntry[i].protocol, "6in4");
				break;
			case CHANNEL_MODE_6TO4:
				strcpy(sEntry[i].protocol, "6to4");
				break;
			default:
				break;
			}
		}

		mType = MEDIA_INDEX(sEntry[i].ifIndex);
		if (mType == MEDIA_ATM)
			linkState = dslState;
		#ifdef CONFIG_PTMWAN
		else if (mType == MEDIA_PTM)
			linkState = dslState && ethState;//???
		#endif /*CONFIG_PTMWAN*/
		else if (mType == MEDIA_ETH)
			linkState = ethState;
#ifdef WLAN_WISP
		else if (mType == MEDIA_WLAN){
			char wisp_name[16];
			getWispWanName(wisp_name, ETH_INDEX(sEntry[i].ifIndex));
			linkState = get_net_link_status(wisp_name);
		}
#endif
		else
			linkState = 0;
		sEntry[i].link_state = linkState;
		// set status flag
		if (getInFlags( sEntry[i].ifname, &flags) == 1)
		{
			if (flags & IFF_UP) {
				if (!linkState) {
					sEntry[i].strStatus = (char *)IF_DOWN;
					sEntry[i].strStatus_IPv6 = (char *)IF_DOWN;
					sEntry[i].itf_state = 0;
				}
				else {
					if (sEntry[i].cmode == CHANNEL_MODE_BRIDGE) {
						sEntry[i].strStatus = (char *)IF_UP;
						sEntry[i].strStatus_IPv6 = (char *)IF_UP;
						sEntry[i].itf_state = 1;
					}
					else {
						if ((sEntry[i].ipver | IPVER_IPV4) &&
								getInAddr(sEntry[i].ifname, IP_ADDR, (void *)&inAddr) == 1) {
							temp = inet_ntoa(inAddr);
							if (strcmp(temp, "64.64.64.64")) {
								sEntry[i].strStatus = (char *)IF_UP;
								sEntry[i].itf_state = 1;															}
							else {
								sEntry[i].strStatus = (char *)IF_DOWN;
								sEntry[i].itf_state = 0;
							}
						}
						else {
							sEntry[i].strStatus = (char *)IF_DOWN;
							sEntry[i].itf_state = 0;
						}
#ifdef CONFIG_IPV6
						if (sEntry[i].ipver | IPVER_IPV6) {
							if(getifip6(sEntry[i].ifname, IPV6_ADDR_UNICAST, &ip6_addr, 1) > 0)
								sEntry[i].strStatus_IPv6 = (char *)IF_UP;
						}
#endif
					}
				}
			}
			else {
				sEntry[i].strStatus = (char *)IF_DOWN;
				sEntry[i].strStatus_IPv6 = (char *)IF_DOWN;
				sEntry[i].itf_state = 0;
			}
		}
		else {
			sEntry[i].strStatus = (char *)IF_NA;
			sEntry[i].strStatus_IPv6 = (char *)IF_NA;
			sEntry[i].itf_state = -1;
		}

		if (sEntry[i].cmode == CHANNEL_MODE_PPPOE || sEntry[i].cmode == CHANNEL_MODE_PPPOA) {
			if (sEntry[i].itf_state <= 0) {
				sEntry[i].ipAddr[0] = '\0';
				sEntry[i].remoteIp[0] = '\0';
			}
			if (entry.pppCtype == CONNECT_ON_DEMAND && entry.pppIdleTime != 0)
				sEntry[i].pppDoD = 1;
			else
				sEntry[i].pppDoD = 0;
		}

	}
	return ifcount;
}

#endif //end of CONFIG_00R0


int isValidMedia(unsigned int ifIndex)
{
	MEDIA_TYPE_T mType;

	mType = MEDIA_INDEX(ifIndex);
	if (1
	#ifdef CONFIG_DEV_xDSL
		&& mType!=MEDIA_ATM
	#endif
	#ifdef CONFIG_PTMWAN
		&& mType!=MEDIA_PTM
	#endif /*CONFIG_PTMWAN*/
	#ifdef CONFIG_ETHWAN
		&& mType!=MEDIA_ETH
	#endif
	#ifdef WLAN_WISP
		&& mType!=MEDIA_WLAN
	#endif
	)
		return 0;
	return 1;
}

// Kaohj -- specific for pvc channel
// map: bit map of used interface, ppp index (0~15) is mapped into high 16 bits,
// while vc index (0~15) is mapped into low 16 bits.
// while major vc index (8 ~ 15) is mapped into hight 8 bits of vc index.
// while minor vc index (0 ~ 7) is mapped into low 8 bits of vc index.
// return: interface index, byte1 for PPP index and byte0 for vc index.
//		0xefff(NA_PPP): PPP not available
//		0xffff(NA_VC) : vc not available
unsigned int if_find_index(int cmode, unsigned int map)
{
	int i;
	unsigned int index, vc_idx, ppp_idx;
#ifdef CONFIG_RTL_MULTI_PVC_WAN
	int j;
#endif

	// find the first available vc index (mpoa interface)
	i = 0;
#ifndef CONFIG_RTL_MULTI_PVC_WAN
	for (i=0; i<MAX_VC_NUM; i++)
	{
		if (!((map>>i) & 1))
			break;
	}
#else
	for (i=0; i<MAX_VC_NUM; i++)
	{
		if (!(((map>>8)>>i) & 1))
			break;
	}

	j=0;
	for (j=0; j<MAX_VC_VIRTUAL_NUM; j++)
	{
		if (!((map>>j) & 1))
			break;
	}
#endif

#ifndef CONFIG_RTL_MULTI_PVC_WAN
	if (i != MAX_VC_NUM)
		vc_idx = i;
	else
		return NA_VC;
#else
	// Mason Yu
	//printf("if_find_index: major=%d, minor=%d\n", i, j);
	if (i != MAX_VC_NUM && j != MAX_VC_VIRTUAL_NUM) {
		if (cmode == CHANNEL_MODE_RT1483 || cmode == CHANNEL_MODE_PPPOA || cmode == CHANNEL_MODE_RT1577)
			j = j | DUMMY_VC_MINOR_INDEX;
		vc_idx = (((i << 4) & 0xf0) | (j & 0x0f));
	}
	else
		return NA_VC;
#endif

	if (cmode == CHANNEL_MODE_PPPOE || cmode == CHANNEL_MODE_PPPOA)
	{
		// find an available PPP index
		map >>= 16;
		i = 0;
		while (map & 1)
		{
			map >>= 1;
			i++;
		}
		ppp_idx = i;
		if (ppp_idx<=(MAX_PPP_NUM-1))
			index = TO_IFINDEX(MEDIA_ATM, ppp_idx, vc_idx);
		else
			return NA_PPP;

		if (cmode == CHANNEL_MODE_PPPOA)
			index = TO_IFINDEX(MEDIA_ATM, ppp_idx, DUMMY_VC_INDEX);
	}
	else
	{
		// don't care the PPP index
		index = TO_IFINDEX(MEDIA_ATM, DUMMY_PPP_INDEX, vc_idx);
	}
	return index;
}

#ifdef CONFIG_ETHWAN
int init_ethwan_config(MIB_CE_ATM_VC_T *pEntry)
{
	if (!pEntry)
		return 0;
	memset(pEntry, 0, sizeof(MIB_CE_ATM_VC_T));
	pEntry->ifIndex = TO_IFINDEX(MEDIA_ETH, DUMMY_PPP_INDEX, 0);
	pEntry->cmode = CHANNEL_MODE_BRIDGE;
	pEntry->enable = 1;
	return 1;
}
#endif
#ifdef CONFIG_PTMWAN
int init_ptmwan_config(MIB_CE_ATM_VC_T *pEntry)
{
	if (!pEntry)
		return 0;
	memset(pEntry, 0, sizeof(MIB_CE_ATM_VC_T));
	pEntry->ifIndex = TO_IFINDEX(MEDIA_PTM, DUMMY_PPP_INDEX, 0);
	pEntry->cmode = CHANNEL_MODE_BRIDGE;
	pEntry->enable = 1;
	return 1;
}
#endif /*CONFIG_PTMWAN*/

//star: to get ppp index for wanname
static int getpppindex(MIB_CE_ATM_VC_T * pEntry)
{
	int ret = -1;
	int mibtotal, i, num = 0, totalnum = 0;
	unsigned int pppindex, tmpindex;
	MIB_CE_ATM_VC_T Entry;

	if (pEntry->cmode != CHANNEL_MODE_PPPOE && pEntry->cmode != CHANNEL_MODE_PPPOA)
		return ret;

	pppindex = PPP_INDEX(pEntry->ifIndex);
	if (pppindex == DUMMY_PPP_INDEX)
		return ret;

	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < mibtotal; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;
		if (Entry.cmode != CHANNEL_MODE_PPPOE && Entry.cmode != CHANNEL_MODE_PPPOA)
			continue;
		tmpindex = PPP_INDEX(Entry.ifIndex);
		if (tmpindex == DUMMY_PPP_INDEX)
			continue;
		if (Entry.vpi == pEntry->vpi && Entry.vci == pEntry->vci) {
			totalnum++;
			if (tmpindex < pppindex)
				num++;
		}
	}

	if (totalnum > 1)
		ret = num;

	return ret;

}

int setWanName(char *str, int applicationtype)
{
#ifdef CONFIG_E8B
	if (applicationtype&X_CT_SRV_TR069)
		strcat(str, "TR069_");
#ifdef CONFIG_USER_RTK_VOIP
	if (applicationtype&X_CT_SRV_VOICE)
		strcat(str, WAN_VOIP_VOICE_NAME_CONN);
#endif
	if (applicationtype&X_CT_SRV_INTERNET)
		strcat(str, "INTERNET_");
	if (applicationtype&X_CT_SRV_OTHER)
		strcat(str, "Other_");
#ifdef CONFIG_YUEME
	if (applicationtype&X_CT_SRV_SPECIAL_SERVICE_1)
		strcat(str, "SPECIAL_SERVICE_1_");
	if (applicationtype&X_CT_SRV_SPECIAL_SERVICE_2)
		strcat(str, "SPECIAL_SERVICE_2_");
	if (applicationtype&X_CT_SRV_SPECIAL_SERVICE_3)
		strcat(str, "SPECIAL_SERVICE_3_");
	if (applicationtype&X_CT_SRV_SPECIAL_SERVICE_4)
		strcat(str, "SPECIAL_SERVICE_4_");
#ifdef SUPPORT_CLOUD_VR_SERVICE
	if (applicationtype&X_CT_SRV_SPECIAL_SERVICE_VR)
		strcat(str, "SPECIAL_SERVICE_VR_");
#endif
#endif
	if (strlen(str)) // remove tailing '_'
		str[strlen(str)-1]='\0';
#else
#ifdef CONFIG_USER_RTK_WAN_CTYPE
	str[0] = '\0';
	if (applicationtype & X_CT_SRV_TR069)
		strcat(str, "TR069_");

	if (applicationtype & X_CT_SRV_INTERNET)
		strcat(str, "Internet_");

	if (applicationtype & X_CT_SRV_OTHER)
		strcat(str, "Other_");

	if (applicationtype & X_CT_SRV_VOICE)
		strcat(str, "Voice_");
#else
	strcpy(str, "Internet_");
#endif
#endif
	return 0;
}


int generateWanName(MIB_CE_ATM_VC_T *entry, char* wanname)
{
#ifdef CONFIG_E8B

	char vpistr[6];
	char vcistr[6];
	char vid[16];
	int i, mibtotal;
	MIB_CE_ATM_VC_T tmpEntry;
	MEDIA_TYPE_T mType;

	if(entry==NULL || wanname ==NULL)
		return -1;

	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0; i< mibtotal; i++)
	{
		mib_chain_get(MIB_ATM_VC_TBL, i, &tmpEntry);
		if(tmpEntry.ifIndex == entry->ifIndex)
			break;
	}
	if(i==mibtotal)
		return -1;
	i++;

	mType = MEDIA_INDEX(entry->ifIndex);
	sprintf(wanname, "%d_", i);

	memset(vpistr, 0, sizeof(vpistr));
	memset(vcistr, 0, sizeof(vcistr));
	if (entry->applicationtype&X_CT_SRV_TR069)
		strcat(wanname, "TR069_");
#ifdef CONFIG_USER_RTK_VOIP
	if (entry->applicationtype&X_CT_SRV_VOICE)
		strcat(wanname, WAN_VOIP_VOICE_NAME_CONN);
#endif
	if (entry->applicationtype&X_CT_SRV_INTERNET)
	{
#if defined(CONFIG_SUPPORT_HQOS_APPLICATIONTYPE)
		if(entry->othertype == OTHER_HQOS_TYPE)
			strcat(wanname, "HQoS_");
		else
#endif
		strcat(wanname, "INTERNET_");
	}
	if (entry->applicationtype&X_CT_SRV_OTHER)
	{
#ifdef CONFIG_SUPPORT_IPTV_APPLICATIONTYPE
		if(entry->othertype == OTHER_IPTV_TYPE)
			strcat(wanname, "IPTV_");
		else
#endif
#ifdef CONFIG_SUPPORT_OTT_APPLICATIONTYPE
		if(entry->othertype == OTHER_OTT_TYPE)
			strcat(wanname, "OTT_");
		else
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
			strcat(wanname, "OTHER_");
#else
			strcat(wanname, "Other_");
#endif
	}
#ifdef CONFIG_YUEME
	if (entry->applicationtype&X_CT_SRV_SPECIAL_SERVICE_1)
		strcat(wanname, "SPECIAL_SERVICE_1_");
	if (entry->applicationtype&X_CT_SRV_SPECIAL_SERVICE_2)
		strcat(wanname, "SPECIAL_SERVICE_2_");
	if (entry->applicationtype&X_CT_SRV_SPECIAL_SERVICE_3)
		strcat(wanname, "SPECIAL_SERVICE_3_");
	if (entry->applicationtype&X_CT_SRV_SPECIAL_SERVICE_4)
		strcat(wanname, "SPECIAL_SERVICE_4_");
#ifdef SUPPORT_CLOUD_VR_SERVICE
	if (entry->applicationtype&X_CT_SRV_SPECIAL_SERVICE_VR)
		strcat(wanname, "SPECIAL_SERVICE_VR_");
#endif
#endif
	if(entry->cmode == CHANNEL_MODE_BRIDGE)
		strcat(wanname, "B_");
	else
		strcat(wanname, "R_");
	if ((mType == MEDIA_ETH) || (mType == MEDIA_PTM)) {
		strcat(wanname, "VID_");
		if(entry->vlan==1){
			sprintf(vid, "%d", entry->vid);
			strcat(wanname, vid);
		}
	}
	else if (mType == MEDIA_ATM) {
		sprintf(vpistr, "%d", entry->vpi);
		sprintf(vcistr, "%d", entry->vci);
		strcat(wanname, vpistr);
		strcat(wanname, "_");
		strcat(wanname, vcistr);
	}
	return 0;
#else
	char vpistr[6];
	char vcistr[6];

	if (entry == NULL || wanname == NULL)
		return -1;
	memset(vpistr, 0, sizeof(vpistr));
	memset(vcistr, 0, sizeof(vcistr));
	setWanName(wanname, entry->applicationtype);
#ifdef CONFIG_USER_RTK_WAN_CTYPE
	wanname[0] = '\0';
	if (entry->applicationtype & X_CT_SRV_TR069)
		strcat(wanname, "TR069_");

	if (entry->applicationtype & X_CT_SRV_INTERNET)
		strcat(wanname, "Internet_");

	if (entry->applicationtype & X_CT_SRV_OTHER)
		strcat(wanname, "Other_");

	if (entry->applicationtype & X_CT_SRV_VOICE)
		strcat(wanname, "Voice_");
#else
	strcpy(wanname, "Internet_");
#endif
	if (entry->cmode == CHANNEL_MODE_BRIDGE)
		strcat(wanname, "B_");
	else
		strcat(wanname, "R_");
	sprintf(vpistr, "%d", entry->vpi);
	sprintf(vcistr, "%d", entry->vci);
	strcat(wanname, vpistr);
	strcat(wanname, "_");
	strcat(wanname, vcistr);
	//star: for multi-ppp in one pvc
	if (entry->cmode == CHANNEL_MODE_PPPOE || entry->cmode == CHANNEL_MODE_PPPOA) {
		char pppindex[6];
		int intindex;
		intindex = getpppindex(entry);
		if (intindex != -1) {
			snprintf(pppindex, 6, "%u", intindex);
			strcat(wanname, "_");
			strcat(wanname, pppindex);
		}
	}

	return 0;

#endif
}

int getWanName(MIB_CE_ATM_VC_T * pEntry, char *name)
{
	if (pEntry == NULL || name == NULL)
		return 0;
#ifdef _CWMP_MIB_
	if (*(pEntry->WanName))
		strcpy(name, pEntry->WanName);
	else
#endif
	{			//if not set by ACS. then generate automaticly.
		generateWanName(pEntry, name);
	}
	return 1;
}

#include <linux/atmdev.h>

#define DEFDATALEN	56
#define PINGCOUNT 3
#define PINGINTERVAL	1	/* second */
#define MAXWAIT		5

static struct sockaddr_in pingaddr;
static int pingsock = -1;
static long ntransmitted = 0, nreceived = 0, nrepeats = 0;
static int myid = 0;
static int finished = 0;

int create_icmp_socket(void)
{
	struct protoent *proto;
	int sock;

	proto = getprotobyname("icmp");
	/* if getprotobyname failed, just silently force
	 * proto->p_proto to have the correct value for "icmp" */
	if ((sock = socket(AF_INET, SOCK_RAW,
			(proto ? proto->p_proto : 1))) < 0) {        /* 1 == ICMP */
		printf("cannot create raw socket\n");
	}

	return sock;
}

int in_cksum(unsigned short *buf, int sz)
{
	int nleft = sz;
	int sum = 0;
	unsigned short *w = buf;
	unsigned short ans = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		*(unsigned char *) (&ans) = *(unsigned char *) w;
		sum += ans;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	ans = ~sum;
	return (ans);
}

static void pingfinal()
{
	finished = 1;
}

static void sendping()
{
	struct icmp *pkt;
	int c;
	char packet[DEFDATALEN + 8];

	pkt = (struct icmp *) packet;
	pkt->icmp_type = ICMP_ECHO;
	pkt->icmp_code = 0;
	pkt->icmp_cksum = 0;
	pkt->icmp_seq = ntransmitted++;
	pkt->icmp_id = myid;
	pkt->icmp_cksum = in_cksum((unsigned short *) pkt, sizeof(packet));

	c = sendto(pingsock, packet, sizeof(packet), 0,
			   (struct sockaddr *) &pingaddr, sizeof(struct sockaddr_in));

	if (c < 0 || c != sizeof(packet)) {
		ntransmitted--;
		finished = 1;
		printf("sock: sendto fail !");
		return;
	}

	signal(SIGALRM, sendping);
	if (ntransmitted < PINGCOUNT) {	/* schedule next in 1s */
		alarm(PINGINTERVAL);
	} else {	/* done, wait for the last ping to come back */
		signal(SIGALRM, pingfinal);
		alarm(MAXWAIT);
	}
}

int utilping(char *str)
{
//	char *submitUrl;
	char tmpBuf[100];
	int c;
	struct hostent *h;
	struct icmp *pkt;
	struct iphdr *iphdr;
	char packet[DEFDATALEN + 8];
	int rcvdseq, ret=0;
	fd_set rset;
	struct timeval tv;

	if (str[0]) {
		if ((pingsock = create_icmp_socket()) < 0) {
			perror("socket");
			snprintf(tmpBuf, 100, "ping: socket create error");
			goto setErr_ping;
		}

		memset(&pingaddr, 0, sizeof(struct sockaddr_in));
		pingaddr.sin_family = AF_INET;

		if ((h = gethostbyname(str)) == NULL) {
			//herror("ping: ");
			//snprintf(tmpBuf, 100, "ping: %s: %s", str, hstrerror(h_errno));
			goto setErr_ping;
		}

		if (h->h_addrtype != AF_INET) {
			//strcpy(tmpBuf, "unknown address type; only AF_INET is currently supported.");
			goto setErr_ping;
		}

		memcpy(&pingaddr.sin_addr, h->h_addr, sizeof(pingaddr.sin_addr));

		printf("PING %s (%s): %d data bytes\n",
		   h->h_name,inet_ntoa(*(struct in_addr *) &pingaddr.sin_addr.s_addr),DEFDATALEN);

		myid = getpid() & 0xFFFF;
		ntransmitted = nreceived = nrepeats = 0;
		finished = 0;
		rcvdseq=ntransmitted-1;
		FD_ZERO(&rset);
		FD_SET(pingsock, &rset);
		/* start the ping's going ... */
		sendping();

		/* listen for replies */
		while (1) {
			struct sockaddr_in from;
			socklen_t fromlen = (socklen_t) sizeof(from);
			int c, hlen, dupflag;

			if (finished)
				break;

			tv.tv_sec = 1;
			tv.tv_usec = 0;

			if (select(pingsock+1, &rset, NULL, NULL, &tv) > 0) {
				if ((c = recvfrom(pingsock, packet, sizeof(packet), 0,
								  (struct sockaddr *) &from, &fromlen)) < 0) {

					printf("sock: recvfrom fail !");
					continue;
				}
			}
			else // timeout or error
				continue;

			if (c < DEFDATALEN+ICMP_MINLEN)
				continue;

			iphdr = (struct iphdr *) packet;
			hlen = iphdr->ihl << 2;
			pkt = (struct icmp *) (packet + hlen);	/* skip ip hdr */
			if (pkt->icmp_id != myid) {
//				printf("not myid\n");
				continue;
			}
			if (pkt->icmp_type == ICMP_ECHOREPLY) {
				++nreceived;
				if (pkt->icmp_seq == rcvdseq) {
					// duplicate
					++nrepeats;
					--nreceived;
					dupflag = 1;
				} else {
					rcvdseq = pkt->icmp_seq;
					dupflag = 0;
					if (nreceived < PINGCOUNT)
					// reply received, send another immediately
						sendping();
				}
				printf("%d bytes from %s: icmp_seq=%u", c,
					   inet_ntoa(*(struct in_addr *) &from.sin_addr.s_addr),
					   pkt->icmp_seq);
				if (dupflag) {
					printf(" (DUP!)");
				}
				printf("\n");
			}
			if (nreceived >= PINGCOUNT) {
				ret = 1;
				break;
			}
		}
		FD_CLR(pingsock, &rset);
		close(pingsock);
		pingsock = -1;
	}
	printf("\n--- ping statistics ---\n");
	printf("%ld packets transmitted, ", ntransmitted);
	printf("%ld packets received\n\n", nreceived);
	if (nrepeats)
		printf("%ld duplicates, ", nrepeats);
	printf("\n");
	return ret;
setErr_ping:
	if (pingsock >= 0) {
		close(pingsock);
		pingsock = -1;
	}
	printf("Ping error!!\n\n");
	return ret;
}

struct ServerSelectionDiagSt {
	char host[128];
	struct sockaddr_in pingaddr;
	unsigned int hostValid;
	unsigned int xmitStamp;
	unsigned int replyRecvd;
	unsigned int xmitSeq;
	unsigned int xmitId;
	unsigned int responseTime;
	/* below for average response time calc */
	unsigned int replySuccCnt;
	unsigned int totalResponseTime;
};

long getCurrTimeinMS()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (tv.tv_sec*1000 + tv.tv_usec/1000);
}

static void ServerSelectionSendPing(struct ServerSelectionDiagSt *pHostAddr, int hostNum)
{
	//struct sysinfo info;
	struct icmp *pkt;
	int ret, idx;
	char packet[DEFDATALEN + 8];

	//sysinfo(&info);
	for (idx=0; idx<hostNum; idx++)
	{
		if (!pHostAddr[idx].hostValid)
			continue;

		pkt = (struct icmp *) packet;
		pkt->icmp_type = ICMP_ECHO;
		pkt->icmp_code = 0;
		pkt->icmp_cksum = 0;
		pkt->icmp_seq = pHostAddr[idx].xmitSeq++;
		pkt->icmp_id = pHostAddr[idx].xmitId;
		pkt->icmp_cksum = in_cksum((unsigned short *) pkt, sizeof(packet));

		//fprintf(stderr, "ping request to host %s.\n", pHostAddr[idx].host);
		ret = sendto(pingsock, packet, sizeof(packet), 0,
				   (struct sockaddr *) &pHostAddr[idx].pingaddr, sizeof(struct sockaddr_in));

		pHostAddr[idx].xmitStamp = getCurrTimeinMS();
		pHostAddr[idx].replyRecvd = 0;
		if (ret < 0 || ret != sizeof(packet)) {
			pHostAddr[idx].xmitSeq--;
		}
	}
}

static void ServerSelectionRecv(struct ServerSelectionDiagSt *pHostAddr, int hostNum, char *packet)
{
	//struct sysinfo info;
	struct iphdr *iphdr;
	struct icmp *pkt;
	int idx, responseTime;
	int hlen;

	//sysinfo(&info);
	iphdr = (struct iphdr *) packet;
	hlen = iphdr->ihl << 2;
	pkt = (struct icmp *) (packet + hlen);	/* skip ip hdr */
	//fprintf(stderr, "icmp from %s, seq %#x id %#x\n", inet_ntoa(*(struct in_addr *)&iphdr->saddr), pkt->icmp_seq, pkt->icmp_id);

	for (idx=0; idx<hostNum; idx++)
	{
		if (!pHostAddr[idx].hostValid)
			continue;

		//fprintf(stderr, "host %s  addr %s  seq %#x id %#x\n", pHostAddr[idx].host, inet_ntoa(pHostAddr[idx].pingaddr.sin_addr), (pHostAddr[idx].xmitSeq-1),
		//		pHostAddr[idx].xmitId);
		if (iphdr->saddr != pHostAddr[idx].pingaddr.sin_addr.s_addr)
			continue;

		if (pkt->icmp_id != pHostAddr[idx].xmitId)
			continue;

		if (pkt->icmp_type == ICMP_ECHOREPLY) {
			if (pkt->icmp_seq == (pHostAddr[idx].xmitSeq-1)) {
				responseTime = getCurrTimeinMS() - pHostAddr[idx].xmitStamp;
				pHostAddr[idx].responseTime = (pHostAddr[idx].responseTime > responseTime)?responseTime:pHostAddr[idx].responseTime;
				pHostAddr[idx].replyRecvd = 1;
				pHostAddr[idx].replySuccCnt++;
				pHostAddr[idx].totalResponseTime += pHostAddr[idx].responseTime;
				fprintf(stderr, "%s ping reply from %s %d ms\n", __func__, inet_ntoa(pHostAddr[idx].pingaddr.sin_addr), pHostAddr[idx].responseTime);

				break;
			}
		}
	}
}

static int ServerSelectionCheckPingFin(struct ServerSelectionDiagSt *pHostAddr, int hostNum, int timeout)
{
	//struct sysinfo info;
	int idx;
	int done = 1;

	//sysinfo(&info);
	for (idx=0; idx<hostNum; idx++)
	{
		if (!pHostAddr[idx].hostValid)
			continue;

		if (!pHostAddr[idx].replyRecvd)
		{
			if ((getCurrTimeinMS()-pHostAddr[idx].xmitStamp) < timeout)
			{
				done = 0;
				break;
			}
		}
	}

	return done;
}

/*
 * INPUT Parameters:
 *  - pHostList: hostlist for ping test, splitted by comma, up to 10 hosts
 *  - pInterface: interface assigned for ping
 *  - numRepetition: ping test times for each host
 *  - timeout: ping timeout period
 * OUTPUT Parameters:
 *  - pFastestHost:
 *  - AverageResponseTime
 */
int ServerSelectionDiagnostics(char *pHostList, char *pInterface, int numRepetition, int timeout, char *pFastestHost, int *AverageResponseTime)
{
	struct ServerSelectionDiagSt diagHost[10];
	char hostList[256+1];
	char *pHost;
	int hostNum=0, idx;
	int nXmitted=0;
	struct hostent *h;
	char packet[DEFDATALEN + 8];
	fd_set rset;
	struct timeval tv;
	unsigned int responseTime=0;
	int fastestHostIdx=-1;

	if (!pHostList)
		return -1;

	/* 1. initialize diagHost */
	memset(diagHost, 0, sizeof(diagHost));

	snprintf(hostList, sizeof(hostList), "%s", pHostList);
	pHost = hostList;
	pHost = strtok(hostList, ",");
	while (pHost != NULL)
	{
		snprintf(diagHost[hostNum].host, sizeof(diagHost[hostNum].host), "%s", pHost);
		hostNum++;
		pHost = strtok(NULL, ",");

		if (hostNum >= 10)
			break;
	}

	for (idx=0; idx<hostNum; idx++)
	{
		diagHost[idx].pingaddr.sin_family = AF_INET;
		if ((h = gethostbyname(diagHost[idx].host)) == NULL) {
			fprintf(stderr, "%s gethostname %s fail\n", __func__, diagHost[idx].host);
			continue;
		}

		if (h->h_addrtype != AF_INET) {
			fprintf(stderr, "%s host %s addrtype is not INET\n", __func__, diagHost[idx].host);
			continue;
		}

		memcpy(&diagHost[idx].pingaddr.sin_addr, h->h_addr, sizeof(diagHost[idx].pingaddr.sin_addr));
		diagHost[idx].xmitSeq = 0;
		diagHost[idx].xmitId = getpid() & 0xFFFF;
		diagHost[idx].hostValid = 1;
		diagHost[idx].responseTime = 0xFFFFFFFF;
	}

	/* 1. initialize socket */
	if ((pingsock = create_icmp_socket()) < 0) {
		perror("ServerSelectionDiagnostics socket");
		return -1;
	}

	if (fcntl(pingsock, F_SETFL, O_NONBLOCK) == -1) {
		fprintf(stderr, "%s Could not set noblock\n", __func__);
		return -1;
	}

	if (pInterface)
	{
		struct ifreq ifr = {0};
		strcpy(ifr.ifr_name, pInterface);

		if (setsockopt(pingsock, SOL_SOCKET, SO_BINDTODEVICE, (char *)(&ifr), sizeof(ifr)))
			printf("[%s %d] bind ping socket to %s fail\n", __func__, __LINE__, pInterface);
	}

	/* 3. start the ping's going ... */
	ServerSelectionSendPing(diagHost, hostNum);
	nXmitted++;

	/* listen for replies */
	while (1) {
		struct sockaddr_in from;
		socklen_t fromlen = (socklen_t) sizeof(from);
		int c, hlen, dupflag;

		FD_ZERO(&rset);
		FD_SET(pingsock, &rset);
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		if (ServerSelectionCheckPingFin(diagHost, hostNum, timeout))
		{
			if (nXmitted >= numRepetition)
			{
				/* final test result is ready */
				break;
			}

			ServerSelectionSendPing(diagHost, hostNum);
			nXmitted++;
		}

		if (select(pingsock+1, &rset, NULL, NULL, &tv) <= 0)
			continue;

		while (1)
		{
			if ((c = recvfrom(pingsock, packet, sizeof(packet), 0,
						(struct sockaddr *) &from, &fromlen)) < 0)
			{
				//perror("ServerSelectionDiagnostics recvfrom!");
				break;
			}

			if (c < DEFDATALEN+ICMP_MINLEN)
				break;

			ServerSelectionRecv(diagHost, hostNum, packet);
		}
	}

	FD_CLR(pingsock, &rset);
	close(pingsock);
	pingsock = -1;

	responseTime = 0xFFFFFFFF;
	for (idx=0; idx<hostNum; idx++)
	{
		if (!diagHost[idx].hostValid)
		{
			fprintf(stderr, "host %s unreachable\n", diagHost[idx].host);
			continue;
		}

		if (!diagHost[idx].replySuccCnt)
		{
			fprintf(stderr, "host %s no reply received\n", diagHost[idx].host);
			continue;
		}

		fprintf(stderr, "host %s success %d fail %d  average response time %d ms\n", diagHost[idx].host, diagHost[idx].replySuccCnt,
				(numRepetition-diagHost[idx].replySuccCnt), diagHost[idx].totalResponseTime/diagHost[idx].replySuccCnt);
		if (responseTime > diagHost[idx].responseTime)
		{
			responseTime = diagHost[idx].responseTime;
			fastestHostIdx = idx;
		}
	}

	if (fastestHostIdx != -1)
	{
		strcpy(pFastestHost, diagHost[fastestHostIdx].host);
		*AverageResponseTime = diagHost[fastestHostIdx].totalResponseTime/diagHost[fastestHostIdx].replySuccCnt;
	}

	return 0;
}

int get_wan_gateway(unsigned int ifIndex, struct in_addr *gateway)
{
    int i=0;
    unsigned int totalEntry;
    char str_ipaddr[20] = {0};
    char str_netmask[20] = {0};
    char str_gateway[20] = {0};
    char ifname[IFNAMSIZ];

    MIB_CE_ATM_VC_T Entry;

    totalEntry = mib_chain_total(MIB_ATM_VC_TBL);

    for (i=0; i<totalEntry; i++) { // check the gw of static route exist on other wan ?
            if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
                    return -1;

            if (Entry.ifIndex == ifIndex) {
                    ifGetName(Entry.ifIndex,ifname,sizeof(ifname));
                    getIPaddrInfo(&Entry, str_ipaddr, str_netmask, str_gateway);
                    if(strcmp(str_gateway, "")==0){
                            return -1;
                    }
                    else{
                            inet_aton(str_gateway, gateway);
                            //printf("get_wan_gateway gateway %s\n", str_gateway);
                            return 0;
                    }
            }
    }

    return -1;
}


int defaultGWAddr(char *gwaddr)
{
	char buff[256], ifname[16];
	int flgs;
	//unsigned long int g;
	//struct in_addr gw;
	struct in_addr gw, dest, mask;
	FILE *fp;

	if (!(fp=fopen("/proc/net/route", "r"))) {
		printf("Error: cannot open /proc/net/route - continuing...\n");
		return -1;
	}

	fgets(buff, sizeof(buff), fp);
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		//if (sscanf(buff, "%s%lx%*lx%X%", ifname, &g, &flgs) != 3) {
		if (sscanf(buff, "%s%x%x%x%*d%*d%*d%x", ifname, &dest, &gw, &flgs, &mask) != 5) {
			printf("Unsupported kernel route format\n");
			fclose(fp);
			return -1;
		}
		if(flgs & RTF_UP) {
			// default gateway
			if (dest.s_addr == 0 && mask.s_addr == 0) {
				if (gw.s_addr != 0) {
					sprintf(gwaddr, "%s", inet_ntoa(gw));
					fclose(fp);
					return 0;
				}
				else {
					if (getInAddr(ifname, DST_IP_ADDR, (void *)&gw) == 1)
						if (gw.s_addr != 0) {
							sprintf(gwaddr, "%s", inet_ntoa(gw));
							fclose(fp);
							return 0;
						}
				}
			}
		}
		/*if((g == 0) && (flgs & RTF_UP)) {
			if (getInAddr(ifname, DST_IP_ADDR, (void *)&gw) == 1)
				if (gw.s_addr != 0) {
					sprintf(gwaddr, "%s", inet_ntoa(gw));
					fclose(fp);
					return 0;
				}
		}
		if (sscanf(buff, "%*s%*lx%lx%X%", &g, &flgs) != 2) {
			printf("Unsupported kernel route format\n");
			fclose(fp);
			return -1;
		}
		if(flgs & RTF_UP) {
			gw.s_addr = g;
			if (gw.s_addr != 0) {
				sprintf(gwaddr, "%s", inet_ntoa(gw));
				fclose(fp);
				return 0;
			}
		}*/
	}
	fclose(fp);
	return -1;
}

int pdnsAddr(char *dnsaddr)
{

	FILE *fp;
	char buff[256];
	if ( (fp = fopen("/var/resolv.conf", "r")) == NULL ) {
		printf("Unable to open resolver file\n");
		return -1;
	}

	fgets(buff, sizeof(buff), fp);
	if (sscanf(buff, "nameserver %s", dnsaddr) != 1) {
		printf("Unsupported kernel route format\n");
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 0;
}

int getNameServers(char *buf)
{
	FILE *fp;
	char line[128], addr[64];
	int count = 0;

	buf[0] = '\0';
	if ((fp = fopen(RESOLV, "r")) == NULL)
		return -1;

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (sscanf(line, "nameserver %s", addr) != 1)
			continue;

		if (count == 0) {
			sprintf(buf, "%s", addr);
		} else {
			sprintf(line, ", %s", addr);
			strcat(buf, line);
		}
		count++;
	}

	fclose(fp);

	return 0;
}

int setNameServers(char *buf)
{
	FILE *fp;
	char line[128], *ptr;

	if ((fp = fopen(RESOLV, "w")) == NULL)
		return -1;

	ptr = strtok(buf, ", ");

	do {
		if (snprintf(line, sizeof(line), "nameserver %s\n", ptr) == 0)
			continue;
		fputs(line, fp);
	} while (ptr = strtok(NULL, ", "));

	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(RESOLV, 0666);
#endif
	return 0;
}

#ifdef ACCOUNT_CONFIG
//Jenny, get user account privilege
int getAccPriv(char *user)
{
	int totalEntry, i;
	MIB_CE_ACCOUNT_CONFIG_T Entry;
	char suName[MAX_NAME_LEN], usName[MAX_NAME_LEN];

	mib_get_s(MIB_SUSER_NAME, (void *)suName, sizeof(suName));
	mib_get_s(MIB_USER_NAME, (void *)usName, sizeof(usName));
	if (strcmp(suName, user) == 0)
		return (int)PRIV_ROOT;
	else if (strcmp(usName, user) == 0)
		return (int)PRIV_USER;
	totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL);
	for (i=0; i<totalEntry; i++)
	{
		if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&Entry))
			continue;
		if (strcmp(Entry.userName, user) == 0)
			return Entry.privilege;
	}
	return -1;
}
#endif

//jim support dsl disconnection when firmware upgrade from Local Side.....
//retrun value: 1-local        0 - wan         -1 - error
static int isAccessFromLocal(unsigned int ip)
{
	unsigned int uLanIp;
	unsigned int uLanMask;
	char secondIpEn;
	unsigned int uSecondIp;
	unsigned int uSecondMask;

	if (!mib_get_s( MIB_ADSL_LAN_IP, (void *)&uLanIp, sizeof(uLanIp) ))
		return -1;
	if (!mib_get_s( MIB_ADSL_LAN_SUBNET, (void *)&uLanMask, sizeof(uLanMask) ))
		return -1;

	if ( (ip & uLanMask) == (uLanIp & uLanMask) ) {//in the same subnet with LAN port
		return 1;
	} else {
		if (!mib_get_s( MIB_ADSL_LAN_ENABLE_IP2, (void *)&secondIpEn, sizeof(secondIpEn) ))
			return -1;

		if (secondIpEn == 1) {//second IP is enabled
			if (!mib_get_s( MIB_ADSL_LAN_IP2, (void *)&uSecondIp, sizeof(uSecondIp)))
				return -1;
			if (!mib_get_s( MIB_ADSL_LAN_SUBNET2, (void *)&uSecondMask, sizeof(uSecondMask)))
				return -1;

			if ( (ip & uSecondMask) == (uSecondIp & uSecondMask) )//in the same subnet with LAN port
				return 1;
		}
	}

	return 0;
}

#ifdef ROUTING
// Jenny, for checking duplicated destination address
int checkRoute(MIB_CE_IP_ROUTE_T rtEntry, int idx)
{
	//unsigned char destNet[4];
	uint32_t destID, netMask, nextHop;
	unsigned int totalEntry = mib_chain_total(MIB_IP_ROUTE_TBL); /* get chain record size */
	MIB_CE_IP_ROUTE_T Entry;
	int i;

	/*destNet[0] = rtEntry.destID[0] & rtEntry.netMask[0];
	destNet[1] = rtEntry.destID[1] & rtEntry.netMask[1];
	destNet[2] = rtEntry.destID[2] & rtEntry.netMask[2];
	destNet[3] = rtEntry.destID[3] & rtEntry.netMask[3];*/
	destID = *((uint32_t *)&rtEntry.destID);
	netMask = *((uint32_t *)&rtEntry.netMask);
	nextHop = *((uint32_t *)&rtEntry.nextHop);

	// check if route exists
	for (i=0; i<totalEntry; i++) {
		long pdestID, pnetMask, pnextHop;
		unsigned char pdID[4];
		char *temp;
		if (i == idx)
			continue;
		if (!mib_chain_get(MIB_IP_ROUTE_TBL, i, (void *)&Entry))
			return 0;

		pdID[0] = Entry.destID[0] & Entry.netMask[0];
		pdID[1] = Entry.destID[1] & Entry.netMask[1];
		pdID[2] = Entry.destID[2] & Entry.netMask[2];
		pdID[3] = Entry.destID[3] & Entry.netMask[3];
		temp = inet_ntoa(*((struct in_addr *)pdID));
		pdestID = ntohl(inet_addr(temp));
		pnetMask = *((unsigned long *)&Entry.netMask);
		pnextHop = *((unsigned long *)&Entry.nextHop);
		if (pdestID == destID && pnetMask == netMask && pnextHop == nextHop && rtEntry.ifIndex == Entry.ifIndex)
			return 0;
	}
	return 1;
}
#endif

int isValidIpAddr(char *ipAddr)
{
	long field[4];

	if (sscanf(ipAddr, "%ld.%ld.%ld.%ld", &field[0], &field[1], &field[2], &field[3]) != 4)
		return 0;

	if (field[0] < 1 || field[0] > 223 || field[0] == 127 || field[1] < 0 || field[1] > 255 || field[2] < 0 || field[2] > 255 || field[3] < 0 || field[3] > 254)
		return 0;

	if (inet_addr(ipAddr) == -1)
		return 0;

	return 1;
}

int isValidHostID(char *ip, char *mask)
{
	long hostIp, netMask, hid, mbit;
	int i, bit, bitcount = 0;

	inet_aton(mask, (struct in_addr *)&netMask);
	hostIp = ntohl(inet_addr(ip));
	netMask = ntohl(netMask);

	hid = ~netMask & hostIp;
	if (hid == 0x0)
		return 0;
	mbit = 0;
	while (1) {
		if (netMask & 0x80000000) {
			mbit++;
			netMask <<= 1;
		}
		else
			break;
	}
	mbit = 32 - mbit;
	for (i=0; i<mbit; i++) {
		bit = hid & 1L;
		if (bit)
			bitcount ++;
		hid >>= 1;
	}
	if (bitcount == mbit)
		return 0;
	return 1;
}

int isValidNetmask(char *mask, int checkbyte)
{
	long netMask;
	int i, bit, isChanged = 0;

	netMask = ntohl(inet_addr(mask));

	// Check most byte (must be 255) and least significant bit (must be 0)
	if (checkbyte) {
		bit = (netMask & 0xFF000000L) >> 24;
		if (bit != 255)
			return 0;
	}

//	bit = netMask & 1L;
//	if (bit)
//		return 0;

	// make sure the bit pattern changes from 0 to 1 only once
	for (i=1; i<31; i++) {
		netMask >>= 1;
		bit = netMask & 1L;

		if (bit) {
			if (!isChanged)
				isChanged = 1;
		}
		else {
			if (isChanged)
				return 0;
		}
	}

	return 1;
}

// check whether an IP address is in the same subnet
int isSameSubnet(char *ipAddr1, char *ipAddr2, char *mask)
{
	long netAddr1, netAddr2, netMask;

	netAddr1 = inet_addr(ipAddr1);
	netAddr2 = inet_addr(ipAddr2);
	netMask = inet_addr(mask);

	if ((netAddr1 & netMask) != (netAddr2 & netMask))
		return 0;

	return 1;
}

int isValidMacString(char *MacStr)
{
	int i, maclen=0;

	if(!MacStr)
		return 0;

	/* We expact two mac format
	 *  AABBCCDDEEFF
	 *	AA:BB:CC:DD:EE:FF
	 */
	maclen = strlen(MacStr);
	if(maclen != 12 && maclen != 17){
		return 0;
	}

	for(i=0;i<maclen;i++){
		if(maclen == 17){
			// for AA:BB:CC:DD:EE:FF case
			if((i+1)%3 == 0){
				if(MacStr[i] != ':')
					return 0;
			}else{
				if(!((MacStr[i] >= '0' && MacStr[i] <= '9')
					|| (MacStr[i] >= 'a' && MacStr[i] <= 'f')
					|| (MacStr[i] >= 'A' && MacStr[i] <= 'F'))){
					return 0;
				}
			}
		}else{
			// for AABBCCDDEEFF
			if(!((MacStr[i] >= '0' && MacStr[i] <= '9')
				|| (MacStr[i] >= 'a' && MacStr[i] <= 'f')
				|| (MacStr[i] >= 'A' && MacStr[i] <= 'F'))){
				return 0;
			}
		}
	}
	return 1;
}

int isValidMacAddr(unsigned char *macAddr)
{
	// Check for bad, multicast, broadcast, or null address
	if ((macAddr[0] & 1) || (macAddr[0] & macAddr[1] & macAddr[2] & macAddr[3] & macAddr[4] & macAddr[5]) == 0xff
		|| (macAddr[0] | macAddr[1] | macAddr[2] | macAddr[3] | macAddr[4] | macAddr[5]) == 0x00)
		return 0;

	return 1;
}

//Ethernet
#if defined(ELAN_LINK_MODE)
#include <linux/sockios.h>
struct mii_ioctl_data {
	unsigned short	phy_id;
	unsigned short	reg_num;
	unsigned short	val_in;
	unsigned short	val_out;
};
#endif

int restart_ethernet(int instnum)
{
#if defined(CONFIG_LUNA) && defined(CONFIG_RTL_MULTI_LAN_DEV)
	MIB_CE_SW_PORT_T Entry;
	int port = 0;
#if defined(CONFIG_COMMON_RT_API)
	rt_port_phy_ability_t ability;
#else
	rtk_port_phy_ability_t ability;
#endif

	int flags = 0;
	int uni_capability = -1;
	static unsigned char link[2][3];

	memset(link, 0, sizeof(link));

	if (instnum > 0 && instnum <= ELANVIF_NUM)
	{
		if (!mib_chain_get(MIB_SW_PORT_TBL, (instnum - 1), &Entry))
			return -1;

		port = rtk_port_get_lan_phyID((instnum - 1));

		if (port == -1)
			return -1;

#if 1 /* update invalid default config */
		if (Entry.enable == 0 && Entry.duplex == 0 && Entry.speed == 0)
		{
			Entry.enable = 1;
			Entry.duplex = DUPLEX_TYPE_AUTO;
			Entry.speed = SPEED_AUTO;
			mib_chain_update(MIB_SW_PORT_TBL, &Entry, (instnum - 1));
			Commit();
		}
#endif

		if (Entry.enable)
		{
			if (getInFlags(SW_LAN_PORT_IF[instnum - 1], &flags) == 1)
			{
				if(!(flags & IFF_UP))
				{
					fprintf(stderr, "[%s@%d] %s is down \n", __FUNCTION__, __LINE__, SW_LAN_PORT_IF[instnum - 1]);
					va_cmd(IFCONFIG, 2, 1, SW_LAN_PORT_IF[instnum - 1], "up");
#if defined(CONFIG_RTL9607C_SERIES)
					set_port_powerdown_state(instnum - 1, 0);
#endif
				}
				else
				{
					fprintf(stderr, "[%s@%d] %s is up \n", __FUNCTION__, __LINE__, SW_LAN_PORT_IF[instnum - 1]);
				}
			}
			else
			{
				va_cmd(IFCONFIG, 2, 1, SW_LAN_PORT_IF[instnum - 1], "up");
#if defined(CONFIG_RTL9607C_SERIES)
				set_port_powerdown_state(instnum - 1, 0);
#endif
			}
#ifdef CONFIG_UNI_CAPABILITY
			uni_capability = getUniPortCapability((instnum - 1));

			if (uni_capability >= 0 && uni_capability != UNI_PORT_NONE)
			{
				if (Entry.duplex != DUPLEX_TYPE_AUTO && Entry.speed != SPEED_AUTO)
				{
					//printf("\033[1;33;40m @@@@@@@@@@@@@@@@@########### %s:%d	!DUPLEX_AUTO !SPEED_AUTO 033[m\n", __FUNCTION__, __LINE__);
					link[Entry.duplex][Entry.speed] = 1;
					if (uni_capability == UNI_PORT_FE && Entry.speed == SPEED_1000M)
					{
						//speed chage to auto
						link[Entry.duplex][SPEED_10M] = 1;
						link[Entry.duplex][SPEED_100M] = 1;
					}
					if (Entry.duplex == DUPLEX_TYPE_HALF && Entry.speed == SPEED_1000M)
					{
						//speed = 1000 && duplex=half, port can not initial, we set duplex as full
						link[DUPLEX_TYPE_FULL][Entry.speed] = 1;
						Entry.duplex = DUPLEX_TYPE_FULL;
						mib_chain_update(MIB_SW_PORT_TBL, &Entry, (instnum - 1));
						Commit();
					}
				}

				if (Entry.duplex == DUPLEX_TYPE_AUTO && Entry.speed != SPEED_AUTO)
				{
					//printf("\033[1;33;40m @@@@@@@@@@@@@@@@@########### %s:%d	DUPLEX_AUTO !SPEED_AUTO \033[m\n\n", __FUNCTION__, __LINE__);
					link[DUPLEX_TYPE_HALF][Entry.speed] = 1;
					link[DUPLEX_TYPE_FULL][Entry.speed] = 1;
					if (uni_capability == UNI_PORT_FE && Entry.speed == SPEED_1000M)
					{
						//speed chage to auto
						link[DUPLEX_TYPE_HALF][SPEED_10M] = 1;
						link[DUPLEX_TYPE_FULL][SPEED_10M] = 1;
						link[DUPLEX_TYPE_HALF][SPEED_100M] = 1;
						link[DUPLEX_TYPE_FULL][SPEED_100M] = 1;
					}
				}

				if (Entry.duplex != DUPLEX_TYPE_AUTO && Entry.speed == SPEED_AUTO)
				{
					//printf("\033[1;33;40m @@@@@@@@@@@@@@@@@########### %s:%d	!DUPLEX_AUTO SPEED_AUTO \033[m\n", __FUNCTION__, __LINE__);
					link[Entry.duplex][SPEED_10M] = 1;
					link[Entry.duplex][SPEED_100M] = 1;
					if (uni_capability == UNI_PORT_GE)
					{
						link[Entry.duplex][SPEED_1000M] = 1;
						if (Entry.duplex == DUPLEX_TYPE_HALF)
						{
							//speed = 1000 && duplex=half, port can not initial, we set duplex as full
							link[DUPLEX_TYPE_FULL][SPEED_1000M] = 1;
							Entry.duplex = DUPLEX_TYPE_FULL;
							mib_chain_update(MIB_SW_PORT_TBL, &Entry, instnum - 1);
							Commit();
						}
					}
				}

				if (Entry.duplex == DUPLEX_TYPE_AUTO && Entry.speed == SPEED_AUTO)
				{
					//printf("\033[1;33;40m @@@@@@@@@@@@@@@@@########### %s:%d	DUPLEX_AUTO SPEED_AUTO \033[m\n", __FUNCTION__, __LINE__);
					link[DUPLEX_TYPE_HALF][SPEED_10M] = 1;
					link[DUPLEX_TYPE_FULL][SPEED_10M] = 1;
					link[DUPLEX_TYPE_HALF][SPEED_100M] = 1;
					link[DUPLEX_TYPE_FULL][SPEED_100M] = 1;
					if (uni_capability == UNI_PORT_GE)
					{
						link[DUPLEX_TYPE_HALF][SPEED_1000M] = 1;
						link[DUPLEX_TYPE_FULL][SPEED_1000M] = 1;
					}
				}
#if 0
				int i=0, j=0;
				for(i=0;i<2;i++){
					for(j=0; j < 3;j++){
						printf("link[%d][%d]=%d\n", i, j, link[i][j]);
					}
				}
#endif

#if defined(CONFIG_COMMON_RT_API)
				rt_port_phyAutoNegoAbility_get(port, &ability);
#else
				rtk_port_phyAutoNegoAbility_get(port, &ability);
#endif
				ability.Half_10 = link[DUPLEX_TYPE_HALF][SPEED_10M];
				ability.Half_100 = link[DUPLEX_TYPE_HALF][SPEED_100M];
				ability.Half_1000 = link[DUPLEX_TYPE_HALF][SPEED_100M];
				ability.Full_10 = link[DUPLEX_TYPE_FULL][SPEED_10M];
				ability.Full_100 = link[DUPLEX_TYPE_FULL][SPEED_100M];
				ability.Full_1000 = link[DUPLEX_TYPE_FULL][SPEED_1000M];

				if(Entry.flowcontrol) {
					ability.FC = 1;
					ability.AsyFC = 1;
				}
				else {
					ability.FC = 0;
					ability.AsyFC = 0;
				}

#if 0
				int i = 0, j = 0;
				for(i=0 ; i < 2; i++){
					for(j=0; j < 3; j++){
						printf("\033[1;33;40m @@@@@@@@@@@@@@@@@########### %s:%d link[%d][%d] = %d \033[m\n", __FUNCTION__, __LINE__, i , j, link[i][j]);
					}
				}
				printf("\033[1;33;40m @@@@@@@@@@@@@@@@@########### %s:%d  port %d duplex %d speed %d ability.Half_10 %d, ability.Half_100 %d, ability.Half_1000 %d, ability.Full_10 %d, ability.Full_100 %d, ability.Full_1000 %d, ability.FC %d, ability.AsyFC %d \033[m\n", __FUNCTION__, __LINE__,
					port,
					Entry.duplex,
					Entry.speed,
					ability.Half_10,
					ability.Half_100,
					ability.Half_1000,
					ability.Full_10,
					ability.Full_100,
					ability.Full_1000,
					ability.FC,
					ability.AsyFC);
#endif
#if defined(CONFIG_COMMON_RT_API)
				rt_port_phyAutoNegoAbility_set(port, &ability);
#else
				rtk_port_phyAutoNegoAbility_set(port, &ability);
#endif
			}
#endif
			 rtk_port_gigaLiteEnable_set(port, Entry.gigalite);
		}
		else
		{
			fprintf(stderr, "[%s@%d] LAN %d disabled \n", __FUNCTION__, __LINE__, instnum);
			va_cmd(IFCONFIG, 2, 1, SW_LAN_PORT_IF[instnum - 1], "down");
#if defined(CONFIG_RTL9607C_SERIES)
			set_port_powerdown_state(instnum - 1, 1);
#endif
		}
	}

	return 0;
#else
	char vChar=0;
	int status=0;

#ifdef _CWMP_MIB_
	//eth0 interface enable or disable
	mib_get_s(CWMP_LAN_ETHIFENABLE, (void *)&vChar, sizeof(vChar));

	if(vChar==0)
	{
		va_cmd(IFCONFIG, 2, 1, ELANIF, "down");
		printf("Disable eth0 interface\n");
		return 0;
	}
	else
	{
#endif
		va_cmd(IFCONFIG, 2, 1, ELANIF, "up");
		printf("Enable eth0 interface\n");

#ifdef ELAN_LINK_MODE_INTRENAL_PHY
		int skfd;
		struct ifreq ifr;
		unsigned char mode;
		//struct mii_ioctl_data *mii = (struct mii_data *)&ifr.ifr_data;
		//MIB_CE_SW_PORT_T Port;
		//int i, k, total;
		struct ethtool_cmd ecmd;


		strcpy(ifr.ifr_name, ELANIF);
		ifr.ifr_data = &ecmd;


		if (!mib_get_s(MIB_ETH_MODE, (void *)&mode, sizeof(mode)))
			return -1;

		skfd = socket(AF_INET, SOCK_DGRAM, 0);
		if(skfd == -1)
		{
			fprintf(stderr, "Socket Open failed Error\n");
			return -1;
		}
		ecmd.cmd = ETHTOOL_GSET;
		if (ioctl(skfd, SIOCETHTOOL, &ifr) < 0) {
			fprintf(stderr, "ETHTOOL_GSET on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
			status=-1;
			goto error;
		}

		ecmd.autoneg = AUTONEG_DISABLE;
		switch(mode) {
		case LINK_10HALF:
			ecmd.speed = SPEED_10;
			ecmd.duplex = DUPLEX_HALF;
			break;
		case LINK_10FULL:
			ecmd.speed = SPEED_10;
			ecmd.duplex = DUPLEX_FULL;
			break;
		case LINK_100HALF:
			ecmd.speed = SPEED_100;
			ecmd.duplex = DUPLEX_HALF;
			break;
		case LINK_100FULL:
			ecmd.speed = SPEED_100;
			ecmd.duplex = DUPLEX_FULL;
			break;
		default:
			ecmd.autoneg = AUTONEG_ENABLE;
		}

		ecmd.cmd = ETHTOOL_SSET;
		if (ioctl(skfd, SIOCETHTOOL, &ifr) < 0) {
			fprintf(stderr, "ETHTOOL_SSET on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
			status=-1;
			goto error;
		}
		/*
		ecmd.cmd = ETHTOOL_NWAY_RST;
		if (ioctl(skfd, SIOCETHTOOL, &ifr) < 0) {
			fprintf(stderr, "ETHTOOL_SSET on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
			status=-1;
		}
		*/

		error:

		close(skfd);
#endif
		return status;
#ifdef _CWMP_MIB_
	}
#endif

	return -1;
#endif
}

#ifdef ELAN_LINK_MODE
// return value:
// 0  : successful
// -1 : failed
int setupLinkMode(void)
{
	int skfd;
	struct ifreq ifr;
	struct mii_ioctl_data *mii = (struct mii_ioctl_data *)&ifr.ifr_data;
	MIB_CE_SW_PORT_T Port;
	int i;
	int status=0;

	strcpy(ifr.ifr_name, ELANIF);
	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	for (i=0; i<SW_LAN_PORT_NUM; i++) {
		if (!mib_chain_get(MIB_SW_PORT_TBL, i, (void *)&Port))
			continue;
		mii->phy_id = i; // phy i
		mii->reg_num = 4; // register 4
		// set NWAY advertisement
		mii->val_in = 0x0401; // enable flow control capability and IEEE802.3
		if (Port.linkMode == LINK_10HALF || Port.linkMode == LINK_AUTO)
			mii->val_in |= (1<<(5+LINK_10HALF));
		if (Port.linkMode == LINK_10FULL || Port.linkMode == LINK_AUTO)
			mii->val_in |= (1<<(5+LINK_10FULL));
		if (Port.linkMode == LINK_100HALF || Port.linkMode == LINK_AUTO)
			mii->val_in |= (1<<(5+LINK_100HALF));
		if (Port.linkMode == LINK_100FULL || Port.linkMode == LINK_AUTO)
			mii->val_in |= (1<<(5+LINK_100FULL));

		if (ioctl(skfd, SIOCSMIIREG, &ifr) < 0) {
			fprintf(stderr, "SIOCSMIIREG on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
			status=-1;
		}

		// restart
		mii->reg_num = 0; // register 0
		mii->val_in = 0x1200; // enable auto-negotiation and restart it
		if (ioctl(skfd, SIOCSMIIREG, &ifr) < 0) {
			fprintf(stderr, "SIOCSMIIREG on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
			status=-1;
		};
	}
	if(skfd!=-1) close(skfd);
	return status;
}
#endif // of ELAN_LINK_MODE

#ifdef WEB_REDIRECT_BY_MAC
struct callout_s landingPage_ch;
void clearLandingPageRule(void *dummy)
{
	int status=0;
	char ipaddr[16], ip_port[32], ip_port2[32];
	char tmpbuf[MAX_URL_LEN];
	int  def_port=WEB_REDIR_BY_MAC_PORT;
	int  def_port2=WEB_REDIR_BY_MAC_PORT_SSL;
	unsigned int uLTime;

	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", "WebRedirectByMAC");

	ipaddr[0]='\0'; ip_port[0]='\0';

	if (mib_get_s(MIB_ADSL_LAN_IP, (void *)tmpbuf, sizeof(tmpbuf)) != 0)
	{
		strncpy(ipaddr, inet_ntoa(*((struct in_addr *)tmpbuf)), 16);
		ipaddr[15] = '\0';
		sprintf(ip_port,"%s:%d",ipaddr,def_port);
		sprintf(ip_port2,"%s:%d",ipaddr,def_port2);
	}

	//iptables -t nat -A WebRedirectByMAC -d 192.168.1.1 -j RETURN
	status|=va_cmd(IPTABLES, 8, 1, "-t", "nat",(char *)FW_ADD,"WebRedirectByMAC",
		"-d", ipaddr, "-j", (char *)FW_RETURN);

	//iptables -t nat -A WebRedirectByMAC -p tcp --dport 80 -j DNAT --to-destination 192.168.1.1:18080
	status|=va_cmd(IPTABLES, 12, 1, "-t", "nat",(char *)FW_ADD,"WebRedirectByMAC",
		"-p", "tcp", "--dport", "80", "-j", "DNAT",
		"--to-destination", ip_port);

	//iptables -t nat -A WebRedirectByMAC -p tcp --dport 443 -j DNAT --to-destination 192.168.1.1:10443
	status|=va_cmd(IPTABLES, 12, 1, "-t", "nat","-A","WebRedirectByMAC",
		"-p", "tcp", "--dport", "443", "-j", "DNAT",
		"--to-destination", ip_port2);
	mib_chain_clear(MIB_WEB_REDIR_BY_MAC_TBL);

	//update to the flash
	#if 0
	itfcfg("sar", 0);
	itfcfg(ELANIF, 0);
	#endif
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	#if 0
	itfcfg("sar", 1);
	itfcfg(ELANIF, 1);
	#endif

        mib_get_s(MIB_WEB_REDIR_BY_MAC_INTERVAL, (void *)&uLTime, sizeof(uLTime));
	TIMEOUT_F(clearLandingPageRule, 0, uLTime, landingPage_ch);
}
#endif

#ifdef CONFIG_NET_IPGRE
struct callout_s gre_ch;
static unsigned long rx_record[MAX_GRE_NUM] = {0, 0};
static unsigned char endpoint_idx[MAX_GRE_NUM] = {1, 1};

void poll_GRE(void *dummy)
{
	struct net_device_stats nds;
	MIB_GRE_T Entry;
	int i, EntryNum;
	char ifname[IFNAMSIZ];

	EntryNum = mib_chain_total(MIB_GRE_TBL);
	for(i=0;i<EntryNum; i++) {
		if(!mib_chain_get(MIB_GRE_TBL, i, (void*)&Entry)||!Entry.enable)
			continue;

		strncpy(ifname, (char *)GREIF[i], sizeof(ifname));
		get_net_device_stats(ifname, &nds);
		//printf("%s rx=%u, rx_record[0]=%u, endpoint_idx[0]=%d\n", ifname, nds.rx_packets, rx_record[i], endpoint_idx[i]);
		if (nds.rx_packets == rx_record[i]) {
			//printf("%s rx=%u, rx_record[0]=%u,  create %d endpoint\n", ifname, nds.rx_packets, rx_record[i], Entry.nextEndpointIdx);
			gre_take_effect(Entry.nextEndpointIdx);
		}
		rx_record[i] = nds.rx_packets;
	}
	TIMEOUT_F(poll_GRE, 0, GRE_BACKUP_INTERVAL, gre_ch);
}
#endif
#if defined(CONFIG_CMCC) && defined(CONFIG_SUPPORT_ADAPT_DPI_V3_3_0)
struct callout_s wanSE_ch;
const char WANSE_file[] = "/tmp/wan_symbol_error";
const char WANSE_file_lock[] = "/tmp/wan_symbol_error.lock";
unsigned long wan_rx_error=0;
void calWanSymbolErrors(){
	rt_stat_port_cntr_t counters;
	unsigned long rx_errs;
	FILE *fp;
	int lock_fd;
	double rx_error_rate;
	uint32 ifInAllPkts;
	lock_fd = lock_file_by_flock(WANSE_FILE_LOCK, 1);
	fp = fopen(WANSE_FILE,"w");
	if(fp){
		rt_stat_port_getAll(rtk_port_get_wan_phyID(), &counters);
		ifInAllPkts = counters.ifInUcastPkts+counters.ifInMulticastPkts+counters.ifInBroadcastPkts;
		if(ifInAllPkts != 0){
			rx_errs = counters.dot3StatsSymbolErrors + counters.dot3ControlInUnknownOpcodes;
			if(rx_errs >= wan_rx_error){
				rx_error_rate = ((double)(rx_errs-wan_rx_error)/(double)ifInAllPkts)*100;
				//printf("calWanSymbolErrors port_rx_errors=%d,port_rx_errors_rate =%.4f\n",rx_errs,((rx_errs-wan_rx_error)/(counters.ifInUcastPkts+counters.ifInMulticastPkts+counters.ifInBroadcastPkts))*100);
			}else{
				rx_error_rate = ((double)rx_errs/(double)ifInAllPkts)*100;
			}
			fprintf(fp, "%d %.4f%% \n", rx_errs, rx_error_rate);
			wan_rx_error = rx_errs;
		}else{
			fprintf(fp, "0 0.0000%% \n");
			wan_rx_error = 0;
		}
		fclose(fp);
	}
	unlock_file_by_flock(lock_fd);

	TIMEOUT_F(calWanSymbolErrors, 0, 60, wanSE_ch);
	return;
}
#endif

#ifdef DMZ
#ifdef AUTO_DETECT_DMZ
#define AUTO_DMZ_INTERVAL 30
static int getDhcpClientIP(char **ppStart, unsigned long *size, char *ip)
{
	struct dhcpOfferedAddrLease entry;
	if (*size < sizeof(entry))
		return -1;
	entry = *((struct dhcpOfferedAddrLease *)*ppStart);
	*ppStart = *ppStart + sizeof(entry);
	*size = *size - sizeof(entry);
	if (entry.expires == 0)
		return 0;
	if (entry.chaddr[0]==0&&entry.chaddr[1]==0&&entry.chaddr[2]==0&&entry.chaddr[3]==0&&entry.chaddr[4]==0&&entry.chaddr[5]==0)
		return 0;
	strcpy(ip, inet_ntoa(*((struct in_addr *)&entry.yiaddr)));
	return 1;
}

static int Get1stArp(char *dmzIP)
{
	FILE *fp;
	char  buf[256];
	char tmp0[32],tmp1[32],tmp2[32];
	int dmzFlags;
	dmzIP[0] = 0; // "" empty
	fp = fopen("/proc/net/arp", "r");
	if (fp == NULL)
		printf("read arp file fail!\n");
	else {
		fgets(buf, 256, fp);//first line!?
		while(fgets(buf, 256, fp) >0) {
			//sscanf(buf, "%s", dmzIP);
			sscanf(buf,"%s	%*s	0x%x %s %s %s ", dmzIP, &dmzFlags,tmp0,tmp1,tmp2);
			if ((dmzFlags == 0) || (strncmp(tmp2,"br",2)!=0))
				continue;
			fclose(fp);
			return 1;
		}
		fclose(fp);
		return 0;
	}
	return 0;
}

struct callout_s autoDMZ_ch;

void poll_autoDMZ(void *dummy)
{
	static char autoDMZ = 0;
	// signal dhcp client to renew
	char dhcpIP[40], *buffer = NULL, *ptr;
	FILE *fp;
	char dmz_ip_str[20];
	int buffer_size;


	if (autoDMZ == 0) // search 1st arp
	{
		unsigned long ulDmz, ulDhcp;
		unsigned char ucPreStat;
		int ret;
		char ip[32];
		struct in_addr ipAddr, dmzIp;

		fflush(stdout);
		if (Get1stArp(dmz_ip_str) == 1)
		{
			if (strlen(dmz_ip_str) == 0)
				goto end; // error

			autoDMZ = 1;
			mib_get_s(MIB_DMZ_ENABLE, (void *)&ucPreStat, sizeof(ucPreStat));
			mib_get_s(MIB_DMZ_IP, (void *)&dmzIp, sizeof(dmzIp));
			ulDmz = *((unsigned long *)&dmzIp);
			strncpy(ip, inet_ntoa(dmzIp), 16); // ip -> old dmz
			if (strcmp(ip,dmz_ip_str) == 0)
				goto end; // no changed!

			inet_aton(dmz_ip_str, &ipAddr);
			ip[15] = '\0';
			if (ucPreStat && ulDmz != 0)
				clearDMZ();

			if (mib_set(MIB_DMZ_IP, (void *)&ipAddr) == 0)
				printf("Set DMZ MIB error!\n");

				if (!mib_set(MIB_DMZ_ENABLE, (void *)&autoDMZ))
					printf("Set DMZ Capability error!\n");
				setDMZ(dmz_ip_str);
				goto end;

		}

	}
	else  // check dhcp and then arp
	{
		unsigned long ulDmz, ulDhcp;
		unsigned char ucPreStat;
		struct in_addr ipAddr, dmzIp;
		int ret;
		char ip[32];
		// siganl DHCP server to update lease file

		if(getDhcpClientLeasesDB(&buffer, &buffer_size) <= 0)
			goto end;

		ptr = buffer;

		while (1) {
			if (getDhcpClientIP(&ptr, &buffer_size, dhcpIP) == 1)
			{
				mib_get_s(MIB_DMZ_ENABLE, (void *)&ucPreStat, sizeof(ucPreStat));
				mib_get_s(MIB_DMZ_IP, (void *)&dmzIp, sizeof(dmzIp));
				ulDmz = *((unsigned long *)&dmzIp);
				strncpy(ip, inet_ntoa(dmzIp), 16);
				if (strcmp(ip, dhcpIP) ==0 )	//dhcp still in using..
				{
					goto end;
				}
			}
			else // find the 1st arp
			{
				// uses the 1st entry
				if (Get1stArp(dmz_ip_str) == 1)
				{
					if (strlen(dmz_ip_str) == 0)
						goto end; // error

					autoDMZ = 1;
					mib_get_s(MIB_DMZ_ENABLE, (void *)&ucPreStat, sizeof(ucPreStat));
					mib_get_s(MIB_DMZ_IP, (void *)&dmzIp, sizeof(dmzIp));
					ulDmz = *((unsigned long *)&dmzIp);
					strncpy(ip, inet_ntoa(dmzIp), 16); // ip -> old dmz
					inet_aton(dmz_ip_str, &ipAddr);
					ip[15] = '\0';
					if (strcmp(ip,dmz_ip_str) == 0)
						goto end; // still the same one

					if (ucPreStat && ulDmz != 0)
						clearDMZ();

					if (mib_set(MIB_DMZ_IP, (void *)&ipAddr) == 0)
						printf("Set DMZ MIB error!\n");

						if (!mib_set(MIB_DMZ_ENABLE, (void *)&autoDMZ))
							printf("Set DMZ Capability error!\n");
						setDMZ(dmz_ip_str);
						goto end;

				}
				else
				{
					// clear rules
					mib_get_s(MIB_DMZ_IP, (void *)&dmzIp, sizeof(dmzIp));
					strncpy(ip, inet_ntoa(dmzIp), 16); // ip -> old dmz
					ip[15] = '\0';

					clearDMZ();
					*((unsigned long *)&dmzIp) = 0;
					autoDMZ = 0;
					mib_set(MIB_DMZ_ENABLE, (void *)&autoDMZ);
					mib_set(MIB_DMZ_IP, (void *)&dmzIp);
				}
				goto end;
			}
		}
	}


end:
	if (buffer)
		free(buffer);
	TIMEOUT_F(poll_autoDMZ, 0, AUTO_DMZ_INTERVAL, autoDMZ_ch);
}
#endif
#endif

#if defined(NEED_CHECK_DHCP_SERVER)||defined(NEED_CEHCK_WLAN_INTERFACE)
#if defined(WLAN_SUPPORT)
int needModifyWlanInterface(void)
{
	int status=0;
	MIB_CE_MBSSIB_T Entry;
	int needUpdate = 0;
	int i;

	for(i=0; i<=NUM_VWLAN_INTERFACE;++i)
	{
		needUpdate = 0;
		if(!mib_chain_get(MIB_MBSSIB_TBL, i, (void *)&Entry))
			continue;

		if(Entry.wlanDisabled==0)
		{
			Entry.wlanDisabled=1;
			needUpdate = 1;
		}

		if(needUpdate)
		{
			mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, i);
			status|=1;
		}
	}

	return status;
}
#endif

int isOnlyOneBridgeWan(void)
{
	int vcTotal;
	MIB_CE_ATM_VC_T Entry;

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	if(vcTotal==1)
	{
		if (mib_chain_get(MIB_ATM_VC_TBL, 0, (void *)&Entry))
		{
			if(Entry.cmode== CHANNEL_MODE_BRIDGE)
			{
				return 1;
			}
		}
	}
	return 0;
}
#ifdef CONFIG_GPON_FEATURE
int isHybridGponmode(void)
{
	char deviceType;
	mib_get_s(MIB_DEVICE_TYPE,  &deviceType, sizeof(deviceType));
	if(deviceType == 2)
	{
		return 1;
	}

	return 0;
}
#endif
#ifdef CONFIG_USER_DHCP_SERVER
int needModifyDhcpMode(void)
{
	DHCP_TYPE_T dtmode;
	unsigned char vmode;

	mib_get_s(MIB_DHCP_MODE, (void *)&vmode, sizeof(vmode)) ;
	dtmode = vmode;

	if(DHCP_LAN_SERVER== dtmode)
	{
		vmode = DHCP_LAN_NONE;
		mib_set(MIB_DHCP_MODE, (void *)&vmode);
		return 1;
	}
	return 0;
}
#endif
int checkSpecifiedFunc(void)
{
	int i;
	int orig_wlan_idx;
#ifdef	NEED_CEHCK_WLAN_INTERFACE
	int need_restart_wlan=0;
#endif
#ifdef	NEED_CHECK_DHCP_SERVER
	int need_restart_dhcp=0;
#endif

#if defined( WLAN_SUPPORT)&&defined(NEED_CEHCK_WLAN_INTERFACE)
	//check wlan
	orig_wlan_idx = wlan_idx;
	for(i = 0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		if( needModifyWlanInterface()>0)
		{
			need_restart_wlan=1;
		}
	}
		wlan_idx = orig_wlan_idx;
#endif
#if defined(CONFIG_USER_DHCP_SERVER)&&defined(NEED_CHECK_DHCP_SERVER)
	//check dhcp server
	if(needModifyDhcpMode()>0)
	{
		need_restart_dhcp=1;
	}
#endif

#ifdef NEED_CEHCK_WLAN_INTERFACE
		if(need_restart_wlan)
		{
#ifdef WLAN_SUPPORT
			config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
#endif
		}
#endif
#ifdef NEED_CHECK_DHCP_SERVER
		if(need_restart_dhcp){
#ifdef CONFIG_USER_DHCP_SERVER
			restart_dhcp();
#endif
#endif
		}
		return 1;
	}
#endif

int pppdbg_get(int unit)
{
	char buff[256];
	FILE *fp;
	int pppinf, pppdbg = 0;

	if (fp = fopen(PPP_DEBUG_LOG, "r")) {
		while (fgets(buff, sizeof(buff), fp) != NULL) {
			if (sscanf(buff, "%d:%d", &pppinf, &pppdbg) != 2)
				break;
			else {
				if (pppinf == unit)
					break;
			}
		}
		fclose(fp);
	}
	return pppdbg;
}

#ifdef CONFIG_ATM_CLIP
void sendAtmInARPRep(unsigned char update)
{
	int i, totalEntry;
	MIB_CE_ATM_VC_T Entry;
	struct data_to_pass_st msg;
	char wanif[16];
	unsigned long addr;

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	if (update) {	// down --> up
		for (i=0; i<totalEntry; i++) {
			mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry);
			if (Entry.enable == 0)
				continue;
			if (Entry.cmode == CHANNEL_MODE_RT1577) {
				addr = *((unsigned long *)Entry.remoteIpAddr);
				ifGetName(Entry.ifIndex, wanif, sizeof(wanif));
				snprintf(msg.data, BUF_SIZE, "mpoactl set %s inarprep %lu", wanif, addr);
				write_to_mpoad(&msg);
			}
		}
	}
}
#endif

// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
void Commit()
{
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
}
#endif // of #if COMMIT_IMMEDIATELY

#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_AWIFI_SUPPORT)
const char RTK_AWIFI_WIFIDOG_PID[]="/var/run/wifidog.pid";
const char RTK_AWIFI_MAC_CONF[]="/var/awifi_mac.conf";
const char RTK_AWIFI_URL_CONF[]="/var/awifi_url.conf";

static int rtk_wan_aWiFi_trustedMacConf_setup()
{
	FILE *fp;
	int index, cnt;
	char macAddr[20] = {0};
	MIB_CE_RTK_AWIFI_MAC_T macEntry;

	if ((fp = fopen(RTK_AWIFI_MAC_CONF, "w")) == NULL) {
		printf("Open file %s failed !\n", RTK_AWIFI_MAC_CONF);
		return 0;
	}

	cnt = mib_chain_total(MIB_RTK_AWIFI_MAC_TBL);
	if(cnt > 0)
	{
		fprintf(fp, "TrustedMACList ");
		for(index = 0; index < cnt; index++)
		{
			bzero(macAddr, sizeof(macAddr));
			bzero(&macEntry, sizeof(macEntry));

			if (!mib_chain_get(MIB_RTK_AWIFI_MAC_TBL, index, (void *)&macEntry)){
	  			printf( "[%s:%d] Get chain record error!\n",__FUNCTION__,__LINE__);
				fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
				chmod(RTK_AWIFI_MAC_CONF, 0666);
#endif
				return 0;
			}

			snprintf(macAddr, sizeof(macAddr), "%02x:%02x:%02x:%02x:%02x:%02x",
				macEntry.macAddr[0], macEntry.macAddr[1], macEntry.macAddr[2],
				macEntry.macAddr[3], macEntry.macAddr[4], macEntry.macAddr[5]);

			fprintf(fp, "%s%s",index>0 ? "," : " ", macAddr);
		}
	}

	//printf("[%s:%d]\n",__func__,__LINE__);

	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(RTK_AWIFI_MAC_CONF, 0666);
#endif
	return 1;
}

static int rtk_wan_aWiFi_trustedUrlConf_setup()
{
	FILE *fp;
	int index, cnt;
	MIB_CE_RTK_AWIFI_URL_T urlEntry;

	if ((fp = fopen(RTK_AWIFI_URL_CONF, "w")) == NULL) {
		printf("Open file %s failed !\n", RTK_AWIFI_URL_CONF);
		return 0;
	}

	cnt = mib_chain_total(MIB_RTK_AWIFI_URL_TBL);
	if(cnt > 0)
	{
		fprintf(fp, "FirewallRuleSet global {    \n");
		for(index = 0; index < cnt; index++)
		{
			bzero(&urlEntry, sizeof(urlEntry));

			if (!mib_chain_get(MIB_RTK_AWIFI_URL_TBL, index, (void *)&urlEntry)){
				printf( "[%s:%d] Get chain record error!\n",__FUNCTION__,__LINE__);
				fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
				chmod(RTK_AWIFI_URL_CONF, 0666);
#endif
				return 0;
			}

			fprintf(fp, "    FirewallRule allow tcp to %s\n",urlEntry.url);
		}
		fprintf(fp, "}");
	}

	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(RTK_AWIFI_URL_CONF, 0666);
#endif
	return 1;
}


void rtk_wan_aWiFi_wifidog_start(char *wanIf, char *wlanIf)
{
	char str1[3] = {0};
	char str2[5] = {0};

	if(!wanIf) return;

	rtk_wan_aWiFi_trustedMacConf_setup();
	rtk_wan_aWiFi_trustedUrlConf_setup();

	#ifdef CONFIG_RTK_SKB_MARK2
	snprintf(str1, sizeof(str1), "%d", SOCK_MARK2_RTK_WIFIDOG_START);
	snprintf(str2, sizeof(str2), "%lld", SOCK_MARK2_RTK_WIFIDOG_BIT_MASK);

	va_niced_cmd("/bin/wifidog", 8, 0, "-l", wlanIf, "-b", str1, "-m", str2, "-d", "0");
	#else
	printf("[%s:%d] please enable CONFIG_RTK_SKB_MARK2 first!\n",__FUNCTION__,__LINE__);
	#endif

}

int rtk_wan_aWiFi_wifidog_stop()
{
	pid_t pid;
	char buf[32];
	pid = read_pid(RTK_AWIFI_WIFIDOG_PID);
	if (pid > 0) {/* wifidog is running */
		snprintf(buf, sizeof(buf), "kill -15 %d", pid);
		system(buf);

		unlink(RTK_AWIFI_WIFIDOG_PID);
		unlink(RTK_AWIFI_MAC_CONF);
		unlink(RTK_AWIFI_URL_CONF);
		return 1;
	}
	return 0;
}

int rtk_wan_aWiFi_start(int ifIndex)
{
	char awifiEnabled, awifi_wlan0_enable, awifi_wlan1_enable;
	int awifiItf;
	char ext_ifname[IFNAMSIZ];
	char wlan_iface[IFNAMSIZ*2]={0};
	char *wlan0 = "wlan0:";
	char *wlan1 = "wlan1:";

	if(!mib_get_s(MIB_RTK_AWIFI_ENABLE, (void *)&awifiEnabled, sizeof(awifiEnabled)) || !awifiEnabled)
		return -1;

	if(!mib_get_s(MIB_RTK_AWIFI_EXT_ITF, (void *)&awifiItf, sizeof(awifiItf)))
		return -1;

	if(!mib_get_s(MIB_RTK_AWIFI_WLAN0_ENABLE, (void *)&awifi_wlan0_enable, sizeof(awifi_wlan0_enable)) )
		return -1;

	if(!mib_get_s(MIB_RTK_AWIFI_WLAN1_ENABLE, (void *)&awifi_wlan1_enable, sizeof(awifi_wlan1_enable)) )
		return -1;

	if( (ifIndex >= 0) && (ifIndex != awifiItf)) /*wanConnection*/
		return -1;
	if(!awifi_wlan0_enable && !awifi_wlan1_enable) // no select wlan interface
		return -1;

	ifGetName(awifiItf, ext_ifname, sizeof(ext_ifname));
	if(awifi_wlan0_enable)
		strncpy(wlan_iface+strlen(wlan_iface), wlan0, strlen(wlan0));
	if(awifi_wlan1_enable)
		strncpy(wlan_iface+strlen(wlan_iface), wlan1, strlen(wlan1));
	//printf("[%s:%d] awifiEnabled=%d, awifiItf=%d, ext_ifname=%s, wlan_itf=%s\n",__FUNCTION__,__LINE__,awifiEnabled, awifiItf, ext_ifname,wlan_iface);

	rtk_wan_aWiFi_wifidog_start(ext_ifname, wlan_iface);

	return 0;
}

int rtk_wan_aWiFi_stop(int ifIndex)
{
	int awifiItf;
	char awifiEnable, buf[32];

	if(!mib_get_s(MIB_RTK_AWIFI_ENABLE, (void *)&awifiEnable, sizeof(awifiEnable)) || !awifiEnable)
		return -1;

	if(!mib_get_s(MIB_RTK_AWIFI_EXT_ITF, (void *)&awifiItf, sizeof(awifiItf)))
		return -1;

	if( (ifIndex >= 0) && (ifIndex != awifiItf)) /*wanConnection*/
		return -1;

	return rtk_wan_aWiFi_wifidog_stop();
}
#endif // CONFIG_USER_AWIFI_SUPPORT

#ifdef CONFIG_USER_CUPS
int getPrinterList(char *str, size_t size)
{
#if defined(CONFIG_USER_CUPS_1_4_6)
	char strbuf[BUF_SIZE], serverIP[INET_ADDRSTRLEN], lpName[BUF_SIZE];
	FILE *fp;
	int offset;
	struct in_addr inAddr;
	int pid=0;

	if (getInAddr((char *)LANIF, IP_ADDR, &inAddr) == 1) {
		strncpy(serverIP, inet_ntoa(inAddr), sizeof(serverIP));
	} else {
		getMIB2Str(MIB_ADSL_LAN_IP, serverIP);
	}

	offset = 0;

	pid = findPidByName("lpstat");
	if(pid > 0){
		kill(pid, SIGKILL);
		printf("Send %d signal to lpstat\n", SIGKILL);
	}

	sprintf(strbuf, "lpstat -a > /tmp/printer.tmp");
	va_cmd("/bin/sh", 2, 0, "-c", strbuf);
	if (!(fp = fopen("/tmp/printer.tmp", "r"))) {
		return 0;
	}

	while (fgets(strbuf, sizeof(strbuf), fp)) {
		/* If no printer online or no printer added, result should be:
		 * lpstat: No destinations added.
		 * lpstat: Connection refused
		 */
		if (!strncmp(strbuf, "lpstat", 6))
			break;

		/* escape blank line */
		if ((strbuf[0] == 0x0d) || (strbuf[0] == 0x0a))
			continue;

		/* lp0 accepting requests since ... */
		sscanf(strbuf, "%s %*s", lpName);

		offset += snprintf(str + offset, size - offset,
					"http://%s:631/printers/%s\n",
					serverIP, lpName);
	}

	fclose(fp);
	unlink("/tmp/printer.tmp");

	return offset;
#else
	char strbuf[BUF_SIZE], serverIP[INET_ADDRSTRLEN], *substr, *chr;
	FILE *fp;
	int offset;
	struct in_addr inAddr;

	if (getInAddr((char *)LANIF, IP_ADDR, &inAddr) == 1) {
		strncpy(serverIP, inet_ntoa(inAddr), sizeof(serverIP));
	} else {
		getMIB2Str(MIB_ADSL_LAN_IP, serverIP);
	}

	offset = 0;
	fp = fopen(CUPSDPRINTERCONF, "r");

	while (fgets(strbuf, sizeof(strbuf), fp)) {
		chr = strchr(strbuf, '#');

		/* search the pattern '<DefaultPrinter lp0>' or '<Printer lp0>' */
		if (substr = strstr(strbuf, "Printer ")) {
			if (chr && chr < substr) {
				/* in comment */
				continue;
			}

			/*
			 * the length of "Printer " is 8,
			 * now substr points to the start of the printer name
			 */
			substr += 8;

			if (chr = strchr(substr, '>'))
				*chr = '\0';

			offset += snprintf(str + offset, size - offset,
					"http://%s:631/printers/%s\n",
					serverIP, substr);
		}
	}

	fclose(fp);

	return offset;
#endif
}
#endif // CONFIG_USER_CUPS

#ifdef CONFIG_USER_MINIDLNA
// success: return 1;
// fail: return 0;
int get_dlna_db_dir(char *db_dir)
{
	FILE *fp;
	char buf[256]="";
	int ret=0;

	fp = fopen("/proc/mounts", "r");
	if (fp) {
		while (fgets(buf, sizeof(buf), fp)) {
			if (strstr(buf, "/dev/sd")) {
				sscanf(buf,"%*s %s", db_dir);
				strcat(db_dir, "/minidlna");
				//warn("get_db_dir: db_dir=%s\n", db_dir);
				// Format is : /dev/sdb /var/mnt/sdb vfat .....................
				ret = 1;
				break;
			}
		}
		fclose(fp);
	}

	return ret;
}

static void createMiniDLNAconf(char *name, char *directory)
{
	FILE *fp;

	fp = fopen(name, "w");
	if(fp) {
		fputs("port=8200\n",fp);
		fputs("network_interface=br0\n", fp);
		fprintf(fp, "media_dir=/mnt\n");
		fprintf(fp, "db_dir=%s\n",directory);
		fputs("friendly_name=My DLNA Server\n", fp);
		fputs("album_art_names=Cover.jpg/cover.jpg/AlbumArtSmall.jpg/albumartsmall.jpg/AlbumArt.jpg/albumart.jpg/Album.jpg/album.jpg/Folder.jpg/folder.jpg/Thumb.jpg/thumb.jpg\n", fp);
		fputs("inotify=yes\n", fp);
		fputs("enable_tivo=no\n", fp);
		fputs("strict_dlna=no\n", fp);
		fputs("notify_interval=10\n", fp);	// 900
		fputs("serial=12345678\n", fp);
		fputs("model_number=1\n", fp);
		fputs("log_level=general,tivo,ssdp,http,inotify,artwork,database,scanner,metadata=off\n", fp);
		fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
		chmod(name, 0666);
#endif

	}
	return;
}

void startMiniDLNA(void)
{
	char *argv[10];
	int i = 0, pid = 0;
	//MIB_DMS_T entry,*p;
	unsigned int enable;
	char db_dir[65]={0};

	if(read_pid((char *)MINIDLNAPID)>0){
		printf( "%s: already start, line=%d\n", __FUNCTION__, __LINE__ );
		return;
	}
#if 0
	if (!get_dlna_db_dir(db_dir))
		return;;
#endif
	// Mason Yu. use table not chain
	mib_get_s(MIB_DMS_ENABLE, (void *)&enable, sizeof(enable));
	if(!enable) {
		printf( "%s: is diaabled, line=%d\n", __FUNCTION__, __LINE__ );
		return;
	}
#if 1
	createMiniDLNAconf("/var/minidlna.conf", "/var/run/minidlna");
#else
	createMiniDLNAconf("/var/minidlna.conf", db_dir);
#endif
	argv[i++]="/bin/minidlna";
//	argv[i++]="-d";
	argv[i++]="-R";
	argv[i++]="-f";
	argv[i++]="/var/minidlna.conf";
	argv[i]=NULL;
	do_nice_cmd( argv[0], argv, 0 );

	while(read_pid((char*)MINIDLNAPID) < 0)
	{
		//printf("MINIDLNA is not running. Please wait!\n");
		usleep(300000);
	}
}

void stopMiniDLNA_force(void)
{
	int i, pid = 0;
	pid = read_pid((char *)MINIDLNAPID);
	if(pid <= 0){
		printf( "%s: already stop, line=%d\n", __FUNCTION__, __LINE__ );
		return;
	}

	kill(pid, SIGKILL);
	rmdir("/var/run/minidlna");
}

void stopMiniDLNA(void)
{
	int i, pid = 0;
	pid = read_pid((char *)MINIDLNAPID);
	if(pid <= 0){
		printf( "%s: already stop, line=%d\n", __FUNCTION__, __LINE__ );
		return;
	}

	//killpg(pid, 9);
	kill(pid, SIGTERM);
	rmdir("/var/run/minidlna");
}

void stopMiniDLNA_wait()
{
	int count = 0;

	stopMiniDLNA();
	while(read_pid((char*)MINIDLNAPID) > 0 && (count < 20))
	{
		usleep(100000);
		count++;
	}

	if (count>=20){
		stopMiniDLNA_force();
		unlink(MINIDLNAPID);
	}
}

void restartMiniDLNA()
{
	stopMiniDLNA_wait();
	startMiniDLNA();
}
#endif

#ifdef CONFIG_USER_DOT1AG_UTILS
//Remvoe entries that WAN interface is not existed anymore.
void arrange_dot1ag_table()
{
	int entryNum = mib_chain_total(MIB_DOT1AG_TBL);
	MIB_CE_DOT1AG_T Entry = {0};
	MIB_CE_ATM_VC_T vc_entry = {0};
	int i;
	char modified = 0;

	for(i = entryNum - 1; i >= 0; i--)
	{
		mib_chain_get(MIB_DOT1AG_TBL, i, (void *)&Entry);

		if(getATMVCEntryByIfIndex(Entry.ifIndex, &vc_entry) == NULL)
		{
			//WAN ifIndex is modified or changed
			mib_chain_delete(MIB_DOT1AG_TBL, i);
			modified = 1;
		}
	}

	if(modified)
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
}

static int filter_dot1ag(const struct dirent *dir)
{
	int ret1, ret2;

	if( fnmatch("dot1agd.pid.*", dir->d_name, 0) == 0
		||  fnmatch("dot1ag_ccd.pid.*", dir->d_name, 0) == 0)
		return 1;
	else
		return 0;
}
void stopDot1ag()
{
	struct dirent **list;
	int n = 0;

	n = scandir("/var/run", &list, filter_dot1ag, NULL);

	if(n > 0)
	{
		while(n--)
		{
			int pid;
			FILE *file;
			char fname[128] = {0};

			sprintf(fname, "/var/run/%s", list[n]->d_name);

			pid = read_pid(fname);
			if(pid <= 0)
			{
				fprintf(stderr, "Read PID file failed from: %s.\n", list[n]->d_name);
				fclose(file);
				free(list[n]);
				continue;
			}

			kill(pid, SIGTERM);

			free(list[n]);
		}
		free(list);
		list = NULL;
	}
}

void startDot1ag()
{
	MIB_CE_DOT1AG_T entry = {0};
	int total;
	int i;
	char arg[60] = {0};

	arrange_dot1ag_table();
	stopDot1ag();

	total = mib_chain_total(MIB_DOT1AG_TBL);
	for(i = 0 ; i < total ; i++)
	{
		char *argv[20] = {0};
		int idx = 1;
		char ifname[IFNAMSIZ] = {0};

		mib_chain_get(MIB_DOT1AG_TBL, i, &entry);

		ifGetName(entry.ifIndex, ifname, IFNAMSIZ);

		argv[idx++] = "-i";
		argv[idx++] = ifname;

		do_nice_cmd("/bin/dot1agd", argv, 0);

		if(entry.ccm_enable)
		{
			char str_interval[20] = {0};
			char str_level[4] = {0};
			char str_mep_id[8] = {0};

			argv[idx++] = "-t";
			argv[idx++] = str_interval;
			sprintf(str_interval, "%u", entry.ccm_interval);

			argv[idx++] = "-d";
			argv[idx++] = entry.md_name;

			argv[idx++] = "-l";
			argv[idx++] = str_level;
			sprintf(str_level, "%u", entry.md_level);

			argv[idx++] = "-a";
			argv[idx++] = entry.ma_name;

			argv[idx++] = "-m";
			argv[idx++] = str_mep_id;
			sprintf(str_mep_id, "%u", entry.mep_id);

			do_nice_cmd("/bin/dot1ag_ccd", argv, 0);
		}
	}
}
#endif


#ifdef _CWMP_MIB_
#define NOT_IPV6_IP 1
#define IPV6_IP 2
int set_endpoint(char *newurl, char *url) //star: remove "http://" from url string
{
	register const char *s;
	register size_t i, n;
	newurl[0] = '\0';
	int urltype = NOT_IPV6_IP;
	int isIP=0;
	struct in_addr ina;
	struct in6_addr ina6;

	if (!url || !*url)
		return -1;
	s = strchr(url, ':');
	if (s && s[1] == '/' && s[2] == '/' && s[3] != '[')
		s += 3;
	else if (s && s[1] == '/' && s[2] == '/' && s[3] == '[') {
		s += 4;
		urltype = IPV6_IP;
	}
	else
		s = url;

	n = strlen(s);
	if(n>256)
		n=256;

	for (i = 0; i < n; i++)
	{
		newurl[i] = s[i];
		// NOT_IPV6_IP
		if (urltype == NOT_IPV6_IP) {
			if (s[i] == '/' || s[i] == ':')
				break;
		}

		// IPV6_IP
		if (urltype == IPV6_IP) {
			if (s[i] == ']') {
				//printf("This is IPv6 IP format\n");
				break;
			}
		}
	}

	newurl[i] = '\0';
	if(inet_pton(AF_INET6, newurl, &ina6) > 0 || inet_pton(AF_INET, newurl, &ina) > 0)
		isIP = 1;

	//printf("newurl=%s, isIP=%d\n", newurl, isIP);
	return isIP;
}

int set_urlport(char *url, char *newurl, unsigned int *port) //star: remove "http://" from url string
{
	register const char *s;
	register size_t i, n;
	newurl[0] = '\0';
	int urltype = NOT_IPV6_IP;
	int isIP=0;
	struct in_addr ina;
	struct in6_addr ina6;
	*port = 80;

	if (!url || !*url)
		return -1;

	if (!strncmp(url, "https:", 6))
    	*port = 443;

	s = strchr(url, ':');
	if (s && s[1] == '/' && s[2] == '/' && s[3] != '[')
		s += 3;
	else if (s && s[1] == '/' && s[2] == '/' && s[3] == '[') {
		s += 4;
		urltype = IPV6_IP;
	}
	else
		s = url;

	n = strlen(s);
	if(n>256)
		n=256;

	for (i = 0; i < n; i++)
	{
		newurl[i] = s[i];
		// NOT_IPV6_IP
		if (urltype == NOT_IPV6_IP) {
			if (s[i] == '/' || s[i] == ':')
				break;
		}

		// IPV6_IP
		if (urltype == IPV6_IP) {
			if (s[i] == ']') {
				//printf("This is IPv6 IP format\n");
				break;
			}
		}
	}

	newurl[i] = '\0';
	if(inet_pton(AF_INET6, newurl, &ina6) > 0 || inet_pton(AF_INET, newurl, &ina) > 0)
		isIP = 1;

	if (s[i] == ':')
	{
		*port = (int)atol(s + i + 1);
		for (i++; i < n; i++)
		if (s[i] == '/')
			break;
	}

	//printf("newurl=%s, isIP=%d\n", newurl, isIP);
	return isIP;
}

static const char OLDACSFILE[] = "/var/oldacs";
static const char OLDACSFILE_FOR_WAN[] = "/var/oldacs_wan";  // Magician: Used for TR-069 WAN interface.

void storeOldACS(void)
{
	FILE* fp;
	char acsurl[256+1]={0};

	unsigned char dynamic_acs_url_selection = 0;
	mib_get_s(CWMP_DYNAMIC_ACS_URL_SELECTION, &dynamic_acs_url_selection, sizeof(dynamic_acs_url_selection));
	int acs_url_mib_id = CWMP_ACS_URL;
	if (dynamic_acs_url_selection)
	{
		acs_url_mib_id = RS_CWMP_USED_ACS_URL;
	}

	mib_get_s(acs_url_mib_id, acsurl, sizeof(acsurl));

	if(strlen(acsurl))
	{
		if(fp = fopen(OLDACSFILE, "w"))
		{
			fprintf(fp, "%s",acsurl);
			fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
			chmod(OLDACSFILE, 0666);
#endif
		}

		if(fp = fopen(OLDACSFILE_FOR_WAN, "w"))
		{
			fprintf(fp, "%s",acsurl);
			fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
			chmod(OLDACSFILE_FOR_WAN, 0666);
#endif

		}
	}
}

int getOldACS(char *acsurl)
{
	FILE* fp;

	acsurl[0]=0;
	fp=fopen(OLDACSFILE,"r");
	if(fp){
		if(fscanf(fp, "%s", acsurl) != 1)
			printf("fscanf failed: %s %d\n", __func__, __LINE__);
		fclose(fp);
		unlink(OLDACSFILE);
	}
	if(strlen(acsurl))
		return 1;
	else
		return 0;
}
/*star:20100305 END*/

int getOldACSforWAN(char *acsurl)
{
	FILE* fp;

	acsurl[0]=0;
	fp=fopen(OLDACSFILE_FOR_WAN,"r");
	if(fp){
		if(fscanf(fp,"%s",acsurl) != 1)
			printf("fscanf failed: %s %d\n", __func__, __LINE__);
		fclose(fp);
		unlink(OLDACSFILE_FOR_WAN);
	}
	if(strlen(acsurl))
		return 1;
	else
		return 0;
}
#endif

char *trim_white_space(char *str)
{
	char *end;

	// Trim leading space
	while (isspace(*str)) str++;

	if(*str == 0)  // All spaces?
		return str;

	// Trim trailing space
	end = str + strlen(str) - 1;
	while (end >= str && isspace(*end)) end--;

	// Write new null terminator
	*(end+1) = 0;

	return str;
}

// Magician: This function is for checking the validation of whole config file.
/*
 *	Check the validation of config file.
 *	Return:
 *		0 for failed or Config file type (CONFIG_DATA_T) if successful.
 */
int checkConfigFile(const char *filename, int check_enc)
{
	FILE *fp;
	char strbuf[MAX_MIB_VAL_STR_LEN], *pbuf, *pstr;
	int linenum = 1, len;
	char inChain = 0, isEnd = 0;
	int chainLevel=0;
	int status;
	char *config_file = (char *)filename;
	char tempFile[32]={0};

	if(check_enc==1 && check_xor_encrypt((char*)filename)==1){
		/* decrypt to plain text config file */
		config_file = tempFile;
		xor_encrypt((char*)filename, tempFile);
	}

	if(!(fp = fopen(config_file, "r")))
	{
		printf("Open config file failed: %s\n", strerror(errno));
		status = 0;
		goto ret;
	}
	flock(fileno(fp), LOCK_SH);

	if(fgets(strbuf, sizeof(strbuf), fp) == NULL || !(len = strlen(strbuf)) || strbuf[len-1] != '\n')
	{
		printf("[%s %d]Miss a newline at line %d!\n", __func__, __LINE__, linenum);
		flock(fileno(fp), LOCK_UN);
		fclose(fp);
		status = 0;
		goto ret;
	}

	if(strbuf[len-2] == '\r' && strbuf[len-1] == '\n') // Remove the CRLF(0x0D0A) which is at the last of a line.
		strbuf[len-2] = 0;
	else if(strbuf[len-1] == '\n') // Remove the LF(0x0A) which is at the last of a line.
		strbuf[len-1] = 0;

	pbuf = trim_white_space(strbuf);
	if(!strcmp(pbuf, CONFIG_HEADER))
		status = CURRENT_SETTING;
	else if(!strcmp(pbuf, CONFIG_HEADER_HS))
		status = HW_SETTING;
	else
	{
		printf("[%s %d]Invalid header: %s\n", __func__, __LINE__, strbuf);
		flock(fileno(fp), LOCK_UN);
		fclose(fp);
		status = 0;
		goto ret;
	}

	while(fgets(strbuf, sizeof(strbuf), fp))
	{
		linenum++;  // The counter of current line on handling.
		len = strlen(strbuf);
		//printf("%d: %d: %s", linenum, len, strbuf);

		if(strbuf[len-1] != '\n')   // If this line does not end with a newline, return an error.
		{
			printf("[%s %d]Miss a newline at line %d!\n", __func__, __LINE__, linenum);
			status = 0;
			break;
		}

		if(strbuf[len-2] == '\r' && strbuf[len-1] == '\n') // Remove the CRLF(0x0D0A) which is at the last of a line.
			strbuf[len-2] = 0;
		else if(strbuf[len-1] == '\n') // Remove the LF(0x0A) which is at the last of a line.
			strbuf[len-1] = 0;

		pbuf = trim_white_space(strbuf);
		if (strlen(pbuf)==0)
			continue;

		if(isEnd)  // It should be at the end of this config file.
		{
			printf("[%s %d]Invalid end of config file at line %d\n", __func__, __LINE__, linenum);
			status = 0;
			break;
		}

		if(!strncmp(pbuf, "<Value Name=\"", 13))  // Check the validation of common <Value Name.... line.
		{
			if(!(pstr = strchr(pbuf+13, '\"')))
			{
				printf("[%s %d]Invalid format at line %d: %s\n", __func__, __LINE__, linenum, strbuf);
				status = 0;
				break;
			}

			do{
				pstr++;
			} while (isspace(*pstr));

			if(strncmp(pstr, "Value=\"", 7))
			{
				printf("[%s %d]Invalid format at line %d: %s\n", __func__, __LINE__, linenum, strbuf);
				status = 0;
				break;
			}

			if(!(pstr = strrchr(pstr+7, '"')))
			{
				printf("[%s %d]Invalid format at line %d: %s\n", __func__, __LINE__, linenum, strbuf);
				status = 0;
				break;
			}

			do{
				pstr++;
			} while (isspace(*pstr));

			if(strncmp(pstr, "/>", 2))
			{
				printf("[%s %d]Invalid format at line %d: %s\n", __func__, __LINE__, linenum, strbuf);
				status = 0;
				break;
			}

			pstr += 2;

			if(*pstr != '\0')
			{
				printf("[%s %d]Invalid format at line %d: %s\n", __func__, __LINE__, linenum, strbuf);
				status = 0;
				break;
			}

			continue;
		}

		if(!strncmp(pbuf, "<chain chainName=\"", 18))  // Enter in a chain table.
		{
			if(!(pstr = strchr(pbuf+18, '\"')))
			{
				printf("[%s %d]Invalid format at line %d: %s\n", __func__, __LINE__, linenum, strbuf);
				status = 0;
				break;
			}

			do{
				pstr++;
			} while (isspace(*pstr));

			if(*pstr != '>')
			{
				printf("[%s %d]Invalid format at line %d: %s\n", __func__, __LINE__, linenum, strbuf);
				status = 0;
				break;
			}

			do{
				pstr++;
			} while (isspace(*pstr));

			if(!strncmp(pstr, "<!--index=", 10)) //if have the index '<!--index=0-->'
			{
				if(!(pstr = strchr(pstr+10, '-')))
				{
					printf("[%s %d]Invalid format at line %d: %s\n", __func__, __LINE__, linenum, strbuf);
					status = 0;
					break;
				}

				pstr++;

				if(strncmp(pstr, "->", 2))
				{
					printf("[%s %d]Invalid format at line %d: %s\n", __func__, __LINE__, linenum, strbuf);
					status = 0;
					break;
				}

				pstr+=2;

			if(*pstr != '\0')
			{
				printf("[%s %d]Invalid format at line %d: %s\n", __func__, __LINE__, linenum, strbuf);
				status = 0;
				break;
			}

			}
			else{  //have no index <!--index=0-->

				if(*pstr != '\0')
				{
					printf("[%s %d]Invalid format at line %d: %s\n", __func__, __LINE__, linenum, strbuf);
					status = 0;
					break;
				}
			}

			chainLevel++;
			continue;
		}

		if(!strncmp(pbuf, "</chain>", 8))  // Leave from a chain table.
		{
			if (chainLevel <=0)
			{
				printf("[%s %d]Invalid structure at line %d: %s\n", __func__, __LINE__, linenum, strbuf);
				status = 0;
				break;
			}

			chainLevel--;
			continue;
		}

		if(!strcmp(pbuf, CONFIG_TRAILER) || !strcmp(pbuf, CONFIG_TRAILER_HS))  // Reach the end of config file.
		{
			isEnd = 1;
			continue;
		}

		printf("[%s %d]Unknown format at line %d: %s\n", __func__, __LINE__, linenum, strbuf);
		status = 0;
		break;
	}

	if(isEnd==0){
		status = 0;
	}
	flock(fileno(fp), LOCK_UN);
	fclose(fp);
ret:
	if (strlen(tempFile))
		unlink(tempFile);
	return status;
}

// Magician: This function is used for memory changing watch. You can put it anywhere to detect if memory usage changes.
// Be sure you assign all these functions in THE SAME process.
#if DEBUG_MEMORY_CHANGE
char last_memsize[128], last_file[32], last_func[32]; // Use to indicate last position where you put ShowMemChange().
int last_line;  // Use to indicate last position where you put ShowMemChange().
int ShowMemChange(char *file, char *func, int line)
{
	FILE *fp, *logfp;
	char buf[128];
	int i;
	char isPntOnChg = 1;  // Print message when memory size changed.
	char isLog = 1;  // Log results in /tmp/memlog
	char status = 0;  // return 1 when memory usage does change or otherwise return 0.

	if(isLog && !(logfp = fopen("/tmp/memlog", "a")))
	{
		perror("/tmp/memlog");
		return -1;
	}

	sprintf(buf, "/proc/%d/status", getpid());

	if(fp = fopen(buf, "r"))
	{
		for( i = 0; i < 11; i++ )
			fgets(buf, 128, fp);

		fclose(fp);

		if(!isPntOnChg || strcmp(buf, last_memsize))
		{
			putchar('\n');

			if(isPntOnChg)
			{
				printf("===== Last Memory size info (%s:%s:%d) =====\n", last_file, last_func, last_line);
				printf(last_memsize);

				if(isLog)
				{
					fprintf(logfp, "===== Last Memory size info (%s:%s:%d) =====\n", last_file, last_func, last_line);
					fprintf(logfp, last_memsize);
				}
			}

			printf("===== Memory size info (%s:%s:%d) =====\n", file, func, line);
			printf(buf);

			if(isLog)
			{
				fprintf(logfp, "===== Memory size info (%s:%s:%d) =====\n", file, func, line);
				fprintf(logfp, "%s\n", buf);
			}

			status = 1;
		}

		strncpy(last_memsize, buf, 128);
		strncpy(last_file, file, 32);
		strncpy(last_func, func, 32);
		last_line = line;
	}

	fclose(logfp);
	return status;
}
#endif
//Kevin:Check whether to enable/disable upstream ip fastpath
void UpdateIpFastpathStatus(void)
{
    /* If any one of the folllowing functions is enabled,
           we have to disable upstream ip fastpath.
               (1) IP Qos
               (2) URL blocking
               (3) Domain blocking
           Otherwise, keep ip fastpath up/downstream both to enhance throughput.
       */

	unsigned char mode=0, fastpath_us_disabled = 0;

#ifdef CONFIG_USER_IP_QOS
	unsigned int qosEnable;
	unsigned int qosRuleNum, carRuleNum;
	unsigned char totalBandWidthEn;

	qosEnable = getQosEnable();
	qosRuleNum = getQosRuleNum();
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	// don't affect in multi phy wan
	totalBandWidthEn = 0;
#else
	mib_get_s(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&totalBandWidthEn, sizeof(totalBandWidthEn));
#endif
	carRuleNum = mib_chain_total(MIB_IP_QOS_TC_TBL);
	if ((qosEnable && qosRuleNum)) {
#if !defined(CONFIG_RTL_ADV_FAST_PATH) && (defined(CONFIG_RTL867X_IPTABLES_FAST_PATH) || defined(CONFIG_RTK_FASTPATH))
		system("/bin/echo 1 > /proc/FastPath");
		fastpath_us_disabled = 1;
#endif
	}
	else if (carRuleNum) {
#if defined(CONFIG_RTL867X_IPTABLES_FAST_PATH) || defined(CONFIG_RTK_FASTPATH)
		system("/bin/echo 1 > /proc/FastPath");
		fastpath_us_disabled = 1;
#endif
	}
#endif /* CONFIG_USER_IP_QOS */

#if defined(CONFIG_RTL867X_IPTABLES_FAST_PATH) || defined(CONFIG_RTK_FASTPATH)
#if defined(URL_BLOCKING_SUPPORT) || defined(URL_ALLOWING_SUPPORT)
	mode=0;
	mib_get_s(MIB_URL_CAPABILITY, (void *)&mode, sizeof(mode));
	if (mode)
	{
		//printf("(%s)URL blocking!\n",__func__);
		system("/bin/echo 1 > /proc/FastPath");
		fastpath_us_disabled = 1;
	}
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
	mode=0;
	mib_get_s(MIB_DOMAINBLK_CAPABILITY, (void *)&mode, sizeof(mode));
	if (mode)
	{
		//printf("(%s)Domain blocking!\n",__func__);
		system("/bin/echo 1 > /proc/FastPath");
		fastpath_us_disabled = 1;
	}
#endif

	//printf("(%s)none!\n",__func__);
	if (fastpath_us_disabled == 0){
		#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)&&defined(CONFIG_RTK_DEV_AP)
		system("/bin/echo 1 > /proc/fc/ctrl/hwnat");
		#else
		system("/bin/echo 2 > /proc/FastPath");
		#endif
	}
#endif /* defined(CONFIG_RTL867X_IPTABLES_FAST_PATH) || defined(CONFIG_RTK_FASTPATH) */
}

#ifdef RESERVE_KEY_SETTING
#ifdef CONFIG_E8B
/*
 * flag
 * 0: for short reset
 * 1: for long reset
 * 2: TR-069 FactoryReset
 * 3: reset all like "flash default cs"
 * 4: for CU liaoning long reset, do not keep LOID
 */
static int reserve_critical_setting(int flag)
{
	MIB_CE_ATM_VC_T tr069_wan;
	int tr069_wan_found = 0;
	int total;
	int i = 0;
	unsigned char functype=0;
	unsigned char cwmp_do_factory_reset = 0;

	//FIXME: need this ?
	//mib_get_s(PROVINCE_CWMP_RESET_AS_FACTORY_RESET, &cwmp_do_factory_reset, sizeof(cwmp_do_factory_reset));

	fprintf(stderr, "Reset flag = %d\n", flag);

	//first backup current setting
	mib_backup(CONFIG_MIB_ALL);

	// Backup TR-069 WAN
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, &tr069_wan) == 0)
			continue;

		if(tr069_wan.applicationtype & X_CT_SRV_TR069)
		{
			tr069_wan_found = 1;
			break;
		}
	}

	// restore current to default
#ifdef CONFIG_USER_XMLCONFIG
	va_cmd("/bin/sh",2,1, "/etc/scripts/config_xmlconfig.sh", "-d");
#else
	mib_sys_to_default(CURRENT_SETTING);
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	//always retrieve restore status
	mib_retrive_table(MIB_RESTORE_STATUS_TBL);
	if(flag == 1)
	{
		//restore framework configuration
#ifdef CONFIG_YUEME
		system("dbus-send --system --print-reply --dest=com.ctc.saf1 /com/ctc/saf1 com.ctc.saf1.framework.Restore");
#elif defined(CONFIG_CU_BASEON_YUEME)
		system("dbus-send --system --print-reply --dest=com.cuc.ufw1 /com/cuc/ufw1 com.cuc.ufw1.framework.Restore");
#endif
	}
#endif

#ifdef CONFIG_CU
	unsigned char cu_sd_reverse_route=0;
	mib_get(PROVINCE_CU_SD_REVERSE_STATIC_ROUTE, (void *)&cu_sd_reverse_route);
	if(cu_sd_reverse_route)
		mib_retrive_chain(MIB_IP_ROUTE_TBL);
#endif
#if defined(CONFIG_CMCC_ENTERPRISE) && defined(_PRMT_X_CMCC_DEVICEINFO_)
	mib_retrive_table(MIB_FACTORYRESET_MODE);
#endif
	if(flag == 3)
		return 0;

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	if(flag == 1)
	{
		unsigned int ch = 0;
		mib_get_s(CWMP_FLAG2, (void *)&ch, sizeof(ch));
		ch &= ~(CWMP_FLAG2_HAD_SENT_LONGRESET);
		mib_set(CWMP_FLAG2, (void *)&ch);
          	return 0;
	}
	else if(flag == 2 && cwmp_do_factory_reset)	//Reset to factory default
		return 0;
#endif

	//now retrieve the key parameters.
#if defined(CONFIG_CU_BASEON_CMCC)
	if(flag != 4)
#endif
	{
		mib_retrive_table(MIB_LOID);
		mib_retrive_table(MIB_LOID_OLD);
		mib_retrive_table(MIB_LOID_PASSWD);
		mib_retrive_table(MIB_LOID_PASSWD_OLD);
	}

#ifdef CONFIG_USER_CTMANAGEDEAMON
	mib_retrive_table(MIB_BUCPE_A_LOCATION_OK);
	mib_retrive_table(MIB_BUCPE_A_LOCATION_LONGITUDE);
	mib_retrive_table(MIB_BUCPE_A_LOCATION_LATITUDE);
	mib_retrive_table(MIB_BUCPE_A_LOCATION_ALTITUDE);
	mib_retrive_table(MIB_BUCPE_A_LOCATION_HORIZONTALERROR);
	mib_retrive_table(MIB_BUCPE_A_LOCATION_ALTITUDEERROR);
	mib_retrive_table(MIB_BUCPE_A_AREACODE);
	mib_retrive_table(MIB_BUCPE_A_GISLOCKTIME);
	mib_retrive_table(MIB_BUCPE_A_GISDIGEST);
	mib_retrive_table(MIB_BUCPE_B_LOCATION_OK);
	mib_retrive_table(MIB_BUCPE_B_LOCATION_LONGITUDE);
	mib_retrive_table(MIB_BUCPE_B_LOCATION_LATITUDE);
	mib_retrive_table(MIB_BUCPE_B_LOCATION_ALTITUDE);
	mib_retrive_table(MIB_BUCPE_B_LOCATION_HORIZONTALERROR);
	mib_retrive_table(MIB_BUCPE_B_LOCATION_ALTITUDEERROR);
	mib_retrive_table(MIB_BUCPE_B_AREACODE);
	mib_retrive_table(MIB_BUCPE_B_GISLOCKTIME);
	mib_retrive_table(MIB_BUCPE_B_GISDIGEST);
	mib_retrive_table(MIB_BUCPE_REGID);
	mib_retrive_table(MIB_BUCPE_UUID);
	mib_retrive_table(MIB_BUCPE_SPEED_URL);
	mib_retrive_table(MIB_BUCPE_SPEED_URL_BAK);
#endif
#if defined(CONFIG_GPON_FEATURE)
	mib_retrive_table(MIB_GPON_PLOAM_PASSWD);
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	//TR-069 Factory Reset & short reset
	mib_retrive_table(CWMP_GUI_PASSWORD_ENABLE);
#endif

#if defined(CONFIG_EPON_FEATURE)
	mib_retrive_chain(MIB_EPON_LLID_TBL);
#endif

#if defined(CONFIG_CU_BASEON_CMCC)
	if(flag == 2 || flag == 4)
#else
	if(flag == 2)
#endif
	{
#ifdef CONFIG_CU
		mib_retrive_table(CWMP_ACS_URL);
		mib_retrive_table(CWMP_ACS_URL_OLD);
		mib_retrive_table(CWMP_ACS_USERNAME);
		mib_retrive_table(CWMP_ACS_PASSWORD);
		mib_retrive_table(CWMP_CONREQ_USERNAME);
		mib_retrive_table(CWMP_CONREQ_PASSWORD);

		if(tr069_wan_found)
		{
			// Reserve old TR-069 WAN
			MIB_CE_ATM_VC_T entry;
			unsigned char hasDefaultTr069Wan = 0;
			total = mib_chain_total(MIB_ATM_VC_TBL);

			for(i=0 ; i < total ; i++)
			{
				mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry);

				if(entry.applicationtype & X_CT_SRV_TR069)
				{
					mib_chain_update(MIB_ATM_VC_TBL, &tr069_wan, i);
					hasDefaultTr069Wan = 1;
					break;
				}
			}
			if(!hasDefaultTr069Wan){
				/*
					The TR069 WAN is added by user.
				*/
				mib_chain_add(MIB_ATM_VC_TBL, &tr069_wan);
			}
		}
#endif

#if defined(CONFIG_YUEME)// || defined(CONFIG_CU_BASEON_YUEME)
		//mib_retrive_chain(MIB_INTERNET_GATEWAY_DEVICE_TBL);
#ifdef CONFIG_USER_L2TPD_L2TPD
		mib_retrive_table(MIB_L2TP_ENABLE);
		mib_retrive_chain(MIB_L2TP_TBL);
		mib_retrive_chain(MIB_L2TP_ROUTE_TBL);
#endif
#ifdef CONFIG_USER_PPTP_CLIENT_PPTP
		mib_retrive_table(MIB_PPTP_ENABLE);
		mib_retrive_chain(MIB_PPTP_TBL);
		mib_retrive_chain(MIB_PPTP_ROUTE_TBL);
#endif
#if 0//def CONFIG_CU
		//CU Management Platform
		mib_retrive_table(CU_SRVMGT_MGTURL);
		mib_retrive_table(CU_SRVMGT_MGTPORT);
		mib_retrive_table(CU_SRVMGT_HB);
		mib_retrive_table(CU_SRVMGT_PROVINCE);
#else
		mib_retrive_table(MIB_CWMP_MGT_URL);
		mib_retrive_table(MIB_CWMP_MGT_PORT);
		mib_retrive_table(MIB_CWMP_MGT_HEARTBEAT);
		mib_retrive_table(MIB_CWMP_MGT_ABILITY);
		mib_retrive_table(MIB_CWMP_MGT_LOCATEPORT);
		mib_retrive_table(MIB_CWMP_MGT_VERSION);
		mib_retrive_table(MIB_CWMP_MGT_APPMODEL);
		mib_retrive_table(MIB_CWMP_MGT_SSN);
		mib_retrive_table(MIB_PLATFORM_BSSADDR_TBL);
#endif
#ifdef WLAN_SUPPORT
#ifdef WIFI_TIMER_SCHEDULE
		mib_retrive_chain(MIB_WIFI_TIMER_EX_TBL);
		mib_retrive_chain(MIB_WIFI_TIMER_TBL);
#endif
#endif
#ifdef LED_TIMER
		mib_retrive_chain(MIB_LED_INDICATOR_TIMER_TBL);
#endif
		mib_retrive_table(MIB_LED_STATUS);
#ifdef SLEEP_TIMER
		mib_retrive_chain(MIB_SLEEP_MODE_SCHED_TBL);
#endif
		mib_retrive_table(MIB_HTTPDOWNLOAD_TBL);

#ifdef FTP_SERVER_INTERGRATION
		mib_retrive_table(MIB_FTP_ENABLE);
		mib_retrive_table(MIB_FTP_ANNONYMOUS);
#else
		mib_retrive_chain(MIB_VSFTP_ACCOUNT_TBL);
		mib_retrive_table(MIB_VSFTP_ENABLE);
		mib_retrive_table(MIB_VSFTP_ANNONYMOUS);
#endif
#ifdef CONFIG_MULTI_FTPD_ACCOUNT
		mib_retrive_chain(MIB_FTP_ACCOUNT_TBL);
#endif

		mib_retrive_table(MIB_SAMBA_ENABLE);
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
		mib_retrive_table(MIB_SAMBA_ENABLE);
		mib_retrive_chain(MIB_SMBD_ACCOUNT_TBL);
#endif
#if 0//def SUPPORT_WEB_REDIRECT
		mib_retrive_table(MIB_REDIRECT_IP);
		mib_retrive_table(MIB_ITMS_ADDR);
		mib_retrive_table(MIB_404_REDIRECT_URL);
		mib_retrive_table(MIB_404_REDIRECT_ENABLE);
#endif
#endif

#ifdef CONFIG_CU
		//CU Management Platform
		mib_retrive_table(CU_SRVMGT_MGTURL);
		mib_retrive_table(CU_SRVMGT_MGTPORT);
		mib_retrive_table(CU_SRVMGT_HB);
		mib_retrive_table(CU_SRVMGT_PROVINCE);
#endif

		// TR-069 FactoryReset should not sent LONGRESET
		unsigned int ch = 0;
		mib_get_s(CWMP_FLAG2, (void *)&ch, sizeof(ch));
		ch |= CWMP_FLAG2_HAD_SENT_LONGRESET;
		mib_set(CWMP_FLAG2, (void *)&ch);

        mib_retrive_table(PROVINCE_RESERVE_KEY_SETTING);
		unsigned char province_reserve = 0;
		mib_get_s(PROVINCE_RESERVE_KEY_SETTING, &province_reserve, sizeof(province_reserve));
		fprintf(stderr, "[%s:%s@%d] province_reserve=%d \n", __FILE__, __FUNCTION__, __LINE__, province_reserve);
        if (province_reserve == RESERVE_KEY_SETTING_SC_1)
        {
            mib_retrive_table(MIB_SUSER_PASSWORD);
        }
		if (province_reserve == RESERVE_KEY_SETTING_JSU_1)
		{
#ifdef _CWMP_MIB_
			mib_retrive_table(CWMP_ACS_USERNAME);
			mib_retrive_table(CWMP_ACS_PASSWORD);
			mib_retrive_table(CWMP_CONREQ_USERNAME);
			mib_retrive_table(CWMP_CONREQ_PASSWORD);
			mib_retrive_table(CWMP_INFORM_ENABLE);
			mib_retrive_table(CWMP_INFORM_INTERVAL);
			mib_retrive_table(CWMP_INFORM_TIME);
#endif

#if defined(_PRMT_X_CT_COM_USERINFO_) && defined(E8B_NEW_DIAGNOSE)
			if(flag != 1)
			{
				/* Not long reset, remember register result for GUI presentation */
				mib_retrive_table(CWMP_USERINFO_RESULT);
				mib_retrive_table(CWMP_USERINFO_STATUS);
			}
#endif
		}

		return 0;
	}

#ifdef _CWMP_MIB_
	mib_retrive_table(CWMP_ACS_URL);
	mib_retrive_table(CWMP_ACS_URL_OLD);
	mib_retrive_table(CWMP_ACS_USERNAME);
	mib_retrive_table(CWMP_ACS_PASSWORD);
	mib_retrive_table(CWMP_CONREQ_USERNAME);
	mib_retrive_table(CWMP_CONREQ_PASSWORD);
#endif

	mib_retrive_table(MIB_SUSER_NAME);
#if !defined(CONFIG_CU_BASEON_CMCC)
	mib_get_s(PROVINCE_MISCFUNC_TYPE, &functype, sizeof(functype));
	if (functype != 1) // Anhui have to restore telecomadmin password
	{
		mib_retrive_table(MIB_SUSER_PASSWORD);
	}
#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	mib_retrive_chain(MIB_IP_ROUTE_TBL);
#endif

	if(flag == 1)
	{
		unsigned int ch = 0;
		mib_get_s(CWMP_FLAG2, (void *)&ch, sizeof(ch));
		ch &= ~(CWMP_FLAG2_HAD_SENT_LONGRESET);
		mib_set(CWMP_FLAG2, (void *)&ch);

		if(tr069_wan_found)
		{
			// Reserve old TR-069 WAN
			MIB_CE_ATM_VC_T entry;
			total = mib_chain_total(MIB_ATM_VC_TBL);

			for(i=0 ; i < total ; i++)
			{
				mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry);

				if(entry.applicationtype & X_CT_SRV_TR069)
				{
					mib_chain_update(MIB_ATM_VC_TBL, &tr069_wan, i);
					break;
				}
			}
		}
	}
	else
	{
		mib_retrive_chain(MIB_ATM_VC_TBL);
#ifdef CONFIG_CU
		// short reset should not send LONGRESET
		// should send 0 BOOTSTRAP
		unsigned int ch = 0;
		mib_get_s(CWMP_FLAG2, (void *)&ch, sizeof(ch));
		ch |= CWMP_FLAG2_HAD_SENT_LONGRESET;
		mib_set(CWMP_FLAG2, (void *)&ch);
#else
#ifdef _CWMP_MIB_
		mib_retrive_table(CWMP_FLAG2);
#endif
#endif
#if defined(CONFIG_RTL867X_VLAN_MAPPING) || defined(CONFIG_APOLLO_ROMEDRIVER)
		mib_retrive_chain(MIB_PORT_BINDING_TBL);
#endif

#if defined(CONFIG_CMCC)
		mib_retrive_chain(MIB_IP_ROUTE_TBL);
#endif

#ifdef WLAN_SUPPORT
		MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_DUALBAND_CONCURRENT
		int orig_wlan_idx = wlan_idx;
		wlan_idx = 0;
#endif
		mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry);
		mib_retrive_chain(MIB_MBSSIB_TBL);
		mib_chain_update(MIB_MBSSIB_TBL,(void *)&Entry, 0); //reset to default for SSID-1
#ifdef WLAN_DUALBAND_CONCURRENT
		mib_chain_get(MIB_WLAN1_MBSSIB_TBL, 0, (void *)&Entry);
		mib_retrive_chain(MIB_WLAN1_MBSSIB_TBL);
		mib_chain_update(MIB_WLAN1_MBSSIB_TBL,(void *)&Entry, 0); //reset to default for SSID-5
		wlan_idx = orig_wlan_idx;
#endif
#endif

#ifdef VOIP_SUPPORT
		mib_retrive_chain(MIB_VOIP_CFG_TBL);
#endif

#ifdef _PRMT_X_CT_COM_USERINFO_
		mib_retrive_table(CWMP_USERINFO_STATUS);
		mib_retrive_table(CWMP_USERINFO_LIMIT);
		mib_retrive_table(CWMP_USERINFO_TIMES);
		mib_retrive_table(CWMP_USERINFO_RESULT);
#endif

#ifndef CONFIG_CU_BASEON_CMCC
#ifdef _PRMT_X_CT_COM_PING_
		mib_retrive_table(CWMP_CT_PING_ENABLE);
		mib_retrive_chain(CWMP_CT_PING_TBL);
#endif
#ifdef TIME_ZONE
		mib_retrive_table(MIB_NTP_ENABLED);
		mib_retrive_table(MIB_NTP_TIMEZONE_DB_INDEX);
		mib_retrive_table(MIB_DST_ENABLED);
		mib_retrive_table(MIB_NTP_SERVER_HOST1);
		mib_retrive_table(MIB_NTP_SERVER_HOST2);
		mib_retrive_table(MIB_NTP_SERVER_HOST3);
		mib_retrive_table(MIB_NTP_SERVER_HOST4);
		mib_retrive_table(MIB_NTP_SERVER_HOST5);
		mib_retrive_table(MIB_NTP_IF_TYPE);
		mib_retrive_table(MIB_NTP_IF_WAN);
		mib_retrive_table(MIB_NTP_INTERVAL);
#endif
#ifdef VIRTUAL_SERVER_SUPPORT
		mib_retrive_chain(MIB_VIRTUAL_SVR_TBL);
#endif
#ifdef _PRMT_X_CT_COM_MWBAND_
		mib_retrive_table(CWMP_CT_MWBAND_MODE);
		mib_retrive_table(CWMP_CT_MWBAND_STB_ENABLE);
		mib_retrive_table(CWMP_CT_MWBAND_CMR_ENABLE);
		mib_retrive_table(CWMP_CT_MWBAND_PC_ENABLE);
		mib_retrive_table(CWMP_CT_MWBAND_PHN_ENABLE);
		mib_retrive_table(CWMP_CT_MWBAND_NUMBER);
		mib_retrive_table(CWMP_CT_MWBAND_STB_NUM);
		mib_retrive_table(CWMP_CT_MWBAND_CMR_NUM);
		mib_retrive_table(CWMP_CT_MWBAND_PC_NUM);
		mib_retrive_table(CWMP_CT_MWBAND_PHN_NUM);
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
		//mib_retrive_table(MIB_RESTORE_STATUS_TBL);
		//mib_retrive_chain(MIB_INTERNET_GATEWAY_DEVICE_TBL);
#if defined(CONFIG_YUEME)
		mib_retrive_chain(MIB_DHCPS_SERVING_POOL_TBL);
		mib_retrive_chain(MIB_PORT_BINDING_TBL);
#endif

#ifdef CONFIG_USER_L2TPD_L2TPD
		mib_retrive_table(MIB_L2TP_ENABLE);
		mib_retrive_chain(MIB_L2TP_TBL);
		mib_retrive_chain(MIB_L2TP_ROUTE_TBL);
#endif

#ifdef CONFIG_USER_PPTP_CLIENT_PPTP
		mib_retrive_table(MIB_PPTP_ENABLE);
		mib_retrive_chain(MIB_PPTP_TBL);
		mib_retrive_chain(MIB_PPTP_ROUTE_TBL);
#endif
#ifdef CONFIG_CU
		//CU Management Platform
		mib_retrive_table(CU_SRVMGT_MGTURL);
		mib_retrive_table(CU_SRVMGT_MGTPORT);
		mib_retrive_table(CU_SRVMGT_HB);
		mib_retrive_table(CU_SRVMGT_PROVINCE);
#else
		mib_retrive_table(MIB_CWMP_MGT_URL);
		mib_retrive_table(MIB_CWMP_MGT_PORT);
		mib_retrive_table(MIB_CWMP_MGT_HEARTBEAT);
		mib_retrive_table(MIB_CWMP_MGT_ABILITY);
		mib_retrive_table(MIB_CWMP_MGT_LOCATEPORT);
		mib_retrive_table(MIB_CWMP_MGT_VERSION);
		mib_retrive_table(MIB_CWMP_MGT_APPMODEL);
		mib_retrive_table(MIB_CWMP_MGT_SSN);
		mib_retrive_table(MIB_PLATFORM_BSSADDR_TBL);
#endif
#endif

#ifdef FTP_SERVER_INTERGRATION
		mib_retrive_table(MIB_FTP_ENABLE);
		mib_retrive_table(MIB_FTP_ANNONYMOUS);
#else
		mib_retrive_chain(MIB_VSFTP_ACCOUNT_TBL);
		mib_retrive_table(MIB_VSFTP_ENABLE);
		mib_retrive_table(MIB_VSFTP_ANNONYMOUS);
#endif
#ifdef CONFIG_MULTI_FTPD_ACCOUNT
		mib_retrive_chain(MIB_FTP_ACCOUNT_TBL);
#endif

		mib_retrive_table(MIB_SAMBA_ENABLE);
		mib_retrive_table(MIB_SMB_ANNONYMOUS);

#ifdef CONFIG_MULTI_SMBD_ACCOUNT
		mib_retrive_chain(MIB_SMBD_ACCOUNT_TBL);
#endif
#if 0//def SUPPORT_WEB_REDIRECT
		mib_retrive_table(MIB_REDIRECT_IP);
		mib_retrive_table(MIB_ITMS_ADDR);
		mib_retrive_table(MIB_404_REDIRECT_URL);
		mib_retrive_table(MIB_404_REDIRECT_ENABLE);
#endif
#endif
#ifdef YUEME_4_0_SPEC
		mib_retrive_chain(MIB_APPROUTE_TBL);
#endif
	}

	mib_retrive_table(MIB_BRCTL_AGEINGTIME);
	mib_retrive_table(MIB_BRCTL_STP);

	return 0;
}
#else
/*
 * flag: reserved for future use.
 */
static int reserve_critical_setting(int flag)
{
	//first backup current setting
	mib_backup(CONFIG_MIB_ALL);

	// restore current to default
#ifdef CONFIG_USER_XMLCONFIG
	va_cmd(shell_name, 2, 1, "/etc/scripts/config_xmlconfig.sh", "-d");
#else
	mib_sys_to_default(CURRENT_SETTING);
#endif
	//now retrieve the key parameters.
#if 0
#ifdef _CWMP_MIB_
	mib_retrive_table(CWMP_ACS_URL);
	mib_retrive_table(CWMP_ACS_USERNAME);
	mib_retrive_table(CWMP_ACS_PASSWORD);
#endif
	mib_retrive_table(MIB_BRCTL_AGEINGTIME);
	mib_retrive_table(MIB_BRCTL_STP);
	mib_retrive_chain(MIB_ATM_VC_TBL);
#ifdef ROUTING
	mib_retrive_chain(MIB_IP_ROUTE_TBL);
#endif
#endif
	mib_retrive_table(MIB_LOID);
	mib_retrive_table(MIB_LOID_PASSWD);
#ifdef CONFIG_TRUE
	mib_retrive_table(MIB_LOID_OLD);
	mib_retrive_table(MIB_LOID_PASSWD_OLD);
#endif
#if defined(CONFIG_GPON_FEATURE)
	mib_retrive_table(MIB_GPON_PLOAM_PASSWD);
#endif
#if defined(CONFIG_EPON_FEATURE)
	mib_retrive_chain(MIB_EPON_LLID_TBL);
#endif

	return 0;
}
#endif // CONFIG_E8B
#endif // RESERVE_KEY_SETTING

static char *get_name(char *name, char *p)
{
	while (isspace(*p))
		p++;
	while (*p) {
		if (isspace(*p))
			break;
		if (*p == ':') {	/* could be an alias */
			char *dot = p, *dotname = name;
			*name++ = *p++;
			while (isdigit(*p))
				*name++ = *p++;
			if (*p != ':') {	/* it wasn't, backup */
				p = dot;
				name = dotname;
			}
			if (*p == '\0')
				return NULL;
			p++;
			break;
		}
		*name++ = *p++;
	}
	*name = '\0';

	return p;
}

static int procnetdev_version(char *buf)
{
	if (strstr(buf, "compressed"))
		return 2;
	if (strstr(buf, "bytes"))
		return 1;
	return 0;
}

static const char *const ss_fmt[] = {
	"%n%llu%llu%llu%lu%lu%n%n%n%llu%llu%llu%lu%lu%lu",
	"%llu%llu%llu%llu%lu%lu%n%n%llu%llu%llu%llu%lu%lu%lu",
	"%llu%llu%llu%llu%lu%lu%lu%llu%llu%llu%llu%llu%lu%lu%lu%lu"
};

static void get_dev_fields(char *bp, struct net_device_stats *nds, int procnetdev_vsn)
{
	memset(nds, 0, sizeof(*nds));

	sscanf(bp, ss_fmt[procnetdev_vsn],
		   &nds->rx_bytes, /* missing for 0 */
		   &nds->rx_packets,
		   &nds->rx_errors,
		   &nds->rx_dropped,
		   &nds->rx_fifo_errors,
		   &nds->rx_frame_errors,
		   &nds->rx_compressed, /* missing for <= 1 */
		   &nds->multicast, /* missing for <= 1 */
		   &nds->tx_bytes, /* missing for 0 */
		   &nds->tx_packets,
		   &nds->tx_errors,
		   &nds->tx_dropped,
		   &nds->tx_fifo_errors,
		   &nds->collisions,
		   &nds->tx_carrier_errors,
		   &nds->tx_compressed /* missing for <= 1 */
		   );

	if (procnetdev_vsn <= 1) {
		if (procnetdev_vsn == 0) {
			nds->rx_bytes = 0;
			nds->tx_bytes = 0;
		}
		nds->multicast = 0;
		nds->rx_compressed = 0;
		nds->tx_compressed = 0;
	}
}
#ifdef CONFIG_RTK_NETIF_EXTRA_STATS

static void get_dev_ext_fields(char *bp, struct net_device_stats *nds)
{
	memset(nds, 0, sizeof(*nds));

	sscanf(bp, " %llu%llu%llu%llu%llu%llu%llu%llu%llu%llu%llu%llu",
		   &nds->rx_uc_bytes,
		   &nds->rx_uc_packets,
		   &nds->tx_uc_bytes,
		   &nds->tx_uc_packets,
		   &nds->rx_bc_bytes,
		   &nds->rx_bc_packets,
		   &nds->tx_bc_bytes,
		   &nds->tx_bc_packets,
		   &nds->rx_mc_bytes,
		   &nds->multicast,
		   &nds->tx_mc_bytes,
		   &nds->tx_mc_packets
		   );

}

#endif
/**
 * list_net_device_with_flags - list network devices with the specified flags
 * @flags: input argument, the network device flags
 * @nr_names: input argument, number of elements in @names
 * @names: output argument, constant pointer to the array of network device names
 *
 * Returns the number of resulted elements in @names for success
 * or negative errno values for failure.
 */
int list_net_device_with_flags(short flags, int nr_names,
				char (* const names)[IFNAMSIZ])
{
	FILE *fh;
	char buf[512];
	struct ifreq ifr;
	int nr_result, skfd, len;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (skfd < 0)
		goto out;

	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh)
		goto out_close_skfd;
	fgets(buf, sizeof(buf), fh);	/* eat line */
	fgets(buf, sizeof(buf), fh);

	nr_result = 0;
	while (fgets(buf, sizeof(buf), fh) && nr_result < nr_names) {
		char name[128];

		get_name(name, buf);
		len = sizeof(ifr.ifr_name);
		memset(ifr.ifr_name, 0, len);
		strncpy(ifr.ifr_name, name, len-1);
		if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
			goto out_close_fh;

		if (ifr.ifr_flags & flags) {
			len = ARRAY_SIZE(names[0]);
			memset(names[nr_result], 0, len);
			strncpy(names[nr_result++], name, len-1);
		}
	}

	if (ferror(fh))
		goto out_close_fh;

	fclose(fh);
	close(skfd);

	return nr_result;

out_close_fh:
	fclose(fh);
out_close_skfd:
	close(skfd);
out:
	warn("%s():%d", __FUNCTION__, __LINE__);

	return -errno;
}

#ifdef CONFIG_RTK_NETIF_EXTRA_STATS

/**
 * get_net_device_ext_stats - get the UC,MC,BCstatistics of the specified network device
 * @ifname: input argument, constant pointer to the network device name
 * @nds: output argument, pointer to network device statistics
 *
 * Returns 0 if not found, 1 if found
 * or negative errno values for failure.
 *
 */
int get_net_device_ext_stats(const char *ifname, struct net_device_stats *nds)
{
	FILE *fh;
	char buf[512];
	int found;

	fh = fopen(_PATH_PROCNET_DEV_EXT, "r");
	if (!fh)
		goto out;
	fgets(buf, sizeof(buf), fh);	/* eat line */
	fgets(buf, sizeof(buf), fh);

	memset(nds, 0, sizeof(*nds));
	found = 0;
	while (fgets(buf, sizeof(buf), fh)) {
		char *s, name[128];
		int n;

		s = get_name(name, buf);

		if (!strcmp(name, ifname)) {
			get_dev_ext_fields(s, nds);
			found = 1;
			break;
		}
	}

	if (ferror(fh))
		goto out_close_fh;

	fclose(fh);

	return found;

out_close_fh:
	fclose(fh);
out:
	warn("%s():%d %s", __FUNCTION__, __LINE__, _PATH_PROCNET_DEV_EXT);

	return -errno;
}
#endif

/**
 * get_net_device_stats - get the statistics of the specified network device
 * @ifname: input argument, constant pointer to the network device name
 * @nds: output argument, pointer to network device statistics
 *
 * Returns 0 if not found, 1 if found
 * or negative errno values for failure.
 *
 */
int get_net_device_stats(const char *ifname, struct net_device_stats *nds)
{
	FILE *fh;
	char buf[512];
	int procnetdev_vsn, found;

	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh)
		goto out;
	fgets(buf, sizeof(buf), fh);	/* eat line */
	fgets(buf, sizeof(buf), fh);

	procnetdev_vsn = procnetdev_version(buf);

	memset(nds, 0, sizeof(*nds));
	found = 0;
	while (fgets(buf, sizeof(buf), fh)) {
		char *s, name[128];
		int n;

		s = get_name(name, buf);

		if (!strcmp(name, ifname)) {
			get_dev_fields(s, nds, procnetdev_vsn);
			found = 1;
			break;
		}
	}

	if (ferror(fh))
		goto out_close_fh;

	fclose(fh);

	return found;

out_close_fh:
	fclose(fh);
out:
	warn("%s():%d %s", __FUNCTION__, __LINE__, _PATH_PROCNET_DEV);

	return -errno;
}

/**
 * ethtool_gstats - get the statistics of the specified network device using ethtool
 * @ifname: input argument, constant pointer to the network device name
 *
 * Returns NULL if an error occurs, non-NULL otherwise
 */
struct ethtool_stats * ethtool_gstats(const char *ifname)
{
	struct ifreq ifr;
	int fd, err;
	struct ethtool_drvinfo drvinfo;
	struct ethtool_stats *stats = NULL;
	unsigned int n_stats;

	/* Setup our control structures. */
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);

	/* Open control socket. */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("Cannot get control socket");
		goto out;
	}

	drvinfo.cmd = ETHTOOL_GDRVINFO;
	ifr.ifr_data = (caddr_t)&drvinfo;
	err = ioctl(fd, SIOCETHTOOL, &ifr);
	if (err < 0) {
		perror("Cannot get driver information");
		goto out_close_fd;
	}

	n_stats = drvinfo.n_stats;
	if (n_stats < 1) {
		fprintf(stderr, "no stats available\n");
		goto out_close_fd;
	}

	stats = calloc(1, n_stats * sizeof(uint64_t) + sizeof(struct ethtool_stats));
	if (!stats) {
		fprintf(stderr, "no memory available\n");
		goto out_close_fd;
	}

	memset(stats, 0, sizeof(stats));
	stats->cmd = ETHTOOL_GSTATS;
	stats->n_stats = n_stats;
	ifr.ifr_data = (caddr_t)stats;
	err = ioctl(fd, SIOCETHTOOL, &ifr);
	if (err < 0) {
		perror("Cannot get stats information");
		free(stats);
		stats = NULL;
		goto out_close_fd;
	}

out_close_fd:
	close(fd);
out:
	return stats;
}

// Kaohj
/*
 *	Get the link status about device.
 *	Return:
 *		-1 on error
 *		0 on link down
 *		1 on link up
 */
int get_net_link_status(const char *ifname)
{
	FILE *fp;
	char buff[32];
	int num;
	int ret;
#ifdef CONFIG_DEV_xDSL
	Modem_LinkSpeed vLs;
#endif

#ifdef CONFIG_DEV_xDSL
	if (!strcmp(ifname, ALIASNAME_DSL0)) {
       		if ( adsl_drv_get(RLCM_GET_LINK_SPEED, (void *)&vLs,
			RLCM_GET_LINK_SPEED_SIZE) && vLs.upstreamRate != 0)
			ret = 1;
		else
			ret = 0;
		return ret;
	}
#endif
#ifdef WLAN_WISP
	bss_info pBss;
	if(!strncmp(ifname, "wlan", 4)){
		getWlBssInfo(ifname, &pBss);
		if (pBss.state == STATE_CONNECTED)
			return 1;
		else
			return 0;
	}
#endif
	if(!strcmp(ifname, ALIASNAME_NAS0)){
		struct ifreq ifr;
		struct ethtool_value edata;
		ifr.ifr_name[sizeof(ifr.ifr_name)-1]='\0';
		strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);
		edata.cmd = ETHTOOL_GLINK;
		ifr.ifr_data = (caddr_t)&edata;

		ret = do_ioctl(SIOCETHTOOL, &ifr);
		if (ret == 0)
			ret = edata.data;
		return ret;
	}

	sprintf(buff, "/sys/class/net/%s/carrier", ifname);
	if (!(fp = fopen(buff, "r"))) {
		printf("Error: cannot open %s\n", buff);
		return -1;
	}
	num = fscanf(fp, "%d", &ret);
	fclose(fp);
	if (num <= 0)
		return -1;

	return ret;
}

/*
 *	Get the link information about device.
 *	Return:
 *		-1 on error
 *		0 on success
 */
int get_net_link_info(const char *ifname, struct net_link_info *info)
{
	struct ifreq ifr;
	struct ethtool_cmd ecmd;
	int ret;

	memset(info, 0, sizeof(struct net_link_info));
	ifr.ifr_name[sizeof(ifr.ifr_name)-1]='\0';
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);
	ecmd.cmd = ETHTOOL_GSET;
	ifr.ifr_data = (caddr_t)&ecmd;

	ret = do_ioctl(SIOCETHTOOL, &ifr);
	if (ret == 0) {
		info->supported = ecmd.supported; // ports, link modes, auto-negotiation
		info->advertising = ecmd.advertising; // link modes, pause frame use, auto-negotiation
		info->speed = ecmd.speed; // 10Mb, 100Mb, gigabit
		info->duplex = ecmd.duplex; // Half, Full, Unknown
		info->phy_address = ecmd.phy_address;
		info->transceiver = ecmd.transceiver;
		info->autoneg = ecmd.autoneg;
	}
	return ret;
}

/*
 *	ifname:
 *		"nas0", "ptm0", "eth0.2", "eth0.3" ...
 *	Return:
 *		>=	0 shapingRate
 *		-1	on error
 */
int get_dev_ShapingRate(const char *ifname)
{
	int ret=-1;
#if CONFIG_HWNAT
	ret = hwnat_itf_get_ShapingRate(ifname);
#endif
	return ret;
}

/*
 *	ifname:
 *		"nas0", "ptm0", "eth0.2", "eth0.3" ...
 *	Return:
 *		>=	0 shapingRate
 *		-1	on error
 */
int get_dev_ifindex(const char *ifname)
{
	int ret=-1;

	FILE *fp;
	char buff[64];
	int num;

	sprintf(buff, "/sys/class/net/%s/ifindex", ifname);
	if (!(fp = fopen(buff, "r"))) {
		printf("Error: cannot open %s\n", buff);
		return -1;
	}
	num = fscanf(fp, "%d", &ret);
	fclose(fp);
	if (num <= 0)
		return -1;

	return ret;
}

/*
 *	ifname:
 *		"nas0", "ptm0", "eth0.2", "eth0.3" ...
 *	Rate:
 *		-1	no shaping
 *		>=0	rate in bps
 *	Return:
 *		>= 0 on success
 *		-1 on error
 */
int set_dev_ShapingRate(const char *ifname, int rate)
{
	int ret=-1;
#if CONFIG_HWNAT
	ret = hwnat_itf_set_ShapingRate(ifname, rate);
#endif
	return ret;
}

/*
 *	ifname:
 *		"nas0", "ptm0", "eth0.2", "eth0.3" ...
 *	Return:
 *		>=0	Burst size in bytes
 *		-1	on error
 */
int get_dev_ShapingBurstSize(const char *ifname)
{
	int ret=-1;
#if CONFIG_HWNAT
	ret = hwnat_itf_get_ShapingBurstSize(ifname);
#endif
	return ret;
}

/*
 *	ifname:
 *		"nas0", "ptm0", "eth0.2", "eth0.3" ...
 *	bsize:
 		Burst size in bytes
 *	Return:
 *		>=0	on success
 *		-1	on error
 */
int set_dev_ShapingBurstSize(const char *ifname, unsigned int bsize)
{
	int ret=-1;
#if CONFIG_HWNAT
	ret = hwnat_itf_set_ShapingBurstSize(ifname, bsize);
#endif
	return ret;
}

// MEM_UTILITY, CPU_UTILITY
enum enum_mem_info {
	DEV_MEM_TOTAL,
	DEV_MEM_FREE,
	DEV_MEM_USAGE_RATIO
};

struct dev_stats {
        unsigned int    user;    // user (application) usage
        unsigned int    nice;    // user usage with "niced" priority
        unsigned int    system;  // system (kernel) level usage
        unsigned int    idle;    // CPU idle and no disk I/O outstanding
        unsigned int    iowait;  // CPU idle but with outstanding disk I/O
        unsigned int    irq;     // Interrupt requests
        unsigned int    softirq; // Soft interrupt requests
        unsigned int    steal;   // Invol wait, hypervisor svcing other virtual CPU
        unsigned int    total;
};

int getdata(struct dev_stats *st)
{
	FILE *fp = NULL;

	if( (fp = fopen("/proc/stat","r")) == NULL)
	{
		printf("%s: getCpuTotal Failed to open file\n", __FUNCTION__);
		return -1;
	}

	if(fscanf(fp, "cpu %u %u %u %u %u %u %u %u",
		&st->user, &st->nice, &st->system, &st->idle,
		&st->iowait, &st->irq, &st->softirq, &st->steal) != 8)
		printf("fscanf failed: %s %d\n", __func__, __LINE__);

	st->total = st->user + st->nice  + st->system + st->idle + st->iowait +
		st->irq  + st->steal + st->softirq;

	fclose(fp);
	return(0);
}

static pthread_t cpu_usage_thread = 0;
static unsigned int  cpuUsage = 0;

static void *  __cpu_usage_thread_func(void *arg)
{
    pthread_detach(pthread_self());

	sigset_t mask;
    int rc;
	sigfillset(&mask);
    rc = pthread_sigmask(SIG_BLOCK, &mask, NULL);
    if (rc != 0) {
        fprintf(stderr, "[%s(%d)]: %d, %s\n", __FILE__, __LINE__, rc, strerror(rc));
		pthread_exit(NULL);
    }

   	struct dev_stats    st, stold;
	unsigned int    curtotal, curidle, curusage;
	unsigned int    usepercent;

	while(1)
	{
	    getdata(&stold);
		sleep(1);
		getdata(&st);

		curtotal = st.total - stold.total;
		curidle = st.idle - stold.idle;
		curusage = curtotal - curidle;

		if (curusage > 0 && curtotal > 0){
			usepercent = curusage * 100 / curtotal;
		}
		else{
			usepercent = 1;
		}
		if(usepercent == 0)
			cpuUsage = 1;
		else
		{
			cpuUsage = usepercent;
		}


	}
    pthread_exit(NULL);
}

int create_cpu_usage_thread(void)
{
    int ret = 0;
    /* start the polling thread */
    if ((cpu_usage_thread == 0)
        || pthread_kill(cpu_usage_thread, 0) == ESRCH) {

        ret = pthread_create(&cpu_usage_thread, NULL,
                           &__cpu_usage_thread_func, NULL);
        if (ret != 0) {
            fprintf(stderr, "create_cpu_usage_thread thead initial fail!\n");
            return -1;
        }
        /*Wait for cpu thread created*/
        sleep(2);
    }
    return 0;
}

int get_cpu_usage(void)
{
	unsigned int usepercent = 0;
	if(create_cpu_usage_thread() < 0){
		return 1;
	}

	usepercent = cpuUsage;
	return usepercent;
}

static int get_mem_info(int info)
{
	FILE* fs = NULL;
	char line[128];
	int total = 0, free = 0, rate = 0, buffers = 0, cache = 0, use = 0;
	char *pToken = NULL,*pLast;
	int ret = 0, count = 0;
	fs = fopen( "/proc/meminfo" , "r");
	if ( fs == NULL )
		return 0;
	else
	{
		while (count < 5)
		{
			fgets(line, 128, fs);
			strtok_r(line, ":", &pLast);
			if (0 == strcasecmp(line, "MemTotal")) {
				total = atoi(pLast);
				count++;
			} else if (0 == strcasecmp(line, "MemFree")) {
				free = atoi(pLast);
				count++;
			} else if (0 == strcasecmp(line, "Buffers")) {
				buffers = atoi(pLast);
				count++;
			} else if (0 == strcasecmp(line, "Cached")) {
				cache = atoi(pLast);
				count++;
			} else if (0 == strcasecmp(line, "MemAvailable")) {
				count++;
			}
		}

		switch(info) {
		case DEV_MEM_TOTAL:
			ret = total;
			break;
		case DEV_MEM_FREE:
			ret = free + buffers + cache;
			break;
		case DEV_MEM_USAGE_RATIO:
#ifdef USE_DEONET /* sghong, 20231130 */
			ret = (total > 0) ? (100*(total-free))/total : 0;
#else
			ret = (total > 0) ? (100*(total-buffers-cache-free))/total : 0;
#endif
		}

		fclose(fs);
		return (int)(ret);
	}
}

int get_mem_free(void)
{
	int m = get_mem_info(DEV_MEM_FREE);
	return m;
}

int get_mem_total(void)
{
	int m = get_mem_info(DEV_MEM_TOTAL);
	return m;
}

int get_mem_usage(void)
{
	int m = get_mem_info(DEV_MEM_USAGE_RATIO);
	return m;
}
int get_flash_writable(void)
{
	char fs[20] = {0};
    int kblocks = 0;
    int used = 0;
    int avail = 0;
    int use_percent = 0;
    char mount_on[20] = {0};
    char line[128] = {0};
    int total=0, use_now=0;
    FILE *fp = NULL;

    system("df >/tmp/df_info");
    fp = fopen("/tmp/df_info", "r");
    if(fp == NULL)
    {
        printf("get flash writable fopen error\n");
        return 0;
    }

    while(fgets(line,sizeof(line),fp) != NULL)
    {
        if(sscanf(line,"%s %d %d %d %d%% %s\n",fs, &kblocks, &used, &avail, &use_percent,mount_on) != 6)
            continue;
        if(used == 0 || avail == 0) //non-writable or memory
            continue;
        use_now += used;
        total += kblocks;
    }
    fclose(fp);
    unlink("/tmp/df_info");
    if(total && use_now)
    {
        use_percent = use_now*100/total;
        return use_percent;
    }
	return 0;
}
#ifdef AUTO_SAVE_ALARM_CODE
int refresh_auto_save_alarm_code_file(void)
{
    FILE *fpr = NULL;
    FILE *fpw = NULL;
    char buff[128] = {0};
    char info[64] = {0};
    int code = 0;
    int time = 0;
	int ret = 0;
    struct timeval tv = {0};
    gettimeofday( &tv, NULL);
    rename(AUTOSAVE_ALARMCODE_FILE, AUTOSAVE_ALARMCODE_FILE_TMP);
    fpr = fopen(AUTOSAVE_ALARMCODE_FILE_TMP, "r");
    if(fpr == NULL)
    {
        printf("%s: fpr open error\n",__FUNCTION__);
        return 0;
    }
     fpw = fopen(AUTOSAVE_ALARMCODE_FILE, "a+");
    if(fpw == NULL)
    {
        printf("%s: fpw open error\n",__FUNCTION__);
        fclose(fpr);
        return 0;
    }
    while (fgets(buff, sizeof(buff), fpr) != NULL) {
	    ret = sscanf(buff, "%d %d %s", &time, &code, info);
		if(ret != 2 && ret != 3)
	    {
            printf("refresh sscanf %s error\n ",buff);
            continue;
        }
        printf(" code:%d,time:%d,info:%s\n",code,time,info);
        if(tv.tv_sec - time < 600)
        {
            fputs(buff, fpw);
        }
    }
    fclose(fpr);
    fclose(fpw);
    return 0;
}

/*Avoid recording the same alarm multi times. */
int is_alarm_info_already_exist(char * alarm_info)
{
    FILE *fpr = NULL;
	char buff[128] = {0};

    fpr = fopen(AUTOSAVE_ALARMCODE_FILE, "r");
    if(fpr == NULL)
    {
        printf("%s: fpr open error\n",__FUNCTION__);
        return 0;
    }

    while (fgets(buff, sizeof(buff), fpr) != NULL) {
	    if(!strncmp(buff,alarm_info,strlen(alarm_info)))
	    {
            fclose(fpr);
			return 1;
        }
    }
    fclose(fpr);
    return 0;
}

int auto_save_alarm_code_add(int code , char *info)
{
	char alarm_info[128] = {0};
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
	/*Do not record port alarm within 60 seconds after startup. */
	if(code == CTCOM_ALARM_PORT_FAILED && tv.tv_sec < 60)
		return 0;
    int lockfd = 0;
    if((lockfd = lock_file_by_flock(AUTOSAVE_ALARMCODE_LOCK, 1)) == -1)
	{
		printf("lock file %s fail\n", AUTOSAVE_ALARMCODE_LOCK);
		return -1;
	}
    refresh_auto_save_alarm_code_file();
	sprintf(alarm_info, "%d %d %s\n",tv.tv_sec,code, info);
	if(is_alarm_info_already_exist(alarm_info) == 0)
	{
    FILE *fp = fopen(AUTOSAVE_ALARMCODE_FILE, "a+");
    if(fp == NULL)
    {
        unlock_file_by_flock(lockfd);
        return -1;
    }
	    fprintf(fp, alarm_info);
    fclose(fp);
	}
    unlock_file_by_flock(lockfd);
    return 0;
}
#endif
int GetWanMode(void)
{
	unsigned int wanmode = 0;

	if(!mib_get_s(MIB_WAN_MODE, (void *)&wanmode, sizeof(wanmode)))
		fprintf(stderr, "Get mib value MIB_WAN_MODE failed!\n");

	return (wanmode & WAN_MODE_MASK);
}

int isInterfaceMatch(unsigned int ifindex)
{
	if(WAN_MODE & MODE_ATM && MEDIA_ATM == MEDIA_INDEX(ifindex))
		return 1;
	else if(WAN_MODE & MODE_Ethernet && MEDIA_ETH == MEDIA_INDEX(ifindex))
		return 1;
	else if(WAN_MODE & MODE_PTM && MEDIA_PTM == MEDIA_INDEX(ifindex))
		return 1;

#ifdef WLAN_WISP
	else if(WAN_MODE & MODE_Wlan && MEDIA_WLAN == MEDIA_INDEX(ifindex))
		return 1;
#endif
	return 0;
}

static int _isDefaultRouteWan(MIB_CE_ATM_VC_Tp pEntry, unsigned int ipv)
{
#ifdef DEFAULT_GATEWAY_V2
	unsigned int dgw;
	if (mib_get_s(MIB_ADSL_WAN_DGW_ITF, (void *)&dgw, sizeof(dgw)) != 0)
	{
		if ((dgw == pEntry->ifIndex)
		#ifdef AUTO_PPPOE_ROUTE
		 || (dgw == DGW_AUTO)
		#endif
		)
			return 1;
	}
#else
#if defined(CONFIG_USER_RTK_WAN_CTYPE) && (!defined(CONFIG_USER_SELECT_DEFAULT_GW_MANUALLY))

#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	unsigned char load_balance_enable = 0;
	mib_get_s(MIB_GLOBAL_LOAD_BALANCE, (void*)&load_balance_enable, sizeof(load_balance_enable));

	if (load_balance_enable && pEntry->balance_type == 0)
		return 0;
#endif

	if(pEntry->cmode != CHANNEL_MODE_BRIDGE)
	{
#if defined(CONFIG_SUPPORT_HQOS_APPLICATIONTYPE)
		if((pEntry->applicationtype & (X_CT_SRV_INTERNET)) && pEntry->othertype != OTHER_HQOS_TYPE)
#else
		if((pEntry->applicationtype & (X_CT_SRV_INTERNET)))
#endif
			return 1;
	}
#endif
	if((pEntry->dgw & ipv) && pEntry->cmode != CHANNEL_MODE_BRIDGE)
		return 1;
	if(pEntry->cmode == CHANNEL_MODE_BRIDGE)
		return 1;
#endif
	return 0;
}

int isDefaultRouteWan(MIB_CE_ATM_VC_Tp pEntry)
{
	return _isDefaultRouteWan(pEntry, IPVER_IPV4_IPV6);
}

int isDefaultRouteWanV4(MIB_CE_ATM_VC_Tp pEntry)
{
#if defined(CONFIG_E8B)
	if((pEntry->dgw & IPVER_IPV4) && pEntry->cmode != CHANNEL_MODE_BRIDGE)
		return 1;
	else
		return 0;
#else
	return _isDefaultRouteWan(pEntry, IPVER_IPV4);
#endif
}

int isDefaultRouteWanV6(MIB_CE_ATM_VC_Tp pEntry)
{
#if defined(CONFIG_E8B)
	if((pEntry->dgw & IPVER_IPV6) && pEntry->cmode != CHANNEL_MODE_BRIDGE)
		return 1;
	else
		return 0;
#else
	return _isDefaultRouteWan(pEntry, IPVER_IPV6);
#endif
}

unsigned int getInternetIPv4WANIfindex(void)
{
	int totalNum=0;
	int i,ret=0;
	MIB_CE_ATM_VC_T Entry={0};

	totalNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			continue;
		}

		if((Entry.enable == 1)
		#ifdef CONFIG_IPV6
			&& (Entry.IpProtocol & IPVER_IPV4)
		#endif
			&& isDefaultRouteWan(&Entry))
		{
			return Entry.ifIndex;
		}
	}
	return DUMMY_IFINDEX;
}

unsigned int getDefaultGWIPv6WANIfindex(void)
{
	int totalNum=0;
	int i,ret=0;
	MIB_CE_ATM_VC_T Entry={0};

	totalNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			continue;
		}

		if((Entry.enable == 1)
#ifdef CONFIG_IPV6
			&& (Entry.IpProtocol & IPVER_IPV6)
#endif
			&& isDefaultRouteWanV6(&Entry))
		{
			return Entry.ifIndex;
		}
	}
	return DUMMY_IFINDEX;
}


#define ARP "/proc/net/arp"
int get_mac_from_IP(char *MAC, char *remote_IP){
		FILE *fd = NULL;
		char tmp[81];
		int len = 81;
		int i;
		char *str;
		int amtRead;
		char * trim_remote_IP;

#ifdef IPV6
		// The address notation for IPv4-mapped-on-IPv6 is ::FFFF:<IPv4 address>.
		//	So we shift 7 bytes (::FFFF:).
		trim_remote_IP = remote_IP+7;
#else
		trim_remote_IP = remote_IP;
#endif
		if((fd = fopen(ARP, "r")) == 0){/*should be able to open the ARP cache*/
			return 0;
		}
		while(fgets(tmp, len, fd)){
			/*MN - should do some more checks!!*/
			str = strtok(tmp, " \t");
			if(strcmp(str, trim_remote_IP) == 0){ /*found IP address*/
				for(i=0;i<3;i++){  /*Move along until the HW address*/
					str = strtok(NULL, " \t");
				}
				strcpy(MAC, str);
				fclose(fd);
				/*MATT2 you could obtain the ethernet device in here if you wanted*/
				return 1;
			}
		}
		fclose(fd);
		return 0;
	}
#ifdef CONFIG_USER_MINIUPNPD
#ifdef CONFIG_00R0
const char miniupnpd_chain[] = "MINIUPNPD";
#endif
unsigned int get_upnp_extif(char *ifname, int sz)
{
	unsigned int upnp_ifidx;

	/* get upnp extif by mib */
	mib_get_s(MIB_UPNP_EXT_ITF, (void *)&upnp_ifidx, sizeof(upnp_ifidx));
	if (upnp_ifidx != DUMMY_IFINDEX) {
		ifGetName(upnp_ifidx, ifname, sz);
		return 0;
	}

	/* If not specify upnp extif in mib, find one with Internet access. */
	upnp_ifidx = getInternetIPv4WANIfindex();
	if (upnp_ifidx == DUMMY_IFINDEX)
		strcpy(ifname, "ppp0");
	else
		ifGetName(upnp_ifidx, ifname, sz);
	return 0;
}

static int start_upnp(int configAll, int wan_IfIndex)
{
	char ifname[IFNAMSIZ];

	if(wan_IfIndex)
		ifGetName(wan_IfIndex, ifname, sizeof(ifname));
	else
		get_upnp_extif(ifname, sizeof(ifname));

	printf("[%s:%d] Start miniupnpd! (wanIf=%s)\n",__FUNCTION__,__LINE__,ifname);
	va_niced_cmd("/bin/upnpctrl", 3, 1, "up", ifname, BRIF);  // restart UPnP daemon

	return 0;
}

static int stop_upnp(int disable_upnp, int wan_IfIndex)
{
	char ifname[IFNAMSIZ];
	FILE *fp;
	unsigned char upnpdEnable;
#ifdef CONFIG_00R0
	MIB_CE_PORT_FW_T Entry;
	unsigned int i, entryNum;
#endif

	if(wan_IfIndex)
		ifGetName(wan_IfIndex, ifname, sizeof(ifname));
	else
	{
		fp = fopen("/var/upnp_extif", "r");
		if (fp) {
			if(fscanf(fp, "%s", ifname) != 1)
				printf("fscanf failed: %s %d\n", __func__, __LINE__);
			fclose(fp);
		}
		else
			get_upnp_extif(ifname, sizeof(ifname));
	}

	printf("[%s:%d] Stop miniupnpd! (wanIf=%s)\n",__FUNCTION__,__LINE__,ifname);
	va_niced_cmd("/bin/upnpctrl", 3, 1, "down", ifname, BRIF);

#ifdef CONFIG_00R0
	entryNum = mib_chain_total(MIB_PORT_FW_TBL);
	i = entryNum;
	while(i--)
	{
		if (mib_chain_get(MIB_PORT_FW_TBL, i, (void *)&Entry))
			if (Entry.dynamic==1)
				mib_chain_delete(MIB_PORT_FW_TBL, i);
	}
	va_cmd(IPTABLES, 2, 1, "-F", miniupnpd_chain);
#endif

	if(disable_upnp){
		upnpdEnable = 0;
		mib_set(MIB_UPNP_DAEMON,(void *)&upnpdEnable);
	}

	return 0;
}

/*
configAll: CONFIGALL or CONFIGONE, CONFIGALL will not care wan_enable and wan_IfIndex.
wan_enable: to decide UPnP enable/disable.
wan_IfIndex: check specific wan index.
disable_upnp: when disable UPnP, set mib disable or not.
*/
int restart_upnp(int configAll, int wan_enable, int wan_IfIndex, int disable_upnp)
{
	unsigned char is_enabled;
	char ifname[IFNAMSIZ];
	unsigned int upnpItf;

	if(!mib_get_s(MIB_UPNP_DAEMON, (void *)&is_enabled, sizeof(is_enabled)))
	{
		printf("[%s:%d] get MIB_UPNP_DAEMON error!\n", __FUNCTION__, __LINE__);
		return 0;
	}
	if(!mib_get_s(MIB_UPNP_EXT_ITF, (void *)&upnpItf, sizeof(upnpItf)))
	{
		printf("[%s:%d] get MIB_UPNP_EXT_ITF error!\n", __FUNCTION__, __LINE__);
		return 0;
	}


	if(configAll == CONFIGALL)//restart, do not care paremeter wan_enable, wan_IfIndex
	{
		stop_upnp(disable_upnp, 0);/* upnp disabled */

		if (is_enabled) { /* upnp enabled */
			/* start upnp */
			start_upnp(configAll, 0);
		}
	}
	else if(configAll == CONFIGONE && wan_IfIndex == upnpItf)
	{
		if(wan_enable && is_enabled)
		{
			va_niced_cmd("/bin/upnpctrl", 1, 1, "sync");
			start_upnp(configAll, wan_IfIndex);
		}
		else
			stop_upnp(disable_upnp, wan_IfIndex);/* upnp disabled in specific wan*/
	}
	else
		printf("[%s:%d] miniupnpd parameter error! (parameter=%d, %d, %d, %d, upnpItf=%d)\n",
			__FUNCTION__, __LINE__, configAll, wan_enable, wan_IfIndex, disable_upnp, upnpItf);

	return 0;
}

void do_upnp_sync(char *wan_name)
{
	unsigned char is_enabled;
	char upnp_ifname[IFNAMSIZ] = {0};

	if (wan_name == NULL)
		return;

	if (!mib_get_s(MIB_UPNP_DAEMON, (void *)&is_enabled, sizeof(is_enabled))) {
		printf("[%s:%d] get MIB_UPNP_DAEMON error!\n", __FUNCTION__, __LINE__);
		return;
	}
	if (is_enabled == 0)
		return;

	get_upnp_extif(upnp_ifname, sizeof(upnp_ifname));
	if (strcmp(wan_name, upnp_ifname) != 0)
		return;

	va_niced_cmd("/bin/upnpctrl", 1, 0, "sync");
}
#endif

#ifdef CONFIG_E8B
/*
 * flag
 * 0: for short reset
 * 1: for long reset
 * 2: TR-069 FactoryReset
 * 3: reset button for short reset
 * 4: reset button for long reset
 * 5: factory default : flash default cs
 * 7: reset button for CU liaoning long reset
 */
int reset_cs_to_default(int flag)
{
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	if(flag == 5)
		update_restore_status(7);
	else
		update_restore_status(flag);
#if defined(YUEME_4_0_SPEC)
	dbus_do_ClearExtraMIB();
#endif
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	clean_filesystem();
#endif

#ifdef CONFIG_USER_CWMP_TR069
	unlink("/var/config/CWMPNotify.txt");
#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT
	if(flag>0){
		if(opendir("/var/config/myca") != NULL)
			system("rm -rf /var/config/myca");
	}
#endif
#ifdef CU_APP_SCHEDULE_LOG
	unlink("/var/config/syslog_upload_path");
	unlink("/var/config/syslog_schedule_upload");
#endif

#ifdef CONFIG_USER_CTCAPD
	system("rm /ctcap/* -rf > /dev/null 2>&1");
	unlink("/var/app/lan_device_info");
#endif

#ifdef RESERVE_KEY_SETTING
#if defined(CONFIG_CU_BASEON_CMCC)
	if(flag==3 || flag==4 || flag==7)
#else
	if(flag==3 || flag==4)
#endif
		flag -= 3;
	else if(flag==5)
		flag = 3;
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	else if(flag==6) // dbus LocalRecovery
		flag = 0;
#ifdef CONFIG_CU_BASEON_YUEME
	else if(flag==8) // dbus restore
		flag = 0;
#endif
#endif

#ifdef CONFIG_CU_BASEON_YUEME
	if(flag==1){
		//load factory default as "flash default cs"
		reserve_critical_setting(3);
	}
	else{
		reserve_critical_setting(flag);
	}
	//restore framework configuration
	system("dbus-send --system --print-reply --dest=com.cuc.ufw1 /com/cuc/ufw1 com.cuc.ufw1.framework.Restore");
#ifdef CONFIG_CU_DPI
	if(flag == 1 || flag == 3)
		system("rm -rf /var/dpi/upper");
#endif
#else
	reserve_critical_setting(flag);
#endif
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	printf("do wait flash sync\n");

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	while(1){
		clock_gettime(CLOCK_MONOTONIC, &end);
		//printf("wait for a while\n");
		if((end.tv_sec - start.tv_sec)>=2)
			break;
		sleep(1);
	}
	return 1;
#else
	return mib_flash_to_default(CURRENT_SETTING);
#endif //RESERVE_KEY_SETTING
}
#else
/*
 * flag: reserved for future use
 */
int reset_cs_to_default(int flag)
{
#ifdef CONFIG_USER_CWMP_TR069
	unlink("/var/config/CWMPNotify.txt");
#endif

#ifdef RESERVE_KEY_SETTING
	reserve_critical_setting(flag);
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
#else
	return mib_flash_to_default(CURRENT_SETTING);
#endif
	return 1;
}
#endif //CONFIG_E8B

#ifdef CONFIG_TR_064
int GetTR064Status(void)
{
	unsigned char tr064_st = 0;

	if(!mib_get_s(MIB_TR064_ENABLED, (void *)&tr064_st, sizeof(tr064_st)))
		fprintf(stderr, "Get mib value MIB_TR064_ENABLED failed!\n");

	return tr064_st;
}
#endif

// Mason Yu. Support ddns status file.
#ifdef CONFIG_USER_DDNS
int stop_all_ddns()
{
	unsigned int entryNum, i;
	MIB_CE_DDNS_T Entry;
	char pidName[128];
	int pid = 0;

	entryNum = mib_chain_total(MIB_DDNS_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_DDNS_TBL, i, (void *)&Entry))
		{
			printf("stop_all_ddns:Get chain record error!\n");
			continue;
		}

		snprintf(pidName, 128, "%s.%s.%s.%s", (char *)DDNSC_PID, Entry.provider, Entry.interface, Entry.hostname);
		pid = read_pid((char *)pidName);
		if (pid > 0) {
			kill(pid, SIGKILL);
			unlink(pidName);
		}
	}
	return 0;
}

int restart_ddns(void)
{
	// Mason Yu.  create DDNS thread dynamically
	//printf("restart_ddns\n");
	// Mason Yu. Specify IP Address
#ifdef CONFIG_IPV6
	va_niced_cmd("/bin/updateddctrl", 2, 1, "all", "3");
#else
	va_niced_cmd("/bin/updateddctrl", 2, 1, "all", "1");
#endif

	return 0;
}

void remove_ddns_status(void)
{
	unsigned int entryNum, i;
	MIB_CE_DDNS_T Entry;
	FILE *fp;
	unsigned char filename[100];

	entryNum = mib_chain_total(MIB_DDNS_TBL);
	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_DDNS_TBL, i, (void *)&Entry))
		{
  			printf("remove_ddns_status:Get chain record error!\n");
			continue;
		}

		// Check all variables that updatedd need
		if ( strlen(Entry.username) == 0 ) {
			printf("remove_ddns_status: username/email is NULL!!\n");
			continue;
		}

		if ( strlen(Entry.password) == 0 ) {
			printf("remove_ddns_status: password/key is NULL!!\n");
			continue;
		}

		if ( strlen(Entry.hostname) == 0 ) {
			printf("remove_ddns_status: Hostname is NULL!!\n");
			continue;
		}

		if ( strlen(Entry.interface) == 0 ) {
			printf("remove_ddns_status: Interface is NULL!!\n");
			continue;
		}

		if ( Entry.Enabled != 1 ) {
			printf("remove_ddns_status: The account is disabled!!\n");
			continue;
		}

		if ( strcmp(Entry.provider, "dyndns") == 0 || strcmp(Entry.provider, "tzo") == 0 || strcmp(Entry.provider, "noip") == 0) {
			// open a status file
			sprintf(filename, "/var/%s.txt", Entry.hostname);
			if((fp=fopen(filename,"r")) ==NULL)
				continue;
			fclose(fp);
			unlink(filename);
		}else {
			//sprintf(account, "%s:%s", Entry.email, Entry.key);
			printf("remove_ddns_status: Not support this provider\n");
			syslog(LOG_INFO, "remove_ddns_status: Not support this provider %s\n", Entry.provider);
			continue;
		}
	}
}
#endif

int check_vlan_reserved(int vid)
{
	if((vid == 0) || (vid >= RESERVED_VID_START && vid <= RESERVED_VID_END))
		return 1;

	return 0;
}

#ifdef CONFIG_USER_VLAN_ON_LAN
int setup_VLANonLAN(int mode)
{
	char ethVlan[24], vidStr[8];
	int i, status = 0;
	unsigned short vid;
	MIB_CE_SW_PORT_T sw_entry;

	if (mode == ADD_RULE) {
		status |= va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N",
				 (char *)BR_VLAN_ON_LAN);
		status |= va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P",
				 (char *)BR_VLAN_ON_LAN, (char *)FW_RETURN);
		status |= va_cmd(EBTABLES, 6, 1, "-t", "broute", (char *)FW_ADD, "BROUTING",
			   "-j", (char *)BR_VLAN_ON_LAN);
	} else if (mode == DEL_RULE) {
		status |= va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F",
				 (char *)BR_VLAN_ON_LAN);
		status |= va_cmd(EBTABLES, 6, 1, "-t", "broute", "-D", "BROUTING",
			   "-j", (char *)BR_VLAN_ON_LAN);
		status |= va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X",
			   (char *)BR_VLAN_ON_LAN);
	}

	for (i = 0; i < ELANVIF_NUM; i++) {
		if (!mib_chain_get(MIB_SW_PORT_TBL, i, &sw_entry))
			return -1;
		if (!sw_entry.vlan_on_lan_enabled)
			continue;
		snprintf(vidStr, sizeof(vidStr), "%u", sw_entry.vid);
		snprintf(ethVlan, sizeof(ethVlan), "%s.%u", ELANVIF[i],
			 sw_entry.vid);

		if (mode == ADD_RULE) {
			// (1) use vconfig to config vlan
			// vconfig add eth0.2 3
			status |= va_cmd("/bin/vconfig", 3, 1, "add",
				   ELANVIF[i], vidStr);

			// (2) use ifconfig to up interface
			// ifconfig eth0.2.3 up
			status |= va_cmd(IFCONFIG, 2, 1, (char *)ethVlan, "up");

			// (3) use brctl to add eth0.2.3 into br0 bridge
			// brctl addif br0 eth0.2.3
			status |= va_cmd(BRCTL, 3, 1, "addif", (char *)BRIF, ethVlan);

			// (4) set drop rule on BROUTING, then tag packet can go to bridge WAN via eth0.2.3
			// ebtables -t broute -A vlan_on_lan -i eth0.2 -p 0x8100 --vlan-id 3 -j DROP
			status |= va_cmd(EBTABLES, 12, 1, "-t", "broute",
				   (char *)FW_ADD, (char *)BR_VLAN_ON_LAN, "-i",
				   ELANVIF[i], "-p", "0x8100",
				   "--vlan-id", vidStr, "-j", "DROP");
		} else if (mode == DEL_RULE) {
			// (1) use brctl to del eth0.2.3 from br0 bridge
			// brctl delif br0 eth0.2.3
			status |= va_cmd(BRCTL, 3, 1, "delif", (char *)BRIF, ethVlan);

			// (2) use vconfig to remove vlan
			// vconfig rem eth0.2.3
			status |= va_cmd("/bin/vconfig", 2, 1, "rem", ethVlan);
		}
	}
	return status;
}
#endif

#ifdef CONFIG_USER_RTK_LBD
void setupLBD(void)
{
	unsigned char enable = 0;
	MIB_CE_LBD_VLAN_T loopdetect_vlan;
	int totalvlan;
	int i,j;
	unsigned short ether_type=0x9000;
	char cmd[512]={0};
#ifdef CONFIG_EPON_FEATURE
	int loopdetect_period=10;
	int loopdrecover_time=300;
	unsigned int pon_mode=0;
#endif
	int pid;
#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_LUNA_G3_SERIES)
	char remapping_cmd[256]={0};
	unsigned char re_map_tbl[MAX_LAN_PORT_NUM];
#endif

	mib_get_s(MIB_LBD_ENABLE, &enable, sizeof(enable));
#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_LUNA_G3_SERIES)
	//echo port remapping to loopback detect in kernel (br.c)
	if(mib_get_s(MIB_PORT_REMAPPING, (void *)re_map_tbl, sizeof(re_map_tbl)))
	{
		snprintf(remapping_cmd, sizeof(remapping_cmd), "echo \"%d %d %d %d\" > /proc/sys/loopback_detect/lan_port_remapping",
			re_map_tbl[0], re_map_tbl[1], re_map_tbl[2], re_map_tbl[3] );
		printf("%s\n", remapping_cmd);
		system(remapping_cmd);
	}
#endif

#ifdef CONFIG_RTL_SMUX_DEV
	totalvlan=mib_chain_total(MIB_LBD_VLAN_TBL);
	for(j = 0 ; j < ELANVIF_NUM ; j++)
	{
		char rule_alias_name[64];
		sprintf(rule_alias_name, "%s-rx-loopdetect+", ELANRIF[j]);
		va_cmd(SMUXCTL, 7, 1, "--if", ELANRIF[j], "--rx", "--tags", "1", "--rule-remove-alias", rule_alias_name);
		if(enable)
		{
			for(i=0; i<totalvlan; i++)
			{
				if(mib_chain_get(MIB_LBD_VLAN_TBL,i,&loopdetect_vlan))
				{
					char vid_str[16];
					if(loopdetect_vlan.vid > 0)
					{
						sprintf(vid_str, "%d", loopdetect_vlan.vid);
						sprintf(rule_alias_name, "%s-rx-loopdetect-%d", ELANRIF[j], loopdetect_vlan.vid);

						va_cmd(SMUXCTL, 13, 1, "--if", ELANRIF[j], "--rx", "--tags", "1", "--filter-vid", vid_str, "1", "--set-rxif", ELANVIF[j], "--rule-alias", rule_alias_name, "--rule-append");
					}
				}
			}
		}
	}
#endif
	if(enable)
	{
#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#if defined(CONFIG_COMMON_RT_API)
		rtk_fc_loop_detection_acl_flush();
		rtk_fc_loop_detection_acl_set();
#endif
#endif
		mib_get_s(MIB_LBD_ETHER_TYPE, &ether_type, sizeof(ether_type));
		sprintf(cmd,"echo 0x%x > /proc/loopback_detect_ethtype",ether_type);
		printf("%s\n",cmd);
		system(cmd);
	}
#ifdef CONFIG_EPON_FEATURE
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
	if(pon_mode == EPON_MODE)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"oamcli set ctc loopdetect");
		//mib_get_s(MIB_LBD_ETHER_TYPE, &ether_type, sizeof(ether_type));
		mib_get_s(MIB_LBD_EXIST_PERIOD, &loopdetect_period, sizeof(loopdetect_period));
		mib_get_s(MIB_LBD_CANCEL_PERIOD, &loopdrecover_time, sizeof(loopdrecover_time));
		sprintf(cmd+strlen(cmd)," %d 0x%x %d %d", enable, ether_type,loopdetect_period,loopdrecover_time);
		totalvlan=mib_chain_total(MIB_LBD_VLAN_TBL);
		for(i=0; i<totalvlan; i++)
		{
			if(mib_chain_get(MIB_LBD_VLAN_TBL,i,&loopdetect_vlan))
			{
				if(i==0)
					sprintf(cmd+strlen(cmd), " %d", loopdetect_vlan.vid);
				else
					sprintf(cmd+strlen(cmd), ",%d", loopdetect_vlan.vid);
			}
		}
		printf("%s\n",cmd);
		system(cmd);

		//to make sure there is only one user loopback process.so before start, kill loopback.
		pid = read_pid("/var/run/loopback.pid");
		if(pid > 0)
		{
			kill(pid, SIGTERM);
			while(read_pid("/var/run/loopback.pid") > 0)
				usleep(200);
#ifdef CONFIG_CU
			// iptables -D ipfilter_out -j DROP
			va_cmd(IPTABLES, 4, 1, (char *)FW_DEL, (char *)FW_IPFILTER_OUT, "-j", (char *)FW_DROP);
#endif
		}
		if(enable)
			va_niced_cmd("/bin/loopback", 0, 0);
		else
			system("echo stop > /proc/loopback_detect");

		return;
	}
#endif

	pid = read_pid("/var/run/loopback.pid");
	if(pid > 0)
	{
		kill(pid, SIGTERM);
		while(read_pid("/var/run/loopback.pid") > 0)
			usleep(200);
#ifdef CONFIG_CU
		// iptables -D ipfilter_out -j DROP
		va_cmd(IPTABLES, 4, 1, (char *)FW_DEL, (char *)FW_IPFILTER_OUT, "-j", (char *)FW_DROP);
#endif
	}

	if(enable)
		va_niced_cmd("/bin/loopback", 0, 0);
	else
		system("echo stop > /proc/loopback_detect");
}
#endif


int update_hosts(char *hostname, struct addrinfo *servinfo)
{
	if(!hostname || !servinfo) return 0;

	struct addrinfo *p;
	struct sockaddr_in *h;
	struct sockaddr_in6 *h6;
	struct in_addr ina;
	struct in6_addr ina6;

	FILE *fp;
	int isV4Existed = 0, isV6Existed = 0;
	char line[80], ip[64];

	if(inet_pton(AF_INET6, hostname, &ina6) > 0 || inet_pton(AF_INET, hostname, &ina) > 0)
		return 0;

	fp = fopen(HOSTS, "a+");
	if(fp){
		while(fgets(line, 80, fp) != NULL)
		{
			char *ip,*name,*saveptr1;
			ip = strtok_r(line," \n", &saveptr1);
			name = strtok_r(NULL," \n", &saveptr1);

		if(name && !strcmp(name, hostname))
		{
			if(ip)
			{
				if(strchr(ip, '.') != NULL)
					isV4Existed = 1;
				else
					isV6Existed = 1;
			}
		}
	}

		// loop through all the results and connect to the first we can
		for(p = servinfo; p != NULL; p = p->ai_next)
		{
			if(p->ai_family == AF_INET)
			{
				h = (struct sockaddr_in *) p->ai_addr;
				ip[sizeof(ip)-1]='\0';
				strncpy(ip, inet_ntoa(h->sin_addr), sizeof(ip)-1);

			if(!isV4Existed)
				fprintf(fp, "%-15s %s\n", ip, hostname);
		}
		else if(p->ai_family == AF_INET6)
		{
			h6 = (struct sockaddr_in6 *) p->ai_addr;
			inet_ntop(AF_INET6, &h6->sin6_addr, ip, sizeof(ip));

			if(!isV6Existed)
	 			fprintf(fp, "%-15s %s\n", ip, hostname);
		}
	}

		fclose(fp);
	}
	return 1;
}

struct getaddrinfo_arg {
	char *hostname;
	IP_PROTOCOL IPVer;
	unsigned char end;
};

static void *getaddrinfo_routine(void *a)
{
	struct getaddrinfo_arg *parg = a;
	struct addrinfo hints, *servinfo = NULL;
	char *hostname = NULL;
	int rv;

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

	pthread_cleanup_push((void (*)(void *))freeaddrinfo, servinfo);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = parg->IPVer == IPVER_IPV4 ? AF_INET : parg->IPVer == IPVER_IPV6 ? AF_INET6 : AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	hostname = strdup(parg->hostname);

	if ((rv = getaddrinfo(hostname, NULL, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s, servinfo: %p\n", gai_strerror(rv), servinfo);
	}

	free(hostname);

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	parg->end = 1;

	pthread_cleanup_pop(0);

	return servinfo;
}

// IMPORTANGE NOTE: caller of this API must use freeaddrinfo() to free the returned memory.
struct addrinfo *hostname_to_ip(char *hostname, IP_PROTOCOL IPVer)
{
	pthread_t tid;
	struct getaddrinfo_arg arg = { hostname, IPVer, 0 };
	struct addrinfo *servinfo = NULL;
	int times = 0, rv;

	pthread_create(&tid, NULL, getaddrinfo_routine, &arg);

	/* wait 2 seconds at most */
	while (1) {
		if (arg.end)
			goto intime;

		if (++times >= 200)
			break;

		/* sleep for 0.01 seconds */
		usleep(10000);
	}

timeout:
	rv = pthread_cancel(tid);
	if (rv == ESRCH) {
		/*  No thread with the ID tid could be found */
		goto intime;
	}

	pthread_detach(tid);

	return NULL;
intime:
	pthread_join(tid, (void **)&servinfo);

	return servinfo;
}

int isHostAlive(char *hostname, IP_PROTOCOL IPVer)
{
	FILE *fp;
	int isV4Existed = 0, isV6Existed = 0;
	char line[80], ip[64];

	fp = fopen(HOSTS, "r");
	if (fp)
	{
		while(fgets(line, 80, fp) != NULL)
		{
			char *ip,*name,*saveptr1;
			ip = strtok_r(line," \n", &saveptr1);
			name = strtok_r(NULL," \n", &saveptr1);

			if(name && !strcmp(name, hostname))
			{
				if(ip)
				{
					if ((strchr(ip, '.') != NULL) && (IPVER_IPV4 == IPVer))
					{
						fclose(fp);
						return 1;
					}
					else if (IPVER_IPV6 == IPVer)
					{
						fclose(fp);
						return 1;
					}
				}
			}
		}
		fclose(fp);
	}

	if (hostname_to_ip(hostname, IPVer) != NULL)
		return 1;

	return 0;
}

//return 1: registered, return 0: failed.
int check_user_is_registered(void)
{
#if defined(_PRMT_X_CT_COM_USERINFO_) && defined(E8B_NEW_DIAGNOSE)
	unsigned int regStatus = 99;
	unsigned int regResult = 99;
	unsigned char no_pop;

	mib_get_s(PROVINCE_NO_POPUP_REG_PAGE, &no_pop, sizeof(no_pop));
	mib_get_s(CWMP_USERINFO_STATUS, &regStatus, sizeof(regStatus));
	mib_get_s(CWMP_USERINFO_RESULT, &regResult, sizeof(regResult));

	if(no_pop)
		return 1;

	if(regResult != 0 && regResult != 1)
		return 0;

	if(regStatus != 0 && regStatus != 5)
		return 0;

	return 1;
#else
	return 0;
#endif
}

int check_vlan_conflict(MIB_CE_ATM_VC_T *pEntry, int idx, char *err_msg)
{
	int i;
	MIB_CE_ATM_VC_T chk_entry = {0};
	int total = mib_chain_total(MIB_ATM_VC_TBL);

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	//return 0;
#endif

	// Accept duplicated vid if WAN interface is disabled
	if(pEntry->enable == 0)
		return 0;

	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, &chk_entry) == 0)
			continue;

		// Do not check it self
		if(i == idx)
			continue;

		// Accept duplicated vid if WAN interface is disabled
		if(chk_entry.enable == 0)
			continue;

		// Accept duplicated vid if different media type
		if(MEDIA_INDEX(pEntry->ifIndex) != MEDIA_INDEX(chk_entry.ifIndex))
			continue;

#ifdef CONFIG_TLKM
		if ((pEntry->cmode != CHANNEL_MODE_BRIDGE && chk_entry.cmode != CHANNEL_MODE_BRIDGE) ||
			(pEntry->cmode == CHANNEL_MODE_BRIDGE && chk_entry.cmode != CHANNEL_MODE_BRIDGE) ||
			(pEntry->cmode != CHANNEL_MODE_BRIDGE && chk_entry.cmode == CHANNEL_MODE_BRIDGE))
			continue;
#endif

		// Both are bridging or both are routing
		if((pEntry->cmode == CHANNEL_MODE_BRIDGE && chk_entry.cmode == CHANNEL_MODE_BRIDGE)
			|| ((pEntry->cmode != CHANNEL_MODE_BRIDGE && pEntry->cmode != CHANNEL_MODE_PPPOE) && (chk_entry.cmode != CHANNEL_MODE_BRIDGE && chk_entry.cmode != CHANNEL_MODE_PPPOE)))
		{
			// Both enable vlan tag and have the same vid,
			// or both disable vlan tag
			if((pEntry->vlan == 1 && chk_entry.vlan == 1 && pEntry->vid == chk_entry.vid))
			{
				// If both media type are ATM, check vpi/vci, too.
				if(MEDIA_INDEX(pEntry->ifIndex) != MEDIA_ATM)
					goto conflict;
				else if(pEntry->vpi == chk_entry.vpi && pEntry->vci == chk_entry.vci)
					goto conflict;
			}
		}
	}

	return 0;

conflict:
	{
		char ifname[IFNAMSIZ] = {0};
		ifGetName(PHY_INTF(chk_entry.ifIndex), ifname, IFNAMSIZ);
		sprintf(err_msg, "VLAN id %d is conflicted with %s\n", pEntry->vid, ifname);
	}
	return -1;
}

#ifdef CONFIG_USER_Y1731
const char Y1731_CONF[] = "/var/eoam.xml";
const char Y1731_MEG_CONF[] = "/var/eoam_meg.xml";
//#define OAMDPID "/var/run/oamd.pid"
const char OAMDPID[] = "/var/run/oamd.pid";
const char BR_Y1731[] = "y1731";
/**
 * Buffer for multicast DA class 1
 */
unsigned char y1731_da1[6];

/**
 * Buffer for multicast DA class 2
 */
unsigned char y1731_da2[6];


static void oam_create_multicast_da(int level, unsigned char *addr, int class) {
    unsigned char da[6];

    da[0] = 0x01;
    da[1] = 0x80;
    da[2] = 0xC2;
    da[3] = 0x0;
    da[4] = 0x0;

    switch (class) {
    case 1:
        da[5] = 0x30;
        da[5] += level;
        break;
    case 2:
        da[5] = 0x38;
        da[5] += level;
        break;
    default:
        da[5] = 0x0;
        printf("Invalid multiclass address class\n");
    }
    memcpy(addr, &da, sizeof(da));
}

/**
 * Function that creates the class 1 multicast destination address.
 * 01-80-C2-00-00-3x where x is 0-7
 *
 * @param level What is the current MEG level
 * @param addr buffer for the addr
 *
 * @note Be sure that addr is long enough to hold the multicast addr.
 */
void oam_create_multicast_da1(int level, unsigned char *addr) {
    oam_create_multicast_da(level, addr, 1);
}

/**
 * Function that creates the class 2 multicast destination address.
 * 01-80-C2-00-00-3y where y is 8-F
 *
 * @param level What is the current MEG level
 * @param addr buffer for the addr
 *
 * @note Be sure that addr is long enough to hold the multicast addr.
 */
void oam_create_multicast_da2(int level, unsigned char *addr) {
    oam_create_multicast_da(level, addr, 2);
}

int Y1731_setup(void) {
	char mode = 0, ccmEnable = 0, pulses = 0, level;
	FILE *fp;
	unsigned short myid, vid;
	char megid[Y1731_MEGID_LEN];
	char incl_iface[Y1731_INCL_IFACE_LEN];
	char loglevel[8];
	unsigned char ccm_interval;
	char ifname[IFNAMSIZ] = {0};
	unsigned int oamExtItf;
	char oamExtItf_ifname[16];

	mib_get_s(Y1731_MODE, (void *)&mode, sizeof(mode));
	if (0==mode)
		return -1;

	fp = fopen( Y1731_CONF, "w");
	if (NULL==fp)
		return -1;

	if (mib_get_s(Y1731_MEGLEVEL, (void *)&level, sizeof(level)) &&
		mib_get_s(Y1731_MYID, (void *)&myid, sizeof(myid)) &&
		mib_get_s(Y1731_VLANID, (void *)&vid, sizeof(vid)) &&
		mib_get_s(Y1731_MEGID, (void *)megid, sizeof(megid)) &&
		mib_get_s(Y1731_INCL_IFACE, (void *)incl_iface, sizeof(incl_iface)) &&
		//mib_get_s(Y1731_WAN_INTERFACE, (void *)&oamExtItf_ifname, sizeof(oamExtItf_ifname)) &&
		mib_get_s(Y1731_LOGLEVEL, (void *)loglevel, sizeof(loglevel)) &&
		mib_get_s(Y1731_ENABLE_CCM, (void *)&ccmEnable, sizeof(ccmEnable)) &&
		mib_get_s(Y1731_CCM_INTERVAL, (void *)&ccm_interval, sizeof(ccm_interval))
		) {
			char *ccm_prec, *ccm_period;

			switch (ccm_interval) {
			case 1: ccm_prec = "ms"; ccm_period = "3"; break;
			case 2: ccm_prec = "ms"; ccm_period = "10"; break;
			case 3: ccm_prec = "ms"; ccm_period = "100"; break;
			case 4: ccm_prec = "s";  ccm_period = "1"; break;
			case 5: ccm_prec = "s";  ccm_period = "10"; break;
			case 6: ccm_prec = "min";  ccm_period = "1"; break;
			case 7: ccm_prec = "min";  ccm_period = "10"; break;
			default:
				ccm_prec = "s";  ccm_period = "1"; break;
			}

			if(ccmEnable)
				pulses = 5;
			else
				pulses = 0;

		fprintf(fp, "<?xml version=\"1.0\"?>\n"
			"<me>\n"
			"<meg name=\"testmeg\" id=\"%s\" level=\"%d\" max=\"16\" />\n"
			"<myid name=\"wel-36\" id=\"%d\"/>\n"
			"<vid name=\"vlan\" id=\"%d\"/>\n"
			"<config log=\"stderr\" loglevel=\"%s\" pulses=\"%d\" dynamic=\"on\" />\n"
			"<ccd use=\"off\" ip=\"127.0.0.1\" port=\"8080\" />\n"
			"<ccm precision=\"%s\" period=\"%s\" one-way-loss-measurement=\"on\" />\n"
			"<lt ttl=\"10\" />\n"
			"<ais precision=\"s\" period=\"1\" />\n"
			"<lck precision=\"s\" period=\"1\" />\n"
			"<mcc oui=\"00:22:68\" />\n"
			"<lm used=\"off\" precision=\"ms\" period=\"100\" warning=\"10\" error=\"20\" samples=\"30\" th_warning=\"40\" th_error=\"50\"/>\n"
			"<dm used=\"off\" optional_fields=\"on\" precision=\"ms\" period=\"100\" warning=\"10\" error=\"20\" samples=\"30\" th_warning=\"40\" th_error=\"50\" />\n"
			"<include ifs=\"%s\" />\n"
			"</me>\n", megid, level, myid, vid, loglevel, pulses, ccm_prec, ccm_period, incl_iface);
		fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
		chmod(Y1731_CONF, 0666);
#endif
	} else {
		goto ERROR_00;
	}

	fp = fopen(Y1731_MEG_CONF, "w");
	if (NULL==fp)
		return -1;

	fprintf(fp, "<?xml version=\"1.0\"?>\n"
		"<participants>\n"
		//"<meg>\n"
		//"<mep name=\"aravis\" id=\"1\">\n"
		//"<mac>%s</mac>\n"
		//"<ifname>%s</ifname>\n"
		//"</mep>\n"
		//"</meg>\n"
		//"</participants>\n", macaddr, oamExtItf_ifname);
		"</participants>\n");
	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(Y1731_MEG_CONF, 0666);
#endif

	return 0;
ERROR_00:
	fclose(fp);
	return -1;
}

#if 0
int set_BROUTE_Rule_Y1731(int mode)
{
	int status = 0;
	char ifname[IFNAMSIZ] = {0};
	int vcTotal, i;
	MIB_CE_ATM_VC_T Entry;
	char level;
	char macaddr[20];

	if (mode == ADD_RULE) {
		status |= va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N",
				 (char *)BR_Y1731);
		status |= va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P",
				 (char *)BR_Y1731, (char *)FW_RETURN);
		status |= va_cmd(EBTABLES, 6, 1, "-t", "broute", "-A", "BROUTING",
			   "-j", (char *)BR_Y1731);


		if (!mib_get_s(Y1731_MEGLEVEL, (void *)&level, sizeof(level))){
			printf("Get Y1731_MEGLEVEL failed\n");
			return -1;
		}
		oam_create_multicast_da1(level, y1731_da1);
		oam_create_multicast_da2(level, y1731_da2);

		vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < vcTotal; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				return -1;
			if (Entry.enable == 0)
				continue;
			if ((CHANNEL_MODE_T)Entry.cmode != CHANNEL_MODE_BRIDGE)
				continue;
			#ifdef CONFIG_ETHWAN
			if(( MEDIA_INDEX(Entry.ifIndex)==MEDIA_ETH ) || ( MEDIA_INDEX(Entry.ifIndex)==MEDIA_PTM ))
			#endif
			{
				snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
					y1731_da1[0], y1731_da1[1], y1731_da1[2], y1731_da1[3], y1731_da1[4], y1731_da1[5]);
				//eg.: ebtables -t broute -A BROUTING -d 01:80:c2:00:00:32 -j DROP
				va_cmd(EBTABLES, 8, 1, "-t", "broute", "-A", BR_Y1731, "-d", macaddr, "-j", "DROP");

				snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
					y1731_da2[0], y1731_da2[1], y1731_da2[2], y1731_da2[3], y1731_da2[4], y1731_da2[5]);
				//eg.: ebtables -t broute -A BROUTING -d 01:80:c2:00:00:38 -j DROP
				va_cmd(EBTABLES, 8, 1, "-t", "broute", "-A", BR_Y1731, "-d", macaddr, "-j", "DROP");
				break;
			}
		}
	} else if (mode == DEL_RULE) {
		status |= va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F",
				 (char *)BR_Y1731);
		status |= va_cmd(EBTABLES, 6, 1, "-t", "broute", "-D", "BROUTING",
			   "-j", (char *)BR_Y1731);
		status |= va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X",
				(char *)BR_Y1731);
	}
	return status;
}
#endif

void Y1731_stop(void) {
	pid_t pid;
	pid = read_pid((char *)OAMDPID);
	if (pid > 0)
	{
		//set_BROUTE_Rule_Y1731(DEL_RULE);
		kill(pid, 15);
		//unlink(OAMDPID);
		while(read_pid((char*)OAMDPID) > 0)
			usleep(250000);
	}
}

int Y1731_start(int dowait) {
	pid_t pid;
	char ccmEnable = 0;

	Y1731_stop();
	if (Y1731_setup())
		return -1;

	//set_BROUTE_Rule_Y1731(ADD_RULE);
	mib_get_s(Y1731_ENABLE_CCM, (void *)&ccmEnable, sizeof(ccmEnable));
	if (ccmEnable) {
		va_niced_cmd("/bin/oamd", 4, dowait,
			"-c", Y1731_CONF,
			"-m", Y1731_MEG_CONF);
	}
	else {
		va_niced_cmd("/bin/oamd", 5, dowait,
			"-c", Y1731_CONF,
			"-m", Y1731_MEG_CONF,
			"-x");  // Not send CCM
	}
	pid = read_pid((char *)OAMDPID);
	return (pid>0) ? 0 : -1;
}

#endif

#ifdef CONFIG_USER_8023AH
#define PID_FILE_8023AH "/var/run/oamlinkd.pid"

void EFM_8023AH_stop(void) {
	pid_t pid;
	pid = read_pid((char *)PID_FILE_8023AH);
	if (pid > 0)
	{
		kill(pid, 9);
		unlink(PID_FILE_8023AH);
	}
}

int EFM_8023AH_start(int dowait) {
	pid_t pid;
	char Enable = 0, Active = 0;
	unsigned int efm_8023ah_ext_if;
	char ifname[IFNAMSIZ] = {0};

	EFM_8023AH_stop();

	mib_get_s(EFM_8023AH_MODE, (void *)&Enable, sizeof(Enable));
	if (Enable == 0)
		return -1;

	mib_get_s(EFM_8023AH_WAN_INTERFACE, (void *)ifname, sizeof(ifname));
	mib_get_s(EFM_8023AH_ACTIVE, (void *)&Active, sizeof(Active));
	if (Active == 0)  // passive
		va_niced_cmd("/bin/oamlinkd", 2, dowait, "-i", ifname);
	if (Active == 1)  //Active
		va_niced_cmd("/bin/oamlinkd", 4, dowait, "-m", "1", "-i", ifname);

	pid = read_pid((char *)PID_FILE_8023AH);
	return (pid>0) ? 0 : -1;
}

#endif

#ifdef CONFIG_USB_SUPPORT
int getUsbDeviceLabel(const char *device, char *label, char *type)
{
        FILE *fp;
        char command[100], buff[256], tmpBuf[256], *pStr;
        int i;

        snprintf(command, 100, "/bin/blkid %s > /tmp/blkid_file", device);
        system(command);


        if ((fp = fopen("/tmp/blkid_file", "r")) == NULL) {
                printf("open /tmp/blkid_file failed.\n");
                return 0;
        }
        else {
                if ( fgets(buff, sizeof(buff), fp) != NULL )
                {
                        memcpy(tmpBuf, buff, 256);
                        if ((pStr = strstr(tmpBuf, "LABEL=")) != NULL) {
                                pStr += strlen("LABEL=");

                                /* LABEL="LISI" */
                                pStr++;
                                i = 0;
                                while (pStr[i] != '"')
                                        i++;
                                pStr[i] = '\0';

                                strcpy(label, pStr);
                        }
                        else
                                strcpy(label, "Unknown");

                        memcpy(tmpBuf, buff, 256);
                        if ((pStr = strstr(tmpBuf, "TYPE=")) != NULL) {
                                pStr += strlen("TYPE=");

                                pStr++;
                                i=0;
                                while (pStr[i] != '"')
                                        i++;
                                pStr[i] = '\0';
                                strcpy(type, pStr);
                        }
                        else
                                strcpy(type, "Unknown");
                }

                fclose(fp);
                unlink("/tmp/blkid_file");
        }
        return 1;
}

#if 1
int usb_filter(const struct dirent *dirent)
{
	return !strncmp(dirent->d_name, "usb", 3);
}
#else
int usb_filter(const struct dirent *dirent)
{
        const char *name = dirent->d_name;
        struct stat statbuff;
        char path[32];

        if((strlen(name) == 4) && (name[0] == 's') && (name[1] == 'd')
           && (name[2] >= 'a') && (name[2] <= 'z')
           && (name[3] >= '0') && (name[3] <= '9'))
                return 1;
        else if((strlen(name) == 3) &&(name[0] == 's') && (name[1] == 'r')
                && (name[2] >= '0') && (name[2] <= '9'))
                return 1;
        else if ((strlen(name) == 3) &&(name[0] == 's') && (name[1] == 'd')
                && (name[2] >= 'a') && (name[2] <= 'z')) {
                sprintf(path, "/mnt/%s", name);
                if (stat(path, (struct stat *)&statbuff) == -1)
                        return 0;
                else if (0 == statbuff.st_blocks)
                        return 0;
                return 1;
        }

        return 0;
}
#endif

int isUSBMounted(void)
{
	struct dirent **namelist;
	int i, n;

	n = scandir("/mnt", &namelist, usb_filter, alphasort);

	/* no match */
	if (n < 0)
		return -1;

	for (i = 0; i < n; i++) {
		free(namelist[i]);
	}
	free(namelist);

	return n;
}

int umountUSBDevie(void)
{
        int errcode = 1;
        struct dirent **namelist;
        char device[100];
        int i, n;

        n = scandir("/mnt", &namelist, usb_filter, alphasort);

        if (n <= 0)
                return 1;

        for (i = 0; i < n; i++)
        {
                if (namelist[i]->d_name[0] == 's')
                {
                        snprintf(device, 100, "/dev/%s", namelist[i]->d_name);
                        va_cmd("/bin/umount", 1, 1, (char *)device);
                }

                free(namelist[i]);
        }
        free(namelist);

        return 1;
}


void getUSBDeviceInfo(int *disk_sum, struct usb_info * disk1,struct usb_info * disk2)
{
        struct dirent **namelist;
        char tmpLabel[100], tmpFSType[10];
        char device[2][20], type[2][100], mounton[2][512];
        int  usb_storage_num[2];
        char usb_storage_dev[2][10];
        char usb_storage_serial[2][100];
        char invert=0;
        char tmpstr1[100], tmpstr2[100];
        char usb1_serial[100], usb2_serial[100];
        char usb1_dev[10], usb2_dev[10];
        char cmd[100];
        char line[512] = {0};
        FILE *pf = NULL;
        int i, n, diskSum=0;
        unsigned long used[2],avail[2];

        n = scandir("/mnt", &namelist, usb_filter, alphasort);
        memset(type, 0, sizeof(type));
        memset(mounton,0,sizeof(mounton));
        memset(used,0,sizeof(used));
        memset(avail,0,sizeof(avail));

        /* no match */
        if (n > 0)
        {
                for (i=0; i<n; i++) {
                        if ((namelist[i]->d_name[0] == 's') && (diskSum < 2)) {
                                if (strlen(namelist[i]->d_name) == 3)
                                        namelist[i]->d_name[3] = '\0';
                                else
                                        namelist[i]->d_name[4] = '\0';
                                snprintf(device[diskSum], 20, "/dev/%s", namelist[i]->d_name);
                                memset(tmpLabel, 0, sizeof(tmpLabel));
                                memset(tmpFSType, 0, sizeof(tmpFSType));
                                getUsbDeviceLabel(device[diskSum], tmpLabel, tmpFSType);
                                snprintf(type[diskSum], 100, "%s", tmpFSType);
                                snprintf(cmd,sizeof(cmd),"df %s > /tmp/usbinfo",device[diskSum]);
                                system(cmd);
                                pf=fopen("/tmp/usbinfo","r");
                                if(pf)
                                {
                                        fgets(line,sizeof(line),pf);
                                        if(fgets(line,sizeof(line),pf))
                                        {
                                                sscanf(line,"%*s %*d %d %d %*s %s\n",&used[diskSum],&avail[diskSum],mounton[diskSum]);
                                        }
                                        fclose(pf);
                                        unlink("/tmp/usbinfo");
                                }
                                diskSum++;
                        }
                        free(namelist[i]);
                }

                free(namelist);
        }
        *disk_sum = diskSum;

        usb_storage_num[0] = usb_storage_num[1] = -1;
        memset(usb_storage_dev, 0, sizeof(usb_storage_dev));
        memset(usb_storage_serial, 0, sizeof(usb_storage_serial));

        snprintf(cmd, 100, "ls /proc/scsi/usb-storage/  > /tmp/storage.tmp 2>&1");
        system(cmd);

        pf = fopen("/tmp/storage.tmp", "r");
        if(pf) {
                i = 0;
                while(fgets(line, sizeof(line), pf)!=NULL) {
                        //printf("%s:%s", __func__, line);
                        sscanf(line, "%d", &usb_storage_num[i]);

                        i++;
                        if (i >= 2)
                                break;
                }
                //printf("usb_storage_num %d %d\n", usb_storage_num[0], usb_storage_num[1]);
                fclose(pf);
        }

        for (i=0; i<2; i++)
        {
                if (usb_storage_num[i] != -1) {
                        snprintf(cmd, 100, "ls /sys/class/scsi_device/%d:0:0:0/device/block/ > /tmp/storage.tmp 2>&1",
                                        usb_storage_num[i]);
                        system(cmd);

                        pf = fopen("/tmp/storage.tmp", "r");
                        if(pf) {
                                fgets(line, sizeof(line), pf);
                                //printf("%s\n", line);
                                sscanf(line, "%s", usb_storage_dev[i]);
                                fclose(pf);
                        }

                        snprintf(cmd, 100, "cat /proc/scsi/usb-storage/%d > /tmp/storage.tmp 2>&1", usb_storage_num[i]);
                        system(cmd);

                        pf = fopen("/tmp/storage.tmp", "r");
                        if(pf) {
                                while(fgets(line, sizeof(line), pf)!=NULL) {
                                        //printf("%s\n", line);
                                        sscanf(line, "%s %*s %s", tmpstr1, tmpstr2);
                                        if (strstr(tmpstr1, "Serial")) {
                                                strcpy(usb_storage_serial[i], tmpstr2);
                                                //printf("%s:storage_serial[%d]:%s\n", __func__, i, usb_storage_serial[i]);
                                                break;
                                        }
                                }
                                fclose(pf);
                        }
                }
        }

        memset(usb1_serial, 0, sizeof(usb1_serial));
        memset(usb2_serial, 0, sizeof(usb2_serial));
        memset(usb1_dev, 0, sizeof(usb1_dev));
        memset(usb2_dev, 0, sizeof(usb2_dev));

        pf = fopen("/sys/bus/usb/devices/2-1/serial", "r");
        if (!pf)
                pf = fopen("/sys/bus/usb/devices/2-2/serial", "r");
        if (pf) {
                fgets(line, sizeof(line), pf);
                sscanf(line, "%s", usb1_serial);
                //printf("%s:usb2_serial:%s", __func__, usb1_serial);
                fclose(pf);
        }

        pf = fopen("/sys/bus/usb/devices/1-2/serial", "r");
        if (!pf)
                pf = fopen("/sys/bus/usb/devices/1-1/serial", "r");
        if (pf) {
                fgets(line, sizeof(line), pf);
                sscanf(line, "%s", usb2_serial);
                //printf("%s:usb1_serial:%s\n", __func__, usb2_serial);
                fclose(pf);
        }

        unlink("/tmp/storage.tmp");

        if (usb1_serial[0]) {//usb1 running(USB2.0)
                for (i=0; i<2; i++) {
                        if (usb_storage_serial[i][0] && !strcmp(usb_storage_serial[i], usb1_serial)) {
                                strcpy(usb1_dev, usb_storage_dev[i]);
                                break;
                        }
                }
                //debug
                if (i >= 2)
                        printf("unknown usb1 dev name\n");
                else {
                        if (strstr(device[0], usb1_dev))
                                invert = 0;
                        else if (strstr(device[1], usb1_dev))
                                invert = 1;
                }
        }

        if (usb2_serial[0]) {//usb2 running(USB3.0)
                for (i=0; i<2; i++) {
                        if (usb_storage_serial[i][0] && !strcmp(usb_storage_serial[i], usb2_serial)) {
                                strcpy(usb2_dev, usb_storage_dev[i]);
                                break;
                        }
                }
                //debug
                if (i >= 2)
                        printf("unknown usb2 dev name\n");
                else {
                        if (strstr(device[0], usb2_dev))
                                invert = 1;
                        else if (strstr(device[1], usb2_dev))
                                invert = 0;
                }
        }

        if(usb1_dev[0])
        {
                strcpy(disk1->disk_type, usb1_dev);
                strcpy(disk1->disk_status, "Mounted");
                if(invert)
                {
                        sprintf(disk1->disk_fs, "%s", type[1]);
                        disk1->disk_used=used[1];
                        disk1->disk_available=avail[1];
						memset(disk1->disk_mounton, 0, sizeof(disk1->disk_mounton));
                        strncpy(disk1->disk_mounton, mounton[1], sizeof(disk1->disk_mounton)-1);
                }
                else
                {
                        sprintf(disk1->disk_fs, "%s", type[0]);
                        disk1->disk_used=used[0];
                        disk1->disk_available=avail[0];
						memset(disk1->disk_mounton, 0, sizeof(disk1->disk_mounton));
                        strncpy(disk1->disk_mounton,  mounton[0], sizeof(disk1->disk_mounton)-1);
                }
        }
        else
        {
                strcpy(disk1->disk_type, "No Device");
                strcpy(disk1->disk_status, "Disconnect");
                strcpy(disk1->disk_fs,"");
                strcpy(disk1->disk_mounton,"");
                disk1->disk_used=0;
                disk1->disk_available=0;
        }

        if(usb2_dev[0])
        {
                strcpy(disk2->disk_type, usb2_dev);
                strcpy(disk2->disk_status, "Mounted");
                if(invert)
                {
                        sprintf(disk2->disk_fs, "%s", type[0]);
                        disk2->disk_used=used[0];
                        disk2->disk_available=avail[0];
						memset(disk2->disk_mounton, 0, sizeof(disk2->disk_mounton));
                        strncpy(disk2->disk_mounton, mounton[0], sizeof(disk2->disk_mounton)-1);
                }
                else
                {
                        sprintf(disk2->disk_fs, "%s", type[1]);
                        disk2->disk_used=used[1];
                        disk2->disk_available=avail[1];
						memset(disk2->disk_mounton, 0, sizeof(disk2->disk_mounton));
                        strncpy(disk2->disk_mounton, mounton[1], sizeof(disk2->disk_mounton)-1);
                }
        }
        else
        {
                strcpy(disk2->disk_type, "No Device");
                strcpy(disk2->disk_status, "Disconnect");
                strcpy(disk2->disk_fs,"");
                strcpy(disk2->disk_mounton,"");
                disk2->disk_used=0;
                disk2->disk_available=0;
        }

}
#endif//end of CONFIG_USB_SUPPORT

/**************************************************************************
 * NAME:    usbRestore
 * DESC:    check if USB is mounted and config file exist. If yes, then go ahead and use it
 *          for restore configuration.
 *ARGS:     NONE
 *RETURN:   0  - succeed,
            -1 - not enable MIB_USBRESTORE
            -2 - no matching config file
            -3 - backup restore failed
 **************************************************************************/
#ifdef _PRMT_USBRESTORE
int usbRestore(void)
{
    char buffer[16], dstname[64], usb_cfg_filename[32];
    int rv = -3, i, n, found=0;
    struct dirent **namelist;
    struct file_pipe pipe;
    unsigned char cpbuf[256], vChar=0;
    const char *config_filename;

    mib_get_s(MIB_USBRESTORE, &vChar, sizeof(vChar));
    if (vChar == 0)
    {
        return -1;
    }

    fprintf(stderr, "ENTER %s\n", __FUNCTION__);
    fprintf(stderr, "check usbRestore...\n");

    // wait for usbmount successfully
    for (i = 0; i < 5; i++) {
        sleep(1);
        n = scandir("/mnt", &namelist, usb_filter, alphasort);

        /* no match */
        if (n <= 0) {
            if (n == 0)
                free(namelist);
            continue;
        } else
            break;
    }

    if (n <= 0) {
        return -2;
    }

    // make usb config filename (dst file)
    snprintf(usb_cfg_filename, sizeof(usb_cfg_filename), "ctce8.cfg");

    // wait for usbmount successfully
    sleep(3);
    pipe.buffer = cpbuf;
    pipe.bufsize = sizeof(cpbuf);
    pipe.func = decode;
    for (i = 0; i < n; i++) {
        snprintf(dstname, sizeof(dstname), "/mnt/%s/%s/%s",
             namelist[i]->d_name, BACKUP_DIRNAME, usb_cfg_filename);
        /* if not exist */
        if (access(dstname, F_OK))
            continue;

        fprintf(stderr, "USB: found %s\n", dstname);
        found = 1;

#if defined(CONFIG_USER_XMLCONFIG) || defined(CONFIG_USE_XML)
        config_filename = CONFIG_XMLFILE;
#else
        config_filename = CONFIG_RAWFILE;
#endif
        rv = file_copy_pipe(dstname, config_filename, &pipe);
        if (rv != 0) {
            fprintf(stderr, "copy failed (%d)\n", rv);
            rv = -3;
            goto out;
        }
        // do restore here...
        fprintf(stderr, "USB: start config restore...\n");
        after_download(config_filename);

        fprintf(stderr, "\n\n\nUSB: config restore done\n\n\n");
        rv = 0;
        sync();
        break;
    }
    if(!found)
        rv = -2;

out:
    for (i = 0; i < n; i++) {
        free(namelist[i]);
    }
    free(namelist);

    return rv;
}
#endif

#ifdef CONFIG_E8B
static unsigned long sn_matrix[8][8] = {
{0x111C3D60, 0x0B5D60DC, 0x05C94DB0, 0x08A9F558, 0x154848D0, 0x001504A2, 0x0D0C652A, 0x141BE95C},
{0x034EAC58, 0x3B079DAA, 0x09FFFBFA, 0x003F9E94, 0x11618D09, 0x0D5E558A, 0x2577BE5C, 0x003E7AF8},
{0x008AE4C9, 0x0855C770, 0x34143744, 0x1AF6F0A8, 0x05686380, 0x11BCF974, 0x1F96A973, 0x0D81EFE7},
{0x2DB9F622, 0x26218B68, 0x16F02844, 0x09B62188, 0x2524312E, 0x0AF9AB28, 0x2360D510, 0x0018C501},
{0x024CCE7F, 0x0568C990, 0x03AEA55A, 0x0B6AD558, 0x0FDA28A2, 0x12F9E4FC, 0x24252765, 0x111D6B0F},
{0x15D4E175, 0x10AA5B93, 0x13C6CDA0, 0x1473DF94, 0x1AAEE3E0, 0x258EDC1B, 0x19F410CF, 0x000654B8},
{0x04D697F6, 0x141C44B1, 0x0E32F287, 0x0622CA9C, 0x1186804E, 0x0031FD64, 0x101ED862, 0x20197942},
{0x0030AC6E, 0x2A8F8EBD, 0x01918510, 0x02BDF1FC, 0x0C551F68, 0x0C330868, 0x0C0434B6, 0x08CD55A6}};

void encode(unsigned char *buf, size_t * buflen)
{
	int pd = 0;
	unsigned char *p = buf;

	if (p == NULL || buflen == 0)
		return;
	while (pd < *buflen) {
		*p ^= ((unsigned char *)sn_matrix)[pd % 256];
		p++;
		pd++;
	}
}

void decode(unsigned char *buf, size_t *buflen)
{
	encode(buf, buflen);
}

int file_copy_pipe(const char *inputfile, const char *outputfile, struct file_pipe *pipe)
{
	FILE *input=NULL, *output=NULL;
	size_t b_read, b_write;
	int quit = 0, rv = 0;

	input = fopen(inputfile, "rb");
	if (input == NULL)
		return errno;

	output = fopen(outputfile, "wb");
	if (output == NULL){
		fclose(input);
		return errno;
	}

	do {
		b_read = fread(pipe->buffer, 1, pipe->bufsize, input);
		if (b_read < pipe->bufsize) {
			quit = 1;
		}
		if (pipe->func)
			pipe->func(pipe->buffer, &b_read);

		b_write = fwrite(pipe->buffer, 1, b_read, output);
		if (b_read != b_write) {
			rv = ferror(output);
			quit = 1;
		}
	} while (!quit);

	fflush(output);
	fclose(input);
	fclose(output);

	return rv;
}

#endif

#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
int set_internet_status(unsigned char internet_status)
{
	unsigned char mib_internet_status;

	mib_get_s(MIB_USER_TROUBLE_WIZARD_INTERNET_STATUS, (void *)&mib_internet_status, sizeof(mib_internet_status));

	if((mib_internet_status == INTERNET_NOT_READY )&& (internet_status == INTERNET_CONNECTED)){
		printf("Save internet status to CONNECTED into MIB!");
		mib_set(MIB_USER_TROUBLE_WIZARD_INTERNET_STATUS, (void *)&internet_status);
		return internet_status;
	}
}

unsigned char get_internet_status()
{
	unsigned char internet_status;

	mib_get_s(MIB_USER_TROUBLE_WIZARD_INTERNET_STATUS, (void *)&internet_status, sizeof(internet_status));
	//printf("Get internet status : %d!\n",internet_status);
	return internet_status;

}
#endif

#ifdef CONFIG_RTK_HOST_SPEEDUP
#define HOST_SPEEDUP "/proc/HostSpeedUP-Info"

/*
 * FUNCTION:ADD_HOST_SPEEDUP
 *
 * Set the IP and Port of local in/out network traffics, which you don't want to pass through netfilter.
 *
 * INPUT:
 *	RIP:
 *		remote ip.
 *	Rport:
 *		remote port.
 *	LIP:
 *		local ip.
 *	Lport:
 *		local port.
 *
 *	RETURN:
 *	0: Sucess
 *	-1: Failure
 */

int add_host_speedup(struct in_addr rip, unsigned short rport, struct in_addr lip, unsigned short lport)
{

	int ret=0;
	FILE *fp=NULL;
	char str_rip[64] = {0};
	char str_lip[64] = {0};

	if(rip.s_addr==0 || lip.s_addr==0){
		printf("srcip or dstip equal to 0 is not accept!\n");
		ret = -1;
		return ret;
	}
	inet_ntop(AF_INET, (const void *) &rip, str_rip, 64);
	inet_ntop(AF_INET, (const void *) &lip, str_lip, 64);
//printf("%s-%d sip=%s, sport=%d, dip=%s, dport=%d\n",__func__,__LINE__, str_srcip,sport,str_dstip,dport);
	fp = fopen(HOST_SPEEDUP, "w");
	if(fp)
	{
		fprintf(fp, "add rip %s rport %d lip %s lport %d\n",str_rip,rport,str_lip,lport);
		fclose(fp);
		ret = 0;
	}else{
		fprintf(stderr, "open %s fail!\n",HOST_SPEEDUP);
		ret = -1;
	}

    va_cmd("/bin/sh", 2, 1, "-c", "echo 1 > /proc/HostSpeedUP");

	return ret;
}

/*
 * FUNCTION:DEL_HOST_SPEEDUP
 *
 * Delete the track of IP and Port for local in/out network traffics, which you don't want to pass through netfilter.
 *
 * INPUT:
 *	RIP:
 *		remote ip.
 *	Rport:
 *		remote port.
 *	LIP:
 *		local ip.
 *	Lport:
 *		local port.
 *
 *	RETURN:
 *	0: Sucess
 *	-1: Failure
 */

int del_host_speedup(struct in_addr rip, unsigned short rport, struct in_addr lip, unsigned short lport)
{

	int ret=0;
	FILE *fp=NULL;
	char str_rip[64] = {0};
	char str_lip[64] = {0};

	inet_ntop(AF_INET, (const void *) &rip, str_rip, 64);
	inet_ntop(AF_INET, (const void *) &lip, str_lip, 64);
//printf("%s-%d sip=%s, sport=%d, dip=%s, dport=%d\n",__func__,__LINE__, str_srcip,sport,str_srcip,dport);
	fp = fopen(HOST_SPEEDUP, "w");
	if(fp)
	{
		fprintf(fp, "del info\n");
		fclose(fp);
		ret = 0;
	}else{
		fprintf(stderr, "open %s fail!\n",HOST_SPEEDUP);
		ret = -1;
	}
    va_cmd("/bin/sh", 2, 1, "-c", "echo 0 > /proc/HostSpeedUP");

	return ret;
}


#define SPEEDUP_HISTORY_RESULT_FILE "/tmp/speedup_history_result"
#define SPEEDUP_HISTORY_RESULT_LOCK_FILE "/var/speedup_history_result_lock"
#define SPEEDUP_COMFIRMRATE_VALID_TIME 600//s
#define SPEEDUP_COMFIRM_TIMES 3

int  check_speedup_history_result(unsigned int *sample_values, int sample_num)
{
	FILE*fp;
	int i,lockfd,historty_num=0;
	long confirmtime=0;
	unsigned long long avg_rate=0,last_avg_rate=0,confirmrate=0;
	struct sysinfo info;
	unsigned long long avg_value=0;
	unsigned int change_flag=0;

	for(i=0; i<sample_num; i++)
	{
		avg_rate += sample_values[i];
	}
	avg_rate /= sample_num;

	if((lockfd = lock_file_by_flock(SPEEDUP_HISTORY_RESULT_LOCK_FILE, 1)) == -1)
	{
		printf("lock file %s fail\n", SPEEDUP_HISTORY_RESULT_LOCK_FILE);
		return 0;
	}
	fp=fopen(SPEEDUP_HISTORY_RESULT_FILE,"r");
	if(fp)
	{
		if(fscanf(fp,"confirmrate:%llu time:%ld num:%d rate:%llu\n", &confirmrate, &confirmtime, &historty_num, &last_avg_rate)!=4)
		{
			confirmrate = 0;
			confirmtime = 0;
			historty_num = 0;
			last_avg_rate = 0;
		}
		fclose(fp);
	}
	sysinfo(&info);
	if((confirmtime!=0) && ((info.uptime-confirmtime) >= SPEEDUP_COMFIRMRATE_VALID_TIME))
	{
		confirmrate = 0;
		last_avg_rate = 0;
		confirmtime = info.uptime;
	}
	if((avg_rate<(last_avg_rate*9/10)) || (avg_rate>(last_avg_rate*11/10)))
	{
		historty_num = 1;
		last_avg_rate = avg_rate;
	}
	else
	{
		last_avg_rate = last_avg_rate*historty_num + avg_rate;
		historty_num++;
		last_avg_rate /= historty_num;
	}
	if(historty_num >= SPEEDUP_COMFIRM_TIMES)
	{
		confirmrate = last_avg_rate;
		confirmtime = info.uptime;
	}

	if(((info.uptime-confirmtime) < SPEEDUP_COMFIRMRATE_VALID_TIME) && (confirmrate!=0)
		&& ((avg_rate<(confirmrate*9/10)) || (avg_rate>(confirmrate*11/10))))
	{
		for(i=0; i<sample_num; i++)
		{
			long long deviation = (long long)confirmrate*(2*i-sample_num)/sample_num;
			srandom(time(NULL)+i);
			deviation = deviation*(random()%20)/1000;
			sample_values[i] = confirmrate+deviation;
		}
		fprintf(stderr, "%s avg_value=%llu\n", __func__, confirmrate);
		change_flag = 1;
	}
	fp=fopen(SPEEDUP_HISTORY_RESULT_FILE,"w");
	if(fp)
	{
		fprintf(stderr,"confirmrate:%llu time:%ld num:%d rate:%llu\n",confirmrate,confirmtime,historty_num,last_avg_rate);
		fprintf(fp,"confirmrate:%llu time:%ld num:%d rate:%llu\n",confirmrate,confirmtime,historty_num,last_avg_rate);
		fclose(fp);
	}

	unlock_file_by_flock(lockfd);
	return change_flag;
}

void delete_speedup_history_result(void)
{
	int lockfd;
	if((lockfd = lock_file_by_flock(SPEEDUP_HISTORY_RESULT_LOCK_FILE, 1)) == -1)
	{
		printf("lock file %s fail\n", SPEEDUP_HISTORY_RESULT_LOCK_FILE);
		return;
	}
	unlink(SPEEDUP_HISTORY_RESULT_FILE);
	unlock_file_by_flock(lockfd);
}

int check_speedup_need_tunedown(int speed)
{
	unsigned char province_name[32]={0};

	mib_get_s(MIB_HW_PROVINCE_NAME, province_name, sizeof(province_name));
	switch(speed)
	{
		case 200:
			if(!strncmp(province_name,"DM",2))
				return 1;
			break;
		case 300:
			if(!strncmp(province_name,"SN",2))
				return 1;
			break;
		case 500:
			if(!strncmp(province_name,"DM",2))
				return 1;
			break;
		default:
			break;
	}
	return 0;
}

#endif /*CONFIG_RTK_HOST_SPEEDUP*/

#ifndef inet_ntoa_r
static unsigned int i2a(char* dest,unsigned int x) {
	register unsigned int tmp=x;
	register unsigned int len=0;
	if (x>=100) { *dest++=tmp/100+'0'; tmp=tmp%100; ++len; }
	if (x>=10) { *dest++=tmp/10+'0'; tmp=tmp%10; ++len; }
	*dest++=tmp+'0';
	return len+1;
}

char *inet_ntoa_r(struct in_addr in,char* buf) {
	unsigned int len;
	unsigned char *ip=(unsigned char*)&in;
	len=i2a(buf,ip[0]); buf[len]='.'; ++len;
	len+=i2a(buf+ len,ip[1]); buf[len]='.'; ++len;
	len+=i2a(buf+ len,ip[2]); buf[len]='.'; ++len;
	len+=i2a(buf+ len,ip[3]); buf[len]=0;
	return buf;
}
#endif

/************************************************
* Propose: checkProcess()
*    Check process exist or not
* Parameter:
*	char* pidfile      pid file path
* Return:
*      1  :  exist
*      0  :  not exist
*      -1:  parameter error
* Author:
*     Alan
*************************************************/
int checkProcess(char* pidfile)
{
	char command[256] = {0};
	char output[256] = {0};
	int pid;

	if(pidfile==NULL || strlen(pidfile)==0){
		fprintf(stderr, "Error: [%s %s:%d] pid file is empty!!\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

	pid = read_pid(pidfile);
	if(pid>0){
		return 1;
	}

	return 0;
}

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
int waitProcessTerminate(char* pidfile, unsigned int timeout)
{
	clock_t time_end;

	if(pidfile==NULL || strlen(pidfile)==0){
		fprintf(stderr, "Error: [%s %s:%d] pid file is empty!!\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

	time_end = clock() + (timeout/10*10);
	while(checkProcess(pidfile)==1){
		if(timeout!=0){
			if(clock() > time_end){
				break;
			}
		}
		usleep(10);
	}
	return 0;
}

/************************************************
* Propose: rtk_isPorcessExist()
*   check is process exist with given name and customizable rules (e.g: process status, process runtime argument, etc.)
*
* Parameter:
*   char* name                    process executable file name
*   int (*match_fn)(char *line)   customizable match function
*                                 input:  a ps command result line, whose process name match argument 1.
*                                 output: -1: error, 0: not match, 1: match
*   pid_t* pid                    if there's process match, return the process pid, otherwise, it is not used.
*
* Return:
*       -1: internal error
*        0: process matching name and customizable rule is not running
*        1: process matching name and customizable rule is running
*
* Author:
*   patrick_cai
*************************************************/
int rtk_isPorcessExist(char *name, int (*match_fn)(char *line), pid_t *pid)
{
	FILE *fp = NULL;
	char cmd[256] = {0}, line[256] = {0};
	int exist = 0;
	pid_t tmp_pid = 0;

	if(!name || !pid)
		return -1;

	snprintf(cmd, sizeof(cmd), "ps | grep %s | grep -v grep", name);
	fp = popen(cmd, "r");
	if(fp)
	{
		while(fgets(line, sizeof(line), fp)!=NULL)
		{
			char buf1[256] = {0}, buf2[256] = {0}, *tmp = NULL, *exename = NULL;

			//match process name, may have <:
			//  a. 556 root      6012 S <  /bin/autoSimu
			//  b. 361 root       652 S    /bin/inetd
			sscanf(line, "%d %*s %*d %*s %s %s %*s", &tmp_pid, buf1, buf2);

			if(strcmp(buf1, "<")==0)
			{
				tmp = strrchr(buf2, '/');
				exename = tmp? (tmp+1): buf2;
			}else{
				tmp = strrchr(buf1, '/');
				exename = tmp? (tmp+1): buf1;
			}

			if(strcmp(exename, name))
				continue;

			//match (*match_fn)
			if(!match_fn || match_fn(line)==1)
			{
				exist = 1;
				break;
			}
		}
		pclose(fp);
	}else{
		exist = -1;
	}

	if(exist)
		*pid = tmp_pid;

	return exist;
}

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
int getMacAddr(char* ifname, unsigned char* macaddr)
{
	int s;
	struct ifreq ifr;

	if(ifname==NULL || strlen(ifname)==0 || macaddr==NULL){
		printf("%s: parameter error\n", __FUNCTION__);
        return -1;
	}

    s = socket(PF_PACKET, SOCK_DGRAM, 0);

    if (s < 0) {
        printf("%s: socket error\n", __FUNCTION__);
        return -1;
    }

	memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if (ioctl(s, SIOCGIFHWADDR, &ifr) < 0) {
        printf("%s: Interface %s not found\n", __FUNCTION__, ifname);
		close(s);
        return -1;
    }
	close(s);

	if (ifr.ifr_hwaddr.sa_family!=ARPHRD_ETHER) {
    	printf("%s: not an Ethernet interface\n", __FUNCTION__);
		return -1;
	}
	memcpy(macaddr, ifr.ifr_hwaddr.sa_data, MAC_ADDR_LEN);

	//printf("%s: %02X:%02X:%02X:%02X:%02X:%02X\n", __FUNCTION__,
    //	macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
    return 0;
}

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

int get_mac_by_ipaddr(struct in_addr *comp_addr, unsigned char mac[MAC_ADDR_LEN])
{
	int ret=-1;
	FILE *fp;
	char  buf[256];
	int Flags;
	char ip_addr[16], ip_addr2[16],tmp0[20],tmp1[32],tmp2[32];
	char ip_str[INET_ADDRSTRLEN];

	fp = fopen("/proc/net/arp", "r");
	if (fp == NULL){
		ret = -1;
		printf("read arp file fail!\n");
	}
	else {
		fgets(buf, 256, fp);//first line!?
		while(fgets(buf, 256, fp) !=NULL) {
			sscanf(buf,"%s  %*s 0x%x %s %s %s ", ip_addr, &Flags,tmp0,tmp1,tmp2);
			if (strstr(tmp2,BRIF)!=0)
				continue;
			strncpy(ip_addr2, inet_ntoa(*((struct in_addr *)comp_addr)), 16);
			if(strncmp(ip_addr, ip_addr2, sizeof(ip_addr))==0){
				ret = 0;
				convertMacFormat(tmp0, mac);
				break;
			}
		}
		fclose(fp);
	}
	return ret;
}

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
unsigned short netmask_to_cidr(char* ipAddress)
{
	unsigned short netmask_cidr;
	int ipbytes[4];
	int i=0;

	netmask_cidr=0;
	sscanf(ipAddress, "%d.%d.%d.%d", &ipbytes[0], &ipbytes[1], &ipbytes[2], &ipbytes[3]);

	for (i=0; i<4; i++)
	{
		switch(ipbytes[i])
		{
			case 0x80:
				netmask_cidr+=1;
				break;
			case 0xC0:
				netmask_cidr+=2;
				break;
			case 0xE0:
				netmask_cidr+=3;
				break;
			case 0xF0:
				netmask_cidr+=4;
				break;
			case 0xF8:
				netmask_cidr+=5;
				break;
			case 0xFC:
				netmask_cidr+=6;
				break;
			case 0xFE:
				netmask_cidr+=7;
				break;
			case 0xFF:
				netmask_cidr+=8;
				break;
			default:
				return netmask_cidr;
				break;
		}
	}
	return netmask_cidr;
}
/*
  unsigned int prefix2mask(int bits) - only for IPv4 not IPv6
  brief creates a netmask from a specified number of bits
  (i.e. 192.168.2.3/24, with a corresponding netmask 255.255.255.0).
  If you need to see what netmask corresponds to the prefix part of the address, this
  is the function.

  param bits is the number of bits to create a mask for.
  return a network mask, in network byte order.
*/
unsigned int cidr_to_netmask(unsigned char bits)
{
	struct in_addr mask;
	memset(&mask, 0, sizeof(mask));
	if (bits)
		return htonl(~((1 << (32 - bits)) - 1));
	else
		return htonl(0);
}


int doRecoverWanConneciton()
{
	unsigned int entryNum=0, i=0;
	char ifname[IFNAMSIZ]={0};
	char sVal[32]={0};
	MIB_CE_ATM_VC_T entry={0};

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry)){
			printf("Get chain record error!\n");
			return -1;
	}

		if (entry.enable == 0)
			continue;

		ifGetName(entry.ifIndex,ifname,sizeof(ifname));

		if(entry.ipDhcp == DHCP_CLIENT ){ //DHCPC
			int dhcpc_pid;
			printf("Renew DHCPClient ifname=%s\n",ifname);
			// DHCP Client
			snprintf(sVal, 32, "%s.%s", (char*)DHCPC_PID, ifname);
			dhcpc_pid = read_pid((char*)sVal);
			if (dhcpc_pid > 0) {
				kill(dhcpc_pid, SIGUSR1); // force renew
			}
		}
		else if(entry.cmode == CHANNEL_MODE_PPPOE){
			char *p_index= &ifname[3];
			unsigned int index= atoi(p_index);

			printf("Restart PPPoE client, ifname=%s, pppoe index=%d\n",ifname,index);
			sprintf(sVal,"/bin/spppctl up %d\n", index);
			system(sVal);
		}
	}
	sleep(1);
	return 0;
}


int getifIndexByWanName(const char *name)
{
	int index=0;
	int cnt=mib_chain_total(MIB_ATM_VC_TBL);
	MIB_CE_ATM_VC_T pvcEntry;
	for(index=0;index<cnt;index++)
		{
			char itfname[MAX_WAN_NAME_LEN]={0};
			if(!mib_chain_get(MIB_ATM_VC_TBL,index,&pvcEntry))
				continue;
			getWanName(&pvcEntry, itfname);
			if(!strncmp(name,itfname,strlen(name)))
				return pvcEntry.ifIndex;
		}
     return -1;
}

#ifdef CONFIG_USER_XML_STRING_ENCRYPTION
#include <openssl/conf.h>
#include <openssl/ossl_typ.h>
#include <openssl/evp.h>
#include <openssl/err.h>

static EVP_CIPHER_CTX *ctx = NULL;

/* Hard coded key & IV, FIXME if you have better idea. */
/* A 128 bit key */
unsigned char *key = (unsigned char *)"realtekrealtekre";
/* A 128 bit IV */
unsigned char *iv = (unsigned char *)"crabcrabcrabcrab";

static size_t str_encrypt_get_real_size(size_t plain_size)
{
	return (plain_size/16 + 1) * 16;
}

size_t rtk_xmlfile_str_encrypt_get_size(size_t plain_size)
{
	/* hex value + prefix "__ENC__\0" */
	return str_encrypt_get_real_size(plain_size) * 2 + 8;
}

int rtk_xmlfile_str_encrypt (const char *de_str, char *en_str)
{
	int k = 0;
	int len = 0, en_str_len = 0;
	size_t encrypted_size = 0;
	unsigned char *encrypted_data = NULL;
	int ret = 0;

	//printf("de_str %s\n", de_str);
	while (isprint (de_str[k])) k++;
	if (k != strlen (de_str)) {
		fprintf (stderr,
			"[ERR] %s(): Unprintable character detected!! The entry value written will be NULL.\n",
			__func__);
		return -1;
	}

	if (en_str == NULL) {
		fprintf (stderr,
			"[ERR] %s(): calloc() failed!! The entry value written will be NULL.\n",
			__func__);
		return -1;
	}

	encrypted_size = str_encrypt_get_real_size(strlen(de_str));
	encrypted_data = malloc(encrypted_size);
	if(!encrypted_data)
	{
		fprintf (stderr,
				"[ERR] %s(): malloc() failed!! The entry value written will be NULL.\n",
				__func__);
			return -1;
	}
	memset(encrypted_data, 0, encrypted_size);

	if(ctx == NULL)
	{
		if(!(ctx = EVP_CIPHER_CTX_new()))
		{
			ERR_print_errors_fp(stderr);
			ret =  -1;
			goto enc_failed;
		}
	}

	if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
	{
		ERR_print_errors_fp(stderr);
		ret =  -1;
		goto enc_failed;
	}

	if(1 != EVP_EncryptUpdate(ctx, encrypted_data, &len, de_str, strlen(de_str)))
	{
		ERR_print_errors_fp(stderr);
		ret =  -1;
		goto enc_failed;
	}

	if(1 != EVP_EncryptFinal_ex(ctx, encrypted_data + len, &len))
	{
		ERR_print_errors_fp(stderr);
		ret =  -1;
		goto enc_failed;
	}

	len = sprintf(en_str, "__ENC__");
	for(k = 0 ; k < encrypted_size ; k++)
	{
		len += sprintf(en_str+len, "%02X", encrypted_data[k]);
	}
	en_str[len] = '\0';

enc_failed:
	if(encrypted_data)
		free(encrypted_data);
	return ret;
}

int rtk_xmlfile_str_decrypt (const char *en_str, char *de_str)
{
	int k = 0;
	int len = 0;
	size_t encrypted_size = 0;
	unsigned char *encrypted_data = NULL;
    size_t decrypted_len = 0;
	int ret = 0;

	//printf("en_str %s\n", en_str);
	while (isprint (en_str[k])) k++;
	if (k != strlen (en_str)) {
		fprintf (stderr,
			"[ERR] %s():%d: Unprintable character detected!! The entry value written will be NULL.\n",
			__func__, __LINE__);
		return -1;
	}

	if (de_str == NULL) {
		fprintf (stderr,
			"[ERR] %s(): calloc() failed!! The entry value written will be NULL.\n",
			__func__);
		return -1;
	}

	if(strstr(en_str, "__ENC__") == NULL)
	{
		// Treat as normal string.
		strcpy(de_str, en_str);
		return 0;
	}
	en_str += 7;

	encrypted_size = strlen(en_str) / 2;
	encrypted_data = malloc(encrypted_size);
	if(!encrypted_data)
	{
		fprintf (stderr,
				"[ERR] %s():%d: Unprintable character detected!! The entry value written will be NULL.\n",
				__func__, __LINE__);
			return -1;
	}
	rtk_string_to_hex(en_str, encrypted_data, encrypted_size * 2);

	if(ctx == NULL)
	{
		if(!(ctx = EVP_CIPHER_CTX_new()))
		{
			ERR_print_errors_fp(stderr);
			ret =  -1;
			goto dec_failed;
		}
	}

	if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
	{
		ERR_print_errors_fp(stderr);
		ret =  -1;
		goto dec_failed;
	}

	if(1 != EVP_DecryptUpdate(ctx, de_str, &len, encrypted_data, encrypted_size))
	{
		ERR_print_errors_fp(stderr);
		ret =  -1;
		goto dec_failed;
	}
    decrypted_len += len;

	if(1 != EVP_DecryptFinal_ex(ctx, de_str + len, &len))
	{
		ERR_print_errors_fp(stderr);
		ret =  -1;
		goto dec_failed;
	}
    decrypted_len += len;
    de_str[decrypted_len] = '\0';

dec_failed:
	if(encrypted_data != NULL)
		free(encrypted_data);
	return ret;
}

#endif

/*
 *	Convert the mib data mib_buf, from mib_get_s(), to a string representation.
 *	Return:
 		string
 */
int mib_to_string_ex(char *string, const void *mib, TYPE_T type, int size, int encrypt)
{
	char tmp[16];
	int array_size, i, ret = 0;

	if(string != NULL) string[0] = '\0';
	if(string == NULL || mib == NULL){
		return -1;
	}

	switch (type) {
	case IA_T:
		if (inet_ntop(AF_INET, mib, string, INET_ADDRSTRLEN) == NULL) {
			string[0] = '\0';
			ret = -1;
		}
		break;
#ifdef CONFIG_IPV6
	case IA6_T:
		if (inet_ntop(AF_INET6, mib, string, INET6_ADDRSTRLEN) == NULL) {
			string[0] = '\0';
			ret = -1;
		}
		break;
#endif
	case BYTE_T:
		sprintf(string, "%hhu", *(unsigned char *)mib);
		break;

	case WORD_T:
		sprintf(string, "%hu", *(unsigned short *)mib);
		break;

	case DWORD_T:
		sprintf(string, "%u", *(unsigned int *)mib);
		break;

	case INTEGER_T:
		sprintf(string, "%d", *(int *)mib);
		break;

	case BYTE5_T:
	case BYTE6_T:
	case BYTE13_T:
		string[0] = '\0';
		for (i = 0; i < size; i++) {
			sprintf(tmp, "%02x", ((unsigned char *)mib)[i]);
			strcat(string, tmp);
		}
		break;

	case BYTE_ARRAY_T:
		string[0] = '\0';
		for (i = 0; i < size; i++) {
			sprintf(tmp, "%02x", ((unsigned char *)mib)[i]);
			if (i != (size - 1))
				strcat(tmp, ",");
			strcat(string, tmp);
		}
		break;

	case WORD_ARRAY_T:
		array_size = size / sizeof(short);

		string[0] = '\0';
		for (i = 0; i < array_size; i++) {
			sprintf(tmp, "%04x", ((unsigned short *)mib)[i]);

			if (i != (array_size - 1))
				strcat(tmp, ",");

			strcat(string, tmp);
		}
		break;

	case INT_ARRAY_T:
		array_size = size / sizeof(int);

		string[0] = '\0';
		for (i = 0; i < array_size; i++) {
			sprintf(tmp, "%08x", ((unsigned int *)mib)[i]);

			if (i != (array_size - 1))
				strcat(tmp, ",");

			strcat(string, tmp);
		}
		break;
	case STRING_T:
#ifndef CONFIG_USER_XML_STRING_ENCRYPTION
	case ENCRYPT_STRING_T:
#endif
		strcpy(string, mib);
		break;
#ifdef CONFIG_USER_XML_STRING_ENCRYPTION
	case ENCRYPT_STRING_T:
		if (encrypt==1 && size > 0) {
			char *tmp=NULL;
			size_t enc_size = rtk_xmlfile_str_encrypt_get_size(size);
			tmp = malloc(enc_size);
			if(tmp != NULL){
				memset(tmp, 0, size);
				rtk_xmlfile_str_encrypt(mib, tmp);
				strncpy(string, tmp, enc_size);
				free(tmp);
			}
		}
		else {
			strcpy(string, mib);
		}
		break;
#endif
	default:
		ret = -1;
		printf("%s: Unknown data type %d!\n", __FUNCTION__, type);
	}

	return ret;
}

int mib_to_string(char *string, const void *mib, TYPE_T type, int size)
{
	return mib_to_string_ex(string, mib, type, size, 0);
}

#ifdef RTK_SMART_ROAMING
#define READ_BUF_SIZE        50
#if 0
extern pid_t findPidByName( char* pidName)
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
		char filename[READ_BUF_SIZE];
		char buffer[READ_BUF_SIZE];
		char name[READ_BUF_SIZE];

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
		if (fgets(buffer, READ_BUF_SIZE-1, status) == NULL) {
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
#endif
#endif


/*return vale: count of delimimter*/

int is_delimiter(char ch)
{
	if(ch == ',' || ch == ' ' || ch == '\0')
		return 1;
	return 0;
}

int eat_delimiter(const char *string, int pos, int strlen)
{
	int count=0;
	while(pos <= strlen)
	{
		if(is_delimiter(string[pos]))
			count++;
		else
			break;
		pos++;
	}
	return count;
}

int copy_hex_str_as_one_int(char *tostr, int topos, const char *fromstr, int frompos, int cp_len)
{
	int unit_size;
	int i;
	unit_size = (sizeof(int)*2);
	if(cp_len < unit_size)
	{
		for(i=0;i<(unit_size-cp_len);i++)
			tostr[topos+i]='0';
	}
	memcpy(tostr+topos+(unit_size-cp_len),fromstr+frompos,cp_len);
	return 0;
}

/*
 *  this function should be comaptible with to_hex_string function
 *  to_hex_string only handle
 *	[hex(8)][delimiter][hex(8)][delimiter][hex(8)]...
 *  delimiter is  ',' or  ' ' or NULL)
 *  multiple delimiter is taken as single one
 *
 * 	to_hex_string_int can
 *  handle format:[hex(0~8)][delimiter][hex(0~8)][delimiter][hex(0~8)]...
 *  delimiter is ',' or  ' ' or NULL(but with full 8 hex)
 *  multiple delimiter is taken as single one
 *  eg:
 *  12345678,12345678,12345678, ...
 *  12345678 12345678 12345678  ...
 *  12345678 , , 12345678 12345678  ...
 *  xxx,xxx,xxx, ...
 *  xxx xxx xxx ...
 *  123456781234567812345678
 */
char *to_hex_string_int(const char *string, int maxstrlen)
{
	char *tostr=NULL;
	int orig_len, pos, hex_count, delimiter_count, from_pos, to_pos, unit_size;
	orig_len=strlen(string);
	pos=0;
	hex_count=0;
	from_pos = 0;
	to_pos = 0;
	unit_size = (sizeof(int)*2);

	tostr = malloc(maxstrlen + 1);
	if (tostr == NULL){
		return NULL;
	}
	memset(tostr,0,maxstrlen+1);

	while(pos <= orig_len)
	{
			delimiter_count = eat_delimiter(string,pos,orig_len);
			if(0 == delimiter_count)
			{
				pos++;
				hex_count++;
				if(hex_count == unit_size) {
					copy_hex_str_as_one_int(tostr,to_pos,string,from_pos,unit_size);
					to_pos += unit_size;
					from_pos += unit_size;
					hex_count = 0;
				}
			} else
			{
				if((pos-from_pos) > 0) {
					copy_hex_str_as_one_int(tostr,to_pos,string,from_pos,(pos-from_pos));
					to_pos += unit_size;
					hex_count = 0;
				}
				pos += delimiter_count;
				from_pos = pos;
			}
			if(to_pos > maxstrlen)
				break;
	}
	tostr[maxstrlen] = '\0';
	return tostr;
}
/*
 *	Remove comma and blank in the string
 */
char *to_hex_string(const char *string)
{
	char *tostr;
	int i, k;
	size_t len = strlen(string);

	tostr = malloc(len + 1);
	if (tostr == NULL)
		return NULL;

	for (i = 0, k = 0; i < len; i++) {
		if (string[i] != ',' && string[i] != ' ')
			tostr[k++] = string[i];
	}
	tostr[k] = '\0';

	return tostr;
}

int rtk_string_to_hex(const char *string, unsigned char *key, int len)
{
	char tmpBuf[4];
	int idx, ii = 0;

	if (string == NULL || key == NULL)
		return 0;

	for (idx = 0; idx < len; idx += 2) {
		tmpBuf[0] = string[idx];
		tmpBuf[1] = string[idx + 1];
		tmpBuf[2] = 0;

		if (!isxdigit(tmpBuf[0]) || !isxdigit(tmpBuf[1]))
			return 0;

		key[ii++] = strtoul(tmpBuf, NULL, 16);
	}

	return 1;
}

int string_to_short(const char *string, unsigned short *key, int len)
{
	char tmpBuf[5];
	int idx, ii = 0, j;

	if (string == NULL || key == NULL)
		return 0;

	for (idx = 0; idx < len; idx += 4) {
		for (j = 0; j < 4; j++) {
			tmpBuf[j] = string[idx + j];
			if (!isxdigit(tmpBuf[j]))
				return 0;
		}

		tmpBuf[4] = 0;

		key[ii++] = strtoul(tmpBuf, NULL, 16);
	}

	return 1;
}

int string_to_integer(const char *string, unsigned int *key, int len)
{
	char tmpBuf[9];
	int idx, ii = 0, j;

	if (string == NULL || key == NULL)
		return 0;

	for (idx = 0; idx < len; idx += 8) {
		for (j = 0; j < 8; j++) {
			tmpBuf[j] = string[idx + j];
			if (!isxdigit(tmpBuf[j]))
				return 0;
		}

		tmpBuf[8] = 0;

		key[ii++] = strtoul(tmpBuf, NULL, 16);
	}

	return 1;
}

int string_to_dec(char *string, int *val)
{
	int i;
	int len = strlen(string);

	for (i=0; i<len; i++)
	{
		if (!isdigit(string[i]))
			return 0;
	}

	*val = strtol(string, (char**)NULL, 10);
	return 1;
}


int string_to_mib_ex(void *mib, const char *string, TYPE_T type, int size, int decrypt)
{
	struct in_addr *ipAddr=NULL;
#ifdef CONFIG_IPV6
	struct in6_addr *ip6Addr=NULL;
#endif
	unsigned char *vChar=NULL;
	unsigned short *vShort=NULL;
	unsigned int *vUInt=NULL;
	int *vInt=NULL;
	int ret=0;

	ret = 0;
	switch (type) {
	case IA_T:
		if (!strlen(string)) {
			string = "0.0.0.0";
		}
		ipAddr = mib;
		if (inet_pton(AF_INET, string, ipAddr) == 0)
			ret = -1;
		break;
#ifdef CONFIG_IPV6
	case IA6_T:
		if (!strlen(string)) {
			string = "::";
		}
		ip6Addr = mib;
		if (inet_pton(AF_INET6, string, ip6Addr) == 0)
			ret = -1;
		break;
#endif
	case BYTE_T:
		vChar = mib;
		*vChar = strtoul(string, NULL, 0);
		break;
	case WORD_T:
		vShort = mib;
		*vShort = strtoul(string, NULL, 0);
		break;
	case DWORD_T:
		vUInt = mib;
		*vUInt = strtoul(string, NULL, 0);
		break;
	case INTEGER_T:
		vInt = mib;
		*vInt = strtol(string, NULL, 0);
		break;
	case BYTE_ARRAY_T:
		string = to_hex_string((char *)string);	// remove comma and blank
		rtk_string_to_hex((char *)string, mib, size * 2);
		free((char *)string);
		break;
	case BYTE5_T:
	case BYTE6_T:
	case BYTE13_T:
		rtk_string_to_hex((char *)string, mib, size * 2);
		break;
	case STRING_T:
#ifndef CONFIG_USER_XML_STRING_ENCRYPTION
	case ENCRYPT_STRING_T:
#endif
		strncpy(mib, string, size);
		break;
#ifdef CONFIG_USER_XML_STRING_ENCRYPTION
	case ENCRYPT_STRING_T:
		if (decrypt==1) {
			char *tmp=NULL;
			tmp = malloc(size);
			if(tmp != NULL){
				memset(tmp, 0, size);
				rtk_xmlfile_str_decrypt(string, tmp);
				strncpy(mib, tmp, size);
				free(tmp);
			}
		}
		else {
			strncpy(mib, string, size);
		}
		break;
#endif
	case WORD_ARRAY_T:
		string = to_hex_string((char *)string);	// remove comma and blank
		string_to_short((char *)string, mib, size * 2);
		free((char *)string);
		break;
	case INT_ARRAY_T:
		string = to_hex_string_int((char *)string, size * 2);//remove comma and blank
		string_to_integer((char *)string, mib, size * 2);
		free((char *)string);
		break;
	default:
		printf("Unknown data type !\n");
		break;
	}

	return ret;
}

int string_to_mib(void *mib, const char *string, TYPE_T type, int size)
{
	return string_to_mib_ex(mib, string, type, size, 0);
}

int before_upload(const char *fname)
{
	int ret = 1;

#ifdef CONFIG_USER_XMLCONFIG
	if (ret = va_cmd(shell_name, 3, 1, "/etc/scripts/config_xmlconfig.sh", "-s", fname)) {
		fprintf(stderr, "exec /etc/scripts/config_xmlconfig.sh error!\n");
	}
#else
#ifdef CONFIG_USE_XML
	if (ret = va_cmd("/bin/saveconfig", 4, 1, "-f", fname, "-t", "xml")) {
		fprintf(stderr, "exec /bin/saveconfig error!\n");
	}
#else
	if (ret = va_cmd("/bin/saveconfig", 4, 1, "-f", fname, "-t", "raw")) {
		fprintf(stderr, "exec /bin/saveconfig error!\n");
	}
#endif
#endif

	return ret;
}

int after_download(const char *fname)
{
	int ret = 1;

#ifdef CONFIG_USER_XMLCONFIG
	if (ret = va_cmd(shell_name, 3, 1, "/etc/scripts/config_xmlconfig.sh", "-l", fname)) {
		fprintf(stderr, "exec /etc/scripts/config_xmlconfig.sh error!\n");
	}
#else
#ifdef CONFIG_USE_XML
	if (ret = va_cmd("/bin/loadconfig", 4, 1, "-f", fname, "-t", "xml")) {
		fprintf(stderr, "exec /bin/loadconfig error!\n");
	}
#else
	if (ret = va_cmd("/bin/loadconfig", 4, 1, "-f", fname, "-t", "raw")) {
		fprintf(stderr, "exec /bin/loadconfig error!\n");
	}
#endif
#endif

	return ret;
}

#ifdef VOIP_SUPPORT
void reSync_reStartVoIP(void)
{
	char strbuf[128]={0};

	printf("Now calling voip_flash_server_update()\n");
	voip_flash_server_update();
	sprintf(strbuf, "echo x > /var/run/solar_control.fifo");
	va_cmd("/bin/sh", 2, 0, "-c", strbuf);
}
#endif

#ifdef CONFIG_00R0
int checkAndModifyWanServiceType(int serv_type_to_check, int change_type, int index)
{
	unsigned int totalEntry;
	MIB_CE_ATM_VC_T Entry;
	int i,ret;

	//if change_type is WAN_CHANGETYPE_ADD, just need to search ATM_VC_TBL found the one has the same applicationtype
	//but if change_type is WAN_CHANGETYPE_MODIFY, need to search ATM_VC_TBL found the one has the same applicationtype
	//but not with the same index.

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);

	for (i=0; i<totalEntry; i++) {
		if (mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			if((change_type==WAN_CHANGETYPE_MODIFY) && (i==index))
				continue;

			if(Entry.applicationtype & serv_type_to_check){
				printf("Wan %d, Entry.applicationtype changed from 0x%x to",i,Entry.applicationtype);
				Entry.applicationtype &= ~serv_type_to_check;
				if(Entry.applicationtype ==0 )
					Entry.applicationtype = X_CT_SRV_OTHER ;

				printf(" 0x%x\n",Entry.applicationtype);
				mib_chain_update(MIB_ATM_VC_TBL, (void *)&Entry, i);

				if(serv_type_to_check & X_CT_SRV_VOICE ){
					//If the modify service type is VOICE, need to sync share memory for VoIP used
					//and restart voip, so need special handle, return 1
					return 1; //Means voip need to do flash re-sync and restart
				}
			}
		}
	}
}

/* record the singal 1490 timer, use for wizard's requirement  */
#define SIGNAL_1490_END_TIMER "/var/setSignal1490_end_timer"
void setSignal1490_end_timer() //Record the time for receieve the SIGNAL1490 signals
{
	struct sysinfo signal_1490_info;
	sysinfo(&signal_1490_info);
	//printf(" setSignal1490_end_timer signal_1490_info.uptime = %d \n", signal_1490_info.uptime);
	int ret=0;
	char buf[64];
	sprintf(buf, "echo %ld > %s", signal_1490_info.uptime, SIGNAL_1490_END_TIMER);
	system(buf);
}

long getSignal1490_end_timer()//Report the time when receieved the SIGNAL1490 signals
{
	FILE *fp;
	int signal1490_timer=0;

	if(!(fp = fopen(SIGNAL_1490_END_TIMER, "r")))
		return 0;

	fscanf(fp, "%d\n", &signal1490_timer);
	fclose(fp);
	//printf(" getSignal1490_end_timer signal1490_timer = %d \n", signal1490_timer);

	return signal1490_timer;
}
#endif

#if 1
/* operations for bsd flock(), also used by the kernel implementation */
#define LOCK_SH		1	/* shared lock */
#define LOCK_EX		2	/* exclusive lock */
#define LOCK_NB		4	/* or'd with one of the above to prevent
				   blocking */
#define LOCK_UN		8	/* remove lock */

#define LOCK_MAND	32	/* This is a mandatory flock ... */
#define LOCK_READ	64	/* which allows concurrent read operations */
#define LOCK_WRITE	128	/* which allows concurrent write operations */
#define LOCK_RW		192	/* which allows concurrent read & write ops */

int unlock_file_by_flock(int lockfd)
{
	while (flock(lockfd, LOCK_UN) == -1 && errno==EINTR) {
		usleep(1000);
		printf("pkt loss write unlock failed by flock. errno=%d\n", errno);
	}
	close(lockfd);
	return 0;
}

int lock_file_by_flock(const char *filename, int wait)
{
	int lockfd;
#ifdef RESTRICT_PROCESS_PERMISSION
	umask(0000);
	if ((lockfd = open(filename, O_CREAT,0666)) == -1) {
		perror("open shm lockfile");
		return lockfd;
	}
#else
	if ((lockfd = open(filename, O_RDWR|O_CREAT,0666)) == -1) {
		perror("open shm lockfile");
		return lockfd;
	}
#endif
	if(wait)
	{
		 while (flock(lockfd, LOCK_EX) == -1 && errno==EINTR) {  //wait util lock can been use.
		 	usleep(1000);
		 	printf("file %s write lock failed by flock. errno=%d\n", filename, errno);
		 }
	}
	else
	{
		if(flock(lockfd, LOCK_EX|LOCK_NB) == -1)
		{
			close(lockfd);  //if failed to lock it. close lockfd.
			printf("fail to lock file %s\n", filename);
			return -1;
		}
	}

	return lockfd;
}
#endif

//#if defined(CONFIG_RTK_DEV_AP)
#define READ_BUF_SIZE	50
pid_t findPidByName( char* pidName)
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
		char filename[READ_BUF_SIZE];
		char buffer[READ_BUF_SIZE];
		char name[READ_BUF_SIZE];

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
		if (fgets(buffer, READ_BUF_SIZE-1, status) == NULL) {
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

int isFileExist(char *file_name)
{
	struct stat status;

	if ( stat(file_name, &status) < 0)
		return 0;

	return 1;
}

int if_readlist_proc(char *target, char *key, char *exclude)
{
	FILE *fh;
	char buf[512];
	char *s, name[16], tmp_str[16];
	int iface_num=0;
	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh) {
		return 0;
	}
	fgets(buf, sizeof buf, fh);	/* eat line */
	fgets(buf, sizeof buf, fh);
	while (fgets(buf, sizeof buf, fh)) {
		s = get_name(name, buf);

		#ifdef CONFIG_8021Q_VLAN_SUPPORTED
		if(strstr(name, "."))
			continue;
		#endif

		if(strstr(name, key)){
			iface_num++;
			if(target[0]==0x0){
				sprintf(target, "%s", name);
			}else{
				sprintf(tmp_str, " %s", name);
				strcat(target, tmp_str);
			}
		}
		//printf("iface name=%s, key=%s\n", name, key);
	}

	fclose(fh);
	return iface_num;
}
//#endif

#ifdef CONFIG_USER_MONITORD
/* action:
 *        0 - Remove process
 *        1 - Add process
 */
int update_monitor_list_file(char *process_name, int action)
{
	FILE *fp = NULL, *fp_new = NULL;
	char *tmpFileName = "/var/monitor_list_tmp";
	char *p = NULL;
	char p_name[16] = {0};
	int monitord_pid = 0, is_Find = 0;
	printf("[%s:%d] process_name = %s, action = %d\n", __FUNCTION__, __LINE__, process_name, action);

	int lock_fd = lock_file_by_flock(MONITOR_LIST_LOCK, 1);
	fp = fopen(MONITOR_LIST, "r");
	fp_new = fopen(tmpFileName, "w+");
	if (fp_new) {
		while (fp && fgets(p_name, sizeof(p_name), fp)) {
			p = strchr(p_name, '\n');
			if (p) {
				*p = '\0';
			}

			if (strncmp(p_name, process_name, strlen(process_name)) == 0) {
				is_Find = 1;
				if (action == 0) {
					continue;
				}
			}
			fprintf(fp_new, "%s\n", p_name);
		}

		if (action == 1 && is_Find == 0) {
			fprintf(fp_new, "%s\n", process_name);
		}
		fclose(fp_new);
	}

	if(fp) {
		fclose(fp);
		unlink(MONITOR_LIST);
	}

	if(rename(tmpFileName, MONITOR_LIST) !=0)
		 printf("Unable to rename the file: %s %d\n", __func__, __LINE__);
	unlock_file_by_flock(lock_fd);

	/* reload monitor list */
	monitord_pid = read_pid(MONITORD_PID);
	if (monitord_pid > 0) {
		kill(monitord_pid, SIGUSR1);
		usleep(1000);
	}

	return 0;
}

int startMonitord(int debug)
{
	int monitord_pid;

	monitord_pid = read_pid(MONITORD_PID);
	if (monitord_pid > 0) {
		kill(monitord_pid, SIGKILL);
	}

#ifdef CONFIG_USER_OSGI_API_SOCKET
	update_monitor_list_file("osgi_server", 1);
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#ifdef CONFIG_APACHE_FELIX_FRAMEWORK
	update_monitor_list_file("dns_msgserver", 1);
#endif
#endif

#if defined(CONFIG_USER_RTK_SYSTEMD)
	update_monitor_list_file("systemd", 1);
#endif

#if defined(CONFIG_USER_P0F)
	update_monitor_list_file("p0f", 1);
#endif

#ifdef CONFIG_CU_DPI
	update_monitor_list_file("informgetd", 1);
#endif

#ifdef CONFIG_USER_LANNETINFO
#ifdef CONFIG_HYBRID_MODE
	if (rtk_pon_get_devmode())
#endif
	update_monitor_list_file("lanNetInfo", 1);
#endif

#if defined(CONFIG_GPON_FEATURE)
	int ponMode=0;
	if (mib_get_s(MIB_PON_MODE, (void *)&ponMode, sizeof(ponMode)) != 0)
	{
		if (ponMode == GPON_MODE)
		{
			/*TBD, enable this after RTK_RG_Sync_OMCI_WAN_INFO be modified*/
			update_monitor_list_file("omci_app", 1);
		}
	}
#endif

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_CMCC)
#ifdef CONFIG_APACHE_FELIX_FRAMEWORK
	update_monitor_list_file("rsniffer", 1);
#endif
#endif

#ifdef USE_DEONET /* wchoule, 20241209 */
    if (deo_wan_nat_mode_get() == DEO_NAT_MODE)
        update_monitor_list_file("igmpproxy", 1);
#endif

	if(debug)
		va_niced_cmd("/bin/monitord", 1, 0, "-d");
	else
		va_niced_cmd("/bin/monitord", 0, 0);
}
#endif

unsigned char getOpMode(void)
{
	unsigned char op_mode=0;
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
	mib_get(MIB_OP_MODE,(void *)&op_mode);
#endif
	return op_mode;
}

int startDhcp(void)
{
	int ret_val;
	unsigned char dhcp_mode;
#ifdef CONFIG_AUTO_DHCP_CHECK
	int auto_dhcp_pid=0;

#endif

	mib_get(MIB_DHCP_MODE, (void *)&dhcp_mode);

#ifdef CONFIG_AUTO_DHCP_CHECK
	if(dhcp_mode==AUTO_DHCPV4_BRIDGE)
	{
		start_lan_Auto_DHCP_Check();
	}
	else{
		kill_by_pidfile_new((char*)AUTO_DHCPPID , SIGTERM);
	}

#endif

#ifdef CONFIG_USER_DHCP_SERVER
	if(dhcp_mode==DHCPV4_LAN_SERVER)
	{
		ret_val=setupDhcpd();
		if (ret_val == 1)
		{
			va_niced_cmd(DHCPD, 2, 0, "-S", DHCPD_CONF);
			start_process_check_pidfile((const char*)DHCPSERVERPID);
		}

	}
	else if(dhcp_mode==DHCPV4_LAN_RELAY)
	{
		startDhcpRelay();
	}
#endif

#ifdef CONFIG_USER_DHCPCLIENT_MODE
	else if(dhcp_mode==DHCPV4_LAN_CLIENT)
	{
		setupDHCPClient();
	}
#endif
	return 0;
}

#ifdef CONFIG_USER_RTK_BRIDGE_MODE
#ifdef CONFIG_AUTO_DHCP_CHECK
void start_lan_Auto_DHCP_Check()
{
	int pid;
	unsigned char opmode=0,dhcp_mode=0;
	mib_get(MIB_OP_MODE,(void *)&opmode);
	mib_get(MIB_DHCP_MODE, (void *)&dhcp_mode);
	pid = read_pid(AUTO_DHCPPID);
	if (pid > 0)
		return;
	if(opmode==BRIDGE_MODE && dhcp_mode == AUTO_DHCPV4_BRIDGE)
	{
		va_cmd("/bin/Auto_DHCP_Check", 8, 0, "-a", "15", "-n", "5", "-d", "30", "-m", "10");
		start_process_check_pidfile((const char*)AUTO_DHCPPID);
	}
}
#endif

int startBridgeApp()
{
	//cltsOnBRMode
#ifdef CONFIG_USER_DUMP_CLTS_INFO_ON_BR_MODE
	va_niced_cmd(GET_BR_CLIENT, 0, 0);
#endif

#ifdef CONFIG_IPV6
#ifdef DHCPV6_ISC_DHCP_4XX
	//in case sever do not have RA
	do_dhcpc6_br((char*)BRIF, 0);
#endif
#endif
	//run dhcpd or dhcpc on br0
	startDhcp();

	//start boa
	if(read_pid("/var/run/boa.pid") < 0)
		va_niced_cmd(WEBSERVER, 0, 0);
	else{
		#ifdef RTK_WEB_HTTP_SERVER_BIND_DEV
		va_cmd("/bin/touch",1,0,RTK_TRIGGLE_BOA_REBIND_SOCK_FILE);
		#endif
	}

	//start arp monitor
#ifndef CONFIG_USER_LANNETINFO
	va_niced_cmd(ARP_MONITOR, 10, 1, "-F", "-d", "-t", "3", "-nf", "3", "-nt", "7", "-i", "br+");
#endif

#ifdef TIME_ZONE
#ifdef CONFIG_E8B
	restartNTP();
#else
	startNTP();
#endif
#endif

// start andlink
#ifdef CONFIG_ANDLINK_SUPPORT
	startAndlink();
#endif

//start elink
#ifdef CONFIG_ELINK_SUPPORT
	startElink();
#endif

#ifdef CONFIG_USER_CTCAPD
	rtk_send_signal_to_daemon(CTCAPD_RUNFILE, SIGUSR1);
#endif

#ifdef CONFIG_USER_CTC_EOS_MIDDLEWARE
	start_eos_middleware();
#endif

#ifdef CONFIG_USER_DM
	restart_dm();
#endif

#ifdef CONFIG_USER_P0F
	restart_p0f();
#endif

#ifdef CONFIG_USER_LANNETINFO
	restartlanNetInfo();
#endif

#ifdef CONFIG_USER_MDNS
	restartMdns();
#endif


	return 0;
}

int setBridgeFirewall()
{
	char skbmark_buf[64]={0};

#ifdef WLAN_SUPPORT
#ifdef CONFIG_GUEST_ACCESS_SUPPORT
	int orig_wlan_idx=0, i, j;
	char ifname[16]={0};
	MIB_CE_MBSSIB_T mbssid_entry;
#endif
#endif
#ifdef REMOTE_ACCESS_CTL
	unsigned char dhcpvalue[32];
	int dhcpmode;
	unsigned char vChar;
#endif
	#if defined(CONFIG_USER_RTK_VLAN_PASSTHROUGH)
	unsigned char vlanpass_enable = 0;
	#endif

	cleanAllFirewallRule();

#if defined(CONFIG_USER_RTK_BRIDGE_MACFILTER_SUPPORT)
	setup_br_macfilter();
#endif

#if defined(CONFIG_RTK_CTCAPD_FILTER_SUPPORT)
	va_cmd(IPTABLES, 12, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-i", BRIF, "-p", "tcp", "-m", "conntrack", "--ctstate", "ESTABLISHED", "-j", WEBURL_CHAIN);
	va_cmd(IPTABLES, 2, 1, "-F", WEBURL_CHAIN);
	va_cmd(IPTABLES, 2, 1, "-X", WEBURL_CHAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 12, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-i", BRIF, "-p", "tcp", "-m", "conntrack", "--ctstate", "ESTABLISHED", "-j", WEBURL_CHAIN);
	va_cmd(IP6TABLES, 2, 1, "-F", WEBURL_CHAIN);
	va_cmd(IP6TABLES, 2, 1, "-X", WEBURL_CHAIN);
#endif
	va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-i", BRIF, "-p", "tcp", "-j", WEBURLPRE_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", WEBURLPRE_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", WEBURLPRE_CHAIN);
	va_cmd(IPTABLES, 11, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "!", "-o", BRIF, "-p", "tcp", "-j", WEBURLPOST_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", WEBURLPOST_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", WEBURLPOST_CHAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 10, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-i", BRIF, "-p", "tcp", "-j", WEBURLPRE_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", WEBURLPRE_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", WEBURLPRE_CHAIN);
	va_cmd(IP6TABLES, 11, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "!", "-o", BRIF, "-p", "tcp", "-j", WEBURLPOST_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", WEBURLPOST_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", WEBURLPOST_CHAIN);
#endif

	rtk_restore_ctcapd_filter_state();
	set_url_filter();
	rtk_set_dns_filter_rules_for_ubus();
#endif
#ifdef CONFIG_RTK_DNS_TRAP
#ifdef CONFIG_RTK_SKB_MARK2

	snprintf(skbmark_buf, sizeof(skbmark_buf), "0x%x", (1<<SOCK_MARK2_FORWARD_BY_PS_START));

	//iptables -t mangle -A PREROUTING -p udp --dport 53 -j MARK2 --or-mark 0x1
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);
	//iptables -t mangle -A PREROUTING -p udp --sport 53 -j MARK2 --or-mark 0x1
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -A PREROUTING -p udp --dport 53 -j MARK2 --or-mark 0x1
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -A PREROUTING -p udp --sport 53 -j MARK2 --or-mark 0x1
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);

#else

	snprintf(skbmark_buf, sizeof(skbmark_buf), "0x%x", (1<<SOCK_MARK_METER_INDEX_END));

	//iptables -t mangle -A PREROUTING -p udp --dport 53 -j MARK --or-mark 0x80000000
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);
	//iptables -t mangle -A PREROUTING -p udp --sport 53 -j MARK --or-mark 0x80000000
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -A PREROUTING -p udp --dport 53 -j MARK --or-mark 0x80000000
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -A PREROUTING -p udp --sport 53 -j MARK --or-mark 0x80000000
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);

#endif
#endif

#ifdef WLAN_SUPPORT
#ifdef CONFIG_GUEST_ACCESS_SUPPORT
	orig_wlan_idx = wlan_idx;

	for(i = 0; i<NUM_WLAN_INTERFACE; i++){
		for (j=0; j<(WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM); j++){
			memset(&mbssid_entry,0,sizeof(MIB_CE_MBSSIB_T));
			if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&mbssid_entry) == 0)
				printf("[%s %d]Error! Get MIB_MBSSIB_TBL error.\n",__FUNCTION__,__LINE__);

			if(mbssid_entry.guest_access && mbssid_entry.userisolation){
				rtk_wlan_get_ifname(i, j, ifname);
				va_cmd(IPTABLES, 10, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-m", "physdev", "--physdev-in", ifname, "-p", "tcp", "-j", (char *)FW_DROP);
			}
		}
	}

	wlan_idx = orig_wlan_idx;
#endif
#endif

#if defined(CONFIG_USER_CTCAPD)
	char cmdBuf[64]={0};
	snprintf(cmdBuf, sizeof(cmdBuf), "echo 1 > %s", CTCAPD_FW_FLAG_FILE);
	va_cmd("/bin/sh", 2, 1, "-c", cmdBuf);

	rtk_set_wifi_permit_rule();
#endif
#if defined(CONFIG_SECONDARY_IP)
	rtk_fw_set_lan_ip2_rule();
#endif
#if defined (CONFIG_IPV6) && defined(CONFIG_SECONDARY_IPV6)
	rtk_fw_set_lan_ipv6_2_rule();
#endif

#ifdef CONFIG_USER_LOCK_NET_FEATURE_SUPPORT
	va_cmd(EBTABLES, 2, 1, "-N", (char *)BR_NETLOCK_INPUT);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)BR_NETLOCK_INPUT, (char *)FW_RETURN);
	// ebtables -I INPUT -i nas+ -j Br_NetLock_Input
	va_cmd(EBTABLES, 6, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-i", "nas+", "-j", (char *)BR_NETLOCK_INPUT);

	va_cmd(EBTABLES, 2, 1, "-N", (char *)BR_NETLOCK_FWD);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)BR_NETLOCK_FWD, (char *)FW_RETURN);
	// ebtables -I FORWARD -j Br_NetLock_Fwd
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", (char *)BR_NETLOCK_FWD);

	va_cmd(EBTABLES, 2, 1, "-N", (char *)BR_NETLOCK_OUTPUT);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)BR_NETLOCK_OUTPUT, (char *)FW_RETURN);
	// ebtables -I OUTPUT -o nas+ -j Br_NetLock_Output
	va_cmd(EBTABLES, 6, 1, (char *)FW_INSERT, (char *)FW_OUTPUT, "-o", "nas+", "-j", (char *)BR_NETLOCK_OUTPUT);

	//flush net lock rules when switch opmode
	rtk_set_net_locked_rules(0);
#endif

#ifdef RTK_MULTI_AP
	uint8_t 	   map_state = 0;
	mib_get(MIB_MAP_CONTROLLER, (void *)&map_state);
	rtk_setup_multi_ap_block(map_state);
#endif

#ifdef REMOTE_ACCESS_CTL
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_INACC);
	va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_INACC,
		 "!", (char *)ARG_I, LANIF, "-m", "mark", "--mark", RMACC_MARK, "-j", FW_ACCEPT);

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	//va_cmd(IPTABLES, 11, 1, "-A", (char *)FW_INACC,
	//	 "!", (char *)ARG_I, LANIF, "-m", "mark", "--mark", RMACC_MARK, "-j", FW_ACCEPT);

	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_IN_COMMING);
	va_cmd(IPTABLES, 4, 1, "-A", (char *)FW_INACC, "-j", (char *)FW_IN_COMMING);
#endif
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_INACC);
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_IN_COMMING);
	va_cmd(IP6TABLES, 4, 1, "-A", (char *)FW_INPUT, "-j", (char *)FW_IN_COMMING);
#endif
#endif

	filter_set_remote_access(1);

#ifdef CONFIG_USER_DHCP_SERVER
	// Added by Mason Yu for dhcp Relay. Open DHCP Relay Port for Incoming Packets.
	if (mib_get_s(MIB_DHCP_MODE, (void *)dhcpvalue, sizeof(dhcpvalue)) != 0)
	{
		dhcpmode = (unsigned int)(*(unsigned char *)dhcpvalue);
		if (dhcpmode == 1 || dhcpmode == 2 ){
			// iptables -A inacc -i ! br0 -p udp --dport 67 -j ACCEPT
			va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_INACC, "!", (char *)ARG_I, BRIF, "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_ACCEPT);
		}
	}
#endif

#ifdef CONFIG_USER_ROUTED_ROUTED
	// Added by Mason Yu. Open RIP Port for Incoming Packets.
	if (mib_get_s( MIB_RIP_ENABLE, (void *)&vChar, sizeof(vChar)) != 0)
	{
		if (1 == vChar)
		{
			// iptables -A inacc -i ! br0 -p udp --dport 520 -j ACCEPT
			va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_INACC, "!", "-i", BRIF, "-p", "udp", (char *)FW_DPORT, "520", "-j", (char *)FW_ACCEPT);
		}
	}
#endif

	// iptables -A INPUT -j inacc
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_INACC);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_INACC);
#endif
	// End of REMOTE_ACCESS_CTL

#elif defined(IP_ACL)
		//	Add Chain(aclblock) for ACL
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_ACL);
	// Add chain(aclblock) on nat
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)FW_ACL);
	// Add chain(aclblock) on nat
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", (char *)FW_ACL);
	restart_acl();
#ifdef CONFIG_00R0
	/* Only take effect on WAN */
	//iptables -A INPUT ! -i br+ -j aclblock
	va_cmd(IPTABLES, 7, 1, (char *)FW_ADD, (char *)FW_INPUT, "!", "-i", "br+", "-j", (char *)FW_ACL);
	// iptables -t nat -A PREROUTING -j aclblock
	va_cmd(IPTABLES, 9, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "!", "-i", "br+", "-j", (char *)FW_ACL);
	// iptables -t mangle -A PREROUTING -j aclblock
	va_cmd(IPTABLES, 9, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "!", "-i", "br+", "-j", (char *)FW_ACL);
#else
	//iptables -A INPUT -j aclblock
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_ACL);
	// iptables -t nat -A PREROUTING -j aclblock
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)FW_ACL);
	// iptables -t mangle -A PREROUTING -j aclblock
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)FW_ACL);
#endif
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_ACL);
	restart_v6_acl();
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_ACL);
#endif
#endif

	//bridge mode nas0 need drop tagged pkts when disable vlan passthrough
	#if defined(CONFIG_USER_RTK_VLAN_PASSTHROUGH)
	if (!mib_get( MIB_VLANPASS_ENABLED,  (void *)&vlanpass_enable)){
        printf("get vlan pass enable failed!\n");
    }
	if(vlanpass_enable == 0)
	#endif
		va_cmd(EBTABLES, 8, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-i", "nas0", "-p", "802_1Q", "-j", "DROP");

	return 0;
}

int stopBridge(void)
{
#ifdef WLAN_MBSSID
	char para2[20];
#endif
	int status=0, i;

	for(i=0;i<ELANVIF_NUM;i++)
	{
		status|=va_cmd(IFCONFIG, 2, 1, ELANVIF[i], "down");
		status|=va_cmd(IFCONFIG, 2, 1, ELANRIF[i], "down");
	}

#ifdef WLAN_SUPPORT
#if defined(TRIBAND_SUPPORT)
{
    int k, orig_wlan_idx;
    orig_wlan_idx = wlan_idx;
    for (k=0; k<NUM_WLAN_INTERFACE; k++)
	{
        wlan_idx = k;
        va_cmd(IFCONFIG, 2, 1, WLANIF[k], "down");
#ifdef WLAN_MBSSID
        for (i=1; i<=WLAN_MBSSID_NUM; i++)
		{
            sprintf(para2, VAP_IF, k, i-1);
            va_cmd(IFCONFIG, 2, 1, para2, "down");
        }
#endif
    }
    wlan_idx = orig_wlan_idx;
}
#else /* !defined(TRIBAND_SUPPORT) */
	va_cmd(IFCONFIG, 2, 1, WLANIF[0], "down");

#ifdef WLAN_MBSSID
	// Set macaddr for VAP
	for (i=1; i<=WLAN_MBSSID_NUM; i++)
	{
		sprintf(para2, "wlan0-" VAP_IF_ONLY, i-1);
		va_cmd(IFCONFIG, 2, 1, para2, "down");
	}
#endif
#if defined(WLAN_DUALBAND_CONCURRENT)
	va_cmd(IFCONFIG, 2, 1, WLANIF[1], "down");
#ifdef WLAN_MBSSID
	// Set macaddr for VAP
	for (i=1; i<=WLAN_MBSSID_NUM; i++)
	{
		sprintf(para2, "wlan1-" VAP_IF_ONLY, i-1);
		va_cmd(IFCONFIG, 2, 1, para2, "down");
	}
#endif
#endif //WLAN_DUALBAND_CONCURRENT
#endif /* defined(TRIBAND_SUPPORT) */
#endif
#ifdef CONFIG_IPV6
	char cmdbuf[100]={0};
	snprintf(cmdbuf, sizeof(cmdbuf),"echo 0 > /proc/sys/net/ipv6/conf/%s/autoconf", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
	snprintf(cmdbuf, sizeof(cmdbuf),"echo 1 > /proc/sys/net/ipv6/conf/%s/forwarding", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
	snprintf(cmdbuf, sizeof(cmdbuf),"echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
	snprintf(cmdbuf, sizeof(cmdbuf),"echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
#endif

	status|=va_cmd(IFCONFIG, 2, 1, BRIF, "down");
	status|=va_cmd(BRCTL, 2, 1, "delbr", (char*)BRIF);

	return status;
}

int startBridge(void)
{
	int i;
	unsigned char value[6]={0};
	unsigned char macaddr[13]={0};

	va_cmd(IFCONFIG, 2, 1, ELANIF, "up");
	for(i=0;i<ELANVIF_NUM;i++)
	{
		va_cmd(IFCONFIG, 2, 1, ELANVIF[i], "up");
		va_cmd(IFCONFIG, 2, 1, ELANRIF[i], "up");
	}

#ifndef CONFIG_RTL_MULTI_PHY_ETH_WAN
	va_cmd(IFCONFIG, 2, 1, ALIASNAME_NAS0, "down");
	mib_get(MIB_ELAN_MAC_ADDR, (void *)value);
	snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
	value[0], value[1], value[2], value[3], value[4], value[5]);
	// ifconfig wan interface hw ether
	va_cmd(IFCONFIG, 4, 1, ALIASNAME_NAS0, "hw", "ether", macaddr);
	//brctl addif br0 wan interface
	va_cmd(BRCTL, 3, 1, "addif", BRIF, ALIASNAME_NAS0);
	va_cmd(IFCONFIG, 2, 1, ALIASNAME_NAS0, "up");
#endif

#ifdef CONFIG_IPV6
	char cmdbuf[100] = {0};
	//Next RA will update this proc depends on M/O, so set to 0 by default
	snprintf(cmdbuf, sizeof(cmdbuf),"echo 0 > /proc/sys/net/ipv6/conf/%s/autoconf", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
	snprintf(cmdbuf, sizeof(cmdbuf),"echo 1 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
	snprintf(cmdbuf, sizeof(cmdbuf),"echo 1 > /proc/sys/net/ipv6/conf/%s/forwarding", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
	snprintf(cmdbuf, sizeof(cmdbuf),"echo 2 > /proc/sys/net/ipv6/conf/%s/accept_ra", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
#endif
#ifdef CONFIG_USER_CTCAPD
	unsigned char wan_mac[MAC_ADDR_LEN]={0};
	rtk_wan_get_mac_by_ifname((const char *)WAN_IF, (char *)wan_mac);
	if(memcmp(wan_mac, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LEN)!=0)
	{
		va_cmd(IFCONFIG, 2, 1, BRIF, "down");

		snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
		wan_mac[0], wan_mac[1], wan_mac[2], wan_mac[3], wan_mac[4], wan_mac[5]);
		va_cmd(IFCONFIG, 4, 1, BRIF, "hw", "ether", macaddr);

		va_cmd(IFCONFIG, 2, 1, BRIF, "up");
	}
#endif
#ifdef CONFIG_RTL_SMUX_DEV   //for bridge mode wan use nas0 to rx/tx
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	int logic_port = rtk_port_get_default_wan_logicPort();
	if (logic_port >= 0)
		va_cmd(SMUXCTL, 9, 1, "--if", ELANRIF[logic_port], "--set-if-rsmux", "--set-if-rx-policy", "CONTINUE",
			"--set-if-tx-policy", "CONTINUE", "--set-if-rxmc-policy", "CONTINUE" );
#else
		va_cmd(SMUXCTL, 9, 1, "--if", ALIASNAME_NAS0, "--set-if-rsmux", "--set-if-rx-policy", "CONTINUE",
										"--set-if-tx-policy", "CONTINUE", "--set-if-rxmc-policy", "CONTINUE" );
#endif
#endif

	setBridgeFirewall();

	startBridgeApp();

#ifdef WLAN_SUPPORT
	//startWLan(CONFIG_WLAN_ALL, CONFIG_SSID_ALL);
	config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
#endif

#ifdef CONFIG_USER_AP_BRIDGE_MODE_QOS
	take_qos_effect();
#endif

	return 0;
}


int killBridgeApp()
{
	char pid_file[64]={0};

#ifdef CONFIG_IPV6
	char cmdbuf[100] = {0};
#ifdef DHCPV6_ISC_DHCP_4XX
	del_dhcpc6_br((char *)BRIF);
#endif
#ifdef CONFIG_USER_RTK_RAMONITOR
	snprintf(cmdbuf, sizeof(cmdbuf), "%s_%s", (char *)RA_INFO_FILE, (char*)BRIF);
	unlink(cmdbuf);
#endif
#endif

#ifdef CONFIG_AUTO_DHCP_CHECK
	kill_by_pidfile_new((const char*)AUTO_DHCPPID , SIGTERM);
#endif

	//kill dhcpd
	kill_by_pidfile_new((const char*)DHCPSERVERPID , SIGTERM);
	unlink(DHCPD_LEASE);

	//kill dhcpc run on br0
	snprintf(pid_file, sizeof(pid_file), "%s.%s", (char*)DHCPC_PID, BRIF);
	kill_by_pidfile_new((const char*)pid_file , SIGTERM);

	//kill spppd
#ifdef CONFIG_PPP
	kill_by_pidfile_new((const char*)PPP_PID , SIGTERM);
#endif

#if defined(CONFIG_USER_DNSMASQ_DNSMASQ)

#ifdef CONFIG_USER_MONITORD
	update_monitor_list_file((char *)DNSRELAY_PROCESS, 0);
#endif

	kill_by_pidfile_new((const char*)DNSRELAYPID , SIGTERM);
	if(!access(RESOLV, F_OK))
		unlink(RESOLV);
#endif

#if defined(DHCPV6_ISC_DHCP_4XX) || defined (CONFIG_IPV6)
	kill_by_pidfile_new((const char*)DHCPSERVER6PID , SIGTERM);

	if ((findPidByName("dhclient")) > 0)
		va_cmd("/bin/killall",1,1,"dhclient");
#endif

	//kill arp monitor
#ifndef CONFIG_USER_LANNETINFO
	kill_by_pidfile_new((const char*)ARP_MONITOR_RUNFILE , SIGTERM);
#endif

	//kill vsntp
#ifdef TIME_ZONE
	kill_by_pidfile_new((const char*)SNTPC_PID , SIGTERM);
#endif

	//kill lanNetInfo
#ifdef CONFIG_USER_LANNETINFO
	kill_by_pidfile_new((const char*)LANNETINFO_RUNFILE , SIGTERM);
#endif

	//cltsOnBRMode
#ifdef CONFIG_USER_DUMP_CLTS_INFO_ON_BR_MODE
	kill_by_pidfile_new((const char*)GET_BR_CLIENT_RUNFILE , SIGTERM);
#endif

	//kill igmpproxy
	kill_by_pidfile_new((const char*)IGMPPROXY_PID , SIGTERM);

	//kill mldproxy
#ifdef CONFIG_USER_MLDPROXY
	kill_by_pidfile_new((const char*)MLDPROXY_PID , SIGTERM);
#endif

	//kill radvd
#if defined(DHCPV6_ISC_DHCP_4XX) || defined(CONFIG_USER_RADVD)
	kill_by_pidfile_new((const char*)RADVD_PID , SIGTERM);
#endif

#ifdef CONFIG_SUPPORT_AUTO_DIAG
	kill_by_pidfile_new("/var/run/autoSimu.pid" , SIGTERM);
#endif

	return 0;
}

int cleanSystem(void)
{
	cleanAllFirewallRule();

	if(getOpMode()!=BRIDGE_MODE)
		deleteConnection(CONFIGALL, NULL);

#if defined(CONFIG_USER_RTK_MULTI_BRIDGE_WAN)
	deleteBrWanConnection(CONFIGALL,NULL);
#endif
#ifdef WLAN_SUPPORT
	config_WLAN(ACT_STOP, CONFIG_SSID_ALL);
#endif
	stopBridge();
	killBridgeApp();

#ifdef USE_LOGINWEB_OF_SERVER
	va_cmd("/bin/touch",1,0,RTK_CLEAR_BOA_LOGIN_INFO_FILE);
#endif

	return 0;
}

int reStartSystem(void)
{
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	rtk_port_update_wan_phyMask();
	rtk_update_br_interface_list();
#endif
#ifdef CONFIG_USER_WAN_PORT_AUTO_SELECTION
	rtk_update_lan_wan_binding();
#endif

	va_cmd("/bin/startup",0,0);
	return 0;
}
#endif

#if 1 //def CONFIG_RTK_DEV_AP
int startTimelycheck(void)
{
	va_cmd("killall", 2, 1, "-9", "timelycheck");
	va_cmd("timelycheck", 0, 0);

	return 0;
}
#endif

#ifdef CONFIG_USER_LLMNR
int restartLlmnr(void)
{
	unsigned char vChar;
	char domain_str[MAX_NAME_LEN];

	kill_by_pidfile_new((const char*)LLMNR_PID, SIGTERM);
	mib_get_s(MIB_LLMNR_ENABLE, (void *)&vChar, sizeof(vChar));

	if(1 == vChar)
	{
		mib_get_s(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)domain_str, sizeof(domain_str));
#ifdef CONFIG_IPV6
		va_cmd("/bin/llmnrd", 6, 0, "-H", domain_str, "-i", ALIASNAME_BR0 , "-d", "-6");
#else
		va_cmd("/bin/llmnrd", 5, 0, "-H", domain_str, "-i", ALIASNAME_BR0 , "-d");
#endif
	}

	return 0;
}
#endif

#ifdef CONFIG_USER_MDNS
int restartMdns(void)
{
	unsigned char vChar;
	char domain_str[MAX_NAME_LEN];

	kill_by_pidfile_new((const char*)MDNS_PID, SIGTERM);
	mib_get_s(MIB_MDNS_ENABLE, (void *)&vChar, sizeof(vChar));

	if(1 == vChar)
	{
		mib_get_s(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)domain_str, sizeof(domain_str));
		va_cmd("/bin/mdnsd", 4, 0, "-i", ALIASNAME_BR0, "-n", domain_str);
	}

	return 0;
}
#endif





#ifdef CONFIG_ANDLINK_SUPPORT
int startAndlink(void)
{
	unsigned char rtl_link_enable = 0;
	int pid = 0;

	if(isFileExist(RTL_LINK_PID_FILE)){
		pid = read_pid((char*)RTL_LINK_PID_FILE);
		if(pid > 0){
			kill(pid, 15);
		}
		unlink(RTL_LINK_PID_FILE);
	}

	if(isFileExist(ANDLINK_IF6_STATUS))
		unlink(ANDLINK_IF6_STATUS);

#ifdef CONFIG_ANDLINK_IF5_SUPPORT
	 //clean andlink_if5
	 if(isFileExist(RTL_LINK_IF5_PID_FILE)){
	 	pid = read_pid((char*)RTL_LINK_IF5_PID_FILE);
		if(pid > 0){
			kill(pid, 15);
		}
        unlink(RTL_LINK_IF5_PID_FILE);
    }
#endif

	mib_get(MIB_RTL_LINK_ENABLE, (void *)&rtl_link_enable);
	if(rtl_link_enable){
		if(!isFileExist(RTL_LINK_PID_FILE)){
			sleep(2); //wait dhcpc get ip
			system("andlink -m1 -d2");
		}
	}

	return 0;
}
#if defined(CONFIG_USER_DHCP_SERVER)
void start_dhcpd_with_ip_Andlink(char *ifname, char *ip, char *ip_start, char *ip_end)
{
	int ret = 0;
	char cmd[50] = {0};

	ret = setupDhcpd_Andlink(ifname, ip, ip_start, ip_end);

	if (ret == 1){
		snprintf(cmd, sizeof(cmd), "ifconfig %s %s", ifname, ip);
		system(cmd);
		va_niced_cmd(DHCPD, 2, 0, "-S", DHCPD_CONF);
	}

	return;
}
#endif
#endif

#ifdef CONFIG_USER_ANDLINK_PLUGIN
//should match romfs/etc/hynetwork.conf
void stop_andlink_plugin()
{
    unsigned char cmd[128] = {0};

    snprintf(cmd, sizeof(cmd), "rm -f %s", ANDLINK_IF6_STATUS);
    printf("%s\n", cmd);
    system(cmd);

    snprintf(cmd, sizeof(cmd), "killall managePlugin");
    printf("%s\n", cmd);
    system(cmd);

    snprintf(cmd, sizeof(cmd), "killall dpiPlugin");
    printf("%s\n", cmd);
    system(cmd);
}

void start_andlink_plugin(void)
{
    unsigned char cmd[128] = {0}, fname[64] = {0};

    snprintf(cmd, sizeof(cmd), "mkdir %s", ANDLINK_PLUGIN_ROOT_PATH);
    printf("%s\n", cmd);
    system(cmd);

    //make hynetwork.conf file writable
    snprintf(fname, sizeof(fname), "/%s/hynetwork.conf", ANDLINK_PLUGIN_ROOT_PATH);
    if(isFileExist(fname)==0)
    {
        //use default hynetwork.conf file
        snprintf(cmd, sizeof(cmd), "cp /etc/hynetwork.conf.default /%s/hynetwork.conf", ANDLINK_PLUGIN_ROOT_PATH);
        printf("%s\n", cmd);
        system(cmd);
    }

    snprintf(cmd, sizeof(cmd), "rm -f %s", ANDLINK_IF6_STATUS);
    printf("%s\n", cmd);
    system(cmd);

    snprintf(cmd, sizeof(cmd), "killall managePlugin");
    printf("%s\n", cmd);
    system(cmd);

    snprintf(cmd, sizeof(cmd), "/bin/managePlugin &");
    printf("%s\n", cmd);
    system(cmd);

    snprintf(cmd, sizeof(cmd), "killall dpiPlugin");
    printf("%s\n", cmd);
    system(cmd);

    snprintf(cmd, sizeof(cmd), "/bin/dpiPlugin &");
    printf("%s\n", cmd);
    system(cmd);
}
#endif

#ifdef CONFIG_USER_GREEN_PLUGIN
void stop_secRouter_plugin()
{
	int pid = 0;
	FILE *fp = NULL;
    unsigned char cmd[128] = {0};

	fp = popen("ps | grep \"/bin/sh /usr/bin/vasdocker_monitor\"", "r");
	if(fp)
	{
		fgets(cmd, sizeof(cmd), fp);
		pclose(fp);

		sscanf(cmd, "  %d %*s", &pid);
		printf("kill -SIGTERM %d\n", pid);
		kill(pid, SIGTERM);
	}

    snprintf(cmd, sizeof(cmd), "killall -SIGKILL vasdocker");
    printf("%s\n", cmd);
    system(cmd);

    snprintf(cmd, sizeof(cmd), "killall -SIGKILL netmonitor");
    printf("%s\n", cmd);
    system(cmd);
}

void start_secRouter_plugin(void)
{
	const char BR_CALL_IPT_FNAME[] = "/proc/sys/net/bridge/bridge-nf-call-iptables";
	const char BR_CALL_IP6T_FNAME[] = "/proc/sys/net/bridge/bridge-nf-call-ip6tables";
	const char NETMONITOR_PROG[] = "netmonitor";
	const char NETMONITOR_PATH[] = "/tmp/netmonitor";
	char cmdbuf[128] = {0};
	int enable = 0;

	/* prepare enable value:
	 * 1.for bridge mode, default set to 0 for performance concern. Plugin will set enable to 1 if needed;
	 * 2.for others, default as 1.
	 */
	if(getOpMode()==BRIDGE_MODE)
		enable = 0;
	else
		enable = 1;
	//set command
	snprintf(cmdbuf, sizeof(cmdbuf), "echo %d > %s", enable, BR_CALL_IPT_FNAME);
	printf("%s\n", cmdbuf);
	system(cmdbuf);

	snprintf(cmdbuf, sizeof(cmdbuf), "echo %d > %s", enable, BR_CALL_IP6T_FNAME);
	printf("%s\n", cmdbuf);
	system(cmdbuf);

	/*
	 * restart netmonitor
	 */
	snprintf(cmdbuf, sizeof(cmdbuf), "killall %s", NETMONITOR_PROG);
	printf("%s\n", cmdbuf);
	system(cmdbuf);

	snprintf(cmdbuf, sizeof(cmdbuf), "%s/%s &", NETMONITOR_PATH, NETMONITOR_PROG);
	printf("%s\n", cmdbuf);
	system(cmdbuf);

	return ;
}
#endif

void strtolower(char *str, int len)
{
    int i;
    for (i = 0; i<len; i++) {
        str[i] = tolower(str[i]);
    }
}

#ifdef CONFIG_RTK_DNS_TRAP
int create_hosts_file(char* domain_name)
{
    FILE* fp;
    fp = fopen(HOSTS_FILE,"w+");
    if(fp == NULL)
    {
        printf("Failed to open hosts file!!!!\n");
        return -1;
    }
    fprintf(fp,"# Do not remove the following line, or various programs\n");
    fprintf(fp,"# that require network functionality will fail.\n");
    fprintf(fp,"127.0.0.1   localhost.localdomain   localhost   rlx-linux\n");
    fprintf(fp,"127.0.0.1   %s\n",domain_name);
    fclose(fp);
    return 0;
}
#endif
/********************************added for elink start********************************/
#ifdef CONFIG_ELINK_SUPPORT
#ifdef CONFIG_ELINKSDK_SUPPORT
int rtl_ElinkCloudClient_start(void)
{
	int status = 0, ret=0;

	/* check elink cloud client alive */
	rtl_elinksdk_is_alive(&status);
	if(status == 1){
	    SDK_DEBUG("is alive!");
		return 0;
	}
//parttion mount
	ret = rtl_elinksdk_parttion_mount();
    SDK_DEBUG("rtl_elinksdk_parttion_mount result : %d\n", ret);
	if(ret != ELINKSDK_SUCCESS){
		return -1;
	}

	/* check elinksdk file */
	if(rtl_elinksdk_start_check() != ELINKSDK_SUCCESS){
	    ret = rtl_elinksdk_download(ELINKSDK_DOWNLOAD_FROM_INIT, NULL);
	    SDK_DEBUG("rtl_elinksdk_download result : %d\n", ret);
		if(ret != ELINKSDK_SUCCESS)
			return -1;
	}
	/* start elinksdk */
	rtl_elinksdk_restore_param();
	rtl_elinksdk_start();
	rtl_elinksdk_timelycheck_ops(ELINKSDK_TIMELYCHECK_START);

	return 0;
}
#endif
int startElink()
{
    unsigned char rtl_link_enabled;
    char tmpBuff[100];
	int pid=0;

	if(isFileExist(RTL_LINK_PID_FILE)){
			pid = read_pid((char*)RTL_LINK_PID_FILE);
			if(pid > 0){
				kill(pid, 15);
			}
			unlink(RTL_LINK_PID_FILE);
		}

#ifdef CONFIG_ELINKSDK_SUPPORT
	rtl_ElinkCloudClient_start();
#endif

    mib_get_s(MIB_RTL_LINK_ENABLE, (void *)&rtl_link_enabled,sizeof(rtl_link_enabled));
    if(rtl_link_enabled){
        if(!isFileExist(RTL_LINK_PID_FILE)){
#ifdef CONFIG_ELINK_SUPPORT
            system("elink -m1 -d2");
#endif
		}
    }

	return 0;
}
#endif  //CONFIG ELINK SUPPORT

// wlan_Schedule
#ifdef WLAN_SCHEDULE_SUPPORT
static void clean_wlan_schedule()
{
	va_cmd("killall", 2, 1, "-9", "wlan_schedule");
}
static void start_wlan_schedule()
{
	unsigned char cmdBuff[20]={0};

	snprintf(cmdBuff,20,"wlan_schedule &");

	system(cmdBuff);
}
int start_func_wlan_schedule()
{

	clean_wlan_schedule();

		start_wlan_schedule();

	return 0;
}
int restartWlanSchedule()
{
	return start_func_wlan_schedule();
}
#endif



/*
 * Unix domain socket with DGRAM APIs
 * Created by Rex_Chen@realtek.com
 */
/***********************************************************************/
/* Return value: -1 means send message failed.
 *                0 means success.
 */
int sendUnixSockMsg(unsigned char waitReply,unsigned int num, ...)
{
	va_list ap;
	int argc=0;
	char *argv[VA_CMD_ARG_SIZE];
	char *s;
	int ret=0;

	if(num < 1){
		fprintf(stderr, "<%s:%d> minimun argumet size should be 1, num=%d\n", __FUNCTION__, __LINE__, num);
		return -1;
	}

	if(num > VA_CMD_ARG_SIZE){
		fprintf(stderr, "<%s:%d> argumet size overflow, num=%d\n", __FUNCTION__, __LINE__, num);
		return -1;
	}

	argv[0] = strdup("send_unix_sock_message");	//compatibility with sysconf
	va_start(ap, num);
	for (argc=0; argc<num; argc++)
	{
		s = va_arg(ap, char *);
		argv[argc+1] = s;
	}

	argv[argc+1] = NULL;

#if 0
	int idx;
	fprintf(stderr, "waitReply=%d\n", waitReply);
	fprintf(stderr, "num=%d\n", num);
	for(idx=0; idx<argc; idx++){
		fprintf(stderr, "argv[%d]=%s\n", idx, argv[idx]);
	}
#endif

	ret=send_unix_sock_message(waitReply, argc+1, argv);
	va_end(ap);
	if(argv[0]) free(argv[0]);

	return (ret==0)?0:-1;
}

/* Return value: -1 means send message failed.
 *               -2 means recv message failed,
 *                0 means success.
 */
int send_unix_sock_message(unsigned char waitReply, int argc, char *argv[])
{
	usock_entry_t usock_entry;
	int usock_ret = 0, ret = 0, i = 0;
	int send_count = 0;
	fd_set fds;
	struct timeval tv;
	char cmd_buff[1024] = {0}, *cmd_buff_p = NULL;
	char buff[1024] = {0};
	int flags=0;

	cmd_buff_p = &cmd_buff[0];

	for (i = 2; i < argc; i++)
	{
		cmd_buff_p += sprintf(cmd_buff_p, "%s ", argv[i]);
	}
	*(cmd_buff_p - 1) = '\0';

	do {
		usock_ret = unixsock_open(0, &usock_entry, argv[1]);
		if (usock_ret < 0)
		{
			printf("Unable to open unix socket file %s!!!\n", argv[1]);
			ret = -1;
			break;
		}
		/* Send message to sock path */
		flags |= MSG_DONTWAIT;
		send_count = unixsock_send(&usock_entry, cmd_buff, strlen(cmd_buff) + 1, flags); //parameter 3 can be MSG_NOSIGNAL
		if (send_count < 0)
		{
			printf("%s: send error. %s.\n", __FUNCTION__, strerror(errno));
			ret = -1;
		}
		/* Receive messsage from that sock_path. Now just for confirm the send message be recevied with target path */
		else if(waitReply){
			tv.tv_sec = 5; // 5s
			tv.tv_usec = 0;
			while (1)
			{
				FD_ZERO(&fds);
				FD_CLR(unixsock_fd(&usock_entry), &fds);
				FD_SET(unixsock_fd(&usock_entry), &fds);

				ret = select(unixsock_fd(&usock_entry) + 1, &fds, NULL, NULL, &tv);
				if (ret == 0)
				{
					//printf("unixsock recv from target %s timeout !!\n", argv[1]);
					ret = -2;
					break;
				}
				else if (ret < 0)
				{
					if (errno == EINTR) continue;
					printf("%s: select error. %s.\n", __FUNCTION__, strerror(errno));
					ret = -2;
					break;
				}
				else if (FD_ISSET(unixsock_fd(&usock_entry), &fds))
				{
					ret = unixsock_recv(&usock_entry, buff, sizeof(buff), 0);
					if (ret > 0)
					{
						//printf("%s recv buf = %s\n", __FUNCTION__, buff);
						if(!strncmp(buff, "Do callback function failed!", strlen(buff)+1))
							ret=-2;
						else
							ret = 0;
						break;
					}
					else{
						printf("%s: recv error. %s.\n", __FUNCTION__, strerror(errno));
						ret=-2;
						break;
					}
				}
			}
		}
		unixsock_close(&usock_entry);
	} while(0);

	return ret;
}

int unixsock_fd(usock_entry_t *usock)
{
	if (usock == NULL) return -1;
	usock_entry_t *us = usock;
	return us->sock;
}

const char *unixsock_server_path(usock_entry_t *usock)
{
	if (usock == NULL) return NULL;
	usock_entry_t *us = usock;
	return us->server.sun_path;
}

const char *unixsock_client_path(usock_entry_t *usock)
{
	if (usock == NULL) return NULL;
	usock_entry_t *us = usock;
	return us->client.sun_path;
}

int unixsock_open(int is_server, usock_entry_t *entry, const char * name)
{
	do {
		/* Check socket path name */
		if (!name) break;

		/* Create Unix Socket */
		entry->sock = socket(AF_UNIX, SOCK_DGRAM, 0);
		if (entry->sock < 0) break;

		/* Prepare server sockaddr */
		entry->server.sun_family = AF_UNIX;
		snprintf(entry->server.sun_path, sizeof(entry->server.sun_path), "%s", name);

		/* If we are server, bind to server socket for incoming traffic. */
		if (is_server)
		{
			if (bind(entry->sock, (struct sockaddr *)&entry->server, sizeof(entry->server)) < 0) break;
		}
		else
		{
			/* If we are client, bind to client socket for incomming traffic and connect to server socket. */
			entry->client.sun_family = AF_UNIX;
			snprintf(entry->client.sun_path, sizeof(entry->client.sun_path), "%s_client_%d", name, getpid());
			if (bind(entry->sock, (struct sockaddr *)&entry->client, sizeof(entry->client)) < 0)
			{
				perror("Unix Socket bind failed!!!");
				break;
			}
			if (connect(entry->sock, (struct sockaddr *)&entry->server, sizeof(entry->server)) < 0)
			{
				perror("Unix Socket Connect to server failed!!!");
				break;
			}
			/* printf("%s: client entry->server pointer = %p\n", __func__, (struct sockaddr *)&entry->server); */
			/* printf("%s: client entry->client pointer = %p\n", __func__, (struct sockaddr *)&entry->client); */
		}
		entry->is_server = is_server;
		return 1;
	} while (0);

	if (entry->sock > 0)
	{
		close(entry->sock);
		entry->sock = -1;
	}

	unlink(is_server ? entry->server.sun_path : entry->client.sun_path);

	return -1;
}

int unixsock_close(usock_entry_t *usock)
{
	if (usock == NULL) return -1;

	if (usock->sock > 0)
	{
		close(usock->sock);
		usock->sock = -1;
	}
	unlink(usock->is_server ? usock->server.sun_path : usock->client.sun_path);

	return 0;
}

int unixsock_send(usock_entry_t *usock, const void *buf, unsigned int len, int flags)
{
	if (usock == NULL) return -1;

	if (usock->is_server)
	{
		if (strlen(usock->client.sun_path) == 0) return -1;
		return sendto(usock->sock, buf, len, flags, (struct sockaddr *)&usock->client, sizeof(usock->client));
	}
	return send(usock->sock, buf, len, flags);
}

int unixsock_recv(usock_entry_t *usock, void *buf, unsigned int len, int flags)
{
	if (usock == NULL) return -1;

	socklen_t fromlen;

	if (usock->is_server)
	{
		fromlen = sizeof(struct sockaddr_un);
		return recvfrom(usock->sock, buf, len, flags, (struct sockaddr *)&usock->client, &fromlen);
	}
	return recv(usock->sock, buf, len, flags);
}

int unixsock_recv_timed(usock_entry_t *usock, void *buf, unsigned int len, int flags, int timeout)
{
	int ret;
	struct timeval tv;
	fd_set fds;

	if (usock == NULL) return -1;

	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(unixsock_fd(usock), &fds);

	ret = select(unixsock_fd(usock) + 1, &fds, NULL, NULL, &tv);

	if (ret > 0) return unixsock_recv(usock, buf, len, flags);
	return ret;
}

void formatPloamPasswordToHex(char *src, char* dest, int length)
{
	char tmp[10] = {0};
	int i = 0;

	if(strlen(src) > length){
		return;
	}

	for(i=0; i < strlen(src) ; i++){
		sprintf(tmp, "%x", src[i]);
		strcat(dest, tmp);
	}

	if(strlen(src) < length){
		for(i=0; i < (length-strlen(src)); i++){
			strcat(dest, "00");
		}
	}
}

/***********************************************************************/
#ifdef HANDLE_DEVICE_EVENT
int rtk_sys_pushbtn_wps_action(int action, int btn_test, int diff_time)
{
	//printf("WPS button action %d, btn_test %d, diff_time %d\n", action, btn_test, diff_time);
	if (action == 0) {	// off
#ifdef WLAN_WPS_PUSHBUTTON_TRIGGER_ONE_INTERFACE
		if (diff_time >= WPS_WLAN0_START_TIME && diff_time < WPS_WLAN1_START_TIME) {
			printf("WPS button do something for %s\n", WLANIF[0]);
			if(!btn_test)
				rtk_wlan_wps_button_action(0);
		}
		else if(diff_time >= WPS_WLAN1_START_TIME){
			printf("WPS button do something for %s\n", WLANIF[1]);
			if(!btn_test)
				rtk_wlan_wps_button_action(1);
		}
#else
		if (diff_time >= WPS_WLAN0_START_TIME) {
			printf("WPS button do something\n");
#ifdef WLAN_SUPPORT
			if(!btn_test)
				rtk_wlan_wps_button_action(-1);
#endif
		}
#endif
	}
	return 0;
}

int rtk_sys_pushbtn_wifi_action(int action, int btn_test, int diff_time)
{
	//printf("WIFI On/Off button action %d, btn_test %d, diff_time %d\n", action, btn_test, diff_time);
	if (action == 0) {	// off
#ifdef WLAN_SUPPORT
		if (diff_time >= 1 && diff_time < 4) {
			printf("WIFI On/Off button do something\n");
			if(!btn_test)
				rtk_wlan_wifi_button_on_off_action();
		}
#endif
	}

	return 0;
}

int rtk_sys_pushbtn_ledpwr_action(int action, int btn_test, int diff_time)
{
	unsigned char led_status;
	//printf("LED On/Off button action %d, btn_test %d, diff_time %d\n", action, btn_test, diff_time);
	if (action == 0) {	// off
		if (diff_time < 4) {
			printf("LED power On/Off button do something\n");
#ifdef LED_TIMER
			if(!btn_test)
			{
				if(getLedStatus(&led_status)==0)
					setLedStatus(!led_status);
			}
#endif
		}
	}

	return 0;
}

int rtk_sys_pushbtn_reset_action(int action, int btn_test, int diff_time)
{
	//printf("Reset button action %d, btn_test %d, diff_time %d\n", action, btn_test, diff_time);
#ifdef CONFIG_MGTS
	rtk_mgts_pushbtn_reset_action(action, btn_test, diff_time);
#elif defined(CONFIG_E8B)
	extern int rtk_e8_pushbtn_reset_action(int action, int btn_test, int diff_time);
	rtk_e8_pushbtn_reset_action(action, btn_test, diff_time);
#else
	rtk_generic_pushbtn_reset_action(action, btn_test, diff_time);
#endif
	return 0;
}
#endif

#if defined(CONFIG_USER_VLAN_MAPPING) && defined(CONFIG_RTK_DEV_AP)
int rtk_get_vlan_mapping_info(char *lan_name,unsigned int *ifindex)
{
	int i = 0,j = 0,ifidx = 0, wan_vid = 0, wan_vlan_index = 0, wanEntryVid = 0,totalwanNum = 0;
	MIB_CE_PORT_BINDING_T pEntry;
	MIB_CE_ATM_VC_T wanEntry;
	struct vlan_pair *vid_pair;
	char lan_vid_temp_0[5] = {0};
	char lan_vid_temp_1[5] = {0};

	for(j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; j++)
	{
		if(strstr(lan_name, (char *)ELANRIF[j]))
		{
			ifidx = j;
			strncpy(lan_vid_temp_0, lan_name + strlen((char *)ELANRIF[j])+1, sizeof(lan_vid_temp_0)-1);
			break;
		}
	}

#ifdef WLAN_SUPPORT
	if(j == PMAP_WLAN0)
	{
		for (j=PMAP_WLAN0; j<=PMAP_WLAN1_VAP_END; j++) {
			if(strstr(lan_name, (char *)wlan[j-PMAP_WLAN0]))
			{
				ifidx = j;
				strncpy(lan_vid_temp_0, lan_name + strlen((char *)wlan[j-PMAP_WLAN0])+1, sizeof(lan_vid_temp_0)-1);
				break;
			}
		}
	}
#endif // of WLAN_SUPPORT

	memset(&pEntry, 0, sizeof(MIB_CE_PORT_BINDING_T));
	mib_chain_get(MIB_PORT_BINDING_TBL, ifidx, (void*)&pEntry);

	if((unsigned char)VLAN_BASED_MODE == pEntry.pb_mode)
	{
		vid_pair = (struct vlan_pair *)&(pEntry.pb_vlan0_a);
		for (i = 0; i < 4; i++)
		{
			if (vid_pair[i].vid_a <= 0)
				continue;

			memset(lan_vid_temp_1, 0, sizeof(lan_vid_temp_1));
			snprintf(lan_vid_temp_1, sizeof(lan_vid_temp_1), "%d", vid_pair[i].vid_a);

			if(strcmp(lan_vid_temp_0, lan_vid_temp_1) == 0)
			{
				wan_vid = vid_pair[i].vid_b;
			}
		}
	}
	else{
		//printf("[%s %d] this lan didn't use vlan binding name:%s\n", __FUNCTION__, __LINE__, name);
		*ifindex = 0;
		return -1;
	}

	totalwanNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(j = 0; j < totalwanNum; j++){
		mib_chain_get(MIB_ATM_VC_TBL, j, (void*)&wanEntry);
		wanEntryVid = wanEntry.vid;

		if (!wanEntry.enable || (wanEntryVid != wan_vid))
			continue;

		//printf("%s %d wanvid:%d wanifIndex:%d +++++++++++++++++++\n", __FUNCTION__, __LINE__, wanEntry.vid, wanEntry.ifIndex);
		*ifindex = wanEntry.ifIndex;
		return 0;
	}

	*ifindex = 0;
	return -1;
}
#endif

//called by dnsmasq-2.80
int getWanIfNameByDomain(char *domain, char *wanIf, int wanif_len)
{
	FILE *fp=NULL;
	char buffer[1024]={0};
	char *pchar=NULL;
	int buffer_len, find=0;
	char tmp_domain[1024]={0};

	if(domain==NULL || wanIf==NULL)
		return -1;

	if(strlen(domain)<1)
		return -1;

	if((fp=fopen(DNSMASQ_CONF, "r"))==NULL)
		return -1;

	snprintf(tmp_domain, sizeof(tmp_domain), "/%s/", domain);

	while(fgets(buffer, sizeof(buffer), fp))
	{
		buffer_len=strlen(buffer);
		if(buffer_len==0)
			continue;

		buffer[buffer_len-1]='\0';
		if(strstr(buffer, tmp_domain)==NULL)
			continue;

		if((pchar = strchr(buffer, '@'))==NULL)
			continue;

		pchar++;

		snprintf(wanIf, wanif_len, "%s", pchar);

		if(strncmp(wanIf, ALIASNAME_MWNAS, strlen(ALIASNAME_MWNAS))==0 || strncmp(wanIf, ALIASNAME_PPP, strlen(ALIASNAME_PPP))==0)
		{
			//printf("\n%s:%d domain=%s wanIf=%s\n",__FUNCTION__,__LINE__,domain,wanIf);
			find=1;
			break;
		}
	}

	fclose(fp);

	return (find ? 0 : -1);
}

int rtk_get_lan_bind_wan_ifname(char *lan_ifname, char *wan_ifname)
{
	int total,i,j;
	int ret_val=-1;
	unsigned int itfGroup;
	unsigned int ifindex=0;
	char tmp_ifname[IFNAMSIZ]={0};
	MIB_CE_ATM_VC_T Entry;

#if defined(CONFIG_USER_VLAN_MAPPING) && defined(CONFIG_RTK_DEV_AP)
	unsigned int ifindex_tmp=0;
	ret_val = rtk_get_vlan_mapping_info(lan_ifname, &ifindex_tmp);
	if(ret_val == 0){
		ifindex = ifindex_tmp;
		//printf("[%s %d]this lan is vlan binding ! vid:%d ifIndex:%d name:%s   ++++++++++++++++\n", __FUNCTION__, __LINE__, *vid, *ifindex, name);
		goto find_exit;
	}
#endif

	total = mib_chain_total(MIB_ATM_VC_TBL);
	if (total > MAX_VC_NUM)
		return ret_val;

	for(i = 0; i < total; i++)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&Entry) == 0)
			printf("\n %s %d\n", __func__, __LINE__);
		if (!Entry.enable || !isValidMedia(Entry.ifIndex))
			continue;

		itfGroup = Entry.itfGroup;

		if(strncmp(lan_ifname, ALIASNAME_ELAN_PREFIX, strlen(ALIASNAME_ELAN_PREFIX))==0)
		for(j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; j++)
		{
			if((itfGroup & (1<<j)) != 0)
			if(strcmp((char *)ELANVIF[j], lan_ifname)==0)
			{
				ifindex = Entry.ifIndex;
				goto find_exit;
			}
		}

#ifdef WLAN_SUPPORT
		if(strncmp(lan_ifname, ALIASNAME_WLAN, strlen(ALIASNAME_WLAN))==0)
		for (j=PMAP_WLAN0; j<=PMAP_WLAN0_VAP_END; j++)
		{
			if(wlan_en[j-PMAP_WLAN0])
			if((itfGroup & (1<<j)) != 0)
			if(strcmp((char *)wlan[j-PMAP_WLAN0], lan_ifname)==0)
			{
				ifindex = Entry.ifIndex;
				goto find_exit;
			}
		}

		if(strncmp(lan_ifname, ALIASNAME_WLAN, strlen(ALIASNAME_WLAN))==0)
		for (j=PMAP_WLAN1; j<=PMAP_WLAN1_VAP_END; j++)
		{
			if(wlan_en[j-PMAP_WLAN0])
			if((itfGroup & (1<<j)) != 0)
			if(strcmp((char *)wlan[j-PMAP_WLAN0],lan_ifname)==0)
			{
				ifindex = Entry.ifIndex;
				goto find_exit;
			}
		}
#endif // of WLAN_SUPPORT
	}

find_exit:
	if(ifindex>0)
	{
		ifGetName(ifindex, tmp_ifname, sizeof(tmp_ifname));
		strcpy(wan_ifname, tmp_ifname);
		ret_val=0;
	}
	return ret_val;
}

#if defined(CONFIG_USER_WAN_PORT_AUTO_SELECTION)
void rtk_update_lan_wan_binding(void)
{
	int i, j, total = 0;
	MIB_CE_ATM_VC_T entry;

#ifndef CONFIG_RTL_MULTI_PHY_ETH_WAN
	int wanPhyPort = rtk_port_get_wan_phyID();
#endif

	total = mib_chain_total(MIB_ATM_VC_TBL);
	if (total > MAX_VC_NUM || total < 1)
		return;

	for (i = PMAP_ETH0_SW0; i < PMAP_WLAN0 && i < ELANVIF_NUM; i++) {
#ifndef CONFIG_RTL_MULTI_PHY_ETH_WAN
		if (rtk_port_get_lan_phyID(i) != wanPhyPort)
			continue;
#else
		if (!rtk_port_is_wan_logic_port(i))
			continue;
#endif
		for (j = 0; j < total; j++) {
			if (!mib_chain_get(MIB_ATM_VC_TBL, j, (void *)&entry))
				continue;
			if (!entry.enable || !isValidMedia(entry.ifIndex))
				continue;

			if ((entry.itfGroup & (1 << i)) != 0) {
				entry.itfGroup &= (~(1 << i));
				mib_chain_update(MIB_ATM_VC_TBL, (void *)&entry, j);
#ifdef COMMIT_IMMEDIATELY
				Commit();
#endif
			}
		}
	}
}
#endif


#if defined(CONFIG_USER_LANNETINFO) && defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
/**************************************************
* @NAME:
* 	rtk_get_mac_by_ipv6_addr
*
* @PARAMETERS:
*	@Input			char *ipv6Addr: lan pc ipv6 address
*	@Output			char *mac: lan pc mac
*
* @RETRUN:
* 	-1				find lanPC mac fail.
*	 0				find lanPC mac success.
*
* @FUNCTION :
* 	get lan pc mac by ipv6 address.
***************************************************/
int rtk_get_mac_by_ipv6_addr(char *ipv6Addr, char *mac)
{
	int i= 0, j=0,count = 0, found=0;
	char ipv6_str[INET6_ADDRSTRLEN]={0};
	char mac_str[MAC_ADDR_STRING_LEN]={0};
	lanHostInfo_t *pLanNetInfo = NULL;


	if (ipv6Addr==NULL || mac==NULL){
		printf("[%s %d]: Parameter Err!\n", __func__, __LINE__);
		return -1;
	}

	if (get_lan_net_info(&pLanNetInfo, &count) !=0){
		printf("[%s %d]: get_lan_net_info Failed!\n", __func__, __LINE__);
		if(pLanNetInfo)
			free(pLanNetInfo);
		return -1;
	}

	inet_ntop(AF_INET6, ipv6Addr, ipv6_str, INET6_ADDRSTRLEN);

#ifdef SUPPORT_GET_MULTIPLE_LANHOST_IPV6_ADDR
	for(i = 0; i < count; i++){  /* need to get offline device */
		for(j = 0; j < MAX_IPV6_CNT; j++){
			if (pLanNetInfo[i].lanhost_ipv6s[j].use){
				if (!memcmp(pLanNetInfo[i].lanhost_ipv6s[j].ipv6Addr, ipv6Addr, IP6_ADDR_LEN)){
					memcpy(mac, pLanNetInfo[i].mac, MAC_ADDR_LEN);
					changeMacToString(mac, mac_str);
					if(pLanNetInfo)
						free(pLanNetInfo);

					printf("[%s %d]: IPv6 Addr %s at  %s!\n", __func__, __LINE__,ipv6_str,mac_str);
					return 0;
				}
			}
		}
	}
#endif

	if(pLanNetInfo)
		free(pLanNetInfo);

	printf("[%s %d]: Can't found mac by ipv6 addr %s\n", __func__, __LINE__, ipv6_str);

	return -1;
}

/**************************************************
* @NAME:
* 	rtk_get_manual_wan_ifname_by_lan_ip
*
* @PARAMETERS:
*	@Input			union mysockaddr *lan_ip_info
*	@Output			wan_ifname: lan pc connect to wan interface name by manual load balance
*
* @RETRUN:
* 	-1				find wan ifname fail.
*	 0				find wan ifname success.
*
* @FUNCTION :
* 	get wan interface name from manual load balance info by lan pc ip address.
***************************************************/
int rtk_get_manual_wan_ifname_by_lan_ip(union mysockaddr *lan_ip_info, char* wan_itfname)
{
	int i = 0, j = 0,find_lan_idx = 0, find_lan_flag = 0;
	int sta_entry_num = 0;
	MIB_CE_ATM_VC_T entry;
	unsigned int count = 0;
	char wanif[IFNAMSIZ] = {0};
	MIB_STATIC_LOAD_BALANCE_T sta_entry;
	lanHostInfo_t  LanNetInfo = {0};
	unsigned char mac_tmp[6] = {0};
	lanHostInfo_t *pLanNetInfo = NULL;
	unsigned char lan_mac[18] = {0};
	int find_wan_flag = 0, ret =-1;
	char global_balance_enable = 0, sta_enable = 0;
	int ret_1 = 0;

	if(wan_itfname == NULL || lan_ip_info ==NULL){
		printf("[%s %d]: pointer wan_itfname or lan_ip_info is NULL!\n", __func__, __LINE__);
		goto get_info_end;
	}

	if((!mib_get_s(MIB_STATIC_LOAD_BALANCE_ENABLE, (void *)&sta_enable,sizeof(sta_enable))) ||
			(!mib_get_s(MIB_GLOBAL_LOAD_BALANCE, (void *)&global_balance_enable,sizeof(global_balance_enable))))
	{
		printf("[%s %d]: get MIB_STATIC_LOAD_BALANCE_ENABLE fail!\n", __func__, __LINE__);
		goto get_info_end;
	}

	//when diasble global load balance or manual load balance,don't check lan info.
	if(sta_enable == 0 || global_balance_enable == 0){
		printf("[%s %d]: manual or global load balance is disable\n", __func__, __LINE__);
		goto get_info_end;
	}

	ret_1 = get_lan_net_info(&pLanNetInfo, &count);

	if(ret_1 != 0){
		printf("[%s %d]: get lan net info fail\n", __func__, __LINE__);
		goto get_info_end;
	}

	sta_entry_num = mib_chain_total(MIB_STATIC_LOAD_BALANCE_TBL);

	if(count>0){
		if(lan_ip_info->sa.sa_family == AF_INET)
		{
			//get lan pc mac by ipv4 address
			for(i=0; i<count; i++)
			{
				if(pLanNetInfo[i].ip == lan_ip_info->in.sin_addr.s_addr)
				{
					find_lan_idx = i;
					find_lan_flag = 1;
					snprintf(lan_mac,sizeof(lan_mac),"%02x:%02x:%02x:%02x:%02x:%02x", pLanNetInfo[find_lan_idx].mac[0],pLanNetInfo[find_lan_idx].mac[1],pLanNetInfo[find_lan_idx].mac[2],pLanNetInfo[find_lan_idx].mac[3],pLanNetInfo[find_lan_idx].mac[4],pLanNetInfo[find_lan_idx].mac[5]);
					break;
				}
			}
		}
		else if(lan_ip_info->sa.sa_family == AF_INET6)
		{
			//get lan pc mac by ipv6 address
			if(!rtk_get_mac_by_ipv6_addr((char *)(&lan_ip_info->in6.sin6_addr), mac_tmp)){
				snprintf(lan_mac,sizeof(lan_mac),"%02x:%02x:%02x:%02x:%02x:%02x", mac_tmp[0], mac_tmp[1], mac_tmp[2], mac_tmp[3], mac_tmp[4], mac_tmp[5]);
				find_lan_flag = 1;
			}
		}
		else{
			//do nothing
		}

		if(find_lan_flag)
		{
			memset(wanif,0,IFNAMSIZ);
			//check munual load balance entry which could has the same mac.
			for(j=0;j<sta_entry_num;j++)
			{
				if(!mib_chain_get(MIB_STATIC_LOAD_BALANCE_TBL, j, &sta_entry))
					continue;

				if(!strncasecmp(lan_mac,sta_entry.mac,MAC_ADDR_STRING_LEN-1))
				{
					find_wan_flag  = 1;
					snprintf(wanif,IFNAMSIZ,"%s",sta_entry.wan_itfname);
					break;
				}
			}
		}
	}

get_info_end:
	if(pLanNetInfo)
		free(pLanNetInfo);

	if(find_wan_flag){
		snprintf(wan_itfname,IFNAMSIZ,"%s",wanif);
		ret = 0;
	}

	return ret;
}

/**************************************************
* @NAME:
*	rtk_get_auto_wan_ifname_by_lan_ip
*
* @PARAMETERS:
*	@Input			union mysockaddr *lan_ip_info
*	@Output 		wan_ifname: lan pc connect to wan interface name by auto load balance
*
* @RETRUN:
*	-1				find wan ifname fail.
*	 0				find wan ifname success.
*
* @FUNCTION :
*	get wan interface name from auto load balance info by lan pc ipv4 address.
***************************************************/
int rtk_get_auto_wan_ifname_by_lan_ip(union mysockaddr *lan_ip_info, char* wan_itfname)
{
	int i = 0, j = 0,k=0,find_lan_idx = 0, find_lan_flag = 0;
	int sta_entry_num = 0;
	unsigned int count = 0, wanCount = 0;
	char wanif[IFNAMSIZ] = {0};
	con_wan_info_T *pConWandata = NULL;
	lanHostInfo_t *pLanNetInfo = NULL;
	unsigned char lan_mac[18] = {0};
	unsigned char mac_tmp[6] = {0};
	int find_wan_flag = 0, ret =-1;
	char global_balance_enable = 0, dyn_enable = 0;
	int ret_1 = 0, ret_2 = 0;

	if(wan_itfname == NULL || lan_ip_info ==NULL)
	{
		printf("[%s %d]: wan_itfname or lan_ip_info is NULL!\n", __func__, __LINE__);
		goto get_info_end;
	}

	if((!mib_get_s(MIB_GLOBAL_LOAD_BALANCE, (void *)&global_balance_enable,sizeof(global_balance_enable))) ||
		(!mib_get_s(MIB_DYNAMIC_LOAD_BALANCE, (void *)&dyn_enable,sizeof(dyn_enable))))
	{
		printf("[%s %d]: get MIB value fail!\n", __func__, __LINE__);
		goto get_info_end;
	}

	//when diasble global load balance or auto load balance,don't check lan info.
	if(dyn_enable == 0 || global_balance_enable == 0){
		printf("[%s %d]: auto or global load balance is disable\n", __func__, __LINE__);
		goto get_info_end;
	}

	ret_1 = get_lan_net_info(&pLanNetInfo, &count);
	ret_2 = get_con_wan_info(&pConWandata,&wanCount);

	if(ret_1 != 0 || ret_2 != 0){
		printf("[%s %d]: get lanNetInfo or wanDataInfo fail\n", __func__, __LINE__);
		goto get_info_end;
	}

	if(count>0){
		if(lan_ip_info->sa.sa_family == AF_INET)
		{
			//get lan pc mac by ipv4 address
			for(i=0; i<count; i++)
			{
				if(pLanNetInfo[i].ip == lan_ip_info->in.sin_addr.s_addr)
				{
					find_lan_idx = i;
					find_lan_flag = 1;
					snprintf(lan_mac,sizeof(lan_mac),"%02x:%02x:%02x:%02x:%02x:%02x", pLanNetInfo[find_lan_idx].mac[0],pLanNetInfo[find_lan_idx].mac[1],pLanNetInfo[find_lan_idx].mac[2],pLanNetInfo[find_lan_idx].mac[3],pLanNetInfo[find_lan_idx].mac[4],pLanNetInfo[find_lan_idx].mac[5]);
					break;
				}
			}
		}
		else if(lan_ip_info->sa.sa_family == AF_INET6)
		{
			//get lan pc mac by ipv6 address
			if(!rtk_get_mac_by_ipv6_addr((char *)(&lan_ip_info->in6.sin6_addr), mac_tmp)){
				snprintf(lan_mac,sizeof(lan_mac),"%02x:%02x:%02x:%02x:%02x:%02x", mac_tmp[0], mac_tmp[1], mac_tmp[2], mac_tmp[3], mac_tmp[4], mac_tmp[5]);
				find_lan_flag = 1;
			}
		}
		else{
			//do nothing
		}

		if(find_lan_flag)
		{
			memset(wanif,0,IFNAMSIZ);

			//check auto load balance entry which could has the same mac.
			if(wanCount > 0 && ret_2 == 0)
			{
				for(k=0;k<wanCount;k++)
				{
					if(pConWandata[k].wan_mark)
					{
						if(find_wan_flag == 1)
							break;

						if(rtk_get_wan_itf_by_mark(wanif,pConWandata[k].wan_mark)!=1)
						{
							//printf("this mark cant find wanitf name.\n");
							continue;
						}
						for(j=0; j<MAX_MAC_NUM; j++)
						{
							if(strlen(pConWandata[k].mac_info[j].mac_str))
							{
								if(!strncasecmp(lan_mac, pConWandata[k].mac_info[j].mac_str, MAC_ADDR_STRING_LEN-1))
								{
									find_wan_flag  = 1;
									break;
								}
							}
						}
					}
				}
			}
		}
	}

get_info_end:
	if(pLanNetInfo)
		free(pLanNetInfo);

	if(pConWandata)
		free(pConWandata);

	if(find_wan_flag){
		snprintf(wan_itfname,IFNAMSIZ,"%s",wanif);
		ret = 0;
	}

	return ret;
}
#endif

/**************************************************
* @NAME:
* 	rtk_get_default_gw_wan_from_route_table
*
* @PARAMETERS:
*	@Output/Input     wan_ifname: save wan interface name
*
* @RETRUN:
* 	RTK_SUCCESS
*	RTK_FAILED
*
* @FUNCTION :
* 	get default route wan interface name from main route table
***************************************************/
int rtk_get_default_gw_wan_from_route_table(char *wan_ifname)
{
	int match_flag=0;
	FILE *fp=NULL;
	char line_buffer[256]={0};

	if(wan_ifname==NULL)
		return RTK_FAILED;

	system("route > /var/tmp_route_table");

	if((fp=fopen("/var/tmp_route_table", "r"))==NULL)
		return RTK_FAILED;

	while(fgets(line_buffer,sizeof(line_buffer), fp))
	{
		if(strstr(line_buffer, "default")==NULL)
			continue;

		if(sscanf(line_buffer, "%*s %*s %*s %*s %*s %*s %*s %s", wan_ifname)==1)
		{
			match_flag=1;
			//printf("\n%s:%d wan_ifname=%s\n",__FUNCTION__,__LINE__,wan_ifname);
			break;
		}
	}

	fclose(fp);

#ifdef CONFIG_IPV6
	//if did not found ipv4 default gw wan, use ipv6 default gw wan
	if (!match_flag)
	{
		memset(wan_ifname, 0,IFNAMSIZ);
		get_ipv6_default_gw_inf(wan_ifname);
		if (strlen(wan_ifname) !=0)
		{
			match_flag=1;
			//printf("\n%s:%d IPV6 default wan_ifname=%s\n",__FUNCTION__,__LINE__,wan_ifname);
		}
	}
#endif

	return (match_flag ? RTK_SUCCESS : RTK_FAILED);
}

int rtk_get_default_gw_ip_by_wan_info(char* wan_name, char wan_mode, char *gw_ip)
{
	FILE *fp=NULL;
	char ipoe_wan_gw_file[24] = {0};
	char tmp_ip[16] = {0};
	int ret = RTK_FAILED;

	if(wan_name==NULL || gw_ip==NULL)
		return RTK_FAILED;

	if(wan_mode == CHANNEL_MODE_IPOE)
	{
		snprintf(ipoe_wan_gw_file,24,"/tmp/MERgw.%s",wan_name);

		if((fp=fopen(ipoe_wan_gw_file, "r"))==NULL)
			return RTK_FAILED;

		while(fgets(tmp_ip,sizeof(tmp_ip), fp))
		{
			if(strlen(tmp_ip)>0){
				snprintf(gw_ip,16,"%s",tmp_ip);
				ret = RTK_SUCCESS;
				break;
			}
		}
		fclose(fp);
	}
	else if(wan_mode == CHANNEL_MODE_PPPOE)
	{
		struct in_addr in;
		if (getInAddr(wan_name, IP_ADDR, (void *)&in) == 0 || in.s_addr == 0)
			return RTK_FAILED;

		if (getInAddr(wan_name, DST_IP_ADDR, (void *)&in) == 0 || in.s_addr == 0)
			return RTK_FAILED;

		snprintf(gw_ip, 16, "%s", inet_ntoa(in));
		ret = RTK_SUCCESS;
	}

	return ret;
}

int rtk_get_default_gw_ip_from_route_table(char *ifname, struct in_addr *gw_ip)
{
	int match_flag=0;
	FILE *fp=NULL;
	char line_buffer[256]={0};
	char tmp_ifname[IFNAMSIZ]={0};
	char tmp_ip[16]={0};

	if(gw_ip==NULL)
		return RTK_FAILED;

	system("route > /var/tmp_route_table");

	if((fp=fopen("/var/tmp_route_table", "r"))==NULL)
		return RTK_FAILED;

	while(fgets(line_buffer,sizeof(line_buffer), fp))
	{
		if(strstr(line_buffer, "default")==NULL)
			continue;

		if(sscanf(line_buffer, "%*s %s %*s %*s %*s %*s %*s %s",tmp_ip, tmp_ifname)==2)
		{
			if(ifname && strcmp(ifname, tmp_ifname)==0)
				match_flag=1;

			if(match_flag)
			{
				inet_aton(tmp_ip, gw_ip);
				break;
			}
		}
	}
	fclose(fp);

	return (match_flag ? RTK_SUCCESS : RTK_FAILED);
}

int rtk_generate_resolv_file(const char *src_file)
{
	char dns_line[256]={0};
	FILE *fp_bak=NULL;
	FILE *fp=NULL;
	char *pchar=NULL;

	fp_bak=fopen(src_file,"r");
	if(fp_bak==NULL)
		return -1;

	fp=fopen(RESOLV, "w");
	if(fp==NULL)
	{
		fclose(fp_bak);
		return -1;
	}

	while(fgets(dns_line,sizeof(dns_line), fp_bak))
	{
		if((pchar = strchr(dns_line, '@')) != NULL)
			*pchar = '\0';
		if((pchar = strchr(dns_line, '\n')) != NULL)
			*pchar = '\0';

		if(strlen(dns_line)>=7)
			fprintf(fp, "nameserver %s\n", dns_line);
	}

	fclose(fp_bak);
	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(RESOLV, 0666);
#endif
	return 0;
}

int rtk_util_ip_version(const char *src)
{
    char buf[16];
    if (inet_pton(AF_INET, src, buf)) {
        return 4;
    } else if (inet_pton(AF_INET6, src, buf)) {
        return 6;
    }
    return -1;
}

#if defined(CONFIG_IPOE_ARPING_SUPPORT)
int parseArpingResult(char* filename)
{
	FILE *fp;
	char buff[256];
	int icmp_suceed = 0, fail_count=0;

	if ((fp = fopen(filename, "r")) == NULL)
			return 0;
	while(fgets(buff, sizeof(buff), fp) != NULL) {
		if(sscanf(buff, "%*s %*s %d %*s", &icmp_suceed)!=1) {
			printf("can't parsing the result from %s\n", filename);
			break;
		}
		if(icmp_suceed == 1){
			fail_count=0;
			break;
		}
		if(icmp_suceed == 0)
			fail_count++;
	}
	fclose(fp);
	if(fail_count==0)
		unlink(filename);
	return fail_count;
}

unsigned long arping_handler_timer_count=0;
struct callout_s arping_lcp_ch;
void arpingLcpHandler(void *dummy)
{
	int fail_count=0, dhcpc_pid=0;
	unsigned int atmVcEntryNum, i;
	MIB_CE_ATM_VC_T atmVcEntry;
	char ifname[IFNAMSIZ]={0}, cmd_Str[256]={0}, gw_addr[32]={0}, arping_result[256]={0}, sVal[32]={0},filename[64]={0};

	atmVcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<atmVcEntryNum; i++)
	{
		if (mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&atmVcEntry))
		{
			if((atmVcEntry.cmode == CHANNEL_MODE_IPOE) && (atmVcEntry.IpProtocol & IPVER_IPV4) &&(atmVcEntry.ipDhcp == DHCP_CLIENT))
			{
				if(atmVcEntry.ip_arp_enable == 0)
					continue;
				if((arping_handler_timer_count%(atmVcEntry.ip_arp_echo_interval))!=0)
					continue;
				ifGetName(atmVcEntry.ifIndex,ifname,sizeof(ifname));
				if(ifGetGatewayByWan(ifname, gw_addr)==1) {
					//printf(" [%s:%d] ifname=%s gw_addr=%s\n", __FUNCTION__, __LINE__, ifname, gw_addr);
				}
				else
					continue;

				sprintf(arping_result, "%s_%s", ARPING_RESULT, ifname);

				fail_count = parseArpingResult(arping_result);
				if( fail_count >= atmVcEntry.ip_arp_retry ) {
					printf("Release DHCPClient ifname=%s because ARPing fail_count(%d) > retry_count(%d)\n",
						ifname, fail_count, atmVcEntry.ip_arp_retry);
					snprintf(sVal, 32, "%s.%s", (char*)DHCPC_PID, ifname);
					dhcpc_pid = read_pid((char*)sVal);
					if (dhcpc_pid > 0) {
						kill(dhcpc_pid, SIGUSR2); // force release
					}
					sleep(1);
					if (dhcpc_pid > 0) {
						kill(dhcpc_pid, SIGUSR1); // force release
					}
				}
			}
		}
	}

	arping_handler_timer_count += ARPING_HANDLER_POLLING_INTERVAL;
	if((arping_handler_timer_count < 0) || (arping_handler_timer_count == ULONG_MAX))
		arping_handler_timer_count = 0;
	TIMEOUT_F(arpingLcpHandler, 0, ARPING_HANDLER_POLLING_INTERVAL, arping_lcp_ch);
}
#endif

const char *ACCESSPROTOCOL[]={"console", "telnet"};
const char *CONNECION_INTERFACE[]={"LAN", "WAN", "WLAN"};
extern char *if_indextoname (unsigned int __ifindex, char *__ifname);
int rtk_bridge_get_ifname_by_portno(const char *brname, int portno, char *ifname)
{
	int ifindices[MAX_LAN_PORT_NUM];
	unsigned long args[4] = { BRCTL_GET_PORT_LIST,
				(unsigned long)ifindices, MAX_LAN_PORT_NUM, 0 };
	struct ifreq ifr;
	int br_socket_fd;

	if ((br_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		return -1;
	}
	memset(ifindices, 0, sizeof(ifindices));
	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, brname, IFNAMSIZ-1);
	ifr.ifr_data = (char *) &args;

	if (ioctl(br_socket_fd, SIOCDEVPRIVATE, &ifr) < 0) {
		printf("get_portno: get ports of %s failed: %s\n",
			brname, strerror(errno));
		close(br_socket_fd);
		return -1;
	}

	if_indextoname(ifindices[portno], ifname);

	close(br_socket_fd);
	return 0;
}


/*
0: mean LAN
1: mean WAN
2: mean WLAN
*/
int rtk_intf_get_intf_by_ip(char* ipaddr)
{
	char cmd[256];
	char intf[15];
	char buff[32];
	char lanip[INET_ADDRSTRLEN] = {0};
	FILE *fp = NULL;
	int dotcount = 0;
	int length = 0;

	memset(intf, 0, sizeof(intf));

	sprintf(cmd,"ip neigh | grep %s | awk '{print $3}' > %s", ipaddr, ACC_INTER);
	system(cmd);
	if ((fp = fopen(ACC_INTER, "r")) == NULL)
		return -1;

	memset(buff, 0, sizeof(buff));
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		sscanf(buff, "%s", intf);

		if (intf[0] == '\0' || strstr(intf, "nas") || strstr(intf, "ppp")) {
			fclose(fp);
			unlink(ACC_INTER);
			return 1;
		}
	}
	fclose(fp);
	unlink(ACC_INTER);

	getMIB2Str(MIB_ADSL_LAN_IP, lanip);
	while (lanip[length] != '\0') {
		if (lanip[length] == '.') {
			dotcount++;
			if (dotcount == 3)
				break;
		}
		length++; 
	}

	if (strncmp(ipaddr, lanip, length) == 0)
		return 0; // local
	else
		return 1; // remote

// for insert port
#if 0 
	//get mac addr
	sprintf(cmd,"ip neigh | grep %s | awk '{print $5}' > %s", ipaddr, ACC_MAC);
	system(cmd);
	if ((fp = fopen(ACC_MAC, "r")) == NULL)
		return -1;

	memset(buff, 0, sizeof(buff));
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		sscanf(buff, "%s", mac);
	}
	fclose(fp);
	unlink(ACC_MAC);


	//get the specific mac bridge port no.
	sprintf(cmd, "brctl showmacs %s | grep %s | awk '{print $1}' > %s", intf, mac, ACC_PORTNUM);
	system(cmd);
	if ((fp = fopen(ACC_PORTNUM, "r")) == NULL)
		return -1;

	memset(buff, 0, sizeof(buff));
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		sscanf(buff, "%d", &portno);

		//get real interface name by bridge port no.
		rtk_bridge_get_ifname_by_portno(intf, portno, ifname);
		*num = portno;
	}
	fclose(fp);
	unlink(ACC_PORTNUM);

	if (buff) {
		if (strstr(ifname, "wlan"))
			return 2; //WLAN
		else // eth0.x  
			return 0; //LAN
	}
#endif

	return -1;
}

int rtk_util_get_ipv4addr_from_ipv6addr(char* ipaddr)
{
	char ipv4Addr[INET_ADDRSTRLEN];
	struct in_addr nipAddr;

	if(strstr(ipaddr, ".") && strstr(ipaddr, ":")){
		memset(ipv4Addr, 0, sizeof(ipv4Addr));
		if(sscanf(ipaddr, "::ffff:%s", ipv4Addr) == 1){
			if(inet_aton(ipv4Addr, &nipAddr)!=0){
				//printf("%s:%d ipaddr %s\n", __FUNCTION__, __LINE__, ipaddr);
				strcpy(ipaddr, ipv4Addr);
				//printf("%s:%d ipv4Addr %s\n", __FUNCTION__, __LINE__, ipv4Addr);
			}
		}
	}
	return 0;
}

int rtk_util_get_wanaddr_from_nf_conntrack(char* remote_ipaddr, int port, char* ipaddr)
{
	char cmd[256];
	char ipaddr_str[INET6_ADDRSTRLEN];
	FILE*fp;
	int ifcount=0, i=0;
	struct wstatus_info sEntry[MAX_VC_NUM+MAX_PPP_NUM];
	ifcount = getWanStatus(sEntry, MAX_VC_NUM+MAX_PPP_NUM);

	snprintf(cmd,sizeof(cmd),"cat /proc/net/nf_conntrack | grep 'src=%s' | grep 'dport=%d' | awk '{print $8}' | awk -F'=' '{print $2}' | awk '!a[$0]++' > /tmp/wan_ipaddr", remote_ipaddr, port);
	//printf(cmd);
	system(cmd);

	if ((fp = fopen("/tmp/wan_ipaddr", "r")) == NULL)
		return 0;

	while(fgets(ipaddr_str, sizeof(ipaddr_str), fp) != NULL) {
		ipaddr_str[strlen(ipaddr_str)-1] = 0;
		//printf("ipaddr_str %s\n", ipaddr_str);
		for(i=0; i < ifcount; i++){
			//printf("sEntry[%d].remoteIp %s\n", i, sEntry[i].remoteIp);
			if(strcmp(ipaddr_str, sEntry[i].ipAddr)==0){
				snprintf(ipaddr, INET6_ADDRSTRLEN,"%s", ipaddr_str);
				fclose(fp);
				return 1;
			}
		}
	}
	fclose(fp);

	return 0;
}

#ifdef CONFIG_GPON_FEATURE
#ifdef CONFIG_RTK_HOST_SPEEDUP
#if defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
int get_ldpid(int flow, int* ldpid, int* cos, int* gemid)
{
	FILE*fp;
	char buff[512], *ptr1, *ptr2, flow_str[20];
	int flow_tmp, ldpid_tmp, cos_tmp, gemid_tmp;
	int success = 0;

	system("cat /proc/fc/sw_dump/pon_streamid > /tmp/pon_streamid");

	if ((fp = fopen("/tmp/pon_streamid", "r")) == NULL)
		return 1;

	while(fgets(buff, sizeof(buff), fp) != NULL) {
		buff[strlen(buff)-1] = 0;
		ptr1 = strstr(buff, "[");
		if(ptr1 == NULL){
			break;
		}
		ptr2 = strstr(buff, "]");
		if(ptr2 == NULL){
			break;
		}

		strncpy(flow_str, ptr1+1, ptr2-(ptr1+1));
		flow_tmp = atoi(flow_str);

		printf("flow_tmp %d, flow_str '%s', ptr2 '%s' \n", flow_tmp, flow_str, ptr2+1);
		if(sscanf(ptr2+1, " ldpid(tcont): 0x%x cos: %d flowid(gemid): %d", &ldpid_tmp, &cos_tmp, &gemid_tmp)!=3) {
			printf("can't parsing the result from /tmp/pon_streamid\n");
			break;
		}
		if(flow_tmp == flow){
			*ldpid = ldpid_tmp;
			*cos = cos_tmp;
			*gemid = gemid_tmp;
			printf("flow %d, ldpid %d, cos %d, gemid %d\n", flow, *ldpid, *cos, *gemid);
			success = 1;
			break;
		}
	}
	fclose(fp);

	if(success == 0) return 1;

	return 0;
}
#endif

int set_speedup_usflow(MIB_CE_ATM_VC_T *pEntry)
{
        char buff[128];
        int wanidx=-1;
        int usflow[WAN_PONMAC_QUEUE_MAX];
        char cmdStr[128];
        int pon_mode=0;
		struct wanif_flowq wanq;

		wanq.wanIfidx=pEntry->ifIndex;
        mib_get_s(MIB_PON_MODE, (void *)&pon_mode, sizeof(pon_mode));

		memset(usflow, 0, sizeof(usflow));

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		if(EPON_MODE == pon_mode)
		{
#if defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
			usflow[0] = 0;
#else
			return -1;
#endif
		}
#if	defined(CONFIG_TR142_MODULE)
		else if(GPON_MODE == pon_mode)
		{
			if(get_wan_flowid(&wanq) == -1)
			{
				printf("<%s %d> not find valid wan(%u) streamid\n",__func__,__LINE__,pEntry->ifIndex);
			    return -1;
			}
			else
			{
				usflow[0]=wanq.usflow[0];
			}
		}
#endif

#if defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
		int ldpid=0, cos=0, gemid=0;
		if(ETH_MODE == pon_mode || FIBER_MODE == pon_mode){
			sprintf(cmdStr, "echo ldpid %d > /proc/HostSpeedUP-FLOWID", rtk_port_get_wan_phyID());
			printf("<%s %d> %s\n", __func__, __LINE__, cmdStr);
			system(cmdStr);
		}
		else{
			if(get_ldpid(usflow[0], &ldpid, &cos, &gemid) == 0){
				sprintf(cmdStr, "echo flowid %d ldpid %d cos %d gemid %d > /proc/HostSpeedUP-FLOWID", usflow[0], ldpid, cos, gemid);
				printf("<%s %d> %s\n", __func__, __LINE__, cmdStr);
				system(cmdStr);
			}
			else{
				printf("<%s %d> get ldpid error\n", __func__, __LINE__);
			}
		}
#else
		sprintf(cmdStr, "echo %d > /proc/HostSpeedUP-FLOWID", usflow[0]);
		//printf("<%s %d> %s\n", __func__, __LINE__, cmdStr);
		system(cmdStr);
#endif

		return 0;
#endif

        return -1;
}

int clear_speedup_usflow(void)
{
	return 0;
}

int routeInternetWanCheck(void)
{
	int total, i, ret;
	MIB_CE_ATM_VC_T entry;
	struct in_addr inAddr;
	char ifname[64];

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_ATM_VC_TBL, i, &entry) == 0)
			continue;

		if ((entry.cmode != CHANNEL_MODE_BRIDGE) && (entry.applicationtype & X_CT_SRV_INTERNET) &&
				ifGetName(entry.ifIndex, ifname, sizeof(ifname)) &&
				getInFlags(ifname, &ret) &&
				(ret & IFF_UP) &&
				getInAddr(ifname, IP_ADDR, &inAddr))
		{
			set_speedup_usflow(&entry);
			return i;
		}
	}

	return -1;
}
#endif
#endif

int rtk_util_check_wan_vid_exist(unsigned short vid)
{
	int total, i, ret;
	MIB_CE_ATM_VC_T entry;

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_ATM_VC_TBL, i, &entry) == 0)
			continue;

		if (entry.vid == vid)
		{
			return i;
		}
	}

	return -1;
}

int rtk_util_parse_remoteport(char* source, int*start, int*end)
{
	char *pch;
	int i = 0;
	char tmp[128] = {0};

	strncpy(tmp, source, sizeof(tmp)-1);

	if(tmp[0] == '!'){
		*start = atoi(&tmp[1]);
		*end = atoi(&tmp[1]);
		return 1;
	}
	else if(!strstr(tmp, "-")){//format x
		*start = atoi(tmp);
		*end = atoi(tmp);
		if(*start == 0 && *end == 0)
			return 3;
		else
			return 2;
	}
	else{//format x-y
		for(i=0; i < strlen(tmp); i++){
			if(tmp[i] == '-'){
				break;
			}
		}
		tmp[i] = 0;
		*start = atoi(tmp);
		*end = atoi(&tmp[i+1]);

		return 0;
	}
	return -1;
}

#if defined(BB_FEATURE_SAVE_LOG) || defined(CONFIG_USER_RTK_SYSLOG)
void writeLogFileHeader(FILE * fp)
{
	unsigned char dev_id_buf[256], log_ip[INET_ADDRSTRLEN];
	unsigned char bufptr[64];
	char manufacturer[64] =  {0};
	char model[64] =  {0};
	char hw_ver[64] =  {0};
	char sw_ver[64]=  {0};

/*star:20081017 modify for some macro is changed to mib entry*/
#ifdef _CWMP_MIB_
	unsigned char ipAddr[IP_ADDR_LEN];
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_00R0)
	mib_get_s(MIB_HW_SERIAL_NUMBER, (void*)dev_id_buf, sizeof(dev_id_buf));
#else
	getOUIfromMAC(dev_id_buf);
	mib_get_s(MIB_HW_SERIAL_NUMBER, bufptr, sizeof(bufptr));
	snprintf(dev_id_buf + strlen(dev_id_buf),sizeof(dev_id_buf) - strlen(dev_id_buf), "-%s", bufptr);
#endif
	mib_get_s(MIB_ADSL_LAN_IP, ipAddr, sizeof(ipAddr));
	log_ip[INET_ADDRSTRLEN-1]='\0';
	strncpy(log_ip, inet_ntoa(*(struct in_addr *)ipAddr),INET_ADDRSTRLEN-1);
#else
	dev_id_buf[255]='\0';
	strncpy(dev_id_buf, "devId",255);
	log_ip[INET_ADDRSTRLEN-1]='\0';
	strncpy(log_ip, "192.168.1.1",INET_ADDRSTRLEN-1);
#endif

	getMIB2Str(MIB_HW_CWMP_MANUFACTURER, manufacturer);
	getMIB2Str(MIB_HW_CWMP_PRODUCTCLASS, model);
	getMIB2Str(MIB_HW_HWVER, hw_ver);
#ifdef CONFIG_CMCC
	mib_get_s(MIB_PROVINCE_SW_VERSION, (void *)sw_ver, sizeof(sw_ver));
#else
	getSYS2Str(SYS_FWVERSION, sw_ver);
#endif
	fprintf(fp,
		"Manufacturer:%s;\nProductClass:%s;\nSerialNumber:%s;\nIP:%s;\nHWVer:%s;\nSWVer:%s;\n\n",
		manufacturer, model, dev_id_buf, log_ip,
		hw_ver, sw_ver);
}

int syslogFileMergeFromFlash()
{
#ifndef USE_DEONET
	unsigned int log_file=0, i=0;
	FILE *fp;
	char filename[MAXFNAME], cmdStr[128];;

	mib_get_s(MIB_SYSLOG_LOOP_FILES, &log_file, sizeof(log_file));
	fp = fopen(SYSLOGDMERGEDFILE, "wb");
	if (!fp) {
		printf("%s-%d open %s failed\n", __FUNCTION__, __LINE__, SYSLOGDMERGEDFILE);
		return -1;
	}
	writeLogFileHeader(fp);
	fclose(fp);

	for(i=log_file; i > 0; i--)
	{
		sprintf(filename, "%s_%d", SYSLOGDFILE, i);
		if( access( filename, F_OK ) != -1 ) {
			sprintf(cmdStr, "/bin/cat %s >> %s", filename, SYSLOGDMERGEDFILE);
			system(cmdStr);
		}
	}

	sprintf(filename, "%s", SYSLOGDFILE);
	if( access( filename, F_OK ) != -1 ) {
		sprintf(cmdStr, "/bin/cat %s >> %s", filename, SYSLOGDMERGEDFILE);
		system(cmdStr);
	}
#endif
	return 0;
}

#endif

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
void rtk_if_statistic_reset(void)
{
	unsigned int atmVcEntryNum, i;
	MIB_CE_ATM_VC_T atmVcEntry;
	char ifname[IFNAMSIZ]={0};

	atmVcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<atmVcEntryNum; i++)
	{
		if (mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&atmVcEntry))
		{
			ifGetName(atmVcEntry.ifIndex,ifname,sizeof(ifname));
			rt_stat_netifMib_reset(ifname);
		}
	}

	for(i=0;i<ELANVIF_NUM;i++)
	{
		if (rtk_port_get_lan_phyID(i) != -1)
		{
			if(rt_stat_port_reset(rtk_port_get_lan_phyID(i)) != RT_ERR_OK)
				printf("rt_stat_port_reset failed: %s %d\n", __func__, __LINE__);
		}
	}

	return;
}
#endif

void putUShort(unsigned char *obuf, u_int32_t val)
{
	u_int16_t tmp = htons(val);
	memcpy(obuf, &tmp, sizeof(tmp));
}

static unsigned char base64chars[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "abcdefghijklmnopqrstuvwxyz0123456789+/=";

static int base64charsIndex(unsigned char c)
{
	int i=0;
	while(i<65){
		if(base64chars[i]==c)
			return i;
		i++;
	}
	return -1;
}

void rtk_util_data_base64decode(unsigned char *input, unsigned char *output, const int outputSize)
{
	unsigned char chr1, chr2, chr3;
	unsigned char enc1, enc2, enc3, enc4;
	int i=0, j=0, len=0;

	if(input == NULL || input[0] == 0 || output == NULL)
		return;

	len = strlen(input);
	if(len < 4) return ;
	if(len/4*3 > outputSize) return;

	for (i = 0; i < strlen(input) - 3; i += 4) {
		enc1 = base64charsIndex(input[i+0]);
		enc2 = base64charsIndex(input[i+1]);
		enc3 = base64charsIndex(input[i+2]);
		enc4 = base64charsIndex(input[i+3]);

		output[j++] = ((enc1 << 2) | (enc2 >> 4));
		if (input[i+2] != base64chars[64])
			output[j++] = (((enc2 << 4) & 0xF0) | ((enc3 >> 2) & 0x0F));
		if (input[i+3] != base64chars[64])
			output [j++] = (((enc3 << 6) & 0xC0) | enc4);
	}
}

void  rtk_util_data_base64encode(unsigned char *input, unsigned char *output, const int outputSize)
{
	unsigned char chr1, chr2, chr3;
	unsigned char enc1, enc2, enc3, enc4;
	int i=0, j=0, len=strlen(input);
	int encodeRetLen = 0;
	if(len%3 != 0)
		encodeRetLen = (len/3+1)*4;
	else
		encodeRetLen = (len/3)*4;
	if(encodeRetLen > outputSize)
		return;


	for (i = 0; i <= len - 3; i += 3)
	{
		output[j++] = base64chars[(input[i] >> 2)];
		output[j++] = base64chars[((input[i] & 3) << 4) | (input[i+1] >> 4)];
		output[j++] = base64chars[((input[i+1] & 15) << 2) | (input[i+2] >> 6)];
		output[j++] = base64chars[input[i+2] & 63];
	}

	if (len % 3 == 2)
	{
		output[j++] = base64chars[(input[i] >> 2)];
		output[j++] = base64chars[((input[i] & 3) << 4) | (input[i+1] >> 4)];
		output[j++] = base64chars[((input[i+1] & 15) << 2)];
		output[j++] = base64chars[64];
	}
	else if (len % 3 == 1)
	{
		output[j++] = base64chars[(input[i] >> 2)];
		output[j++] = base64chars[((input[i] & 3) << 4)];
		output[j++] = base64chars[64];
		output[j++] = base64chars[64];
	}
}

void rtk_util_convert_to_star_string(char *star_string, int length)
{
	int i=0;
	for(i=0;i<length;i++)
		strcat(star_string, "*");
}

/* Maps special characters in HTML to their equivalent entites. */
static const char * const html_code_map[256] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,"&quot;",NULL,NULL,NULL,"&amp;",NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	"&lt;",NULL,"&gt;",NULL,NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL
};

/*
 * Encoded string, newly allocated on the heap.
 * Caller is responsible for freeing it with free().
 * If s is NULL, or on out-of memory, returns NULL.
 */
char *rtk_util_html_encode(const char *s)
{
   char * buf;
   size_t buf_size;

   if (s == NULL)
   {
      return NULL;
   }

   /* each input char can expand to at most 6 chars */
   buf_size = (strlen(s) * 6) + 1;
   buf = (char *) malloc(buf_size);

   if (buf)
   {
      char c;
      char * p = buf;

	  memset(buf, 0, buf_size);
      while ( (c = *s++) != '\0')
      {
         const char * replace_with = html_code_map[(unsigned char) c];
         if(replace_with != NULL)
         {
            const size_t bytes_written = (size_t)(p - buf);
            strncpy(p, replace_with, buf_size - bytes_written);
			p += strlen(replace_with);
         }
         else
         {
            *p++ = c;
         }
      }

      *p = '\0';
   }

   return(buf);
}

#ifdef CONFIG_RTK_SMB_SPEEDUP
int rtk_smb_speedup(void)
{
	char serverIP[INET_ADDRSTRLEN], cmd[128];
	struct in_addr inAddr;

	if (getInAddr((char *)LANIF, IP_ADDR, &inAddr) == 1) {
		strncpy(serverIP, inet_ntoa(inAddr), sizeof(serverIP));
	} else {
		getMIB2Str(MIB_ADSL_LAN_IP, serverIP);
	}

	system("echo set 1 > /proc/smbshortcut");
	snprintf(cmd, sizeof(cmd), "echo sip %s > /proc/smbshortcut", serverIP);
	system(cmd);

	return 0;
}
#endif

int rtk_get_interface_packet_statistics(unsigned char *interface_name,
	unsigned long long *rx_packets, unsigned long long *tx_packets,
	unsigned long long *rx_bytes, unsigned long long *tx_bytes)
{
	unsigned long long temp_rx_packets = 0, temp_tx_packets = 0;
	unsigned long long temp_rx_bytes = 0, temp_tx_bytes = 0;
	char number_buffer[512] = {0};
	FILE *fp = NULL;
	char ifname[IFNAMSIZ + 1] = {0}; //need to add : character
	int flag;
	if (!interface_name || !interface_name[0] || getInFlags(interface_name, &flag) != 1)
		return -1;

	if (!(rx_packets || tx_packets || rx_bytes || tx_bytes)) return -1;


	if ((fp = fopen(PROC_NET_DEV_PATH, "r")))
	{
		/* skip 2 unnessary line */
		fgets(number_buffer, sizeof(number_buffer), fp);
		fgets(number_buffer, sizeof(number_buffer), fp);

		snprintf(ifname, sizeof(ifname), "%s:", interface_name);

		while (fgets(number_buffer, sizeof(number_buffer), fp))
		{
			if(strstr(number_buffer, ifname))
			{
				sscanf(number_buffer, "%*s%llu%u%*d%*d%*d%*d%*d%*d%llu%u", &temp_rx_bytes, &temp_rx_packets, &temp_tx_bytes, &temp_tx_packets);
				/*
				printf("rx_bytes = %lld\n", temp_rx_bytes);
				printf("rx_packets = %d\n", temp_rx_packets);
				printf("tx_bytes = %lld\n", temp_tx_bytes);
				printf("tx_packets = %d\n", temp_tx_packets);
				*/
				break;
			}
		}
		fclose(fp);

		if (rx_packets)
		{
			*rx_packets = temp_rx_packets;
		}
		if (tx_packets)
		{
			*tx_packets = temp_tx_packets;
		}
		if (rx_bytes)
		{
			*rx_bytes = temp_rx_bytes;
		}
		if (tx_bytes)
		{
			*tx_bytes = temp_tx_bytes;
		}

		return 1;
	}

	return 0;
}

#if defined(CONFIG_YUEME)
const char INTERFACE_STATS_FILE[] = "/var/interface_stats_file";
#define IntfStatsLock "/var/run/intfStatistic"
#define LOCK_INTFSTATS()	\
int lockfd; \
do {	\
	if ((lockfd = open(IntfStatsLock, O_RDWR)) == -1) {	\
		perror("open INTFSTATS lockfile");	\
		return 0;	\
	}	\
	while (flock(lockfd, LOCK_EX)) { \
		if (errno != EINTR) \
			break; \
	}	\
} while (0)

#define UNLOCK_INTFSTATS()	\
do {	\
	flock(lockfd, LOCK_UN);	\
	close(lockfd);	\
} while (0)

int rtk_set_interface_packet_statistics(unsigned char *interface_name,
	unsigned long long rx_packets, unsigned long long tx_packets,
	unsigned long long rx_bytes, unsigned long long tx_bytes)
{
	char cmdbuf[256] = {0};
	int flag;
	if (!interface_name || !interface_name[0] || getInFlags(interface_name, &flag) != 1)
		return -1;

	snprintf(cmdbuf, sizeof(cmdbuf)-1, "echo stats %s rx_packets %lld tx_packets %lld rx_bytes %lld tx_bytes %lld > /proc/rtk_smuxdev/interface",
									interface_name, rx_packets, tx_packets, rx_bytes, tx_bytes);
	va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);

	return 0;
}

int rtk_reset_interface_packet_statistics(unsigned char *interface_name)
{
	char cmdbuf[256] = {0};
	int flag;
	if (!interface_name || !interface_name[0] || getInFlags(interface_name, &flag) != 1)
		return -1;

	snprintf(cmdbuf, sizeof(cmdbuf)-1, "echo stats %s reset > /proc/rtk_smuxdev/interface", interface_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);

#ifdef CONFIG_COMMON_RT_API
	rt_stat_netifMib_reset(interface_name);
#endif

	return 0;
}

int rtk_save_interface_packet_statistics_log_file(unsigned char *interface_name,
	unsigned long long rx_packets, unsigned long long tx_packets,
	unsigned long long rx_bytes, unsigned long long tx_bytes)
{
	char fileName[128] = {0};
	FILE *fp;
	int flag;
	if (!interface_name || !interface_name[0] || getInFlags(interface_name, &flag) != 1)
		return -1;

	LOCK_INTFSTATS();
	snprintf(fileName, sizeof(fileName)-1, "%s_%s", INTERFACE_STATS_FILE, interface_name);
	unlink(fileName);
	if(!(fp = fopen(fileName, "a"))) {
		UNLOCK_INTFSTATS();
		return -1;
	}
	fprintf(fp, "ifname=%s rx_packets=%lld tx_packets=%lld rx_bytes=%lld tx_bytes=%lld\n", interface_name, rx_packets, tx_packets, rx_bytes, tx_bytes);
	fclose(fp);
	UNLOCK_INTFSTATS();

 	//AUG_PRT("Saved ifname=%s rx_packets=%lld tx_packets=%lld rx_bytes=%lld tx_bytes=%lld\n", interface_name, rx_packets, tx_packets, rx_bytes, tx_bytes);
	return 0;
}

int rtk_load_interface_packet_statistics_log_file(unsigned char *interface_name,
	unsigned long long *rx_packets, unsigned long long *tx_packets,
	unsigned long long *rx_bytes, unsigned long long *tx_bytes)
{
	unsigned long long rx_packets_f, tx_packets_f, rx_bytes_f, tx_bytes_f;
	char fileName[128] = {0}, ifname_f[64];
	char line[1024];
	FILE *fp;
	int ret;
	int flag;
	if (!interface_name || !interface_name[0] || getInFlags(interface_name, &flag) != 1)
		return -1;

	LOCK_INTFSTATS();
	snprintf(fileName, sizeof(fileName)-1, "%s_%s", INTERFACE_STATS_FILE, interface_name);

	if(!(fp = fopen(fileName, "r"))) {
		UNLOCK_INTFSTATS();
		return -1;
	}

	while(fgets(line, sizeof(line)-1, fp) != NULL)
	{
		ret = sscanf(line, "ifname=%s rx_packets=%lld tx_packets=%lld rx_bytes=%lld tx_bytes=%lld\n", &ifname_f[0], &rx_packets_f, &tx_packets_f, &rx_bytes_f, &tx_bytes_f);
		if(ret==5)
		{
			fclose(fp);
			unlink(fileName);
			*rx_packets = rx_packets_f;
			*tx_packets = tx_packets_f;
			*rx_bytes = rx_bytes_f;
			*tx_bytes = tx_bytes_f;
			UNLOCK_INTFSTATS();
			//AUG_PRT("Restore ifname=%s rx_packets=%lld tx_packets=%lld rx_bytes=%lld tx_bytes=%lld\n", interface_name, *rx_packets, *tx_packets, *rx_bytes, *tx_bytes);
			return 0;
		}
	}

	fclose(fp);
	UNLOCK_INTFSTATS();
	return -1;
}

int rtk_init_interface_packet_statistics_log_file(void)
{
	FILE *f;

	if((f = fopen(IntfStatsLock, "w")) == NULL)
			return -1;

	fclose(f);
#ifdef RESTRICT_PROCESS_PERMISSION
		chmod (IntfStatsLock, 0666);
#endif
}
#endif

#ifdef RESTRICT_PROCESS_PERMISSION
void set_otherUser_Capability(uid_t euid)
{
	struct __user_cap_header_struct cap_header_data;
	cap_user_header_t cap_header = &cap_header_data;
	struct __user_cap_data_struct cap_data_data[_LINUX_CAPABILITY_U32S_3];
	cap_user_data_t cap_data = cap_data_data;

	seteuid(euid);

	cap_header->pid = getpid();
	cap_header->version = _LINUX_CAPABILITY_VERSION_3;
	if (capget(cap_header, cap_data) < 0) {
		perror("Failed capget");
		exit(1);
	}
	//printf("before: Cap data %08x%08x, %08x%08x, %08x%08x\n", cap_data[1].effective, cap_data[0].effective,
	//	cap_data[1].permitted, cap_data[0].permitted, cap_data[1].inheritable, cap_data[0].inheritable);

	cap_data[0].effective |= (1<<CAP_NET_ADMIN);
	cap_data[0].effective |= (1<<CAP_NET_RAW);
	cap_data[0].effective |= (1<<CAP_NET_BIND_SERVICE);
	cap_data[0].effective |= (1<<CAP_KILL);
	cap_data[0].effective |= (1<<CAP_SYS_NICE);
	cap_data[0].effective |= (1<<CAP_SETFCAP);
	cap_data[0].inheritable = cap_data[0].permitted;
	cap_data[1].inheritable = cap_data[1].permitted;

	if (capset(cap_header, cap_data) < 0)
		perror("Failed capset");

	if (capget(cap_header, cap_data) < 0) {
		perror("Failed capget");
		exit(1);
	}
	//printf("after: Cap data %08x%08x, %08x%08x, %08x%08x\n", cap_data[1].effective, cap_data[0].effective,
	//	cap_data[1].permitted, cap_data[0].permitted, cap_data[1].inheritable, cap_data[0].inheritable);
}
#endif

int getMacFromIp(void *ipaddr, int family, unsigned char *macaddr)
{
	if((ipaddr == NULL) || (macaddr == NULL))
		return -1;
	if(family == AF_INET)
	{
		int s;
		struct arpreq arpreq;
		struct sockaddr_in *sin;

		// get mac address
		memset(&arpreq, 0, sizeof(struct arpreq));
		sin = (struct sockaddr_in *) &arpreq.arp_pa;
		sin->sin_family = AF_INET;
		memcpy(&sin->sin_addr, ipaddr, IP_ADDR_LEN);
		strcpy(arpreq.arp_dev, "br0");

		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (s < 0)
		{
			perror("[getMacFromIp] socket");
			return -1;
		}
		if (ioctl(s, SIOCGARP, &arpreq) < 0)
		{
			perror("[getMacFromIp] ioctl");
			close(s);
			return -1;
		}

		if (!(arpreq.arp_flags & ATF_COM))
		{
			close(s);
			fprintf(stderr,"[getMacFromIp] arp entry not complete\n");
			return -1;
		}
		memcpy(macaddr, arpreq.arp_ha.sa_data, MAC_ADDR_LEN);
		close(s);
	}
	else if(family == AF_INET6)
	{
		FILE *fp = NULL;
		int count = 0, ret = 0;
		char cmd[32] = {0}, buf[256] = {0};
		unsigned char mac[6] = {0};
		char *delim = " ";
		char *p = NULL, *ep = NULL;
		struct in6_addr addr6;

		snprintf(cmd, sizeof(cmd), "ip -6 neighbor");
		count = 0;
		while (fp == NULL && count < 5)
		{
			fp = popen(cmd, "r");
			count++;
			if(fp == NULL)
				usleep(100000); // 100ms
		}

		if (fp)
		{
			while (fgets(buf, sizeof(buf), fp))
			{
				ep = buf + strlen(buf);
				if (*(ep - 1) != '\n')
				{
					break;
				}
				//fprintf(stderr, "[%s:%d] buf = %s\n", __func__, __LINE__, buf);
				p = strtok(buf, delim);
				//fprintf(stderr, "[%s:%d] v6 addr = %s\n", __func__, __LINE__, p);

				inet_pton(AF_INET6, p, &(addr6));
				ret = memcmp(&addr6, ipaddr, IP6_ADDR_LEN);

				count = 0;
				if (0 == ret)
				{
					while (p = strtok(NULL, delim))
					{
						count++;
						if (count == 4)
						{
							//fprintf(stderr, "[%s:%d] MAC = %s\n", __func__, __LINE__, p);
							if(sscanf(p,"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",&(macaddr[0]),&(macaddr[1]),&(macaddr[2]),
								&(macaddr[3]),&(macaddr[4]),&(macaddr[5])) == 6)
							{
								pclose(fp);
								return 0;
							}
							else
							{
								pclose(fp);
								fprintf(stderr,"[getMacFromIp] pass ipv6 mac addr fail\n");
								return -1;
							}
							break;
						}
					}
				}
			}
			pclose(fp);
			fprintf(stderr,"[getMacFromIp]not found ipv6 addr\n");
			return -1;
		}
		else
		{
			fprintf(stderr,"[getMacFromIp] popen fail\n");
			return -1;
		}
	}
	else
	{
		fprintf(stderr,"[getMacFromIp] ip family not correct\n");
		return -1;
	}
	return 0;
}

int getHostNameByMac(unsigned char *macAddress, char *hostName)
{
	char macAddr[40], *buf=NULL;
	unsigned long fsize, leaseCnt;
	int ret;

	if(getDhcpClientLeasesDB(&buf, &fsize) <= 0)
		goto err;

	sscanf(macAddress, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &(macAddr[0]),&(macAddr[1]),&(macAddr[2]),
			&(macAddr[3]),&(macAddr[4]),&(macAddr[5]));
	leaseCnt = fsize/sizeof(struct dhcpOfferedAddrLease);
	ret = getDhcpClientHostName(buf, leaseCnt, macAddr, hostName);
	if (ret)
	{
		free(buf);
		return 0;
	}

err:
	if (buf)
		free(buf);
	return -1;
}

int flock_set(int fd, int type)
{
	struct flock lock;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_type = type;
	lock.l_pid = getpid();

	if((fcntl(fd, F_SETLKW, &lock)) < 0)
	{
		printf("Lock failed:type = %d\n", lock.l_type);
		return 1;
	}
	return 0;
}


int convertMacFormat(char *str, unsigned char *mac)
{
	char tmpBuf[3];
	int i, j = 0, len;
	char *p;

	if(str == NULL || mac == NULL) return 0;

	p = strpbrk(str, ":-");
	len = strlen(str);
	for (i = 0; i < len;)
	{
		tmpBuf[0] = str[i];
		tmpBuf[1] = str[i+1];
		tmpBuf[2] = 0;

		if (!isxdigit(tmpBuf[0]) || !isxdigit(tmpBuf[1]))
			return 0;

		mac[j++] = (unsigned char)strtol(tmpBuf, (char**)NULL, 16);

		if(p) i+=3;
		else i+=2;
	}
	//printf("%x %x %x %x %x %x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	return ((j==6) ? 1 : 0);
}

unsigned char isZeroMac(unsigned char *pMac)
{
	return !(pMac[0]|pMac[1]|pMac[2]|pMac[3]|pMac[4]|pMac[5]);
}

unsigned int macAddrCmp(unsigned char *addr1, unsigned char *addr2)
{
	unsigned short *a = (unsigned short *)addr1;
	unsigned short *b = (unsigned short *)addr2;

	return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) != 0;
}

void changeMacToString(unsigned char *mac, unsigned char *macString)
{
	if( (mac==NULL)||(macString==NULL) )
		return;
	sprintf(macString, "%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

void fillcharZeroToMacString(unsigned char *macString)
{
	int i=0;
	if(macString==NULL)
		return;
	while(*(macString+i))
	{
		if(*(macString+i)==' ')
			*(macString+i)='0';
		i++;
	}
}

#if 0
int rtk_epon_getAuthState(int llid)
{
    FILE *fp;
    char cmdStr[256], line[256];
    char tmpStr[20];
    int ret=2;

    snprintf(cmdStr, 256, "oamcli get ctc auth %d", llid);

    fp = popen(cmdStr, "r");
    if (fp)
    {
        fgets(line, sizeof(line), fp);
        sscanf(line, "%*[^:]: %s", tmpStr);
        if (!strcmp(tmpStr, "success"))
            ret = 1;
        else if (!strcmp(tmpStr, "fail"))
            ret = 0;

        pclose(fp);
    }
    return (ret);
}
#endif

//change one line of file [fileName]
//[old_line_keyWord] decide which line should be changed
//[new_line] set new statements
//if old line not exist, add new line to the end of file
int change_line_of_file(char *filename, char* old_line_keyWord, char* new_line)
{
	FILE* fp=NULL,*fp_tmp=NULL;
	char file_tmp[128]={0};
	char line_buff[256]={0};
	int found=0;

	if(!filename||!old_line_keyWord||!new_line){
		fprintf(stderr, "Invalid input!\n");
		return -1;
	}
	snprintf(file_tmp,sizeof(file_tmp),"%s_bak",filename);
	if(!isFileExist(filename)){
		snprintf(line_buff,sizeof(line_buff),"echo '%s'>%s",new_line,filename);
		system(line_buff);
		//printf("%s:%d file not exist  %s \n",__FUNCTION__,__LINE__,line_buff);
		return 1;
	}else{
		if(!(fp=fopen(filename,"r"))){
			fprintf(stderr, "Open file %s fail!\n",filename);
			return -1;
		}
	}
	if(!(fp_tmp=fopen(file_tmp,"w"))){
		fprintf(stderr, "Open file %s fail!\n",(char *)fp_tmp);
		fclose(fp);
		return -1;
	}
	while(fgets(line_buff,sizeof(line_buff),fp)){
		if(strstr(line_buff,old_line_keyWord) && found==0){
			bzero(line_buff,sizeof(line_buff));
			snprintf(line_buff,sizeof(line_buff),"%s\n",new_line);
			found=1;
		}
		//printf("%s:%d fputs  %s \n",__FUNCTION__,__LINE__,line_buff);
		fputs(line_buff,fp_tmp);
	}
	fclose(fp);
	fclose(fp_tmp);
	if(rename(file_tmp,filename) != 0)
		printf("Unable to rename the file: %s %d\n", __func__, __LINE__);
	if(found==0){
		snprintf(line_buff,sizeof(line_buff),"echo '%s'>>%s",new_line,filename);
		system(line_buff);
		//printf("%s:%d keyword %s not find!  %s \n",__FUNCTION__,__LINE__,old_line_keyWord,line_buff);
		return 1;
	}
	return 0;
}

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
void decrypt_fwimg(char* filename)
{
	char decrypt_file[32], cmd[128];
	char openssl_exe[] = "openssl";
	char encrypt_algo[] = "aes-128-cbc";
	char openssl_key[] = "realtek";
	unsigned int fileSize;
	struct stat st;

	sprintf(decrypt_file, "%s.tmp", filename);
	sprintf(cmd, "cat %s | %s %s -d -out %s -k %s", filename, openssl_exe, encrypt_algo, decrypt_file, openssl_key);
	system(cmd);

	if(stat(decrypt_file, &st) == -1)
		printf("[%s %d] stat fail\n",__FUNCTION__, __LINE__);
	fileSize = st.st_size;
	if( fileSize!=0 )
	{
		sprintf(cmd, "cp %s %s", decrypt_file, filename);
		system(cmd);
	}
}
#endif

#ifdef RTK_MULTI_AP
void _set_up_backhaul_credentials(MIB_CE_MBSSIB_Tp pEntry)
{
	unsigned int seed       = 0;
	int          randomData = open("/dev/urandom", O_RDONLY);
	//int          mibVal     = 1;
	if (randomData < 0) {
		// something went wrong, use fallback
		seed = time(NULL) + rand();
	} else {
		char    myRandomData[50];
		ssize_t result = read(randomData, myRandomData, sizeof myRandomData);
		if (result < 0) {
			// something went wrong, use fallback
			seed = time(NULL) + rand();
		}
		int i = 0;
		for (i = 0; i < 50; i++) {
			seed += (unsigned char)myRandomData[i];
			if (i % 5 == 0) {
				seed = seed * 10;
			}
		}
	}
	srand(seed);
	char SSIDDic[62]       = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
	char NetworkKeyDic[83] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxy"
	                         "z1234567890~!@#0^&*()_+{}[]:;..?";

	char backhaulSSID[21], backhaulNetworkKey[31];
	strcpy(backhaulSSID, "EasyMeshBH-");
	backhaulSSID[20]       = '\0';
	backhaulNetworkKey[30] = '\0';

	// randomly generate SSID post-fix
	int i;
#ifdef MULTI_AP_OPEN_ENCRYPT_SUPPORT
	unsigned char backhaul_encrypt_none_en = 0;
	mib_get_s(MIB_MAP_BACKHAUL_ENCRYPT_NONE, (void *)&backhaul_encrypt_none_en,sizeof(backhaul_encrypt_none_en));
#endif
	for (i = 11; i < 20; i++) {
		backhaulSSID[i] = SSIDDic[rand() % 62];
	}
	// randomly generate network key
	for (i = 0; i < 30; i++) {
		backhaulNetworkKey[i] = NetworkKeyDic[rand() % 83];
	}

	// set into mib
#ifdef MAP_GROUP_SUPPORT
	unsigned char map_group_ssid[MAX_SSID_LEN]={0}, map_group_wpaPsk[MAX_PSK_LEN+1]={0};
	mib_get_s(MIB_MAP_GROUP_SSID, (void *)&map_group_ssid,sizeof(map_group_ssid));
	if(strlen(map_group_ssid))
		strcpy(pEntry->ssid, map_group_ssid);
	else
		strcpy(pEntry->ssid, backhaulSSID);

	mib_get_s(MIB_MAP_GROUP_WPA_PSK, (void *)&map_group_wpaPsk,sizeof(map_group_wpaPsk));
	if(strlen(map_group_wpaPsk))
	{
		strcpy(pEntry->wpaPSK, map_group_wpaPsk);
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
		strcpy(pEntry->wscPsk, map_group_wpaPsk);
#endif
	}
	else
	{
		strcpy(pEntry->wpaPSK, backhaulNetworkKey);
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
		strcpy(pEntry->wscPsk, backhaulNetworkKey);
#endif
	}
#else
	strcpy(pEntry->ssid, backhaulSSID);
	strcpy(pEntry->wpaPSK, backhaulNetworkKey);
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
	strcpy(pEntry->wscPsk, backhaulNetworkKey);
#endif
#endif

#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)

#ifdef MULTI_AP_OPEN_ENCRYPT_SUPPORT
	if(backhaul_encrypt_none_en == 0){
		pEntry->wsc_auth = WSC_AUTH_WPA2PSK;
		pEntry->wsc_enc = WSC_ENCRYPT_AES;
	}
	else{
		pEntry->wsc_auth = WSC_AUTH_OPEN;
		pEntry->wsc_enc = WSC_ENCRYPT_NONE;
	}
#else
	pEntry->wsc_auth = WSC_AUTH_WPA2PSK;
	pEntry->wsc_enc = WSC_ENCRYPT_AES;
#endif
	pEntry->wsc_configured = 1;
#endif
	pEntry->unicastCipher = 0;
	pEntry->wpa2UnicastCipher = WPA_CIPHER_AES;
	pEntry->hidessid = 1;
#ifdef WLAN_11W

#ifdef WLAN_WPA3
	if(pEntry->encrypt == WIFI_SEC_WPA2_WPA3_MIXED)
	{
		pEntry->dotIEEE80211W = 1;
		pEntry->sha256 = 0;
	}
	else
	{
		pEntry->dotIEEE80211W = 0;
		pEntry->sha256 = 0;
	}
#else

	pEntry->dotIEEE80211W = 0;
	pEntry->sha256 = 0;
#endif
#endif
}

#ifdef CONFIG_USER_RTK_SYSLOG
void check_syslogd_enable(void)
{
	unsigned char slogd_enable = 0;
	mib_get_s(MIB_SYSLOG, (void *)&slogd_enable, sizeof(slogd_enable));
	if(slogd_enable == 0)
	{
		slogd_enable = 1;
		mib_set(MIB_SYSLOG, (void *)&slogd_enable);
		stopLog();
		startLog();
	}
}
#endif

#if defined(MAP_DYNAMIC_BACKHAUL_BSS_IF)
int rtk_wlan_get_map_bss_idx(int wlanIdx, int *map_idx)
{
	MIB_CE_MBSSIB_T Entry;
	int i=0, map_enable_idx=-1, avaliable_idx=WLAN_MBSSID_NUM+1, ret=0;

	for(i=0; i<=WLAN_MBSSID_NUM; i++)
	{
		mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIdx, i, (void *)&Entry);
		if(Entry.multiap_bss_type&MAP_BACKHAUL_BSS)
		{
			map_enable_idx = i;
			break;
		}
		else if(Entry.wlanDisabled && (i<avaliable_idx))
		{
			avaliable_idx = i;
		}
	}

	if(map_enable_idx>=0)
	{
		*map_idx = map_enable_idx;
		ret = 1;
	}
	else if(avaliable_idx != (WLAN_MBSSID_NUM+1))
	{
		*map_idx = avaliable_idx;
		ret = 2;
	}
	else
	{
		ret = 0;
	}

	return ret;
}
#endif

#ifdef MULTI_AP_OPEN_ENCRYPT_SUPPORT
void _set_up_backhaul_encrypt(MIB_CE_MBSSIB_Tp pEntry)
{
	unsigned char backhaul_encrypt_none_en = 0;
	mib_get_s(MIB_MAP_BACKHAUL_ENCRYPT_NONE, (void *)&backhaul_encrypt_none_en,sizeof(backhaul_encrypt_none_en));


#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)

	if(backhaul_encrypt_none_en == 0){
		pEntry->wsc_auth = WSC_AUTH_WPA2PSK;
		pEntry->wsc_enc = WSC_ENCRYPT_AES;
	}
	else{
		pEntry->wsc_auth = WSC_AUTH_OPEN;
		pEntry->wsc_enc = WSC_ENCRYPT_NONE;
	}
	pEntry->wsc_configured = 1;
#endif
	pEntry->unicastCipher = 0;
	pEntry->wpa2UnicastCipher = WPA_CIPHER_AES;
	pEntry->hidessid = 1;
#ifdef WLAN_11W

#ifdef WLAN_WPA3
	if(pEntry->encrypt == WIFI_SEC_WPA2_WPA3_MIXED)
	{
		pEntry->dotIEEE80211W = 1;
		pEntry->sha256 = 0;
	}
	else
	{
		pEntry->dotIEEE80211W = 0;
		pEntry->sha256 = 0;
	}
#else

	pEntry->dotIEEE80211W = 0;
	pEntry->sha256 = 0;
#endif
#endif
}


int setupMultiAPController_encrypt(void)
{
	int i=0, j =0, map_idx= WLAN_VAP_ITF_INDEX;
	int backhaul_band = 1;
	unsigned char mibVal = 0;
	unsigned char backhaul_encrypt_none_en = 0;
	MIB_CE_MBSSIB_T Entry;
	mib_get_s(MIB_MAP_BACKHAUL_ENCRYPT_NONE, (void *)&backhaul_encrypt_none_en,sizeof(backhaul_encrypt_none_en));

#if defined(MAP_DYNAMIC_BACKHAUL_BSS_IF)
	map_idx = WLAN_VAP_ITF_INDEX;
#elif defined(RTK_RSV_VAP_FOR_EASYMESH)
	map_idx = MULTI_AP_BSS_IDX;
#else
	map_idx = WLAN_VAP_ITF_INDEX;
#endif
	mib_get_s(MIB_MAP_CONTROLLER, (void *)&mibVal,sizeof(mibVal));
	if(mibVal == MAP_CONTROLLER)
	{
		for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
			for (j = 0; j < NUM_VWLAN_INTERFACE; j++) {
					wlan_getEntry(&Entry, j);
					if (MAP_BACKHAUL_BSS & Entry.multiap_bss_type) {
						backhaul_band = i;
						break;
					}
			}
		}

		mib_chain_local_mapping_get(MIB_MBSSIB_TBL, backhaul_band, map_idx, (void *)&Entry);
		if(backhaul_encrypt_none_en){
			Entry.encrypt = WIFI_SEC_NONE;
		}else{
#ifdef MULTI_AP_BACKHAUL_BSS_DEFAULT_ENCRYPT
			Entry.encrypt = MULTI_AP_BACKHAUL_BSS_DEFAULT_ENCRYPT;
#else
#ifdef WLAN_WPA3
			Entry.encrypt = WIFI_SEC_WPA2_WPA3_MIXED;
#else
			Entry.encrypt = WIFI_SEC_WPA2;
#endif
#endif
		}
		_set_up_backhaul_encrypt(&Entry);
		mib_chain_local_mapping_update(MIB_MBSSIB_TBL, backhaul_band, (void *)&Entry, map_idx);
	}
	return 0;
}
#endif

int setupMultiAPController(char *strVal, char *role_prev, char *controller_backhaul_link, unsigned char *dhcp_mode)
{
	int i=0, j=0;
	MIB_CE_MBSSIB_T Entry;
	unsigned char mibVal = 1;
	char opmode=0;
	int map_idx=WLAN_VAP_ITF_INDEX;
#if defined(MAP_DYNAMIC_BACKHAUL_BSS_IF)
	int backhaul_bss_enable=1;
#endif
#ifdef MULTI_AP_OPEN_ENCRYPT_SUPPORT
	unsigned char backhaul_encrypt_none_en = 0;
	mib_get_s(MIB_MAP_BACKHAUL_ENCRYPT_NONE, (void *)&backhaul_encrypt_none_en,sizeof(backhaul_encrypt_none_en));
#endif

	// Set to controller
	mibVal = MAP_CONTROLLER;
	mib_set(MIB_MAP_CONTROLLER, (void *)&mibVal);
#ifdef CROSS_BAND_BACKHAUL_STEERING
		for (i = 0; i < NUM_WLAN_INTERFACE; i++)
		{
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, 0, (void *)&Entry);
			Entry.wlanDisabled = 0;
			mib_local_mapping_set(MIB_WLAN_DISABLED, i, (void *)&(Entry.wlanDisabled));
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
			Entry.wsc_disabled = 0;
#endif
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, 0);
		}
#else
#ifdef BACKHAUL_LINK_SELECTION
	for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
		if(i == (controller_backhaul_link[0] - '0')) {
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, 0, (void *)&Entry);
			Entry.wlanDisabled = 0;
			mib_local_mapping_set(MIB_WLAN_DISABLED, i, (void *)&(Entry.wlanDisabled));
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
			Entry.wsc_disabled = 0;
#endif
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, 0);
		}
	}
#else
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, 0, 0, (void *)&Entry);
	Entry.wlanDisabled = 0;
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
	Entry.wsc_disabled = 0;
#endif
	mib_chain_local_mapping_update(MIB_MBSSIB_TBL, 0, (void *)&Entry, 0);
#endif
#endif

#ifdef CONFIG_USER_RTK_BRIDGE_MODE
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	mib_get(MIB_OP_MODE, (void *)&opmode);
	if(opmode == GATEWAY_MODE){
		*dhcp_mode = mibVal = DHCP_LAN_SERVER;
	}else if(opmode == BRIDGE_MODE){
		*dhcp_mode = mibVal = AUTO_DHCPV4_BRIDGE;
	}
	mib_set(MIB_DHCP_MODE, (void *)&mibVal);
#endif
#endif

#ifdef WLAN_UNIVERSAL_REPEATER
	// Disable repeater
	mibVal = 0;
	// Disable vxd
	for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
		mib_local_mapping_set(MIB_REPEATER_ENABLED1, i, (void *)&mibVal);
		mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_REPEATER_ITF_INDEX, (void *)&Entry);
		Entry.wlanDisabled = 1;
		mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, WLAN_REPEATER_ITF_INDEX);
	}
#endif
	// if different from prev role, reset this mib to 0
	if (strcmp(strVal, role_prev)) {
		mibVal = 0;
		mib_set(MIB_MAP_CONFIGURED_BAND, (void *)&mibVal);
	}

	mibVal = MAP_FRONTHAUL_BSS;
	//int val;
	for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
		for (j = 0; j < (WLAN_MBSSID_NUM+1); j++) {
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&Entry);
#if defined(MAP_DYNAMIC_BACKHAUL_BSS_IF)
			if(Entry.multiap_bss_type&MAP_BACKHAUL_BSS)
			{
				if (Entry.wlanDisabled == 1)
					Entry.wlanDisabled = 0;
			}
			else
#endif
			if (Entry.wlanDisabled == 0) {// only set to fronthaul if this interface is enabled
				Entry.multiap_bss_type = mibVal;
			}

#ifdef WLAN_11K
			Entry.rm_activated = 1;
#endif
#ifdef WLAN_11V
			Entry.BssTransEnable = 1;
#endif
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, j);
		}
	}
#ifdef WLAN_BAND_STEERING
	mib_get_s(MIB_WIFI_STA_CONTROL, &mibVal, sizeof(mibVal));
	mibVal |= STA_CONTROL_ENABLE;
	mib_set(MIB_WIFI_STA_CONTROL, &mibVal);

	rtk_wlan_update_band_steering_status();
#endif

	mibVal = MAP_BACKHAUL_BSS;
	// wlan0-vap0, wlan1-vap0
	for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
#if defined(MAP_DYNAMIC_BACKHAUL_BSS_IF)
		backhaul_bss_enable = rtk_wlan_get_map_bss_idx(i, &map_idx);
#elif defined(RTK_RSV_VAP_FOR_EASYMESH)
		map_idx = MULTI_AP_BSS_IDX;
#else
		map_idx = WLAN_VAP_ITF_INDEX;
#endif
		mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, map_idx, (void *)&Entry);
#ifdef BACKHAUL_LINK_SELECTION
		if(i == (controller_backhaul_link[0] - '0')) {
#endif
#if defined(MAP_DYNAMIC_BACKHAUL_BSS_IF)
			if(backhaul_bss_enable == 0)
			{
				printf("MAP: NO avaliable interface for EasyMesh Backhaul BSS\n");
				return -1;
			}
#endif
			Entry.wlanDisabled = 0;
#ifdef MULTI_AP_BACKHAUL_BSS_DEFAULT_ENCRYPT
			Entry.encrypt = MULTI_AP_BACKHAUL_BSS_DEFAULT_ENCRYPT;
#else
#ifdef WLAN_WPA3
			Entry.encrypt = WIFI_SEC_WPA2_WPA3_MIXED;
#else
			Entry.encrypt = WIFI_SEC_WPA2;
#endif
#endif
#ifdef MULTI_AP_OPEN_ENCRYPT_SUPPORT
		if(backhaul_encrypt_none_en)
			Entry.encrypt = WIFI_SEC_NONE;

#endif
			Entry.wpaAuth = WPA_AUTH_PSK;
			Entry.multiap_bss_type = mibVal;
			if (strcmp(strVal, role_prev) || NULL == strstr(Entry.ssid, "EasyMeshBH") || 0 == strlen(Entry.wpaPSK)) {
				_set_up_backhaul_credentials(&Entry);
			}
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, map_idx);
#ifdef BACKHAUL_LINK_SELECTION
		} else
#if defined(MAP_DYNAMIC_BACKHAUL_BSS_IF)
			if(backhaul_bss_enable == 1)
#endif
			{
				Entry.multiap_bss_type = 0;
				Entry.wlanDisabled = 1;
				mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, map_idx);
			}
#endif
		//mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, WLAN_VAP_ITF_INDEX);
	}
#ifdef CONFIG_USER_RTK_SYSLOG
	check_syslogd_enable();
#endif
	return 0;
}

void setupMultiAPAgent(char *strVal, char *role_prev, char *agent_backhaul_link, unsigned char *dhcp_mode)
{
	int i=0, j=0, radio_index = 0;
	MIB_CE_MBSSIB_T Entry;
	unsigned char mibVal = 1, chan = 0;

	// Set to agent
	mibVal = MAP_AGENT;
	mib_set(MIB_MAP_CONTROLLER, (void *)&mibVal);
#ifdef CROSS_BAND_BACKHAUL_STEERING
	for (i = 0; i < NUM_WLAN_INTERFACE; i++)
	{

		mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, 0, (void *)&Entry);
		Entry.wlanDisabled = 0;
		mib_local_mapping_set(MIB_WLAN_DISABLED, i, (void *)&(Entry.wlanDisabled));
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
		Entry.wsc_disabled = 0;
#endif
		mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, 0);

		//disable auto channel for agent when backhaul link is 5G
#if defined(WLAN1_5G_SUPPORT)
		if (i == 1)
#else
		if (i == 0)
#endif
		{
			chan = 0;
			mib_local_mapping_set(MIB_WLAN_AUTO_CHAN_ENABLED, i, (void *)&chan);
			mib_local_mapping_get(MIB_WLAN_CHAN_NUM, i, (void *)&chan);
			if (chan == 0)
			{
				chan = 149;
				mib_local_mapping_set(MIB_WLAN_CHAN_NUM, i, (void *)&chan);
			}
		}
	}
#else//CROSS_BAND_BACKHAUL_STEERING
#ifdef BACKHAUL_LINK_SELECTION
	for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
		if(i == (agent_backhaul_link[0] - '0')) {
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, 0, (void *)&Entry);
			Entry.wlanDisabled = 0;
			mib_local_mapping_set(MIB_WLAN_DISABLED, i, (void *)&(Entry.wlanDisabled));
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
			Entry.wsc_disabled = 0;
#endif
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, 0);

			//disable auto channel for agent when backhaul link is 5G
#if defined (WLAN1_5G_SUPPORT)
			if (i == 1)
#else
			if (i == 0)
#endif
			{
				chan = 0;
				mib_local_mapping_set(MIB_WLAN_AUTO_CHAN_ENABLED, i, (void *)&chan);
				mib_local_mapping_get(MIB_WLAN_CHAN_NUM, i, (void *)&chan);
				if (chan == 0) {
					chan = 149;
					mib_local_mapping_set(MIB_WLAN_CHAN_NUM, i, (void *)&chan);
				}
			}
		}
	}
#else
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, 0, 0, (void *)&Entry);
	Entry.wlanDisabled = 0;
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
	Entry.wsc_disabled = 0;
#endif
	mib_chain_local_mapping_update(MIB_MBSSIB_TBL, 0, (void *)&Entry, 0);

	//disable auto channel for agent when backhaul link is 5G
#if defined (WLAN1_5G_SUPPORT)
	radio_index = 1;
#else
	radio_index = 0;
#endif
	{
		chan = 0;
		mib_local_mapping_set(MIB_WLAN_AUTO_CHAN_ENABLED, radio_index, (void *)&chan);
		mib_local_mapping_get(MIB_WLAN_CHAN_NUM, radio_index, (void *)&chan);
		if (chan == 0) {
			chan = 149;
			mib_local_mapping_set(MIB_WLAN_CHAN_NUM, radio_index, (void *)&chan);
		}
	}
#endif
#endif//CROSS_BAND_BACKHAUL_STEERING

#ifdef CONFIG_USER_DHCPCLIENT_MODE
	*dhcp_mode = mibVal = DHCPV4_LAN_CLIENT;
	mib_set(MIB_DHCP_MODE, (void *)&mibVal);
#endif

	char dummy_repeater_ssid[] = "MAP_RPT";

	for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
		for (j = 0; j < (WLAN_MBSSID_NUM+1); j++) {
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&Entry);
#ifdef WLAN_11K
			Entry.rm_activated = 1;
#endif
#ifdef WLAN_11V
			Entry.BssTransEnable = 1;
#endif

			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, j);
		}
	}

#ifdef WLAN_UNIVERSAL_REPEATER
	// Enable repeater
	// Enable vxd, set mode and enable wsc on vxd
#ifdef CROSS_BAND_BACKHAUL_STEERING
	if (strcmp(strVal, role_prev))
	{
		for (i = 0; i < NUM_WLAN_INTERFACE; i++)
		{
			mibVal = 1;
			mib_local_mapping_set(MIB_REPEATER_ENABLED1, i, (void *)&mibVal);
			mib_local_mapping_set(MIB_REPEATER_SSID1, i, (void *)dummy_repeater_ssid);
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_REPEATER_ITF_INDEX, (void *)&Entry);
			Entry.wlanDisabled = 0;
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
	// Enable vxd, set mode and enable wsc on vxd
			Entry.wsc_disabled = 0;
#endif
			Entry.wlanMode = CLIENT_MODE;
			Entry.multiap_bss_type = MAP_BACKHAUL_STA;
#ifdef WLAN_11W
			Entry.dotIEEE80211W = 0;
			Entry.sha256 = 0;
#endif
#ifdef MAP_GROUP_SUPPORT
			unsigned char map_group_ssid[MAX_SSID_LEN] = {0}, map_group_wpaPsk[MAX_PSK_LEN + 1] = {0};
			i = 0;
			for (i = 0; i < 2; i++)
			{
				mib_get_s(MIB_MAP_GROUP_SSID, (void *)&map_group_ssid, sizeof(map_group_ssid));
				if (strlen(map_group_ssid))
				{
					strcpy(Entry.ssid, map_group_ssid);
					mib_local_mapping_set(MIB_REPEATER_SSID1, i, (void *)map_group_ssid);
				}
				else
					strcpy(Entry.ssid, dummy_repeater_ssid);

				mib_get_s(MIB_MAP_GROUP_WPA_PSK, (void *)&map_group_wpaPsk, sizeof(map_group_wpaPsk));
				if (strlen(map_group_wpaPsk))
				{
					Entry.encrypt = WIFI_SEC_WPA2;
					Entry.wpaAuth = WPA_AUTH_PSK;
					strcpy(Entry.wpaPSK, map_group_wpaPsk);
					Entry.wpa2UnicastCipher = WPA_CIPHER_AES;
				}
			}
#else

			strcpy(Entry.ssid, dummy_repffeater_ssid);
#endif
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, WLAN_REPEATER_ITF_INDEX);
		}
	}
#else
#ifdef BACKHAUL_LINK_SELECTION
	i = 0;
	for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
		unsigned char enable_radio = 0;
		unsigned char multiap_bss_type = 0;
		if (i == (agent_backhaul_link[0] - '0')) {
			enable_radio = 1;
			multiap_bss_type = MAP_BACKHAUL_STA;
		}
#if defined(WLAN_MULTI_AP_ROLE_AUTO_SELECT)
		if(!strstr(role_prev, "auto"))
#endif
		{
		mib_local_mapping_set(MIB_REPEATER_ENABLED1, i, (void *)&enable_radio);
		mib_local_mapping_set(MIB_REPEATER_SSID1, i, (void *)dummy_repeater_ssid);
		mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_REPEATER_ITF_INDEX, (void *)&Entry);
		Entry.wlanDisabled = !enable_radio;
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
		Entry.wsc_disabled = !enable_radio;
#endif
		Entry.wlanMode = CLIENT_MODE;
#ifdef WLAN_11W
		Entry.dotIEEE80211W = 0;
		Entry.sha256 = 0;
#endif
#ifdef WLAN_11K
		Entry.rm_activated = 1;
#endif
#ifdef WLAN_11V
		Entry.BssTransEnable = 1;
#endif
		Entry.multiap_bss_type = multiap_bss_type;
#ifdef MAP_GROUP_SUPPORT
		unsigned char map_group_ssid[MAX_SSID_LEN]={0}, map_group_wpaPsk[MAX_PSK_LEN+1]={0};
		if(i == (agent_backhaul_link[0] - '0'))
		{
			mib_get_s(MIB_MAP_GROUP_SSID, (void *)&map_group_ssid,sizeof(map_group_ssid));
			if(strlen(map_group_ssid))
			{
				strcpy(Entry.ssid, map_group_ssid);
				mib_local_mapping_set(MIB_REPEATER_SSID1, i, (void *)map_group_ssid);
			}
			else
				strcpy(Entry.ssid, dummy_repeater_ssid);

			mib_get_s(MIB_MAP_GROUP_WPA_PSK, (void *)&map_group_wpaPsk,sizeof(map_group_wpaPsk));
			if(strlen(map_group_wpaPsk))
			{
				Entry.encrypt = WIFI_SEC_WPA2;
				Entry.wpaAuth = WPA_AUTH_PSK;
				strcpy(Entry.wpaPSK, map_group_wpaPsk);
				Entry.wpa2UnicastCipher = WPA_CIPHER_AES;
			}
		}
#else

		strcpy(Entry.ssid, dummy_repeater_ssid);
#endif
		mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, WLAN_REPEATER_ITF_INDEX);
		}
	}
#else
#ifdef WLAN_CTC_MULTI_AP
	for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
		mibVal = 0;
		mib_local_mapping_set(MIB_REPEATER_ENABLED1, i, (void *)&mibVal);
		mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_REPEATER_ITF_INDEX, (void *)&Entry);
		Entry.wlanDisabled = 1;
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
		Entry.wsc_disabled = 0;
#endif
		Entry.wlanMode = CLIENT_MODE;
		Entry.multiap_bss_type = 0;
		mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, WLAN_REPEATER_ITF_INDEX);
	}
#else
#if defined (WLAN1_5G_SUPPORT)
	radio_index = 1;
#else
	radio_index = 0;
#endif // WLAN1_5G_SUPPORT
	mibVal = 1;
	mib_local_mapping_set(MIB_REPEATER_ENABLED1, radio_index, (void *)&mibVal);
	mib_local_mapping_set(MIB_REPEATER_SSID1, radio_index, (void *)dummy_repeater_ssid);
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, radio_index, WLAN_REPEATER_ITF_INDEX, (void *)&Entry);
	Entry.wlanDisabled = 0;
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
	Entry.wsc_disabled = 0;
#endif
	Entry.wlanMode = CLIENT_MODE;
	Entry.multiap_bss_type = MAP_BACKHAUL_STA;
#ifdef WLAN_11W
	Entry.dotIEEE80211W = 0;
	Entry.sha256 = 0;
#endif
	strcpy(Entry.ssid, dummy_repeater_ssid);
#ifdef WLAN_11K
	Entry.rm_activated = 1;
#endif
#ifdef WLAN_11V
	Entry.BssTransEnable = 1;
#endif
	mib_chain_local_mapping_update(MIB_MBSSIB_TBL, radio_index, (void *)&Entry, WLAN_REPEATER_ITF_INDEX);
#endif // WLAN_CTC_MULTI_AP
#endif // BACKHAUL_LINK_SELECTION
#endif //CROSS_BAND_BACKHAUL_STEERING
#endif // WLAN_UNIVERSAL_REPEATER

	// if different from prev role, reset this mib to 0
	if (strcmp(strVal, role_prev)) {
		mibVal = 0;
		mib_set(MIB_MAP_CONFIGURED_BAND, (void *)&mibVal);
	}
#ifdef CONFIG_USER_RTK_SYSLOG
	check_syslogd_enable();
#endif
}

#if defined(WLAN_MULTI_AP_ROLE_AUTO_SELECT)
void setupMultiAPAuto(char *strVal, char *role_prev, char *agent_backhaul_link, unsigned char *dhcp_mode)
{
	int i=0, j=0;
	MIB_CE_MBSSIB_T Entry;
	unsigned char mibVal = 1;
	char opmode=0;

	// Set to auto
	mibVal = MAP_AUTO;
	mib_set(MIB_MAP_CONTROLLER, (void *)&mibVal);

#ifdef BACKHAUL_LINK_SELECTION
	for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
		if(i == (agent_backhaul_link[0] - '0')) {
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, 0, (void *)&Entry);
			Entry.wlanDisabled = 0;
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
			Entry.wsc_disabled = 0;
#endif
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, 0);
		}
	}
#else
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, 0, 0, (void *)&Entry);
	Entry.wlanDisabled = 0;
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
	Entry.wsc_disabled = 0;
#endif
	mib_chain_local_mapping_update(MIB_MBSSIB_TBL, 0, (void *)&Entry, 0);
#endif

#ifdef CONFIG_USER_RTK_BRIDGE_MODE
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	mib_get(MIB_OP_MODE, (void *)&opmode);
	if(opmode == BRIDGE_MODE){
		*dhcp_mode = mibVal = AUTO_DHCPV4_BRIDGE;
		mib_set(MIB_DHCP_MODE, (void *)&mibVal);
	}
#endif
#endif

	char dummy_repeater_ssid[] = "MAP_RPT";

#ifdef WLAN_UNIVERSAL_REPEATER
	// Enable repeater
	// Enable vxd, set mode and enable wsc on vxd
#ifdef BACKHAUL_LINK_SELECTION
	i = 0;
	for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
		unsigned char enable_radio = 0;
		unsigned char multiap_bss_type = 0;
		if (i == (agent_backhaul_link[0] - '0')) {
			enable_radio = 1;
			multiap_bss_type = MAP_BACKHAUL_STA;
		}
		mib_local_mapping_set(MIB_REPEATER_ENABLED1, i, (void *)&enable_radio);
		mib_local_mapping_set(MIB_REPEATER_SSID1, i, (void *)dummy_repeater_ssid);
		mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_REPEATER_ITF_INDEX, (void *)&Entry);
		Entry.wlanDisabled = !enable_radio;
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
		Entry.wsc_disabled = !enable_radio;
#endif
		Entry.wlanMode = CLIENT_MODE;
#ifdef WLAN_11W
		Entry.dotIEEE80211W = 0;
		Entry.sha256 = 0;
#endif
		Entry.multiap_bss_type = multiap_bss_type;
		strcpy(Entry.ssid, dummy_repeater_ssid);
		mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, WLAN_REPEATER_ITF_INDEX);
	}
#else
#ifdef WLAN_CTC_MULTI_AP
	mibVal = 1;
	for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
		mib_local_mapping_set(MIB_REPEATER_ENABLED1, i, (void *)&mibVal);
		mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_REPEATER_ITF_INDEX, (void *)&Entry);
		Entry.wlanDisabled = 0;
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
		Entry.wsc_disabled = 0;
#endif
		Entry.wlanMode = CLIENT_MODE;
		Entry.multiap_bss_type = WLAN_MAP_VXD_VALUE; //Set bss type to 128 for vxd
		mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, WLAN_REPEATER_ITF_INDEX);
	}
#else
#if defined (WLAN1_5G_SUPPORT)
	int radio_index = 1;
#else
	int radio_index = 0;
#endif // WLAN1_5G_SUPPORT
	mibVal = 1;
	mib_local_mapping_set(MIB_REPEATER_ENABLED1, radio_index, (void *)&mibVal);
	mib_local_mapping_set(MIB_REPEATER_SSID1, radio_index, (void *)dummy_repeater_ssid);
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, radio_index, WLAN_REPEATER_ITF_INDEX, (void *)&Entry);
	Entry.wlanDisabled = 0;
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
	Entry.wsc_disabled = 0;
#endif
	Entry.wlanMode = CLIENT_MODE;
	Entry.multiap_bss_type = MAP_BACKHAUL_STA;
#ifdef WLAN_11W
	Entry.dotIEEE80211W = 0;
	Entry.sha256 = 0;
#endif
	strcpy(Entry.ssid, dummy_repeater_ssid);
	mib_chain_local_mapping_update(MIB_MBSSIB_TBL, radio_index, (void *)&Entry, WLAN_REPEATER_ITF_INDEX);
#endif // WLAN_CTC_MULTI_AP
#endif // BACKHAUL_LINK_SELECTION
#endif // WLAN_UNIVERSAL_REPEATER

	// if different from prev role, reset this mib to 0
	if (strcmp(strVal, role_prev)) {
		mibVal = 0;
		mib_set(MIB_MAP_CONFIGURED_BAND, (void *)&mibVal);
	}
#ifdef CONFIG_USER_RTK_SYSLOG
	check_syslogd_enable();
#endif
}
#endif

void setupMultiAPDisabled()
{
	int i=0, j=0;
	MIB_CE_MBSSIB_T Entry;
	unsigned char mibVal = 1;
	char opmode=0;
	char rptr_enabled = 1;

#ifdef CONFIG_USER_RTK_BRIDGE_MODE
	mib_get(MIB_OP_MODE, (void *)&opmode);
#endif
#if defined(WLAN_UNIVERSAL_REPEATER) && defined(CONFIG_USER_RTK_REPEATER_MODE)
	mib_get(MIB_REPEATER_MODE, (void *)&rptr_enabled);
#endif

	// Set to disabled
	mibVal = MAP_DISABLED;
	mib_set(MIB_MAP_CONTROLLER, (void *)&mibVal);

	for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
#ifdef WLAN_UNIVERSAL_REPEATER
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
		if((opmode == BRIDGE_MODE) && (rptr_enabled == 1) ){
			//enable repeater
			mibVal = 1;
			mib_local_mapping_set(MIB_REPEATER_ENABLED1, i, (void *)&mibVal);
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_REPEATER_ITF_INDEX, (void *)&Entry);
			Entry.wlanDisabled = 0;
			Entry.multiap_bss_type = 0;
			Entry.wlanMode = CLIENT_MODE;
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, WLAN_REPEATER_ITF_INDEX);
		}else
#endif
		{
			// Disable repeater
			mibVal = 0;
			mib_local_mapping_set(MIB_REPEATER_ENABLED1, i, (void *)&mibVal);
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_REPEATER_ITF_INDEX, (void *)&Entry);
			Entry.wlanDisabled = 1;
			Entry.multiap_bss_type = 0;
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, WLAN_REPEATER_ITF_INDEX);
		}
#endif
		for (j = 0; j < NUM_VWLAN_INTERFACE; j++) {
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&Entry);
			Entry.multiap_bss_type = 0;
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, j);
		}
	}

	// reset configured band to 0
	mibVal = 0;
	mib_set(MIB_MAP_CONFIGURED_BAND, (void *)&mibVal);
}
#endif

/*copy from startup.c:sys_setup()*/
/*it should be written in the utility.c,
  but this function need to link with -lcrypt.(crypt).
  increse the memory size about 300kbytes
  we move the real function to systemd
  */
void rtk_util_update_user_account(void)
{
	system("sysconf send_unix_sock_message /var/run/systemd.usock do_update_user_account");
}

void rtk_util_update_boa_user_account_ex(int logout_users)
{
	char cmd[256] = {0};
	snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_write_boa_passwd_file %d", logout_users);
	system(cmd);
}

void rtk_util_update_boa_user_account(void)
{
	rtk_util_update_boa_user_account_ex(0);
}

#ifdef CONFIG_USER_RTK_FASTPASSNF_MODULE
int rtk_fastpassnf_smbspeedup(int enable_shortcut)
{
	char serverIP[INET_ADDRSTRLEN], cmd[128];
	struct in_addr inAddr;
	unsigned char smba_enable = 0;

	if (enable_shortcut)
	{
		mib_get_s(MIB_SAMBA_ENABLE, (void *)&smba_enable, sizeof(smba_enable));
		if (2 == smba_enable)
		{
			/* remote samba allowed only */
			enable_shortcut = 0;
		}
	}

	if(enable_shortcut)
	{
		if (getInAddr((char *)LANIF, IP_ADDR, &inAddr) == 1) {
			strncpy(serverIP, inet_ntoa(inAddr), sizeof(serverIP));
		} else {
			getMIB2Str(MIB_ADSL_LAN_IP, serverIP);
		}

		snprintf(cmd, sizeof(cmd), "echo set 1 sip %s > /proc/fastPassNF/PASS_SAMBA", serverIP);
	}
	else
	{
		snprintf(cmd, sizeof(cmd), "echo set 0 > /proc/fastPassNF/PASS_SAMBA");
	}

	system(cmd);

	return 0;
}

int rtk_fastpassnf_ipsecspeedup(int enable_shortcut)
{
	char cmd[128];
	if(enable_shortcut)
	{
		snprintf(cmd, sizeof(cmd), "echo set 1 hook 0x1f > /proc/fastPassNF/PASS_IPSEC");
	}
	else
	{
		snprintf(cmd, sizeof(cmd), "echo set 0 > /proc/fastPassNF/PASS_IPSEC");
	}

	system(cmd);

	return 0;
}

#endif
#ifdef CONFIG_USER_RTK_FAST_BR_PASSNF_MODULE
int rtk_fastpassnf_br_ipsecspeedup(int enable_shortcut)
{
	char cmd[128];
	if(enable_shortcut)
	{
		snprintf(cmd, sizeof(cmd), "echo set 1 hook 0x1f > /proc/fastBrPassNF/PASS_IPSEC");
	}
	else
	{
		snprintf(cmd, sizeof(cmd), "echo set 0 > /proc/fastBrPassNF/PASS_IPSEC");
	}

	system(cmd);

	return 0;
}
#endif

#if defined(CONFIG_GPON_FEATURE)
#if defined(DHCPV6_ISC_DHCP_4XX)
// return 1: found IA_NA.
// return 0: can not find IA_NA.
int getIANAInfo(const char *fname, IANA_INFO_Tp pInfo)
{
	FILE *fp;
	char temps[0x100];
	char *str, *endStr;
	char ifIndex_str[16]={0};
	int offset, RNTime, RBTime, PLTime, MLTime;
	struct in6_addr addr6;
	int in_lease6=0, in_iapd=0, in_iana=0;

	int ret=0;

	if ((fp = fopen(fname, "r")) == NULL)
	{
		printf("Open file %s fail !\n", fname);
		return 0;
	}

	while (fgets(temps,0x100,fp))
	{
		if (temps[strlen(temps)-1]=='\n')
			temps[strlen(temps)-1] = 0;

		// Get interface
		if ( str=strstr(temps, "interface") )
		{
			offset = strlen("interface")+2;
			if ( endStr=strchr(str+offset, '"')) {
				*endStr=0;
				sscanf(str+offset, "%s", &ifIndex_str);
			}
			memcpy(pInfo->wanIfname, ifIndex_str,  sizeof(ifIndex_str));
		}

		if (str=strstr(temps, "lease6")) {
			// clean pInfo
			memset(pInfo, 0, sizeof(DLG_INFO_T));
			in_lease6 = 1;
		}
		if (str=strstr(temps, "ia-na"))
		{
			in_iapd = 1;
			ret = 1;
			fgets(temps,0x100,fp);

			// Get renew
			fgets(temps,0x100,fp);
			if ( str=strstr(temps, "renew") ) {
				offset = strlen("renew")+1;
				if ( endStr=strchr(str+offset, ';')) {
					*endStr=0;
					sscanf(str+offset, "%u", &RNTime);
					pInfo->RNTime = RNTime;
				}
			}

			// Get rebind
			fgets(temps,0x100,fp);
			if ( str=strstr(temps, "rebind")) {
				offset = strlen("rebind")+1;
				if ( endStr=strchr(str+offset, ';')) {
					*endStr=0;
					sscanf(str+offset, "%u", &RBTime);
					pInfo->RBTime = RBTime;
				}
			}

			// the rest of ia-pd declaration
			while (in_iapd) {
				// Get prefix
				fgets(temps,0x100,fp);
				if ( str=strstr(temps, "iaaddr")) {
					in_iana = 1;
					offset = strlen("iaaddr")+1;
					if ( endStr=strchr(str+offset, ' ')) {
						*endStr=0;

						// IA_NA address
						inet_pton(PF_INET6, (str+offset), &addr6);
						memcpy(pInfo->ia_na, &addr6, IP6_ADDR_LEN);
					}

					fgets(temps,0x100,fp);

					// Get preferred-life
					fgets(temps,0x100,fp);
					if ( str=strstr(temps, "preferred-life")) {
						offset = strlen("preferred-life")+1;
						if ( endStr=strchr(str+offset, ';') ) {
							*endStr=0;
							sscanf(str+offset, "%u", &PLTime);
							pInfo->PLTime = PLTime;
						}
					}

					// Get max-life
					fgets(temps,0x100,fp);
					if( str=strstr(temps, "max-life")) {
						offset = strlen("max-life")+1;
						if ( endStr=strchr(str+offset, ';')) {
							*endStr=0;
							sscanf(str+offset, "%u", &MLTime);
							pInfo->MLTime = MLTime;
						}
					}
				}
				else if (strstr(temps, "}")) {
					if (in_iana)
						in_iana = 0;
					else
						in_iapd = 0;
				}
			}
		}
		if ( str=strstr(temps, "dhcp6.name-servers")) {
			offset = strlen("dhcp6.name-servers")+1;
			if ( endStr=strchr(str+offset, ';')) {
				*endStr=0;
				//printf("Name server=%s\n", str+offset);
				memcpy(pInfo->nameServer, (unsigned char *)(str+offset),  256);
			}
		}
		if (strstr(temps, "}")) {
			if (in_lease6)
				in_lease6 = 0;
		}
	}
	fclose(fp);

	return ret;
}
#endif // defined(DHCPV6_ISC_DHCP_4XX)

void dumpWanInfo(const char* filename, wan_info_t *p){
/*
1byte: wan index: (ifIndex)
32byte ServiceName: (ifname)
1byte ServiceType: 1:Internet, 2:VoIP, 3:TR069+Internet(serviceType)
1byte ConnectionType: 1 (IP-routed): 01 (protocol)
1byte ConnectionStatus: 2:Connected, 5:Disconnected (IPv4Status, IPv6Status)
1byte AccessType: 1:DHCP, 3:PPPoE (protocol+ipDhcp)
4byte IP: 5FA5F93A (ipAddr)
4byte Mask : FFFFFFF0 (mask)
4byte Gateway: 5FA5F931 (remoteIp)
2byte TCI (PBIT+VID): 001E (vprio+vid)
1byte Option60: 0 : 00 (enable_opt_60)
1byte Switch: hw ont always return 01 (Unknown)
6byte MAC : 8038BC5FE2E6 (wanMac)
1byte Priority: hw ont always return 01 (Unknown)
1byte L2-encap-type: hw ont always return 01 (Unknown)
1byte IPv4 Enable/Disable: 01 (ipver)
1byte IPv6 Enable/Disable: 00 (ipver => 1: IPv4, 2:IPv6, 3: IPv4 and IPv6)
16byte IPv6 Prefix: Following items fill zero when IPv6 is Disable (IPv6Prefix)
1byte Prefix Length (IPv6Prefix_len)
1byte Prefix access mode (IPv6Prefix_Mode)
4byte Prefix Preferred time (IPv6Prefix_PLTime)
4byte Prefix Valid time (IPv6Prefix_MLTime)
16byte IPv6 Address (IPv6_Address)
1byte IPv6 address Status: (IPv6Addr_Status)
1byte IPv6 Address Access Mode (IPv6Addr_Mode ==> enum { IPV6_WAN_NONE = 0, IPV6_WAN_AUTO = 1, IPV6_WAN_STATIC = 2, IPV6_WAN_DSLITE = 4, IPV6_WAN_6RD = 8, IPV6_WAN_DHCP = 16 })
4byte IPv6 address Preferred time (IPv6Addr_PLTime)
4byte IPv6 address Valid time (IPv6Addr_MLTime)
*/

	FILE *fp=NULL;
	char wanMac[30]={0};
#ifdef CONFIG_IPV6
	int i,j;
	char buf[256], str_ip6[INET6_ADDRSTRLEN];
#endif

	if(!(fp = fopen(filename, "a+")))
		return;

	fprintf(fp, "******************\n");
	fprintf(fp, "ifindex=%d\n",p->ifIndex);
	fprintf(fp, "ifname=%s\n",p->ifname);
	sprintf(wanMac,"%02x%02x%02x%02x%02x%02x",p->MacAddr[0],p->MacAddr[1],p->MacAddr[2],p->MacAddr[3],p->MacAddr[4],p->MacAddr[5]);
	fprintf(fp, "wanMac=%s\n", wanMac);
	fprintf(fp, "vlan=%d\n",p->vlan.enable);
	fprintf(fp, "vid=%d\n",p->vlan.vid);
	fprintf(fp, "vprio=%d\n",p->vlan.vprio);
	fprintf(fp, "vpass=%d\n",p->vlan.vpass);
	fprintf(fp, "cmode=%d\n", p->cmode);
	fprintf(fp, "protocol=%s\n", p->protocol);
	fprintf(fp, "ipver=%d\n", p->ipver);
	fprintf(fp, "serviceType=%s\n",p->serviceType);
	fprintf(fp, "IPv4Status=%s\n",p->ipv4.status);
	fprintf(fp, "ipDhcp=%d\n",p->ipv4.isDhcp);
	fprintf(fp, "ipAddr=%08x\n",p->ipv4.ipAddr);
	fprintf(fp, "remoteIp=%08x\n",p->ipv4.gwAddr);
	fprintf(fp, "mask=%08x\n",p->ipv4.mask);
	if(p->ipv4.isDhcp){
#ifdef CONFIG_USER_DHCP_OPT_GUI_60
		fprintf(fp, "enable_opt_60=%d\n",p->ipv4.dhcp_config.enable_opt_60);
		fprintf(fp, "opt60_val=%s\n",p->ipv4.dhcp_config.opt60_val);
		fprintf(fp, "enable_opt_61=%d\n",p->ipv4.dhcp_config.enable_opt_61);
		fprintf(fp, "iaid=%d\n",p->ipv4.dhcp_config.iaid);
		fprintf(fp, "duid_type=%d\n",p->ipv4.dhcp_config.duid_type);
		fprintf(fp, "duid_id=%s\n",p->ipv4.dhcp_config.duid_id);
		fprintf(fp, "enable_opt_125=%d\n",p->ipv4.dhcp_config.enable_opt_125);
		fprintf(fp, "manufacturer=%s\n",p->ipv4.dhcp_config.manufacturer);
		fprintf(fp, "product_class=%s\n",p->ipv4.dhcp_config.product_class);
		fprintf(fp, "model_name=%s\n",p->ipv4.dhcp_config.model_name);
		fprintf(fp, "serial_num=%s\n",p->ipv4.dhcp_config.serial_num);
#endif
	}

#ifdef CONFIG_IPV6
	fprintf(fp, "IPv6Status=%s\n",p->ipv6.status);
	buf[0]=0;
	if (p->ipv6.totalIPv6Num) {
		for (j=0; j<p->ipv6.totalIPv6Num; j++) {
			if (j == 0){
				sprintf(buf, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[0], (int)p->ipv6.ip6_addr[j].addr.s6_addr[1],
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[2], (int)p->ipv6.ip6_addr[j].addr.s6_addr[3],
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[4], (int)p->ipv6.ip6_addr[j].addr.s6_addr[5],
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[6], (int)p->ipv6.ip6_addr[j].addr.s6_addr[7],
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[8], (int)p->ipv6.ip6_addr[j].addr.s6_addr[9],
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[10], (int)p->ipv6.ip6_addr[j].addr.s6_addr[11],
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[12], (int)p->ipv6.ip6_addr[j].addr.s6_addr[13],
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[14], (int)p->ipv6.ip6_addr[j].addr.s6_addr[15]);
			}else{
				sprintf(buf+strlen(buf), ",%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[0], (int)p->ipv6.ip6_addr[j].addr.s6_addr[1],
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[2], (int)p->ipv6.ip6_addr[j].addr.s6_addr[3],
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[4], (int)p->ipv6.ip6_addr[j].addr.s6_addr[5],
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[6], (int)p->ipv6.ip6_addr[j].addr.s6_addr[7],
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[8], (int)p->ipv6.ip6_addr[j].addr.s6_addr[9],
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[10], (int)p->ipv6.ip6_addr[j].addr.s6_addr[11],
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[12], (int)p->ipv6.ip6_addr[j].addr.s6_addr[13],
							  (int)p->ipv6.ip6_addr[j].addr.s6_addr[14], (int)p->ipv6.ip6_addr[j].addr.s6_addr[15]);
			}
		}

		fprintf(fp, "IPv6Addr=%s\n", buf);
	}

#if defined(DHCPV6_ISC_DHCP_4XX)
	// DHCPv6
	for(i=0;i<IP6_ADDR_LEN;i++){
		if(i==0){
			fprintf(fp, "IPv6Addr=%02x", p->ipv6.ianaInfo.ia_na[i]);
		}else{
			fprintf(fp, "%02x", p->ipv6.ianaInfo.ia_na[i]);
		}
	}
	fprintf(fp, "\n");
	fprintf(fp, "IPv6Addr_Status=%d\n", IPV6_WAN_DHCP);
	fprintf(fp, "IPv6Addr_Mode=%d\n", IPV6_WAN_DHCP);
	fprintf(fp, "IPv6Addr_PLTime=%u\n", p->ipv6.ianaInfo.PLTime);
	fprintf(fp, "IPv6Addr_MLTime=%u\n", p->ipv6.ianaInfo.MLTime);

	for(i=0;i<IP6_ADDR_LEN;i++){
		if(i==0){
			fprintf(fp, "IPv6Prefix=%02x", p->ipv6.dlgInfo.prefixIP[i]);
		}else{
			fprintf(fp, "%02x", p->ipv6.dlgInfo.prefixIP[i]);
		}
	}
	fprintf(fp, "\n");
	fprintf(fp, "IPv6Prefix_len=%d\n", p->ipv6.dlgInfo.prefixLen);
	fprintf(fp, "IPv6Prefix_Mode=%d\n", IPV6_WAN_DHCP);
	fprintf(fp, "IPv6Prefix_PLTime=%u\n", p->ipv6.dlgInfo.PLTime);
	fprintf(fp, "IPv6Prefix_MLTime=%u\n", p->ipv6.dlgInfo.MLTime);
#endif // DHCPV6_ISC_DHCP_4XX
#endif // CONFIG_IPV6

	fclose(fp);
}

/**
 *    Get WAN current infomation for this wan interface
 *    Input:
 *      unsigned int ifIndex => wan index in MIB_CE_ATM_VC_T, such as ifIndex=130816
 *    Output:
 *      wan_info_t *p => run time wan information for this ifIndex
 *
 *    Return:
 *      0 : Success
 *      -1: fail
 **/
int getWanInfoByIfIndex(unsigned int ifIndex, wan_info_t *p)
{
	unsigned char found=0;
	unsigned int i, num;
	MIB_CE_ATM_VC_T tmp;
	struct in_addr inAddr;
	char *tempIpAddr;
	int flags;
	FILE *fp;
	int linkState, dslState=0, ethState=0;
	MEDIA_TYPE_T mType;
#ifdef CONFIG_IPV6
#ifdef DHCPV6_ISC_DHCP_4XX
	unsigned char leasefile[128]={0};
#endif
#endif

	if( (p==NULL) || (ifIndex==DUMMY_IFINDEX) ) return -1;

	num = mib_chain_total( MIB_ATM_VC_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_ATM_VC_TBL, i, (void*)&tmp ))
			continue;

		if(tmp.ifIndex != ifIndex)
			continue;

		found=1;
		memset(p, 0x0, sizeof(wan_info_t)); // init

		// assigned interface info
		p->ifIndex = tmp.ifIndex;
		ifGetName(p->ifIndex, p->ifname, sizeof(p->ifname));
		memcpy(p->MacAddr, tmp.MacAddr, MAC_ADDR_LEN);
		p->ipver = tmp.IpProtocol;	// 1: IPv4, 2:IPv6, 3: IPv4 and IPv6
		p->cmode = tmp.cmode;
		switch(p->cmode) {
			case CHANNEL_MODE_BRIDGE: // 0
				strcpy(p->protocol, "Bridged");
				break;
			case CHANNEL_MODE_IPOE:	// 1
				strcpy(p->protocol, "IPoE");
				break;
			case CHANNEL_MODE_PPPOE: // 2
				strcpy(p->protocol, "PPPoE");
				break;
			case CHANNEL_MODE_PPPOA: // 3
				strcpy(p->protocol, "PPPoA");
				break;
			case CHANNEL_MODE_RT1483: // 4
				strcpy(p->protocol, "RT1483");
				break;
			case CHANNEL_MODE_RT1577: // 5
				strcpy(p->protocol, "RT1577");
				break;
			case CHANNEL_MODE_DSLITE: // 6
				strcpy(p->protocol, "DSLITE");
				break;
			case CHANNEL_MODE_6RD: // 8
				strcpy(p->protocol, "6rd");
				break;
			case CHANNEL_MODE_6IN4: // 9
				strcpy(p->protocol, "6in4");
				break;
			case CHANNEL_MODE_6TO4: // 10
				strcpy(p->protocol, "6to4");
				break;
			default:
				break;
		}

		getServiceType(p->serviceType,tmp.applicationtype);

		// assigned vlan info
		if(tmp.vlan){
			p->vlan.enable = tmp.vlan;
			p->vlan.vid = tmp.vid;
			p->vlan.vprio = tmp.vprio;
			p->vlan.vpass = tmp.vpass;
		}

		// assigned IPv4 info
		if (getInAddr( p->ifname, IP_ADDR, (void *)&inAddr) == 1)
		{
			tempIpAddr = inet_ntoa(inAddr);
			if (getInFlags( p->ifname, &flags) == 1)
				if ((strcmp(tempIpAddr, "10.0.0.1") == 0) && flags & IFF_POINTOPOINT){	// IP Passthrough or IP unnumbered{
					printf("Invalid Address, %s\n", tempIpAddr);
				}else if (strcmp(tempIpAddr, "64.64.64.64") == 0){
					printf("Invalid Address, %s\n", tempIpAddr);
				}else{
					memcpy(&(p->ipv4.ipAddr), &inAddr, sizeof(struct in_addr));
				}
		}

		if(getInAddr( p->ifname, DST_IP_ADDR, (void *)&inAddr) == 1){
			tempIpAddr = inet_ntoa(inAddr);
			if (getInFlags( p->ifname, &flags) == 1){
				if (flags & IFF_BROADCAST) {
					unsigned char value[32];
					char remoteIp[20];
					snprintf(value, 32, "%s.%s", (char *)MER_GWINFO, p->ifname);
					if (fp = fopen(value, "r")) {
						fscanf(fp, "%s\n", remoteIp);
						if (inet_aton(remoteIp, &(p->ipv4.gwAddr)) == 0) {
							fprintf(stderr, "Invalid Gateway address\n");
						}
						fclose(fp);
					}
				}
			}
		}

		getInAddr( p->ifname, SUBNET_MASK, (void *)&(p->ipv4.mask));

		// ipv4 dhcp setting
		p->ipv4.isDhcp = tmp.ipDhcp;
		if(p->ipv4.isDhcp){
#ifdef CONFIG_USER_DHCP_OPT_GUI_60
			p->ipv4.dhcp_config.enable_opt_60 = tmp.enable_opt_60;
			strncpy(p->ipv4.dhcp_config.opt60_val, tmp.opt60_val, sizeof(tmp.opt60_val));
			p->ipv4.dhcp_config.enable_opt_61 = tmp.enable_opt_61;
			p->ipv4.dhcp_config.iaid = tmp.iaid;
			p->ipv4.dhcp_config.duid_type = tmp.duid_type;
			strncpy(p->ipv4.dhcp_config.duid_id, tmp.duid_id, sizeof(tmp.duid_id));
			p->ipv4.dhcp_config.enable_opt_125 = tmp.enable_opt_125;
			strncpy(p->ipv4.dhcp_config.manufacturer, tmp.manufacturer, sizeof(tmp.manufacturer));
			strncpy(p->ipv4.dhcp_config.product_class, tmp.product_class, sizeof(tmp.product_class));
			strncpy(p->ipv4.dhcp_config.model_name, tmp.model_name, sizeof(tmp.model_name));
			strncpy(p->ipv4.dhcp_config.serial_num, tmp.serial_num, sizeof(tmp.serial_num));
#endif
		}


#ifdef CONFIG_DEV_xDSL
		// check for xDSL link
		if (!adsl_drv_get(RLCM_GET_LINK_SPEED, (void *)&vLs, RLCM_GET_LINK_SPEED_SIZE) || vLs.upstreamRate == 0)
			dslState = 0;
		else
			dslState = 1;
#endif
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN)
		// todo
		ethState = 1;
#endif

		mType = MEDIA_INDEX(p->ifIndex);
		if (mType == MEDIA_ATM)
			linkState = dslState;
#ifdef CONFIG_PTMWAN
		else if (mType == MEDIA_PTM)
			linkState = dslState && ethState;//???
#endif /*CONFIG_PTMWAN*/
		else if (mType == MEDIA_ETH)
			linkState = ethState;
#ifdef WLAN_WISP
		else if (mType == MEDIA_WLAN){
			char wisp_name[16];
			getWispWanName(wisp_name, ETH_INDEX(p->ifIndex));
			linkState = get_net_link_status(wisp_name);
		}
#endif
		else
			linkState = 0;

		// set status flag
		if (getInFlags( p->ifname, &flags) == 1)
		{
			if (flags & IFF_UP) {
				if (!linkState) {
					p->ipv4.status = (char *)IF_DOWN;
//					sEntry[i].itf_state = 0;
				}
				else {
					if (p->cmode == CHANNEL_MODE_BRIDGE) {
						p->ipv4.status = (char *)IF_UP;
//						sEntry[i].itf_state = 1;
					}
					else
						if (getInAddr(p->ifname, IP_ADDR, (void *)&inAddr) == 1) {
							tempIpAddr = inet_ntoa(inAddr);
							if (strcmp(tempIpAddr, "64.64.64.64")) {
								p->ipv4.status = (char *)IF_UP;
//								sEntry[i].itf_state = 1;
							}else {
								p->ipv4.status = (char *)IF_DOWN;
//								sEntry[i].itf_state = 0;
							}
						}
						else {
							p->ipv4.status = (char *)IF_DOWN;
//							sEntry[i].itf_state = 0;
						}
				}
			}
			else {
				p->ipv4.status = (char *)IF_DOWN;
//				sEntry[i].itf_state = 0;
			}
		}
		else {
			p->ipv4.status = (char *)IF_NA;
//			sEntry[i].itf_state = -1;
		}

#ifdef CONFIG_IPV6
		p->ipv6.totalIPv6Num = getifip6(p->ifname, IPV6_ADDR_UNICAST, p->ipv6.ip6_addr, MAX_IPV6_NUM);
		if(p->ipv6.totalIPv6Num == 0){
			p->ipv6.status = (char *)IF_DOWN;
		}else{
			p->ipv6.status = (char *)IF_UP;
		}

#ifdef DHCPV6_ISC_DHCP_4XX
		snprintf(leasefile, sizeof(leasefile), "%s.%s", PATH_DHCLIENT6_LEASE, p->ifname);
		getIANAInfo(leasefile, &(p->ipv6.ianaInfo));
		getLeasesInfo(leasefile, &(p->ipv6.dlgInfo));
#endif // DHCPV6_ISC_DHCP_4XX
#endif // CONFIG_IPV6
		if(found)
			break;
	}

	return 0;
}

void dumpAllWanToFile(const char* filename){
	unsigned int i,num;
	MIB_CE_ATM_VC_T tmp;
	wan_info_t p;
	unlink(WAN_INFO_FILE);
	num = mib_chain_total( MIB_ATM_VC_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_ATM_VC_TBL, i, (void*)&tmp ))
			continue;

		getWanInfoByIfIndex(tmp.ifIndex, &p);
		dumpWanInfo(filename, &p);
	}
}
#endif // defined(CONFIG_GPON_FEATURE)

#if 1//FTP_ACCOUNT_INDEPENDENT
void delFtpAccessFw(){
	char FW_CHAIN[32]={0};
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	snprintf(FW_CHAIN, sizeof(FW_CHAIN), "%s", FW_INACC);
#else
	snprintf(FW_CHAIN, sizeof(FW_CHAIN), "%s", FW_INPUT);
#endif

	syslog(LOG_INFO, "iptables:in %s(%d)\r\n", __FUNCTION__, __LINE__);

	va_cmd(IPTABLES, 4, 1, "-D", (char *)FW_CHAIN, "-j", (char *)FW_FTP_ACCOUNT);
	//iptables -D INPUT -j ftp_account
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_FTP_ACCOUNT);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_FTP_ACCOUNT);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-D", (char *)FW_INPUT, "-j", (char *)FW_FTP_ACCOUNT);
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)FW_FTP_ACCOUNT);
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_FTP_ACCOUNT);
#endif
}

void setFtpAccessFw(int enable){
	char FW_CHAIN[32]={0};
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	snprintf(FW_CHAIN, sizeof(FW_CHAIN), "%s", FW_INACC);
#else
	snprintf(FW_CHAIN, sizeof(FW_CHAIN), "%s", FW_INPUT);
#endif
	printf("%s %d ftp_enable=%d\n", __FUNCTION__, __LINE__, enable);

	syslog(LOG_INFO, "iptables:in %s(%d):ftp_enable[%d]\r\n", __FUNCTION__, __LINE__, enable);

	//iptables -N ftp_account
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_FTP_ACCOUNT);
	//iptables -A INPUT -j ftp_account
	va_cmd(IPTABLES, 4, 1, "-I", (char *)FW_CHAIN, "-j", (char *)FW_FTP_ACCOUNT);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_FTP_ACCOUNT);
	va_cmd(IP6TABLES, 4, 1, "-I", (char *)FW_INPUT, "-j", (char *)FW_FTP_ACCOUNT);
#endif

	if(enable == 0){//0:local disable,remote disable;
		//iptables -A ftp_account -i nas+  -p TCP --dport 21 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_DROP);

	   //iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_DROP);

	    //iptables -A ftp_account -i br0  -p TCP --dport 21 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_DROP);

#ifdef CONFIG_IPV6
		//iptables -A ftp_account -i nas+  -p TCP --dport 21 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "21", "-j", (char *)FW_DROP);

		//iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "21", "-j", (char *)FW_DROP);

		//iptables -A ftp_account -i br0	-p TCP --dport 21 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "21", "-j", (char *)FW_DROP);
#endif
	}
	else if(enable == 1){//1:local enbale,remote disable;
		//iptables -A ftp_account -i nas+  -p TCP --dport 21 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_DROP);

	   //iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_DROP);

	    //iptables -A ftp_account -i br0  -p TCP --dport 21 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_ACCEPT);

#ifdef CONFIG_IPV6
		//iptables -A ftp_account -i nas+  -p TCP --dport 21 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "21", "-j", (char *)FW_DROP);

		//iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "21", "-j", (char *)FW_DROP);

		//iptables -A ftp_account -i br0	-p TCP --dport 21 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "21", "-j", (char *)FW_ACCEPT);
#endif
	}else if(enable == 2){//2:local disable,remote enable;
        //iptables -A ftp_account -i nas+  -p TCP --dport 21 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_ACCEPT);

        //iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_ACCEPT);

        //iptables -A ftp_account -i br0  -p TCP --dport 21 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_DROP);

#ifdef CONFIG_IPV6
        //iptables -A ftp_account -i nas+  -p TCP --dport 21 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_ACCEPT);

        //iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_ACCEPT);

        //iptables -A ftp_account -i br0  -p TCP --dport 21 -j DROP
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_DROP);
#endif
    }else if(enable == 3){//3:local enbale,remote enbale
        //iptables -A ftp_account -i nas+  -p TCP --dport 21 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_ACCEPT);

        //iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_ACCEPT);

        //iptables -A ftp_account -i br0  -p TCP --dport 21 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_ACCEPT);

#ifdef CONFIG_IPV6
        //iptables -A ftp_account -i nas+  -p TCP --dport 21 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_ACCEPT);

        //iptables -A ftp_account -i ppp+  -p TCP --dport 21 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_ACCEPT);

        //iptables -A ftp_account -i br0  -p TCP --dport 21 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_FTP_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "21", "-j", (char *)FW_ACCEPT);
#endif
    }else{
		printf("[%s:%s:%d] invlaid ftp enable (%d) status!",__FILE__,__FUNCTION__,__LINE__, enable);
	}
}

const char FW_TELNET_ACCOUNT[] = "telnet_account";
void delTelnetAccessFw(){
	char FW_CHAIN[32]={0};
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	snprintf(FW_CHAIN, sizeof(FW_CHAIN), "%s", FW_INACC);
#else
	snprintf(FW_CHAIN, sizeof(FW_CHAIN), "%s", FW_INPUT);
#endif
	syslog(LOG_INFO, "iptables:in %s(%d)\r\n", __FUNCTION__, __LINE__);

	//iptables -X telnet_account
	va_cmd(IPTABLES, 4, 1, "-D", (char *)FW_CHAIN, "-j", (char *)FW_TELNET_ACCOUNT);
	//iptables -D INPUT -j telnet_account
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_TELNET_ACCOUNT);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_TELNET_ACCOUNT);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-D", (char *)FW_INPUT, "-j", (char *)FW_TELNET_ACCOUNT);
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)FW_TELNET_ACCOUNT);
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_TELNET_ACCOUNT);
#endif
}

void setTelnetAccessFw(int enable){
	char FW_CHAIN[32]={0};
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	snprintf(FW_CHAIN, sizeof(FW_CHAIN), "%s", FW_INACC);
#else
	snprintf(FW_CHAIN, sizeof(FW_CHAIN), "%s", FW_INPUT);
#endif
	syslog(LOG_INFO, "iptables:in %s(%d):telnet_enable[%d]\r\n", __FUNCTION__, __LINE__, enable);

	//iptables -N telnet_account
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_TELNET_ACCOUNT);
	//iptables -A INPUT -j telnet_account
	va_cmd(IPTABLES, 4, 1, "-I", (char *)FW_CHAIN, "-j", (char *)FW_TELNET_ACCOUNT);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_TELNET_ACCOUNT);
	va_cmd(IP6TABLES, 4, 1, "-I", (char *)FW_INPUT, "-j", (char *)FW_TELNET_ACCOUNT);
#endif

	if(enable == 0){//0:local disable,remote disable;
		//iptables -A telnet_account -i nas+  -p TCP --dport 23 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_DROP);

	   //iptables -A telnet_account -i ppp+  -p TCP --dport 23 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_DROP);

#ifdef CONFIG_CU
	    //iptables -A telnet_account -i br0  -p TCP --dport 23 -j REJECT --reject-with tcp-reset
        va_cmd(IPTABLES, 12, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", "REJECT", "--reject-with", "tcp-reset");
#else
	    //iptables -A telnet_account -i br0  -p TCP --dport 23 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_DROP);
#endif

#ifdef CONFIG_IPV6
		//iptables -A telnet_account -i nas+  -p TCP --dport 23 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "23", "-j", (char *)FW_DROP);

		//iptables -A telnet_account -i ppp+  -p TCP --dport 23 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "23", "-j", (char *)FW_DROP);

#ifdef CONFIG_CU
		//ip6tables -A telnet_account -i br0  -p TCP --dport 23 -j REJECT --reject-with tcp-reset
		va_cmd(IP6TABLES, 12, 1, "-A", (char *)FW_TELNET_ACCOUNT,
		(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "23", "-j", "REJECT", "--reject-with", "tcp-reset");
#else
		//iptables -A telnet_account -i br0	-p TCP --dport 23 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "23", "-j", (char *)FW_DROP);
#endif
#endif
	}
	else if(enable == 1){//1:local enbale,remote disable;
		//iptables -A telnet_account -i nas+  -p TCP --dport 23 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_DROP);

	   //iptables -A telnet_account -i ppp+  -p TCP --dport 23 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_DROP);

	    //iptables -A telnet_account -i br0  -p TCP --dport 23 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_ACCEPT);

#ifdef CONFIG_IPV6
		//iptables -A telnet_account -i nas+  -p TCP --dport 23 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "23", "-j", (char *)FW_DROP);

		//iptables -A telnet_account -i ppp+  -p TCP --dport 23 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "23", "-j", (char *)FW_DROP);

		//iptables -A telnet_account -i br0	-p TCP --dport 23 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "23", "-j", (char *)FW_ACCEPT);
#endif
	}else if(enable == 2){//2:local disable,remote enable;
        //iptables -A telnet_account -i nas+  -p TCP --dport 23 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_ACCEPT);

        //iptables -A telnet_account -i ppp+  -p TCP --dport 23 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_ACCEPT);

#ifdef CONFIG_CU
		//iptables -A telnet_account -i br0  -p TCP --dport 23 -j REJECT --reject-with tcp-reset
		va_cmd(IPTABLES, 12, 1, "-A", (char *)FW_TELNET_ACCOUNT,
		(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "23", "-j", "REJECT", "--reject-with", "tcp-reset");
#else
        //iptables -A telnet_account -i br0  -p TCP --dport 23 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_DROP);
#endif

#ifdef CONFIG_IPV6
        //iptables -A telnet_account -i nas+  -p TCP --dport 23 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_ACCEPT);

        //iptables -A telnet_account -i ppp+  -p TCP --dport 23 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_ACCEPT);

#ifdef CONFIG_CU
		//ip6tables -A telnet_account -i br0  -p TCP --dport 23 -j REJECT --reject-with tcp-reset
		va_cmd(IP6TABLES, 12, 1, "-A", (char *)FW_TELNET_ACCOUNT,
		(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "23", "-j", "REJECT", "--reject-with", "tcp-reset");
#else
        //iptables -A telnet_account -i br0  -p TCP --dport 23 -j DROP
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_DROP);
#endif
#endif
    }else if(enable == 3){//3:local enbale,remote enbale
        //iptables -A telnet_account -i nas+  -p TCP --dport 23 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_ACCEPT);

        //iptables -A telnet_account -i ppp+  -p TCP --dport 23 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_ACCEPT);

        //iptables -A telnet_account -i br0  -p TCP --dport 23 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_ACCEPT);

#ifdef CONFIG_IPV6
        //iptables -A telnet_account -i nas+  -p TCP --dport 23 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_ACCEPT);

        //iptables -A telnet_account -i ppp+  -p TCP --dport 23 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_ACCEPT);

        //iptables -A telnet_account -i br0  -p TCP --dport 23 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_TELNET_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "23", "-j", (char *)FW_ACCEPT);
#endif
    }else{
		printf("[%s:%s:%d] invlaid telnet enable (%d) status!",__FILE__,__FUNCTION__,__LINE__, enable);
	}
}
#endif

#if defined(INETD_BIND_IP)
int update_inetd_conf(void)
{
	int i = 0, totalEntry = 0, count = 0;
	MIB_CE_ATM_VC_T Entry;
	char ip_str[64] = {0}, netmask_str[64] = {0}, gateway_str[64] = {0};
	unlink(INETD_BIND_CONF);

	FILE *fp;
	fp = fopen(INETD_BIND_CONF,"wb+");
	if (fp)
	{
#if defined(TELNET_BIND_IP)
		getMIB2Str_s(MIB_ADSL_LAN_IP, ip_str, sizeof(ip_str));
		if (strlen(ip_str))
		{
			fprintf(fp, "telnet@%s\n", ip_str);
			count++;
		}

#ifdef CONFIG_IPV6
		mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)ip_str, sizeof(ip_str));
		if (strlen(ip_str))
		{
			fprintf(fp, "telnet@%s\n", ip_str);
			count++;
		}
#endif

		totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < totalEntry; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				continue;

			if (Entry.applicationtype & X_CT_SRV_INTERNET)
			{
				getIPaddrInfo(&Entry, ip_str, netmask_str, gateway_str);
				if (strlen(ip_str))
				{
					fprintf(fp, "telnet@%s\n", ip_str);
					count++;
					break;
				}
			}
		}
#endif
		fclose(fp);
	}

	if (count)
	{
		int pid = read_pid((char *)INETD_PID_FILE);
		if (pid > 0)
		{
			kill(pid, SIGHUP);
		}
	}
	return 0;
}
#endif

#ifdef CONFIG_USER_DM
void restart_dm(void)
{
	int cnt=0;
	int pid;
	unsigned char rtl_dm_enabled;
	mib_get_s(MIB_DM_ENABLE, (void *)&rtl_dm_enabled,sizeof(rtl_dm_enabled));

	printf("restart_dm...\n");
	pid = read_pid((char*)DM_PID);
	if (pid > 0)
		va_cmd("/bin/killall", 2, 1, "-2", "dm");//SIGINT

	while(read_pid((char*)DM_PID)>0)
	{
		/* wait up to 3s */
		if (cnt++ >= 1000)
		{
			unlink(DM_PID);
			break;
		}
		usleep(3000);
	}
	if(rtl_dm_enabled)
		va_cmd("/bin/dm", 1, 0, "0");

}

void stop_dm(void)
{
	printf("stop_dm...\n");
	va_cmd("/bin/killall", 2, 1, "-2", "dm");//SIGINT
}
#endif

#if defined(CONFIG_USER_LANNETINFO) && defined(CONFIG_USER_REFINE_CHECKALIVE_BY_ARP)
int get_lanNetInfo_portSpeed(lan_net_port_speed_T *portSpeed, int num)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret, size;

	if(portSpeed == NULL)
		return -1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_GET_PORT_SPEED;

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	memset(&qbuf, 0, sizeof(struct lanNetInfoMsg));
	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			size = (sizeof(lan_net_port_speed_T)*num >= MAX_DEVICE_INFO_SIZE)? MAX_DEVICE_INFO_SIZE:(sizeof(lan_net_port_speed_T)*num);
			memcpy(portSpeed, mymsg->mtext, size);
			return 0;
		}
	}

	printf("[%s %d]: Get lanNetInfo port speed failed:ret=%d!\n", __func__, __LINE__, ret);
	return -4;
}
#endif
int rtk_is_inforeGroundpgrp(void)
{
	int pgid;
	pgid=tcgetpgrp(1);
	if(pgid<0)
		return pgid;
	return (pgid == getpgid(getpid()));
}
#ifdef CONFIG_USER_P0F
void restart_p0f(void)
{
	if ((findPidByName("p0f")) > 0)
		va_cmd("/bin/killall",1,1,"p0f");

	va_niced_cmd("/bin/p0f", 8, 1, "-i", "br0", "-p", "-f", "/etc/p0f.fp", "-s", "/var/run/p0f.sock", "-d");
	return;
}
#endif
#ifdef CONFIG_USER_LANNETINFO
void restartlanNetInfo(void)
{
	printf("killall lanNetInfo");

	if ((findPidByName("lanNetInfo")) > 0)
		va_cmd("/bin/killall",1,1,"lanNetInfo");

	printf("restart lanNetInfo");
	va_niced_cmd(LANNETINFO, 0, 0);
}
#endif
#if defined(CONFIG_USER_MLDPROXY)
// On NEWADDR, signal to update mldproxy interfaces
void update_mldproxy_vif(MIB_CE_ATM_VC_T *pEntry)
{
	int mypid;
	unsigned int mldItf;

	mypid = read_pid(MLDPROXY_PID);
	if (mypid >= 1) {
#ifdef CONFIG_MLDPROXY_MULTIWAN
		if (pEntry->enable && pEntry->enableMLD) {
			kill(mypid, SIGUSR1);
		}
#else
		if (mib_get(MIB_MLD_PROXY_EXT_ITF, (void *)&mldItf) != 0) {
			if (mldItf == pEntry->ifIndex) {
				kill(mypid, SIGUSR1);
			}
		}
#endif
	}
}
#endif /* CONFIG_USER_MLDPROXY */

#ifdef CONFIG_USER_WAN_PORT_AUTO_SELECTION
void start_wan_port_auto_selection(void)
{
	unsigned char enable=0;
	int pid;
	mib_get_s(MIB_WAN_PORT_AUTO_SELECTION_ENABLE, (void *)&enable, sizeof(enable));

	if(enable==0)
	{
		kill_by_pidfile_new((const char *)WAN_PORT_AUTO_SELECTION_RUNFILE, SIGTERM);
		return ;
	}

	pid=read_pid((const char *)WAN_PORT_AUTO_SELECTION_RUNFILE);

	if(pid<= 0)
		va_cmd("/bin/wan_port_auto_selection", 0, 0);
	else
		kill(pid, SIGUSR1);
}
#endif

int rtk_get_default_route(char *interface, struct in_addr *route)
{
	char buff[1024] = {0}, iface[16] = {0};
	char gate_addr[128] = {0}, net_addr[128] = {0}, mask_addr[128] = {0};
	int num, iflags, metric, refcnt, use, mss, window, irtt;
	FILE *fp;
	char *fmt="%16s %128s %128s %X %d %d %d %128s %d %d %d";
	int ret_val=RTK_FAILED;
	unsigned long addr;

	if ((fp = fopen("/proc/net/route", "r"))==NULL)
	{
		printf("\n%s:%d open file /proc/net/route error!\n",__FUNCTION__,__LINE__);
		return ret_val;
	}

	while (fgets(buff, 1023, fp))
	{
		num = sscanf(buff, fmt, iface, net_addr, gate_addr,
		&iflags, &refcnt, &use, &metric, mask_addr, &mss, &window, &irtt);
		if (num < 10 || !(iflags & RTF_UP) || !(iflags & RTF_GATEWAY) || strcmp(iface, interface))
			continue;

		sscanf(gate_addr, "%lx", &addr );
		*route = *((struct in_addr *)&addr);

		ret_val=RTK_SUCCESS;
		break;
	}

	fclose(fp);
	return ret_val;
}

int rtk_get_mac_by_ipv4_addr(char* str_mac_addr, int str_mac_len, char* ipaddr)
{
	char cmd[128] = {0}, *ptr = NULL;
	FILE *pp = NULL;
	int ret = RTK_FAILED;

	if (!ipaddr || !str_mac_addr || str_mac_len <= 0) {
		return ret;
	}

	snprintf(cmd, sizeof(cmd), "ip neigh | grep %s | awk '{print $5}'", ipaddr);
	pp = popen(cmd, "r");
	if (pp) {
		if (fgets(str_mac_addr, str_mac_len, pp)) {
			ptr = strstr(str_mac_addr, "\n");
			if (ptr) {
				*ptr = '\0';
			}
			ret = RTK_SUCCESS;
		}
		pclose(pp);
	}

	return ret;
}

int rtk_get_intfname_by_mac(char* intf_name, char* macaddr)
{
	char cmd[256] = {0}, *ptr = NULL;
	char intf[IFNAMSIZ] = {0};
	char portno[15] = {0};
	char ifname[IFNAMSIZ] = {0};
	int ret = RTK_FAILED;

	if (!intf_name || !macaddr) {
		return ret;
	}

	sprintf(cmd,"ip neigh | grep %s | awk '{print $3}'", macaddr);
	FILE *pp = popen(cmd, "r");

	if (pp) {
		if (fgets(intf, IFNAMSIZ, pp)) {
			ptr = strstr(intf, "\n");
			if (ptr) {
				*ptr = '\0';
			}

			ret = RTK_SUCCESS;
			if (strstr(intf, "nas") || strstr(intf, "ppp")) {
				//printf("intf=%s\n", intf);
				memcpy(intf_name, intf, ((sizeof(intf)<IFNAMSIZ) ? sizeof(intf) : IFNAMSIZ) );
				pclose(pp);
				return ret;
			}
		}
		pclose(pp);
	}

	if (ret == RTK_FAILED) {
		return ret;
	}

	//get the specific mac bridge port no.
	sprintf(cmd,"brctl showmacs %s | grep %s | awk '{print $1}'", intf, macaddr);
	//printf(cmd);
	pp = popen(cmd, "r");

	if (pp) {
		if (fgets(portno, 15, pp)) {
			ptr = strstr(portno, "\n");
			if (ptr) {
				*ptr = '\0';
			}

			//get real interface name by bridge port no.
			rtk_bridge_get_ifname_by_portno(intf, atoi(portno), ifname);
			//printf("intf=%s,portno=%s,ifname=%s\n", intf,portno,ifname);

			memcpy(intf_name, ifname, ((sizeof(ifname)<IFNAMSIZ) ? sizeof(ifname) : IFNAMSIZ) );
			ret = RTK_SUCCESS;
		}
		pclose(pp);
	}

	return ret;
}

int rtk_get_phyport_by_intfname(char* intf_name)
{
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	return rtk_port_get_phyport_by_ifname(intf_name);
#else
	int i, port;

	if (!intf_name) {
		return RTK_FAILED;
	}

	if (!strncmp(intf_name, "nas", 3) || !strncmp(intf_name, "ppp", 3)) {
		return rtk_port_get_wan_phyID();
	}

	for (i = 0; i < ELANVIF_NUM; i++) {
		if (!strcmp(intf_name, ELANVIF[i])) {
			return rtk_port_get_lan_phyID(i);
		}
	}

	return RTK_FAILED;
#endif
}

int rtk_send_ping(char *ipaddr)
{
	char buf[128] = {0};
	if (!ipaddr) {
		return RTK_FAILED;
	}

	printf("[%s:%d] \n", __FUNCTION__, __LINE__);
	snprintf(buf, sizeof(buf), "/bin/ping -w 2 %s > /dev/null\n", ipaddr);
	va_cmd("/bin/sh", 2, 1,"-c", buf);

	return RTK_SUCCESS;
}

#ifdef CONFIG_USER_AP_BRIDGE_MODE_QOS
int rtk_get_bridge_mode_uplink_port(void)
{
	struct in_addr route;
	char intf_name[IFNAMSIZ] = {0}, str_mac_addr[MAC_ADDR_STRING_LEN] = {0}, *str_ip_addr;
	int uplink_port;
	unsigned char is_repeater_mode = 0;

	// skip checking in repeater mode
#if defined(WLAN_UNIVERSAL_REPEATER) && defined(CONFIG_USER_RTK_REPEATER_MODE)
	if(mib_get(MIB_REPEATER_MODE, (void *)&is_repeater_mode) && is_repeater_mode){
		printf("%s:%d Skip because the dut is in repeater mode!\n",__FUNCTION__,__LINE__);
		return RTK_FAILED;
	}
#endif
	if (getOpMode() == BRIDGE_MODE) {
		// by default route, we get uplink port in bridge mode
		if (rtk_get_default_route((char *)BRIF, &route) == RTK_SUCCESS) {
			str_ip_addr = inet_ntoa(route);
			printf("[%s:%d] gw:%s\n", __FUNCTION__, __LINE__, str_ip_addr);

			// if get neighbour fail, ping and get it again.
			if (rtk_get_mac_by_ipv4_addr(str_mac_addr, MAC_ADDR_STRING_LEN, str_ip_addr) != RTK_SUCCESS) {
				rtk_send_ping(str_ip_addr);
				if (!rtk_get_mac_by_ipv4_addr(str_mac_addr, MAC_ADDR_STRING_LEN, str_ip_addr)) {
					return RTK_FAILED;
				}
			}

			if (rtk_get_intfname_by_mac(intf_name, str_mac_addr) == RTK_SUCCESS) {
				uplink_port = rtk_get_phyport_by_intfname(intf_name);
				printf("[%s:%d] uplink_port: %d\n", __FUNCTION__, __LINE__, uplink_port);
				return uplink_port;
			}
		}
	}

	return RTK_FAILED;
}
#endif

int ezmesh_send_arp(u_int32_t yiaddr, u_int32_t ip, unsigned char *mac, char *interface)
{
	//int   timeout = 1;
	int 	optval = 1;
	int	s;			/* socket */
	int	rv = 1;			/* return value */
	struct sockaddr addr;		/* for interface name */
	struct ezmesh_arpMsg	arp;
	fd_set		fdset;
	//struct timeval	tm;
	//time_t		prevTime;
	u_int32_t u32_srcIP;

	if ((s = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1) {
		printf("Could not open raw socket\n");
		return -1;
	}

	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
		printf("Could not setsocketopt on raw socket\n");
		close(s);
		return -1;
	}

	/* send arp request */
	memset(&arp, 0, sizeof(arp));
	memcpy(arp.ethhdr.h_dest, MAC_BCAST_ADDR, 6);	/* MAC DA */
	memcpy(arp.ethhdr.h_source, mac, 6);		/* MAC SA */
	arp.ethhdr.h_proto = htons(ETH_P_ARP);		/* protocol type (Ethernet) */
	arp.htype = htons(ARPHRD_ETHER);		/* hardware type */
	arp.ptype = htons(ETH_P_IP);			/* protocol type (ARP message) */
	arp.hlen = 6;					/* hardware address length */
	arp.plen = 4;					/* protocol address length */
	arp.operation = htons(ARPOP_REQUEST);		/* ARP op code */
	memcpy(arp.sInaddr, &ip, sizeof(ip));		/* source IP address */
	memcpy(arp.sHaddr, mac, 6);			/* source hardware address */
	memcpy(arp.tInaddr, &yiaddr, sizeof(yiaddr));	/* target IP address */

	memset(&addr, 0, sizeof(addr));
	strncpy(addr.sa_data, interface, sizeof(addr.sa_data)-1);
	//printf("EZMESH[%s %d],send arp on if:%s\n",__FUNCTION__,__LINE__,interface);
	if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0){
		printf("EZMESH[%s %d],send arp on %s fail, errno:%d \n",__FUNCTION__,__LINE__,interface,errno);
		rv = 0;
	}

	close(s);
	return rv;
}

int rtk_get_default_gw_wan_ifname(char *wan_ifname)
{
	int total, i;
	int ret_val=-1;
	unsigned int ifindex=0;
	char tmp_ifname[IFNAMSIZ]={0};
	MIB_CE_ATM_VC_T Entry;
	struct in_addr inAddr;
	int flags = 0;

	total = mib_chain_total(MIB_ATM_VC_TBL);
	if (total > MAX_VC_NUM)
		return ret_val;

	for(i = 0; i < total; i++)
	{
		mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&Entry);
		if (!Entry.enable || !isValidMedia(Entry.ifIndex))
			continue;

		if(Entry.dgw)
		{
			if (ifGetName(Entry.ifIndex, wan_ifname, IFNAMSIZ) == 0)
				continue;

			if (getInFlags(wan_ifname, &flags) == 1 && flags & IFF_UP)
			{
#ifdef CONFIG_IPV6
				if (Entry.IpProtocol & IPVER_IPV4)
				{
					if (getInAddr(wan_ifname, IP_ADDR, (void *)&inAddr) == 1)
						return 0;
				}

				if (Entry.IpProtocol & IPVER_IPV4)
				{
					struct ipv6_ifaddr addr6;
					if ((Entry.AddrMode & IPV6_WAN_STATIC) || (getifip6(wan_ifname, IPV6_ADDR_UNICAST, &addr6, 1) == 1))
						return 0;
				}
#else
				if (getInAddr(wan_ifname, IP_ADDR, (void *)&inAddr) == 1)
					return 0;
#endif
			}

		}
	}

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	int l2tpEnable;
	unsigned int l2tpEntryNum;
	MIB_L2TP_T l2tpEntry;

	mib_get_s(MIB_L2TP_ENABLE, (void *)&l2tpEnable, sizeof(l2tpEnable));
	if(l2tpEnable)
	{
		l2tpEntryNum = mib_chain_total(MIB_L2TP_TBL);
		for (i=0; i<l2tpEntryNum; i++)
		{
			if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tpEntry) )
				continue;

			if (l2tpEntry.dgw)
			{
				ifGetName(l2tpEntry.ifIndex, tmp_ifname, sizeof(tmp_ifname));
				snprintf(wan_ifname, IFNAMSIZ, "%s", tmp_ifname);
				ret_val=0;
				return ret_val;
			}
		}
	}
#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	int pptpEnable;
	unsigned int pptpEntryNum;
	MIB_PPTP_T ptpEntry;

	mib_get_s(MIB_PPTP_ENABLE, (void *)&pptpEnable, sizeof(pptpEnable));
	if(pptpEnable)
	{
		pptpEntryNum = mib_chain_total(MIB_PPTP_TBL);
		for (i=0; i<pptpEntryNum; i++)
		{
			if (!mib_chain_get(MIB_PPTP_TBL, i, (void *)&ptpEntry))
				continue;

			if (ptpEntry.dgw)
			{
				ifGetName(ptpEntry.ifIndex, tmp_ifname, sizeof(tmp_ifname));
				snprintf(wan_ifname, IFNAMSIZ, "%s", tmp_ifname);
				ret_val=0;
				return ret_val;
			}
		}
	}
#endif

	return ret_val;
}

#ifdef CONFIG_RTK_GENERIC_NETLINK_API
/**
 * rtk_nla_attr_size - length of attribute size, NOT including padding
 * @param payload   length of payload
 * @return
 */
int rtk_nla_attr_size(int payload)
{
	return NLA_HDRLEN + payload;
}

/**
 * rtk_nla_total_size - total length of attribute including padding
 * @param payload   length of payload, NOT including NLA_HDR
 */
int rtk_nla_total_size(int payload)
{
	return NLA_ALIGN(rtk_nla_attr_size(payload));
}

int rtk_genlmsg_open(int *sockfd)
{
    struct sockaddr_nl nladdr;

	if(!sockfd){
        printf("NULL sockfd\n");
        return -1;
	}

    *sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    if (*sockfd < 0)
    {
        printf("socket: %m\n");
        return -1;
    }

    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    nladdr.nl_pid = getpid();

    if (bind(*sockfd, (struct sockaddr *)&nladdr, sizeof(nladdr)) < 0)
    {
        printf("bind: %s(%d)\n", strerror(errno), errno);
		close(*sockfd);
		return -1;
    }

    return *sockfd;
}

void rtk_genlmsg_close(int sockfd)
{
    if (sockfd >= 0)
        close(sockfd);
}

/**
 * rtk_genlmsg_alloc - allocate generic netlink socket buffer
 * @family_hdr: genl family header length
 * @payload_size: genl payloal length
 *
 * Returns:
 *	NULL if fail
 *	socket buffer pointer if success
 */
void *rtk_genlmsg_alloc(int family_hdr, int payload_size)
{
    char *buf;
    int len;

	len = GENLMSG_TOTAL_LEN(family_hdr, payload_size);
    buf = (char *)malloc(len);
    if (!buf)
        return NULL;

    memset(buf, 0, len);
    return buf;
}

void rtk_genlmsg_free(void *buf)
{
	if (buf)
		free(buf);
}

int rtk_genlmsg_send(int sockfd, unsigned short nlmsg_type, unsigned int nlmsg_pid,
        unsigned char genl_cmd, unsigned char genl_version, unsigned int family_hdr,
        unsigned short nla_type, const void *nla_data, unsigned int nla_len)
{
	struct nlmsghdr *nlh = NULL;    //netlink message header
	struct genlmsghdr *glh = NULL;  //generic netlink message header
	void *userhdr = NULL;			//user specific header
	struct nlattr *nla = NULL;      //netlink attribute header
	struct sockaddr_nl nladdr;
	unsigned char *buf = NULL;
	int len = 0, count = 0, ret = 0;

	if ((nlmsg_type == 0) || (!nla_data) || (nla_len <= 0)){
		return -1;
	}

	buf = rtk_genlmsg_alloc(family_hdr, nla_len);
	if (!buf)
		return -1;

	len = GENLMSG_TOTAL_LEN(family_hdr, nla_len);	// get total nlmsg length

	nlh = (struct nlmsghdr *)buf;
	nlh->nlmsg_len = len;
	nlh->nlmsg_type = nlmsg_type;
	nlh->nlmsg_flags = NLM_F_REQUEST;
	nlh->nlmsg_seq = 0;
	nlh->nlmsg_pid = nlmsg_pid;

	glh = (struct genlmsghdr *)NLMSG_DATA(nlh);
	glh->cmd = genl_cmd;
	glh->version = genl_version;

	userhdr = (void *)GENLMSG_DATA(glh);
	if(family_hdr){
		fprintf(stderr, "[%s:%d] TODO: create user header here.\n", __FUNCTION__, __LINE__);
	}

	nla = (struct nlattr *)((char*)userhdr + family_hdr);	//the first attribute
	nla->nla_type = nla_type;
	nla->nla_len = rtk_nla_attr_size(nla_len);
	memcpy(NLA_DATA(nla), nla_data, nla_len);

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;

	count = 0;
	ret = 0;
	do {
		ret = sendto(sockfd, &buf[count], len - count, 0,
						(struct sockaddr *)&nladdr, sizeof(nladdr));
		if (ret < 0){
			if (errno != EAGAIN) {
				count = -1;
				goto out;
			}
		} else {
			count += ret;
		}
	}while (count < len);

out:
	rtk_genlmsg_free(buf);
	printf("send return %d\n", count);
	return count;
}

/**
 *
 * @param sockfd    generic netlink socket fd
 * @param buf       the 'buf' is including the struct nlmsghdr,
 *                  struct genlmsghdr and struct nlattr
 * @param len       size of 'buf'
 * @return  >0      size of genlmsg
 *          <0      error occur
 */
int rtk_genlmsg_recv(int sockfd, unsigned char *buf, unsigned int len)
{
	struct sockaddr_nl nladdr;
	struct msghdr msg;
	struct iovec iov;
	int ret;

	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = getpid();
	//    nladdr.nl_groups = 0xffffffff;

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_name = (void *)&nladdr;
	msg.msg_namelen = sizeof(nladdr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;
	ret = recvmsg(sockfd, &msg, 0);
	ret = ret > 0 ? ret : -1;
	printf("recv return %d\n", ret);
	return ret;
}

int rtk_genlmsg_dispatch(struct nlmsghdr *nlmsghdr, unsigned int nlh_len, int nlmsg_type,
						unsigned int family_hdr, int nla_type, unsigned char *buf, int *len)
{
	struct nlmsghdr *nlh=NULL;
	struct genlmsghdr *glh=NULL;
	void *userhdr = NULL;			//user specific header
	struct nlattr *nla=NULL;
	int nla_len=0, l=0, i=0, ret=-1;

	if (!nlmsghdr || !buf || !len)
		return ret;

	memset(buf, 0, *len);

	if (nlmsg_type && (nlmsghdr->nlmsg_type != nlmsg_type)){
		printf("[%s] nlmsg_type=%d, nlmsghdr->nlmsg_type=%d\n", __FUNCTION__, nlmsg_type, nlmsghdr->nlmsg_type);
		return ret;
	}

	for (nlh = nlmsghdr; NLMSG_OK(nlh, nlh_len); nlh = NLMSG_NEXT(nlh, nlh_len))
	{
		/* The end of multipart message. */
		if (nlh->nlmsg_type == NLMSG_DONE){
			printf("get NLMSG_DONE\n");
			ret = 0;
			break;
		}

		if (nlh->nlmsg_type == NLMSG_ERROR){
			printf("get NLMSG_ERROR\n");
			ret = -1;
			break;
		}

		glh = (struct genlmsghdr *)NLMSG_DATA(nlh);
		userhdr = (void *)GENLMSG_DATA(glh);
		if(family_hdr){
			fprintf(stderr, "[%s:%d] TODO: parser user header here.\n", __FUNCTION__, __LINE__);
		}

		nla = (struct nlattr *)((char*)userhdr + family_hdr);	//the first attribute
		nla_len = nlh->nlmsg_len - GENL_HDRLEN;					//len of attributes
		for (i = 0; NLA_OK(nla, nla_len); nla = NLA_NEXT(nla, nla_len), ++i)
		{
			/* Match the family ID, copy the data to user */
			if (nla_type == nla->nla_type)
			{
				l = nla->nla_len - NLA_HDRLEN;
				*len = (*len > l)? l : *len;
				memcpy(buf, NLA_DATA(nla), *len);
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

int rtk_genlmsg_get_family_id(int sockfd, const char *family_name)
{
	void *buf=NULL;
	int len;
	unsigned short id;
	int l;
	int ret;

	ret = rtk_genlmsg_send(sockfd, GENL_ID_CTRL, 0, CTRL_CMD_GETFAMILY, 1, 0,
			CTRL_ATTR_FAMILY_NAME, family_name, strlen(family_name) + 1);
	if (ret < 0)
		return -1;

	len = 256;
	buf = rtk_genlmsg_alloc(0, 256);
	if (!buf)
		return -1;

	len = GENLMSG_TOTAL_LEN(0, 256);	// get total nlmsg length
	len = rtk_genlmsg_recv(sockfd, buf, len);
	if (len < 0)
		return len;

	id = 0;
	l = sizeof(id);
	rtk_genlmsg_dispatch((struct nlmsghdr *)buf, len, 0, 0, CTRL_ATTR_FAMILY_ID, (unsigned char *)&id, &l);

	rtk_genlmsg_free(buf);

	return id > 0 ? id : -1;
}

int rtk_genlmsg_get_group_id(int sockfd, const char *family_name, const char *grp_name)
{
	void *buf;
	int len;
	unsigned char data[1024];
	unsigned int data_len=0, sub_nla_len=0;
	int nla_len=0, i=0, j=0;
	int ret;
	struct nlattr *nla=NULL, *sub_nla=NULL;
	int grp_id=0, found=0;

	ret = rtk_genlmsg_send(sockfd, GENL_ID_CTRL, 0, CTRL_CMD_GETFAMILY, 1, 0,
	CTRL_ATTR_FAMILY_NAME, family_name, strlen(family_name) + 1);
	if (ret < 0)
		return -1;

	len = 256;
	buf = rtk_genlmsg_alloc(0, 256);
	if (!buf)
		return -1;

	len = rtk_genlmsg_recv(sockfd, buf, len);
	if (len < 0)
		return len;

	memset(data, 0, sizeof(data));
	data_len = sizeof(data);
	rtk_genlmsg_dispatch((struct nlmsghdr *)buf, len, 0, 0, CTRL_ATTR_MCAST_GROUPS, (unsigned char *)data, &data_len);

	nla = (struct nlattr *)data;
	/* genl group is follow nlattr format */
	for (i = 0; !found && NLA_OK(nla, data_len); nla = NLA_NEXT(nla, data_len), ++i){
		sub_nla = NLA_DATA(nla);
		sub_nla_len = nla->nla_len;
		for (j = 0; NLA_OK(sub_nla, sub_nla_len); sub_nla = NLA_NEXT(sub_nla, sub_nla_len), ++j){
			if(sub_nla->nla_type == CTRL_ATTR_MCAST_GRP_ID){
				grp_id = *(int *)NLA_DATA(sub_nla);
			}
			if(sub_nla->nla_type == CTRL_ATTR_MCAST_GRP_NAME){
				if(0 == strcmp(grp_name, (char *)NLA_DATA(sub_nla)))
					found=1;
			}
		}
	}

	if(found){
		ret = grp_id;
	}else{
		ret = -1;
	}

	rtk_genlmsg_free(buf);
	return ret;
}
#endif	/* CONFIG_RTK_GENERIC_NETLINK_API */

#if defined(CONFIG_YUEME) || defined(CONFIG_CU)
//cxy 2018-4-2: patch for webinspect scan: Parameter Based Redirection to unknow url
int checkValidRedirect(char*redirecturl, char** checklist)
{
	if(redirecturl == NULL || checklist == NULL)
		return 1;
	while(*checklist != NULL)
	{
#ifdef CONFIG_CU
		if(!strcmp(redirecturl, *checklist))//valid redirect
#else
		if(strstr(redirecturl, *checklist))//valid redirect
#endif
			return 1;
		checklist++;
	}
	return 0;

}
#endif

#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
int rtk_get_wan_itf_by_mark(char *wan_name, unsigned int mark)
{
	int entry_num = 0, i = 0;
	MIB_CE_ATM_VC_T entry;
	unsigned int wan_mark = 0, wan_mask = 0;
	char wanif[IFNAMSIZ] = {0};

	if(wan_name ==NULL)
		return -1;

	entry_num = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<entry_num;i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
				continue;

		ifGetName(entry.ifIndex, wanif, sizeof(wanif));
		rtk_net_get_wan_policy_route_mark(wanif, &wan_mark, &wan_mask);
		if(wan_mark == mark)
		{
			snprintf(wan_name, IFNAMSIZ,"%s", wanif);
			return 1;
		}
	}

	return 0;
}
#if defined(CONFIG_USER_LANNETINFO)
int rtk_calc_manual_load_balance_num(char *wan_itf_name)
{
	int calc_cnt = 0 , i =0, j=0;
	MIB_CE_ATM_VC_T wan_entry;
	lanHostInfo_t *pLanNetInfo = NULL;
	unsigned int count = 0;
	unsigned int sta_load_entry_num;
	MIB_STATIC_LOAD_BALANCE_T sta_entry;
	unsigned char lan_mac[18] = {0};
	char sta_enable = 0;

	if(wan_itf_name == NULL)
	{
		goto get_info_end;
	}

	if(!mib_get_s(MIB_STATIC_LOAD_BALANCE_ENABLE, (void *)&sta_enable,sizeof(sta_enable)))
	{
		goto get_info_end;
	}

	if(sta_enable==0)
	{
		//calc_cnt is zero when disable manual load balance.
		calc_cnt = 0;
		goto get_info_end;
	}

	if(get_lan_net_info(&pLanNetInfo, &count) != 0)
	{
		goto get_info_end;
	}

	if(count>0)
	{
		sta_load_entry_num = mib_chain_total(MIB_STATIC_LOAD_BALANCE_TBL);
		if(sta_load_entry_num > 0){
			for(i= 0; i<count; i++)
			{
				snprintf(lan_mac,sizeof(lan_mac),"%02x:%02x:%02x:%02x:%02x:%02x", pLanNetInfo[i].mac[0],pLanNetInfo[i].mac[1],pLanNetInfo[i].mac[2],pLanNetInfo[i].mac[3],pLanNetInfo[i].mac[4],pLanNetInfo[i].mac[5]);

				for(j=0; j<sta_load_entry_num; j++)
				{
					if(!mib_chain_get(MIB_STATIC_LOAD_BALANCE_TBL, j, &sta_entry))
						continue;

					if((!strncasecmp(lan_mac, sta_entry.mac, MAC_ADDR_STRING_LEN-1)) && (!strncmp(sta_entry.wan_itfname,wan_itf_name,IFNAMSIZ)))
					{
						calc_cnt++;
					}
				}
			}
		}
	}

get_info_end:
	if(pLanNetInfo)
		free(pLanNetInfo);

	return calc_cnt;
}
#endif
#endif

#ifdef CONFIG_USB_SUPPORT
// restart USB to trigger hotplug add uevent
void resetUsbDevice(void){
	DIR *dir=NULL;
	struct dirent *next=NULL;
	char buf[128]={0};

	dir = opendir("/sys/bus/usb/devices");
	if (!dir) {
		printf("Cannot open /sys/bus/usb/devices");
		return;
	}

	while ((next = readdir(dir)) != NULL) {
		/* Must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, "..") == 0)
			continue;

		/* only interesting "usb" prefix direction */
		if (strncasecmp(next->d_name, "usb", 3) != 0)
			continue;

		fprintf(stderr, "[%s]reset /sys/bus/usb/devices/%s/authorized\n", __FUNCTION__, next->d_name);
		// re-authorized usb device
		snprintf(buf, sizeof(buf), "echo 0 > /sys/bus/usb/devices/%s/authorized", next->d_name);
		system(buf);

		snprintf(buf, sizeof(buf), "echo 1 > /sys/bus/usb/devices/%s/authorized", next->d_name);
		system(buf);
	}
	closedir(dir);

	return;
}
#endif //CONFIG_USB_SUPPORT

#if defined(CONFIG_PPP) && defined(CONFIG_USER_PPPD)
void WRITE_PPP_FILE(int fh, unsigned char *buf)
{
	if (!buf) return;
	if ( write(fh, buf, strlen(buf)) != strlen(buf) ) {
		printf("Write PPP script file error!\n");
	}
}

void rtk_create_ppp_secrets(void)
{
	int fdchap = -1, fdpap = -1;
	char buff[128];
	int total, i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ]={0};
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	MIB_L2TP_T l2tp_entry;
	int l2tp_enable = 0;
	int l2tp_mode = L2TP_MODE_LAC;
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	MIB_PPTP_T pptp_entry;
	int pptp_enable;
	int pptp_mode = PPTP_MODE_CLIENT;
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD) || defined(CONFIG_USER_PPTP_CLIENT)
#if defined(CONFIG_USER_L2TPD_LNS) || defined(CONFIG_USER_PPTP_SERVER)
	MIB_VPND_T server;
	MIB_VPN_ACCOUNT_T lns_entry;
	int acc_total=0, j=0;
#endif
#endif

	/**********************************************************************/
	/* Write CHAP/PAP secrets file												   */
	/**********************************************************************/
	fdchap = open("/etc/ppp/chap-secrets", O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
	if (fdchap == -1) {
		printf("Create /etc/ppp/chap-secrets file error!\n");
		return;
	}

	fdpap = open("/etc/ppp/pap-secrets", O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
	if (fdpap == -1) {
		close(fdchap);
		printf("Create /etc/ppp/pap-secrets file error!\n");
		return;
	}

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		/* get the specified chain record */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if (Entry.cmode != CHANNEL_MODE_PPPOE && Entry.cmode != CHANNEL_MODE_PPPOA)
			continue;

		snprintf(buff, sizeof(buff), "%s * %s *\n", Entry.pppUsername, Entry.pppPassword);
		if (Entry.pppAuth != PPP_AUTH_PAP && Entry.pppAuth != PPP_AUTH_NONE)
			WRITE_PPP_FILE(fdchap, buff);
		if (Entry.pppAuth == PPP_AUTH_AUTO || Entry.pppAuth == PPP_AUTH_PAP)
			WRITE_PPP_FILE(fdpap, buff);
	}

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if ( !mib_get_s(MIB_L2TP_ENABLE, (void *)&l2tp_enable, sizeof(l2tp_enable)) )
		goto finish_l2tp;

	if(!l2tp_enable)
		goto finish_l2tp;

	if ( !mib_get_s(MIB_L2TP_MODE, (void *)&l2tp_mode, sizeof(l2tp_mode)) )
		goto finish_l2tp;

	switch(l2tp_mode){
	case L2TP_MODE_LAC:
		{
			total = mib_chain_total(MIB_L2TP_TBL);
			for (i=0; i<total; i++)
			{
				if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tp_entry) )
					continue;

				if(l2tp_entry.vpn_enable != VPN_ENABLE)
					continue;

				ifGetName(l2tp_entry.ifIndex, ifname, IFNAMSIZ);
				snprintf(buff, sizeof(buff), "%s %s %s *\n", l2tp_entry.username, ifname, l2tp_entry.password);
				if (l2tp_entry.authtype == AUTH_AUTO || l2tp_entry.authtype == AUTH_CHAP || l2tp_entry.authtype == AUTH_CHAPMSV2)
					WRITE_PPP_FILE(fdchap, buff);
				if (l2tp_entry.authtype == AUTH_AUTO || l2tp_entry.authtype == AUTH_PAP)
					WRITE_PPP_FILE(fdpap, buff);
			}
		}
		break;
	case L2TP_MODE_LNS:
		{
#if defined(CONFIG_USER_L2TPD_LNS)
			total = mib_chain_total(MIB_VPN_SERVER_TBL);

			for (i=0 ; i<total ; i++) {
				int fdoptions;
				char xl2tpdopt_str[32]={0};
				if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, (void *)&server))
					continue;

				if(server.type != VPN_L2TP)
					continue;

				acc_total = mib_chain_total(MIB_VPN_ACCOUNT_TBL);
				for(j=0; j < acc_total; j++){
					if (!mib_chain_get(MIB_VPN_ACCOUNT_TBL, j, (void *)&lns_entry))
						continue;

					if(!lns_entry.enable || (lns_entry.type != VPN_L2TP) || strncmp(lns_entry.name, server.name, strlen(server.name)+1))
						continue;

					snprintf(buff, sizeof(buff), "%s %s %s *\n", lns_entry.username, lns_entry.name, lns_entry.password);
					if (server.authtype == AUTH_AUTO || server.authtype == AUTH_CHAP || server.authtype == AUTH_CHAPMSV2)
						WRITE_PPP_FILE(fdchap, buff);
					if (server.authtype == AUTH_AUTO || server.authtype == AUTH_PAP)
						WRITE_PPP_FILE(fdpap, buff);
				}
			}
#else
			fprintf(stderr, "[%s] NOT support L2TP_MODE_LNS\n", __FUNCTION__);
#endif
		}
		break;
	default:
		fprintf(stderr, "[%s] unknown L2TP Mode(%d)\n", __FUNCTION__, l2tp_mode);
		goto finish_l2tp;
	}
finish_l2tp:	// add compound statement "{}" to avoid compile fail(EX: int x;).
{
}
#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if ( !mib_get(MIB_PPTP_ENABLE, (void *)&pptp_enable) )
		goto finish_pptp;

	if(!pptp_enable)
		goto finish_pptp;

#ifdef CONFIG_USER_PPTP_SERVER
	if ( !mib_get_s(MIB_PPTP_MODE, (void *)&pptp_mode, sizeof(pptp_mode)) )
		goto finish_pptp;
#endif

	switch(pptp_mode) {
	case PPTP_MODE_CLIENT:
		{
	total = mib_chain_total(MIB_PPTP_TBL);
	for (i=0 ; i<total ; i++) {
		if (!mib_chain_get(MIB_PPTP_TBL, i, (void *)&pptp_entry))
			continue;

		ifGetName(pptp_entry.ifIndex, ifname, IFNAMSIZ);
		snprintf(buff, sizeof(buff), "%s %s %s *\n", pptp_entry.username, ifname, pptp_entry.password);
		if (pptp_entry.authtype == AUTH_AUTO || pptp_entry.authtype == AUTH_CHAP || pptp_entry.authtype == AUTH_CHAPMSV2)
			WRITE_PPP_FILE(fdchap, buff);
		if (pptp_entry.authtype == AUTH_AUTO || pptp_entry.authtype == AUTH_PAP)
			WRITE_PPP_FILE(fdpap, buff);
	}
		}
		break;
	case PPTP_MODE_SERVER:
		{
#if defined(CONFIG_USER_PPTP_SERVER)
			total = mib_chain_total(MIB_VPN_SERVER_TBL);

			for (i=0 ; i<total ; i++) {
				int fdoptions;
				if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, (void *)&server))
					continue;

				if(server.type != VPN_PPTP)
					continue;

				acc_total = mib_chain_total(MIB_VPN_ACCOUNT_TBL);
				for(j=0; j < acc_total; j++){
					if (!mib_chain_get(MIB_VPN_ACCOUNT_TBL, j, (void *)&lns_entry))
						continue;

					if(!lns_entry.enable || (lns_entry.type != VPN_PPTP) || strncmp(lns_entry.name, server.name, strlen(server.name)+1))
						continue;

					snprintf(buff, sizeof(buff), "%s %s %s *\n", lns_entry.username, lns_entry.name, lns_entry.password);
					if (server.authtype == AUTH_AUTO || server.authtype == AUTH_CHAP || server.authtype == AUTH_CHAPMSV2)
						WRITE_PPP_FILE(fdchap, buff);
					if (server.authtype == AUTH_AUTO || server.authtype == AUTH_PAP)
						WRITE_PPP_FILE(fdpap, buff);
				}
			}
#else
			fprintf(stderr, "[%s] NOT support PPTP_MODE_SERVER\n", __FUNCTION__);
#endif
		}
		break;
	default:
		fprintf(stderr, "[%s] unknown PPTP Mode(%d)\n", __FUNCTION__, pptp_mode);
		goto finish_pptp;
	}

finish_pptp:	// add compound statement "{}" to avoid compile fail(EX: int x;).
{
}
#endif

	close(fdchap);
	close(fdpap);
}
#endif

void escape(char* s,char* t)
{
	int i,j;
	i=j=0;
	while(t[i] != '\0')
	{
		switch(t[i])
		{
			case '\"':
				s[j]='\\';
				++j;
				s[j]='\"';
			break;
			default:
				s[j]=t[i];
			break;
		}
		++i;
		++j;
	}
	s[j]='\0';
}

#if defined(CONFIG_USER_USP_TR369)
int rtk_tr369_mtp_get_entry_by_instnum(MIB_TR369_LOCALAGENT_MTP_T *pentry, unsigned int instnum, unsigned int *chainid)
{
	int ret = -1;
	unsigned int i = 0, total = 0;

	if (!pentry || !chainid) return ret;

	total = mib_chain_total(MIB_TR369_LOCALAGENT_MTP_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_TR369_LOCALAGENT_MTP_TBL, i, (void *)pentry))
			continue;

		if (pentry->instnum == instnum)
		{
			*chainid = i;
			ret = 0;
			break;
		}
	}

	return ret;
}

unsigned int rtk_tr369_mtp_get_new_instnum(void)
{
	unsigned int i = 0, total = 0, instnum = 0;
	MIB_TR369_LOCALAGENT_MTP_T *p, entry;
	p = &entry;

	total = mib_chain_total(MIB_TR369_LOCALAGENT_MTP_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_TR369_LOCALAGENT_MTP_TBL, i, (void *)p))
			continue;

		if (p->instnum > instnum)
		{
			instnum = p->instnum;
		}
	}

	return (instnum + 1);
}

int rtk_tr369_controller_get_entry_by_instnum(MIB_TR369_LOCALAGENT_CONTROLLER_T *pentry, unsigned int instnum, unsigned int *chainid)
{
	int ret = -1;
	unsigned int i = 0, total = 0;

	if (!pentry || !chainid) return ret;

	total = mib_chain_total(MIB_TR369_LOCALAGENT_CONTROLLER_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_TR369_LOCALAGENT_CONTROLLER_TBL, i, (void *)pentry))
			continue;

		if (pentry->instnum == instnum)
		{
			*chainid = i;
			ret = 0;
			break;
		}
	}

	return ret;
}

unsigned int rtk_tr369_controller_get_new_instnum(void)
{
	unsigned int i = 0, total = 0, instnum = 0;
	MIB_TR369_LOCALAGENT_CONTROLLER_T *p, entry;
	p = &entry;

	total = mib_chain_total(MIB_TR369_LOCALAGENT_CONTROLLER_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_TR369_LOCALAGENT_CONTROLLER_TBL, i, (void *)p))
			continue;

		if (p->instnum > instnum)
		{
			instnum = p->instnum;
		}
	}

	return (instnum + 1);
}

int rtk_tr369_controller_mtp_get_entry_by_instnum(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_T *pentry, unsigned int instnum1, unsigned int instnum2, unsigned int *chainid)
{
	int ret = -1;
	unsigned int i = 0, total = 0;

	if (!pentry || !chainid) return ret;

	total = mib_chain_total(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL, i, (void *)pentry))
			continue;

		if (pentry->controller_instnum == instnum1 && pentry->instnum == instnum2)
		{
			*chainid = i;
			ret = 0;
			break;
		}
	}

	return ret;
}

unsigned int rtk_tr369_controller_mtp_get_new_instnum(unsigned int controller_instnum)
{
	unsigned int i = 0, total = 0, instnum = 0;
	MIB_TR369_LOCALAGENT_CONTROLLER_MTP_T *p, entry;
	p = &entry;

	total = mib_chain_total(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL, i, (void *)p))
			continue;

		if (p->controller_instnum == controller_instnum && p->instnum > instnum)
		{
			instnum = p->instnum;
		}
	}

	return (instnum + 1);
}

int rtk_tr369_stomp_get_entry_by_instnum(MIB_TR369_STOMP_CONNECTION_T *pentry, unsigned int instnum, unsigned int *chainid)
{
	int ret = -1;
	unsigned int i = 0, total = 0;

	if (!pentry || !chainid) return ret;

	total = mib_chain_total(MIB_TR369_STOMP_CONNECTION_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_TR369_STOMP_CONNECTION_TBL, i, (void *)pentry))
			continue;

		if (pentry->instnum == instnum)
		{
			*chainid = i;
			ret = 0;
			break;
		}
	}

	return ret;
}

unsigned int rtk_tr369_stomp_get_new_instnum(void)
{
	unsigned int i = 0, total = 0, instnum = 0;
	MIB_TR369_STOMP_CONNECTION_T *p, entry;
	p = &entry;

	total = mib_chain_total(MIB_TR369_STOMP_CONNECTION_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_TR369_STOMP_CONNECTION_TBL, i, (void *)p))
			continue;

		if (p->instnum > instnum)
		{
			instnum = p->instnum;
		}
	}

	return (instnum + 1);
}

int rtk_tr369_mqtt_client_get_entry_by_instnum(MIB_TR369_MQTT_CLIENT_T *pentry, unsigned int instnum, unsigned int *chainid)
{
	int ret = -1;
	unsigned int i = 0, total = 0;

	if (!pentry || !chainid) return ret;

	total = mib_chain_total(MIB_TR369_MQTT_CLIENT_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_TR369_MQTT_CLIENT_TBL, i, (void *)pentry))
			continue;

		if (pentry->instnum == instnum)
		{
			*chainid = i;
			ret = 0;
			break;
		}
	}

	return ret;
}

unsigned int rtk_tr369_mqtt_client_get_new_instnum(void)
{
	unsigned int i = 0, total = 0, instnum = 0;
	MIB_TR369_MQTT_CLIENT_T *p, entry;
	p = &entry;

	total = mib_chain_total(MIB_TR369_MQTT_CLIENT_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_TR369_MQTT_CLIENT_TBL, i, (void *)p))
			continue;

		if (p->instnum > instnum)
		{
			instnum = p->instnum;
		}
	}

	return (instnum + 1);
}

int rtk_tr369_mqtt_client_subscription_get_entry_by_instnum(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_T *pentry, unsigned int instnum1, unsigned int instnum2, unsigned int *chainid)
{
	int ret = -1;
	unsigned int i = 0, total = 0;

	if (!pentry || !chainid) return ret;

	total = mib_chain_total(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL, i, (void *)pentry))
			continue;

		if (pentry->client_instnum == instnum1 && pentry->instnum == instnum2)
		{
			*chainid = i;
			ret = 0;
			break;
		}
	}

	return ret;
}

unsigned int rtk_tr369_mqtt_client_subscription_get_new_instnum(unsigned int client_instnum)
{
	unsigned int i = 0, total = 0, instnum = 0;
	MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_T *p, entry;
	p = &entry;

	total = mib_chain_total(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL, i, (void *)p))
			continue;

		if (p->client_instnum == client_instnum && p->instnum > instnum)
		{
			instnum = p->instnum;
		}
	}

	return (instnum + 1);
}

int rtk_tr369_get_ifname(char *ifname)
{
	int i = 0, total = mib_chain_total(MIB_ATM_VC_TBL);
	MIB_CE_ATM_VC_T entry;

	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_ATM_VC_TBL, i, &entry) != 1)
			continue;

		if (entry.applicationtype & X_CT_SRV_TR069)
		{
			if (ifGetName(entry.ifIndex, ifname, 16))
				return 1;
		}
	}
	return 0;
}

void rtk_tr369_restart_obuspa(void)
{
	unsigned int tr369_flag = 0;
	char obuspa_cmd[64] = {0}, obuspa_ifname[16] = {0};
	if ((findPidByName("obuspa")) > 0)
		va_cmd("/bin/killall", 1, 1, "obuspa");

	unlink(TR369_OBUSPA_DATABASE);

	mib_get_s(MIB_TR369_FLAG, &tr369_flag, sizeof(tr369_flag));

	if (!(tr369_flag & TR369_FLAG_USP_DISABLE))
	{
		if (rtk_tr369_get_ifname(obuspa_ifname))
		{
			snprintf(obuspa_cmd, sizeof(obuspa_cmd), "/bin/obuspa -p -v 4 -i %s &", obuspa_ifname);
			va_cmd("/bin/sh", 2, 0, "-c", obuspa_cmd);
		}
	}
	return;
}
#endif

#ifdef CONFIG_USER_ANDLINK_FST_BOOT_MONITOR
void start_fstBootMonitor(){
	unsigned char fstBootFlag = 0;

	if (read_pid(RTL_FST_BOOT_MONITOR_RUNFILE) <= 0){
		if (!mib_get_s(MIB_RTL_FST_BOOT_FLAG, (void *)&fstBootFlag, sizeof(fstBootFlag))){
			printf("[%s:%d] mib_get(MIB_RTL_FST_BOOT_FLAG) fail!!!\n",__FUNCTION__,__LINE__);
			return;
		}

		if (fstBootFlag)
			va_niced_cmd(RTL_FST_BOOT_MONITOR,1,0,"&");
	}
}

void notify_fstBootMonitor(char *ifName, int cnt, int status){
	int client_sock;
	unsigned int max_nums;
	char buff[RTL_FST_BOOT_MONITOR_MSG_SIZE];

	if (read_pid(RTL_FST_BOOT_MONITOR_RUNFILE) <= 0)
		return;

	if (isFileExist(RTL_FST_BOOT_MONITOR_TMP))
		return;

	if (!ifName || strcmp(ifName, BRIF) != 0)
		return;

	if (!mib_get_s(MIB_RTL_FST_BOOT_DHCPC_DISCOVER_NUMS, (void *)&max_nums, sizeof(max_nums))){
		printf("[%s:%d] mib_get_s MIB_RTL_FST_BOOT_DHCPC_DISCOVER_NUMS failed!\n",__FUNCTION__,__LINE__);
		max_nums = RTL_FST_BOOT_MONITOR_CNT_NO_REPLY;
	}

//	printf("[%s:%d] ifName=%s, cnt=%d(max_cnt=%d), status=%d\n",__FUNCTION__,__LINE__,ifName, cnt, max_nums, status);

	if (status == 0 && (cnt >= 0 && cnt <= max_nums))
		return;

	snprintf(buff, sizeof(buff), "touch %s", RTL_FST_BOOT_MONITOR_TMP);
	system(buff);

	client_sock = rtk_create_unix_domain_socket(0, (const char *)RTL_FST_BOOT_MONITOR_USOCKET, 0);
	if(client_sock < 0)
		return;

	memset(buff, 0, sizeof(buff));
	snprintf(buff, sizeof(buff), "(pid=%d) %s get IP %s", getpid(), ifName, status==0?"FAIL":"OK");
	send(client_sock, buff, sizeof(buff), 0);

	close(client_sock);
}
#endif
int get_process_info_by_pid(pid_t pid, process_pid_info *pInfo)
{
	char buf[256], *pbuf, *p;
	FILE *fp;
	int len, ret = 1;

	memset(pInfo, 0, sizeof(process_pid_info));
	snprintf(buf, sizeof(buf), "/proc/%d/stat", pid);
	if((fp = fopen(buf, "r")))
	{
		len = fread(buf, sizeof (char), sizeof (buf), fp);
		fclose(fp);

		buf[len] = '\0';
		if(len > 0)
		{
			pbuf = buf;
			// pid %d
			if((p = strchr(pbuf, ' ')))
			{
				*p = '\0';
				pInfo->pid = atoi(pbuf);
				if(pid != pInfo->pid) goto error;
				pbuf = p+1;
			}
			// name %s
			if((p = strchr(pbuf, ' ')))
			{
				*p = '\0'; *(p-1) = '\0';
				strncpy(pInfo->name, pbuf+1, sizeof(pInfo->name)-1);
				pbuf = p+1;
			}
			// stat %c
			if((p = strchr(pbuf, ' ')))
			{
				*p = '\0';
				pInfo->stat = *pbuf;
				if(pInfo->stat == 'Z') goto error;
				pbuf = p+1;
			}
			// ppid %d
			if((p = strchr(pbuf, ' ')))
			{
				*p = '\0';
				pInfo->ppid = atoi(pbuf);
				pbuf = p+1;
			}

			snprintf(buf, sizeof(buf), "/proc/%d/exe", pid);
			if(readlink(buf, pInfo->exe, sizeof(pInfo->exe)-1) < 0)
			{
				pInfo->exe[0] = '\0';
				if(pInfo->ppid == 2)
					pInfo->isKernel = 1;
			}
			ret = 0;
		}
	}

	return ret;

error:
	return -1;
}

process_pid_info* get_pidinfo_in_array(process_pid_info *pArray, int max, pid_t pid, int *index)
{
	int i;
	if(pArray == NULL || max <= 0) return NULL;
	for(i = 0; i<max; i++){
		if(pArray[i].pid == pid){
			if(index) *index = i;
			return &pArray[i];
		}
	}
	return NULL;
}

/******************************************************************************
 Function    : find_pidinfo_by_name
 Description : get proceess info
 - support full match or partial match
   partial match: *test, *test*, test*
******************************************************************************/
int find_pidinfo_by_name( char* ProcName, process_pid_info *pArray, int max)
{
	DIR             	*dir, *dir2;
	struct dirent   	*d, *d2;
	int             	pid, pid2, i, j;
	char            	*s, *s1, *processName, *pName;
	int 				pnlen, mode;
	process_pid_info	p;
	char 				exe [64], path[64];
	int 				len, match;
	int 				namelen;

	if(ProcName == NULL || pArray == NULL)
		return -1;

	processName = strdup(ProcName);
	if(processName == NULL)
	{
		fprintf(stderr, "[%s] allocate memory \n", __FUNCTION__);
		return -1;
	}
	mode=0;
	pName = processName;
	while((*pName == '*')) pName++;
	if(pName != processName)
		mode |= 0x1;
	pnlen = strlen(pName);
	if(pnlen)
	{
		for(i=pnlen; *(pName+i-1) == '*'; i--)
			;
		if(i != pnlen)
			mode |= 0x2;
		*(pName+i) = '\0';
		pnlen = i;
	}

	i = 0;
	/* Open the /proc directory. */
	dir = opendir("/proc");
	if (!dir)
	{
		if(processName) free(processName);
		fprintf(stderr, "[%s] cannot open /proc\n", __FUNCTION__);
		return -1;
	}

	/* Walk through the directory. */
	while ((d = readdir(dir)) != NULL && i < max)
	{
		/* See if this is a process */
		if ((pid = atoi(d->d_name)) == 0 || get_process_info_by_pid(pid, &p) != 0)       continue;

		snprintf(exe, sizeof(exe), "/proc/%s/exe", d->d_name);
		if ((len = readlink(exe, path, sizeof(path)-1)) < 0)
				continue;
		path[len] = '\0';

		/* match ProcName */
		s = strrchr(p.exe, '/');
		if(s != NULL) s++;
		else s = p.exe;

		match = 0;
		if(mode)
		{
			if(pnlen <= 0)
				match = 1;
			else
			{
				if((s1 = strstr(p.name, pName)))
				{
					if(mode == 0x1 && (s1-p.name+pnlen) == strlen(p.name))
						match = 1;
					else if(mode == 0x2 && s1 == p.name)
						match = 1;
					else if(mode == 0x3)
						match = 1;
				}

				if(match == 0 && s && (s1 = strstr(s, pName)))
				{
					if(mode == 0x1 && (s1-s+pnlen) == strlen(s))
						match = 1;
					else if(mode == 0x2 && s1 == s)
						match = 1;
					else if(mode == 0x3)
						match = 1;
				}
			}
		}
		else
		{
			match = (!strcmp(p.name, pName) || (s && !strcmp(s, pName)));
		}

		if(match) {
			/* to avoid subname like search proc tao but proc taolinke matched */
			if(get_pidinfo_in_array(pArray, i, pid, NULL) == NULL)
			{
				memcpy(&pArray[i++], &p, sizeof(p));
				sprintf(path, "/proc/%d/task", pid);
				dir2 = opendir(path);
				if (!dir2)
				{
					fprintf(stderr, "[%s] cannot open %s\n", __FUNCTION__, path);
				}
				else
				{
					while ((d2 = readdir(dir2)) != NULL && i < max)
					{
						if ((pid2 = atoi(d2->d_name)) == 0 || pid2 == pid)  continue;
						if(get_pidinfo_in_array(pArray, i, pid2, NULL) == NULL &&
						   get_process_info_by_pid((pid_t)pid2, &pArray[i]) == 0)
						{
							pArray[i].isThread = 1;
							i++;
						}
					}
					closedir(dir2);
				}
			}
		}
	}

	closedir(dir);
	if(processName) free(processName);

	return  i;
}

#ifdef CONFIG_SECONDARY_IP
int rtk_set_secondary_ip(void)
{
	unsigned char value[6]={0};
	char ipaddr1[16]={0},ipaddr2[16]={0},subnet[16]={0},mtuStr[10];
	int mtu=1500;
	struct in_addr inAddr;
	mib_get_s(MIB_ADSL_LAN_ENABLE_IP2, (void *)value, sizeof(value));
	if (value[0] == 1) {
		// ifconfig LANIF LAN_IP netmask LAN_SUBNET
		if (mib_get_s(MIB_ADSL_LAN_IP2, (void *)value, sizeof(value)) != 0)
		{
			strncpy(ipaddr2, inet_ntoa(*((struct in_addr *)value)), 16);
			ipaddr2[15] = '\0';
		}

		if (getInAddr((char*)BRIF, IP_ADDR, (void *)&inAddr) == 1)
			strncpy(ipaddr1, inet_ntoa(inAddr), 16);
		else
			getMIB2Str(MIB_ADSL_LAN_IP, ipaddr1);

		if(!strcmp(ipaddr1,ipaddr2))
			return RTK_SUCCESS;

		if (mib_get_s(MIB_ADSL_LAN_SUBNET2, (void *)value, sizeof(value)) != 0)
		{
			strncpy(subnet, inet_ntoa(*((struct in_addr *)value)), 16);
			subnet[15] = '\0';
		}

		mtu=rtk_wan_get_max_wan_mtu();
		snprintf(mtuStr, sizeof(mtuStr), "%u", mtu);
		va_cmd(IFCONFIG, 6, 1, (char*)LAN_ALIAS, ipaddr2, "netmask", subnet, "mtu", mtuStr);
	}
	return RTK_SUCCESS;
}


#endif

void calPasswd(char *pass, char *crypt_pass)
{
	char *xpass;
	char salt[128];
	rtk_get_crypt_salt(salt, sizeof(salt));
	xpass = crypt(pass, salt);
	if(xpass)
		strcpy(crypt_pass, xpass);
	else
		*crypt_pass = '\0';
}

void rtk_get_crypt_salt(char *salt, unsigned int len){
	char saltkey[17]={0};

	if(!salt || !len)
		return;

	memset(salt, 0x0, len);
	if(mib_get_s(MIB_PWD_SALTKEY, saltkey, sizeof(saltkey))){
		if(saltkey[0]){
			snprintf(salt, len, "$5$%s$", saltkey);
		}else{
			snprintf(salt, len, "$5$");
		}
	}

	return;
}
/* Change the mac address from "xx:xx:xx:xx:xx:xx" to "xxxxxxxxxxxx"*/
int changeMacFormatWlan(char *str, char s)
{
	int i = 0, j = 0;
	int length = strlen(str);
	for(i = 0; i < length; i++)
	{
		if(str[i] == s)
		{
			for(j = i; j<length; ++j)
				str[j] = str[j+1];
			str[j] = '\0';
		}
	}
	return 0;
}

int ValidatePINCode_s(char *code)
{
	int i, accum = 0;

	for( i = 0; i < 8; i++ )
		accum += (1 + ((i+1) & 1) * 2) * (code[i] - '0');

	return accum % 10? -3: 0;
}

int CheckPINCode_s(char *PIN)
{
	int i, code_len = 0;

	code_len = strlen(PIN);

	if(code_len != 8 && code_len != 4)
		return -1;

	for( i = 0; i < code_len; i++ )
	{
		if( PIN[i] < '0' || PIN[i] > '9' )
			return -2;
	}

	if(code_len == 8)
		return ValidatePINCode_s(PIN);
	else
		return 0;
}

int getIpRange(char *src, char* start, char*end)
{
	struct in6_addr address6;
	struct in_addr address;
	int iptype=0;
	int ip_addr1, ip_addr2, ip_addr3;

	if(!strcmp(src, "") || !strcmp(src, "0")){
	    iptype = 0;
	}
	else if(inet_pton(AF_INET, src, &address) == 1){
	    iptype = IPVER_IPV4;
	}
	else if(inet_pton(AF_INET6, src, &address6) == 1){
	    iptype = IPVER_IPV6;
	}
	else{
	    iptype = 0;
	}

	if(iptype==IPVER_IPV4){
			address.s_addr = ntohl(address.s_addr);
			if(address.s_addr & 0x00000FF){//single ip
			sprintf(start, "%s", src);
			sprintf(end, "%s", src);
			return 1;
	        }
        	else{//range
			sscanf(src, "%d.%d.%d.*%d", &ip_addr1, &ip_addr2, &ip_addr3);
			sprintf(start, "%d.%d.%d.1", ip_addr1, ip_addr2, ip_addr3);
			sprintf(end, "%d.%d.%d.255", ip_addr1, ip_addr2, ip_addr3);
			return 2;
		}
	}
	else if(iptype==IPVER_IPV6){
		if(address6.s6_addr32[3]==0){//range
			strcpy(start, src);
			address6.s6_addr32[3]=0xffffffff;
			inet_ntop(AF_INET6, &address6, end, 64);
			return 2;
		}
		else{
			sprintf(start, "%s", src);
			sprintf(end, "%s", src);
			return 1;
		}
	}
	return -1;
}

//#ifdef COM_CUC_IGD1_AUTODIAG
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
#ifdef CONFIG_COMMON_RT_API
#include <rtk/rt/rt_ponmisc.h>
#endif
#endif
int is_laser_force_on(void)
{
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
#ifdef CONFIG_COMMON_RT_API
	rt_ponmisc_laser_status_t status;
	if(rt_ponmisc_forceLaserState_get(&status) == RT_ERR_OK) {
		if(status == RT_PONMISC_LASER_STATUS_FORCE_ON)
			return 1;
	}
#else
	unsigned int pon_mode;

	mib_get(MIB_PON_MODE, &pon_mode);
	if (pon_mode == GPON_MODE) {
#ifdef CONFIG_GPON_FEATURE
		FILE *fp;
    		char cmdStr[256], line[256],tmpStr[32] = {0};

		snprintf(cmdStr, 256, "diag gpon get tx &> /tmp/gpon_laser_state");
		va_cmd("/bin/sh", 2, 1, "-c", cmdStr);
		fp = fopen("/tmp/gpon_laser_state", "r");
		if (fp)
		{
		    while(fgets(line, sizeof(line), fp)){
				if(sscanf(line, "TX laser mode: Force %s", tmpStr)){
					break;
				}
			}
		    fclose(fp);
		}
		if(!strcmp(tmpStr,"on"))
			return 1;
#endif
	} else if (pon_mode == EPON_MODE) {
#ifdef CONFIG_EPON_FEATURE
		rtk_epon_laser_status_t state;
#if defined(CONFIG_RTK_L34_ENABLE)
		rtk_rg_epon_forceLaserState_get(&state);
#else
		rtk_epon_forceLaserState_get(&state);
#endif
		if (state == RTK_EPON_LASER_STATUS_FORCE_ON)
			return 1;
#endif
	}
#endif
#endif
	return 0;
}

int is_pon_los(void)
{
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	rtk_enable_t pon_lost = DISABLED;
	unsigned int pon_mode;

	mib_get(MIB_PON_MODE, &pon_mode);
	if (pon_mode == GPON_MODE) {
#ifdef CONFIG_GPON_FEATURE
#if defined(CONFIG_RTK_L34_ENABLE)
		rtk_rg_ponmac_losState_get(&pon_lost);
#else
		rtk_ponmac_losState_get(&pon_lost);
#endif
#endif
	} else if (pon_mode == EPON_MODE) {
#ifdef CONFIG_EPON_FEATURE
#if defined(CONFIG_RTK_L34_ENABLE)
		rtk_rg_epon_losState_get(&pon_lost);
#else
		rtk_epon_losState_get(&pon_lost);
#endif
#endif
	}
	if (pon_lost == ENABLED )
		return 1;
#endif
	return 0;
}

int is_optical_fiber_match_with_pon_mode(void)
{
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	rtk_ponmac_mode_t check_mode;
    unsigned int sd, sync;
	unsigned int pon_mode;
	int ret;

	mib_get(MIB_PON_MODE, &pon_mode);
	if(pon_mode == GPON_MODE)
		check_mode = 0;
	else if(pon_mode == EPON_MODE)
		check_mode = 1;
	else
		return 1;

    if((ret = rtk_ponmac_linkState_get(check_mode, &sd, &sync))!= RT_ERR_OK)
        return 1;

	/* wrong pon mode */
    if((sd == 1) && (sync == 0)){
		printf("[%s] optical fiber doesn't match with the pon mode.",__func__);
		return 0;
    }

#endif
	return 1;
}

unsigned char get_oam_max_state(void)
{
	FILE *fp = NULL;
	unsigned char max_oam_state = 0;
    unsigned char buff[256];

	fp = fopen("/tmp/max_oam_state_during_regfail","r");
	if(fp){
	    if(fgets(buff, sizeof(buff), fp) != NULL)
	        sscanf(buff, "%hhu\n", &max_oam_state);
		fclose(fp);
	}

	return max_oam_state;
}

void clear_oam_max_state(void)
{
	system("rm /tmp/max_oam_state_during_regfail");
}

void set_oam_max_state(unsigned char oam_state)
{
	char cmd[1024] ={0};
	printf("[set_oam_max_state] oam state max = %d!\n",oam_state);
	snprintf(cmd,sizeof(cmd)-1,"echo %d > /tmp/max_oam_state_during_regfail",oam_state);
	system(cmd);
}

#define PON_ONU_LOID_SUCCESSFUL_AUTHENTICATION 1
int is_loid_reg_timeout(void)
{
    FILE *fp;
    unsigned char buff[256];
	unsigned char authStatus = 255;
    int timeout = 0;

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	unsigned int pon_mode;
	mib_get(MIB_PON_MODE, &pon_mode);
#ifdef CONFIG_GPON_FEATURE
	if (pon_mode == GPON_MODE) {
	    fp = popen("omcicli get loidauth | awk -F:  '/Auth Status : /{print $2}' | sed 's/ //g'", "r");
		if(fp){
		    if(fgets(buff, sizeof(buff), fp) != NULL){
		        sscanf(buff, "%hhu\n", &authStatus);
				/*
					loidauth = 0 (loid auth init)   : onu disabled before OLT registered
					loidauth = 1 (loid auth success): onu disabled after OLT registered
				*/
				if(authStatus <= PON_ONU_LOID_SUCCESSFUL_AUTHENTICATION)
					timeout = 1;
		    }
		    pclose(fp);
		}
	}
#endif
#ifdef CONFIG_EPON_FEATURE
	if (pon_mode == EPON_MODE) {
		fp = fopen("/tmp/max_oam_state_during_regfail","r");
		if(fp){
		    if(fgets(buff, sizeof(buff), fp) != NULL){
		        sscanf(buff, "%hhu\n", &authStatus);
				//OAM COMPLETE: O5 ---> can't do loid auth
				if(authStatus <= RTK_EPON_STATE_OAM_COMPLETE)
					timeout = 1;
		    }
			fclose(fp);
		}
	}
#endif
#endif

END:
	return timeout;
}

int is_olt_register_ok(void)
{
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	int loid_exist=0;
	unsigned int pon_mode;
	int pon_state;

	mib_get(MIB_PON_MODE, &pon_mode);
	if (pon_mode == GPON_MODE) {
#ifdef CONFIG_GPON_FEATURE
		pon_state = getGponONUState();
		if (pon_state == 5)
			return 1;
#endif
	} else if (pon_mode == EPON_MODE) {
#ifdef CONFIG_EPON_FEATURE
		pon_state = getEponONUState(0);
		if (pon_state == 5)
		{
			if(1 == epon_getAuthState(0)) //0--not complete ,1--successful, >=2-- fail
				return 1;
		}
#endif
	}
#endif
	return 0;
}

int is_pppoe_internet_wan_exist(void)
{
	int iWanTotal = 0, i = 0, iFound = 0;
	MIB_CE_ATM_VC_T WanEntry;

	iWanTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < iWanTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&WanEntry))
			continue;

		//check pppoe internet route wan
		if (WanEntry.cmode == CHANNEL_MODE_PPPOE && (WanEntry.applicationtype & X_CT_SRV_INTERNET))
		{
			iFound = 1;
			break;
		}
	}

	return iFound;
}

const char DBUS_AUTO_DIAG_RESULT[] = "/tmp/auto_diag_result";
unsigned char getAutoDiagResult(void)
{
	FILE *fp;
	unsigned char result = INIT_FAULT_CODE;
	char buff[256] = {0};
	fp=fopen(DBUS_AUTO_DIAG_RESULT, "r");
	if(fp){
		if (fgets(buff, sizeof(buff), fp) != NULL){
			sscanf(buff, "%hhu\n", &result);
		}
		fclose(fp);
	}
	return result;
}

int needAutoDiagRedirect(void)
{
	unsigned char urlredirect_enable = 0;

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	mib_get(MIB_AUTODIAG_REDIRECT_ENABLE, (void *)&urlredirect_enable);
#endif

	if(urlredirect_enable == 0)
		return 0;

	/*
		ChinaUnicom Auto Diagnostics:
		Auto Diag Function works with PPPoE Internet.
		Don't take effect when no PPPoE Internet exists.
	*/
	if(0 == is_pppoe_internet_wan_exist())
		return 0;

	return (getAutoDiagResult() != ERR_AUTODIAG_NONE);
}

void setAutoDiagResult(unsigned char result)
{
	FILE *fp;
	char buff[256] = {0};

	switch(result){
		case ERR_PON_LOS:
		case ERR_ONU_LASER_FORCE_ON:
			break;
		case ERR_REG_FAIL:
		case ERR_REG_TIMEOUT:
			if(is_laser_force_on() || is_pon_los())
				return;
			if(is_loid_reg_timeout())
				result = ERR_REG_TIMEOUT;
			break;
		case ERR_PPPOE_AUTH_FAIL:
		case ERR_PPPOE_AUTH_TIMEOUT:
		case ERR_PPPOE_ACQUIRE_IP_FAIL:
		case ERR_AUTODIAG_NONE:
			if(is_laser_force_on() || is_pon_los() || !is_olt_register_ok())
				return;
			break;
		default:
			return;
	}

	if(getAutoDiagResult() != result){
		printf("%s:   result = %d\n",__FUNCTION__,result);
		fp=fopen(DBUS_AUTO_DIAG_RESULT, "w");
		if(fp){
			sprintf(buff, "%d\n", result);
			fputs(buff, fp);
			fclose(fp);
		}
#ifdef COM_CUC_IGD1_AUTODIAG
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
		restart_dnsrelay();
#endif
#endif
	}
}

enum ppp_last_connection_error {
	ERROR_NONE=0, ERROR_ISP_TIME_OUT=1, ERROR_COMMAND_ABORTED=2,
	ERROR_NOT_ENABLED_FOR_INTERNET=3, ERROR_BAD_PHONE_NUMBER=4,
	ERROR_USER_DISCONNECT=5, ERROR_ISP_DISCONNECT=6, ERROR_IDLE_DISCONNECT=7,
	ERROR_FORCED_DISCONNECT=8, ERROR_SERVER_OUT_OF_RESOURCES=9,
	ERROR_RESTRICTED_LOGON_HOURS=10, ERROR_ACCOUNT_DISABLED=11,
	ERROR_ACCOUNT_EXPIRED=12, ERROR_PASSWORD_EXPIRED=13,
	ERROR_AUTHENTICATION_FAILURE=14, ERROR_NO_DIALTONE=15, ERROR_NO_CARRIER=16,
	ERROR_NO_ANSWER=17, ERROR_LINE_BUSY=18, ERROR_UNSUPPORTED_BITSPERSECOND=19,
	ERROR_TOO_MANY_LINE_ERRORS=20, ERROR_IP_CONFIGURATION=21, ERROR_UNKNOWN=22
	,ERROR_ISP_DISCONNECT_IPv4=23
	,ERROR_ISP_DISCONNECT_IPv6=24
};
void checkAutoDiagPPPoEResult(void)
{
	int i = 0, total = 0;
	int pppif = 0, ppp_unit = 0, found = 0;
	MIB_CE_ATM_VC_T wanEntry;
	FILE *fp;
	char buff[256];
	unsigned char autodiag_result = INIT_FAULT_CODE;
	enum ppp_last_connection_error error = ERROR_UNKNOWN;

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0 ; i < total ; i++){
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&wanEntry))
			continue;
		/*CU: There is only one PPPoE Internet Wan*/
		if((wanEntry.applicationtype & X_CT_SRV_INTERNET) && (wanEntry.cmode == CHANNEL_MODE_PPPOE)){
			ppp_unit = PPP_INDEX(wanEntry.ifIndex);
			break;
		}
	}

	fp = fopen("/tmp/ppp_error_log", "r");
	if(fp){
		while(fgets(buff, sizeof(buff), fp) != NULL){
			sscanf(buff, "%d:%d", &pppif, (int*)&error);
			if(pppif == ppp_unit){
				found = 1;
				break;
			}
		}
		fclose(fp);
	}
	if(found){
		if(error == ERROR_AUTHENTICATION_FAILURE)
			autodiag_result = ERR_PPPOE_AUTH_FAIL;
		else if(error == ERROR_ISP_TIME_OUT || error == ERROR_ACCOUNT_EXPIRED
			|| error == ERROR_PASSWORD_EXPIRED)
			autodiag_result = ERR_PPPOE_AUTH_TIMEOUT;
		else if(error == ERROR_IP_CONFIGURATION || error == ERROR_NOT_ENABLED_FOR_INTERNET
			|| error == ERROR_NO_ANSWER)
			autodiag_result = ERR_PPPOE_ACQUIRE_IP_FAIL;
		if(autodiag_result != INIT_FAULT_CODE && autodiag_result != getAutoDiagResult()){
			//printf("[%s:%d] setAutoDiagResult = %d\n",__func__,__LINE__,autodiag_result);
			setAutoDiagResult(autodiag_result);
		}
	}
}
//#endif
int get_host_connected_interface(const char *brname, unsigned char *macaddr, char *ifname)
{
	struct __fdb_entry fdb[256];
	int offset = 0;
	unsigned long args[4];
	struct ifreq ifr;
	int br_socket_fd = -1;

	args[0] = BRCTL_GET_FDB_ENTRIES;
	args[1] = (unsigned long)fdb;
	args[2] = 256;
	if ((br_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		return -1;
	}
	else
	{
		memcpy(ifr.ifr_name, brname, IFNAMSIZ);
		((unsigned long *)(&ifr.ifr_data))[0] = (unsigned long)args;
		while (1)
		{
			int i = 0;
			int num = 0;

			args[3] = offset;
			num = ioctl(br_socket_fd, SIOCDEVPRIVATE, &ifr);

			if (!num)
				break;

			for (i = 0; i < num; i++) {
				struct __fdb_entry *f = fdb + i;

				if (memcmp(f->mac_addr, macaddr, 6) == 0) {
					int ifindices[256];
					int list_num = 0;
					unsigned long list_args[4] = { BRCTL_GET_PORT_LIST, (unsigned long)ifindices, 256, 0 };
					struct ifreq list_ifr;

					memset(ifindices, 0, sizeof(ifindices));
					strncpy(list_ifr.ifr_name, brname, IFNAMSIZ);
					list_ifr.ifr_data = (char *) &list_args;

					list_num = ioctl(br_socket_fd, SIOCDEVPRIVATE, &list_ifr);
					if (list_num < 0) {
						close(br_socket_fd);
						return -1;
					}

					if_indextoname(ifindices[f->port_no], ifname);
					close(br_socket_fd);
					return 0;
				}
			}

			offset += num;
		}
		close(br_socket_fd);
	}
	return 1;
}

void rtk_send_signal_to_daemon(char *pidfile, int sig)
{
	pid_t pid;

	pid = read_pid(pidfile);
	if (pid <= 0)
		return;

	kill(pid, sig);
}

int rtk_util_write_str(const char *path, int wait, char *content)
{
	int fd, res=0;
	int flags = O_WRONLY | O_CREAT;

	if(path == NULL || content == NULL)
		return -1;

	if(wait)
		flags |= O_NONBLOCK;

	fd = open(path, flags, 0666);
	if (fd == -1)
	    return -1;

	res = write(fd, content, strlen(content)+1);

	if (res == -1) {
	    fprintf(stderr, "write %s failed\n", path);
	    close(fd);
	    return -1;
	}

	close(fd);
	return res;
}

long convert_timeStr_to_epoch(int year, int mon,int day, int hour, int min, int sec)
{
	struct tm t;
	t.tm_year = year-1900;		// Year - 1900
	t.tm_mon = mon -1;		// Month, where 0 = jan
	t.tm_mday = day;			// Day of the month
	t.tm_hour = hour;
	t.tm_min = min;
	t.tm_sec = sec;
	t.tm_isdst = -1;		// Is DST on? 1 = yes, 0 = no, -1 = unknown
	return mktime(&t);
}

char* string_skip_space(char* str)
{
	char* str_s = NULL;
	for(; *str; str++)
	{
		if(*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n')
		{
			if(str_s == NULL)
				continue;
			else
			{
				*str='\0';
				break;
			}
		}
		else if(str_s == NULL)
			str_s = str;
	}
	if(str_s == NULL)
		str_s = str;
	return str_s;
}

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
int rtk_iconv_scan_multi_bytes(const char *pSrc)
{
	int i;
	for (i = 0; i < strlen(pSrc); i ++)
	if (*(pSrc+i) & 0x80)
		return 1;
	return 0;
}

int utf8_check(const char* str, size_t length)
{
	size_t i;
	int nBytes;
	unsigned char chr;

	i = 0;
	nBytes = 0;
	while (i < length) {
		chr = *(str + i);

		if (nBytes == 0) {
			if ((chr & 0x80) != 0) {
				while ((chr & 0x80) != 0) {
					chr <<= 1;
					nBytes++;
				}
				if ((nBytes < 2) || (nBytes > 6)) {
					return 0;
				}
				nBytes--;
			}
		} else {
			if ((chr & 0xC0) != 0x80) {
				return 0;
			}
			nBytes--;
		}
		i++;
	}
	return (nBytes == 0);
}


int gbk_check(const char* str, int len)  {
    int i  = 0;
    while (i < len)  {
        if (str[i] <= 0x7f) {
            //this is not multyBytes char
            i++;
            continue;
        } else {
            //check multyBytes is gbk(1st byte range:x81-0xFE;2nd byte range:0x40-0xFE)
            if  (str[i] >= 0x81 &&
                str[i] <= 0xfe &&
                str[i + 1] >= 0x40 &&
                str[i + 1] <= 0xfe &&
                str[i + 1] != 0xf7) {
                i += 2;
                continue;
            } else {
                return 0;
            }
        }
    }
    return 1;
}

int rtk_iconv_utf8_to_gbk(char* src, char *dst, int len)
{
	char file_name[128];
	char gbk_file_name[128];
	char cmd[512];
	int fd, read_size;
	struct stat status;
	char *pOut=NULL;

	//if the string is utf8 string, just return
	if((utf8_check(src, strlen(src))==1) && (gbk_check(src, strlen(src)) == 0))
	{
		printf("%s:%d already utf8\n", __FUNCTION__, __LINE__);
		strcpy(dst, src);
		return 0;
	}

	snprintf(gbk_file_name, sizeof(gbk_file_name), "/tmp/gbk");
	snprintf(file_name, sizeof(file_name), "/tmp/gbk_to_utf8");

	fd = open(gbk_file_name, O_RDWR|O_CREAT, 0644);
	if (fd < 0)
	{
		printf("%s:%d can not create file\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if( !flock_set(fd, F_WRLCK) )
	{
		write(fd, (void *)src, strlen(src));
		flock_set(fd, F_UNLCK);
	}
	close(fd);

	snprintf(cmd, sizeof(cmd), "/bin/iconv -f UTF-8 -t GBK %s > %s", gbk_file_name, file_name);
	system(cmd);

	if ( stat(file_name, &status) < 0 ){
		printf("%s:%d can not stat file\n", __FUNCTION__, __LINE__);
		return -1;
	}
	read_size = (unsigned long)(status.st_size);
	if(read_size <=0){
		printf("%s:%d file empty\n", __FUNCTION__, __LINE__);
		return -1;
	}
	pOut = (char*)malloc(read_size+1);
	memset(pOut, 0, read_size+1);

	fd = open(file_name, O_RDONLY);
	if(fd < 0){
		unlink(gbk_file_name);
		unlink(file_name);
		free(pOut);
		printf("%s:%d can not read file\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if(!flock_set(fd, F_RDLCK))
	{
		read_size = read(fd, (void*)pOut, read_size);
		flock_set(fd, F_UNLCK);
	}
	close(fd);

	memcpy(dst, pOut, len);
	free(pOut);
	unlink(gbk_file_name);
	unlink(file_name);

	return 0;
}

int rtk_iconv_gbk_to_utf8(char* src, char *dst, int len)
{
	char file_name[128];
	char gbk_file_name[128];
	char cmd[512];
	int fd, read_size;
	struct stat status;
	char *pOut=NULL;

	//if the string is utf8 string, just return
	if(utf8_check(src, strlen(src))==1)
	{
		//printf("%s:%d already utf8\n", __FUNCTION__, __LINE__);
		strcpy(dst, src);
		return 0;
	}

	snprintf(gbk_file_name, sizeof(gbk_file_name), "/tmp/gbk");
	snprintf(file_name, sizeof(file_name), "/tmp/gbk_to_utf8");

	fd = open(gbk_file_name, O_RDWR|O_CREAT, 0644);
	if (fd < 0)
	{
		printf("%s:%d can not create file\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if( !flock_set(fd, F_WRLCK) )
	{
		write(fd, (void *)src, strlen(src));
		flock_set(fd, F_UNLCK);
	}
	close(fd);

	snprintf(cmd, sizeof(cmd), "/bin/iconv -f GBK -t UTF-8 %s > %s", gbk_file_name, file_name);
	system(cmd);

	if ( stat(file_name, &status) < 0 ){
		printf("%s:%d can not stat file\n", __FUNCTION__, __LINE__);
		return -1;
	}
	read_size = (unsigned long)(status.st_size);
	if(read_size <=0){
		printf("%s:%d file empty\n", __FUNCTION__, __LINE__);
		return -1;
	}
	pOut = (char*)malloc(read_size+1);
	memset(pOut, 0, read_size+1);

	fd = open(file_name, O_RDONLY);
	if(fd < 0){
		unlink(gbk_file_name);
		unlink(file_name);
		free(pOut);
		printf("%s:%d can not read file\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if(!flock_set(fd, F_RDLCK))
	{
		read_size = read(fd, (void*)pOut, read_size);
		flock_set(fd, F_UNLCK);
	}
	close(fd);

	memcpy(dst, pOut, len);
	free(pOut);
	unlink(gbk_file_name);
	unlink(file_name);
	return 0;
}
#endif



#ifdef COM_CUC_IGD1_WMMConfiguration
WIFI_QOS_MAPPING_T wifi_mapping[] = {
	{1,0}, {2,0}, {3,1},{4,2},{5,2},{6,3},{7,3},
};

int checkWmmServiceExists()
{
	unsigned int ret=0, i,num;
	MIB_CE_WMM_SERVICE_T wmm_entity;
	unsigned char zero[IP_ADDR_LEN] = {0};

	num = mib_chain_total( MIB_WMM_SERVICE_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_WMM_SERVICE_TBL, i, (void*)&wmm_entity ))
			continue;

		if(memcmp(zero, wmm_entity.sip, IP_ADDR_LEN) != 0
			|| memcmp(zero, wmm_entity.dip, IP_ADDR_LEN) != 0
			|| wmm_entity.sPort>0 || (wmm_entity.dPort>0)
			)
		{
			ret = 1;
			break;
		}
	}

	return ret;
}

void disableWifiTxQosMapping()
{
	char sysbuf[256]={0};
	int i;

	sprintf(sysbuf, "/bin/echo disable > /proc/fc/ctrl/wifi_tx_qos_mapping");
	system(sysbuf);
	for(i=0 ; i<MAX_QOS_QUEUE_NUM ; i++)
	{
		sprintf(sysbuf, "/bin/echo %d 1 > /proc/fc/ctrl/wifi_tx_qos_mapping", i);
		system(sysbuf);
	}
}

void enableWifiTxQosMapping()
{
	int i, len=0;
	char sysbuf[256]={0};

	len = sizeof(wifi_mapping)/sizeof(WIFI_QOS_MAPPING_T);
	for(i=0 ; i<len ; i++)
	{
		sprintf(sysbuf, "/bin/echo %d %d > /proc/fc/ctrl/wifi_tx_qos_mapping", wifi_mapping[i].prior, wifi_mapping[i].wifiQueueIndex);
//		printf("system(): %s\n", sysbuf);
		system(sysbuf);
	}

	sprintf(sysbuf, "/bin/echo enable > /proc/fc/ctrl/wifi_tx_qos_mapping");
//	printf("system(): %s\n", sysbuf);
	system(sysbuf);
}

void setup_wmmservice_rules()
{
	int num;

	num = mib_chain_total( MIB_WMM_SERVICE_TBL );
	if(checkWmmServiceExists())
	{
		enableWifiTxQosMapping();
	}
	else
	{
		if(num == 0)
			flush_wmm_service_tbl();
		disableWifiTxQosMapping();
	}
}

int setup_WmmService(MIB_CE_WMM_SERVICE_T *poldWmmRule, MIB_CE_WMM_SERVICE_T *pWmmRule, int chainid, int addflag)
{
	int wmmenable=0;
	if((NULL == pWmmRule) || (NULL == poldWmmRule))
		return -1;

	wmmenable = getWMMEnable(pWmmRule->wmmconfinst);
	if(addflag && (wmmenable == 0))
		return -1;

	rtk_fc_del_wmm_service_rule(poldWmmRule);
	if(addflag)
	{
		rtk_fc_add_wmm_service_rule(pWmmRule);
	}

	setup_wmmservice_rules();
	return 0;
}

void setup_WmmRule_startup()
{
	int i, num;
	MIB_CE_WMM_SERVICE_T *p, wmm_entity;

	num = mib_chain_total( MIB_WMM_SERVICE_TBL );
	if(num > 0)
		set_wmm_service_tbl();
	for( i=0; i<num;i++ )
	{
		p = &wmm_entity;
		if( !mib_chain_get( MIB_WMM_SERVICE_TBL, i, (void*)p ))
			continue;

		rtk_fc_add_wmm_service_rule(p);
	}
}
#endif


#ifdef COM_CUC_IGD1_WMMConfiguration_ipset
WIFI_QOS_MAPPING_T wifi_mapping[] = {
	{1,0}, {2,0}, {3,1},{4,2},{5,2},{6,3},{7,3},
};

void disableWifiTxQosMapping()
{
	char sysbuf[256]={0};
	int i;

	sprintf(sysbuf, "/bin/echo disable > /proc/fc/ctrl/wifi_tx_qos_mapping");
	system(sysbuf);
	for(i=0 ; i<MAX_QOS_QUEUE_NUM ; i++)
	{
		sprintf(sysbuf, "/bin/echo %d 1 > /proc/fc/ctrl/wifi_tx_qos_mapping", i);
		system(sysbuf);
	}
}

void enableWifiTxQosMapping()
{
	int i, len=0;
	char sysbuf[256]={0};

	len = sizeof(wifi_mapping)/sizeof(WIFI_QOS_MAPPING_T);
	for(i=0 ; i<len ; i++)
	{
		sprintf(sysbuf, "/bin/echo %d %d > /proc/fc/ctrl/wifi_tx_qos_mapping", wifi_mapping[i].prior, wifi_mapping[i].wifiQueueIndex);
//		printf("system(): %s\n", sysbuf);
		system(sysbuf);
	}

	sprintf(sysbuf, "/bin/echo enable > /proc/fc/ctrl/wifi_tx_qos_mapping");
//	printf("system(): %s\n", sysbuf);
	system(sysbuf);
}

void setup_wmmservice_rules()
{
	int num;

	enableWifiTxQosMapping();

	//disableWifiTxQosMapping();
}
#endif

void updateScheduleCrondFile(char *pathname, int startup)
{
	char *strVal;
	int totalnum, i, j, first, crond_pid;
	FILE *fp = NULL, *getPIDS = NULL;
	char tmpbuf[100], day_string[20], kill_cmd[100], filename[100];
#ifdef WIFI_TIMER_SCHEDULE
	MIB_CE_WIFI_TIMER_EX_T Entry_ex;
	MIB_CE_WIFI_TIMER_T Entry;
	unsigned char startHour, startMinute, endHour, endMinute;
#endif
#ifdef LED_TIMER
	MIB_CE_DAY_SCHED_T ledEntry;
	unsigned char led_status=0;
	unsigned char led_timer_enable=0;
#endif
#ifdef SLEEP_TIMER
	MIB_CE_SLEEPMODE_SCHED_T sleepEntry;
#endif
	int schedFileExist = 0;
	char site_survey_time; // minute
	char guest_ssid; // ssid_index
	unsigned int guest_ssid_duration;
	int guestSSIDExist=0;
	unsigned long guest_ssid_endtime;
	int current_state;

#ifdef CONFIG_E8B
	if(startup){
		//snprintf(tmpbuf, 100, "mkdir -p %s", pathname);
		//system(tmpbuf);
#ifdef WLAN_CLIENT
		getSiteSurveyWlanNeighborAsync(0);
#ifdef WLAN_DUALBAND_CONCURRENT
		getSiteSurveyWlanNeighborAsync(1);
#endif
#endif
	}
#endif //CONFIG_E8B

#ifdef CONFIG_YUEME
	snprintf(tmpbuf, sizeof(tmpbuf), "root");
#else
	if ( !mib_get_s(MIB_SUSER_NAME, (void *)tmpbuf, sizeof(tmpbuf)) ) {
		printf("ERROR: Get superuser name from MIB database failed.\n");
		return;
	}
#endif
	snprintf(filename, 100, "%s/%s", pathname, tmpbuf);

	crond_pid = read_pid("/var/run/crond.pid");
	if(crond_pid > 0){
		kill(crond_pid, 9);
		unlink("/var/run/crond.pid");
	}

	fp = fopen(filename, "w");
    if (fp == NULL)
    {
        printf("ERROR: error to file open %s .\n", filename);
        return;
    }

#ifdef CONFIG_E8B
#ifdef WIFI_TIMER_SCHEDULE
	totalnum = mib_chain_total(MIB_WIFI_TIMER_EX_TBL);
	for(i=0; i<totalnum; i++){
		mib_chain_get(MIB_WIFI_TIMER_EX_TBL, i, &Entry_ex);

		first = 1;
		if(Entry_ex.day & (1<<7)){
			if(first){
				snprintf(day_string, sizeof(day_string), "0");
				first = 0;
			}
			else
				strncat(day_string, "%s,0", sizeof(day_string)-strlen(day_string)-1);
		}
		for(j=1;j<7;j++){
			if(Entry_ex.day & (1<<j)){
				if(first){
					snprintf(day_string, sizeof(day_string), "%d", j);
					first = 0;
				}
				else
				{
					int len = strlen(day_string);
					snprintf(day_string+len, sizeof(day_string)-len,",%d", j);
				}
			}
		}
		if(Entry_ex.enable){
			sscanf(Entry_ex.Time, "%hhu:%hhu", &startHour, &startMinute);
			snprintf(tmpbuf, 100, "%hhu %hhu * * %s /bin/config_wlan %d %u\n", startMinute, startHour, day_string, Entry_ex.onoff, Entry_ex.SSIDMask);
			fputs(tmpbuf,fp);
			if(schedFileExist == 0)
				schedFileExist = 1;
		}
	}

	totalnum = mib_chain_total(MIB_WIFI_TIMER_TBL);
	for(i=0; i<totalnum; i++){
		mib_chain_get(MIB_WIFI_TIMER_TBL, i, &Entry);
		if(Entry.enable){
			sscanf(Entry.startTime, "%hhu:%hhu", &startHour, &startMinute);
			snprintf(tmpbuf, 100, "%hhu %hhu */%hhu * * /bin/config_wlan 1 %u\n", startMinute, startHour, Entry.controlCycle, Entry.SSIDMask);
			fputs(tmpbuf,fp);
			sscanf(Entry.endTime, "%hhu:%hhu", &endHour, &endMinute);
			snprintf(tmpbuf, 100, "%hhu %hhu */%hhu * * /bin/config_wlan 0 %u\n", endMinute, endHour, Entry.controlCycle, Entry.SSIDMask);
			fputs(tmpbuf,fp);
			if(schedFileExist == 0)
				schedFileExist = 1;
		}
	}
#endif

#ifdef LED_TIMER
	if(startup){
		if(getLedStatus(&led_status)==0)
			setLedStatus(led_status);
	}
	current_state = 1;
	totalnum = mib_chain_total(MIB_LED_INDICATOR_TIMER_TBL);
	for(i=0; i<totalnum; i++)
	{
		mib_chain_get(MIB_LED_INDICATOR_TIMER_TBL, i, &ledEntry);
		if(ledEntry.enable)
		{
			current_state = _crond_check_current_state(ledEntry.startHour, ledEntry.startMin, ledEntry.endHour, ledEntry.endMin);
			snprintf(tmpbuf, 100, "%hhu %hhu */%hhu * * /bin/config_led 1\n", ledEntry.startMin, ledEntry.startHour, ledEntry.ctlCycle);
			fputs(tmpbuf,fp);
			snprintf(tmpbuf, 100, "%hhu %hhu */%hhu * * /bin/config_led 0\n", ledEntry.endMin, ledEntry.endHour, ledEntry.ctlCycle);
			fputs(tmpbuf,fp);
			if(schedFileExist == 0)
				schedFileExist = 1;
			if(led_timer_enable == 0)
				led_timer_enable = 1;
		}
	}
	if(led_timer_enable){
		if(current_state == 1)
		{
			system("/bin/config_led 1");
		}
		else
		{
			system("/bin/config_led 0");
		}
	}
#endif

#ifdef SLEEP_TIMER
	totalnum = mib_chain_total(MIB_SLEEP_MODE_SCHED_TBL);
	for(i=0; i<totalnum; i++)
	{
		mib_chain_get(MIB_SLEEP_MODE_SCHED_TBL, i, &sleepEntry);
		if(1 == sleepEntry.day)
		{
			if(startup == 0 /*|| (startup==1 && sleepEntry.onoff==1) */)
			{
				//action rightnow!
				snprintf(tmpbuf, 100, "/bin/config_sleepmode %d", sleepEntry.onoff);
				system(tmpbuf);
			}
		}
		else if(sleepEntry.enable)
		{
			{
				first = 1;
				if(sleepEntry.day & (1<<7)){
					if(first){
						snprintf(day_string, 20, "0");
						first = 0;
					}
					else
						snprintf(day_string, 20,"%s,0", day_string);
				}
				for(j=1;j<7;j++){
					if(sleepEntry.day & (1<<j)){
						if(first){
							snprintf(day_string, 20, "%d", j);
							first = 0;
						}
						else
							snprintf(day_string, 20,"%s,%d", day_string, j);
					}
				}
				//printf("[%s %d]day_string=%s\n", __func__, __LINE__, day_string);
				snprintf(tmpbuf, 100, "%hhu %hhu * * %s /bin/config_sleepmode %d\n", sleepEntry.minute, sleepEntry.hour, day_string, sleepEntry.onoff);
				//printf("[%s %d]tmpbuf=%s\n", __func__, __LINE__, tmpbuf);
				fputs(tmpbuf,fp);

				if(schedFileExist == 0)
					schedFileExist = 1;
			}
		}
	}
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	int wlanIndex;
#ifdef WLAN_CLIENT
	mib_get_s(MIB_WLAN_SITE_SURVEY_TIME, (void *)&site_survey_time, sizeof(site_survey_time));
	if(site_survey_time)
	{
		snprintf(tmpbuf, 100, "*/%d * * * * /bin/SiteSurveyWLANNeighbor 0\n", site_survey_time);
		fputs(tmpbuf,fp);
#ifdef WLAN_DUALBAND_CONCURRENT
		snprintf(tmpbuf, 100, "*/%d * * * * /bin/SiteSurveyWLANNeighbor 1\n", site_survey_time);
		fputs(tmpbuf,fp);
#endif
		if(schedFileExist == 0)
			schedFileExist = 1;
	}
#endif
#ifdef CONFIG_WLAN
	for(i=0 ; i<NUM_WLAN_INTERFACE ; i++)
	{
		wlanIndex=i;
		mib_local_mapping_get(MIB_WLAN_GUEST_SSID, wlanIndex, (void *)&guest_ssid);
		mib_local_mapping_get(MIB_WLAN_GUEST_SSID_DURATION, wlanIndex, (void *)&guest_ssid_duration);
		mib_local_mapping_get(MIB_WLAN_GUEST_SSID_ENDTIME, wlanIndex, (void *)&guest_ssid_endtime);
		if(guest_ssid && guest_ssid_duration && guest_ssid_endtime) {
			char ifname[16], timeStrBuf[200];
			struct tm tm_time;
			time_t tm;
			tm = guest_ssid_endtime;
			memcpy(&tm_time, localtime(&tm), sizeof(tm_time));
			strftime(timeStrBuf, 200, "%M %H %d %m %w", &tm_time);
#ifdef CONFIG_CU_BASEON_YUEME
			if(startup)
				continue;
			if(guest_ssid_duration != 0){
				unsigned int mask = 0;
				if(wlanIndex == 0)
					mask = (1 << guest_ssid);
				else
					mask = (1 << (SSID_INTERFACE_SHIFT_BIT + guest_ssid));
				snprintf(tmpbuf, 100, "%s sleep %d;/bin/config_wlan 0 %d\n",timeStrBuf, tm_time.tm_sec,mask);
			}
#else
			get_ifname_by_ssid_index(guest_ssid, ifname);
			if(wlanIndex==0){
				snprintf(tmpbuf, 100, "%s sleep %d;/etc/scripts/wlan_off.sh %s %d %d\n", timeStrBuf, tm_time.tm_sec, ifname, wlanIndex, guest_ssid-1);
			}
			else{
				snprintf(tmpbuf, 100, "%s sleep %d;/etc/scripts/wlan_off.sh %s %d %d\n", timeStrBuf, tm_time.tm_sec, ifname, wlanIndex, guest_ssid-5);
			}
#endif

			fputs(tmpbuf,fp);
			if(schedFileExist == 0)
				schedFileExist = 1;
		}
	}
#endif
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_
	unsigned char scWlanScan = 0;
	mib_get_s(PROVINCE_SICHUAN_WLAN_SURVEY_TIME, &scWlanScan, sizeof(scWlanScan));
	if(scWlanScan)
	{
		snprintf(tmpbuf, 100, "*/%d * * * * /bin/ShowWirelessChannel all\n", scWlanScan);
		fputs(tmpbuf,fp);
		if(schedFileExist == 0)
		{
			schedFileExist = 1;
		}
	}
#endif

#ifdef CONFIG_USER_LANNETINFO_COLLECT_OFFLINE_DEVINFO
	snprintf(tmpbuf, 100, "1 */%d * * * /bin/cp %s %s\n", 1, OFFLINELANNETINFOFILE, OFFLINELANNETINFOFILE_SAVE);
	fputs(tmpbuf,fp);

	if(schedFileExist == 0)
		schedFileExist = 1;
#endif

#ifdef SUPPORT_TIME_SCHEDULE
	int totalSched=0, found=0, end_hour=0, end_min=0, carry_week=0;
	char week_str[32]={0}, tmp_str[8];
	MIB_CE_TIME_SCHEDULE_T schdEntry = {0};
	totalSched = mib_chain_total(MIB_TIME_SCHEDULE_TBL);

	for (i = 0; i < totalSched; i++)
	{
		if (!mib_chain_get(MIB_TIME_SCHEDULE_TBL, i, (void *)&schdEntry))
		{
			continue;
		}
		memset(week_str, 0, sizeof(week_str));
		found = 0;
		//Handle the end time
		end_min = schdEntry.sch_min[3] + 1;
		end_hour = schdEntry.sch_min[2];
		if (end_min >= 60)
		{
			end_hour +=1 ;
			end_min%=60;
			if( end_hour >=24 )
			{
				end_hour%= 24;
				carry_week=1;
			}
		}

		for (j=0; j< 7 ;j++)
		{
			if(((unsigned int *)schdEntry.sch_time)[j])
			{
				if(carry_week==0)
					snprintf(tmp_str, 8, "%d,", j);
				else
					snprintf(tmp_str, 8, "%d,", (j+1)%7);
				strcat(week_str, tmp_str);
				found++;
			}
		}
		week_str[strlen(week_str)-1] ='\0';

		if(found == 7)
			snprintf(week_str, sizeof(week_str), "*");
		if (found)
		{
			//start time
			snprintf(tmpbuf, 100, "%d %d * * %s /bin/schedule_time_update\n", schdEntry.sch_min[1], schdEntry.sch_min[0], week_str);
			fputs(tmpbuf,fp);
			//end time
			snprintf(tmpbuf, 100, "%d %d * * %s /bin/schedule_time_update\n", end_min, end_hour, week_str);
			fputs(tmpbuf,fp);
		}
	}
#endif
#endif // CONFIG_E8B

#if defined(BB_FEATURE_SAVE_LOG)  ||  defined(CONFIG_USER_RTK_SYSLOG)
#ifndef USE_DEONET
	unsigned int interval=0, hour=0;
	if ( !mib_get_s(MIB_SYSLOG_FILE_SYNC_INTERVAL, (void *)&interval, sizeof(interval)) ) {
		printf("ERROR: Get superuser name from MIB database failed.\n");
	}else{
		if ( interval <= 0 )
			interval = 60;

		if ( interval >= 1440 ) //force to save syslog per days
		{
			snprintf(tmpbuf, 100, "0 0 * * * /etc/scripts/slogd_sync.sh %s %s\n",  SYSLOGDFILE, SYSLOGDFILE_SAVE);
		}
		else if ( interval >= 60 ) //hours ,* */%d * * *  didn't work in busybox cron
		{
			hour = interval/60;
			snprintf(tmpbuf, 100, "1 */%d * * * /etc/scripts/slogd_sync.sh %s %s\n", hour, SYSLOGDFILE, SYSLOGDFILE_SAVE);
		}
		else
			snprintf(tmpbuf, 100, "*/%d * * * * /etc/scripts/slogd_sync.sh %s %s\n", interval, SYSLOGDFILE, SYSLOGDFILE_SAVE);
		fputs(tmpbuf,fp);
		if(schedFileExist == 0)
		{
			schedFileExist = 1;
		}
	}
#endif
#endif

    if (fp != NULL)
   {
	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(filename,0666);
#endif
    }
	if(schedFileExist) {
		// file is not empty
		va_niced_cmd("/bin/crond", 0, 1);
	}
	else
		unlink(filename);
}

unsigned char is_interface_valid(char *interface)
{
	int          skfd = 0;
	struct ifreq ifr;
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (skfd < 0) {
		return 0;
	}
	strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
	if (ioctl(skfd, SIOCGIFINDEX, &ifr) == -1) {
		close(skfd);
		return 0;
	}
	close(skfd);
	return 1;
}

#ifdef DELAY_WRITE_SYSLOG_TO_FLASH
int force_save_syslog(void)
{
	int mypid;

	mypid = read_pid("/var/run/syslogd.pid");
	if (mypid > 0) {
		kill(mypid, SIGUSR1);
	}
}
#endif

/*
function:get_column_content
get Column content of one line
parameters:
content: 	line content;
value:		column value to put;
value_maxlen: vlue max length;
column: 	column want to getï¼?if set -1, return cnt of columns
*/
unsigned int get_column_content
(
    char *content, char *value,
    int value_maxlen,
    int column
){
	int i = 0;
	unsigned int col_cur = 1;
	int length = 0;
	int start = 0;
	int find_blank = 0;

	while(content[i]){
		//ignore blank
		while(content[i] == ' '){
			find_blank = 1;
			i++;
		}

		if(find_blank == 1){
			col_cur++;
			find_blank = 0;
		}

		if(col_cur == column){
			start = i;
			//printf("start is %d\n", start);
			//get length
			while((content[i] != ' ') && (content[i] != '\0') && (content[i] != '\n')){
				length++;
				i++;
			}
			//printf("length is %d\n", length);

			length = (length > value_maxlen)?value_maxlen:length;
			memcpy(value, content+start, length);
			break;
		}
		i++;

		if(i > 10000)
			break;
	}

	return col_cur;
}

unsigned int if_nametoindex(const char *ifname)
{
	int index;
	int ctl_sock;
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;
	index = 0;
	if ((ctl_sock = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
		if (ioctl(ctl_sock, SIOCGIFINDEX, &ifr) >= 0) {
			index = ifr.ifr_ifindex;
		}
		close(ctl_sock);
	}
	return index;
}

#ifdef CONFIG_CU_BASEON_YUEME
int CheckPoolExist()
{
	int i, entryNum;
	int ret = 0;
	DHCPS_SERVING_POOL_T dhcppoolentry;
	entryNum = mib_chain_total(MIB_DHCPS_SERVING_POOL_TBL);
	for( i = 0; i < entryNum; i++)
	{
		if(!mib_chain_get(MIB_DHCPS_SERVING_POOL_TBL, i, (void *)&dhcppoolentry))
			continue;
		if(dhcppoolentry.poolname)
		{
			if(!strcmp(dhcppoolentry.poolname, "Computer"))
			{
				ret |= PC_POOL_EXIST;
			}
			else if(!strcmp(dhcppoolentry.poolname, "Camera"))
			{
				ret |= CAMERA_POOL_EXIST;
			}
			else if(!strcmp(dhcppoolentry.poolname, "STB"))
			{
				ret |= STB_POOL_EXIST;
			}
#ifdef CONFIG_CU
			else if(!strcmp(dhcppoolentry.poolname, "MVT"))
#else
			else if(!strcmp(dhcppoolentry.poolname, "Phone"))
#endif
			{
				ret |= PHONE_POOL_EXIST;
			}
			else if(!strcmp(dhcppoolentry.poolname, "WlanAP"))
			{
				ret |= WLANAP_POOL_EXIST;
			}

		}
	}

	return ret;
}
#endif
#if defined(CONFIG_CMCC)
static int isOtherWanRouter()
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
			printf("isOtherWanBridge: cannot get ATM_VC_TBL(ch=%d) entry\n", i);
			continue;
		}
		if(Entry.applicationtype==X_CT_SRV_OTHER && Entry.cmode!=CHANNEL_MODE_BRIDGE)
		{
			return 1;
		}
	}
	return 0;
}

static int read_stb_white_list(STB_IGMP_WHITE_LIST_Tp pList)
{
	FILE*fp=NULL;
	int cnt=0;
	int rc;

	if(!pList)
		return -1;

	if (access(STB_IGMP_WHITE_LIST_FILE, F_OK)== -1)
		return cnt;

	fp=fopen(STB_IGMP_WHITE_LIST_FILE,"r");
	if(!fp)
	{
		printf("[%s %d]file %s open failed!\n", __FUNCTION__, __LINE__, STB_IGMP_WHITE_LIST_FILE);
		return -1;
	}

	while (!feof(fp)) {
		rc = fscanf(fp, "%s\t%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX\t%ld\n",
		pList[cnt].ifname, &(pList[cnt].mac[0]), &(pList[cnt].mac[1]), &(pList[cnt].mac[2]), &(pList[cnt].mac[3]), &(pList[cnt].mac[4]), &(pList[cnt].mac[5]),
		&(pList[cnt].uptime));
		if (8 == rc && EOF != rc) {
			cnt++;
		}
	}
	fclose(fp);
	return cnt;
}

static int write_stb_white_list(STB_IGMP_WHITE_LIST_Tp pList, int cnt)
{
	FILE*fp=NULL;
	int i;

	if(!pList)
		return -1;

	if(cnt>MAX_STB_IGMP_WHITE_LIST_NUM)
	{
		printf("[%s %d]too much stb entry!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if(cnt==0)
	{
		unlink(STB_IGMP_WHITE_LIST_FILE);
		return 0;
	}

	fp=fopen(STB_IGMP_WHITE_LIST_FILE,"w");
	if(!fp)
	{
		printf("[%s %d]file %s open failed! Reason: %s\n", __FUNCTION__, __LINE__, STB_IGMP_WHITE_LIST_FILE, strerror(errno));
		return -1;
	}

	for(i=0; i<cnt; i++)
	{
		if(fprintf(fp, "%s\t%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX\t%ld\n",
			pList[i].ifname, pList[i].mac[0], pList[i].mac[1], pList[i].mac[2], pList[i].mac[3], pList[i].mac[4], pList[i].mac[5],
			pList[i].uptime)<0)
		{
			printf("[%s %d]sprintf failed!\n", __FUNCTION__, __LINE__);
			fclose(fp);
			return -1;
		}
	}
	fclose(fp);
	return 0;
}

int stb_white_list_find_by_IP(unsigned int ip)
{
	STB_IGMP_WHITE_LIST_T list[MAX_STB_IGMP_WHITE_LIST_NUM];
	int i=0,lockfd=-1,ret=0,cnt=0;
	char province[32]={0};
	unsigned char mac[MAC_ADDR_LEN];

	mib_get_s(MIB_HW_PROVINCE_NAME, province, sizeof(province));
	if(strncmp(province, "ZJ", 2))
		return 1; //If not ZJ province, always in white list
	if(get_mac_by_ipaddr((struct in_addr *)(&ip), mac)==-1)
	{
		printf("%s %d stb mac not found in arp!\n", __FUNCTION__, __LINE__);
		return ret;
	}
	if((lockfd = lock_file_by_flock(STB_IGMP_WHITE_LIST_FILE_LOCK, 1)) == -1)
	{
		printf("[%s %d]lock file %s fail!\n", __FUNCTION__, __LINE__, STB_IGMP_WHITE_LIST_FILE_LOCK);
		ret=-1;
		goto RETURN;
	}
	cnt=read_stb_white_list(list);
	for(i=0; i<cnt; i++)
	{
		if(!memcmp(list[i].mac, mac,MAC_ADDR_LEN))
		{
			ret=1;
			break;
		}
	}

RETURN:
	if(lockfd!=-1)
		unlock_file_by_flock(lockfd);
	return ret;
}

int add_stb_white_list(char ifname[IFNAMSIZ], unsigned char mac[MAC_ADDR_LEN])
{
	STB_IGMP_WHITE_LIST_T list[MAX_STB_IGMP_WHITE_LIST_NUM];
	unsigned char initMac[MAC_ADDR_LEN]=DEFAULT_INIT_STB_WHITE_LIST_MAC;
	struct sysinfo info;
	char cmd[128]={};
	int i=0,lockfd=-1,ret=0,line=0,oldestIdx=0,existIdx=0,cnt=0;
	long oldestTime=0;

	memset(list,0,sizeof(STB_IGMP_WHITE_LIST_T)*MAX_STB_IGMP_WHITE_LIST_NUM);
	sysinfo(&info);
	oldestTime=info.uptime;
	if((lockfd = lock_file_by_flock(STB_IGMP_WHITE_LIST_FILE_LOCK, 1)) == -1)
	{
		printf("[%s %d]lock file %s fail!\n", __FUNCTION__, __LINE__, STB_IGMP_WHITE_LIST_FILE_LOCK);
		ret=-1;
		goto RETURN;
	}

	if(!isOtherWanRouter())
	{
		//printf("%s %d, other wan is bridge or not exist\n", __FUNCTION__, __LINE__);
		ret=-1;
		goto RETURN;
	}

	cnt=read_stb_white_list(list);
	if(cnt<0)
	{
		ret=-1;
		goto RETURN;
	}

	for(i=1; i<cnt; i++)
	{
		if(list[i].uptime<oldestTime)
		{
			oldestTime=list[i].uptime;
			oldestIdx=i;
		}
		if(!memcmp(list[i].mac, mac, MAC_ADDR_LEN) && !strcmp(ifname, list[i].ifname))
			existIdx=i;
	}

	if(existIdx)
	{
		list[existIdx].uptime=info.uptime;
		write_stb_white_list(list,cnt);
	}
	else
	{
		if(cnt==MAX_STB_IGMP_WHITE_LIST_NUM)
		{
			//need recycle oldest one
			snprintf(cmd, 128, "echo DELBY_PATTEN 1 DEVIFNAME %s SMAC %02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX > %s",
				list[oldestIdx].ifname, list[oldestIdx].mac[0], list[oldestIdx].mac[1], list[oldestIdx].mac[2],
				list[oldestIdx].mac[3], list[oldestIdx].mac[4], list[oldestIdx].mac[5], IGMP_WHITE_LIST_CTRL_FILE);
			printf("[%s %d]cmd=%s\n", __FUNCTION__, __LINE__, cmd);
			system(cmd);

			memcpy(list[oldestIdx].mac, mac, MAC_ADDR_LEN);
			list[oldestIdx].uptime=info.uptime;
			strcpy(list[oldestIdx].ifname, ifname);
			snprintf(cmd, 128, "echo DEVIFNAME %s SMAC %02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX > %s",
				ifname, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], IGMP_WHITE_LIST_CTRL_FILE);
			printf("[%s %d]cmd=%s\n", __FUNCTION__, __LINE__, cmd);
			system(cmd);
			write_stb_white_list(list,cnt);
		}
		else
		{
			memset(&list[cnt], 0, sizeof(STB_IGMP_WHITE_LIST_T));
			memcpy(list[cnt].mac, mac,MAC_ADDR_LEN);
			list[cnt].uptime=info.uptime;
			strcpy(list[cnt].ifname, ifname);
			snprintf(cmd, 128, "echo DEVIFNAME %s SMAC %02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX > %s",
				ifname, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], IGMP_WHITE_LIST_CTRL_FILE);
			printf("[%s %d]cmd=%s\n", __FUNCTION__, __LINE__, cmd);
			system(cmd);
			write_stb_white_list(list,cnt+1);
		}
	}

RETURN:
	if(lockfd!=-1)
		unlock_file_by_flock(lockfd);
	return ret;
}

int del_stb_white_list(unsigned char mac[MAC_ADDR_LEN])
{
	STB_IGMP_WHITE_LIST_T list[MAX_STB_IGMP_WHITE_LIST_NUM];
	STB_IGMP_WHITE_LIST_T listSorted[MAX_STB_IGMP_WHITE_LIST_NUM];
	struct sysinfo info;
	char cmd[128]={};
	int i=0,j=0,lockfd=-1,ret=0,cnt=0;
	unsigned char initMac[MAC_ADDR_LEN]=DEFAULT_INIT_STB_WHITE_LIST_MAC;

	if((lockfd = lock_file_by_flock(STB_IGMP_WHITE_LIST_FILE_LOCK, 1)) == -1)
	{
		printf("[%s %d]lock file %s fail!\n", __FUNCTION__, __LINE__, STB_IGMP_WHITE_LIST_FILE_LOCK);
		ret=-1;
		goto RETURN;
	}

	if(!isOtherWanRouter())
	{
		//printf("%s %d, other wan is bridge or not exist\n", __FUNCTION__, __LINE__);
		ret=-1;
		goto RETURN;
	}

	memset(list,0,sizeof(STB_IGMP_WHITE_LIST_T)*MAX_STB_IGMP_WHITE_LIST_NUM);
	memset(listSorted, 0, sizeof(STB_IGMP_WHITE_LIST_T)*MAX_STB_IGMP_WHITE_LIST_NUM);
	cnt=read_stb_white_list(list);
	for(i=1; i<cnt; i++)
	{
		if(!memcmp(list[i].mac, mac, MAC_ADDR_LEN))
		{
			snprintf(cmd, 128, "echo DELBY_PATTEN 1 DEVIFNAME %s SMAC %02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX > %s",
				list[i].ifname, list[i].mac[0], list[i].mac[1], list[i].mac[2],
				list[i].mac[3], list[i].mac[4], list[i].mac[5], IGMP_WHITE_LIST_CTRL_FILE);
			printf("[%s %d]cmd=%s\n", __FUNCTION__, __LINE__, cmd);
			system(cmd);

			list[i].uptime=0;
		}
	}

	for(i=0; i<cnt; i++)
	{
		if(list[i].uptime==0)
			continue;
		memcpy(listSorted+j, list+i, sizeof(STB_IGMP_WHITE_LIST_T));
		j++;
	}

	write_stb_white_list(listSorted,j);
RETURN:
	if(lockfd!=-1)
		unlock_file_by_flock(lockfd);
	return ret;
}

int init_stb_igmp_white_list()
{
	int i=0,lockfd=-1,ret=0,cnt=0;
	char cmd[128]={};
	unsigned char initMac[MAC_ADDR_LEN]=DEFAULT_INIT_STB_WHITE_LIST_MAC;
	struct sysinfo info;
	STB_IGMP_WHITE_LIST_T list[MAX_STB_IGMP_WHITE_LIST_NUM];

	memset(list,0,sizeof(STB_IGMP_WHITE_LIST_T)*MAX_STB_IGMP_WHITE_LIST_NUM);
	sysinfo(&info);

	if((lockfd = lock_file_by_flock(STB_IGMP_WHITE_LIST_FILE_LOCK, 1)) == -1)
	{
		printf("[%s %d]lock file %s fail!\n", __FUNCTION__, __LINE__, STB_IGMP_WHITE_LIST_FILE_LOCK);
		ret=-1;
		goto RETURN;
	}

	for(i=0;i<SW_LAN_PORT_NUM;i++)
	{
		snprintf(cmd, 128, "echo DEVIFNAME %s SMAC %02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX > %s",
			ELANVIF[i], initMac[0], initMac[1], initMac[2], initMac[3], initMac[4], initMac[5], IGMP_WHITE_LIST_CTRL_FILE);
		printf("[%s %d]cmd=%s\n", __FUNCTION__, __LINE__, cmd);
		system(cmd);
	}

	list[0].uptime=info.uptime;
	memcpy(list[0].mac, initMac, MAC_ADDR_LEN);
	strcpy(list[0].ifname, ELANIF);
	write_stb_white_list(list,1);

RETURN:
	if(lockfd!=-1)
		unlock_file_by_flock(lockfd);
	return ret;
}

int flush_stb_igmp_white_list()
{
	int i=0,lockfd=-1,ret=0,cnt=0;
	STB_IGMP_WHITE_LIST_T list[MAX_STB_IGMP_WHITE_LIST_NUM];
	unsigned char initMac[MAC_ADDR_LEN]=DEFAULT_INIT_STB_WHITE_LIST_MAC;
	char cmd[128]={};

	if((lockfd = lock_file_by_flock(STB_IGMP_WHITE_LIST_FILE_LOCK, 1)) == -1)
	{
		printf("[%s %d]lock file %s fail!\n", __FUNCTION__, __LINE__, STB_IGMP_WHITE_LIST_FILE_LOCK);
		ret=-1;
		goto RETURN;
	}
	cnt=read_stb_white_list(list);
	if(cnt>0)
	{
		for(i=0;i<SW_LAN_PORT_NUM;i++)
		{
			snprintf(cmd, 128, "echo DELBY_PATTEN 1 DEVIFNAME %s SMAC %02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX > %s",
				ELANVIF[i], initMac[0], initMac[1], initMac[2], initMac[3], initMac[4], initMac[5], IGMP_WHITE_LIST_CTRL_FILE);
			printf("[%s %d]cmd=%s\n", __FUNCTION__, __LINE__, cmd);
			system(cmd);
		}
		for(i=1; i<cnt; i++)
		{
			snprintf(cmd, 128, "echo DELBY_PATTEN 1 DEVIFNAME %s SMAC %02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX > %s",
				list[i].ifname, list[i].mac[0], list[i].mac[1], list[i].mac[2], list[i].mac[3], list[i].mac[4], list[i].mac[5], IGMP_WHITE_LIST_CTRL_FILE);
			printf("[%s %d]cmd=%s\n", __FUNCTION__, __LINE__, cmd);
			system(cmd);
		}
		unlink(STB_IGMP_WHITE_LIST_FILE);
	}

RETURN:
	if(lockfd!=-1)
		unlock_file_by_flock(lockfd);
	return ret;
}
#endif

//If the DUT is on a call, getDialStatus return 1. else return 0.
#define VOIP_TRAP "/proc/voip/trap"
int getDialStatus()
{
	char line[256];
	char *pline;
	int linenum = 0;
	int ret = 0;
	FILE *fd = fopen(VOIP_TRAP, "r");
	if(fd==NULL)
		return 0;
	while(fgets(line, sizeof(line), fd) != NULL)
	{
		pline = line;
		linenum++;
		//printf("linenum=%d  info is %s\n",linenum, line);
		if(linenum >=5){
			ret = 1;
			break;
		}
	}
	fclose(fd);
	return ret;
}

#ifdef CONFIG_IPV6
/**************************************************************************************************************************
 * Function:compare_prefixip_with_unspecip_by_bit
***************************************************************************************************************************/

int compare_prefixip_with_unspecip_by_bit(unsigned char *prefixIP, unsigned char *unspecIP, unsigned char prefixLen)
{
	int k,m,i,pre=0,len=(int)prefixLen;
	unsigned char mask, tmask;
	unsigned char compareIPP, compareIPS;

	if (len > 0 && len < 128){
		k = len/8;
		m = len%8;
		mask = (unsigned char)0;
		tmask = (unsigned char)0x80;
		for (i=0; i<m; i++) {
			mask |= tmask;
			tmask = tmask>>1;
		}
		compareIPP = *(prefixIP+k);
		compareIPS = *(unspecIP+k);
		compareIPP &= mask;
		compareIPS &= mask;

		pre = memcmp(prefixIP,unspecIP,(unsigned int)k);
		return pre==0?compareIPP-compareIPS:pre;
	}else if (len >= 128){
		return memcmp(prefixIP,unspecIP,(unsigned int)16);
	}
	return pre;
}
#endif

void get_ipv4_from_interface(char *buf, int type)
{
	FILE *fp = NULL;
	char cmd[128] = {0};
	char line[IFNAMSIZ] = {0};

	if (!type)
		strcpy(cmd, "ip -4 route show | grep -i 'default via' | awk '{print $3}'");
	else if (type == 1)
		strcpy(cmd, "ifconfig br0 | grep 'inet addr:' |awk '{print $2}' | tr -d addr:");
	else if (type == 2)
		strcpy(cmd, "ip -4 route show | grep -i 'nas0_0  proto kernel' | awk '{print $9}'");
	else if (type == 3)
		strcpy(cmd, "ifconfig br0:1 | grep 'inet addr:' |awk '{print $2}' | tr -d addr:");

	// printf("cmd: [%s] \n", cmd);

	if ((fp = popen(cmd, "r")) != NULL) {
		while (fgets(line, sizeof(line), fp)) {
			line[strlen(line) - 1] = '\0';
			snprintf(buf, IFNAMSIZ, "%s", line);
		}

		pclose(fp);
	}
}

void get_ipv4_from_dropbear(char *buf)
{
	FILE *fp = NULL;
	char cmd[128] = {0};
	char line[IFNAMSIZ] = {0};

	strcpy(cmd, "ps -ef | grep dropbear | awk '{print $11}' | awk '{print substr($0, 1, length($0)-3)}' | sed '/^$/d'");

	if ((fp = popen(cmd, "r")) != NULL) {
		while (fgets(line, sizeof(line), fp)) {
			line[strlen(line) - 1] = '\0';
			snprintf(buf, IFNAMSIZ, "%s", line);
		}
		pclose(fp);
	}
}

void get_mac_from_interface(char *name, char *buf)
{
	FILE *fp = NULL;
	char cmd[128] = {0};
	char line[18] = {0};

	snprintf(cmd, sizeof(cmd), "ifconfig %s | grep HWaddr | awk '{print $5}' | sed 's/://;s/://;s/://;s/://;s/://;'", name);

	if ((fp = popen(cmd, "r")) != NULL) {
		while (fgets(line, sizeof(line), fp)) {
			line[strlen(line) - 1] = '\0';
			snprintf(buf, 14, "%s", line); // 00E04C3333344
		}
		pclose(fp);
	}
}

#ifdef USE_DEONET /* sghong, 20221124 */
int deo_port_autonego_fc_set(int port, int fc)
{
	int ret = 0;
	rt_port_phy_ability_t ability;

	rt_port_phyAutoNegoAbility_get(port, &ability);

	/* 설정 값에 따라, ENABLED / DISABLED 처리 */
	if (fc == FC_OFF)
	{
		ability.AsyFC = DISABLED;
		ability.FC = DISABLED;
	}
	else if (fc == FC_SYMM)
	{
		ability.AsyFC = DISABLED;
		ability.FC = ENABLED;
	}
	else if (fc == FC_ASYMM)
	{
		ability.AsyFC = ENABLED;
		ability.FC = DISABLED;
	}
	else if (fc == FC_BOTH)
	{
		ability.AsyFC = ENABLED;
		ability.FC = ENABLED;
	}

	ret = rt_port_phyAutoNegoAbility_set(port, &ability);
	if (ret != RT_ERR_OK)
		return -1;

	return 0;
}

int deo_ipv6_status_get(void)
{
	unsigned char ipv6mode = 0;

	if(!mib_get_s(MIB_V6_IPV6_ENABLE, (void *)&ipv6mode, sizeof(ipv6mode)))
		fprintf(stderr, "Get mib value MIB_V6_IPV6_ENABLE failed!\n");

	return ipv6mode;
}

void deo_ipv6_status_set(unsigned char mode)
{
	if(mib_set(MIB_V6_IPV6_ENABLE, (void *)&mode) == 0)
		fprintf(stderr, "Set mib value MIB_V6_IPV6_ENABLE failed!\n");
}

int deo_wan_nat_mode_get(void)
{
	unsigned int wanmode = 0;

	if (g_wan_mode == 0)
	{
		if(!mib_get_s(MIB_WAN_NAT_MODE, (void *)&wanmode, sizeof(wanmode)))
			fprintf(stderr, "Get mib value MIB_WAN_NAT_MODE failed!\n");
		g_wan_mode = wanmode;
	}

	return g_wan_mode;
}

void deo_wan_nat_mode_set(int wanmode)
{
	unsigned char dhcpMode = DHCP_LAN_SERVER;
	if (mib_set(MIB_WAN_NAT_MODE, (void *)&wanmode) == 0)
		printf("Set WAN_NAT_MODE MIB error!\n");

	if(wanmode == DEO_BRIDGE_MODE)
		dhcpMode = DHCP_LAN_NONE;
	else
		dhcpMode = DHCP_LAN_SERVER;

	if (mib_set(MIB_DHCP_MODE, (void *)&dhcpMode) == 0)
		printf("Set DHCP_MODE MIB error!\n");
}

int deo_wan_intf_enable_set_by_index(int index, int is_enable)
{
	MIB_CE_ATM_VC_T Entry;

	if (mib_chain_get(MIB_ATM_VC_TBL, index, (void *)&Entry))
	{
		Entry.enable = is_enable;
		mib_chain_update(MIB_ATM_VC_TBL, (void*)&Entry, index);
	}

	return 0;
}

void rt_acl_drop_managemet_ip_rule(int flag)
{
	int32 ret = RT_ERR_FAILED;
	int acl_filter_idx = 0;
	rt_acl_filterAndQos_t rule;

	memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));
	rule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rule.acl_weight = WEIGHT_QOS_HIGH;
	rule.ingress_port_mask = 0x20;
	rule.ingress_dest_ipv4_addr_start = 0xc0a84b01;
	rule.ingress_dest_ipv4_addr_end = 0xc0a84bff;
	rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;

	ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);
	/*
	if (ret == RT_ERR_OK)
		printf("add drop acl entry[%d] success for ipv4 management range drop rule!\n",
				acl_filter_idx);
	else
		printf("add drop acl entry failed for ipv4 management range drop rule!\n");
	*/
}

/* drop udp 136, 137, 138, 445 on bridge pon port ingress */
void multicast_drop_specific_port(int flag)
{
	int32 ret = RT_ERR_FAILED;
	int acl_filter_idx = 0;
	rt_acl_filterAndQos_t rule;

	memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));
	rule.filter_fields |= RT_ACL_INGRESS_L4_UDP_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_L4_SPORT_RANGE_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
	rule.acl_weight = WEIGHT_HIGH;
	rule.ingress_l4_protocal = 0;
	rule.ingress_port_mask = rtk_port_get_wan_phyMask();
	rule.ingress_src_l4_port_start = 0;
	rule.ingress_src_l4_port_end = 0xffff;
	rule.ingress_dest_l4_port_start = 136;
	rule.ingress_dest_l4_port_end = 138;
	rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;

	ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);
	/*
	if (ret == RT_ERR_OK)
		printf("add drop acl entry[%d] success for ipv4 forward rule!\n", acl_filter_idx);
	else
		printf("add drop acl entry failed for ipv4 forward rule!\n");
	*/

	memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));
	rule.filter_fields |= RT_ACL_INGRESS_L4_UDP_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_L4_SPORT_RANGE_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
	rule.acl_weight = WEIGHT_HIGH;
	rule.ingress_l4_protocal = 0x11;
	rule.ingress_port_mask = rtk_port_get_wan_phyMask();
	rule.ingress_src_l4_port_start = 0;
	rule.ingress_src_l4_port_end = 0xffff;
	rule.ingress_dest_l4_port_start = rule.ingress_dest_l4_port_end = 445;
	rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;

	ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);
	/*
	if (ret == RT_ERR_OK)
		printf("add drop acl entry[%d] success for ipv4 forward rule!\n", acl_filter_idx);
	else
		printf("add drop acl entry failed for ipv4 forward rule!\n");
	*/
}

/* forward IAPP(224.0.1.65, 224.0.1.76, 224.0.1.178) on lan to lan */
void multicast_forward_shared_specific_address(int v4, int v6)
{
	int32 ret = RT_ERR_FAILED;
	int acl_filter_idx = 0;
	rt_acl_filterAndQos_t rule;

	memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));
	rule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rule.acl_weight = WEIGHT_HIGH;
	rule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
	rule.ingress_dest_ipv4_addr_start = rule.ingress_dest_ipv4_addr_end = 0xe0000141;
	rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT;

	ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);
	/*
	if (ret == RT_ERR_OK)
		printf("add drop acl entry[%d] success for ipv4 forward rule!\n", acl_filter_idx);
	else
		printf("add drop acl entry failed for ipv4 forward rule!\n");
	*/

	memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));
	rule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rule.acl_weight = WEIGHT_HIGH;
	rule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
	rule.ingress_dest_ipv4_addr_start = rule.ingress_dest_ipv4_addr_end = 0xe000014C;
	rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT;

	ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);
	/*
	if (ret == RT_ERR_OK)
		printf("add drop acl entry[%d] success for ipv4 forward rule!\n", acl_filter_idx);
	else
		printf("add drop acl entry failed for ipv4 forward rule!\n");
	*/

	memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));
	rule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rule.acl_weight = WEIGHT_HIGH;
	rule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
	rule.ingress_dest_ipv4_addr_start = rule.ingress_dest_ipv4_addr_end = 0xe00001b2;
	rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT;

	ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);
	/*
	if (ret == RT_ERR_OK)
		printf("add drop acl entry[%d] success for ipv4 forward rule!\n", acl_filter_idx);
	else
		printf("add drop acl entry failed for ipv4 forward rule!\n");
	*/
}

void multicast_drop_specific_address(int v4, int v6)
{
	int32 ret = RT_ERR_FAILED;
	int acl_filter_idx = 0;
	unsigned char ip6Addr[IP6_ADDR_LEN]={0}, mask[IP6_ADDR_LEN]={0};
	rt_acl_filterAndQos_t rule;

	// drop SKB UDP Multicast: 239.192.0.0/16
	memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));
	rule.filter_fields |= RT_ACL_INGRESS_L4_UDP_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rule.acl_weight = WEIGHT_HIGH;
	rule.ingress_l4_protocal = 0x20;
	rule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
	rule.ingress_dest_ipv4_addr_start = 0xefc00000;
	rule.ingress_dest_ipv4_addr_end = 0xefc0ffff;
	rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;

	ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);
	if (ret == RT_ERR_OK)
		printf("add drop acl entry[%d] success for ipv4 range drop rule!\n",
				acl_filter_idx);
	else
		printf("add drop acl entry failed for ipv4 range drop rule!\n");

	// drop 224.0.0.1
	memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));
	rule.filter_fields |= RT_ACL_INGRESS_ETHERTYPE_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rule.acl_weight = WEIGHT_HIGH;
	rule.ingress_ethertype = 0x0800;
	rule.ingress_ethertype_mask = 0xffff;
	rule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
	rule.ingress_dest_ipv4_addr_start = 0xe0000001;
	rule.ingress_dest_ipv4_addr_end = 0xe0000001;
	rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;

	ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);
	if (ret == RT_ERR_OK)
		printf("add drop acl entry[%d] success for ipv4 range drop rule!\n",
				acl_filter_idx);
	else
		printf("add drop acl entry failed for ipv4 range drop rule!\n");

	// Ipv6 drop rule(SSDP, LNMNR, UPNP, mDNS) on NAT
	if (v4 == 1) {
		// LLMNR
		memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));
		rule.filter_fields |= RT_ACL_INGRESS_IPV6_DIP_BIT;
		rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		rule.filter_fields |= RT_ACL_INGRESS_IPV6_TAGIF_BIT;
		rule.acl_weight = WEIGHT_HIGH;
		rule.ingress_port_mask = 0x20;
		inet_pton(PF_INET6, "ff02::1:3",(void *)ip6Addr);
		memcpy(rule.ingress_dest_ipv6_addr, ip6Addr, IPV6_ADDR_LEN);
		inet_pton(PF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",(void *)mask);
		memcpy(rule.ingress_dest_ipv6_addr_mask, mask, IPV6_ADDR_LEN);
		rule.ingress_ipv6_tagif = 1;
		rule.action_priority_group_trap_priority = 7;

		rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;
		ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);
		/*
		if (ret == RT_ERR_OK)
			printf("add drop acl entry[%d] success for multicast_drop_specific_address(LLMNR)!\n",
					acl_filter_idx);
		else
			printf("add drop acl entry failed for multicast_drop_specific_address(LLMNR)!\n");
		*/

		// SSDP
		memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));
		memset(ip6Addr, 0, sizeof(ip6Addr));
		memset(mask, 0, sizeof(mask));
		rule.filter_fields |= RT_ACL_INGRESS_IPV6_DIP_BIT;
		rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		rule.filter_fields |= RT_ACL_INGRESS_IPV6_TAGIF_BIT;
		rule.acl_weight = WEIGHT_HIGH;
		rule.ingress_port_mask = 0x20;
		inet_pton(PF_INET6, "ff02::c",(void *)ip6Addr);
		memcpy(rule.ingress_dest_ipv6_addr, ip6Addr, IPV6_ADDR_LEN);
		inet_pton(PF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",(void *)mask);
		memcpy(rule.ingress_dest_ipv6_addr_mask, mask, IPV6_ADDR_LEN);
		rule.ingress_ipv6_tagif = 1;
		rule.action_priority_group_trap_priority = 7;
		rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;

		ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);
		/*
		if (ret == RT_ERR_OK)
			printf("add drop acl entry[%d] success for multicast_drop_specific_address(SSDP)!\n",
					acl_filter_idx);
		else
			printf("add drop acl entry failed for multicast_drop_specific_address(SSDP)!\n");
		*/

		// mDNS
		memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));
		memset(ip6Addr, 0, sizeof(ip6Addr));
		memset(mask, 0, sizeof(mask));
		rule.filter_fields |= RT_ACL_INGRESS_IPV6_DIP_BIT;
		rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		rule.filter_fields |= RT_ACL_INGRESS_IPV6_TAGIF_BIT;
		rule.acl_weight = WEIGHT_HIGH;
		rule.ingress_port_mask = 0x20;
		inet_pton(PF_INET6, "ff01::fb",(void *)ip6Addr);
		memcpy(rule.ingress_dest_ipv6_addr, ip6Addr, IPV6_ADDR_LEN);
		inet_pton(PF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",(void *)mask);
		memcpy(rule.ingress_dest_ipv6_addr_mask, mask, IPV6_ADDR_LEN);
		rule.ingress_ipv6_tagif = 1;
		rule.action_priority_group_trap_priority = 7;
		rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;

		ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);
		/*
		if (ret == RT_ERR_OK)
			printf("add drop acl entry[%d] success for multicast_drop_specific_address(mDNS)!\n",
					acl_filter_idx);
		else
			printf("add drop acl entry failed for multicast_drop_specific_address(mDNS)!\n");
		*/

		// UPNP
		memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));
		memset(ip6Addr, 0, sizeof(ip6Addr));
		memset(mask, 0, sizeof(mask));
		rule.acl_weight = WEIGHT_HIGH;
		rule.filter_fields |= RT_ACL_INGRESS_IPV6_DIP_BIT;
		rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		rule.filter_fields |= RT_ACL_INGRESS_IPV6_TAGIF_BIT;
		rule.ingress_port_mask = 0x20;
		inet_pton(PF_INET6, "ff02::f",(void *)ip6Addr);
		memcpy(rule.ingress_dest_ipv6_addr, ip6Addr, IPV6_ADDR_LEN);
		inet_pton(PF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",(void *)mask);
		memcpy(rule.ingress_dest_ipv6_addr_mask, mask, IPV6_ADDR_LEN);
		rule.ingress_ipv6_tagif = 1;
		rule.action_priority_group_trap_priority = 7;
		rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;

		ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);
		/*
		if (ret == RT_ERR_OK)
			printf("add drop acl entry[%d] success for multicast_drop_specific_address(UPNP)!\n",
					acl_filter_idx);
		else
			printf("add drop acl entry failed for multicast_drop_specific_address(UPNP)!\n");
		*/
	}
}

// for mesh config packet
void multicast_trap_IEEE_1905_acl()
{
	int32 ret = RT_ERR_FAILED;
	int acl_filter_idx = 0;
	rt_acl_filterAndQos_t rule;

	memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));
	rule.filter_fields |= RT_ACL_INGRESS_ETHERTYPE_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_DMAC_BIT;
	rule.ingress_dmac_mask[0] = 0xff;
	rule.ingress_dmac_mask[1] = 0xff;
	rule.ingress_dmac_mask[2] = 0xff;
	rule.ingress_dmac_mask[3] = 0xff;
	rule.ingress_dmac_mask[4] = 0xff;
	rule.ingress_dmac_mask[5] = 0xff;
	rule.ingress_dmac[0] = 0x01;
	rule.ingress_dmac[1] = 0x80;
	rule.ingress_dmac[2] = 0xc2;
	rule.ingress_dmac[3] = 0x00;
	rule.ingress_dmac[4] = 0x00;
	rule.ingress_dmac[5] = 0x13;

	rule.ingress_ethertype = 0x893a;
	rule.ingress_ethertype_mask = 0xffff;
	rule.acl_weight = WEIGHT_HIGH;
	rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT; 
	rule.ingress_port_mask = 0xf; 

	ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);

	/*
	if (ret == RT_ERR_OK)
		printf("add drop acl entry[%d] success for multicast_drop_specific_address(UPNP)!\n",
				acl_filter_idx);
	else
		printf("add drop acl entry failed for multicast_drop_specific_address(UPNP)!\n");
	*/
}

int ethertype_acl_set(int dir, uint16 value)
{
	int32 ret = RT_ERR_FAILED;
	int acl_filter_idx = 0;
	rt_acl_filterAndQos_t rule;

	memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));
	rule.filter_fields |= RT_ACL_INGRESS_ETHERTYPE_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rule.acl_weight = WEIGHT_HIGH;
	rule.ingress_ethertype = value; // ethertype value
	rule.ingress_ethertype_mask = 0xffff;
	rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;

	if (dir == DIR_OUT) // lan port
		rule.ingress_port_mask = 0xf;
	else // DIR_IN: wan port
		rule.ingress_port_mask = 0x20;

	ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);
	/*
	if (ret == RT_ERR_OK)
		fprintf(stderr, "add ethertype filter acl entry[%d] for DROP success!\n", acl_filter_idx);
	else
		fprintf(stderr, "add ethertype filter acl entry failed!\n");
	*/

	return acl_filter_idx;
}

int ethertype_acl_del(int acl_idx)
{
	int32 ret = RT_ERR_FAILED;

	ret = rt_acl_filterAndQos_del(acl_idx);
	/*
	if (ret == RT_ERR_OK)
		fprintf(stderr, "delete ethertype filter acl entry[%d] for DROP success!\n", acl_idx);
	else
		fprintf(stderr, "delete ethertype filter acl entry failed!\n");
	*/

	return ret;
}

void ethertype_acl_clear()
{
	int totalEntry, i;
	int idx = 0;
	MIB_CE_IP_PORT_FILTER_T Entry;

	totalEntry = mib_chain_total(MIB_IP_PORT_FILTER_TBL);

	for (i = 0; i < totalEntry; i++) {
		if (!mib_chain_get(MIB_IP_PORT_FILTER_TBL, i, (void *)&Entry))
			break;

		if (Entry.etherType != ETHER_NONE && Entry.etherIndex > 0) {
			idx = Entry.etherIndex;
			ethertype_acl_del(idx);
		}
	}
}

#define  NTP_TIME_STATUS    "/tmp/timeStatus"
int holepunching_mib_get( unsigned char *holecap, char *holeServer,
		unsigned short *holePort, unsigned short *holeInterval)
{
	if (!mib_get(MIB_HOLE_PUNCHING, (void *)holecap))
		return -1;

	if (!mib_get(MIB_HOLE_PUNCHING_SERVER, (void *)holeServer))
		return -1;

	if (!mib_get(MIB_HOLE_PUNCHING_PORT_ENC, (void *)holePort))
		return -1;

	if (!mib_get(MIB_HOLE_PUNCHING_INTERVAL, (void *)holeInterval))
		return -1;
	return 0;
}

int holepunching_mib_set( unsigned char holecap, char *holeServer,
		unsigned short holePort, unsigned short holeInterval)
{
    unsigned char currHolecap, currHoleServer[URL_LEN];
    unsigned short currHoldPort, currHoleInterval;
    int ret;

    holepunching_mib_get(&currHolecap, currHoleServer, &currHoldPort, &currHoleInterval);
    if(ret < 0)
        return -1;

    if(currHolecap == holecap && !strcmp(currHoleServer, holeServer) && currHoldPort == holePort && currHoleInterval == holeInterval)
        return 0;

    if(holecap == 0)
	{
		if ( !mib_set(MIB_HOLE_PUNCHING, (void *)&holecap))
			return -1;
		return 1;
	}

	if ( !mib_set(MIB_HOLE_PUNCHING, (void *)&holecap))
		return -1;

	if (holeServer[0])
	{
		if (!mib_set(MIB_HOLE_PUNCHING_SERVER, (void *)holeServer))
			return -1;
	}
	else
	{
		if (!mib_set(MIB_HOLE_PUNCHING_SERVER, (void *)""))
			return -1;
	}

	if (!mib_set(MIB_HOLE_PUNCHING_PORT_ENC, (void *)&holePort))
		return -1;

	if (!mib_set(MIB_HOLE_PUNCHING_INTERVAL, (void *)&holeInterval))
		return -1;
	return 1;
}

#define UDPHOLE_RUNFILE "/var/run/udphole.pid"
void stopHolepunching(void)
{
#if 0
	int holePid=0;
	int status=0;

	holePid = read_pid((char*)UDPHOLE_RUNFILE);
	if(holePid > 0) {
		kill(holePid, 9);
		unlink(UDPHOLE_RUNFILE);
	}
#endif
}

int startHolepunching(void)
{
#if 0 
	unsigned char holecap;
	char *argv[2];

	if (!mib_get(MIB_HOLE_PUNCHING, (void *)&holecap))
		return;
	if(holecap == 0)
		return;

	argv[1] = NULL;
	do_nice_cmd( "/bin/udphole", argv, 0 );
#else
	FILE *fp;
	char pid_str[128];

	const char *cmd = "/sbin/pidof holepunching_monitor";

	fp = popen(cmd, "r");
	if (fp == NULL) {
		fprintf(stderr, "[ERROR] Failed to run '%s': %s\n", cmd, strerror(errno));
		return -1;
	}

	if (fscanf(fp, "%127s", pid_str) == EOF) {
		fprintf(stderr, "[INFO] holepunching_monitor 프로세스가 없습니다.\n");
		pclose(fp);
		return -1;
	}

	char *token = strtok(pid_str, " ");
	while (token != NULL) {
		pid_t pid = (pid_t)strtol(token, NULL, 10);
		if (pid > 0) {
			if (kill(pid, SIGUSR2) != 0) {
				fprintf(stderr, "[ERROR] Failed to send SIGUSR2 to PID %d: %s\n", pid, strerror(errno));
			} else {
				printf("[INFO] Sent SIGUSR2 to PID %d\n", pid);
			}
		}
		token = strtok(NULL, " ");
	}

	while (fscanf(fp, "%127s", pid_str) != EOF) {
		token = strtok(pid_str, " ");
		while (token != NULL) {
			pid_t pid = (pid_t)strtol(token, NULL, 10);
			if (pid > 0) {
				if (kill(pid, SIGUSR2) != 0) {
					fprintf(stderr, "[ERROR] Failed to send SIGUSR2 to PID %d: %s\n", pid, strerror(errno));
				} else {
					printf("[INFO] Sent SIGUSR2 to PID %d\n", pid);
				}
			}
			token = strtok(NULL, " ");
		}
	}

	pclose(fp);
	return 0;
#endif

}

void deo_wan_intf_enable_set(int wan_mode, unsigned char confIpv6Enbl)
{
    if(wan_mode == DEO_NAT_MODE)
    {
        deo_wan_intf_enable_set_by_index(0, 1); /* nas0_0 */
        if(confIpv6Enbl)
            deo_wan_intf_enable_set_by_index(1, 1); /* nas0_1 */
        else
            deo_wan_intf_enable_set_by_index(1, 0); /* nas0_1 */
    }
    else if(wan_mode == DEO_BRIDGE_MODE)
    {
        deo_wan_intf_enable_set_by_index(0, 0); /* nas0_0 */
        deo_wan_intf_enable_set_by_index(1, 1); /* nas0_1 */

    }
}
int dns_mode_change(int dnsMode,
				unsigned char *dns1, unsigned char *dns2,
                int *need_restartWan)
{
	int total,i;
	MIB_CE_ATM_VC_T *pEntry, vc_entity;
	unsigned int wanmode = 0;
	unsigned char cmode;

	wanmode = deo_wan_nat_mode_get();
	if(wanmode == 0)
	{
		printf("failed to set %s(%d)\n", __func__, __LINE__);
		return -1;
	}

	if(wanmode == 1)
		cmode = CHANNEL_MODE_IPOE;
	else if(wanmode == 2)
		cmode = CHANNEL_MODE_BRIDGE;
	else
	{
		printf("failed to set %s(%d)\n", __func__, __LINE__);
		return -1;
	}

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for( i=0; i<total; i++ )
	{
		pEntry = &vc_entity;
		if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)pEntry ) )
			continue;

		if(pEntry->cmode == cmode)
		{
			if(dnsMode == REQUEST_DNS_NONE)
			{
				if(dns1)
					inet_aton(dns1, (struct in_addr*)&pEntry->v4dns1);
				if(dns2)
					inet_aton(dns2, (struct in_addr*)&pEntry->v4dns2);
			}
			pEntry->dnsMode = dnsMode;
			mib_chain_update( MIB_ATM_VC_TBL, (unsigned char*)pEntry, i );
            *need_restartWan = 1;
			return 1;
		}
	}
	return 0;
}

int ipv6_mode_change(int newIpv6Mode)
{

	unsigned int wanmode = 0;
	unsigned char isIpv6Enable = 0;

	isIpv6Enable = deo_ipv6_status_get();

	if(newIpv6Mode != 0 && newIpv6Mode != 1)
		return -1;

	if(newIpv6Mode == isIpv6Enable)
		return 0;

	deo_ipv6_status_set(newIpv6Mode);
	return 1;
}
/* 1. nat, bridge : WAN_NAT_MODE 1 or 0
   2. nas0_0/nas0_1 enable/disable
   nat,      ipv4        nas0_0 enable, nas0_1 disable
   nat,      ipv4, ipv6  nas0_0 enable, nas0_1 enable
   bridge,   ipv4        nas0_0 disable, nas0_1 enable
   bridge,   ipv4, ipv6  nas0_0 disable, nas0_1 enable
   deo_wan_intf_enable_set_by_index(index, is_enable)
   3. ATM_VC_TBL
   ChannelAddrType, ChannelStatus, DNSMode
   */
int wan_mode_change(int newIpv6Enbl, int newWanmode, int *needReboot)
{

	int total,i;
	MIB_CE_ATM_VC_T *pEntry, vc_entity;
	unsigned int curWanmode = 0;
	unsigned char cmode;
    unsigned char curIpv6Enable = 0;

	if(newIpv6Enbl == -1 && newWanmode == -1)
		return 0;

	curWanmode = deo_wan_nat_mode_get();
    curIpv6Enable = deo_ipv6_status_get();

	if(newIpv6Enbl == -1) newIpv6Enbl = curIpv6Enable;
	if(newWanmode == -1) newWanmode = curWanmode;

    if(curWanmode == newWanmode && curIpv6Enable == newIpv6Enbl)
        return 0;

	if(newWanmode == DEO_NAT_MODE)
		cmode = CHANNEL_MODE_IPOE;
	else if(newWanmode == DEO_BRIDGE_MODE)
		cmode = CHANNEL_MODE_BRIDGE;

	ipv6_mode_change(newIpv6Enbl);

    deo_wan_nat_mode_set(newWanmode);

    deo_wan_intf_enable_set(newWanmode, newIpv6Enbl);

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for( i=0; i<total; i++ )
	{
		pEntry = &vc_entity;
		if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)pEntry ) )
			continue;

		if(pEntry->cmode == cmode)
		{
			pEntry->ipDhcp = 1;
            pEntry->dnsMode = 1;
			if(newWanmode == DEO_NAT_MODE)
			{
				pEntry->IpProtocol = 1;
				pEntry->AddrMode = 0;
				pEntry->Ipv6Dhcp = 0;
				pEntry->Ipv6DhcpRequest = 0;
				pEntry->dnsv6Mode = 0;
			}
			else if(newIpv6Enbl && newWanmode == DEO_BRIDGE_MODE)
			{
				pEntry->IpProtocol = 3;
				pEntry->AddrMode = 32;
				pEntry->Ipv6Dhcp = 1;
				pEntry->Ipv6DhcpRequest = 3;
				pEntry->dnsv6Mode = 1;
			}
			else if(!newIpv6Enbl && newWanmode == DEO_BRIDGE_MODE)
			{
				pEntry->IpProtocol = 1;
				pEntry->AddrMode = 0;
				pEntry->Ipv6Dhcp = 0;
				pEntry->Ipv6DhcpRequest = 0;
				pEntry->dnsv6Mode = 0;
			}

			mib_chain_update( MIB_ATM_VC_TBL, (unsigned char*)pEntry, i );
		}
	}
	*needReboot = 1;
	return 0;
}

static int lan_config_get(unsigned char *isEnable,
        struct in_addr *poolStart, struct in_addr *poolEnd,
        unsigned char *start, unsigned char *end)
{
    if(!mib_get(MIB_ADSL_LAN_DHCP, (void *)isEnable))
        return -1;

    if(!mib_get(MIB_ADSL_LAN_CLIENT_START, (void *)start))
        return -1;

    if(!mib_get(MIB_ADSL_LAN_CLIENT_END, (void *)end))
        return -1;

    if(!mib_get(MIB_DHCP_POOL_START, (void *)poolStart))
        return -1;

    if(!mib_get(MIB_DHCP_POOL_END, (void *)poolEnd))
        return -1;

    return 0;
}

int lan_config_change(unsigned char isEnable,
        struct in_addr poolStart, struct in_addr poolEnd,
        unsigned char start, unsigned char end)
{
    unsigned char currEnable = 0;
    struct in_addr currPoolStart, currPoolEnd;
    unsigned char currStart = 0, currEnd = 0;
    int ret = 0;

    ret = lan_config_get(&currEnable, &currPoolStart, &currPoolEnd,
                    &currStart, &currEnd);

    if(ret < 0)
        return -1;

    if(currEnable == isEnable && currStart == start && currEnd == end
            && currPoolStart.s_addr == poolStart.s_addr
            && currPoolEnd.s_addr == poolEnd.s_addr)
        return 0;

    if(!mib_set(MIB_ADSL_LAN_DHCP, (void *)&isEnable))
        return -1;
    if(isEnable)
    {
        if(start != 0)
        {
            if(!mib_set(MIB_ADSL_LAN_CLIENT_START, (void *)&start))
                return -1;
        }

        if(end != 0)
        {
            if(!mib_set(MIB_ADSL_LAN_CLIENT_END, (void *)&end))
                return -1;
        }

        if(poolStart.s_addr != 0)
        {
            if(!mib_set(MIB_DHCP_POOL_START, (void *)&poolStart))
                return -1;
        }

        if(poolEnd.s_addr != 0)
        {
            if(!mib_set(MIB_DHCP_POOL_END, (void *)&poolEnd))
                return -1;
        }
    }

	return 1;
}

static igmp_proxy_config_get(int *q_interval, int *q_rsp_interval,
                            int *fast_leave_enbl)
{
    if(!mib_get(MIB_IGMP_QUERY_INTERVAL, (void *)q_interval))
        return -1;

    if(!mib_get(MIB_IGMP_QUERY_RESPONSE_INTERVAL, (void *)q_rsp_interval))
        return -1;

    if(!mib_get(MIB_IGMP_FAST_LEAVE, (void *)fast_leave_enbl))
        return -1;

    return 0;
}

int igmp_proxy_config_change(int q_interval, int q_rsp_interval,
                            int fast_leave_enbl)
{
    int curr_q_interval = 0, curr_q_rsp_interval = 0, curr_fast_leave_enbl = 0;
    int ret;

    ret = igmp_proxy_config_get(&curr_q_interval, &curr_q_rsp_interval,
                            &curr_fast_leave_enbl);
    if(ret < 0)
        return -1;

    if(curr_q_interval == q_interval && curr_q_rsp_interval == q_rsp_interval
            && curr_fast_leave_enbl == fast_leave_enbl)
        return 0;

    if(q_interval != -1)
    {
        if(!mib_set(MIB_IGMP_QUERY_INTERVAL, (void *)&q_interval))
            return -1;
    }
    if(q_rsp_interval != -1)
    {
        if(!mib_set(MIB_IGMP_QUERY_RESPONSE_INTERVAL, (void *)&q_rsp_interval))
            return -1;
    }
    if(fast_leave_enbl != -1)
    {
        if(!mib_set(MIB_IGMP_FAST_LEAVE, (void *)&fast_leave_enbl))
            return -1;
    }

	return 1;
}
int igmp_proxy_mode_change(unsigned char enbl, int *need_restartWan)
{
	int total,i;
	MIB_CE_ATM_VC_T *pEntry, vc_entity;

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for( i=0; i<total; i++ )
	{
		pEntry = &vc_entity;
		if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)pEntry ) )
			continue;

		if(pEntry->cmode != CHANNEL_MODE_IPOE)
            continue;

        if(pEntry->enableIGMP == enbl)
            return 0;

        pEntry->enableIGMP = enbl;
        mib_chain_update( MIB_ATM_VC_TBL, (unsigned char*)pEntry, i );
        *need_restartWan = 1;
        return 1;
    }
	return -1;
}

int snmp_config_get(unsigned char *enable,
                        char *roCommunity, char *rwCommunity,
                        char *trapCommunity)
{
    if(!mib_get(MIB_SNMPD_ENABLE, (void *)enable))
        return -1;

    if(!mib_get(MIB_SNMP_COMM_RO_ENC, (void *)roCommunity))
        return -1;

    if(!mib_get(MIB_SNMP_COMM_RW_ENC, (void *)rwCommunity))
        return -1;

    if(!mib_get(MIB_SNMP_COMM_TRAP_ENC, (void *)trapCommunity))
        return -1;

    return 0;
}

int snmp_config_change(unsigned char enable,
                        char *roCommunity, char *rwCommunity,
                        char *trapCommunity)
{
    unsigned char currEnable = 0;
    char currRoCommunity[SNMP_STRING_LEN] = {0};
    char currRwCommunity[SNMP_STRING_LEN] = {0};
    char currTrapCommunity[SNMP_STRING_LEN] = {0};
    int ret  = 0;

    ret = snmp_config_get(&currEnable, currRoCommunity, currRwCommunity,
                        currTrapCommunity);
    if(ret < 0)
        return -1;

    if(currEnable == enable && !strcmp(currRoCommunity, roCommunity)
            && !strcmp(currRwCommunity, rwCommunity)
            && !strcmp(currTrapCommunity, trapCommunity))
        return 0;

    if(!mib_set(MIB_SNMPD_ENABLE, (void *)&enable))
        return -1;

    if(roCommunity[0] != 0)
    {
        if(!mib_set(MIB_SNMP_COMM_RO_ENC, (void *)roCommunity))
            return -1;
    }

    if(rwCommunity[0] != 0)
    {
        if(!mib_set(MIB_SNMP_COMM_RW_ENC, (void *)rwCommunity))
            return -1;
    }

    if(trapCommunity[0] != 0)
    {
        if(!mib_set(MIB_SNMP_COMM_TRAP_ENC, (void *)trapCommunity))
            return -1;
    }

	return 1;
}

#define  NTP_TIME_STATUS    "/tmp/timeStatus"

int deo_ntp_is_sync(void)
{
	int		fd;
	int		nRead;
	char	status;

	if ((fd = open(NTP_TIME_STATUS, O_RDONLY)) < 0)
	{
		return -1;
	}

	nRead = read(fd, &status, sizeof(char));
	if (nRead < 0)
	{
		printf("\n%s Failed to update NTP Time Status: %m\n", __func__);
		close(fd);
		return -1;
	}
	status -= '0'; /* ascii code to number convert */

	close(fd);

	if (status == eTStatusSynchronized)
		return 1;

	return 0;
}

#define AUTOREBOOT_RUNFILE "/var/run/autoreboot.pid"
void stopAutoReboot(void)
{
	int autorebootPid=0;

	autorebootPid = read_pid((char*)AUTOREBOOT_RUNFILE);
	if(autorebootPid > 0) {
		kill(autorebootPid, 9);
		unlink(AUTOREBOOT_RUNFILE);
	}
}

void startAutoReboot(void)
{
	char *argv[2];

	argv[1] = NULL;
	do_nice_cmd("/bin/autoreboot", argv, 0);
}

int ldap_mib_get( unsigned char *ldapcap, unsigned char *ldapUrl,
		        unsigned char *ldapFile,
				unsigned char *ldapRelativepath)

{
	if (!mib_get(MIB_LDAP_SERVER_ENABLE, (void *)ldapcap))
		return -1;

	if(!mib_get(MIB_LDAP_SERVER_URL, (void *)ldapUrl))
        return -1;

    if(!mib_get(MIB_LDAP_SERVER_FILE, (void *)ldapFile))
        return -1;

    if(!mib_get(MIB_LDAP_SERVER_RELATIVEPATH, (void *)ldapRelativepath))
        return -1;
	return 0;
}

int ldap_mib_set( unsigned char ldapcap, unsigned char *ldapUrl,
		        unsigned char *ldapFile, unsigned char *ldapRelativepath)
{
    unsigned char currLdapcap;
    unsigned char currLdapUrl[URL_LEN];
    unsigned char currLdapFile[URL_LEN];
    unsigned char currLdapRelativepath[URL_LEN];
    int ret;

    ret =ldap_mib_get(&currLdapcap, currLdapUrl,
			currLdapFile, currLdapRelativepath);
    if(ret < 0)
        return -1;

    if(currLdapcap == ldapcap
            && !strcmp(currLdapUrl, ldapUrl)
            && !strcmp(currLdapFile, ldapFile)
            && !strcmp(currLdapRelativepath, ldapRelativepath))
        return 0;

    if(!mib_set(MIB_LDAP_SERVER_ENABLE, (void *)&ldapcap))
        return -1;

    if(ldapcap == 0)
        return 1;

    if(!mib_set(MIB_LDAP_SERVER_URL, (void *)ldapUrl))
        return -1;

    if(!mib_set(MIB_LDAP_SERVER_FILE, (void *)ldapFile))
        return -1;

    if(!mib_set(MIB_LDAP_SERVER_RELATIVEPATH, (void *)ldapRelativepath))
        return -1;

	return 1;
}

void startLdap(void)
{
	char *argv[2];

	argv[1] = NULL;
	do_nice_cmd( "/bin/ldap", argv, 0 );

}

int autoReboot_ldap_mib_get(MIB_AUTOREBOOT_T *entry)
{
	if (!mib_get_s(MIB_AUTOREBOOT_LDAP_CONFIG_SERVER, &entry->ldapConfigServer, sizeof(char)))
		return -1;
	if (!mib_get_s(MIB_AUTOREBOOT_LDAP_FILE_VERSION_SERVER, entry->ldapFileVersionServer, 128))
		return -1;
	if (!mib_get_s(MIB_AUTOREBOOT_ON_IDEL_SERVER, &entry->autoRebootOnIdelServer, sizeof(char)))
		return -1;
	if (!mib_get_s(MIB_AUTOREBOOT_UPTIME_SERVER, &entry->autoRebootUpTimeServer, sizeof(int)))
		return -1;
	if (!mib_get_s(MIB_AUTOREBOOT_WAN_PORT_IDLE_SERVER, &entry->autoRebootWanPortIdleServer, sizeof(char)))
		return -1;
	if (!mib_get_s(MIB_AUTOREBOOT_HR_STARTHOUR_SERVER, &entry->autoRebootHRangeStartHourServer, sizeof(int)))
		return -1;
	if (!mib_get_s(MIB_AUTOREBOOT_HR_STARTMIN_SERVER, &entry->autoRebootHRangeStartMinServer, sizeof(int)))
		return -1;
	if (!mib_get_s(MIB_AUTOREBOOT_HR_ENDHOUR_SERVER, &entry->autoRebootHRangeEndHourServer, sizeof(int)))
		return -1;
	if (!mib_get_s(MIB_AUTOREBOOT_HR_ENDMIN_SERVER, &entry->autoRebootHRangeEndMinServer, sizeof(int)))
		return -1;
	if (!mib_get_s(MIB_AUTOREBOOT_DAY_SERVER, &entry->autoRebootDayServer, sizeof(int)))
		return -1;
	if (!mib_get_s(MIB_AUTOREBOOT_AVR_DATA_SERVER, &entry->autoRebootAvrDataServer, sizeof(int)))
		return -1;
	if (!mib_get_s(MIB_AUTOREBOOT_AVR_DATA_EPG_SERVER, &entry->autoRebootAvrDataEPGServer, sizeof(int)))
		return -1;
	if (!mib_get_s(MIB_AUTOREBOOT_CRC_SERVER, &entry->autoRebootCrcServer, sizeof(int)))
		return -1;

	return 0;
}

int autoReboot_ldap_mib_set(MIB_AUTOREBOOT_T *entry)
{
	if (!mib_set(MIB_AUTOREBOOT_LDAP_CONFIG_SERVER, &entry->ldapConfigServer))
		return -1;
	if (!mib_set(MIB_AUTOREBOOT_LDAP_FILE_VERSION_SERVER, entry->ldapFileVersionServer))
		return -1;
	if (!mib_set(MIB_AUTOREBOOT_ON_IDEL_SERVER, &entry->autoRebootOnIdelServer))
		return -1;
	if (!mib_set(MIB_AUTOREBOOT_UPTIME_SERVER, &entry->autoRebootUpTimeServer))
		return -1;
	if (!mib_set(MIB_AUTOREBOOT_WAN_PORT_IDLE_SERVER, &entry->autoRebootWanPortIdleServer))
		return -1;
	if (!mib_set(MIB_AUTOREBOOT_HR_STARTHOUR_SERVER, &entry->autoRebootHRangeStartHourServer))
		return -1;
	if (!mib_set(MIB_AUTOREBOOT_HR_STARTMIN_SERVER, &entry->autoRebootHRangeStartMinServer))
		return -1;
	if (!mib_set(MIB_AUTOREBOOT_HR_ENDHOUR_SERVER, &entry->autoRebootHRangeEndHourServer))
		return -1;
	if (!mib_set(MIB_AUTOREBOOT_HR_ENDMIN_SERVER, &entry->autoRebootHRangeEndMinServer))
		return -1;
	if (!mib_set(MIB_AUTOREBOOT_DAY_SERVER, &entry->autoRebootDayServer))
		return -1;
	if (!mib_set(MIB_AUTOREBOOT_AVR_DATA_SERVER, &entry->autoRebootAvrDataServer))
		return -1;
	if (!mib_set(MIB_AUTOREBOOT_AVR_DATA_EPG_SERVER, &entry->autoRebootAvrDataEPGServer))
		return -1;
	if (!mib_set(MIB_AUTOREBOOT_CRC_SERVER, &entry->autoRebootCrcServer))
		return -1;

	if (!mib_set(MIB_ENABLE_AUTOREBOOT, &entry->autoRebootOnIdelServer))
		return -1;

	return 0;
}

#define CONNTRACK_MON_RUNFILE "/var/run/conntrack_mon.pid"
void stopConntrack_mon(void)
{
	int connPid=0;

	connPid = read_pid((char*)CONNTRACK_MON_RUNFILE);
	if(connPid > 0) {
		kill(connPid, 9);
		unlink(CONNTRACK_MON_RUNFILE);
	}
}

void startConntrack_mon(void)
{
	char *argv[2];

	argv[1] = NULL;
	do_nice_cmd("/bin/conntrack_mon", argv, 0);
}

#define USEAGE_RUNFILE "/var/run/useage.pid"
void stopUseage(void)
{
	int connPid=0;

	connPid = read_pid((char*)USEAGE_RUNFILE);
	if(connPid > 0) {
		kill(connPid, 9);
		unlink(USEAGE_RUNFILE);
	}
}

void startUseage(void)
{
	char *argv[2];

	argv[1] = NULL;
	do_nice_cmd("/bin/useage", argv, 0);
}

#define DHCPRELAY_RUNFILE "/var/run/dhcprelay.pid"
void stopDhcpRelay(void)
{
	int drPid=0;

	drPid = read_pid((char*)DHCPRELAY_RUNFILE);
	if(drPid > 0) {
		kill(drPid, 9);
		unlink(DHCPRELAY_RUNFILE);
	}
}

void deo_startDhcpRelay(void)
{
	char *argv[2];

	argv[1] = NULL;
	do_nice_cmd("/bin/dhcprelay", argv, 0);
}

#define DHCPRELAY6_RUNFILE "/var/run/dhcprelay6.pid"
void stopDhcpRelay6(void)
{
	int drPid=0;

	drPid = read_pid((char*)DHCPRELAY6_RUNFILE);
	if(drPid > 0) {
		kill(drPid, 9);
		unlink(DHCPRELAY6_RUNFILE);
	}
}

void deo_startDhcpRelay6(void)
{
	char *argv[2];

	argv[1] = NULL;
	do_nice_cmd("/bin/dhcprelay6", argv, 0);
}

#define DNSPROXY_RUNFILE "/var/run/dnsproxy.pid"
void stopDnsProxy(void)
{
	int drPid=0;

	drPid = read_pid((char*)DNSPROXY_RUNFILE);
	if(drPid > 0) {
		kill(drPid, 9);
		unlink(DNSPROXY_RUNFILE);
	}
}

void deo_startDnsProxy(void)
{
	char *argv[2];

	argv[1] = NULL;
	do_nice_cmd("/bin/dnsproxy", argv, 0);
}

int start_ssh(void)
{
	char cmd[64] = {0};
	char lan_ip[16] = {0};
	char wan_ip[16] = {0};
	unsigned char status = 0;

	mib_get(MIB_SSH_ENABLE, (void *)&status);

	if (status) {
		if (deo_wan_nat_mode_get() != DEO_BRIDGE_MODE) {
			get_ipv4_from_interface(lan_ip, 1);
			get_ipv4_from_interface(wan_ip, 2);
		} else {
			get_ipv4_from_interface(lan_ip, 3);
			get_ipv4_from_interface(wan_ip, 1);
		}

		snprintf(cmd, sizeof(cmd), "/bin/dropbear -F -p %s:22 &", wan_ip);
		system(cmd);

		memset(cmd, 0, sizeof(cmd));
		snprintf(cmd, sizeof(cmd), "/bin/dropbear -F -p %s:22 &", lan_ip);
		system(cmd);

		// allow tcp 22 port from FC
		system("echo ssh_mode 0 > /proc/driver/realtek/ssh_mode &");
	}
}

void deo_dns_value_get(char *dns1, char *dns2)
{
	struct in_addr tmpdns1, tmpdns2;
	MIB_CE_ATM_VC_T Entry;

	if (deo_wan_nat_mode_get() == DEO_NAT_MODE)
		mib_chain_get(MIB_ATM_VC_TBL, 0, (void *)&Entry);
	else
		mib_chain_get(MIB_ATM_VC_TBL, 1, (void *)&Entry);

	if (Entry.dnsMode == 0)
	{
		sprintf(dns1, "%d.%d.%d.%d", Entry.v4dns1[0], Entry.v4dns1[1], Entry.v4dns1[2], Entry.v4dns1[3]);
		sprintf(dns2, "%d.%d.%d.%d", Entry.v4dns2[0], Entry.v4dns2[1], Entry.v4dns2[2], Entry.v4dns2[3]);
	}
	else
	{
		mib_get_s(MIB_DHCPS_DNS1, (void *)&tmpdns1, sizeof(tmpdns1));
		strcpy(dns1, inet_ntoa(tmpdns1));

		mib_get_s(MIB_DHCPS_DNS2, (void *)&tmpdns2, sizeof(tmpdns2));
		strcpy(dns2, inet_ntoa(tmpdns2));
	}

	return;
}

void deo_dns_addr_value_get(unsigned char *dns1, unsigned char *dns2)
{
	struct in_addr tmpdns1, tmpdns2;
	MIB_CE_ATM_VC_T Entry;

	if (deo_wan_nat_mode_get() == DEO_NAT_MODE)
		mib_chain_get(MIB_ATM_VC_TBL, 0, (void *)&Entry);
	else
		mib_chain_get(MIB_ATM_VC_TBL, 1, (void *)&Entry);

	if (Entry.dnsMode == 0)
	{
		memcpy(dns1, &Entry.v4dns1, sizeof(Entry.v4dns1));
		memcpy(dns2, &Entry.v4dns2, sizeof(Entry.v4dns2));
	}
	else
	{
		mib_get_s(MIB_DHCPS_DNS1, (void *)&tmpdns1, sizeof(tmpdns1));
		memcpy(dns1, &tmpdns1, sizeof(tmpdns1));

		mib_get_s(MIB_DHCPS_DNS2, (void *)&tmpdns2, sizeof(tmpdns2));
		memcpy(dns2, &tmpdns2, sizeof(tmpdns2));
	}

	return;
}

void deo_resolv_value_update(char *ifname, char *resolv_fname)
{
	FILE *wfp;
	char wanip[16];
	char dns1[16];
	char dns2[16];
	char line[80];

	memset(wanip, 0, sizeof(wanip));
	memset(dns1, 0, sizeof(dns1));
	memset(dns2, 0, sizeof(dns2));
	memset(line, 0, sizeof(line));

	getIfAddr(ifname, wanip);

	wfp = fopen(resolv_fname, "w");
	if (wfp == NULL)
	{
		printf("\n%s file open error.", resolv_fname);
		return;
	}

	deo_dns_value_get(dns1, dns2);

	sprintf(line, "%s@%s\n", dns1, wanip);
	fputs(line, wfp);

	sprintf(line, "%s@%s\n", dns2, wanip);
	fputs(line, wfp);

	fclose(wfp);

	return;
}

void deo_dns_mode_get(int *mode)
{
	MIB_CE_ATM_VC_T Entry;

	if (deo_wan_nat_mode_get() == DEO_NAT_MODE)
		mib_chain_get(MIB_ATM_VC_TBL, 0, (void *)&Entry);
	else
		mib_chain_get(MIB_ATM_VC_TBL, 1, (void *)&Entry);

	*mode = Entry.dnsMode;

	return;
}

void deo_dns_fixed_value_get(int *mode, char *dns1, char *dns2)
{
	MIB_CE_ATM_VC_T Entry;

	if (deo_wan_nat_mode_get() == DEO_NAT_MODE)
		mib_chain_get(MIB_ATM_VC_TBL, 0, (void *)&Entry);
	else
		mib_chain_get(MIB_ATM_VC_TBL, 1, (void *)&Entry);

	*mode = Entry.dnsMode;

	if (Entry.dnsMode == 0)
	{
		memcpy(dns1, Entry.v4dns1, 4);
		memcpy(dns2, Entry.v4dns2, 4);
	}

	return;
}

void deo_dns_fixed_addr_mode_get(int *mode)
{
	MIB_CE_ATM_VC_T Entry;

	if (deo_wan_nat_mode_get() == DEO_NAT_MODE)
		mib_chain_get(MIB_ATM_VC_TBL, 0, (void *)&Entry);
	else
		mib_chain_get(MIB_ATM_VC_TBL, 1, (void *)&Entry);

	*mode = Entry.ipDhcp;

	return;
}

/* enable (0: fixed dns, 1:auto),
 * return value : same value : 0, different value : 1 */
int deo_dns_auto_set(int enable)
{
	char cmd[256];
	int entrynum = 0;
	struct in_addr in_addr_dns1, in_addr_dns2;
	MIB_CE_ATM_VC_T Entry;

	if (deo_wan_nat_mode_get() == DEO_NAT_MODE)
		entrynum = 0;
	else
		entrynum = 1;

	mib_chain_get(MIB_ATM_VC_TBL, entrynum, (void *)&Entry);

	if (Entry.dnsMode == enable)
		return 0;

	Entry.dnsMode = enable;

	if (enable == 0)
	{
		if (!inet_aton(SKB_DEF_DNS1, (struct in_addr *)&Entry.v4dns1))
		{
			printf("invalid or unknown target %s", SKB_DEF_DNS1);
			return 0;
		}

		if (!inet_aton(SKB_DEF_DNS2, (struct in_addr *)&Entry.v4dns2))
		{
			printf("invalid or unknown target %s", SKB_DEF_DNS2);
			return 0;
		}

		if (!inet_aton(SKB_DEF_DNS1, &in_addr_dns1)) {
			printf("invalid or unknown target %s", SKB_DEF_DNS1);
			return 0;
		}

		if (!inet_aton(SKB_DEF_DNS2, &in_addr_dns2)) {
			printf("invalid or unknown target %s", SKB_DEF_DNS2);
			return 0;
		}

		mib_set(MIB_DHCPS_DNS1, (void *)&in_addr_dns1);
		mib_set(MIB_DHCPS_DNS2, (void *)&in_addr_dns2);

		if (deo_wan_nat_mode_get() == DEO_NAT_MODE)
		{
			deo_udhcpd_conf_update(SKB_DEF_DNS1, SKB_DEF_DNS2);

			memset(cmd, 0, sizeof(cmd));
			snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_restart_dhcpd");
			system(cmd);
		}
	}

	mib_chain_update(MIB_ATM_VC_TBL, (void *)&Entry, entrynum);

	return 1;
}

int deo_udhcpd_conf_update(char *dns1, char *dns2)
{
	FILE *file, *temp;
	int keep_reading = 1;
	char buffer[MAX_LINE];
	char tmp_buffer[MAX_LINE];
	int first = 0;
	int ret  = 0;

	file = fopen(DNS_FILE_NAME, "r");
	if (file == NULL)
		return -1;

	temp = fopen(TEMP_DNS_FILE_NAME, "w");
	if (temp == NULL)
	{
		printf("%s Error temp opening files(%s).\n", __func__, TEMP_DNS_FILE_NAME);
		fclose(file);
		return -1;
	}

	do
	{
		fgets(buffer, MAX_LINE, file);

		if (strncmp(buffer, "poolend end", strlen("poolend end")) == 0)
			keep_reading = 0;

		if (strncmp(buffer, "opt dns", strlen("opt dns")) == 0)
		{
			first++;
			if (first == 1)
				sprintf(tmp_buffer, "opt dns %s\n", dns1);
			else
				sprintf(tmp_buffer, "opt dns %s\n", dns2);

			fputs(tmp_buffer, temp);

			if (strncmp(buffer, tmp_buffer, strlen(tmp_buffer)) != 0)
				ret = 1;
		}
		else
			fputs(buffer, temp);

	} while (keep_reading);

	fclose(file);
	fclose(temp);

	remove(DNS_FILE_NAME);
	rename(TEMP_DNS_FILE_NAME, DNS_FILE_NAME);

	return ret;
}

/*
 SKB 12차 규격
 Server                                CPE 예상결과
 1st               2nd                 1st             2nd
 210.220.163.82    219.250.36.130      210.220.163.82  219.250.36.130
 210.220.163.82    8.8.8.8             210.220.163.82  8.8.8.8
 210.220.163.82    -                   210.220.163.82  219.250.36.130
 219.250.36.130    210.220.163.82      219.250.36.130  210.220.163.82
 219.250.36.130    8.8.8.8             219.250.36.130  8.8.8.8
 219.250.36.130    -                   219.250.36.130  210.220.163.82
 8.8.8.8           210.220.163.82      219.250.36.130  210.220.163.82
 8.8.8.8           219.250.36.130      219.250.36.130  210.220.163.82
 8.8.8.8           9.9.9.9             219.250.36.130  9.9.9.9
 8.8.8.8           -                   219.250.36.130  8.8.8.8
 -                 -                   219.250.36.130  210.220.163.82
 */

/* EX)
 * dns1: "219.250.36.130"
 * dns2: "210.220.163.82"
 */
int deo_dnsModifyUpdate(char *dns1, char *dns2)
{
	int restart_dhcpd = 0;
	char updateDns1[16];
	char updateDns2[16];
	char cmd[256];
	struct in_addr in_addr_dns1, in_addr_dns2;

	memset(updateDns1, 0, 16);
	memset(updateDns2, 0, 16);

	if ((dns1[0] == 0) && (dns2[0] == 0))
	{
		strcpy(updateDns1, SKB_DEF_DNS1);
		strcpy(updateDns2, SKB_DEF_DNS2);
	}
	else
	{
		if ((strcmp(dns1, SKB_DEF_DNS1) != 0) && (strcmp(dns1, SKB_DEF_DNS2) != 0))
		{
			strcpy(updateDns1, SKB_DEF_DNS1);
			if ((strcmp(dns2, SKB_DEF_DNS1) != 0) && (strcmp(dns2, SKB_DEF_DNS2) != 0))
			{
				if (dns2[0] == 0)
					strcpy(updateDns2, dns1);
				else
					strcpy(updateDns2, dns2);
			}
			else
			{
				if ((strcmp(dns2, SKB_DEF_DNS1) == 0) || (strcmp(dns2, SKB_DEF_DNS2) == 0))
					strcpy(updateDns2, SKB_DEF_DNS2);
			}
		}
		else if (strcmp(dns1, SKB_DEF_DNS1) == 0)
		{
			strcpy(updateDns1, SKB_DEF_DNS1);
			if ((strcmp(dns2, SKB_DEF_DNS1) != 0) && (strcmp(dns2, SKB_DEF_DNS2) != 0))
			{
				if (dns2[0] == 0)
					strcpy(updateDns2, SKB_DEF_DNS2);
				else
					strcpy(updateDns2, dns2);
			}
			else
			{
				strcpy(updateDns2, SKB_DEF_DNS2);
			}
		}
		else if (strcmp(dns1, SKB_DEF_DNS2) == 0)
		{
			strcpy(updateDns1, SKB_DEF_DNS2);
			if ((strcmp(dns2, SKB_DEF_DNS1) != 0) && (strcmp(dns2, SKB_DEF_DNS2) != 0))
			{
				if (dns2[0] == 0)
					strcpy(updateDns2, SKB_DEF_DNS1);
				else
					strcpy(updateDns2, dns2);
			}
			else
			{
				strcpy(updateDns2, SKB_DEF_DNS1);
			}
		}
	}

	if (!inet_aton(updateDns1, &in_addr_dns1)) {
		printf("invalid or unknown target %s", updateDns1);
		return -1;
	}

	if (!inet_aton(updateDns2, &in_addr_dns2)) {
		printf("invalid or unknown target %s", updateDns2);
		return -1;
	}

	mib_set(MIB_DHCPS_DNS1, (void *)&in_addr_dns1);
	mib_set(MIB_DHCPS_DNS2, (void *)&in_addr_dns2);

	restart_dhcpd = deo_udhcpd_conf_update(updateDns1, updateDns2);

	if (deo_wan_nat_mode_get() == DEO_NAT_MODE)
	{
		if (restart_dhcpd == 1)
		{
			memset(cmd, 0, sizeof(cmd));
			snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_restart_dhcpd");
			system(cmd);
		}
	}

	return 0;
}
#endif

#define  NTP_TIME_STATUS    "/tmp/timeStatus"
int deo_ntp_time_status_get(unsigned char *status)
{
	int         fd;
	int         nRead;

	if ((fd = open(NTP_TIME_STATUS, O_RDONLY)) < 0)
	{
		return -1;
	}

	nRead = read(fd, status, sizeof(char));
	if (nRead < 0)
	{
		printf("Failed to update NTP Time Status: %m\n");
		close(fd);
		return -1;
	}
	*status -= '0'; /* ascii code to number convert */

	close(fd);

	return 0;
}

int source_mac_drop(int mode)
{
	int32 ret = RT_ERR_FAILED;
	int acl_filter_idx = 0;
	char mac[16] = {0};
	unsigned char hmac[6];
	unsigned int allLanPortMask = ((1 << CONFIG_LAN_PORT_NUM) - 1);

	rt_acl_filterAndQos_t rule;

	if (mode == 1) { // only nat for nas0_0
		memset(mac, 0, sizeof(mac));
		memset(hmac, 0, sizeof(hmac));
		memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));

		get_mac_from_interface("nas0_0", mac);
		rtk_string_to_hex(mac, hmac, 12);

		rule.filter_fields |= RT_ACL_INGRESS_SMAC_BIT;
		rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		rule.ingress_port_mask = rtk_port_get_wan_phyMask();

		rule.acl_weight = WEIGHT_HIGH;
		rule.ingress_smac_mask[0] = 0xff;
		rule.ingress_smac_mask[1] = 0xff;
		rule.ingress_smac_mask[2] = 0xff;
		rule.ingress_smac_mask[3] = 0xff;
		rule.ingress_smac_mask[4] = 0xff;
		rule.ingress_smac_mask[5] = 0xff;
		rule.ingress_smac[0] = hmac[0];
		rule.ingress_smac[1] = hmac[1];
		rule.ingress_smac[2] = hmac[2];
		rule.ingress_smac[3] = hmac[3];
		rule.ingress_smac[4] = hmac[4];
		rule.ingress_smac[5] = hmac[5];

		rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;
		ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);

		/*
		if (ret == RT_ERR_OK)
			printf("%s: add invalid mac filter acl entry[%d] success!\n", __func__, acl_filter_idx);
		else
			printf("%s: add invalid mac filter acl entry failed!\n", __func__);
		*/
	}

	// nat & bridge for br0 interface
	memset(mac, 0, sizeof(mac));
	memset(hmac, 0, sizeof(hmac));
	memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));

	get_mac_from_interface("br0", mac);
	rtk_string_to_hex(mac, hmac, 12);

	rule.filter_fields |= RT_ACL_INGRESS_SMAC_BIT;
	rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;

	if (mode == 1) // only nat for nas0_0
		rule.ingress_port_mask = rtk_port_get_lan_phyMask(allLanPortMask);
	else
		rule.ingress_port_mask = (rtk_port_get_wan_phyMask() | rtk_port_get_lan_phyMask(allLanPortMask));

	rule.acl_weight = WEIGHT_HIGH;
	rule.ingress_smac_mask[0] = 0xff;
	rule.ingress_smac_mask[1] = 0xff;
	rule.ingress_smac_mask[2] = 0xff;
	rule.ingress_smac_mask[3] = 0xff;
	rule.ingress_smac_mask[4] = 0xff;
	rule.ingress_smac_mask[5] = 0xff;
	rule.ingress_smac[0] = hmac[0];
	rule.ingress_smac[1] = hmac[1];
	rule.ingress_smac[2] = hmac[2];
	rule.ingress_smac[3] = hmac[3];
	rule.ingress_smac[4] = hmac[4];
	rule.ingress_smac[5] = hmac[5];

	rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;
	ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);

	/*
	if (ret == RT_ERR_OK)
		printf("%s: add invalid source mac filter acl entry[%d] success!\n", __func__, acl_filter_idx);
	else
		printf("%s: add invalid mac filter acl entry failed!\n", __func__);
	*/

	// when smac is multicast mac & dmac is all ff, drop
	memset(mac, 0, sizeof(mac));
	memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));

	rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rule.ingress_port_mask = rtk_port_get_wan_phyMask() | rtk_port_get_lan_phyMask(allLanPortMask);

	rule.filter_fields |= RT_ACL_INGRESS_SMAC_BIT;
	rule.ingress_smac_mask[0] = 0xff;
	rule.ingress_smac_mask[1] = 0xff;
	rule.ingress_smac_mask[2] = 0xff;
	rule.ingress_smac_mask[3] = 0x80;
	rule.ingress_smac_mask[4] = 0x00;
	rule.ingress_smac_mask[5] = 0x00;
	rule.ingress_smac[0] = 0x01;
	rule.ingress_smac[1] = 0x00;
	rule.ingress_smac[2] = 0x5e;
	rule.ingress_smac[3] = 0x00;
	rule.ingress_smac[4] = 0x00;
	rule.ingress_smac[5] = 0x00;

	rule.acl_weight = WEIGHT_HIGH;

	rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;
	ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);


	// when smac is all FF, drop
	memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));

	rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rule.ingress_port_mask = rtk_port_get_wan_phyMask() | rtk_port_get_lan_phyMask(allLanPortMask);

	rule.filter_fields |= RT_ACL_INGRESS_SMAC_BIT;
	rule.ingress_smac_mask[0] = 0xff;
	rule.ingress_smac_mask[1] = 0xff;
	rule.ingress_smac_mask[2] = 0xff;
	rule.ingress_smac_mask[3] = 0xff;
	rule.ingress_smac_mask[4] = 0xff;
	rule.ingress_smac_mask[5] = 0xff;
	rule.ingress_smac[0] = 0xff;
	rule.ingress_smac[1] = 0xff;
	rule.ingress_smac[2] = 0xff;
	rule.ingress_smac[3] = 0xff;
	rule.ingress_smac[4] = 0xff;
	rule.ingress_smac[5] = 0xff;

	rule.acl_weight = WEIGHT_HIGH;

	rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;
	ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);


	// when smac is all 00, drop
	memset(&rule, 0, sizeof(rt_acl_filterAndQos_t));

	rule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rule.ingress_port_mask = rtk_port_get_wan_phyMask() | rtk_port_get_lan_phyMask(allLanPortMask);

	rule.filter_fields |= RT_ACL_INGRESS_SMAC_BIT;
	rule.ingress_smac_mask[0] = 0xff;
	rule.ingress_smac_mask[1] = 0xff;
	rule.ingress_smac_mask[2] = 0xff;
	rule.ingress_smac_mask[3] = 0xff;
	rule.ingress_smac_mask[4] = 0xff;
	rule.ingress_smac_mask[5] = 0xff;
	rule.ingress_smac[0] = 0x00;
	rule.ingress_smac[1] = 0x00;
	rule.ingress_smac[2] = 0x00;
	rule.ingress_smac[3] = 0x00;
	rule.ingress_smac[4] = 0x00;
	rule.ingress_smac[5] = 0x00;

	rule.acl_weight = WEIGHT_HIGH;

	rule.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;
	ret = rt_acl_filterAndQos_add(rule, &acl_filter_idx);

	return 0;
}


int check_ip_address(char* ifname)
{
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in *sa;
	char ip_address[128];

	if (getifaddrs(&ifap) == -1)
		return 0;

	for (ifa = ifap; ifa; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr && (ifa->ifa_flags & IFF_UP) && strcmp(ifa->ifa_name, ifname) == 0)
		{
			sa = (struct sockaddr_in *) ifa->ifa_addr;
			inet_ntop(AF_INET, &sa->sin_addr, ip_address, INET_ADDRSTRLEN);
			printf("detect ipaddress.%s\n", ip_address);
			freeifaddrs(ifap);
			return 1;
		}
	}
	freeifaddrs(ifap);
	return 0;
}

int getWanIpAddr(char *ifname, struct in_addr *addr)
{
	struct ifreq ifr;
	struct sockaddr_in *saddr;
	int fd, ret = 0;

	addr->s_addr = 0;
	fd = get_sockfd();
	if (fd >= 0) {
		strcpy(ifr.ifr_name, ifname);
		ifr.ifr_addr.sa_family = AF_INET;
		if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
		{
			saddr = (struct sockaddr_in *)&ifr.ifr_addr;
			*addr = saddr->sin_addr;
			ret = 1;
		}
		close(fd);
	}

	return ret;
}

int getgatewayandiface(in_addr_t * addr)
{
	long destination, gateway;
	char iface[16];
	char buf[BUF_SIZE];
	FILE * file;

	memset(iface, 0, sizeof(iface));
	memset(buf, 0, sizeof(buf));

	file = fopen("/proc/net/route", "r");
	if (!file)
		return -1;

	while (fgets(buf, sizeof(buf), file))
	{
		if (sscanf(buf, "%s %lx %lx", iface, &destination, &gateway) == 3)
		{
			if (destination == 0)
			{ /* default */
				*addr = gateway;
				fclose(file);
				return 0;
			}
		}
	}

	/* default route not found */
	if (file)
		fclose(file);
	return -1;
}



int deo_defaultGW_get(char *gateway)
{
	in_addr_t addr = 0;

	getgatewayandiface(&addr);

	strcpy(gateway, inet_ntoa(*(struct in_addr *) &addr));

	return 0;
}

int  deo_arp(char *ip_addr)
{
	int sd;
	unsigned char buffer[BUF_SIZE];
	unsigned char rcv_buffer[BUF_SIZE];
	unsigned char source_ip[4] = {0, };
	unsigned char target_ip[4] = {0, };
	struct ifreq ifr;
	struct ethhdr *send_req = (struct ethhdr *)buffer;
	struct ethhdr *rcv_resp= (struct ethhdr *)rcv_buffer;
	struct arp_header *arp_req = (struct arp_header *)(buffer+ETH2_HEADER_LEN);
	struct arp_header *arp_resp = (struct arp_header *)(buffer+ETH2_HEADER_LEN);
	struct sockaddr_ll socket_address;
	struct in_addr addr;
	const int sock_value = 255;
	in_addr_t target_addr = 0;
	int index, ret, length=0, ifindex;
	int loop_cnt = 0;
	int sleep_cnt = 0;
	int nat_mode;

	/* open socket */
	sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sd == -1) {
		printf("\n%s socket create error ", __func__);
		return -1;
	}

	mib_get_s(MIB_WAN_NAT_MODE, &nat_mode, sizeof(int));
	if (nat_mode == DEO_BRIDGE_MODE)
		strcpy(ifr.ifr_name, "br0");
	else
		strcpy(ifr.ifr_name, "nas0_0");


	memset(buffer,0x00,60);

	/* retrieve ethernet interface index */
	if (ioctl(sd, SIOCGIFINDEX, &ifr) == -1) {
		printf("\n%s ioctl SIOCGIFINDEX error ", __func__);
		return -1;
	}

	ifindex = ifr.ifr_ifindex;

	/*retrieve corresponding MAC*/
	if (ioctl(sd, SIOCGIFHWADDR, &ifr) == -1) {
		printf("\n%s ioctl SIOCGIFHWADDR error ", __func__);
		return -1;
	}
	close (sd);

	getWanIpAddr(ifr.ifr_name, &addr);
	{
		source_ip[0] = (addr.s_addr >> 24) & 0xFF;
		source_ip[1] = (addr.s_addr >> 16) & 0xFF;
		source_ip[2] = (addr.s_addr >> 8) & 0xFF;
		source_ip[3] = addr.s_addr & 0xFF;

	}
	getgatewayandiface(&target_addr);
	{
		target_ip[0] = (target_addr >> 24) & 0xFF;
		target_ip[1] = (target_addr >> 16) & 0xFF;
		target_ip[2] = (target_addr >> 8) & 0xFF;
		target_ip[3] = target_addr & 0xFF;
	}
	for (index = 0; index < 6; index++)
	{
		send_req->h_dest[index] = (unsigned char)0xff;
		arp_req->target_mac[index] = (unsigned char)0x00;

		/* Filling the source  mac address in the header*/
		send_req->h_source[index] = (unsigned char)ifr.ifr_hwaddr.sa_data[index];
		arp_req->sender_mac[index] = (unsigned char)ifr.ifr_hwaddr.sa_data[index];
		socket_address.sll_addr[index] = (unsigned char)ifr.ifr_hwaddr.sa_data[index];
	}
	/*prepare sockaddr_ll*/
	socket_address.sll_family = AF_PACKET;
	socket_address.sll_protocol = htons(ETH_P_ARP);
	socket_address.sll_ifindex = ifindex;
	socket_address.sll_hatype = htons(ARPHRD_ETHER);
	socket_address.sll_pkttype = (PACKET_BROADCAST);
	socket_address.sll_halen = MAC_LENGTH;
	socket_address.sll_addr[6] = 0x00;
	socket_address.sll_addr[7] = 0x00;

	/* Setting protocol of the packet */
	send_req->h_proto = htons(ETH_P_ARP);

	/* Creating ARP request */
	arp_req->hardware_type = htons(HW_TYPE);
	arp_req->protocol_type = htons(ETH_P_IP);
	arp_req->hardware_len = MAC_LENGTH;
	arp_req->protocol_len =IPV4_LENGTH;
	arp_req->opcode = htons(ARP_REQUEST);

	for (index=0; index<4; index++)
	{
		arp_req->sender_ip[index] = (unsigned char)source_ip[index];
		arp_req->target_ip[index] = (unsigned char)target_ip[index];
	}
	if ((sd = socket (PF_PACKET, SOCK_RAW, htons(ETH_P_ARP))) < 0)
	{
		printf("\n%s socket create error ", __func__);
		return -1;
	}

	if ( fcntl(sd, F_SETFL, O_NONBLOCK) != 0 )
	{
		printf("arp() request nonblocking I/O failed : [%02d] %s\n", errno, strerror(errno));
		return -1;
	}

	for (loop_cnt = 0; loop_cnt < COUNT_CHECK_LOOP+1; loop_cnt++)
	{
		length = recvfrom(sd, rcv_buffer, BUF_SIZE, 0, NULL, NULL);
		if (length > 0)
		{
			if(htons(rcv_resp->h_proto) == PROTO_ARP)
			{
				if (memcmp(send_req->h_source, rcv_resp->h_dest, ETH_ALEN) == 0)
				{
					close(sd);
					return 0;
				}
			}
		}
		if (loop_cnt < COUNT_CHECK_LOOP)
		{
			buffer[42]=0x00;
			ret = sendto(sd, buffer, 42, 0, (struct  sockaddr*)&socket_address, sizeof(socket_address));
			if (ret == -1)
			{
				printf("arp() sendto failed : [%02d] %s\n", errno, strerror(errno));
			}
		}

		for (sleep_cnt = 0 ; sleep_cnt < COUNT_CHECK_LOOP; sleep_cnt++ )
			usleep(TIME_LOOP_WAIT);
	}

	close(sd);
	return -1;
}

#define TRAP_MSG_SIZE 2048
#define TRAP_WAIT_TIME 40// sec

void send_snmp_trap(char *msg, int trapId, int isencrypt)
{
	unsigned char trapip[IP_ADDR_LEN]	= {0,};
	unsigned char url[64]				= {0,};
	unsigned char ip[32]				= {0,};
	unsigned char cmd[TRAP_MSG_SIZE]	= {0,};
	unsigned char community[64]			= {0,};
	unsigned char var;
	char defGateWay[16]					= {0,};
	int	i								= 0;
	in_addr_t addr						= 0;


	if(mib_get(MIB_SNMPD_ENABLE, &var))
	{
		if(var == 0)
		{
			return;
		}
	}
	
	if(isencrypt)
	{
		if(!mib_get(MIB_SNMP_COMM_TRAP_ENC, community))
			return;
	}
	else
		strcpy(community, "iptvshrw^_");

	if(mib_get(MIB_SNMP_COMM_TRAP_ON, &var) && var == 1)
	{
		if(mib_get(MIB_SNMP_TRAP_IP_IA, trapip))
		{
			if(isencrypt)
				sprintf(ip, "%d.%d.%d.%d:30162", trapip[0], trapip[1], trapip[2], trapip[3]);
			else
				sprintf(ip, "%d.%d.%d.%d:162", trapip[0], trapip[1], trapip[2], trapip[3]);
			if(strcmp(ip, "0.0.0.0"))
			{
				snprintf(cmd, sizeof(cmd), msg, community, ip);
				system(cmd);
			}
		}
	}

	memset(trapip, 0, IP_ADDR_LEN);
	memset(cmd, 0, 512);

	if(isencrypt && (trapId != MIB_SNMP_COMM_TRAP1_IP))
	{
		trapId = MIB_SNMP_COMM_TRAP17_IP;
	}

	if(mib_get(trapId, trapip))
	{
		if(isencrypt)
			sprintf(ip, "%d.%d.%d.%d:30162", trapip[0], trapip[1], trapip[2], trapip[3]);
		else
			sprintf(ip, "%d.%d.%d.%d:162", trapip[0], trapip[1], trapip[2], trapip[3]);

		if(strcmp(ip, "0.0.0.0"))
		{
			snprintf(cmd, sizeof(cmd), msg, community, ip);
			system(cmd);
		}
	}

	return;
}


#if 0
#define TRAP_MSG_SIZE 2048
#define TRAP_WAIT_TIME 40// sec
struct ThreadArgs {
	int trap_id;
	char msg[TRAP_MSG_SIZE];
};

void *send_snmp_trap_thread(void *arg)
{
	struct ThreadArgs *args				= (struct ThreadArgs *)arg;
	char *msg							= args->msg;
	int trapId							= args->trap_id;
	unsigned char trapip[IP_ADDR_LEN]	= {0,};
	unsigned char url[64]				= {0,};
	unsigned char ip[32]				= {0,};
	unsigned char ip2[32]				= {0,};
	unsigned char cmd[TRAP_MSG_SIZE]	= {0,};
	unsigned char cmd2[TRAP_MSG_SIZE]	= {0,};
	unsigned char var;
	char defGateWay[16]					= {0,};
	int	i								= 0;
	in_addr_t addr						= 0;


	if(mib_get(MIB_SNMPD_ENABLE, &var))
		if(var == 0)
		{
			free(args);
			return;
		}

	for(i = 0; i < TRAP_WAIT_TIME; i++)
	{
		if(getgatewayandiface(&addr) == 0)
		{
			break;
		}
		sleep(1);
	}
	if(mib_get(MIB_SNMP_COMM_TRAP_ON, &var) && var == 1)
	{
		if(mib_get(MIB_SNMP_TRAP_IP_IA, trapip))
		{
			sprintf(ip, "%d.%d.%d.%d", trapip[0], trapip[1], trapip[2], trapip[3]);
			sprintf(ip2, "%d.%d.%d.%d:30162", trapip[0], trapip[1], trapip[2], trapip[3]);
			if(strcmp(ip, "0.0.0.0"))
			{
				snprintf(cmd, sizeof(cmd), msg, ip);
				system(cmd);

				snprintf(cmd2, sizeof(cmd2), msg, ip2);
				system(cmd2);
			}
		}
	}

	memset(trapip, 0, IP_ADDR_LEN);
	memset(cmd, 0, 512);

	for(i = 0; i < TRAP_WAIT_TIME; i++)
	{
		if(mib_get(trapId, trapip))
		{
			sprintf(ip, "%d.%d.%d.%d", trapip[0], trapip[1], trapip[2], trapip[3]);
			sprintf(ip2, "%d.%d.%d.%d:30162", trapip[0], trapip[1], trapip[2], trapip[3]);

			if(strcmp(ip, "0.0.0.0"))
			{
				snprintf(cmd, sizeof(cmd), msg, ip);
				system(cmd);

				snprintf(cmd2, sizeof(cmd2), msg, ip2);
				system(cmd2);
				break;
			}
		}
		sleep(1);
	}

	free(args);
	return;
}

int send_snmp_trap(char *msg, int trapId)
{
	struct ThreadArgs *args = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs));
	pthread_t trap_thread;

	if(args == NULL)
		return -1;

	memset(args, 0, sizeof(struct ThreadArgs));
	args->trap_id = trapId;

	strncpy(args->msg, msg, TRAP_MSG_SIZE - 1);
	args->msg[sizeof(args->msg) - 1] = '\0';

	if(pthread_create(&trap_thread, NULL, send_snmp_trap_thread, (void *)args) != 0)
	{
		free(args);
		return -1;
	}
	pthread_detach(trap_thread);

	return 0;
}
#endif

int convert_weekdays(char *str, char *convert)
{
	char *search;
	unsigned int days = 0;

	search = strstr(str, "Sun");
	if (search != NULL) days = 0x01;
	search = strstr(str, "Mon");
	if (search != NULL) days |= 0x2;
	search = strstr(str, "Tue");
	if (search != NULL) days |= 0x4;
	search = strstr(str, "Wed");
	if (search != NULL) days |= 0x8;
	search = strstr(str, "Thu");
	if (search != NULL) days |= 0x10;
	search = strstr(str, "Fri");
	if (search != NULL) days |= 0x20;
	search = strstr(str, "Sat");
	if (search != NULL) days |= 0x40;

	if (!(days & 0x1))
		strcat(convert, "Sun");
	if (!(days & 0x2))
	{
		if (convert[0] == 0)
			strcat(convert, "Mon");
		else
			strcat(convert, ",Mon");
	}
	if (!(days & 0x4))
	{
		if (convert[0] == 0)
			strcat(convert, "Tue");
		else
			strcat(convert, ",Tue");
	}
	if (!(days & 0x8))
	{
		if (convert[0] == 0)
			strcat(convert, "Wed");
		else
			strcat(convert, ",Wed");
	}

	if (!(days & 0x10))
	{
		if (convert[0] == 0)
			strcat(convert, "Thu");
		else
			strcat(convert, ",Thu");
	}
	if (!(days & 0x20))
	{
		if (convert[0] == 0)
			strcat(convert, "Fri");
		else
			strcat(convert, ",Fri");
	}

	if (!(days & 0x40))
	{
		if (convert[0] == 0)
			strcat(convert, "Sat");
		else
			strcat(convert, ",Sat");
	}

	return 0;
}

int convert_hours(int start, int end, int *start1, int *end1, int *start2, int *end2)
{
	if (start != 0)
	{
		*start1 = 0;
		*end1 = start;
	}
	else
	{
		*start1 = 0;
		*end1 = 0;
	}

	if (end != 23)
	{
		*start2 = end;
		*end2 = 23;
	}
	else
	{
		*start2 = 23;
		*end2 = 23;
	}

	return 0;
}

int convert_mins(int start_min, int end_min, int *from_hour, int *to_hour, int *from1_min, int *to1_min, int *from2_min, int *to2_min)
{
	if (start_min != 0)
	{
		*from1_min = 0;
		*to1_min = start_min - 1;
	}
	else
	{
		if (*from_hour != 0)
			*from_hour -= 1;
		*from1_min = 0;
		*to1_min = 59;
	}

	if (end_min != 59)
	{
		*from2_min = end_min + 1;
		*to2_min = 59;
	}
	else
	{
		if (*to_hour != 23)
			*to_hour += 1;
		*from2_min = 0;
		*to2_min = 59;
	}

	return 0;
}

#define MAX_CMD_NUM     20

void accesslimit_add_iptables_set(MIB_ACCESSLIMIT_T *Entry, char *br_ip)
{
	int start1, start2, end1, end2;
	int start1_min, start2_min, end1_min, end2_min;
	int idx;
	char cmd[MAX_CMD_NUM][256];

	memset(cmd, 0, sizeof(cmd));

	if (Entry->accessBlock == TRUE)
	{
		idx = 0;
		sprintf(cmd[idx++], "iptables -t nat -I access_limit -p tcp --dport 80 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34159",
				Entry->fromTimeHour, Entry->fromTimeMin, Entry->toTimeHour, Entry->toTimeMin, Entry->weekdays, Entry->accessMac, br_ip);
		sprintf(cmd[idx++], "iptables -t nat -I access_limit -p tcp --dport 80 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s",
				Entry->fromTimeHour, Entry->fromTimeMin, Entry->toTimeHour, Entry->toTimeMin, Entry->weekdays, br_ip, Entry->accessMac, br_ip);

		sprintf(cmd[idx++], "iptables -t nat -I access_limit -p tcp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34160",
				Entry->fromTimeHour, Entry->fromTimeMin, Entry->toTimeHour, Entry->toTimeMin, Entry->weekdays, Entry->accessMac, br_ip);
		sprintf(cmd[idx++], "iptables -t nat -I access_limit -p tcp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s",
				Entry->fromTimeHour, Entry->fromTimeMin, Entry->toTimeHour, Entry->toTimeMin, Entry->weekdays, br_ip, Entry->accessMac, br_ip);

		sprintf(cmd[idx++], "iptables -t nat -I access_limit -p udp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34160",
				Entry->fromTimeHour, Entry->fromTimeMin, Entry->toTimeHour, Entry->toTimeMin, Entry->weekdays, Entry->accessMac, br_ip);
		sprintf(cmd[idx++], "iptables -t nat -I access_limit -p udp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s",
				Entry->fromTimeHour, Entry->fromTimeMin, Entry->toTimeHour, Entry->toTimeMin, Entry->weekdays, br_ip, Entry->accessMac, br_ip);
	}
	else
	{
		idx = 0;
		convert_hours(Entry->fromTimeHour, Entry->toTimeHour, &start1, &end1, &start2, &end2);
		convert_mins(Entry->fromTimeMin, Entry->toTimeMin, &end1, &start2, &start1_min, &end1_min, &start2_min, &end2_min);

		if (!((start1 == 0) && (end1 == 0) && (start1_min == 0) && (end1_min == 0)))
		{
			sprintf(cmd[idx++], "iptables -t nat -I access_limit -p tcp --dport 80 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34159",
					start1, start1_min, end1, end1_min, Entry->weekdays, Entry->accessMac, br_ip);
			sprintf(cmd[idx++], "iptables -t nat -I access_limit -p tcp --dport 80 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s",
					start1, start1_min, end1, end1_min, Entry->weekdays, br_ip, Entry->accessMac, br_ip);

			sprintf(cmd[idx++], "iptables -t nat -I access_limit -p tcp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34160",
					start1, start1_min, end1, end1_min, Entry->weekdays, Entry->accessMac, br_ip);
			sprintf(cmd[idx++], "iptables -t nat -I access_limit -p tcp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s",
					start1, start1_min, end1, end1_min, Entry->weekdays, br_ip, Entry->accessMac, br_ip);

			sprintf(cmd[idx++], "iptables -t nat -I access_limit -p udp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34160",
					start1, start1_min, end1, end1_min, Entry->weekdays, Entry->accessMac, br_ip);
			sprintf(cmd[idx++], "iptables -t nat -I access_limit -p udp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s",
					start1, start1_min, end1, end1_min, Entry->weekdays, br_ip, Entry->accessMac, br_ip);
		}
		if (!((start2 == 0) && (end2 == 0) && (start2_min == 0) && (end2_min == 0)))
		{
			sprintf(cmd[idx++], "iptables -t nat -I access_limit -p tcp --dport 80 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34159",
					start2, start2_min, end2, end2_min, Entry->weekdays, Entry->accessMac, br_ip);
			sprintf(cmd[idx++], "iptables -t nat -I access_limit -p tcp --dport 80 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s",
					start2, start2_min, end2, end2_min, Entry->weekdays, br_ip, Entry->accessMac, br_ip);

			sprintf(cmd[idx++], "iptables -t nat -I access_limit -p tcp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34160",
					start2, start2_min, end2, end2_min, Entry->weekdays, Entry->accessMac, br_ip);
			sprintf(cmd[idx++], "iptables -t nat -I access_limit -p tcp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s",
					start2, start2_min, end2, end2_min, Entry->weekdays, br_ip, Entry->accessMac, br_ip);

			sprintf(cmd[idx++], "iptables -t nat -I access_limit -p udp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34160",
					start2, start2_min, end2, end2_min, Entry->weekdays, Entry->accessMac, br_ip);
			sprintf(cmd[idx++], "iptables -t nat -I access_limit -p udp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s",
					start2, start2_min, end2, end2_min, Entry->weekdays, br_ip, Entry->accessMac, br_ip);
		}
	}

	for (idx = 0; idx < MAX_CMD_NUM; idx++)
	{
		if (cmd[idx][0] != 0)
			system(cmd[idx]);
	}

	return;
}

int deo_accesslimit_set(void)
{
	int idx;
	int totalEntry;
	char br_ip[16];
	MIB_ACCESSLIMIT_T Entry;

	memset(br_ip, 0, sizeof(br_ip));

	getIfAddr("br0", br_ip);

	totalEntry = mib_chain_total(MIB_ACCESSLIMIT_TBL); /* get chain record size */

	for (idx = 0; idx < totalEntry; idx++)
	{
		mib_chain_get(MIB_ACCESSLIMIT_TBL, idx, (void *)&Entry);

		if (Entry.used == 1)
			accesslimit_add_iptables_set(&Entry, br_ip);
	}

	return 0;
}

int deo_accesslimit_iptables_flush(void)
{
	char *argv[6];

	argv[1] = "-t";
	argv[2] = "nat";
	argv[3] = "-F";
	argv[4] = "access_limit";
	argv[5] = NULL;

	do_nice_cmd("/bin/iptables", argv, 0 );

	return 0;
}

static void * _timer_thread(void * data);
static pthread_t g_thread_id;
static struct timer_node *g_head = NULL;

int timer_initialize()
{
	if(pthread_create(&g_thread_id, NULL, _timer_thread, NULL))
	{
		/*Thread creation failed*/
		return 0;
	}

	return 1;
}

size_t start_timer(unsigned int interval, time_handler handler, t_timer type, void * user_data)
{
	struct timer_node * new_node = NULL;
	struct itimerspec new_value;

	new_node = (struct timer_node *)malloc(sizeof(struct timer_node));
	if(new_node == NULL) return 0;

	new_node->callback  = handler;
	new_node->user_data = user_data;
	new_node->interval  = interval;
	new_node->type      = type;

	new_node->fd = timerfd_create(CLOCK_REALTIME, 0);

	if (new_node->fd == -1)
	{
		free(new_node);
		return 0;
	}

	new_value.it_value.tv_sec  = interval;
	new_value.it_value.tv_nsec = 0;

	if (type == TIMER_PERIODIC)
	{
		new_value.it_interval.tv_sec = interval;
	}
	else
	{
		new_value.it_interval.tv_sec = 0;
	}

	new_value.it_interval.tv_nsec = 0;

	timerfd_settime(new_node->fd, 0, &new_value, NULL);

	/*Inserting the timer node into the list*/
	new_node->next = g_head;
	g_head = new_node;

	return (size_t)new_node;
}

void stop_timer(size_t timer_id)
{
	struct timer_node * tmp = NULL;
	struct timer_node * node = (struct timer_node *)timer_id;

	if (node == NULL) return;

	close(node->fd);

	if(node == g_head)
	{
		g_head = g_head->next;
	}

	tmp = g_head;

	while(tmp && tmp->next != node) tmp = tmp->next;

	if(tmp && tmp->next)
	{
		tmp->next = tmp->next->next;
	}

	if(node) free(node);
}

void timer_finalize()
{
	while(g_head) stop_timer((size_t)g_head);

	pthread_cancel(g_thread_id);
	pthread_join(g_thread_id, NULL);
}

struct timer_node * _get_timer_from_fd(int fd)
{
	struct timer_node * tmp = g_head;

	while(tmp)
	{
		if(tmp->fd == fd) return tmp;

		tmp = tmp->next;
	}
	return NULL;
}

void * _timer_thread(void * data)
{
	struct pollfd ufds[MAX_TIMER_COUNT] = {{0}};
	int iMaxCount = 0;
	struct timer_node * tmp = NULL;
	int read_fds = 0, i, s;
	uint64_t exp;

	while(1)
	{
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_testcancel();
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

		iMaxCount = 0;
		tmp = g_head;

		memset(ufds, 0, sizeof(struct pollfd)*MAX_TIMER_COUNT);
		while(tmp)
		{
			ufds[iMaxCount].fd = tmp->fd;
			ufds[iMaxCount].events = POLLIN;
			iMaxCount++;

			tmp = tmp->next;
		}
		read_fds = poll(ufds, iMaxCount, 100);

		if (read_fds <= 0) continue;

		for (i = 0; i < iMaxCount; i++)
		{
			if (ufds[i].revents & POLLIN)
			{
				s = read(ufds[i].fd, &exp, sizeof(uint64_t));

				if (s != sizeof(uint64_t)) continue;

				tmp = _get_timer_from_fd(ufds[i].fd);

				if(tmp && tmp->callback) tmp->callback((size_t)tmp, tmp->user_data);
			}
		}
	}

	return NULL;
}

int get_str_by_popen(char *cmd, char *str, int size)
{
	FILE *fp = NULL;
	char line[256] = {0,};

	if ((fp = popen(cmd, "r")) != NULL) {
		while (fgets(line, sizeof(line), fp)) {
			line[strlen(line) - 1] = '\0';
			snprintf(str, size, "%s", line);
		}

		pclose(fp);
	}
	return 0;
}

#ifdef MAC_FILTER
int checkRule_macfilter(MIB_CE_MAC_FILTER_T macEntry)
{
	int total, i;
	MIB_CE_MAC_FILTER_T MacEntry;

	if ( macEntry.srcMac[0]==macEntry.dstMac[0] && macEntry.srcMac[1]==macEntry.dstMac[1] && macEntry.srcMac[2]==macEntry.dstMac[2] &&
			macEntry.srcMac[3]==macEntry.dstMac[3] && macEntry.srcMac[4]==macEntry.dstMac[4] && macEntry.srcMac[5]==macEntry.dstMac[5]   ) {
		return 0;
	}

	total = mib_chain_total(MIB_MAC_FILTER_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_MAC_FILTER_TBL, i, (void *)&MacEntry))
			return 0;

		if ( MacEntry.srcMac[0]==macEntry.srcMac[0] && MacEntry.srcMac[1]==macEntry.srcMac[1] && MacEntry.srcMac[2]==macEntry.srcMac[2] &&
				MacEntry.srcMac[3]==macEntry.srcMac[3] && MacEntry.srcMac[4]==macEntry.srcMac[4] && MacEntry.srcMac[5]==macEntry.srcMac[5] &&
				MacEntry.dstMac[0]==macEntry.dstMac[0] && MacEntry.dstMac[1]==macEntry.dstMac[1] && MacEntry.dstMac[2]==macEntry.dstMac[2] &&
				MacEntry.dstMac[3]==macEntry.dstMac[3] && MacEntry.dstMac[4]==macEntry.dstMac[4] && MacEntry.dstMac[5]==macEntry.dstMac[5] &&
				MacEntry.dir == macEntry.dir &&
				MacEntry.ifIndex == macEntry.ifIndex) {
			//printf("This ia a duplicate MacFilter Rule\n");
			return 0;
		}
	}

	return 1;
}

int getRule_macfilterIdx(MIB_CE_MAC_FILTER_T macEntry)
{
	int total, i;
	MIB_CE_MAC_FILTER_T MacEntry;

	total = mib_chain_total(MIB_MAC_FILTER_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_MAC_FILTER_TBL, i, (void *)&MacEntry))
			return -1;

		if ( MacEntry.srcMac[0]==macEntry.srcMac[0] && MacEntry.srcMac[1]==macEntry.srcMac[1] && MacEntry.srcMac[2]==macEntry.srcMac[2] &&
				MacEntry.srcMac[3]==macEntry.srcMac[3] && MacEntry.srcMac[4]==macEntry.srcMac[4] && MacEntry.srcMac[5]==macEntry.srcMac[5] &&
				MacEntry.dstMac[0]==macEntry.dstMac[0] && MacEntry.dstMac[1]==macEntry.dstMac[1] && MacEntry.dstMac[2]==macEntry.dstMac[2] &&
				MacEntry.dstMac[3]==macEntry.dstMac[3] && MacEntry.dstMac[4]==macEntry.dstMac[4] && MacEntry.dstMac[5]==macEntry.dstMac[5] &&
				MacEntry.dir == macEntry.dir &&
				MacEntry.ifIndex == macEntry.ifIndex) {
			return i;
		}
	}

	return -1;
}
#endif

int get_ConntrackCnt()
{
	int bRet = 0;
	FILE *fp;
	char cmd[] = "/proc/sys/net/netfilter/nf_conntrack_count";
	char buf[256] = {0,};

	fp = fopen(cmd, "r");
	if (fp == NULL)
		return -1;

	if (fgets(buf, 256, fp) != NULL)
	{
		bRet = atoi(buf);
	}

	fclose(fp);

	return  bRet;
}

int get_ConntrackHwCnt()
{
	int bRet = 0;
	FILE *cmd = NULL;
	char buff[8];
	size_t n;

	cmd = popen("cat /proc/fc/hw_dump/flow_statistic | grep 'Hw flow' | awk '{print $NF}'", "r");
	while ((n = fread(buff, 1, sizeof(buff) - 1, cmd)) > 0)
		buff[n] = '\0';

	bRet = atoi(buff);

	pclose(cmd);

	return  bRet;
}

int get_wan_crc_counter(void)
{
	unsigned int counter = 0;
	FILE *fp;
	char cmd[] = "/proc/deonet_cnt/deonet_wan_bip_cnt";

	fp = fopen(cmd, "r");
	if (fp == NULL)
		return -1;

	fscanf(fp, "%u", &counter);

	fclose(fp);

	return  counter;
}

int set_wan_crc_counter(unsigned int cnt)
{
	char cmd[64] = {0, };

	sprintf(cmd, "/sbin/echo %u > /proc/deonet_cnt/deonet_wan_bip_cnt", cnt);

	system(cmd);

	return  cnt;
}

#define  WAN_PORT		4
#define  LAN_PORT_1		3
#define  LAN_PORT_2		2
#define  LAN_PORT_3		1
#define  LAN_PORT_4		0

int copy_port_statistics(int idx, struct port_statistics_t *port_stat)
{
	int mib_rx_id, mib_tx_id, mib_crc_id;

	if (idx == WAN_PORT)
	{
		mib_rx_id = MIB_STAT_RX_BPS_WAN;
		mib_tx_id = MIB_STAT_TX_BPS_WAN;
		mib_crc_id = MIB_STAT_RX_CRC_WAN;
	}
	else if (idx == LAN_PORT_1)
	{
		mib_rx_id = MIB_STAT_RX_BPS_PORT1;
		mib_tx_id = MIB_STAT_TX_BPS_PORT1;
		mib_crc_id = MIB_STAT_RX_CRC_PORT1;
	}
	else if (idx == LAN_PORT_2)
	{
		mib_rx_id = MIB_STAT_RX_BPS_PORT2;
		mib_tx_id = MIB_STAT_TX_BPS_PORT2;
		mib_crc_id = MIB_STAT_RX_CRC_PORT2;
	}
	else if (idx == LAN_PORT_3)
	{
		mib_rx_id = MIB_STAT_RX_BPS_PORT3;
		mib_tx_id = MIB_STAT_TX_BPS_PORT3;
		mib_crc_id = MIB_STAT_RX_CRC_PORT3;
	}
	else if (idx == LAN_PORT_4)
	{
		mib_rx_id = MIB_STAT_RX_BPS_PORT4;
		mib_tx_id = MIB_STAT_TX_BPS_PORT4;
		mib_crc_id = MIB_STAT_RX_CRC_PORT4;
	}

	if (!mib_set(mib_rx_id, (void *)&(port_stat->bytesReceived)))
		return -1;

	if (!mib_set(mib_tx_id, (void *)&(port_stat->bytesSent)))
		return -1;

	if (!mib_set(mib_crc_id, (void *)&(port_stat->crc)))
		return -1;

	return 0;
}

void deo_port_statistics_get(int idx, struct port_statistics_t *port_stat)
{
	int mib_rx_id, mib_tx_id, mib_crc_id;

	if (idx == WAN_PORT)
	{
		mib_rx_id = MIB_STAT_RX_BPS_WAN;
		mib_tx_id = MIB_STAT_TX_BPS_WAN;
		mib_crc_id = MIB_STAT_RX_CRC_WAN;
	}
	else if (idx == LAN_PORT_1)
	{
		mib_rx_id = MIB_STAT_RX_BPS_PORT1;
		mib_tx_id = MIB_STAT_TX_BPS_PORT1;
		mib_crc_id = MIB_STAT_RX_CRC_PORT1;
	}
	else if (idx == LAN_PORT_2)
	{
		mib_rx_id = MIB_STAT_RX_BPS_PORT2;
		mib_tx_id = MIB_STAT_TX_BPS_PORT2;
		mib_crc_id = MIB_STAT_RX_CRC_PORT2;
	}
	else if (idx == LAN_PORT_3)
	{
		mib_rx_id = MIB_STAT_RX_BPS_PORT3;
		mib_tx_id = MIB_STAT_TX_BPS_PORT3;
		mib_crc_id = MIB_STAT_RX_CRC_PORT3;
	}
	else if (idx == LAN_PORT_4)
	{
		mib_rx_id = MIB_STAT_RX_BPS_PORT4;
		mib_tx_id = MIB_STAT_TX_BPS_PORT4;
		mib_crc_id = MIB_STAT_RX_CRC_PORT4;
	}

	mib_get(mib_rx_id, (void *)&port_stat->bytesReceived);
	mib_get(mib_tx_id, (void *)&port_stat->bytesSent);
	mib_get(mib_crc_id, (void *)&port_stat->crc);

	return;
}

char *strip_ffff_in_ip(char *address)
{
	if (strncmp(address, "::ffff:", 7) == 0)
	{
		return address + 7;
	}
	else
	{
		return address;
	}
}

void strup(char *str)
{
	int i = 0;

	while (str[i]) {
		if (str[i] >= 'a' && str[i] <= 'z'){
			str[i] = str[i] - 32;
		}
		i++;
	}
}

void deo_change_with_commas(unsigned long long n, char *value)
{
	int len;
	int i;
	char buffer[50]; /* 문자열을 저장할 버퍼 */
	char tmp_buf[2]; /* 문자열을 저장할 버퍼 */
	char* digits; /* 문자열 포인터 */

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%llu", n);

	len = strlen(buffer); /* 문자열 길이 계산 */
	digits = buffer; /* 포인터에 버퍼 주소 할당 */

	/* 문자열을 역순으로 출력하며 천 단위마다 콤마를 추가 */
	for (i = len; i > 0;)
	{
		sprintf(tmp_buf, "%c", *digits++);
		strcat(value, tmp_buf);

		i--;
		if (i > 0 && (i % 3) == 0)
			strcat(value, ",");
	}

	return;
}

int isValidPingAddressValue(char *ipaddr)
{
	int i = 0;
	unsigned char c;

	for (i = 0; i < strlen(ipaddr); i++)
	{
		c = ipaddr[i];

		if ((c>='0' && c<='9')||(c == '.')||(c>='a' && c<='z')||(c>='A' && c<='Z'))
			continue;
		else
			return -1;
	}

	return 0;
}

int isValidPing6AddressValue(char *ipaddr)
{
	int i = 0;
	unsigned char c;

	for (i = 0; i < strlen(ipaddr); i++)
	{
		c = ipaddr[i];

		if ((c>='0' && c<='9')||(c == ':')||(c == '.')||(c>='a' && c<='z')||(c>='A' && c<='Z'))
			continue;
		else
			return -1;
	}

	return 0;
}

int igmp_restart_get_mode()
{
	char line[64] = {0};
	int enable= 0;
	FILE *fp;

	fp = fopen(IGMP_RESTART_PROC_FILE, "r");
	if (fp != NULL) {
		if (fgets(line, sizeof(line), fp) != NULL) {
			enable = line[0] - '0';
		}

		fclose(fp);
	}

	return enable;
}
/* ********************************************
 * protocol entryption
 * *********************************************/

int get_encrypt_version_info(unsigned char *version, int size)
{
	int status = 0, defaultKeyLen = 0, i = 0;
	unsigned char iv[1024] = {0,};
	unsigned char defaultKey[1024] = {0,};
	unsigned int tmp[PROTOCOL_ENCRYPT_VERSION_LEN] = {0,};

	if(version == NULL)
		return -1;

	if(size < PROTOCOL_ENCRYPT_VERSION_LEN)
		return -1;

	mib_get(MIB_KEY_EXCHANGE_STATUS, &status);
	mib_get(CWMP_PARAMETERKEY, defaultKey);
	defaultKeyLen = strlen(defaultKey);

	if(status == KEY_EXCHANGE_SUCCESS)
	{   
		mib_get(CWMP_RB_COMMANDKEY, iv);
	}
	else if(status == KEY_EXCHANGE_FAIL && defaultKeyLen != 0)
	{   
		if (access("/var/run/nonedefkey", F_OK) == 0)
			mib_get(CWMP_RB_COMMANDKEY, iv);
		else
			mib_get(CWMP_DL_COMMANDKEY, iv);
	}
	else
		return -1;

	sscanf(iv, "%u-%u-%u-%u", &tmp[0], &tmp[1], &tmp[2], &tmp[3]);

	for(i = 0; i < PROTOCOL_ENCRYPT_VERSION_LEN; i++)
	{
		if(tmp[i] > 255)
			return -1;
		version[i] = (unsigned char)tmp[i];
	}
	return 0;
}

int generate_protocol_encrypt_iv(char *out, size_t len)
{
	static const char alphanum[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789";
	const size_t alphalen = sizeof(alphanum) - 1; // 널문자 제외 길이
	unsigned char rnd;
	int i = 0;

	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		return -1;

	for (i = 0; i < len; i++) {
		if (read(fd, &rnd, 1) != 1) {
			close(fd);
			return -1;
		}
		out[i]= alphanum[rnd % alphalen];
	}
	out[len] = '\0';

	close(fd);
	return 0;
}

int get_encrypt_IV_data(unsigned char *iv_data, int size)
{
	int status = 0, defaultKeyLen = 0;
	unsigned char iv[1024] = {0,};
	unsigned char defaultKey[1024] = {0,};

	if(iv_data == NULL)
		return -1;

	if(size < PROTOCOL_ENCRYPT_IV_LEN)
		return -1;

	mib_get(MIB_KEY_EXCHANGE_STATUS, &status);
	mib_get(CWMP_PARAMETERKEY, defaultKey);
	defaultKeyLen = strlen(defaultKey);

	if(status == KEY_EXCHANGE_SUCCESS)
	{
		if(generate_protocol_encrypt_iv(iv, PROTOCOL_ENCRYPT_IV_LEN) < 0)
			return -1;
		memcpy(iv_data, iv, PROTOCOL_ENCRYPT_IV_LEN);
		return 0;
	}
	else if(status == KEY_EXCHANGE_FAIL && defaultKeyLen != 0)
	{
		if(generate_protocol_encrypt_iv(iv, PROTOCOL_ENCRYPT_IV_LEN) < 0)
			return -1;
		memcpy(iv_data, iv, PROTOCOL_ENCRYPT_IV_LEN);
		return 0;
	}
	return -1;
}

int get_encrypt_aes256key(unsigned char *key, int size)
{
	int i = 0;
	int status = 0, defaultKeyLen = 0;
	unsigned char mibkey[1024] = {0,};
	unsigned char defaultKey[1024] = {0,};

	if(key == NULL)
		return -1;

	if(size < PROTOCOL_ENCRYPT_KEY_LEN)
		return -1;

	mib_get(MIB_KEY_EXCHANGE_STATUS, &status);
	mib_get(CWMP_PARAMETERKEY, defaultKey);
	defaultKeyLen = strlen(defaultKey);

	if(status == KEY_EXCHANGE_SUCCESS)
	{
		mib_get(CWMP_SI_COMMANDKEY, mibkey);
		memcpy(key, mibkey, PROTOCOL_ENCRYPT_KEY_LEN);
		return 0;
	}
	else if(status == KEY_EXCHANGE_FAIL && defaultKeyLen != 0)
	{
		if (access("/var/run/nonedefkey", F_OK) == 0)
			mib_get(CWMP_SI_COMMANDKEY, mibkey);
		else
			mib_get(CWMP_PARAMETERKEY, mibkey);
		memcpy(key, mibkey, PROTOCOL_ENCRYPT_KEY_LEN);
		return 0;
	}
	return -1;
}

typedef struct {
	char encrypt_key[PROTOCOL_ENCRYPT_KEY_LEN + 1]; 
	char iv[PROTOCOL_ENCRYPT_IV_LEN + 1];
	unsigned char key_index[PROTOCOL_ENCRYPT_VERSION_LEN];
} NonedefKeyEntry;

/*
 * 한 줄의 문자열을 받아서 파싱하는 함수
 * 입력 형식: "encryptkey|iv|keyindex" 예) "npqhoey2B8m1kki9ndft9ncJ5bn5Wpse|ivkegen12345|0-0-10-8"
 * 리턴값: 성공 시 0, 실패 시 -1를 반환
 */
int parse_nonedefkey_line(const char* line, NonedefKeyEntry* entry) {
	char buffer[1024];
	char *saveptr1 = NULL, *saveptr2 = NULL;
	int count = 0; 
	char *token = NULL, *keyToken = NULL;

	strncpy(buffer, line, sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = '\0';

	// '|' 구분자로 토큰 분리 (strtok_r 사용)
	token = strtok_r(buffer, "|", &saveptr1);
	if (!token) return -1;
	strncpy(entry->encrypt_key, token, PROTOCOL_ENCRYPT_KEY_LEN);
	entry->encrypt_key[PROTOCOL_ENCRYPT_KEY_LEN] = '\0';

	token = strtok_r(NULL, "|", &saveptr1);
	if (!token) return -1;
	strncpy(entry->iv, token, PROTOCOL_ENCRYPT_IV_LEN);
	entry->iv[PROTOCOL_ENCRYPT_IV_LEN] = '\0';

	token = strtok_r(NULL, "|", &saveptr1);
	if (!token) return -1;

	keyToken = strtok_r(token, "-", &saveptr2);
	while(keyToken != NULL && count < 4) {
		int value = atoi(keyToken);
		if (value < 0 || value > 255) {                                                                               
			fprintf(stderr, "오류: key index 값이 0~255 범위를 벗어남: %d\n", value);                                 
			return -1;
		}
		entry->key_index[count] = (unsigned char) value;
		count++;
		keyToken = strtok_r(NULL, "-", &saveptr2);
	}
	if (count != 4) {
		fprintf(stderr, "오류: key index 형식이 올바르지 않음 (4개 숫자 필요): %s\n", token);
		return -1;
	}
	return 0;
}
						

NonedefKeyEntry* parse_nonedefkey_data(const char* data, size_t* out_count) {
	char *data_copy = NULL;
	char *line = NULL;
	char *saveptr = NULL;
	size_t index = 0;
	size_t count = 0;
	char *p = NULL;

	if (data == NULL || out_count == NULL) return NULL;

	data_copy = strdup(data);
	if (data_copy == NULL) return NULL;

	for (p = data_copy; *p; p++) {
		if (*p == '\n') count++;
	}
	if (strlen(data_copy) > 0 && data_copy[strlen(data_copy) - 1] != '\n') {
		count++;
	}

	NonedefKeyEntry* entries = (NonedefKeyEntry*) malloc(count * sizeof(NonedefKeyEntry));
	if (entries == NULL) {
		free(data_copy);
		return NULL;
	}
	memset(entries, 0, sizeof(NonedefKeyEntry) * count);

	line = strtok_r(data_copy, "\n", &saveptr);
	while (line != NULL) {
		if (strlen(line) > 0) {
			if (parse_nonedefkey_line(line, &entries[index]) == 0) {
				index++;
			} else {
				fprintf(stderr, "파싱 오류 발생: %s\n", line);
			}
		}
		line = strtok_r(NULL, "\n", &saveptr);
	}

	*out_count = index;
	free(data_copy);
	return entries;
}	
											   
int get_iv_key_by_version(unsigned char *version, char *key, char *iv) 
{
	char *filename = "/var/run/nonedefkey";
	char *decrypted_content = keytable_get_decrypted_content(filename);
	size_t entry_count = 0;
	int i = 0, status;

	mib_get(MIB_KEY_EXCHANGE_STATUS, &status);
	if(status == KEY_EXCHANGE_FAIL)
		filename = "/config/defkey";

	decrypted_content = keytable_get_decrypted_content(filename);

	if(decrypted_content == NULL)
	{
		return -1;
	}

	NonedefKeyEntry* entries = parse_nonedefkey_data(decrypted_content, &entry_count);
	if (entries == NULL) {
		free(decrypted_content);
		return -1;
	}

#if 0
	for (i = 0; i < entry_count; i++) {
		printf("Entry %zu:\n", i + 1);
		printf("  Encrypt Key: %s\n", entries[i].encrypt_key);
		printf("  IV: %s\n", entries[i].iv);
		printf("  Key Index Bytes: %u %u %u %u\n", 
				entries[i].key_index[0], 
				entries[i].key_index[1], 
				entries[i].key_index[2], 
				entries[i].key_index[3]);
		printf("-------------------------------------\n");
	}
#endif
	for (i = 0; i < entry_count; i++) {
		if(!memcmp(version, entries[i].key_index, 4))
		{
			memcpy(key, entries[i].encrypt_key, PROTOCOL_ENCRYPT_KEY_LEN);	
			memcpy(iv, entries[i].iv, PROTOCOL_ENCRYPT_IV_LEN);
		}
	}

	free(entries);
	free(decrypted_content);
	return 0;
}

/* ********************************************
 * protocol entryption end
 * *********************************************/

/* ********************************************
 * protocol entryption TRAP
 * *********************************************/
#define RETRY_COUNT 5
#define RETRY_DELAY_SEC 1

int send_message_to_trapprocess(const char *message, int trapIdx) {
	MyMessage msg;
	mqd_t mq = (mqd_t)-1;
	int retries = 0;

	memset(&msg, 0, sizeof(MyMessage));
	strncpy(msg.text, message, MAX_TEXT_SIZE -2);
	msg.trap_number = trapIdx;

	// (1) 재시도 로직: 최대 RETRY_COUNT번
	while (retries < RETRY_COUNT) {
		mq = mq_open(QUEUE_NAME, O_WRONLY);
		if (mq != (mqd_t)-1) {
			break;
		} else {
			if (errno == ENOENT) {
				fprintf(stderr, "[Sender] Queue not found, retrying...\n");
				sleep(RETRY_DELAY_SEC);
				retries++;
				continue;
			} else {
				perror("[Sender] mq_open");
				return -1;
			}
		}
	}

	if (mq == (mqd_t)-1) {
		fprintf(stderr, "[Sender] Failed to open queue after %d retries.\n", RETRY_COUNT);
		return -1;
	}

	if (mq_send(mq, (const char*)&msg, sizeof(msg), 0) == -1) {
		perror("[Sender] mq_send");
		mq_close(mq);
		return -1;
	}

	if (mq_close(mq) == -1) {
		perror("[Sender] mq_close");
		return -1;
	}

	return 0;
}
/* ********************************************
 * protocol entryption TRAP end
 * *********************************************/

/* ********************************************
 * protocol entryption SM
 * *********************************************/
static void changeState(KeyExchangeSM *sm, SMState newState) {
	sm->currentState = newState;

	switch (newState) {
		case SM_INIT:
			if (sm->callbacks.onEnterInit) {
				sm->callbacks.onEnterInit(sm);
			}
			break;
		case SM_FAIL_ENCRYPT:
			if (sm->callbacks.onEnterFailEncrypt) {
				sm->callbacks.onEnterFailEncrypt(sm);
			}
			break;
		case SM_FAIL_NORMAL:
			if (sm->callbacks.onEnterFailNormal) {
				sm->callbacks.onEnterFailNormal(sm);
			}
			break;
		case SM_SUCCESS:
			if (sm->callbacks.onEnterSuccess) {
				sm->callbacks.onEnterSuccess(sm);
			}
			break;
		default:
			// 정의되지 않은 상태
			break;
	}
}

void KeyExchangeSM_Init(KeyExchangeSM *sm,
		const KeyExchangeCallbacks *callbacks,
		char  hasDefaultKey,
		char  secMode)
{
	memset(sm, 0, sizeof(*sm));

	sm->currentState   = SM_INIT;
	sm->currentKeyExST = KEY_EXCHANGE_INIT;
	sm->isFirstSuccess = 0;

	sm->hasDefaultKey  = hasDefaultKey;
	sm->secMode		   = secMode;
	sm->ldapCnt		   = 0;

	if (callbacks) {
		sm->callbacks = *callbacks;
	}

	if (sm->callbacks.onEnterInit) {
		sm->callbacks.onEnterInit(sm);
	}
}

int check_kfailcnt(const char *filepath) {
	FILE *fp = fopen(filepath, "r");
	int count = 0;

	if (!fp) {
		return 1;
	}

	if (fscanf(fp, "%d", &count) != 1) {
		fclose(fp);
		return 0;
	}
	fclose(fp);

	return (count >= 2);
}

int isDnsLookUp(void)
{
#define PROTOCOL_ENCRYPT_DNS_CHK_URL            "apldap.skbroadband.com"
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;     // IPv4, IPv6 모두 허용
	hints.ai_socktype = SOCK_STREAM; // TCP 연결용 (lookup만 사용)

	int ret = getaddrinfo(PROTOCOL_ENCRYPT_DNS_CHK_URL, NULL, &hints, &res);
	if (ret != 0) {
		return 0;
	}

	freeaddrinfo(res);
	return 1;
}

void KeyExchangeSM_UpdateKeyExchangeST(KeyExchangeSM *sm, KeyExchangeST newST)
{
	int ldapSuccess = 0;
	const char *kfailcntFile = "/tmp/kfailcnt";
	
	if(mib_get(MIB_LDAP_SUCCESS, &ldapSuccess))
	{
		if(ldapSuccess == 0)	
			return;
	}
	else
		return;

	if(sm->ldapCnt < 2)
	{
		sm->ldapCnt++;
		return;
	}

	if(sm->currentKeyExST == KEY_EXCHANGE_INIT 
			&& !isDnsLookUp()) {
		return;
	}
	if (sm->currentKeyExST == newST) {
		return;
	}
	
	switch (newST) {
		case KEY_EXCHANGE_SUCCESS:
			changeState(sm, SM_SUCCESS);

			if (!sm->isFirstSuccess) {
				sm->isFirstSuccess = 1;
			}
			break;

		case KEY_EXCHANGE_FAIL:
			if (sm->hasDefaultKey) {
				changeState(sm, SM_FAIL_ENCRYPT);
			} else {
				/* TODO : 키연동 실패 횟수가 2번이상이면 상태변경 */
				/* sm->secMode == SEC_ENCRYPT_NORMAL */
				if(sm->secMode == SEC_ENCRYPT_NORMAL)
				{
					if (check_kfailcnt(kfailcntFile))
						changeState(sm, SM_FAIL_NORMAL);
					else
						return;
				}
				else
					changeState(sm, SM_FAIL_NORMAL);

			}
			break;

		default:
			printf("Unknown KeyExchangeST: %d\n", newST);
			return;
	}
	sm->currentKeyExST = newST;

}

void KeyExchangeSM_ProcessEvent(KeyExchangeSM *sm, const SMEvent *evt)
{
	if (sm->callbacks.processEventInCurrentState) {
		sm->callbacks.processEventInCurrentState(sm, evt);
	}
}
		
/* ********************************************
 * protocol entryption SM end
 * *********************************************/
int keytable_write(char *filename, char *data) 
{
	FILE *fp = fopen(filename, "w");

	if (fp == NULL)
		return -1;
	
	if (fputs(data, fp) == EOF) {
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;
}

char *keytable_read_file(char *filename) 
{
	FILE *fp = fopen(filename, "r");
	long file_size;
	char *buffer;

	if (fp == NULL) 
		return NULL;
	
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	buffer = (char*)malloc(file_size + 1);
	if (buffer == NULL) {
		fclose(fp);
		return NULL;
	}

	if (fread(buffer, 1, file_size, fp) != file_size) {
		fclose(fp);
		free(buffer);
		return NULL;
	}

	buffer[file_size] = '\0';
	fclose(fp);
	return buffer;
}

void keytable_xor_crypt(char *data, size_t length) 
{
	char key = 0xAA;
	size_t i;

	for (i = 0; i < length; i++) {
		data[i] = data[i] ^ key;
	}
}

int keytable_encrypt_file(char *filename) 
{
	char *content = keytable_read_file(filename);
	size_t len;

	if (!content) 
		return -1;

	len = strlen(content);
	keytable_xor_crypt(content, len);

	if (keytable_write(filename, content) == -1) {
		free(content);
		return -1;
	}

	free(content);
	return 0;
}

int keytable_decrypt_file(char *filename) 
{
	char *content = keytable_read_file(filename);
	size_t len;

	if (!content) 
		return -1;

	len = strlen(content);
	keytable_xor_crypt(content, len);

	if (keytable_write(filename, content) == -1) {
		free(content);
		return -1;
	}

	free(content);
	return 0;
}

char *keytable_get_decrypted_content(char *filename) 
{
	char *encrypted_content = keytable_read_file(filename);
	char *decrypted_content = NULL;
	size_t len;

	if (!encrypted_content)
		return NULL;

	len = strlen(encrypted_content);

	decrypted_content = (char *)malloc(len + 1);
	if (!decrypted_content) {
		free(encrypted_content);
		return NULL;
	}

	strcpy(decrypted_content, encrypted_content);
	keytable_xor_crypt(decrypted_content, len);

	free(encrypted_content);
	return decrypted_content;
}

void stop_keyupdate()
{
	FILE *fp;
	char buffer[1024];
	int pid = -1;

	fp = popen("ps -ef | grep '/bin/requestkey' | grep -v grep | awk '{print $2}'", "r");
	if (fp != NULL) {
		memset(buffer, 0, sizeof(buffer));
		if (fgets(buffer, sizeof(buffer), fp) != NULL) {
			pid = atoi(buffer);
		}

		pclose(fp);
	}

	if (pid > 0)
		kill(pid, SIGTERM); 
}


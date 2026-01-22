/*
 *	option.h
 */


#ifndef INCLUDE_OPTIONS_H
#define INCLUDE_OPTIONS_H
#undef DNS_QUERY_WITH_DIFFERENT_SPORT

#if defined(EMBED) || defined(__KERNEL__)
#include <linux/config.h>
#else
#include "../../../../include/linux/autoconf.h"
#endif

#ifdef EMBED
#include <config/autoconf.h>
#else
#include "../../../../config/autoconf.h"
#endif

#ifdef CONFIG_RTL8852AE_BACKPORTS
#ifdef EMBED
#include <backport/autoconf.h>
#else
#include "../../../../include/backport/autoconf.h"
#endif
#endif

//jiunming, redirect the web page to specific page only once per pc for wired router
#undef WEB_REDIRECT_BY_MAC

//alex
#undef CONFIG_USBCLIENT
//jim for power_led behavior according to TR068..
#undef WLAN_BUTTON_LED
// try to restore the user setting when config checking error
#undef KEEP_CRITICAL_HW_SETTING
#undef KEEP_CRITICAL_CURRENT_SETTING

#ifndef CONFIG_LUNA_FIRMWARE_UPGRADE_SUPPORT
//ql--if image header error, dont reboot the system.
#define ENABLE_SIGNATURE_ADV
#undef ENABLE_SIGNATURE
#ifdef ENABLE_SIGNATURE
#define SIGNATURE	""
#endif
#endif

#ifdef CONFIG_00R0
#define FWU_VERIFY_CHECKSUM	//Need openssl
#define FWU_VERIFY_HWVER
#endif

#ifdef CONFIG_TLKM
#define _PRMT_X_TRUE_SECURITY_
#define USE_BASE64_PASSWORD_ENCRYPT
#endif

#ifdef CONFIG_TELMEX_DEV
#define CWMP_USE_DEFAULT_ACS_SETTING			1
#define _PRMT_X_CT_SUPPER_DHCP_LEASE_SC
#define FIREWALL_SECURITY
#ifdef CONFIG_USER_CWMP_TR069
//#define USE_BUSYBOX_KLOGD
#define CTCOM_NAME_PREFIX "TELMEX_COM"
#endif

#define _PRMT_X_TELMEX_ACCESSPOINT_ACCOUNT_
#define _PRMT_WIFIACCESSPOINT_WPS_VAP_
#endif

//ql-- limit upstream traffic
#undef UPSTREAM_TRAFFIC_CTL

//august: NEW_PORTMAPPING is used in user space
#ifdef CONFIG_USER_PORTMAPPING
#define NEW_PORTMAPPING
#endif

#ifdef CONFIG_BRIDGE_VLAN_FILTERING
#define SUPPORT_BRIDGE_VLAN_FILTER_PORTBIND
#endif

#define INCLUDE_DEFAULT_VALUE		1

#ifdef CONFIG_USER_UDHCP099PRE2
#define COMBINE_DHCPD_DHCRELAY
#endif

#ifdef CONFIG_00R0
/* VOIP WAN has IPHost1 and Internet WAN has IPHost2.
 * If IPHost 1 is inaccessible than test call should be available using SIP
 * telephone and in-built SIP client. DNS queries and other unrelated packets
 * are sent to IPHost2.
 */
#define VOIP_DNS_GET_THROUGH_ANY_INTF
#endif

#define APPLY_CHANGE
#define E8B_GET_OUI

//#define SECONDARY_IP
//star: for set acl ip range
#if defined(CONFIG_GENERAL_WEB) && !defined(CONFIG_00R0)
#define ACL_IP_RANGE
#endif
//star: for layer7 filter
#undef LAYER7_FILTER_SUPPORT

#if defined(CONFIG_USER_DHCPV6_ISC_DHCP411) || defined(CONFIG_USER_DHCPV6_ISC_DHCP441)
#define DHCPV6_ISC_DHCP_4XX	1
#endif

#ifdef CONFIG_YUEME
#define PPP_DELAY_UP
#ifdef PPP_DELAY_UP
#define PPP_UP_MAX_DELAY 2
#endif
// Define the specail DHCPD option 125 for CTC yumme test
#define CTC_YUNMESTB_DHCPD_DHCPOPTION	1
//#ifdef CONFIG_USER_DBUS_PROXY_VERSION_3_0
#define YUEME_3_0_SPEC					1
#define YUEME_3_0_SPEC_SSID_ALIAS		1
#define YUEME_4_0_SPEC					1
#define YUEME_DHCPV6_SERVER_PD_SUPPORT	1
//#endif
#define SUPPORT_CLOUD_VR_SERVICE		1
#define CONFIG_QOS_SUPPORT_DOWNSTREAM 		1
#define CONFIG_QOS_SUPPORT_PROTOCOL_LIST 	1 //i.e. more protocol than TCP/UDP, ex. TCP,UDP,ICMP,RTP in one QoS entry
#define SUPPORT_NON_SESSION_IPOE 		1

#endif

#if defined(YUEME_4_0_SPEC) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#define VPN_MULTIPLE_RULES_BY_FILE          1 //This feature will make VPN route rule be set by ipset instead of iptables.
#endif

#ifdef CONFIG_E8B
#ifdef CONFIG_RTK_DEV_AP
#define CTC_SKIP_CHECK_APPLICATIONTYPE_NUM			1
#endif
#endif

#ifdef CONFIG_USER_SNTPV6
#define NTP_SUPPORT_V6		1
#endif

#ifdef CONFIG_RESTRICT_PROCESS_PERMISSION
#define RESTRICT_PROCESS_PERMISSION 1
#endif

#if defined(CONFIG_USER_WIRELESS_TOOLS) || defined(RTK_MULTI_AP_ETH_ONLY)

#if !defined(RTK_MULTI_AP_ETH_ONLY)
#define WLAN_SUPPORT			1
#endif
#define WLAN_WPA			1
#ifdef CONFIG_USER_WIRELESS_WDS
#define WLAN_WDS			1
#endif
#define WLAN_1x				1
#ifdef WLAN_1x
#define WLAN_1x_WPA2_WPA3_ONLY	1
#endif
#define WLAN_RADIUS_2SET	1
#define WLAN_ACL			1
#ifdef CONFIG_RTK_DEV_AP
#define WLAN_VAP_ACL			1
#endif
#undef WIFI_TEST
#if defined(CONFIG_USER_WLAN_QOS) || defined(RTK_MULTI_AP_ETH_ONLY)
#define WLAN_QoS
#endif
#if defined(CONFIG_USER_WIRELESS_MBSSID) || defined(RTK_MULTI_AP_ETH_ONLY)
#define WLAN_MBSSID			1
#endif

#if 1//!defined(CONFIG_RTK_DEV_AP)
#if defined(CONFIG_RTL8192CD) || defined(CONFIG_RTL8192CD_MODULE) || defined(CONFIG_RTL8852AE_BACKPORTS) || \
	defined(CONFIG_RTL8852AE) || defined(CONFIG_RTL8852AE_MODULE) || \
	defined(CONFIG_RTLWIFI6) || defined (CONFIG_RTLWIFI6_MODULE)
//after rtl8192cd v4.0.8 & g6_wifi_driver v1.1.3
#define WLAN_USE_DRVCOMMON_HEADER	1
#endif
#define WLAN_DRVCOMMON_HEADER		1
#endif

#if defined(CONFIG_RTL_CLIENT_MODE_SUPPORT) || (defined(CONFIG_RTW_CLIENT_MODE_SUPPORT) && defined(CONFIG_USER_WPAS))
#ifndef CONFIG_TELMEX_DEV
#define WLAN_CLIENT
#endif
#endif

#if defined(CONFIG_RTW_REPEATER_MODE_SUPPORT) && defined(CONFIG_USER_WPAS)
#ifndef CONFIG_RTL_REPEATER_MODE_SUPPORT
#define CONFIG_RTL_REPEATER_MODE_SUPPORT
#endif
#endif

#if defined(CONFIG_RTL_REPEATER_MODE_SUPPORT) || (defined(CONFIG_RTW_REPEATER_MODE_SUPPORT) && defined(CONFIG_USER_WPAS))
#define WLAN_UNIVERSAL_REPEATER
#endif


#if defined (CONFIG_RTLWIFI6_MODULE) || defined(CONFIG_RTLWIFI6)
#define WLAN_WPA3_ENTERPRISE
#endif

#ifdef CONFIG_RTL8852CE
#define WLAN_SUPPPORT_160M
#endif

#ifdef CONFIG_RTK_DEV_AP
#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
#define	WLAN_SITE_SURVEY_SUPPORT
#if defined(WLAN_UNIVERSAL_REPEATER)
#define WLAN_SITE_SURVEY_SIMUL 	//site survey at the same time
#endif
#endif

#ifndef TIMER_TASK_SUPPORT
#define TIMER_TASK_SUPPORT
#endif
#if defined(CONFIG_RTL8852AE_BACKPORTS) || defined(CONFIG_RTLWIFI6) || defined(CONFIG_RTLWIFI6_MODULE)
#define IS_AX_SUPPORT 1
#else
#undef IS_AX_SUPPORT
#endif

#if defined(CONFIG_RTL8852AE_BACKPORTS) || defined(CONFIG_RTLWIFI6) || defined(CONFIG_RTLWIFI6_MODULE)
#define WIFI5_WIFI6_COMP
#endif
#endif //CONFIG_RTK_DEV_AP

#ifdef CONFIG_USER_WIRELESS_TOOLS_RTL8185_IAPP
#define CONFIG_IAPP_SUPPORT
#endif

#if !defined(CONFIG_RTK_DEV_AP)
#if (defined(CONFIG_RTL8852AE_BACKPORTS) && (defined(CONFIG_RTL8192CD_MODULE) || defined(CONFIG_RTL8192CD))) || \
	((defined(CONFIG_RTLWIFI6_MODULE) || defined(CONFIG_RTLWIFI6)) && (defined(CONFIG_RTL8192CD_MODULE) || defined(CONFIG_RTL8192CD)))
#define WIFI5_WIFI6_COMP 1
#if (defined(CONFIG_USE_PCIE_SLOT_0) && defined(CONFIG_USE_PCIE_SLOT_1))
#define WIFI5_WIFI6_RUNTIME 1
#endif
#ifndef CONFIG_AX1500
#define CONFIG_AX1500 1
#endif
#ifdef CONFIG_YUEME
#ifdef WLAN_CLIENT
#undef WLAN_CLIENT
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
#undef WLAN_UNIVERSAL_REPEATER
#endif
#endif //CONFIG_YUEME
#ifdef CONFIG_CMCC
#ifdef WLAN_UNIVERSAL_REPEATER
#undef WLAN_UNIVERSAL_REPEATER
#endif
#endif //CONFIG_CMCC
#define WLAN_NOT_USE_MIBSYNC	1
#endif
#endif //!defined(CONFIG_RTK_DEV_AP)

//#define WLAN_FAST_INIT

#ifdef CONFIG_USER_WLAN_QCSAPI
#define WLAN_QTN
#define WLAN1_QTN
#endif

#if (defined(CONFIG_USE_PCIE_SLOT_0) && defined(CONFIG_USE_PCIE_SLOT_1)) || \
	(defined(CONFIG_WLAN_HAL_8198F) && defined(CONFIG_USE_PCIE_SLOT_0)) || \
	(defined(WLAN1_QTN)) || \
	(defined(CONFIG_RTL8852AE_BACKPORTS)) || \
	defined(CONFIG_RTL8852AE_MODULE) || defined(CONFIG_RTL8852AE) || defined(CONFIG_RTLWIFI6) || defined(CONFIG_RTLWIFI6_MODULE) || \
	defined(RTK_MULTI_AP_ETH_ONLY)
#define WLAN_DUALBAND_CONCURRENT 1
#endif

#if defined(CONFIG_RTL_TRIBAND_SUPPORT) && !defined(CONFIG_USB_AS_WLAN1)
#define TRIBAND_SUPPORT
#define TRIBAND_WPS
#endif /* defined(CONFIG_RTL_TRIBAND_SUPPORT) */

#ifdef WLAN_DUALBAND_CONCURRENT
#ifdef WLAN1_QTN
#define BAND_2G_ON_WLAN0	1
#elif defined(CONFIG_DUALBAND_CONCURRENT) //dual-linux
#if defined(CONFIG_WLAN0_5G_WLAN1_2G)
#define BAND_5G_ON_WLAN0	1
#elif defined(CONFIG_WLAN0_2G_WLAN1_5G)
#define BAND_2G_ON_WLAN0	1
#endif
#else
#if defined(CONFIG_RTL8852AE_BACKPORTS) || defined(CONFIG_RTL8852AE_MODULE) || defined(CONFIG_RTL8852AE)|| defined(CONFIG_RTLWIFI6) || defined(CONFIG_RTLWIFI6_MODULE)
#if defined(CONFIG_RTL8192CD_MODULE) && !defined(WIFI5_WIFI6_RUNTIME) && !defined(CONFIG_RTK_DEV_AP)
#define BAND_5G_ON_WLAN0	1
#ifndef CONFIG_BAND_5G_ON_WLAN0
#define CONFIG_BAND_5G_ON_WLAN0
#endif
#elif defined(CONFIG_2G_ON_WLAN0)
#define BAND_2G_ON_WLAN0	1
#ifndef CONFIG_BAND_2G_ON_WLAN0
#define CONFIG_BAND_2G_ON_WLAN0
#endif
#else
#define BAND_5G_ON_WLAN0	1
#ifndef CONFIG_BAND_5G_ON_WLAN0
#define CONFIG_BAND_5G_ON_WLAN0
#endif
#endif
#elif defined(CONFIG_BAND_5G_ON_WLAN0)
#define BAND_5G_ON_WLAN0	1
#elif defined(CONFIG_BAND_2G_ON_WLAN0)
#define BAND_2G_ON_WLAN0	1
#else
#define BAND_5G_ON_WLAN0	1
#endif
#endif
#endif

#if !defined (CONFIG_RTK_DEV_AP)
#if defined(WLAN_DUALBAND_CONCURRENT) && !defined(TRIBAND_SUPPORT)
#ifdef BAND_2G_ON_WLAN0
#define SWAP_HW_WLAN_MIB_INDEX  1
#endif
#endif
#endif // !defined (CONFIG_RTK_DEV_AP)

#if (defined(CONFIG_RTL_8812_SUPPORT)|| \
     defined(CONFIG_WLAN_HAL_8814AE) || \
     defined(CONFIG_WLAN_HAL_8814BE) || \
     defined(CONFIG_WLAN_HAL_8822BE) || \
     defined(CONFIG_WLAN_HAL_8812FE) || \
     defined(CONFIG_RTL8852AE_BACKPORTS) || \
     defined(CONFIG_RTL8852AE_MODULE) || defined(CONFIG_RTL8852AE) || defined(CONFIG_RTLWIFI6) || defined(CONFIG_RTLWIFI6_MODULE))
#define IS_5G_SUPPORT 1
#else
#define IS_5G_SUPPORT 0
#endif

#if (!defined(CONFIG_RTL_8812AR_VN_SUPPORT) && \
    (defined(CONFIG_RTL_8812_SUPPORT)|| \
     defined(CONFIG_WLAN_HAL_8814AE) || \
     defined(CONFIG_WLAN_HAL_8814BE) || \
     defined(CONFIG_WLAN_HAL_8822BE) || \
     defined(CONFIG_WLAN_HAL_8812FE) || \
     defined(CONFIG_RTL8852AE_BACKPORTS) || \
     defined(CONFIG_RTL8852AE_MODULE) || defined(CONFIG_RTL8852AE) || defined(CONFIG_RTLWIFI6) || defined(CONFIG_RTLWIFI6_MODULE)))
#define IS_AC_SUPPORT 1
#else
#define IS_AC_SUPPORT 0
#endif
#if	!defined(CONFIG_RTL_DFS_SUPPORT) && \
    (defined(CONFIG_RTL8852AE_BACKPORTS) || \
     defined(CONFIG_RTL8852AE_MODULE) || defined(CONFIG_RTL8852AE) || defined(CONFIG_RTLWIFI6) || defined(CONFIG_RTLWIFI6_MODULE))
#define CONFIG_RTL_DFS_SUPPORT
#endif
#if IS_AX_SUPPORT || defined(WIFI5_WIFI6_COMP)
#define LINK_FILE_NAME_FMT "/var/%s/%s"
#else
#define LINK_FILE_NAME_FMT "/proc/%s/%s"
#endif

#if defined(CONFIG_RTLWIFI6_MODULE) || defined(CONFIG_RTLWIFI6)
#define PROC_WLAN_DIR_NAME "/proc/net/rtk_wifi6"
#else
#define PROC_WLAN_DIR_NAME "/proc/net/rtl8852ae"
#endif

#if (defined(CONFIG_WLAN_HAL_8814AE) || defined(CONFIG_WLAN_HAL_8814BE))
#define WLAN_8814_SERIES_SUPPORT	1
#endif

#if (IS_5G_SUPPORT && (!defined(WLAN_DUALBAND_CONCURRENT) || defined(CONFIG_NO_WLAN_DRIVER))) || \
	(defined(BAND_5G_ON_WLAN0))
	//single band 5G,  dual band 5G, dual linux slave wlan only 5G
#define WLAN0_5G_SUPPORT  1
#endif

#if IS_AC_SUPPORT && (!defined(WLAN_DUALBAND_CONCURRENT) || defined(BAND_5G_ON_WLAN0) || defined(CONFIG_NO_WLAN_DRIVER))
	// single band 5G 11ac, dual band 5G 11ac, dual linux slave wlan only 5G 11ac
#define WLAN0_5G_11AC_SUPPORT 1
#endif

#if defined(WLAN_DUALBAND_CONCURRENT)
#if defined(BAND_2G_ON_WLAN0) || defined(WLAN1_QTN) || \
	(defined(TRIBAND_SUPPORT) && defined(CONFIG_RTL_5G_SLOT_1))
	//dualband
#define WLAN1_5G_SUPPORT 1
#endif

#if defined(CONFIG_WLAN1_5G_11AC_SUPPORT) || defined(WLAN1_QTN) || \
	(defined(TRIBAND_SUPPORT) && defined(CONFIG_RTL_5G_SLOT_1)) || \
    (IS_AC_SUPPORT && defined(BAND_2G_ON_WLAN0))
	//dual linux slave wlan 5G 11ac, dual band single linux wlan1 5G 11ac
#define WLAN1_5G_11AC_SUPPORT 1
#endif
#endif //WLAN_DUALBAND_CONCURRENT

#ifdef CONFIG_WLAN_6G_SUPPORT
#define WLAN_6G_SUPPORT		1
#endif
#ifdef WLAN_6G_SUPPORT
#ifndef WLAN_SUPPPORT_160M
#define WLAN_SUPPPORT_160M	1
#endif
#endif

#if defined(WLAN_SUPPPORT_160M)
#if !defined(WLAN_DUALBAND_CONCURRENT) || defined(BAND_5G_ON_WLAN0)
#define WLAN0_5G_160M_SUPPORT 1
#endif

#if defined(WLAN_DUALBAND_CONCURRENT) && defined(BAND_2G_ON_WLAN0)
#define WLAN1_5G_160M_SUPPORT 1
#endif
#endif //WLAN_SUPPPORT_160M

#if defined(TRIBAND_SUPPORT) && !defined(CONFIG_USB_AS_WLAN1)
#define NUM_WLAN_INTERFACE		3	// number of wlan interface supported
#elif (defined(WLAN_DUALBAND_CONCURRENT) && !defined(WLAN1_QTN))
#define NUM_WLAN_INTERFACE		2	// number of wlan interface supported
#else
#define NUM_WLAN_INTERFACE		1	// number of wlan interface supported
#endif

#if defined(CONFIG_RTL8852AE_BACKPORTS) || defined(CONFIG_RTL8852AE_MODULE) || defined(CONFIG_RTL8852AE) || \
	defined(CONFIG_RTLWIFI6) || defined(CONFIG_RTLWIFI6_MODULE) || \
	defined(RTK_MULTI_AP_ETH_ONLY)
#define WLAN_HAPD					1
#define WLAN_WPAS					1
#ifdef CONFIG_E8B
#ifdef WLAN0_5G_SUPPORT
#undef WLAN0_5G_SUPPORT
#define	WLAN_SWAP_INTERFACE			1
#endif
#ifdef WLAN0_5G_11AC_SUPPORT
#undef WLAN0_5G_11AC_SUPPORT
#endif
#ifndef WLAN1_5G_SUPPORT
#define WLAN1_5G_SUPPORT  			1
#endif
#ifndef WLAN1_5G_11AC_SUPPORT
#define WLAN1_5G_11AC_SUPPORT		1
#endif
#if defined(WLAN_SUPPPORT_160M)
#ifdef WLAN0_5G_160M_SUPPORT
#undef WLAN0_5G_160M_SUPPORT
#endif
#ifndef WLAN1_5G_160M_SUPPORT
#define WLAN1_5G_160M_SUPPORT		1
#endif
#endif //WLAN_SUPPPORT_160M
#endif //CONFIG_E8B
#define WLAN_11AX 					1
//#define SWAP_HW_WLAN_MIB_INDEX	1

//CONFIG_IAPP_SUPPORT
//#define WLAN_TX_BEAMFORMING
//#define TDLS_SUPPORT
//#define WLAN_BAND_STEERING
#define WLAN_WPS
#define WLAN_ON_OFF_BUTTON //force enable on_off button
#endif //CONFIG_RTL8852AE_BACKPORTS

#if (CONFIG_WLAN_MBSSID_NUM == 7)
#define WLAN_8_SSID_SUPPORT             1
#endif

#define WLAN_MIB_MAPPING_STEP		3

#if defined(CONFIG_RTL8852AE_BACKPORTS) || defined(CONFIG_RTL8852AE_MODULE) || defined(CONFIG_RTL8852AE) || defined(CONFIG_RTLWIFI6) || defined(CONFIG_RTLWIFI6_MODULE)
#define LIMITED_VAP_NUM (((CONFIG_LIMITED_AP_NUM-1)<4)?(CONFIG_LIMITED_AP_NUM-1):4) //the smaller value between driver support and web support
#else
#define LIMITED_VAP_NUM			4
#endif

#ifdef WLAN_MBSSID
#ifdef WLAN_8_SSID_SUPPORT //support 8 ssid
#define WLAN_MBSSID_NUM			7
#define MAX_WLAN_VAP			7
#else
#if defined(CONFIG_E8B) || defined(CONFIG_00R0)
#ifdef CONFIG_RTK_DEV_AP
#if defined(CONFIG_USER_CTCAPD) && defined(CONFIG_USER_RTK_MULTI_AP)
#define MAP_DYNAMIC_BACKHAUL_BSS_IF
#if defined(MAP_DYNAMIC_BACKHAUL_BSS_IF)
#define WLAN_MBSSID_NUM			3
#else
#define RTK_RSV_VAP_FOR_EASYMESH
#define MULTI_AP_BSS_IDX       1
#define WLAN_MBSSID_NUM			4
#endif

#else
#if defined(CONFIG_CMCC)
#define WLAN_MBSSID_NUM			3
#else
#define WLAN_MBSSID_NUM			4
#endif//#defined(CONFIG_CMCC)
#endif
#else
#define WLAN_MBSSID_NUM			3
#endif //CONFIG_RTK_DEV_AP
#else
#if defined(CONFIG_RTL8852AE_BACKPORTS) || defined(CONFIG_RTL8852AE_MODULE) || defined(CONFIG_RTL8852AE) || defined(CONFIG_RTLWIFI6) || defined(CONFIG_RTLWIFI6_MODULE)
#ifdef CONFIG_TELMEX_DEV
#define WLAN_MBSSID_NUM			3
#else
#define WLAN_MBSSID_NUM			LIMITED_VAP_NUM
#endif
#else
#if defined(CONFIG_TRUE) ||defined(CONFIG_TELMEX_DEV)
#define WLAN_MBSSID_NUM			3
#else
#define WLAN_MBSSID_NUM			4
#endif
#endif
#endif //CONFIG_E8B
#define MAX_WLAN_VAP			4
#endif // WLAN_8_SSID_SUPPORT
#else
#define WLAN_MBSSID_NUM			0
#define MAX_WLAN_VAP			0
#endif //WLAN_MBSSID

//#define WLAN_USE_VAP_AS_SSID1
#ifdef WLAN_USE_VAP_AS_SSID1
#ifdef WLAN_8_SSID_SUPPORT
#define WLAN_VAP_USED_NUM		WLAN_MBSSID_NUM
#else
#define WLAN_VAP_USED_NUM		(WLAN_MBSSID_NUM+1)
#endif
#else
#define WLAN_VAP_USED_NUM		WLAN_MBSSID_NUM
#endif

#ifdef WLAN_USE_VAP_AS_SSID1
#define WLAN_VAP_ITF_INDEX		0 // root
#define WLAN_ROOT_ITF_INDEX		(WLAN_VAP_USED_NUM)
#else
#define WLAN_VAP_ITF_INDEX		1 // root
#define WLAN_ROOT_ITF_INDEX		0
#endif
#define WLAN_REPEATER_ITF_INDEX		(1+MAX_WLAN_VAP) // root+VAP
#ifdef WLAN_UNIVERSAL_REPEATER
#define NUM_VWLAN_INTERFACE	(MAX_WLAN_VAP+1) // VAP+Repeater
#else
#define NUM_VWLAN_INTERFACE	MAX_WLAN_VAP // VAP
#endif

#define WLAN_SSID_NUM	(1+WLAN_MBSSID_NUM)

#define WLAN_MAX_ITF_INDEX	(MAX_WLAN_VAP+1)

#undef E8A_CHINA

#undef WLAN_ONOFF_BUTTON_SUPPORT

//xl_yue: support zte531b--light:wps function is ok; die:wps function failed; blink: wps connecting
//#undef	 REVERSE_WPS_LED

#define ENABLE_WPAAES_WPA2TKIP

#if !defined(CONFIG_00R0) && !defined(CONFIG_E8B) && !defined(CONFIG_TRUE)
#define NEW_WIFI_SEC //wifi security requirements after 2014.01.01
#endif

#if defined(CONFIG_RTL_WPA3_SUPPORT) || defined(CONFIG_WIFI6_WPA3) || IS_AX_SUPPORT || defined(RTK_MULTI_AP_ETH_ONLY)
#define WLAN_WPA3
#endif

#ifdef WLAN_WPA3
#if defined(CONFIG_USER_HAPD_2_10) || defined(CONFIG_USER_WPAS_2_10)
#define WLAN_WPA3_H2E	1
#if defined(CONFIG_USER_HAPD_2_10)
#define WLAN_WPA3_H2E_HAPD	1
#endif
#if defined(CONFIG_USER_WPAS_2_10)
#define WLAN_WPA3_H2E_WPAS	1
#endif
#elif !defined(CONFIG_USER_HAPD) && !defined(CONFIG_USER_WPAS) && defined(CONFIG_RTL_WPA3_SUPPORT)
#define WLAN_WPA3_H2E	1
#endif
#endif //WLAN_WPA3

#ifdef CONFIG_RTL_REPEATER_MODE_SUPPORT
#ifdef CONFIG_USER_RTK_WIRELESS_WAN
#define WLAN_WISP
#endif
#endif

#if defined(CONFIG_RTL_11W_SUPPORT) || defined(CONFIG_RTW_80211W_SUPPORT) || defined(RTK_MULTI_AP_ETH_ONLY)
#define WLAN_11W
#endif

//#ifdef CONFIG_RTL_11R_SUPPORT
#if defined(CONFIG_RTW_80211R) || defined(CONFIG_RTL_11R_SUPPORT)
#define WLAN_11R
#endif

#if defined(WLAN_11R) && defined(WLAN_HAPD)
#define WLAN_11R_HAPD
#endif

#if defined(CONFIG_RTW_80211K) || defined(CONFIG_RTL_DOT11K_SUPPORT)
#define WLAN_11K
#endif

#if defined(CONFIG_RTW_80211V) || defined(CONFIG_RTL_11V_SUPPORT)
#define WLAN_11V
#endif

#ifdef CONFIG_USER_WIRELESS_SITESURVEY_DISPLAY
#define WLAN_SITESURVEY
#endif

#ifdef CONFIG_USER_WIRELESS_LIMITED_STA_NUM
#define WLAN_LIMITED_STA_NUM
#endif

#ifdef CONFIG_WLAN_11N_COEXIST
#define WLAN_11N_COEXIST
#endif

#if defined(CONFIG_RTW_CURRENT_RATE_ACCOUNTING) && !defined(CONFIG_CURRENT_RATE_ACCOUNTING)
#define CONFIG_CURRENT_RATE_ACCOUNTING
#endif

#if defined(CONFIG_SLOT_0_TX_BEAMFORMING) || defined(CONFIG_SLOT_1_TX_BEAMFORMING) || defined(WIFI5_WIFI6_COMP)
#define WLAN_TX_BEAMFORMING
#endif

#if defined(CONFIG_SLOT_0_TX_BEAMFORMING) || defined(CONFIG_SLOT_1_TX_BEAMFORMING)
#define WLAN_TX_BEAMFORMING_ENABLE
#endif

#if defined(CONFIG_RTL_TDLS_SUPPORT)
#define TDLS_SUPPORT
#endif
#if !defined(CONFIG_MBO_SUPPORT) && \
    (defined(CONFIG_RTL8852AE_BACKPORTS) || \
     defined(CONFIG_RTL8852AE_MODULE) || defined(CONFIG_RTL8852AE) || defined(CONFIG_RTLWIFI6) || defined(CONFIG_RTLWIFI6_MODULE))
#if defined(WLAN_11K) && defined(WLAN_11V) && defined(WLAN_11W)
#define CONFIG_MBO_SUPPORT
#endif
#endif

#if (defined(CONFIG_SLOT_0_ANT_SWITCH) && defined(CONFIG_SLOT_0_8192EE)) || (defined(CONFIG_SLOT_1_ANT_SWITCH) && defined(CONFIG_SLOT_1_8192EE))
#if defined(TRIBAND_SUPPORT)
#define WLAN_INTF_TXBF_DISABLE 2
#elif defined (WLAN_DUALBAND_CONCURRENT) && defined(BAND_5G_ON_WLAN0)
#define WLAN_INTF_TXBF_DISABLE 1
#else
#define WLAN_INTF_TXBF_DISABLE 0
#endif
#endif

#ifdef CONFIG_RTL8832BR
#define WLAN_INTF_TSSI_SLOPE_K 1
#endif

#if defined(CONFIG_RTL_MESH_SUPPORT)
#define WLAN_MESH
#define WLAN_MESH_ACL_ENABLE
#endif

#if defined(CONFIG_RTK_SMART_ROAMING)
#define RTK_SMART_ROAMING
#endif

#if defined(CONFIG_STA_ROAMING_CHECK)
#ifndef STA_ROAMING_CHECK
#define STA_ROAMING_CHECK
#endif
#endif

#if defined(CONFIG_SBWC) || defined(CONFIG_RTW_SBWC)
#ifndef SBWC
#define SBWC
#endif
#endif

#if defined(CONFIG_RTL_CROSSBAND_REPEATER_SUPPORT) || defined(CONFIG_RTW_CROSSBAND_REPEATER_SUPPORT)
#ifndef RTK_CROSSBAND_REPEATER
#define RTK_CROSSBAND_REPEATER
#endif
#endif

#if (defined(CONFIG_RTL_STA_CONTROL_SUPPORT) || defined(CONFIG_BAND_STEERING)) && defined(WLAN_DUALBAND_CONCURRENT)
#define WLAN_BAND_STEERING
#endif

#if defined(WLAN_BAND_STEERING) && defined(CONFIG_WLAN_MANAGER)
#ifndef CONFIG_E8B
#define WLAN_MANAGER_SUPPORT
#endif
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_E8B)
#ifdef CONFIG_RTK_DEV_AP
#define WLAN_ETHER_MAC_ADDR_NEED_OFFSET
#endif
#endif

#if defined(CONFIG_USER_RTK_MULTI_AP)
	#define WLAN_RTK_MULTI_AP
	#if !defined(RTK_MULTI_AP)
		#define RTK_MULTI_AP
	#endif
#ifdef RTK_MULTI_AP
	#if defined(CONFIG_USER_HAPD) || defined(CONFIG_USER_WPAS)
		#define RTK_HAPD_MULTI_AP
	#endif
#endif
	#ifdef CONFIG_USER_RTK_MULTI_AP_MAP_GROUP_SUPPORT
		#define MAP_GROUP_SUPPORT
	#endif

	#if defined(CONFIG_USER_RTK_MULTI_AP_WFA)
		#if !defined(RTK_MULTI_AP_WFA)
			#define RTK_MULTI_AP_WFA
		#endif
	#endif
	#if defined(CONFIG_USER_RTK_MULTI_AP_AGENT_LOOP_DETECT)
		#define MULTI_AP_AGENT_LOOP_DETECT
	#endif
	#if defined(CONFIG_USER_RTK_MULTI_AP_SWTICH_MODE_STP)
		#define MULTI_AP_STP_SWTICH
	#endif
	#if defined(CONFIG_USER_RTK_MULTI_AP_CROSS_BAND_BACKHAUL_STEERING)
		#define CROSS_BAND_BACKHAUL_STEERING
	#endif
	#if defined(CONFIG_USER_RTK_MULTI_AP_BACKHAUL_LINK_SELECTION)
		#define BACKHAUL_LINK_SELECTION
	#endif
	#if defined(CONFIG_USER_RTK_MULTI_AP_BACKHAUL_LINK_SWITCH)
		//#define BACKHAUL_REAL_TIME_SWITCH
		#define ARP_LOOP_DETECTION
	#endif
	#if defined(CONFIG_USER_RTK_MULTI_AP_ROLE_AUTO_SELECTION)
		#define WLAN_MULTI_AP_ROLE_AUTO_SELECT
	#endif
	#if defined(CONFIG_RTK_MULTI_AP_R3) || defined(CPTCFG_RTK_MULTI_AP_R3) || defined(CPTCFG_RTW_MULTI_AP_R3)
		#if !defined(RTK_MULTI_AP_R3)
			#define RTK_MULTI_AP_R3
			#define RTK_MULTI_AP_R2
		#endif
	#endif
	#if defined(CONFIG_RTK_MULTI_AP_R2) || defined(CPTCFG_RTK_MULTI_AP_R2) || defined(CONFIG_RTW_MULTI_AP_R2) || defined(CPTCFG_RTW_MULTI_AP_R2)
		#if !defined(RTK_MULTI_AP_R2)
			#define RTK_MULTI_AP_R2
		#endif
	#else
		#if !defined(RTK_MULTI_AP_R1)
			#define RTK_MULTI_AP_R1
		#endif
	#endif
	#ifdef CONFIG_USER_RTK_MULTI_AP_OPEN_ENCRYPT_SUPPORT
		#define MULTI_AP_OPEN_ENCRYPT_SUPPORT
	#endif
#endif

#if !defined(CONFIG_RTK_DEV_AP)
#if defined(CONFIG_CMCC) && defined(RTK_MULTI_AP)
#define MULTI_AP_BACKHAUL_BSS_DEFAULT_ENCRYPT	WIFI_SEC_WPA2
#endif
#endif

#if defined(CONFIG_USER_RTK_WLAN_MANAGER) && defined(CONFIG_RTK_DEV_AP)
#define RTK_WLAN_MANAGER
#ifdef CONFIG_BAND_STEERING
#define BAND_STEERING_SUPPORT
#ifdef WLAN_BAND_STEERING
#undef WLAN_BAND_STEERING
#endif
#endif
#endif

#if 0 // !defined(CONFIG_RTK_DEV_AP) && defined(WIFI5_WIFI6_COMP)
//for non-AP AX1500 band steering
#ifdef WLAN_BAND_STEERING
#define RTK_WLAN_MANAGER
#define BAND_STEERING_SUPPORT
#undef WLAN_BAND_STEERING
#endif
#endif

#if defined(CONFIG_USER_RTK_COMMON_NETLINK)
#define RTK_COMMON_NETLINK
#endif


#if defined(CONFIG_RTL_HS2_SUPPORT)
#define HS2_SUPPORT
#endif

#if defined(CONFIG_RTL_ATM_SUPPORT)
#define WLAN_AIRTIME_FAIRNESS
#endif

#if !(defined(CONFIG_RTL8852AE_BACKPORTS) || defined(CONFIG_RTL8852AE_MODULE) || defined(CONFIG_RTL8852AE) || defined(CONFIG_RTLWIFI6) || defined(CONFIG_RTLWIFI6_MODULE))
#define WLAN_DENY_LEGACY
#endif

#if defined(CONFIG_WIFI_SIMPLE_CONFIG)
#ifdef CONFIG_USER_WLAN_WPS_QUERY
#define WPS_QUERY
#endif
#ifdef CONFIG_USER_WLAN_WPS_VAP
#define WLAN_WPS_VAP
#define WLAN_WPS_MULTI_DAEMON
#endif
#endif

#ifdef WLAN_WPS
#ifdef CONFIG_USER_HAPD
#define WLAN_WPS_HAPD
#ifndef WPS20
#define WPS20
#endif
#ifdef CONFIG_USER_WLAN_WPS_VAP
#define WLAN_WPS_VAP
#endif
#endif //CONFIG_USER_HAPD
#ifdef CONFIG_USER_WPAS
#define WLAN_WPS_WPAS
#ifndef WPS20
#define WPS20
#endif
#endif //CONFIG_USER_WPAS
#endif

#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
#ifdef WLAN_WPA3
#define WLAN_WPA3_NOT_SUPPORT_WPS 1
#endif
#endif

//#if !defined(WLAN_HAPD)
//for hostapd vap will share the same band configuration as root
#define WLAN_BAND_CONFIG_MBSSID	1
//#endif
#ifdef CONFIG_RTK_DEV_AP
#define WLAN_BAND_SETTING 1
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#define WLAN_LIFETIME
#define WLAN_RATE_PRIOR
#if !defined(CONFIG_CU_BASEON_YUEME)
#define WLAN_TXPOWER_HIGH
#endif
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
#define WLAN_SMARTAPP_ENABLE
#define WLAN_ROAMING
#define YUEME_WLAN_USE_MAPPING_IDX
#define WLAN_VSIE_SERVICE
#endif
#if defined(CONFIG_USER_RTK_MULTI_AP) && defined(CONFIG_YUEME)
#define WLAN_CTC_MULTI_AP
#endif
#endif

#ifdef CONFIG_USER_CTCAPD
#define WLAN_VSIE_SERVICE
#define WLAN_SEC_WEB_ENABLE
#define CONFIG_GUEST_ACCESS_SUPPORT
#endif

#ifdef CONFIG_RTK_DEV_AP
#ifndef CONFIG_GUEST_ACCESS_SUPPORT
#define CONFIG_GUEST_ACCESS_SUPPORT
#endif
#endif

#ifdef CONFIG_TRUE
#define REMOTE_HTTP_ACCESS_AUTO_TIMEOUT
#define USE_BASE64_PASSWORD_ENCRYPT
#define FORBID_NORMAL_USER_MOD_VALID_LOID

#define _PRMT_X_TRUE_SECURITY_ 1
#define _PRMT_X_TRUE_FIREWALL_
#define _PRMT_X_TRUE_USERINTERFACE_ 1
#define _PRMT_X_TRUE_UPNP_ 1
#define _PRMT_X_TRUE_DDNS_ 1
#define _PRMT_X_TRUE_PORTFORWARD_ 1
#define _PRMT_X_TRUE_WLAN_ 1
#define _PRMT_X_TRUE_UPTIME_ 1
#define TRUE_NAME_PREFIX "TRUE"
#define TRUECOM_NAME_PREFIX "TRUE-COM"

#define _PRMT_X_TRUE_COM_ALG_
#define _PRMT_X_TRUE_COM_DDNS_
#define _PRMT_X_TRUE_DEVICEINFO_
#define _PRMT_X_TRUE_DEVINFO_
#define _PRMT_X_TRUE_WAN_VLANID_
#define _PRMT_X_TRUE_CATV_
#define _PRMT_X_TRUE_POLICY_ROUTE_
#define _PRMT_X_RTK_DEVICE_TYPE_
#define _PRMT_X_RTK_ACCESS_TYPE_
#define _PRMT_X_RTK_CHANNELUTILIZATION_
#define _PRMT_X_RTK_OUI_FROM_MIB_
#define _PRMT_X_RTK_CLIENT_ACTIVE_JUDGE_
#define _PRMT_X_RTK_RADIO_NEIGHBOR_
#define _PRMT_X_TRUE_LASTSERVEICE
#define _PRMT_X_TRUE_OPTION82
#define _PRMT_X_DHCP_IPv6
#define _PRMT_X_DEFAULT_WANNAME
#define _PRMT_X_TRUE_ACCOUNT_
#define _PRMT_X_TRUE_WANACL_MODE
#define _PRMT_X_TRUE_WIFI_NEIGHBOR
#define _PRMT_X_TRUE_WANEXT_
#define _PRMT_X_TRUE_IGMP_SETTING_
#define NEW_PORTMAPPING
#define PPP_DELAY_DOWN
#define PPP_DELAY_UP
#ifdef PPP_DELAY_UP
#define PPP_UP_MAX_DELAY 5
#endif
#define DHCPC_DELAY_TO_UP
#define NEWEST_TEN_FLOW_INFO
#ifdef WLAN_DUALBAND_CONCURRENT
#define WLAN_VAP_BAND_STEERING
#endif
#define WLAN_ROAMING
#define WLAN_COUNTRY_SETTING
#define DHCPD_MULTI_THREAD_SUPPORT
#ifdef WLAN_RTK_MULTI_AP
#define EASY_MESH_STA_INFO
#define EASY_MESH_BACKHAUL
#endif
#endif

#ifdef CONFIG_E8B
#define WLAN_USE_DEFAULT_AUTO_CHANNEL		1
#define WLAN_USE_DEFAULT_SSID_PSK			1
#define WLAN_USE_DEFAULT_CHANNELWIDTH_20M	1 //for 2.4G only
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
//#ifdef WLAN_WPS_VAP
#define WLAN_USE_WSC_DEFAULT_CONFIGURED		1
//#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CU)
#define WLAN_USE_WSC_DEFAULT_DISABLE_UPNP	1
#endif
#endif // CONFIG_WIFI_SIMPLE_CONFIG
#if defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(CONFIG_YUEME)
#define WLAN_STA_RSSI_DIAG 1
#define WLAN_CHECK_MIB_BEFORE_RESTART		1
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_AP_CMCC)
#ifdef WLAN_BAND_STEERING
#undef WLAN_BAND_STEERING // FIXME: webpage not support
#endif
#endif // CONFIG_CMCC || CONFIG_CU_BASEON_CMCC || CONFIG_USER_AP_CMCC
#endif //CONFIG_E8B

#define WLAN_ROOT_MIB_SYNC
#endif // CONFIG_USER_WIRELESS_TOOLS

#ifndef CONFIG_SFU
#define IP_PORT_FILTER
#define MAC_FILTER
#ifdef CONFIG_RTK_DEV_AP
#define IGMPHOOK_MAC_FILTER
#endif
#define PORT_FORWARD_GENERAL
//#define URL_BLOCKING_SUPPORT // unused option for deonet
#undef URL_ALLOWING_SUPPORT
//#define DOMAIN_BLOCKING_SUPPORT // unused option for deonet
#ifdef CONFIG_USER_PARENTAL_CONTROL
#define PARENTAL_CTRL
#endif
//uncomment for TCP/UDP connection limit
//#define TCP_UDP_CONN_LIMIT	1
#undef TCP_UDP_CONN_LIMIT
#undef NAT_CONN_LIMIT
#undef NATIP_FORWARDING
#define DMZ
#undef ADDRESS_MAPPING
#define ROUTING
#ifdef CONFIG_USER_RTK_IP_PASSTHROUGH
#define IP_PASSTHROUGH
#endif
#if !(defined(CONFIG_TRUE)||defined(CONFIG_TELMEX_DEV))
#define IP_ACL
#endif
#ifndef IP_ACL
#define REMOTE_ACCESS_CTL
#endif
#ifdef CONFIG_E8B
#define MAC_FILTER_SRC_ONLY 1
#define MAC_FILTER_SRC_WHITELIST 1
//#ifdef CONFIG_RTL9607C_SERIES
/* mac filter times support */
#if !defined(CONFIG_CMCC)
#if !defined(CONFIG_CU_BASEON_CMCC)
#define MAC_FILTER_BLOCKTIMES_SUPPORT	1
#endif
#define REMOTE_ACCESS_CTL
#endif
#define URL_ALLOWING_SUPPORT
#endif //CONFIG_E8B
#endif

#ifdef CONFIG_USER_PORT_TRIGGER_STATIC
#define PORT_TRIGGERING_STATIC
#endif

#ifdef CONFIG_USER_PORT_TRIGGER_DYNAMIC
#define PORT_TRIGGERING_DYNAMIC
#endif

// Mason Yu
#undef PORT_FORWARD_ADVANCE
#ifdef CONFIG_USER_RTK_PPPOE_PASSTHROUGH
#define PPPOE_PASSTHROUGH
#endif
#undef  MULTI_ADDRESS_MAPPING
#undef CONFIG_IGMP_FORBID
#if defined(CONFIG_CMCC) ||defined(CONFIG_CU_BASEON_CMCC)
#define FORCE_IGMP_V2			1
#endif

#ifdef CONFIG_USER_BOA_AUTH_DIGEST_SUPPORT
#define	SUPPORT_AUTH_DIGEST 1
#else
#undef SUPPORT_AUTH_DIGEST
#endif

#if defined(CONFIG_USB_SUPPORT) && defined(CONFIG_E8B)
#define _PRMT_USBRESTORE            1
#endif

#define WEB_UPGRADE			1
//#undef WEB_UPGRADE			// jimluo e8-A spec, unsupport web upgrade.

// Mason Yu
#undef AUTO_PVC_SEARCH_TR068_OAMPING
#define AUTO_PVC_SEARCH_PURE_OAMPING
#define AUTO_PVC_SEARCH_AUTOHUNT

#ifndef AUTO_PVC_SEARCH_TR068_OAMPING
#define AUTO_PVC_SEARCH_TABLE
#endif

#ifdef CONFIG_USER_VSNTP
#define TIME_ZONE			1
#endif
#ifdef CONFIG_WIFI_TIMER_SCHEDULE
#define WIFI_TIMER_SCHEDULE		1
#endif


#ifdef CONFIG_USER_IPROUTE2_TC_TC

#define	IP_QOS_VPORT		1
#undef   CONFIG_8021P_PRIO
#ifdef CONFIG_IP_NF_TARGET_DSCP
#define QOS_DSCP		1
#endif
#endif
#ifdef CONFIG_USER_IPROUTE2_IP_IP
#ifdef CONFIG_USER_IP_QOS
//#define IP_POLICY_ROUTING		1
#endif
#endif

#ifdef CONFIG_USER_IP_QOS_TRAFFIC_SHAPING_BY_VLANID
#define QOS_TRAFFIC_SHAPING_BY_VLANID
#endif

#ifdef CONFIG_USER_IP_QOS_TRAFFIC_SHAPING_BY_SSID
#define QOS_TRAFFIC_SHAPING_BY_SSID
#endif

#ifdef CONFIG_RTK_DEV_AP
#define CONFIG_USER_AP_QOS
#if defined(CONFIG_RTL_G3LITE_QOS_SUPPORT)
#define QOS_BANDWIDTH_UNLIMITED					2500000		// 8226 2.5Gbps
#else
#define QOS_BANDWIDTH_UNLIMITED					1048568		// 0x1FFFF
#endif

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
#if defined(CONFIG_RTL_83XX_QOS_SUPPORT) || defined(CONFIG_RTL_G3LITE_QOS_SUPPORT) || defined(CONFIG_RTK_SOC_RTL8198D) || defined(CONFIG_RTL8198X_SERIES)
#define CONFIG_USER_AP_HW_QOS
#if defined(CONFIG_USER_RTK_BRIDGE_MODE)
#define CONFIG_USER_AP_BRIDGE_MODE_QOS
#endif
#if defined(CONFIG_EXTERNAL_PHY_POLLING)
#define CONFIG_RTK_EXT_PHY_QOS
#endif
#endif
#endif

#if defined(CONFIG_RTK_SOC_RTL8198D)
#define CONFIG_USER_AP_QUEUE_METER
#if defined(CONFIG_E8B)
#ifndef CONFIG_CMCC
#define CONFIG_USER_AP_CT_QOS
#endif
#define CONFIG_USER_AP_E8B_QOS
#define CONFIG_USER_AP_MAC_SPEED_LIMIT_QOS
#else
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
#define CONFIG_USER_AP_QOS_PHY_MULTI_WAN     //defined in Retailor
#define QOS_MAX_PORT_NUM 5 //max phy port numb, change based on the max value of SW_LAN_PORT_NUM among preconfigs
#endif
#endif
#endif
#endif

#if defined(CONFIG_USER_CTCAPD) || defined( CONFIG_USER_SC_API) || defined(CONFIG_ANDLINK_SUPPORT)
#define CONFIG_QOS_SPEED_LIMIT_UNIT_ONE
#endif
#ifdef CONFIG_QOS_SPEED_LIMIT_UNIT_ONE
#define QOS_SPEED_LIMIT_UNIT 1      // kbps
#else
#define QOS_SPEED_LIMIT_UNIT 512    // kbsp
#define QOS_SPEED_LIMIT_UNIT_1000 500
#endif

#if defined(CONFIG_ANDLINK_SUPPORT) || defined(CONFIG_USER_ANDLINK_PLUGIN)
#define WLAN_POWER_PERCENT_SUPPORT
#endif

#define ENABLE_802_1Q		// enable_802_1p_090722
#undef VLAN_GROUP
#undef ELAN_LINK_MODE
#undef ELAN_LINK_MODE_INTRENAL_PHY
#define DIAGNOSTIC_TEST			1

//xl_yue
#if defined(CONFIG_RTK_DOS) || defined(CONFIG_DOS)
#define DOS_SUPPORT
#else
#ifdef CONFIG_E8B
#define DOS_SUPPORT
#else
#undef  DOS_SUPPORT
#endif
#endif

#define NEW_DGW_POLICY   // E8B: the dgw is useless in MIB_ATM_VC_TBL, the INTERNET type connection who first get ip will be default gateway
#define DEFAULT_GATEWAY_V1	//set dgw per pvc
#undef DEFAULT_GATEWAY_V2	// set dgw interface in routing page
#ifndef DEFAULT_GATEWAY_V2
#ifndef DEFAULT_GATEWAY_V1
#define DEFAULT_GATEWAY_V1	1
#endif
#endif
#ifdef DEFAULT_GATEWAY_V2
#define AUTO_PPPOE_ROUTE	1
//#undef AUTO_PPPOE_ROUTE
#endif

//alex_huang
#undef  CONFIG_SPPPD_STATICIP
#ifdef CONFIG_00R0
#define XOR_ENCRYPT
#else
#undef XOR_ENCRYPT
#endif

#ifdef CONFIG_XML_ENABLE_ENCRYPT
#define XOR_ENCRYPT
#endif

#define TELNET_IDLE_TIME	600 //10*60 sec. Please compile boa and telnetd

#if defined(CONFIG_YUEME) || defined(CONFIG_CU) || defined(CONFIG_00R0)
#define CTC_TELNET_LOGIN_TRY_LIMIT
#define CTC_TELNET_LOGIN_TRY_MAX 1
#define CTC_TELNET_LOGIN_TRY_LOCK_TIME 60
#define CTC_TELNET_LOGIN_FAIL_MAX 3

#define CTC_TELNET_LOGOUT_IDLE
#define CTC_TELNET_LOGOUT_IDLE_TIME 300 //5*60 sec.

#define CTC_TELNET_ONE_USER_LIMIT
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU)
/* CTC TELNET flag */
#define CTC_TELNET_LOGIN_WITH_NORMAL_USER_PRIVILEGE

#define CTC_TELNET_SCHEDULED_CLOSE
#define CTC_TELNET_SCHEDULED_CLOSE_TIME (24*60*60) // 24 hours

#if defined(CONFIG_USER_MENU_CLI)
#define CTC_YUEME_MENU_CLI
#endif

#define CTC_TELNET_CLI_CTRL

#define CTC_TELNET_CMD_CTRL 1
/* CTC TELNET flag end */
#endif

/* wpeng 20120412 END*/

#ifdef CONFIG_USER_USP_TR369
/* for web page */
#define USP_TR369_ENABLE_STOMP
#define USP_TR369_ENABLE_COAP
#define USP_TR369_ENABLE_WEBSOCKET
#define USP_TR369_ENABLE_MQTT
#endif

#define _SUPPORT_INTFGRPING_PROFILE_ 1

#ifdef CONFIG_USER_CWMP_TR069
// Mason Yu
#define _CONFIG_DHCPC_OPTION43_ACSURL_         1
#define _CWMP_MIB_				1
#ifdef CONFIG_USER_CWMP_WITH_SSL
#define _CWMP_WITH_SSL_				1
#endif //CONFIG_USER_CWMP_WITH_SSL
#define _PRMT_SERVICES_				1
#define _PRMT_CAPABILITIES_			1
#define _PRMT_DEVICECONFIG_			1
#ifdef CONFIG_00R0
#define _PRMT_USERINTERFACE_			1
#define _PRMT_X_RTK_ 1
#endif
#ifdef CONFIG_USER_DNS_RELAY_PROXY
#define _PRMT_DNS_				1
#endif
/*disable connection request authentication*/
//#define _TR069_CONREQ_AUTH_SELECT_		1
#ifdef CONFIG_USER_TR143
#define _PRMT_TR143_				1
#endif //CONFIG_USER_TR143
#ifdef CONFIG_USB_ETH
#define _PRMT_USB_ETH_				1
#endif //CONFIG_USB_ETH

#if defined(CONFIG_00R0)
#define CONFIG_CWMP_TRANSFER_QUEUE
#define _DEFAULT_ROUTE_AS_TR069_INTERFACE_ 1
#endif

/*ping_zhang:20081217 START:patch from telefonica branch to support WT-107*/
#define _PRMT_WT107_					1
#ifdef _PRMT_WT107_
#define _SUPPORT_TRACEROUTE_PROFILE_		1
#undef _SUPPORT_CAPTIVEPORTAL_PROFILE_
#ifdef CONFIG_DEV_xDSL
#define _SUPPORT_ADSL2DSLDIAG_PROFILE_		1
#define _SUPPORT_ADSL2WAN_PROFILE_		1
#endif
#endif //_PRMT_WT107_
/*ping_zhang:20081217 END*/

#define _SUPPORT_L2BRIDGING_PROFILE_ 1

#if defined(CONFIG_CMCC_ENTERPRISE)
#define _SUPPORT_L2BRIDGING_DHCP_SERVER_ 1
#endif
#define _PRMT_X_TELEFONICA_ES_DHCPOPTION_	1
#ifdef CONFIG_00R0
#define CONFIG_USER_DHCP_OPT_GUI_60
#endif

/*ping_zhang:20081223 START:define for support multi server by default*/
#define SNTP_MULTI_SERVER			1
/*ping_zhang:20081223 END*/

#if defined(CONFIG_USER_RTK_SYSLOG_SAVE_TO_FLASH) || defined(CONFIG_00R0) || defined(CONFIG_E8B) || defined(CONFIG_TRUE)
#define BB_FEATURE_SAVE_LOG
#endif

#ifdef CONFIG_E8B
#define USE_BUSYBOX_KLOGD
#if defined(CONFIG_LUNA) || defined(CONFIG_CTC_E8_CLIENT_LIMIT)
#define IP_BASED_CLIENT_TYPE
#endif

#ifdef CONFIG_USER_ADAPTER_API_ISP
#ifdef CONFIG_USER_SAMBA
#define SAMBA_ACCOUNT_INDEPENDENT
#endif

#ifdef CONFIG_USER_FTPD_FTPD
#define FTP_ACCOUNT_INDEPENDENT
#endif

#ifdef CONFIG_USER_TELNETD_TELNETD
#define TELNET_ACCOUNT_INDEPENDENT
#endif
#endif

	/* copy from e8-user*/
#define CTC_TELECOM_ACCOUNT
#if !defined(CONFIG_YUEME)
#define FTP_ACCOUNT_INDEPENDENT
#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME) || defined(CONFIG_CMCC)
#define FTP_SERVER_INTERGRATION
#define FTP_SERVER_API_INTERGRATION
#endif
#define TELNET_ACCOUNT_INDEPENDENT

#define SSH_ACCOUNT_INDEPENDENT
#define TFTP_ACCOUNT_INDEPENDENT
#define SNMP_ACCOUNT_INDEPENDENT

#ifdef CONFIG_YUEME
#define DHCPC_RIGOROUSNESS_SUPPORT
#define STB_L2_FRAME_LOSS_RATE
#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#define CTC_DNS_SPEED_LIMIT
#define CTC_DNS_TUNNEL
#endif

#define E8B_NEW_DIAGNOSE
#define _PRMT_X_CT_EXT_ENABLE_
#ifdef CONFIG_CU
#define CTCOM_NAME_PREFIX "CU"
#elif defined(CONFIG_CMCC)
#define CTCOM_NAME_PREFIX "CMCC"
#else
#define CTCOM_NAME_PREFIX "CT-COM"
#endif
#if !defined(CONFIG_CMCC) && !defined(CONFIG_CU_BASEON_CMCC)
#define USE_WEB_CTC_DIR
#endif

#ifdef CONFIG_RTK_DEV_AP
#define CONFIG_E8BUI_SHOW_CONREQ_PATH_PORT
#endif
#endif	// CONFIG_E8B

#ifdef _PRMT_X_CT_EXT_ENABLE_
	/*TW's ACS has some problem with this extension field*/
	#define _INFORM_EXT_FOR_X_CT_		1
	#define _INFORM_RESPONSE_EXT_FOR_X_CT_ 1
    #ifdef _PRMT_SERVICES_
	#define _PRMT_X_CT_COM_IPTV_		1
	#define _PRMT_X_CT_COM_MWBAND_		1
    #endif //_PRMT_SERVICES_
	#define	_PRMT_X_CT_COM_DDNS_		1
	#define _PRMT_X_CT_COM_ALG_		1
	#define _PRMT_X_CT_COM_ACCOUNT_		1
	#define _PRMT_X_CT_COM_RECON_		1
	#define _PRMT_X_CT_COM_PORTALMNT_	1
	#define _PRMT_X_CT_COM_SRVMNG_		1	/*ServiceManage*/
	#define _PRMT_X_CT_COM_PPPOE_PROXY_	1
    #ifdef WLAN_SUPPORT
	#define _PRMT_X_CT_COM_WLAN_		1
    #endif //WLAN_SUPPORT
	#define _PRMT_X_CT_COM_DHCP_		1
	#define _PRMT_X_CT_COM_WANEXT_		1
	#define _PRMT_X_CT_COM_DLNA_		1
	#define _PRMT_X_CT_COM_UPNP_		1
	#define _PRMT_X_CT_COM_DEVINFO_		1
	#define _PRMT_X_CT_COM_ALARM_MONITOR_	1
	#define _PRMT_X_CT_COM_IPv6_		1
	#define _PRMT_X_CT_COM_ETHLINK_ 	1
	#define _PRMT_X_CT_COM_PING_		1
	#define _PRMT_X_CT_COM_TIME_		1
	#define _PRMT_X_CT_COM_QOS_			1
	#define _PRMT_X_CT_COM_USERINFO_	1
	#define _PRMT_X_CT_COM_SYSLOG_	1
	#define _PRMT_X_CT_COM_QOE_			1
	#define _PRMT_X_CT_COM_VLAN_BOUND_	1
#ifdef CONFIG_USER_RTK_LBD
	#define _PRMT_X_CT_COM_LBD_ 1
#endif
#ifdef SUPPORT_NON_SESSION_IPOE
	#define _PRMT_X_CT_COM_NEIGHBORDETECTION_ 1
#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	#define _PRMT_X_CT_COM_MGT_		1
	#define _PRMT_X_CT_COM_TRACEROUTE_	1

	//FIXME: Need FC version
	//#define _PRMT_X_CT_COM_PERFORMANCE_REPORT_		1

	#define _PRMT_X_CT_COM_MACFILTER_		1
#if defined(CONFIG_SUPPORT_AUTO_DIAG)
	#define _PRMT_X_CT_COM_IPoEDiagnostics_ 		1
#endif
#ifdef WLAN_SUPPORT
	#define _PRMT_X_WLANFORISP_ 1
	#define _PRMT_X_CT_COM_WirelessTestDiagnostics_ 		1
#endif
#ifdef CONFIG_YUEME
	/* 2019/08/23 now we just test yueme fc, we temporarily don't open SICHUAN CT_COM
	 * to avoid some rg fuction and CWMP compile error. If we need to use SICHUAN CT_
	 * COM feature, you can try to fix it.
	 */
	//#define _PRMT_SC_CT_COM_			1
#ifdef _PRMT_SC_CT_COM_
	#define _PRMT_SC_CT_COM_NAME "SC_CT-COM"
	#define _PRMT_SC_CT_COM_Device_
	#define _PRMT_SC_CT_COM_InternetService_
	#define _PRMT_SC_CT_COM_GroupCompanyService_
#ifdef _PRMT_SC_CT_COM_InternetService_
	//maybe _PRMT_SC_CT_COM_InternetService_MAXSession_ is only for rg
	#define _PRMT_SC_CT_COM_InternetService_MAXSession_		1
	#define _PRMT_SC_CT_COM_InternetService_UserQoS_		1
#endif //_PRMT_SC_CT_COM_InternetService_
#ifdef _PRMT_SC_CT_COM_GroupCompanyService_
	#define _PRMT_SC_CT_COM_GroupCompanyService_Plugin_	1
#endif
#ifdef _PRMT_SC_CT_COM_Device_
	#define _PRMT_SC_CT_COM_Device_LightSwitch_			1
#endif

	#define _PRMT_X_CT_CONTROL_PORT_SC	1
	#define _PRMT_X_CT_IPTRACEROUTE_SC	1
	#define TERMINAL_INSPECTION_SC	1
	#define _PRMT_SC_CT_COM_Ping_Enable_ 1
#endif //_PRMT_SC_CT_COM_

	#define _PRMT_X_CT_COM_MULTICAST_DIAGNOSIS_     1
	// FIXME: Need FC version
	//#define _PRMT_X_CT_COM_LANBINDING_CONFIG_ 		1

	#define EXTERNAL_ACCESS_SC 1
	#define _PRMT_X_CT_ACCESS_EQUIPMENTMAC 1
	#define _PRMT_X_CT_SUPPER_DHCP_LEASE_SC 1
	#define _PRMT_X_CT_COM_REGSTATISTICS_ 1
#endif
#ifdef SUPPORT_CLOUD_VR_SERVICE
	#define _PRMT_X_CT_CLOUD_VR_ 1
#endif
#endif //CONFIG_YUEME
#endif //_PRMT_X_CT_EXT_ENABLE_

#ifdef CONFIG_CU
#define _PRMT_X_CU_EXTEND_ 1
#define _SUPPORT_CAPTIVEPORTAL_PROFILE_		1
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
#define CONFIG_SUPPORT_CAPTIVE_PORTAL
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE) || defined(CONFIG_FIBER_FEATURE)
#define CONFIG_SUPPORT_PON_LINK_DOWN_PROMPT
#endif
#define _PRMT_C_CU_IGMP_ 1
#define  _PRMT_C_CU_FIREWALL_ 1
#define  _PRMT_C_CU_WEB_ 1
#define _PRMT_C_CU_LOGALARM_ 1
#define _PRMT_C_CU_TELNET_ 1
#define _PRMT_C_CU_DDNS_ 1
#define _PRMT_C_CU_DMZ_ 1
//#define _PRMT_C_CU_USERACCOUNT_ 1
//#define _PRMT_C_CU_FTPSERVICE_ 1
#ifdef CONFIG_USER_CUSPEEDTEST
#define _PRMT_C_CU_SPEEDTEST_ 1
#endif

#ifdef _PRMT_SERVICES_
#define _PRMT_C_CU_FTPSERVICE_ 1
#define _PRMT_C_CU_USERACCOUNT_ 1
#endif
#define _PRMT_C_CU_FACTORYRESET_ 1
#if defined(CONFIG_USER_CUMANAGEDEAMON) || defined(CONFIG_CU_BASEON_YUEME)
#define _PRMT_C_CU_SERVICEMGT_ 1
#endif
#define _PRMT_X_CU_DEVICEINFO_ 1
#define _PRMT_X_CU_MANAGEMENTSERVER_ 1
#define _PRMT_X_CU_COM_TIME_ 1
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE) || defined(CONFIG_FIBER_FEATURE)
#define _PRMT_X_CU_XPON_INTERFACE_CONFIG_ 1
#endif
//open for parent control
#define _PRMT_X_CMCC_SECURITY_ 1

#define CU_APP_SCHEDULE_LOG 1
#ifdef CONFIG_USER_CUMANAGEDEAMON
#define CU_CUMANAGEDEAMON_NEW_SPEC	1
#endif

#define _PRMT_X_CT_SUPPER_DHCP_LEASE_SC 1

#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_AP_QOS)
#ifdef WLAN_SUPPORT
	#define CONFIG_QOS_SUPPORT_WLAN_INTF 1
#endif
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_AP_E8B_QOS)
#ifndef CONFIG_QOS_SUPPORT_8_QUEUES
	#define CONFIG_QOS_SUPPORT_8_QUEUES 1
#endif
#endif

#if defined(CONFIG_CMCC)
#define SUPPORT_GET_MULTIPLE_LANHOST_IPV6_ADDR 1
#define GET_VENDOR_CLASS_ID 1
#endif

#if defined(CONFIG_CMCC)
#define CONFIG_UPSTREAM_ISOLATION 1
#define AUTO_SAVE_ALARM_CODE 1
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_00R0)
#define CWMP_ONE_WAN_CONNDEV_OBJ 1
#endif

#if defined(CONFIG_CMCC)
#ifdef CONFIG_CMCC_OSGIMANAGE
#define _PRMT_X_CMCC_JSON_ 1
#endif
#ifdef CONFIG_USER_OPENJDK8
#define _PRMT_X_CMCC_OSGI_ 1
#endif

#define _PRMT_X_CMCC_LEDCONTROL_ 1
#define _PRMT_X_CMCC_DEVICEINFO_ 1
#define _PRMT_X_CMCC_LAYER3FORWARDING_ 1
#ifdef WLAN_SUPPORT
#define _PRMT_X_CMCC_WLANSHARE_ 1
#define _PRMT_X_CMCC_WLANFORGUEST_ 1
#endif
#define _PRMT_X_CMCC_LANINTERFACES_ 1
#define _PRMT_X_CMCC_SECURITY_ 1
#define _PRMT_X_CMCC_IPOEDIAGNOSTICS_ 1
#ifdef WIFI_TIMER_SCHEDULE
#define _PRMT_X_CMCC_WLANSWITCHTC_ 1
#endif
#define _PRMT_X_CMCC_LANDEVICE_ 1
#define _PRMT_X_CMCC_AUTORESETCONFIG_ 1
#ifdef CONFIG_USER_VXLAN
#define _PRMT_X_CMCC_VXLAN_ 1
#endif
#ifdef CONFIG_CMCC_ENTERPRISE
#define _PRMT_X_CMCC_GUEST_PORTAL_ 1
#endif
#endif

#endif //CONFIG_USER_CWMP_TR069

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_AP_QOS)
#define _PRMT_X_CT_COM_DATA_SPEED_LIMIT_ 1
#if defined(CONFIG_RTK_SOC_RTL8198D) && defined(SBWC)
#define CONFIG_ETH_AND_WLAN_QOS_SUPPORT
#endif
#endif


#ifdef WEB_UPGRADE
#define	UPGRADE_V1			1
#endif // if WEB_UPGRADE

//ql add
#ifdef CONFIG_RESERVE_KEY_SETTING
#define	RESERVE_KEY_SETTING
#endif

#ifdef CONFIG_PPP
#undef WEB_ENABLE_PPP_DEBUG
#endif
// Mason Yu
#undef SEND_LOG

//xl_yue add,when finishing maintenance,inform ITMS to change password
#undef	FINISH_MAINTENANCE_SUPPORT

//xl_yue add,web logining is maintenanced by web server
#if defined(CONFIG_00R0) || defined(CONFIG_E8B) || defined(CONFIG_USER_LOGINWEB_PAGE) || defined(CONFIG_TRUE) || defined(CONFIG_TLKM)
#define USE_LOGINWEB_OF_SERVER
#else
#undef USE_LOGINWEB_OF_SERVER
#endif

#ifdef CONFIG_TELMEX_DEV
#define USE_LOGINWEB_OF_SERVER
#endif

#if defined(CONFIG_USER_LOGINWEB_CAPCTHA)
#define USE_CAPCTHA_OF_LOGINWEB
#else
#undef USE_CAPCTHA_OF_LOGINWEB
#endif

//#define USE_LOGINWEB_OF_SERVER

#ifdef USE_LOGINWEB_OF_SERVER
//xl_yue add,if have logined error for three times continuely,please relogin after 1 minute
#define LOGIN_ERR_TIMES_LIMITED 1
//xl_yue add,only one user can login with the same account at the same time
#ifndef CONFIG_DEONET_SESSION
#define ONE_USER_LIMITED	1
#ifdef CONFIG_TELMEX_DEV
#define ONE_USER_BY_SESSIONID 1
#endif
#endif
//xl_yue add,if no action lasting for 5 minutes,auto logout
#define AUTO_LOGOUT_SUPPORT	1
#if defined(CONFIG_00R0) || defined(CONFIG_TRUE)
#define USE_BASE64_MD5_PASSWD 1
#else
#undef USE_BASE64_MD5_PASSWD
#endif
#endif

#ifdef CONFIG_USER_LOGIN_WEB_PASSWORD_ENCRYPT
#ifdef USE_BASE64_MD5_PASSWD
#undef BOA_AUTH_WEB_PASSWORD_ENCRYPT
#else
#define BOA_AUTH_WEB_PASSWORD_ENCRYPT 1
#endif
#endif

#ifndef USE_LOGINWEB_OF_SERVER
//xl_yue add, not used
//#define ACCOUNT_LOGIN_CONTROL		1
#endif


/*######################*/
//jim 2007-05-22
//4 jim_luo Bridge Mode only access on web
//#define BRIDGE_ONLY_ON_WEB
#undef  BRIDGE_ONLY_ON_WEB

//4 E8-A unsupport save and restore configuration file, then should remark belwo macro CONFIG_SAVE_RESTORE
#define CONFIG_SAVE_RESTORE

//E8-A unsupport web upgrade image, we should enable #undef WEB_UPGRADE at line 52
/*########################*/

//add by ramen
//#define  DNS_BIND_PVC_SUPPORT
//#define	 DNSV6_BIND_PVC_SUPPORT
//#define  POLICY_ROUTING_DNSV4RELAY
#define  NAT_LOOPBACK

#define  DHCPS_POOL_COMPLETE_IP
#define  DHCPS_DNS_OPTIONS
#undef ACCOUNT_CONFIG
#undef MULTI_USER_PRIV
#ifdef MULTI_USER_PRIV
#define ACCOUNT_CONFIG
#endif
#ifdef CONFIG_CMCC_ENTERPRISE
#define ACCOUNT_CONFIG
#define CONFIG_INDEPENDENT_USER_LOGOUT_TIME
#endif
//#define SAMBA_SYSTEM_ACCOUNT

/*xl_yue:20090210 add cli cmdedit*/
#ifdef CONFIG_USER_CMD_CLI
#define CONFIG_CLI_CMD_EDIT
#define CONFIG_CLI_TAB_FEATURE
#endif

#if defined(WLAN_SUPPORT) && (defined(CONFIG_RTK_L34_ENABLE) ||defined(CONFIG_TELMEX_DEV))&& defined(CONFIG_FON_GRE)
#define SUPPORT_FON_GRE	1
#endif

#if (defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC))
#define SUPPORT_DHCPV6_RELAY
#endif
//added by ql to support imagenio service
//#define IMAGENIO_IPTV_SUPPORT		// base on option60 with option240~241

/*added by linus_pang, show cpu/mem utility in status.asp, 2016.3.9*/
#define CPU_UTILITY
#define MEM_UTILITY

// FIXME: Do we need FC version?
//#define SUPPORT_FON_GRE	1

#undef AUTO_DETECT_DMZ
// add by yq_zhou
#undef CONFIG_11N_SAGEM_WEB

// Magician
#define COMMIT_IMMEDIATELY

//cathy
#define USE_11N_UDP_SERVER

//support reserved IP addresses for DHCP, jiunming
#define SUPPORT_DHCP_RESERVED_IPADDR	1

#define CONFIG_IGMPPROXY_MULTIWAN

#ifdef CONFIG_RTK_DEV_AP
#ifdef CONFIG_RTL_MULTI_ETH_WAN
#ifndef CONFIG_MLDPROXY_MULTIWAN
#define CONFIG_MLDPROXY_MULTIWAN
#endif
#endif
#define LIMIT_MULTI_PPPOE_DIAL_ON_SAME_DEVICE
#define DISABLE_MICROSOFT_AUTO_IP_CONFIGURATION
#endif


//for FIELD_TRY_SAFE_MODE web control, need ADSL driver support
#undef FIELD_TRY_SAFE_MODE

#define DEBUG_MEMORY_CHANGE 0  // Magician: for something about memory debugging.
#ifdef CONFIG_IPV6
#define DUAL_STACK_LITE
#define CONFIG_IPV6_VPN
#endif

#undef ENABLE_ADSL_MODE_GINP

#if (defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU))
//define for yueme DNS port binding request.
#ifndef CONFIG_USER_DNSMASQ_DNSMASQ280
#ifdef CONFIG_SOCK_RCVMARK
#define DNSQUERY_SKBMARK_BINDING
#define SO_RCVMARK 101
#else
#define DNSQUERY_PORT_BINDING
#endif
#endif
#endif
// Mason Yu.
// Define all functions on boa for LUNA
#if defined(CONFIG_E8B) || defined(CONFIG_USER_GEN_WAN_MAC)
#define GEN_WAN_MAC
#endif

#ifdef CONFIG_E8B
#define CTC_WAN_NAME
#define VIRTUAL_SERVER_SUPPORT
#ifdef VIRTUAL_SERVER_SUPPORT
#if (defined(CONFIG_CMCC) || defined(CONFIG_CU))
#define VIRTUAL_SERVER_INTERFACE
#define VIRTUAL_SERVER_MODE_NTO1
#endif
#endif
#define IP_RANGE_FILTER_SUPPORT
#define WEB_AUTH_PRIVILEGE
#define SUPPORT_ACCESS_RIGHT

#ifdef CONFIG_E8B
//#define SUPPORT_WEB_REDIRECT //FIXME!! later~
#endif

//FIXME!! we need implement this feature based on FC structure.
//#define SUPPORT_WEB_PUSHUP
#ifdef CONFIG_YUEME
#define SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
#define WEB_REDIR_PORT_BY_FWUPGRADE		19080
#endif
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#define SUPPORT_WEB_PUSHUP //this feature is depend on CONFIG_RTK_L34_ENABLE, TODO in FC structure, now we open it for dbus FW upgrade. 20191009.
#define SUPPORT_INCOMING_FILTER
#ifdef URL_BLOCKING_SUPPORT
#ifndef CONFIG_CU
#define SUPPORT_URL_FILTER
#endif
#endif
#ifdef DOMAIN_BLOCKING_SUPPORT
#ifndef CONFIG_CU
#define SUPPORT_DNS_FILTER
#define SUPPORT_APPFILTER
#endif
#endif
#if (defined(SUPPORT_URL_FILTER) || defined(SUPPORT_DNS_FILTER)) && defined(YUEME_4_0_SPEC)
#define SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
#endif
#else //CONFIG_YUEME
#ifdef CONFIG_RTK_CTCAPD_FILTER_SUPPORT
#define SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
#define SUPPORT_URL_FILTER
#endif
#endif

#if !defined(CONFIG_CMCC) && !defined(CONFIG_CU_BASEON_CMCC)
#define CTCOM_WLAN_REQ		1   //CTCOM request tr069 wireless mssid entity can be add and del
#endif

#ifdef CONFIG_CMCC_ENTERPRISE
#define SUPPORT_TIME_SCHEDULE
#define DHCP_ARP_IGMP_RATE_LIMIT
#define QOS_USING_CLASSFICATION
#endif

#ifdef CONFIG_E8B
#define PROCESS_PERMISSION_LIMIT 1
#define PROCESS_PERMISSION_FILE "/tmp/process_permission"
#endif

#if defined(CONFIG_CMCC) && !defined(CONFIG_CMCC_ENTERPRISE) && !defined(CONFIG_USER_DHCPCLIENT_MODE)
#define SERVICE_BIND_IP
#endif

#ifdef SERVICE_BIND_IP
#ifndef CONFIG_RTK_DEV_AP
#define WEB_HTTP_SERVER_BIND_IP
#else
#define RTK_WEB_HTTP_SERVER_BIND_DEV
#endif

#define CWMP_HTTP_SERVER_BIND_IP
#define INETD_BIND_IP
#ifdef CONFIG_USER_SAMBA
#define SMBD_BIND_IP
#endif
#define OSGIAPP_BIND_IP
#endif

#ifdef INETD_BIND_IP
#define INETD_PID_FILE	"/var/run/inetd.pid"
#define INETD_BIND_CONF "/var/inetd_bind_addr.conf"
#define TELNET_BIND_IP
#endif

#ifdef WEB_HTTP_SERVER_BIND_IP
#define WEB_HTTP_SERVER_REBIND_FILE "/var/boa_rebind"
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU)
#define NESSUS_FILTER
#endif

#ifdef WLAN_SUPPORT
#ifdef WLAN_MBSSID
#if !(defined(WIFI5_WIFI6_COMP) && defined(CONFIG_RTK_DEV_AP))
#define WLAN_GEN_MBSSID_MAC
#endif
#endif
#endif

#define PROVINCE_SICHUAN_TRACEROUTE_TEST 0x1
#define PROVINCE_SICHUAN_RESETFACTORY_TEST 0x2
#define PROVINCE_SICHUAN_PORTCONTROL_TEST 0x4
#define PROVINCE_SICHUAN_TERMINAL_INSPECTION 0x8
#define PROVINCE_SICHUAN_PING_ENABLE 0x10
#endif //CONFIG_E8B

//FIXME: Need to support FC version
//#define SUPPORT_MCAST_TEST

#if  defined(CONFIG_APACHE_FELIX_FRAMEWORK)
#define OSGI_SUPPORT
#undef ENABLE_SIGNATURE_ADV // undefine and boa can upload over 1M file
#endif

#if defined(CONFIG_USER_WEB_WIZARD) && defined(CONFIG_00R0)
#define USER_WEB_WIZARD
#endif
//#define LOOP_LENGTH_METER
//#define _PRMT_NSLOOKUP_

/* Do NOT check HS and force to read it.
	Enable it if you need to keep HS unchanged all the time. */
#undef FORCE_HS	// enable for testing and debugging purpose

#if defined(SUPPORT_FON_GRE) || defined(CONFIG_XFRM)
#define SUPPORT_BYPASS_RG_FWDENGINE
#endif

#ifdef CONFIG_PUSHBUTTON_USE_EVENT_HANDLE //handle in systemd
/* 20230104, mkkwon, reset button */
/* /proc/load_default를 사용하기위해 */
#if 0
#define HANDLE_DEVICE_EVENT 1
#else
#undef HANDLE_DEVICE_EVENT
#endif
#ifdef CONFIG_WLAN_WIFIWPSONOFF_BUTTON
#define WLAN_WIFIWPSONOFF_BUTTON	1
#endif
#if defined(CONFIG_E8B) || defined(WIFI5_WIFI6_COMP) || defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
#ifdef WLAN_DUALBAND_CONCURRENT
#define WLAN_WPS_PUSHBUTTON_TRIGGER_ONE_INTERFACE	1
#endif
#endif
#ifdef WLAN_WIFIWPSONOFF_BUTTON
#define WPS_WLAN0_START_TIME 4
#define WPS_WLAN1_START_TIME 7
#else
#define WPS_WLAN0_START_TIME 1
#define WPS_WLAN1_START_TIME 4
#endif
#endif

#if defined(CONFIG_RTL_MESH_SINGLE_IFACE) && defined (CONFIG_RTK_DEV_AP) // for ap platform
#define WLAN_MESH_SINGLE_IFACE
#elif !defined(CONFIG_RTK_DEV_AP) // for dsl or pon platform
#define WLAN_MESH_SINGLE_IFACE
#endif

#ifdef CONFIG_WLAN_SCHEDULE_SUPPORT
#define WLAN_SCHEDULE_SUPPORT
#endif

//#define DEBUGPRINT  fprintf(stderr,"%s %d %s.............\n",__FILE__,__LINE__,__FUNCTION__);
#define DEBUGPRINT

#ifndef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE_MODULE
#define CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE 1
#endif
#endif

#ifdef CONFIG_USER_SLEEP_TIMER
#define SLEEP_TIMER
#endif

#ifdef CONFIG_LED_INDICATOR_TIMER
#define LED_TIMER
#endif

#define NEW_IPV6_GENERATION /* This flag is for E8B's other program use new IPv6 function. */

#define DHCP_USE_SYSTEM_UPTIME 1

/*swap wlan hs/cs mib*/
#if defined(CONFIG_RTK_DEV_AP) && defined(WLAN_DUALBAND_CONCURRENT)
#define SWAP_WLAN_MIB
#define SWAP_WLAN_MIB_AUTO
#endif

#if defined(CONFIG_CU_BASEON_CMCC)
#define CONFIG_USER_LAN_BANDWIDTH_EX_CONTROL
#define SUPPORT_SET_WANTYPE_FOR_STATIC_ROUTE
#define PON_HISTORY_TRAFFIC_MONITOR
#endif

#if defined(CONFIG_CU_BASEON_YUEME)

#define DHCPD_MULTI_THREAD_SUPPORT
#define PON_HISTORY_TRAFFIC_MONITOR
#define SUPPORT_LOG_TO_TEMP_FILE
#define SUPPORT_ACCELERATION_FEATURE
#define SUPPORT_SET_WANTYPE_FOR_STATIC_ROUTE

/* OPTIONS FOR DBUS INTERFACE*/
#define COM_CUC_IGD1_WIFIINFO
#define COM_CUC_IGD1_WPS
#define COM_CUC_IGD1_WLANCONFIGURATION
#define COM_CUC_IGD1_LANHOST_MANAGER
#define COM_CUC_IGD1_LANHOST
#define COM_CUC_IGD1_WANCONNECTION
#define COM_CUC_IGD1_VOIPINFO
#define COM_CUC_IGD1_PPPoEAccount
#define COM_CUC_IGD1_Layer3ForwardingIPv4Config
#define COM_CUC_IGD1_Layer3ForwardingIPv6Config
#define COM_CUC_IGD1_ParentalControlConfig
#ifdef CONFIG_CMCC_FORWARD_RULE_SUPPORT
#define COM_CUC_IGD1_TrafficForward
#endif
#define COM_CUC_IGD1_TrafficMirror
#ifdef CONFIG_CMCC_TRAFFIC_PROCESS_RULE_SUPPORT
#define COM_CUC_IGD1_TrafficDetailProcess
#endif
#define COM_CUC_IGD1_TrafficQoSFlow
#define COM_CUC_IGD1_TrafficMonitorConfig
#ifdef CONFIG_CU_FASTPATH_SPEEDUP_RULE_SUPPORT
#define COM_CUC_IGD1_FastPathSpeedUp
#endif
#if defined(WLAN_SUPPORT)
#if defined(CONFIG_USER_WMM_SERVICE)
#define COM_CUC_IGD1_WMMConfiguration
#elif defined(CONFIG_USER_WMM_SERVICE_IPSET)
#define COM_CUC_IGD1_WMMConfiguration_ipset
#define COM_CUC_IGD1_WMMProfile_ipset
#endif
#endif
#ifdef SUPPORT_ACCELERATION_FEATURE
#define COM_CUC_IGD1_IPSet
#define COM_CUC_IGD1_NETWORK_ACCELERATION
#define COM_CUC_IGD1_L2TPVPN
#define COM_CUC_IGD1_TrafficControl
#define COM_CUC_IGD1_TrafficControlProfile
#endif
#define COM_CUC_IGD1_NASACCESS
#define COM_CUC_IGD1_AUTODIAG
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#define SUPPORT_ZERO_CONFIG
#endif

#ifdef CONFIG_RTK_DEV_AP
#if defined(CONFIG_E8B) && defined(CONFIG_CMCC)
#define CONFIG_DEFAULT_IPOE_BRIDGE_WAN
#endif
#endif

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
#if defined(CONFIG_LUNA_G3_SERIES)
#define MAX_WAN_LINK_RATE 32767999 //(Kbps) 10G
#else
#define MAX_WAN_LINK_RATE 2097144 //(Kbps) 2.5G
#endif
#elif defined(CONFIG_RTK_DEV_AP)
#if defined(CONFIG_RTL_G3LITE_QOS_SUPPORT)
#define MAX_WAN_LINK_RATE 2500000 // 8226 2.5Gbps
#else
#define MAX_WAN_LINK_RATE 1048568 // 0x1FFFF
#endif
#else
#define MAX_WAN_LINK_RATE 1024000 //(Kbps) 1G
#endif

#ifdef CONFIG_E8B
#if !defined(CONFIG_RTK_DEV_AP)
#define SUPPORT_USEREG_FEATURE
#else
#undef SUPPORT_USEREG_FEATURE
#endif
#endif

#ifdef CONFIG_RTL_SMUX_LEGACY
#if defined(CONFIG_RTK_DEV_AP)
#define SUPPORT_PORT_BINDING
#endif
#endif

#ifdef CONFIG_RTIC
#define WLAN_RTIC_SUPPORT
#endif

#ifdef CONFIG_USER_LOCK_NET_FEATURE_SUPPORT
#define RTK_BOA_PORTAL_TO_NET_LOCKED_UI
#endif

#ifdef CONFIG_RTK_DEV_AP
#define CONFIG_USER_RETRY_GET_FLOCK
#endif


//MP feature support
#if (!defined(CONFIG_RF_DPK_SETTING_SUPPORT) && defined(CONFIG_RTL8192FE_MP_SUPPORT))
#define CONFIG_RF_DPK_SETTING_SUPPORT
#endif

#ifdef CONFIG_CU
#define CU_APPSCAN_RSP_DBG
#define CONFIG_QOS_SUPPORT_DOWNSTREAM 1
#if defined(CONFIG_USER_CUSPEEDTEST) && defined(CONFIG_RTK_HOST_SPEEDUP)
#define SUPPORT_UPLOAD_SPEEDTEST
#endif
#endif

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
#define EXTERNAL_SWITCH_PORT_OFFSET CONFIG_EXTERNAL_SWITCH_PORT_OFFSET
#endif

#if defined(CONFIG_CU)
#define DHCP_VENDORCLASSID_LEN 120
#else
#define DHCP_VENDORCLASSID_LEN 36
#endif
#ifdef CONFIG_CMCC
#define CONFIG_WAN_INTERVAL_TRAFFIC
#endif

#endif  // INCLUDE_OPTIONS_H



#ifndef SUBR_WLAN_H
#define SUBR_WLAN_H

#include "utility.h"
#ifdef WLAN_USE_DRVCOMMON_HEADER
#ifdef WLAN_11AX
#include <include/wifi_common.h>
#else
#include <wifi_common.h>
#endif
#endif //WLAN_USE_DRVCOMMON_HEADER

#ifdef WIFI5_WIFI6_COMP
#include <ieee802_mib.h>
#else
#ifndef WLAN_11AX
#include <ieee802_mib.h>
#endif
#endif

/* ------------- IOCTL STUFF FOR 802.1x DAEMON--------------------- */

#define RTL8192CD_IOCTL_GET_MIB 0x89f2
#define RTL8192CD_IOCTL_DEL_STA	0x89f7
#define SIOCGIWIND      0x89fc
#define SIOCGIWRTLSTAINFO  0x8B30
#define SIOCGIWRTLSTANUM                0x8B31  // get the number of stations in table
#define SIOCGIWRTLDRVVERSION            0x8B32
#define SIOCGIWRTLSCANREQ               0x8B33  // scan request
#define SIOCGIWRTLGETBSSDB              0x8B34  // get bss data base
#define SIOCGIWRTLJOINREQ               0x8B35  // join request
#define SIOCGIWRTLJOINREQSTATUS         0x8B36  // get status of join request
#define SIOCGIWRTLGETBSSINFO            0x8B37  // get currnet bss info
#define SIOCGIWRTLGETWDSINFO            0x8B38
#define SIOCGIWRTLSTAEXTRAINFO			0x8B40
#define SIOCMIBINIT             0x8B42
#define SIOCMIBSYNC             0x8B43
#define SIOCGMISCDATA   0x8B48  //get_misc_data
#define SIOC92DAUTOCH	0x8BC5 // manual auto channel
#define SIOCSSREQ		0x8B5C
#define SIOCGIWRTLSSSCORE	0x9002
#define SIOCGISETBCNVSIE	0x9003
#define SIOCGISETPRBVSIE	0x9004
#define SIOCGISETPRBRSPVSIE	0x900E

#define RTK_IOCTL_STARTPROBE		0x8BF8
#define RTK_IOCTL_STOPPROBE			0x8BF9
#define RTK_IOCTL_PROBEINFO			0x8BFA
#define RTK_IOCTL_START_ALLCHPROBE	0x8BFB
#define RTK_IOCTL_SCANSTA 			0x8BFC
#define RTK_IOCTL_GETUNASSOCSTA 0x900B

#define SIOCGIROAMINGBSSTRANSREQ	0x9007

#define SIOC11KBEACONREQ            0x8BD2
#define SIOC11KBEACONREP            0x8BD3

#define HT_RATE_ID			0x80
#define VHT_RATE_ID			0xA0

#ifdef WLAN_MBSSID
#define MIN_WLAN_BEACON_INTERVAL 100
#else
#define MIN_WLAN_BEACON_INTERVAL 20
#endif

extern int wlan_idx;
extern const char *WLANIF[];
#ifdef WIFI5_WIFI6_COMP
extern const char* WLAN_VXD_IF[];
#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
extern const char WLAN_MONITOR_DAEMON[];
#endif
extern const char IWPRIV[];
extern const char AUTH_DAEMON[];
extern const char IWCONTROL[];
extern const char AUTH_PID[];
extern const char WLAN_AUTH_CONF[];
extern const char WSC_DAEMON_PROG[];
extern const char *wlan_encrypt[];
#ifdef WLAN_WDS
extern const char WDSIF[];
#endif
extern char *WLANAPIF[];
extern const char WLAN_DEV_PREFIX[]; 

extern const char *wlan_band[];
extern const char *wlan_mode[];
extern const char *wlan_rate[];
extern const char *wlan_auth[];
extern const char *wlan_preamble[];
//extern const char *wlan_encrypt[];
extern const char *wlan_pskfmt[];
extern const char *wlan_wepkeylen[];
extern const char *wlan_wepkeyfmt[];
extern const char *wlan_Cipher[];

//#define ISP_API_DEBUG_ENABLE
#ifdef ISP_API_DEBUG_ENABLE
#define ISP_API_DEBUG(fmt, arg...)					do{printf("[%s:%d]"fmt"\n", __FUNCTION__, __LINE__, ##arg);}while(0)
#else
#define ISP_API_DEBUG(fmt, arg...)					do{}while(0)
#endif
#define ISP_API_GOTO_LABLE_ON(condition, lable)		do{ISP_API_DEBUG();if(condition) goto lable;}while(0);
#define ISP_API_EXIT_ON(condition)					do{ISP_API_DEBUG();if(condition) return;}while(0);
#define ISP_API_EXIT_WITH_VAL_ON(condition, r_val)	do{ISP_API_DEBUG();if(condition) return r_val;}while(0);

typedef struct rtk_wlan_auth_info {
	unsigned char  __enable_1x;	//1:enable 802.1X
	
#ifdef CONFIG_RTL_WAPI_SUPPORT
	unsigned char  __wapi_auth;
#endif

	union{
		unsigned char  __auth_type;	//enum AUTH_TYPE_T for WEP, enum WPA_AUTH_T for WPA/WPA2/WPA-mix
		struct {
			unsigned char  __rs_addr[4];	//Radius IP addr
			unsigned short __rs_port;		//Radius port
			unsigned char  __rs_psw[MAX_RS_PASS_LEN];	//Radius password
		} __wlan_rs_info;
	};
}RTK_WLAN_AUTH_INFO;

typedef struct rtk_wlan_encrypt_info {
	//type
	unsigned char 		  encrypt_type;	//RW, enum ENCRYPT_T

	//authentication
	RTK_WLAN_AUTH_INFO 	  __auth_info;	//RW, authentication information

	//encryption
	union{
		struct {
			unsigned char __wep_t;	//enum WEP_T
			unsigned char __wep_key_type;	//enum KEY_TYPE_T
			unsigned char __wep_key64[WEP64_KEY_LEN];
			unsigned char __wep_key128[WEP128_KEY_LEN];
		} __wep_info;
		struct {
			unsigned char __wpa_cipher_suite;	//enum WPA_CIPHER_T
			unsigned char __wpa2_cipher_suite;	//enum WPA_CIPHER_T
			unsigned char __wpa_psk_format;		//enum PASK_FORMAT_T
			unsigned char __wpa_key[MAX_PSK_LEN+1];
		} __wpa_info;
#ifdef CONFIG_RTL_WAPI_SUPPORT
		struct {
			unsigned char __wapi_psk[MAX_PSK_LEN+1];
			unsigned char __wapi_psk_len;
			unsigned char __wapi_psk_format;    //enum PASK_FORMAT_T
			unsigned char __wapi_as_ipaddr[4];
		} __wapi_info;
#endif
	};

#ifdef CONFIG_RTL_WAPI_SUPPORT
#define rtk_api_wapi_auth		__auth_info.__wapi_auth
#endif
#define rtk_api_enable_1x		__auth_info.__enable_1x
#define rtk_api_auth_type		__auth_info.__auth_type
#define rtk_api_rs_addr			__auth_info.__wlan_rs_info.__rs_addr
#define rtk_api_rs_port			__auth_info.__wlan_rs_info.__rs_port
#define rtk_api_rs_pass			__auth_info.__wlan_rs_info.__rs_psw

#define rtk_api_wep_key_len 	__wep_info.__wep_t
#define rtk_api_wep_key_type	__wep_info.__wep_key_type
#define rtk_api_wep_key64		__wep_info.__wep_key64
#define rtk_api_wep_key128		__wep_info.__wep_key128

#define rtk_api_wpa_cipher_suite __wpa_info.__wpa_cipher_suite
#define rtk_api_wpa2_cipher_suite __wpa_info.__wpa2_cipher_suite
#define rtk_api_wpa_psk_format	__wpa_info.__wpa_psk_format
#define rtk_api_wpa_key			__wpa_info.__wpa_key

#ifdef CONFIG_RTL_WAPI_SUPPORT
#define rtk_api_wapi_key		__wapi_info.__wapi_psk
#define rtk_api_wapi_key_len	__wapi_info.__wapi_psk_len
#define rtk_api_wapi_key_format	__wapi_info.__wapi_psk_format
#define rtk_api_wapi_as_ipaddr	__wapi_info.__wapi_as_ipaddr
#endif
}RTK_WLAN_ENCRYPT_INFO;

extern int rtk_wlan_get_encryption(unsigned char *ifname, RTK_WLAN_ENCRYPT_INFO *pEncInfo);
extern int rtk_wlan_set_encryption(unsigned char *ifname, RTK_WLAN_ENCRYPT_INFO encInfo);


#define MAX_REQUEST_IE_LEN          16
#define MAX_AP_CHANNEL_REPORT       4
#define MAX_AP_CHANNEL_NUM          8
#ifdef WLAN_11AX
#define MAX_BEACON_REPORT 			16
#else
#define MAX_BEACON_REPORT 			64
#endif
#define MAX_BEACON_SUBLEMENT_LEN           226
#ifdef WLAN_11AX
#define MAX_PROBE_REQ_STA 			48
#else
#define MAX_PROBE_REQ_STA 			64
#endif
#define MAX_UNASSOC_STA				48

#define BEACON_VSIE_LEN 250
#define PRB_REQ_VSIE_LEN 250

#define NUM_BCNVSIE_SERVICE 3
#define MAX_NUM_BCNVSIE_SERVICE 8

#define NUM_PRBVSIE_SERVICE 16

#define SSID_LEN	32
//#if defined(WLAN_CLIENT) || defined(WLAN_SITESURVEY)
#define	MAX_BSS_DESC	64
#define MESH_ID_LEN 32

#define MINUTES_OF_A_DAY 1440

typedef struct _OCTET_STRING {
    unsigned char *Octet;
    unsigned short Length;
} OCTET_STRING;
typedef enum _BssType {
    infrastructure = 1,
    independent = 2,
} BssType;
typedef	struct _IbssParms {
    unsigned short	atimWin;
} IbssParms;

/* WLAN sta info structure */
//should sync. with the struct in wlan driver
#ifdef WLAN_USE_DRVCOMMON_HEADER
//use wifi_common.h
#ifdef WLAN_11AX
#ifdef WIFI5_WIFI6_COMP
typedef sta_info_2_web WLAN_STA_INFO_T, *WLAN_STA_INFO_Tp;
#else
typedef sta_entry_2_web WLAN_STA_INFO_T, *WLAN_STA_INFO_Tp;
#endif
#else
typedef sta_info_2_web WLAN_STA_INFO_T, *WLAN_STA_INFO_Tp;
#endif
#else
#ifdef WLAN_11AX
typedef struct _sheet_hdr_2_web {
	unsigned char  sheet_sequence;
	unsigned char  sheet_total;
#ifdef WLAN_DRVCOMMON_HEADER
} __PACK__ sheet_hdr_2_web;
#else
} sheet_hdr_2_web;
#endif

#ifdef WLAN_DRVCOMMON_HEADER
//sync from wifi_common.h
#ifdef WIFI5_WIFI6_COMP
typedef struct wlan_sta_info {
	unsigned short	aid;
	unsigned char	addr[MAC_ADDR_LEN];
	unsigned long	tx_packets;
	unsigned long	rx_packets;
	unsigned long	expired_time;  // 10 msec unit
	unsigned short	flags;
	unsigned char	TxOperaRate;
	unsigned char	rssi;
	unsigned long	link_time;     // 1 sec unit
	unsigned long	idle_time;
	unsigned long	tx_fail;
	unsigned long long	tx_bytes;
	unsigned long long	rx_bytes;
	unsigned char	network;
	unsigned char	ht_info;       // bit0: 0=20M mode, 1=40M mode; bit1: 0=longGI, 1=shortGI
	unsigned char	RxOperaRate;
	unsigned char	auth_type;
	unsigned char	enc_type;
	unsigned char	snr;
	unsigned char	status_support;
	unsigned char	resv_1;
	unsigned short	acTxOperaRate;
	unsigned char	multiap_profile;
} __PACK__ WLAN_STA_INFO_T, *WLAN_STA_INFO_Tp;
#else
typedef struct wlan_sta_info {
	unsigned short		aid;
	unsigned char		addr[MAC_ADDR_LEN];
	unsigned int		link_time;
	unsigned int		expired_time;
	unsigned short		flags;
	unsigned short		tx_op_rate;
	unsigned short		rx_op_rate;
	unsigned char		tx_gi_ltf;
	unsigned char		rx_gi_ltf;
	unsigned long long	tx_packets;
	unsigned long long	rx_packets;
	unsigned long long	tx_fails;
	unsigned long long	tx_bytes;
	unsigned long long	rx_bytes;
	unsigned char		channel_bandwidth;
	unsigned char		rssi;
	unsigned char		signal_quality;
	unsigned char		wireless_mode; /* a/b/g/n/ac/ax */
	unsigned int		encrypt;       /* open/WEP/WPA/RSN */
	unsigned int		pairwise_cipher;
	unsigned int		group_cipher;
	unsigned int		akm;
	unsigned char		tln_stats_resv[2];
	unsigned char		status_support;
	unsigned char		multi_ap_profile;
	unsigned char		rm_cap;
	unsigned char		btm_support;
	unsigned char		resv_for_future[38]; /* for future use */
} __PACK__ WLAN_STA_INFO_T, *WLAN_STA_INFO_Tp;
#endif //WIFI5_WIFI6_COMP
#else
#ifdef WIFI5_WIFI6_COMP
typedef struct wlan_sta_info {
	unsigned short  aid;
	unsigned char   addr[6];
	unsigned long   tx_packets;
	unsigned long   rx_packets;
	unsigned long	expired_time;  // 10 mini-sec
	unsigned short  flags;
	unsigned char   TxOperaRate;
	unsigned char	rssi;
	unsigned long	link_time;		// 1 sec unit
	unsigned long	tx_fail;
#ifdef CONFIG_RTK_DEV_AP
	unsigned long	tx_bytes;
	unsigned long	rx_bytes;
#else
	unsigned long long	tx_bytes;
	unsigned long long	rx_bytes;
#endif
	unsigned char network;
	unsigned char ht_info;	// bit0: 0=20M mode, 1=40M mode; bit1: 0=longGI, 1=shortGI; bit2: 80M mode; bit3: 160M mode
	unsigned char	RxOperaRate;
	unsigned char	snr;
	unsigned char	resv[2];
	unsigned short	acTxOperaRate;
#if defined(RTK_MULTI_AP)
	unsigned char   multiap_profile;
#endif
} WLAN_STA_INFO_T, *WLAN_STA_INFO_Tp;

#else
typedef struct wlan_sta_info {
	unsigned short  	aid;
	unsigned char   	addr[MAC_ADDR_LEN];
	unsigned int		link_time;		// 1 sec unit
	unsigned int		expired_time;  // 10 mini-sec
	unsigned short  	flags;
	unsigned short		tx_op_rate;
	unsigned short		rx_op_rate;
	unsigned char		tx_gi_ltf;
	unsigned char		rx_gi_ltf;
	unsigned long long	tx_packets;
	unsigned long long	rx_packets;
	unsigned long long	tx_fails;
	unsigned long long	tx_bytes;
	unsigned long long	rx_bytes;
	unsigned char		channel_bandwidth;
	unsigned char		rssi;
	unsigned char		signal_quality;
	unsigned char		wireless_mode;
	unsigned int		encrypt;
	unsigned int		pairwise_cipher;
	unsigned int		group_cipher;
	unsigned int		akm;
#ifdef CONFIG_TRUE
	unsigned char		snr;
	unsigned char		resv[2];
#else
	unsigned char		resv[3];
#endif
	unsigned char		multi_ap_profile_resv;
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	unsigned char		rm_cap;
	unsigned char		btm_support;
#else
	unsigned char		rm_cap_resv;
	unsigned char		btm_support_resv;
#endif
	unsigned char		resv_for_future[38];
}WLAN_STA_INFO_T, *WLAN_STA_INFO_Tp;
#endif //WIFI5_WIFI6_COMP
#endif //WLAN_DRVCOMMON_HEADER
#else
#ifdef WLAN_DRVCOMMON_HEADER
//sync from wifi_common.h
typedef struct wlan_sta_info {
	unsigned short	aid;
	unsigned char	addr[MAC_ADDR_LEN];
	unsigned long	tx_packets;
	unsigned long	rx_packets;
	unsigned long	expired_time;  // 10 msec unit
	unsigned short	flags;
	unsigned char	TxOperaRate;
	unsigned char	rssi;
	unsigned long	link_time;     // 1 sec unit
	unsigned long	idle_time;
	unsigned long	tx_fail;
	unsigned long long	tx_bytes;
	unsigned long long	rx_bytes;
	unsigned char	network;
	unsigned char	ht_info;       // bit0: 0=20M mode, 1=40M mode; bit1: 0=longGI, 1=shortGI
	unsigned char	RxOperaRate;
	unsigned char	auth_type;
	unsigned char	enc_type;
	unsigned char	snr;
	unsigned char	status_support;
	unsigned char	resv_1;
	unsigned short	acTxOperaRate;
	unsigned char	multiap_profile;
} __PACK__ WLAN_STA_INFO_T, *WLAN_STA_INFO_Tp;
#else
typedef struct wlan_sta_info {
	unsigned short  aid;
	unsigned char   addr[6];
	unsigned long   tx_packets;
	unsigned long   rx_packets;
	unsigned long	expired_time;  // 10 mini-sec
	unsigned short  flags;
	unsigned char   TxOperaRate;
	unsigned char	rssi;
	unsigned long	link_time;		// 1 sec unit
	unsigned long	tx_fail;
	unsigned long long	tx_bytes;
	unsigned long long	rx_bytes;
	unsigned char network;
	unsigned char ht_info;	// bit0: 0=20M mode, 1=40M mode; bit1: 0=longGI, 1=shortGI; bit2: 80M mode; bit3: 160M mode
	unsigned char	RxOperaRate;
#ifdef CONFIG_TRUE
	unsigned char	snr;
	unsigned char	resv[2];
#else
	unsigned char	resv[3];
#endif
	unsigned short	acTxOperaRate;
#if defined(RTK_MULTI_AP)
	unsigned char   multiap_profile;
#endif
} WLAN_STA_INFO_T, *WLAN_STA_INFO_Tp;
#endif //WLAN_DRVCOMMON_HEADER
#endif //WLAN_11AX
#endif //WLAN_USE_DRVCOMMON_HEADER
#ifdef WLAN_USE_DRVCOMMON_HEADER
//use wifi_common.h
typedef sta_extra_info_2_web WLAN_STA_EXTRA_INFO_T, *WLAN_STA_EXTRA_INFO_Tp;
#else
#ifdef WLAN_DRVCOMMON_HEADER
//sync from wifi_common.h
typedef struct _wlan_sta_extra_info {
	unsigned short aid;
	unsigned char  addr[MAC_ADDR_LEN];
	unsigned long  tx_packets;
	unsigned long  rx_packets;
	unsigned long  expired_time;  // 10 msec unit
	unsigned short flags;
	unsigned char  TxOperaRate;
	unsigned char  rssi;
	unsigned long  link_time;     // 1 sec unit
	unsigned long  idle_time;
	unsigned long  tx_fail;
	unsigned long long  tx_bytes;
	unsigned long long  rx_bytes;
	unsigned char  network;
	unsigned char  ht_info;       // bit0: 0=20M mode, 1=40M mode; bit1: 0=longGI, 1=shortGI
	unsigned char  RxOperaRate;
	unsigned char  auth_type;
	unsigned char  enc_type;
	unsigned char  resv_1;
	unsigned short acTxOperaRate;
	unsigned char  client_host_name[256];
	unsigned char  client_host_ip[4];
	unsigned int   tx_bytes_1s;
	unsigned int   rx_bytes_1s;
	char           rxrate[10];
	char           txrate[10];
} __PACK__ WLAN_STA_EXTRA_INFO_T, *WLAN_STA_EXTRA_INFO_Tp;
#else
typedef struct _sta_extra_info_2_web {
	unsigned short  aid;
	unsigned char   addr[6];
	unsigned long   tx_packets;
	unsigned long   rx_packets;
	unsigned long   expired_time;   // 10 msec unit
	unsigned short  flags;
	unsigned char   TxOperaRate;
	unsigned char   rssi;
	unsigned long   link_time;              // 1 sec unit
#if 0 // defined(__OSK__) && defined(CONFIG_FON)
	unsigned long idle_time;
#endif
	unsigned long   tx_fail;
	unsigned long long tx_bytes;
	unsigned long long rx_bytes;
	unsigned char   network;
	unsigned char   ht_info;                // bit0: 0=20M mode, 1=40M mode; bit1: 0=longGI, 1=shortGI
	unsigned char   RxOperaRate;
#if 0 //def TLN_STATS
	unsigned char   auth_type;
	unsigned char   enc_type;
	unsigned char   resv[1];
#else
	unsigned char   resv[3];
#endif
	unsigned short  acTxOperaRate;
#ifdef CONFIG_RECORD_CLIENT_HOST
	unsigned char   client_host_name[256];
	unsigned char   client_host_ip[4];
#endif
//#if defined(RTK_ATM) || defined(RTK_CURRENT_RATE_ACCOUNTING)
	unsigned int tx_bytes_1s;
	unsigned int rx_bytes_1s;
	char rxrate[10];
	char txrate[10];
//#endif
} WLAN_STA_EXTRA_INFO_T, *WLAN_STA_EXTRA_INFO_Tp;
#endif //WLAN_DRVCOMMON_HEADER
#endif//WLAN_USE_DRVCOMMON_HEADER

//use ieee802_mib.h struct bss_desc instead
#if 0
typedef struct _BssDscr {
    unsigned char bdBssId[6];
    unsigned char bdSsIdBuf[SSID_LEN];
    OCTET_STRING  bdSsId;
    unsigned char	meshid[MESH_ID_LEN];
    unsigned char	*meshidptr;			// unused, for backward compatible
    unsigned short	meshidlen;
    BssType bdType;
    unsigned short bdBcnPer;			// beacon period in Time Units
    unsigned char bdDtimPer;			// DTIM period in beacon periods
    unsigned long bdTstamp[2];			// 8 Octets from ProbeRsp/Beacon
    IbssParms bdIbssParms;			// empty if infrastructure BSS
    unsigned short bdCap;				// capability information
    unsigned char ChannelNumber;			// channel number
    unsigned long bdBrates;
    unsigned long bdSupportRates;
    unsigned char bdsa[6];			// SA address
    unsigned char rssi, sq;			// RSSI and signal strength
    unsigned char network;			// 1: 11B, 2: 11G, 4:11G
    // P2P_SUPPORT
    unsigned char p2pdevname[33];
    unsigned char p2prole;
    unsigned short p2pwscconfig;
    unsigned char p2paddress[6];
    unsigned char stage;			//for V3.3
} BssDscr, *pBssDscr;
#endif
#ifdef WLAN_11AX
#ifdef WLAN_USE_DRVCOMMON_HEADER
//use wifi_common.h
#else
typedef struct _bss_desc_2_web {
	unsigned char  bssid[MAC_ADDR_LEN];
	unsigned char  ssid[33];
	unsigned char  ssidlen;
	unsigned char  mesh_id[33];
	unsigned char  mesh_id_length;
	unsigned char  channel;
	unsigned char  channel_bandwidth;
	unsigned char  rssi;
	unsigned char  signal_quality;
	unsigned short capability;
	unsigned short beacon_period;
	unsigned short atim_window;
	unsigned char  dtim_period;
	unsigned short support_rate;
	unsigned short basic_rate;
	unsigned char  infra_mode;    /* Infrastructure/IBSS */
	unsigned char  wireless_mode; /* a/b/g/n/ac/ax */
	unsigned int   encrypt;       /* open/WEP/WPA/RSN */
	unsigned int   pairwise_cipher;
	unsigned int   group_cipher;
	unsigned int   akm;
	unsigned int   time_stamp;
	//add after V1.1.2.2
	unsigned char  network_type;
	//after V2.0.0
	unsigned char mfp_opt;
#ifdef WLAN_DRVCOMMON_HEADER
} __PACK__ bss_desc_2_web;
#else
} bss_desc_2_web;
#endif
#endif //WLAN_USE_DRVCOMMON_HEADER
#ifdef WIFI5_WIFI6_COMP
typedef bss_desc_2_web BssDscr_ax, *pBssDscr_ax;
typedef struct bss_desc BssDscr, *pBssDscr;
#else
typedef bss_desc_2_web BssDscr, *pBssDscr;
#endif
#else
typedef struct bss_desc BssDscr, *pBssDscr;
#endif
typedef struct _sitesurvey_status {
    unsigned char number;
    unsigned char pad[3];
    BssDscr bssdb[MAX_BSS_DESC];
} __attribute__((packed,aligned(__alignof__(BssDscr)))) SS_STATUS_T, *SS_STATUS_Tp;


#ifdef WLAN_SITE_SURVEY_SIMUL
typedef struct _sitesurvey_status_total {
    unsigned char number;
    unsigned char pad[3];
    BssDscr bssdb[MAX_BSS_DESC*NUM_WLAN_INTERFACE];
} SS_STATUS_Total_T, *SS_STATUS_Total_Tp;
#endif

typedef enum _Capability {
    cESS 		= 0x01,
    cIBSS		= 0x02,
    cPollable		= 0x04,
    cPollReq		= 0x01,
    cPrivacy		= 0x10,
    cShortPreamble	= 0x20,
} Capability;
#ifdef WLAN_11AX
typedef enum _NDIS_802_11_NETWORK_INFRASTRUCTURE {
	Ndis802_11IBSS,
	Ndis802_11Infrastructure,
	Ndis802_11AutoUnknown,
	Ndis802_11InfrastructureMax,     /* Not a real value, defined as upper bound */
	Ndis802_11APMode,
	Ndis802_11Monitor,
	Ndis802_11_mesh,
} NDIS_802_11_NETWORK_INFRASTRUCTURE;
enum wlan_mode {
	WLAN_MD_INVALID = 0,
	WLAN_MD_11B	= 0x01,
	WLAN_MD_11A	= 0x02,
	WLAN_MD_11G	= 0x04,
	WLAN_MD_11N	= 0x08,
	WLAN_MD_11AC	= 0x10,
	WLAN_MD_11AX	= 0x20,
};

#ifdef WIFI5_WIFI6_COMP
typedef enum {
	ENCRYP_PROTOCOL_OPENSYS,   /* open system */
	ENCRYP_PROTOCOL_WEP,       /* WEP */
	ENCRYP_PROTOCOL_WPA,       /* WPA */
	ENCRYP_PROTOCOL_RSN,       /* RSN(WPA2/WPA3) */
	ENCRYP_PROTOCOL_WAPI,      /* WAPI: Not support in this version */
	ENCRYP_PROTOCOL_MAX
} ENCRYP_PROTOCOL_E;
#else
typedef enum {
	ENCRYP_PROTOCOL_OPENSYS,   /* open system */
	ENCRYP_PROTOCOL_WEP,       /* WEP */
	ENCRYP_PROTOCOL_WPA,       /* WPA */
	ENCRYP_PROTOCOL_WPA2,      /* WPA2 */
	ENCRYP_PROTOCOL_WAPI,      /* WAPI: Not support in this version */
	ENCRYP_PROTOCOL_MAX
} ENCRYP_PROTOCOL_E;
#endif
#define _WPA_CIPHER_NONE	(1<<0)
#define _WPA_CIPHER_WEP40 (1<<1)
#define _WPA_CIPHER_WEP104 (1<<2)
#define _WPA_CIPHER_TKIP	(1<<3)
#define _WPA_CIPHER_CCMP	(1<<4)
#define _WPA_CIPHER_GCMP	(1<<5)
#define _WPA_CIPHER_GCMP_256	(1<<6)
#define _WPA_CIPHER_CCMP_256	(1<<7)
#define _WPA_CIPHER_BIP_CMAC_128	(1<<8)
#define _WPA_CIPHER_BIP_GMAC_128	(1<<9)
#define _WPA_CIPHER_BIP_GMAC_256	(1<<10)
#define _WPA_CIPHER_BIP_CMAC_256	(1<<11)

#define WLAN_AKM_TYPE_8021X (1<<0)
#define WLAN_AKM_TYPE_PSK (1<<1)
#define WLAN_AKM_TYPE_FT_8021X (1<<2)
#define WLAN_AKM_TYPE_FT_PSK (1<<3)
#define WLAN_AKM_TYPE_8021X_SHA256 (1<<4)
#define WLAN_AKM_TYPE_PSK_SHA256 (1<<5)
#define WLAN_AKM_TYPE_TDLS (1<<6)
#define WLAN_AKM_TYPE_SAE (1<<7)
#define WLAN_AKM_TYPE_FT_OVER_SAE (1<<8)
#define WLAN_AKM_TYPE_8021X_SUITE_B (1<<9)
#define WLAN_AKM_TYPE_8021X_SUITE_B_192 (1<<10)
#define WLAN_AKM_TYPE_FILS_SHA256 (1<<11)
#define WLAN_AKM_TYPE_FILS_SHA384 (1<<12)
#define WLAN_AKM_TYPE_FT_FILS_SHA256 (1<<13)
#define WLAN_AKM_TYPE_FT_FILS_SHA384 (1<<14)

static inline int rtk_wlan_akm_wpa_ieee8021x(int akm)
{
	return !!(akm & (WLAN_AKM_TYPE_8021X |
			 WLAN_AKM_TYPE_FT_8021X |
			 /*WPA_KEY_MGMT_FT_IEEE8021X_SHA384 |
			 WPA_KEY_MGMT_CCKM |
			 WPA_KEY_MGMT_OSEN | */
			 WLAN_AKM_TYPE_8021X_SHA256 |
			 WLAN_AKM_TYPE_8021X_SUITE_B |
			 WLAN_AKM_TYPE_8021X_SUITE_B_192 |
			 WLAN_AKM_TYPE_FILS_SHA256 |
			 WLAN_AKM_TYPE_FILS_SHA384 |
			 WLAN_AKM_TYPE_FT_FILS_SHA256 |
			 WLAN_AKM_TYPE_FT_FILS_SHA384));
}

static inline int rtk_wlan_akm_wpa_psk(int akm)
{
	return !!(akm & (WLAN_AKM_TYPE_PSK |
			 WLAN_AKM_TYPE_FT_PSK |
			 WLAN_AKM_TYPE_PSK_SHA256 /*|
			 WLAN_AKM_TYPE_SAE |
			 WLAN_AKM_TYPE_FT_OVER_SAE */));
}

static inline int rtk_wlan_akm_sae(int akm)
{
	return !!(akm & (WLAN_AKM_TYPE_SAE |
			 WLAN_AKM_TYPE_FT_OVER_SAE));
}

#endif
typedef enum _Synchronization_Sta_State{
    STATE_Min		= 0,
    STATE_No_Bss	= 1,
    STATE_Bss		= 2,
    STATE_Ibss_Active	= 3,
    STATE_Ibss_Idle	= 4,
    STATE_Act_Receive	= 5,
    STATE_Pas_Listen	= 6,
    STATE_Act_Listen	= 7,
    STATE_Join_Wait_Beacon = 8,
    STATE_Max		= 9
} Synchronization_Sta_State;
//#endif	// of WLAN_CLIENT || WLAN_SITESURVEY

//t_stamp[1], b5-b7 = channel width information
#define BSS_BW_SHIFT    5
#define BSS_BW_MASK             0x7

//t_stamp[1], pmf info
#define BSS_PMF_REQ		0x600
#define BSS_PMF_CAP		0x400
#define BSS_PMF_NONE	0x200

//mfp_opt, pmf info
#define BSS_MFP_NO			0
#define BSS_MFP_INVALID		1
#define BSS_MFP_OPTIONAL	2
#define BSS_MFP_REQUIRED	3

typedef enum _wlan_mac_state {
    STATE_DISABLED=0, STATE_IDLE, STATE_SCANNING, STATE_STARTED, STATE_CONNECTED, STATE_WAITFORKEY
} wlan_mac_state;

typedef enum _config_wlan_target {
    CONFIG_WLAN_ALL=0, CONFIG_WLAN_2G, CONFIG_WLAN_5G
} config_wlan_target;

typedef enum _config_wlan_ssid {
#ifdef WLAN_USE_VAP_AS_SSID1
	CONFIG_SSID1=0, CONFIG_SSID2, CONFIG_SSID3, CONFIG_SSID4, CONFIG_SSID5, CONFIG_SSID6, CONFIG_SSID7, CONFIG_SSID_ROOT, CONFIG_SSID_ALL
#else
    CONFIG_SSID_ROOT=0, CONFIG_SSID1=0, CONFIG_SSID2, CONFIG_SSID3, CONFIG_SSID4, CONFIG_SSID5, CONFIG_SSID6, CONFIG_SSID7, CONFIG_SSID8, CONFIG_SSID_ALL
#endif
} config_wlan_ssid;

#ifdef WLAN_USE_DRVCOMMON_HEADER
//user wifi_common.h
//typedef bss_info_2_web bss_info;
#else
typedef struct _bss_info {
    unsigned char state;
    unsigned char channel;
    unsigned char txRate;
    unsigned char bssid[6];
    unsigned char rssi, sq;	// RSSI  and signal strength
    unsigned char ssid[SSID_LEN+1];
#ifdef WLAN_DRVCOMMON_HEADER
} __PACK__ bss_info;
#else
} bss_info;
#endif
#endif //WLAN_USE_DRVCOMMON_HEADER

#ifdef WLAN_WDS
typedef enum _wlan_wds_state {
    STATE_WDS_EMPTY=0, STATE_WDS_DISABLED, STATE_WDS_ACTIVE
} wlan_wds_state;

typedef struct _WDS_INFO {
	unsigned char	state;
	unsigned char	addr[6];
	unsigned long	tx_packets;
	unsigned long	rx_packets;
	unsigned long	tx_errors;
	unsigned char	txOperaRate;
} WDS_INFO_T, *WDS_INFO_Tp;

#endif //WLAN_WDS

struct _misc_data_ {
	unsigned char	mimo_tr_hw_support;
	unsigned char	mimo_tr_used;
	unsigned char	resv[30];
};

typedef struct _dot11k_ap_channel_report
{
    unsigned char len;
    unsigned char op_class;
    unsigned char channel[MAX_AP_CHANNEL_NUM];
}__PACK__ dot11k_ap_channel_report;

typedef enum {
    MEASUREMENT_UNKNOWN = 0,
    MEASUREMENT_PROCESSING = 1,
    MEASUREMENT_SUCCEED = 2,
    MEASUREMENT_INCAPABLE = 3,
    MEASUREMENT_REFUSED = 4,   
}MEASUREMENT_RESULT;

typedef struct _dot11k_beacon_measurement_req
{
    unsigned char op_class;
    unsigned char channel;
    unsigned short random_interval;    
    unsigned short measure_duration;    
    unsigned char mode;     
    unsigned char bssid[MAC_ADDR_LEN];
    char ssid[SSID_LEN+1];
    unsigned char report_detail; /* 0: no-fixed len field and element, 
                                                               1: all fixed len field and elements in Request ie,
                                                               2: all fixed len field and elements (default)*/
    unsigned char request_ie_len;
    unsigned char request_ie[MAX_REQUEST_IE_LEN];   
    dot11k_ap_channel_report ap_channel_report[MAX_AP_CHANNEL_REPORT];    
}__PACK__ dot11k_beacon_measurement_req;

typedef struct _dot11k_beacon_measurement_report_info
{
    unsigned char op_class;
    unsigned char channel;
    unsigned int  measure_time_hi;
    unsigned int  measure_time_lo;
    unsigned short measure_duration;
    unsigned char frame_info;
    unsigned char RCPI;
    unsigned char RSNI;
    unsigned char bssid[MAC_ADDR_LEN];
    unsigned char antenna_id;
    unsigned int  parent_tsf;
}__PACK__ dot11k_beacon_measurement_report_info;

typedef struct _dot11k_beacon_measurement_report
{
    dot11k_beacon_measurement_report_info info;
    unsigned char subelements_len;
    unsigned char subelements[MAX_BEACON_SUBLEMENT_LEN];
}__PACK__ dot11k_beacon_measurement_report;

#ifdef WLAN_11AX
#ifdef WIFI5_WIFI6_COMP
typedef struct sta_mac_rssi {
	unsigned char			channel;
	unsigned char			addr[MAC_ADDR_LEN];
	signed char			rssi;
	unsigned char 			used;
	unsigned char 			Entry;
	unsigned char 			status;
	unsigned long 				time_stamp; // jiffies time of last probe request
} sta_mac_rssi;
#endif
#ifdef WLAN_USE_DRVCOMMON_HEADER
#ifdef WIFI5_WIFI6_COMP
typedef wifi_diag_sta_entry_2_web sta_mac_rssi_ax;
#else
typedef wifi_diag_sta_entry_2_web sta_mac_rssi;
#endif
#else
//please check driver with "wifi_diag_sta_entry_2_web" in include/rtw_ioctl_ap.h
struct _sta_mac_rssi {
	unsigned char  channel;
	unsigned char  addr[MAC_ADDR_LEN];
	unsigned char  rssi;
	/* following is connected bss info */
	unsigned char  bssid[MAC_ADDR_LEN];
	unsigned char  ssid[33];
	unsigned char  ssid_length;
	unsigned int bss_encrypt;  /* open/WEP/WPA/RSN */
	unsigned int bss_pairwise_cipher;
	unsigned int bss_group_cipher;
	unsigned int bss_akm;
	/* end of connected bss info */
	unsigned char  used;
	unsigned char  entry;
	unsigned char  status;
	unsigned int time_stamp;
#ifdef WLAN_DRVCOMMON_HEADER
} __PACK__;
#else
};
#endif
#ifdef WIFI5_WIFI6_COMP
typedef struct _sta_mac_rssi sta_mac_rssi_ax;
#else
typedef struct _sta_mac_rssi sta_mac_rssi;
#endif
#endif
#else
typedef struct _sta_mac_rssi {
	unsigned char			channel;
	unsigned char			addr[MAC_ADDR_LEN];
	signed char			rssi;	
	unsigned char 			used;
	unsigned char 			Entry;
	unsigned char 			status;		
	unsigned long 				time_stamp; // jiffies time of last probe request
} sta_mac_rssi;
#endif
#ifdef WLAN_11AX
#ifdef WIFI5_WIFI6_COMP
typedef struct sta_mac_rssi_detail {
	unsigned char                 channel;
	unsigned char                 addr[MAC_ADDR_LEN];
	char                 rssi;
	unsigned char           bssid[MAC_ADDR_LEN];
	unsigned char           ssid[33];
	unsigned char           ssid_length;
	unsigned char                 encrypt;
	unsigned char                         used;
	unsigned char                         Entry;
	unsigned char                         status;
	unsigned long                          time_stamp;
}sta_mac_rssi_detail;
#endif
#ifdef WLAN_USE_DRVCOMMON_HEADER
#ifdef WIFI5_WIFI6_COMP
typedef wifi_diag_sta_entry_2_web sta_mac_rssi_detail_ax;
#else
typedef wifi_diag_sta_entry_2_web sta_mac_rssi_detail;
#endif
#else
//please check driver with "wifi_diag_sta_entry_2_web" in include/rtw_ioctl_ap.h
struct _sta_mac_rssi_detail {
	unsigned char  channel;
	unsigned char  addr[MAC_ADDR_LEN];
	unsigned char  rssi;
	/* following is connected bss info */
	unsigned char  bssid[MAC_ADDR_LEN];
	unsigned char  ssid[33];
	unsigned char  ssid_length;
	unsigned int bss_encrypt;  /* open/WEP/WPA/RSN */
	unsigned int bss_pairwise_cipher;
	unsigned int bss_group_cipher;
	unsigned int bss_akm;
	/* end of connected bss info */
	unsigned char  used;
	unsigned char  entry;
	unsigned char  status;
	unsigned int time_stamp;
#ifdef WLAN_DRVCOMMON_HEADER
} __PACK__;
#else
};
#endif
#ifdef WIFI5_WIFI6_COMP
typedef struct _sta_mac_rssi_detail sta_mac_rssi_detail_ax;
#else
typedef struct _sta_mac_rssi_detail sta_mac_rssi_detail;
#endif
#endif
#else
typedef struct sta_mac_rssi_detail {
	unsigned char                 channel;
	unsigned char                 addr[MAC_ADDR_LEN];
	char                 rssi; 
	unsigned char           bssid[MAC_ADDR_LEN];
	unsigned char           ssid[33];
	unsigned char           ssid_length;
	unsigned char                 encrypt;
	unsigned char                         used;
	unsigned char                         Entry;
	unsigned char                         status;             
	unsigned long                          time_stamp;
}sta_mac_rssi_detail;
#endif

enum { WPS_VERSION_V1=0, WPS_VERSION_V2=1 };
enum { WLAN_STA_TX_RATE=0, WLAN_STA_RX_RATE};

#if defined(CONFIG_USER_DATA_ELEMENTS_AGENT) || defined(CONFIG_USER_DATA_ELEMENTS_COLLECTOR)
enum { DE_ROLE_DISABLED, DE_ROLE_AGENT, DE_ROLE_COLLECTOR};
enum { DE_DBG_DISABLED, DE_DBG_ERR, DE_DBG_WARN, DE_DBG_INFO, DE_DBG_DETAIL};
#define DATA_ELEMENT_DEF_PORT	8080
extern int start_data_element(void);
#endif

#ifdef WLAN_GEN_MBSSID_MAC
void _gen_guest_mac(const unsigned char *base_mac, int maxBss, const int guestNum, unsigned char *hwaddr);
#endif

#ifdef WLAN_SUPPORT
int getWlStaInfo( char *interface,  WLAN_STA_INFO_Tp pInfo );
#ifdef WIFI5_WIFI6_COMP
int getWlStaExtraInfo( char *interface,  WLAN_STA_EXTRA_INFO_Tp pInfo );
#endif
int set_wlan_idx_by_wlanIf(unsigned char * ifname,int *vwlan_idx);
#endif
int config_WLAN_by_fork(int action_type, config_wlan_ssid ssid_index);
int config_WLAN( int action_type, config_wlan_ssid ssid_index);
#ifdef CONFIG_USER_FON
void startFonSpot(void);
int setFonFirewall(void);
#endif

#ifdef WLAN_SCHEDULE_SUPPORT
extern int restartWlanSchedule(void);
extern int start_func_wlan_schedule(void);
#endif

extern int wl_isNband(unsigned char band);
extern void wl_updateSecurity(unsigned char band);
extern unsigned char wl_cipher2mib(unsigned char cipher);

int getWlVersion(const char *interface, char *verstr );
char *getWlanIfName(void);
void getWscPidName(char *wscd_pid_name);
#ifdef WLAN_WISP
void getWispWanName(char *name, int idx);
void setWlanDevFlag(char *ifname, int set_wan);
#endif
int setup_wlan_block(void);
int getMiscData(char *interface, struct _misc_data_ *pData);
#if defined(WLAN_CLIENT) 
int getWlJoinRequest(char *interface, pBssDscr pBss, unsigned char *res);
int getWlJoinResult(char *interface, unsigned char *res);
void getSiteSurveyWlanNeighborAsync(char wlan_idx);
#endif
int getWlSiteSurveyRequest(char *interface, int *pStatus);
int getWlSiteSurveyResult(char *interface, SS_STATUS_Tp pStatus );
#ifdef WLAN_11R
int genFtKhConfig(void);
#endif
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
int update_wps_configured(int reset_flag);
#endif
int getWlBssInfo(const char *interface, bss_info *pInfo);

typedef enum { WLAN_AGGREGATION_AMPDU=0, WLAN_AGGREGATION_AMSDU=1} WLAN_AGGREGATION_FLAG_T;

#ifdef RTK_SMART_ROAMING
#define CAPWAP_APP_VAR_DIR "/var/capwap"
#define CAPWAP_APP_ETC_DIR "/etc/capwap"
#define CAPWAP_APP_WLAN_CONFIG CAPWAP_APP_VAR_DIR"/wlan.config"
#define CAPWAP_APP_DHCP_CONFIG CAPWAP_APP_VAR_DIR"/dhcp_mode.config"
#define CAPWAP_APP_CAPWAP_CONFIG CAPWAP_APP_VAR_DIR"/capwap_mode.config"

#define CAPWAP_SMART_ROAM_SCRIPT CAPWAP_APP_VAR_DIR"/sr_script"	// hook point between sys and smart roaming daemon
#define CAPWAP_SR_AUTO_SYNC_CONFIG CAPWAP_APP_VAR_DIR"/wlan_auto_sync.config"	//smart roaming auto sync config file which used to update wlan setting.
#define CAPWAP_APPLY_CHANGE_NOTIFY_FILE CAPWAP_APP_VAR_DIR"/config_notify"

#define ROAMING_ENABLE	(CAPWAP_WTP_ENABLE | CAPWAP_AC_ENABLE | CAPWAP_ROAMING_ENABLE)

void update_RemoteAC_Config(void);
int setup_capwap_script(void);
void setup_capwap_config(void);
void stop_capwap(void);
void start_capwap(void);
#endif
#ifdef RTK_CROSSBAND_REPEATER
int setupWlanCrossband(char *ifname, unsigned char wlan_mode);
int start_crossband(void);
#endif
typedef enum {
    STA_CONTROL_ENABLE = (1<<0),
    STA_CONTROL_PREFER_BAND = (1<<1),
} STA_CONTROL_T;

#ifdef WLAN_BAND_STEERING
int rtk_wlan_get_band_steering_status(void);
#ifdef WLAN_VAP_BAND_STEERING
int rtk_wlan_vap_get_band_steering_status(int vapIndex);
void rtk_wlan_vap_set_band_steering(void);
#endif
int rtk_wlan_update_band_steering_status(void);
void rtk_wlan_set_band_steering(void);
#endif
#if defined(BAND_STEERING_SUPPORT)
int rtk_wlan_get_band_steering_status(void);
int rtk_wlan_update_band_steering_status(void);
#endif
unsigned int check_wlan_module(void);
int wlan_getEntry(MIB_CE_MBSSIB_T *pEntry, int index);
int wlan_setEntry(MIB_CE_MBSSIB_T *pEntry, int index);
int getWlEnc( char *interface , char *buffer, char *num);
int getWlWpaChiper( char *interface , char *buffer, int *num);
int rtk_wlan_get_wlan_enable(unsigned char *ifname, unsigned int *enable);
int getWlStaNum( char *interface, int *num );
int isValid_wlan_idx(int idx);
int is8021xEnabled(int vwlan_idx);
#ifdef SWAP_WLAN_MIB
int needSwapWlanMib(CONFIG_DATA_T data_type);
int swapWlanMibSetting_HS(CONFIG_DATA_T data_type);
int swapWlanMibSetting_CS(unsigned char wlanifNumA, unsigned char wlanifNumB, char *tmpBuf);
#endif
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
void sync_wps_config_mib(void);
void update_wps_mib(void);
#endif
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
void update_wps_from_file(void);
int checkWpsDisableStatus(MIB_CE_MBSSIB_Tp Entry);
int restartWPS(int ssid_idx);
void set_wps_ssid(unsigned char ssid_number);
#endif
int startWLan(config_wlan_target target, config_wlan_ssid ssid_index);
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
int mib_update_all(void);
#endif
//#ifndef CONFIG_RTK_DEV_AP
#ifdef WLAN_WPS_VAP
int WPS_updateWscConf(char *in, char *out, int genpin, MIB_CE_MBSSIB_T *Entry, int vwlan_idx, int wlanIdx);
//#else
//int WPS_updateWscConf(char *in, char *out, int genpin, char *wlanif_name);
#endif
//#endif

#ifdef CONFIG_GUEST_ACCESS_SUPPORT
#ifdef IS_AX_SUPPORT
int setup_wlan_guestaccess();
int clean_wlan_guestaccess();
#else
int setup_wlan_guestaccess(int vwlan_idx);
#endif
#endif
void rtk_wlan_get_ifname(int wlan_index, int mib_chain_idx, char *ifname);
void rtk_wlan_get_ifname_by_ssid_idx(int ssid_idx, char *ifname);
void rtk_wlan_wifi_button_on_off_action(void);
void rtk_wlan_wps_button_action(int wlan_index);
int set_wlan_led_status(int led_mode);
#ifdef _PRMT_X_WLANFORISP_
int isWLANForISP(int vwlan_idx);
void update_WLANForISP_configured(void);
void sync_WLANForISP(int ssid_idx, MIB_CE_MBSSIB_T *Entry);
int getWLANForISP_ifname(char *ifname, MIB_WLANFORISP_T *wlan_isp_entry);
#ifdef WLAN_11AX
int rtk_wlan_get_wlanforisp_entry(int wlanIdx, int vwlan_idx, MIB_WLANFORISP_Tp pEntry);
#endif
#endif
#ifdef WPS_QUERY
enum {  NOT_USED=-1,
		PROTOCOL_START=0, PROTOCOL_PBC_OVERLAPPING=1,
		PROTOCOL_TIMEOUT=2, PROTOCOL_SUCCESS=3 ,
		SEND_EAPOL_START, RECV_EAPOL_START, SEND_EAP_IDREQ, RECV_EAP_IDRSP,
        SEND_EAP_START, SEND_M1, RECV_M1, SEND_M2, RECV_M2, RECV_M2D, SEND_M3, RECV_M3,
        SEND_M4, RECV_M4, SEND_M5, RECV_M5, SEND_M6, RECV_M6, SEND_M7, RECV_M7,
        SEND_M8, RECV_M8, PROC_EAP_ACK, WSC_EAP_FAIL, HASH_FAIL, HMAC_FAIL, PWD_AUTH_FAIL,
        PROTOCOL_PIN_NUM_ERR,  PROC_EAP_DONE,
};      //PROTOCOL_TIMEOUT means fail
void run_wps_query(char *wps_status, char *devinfo);
#endif
#ifdef WLAN_WPS_HAPD
void run_wps_query(int ssid_idx, char *wps_status, char *devinfo);
#endif
void rtk_wlan_get_sta_rate(WLAN_STA_INFO_Tp pInfo, int rate_type, char *rate_str, unsigned int len);
#ifdef WIFI5_WIFI6_COMP
void rtk_wlan_get_sta_extra_rate(void *addr, WLAN_STA_EXTRA_INFO_Tp pInfo, int rate_type, char *rate_str, unsigned int len);
#endif
int getWlChannel( char *interface, unsigned int *num);
int getWlMaxStaNum( char *interface, unsigned int *num);
#ifdef CONFIG_E8B
int waitWlChannelSelect(char *ifname);
unsigned char get_wlan_phyband(void);
int get_ifname_by_ssid_index(int ssid_index, char *ifname);
#if defined(WLAN_BAND_STEERING)
unsigned int get_root_wlan_status(void);
#endif
#ifdef WPS_QUERY
int is_wps_running(void);
#endif
double rtk_wlan_get_sta_current_tx_rate(WLAN_STA_INFO_Tp pInfo);
#ifdef WLAN_11AX
unsigned int rtk_wlan_get_bss_rate(unsigned char, unsigned char, unsigned int, unsigned int, unsigned char);
#endif

typedef struct _wlan_channel_info{
	unsigned int channel;
	unsigned int score;
}wlan_channel_info;

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
typedef struct _ipPortRange {
	short int sin_family;
	unsigned char start_addr[16];
	unsigned char end_addr[16];
	unsigned short int start_port;
	unsigned short int end_port;
	unsigned short ip_protocol;
	unsigned short eth_protocol;
} ipPortRange;

typedef struct _wl_ipport_rule {
	ipPortRange ipport;
	unsigned int wlan_idx_mask;
	unsigned char action;
	struct _wl_ipport_rule *next;
} wl_ipport_rule;
int setup_wlan_accessRule_netfilter_init(void);
int setup_wlan_accessRule_netfilter_iptables_init(void);
int setup_wlan_accessRule_netfilter(char *ifname, MIB_CE_MBSSIB_Tp pEntry);
#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU)
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#ifdef WLAN_11AX
int setup_wlan_MAC_ACL(int,int);
#else
int setup_wlan_MAC_ACL(void);
#endif
int get_wlan_MAC_ACL_BlockTimes(const unsigned char *mac);
void rtk_wlan_monitor_restart_daemon(void);
#endif
int check_wlan_encrypt(MIB_CE_MBSSIB_Tp Entry);
int get_TxPowerValue(int phyband, int mode);
#if defined(WLAN_BAND_STEERING)
unsigned int get_root_wlan_status(void);
int SetOrCancelSameSSID(unsigned char sta_control);
void setSameSSID(unsigned int ssidindex24, unsigned int ssidindex58, unsigned int *result, char *err);
void cancelSameSSID(unsigned int *result, char *err);
#endif
int get_wlan_channel_scan_status(int idx);
int start_wlan_channel_scan(int idx);
int switch_wlan_channel(int idx);
int get_wlan_channel_score(int idx, wlan_channel_info **chInfo, int *count);
#ifdef WLAN_ROAMING
int doWlStartAPRSSIQueryRequest(char *ifname, unsigned char * macaddr, dot11k_beacon_measurement_req* beacon_req);
int getWlStartAPRSSIQueryResult(char *ifname, unsigned char *macaddr, unsigned char* measure_result, int *bss_num, dot11k_beacon_measurement_report *beacon_report);
int getWlStartSTABSSTransitionRequest(char *interface, unsigned char *mac_sta, unsigned int channel, unsigned char *bssid);
#endif
#if defined(WLAN_ROAMING) || defined(WLAN_STA_RSSI_DIAG)
int getWlSTARSSIScanInterval( char *interface, unsigned int *num);
int doWlSTARSSIQueryRequest(char *interface, char *mac, unsigned int channel);
int getWlSTARSSIDETAILQueryResult(char *interface, void *sta_mac_rssi_info, unsigned char type);
int getWlSTARSSIQueryResult(char *interface, void *sta_mac_rssi_info);
int stopWlSTARSSIQueryRequest(char *interface);
#endif
#ifdef WLAN_STA_RSSI_DIAG
int rtk_wlan_get_wifi_unassocsta(char *ifname, sta_mac_rssi_detail *sta_mac_rssi_result);
int rtk_wlan_get_wifi_unasso_sta_info(char *ifname, void **sta_mac_rssi_result, int *pCount);
#endif
int rtk_wlan_set_bcnvsie( char *interface, char *data, int data_len);
int rtk_wlan_set_prbvsie(char *interface, char *data, int data_len);
int rtk_wlan_set_prbrspvsie(char *interface, char *data, int data_len);
#endif
#endif //#ifdef CONFIG_E8B
typedef struct _net_device_sta_stats
{
	unsigned int            tx_bytes;
	unsigned int            rx_bytes;
	unsigned int            tx_pkts;
	unsigned int            rx_pkts;
	unsigned int            tx_fail;
}net_device_sta_stats;

int get_wlan_sta_stats(const char *ifname, net_device_sta_stats *ndss);

#ifdef _PRMT_X_CMCC_WLANSHARE_
void setup_wlan_share(void);
#endif
#if defined WLAN_QoS && (!defined CONFIG_RTL8192CD && !defined(CONFIG_RTL8192CD_MODULE))
int setupWLanQos(char *argv[]);
#endif

#ifdef CONFIG_USER_LANNETINFO
int doWlKickOutDevice( char *interface,  unsigned char *mac_addr);
void kickOutDevice(unsigned char *mac_addr, int ssid_idx, unsigned int *result, char *error);
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
int getWlNoiseInfo(char *interface, signed int *pInfo);
int getWlInterferencePercentInfo(char *interface, unsigned long *pInfo);
int getWlIdlePercentInfo(char *interface, unsigned int *pInfo);
#endif
enum {  
		HT20_40=0, 
		HT20=1,
		HT40=2,
		HT80=3,
		HT160=4,
		HT80_80=5
};

int getWlCurrentChannelWidthInfo(char *interface, unsigned int *pInfo);

#ifdef CONFIG_CT_AWIFI_JITUAN_FEATURE
int rtk_wlan_check_ssid_is_awifi(int wlanidx, int ssid_idx);
#endif
#ifdef WLAN_CTC_MULTI_AP
int rtk_wlan_check_ssid_is_eashmesh_backhaul(int wlanidx, int ssid_idx);
#endif
#if defined(WLAN_HAPD) || defined(WLAN_WPAS)
#ifdef WIFI5_WIFI6_COMP
int writeHostapdGeneralScript(const char *in, const char *out);
#endif
#endif
#ifdef WLAN_HAPD
#ifdef WLAN_WPS_HAPD
extern const char HOSTAPD_WPS_SCRIPT[];
int rtk_wlan_get_wps_status(char *ifname);
void rtk_wlan_update_wlan_wps_from_file(char *ifname);
void rtk_wlan_update_wlan_wps_from_string(char *ifname, char *hexdump);
#endif
int rtk_wlan_channel_change_event(char *ifname);
#endif
#ifdef WLAN_WPAS
#ifdef WLAN_WPS_WPAS
extern const char WPAS_WPS_SCRIPT[];
void rtk_wlan_update_wlan_client_wps_from_file(char *ifname);
void rtk_wlan_store_wpas_wps_status(char *event_str, char *ifname);
#ifdef WIFI5_WIFI6_COMP
int rtk_wlan_get_wps_led_interface(char *ifname);
#endif
#endif
#endif
#if defined(WLAN_HAPD) || defined(WLAN_WPAS)
#if defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
#ifdef WIFI5_WIFI6_COMP
#define WPS_LED_INTERFACE "/tmp/wps_led_interface"
int writeWpsScript(const char *in, const char *out);
int _set_wlan_wps_led_status(int led_mode);
#endif
#endif
#endif

#ifdef WLAN_11AX
int rtk_wlan_get_mib_index_mapping_by_ifname(const char *ifname, int *wlan_index, int *vwlan_idx);
int rtk_wlan_setup_for_driver(char *ifname, unsigned int flags);
#ifdef WLAN_BAND_STEERING
void rtk_wlan_stop_band_steering(void);
void rtk_wlan_setup_band_steering(void);
#endif
#endif

int rtk_wlan_is_vxd_ifname(char *ifname);

#ifdef SBWC
typedef struct _wifi_sbwc_req{
	unsigned char clientMAC[32];
	unsigned char tx_limit[32];				// kbps
	unsigned char rx_limit[32];				// kbps
	unsigned int mib_changed;
}wifi_sbwc_req_T, *wifi_sbwc_reqTp;

typedef struct _wifi_sbwc_info{
	unsigned int sbwc_enable;
	unsigned int sbwc_num;
	MIB_CE_WLAN_SBWC_ENTRY_T sbwc_info[MAX_WLAN_SBWC_NUM];
}wifi_sbwc_info_T, *wifi_sbwc_info_Tp;

int rtk_wlan_set_sbwc_req(unsigned char *ifname, wifi_sbwc_req_T *sbwc_req);
int rtk_wlan_get_sbwc_mode(unsigned char *ifname, unsigned int *mode);
int rtk_wlan_set_sbwc_mode(unsigned char *ifname, unsigned int mode);
#endif
enum {
	GBWC_MODE_DISABLE = 0,
	GBWC_MODE_LIMIT_MAC_INNER,
	GBWC_MODE_LIMIT_MAC_OUTTER,
	GBWC_MODE_LIMIT_IF_TX,
	GBWC_MODE_LIMIT_IF_RX,
	GBWC_MODE_LIMIT_IF_TRX,
};

#ifdef WLAN_WPS_HAPD
int rtk_wlan_get_wps_status(char *ifname);
int rtk_wlan_cancel_wps(char *ifname);
int rtk_wlan_cancel_all_wps(void);
int rtk_wlan_get_all_wps_status(void);
#endif
int mib_chain_get_wlan(unsigned char *ifname,MIB_CE_MBSSIB_T *WlanEntry,int *vwlanidx);
int deauth_wlan_clients(void);

#ifdef WIFI5_WIFI6_COMP
int rtk_get_interface_flag(const char *ifname, int count, int flag);
int rtk_wlan_is_ax_support_by_index(int wlan_index);
int rtk_wlan_is_ax_support(const char *ifname);
#endif

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/file.h>
#include <arpa/inet.h>
#include "debug.h"
#include "utility.h"
#include "subr_net.h"
#include <linux/wireless.h>
#include <sys/sysinfo.h>
#if defined(CONFIG_CTC_SDN)
#include "subr_ctcsdn.h"
#endif

#ifdef RTK_MULTI_AP
#include "subr_multiap.h"
#endif

#include "hapd_conf.h"
#include "wifi6_priv_conf.h"
//#include "wlan_manager_conf.h"

#if defined(WLAN_WPS_HAPD)
const char HOSTAPD_WPS_SCRIPT[] = "/var/hostapd_wps.sh";
#endif
#if defined(WLAN_WPS_HAPD)
const char WPAS_WPS_SCRIPT[] = "/var/wpa_supplicant_wps.sh";
#endif
const char HOSTAPD_GENERAL_SCRIPT[] = "/var/hostapd_general_event.sh";
const char WPAS_GENERAL_SCRIPT[] = "/var/wpa_supplicant_general_event.sh";

static const char HOSTAPD_GLOBAL_CTRL_PATH[]="/var/run/hostapd/global";

#define SIOCDEVPRIVATEAXEXT 0x89f2 //SIOCDEVPRIVATE+2
#ifdef CONFIG_RF_DPK_SETTING_SUPPORT
#define PWSF_2G_LEN 3
#define LUT_2G_LEN 64
#define PWSF_5G_LEN 9
#define LUT_5G_LEN 16
#define LUT_SWAP(_DST_,_LEN_) do{}while(0)
#endif

#ifdef CONFIG_2G_ON_WLAN0
#define HW_MIB_2G(name) MIB_HW_##name
#else
#define HW_MIB_2G(name) MIB_HW_WLAN1_##name
#endif

#define HW_MIB_FROM(name,idx) (idx ? MIB_HW_WLAN1_##name : MIB_HW_##name)

#ifdef WLAN_11R
const char FT_DAEMON_PROG[]	= "/bin/ftd";
const char FT_WLAN_IF[]		= "wlan0";
const char FT_CONF[]		= "/tmp/ft.conf";
const char FT_PID[]			= "/var/run/ft.pid";
#endif

#ifdef WLAN_SMARTAPP_ENABLE
const char SMART_WLANAPP_PROG[]	= "/bin/smart_wlanapp_wifi5";
const char SMART_WLANAPP_PID[]	= "/var/run/smart_wlanapp_wifi5.pid";
#endif

#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
static unsigned int _wlIdxRadioDoChange = 0;
static unsigned int _wlIdxSsidDoChange = 0;
static int checkWlanRootChange(int *p_action_type, config_wlan_ssid *p_ssid_index);
#endif

#ifdef WLAN_NOT_USE_MIBSYNC
static int setup_wlan_priv_mib_or_proc(int wlan_index, int vwlan_index);
static int setup_wlan_post_process(int wlan_index, int vwlan_index);
#endif

int set_wifi_misc(int wlan_index, int vwlan_index);
#ifdef RTK_WLAN_MANAGER
extern int rtk_start_wlan_manager(void);
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC	02000000	/* set close_on_exec */
#endif

#define ConfigWlanLock "/var/run/configWlanLock"
#define LOCK_WLAN()	\
do {	\
	if ((lockfd = open(ConfigWlanLock, O_RDWR | O_CLOEXEC)) == -1) {	\
		perror("open wlan lockfile");	\
		return 0;	\
	}	\
	while (flock(lockfd, LOCK_EX)) { \
		if (errno != EINTR) \
			break; \
	}	\
} while (0)

#define UNLOCK_WLAN()	\
do {	\
	flock(lockfd, LOCK_UN);	\
	close(lockfd);	\
} while (0)


#if defined(TRIBAND_SUPPORT)
const char* WLANIF[] = {"wlan0", "wlan1", "wlan2"};
#else
#if defined(WLAN_SWAP_INTERFACE)
const char* WLANIF[] = {"wlan1", "wlan0"};
#else
const char* WLANIF[] = {"wlan0", "wlan1"};
#endif
#endif
#if defined(TRIBAND_SUPPORT)
const char* WLAN_VXD_IF[] = {"wlan0-vxd", "wlan1-vxd", "wlan2-vxd"};
#else
#if defined(WLAN_SWAP_INTERFACE)
const char* WLAN_VXD_IF[] = {"wlan1-vxd", "wlan0-vxd"};
#else
const char* WLAN_VXD_IF[] = {"wlan0-vxd", "wlan1-vxd"};
#endif
#endif
const char WLAN_DEV_PREFIX[] = "wlan";

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
const char WLAN_MONITOR_DAEMON[] = "hostapd";
#endif
const char IWPRIV[] = "/bin/iwpriv";

const char WSC_DAEMON_PROG[] = "/bin/wscd";

int wlan_idx=0;	// interface index

const char *wlan_band[] = {
	0, "2.4 GHz (B)", "2.4 GHz (G)", "2.4 GHz (B+G)", 0
	, 0, 0, 0, "2.4 GHz (N)", 0, "2.4 GHz (G+N)", "2.4 GHz (B+G+N)", 0
};

const char *wlan_mode[] = {
	//"AP", "Client", "AP+WDS"
	"AP", "Client", "WDS", "AP+WDS"
};

const char *wlan_rate[] = {
	"1M", "2M", "5.5M", "11M", "6M", "9M", "12M", "18M", "24M", "36M", "48M", "54M"
	, "MCS0", "MCS1", "MCS2", "MCS3", "MCS4", "MCS5", "MCS6", "MCS7", "MCS8", "MCS9", "MCS10", "MCS11", "MCS12", "MCS13", "MCS14", "MCS15"
};

const char *wlan_auth[] = {
	"Open", "Shared", "Auto"
};

const char *wlan_preamble[] = {
	"Long", "Short"
};

const char *wlan_encrypt[] = {
	"None",
	"WEP",
	"WPA",
	"WPA2",
	"WPA2 Mixed",
#ifdef CONFIG_RTL_WAPI_SUPPORT
	"WAPI",
#else
	"",
#endif
#ifdef WLAN_WPA3
	"WPA3",
#ifdef WLAN_WPA3_H2E
	"WPA3 Transition",
#else
	"WPA2+WPA3 Mixed",
#endif
#else
	"",
	"",
#endif
};

const char *wlan_pskfmt[] = {
	"Passphrase", "Hex"
};

const char *wlan_wepkeylen[] = {
	"Disable", "64-bit", "128-bit"
};

const char *wlan_wepkeyfmt[] = {
	"ASCII", "Hex"
};

// Mason Yu. 201009_new_security
const char *wlan_Cipher[] = {
	//"TKIP", "AES", "Both"
	"TKIP", "AES", "TKIP+AES"
};

const unsigned char *MCS_DATA_RATEStr[2][2][32] =
{
	{{"6.5", "13", "19.5", "26", "39", "52", "58.5", "65",
	  "13", "26", "39" ,"52", "78", "104", "117", "130",
	  "19.5", "39", "58.5", "78" ,"117", "156", "175.5", "195",
	  "26", "52", "78", "104", "156", "208", "234", "260"},					// Long GI, 20MHz
	 {"7.2", "14.4", "21.7", "28.9", "43.3", "57.8", "65", "72.2",
	  "14.4", "28.9", "43.3", "57.8", "86.7", "115.6", "130", "144.5",
	  "21.7", "43.3", "65", "86.7", "130", "173.3", "195", "216.7",
	  "28.9", "57.8", "86.7", "115.6", "173.3", "231.1", "260", "288.9"}},	// Short GI, 20MHz
	{{"13.5", "27", "40.5", "54", "81", "108", "121.5", "135",
	  "27", "54", "81", "108", "162", "216", "243", "270",
	  "40.5", "81", "121.5", "162", "243", "324", "364.5", "405",
	  "54", "108", "162", "216", "324", "432", "486", "540"},				// Long GI, 40MHz
	 {"15", "30", "45", "60", "90", "120", "135", "150",
	  "30", "60", "90", "120", "180", "240", "270", "300",
	  "45", "90", "135", "180", "270", "360", "405", "450",
	  "60", "120", "180", "240", "360", "480", "540", "600"}	}			// Short GI, 40MHz
};


//changes in following table should be synced to VHT_MCS_DATA_RATE[] in 8812_vht_gen.c
const unsigned short VHT_MCS_DATA_RATE[3][2][30] =
        {       {       {13, 26, 39, 52, 78, 104, 117, 130, 156, 156,
                         26, 52, 78, 104, 156, 208, 234, 260, 312, 312,
                         39, 78, 117, 156, 234, 312, 351, 390, 468, 520},                        // Long GI, 20MHz
                        {14, 29, 43, 58, 87, 116, 130, 144, 173, 173,
                        29, 58, 87, 116, 173, 231, 260, 289, 347, 347,
                        43, 86, 130, 173, 260, 347, 390, 433, 520, 578}  },              // Short GI, 20MHz
                {       {27, 54, 81, 108, 162, 216, 243, 270, 324, 360,
                        54, 108, 162, 216, 324, 432, 486, 540, 648, 720,
                        81, 162, 243, 342, 486, 648, 729, 810, 972, 1080},               // Long GI, 40MHz
                        {30, 60, 90, 120, 180, 240, 270, 300,360, 400,
                        60, 120, 180, 240, 360, 480, 540, 600, 720, 800,
                        90, 180, 270, 360, 540, 720, 810, 900, 1080, 1200}},              // Short GI, 40MHz
                {       {59, 117,  176, 234, 351, 468, 527, 585, 702, 780,
                        117, 234, 351, 468, 702, 936, 1053, 1170, 1404, 1560,
                        176, 351, 527, 702, 1053, 1408, 1408, 1745, 2106, 2340},  // Long GI, 80MHz
                        {65, 130, 195, 260, 390, 520, 585, 650, 780, 867,
                        130, 260, 390, 520, 780, 1040, 1170, 1300, 1560,1733,
                        195, 390, 585, 780, 1170, 1560, 1560, 1950, 2340, 2600}   }       // Short GI, 80MHz
        };

typedef struct _DOT11_DISCONNECT_REQ{
		unsigned char	EventId;
		unsigned char	IsMoreEvent;
		unsigned short	Reason;
		char			MACAddr[6];
}DOT11_DISCONNECT_REQ;
typedef enum{
	DOT11_EVENT_NO_EVENT = 1,
	DOT11_EVENT_REQUEST = 2,
	DOT11_EVENT_ASSOCIATION_IND = 3,
	DOT11_EVENT_ASSOCIATION_RSP = 4,
	DOT11_EVENT_AUTHENTICATION_IND = 5,
	DOT11_EVENT_REAUTHENTICATION_IND = 6,
	DOT11_EVENT_DEAUTHENTICATION_IND = 7,
	DOT11_EVENT_DISASSOCIATION_IND = 8,
	DOT11_EVENT_DISCONNECT_REQ = 9,
	DOT11_EVENT_SET_802DOT11 = 10,
	DOT11_EVENT_SET_KEY = 11,
	DOT11_EVENT_SET_PORT = 12,
	DOT11_EVENT_DELETE_KEY = 13,
	DOT11_EVENT_SET_RSNIE = 14,
	DOT11_EVENT_GKEY_TSC = 15,
	DOT11_EVENT_MIC_FAILURE = 16,
	DOT11_EVENT_ASSOCIATION_INFO = 17,
	DOT11_EVENT_INIT_QUEUE = 18,
	DOT11_EVENT_EAPOLSTART = 19,

	DOT11_EVENT_ACC_SET_EXPIREDTIME = 31,
	DOT11_EVENT_ACC_QUERY_STATS = 32,
	DOT11_EVENT_ACC_QUERY_STATS_ALL = 33,
	DOT11_EVENT_REASSOCIATION_IND = 34,
	DOT11_EVENT_REASSOCIATION_RSP = 35,
	DOT11_EVENT_STA_QUERY_BSSID = 36,
	DOT11_EVENT_STA_QUERY_SSID = 37,
	DOT11_EVENT_EAP_PACKET = 41,

#ifdef RTL_WPA2_PREAUTH
	DOT11_EVENT_EAPOLSTART_PREAUTH = 45,
	DOT11_EVENT_EAP_PACKET_PREAUTH = 46,
#endif

#ifdef RTL_WPA2_CLIENT
	DOT11_EVENT_WPA2_MULTICAST_CIPHER = 47,
#endif

	DOT11_EVENT_WPA_MULTICAST_CIPHER = 48,

#ifdef AUTO_CONFIG
	DOT11_EVENT_AUTOCONF_ASSOCIATION_IND = 50,
	DOT11_EVENT_AUTOCONF_ASSOCIATION_CONFIRM = 51,
	DOT11_EVENT_AUTOCONF_PACKET = 52,
	DOT11_EVENT_AUTOCONF_LINK_IND = 53,
#endif

#ifdef WIFI_SIMPLE_CONFIG
	DOT11_EVENT_WSC_SET_IE = 55,
	DOT11_EVENT_WSC_PROBE_REQ_IND = 56,
	DOT11_EVENT_WSC_PIN_IND = 57,
	DOT11_EVENT_WSC_ASSOC_REQ_IE_IND = 58,
	DOT11_EVENT_WSC_START_IND = 70,
	DOT11_EVENT_WSC_MODE_IND = 71,
	DOT11_EVENT_WSC_STATUS_IND = 72,
	DOT11_EVENT_WSC_METHOD_IND = 73,
	DOT11_EVENT_WSC_STEP_IND = 74,
	DOT11_EVENT_WSC_OOB_IND = 75,
#endif
	DOT11_EVENT_WSC_PBC_IND = 76,
	// for WPS2DOTX
	DOT11_EVENT_WSC_SWITCH_MODE = 100,	// for P2P P2P_SUPPORT
	DOT11_EVENT_WSC_STOP = 101	,
	DOT11_EVENT_WSC_SET_MY_PIN = 102,		// for WPS2DOTX
	DOT11_EVENT_WSC_SPEC_SSID = 103,
	DOT11_EVENT_WSC_SPEC_MAC_IND = 104,
	DOT11_EVENT_WSC_CHANGE_MODE = 105,
	DOT11_EVENT_WSC_RM_PBC_STA = 106,
	DOT11_EVENT_WSC_CHANGE_MAC_IND=107,
	DOT11_EVENT_WSC_SWITCH_WLAN_MODE=108
	} DOT11_EVENT;

void rtk_wlan_get_ifname(int wlan_index, int mib_chain_idx, char *ifname)
{
	if(mib_chain_idx==WLAN_ROOT_ITF_INDEX)
	{
		strncpy(ifname, WLANIF[wlan_index], 15);
		*(ifname+15)='\0';
	}
#ifdef WLAN_MBSSID
	else if (mib_chain_idx>=WLAN_VAP_ITF_INDEX && mib_chain_idx<(WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM))
		snprintf(ifname, 16, "%s-"VAP_IF_ONLY, (char *)WLANIF[wlan_index], mib_chain_idx-WLAN_VAP_ITF_INDEX);
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
	else if (mib_chain_idx == WLAN_REPEATER_ITF_INDEX) {
		snprintf(ifname, 16, "%s-"VXD_IF_ONLY, (char *)WLANIF[wlan_index]);
	}
#endif
}

/*
 * ssid_index: 1~MAX_MBSSID_NUM for single-band 1~(MAX_MBSSID_NUM*2) for dual-band
 */
int get_ifname_by_ssid_index(int ssid_index, char *ifname)
{
	int max_ssid_index;

#ifdef WLAN_DUALBAND_CONCURRENT
	max_ssid_index = (WLAN_SSID_NUM*2);
#else
	max_ssid_index = WLAN_SSID_NUM;
#endif
	if(ssid_index<1 || ssid_index>max_ssid_index)
		return -1;

	rtk_wlan_get_ifname_by_ssid_idx(ssid_index, ifname);
	return 0;
}

#if defined(WLAN_WPS_HAPD)
#ifdef WLAN_UNIVERSAL_REPEATER
int vxd_link_status(int wlan_index)
{
	int is_link=0;
	FILE *fd;
	char buf[1024]={0}, newline[1024]={0};

	snprintf(buf, sizeof(buf), "%s status -i %s", "/bin/wpa_cli", WLAN_VXD_IF[wlan_index]);

	fd = popen(buf, "r");
	if(fd)
	{
		if(fgets(newline, 1024, fd) != NULL){
			if(strstr(newline, "bssid="))
			{
				is_link = 1;
				printf("[%s](%d)wlan%d_vxd is link\n",__FUNCTION__,__LINE__,wlan_index);
			}
		}
		pclose(fd);
	}

	return is_link;
}
#endif
#endif

void rtk_wlan_wps_button_action(int wlan_index)
{
#if defined(WLAN_WPS_HAPD)
	int tmp_vwlan_idx;
	MIB_CE_MBSSIB_T Entry;
	//todo: dualband ?
#ifdef WLAN_UNIVERSAL_REPEATER
	unsigned char rpt_enabled=0;
	mib_local_mapping_get(MIB_REPEATER_ENABLED1, wlan_index, (void *)&rpt_enabled);
	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)(WLAN_VXD_IF[wlan_index]),&Entry,&tmp_vwlan_idx)!=0){
			return;
	}
	if(rpt_enabled && !Entry.wsc_disabled && !vxd_link_status(wlan_index))
	{
		va_niced_cmd(WPA_CLI, 3 , 1 , "wps_pbc", "-i", WLAN_VXD_IF[wlan_index]);
	}
	else
#endif
	{
		memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
		if(mib_chain_get_wlan((unsigned char *)(WLANIF[wlan_index]),&Entry,&tmp_vwlan_idx)!=0){
			return;
		}

		if(Entry.wlanDisabled || Entry.wsc_disabled)
			return;
#ifdef WLAN_CLIENT
		if(Entry.wlanMode == AP_MODE)
			va_niced_cmd(HOSTAPD_CLI, 3 , 1 , "wps_pbc", "-i", WLANIF[wlan_index]);
		else if(Entry.wlanMode == CLIENT_MODE)
			va_niced_cmd(WPA_CLI, 3 , 1 , "wps_pbc", "-i", WLANIF[wlan_index]);
#else
		va_niced_cmd(HOSTAPD_CLI, 3 , 1 , "wps_pbc", "-i", WLANIF[wlan_index]);
#endif
	}
#elif defined(CONFIG_WIFI_SIMPLE_CONFIG)
	char cmd[128]={0};
	if(wlan_index == -1){
		va_cmd("/bin/wscd", 1, 1, "-sig_pbc");
	}
	else if(wlan_index>=0 && wlan_index<NUM_WLAN_INTERFACE){
#if defined(WLAN_DUALBAND_CONCURRENT) && !defined(WLAN_WPS_VAP)
		snprintf(cmd, sizeof(cmd), "echo 1 > /var/wps_start_interface%d", wlan_index);
		system(cmd);
#endif
		va_cmd("/bin/wscd", 2 , 1 , "-sig_pbc" , WLANIF[wlan_index]);
	}
#endif
}

static inline int
iw_get_ext(int                  skfd,           /* Socket to the kernel */
           const char *               ifname,         /* Device name */
           int                  request,        /* WE ID */
           struct iwreq *       pwrq)           /* Fixed part of the request */
{
  /* Set device name */
	memset(pwrq->ifr_name, 0, IFNAMSIZ);
	strncpy(pwrq->ifr_name, ifname, IFNAMSIZ-1);
	/* Do the request */
	return(ioctl(skfd, request, pwrq));
}

int getWlStaInfo( char *interface,  WLAN_STA_INFO_Tp pInfo )
{
#if 0
	int skfd;
	struct iwreq wrq;
	struct ifreq ifr;
	union iwreq_data u;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
	  close( skfd );
	  /* If no wireless name : no wireless extensions */
	  return -1;
	}

	strcpy((char*)pInfo, "get_sta_info");

	memset(&u, 0, sizeof(union iwreq_data));
	u.data.pointer = pInfo;
	u.data.length = sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1);

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_ifrn.ifrn_name, interface, strlen(interface));
	ifr.ifr_ifru.ifru_data = &u;

	if (iw_get_ext(skfd, interface, SIOCDEVPRIVATE, (struct iwreq *)&ifr) < 0) {
	  close( skfd );
	  return -1;
	}

	close( skfd );

	//memcpy(pInfo, buffer, sizeof(*pInfo));

	return 0;
#else
	int skfd, cmd_id, vwlan_idx=0;
	struct iwreq wrq;
	MIB_CE_MBSSIB_T Entry;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
	{
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)interface,&Entry,&vwlan_idx)!=0){
		if(skfd != -1){
			close( skfd );
		}
		return -1;
	}

	wrq.u.data.pointer = (caddr_t)pInfo;
	wrq.u.data.length = sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1);
	if(Entry.is_ax_support == 1){
		wrq.u.data.flags = SIOCGIWRTLSTAINFO;
		cmd_id = SIOCDEVPRIVATEAXEXT;
	}else{
		cmd_id = SIOCGIWRTLSTAINFO;
	}

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0)
	{
		close( skfd );
		return -1;
	}

	if(skfd != -1)
		close( skfd );

	return 0;
#endif
}

int getWlStaExtraInfo( char *interface,  WLAN_STA_EXTRA_INFO_Tp pInfo )
{
	int skfd, cmd_id, vwlan_idx=0;
	struct iwreq wrq;
	MIB_CE_MBSSIB_T Entry;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
	{
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)interface,&Entry,&vwlan_idx)!=0){
		if(skfd != -1){
			close( skfd );
		}
		return -1;
	}

	wrq.u.data.pointer = (caddr_t)pInfo;
	wrq.u.data.length = sizeof(WLAN_STA_EXTRA_INFO_T) * (MAX_STA_NUM+1);
	if(Entry.is_ax_support == 1){
		wrq.u.data.flags = SIOCGIWRTLSTAEXTRAINFO;
		cmd_id = SIOCDEVPRIVATEAXEXT;
	}else{
		cmd_id = SIOCGIWRTLSTAEXTRAINFO;
	}

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0)
	{
		close( skfd );
		return -1;
	}

	if(skfd != -1)
		close( skfd );

	return 0;
}


void rtk_wlan_wifi_button_on_off_action(void)
{
#if defined( WLAN_SUPPORT) && (defined(CONFIG_WLAN_ON_OFF_BUTTON) || defined(WLAN_ON_OFF_BUTTON))
	//int button_flag;
	unsigned char wlan_flag;
	//FILE *fpwlan;
	int orig_wlanid = wlan_idx;
	MIB_CE_MBSSIB_T mEntry;
	unsigned char vChar;
	//fpwlan = fopen("/proc/wlan_onoff","r+");
	//if(fpwlan)
	{
		//fscanf(fpwlan,"%d",&button_flag);

		//if(button_flag==1)
		{
	#if defined(TRIBAND_SUPPORT)
			{
				int k;
				for (k=0; k<NUM_WLAN_INTERFACE; k++) {
					wlan_idx = k;
					wlan_getEntry(&mEntry, 0);
					if (k==0) {
						vChar = mEntry.wlanDisabled;
						if(vChar == 0)
							vChar = 1;
						else
							vChar = 0;
					}
					mEntry.wlanDisabled = vChar;
					wlan_setEntry(&mEntry, 0);
				}
			}
	#else /* !defined(TRIBAND_SUPPORT) */
#if defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU)
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
			mib_local_mapping_get(MIB_WLAN_DISABLED, 0, (void *)&vChar);
			vChar = vChar? 0:1;
			mib_local_mapping_set(MIB_WLAN_DISABLED, 0, (void *)&vChar);
#ifdef WLAN_DUALBAND_CONCURRENT
			mib_local_mapping_set(MIB_WLAN_DISABLED, 1, (void *)&vChar);
#endif
#else
			mib_get_s(MIB_WIFI_MODULE_DISABLED, (void *)&vChar, sizeof(vChar));
			vChar = vChar? 0:1;
			mib_set(MIB_WIFI_MODULE_DISABLED, (void *)&vChar);
#endif
#else
			wlan_idx=0;
			wlan_getEntry(&mEntry, 0);
			vChar = mEntry.wlanDisabled;
			if(vChar == 0)
				vChar = 1;
			else
				vChar = 0;

			mEntry.wlanDisabled = vChar;
			wlan_setEntry(&mEntry, 0);
		#ifdef WLAN_DUALBAND_CONCURRENT
			wlan_idx=1;
			wlan_getEntry(&mEntry, 0);
			mEntry.wlanDisabled = vChar;
			wlan_setEntry(&mEntry, 0);
		#endif
	#endif /* defined(TRIBAND_SUPPORT) */
			wlan_idx = orig_wlanid;
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS) // WPS
			update_wps_configured(0);
#endif
#endif
			if(mib_update(CURRENT_SETTING, CONFIG_MIB_ALL) == 0)
				printf("CS Flash error! \n");
			config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);

			//wlan_flag = '0';
			//fwrite(&wlan_flag,1,1,fpwlan);
		}
		//else if(button_flag==2){
		//	config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);

		//	wlan_flag = '0';
		//	fwrite(&wlan_flag,1,1,fpwlan);
		//}
		//fclose(fpwlan);
	}
#endif
}


int isValid_wlan_idx(int idx)
{
	if (idx >=0 && idx < NUM_WLAN_INTERFACE)
		return 1;
#ifdef WLAN1_QTN
	if (idx >=0 && idx < 2)
		return 1;
#endif
	else
		return 0;
	return 0;
}

/* andrew: new test plan require N mode to avoid using TKIP. This function check the new band
   and unmask TKIP security if it's N mode.
*/
int wl_isNband(unsigned char band) {
	return (band >= 8);
}

#if 1 // defined(CONFIG_RTK_DEV_AP)
/* there is no vwlan_idx in boa */
int SetWlan_idx(char * wlan_iface_name)
{
	int idx;
	idx = atoi(&wlan_iface_name[4]);
	if (idx >= NUM_WLAN_INTERFACE) {
		printf("invalid wlan interface index number!\n");
		return 0;
	}
	wlan_idx = idx;

	return 1;
}
#endif

int getTxPowerHigh(int phyband)
{
	unsigned char vChar, txpower_high=0;
#ifdef WLAN_DUALBAND_CONCURRENT
	int orig_wlan_idx = wlan_idx;
	int i;
	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
		if(phyband == vChar){
#endif
			mib_get_s(MIB_WLAN_TX_POWER_HIGH, &txpower_high, sizeof(txpower_high));
#ifdef WLAN_DUALBAND_CONCURRENT
			break;
		}
	}
	wlan_idx = orig_wlan_idx;
#endif
	return txpower_high;

}

// 100mW = 20dB, 200mW=23dB
// 80mw = 19.03, (20-19.03)*2 = 2
int getTxPowerScale(int phyband, int mode)
{
#ifdef WLAN_TXPOWER_HIGH
	if(phyband == PHYBAND_2G)
	{
		switch (mode)
		{
			case 0: //100% or 200%
				if(getTxPowerHigh(phyband))
					return -4; //200%
				return 0; //100%
			case 1: //80%
				return 2;
			case 2: //60%
				return 5;
			case 3: //35%
				return 9;
			case 4: //15%
				return 17;
		}
	}
	else{ //5G
		switch (mode)
		{
			case 0: //100% or 140%
				if(getTxPowerHigh(phyband))
					return -3; //140%
					//return -6; //200%
				return 0; //100%
			case 1: //80%
				return 2;
			case 2: //60%
				return 5;
			case 3: //35%
				return 9;
			case 4: //15%
				return 17;
		}
	}
#else
	switch (mode)
	{
		case 0: //100%
			return 0;
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
		case 1: //80%
			return 2;
		case 2: //60%
			return 4;
		case 3: //40%
			return 8;
		case 4: //20%
			return 14;
#else
		case 1: //70%
			return 3;
		case 2: //50%
			return 6;
		case 3: //35%
			return 9;
		case 4: //15%
			return 17;
#endif
	}
#endif
	return 0;
}

int get_TxPowerValue(int phyband, int mode)
{
	// 2.4G: 100mW => 20 dbm ; 5G: 200mW => 23dbm
	int intVal = 0, power = 0;
	intVal = getTxPowerScale(phyband, mode);

	if (phyband == PHYBAND_2G) {
#if defined(CONFIG_YUEME)
		power = 21 - (intVal / 2);
#else
		power = 20 - (intVal / 2);
#endif
	}
	else if (phyband == PHYBAND_5G) {
		power = 21 - (intVal / 2);
	}
	return power;
}

int getWlVersion(const char *interface, char *verstr )
{
	int skfd, cmd_id, vwlan_idx=0;
	unsigned char vernum[4];
	struct iwreq wrq;
	MIB_CE_MBSSIB_T Entry;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)interface,&Entry,&vwlan_idx)!=0){
		return -1;
	}

	memset(vernum, 0, 4);
	wrq.u.data.pointer = (caddr_t)&vernum[0];
	wrq.u.data.length = sizeof(vernum);
	if(Entry.is_ax_support == 1){
		wrq.u.data.flags = SIOCGIWRTLDRVVERSION;
		cmd_id = SIOCDEVPRIVATEAXEXT;
	}else{
		cmd_id = SIOCGIWRTLDRVVERSION;
	}

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0) {
		close( skfd );
		return -1;
	}

	if(skfd != -1)
		close( skfd );

	sprintf(verstr, "%d.%d.%d", vernum[0], vernum[1], vernum[2]);

	return 0;
}


char *getWlanIfName(void)
{
	if(wlan_idx == 0)
		return (char *)WLANIF[0];
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(WLAN_DUALBAND_CONCURRENT)
	else if(wlan_idx == 1)
		return (char *)WLANIF[1];
#endif //CONFIG_RTL_92D_SUPPORT
#if defined(TRIBAND_SUPPORT)
	else if(wlan_idx == 2)
		return (char *)WLANIF[2];
#endif /* defined(TRIBAND_SUPPORT) */

	printf("%s: Wrong wlan_idx!\n", __func__);

	return NULL;
}

void rtk_wlan_get_ifname_by_ssid_idx(int ssid_idx, char *ifname)
{
	int wlan_index=0;
#ifdef WLAN_DUALBAND_CONCURRENT
	if(ssid_idx>WLAN_SSID_NUM)
	{
		ssid_idx-=WLAN_SSID_NUM;
		wlan_index=1;
	}
#endif
	rtk_wlan_get_ifname(wlan_index, ssid_idx-1, ifname);
}

#ifdef WLAN_GEN_MBSSID_MAC
void _gen_guest_mac(const unsigned char *base_mac, int maxBss, const int guestNum, unsigned char *hwaddr)
{
	if(maxBss < 8)
		maxBss = 8;

	memcpy(hwaddr,base_mac,MAC_ADDR_LEN);
	hwaddr[0] = 96 + (hwaddr[5]%(maxBss - 1) * 8);
	hwaddr[0] |= 0x2;
	hwaddr[5] = (hwaddr[5]&~(maxBss-1))|((maxBss-1)&(hwaddr[5]+guestNum));
}
#endif

int wlan_getEntry(MIB_CE_MBSSIB_T *pEntry, int index)
{
	int ret;
	unsigned char vChar;
	WLAN_MODE_T root_mode;

	ret=1;
	ret = mib_chain_get(MIB_MBSSIB_TBL, WLAN_ROOT_ITF_INDEX, (void *)pEntry);
	root_mode = (WLAN_MODE_T)pEntry->wlanMode;
	if (index!=WLAN_ROOT_ITF_INDEX) {
		ret=mib_chain_get(MIB_MBSSIB_TBL, index, (void *)pEntry);
		pEntry->wlanMode=root_mode;
#ifdef WLAN_UNIVERSAL_REPEATER
		if (index == WLAN_REPEATER_ITF_INDEX) {
			mib_get_s( MIB_REPEATER_ENABLED1, (void *)&vChar, sizeof(vChar));
			pEntry->wlanDisabled= (vChar==0?1:0);
			if (root_mode==CLIENT_MODE)
				pEntry->wlanMode=AP_MODE;
			else
				pEntry->wlanMode=CLIENT_MODE;

		}
#endif
	}
	return ret;
}

#ifdef WLAN_ROOT_MIB_SYNC
void MBSSID_SetRootEntry(MIB_CE_MBSSIB_T *Entry) {
	//Entry->idx = 0;

	mib_set(MIB_WLAN_ENCRYPT, &Entry->encrypt);
	mib_set(MIB_WLAN_ENABLE_1X, &Entry->enable1X);
	mib_set(MIB_WLAN_WEP, &Entry->wep);
	mib_set(MIB_WLAN_WPA_AUTH, &Entry->wpaAuth);
	mib_set(MIB_WLAN_WPA_PSK_FORMAT, &Entry->wpaPSKFormat);
	mib_set(MIB_WLAN_WPA_PSK, Entry->wpaPSK);
	mib_set(MIB_WLAN_RS_PORT, &Entry->rsPort);
	mib_set(MIB_WLAN_RS_IP, Entry->rsIpAddr);

	mib_set(MIB_WLAN_RS_PASSWORD, Entry->rsPassword);
	mib_set(MIB_WLAN_DISABLED, &Entry->wlanDisabled);
	mib_set(MIB_WLAN_SSID, Entry->ssid);
	mib_set(MIB_WLAN_MODE, &Entry->wlanMode);
	mib_set(MIB_WLAN_AUTH_TYPE, &Entry->authType);
	//added by xl_yue
	// Mason Yu. 201009_new_security
	mib_set( MIB_WLAN_WPA_CIPHER_SUITE, &Entry->unicastCipher);
	mib_set( MIB_WLAN_WPA2_CIPHER_SUITE, &Entry->wpa2UnicastCipher);
	mib_set( MIB_WLAN_WPA_GROUP_REKEY_TIME, &Entry->wpaGroupRekeyTime);

#ifdef CONFIG_RTL_WAPI_SUPPORT
	mib_set( MIB_WLAN_WAPI_PSK, Entry->wapiPsk);
	mib_set( MIB_WLAN_WAPI_PSKLEN, &Entry->wapiPskLen);
	mib_set( MIB_WLAN_WAPI_PSK_FORMAT, &Entry->wapiPskFormat);
	mib_set( MIB_WLAN_WAPI_AUTH, &Entry->wapiAuth);
	mib_set( MIB_WLAN_WAPI_ASIPADDR, Entry->wapiAsIpAddr);
	//mib_get_s( MIB_WLAN_WAPI_SEARCH_CERTINFO, Entry->wapiSearchCertInfo, sizeof(Entry->wapiSearchCertInfo));
	//mib_get_s( MIB_WLAN_WAPI_SEARCH_CERTINDEX, &Entry->wapiSearchIndex, sizeof(Entry->wapiSearchIndex));
	//mib_get_s( MIB_WLAN_WAPI_MCAST_REKEYTYPE, &Entry->wapiMcastkey, sizeof(Entry->wapiMcastkey));
	//mib_get_s( MIB_WLAN_WAPI_MCAST_TIME, &Entry->wapiMcastRekeyTime, sizeof(Entry->wapiMcastRekeyTime));
	//mib_get_s( MIB_WLAN_WAPI_MCAST_PACKETS, &Entry->wapiMcastRekeyPackets, sizeof(Entry->wapiMcastRekeyPackets));
	//mib_get_s( MIB_WLAN_WAPI_UCAST_REKETTYPE, &Entry->wapiUcastkey, sizeof(Entry->wapiUcastkey));
	//mib_get_s( MIB_WLAN_WAPI_UCAST_TIME, &Entry->wapiUcastRekeyTime, sizeof(Entry->wapiUcastRekeyTime));
	//mib_get_s( MIB_WLAN_WAPI_UCAST_PACKETS, &Entry->wapiUcastRekeyPackets, sizeof(Entry->wapiUcastRekeyPackets));
	//mib_get_s( MIB_WLAN_WAPI_CA_INIT, &Entry->wapiCAInit, sizeof(Entry->wapiCAInit));

#endif

	// Mason Yu. 201009_new_security
	mib_set(MIB_WLAN_WEP_KEY_TYPE, &Entry->wepKeyType);      // wep Key Format
	mib_set(MIB_WLAN_WEP64_KEY1, Entry->wep64Key1);
	mib_set(MIB_WLAN_WEP128_KEY1, Entry->wep128Key1);
#ifdef WLAN_ROOT_MIB_SYNC
	mib_set(MIB_WLAN_WEP64_KEY2, Entry->wep64Key2);
	mib_set(MIB_WLAN_WEP64_KEY3, Entry->wep64Key3);
	mib_set(MIB_WLAN_WEP64_KEY4, Entry->wep64Key4);
	mib_set(MIB_WLAN_WEP128_KEY2, Entry->wep128Key2);
	mib_set(MIB_WLAN_WEP128_KEY3, Entry->wep128Key3);
	mib_set(MIB_WLAN_WEP128_KEY4, Entry->wep128Key4);
	mib_set(MIB_WLAN_WEP_DEFAULT_KEY, &Entry->wepDefaultKey);
#endif
#if defined(WLAN_BAND_CONFIG_MBSSID)
	mib_set(MIB_WLAN_BAND, &Entry->wlanBand);
#endif
#ifdef WLAN_11W
	mib_set(MIB_WLAN_DOTIEEE80211W, &Entry->dotIEEE80211W);
	mib_set(MIB_WLAN_SHA256, &Entry->sha256);
#endif
#ifdef WLAN_ROOT_MIB_SYNC
	mib_set(MIB_WLAN_HIDDEN_SSID, &Entry->hidessid);
	mib_set(MIB_WLAN_QoS, &Entry->wmmEnabled);
	mib_set(MIB_WLAN_BLOCK_RELAY, &Entry->userisolation);
	mib_set(MIB_WLAN_RATE_ADAPTIVE_ENABLED, &Entry->rateAdaptiveEnabled);
	mib_set(MIB_WLAN_FIX_RATE, &Entry->fixedTxRate);
	mib_set(MIB_WLAN_MCAST2UCAST_DISABLE, &Entry->mc2u_disable);
#if defined(WLAN_WPS)
	mib_set(MIB_WSC_DISABLE, &Entry->wsc_disabled);
	mib_set(MIB_WSC_CONFIGURED, &Entry->wsc_configured);
	mib_set(MIB_WSC_UPNP_ENABLED, &Entry->wsc_upnp_enabled);
	mib_set(MIB_WSC_AUTH, &Entry->wsc_auth);
	mib_set(MIB_WSC_ENC, &Entry->wsc_enc);
	mib_set(MIB_WSC_PSK, Entry->wscPsk);
#endif
#endif
}
#endif

int wlan_setEntry(MIB_CE_MBSSIB_T *pEntry, int index)
{
	int ret=1;

	ret=mib_chain_update(MIB_MBSSIB_TBL, (void *)pEntry, index);

	return ret;
}

int getWlWpaChiper( char *interface , char *buffer, int *num)
{
	int skfd, cmd_id, vwlanidx=0;
	struct iwreq wrq;
	MIB_CE_MBSSIB_T WlanEnable_Entry;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
	{
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(&WlanEnable_Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan(interface,&WlanEnable_Entry,&vwlanidx)!=0)
		return -1;

	wrq.u.data.pointer = (caddr_t)buffer;
	if(WlanEnable_Entry.is_ax_support)
	{
		wrq.u.data.flags = RTL8192CD_IOCTL_GET_MIB;
		cmd_id = SIOCDEVPRIVATEAXEXT;
	}
	else
	{
		cmd_id = RTL8192CD_IOCTL_GET_MIB;
	}

	wrq.u.data.length = strlen(buffer);

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0)
	{
		close( skfd );
		return -1;
	}
	memcpy(num, buffer, sizeof(int));
	if(skfd != -1)
		close( skfd );

	return 0;
}

int mib_chain_get_wlan(unsigned char *ifname,MIB_CE_MBSSIB_T *WlanEntry,int *vwlanidx)
{
	int ret=0;
	int vwlan_idx;
	memset(WlanEntry,0,sizeof(MIB_CE_MBSSIB_T));

	mib_save_wlanIdx();
	if((set_wlan_idx_by_wlanIf(ifname,&vwlan_idx))==(-1)){
		mib_recov_wlanIdx();
		return (-1);
	}
	if (!mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)WlanEntry)){
		printf("mib_chain get error\n");
		ret=(-1);
	}
	mib_recov_wlanIdx();

	if(vwlanidx!=NULL)
		*vwlanidx=vwlan_idx;

	return ret;
}

int rtk_wlan_get_wlan_enable(unsigned char *ifname, unsigned int *enable)
{
	int ret = 0;
	int vwlan_idx;
	MIB_CE_MBSSIB_T WlanEnable_Entry;

	if(ifname==NULL||enable==NULL)
		return (-1);

	memset(&WlanEnable_Entry,0,sizeof(MIB_CE_MBSSIB_T));

	ret=mib_chain_get_wlan(ifname,&WlanEnable_Entry,&vwlan_idx);
	if(ret!=0)
		return (-1);

	if(WlanEnable_Entry.wlanDisabled == 0)
		*enable = 1;
	else
		*enable = 0;

	return ret;
}

int rtk_wlan_is_ax_support_by_index(int wlan_index)
{
	MIB_CE_MBSSIB_T Entry;

	if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, WLAN_ROOT_ITF_INDEX, (void *)&Entry)==0)
		return -1;

	if(Entry.is_ax_support)
		return 1;
	else
		return 0;
}

int rtk_wlan_is_ax_support(const char *ifname)
{
	MIB_CE_MBSSIB_T Entry;
	int wlan_index=0, vwlan_index=0;

	if(rtk_wlan_get_mib_index_mapping_by_ifname(ifname, &wlan_index, &vwlan_index)==-1)
		return -1;
	if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, vwlan_index, (void *)&Entry)==0)
		return -1;

	if(Entry.is_ax_support)
		return 1;
	else
		return 0;
}
char *get_token(char *data, char *token)
{
	char *ptr=data;
	int len=0, idx=0;
	while (*ptr && *ptr != '\n' ) {
		if (*ptr == '=') {
			if (len <= 1)
				return NULL;
			memcpy(token, data, len);

			/* delete ending space */
			for (idx=len-1; idx>=0; idx--) {
				if (token[idx] !=  ' ')
					break;
			}
			token[idx+1] = '\0';

			return ptr+1;
		}
		len++;
		ptr++;
	}
	return NULL;
}

int get_value(char *data, char *value)
{
	char *ptr=data;
	int len=0, idx, i;

	while (*ptr && *ptr != '\n' && *ptr != '\r') {
		len++;
		ptr++;
	}

	/* delete leading space */
	idx = 0;
	while (len-idx > 0) {
		if (data[idx] != ' ')
			break;
		idx++;
	}
	len -= idx;

	/* delete bracing '"' */
	if (data[idx] == '"') {
		for (i=idx+len-1; i>idx; i--) {
			if (data[i] == '"') {
				idx++;
				len = i - idx;
			}
			break;
		}
	}

	if (len > 0) {
		memcpy(value, &data[idx], len);
		value[len] = '\0';
	}
	return len;
}

static char *get_wlan_proc_token(char *data, char *token)
{
	char *ptr=NULL, *data_ptr=data;
	int len=0;

	/* skip leading space */
	while(*data_ptr && *data_ptr == ' '){
		data_ptr++;
	}

	ptr = data_ptr;
	while (*ptr && *ptr != '\n' ) {
		if (*ptr == ':') {
			if (len <= 1)
				return NULL;
			memcpy(token, data_ptr, len);

			token[len] = '\0';

			return ptr+1;
		}
		len++;
		ptr++;
	}
	return NULL;
}

static int get_wlan_proc_value(char *data, char *value)
{
	char *ptr=data;
	int len=0, idx=0;

	while (*ptr && *ptr != '\n' && *ptr != '\r'&& *ptr != '(') {
		len++;
		ptr++;
	}

	/* delete leading space */
	idx = 0;
	while (len-idx > 0) {
		if (data[idx] != ' ')
			break;
		idx++;
	}
	len -= idx;

	if (len > 0) {
		memcpy(value, &data[idx], len);
		value[len] = '\0';
	}
	return len;
}

int get_wlan_net_device_stats(const char *ifname, struct net_device_stats *nds)
{
	char wlan_path[256] = {0};
	char line[1024] = {0};
	char token[512]={0}, value[512]={0};
	FILE *fp = NULL;
	char *ptr = NULL;
	//snprintf(wlan_path, sizeof(wlan_path), "/proc/%s/stats", ifname);
	snprintf(wlan_path, sizeof(wlan_path), LINK_FILE_NAME_FMT, ifname, "stats");

	fp = fopen(wlan_path, "r");
	if (fp) {
		while (fgets(line, sizeof(line),fp)) {
			ptr = get_wlan_proc_token(line, token);
			if (ptr == NULL)
				continue;
			if (get_wlan_proc_value(ptr, value)==0){
				continue;
			}
			//fprintf(stderr, "%s::%s:%s\n", __func__,token, value);
			if (!strcmp(token, "rx_bytes")) {
				nds->rx_bytes = strtoul(value, NULL, 10);
			}
			else if (!strcmp(token, "rx_packets")) {
				nds->rx_packets = strtoul(value, NULL, 10);
			}
			else if (!strcmp(token, "rx_errors")) {
				nds->rx_errors = strtoul(value, NULL, 10);
			}
			else if (!strcmp(token, "rx_data_drops")) {
				nds->rx_dropped = strtoul(value, NULL, 10);
			}
			else if (!strcmp(token, "tx_bytes")) {
				nds->tx_bytes = strtoul(value, NULL, 10);
			}
			else if (!strcmp(token, "tx_packets")) {
				nds->tx_packets = strtoul(value, NULL, 10);
			}
			else if (!strcmp(token, "tx_fails")) {
				nds->tx_errors = strtoul(value, NULL, 10);
			}
			else if (!strcmp(token, "tx_drops")) {
				nds->tx_dropped = strtoul(value, NULL, 10);
			}
			else if (!strcmp(token, "tx_ucast_pkts")) {
				nds->tx_ucast_pkts = strtoul(value, NULL, 10);
			}
			else if (!strcmp(token, "tx_mcast_pkts")) {
				nds->tx_mcast_pkts = strtoul(value, NULL, 10);
			}
			else if (!strcmp(token, "tx_bcast_pkts")) {
				nds->tx_bcast_pkts = strtoul(value, NULL, 10);
			}
		}
		fclose(fp);
	}

#if 0
	fprintf(stderr,"ifname:%s\n", ifname);
	fprintf(stderr,"rx_bytes:%lu\n", nds->rx_bytes);
	fprintf(stderr,"rx_packets:%lu\n", nds->rx_packets);
	fprintf(stderr,"rx_errors:%lu\n", nds->rx_errors);
	fprintf(stderr,"rx_dropped:%lu\n", nds->rx_dropped);
	fprintf(stderr,"tx_bytes:%lu\n", nds->tx_bytes);
	fprintf(stderr,"tx_packets:%lu\n", nds->tx_packets);
	fprintf(stderr,"tx_errors:%lu\n", nds->tx_errors);
	fprintf(stderr,"tx_dropped:%lu\n", nds->tx_dropped);
	fprintf(stderr,"tx_ucast_pkts:%lu\n", nds->tx_ucast_pkts);
	fprintf(stderr,"tx_mcast_pkts:%lu\n", nds->tx_mcast_pkts);
	fprintf(stderr,"tx_bcast_pkts:%lu\n", nds->tx_bcast_pkts);
#endif

	return 0;
}

int get_wlan_sta_stats(const char *ifname, net_device_sta_stats *ndss)
{
	char wlan_path[256] = {0};
	char line[1024] = {0};
	char token[512]={0}, value[512]={0};
	FILE *fp = NULL;
	char *ptr = NULL;
	//snprintf(wlan_path, sizeof(wlan_path), "/proc/%s/sta_info", ifname);
	snprintf(wlan_path, sizeof(wlan_path), LINK_FILE_NAME_FMT, ifname, "sta_info");

	fp = fopen(wlan_path, "r");
	if (fp) {
		while (fgets(line, sizeof(line),fp)) {
			ptr = get_wlan_proc_token(line, token);
			if (ptr == NULL)
				continue;
			if (get_wlan_proc_value(ptr, value)==0){
				continue;
			}
			//fprintf(stderr, "%s::%s:%s\n", __func__,token, value);
			if (!strcmp(token, "tx_bytes")) {
				ndss->tx_bytes = strtoul(value, NULL, 10);
			}
			else if (!strcmp(token, "tx_pkts")) {
				ndss->tx_pkts = strtoul(value, NULL, 10);
			}
			else if (!strcmp(token, "tx_fails")) {
				ndss->tx_fail = strtoul(value, NULL, 10);
			}
			else if (!strcmp(token, "rx_bytes")) {
				ndss->rx_bytes = strtoul(value, NULL, 10);
			}
			else if (!strcmp(token, "rx_pkts")) {
				ndss->rx_pkts = strtoul(value, NULL, 10);
			}
		}
		fclose(fp);
	}

	return 0;
}


int getWlBssInfo(const char *interface, bss_info *pInfo)
{
#if 0
	int skfd;
	struct iwreq wrq;
	struct ifreq ifr;
	union iwreq_data u;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
	  close( skfd );
	  /* If no wireless name : no wireless extensions */
	  return -1;
	}

	strcpy((char*)pInfo, "get_bss_info");

	memset(&u, 0, sizeof(union iwreq_data));
	u.data.pointer = pInfo;
	u.data.length = (unsigned short)sizeof(bss_info);

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_ifrn.ifrn_name, interface, strlen(interface));
	ifr.ifr_ifru.ifru_data = &u;

	if (iw_get_ext(skfd, interface, SIOCDEVPRIVATE, (struct iwreq *)&ifr) < 0) {
	  close( skfd );
	  return -1;
	}

	close( skfd );

	return 0;

#else
	int skfd, cmd_id, vwlan_idx=0;
	struct iwreq wrq;
	MIB_CE_MBSSIB_T Entry;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)interface,&Entry,&vwlan_idx)!=0){
		if(skfd != -1){
			close( skfd );
		}
		return -1;
	}

	wrq.u.data.pointer = (caddr_t)pInfo;
	wrq.u.data.length = sizeof(bss_info);

	if(Entry.is_ax_support){
		wrq.u.data.flags = SIOCGIWRTLGETBSSINFO;
		cmd_id = SIOCDEVPRIVATEAXEXT;
	}else{
		cmd_id = SIOCGIWRTLGETBSSINFO;
	}

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0) {
		close( skfd );
		return -1;
	}

	close( skfd );

	return 0;
#endif
}

int getWlNoiseInfo(char *interface, signed int *pInfo){
	int ret = -1;
	FILE *fp = NULL;
	char wlan_path[128] = {0}, line[1024] = {0};
	//snprintf(wlan_path, sizeof(wlan_path), "/proc/%s/mib_noise_level" , interface);
	snprintf(wlan_path, sizeof(wlan_path), LINK_FILE_NAME_FMT, interface, "mib_noise_level");

	fp = fopen(wlan_path, "r");
	if(fp){
		if(fgets(line, sizeof(line),fp)){
			//cat /proc/wlan0/mib_noise_level
			//noise level:[-80]
			if(sscanf(line,"%*[^:]: [%d]", pInfo))
				ret = 0;
			//fprintf(stderr, "[%s] ifname [%s], *pInfo=%d\n", __func__, interface, *pInfo);
		}else{
			fprintf(stderr, "read file %s line fail\n", wlan_path);
		}
		fclose(fp);
	}
	return ret;
}


int getWlInterferencePercentInfo(char *interface, unsigned long *pInfo){
	int ret = -1;
	FILE *fp = NULL;
	char wlan_path[128] = {0}, line[1024] = {0};
	unsigned long percent = 0;
	//snprintf(wlan_path, sizeof(wlan_path), "/proc/%s/stats", interface);
	snprintf(wlan_path, sizeof(wlan_path), LINK_FILE_NAME_FMT, interface, "stats");

	fp = fopen(wlan_path, "r");
	if (fp) {
		while (fgets(line, sizeof(line),fp)) {
			if (strstr(line, "ch_utilization")) {
				if(sscanf(line,"%*[^:]: %lu", &percent))
					*pInfo = percent*100/255;
				ret = 0	;
				//fprintf(stderr, "[%s] ifname [%s], *pInfo = %lu\n", __func__, interface, *pInfo);
				break;
			}
		}
		fclose(fp);
	}
	return ret;
}

int getWlCurrentChannelWidthInfo(char *interface, unsigned int *pInfo){
	int ret = -1;
	FILE *fp = NULL;
	char wlan_path[128] = {0}, line[1024] = {0};
	unsigned int use40M = 0;
	unsigned int coexist = 0;
	snprintf(wlan_path, sizeof(wlan_path), LINK_FILE_NAME_FMT, interface, "mib_11n");

	fp = fopen(wlan_path, "r");
	if (fp) {
		while (fgets(line, sizeof(line),fp)) {
			if (strstr(line, "use40M")) {
				if(sscanf(line,"%*[^:]: %u", &use40M)){
				}
			}
			if (strstr(line, "coexist")) {
				if(sscanf(line,"%*[^:]: %u", &coexist)){
				}
			}
		}
		fclose(fp);
	}
	if(use40M==0){
		*pInfo = HT20;
	}
	else if(use40M==1 && coexist==0){
		*pInfo = HT40;
	}
	else if(use40M==1 && coexist==1){
		*pInfo = HT20_40;
	}
	else if(use40M==2){
		*pInfo = HT80;
	}
	//fprintf(stderr, "[%s] ifname [%s], use40M = %u, coexist = %u, *pInfo = %u\n", __func__, interface, use40M, coexist, *pInfo);
	
	return ret;
}


void set_bssdb_info(SS_STATUS_Tp pStatus, bss_desc_2_web *ss_result, int ss_number)
{
	int i, j, tmp_num=0, tmp_flag=0;
	for(i=pStatus->number, j=0; j<ss_number; i++, j++)
	{
		if(tmp_flag == 1)
		{
			i--;
			tmp_flag = 0;
		}

		if(ss_result[j].ssidlen == 0 || strlen(ss_result[j].ssid) == 0)
		{
			tmp_flag = 1;
			continue;
		}
		memcpy(pStatus->bssdb[i].bssid, ss_result[j].bssid, 6);
		strncpy(pStatus->bssdb[i].ssid, ss_result[j].ssid, sizeof(pStatus->bssdb[i].ssid));
		if(strlen(ss_result[j].ssid) < sizeof(pStatus->bssdb[i].ssid))
			pStatus->bssdb[i].ssidlen = strlen(ss_result[j].ssid);
		else
			pStatus->bssdb[i].ssidlen = sizeof(pStatus->bssdb[i].ssid);
		pStatus->bssdb[i].channel = ss_result[j].channel;
		pStatus->bssdb[i].rssi =ss_result[j].rssi;
		pStatus->bssdb[i].capability = ss_result[j].capability;
		pStatus->bssdb[i].beacon_prd = ss_result[j].beacon_period;
		pStatus->bssdb[i].supportrate = ss_result[j].support_rate;
		pStatus->bssdb[i].basicrate = ss_result[j].basic_rate;
		pStatus->bssdb[i].network = 0;
		if(ss_result[j].wireless_mode & 0x01)
			pStatus->bssdb[i].network |= BAND_11B;
		if(ss_result[j].wireless_mode & 0x02)
			pStatus->bssdb[i].network |= BAND_11A;
		if(ss_result[j].wireless_mode & 0x04)
			pStatus->bssdb[i].network |= BAND_11G;
		if(ss_result[j].wireless_mode & 0x08)
			pStatus->bssdb[i].network |= BAND_11N;
		if(ss_result[j].wireless_mode & 0x10)
			pStatus->bssdb[i].network |= BAND_5G_11AC;
		if(ss_result[j].wireless_mode & 0x20)
			pStatus->bssdb[i].network |= BAND_5G_11AX;

		pStatus->bssdb[i].t_stamp[0] = 0;
		pStatus->bssdb[i].t_stamp[1] = 0;
		if(ss_result[j].encrypt == ENCRYP_PROTOCOL_WEP)
		{
			pStatus->bssdb[i].t_stamp[0] = 0;
		}
		else if(ss_result[j].encrypt == ENCRYP_PROTOCOL_WPA)
		{
			if(ss_result[j].akm & 2)
				pStatus->bssdb[i].t_stamp[0] |= (0x4 << 12);
			if(ss_result[j].akm & 1)
				pStatus->bssdb[i].t_stamp[0] |= (0x2 << 12);

			if(ss_result[j].pairwise_cipher & 0x10)
				pStatus->bssdb[i].t_stamp[0] |= (0x4 << 8);
			if(ss_result[j].pairwise_cipher & 0x08)
				pStatus->bssdb[i].t_stamp[0] |= (0x1 << 8);
		}
		else if(ss_result[j].encrypt == ENCRYP_PROTOCOL_RSN)
		{
			//WPA2/WPA2-MIXED/WPA3/WPA2-WPA3-MIXED
			if(ss_result[j].akm & 2)
				pStatus->bssdb[i].t_stamp[0] |= (0x4 << 28);//psk
			if(ss_result[j].akm & 1)
				pStatus->bssdb[i].t_stamp[0] |= (0x2 << 28);//8021x

#if !defined(CONFIG_RTK_DEV_AP)
			if(ss_result[j].mfp_opt == BSS_MFP_REQUIRED)
				pStatus->bssdb[i].t_stamp[1] |= BSS_PMF_REQ;
			else if(ss_result[j].mfp_opt == BSS_MFP_OPTIONAL)
				pStatus->bssdb[i].t_stamp[1] |= BSS_PMF_CAP;
			else if(ss_result[j].mfp_opt == BSS_MFP_NO)
				pStatus->bssdb[i].t_stamp[1] |= BSS_PMF_NONE;
#endif

			if(ss_result[j].akm & 128)//WPA3
				pStatus->bssdb[i].t_stamp[1] |= 0x00000800;

			if(ss_result[j].pairwise_cipher & 0x10)
				pStatus->bssdb[i].t_stamp[0] |= (0x4 << 24);//aes
			if(ss_result[j].pairwise_cipher & 0x08)
				pStatus->bssdb[i].t_stamp[0] |= (0x1 << 24);//tkip

			//WPA2-MIXED
			if((ss_result[j].pairwise_cipher & 0x10) && (ss_result[j].pairwise_cipher & 0x08))
			{
				if(ss_result[j].akm & 2)
				{
					pStatus->bssdb[i].t_stamp[0] |= (0x4 << 12);//psk
				}
				else if(ss_result[j].akm & 1)
				{
					pStatus->bssdb[i].t_stamp[0] |= (0x2 << 12);//8021x
				}

				pStatus->bssdb[i].t_stamp[0] |= (0x5 << 8);//aes+tkip
			}
		}

	pStatus->bssdb[i].t_stamp[1] |= (ss_result[j].channel_bandwidth << BSS_BW_SHIFT);
		tmp_num++;
	}
	pStatus->number += tmp_num;

}

int getWlSiteSurveyResult(char *interface, SS_STATUS_Tp pStatus )
{
	int skfd, cmd_id, vwlan_idx=0;
	struct iwreq wrq;
	MIB_CE_MBSSIB_T Entry;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)interface,&Entry,&vwlan_idx)!=0){
		if(skfd != -1){
			close( skfd );
		}
		return -1;
	}

	if(Entry.is_ax_support)
	{

		char buffer[sizeof(sheet_hdr_2_web)+ (MAX_BSS_DESC*sizeof(BssDscr_ax))];
		bss_desc_2_web *ss_result = (bss_desc_2_web *)(buffer + sizeof(sheet_hdr_2_web));
		sheet_hdr_2_web *ss_hdr = (sheet_hdr_2_web *) buffer;
		int ss_number = 0, i, j;
		unsigned char sheet_sequence = 0, sheet_total = 0;

		memset(buffer, 0, sizeof(buffer));
		strcpy((char*)buffer, "sheet=0");

		wrq.u.data.pointer = buffer;
		wrq.u.data.length = sizeof(buffer);

		wrq.u.data.flags = SIOCGIWRTLGETBSSDB;
		cmd_id = SIOCDEVPRIVATEAXEXT;
		if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0) {
			close( skfd );
			if(errno == EINPROGRESS){
				pStatus->number = 0xff;
				return 0;
			}
			else{
				return -1;
			}
		}

		close( skfd );

		if(pStatus->number == 1){
			//do nothing
		}
		else{
			pStatus->number = 0;
			sheet_total = ss_hdr->sheet_total;
			if(sheet_total == 0)
				return -1;
			ss_number = (wrq.u.data.length-sizeof(sheet_hdr_2_web))/sizeof(BssDscr_ax);
			set_bssdb_info(pStatus, ss_result, ss_number);
			sheet_sequence++;
			//printf("%s %d %d %d %d\n",__func__, ss_hdr->sheet_total, ss_hdr->sheet_sequence, wrq.u.data.length, ss_number);
			while(sheet_sequence < sheet_total){
				skfd = socket(AF_INET, SOCK_DGRAM, 0);
				if(skfd==-1)
					return -1;

				/* Get wireless name */
				if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
				  close( skfd );
				  /* If no wireless name : no wireless extensions */
				  return -1;
				}

				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer), "sheet=%d", sheet_sequence);

				wrq.u.data.pointer = buffer;
				wrq.u.data.length = sizeof(buffer);

				wrq.u.data.flags = SIOCGIWRTLGETBSSDB;
				cmd_id = SIOCDEVPRIVATEAXEXT;
				if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0) {
					close( skfd );
					if(errno == EINPROGRESS){
						//pStatus->number = 0xff;
						return 0;
					}
					else{
						return -1;
					}
				}

				close( skfd );

				sheet_total = ss_hdr->sheet_total;
				ss_number = (wrq.u.data.length-sizeof(sheet_hdr_2_web))/sizeof(BssDscr_ax);
				//printf("%s %d %d %d %d\n",__func__, ss_hdr->sheet_total, ss_hdr->sheet_sequence, u.data.length, ss_number);
				if((pStatus->number+ss_number) > MAX_BSS_DESC){
					printf("%s %d: too many bss number!\n", __func__, __LINE__);
					break;
				}

				set_bssdb_info(pStatus, ss_result, ss_number);
				sheet_sequence++;
			}
		}
	}
	else
	{
		wrq.u.data.pointer = (caddr_t)pStatus;
		if( pStatus->number == 0 ){
			wrq.u.data.length = sizeof(SS_STATUS_T);
		}
		else{
			wrq.u.data.length = sizeof(pStatus->number);
		}

		cmd_id = SIOCGIWRTLGETBSSDB;
		if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0) {
			close( skfd );
			return -1;
		}

		close( skfd );
	}

	return 0;
}

int getWlSiteSurveyRequest(char *interface, int *pStatus)
{
	int skfd, cmd_id, vwlan_idx=0;
	struct iwreq wrq;
	unsigned char result;
	MIB_CE_MBSSIB_T Entry;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)interface,&Entry,&vwlan_idx)!=0){
		if(skfd != -1){
			close( skfd );
		}
		return -1;
	}

	wrq.u.data.pointer = (caddr_t)&result;
	wrq.u.data.length = sizeof(result);

	if(Entry.is_ax_support)
	{
		wrq.u.data.flags = SIOCGIWRTLSCANREQ;
		cmd_id = SIOCDEVPRIVATEAXEXT;
	}
	else
	{
		cmd_id = SIOCGIWRTLSCANREQ;
	}

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0) {
		close( skfd );
		if(Entry.is_ax_support)
		{
			if(errno == EALREADY){
				*pStatus  = -1; //retry later
				return 0;
			}
			else
				return -1;
		}
		else
		{
			if(errno == 4 || errno == 1)
			{
				*pStatus  = -1; //retry later
				return 0;
			}
			else
				return -1;
		}
	}

	close( skfd );

	if(Entry.is_ax_support)
		*pStatus = 0;
	else
	{
		if ( result == 0xff )
			*pStatus = -1;
		else
			*pStatus = (int) result;
	}

	return 0;
}

#if defined(WLAN_CLIENT)
/////////////////////////////////////////////////////////////////////////////
int getWlJoinRequest(char *interface, pBssDscr pBss, unsigned char *res)
{
	int skfd, cmd_id, vwlan_idx=0;
	struct iwreq wrq;
	MIB_CE_MBSSIB_T Entry;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;
	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)interface,&Entry,&vwlan_idx)!=0){
		if(skfd != -1){
			close( skfd );
		}
		return -1;
	}

	wrq.u.data.pointer = (caddr_t)pBss;
	wrq.u.data.length = sizeof(BssDscr);
	if(Entry.is_ax_support)
	{
		wrq.u.data.flags = SIOCGIWRTLJOINREQ;
		cmd_id = SIOCDEVPRIVATEAXEXT;
	}
	else
	{
		cmd_id = SIOCGIWRTLJOINREQ;
	}

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0) {
		close( skfd );
		return -1;
	}

	close( skfd );

	memcpy( res, wrq.u.data.pointer, 1);

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
int getWlJoinResult(char *interface, unsigned char *res)
{
	int skfd, cmd_id, vwlan_idx=0;
	struct iwreq wrq;
	MIB_CE_MBSSIB_T Entry;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)interface,&Entry,&vwlan_idx)!=0){
		if(skfd != -1){
			close( skfd );
		}
		return -1;
	}

	wrq.u.data.pointer = (caddr_t)res;
	wrq.u.data.length = 1;
	if(Entry.is_ax_support)
	{
		wrq.u.data.flags = SIOCGIWRTLJOINREQSTATUS;
		cmd_id = SIOCDEVPRIVATEAXEXT;
	}
	else
	{
		cmd_id = SIOCGIWRTLJOINREQSTATUS;
	}

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0) {
		close( skfd );
		return -1;
	}

	close( skfd );

	return 0;
}

void getSiteSurveyWlanNeighborAsync(char wlan_idx)
{
	char *argv[3]={0}, argv_buf[32];
	char *envp[2]={0};
	const char filename[40];
	int pid;

	snprintf((char *)filename, sizeof(filename), "/bin/SiteSurveyWLANNeighbor");
	argv[0] = (char *)filename;
	snprintf(argv_buf, 32, "%d", wlan_idx);
	argv[1] = (char *)argv_buf;
	argv[2] = NULL;
	pid=fork();
	if(pid < 0) {
		AUG_PRT("fork() fail ! \n");
	} else if (pid == 0) {
		envp[0] = "PATH=/bin:/usr/bin:/etc:/sbin:/usr/sbin";
		envp[1] = NULL;
		execve(filename, argv, envp);
		AUG_PRT("exec %s failed\n", filename);
		_exit(2);
	}
}
#endif	// of WLAN_CLIENT


int getMiscData(char *interface, struct _misc_data_ *pData)
{
	int vwlan_idx = 0;
	MIB_CE_MBSSIB_T Entry;

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)interface,&Entry,&vwlan_idx)!=0){
		return -1;
	}

	if(Entry.is_ax_support == 0){
		int skfd;
		struct iwreq wrq;

		skfd = socket(AF_INET, SOCK_DGRAM, 0);

		/* Get wireless name */
		if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0){
			/* If no wireless name : no wireless extensions */
			close(skfd);
			return -1;
		}

		wrq.u.data.pointer = (caddr_t)pData;
		wrq.u.data.length = sizeof(struct _misc_data_);

		if (iw_get_ext(skfd, interface, SIOCGMISCDATA, &wrq) < 0){
			close(skfd);
			return -1;
		}
		if(skfd != -1)
			close(skfd);
	}else{
		//hard coded for 8852AE
		memset(pData, '\0', sizeof(struct _misc_data_));
		pData->mimo_tr_used = 2;
	}

	return 0;
}

int getWlStaNum( char *interface, int *num )
{
#if 0
	int skfd;
	struct iwreq wrq;
	struct ifreq ifr;
	union iwreq_data u;
	char buffer[] = "get_sta_num";

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
	  close( skfd );
	  /* If no wireless name : no wireless extensions */
	  return -1;
	}

	memset(&u, 0, sizeof(union iwreq_data));
	u.data.pointer = buffer;
	u.data.length = (unsigned short)sizeof(buffer);

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_ifrn.ifrn_name, interface, strlen(interface));
	ifr.ifr_ifru.ifru_data = &u;

	if (iw_get_ext(skfd, interface, SIOCDEVPRIVATE, (struct iwreq *)&ifr) < 0) {
	  close( skfd );
	  return -1;
	}

	close( skfd );

	*num  = *(unsigned char*)buffer;

	return 0;
#else
	int skfd, cmd_id, vwlan_idx=0;
	unsigned char staNum1;
	unsigned short staNum2;
	struct iwreq wrq;
	MIB_CE_MBSSIB_T Entry;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)interface,&Entry,&vwlan_idx)!=0){
		if(skfd != -1){
			close( skfd );
		}
		return -1;
	}

	if(Entry.is_ax_support){
		wrq.u.data.pointer = (caddr_t)&staNum1;
		wrq.u.data.length = sizeof(staNum1);
		wrq.u.data.flags = SIOCGIWRTLSTANUM;
		cmd_id = SIOCDEVPRIVATEAXEXT;
	}else{
		wrq.u.data.pointer = (caddr_t)&staNum2;
		wrq.u.data.length = sizeof(staNum2);
		cmd_id = SIOCGIWRTLSTANUM;
	}

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0) {
		close( skfd );
		return -1;
	}

	if(Entry.is_ax_support){
		*num  = (int)staNum1;
	}else{
		*num  = (int)staNum2;
	}

	//*num  = (int)staNum;

	if(skfd != -1)
		close( skfd );

	return 0;
#endif
}

int getWlEnc( char *interface , char *buffer, char *num)
{
	int skfd, cmd_id, vwlanidx=0;
	struct iwreq wrq;
	MIB_CE_MBSSIB_T WlanEnable_Entry;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
	{
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(&WlanEnable_Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan(interface,&WlanEnable_Entry,&vwlanidx)!=0)
		return -1;

	wrq.u.data.pointer = (caddr_t)buffer;
	if(WlanEnable_Entry.is_ax_support)
	{
		wrq.u.data.flags = RTL8192CD_IOCTL_GET_MIB;
		cmd_id = SIOCDEVPRIVATEAXEXT;
	}
	else
	{
		cmd_id = RTL8192CD_IOCTL_GET_MIB;
	}

	wrq.u.data.length = strlen(buffer);

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0)
	{
	close( skfd );
		return -1;
	}

	*num  = buffer[0];
	if(skfd != -1)
		close( skfd );

	return 0;
}

void rtk_wlan_get_sta_rate(WLAN_STA_INFO_Tp pInfo, int rate_type, char *rate_str, unsigned int len)
{
	unsigned char OperaRate = 0;
	char channelWidth = 0;//20M 0,40M 1,80M 2

	if(rate_type == WLAN_STA_TX_RATE)
		OperaRate = pInfo->TxOperaRate;
	else if(rate_type == WLAN_STA_RX_RATE)
		OperaRate = pInfo->RxOperaRate;

	if(OperaRate >= VHT_RATE_ID) {
		if(pInfo->ht_info & 0x4)
			channelWidth=2;
		else if(pInfo->ht_info & 0x1)
			channelWidth=1;
		else
			channelWidth=0;
		snprintf(rate_str, len, "%d", VHT_MCS_DATA_RATE[channelWidth][(pInfo->ht_info & 0x2)?1:0][OperaRate-VHT_RATE_ID]>>1);
	} else if(OperaRate >= HT_RATE_ID){
		snprintf(rate_str, len, "%s", MCS_DATA_RATEStr[(pInfo->ht_info & 0x1)?1:0][(pInfo->ht_info & 0x2)?1:0][OperaRate-HT_RATE_ID]);
	}
	else{
		if(OperaRate % 2) {
			snprintf(rate_str, len, "%d%s",	OperaRate / 2, ".5");
		}else{
			snprintf(rate_str, len, "%d", OperaRate / 2);
		}
	}
}

void rtk_wlan_get_sta_extra_rate(void *addr, WLAN_STA_EXTRA_INFO_Tp pInfo, int rate_type, char *rate_str, unsigned int len)
{
	int i = 0, found = 0;
	if(addr != NULL){
		for (i = 1; i <= MAX_STA_NUM; i++){
			if(memcmp(addr, pInfo->addr, MAC_ADDR_LEN) == 0){
				found = 1;
				break;
			}
			pInfo++;
		}
	}
	else if(pInfo != NULL){
		found = 1;
	}
	if(found == 1){
		if(rate_type == WLAN_STA_TX_RATE)
			snprintf(rate_str, len, "%s", pInfo->txrate);
		else
			snprintf(rate_str, len, "%s", pInfo->rxrate);
	}
}


#ifdef CONFIG_E8B
double rtk_wlan_get_sta_current_tx_rate(WLAN_STA_INFO_Tp pInfo)
{
	char txrate_str[20]={0};
	double txrate = 0.0;
	rtk_wlan_get_sta_rate(pInfo, WLAN_STA_TX_RATE, txrate_str, 20);
	txrate = atof(txrate_str);
	return txrate; //Mbps
}

#if defined(WLAN_BAND_STEERING)
// return value:
// 0  : one of wlan root interface is disabled
// 1 : both enable
unsigned int get_root_wlan_status(void)
{
	int i, orig_wlan_idx = wlan_idx;
	MIB_CE_MBSSIB_T Entry;
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
	unsigned char no_wlan;
#endif
	for(i=0; i<NUM_WLAN_INTERFACE; i++) {
		wlan_idx = i;
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
		mib_get_s(MIB_WLAN_MODULE_DISABLED, (void *)&no_wlan, sizeof(no_wlan));
		if(no_wlan == 1) {
			wlan_idx = orig_wlan_idx;
			return 0;
		}
#endif
		wlan_getEntry(&Entry, 0);
		if(Entry.wlanDisabled == 1) {
			wlan_idx = orig_wlan_idx;
			return 0;
		}
	}
	wlan_idx = orig_wlan_idx;
	return 1;
}
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#if defined(CONFIG_USER_RTK_FASTPASSNF_MODULE)
#define PROC_FASTPASSNF_IGNORE_SAMBA_IF "/proc/fastPassNF/IGNORE_SAMBA_IF"
#endif

void rtk_wlan_monitor_restart_daemon(void)
{
	//todo
}
int IpPortStrToSockaddr(char *str, ipPortRange *ipport)
{
	char *c, *tmp_port = NULL, *cc;
	char buf[65], *v2 = NULL;
	int port;
	if(str == NULL || ipport == NULL) return 0;

	memset(ipport, 0, sizeof(ipPortRange));

#ifdef CONFIG_IPV6
	cc = strrchr(str, ']');
	if(cc == NULL)
		cc = str;
#else
		cc = str;
#endif //CONFIG_IPV6

	if((c = strchr(cc, ':'))){
		v2 = NULL;
		tmp_port = c+1;
		strncpy(buf, str, c-str);
		buf[c-str] = '\0';
		if((c = strchr(buf, '-'))){
			*c = '\0';
			v2 = c+1;
		}
#ifdef CONFIG_IPV6
		if('['==buf[0]) //ipv6
		{
			ipport->sin_family = AF_INET6;
			ipport->eth_protocol = 0x86DD;
			buf[strlen(buf)-1] = '\0'; //remove ']'
			if(inet_pton(AF_INET6, buf+1, ipport->start_addr) != 1)
				return 0;
			if(v2){
				if((c = strchr(v2, ']')))
					*c = '\0'; //remove ']'
				if(inet_pton(AF_INET6, v2+1, ipport->end_addr) != 1)
					return 0;
			}else
				memcpy(ipport->end_addr, ipport->start_addr, sizeof(ipport->end_addr));

		}
		else
#endif //CONFIG_IPV6
		{
			ipport->sin_family = AF_INET;
			ipport->eth_protocol = 0x0800;
			if(inet_aton(buf, (struct in_addr *)ipport->start_addr) == 0)
				return 0;
			if(v2){
				if(inet_aton(v2, (struct in_addr *)ipport->end_addr) == 0)
					return 0;
			}else
				memcpy(ipport->end_addr, ipport->start_addr, sizeof(ipport->end_addr));
		}
	}

	if(tmp_port){
		v2 = NULL;
		strcpy(buf, tmp_port);
		if((c = strchr(buf, '-'))){
			*c = '\0';
			v2 = c+1;
		}
		port = atoi(buf);
		if(port<0 || port>65535) return 0;
		ipport->start_port = port;

		if(v2){
			port = atoi(buf);
			if(port<0 || port>65535) return 0;
			ipport->end_port = port;
		}
		else
			ipport->end_port = ipport->start_port;
	}
	return 1;
}

int setup_wlan_accessRule_netfilter_init()
{
	//va_cmd(EBTABLES, 6, 1, "-D", "INPUT", "-i", "wlan+", "-j", "WLACL_INPUT");
	//va_cmd(EBTABLES, 6, 1, "-D", "FORWARD", "-i", "wlan+", "-j", "WLACL_FORWARD");

	//va_cmd(EBTABLES, 2, 1, "-X", "WLACL_INPUT");
	//va_cmd(EBTABLES, 2, 1, "-X", "WLACL_FORWARD");
	va_cmd(EBTABLES, 2, 1, "-N", "WLACL_INPUT");
	va_cmd(EBTABLES, 2, 1, "-N", "WLACL_FORWARD");
	va_cmd(EBTABLES, 3, 1, "-P", "WLACL_INPUT", "RETURN");
	va_cmd(EBTABLES, 3, 1, "-P", "WLACL_FORWARD", "RETURN");
	//va_cmd(EBTABLES, 3, 1, "-A", "INPUT", "--logical-in wlan+ -j WLACL_INPUT");
	//va_cmd(EBTABLES, 3, 1, "-A", "FORWARD", "--logical-in wlan+ -j WLACL_FORWARD");
	va_cmd(EBTABLES, 6, 1, "-A", "INPUT", "-i", "wlan+", "-j", "WLACL_INPUT");
	va_cmd(EBTABLES, 6, 1, "-A", "FORWARD", "-i", "wlan+", "-j", "WLACL_FORWARD");

	return 1;
}

static int _setup_wlan_accessRule_netfilter_iptables_only(char *ifname, char *chain_input, char *chain_forward)
{
	int n=0, i=0;
	MIB_CE_ATM_VC_T atm_entry;
	char wan_name[IFNAMSIZ];

	va_cmd(IPTABLES, 2, 1, "-N", chain_input);
	va_cmd(IPTABLES, 8, 1, "-A", "WLACL_INPUT", "-m", "physdev", "--physdev-in", ifname, "-j", chain_input);
	va_cmd(IPTABLES, 8, 1, "-A", chain_input, "-p", "udp", "--dport", "67:68", "-j", "ACCEPT");
	va_cmd(IPTABLES, 8, 1, "-A", chain_input, "-p", "udp", "--dport", "53", "-j", "ACCEPT");
	va_cmd(IPTABLES, 4, 1, "-A", chain_input, "-j", "DROP");
	va_cmd(IPTABLES, 2, 1, "-N", chain_forward);
	va_cmd(IPTABLES, 8, 1, "-A", "WLACL_FORWARD", "-m", "physdev", "--physdev-in", ifname, "-j", chain_forward);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", chain_input);
	va_cmd(IP6TABLES, 8, 1, "-A", "WLACL_INPUT", "-m", "physdev", "--physdev-in", ifname, "-j", chain_input);
	va_cmd(IP6TABLES, 8, 1, "-A", chain_input, "-p", "icmpv6", "--icmpv6-type","router-solicitation", "-j", "ACCEPT");
	va_cmd(IP6TABLES, 8, 1, "-A", chain_input, "-p", "icmpv6", "--icmpv6-type","router-advertisement", "-j", "ACCEPT");
	va_cmd(IP6TABLES, 8, 1, "-A", chain_input, "-p", "icmpv6", "--icmpv6-type","neighbour-solicitation", "-j", "ACCEPT");
	va_cmd(IP6TABLES, 8, 1, "-A", chain_input, "-p", "icmpv6", "--icmpv6-type","neighbour-advertisement", "-j", "ACCEPT");
	va_cmd(IP6TABLES, 8, 1, "-A", chain_input, "-p", "udp", "--dport", "546:547", "-j", "ACCEPT");
	va_cmd(IP6TABLES, 8, 1, "-A", chain_input, "-p", "udp", "--dport", "53", "-j", "ACCEPT");
	va_cmd(IP6TABLES, 4, 1, "-A", chain_input, "-j", "DROP");
	va_cmd(IP6TABLES, 2, 1, "-N", chain_forward);
	va_cmd(IP6TABLES, 8, 1, "-A", "WLACL_FORWARD", "-m", "physdev", "--physdev-in", ifname, "-j", chain_forward);
#endif
	// check all wan interfaces
	n = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < n; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&atm_entry))
			continue;

		if(!atm_entry.enable)
			continue;

		if((atm_entry.cmode != CHANNEL_MODE_BRIDGE) && (!(atm_entry.applicationtype & X_CT_SRV_INTERNET)))
		{
			ifGetName(atm_entry.ifIndex, wan_name, sizeof(wan_name));
			va_cmd(IPTABLES, 6, 1, "-A", chain_forward, "-o", wan_name, "-j", "DROP");
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 6, 1, "-A", chain_forward, "-o", wan_name, "-j", "DROP");
#endif
		}
	}
	return 1;
}

static int _setup_wlan_accessRule_netfilter_iptables(void)
{
#if defined(CONFIG_USER_RTK_FASTPASSNF_MODULE)
	char cmd_str[256];
#endif
	int i=0, j=0;
	MIB_CE_MBSSIB_T Entry;
	char chain_input[64], chain_forward[64];
	char ifname[IFNAMSIZ];

#if defined(CONFIG_USER_RTK_FASTPASSNF_MODULE)
	snprintf(cmd_str, sizeof(cmd_str), "echo FLUSH > " PROC_FASTPASSNF_IGNORE_SAMBA_IF);
	va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#endif

	for(j=0; j<NUM_WLAN_INTERFACE; j++)
		for(i=0; i<WLAN_SSID_NUM; i++){
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, j, i, &Entry);
			if(!Entry.wlanDisabled){
				rtk_wlan_get_ifname( j, i, ifname);
				sprintf(chain_input, "WLACL_%s_INPUT", ifname);
				va_cmd(IPTABLES, 8, 1, "-D", "WLACL_INPUT", "-m", "physdev", "--physdev-in", ifname, "-j", chain_input);
				va_cmd(IPTABLES, 2, 1, "-F", chain_input);
				va_cmd(IPTABLES, 2, 1, "-X", chain_input);
#ifdef CONFIG_IPV6
				va_cmd(IP6TABLES, 8, 1, "-D", "WLACL_INPUT", "-m", "physdev", "--physdev-in", ifname, "-j", chain_input);
				va_cmd(IP6TABLES, 2, 1, "-F", chain_input);
				va_cmd(IP6TABLES, 2, 1, "-X", chain_input);
#endif
				sprintf(chain_forward, "WLACL_%s_FORWARD", ifname);
				va_cmd(IPTABLES, 8, 1, "-D", "WLACL_FORWARD", "-m", "physdev", "--physdev-in", ifname, "-j", chain_forward);
				va_cmd(IPTABLES, 2, 1, "-F", chain_forward);
				va_cmd(IPTABLES, 2, 1, "-X", chain_forward);
#ifdef CONFIG_IPV6
				va_cmd(IP6TABLES, 8, 1, "-D", "WLACL_FORWARD", "-m", "physdev", "--physdev-in", ifname, "-j", chain_forward);
				va_cmd(IP6TABLES, 2, 1, "-F", chain_forward);
				va_cmd(IP6TABLES, 2, 1, "-X", chain_forward);
#endif
				if(Entry.accessRuleEnable==1)
					_setup_wlan_accessRule_netfilter_iptables_only(ifname, chain_input, chain_forward);

#if defined(CONFIG_USER_RTK_FASTPASSNF_MODULE)
				if(Entry.accessRuleEnable)
				{
					snprintf(cmd_str, sizeof(cmd_str), "echo ADD %s > " PROC_FASTPASSNF_IGNORE_SAMBA_IF, ifname);
					va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
				}
#endif
			}
		}
	return 1;
}

int setup_wlan_accessRule_netfilter_iptables_init(void)
{
	va_cmd(IPTABLES, 2, 1, "-N", "WLACL_INPUT");
	va_cmd(IPTABLES, 8, 1, "-A", "INPUT", "-m", "physdev", "--physdev-in", "wlan+", "-j", "WLACL_INPUT");
	va_cmd(IPTABLES, 2, 1, "-N", "WLACL_FORWARD");
	va_cmd(IPTABLES, 8, 1, "-A", "FORWARD", "-m", "physdev", "--physdev-in", "wlan+", "-j", "WLACL_FORWARD");
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", "WLACL_INPUT");
	va_cmd(IP6TABLES, 8, 1, "-A", "INPUT", "-m", "physdev", "--physdev-in", "wlan+", "-j", "WLACL_INPUT");
	va_cmd(IP6TABLES, 2, 1, "-N", "WLACL_FORWARD");
	va_cmd(IP6TABLES, 8, 1, "-A", "FORWARD", "-m", "physdev", "--physdev-in", "wlan+", "-j", "WLACL_FORWARD");
#endif
	_setup_wlan_accessRule_netfilter_iptables();
	return 1;
}

#ifndef inet_ntoa_r
extern char *inet_ntoa_r(struct in_addr in,char* buf);
#endif

int setup_wlan_accessRule_netfilter(char *ifname, MIB_CE_MBSSIB_Tp pEntry)
{
#if defined(CONFIG_USER_RTK_FASTPASSNF_MODULE)
	char cmd_str[256];
#endif
	char chain_input[64], chain_forward[64];
	char port[32], addr[64];
	char *protocol[] = {"tcp","udp"};
	char *s, *s_tmp, *rule, *pcmd, *paddr;
	char *argv[24];
	int n, i;
	ipPortRange ipport;

	sprintf(chain_input, "WLACL_%s_INPUT", ifname);
	sprintf(chain_forward, "WLACL_%s_FORWARD", ifname);

	//sprintf(cmd, "--logical-in %s -j %s", ifname, chain_input);
	va_cmd(IPTABLES, 8, 1, "-D", "WLACL_INPUT", "-m", "physdev", "--physdev-in", ifname, "-j", chain_input);
	va_cmd(IPTABLES, 8, 1, "-D", "WLACL_FORWARD", "-m", "physdev", "--physdev-in", ifname, "-j", chain_forward);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 8, 1, "-D", "WLACL_INPUT", "-m", "physdev", "--physdev-in", ifname, "-j", chain_input);
	va_cmd(IP6TABLES, 8, 1, "-D", "WLACL_FORWARD", "-m", "physdev", "--physdev-in", ifname, "-j", chain_forward);
#endif
	va_cmd(EBTABLES, 6, 1, "-D", "WLACL_INPUT", "-i", ifname, "-j", chain_input);
	//sprintf(cmd, "--logical-in %s -j %s", ifname, chain_forward);
	va_cmd(EBTABLES, 6, 1, "-D", "WLACL_FORWARD", "-i", ifname, "-j", chain_forward);

	va_cmd(IPTABLES, 2, 1, "-F", chain_input);
	va_cmd(IPTABLES, 2, 1, "-X", chain_input);
	va_cmd(IPTABLES, 2, 1, "-F", chain_forward);
	va_cmd(IPTABLES, 2, 1, "-X", chain_forward);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", chain_input);
	va_cmd(IP6TABLES, 2, 1, "-X", chain_input);
	va_cmd(IP6TABLES, 2, 1, "-F", chain_forward);
	va_cmd(IP6TABLES, 2, 1, "-X", chain_forward);
#endif
	va_cmd(EBTABLES, 2, 1, "-X", chain_input);
	va_cmd(EBTABLES, 2, 1, "-X", chain_forward);

	if(!pEntry->wlanDisabled && pEntry->accessRuleEnable)
	{
		va_cmd(EBTABLES, 2, 1, "-N", chain_input);
		va_cmd(EBTABLES, 2, 1, "-N", chain_forward);
		va_cmd(EBTABLES, 3, 1, "-P", chain_input, "RETURN");
		va_cmd(EBTABLES, 3, 1, "-P", chain_forward, "RETURN");

		//sprintf(cmd, "--logical-in %s -j %s", ifname, chain_input);
		va_cmd(EBTABLES, 6, 1, "-A", "WLACL_INPUT", "-i", ifname, "-j", chain_input);
		//sprintf(cmd, "--logical-in %s -j %s", ifname, chain_forward);
		va_cmd(EBTABLES, 6, 1, "-A", "WLACL_FORWARD", "-i", ifname, "-j", chain_forward);

		if(pEntry->accessRuleEnable == 1)
		{
			//va_cmd(EBTABLES, 6, 1, "-A", chain_input, "-p", "arp", "-j", "ACCEPT");
			//va_cmd(EBTABLES, 10, 1, "-A", chain_input, "-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "ACCEPT");
			//va_cmd(EBTABLES, 10, 1, "-A", chain_input, "-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "53", "-j", "ACCEPT");
			//va_cmd(EBTABLES, 4, 1, "-A", chain_input, "-j", "DROP");
			_setup_wlan_accessRule_netfilter_iptables_only(ifname, chain_input, chain_forward);

			va_cmd(EBTABLES, 6, 1, "-A", chain_forward, "-p", "arp", "-j", "ACCEPT");
			va_cmd(EBTABLES, 6, 1, "-A", chain_forward, "-o", "nas+", "-j", "ACCEPT");
			va_cmd(EBTABLES, 4, 1, "-A", chain_forward, "-j", "DROP");
		}
		else if(pEntry->accessRuleEnable == 2)
		{
			va_cmd(EBTABLES, 6, 1, "-A", chain_input, "-p", "arp", "-j", "ACCEPT");
			va_cmd(EBTABLES, 10, 1, "-A", chain_input, "-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "ACCEPT");
			va_cmd(EBTABLES, 10, 1, "-A", chain_input, "-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "53", "-j", "ACCEPT");
#ifdef CONFIG_IPV6
			va_cmd(EBTABLES, 10, 1, "-A", chain_input, "-p", "IPv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "133:136", "-j", "ACCEPT");
			va_cmd(EBTABLES, 10, 1, "-A", chain_input, "-p", "IPv6", "--ip6-proto", "udp", "--ip6-dport", "546:547", "-j", "ACCEPT");
#endif
			va_cmd(EBTABLES, 6, 1, "-A", chain_forward, "-p", "arp", "-j", "ACCEPT");
			va_cmd(EBTABLES, 10, 1, "-A", chain_forward, "-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "ACCEPT");
			va_cmd(EBTABLES, 10, 1, "-A", chain_forward, "-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "53", "-j", "ACCEPT");

			if(pEntry->allowedIPPort[0] && (rule = strdup(pEntry->allowedIPPort)))
			{
				s = rule;
				while(s = strtok_r(s, ",", &s_tmp))
				{
					if(IpPortStrToSockaddr(s, &ipport))
					{
						if(ipport.sin_family == AF_INET)
						{
							struct in_addr addr_v4 = *((struct in_addr *)&(ipport.start_addr));

							for(i=0; i<(sizeof(protocol)/sizeof(char*)); i++)
							{
								port[0] = 0;
								addr[0] = 0;
								paddr = NULL;
								memset(argv, 0, sizeof(argv));

								if(addr_v4.s_addr > 0) paddr = inet_ntoa_r(addr_v4, addr);
								if(ipport.start_port > 0) {
									pcmd = port;
									pcmd += sprintf(pcmd, "%u", ipport.start_port);
									if(ipport.end_port > ipport.start_port)
										sprintf(pcmd, ":%u", ipport.end_port);
								}
								if(port[0] == 0 && paddr == NULL) break;
								n = 0;
								argv[n++] = NULL;
								argv[n++] = "-A";
								argv[n++] = chain_input;
								argv[n++] = "-p";
								argv[n++] = "IPv4";
								if(*paddr)
								{
									argv[n++] = "--ip-dst";
									argv[n++] = paddr;
								}
								if(port[0])
								{
									argv[n++] = "--ip-proto";
									argv[n++] = protocol[i];
									argv[n++] = "--ip-dport";
									argv[n++] = port;
								}
								argv[n++] = "-j";
								argv[n++] = "ACCEPT";

								do_cmd(EBTABLES, argv, 1);
								argv[2] = chain_forward;
								do_cmd(EBTABLES, argv, 1);
							}
						}
#ifdef CONFIG_IPV6
						else if(ipport.sin_family == AF_INET6)
						{
							struct in6_addr addr_v6 = *((struct in6_addr *)&(ipport.start_addr));
							char ipv6_str[INET6_ADDRSTRLEN]={0};

							for(i=0; i<(sizeof(protocol)/sizeof(char*)); i++)
							{
								port[0] = 0;
								addr[0] = 0;
								paddr = NULL;
								memset(argv, 0, sizeof(argv));

								memset(ipv6_str, '\0', sizeof(ipv6_str));
								if(inet_ntop(AF_INET6, &(addr_v6), ipv6_str, INET6_ADDRSTRLEN)==NULL)
									break;

								paddr = ipv6_str;
								if(ipport.start_port > 0) {
									pcmd = port;
									pcmd += sprintf(pcmd, "%u", ipport.start_port);
									if(ipport.end_port > ipport.start_port)
										sprintf(pcmd, ":%u", ipport.end_port);
								}
								if(port[0] == 0 /*&& paddr == NULL*/) break;
								n = 0;
								argv[n++] = NULL;
								argv[n++] = "-A";
								argv[n++] = chain_input;
								argv[n++] = "-p";
								argv[n++] = "IPv6";
								if(*paddr)
								{
									argv[n++] = "--ip6-dst";
									argv[n++] = paddr;
								}
								if(port[0])
								{
									argv[n++] = "--ip6-proto";
									argv[n++] = protocol[i];
									argv[n++] = "--ip6-dport";
									argv[n++] = port;
								}
								argv[n++] = "-j";
								argv[n++] = "ACCEPT";

								do_cmd(EBTABLES, argv, 1);
								argv[2] = chain_forward;
								do_cmd(EBTABLES, argv, 1);
							}
						}
#endif //CONFIG_IPV6
					}
					s = s_tmp;
				}
				free(rule);
			}

			va_cmd(EBTABLES, 6, 1, "-A", chain_input, "-p", "IPv4", "-j", "DROP");
#ifdef CONFIG_IPV6
			va_cmd(EBTABLES, 6, 1, "-A", chain_input, "-p", "IPv6", "-j", "DROP");
#endif
			va_cmd(EBTABLES, 4, 1, "-A", chain_forward, "-j", "DROP");
		}
#if defined(CONFIG_USER_RTK_FASTPASSNF_MODULE)
		snprintf(cmd_str, sizeof(cmd_str), "echo ADD %s > " PROC_FASTPASSNF_IGNORE_SAMBA_IF, ifname);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#endif
	}
#if defined(CONFIG_USER_RTK_FASTPASSNF_MODULE)
	else if (!pEntry->accessRuleEnable)
	{
		snprintf(cmd_str, sizeof(cmd_str), "echo DEL %s > " PROC_FASTPASSNF_IGNORE_SAMBA_IF, ifname);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
	}
#endif
}
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU)
typedef struct macfilter_t{
	unsigned char mac[MAC_ADDR_LEN];
	unsigned char tail;
	struct macfilter_t *next;
}macfilter_s;

static int findMacFilterEntry(char *mac, macfilter_s *list)
{
	macfilter_s *t = list;
	while(t && t->tail == 0){
		if(!memcmp(mac, t->mac, MAC_ADDR_LEN))
			return 1;
		t = t->next;
	}
	return 0;
}

static void freeMacFilterList(macfilter_s *list)
{
	macfilter_s *t = list, *tmp;
	while(t && t->tail == 0){
		tmp = t;
		t = t->next;
		free(tmp);
	}
}

#define PROC_WLAN_MAC_ADDR_ACL PROC_WLAN_DIR_NAME"/%s/macaddr_acl"

int setup_wlan_MAC_ACL(int wlan_index, int vwlan_index)
{
	char parm[64], cmd_str[256], ifname[IFNAMSIZ];
#if defined(SUPPORT_ACCESS_RIGHT)
	MIB_LAN_HOST_ACCESS_RIGHT_T hostEntry;
#endif
	MIB_CE_MBSSIB_T wlEntry;
	MIB_CE_ROUTEMAC_T macEntry;
	macfilter_s whiteList, blackList, hostBlockList;
	macfilter_s *pwhiteList, *pblackList, *phostBlockList, *tmpList;
	int totalNum, index, i, j;
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	unsigned char macFilterEnable = 0;
	mib_get_s(MIB_MAC_FILTER_SRC_ENABLE, &macFilterEnable, sizeof(macFilterEnable));
#endif

	pwhiteList = &whiteList; whiteList.tail = 1;
	pblackList = &blackList; blackList.tail = 1;
	phostBlockList = &hostBlockList; hostBlockList.tail = 1;

#if defined(SUPPORT_ACCESS_RIGHT)
	totalNum = mib_chain_total(MIB_LAN_HOST_ACCESS_RIGHT_TBL);
	for(i=0; i<totalNum; i++)
	{
		if(mib_chain_get(MIB_LAN_HOST_ACCESS_RIGHT_TBL,i,&hostEntry) &&
			hostEntry.internetAccessRight == INTERNET_ACCESS_DENY)
		{
			tmpList = calloc(1, sizeof(macfilter_s));
			if(tmpList)
			{
				memcpy(tmpList->mac, hostEntry.mac, MAC_ADDR_LEN);
				tmpList->next = phostBlockList;
				phostBlockList = tmpList;
			}
		}
	}
#endif

	totalNum = mib_chain_total(MIB_MAC_FILTER_ROUTER_TBL);
	for(i=0; i<totalNum; i++)
	{
		if(mib_chain_get(MIB_MAC_FILTER_ROUTER_TBL, i, &macEntry))
		{
#ifdef MAC_FILTER_BLOCKTIMES_SUPPORT
			if(macEntry.enable)
#endif
			{
				tmpList = calloc(1, sizeof(macfilter_s));
				if(tmpList)
				{
					convertMacFormat(macEntry.mac, tmpList->mac);
					if(macEntry.mode){
						tmpList->next = pwhiteList;
						pwhiteList = tmpList;
					}
					else{
						tmpList->next = pblackList;
						pblackList = tmpList;
					}
				}
			}
		}
	}

	for(i=0; i<NUM_WLAN_INTERFACE; i++)
	{
		for (j=0; j<WLAN_SSID_NUM; j++)
		{
			if (!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&wlEntry)) {
				printf("Error! Get MIB_MBSSIB_TBL error.\n");
			}
			else if(!wlEntry.wlanDisabled)
			{
				if(wlEntry.is_ax_support==1)
					if((wlan_index >=0 && vwlan_index >=0) && !(wlan_index==i && vwlan_index==j))
						continue;

				rtk_wlan_get_ifname(i, j, ifname);

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
				if(wlEntry.macAccessMode == 1) //Black List
#elif defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
				if(macFilterEnable == 1) //Black List
#endif
				{
					if(wlEntry.is_ax_support==0)
						va_cmd(IWPRIV, 2, 1, ifname, "clear_acl_table");
					//va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "accept_acl", "CLEAR");
					//va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "deny_acl", "CLEAR");
					else{
						snprintf(cmd_str, sizeof(cmd_str), "echo 1 clr > " PROC_WLAN_MAC_ADDR_ACL, ifname);
						va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					}
					//printf("====> iwpriv %s clear_acl_table\n", ifname);
					if(wlEntry.is_ax_support==0){
						snprintf(parm, sizeof(parm), "aclmode=2");
						va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
					}
					//va_cmd(HOSTAPD_CLI, 5, 1, "-i", ifname, "set", "macaddr_acl", "0");
					else{
						snprintf(cmd_str, sizeof(cmd_str), "echo 1 mode 1 > " PROC_WLAN_MAC_ADDR_ACL, ifname);
						va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					}
					//printf("====> iwpriv %s set_mib %s\n", ifname, parm);

					tmpList = pblackList;
					while(tmpList && tmpList->tail == 0)
					{
						if(wlEntry.is_ax_support==0){
							snprintf(parm, sizeof(parm), "%.2x%.2x%.2x%.2x%.2x%.2x",
								tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
								tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
							va_cmd(IWPRIV, 3, 1, ifname, "add_acl_table", parm);
						}
						else{
							snprintf(parm, sizeof(parm), "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
								tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
								tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
							//va_cmd(HOSTAPD_CLI, 5, 1, "-i", ifname, "deny_acl", "ADD_MAC", parm);
							snprintf(cmd_str, sizeof(cmd_str), "echo 1 add %s > " PROC_WLAN_MAC_ADDR_ACL, parm, ifname);
							va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
							va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "disassociate", parm);
						}
						//printf("====> iwpriv %s add_acl_table %s\n", ifname, parm);
						tmpList = tmpList->next;
					}

					tmpList = phostBlockList;
					while(tmpList && tmpList->tail == 0)
					{
						if(!findMacFilterEntry(tmpList->mac, pblackList))
						{
							if(wlEntry.is_ax_support==0){
								snprintf(parm, sizeof(parm), "%.2x%.2x%.2x%.2x%.2x%.2x",
									tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
									tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
								va_cmd(IWPRIV, 3, 1, ifname, "add_acl_table", parm);
							}
							else{
								snprintf(parm, sizeof(parm), "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
									tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
									tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
								//va_cmd(HOSTAPD_CLI, 5, 1, "-i", ifname, "deny_acl", "ADD_MAC", parm);
								snprintf(cmd_str, sizeof(cmd_str), "echo 1 add %s > " PROC_WLAN_MAC_ADDR_ACL, parm, ifname);
								va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
								va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "disassociate", parm);
							}
							//printf("====> iwpriv %s add_acl_table %s\n", ifname, parm);
						}
						tmpList = tmpList->next;
					}
				}
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
				else if(wlEntry.macAccessMode == 2 && pwhiteList->tail == 0) //White List
#elif defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
				else if(macFilterEnable == 2 && pwhiteList->tail == 0) //White List
#endif
				{
					if(wlEntry.is_ax_support==0)
						va_cmd(IWPRIV, 2, 1, ifname, "clear_acl_table");
					//va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "accept_acl", "CLEAR");
					//va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "deny_acl", "CLEAR");
					else{
						snprintf(cmd_str, sizeof(cmd_str), "echo 1 clr > " PROC_WLAN_MAC_ADDR_ACL, ifname);
						va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					}
					//printf("====> iwpriv %s clear_acl_table\n", ifname);
					if(wlEntry.is_ax_support==0){
						snprintf(parm, sizeof(parm), "aclmode=1");
						va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
					}
					//va_cmd(HOSTAPD_CLI, 5, 1, "-i", ifname, "set", "macaddr_acl", "1");
					else{
						snprintf(cmd_str, sizeof(cmd_str), "echo 1 mode 2 > " PROC_WLAN_MAC_ADDR_ACL, ifname);
						va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					}
					//printf("====> iwpriv %s set_mib %s\n", ifname, parm);

					tmpList = pwhiteList;
					while(tmpList && tmpList->tail == 0)
					{
						if(!findMacFilterEntry(tmpList->mac, phostBlockList))
						{
							if(wlEntry.is_ax_support==0){
								snprintf(parm, sizeof(parm), "%.2x%.2x%.2x%.2x%.2x%.2x",
									tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
									tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
								va_cmd(IWPRIV, 3, 1, ifname, "add_acl_table", parm);
							}
							else{
								snprintf(parm, sizeof(parm), "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
									tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
									tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
								//va_cmd(HOSTAPD_CLI, 5, 1, "-i", ifname, "accept_acl", "ADD_MAC", parm);
								snprintf(cmd_str, sizeof(cmd_str), "echo 1 add %s > " PROC_WLAN_MAC_ADDR_ACL, parm, ifname);
								va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
							}
							//printf("====> iwpriv %s add_acl_table %s\n", ifname, parm);
						}
						tmpList = tmpList->next;
					}
					//force disconnect STA when the white list of MAC filter are configured
					WLAN_STA_INFO_Tp wlan_sta_buf = calloc(1, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));
					if(wlan_sta_buf)
					{
						if(getWlStaInfo(ifname, wlan_sta_buf) == 0 )
						{
							int i, found_sta=0;
							WLAN_STA_INFO_Tp pInfo;
							for (i=1; i<=MAX_STA_NUM; i++)
							{
								pInfo = &wlan_sta_buf[i];
								if (pInfo->aid && (pInfo->flags & STA_INFO_FLAG_ASOC))
								{
									if(!findMacFilterEntry(pInfo->addr, pwhiteList) || findMacFilterEntry(pInfo->addr, phostBlockList))
									{
										if(wlEntry.is_ax_support==0){
											snprintf(parm, sizeof(parm), "%02x%02x%02x%02x%02x%02x",
												pInfo->addr[0], pInfo->addr[1], pInfo->addr[2],
												pInfo->addr[3], pInfo->addr[4], pInfo->addr[5]);
											va_cmd(IWPRIV, 3, 1, ifname, "del_sta", parm);
										}
										else{
											snprintf(parm, sizeof(parm), "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
												pInfo->addr[0], pInfo->addr[1], pInfo->addr[2],
												pInfo->addr[3], pInfo->addr[4], pInfo->addr[5]);
											va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "disassociate", parm);
										}
										//printf("====> iwpriv %s del_sta %s\n", ifname, parm);
									}
								}
							}
						}
						free(wlan_sta_buf);
					}
				}
				else
				{
					if(wlEntry.is_ax_support==0)
						va_cmd(IWPRIV, 2, 1, ifname, "clear_acl_table");
					//va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "accept_acl", "CLEAR");
					//va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "deny_acl", "CLEAR");
					else{
						snprintf(cmd_str, sizeof(cmd_str), "echo 1 clr > " PROC_WLAN_MAC_ADDR_ACL, ifname);
						va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					}
					//printf("====> iwpriv %s clear_acl_table\n", ifname);
					if(wlEntry.is_ax_support==0){
						snprintf(parm, sizeof(parm), "aclmode=%d", (phostBlockList->tail) ? 0 : 2);
						va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
					}
					//va_cmd(HOSTAPD_CLI, 5, 1, "-i", ifname, "set", "macaddr_acl", "0");
					else{
						snprintf(cmd_str, sizeof(cmd_str), "echo 1 mode %d > " PROC_WLAN_MAC_ADDR_ACL, (phostBlockList->tail)? 0:1, ifname);
						va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					}
					//printf("====> iwpriv %s set_mib %s\n", ifname, parm);

					tmpList = phostBlockList;
					while(tmpList && tmpList->tail == 0)
					{
						if(wlEntry.is_ax_support==0){
							snprintf(parm, sizeof(parm), "%.2x%.2x%.2x%.2x%.2x%.2x",
								tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
								tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
							va_cmd(IWPRIV, 3, 1, ifname, "add_acl_table", parm);
						}
						else{
							snprintf(parm, sizeof(parm), "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
								tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
								tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
							//va_cmd(HOSTAPD_CLI, 5, 1, "-i", ifname, "deny_acl", "ADD_MAC", parm);
							snprintf(cmd_str, sizeof(cmd_str), "echo 1 add %s > " PROC_WLAN_MAC_ADDR_ACL, parm, ifname);
							va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
							va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "disassociate", parm);
						}
						//printf("====> iwpriv %s add_acl_table %s\n", ifname, parm);
						tmpList = tmpList->next;
					}
				}
			}
		}
	}

	freeMacFilterList(pwhiteList);
	freeMacFilterList(pblackList);
	freeMacFilterList(phostBlockList);

	return 1;
}

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
int get_wlan_MAC_ACL_BlockTimes(const unsigned char *mac)
{
	int i, j;
	MIB_CE_MBSSIB_T wlEntry;
	char ifname[IFNAMSIZ], line[128] = {0}, file_block[64], *tmp;
	unsigned int blocktimes = 0, n;
	FILE *fp = NULL;

	for(i=0; i<NUM_WLAN_INTERFACE; i++)
	{
		for (j=0; j<WLAN_SSID_NUM; j++)
		{
			if (mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&wlEntry)
				&& !wlEntry.wlanDisabled
				&& wlEntry.macAccessMode == 1 )
			{
				rtk_wlan_get_ifname(i, j, ifname);

				snprintf(file_block, sizeof(file_block), "/tmp/%s_deny_sta_%02x%02x%02x%02x%02x%02x", ifname, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				if(wlEntry.is_ax_support == 1)
					snprintf(line, sizeof(line), "cat "PROC_WLAN_DIR_NAME"/%s/macaddr_acl | grep %02x:%02x:%02x:%02x:%02x:%02x > %s",
								ifname, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], file_block);
				else
					snprintf(line, sizeof(line), "cat" LINK_FILE_NAME_FMT "| grep %02x%02x%02x%02x%02x%02x > %s",
								ifname, "mib_staconfig", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], file_block);
				system(line);

				if((fp = fopen(file_block, "r")))
				{
					n = 0;
					line[0] = '\0';
					fgets(line, sizeof(line)-1, fp);
					tmp = strstr(line, "BlockTimes:");
					if(tmp){
						tmp+=11;
						n = strtoul(tmp, NULL, 0);
					}
					blocktimes += n;
					fclose(fp);
					unlink(file_block);
				}
			}
		}
	}

	return blocktimes;
}
#endif

/*----------------------------------------------------------------------------
 * Name:
 *      check_wlan_encrypt
 * Descriptions:
 *      check wlan encryption settings
 * Input:
 * Entry: MIB_CE_MBSSIB_Tp with new encryption settings
 * return:
 * 0: ok
 * -1: something is invalid, may cause wlan setup error
 *---------------------------------------------------------------------------*/
int check_wlan_encrypt(MIB_CE_MBSSIB_Tp Entry)
{
	int len, keyLen;
	int i;
	char tmpBuf[100];

	if (Entry->encrypt == WIFI_SEC_NONE || Entry->encrypt == WIFI_SEC_WEP) {

		if (Entry->encrypt == WIFI_SEC_WEP) {
			if(Entry->enable1X != 1){
				// Mason Yu. 201009_new_security. If wireless do not use 802.1x for wep mode. We should set wep key and Authentication type.
				// (1) Authentication Type
				if(Entry->authType < AUTH_OPEN || Entry->authType > AUTH_BOTH)
					goto setErr_encrypt;

				// (2) Key Length
				if(Entry->wep < WEP64 || Entry->wep > WEP128)
					goto setErr_encrypt;

				// (3) Key Format
				if(Entry->wepKeyType < KEY_ASCII  || Entry->wepKeyType > KEY_HEX)
					goto setErr_encrypt;

				if (Entry->wep == WEP64) {

					keyLen = WEP64_KEY_LEN;

					if(strlen(Entry->wep64Key1) != keyLen)
						goto setErr_encrypt;
				}
				else {

					keyLen = WEP128_KEY_LEN;

					if(strlen(Entry->wep128Key1) != keyLen)
						goto setErr_encrypt;
				}
			}
			else if(Entry->enable1X){
				if(Entry->wep < WEP64 || Entry->wep > WEP128)
					goto setErr_encrypt;
			}
		}
	}
	else if(Entry->encrypt == WIFI_SEC_WPA || Entry->encrypt == WIFI_SEC_WPA2 || Entry->encrypt == WIFI_SEC_WPA2_MIXED // WPA
#ifdef WLAN_WPA3
			|| Entry->encrypt == WIFI_SEC_WPA3 || Entry->encrypt == WIFI_SEC_WPA2_WPA3_MIXED
#endif
		) {

		// Mason Yu. 201009_new_security. Set ciphersuite(wpa_cipher) for wpa/wpa mixed
		if ((Entry->encrypt == WIFI_SEC_WPA) || (Entry->encrypt == WIFI_SEC_WPA2_MIXED)) {

			if(Entry->unicastCipher < WPA_CIPHER_TKIP || Entry->unicastCipher > WPA_CIPHER_MIXED)
					goto setErr_encrypt;

		}

		// Mason Yu. 201009_new_security. Set wpa2ciphersuite(wpa2_cipher) for wpa2/wpa mixed
		if ((Entry->encrypt == WIFI_SEC_WPA2) || (Entry->encrypt == WIFI_SEC_WPA2_MIXED)) {

			if(Entry->wpa2UnicastCipher < WPA_CIPHER_TKIP || Entry->wpa2UnicastCipher > WPA_CIPHER_MIXED)
					goto setErr_encrypt;

		}
#if 0 //def WLAN_WPA3
		if((Entry->encrypt == WIFI_SEC_WPA3) || (Entry->encrypt == WIFI_SEC_WPA2_WPA3_MIXED))
			if((Entry->wpa2UnicastCipher != WPA_CIPHER_AES) || (Entry->unicastCipher != 0))
				goto setErr_encrypt;
#endif
		// pre-shared key
		if ( Entry->wpaAuth == WPA_AUTH_PSK ) {

			if(Entry->wpaPSKFormat < KEY_ASCII || Entry->wpaPSKFormat > KEY_HEX)
				goto setErr_encrypt;

			len = strlen(Entry->wpaPSK);

			if (Entry->wpaPSKFormat == KEY_HEX) { // hex
				if (len!=MAX_PSK_LEN || !rtk_string_to_hex(Entry->wpaPSK, tmpBuf, MAX_PSK_LEN)) {
					goto setErr_encrypt;
				}
			}
			else { // passphras
				if (len==0 || len > (MAX_PSK_LEN - 1) ) {
					goto setErr_encrypt;
				}
			}
		}
		else if( Entry->wpaAuth == WPA_AUTH_AUTO){
			//need check ?
		}
	}
	else
		goto setErr_encrypt;

set_OK:
	return 0;

setErr_encrypt:
	return -1;
}

#if defined(WLAN_BAND_STEERING)
int SetOrCancelSameSSID(unsigned char sta_control) //update SSID & encryption
{
	MIB_CE_MBSSIB_T Entry24G, Entry5G;
	unsigned char ssid[MAX_SSID_LEN]={0};
	int ret=0;
	unsigned char phyband;
#ifdef WLAN0_5G_WLAN1_2G
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, 0, 0, &Entry5G);
	mib_chain_local_mapping_get(MIB_WLAN1_MBSSIB_TBL, 0, 0, &Entry24G);
#else
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, 0, 0, &Entry24G);
	mib_chain_local_mapping_get(MIB_WLAN1_MBSSIB_TBL, 0, 0, &Entry5G);
#endif
	if(sta_control){
		strcpy(Entry5G.ssid, Entry24G.ssid);
		Entry5G.encrypt=Entry24G.encrypt;
		if(Entry24G.encrypt == WIFI_SEC_WEP){
			Entry5G.wep = Entry24G.wep;
			Entry5G.wepKeyType = Entry24G.wepKeyType;
			Entry5G.authType = Entry24G.authType;
			if(Entry24G.wep==WEP64){
				strncpy(Entry5G.wep64Key1, Entry24G.wep64Key1, WEP64_KEY_LEN);
			}
			else if(Entry24G.wep==WEP128){
				strncpy(Entry5G.wep128Key1, Entry24G.wep128Key1, WEP128_KEY_LEN);
			}
		}
		else if(Entry24G.encrypt>= WIFI_SEC_WPA){
			Entry5G.unicastCipher = Entry24G.unicastCipher;
			Entry5G.wpa2UnicastCipher = Entry24G.wpa2UnicastCipher;
			Entry5G.wpaAuth = Entry24G.wpaAuth;
			if(Entry24G.wpaAuth==WPA_AUTH_PSK){
				Entry5G.wpaPSKFormat = Entry24G.wpaPSKFormat;
				strncpy(Entry5G.wpaPSK, Entry24G.wpaPSK, MAX_PSK_LEN+1);
			}
		}
	}
	else{
		snprintf(ssid, MAX_SSID_LEN, "%s-5G", Entry24G.ssid);
		strcpy(Entry5G.ssid, ssid);
	}
#ifdef WLAN0_5G_WLAN1_2G
	ret = mib_chain_local_mapping_update(MIB_MBSSIB_TBL, 0, &Entry5G, 0);
#else
	ret = mib_chain_local_mapping_update(MIB_WLAN1_MBSSIB_TBL, 0, &Entry5G, 0);
#endif
	return ret;
}
void setSameSSID(unsigned int ssidindex24, unsigned int ssidindex58, unsigned int *result, char *err)
{
	unsigned char wlan_sta_control;
	if(!(ssidindex24==1 && ssidindex58==(WLAN_SSID_NUM+1)))
	{
		*result = 1; //FAILED
		sprintf(err, "only support with root interface SSID index!");
		return;
	}
	if(	mib_get_s(MIB_WIFI_STA_CONTROL, (void *)&wlan_sta_control, sizeof(wlan_sta_control))==0)
	{
		*result = 1; //FAILED
		sprintf(err, "get SameSSIDStatus failed!");
		return;
	}
	if(wlan_sta_control == 0){
		wlan_sta_control = 1;
		if(mib_set(MIB_WIFI_STA_CONTROL, (void *)&wlan_sta_control)==0){
			*result = 1; //FAILED
			sprintf(err, "set SameSSIDStatus failed!");
			return;
		}
		SetOrCancelSameSSID(wlan_sta_control);
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
		update_wps_configured(0);
#endif
		config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
		*result = 0; //SUCCESS
		sprintf(err, "success!");
		return;
	}
	else{
		*result = 1; //FAILED
		sprintf(err, "SameSSIDStatus is already true!");
		return;
	}

}
void cancelSameSSID(unsigned int *result, char *err)
{
	unsigned char wlan_sta_control;
	if(	mib_get_s(MIB_WIFI_STA_CONTROL, (void *)&wlan_sta_control, sizeof(wlan_sta_control))==0)
	{
		*result = 1; //FAILED
		sprintf(err, "get SameSSIDStatus failed!");
		return;
	}
	if(wlan_sta_control == 1){
		wlan_sta_control = 0;
		if(mib_set(MIB_WIFI_STA_CONTROL, (void *)&wlan_sta_control)==0){
			*result = 1; //FAILED
			sprintf(err, "set SameSSIDStatus failed!");
			return;
		}
		SetOrCancelSameSSID(wlan_sta_control);
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
		update_wps_configured(0);
#endif
		config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
		*result = 0; //SUCCESS
		sprintf(err, "success!");
		return;
	}
	else{
		*result = 1; //FAILED
		sprintf(err, "SameSSIDStatus is already false!");
		return;
	}

}

#endif

/***
get_wlan_channel_scan_status
return:
	0 => error,
	1 => not compelete or not running,
	2 => compelete
***/
int get_wlan_channel_scan_status(int idx)
{
	char buf[256] = {0};
	FILE *fp = NULL;
	int ret = 0;
	MIB_CE_MBSSIB_T Entry;
	if(idx >= 0 && idx < (sizeof(WLANIF)/sizeof(char*))){
		//printf("%s: %s, idx = %d\n", __FUNCTION__, WLANIF[idx], idx);

		if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, idx, WLAN_ROOT_ITF_INDEX, &Entry)==0){
			return ret;
		}

		if(Entry.is_ax_support){
			snprintf(buf, sizeof(buf), PROC_WLAN_DIR_NAME"/%s/chan_info", WLANIF[idx]);
			fp = fopen(buf, "r");
			if(fp){
				if(fgets(buf, sizeof(buf)-1, fp) != NULL){
					if(strlen(buf)<=1) //empty
						ret = 1;
					else
						ret = 2;
				}
				fclose(fp);
			}
		}
		else{
			//sprintf(buf, "/proc/%s/SS_Result", WLANIF[idx]);
			snprintf(buf, sizeof(buf), LINK_FILE_NAME_FMT, WLANIF[idx], "SS_Result");
			fp = fopen(buf, "r");
			if(fp){
				if(fgets(buf, sizeof(buf)-1, fp) != NULL){
					if(!strncmp(buf, "waitting", 8)){
						ret = 1;
					}
					else if(!strncmp(buf, " SiteSurvey result", 18)){
						if(fgets(buf, sizeof(buf)-1, fp)
							&& fgets(buf, sizeof(buf)-1, fp)) // rechek line 3
						{
							/*if(!strncmp(buf, "none", 4)){
								ret = 1;
							}
							else*/
							{
								ret = 2;
							}
						}
					}
				}
				fclose(fp);
			}
		}

	}
	//printf(">> %s: ret = %d\n", __FUNCTION__, ret);
	return ret;
}

/***
start_wlan_channel_scan
return:
	0 => error,
	1 => success,
***/
int start_wlan_channel_scan(int idx)
{
	int ret = 0, skfd = -1;
	struct iwreq wrq;
	char cmd_str[256];
	MIB_CE_MBSSIB_T Entry;

	if(idx >= 0 && idx < (sizeof(WLANIF)/sizeof(char*)))
	{
		if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, idx, WLAN_ROOT_ITF_INDEX, &Entry)==0)
			return ret;

		if(Entry.is_ax_support){
			snprintf(cmd_str, sizeof(cmd_str), "echo \"acs\" > "PROC_WLAN_DIR_NAME"/%s/survey_info", WLANIF[idx]);
			if(va_cmd("/bin/sh", 2, 1, "-c", cmd_str)==0)
				ret = 1;
		}
		else{
			memset(&wrq, 0, sizeof(struct iwreq));
			skfd = socket(AF_INET, SOCK_DGRAM, 0);

			if ( iw_get_ext(skfd,(char*) WLANIF[idx], SIOCGIWNAME, &wrq) < 0)
			{
			  /* If no wireless name : no wireless extensions */
				printf("Error to get wlan(%s)\n", WLANIF[idx]);
				goto out;
			}

			wrq.u.data.pointer = NULL;
			wrq.u.data.length = 0;

			if(iw_get_ext(skfd,(char*) WLANIF[idx], SIOCSSREQ, &wrq) >= 0){
				ret = 1;
			}
			else{
				printf("Error to start channel scanning on wlan(%s)\n", WLANIF[idx]);
			}
		}
	}
out:
	//printf(">> %s: ret = %d\n", __FUNCTION__, ret);
	if (skfd != -1) close(skfd);
	return ret;
}


/***
get_wlan_channel_score
return:
	0 => error,
	1 => success,
***/
int get_wlan_channel_score(int idx, wlan_channel_info **chInfo, int *count)
{
	int ret = 0;
	int skfd,i,len,chCount = 0, channel, score;
	struct iwreq wrq;
	char buf[256] = {0}, *s1, *s2, *tmp_s;
	FILE *fp=NULL;
	MIB_CE_MBSSIB_T Entry;

	if(chInfo == NULL || count == NULL)
	{
		printf("%s: Error output argument\n", __FUNCTION__);
	}
	else if(idx >= 0 && idx < (sizeof(WLANIF)/sizeof(char*)))
	{
		//printf("%s: %s, idx = %d\n", __FUNCTION__, WLANIF[idx], idx);
		if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, idx, WLAN_ROOT_ITF_INDEX, &Entry)==0)
			return ret;

		if(get_wlan_channel_scan_status(idx) == 2)
		{
			if(Entry.is_ax_support)
			{

				sprintf(buf, PROC_WLAN_DIR_NAME"/%s/chan_info", WLANIF[idx]);
				fp = fopen(buf, "r");
				if(fp){
					if(fgets(buf, sizeof(buf)-1, fp) != NULL){
						len = strlen(buf);
						for(i = 0;i<len;i++)
							if(buf[i] == ':')
								chCount++;
					}
					fclose(fp);
				}
				if(chCount){
					*chInfo = malloc(chCount*sizeof(wlan_channel_info));
					s1 = buf;
					i = 0;
					while((s2 = strtok_r(s1, ",", &tmp_s)) && i < chCount)
					{
						if(sscanf(s2, "%d:%d", &channel, &score) == 2){
							if(score!=-1){
								((*chInfo)+i)->channel = channel;
								((*chInfo)+i)->score = score;
								i++;
							}
						}
						s1 = tmp_s;
					}
					if(i != 0){
						*count = i;
						ret = 1;
					}
					else{
						free(*chInfo);
						*chInfo = NULL;
						*count = 0;
					}
				}
			}
			else
			{
				memset(&wrq, 0, sizeof(struct iwreq));
				skfd = socket(AF_INET, SOCK_DGRAM, 0);

				if ( iw_get_ext(skfd, WLANIF[idx], SIOCGIWNAME, &wrq) < 0)
				{
					close( skfd );
				  /* If no wireless name : no wireless extensions */
					printf("Error to get wlan(%s)\n", WLANIF[idx]);
					goto out;
				}

				wrq.u.data.pointer = (caddr_t)buf;
				wrq.u.data.length = 256;

				if(iw_get_ext(skfd, WLANIF[idx], SIOCGIWRTLSSSCORE, &wrq) >= 0)
				{
					len = strlen(buf);
					for(i = 0;i<len;i++)
						if(buf[i] == ':')
							chCount++;
					if(chCount){
						*chInfo = malloc(chCount*sizeof(wlan_channel_info));
						s1 = buf;
						i = 0;
						while((s2 = strtok_r(s1, " ", &tmp_s)) && i < chCount)
						{
							if(sscanf(s2, "ch%d:%d", &channel, &score) == 2){
								((*chInfo)+i)->channel = channel;
								((*chInfo)+i)->score = 100 - score;
								i++;
							}
							s1 = tmp_s;
						}
						if(i != 0){
							*count = i;
							ret = 1;
						}
						else{
							free(*chInfo);
							*chInfo = NULL;
							*count = 0;
						}
					}
				}
				else{
					printf("Error to get wlan(%s) channel scan score\n", WLANIF[idx]);
				}
				close( skfd );
			}
		}
	}
out:
	//printf(">> %s: ret = %d\n", __FUNCTION__, ret);
	return ret;
}

/***
start_wlan_channel_scan
return:
	0 => error,
	1 => success,
***/
int switch_wlan_channel(int idx)
{
	int ret = 0;
	unsigned char vChar = 0;
	int skfd = -1;
	struct iwreq wrq;
	char cmd_str[256];
	MIB_CE_MBSSIB_T Entry;

	if(idx >= 0 && idx < (sizeof(WLANIF)/sizeof(char*)))
	{
		if(idx == 0) mib_get_s(MIB_WLAN_AUTO_CHAN_ENABLED, &vChar, sizeof(vChar));
		else mib_get_s(MIB_WLAN1_AUTO_CHAN_ENABLED, &vChar, sizeof(vChar));

		if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, idx, WLAN_ROOT_ITF_INDEX, &Entry)==0)
			return ret;

		if(vChar){
			if(Entry.is_ax_support){
				snprintf(cmd_str, sizeof(cmd_str), "echo \"switch\" > "PROC_WLAN_DIR_NAME"/%s/acs", WLANIF[idx]);
				if(va_cmd("/bin/sh", 2, 1, "-c", cmd_str)==0)
					ret = 1;
			}
			else{
				memset(&wrq, 0, sizeof(struct iwreq));
				skfd = socket(AF_INET, SOCK_DGRAM, 0);

				if ( iw_get_ext(skfd, WLANIF[idx], SIOCGIWNAME, &wrq) < 0)
				{
				  /* If no wireless name : no wireless extensions */
					printf("Error to get wlan(%s)\n", WLANIF[idx]);
					goto out;
				}

				wrq.u.data.pointer = NULL;
				wrq.u.data.length = 0;

				if(iw_get_ext(skfd, WLANIF[idx], SIOC92DAUTOCH, &wrq) >= 0){
					ret = 1;
				}
				else{
					printf("Error to start channel scanning on wlan(%s)\n", WLANIF[idx]);
				}
			}
		}
		else{
			printf("%s: %s no enable auto channel function !!!\n ", __FUNCTION__, WLANIF[idx]);
		}
	}
out:
	//printf(">> %s: ret = %d\n", __FUNCTION__, ret);
	if (skfd != -1) close(skfd);
	return ret;
}

#ifdef WLAN_ROAMING
int doWlStartAPRSSIQueryRequest(char *ifname, unsigned char * macaddr,
                                    dot11k_beacon_measurement_req* beacon_req)
{
    int sock = -1;
    struct iwreq wrq;
	int ret = -1;
    int err;
    int len = 0;
	unsigned char buf[MAC_ADDR_LEN+sizeof(dot11k_beacon_measurement_req)]={0};

    /*** Inizializzazione socket ***/
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        err = errno;
        printf("[%s %d]: Can't create socket for ioctl. %s(%d)", __FUNCTION__, __LINE__, ifname, err);
        goto out;
    }
    memcpy(buf, macaddr, MAC_ADDR_LEN);
    len += MAC_ADDR_LEN;
    memcpy(buf + len, beacon_req, sizeof(dot11k_beacon_measurement_req));
    len += sizeof(dot11k_beacon_measurement_req);

    /*** Inizializzazione struttura iwreq ***/
    memset(&wrq, 0, sizeof(wrq));
    strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

    /*** give parameter and buffer ***/
    wrq.u.data.pointer = (caddr_t)buf;
    wrq.u.data.length = len;

    /*** ioctl ***/
    if(ioctl(sock, SIOC11KBEACONREQ, &wrq) < 0)
    {
        err = errno;
        printf("[%s %d]: %s ioctl Error.(%d)", __FUNCTION__, __LINE__, ifname, err);
        goto out;
    }
    ret = 0;

out:
    if(sock != -1)
        close(sock);
    return ret;
}

int getWlStartAPRSSIQueryResult(char *ifname, unsigned char *macaddr,
        unsigned char* measure_result, int *bss_num,
        dot11k_beacon_measurement_report *beacon_report)
{
    int sock = -1;
    struct iwreq wrq;
    int ret = -1;
    int err;
	unsigned char buf[1+1+sizeof(dot11k_beacon_measurement_report)*MAX_BEACON_REPORT]={0};
	MIB_CE_MBSSIB_T Entry;

	if(mib_chain_get_wlan(ifname,&Entry, NULL)!=0)
		return ret;

    /*** Inizializzazione socket ***/
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        err = errno;
        printf("[%s %d]: Can't create socket for ioctl. %s(%d)", __FUNCTION__, __LINE__, ifname, err);
        goto out;
    }

    /*** Inizializzazione struttura iwreq ***/
    memset(&wrq, 0, sizeof(wrq));
    strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

    memcpy(buf, macaddr, MAC_ADDR_LEN);
    /*** give parameter and buffer ***/
    wrq.u.data.pointer = (caddr_t)buf;
    wrq.u.data.length = MAC_ADDR_LEN;

	if(Entry.is_ax_support){
		wrq.u.data.flags = SIOC11KBEACONREP;

	    /*** ioctl ***/
	    if(ioctl(sock, SIOCDEVPRIVATE+2, &wrq) < 0)
	    {
	        err = errno;
	        printf("[%s %d]: ioctl Error.%s(%d)", __FUNCTION__, __LINE__, ifname, err);
	        goto out;
	    }

	    ret = 0;
	    *measure_result = *(unsigned char *)wrq.u.data.pointer;
	    if(*measure_result == 0)
	    {
	        *bss_num = *((unsigned char *)wrq.u.data.pointer + 1);
	        if((*bss_num > 0) && (*bss_num <= MAX_BEACON_REPORT))
	        {
				//memcpy(beacon_report, (unsigned char *)wrq.u.data.pointer + 2, wrq.u.data.length - 2);
				memcpy(beacon_report, (unsigned char *)wrq.u.data.pointer + 2, (*bss_num) * sizeof(dot11k_beacon_measurement_report));
	        }
	    }
	}
	else{
		/*** ioctl ***/
	    if(ioctl(sock, SIOC11KBEACONREP, &wrq) < 0)
	    {
	        err = errno;
	        printf("[%s %d]: ioctl Error.%s(%d)", __FUNCTION__, __LINE__, ifname, err);
	        goto out;
	    }

	    ret = 0;
	    *measure_result = *(unsigned char *)wrq.u.data.pointer;
	    if(*measure_result == MEASUREMENT_SUCCEED)
	    {
	        *bss_num = *((unsigned char *)wrq.u.data.pointer + 1);
	        if(*bss_num)
	        {
				memcpy(beacon_report, (unsigned char *)wrq.u.data.pointer + 2, wrq.u.data.length - 2);
	        }
	    }
	}
out:
    if(sock != -1)
        close(sock);
    return ret;

}
int getWlStartSTABSSTransitionRequest(char *interface, unsigned char *mac_sta, unsigned int channel, unsigned char *bssid)
{
	int skfd;
	struct iwreq wrq;
	int ret=0;
	unsigned char buf[MAC_ADDR_LEN+1+MAC_ADDR_LEN]={0};
	unsigned char chan = channel;
	MIB_CE_MBSSIB_T Entry;
	int cmd_id=0;

	if(mib_chain_get_wlan(interface,&Entry, NULL)!=0)
		return -1;

	memcpy(buf, mac_sta, MAC_ADDR_LEN);
	memcpy(buf+MAC_ADDR_LEN, &chan, 1);
	memcpy(buf+MAC_ADDR_LEN+1, bssid, MAC_ADDR_LEN);

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	wrq.u.data.pointer = (caddr_t)buf;
	wrq.u.data.length = sizeof(buf);

	if(Entry.is_ax_support){
		wrq.u.data.flags = SIOCGIROAMINGBSSTRANSREQ;
		cmd_id = SIOCDEVPRIVATE+2;
	}
	else{
		cmd_id = SIOCGIROAMINGBSSTRANSREQ;
	}
	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0) {
		close( skfd );
		return -1;
	}
	close( skfd );

	//printf("%s %d ret %d\n",__func__, __LINE__ ,buf[0]);
	//if(buf[0]==0)
	//	return -1;
	return 0;
}
#endif
#endif

#if defined(WLAN_ROAMING) || defined(WLAN_STA_RSSI_DIAG)
int getWlSTARSSIScanInterval( char *interface, unsigned int *num)
{
	int skfd, cmd_id, vwlanidx=0;
	struct iwreq wrq;
	unsigned char buffer[32]={0};
	MIB_CE_MBSSIB_T WlanEnable_Entry;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
	{
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(&WlanEnable_Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan(interface,&WlanEnable_Entry,&vwlanidx)!=0)
		return -1;

	strcpy(buffer,"scan_interval");
	wrq.u.data.pointer = (caddr_t)buffer;

	if(WlanEnable_Entry.is_ax_support)
	{
		wrq.u.data.flags = RTL8192CD_IOCTL_GET_MIB;
		cmd_id = SIOCDEVPRIVATEAXEXT;
	}
	else
	{
		cmd_id = RTL8192CD_IOCTL_GET_MIB;
	}

	wrq.u.data.length = strlen("scan_interval");

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0)
	{
		close( skfd );
		return -1;
	}

	*num = *(unsigned int*)buffer;
	close( skfd );

	return 0;
}

static int _doWlSTARSSIQueryRequest_wifi5(char *interface, char *mac, unsigned int channel)
{
	int skfd;
	struct iwreq wrq;
	unsigned char buf[MAC_ADDR_LEN+1];
	char fake_mac[MAC_ADDR_LEN]={0xff,0xff,0xff,0xff,0xff,0xff};

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	buf[0] = (unsigned char)channel;
	if(mac)
		memcpy(&buf[1], mac, MAC_ADDR_LEN);
	else
		memcpy(&buf[1], fake_mac, MAC_ADDR_LEN);

	wrq.u.data.pointer = (caddr_t)&buf[0];
	wrq.u.data.length = sizeof(buf);

	if (iw_get_ext(skfd, interface, RTK_IOCTL_SCANSTA, &wrq) < 0) {
		close( skfd );
		return -1;
	}
#if 0
	if (iw_get_ext(skfd, interface, RTK_IOCTL_STARTPROBE, &wrq) < 0) {
		close( skfd );
		return -1;
	}
#endif
	close( skfd );

	return 0;
}


#if defined(WLAN_ROAMING)
static int _doWlSTARSSIQueryAddSta(char *interface, char *mac)
{
	int skfd;
	struct iwreq wrq;
	struct ifreq ifr;
	union iwreq_data u;
	char buffer[128];
	int ret=0;

	memset(buffer, '\0', sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "diag_start mode=3,addr=%02x:%02x:%02x:%02x:%02x:%02x",
		(unsigned char)mac[0], (unsigned char)mac[1], (unsigned char)mac[2], (unsigned char)mac[3], (unsigned char)mac[4], (unsigned char)mac[5]);

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(&u, 0, sizeof(union iwreq_data));
	u.data.pointer = buffer;
	u.data.length = (unsigned short)strlen(buffer)+1; //?

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_ifrn.ifrn_name, interface, strlen(interface));
	ifr.ifr_ifru.ifru_data = &u;

	ret = iw_get_ext(skfd, interface, SIOCDEVPRIVATE, (struct iwreq *)&ifr);

	if(ret < 0){
		close( skfd );
		return -1;
	}

	close( skfd );
	return 0;
}

int doWlSTARSSIQueryRequest(char *interface, char *mac, unsigned int channel)
{
	int skfd;
	struct iwreq wrq;
	struct ifreq ifr;
	union iwreq_data u;
	char buffer[128];
	int ret=0;
	MIB_CE_MBSSIB_T Entry;

	if(mib_chain_get_wlan(interface,&Entry, NULL)!=0)
		return -1;

	if(Entry.is_ax_support){

		if(_doWlSTARSSIQueryAddSta(interface, mac)<0)
			return -1;

		memset(buffer, '\0', sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "diag_start mode=2,ch=%u", channel);

		skfd = socket(AF_INET, SOCK_DGRAM, 0);
		if(skfd==-1)
			return -1;

		/* Get wireless name */
		if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
			close( skfd );
			/* If no wireless name : no wireless extensions */
			return -1;
		}

		memset(&u, 0, sizeof(union iwreq_data));
		u.data.pointer = buffer;
		u.data.length = (unsigned short)strlen(buffer)+1; //?

		memset(&ifr, 0, sizeof(struct ifreq));
		strncpy(ifr.ifr_ifrn.ifrn_name, interface, strlen(interface));
		ifr.ifr_ifru.ifru_data = &u;

		ret = iw_get_ext(skfd, interface, SIOCDEVPRIVATE, (struct iwreq *)&ifr);

		if(ret < 0){
			close( skfd );
			return -1;
		}

		close( skfd );
		return 0;
	}
	else
		return _doWlSTARSSIQueryRequest_wifi5(interface, mac, channel);

}
#else
int doWlSTARSSIQueryRequest(char *interface, char *mac, unsigned int channel)
{

	int skfd;
	struct iwreq wrq;
	struct ifreq ifr;
	union iwreq_data u;
	char buffer[] = "diag_start mode=1";
	int ret=0;
	MIB_CE_MBSSIB_T Entry;

	if(mib_chain_get_wlan(interface,&Entry, NULL)!=0)
		return -1;

	if(Entry.is_ax_support){

		skfd = socket(AF_INET, SOCK_DGRAM, 0);
		if(skfd==-1)
			return -1;

		/* Get wireless name */
		if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
			close( skfd );
			/* If no wireless name : no wireless extensions */
			return -1;
		}

		memset(&u, 0, sizeof(union iwreq_data));
		u.data.pointer = buffer;
		u.data.length = (unsigned short)sizeof(buffer); //?

		memset(&ifr, 0, sizeof(struct ifreq));
		strncpy(ifr.ifr_ifrn.ifrn_name, interface, strlen(interface));
		ifr.ifr_ifru.ifru_data = &u;

		ret = iw_get_ext(skfd, interface, SIOCDEVPRIVATE, (struct iwreq *)&ifr);

		if(ret < 0){
			close( skfd );
			return -1;
		}

		close( skfd );
		return 0;
	}
	else
		return _doWlSTARSSIQueryRequest_wifi5(interface, mac, channel);

}
#endif

static int _getWlSTARSSIDETAILQueryResult_ax(char *interface, void *sta_mac_rssi_info, unsigned char type)
{
	int skfd;
	struct iwreq wrq;
	struct ifreq ifr;
	union iwreq_data u;
	unsigned char buf[sizeof(sta_mac_rssi_detail_ax)*MAX_UNASSOC_STA]={0};
	int ret=0;
	int i=0;
	sta_mac_rssi_detail_ax *pResult_ax = (sta_mac_rssi_detail_ax *)buf;
	sta_mac_rssi_detail *pResult = (sta_mac_rssi_detail *)sta_mac_rssi_info;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	strcpy((char*)buf, "diag_result mode=1");

	memset(&u, 0, sizeof(union iwreq_data));
	u.data.pointer = buf;
	u.data.length = sizeof(buf);

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_ifrn.ifrn_name, interface, strlen(interface));
	ifr.ifr_ifru.ifru_data = &u;

	ret = iw_get_ext(skfd, interface, SIOCDEVPRIVATE, (struct iwreq *)&ifr);

	if(ret < 0){
		close( skfd );
		return -1;
	}

	close( skfd );

	for(i=0; i<MAX_UNASSOC_STA; i++){
		pResult->channel = pResult_ax->channel;
		memcpy(pResult->addr, pResult_ax->addr, MAC_ADDR_LEN);
		pResult->rssi = pResult_ax->rssi-100;
		memcpy(pResult->bssid, pResult_ax->bssid, MAC_ADDR_LEN);
		memcpy(pResult->ssid, pResult_ax->ssid, 33);
		pResult->ssid_length = pResult_ax->ssid_length;
		if(pResult_ax->bss_encrypt == ENCRYP_PROTOCOL_OPENSYS)
			pResult->encrypt = 1;
		else if(pResult_ax->bss_encrypt == ENCRYP_PROTOCOL_WEP)
			pResult->encrypt = 2;
		else if(pResult_ax->bss_encrypt == ENCRYP_PROTOCOL_WPA)
			pResult->encrypt = 4;
		else if(pResult_ax->bss_encrypt == ENCRYP_PROTOCOL_RSN)
			pResult->encrypt = 8;
		pResult->used = pResult_ax->used;
		pResult->Entry = pResult_ax->entry;
		pResult->status = pResult_ax->status;
		pResult->time_stamp = pResult_ax->time_stamp;
	}

	return 0;

}

int getWlSTARSSIDETAILQueryResult(char *interface, void *sta_mac_rssi_info, unsigned char type)
{
	int skfd;
	struct iwreq wrq;
	unsigned char buf[sizeof(sta_mac_rssi_detail)*MAX_UNASSOC_STA];
	MIB_CE_MBSSIB_T Entry;

	if(mib_chain_get_wlan(interface,&Entry, NULL)!=0)
		return -1;

	if(Entry.is_ax_support){
		return _getWlSTARSSIDETAILQueryResult_ax(interface, sta_mac_rssi_info, type);
	}
	else{
		skfd = socket(AF_INET, SOCK_DGRAM, 0);

		/* Get wireless name */
		if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
			close( skfd );
			/* If no wireless name : no wireless extensions */
			return -1;
		}

		memset(buf, 0, sizeof(buf));
		buf[0] = (unsigned char)type;

		wrq.u.data.pointer = (caddr_t)buf;
		wrq.u.data.length = sizeof(buf);

		if (iw_get_ext(skfd, interface, RTK_IOCTL_GETUNASSOCSTA, &wrq) < 0) {
			fprintf(stderr, "error:%s\n", strerror(errno));
			close( skfd );
			return -1;
		}

		memset(sta_mac_rssi_info, 0, sizeof(sta_mac_rssi_info));
		memcpy(sta_mac_rssi_info, buf, sizeof(buf));
		close( skfd );
		return 0;
	}
}

int _getWlSTARSSIQueryResult_ax(char *interface, void *sta_mac_rssi_info)
{
	int skfd;
	struct iwreq wrq;
	struct ifreq ifr;
	union iwreq_data u;
	int ret=0;
	unsigned char buf[sizeof(sta_mac_rssi_ax)*MAX_PROBE_REQ_STA]={0};
	int i=0;
	sta_mac_rssi_ax *pResult_ax = (sta_mac_rssi_ax *)buf;
	sta_mac_rssi *pResult = (sta_mac_rssi *)sta_mac_rssi_info;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(pResult_ax, '\0', sizeof(sta_mac_rssi_ax)*MAX_PROBE_REQ_STA);
	strcpy((char*)pResult_ax, "diag_result mode=2");

	memset(&u, 0, sizeof(union iwreq_data));
	u.data.pointer = (caddr_t)pResult_ax;
	u.data.length = sizeof(sta_mac_rssi_ax)*MAX_PROBE_REQ_STA;

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_ifrn.ifrn_name, interface, strlen(interface));
	ifr.ifr_ifru.ifru_data = &u;

	ret = iw_get_ext(skfd, interface, SIOCDEVPRIVATE, (struct iwreq *)&ifr);

	if(ret < 0){
		close( skfd );
		return -1;
	}

	close( skfd );


	for(i=0; i<MAX_PROBE_REQ_STA; i++){
		pResult->channel = pResult_ax->channel;
		memcpy(pResult->addr, pResult_ax->addr, MAC_ADDR_LEN);
		pResult->rssi = pResult_ax->rssi-100;
		pResult->used = pResult_ax->used;
		pResult->Entry = pResult_ax->entry;
		pResult->status = pResult_ax->status;
		pResult->time_stamp = pResult_ax->time_stamp;
	}

	return 0;
}

int getWlSTARSSIQueryResult(char *interface, void *sta_mac_rssi_info)
{
	int skfd;
	//sta_mac_rssi sta_query_info[MAX_PROBE_REQ_STA];
	struct iwreq wrq;

	MIB_CE_MBSSIB_T Entry;

	if(mib_chain_get_wlan(interface,&Entry, NULL)!=0)
		return -1;

	if(Entry.is_ax_support){
		return _getWlSTARSSIQueryResult_ax(interface, sta_mac_rssi_info);
	}
	else{
		skfd = socket(AF_INET, SOCK_DGRAM, 0);

		/* Get wireless name */
		if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
			close( skfd );
			/* If no wireless name : no wireless extensions */
			return -1;
		}

		wrq.u.data.pointer = (caddr_t)sta_mac_rssi_info;
		wrq.u.data.length = sizeof(sta_mac_rssi)*MAX_PROBE_REQ_STA;

		if (iw_get_ext(skfd, interface, RTK_IOCTL_PROBEINFO, &wrq) < 0) {
			close( skfd );
			return -1;
		}

		close( skfd );

		return 0;
	}
}
int stopWlSTARSSIQueryRequest(char *interface)
{
	int skfd;
	struct iwreq wrq;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	//wrq.u.data.pointer = (caddr_t)sta_mac_info;
	//wrq.u.data.length = sizeof(sta_mac_rssi)*MAX_PROBE_REQ_STA;

	if (iw_get_ext(skfd, interface, RTK_IOCTL_STOPPROBE, &wrq) < 0) {
		close( skfd );
		return -1;
	}
#if 0
	if (iw_get_ext(skfd, interface, RTK_IOCTL_STARTPROBE, &wrq) < 0) {
		close( skfd );
		return -1;
	}
#endif
	close( skfd );

	return 0;
}
#endif
#ifdef WLAN_STA_RSSI_DIAG
int rtk_wlan_get_wifi_unassocsta(char *ifname, sta_mac_rssi_detail *sta_mac_rssi_result)
{
	int ret=0;
	unsigned int channel;
	unsigned int scan_interval=0;
	MIB_CE_MBSSIB_T Entry;

	if(mib_chain_get_wlan(ifname, &Entry, NULL)!=0)
		return -1;

	if(Entry.is_ax_support){
		scan_interval=6;
		if ( doWlSTARSSIQueryRequest(ifname,  NULL, channel) < 0 ) {
			printf("doWlSTARSSIQueryRequest failed!");
			ret=-1;
			return ret;
		}
		printf("sleeping %ds\n",scan_interval);
		sleep(scan_interval);
		if ( getWlSTARSSIDETAILQueryResult(ifname, (sta_mac_rssi_detail *)sta_mac_rssi_result, 1) < 0 ) {
			printf("getWlSTARSSIDETAILQueryResult failed!");
			ret=-1;
			return ret;
		}
	}
	else{
		if(getWlChannel(ifname, &channel)<0)
		{
			printf("getWlChannel failed!");
			goto sr_err;
		}

		if ( doWlSTARSSIQueryRequest(ifname,  NULL, channel) < 0 ) {
			printf("doWlSTARSSIQueryRequest failed!");
			ret=-1;
			goto sr_err;
		}
		getWlSTARSSIScanInterval(ifname, &scan_interval);
		scan_interval /=1000;
		printf("sleeping %ds\n",scan_interval);
		sleep(scan_interval);
		if ( getWlSTARSSIDETAILQueryResult(ifname, (sta_mac_rssi_detail *)sta_mac_rssi_result, 1) < 0 ) {
			printf("getWlSTARSSIDETAILQueryResult failed!");
			ret=-1;
			goto sr_err;
		}
sr_err:
		stopWlSTARSSIQueryRequest(ifname);
	}
	return ret;
}

int rtk_wlan_get_wifi_unasso_sta_info(char *ifname, void **sta_mac_rssi_result, int *pCount)
{
	struct stat status;
	int fd, i, read_size, pid;
	sta_mac_rssi_detail *pLANNetInfoData = NULL;
	char cmd[256];
	char filename[256];
	unsigned int scan_period = 0;

	if( (sta_mac_rssi_result==NULL) )
		return -1;

	if(mib_get(MIB_UNASSOSTA_SCAN_PERIOD, &scan_period) == 0){
		fprintf(stderr, "get unassociate station scan period fail");;
	}
	if(scan_period==0){
		pid = read_pid(LANNETINFO_RUNFILE);
		if( pid>0 ){
			sprintf(cmd, "touch %s", WIFIHOSTFILE);
			system(cmd);
			kill(pid, SIGUSR2);
		}
	}
	sprintf(filename, "%s_%s", LANNETINFO_UNASSOSTA_FILE, ifname);

	if ( stat(filename, &status) < 0 )
		return -3;

	fd = open(filename, O_RDONLY);
	if(fd < 0)
		return -3;

	read_size = (unsigned long)(status.st_size);
	*pCount = read_size / sizeof(sta_mac_rssi_detail);

	pLANNetInfoData = (sta_mac_rssi_detail *) malloc(read_size);

	if(pLANNetInfoData == NULL)
	{
		close(fd);
		return -3;
	}

	*sta_mac_rssi_result = pLANNetInfoData;
	memset(pLANNetInfoData, 0, read_size);

	if(!flock_set(fd, F_RDLCK))
	{
		read_size = read(fd, (void*)pLANNetInfoData, read_size);
		flock_set(fd, F_UNLCK);
	}

	close(fd);
	return 0;
}
#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU)
int rtk_wlan_set_bcnvsie( char *interface, char *data, int data_len )
{
	int skfd;
	struct iwreq wrq;
	MIB_CE_MBSSIB_T Entry;
	int cmd_id=0;

	if(mib_chain_get_wlan(interface, &Entry, NULL)!=0)
		return -1;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	wrq.u.data.pointer = (caddr_t)data;
	wrq.u.data.length = (unsigned short)data_len;

	if(Entry.is_ax_support){
		wrq.u.data.flags = SIOCGISETBCNVSIE;
		cmd_id = SIOCDEVPRIVATE+2;
	}
	else{
		cmd_id = SIOCGISETBCNVSIE;
	}

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0) {
		close( skfd );
		return -1;
	}

	close( skfd );
	return (int) data[0];
}
int rtk_wlan_set_prbvsie( char *interface, char *data, int data_len )
{
	int skfd;
	struct iwreq wrq;
	MIB_CE_MBSSIB_T Entry;
	int cmd_id=0;

	if(mib_chain_get_wlan(interface, &Entry, NULL)!=0)
		return -1;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	wrq.u.data.pointer = (caddr_t)data;
	wrq.u.data.length = (unsigned short)data_len;

	if(Entry.is_ax_support){
		wrq.u.data.flags = SIOCGISETPRBVSIE;
		cmd_id = SIOCDEVPRIVATE+2;
	}
	else{
		cmd_id = SIOCGISETPRBVSIE;
	}

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0) {
		close( skfd );
		return -1;
	}

	close( skfd );
	return (int) data[0];
}
int rtk_wlan_set_prbrspvsie(char *interface, char *data, int data_len)
{
	int skfd;
	struct iwreq wrq;
	MIB_CE_MBSSIB_T Entry;
	int cmd_id=0;

	if(mib_chain_get_wlan(interface, &Entry, NULL)!=0)
		return -1;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	wrq.u.data.pointer = (caddr_t)data;
	wrq.u.data.length = (unsigned short)data_len;

	if(Entry.is_ax_support){
		wrq.u.data.flags = SIOCGISETPRBRSPVSIE;
		cmd_id = SIOCDEVPRIVATE+2;
	}
	else{
		cmd_id = SIOCGISETPRBRSPVSIE;
	}

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0) {
		close( skfd );
		return -1;
	}

	close( skfd );
	return (int) data[0];
}
#endif
#ifdef WLAN_CTC_MULTI_AP
int rtk_wlan_check_ssid_is_eashmesh_backhaul(int wlanidx, int ssid_idx)
{
	int ret = 0;
	if(ssid_idx == 1)//wlan0-vap0, wlan1-vap0 is easymesh backhaul
	{
		ret = 1;
	}
	return ret;
}
#endif
#ifdef WLAN_SMARTAPP_ENABLE
static int start_smart_wlanapp(void)
{
	int status=0;
	int i=0, idx=0, j=0;
	MIB_CE_MBSSIB_T Entry;
	char *cmd_opt[32];
	int cmd_cnt = 0;
	int intfNum=0;
	char intfList[((WLAN_MAX_ITF_INDEX+1)*2)][32]={0};
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
	unsigned char no_wlan;
#endif
	char ifname[IFNAMSIZ]={0};

	cmd_opt[cmd_cnt++] = "";
	cmd_opt[cmd_cnt++] = "-w";

	for(i = 0; i<NUM_WLAN_INTERFACE; i++){
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
		if(mib_local_mapping_get(MIB_WLAN_MODULE_DISABLED, i, (void *)&no_wlan) == 1){
			if(no_wlan == 1) //disabled, do nothing
				continue;
		}
#endif

		for(j = 0; j<WLAN_SSID_NUM; j++){
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, &Entry);
			if(Entry.is_ax_support == 0){
				if(Entry.wlanDisabled==0){
					rtk_wlan_get_ifname(i, j, intfList[intfNum]);
					cmd_opt[cmd_cnt++] = intfList[intfNum];
					intfNum++;
				}
			}
		}
	}

	if(intfNum>0){
		cmd_opt[cmd_cnt] = 0;
		printf("CMD ARGS: ");
		for (idx=0; idx<cmd_cnt;idx++)
			printf("%s ", cmd_opt[idx]);
		printf("\n");
		//status |= do_nice_cmd(SMART_WLANAPP_PROG, cmd_opt, 0);
		status |= do_nice_cmd(SMART_WLANAPP_PROG, cmd_opt, 0);
	}

	return status;
}
#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#ifdef WLAN_RATE_PRIOR
#ifdef WLAN_11AX
#ifdef WIFI5_WIFI6_COMP
#define RATE_PRIOR_BAND_2G (BAND_11N)
#else
#define RATE_PRIOR_BAND_2G (BAND_11N|BAND_11AX)
#endif
#define RATE_PRIOR_BAND_5G (BAND_5G_11AC|BAND_11AX)
#else
#define RATE_PRIOR_BAND_2G (BAND_11N)
#define RATE_PRIOR_BAND_5G (BAND_5G_11AC)
#endif
static int checkWlanRateRriorSetting(int wlanIdx)
{
	int i=0;
	unsigned char channel_width = 0, coexist=0, phyband=0, rate_prior=0;
	MIB_CE_MBSSIB_T Entry;
	unsigned char change = 0;
#if !defined(WLAN_BAND_CONFIG_MBSSID)
	unsigned char wlanBand=0;
#endif
	mib_local_mapping_get(MIB_WLAN_RATE_PRIOR, wlanIdx, (void *)&rate_prior);

	if(rate_prior == 0) // do not check if rate prior is disabled.
		return 0;

	mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlanIdx, (void *)&phyband);

	//update channelwidth
	mib_local_mapping_get(MIB_WLAN_CHANNEL_WIDTH, wlanIdx,(void *)&channel_width);
	if(phyband == PHYBAND_2G){
		mib_local_mapping_get(MIB_WLAN_11N_COEXIST, wlanIdx,(void *)&coexist);
		if((channel_width!=1) || (coexist!=1)){
			if(channel_width!=1){
				channel_width = 1;
				change = 1;
				mib_local_mapping_set(MIB_WLAN_CHANNEL_WIDTH, wlanIdx, (void *)&channel_width);
			}
			if(coexist!=1){
				coexist = 1;
				change = 1;
				mib_local_mapping_set(MIB_WLAN_11N_COEXIST, wlanIdx, (void *)&coexist);
			}
		}
	}
	else if(phyband == PHYBAND_5G){
		if(channel_width!=2){
			if(channel_width!=2){
				channel_width = 2;
				change = 1;
				mib_local_mapping_set(MIB_WLAN_CHANNEL_WIDTH, wlanIdx, (void *)&channel_width);
			}
		}
	}

#ifdef WLAN_BAND_CONFIG_MBSSID
	for(i=0; i<WLAN_SSID_NUM; i++){
		if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIdx, i, (void *)&Entry))
			continue;
		if(phyband == PHYBAND_2G){
			if(Entry.wlanBand != RATE_PRIOR_BAND_2G){
				Entry.wlanBand = RATE_PRIOR_BAND_2G;
				change = 1;
				mib_chain_local_mapping_update(MIB_MBSSIB_TBL, wlanIdx, (void *)&Entry, i);
			}
		}
		else if(phyband == PHYBAND_5G){
			if(Entry.wlanBand != RATE_PRIOR_BAND_5G){
				Entry.wlanBand = RATE_PRIOR_BAND_5G;
				change = 1;
				mib_chain_local_mapping_update(MIB_MBSSIB_TBL, wlanIdx, (void *)&Entry, i);
			}
		}
	}
#else
	mib_local_mapping_get(MIB_WLAN_BAND, wlanIdx,(void *)&wlanBand);
	if(phyband == PHYBAND_2G){
		if(wlanBand != RATE_PRIOR_BAND_2G){
			wlanBand = RATE_PRIOR_BAND_2G;
			change = 1;
			mib_local_mapping_set(MIB_WLAN_BAND, wlanIdx,(void *)&wlanBand);
		}
	}
	else if(phyband == PHYBAND_5G){
		if(wlanBand != RATE_PRIOR_BAND_5G){
			wlanBand = RATE_PRIOR_BAND_5G;
			change = 1;
			mib_local_mapping_set(MIB_WLAN_BAND, wlanIdx,(void *)&wlanBand);
		}
	}
#endif
	if(change == 1)
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);

	return 1;
}
#endif
static void checkWlanSetting(int wlanIdx)
{
#ifdef WLAN_BAND_CONFIG_MBSSID
	MIB_CE_MBSSIB_T Entry;
#endif
	unsigned char vBandwidth = 0, phymode = 0, phyband = 0;
	unsigned char change = 0;

#ifdef WLAN_RATE_PRIOR
	if(checkWlanRateRriorSetting(wlanIdx)==1)
		return;
#endif

#ifdef WLAN_BAND_CONFIG_MBSSID
	if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIdx, WLAN_ROOT_ITF_INDEX, (void *)&Entry)){
		printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");
		return ;
	}

	phymode = Entry.wlanBand;
#else
	mib_local_mapping_get(MIB_WLAN_BAND, wlanIdx, (void *)&phymode);
#endif

	mib_local_mapping_get(MIB_WLAN_CHANNEL_WIDTH, wlanIdx,(void *)&vBandwidth);
	mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlanIdx, (void *)&phyband);

	if(phyband == PHYBAND_2G)
	{
		if(phymode == BAND_11B || phymode == BAND_11G || phymode == BAND_11BG )
		{
			if(vBandwidth != 0){
				vBandwidth = 0;
				mib_local_mapping_set(MIB_WLAN_CHANNEL_WIDTH, wlanIdx,(void *)&vBandwidth);
				change = 1;
			}
		}
	}
	else if(phyband == PHYBAND_5G)
	{
		if(phymode == BAND_11A)
		{
			if(vBandwidth != 0){
				vBandwidth = 0;
				mib_local_mapping_set(MIB_WLAN_CHANNEL_WIDTH, wlanIdx,(void *)&vBandwidth);
				change = 1;
			}
		}
		else if(phymode == BAND_5G_11AN || phymode == BAND_11N)
		{
			if(vBandwidth >= 2){
				vBandwidth = 0;
				mib_local_mapping_set(MIB_WLAN_CHANNEL_WIDTH, wlanIdx,(void *)&vBandwidth);
				change = 1;
			}
		}
	}
	if(change == 1)
		mib_update(CURRENT_SETTING, CONFIG_MIB_TABLE);
}
#endif
#endif

#ifdef CONFIG_YUEME
#ifdef WLAN_VSIE_SERVICE
static void setup_wlan_prbvsie(char *ifname, unsigned char phyband)
{
	int i=0, total=0;
	MIB_PROBE_RXVSIE_ENTRY_T Entry;
	MIB_PROBE_RXVSIE_ENTRY_Tp pentry = &Entry;
	char vsie_data[252]={0}; //beacon vsie data (252 bytes)
	char *data_ptr = vsie_data;
	int data_len = 0;
	unsigned char service_len = 0;

	total = mib_chain_total(MIB_PROBE_RXVSIE_TBL);
	for(i=0; i<total; i++){
		if(mib_chain_get(MIB_PROBE_RXVSIE_TBL, i, pentry) == 0)
			continue;
		if(((pentry->probeband == 0) && (phyband == PHYBAND_2G))
			|| ((pentry->probeband == 1) && (phyband == PHYBAND_5G))
			|| (pentry->probeband == 2)){

			service_len = strlen(pentry->ServiceName);
			if(service_len == 0)
				continue;

			data_ptr = vsie_data;
			data_len = 0;
			memset(vsie_data, '\0', sizeof(vsie_data));

			data_ptr[0] = 1; //1: add, 0: delete
			data_ptr++;
			data_len++;

			//copy oui
			memcpy(data_ptr, pentry->oui, 3);
			data_ptr += 3;
			data_len += 3;

			data_ptr[0] = service_len;
			data_ptr ++;
			data_len ++;

			memcpy(data_ptr, pentry->ServiceName, service_len);
			data_ptr += service_len;
			data_len += service_len;

			rtk_wlan_set_prbvsie(ifname, vsie_data, data_len);
		}
	}
}
static void setup_wlan_prbrspvsie(char *ifname, unsigned char phyband)
{
	int i=0, total=0;
	MIB_PROBERSP_TXVSIE_ENTRY_T Entry;
	MIB_PROBERSP_TXVSIE_ENTRY_Tp pentry = &Entry;
	char vsie_data[1+6+1+255]={0}; //type (1 byte), mac addr (6 bytes), beacon vsie len(1 byte), beacon vsie (252+3 bytes)
	char *data_ptr = vsie_data;
	int data_len = 0;

	total = mib_chain_total(MIB_PROBERSP_TXVSIE_TBL);
	for(i=0; i<total; i++){
		if(mib_chain_get(MIB_PROBERSP_TXVSIE_TBL, i, pentry) == 0)
			continue;
		if(((pentry->band == 0) && (phyband == PHYBAND_2G))
			|| ((pentry->band == 1) && (phyband == PHYBAND_5G))){
			if(strlen(pentry->mac)==0 || pentry->LenofIEData==0)
				continue;
			data_ptr = vsie_data;
			data_len = 0;
			memset(vsie_data, '\0', sizeof(vsie_data));

			data_ptr[0] = 1; //1: add, 0: delete
			data_ptr++;
			data_len++;

			memcpy(data_ptr, pentry->mac, MAC_ADDR_LEN);
			data_ptr += MAC_ADDR_LEN;
			data_len += MAC_ADDR_LEN;

			data_ptr[0] = (unsigned char) pentry->LenofIEData;
			data_ptr++;
			data_len++;

			memcpy(data_ptr, pentry->IEData, pentry->LenofIEData);
			data_ptr += pentry->LenofIEData;
			data_len += pentry->LenofIEData;

			rtk_wlan_set_prbrspvsie(ifname, vsie_data, data_len);
		}
	}
}
#endif

static void setupWlanVSIE(int wlan_index,unsigned char phyband)
{
#ifdef WLAN_VSIE_SERVICE
	int j=0;
	char ifname[IFNAMSIZ]={0};
	MIB_CE_MBSSIB_T Entry;
	unsigned char enable = 0;
	mib_get_s(MIB_PROBERXVSIEMGR_ENABLE, &enable, sizeof(enable));

	for(j=0; j<WLAN_SSID_NUM; j++){
		if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, j, &Entry)==0)
			continue;
		if(Entry.wlanDisabled == 0){
			rtk_wlan_get_ifname(wlan_index, j, ifname);
			if(enable)
				setup_wlan_prbvsie(ifname, phyband);
			setup_wlan_prbrspvsie(ifname, phyband);
		}
	}
#endif
}
#endif

#ifdef CONFIG_USER_LANNETINFO
int doWlKickOutDevice( char *interface,  unsigned char *mac_addr)
{
	int vwlan_idx=0;
	MIB_CE_MBSSIB_T Entry;
	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)interface,&Entry,&vwlan_idx)!=0){
		return RTK_FAILED;
	}

	if(Entry.is_ax_support){
		unsigned char mac_addr_str[18];
		char tmp_cmd[128] = {0};

		if(mac_addr == NULL || interface == NULL)
		{
			return -1;
		}

		snprintf(mac_addr_str, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
			mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
		snprintf(tmp_cmd, sizeof(tmp_cmd), "hostapd_cli -i %s disassociate %s", interface, mac_addr_str);

		system(tmp_cmd);
	}else{
		int skfd;
		struct iwreq wrq;

		skfd = socket(AF_INET, SOCK_DGRAM, 0);

		/* Get wireless name */
		if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
		{
			close( skfd );
			/* If no wireless name : no wireless extensions */
			return -1;
		}

		wrq.u.data.pointer = (caddr_t)mac_addr;
		wrq.u.data.length = 12;

		if (iw_get_ext(skfd, interface, RTL8192CD_IOCTL_DEL_STA, &wrq) < 0)
		{
			close( skfd );
			return -1;
		}
		close( skfd );
	}
	return 0;
}

void kickOutDevice(unsigned char *mac_addr, int ssid_idx, unsigned int *result, char *error)
{
	char interface_name[16];
	unsigned char mac_addr_str[13];
	WLAN_STA_INFO_T buff[MAX_STA_NUM + 1]={0};
	WLAN_STA_INFO_Tp pInfo;
	int i, found=0;

	rtk_wlan_get_ifname_by_ssid_idx(ssid_idx, interface_name);

	if (getWlStaInfo(interface_name, (WLAN_STA_INFO_Tp) buff) < 0) {
		printf("Read wlan sta info failed!\n");
		*result = 0;
		sprintf(error, "check sta info failed");
		return;
	}

	for (i = 1; i <= MAX_STA_NUM; i++) {
		pInfo = (WLAN_STA_INFO_Tp) &buff[i];
		if (pInfo->aid && (pInfo->flags & STA_INFO_FLAG_ASOC)) {
			if(!strncmp(pInfo->addr, mac_addr, 6)){
				found = 1;
				break;
			}
		}
	}
	if(found == 0){
		*result = 0;
		sprintf(error, "no sta exists");
		return;
	}

	snprintf(mac_addr_str, 13, "%02x%02x%02x%02x%02x%02x",
		mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

	doWlKickOutDevice(interface_name, mac_addr_str);

	if (getWlStaInfo(interface_name, (WLAN_STA_INFO_Tp) buff) < 0) {
		printf("Read wlan sta info failed!\n");
		*result = 0;
		sprintf(error, "check sta info failed");
		return;
	}

	found = 0;

	for (i = 1; i <= MAX_STA_NUM; i++) {
		pInfo = (WLAN_STA_INFO_Tp) &buff[i];
		if (pInfo->aid && (pInfo->flags & STA_INFO_FLAG_ASOC)) {
			if(!strncmp(pInfo->addr, mac_addr, 6)){
				found = 1;
				break;
			}
		}
	}
	if(found == 0){
		*result = 1; //SUCCESS
	}
	else{
		*result = 0; //FAILED
		sprintf(error, "del sta failed");
	}

}
#endif


#ifndef WIFI_SITE_MONITOR
#define WIFI_SITE_MONITOR 0x00000800
#endif
#ifndef WIFI_WAIT_FOR_CHANNEL_SELECT
#define WIFI_WAIT_FOR_CHANNEL_SELECT 0x08000000
#endif
int waitWlChannelSelect(char *ifname)
{
	unsigned char buffer[10];
	unsigned int opmode;
	int skfd, cmd_id, vwlanidx=0;
	struct iwreq wrq;
	int query_cnt = 0;
	MIB_CE_MBSSIB_T WlanEnable_Entry;

	memset(&WlanEnable_Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan(ifname,&WlanEnable_Entry,&vwlanidx)!=0)
		return -1;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	do {            //wait for selecting channel
		strcpy(wrq.ifr_name, ifname);
		strcpy(buffer,"opmode");
		wrq.u.data.pointer = (caddr_t)buffer;
		if(WlanEnable_Entry.is_ax_support)
		{
			wrq.u.data.flags = RTL8192CD_IOCTL_GET_MIB;
			cmd_id = SIOCDEVPRIVATEAXEXT;
		}
		else
		{
			cmd_id = RTL8192CD_IOCTL_GET_MIB;
		}

		wrq.u.data.length = strlen(buffer);

		if(ioctl(skfd, cmd_id, &wrq) == 0){
			opmode = *((unsigned int*)buffer);
			if((opmode & WIFI_SITE_MONITOR)
				|| (opmode & WIFI_WAIT_FOR_CHANNEL_SELECT)
			)
				sleep(1);
			else break;
		}
		query_cnt ++;
	}while(query_cnt < 10);       //WIFI_WAIT_FOR_CHANNEL_SELECT 0x08000000 (8192cd.h)

	close( skfd );

	return (query_cnt < 10) ? 1 : 0;
}
int getWlChannel( char *interface, unsigned int *num)
{
	int skfd, cmd_id, vwlanidx=0;
	struct iwreq wrq;
	unsigned char buffer[32]={0};
	MIB_CE_MBSSIB_T WlanEnable_Entry;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
	{
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(&WlanEnable_Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan(interface,&WlanEnable_Entry,&vwlanidx)!=0)
		return -1;

	strcpy(buffer,"channel");
	wrq.u.data.pointer = (caddr_t)buffer;
	if(WlanEnable_Entry.is_ax_support)
	{
		wrq.u.data.flags = RTL8192CD_IOCTL_GET_MIB;
		cmd_id = SIOCDEVPRIVATEAXEXT;
	}
	else
	{
		cmd_id = RTL8192CD_IOCTL_GET_MIB;
	}

	wrq.u.data.length = 10;

	if (iw_get_ext(skfd, interface, cmd_id, &wrq) < 0)
	{
		close( skfd );
		return -1;
	}
	*num = (unsigned int)buffer[wrq.u.data.length - 1];
	close( skfd );

	return 0;
}

unsigned char get_wlan_phyband(void)
{
	unsigned char phyband=PHYBAND_2G;
#ifdef WLAN_DUALBAND_CONCURRENT
	mib_get_s(MIB_WLAN_PHY_BAND_SELECT, &phyband, sizeof(phyband));
#endif
	return phyband;
}

//check flash config if wlan0 is up, called by BOA,
//0:down, 1:up
int wlan_is_up(void)
{
	int value=0;
	MIB_CE_MBSSIB_T Entry;
	if(mib_chain_get(MIB_MBSSIB_TBL,0,&Entry) == 0)
		printf("\n %s %d\n", __func__, __LINE__);

	if (Entry.wlanDisabled==0) {  // wlan0 is enabled
		value=1;
	}
	return value;
}


int setup_wlan_block(void)
{
#ifdef WLAN_MBSSID
	int i;
	char wlan_dev[16];
	unsigned char wlan_hw_diabled;
#ifdef CONFIG_E8B
	int j;
	MIB_CE_MBSSIB_T Entry;
#endif
#endif
	unsigned char enable = 0;

	va_cmd(EBTABLES, 2, 1, "-F", "wlan_block");

	// Between ELAN & WIFI
	mib_get_s(MIB_WLAN_BLOCK_ETH2WIR, (void *)&enable, sizeof(enable));
#ifdef CONFIG_RTK_L34_ENABLE
	rg_eth2wire_block(enable);
#endif
	if(enable)
	{
		va_cmd(EBTABLES, 8, 1, "-A", "wlan_block", "-i", "wlan+", "-o", "eth0.+", "-j", "DROP");
		va_cmd(EBTABLES, 8, 1, "-A", "wlan_block", "-i", "eth0.+", "-o", "wlan+", "-j", "DROP");
	}

#ifdef WLAN_MBSSID
	//mib_get_s(MIB_WLAN_BLOCK_MBSSID, (void *)&enable, sizeof(enable));
	for(i = 0; i<NUM_WLAN_INTERFACE; i++)
	{
		mib_local_mapping_get(MIB_WLAN_MODULE_DISABLED, i, (void *)&wlan_hw_diabled);
		if(wlan_hw_diabled)
			continue;

		mib_local_mapping_get(MIB_WLAN_BLOCK_MBSSID, i, (void *)&enable);
		if(enable)
		{
			// Between MBSSIDs
			rtk_wlan_get_ifname(i, 0, wlan_dev);
			strcat(wlan_dev, "+");
			va_cmd(EBTABLES, 8, 1, "-A", "wlan_block", "-i", wlan_dev, "-o", wlan_dev, "-j", "DROP");
		}
#ifdef CONFIG_E8B
		for ( j=0; j<=NUM_VWLAN_INTERFACE; j++)
		{
			if (!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&Entry)) {
				printf("Error! Get MIB_MBSSIB_TBL error.\n");
				continue;
			}
			if(Entry.wlanDisabled)
				continue;
#ifdef CONFIG_RTK_L34_ENABLE
			RG_Wlan_Portisolation_Set(Entry.ssidisolation, ((i*(NUM_VWLAN_INTERFACE+1))+(j+1)));
#endif
			if(Entry.ssidisolation)
			{
				rtk_wlan_get_ifname(i, j, wlan_dev);
				va_cmd(EBTABLES, 9, 1, "-A", "wlan_block", "-i", wlan_dev, "-o", "!" , "nas+", "-j", "DROP");
				va_cmd(EBTABLES, 9, 1, "-A", "wlan_block", "-i", "!", "nas+", "-o", wlan_dev, "-j", "DROP");
			}
		}
#endif
	}
#endif
	return 0;
}

#ifdef CONFIG_GUEST_ACCESS_SUPPORT
int setup_wlan_guestaccess()
{
	MIB_CE_MBSSIB_T mbssid_entry, root_Entry;
	char ifname[16]={0}, parm[64]={0};
	int i, j;
	int ori_wlan_idx = wlan_idx;

	//clean old chain
	clean_wlan_guestaccess();

	//add new chain
	va_cmd(EBTABLES, 2, 1, "-N", GUEST_ACCESS_CHAIN_EB);
	va_cmd(EBTABLES, 3, 1, "-P", GUEST_ACCESS_CHAIN_EB, "RETURN");
	va_cmd(EBTABLES, 4, 1, "-I", FW_FORWARD, "-j", GUEST_ACCESS_CHAIN_EB);
	va_cmd(IPTABLES, 2, 1, "-N", GUEST_ACCESS_CHAIN_IP);
	va_cmd(IPTABLES, 4, 1, "-I", FW_INPUT, "-j", GUEST_ACCESS_CHAIN_IP);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", GUEST_ACCESS_CHAIN_IPV6);
	va_cmd(IP6TABLES, 4, 1, "-I", FW_INPUT, "-j", GUEST_ACCESS_CHAIN_IPV6);
#endif

	for(i=0; i<NUM_WLAN_INTERFACE; ++i){
		wlan_idx = i;
		memset(&root_Entry, 0, sizeof(MIB_CE_MBSSIB_T));
		mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&root_Entry);
		if(root_Entry.wlanDisabled)
			continue;
		for(j=0; j<=NUM_VWLAN_INTERFACE; ++j){
			memset(&mbssid_entry, 0, sizeof(MIB_CE_MBSSIB_T));
			if(mib_chain_get(MIB_MBSSIB_TBL, j, (void *)&mbssid_entry) == 0)
				printf("[%s %d]Error! Get MIB_MBSSIB_TBL error.\n",__FUNCTION__,__LINE__);
			if(mbssid_entry.wlanDisabled)
				continue;
			if(mbssid_entry.guest_access){
				rtk_wlan_get_ifname(i, j, ifname);
				/*********between guest access and lan**********/
				va_cmd(EBTABLES, 8, 1, "-A", GUEST_ACCESS_CHAIN_EB, "-i", ifname, "-o", "eth+", "-j", (char *)FW_DROP);
				/*********between guest access and wlan**********/
				va_cmd(EBTABLES, 8, 1, "-A", GUEST_ACCESS_CHAIN_EB, "-i", ifname, "-o", "wlan+", "-j", (char *)FW_DROP);
				/*********guest access sta can't access to DUT web page*******/
				va_cmd(IPTABLES, 10, 1, "-A", GUEST_ACCESS_CHAIN_IP, "-m", "physdev", "--physdev-in", ifname, "-p", "tcp", "-j", (char *)FW_DROP);
				#ifdef CONFIG_IPV6
				va_cmd(IP6TABLES, 10, 1, "-A", GUEST_ACCESS_CHAIN_IPV6, "-m", "physdev", "--physdev-in", ifname, "-p", "tcp", "-j", (char *)FW_DROP);
				#endif
			}
		}
	}
	wlan_idx = ori_wlan_idx;
	return 0;
}

int clean_wlan_guestaccess()
{
	va_cmd(EBTABLES, 2, 1, "-F",GUEST_ACCESS_CHAIN_EB);//delete all rules in GUEST_ACCESS_CHAIN_EB
	va_cmd(EBTABLES, 4, 1, "-D", FW_FORWARD, "-j", GUEST_ACCESS_CHAIN_EB); //delete GUEST_ACCESS_CHAIN_EB referenced in FORWARD
	va_cmd(EBTABLES, 2, 1, "-X",GUEST_ACCESS_CHAIN_EB); //delete GUEST_ACCESS_CHAIN_EB
	va_cmd(IPTABLES, 2, 1, "-F",GUEST_ACCESS_CHAIN_IP);
	va_cmd(IPTABLES, 4, 1, "-D", FW_INPUT, "-j", GUEST_ACCESS_CHAIN_IP);
	va_cmd(IPTABLES, 2, 1, "-X",GUEST_ACCESS_CHAIN_IP);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F",GUEST_ACCESS_CHAIN_IPV6);
	va_cmd(IP6TABLES, 4, 1, "-D", FW_INPUT, "-j", GUEST_ACCESS_CHAIN_IPV6);
	va_cmd(IP6TABLES, 2, 1, "-X",GUEST_ACCESS_CHAIN_IPV6);
#endif
	return 0;
}
#endif

#ifdef WLAN_BAND_STEERING
int rtk_wlan_get_band_steering_status(void)
{
	int i = 0, check_wpa_cipher = 0;
	int orig_wlan_idx = wlan_idx;
	MIB_CE_MBSSIB_T wlan_entry[NUM_WLAN_INTERFACE];

	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		wlan_getEntry(&wlan_entry[i], 0);
	}
	wlan_idx = orig_wlan_idx;

	if(!strcmp(wlan_entry[0].ssid, wlan_entry[1].ssid))
	{
		if(wlan_entry[0].encrypt == wlan_entry[1].encrypt)
		{
			if(wlan_entry[0].encrypt == WIFI_SEC_NONE)
			{
				if(wlan_entry[0].enable1X == wlan_entry[1].enable1X)
					return 1;
			}
			else if(wlan_entry[0].encrypt == WIFI_SEC_WEP)
			{
				if(wlan_entry[0].enable1X == wlan_entry[1].enable1X)
				{
					if(wlan_entry[0].enable1X == 0)
					{
						if((wlan_entry[0].wep == wlan_entry[1].wep) && (wlan_entry[0].authType == wlan_entry[1].authType))
						{
							if(wlan_entry[0].wep == WEP64)
							{
								if(!strncmp(wlan_entry[0].wep64Key1, wlan_entry[1].wep64Key1, WEP64_KEY_LEN))
									return 1;
							}
							else if(wlan_entry[0].wep == WEP128)
							{
								if(!strncmp(wlan_entry[0].wep128Key1, wlan_entry[1].wep128Key1, WEP128_KEY_LEN))
									return 1;
							}
						}
					}
					else
					{
						if(wlan_entry[0].wep == wlan_entry[1].wep)
							return 1;
					}
				}
			}
			else if(wlan_entry[0].encrypt >= WIFI_SEC_WPA)
			{
				switch(wlan_entry[0].encrypt)
				{
					case WIFI_SEC_WPA:
						if(wlan_entry[0].unicastCipher == wlan_entry[1].unicastCipher)
							check_wpa_cipher = 1;
						break;
					case WIFI_SEC_WPA2:
						if((wlan_entry[0].wpa2UnicastCipher == wlan_entry[1].wpa2UnicastCipher)
#ifdef WLAN_11W
						&& (wlan_entry[0].dotIEEE80211W == wlan_entry[1].dotIEEE80211W)
#endif
						)
						{
#ifdef WLAN_11W

							if((wlan_entry[0].dotIEEE80211W == 1) && (wlan_entry[0].sha256 == wlan_entry[1].sha256)) //1: MGMT_FRAME_PROTECTION_OPTIONAL
								check_wpa_cipher = 1;
							else if(wlan_entry[0].dotIEEE80211W != 1)
#endif
								check_wpa_cipher = 1;
						}
						break;
					case WIFI_SEC_WPA2_MIXED:
						if((wlan_entry[0].unicastCipher == wlan_entry[1].unicastCipher) &&
							(wlan_entry[0].wpa2UnicastCipher == wlan_entry[1].wpa2UnicastCipher))
							check_wpa_cipher = 1;
						break;
#ifdef WLAN_WPA3
					case WIFI_SEC_WPA3:
					case WIFI_SEC_WPA2_WPA3_MIXED:
						check_wpa_cipher = 1;
						break;
#endif
					default:
						break;
				}
				if(wlan_entry[0].wpaAuth == WPA_AUTH_PSK)
				{
					if(check_wpa_cipher && (!strncmp(wlan_entry[0].wpaPSK, wlan_entry[1].wpaPSK, MAX_PSK_LEN+1)))
						return 1;
				}
				else
					if(check_wpa_cipher)
						return 1;

			}
		}
	}

	return 0;
}

/*
 * call when two SSIDs or Encryption change, also before mib update & config_WLAN
 */
int rtk_wlan_update_band_steering_status(void)
{
	unsigned char sta_control = 0;
	if(rtk_wlan_get_band_steering_status() == 0)
	{
		mib_get_s(MIB_WIFI_STA_CONTROL, &sta_control, sizeof(sta_control));
		if(sta_control & STA_CONTROL_ENABLE){
			sta_control &= ~STA_CONTROL_ENABLE;
			mib_set(MIB_WIFI_STA_CONTROL, &sta_control);
			syslog(LOG_ALERT, "WLAN: Disable Band Steering");
			return 1;
		}
	}
	return 0;
}
#define WLAN_MGR_CONF "/var/wlan_manager.conf"
const char WLAN_MGR_DAEMON[] = "/bin/wlan_manager";

void rtk_wlan_stop_band_steering(void)
{
	pid_t p_num = 0;
	if((p_num = findPidByName("wlan_manager"))>0)
		kill(p_num, SIGTERM);
}
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
void rtk_wlan_setup_band_steering(void)
{
	unsigned char vuChar = 0;
	unsigned int intVal = 0;
	char parm[64]={0};
	//char ifname[IFNAMSIZ]={0};
	//unsigned char phyband = 0;
	FILE *fp=NULL;
	//rtk_wlan_get_ifname(wlanIdx, 0, ifname);
	if(mib_get_s( MIB_WIFI_STA_CONTROL, (void *)&vuChar, sizeof(vuChar)) && vuChar)
	{
		fp = fopen(WLAN_MGR_CONF, "w");
		if(fp){

			if(mib_get_s(MIB_WLAN_BANDST_ENABLE, &vuChar, sizeof(vuChar)) && !vuChar)
			{
				vuChar = 1;
				mib_set(MIB_WLAN_BANDST_ENABLE, &vuChar);
			}

			if(mib_get_s(MIB_WLAN_BANDST_RSSTHRDHIGH, &intVal, sizeof(intVal)))
			{
				intVal = intVal+100;
				//snprintf(parm, sizeof(parm), "stactrl_rssi_th_high=%d", intVal);
				//va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
				fprintf(fp, "prefer_band_rssi_high=%d\n", intVal);
			}

			if(mib_get_s(MIB_WLAN_BANDST_RSSTHRDLOW, &intVal, sizeof(intVal)))
			{
				intVal = intVal+100;
				//snprintf(parm, sizeof(parm), "stactrl_rssi_th_low=%d", intVal);
				//va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
				fprintf(fp, "prefer_band_rssi_low=%d\n", intVal);
			}

			if(mib_get_s(MIB_WLAN_BANDST_CHANNELUSETHRD, &intVal, sizeof(intVal)))
			{
				//intVal = ((intVal*255)/100);
				//snprintf(parm, sizeof(parm), "stactrl_ch_util_th=%d", intVal);
				//va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
				fprintf(fp, "roam_ch_clm_th=%d\n", intVal);
			}

			if(mib_get_s(MIB_WLAN_BANDST_STEERINGINTERVAL, &intVal, sizeof(intVal)))
			{
				//snprintf(parm, sizeof(parm), "stactrl_steer_detect=%d", intVal);
				//va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
				intVal = intVal/2;
				fprintf(fp, "roam_detect_th=%d\n", intVal);
			}

			if(mib_get_s(MIB_WLAN_BANDST_STALOADTHRESHOLD2G, &intVal, sizeof(intVal)))
			{
				//snprintf(parm, sizeof(parm), "stactrl_sta_load_th_2g=%d", intVal);
				//va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
				fprintf(fp, "roam_sta_tp_th=%d\n", intVal);
			}

			//mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlanIdx, (void *)&phyband);
			//snprintf(parm, sizeof(parm), "stactrl_prefer_band=%d", (phyband == PHYBAND_5G)?1:0);
			//va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
			fprintf(fp, "prefer_band=%s\n", "5G");

#ifdef WLAN_USE_VAP_AS_SSID1
			//snprintf(parm, sizeof(parm), "stactrl_groupID=2");
			//status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
			fprintf(fp, "prefer_band_grp_ssid1=1\n");
			fprintf(fp, "non_prefer_band_grp_ssid1=1\n");
#else
			fprintf(fp, "prefer_band_grp_ssid0=0\n");
			fprintf(fp, "non_prefer_band_grp_ssid0=0\n");
#endif
			fclose(fp);
			va_niced_cmd(WLAN_MGR_DAEMON, 2, 0, "-config_fname", WLAN_MGR_CONF);
		}
	}
}
#else
#ifdef WLAN_MANAGER_SUPPORT
typedef struct
{
	const char *name;
	int mib_id;
	TYPE_T mib_type;
}_wlan_mgr_conf;

_wlan_mgr_conf wlan_mgr_conf_table[]=
{
	{"bss_tm_req_th",					MIB_WLAN_MGR_BSS_TM_REQ_TH,					BYTE_T},
	{"bss_tm_req_disassoc_imminent",	MIB_WLAN_MGR_BSS_TM_REQ_DISASSOC_IMMINENT,	BYTE_T},
	{"bss_tm_req_disassoc_timer",		MIB_WLAN_MGR_BSS_TM_REQ_DISASSOC_TIMER,		DWORD_T},
	{"roam_detect_th",					MIB_WLAN_MGR_ROAM_DETECT_TH,				BYTE_T},
	{"roam_sta_tp_th",					MIB_WLAN_MGR_ROAM_STA_TP_TH,				DWORD_T},
	{"roam_ch_clm_th",					MIB_WLAN_MGR_ROAM_CH_CLM_TH,				BYTE_T},
	{"prefer_band_rssi_high",			MIB_WLAN_MGR_PREFER_BAND_RSSI_HIGH, 		BYTE_T},
	{"prefer_band_rssi_low",			MIB_WLAN_MGR_PREFER_BAND_RSSI_LOW, 			BYTE_T},
	{"prefer_band_grp_ssid0",			MIB_WLAN_MGR_PREFER_BAND_GRP_SSID0, 		BYTE_T},
	{"non_prefer_band_grp_ssid0",		MIB_WLAN_MGR_NON_PREFER_BAND_GRP_SSID0, 	BYTE_T},
	{"prefer_band_grp_ssid1",			MIB_WLAN_MGR_PREFER_BAND_GRP_SSID1, 		BYTE_T},
	{"non_prefer_band_grp_ssid1",		MIB_WLAN_MGR_NON_PREFER_BAND_GRP_SSID1, 	BYTE_T},
	{"prefer_band_grp_ssid2",			MIB_WLAN_MGR_PREFER_BAND_GRP_SSID2, 		BYTE_T},
	{"non_prefer_band_grp_ssid2",		MIB_WLAN_MGR_NON_PREFER_BAND_GRP_SSID2, 	BYTE_T},
	{"prefer_band_grp_ssid3",			MIB_WLAN_MGR_PREFER_BAND_GRP_SSID3, 		BYTE_T},
	{"non_prefer_band_grp_ssid3",		MIB_WLAN_MGR_NON_PREFER_BAND_GRP_SSID3, 	BYTE_T},
	{"prefer_band_grp_ssid4",			MIB_WLAN_MGR_PREFER_BAND_GRP_SSID4, 		BYTE_T},
	{"non_prefer_band_grp_ssid4",		MIB_WLAN_MGR_NON_PREFER_BAND_GRP_SSID4, 	BYTE_T},
};
#endif

void rtk_wlan_setup_band_steering(void)
{
	unsigned char sta_control = 0/*, phyband = 0*/, stactrl_prefer_band = 0;
	//char parm[64]={0};
	//char ifname[IFNAMSIZ]={0};
	FILE *fp=NULL;
#ifdef WLAN_MANAGER_SUPPORT
	int i=0, size=0;
	unsigned char value[4];
#endif

	//rtk_wlan_get_ifname(wlanIdx, 0, ifname); //SSID1

	mib_get_s(MIB_WIFI_STA_CONTROL, &sta_control, sizeof(sta_control));
	if(sta_control & STA_CONTROL_ENABLE){
		fp = fopen(WLAN_MGR_CONF, "w");
		if(fp){
			//snprintf(parm, sizeof(parm), "stactrl_enable=%d", (sta_control & STA_CONTROL_ENABLE)?1:0);
			//va_cmd(IWPRIV, 3, 1, (char *)ifname, "set_mib", parm);

			//if(sta_control & STA_CONTROL_ENABLE){
				//mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlanIdx, (void *)&phyband);
				stactrl_prefer_band = (sta_control & STA_CONTROL_PREFER_BAND)? PHYBAND_2G:PHYBAND_5G;
				//if(stactrl_prefer_band == phyband)
				//	snprintf(parm, sizeof(parm), "stactrl_prefer_band=1");
				//else
				//	snprintf(parm, sizeof(parm), "stactrl_prefer_band=0");
				fprintf(fp, "prefer_band=%s\n", (stactrl_prefer_band == PHYBAND_5G)? "5G":"24G");
#ifdef WLAN_MANAGER_SUPPORT
				size = sizeof(wlan_mgr_conf_table)/sizeof(_wlan_mgr_conf);
				for(i=0; i<size; i++){
					switch (wlan_mgr_conf_table[i].mib_type) {
						case BYTE_T:
							mib_get_s(wlan_mgr_conf_table[i].mib_id, value, sizeof(value));
							fprintf(fp, "%s=%hhu\n", wlan_mgr_conf_table[i].name, *(unsigned char *)value);
							break;
						case DWORD_T:
							mib_get_s(wlan_mgr_conf_table[i].mib_id, value, sizeof(value));
							fprintf(fp, "%s=%u\n", wlan_mgr_conf_table[i].name, *(unsigned int *)value);
							break;
						default:
							printf("%s: Unknown data type %d!\n", __func__, wlan_mgr_conf_table[i].mib_type);
					}
				}
#else
#ifdef WLAN_USE_VAP_AS_SSID1
				//snprintf(parm, sizeof(parm), "stactrl_groupID=2");
				//va_cmd(IWPRIV, 3, 1, (char *)ifname, "set_mib", parm);
				fprintf(fp, "prefer_band_grp_ssid1=1\n");
				fprintf(fp, "non_prefer_band_grp_ssid1=1\n");
#else
				fprintf(fp, "prefer_band_grp_ssid0=0\n");
				fprintf(fp, "non_prefer_band_grp_ssid0=0\n");
#endif
#endif //WLAN_MANAGER_SUPPORT
				fclose(fp);
				va_niced_cmd(WLAN_MGR_DAEMON, 2, 0, "-config_fname", WLAN_MGR_CONF);
			//}
			//else
			//	snprintf(parm, sizeof(parm), "stactrl_prefer_band=0");
			//va_cmd(IWPRIV, 3, 1, (char *)ifname, "set_mib", parm);
		}
	}
}
#endif
#endif


#ifdef _PRMT_X_CMCC_WLANSHARE_
#define FW_BR_WLANSHARE "WLAN_SHARE"

static int get_bound_br_wan_by_ssid_index(int ssid_index, char *wan_ifname)
{
	MIB_CE_ATM_VC_T entry;
	int total = mib_chain_total(MIB_ATM_VC_TBL);
	int i;
	int base = PMAP_WLAN0, mask = 0;

	//calculate mask
#ifdef WLAN_DUALBAND_CONCURRENT
	if(ssid_index < 1 || ssid_index > 8)
		return -1;

	if(ssid_index > 4)
	{
#if (defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU))&& defined(CONFIG_LUNA_DUAL_LINUX)
		base = PMAP_WLAN0; //ssid 1-4 in wlan1, ssid 5-8 in wlan0
#else
		base = PMAP_WLAN1;
#endif
		ssid_index -= 4;
	}
#if (defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU))&& defined(CONFIG_LUNA_DUAL_LINUX)
	else
		base = PMAP_WLAN1; //ssid 1-4 in wlan1, ssid 5-8 in wlan0
#endif

#else
	if(ssid_index < 1 || ssid_index > 4)
		return -1;
#endif

	mask = (1 << (base + ssid_index - 1));

	for(i = 0 ; i < total; i++)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, &entry) == 0)
			continue;

		if(entry.cmode != CHANNEL_MODE_BRIDGE)
			continue;

		if(entry.itfGroup & mask)
		{
			if(ifGetName(entry.ifIndex, wan_ifname, IFNAMSIZ) != NULL)
				return 0;
		}
	}

	return -1;
}

void setup_wlan_share(void)
{
	int total = mib_chain_total(MIB_WLAN_SHARE_TBL);
	int i;
	MIB_CE_WLAN_SHARE_T entry;
	char ifname[IFNAMSIZ] = {0};
	int pid = -1;
	char wan_ifname[IFNAMSIZ] = {0};

	// Stop dhcp relay
	pid = read_pid("/var/run/dhcrelay.pid");
	if(pid > 0)
	{
		kill(pid, SIGTERM);
		usleep(200000);
	}
	// setup ebtables rules
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N", FW_BR_WLANSHARE);
	va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P", FW_BR_WLANSHARE, "RETURN");

	// ebtables -t broute -D BROUTING -p IPv4 --ip-proto udp --ip-sport 68 --ip-dport 67 -j WLAN_SHARE
	va_cmd(EBTABLES, 14, 1, "-t", "broute", "-D", "BROUTING", "-p", "IPv4", "--ip-proto", "udp",
					"--ip-sport", "68", "--ip-dport", "67", "-j", FW_BR_WLANSHARE);

	// ebtables -t broute -A BROUTING -p IPv4 --ip-proto udp --ip-sport 68 --ip-dport 67 -j WLAN_SHARE
	va_cmd(EBTABLES, 14, 1, "-t", "broute", "-A", "BROUTING", "-p", "IPv4", "--ip-proto", "udp",
					"--ip-sport", "68", "--ip-dport", "67", "-j", FW_BR_WLANSHARE);

	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", FW_BR_WLANSHARE);

	if(total == 0)
		return;
	for(i = 0 ; i < MAX_WLAN_SHARE ; i++)
	{
		if(mib_chain_get(MIB_WLAN_SHARE_TBL, i, &entry) == 0)
			continue;

		if(get_ifname_by_ssid_index(entry.ssid_idx, ifname) < 0)
		{
			fprintf(stderr, "Get SSID%d interface name faileld\n", entry.ssid_idx);
			continue;
		}

		if(entry.userid_enable && entry.userid[0] != '\0' && get_bound_br_wan_by_ssid_index(entry.ssid_idx, wan_ifname) == 0)
		{
			//ebtables -t broute -A WLAN_SHARE -i wlan0-vap0 -j DROP
			va_cmd(EBTABLES, 8, 1, "-t", "broute", "-A", FW_BR_WLANSHARE, "-i", ifname, "-j", "DROP");

			//dhcrelayV6 -i wlan0 -o nas0_0 -4 -r 09557700844
			va_cmd(DHCREALYV6, 7, 1, "-4", "-i", ifname, "-o", wan_ifname, "-r", entry.userid);

		}

		// only support 1 instance currently.
		break;
	}
}
#endif

#if (defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)) && !defined(CONFIG_RTK_DEV_AP)
static int getWlanStatus(int idx)
{
	MIB_CE_MBSSIB_T Entry;
	int j=0;
	unsigned int intf_map=0;
	unsigned char wlanDisabled =0;

#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
	mib_local_mapping_get(MIB_WLAN_MODULE_DISABLED, idx, &wlanDisabled);
#else
	mib_get_s(MIB_WIFI_MODULE_DISABLED, &wlanDisabled, sizeof(wlanDisabled));
#endif

	if(wlanDisabled == 0)
	{
		for (j=0; j<WLAN_SSID_NUM; j++) {
			if (!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, idx, j, (void *)&Entry)) {
				//printf("Error! Get MIB_MBSSIB_TBL error.\n");
				continue;
			}
			else{
				if (!Entry.wlanDisabled) {
					intf_map |= (1 << j);
				}
			}
		}
	}
	//printf("%s %s!\n", WLANIF[idx], intf_map!=0? "enable":"disable");
	return intf_map!=0;
}
#elif defined(CONFIG_USER_ANDLINK_PLUGIN)
static int getWlanStatus(int idx)
{
	MIB_CE_MBSSIB_T Entry;
	int j=0;
	unsigned int intf_map=0;
	unsigned char wlanDisabled =0;

	mib_local_mapping_get(MIB_WLAN_DISABLED, idx, &wlanDisabled);
	if(wlanDisabled == 0)
	{
		for (j=0; j<WLAN_SSID_NUM; j++) {
			if (!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, idx, j, (void *)&Entry)) {
				continue;
			}
			else{
				if (!Entry.wlanDisabled) {
					intf_map |= (1 << j);
				}
			}
		}
	}
	return intf_map!=0;
}
#else
static int getWlanStatus(int idx)
{
	unsigned char wlanDisabled =0;

	mib_get_s(MIB_WIFI_MODULE_DISABLED, &wlanDisabled, sizeof(wlanDisabled));
	return (wlanDisabled == 0);
}
#endif

#if defined(CONFIG_WIFI_LED_USE_SOC_GPIO)
#define LED_WIFI_LABEL "LED_WIFI"
// led_mode: 0:led off   1:led on 2:led state restore
static int _set_wifi_led_status(int led_mode)
{
	unsigned int max_brightness = 0;
	char buf[128]={0}, cmd[256]={0};
	FILE *fp=NULL;
	snprintf(buf, sizeof(buf), "/sys/class/leds/%s/brightness", LED_WIFI_LABEL);
	if(access(buf, F_OK) != 0)
		return -1;

	if(led_mode != 2){
		if(led_mode == 1){
			snprintf(buf, sizeof(buf), "/sys/class/leds/%s/max_brightness", LED_WIFI_LABEL);
			fp=fopen(buf, "r");
			if(fp){
				fscanf(fp, "%u", &max_brightness);
				fclose(fp);
			}
		}
		snprintf(cmd, sizeof(cmd), "echo none > /sys/class/leds/%s/trigger", LED_WIFI_LABEL);
		va_niced_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "echo %u > /sys/class/leds/%s/brightness", max_brightness, LED_WIFI_LABEL);
		va_niced_cmd("/bin/sh", 2, 1, "-c", cmd);
	}
	else{ //restore
		snprintf(cmd, sizeof(cmd), "echo wlanled > /sys/class/leds/%s/trigger", LED_WIFI_LABEL);
		va_niced_cmd("/bin/sh", 2, 1, "-c", cmd);
	}
	return 0;
}
#endif

// led_mode: 0:led off   1:led on 2:led state restore
int set_wlan_led_status(int led_mode)
{
        int i=0;
        char cmd_str[256]={0};
        int led_value;
        unsigned int wlSt = 0;

        for(i=0; i<NUM_WLAN_INTERFACE; i++)
        {
#ifdef CONFIG_RTK_DEV_AP
                if(led_mode == 2)
                {
                        wlSt |= (getWlanStatus(i)) ? 1<<i : 0;
                        if((wlSt&(1<<i)))
                                led_value = 2;
                        else
                                led_value = 0; // set wifi led off when root is func_off and no vap interface up
                }
                else led_value = led_mode;
#else
                led_value = led_mode;
#endif
#if !(defined(CONFIG_WIFI_LED_USE_SOC_GPIO) || defined(CONFIG_WIFI_LED_SHARED))
                //snprintf(cmd_str, sizeof(cmd_str), "echo %d > /proc/%s/led", led_value, WLANIF[i]);
                snprintf(cmd_str, sizeof(cmd_str), "echo %d > " LINK_FILE_NAME_FMT, led_value, WLANIF[i], "led");
                va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#endif
#ifdef CONFIG_SLAVE_WLAN1_ENABLE
                if(!strcmp("wlan1", WLANIF[i])){
                        //snprintf(cmd_str, sizeof(cmd_str), "echo %d >> /proc/%s/led", led_value, WLANIF[i]);
                snprintf(cmd_str, sizeof(cmd_str), "echo %d >> " LINK_FILE_NAME_FMT, led_value, WLANIF[i], "led");
                va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
                }
#endif
        }
#if defined(CONFIG_WIFI_LED_SHARED)
#if 0
        led_value = (led_mode == 2) ? ((wlSt) ? 2 : 0) : led_mode;
        //snprintf(cmd_str, sizeof(cmd_str), "echo %d > /proc/%s/led", led_value, WLANIF[0]);
        snprintf(cmd_str, sizeof(cmd_str), "echo %d > " LINK_FILE_NAME_FMT, led_value, WLANIF[0], "led");
        va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#else
	snprintf(cmd_str, sizeof(cmd_str), "echo %d > " LINK_FILE_NAME_FMT, led_mode, WLANIF[0], "led");
	va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#endif
#endif
#if defined(CONFIG_WIFI_LED_USE_SOC_GPIO)
#if 0
	led_value = (led_mode == 2) ? ((wlSt) ? 2 : 0) : led_mode;
	snprintf(cmd_str, sizeof(cmd_str), "echo %d > /proc/led_wifi", led_value, WLANIF[0]);
	va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#else
	_set_wifi_led_status(led_mode);
#endif
#endif

#if defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
	_set_wlan_wps_led_status(led_mode);
#endif
        return 0;
}

#ifdef _PRMT_X_WLANFORISP_
int isWLANForISP(int vwlan_idx)
{
	int k, total = mib_chain_total(MIB_WLANFORISP_TBL);
	MIB_WLANFORISP_T Entry;

	for(k=0; k<total; k++){
		if(mib_chain_get(MIB_WLANFORISP_TBL, k, &Entry)==0)
			continue;
		if(Entry.SSID_IDX == (wlan_idx*(1+WLAN_MBSSID_NUM)+(vwlan_idx+1)))
			return 1;
	}
	return 0;
}
//update when MIB_WLANFORISP_TBL change
void update_WLANForISP_configured(void)
{
	int i, j, k, total = mib_chain_total(MIB_WLANFORISP_TBL);
	MIB_WLANFORISP_T Entry;
	MIB_CE_MBSSIB_T mEntry;
	int found=0, change_flag=0, change_update_flag=0;

	for(i=0; i<NUM_WLAN_INTERFACE;i++){
		for(j=0; j<WLAN_SSID_NUM; j++){
			if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, &mEntry)==0)
				continue;
			found = 0;
			change_flag = 0;
			for(k=0; k<total; k++){
				if(mib_chain_get(MIB_WLANFORISP_TBL, k, &Entry)==0)
					continue;
				if(Entry.SSID_IDX == (i*(WLAN_SSID_NUM)+(j+1))){
					if(mEntry.encrypt != WIFI_SEC_WPA2_MIXED){
						mEntry.encrypt = WIFI_SEC_WPA2_MIXED;
						change_flag = 1;
					}
					if(mEntry.wpaAuth != WPA_AUTH_AUTO){
						mEntry.wpaAuth = WPA_AUTH_AUTO;
						change_flag = 1;
					}
					if(mEntry.enable1X != 0){
						mEntry.enable1X = 0;
						change_flag = 1;
					}
					if(strcmp(mEntry.ssid, Entry.SSID)){
						strcpy(mEntry.ssid, Entry.SSID);
						change_flag = 1;
					}
					if(strcmp(mEntry.rsPassword, Entry.RadiusKey)){
						strcpy(mEntry.rsPassword, Entry.RadiusKey);
						change_flag = 1;
					}
					if(*((unsigned long *)mEntry.rsIpAddr)!= *((unsigned long *)Entry.RadiusServer)){
						*((unsigned long *)mEntry.rsIpAddr) =  *((unsigned long *)Entry.RadiusServer);
						change_flag = 1;
					}
					found = 1;
					break;
				}
			}
			if(found==0){ //change back if entry is deleted
				if(mEntry.enable1X==1){
					mEntry.enable1X = 0;
					change_flag = 1;
				}
				if(mEntry.wpaAuth == WPA_AUTH_AUTO){
					mEntry.wpaAuth = WPA_AUTH_PSK;
					change_flag = 1;
				}
			}
			if(change_flag){
				mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, &mEntry, j);
				change_update_flag = 1;
			}
		}
	}
	if(change_update_flag){
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
	}
	return;
}
//sync when MIB_MBSSIB_TBL change
void sync_WLANForISP(int ssid_idx, MIB_CE_MBSSIB_T *Entry)
{
	int k, total = mib_chain_total(MIB_WLANFORISP_TBL);
	MIB_WLANFORISP_T wEntry;
	int change_flag=0;

	for(k=0; k<total; k++){
		if(mib_chain_get(MIB_WLANFORISP_TBL, k, &wEntry)==0)
			continue;
		if(wEntry.SSID_IDX == ssid_idx+1){
			if(strcmp(Entry->ssid, wEntry.SSID)){
				strcpy(wEntry.SSID, Entry->ssid);
				change_flag = 1;
			}
			if(Entry->encrypt==WIFI_SEC_NONE || Entry->encrypt==WIFI_SEC_WEP){
				if(Entry->enable1X == 0){
					Entry->enable1X = 1;
				}
				if(Entry->wpaAuth == WPA_AUTH_AUTO){
					Entry->wpaAuth = WPA_AUTH_PSK;
				}
			}
			else if(Entry->encrypt>=WIFI_SEC_WPA){
				if(Entry->wpaAuth == WPA_AUTH_PSK){
					Entry->wpaAuth = WPA_AUTH_AUTO;
				}
				if(Entry->enable1X == 1){
					Entry->enable1X = 0;
				}
			}
			break;
		}
	}
	if(change_flag)
		mib_chain_update(MIB_WLANFORISP_TBL, &wEntry, k);
}
int getWLANForISP_ifname(char *ifname, MIB_WLANFORISP_T *wlan_isp_entry)
{
	int entry_total= mib_chain_total(MIB_IP_ROUTE_TBL);
	int i=0;
	MIB_CE_IP_ROUTE_T entry;

	for(i=0; i<entry_total; i++){
		if(mib_chain_get(MIB_IP_ROUTE_TBL, i, &entry)==0){
			continue;
		}
		if(*((unsigned long *)wlan_isp_entry->RadiusServer)== *((unsigned long *)entry.destID)){
			if(ifGetName(entry.ifIndex, ifname, 16)){
				//printf("%s %d %s\n", __func__, __LINE__, ifname);
				return 0;
			}
			else{
				printf("%s %d ifGetName failed!\n",__func__, __LINE__);
				return -1;
			}
		}
	}
	printf("%s %d IP_ROUTE_TBL not found!\n",__func__, __LINE__);
	return -1;
}
int rtk_wlan_get_wlanforisp_entry(int wlanIdx, int vwlan_idx, MIB_WLANFORISP_Tp pEntry)
{
	int i, total = mib_chain_total(MIB_WLANFORISP_TBL);
	int ssid_idx = wlanIdx*(1+WLAN_MBSSID_NUM)+(1+vwlan_idx);

	for(i=0; i<total; i++){
		if(mib_chain_get(MIB_WLANFORISP_TBL, i, pEntry)==0)
			continue;
		if(pEntry->SSID_IDX == ssid_idx){
			return 0;
		}
	}
	return -1;
}
#endif


#ifdef WLAN_WPS
#define MIB_GET(id, val, val_len) do { \
		if (0==mib_get_s(id, (void *)val, val_len)) { \
		} \
	} while (0)

void sync_wps_config_mib()
{
	MIB_CE_MBSSIB_T Entry;
	int i, orig_idx;

	orig_idx = wlan_idx;
#ifdef WLAN1_QTN
	for(i=0; i<2; i++)
#else
	for(i=0; i<NUM_WLAN_INTERFACE; i++)
#endif
	{
		wlan_idx = i;
		if(mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry) == 0)
			printf("\n %s %d\n", __func__, __LINE__);
		mib_get_s(MIB_WSC_CONFIGURED, &Entry.wsc_configured, sizeof(Entry.wsc_configured));
		mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, 0);
	}
	wlan_idx = orig_idx;
}

void update_wps_from_mibtable()
{
	MIB_CE_MBSSIB_T Entry;
	int i, orig_idx;

#ifdef WLAN_DUALBAND_CONCURRENT
	orig_idx = wlan_idx;
	for(i=0;i<NUM_WLAN_INTERFACE;i++){
		wlan_idx = i;
#endif

		if(mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry) == 0)
			printf("\n %s %d\n", __func__, __LINE__);

		mib_get_s(MIB_WLAN_SSID, Entry.ssid, sizeof(Entry.ssid));
		mib_get_s(MIB_WLAN_ENCRYPT, &Entry.encrypt, sizeof(Entry.encrypt));
		mib_get_s(MIB_WLAN_AUTH_TYPE, &Entry.authType, sizeof(Entry.authType));
		mib_get_s(MIB_WSC_AUTH, &Entry.wsc_auth, sizeof(Entry.wsc_auth));
		mib_get_s(MIB_WLAN_WPA_AUTH, &Entry.wpaAuth, sizeof(Entry.wpaAuth));
		mib_get_s(MIB_WLAN_WPA_PSK_FORMAT, &Entry.wpaPSKFormat, sizeof(Entry.wpaPSKFormat));
		mib_get_s(MIB_WLAN_WPA_PSK, Entry.wpaPSK, sizeof(Entry.wpaPSK));
		mib_get_s(MIB_WSC_PSK, Entry.wscPsk, sizeof(Entry.wscPsk));
		mib_get_s(MIB_WLAN_WPA_CIPHER_SUITE, &Entry.unicastCipher, sizeof(Entry.unicastCipher));
		mib_get_s(MIB_WLAN_WPA2_CIPHER_SUITE, &Entry.wpa2UnicastCipher, sizeof(Entry.wpa2UnicastCipher));
		mib_get_s(MIB_WLAN_WEP, &Entry.wep, sizeof(Entry.wep));
		mib_get_s(MIB_WLAN_WEP_DEFAULT_KEY, &Entry.wepDefaultKey, sizeof(Entry.wepDefaultKey));
		mib_get_s(MIB_WLAN_WEP64_KEY1, Entry.wep64Key1, sizeof(Entry.wep64Key1));
		mib_get_s(MIB_WLAN_WEP64_KEY2, Entry.wep64Key2, sizeof(Entry.wep64Key2));
		mib_get_s(MIB_WLAN_WEP64_KEY3, Entry.wep64Key3, sizeof(Entry.wep64Key3));
		mib_get_s(MIB_WLAN_WEP64_KEY4, Entry.wep64Key4, sizeof(Entry.wep64Key4));
		mib_get_s(MIB_WLAN_WEP128_KEY1, Entry.wep128Key1, sizeof(Entry.wep128Key1));
		mib_get_s(MIB_WLAN_WEP128_KEY2, Entry.wep128Key2, sizeof(Entry.wep128Key2));
		mib_get_s(MIB_WLAN_WEP128_KEY3, Entry.wep128Key3, sizeof(Entry.wep128Key3));
		mib_get_s(MIB_WLAN_WEP128_KEY4, Entry.wep128Key4, sizeof(Entry.wep128Key4));
		mib_get_s(MIB_WLAN_WEP_KEY_TYPE, &Entry.wepKeyType, sizeof(Entry.wepKeyType));		// wep Key Format
		mib_get_s(MIB_WSC_ENC, &Entry.wsc_enc, sizeof(Entry.wsc_enc));

		mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, 0);
#ifdef WLAN_DUALBAND_CONCURRENT
	}
	wlan_idx = orig_idx;
#endif
	sync_wps_config_mib();
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);

}

void update_wps_mib()
{
	MIB_CE_MBSSIB_T Entry;
	if(mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry) == 0)
		printf("\n %s %d\n", __func__, __LINE__);

	mib_set(MIB_WLAN_ENCRYPT, &Entry.encrypt);
	mib_set(MIB_WLAN_ENABLE_1X, &Entry.enable1X);
	mib_set(MIB_WLAN_WEP, &Entry.wep);
	mib_set(MIB_WLAN_WPA_AUTH, &Entry.wpaAuth);
	mib_set(MIB_WLAN_WPA_PSK_FORMAT, &Entry.wpaPSKFormat);
	mib_set(MIB_WLAN_WPA_PSK, Entry.wpaPSK);
	mib_set(MIB_WLAN_RS_PORT, &Entry.rsPort);
	mib_set(MIB_WLAN_RS_IP, Entry.rsIpAddr);

	mib_set(MIB_WLAN_RS_PASSWORD, Entry.rsPassword);
	mib_set(MIB_WLAN_DISABLED, &Entry.wlanDisabled);
	mib_set(MIB_WLAN_SSID, Entry.ssid);
	mib_set(MIB_WLAN_MODE, &Entry.wlanMode);
	mib_set(MIB_WLAN_AUTH_TYPE, &Entry.authType);

	mib_set(MIB_WLAN_WPA_CIPHER_SUITE, &Entry.unicastCipher);
	mib_set(MIB_WLAN_WPA2_CIPHER_SUITE, &Entry.wpa2UnicastCipher);
	mib_set(MIB_WLAN_WPA_GROUP_REKEY_TIME, &Entry.wpaGroupRekeyTime);

	mib_set(MIB_WLAN_WEP_KEY_TYPE, &Entry.wepKeyType);      // wep Key Format
	mib_set(MIB_WLAN_WEP_DEFAULT_KEY, &Entry.wepDefaultKey);
	mib_set(MIB_WLAN_WEP64_KEY1, Entry.wep64Key1);
	mib_set(MIB_WLAN_WEP64_KEY2, Entry.wep64Key2);
	mib_set(MIB_WLAN_WEP64_KEY3, Entry.wep64Key3);
	mib_set(MIB_WLAN_WEP64_KEY4, Entry.wep64Key4);
	mib_set(MIB_WLAN_WEP128_KEY1, Entry.wep128Key1);
	mib_set(MIB_WLAN_WEP128_KEY2, Entry.wep128Key2);
	mib_set(MIB_WLAN_WEP128_KEY3, Entry.wep128Key3);
	mib_set(MIB_WLAN_WEP128_KEY4, Entry.wep128Key4);
#if defined(WLAN_BAND_CONFIG_MBSSID)
	mib_set(MIB_WLAN_BAND, &Entry.wlanBand);
#endif
	mib_set(MIB_WSC_DISABLE, &Entry.wsc_disabled);
	//mib_set(MIB_WSC_CONFIGURED, &Entry.wsc_configured);
	mib_set(MIB_WSC_UPNP_ENABLED, &Entry.wsc_upnp_enabled);
	mib_set(MIB_WSC_AUTH, &Entry.wsc_auth);
	mib_set(MIB_WSC_ENC, &Entry.wsc_enc);
	mib_set(MIB_WSC_PSK, Entry.wscPsk);

}


#ifdef WLAN_WPS_VAP
int update_wps_configured(int reset_flag)
{
	char is_configured, encrypt1, encrypt2, auth, disabled, iVal, mode, format, encryptwps;
	char ssid1[100], ssid2[100];
	unsigned char tmpbuf[100];
#ifdef WPS20
	unsigned char wpsUseVersion;
#endif
	MIB_CE_MBSSIB_T Entry, Entry_bk;
#ifdef WLAN_USE_DEFAULT_SSID_PSK
	unsigned char phyband=0;
#endif
	int i, j, orig_wlan_idx;
	int lockfd;

	LOCK_WLAN();
	orig_wlan_idx = wlan_idx;

	for(j=0; j<NUM_WLAN_INTERFACE; j++)
	{
		wlan_idx = j;
		//fprintf(stderr, "update_wps_configured(%d)\n", reset_flag);
		for (i=0; i<=WLAN_MBSSID_NUM; i++) {
			wlan_getEntry(&Entry, i);
			if (Entry.wlanDisabled)
				continue;
			mib_chain_getDef(MIB_MBSSIB_TBL, i, &Entry_bk);
#ifdef WLAN_USE_DEFAULT_SSID_PSK
			if(i==0){
				mib_get(MIB_DEFAULT_WLAN_SSID, (void *)ssid2);
				mib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband);
				if(phyband == PHYBAND_5G)
					strcat(ssid2, "-5G");
			}
			else
#endif
			{
				strcpy(ssid2, Entry_bk.ssid);
			}
			//printf("ssid %s\n", Entry_bk.ssid);

			is_configured = Entry.wsc_configured;
			MIB_GET(MIB_WLAN_MODE, (void *)&mode, sizeof(mode));

			if (!is_configured && mode == AP_MODE) {
				//MIB_GET(MIB_WLAN_SSID, (void *)ssid1, sizeof(ssid1));
				//mib_getDef(MIB_WLAN_SSID, (void *)ssid2);
				if (strcmp(Entry.ssid, ssid2)) { // Magician: Fixed parsing error by Source Insight
					is_configured = 1;

					Entry.wsc_configured = is_configured;

					MIB_GET(MIB_WSC_CONFIG_BY_EXT_REG, (void *)&iVal, sizeof(iVal));
					if (is_configured && iVal == 0) {
						iVal = 1;
						mib_set(MIB_WSC_MANUAL_ENABLED, (void *)&iVal);
					}
					//return;
				}

				//MIB_GET(MIB_WLAN_ENCRYPT, (void *)&encrypt1, sizeof(encrypt1));
				//mib_getDef(MIB_WLAN_ENCRYPT, (void *)&encrypt2);

				if (Entry.encrypt != Entry_bk.encrypt) {
					is_configured = 1;
					Entry.wsc_configured = is_configured;
				}
			}
			//mib_chain_update(MIB_MBSSIB_TBL, &Entry, 0);

			//MIB_GET(MIB_WSC_DISABLE, (void *)&disabled, sizeof(disabled));
#ifdef WPS20
			MIB_GET(MIB_WSC_VERSION, (void *)&wpsUseVersion, sizeof(wpsUseVersion));
			if (wpsUseVersion == 0 && Entry.wsc_disabled)
				continue;
#else
			if (Entry.wsc_disabled)
				continue;
#endif

			//MIB_GET(MIB_WLAN_ENCRYPT, (void *)&encrypt1, sizeof(encrypt1));
			encrypt1 = Entry.encrypt;
			if (encrypt1 == WIFI_SEC_NONE) {
				auth = WSC_AUTH_OPEN;
				encrypt2 = WSC_ENCRYPT_NONE;
			}
			else if (encrypt1 == WIFI_SEC_WEP) {
				auth = WSC_AUTH_OPEN;
				encrypt2 = WSC_ENCRYPT_WEP;
			}
			else if (encrypt1 == WIFI_SEC_WPA) {
				//MIB_GET(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&encryptwps, sizeof(encryptwps));
				encryptwps = Entry.unicastCipher;
				auth = WSC_AUTH_WPAPSK;
				encrypt2 = WSC_ENCRYPT_TKIPAES;
				if (encryptwps == WPA_CIPHER_AES)
					encrypt2 = WSC_ENCRYPT_AES;
				if (encryptwps == WPA_CIPHER_TKIP)
					encrypt2 = WSC_ENCRYPT_TKIP;
				if (encryptwps == WPA_CIPHER_MIXED)
					encrypt2 = WSC_ENCRYPT_TKIPAES;
			}
			else if (encrypt1 == WIFI_SEC_WPA2) {
				//MIB_GET(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&encryptwps, sizeof(encryptwps));
				encryptwps = Entry.wpa2UnicastCipher;
				auth = WSC_AUTH_WPA2PSK;
				encrypt2 = WSC_ENCRYPT_TKIPAES;
				if (encryptwps == WPA_CIPHER_AES)
					encrypt2 = WSC_ENCRYPT_AES;
				if (encryptwps == WPA_CIPHER_TKIP)
					encrypt2 = WSC_ENCRYPT_TKIP;
				if (encryptwps == WPA_CIPHER_MIXED)
					encrypt2 = WSC_ENCRYPT_TKIPAES;
			}
#ifdef WLAN_WPA3
			else if (encrypt1 == WIFI_SEC_WPA3) {
				//MIB_GET(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&encryptwps, sizeof(encryptwps));
				encryptwps = Entry.wpa2UnicastCipher;
				auth = WSC_AUTH_WPA2PSK;
				encrypt2 = WSC_ENCRYPT_TKIPAES;
				if (encryptwps == WPA_CIPHER_AES)
					encrypt2 = WSC_ENCRYPT_AES;
				if (encryptwps == WPA_CIPHER_TKIP)
					encrypt2 = WSC_ENCRYPT_TKIP;
				if (encryptwps == WPA_CIPHER_MIXED)
					encrypt2 = WSC_ENCRYPT_TKIPAES;
			}
			else if (encrypt1 == WIFI_SEC_WPA2_WPA3_MIXED) {
				//MIB_GET(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&encryptwps, sizeof(encryptwps));
				encryptwps = Entry.wpa2UnicastCipher;
				auth = WSC_AUTH_WPA2PSK;
				encrypt2 = WSC_ENCRYPT_TKIPAES;
				if (encryptwps == WPA_CIPHER_AES)
					encrypt2 = WSC_ENCRYPT_AES;
				if (encryptwps == WPA_CIPHER_TKIP)
					encrypt2 = WSC_ENCRYPT_TKIP;
				if (encryptwps == WPA_CIPHER_MIXED)
					encrypt2 = WSC_ENCRYPT_TKIPAES;
			}
#endif
			else {
				auth = WSC_AUTH_WPA2PSKMIXED;
				encrypt2 = WSC_ENCRYPT_TKIPAES;
		// sync with ap team at 2011-04-25, When mixed mode, if no WPA2-AES, try to use WPA-AES or WPA2-TKIP
					//MIB_GET(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&encrypt1, sizeof(encrypt1));
					encrypt1 = Entry.unicastCipher;
					//MIB_GET(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&iVal, sizeof(iVal));
					iVal = Entry.wpa2UnicastCipher;
				if (encrypt1 == iVal) {	//cathy, fix wps web bug when encryption is: WPA-AES+WPA2-AES / WPA-TKIP+WPA2-AES
					if (encrypt1 == WPA_CIPHER_TKIP)
						encrypt2 = WSC_ENCRYPT_TKIP;
					else if (encrypt1 == WPA_CIPHER_AES)
						encrypt2 = WSC_ENCRYPT_AES;
				}
				else if (!(iVal & WPA_CIPHER_AES)) {
					if (encrypt1 & WPA_CIPHER_AES) {
						encrypt2 = WSC_ENCRYPT_AES;
					}
				}
		//-------------------------------------------- david+2008-01-03
			}
			//mib_set(MIB_WSC_AUTH, (void *)&auth);
			Entry.wsc_auth = auth;
			//mib_set(MIB_WSC_ENC, (void *)&encrypt2);
			Entry.wsc_enc = encrypt2;

			//MIB_GET(MIB_WLAN_ENCRYPT, (void *)&encrypt1, sizeof(encrypt1));
				encrypt1 = Entry.encrypt;
			if (encrypt1 == WIFI_SEC_WPA || encrypt1 == WIFI_SEC_WPA2
				|| encrypt1 == WIFI_SEC_WPA2_MIXED
#ifdef WLAN_WPA3
				|| encrypt1 == WIFI_SEC_WPA3 || encrypt1 == WIFI_SEC_WPA2_WPA3_MIXED
#endif
				) {
				//MIB_GET(MIB_WLAN_WPA_AUTH, (void *)&format, sizeof(format));
				format = Entry.wpaAuth;
				if (format & 2) { // PSK
					//MIB_GET(MIB_WLAN_WPA_PSK, (void *)tmpbuf, sizeof(tmpbuf));
					strcpy(tmpbuf, Entry.wpaPSK);
					//mib_set(MIB_WSC_PSK, (void *)tmpbuf);
					strcpy(Entry.wscPsk, tmpbuf);
				}
			}
			if (reset_flag) {
				reset_flag = 0;
				mib_set(MIB_WSC_CONFIG_BY_EXT_REG, (void *)&reset_flag);
			}

			MIB_GET(MIB_WSC_CONFIG_BY_EXT_REG, (void *)&iVal, sizeof(iVal));
			if (is_configured && iVal == 0) {
				iVal = 1;
				mib_set(MIB_WSC_MANUAL_ENABLED, (void *)&iVal);
			}
			iVal = 0;
			if (mode == AP_MODE || mode == AP_WDS_MODE) {
				if (encrypt1 == WIFI_SEC_WEP || encrypt1 == WIFI_SEC_NONE) {
					//MIB_GET(MIB_WLAN_ENABLE_1X, (void *)&encrypt2, sizeof(encrypt2));
					encrypt2 = Entry.enable1X;
					if (encrypt2)
						iVal = 1;
				}
				else {
					//MIB_GET(MIB_WLAN_WPA_AUTH, (void *)&encrypt2, sizeof(encrypt2));
					encrypt2 = Entry.wpaAuth;
					if (encrypt2 == WPA_AUTH_AUTO)
						iVal = 1;
				}
			}
			else if (mode == CLIENT_MODE || mode == AP_WDS_MODE)
				iVal = 1;
			if (iVal) {
				iVal = 0;
				mib_set(MIB_WSC_MANUAL_ENABLED, (void *)&iVal);
				//mib_set(MIB_WSC_CONFIGURED, (void *)&iVal);
				Entry.wsc_configured = iVal;
				mib_set(MIB_WSC_CONFIG_BY_EXT_REG, (void *)&iVal);
			}
			mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, i);
			//sync_wps_config_mib();
		}
	}

	wlan_idx = orig_wlan_idx;
	UNLOCK_WLAN();

	return 0;
}
#else
int update_wps_configured(int reset_flag)
{
	char is_configured, encrypt1, encrypt2, auth, disabled, iVal, mode, format, encryptwps;
	char ssid1[100], ssid2[100];
	unsigned char tmpbuf[100];
#ifdef WPS20
	unsigned char wpsUseVersion;
#endif
	MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_USE_DEFAULT_SSID_PSK
	unsigned char phyband=0;
	MIB_CE_MBSSIB_T Entry_bk;
#endif

#ifdef CONFIG_E8B
	int i=0, orig_wlan_idx;
	int lockfd;

	LOCK_WLAN();
	orig_wlan_idx = wlan_idx;

	for(i=0; i<NUM_WLAN_INTERFACE; i++)
	{
		wlan_idx = i;
#endif

	if(!mib_chain_get(MIB_MBSSIB_TBL, 0, &Entry))
#ifdef CONFIG_E8B
		continue;
#else
		return 0;
#endif

	update_wps_mib();

	//fprintf(stderr, "update_wps_configured(%d)\n", reset_flag);

	MIB_GET(MIB_WSC_CONFIGURED, (void *)&is_configured, sizeof(is_configured));
	MIB_GET(MIB_WLAN_MODE, (void *)&mode, sizeof(mode));

	if (!is_configured && mode == AP_MODE) {
		MIB_GET(MIB_WLAN_SSID, (void *)ssid1, sizeof(ssid1));
#ifdef WLAN_USE_DEFAULT_SSID_PSK
		mib_get(MIB_DEFAULT_WLAN_SSID, (void *)ssid2);
		mib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband);
		if(phyband == PHYBAND_5G)
			strcat(ssid2, "-5G");
#else
		mib_getDef(MIB_WLAN_SSID, (void *)ssid2);
#endif
		if (strcmp(ssid1, ssid2)) { // Magician: Fixed parsing error by Source Insight
			is_configured = 1;
			mib_set(MIB_WSC_CONFIGURED, (void *)&is_configured);
			Entry.wsc_configured = is_configured;

			MIB_GET(MIB_WSC_CONFIG_BY_EXT_REG, (void *)&iVal, sizeof(iVal));
			if (is_configured && iVal == 0) {
				iVal = 1;
				mib_set(MIB_WSC_MANUAL_ENABLED, (void *)&iVal);
			}
			//return;
		}

		MIB_GET(MIB_WLAN_ENCRYPT, (void *)&encrypt1, sizeof(encrypt1));
#ifdef WLAN_USE_DEFAULT_SSID_PSK
		mib_chain_getDef(MIB_MBSSIB_TBL, 0, &Entry_bk);
		encrypt2 = Entry_bk.encrypt;
#else
		mib_getDef(MIB_WLAN_ENCRYPT, (void *)&encrypt2);
#endif

		if (encrypt1 != encrypt2) {
			is_configured = 1;
			mib_set(MIB_WSC_CONFIGURED, (void *)&is_configured);
			Entry.wsc_configured = is_configured;
		}
	}
	mib_chain_update(MIB_MBSSIB_TBL, &Entry, 0);

	MIB_GET(MIB_WSC_DISABLE, (void *)&disabled, sizeof(disabled));
#ifdef WPS20
	MIB_GET(MIB_WSC_VERSION, (void *)&wpsUseVersion, sizeof(wpsUseVersion));
	if (wpsUseVersion == 0 && disabled)
#ifdef CONFIG_E8B
		continue;
#else
		return 0;
#endif
#else
	if (disabled)
#ifdef CONFIG_E8B
		continue;
#else
		return 0;
#endif
#endif

	MIB_GET(MIB_WLAN_ENCRYPT, (void *)&encrypt1, sizeof(encrypt1));
	if (encrypt1 == WIFI_SEC_NONE) {
		auth = WSC_AUTH_OPEN;
		encrypt2 = WSC_ENCRYPT_NONE;
	}
	else if (encrypt1 == WIFI_SEC_WEP) {
		auth = WSC_AUTH_OPEN;
		encrypt2 = WSC_ENCRYPT_WEP;
	}
	else if (encrypt1 == WIFI_SEC_WPA) {
		MIB_GET(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&encryptwps, sizeof(encryptwps));
		auth = WSC_AUTH_WPAPSK;
		encrypt2 = WSC_ENCRYPT_TKIPAES;
		if (encryptwps == WPA_CIPHER_AES)
			encrypt2 = WSC_ENCRYPT_AES;
		if (encryptwps == WPA_CIPHER_TKIP)
			encrypt2 = WSC_ENCRYPT_TKIP;
		if (encryptwps == WPA_CIPHER_MIXED)
			encrypt2 = WSC_ENCRYPT_TKIPAES;
	}
	else if (encrypt1 == WIFI_SEC_WPA2) {
		MIB_GET(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&encryptwps, sizeof(encryptwps));
		auth = WSC_AUTH_WPA2PSK;
		encrypt2 = WSC_ENCRYPT_TKIPAES;
		if (encryptwps == WPA_CIPHER_AES)
			encrypt2 = WSC_ENCRYPT_AES;
		if (encryptwps == WPA_CIPHER_TKIP)
			encrypt2 = WSC_ENCRYPT_TKIP;
		if (encryptwps == WPA_CIPHER_MIXED)
			encrypt2 = WSC_ENCRYPT_TKIPAES;
	}
#ifdef WLAN_WPA3
	else if (encrypt1 == WIFI_SEC_WPA3) {
		MIB_GET(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&encryptwps, sizeof(encryptwps));
		auth = WSC_AUTH_WPA2PSK;
		encrypt2 = WSC_ENCRYPT_TKIPAES;
		if (encryptwps == WPA_CIPHER_AES)
			encrypt2 = WSC_ENCRYPT_AES;
		if (encryptwps == WPA_CIPHER_TKIP)
			encrypt2 = WSC_ENCRYPT_TKIP;
		if (encryptwps == WPA_CIPHER_MIXED)
			encrypt2 = WSC_ENCRYPT_TKIPAES;
	}
	else if (encrypt1 == WIFI_SEC_WPA2_WPA3_MIXED) {
		MIB_GET(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&encryptwps, sizeof(encryptwps));
		auth = WSC_AUTH_WPA2PSK;
		encrypt2 = WSC_ENCRYPT_TKIPAES;
		if (encryptwps == WPA_CIPHER_AES)
			encrypt2 = WSC_ENCRYPT_AES;
		if (encryptwps == WPA_CIPHER_TKIP)
			encrypt2 = WSC_ENCRYPT_TKIP;
		if (encryptwps == WPA_CIPHER_MIXED)
			encrypt2 = WSC_ENCRYPT_TKIPAES;
	}
#endif
	else {
		auth = WSC_AUTH_WPA2PSKMIXED;
		encrypt2 = WSC_ENCRYPT_TKIPAES;
// sync with ap team at 2011-04-25, When mixed mode, if no WPA2-AES, try to use WPA-AES or WPA2-TKIP
		MIB_GET(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&encrypt1, sizeof(encrypt1));
		MIB_GET(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&iVal, sizeof(iVal));
		if (encrypt1 == iVal) {	//cathy, fix wps web bug when encryption is: WPA-AES+WPA2-AES / WPA-TKIP+WPA2-AES
			if (encrypt1 == WPA_CIPHER_TKIP)
				encrypt2 = WSC_ENCRYPT_TKIP;
			else if (encrypt1 == WPA_CIPHER_AES)
				encrypt2 = WSC_ENCRYPT_AES;
		}
		else if (!(iVal & WPA_CIPHER_AES)) {
			if (encrypt1 & WPA_CIPHER_AES) {
				encrypt2 = WSC_ENCRYPT_AES;
			}
		}
//-------------------------------------------- david+2008-01-03
	}
	mib_set(MIB_WSC_AUTH, (void *)&auth);
	Entry.wsc_auth = auth;
	mib_set(MIB_WSC_ENC, (void *)&encrypt2);
	Entry.wsc_enc = encrypt2;

	MIB_GET(MIB_WLAN_ENCRYPT, (void *)&encrypt1, sizeof(encrypt1));
	if (encrypt1 == WIFI_SEC_WPA || encrypt1 == WIFI_SEC_WPA2
		|| encrypt1 == WIFI_SEC_WPA2_MIXED
#ifdef WLAN_WPA3
		|| encrypt1 == WIFI_SEC_WPA3 || encrypt1 == WIFI_SEC_WPA2_WPA3_MIXED
#endif
		) {
		MIB_GET(MIB_WLAN_WPA_AUTH, (void *)&format, sizeof(format));
		if (format & 2) { // PSK
			MIB_GET(MIB_WLAN_WPA_PSK, (void *)tmpbuf, sizeof(tmpbuf));
			mib_set(MIB_WSC_PSK, (void *)tmpbuf);
			strcpy(Entry.wscPsk, tmpbuf);
		}
	}
	if (reset_flag) {
		reset_flag = 0;
		mib_set(MIB_WSC_CONFIG_BY_EXT_REG, (void *)&reset_flag);
	}

	MIB_GET(MIB_WSC_CONFIG_BY_EXT_REG, (void *)&iVal, sizeof(iVal));
	if (is_configured && iVal == 0) {
		iVal = 1;
		mib_set(MIB_WSC_MANUAL_ENABLED, (void *)&iVal);
	}
	iVal = 0;
	if (mode == AP_MODE || mode == AP_WDS_MODE) {
#if 0
		if (encrypt1 == WIFI_SEC_WEP || encrypt1 == WIFI_SEC_NONE) {
			MIB_GET(MIB_WLAN_ENABLE_1X, (void *)&encrypt2, sizeof(encrypt2));
			if (encrypt2)
				iVal = 1;
		}
		else {
			MIB_GET(MIB_WLAN_WPA_AUTH, (void *)&encrypt2, sizeof(encrypt2));
			if (encrypt2 == WPA_AUTH_AUTO)
				iVal = 1;
		}
#endif
	}
	else if (mode == CLIENT_MODE || mode == AP_WDS_MODE)
		iVal = 1;
	if (iVal) {
		iVal = 0;
		mib_set(MIB_WSC_MANUAL_ENABLED, (void *)&iVal);
		mib_set(MIB_WSC_CONFIGURED, (void *)&iVal);
		Entry.wsc_configured = iVal;
		mib_set(MIB_WSC_CONFIG_BY_EXT_REG, (void *)&iVal);
	}
	mib_chain_update(MIB_MBSSIB_TBL, &Entry, 0);

#ifdef CONFIG_E8B
	}
	wlan_idx = orig_wlan_idx;
	UNLOCK_WLAN();
#endif

	sync_wps_config_mib();

	return 0;
}
#endif
#endif //WLAN_WPS

int setupWLanQos(char *argv[6])
{
	//for compile
	return 0;
}

unsigned int check_wlan_module(void)
{
	unsigned char vChar;
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
	mib_local_mapping_get(MIB_WLAN_MODULE_DISABLED, 0, (void *)&vChar);
	//if you want to check wlan1 please add following mib get and do your checking
	//you should avoid to use this function for YUEME 3.0 spec since wlan0/1 can be enable separately
	//mib_local_mapping_get(MIB_WLAN_DISABLED, 1, (void *)&vChar);
	//todo
#else
	mib_get_s(MIB_WIFI_MODULE_DISABLED, (void *)&vChar, sizeof(vChar));
#endif
	if(vChar)
		return 0;
	else
		return 1;
}

void launch_hapd_by_cmd(int if_idx)
{
	char global_ctrl_path[128]={0}, tmp_cmd[128]={0}, coex_2g_40m_cmd[128] = {0}, pid_path[64]={0};
	unsigned char auto_ch = 0;
	char conf_filename[128]={0}, pid_filename[128]={0};
	MIB_CE_MBSSIB_T Entry;
#ifdef CONFIG_USER_RTK_SYSTEMD
#ifdef CONFIG_USER_HAPD
	char cpidfilename[128]={0};
	int hostapdcli_pid = 0;
#ifdef WLAN_MBSSID
	int j=0;
	char ifname[IFNAMSIZ]={0};
#endif
#endif
#endif

	printf("launch hapd: wlan_idx = %d \n",if_idx);
	snprintf(global_ctrl_path, sizeof(global_ctrl_path), "%s.%s", HOSTAPD_GLOBAL_CTRL_PATH, WLANIF[if_idx]);

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, 0, (void *)&Entry);
	if(Entry.is_ax_support == 1)
	{
#ifdef CONFIG_RTK_DEV_AP
	 	mib_get_s(MIB_WLAN_AUTO_CHAN_ENABLED, (void *)&auto_ch, sizeof(auto_ch));
#else
		auto_ch = 1; //always echo 1 to coex_2g_40m
#endif
		if(auto_ch) {
			printf("%s(%d) echo 1 into %s coex_2g_40m\n", __FUNCTION__, __LINE__, WLANIF[wlan_idx]);
			snprintf(coex_2g_40m_cmd, sizeof(coex_2g_40m_cmd), "/bin/echo 1 > "LINK_FILE_NAME_FMT, WLANIF[wlan_idx], "coex_2g_40m");
			system(coex_2g_40m_cmd);
		}
		else {
			printf("%s(%d) echo 0 into %s coex_2g_40m\n", __FUNCTION__, __LINE__, WLANIF[wlan_idx]);
			snprintf(coex_2g_40m_cmd, sizeof(coex_2g_40m_cmd), "/bin/echo 0 > "LINK_FILE_NAME_FMT, WLANIF[wlan_idx], "coex_2g_40m");
			system(coex_2g_40m_cmd);
		}
	}

	snprintf(conf_filename, sizeof(conf_filename), HAPD_CONF_PATH_NAME, WLANIF[if_idx]);
	snprintf(pid_filename, sizeof(pid_filename), HAPD_PID_PATH_NAME, WLANIF[if_idx]);
	va_cmd("/bin/hostapd", 6, 1, conf_filename, "-g", global_ctrl_path, "-P", pid_filename, "-B");
	if(rtk_get_interface_flag(WLANIF[if_idx], TIMER_COUNT, IS_RUN) == 0)
		printf("launch hapd: wlan_idx = %d failed\n",if_idx);

#ifdef CONFIG_USER_RTK_SYSTEMD
	if(Entry.wlanDisabled == 0)
	{
#ifdef CONFIG_USER_HAPD
		memset(cpidfilename,0,sizeof(cpidfilename));
		snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.general", HOSTAPDCLIPID, WLANIF[if_idx]);
		hostapdcli_pid = read_pid(cpidfilename);
		if(hostapdcli_pid<=0){
			va_niced_cmd(HOSTAPD_CLI, 7, 0, "-a", HOSTAPD_GENERAL_SCRIPT, "-B", "-P", cpidfilename, "-i", WLANIF[if_idx]);
		}
#ifdef WLAN_WPS_HAPD
		memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
		mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, 0, (void *)&Entry);
		if(Entry.wsc_disabled == 0)
		{
			memset(cpidfilename,0,sizeof(cpidfilename));
			snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", HOSTAPDCLIPID, WLANIF[if_idx]);
			hostapdcli_pid = read_pid(cpidfilename);
			if(hostapdcli_pid<=0){
				va_niced_cmd(HOSTAPD_CLI, 7, 0, "-a", HOSTAPD_WPS_SCRIPT, "-B", "-P", cpidfilename, "-i", WLANIF[if_idx]);
			}
		}
#endif
#ifdef CONFIG_USER_LANNETINFO
		memset(cpidfilename,0,sizeof(cpidfilename));
		snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.lannetinfo", HOSTAPDCLIPID, WLANIF[if_idx]);
		hostapdcli_pid = read_pid(cpidfilename);
		if(hostapdcli_pid<=0){
			va_niced_cmd(HOSTAPD_CLI, 7, 0, "-a", LANNETINFO_ACTION, "-B", "-P", cpidfilename, "-i", WLANIF[if_idx]);
		}
#endif
	}
//for vif
#if defined(WLAN_MBSSID)
	for(j=WLAN_VAP_ITF_INDEX; j<(WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM); j++){
		if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, j, &Entry) == 1){
			if(Entry.wlanDisabled == 0){
				rtk_wlan_get_ifname(if_idx, j, ifname);
				memset(cpidfilename,0,sizeof(cpidfilename));
				snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.general", HOSTAPDCLIPID, ifname);
				hostapdcli_pid = read_pid(cpidfilename);
				if(hostapdcli_pid<=0){
					va_niced_cmd(HOSTAPD_CLI, 7, 0, "-a", HOSTAPD_GENERAL_SCRIPT, "-B", "-P", cpidfilename, "-i", ifname);
				}
#ifdef WLAN_WPS_VAP
				if(Entry.wsc_disabled == 0)
				{
					memset(cpidfilename,0,sizeof(cpidfilename));
					snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", HOSTAPDCLIPID, ifname);
					hostapdcli_pid = read_pid(cpidfilename);
					if(hostapdcli_pid<=0){
						va_niced_cmd(HOSTAPD_CLI, 7, 0, "-a", HOSTAPD_WPS_SCRIPT, "-B", "-P", cpidfilename, "-i", ifname);
					}
				}
#endif
#ifdef CONFIG_USER_LANNETINFO
				memset(cpidfilename,0,sizeof(cpidfilename));
				snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.lannetinfo", HOSTAPDCLIPID, ifname);
				hostapdcli_pid = read_pid(cpidfilename);
				if(hostapdcli_pid<=0){
					va_niced_cmd(HOSTAPD_CLI, 7, 0, "-a", LANNETINFO_ACTION, "-B", "-P", cpidfilename, "-i", ifname);
				}
#endif
			}
		}
	}
#endif
#endif
#endif

}

#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
void launch_wpas_by_cmd(int if_idx, int index)
{
	char tmp_cmd[50]={0};
	char conf_filename[128]={0}, pid_filename[128]={0};
#ifdef CONFIG_USER_RTK_SYSTEMD
#ifdef CONFIG_USER_WPAS
	char cpidfilename[128]={0}, if_name[16]={0};
	int wpacli_pid=0;
#ifdef WLAN_WPS_WPAS
	MIB_CE_MBSSIB_T Entry;
#endif
#endif
#endif

	printf("launch wpas: wlan_idx = %d vwlan_idx = %d\n",if_idx, index);

#ifdef WLAN_UNIVERSAL_REPEATER
	if(index == WLAN_REPEATER_ITF_INDEX)
	{
		snprintf(conf_filename, sizeof(conf_filename), WPAS_CONF_PATH_VXD_NAME, WLANIF[if_idx]);
		snprintf(pid_filename, sizeof(pid_filename), WPAS_PID_PATH_VXD_NAME, WLANIF[if_idx]);
		va_cmd("/bin/wpa_supplicant", 11, 1, "-D", "nl80211", "-b", BRIF, "-i", WLAN_VXD_IF[if_idx], "-c", conf_filename, "-P", pid_filename, "-B");
		if(rtk_get_interface_flag(WLAN_VXD_IF[if_idx], TIMER_COUNT, IS_RUN) == 0)
			printf("launch hapd: wlan_idx = %d vxd failed\n",if_idx);
		snprintf(tmp_cmd, sizeof(tmp_cmd), "brctl addif %s %s", BRIF, WLAN_VXD_IF[if_idx]);
	}
	else
#endif
	{
		snprintf(conf_filename, sizeof(conf_filename), WPAS_CONF_PATH_NAME, WLANIF[if_idx]);
		snprintf(pid_filename, sizeof(pid_filename), WPAS_PID_PATH_NAME, WLANIF[if_idx]);
		va_cmd("/bin/wpa_supplicant", 11, 1, "-D", "nl80211", "-b", BRIF, "-i", WLANIF[if_idx], "-c", conf_filename, "-P", pid_filename, "-B");
		if(rtk_get_interface_flag(WLANIF[if_idx], TIMER_COUNT, IS_RUN) == 0)
			printf("launch hapd: wlan_idx = %d failed\n",if_idx);
		snprintf(tmp_cmd, sizeof(tmp_cmd), "brctl addif %s %s", BRIF, WLANIF[if_idx]);
	}

	system(tmp_cmd);


#ifdef CONFIG_USER_RTK_SYSTEMD
#ifdef CONFIG_USER_WPAS
	memset(cpidfilename,0,sizeof(cpidfilename));
	memset(if_name, 0, sizeof(if_name));
#ifdef WLAN_UNIVERSAL_REPEATER
	if(index == WLAN_REPEATER_ITF_INDEX)
			snprintf(if_name, sizeof(if_name), "%s", WLAN_VXD_IF[if_idx]);
	else
#endif
			snprintf(if_name, sizeof(if_name), "%s", WLANIF[if_idx]);
	snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.general", WPACLIPID, if_name);
	wpacli_pid = read_pid(cpidfilename);
	if(wpacli_pid<=0){
		va_niced_cmd(WPA_CLI, 7, 0, "-a", WPAS_GENERAL_SCRIPT, "-B", "-P", cpidfilename, "-i", if_name);
	}
#ifdef WLAN_WPS_WPAS
	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, index, (void *)&Entry);
	if(Entry.wsc_disabled == 0)
	{
#ifdef WLAN_UNIVERSAL_REPEATER
		if(index == WLAN_REPEATER_ITF_INDEX)
		{
			memset(cpidfilename,0,sizeof(cpidfilename));
			snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", WPACLIPID, WLAN_VXD_IF[if_idx]);
			wpacli_pid = read_pid(cpidfilename);
			if(wpacli_pid<=0){
				va_niced_cmd(WPA_CLI, 7, 0, "-a", WPAS_WPS_SCRIPT, "-B", "-P", cpidfilename, "-i", WLAN_VXD_IF[if_idx]);
			}
		}
		else
#endif
		{

			memset(cpidfilename,0,sizeof(cpidfilename));
			snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", WPACLIPID, WLANIF[if_idx]);
			wpacli_pid = read_pid(cpidfilename);
			if(wpacli_pid<=0){
				va_niced_cmd(WPA_CLI, 7, 0, "-a", WPAS_WPS_SCRIPT, "-B", "-P", cpidfilename, "-i", WLANIF[if_idx]);
			}
		}
	}
#endif
#endif
#endif
}

#endif

#if defined(WLAN_MBSSID_NUM) && (WLAN_MBSSID_NUM > 0)
void launch_hapd_vif_by_cmd(int if_idx, int vwlan_idx, int flag)
{
	char bss_config[256]={0}, buffer[256]={0}, global_ctrl_path[128]={0};
	char ifname[20]={0};
	MIB_CE_MBSSIB_T Entry;
	char conf_filename[128]={0}, pid_filename[128]={0};
#ifdef CONFIG_USER_RTK_SYSTEMD
#ifdef CONFIG_USER_HAPD
	char cpidfilename[128]={0};
	int hostapdcli_pid = 0;
#if defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
	char wps_ifname[16]={0};
#endif
#endif
#endif

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, vwlan_idx, (void *)&Entry);

	snprintf(global_ctrl_path, sizeof(global_ctrl_path), "%s.%s", HOSTAPD_GLOBAL_CTRL_PATH, WLANIF[if_idx]);
	if(flag)
	{
		rtk_wlan_get_ifname(if_idx, vwlan_idx, ifname);
		snprintf(bss_config, sizeof(bss_config), "/var/run/hapd_conf_%s", ifname);

		gen_hapd_vif_cfg(if_idx, vwlan_idx, bss_config);

#ifdef CONFIG_RTK_DEV_AP
#ifdef BAND_2G_ON_WLAN0
		if(if_idx == 0)
			snprintf(buffer, sizeof(buffer)-strlen(bss_config), "bss_config=phy1:%s", bss_config);
		else if(if_idx == 1)
			snprintf(buffer, sizeof(buffer)-strlen(bss_config), "bss_config=phy0:%s", bss_config);
#else
		snprintf(buffer, sizeof(buffer)-strlen(bss_config), "bss_config=phy%d:%s", if_idx, bss_config);
#endif
#else
		snprintf(buffer, sizeof(buffer)-strlen(bss_config), "bss_config=phy%c:%s", WLANIF[if_idx][4], bss_config);
#endif
		va_cmd(WPA_CLI, 5, 1, "-g", global_ctrl_path, "raw", "ADD", buffer);

#ifdef CONFIG_USER_RTK_SYSTEMD
#ifdef CONFIG_USER_HAPD
		memset(cpidfilename,0,sizeof(cpidfilename));
		snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.general", HOSTAPDCLIPID, ifname);
		hostapdcli_pid = read_pid(cpidfilename);
		if(hostapdcli_pid<=0){
			va_niced_cmd(HOSTAPD_CLI, 7, 0, "-a", HOSTAPD_GENERAL_SCRIPT, "-B", "-P", cpidfilename, "-i", ifname);
		}
#ifdef WLAN_WPS_VAP
		if(Entry.wsc_disabled == 0)
		{
			memset(cpidfilename,0,sizeof(cpidfilename));
			snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", HOSTAPDCLIPID, ifname);
			hostapdcli_pid = read_pid(cpidfilename);
			if(hostapdcli_pid<=0){
				va_niced_cmd(HOSTAPD_CLI, 7, 0, "-a", HOSTAPD_WPS_SCRIPT, "-B", "-P", cpidfilename, "-i", ifname);
			}
		}
#endif
#ifdef CONFIG_USER_LANNETINFO
		memset(cpidfilename,0,sizeof(cpidfilename));
		snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.lannetinfo", HOSTAPDCLIPID, ifname);
		hostapdcli_pid = read_pid(cpidfilename);
		if(hostapdcli_pid<=0){
			va_niced_cmd(HOSTAPD_CLI, 7, 0, "-a", LANNETINFO_ACTION, "-B", "-P", cpidfilename, "-i", ifname);
		}
#endif
#endif
#endif
	}
	else
	{
		memset(ifname,0,sizeof(ifname));
		rtk_wlan_get_ifname(if_idx, vwlan_idx, ifname);

#ifdef CONFIG_USER_RTK_SYSTEMD
#ifdef CONFIG_USER_HAPD
		memset(cpidfilename,0,sizeof(cpidfilename));
		snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.general", HOSTAPDCLIPID, ifname);
		if(read_pid(cpidfilename)>0)
		{
			kill_by_pidfile(cpidfilename, SIGTERM);
			unlink(cpidfilename);
		}
#ifdef WLAN_WPS_VAP
#if defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
		rtk_wlan_get_wps_led_interface(wps_ifname);
#endif
		memset(cpidfilename,0,sizeof(cpidfilename));
		snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", HOSTAPDCLIPID, ifname);
		if(read_pid(cpidfilename)>0)
		{
			kill_by_pidfile(cpidfilename, SIGTERM);
			if(!strcmp(wps_ifname, ifname)){
				va_cmd(HOSTAPD_WPS_SCRIPT, 2, 1, ifname, "WPS-TIMEOUT");
				unlink(WPS_LED_INTERFACE);
			}
			unlink(cpidfilename);
		}
#endif
#ifdef CONFIG_USER_LANNETINFO
		memset(cpidfilename,0,sizeof(cpidfilename));
		snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.lannetinfo", HOSTAPDCLIPID, ifname);
		if(read_pid(cpidfilename)>0){
			kill_by_pidfile(cpidfilename, SIGTERM);
			unlink(cpidfilename);
		}
#endif
#endif
#endif
		va_cmd(IFCONFIG, 2, 1, ifname, "down");
		va_cmd(WPA_CLI, 5, 1, "-g", global_ctrl_path, "raw", "REMOVE", ifname);
	}

}
#endif

void start_hapd_wpas_process(int wlan_index, config_wlan_ssid ssid_index)
{
	int i, orig_wlan_idx = 0, up_root = 0;
	MIB_CE_MBSSIB_T Entry, Entry2;
	unsigned char vChar;
	unsigned char no_wlan = 0;
	char conf_filename[128]={0};
#ifdef WLAN_UNIVERSAL_REPEATER
	int up_vxd = 0;
#endif
#if defined(WLAN_MBSSID_NUM) && (WLAN_MBSSID_NUM > 0)
	int up_vap = 0;
	char ifname[16]={0};
	int hapd_pid = 0;
	char tmp_buf[128]={0};
#endif

#ifdef WLAN_UNIVERSAL_REPEATER
	if(ssid_index == WLAN_REPEATER_ITF_INDEX)
		up_vxd = 1;
	else
#endif
	{
		if(ssid_index == CONFIG_SSID_ALL || ssid_index == CONFIG_SSID_ROOT)
		{
			up_root = 1;
#ifdef WLAN_UNIVERSAL_REPEATER
			if(ssid_index == CONFIG_SSID_ALL)
				up_vxd = 1;
#endif
		}
		else
		{
#if defined(WLAN_MBSSID_NUM) && (WLAN_MBSSID_NUM > 0)
			up_vap = 1;
#endif
		}
	}

#if defined(WLAN_MBSSID_NUM) && (WLAN_MBSSID_NUM > 0)
	if(up_vap == 1){
		snprintf(tmp_buf, sizeof(tmp_buf), HAPD_PID_PATH_NAME, WLANIF[wlan_index]);
		hapd_pid = read_pid(tmp_buf);
		if(hapd_pid <= 0){
			up_root = 1;
			up_vap = 0;
		}
	}
#endif

	orig_wlan_idx = wlan_idx;

	wlan_idx = wlan_index;
	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, WLAN_ROOT_ITF_INDEX, &Entry);
#ifdef CONFIG_RTK_DEV_AP
	no_wlan = Entry.wlanDisabled;
#else
#if defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(YUEME_3_0_SPEC)
	mib_local_mapping_get(MIB_WLAN_MODULE_DISABLED, wlan_index, (void *)&no_wlan);
	if(no_wlan == 0)
		no_wlan = getWlanStatus(wlan_index)? 0:1;
#else
	no_wlan = Entry.wlanDisabled;
#endif
#endif

	printf("[%d] wlanDisabled=%d \n", wlan_index, no_wlan);

	if (no_wlan){
		wlan_idx = orig_wlan_idx;
		return;
	}

	if(up_root)
	{
#ifdef WLAN_CLIENT
		if(Entry.wlanMode == AP_MODE)
		{
			snprintf(conf_filename, sizeof(conf_filename), HAPD_CONF_PATH_NAME, WLANIF[wlan_index]);
			gen_hapd_cfg(wlan_idx, conf_filename);

			launch_hapd_by_cmd(wlan_idx);
		}
		else if(Entry.wlanMode == CLIENT_MODE)
		{
			snprintf(conf_filename, sizeof(conf_filename), WPAS_CONF_PATH_NAME, WLANIF[wlan_index]);
			gen_wpas_cfg(wlan_idx, 0, conf_filename);
			launch_wpas_by_cmd(wlan_idx, 0);
		}
#else
		snprintf(conf_filename, sizeof(conf_filename), HAPD_CONF_PATH_NAME, WLANIF[wlan_index]);
		gen_hapd_cfg(wlan_idx, conf_filename);
		launch_hapd_by_cmd(wlan_idx);
#endif
#ifdef WLAN_CLIENT
		if(Entry.wlanMode == AP_MODE)
#endif
		{
#if defined(WLAN_MBSSID_NUM) && (WLAN_MBSSID_NUM > 0)
			for (i = WLAN_VAP_ITF_INDEX; i < (WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM); i++)
			{
				memset(&Entry2,0,sizeof(MIB_CE_MBSSIB_T));
				mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, i, &Entry2);
				if(Entry2.wlanDisabled == 0)
				{
#if 0 // defined(CONFIG_RTK_DEV_AP)
					snprintf(ifname,sizeof(ifname),VAP_IF,wlan_idx,i-1);
#else
					rtk_wlan_get_ifname(wlan_index, i, ifname);
#endif
					if(rtk_get_interface_flag(ifname, TIMER_COUNT, IS_RUN) == 0)
						printf("launch hapd: %s failed\n",ifname);
				}
			}
#endif
		}
	}

#ifdef WLAN_UNIVERSAL_REPEATER
	if(up_vxd)
	{
		mib_local_mapping_get(MIB_REPEATER_ENABLED1, wlan_index, (void *)&vChar);
		if(vChar)
		{
			memset(&Entry2,0,sizeof(MIB_CE_MBSSIB_T));
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, WLAN_REPEATER_ITF_INDEX, &Entry2);
			if(Entry2.wlanMode == CLIENT_MODE)
			{
				snprintf(conf_filename, sizeof(conf_filename), WPAS_CONF_PATH_VXD_NAME, WLANIF[wlan_index]);
				gen_wpas_cfg(wlan_idx, WLAN_REPEATER_ITF_INDEX, conf_filename);
#ifdef WLAN_NOT_USE_MIBSYNC
				setup_wlan_priv_mib_or_proc(wlan_index, WLAN_REPEATER_ITF_INDEX);
#endif
				launch_wpas_by_cmd(wlan_idx, WLAN_REPEATER_ITF_INDEX);
			}
		}
	}
#endif

#if defined(WLAN_MBSSID_NUM) && (WLAN_MBSSID_NUM > 0)
	if(up_vap)
	{
		memset(&Entry2,0,sizeof(MIB_CE_MBSSIB_T));
		mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, ssid_index, &Entry2);
		if(!Entry2.wlanDisabled)
		{
			launch_hapd_vif_by_cmd(wlan_idx, ssid_index, 1);
		}
	}
#endif

	wlan_idx = orig_wlan_idx;

}

void stop_hapd_wpas_process(int wlan_index, config_wlan_ssid ssid_index)
{
	int hapd_pid, down_root = 0, j;
	MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_UNIVERSAL_REPEATER
	int down_vxd = 0;
#endif
#if defined(WLAN_MBSSID_NUM) && (WLAN_MBSSID_NUM > 0)
	int down_vap = 0;
#endif
#ifdef CONFIG_USER_RTK_SYSTEMD
#if defined(CONFIG_USER_HAPD) || defined(CONFIG_USER_WPAS)
	int hapd_cli_pid;
	char cpidfilename[128]={0};
#ifdef WLAN_MBSSID
	char ifname[IFNAMSIZ]={0};
#endif
#endif
#endif
#if defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
	char wps_ifname[16]={0};
#endif
	char tmp_buf[100]={0};

#if defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
	rtk_wlan_get_wps_led_interface(wps_ifname);
#endif

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, WLAN_ROOT_ITF_INDEX, (void *)&Entry);
#ifdef WLAN_UNIVERSAL_REPEATER
	if(ssid_index == WLAN_REPEATER_ITF_INDEX)
		down_vxd = 1;
	else
#endif
	{
		if(ssid_index == CONFIG_SSID_ALL || ssid_index == CONFIG_SSID_ROOT)
		{
			down_root = 1;
#ifdef WLAN_UNIVERSAL_REPEATER
			if(ssid_index == CONFIG_SSID_ALL)
				down_vxd = 1;
#endif
#if defined(WLAN_MBSSID_NUM) && (WLAN_MBSSID_NUM > 0)
			if(ssid_index == CONFIG_SSID_ALL)
			{
				down_vap = 1;
			}
#endif
		}
		else
		{
#if defined(WLAN_MBSSID_NUM) && (WLAN_MBSSID_NUM > 0)
			down_vap = 1;
#if !defined(CONFIG_RTK_DEV_AP)
#if defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(YUEME_3_0_SPEC)
			if( !getWlanStatus(wlan_index))
				down_root = 1;
#endif
#endif
#endif
		}
	}

#ifdef CONFIG_USER_RTK_SYSTEMD
#ifdef CONFIG_USER_HAPD
	if(down_root)
	{
		memset(cpidfilename,0,sizeof(cpidfilename));
		snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.general", HOSTAPDCLIPID, WLANIF[wlan_index]);
		if(read_pid(cpidfilename)>0)
		{
			kill_by_pidfile(cpidfilename, SIGTERM);
			unlink(cpidfilename);
		}
#ifdef WLAN_WPS_HAPD
		memset(cpidfilename,0,sizeof(cpidfilename));
		snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", HOSTAPDCLIPID, WLANIF[wlan_index]);
		if(read_pid(cpidfilename)>0){
			kill_by_pidfile(cpidfilename, SIGTERM);
			if(!strcmp(wps_ifname, WLANIF[wlan_index])){
				va_cmd(HOSTAPD_WPS_SCRIPT, 2, 1, WLANIF[wlan_index], "WPS-TIMEOUT");
				unlink(WPS_LED_INTERFACE);
			}
			unlink(cpidfilename);
		}
#endif
#ifdef CONFIG_USER_LANNETINFO
		memset(cpidfilename,0,sizeof(cpidfilename));
		snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.lannetinfo", HOSTAPDCLIPID, WLANIF[wlan_index]);
		if(read_pid(cpidfilename)>0){
			kill_by_pidfile(cpidfilename, SIGTERM);
			unlink(cpidfilename);
		}
#endif
		//for vif
#ifdef WLAN_MBSSID
		for(j=WLAN_VAP_ITF_INDEX; j<(WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM); j++){
			rtk_wlan_get_ifname(wlan_index, j, ifname);
			memset(cpidfilename,0,sizeof(cpidfilename));
			snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.general", HOSTAPDCLIPID, ifname);
			if(read_pid(cpidfilename)>0)
			{
				kill_by_pidfile(cpidfilename, SIGTERM);
				unlink(cpidfilename);
			}
#ifdef WLAN_WPS_VAP
			memset(cpidfilename,0,sizeof(cpidfilename));
			snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", HOSTAPDCLIPID, ifname);
			if(read_pid(cpidfilename)>0)
			{
				kill_by_pidfile(cpidfilename, SIGTERM);
				if(!strcmp(wps_ifname, ifname)){
					va_cmd(HOSTAPD_WPS_SCRIPT, 2, 1, ifname, "WPS-TIMEOUT");
					unlink(WPS_LED_INTERFACE);
				}
				unlink(cpidfilename);
			}
#endif
#ifdef CONFIG_USER_LANNETINFO
			memset(cpidfilename,0,sizeof(cpidfilename));
			snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.lannetinfo", HOSTAPDCLIPID, ifname);
			if(read_pid(cpidfilename)>0){
				kill_by_pidfile(cpidfilename, SIGTERM);
				unlink(cpidfilename);
			}
#endif
		}
#endif
	}
#endif

#ifdef CONFIG_USER_WPAS
	if(down_root)
	{
#ifdef WLAN_WPS_WPAS
		memset(cpidfilename,0,sizeof(cpidfilename));
		snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", WPACLIPID, WLANIF[wlan_index]);
		if(read_pid(cpidfilename)>0){
			kill_by_pidfile(cpidfilename, SIGTERM);
			if(!strcmp(wps_ifname, WLANIF[wlan_index])){
				va_cmd(WPAS_WPS_SCRIPT, 2, 1, WLANIF[wlan_index], "WPS-TIMEOUT");
				unlink(WPS_LED_INTERFACE);
			}
			unlink(cpidfilename);
		}
#endif
	}

#ifdef WLAN_UNIVERSAL_REPEATER
	if(down_vxd)
	{
#ifdef WLAN_WPS_WPAS
		memset(cpidfilename,0,sizeof(cpidfilename));
		snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", WPACLIPID, WLAN_VXD_IF[wlan_index]);
		if(read_pid(cpidfilename)>0){
			kill_by_pidfile(cpidfilename, SIGTERM);
			if(!strcmp(wps_ifname, WLAN_VXD_IF[wlan_index])){
				va_cmd(WPAS_WPS_SCRIPT, 2, 1, WLAN_VXD_IF[wlan_index], "WPS-TIMEOUT");
				unlink(WPS_LED_INTERFACE);
			}
			unlink(cpidfilename);
		}
#endif
	}
#endif
#endif
#endif

#ifdef WLAN_UNIVERSAL_REPEATER
	if(down_vxd)
	{
		va_cmd(IFCONFIG, 2, 1, WLAN_VXD_IF[wlan_index], "down");
		memset(tmp_buf,0,sizeof(tmp_buf));
		snprintf(tmp_buf, sizeof(tmp_buf), WPAS_PID_PATH_VXD_NAME, WLANIF[wlan_index]);
		hapd_cli_pid = read_pid(tmp_buf);
		if(hapd_cli_pid > 0){
			kill(hapd_cli_pid, 15);
			unlink(tmp_buf);
		}
		va_cmd(BRCTL, 3, 1, "delif", BRIF, WLAN_VXD_IF[wlan_index]);
	}
#endif

#if defined(WLAN_MBSSID_NUM) && (WLAN_MBSSID_NUM > 0)
	if(down_vap)
	{
		if(ssid_index == CONFIG_SSID_ALL)
		{
			for (j=WLAN_VAP_ITF_INDEX; j<(WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM); j++)
			{
				launch_hapd_vif_by_cmd(wlan_index, j, 0);
			}
		}
		else
			launch_hapd_vif_by_cmd(wlan_index, ssid_index, 0);
	}
#endif

	if(down_root)
	{
		va_cmd(IFCONFIG, 2, 1, WLANIF[wlan_index], "down");
		memset(tmp_buf,0,sizeof(tmp_buf));
		snprintf(tmp_buf, sizeof(tmp_buf), HAPD_PID_PATH_NAME, WLANIF[wlan_index]);
		hapd_cli_pid = read_pid(tmp_buf);
		if(hapd_cli_pid > 0){
			kill(hapd_cli_pid, 15);
			unlink(tmp_buf);
		}

#ifdef WLAN_CLIENT
		memset(tmp_buf,0,sizeof(tmp_buf));
		snprintf(tmp_buf, sizeof(tmp_buf), WPAS_PID_PATH_NAME, WLANIF[wlan_index]);
		hapd_cli_pid = read_pid(tmp_buf);
		if(hapd_cli_pid > 0){
			kill(hapd_cli_pid, 15);
			unlink(tmp_buf);
			va_cmd(BRCTL, 3, 1, "delif", BRIF, WLANIF[wlan_index]);
		}
#endif
	}
}

// Added by Mason Yu
int stopwlan(config_wlan_target target, config_wlan_ssid ssid_index)
{
	int status = 0;
#ifdef WLAN_SMARTAPP_ENABLE
	int smart_wlanapp_pid=0;
#endif

//#if IS_AX_SUPPORT
	int j;
	unsigned char phy_band_select;
//#endif

#ifdef WLAN_BAND_STEERING
	rtk_wlan_stop_band_steering();
#endif

#ifdef WLAN_SMARTAPP_ENABLE
	smart_wlanapp_pid = read_pid(SMART_WLANAPP_PID);
	if(smart_wlanapp_pid > 0) {
		kill(smart_wlanapp_pid, 9);
		unlink(SMART_WLANAPP_PID);
	}
#endif

//#if IS_AX_SUPPORT
#ifdef RTK_HAPD_MULTI_AP
	rtk_stop_multi_ap_service();
#endif
	//kill hostapd
	for(j=0;j<NUM_WLAN_INTERFACE;j++){
		mib_local_mapping_get( MIB_WLAN_PHY_BAND_SELECT, j, (void *)&phy_band_select);
		if((target == CONFIG_WLAN_2G && phy_band_select == PHYBAND_5G)
		|| (target == CONFIG_WLAN_5G && phy_band_select == PHYBAND_2G)) {
			continue;
		}

		stop_hapd_wpas_process(j, ssid_index);

	}
//#endif

#ifdef RTK_WLAN_MANAGER
	rtk_stop_wlan_manager();
#endif

	return status;
}

#ifdef WIFI5_WIFI6_RUNTIME
#define B1_G1	40
#define B1_G2	48

#define B2_G1	56
#define B2_G2	64

#define B3_G1	104
#define B3_G2	112
#define B3_G3	120
#define B3_G4	128
#define B3_G5	136
#define B3_G6	144

#define B4_G1	153
#define B4_G2	161
#define B4_G3	169
#define B4_G4	177

void assign_diff_AC(unsigned char* pMib, unsigned char* pVal)
{
/*
	int i;
	printf("%s ", __FUNCTION__);
	for(i=0; i<14;i++)
		printf("%02x ", pVal[i]);
	printf("\n");
*/
	memset(pMib, 0, 35);
	memset((pMib+35), pVal[0], (B1_G1-35));
	memset((pMib+B1_G1), pVal[1], (B1_G2-B1_G1));
	memset((pMib+B1_G2), pVal[2], (B2_G1-B1_G2));
	memset((pMib+B2_G1), pVal[3], (B2_G2-B2_G1));
	memset((pMib+B2_G2), pVal[4], (B3_G1-B2_G2));
	memset((pMib+B3_G1), pVal[5], (B3_G2-B3_G1));
	memset((pMib+B3_G2), pVal[6], (B3_G3-B3_G2));
	memset((pMib+B3_G3), pVal[7], (B3_G4-B3_G3));
	memset((pMib+B3_G4), pVal[8], (B3_G5-B3_G4));
	memset((pMib+B3_G5), pVal[9], (B3_G6-B3_G5));
	memset((pMib+B3_G6), pVal[10], (B4_G1-B3_G6));
	memset((pMib+B4_G1), pVal[11], (B4_G2-B4_G1));
	memset((pMib+B4_G2), pVal[12], (B4_G3-B4_G2));
	memset((pMib+B4_G3), pVal[13], (B4_G4-B4_G3));
/*
	for(i=0; i<178;i++)
		printf("%02x", pMib[i]);
	printf("\n");
*/
}

typedef enum {
	CHIP_UNKNOWN=0,
		CHIP_RTL8188C=1,
		CHIP_RTL8192C=2,
		CHIP_RTL8192D=3,
		CHIP_RTL8192E=4,
		CHIP_RTL8188E=5,
		CHIP_RTL8197G=6,
		CHIP_RTL8812F=7,
		CHIP_RTL8814B=8,
		CHIP_RTL8822B=9,
		CHIP_RTL8812A=10,
		CHIP_RTL8198F=11,
} CHIP_VERSION_T;

#ifdef __ECOS
#define FILE_MIB_RF	"/tmp/mib_rf.log"
#endif
static unsigned int getWLAN_ChipVersion(const char *ifname)
{
	FILE *stream;
	char buffer[128], cmd[32];

	CHIP_VERSION_T chipVersion = CHIP_UNKNOWN;
#ifdef __ECOS
	stream = stdout;
	if((stdout = fopen(FILE_MIB_RF,"w")) == NULL)
	{
		printf("redirect mib_rf fail!\n");
		stdout = stream;
		return -1;
	}
	int wlan_idx = apmib_get_wlanidx();
	sprintf(cmd,WLAN_IF " mib_rf", wlan_idx);
	//RunSystemCmd(NULL_FILE, ifname, "mib_rf", NULL_STR);
	run_clicmd(cmd);
	fclose(stdout);
	stdout = stream;
	if((stream = fopen(FILE_MIB_RF,"r")) == NULL)
	{
		printf("open mib_rf file fail!\n");
		return -1;
	}
#else
	//sprintf(buffer,"/proc/wlan%d/mib_rf",wlan_idx);
	//snprintf(ifname, sizeof(ifname), WLAN_IF, wlan_idx);
	snprintf(buffer, sizeof(buffer), LINK_FILE_NAME_FMT, ifname, "mib_rf");
	if((stream = fopen(buffer, "r"))== NULL)
	{
		printf("open mib_rf file fail!\n");
		return -1;
	}
#endif
	if ( stream != NULL )
	{
		char *strtmp;
		char line[100];

		while (fgets(line, sizeof(line), stream))
		{

			strtmp = line;

			if(strstr(strtmp,"RTL8192SE") != 0)
			{
				chipVersion = CHIP_UNKNOWN;
			}
			else if(strstr(strtmp,"RTL8188C") != 0||strstr(strtmp,"RTL8188R") != 0|| strstr(strtmp,"RTL6195B") != 0)
			{
				chipVersion = CHIP_RTL8188C;
			}
			else if(strstr(strtmp,"RTL8188E") != 0)
			{
				chipVersion = CHIP_RTL8188E;
			}
			else if(strstr(strtmp,"RTL8192C") != 0)
			{
				chipVersion = CHIP_RTL8192C;
			}
			else if(strstr(strtmp,"RTL8192D") !=0)
			{
				chipVersion = CHIP_RTL8192D;
			}
			else if(strstr(strtmp,"RTL8812F") !=0)
			{
				chipVersion = CHIP_RTL8812F;
			}
			else if(strstr(strtmp,"RTL8812") !=0)
			{
				chipVersion = CHIP_RTL8812A;
			}
			else if(strstr(strtmp,"RTL8192E") !=0)
			{
				chipVersion = CHIP_RTL8192E;
			}
			else if(strstr(strtmp,"RTL8197G") !=0)
			{
				chipVersion = CHIP_RTL8197G;
			}
			else if(strstr(strtmp,"RTL8814B") !=0)
			{
				chipVersion = CHIP_RTL8814B;
			}
			else if(strstr(strtmp,"RTL8822B") !=0)
			{
				chipVersion = CHIP_RTL8822B;
			}
			else if(strstr(strtmp,"RTL8198F") !=0)
			{
				chipVersion = CHIP_RTL8198F;
			}

		}
		fclose ( stream );
	}

	return chipVersion;
}
#endif //WIFI5_WIFI6_RUNTIME

enum {IWPRIV_GETMIB=1, IWPRIV_HS=2, IWPRIV_INT=4, IWPRIV_HW2G=8, IWPRIV_TXPOWER=16, IWPRIV_HWDPK=32, IWPRIV_AX=64, };
int iwpriv_cmd(int type, ...)
{
	va_list ap;
	int k=0, i, len;
	char *s, *s2;
	char *argv[24];
	int status;
	unsigned char value[196];
	char parm[2048];
	unsigned char pMib[178];
	int mib_id, mode, intVal, dpk_len=0, tssi_len=0;
	unsigned char dpk_value[1024]={0};
#ifdef WIFI5_WIFI6_RUNTIME
	int mib_hw_index = 0;
	unsigned char tssi_value[1]={0};
	int wlan_index=0, vwlan_index=0;
#endif

	TRACE(STA_SCRIPT, "%s ", IWPRIV);
	va_start(ap, type);

	s = va_arg(ap, char *); //wlan interface name
	argv[++k] = s;
	TRACE(STA_SCRIPT|STA_NOTAG, "%s ", s);
	s = va_arg(ap, char *); //cmd, ie set_mib
	argv[++k] = s;
	TRACE(STA_SCRIPT|STA_NOTAG, "%s ", s);
	s = va_arg(ap, char *); //cmd detail

	if(type & IWPRIV_GETMIB){
		mib_id = va_arg(ap, int);
		mib_get_s(mib_id, (void *)value, sizeof(value));
	}
	else{
		if(type & IWPRIV_HS){
			if(type & IWPRIV_HW2G)
				memcpy(value, va_arg(ap, char *), MAX_CHAN_NUM);
			else if(type & IWPRIV_HWDPK)
			{
				dpk_len = va_arg(ap, int);
				memcpy(dpk_value, va_arg(ap, char *), dpk_len);
			}
			else if(type & IWPRIV_AX){
				tssi_len = va_arg(ap, int);
				memcpy(value, va_arg(ap, char *), tssi_len);
			}
			else{ //5G
				if(type & IWPRIV_TXPOWER)
					memcpy(value, va_arg(ap, char *), MAX_5G_CHANNEL_NUM);
				else
					memcpy(value, va_arg(ap, char *), MAX_5G_DIFF_NUM);
			}
		}
	}

	if(!(type & IWPRIV_HS)){
		if(type & IWPRIV_GETMIB){
			if(type & IWPRIV_INT) //int
				snprintf(parm, sizeof(parm), "%s=%u", s, value[0]);
			else //string
				snprintf(parm, sizeof(parm), "%s=%s", s, value);
		}
		else{
			if(type & IWPRIV_INT){ //int
				intVal = va_arg(ap, int);
				snprintf(parm, sizeof(parm), "%s=%u", s, intVal);
			}
			else{ //string
				s2 = va_arg(ap, char *);
				snprintf(parm, sizeof(parm), "%s=%s", s, s2);
			}
		}
	}
	else{
		snprintf(parm, sizeof(parm), "%s=", s);
		if(type & IWPRIV_AX){
			for(i=0; i<tssi_len; i++) {
				len = sizeof(parm)-strlen(parm);
				if(snprintf(parm+strlen(parm), len, "%02x", value[i]) >= len){
					printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
				}
			}

		}
		else if(type & IWPRIV_HW2G){ //2G
			if(type & IWPRIV_TXPOWER){
				mode = va_arg(ap, int);
#ifdef WIFI5_WIFI6_RUNTIME
				if(rtk_wlan_get_mib_index_mapping_by_ifname(argv[1], &wlan_index, &vwlan_index)==0)
					mib_hw_index = read_wlan_hwmib_from(wlan_index);
				mib_get_s(HW_MIB_FROM(11N_TSSI_ENABLE, mib_hw_index), (void *)tssi_value, sizeof(tssi_value));
				if(tssi_value[0])
					intVal = 0;
				else{
					intVal = getTxPowerScale(PHYBAND_2G, mode);
					/*add by taro: 98F 0.25 dB/unit */
					int chipVersion = getWLAN_ChipVersion(argv[1]);
					if(chipVersion == CHIP_RTL8198F)
						intVal<<=1;
					/*end by taro*/
				}
#else
				intVal = getTxPowerScale(PHYBAND_2G, mode);
#endif
				for(i=0; i<MAX_CHAN_NUM; i++) {
					if(value[i]!=0){
						if((value[i] - intVal)>=1)
							value[i] -= intVal;
						else
							value[i] = 1;
					}
					len = sizeof(parm)-strlen(parm);
					if(snprintf(parm+strlen(parm), len, "%02x", value[i]) >= len){
						printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
					}
				}
			}
			else{
				for(i=0; i<MAX_CHAN_NUM; i++) {
					len = sizeof(parm)-strlen(parm);
					if(snprintf(parm+strlen(parm), len, "%02x", value[i]) >= len){
						printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
					}
				}
			}

		}
		else if(type & IWPRIV_HWDPK){
			for(i=0; i<dpk_len; i++) {
				len = sizeof(parm)-strlen(parm);
				if(snprintf(parm+strlen(parm), len, "%02x", dpk_value[i]) >= len){
					printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
				}
			}
		}
#ifdef WIFI5_WIFI6_RUNTIME
		else{ //5G

			if(type & IWPRIV_TXPOWER){
				mode = va_arg(ap, int);
				if(rtk_wlan_get_mib_index_mapping_by_ifname(argv[1], &wlan_index, &vwlan_index)==0)
					mib_hw_index = read_wlan_hwmib_from(wlan_index);
				mib_get_s(HW_MIB_FROM(11N_TSSI_ENABLE, mib_hw_index), (void *)tssi_value, sizeof(tssi_value));
				if(tssi_value[0])
					intVal = 0;
				else{
					intVal = getTxPowerScale(PHYBAND_5G, mode);
					s2 = strstr(s, "TSSI");
					if (s2) {
						// 0.125 dB/unit in TSSI DE, 0.5 dB/unit in AGC, so need to multiply it by 4 .
						intVal *= 4;
						intVal = -intVal;
					} else {
						/*add by taro: 8814B 8812F 0.25 dB/unit */
						int chipVersion = getWLAN_ChipVersion(argv[1]);
						if(chipVersion == CHIP_RTL8814B || chipVersion == CHIP_RTL8812F)
							intVal<<=1;
						/*end by taro*/
					}
				}
				for(i=0; i<MAX_5G_CHANNEL_NUM; i++) {
					if(value[i]!=0){
						if((value[i] - intVal)>=1)
							value[i] -= intVal;
						else
							value[i] = 1;
					}
					len = sizeof(parm)-strlen(parm);
					if(snprintf(parm+strlen(parm), len, "%02x", value[i]) >= len){
						printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
					}
				}
			}
			else{
				assign_diff_AC(pMib, value);
				for(i=0; i<177; i++) {
					len = sizeof(parm)-strlen(parm);
					if(snprintf(parm+strlen(parm), len, "%02x", pMib[i]) >= len){
						printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
					}
				}
			}


		}
#endif
	}
	argv[++k] = parm;
	TRACE(STA_SCRIPT|STA_NOTAG, "%s ", argv[k]);
	TRACE(STA_SCRIPT|STA_NOTAG, "\n");
	argv[k+1] = NULL;
	status = do_cmd(IWPRIV, argv, 1);
	va_end(ap);

	return status;
}



#define MAX_2G_TSSI_CCK_SIZE_AX    6
#define MAX_2G_TSSI_BW40_SIZE_AX   5
#define MAX_5G_TSSI_BW40_SIZE_AX  14

#ifdef CONFIG_RTIC
int setup_wlan_hs_rtic()
{
	char wlanif[64]={0};
	unsigned char value[128]={0};
	int skfd, cnt=0;
	struct iwreq wrq;

	snprintf(wlanif, sizeof(wlanif), WLAN_IF, 2); //IC 8701 is fixed as wlan2
	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("[%s %d] socket error \n", __FUNCTION__, __LINE__);
		return -1;
	}
	while ( iw_get_ext(skfd, wlanif, SIOCGIWNAME, &wrq) < 0)
	{
		if(cnt > 10){
			printf("[%s %d] the interface %s is't up yet!", __FUNCTION__, __LINE__, wlanif);
			close(skfd);
			return -1;
		}
		sleep(1);
		cnt++;
	}
	close(skfd);

	mib_get_s(MIB_HW_XCAP_WLAN2_AX, (void *)value, sizeof(value));
	iwpriv_cmd(IWPRIV_INT, wlanif, "flash_set", "xcap", value[0]);

	mib_get_s(MIB_HW_THERMAL_A_WLAN2, (void *)value, sizeof(value));
	iwpriv_cmd(IWPRIV_INT, wlanif, "flash_set", "thermalA", value[0]);

	mib_get_s(MIB_HW_THERMAL_B_WLAN2, (void *)value, sizeof(value));
	iwpriv_cmd(IWPRIV_INT, wlanif, "flash_set", "thermalB", value[0]);

	mib_get_s(MIB_HW_THERMAL_C_WLAN2, (void *)value, sizeof(value));
	iwpriv_cmd(IWPRIV_INT, wlanif, "flash_set", "thermalC", value[0]);

	mib_get_s(MIB_HW_THERMAL_D_WLAN2, (void *)value, sizeof(value));
	iwpriv_cmd(IWPRIV_INT, wlanif, "flash_set", "thermalD", value[0]);

	return 0;
}
#endif

int rtk_get_interface_flag(const char *ifname, int count, int flag)
{
	int ret=0, skfd;
	struct iwreq wrq;
	struct sysinfo sys_info;
	unsigned long timestamp=0;

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("[%s %d] socket error \n", __FUNCTION__, __LINE__);
		ret = 0;
		return ret;
	}

	sysinfo(&sys_info);
	timestamp = sys_info.uptime;

	while(iw_get_ext(skfd, ifname, SIOCGIWNAME, &wrq) < 0)
	{
		sysinfo(&sys_info);
		if((sys_info.uptime - timestamp) > count)
		{
			ret = 0;
			printf("======>%s %d %s interface doesn't exist %d\n", __FUNCTION__, __LINE__, ifname, count);
			close(skfd);
			return ret;
		}
		sleep(1);
	}

	close(skfd);

	if(flag == IS_EXIST)
	{
		ret = 1;
		return ret;
	}

	sysinfo(&sys_info);
	timestamp = sys_info.uptime;

	while(!is_interface_run(ifname))
	{
		sysinfo(&sys_info);
		if((sys_info.uptime - timestamp) > count)
		{
			ret = 0;
			printf("======>%s %d %s interface doesn't run %d\n", __FUNCTION__, __LINE__, ifname, count);
			return ret;
		}
		sleep(1);
	}

	if(flag == IS_RUN)
		ret = 1;

	return ret;
}

#ifdef WLAN_ROOT_MIB_SYNC
#ifdef WLAN_SUPPORT
int read_wlan_hwmib_from(int wlan_index)
{
#if defined(CONFIG_RTK_DEV_AP)
	unsigned char hw_wlan0_5g, hw_5g_wlan;
	int ret = -1;
	unsigned char wlan_interface[20] = {0};
	
	if(mib_get_s(MIB_HW_5G_ON_WLAN, (void *)&hw_5g_wlan, sizeof(hw_5g_wlan))==0){
		hw_5g_wlan = 0;
		printf("mib_get MIB_HW_5G_ON_WLAN fail, hw 5g on wlan0 in default !!!!!\n");
	}

	if(hw_5g_wlan==0)
		hw_wlan0_5g = 1;
	else
		hw_wlan0_5g = 0;
	
#ifdef CONFIG_2G_ON_WLAN0
	if((wlan_index==0 && hw_wlan0_5g) || (wlan_index==1 && !hw_wlan0_5g))
		ret = 1;
	else if((wlan_index==0 && !hw_wlan0_5g) || (wlan_index==1 && hw_wlan0_5g))
		ret = 0;		
#else
	if((wlan_index==0 && hw_wlan0_5g) || (wlan_index==1 && !hw_wlan0_5g))
		ret = 0;
	else if((wlan_index==0 && !hw_wlan0_5g) || (wlan_index==1 && hw_wlan0_5g))
		ret = 1;	
#endif

#if 0 // defined(CONFIG_RTK_DEV_AP)
	snprintf(wlan_interface, sizeof(wlan_interface), WLAN_IF, wlan_index);
#else
	rtk_wlan_get_ifname(wlan_index, WLAN_ROOT_ITF_INDEX, wlan_interface);
#endif
#ifdef CONFIG_2G_ON_WLAN0
	printf("(%s)[hs]wlan0_5g:%d, [cs]select 2g on wlan0, read hwmib from HW_WLAN%d\n", wlan_interface, hw_wlan0_5g, ret);
#else
	printf("(%s)[hs]wlan0_5g:%d, [cs]select 5g on wlan0, read hwmib from HW_WLAN%d\n", wlan_interface, hw_wlan0_5g, ret);
#endif

	return ret;
#else
	int wlan_hwmib_index = 0;
#ifdef WLAN_DUALBAND_CONCURRENT
	unsigned char phyband=PHYBAND_2G;
	mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlan_index, &phyband);
	if(phyband==PHYBAND_2G)
		wlan_hwmib_index = 1;
	else
		wlan_hwmib_index = 0;
#endif
	return wlan_hwmib_index;
#endif //CONFIG_RTK_DEV_AP
}

int check_sync_info(void)
{
	unsigned char vChar;
	unsigned char ssid[MAX_SSID_LEN]={0}, password[MAX_PSK_LEN+1]={0};
	unsigned char rsIpAddr[IP_ADDR_LEN];
	unsigned short vShort;
	unsigned char wep64Key[WEP64_KEY_LEN]={0}, wep128Key[WEP128_KEY_LEN]={0};
	unsigned int vInt;
	MIB_CE_MBSSIB_T Entry;

	memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));
	mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry);

	mib_get_s(MIB_WLAN_ENCRYPT, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.encrypt)
		return 1;
	mib_get_s(MIB_WLAN_ENABLE_1X, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.enable1X)
		return 1;
	mib_get_s(MIB_WLAN_WEP, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wep)
		return 1;
	mib_get_s(MIB_WLAN_WPA_AUTH, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wpaAuth)
		return 1;
	mib_get_s(MIB_WLAN_WPA_PSK_FORMAT, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wpaPSKFormat)
		return 1;
	mib_get_s(MIB_WLAN_WPA_PSK, (void *)password, sizeof(password));
	if(memcmp(password, Entry.wpaPSK, sizeof(password)))
		return 1;
	mib_get_s(MIB_WLAN_WPA_GROUP_REKEY_TIME, (void *)&vInt, sizeof(vInt));
	if(vInt!=Entry.wpaGroupRekeyTime)
		return 1;
	mib_get_s(MIB_WLAN_RS_IP, (void *)rsIpAddr, sizeof(rsIpAddr));
	if(memcmp(rsIpAddr, Entry.rsIpAddr, sizeof(rsIpAddr)))
		return 1;
	mib_get_s(MIB_WLAN_RS_PORT, (void *)&vShort, sizeof(vShort));
	if(vShort!=Entry.rsPort)
		return 1;
	mib_get_s(MIB_WLAN_RS_PASSWORD, (void *)password, sizeof(password));
	if(memcmp(password, Entry.rsPassword, sizeof(password)))
		return 1;
	mib_get_s(MIB_WLAN_DISABLED, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wlanDisabled)
		return 1;
	mib_get_s(MIB_WLAN_SSID, (void *)ssid, sizeof(ssid));
	if(memcmp(ssid, Entry.ssid, sizeof(ssid)))
		return 1;
	mib_get_s(MIB_WLAN_MODE, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wlanMode)
		return 1;
	mib_get_s(MIB_WLAN_AUTH_TYPE, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.authType)
		return 1;
	mib_get_s(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.unicastCipher)
		return 1;
	mib_get_s(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wpa2UnicastCipher)
		return 1;
	mib_get_s(MIB_WLAN_HIDDEN_SSID, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.hidessid)
		return 1;
	mib_get_s(MIB_WLAN_QoS, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wmmEnabled)
		return 1;
	mib_get_s(MIB_WLAN_MCAST2UCAST_DISABLE, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.mc2u_disable)
		return 1;
	mib_get_s(MIB_WLAN_BLOCK_RELAY, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.userisolation)
		return 1;
	mib_get_s(MIB_WLAN_WEP_KEY_TYPE, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wepKeyType)
		return 1;
	mib_get_s(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wepDefaultKey)
		return 1;
	mib_get_s(MIB_WLAN_WEP64_KEY1, (void *)wep64Key, sizeof(wep64Key));
	if(memcmp(wep64Key, Entry.wep64Key1, sizeof(wep64Key)))
		return 1;
	mib_get_s(MIB_WLAN_WEP64_KEY2, (void *)wep64Key, sizeof(wep64Key));
	if(memcmp(wep64Key, Entry.wep64Key2, sizeof(wep64Key)))
		return 1;
	mib_get_s(MIB_WLAN_WEP64_KEY3, (void *)wep64Key, sizeof(wep64Key));
	if(memcmp(wep64Key, Entry.wep64Key3, sizeof(wep64Key)))
		return 1;
	mib_get_s(MIB_WLAN_WEP64_KEY4, (void *)wep64Key, sizeof(wep64Key));
	if(memcmp(wep64Key, Entry.wep64Key4, sizeof(wep64Key)))
		return 1;
	mib_get_s(MIB_WLAN_WEP128_KEY1, (void *)wep128Key, sizeof(wep128Key));
	if(memcmp(wep128Key, Entry.wep128Key1, sizeof(wep128Key)))
		return 1;
	mib_get_s(MIB_WLAN_WEP128_KEY2, (void *)wep128Key, sizeof(wep128Key));
	if(memcmp(wep128Key, Entry.wep128Key2, sizeof(wep128Key)))
		return 1;
	mib_get_s(MIB_WLAN_WEP128_KEY3, (void *)wep128Key, sizeof(wep128Key));
	if(memcmp(wep128Key, Entry.wep128Key3, sizeof(wep128Key)))
		return 1;
	mib_get_s(MIB_WLAN_WEP128_KEY4, (void *)wep128Key, sizeof(wep128Key));
	if(memcmp(wep128Key, Entry.wep128Key4, sizeof(wep128Key)))
		return 1;
	mib_get_s(MIB_WLAN_RATE_ADAPTIVE_ENABLED, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.rateAdaptiveEnabled)
		return 1;
#if defined(WLAN_BAND_CONFIG_MBSSID)
	mib_get_s(MIB_WLAN_BAND, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wlanBand)
		return 1;
#endif
	mib_get_s(MIB_WLAN_FIX_RATE, (void *)&vInt, sizeof(vInt));
	if(vInt!=Entry.fixedTxRate)
		return 1;
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
	mib_get_s(MIB_WSC_DISABLE, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wsc_disabled)
		return 1;
	mib_get_s(MIB_WSC_CONFIGURED, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wsc_configured)
		return 1;
	mib_get_s(MIB_WSC_UPNP_ENABLED, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wsc_upnp_enabled)
		return 1;
	mib_get_s(MIB_WSC_AUTH, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wsc_auth)
		return 1;
	mib_get_s(MIB_WSC_ENC, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.wsc_enc)
		return 1;
	mib_get_s(MIB_WSC_PSK, (void *)password, sizeof(password));
	if(memcmp(password, Entry.wscPsk, sizeof(password)))
		return 1;
#endif
#ifdef WLAN_11W
	mib_get_s(MIB_WLAN_DOTIEEE80211W, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.dotIEEE80211W)
		return 1;
	mib_get_s(MIB_WLAN_SHA256, (void *)&vChar, sizeof(vChar));
	if(vChar!=Entry.sha256)
		return 1;
#endif
	return 0;
}

#if defined(WIFI5_WIFI6_RUNTIME)
static int setupWlanHWRegSettings(int wlan_index, char *ifname)
{
	unsigned char phyband = 0;
	unsigned char value[34];
	char parm[64];
	unsigned char mode=0;
	int intVal = 0, status = 0;
	int hwmib_idx = 0;

	hwmib_idx = read_wlan_hwmib_from(wlan_index);

	mib_get_s(HW_MIB_FROM(11N_TSSI_ENABLE, hwmib_idx), (void *)value, sizeof(value));

	if(value[0]){
		mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlan_index, (void *)&phyband);
		if ( mib_local_mapping_get( MIB_TX_POWER, wlan_index, (void *)&mode) == 0) {
			printf("Get MIB_TX_POWER error!\n");
			status=-1;
		}
		intVal = getTxPowerScale(phyband, mode);
		snprintf(parm, sizeof(parm), "all=%d", intVal*(-2));
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_reg_pg", parm);
	}
	return status;
}

static int _setup8812Wlan(int hwmib_idx, char *wlan_interface, unsigned char mode)
{
	unsigned char value[196];

	mib_get_s(HW_MIB_FROM(TX_POWER_5G_HT40_1S_A, hwmib_idx), (void *)value, sizeof(value));
	iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevel5GHT40_1S_A", value, mode);

	mib_get_s(HW_MIB_FROM(TX_POWER_5G_HT40_1S_B, hwmib_idx), (void *)value, sizeof(value));
	iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevel5GHT40_1S_B", value, mode);

	mib_get_s(HW_MIB_FROM(TX_POWER_TSSI_5G_HT40_1S_A, hwmib_idx), (void *)value, sizeof(value));
	iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevel_TSSI5GHT40_1S_A", value, mode);

	mib_get_s(HW_MIB_FROM(TX_POWER_TSSI_5G_HT40_1S_B, hwmib_idx), (void *)value, sizeof(value));
	iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevel_TSSI5GHT40_1S_B", value, mode);

	mib_get_s(HW_MIB_FROM(TX_POWER_DIFF_5G_20BW1S_OFDM1T_A, hwmib_idx), (void *)value, sizeof(value));
	if(value[0] != 0)
		iwpriv_cmd(IWPRIV_HS, wlan_interface, "set_mib", "pwrdiff_5G_20BW1S_OFDM1T_A", value);

	mib_get_s(HW_MIB_FROM(TX_POWER_DIFF_5G_40BW2S_20BW2S_A, hwmib_idx), (void *)value, sizeof(value));
	if(value[0] != 0)
		iwpriv_cmd(IWPRIV_HS, wlan_interface, "set_mib", "pwrdiff_5G_40BW2S_20BW2S_A", value);

	mib_get_s(HW_MIB_FROM(TX_POWER_DIFF_5G_80BW1S_160BW1S_A, hwmib_idx), (void *)value, sizeof(value));
	if(value[0] != 0)
		iwpriv_cmd(IWPRIV_HS, wlan_interface, "set_mib", "pwrdiff_5G_80BW1S_160BW1S_A", value);

	mib_get_s(HW_MIB_FROM(TX_POWER_DIFF_5G_80BW2S_160BW2S_A, hwmib_idx), (void *)value, sizeof(value));
	if(value[0] != 0)
		iwpriv_cmd(IWPRIV_HS, wlan_interface, "set_mib", "pwrdiff_5G_80BW2S_160BW2S_A", value);

	mib_get_s(HW_MIB_FROM(TX_POWER_DIFF_5G_20BW1S_OFDM1T_B, hwmib_idx), (void *)value, sizeof(value));
	if(value[0] != 0)
		iwpriv_cmd(IWPRIV_HS, wlan_interface, "set_mib", "pwrdiff_5G_20BW1S_OFDM1T_B", value);

	mib_get_s(HW_MIB_FROM(TX_POWER_DIFF_5G_40BW2S_20BW2S_B, hwmib_idx), (void *)value, sizeof(value));
	if(value[0] != 0)
		iwpriv_cmd(IWPRIV_HS, wlan_interface, "set_mib", "pwrdiff_5G_40BW2S_20BW2S_B", value);

	mib_get_s(HW_MIB_FROM(TX_POWER_DIFF_5G_80BW1S_160BW1S_B, hwmib_idx), (void *)value, sizeof(value));
	if(value[0] != 0)
		iwpriv_cmd(IWPRIV_HS, wlan_interface, "set_mib", "pwrdiff_5G_80BW1S_160BW1S_B", value);

	mib_get_s(HW_MIB_FROM(TX_POWER_DIFF_5G_80BW2S_160BW2S_B, hwmib_idx), (void *)value, sizeof(value));
	if(value[0] != 0)
		iwpriv_cmd(IWPRIV_HS, wlan_interface, "set_mib", "pwrdiff_5G_80BW2S_160BW2S_B", value);

	mib_get_s(HW_MIB_FROM(TX_POWER_TSSI_5G_HT40_1S_A, hwmib_idx), (void *)value, sizeof(value));
	iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevel_TSSI5GHT40_1S_A", value, mode);

	mib_get_s(HW_MIB_FROM(TX_POWER_TSSI_5G_HT40_1S_B, hwmib_idx), (void *)value, sizeof(value));
	iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevel_TSSI5GHT40_1S_B", value, mode);

	mib_get_s(HW_MIB_FROM(TX_POWER_TSSI_5G_HT40_1S_C, hwmib_idx), (void *)value, sizeof(value));
	iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevel_TSSI5GHT40_1S_C", value, mode);

	mib_get_s(HW_MIB_FROM(TX_POWER_TSSI_5G_HT40_1S_D, hwmib_idx), (void *)value, sizeof(value));
	iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevel_TSSI5GHT40_1S_D", value, mode);

	return 0;
}
#endif

int set_hw_mib(int ax_flag, int wlan_index)
{
	/*TBD: move to individual function*/
	unsigned char value[128];
	char wlan_interface[64];
	unsigned char hw_wlan0_5g = 0, hwmib_idx;
	memset(wlan_interface, 0, sizeof(wlan_interface));
#if 0 // defined(CONFIG_RTK_DEV_AP)
	snprintf(wlan_interface, sizeof(wlan_interface), WLAN_IF, wlan_index);
#else
	rtk_wlan_get_ifname(wlan_index, WLAN_ROOT_ITF_INDEX, wlan_interface);
#endif
	
#if 1//def RTW_MP_CALIBRATE_DATA
	if(ax_flag == 1)
	{
		/* Get wireless name */
		if(rtk_get_interface_flag(wlan_interface, TIMER_COUNT, IS_EXIST) == 0)
		{
			printf("[%s %d] the interface is't up yet!", __FUNCTION__, __LINE__);
			return -1;
		}

		hwmib_idx = read_wlan_hwmib_from(wlan_index);
		if(hwmib_idx==0){
			mib_get_s(MIB_HW_XCAP_WLAN0_AX, (void *)value, sizeof(value));
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "xcap", value[0]);

			mib_get_s(MIB_HW_RFE_TYPE_WLAN0, (void *)value, sizeof(value));
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "rfe", value[0]);

			mib_get_s(MIB_HW_THERMAL_A_WLAN0, (void *)value, sizeof(value));
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "thermalA", value[0]);

			mib_get_s(MIB_HW_THERMAL_B_WLAN0, (void *)value, sizeof(value));
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "thermalB", value[0]);

			mib_get_s(MIB_HW_THERMAL_C_WLAN0, (void *)value, sizeof(value));
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "thermalC", value[0]);

			mib_get_s(MIB_HW_THERMAL_D_WLAN0, (void *)value, sizeof(value));
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "thermalD", value[0]);
		}else if(hwmib_idx==1){
			mib_get_s(MIB_HW_XCAP_WLAN1_AX, (void *)value, sizeof(value));
#if defined(CONFIG_RTK_DEV_AP)
		#ifdef CONFIG_SHARE_XSTAL
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "xcap", value[0]);
		#else
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "xcap", 0xFF);
		#endif /*CONFIG_SHARE_XSTAL*/
#else
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "xcap", value[0]);
#endif

			mib_get_s(MIB_HW_RFE_TYPE_WLAN1, (void *)value, sizeof(value));
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "rfe", value[0]);

			mib_get_s(MIB_HW_THERMAL_A_WLAN1, (void *)value, sizeof(value));
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "thermalA", value[0]);

			mib_get_s(MIB_HW_THERMAL_B_WLAN1, (void *)value, sizeof(value));
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "thermalB", value[0]);

			mib_get_s(MIB_HW_THERMAL_C_WLAN1, (void *)value, sizeof(value));
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "thermalC", value[0]);

			mib_get_s(MIB_HW_THERMAL_D_WLAN1, (void *)value, sizeof(value));
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "thermalD", value[0]);
		}else
			printf("wrong wlan hw mib idx !!!\n");

		mib_get_s(MIB_HW_CUSTOM_PARA_PATH, (void *)value, sizeof(value));
		iwpriv_cmd(0, wlan_interface, "flash_set", "para_path", value);

		/* RF TSSI DE calibration */
		mib_get_s(MIB_HW_2G_CCK_TSSI_A, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_cck_tssi_A", MAX_2G_TSSI_CCK_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_CCK_TSSI_B, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_cck_tssi_B", MAX_2G_TSSI_CCK_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_CCK_TSSI_C, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_cck_tssi_C", MAX_2G_TSSI_CCK_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_CCK_TSSI_D, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_cck_tssi_D", MAX_2G_TSSI_CCK_SIZE_AX, value);

		mib_get_s(MIB_HW_2G_BW40_1S_TSSI_A, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_bw40_1s_tssi_A", MAX_2G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_BW40_1S_TSSI_B, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_bw40_1s_tssi_B", MAX_2G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_BW40_1S_TSSI_C, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_bw40_1s_tssi_C", MAX_2G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_BW40_1S_TSSI_D, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_bw40_1s_tssi_D", MAX_2G_TSSI_BW40_SIZE_AX, value);

		mib_get_s(MIB_HW_5G_BW40_1S_TSSI_A, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "5G_bw40_1s_tssi_A", MAX_5G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_5G_BW40_1S_TSSI_B, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "5G_bw40_1s_tssi_B", MAX_5G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_5G_BW40_1S_TSSI_C, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "5G_bw40_1s_tssi_C", MAX_5G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_5G_BW40_1S_TSSI_D, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "5G_bw40_1s_tssi_D", MAX_5G_TSSI_BW40_SIZE_AX, value);

#if defined(WLAN_INTF_TSSI_SLOPE_K)
		/* RF TSSI SLOPE K, GAIN DIFF */
		mib_get_s(MIB_HW_2G_CCK_TSSI_GAIN_DIFF_A, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_CCK_GAIN_DIFF_A", MAX_2G_TSSI_CCK_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_CCK_TSSI_GAIN_DIFF_B, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_CCK_GAIN_DIFF_B", MAX_2G_TSSI_CCK_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_CCK_TSSI_GAIN_DIFF_C, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_CCK_GAIN_DIFF_C", MAX_2G_TSSI_CCK_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_CCK_TSSI_GAIN_DIFF_D, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_CCK_GAIN_DIFF_D", MAX_2G_TSSI_CCK_SIZE_AX, value);

		mib_get_s(MIB_HW_2G_TSSI_GAIN_DIFF_A, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_GAIN_DIFF_A", MAX_2G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_TSSI_GAIN_DIFF_B, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_GAIN_DIFF_B", MAX_2G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_TSSI_GAIN_DIFF_C, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_GAIN_DIFF_C", MAX_2G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_TSSI_GAIN_DIFF_D, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_GAIN_DIFF_D", MAX_2G_TSSI_BW40_SIZE_AX, value);

		mib_get_s(MIB_HW_5G_TSSI_GAIN_DIFF_A, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "5G_GAIN_DIFF_A", MAX_5G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_5G_TSSI_GAIN_DIFF_B, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "5G_GAIN_DIFF_B", MAX_5G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_5G_TSSI_GAIN_DIFF_C, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "5G_GAIN_DIFF_C", MAX_5G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_5G_TSSI_GAIN_DIFF_D, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "5G_GAIN_DIFF_D", MAX_5G_TSSI_BW40_SIZE_AX, value);

		/* RF TSSI SLOPE K, CW DIFF */
		mib_get_s(MIB_HW_2G_CCK_TSSI_CW_DIFF_A, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_CCK_CW_DIFF_A", MAX_2G_TSSI_CCK_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_CCK_TSSI_CW_DIFF_B, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_CCK_CW_DIFF_B", MAX_2G_TSSI_CCK_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_CCK_TSSI_CW_DIFF_C, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_CCK_CW_DIFF_C", MAX_2G_TSSI_CCK_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_CCK_TSSI_CW_DIFF_D, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_CCK_CW_DIFF_D", MAX_2G_TSSI_CCK_SIZE_AX, value);

		mib_get_s(MIB_HW_2G_TSSI_CW_DIFF_A, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_CW_DIFF_A", MAX_2G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_TSSI_CW_DIFF_B, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_CW_DIFF_B", MAX_2G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_TSSI_CW_DIFF_C, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_CW_DIFF_C", MAX_2G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_2G_TSSI_CW_DIFF_D, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "2G_CW_DIFF_D", MAX_2G_TSSI_BW40_SIZE_AX, value);

		mib_get_s(MIB_HW_5G_TSSI_CW_DIFF_A, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "5G_CW_DIFF_A", MAX_5G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_5G_TSSI_CW_DIFF_B, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "5G_CW_DIFF_B", MAX_5G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_5G_TSSI_CW_DIFF_C, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "5G_CW_DIFF_C", MAX_5G_TSSI_BW40_SIZE_AX, value);
		mib_get_s(MIB_HW_5G_TSSI_CW_DIFF_D, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS | IWPRIV_AX, wlan_interface, "flash_set", "5G_CW_DIFF_D", MAX_5G_TSSI_BW40_SIZE_AX, value);
#endif

		/* RF 2G RX GAIN K */
		mib_get_s(MIB_HW_RX_GAINGAP_2G_CCK_AB, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "2G_rx_gain_cck", value[0]);
		mib_get_s(MIB_HW_RX_GAINGAP_2G_OFDM_AB, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "2G_rx_gain_ofdm", value[0]);

		/* RF 5G RX GAIN K */
		mib_get_s(MIB_HW_RX_GAINGAP_5GL_AB, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "5G_rx_gain_low", value[0]);
		mib_get_s(MIB_HW_RX_GAINGAP_5GM_AB, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "5G_rx_gain_mid", value[0]);
		mib_get_s(MIB_HW_RX_GAINGAP_5GH_AB, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "5G_rx_gain_high", value[0]);

		mib_get_s(MIB_HW_CHANNEL_PLAN_5G, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "chan_plan", value[0]);
	}
	else
	{
		unsigned char parm[64];
		unsigned char value[1024];
		int mib_index = 0;
		unsigned char phyband = 0, mode = 0;

		mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));

#if !defined(WIFI5_WIFI6_RUNTIME)
		if(phyband != PHYBAND_2G)
		{
			printf("%s(%d) The phyband is wrong!\n");
			return -1;
		}
#endif
		hwmib_idx = read_wlan_hwmib_from(wlan_index);

		if ( mib_get_s( MIB_TX_POWER, (void *)&mode, sizeof(mode)) == 0) {
			printf("Get MIB_TX_POWER error!\n");
		}

		mib_get_s(HW_MIB_FROM(TX_POWER_CCK_A, hwmib_idx), (void *)value, sizeof(value));
		if(value[0] != 0) {
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevelCCK_A", value, mode);
		}
		mib_get_s(HW_MIB_FROM(TX_POWER_CCK_B, hwmib_idx), (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevelCCK_B", value, mode);

		mib_get_s(HW_MIB_FROM(TX_POWER_TSSI_CCK_A, hwmib_idx), (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevel_TSSICCK_A", value, mode);

		mib_get_s(HW_MIB_FROM(TX_POWER_TSSI_CCK_B, hwmib_idx), (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevel_TSSICCK_B", value, mode);

		mib_get_s(HW_MIB_FROM(TX_POWER_HT40_1S_A, hwmib_idx), (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevelHT40_1S_A", value, mode);

		mib_get_s(HW_MIB_FROM(TX_POWER_HT40_1S_B, hwmib_idx), (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevelHT40_1S_B", value, mode);

		mib_get_s(HW_MIB_FROM(TX_POWER_TSSI_HT40_1S_A, hwmib_idx), (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevel_TSSIHT40_1S_A", value, mode);

		mib_get_s(HW_MIB_FROM(TX_POWER_TSSI_HT40_1S_B, hwmib_idx), (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS |IWPRIV_HW2G|IWPRIV_TXPOWER, wlan_interface, "set_mib", "pwrlevel_TSSIHT40_1S_B", value, mode);

		mib_get_s(HW_MIB_FROM(TX_POWER_HT40_2S, hwmib_idx), (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, wlan_interface, "set_mib", "pwrdiffHT40_2S", value);

		mib_get_s(HW_MIB_FROM(TX_POWER_HT20, hwmib_idx), (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, wlan_interface, "set_mib", "pwrdiffHT20", value);

		mib_get_s(HW_MIB_FROM(TX_POWER_DIFF_OFDM, hwmib_idx), (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, wlan_interface, "set_mib", "pwrdiffOFDM", value);

		mib_get_s(HW_MIB_FROM(11N_TSSI_ENABLE, hwmib_idx), (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_INT, wlan_interface, "set_mib", "tssi_enable", value[0]);

		mib_get_s(HW_MIB_FROM(11N_TSSI1, hwmib_idx), (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "set_mib", "tssi1", value[0]);

		mib_get_s(HW_MIB_FROM(11N_TSSI2, hwmib_idx), (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_INT, wlan_interface, "set_mib", "tssi2", value[0]);

		mib_get_s(HW_MIB_FROM(11N_THER, hwmib_idx), (void *)value, sizeof(value));
		if(value[0] != 0) {
			memset(parm, 0, sizeof(parm));
			snprintf(parm, sizeof(parm), "ther=%d", value[0]);
			va_cmd(IWPRIV, 3, 1, (char *) wlan_interface, "set_mib", parm);

			memset(parm, 0, sizeof(parm));
			snprintf(parm, sizeof(parm), "thermal1=%d", value[0]);
			va_cmd(IWPRIV, 3, 1, (char *) wlan_interface, "set_mib", parm);
		}

		mib_get_s(HW_MIB_FROM(11N_THER2, hwmib_idx), (void *)value, sizeof(value));
		if(value[0] != 0) {
			memset(parm, 0, sizeof(parm));
			snprintf(parm, sizeof(parm), "thermal2=%d", value[0]);
			va_cmd(IWPRIV, 3, 1, (char *) wlan_interface, "set_mib", parm);
		}

		mib_get_s(HW_MIB_FROM(RF_XCAP, hwmib_idx), (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_INT, wlan_interface, "set_mib", "xcap", value[0]);


		int tr_mode[1] = {-1};
		//get_WlanHWMIB(mib_index, MIMO_TR_MODE, tr_mode);
		mib_get_s(HW_MIB_FROM(MIMO_TR_MODE, hwmib_idx), tr_mode, sizeof(tr_mode));
		if(tr_mode[0] >= 0) {
			snprintf(parm, sizeof(parm), "MIMO_TR_mode=%d", tr_mode[0]);
			va_cmd(IWPRIV, 3, 1, (char *) wlan_interface, "set_mib", parm);
		}

#if defined(WIFI5_WIFI6_RUNTIME)
		if(phyband == PHYBAND_5G){
			_setup8812Wlan(hwmib_idx, wlan_interface, mode);

#ifdef CONFIG_RF_DPK_SETTING_SUPPORT
			unsigned int  lut_val[PWSF_5G_LEN][LUT_5G_LEN];
			unsigned char is_5g_pdk_patha_ok = 0, is_5g_pdk_pathb_ok = 0;
			unsigned char mib_name[64];
			int mib_id;
			int len=0, i=0;

			if (mib_get_s(MIB_HW_RF_DPK_DP_5G_PATH_A_OK, (void *)&is_5g_pdk_patha_ok, sizeof(is_5g_pdk_patha_ok))) {
				snprintf(parm, sizeof(parm), "is_5g_pdk_patha_ok=%d", is_5g_pdk_patha_ok);
				va_cmd(IWPRIV, 3, 1, wlan_interface, "set_mib", parm);
			}
			if (mib_get_s(MIB_HW_RF_DPK_DP_5G_PATH_B_OK, (void *)&is_5g_pdk_pathb_ok, sizeof(is_5g_pdk_pathb_ok))) {
				snprintf(parm, sizeof(parm), "is_5g_pdk_pathb_ok=%d", is_5g_pdk_pathb_ok);
				va_cmd(IWPRIV, 3, 1, wlan_interface, "set_mib", parm);
			}

			if (is_5g_pdk_patha_ok==1 && is_5g_pdk_pathb_ok==1) {
				len = PWSF_5G_LEN;

				if (mib_get_s(MIB_HW_RF_DPK_PWSF_5G_A, (void *)value, sizeof(value))) {
					iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "pwsf_5g_a", len, value);
				}

				if (mib_get_s(MIB_HW_RF_DPK_PWSF_5G_B, (void *)value, sizeof(value))) {
					iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "pwsf_5g_b", len, value);
				}

				len = LUT_5G_LEN * 4;

				//5G Path A
				memset(lut_val, '\0', sizeof(lut_val));
				for (i=0; i<PWSF_5G_LEN; i++) {
					mib_id = MIB_HW_RF_DPK_LUT_5G_EVEN_A0+i*4;
					if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
						memcpy(lut_val[i], value, LUT_5G_LEN*4);
						LUT_SWAP(lut_val[i], LUT_5G_LEN);
						sprintf(mib_name, "lut_5g_even_a%1d", i);
						iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", mib_name, len, lut_val[i]);
					}
				}
				memset(lut_val, '\0', sizeof(lut_val));
				for (i=0; i<PWSF_5G_LEN; i++) {
					mib_id = MIB_HW_RF_DPK_LUT_5G_ODD_A0+i*4;
					if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
						memcpy(lut_val[i], value, LUT_5G_LEN*4);
						LUT_SWAP(lut_val[i], LUT_5G_LEN);
						sprintf(mib_name, "lut_5g_odd_a%1d", i);
						iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", mib_name, len, lut_val[i]);
					}
				}

				//5G Path B
				memset(lut_val, '\0', sizeof(lut_val));
				for (i=0; i<PWSF_5G_LEN; i++) {
					mib_id = MIB_HW_RF_DPK_LUT_5G_EVEN_B0+i*4;
					if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
						memcpy(lut_val[i], value, LUT_5G_LEN*4);
						LUT_SWAP(lut_val[i], LUT_5G_LEN);
						sprintf(mib_name, "lut_5g_even_b%1d", i);
						iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", mib_name, len, lut_val[i]);
					}
				}
				memset(lut_val, '\0', sizeof(lut_val));
				for (i=0; i<PWSF_5G_LEN; i++) {
					mib_id = MIB_HW_RF_DPK_LUT_5G_ODD_B0+i*4;
					if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
						memcpy(lut_val[i], value, LUT_5G_LEN*4);
						LUT_SWAP(lut_val[i], LUT_5G_LEN);
						sprintf(mib_name, "lut_5g_odd_b%1d", i);
						iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", mib_name, len, lut_val[i]);
					}
				}
			}
#endif
		}
		else
#endif
		{
#ifdef CONFIG_RF_DPK_SETTING_SUPPORT
		unsigned int lut_val[PWSF_2G_LEN][LUT_2G_LEN];

		mib_get_s(MIB_HW_RF_DPK_DP_PATH_A_OK, (void *)&value, sizeof(value));
		snprintf(parm, sizeof(parm), "bDPPathAOK=%d", value[0]);
		va_cmd(IWPRIV, 3, 1, wlan_interface, "set_mib", parm);

		mib_get_s(MIB_HW_RF_DPK_DP_PATH_B_OK, (void *)value, sizeof(value));
		snprintf(parm, sizeof(parm), "bDPPathBOK=%d", value[0]);
		va_cmd(IWPRIV, 3, 1, wlan_interface, "set_mib", parm);

		mib_get_s(MIB_HW_RF_DPK_PWSF_2G_A, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "pwsf_2g_a", PWSF_2G_LEN, value);

		mib_get_s(MIB_HW_RF_DPK_PWSF_2G_B, (void *)value, sizeof(value));
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "pwsf_2g_b", PWSF_2G_LEN, value);

		memset(lut_val, '\0', sizeof(lut_val));
		mib_get_s(MIB_HW_RF_DPK_LUT_2G_EVEN_A0, (void *)value, sizeof(value));
		memcpy(lut_val[0], value, LUT_2G_LEN*4);
		LUT_SWAP(lut_val[0], LUT_2G_LEN);
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "lut_2g_even_a0", LUT_2G_LEN * 4, lut_val[0]);

		mib_get_s(MIB_HW_RF_DPK_LUT_2G_EVEN_A1, (void *)value, sizeof(value));
		memcpy(lut_val[1], value, LUT_2G_LEN*4);
		LUT_SWAP(lut_val[1], LUT_2G_LEN);
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "lut_2g_even_a1", LUT_2G_LEN * 4, lut_val[1]);

		mib_get_s(MIB_HW_RF_DPK_LUT_2G_EVEN_A2, (void *)value, sizeof(value));
		memcpy(lut_val[2], value, LUT_2G_LEN*4);
		LUT_SWAP(lut_val[2], LUT_2G_LEN);
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "lut_2g_even_a2", LUT_2G_LEN * 4, lut_val[2]);

		memset(lut_val, '\0', sizeof(lut_val));
		mib_get_s(MIB_HW_RF_DPK_LUT_2G_ODD_A0, (void *)value, sizeof(value));
		memcpy(lut_val[0], value, LUT_2G_LEN*4);
		LUT_SWAP(lut_val[0], LUT_2G_LEN);
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "lut_2g_odd_a0", LUT_2G_LEN * 4, lut_val[0]);

		mib_get_s(MIB_HW_RF_DPK_LUT_2G_ODD_A1, (void *)value, sizeof(value));
		memcpy(lut_val[1], value, LUT_2G_LEN*4);
		LUT_SWAP(lut_val[1], LUT_2G_LEN);
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "lut_2g_odd_a1", LUT_2G_LEN * 4, lut_val[1]);

		mib_get_s(MIB_HW_RF_DPK_LUT_2G_ODD_A2, (void *)value, sizeof(value));
		memcpy(lut_val[2], value, LUT_2G_LEN*4);
		LUT_SWAP(lut_val[2], LUT_2G_LEN);
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "lut_2g_odd_a2", LUT_2G_LEN * 4, lut_val[2]);

		memset(lut_val, '\0', sizeof(lut_val));
		mib_get_s(MIB_HW_RF_DPK_LUT_2G_EVEN_B0, (void *)value, sizeof(value));
		memcpy(lut_val[0], value, LUT_2G_LEN*4);
		LUT_SWAP(lut_val[0], LUT_2G_LEN);
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "lut_2g_even_b0", LUT_2G_LEN * 4, lut_val[0]);

		mib_get_s(MIB_HW_RF_DPK_LUT_2G_EVEN_B1, (void *)value, sizeof(value));
		memcpy(lut_val[1], value, LUT_2G_LEN*4);
		LUT_SWAP(lut_val[1], LUT_2G_LEN);
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "lut_2g_even_b1", LUT_2G_LEN * 4, lut_val[1]);

		mib_get_s(MIB_HW_RF_DPK_LUT_2G_EVEN_B2, (void *)value, sizeof(value));
		memcpy(lut_val[2], value, LUT_2G_LEN*4);
		LUT_SWAP(lut_val[2], LUT_2G_LEN);
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "lut_2g_even_b2", LUT_2G_LEN * 4, lut_val[2]);

		memset(lut_val, '\0', sizeof(lut_val));
		mib_get_s(MIB_HW_RF_DPK_LUT_2G_ODD_B0, (void *)value, sizeof(value));
		memcpy(lut_val[0], value, LUT_2G_LEN*4);
		LUT_SWAP(lut_val[0], LUT_2G_LEN);
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "lut_2g_odd_b0", LUT_2G_LEN * 4, lut_val[0]);

		mib_get_s(MIB_HW_RF_DPK_LUT_2G_ODD_B1, (void *)value, sizeof(value));
		memcpy(lut_val[1], value, LUT_2G_LEN*4);
		LUT_SWAP(lut_val[1], LUT_2G_LEN);
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "lut_2g_odd_b1", LUT_2G_LEN * 4, lut_val[1]);

		mib_get_s(MIB_HW_RF_DPK_LUT_2G_ODD_B2, (void *)value, sizeof(value));
		memcpy(lut_val[2], value, LUT_2G_LEN*4);
		LUT_SWAP(lut_val[2], LUT_2G_LEN);
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, wlan_interface, "set_mib", "lut_2g_odd_b2", LUT_2G_LEN * 4, lut_val[2]);
#endif
		}
	}
#endif /*RTW_MP_DATA*/
	return 0;
}

int check_wlan_mib()
{
	int wlan_idx_bak = wlan_idx;
	MIB_CE_MBSSIB_T Entry;
	int i=0, j=0, syncRoot=0, ax_flag=0;

	for (i = 0; i < NUM_WLAN_INTERFACE; i++)
	{
		memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));
		mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, 0, (void *)&Entry);

		//Change the is_ax_support value of vaps
		if(Entry.is_ax_support == 1){
			for (j = 1; j <= NUM_VWLAN_INTERFACE; j++) {
				mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&Entry);
				if (Entry.is_ax_support != 1) {
					Entry.is_ax_support = 1;
				}
				mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, j);
			}
		}

		//Sync root mib
		wlan_idx = i;
		if(syncRoot != 1)
			syncRoot = check_sync_info();

		//Set hw mib
		ax_flag = Entry.is_ax_support;
		if(set_hw_mib(ax_flag, wlan_idx))
			printf("%s (%d) Set hw mib failed!\n", __FUNCTION__, __LINE__);
	}
	
#ifdef CONFIG_RTIC
	setup_wlan_hs_rtic();
#endif

	if(syncRoot){
		printf("[%s]Sync wlan root mib from table\n", __FUNCTION__);
		for(j=0; j<NUM_WLAN_INTERFACE; ++j)
		{
			memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));
			wlan_idx = j;
			mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry);
			MBSSID_SetRootEntry(&Entry);
		}
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	}

	wlan_idx = wlan_idx_bak;

	return 0;
}
#endif
#endif

#ifdef CONFIG_USER_CTCAPD
#ifdef WLAN_WPS_HAPD
void hapd_wps_trigger_process_for_ctcapd(void)
{
	FILE *fp;
	char line[20]={0};
	char tmpbuf;
	char intfname[16]={0}, tmp_buffer[50]={0};
	int n=0, wps_pid = -1, i;
	MIB_CE_MBSSIB_T Entry;
#ifdef RTK_MULTI_AP
	unsigned char mode=0;
	FILE *fp2;
	int wps_status=1;
#endif

	if((fp = fopen("tmp/wscd_trigger", "r")) != NULL)
	{
		if(fgets(line, sizeof(line), fp))
		{
			if (sscanf(line, "%c", &tmpbuf))
			{
				if (tmpbuf == '0')
				{
					system("ubus send \"propertieschanged\" \'{\"wpsswitch\":\"off\"}\'");
				}
				else if(tmpbuf == '1')
				{
					system("ubus send \"propertieschanged\" \'{\"wpsswitch\":\"on\"}\'");
#ifdef RTK_MULTI_AP
					mib_get_s(MIB_MAP_CONTROLLER, (void *)&(mode), sizeof(mode));
					if(mode)
					{
						system("echo 2 > /tmp/virtual_push_button");
						for(i=0;i<3;i++)
						{
							wps_status = 1;

							if(rtk_wlan_get_all_wps_status())
							{
								wps_status = 0;
							}
							else
								wps_status = 1;

							if(wps_status == 0)
								break;
							else
							{
								system("echo 2 > /tmp/virtual_push_button");
							}

							sleep(1);
						}
					}
					else
					{
#endif
						for(i=0; i<NUM_WLAN_INTERFACE; i++)
						{
							memset(intfname, 0, sizeof(intfname));
							snprintf(intfname,sizeof(intfname),WLAN_IF,i);

							memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
							mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_ROOT_ITF_INDEX, (void *)&Entry);
							if(Entry.wlanDisabled)
								continue;

#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
							if(Entry.wsc_disabled)
								continue;
#endif
							for(n=0; n<2; n++)
							{
								wps_pid = findPidByName("hostapd");
								if(wps_pid <= 0)
								{
									break;
								}

								sleep(1);
							}

							if(n==2 && wps_pid>0)
							{
								snprintf(tmp_buffer,sizeof(tmp_buffer),"/bin/hostapd_cli wps_pbc -i %s",intfname);
								system(tmp_buffer);
							}
						}
#ifdef RTK_MULTI_AP
					}
#endif
				}

			}
		}

		fclose(fp);
		fp = NULL;
		unlink("tmp/wscd_trigger");
	}

}
#endif
#endif

static int _rtk_wlan_do_after_interface_up(void)
{
#if defined(CONFIG_USER_DBUS_PROXY)
	int i=0;
	char ifname[IFNAMSIZ]={0};
	for(i=0; i< NUM_WLAN_INTERFACE; i++){
		rtk_wlan_get_ifname(i, WLAN_ROOT_ITF_INDEX, ifname);
		if(rtk_wlan_is_ax_support(ifname)==1)
			rtk_wlan_channel_change_event(ifname);
	}
#endif
	return 0;
}

#ifdef MULTI_AP_OPEN_ENCRYPT_SUPPORT

#define SUPPORT_OPEN_ENCRYPT 		1
#define NOT_SUPPORT_OPEN_ENCRYPT 	0

int write_backhaul_encrypt_mode_flag(void)
{
	FILE *fp=NULL;
	unsigned char backhaul_encrypt_none_en = 0;
	mib_get_s(MIB_MAP_BACKHAUL_ENCRYPT_NONE, (void *)&backhaul_encrypt_none_en,sizeof(backhaul_encrypt_none_en));

	if((fp = fopen("/tmp/backhaul_encrypt_mode", "w")) == NULL)
	{
		printf("***** Open file /tmp/backhaul_encrypt_mode failed !\n");
		return -1;
	}
	fprintf(fp, "%d\n", backhaul_encrypt_none_en);
	fclose(fp);
	return 0;

}

#endif

int startWLan(config_wlan_target target, config_wlan_ssid ssid_index)
{
	int status = 0;
	int orig_wlan_idx;
//#if IS_AX_SUPPORT
	int i = 0, j = 0;
	unsigned char phy_band_select;
	MIB_CE_MBSSIB_T Entry;
//#endif
#ifdef RTK_WLAN_MANAGER
	unsigned char stactrl_enable=0;
	unsigned char crossband_enable=0;
#endif

#ifdef WLAN_ROOT_MIB_SYNC
#ifdef WLAN_SUPPORT
	check_wlan_mib();
#endif
#endif

#if !defined(YUEME_3_0_SPEC) && !defined(CONFIG_CU_BASEON_YUEME)
	if(!check_wlan_module()){
		return 0;
	}
#endif

#ifdef WLAN_WPS_HAPD
	if(access(HOSTAPD_WPS_SCRIPT, F_OK))
		status |= writeWpsScript("/etc/hostapd_wps.sh", HOSTAPD_WPS_SCRIPT); // etc/hostapd_wps.sh in user/hostapd/files
#endif
#ifdef WLAN_WPS_WPAS
	if(access(WPAS_WPS_SCRIPT, F_OK))
		status |= writeWpsScript("/etc/wpa_supplicant_wps.sh", WPAS_WPS_SCRIPT); // etc/wpa_supplicant_wps.sh in user/wpa_supplicant/files
#endif
	if(access(HOSTAPD_GENERAL_SCRIPT, F_OK))
		status |= writeHostapdGeneralScript("/etc/hostapd_general_event.sh", HOSTAPD_GENERAL_SCRIPT); // etc/hostapd_general_event.sh in user/hostapd/files

	if(access(WPAS_GENERAL_SCRIPT, F_OK))
		status |= writeHostapdGeneralScript("/etc/wpa_supplicant_general_event.sh", WPAS_GENERAL_SCRIPT); // etc/wpa_supplicant_general_event.sh in user/wpa_supplicant/files

	orig_wlan_idx = wlan_idx;

#ifdef MULTI_AP_OPEN_ENCRYPT_SUPPORT
	write_backhaul_encrypt_mode_flag();
	setupMultiAPController_encrypt();
#endif

//#if IS_AX_SUPPORT
	for(i = 0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		mib_local_mapping_get( MIB_WLAN_PHY_BAND_SELECT, i, (void *)&phy_band_select);

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
		checkWlanSetting(i);
#endif
		if((target == CONFIG_WLAN_2G && phy_band_select == PHYBAND_5G)
		  || (target == CONFIG_WLAN_5G && phy_band_select == PHYBAND_2G)) {
			continue;
		}

		if(ssid_index==CONFIG_SSID_ALL || ssid_index==CONFIG_SSID_ROOT)
#ifdef WLAN_NOT_USE_MIBSYNC
			setup_wlan_priv_mib_or_proc(i, WLAN_ROOT_ITF_INDEX);
#else
			start_wifi_priv_cfg(i ,WLAN_ROOT_ITF_INDEX);
#endif

		memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
		mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_ROOT_ITF_INDEX, (void *)&Entry);
		if(Entry.is_ax_support)
			start_hapd_wpas_process(i, ssid_index);

		for (j=WLAN_VAP_ITF_INDEX; j<(WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM); j++){
			if(ssid_index!=CONFIG_SSID_ALL && ssid_index!=j) {
				continue;
			}
#ifdef WLAN_NOT_USE_MIBSYNC
			setup_wlan_priv_mib_or_proc(i, j);
#else
			start_wifi_priv_cfg(i, j);
			set_wifi_misc(i, j);
#endif
		}

		if(!Entry.is_ax_support)
			start_hapd_wpas_process(i, ssid_index);

#ifdef WLAN_NOT_USE_MIBSYNC
		for (j=WLAN_VAP_ITF_INDEX; j<(WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM); j++){
			if(ssid_index!=CONFIG_SSID_ALL && ssid_index!=j) {
				continue;
			}
			setup_wlan_post_process(i, j);
		}
#endif


#ifdef WLAN_NOT_USE_MIBSYNC
		setup_wlan_post_process(i, WLAN_ROOT_ITF_INDEX);
#else
		set_wifi_misc(i, 0);
#endif

#ifdef WLAN_UNIVERSAL_REPEATER
#ifdef WLAN_NOT_USE_MIBSYNC
		setup_wlan_post_process(i, WLAN_REPEATER_ITF_INDEX);
#else
		set_wifi_misc(i, WLAN_REPEATER_ITF_INDEX);
#endif
#endif
#ifdef CONFIG_YUEME
		setupWlanVSIE(i, phy_band_select); //must set after interface up
#endif
	}

#ifdef CONFIG_RTIC
        if (read_pid("/var/run/rtic.pid") <= 0)
                va_cmd("/bin/hostapd", 4, 1, "/etc/conf/hapd_rtic.conf", "-P", "/var/run/rtic.pid", "-B");
#endif

#ifdef RTK_HAPD_MULTI_AP
	rtk_start_multi_ap_service();
#endif
//#endif

#ifdef RTK_WLAN_MANAGER	
#ifdef BAND_STEERING_SUPPORT
	mib_get_s(MIB_WIFI_STA_CONTROL_ENABLE, (void *)&stactrl_enable, sizeof(stactrl_enable));
#endif
#ifdef RTK_CROSSBAND_REPEATER
	mib_get_s(MIB_CROSSBAND_ENABLE, (void *)&crossband_enable, sizeof(crossband_enable));
#endif
	if(stactrl_enable || crossband_enable){
		rtk_start_wlan_manager();
	}
#endif

#ifdef WLAN_BAND_STEERING
	rtk_wlan_setup_band_steering();
#endif

#if 1//def WLAN_MBSSID
	setup_wlan_block();
#endif
#ifdef _PRMT_X_CMCC_WLANSHARE_
	setup_wlan_share();
#endif

#ifdef CONFIG_GUEST_ACCESS_SUPPORT
	setup_wlan_guestaccess();
#endif

#ifdef WLAN_SCHEDULE_SUPPORT
	start_func_wlan_schedule();
#endif

	wlan_idx = orig_wlan_idx;

#ifdef CONFIG_USER_CTCAPD
#ifdef WLAN_WPS_HAPD
	hapd_wps_trigger_process_for_ctcapd();
#endif
#endif

#ifdef WLAN_SMARTAPP_ENABLE
	status |= start_smart_wlanapp();
#endif

	status |= _rtk_wlan_do_after_interface_up();

	return status;
}

int startWLan_with_lock(config_wlan_target target, config_wlan_ssid ssid_index)
{
	int lockfd, ret = 0;
	LOCK_WLAN();
	ret = startWLan(target, ssid_index);
	UNLOCK_WLAN();
	return ret;
}

char* config_WLAN_action_type_to_str( int action_type, config_wlan_ssid ssid_index, char *ret )
{
	char action_type_str[128] = {0};
	char ssid_index_str[128] = {0};
	char band_type_str[32] = {0};
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	char ifname[16];
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	if(action_type==ACT_START_2G || action_type==ACT_RESTART_2G)
		get_ifname_by_ssid_index(ssid_index+1, ifname);
	else if(action_type==ACT_START_5G || action_type==ACT_RESTART_5G)
		get_ifname_by_ssid_index(ssid_index+5, ifname);
	//va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=0");
#endif

	switch( action_type )
	{
		case ACT_START:
#ifdef WLAN_DUALBAND_CONCURRENT
			sprintf(action_type_str, "WLAN start 2G+5G");
#else
			sprintf(action_type_str, "WLAN start");
#endif
			break;
		case ACT_RESTART:
#ifdef WLAN_DUALBAND_CONCURRENT
			sprintf(action_type_str, "WLAN restart 2G+5G");
#else
			sprintf(action_type_str, "WLAN restart");
#endif
			break;
		case ACT_STOP:
#ifdef WLAN_DUALBAND_CONCURRENT
			sprintf(action_type_str, "WLAN stop 2G+5G");
#else
			sprintf(action_type_str, "WLAN stop");
#endif
			break;
		case ACT_START_2G:
			sprintf(action_type_str, "WLAN start 2G");
			sprintf(band_type_str, "2G");
			break;
		case ACT_RESTART_2G:
			sprintf(action_type_str, "WLAN restart 2G");
			sprintf(band_type_str, "2G");
			break;
		case ACT_STOP_2G:
			sprintf(action_type_str, "WLAN stop 2G");
			sprintf(band_type_str, "2G");
			break;
		case ACT_START_5G:
			sprintf(action_type_str, "WLAN start 5G");
			sprintf(band_type_str, "5G");
			break;
		case ACT_RESTART_5G:
			sprintf(action_type_str, "WLAN restart 5G");
			sprintf(band_type_str, "5G");
			break;
		case ACT_STOP_5G:
			sprintf(action_type_str, "WLAN stop 5G");
			sprintf(band_type_str, "5G");
			break;
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
		case ACT_RESTART_AND_WPS:
			sprintf(action_type_str, "WLAN restart 2G+5G+WPS");
			break;
		case ACT_RESTART_WPS:
			sprintf(action_type_str, "WLAN restart WPS");
			break;
#endif
		default:
			sprintf(ret, "Invalid action_type !");
			return ret;
	}

	switch( ssid_index )
	{
#ifdef WLAN_USE_VAP_AS_SSID1
		case CONFIG_SSID_ROOT:
			sprintf(ssid_index_str, "root (%s-%d)", band_type_str, ssid_index+1);
			break;
		case CONFIG_SSID1:
		case CONFIG_SSID2:
		case CONFIG_SSID3:
		case CONFIG_SSID4:
		case CONFIG_SSID5:
		case CONFIG_SSID6:
		case CONFIG_SSID7:
			sprintf(ssid_index_str, "vap%d (%s-%d)", ssid_index, band_type_str, ssid_index+1);
			break;
		case CONFIG_SSID_ALL:
			sprintf(ssid_index_str, "all");
			break;
#else
		case CONFIG_SSID_ROOT: //CONFIG_SSID1
			sprintf(ssid_index_str, "root (%s-1)", band_type_str);
			break;
		case CONFIG_SSID2:
		case CONFIG_SSID3:
		case CONFIG_SSID4:
		case CONFIG_SSID5:
		case CONFIG_SSID6:
		case CONFIG_SSID7:
		case CONFIG_SSID8:
			sprintf(ssid_index_str, "vap%d (%s-%d)", ssid_index, band_type_str, ssid_index);
			break;
		case CONFIG_SSID_ALL:
			sprintf(ssid_index_str, "all");
			break;
#endif
		default:
			sprintf(ret, "Invalid ssid_index !");
			return ret;
	}

	sprintf(ret, "%s %s", action_type_str, ssid_index_str);
	return ret;

}

int config_WLAN_by_fork(int action_type, config_wlan_ssid ssid_index)
{
	pid_t pid, pid2;
	pid = fork();

	if(pid == 0)
	{
		pid2 = fork();

		if(pid2 == 0)
		{
			mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
			config_WLAN(action_type, ssid_index);
			exit(0);
		}
		else if(pid2 > 0)
		{
			exit(0);
		}
	}
	else if(pid > 0)
	{
		waitpid(pid, NULL, 0);
	}

	return 0;

}

#if defined(CONFIG_USER_DBUS_PROXY)
#include <rtk/smart_proxy_common.h>
#endif
int rtk_wlan_channel_change_event(char *ifname)
{
#if defined(CONFIG_USER_DBUS_PROXY)
	mib2dbus_notify_app_t notifyMsg;
	memset((char*)&notifyMsg, 0, sizeof(notifyMsg));
	notifyMsg.table_id = COM_CTC_SMART_APP_WLAN_CHAN_TAB;
	notifyMsg.Signal_id = e_dbus_proxy_signal_wlan_channel_changed_id;
	memcpy(notifyMsg.content, ifname, strlen(ifname));
	mib_2_dbus_notify_dbus_api(&notifyMsg);
#endif
	return 0;
}

int config_WLAN( int action_type, config_wlan_ssid ssid_index)
{
	int lockfd, orig_wlan_idx, ret=0;
	char action_type_str[128] = {0};
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	char ifname[IFNAMSIZ] = {0};
#endif

	LOCK_WLAN();

#ifdef WLAN_CTC_MULTI_AP
	unsigned char map_mode = 0;
	FILE *fp = NULL;
	char str_buf[256]={0}, progname[64]={0};
	mib_get(MIB_MAP_CONTROLLER, &map_mode);
	if((map_mode == MAP_AGENT) && (findPidByName("map_agent") > 0)){
		fp = fopen("/proc/self/status", "r");
		if(fp){
			if(fgets(str_buf, sizeof(str_buf), fp)!=NULL){
				//printf("%s", str_buf);
				sscanf(str_buf, "Name:%*[ \t]%s", progname);
				//printf("prog: %s\n", progname);
			}
			fclose(fp);
			if(strcmp(progname, "sysconf")){
				snprintf(str_buf, sizeof(str_buf), "echo z 9 > /tmp/map_command");
				va_cmd("/bin/sh", 2, 1, "-c", str_buf);
				UNLOCK_WLAN();
				return ret;
			}
		}
	}
#endif

	AUG_PRT("%s %s\n", __func__, config_WLAN_action_type_to_str(action_type, ssid_index, action_type_str));
	syslog(LOG_INFO, "%s", config_WLAN_action_type_to_str(action_type, ssid_index, action_type_str));

	//Chk_WLAN_LED_ST(action_type, ssid_index, 0);

	orig_wlan_idx = wlan_idx;
#ifdef CONFIG_USER_BRIDGE_GROUPING
	setup_bridge_grouping(DEL_RULE);
#endif
#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
	checkWlanRootChange(&action_type, &ssid_index);
#endif
	switch( action_type )
	{
	case ACT_START:
		startWLan(CONFIG_WLAN_ALL, ssid_index);
		break;

	case ACT_RESTART:
		stopwlan(CONFIG_WLAN_ALL, ssid_index);
		startWLan(CONFIG_WLAN_ALL, ssid_index);
		break;

	case ACT_STOP:
		stopwlan(CONFIG_WLAN_ALL, ssid_index);
		break;

	case ACT_START_2G:
		startWLan(CONFIG_WLAN_2G, ssid_index);
		break;

	case ACT_RESTART_2G:
		stopwlan(CONFIG_WLAN_2G, ssid_index);
		startWLan(CONFIG_WLAN_2G, ssid_index);
		break;

	case ACT_STOP_2G:
		stopwlan(CONFIG_WLAN_2G, ssid_index);
		break;

	case ACT_START_5G:
		startWLan(CONFIG_WLAN_5G, ssid_index);
		break;

	case ACT_RESTART_5G:
		stopwlan(CONFIG_WLAN_5G, ssid_index);
		startWLan(CONFIG_WLAN_5G, ssid_index);
		break;

	case ACT_STOP_5G:
		stopwlan(CONFIG_WLAN_5G, ssid_index);
		break;
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	case ACT_RESTART_AND_WPS:
		stopwlan(CONFIG_WLAN_ALL, ssid_index);
		startWLan(CONFIG_WLAN_ALL, ssid_index);
		rtk_wlan_get_ifname(wlan_idx,0,ifname);
		va_niced_cmd(HOSTAPD_CLI, 3 , 1 , "wps_pbc", "-i", ifname);
		//va_cmd("/bin/wscd", 3, 1, "-sig_pbc",ifname);
		printf("trigger PBC to %s\n",ifname);
		break;
#ifdef WLAN_WPS_VAP
	case ACT_RESTART_WPS:
		//UNLOCK_WLAN();
		//restartWPS(0);
		//LOCK_WLAN();
		rtk_wlan_get_ifname(wlan_idx,0,ifname);
		va_niced_cmd(HOSTAPD_CLI, 3 , 1 , "wps_pbc", "-i", ifname);
		//va_cmd("/bin/wscd", 3, 1, "-sig_pbc",ifname);
		printf("trigger PBC to %s\n",ifname);
		break;
#endif
#endif

	default:
		ret = -1;
		break;
	}
#ifdef CONFIG_USER_BRIDGE_GROUPING
	setup_bridge_grouping(ADD_RULE);
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rtk_igmp_mld_snooping_module_init(BRIF, CONFIGALL, NULL);
#endif

#ifdef CONFIG_RTK_L34_ENABLE // Rostelecom, Port Binding function
	unsigned int set_wanlist = 0;

	if (set_port_binding_mask(&set_wanlist) > 0)
	{
		rg_set_port_binding_mask(set_wanlist);
	}
#ifdef CONFIG_00R0
	// update DNS info
	restart_dnsrelay_ex("all", 0);
	sleep(1);       //wait a second for DNS updating
#endif
#endif
#endif
	wlan_idx = orig_wlan_idx;
#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
	mib_backup(CONFIG_MIB_ALL);
#endif

	//Chk_WLAN_LED_ST(action_type, ssid_index, 1);

	UNLOCK_WLAN();
#if defined(CONFIG_IPV6) && defined(CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR)
    //Port/VLAN mapping need re-config WLAN interface ipv6 proc_config/policy route/ LL and global address.
    restartLanV6Server();
#endif
	return ret;
}

#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
typedef struct {
	int mib_id;
	int size;
	int do_down_up;
} _WL_CHK_RADIO_TBL_;
_WL_CHK_RADIO_TBL_ wlChkRadioTbl[] = {
	{MIB_WLAN_DISABLED,				_SIZE(wlanDisabled), 			1},
	{MIB_WLAN_BASIC_RATE,			_SIZE(basicRates), 				1},
	{MIB_WLAN_SUPPORTED_RATE,		_SIZE(supportedRates), 			1},
	{MIB_WLAN_RTS_THRESHOLD,		_SIZE(rtsThreshold), 			0},
	{MIB_WLAN_FRAG_THRESHOLD,		_SIZE(fragThreshold), 			0},
	{MIB_WLAN_PREAMBLE_TYPE,		_SIZE(preambleType), 			0},
	{MIB_WLAN_INACTIVITY_TIME,		_SIZE(inactivityTime), 			0},
	{MIB_WLAN_DTIM_PERIOD,			_SIZE(dtimPeriod), 				0},
	{MIB_TX_POWER,					_SIZE(txPower), 				1},
#ifdef CONFIG_CU
	{MIB_FAKE_TX_POWER,				_SIZE(fakeTxPower), 			1},
#endif
	{MIB_WLAN_TX_POWER_HIGH,		_SIZE(txPower_high), 			1},
	{MIB_WLAN_BEACON_INTERVAL,		_SIZE(beaconInterval), 			1},
	{MIB_WLAN_PROTECTION_DISABLED,	_SIZE(wlanProtectionDisabled), 	0},
	{MIB_WLAN_SHORTGI_ENABLED,		_SIZE(wlanShortGIEnabled), 		0},
	{MIB_WLAN_AUTO_CHAN_ENABLED, 	_SIZE(auto_channel), 			1},
	{MIB_WLAN_CHAN_NUM,			 	_SIZE(channel), 				1},
	{MIB_WLAN_CONTROL_BAND,			_SIZE(wlanContrlBand), 			1},
	{MIB_WLAN_CHANNEL_WIDTH,		_SIZE(wlanChannelWidth), 		1},
	{MIB_WLAN_11N_COEXIST,			_SIZE(wlan11nCoexist), 			1},
#if defined(WLAN_BAND_STEERING)
	{MIB_WIFI_STA_CONTROL,				_SIZE(wifi_sta_control),		1},
#endif
#ifdef WLAN_RATE_PRIOR
	{MIB_WLAN_RATE_PRIOR,			_SIZE(wlan_rate_prior),			1},
#endif
	{MIB_WIFI_MODULE_DISABLED, 			_SIZE(wifi_module_disabled),	1}
};

static int checkWlanRootChange(int *p_action_type, config_wlan_ssid *p_ssid_index)
{
	_wlIdxSsidDoChange = 0;
	_wlIdxRadioDoChange = 0;
	if ((*p_action_type == ACT_START || *p_action_type == ACT_RESTART || *p_action_type == ACT_STOP ||
		*p_action_type == ACT_START_2G || *p_action_type == ACT_RESTART_2G || *p_action_type == ACT_STOP_2G ||
		*p_action_type == ACT_START_5G || *p_action_type == ACT_RESTART_5G || *p_action_type == ACT_STOP_5G) &&
		*p_ssid_index == CONFIG_SSID_ROOT)
		*p_ssid_index = CONFIG_SSID_ALL;
	if((*p_action_type == ACT_RESTART || *p_action_type == ACT_RESTART_2G || *p_action_type == ACT_RESTART_5G))
	{
		void *ptr = NULL, *bptr = NULL;
		int i = 0, idx = 0, size = 0, lsize = 0, match = 0, change = 0;
		//char buf[512], *pstr = NULL;
		_WL_CHK_RADIO_TBL_ *chkEntry;
		unsigned char phy_band_select=0;

		for(idx=0;idx<NUM_WLAN_INTERFACE;idx++)
		{
			mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, idx, &phy_band_select);
			/*
			if((*p_action_type == ACT_RESTART_2G && phy_band_select == PHYBAND_5G)
		|| (*p_action_type == ACT_RESTART_5G && phy_band_select == PHYBAND_2G)) {
				continue;
		}
			*/

			// Check WLAN Radio Change
			change = 0;
			size = sizeof(wlChkRadioTbl) / sizeof(_WL_CHK_RADIO_TBL_);
			lsize = 0;
			for(i=0;i<size;i++)
			{
				chkEntry = &wlChkRadioTbl[i];
				if(chkEntry->size > lsize)
				{
					ptr = (ptr) ? realloc(ptr, chkEntry->size) : malloc(chkEntry->size);
					bptr = (bptr) ? realloc(bptr, chkEntry->size) : malloc(chkEntry->size);
					lsize = chkEntry->size;
				}

				if(ptr && bptr)
				{
					memset(ptr, 0, lsize);
					memset(bptr, 0, lsize);
					if(mib_local_mapping_get(chkEntry->mib_id, idx, ptr) &&
						mib_local_mapping_backup_get(chkEntry->mib_id, idx, bptr))
					{

						match = 0;
						if(memcmp(ptr, bptr, chkEntry->size) == 0)
							match = 1;
						//printf("===> MIB_ID[%d]: match=%d, do_up=%d\n", chkEntry->mib_id, match, chkEntry->do_down_up);
						if(match == 0) change |= chkEntry->do_down_up;
					}
				}
				else
				{
					if(ptr){ free(ptr); ptr = NULL; }
					if(bptr){ free(bptr); bptr = NULL; }
					lsize = 0;
					change |= 1;
				}

				if(change) break;
			}

			if(ptr){ free(ptr); ptr = NULL; }
			if(bptr){ free(bptr); bptr = NULL; }
			lsize = 0;

			// Check WLAN per SSID Change
			if(change)
			{
				_wlIdxRadioDoChange |= 1 << idx;
#ifdef	WLAN_MBSSID
				for(i=0; i<=NUM_VWLAN_INTERFACE; i++)
				{
					_wlIdxSsidDoChange |= (1 << (i+idx*(NUM_VWLAN_INTERFACE+1)));
				}
				*p_ssid_index = CONFIG_SSID_ALL;
#endif
				if((*p_action_type == ACT_RESTART_2G) && (phy_band_select == PHYBAND_5G))
					*p_action_type = ACT_RESTART;
				else if((*p_action_type == ACT_RESTART_5G) && (phy_band_select == PHYBAND_2G))
					*p_action_type = ACT_RESTART;
			}
			else
			{
#ifdef	WLAN_MBSSID
				lsize = sizeof(MIB_CE_MBSSIB_T);
				ptr = calloc(lsize, 1);
				bptr = calloc(lsize, 1);
				if(ptr && bptr)
				{
					// check root wlan SSID change
					for(i=0; i<=NUM_VWLAN_INTERFACE; i++)
					{
						// if root ssid has change , other ssid set change bit
						if((i!=0) && (_wlIdxSsidDoChange & (1 << (0+idx*(NUM_VWLAN_INTERFACE+1)))))
							_wlIdxSsidDoChange |= (1 << (i+idx*(NUM_VWLAN_INTERFACE+1)));
						else if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, idx, i, ptr) &&
							mib_chain_local_mapping_backup_get(MIB_MBSSIB_TBL, idx, i, bptr))
						{
							match = 0;
							if(memcmp(ptr, bptr, lsize) == 0)
								match = 1;
							if(match == 0)
							{
								_wlIdxSsidDoChange |= (1 << (i+idx*(NUM_VWLAN_INTERFACE+1)));
								if(i == 0) *p_ssid_index = CONFIG_SSID_ALL;

								if((*p_action_type == ACT_RESTART_2G) && (phy_band_select == PHYBAND_5G))
									*p_action_type = ACT_RESTART;
								else if((*p_action_type == ACT_RESTART_5G) && (phy_band_select == PHYBAND_2G))
									*p_action_type = ACT_RESTART;
							}
						}
					}
				}
				if(ptr){ free(ptr); ptr = NULL; }
				if(bptr){ free(bptr); bptr = NULL; }
#endif
			}
		}

		printf(">>>[%s] action_type=%d, ssid_index=%d, RadioDoChange=0x%04x, SsidDoChange=0x%04x\n",
				__FUNCTION__, *p_action_type, *p_ssid_index, _wlIdxRadioDoChange, _wlIdxSsidDoChange);
	}
	else if((*p_ssid_index!=CONFIG_SSID_ALL) && (*p_action_type == ACT_STOP_2G || *p_action_type == ACT_STOP_5G))
	{
		int idx = 0;
		unsigned char phy_band_select = 0;
		for(idx=0;idx<NUM_WLAN_INTERFACE;idx++)
		{
			mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, idx, &phy_band_select);
			if(((phy_band_select==PHYBAND_2G) && (*p_action_type == ACT_STOP_2G))
				|| ((phy_band_select==PHYBAND_5G) && (*p_action_type == ACT_STOP_5G)))
				_wlIdxSsidDoChange |= (1<<((*p_ssid_index)+ idx*(NUM_VWLAN_INTERFACE+1)));
		}

		printf(">>>[%s] action_type=%d, ssid_index=%d, RadioDoChange=0x%04x, SsidDoChange=0x%04x\n",
				__FUNCTION__, *p_action_type, *p_ssid_index, _wlIdxRadioDoChange, _wlIdxSsidDoChange);
	}
	return 0;
}
#endif

#if defined(SWAP_WLAN_MIB)
int getWlanHSMib_K(WLAN_HS_K_VALUE_Tp pValue    , int wlan_idx)
{
	int start_ixd_2g=0, start_idx_5g=35;

	if(!pValue){
		printf("Invalid pValue!\n");
		return -1;
	}

	if(wlan_idx==0){
		mib_get_s(MIB_HW_TX_POWER_CCK_A, (void *)pValue->CCK_A, sizeof(pValue->CCK_A)); 	//2g k
		mib_get_s(MIB_HW_TX_POWER_5G_HT40_1S_A, (void *)pValue->HT40_5G_1S_A, sizeof(pValue->HT40_5G_1S_A)); //5g k
		mib_get_s(MIB_HW_TX_POWER_TSSI_CCK_A, (void *)pValue->TSSI_CCK_A, sizeof(pValue->TSSI_CCK_A)); //2g k
		mib_get_s(MIB_HW_TX_POWER_TSSI_5G_HT40_1S_A, (void *)pValue->TSSI_HT40_5G_1S_A, sizeof(pValue->TSSI_HT40_5G_1S_A)); //5g k
	}else if(wlan_idx==1){
		mib_get_s(MIB_HW_WLAN1_TX_POWER_CCK_A, (void *)pValue->CCK_A, sizeof(pValue->CCK_A)); 	//2g k
		mib_get_s(MIB_HW_WLAN1_TX_POWER_5G_HT40_1S_A, (void *)pValue->HT40_5G_1S_A, sizeof(pValue->HT40_5G_1S_A)); //5g k
		mib_get_s(MIB_HW_WLAN1_TX_POWER_TSSI_CCK_A, (void *)pValue->TSSI_CCK_A, sizeof(pValue->TSSI_CCK_A)); //2g k
		mib_get_s(MIB_HW_WLAN1_TX_POWER_TSSI_5G_HT40_1S_A, (void *)pValue->TSSI_HT40_5G_1S_A, sizeof(pValue->TSSI_HT40_5G_1S_A)); //5g k
	}

	printf("Wlan%d:TX_POWER_CCK_A[%d]:%02x, TX_POWER_5G_HT40_1S_A[%d]:%02x\n", wlan_idx,
		start_ixd_2g, pValue->CCK_A[start_ixd_2g], start_idx_5g, pValue->HT40_5G_1S_A[start_idx_5g]);
	printf("Wlan%d:TX_POWER_TSSI_CCK_A[%d]:%02x, TX_POWER_TSSI_5G_HT40_1S_A[%d]:%02x\n", wlan_idx,
		start_ixd_2g, pValue->TSSI_CCK_A[start_ixd_2g], start_idx_5g, pValue->TSSI_HT40_5G_1S_A[start_idx_5g]);

	if(!pValue->CCK_A[start_ixd_2g] && !pValue->HT40_5G_1S_A[start_idx_5g] &&
		!pValue->TSSI_CCK_A[start_ixd_2g] && !pValue->TSSI_HT40_5G_1S_A[start_idx_5g])
	{
		printf("Wlan HW no calibration !No need to swap hs mib\n");
		return -1;
	}

	return 0;
}

static int get_wlan_hw_K_flag(int *f_2g, int *f_5g, int wlanIdx)
{
	int ret = (-1);
	int old_wlan_idx = wlan_idx;
	int start_ixd_2g=0, start_idx_5g=35;
	WLAN_HS_K_VALUE_T wlanHsInfo;

	if (wlanIdx != 0 && wlanIdx != 1)
		return ret;

	if (f_2g == NULL || f_5g == NULL)
		return ret;

	wlan_idx = wlanIdx;
	memset(&wlanHsInfo, 0, sizeof(wlanHsInfo));
	if(getWlanHSMib_K(&wlanHsInfo, wlan_idx)) {
		wlan_idx = old_wlan_idx;
		return ret;
	}
	wlan_idx = old_wlan_idx;

	if (wlanHsInfo.CCK_A[start_ixd_2g] || wlanHsInfo.TSSI_CCK_A[start_ixd_2g])
		*f_2g = 1;

	if (wlanHsInfo.HT40_5G_1S_A[start_idx_5g] || wlanHsInfo.TSSI_HT40_5G_1S_A[start_idx_5g])
		*f_5g = 1;

	ret = 0;
	return ret;
}

static int checkWlanHwValueIsCorrect(void)
{
	int ret = (-1); //0:correct -1:error
	int wlanIdx = 0;;
	int wlan0_2g = 0, wlan0_5g = 0, wlan1_2g = 0, wlan1_5g = 0;

	wlanIdx = 0;
	if (get_wlan_hw_K_flag(&wlan0_2g, &wlan0_5g, wlanIdx) != 0)
		return ret;

	wlanIdx = 1;
	if (get_wlan_hw_K_flag(&wlan1_2g, &wlan1_5g, wlanIdx) != 0)
		return ret;

	if(!((wlan0_2g || wlan1_5g) && (!wlan0_5g && !wlan1_2g) ||
		(!wlan0_2g && !wlan1_5g) && (wlan0_5g || wlan1_2g)))
	{
		ret = (-1);
	}
	else
	{
		ret = 0;
	}

	return ret;
}

int needSwapWlanMib(CONFIG_DATA_T data_type)
{
	int ret = 0;
	char phyBandSelect = 0;
	int old_wlan_idx = wlan_idx;
	unsigned char value[196]={0}, value1[196]={0}, value2[196]={0}, value3[196]={0};
	int start_ixd_2g=0, start_idx_5g=35;
	WLAN_HS_K_VALUE_T wlan_hs_info;

#if defined(WLAN0_5G_SUPPORT) || defined(WLAN1_5G_SUPPORT)
#if defined(WLAN0_5G_SUPPORT)
	printf("Select 5G on wlan0\n");
#else
	printf("Select 2.4G on wlan0\n");
#endif
	if(data_type==HW_SETTING){
		printf("Check wlan hs mib...\n");

		if (checkWlanHwValueIsCorrect() != 0)
		{
			ret = 0;
			printf("Wlan HW no calibration or Calibration value error, please Calibrate again!\n");
			return ret;
		}

		wlan_idx = 0;
		memset(&wlan_hs_info, 0, sizeof(wlan_hs_info));
		if(getWlanHSMib_K(&wlan_hs_info, wlan_idx)){
			wlan_idx = old_wlan_idx;
			return ret;
		}
#if defined(WLAN0_5G_SUPPORT)
		if(((wlan_hs_info.CCK_A[start_ixd_2g]) || (wlan_hs_info.TSSI_CCK_A[start_ixd_2g]))
			&& ((wlan_hs_info.HT40_5G_1S_A[start_idx_5g]) || (wlan_hs_info.TSSI_HT40_5G_1S_A[start_idx_5g])) )
		{
			ret = 0;
			printf("The WLAN K value of this board is wrong, please check again;\n");
		}
		else if(wlan_hs_info.CCK_A[start_ixd_2g])
			ret = 1;
		else if(wlan_hs_info.TSSI_CCK_A[start_ixd_2g])
			ret = 1;
		else{
			/*if just k one band*/
			wlan_idx = 1;
			memset(&wlan_hs_info, 0, sizeof(wlan_hs_info));
			if(getWlanHSMib_K(&wlan_hs_info, wlan_idx)){
				wlan_idx = old_wlan_idx;
				return ret;
			}

			if(((wlan_hs_info.CCK_A[start_ixd_2g]) || (wlan_hs_info.TSSI_CCK_A[start_ixd_2g]))
				&& ((wlan_hs_info.HT40_5G_1S_A[start_idx_5g]) || (wlan_hs_info.TSSI_HT40_5G_1S_A[start_idx_5g])) )
			{
				ret = 0;
				printf("The WLAN K value of this board is wrong, please check again;\n");
			}
			else if(wlan_hs_info.HT40_5G_1S_A[start_idx_5g])
				ret = 1;
			else if(wlan_hs_info.TSSI_HT40_5G_1S_A[start_idx_5g])
				ret = 1;
		}
#else
		if(((wlan_hs_info.CCK_A[start_ixd_2g]) || (wlan_hs_info.TSSI_CCK_A[start_ixd_2g]))
			&& ((wlan_hs_info.HT40_5G_1S_A[start_idx_5g]) || (wlan_hs_info.TSSI_HT40_5G_1S_A[start_idx_5g])) )
		{
			ret = 0;
			printf("The WLAN K value of this board is wrong, please check again;\n");
		}
		else if(wlan_hs_info.HT40_5G_1S_A[start_idx_5g])
			ret = 1;
		else if(wlan_hs_info.TSSI_HT40_5G_1S_A[start_idx_5g])
			ret = 1;
		else {
			/*if just k one band*/
			wlan_idx = 1;
			memset(&wlan_hs_info, 0, sizeof(wlan_hs_info));
			if(getWlanHSMib_K(&wlan_hs_info, wlan_idx)){
				wlan_idx = old_wlan_idx;
				return ret;
			}

			if(((wlan_hs_info.CCK_A[start_ixd_2g]) || (wlan_hs_info.TSSI_CCK_A[start_ixd_2g]))
				&& ((wlan_hs_info.HT40_5G_1S_A[start_idx_5g]) || (wlan_hs_info.TSSI_HT40_5G_1S_A[start_idx_5g])) )
			{
				ret = 0;
				printf("The WLAN K value of this board is wrong, please check again;\n");
			}
			else if(wlan_hs_info.CCK_A[start_ixd_2g])
				ret = 1;
			else if(wlan_hs_info.TSSI_CCK_A[start_ixd_2g])
				ret = 1;
		}
#endif
		wlan_idx = old_wlan_idx;
	}else if(data_type==CURRENT_SETTING){
		wlan_idx = 0;
		mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyBandSelect, sizeof(phyBandSelect));
		printf("Check wlan cs mib...\n");
		printf("Wlan%d:%s\n", wlan_idx, (phyBandSelect==PHYBAND_5G)?"5G":"2G");
#if defined(WLAN0_5G_SUPPORT)
		if(phyBandSelect != PHYBAND_5G){
			ret = 1;
		}
#else
#ifndef CONFIG_APP_APPLE_MFI_WAC
		if(phyBandSelect != PHYBAND_2G){
			ret = 1;
		}
#endif
#endif
		wlan_idx = old_wlan_idx;
	}
#endif

	if(!ret)
		printf("No need swap wlan %s mib!\n", (data_type==CURRENT_SETTING)?"cs":"hs");
	else
		printf("Need Swap wlan %s mib!\n", (data_type==CURRENT_SETTING)?"cs":"hs");

	return ret;
}

int swapWlanMibSetting_HS(CONFIG_DATA_T data_type)
{
	char cmd[200]={0};
	int ret = 0;

	snprintf(cmd, sizeof(cmd), "%s", "recover_xmlfile.sh -Swap wlan hs");
	system(cmd);

#ifdef CONFIG_USER_XML_BACKUP
	snprintf(cmd, sizeof(cmd), "cp -f %s %s", CONF_ON_XMLFILE_HS, CONF_ON_XMLFILE_HS_BAK);
	system(cmd);
#endif

	return ret;
}

int swapWlanMibSetting_CS(unsigned char wlanifNumA, unsigned char wlanifNumB, char *tmpBuf)
{
	int i = 0;
#ifdef WLAN_MBSSID
	MIB_CE_MBSSIB_T entryA, entryB;
	unsigned char idx;
#endif	//WLAN_MBSSID
#ifdef WLAN_ACL
	MIB_CE_WLAN_AC_T entryACL;
	int entryNumACL;
#endif
	unsigned char cmd[200]={0};

	if((wlanifNumA >= NUM_WLAN_INTERFACE) || (wlanifNumB >= NUM_WLAN_INTERFACE)) {
		printf("%s: wrong wlan interface number!\n", __func__);
		goto setErr_wlan;
	}

	// i += 3 => MIB_MAPPING does not care about triple or dual band
	for(i = DUAL_WLAN_START_ID + 1; i < DUAL_WLAN_END_ID; i += 3){
		// do not care about triple band swap
		if(mib_swap( i, i + 1) == 0){
			strcpy(tmpBuf, "Swap WLAN MIB failed!");
			goto setErr_wlan;
		}
	}

#ifdef WLAN_MBSSID
	for(i=0; i<=WLAN_REPEATER_ITF_INDEX; i++)
		mib_chain_swap(MIB_MBSSIB_TBL, i, i);
#endif	//WLAN_MBSSID

#ifdef WLAN_ACL
	entryNumACL = mib_chain_total(MIB_WLAN_AC_TBL);
	for(i=0; i<entryNumACL; i++) {
		if(!mib_chain_get(MIB_WLAN_AC_TBL, i, (void *)&entryACL)) {
			strcpy(tmpBuf, "Get chain record error!\n");
			goto setErr_wlan;
		}
		if(entryACL.wlanIdx == wlanifNumA)
			entryACL.wlanIdx = wlanifNumB;
		else if(entryACL.wlanIdx == wlanifNumB)
			entryACL.wlanIdx = wlanifNumA;
		else
			continue;

		mib_chain_update(MIB_WLAN_AC_TBL, (void *)&entryACL, i);
	}
#endif

	//vlan config
	//wlan profile
	//tr069

	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);

#ifdef CONFIG_USER_XML_BACKUP
	snprintf(cmd, sizeof(cmd), "cp -f %s %s", CONF_ON_XMLFILE, CONF_ON_XMLFILE_BAK);
	system(cmd);
#endif

	return 0;

setErr_wlan:
	return -1;
}

int getConfigPartitionCount(pCPC_info pInfo)
{
	#define CONFIG_PARTITION_PATH "/var/run/.config_partition_write_count"
	FILE *fp=NULL;
	int ret = 0;

	fp = fopen(CONFIG_PARTITION_PATH, "r");
	if(fp){
		fscanf(fp, "CS: %d , UpdateTime: %ld\n", &pInfo->cs_count, &pInfo->cs_time);
		fscanf(fp, "HS: %d , UpdateTime: %ld\n", &pInfo->hs_count, &pInfo->hs_time);
		fclose(fp);
	}
	else{
		printf("open %s error\n", CONFIG_PARTITION_PATH);
		ret = 1;
	}

	return ret;
}
#endif

#if defined(WLAN_SUPPORT)
#if defined(CONFIG_WLAN_SCHEDULE_SUPPORT)
#ifndef MBSSID
#define MBSSID
#endif
#ifndef UNIVERSAL_REPEATER
#define UNIVERSAL_REPEATER
#endif
#endif
int set_wlan_idx_by_wlanIf(unsigned char * ifname,int *vwlan_idx)
{
	int idx;

#if 0 // defined(CONFIG_RTK_DEV_AP)
	if(strlen(ifname)<5 || strncmp(ifname, "wlan", strlen("wlan")))
		return (-1);

	idx = atoi(&ifname[4]);
#else
	for(idx=0; idx<NUM_WLAN_INTERFACE ; idx++)
	{
		if(!strncmp(ifname, WLANIF[idx], strlen(WLANIF[idx])))
			break;
	}
#endif
	if (idx >= NUM_WLAN_INTERFACE) {
		 printf("invalid wlan interface index number[%d]!\n", idx);
		 return (-1);
	}
	wlan_idx = idx;
	*vwlan_idx = 0;
#ifdef WLAN_MBSSID
	if (strlen(ifname) >= 9 && ifname[5] == '-' && ifname[6] == 'v' && ifname[7] == 'a' && ifname[8] == 'p') {
		 idx = atoi(&ifname[9]);
		 if (idx >= NUM_VWLAN_INTERFACE) {
			 printf("invalid virtual wlan interface index number[%d]!\n", idx);
			 return (-1);
		 }
		 *vwlan_idx = idx+1;
	}
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
	if (strlen(ifname) >= 9 && ifname[5] == '-' && !memcmp(&ifname[6], "vxd", 3)) {
		 *vwlan_idx = NUM_VWLAN_INTERFACE;
	}
#endif
	return 0;
}
#endif

#ifdef WLAN_WPS_HAPD
int rtk_wlan_get_wps_status(char *ifname)
{
	FILE *fp=NULL;
	char buf[1024]={0}, newline[1024]={0};
	MIB_CE_MBSSIB_T Entry;
	int vwlanidx, status = 0;

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)ifname,&Entry,&vwlanidx)!=0)
	{
		return 0;
	}

	if(Entry.wlanDisabled)
		return 0;

#ifdef WLAN_CLIENT
	if(Entry.wlanMode == AP_MODE)
	{
		snprintf(buf, sizeof(buf), "%s wps_get_status -i %s", HOSTAPD_CLI, ifname);
		fp = popen(buf, "r");
	}
	else if(Entry.wlanMode == CLIENT_MODE)
	{
		fp = fopen("/var/wpas_wps_stas", "r");
	}
#else
	snprintf(buf, sizeof(buf), "%s wps_get_status -i %s", HOSTAPD_CLI, ifname);
	fp = popen(buf, "r");
#endif

	if(fp)
	{
		if(fgets(newline, 1024, fp) != NULL){
			sscanf(newline, "%*[^:]:%s", buf);
			//printf("sw_commit %d\n", version);
			printf("%s\n", buf);
		}

		if(!strcmp(buf, "Active"))
			status = 1;
#ifdef WLAN_CLIENT
		if(Entry.wlanMode == AP_MODE)
			pclose(fp);
		else if(Entry.wlanMode == CLIENT_MODE)
		{
			if(status == 1)
			{
				if(fgets(newline, 1024, fp) != NULL){
					if(fgets(newline, 1024, fp) != NULL)
					{
						sscanf(newline, "%*[^:]:%s", buf);
						//printf("sw_commit %d\n", version);
						printf("%s\n", buf);
					}
				}

				if(strcmp(buf, ifname))
					status = 0;
			}
			fclose(fp);
		}
#else
		pclose(fp);
#endif
	}

		return status;
}

int rtk_wlan_cancel_wps(char *ifname)
{
	MIB_CE_MBSSIB_T Entry;
	int vwlanidx;
	char buf[1024]={0};

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)ifname,&Entry,&vwlanidx)!=0)
	{
		return -1;
	}

	if(Entry.wlanDisabled || Entry.wsc_disabled)
		return 0;

	if(rtk_wlan_get_wps_status(ifname) == 0)
		return 0;

#ifdef WLAN_CLIENT
	if(Entry.wlanMode == AP_MODE)
	{
		snprintf(buf, sizeof(buf), "%s wps_cancel -i %s", HOSTAPD_CLI, ifname);
		system(buf);
	}
	else if(Entry.wlanMode == CLIENT_MODE)
	{
		snprintf(buf, sizeof(buf), "%s wps_cancel -i %s", WPA_CLI, ifname);
		system(buf);
		snprintf(buf, sizeof(buf), "/bin/systemd_action %s WPS-TIMEOUT", ifname);
		system(buf);
	}
#else
	snprintf(buf, sizeof(buf), "%s wps_cancel -i %s", HOSTAPD_CLI, ifname);
	system(buf);
#endif

	return 0;
}

int rtk_wlan_cancel_all_wps(void)
{
	int j, k;
	char ifname[16]={0};

	for (k = 0; k < NUM_WLAN_INTERFACE; k++)
	{
		for (j = 0; j <= NUM_VWLAN_INTERFACE; j++)
		{
			memset(ifname, 0, sizeof(ifname));
			rtk_wlan_get_ifname(k, j, ifname);
			rtk_wlan_cancel_wps(ifname);
		}
	}

	return 0;
}

int rtk_wlan_get_all_wps_status(void)
{
	int j, k, wps_status=0;
	char ifname[16]={0};

	for (k = 0; k < NUM_WLAN_INTERFACE; k++)
	{
		for (j = 0; j <= NUM_VWLAN_INTERFACE; j++)
		{
			memset(ifname, 0, sizeof(ifname));
			rtk_wlan_get_ifname(k, j, ifname);
			wps_status = rtk_wlan_get_wps_status(ifname);

			if(wps_status == 1)
				break;
		}

		if(wps_status == 1)
			break;
	}

	return wps_status;

}

#endif

int rtk_wlan_is_vxd_ifname(char *ifname)
{
	int i;
	char wlan_ifname[IFNAMSIZ]={0};

	if(ifname==NULL)
		return 0;

	for(i=0; i<NUM_WLAN_INTERFACE; i++)
	{
		snprintf(wlan_ifname, sizeof(wlan_ifname), VXD_IF, i);
		if(strcmp(ifname, wlan_ifname)==0)
			break;
	}
	if(i<NUM_WLAN_INTERFACE)
		return 1;

	return 0;
}

#define SIOCGIWIND_TEMP     0x89ff  // RTL8192CD_IOCTL_USER_DAEMON_REQUEST
static int IssueDisconnect(unsigned char *pucMacAddr, char* wlan_if,unsigned short reason)
{
	int vwlan_idx=0;
	MIB_CE_MBSSIB_T Entry;
	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	if(mib_chain_get_wlan((unsigned char *)wlan_if,&Entry,&vwlan_idx)!=0){
		return -1;
	}

	if(Entry.is_ax_support){
		unsigned char mac_addr_str[18];
		char tmp_cmd[128] = {0};

		if(pucMacAddr == NULL || wlan_if == NULL)
		{
			return -1;
		}

		snprintf(mac_addr_str, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
			pucMacAddr[0], pucMacAddr[1], pucMacAddr[2], pucMacAddr[3], pucMacAddr[4], pucMacAddr[5]);
		snprintf(tmp_cmd, sizeof(tmp_cmd), "hostapd_cli -i %s disassociate %s", wlan_if, mac_addr_str);

		system(tmp_cmd);
	}else{
		int skfd;
		int retVal = 0;
		struct iwreq wrq;
		DOT11_DISCONNECT_REQ Disconnect_Req;
		if(pucMacAddr == NULL || wlan_if == NULL)
		{
			return -1;
		}
		skfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (skfd < 0) {
			printf("socket() error!\n");
			return -1;
		}

		Disconnect_Req.EventId = DOT11_EVENT_DISCONNECT_REQ;

		Disconnect_Req.IsMoreEvent = 0;
		Disconnect_Req.Reason = reason;
		memcpy(Disconnect_Req.MACAddr,  pucMacAddr, 6);

		strcpy(wrq.ifr_name, wlan_if);

		wrq.u.data.pointer = (caddr_t)&Disconnect_Req;
		wrq.u.data.length = sizeof(DOT11_DISCONNECT_REQ);

		if(ioctl(skfd, SIOCGIWIND_TEMP, &wrq) < 0)
		{
			printf("Issues disassociation : ioctl error!\n");
			retVal = -1;
			return retVal;
		}

		close(skfd);
	}
	return 0;
}


static int wlan_get_mode(unsigned char *ifname, unsigned int *mode)
{
	int ret = RTK_SUCCESS;
	int vwlan_idx;
	MIB_CE_MBSSIB_T WlanEntry;

	if(ifname==NULL || mode==NULL)
		return RTK_FAILED;

	memset(&WlanEntry,0,sizeof(MIB_CE_MBSSIB_T));
	ret=mib_chain_get_wlan(ifname,&WlanEntry,&vwlan_idx);

	*mode = WlanEntry.wlanMode;

	return ret;
}

int deauth_wlan_clients(void)
{
	char *buff;
	WLAN_STA_INFO_Tp pInfo;
	int i, w_idx, vw_idx, ret = RTK_SUCCESS, is_client_exist=0;
	int wlan_enable;
	int wlan_mode;
 	char intfname[16]={0};
	unsigned char sta_mac[13]={0};

	buff = calloc(1, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));
	if (buff == 0)
	{
		printf("Allocate buffer failed!\n");
		return RTK_FAILED;
	}

	for(w_idx=0; w_idx<NUM_WLAN_INTERFACE; w_idx++)
	{
		for(vw_idx=0; vw_idx<=WLAN_MBSSID_NUM; vw_idx++)
		{
			memset(intfname, 0, sizeof(intfname));
			if(vw_idx == 0)
				snprintf(intfname,sizeof(intfname),WLAN_IF,w_idx);
			else
				snprintf(intfname,sizeof(intfname),VAP_IF,w_idx,vw_idx-1);
			if(rtk_wlan_get_wlan_enable(intfname,&wlan_enable) == RTK_FAILED)
			{
				ret = RTK_FAILED;
				free(buff);
				buff = NULL;
				return ret;
			}
			if(wlan_get_mode(intfname,&wlan_mode) == RTK_FAILED)
			{
				ret = RTK_FAILED;
				free(buff);
				buff = NULL;
				return ret;
			}
			if(wlan_enable == 1 && wlan_mode == AP_MODE && getWlStaInfo(intfname,(WLAN_STA_INFO_Tp)buff) == 0)
			{
				for (i=1; i<=MAX_STA_NUM; i++)
				{
					pInfo = (WLAN_STA_INFO_Tp)&buff[i*sizeof(WLAN_STA_INFO_T)];
					if (pInfo->aid && (pInfo->flags & STA_INFO_FLAG_ASOC)){
					//	printf("pInfo->addr=%02X%02X%02X%02X%02X%02X\n",
						//	pInfo->addr[0], pInfo->addr[1], pInfo->addr[2], pInfo->addr[3], pInfo->addr[4], pInfo->addr[5]);
						IssueDisconnect(pInfo->addr,intfname,1);
					}
				}
			}

		}
	}

	free(buff);
	buff = NULL;
	return ret;
}

#ifdef WLAN_NOT_USE_MIBSYNC
#define WLAN_DENY_LEGACY_CONTROL_PROC "echo %d > "PROC_WLAN_DIR_NAME"/%s/deny_legacy"
static int _set_wlan_11ax_deny_legacy(char *ifname, unsigned char phyband, unsigned char wlanBand)
{
	char cmd_str[256]={0};
	unsigned char deny_legacy = 0;

	if(phyband==PHYBAND_5G){
		switch(wlanBand)
		{
			case BAND_11N:
			case (BAND_11N|BAND_5G_11AC):
			case (BAND_11N|BAND_5G_11AC|BAND_11AX):
				deny_legacy = WLAN_MD_11A;
				break;
			case BAND_5G_11AC:
			case (BAND_5G_11AC|BAND_11AX):
				deny_legacy = (WLAN_MD_11A|WLAN_MD_11N);
				break;
			case BAND_11AX:
				deny_legacy = (WLAN_MD_11A|WLAN_MD_11N|WLAN_MD_11AC);
				break;
			default:
				deny_legacy = 0;
				break;
		}
	}
	else{
		switch(wlanBand)
		{
			case BAND_11G:
			case (BAND_11G|BAND_11N):
			case (BAND_11G|BAND_11N|BAND_11AX):
				deny_legacy = WLAN_MD_11B;
				break;
			case BAND_11N:
			case (BAND_11N|BAND_11AX):
				deny_legacy = (WLAN_MD_11B|WLAN_MD_11G);
				break;
			case BAND_11AX:
				deny_legacy = (WLAN_MD_11B|WLAN_MD_11G|WLAN_MD_11N);
				break;
			default:
				deny_legacy = 0;
				break;
		}
	}
	snprintf(cmd_str, sizeof(cmd_str), WLAN_DENY_LEGACY_CONTROL_PROC, deny_legacy, ifname);
	va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
}

static int _set_wlan_deny_legacy(char *ifname, unsigned char phyband, unsigned char wlanBand)
{
	char parm[256]={0};
	unsigned char deny_legacy = 0;

	if(phyband==PHYBAND_5G){
		switch(wlanBand)
		{
			case BAND_11N:
			case (BAND_11N|BAND_5G_11AC):
				deny_legacy = BAND_11A;
				break;
			case BAND_5G_11AC:
				deny_legacy = (BAND_11A|BAND_11N);
				break;
			default:
				deny_legacy = 0;
				break;
		}
	}
	else{
		switch(wlanBand)
		{
			case BAND_11G:
			case (BAND_11G|BAND_11N):
				deny_legacy = BAND_11B;
				break;
			case BAND_11N:
				deny_legacy = (BAND_11B|BAND_11G);
				break;
			default:
				deny_legacy = 0;
				break;
		}
	}
	wlanBand += deny_legacy;
	snprintf(parm, sizeof(parm), "band=%hhu", wlanBand);
	va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	snprintf(parm, sizeof(parm), "deny_legacy=%hhu", deny_legacy);
	va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
}

static int setupWLan_802_1x(char *ifname, void *Entry)
{
	int status = 0;
	MIB_CE_MBSSIB_Tp pEntry = (MIB_CE_MBSSIB_Tp) Entry;

	// Set 802.1x flag
	if (pEntry->encrypt <= WIFI_SEC_WEP){
		// 802_1x
		if(pEntry->enable1X)
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "802_1x=1");
		else
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "802_1x=0");

		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "psk_enable=0");
	}
	else
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "802_1x=1");

	return status;
}

#ifdef RTK_MULTI_AP
static int setupWLan_multi_ap(char *ifname, void *Entry, int wlan_index, int vwlan_index)
{
	char parm[128];
	int status = 0;
	MIB_CE_MBSSIB_Tp pEntry = (MIB_CE_MBSSIB_Tp) Entry;
	unsigned char map_state = 0;

	mib_get_s(MIB_MAP_CONTROLLER, (void *)&map_state, sizeof(map_state));
	if(map_state) {
		if(pEntry->multiap_bss_type == 0){
			pEntry->multiap_bss_type = MAP_FRONTHAUL_BSS;
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, wlan_index, (void *)pEntry, vwlan_index);
#ifdef COMMIT_IMMEDIATELY
			Commit();
#endif // of #if COMMIT_IMMEDIATELY
		}
		//if((Entry.multiap_bss_type == MAP_BACKHAUL_BSS) || (Entry.multiap_bss_type == MAP_BACKHAUL_STA))
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "a4_enable=1");
		//else
		//	statuss|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "a4_enable=0");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "multiap_monitor_mode_disable=0");
	} else {
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "a4_enable=0");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "multiap_monitor_mode_disable=1");
	}

	snprintf(parm, sizeof(parm), "multiap_bss_type=%d", pEntry->multiap_bss_type);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	return status;
}
#endif

static int setupWlan_TRX_Restrict(char *ifname, void *Entry)
{
    int tx_bandwidth, rx_bandwidth, qbwc_mode = GBWC_MODE_DISABLE;
	MIB_CE_MBSSIB_Tp pEntry = (MIB_CE_MBSSIB_Tp) Entry;
	int status = 0;

#if 1
	tx_bandwidth = pEntry->tx_restrict;
	rx_bandwidth = pEntry->rx_restrict;
#else

#if defined(WLAN_DUALBAND_CONCURRENT) && !defined(CONFIG_RTK_DEV_AP)
	if(strcmp(ifname ,WLANIF[1]) == 0) { // wlan1
		mib_get_s(MIB_WLAN1_TX_RESTRICT, (void *)&tx_bandwidth, sizeof(tx_bandwidth));
        mib_get_s(MIB_WLAN1_RX_RESTRICT, (void *)&rx_bandwidth, sizeof(rx_bandwidth));
    }
	else
#endif
    {
        mib_get_s(MIB_WLAN_TX_RESTRICT, (void *)&tx_bandwidth, sizeof(tx_bandwidth));
        mib_get_s(MIB_WLAN_RX_RESTRICT, (void *)&rx_bandwidth, sizeof(rx_bandwidth));
    }
#endif
    if (tx_bandwidth && rx_bandwidth == 0)
        qbwc_mode = GBWC_MODE_LIMIT_IF_TX;
    else if (tx_bandwidth == 0 && rx_bandwidth)
        qbwc_mode = GBWC_MODE_LIMIT_IF_RX;
    else if (tx_bandwidth && rx_bandwidth)
        qbwc_mode = GBWC_MODE_LIMIT_IF_TRX;

    status|=iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "gbwcmode", qbwc_mode);
#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_CTCAPD)
    status|=iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "gbwcthrd_tx", tx_bandwidth);
    status|=iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "gbwcthrd_rx", rx_bandwidth);
#else
    status|=iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "gbwcthrd_tx", tx_bandwidth*1024);
    status|=iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "gbwcthrd_rx", rx_bandwidth*1024);
#endif
	return status;
}

#ifdef WLAN_ROAMING
static int setupWLan_Roaming(char *ifname, unsigned char phyband)
{
	char parm[128];
	int status=0;
	unsigned char enable, rssi_th1, rssi_th2;
	unsigned int start_time;

	if(phyband==PHYBAND_5G){
		mib_get_s(MIB_ROAMING5G_ENABLE, &enable, sizeof(enable));
		mib_get_s(MIB_ROAMING5G_STARTTIME, &start_time, sizeof(start_time));
		mib_get_s(MIB_ROAMING5G_RSSI_TH1, &rssi_th1, sizeof(rssi_th1));
		mib_get_s(MIB_ROAMING5G_RSSI_TH2, &rssi_th2, sizeof(rssi_th2));
	}
	else{
		mib_get_s(MIB_ROAMING2G_ENABLE, &enable, sizeof(enable));
		mib_get_s(MIB_ROAMING2G_STARTTIME, &start_time, sizeof(start_time));
		mib_get_s(MIB_ROAMING2G_RSSI_TH1, &rssi_th1, sizeof(rssi_th1));
		mib_get_s(MIB_ROAMING2G_RSSI_TH2, &rssi_th2, sizeof(rssi_th2));
	}

	snprintf(parm, sizeof(parm), "roaming_enable=%hhu", enable);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	snprintf(parm, sizeof(parm), "roaming_start_time=%u", start_time);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	snprintf(parm, sizeof(parm), "roaming_rssi_th1=%hhu", rssi_th1);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	snprintf(parm, sizeof(parm), "roaming_rssi_th2=%hhu", rssi_th2);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	snprintf(parm, sizeof(parm), "roaming_wait_time=3");
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	return status;
}
#endif

#ifdef RTK_CROSSBAND_REPEATER
static int setupWlanCrossband(char *ifname, void *Entry)
{
	int status=0;
	unsigned char vChar=0;
	MIB_CE_MBSSIB_Tp pEntry = (MIB_CE_MBSSIB_Tp) Entry;

	if (pEntry->wlanMode == AP_MODE || pEntry->wlanMode == AP_MESH_MODE || pEntry->wlanMode == AP_WDS_MODE){
		mib_get_s(MIB_CROSSBAND_ENABLE, &vChar, sizeof(vChar));
		if(vChar){
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "crossband_enable=1");
		}
	}

	return status;
}
#endif

static int setup_wlan_priv_mib_or_proc(int wlan_index, int vwlan_index)
{
	MIB_CE_MBSSIB_T WlanEntry;
	unsigned char phyband = 0;
	unsigned char vChar = 0;
	unsigned int vInt = 0;
	int status = 0;
	char parm[128];
	char ifname[16];
#ifdef CONFIG_RTK_DEV_AP
	unsigned char def_gwmac[6]={0};
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
	unsigned char rpt_enabled=0;
	MIB_CE_MBSSIB_T WlanEntry_root;
#endif

	if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, vwlan_index, &WlanEntry) == 0)
		return -1;

	if((vwlan_index != WLAN_ROOT_ITF_INDEX) && (WlanEntry.wlanDisabled == 1))
		return 0;

	rtk_wlan_get_ifname(wlan_index, vwlan_index, ifname);

	mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlan_index, (void *) &phyband);

	if(WlanEntry.is_ax_support == 1){
		//func_off
		if(vwlan_index == WLAN_ROOT_ITF_INDEX){
			if(WlanEntry.wlanDisabled)
				status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=1");
			else{
#if (defined(CONFIG_00R0) && defined(_CWMP_MIB_)) || (defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_CTCAPD))
				if(WlanEntry.func_off == 1)
					status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=1");
				else
					status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=0");
#else
				status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=0");
#endif
			}
		}
#if (defined(CONFIG_00R0) && defined(_CWMP_MIB_)) || (defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_CTCAPD))
		if(vwlan_index >= WLAN_VAP_ITF_INDEX && vwlan_index < WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM){
			if(WlanEntry.func_off == 1)
				status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=1");
			else
				status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=0");
		}
#endif
	}

	//disable_protection
	if(WlanEntry.is_ax_support == 0){
		mib_local_mapping_get(MIB_WLAN_PROTECTION_DISABLED, wlan_index, (void *)&vChar);
		snprintf(parm, sizeof(parm), "disable_protection=%u", vChar);
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	}

	//aggregation
	if(WlanEntry.is_ax_support == 0){
		mib_local_mapping_get(MIB_WLAN_AGGREGATION, wlan_index, (void *)&vChar);
		if((vChar&(1<<WLAN_AGGREGATION_AMPDU))==0)
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "ampdu=0");
		else
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "ampdu=1");

		if((vChar&(1<<WLAN_AGGREGATION_AMSDU))==0)
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "amsdu=0");
		else
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "amsdu=2");
	}

	//iapp_enable ?

	//multi-ap
#ifdef RTK_MULTI_AP
	status |= setupWLan_multi_ap(ifname, (void *)&WlanEntry, wlan_index, vwlan_index);
#endif

	//gbwcmode & gbwcthrd_tx & gbwcthrd_rx
	status |= setupWlan_TRX_Restrict(ifname, (void *)&WlanEntry);

#ifdef CONFIG_RTK_DEV_AP
	//telco_selected
	mib_get_s(MIB_TELCO_SELECTED, (void *)&vChar, sizeof(vChar));
	snprintf(parm, sizeof(parm), "telco_selected=%u", vChar);
	status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
#endif

	if(vwlan_index == WLAN_ROOT_ITF_INDEX){
		if(WlanEntry.is_ax_support == 0){
		//regdomain
#ifdef WLAN_DUALBAND_CONCURRENT
			if(wlan_index == 1)
				mib_get_s(MIB_HW_WLAN1_REG_DOMAIN, (void *)&vChar, sizeof(vChar));
			else
#endif
				mib_get_s(MIB_HW_REG_DOMAIN, (void *)&vChar, sizeof(vChar));
			snprintf(parm, sizeof(parm), "regdomain=%u", vChar);
			status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

		//ledtype
#if 0
			#ifdef CONFIG_WIFI_LED_USE_SOC_GPIO
			 #define LEDTYPE "0"
			#elif defined(CONFIG_E8B)
			 #define LEDTYPE "7"
			#elif defined(CONFIG_TRUE) || defined(CONFIG_TLKM)
			 #define LEDTYPE "12"
			#else
			 #ifdef CONFIG_00R0
			  #define LEDTYPE "7"
			 #elif defined(CONFIG_RTL_92C_SUPPORT)
			  #define LEDTYPE "11"
			 #else
			  #define LEDTYPE "7"
			 #endif
			#endif

			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "led_type=" LEDTYPE);
#else
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "led_type=7" );
#endif
		}
	}

#ifdef CONFIG_RTK_SSID_PRIORITY
	//manual_priority
	snprintf(parm, sizeof(parm), "manual_priority=%u", WlanEntry.manual_priority);
	status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
#endif

	//autorate
	if(WlanEntry.rateAdaptiveEnabled == 0){
		status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "autorate=0");
		snprintf(parm, sizeof(parm), "fixrate=%u", WlanEntry.fixedTxRate);
		status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	}
	else{
		status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "autorate=1");
		status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "fixrate=0");
	}

	//lgyEncRstrct?
	//status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "lgyEncRstrct=15");
	//cts2self?
	//if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
	//	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "cts2self=1");

	//coexist ?
	if(vwlan_index == WLAN_ROOT_ITF_INDEX){
		//if(WlanEntry.is_ax_support == 0){
#if defined(CONFIG_00R0) && defined(WLAN_11N_COEXIST)
			mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlan_index, (void *)&vChar);
			if(vChar == PHYBAND_2G)
			{
#endif
				//11N Co-Existence
				mib_local_mapping_get(MIB_WLAN_11N_COEXIST, wlan_index, (void *)&vChar);
				if(vChar==0)
					va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "coexist=0");
				else
					va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "coexist=1");

#if defined(CONFIG_00R0) && defined(WLAN_11N_COEXIST)
			}
#endif
		//}
	}

#ifdef RTK_CROSSBAND_REPEATER
	//crossband_enable
	if(vwlan_index == WLAN_ROOT_ITF_INDEX)
		status |= setupWlanCrossband(ifname, (void *)&WlanEntry);
#endif

	if(vwlan_index == WLAN_ROOT_ITF_INDEX){
		//ofdma
		mib_local_mapping_get(MIB_WLAN_OFDMA_ENABLED, wlan_index, (void *)&vChar);
		snprintf(parm, sizeof(parm), "ofdma_enable=%u", vChar);
		status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		//txbf, txbf_mu
#ifdef WLAN_TX_BEAMFORMING
		snprintf(parm, sizeof(parm), "txbf=%u", WlanEntry.txbf);
		status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		snprintf(parm, sizeof(parm), "txbf_mu=%u", WlanEntry.txbf? WlanEntry.txbf_mu:0);
		status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		if (WlanEntry.is_ax_support == 1){
			snprintf(parm, sizeof(parm), "txbf_auto_snd=%u", WlanEntry.txbf? WlanEntry.txbf_mu:0);
			status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
			snprintf(parm, sizeof(parm), "txbf_mu_1ss=%u", WlanEntry.txbf? WlanEntry.txbf_mu:0);
			status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}
#endif
	}

#ifdef CONFIG_RTK_DEV_AP
	//roaming_switch
	mib_get_s(MIB_RTL_LINK_ROAMING_SWITCH, (void *)&vChar,sizeof(vChar));
	snprintf(parm, sizeof(parm), "roaming_switch=%u", vChar);
	status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	//roaming_qos
	mib_get_s(MIB_RTL_LINK_ROAMING_QOS, (void *)&vChar,sizeof(vChar));
	snprintf(parm, sizeof(parm), "roaming_qos=%u", vChar);
	status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	//fail_ratio
	mib_get_s(MIB_RTL_LINK_ROAMING_FAIL_RATIO, (void *)&vChar,sizeof(vChar));
	snprintf(parm, sizeof(parm), "fail_ratio=%u", vChar);
	status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	//retry_ratio
	mib_get_s(MIB_RTL_LINK_ROAMING_RETRY_RATIO, (void *)&vChar,sizeof(vChar));
	snprintf(parm, sizeof(parm), "retry_ratio=%u", vChar);
	status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	//RSSIThreshold
	snprintf(parm, sizeof(parm), "RSSIThreshold=%u", WlanEntry.rtl_link_rssi_th);
	status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	//dfgw_mac
	mib_get_s(MIB_DEF_GW_MAC, (void *)def_gwmac, sizeof(def_gwmac));
	snprintf(parm, sizeof(parm), "dfgw_mac=%02x%02x%02x%02x%02x%02x", def_gwmac[0], def_gwmac[1], def_gwmac[2], def_gwmac[3], def_gwmac[4], def_gwmac[5]);
	status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
#endif

	if(WlanEntry.is_ax_support == 0){
		//opmode
		if(vwlan_index == WLAN_ROOT_ITF_INDEX){
			if (WlanEntry.wlanMode == CLIENT_MODE)
				status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "opmode=8");	// client
			else	// 0(AP_MODE) or 3(AP_WDS_MODE)
				status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "opmode=16");	// AP
		}
#ifdef WLAN_UNIVERSAL_REPEATER
		// Repeater opmode
		if (vwlan_index == WLAN_REPEATER_ITF_INDEX){
			mib_local_mapping_get( MIB_REPEATER_ENABLED1, wlan_index, (void *)&rpt_enabled);
			if (rpt_enabled) {
				mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, WLAN_ROOT_ITF_INDEX, (void *)&WlanEntry_root);
				if (WlanEntry_root.wlanMode == CLIENT_MODE)
					status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "opmode=16");
				else
					status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "opmode=8");
			}
		}
#endif
	}

#ifdef WLAN_ROAMING
	if(vwlan_index == 0) //for ssid1 only
		setupWLan_Roaming(ifname, phyband);
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "force_qos=1");
	
	// enable CTC DSCP to WIFI QoS
	iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "ctc_dscp", 1);
#endif

	if(WlanEntry.is_ax_support==0){
		snprintf(parm, sizeof(parm), "block_relay=%u", WlanEntry.userisolation);
		status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

		//mc2u_disable
		snprintf(parm, sizeof(parm), "mc2u_disable=%u", WlanEntry.mc2u_disable);
		status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

		//lifetime
#ifdef WLAN_LIFETIME
		if(vwlan_index == WLAN_ROOT_ITF_INDEX){
			mib_local_mapping_get(MIB_WLAN_LIFETIME, wlan_index, (void *)&vInt);
			snprintf(parm, sizeof(parm), "lifetime=%u", vInt);
			status |= va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}
#endif
		status |= setupWLan_802_1x(ifname, (void *)&WlanEntry);
	}

	return status;
}
static int setup_wlan_post_process(int wlan_index, int vwlan_index)
{
	char ifname[16] = {0};
	char filename[64] = {0}, filename_new[64] = {0};
	int ret = 0;
	MIB_CE_MBSSIB_T WlanEntry, WlanEntry_root;
	unsigned char mode=0, level_percent=0;
#ifdef WLAN_LIFETIME
	unsigned int lifetime=0;
#endif
	unsigned char wlanBand = 0, phyband = 0;

	memset(ifname, 0, sizeof(ifname));
	memset(&WlanEntry,0,sizeof(MIB_CE_MBSSIB_T));

	rtk_wlan_get_ifname(wlan_index, vwlan_index, ifname);

	ret = mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, vwlan_index, (void*)&WlanEntry);

	if(ret == 0)
	{
		printf("%s(%d)The mibtbl get failed.\n",__FUNCTION__,__LINE__);
		return -1;
	}

	if(vwlan_index != WLAN_ROOT_ITF_INDEX)
	{
		snprintf(filename_new, sizeof(filename_new), "/var/%s", ifname);
		if(!WlanEntry.wlanDisabled)
		{
			if(NULL != opendir(filename_new))
			{
				printf("File: %s has been linked.\n", filename_new);
			}
			else
			{
				if(1 == WlanEntry.is_ax_support)
					snprintf(filename, sizeof(filename), PROC_WLAN_DIR_NAME"/%s", ifname);
				else if(0 == WlanEntry.is_ax_support)
					snprintf(filename, sizeof(filename), "/proc/%s", ifname);

				ret |= va_cmd("/bin/ln", 3, 1, "-snf", filename, filename_new);
			}
		}
	}
	else
	{
		if(WlanEntry.is_ax_support == 1)
		{
			mib_local_mapping_get(MIB_TX_POWER, wlan_index, (void *)&mode);
			if(mode < 5)
			{
				switch(mode)
				{
					case 0:
						level_percent = 100;
						break;
					case 1:
#ifdef CONFIG_E8B
						level_percent = 80;
#else
						level_percent = 70;
#endif
						break;
					case 2:
#ifdef CONFIG_E8B
						level_percent = 60;
#else
						level_percent = 50;
#endif
						break;
					case 3:
						level_percent = 35;
						break;
					case 4:
						level_percent = 15;
						break;
					default:
						break;
				}
			}
			else if(mode >= 10 && mode <= 90 && mode%10 == 0)
				level_percent = mode;
			else
			{
				level_percent = 100;
				printf("%s(%d) The value of MIB_TX_POWER set is wrong!\n", __FUNCTION__, __LINE__);
			}

			if(is_interface_run(ifname)){
				ret |= iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "powerpercent", level_percent);
#ifdef WLAN_LIFETIME
				mib_local_mapping_get(MIB_WLAN_LIFETIME, wlan_index, (void*)&lifetime);
				ret |= iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "lifetime", lifetime);
#endif
			}
		}
		else
		{
#ifdef WIFI5_WIFI6_RUNTIME
			setupWlanHWRegSettings(wlan_index, ifname);
#endif
		}
	}

	if(!WlanEntry.wlanDisabled)
	{
		//deny_legacy, band
		mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlan_index, (void *) &phyband);
#ifdef WLAN_BAND_CONFIG_MBSSID
		wlanBand = WlanEntry.wlanBand;
#ifdef WLAN_UNIVERSAL_REPEATER
		if(vwlan_index == WLAN_REPEATER_ITF_INDEX){
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, WLAN_ROOT_ITF_INDEX, &WlanEntry_root);
			wlanBand = WlanEntry_root.wlanBand;
		}
#endif
#else
		mib_local_mapping_get(MIB_WLAN_BAND, wlan_index, (void *) &wlanBand);
#endif

		if(WlanEntry.is_ax_support == 1){
#ifdef WLAN_BAND_CONFIG_MBSSID
			if(vwlan_index != WLAN_ROOT_ITF_INDEX){
				ret |= iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "band", wlanBand);
			}
#endif
#ifdef WLAN_CLIENT
			if(vwlan_index == WLAN_ROOT_ITF_INDEX && WlanEntry.wlanMode == CLIENT_MODE)
				ret |= iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "band", wlanBand);
#endif
			_set_wlan_11ax_deny_legacy(ifname, phyband, wlanBand);
		}
		else
			_set_wlan_deny_legacy(ifname, phyband, wlanBand);
	}

	if(WlanEntry.is_ax_support == 0){
		//func_off
		if(vwlan_index == WLAN_ROOT_ITF_INDEX){
			if(WlanEntry.wlanDisabled)
				ret|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=1");
			else{
#if (defined(CONFIG_00R0) && defined(_CWMP_MIB_)) || (defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_CTCAPD))
				if(WlanEntry.func_off == 1)
					ret|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=1");
				else
					ret|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=0");
#else
				ret|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=0");
#endif
			}
		}
#if (defined(CONFIG_00R0) && defined(_CWMP_MIB_)) || (defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_CTCAPD))
		if(vwlan_index >= WLAN_VAP_ITF_INDEX && vwlan_index < WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM){
			if(WlanEntry.func_off == 1)
				ret|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=1");
			else
				ret|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=0");
		}
#endif
	}

	return ret;
}
#endif // WLAN_NOT_USE_MIBSYNC

int set_wifi_misc(int wlan_index, int vwlan_index)
{
	int orig_wlan_idx = 0;
	unsigned char ifname[64] = {0};
	char filename_ax[64] = {0}, filename_ac[64] = {0}, cmd_file[128] = {0}, filename_new[64] = {0}, cmd[128] = {0};
	int ret = RTK_SUCCESS, ret_root = RTK_SUCCESS;
	MIB_CE_MBSSIB_T WlanEntry, WlanEntry_root;

	orig_wlan_idx = wlan_idx;

	wlan_idx = wlan_index;
	memset(ifname, 0, sizeof(ifname));
	memset(&WlanEntry,0,sizeof(MIB_CE_MBSSIB_T));
	memset(&WlanEntry_root,0,sizeof(MIB_CE_MBSSIB_T));
#if 0 // defined(CONFIG_RTK_DEV_AP)
#ifdef WLAN_UNIVERSAL_REPEATER
	if(vwlan_index == WLAN_REPEATER_ITF_INDEX)
		snprintf(ifname, sizeof(ifname), VXD_IF, wlan_idx);
	else
#endif
	if(vwlan_index == WLAN_ROOT_ITF_INDEX)
		snprintf(ifname, sizeof(ifname), WLAN_IF, wlan_idx);
	else
		snprintf(ifname, sizeof(ifname), VAP_IF, wlan_idx, vwlan_index-1);
#else
	rtk_wlan_get_ifname(wlan_idx, vwlan_index, ifname);
#endif
	ret = mib_chain_get(MIB_MBSSIB_TBL, vwlan_index, (void*)&WlanEntry);
	ret_root = mib_chain_get(MIB_MBSSIB_TBL, 0, (void*)&WlanEntry_root);

	if((ret == RTK_FAILED) || (ret_root == RTK_FAILED))
	{
		printf("%s(%d)The mibtbl get failed.\n",__FUNCTION__,__LINE__);
		return RTK_FAILED;
	}

	if(vwlan_index != WLAN_ROOT_ITF_INDEX)
	{
		snprintf(filename_new, sizeof(filename_new), "/var/%s", ifname);
		if(!WlanEntry.wlanDisabled)
		{
			if(NULL != opendir(filename_new))
			{
				printf("File: %s has been linked.\n", filename_new);
			}
			else
			{
				snprintf(filename_ax, sizeof(filename_ax), PROC_WLAN_DIR_NAME"/%s", ifname);
				snprintf(filename_ac, sizeof(filename_ac), "/proc/%s", ifname);

				if(1 == WlanEntry.is_ax_support)
					snprintf(cmd_file, sizeof(cmd_file), "ln -snf %s %s", filename_ax, filename_new);
				else if(0 == WlanEntry.is_ax_support)
					snprintf(cmd_file, sizeof(cmd_file), "ln -snf %s %s", filename_ac, filename_new);
				system(cmd_file);
			}

			if(vwlan_index != WLAN_REPEATER_ITF_INDEX && WlanEntry.wlanBand != WlanEntry_root.wlanBand){
				snprintf(cmd, sizeof(cmd), "iwpriv %s set_mib band=%d", ifname, WlanEntry.wlanBand);
				system(cmd);
			}
		}
	}
	else
	{
		unsigned char mode=0, level_percent=0;
		unsigned char cmd[128] = {0};
		char vChar;
#ifdef WLAN_LIFETIME
		unsigned int lifetime=0;
		mib_local_mapping_get(MIB_WLAN_LIFETIME, wlan_idx, (void*)&lifetime);
#endif

		if(WlanEntry_root.is_ax_support == 0)
		{
			//set mc2u_disable
			mib_get_s(MIB_WLAN_MCAST2UCAST_DISABLE, (void *)&vChar, sizeof(vChar));
			if(vChar != WlanEntry_root.mc2u_disable)
				iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "mc2u_disable", vChar);
		}
		else
		{
			mib_local_mapping_get(MIB_TX_POWER, wlan_idx, (void *)&mode);
			if(mode < 5)
			{
				switch(mode)
					{
						case 0:
							level_percent = 100;
							break;
						case 1:
#ifdef CONFIG_E8B
							level_percent = 80;
#else
							level_percent = 70;
#endif
							break;
						case 2:
#ifdef CONFIG_E8B
							level_percent = 60;
#else
							level_percent = 50;
#endif
							break;
						case 3:
							level_percent = 35;
							break;
						case 4:
							level_percent = 15;
							break;
						default:
							break;
					}
			}
			else if(mode >= 10 && mode <= 90 && mode%10 == 0)
				level_percent = mode;
			else
			{
				level_percent = 100;
				printf("%s(%d) The value of MIB_TX_POWER set is wrong!\n", __FUNCTION__, __LINE__);
			}

			if(is_interface_run(ifname)){
				iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "powerpercent", level_percent);
#ifdef WLAN_LIFETIME
				iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "lifetime", lifetime);
#endif
			}
		}
	}

	wlan_idx = orig_wlan_idx;
	return RTK_SUCCESS;
}

#if defined(CONFIG_USER_DATA_ELEMENTS_AGENT) || defined(CONFIG_USER_DATA_ELEMENTS_COLLECTOR)
int start_data_element()
{
	unsigned char dbg = 0, role = DE_ROLE_DISABLED;
	unsigned int interval = 0, port = 0;
	unsigned char cmd[256] = {0}, tmp[64] = {0};

	mib_get_s(MIB_DATA_ELEMENT_ROLE, (void *)&role, sizeof(role));
	mib_get_s(MIB_DATA_ELEMENT_DEBUG, (void *)&dbg, sizeof(dbg));
	mib_get_s(MIB_DATA_ELEMENT_COLLECT_INTERVAL, (void *)&interval, sizeof(interval));
	mib_get_s(MIB_DATA_ELEMENT_PORT, (void *)&port, sizeof(port));

	if(role==DE_ROLE_DISABLED)
		return 0;

	if(role==DE_ROLE_AGENT)
	{
		//de_agent_ossrv
		snprintf(cmd, sizeof(cmd), "/bin/de_agent_ossrv ");
		if(dbg)
		{
			snprintf(tmp, sizeof(tmp), "-d");
			strcat(cmd, tmp);
		}
		strcat(cmd, " &");
		printf(cmd);
		system(cmd);
		
		//de_agent, force to SPEC version r1 now (r2 still DRAFT)
		snprintf(cmd, sizeof(cmd), "/bin/de_agent -v r1 ");
		if(interval)
		{
			snprintf(tmp, sizeof(tmp), "-i %d", interval);
			strcat(cmd, tmp);
		}
		if(dbg)
		{
			snprintf(tmp, sizeof(tmp), "-d %d", dbg);
			strcat(cmd, tmp);
		}
		if(port)
		{
			snprintf(tmp, sizeof(tmp), "-p %d", port);
			strcat(cmd, tmp);
		}
		strcat(cmd, " &");
		printf(cmd);
		system(cmd);
	}else if(role==DE_ROLE_COLLECTOR)
	{
		//TODO
	}

	return 0;
}
#endif

#ifdef SBWC
int rtk_wlan_delall_sbwc_tbl()
{
	int entryNum=0, i=0, idx, enable=0;
	int ret = RTK_SUCCESS;
	MIB_CE_WLAN_SBWC_ENTRY_T Entry;

	//disable SBWC_ENABLE
	rtk_wlan_set_sbwc_mode(NULL, enable);
	
	// delete all entry
	entryNum = mib_chain_total(MIB_SBWC_TBL);
	for(i=entryNum; i>0; i--){
		if (!mib_chain_get(MIB_SBWC_TBL, i-1, (void *)&Entry)){
			ret = RTK_FAILED;
			goto error_out;
		}
	
		if(mib_chain_delete(MIB_SBWC_TBL, i-1) != 1) {
			ret = RTK_FAILED;
			goto error_out;
		}
	}
	printf("[%s %d]Delete all sbwc tbl entry\n", __FUNCTION__,__LINE__);

error_out:
	return ret;
}

void rtk_setup_wlan_mac_qos(unsigned char enable)
{
	int w_idx = 0, vw_idx = 0;
	int entry_num, i, wlan_enable = 0;
	unsigned char ifname[IFNAMSIZ] = {0};
	char cmd[128] = {0}, str_mac[32] = {0};
	MIB_CE_WLAN_SBWC_ENTRY_T entry;

	entry_num = mib_chain_total(MIB_SBWC_TBL);
	for (w_idx = 0; w_idx < NUM_WLAN_INTERFACE; w_idx++) {
		for (vw_idx = 0; vw_idx < NUM_VWLAN_INTERFACE; vw_idx++) {
			if(vw_idx == 0)
				snprintf(ifname, sizeof(ifname), WLAN_IF, w_idx);
			else
				snprintf(ifname, sizeof(ifname), VAP_IF, w_idx, vw_idx-1); 
			
			rtk_wlan_get_wlan_enable(ifname, &wlan_enable);
			for(i = 0; i < entry_num; i++) {
				memset(&entry, 0, sizeof(MIB_CE_WLAN_SBWC_ENTRY_T));
				if (!mib_chain_get(MIB_SBWC_TBL, i, (void *)&entry)){
					printf("[%s:%d] get MIB_SBWC_TBL fail.\n", __FUNCTION__, __LINE__);
					continue;
				}

				snprintf(str_mac, sizeof(str_mac), "%02x%02x%02x%02X%02x%02x", 
					entry.sbwc_sta_mac[0], entry.sbwc_sta_mac[1], entry.sbwc_sta_mac[2], 
					entry.sbwc_sta_mac[3], entry.sbwc_sta_mac[4], entry.sbwc_sta_mac[5]);

				if (enable && wlan_enable)
					snprintf(cmd, sizeof(cmd), "iwpriv %s sta_bw_control %s,%d,%d", ifname, str_mac, entry.sbwc_sta_rx_limit, entry.sbwc_sta_tx_limit);
				else
					snprintf(cmd, sizeof(cmd), "iwpriv %s sta_bw_control %s,%d,%d", ifname, str_mac, 0, 0);
				printf("[%s:%d] cmd:%s\n", __FUNCTION__, __LINE__, cmd);
				system(cmd);
			}		
		}
	}
	
	return;
}

int rtk_wlan_set_sbwc_mode(unsigned char *ifname, unsigned int mode)
{
	int ret=RTK_SUCCESS;	

	mib_set(MIB_WLAN_SBWC_ENABLE, (void *)&mode);	
	
	return ret; 
}

int rtk_wlan_set_sbwc_req(unsigned char *ifname, wifi_sbwc_req_T *sbwc_req)
{
	int ret = RTK_SUCCESS, entryNum=0, i=0;
	double tx_limit, rx_limit, match=0, intVal, idx;
	char str_mac[32] = {0};
	MIB_CE_WLAN_SBWC_ENTRY_T Entry, Entry_tmp;

	memset(&Entry_tmp, 0, sizeof(MIB_CE_WLAN_SBWC_ENTRY_T));
	rtk_string_to_hex(sbwc_req->clientMAC, Entry_tmp.sbwc_sta_mac, 12);	
	
	tx_limit = atof(sbwc_req->tx_limit);
	rx_limit = atof(sbwc_req->rx_limit);
	Entry_tmp.sbwc_sta_rx_limit = rx_limit;
	Entry_tmp.sbwc_sta_tx_limit = tx_limit;
	
	entryNum = mib_chain_total(MIB_SBWC_TBL);
	for(i=0; i<entryNum; i++){
		memset(&Entry, 0, sizeof(MIB_CE_WLAN_SBWC_ENTRY_T));
		if (!mib_chain_get(MIB_SBWC_TBL, i, (void *)&Entry)){
  			ret = RTK_FAILED;
			goto error_out;
		}
		snprintf(str_mac, 32, "%02x%02x%02x%02X%02x%02x", Entry.sbwc_sta_mac[0], Entry.sbwc_sta_mac[1], Entry.sbwc_sta_mac[2], Entry.sbwc_sta_mac[3], Entry.sbwc_sta_mac[4], Entry.sbwc_sta_mac[5]);
		if(!strncasecmp(sbwc_req->clientMAC, str_mac, sizeof(sbwc_req->clientMAC))){
			printf("(%s)%d Device(MAC:%s) has already been store into SBWC TBL\n",__FUNCTION__,__LINE__,sbwc_req->clientMAC);
			if(Entry_tmp.sbwc_sta_tx_limit != Entry.sbwc_sta_tx_limit ||
				Entry_tmp.sbwc_sta_rx_limit != Entry.sbwc_sta_rx_limit){
				//update device's tx or rx rate limit
				if(mib_chain_delete(MIB_SBWC_TBL, i) != 1) {
					ret = RTK_FAILED;
					goto error_out;
				}
				printf("(%s)%d Updata device(MAC:%s)'s tx or rx rate limit\n",__FUNCTION__,__LINE__,sbwc_req->clientMAC);
				mib_chain_add(MIB_SBWC_TBL, (void *)&Entry_tmp);
				sbwc_req->mib_changed = 1;
			}
			match = 1;
			break;
		}
	}
	
	if(!match){
		//add into WLAN AC TBL
		intVal = mib_chain_add(MIB_SBWC_TBL, (unsigned char*)&Entry_tmp);
		printf("(%s)%d Add device(MAC:%s) into SBWC TBL\n",__FUNCTION__,__LINE__,sbwc_req->clientMAC);
		if (intVal == 0) {
			printf("[%s]add sbwc entry fail\n", __FUNCTION__);
			ret = RTK_FAILED;
			goto error_out;
		}
		else if (intVal == -1) {
			printf("[%s]add sbwc entry full\n", __FUNCTION__);
			ret = RTK_FAILED;
			goto error_out;
		}
		sbwc_req->mib_changed = 1;
	}

error_out:
	return ret;
}

int rtk_wlan_get_sbwc_info(wifi_sbwc_info_Tp psbwc_tbl_info)
{
	int ret = RTK_SUCCESS;
	int enable = 0, entryNum = 0, i=0;
	MIB_CE_WLAN_SBWC_ENTRY_T Entry;

	if(psbwc_tbl_info == NULL){
		ret = RTK_FAILED;
		goto error_out;
	}
	memset(psbwc_tbl_info,0,sizeof(wifi_sbwc_info_T));
	
	mib_get(MIB_WLAN_SBWC_ENABLE, (void *)&enable);
	psbwc_tbl_info->sbwc_enable = enable;
	
	entryNum = mib_chain_total(MIB_SBWC_TBL);
	psbwc_tbl_info->sbwc_num = entryNum;	
	for(i=0; i<entryNum; i++){
		memset(&Entry, 0, sizeof(MIB_CE_WLAN_SBWC_ENTRY_T));
		if (!mib_chain_get(MIB_SBWC_TBL, i, (void *)&Entry)){
  			ret = RTK_FAILED;
			goto error_out;
		}

		memcpy(&psbwc_tbl_info->sbwc_info[i], &Entry, sizeof(MIB_CE_WLAN_SBWC_ENTRY_T));
		
		//printf("[====%s %d====] i:%d ---> psbwc_tbl_info sbwc_sta_mac is %02x%02x%02x%02x%02x%02x\n",__FUNCTION__,__LINE__, i, 
		//	psbwc_tbl_info->sbwc_info[i].sbwc_sta_mac[0],psbwc_tbl_info->sbwc_info[i].sbwc_sta_mac[1],psbwc_tbl_info->sbwc_info[i].sbwc_sta_mac[2],
		//	psbwc_tbl_info->sbwc_info[i].sbwc_sta_mac[3],psbwc_tbl_info->sbwc_info[i].sbwc_sta_mac[4],psbwc_tbl_info->sbwc_info[i].sbwc_sta_mac[5]);	
		//printf("[====%s %d====] i:%d ---> psbwc_tbl_info sbwc_sta_tx_limit is %d, sbwc_sta_rx_limit is %d\n",__FUNCTION__,__LINE__, i, 
		//	psbwc_tbl_info->sbwc_info[i].sbwc_sta_tx_limit, psbwc_tbl_info->sbwc_info[i].sbwc_sta_rx_limit);
		
	}
	
error_out:
	return ret;
}
#endif

#ifdef WLAN_11R
int genFtKhConfig(void)
{
	MIB_CE_WLAN_FTKH_T ent;
	int ii, i, entryNum;
	int j, wlanIdxBackup=wlan_idx;
	char ifname[17]={0};
	char tmpbuf[200], macStr[18]={0};
	int status=0, ishex;
	FILE *fp;
	char enckey[35]={0};

	if((fp=fopen(FT_CONF, "w")) == NULL)
	{
		printf("%s: Cannot create %s!\n", __func__, FT_CONF);
		return -1;
	}

	for (j=0; j<NUM_WLAN_INTERFACE; j++) {
		wlan_idx = j;
		entryNum = mib_chain_total(MIB_WLAN_FTKH_TBL);
		for (i=0; i<entryNum; i++) {
			if (!mib_chain_get(MIB_WLAN_FTKH_TBL, i, (void *)&ent))
			{
				printf("%s: Get chain record error!\n", __func__);
				return -1;
			}

			rtk_wlan_get_ifname(j, ent.intfIdx, ifname);

			snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
				ent.addr[0], ent.addr[1], ent.addr[2], ent.addr[3], ent.addr[4], ent.addr[5]);

			// check key format
			ishex = 1;
			if (strlen(ent.key) != 32) {
				ishex=0;
			}
			else
				for (ii=0; ii<32; ii++)
					if (!isxdigit(ent.key[ii])) {
						ishex = 0;
						break;
					}
			if (!ishex)
				snprintf(enckey, sizeof(enckey), "\"%s\"", ent.key);
			else
				strcpy(enckey, ent.key);

			fprintf(fp, "r0kh=%s %s %s %s\n", macStr, ent.r0kh_id, enckey, ifname);
			fprintf(fp, "r1kh=%s %s %s %s\n", macStr, macStr, enckey, ifname);
		}
	}

	wlan_idx = wlanIdxBackup;
	fclose(fp);
	return 0;
}
#endif

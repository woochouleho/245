#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "debug.h"
#include "utility.h"
#include "subr_net.h"
#include <linux/wireless.h>
#include <dirent.h>
#ifdef RTK_MULTI_AP
#include "subr_multiap.h"
#endif
//#include "subr_wlan.h"
#ifdef WLAN_FAST_INIT
#include "../../../linux-2.6.x/drivers/net/wireless/rtl8192cd/ieee802_mib.h"
#endif
#if defined(CONFIG_CTC_SDN)
#include "subr_ctcsdn.h"		
#endif

#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
static unsigned int _wlIdxRadioDoChange = 0;
static unsigned int _wlIdxSsidDoChange = 0;
#endif

#if defined(TRIBAND_SUPPORT)
const char* WLANIF[] = {"wlan0", "wlan1", "wlan2"};
#else
#if defined(WLAN_SWAP_INTERFACE)
const char* WLANIF[] = {"wlan1", "wlan0"};
#else
const char* WLANIF[] = {"wlan0", "wlan1"};
#endif
#endif
const char WLAN_DEV_PREFIX[] = "wlan"; 

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
const char WLAN_MONITOR_DAEMON[] = "iwcontrol";
#endif
const char IWPRIV[] = "/bin/iwpriv";
const char AUTH_DAEMON[] = "/bin/auth";
const char IWCONTROL[] = "/bin/iwcontrol";
const char AUTH_PID[] = "/var/run/auth-wlan0.pid";

#ifdef WLAN_11R
const char FT_DAEMON_PROG[]	= "/bin/ftd";
const char FT_WLAN_IF[]		= "wlan0";
const char FT_CONF[]		= "/tmp/ft.conf";
const char FT_PID[]			= "/var/run/ft.pid";
#endif

#ifdef WLAN_11K
const char DOT11K_DAEMON_PROG[] = "/bin/dot11k_deamon";
#endif

#ifdef HS2_SUPPORT
#define HS2PID  "/var/run/hs2.pid"
#endif

#define IWCONTROLPID  "/var/run/iwcontrol.pid"
#if defined(WLAN_DUALBAND_CONCURRENT)
#define WLAN0_WLAN1_WSCDPID  "/var/run/wscd-wlan0-wlan1.pid"
#endif //WLAN_DUALBAND_CONCURRENT
#define WSCDPID "/var/run/wscd-%s.pid"
const char WSCD_FIFO[] = "/var/wscd-%s.fifo";
const char WSCD_CONF[] = "/var/wscd.conf";
const char WSC_DAEMON_PROG[] = "/bin/wscd";
#ifdef WLAN_WDS
const char WDSIF[]="%s-wds0";
#endif
#ifdef CONFIG_USER_FON
const char FONIF[] = "tun0";
#endif
#if defined(WLAN_MESH)
const char PATHSEL_PID_FILE[] = "/var/run/pathsel-wlan-msh.pid";
const char MESH_DAEMON_PROG[]	= "/bin/pathsel";
const char MESH_IF[] = "wlan-msh";
#endif
#ifdef CONFIG_IAPP_SUPPORT
#define IAPP_PID_FILE "/var/run/iapp.pid"
#endif
#ifdef RTK_CROSSBAND_REPEATER
#define CROSSBAND_PID_FILE "/var/run/crossband.pid"
#endif

#ifdef WLAN_SMARTAPP_ENABLE
const char SMART_WLANAPP_PROG[]	= "/bin/smart_wlanapp";
const char SMART_WLANAPP_PID[]	= "/var/run/smart_wlanapp.pid";
#endif

#if 1//def CONFIG_RTK_DEV_AP
#define PARAM_TEMP_FILE				("/tmp/flash_param")
#endif

int wlan_idx=0;	// interface index

static unsigned int useAuth_RootIf=0;
#define 	BIT(x) 		(1<<x)
#if 0//def CONFIG_RTK_DEV_AP
static char useAuth_RootIfname[10] = {0};
#endif
static int wlan_num=0;
/* 2010-10-27 krammer :  change to 16 for dual band*/
static char para_iwctrl[16][20];
int is8021xEnabled(int vwlan_idx);
#ifdef WLAN_WPS_VAP
static int check_wps_enc(MIB_CE_MBSSIB_Tp Entry);
#endif
static int check_iwcontrol_8021x(void);
#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
static int checkWlanRootChange(int *p_action_type, config_wlan_ssid *p_ssid_index);
#endif
#ifdef RTK_SMART_ROAMING
static int setupWLanSmartRoaming(void);
#endif
static int getWlanStatus(int idx);
#ifdef CONFIG_YUEME
static void setupWlanVSIE(int wlan_index, unsigned char phyband);
#endif
#ifdef CONFIG_00R0
static void set5GAutoChannelLow();
static void setSiteSurveyTime();
#endif

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
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_00R0)
		case 1: //80%
			return 2;
		case 2: //60%
			return 4;
		case 3: //40%
			return 8;
		case 4: //20%
			return 14;
#elif defined(CONFIG_TELMEX_DEV)
		case 1: //75%
			return 2;
		case 2: //50%
			return 6;
		case 3: //25%
			return 12;
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
#ifdef WLAN_POWER_PERCENT_SUPPORT
	switch (mode)
	{
		case 10:    //Andlink RadioConfig transmitPower 10%
			return 20;
		case 20:   //Andlink RadioConfig transmitPower 20%
			return 14;
		case 30:   //Andlink RadioConfig transmitPower 30%
			return 10;
		case 40:   //Andlink RadioConfig transmitPower 40%
			return 8;
		case 50:   //Andlink RadioConfig transmitPower 50%
			return 6;
		case 60:   //Andlink RadioConfig transmitPower 60%
			return 4;
		case 70:   //Andlink RadioConfig transmitPower 70%
			return 3;
		case 80:   //Andlink RadioConfig transmitPower 80%
			return 2;
		case 90:   //Andlink RadioConfig transmitPower 90%
			return 1;
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
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
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

#if 1//def CONFIG_RTK_DEV_AP
char wlan_valid_interface[512]={0};
#endif

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
static unsigned int getWLAN_ChipVersion()
{
	FILE *stream;
	char buffer[128], cmd[32];
	char ifname[64] = {0};
	
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
	snprintf(ifname, sizeof(ifname), WLAN_IF, wlan_idx);
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

static int _get_wlan_hw_mib_index(char *ifname)
{
	int mib_index = 0;
#if defined(TRIBAND_SUPPORT)
	if(strcmp(ifname ,WLANIF[2]) == 0) // wlan2
		mib_index = 2;
	else
#endif
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_index = 1;

	return mib_index;
}

//#ifndef WLAN_FAST_INIT
#ifdef TRIBAND_SUPPORT
#define get_WlanHWMIB(index, mib_name, value) do{ \
	if(index==0) \
		mib_get_s(MIB_HW_##mib_name, (void *)value, sizeof(value)); \
	else if(index==1) \
		mib_get_s(MIB_HW_WLAN1_##mib_name, (void *)value, sizeof(value)); \
	else if(index==2) \
		mib_get_s(MIB_HW_WLAN2_##mib_name, (void *)value, sizeof(value)); \
}while(0)
#elif defined(WLAN_DUALBAND_CONCURRENT)
#ifdef SWAP_HW_WLAN_MIB_INDEX
#define get_WlanHWMIB(index, mib_name, value) do{ \
	if(index==1) \
		mib_get_s(MIB_HW_##mib_name, (void *)value, sizeof(value)); \
	else \
		mib_get_s(MIB_HW_WLAN1_##mib_name, (void *)value, sizeof(value)); \
}while(0)
#else
#define get_WlanHWMIB(index, mib_name, value) do{ \
	if(index==0) \
		mib_get_s(MIB_HW_##mib_name, (void *)value, sizeof(value)); \
	else \
		mib_get_s(MIB_HW_WLAN1_##mib_name, (void *)value, sizeof(value)); \
}while(0)
#endif //SWAP_HW_WLAN_MIB_INDEX
#else
#define get_WlanHWMIB(index, mib_name, value) do{ \
	mib_get_s(MIB_HW_##mib_name, (void *)value, sizeof(value)); \
}while(0)
#endif //TRIBAND_SUPPORT
//#endif

enum {IWPRIV_GETMIB=1, IWPRIV_HS=2, IWPRIV_INT=4, IWPRIV_HW2G=8, IWPRIV_TXPOWER=16, IWPRIV_HWDPK=32};
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
	int mib_id, mode, intVal, dpk_len=0;
	unsigned char dpk_value[1024]={0};
	int mib_hw_index = 0;
	unsigned char tssi_value[1]={0};

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
			else
				memcpy(value, va_arg(ap, char *), MAX_5G_CHANNEL_NUM);
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
		if(type & IWPRIV_HW2G){ //2G
			if(type & IWPRIV_TXPOWER){
				mode = va_arg(ap, int);
				mib_hw_index = _get_wlan_hw_mib_index(argv[1]);
				get_WlanHWMIB(mib_hw_index, 11N_TSSI_ENABLE, tssi_value);
				if(tssi_value[0])
					intVal = 0;
				else{
					intVal = getTxPowerScale(PHYBAND_2G, mode);
					/*add by taro: 98F 0.25 dB/unit */
					int chipVersion = getWLAN_ChipVersion();
					if(chipVersion == CHIP_RTL8198F)
						intVal<<=1;
					/*end by taro*/
				}
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
		else{ //5G

			if(type & IWPRIV_TXPOWER){
				mode = va_arg(ap, int);
				mib_hw_index = _get_wlan_hw_mib_index(argv[1]);
				get_WlanHWMIB(mib_hw_index, 11N_TSSI_ENABLE, tssi_value);
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
						int chipVersion = getWLAN_ChipVersion();
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
	}
	argv[++k] = parm;
	TRACE(STA_SCRIPT|STA_NOTAG, "%s ", argv[k]);
	TRACE(STA_SCRIPT|STA_NOTAG, "\n");
	argv[k+1] = NULL;
	status = do_cmd(IWPRIV, argv, 1);
	va_end(ap);

	return status;
}

#if 0
// Mason Yu. 201009_new_security
void MBSSID_GetRootEntry(MIB_CE_MBSSIB_T *Entry) {
	//Entry->idx = 0;

	mib_get_s(MIB_WLAN_ENCRYPT, &Entry->encrypt, sizeof(Entry->encrypt));
	mib_get_s(MIB_WLAN_ENABLE_1X, &Entry->enable1X, sizeof(Entry->enable1X));
	mib_get_s(MIB_WLAN_WEP, &Entry->wep, sizeof(Entry->wep));
	mib_get_s(MIB_WLAN_WPA_AUTH, &Entry->wpaAuth, sizeof(Entry->wpaAuth));
	mib_get_s(MIB_WLAN_WPA_PSK_FORMAT, &Entry->wpaPSKFormat, sizeof(Entry->wpaPSKFormat));
	mib_get_s(MIB_WLAN_WPA_PSK, Entry->wpaPSK, sizeof(Entry->wpaPSK));
	mib_get_s(MIB_WLAN_RS_PORT, &Entry->rsPort, sizeof(Entry->rsPort));
	mib_get_s(MIB_WLAN_RS_IP, Entry->rsIpAddr, sizeof(Entry->rsIpAddr));

	mib_get_s(MIB_WLAN_RS_PASSWORD, Entry->rsPassword, sizeof(Entry->rsPassword));
	mib_get_s(MIB_WLAN_DISABLED, &Entry->wlanDisabled, sizeof(Entry->wlanDisabled));
	mib_get_s(MIB_WLAN_SSID, Entry->ssid, sizeof(Entry->ssid));
	mib_get_s(MIB_WLAN_MODE, &Entry->wlanMode, sizeof(Entry->wlanMode));
	mib_get_s(MIB_WLAN_AUTH_TYPE, &Entry->authType, sizeof(Entry->authType));
	//added by xl_yue
	// Mason Yu. 201009_new_security
	mib_get_s( MIB_WLAN_WPA_CIPHER_SUITE, &Entry->unicastCipher, sizeof(Entry->unicastCipher));
	mib_get_s( MIB_WLAN_WPA2_CIPHER_SUITE, &Entry->wpa2UnicastCipher, sizeof(Entry->wpa2UnicastCipher));
	mib_get_s( MIB_WLAN_WPA_GROUP_REKEY_TIME, &Entry->wpaGroupRekeyTime, sizeof(Entry->wpaGroupRekeyTime));

#ifdef CONFIG_RTL_WAPI_SUPPORT
	mib_get_s( MIB_WLAN_WAPI_PSK, Entry->wapiPsk, sizeof(Entry->wapiPsk));
	mib_get_s( MIB_WLAN_WAPI_PSKLEN, &Entry->wapiPskLen, sizeof(Entry->wapiPskLen));
	mib_get_s( MIB_WLAN_WAPI_PSK_FORMAT, &Entry->wapiPskFormat, sizeof(Entry->wapiPskFormat));
	mib_get_s( MIB_WLAN_WAPI_AUTH, &Entry->wapiAuth, sizeof(Entry->wapiAuth));
	mib_get_s( MIB_WLAN_WAPI_ASIPADDR, Entry->wapiAsIpAddr, sizeof(Entry->wapiAsIpAddr));
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
	mib_get_s(MIB_WLAN_WEP_KEY_TYPE, &Entry->wepKeyType, sizeof(Entry->wepKeyType));      // wep Key Format
	mib_get_s(MIB_WLAN_WEP64_KEY1, Entry->wep64Key1, sizeof(Entry->wep64Key1));
	mib_get_s(MIB_WLAN_WEP128_KEY1, Entry->wep128Key1, sizeof(Entry->wep128Key1));
	mib_get_s(MIB_WLAN_BAND, &Entry->wlanBand, sizeof(Entry->wlanBand));
#ifdef WLAN_11W
	mib_get_s(MIB_WLAN_DOTIEEE80211W, &Entry->dotIEEE80211W, sizeof(Entry->dotIEEE80211W));
	mib_get_s(MIB_WLAN_SHA256, &Entry->sha256, sizeof(Entry->sha256));
#endif
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
	mib_set(MIB_WLAN_BAND, &Entry->wlanBand);
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
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
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

int rtk_wlan_get_mib_entry_by_ifname(MIB_CE_MBSSIB_T *pEntry, const char *ifname)
{
	int i = 0, wlan_index = 0, vwlan_idx = 0, orig_wlan_idx;
	const char *vap_name = NULL;

	for(i=0;i<NUM_WLAN_INTERFACE;i++)
	{
		if(!strncmp(ifname, WLANIF[i], 5))
		{
			wlan_index = i;
			if(!strcmp(ifname, WLANIF[i]))
				vwlan_idx = WLAN_ROOT_ITF_INDEX;
			else
			{
				vap_name = &ifname[6];
				if(!strncmp(vap_name, "vap", 3))
				{
					vwlan_idx = (vap_name[3] - '0') + WLAN_VAP_ITF_INDEX;
				}
				else if(!strncmp(vap_name, "vxd", 3))
				{
					vwlan_idx = WLAN_REPEATER_ITF_INDEX;
				}
				else
					return -1;
			}
			break;
		}
	}
	//printf("%s %d %d\n", __func__, wlan_idx, vwlan_idx);

#ifdef WLAN_DUALBAND_CONCURRENT
	orig_wlan_idx = wlan_idx;
	wlan_idx = wlan_index;
#endif
	wlan_getEntry(pEntry, vwlan_idx);
#ifdef WLAN_DUALBAND_CONCURRENT
	wlan_idx = orig_wlan_idx;
#endif
	return wlan_index;
}

/////////////////////////////////////////////////////////////////////////////
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

    wrq.u.data.pointer = (caddr_t)pInfo;
    wrq.u.data.length = sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1);

    if (iw_get_ext(skfd, interface, SIOCGIWRTLSTAINFO, &wrq) < 0)
    {
    	close( skfd );
	return -1;
    }

    if(skfd != -1)
        close( skfd );

    return 0;
}
int getWlEnc( char *interface , char *buffer, char *num)
{
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

    wrq.u.data.pointer = (caddr_t)buffer;
    wrq.u.data.length = strlen(buffer);

    if (iw_get_ext(skfd, interface, RTL8192CD_IOCTL_GET_MIB, &wrq) < 0)
    {
    	close( skfd );
		return -1;
    }
	*num  = buffer[0];
    if(skfd != -1)
        close( skfd );

    return 0;
}

int getWlWpaChiper( char *interface , char *buffer, int *num)
{
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

    wrq.u.data.pointer = (caddr_t)buffer;
    wrq.u.data.length = strlen(buffer);

    if (iw_get_ext(skfd, interface, RTL8192CD_IOCTL_GET_MIB, &wrq) < 0)
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

#ifdef WLAN_FAST_INIT
#ifdef WLAN_ACL
void set_wlan_acl(struct wifi_mib *pmib)
{
	MIB_CE_WLAN_AC_T Entry;
	int num=0, i;
	char vChar;

	// aclnum=0
	(&pmib->dot11StationConfigEntry)->dot11AclNum = 0;

	// aclmode
	mib_get_s(MIB_WLAN_AC_ENABLED, (void *)&vChar, sizeof(vChar));
	(&pmib->dot11StationConfigEntry)->dot11AclMode = vChar;

	if (vChar){ // ACL enable
		if((num = mib_chain_total(MIB_WLAN_AC_TBL))==0)
			return;
		for (i=0; i<num; i++) {
			if (!mib_chain_get(MIB_WLAN_AC_TBL, i, (void *)&Entry))
				return;
			if(Entry.wlanIdx != wlan_idx)
				continue;

			// acladdr
			memcpy(&((&pmib->dot11StationConfigEntry)->dot11AclAddr[i][0]), &(Entry.macAddr[0]), 6);
			(&pmib->dot11StationConfigEntry)->dot11AclNum++;
		}
	}
}
#endif
#else
#ifdef WLAN_ACL
#ifdef WLAN_VAP_ACL
// return value:
// 0  : successful
// -1 : failed
int set_wlan_acl(int vwlan_idx)

{
	unsigned char value[32];
	char parm[64];
	int num, i;
	MIB_CE_WLAN_AC_T Entry;
	int status=0;

	char ifname[15];
	MIB_CE_MBSSIB_T wlanEntry;

	rtk_wlan_get_ifname(wlan_idx, vwlan_idx, ifname);


	// aclnum=0
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "aclnum=0");

	// aclmode
	if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_idx, vwlan_idx, (void *)&wlanEntry))
		printf("Error! Get MIB_MBSSIB_TBL error.\n");
	snprintf(parm, sizeof(parm), "aclmode=%d", wlanEntry.acl_enable);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	if (wlanEntry.acl_enable == 0) // ACL disabled
		return status;

	if ((num = mib_chain_total(MIB_WLAN_AC_TBL+vwlan_idx*WLAN_MIB_MAPPING_STEP)) == 0)
		return status;

	for (i=0; i<num; i++) {
		memset(&Entry, 0, sizeof(Entry));
		if (!mib_chain_local_mapping_get(MIB_WLAN_AC_TBL+vwlan_idx*WLAN_MIB_MAPPING_STEP, wlan_idx, i, (void *)&Entry))
			return -1;

		if(Entry.wlanIdx != wlan_idx)
			continue;

		// acladdr
		snprintf(parm, sizeof(parm), "acladdr=%.2x%.2x%.2x%.2x%.2x%.2x",
			Entry.macAddr[0], Entry.macAddr[1], Entry.macAddr[2],
			Entry.macAddr[3], Entry.macAddr[4], Entry.macAddr[5]);
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	}
	return status;
}
#else
int set_wlan_acl(char *ifname)
{
	unsigned char value[32];
	char parm[64];
	int num, i;
	MIB_CE_WLAN_AC_T Entry;
	int status=0;

	// aclnum=0
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "aclnum=0");

	// aclmode

	mib_get_s(MIB_WLAN_AC_ENABLED, (void *)value, sizeof(value));
	snprintf(parm, sizeof(parm), "aclmode=%u", value[0]);

	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);


	if (value[0] == 0) // ACL disabled
		return status;


	if ((num = mib_chain_total(MIB_WLAN_AC_TBL)) == 0)
		return status;

	for (i=0; i<num; i++) {
		if (!mib_chain_get(MIB_WLAN_AC_TBL, i, (void *)&Entry))
			return -1;

		if(Entry.wlanIdx != wlan_idx)
			continue;


		// acladdr
		snprintf(parm, sizeof(parm), "acladdr=%.2x%.2x%.2x%.2x%.2x%.2x",
			Entry.macAddr[0], Entry.macAddr[1], Entry.macAddr[2],
			Entry.macAddr[3], Entry.macAddr[4], Entry.macAddr[5]);
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	}
	return status;
}
#endif
#endif
#endif

#ifdef SBWC
int setup_sbwc(char *ifname)
{
	unsigned int enable=0;
	int entryNum=0, i=0;
	char cmd[128] = {0};
	char parm[64];
	int status=0;
	MIB_CE_WLAN_SBWC_ENTRY_T Entry;

	mib_get(MIB_WLAN_SBWC_ENABLE, (void *)&enable);
	if(enable > 0){
		entryNum = mib_chain_total(MIB_SBWC_TBL);
	
		for(i=0; i<entryNum; i++){
			memset(&Entry, 0, sizeof(MIB_CE_WLAN_SBWC_ENTRY_T));
			if (!mib_chain_get(MIB_SBWC_TBL, i, (void *)&Entry)){
	  			return;
			}

			//snprintf(cmd, sizeof(cmd), "iwpriv %s sta_bw_control %02x%02x%02x%02x%02x%02x,%d,%d", ifname,
			//	Entry.sbwc_sta_mac[0], Entry.sbwc_sta_mac[1], Entry.sbwc_sta_mac[2],
			//	Entry.sbwc_sta_mac[3], Entry.sbwc_sta_mac[4], Entry.sbwc_sta_mac[5],
			//	Entry.sbwc_sta_rx_limit, Entry.sbwc_sta_tx_limit);
			//printf("[====%s %d====] cmd is %s\n",__FUNCTION__,__LINE__,cmd);
			//system(cmd);
			snprintf(parm, sizeof(parm), "%02x%02x%02x%02x%02x%02x,%d,%d",
				Entry.sbwc_sta_mac[0], Entry.sbwc_sta_mac[1], Entry.sbwc_sta_mac[2],
				Entry.sbwc_sta_mac[3], Entry.sbwc_sta_mac[4], Entry.sbwc_sta_mac[5],
				Entry.sbwc_sta_rx_limit, Entry.sbwc_sta_tx_limit);
			//printf("[====%s %d====] parm is %s\n",__FUNCTION__,__LINE__,parm);
			status|=va_cmd(IWPRIV, 3, 1, ifname, "sta_bw_control", parm);
		}
	}
	return status;
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

int rtk_wlan_get_sbwc_mode(unsigned char *ifname, unsigned int *mode)
{
	int ret = RTK_SUCCESS;	
	
	if(mode==NULL)
		return RTK_FAILED;
	
	mib_get(MIB_WLAN_SBWC_ENABLE, (void *)mode);	
	return ret;
}

int rtk_wlan_set_sbwc_mode(unsigned char *ifname, unsigned int mode)
{
	int ret=RTK_SUCCESS;	

	mib_set(MIB_WLAN_SBWC_ENABLE, (void *)&mode);	
	
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
#endif

#if 0
#define RTL8185_IOCTL_READ_EEPROM	0x89f9
static int check_wlan_eeprom()
{
	int skfd,i;
	struct iwreq wrq;
	unsigned char tmp[162];
	char mode;
	char parm[64];
	char *argv[6];
	argv[1] = (char*)getWlanIfName();
	argv[2] = "set_mib";

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	/* Set device name */
	strncpy(wrq.ifr_name, (char *)getWlanIfName(), IFNAMSIZ);
	strcpy(tmp,"RFChipID");
	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = 10;
	ioctl(skfd, RTL8185_IOCTL_READ_EEPROM, &wrq);
	if(wrq.u.data.length>0){
		printf("read eeprom success!\n");
		//return 1;
	}
	else{
		printf("read eeprom fail!\n");
		if(skfd!=-1) close(skfd);
		return 0;
	}
	//set TxPowerCCK from eeprom
	strcpy(tmp,"TxPowerCCK");
	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = 20;
	ioctl(skfd, RTL8185_IOCTL_READ_EEPROM, &wrq);
	snprintf(parm, sizeof(parm), "TxPowerCCK=");
	if ( mib_get_s( MIB_TX_POWER, (void *)&mode, sizeof(mode)) == 0) {
   		printf("Get MIB_TX_POWER error!\n");
	}

//added by xl_yue:
#ifdef WLAN_TX_POWER_DISPLAY
	if ( mode==0 ) {          // 100%
		for(i=0; i<=13; i++)
		{
	//		value[i] = 8;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}else if ( mode==1 ) {    // 80%
		for(i=0; i<=13; i++)
		{
		    	wrq.u.data.pointer[i] -= 1;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}

	}else if ( mode==2 ) {    // 50%
		for(i=0; i<=13; i++)
		{
		    	wrq.u.data.pointer[i] -= 3;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}else if ( mode==3 ) {    // 25%
		for(i=0; i<=13; i++)
		{
		   	wrq.u.data.pointer[i] -= 6;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}else if ( mode==4 ) {    // 10%
		for(i=0; i<=13; i++)
		{
		   	wrq.u.data.pointer[i] -= 10;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}
#else
	if ( mode==2 ) {          // 60mW
		for(i=0; i<=13; i++)
		{
	//		value[i] = 8;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}else if ( mode==1 ) {    // 30mW
		for(i=0; i<=13; i++)
		{
		    	wrq.u.data.pointer[i] -= 3;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}else if ( mode==0 ) {    // 15mW
		for(i=0; i<=13; i++)
		{
		   	wrq.u.data.pointer[i] -= 6;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}
#endif
	argv[3] = parm;
	argv[4] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
	do_cmd(IWPRIV, argv, 1);

	strcpy(tmp,"TxPowerOFDM");
	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = 20;
	ioctl(skfd, RTL8185_IOCTL_READ_EEPROM, &wrq);
	snprintf(parm, sizeof(parm), "TxPowerOFDM=");
	if ( mib_get_s( MIB_TX_POWER, (void *)&mode, sizeof(mode)) == 0) {
   		printf("Get MIB_TX_POWER error!\n");
	}

//added by xl_yue:
#ifdef WLAN_TX_POWER_DISPLAY
	if ( mode==0 ) {          // 100%
		for(i=0; i<=13; i++)
		{
	//		value[i] = 8;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}else if ( mode==1 ) {    // 80%
		for(i=0; i<=13; i++)
		{
		    	wrq.u.data.pointer[i] -= 1;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}else if ( mode==2 ) {    // 50%
		for(i=0; i<=13; i++)
		{
		    	wrq.u.data.pointer[i] -= 3;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}else if ( mode==3 ) {    // 25%
		for(i=0; i<=13; i++)
		{
		   	wrq.u.data.pointer[i] -= 6;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}else if ( mode==4 ) {    // 10%
		for(i=0; i<=13; i++)
		{
		   	wrq.u.data.pointer[i] -= 10;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}
#else
	if ( mode==2 ) {          // 60mW
		for(i=0; i<=13; i++)
		{
	//		value[i] = 8;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}else if ( mode==1 ) {    // 30mW
		for(i=0; i<=13; i++)
		{
		    	wrq.u.data.pointer[i] -= 3;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}else if ( mode==0 ) {    // 15mW
		for(i=0; i<=13; i++)
		{
		   	wrq.u.data.pointer[i] -= 6;
			snprintf(parm, sizeof(parm), "%s%02x", parm, wrq.u.data.pointer[i]);
		}
	}
#endif
	argv[3] = parm;
	argv[4] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
	do_cmd(IWPRIV, argv, 1);

	close( skfd );
	return 1;
}
#endif
/* andrew: new test plan require N mode to avoid using TKIP. This function check the new band
   and unmask TKIP security if it's N mode.
*/
int wl_isNband(unsigned char band) {
	return (band >= 8);
}

void wl_updateSecurity(unsigned char band) {
	if (wl_isNband(band)) {
		MIB_CE_MBSSIB_T Entry;
		if(!mib_chain_get(MIB_MBSSIB_TBL, 0, &Entry))
			return;
		Entry.unicastCipher &= ~(WPA_CIPHER_TKIP);
		Entry.wpa2UnicastCipher &= ~(WPA_CIPHER_TKIP);
		mib_chain_update(MIB_MBSSIB_TBL, &Entry, 0);
	}
}

unsigned char wl_cipher2mib(unsigned char cipher) {
	unsigned char mib = 0;
	if (cipher & WPA_CIPHER_TKIP)
		mib |= 2;
	if (cipher & WPA_CIPHER_AES)
		mib |= 8;
	return mib;
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

#if 1 //def CONFIG_RTK_DEV_AP
int is_root_iface(char *ifname)
{
	int ret = 1;
	if(strstr(ifname, "-vxd") || strstr(ifname, "-va") || strstr(ifname, "-wds") || strstr(ifname, "-msh"))
		ret = 0;

	return ret;
}

int is_vxd_iface(char *ifname)
{
	int ret = 0;
	if(strstr(ifname, "-vxd"))
		ret  =1;

	return ret;
}

int get_wlan_idx_by_iface(char *wlan_iface_name, int *root_idx, int *vindex)
{
	int idx1=0, idx2=0;

	idx1 = atoi(&wlan_iface_name[4]);
	if (idx1 >= NUM_WLAN_INTERFACE) {
		printf("invalid wlan interface index number!\n");
		return 0;
	}

#ifdef WLAN_MBSSID
	if (strlen(wlan_iface_name) >= 9 && wlan_iface_name[5] == '-' &&
		wlan_iface_name[6] == 'v' && wlan_iface_name[7] == 'a')
	{
		idx1 = atoi(&wlan_iface_name[4]);
		idx2 = atoi(&wlan_iface_name[8]) + 1;

		if (idx2 > WLAN_MBSSID_NUM) {
			printf("invalid virtual wlan interface index number!\n");
			return 0;
		}
	}
#endif

#ifdef WLAN_UNIVERSAL_REPEATER
	if (strlen(wlan_iface_name) >= 9 && wlan_iface_name[5] == '-' &&
			!memcmp(&wlan_iface_name[6], "vxd", 3))
	{
		idx1 = atoi(&wlan_iface_name[4]);
		idx2 = WLAN_REPEATER_ITF_INDEX;
	}
#endif

	*root_idx = idx1;
	*vindex = idx2;

	return 0;
}
#endif


#ifdef WLAN_FAST_INIT
int setupWDS(struct wifi_mib *pmib)
{
#ifdef WLAN_WDS
	unsigned char value[128];
	char macaddr[16];
	char vChar, wds_enabled;
	char parm[128];
	char wds_num;
	char wdsPrivacy;
	WDS_T Entry;
	char wdsif[11];
	int i;
	int status = 0;
	MIB_CE_MBSSIB_T mbssid_entry;

	wlan_getEntry(&mbssid_entry, 0);
	vChar = mbssid_entry.wlanMode;
	mib_get_s(MIB_WLAN_WDS_ENABLED, (void *)&wds_enabled, sizeof(wds_enabled));
	if (vChar != AP_WDS_MODE || wds_enabled == 0) {
		(&pmib->dot11WdsInfo)->wdsNum = 0;
		(&pmib->dot11WdsInfo)->wdsEnabled = 0;

		for (i=0; i<MAX_WDS_NUM; i++) {
			snprintf(wdsif, 10, (char *)WDSIF, (char*)getWlanIfName());
			wdsif[9] = '0' + i;
			wdsif[10] = '\0';
			//ifconfig wlanX-wdsX down
			status|=va_cmd(IFCONFIG, 2, 1, wdsif, "down");
			//brctl delif br0 wlanX-wdsX
			status|=va_cmd(BRCTL, 3, 1, "delif", (char *)BRIF, wdsif);
		}
		return 0;
	}

	// wds_pure
	(&pmib->dot11WdsInfo)->wdsPure = 0;

	// wds_enable
	mib_get_s(MIB_WLAN_WDS_ENABLED, (void *)&vChar, sizeof(vChar));
	(&pmib->dot11WdsInfo)->wdsEnabled = vChar;
	(&pmib->dot11WdsInfo)->wdsNum = 0;

	//mib_get_s(MIB_WLAN_WDS_ENABLED, (void *)&vChar, sizeof(vChar));
	//if(vChar==1){
		for (i=0; i<MAX_WDS_NUM; i++) {
			snprintf(wdsif, 10, (char *)WDSIF, (char*)getWlanIfName());

			wdsif[9] = '0' + i;
			wdsif[10] = '\0';
			//ifconfig wlanX-wdsX down
			status|=va_cmd(IFCONFIG, 2, 1, wdsif, "down");
			//brctl delif br0 wlanX-wdsX
			status|=va_cmd(BRCTL, 3, 1, "delif", (char *)BRIF, wdsif);
		}
	//}

	mib_get_s(MIB_WLAN_WDS_NUM, &wds_num, sizeof(wds_num));
	snprintf(wdsif, 10, (char *)WDSIF, (char*)getWlanIfName());
	(&pmib->dot11WdsInfo)->wdsNum = 0;

	for(i=0;i<wds_num;i++){
		if (!mib_chain_get(MIB_WDS_TBL, i, (void *)&Entry))
			continue;

		memcpy((&(&pmib->dot11WdsInfo)->entry[i])->macAddr, Entry.macAddr, 6);
		(&(&pmib->dot11WdsInfo)->entry[i])->txRate = Entry.fixedTxRate;
		(&pmib->dot11WdsInfo)->wdsNum++;
		//ifconfig wlanX-wdsX hw ether 00e04c867001
		getInAddr(getWlanIfName(), HW_ADDR, macaddr);
		wdsif[9] = '0'+i;
		wdsif[10] = '\0';
		status|=va_cmd(IFCONFIG, 4, 1, wdsif, "hw", "ether", macaddr);

		//brctl delif br0 wlanX-wdsX
		//va_cmd(BRCTL, 3, 1, "delif", (char *)BRIF, wdsif);
		//brctl addif br0 wlanX-wdsX
		status|=va_cmd(BRCTL, 3, 1, "addif", (char *)BRIF, wdsif);
		//ifconfig wlanX-wdsX up
		status|=va_cmd(IFCONFIG, 2, 1, wdsif, "up");
	}

	// wds_encrypt
	mib_get_s(MIB_WLAN_WDS_ENCRYPT, &vChar, sizeof(vChar));
	if (vChar == WDS_ENCRYPT_DISABLED)//open
		wdsPrivacy = 0;
	else if (vChar == WDS_ENCRYPT_WEP64) {//wep 40
		wdsPrivacy = 1;
	}
	else if (vChar == WDS_ENCRYPT_WEP128) {//wep 104
		wdsPrivacy = 5;
	}
	else if (vChar == WDS_ENCRYPT_TKIP){//tkip
		wdsPrivacy = 2;
	}
	else if(vChar == WDS_ENCRYPT_AES){//ccmp
		wdsPrivacy = 4;
	}
	if(wdsPrivacy == 1 || wdsPrivacy == 5){
		mib_get_s(MIB_WLAN_WDS_WEP_KEY, (void *)value, sizeof(value));
		if(wdsPrivacy == 1)
			rtk_string_to_hex((char *)value, (&pmib->dot11WdsInfo)->wdsWepKey, 10);
		else
			rtk_string_to_hex((char *)value, (&pmib->dot11WdsInfo)->wdsWepKey, 26);

	}
	else if(wdsPrivacy == 2|| wdsPrivacy == 4){
		mib_get_s(MIB_WLAN_WDS_PSK, (void *)value, sizeof(value));
		strcpy((&pmib->dot11WdsInfo)->wdsPskPassPhrase, value);
	}

	(&pmib->dot11WdsInfo)->wdsPrivacy = wdsPrivacy;

	(&pmib->dot11WdsInfo)->wdsPriority = 1;

	//for MOD multicast-filter
	mib_get_s(MIB_WLAN_WDS_ENABLED, (void *)&vChar, sizeof(vChar));
	if (vChar) {
		//brctl clrfltrport br0
		va_cmd(BRCTL,2,1,"clrfltrport",(char *)BRIF);
		//brctl setfltrport br0 11111
		va_cmd(BRCTL,3,1,"setfltrport",(char *)BRIF,"11111");
		//brctl setfltrport br0 55555
		va_cmd(BRCTL,3,1,"setfltrport",(char *)BRIF,"55555");
		//brctl setfltrport br0 2105
		va_cmd(BRCTL,3,1,"setfltrport",(char *)BRIF,"2105");
		//brctl setfltrport br0 2105
		va_cmd(BRCTL,3,1,"setfltrport",(char *)BRIF,"2107");
	}

	return status;
#else
	return 0;
#endif
}
#else
int setupWDS()
{
#ifdef WLAN_WDS
	unsigned char value[128];
	char macaddr[16];
	char vChar, wds_enabled;
	char parm[128];
	char wds_num;
	char wdsPrivacy;
	WDS_T Entry;
	char wdsif[11];
	int i;
	int status = 0;

#ifdef CONFIG_RTK_DEV_AP
   unsigned char tmpmac[6] = {0};
#endif

	MIB_CE_MBSSIB_T mbssid_entry;

	wlan_getEntry(&mbssid_entry, 0);
	vChar = mbssid_entry.wlanMode;
	mib_get_s(MIB_WLAN_WDS_ENABLED, (void *)&wds_enabled, sizeof(wds_enabled));
	if (vChar != AP_WDS_MODE || wds_enabled == 0) {
		status|=va_cmd(IWPRIV, 3, 1, (char*)getWlanIfName(), "set_mib", "wds_num=0");
		status|=va_cmd(IWPRIV, 3, 1, (char*)getWlanIfName(), "set_mib", "wds_enable=0");

		for (i=0; i<MAX_WDS_NUM; i++) {
			snprintf(wdsif, 10, (char *)WDSIF, (char*)getWlanIfName());
			wdsif[9] = '0' + i;
			wdsif[10] = '\0';
			//ifconfig wlanX-wdsX down
			status|=va_cmd(IFCONFIG, 2, 1, wdsif, "down");
			//brctl delif br0 wlanX-wdsX
			status|=va_cmd(BRCTL, 3, 1, "delif", (char *)BRIF, wdsif);
		}
		return 0;
	}

	// wds_pure
	status|=va_cmd(IWPRIV, 3, 1, (char*)getWlanIfName(), "set_mib", "wds_pure=0");

	// wds_enable
	mib_get_s(MIB_WLAN_WDS_ENABLED, (void *)&vChar, sizeof(vChar));
	snprintf(parm,  sizeof(parm), "wds_enable=%u", vChar);
	status|=va_cmd(IWPRIV, 3, 1, (char*)getWlanIfName(), "set_mib", parm);

	status|=va_cmd(IWPRIV, 3, 1, (char*)getWlanIfName(), "set_mib", "wds_num=0");

	//mib_get_s(MIB_WLAN_WDS_ENABLED, (void *)&vChar, sizeof(vChar));
	//if(vChar==1){
		for (i=0; i<MAX_WDS_NUM; i++) {
			snprintf(wdsif, 10, (char *)WDSIF, (char*)getWlanIfName());

			wdsif[9] = '0' + i;
			wdsif[10] = '\0';
			//ifconfig wlanX-wdsX down
			status|=va_cmd(IFCONFIG, 2, 1, wdsif, "down");
			//brctl delif br0 wlanX-wdsX
			status|=va_cmd(BRCTL, 3, 1, "delif", (char *)BRIF, wdsif);
		}
	//}

	mib_get_s(MIB_WLAN_WDS_NUM, &wds_num, sizeof(wds_num));
	snprintf(wdsif, 10, (char *)WDSIF, (char*)getWlanIfName());

	for(i=0;i<wds_num;i++){
		if (!mib_chain_get(MIB_WDS_TBL, i, (void *)&Entry))
			continue;
		snprintf(parm, sizeof(parm), "wds_add=%02x%02x%02x%02x%02x%02x,%u",
			Entry.macAddr[0], Entry.macAddr[1], Entry.macAddr[2],
			Entry.macAddr[3], Entry.macAddr[4], Entry.macAddr[5], Entry.fixedTxRate);
		status|=va_cmd(IWPRIV, 3, 1, (char*)getWlanIfName(), "set_mib", parm);
		//ifconfig wlanX-wdsX hw ether 00e04c867001
		getInAddr(getWlanIfName(), HW_ADDR, macaddr);
		wdsif[9] = '0'+i;
		wdsif[10] = '\0';
	
#ifdef CONFIG_RTK_DEV_AP
		int j;
		mib_get_s(MIB_ELAN_MAC_ADDR, (void *)tmpmac, sizeof(tmpmac)); // same as wlan0 mac
		if(wlan_idx == 1){
			/* derive wlan1 mac */
#ifdef WLAN_MBSSID					
			for (j=1; j<=WLAN_MBSSID_NUM; j++) {
				setup_mac_addr(tmpmac, 1);
			}
#endif
			setup_mac_addr(tmpmac, 1);	
			sprintf(macaddr, "%02x%02x%02x%02x%02x%02x", tmpmac[0], tmpmac[1],
				tmpmac[2], tmpmac[3], tmpmac[4], tmpmac[5]);			
		}
#endif
		
		status|=va_cmd(IFCONFIG, 4, 1, wdsif, "hw", "ether", macaddr);

		//brctl delif br0 wlanX-wdsX
		//va_cmd(BRCTL, 3, 1, "delif", (char *)BRIF, wdsif);
		//brctl addif br0 wlanX-wdsX
		status|=va_cmd(BRCTL, 3, 1, "addif", (char *)BRIF, wdsif);
		//ifconfig wlanX-wdsX up
		status|=va_cmd(IFCONFIG, 2, 1, wdsif, "up");
	}

	// wds_encrypt
	mib_get_s(MIB_WLAN_WDS_ENCRYPT, &vChar, sizeof(vChar));
	if (vChar == WDS_ENCRYPT_DISABLED)//open
		wdsPrivacy = 0;
	else if (vChar == WDS_ENCRYPT_WEP64) {//wep 40
		wdsPrivacy = 1;
	}
	else if (vChar == WDS_ENCRYPT_WEP128) {//wep 104
		wdsPrivacy = 5;
	}
	else if (vChar == WDS_ENCRYPT_TKIP){//tkip
		wdsPrivacy = 2;
	}
	else if(vChar == WDS_ENCRYPT_AES){//ccmp
		wdsPrivacy = 4;
	}
	if(wdsPrivacy == 1 || wdsPrivacy == 5){
		mib_get_s(MIB_WLAN_WDS_WEP_KEY, (void *)value, sizeof(value));
		snprintf(parm, sizeof(parm), "wds_wepkey=%s", value);
		status|=va_cmd(IWPRIV, 3, 1, (char*)getWlanIfName(), "set_mib", parm);

	}
	else if(wdsPrivacy == 2|| wdsPrivacy == 4){
		mib_get_s(MIB_WLAN_WDS_PSK, (void *)value, sizeof(value));
		snprintf(parm, sizeof(parm), "wds_passphrase=%s", value);
		status|=va_cmd(IWPRIV, 3, 1, (char*)getWlanIfName(), "set_mib", parm);
	}
	snprintf(parm, sizeof(parm), "wds_encrypt=%u", wdsPrivacy);
	status|=va_cmd(IWPRIV, 3, 1, (char*)getWlanIfName(), "set_mib", parm);

	status|=va_cmd(IWPRIV, 3, 1, (char*)getWlanIfName(), "set_mib", "wds_priority=1");

	//for MOD multicast-filter
	mib_get_s(MIB_WLAN_WDS_ENABLED, (void *)&vChar, sizeof(vChar));
	if (vChar) {
		//brctl clrfltrport br0
		va_cmd(BRCTL,2,1,"clrfltrport",(char *)BRIF);
		//brctl setfltrport br0 11111
		va_cmd(BRCTL,3,1,"setfltrport",(char *)BRIF,"11111");
		//brctl setfltrport br0 55555
		va_cmd(BRCTL,3,1,"setfltrport",(char *)BRIF,"55555");
		//brctl setfltrport br0 2105
		va_cmd(BRCTL,3,1,"setfltrport",(char *)BRIF,"2105");
		//brctl setfltrport br0 2105
		va_cmd(BRCTL,3,1,"setfltrport",(char *)BRIF,"2107");
	}

	return status;
#else
	return 0;
#endif
}
#endif
//krammer
#ifdef WLAN_MBSSID
static void _set_vap_para(const char* arg1, const char* arg2){
	int i, start, end;
	char ifname[16];

	for(i=0; i<WLAN_VAP_USED_NUM; i++){
		snprintf(ifname, 16, "%s-" VAP_IF_ONLY, (char *)getWlanIfName(), i);
		va_cmd(IWPRIV, 3, 1, ifname, arg1, arg2);
	}
}

#define set_vap_para(a, b) _set_vap_para(a, b)
#else
#define set_vap_para(a, b)
#endif

#ifdef WLAN_MBSSID
static void _set_vap_para_by_index(const char* arg1, const char* arg2, int i){
	char ifname[16];

	if(i >= WLAN_VAP_ITF_INDEX && i < WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM){
		snprintf(ifname, 16, "%s-" VAP_IF_ONLY, (char *)getWlanIfName(), i-WLAN_VAP_ITF_INDEX);
		va_cmd(IWPRIV, 3, 1, ifname, arg1, arg2);
	}
}

#define set_vap_para_by_index(a, b, c) _set_vap_para_by_index(a, b, c)
#else
#define set_vap_para_by_index(a, b, c)
#endif

#ifdef WLAN_FAST_INIT
static void setupWLan_WPA(struct wifi_mib *pmib, int vwlan_idx)
{
	char parm[64];
	int vInt, status;
	MIB_CE_MBSSIB_T Entry;

	mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry);

	// encmode
	vInt = (int)Entry.encrypt;

	if (Entry.encrypt == WIFI_SEC_WEP)	// WEP mode
	{
		// psk_enable: 0
		(&pmib->dot1180211AuthEntry)->dot11EnablePSK = 0;
		if (Entry.wep == WEP64) {
			// 64 bits
			vInt = 1; // encmode = 1
			// wepkey1
			// Mason Yu. 201009_new_security.
			memcpy((&pmib->dot11DefaultKeysTable)->keytype,Entry.wep64Key1,5);
		}
		else {
			// 128 bits
			vInt = 5; // encmode = 5
			// wepkey1
			// Mason Yu. 201009_new_security.
			memcpy((&pmib->dot11DefaultKeysTable)->keytype,Entry.wep128Key1,13);
		}
	}
	// Kaohj --- this is for driver level security.
	// ignoring it if using userland 'auth' security
	#ifdef CONFIG_RTL_WAPI_SUPPORT
	else if (Entry.encrypt == WIFI_SEC_WAPI) {
		//char wapiAuth;
		vInt = 0; // encmode = 0 for WAPI
		(&pmib->wapiInfo)->wapiType = Entry.wapiAuth;
		// psk_enable: 0
		(&pmib->dot1180211AuthEntry)->dot11EnablePSK = 0;

		if (Entry.wapiAuth==2) { //PSK
			//char pskFormat, pskLen, pskValue[MAX_PSK_LEN];
			//mib_get_s(MIB_WLAN_WAPI_PSK_FORMAT, (void *)&pskFormat, sizeof(pskFormat));
			//mib_get_s(MIB_WLAN_WAPI_PSK, (void *)pskValue, sizeof(pskValue));
			//mib_get_s(MIB_WLAN_WAPI_PSKLEN, (void *)&pskLen, sizeof(pskLen));


			if (Entry.wapiPskFormat) { // HEX
				int i;
				char hexstr[4];
				for (i=0;i<Entry.wapiPskLen;i++) {
					snprintf(hexstr, sizeof(hexstr), "%02x", Entry.wapiPsk[i]);
					strcat(parm, hexstr);
				}
				(&(&pmib->wapiInfo)->wapiPsk)->len = Entry.wapiPskLen;
				memcpy((&(&pmib->wapiInfo)->wapiPsk)->octet, parm, Entry.wapiPskLen*2);


			} else { // passphrase
				(&(&pmib->wapiInfo)->wapiPsk)->len = Entry.wapiPskLen;
				memcpy((&(&pmib->wapiInfo)->wapiPsk)->octet, Entry.wapiPsk, Entry.wapiPskLen);
			}
		} else { //AS

		}


	}
	#endif
	else if (Entry.encrypt >= WIFI_SEC_WPA) {	// WPA setup
		// Mason Yu. 201009_new_security. Start
		if (Entry.encrypt == WIFI_SEC_WPA
			|| Entry.encrypt == WIFI_SEC_WPA2_MIXED) {
			(&pmib->dot1180211AuthEntry)->dot11WPACipher = wl_cipher2mib(Entry.unicastCipher);
		} else {
			(&pmib->dot1180211AuthEntry)->dot11WPACipher = 0;
		}

		if (Entry.encrypt == WIFI_SEC_WPA2
			|| Entry.encrypt == WIFI_SEC_WPA2_MIXED) {
			(&pmib->dot1180211AuthEntry)->dot11WPA2Cipher = wl_cipher2mib(Entry.wpa2UnicastCipher);
		} else {
			(&pmib->dot1180211AuthEntry)->dot11WPA2Cipher = 0;
		}
		// Mason Yu. 201009_new_security. End

		if (!is8021xEnabled(vwlan_idx)) {
			// psk_enable: 1: WPA, 2: WPA2, 3: WPA+WPA2
			(&pmib->dot1180211AuthEntry)->dot11EnablePSK = Entry.encrypt/2;

			// passphrase
			strcpy((&pmib->dot1180211AuthEntry)->dot11PassPhrase, Entry.wpaPSK);

			(&pmib->dot1180211AuthEntry)->dot11GKRekeyTime = Entry.wpaGroupRekeyTime;
		}
		else {
			// psk_enable: 0
			(&pmib->dot1180211AuthEntry)->dot11EnablePSK = 0;
		}
		vInt = 2;
	}
	else {
		// psk_enable: 0
		(&pmib->dot1180211AuthEntry)->dot11EnablePSK = 0;
	}
	(&pmib->dot1180211AuthEntry)->dot11PrivacyAlgrthm = vInt;
}

#else
/*
 *	vwlan_idx:
 *	0:	Root
 *	1 ~ :	VAP, Repeater
 */
static int setupWLan_WPA(int vwlan_idx)
{
	char ifname[16];
	char parm[128];
	int vInt, status=0;
	MIB_CE_MBSSIB_T Entry;

	if(mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry) == 0)
		printf("\n %s %d\n", __func__, __LINE__);

	rtk_wlan_get_ifname(wlan_idx, vwlan_idx, ifname);

	// encmode
	vInt = (int)Entry.encrypt;

	if (Entry.encrypt == WIFI_SEC_WEP)	// WEP mode
	{
		// psk_enable: 0
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "psk_enable=0");

		//wep key id
		snprintf(parm, sizeof(parm), "wepdkeyid=%d", Entry.wepDefaultKey);
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

		if (Entry.wep == WEP64) {
			// 64 bits
			vInt = 1; // encmode = 1
			// wepkey1
			// Mason Yu. 201009_new_security.
			snprintf(parm, sizeof(parm), "wepkey1=%02x%02x%02x%02x%02x", Entry.wep64Key1[0],
				Entry.wep64Key1[1], Entry.wep64Key1[2], Entry.wep64Key1[3], Entry.wep64Key1[4]);
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
			snprintf(parm, sizeof(parm), "wepkey2=%02x%02x%02x%02x%02x", Entry.wep64Key2[0],
				Entry.wep64Key2[1], Entry.wep64Key2[2], Entry.wep64Key2[3], Entry.wep64Key2[4]);
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
			snprintf(parm, sizeof(parm), "wepkey3=%02x%02x%02x%02x%02x", Entry.wep64Key3[0],
				Entry.wep64Key3[1], Entry.wep64Key3[2], Entry.wep64Key3[3], Entry.wep64Key3[4]);
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
			snprintf(parm, sizeof(parm), "wepkey4=%02x%02x%02x%02x%02x", Entry.wep64Key4[0],
				Entry.wep64Key4[1], Entry.wep64Key4[2], Entry.wep64Key4[3], Entry.wep64Key4[4]);
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}
		else {
			// 128 bits
			vInt = 5; // encmode = 5
			// wepkey1
			// Mason Yu. 201009_new_security.
			snprintf(parm, sizeof(parm), "wepkey1=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				Entry.wep128Key1[0], Entry.wep128Key1[1], Entry.wep128Key1[2], Entry.wep128Key1[3], Entry.wep128Key1[4],
				Entry.wep128Key1[5], Entry.wep128Key1[6], Entry.wep128Key1[7], Entry.wep128Key1[8], Entry.wep128Key1[9],
				Entry.wep128Key1[10], Entry.wep128Key1[11], Entry.wep128Key1[12]);
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
			snprintf(parm, sizeof(parm), "wepkey2=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				Entry.wep128Key2[0], Entry.wep128Key2[1], Entry.wep128Key2[2], Entry.wep128Key2[3], Entry.wep128Key2[4],
				Entry.wep128Key2[5], Entry.wep128Key2[6], Entry.wep128Key2[7], Entry.wep128Key2[8], Entry.wep128Key2[9],
				Entry.wep128Key2[10], Entry.wep128Key2[11], Entry.wep128Key2[12]);
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
			snprintf(parm, sizeof(parm), "wepkey3=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				Entry.wep128Key3[0], Entry.wep128Key3[1], Entry.wep128Key3[2], Entry.wep128Key3[3], Entry.wep128Key3[4],
				Entry.wep128Key3[5], Entry.wep128Key3[6], Entry.wep128Key3[7], Entry.wep128Key3[8], Entry.wep128Key3[9],
				Entry.wep128Key3[10], Entry.wep128Key3[11], Entry.wep128Key3[12]);
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
			snprintf(parm, sizeof(parm), "wepkey4=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				Entry.wep128Key4[0], Entry.wep128Key4[1], Entry.wep128Key4[2], Entry.wep128Key4[3], Entry.wep128Key4[4],
				Entry.wep128Key4[5], Entry.wep128Key4[6], Entry.wep128Key4[7], Entry.wep128Key4[8], Entry.wep128Key4[9],
				Entry.wep128Key4[10], Entry.wep128Key4[11], Entry.wep128Key4[12]);
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}
	}
	// Kaohj --- this is for driver level security.
	// ignoring it if using userland 'auth' security
	#ifdef CONFIG_RTL_WAPI_SUPPORT
	else if (Entry.encrypt == WIFI_SEC_WAPI) {
		//char wapiAuth;
		vInt = 0; // encmode = 0 for WAPI
		snprintf(parm, sizeof(parm), "wapiType=%d", Entry.wapiAuth);
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

		// psk_enable: 0
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "psk_enable=0");

		if (Entry.wapiAuth==2) { //PSK
			//char pskFormat, pskLen, pskValue[MAX_PSK_LEN];
			//mib_get_s(MIB_WLAN_WAPI_PSK_FORMAT, (void *)&pskFormat, sizeof(pskFormat));
			//mib_get_s(MIB_WLAN_WAPI_PSK, (void *)pskValue, sizeof(pskValue));
			//mib_get_s(MIB_WLAN_WAPI_PSKLEN, (void *)&pskLen, sizeof(pskLen));


			if (Entry.wapiPskFormat) { // HEX
				int i;
				char hexstr[4];
				snprintf(parm, sizeof(parm), "wapiPsk=");
				for (i=0;i<Entry.wapiPskLen;i++) {
					snprintf(hexstr, sizeof(hexstr), "%02x", Entry.wapiPsk[i]);
					strcat(parm, hexstr);
				}
				strcat(parm, ",");
				snprintf(hexstr, sizeof(hexstr), "%x", Entry.wapiPskLen);
				strcat(parm, hexstr);
				status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

			} else { // passphrase
				snprintf(parm, sizeof(parm), "wapiPsk=%s,%x", Entry.wapiPsk, Entry.wapiPskLen);
				status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
			}
		} else { //AS

		}


	}
	#endif
	else if (Entry.encrypt >= WIFI_SEC_WPA) {	// WPA setup
		// Mason Yu. 201009_new_security. Start
		if (Entry.encrypt == WIFI_SEC_WPA
#ifndef NEW_WIFI_SEC
			|| Entry.encrypt == WIFI_SEC_WPA2_MIXED
#endif
			) {
			snprintf(parm, sizeof(parm), "wpa_cipher=%d", wl_cipher2mib(Entry.unicastCipher));
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}
#ifdef NEW_WIFI_SEC
		else if (Entry.encrypt == WIFI_SEC_WPA2_MIXED){
			snprintf(parm, sizeof(parm), "wpa_cipher=%d", wl_cipher2mib(WPA_CIPHER_MIXED));
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}
#endif
		else {
			snprintf(parm, sizeof(parm), "wpa_cipher=0");
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}

		if (Entry.encrypt == WIFI_SEC_WPA2
#ifndef NEW_WIFI_SEC
			|| Entry.encrypt == WIFI_SEC_WPA2_MIXED
#endif
			) {
			snprintf(parm, sizeof(parm), "wpa2_cipher=%d", wl_cipher2mib(Entry.wpa2UnicastCipher));
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}
#ifdef NEW_WIFI_SEC
		else if (Entry.encrypt == WIFI_SEC_WPA2_MIXED){
			snprintf(parm, sizeof(parm), "wpa2_cipher=%d", wl_cipher2mib(WPA_CIPHER_MIXED));
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}
#endif
#ifdef WLAN_WPA3
		else if (Entry.encrypt ==  WIFI_SEC_WPA3 || Entry.encrypt == WIFI_SEC_WPA2_WPA3_MIXED){
			snprintf(parm, sizeof(parm), "wpa2_cipher=%d", wl_cipher2mib(WPA_CIPHER_AES));
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}
#endif
		else {
			snprintf(parm, sizeof(parm), "wpa2_cipher=0");
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}
		// Mason Yu. 201009_new_security. End
		if (!is8021xEnabled(vwlan_idx)) {
			// psk_enable: 1: WPA, 2: WPA2, 3: WPA+WPA2
			snprintf(parm, sizeof(parm), "psk_enable=%d", Entry.encrypt/2);
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

			// passphrase
			snprintf(parm, sizeof(parm), "passphrase=%s", Entry.wpaPSK);
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

		}
		else {
			// psk_enable: 0
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "psk_enable=0");
		}

		snprintf(parm, sizeof(parm), "gk_rekey=%u", Entry.wpaGroupRekeyTime);
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

		vInt = 2;
	}
	else {
		// psk_enable: 0
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "psk_enable=0");
	}
#ifdef WLAN_11W
	if (Entry.encrypt == WIFI_SEC_WPA2){
		snprintf(parm, sizeof(parm), "dot11IEEE80211W=%d", Entry.dotIEEE80211W);
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		snprintf(parm, sizeof(parm), "enableSHA256=%d", Entry.sha256);
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	}
#ifdef WLAN_WPA3
	else if (Entry.encrypt == WIFI_SEC_WPA3){
		snprintf(parm, sizeof(parm), "dot11IEEE80211W=2");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		snprintf(parm, sizeof(parm), "enableSHA256=1");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	}
	else if (Entry.encrypt == WIFI_SEC_WPA2_WPA3_MIXED){
		snprintf(parm, sizeof(parm), "dot11IEEE80211W=1");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		snprintf(parm, sizeof(parm), "enableSHA256=1");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	}
#endif
	else{
		snprintf(parm, sizeof(parm), "dot11IEEE80211W=0");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		snprintf(parm, sizeof(parm), "enableSHA256=0");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	}
#endif
	snprintf(parm, sizeof(parm), "encmode=%u", vInt);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	return status;
}
#endif

#ifdef WLAN_11R
static int setupWLan_FT(int vwlan_idx)
{
	char ifname[16];
	char parm[64];
	int vInt, status;
	MIB_CE_MBSSIB_T Entry;

	mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry);

	rtk_wlan_get_ifname(wlan_idx, vwlan_idx, ifname);

	snprintf(parm, sizeof(parm), "ft_enable=%d", Entry.ft_enable);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	snprintf(parm, sizeof(parm), "ft_mdid=%s", Entry.ft_mdid);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	snprintf(parm, sizeof(parm), "ft_over_ds=%d", Entry.ft_over_ds);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	snprintf(parm, sizeof(parm), "ft_res_request=%d", Entry.ft_res_request);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	snprintf(parm, sizeof(parm), "ft_r0key_timeout=%d", Entry.ft_r0key_timeout);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	snprintf(parm, sizeof(parm), "ft_reasoc_timeout=%d", Entry.ft_reasoc_timeout);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	snprintf(parm, sizeof(parm), "ft_r0kh_id=%s", Entry.ft_r0kh_id);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	snprintf(parm, sizeof(parm), "ft_push=%d", Entry.ft_push);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	return status;
}

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

int start_FT()
{
	MIB_CE_MBSSIB_T Entry;
	int vwlan_idx=0, i;
	int status=0;
	char wlanDisabled=0;
	char *cmd_opt[16];
	int cmd_cnt = 0;
	int idx;
	//char intfList[200]="";
	char intfList[((WLAN_MAX_ITF_INDEX+1)*2)][32]={0};
	int intfNum=0;
	int enablePush=0;
#if defined(WLAN_DUALBAND_CONCURRENT)
	int orig_wlan_idx=wlan_idx;
#endif

	genFtKhConfig();
	for(i = 0; i<NUM_WLAN_INTERFACE; i++) {
		wlan_idx = i;
#ifdef WLAN_MBSSID
		for (vwlan_idx=0; vwlan_idx<=NUM_VWLAN_INTERFACE; vwlan_idx++) {
#endif
			wlan_getEntry(&Entry, vwlan_idx);
#ifndef WLAN_USE_VAP_AS_SSID1
			if (vwlan_idx==WLAN_ROOT_ITF_INDEX) // root
				wlanDisabled = Entry.wlanDisabled;
#endif
			if (wlanDisabled || Entry.wlanDisabled)
				continue;
			if (Entry.wlanMode == CLIENT_MODE)
				continue;

			if ((Entry.encrypt==4||Entry.encrypt==6) && Entry.ft_enable) {

				rtk_wlan_get_ifname(i, vwlan_idx, intfList[intfNum]);

				intfNum++;
				if (Entry.ft_push)
					enablePush = 1;
			}
#ifdef WLAN_MBSSID
		}
#endif
	}

	fprintf(stderr, "START 802.11r SETUP!\n");
	cmd_opt[cmd_cnt++] = "";

	if (intfNum) {
		cmd_opt[cmd_cnt++] = "-w";
		for (i=0; i<intfNum; i++)
			cmd_opt[cmd_cnt++] = (char *)intfList[i];

		cmd_opt[cmd_cnt++] = "-c";
		cmd_opt[cmd_cnt++] = (char *)FT_CONF;

		cmd_opt[cmd_cnt++] = "-pid";
		cmd_opt[cmd_cnt++] = (char *)FT_PID;

		cmd_opt[cmd_cnt] = 0;
		printf("CMD ARGS: ");
		for (idx=0; idx<cmd_cnt;idx++)
			printf("%s ", cmd_opt[idx]);
		printf("\n");

		status |= do_nice_cmd(FT_DAEMON_PROG, cmd_opt, 0);
	}

#if defined(WLAN_DUALBAND_CONCURRENT)
	wlan_idx = orig_wlan_idx;
#endif

	return status;
}
#endif

#ifdef WLAN_11K
static int setupWLan_dot11K(int vwlan_idx)
{
	char ifname[16];
	char parm[128];
	int vInt, status = 0;
	MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_ROAMING
	unsigned char phyband=0, roaming_enable=0;
#endif

	if(mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry) == 0)
		printf("\n %s %d\n", __func__, __LINE__);

	rtk_wlan_get_ifname(wlan_idx, vwlan_idx, ifname);

#ifdef WLAN_ROAMING
	if(vwlan_idx == 0 && Entry.rm_activated == 0){
		mib_get_s(MIB_WLAN_PHY_BAND_SELECT, &phyband, sizeof(phyband));
		mib_get_s(phyband==PHYBAND_5G? MIB_ROAMING5G_ENABLE:MIB_ROAMING2G_ENABLE, &roaming_enable, sizeof(roaming_enable));
		if(roaming_enable == 1)
			Entry.rm_activated = roaming_enable;
	}
#endif

	snprintf(parm, sizeof(parm), "rm_activated=%d", Entry.rm_activated);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	if(Entry.rm_activated){
		snprintf(parm, sizeof(parm), "rm_link_measure=1");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

		snprintf(parm, sizeof(parm), "rm_beacon_passive=1");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

		snprintf(parm, sizeof(parm), "rm_beacon_active=1");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

		snprintf(parm, sizeof(parm), "rm_beacon_table=1");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

		snprintf(parm, sizeof(parm), "rm_neighbor_report=1");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

		snprintf(parm, sizeof(parm), "rm_ap_channel_report=1");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

#ifdef WLAN_11V
		snprintf(parm, sizeof(parm), "Is11kDaemonOn=1");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
#endif
	}

#ifdef RTK_MULTI_AP
	unsigned char map_state = 0;
	mib_get(MIB_MAP_CONTROLLER, (void *)&map_state);
	if(map_state)
		return status;
#endif

#ifndef WLAN_ROAMING
	if(Entry.wlanDisabled==0 && Entry.rm_activated)
		status|=va_niced_cmd(DOT11K_DAEMON_PROG, 2, 0, "-i", ifname);
#endif

	return status;
}
#endif

#ifdef WLAN_11V
static int setupWLan_dot11V(int vwlan_idx)
{
	char ifname[16];
	char parm[128];
	int vInt, status = 0;
	MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_ROAMING
	unsigned char phyband=0, roaming_enable=0;
#endif

	if(mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry) == 0)
		printf("\n %s %d\n", __func__, __LINE__);

	rtk_wlan_get_ifname(wlan_idx, vwlan_idx, ifname);
#ifdef WLAN_ROAMING
	if(vwlan_idx == 0 && Entry.rm_activated == 0){
		mib_get_s(MIB_WLAN_PHY_BAND_SELECT, &phyband, sizeof(phyband));
		mib_get_s(phyband==PHYBAND_5G? MIB_ROAMING5G_ENABLE:MIB_ROAMING2G_ENABLE, &roaming_enable, sizeof(roaming_enable));
		if(roaming_enable == 1)
			Entry.rm_activated = Entry.BssTransEnable = roaming_enable;
	}
#endif

	if(Entry.rm_activated){
		snprintf(parm, sizeof(parm), "BssTransEnable=%d", Entry.BssTransEnable);
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	}
	else{
		snprintf(parm, sizeof(parm), "BssTransEnable=0");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	}
	return status;
}
#endif

#ifdef RTK_MULTI_AP
static int setupWLan_multi_ap(int vwlan_idx)
{
	char ifname[16];
	char parm[128];
	int vInt, status = 0;
	MIB_CE_MBSSIB_T Entry;
#ifdef MAP_GROUP_SUPPORT
	unsigned char map_group_ssid[MAX_SSID_LEN]={0}, map_group_wpaPsk[MAX_PSK_LEN+1]={0};
#endif

	if(!mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry)) {
		printf("Error! Get MIB_MBSSIB_TBL(setupWLan_multi_ap) error.\n");
	}


	if (vwlan_idx==0)
		strncpy(ifname, (char*)getWlanIfName(), 16);
	else {
		#ifdef WLAN_MBSSID
		if (vwlan_idx>=WLAN_VAP_ITF_INDEX && vwlan_idx<(WLAN_VAP_ITF_INDEX+WLAN_MBSSID_NUM))
			snprintf(ifname, 16, "%s-" VAP_IF_ONLY, (char *)getWlanIfName(), vwlan_idx-1);
		#endif
		#ifdef WLAN_UNIVERSAL_REPEATER
		if (vwlan_idx == WLAN_REPEATER_ITF_INDEX)
			snprintf(ifname, 16, "%s-" VXD_IF_ONLY, (char *)getWlanIfName());
		#endif

		#ifdef MAP_GROUP_SUPPORT

		if(
#ifdef RTK_RSV_VAP_FOR_EASYMESH
			vwlan_idx == MULTI_AP_BSS_IDX
#else
			vwlan_idx == WLAN_VAP_ITF_INDEX
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
			|| vwlan_idx == WLAN_REPEATER_ITF_INDEX
#endif
		)
		{
			if(Entry.wlanDisabled == 0)
			{
	
				mib_get_s(MIB_MAP_GROUP_SSID, (void *)&map_group_ssid,sizeof(map_group_ssid));
				mib_get_s(MIB_MAP_GROUP_WPA_PSK, (void *)&map_group_wpaPsk,sizeof(map_group_wpaPsk));
				if(strlen(map_group_ssid))
				{
					strcpy(Entry.ssid, map_group_ssid);
					#ifdef WLAN_UNIVERSAL_REPEATER
						if (vwlan_idx == WLAN_REPEATER_ITF_INDEX)
							mib_set( MIB_REPEATER_SSID1, (void *)map_group_ssid);
					#endif
					snprintf(parm, sizeof(parm), "ssid=%s", Entry.ssid);
					status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
				}

				if(strlen(map_group_wpaPsk))
				{
					strcpy(Entry.wpaPSK, map_group_wpaPsk);
					Entry.encrypt = WIFI_SEC_WPA2;
					Entry.wpaAuth = WPA_AUTH_PSK;
					Entry.wpa2UnicastCipher = WPA_CIPHER_AES;
					snprintf(parm, sizeof(parm), "psk_enable=%d", Entry.encrypt/2);
					status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
					snprintf(parm, sizeof(parm), "passphrase=%s", Entry.wpaPSK);
					status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
					snprintf(parm, sizeof(parm), "wpa2_cipher=%d", wl_cipher2mib(Entry.wpa2UnicastCipher));
					status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
					vInt = 2;
					snprintf(parm, sizeof(parm), "encmode=%u", vInt);
					status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
				}

				if(strlen(map_group_ssid) || strlen(map_group_wpaPsk))
					mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, vwlan_idx);
			}
		}
		#endif
	}

	{
		unsigned char map_state = 0;
		mib_get(MIB_MAP_CONTROLLER, (void *)&map_state);
		if(map_state) {
			if(Entry.multiap_bss_type == 0){
				Entry.multiap_bss_type = MAP_FRONTHAUL_BSS;
				mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, vwlan_idx);
#ifdef COMMIT_IMMEDIATELY
				Commit();
#endif // of #if COMMIT_IMMEDIATELY
			}

			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "a4_enable=1");
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "multiap_monitor_mode_disable=0");
		} else {
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "a4_enable=0");
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "multiap_monitor_mode_disable=1");
		}
	}

	snprintf(parm, sizeof(parm), "multiap_bss_type=%d", Entry.multiap_bss_type);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	return status;
}
#endif

#ifdef WLAN_FAST_INIT
static void setupWLan_802_1x(struct wifi_mib * pmib, int vwlan_idx)
{
	char parm[64];
	int vInt, status;
	MIB_CE_MBSSIB_T Entry;

	mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry);

	// Set 802.1x flag
	vInt = (int)Entry.encrypt;

	if (vInt <= WIFI_SEC_WEP)
	{
		// 802_1x
		(&pmib->dot118021xAuthEntry)->dot118021xAlgrthm = Entry.enable1X;
	}
#ifdef CONFIG_RTL_WAPI_SUPPORT
	else if (vInt == WIFI_SEC_WAPI)
		(&pmib->dot118021xAuthEntry)->dot118021xAlgrthm = 0;
#endif
	else
		(&pmib->dot118021xAuthEntry)->dot118021xAlgrthm = 1;
}
#else
/*
 *	vwlan_idx:
 *	0:	Root
 *	1 ~ :	VAP, Repeater
 */
static int setupWLan_802_1x(int vwlan_idx)
{
	char ifname[16];
	char parm[64];
	int vInt, status=0;
	MIB_CE_MBSSIB_T Entry;

	if(mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry) == 0)
		printf("\n %s %d\n", __func__, __LINE__);

	rtk_wlan_get_ifname(wlan_idx, vwlan_idx, ifname);

	// Set 802.1x flag
	vInt = (int)Entry.encrypt;

	if (vInt <= WIFI_SEC_WEP)
	{
		// 802_1x
		snprintf(parm, sizeof(parm), "802_1x=%u", Entry.enable1X);
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	}
#ifdef CONFIG_RTL_WAPI_SUPPORT
	else if (vInt == WIFI_SEC_WAPI)
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "802_1x=0");
#endif
	else
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "802_1x=1");

	return status;
}
#endif
#ifdef WLAN_FAST_INIT
static void setupWLan_dot11_auth(struct wifi_mib* pmib, int vwlan_idx)
{
	char parm[64];
	unsigned char auth;
	#ifdef CONFIG_WIFI_SIMPLE_CONFIG
	unsigned char wsc_disabled;
	#endif
	int vInt, status;
	MIB_CE_MBSSIB_T Entry;

	mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry);

	auth = Entry.authType;

	if (Entry.authType == AUTH_SHARED && Entry.encrypt != WIFI_SEC_WEP)
		// shared-key and not WEP enabled, force to open-system
		auth = (AUTH_TYPE_T)AUTH_OPEN;
	#ifdef CONFIG_RTL_WAPI_SUPPORT
	if (Entry.encrypt==WIFI_SEC_WAPI)
		auth = (AUTH_TYPE_T)AUTH_OPEN;
	else
		(&pmib->wapiInfo)->wapiType = 0;
	#endif

	#ifdef CONFIG_WIFI_SIMPLE_CONFIG //cathy
	if (vwlan_idx == 0) // root
		//clear wsc_enable, this parameter will be set in wsc daemon
		(&pmib->wscEntry)->wsc_enable = 0;
#if 0
	wsc_disabled = Entry.wsc_disabled;
	if(vwlan_idx == 0 && Entry.authType == AUTH_SHARED && Entry.encrypt == WIFI_SEC_WEP && wsc_disabled!=1)
		auth = (AUTH_TYPE_T)AUTH_BOTH;	//if root shared-key and wep/wps enable, force to open+shared system for wps
#endif
	#endif
	(&pmib->dot1180211AuthEntry)->dot11AuthAlgrthm = auth;

}
#else
/*
 *	vwlan_idx:
 *	0:	Root
 *	1 ~ :	VAP, Repeater
 */
static int setupWLan_dot11_auth(int vwlan_idx)
{
	char ifname[16];
	char parm[64];
	unsigned char auth;
	#ifdef CONFIG_WIFI_SIMPLE_CONFIG
	unsigned char wsc_disabled;
	#endif
	int vInt, status=0;
	MIB_CE_MBSSIB_T Entry;
	if(mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry) == 0)
		printf("\n %s %d\n", __func__, __LINE__);

	rtk_wlan_get_ifname(wlan_idx, vwlan_idx, ifname);

	auth = Entry.authType;

	if (Entry.authType == AUTH_SHARED && Entry.encrypt != WIFI_SEC_WEP)
		// shared-key and not WEP enabled, force to open-system
		auth = (AUTH_TYPE_T)AUTH_OPEN;
	#ifdef CONFIG_RTL_WAPI_SUPPORT
	if (Entry.encrypt==WIFI_SEC_WAPI)
		auth = (AUTH_TYPE_T)AUTH_OPEN;
	else
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "wapiType=0");
	#endif

	#ifdef CONFIG_WIFI_SIMPLE_CONFIG //cathy
#ifndef CONFIG_RTK_DEV_AP 
	if (vwlan_idx == 0) // root
#endif	
	{ // root
		//clear wsc_enable, this parameter will be set in wsc daemon
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "wsc_enable=0");
	}
#if 0
	wsc_disabled = Entry.wsc_disabled;
	if(vwlan_idx == 0 && Entry.authType == AUTH_SHARED && Entry.encrypt == WIFI_SEC_WEP && wsc_disabled!=1)
		auth = (AUTH_TYPE_T)AUTH_BOTH;	//if root shared-key and wep/wps enable, force to open+shared system for wps
#endif
	#endif
	snprintf(parm, sizeof(parm), "authtype=%u", auth);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	return status;
}
#endif
#ifdef WLAN_FAST_INIT
void setupWlanHWSetting(char *ifname, struct wifi_mib *pmib)
{
		unsigned char value[34];
#if defined(WLAN0_5G_SUPPORT) || defined(WLAN1_5G_SUPPORT)
		mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)value, sizeof(value));
		(&pmib->dot11RFEntry)->phyBandSelect = value[0];
#endif

#ifdef WLAN_DUALBAND_CONCURRENT
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_CCK_A, (void *)value, sizeof(value));
		else // wlan0
#endif
		mib_get_s(MIB_HW_TX_POWER_CCK_A, (void *)value, sizeof(value));
	if(value[0]!=0){
		(&pmib->efuseEntry)->enable_efuse = 0;
		memcpy((&pmib->dot11RFEntry)->pwrlevelCCK_A, value, MAX_CHAN_NUM);
	}
	else
		(&pmib->efuseEntry)->enable_efuse = 1;

#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_CCK_B, (void *)value, sizeof(value));
	else // wlan0
#endif
		mib_get_s(MIB_HW_TX_POWER_CCK_B, (void *)value, sizeof(value));
	if(value[0]!=0)
		memcpy((&pmib->dot11RFEntry)->pwrlevelCCK_B, value, MAX_CHAN_NUM);

#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_HT40_1S_A, (void *)value, sizeof(value));
	else // wlan0
#endif
		mib_get_s(MIB_HW_TX_POWER_HT40_1S_A, (void *)value, sizeof(value));
	if(value[0]!=0)
		memcpy((&pmib->dot11RFEntry)->pwrlevelHT40_1S_A, value, MAX_CHAN_NUM);

#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_HT40_1S_B, (void *)value, sizeof(value));
	else // wlan0
#endif
		mib_get_s(MIB_HW_TX_POWER_HT40_1S_B, (void *)value, sizeof(value));
	if(value[0]!=0)
		memcpy((&pmib->dot11RFEntry)->pwrlevelHT40_1S_B, value, MAX_CHAN_NUM);

#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_HT40_2S, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_TX_POWER_HT40_2S, (void *)value, sizeof(value));
		memcpy((&pmib->dot11RFEntry)->pwrdiffHT40_2S, value, MAX_CHAN_NUM);

#ifdef WLAN_DUALBAND_CONCURRENT
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_HT20, (void *)value, sizeof(value));
		else
#endif
		mib_get_s(MIB_HW_TX_POWER_HT20, (void *)value, sizeof(value));
		memcpy((&pmib->dot11RFEntry)->pwrdiffHT20, value, MAX_CHAN_NUM);

#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_OFDM, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_TX_POWER_DIFF_OFDM, (void *)value, sizeof(value));
		memcpy((&pmib->dot11RFEntry)->pwrdiffOFDM, value, MAX_CHAN_NUM);

#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_11N_TSSI1 , (void *) value, sizeof(value));
	else // wlan0
#endif
		mib_get_s(MIB_HW_11N_TSSI1, (void *)value, sizeof(value));

	if(value[0] != 0)
		(&pmib->dot11RFEntry)->tssi1 = value[0];

#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_11N_TSSI2, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_11N_TSSI2, (void *)value, sizeof(value));

	if(value[0] != 0)
		(&pmib->dot11RFEntry)->tssi2 = value[0];

#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_11N_THER, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_11N_THER, (void *)&value, sizeof(value));

	if(value[0] != 0){
		(&pmib->dot11RFEntry)->ther = value[0];
		(&pmib->dot11RFEntry)->thermal[0] = value[0];
	}
#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_11N_THER2, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_11N_THER2, (void *)&value, sizeof(value));

	if(value[0] != 0)
		(&pmib->dot11RFEntry)->thermal[1] = value[0];

#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_11N_THER3, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_11N_THER3, (void *)&value, sizeof(value));

	if(value[0] != 0)
		(&pmib->dot11RFEntry)->thermal[2] = value[0];

#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_11N_THER4, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_11N_THER4, (void *)&value, sizeof(value));

	if(value[0] != 0)
		(&pmib->dot11RFEntry)->thermal[3] = value[0];

#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get(MIB_HW_WLAN1_11N_TSSI_ENABLE, (void *)value);
	else
#endif
		mib_get(MIB_HW_11N_TSSI_ENABLE, (void *)&value);
		(&pmib->dot11RFEntry)->tssi_enable = value[0];

#if defined (WLAN_DUALBAND_CONCURRENT)
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_RF_XCAP, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_RF_XCAP, (void *)value, sizeof(value));
		(&pmib->dot11RFEntry)->xcap = value[0];
}
#else

static int setupWlanHWRegSettings(char *ifname)
{
	unsigned char phyband = 0;
	unsigned char value[34];
	char parm[64];
	unsigned char mode=0;
	int mib_index = 0;
	int intVal = 0, status = 0;

	mib_index = _get_wlan_hw_mib_index(ifname);

	get_WlanHWMIB(mib_index, 11N_TSSI_ENABLE, value);

	if(value[0]){
		mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));
		if ( mib_get_s( MIB_TX_POWER, (void *)&mode, sizeof(mode)) == 0) {
			printf("Get MIB_TX_POWER error!\n");
			status=-1;
		}
		intVal = getTxPowerScale(phyband, mode);
		snprintf(parm, sizeof(parm), "all=%d", intVal*(-2));
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_reg_pg", parm);
	}
	return status;
}
int setupWlanHWSetting(char *ifname)
{
	int status=0;
	unsigned char value[34];
	char parm[64];
	char mode=0;
	int mib_index = 0;
#if defined(WLAN_DUALBAND_CONCURRENT)
    unsigned char phyband = 0;
#endif

#if defined(WLAN_DUALBAND_CONCURRENT)
	mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));
#if defined(TRIBAND_SUPPORT)
	if(strcmp(ifname ,WLANIF[2]) == 0) // wlan2
		mib_index = 2;
	else
#endif
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_index = 1;
#endif


	if ( mib_get_s( MIB_TX_POWER, (void *)&mode, sizeof(mode)) == 0) {
   			printf("Get MIB_TX_POWER error!\n");
   			status=-1;
	}

	get_WlanHWMIB(mib_index, TX_POWER_CCK_A, value);
#if 0
	#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_CCK_A, (void *)value, sizeof(value));
	else // wlan0
	#endif
		mib_get_s(MIB_HW_TX_POWER_CCK_A, (void *)value, sizeof(value));
#endif


	if(value[0] != 0) {
#ifdef CONFIG_ENABLE_EFUSE
		//disable efuse
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "use_efuse=0");
#endif
		status|=iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevelCCK_A", value, mode);
	}
#ifdef CONFIG_ENABLE_EFUSE
	else {
		//enable efuse
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "use_efuse=1");
	}
#endif

	get_WlanHWMIB(mib_index, TX_POWER_CCK_B, value);
#if 0
	#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_CCK_B, (void *)value, sizeof(value));
	else // wlan0
	#endif
		mib_get_s(MIB_HW_TX_POWER_CCK_B, (void *)value, sizeof(value));
#endif

	if(value[0] != 0)
		status|=iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevelCCK_B", value, mode);

	get_WlanHWMIB(mib_index, TX_POWER_TSSI_CCK_A, value);
#if 0
#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_TSSI_CCK_A, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_TX_POWER_TSSI_CCK_A, (void *)value, sizeof(value));
#endif

	//if(value[0] != 0)
	{
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel_TSSICCK_A", value, mode);
	}

	get_WlanHWMIB(mib_index, TX_POWER_TSSI_CCK_B, value);
#if 0
#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_TSSI_CCK_B, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_TX_POWER_TSSI_CCK_B, (void *)value, sizeof(value));
#endif

	//if(value[0] != 0)
	{
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel_TSSICCK_B", value, mode);
	}

#if defined(WLAN_8814_SERIES_SUPPORT) || defined(CONFIG_WLAN_HAL_8198F)

	get_WlanHWMIB(mib_index, TX_POWER_CCK_C, value);
#if 0
#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_CCK_C, (void *)value, sizeof(value));
	else // wlan0
#endif
		mib_get_s(MIB_HW_TX_POWER_CCK_C, (void *)value, sizeof(value));
#endif

	if(value[0] != 0)
		status|=iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevelCCK_C", value, mode);

	get_WlanHWMIB(mib_index, TX_POWER_CCK_D, value);
#if 0
#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_CCK_D, (void *)value, sizeof(value));
	else // wlan0
#endif
		mib_get_s(MIB_HW_TX_POWER_CCK_D, (void *)value, sizeof(value));
#endif

	if(value[0] != 0)
		status|=iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevelCCK_D", value, mode);

	get_WlanHWMIB(mib_index, TX_POWER_TSSI_CCK_C, value);
#if 0
#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_TSSI_CCK_C, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_TX_POWER_TSSI_CCK_C, (void *)value, sizeof(value));
#endif

	//if(value[0] != 0)
	{
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel_TSSICCK_C", value, mode);
	}

	get_WlanHWMIB(mib_index, TX_POWER_TSSI_CCK_D, value);
#if 0
#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_TSSI_CCK_D, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_TX_POWER_TSSI_CCK_D, (void *)value, sizeof(value));
#endif

	//if(value[0] != 0)
	{
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel_TSSICCK_D", value, mode);
	}
#endif


	get_WlanHWMIB(mib_index, TX_POWER_HT40_1S_A, value);
#if 0
	#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_HT40_1S_A, (void *)value, sizeof(value));
	else // wlan0
	#endif
		mib_get_s(MIB_HW_TX_POWER_HT40_1S_A, (void *)value, sizeof(value));
#endif

	if(value[0] != 0)
		status|=iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevelHT40_1S_A", value, mode);

	get_WlanHWMIB(mib_index, TX_POWER_HT40_1S_B, value);
#if 0
	#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_HT40_1S_B, (void *)value, sizeof(value));
	else // wlan0
	#endif
		mib_get_s(MIB_HW_TX_POWER_HT40_1S_B, (void *)value, sizeof(value));
#endif

	if(value[0] != 0)
		status|=iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevelHT40_1S_B", value, mode);

	get_WlanHWMIB(mib_index, TX_POWER_TSSI_HT40_1S_A, value);
#if 0
#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_TSSI_HT40_1S_A, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_TX_POWER_TSSI_HT40_1S_A, (void *)value, sizeof(value));
#endif
	//if(value[0] != 0)
	{
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel_TSSIHT40_1S_A", value, mode);
	}

	get_WlanHWMIB(mib_index, TX_POWER_TSSI_HT40_1S_B, value);
#if 0
#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_TSSI_HT40_1S_B, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_TX_POWER_TSSI_HT40_1S_B, (void *)value, sizeof(value));
#endif
	//if(value[0] != 0)
	{
		iwpriv_cmd(IWPRIV_HS |IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel_TSSIHT40_1S_B", value, mode);
	}

#if defined(WLAN_8814_SERIES_SUPPORT) || defined(CONFIG_WLAN_HAL_8198F)
	get_WlanHWMIB(mib_index, TX_POWER_HT40_1S_C, value);
#if 0
#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_HT40_1S_C, (void *)value, sizeof(value));
	else // wlan0
#endif
		mib_get_s(MIB_HW_TX_POWER_HT40_1S_C, (void *)value, sizeof(value));
#endif

	if(value[0] != 0)
		status|=iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevelHT40_1S_C", value, mode);

	get_WlanHWMIB(mib_index, TX_POWER_HT40_1S_D, value);
#if 0
#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_HT40_1S_D, (void *)value, sizeof(value));
	else // wlan0
#endif
		mib_get_s(MIB_HW_TX_POWER_HT40_1S_D, (void *)value, sizeof(value));
#endif

	if(value[0] != 0)
		status|=iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevelHT40_1S_D", value, mode);

	get_WlanHWMIB(mib_index, TX_POWER_TSSI_HT40_1S_C, value);
#if 0
#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_TSSI_HT40_1S_C, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_TX_POWER_TSSI_HT40_1S_C, (void *)value, sizeof(value));
#endif
	//if(value[0] != 0)
	{
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel_TSSIHT40_1S_C", value, mode);
	}

	get_WlanHWMIB(mib_index, TX_POWER_TSSI_HT40_1S_D, value);
#if 0
#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_TSSI_HT40_1S_D, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_TX_POWER_TSSI_HT40_1S_D, (void *)value, sizeof(value));
#endif
	//if(value[0] != 0)
	{
		iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G|IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel_TSSIHT40_1S_D", value, mode);
	}

#endif


//#if defined(CONFIG_WLAN_HAL_8192EE) || defined(WLAN_DUALBAND_CONCURRENT)
#if defined(TRIBAND_SUPPORT)
    if(phyband == PHYBAND_2G)
#elif defined(CONFIG_WLAN0_5G_WLAN1_2G)
	if(wlan_idx==1)
#elif defined(CONFIG_WLAN_HAL_8198F) && defined(CONFIG_BAND_5G_ON_WLAN0)
	if(wlan_idx==1)
#elif defined(WLAN_DUALBAND_CONCURRENT)
	if(phyband == PHYBAND_2G)
#endif
{
#if defined(CONFIG_WLAN_HAL_8198F)
	get_WlanHWMIB(mib_index, TX_POWER_DIFF_20BW1S_OFDM1T_A, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_20BW1S_OFDM1T_A, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_20BW1S_OFDM1T_A", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW2S_20BW2S_A, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW2S_20BW2S_A, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW2S_20BW2S_A", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW3S_20BW3S_A, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW3S_20BW3S_A, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW3S_20BW3S_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW3S_20BW3S_A", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW4S_20BW4S_A, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW4S_20BW4S_A, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW4S_20BW4S_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW4S_20BW4S_A", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_20BW1S_OFDM1T_B, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_20BW1S_OFDM1T_B, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_20BW1S_OFDM1T_B", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW2S_20BW2S_B, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW2S_20BW2S_B, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW2S_20BW2S_B", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW3S_20BW3S_B, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW3S_20BW3S_B, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW3S_20BW3S_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW3S_20BW3S_B", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW4S_20BW4S_B, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW4S_20BW4S_B, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW4S_20BW4S_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW4S_20BW4S_B", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_20BW1S_OFDM1T_C, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_20BW1S_OFDM1T_C, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_20BW1S_OFDM1T_C", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW2S_20BW2S_C, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW2S_20BW2S_C, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW2S_20BW2S_C", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW3S_20BW3S_C, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW3S_20BW3S_C, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW3S_20BW3S_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW3S_20BW3S_C", value);
	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW4S_20BW4S_C, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW4S_20BW4S_C, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW4S_20BW4S_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW4S_20BW4S_C", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_20BW1S_OFDM1T_D, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_20BW1S_OFDM1T_D, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_20BW1S_OFDM1T_D", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW2S_20BW2S_D, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW2S_20BW2S_D, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW2S_20BW2S_D", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW3S_20BW3S_D, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW3S_20BW3S_D, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW3S_20BW3S_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW3S_20BW3S_D", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW4S_20BW4S_D, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW4S_20BW4S_D, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW4S_20BW4S_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW4S_20BW4S_D", value);

#else
	get_WlanHWMIB(mib_index, TX_POWER_HT40_2S, value);
#if 0
	#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_HT40_2S, (void *)value, sizeof(value));
	else
	#endif
		mib_get_s(MIB_HW_TX_POWER_HT40_2S, (void *)value, sizeof(value));
#endif

	// may be 0 for power difference
	//if(value[0] != 0) {
		status|=iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, getWlanIfName(), "set_mib", "pwrdiffHT40_2S", value);
	//}

	get_WlanHWMIB(mib_index, TX_POWER_HT20, value);
#if 0
	#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_HT20, (void *)value, sizeof(value));
	else
	#endif
		mib_get_s(MIB_HW_TX_POWER_HT20, (void *)value, sizeof(value));
#endif

	// may be 0 for power difference
	//if(value[0] != 0) {
		status|=iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, getWlanIfName(), "set_mib", "pwrdiffHT20", value);
	//}

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_OFDM, value);
#if 0
	#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_OFDM, (void *)value, sizeof(value));
	else
	#endif
		mib_get_s(MIB_HW_TX_POWER_DIFF_OFDM, (void *)value, sizeof(value));
#endif

	// may be 0 for power difference
	//if(value[0] != 0) {
		status|=iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, getWlanIfName(), "set_mib", "pwrdiffOFDM", value);
	//}
#endif
}
//#endif
#if defined(CONFIG_WLAN_HAL_8198F) && !defined(WLAN_DUALBAND_CONCURRENT)
	get_WlanHWMIB(mib_index, TX_POWER_DIFF_20BW1S_OFDM1T_A, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_20BW1S_OFDM1T_A, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_20BW1S_OFDM1T_A", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW2S_20BW2S_A, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW2S_20BW2S_A, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW2S_20BW2S_A", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW3S_20BW3S_A, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW3S_20BW3S_A, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW3S_20BW3S_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW3S_20BW3S_A", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW4S_20BW4S_A, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW4S_20BW4S_A, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW4S_20BW4S_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW4S_20BW4S_A", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_20BW1S_OFDM1T_B, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_20BW1S_OFDM1T_B, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_20BW1S_OFDM1T_B", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW2S_20BW2S_B, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW2S_20BW2S_B, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW2S_20BW2S_B", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW3S_20BW3S_B, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW3S_20BW3S_B, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW3S_20BW3S_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW3S_20BW3S_B", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW4S_20BW4S_B, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW4S_20BW4S_B, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW4S_20BW4S_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW4S_20BW4S_B", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_20BW1S_OFDM1T_C, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_20BW1S_OFDM1T_C, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_20BW1S_OFDM1T_C", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW2S_20BW2S_C, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW2S_20BW2S_C, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW2S_20BW2S_C", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW3S_20BW3S_C, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW3S_20BW3S_C, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW3S_20BW3S_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW3S_20BW3S_C", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW4S_20BW4S_C, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW4S_20BW4S_C, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW4S_20BW4S_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW4S_20BW4S_C", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_20BW1S_OFDM1T_D, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_20BW1S_OFDM1T_D, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_20BW1S_OFDM1T_D", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW2S_20BW2S_D, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW2S_20BW2S_D, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW2S_20BW2S_D", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW3S_20BW3S_D, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW3S_20BW3S_D, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW3S_20BW3S_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW3S_20BW3S_D", value);

	get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW4S_20BW4S_D, value);
#if 0
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW4S_20BW4S_D, (void *)value, sizeof(value));
	else
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW4S_20BW4S_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW4S_20BW4S_D", value);

#endif

#if defined(WLAN_8814_SERIES_SUPPORT) || (defined(CONFIG_RTL_8812_SUPPORT) && !defined(WLAN_DUALBAND_CONCURRENT))
#if defined(WLAN_DUALBAND_CONCURRENT)
	if(phyband == PHYBAND_2G)
#endif
	{
		get_WlanHWMIB(mib_index, TX_POWER_DIFF_20BW1S_OFDM1T_A, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_20BW1S_OFDM1T_A, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_20BW1S_OFDM1T_A", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW2S_20BW2S_A, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW2S_20BW2S_A, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW2S_20BW2S_A", value);

#if defined(WLAN_8814_SERIES_SUPPORT)
		get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW3S_20BW3S_A, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW3S_20BW3S_A, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW3S_20BW3S_A", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW4S_20BW4S_A, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW4S_20BW4S_A, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW4S_20BW4S_A", value);
#endif

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_20BW1S_OFDM1T_B, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_20BW1S_OFDM1T_B, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_20BW1S_OFDM1T_B", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW2S_20BW2S_B, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW2S_20BW2S_B, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW2S_20BW2S_B", value);

#if defined(WLAN_8814_SERIES_SUPPORT)
		get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW3S_20BW3S_B, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW3S_20BW3S_B, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW3S_20BW3S_B", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW4S_20BW4S_B, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW4S_20BW4S_B, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW4S_20BW4S_B", value);
#endif

#if defined(WLAN_8814_SERIES_SUPPORT)
		get_WlanHWMIB(mib_index, TX_POWER_DIFF_20BW1S_OFDM1T_C, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_20BW1S_OFDM1T_C, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_20BW1S_OFDM1T_C", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW2S_20BW2S_C, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW2S_20BW2S_C, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW2S_20BW2S_C", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW3S_20BW3S_C, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW3S_20BW3S_C, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW3S_20BW3S_C", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW4S_20BW4S_C, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW4S_20BW4S_C, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW4S_20BW4S_C", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_20BW1S_OFDM1T_D, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_20BW1S_OFDM1T_D, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_20BW1S_OFDM1T_D", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW2S_20BW2S_D, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW2S_20BW2S_D, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW2S_20BW2S_D", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW3S_20BW3S_D, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW3S_20BW3S_D, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW3S_20BW3S_D", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_40BW4S_20BW4S_D, value);
		//mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_40BW4S_20BW4S_D, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, ifname, "set_mib", "pwrdiff_40BW4S_20BW4S_D", value);
#endif
	}
#endif

	get_WlanHWMIB(mib_index, 11N_TSSI_ENABLE, value);
#if 0
	#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_11N_TSSI_ENABLE , (void *) value, sizeof(value));
	else // wlan0
	#endif
		mib_get_s(MIB_HW_11N_TSSI_ENABLE, (void *)value, sizeof(value));
#endif

	//if(value[0] != 0)
		status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "tssi_enable", value[0]);

	get_WlanHWMIB(mib_index, 11N_TSSI1, value);
#if 0
	#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_11N_TSSI1 , (void *) value, sizeof(value));
	else // wlan0
	#endif
		mib_get_s(MIB_HW_11N_TSSI1, (void *)value, sizeof(value));
#endif

	if(value[0] != 0)
		status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "tssi1", value[0]);

	get_WlanHWMIB(mib_index, 11N_TSSI2, value);
#if 0
	#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_11N_TSSI2, (void *)value, sizeof(value));
	else
	#endif
		mib_get_s(MIB_HW_11N_TSSI2, (void *)value, sizeof(value));
#endif

	if(value[0] != 0)
		status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "tssi2", value[0]);

	get_WlanHWMIB(mib_index, 11N_THER, value);
#if 0
	#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_11N_THER, (void *)value, sizeof(value));
	else
	#endif
		mib_get_s(MIB_HW_11N_THER, (void *)value, sizeof(value));
#endif

	if(value[0] != 0) {
		snprintf(parm, sizeof(parm), "ther=%d", value[0]);
		status|=va_cmd(IWPRIV, 3, 1, (char *) getWlanIfName(), "set_mib", parm);

		snprintf(parm, sizeof(parm), "thermal1=%d", value[0]);
		status|=va_cmd(IWPRIV, 3, 1, (char *) getWlanIfName(), "set_mib", parm);
	}

	get_WlanHWMIB(mib_index, 11N_THER2, value);
#if 0
	#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_11N_THER2, (void *)value, sizeof(value));
	else
	#endif
		mib_get_s(MIB_HW_11N_THER2, (void *)value, sizeof(value));
#endif

	if(value[0] != 0) {
		snprintf(parm, sizeof(parm), "thermal2=%d", value[0]);
		status|=va_cmd(IWPRIV, 3, 1, (char *) getWlanIfName(), "set_mib", parm);
	}

	get_WlanHWMIB(mib_index, 11N_THER3, value);
#if 0
	#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_11N_THER3, (void *)value, sizeof(value));
	else
	#endif
		mib_get_s(MIB_HW_11N_THER3, (void *)value, sizeof(value));
#endif

	if(value[0] != 0) {
		snprintf(parm, sizeof(parm), "thermal3=%d", value[0]);
		status|=va_cmd(IWPRIV, 3, 1, (char *) getWlanIfName(), "set_mib", parm);
	}

	get_WlanHWMIB(mib_index, 11N_THER4, value);
#if 0
	#ifdef WLAN_DUALBAND_CONCURRENT
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_11N_THER4, (void *)value, sizeof(value));
	else
	#endif
		mib_get_s(MIB_HW_11N_THER4, (void *)value, sizeof(value));
#endif

	if(value[0] != 0) {
		snprintf(parm, sizeof(parm), "thermal4=%d", value[0]);
		status|=va_cmd(IWPRIV, 3, 1, (char *) getWlanIfName(), "set_mib", parm);
	}

	get_WlanHWMIB(mib_index, RF_XCAP, value);
#if 0
#if defined (WLAN_DUALBAND_CONCURRENT)
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_get_s(MIB_HW_WLAN1_RF_XCAP, (void *)value, sizeof(value));
	else
#endif
		mib_get_s(MIB_HW_RF_XCAP, (void *)value, sizeof(value));
#endif

	status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "xcap", value[0]);

#if defined (WLAN_DUALBAND_CONCURRENT)
        if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
                mib_get_s(MIB_HW_WLAN1_COUNTRYCODE, (void *)value, sizeof(value));
        else
#endif
                mib_get_s(MIB_HW_WLAN0_COUNTRYCODE, (void *)value, sizeof(value));

        status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "countrycode", value[0]);

#if defined (WLAN_DUALBAND_CONCURRENT)
        if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
                mib_get_s(MIB_HW_WLAN1_COUNTRYSTR, (void *)value, sizeof(value));
        else
#endif
                mib_get_s(MIB_HW_WLAN0_COUNTRYSTR, (void *)value, sizeof(value));
        if(value[0] != 0) {
                snprintf(parm, sizeof(parm), "countrystr=%s", value);
                status|=va_cmd(IWPRIV, 3, 1, (char *) getWlanIfName(), "set_mib", parm);
        }

	int tr_mode[1] = {-1};
	get_WlanHWMIB(mib_index, MIMO_TR_MODE, tr_mode);

	// (tr_mode == -1) means use driver default value.
	if(tr_mode[0] >= 0) {
		snprintf(parm, sizeof(parm), "MIMO_TR_mode=%d", tr_mode[0]);
		status|=va_cmd(IWPRIV, 3, 1, (char *) getWlanIfName(), "set_mib", parm);
	}
	return status;


}
#endif
#ifdef WLAN_FAST_INIT
void setup8812Wlan(char *ifname, struct wifi_mib *pmib)
{
	unsigned char buf1[1024];
	struct Dot11RFEntry *rf_entry;
#if defined(WLAN_DUALBAND_CONCURRENT)
#if defined(WLAN0_5G_SUPPORT)
	if(!strcmp(ifname , WLANIF[0])) //wlan0:5G, wlan1:2G
#else
	if(!strcmp(ifname , WLANIF[1])) //wlan0:2G, wlan1:5G
#endif
#endif
	{
		rf_entry = &pmib->dot11RFEntry;

		mib_get_s(MIB_HW_TX_POWER_5G_HT40_1S_A, (void*) buf1, sizeof(buf1));
		memcpy(rf_entry->pwrlevel5GHT40_1S_A, (unsigned char*) buf1, MAX_5G_CHANNEL_NUM);
		mib_get_s(MIB_HW_TX_POWER_5G_HT40_1S_B, (void*) buf1, sizeof(buf1));
		memcpy(rf_entry->pwrlevel5GHT40_1S_B, (unsigned char*) buf1, MAX_5G_CHANNEL_NUM);
		// 5G
		mib_get_s(MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_A, (void *)buf1, sizeof(buf1));
		assign_diff_AC(rf_entry->pwrdiff_5G_20BW1S_OFDM1T_A, (unsigned char*) buf1);
		mib_get_s(MIB_HW_TX_POWER_DIFF_5G_40BW2S_20BW2S_A, (void *)buf1, sizeof(buf1));
		assign_diff_AC(rf_entry->pwrdiff_5G_40BW2S_20BW2S_A, (unsigned char*)buf1);
		mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW1S_160BW1S_A, (void *)buf1, sizeof(buf1));
		assign_diff_AC(rf_entry->pwrdiff_5G_80BW1S_160BW1S_A, (unsigned char*)buf1);
		mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW2S_160BW2S_A, (void *)buf1, sizeof(buf1));
		assign_diff_AC(rf_entry->pwrdiff_5G_80BW2S_160BW2S_A, (unsigned char*)buf1);

		mib_get_s(MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_B, (void *)buf1, sizeof(buf1));
		assign_diff_AC(rf_entry->pwrdiff_5G_20BW1S_OFDM1T_B, (unsigned char*)buf1);
		mib_get_s(MIB_HW_TX_POWER_DIFF_5G_40BW2S_20BW2S_B, (void *)buf1, sizeof(buf1));
		assign_diff_AC(rf_entry->pwrdiff_5G_40BW2S_20BW2S_B, (unsigned char*)buf1);
		mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW1S_160BW1S_B, (void *)buf1, sizeof(buf1));
		assign_diff_AC(rf_entry->pwrdiff_5G_80BW1S_160BW1S_B, (unsigned char*)buf1);
		mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW2S_160BW2S_B, (void *)buf1, sizeof(buf1));
		assign_diff_AC(rf_entry->pwrdiff_5G_80BW2S_160BW2S_B, (unsigned char*)buf1);

		// 2G
		mib_get_s(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_A, (void *)buf1, sizeof(buf1));
		memcpy(rf_entry->pwrdiff_20BW1S_OFDM1T_A, buf1, MAX_CHAN_NUM);
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_A, (void *)buf1, sizeof(buf1));
		memcpy(rf_entry->pwrdiff_40BW2S_20BW2S_A, buf1, MAX_CHAN_NUM);

		mib_get_s(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_B, (void *)buf1, sizeof(buf1));
		memcpy(rf_entry->pwrdiff_20BW1S_OFDM1T_B, buf1, MAX_CHAN_NUM);
		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_B, (void *)buf1, sizeof(buf1));
		memcpy(rf_entry->pwrdiff_40BW2S_20BW2S_B, buf1, MAX_CHAN_NUM);
	}
}
#else
int setup8812Wlan(	char *ifname)
{
	int i = 0 ;
	int status = 0 ;
	unsigned char value[196];
	char parm[1024];
	int intVal;
	char mode=0;
	int mib_index = 0;
#if defined(WLAN_DUALBAND_CONCURRENT)
    unsigned char phyband=0;
    mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));
    if(phyband == PHYBAND_2G){
        return status;
    }
#endif
#if defined(WLAN_DUALBAND_CONCURRENT)
#if defined(TRIBAND_SUPPORT)
	if(strcmp(ifname ,WLANIF[2]) == 0) // wlan2
		mib_index = 2;
	else
#endif
	if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
		mib_index = 1;
#endif
		unsigned char pMib[178];


		if ( mib_get_s( MIB_TX_POWER, (void *)&mode, sizeof(mode)) == 0) {
   			printf("Get MIB_TX_POWER error!\n");
   			status=-1;
		}

		get_WlanHWMIB(mib_index, TX_POWER_5G_HT40_1S_A, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_5G_HT40_1S_A, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_5G_HT40_1S_A, (void *)value, sizeof(value));
#endif
		//if(value[0] != 0)
		{
			iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel5GHT40_1S_A", value, mode);
		}

		get_WlanHWMIB(mib_index, TX_POWER_5G_HT40_1S_B, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_5G_HT40_1S_B, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_5G_HT40_1S_B, (void *)value, sizeof(value));
#endif
		//if(value[0] != 0)
		{
			iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel5GHT40_1S_B", value , mode);
		}

		get_WlanHWMIB(mib_index, TX_POWER_TSSI_5G_HT40_1S_A, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_TSSI_5G_HT40_1S_A, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_TSSI_5G_HT40_1S_A, (void *)value, sizeof(value));
#endif
		//if(value[0] != 0)
		{
			iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel_TSSI5GHT40_1S_A", value , mode);
		}

		get_WlanHWMIB(mib_index, TX_POWER_TSSI_5G_HT40_1S_B, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_TSSI_5G_HT40_1S_B, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_TSSI_5G_HT40_1S_B, (void *)value, sizeof(value));
#endif
		//if(value[0] != 0)
		{
			iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel_TSSI5GHT40_1S_B", value , mode);
		}

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_20BW1S_OFDM1T_A, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_20BW1S_OFDM1T_A, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_20BW1S_OFDM1T_A", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_40BW2S_20BW2S_A, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_40BW2S_20BW2S_A, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_40BW2S_20BW2S_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_40BW2S_20BW2S_A", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW1S_160BW1S_A, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW1S_160BW1S_A, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW1S_160BW1S_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW1S_160BW1S_A", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW2S_160BW2S_A, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW2S_160BW2S_A, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW2S_160BW2S_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW2S_160BW2S_A", value);

#if defined(WLAN_8814_SERIES_SUPPORT)
		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_40BW3S_20BW3S_A, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_40BW3S_20BW3S_A, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_40BW3S_20BW3S_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_40BW3S_20BW3S_A", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_40BW4S_20BW4S_A, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_40BW4S_20BW4S_A, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_40BW4S_20BW4S_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_40BW4S_20BW4S_A", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW3S_160BW3S_A, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW3S_160BW3S_A, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW3S_160BW3S_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW3S_160BW3S_A", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW4S_160BW4S_A, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW4S_160BW4S_A, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW4S_160BW4S_A, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW4S_160BW4S_A", value);
#endif

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_20BW1S_OFDM1T_B, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_20BW1S_OFDM1T_B, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_20BW1S_OFDM1T_B", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_40BW2S_20BW2S_B, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_40BW2S_20BW2S_B, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_40BW2S_20BW2S_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_40BW2S_20BW2S_B", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW1S_160BW1S_B, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW1S_160BW1S_B, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW1S_160BW1S_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW1S_160BW1S_B", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW2S_160BW2S_B, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW2S_160BW2S_B, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW2S_160BW2S_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW2S_160BW2S_B", value);

#if defined(WLAN_8814_SERIES_SUPPORT)
		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_40BW3S_20BW3S_B, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_40BW3S_20BW3S_B, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_40BW3S_20BW3S_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_40BW3S_20BW3S_B", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_40BW4S_20BW4S_B, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_40BW4S_20BW4S_B, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_40BW4S_20BW4S_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_40BW4S_20BW4S_B", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW3S_160BW3S_B, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW3S_160BW3S_B, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW3S_160BW3S_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW3S_160BW3S_B", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW4S_160BW4S_B, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW4S_160BW4S_B, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW4S_160BW4S_B, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW4S_160BW4S_B", value);
#endif

#if 0
		// 2G
		mib_get_s(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_A, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, getWlanIfName(), "set_mib", "pwrdiff_20BW1S_OFDM1T_A", value);

		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_A, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, getWlanIfName(), "set_mib", "pwrdiff_40BW2S_20BW2S_A", value);

		mib_get_s(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_B, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, getWlanIfName(), "set_mib", "pwrdiff_20BW1S_OFDM1T_B", value);

		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_B, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, getWlanIfName(), "set_mib", "pwrdiff_40BW2S_20BW2S_B", value);
#endif

#if defined(WLAN_8814_SERIES_SUPPORT)
		get_WlanHWMIB(mib_index, TX_POWER_5G_HT40_1S_C, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_5G_HT40_1S_C, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_5G_HT40_1S_C, (void *)value, sizeof(value));
#endif
		//if(value[0] != 0)
		{
			iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel5GHT40_1S_C", value, mode);
		}

		get_WlanHWMIB(mib_index, TX_POWER_5G_HT40_1S_D, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_5G_HT40_1S_D, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_5G_HT40_1S_D, (void *)value, sizeof(value));
#endif
		//if(value[0] != 0)
		{
			iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel5GHT40_1S_D", value , mode);
		}

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_20BW1S_OFDM1T_C, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_20BW1S_OFDM1T_C, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_20BW1S_OFDM1T_C", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_40BW2S_20BW2S_C, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_40BW2S_20BW2S_C, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_40BW2S_20BW2S_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_40BW2S_20BW2S_C", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW1S_160BW1S_C, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW1S_160BW1S_C, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW1S_160BW1S_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW1S_160BW1S_C", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW2S_160BW2S_C, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW2S_160BW2S_C, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW2S_160BW2S_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW2S_160BW2S_C", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_40BW3S_20BW3S_C, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_40BW3S_20BW3S_C, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_40BW3S_20BW3S_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_40BW3S_20BW3S_C", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_40BW4S_20BW4S_C, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_40BW4S_20BW4S_C, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_40BW4S_20BW4S_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_40BW4S_20BW4S_C", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW3S_160BW3S_C, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW3S_160BW3S_C, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW3S_160BW3S_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW3S_160BW3S_C", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW4S_160BW4S_C, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW4S_160BW4S_C, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW4S_160BW4S_C, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW4S_160BW4S_C", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_20BW1S_OFDM1T_D, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_20BW1S_OFDM1T_D, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_20BW1S_OFDM1T_D", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_40BW2S_20BW2S_D, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_40BW2S_20BW2S_D, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_40BW2S_20BW2S_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_40BW2S_20BW2S_D", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW1S_160BW1S_D, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW1S_160BW1S_D, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW1S_160BW1S_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW1S_160BW1S_D", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW2S_160BW2S_D, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW2S_160BW2S_D, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW2S_160BW2S_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW2S_160BW2S_D", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_40BW3S_20BW3S_D, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_40BW3S_20BW3S_D, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_40BW3S_20BW3S_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_40BW3S_20BW3S_D", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_40BW4S_20BW4S_D, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_40BW4S_20BW4S_D, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_40BW4S_20BW4S_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_40BW4S_20BW4S_D", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW3S_160BW3S_D, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW3S_160BW3S_D, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW3S_160BW3S_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW3S_160BW3S_D", value);

		get_WlanHWMIB(mib_index, TX_POWER_DIFF_5G_80BW4S_160BW4S_D, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_TX_POWER_DIFF_5G_80BW4S_160BW4S_D, (void *)value, sizeof(value));
		else
			mib_get_s(MIB_HW_TX_POWER_DIFF_5G_80BW4S_160BW4S_D, (void *)value, sizeof(value));
#endif
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS, getWlanIfName(), "set_mib", "pwrdiff_5G_80BW4S_160BW4S_D", value);

#if 0
		// 2G
		mib_get_s(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_C, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, getWlanIfName(), "set_mib", "pwrdiff_20BW1S_OFDM1T_C", value);

		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_C, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, getWlanIfName(), "set_mib", "pwrdiff_40BW2S_20BW2S_C", value);

		mib_get_s(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_D, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, getWlanIfName(), "set_mib", "pwrdiff_20BW1S_OFDM1T_D", value);

		mib_get_s(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_D, (void *)value, sizeof(value));
		if(value[0] != 0)
			iwpriv_cmd(IWPRIV_HS|IWPRIV_HW2G, getWlanIfName(), "set_mib", "pwrdiff_40BW2S_20BW2S_D", value);
#endif

#endif
		get_WlanHWMIB(mib_index, TX_POWER_TSSI_5G_HT40_1S_A, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get(MIB_HW_WLAN1_TX_POWER_TSSI_5G_HT40_1S_A, (void *)value);
		else
			mib_get(MIB_HW_TX_POWER_TSSI_5G_HT40_1S_A, (void *)value);
#endif
		//if(value[0] != 0)
		{
			iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel_TSSI5GHT40_1S_A", value, mode);
		}

		get_WlanHWMIB(mib_index, TX_POWER_TSSI_5G_HT40_1S_B, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get(MIB_HW_WLAN1_TX_POWER_TSSI_5G_HT40_1S_B, (void *)value);
		else
			mib_get(MIB_HW_TX_POWER_TSSI_5G_HT40_1S_B, (void *)value);
#endif
		//if(value[0] != 0)
		{
			iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel_TSSI5GHT40_1S_B", value, mode);
		}

		get_WlanHWMIB(mib_index, TX_POWER_TSSI_5G_HT40_1S_C, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get(MIB_HW_WLAN1_TX_POWER_TSSI_5G_HT40_1S_C, (void *)value);
		else
			mib_get(MIB_HW_TX_POWER_TSSI_5G_HT40_1S_C, (void *)value);
#endif
		//if(value[0] != 0)
		{
			iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel_TSSI5GHT40_1S_C", value, mode);
		}

		get_WlanHWMIB(mib_index, TX_POWER_TSSI_5G_HT40_1S_D, value);
#if 0
		if(strcmp(ifname ,WLANIF[1]) == 0) // wlan1
			mib_get(MIB_HW_WLAN1_TX_POWER_TSSI_5G_HT40_1S_D, (void *)value);
		else
			mib_get(MIB_HW_TX_POWER_TSSI_5G_HT40_1S_D, (void *)value);
#endif
		//if(value[0] != 0)
		{
			iwpriv_cmd(IWPRIV_HS | IWPRIV_TXPOWER, getWlanIfName(), "set_mib", "pwrlevel_TSSI5GHT40_1S_D", value, mode);
		}

	return status;
}
#endif
int setupWlanDPK()
{
	int status = 0;
#define LUT_2G_LEN 64
#define PWSF_2G_LEN 3
//#define DWORD_SWAP(dword) ((dword & 0x000000ff) << 24 | (dword & 0x0000ff00) << 8 |(dword & 0x00ff0000) >> 8 | (dword & 0xff000000) >> 24)
#if 0//!defined(CONFIG_CPU_BIG_ENDIAN)
#define LUT_SWAP(_DST_,_LEN_) \
		do{ \
			{ \
				int k; \
				for (k=0; k<_LEN_; k++) { \
				    _DST_[k] = DWORD_SWAP(_DST_[k]); \
				} \
			} \
		}while(0)
#else
#define LUT_SWAP(_DST_,_LEN_) do{}while(0)
#endif

	unsigned char phyband=0;
	unsigned int lut_val[PWSF_2G_LEN][LUT_2G_LEN];
	int i=0;
	char ifname[IFNAMSIZ];
	char parm[64];
	unsigned char value[1024], bDPPathAOK=0, bDPPathBOK=0;
#if defined(CONFIG_ARCH_RTL8198F)
    unsigned char bDPPathCOK=0, bDPPathDOK=0;
#endif
#if defined(CONFIG_ARCH_RTL8198F) || defined(CONFIG_WLAN_HAL_8812FE) || defined(WLAN_8814_SERIES_SUPPORT)
    unsigned char mib_name[64];
    int mib_id;
#endif
	int len;

	mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));
	if(phyband == PHYBAND_2G){

		strcpy(ifname, getWlanIfName());

		if(mib_get_s(MIB_HW_RF_DPK_DP_PATH_A_OK, (void *)&bDPPathAOK, sizeof(bDPPathAOK))){
#ifdef CONFIG_RF_DPK_SETTING_SUPPORT			
			snprintf(parm, sizeof(parm), "bDPPathAOK=%d", bDPPathAOK);			
			va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
#endif
		}
		if(mib_get_s(MIB_HW_RF_DPK_DP_PATH_B_OK, (void *)&bDPPathBOK, sizeof(bDPPathBOK))){
#ifdef CONFIG_RF_DPK_SETTING_SUPPORT			
			snprintf(parm, sizeof(parm), "bDPPathBOK=%d", bDPPathBOK);	
			va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
#endif
		}

#if defined(CONFIG_ARCH_RTL8198F)
		if(mib_get_s(MIB_HW_RF_DPK_DP_PATH_C_OK, (void *)&bDPPathCOK, sizeof(bDPPathCOK))){
#ifdef CONFIG_RF_DPK_SETTING_SUPPORT			
			snprintf(parm, sizeof(parm), "bDPPathCOK=%d", bDPPathCOK);
			va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
#endif			
		}

		if(mib_get_s(MIB_HW_RF_DPK_DP_PATH_D_OK, (void *)&bDPPathDOK, sizeof(bDPPathDOK))){
#ifdef CONFIG_RF_DPK_SETTING_SUPPORT			
			snprintf(parm, sizeof(parm), "bDPPathDOK=%d", bDPPathDOK);
			va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
#endif
		}
#endif

		if(bDPPathAOK==1 && bDPPathBOK==1
#if defined(CONFIG_ARCH_RTL8198F)
            && bDPPathCOK==1 && bDPPathDOK==1
#endif
        )
		{
#ifndef CONFIG_RF_DPK_SETTING_SUPPORT
			printf("Warning!!Please enable DPK setting!!\n\n");
			return status;
		}
	}
#else
			len = PWSF_2G_LEN;
		
			if(mib_get_s(MIB_HW_RF_DPK_PWSF_2G_A, (void *)value, sizeof(value))){
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "pwsf_2g_a", len, value);
			}

			if(mib_get_s(MIB_HW_RF_DPK_PWSF_2G_B, (void *)value, sizeof(value))){
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "pwsf_2g_b", len, value);
			}
#if defined(CONFIG_ARCH_RTL8198F)
			if(mib_get_s(MIB_HW_RF_DPK_PWSF_2G_C, (void *)value, sizeof(value))){
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "pwsf_2g_c", len, value);
			}

			if(mib_get_s(MIB_HW_RF_DPK_PWSF_2G_D, (void *)value, sizeof(value))){
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "pwsf_2g_d", len, value);
			}
#endif

			len = LUT_2G_LEN * 4;

			memset(lut_val, '\0', sizeof(lut_val));
			if(	mib_get_s(MIB_HW_RF_DPK_LUT_2G_EVEN_A0, (void *)value, sizeof(value))){
				memcpy(lut_val[0], value, LUT_2G_LEN*4);
				LUT_SWAP(lut_val[0], LUT_2G_LEN);
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_even_a0", len, lut_val[0]);
			}
			if(	mib_get_s(MIB_HW_RF_DPK_LUT_2G_EVEN_A1, (void *)value, sizeof(value))){
				memcpy(lut_val[1], value, LUT_2G_LEN*4);
				LUT_SWAP(lut_val[1], LUT_2G_LEN);
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_even_a1", len, lut_val[1]);
			}
			if(	mib_get_s(MIB_HW_RF_DPK_LUT_2G_EVEN_A2, (void *)value, sizeof(value))){
				memcpy(lut_val[2], value, LUT_2G_LEN*4);
				LUT_SWAP(lut_val[2], LUT_2G_LEN);
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_even_a2", len, lut_val[2]);
			}
			//for(i=0; i<PWSF_2G_LEN; i++)
			//	memcpy(value+i, lut_val[i], LUT_2G_LEN * 4);
			//iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_even_a", len, value);

			memset(lut_val, '\0', sizeof(lut_val));
			if(	mib_get_s(MIB_HW_RF_DPK_LUT_2G_ODD_A0, (void *)value, sizeof(value))){
				memcpy(lut_val[0], value, LUT_2G_LEN*4);
				LUT_SWAP(lut_val[0], LUT_2G_LEN);
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_odd_a0", len, lut_val[0]);
			}
			if(	mib_get_s(MIB_HW_RF_DPK_LUT_2G_ODD_A1, (void *)value, sizeof(value))){
				memcpy(lut_val[1], value, LUT_2G_LEN*4);
				LUT_SWAP(lut_val[1], LUT_2G_LEN);
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_odd_a1", len, lut_val[1]);
			}
			if(	mib_get_s(MIB_HW_RF_DPK_LUT_2G_ODD_A2, (void *)value, sizeof(value))){
				memcpy(lut_val[2], value, LUT_2G_LEN*4);
				LUT_SWAP(lut_val[2], LUT_2G_LEN);
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_odd_a2", len, lut_val[2]);
			}
			//for(i=0; i<PWSF_2G_LEN; i++)
			//	memcpy(value+i, lut_val[i], LUT_2G_LEN * 4);
			//iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_odd_a", len, lut_val);

			memset(lut_val, '\0', sizeof(lut_val));
			if(	mib_get_s(MIB_HW_RF_DPK_LUT_2G_EVEN_B0, (void *)value, sizeof(value))){
				memcpy(lut_val[0], value, LUT_2G_LEN*4);
				LUT_SWAP(lut_val[0], LUT_2G_LEN);
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_even_b0", len, lut_val[0]);
			}
			if(	mib_get_s(MIB_HW_RF_DPK_LUT_2G_EVEN_B1, (void *)value, sizeof(value))){
				memcpy(lut_val[1], value, LUT_2G_LEN*4);
				LUT_SWAP(lut_val[1], LUT_2G_LEN);
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_even_b1", len, lut_val[1]);
			}
			if(	mib_get_s(MIB_HW_RF_DPK_LUT_2G_EVEN_B2, (void *)value, sizeof(value))){
				memcpy(lut_val[2], value, LUT_2G_LEN*4);
				LUT_SWAP(lut_val[2], LUT_2G_LEN);
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_even_b2", len, lut_val[2]);
			}
			//for(i=0; i<PWSF_2G_LEN; i++)
			//	memcpy(value+i, lut_val[i], LUT_2G_LEN * 4);
			//iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_even_b", len, lut_val);

			memset(lut_val, '\0', sizeof(lut_val));
			if(	mib_get_s(MIB_HW_RF_DPK_LUT_2G_ODD_B0, (void *)value, sizeof(value))){
				memcpy(lut_val[0], value, LUT_2G_LEN*4);
				LUT_SWAP(lut_val[0], LUT_2G_LEN);
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_odd_b0", len, lut_val[0]);
			}
			if(	mib_get_s(MIB_HW_RF_DPK_LUT_2G_ODD_B1, (void *)value, sizeof(value))){
				memcpy(lut_val[1], value, LUT_2G_LEN*4);
				LUT_SWAP(lut_val[1], LUT_2G_LEN);
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_odd_b1", len, lut_val[1]);
			}
			if(	mib_get_s(MIB_HW_RF_DPK_LUT_2G_ODD_B2, (void *)value, sizeof(value))){
				memcpy(lut_val[2], value, LUT_2G_LEN*4);
				LUT_SWAP(lut_val[2], LUT_2G_LEN);
				iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_odd_b2", len, lut_val[2]);
			}
			//for(i=0; i<PWSF_2G_LEN; i++)
			//	memcpy(value+i, lut_val[i], LUT_2G_LEN * 4);
			//iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "lut_2g_odd_b", len, lut_val);

#if defined(CONFIG_ARCH_RTL8198F)
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i < PWSF_2G_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_2G_EVEN_C0+i;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_2G_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_2G_LEN);
                    sprintf(mib_name, "lut_2g_even_c%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i < PWSF_2G_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_2G_ODD_C0+i;
                if( mib_get_s(mib_id, (void *)value, sizeof(value))){
                    memcpy(lut_val[i], value, LUT_2G_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_2G_LEN);
                    sprintf(mib_name, "lut_2g_odd_c%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }

            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i < PWSF_2G_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_2G_EVEN_D0+i;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_2G_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_2G_LEN);
                    sprintf(mib_name, "lut_2g_even_d%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i < PWSF_2G_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_2G_ODD_D0+i;
                if( mib_get_s(mib_id, (void *)value, sizeof(value))){
                    memcpy(lut_val[i], value, LUT_2G_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_2G_LEN);
                    sprintf(mib_name, "lut_2g_odd_d%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }
#endif //defined(CONFIG_ARCH_RTL8198F)      
		}		else{
#ifdef CONFIG_RF_DPK_SETTING_SUPPORT
			printf("Warning!!Not support DPK!!\n\n");
			return status;
#endif
		}		
	}

#if defined(CONFIG_WLAN_HAL_8812FE) || defined(WLAN_8814_SERIES_SUPPORT)
	if (phyband == PHYBAND_5G) {
        #define LUT_5G_LEN 16
        #define PWSF_5G_LEN 9
        unsigned int  lut_val[PWSF_5G_LEN][LUT_5G_LEN];
        unsigned char is_5g_pdk_patha_ok = 0, is_5g_pdk_pathb_ok = 0;
    #if defined(WLAN_8814_SERIES_SUPPORT)
        unsigned char is_5g_pdk_pathc_ok = 0, is_5g_pdk_pathd_ok = 0;
    #endif

    #if defined(TRIBAND_SUPPORT)
        if (wlan_idx != 0)
            goto END_5G1;
    #endif

        strcpy(ifname, getWlanIfName());
    
        if (mib_get_s(MIB_HW_RF_DPK_DP_5G_PATH_A_OK, (void *)&is_5g_pdk_patha_ok, sizeof(is_5g_pdk_patha_ok))) {
            snprintf(parm, sizeof(parm), "is_5g_pdk_patha_ok=%d", is_5g_pdk_patha_ok);
            va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
        }
        if (mib_get_s(MIB_HW_RF_DPK_DP_5G_PATH_B_OK, (void *)&is_5g_pdk_pathb_ok, sizeof(is_5g_pdk_pathb_ok))) {
            snprintf(parm, sizeof(parm), "is_5g_pdk_pathb_ok=%d", is_5g_pdk_pathb_ok);
            va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
        }

    #if defined(WLAN_8814_SERIES_SUPPORT)
        if (mib_get_s(MIB_HW_RF_DPK_DP_5G_PATH_C_OK, (void *)&is_5g_pdk_pathc_ok, sizeof(is_5g_pdk_pathc_ok))) {
            snprintf(parm, sizeof(parm), "is_5g_pdk_pathc_ok=%d", is_5g_pdk_pathc_ok);
            va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
        }
        if (mib_get_s(MIB_HW_RF_DPK_DP_5G_PATH_D_OK, (void *)&is_5g_pdk_pathd_ok, sizeof(is_5g_pdk_pathd_ok))) {
            snprintf(parm, sizeof(parm), "is_5g_pdk_pathd_ok=%d", is_5g_pdk_pathd_ok);
            va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
        }
    #endif

        if (is_5g_pdk_patha_ok==1 && is_5g_pdk_pathb_ok==1
        #if defined(WLAN_8814_SERIES_SUPPORT)
            && is_5g_pdk_pathc_ok==1 && is_5g_pdk_pathd_ok==1
        #endif
            ) {
            len = PWSF_5G_LEN;
        
            if (mib_get_s(MIB_HW_RF_DPK_PWSF_5G_A, (void *)value, sizeof(value))) {
                iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "pwsf_5g_a", len, value);
            }

            if (mib_get_s(MIB_HW_RF_DPK_PWSF_5G_B, (void *)value, sizeof(value))) {
                iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "pwsf_5g_b", len, value);
            }

        #if defined(WLAN_8814_SERIES_SUPPORT)
            if (mib_get_s(MIB_HW_RF_DPK_PWSF_5G_C, (void *)value, sizeof(value))) {
                iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "pwsf_5g_c", len, value);
            }

            if (mib_get_s(MIB_HW_RF_DPK_PWSF_5G_D, (void *)value, sizeof(value))) {
                iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "pwsf_5g_d", len, value);
            }
        #endif

            len = LUT_5G_LEN * 4;

            //5G Path A
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G_EVEN_A0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G_LEN);
                    sprintf(mib_name, "lut_5g_even_a%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G_ODD_A0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G_LEN);
                    sprintf(mib_name, "lut_5g_odd_a%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
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
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G_ODD_B0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G_LEN);
                    sprintf(mib_name, "lut_5g_odd_b%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }

        #if defined(WLAN_8814_SERIES_SUPPORT)
            //5G Path C
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G_EVEN_C0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G_LEN);
                    sprintf(mib_name, "lut_5g_even_c%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G_ODD_C0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G_LEN);
                    sprintf(mib_name, "lut_5g_odd_c%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }

            //5G Path D
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G_EVEN_D0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G_LEN);
                    sprintf(mib_name, "lut_5g_even_d%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G_ODD_D0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G_LEN);
                    sprintf(mib_name, "lut_5g_odd_d%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }
        #endif
        }
    }
#if defined(TRIBAND_SUPPORT)
END_5G1:
#endif
#endif

#if defined(TRIBAND_SUPPORT)
    if (phyband == PHYBAND_5G) {
    #define LUT_5G2_LEN 16
    #define PWSF_5G2_LEN 9
        unsigned int  lut_val[PWSF_5G2_LEN][LUT_5G2_LEN];
        unsigned char is_5g2_pdk_patha_ok = 0, is_5g2_pdk_pathb_ok = 0;
    #if defined(WLAN_8814_SERIES_SUPPORT)
        unsigned char is_5g2_pdk_pathc_ok = 0, is_5g2_pdk_pathd_ok = 0;
    #endif

        if (wlan_idx != 1)
            goto END_5G2;

        strcpy(ifname, getWlanIfName());
    
        if (mib_get_s(MIB_HW_RF_DPK_DP_5G2_PATH_A_OK, (void *)&is_5g2_pdk_patha_ok, sizeof(is_5g2_pdk_patha_ok))) {
            snprintf(parm, sizeof(parm), "is_5g2_pdk_patha_ok=%d", is_5g2_pdk_patha_ok);
            va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
        }
        if (mib_get_s(MIB_HW_RF_DPK_DP_5G2_PATH_B_OK, (void *)&is_5g2_pdk_pathb_ok, sizeof(is_5g2_pdk_pathb_ok))) {
            snprintf(parm, sizeof(parm), "is_5g2_pdk_pathb_ok=%d", is_5g2_pdk_pathb_ok);
            va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
        }

    #if defined(WLAN_8814_SERIES_SUPPORT)
        if (mib_get_s(MIB_HW_RF_DPK_DP_5G2_PATH_C_OK, (void *)&is_5g2_pdk_pathc_ok, sizeof(is_5g2_pdk_pathc_ok))) {
            snprintf(parm, sizeof(parm), "is_5g2_pdk_pathc_ok=%d", is_5g2_pdk_pathc_ok);
            va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
        }
        if (mib_get_s(MIB_HW_RF_DPK_DP_5G2_PATH_D_OK, (void *)&is_5g2_pdk_pathd_ok, sizeof(is_5g2_pdk_pathd_ok))) {
            snprintf(parm, sizeof(parm), "is_5g2_pdk_pathd_ok=%d", is_5g2_pdk_pathd_ok);
            va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
        }
    #endif

        if (is_5g2_pdk_patha_ok==1 && is_5g2_pdk_pathb_ok==1
        #if defined(WLAN_8814_SERIES_SUPPORT)
            && is_5g2_pdk_pathc_ok==1 && is_5g2_pdk_pathd_ok==1
        #endif
            ) {
            len = PWSF_5G2_LEN;
        
            if (mib_get_s(MIB_HW_RF_DPK_PWSF_5G2_A, (void *)value, sizeof(value))) {
                iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "pwsf_5g2_a", len, value);
            }

            if (mib_get_s(MIB_HW_RF_DPK_PWSF_5G2_B, (void *)value, sizeof(value))) {
                iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "pwsf_5g2_b", len, value);
            }

        #if defined(WLAN_8814_SERIES_SUPPORT)
            if (mib_get_s(MIB_HW_RF_DPK_PWSF_5G2_C, (void *)value, sizeof(value))) {
                iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "pwsf_5g2_c", len, value);
            }

            if (mib_get_s(MIB_HW_RF_DPK_PWSF_5G2_D, (void *)value, sizeof(value))) {
                iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", "pwsf_5g2_d", len, value);
            }
        #endif

            len = LUT_5G2_LEN * 4;

            //5G Path A
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G2_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G2_EVEN_A0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G2_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G2_LEN);
                    sprintf(mib_name, "lut_5g2_even_a%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G2_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G2_ODD_A0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G2_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G2_LEN);
                    sprintf(mib_name, "lut_5g2_odd_a%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }

            //5G Path B
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G2_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G2_EVEN_B0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G2_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G2_LEN);
                    sprintf(mib_name, "lut_5g2_even_b%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G2_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G2_ODD_B0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G2_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G2_LEN);
                    sprintf(mib_name, "lut_5g2_odd_b%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }

        #if defined(WLAN_8814_SERIES_SUPPORT)
            //5G Path C
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G2_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G2_EVEN_C0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G2_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G2_LEN);
                    sprintf(mib_name, "lut_5g2_even_c%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G2_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G2_ODD_C0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G2_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G2_LEN);
                    sprintf(mib_name, "lut_5g2_odd_c%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }

            //5G Path D
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G2_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G2_EVEN_D0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G2_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G2_LEN);
                    sprintf(mib_name, "lut_5g2_even_d%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }
            memset(lut_val, '\0', sizeof(lut_val));
            for (i=0; i<PWSF_5G2_LEN; i++) {
                mib_id = MIB_HW_RF_DPK_LUT_5G2_ODD_D0+i*4;
                if (mib_get_s(mib_id, (void *)value, sizeof(value))) {
                    memcpy(lut_val[i], value, LUT_5G2_LEN*4);
                    LUT_SWAP(lut_val[i], LUT_5G2_LEN);
                    sprintf(mib_name, "lut_5g2_odd_d%1d", i);
                    iwpriv_cmd(IWPRIV_HS|IWPRIV_HWDPK, ifname, "set_mib", mib_name, len, lut_val[i]);
                }
            }
        #endif
        }
    }
END_5G2:
#endif
#endif //#ifndef CONFIG_RF_DPK_SETTING_SUPPORT
	return status;
}
#if defined WLAN_QoS && (!defined CONFIG_RTL8192CD && !defined(CONFIG_RTL8192CD_MODULE)) 
int setupWLanQos(char *argv[6])
{
	int i, status;
	unsigned char value[34];
	char parm[64];
	MIB_WLAN_QOS_T QOSEntry;

	for (i=0; i<4; i++) {
		if (!mib_chain_get(MIB_WLAN_QOS_AP_TBL, i, (void *)&QOSEntry)) {
  			printf("Error! Get MIB_WLAN_AP_QOS_TBL error.\n");
  			continue;
		}
		switch(i){
			case 0://VO
				value[0] = QOSEntry.txop;
				snprintf(parm,sizeof(parm),"vo_txop=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmax;
				snprintf(parm,sizeof(parm),"vo_ecwmax=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmin;
				snprintf(parm,sizeof(parm),"vo_ecwmin=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.aifsn;
				snprintf(parm,sizeof(parm),"vo_aifsn=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				break;
			case 1://VI
				value[0] = QOSEntry.txop;
				snprintf(parm,sizeof(parm),"vi_txop=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmax;
				snprintf(parm,sizeof(parm),"vi_ecwmax=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmin;
				snprintf(parm,sizeof(parm),"vi_ecwmin=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.aifsn;
				snprintf(parm,sizeof(parm),"vi_aifsn=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				break;
			case 2://BE
				value[0] = QOSEntry.txop;
				snprintf(parm,sizeof(parm),"be_txop=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmax;
				snprintf(parm,sizeof(parm),"be_ecwmax=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmin;
				snprintf(parm,sizeof(parm),"be_ecwmin=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.aifsn;
				snprintf(parm,sizeof(parm),"be_aifsn=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				break;
			case 3://BK
				value[0] = QOSEntry.txop;
				snprintf(parm,sizeof(parm),"bk_txop=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmax;
				snprintf(parm,sizeof(parm),"bk_ecwmax=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmin;
				snprintf(parm,sizeof(parm),"bk_ecwmin=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.aifsn;
				snprintf(parm,sizeof(parm),"bk_aifsn=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				break;
			default:
				break;
		}
	}

	//sta
	for (i=0; i<4; i++) {
		if (!mib_chain_get(MIB_WLAN_QOS_STA_TBL, i, (void *)&QOSEntry)) {
  			printf("Error! Get MIB_WLAN_STA_QOS_TBL error.\n");
  			continue;
		}
		switch(i){
			case 0://VO
				value[0] = QOSEntry.txop;
				snprintf(parm,sizeof(parm),"sta_vo_txop=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmax;
				snprintf(parm,sizeof(parm),"sta_vo_ecwmax=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmin;
				snprintf(parm,sizeof(parm),"sta_vo_ecwmin=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.aifsn;
				snprintf(parm,sizeof(parm),"sta_vo_aifsn=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				break;
			case 1://VI
				value[0] = QOSEntry.txop;
				snprintf(parm,sizeof(parm),"sta_vi_txop=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmax;
				snprintf(parm,sizeof(parm),"sta_vi_ecwmax=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmin;
				snprintf(parm,sizeof(parm),"sta_vi_ecwmin=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.aifsn;
				snprintf(parm,sizeof(parm),"sta_vi_aifsn=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				break;
			case 2://BE
				value[0] = QOSEntry.txop;
				snprintf(parm,sizeof(parm),"sta_be_txop=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmax;
				snprintf(parm,sizeof(parm),"sta_be_ecwmax=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmin;
				snprintf(parm,sizeof(parm),"sta_be_ecwmin=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.aifsn;
				snprintf(parm,sizeof(parm),"sta_be_aifsn=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				break;
			case 3://BK
				value[0] = QOSEntry.txop;
				snprintf(parm,sizeof(parm),"sta_bk_txop=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmax;
				snprintf(parm,sizeof(parm),"sta_bk_ecwmax=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.ecwmin;
				snprintf(parm,sizeof(parm),"sta_bk_ecwmin=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				value[0] = QOSEntry.aifsn;
				snprintf(parm,sizeof(parm),"sta_bk_aifsn=%u",value[0]);
				argv[3]=parm;
				TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
				status|=do_cmd(IWPRIV, argv, 1);
				break;
			default:
				break;
		}
	}
	return status;
}
#endif

void setupWlan_TRX_Restrict(int vwlan_idx)
{
    int tx_bandwidth, rx_bandwidth, qbwc_mode = GBWC_MODE_DISABLE;
	char ifname[16];
	MIB_CE_MBSSIB_T Entry;

	rtk_wlan_get_ifname(wlan_idx, vwlan_idx, ifname);
	
	if(!mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry))
		printf("Error! Get MIB_MBSSIB_TBL error.\n");
#if 1
	tx_bandwidth = Entry.tx_restrict;
	rx_bandwidth = Entry.rx_restrict;
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

    iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "gbwcmode", qbwc_mode);
#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_CTCAPD)
    iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "gbwcthrd_tx", tx_bandwidth);    
    iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "gbwcthrd_rx", rx_bandwidth);
#else
    iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "gbwcthrd_tx", tx_bandwidth*1024);    
    iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "gbwcthrd_rx", rx_bandwidth*1024);
#endif
}

void setupWlan_IAPP(char *ifname)
{
#ifdef CONFIG_IAPP_SUPPORT
    unsigned int enabled;
#if 0 // defined(WLAN_DUALBAND_CONCURRENT) && !defined(CONFIG_RTK_DEV_AP)
	if(strcmp(ifname ,WLANIF[1]) == 0) { // wlan1
		mib_get_s(MIB_WLAN1_IAPP_ENABLED, (void *)&enabled, sizeof(enabled));
    }
	else 
#endif
    {
        mib_get_s(MIB_WLAN_IAPP_ENABLED, (void *)&enabled, sizeof(enabled));
    }

    iwpriv_cmd(IWPRIV_INT, (char *)getWlanIfName(), "set_mib", "iapp_enable", enabled);
#endif
}

void setupWlan_STBC(char *ifname)
{
    unsigned int enabled;
#if 0 // defined(WLAN_DUALBAND_CONCURRENT) && !defined(CONFIG_RTK_DEV_AP)
	if(strcmp(ifname ,WLANIF[1]) == 0) { // wlan1
		mib_get_s(MIB_WLAN1_STBC_ENABLED, (void *)&enabled, sizeof(enabled));
    }
	else 
#endif
    {
        mib_get_s(MIB_WLAN_STBC_ENABLED, (void *)&enabled, sizeof(enabled));
    }

    iwpriv_cmd(IWPRIV_INT, (char *)getWlanIfName(), "set_mib", "stbc", enabled);
}

void setupWlan_LDPC(char *ifname)
{
    unsigned int enabled;
#if 0 // defined(WLAN_DUALBAND_CONCURRENT) && !defined(CONFIG_RTK_DEV_AP)
	if(strcmp(ifname ,WLANIF[1]) == 0) { // wlan1
		mib_get_s(MIB_WLAN1_LDPC_ENABLED, (void *)&enabled, sizeof(enabled));
    }
	else 
#endif
    {
        mib_get_s(MIB_WLAN_LDPC_ENABLED, (void *)&enabled, sizeof(enabled));
    }

    iwpriv_cmd(IWPRIV_INT, (char *)getWlanIfName(), "set_mib", "ldpc", enabled);
}

void setupWlan_TDLS(char *ifname)
{
#if defined(TDLS_SUPPORT)
    unsigned char val, val2;
#if 0 // defined(WLAN_DUALBAND_CONCURRENT) && !defined(CONFIG_RTK_DEV_AP)
	if(strcmp(ifname ,WLANIF[1]) == 0) { // wlan1
		mib_get_s(MIB_WLAN1_TDLS_PROHIBITED, (void *)&val, sizeof(val));
        mib_get_s(MIB_WLAN1_TDLS_CS_PROHIBITED, (void *)&val2, sizeof(val2));
    }
	else 
#endif
    {
        mib_get_s(MIB_WLAN_TDLS_PROHIBITED, (void *)&val, sizeof(val));
        mib_get_s(MIB_WLAN_TDLS_CS_PROHIBITED, (void *)&val2, sizeof(val2));
    }

    iwpriv_cmd(IWPRIV_INT, (char *)getWlanIfName(), "set_mib", "tdls_prohibited", val);
    iwpriv_cmd(IWPRIV_INT, (char *)getWlanIfName(), "set_mib", "tdls_cs_prohibited", val2);
#endif //TDLS_SUPPORT
}

void setupWlanExtraMib()
{
    char *ifname = (char *)getWlanIfName();
    setupWlan_IAPP(ifname);
    setupWlan_STBC(ifname);
    setupWlan_LDPC(ifname);
    setupWlan_TDLS(ifname);
}

#if defined(CONFIG_00R0) && (defined(WLAN0_5G_SUPPORT) || defined(WLAN1_5G_SUPPORT))

struct channel_list{
	unsigned char   channel[31];
	unsigned char   len;
};

#ifdef CONFIG_RTL_DFS_SUPPORT
//copied from 8192cd_util.c
static struct channel_list reg_channel_5g_full_band[] = {
        /* FCC */               {{36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165},20},
        /* IC */                {{36,40,44,48,52,56,60,64,149,153,157,161},12},
        /* ETSI */              {{36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140},19},
        /* SPAIN */             {{36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140},19},
        /* FRANCE */    {{36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140},19},
        /* MKK */               {{36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140},19},
        /* ISRAEL */    {{36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140},19},
        /* MKK1 */              {{34,38,42,46},4},
        /* MKK2 */              {{36,40,44,48},4},
        /* MKK3 */              {{36,40,44,48,52,56,60,64},8},
        /* NCC (Taiwan) */      {{56,60,64,100,104,108,112,116,136,140,149,153,157,161,165},15},
        /* RUSSIAN */   {{36,40,44,48,52,56,60,64,132,136,140,149,153,157,161,165},16},
        /* CN */                {{36,40,44,48,52,56,60,64,149,153,157,161,165},13},
        /* Global */            {{36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165},20},
        /* World_wide */        {{36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165},20},
        /* Test */              {{36,40,44,48, 52,56,60,64, 100,104,108,112, 116,120,124,128, 132,136,140,144, 149,153,157,161, 165,169,173,177}, 28},
        /* 5M10M */             {{146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170}, 25},
};
#else
static struct channel_list reg_channel_5g_full_band[] = {
        /* FCC */               {{36,40,44,48,149,153,157,161,165},9},
        /* IC */                {{36,40,44,48,149,153,157,161},8},
        /* ETSI */              {{36,40,44,48},4},
        /* SPAIN */             {{36,40,44,48},4},
        /* FRANCE */    {{36,40,44,48},4},
        /* MKK */               {{36,40,44,48},4},
        /* ISRAEL */    {{36,40,44,48},4},
        /* MKK1 */              {{34,38,42,46},4},
        /* MKK2 */              {{36,40,44,48},4},
        /* MKK3 */              {{36,40,44,48},4},
        /* NCC (Taiwan) */      {{56,60,64,149,153,157,161,165},8},
        /* RUSSIAN */   {{36,40,44,48,149,153,157,161,165},9},
        /* CN */                {{36,40,44,48,149,153,157,161,165},9},
        /* Global */            {{36,40,44,48,149,153,157,161,165},9},
        /* World_wide */        {{36,40,44,48,149,153,157,161,165},9},
        /* Test */              {{36,40,44,48, 52,56,60,64, 100,104,108,112, 116,120,124,128, 132,136,140,144, 149,153,157,161, 165,169,173,177}, 28},
        /* 5M10M */             {{146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170}, 25},
};
#endif

static int setupWLan_5G_channel(char *ifname)
{
	unsigned char regdomain = 1;
	unsigned char channel_width = 0;
	unsigned char channel = 0;
	int status=0, i;
	
	if(!mib_get_s(MIB_WLAN_CHANNEL_WIDTH, &channel_width, sizeof(channel_width)))
		printf("get MIB_WLAN_CHANNEL_WIDTH failed\n");	
	
	if(channel_width!=0){ //40MHz or 80MHz

		if(!mib_get_s(MIB_HW_REG_DOMAIN, &regdomain, sizeof(regdomain)))
			printf("get MIB_HW_REG_DOMAIN failed\n");
		regdomain = regdomain - 1;
		
		if(!mib_get_s(MIB_WLAN_CHAN_NUM, &channel, sizeof(channel)))
			printf("get MIB_WLAN_CHAN_NUM failed\n");
		
		for(i=0; i<(reg_channel_5g_full_band[regdomain].len-1); i++){
			if(reg_channel_5g_full_band[regdomain].channel[i]==channel)
				break;
			else if(channel > reg_channel_5g_full_band[regdomain].channel[i] && channel < reg_channel_5g_full_band[regdomain].channel[i+1] )
			{
				channel = reg_channel_5g_full_band[regdomain].channel[i];
				break;
			}
		}
		status|=iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "channel", channel);
	}
	else{ //20MHz
		status|=iwpriv_cmd(IWPRIV_GETMIB|IWPRIV_INT, ifname, "set_mib", "channel", MIB_WLAN_CHAN_NUM);
	}

	return status;
	
}
#endif

#if defined(CONFIG_ANDLINK_SUPPORT)
static int get_file_value(char *filename, int *value)
{
	FILE *fp = NULL;
	char buff[1024] = {0};

	if(isFileExist(filename)){
		fp = fopen(filename, "r");
		if(fp==NULL){
			printf("open %s fail \n", filename);
			return -1;
		}
		
		while (fgets(buff, sizeof(buff), fp)) {
			buff[strlen(buff)-1]='\0';
			*value = atoi(buff);
			break;
		}

		fclose(fp);
	}
	
	return 0;	
}

static int setupAndlink()
{
	char link_enable=0, rtl_link_mode=0, sta_num, rssi_th, time_gap, switch_enable, qos_enable;
	char retry_ratio, fail_ratio, dismiss_time, rssi_gap;
	char vifname[20] = {0}, cmcc_qlink[20]={0}, parm[64]={0};
	int i, j, ret = 0;
	int dev_reg=0, cmcc_qlink_len=0, tmpInt=0;
	MIB_CE_MBSSIB_T Entry;

	//printf("============> [%s %d]\n", __FUNCTION__,__LINE__);

	mib_get(MIB_RTL_LINK_ENABLE, (void *)&link_enable);
	mib_get(MIB_RTL_LINK_MODE, (void *)&rtl_link_mode);
	mib_get(MIB_RTL_LINK_DEV_REGISTER, (void *)&dev_reg);
	mib_get(MIB_RTL_LINK_ROAMING_TIME_GAP, (void *)&time_gap);
	mib_get(MIB_RTL_LINK_ROAMING_RSSI_GAP, (void *)&rssi_gap);
	mib_get(MIB_RTL_LINK_ROAMING_SWITCH, (void *)&switch_enable);
	mib_get(MIB_RTL_LINK_ROAMING_QOS, (void *)&qos_enable);
	mib_get(MIB_RTL_LINK_ROAMING_RETRY_RATIO, (void *)&retry_ratio);
	mib_get(MIB_RTL_LINK_ROAMING_FAIL_RATIO, (void *)&fail_ratio);
	mib_get(MIB_RTL_LINK_ROAMING_DISMISS_TIME, (void *)&dismiss_time);	
	memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));	
	if(link_enable)
	{
		for(i=0; i<NUM_WLAN_INTERFACE;++i){
			for(j=0; j<=WLAN_MBSSID_NUM; ++j){

				rtk_wlan_get_ifname(i, j, vifname);

				if(time_gap==0)
					time_gap = 1;
				ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "sta_roaming_time_gap", time_gap);	
				ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "sta_roaming_rssi_gap", rssi_gap);
				ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "roaming_switch", switch_enable);
				ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "roaming_qos", qos_enable);				
				ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "fail_ratio", fail_ratio);
				ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "retry_ratio", retry_ratio);		
				ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "block_aging", dismiss_time);

				if(!mib_chain_get(MIB_MBSSIB_TBL, j, (void *)&Entry))
					printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");
				ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "RSSIThreshold", Entry.rtl_link_rssi_th);
				ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "stanum", Entry.rtl_link_sta_num);				
			}
		}    	
    }

	// if andlink repeater mode and not registered, then set 2.4g vxd ssid to CMCC-QLINK
#if defined (WLAN1_5G_SUPPORT)
	int qlink_band_index = 0;
#else
	int qlink_band_index = 1;
#endif
	//snprintf(vifname, sizeof(vifname), "%s", WLANIF[qlink_band_index]);
	if(link_enable && (rtl_link_mode==1)){
		//if(!strncmp(getWlanIfName(), vifname, 5))
		//{
			if(dev_reg==ANDLINK_DEV_NOT_REG && !isFileExist(ANDLINK_BLOCK_CHANGE_QLINK_SSID)){
				snprintf(cmcc_qlink, sizeof(cmcc_qlink), "%s", "CMCC-QLINK");
				snprintf(vifname, sizeof(vifname), VXD_IF, qlink_band_index);
				printf("Andlink Repeater Mode, Set vxd SSID to CMCC-QLINK for %s\n", vifname);	//shrink time
				//set ssid
				snprintf(parm, sizeof(parm), "ssid=%s", cmcc_qlink);
				ret|=va_cmd(IWPRIV, 3, 1, vifname, "set_mib", parm);
				//set encmode
				ret|=va_cmd(IWPRIV, 3, 1, vifname, "set_mib", "encmode=0");
			}
			if(isFileExist(ANDLINK_BLOCK_CHANGE_QLINK_SSID)){
				/*1: qlink success; 
				  2: vxd wps success, the file is unlink in andlink */
				get_file_value(ANDLINK_BLOCK_CHANGE_QLINK_SSID, &tmpInt);
				if(tmpInt==1)
					unlink(ANDLINK_BLOCK_CHANGE_QLINK_SSID);
			}
		//}
	}

	return ret;
}

static int rtl_link_get_channel(char *ifname)
{
	unsigned char root_ifname[10] = {0}, filename[50] = {0};
	FILE *fp;
	char line_buffer[256] = {0};
	char *pchar;
	int channel;
	unsigned char rtl_link_mode = 0;

	mib_get(MIB_RTL_LINK_MODE, (void *)&rtl_link_mode);
	if(rtl_link_mode != 1) // andlink repeater mode
		return;

	snprintf(root_ifname, sizeof(root_ifname), "%s", ifname);	
	root_ifname[5] = '\0';
	//snprintf(filename, sizeof(filename), "/proc/%s/mib_rf", root_ifname);
	snprintf(filename, sizeof(filename), LINK_FILE_NAME_FMT, root_ifname, "mib_rf");
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
		}else{
			channel = 1; // just care about 2G
		} 

		return channel;		
}

static void rtl_link_set_channel(char *ifname, int channel)
{
	unsigned char rtl_link_mode = 0, phyband=0, getValue = 0;
	unsigned char cmd[50] = {0}, root_ifname[10] = {0};	

	mib_get(MIB_RTL_LINK_MODE, (void *)&rtl_link_mode);
	if(rtl_link_mode != 1) // andlink repeater mode
		return;

	mib_save_wlanIdx();	

	snprintf(root_ifname, sizeof(root_ifname), "%s", ifname);
	root_ifname[5] = '\0';	
	SetWlan_idx(root_ifname);	
	mib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband);
	mib_get(MIB_WLAN_CHAN_NUM, (void *)&getValue);
	if(phyband == PHYBAND_2G && getValue==0){ 		
		snprintf(cmd, sizeof(cmd), "iwpriv %s set_mib channel=%d", ifname, channel);		
		system(cmd);
		//printf("=======>[%s %d]cmd:%s\n", __FUNCTION__,__LINE__,cmd);
	}

	mib_recov_wlanIdx();
	return;	
}
#endif
#if defined( CONFIG_ELINK_SUPPORT) || defined(CONFIG_USER_CTCAPD) || defined(CONFIG_ANDLINK_SUPPORT) || defined(CONFIG_USER_ANDLINK_PLUGIN)
static int setTelcoSelected(TELCO_SELECTED_T type)
{
	unsigned char telco_selected;
	unsigned char telco_type;
	int i=0,j=0,ret=0;
	char vifname[20] = {0};
	MIB_CE_MBSSIB_T Entry;

	telco_type=(unsigned char)type;
	mib_get_s(MIB_TELCO_SELECTED, (void *)&telco_selected,sizeof(telco_selected));
	
	if(telco_selected!=telco_type){
		telco_selected=telco_type;
		mib_set(MIB_TELCO_SELECTED, (void *)&telco_selected);
	}

	for(i=0; i<NUM_WLAN_INTERFACE;++i){
		for(j=0; j<=NUM_VWLAN_INTERFACE; ++j){

			rtk_wlan_get_ifname(i, j, vifname);
			memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&Entry);
			if(Entry.wlanDisabled)
				continue;

			ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "telco_selected", telco_type);
		}
	}
        mib_update(CURRENT_SETTING,CONFIG_MIB_ALL);
	
        return ret;
}

static int setupAndlinkPlugin()
{
    unsigned char link_enable, sta_num, switch_enable;
    char vifname[20] = {0};
    int i, j, ret = 0;
    MIB_CE_MBSSIB_T Entry;

    mib_get_s(MIB_RTL_LINK_ROAMING_SWITCH, (void *)&switch_enable,sizeof(switch_enable));
    memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));

	setTelcoSelected(TELCO_CMCC); //set MIB_TELCO_SELECTED as CMCC

    for(i=0; i<NUM_WLAN_INTERFACE;++i){
        for(j=0; j<=WLAN_MBSSID_NUM; ++j){
            if(j==0)
            	snprintf(vifname, sizeof(vifname), WLAN_IF, i);
            else
                snprintf(vifname, sizeof(vifname), VAP_IF, i, j-1);

            ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "roaming_switch", switch_enable);

			if(!mib_chain_get(MIB_MBSSIB_TBL, j, (void *)&Entry))
                    printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");

            ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "RSSIThreshold", Entry.rtl_link_rssi_th);
            ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "stanum", Entry.rtl_link_sta_num);
        }
    }

        return ret;
}
#endif
#ifdef CONFIG_ELINK_SUPPORT
static int setupElink()
{
    unsigned char link_enable, sta_num, rssi_th, time_gap, switch_enable, qos_enable;
    char retry_ratio, fail_ratio, dismiss_time, rssi_gap;
    char vifname[20] = {0};
    int i, j, ret = 0;
    MIB_CE_MBSSIB_T Entry;

    mib_get_s(MIB_RTL_LINK_ENABLE, (void *)&link_enable,sizeof(link_enable));
    mib_get_s(MIB_RTL_LINK_ROAMING_SWITCH, (void *)&switch_enable,sizeof(switch_enable));
    mib_get_s(MIB_RTL_LINK_ROAMING_QOS, (void *)&qos_enable,sizeof(qos_enable));

    memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));

    if(link_enable)
    {
        for(i=0; i<NUM_WLAN_INTERFACE;++i){
            for(j=0; j<=WLAN_MBSSID_NUM; ++j){
                rtk_wlan_get_ifname(i, j, vifname);

                ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "roaming_switch", switch_enable);
                ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "roaming_qos", qos_enable);
                printf("[%s %d]switch_enable:%d  qos_enable:%d ",__FUNCTION__,__LINE__,switch_enable,qos_enable);

				if(!mib_chain_get(MIB_MBSSIB_TBL, j, (void *)&Entry))
                        printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");
				
                ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "RSSIThreshold", Entry.rtl_link_rssi_th);
                ret|=iwpriv_cmd(IWPRIV_INT, vifname, "set_mib", "stanum", Entry.rtl_link_sta_num);
            }
        }
    }

        return ret;
}
#endif
#ifdef WLAN_ROAMING
static int setupWLan_Roaming(int vwlan_idx)
{
	char ifname[16];
	char parm[128];
	int status=0;
	unsigned char enable, rssi_th1, rssi_th2;
	unsigned int start_time;
	unsigned char phyband;

	if(vwlan_idx != 0) //now support 2G-1, 5G-1 only
		return 0;

	rtk_wlan_get_ifname(wlan_idx, vwlan_idx, ifname);

	mib_get_s(MIB_WLAN_PHY_BAND_SELECT, &phyband, sizeof(phyband));

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
			if(Entry.wlanDisabled==0){
				rtk_wlan_get_ifname(i, j, intfList[intfNum]);
				cmd_opt[cmd_cnt++] = intfList[intfNum];
				intfNum++;
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
static int checkWlanRateRriorSetting(void)
{
	int i=0;
	unsigned char channel_width = 0, coexist=0, phyband=0, rate_prior=0;
	MIB_CE_MBSSIB_T Entry;
	unsigned char change = 0;
	mib_get_s(MIB_WLAN_RATE_PRIOR, (void *)&rate_prior, sizeof(rate_prior));

	if(rate_prior == 0) // do not check if rate prior is disabled.
		return 0;

	mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));

	//update channelwidth
	mib_get_s(MIB_WLAN_CHANNEL_WIDTH,(void *)&channel_width, sizeof(channel_width));
	if(phyband == PHYBAND_2G){
		mib_get_s(MIB_WLAN_11N_COEXIST,(void *)&coexist, sizeof(coexist));
		if((channel_width!=1) || (coexist!=1)){
			if(channel_width!=1){
				channel_width = 1;
				change = 1;
				mib_set(MIB_WLAN_CHANNEL_WIDTH, (void *)&channel_width);
			}
			if(coexist!=1){
				coexist = 1;
				change = 1;
				mib_set(MIB_WLAN_11N_COEXIST, (void *)&coexist);
			}
		}
	}
	else if(phyband == PHYBAND_5G){
		if(channel_width!=2){
			if(channel_width!=2){
				channel_width = 2;
				change = 1;
				mib_set(MIB_WLAN_CHANNEL_WIDTH, (void *)&channel_width);
			}
		}
	}

	for(i=0; i<WLAN_SSID_NUM; i++){
		if(!mib_chain_get(MIB_MBSSIB_TBL, i, (void *)&Entry))
			continue;
		if(phyband == PHYBAND_2G){
			if(Entry.wlanBand != BAND_11N){
				Entry.wlanBand = BAND_11N;
				change = 1;
				mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, i);
			}
		}
		else if(phyband == PHYBAND_5G){
			if(Entry.wlanBand != BAND_5G_11AC){
				Entry.wlanBand = BAND_5G_11AC;
				change = 1;
				mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, i);
			}
		}
	}
	if(change == 1)
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);

	return 1;
}
#endif
static void checkWlanSetting(void)
{
	MIB_CE_MBSSIB_T Entry;
	unsigned char vBandwidth = 0, phymode = 0, phyband = 0;
	unsigned char change = 0;

#ifdef WLAN_RATE_PRIOR
	if(checkWlanRateRriorSetting()==1)
		return;
#endif

	if(!mib_chain_get(MIB_MBSSIB_TBL, WLAN_ROOT_ITF_INDEX, (void *)&Entry)){
		printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");
		return ;
	}

	phymode = Entry.wlanBand;

	mib_get_s(MIB_WLAN_CHANNEL_WIDTH,(void *)&vBandwidth, sizeof(vBandwidth));
	mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));

	if(phyband == PHYBAND_2G)
	{
		if(phymode == BAND_11B || phymode == BAND_11G || phymode == BAND_11BG )
		{
			if(vBandwidth != 0){
				vBandwidth = 0;
				mib_set(MIB_WLAN_CHANNEL_WIDTH,(void *)&vBandwidth);
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
				mib_set(MIB_WLAN_CHANNEL_WIDTH,(void *)&vBandwidth);
				change = 1;
			}
		}
		else if(phymode == BAND_5G_11AN || phymode == BAND_11N)
		{
			if(vBandwidth >= 2){
				vBandwidth = 0;
				mib_set(MIB_WLAN_CHANNEL_WIDTH,(void *)&vBandwidth);
				change = 1;
			}
		}
	}
	if(change == 1)
		mib_update(CURRENT_SETTING, CONFIG_MIB_TABLE);
}
#endif

static int _rtk_wlan_set_special_fixed_settings(unsigned char phyband, char *ifname, int vwlan_idx)
{
	int status = 0;
#if defined(CONFIG_00R0)
	MIB_CE_MBSSIB_T Entry;
#endif

#if defined(CONFIG_00R0)
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
#if defined(CONFIG_WLAN_HAL_8814AE) && defined(CONFIG_WLAN_HAL_8192EE)
		if(phyband == PHYBAND_5G)
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "limit_rxloop=256");
#endif
#if defined(CONFIG_WLAN_HAL_8814AE) && (defined(CONFIG_SLOT_0_8194AE) || defined(CONFIG_SLOT_1_8194AE))
		if(phyband == PHYBAND_2G)
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "limit_rxloop=140");
#endif
	}
#endif //end of defined(CONFIG_00R0)
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "rx_min_rsvd_mem=5000");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "amsduMax=0");
	}
#endif
#ifdef CONFIG_00R0
	set5GAutoChannelLow();
	setSiteSurveyTime();

	va_cmd(IWPRIV, 3, 1, "wlan0", "set_mib", "ss_igi=48");
	va_cmd(IWPRIV, 3, 1, "wlan1", "set_mib", "ss_igi=48");

        if(!mib_chain_get(MIB_MBSSIB_TBL, WLAN_ROOT_ITF_INDEX, (void *)&Entry))
	                printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");

	if(strcmp( getWlanIfName(), "wlan0")==0 && Entry.func_off==1) 
		va_cmd(IWPRIV, 3, 1, "wlan0", "set_mib", "func_off=1");
	else if(strcmp( getWlanIfName(), "wlan0")==0 && Entry.func_off==0)
		va_cmd(IWPRIV, 3, 1, "wlan0", "set_mib", "func_off=0");
	else if(strcmp( getWlanIfName(), "wlan1")==0 && Entry.func_off==1)
		va_cmd(IWPRIV, 3, 1, "wlan1", "set_mib", "func_off=1");
	else if(strcmp( getWlanIfName(), "wlan1")==0 && Entry.func_off==0)
		va_cmd(IWPRIV, 3, 1, "wlan1", "set_mib", "func_off=0");
#endif
	return status;
}

#ifdef WLAN_FAST_INIT
static int setupWLan(char *ifname, int vwlan_idx)
{
	struct wifi_mib *pmib;
	int i, skfd, intVal;
	unsigned int vInt;
	unsigned short int sInt;
	unsigned char buf1[1024];
	unsigned char value[196];
	struct iwreq wrq, wrq_root;
	MIB_CE_MBSSIB_T Entry;
	unsigned char wlan_mode, vChar, band, phyband;
	int vap_enable=0;
#ifdef WLAN_UNIVERSAL_REPEATER
	char rpt_enabled;
#endif

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd < 0)
		return -1;

    // Get wireless name
    if ( iw_get_ext(skfd, ifname, SIOCGIWNAME, &wrq) < 0) {
      close( skfd );
      // If no wireless name : no wireless extensions
      return -1;
    }

	if ((pmib = (struct wifi_mib *)malloc(sizeof(struct wifi_mib))) == NULL) {
		printf("MIB buffer allocation failed!\n");
		close(skfd);
		return -1;
	}

	// Disable WLAN MAC driver and shutdown interface first
	//sprintf((char *)buf1, "ifconfig %s down", ifname);
	//system((char *)buf1);

	//va_cmd(IFCONFIG, 2, 1, ifname, "down");

	if (vwlan_idx == 0) {
		// shutdown all WDS interface
		#if 0 //def WLAN_WDS
		for (i=0; i<8; i++) {
			//sprintf((char *)buf1, "ifconfig %s-wds%d down", ifname, i);
			//system((char *)buf1);
			sprintf((char *)buf1, "%s-wds%d", ifname, i);
			va_cmd(IFCONFIG, 2, 1, buf1, "down");
		}
		#endif
		// kill wlan application daemon
		//sprintf((char *)buf1, "wlanapp.sh kill %s", ifname);
		//system((char *)buf1);
	}
	else { // virtual interface
		snprintf((char *)buf1, sizeof(buf1), WLAN_IF, wlan_idx);
		memset(wrq_root.ifr_name, 0, IFNAMSIZ);
		strncpy(wrq_root.ifr_name, (char *)buf1, IFNAMSIZ-1);
		if (ioctl(skfd, SIOCGIWNAME, &wrq_root) < 0) {
			printf("Root Interface %s open failed!\n", buf1);
			free(pmib);
			close(skfd);
			return -1;
		}
	}

#if 0
	if (vwlan_idx == 0) {
		mib_get_s(MIB_HW_RF_TYPE, (void *)&vChar, sizeof(vChar));
		if (vChar == 0) {
			printf("RF type is NULL!\n");
			free(pmib);
			close(skfd);
			return 0;
		}
	}
#endif
	if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
		printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");

	vChar = Entry.wlanDisabled;

	if (vChar == 1) {
		free(pmib);
		close(skfd);
		return 0;
	}

	// get mib from driver
	wrq.u.data.pointer = (caddr_t)pmib;
	wrq.u.data.length = sizeof(struct wifi_mib);

	if (vwlan_idx == 0) {
		if (ioctl(skfd, SIOCMIBINIT, &wrq) < 0) {
			printf("Get WLAN MIB failed!\n");
			free(pmib);
			close(skfd);
			return -1;
		}
	}
	else {
		wrq_root.u.data.pointer = (caddr_t)pmib;
		wrq_root.u.data.length = sizeof(struct wifi_mib);
		if (ioctl(skfd, SIOCMIBINIT, &wrq_root) < 0) {
			printf("Get WLAN MIB failed!\n");
			free(pmib);
			close(skfd);
			return -1;
		}
	}

	// check mib version
	if (pmib->mib_version != MIB_VERSION) {
		printf("WLAN MIB version mismatch!\n");
		free(pmib);
		close(skfd);
		return -1;
	}

	if (vwlan_idx > 0) {
		//if not root interface, clone root mib to virtual interface
		wrq.u.data.pointer = (caddr_t)pmib;
		wrq.u.data.length = sizeof(struct wifi_mib);
		if (ioctl(skfd, SIOCMIBSYNC, &wrq) < 0) {
			printf("Set WLAN MIB failed!\n");
			free(pmib);
			close(skfd);
			return -1;
		}
	}
	//pmib->miscEntry.func_off = 0;
	if (!mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry)) {
			printf("Error! Get MIB_MBSSIB_TBL for VAP SSID error.\n");
		free(pmib);
		close(skfd);
		return -1;
	}



#ifdef WLAN_MBSSID
	if(vwlan_idx > 0 && vwlan_idx < WLAN_REPEATER_ITF_INDEX)
#endif
	{
		struct sockaddr hwaddr;
		getInAddr(ifname, HW_ADDR, &hwaddr);
		memcpy(&(pmib->dot11OperationEntry.hwaddr[0]), hwaddr.sa_data, 6);
	}
	if (vwlan_idx == 0) {	//root
		mib_get_s(MIB_WIFI_TEST, (void *)value, sizeof(value));
		if(value[0])
			va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "mp_specific=1");
		else
			va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "mp_specific=0");

		mib_get_s(MIB_HW_REG_DOMAIN, (void *)&vChar, sizeof(vChar));
		pmib->dot11StationConfigEntry.dot11RegDomain = vChar;

		//mib_get_s(MIB_HW_LED_TYPE, (void *)&vChar, sizeof(vChar));
#if defined(CONFIG_RTL_92C_SUPPORT)
		pmib->dot11OperationEntry.ledtype = 11;
#else
		pmib->dot11OperationEntry.ledtype = 3;
#endif
	}

	wlan_mode = Entry.wlanMode;

	//SSID setting
	memset(pmib->dot11StationConfigEntry.dot11DesiredSSID, 0, 32);
	memset(pmib->dot11StationConfigEntry.dot11SSIDtoScan, 0, 32);

	if(vwlan_idx == 0 || (vwlan_idx > 0 && vwlan_idx < WLAN_REPEATER_ITF_INDEX && (wlan_mode == AP_MODE || wlan_mode == AP_WDS_MODE))){
		pmib->dot11StationConfigEntry.dot11DesiredSSIDLen = strlen((char *)Entry.ssid);
		memcpy(pmib->dot11StationConfigEntry.dot11DesiredSSID, Entry.ssid, strlen((char *)Entry.ssid));
		pmib->dot11StationConfigEntry.dot11SSIDtoScanLen = strlen((char *)Entry.ssid);
		memcpy(pmib->dot11StationConfigEntry.dot11SSIDtoScan, Entry.ssid, strlen((char *)Entry.ssid));
	}
#ifdef WLAN_UNIVERSAL_REPEATER
	else if(vwlan_idx == WLAN_REPEATER_ITF_INDEX){
		mib_get_s( MIB_REPEATER_ENABLED1, (void *)&rpt_enabled, sizeof(rpt_enabled));
		if (rpt_enabled) {
			pmib->dot11StationConfigEntry.dot11DesiredSSIDLen = strlen((char *)Entry.ssid);
			memcpy(pmib->dot11StationConfigEntry.dot11DesiredSSID, Entry.ssid, strlen((char *)Entry.ssid));
			pmib->dot11StationConfigEntry.dot11SSIDtoScanLen = strlen((char *)Entry.ssid);
			memcpy(pmib->dot11StationConfigEntry.dot11SSIDtoScan, Entry.ssid, strlen((char *)Entry.ssid));
		}
	}
#endif

#ifdef WLAN_MBSSID
	if(vwlan_idx==0){
		// opmode
		if (wlan_mode == CLIENT_MODE)
			vInt = 8;	// client
		else	// 0(AP_MODE) or 3(AP_WDS_MODE)
			vInt = 16;	// AP
		pmib->dot11OperationEntry.opmode = vInt;

		if(pmib->dot11OperationEntry.opmode & 0x00000010){// AP mode
			for (vwlan_idx = 1; vwlan_idx < (WLAN_MBSSID_NUM+1); vwlan_idx++) {
				mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry);
				if (Entry.wlanDisabled == 0)
					vap_enable++;
			}
			vwlan_idx = 0;
		}
		if (vap_enable && (wlan_mode ==  AP_MODE || wlan_mode ==  AP_WDS_MODE
#ifdef WLAN_MESH
			|| wlan_mode ==  AP_MESH_MODE
#endif
		))
			pmib->miscEntry.vap_enable=1;
		else
			pmib->miscEntry.vap_enable=0;
	}
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
	// Repeater opmode
	if(vwlan_idx == WLAN_REPEATER_ITF_INDEX){
		if (rpt_enabled) {
			if (wlan_mode == CLIENT_MODE)
				vInt = 16;	// Repeater is AP
			else
				vInt = 8;	// Repeater is Client

			pmib->dot11OperationEntry.opmode = vInt;
		}
	}
#endif

	if (vwlan_idx == 0) //root
	{
		//hw setting
		setupWlanHWSetting(ifname, pmib);
		setup8812Wlan(ifname, pmib);

		// tw power scale
		mib_get_s(MIB_TX_POWER, (void *)&vChar, sizeof(vChar));
		intVal = getTxPowerScale(vChar);

		if (intVal) {
			for (i=0; i<MAX_CHAN_NUM; i++) {
				if(pmib->dot11RFEntry.pwrlevelCCK_A[i] != 0){
					if ((pmib->dot11RFEntry.pwrlevelCCK_A[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevelCCK_A[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevelCCK_A[i] = 1;
				}
				if(pmib->dot11RFEntry.pwrlevelCCK_B[i] != 0){
					if ((pmib->dot11RFEntry.pwrlevelCCK_B[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevelCCK_B[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevelCCK_B[i] = 1;
				}
				if(pmib->dot11RFEntry.pwrlevelHT40_1S_A[i] != 0){
					if ((pmib->dot11RFEntry.pwrlevelHT40_1S_A[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevelHT40_1S_A[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevelHT40_1S_A[i] = 1;
				}
				if(pmib->dot11RFEntry.pwrlevelHT40_1S_B[i] != 0){
					if ((pmib->dot11RFEntry.pwrlevelHT40_1S_B[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevelHT40_1S_B[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevelHT40_1S_B[i] = 1;
				}
			}

			for (i=0; i<MAX_5G_CHANNEL_NUM; i++) {
				if(pmib->dot11RFEntry.pwrlevel5GHT40_1S_A[i] != 0){
					if ((pmib->dot11RFEntry.pwrlevel5GHT40_1S_A[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevel5GHT40_1S_A[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevel5GHT40_1S_A[i] = 1;
				}
				if(pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[i] != 0){
					if ((pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[i] = 1;
				}
			}
		}

		mib_get_s(MIB_WLAN_BEACON_INTERVAL, (void *)&sInt, sizeof(sInt));
		pmib->dot11StationConfigEntry.dot11BeaconPeriod = sInt;

		mib_get_s(MIB_WLAN_AUTO_CHAN_ENABLED, (void *)value, sizeof(value));
		if(value[0])
			pmib->dot11RFEntry.dot11channel = 0;
		else{
			mib_get_s(MIB_WLAN_CHAN_NUM, (void *)value, sizeof(value));
			pmib->dot11RFEntry.dot11channel = value[0];
		}
		band = Entry.wlanBand;
#ifdef WIFI_TEST
		if(band == 4) {// WiFi-G
			pmib->dot11StationConfigEntry.dot11BasicRates = 0x15f;
			pmib->dot11StationConfigEntry.dot11SupportedRates = 0xfff;
		}
		else if (band == 5){ // WiFi-BG
			pmib->dot11StationConfigEntry.dot11BasicRates = 0x00f;
			pmib->dot11StationConfigEntry.dot11SupportedRates = 0xfff;
		}
		else
#endif
		{
			mib_get_s(MIB_WIFI_SUPPORT, (void*)value, sizeof(value));
			if(value[0] == 1){
				if(band == 2) {// WiFi-G
					pmib->dot11StationConfigEntry.dot11BasicRates = 0x15f;
					pmib->dot11StationConfigEntry.dot11SupportedRates = 0xfff;
				}
				else if(band == 3){ // WiFi-BG
					pmib->dot11StationConfigEntry.dot11BasicRates = 0x00f;
					pmib->dot11StationConfigEntry.dot11SupportedRates = 0xfff;
				}
				else{
					mib_get_s(MIB_WLAN_BASIC_RATE, (void *)&sInt, sizeof(sInt));
					pmib->dot11StationConfigEntry.dot11BasicRates = sInt;
					mib_get_s(MIB_WLAN_SUPPORTED_RATE, (void *)&sInt, sizeof(sInt));
					pmib->dot11StationConfigEntry.dot11SupportedRates = sInt;
				}
			}
			else{
				mib_get_s(MIB_WLAN_BASIC_RATE, (void *)&sInt, sizeof(sInt));
				pmib->dot11StationConfigEntry.dot11BasicRates = sInt;
				mib_get_s(MIB_WLAN_SUPPORTED_RATE, (void *)&sInt, sizeof(sInt));
				pmib->dot11StationConfigEntry.dot11SupportedRates = sInt;

			}
		}

		mib_get_s(MIB_WLAN_INACTIVITY_TIME, (void *)&vInt, sizeof(vInt));
		pmib->dot11OperationEntry.expiretime = vInt;
		mib_get_s(MIB_WLAN_PREAMBLE_TYPE, (void *)&vChar, sizeof(vChar));
		pmib->dot11RFEntry.shortpreamble = vChar;
		vChar = Entry.hidessid;
		pmib->dot11OperationEntry.hiddenAP = vChar;
		mib_get_s(MIB_WLAN_DTIM_PERIOD, (void *)&vChar, sizeof(vChar));
		pmib->dot11StationConfigEntry.dot11DTIMPeriod = vChar;

#ifdef HS2_SUPPORT
		mib_get(MIB_WLAN_HS2_ENABLED, (void *)&vChar);
		pmib->hs2Entry.hs_enable = vChar;
#endif

#ifdef WLAN_ACL
		set_wlan_acl(pmib);
#endif

#ifdef SBWC
		setup_sbwc(ifname);
#endif

#ifdef WLAN_MBSSID
		setup_wlan_block();
#endif

#ifdef WLAN_WDS
		setupWDS(pmib);
#endif
	} // root
#ifdef WLAN_UNIVERSAL_REPEATER
	if(vwlan_idx == WLAN_REPEATER_ITF_INDEX && !rpt_enabled){}
#endif
	{
		//auth
		setupWLan_dot11_auth(pmib, vwlan_idx);
#ifdef WLAN_WPA
		setupWLan_WPA(pmib, vwlan_idx);
#endif
		setupWLan_802_1x(pmib, vwlan_idx);
	}
#ifdef WLAN_UNIVERSAL_REPEATER
	if(vwlan_idx != WLAN_REPEATER_ITF_INDEX)
#endif
	{
		mib_get_s(MIB_WLAN_RTS_THRESHOLD, (void *)&sInt, sizeof(sInt));
		pmib->dot11OperationEntry.dot11RTSThreshold = sInt;
		mib_get_s(MIB_WLAN_FRAG_THRESHOLD, (void *)&sInt, sizeof(sInt));
		pmib->dot11OperationEntry.dot11FragmentationThreshold = sInt;

		// band
		if (!mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry)) {
				printf("Error! Get MIB_MBSSIB_TBL for VAP SSID error.\n");
		}
		band = Entry.wlanBand;
#ifdef WIFI_TEST
		if (band == 4) // WiFi-G
			band = 3; // 2.4 GHz (B+G)
		else if (band == 5) // WiFi-BG
			band = 3; // 2.4 GHz (B+G)
#endif
		mib_get_s(MIB_WIFI_SUPPORT, (void*)&vChar, sizeof(vChar));
		if(vChar==1) {
			if(band == 2)
				band = 3;
		}
		if (band == 8) { //pure 11n
			mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));
			if(phyband == PHYBAND_5G) {//5G
				band += 4; // a
				vChar = 4;
			}
			else{
				band += 3;	//b+g+n
				vChar = 3;
			}
		}
		else if (band == 2) {	//pure 11g
			band += 1;	//b+g
			vChar = 1;
		}
		else if (band == 10) {	//g+n
			band += 1;	//b+g+n
			vChar = 1;
		}
		else if(band == 64) {	//pure 11ac
			band += 12; 	//a+n
			vChar = 12;
		}
		else if(band == 72) {	//n+ac
			band += 4; 	//a
			vChar = 4;
		}
		else vChar = 0;

		pmib->dot11BssType.net_work_type = band;
		pmib->dot11StationConfigEntry.legacySTADeny = vChar;
		pmib->dot11nConfigEntry.dot11nLgyEncRstrct = 15;
		pmib->dot11OperationEntry.wifi_specific = 2;
		pmib->dot11ErpInfo.ctsToSelf = 1;

		if(vwlan_idx==0){
			value[0] = Entry.rateAdaptiveEnabled;
			if (value[0] == 0) {
				pmib->dot11StationConfigEntry.autoRate = 0;
				vInt = Entry.fixedTxRate;
				pmib->dot11StationConfigEntry.fixedTxRate = vInt;
			}
			else
				pmib->dot11StationConfigEntry.autoRate = 1;
			pmib->dot11OperationEntry.block_relay = Entry.userisolation;

#ifdef WIFI_TEST
			band = Entry.wlanBand;
			if (band == 4 || band == 5) {// WiFi-G or WiFi-BG
				pmib->dot11OperationEntry.block_relay = 0;
				pmib->dot11OperationEntry.wifi_specific = 1;
			}
#endif
			//jim do wifi test tricky,
			//    1 for wifi logo test,
			//    0 for normal case...
			mib_get_s(MIB_WIFI_SUPPORT, (void*)&vChar, sizeof(vChar));
			if(vChar==1){
				band = Entry.wlanBand;
				if (band == 2 || band == 3) {// WiFi-G or WiFi-BG
					pmib->dot11OperationEntry.block_relay = 0;
					pmib->dot11OperationEntry.wifi_specific = 1;
				}
				else {// WiFi-11N
				    printf("In MIB_WLAN_BAND = 2 or 3\n");
					pmib->dot11OperationEntry.block_relay = 0;
					pmib->dot11OperationEntry.wifi_specific = 2;
				}
			}
#ifdef WLAN_QoS
			pmib->dot11QosEntry.dot11QosEnable = Entry.wmmEnabled;
#endif
		}
		else{
			pmib->dot11StationConfigEntry.autoRate = Entry.rateAdaptiveEnabled;
			if(Entry.rateAdaptiveEnabled == 0)
				pmib->dot11StationConfigEntry.fixedTxRate = Entry.fixedTxRate;
			pmib->dot11OperationEntry.hiddenAP = Entry.hidessid;
			pmib->dot11QosEntry.dot11QosEnable = Entry.wmmEnabled;
			pmib->dot11OperationEntry.block_relay = Entry.userisolation;
		}

		mib_get_s(MIB_WLAN_PROTECTION_DISABLED, (void *)value, sizeof(value));
		pmib->dot11StationConfigEntry.protectionDisabled = value[0];

		//Channel Width
		mib_get_s(MIB_WLAN_CHANNEL_WIDTH, (void *)value, sizeof(value));
#ifdef CONFIG_TRUE
		if (value[0] == 3)//auto == 80M
			value[0] = 2;
#endif
		pmib->dot11nConfigEntry.dot11nUse40M = value[0];

		//Conntrol Sideband
		if(value[0]==0) {	//20M
			pmib->dot11nConfigEntry.dot11n2ndChOffset = 0;
		}
		else {	//40M
			mib_get_s(MIB_WLAN_CONTROL_BAND, (void *)value, sizeof(value));
			if(value[0]==0)	//upper
				pmib->dot11nConfigEntry.dot11n2ndChOffset = 1;
			else	//lower
				pmib->dot11nConfigEntry.dot11n2ndChOffset = 2;
#if defined(CONFIG_WLAN_HAL_8814AE) || defined (CONFIG_RTL_8812_SUPPORT) || defined(WLAN_DUALBAND_CONCURRENT)
			mib_get_s(MIB_WLAN_AUTO_CHAN_ENABLED, &vChar, sizeof(vChar));
			mib_get_s(MIB_WLAN_CHAN_NUM, (void *)value, sizeof(value));
			if(vChar == 0 && value[0] > 14){
				printf("!!! adjust 5G 2ndoffset for 8812 !!!\n");
				if(value[0]==36 || value[0]==44 || value[0]==52 || value[0]==60)
					pmib->dot11nConfigEntry.dot11n2ndChOffset = 2;
				else
					pmib->dot11nConfigEntry.dot11n2ndChOffset = 1;

			}
#endif
		}
		//11N Co-Existence
		mib_get_s(MIB_WLAN_11N_COEXIST, (void *)value, sizeof(value));
		pmib->dot11nConfigEntry.dot11nCoexist = value[0];

		//short GI
		mib_get_s(MIB_WLAN_SHORTGI_ENABLED, (void *)value, sizeof(value));
		pmib->dot11nConfigEntry.dot11nShortGIfor20M = value[0];
		pmib->dot11nConfigEntry.dot11nShortGIfor40M = value[0];

		//aggregation
		mib_get_s(MIB_WLAN_AGGREGATION, (void *)value, sizeof(value));
		pmib->dot11nConfigEntry.dot11nAMPDU = (value[0])? 1:0;
		pmib->dot11nConfigEntry.dot11nAMSDU = 0;

	}
#ifdef CONFIG_RTL_WAPI_SUPPORT
	if(vwlan_idx==0){
		mib_get_s(MIB_WLAN_WAPI_UCAST_REKETTYPE, (void *)&vChar, sizeof(vChar));
		pmib->wapiInfo.wapiUpdateUCastKeyType = vChar;
		if (vChar!=1) {
			mib_get_s(MIB_WLAN_WAPI_UCAST_TIME, (void *)&vInt, sizeof(vInt));
			pmib->wapiInfo.wapiUpdateUCastKeyTimeout = vInt;
			mib_get_s(MIB_WLAN_WAPI_UCAST_PACKETS, (void *)&vInt, sizeof(vInt));
			pmib->wapiInfo.wapiUpdateUCastKeyPktNum = vInt;
		}

		mib_get_s(MIB_WLAN_WAPI_MCAST_REKEYTYPE, (void *)&vChar, sizeof(vChar));
		pmib->wapiInfo.wapiUpdateMCastKeyType = vChar;
		if (vChar!=1) {
			mib_get_s(MIB_WLAN_WAPI_MCAST_TIME, (void *)&vInt, sizeof(vInt));
			pmib->wapiInfo.wapiUpdateMCastKeyTimeout = vInt;
			mib_get_s(MIB_WLAN_WAPI_MCAST_PACKETS, (void *)&vInt, sizeof(vInt));
			pmib->wapiInfo.wapiUpdateMCastKeyPktNum = vInt;
		}
	}
#endif

	//sync mib
	wrq.u.data.pointer = (caddr_t)pmib;
	wrq.u.data.length = sizeof(struct wifi_mib);
	if (ioctl(skfd, SIOCMIBSYNC, &wrq) < 0) {
		printf("Set WLAN MIB failed!\n");
		free(pmib);
		close(skfd);
		return -1;
	}
	close(skfd);

	free(pmib);
	return 0;
}
#else
// return value:
// 0  : success
// -1 : failed
int setupWLan(int vwlan_idx)
{
	char *argv[6];
	unsigned char value[34], phyband;
	char parm[64], para2[16];
	int i, vInt, autoRate, autoRateRoot;
	unsigned char intf_map=1;
	unsigned char wlan_mode;
#ifdef WLAN_RATE_PRIOR
	unsigned char rate_prior=0;
#endif
	char mode=0;
	int status=0;
	unsigned char stringbuf[MAX_SSID_LEN + 1];
	unsigned char rootSSID[MAX_SSID_LEN + 1];
	MIB_CE_MBSSIB_T Entry;
	unsigned int intVal;
#ifdef CONFIG_RTK_DEV_AP
	unsigned char telco_selected, def_gwmac[6]={0};
#endif
#ifdef CONFIG_USER_CTCAPD
	unsigned char switch_enable, qos_enable;
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
	char rpt_enabled=0;
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		checkWlanSetting();
#endif

#ifdef WLAN_RATE_PRIOR
	mib_get_s(MIB_WLAN_RATE_PRIOR, (void *)&rate_prior, sizeof(rate_prior));
#endif

	mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));

	if(!mib_chain_get(MIB_MBSSIB_TBL, WLAN_ROOT_ITF_INDEX, (void *)&Entry))
		printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");

	wlan_mode = Entry.wlanMode;
#ifdef WLAN_UNIVERSAL_REPEATER
	if(vwlan_idx == WLAN_REPEATER_ITF_INDEX)
		mib_get_s( MIB_REPEATER_ENABLED1, (void *)&rpt_enabled, sizeof(rpt_enabled));
#endif

	//root
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		argv[1] = (char*)getWlanIfName();
		argv[2] = "set_mib";

		status|=iwpriv_cmd(IWPRIV_GETMIB | IWPRIV_INT, getWlanIfName(), "set_mib", "mp_specific", MIB_WIFI_TEST);
#if defined(TRIBAND_SUPPORT)
		status|=iwpriv_cmd(IWPRIV_GETMIB | IWPRIV_INT, getWlanIfName(), "set_mib", "regdomain", MIB_HW_REG_DOMAIN);
#else
#ifdef WLAN_DUALBAND_CONCURRENT
		if(strcmp(argv[1] ,WLANIF[1]) == 0) // wlan1
			mib_get_s(MIB_HW_WLAN1_REG_DOMAIN, (void *)value, sizeof(value));
		else // wlan0
#endif
			mib_get_s(MIB_HW_REG_DOMAIN, (void *)value, sizeof(value));

		if(value[0] != 0) {
			status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "regdomain", value[0]);
		}
#endif
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

		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "led_type=" LEDTYPE);

#if defined(CONFIG_USER_WIFI_LED_SHARE_WITH_WPS)
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "led_type=61");
#endif

#ifdef WLAN_LIFETIME
		mib_get_s(MIB_WLAN_LIFETIME, (void *)&intVal, sizeof(intVal));
		status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "lifetime", intVal);
#endif

#ifdef WLAN_AIRTIME_FAIRNESS
		status|=iwpriv_cmd(IWPRIV_GETMIB | IWPRIV_INT, getWlanIfName(), "set_mib", "swqatm_en", MIB_WLAN_AIRTIME_ENABLE);
#endif

#if defined CONFIG_TRUE
#if defined(CONFIG_WLAN_HAL_8814BE) || defined(CONFIG_WLAN_HAL_8812FE)
		mib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband);
		if(phyband == PHYBAND_2G) {//2G
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "acs_type=4");
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "autoch_timeout=300");
		}
#endif
#endif

		// ssid
		// Root AP's SSID
		snprintf(parm, sizeof(parm), "ssid=%s", Entry.ssid);
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);

#ifdef CONFIG_RTK_SSID_PRIORITY//CONFIG_USER_AP_CMCC
	    status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "manual_priority", Entry.manual_priority);
#endif

#ifdef CONFIG_USER_CTCAPD
	    mib_get_s(MIB_RTL_LINK_ROAMING_SWITCH, (void *)&switch_enable,sizeof(switch_enable));
	    mib_get_s(MIB_RTL_LINK_ROAMING_QOS, (void *)&qos_enable,sizeof(qos_enable));
	    status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "roaming_switch", switch_enable);
	    status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "roaming_qos", qos_enable);
	    status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "RSSIThreshold", Entry.rtl_link_rssi_th);
	    status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "stanum", Entry.rtl_link_sta_num);
#endif

	}


#ifdef WLAN_MBSSID
	// VAP's SSID
	if (wlan_mode == AP_MODE || wlan_mode == AP_WDS_MODE
#ifdef WLAN_MESH
		|| wlan_mode ==  AP_MESH_MODE
#endif
	) {
		if(vwlan_idx >= WLAN_VAP_ITF_INDEX && vwlan_idx < WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM){
			if (!mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry)) {
  				printf("Error! Get MIB_MBSSIB_TBL for VAP SSID error.\n");
			}

 			//if ( Entry.wlanDisabled == 1 ) {
				rtk_wlan_get_ifname(wlan_idx, vwlan_idx, para2);
				snprintf(parm, sizeof(parm), "ssid=%s", Entry.ssid);
				status|=va_cmd(IWPRIV, 3, 1, para2, "set_mib", parm);
			//}
            #ifdef CONFIG_RTK_SSID_PRIORITY//CONFIG_USER_AP_CMCC
                status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "manual_priority", Entry.manual_priority);
            #endif
			
		}
		argv[1] = (char*)getWlanIfName();
	}
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
	if (vwlan_idx == WLAN_REPEATER_ITF_INDEX){
		if (rpt_enabled) {
			if (!mib_chain_get(MIB_MBSSIB_TBL, WLAN_REPEATER_ITF_INDEX, (void *)&Entry)) {
				printf("Error! Get MIB_MBSSIB_TBL for VXD SSID error.\n");
			}

			rtk_wlan_get_ifname(wlan_idx, WLAN_REPEATER_ITF_INDEX, para2);
			snprintf(parm, sizeof(parm), "ssid=%s", Entry.ssid);
			status|=va_cmd(IWPRIV, 3, 1, para2, "set_mib", parm);
		#ifdef CONFIG_RTK_SSID_PRIORITY
			status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "manual_priority", Entry.manual_priority);
		#endif
		#ifdef CONFIG_ANDLINK_SUPPORT
			//prefer_bssid
			snprintf(parm, sizeof(parm), "prefer_bssid=%02x%02x%02x%02x%02x%02x",
				Entry.prefer_bssid[0],Entry.prefer_bssid[1],Entry.prefer_bssid[2],
				Entry.prefer_bssid[3],Entry.prefer_bssid[4],Entry.prefer_bssid[5]);
			status|=va_cmd(IWPRIV, 3, 1, para2, "set_mib", parm);
		#endif

			argv[1] = (char*)getWlanIfName();
		}
	}
#endif

	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		// opmode
		if (wlan_mode == CLIENT_MODE)
			vInt = 8;	// client
		else	// 0(AP_MODE) or 3(AP_WDS_MODE)
			vInt = 16;	// AP

		status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "opmode", vInt);
	}

#ifdef WLAN_UNIVERSAL_REPEATER
	// Repeater opmode
	if (vwlan_idx == WLAN_REPEATER_ITF_INDEX){
		if (rpt_enabled) {
			if (wlan_mode == CLIENT_MODE)
				vInt = 16;	// Repeater is AP
			else
				vInt = 8;	// Repeater is Client
			rtk_wlan_get_ifname(wlan_idx, WLAN_REPEATER_ITF_INDEX, para2);
			status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "opmode", vInt);
			argv[1] = (char*)getWlanIfName();
		}
	}
#endif


	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "phyBandSelect", phyband);
	}

	status |= _rtk_wlan_set_special_fixed_settings(phyband, getWlanIfName(), vwlan_idx);


	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
#if 0//defined(TRIBAND_SUPPORT)
		status |= setupWlanHWSetting("wlan0");
#else
		status |= setupWlanHWSetting(getWlanIfName());
#endif

#if defined(WLAN0_5G_SUPPORT) || defined(WLAN1_5G_SUPPORT)
#if 0//defined(TRIBAND_SUPPORT)
	    status |= setup8812Wlan("wlan0");
#else
		status |= setup8812Wlan(getWlanIfName());
#endif
#endif

#ifndef CONFIG_USER_WIRELESS_MP_MODE
#ifdef CONFIG_RF_DPK_SETTING_SUPPORT
		status |= setupWlanDPK();
#endif
#endif
	}


	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		// bcnint
		mib_get_s(MIB_WLAN_BEACON_INTERVAL, (void *)value, sizeof(value));
		vInt = (int)(*(unsigned short *)value);
		snprintf(parm, sizeof(parm), "bcnint=%u", vInt);
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);

		// channel
		mib_get_s(MIB_WLAN_AUTO_CHAN_ENABLED, (void *)value, sizeof(value));
		if(value[0]){
#if defined(CONFIG_ANDLINK_SUPPORT)
			char rtl_link_mode;
			mib_get(MIB_RTL_LINK_MODE, (void *)&rtl_link_mode);
			if((phyband == PHYBAND_2G && rtl_link_mode==1)){
				vInt = rtl_link_get_channel((char *)getWlanIfName());
				rtl_link_set_channel((char *)getWlanIfName(), vInt);
			}else
#endif
			{
#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
				if(_wlIdxRadioDoChange != 0)
#endif
					status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "channel=0");

				*((unsigned short*)(value)) = 0;
				mib_get_s(MIB_WLAN_AUTO_CHAN_TIMEOUT, (void *)value, sizeof(value));
				snprintf(parm, sizeof(parm), "autoch_timeout=%u", *((unsigned short*)(value)));
				status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);

				*((unsigned short*)(value)) = 0;
				mib_get_s(MIB_WLAN_AUTO_CHAN_DIFFPKT, (void *)value, sizeof(value));
				snprintf(parm, sizeof(parm), "autoch_diff_pkt=%u", *((unsigned short*)(value)));
				status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);

				*((unsigned char*)(value)) = 0;
				mib_get_s(MIB_WLAN_AUTO_CHAN_PRIMARY, (void *)value, sizeof(value));

				if(phyband == PHYBAND_5G)
				{
					snprintf(parm, sizeof(parm), "autoch_3664157_enable=%d", (*((unsigned char*)(value)))?1:0);
					status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
				}
				else
				{
					snprintf(parm, sizeof(parm), "autoch_1611_enable=%d", (*((unsigned char*)(value)))?1:0);
					status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
				}
			}
		}
		else
		{
#if defined(CONFIG_00R0) && (defined(WLAN0_5G_SUPPORT) || defined(WLAN1_5G_SUPPORT))
			if(phyband==PHYBAND_5G)
				status|= setupWLan_5G_channel(getWlanIfName());
			else
#endif
			status|=iwpriv_cmd(IWPRIV_GETMIB|IWPRIV_INT, getWlanIfName(), "set_mib", "channel", MIB_WLAN_CHAN_NUM);
#ifdef CONFIG_RTK_DEV_AP
#ifdef WLAN_UNIVERSAL_REPEATER
			if (rpt_enabled) {
				char channel_num;
				mib_get_s(MIB_WLAN_CHAN_NUM, (void *)&channel_num,sizeof(channel_num)); 
				snprintf(para2, sizeof(para2), "%s-vxd", getWlanIfName());
				snprintf(parm, sizeof(parm), "channel=%d", channel_num);
				status|=va_cmd(IWPRIV, 3, 1, para2, "set_mib", parm);			
			}
#endif
#endif

			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "autoch_timeout=0");
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "autoch_diff_pkt=0");
			if(phyband == PHYBAND_5G)
				status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "autoch_3664157_enable=0");
			else
				status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "autoch_1611_enable=0");
		}
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
		if(phyband==PHYBAND_5G)
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "dfs_acs_offset=10000");
		//For yueme channel scan score timeout
		va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "ss_score_timeout=60");
#endif
	}

	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
#ifdef WIFI_TEST
		if(!mib_chain_get(MIB_MBSSIB_TBL, WLAN_ROOT_ITF_INDEX, (void *)&Entry))
			printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");
		value[0] = Entry.wlanBand;
		if (value[0] == 4){ // WiFi-G
			snprintf(parm, sizeof(parm), "basicrates=%u", 0x15f);
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
		}
		else if (value[0] == 5){ // WiFi-BG
			snprintf(parm, sizeof(parm), "basicrates=%u", 0x00f);
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
		}
		else {
#endif
		//jim do wifi test tricky,
		//    1 for wifi logo test,
		//    0 for normal case...
		mib_get_s(MIB_WIFI_SUPPORT, (void*)value, sizeof(value));
		if(value[0]==1){
			if(!mib_chain_get(MIB_MBSSIB_TBL, WLAN_ROOT_ITF_INDEX, (void *)&Entry))
				printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");
			value[0] = Entry.wlanBand;
			if (value[0] == 2) {// WiFi-G
				snprintf(parm, sizeof(parm), "basicrates=%u", 0x15f);
				status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
			}
			else if (value[0] == 3){// WiFi-BG
				snprintf(parm, sizeof(parm), "basicrates=%u", 0x00f);
				status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
			}
			else {
				mib_get_s(MIB_WLAN_BASIC_RATE, (void *)value, sizeof(value));
				vInt = (int)(*(unsigned short *)value);
				status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "basicrates", vInt);
			}
		}
		else{
			// basicrates
			mib_get_s(MIB_WLAN_BASIC_RATE, (void *)value, sizeof(value));
			vInt = (int)(*(unsigned short *)value);
			status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "basicrates", vInt);
		}
#ifdef WIFI_TEST
		}
#endif

#ifdef WIFI_TEST
		if(!mib_chain_get(MIB_MBSSIB_TBL, WLAN_ROOT_ITF_INDEX, (void *)&Entry))
			printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");
		value[0] = Entry.wlanBand;
		if (value[0] == 4) {// WiFi-G
			snprintf(parm, sizeof(parm), "oprates=%u", 0xfff);
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
		}
		else if (value[0] == 5) {// WiFi-BG
			snprintf(parm, sizeof(parm), "oprates=%u", 0xfff);
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
		}
		else {
#endif
		//jim do wifi test tricky,
		//    1 for wifi logo test,
		//    0 for normal case...
		mib_get_s(MIB_WIFI_SUPPORT, (void*)value, sizeof(value));
		if(value[0]==1){
			if(!mib_chain_get(MIB_MBSSIB_TBL, WLAN_ROOT_ITF_INDEX, (void *)&Entry))
				printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");
			value[0] = Entry.wlanBand;
			if (value[0] == 2) {// WiFi-G
				snprintf(parm, sizeof(parm), "oprates=%u", 0xfff);
				status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
			}
			else if (value[0] == 3) {// WiFi-BG
				snprintf(parm, sizeof(parm), "oprates=%u", 0xfff);
				status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
			}
			else {
				mib_get_s(MIB_WLAN_SUPPORTED_RATE, (void *)value, sizeof(value));
				vInt = (int)(*(unsigned short *)value);
				status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "oprates", vInt);
			}
		}
		else{
			// oprates
			mib_get_s(MIB_WLAN_SUPPORTED_RATE, (void *)value, sizeof(value));
			vInt = (int)(*(unsigned short *)value);
			status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "oprates", vInt);
		}
#ifdef WIFI_TEST
		}
#endif
	}

	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		if(!mib_chain_get(MIB_MBSSIB_TBL, WLAN_ROOT_ITF_INDEX, (void *)&Entry))
			printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");
		// autorate
		autoRateRoot = Entry.rateAdaptiveEnabled;
		snprintf(parm, sizeof(parm), "autorate=%u", autoRateRoot);
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
	}

	// rtsthres
	mib_get_s(MIB_WLAN_RTS_THRESHOLD, (void *)value, sizeof(value));
	vInt = (int)(*(unsigned short *)value);
	snprintf(parm, sizeof(parm), "rtsthres=%u", vInt);
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
	set_vap_para_by_index("set_mib", parm, vwlan_idx);

	// fragthres
	mib_get_s(MIB_WLAN_FRAG_THRESHOLD, (void *)value, sizeof(value));
	vInt = (int)(*(unsigned short *)value);
	snprintf(parm, sizeof(parm), "fragthres=%u", vInt);
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
	set_vap_para_by_index("set_mib", parm, vwlan_idx);

	// dtimperiod
	mib_get_s(MIB_WLAN_DTIM_PERIOD, (void *)value, sizeof(value));
	snprintf(parm, sizeof(parm), "dtimperiod=%u", value[0]);
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
	set_vap_para_by_index("set_mib", parm, vwlan_idx);

	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		// expired_time
		mib_get_s(MIB_WLAN_INACTIVITY_TIME, (void *)value, sizeof(value));
		vInt = (int)(*(unsigned long *)value);
		snprintf(parm, sizeof(parm), "expired_time=%u", vInt);
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);

		// preamble
		status|=iwpriv_cmd(IWPRIV_GETMIB|IWPRIV_INT, getWlanIfName(), "set_mib", "preamble", MIB_WLAN_PREAMBLE_TYPE);

		// hiddenAP
		snprintf(parm, sizeof(parm), "hiddenAP=%u", Entry.hidessid);
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);

#ifdef WLAN_TX_BEAMFORMING
		status|=iwpriv_cmd(IWPRIV_INT, (char *)getWlanIfName(), "set_mib", "txbf", Entry.txbf);
		if(Entry.txbf)
			status|=iwpriv_cmd(IWPRIV_INT, (char *)getWlanIfName(), "set_mib", "txbf_mu", Entry.txbf_mu);
		else
			status|=iwpriv_cmd(IWPRIV_INT, (char *)getWlanIfName(), "set_mib", "txbf_mu", 0);
#endif
		status|=iwpriv_cmd(IWPRIV_INT, (char *)getWlanIfName(), "set_mib", "mc2u_disable", Entry.mc2u_disable);

#ifdef WLAN_INTF_TXBF_DISABLE
		//txbf must disable if enable antenna diversity
		if(wlan_idx == WLAN_INTF_TXBF_DISABLE){
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "txbf=0");
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "txbf_mu=0");
		}
#endif
	}

#ifdef WLAN_ACL
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
#ifdef WLAN_VAP_ACL
		status|=set_wlan_acl(vwlan_idx);
#else
		rtk_wlan_get_ifname(wlan_idx, WLAN_ROOT_ITF_INDEX, parm);
		status|=set_wlan_acl(parm);
#endif
	}
#ifdef WLAN_MBSSID
	if (wlan_mode == AP_MODE || wlan_mode == AP_WDS_MODE
#ifdef WLAN_MESH
		|| wlan_mode ==  AP_MESH_MODE
#endif
	) {
		if(vwlan_idx >= WLAN_VAP_ITF_INDEX && vwlan_idx < WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM){
#ifdef WLAN_VAP_ACL
			status|=set_wlan_acl(vwlan_idx);
#else
			rtk_wlan_get_ifname(wlan_idx, vwlan_idx, para2);
			status|=set_wlan_acl(para2);
#endif
		}
	}
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
	if(vwlan_idx == WLAN_REPEATER_ITF_INDEX){
		if (wlan_mode != WDS_MODE) {
			if (rpt_enabled){
#ifdef WLAN_VAP_ACL
				status|=set_wlan_acl(vwlan_idx);
#else
				rtk_wlan_get_ifname(wlan_idx, WLAN_REPEATER_ITF_INDEX, para2);
				status|=set_wlan_acl(para2);
#endif
			}
		}
	}
#endif
#endif

#ifdef WLAN_BAND_STEERING
	if(vwlan_idx == 0) //only for ssid1
	{
		rtk_wlan_set_band_steering();
#ifdef WLAN_VAP_BAND_STEERING
		rtk_wlan_vap_set_band_steering();
#endif
	}
#endif

#ifdef SBWC
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		status|=setup_sbwc(getWlanIfName());
#endif

#if 1 //def WLAN_MBSSID
	setup_wlan_block();
#endif

#ifdef CONFIG_GUEST_ACCESS_SUPPORT
	//guest access
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		setup_wlan_guestaccess(WLAN_ROOT_ITF_INDEX);  //root
#ifdef WLAN_MBSSID
	if(vwlan_idx >= WLAN_VAP_ITF_INDEX && vwlan_idx < WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM)
		setup_wlan_guestaccess(vwlan_idx);  //vap
#endif
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	if(vwlan_idx >= 0 && vwlan_idx < WLAN_SSID_NUM){
		rtk_wlan_get_ifname(wlan_idx, vwlan_idx, parm);
		if (mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry)) {
			setup_wlan_accessRule_netfilter(parm, &Entry);
		}
	}
#endif

	// authtype
	// Root AP's authtype
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		setupWLan_dot11_auth(WLAN_ROOT_ITF_INDEX);

#ifdef WLAN_MBSSID
	// VAP
	if(vwlan_idx >= WLAN_VAP_ITF_INDEX && vwlan_idx < WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM) {
		setupWLan_dot11_auth(vwlan_idx);
		if (!mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry)) {
  			printf("Error! Get MIB_MBSSIB_TBL for VAP SSID error.\n");
		}

		rtk_wlan_get_ifname(wlan_idx, vwlan_idx, para2);
		argv[1] = para2;

		//for wifi-vap band
		value[0] = Entry.wlanBand;
#ifdef WIFI_TEST
		if (value[0] == 4) // WiFi-G
			value[0] = 3; // 2.4 GHz (B+G)
		else if (value[0] == 5) // WiFi-BG
			value[0] = 3; // 2.4 GHz (B+G)
#endif
		unsigned char vChar;
		mib_get_s(MIB_WIFI_SUPPORT, (void*)&vChar, sizeof(vChar));
		if(vChar==1) {
			if(value[0] == 2)
				value[0] = 3;
		}
#ifdef WLAN_RATE_PRIOR
		if(rate_prior == 0){
#endif
			if (value[0] == 8) { //pure 11n
				//mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));
				if(phyband == PHYBAND_5G) {//5G
					value[0] += 4; // a
					vChar = 4;
				}
				else{
					value[0] += 3;	//b+g+n
					vChar = 3;
				}
			}
			else if (value[0] == 2) {	//pure 11g
				value[0] += 1;	//b+g
				vChar = 1;
			}
			else if (value[0] == 10) {	//g+n
				value[0] += 1;	//b+g+n
				vChar = 1;
			}
			else if(value[0] == 64) {	//pure 11ac
				value[0] += 12; 	//a+n
				vChar = 12;
			}
			else if(value[0] == 72) {	//n+ac
				value[0] += 4; 	//a
				vChar = 4;
			}
			else vChar = 0;
#ifdef WLAN_RATE_PRIOR
		}
		else{
			if(phyband == PHYBAND_5G) {//5G
				value[0] = 76; //pure 11ac
				vChar = 12;
			}
			else{
				value[0] = 11;	//pure 11n
				vChar = 3;
			}
		}
#endif

		status |= iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "band", value[0]); //802.11b:1, 802.11g:2, 802.11n:8

		// for wifi-vap autorate
		value[0] = Entry.rateAdaptiveEnabled;
		autoRate = (int)value[0];
		status |=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "autorate", autoRate);
		if (autoRate == 0) // for wifi-vap fixrate
			status |=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "fixrate", Entry.fixedTxRate);

		//for wifi-vap hidden AP
		status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "hiddenAP", Entry.hidessid);

		// for wifi-vap WMM
		status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "qos_enable", Entry.wmmEnabled);

		//for wifi-vap max block_relay
		status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "block_relay", Entry.userisolation);

		//deny legacy
		status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "deny_legacy", vChar);

#if defined(CONFIG_FORCE_QOS_SUPPORT) && (defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME))
		status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "force_qos", 1);
		
		// enable CTC DSCP to WIFI QoS
		status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "ctc_dscp", 1);
#endif

#ifdef WLAN_LIMITED_STA_NUM
		status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "stanum", Entry.stanum);
#endif

#if 0
#ifdef WLAN_TX_BEAMFORMING
		status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "txbf", Entry.txbf);
		if(Entry.txbf)
			status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "txbf_mu", Entry.txbf_mu);
		else
			status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "txbf_mu", 0);
#endif
#ifdef WLAN_INTF_TXBF_DISABLE
		//txbf must disable if enable antenna diversity
		if(wlan_idx == WLAN_INTF_TXBF_DISABLE){
			status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "txbf", 0);
			status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "txbf_mu", 0);
		}
#endif
#endif

#if (defined(CONFIG_00R0) && defined(_CWMP_MIB_)) || (defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_CTCAPD))
		status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "func_off", Entry.func_off);
#endif
#if defined(CONFIG_00R0)
		// for tx_restrict & rx_restrict for per SSID
		int tx_bandwidth=0, rx_bandwidth=0, qbwc_mode = GBWC_MODE_DISABLE;
		tx_bandwidth = Entry.tx_restrict;
		rx_bandwidth = Entry.rx_restrict;
		if (tx_bandwidth && rx_bandwidth == 0)
			qbwc_mode = GBWC_MODE_LIMIT_IF_TX;
		else if (tx_bandwidth == 0 && rx_bandwidth)
			qbwc_mode = GBWC_MODE_LIMIT_IF_RX;
		else if (tx_bandwidth && rx_bandwidth)
			qbwc_mode = GBWC_MODE_LIMIT_IF_TRX;
		iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "gbwcmode", qbwc_mode);
		iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "gbwcthrd_tx", Entry.tx_restrict);
		iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "gbwcthrd_rx", Entry.rx_restrict);
#endif

	}
	argv[1] = (char*)getWlanIfName();
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
	if (vwlan_idx == WLAN_REPEATER_ITF_INDEX){
		if (rpt_enabled) {
			setupWLan_dot11_auth(WLAN_REPEATER_ITF_INDEX);
		}
	}
#endif // of WLAN_UNIVERSAL_REPEATER

#if defined WLAN_QoS && (!defined (CONFIG_RTL8192CD) && !defined(CONFIG_RTL8192CD_MODULE))
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		argv[1] = (char*)getWlanIfName();
		argv[2] = "set_mib";
		status|=setupWLanQos(argv);
	}
#endif

#ifdef WLAN_ROAMING
	if(vwlan_idx == 0) //for ssid1 only
		setupWLan_Roaming(vwlan_idx);
#endif

#ifdef WLAN_WPA

	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		setupWLan_WPA(WLAN_ROOT_ITF_INDEX); // Root
#ifdef WLAN_11R
		setupWLan_FT(WLAN_ROOT_ITF_INDEX);
#endif
#ifdef WLAN_11K
		setupWLan_dot11K(WLAN_ROOT_ITF_INDEX);
#endif
#ifdef WLAN_11V
		setupWLan_dot11V(WLAN_ROOT_ITF_INDEX);
#endif
#ifdef RTK_MULTI_AP
	setupWLan_multi_ap(WLAN_ROOT_ITF_INDEX);
#endif
	}

#ifdef WLAN_MBSSID
	// encmode
	if(vwlan_idx >= WLAN_VAP_ITF_INDEX && vwlan_idx < WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM) {
		setupWLan_WPA(vwlan_idx);
#ifdef WLAN_11R
		setupWLan_FT(vwlan_idx);
#endif
#ifdef WLAN_11K
		setupWLan_dot11K(vwlan_idx);
#endif
#ifdef WLAN_11V
		setupWLan_dot11V(vwlan_idx);
#endif
#ifdef RTK_MULTI_AP
		setupWLan_multi_ap(vwlan_idx);
#endif
	}
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
	if(vwlan_idx == WLAN_REPEATER_ITF_INDEX){
		if (rpt_enabled){
			setupWLan_WPA(WLAN_REPEATER_ITF_INDEX);
#ifdef RTK_MULTI_AP
		setupWLan_multi_ap(WLAN_REPEATER_ITF_INDEX);
#endif
		}
	}
#endif

	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		// Set 802.1x flag
		setupWLan_802_1x(WLAN_ROOT_ITF_INDEX); // Root
	}

#ifdef WLAN_MBSSID
	// Set 802.1x flag
	if(vwlan_idx >= WLAN_VAP_ITF_INDEX && vwlan_idx < WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM)
		setupWLan_802_1x(vwlan_idx); // VAP
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
	if (vwlan_idx == WLAN_REPEATER_ITF_INDEX){
		if (rpt_enabled)
			setupWLan_802_1x(WLAN_REPEATER_ITF_INDEX); // Repeater
	}
#endif

#endif // of WLAN_WPA

	if(!mib_chain_get(MIB_MBSSIB_TBL, WLAN_ROOT_ITF_INDEX, (void *)&Entry))
		printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");
	// band
	value [0] = Entry.wlanBand;
#ifdef WIFI_TEST
	if (value[0] == 4) // WiFi-G
		value[0] = 3; // 2.4 GHz (B+G)
	else if (value[0] == 5) // WiFi-BG
		value[0] = 3; // 2.4 GHz (B+G)
#endif
	//jim do wifi test tricky,
	//    1 for wifi logo test,
	//    0 for normal case...
	unsigned char vChar;
	mib_get_s(MIB_WIFI_SUPPORT, (void*)&vChar, sizeof(vChar));
	if(vChar==1){
		if(value[0] == 2)
			value[0] = 3;
	}

#ifdef WLAN_RATE_PRIOR
	if(rate_prior == 0){
#endif
		if(value[0] == 8) {	//pure 11n
			//mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));
			if(phyband == PHYBAND_5G) {//5G
				value[0] += 4; // a
				vChar = 4;
			}
			else{
				value[0] += 3;	//b+g+n
				vChar = 3;
			}
		}
		else if(value[0] == 2) {	//pure 11g
			value[0] += 1;	//b+g
			vChar = 1;
		}
		else if(value[0] == 10) {	//g+n
			value[0] += 1; 	//b+g+n
			vChar = 1;
		}
		else if(value[0] == 64) {	//pure 11ac
			value[0] += 12; 	//a+n
			vChar = 12;
		}
		else if(value[0] == 72) {	//n+ac
			value[0] += 4; 	//a
			vChar = 4;
		}
		else vChar = 0;
#ifdef WLAN_RATE_PRIOR
	}
	else{
		if(phyband == PHYBAND_5G) {//5G
			value[0] = 76; //pure 11ac
			vChar = 12;
		}
		else{
			value[0] = 11;	//pure 11n
			vChar = 3;
		}
	}
#endif

	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "band", value[0]); //802.11b:1, 802.11g:2, 802.11n:8
#ifdef WLAN_UNIVERSAL_REPEATER
	if(vwlan_idx == WLAN_REPEATER_ITF_INDEX){
		rtk_wlan_get_ifname(wlan_idx, WLAN_REPEATER_ITF_INDEX, para2);
		status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "band", value[0]); // Repeater
	}
#endif

	//deny legacy
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		status|=iwpriv_cmd(IWPRIV_INT, getWlanIfName(), "set_mib", "deny_legacy", vChar);

#ifdef WLAN_UNIVERSAL_REPEATER
	if(vwlan_idx == WLAN_REPEATER_ITF_INDEX){
		rtk_wlan_get_ifname(wlan_idx, WLAN_REPEATER_ITF_INDEX, para2);
		status|=iwpriv_cmd(IWPRIV_INT, para2, "set_mib", "deny_legacy", vChar);
	}
#endif

#ifdef CONFIG_CMCC_ENTERPRISE
	mib_get_s(MIB_WIFI_DENY_LEGACY2, (void*)&vChar, sizeof(vChar));
	if(vChar==1){
		va_cmd(IWPRIV, 3, 1, "wlan0", "set_mib", "deny_legacy=3");
	}
	mib_get_s(MIB_WIFI_DENY_LEGACY5, (void*)&vChar, sizeof(vChar));
	if(vChar==1){
		va_cmd(IWPRIV, 3, 1, "wlan1", "set_mib", "deny_legacy=3");
	}
#endif
	// For TKIP g mode issue (WiFi Cert 4.2.44: Disallow TKIP with HT Rates Test). Added by Annie, 2010-06-29.
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "lgyEncRstrct=15");

	set_vap_para_by_index("set_mib","lgyEncRstrct=15", vwlan_idx);

	// For WiFi Test Plan. Added by Annie, 2010-06-29.
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "wifi_specific=2");

	set_vap_para_by_index("set_mib","wifi_specific=2", vwlan_idx);

	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		if (autoRateRoot == 0)
		{
			// fixrate
			snprintf(parm, sizeof(parm), "fixrate=%u", Entry.fixedTxRate);
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
		}
	}

#ifdef WLAN_WDS
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		setupWDS();
#endif

//12/23/04' hrchen, these MIBs are for CLIENT mode, disable them
#if 0
	//12/16/04' hrchen, disable nat25_disable
	// nat25_disable
	value[0] = 0;
	sprintf(parm, "nat25_disable=%u", value[0]);
	argv[3] = parm;
	argv[4] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
	status|=do_cmd(IWPRIV, argv, 1);

	//12/16/04' hrchen, disable macclone_enable
	// macclone_enable
	value[0] = 0;
	sprintf(parm, "macclone_enable=%u", value[0]);
	argv[3] = parm;
	argv[4] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
	status|=do_cmd(IWPRIV, argv, 1);
#endif

#if 0
	//12/16/04' hrchen, debug flag
	// debug_err
	sprintf(parm, "debug_err=ffffffff");
	argv[3] = parm;
	argv[4] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
	status|=do_cmd(IWPRIV, argv, 1);
	// debug_info
	sprintf(parm, "debug_info=0");
	argv[3] = parm;
	argv[4] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
	status|=do_cmd(IWPRIV, argv, 1);
	// debug_warn
	sprintf(parm, "debug_warn=0");
	argv[3] = parm;
	argv[4] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
	status|=do_cmd(IWPRIV, argv, 1);
	// debug_trace
	sprintf(parm, "debug_trace=0");
	argv[3] = parm;
	argv[4] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
	status|=do_cmd(IWPRIV, argv, 1);
#endif

	//12/16/04' hrchen, for protection mode
	// cts2self
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "cts2self=1");

	//12/16/04' hrchen, set 11g protection mode
	// disable_protection
	mib_get_s(MIB_WLAN_PROTECTION_DISABLED, (void *)value, sizeof(value));
	snprintf(parm, sizeof(parm), "disable_protection=%u", value[0]);
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
	set_vap_para_by_index("set_mib", parm, vwlan_idx);

	//12/16/04' hrchen, chipVersion
	// chipVersion
	/*
	sprintf(parm, "chipVersion=0");
	argv[3] = parm;
	argv[4] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
	status|=do_cmd(IWPRIV, argv, 1);
	*/

#if 0	// not necessary for AP
	//12/16/04' hrchen, defssid
	// defssid
	sprintf(parm, "defssid=\"defaultSSID\"");
	argv[3] = parm;
	argv[4] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s %s\n", IWPRIV, argv[1], argv[2], argv[3]);
	status|=do_cmd(IWPRIV, argv, 1);
#endif

	//12/16/04' hrchen, set block relay
	// block_relay
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		snprintf(parm, sizeof(parm), "block_relay=%u", Entry.userisolation);
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
	}

#ifdef WIFI_TEST
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		value[0] = Entry.wlanBand;
		if (value[0] == 4 || value[0] == 5) {// WiFi-G or WiFi-BG
			// block_relay=0
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "block_relay=0");
			// wifi_specific=1
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "wifi_specific=1");
		}
	}
#endif
	//jim do wifi test tricky,
	//    1 for wifi logo test,
	//    0 for normal case...
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		mib_get_s(MIB_WIFI_SUPPORT, (void*)value, sizeof(value));
		if(value[0]==1){
			value[0] = Entry.wlanBand;
			if (value[0] == 2 || value[0] == 3) {// WiFi-G or WiFi-BG
				// block_relay=0
				status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "block_relay=0");
				// wifi_specific=1
				status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "wifi_specific=1");
			}
			else {// WiFi-11N
			    printf("In MIB_WLAN_BAND = 2 or 3\n");
				status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "block_relay=0");
				// wifi_specific=2
				status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "wifi_specific=2");
			}
		}
	}

#ifdef WLAN_QoS
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		value[0] = Entry.wmmEnabled;
		if(value[0]==0){
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "qos_enable=0");
			//set_vap_para("set_mib","qos_enable=0");
		}
		else if(value[0]==1){
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "qos_enable=1");
			//set_vap_para("set_mib","qos_enable=1");
		}

		//for wmm power saving
		mib_get_s(MIB_WLAN_APSD_ENABLE, (void *)value, sizeof(value));
		if(value[0]==0)
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "apsd_enable=0");
		else if(value[0]==1)
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "apsd_enable=1");

#if defined(CONFIG_FORCE_QOS_SUPPORT) && (defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME))
		va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "force_qos=1");
#endif
	}
#endif

#ifdef WLAN_LIMITED_STA_NUM
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		snprintf(parm, sizeof(parm), "stanum=%d", Entry.stanum);
		va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
	}
#endif
#if (defined(CONFIG_00R0) && defined(_CWMP_MIB_)) || (defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_CTCAPD))
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		snprintf(parm, sizeof(parm), "func_off=%d", Entry.func_off);
		va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
	}
#endif

#ifdef WLAN_RATE_PRIOR
	if(rate_prior==0){
#endif
		//Channel Width
		mib_get_s(MIB_WLAN_CHANNEL_WIDTH, (void *)value, sizeof(value));
		if(value[0]==0)	// 20MHZ
		{
			if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
				va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "use40M=0");
			set_vap_para_by_index("set_mib", "use40M=0", vwlan_idx);
		}
		else if(value[0]==1)	// 40MHZ
		{
			if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
				va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "use40M=1");
			set_vap_para_by_index("set_mib", "use40M=1", vwlan_idx);
		}
		else	// 80MHZ
		{
			if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
				va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "use40M=2");
			set_vap_para_by_index("set_mib", "use40M=2", vwlan_idx);
		}
#ifdef WLAN_RATE_PRIOR
	}
	else{
		mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));
		if(phyband == PHYBAND_5G) {//80Mz
			if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
				va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "use40M=2");
			set_vap_para_by_index("set_mib", "use40M=2", vwlan_idx);
		}
		else{
			if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
				va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "use40M=1");
			set_vap_para_by_index("set_mib", "use40M=1", vwlan_idx);
		}
	}
#endif

	//Conntrol Sideband
	if(value[0]==0) {	//20M
		if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "2ndchoffset=0");
		set_vap_para_by_index("set_mib", "2ndchoffset=0", vwlan_idx);
	}
	else {	//40M
		mib_get_s(MIB_WLAN_CONTROL_BAND, (void *)value, sizeof(value));
		if(value[0]==0){	//upper
			if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
				va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "2ndchoffset=1");
			set_vap_para_by_index("set_mib", "2ndchoffset=1", vwlan_idx);
		}
		else{		//lower
			if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
				va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "2ndchoffset=2");
			set_vap_para_by_index("set_mib", "2ndchoffset=2", vwlan_idx);
		}
#if defined(CONFIG_WLAN_HAL_8814AE) || defined (CONFIG_RTL_8812_SUPPORT) || defined(WLAN_DUALBAND_CONCURRENT)
		mib_get_s(MIB_WLAN_AUTO_CHAN_ENABLED, &vChar, sizeof(vChar));
		mib_get_s(MIB_WLAN_CHAN_NUM, (void *)value, sizeof(value));
		if(vChar == 0 && value[0] > 14)
		{
			printf("!!! adjust 5G 2ndoffset for 8812 !!!\n");
			if(value[0]==36 || value[0]==44 || value[0]==52 || value[0]==60){
				if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
					va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "2ndchoffset=2");
				set_vap_para_by_index("set_mib", "2ndchoffset=2", vwlan_idx);
			}
			else{
				if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
					va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "2ndchoffset=1");
				set_vap_para_by_index("set_mib", "2ndchoffset=1", vwlan_idx);
			}
		}
#endif
	}
#if defined(CONFIG_00R0) && defined(WLAN_11N_COEXIST)
	mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
	if(vChar == PHYBAND_2G)
	{
#endif
#ifdef WLAN_RATE_PRIOR
		if(rate_prior==0){
#endif
			//11N Co-Existence
			mib_get_s(MIB_WLAN_11N_COEXIST, (void *)value, sizeof(value));
			if(value[0]==0)
			{
				if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
					va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "coexist=0");
				set_vap_para_by_index("set_mib", "coexist=0", vwlan_idx);
			}
			else
			{
				if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
					va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "coexist=1");
				set_vap_para_by_index("set_mib", "coexist=1", vwlan_idx);

			}
#ifdef WLAN_RATE_PRIOR
		}
		else{
			if(phyband == PHYBAND_2G) {
				if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
					va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "coexist=1");
				set_vap_para_by_index("set_mib", "coexist=1", vwlan_idx);
			}
		}
#endif
#if defined(CONFIG_00R0) && defined(WLAN_11N_COEXIST)
	}
#endif
	//short GI
	mib_get_s(MIB_WLAN_SHORTGI_ENABLED, (void *)value, sizeof(value));
	if(value[0]==0) {
		if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "shortGI20M=0");
		set_vap_para_by_index("set_mib", "shortGI20M=0", vwlan_idx);
		if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "shortGI40M=0");
		set_vap_para_by_index("set_mib", "shortGI40M=0", vwlan_idx);
		if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "shortGI80M=0");
		set_vap_para_by_index("set_mib", "shortGI80M=0", vwlan_idx);
#ifdef WLAN_UNIVERSAL_REPEATER
		if(vwlan_idx == WLAN_REPEATER_ITF_INDEX){
			rtk_wlan_get_ifname(wlan_idx, WLAN_REPEATER_ITF_INDEX, para2);
			va_cmd(IWPRIV, 3, 1, para2, "set_mib", "shortGI20M=0");
			va_cmd(IWPRIV, 3, 1, para2, "set_mib", "shortGI40M=0");
			va_cmd(IWPRIV, 3, 1, para2, "set_mib", "shortGI80M=0");
		}
#endif

	}
	else {
		if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "shortGI20M=1");
		set_vap_para_by_index("set_mib", "shortGI20M=1", vwlan_idx);
		if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "shortGI40M=1");
		set_vap_para_by_index("set_mib", "shortGI40M=1", vwlan_idx);
		if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "shortGI80M=1");
		set_vap_para_by_index("set_mib", "shortGI80M=1", vwlan_idx);
#ifdef WLAN_UNIVERSAL_REPEATER
		if(vwlan_idx == WLAN_REPEATER_ITF_INDEX){
			rtk_wlan_get_ifname(wlan_idx, WLAN_REPEATER_ITF_INDEX, para2);
			va_cmd(IWPRIV, 3, 1, para2, "set_mib", "shortGI20M=1");
			va_cmd(IWPRIV, 3, 1, para2, "set_mib", "shortGI40M=1");
			va_cmd(IWPRIV, 3, 1, para2, "set_mib", "shortGI80M=1");
		}
#endif
	}

	//aggregation
	mib_get_s(MIB_WLAN_AGGREGATION, (void *)value, sizeof(value));
	if((value[0]&(1<<WLAN_AGGREGATION_AMPDU))==0) {
		if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "ampdu=0");
		set_vap_para_by_index("set_mib", "ampdu=0", vwlan_idx);
#ifdef WLAN_UNIVERSAL_REPEATER
		if(vwlan_idx == WLAN_REPEATER_ITF_INDEX){
			rtk_wlan_get_ifname(wlan_idx, WLAN_REPEATER_ITF_INDEX, para2);
			va_cmd(IWPRIV, 3, 1, para2, "set_mib", "ampdu=0");
		}
#endif
		
	}
	else {
		if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "ampdu=1");
		set_vap_para_by_index("set_mib", "ampdu=1", vwlan_idx);
#ifdef WLAN_UNIVERSAL_REPEATER
		if(vwlan_idx == WLAN_REPEATER_ITF_INDEX){
			rtk_wlan_get_ifname(wlan_idx, WLAN_REPEATER_ITF_INDEX, para2);
			va_cmd(IWPRIV, 3, 1, para2, "set_mib", "ampdu=1");
		}
#endif
	}
	
	if((value[0]&(1<<WLAN_AGGREGATION_AMSDU))==0) {
		if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "amsdu=0");
		set_vap_para_by_index("set_mib", "amsdu=0", vwlan_idx);
	}
	else{
		if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "amsdu=2");
		set_vap_para_by_index("set_mib", "amsdu=2", vwlan_idx);
	}

#if 1 //def CONFIG_E8B
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		if (wlan_mode == AP_MODE || wlan_mode == AP_WDS_MODE
#ifdef WLAN_MESH
			|| wlan_mode ==  AP_MESH_MODE
#endif
		)
#ifdef WLAN_MBSSID
			//always set vap_enable=1, then no need to down up root interface when vap enable is changed
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "vap_enable=1");
#else
			va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "vap_enable=0");
#endif
	}
#else
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		if (wlan_mode == AP_MODE || wlan_mode == AP_WDS_MODE
#ifdef WLAN_MESH
			|| wlan_mode ==  AP_MESH_MODE
#endif
		) {
			for(i = 1; i <=WLAN_MBSSID_NUM; i++){
				if (!mib_chain_get(MIB_MBSSIB_TBL, i, (void *)&Entry)) {
					printf("Error! Get MIB_MBSSIB_TBL for VAP SSID error.\n");
					continue;
				}
				if (!Entry.wlanDisabled) {
					intf_map |= (1 << i);
				}
			}
			if (intf_map!=1) {
				va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "vap_enable=1");
			}
			else {
				va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", "vap_enable=0");
			}
		}
	}
#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		mib_get_s(MIB_WLAN_WAPI_UCAST_REKETTYPE, (void *)&vChar, sizeof(vChar));
		snprintf(parm, sizeof(parm), "wapiUCastKeyType=%d", vChar);
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
		if (vChar!=1) {
			mib_get_s(MIB_WLAN_WAPI_UCAST_TIME, (void *)&vInt, sizeof(vInt));
			snprintf(parm, sizeof(parm), "wapiUCastKeyTimeout=%d", vInt);
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
			mib_get_s(MIB_WLAN_WAPI_UCAST_PACKETS, (void *)&vInt, sizeof(vInt));
			snprintf(parm, sizeof(parm), "wapiUCastKeyPktNum=%d", vInt);
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
		}


		mib_get_s(MIB_WLAN_WAPI_MCAST_REKEYTYPE, (void *)&vChar, sizeof(vChar));
		snprintf(parm, sizeof(parm), "wapiMCastKeyType=%d", vChar);
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
		if (vChar!=1) {
			mib_get_s(MIB_WLAN_WAPI_MCAST_TIME, (void *)&vInt, sizeof(vInt));
			snprintf(parm, sizeof(parm), "wapiMCastKeyTimeout=%d", vInt);
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
			mib_get_s(MIB_WLAN_WAPI_MCAST_PACKETS, (void *)&vInt, sizeof(vInt));
			snprintf(parm, sizeof(parm), "wapiMCastKeyPktNum=%d", vInt);
			status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
		}
	}
#endif
#ifdef CONFIG_RTL_DFS_SUPPORT
	mib_get_s(MIB_WLAN_PHY_BAND_SELECT, &phyband,sizeof(phyband));
	if(phyband == PHYBAND_5G)
	{
		unsigned char dfs_disable=0;
		mib_get_s(MIB_WLAN_DFS_DISABLE, &dfs_disable,sizeof(dfs_disable));
		snprintf(parm, sizeof(parm), "disable_DFS=%d", dfs_disable);
		status|=va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
#ifdef CONFIG_00R0
		if(dfs_disable)
			va_cmd(IWPRIV, 3, 1, getWlanIfName(), "set_mib", "dfsdbgmode=1");
#endif
	}
#endif

#if defined(WLAN_MESH)
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
		setupWLanMesh();
#if defined(WLAN_MESH_ACL_ENABLE)
		set_wlan_mesh_acl(getWlanIfName());
#endif
	}
#endif
#ifdef RTK_SMART_ROAMING
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		setupWLanSmartRoaming();
#endif

#ifdef RTK_CROSSBAND_REPEATER
	status|=setupWlanCrossband(getWlanIfName(), wlan_mode);
#endif

	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
	    setupWlanExtraMib();
	//root
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
    	setupWlan_TRX_Restrict(vwlan_idx);
	//vap
	if (vwlan_idx >= WLAN_VAP_ITF_INDEX && vwlan_idx < WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM) {
		setupWlan_TRX_Restrict(vwlan_idx);
	}
	
#ifdef CONFIG_RTK_DEV_AP
	mib_get(MIB_TELCO_SELECTED, (void *)&telco_selected);
	snprintf(parm, sizeof(parm), "telco_selected=%d", telco_selected);
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		status |= va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
	//set_vap_para("set_mib", parm);
	set_vap_para_by_index("set_mib", parm, vwlan_idx);

	mib_get(MIB_DEF_GW_MAC, (void *)&def_gwmac);
	snprintf(parm, sizeof(parm), "dfgw_mac=%02x%02x%02x%02x%02x%02x", def_gwmac[0], def_gwmac[1], def_gwmac[2], def_gwmac[3], def_gwmac[4], def_gwmac[5]);
	if(vwlan_idx == WLAN_ROOT_ITF_INDEX)
		status |= va_cmd(IWPRIV, 3, 1, (char *)getWlanIfName(), "set_mib", parm);
	//set_vap_para("set_mib", parm);
	set_vap_para_by_index("set_mib", parm, vwlan_idx);
#endif

	return status;
}
#endif

#ifdef RTK_CROSSBAND_REPEATER
int setupWlanCrossband(char *ifname, unsigned char wlan_mode)
{
	int status=0;
	char intVal;

	mib_get_s(MIB_CROSSBAND_ENABLE, &intVal, sizeof(intVal));
	if ((wlan_mode == AP_MODE || wlan_mode == AP_MESH_MODE || wlan_mode == AP_WDS_MODE)){
		if(intVal){
			//pmib->crossBand.crossband_enable = 1; //setmib to enable crossband in driver
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "crossband_enable=1");
		}
	}
	
	return status;
}
#endif

#ifdef RTK_SMART_ROAMING
/*
 *	Send SIGUSR1 to WTP, and WTP update newest wlan.conf to AC
 *	for new, it only append on related form function,
 *		such as: formMeshSetup, formWlanSetup, formWlanMultipleAP, formWlEncrypt
 */
void update_RemoteAC_Config(void){
	int pid=0;
	FILE *file = NULL;
	char cmd[100];
	char capwapMode;
	int sleep_count=0, val=1;
	mib_get_s(MIB_CAPWAP_MODE, &capwapMode, sizeof(capwapMode));
	if(capwapMode & CAPWAP_AUTO_CONFIG_ENABLE)
	{
		//create newest wlan.conf
		setup_capwap_config();
		sprintf(cmd, "echo 1 > %s", CAPWAP_APPLY_CHANGE_NOTIFY_FILE);
		system(cmd);
		pid = findPidByName("WTP");
		if(pid > 0){
			kill(pid, SIGUSR1);
			printf("Send SIGUSR1 signal to WTP\n");

			// wait AC finish procedure
			while(val && sleep_count<=5)
			{
				sleep(1);
				sleep_count++;
				file = fopen(CAPWAP_APPLY_CHANGE_NOTIFY_FILE, "r");
				char tmpbuf[10] = {0};
				if(file){
					fgets(tmpbuf,10,file);
					val = atoi(tmpbuf);
					fclose(file);
				}
			}
		}
		else
			printf("WTP cannot be found...\n");
		printf("<%s>%d: capwapMode=%d count=%d\n",__FUNCTION__,__LINE__,capwapMode,sleep_count);
	}
}

/*
 *	Create Smart Roaming System configure script,
 *	you can change env variable "reason" to add new script case
 *		reason: SYS_UPDATE�BSYS_REINIT�BSYS_UPDATE_REINTI
 */
int setup_capwap_script(void){
	FILE *fp;
	if ((fp = fopen(CAPWAP_SMART_ROAM_SCRIPT, "w")) == NULL)
	{
		printf("Open file %s failed !\n", CAPWAP_SMART_ROAM_SCRIPT);
		return -1;
	}

	fprintf(fp, "#!/bin/sh\n");
	fprintf(fp, "case \"$reason\" in\n");
	fprintf(fp, "SYS_UPDATE)\n");
	fprintf(fp, "  if [ \"$#\" -eq 1 ]; then\n");
	fprintf(fp, "    sysconf $reason $1\n");
	fprintf(fp, "  else\n");
	fprintf(fp, "    echo \"wrong script parameter.\"\n");
	fprintf(fp, "  fi\n");
	fprintf(fp, ";;\n");
	fprintf(fp, "SYS_REINIT)\n");
//	fprintf(fp, "  if [ \"$#\" -eq 0 ]; then\n");
	fprintf(fp, "    sysconf $reason\n");
//	fprintf(fp, "  fi\n");
	fprintf(fp, ";;\n");
	fprintf(fp, "SYS_UPDATE_REINIT)\n");
	fprintf(fp, "  if [ \"$#\" -eq 1 ]; then\n");
	fprintf(fp, "    sysconf $reason $1\n");
	fprintf(fp, "  else\n");
	fprintf(fp, "    echo \"wrong script parameter.\"\n");
	fprintf(fp, "  fi\n");
	fprintf(fp, ";;\n");
	fprintf(fp, "esac\n");
	fclose(fp);

	chmod(CAPWAP_SMART_ROAM_SCRIPT, 484);
	return 0;
}

/*
 *	config WiFi MIB
 */
static int setupWLanSmartRoaming(void){
	char ifname[16]={0};
	int status=0;
	char capwapMode;

	strncpy(ifname, (char*)getWlanIfName(), 16);
	mib_get_s(MIB_CAPWAP_MODE, &capwapMode, sizeof(capwapMode));

	if(capwapMode & CAPWAP_ROAMING_ENABLE){
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "sr_enable=1");
	}else{
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "sr_enable=0");
	}

	return status;
}

void setup_capwap_config(void){
	FILE *fp=NULL;
	MIB_CE_MBSSIB_T mbssid_entry;
	int i=0, vwlan_idx=0;
	unsigned char buff[2048]={0};
	unsigned char vChar;
	char ifname[IFNAMSIZ];
#ifdef WLAN_DUALBAND_CONCURRENT
	int orig_wlan_idx = wlan_idx;
#endif

//	va_cmd("/bin/mkdir", 2, 1, "-p", CAPWAP_APP_VAR_DIR);	//remote to /etc/rc
	if(!(fp = fopen(CAPWAP_APP_WLAN_CONFIG, "w"))){
		printf("open %s file fail\n", CAPWAP_APP_WLAN_CONFIG);
		return;
	}

	for(i = 0; i<NUM_WLAN_INTERFACE; i++) {
		wlan_idx = i;
		vwlan_idx = 0;
#ifdef WLAN_MBSSID
		for (vwlan_idx=0; vwlan_idx<=NUM_VWLAN_INTERFACE; vwlan_idx++) 
#endif
		{
			if(!wlan_getEntry(&mbssid_entry, vwlan_idx)){
				continue;
			}

			if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
				//root entry
				fprintf(fp, "#####################\n");
				rtk_wlan_get_ifname(i, vwlan_idx, ifname);
				fprintf(fp, "INTERFACE=%s\n", ifname);
				fprintf(fp, "#####################\n");

				if(mib_get_s( MIB_WLAN_AUTO_CHAN_ENABLED, (void *)&vChar, sizeof(vChar))){
					if(vChar==1)
						fprintf(fp, "WLAN_CHANNEL=0\n");
					else
					if(mib_get_s( MIB_WLAN_CHAN_NUM, (void *)&vChar, sizeof(vChar)))
						fprintf(fp, "WLAN_CHANNEL=%d\n", vChar);
				}
				if(mib_get_s( MIB_TX_POWER, (void *)&vChar, sizeof(vChar)))
					fprintf(fp, "WLAN_RFPOWER_SCALE=%d\n", vChar);
				if(mib_get_s( MIB_WLAN_MESH_ENABLE, (void *)&vChar, sizeof(vChar)))
					fprintf(fp, "WLAN_MESH_ENABLE=%d\n", vChar);
				if(mib_get_s( MIB_WLAN_MESH_ID, (void *)buff, sizeof(buff)))
					fprintf(fp, "WLAN_MESH_ID=%s\n", buff);
				if(mib_get_s( MIB_WLAN_MESH_ENCRYPT, (void *)&vChar, sizeof(vChar)))
					fprintf(fp, "WLAN_MESH_ENCRYPT=%d\n", vChar);
				if(mib_get_s( MIB_WLAN_MESH_WPA_AUTH, (void *)&vChar, sizeof(vChar)))
					fprintf(fp, "WLAN_MESH_WPA_AUTH=%d\n", vChar);
				if(mib_get_s( MIB_WLAN_MESH_WPA2_CIPHER_SUITE, (void *)&vChar, sizeof(vChar)))
					fprintf(fp, "WLAN_MESH_WPA2_CIPHER_SUITE=%d\n", vChar);
				if(mib_get_s( MIB_WLAN_MESH_PSK_FORMAT, (void *)&vChar, sizeof(vChar)))
					fprintf(fp, "WLAN_MESH_PSK_FORMAT=%d\n", vChar);
				if(mib_get_s( MIB_WLAN_MESH_WPA_PSK, (void *)buff, sizeof(buff)))
					fprintf(fp, "WLAN_MESH_WPA_PSK=%s\n", buff);
			}else{
				fprintf(fp, "#####################\n");
				rtk_wlan_get_ifname(i, vwlan_idx, ifname);
				fprintf(fp, "INTERFACE=%s\n", ifname);
				fprintf(fp, "#####################\n");
			}

			fprintf(fp, "WLAN_DISABLED=%d\n", mbssid_entry.wlanDisabled);
			fprintf(fp, "WLAN_MODE=%d\n", mbssid_entry.wlanMode);
			fprintf(fp, "WLAN_BAND=%d\n", mbssid_entry.wlanBand);
			fprintf(fp, "WLAN_SSID=%s\n", mbssid_entry.ssid);
			fprintf(fp, "WLAN_ENCRYPT=%d\n", mbssid_entry.encrypt);
			fprintf(fp, "WLAN_ENABLE_1X=%d\n", mbssid_entry.enable1X);
			fprintf(fp, "WLAN_AUTH_TYPE=%d\n", mbssid_entry.authType);
			fprintf(fp, "WLAN_WEP=%d\n", mbssid_entry.wep);
			fprintf(fp, "WLAN_WEP_KEY_TYPE=%d\n", mbssid_entry.wepKeyType);

			mib_to_string(buff, mbssid_entry.wep64Key1, BYTE5_T, sizeof(mbssid_entry.wep64Key1));
			fprintf(fp, "WLAN_WEP64_KEY1=%s\n", buff);
			mib_to_string(buff, mbssid_entry.wep64Key2, BYTE5_T, sizeof(mbssid_entry.wep64Key2));
			fprintf(fp, "WLAN_WEP64_KEY2=%s\n", buff);
			mib_to_string(buff, mbssid_entry.wep64Key3, BYTE5_T, sizeof(mbssid_entry.wep64Key3));
			fprintf(fp, "WLAN_WEP64_KEY3=%s\n", buff);
			mib_to_string(buff, mbssid_entry.wep64Key4, BYTE5_T, sizeof(mbssid_entry.wep64Key4));
			fprintf(fp, "WLAN_WEP64_KEY4=%s\n", buff);
			mib_to_string(buff, mbssid_entry.wep128Key1, BYTE13_T, sizeof(mbssid_entry.wep128Key1));
			fprintf(fp, "WLAN_WEP128_KEY1=%s\n", buff);
			mib_to_string(buff, mbssid_entry.wep128Key2, BYTE13_T, sizeof(mbssid_entry.wep128Key2));
			fprintf(fp, "WLAN_WEP128_KEY2=%s\n", buff);
			mib_to_string(buff, mbssid_entry.wep128Key3, BYTE13_T, sizeof(mbssid_entry.wep128Key3));
			fprintf(fp, "WLAN_WEP128_KEY3=%s\n", buff);
			mib_to_string(buff, mbssid_entry.wep128Key4, BYTE13_T, sizeof(mbssid_entry.wep128Key4));
			fprintf(fp, "WLAN_WEP128_KEY4=%s\n", buff);

			fprintf(fp, "WLAN_WPA_AUTH=%d\n", mbssid_entry.wpaAuth);
			fprintf(fp, "WLAN_WPA_CIPHER_SUITE=%d\n", mbssid_entry.unicastCipher);
			fprintf(fp, "WLAN_WPA2_CIPHER_SUITE=%d\n", mbssid_entry.wpa2UnicastCipher);
			fprintf(fp, "WLAN_PSK_FORMAT=%d\n", mbssid_entry.wpaPSKFormat);
			fprintf(fp, "WLAN_WPA_PSK=%s\n", mbssid_entry.wpaPSK);
			mib_to_string(buff, mbssid_entry.rsIpAddr, IA_T, sizeof(mbssid_entry.rsIpAddr));
			fprintf(fp, "WLAN_RS_IP=%s\n", buff);
			mib_to_string(buff, &mbssid_entry.rsPort, WORD_T, sizeof(mbssid_entry.rsPort));
			fprintf(fp, "WLAN_RS_PORT=%s\n", buff);
			fprintf(fp, "WLAN_RS_PASSWORD=%s\n", mbssid_entry.rsPassword);
#ifdef WLAN_11W
			fprintf(fp, "WLAN_IEEE80211W=%d\n", mbssid_entry.dotIEEE80211W);
			fprintf(fp, "WLAN_SHA256_ENABLE=%d\n", mbssid_entry.sha256);
#endif
		}
	}

	fclose(fp);
#ifdef WLAN_DUALBAND_CONCURRENT
	wlan_idx = orig_wlan_idx;
#endif

	if(!(fp = fopen(CAPWAP_APP_DHCP_CONFIG, "w"))){
		printf("open %s file fail\n", CAPWAP_APP_DHCP_CONFIG);
		return;
	}

	if(mib_get_s( MIB_DHCP_MODE, (void *)&vChar, sizeof(vChar))){
		fprintf(fp, "%d", vChar);
	}

	fclose(fp);

	if(!(fp = fopen(CAPWAP_APP_CAPWAP_CONFIG, "w"))){
		printf("open %s file fail\n", CAPWAP_APP_CAPWAP_CONFIG);
		return;
	}

	if(mib_get_s( MIB_CAPWAP_MODE, (void *)&vChar, sizeof(vChar))){
		fprintf(fp, "%d", vChar);
	}

	fclose(fp);

	setup_capwap_script();
}

void stop_capwap(void) {
	if(findPidByName("WTP")>0)
		system("killall -9 WTP >/dev/null 2>&1");
	if(findPidByName("AC")>0)
		system("killall -9 AC >/dev/null 2>&1");
//	if(findPidByName("AACWTP")>0)
//		system("killall -9 AACWTP >/dev/null 2>&1");

	unlink(CAPWAP_APP_WLAN_CONFIG);
	unlink(CAPWAP_APP_DHCP_CONFIG);
	unlink(CAPWAP_APP_CAPWAP_CONFIG);
	unlink(CAPWAP_SMART_ROAM_SCRIPT);
	unlink(CAPWAP_SR_AUTO_SYNC_CONFIG);
	unlink(CAPWAP_APPLY_CHANGE_NOTIFY_FILE);
}

void start_capwap(void) {
	stop_capwap();

	//disable daemon when both interfaces disabled
	char wlan0_disabled, wlan1_disabled;

	mib_get_s(MIB_WLAN_DISABLED, (void *)&wlan0_disabled, sizeof(wlan0_disabled));
	mib_get_s(MIB_WLAN1_DISABLED, (void *)&wlan1_disabled, sizeof(wlan1_disabled));

	if (wlan0_disabled && wlan1_disabled) {
		return;
	}

	char capwapMode;
	mib_get_s(MIB_CAPWAP_MODE, &capwapMode, sizeof(capwapMode));
	if (!capwapMode) {
		return;
	}

	if(capwapMode & CAPWAP_ROAMING_ENABLE){
		setup_capwap_config();

		// for wtp
		if (capwapMode & CAPWAP_WTP_ENABLE) {
			system("WTP "CAPWAP_APP_ETC_DIR);
			printf("WTP is running\n");
		}
	}
}
#endif

unsigned int check_wlan_module(void)
{
	unsigned char vChar;
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
	mib_local_mapping_get(MIB_WLAN_MODULE_DISABLED, 0, (void *)&vChar);
	//if you want to check wlan1 please add following mib get and do your checking
	//you should avoid to use this function for YUEME 3.0 spec since wlan0/1 can be enable separately
	//mib_local_mapping_get(MIB_WLAN_MODULE_DISABLED, 1, (void *)&vChar);
	//todo
#else
	mib_get_s(MIB_WIFI_MODULE_DISABLED, (void *)&vChar, sizeof(vChar));
#endif
	if(vChar)
		return 0;
	else
		return 1;
}


#if 0
static int rtk_wlan_get_index_by_band(unsigned char band)
{
	int i=0;
	unsigned char phyband=0;
	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		mib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband);
		if(band == phyband)
			return i;
	}
	return 0;
}
	struct mib_info mib_data;
	memset(&mib_data, 0, sizeof(struct mib_info));
	_read_mib_to_config(&mib_data);

	if (map_state) {
		_write_mib_data(&mib_data, "/var/multiap_mib.conf");
	}

#endif
// Added by Mason Yu
int stopwlan(config_wlan_target target, config_wlan_ssid ssid_index)
{
	int status = 0;
	int wirelessauthpid=0,iwcontrolpid=0, wscdpid=0, upnppid=0, run_mini_upnpd = 0;
#ifdef WLAN_11R
	int ftpid=0;
#endif
#ifdef WLAN_SMARTAPP_ENABLE
	int smart_wlanapp_pid=0;
#endif
	int AuthPid;
	int i,j, flags;
	unsigned char no_wlan;
	char s_auth_pid[32], s_auth_conf[32], s_auth_fifo[32];
#ifdef WLAN_11K
	int dot11kPid;
	char s_dot11k_pid[64];
#endif
	char s_ifname[16];
	char *argv[7];
#ifdef	WLAN_MBSSID
	MIB_CE_MBSSIB_T Entry;
	int orig_wlan_idx;
#endif
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
	char wscd_pid_name[32];
	char wscd_fifo_name[32];
#ifdef WLAN_WPS_VAP
	char wscd_conf_name[32];
#endif
#endif
	#ifdef CONFIG_IAPP_SUPPORT
	int iapppid=-1;
	#endif
	unsigned char phy_band_select;
	char brname[32] = {0};
	int ret = 0;
#ifdef RTK_CROSSBAND_REPEATER
	int crossbandpid=-1;
#endif
#if (defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(YUEME_3_0_SPEC)) && !defined(CONFIG_RTK_DEV_AP)
	int wlan_status = 0;
#endif


	// Kill iwcontrol
#if defined(CONFIG_USER_MONITORD) && (defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME))
	update_monitor_list_file("iwcontrol", 0);
#endif
	iwcontrolpid = read_pid((char*)IWCONTROLPID);
	if(iwcontrolpid > 0){
		kill(iwcontrolpid, 9);
		unlink(IWCONTROLPID);
	}

	// Kill Auth
	for(j=0;j<NUM_WLAN_INTERFACE;j++){
			mib_local_mapping_get( MIB_WLAN_PHY_BAND_SELECT, j, (void *)&phy_band_select);
			if((target == CONFIG_WLAN_2G && phy_band_select == PHYBAND_5G)
			|| (target == CONFIG_WLAN_5G && phy_band_select == PHYBAND_2G)) {
				continue;
			}
#if (defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(YUEME_3_0_SPEC)) && !defined(CONFIG_RTK_DEV_AP)
			wlan_status = getWlanStatus(j);
#endif

			for ( i=0; i<=NUM_VWLAN_INTERFACE; i++) {
				if(ssid_index != CONFIG_SSID_ALL && ssid_index != i
#if (defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(YUEME_3_0_SPEC)) && !defined(CONFIG_RTK_DEV_AP)
					&& wlan_status
#endif
					)
					continue;
				rtk_wlan_get_ifname(j, i, s_ifname);
				if (i==WLAN_ROOT_ITF_INDEX) {
					snprintf(s_auth_pid, 32, "/var/run/auth-%s.pid", (char *)s_ifname);
					snprintf(s_auth_conf, 32, "/var/config/%s.conf", (char *)s_ifname);
					snprintf(s_auth_fifo, 32, "/var/auth-%s.fifo", (char *)s_ifname);
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
#ifdef WLAN_WPS_VAP
					snprintf(wscd_pid_name, 32, "/var/run/wscd-%s.pid", (char *)s_ifname);
					snprintf(wscd_conf_name, 32, "/var/wscd-%s.conf", (char *)s_ifname);
					snprintf(wscd_fifo_name, 32, "/var/wscd-%s.fifo", (char *)s_ifname);
#endif
#endif
#ifdef WLAN_11K
					snprintf(s_dot11k_pid, 64, "/var/run/dot11k-%s.pid", (char *)s_ifname);
#endif
				}
#ifdef	WLAN_MBSSID
				if (i >= WLAN_VAP_ITF_INDEX && i < WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM) {
					snprintf(s_auth_pid, 32, "/var/run/auth-%s.pid", (char *)s_ifname);
					snprintf(s_auth_conf, 32, "/var/config/%s.conf", (char *)s_ifname);
					snprintf(s_auth_fifo, 32, "/var/auth-%s.fifo", (char *)s_ifname);
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
#ifdef WLAN_WPS_VAP
					snprintf(wscd_pid_name, 32, "/var/run/wscd-%s.pid", (char *)s_ifname);
					snprintf(wscd_conf_name, 32, "/var/wscd-%s.conf", (char *)s_ifname);
					snprintf(wscd_fifo_name, 32, "/var/wscd-%s.fifo", (char *)s_ifname);
#endif
#endif
#ifdef WLAN_11K
					snprintf(s_dot11k_pid, 64, "/var/run/dot11k-%s.pid", (char *)s_ifname);
#endif
				}
#endif //WLAN_MBSSID
#ifdef WLAN_UNIVERSAL_REPEATER
				if (i == WLAN_REPEATER_ITF_INDEX) {
					snprintf(s_auth_pid, 32, "/var/run/auth-%s.pid", (char *)s_ifname);
					snprintf(s_auth_conf, 32, "/var/config/%s.conf", (char *)s_ifname);
					snprintf(s_auth_fifo, 32, "/var/auth-%s.fifo", (char *)s_ifname);
#ifdef WLAN_11K
					snprintf(s_dot11k_pid, 64, "/var/run/dot11k-%s.pid", (char *)s_ifname);
#endif
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
#ifdef WLAN_WPS_VAP
					snprintf(wscd_pid_name, 32, "/var/run/wscd-%s.pid", (char *)s_ifname);
					snprintf(wscd_conf_name, 32, "/var/wscd-%s.conf", (char *)s_ifname);
					snprintf(wscd_fifo_name, 32, "/var/wscd-%s.fifo", (char *)s_ifname);
#else
					snprintf(wscd_pid_name, 32, "/var/run/wscd-%s.pid", (char *)s_ifname);
					snprintf(wscd_fifo_name, 32, "/var/wscd-%s.fifo",(char *)s_ifname);
#endif
#endif

				}
#endif
				AuthPid = read_pid(s_auth_pid);
				if(AuthPid > 0) {
					kill(AuthPid, 9);
					unlink(s_auth_conf);
					unlink(s_auth_pid);
					unlink(s_auth_fifo);
        		}
#ifdef WLAN_11K
				dot11kPid = read_pid(s_dot11k_pid);
				if(dot11kPid > 0)
					kill(dot11kPid, 9);
#endif
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
#ifdef WLAN_WPS_VAP
				wscdpid = read_pid(wscd_pid_name);
				if(wscdpid > 0) {
					system("/bin/echo 0 > /proc/gpio");
					kill(wscdpid, 9);
					unlink(wscd_conf_name);
					unlink(wscd_pid_name);
					unlink(wscd_fifo_name);
				}
#else
				wscdpid = read_pid(wscd_pid_name);
				if(wscdpid > 0){
					kill(wscdpid, 9);
#ifdef WLAN_UNIVERSAL_REPEATER
					if(i == WLAN_REPEATER_ITF_INDEX)
						unlink(wscd_fifo_name);
#endif
					unlink(wscd_pid_name);
				}
#endif
#endif
        	}
	}

#ifdef WLAN_SUPPORT
#ifdef WLAN_11R
	ftpid = read_pid(FT_PID);
	if(ftpid > 0) {
		kill(ftpid, 9);
		unlink(FT_PID);
		unlink(FT_CONF);
	}
#endif
#ifdef WLAN_SMARTAPP_ENABLE
	smart_wlanapp_pid = read_pid(SMART_WLANAPP_PID);
	if(smart_wlanapp_pid > 0) {
		kill(smart_wlanapp_pid, 9);
		unlink(SMART_WLANAPP_PID);
	}
#endif
#ifdef CONFIG_WIFI_SIMPLE_CONFIG // WPS
#if 1//def CONFIG_RTK_DEV_AP
	int pid=-1;
	int wsc_pid_exist = 0;
	do{
	   if(isFileExist("/var/run/wscd-wlan0.pid"))
	   {
		   pid = read_pid("/var/run/wscd-wlan0.pid");
		   if(pid != -1){
			   if (kill(pid, SIGKILL) != 0) {
			       printf("Could not kill pid '%d'", pid);
			   }
			   wsc_pid_exist = 1;
		   }
		   else
			   break;
		   unlink("/var/run/wscd-wlan0.pid");
		   sleep(1);		   
	   }
	   else if(isFileExist("/var/run/wscd-wlan1.pid"))
	   {
		   pid = read_pid("/var/run/wscd-wlan1.pid");
		   if(pid != -1){
			   if (kill(pid, SIGKILL) != 0) {
			       printf("Could not kill pid '%d'", pid);
			   }
			   wsc_pid_exist = 1;			   
		   }
		   else
			   break;
		   unlink("/var/run/wscd-wlan1.pid");
		   sleep(1);				   
	   }
	   else if(isFileExist("/var/run/wscd-wlan0-vxd.pid"))
	   {
		   pid = read_pid("/var/run/wscd-wlan0-vxd.pid");
		   if(pid != -1){
			   if (kill(pid, SIGKILL) != 0) {
			       printf("Could not kill pid '%d'", pid);
			   }
			   wsc_pid_exist = 1;
		   }
		   else
			   break;
		   unlink("/var/run/wscd-wlan0-vxd.pid");
		   sleep(1);				   
	   }	   
	   else if(isFileExist("/var/run/wscd-wlan1-vxd.pid"))
	   {
		   pid = read_pid("/var/run/wscd-wlan1-vxd.pid");
		   if(pid != -1){
			   if (kill(pid, SIGKILL) != 0) {
			       printf("Could not kill pid '%d'", pid);
			   }
			   wsc_pid_exist = 1;
		   }
		   else
			   break;
		   unlink("/var/run/wscd-wlan1-vxd.pid");
		   sleep(1);				   
	   }			 
	   else if(isFileExist("/var/run/wscd-wlan0-wlan1.pid"))
	   {
		   pid = read_pid("/var/run/wscd-wlan0-wlan1.pid");
		   if(pid != -1){
			   if (kill(pid, SIGKILL) != 0) {
			       printf("Could not kill pid '%d'", pid);
			   }
			   wsc_pid_exist = 1;
		   }
		   else
			   break;
		   unlink("/var/run/wscd-wlan0-wlan1.pid");
		   sleep(1);				   
	   }
	   else if(isFileExist("/var/run/wscd-wlan0-wlan1-c.pid"))
	   {
		   pid = read_pid("/var/run/wscd-wlan0-wlan1-c.pid");
		   if(pid != -1){
			   if (kill(pid, SIGKILL) != 0) {
			       printf("Could not kill pid '%d'", pid);
			   }
			   wsc_pid_exist = 1;
		   }
		   else
			   break;
		   unlink("/var/run/wscd-wlan0-wlan1-c.pid");
		   sleep(1);				   
	   }
	   else
		   break;
	}while(findPidByName("wscd") > 0);

#if defined(TRIBAND_WPS)
{
    DIR *dir;
    struct dirent *next;
    char filename[512];

    dir = opendir("/var/run");
    if (!dir) {
        printf("Cannot open %s", "/var/run");
    } else {
        while ((next = readdir(dir)) != NULL) {
            if (!strncmp(next->d_name, "wscd", strlen("wscd"))) {
                //printf("=====> %s:%d, next->d_name[%s]\n", __func__, __LINE__, next->d_name);
                sprintf(filename, "/var/run/%s", next->d_name);

                pid = read_pid(filename);
                if (pid > 1) {
                    if (kill(pid, SIGKILL) != 0) {
			           printf("Could not kill pid '%d'", pid);
			        }
                    wsc_pid_exist = 1;
                } else
                    break;

                unlink(filename);
                sleep(1);
            }
        }
        closedir(dir);
    }
}
#endif /* defined(TRIBAND_WPS) */
	if(wsc_pid_exist){
		system("/bin/echo 0 > /proc/gpio");	
		system("rm -f /var/wsc*.fifo");
		system("rm -f /var/wsc*.conf");
	}
#else
#ifndef WLAN_WPS_VAP
	// Kill wscd-wlan0.pid
	getWscPidName(wscd_pid_name);
	wscdpid = read_pid(wscd_pid_name);
	if(wscdpid > 0){
		system("/bin/echo 0 > /proc/gpio");
		system("rm -f /var/wsc*.fifo");
		kill(wscdpid, 9);
		unlink(wscd_pid_name);		
		//unlink(wscd_fifo_name);		
		unlink(WSCD_CONF);
	}
#endif
#endif	

	startSSDP();

#endif
#endif

#ifdef HS2_SUPPORT
{
    DIR *dir;
    struct dirent *next;
    char filename[512];

    dir = opendir("/var/run");
    if (!dir) {
        printf("Cannot open %s", "/var/run");
    } else {
        while ((next = readdir(dir)) != NULL) {
            if (!strncmp(next->d_name, "hs2", strlen("hs2"))) {
                printf("=====> %s:%d, next->d_name[%s]\n", __func__, __LINE__, next->d_name);
                sprintf(filename, "/var/run/%s", next->d_name);

                pid = read_pid(filename);
                if (pid > 1) {
                    if (kill(pid, SIGKILL) != 0) {
			            printf("Could not kill pid '%d'", pid);
			        }
                } else
                    break;

                unlink(filename);
                sleep(1);
            }
        }
        closedir(dir);
    }
	system("rm -f /var/hs2*.fifo");
}
#endif

#if defined(WLAN_MESH)
	// Kill pathsel
	int pathselpid;
#if defined(WLAN_MESH_SINGLE_IFACE)
	pathselpid = read_pid((char*)PATHSEL_PID_FILE);
	if(pathselpid > 0){
		kill(pathselpid, 9);
		unlink(PATHSEL_PID_FILE);
	}
	status|=va_cmd(IFCONFIG, 2, 1, MESH_IF, "down");
	status|=va_cmd(BRCTL, 3, 1, "delif", (char *)BRIF, MESH_IF);
#else /* !defined(WLAN_MESH_SINGLE_IFACE) */
{
    char pid_file_name[40];
    char msh_name[16];
    int i;

    for (i=0; i<NUM_WLAN_INTERFACE; i++) {
        sprintf(pid_file_name, "/var/run/pathsel-%s-msh.pid", WLANIF[i]);
        sprintf(msh_name, "%s-msh", WLANIF[i]);

    	pathselpid = read_pid(pid_file_name);
    	if(pathselpid > 0){
    		kill(pathselpid, 9);
    		unlink(pid_file_name);
    	}
    	status|=va_cmd(IFCONFIG, 2, 1, msh_name, "down");
    	status|=va_cmd(BRCTL, 3, 1, "delif", (char *)BRIF, msh_name);
    }
}
#endif /* defined(WLAN_MESH_SINGLE_IFACE) */
#endif

#ifdef CONFIG_IAPP_SUPPORT 
if(isFileExist(IAPP_PID_FILE)){
       iapppid=read_pid(IAPP_PID_FILE);
       if (iapppid > 0){
          kill(iapppid, 9);
          unlink(IAPP_PID_FILE);
       }
   }
#endif


#ifdef RTK_SMART_ROAMING
	stop_capwap();
#endif

#ifdef RTK_MULTI_AP
	rtk_stop_multi_ap_service();
#endif


#ifdef RTK_CROSSBAND_REPEATER
	if(isFileExist(CROSSBAND_PID_FILE)){
		crossbandpid=read_pid(CROSSBAND_PID_FILE);
		if(crossbandpid > 0){
			kill(crossbandpid, 9);
			unlink(CROSSBAND_PID_FILE);
		}
		unlink(CROSSBAND_PID_FILE);
	}
#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT
		system("killall aeUdpClient");
#endif // WAPI


		for(j=0;j<NUM_WLAN_INTERFACE;j++){
			mib_local_mapping_get( MIB_WLAN_PHY_BAND_SELECT, j, (void *)&phy_band_select);
			if((target == CONFIG_WLAN_2G && phy_band_select == PHYBAND_5G)
			|| (target == CONFIG_WLAN_5G && phy_band_select == PHYBAND_2G)) {
				continue;
			}
#if (defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(YUEME_3_0_SPEC)) && !defined(CONFIG_RTK_DEV_AP)
			wlan_status = getWlanStatus(j);
#endif
			for ( i=0; i<=NUM_VWLAN_INTERFACE; i++) {
				if(ssid_index != CONFIG_SSID_ALL && ssid_index != i
#if (defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(YUEME_3_0_SPEC)) && !defined(CONFIG_RTK_DEV_AP)
					&& wlan_status
#endif
					)
					continue;
				if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, j, i, (void *)&Entry) == 0)
					continue;
				rtk_wlan_get_ifname(j,i,s_ifname);
#if defined(CONFIG_CTC_SDN)
				unsigned char portType;
				if(phy_band_select == PHYBAND_5G)
					portType = OFP_PTYPE_WLAN5G;
				else
					portType = OFP_PTYPE_WLAN2G;
				if(Entry.sdn_enable && check_ovs_enable() && check_ovs_running() == 0
					&& unbind_ovs_interface_by_mibentry(portType, &Entry, i, j, 1) == 0)
				{
					printf("=====> Do unbind WLAN(%s) to SDN <=====\n", s_ifname);
				}
				else
#endif
        		if (getInFlags( s_ifname, &flags) == 1){
					if ((flags & IFF_UP)
#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
						&& (
#if (defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(YUEME_3_0_SPEC)) && !defined(CONFIG_RTK_DEV_AP)
						(wlan_status == 0) ||
#endif
						(ssid_index == CONFIG_SSID_ALL) || (_wlIdxSsidDoChange & (1 << (i+j*(NUM_VWLAN_INTERFACE+1)))))
#endif
					){
						status |= va_cmd(IFCONFIG, 2, 1, s_ifname, "down");

						strcpy(brname, BRIF);
#if defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_)
						if (ret == 0 || get_grouping_ifname_by_groupnum(Entry.itfGroup, brname, sizeof(brname)) == NULL || !getInFlags(brname, NULL))
						{
							strcpy(brname, BRIF);
						}
#endif

				#ifdef CONFIG_USER_FON //not under br0
						if(j == 0 && i == WLAN_MBSSID_NUM) continue;
				#endif
						status|=va_cmd(BRCTL, 3, 1, "delif", brname, s_ifname);
					}
				}
			}
		}
#ifdef WLAN_WISP
	int dhcp_pid;
	unsigned char value[32];
	char itf_name[IFNAMSIZ];
	for(i=0;i<NUM_WLAN_INTERFACE;i++){
		getWispWanName(itf_name, i);
		//snprintf(value, 32, "%s.%s", (char*)DHCPC_PID, "wlan0");
		snprintf(value, 32, "%s.%s", (char*)DHCPC_PID, itf_name);
		dhcp_pid = read_pid((char*)value);
		if(dhcp_pid > 0){
			kill(dhcp_pid, SIGUSR1); //dhcp new
		}
	}
#endif
#ifdef CONFIG_GUEST_ACCESS_SUPPORT
	int vwlan_idx=0;
	MIB_CE_MBSSIB_T mbssid_entry;
	char ifname[16]={0};
	for (vwlan_idx=0; vwlan_idx<=WLAN_MBSSID_NUM; vwlan_idx++) {
		if(!wlan_getEntry(&mbssid_entry, vwlan_idx)){
			continue;
		}
		if(vwlan_idx == 0)
			snprintf(ifname, sizeof(ifname), "%s", (char *)getWlanIfName());
		else
			snprintf(ifname, sizeof(ifname), "%s-" VAP_IF_ONLY, (char *)getWlanIfName(), vwlan_idx-1);
		
		if((mbssid_entry.guest_access == 0) || (mbssid_entry.userisolation == 0)){
			va_cmd(EBTABLES, 8, 1, "-D", (char *)FW_FORWARD, "-i", ifname, "-o", "eth+", "-j", "DROP");
			va_cmd(EBTABLES, 8, 1, "-D", (char *)FW_FORWARD, "-i", ifname, "-o", "wlan+", "-j", "DROP");
			//iptables -D INPUT -m physdev --physdev-in wlan1-vap0 -p tcp -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, (char *)FW_INPUT, "-m", "physdev", "--physdev-in", ifname, "-p", "tcp", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6		
			va_cmd(IP6TABLES, 10, 1, (char *)FW_DEL, (char *)FW_INPUT, "-m", "physdev", "--physdev-in", ifname, "-p", "tcp", "-j", (char *)FW_DROP);
#endif
		}
	}
#endif
#ifdef RTK_MULTI_AP
	rtk_stop_multi_ap_service();
#endif
	return status;
}

#define ConfigWlanLock "/var/run/configWlanLock"
#define LOCK_WLAN()	\
do {	\
	if ((lockfd = open(ConfigWlanLock, O_RDWR)) == -1) {	\
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

#ifdef CONFIG_USER_FON
#define FONCOOVACHILLI "/var/run/chilli.pid"

void enableFonSpot(void)
{
	int pid = 0;
	char str[20], str2[100], str3[100];
	unsigned char buffer[32];
	unsigned char devAddr[6];
	//char *argv[18];
	pid = read_pid((char *)FONCOOVACHILLI);
	if(pid > 0){
		printf( "%s: already start.\n", __FUNCTION__ );
		return;
	}
	snprintf(str, sizeof(str), "wlan0-" VAP_IF_ONLY, WLAN_MBSSID_NUM - 1);
	mib_get_s(MIB_ADSL_LAN_IP, (void *) buffer, sizeof(buffer));
	snprintf(str2, sizeof(str2), "--dns1=%s --dns2=%s", inet_ntoa(*((struct in_addr *)buffer)), inet_ntoa(*((struct in_addr *)buffer)));
	mib_get_s(MIB_ELAN_MAC_ADDR, (void *)devAddr, sizeof(devAddr));
	snprintf(str3, sizeof(str3), "--radiusnasid=%02x-%02x-%02x-%02x-%02x-%02x",
		devAddr[0], devAddr[1], devAddr[2],
		devAddr[3], devAddr[4], devAddr[5]);
	va_cmd_no_echo("/bin/chilli", 17, 0, "-c", "/var/chilli.conf", "--statip=192.168.182.0/24", str3,
		"--dhcpif", str, "--papalwaysok", "--localusers=/etc/fon/localusers", "--wwwbin=/bin/true",
		"--ipup=/bin/true", "--ipdown=/bin/true", "--conup=/bin/true", "--condown=/bin/true",
		"--pidfile", "/var/run/chilli.pid", str2, "--ipwhitelist=/tmp/whitelist.cache");
/*	va_cmd("/bin/chilli", 19, 0, "-c", "/var/chilli.conf", "--statip=192.168.182.0/24", str3,
		"--dhcpif", str, "--papalwaysok", "--localusers=/etc/fon/localusers", "--wwwbin=/bin/true",
		"--ipup=/bin/true", "--ipdown=/bin/true", "--conup=/bin/true", "--condown=/bin/true",
		"--pidfile", "/var/run/chilli.pid", str2, "--ipwhitelist=/tmp/whitelist.cache", "--debug", "--debugfacility=31");*/
	printf("fon spot service start.\n");
}

void disableFonSpot(void)
{
	int pid = 0;
	pid = read_pid((char *)FONCOOVACHILLI);
	if(pid <= 0){
		printf( "%s: already stop.\n", __FUNCTION__ );
	}
	else{
		kill(pid, 9);
		unlink(FONCOOVACHILLI);
	}
}
void startFonSpot(void)
{
	MIB_CE_MBSSIB_T Entry;
	unsigned char fon_onoffline;
	mib_get_s(MIB_FON_ONOFF, (void *)&fon_onoffline, sizeof(fon_onoffline));
	if (!mib_chain_get(MIB_MBSSIB_TBL, WLAN_MBSSID_NUM, (void *)&Entry)) {
		printf("mib get failed!\n");
		return;
	}
	if( !fon_onoffline || (fon_onoffline && Entry.wlanDisabled) )
		disableFonSpot();
	else if( fon_onoffline && !Entry.wlanDisabled)
		enableFonSpot();

}
/*
 *	Setup firewall for fon
 */
int setFonFirewall(void)
{
	// iptables -N fongw
	va_cmd(IPTABLES, 2, 1, "-N", "fongw");

	// iptables -A INPUT -i tun0 -j fongw
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_INPUT, "-i", (char *)FONIF, "-j", "fongw");

	// iptables -A fongw -m state --state RELATED,ESTABLISHED -j ACCEPT
#if defined(CONFIG_00R0)
	char ipfilter_spi_state=1;
	mib_get_s(MIB_IPFILTER_SPI_ENABLE, (void*)&ipfilter_spi_state, sizeof(ipfilter_spi_state));
	if(ipfilter_spi_state != 0)
#endif
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, "fongw",
		"-m", "state", "--state", "RELATED,ESTABLISHED", "-j", (char *)FW_ACCEPT);

	// iptables -A fongw -p tcp -m tcp --dport 3990 --syn -j ACCEPT
	va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, "fongw",
		"-p", "tcp", "-m", "tcp", "--dport", "3990", "--syn", "-j", (char *)FW_ACCEPT);
	// iptables -A fongw -p udp -m udp --dport 53 -j ACCEPT
	va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, "fongw",
		"-p", "udp", "-m", "udp", "--dport", "53", "-j", (char *)FW_ACCEPT);

	// iptables -A fongw -j DROP
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, "fongw", "-j", (char *)FW_DROP);


	// iptables -N fongw_fwd
	va_cmd(IPTABLES, 2, 1, "-N", "fongw_fwd");

	//iptables -A FORWARD -j fongw_fwd
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", "fongw_fwd");

	// iptables -A fongw_fwd -i tun0 -j ACCEPT
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, "fongw_fwd", "-i", (char *)FONIF, "-j", (char *)FW_ACCEPT);

	// iptables -A fongw_fwd -o tun0 -j ACCEPT
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, "fongw_fwd", "-o", (char *)FONIF, "-j", (char *)FW_ACCEPT);

}
#endif // of CONFIG_USER_FON

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
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	char ifname[16];
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	if(action_type==ACT_START_2G || action_type==ACT_RESTART_2G)
		get_ifname_by_ssid_index(ssid_index+1, ifname);
	else if(action_type==ACT_START_5G || action_type==ACT_RESTART_5G)
		get_ifname_by_ssid_index(ssid_index+(WLAN_SSID_NUM)+1, ifname);
	va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=0");
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
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
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
			sprintf(ssid_index_str, "vap%d (%s-%d)", ssid_index-WLAN_VAP_ITF_INDEX, band_type_str, ssid_index+1);
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
			sprintf(ssid_index_str, "vap%d (%s-%d)", ssid_index-WLAN_VAP_ITF_INDEX, band_type_str, ssid_index+1);
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

#if defined(WLAN_CTC_MULTI_AP)
// dbusproxy/reg/include/smart_proxy)common.h
#ifdef CONFIG_USER_DBUS_PROXY
#ifndef COM_CTC_SMART_APP_SYSTEMCMD_TAB
#define COM_CTC_SMART_APP_WLAN_EASYMESH_TAB 0x125
#define e_dbus_proxy_signal_wlan_easymesh_id 0x8101
#endif
#endif
void easyMeshEmitSignal(void)
{
#ifdef CONFIG_USER_DBUS_PROXY
	mib2dbus_notify_app_t notifyMsg;
	memset((char*)&notifyMsg, 0, sizeof(notifyMsg));
	notifyMsg.table_id = COM_CTC_SMART_APP_WLAN_EASYMESH_TAB;
	notifyMsg.Signal_id = e_dbus_proxy_signal_wlan_easymesh_id;
	mib_2_dbus_notify_dbus_api(&notifyMsg);
#endif
}
#endif

int config_WLAN( int action_type, config_wlan_ssid ssid_index)
{
	int lockfd, orig_wlan_idx, ret=0;
	char action_type_str[128] = {0};
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	char ifname[IFNAMSIZ] = {0};
#endif

	LOCK_WLAN();
	AUG_PRT("%s %s\n", __func__, config_WLAN_action_type_to_str(action_type, ssid_index, action_type_str));
	syslog(LOG_INFO, "%s", config_WLAN_action_type_to_str(action_type, ssid_index, action_type_str));

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
#ifdef WLAN_QTN
		startWLan_qtn();
#endif
#ifdef CONFIG_USER_FON
		startFonSpot();
#endif
		break;

	case ACT_RESTART:
		stopwlan(CONFIG_WLAN_ALL, ssid_index);
		startWLan(CONFIG_WLAN_ALL, ssid_index);
#ifdef WLAN_QTN
		stopwlan_qtn();
		startWLan_qtn();
#endif
#ifdef CONFIG_USER_FON
		startFonSpot();
#endif
		break;

	case ACT_STOP:
		stopwlan(CONFIG_WLAN_ALL, ssid_index);
#ifdef WLAN_QTN
		stopwlan_qtn();
#endif
#ifdef CONFIG_USER_FON
		disableFonSpot();
#endif
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
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	case ACT_RESTART_AND_WPS:
		stopwlan(CONFIG_WLAN_ALL, ssid_index);
		startWLan(CONFIG_WLAN_ALL, ssid_index);
		rtk_wlan_get_ifname(wlan_idx,0,ifname);
		va_cmd(WSC_DAEMON_PROG, 3, 1, "-sig_pbc",ifname);
		printf("trigger PBC to %s\n",ifname);
		break;
#ifdef WLAN_WPS_VAP
	case ACT_RESTART_WPS:
		UNLOCK_WLAN();
		restartWPS(0);
		LOCK_WLAN();
		rtk_wlan_get_ifname(wlan_idx,0,ifname);
		va_cmd(WSC_DAEMON_PROG, 3, 1, "-sig_pbc",ifname);
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
#endif
	wlan_idx = orig_wlan_idx;
#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
	mib_backup(CONFIG_MIB_ALL);
#endif

#ifdef WLAN_CTC_MULTI_AP
	easyMeshEmitSignal();
#endif

	UNLOCK_WLAN();
#if defined(CONFIG_IPV6) && defined(CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR)
	//Port/VLAN mapping need re-config WLAN interface ipv6 proc_config/policy route/ LL and global address.
	restartLanV6Server();
#endif
	return ret;
}

// Wlan configuration
#ifdef WLAN_1x
static void WRITE_WPA_FILE(int fh, unsigned char *buf)
{
	if ( write(fh, buf, strlen(buf)) != strlen(buf) ) {
		printf("Write WPA config file error!\n");
		//exit(1);
	}
}

// return value:
// 0  : success
// -1 : failed
#ifdef _PRMT_X_WLANFORISP_
static int generateWpaConf(char *outputFile, int isWds, MIB_CE_MBSSIB_T *Entry, MIB_WLANFORISP_T *wlan_isp_entry)
#else
static int generateWpaConf(char *outputFile, int isWds, MIB_CE_MBSSIB_T *Entry)
#endif
{
	int fh;
	unsigned int intVal;
	unsigned char chVal, wep, encrypt, enable1x;
	unsigned char buf1[1024], buf2[1024];
	unsigned short sintVal;

	fh = open(outputFile, O_RDWR|O_CREAT|O_TRUNC, 0666);
	if (fh == -1) {
		printf("Create WPA config file error!\n");
		return -1;
	}
	if (!isWds) {

	encrypt = Entry->encrypt;
	snprintf(buf2, sizeof(buf2), "encryption = %d\n", encrypt);
	WRITE_WPA_FILE(fh, buf2);

	strcpy(buf1, Entry->ssid);
	snprintf(buf2, sizeof(buf2), "ssid = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);

	enable1x = Entry->enable1X;
	snprintf(buf2, sizeof(buf2), "enable1x = %d\n", enable1x);
	WRITE_WPA_FILE(fh, buf2);

	//mib_get_s( MIB_WLAN_ENABLE_MAC_AUTH, (void *)&intVal, sizeof(intVal));
	snprintf(buf2, sizeof(buf2), "enableMacAuth = %d\n", 0);
	WRITE_WPA_FILE(fh, buf2);

/*
	mib_get_s( MIB_WLAN_ENABLE_SUPP_NONWPA, (void *)&intVal, sizeof(intVal));
	if (intVal)
		mib_get_s( MIB_WLAN_SUPP_NONWPA, (void *)&intVal, sizeof(intVal));
*/

	snprintf(buf2, sizeof(buf2), "supportNonWpaClient = %d\n", 0);
	WRITE_WPA_FILE(fh, buf2);

	wep = Entry->wep;
	snprintf(buf2, sizeof(buf2), "wepKey = %d\n", wep);
	WRITE_WPA_FILE(fh, buf2);

/*
	if ( encrypt==1 && enable1x ) {
		if (wep == 1) {
			mib_get_s( MIB_WLAN_WEP64_KEY1, (void *)buf1, sizeof(buf1));
			sprintf(buf2, "wepGroupKey = \"%02x%02x%02x%02x%02x\"\n", buf1[0],buf1[1],buf1[2],buf1[3],buf1[4]);
		}
		else {
			mib_get_s( MIB_WLAN_WEP128_KEY1, (void *)buf1, sizeof(buf1));
			sprintf(buf2, "wepGroupKey = \"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\"\n",
				buf1[0],buf1[1],buf1[2],buf1[3],buf1[4],
				buf1[5],buf1[6],buf1[7],buf1[8],buf1[9],
				buf1[10],buf1[11],buf1[12]);
		}
	}
	else
*/
	strcpy(buf2, "wepGroupKey = \"\"\n");
	WRITE_WPA_FILE(fh, buf2);


#ifdef WLAN_11R
	if (Entry->wpaAuth == 1 && Entry->ft_enable)
		chVal = 3;
	else
#endif
	chVal = Entry->wpaAuth;
	snprintf(buf2, sizeof(buf2), "authentication = %d\n", chVal); //1: radius 2:PSK 3:FT
	WRITE_WPA_FILE(fh, buf2);

	chVal = Entry->unicastCipher;
	snprintf(buf2, sizeof(buf2), "unicastCipher = %d\n", chVal);
	WRITE_WPA_FILE(fh, buf2);

	chVal = Entry->wpa2UnicastCipher;
	snprintf(buf2, sizeof(buf2), "wpa2UnicastCipher = %d\n", chVal);
	WRITE_WPA_FILE(fh, buf2);

/*
	mib_get_s( MIB_WLAN_WPA2_PRE_AUTH, (void *)&intVal, sizeof(intVal));
	sprintf(buf2, "enablePreAuth = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);
*/

	chVal = Entry->wpaPSKFormat;
	if (chVal==0)
		snprintf(buf2, sizeof(buf2), "usePassphrase = 1\n");
	else
		snprintf(buf2, sizeof(buf2), "usePassphrase = 0\n");
	WRITE_WPA_FILE(fh, buf2);


	strcpy(buf1, Entry->wpaPSK);
	snprintf(buf2, sizeof(buf2), "psk = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);

	mib_get_s( MIB_WLAN_WPA_GROUP_REKEY_TIME, (void *)&intVal, sizeof(intVal));
	snprintf(buf2, sizeof(buf2), "groupRekeyTime = %u\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

#if 1 // not support RADIUS

	sintVal = Entry->rsPort;
	snprintf(buf2, sizeof(buf2), "rsPort = %d\n", sintVal);
	WRITE_WPA_FILE(fh, buf2);

	*((uint32_t *)buf1) = *((uint32_t *)Entry->rsIpAddr);
	snprintf(buf2, sizeof(buf2), "rsIP = %s\n", inet_ntoa(*((struct in_addr *)buf1)));
	WRITE_WPA_FILE(fh, buf2);

	strcpy(buf1, Entry->rsPassword);
	snprintf(buf2, sizeof(buf2), "rsPassword = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);

#ifdef WLAN_RADIUS_2SET
	if( ((struct in_addr *)Entry->rs2IpAddr)->s_addr != INADDR_NONE && ((struct in_addr *)Entry->rs2IpAddr)->s_addr != 0 )
	{
		sintVal = Entry->rs2Port;
		snprintf(buf2, sizeof(buf2), "rs2Port = %d\n", sintVal);
		WRITE_WPA_FILE(fh, buf2);

		*((uint32_t *)buf1) = *((uint32_t *)Entry->rs2IpAddr);
		snprintf(buf2, sizeof(buf2), "rs2IP = %s\n", inet_ntoa(*((struct in_addr *)buf1)));
		WRITE_WPA_FILE(fh, buf2);

		strcpy(buf1, Entry->rs2Password);
		snprintf(buf2, sizeof(buf2), "rs2Password = \"%s\"\n", buf1);
		WRITE_WPA_FILE(fh, buf2);
	}
#endif

	mib_get_s( MIB_WLAN_RS_RETRY, (void *)&chVal, sizeof(chVal));
	snprintf(buf2, sizeof(buf2), "rsMaxReq = %d\n", chVal);
	WRITE_WPA_FILE(fh, buf2);

	mib_get_s( MIB_WLAN_RS_INTERVAL_TIME, (void *)&sintVal, sizeof(sintVal));
	snprintf(buf2, sizeof(buf2), "rsAWhile = %d\n", sintVal);
	WRITE_WPA_FILE(fh, buf2);

#ifdef _PRMT_X_WLANFORISP_
	snprintf(buf2, sizeof(buf2), "accountRsEnabled = %d\n", wlan_isp_entry->RadiusAccountEnable);
	WRITE_WPA_FILE(fh, buf2);
#else
	mib_get_s( MIB_WLAN_ACCOUNT_RS_ENABLED, (void *)&chVal, sizeof(chVal));
	snprintf(buf2, sizeof(buf2), "accountRsEnabled = %d\n", chVal);
	WRITE_WPA_FILE(fh, buf2);
#endif

	mib_get_s( MIB_WLAN_ACCOUNT_RS_PORT, (void *)&sintVal, sizeof(sintVal));
	snprintf(buf2, sizeof(buf2), "accountRsPort = %d\n", sintVal);
	WRITE_WPA_FILE(fh, buf2);

	mib_get_s( MIB_WLAN_ACCOUNT_RS_IP, (void *)buf1, sizeof(buf1));
	snprintf(buf2, sizeof(buf2), "accountRsIP = %s\n", inet_ntoa(*((struct in_addr *)buf1)));
	WRITE_WPA_FILE(fh, buf2);

	mib_get_s( MIB_WLAN_ACCOUNT_RS_PASSWORD, (void *)buf1, sizeof(buf1));
	snprintf(buf2, sizeof(buf2), "accountRsPassword = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);

	mib_get_s( MIB_WLAN_ACCOUNT_UPDATE_ENABLED, (void *)&chVal, sizeof(chVal));
	snprintf(buf2, sizeof(buf2), "accountRsUpdateEnabled = %d\n", chVal);
	WRITE_WPA_FILE(fh, buf2);

	mib_get_s( MIB_WLAN_ACCOUNT_UPDATE_DELAY, (void *)&sintVal, sizeof(sintVal));
	snprintf(buf2, sizeof(buf2), "accountRsUpdateTime = %d\n", sintVal);
	WRITE_WPA_FILE(fh, buf2);

	mib_get_s( MIB_WLAN_ACCOUNT_RS_RETRY, (void *)&chVal, sizeof(chVal));
	snprintf(buf2, sizeof(buf2), "accountRsMaxReq = %d\n", chVal);
	WRITE_WPA_FILE(fh, buf2);

	mib_get_s( MIB_WLAN_ACCOUNT_RS_INTERVAL_TIME, (void *)&sintVal, sizeof(sintVal));
	snprintf(buf2, sizeof(buf2), "accountRsAWhile = %d\n", sintVal);
	WRITE_WPA_FILE(fh, buf2);
#endif

#ifdef WLAN_11W
	if (encrypt == WIFI_SEC_WPA2) {
		snprintf(buf2, sizeof(buf2), "ieee80211w = %d\n", Entry->dotIEEE80211W);
		WRITE_WPA_FILE(fh, buf2);
		snprintf(buf2, sizeof(buf2), "sha256 = %d\n", Entry->sha256);
		WRITE_WPA_FILE(fh, buf2);
	}
	else {
		snprintf(buf2, sizeof(buf2), "ieee80211w = %d\n", 0);
		WRITE_WPA_FILE(fh, buf2);
		snprintf(buf2, sizeof(buf2), "sha256 = %d\n", 0);
		WRITE_WPA_FILE(fh, buf2);
	}
#endif

#ifdef _PRMT_X_WLANFORISP_
#if 1 // force config, because need userId and called ID to register
		snprintf(buf2, sizeof(buf2), "rsNasId = \"%s\"\n", wlan_isp_entry->NasID);
		WRITE_WPA_FILE(fh, buf2);

		snprintf(buf2, sizeof(buf2), "EnableUserId = %d\n", 1);
		WRITE_WPA_FILE(fh, buf2);

		snprintf(buf2, sizeof(buf2), "EnableCalledId = %d\n", 1);
		WRITE_WPA_FILE(fh, buf2);
#else
		if(strlen(wlan_isp_entry->NasID)){
			strcpy(buf1, wlan_isp_entry->NasID);
			snprintf(buf2, sizeof(buf2), "rsNasId = \"%s\"\n", buf1);
			WRITE_WPA_FILE(fh, buf2);
		}

		snprintf(buf2, sizeof(buf2), "EnableUserId = %d\n", wlan_isp_entry->EnableUserId);
		WRITE_WPA_FILE(fh, buf2);

		snprintf(buf2, sizeof(buf2), "EnableCalledId = %d\n", wlan_isp_entry->EnableCalledId);
		WRITE_WPA_FILE(fh, buf2);
#endif
#else
	snprintf(buf2, sizeof(buf2), "EnableUserId = %d\n", 1);
	WRITE_WPA_FILE(fh, buf2);
	snprintf(buf2, sizeof(buf2), "EnableCalledId = %d\n", 1);
	WRITE_WPA_FILE(fh, buf2);
#endif
	}

	else {
#if 0 // not support WDS
#endif
	}

	close(fh);

	return 0;
}

int is8021xEnabled(int vwlan_idx) {
#ifdef WLAN_1x
	MIB_CE_MBSSIB_T Entry;

	if(mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry) == 0)
		printf("\n %s %d\n", __func__, __LINE__);

	if (Entry.enable1X) {
		return 1;
	} else {
		if (Entry.encrypt >= WIFI_SEC_WPA) {
			///fprintf(stderr, "wpaAuth=%d\n", wpaAuth);
			if (WPA_AUTH_AUTO == Entry.wpaAuth)
				return 1;

		}
	}
#endif
	return 0;
}

#ifdef CONFIG_WIFI_SIMPLE_CONFIG // WPS
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
#if 1//def CONFIG_RTK_DEV_AP
void update_wps_from_file(void)
{
	FILE *fp=NULL;
	char line[200], value[100], token[40], tmpBuf[40]={0}, *ptr;
	int set_band=0, i;
	int ori_wlan_idx = wlan_idx, virtual_idx=0;
	MIB_CE_MBSSIB_T Entry, Entry1;
	int vwlan_idx[NUM_WLAN_INTERFACE]={0}, root_idx[NUM_WLAN_INTERFACE]={0};
#ifdef WLAN_MESH		
	unsigned char id[MAX_SSID_LEN];
	unsigned char enc=0, cipher=0, auth=0, psk_format=0, only_for_mesh=0, is_both_mesh=0, band, is_mesh_reg=0;
	unsigned char channel;
	unsigned char network_key[MAX_PSK_LEN+1]={0};
	unsigned char both_channel[NUM_WLAN_INTERFACE]={0}, both_enc[NUM_WLAN_INTERFACE]={0}, both_cipher[NUM_WLAN_INTERFACE]={0}, both_auth[NUM_WLAN_INTERFACE]={0}, both_psk_format[NUM_WLAN_INTERFACE]={0}, both_meshWpsMode[NUM_WLAN_INTERFACE]={0};
	unsigned char both_id[NUM_WLAN_INTERFACE][MAX_SSID_LEN]={0}, both_network_key[NUM_WLAN_INTERFACE][MAX_PSK_LEN+1]={0}, both_meshReserved[NUM_WLAN_INTERFACE][MAX_MESH_RESERVED_LEN]={0};
#endif
	fp = fopen(PARAM_TEMP_FILE, "r");
	if(!fp){
		printf("[%s]open %s fail\n", __FUNCTION__, PARAM_TEMP_FILE);
		return;
	}	
	//printf("=============================\n\n");
	memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));
	while ( fgets(line, sizeof(line), fp) ) {
		ptr = (char *)get_token(line, token);
		if (ptr == NULL)
			continue;
		if (get_value(ptr, value)==0)
			continue;
		//printf("%s=%s\n", token, value);
		if(strstr(token, "_INTERFACE")){
			for(i=0; i<NUM_WLAN_INTERFACE; ++i){
				snprintf(tmpBuf, sizeof(tmpBuf), "WLAN%d_INTERFACE", i);
				if(!strcmp(token, tmpBuf)){
					set_band |= BIT(i);				
					get_wlan_idx_by_iface(value, &root_idx[i], &vwlan_idx[i]);
				}
				#ifdef WLAN_MESH
				snprintf(tmpBuf, sizeof(tmpBuf), "IS_REG_WLAN%d_INTERFACE", i);
				if(!strcmp(token, tmpBuf)){
					is_mesh_reg = 1;
					only_for_mesh = 1;
					set_band |= BIT(i);				
					get_wlan_idx_by_iface(value, &root_idx[i], &vwlan_idx[i]);
				}

				snprintf(tmpBuf, sizeof(tmpBuf), "MESHBOTHBAND_IS_REG_WLAN%d_INTERFACE", i);
				if(!strcmp(token, tmpBuf)){
					band = 0;
					is_mesh_reg = 1;
					is_both_mesh = 1;
					set_band |= BIT(i);				
					get_wlan_idx_by_iface(value, &root_idx[i], &vwlan_idx[i]);
					band = i+1;
				}
				
				snprintf(tmpBuf, sizeof(tmpBuf), "MESHBOTHBAND_BAND%d_INTERFACE", i);
				if(!strcmp(token, tmpBuf)){
					band = 0;
					set_band |= BIT(i);
					get_wlan_idx_by_iface(value, &root_idx[i], &vwlan_idx[i]);
					is_both_mesh = 1;
					band = i+1;
				}
				#endif
			}	
		}else if(!strcmp(token, "SSID")){
			memset(Entry.ssid, 0, sizeof(Entry.ssid));
			strncpy(Entry.ssid, value, sizeof(Entry.ssid)-1);
		}else if(!strcmp(token, "WSC_SSID")){
		}else if(!strcmp(token, "AUTH_TYPE")){
			Entry.authType = atoi(value);
		}else if(!strcmp(token, "ENCRYPT")){
			Entry.encrypt = atoi(value);
		}else if(!strcmp(token, "WSC_AUTH")){
			Entry.wsc_auth = atoi(value);
		}else if(!strcmp(token, "WPA_AUTH")){
			Entry.wpaAuth = atoi(value);
		}else if(!strcmp(token, "WPA_PSK")){
			strncpy(Entry.wpaPSK, value, sizeof(Entry.wpaPSK));
		}else if(!strcmp(token, "PSK_FORMAT")){
			Entry.wpaPSKFormat = atoi(value);
		}else if(!strcmp(token, "WSC_PSK")){
			strncpy(Entry.wscPsk, value, sizeof(Entry.wscPsk));
		}else if(!strcmp(token, "WPA_CIPHER_SUITE")){
			Entry.unicastCipher = atoi(value);
		}else if(!strcmp(token, "WPA2_CIPHER_SUITE")){
			Entry.wpa2UnicastCipher = atoi(value);
		}else if(!strcmp(token, "WEP")){
			Entry.wep = atoi(value);			
		}else if(!strcmp(token, "WSC_ENC")){
			Entry.wsc_enc = atoi(value);
		}else if(!strcmp(token, "WSC_CONFIGURED")){
			Entry.wsc_configured = atoi(value);
		}else if(!strcmp(token, "WSC_CONFIGBYEXTREG")){
			//do nothing
			//printf("%s do nothing\n", token);
		}
#ifdef WLAN_MESH
		else if (!strcmp(token, "ONLY_FOR_MESH")){
			only_for_mesh  = atoi(value);
		}else if (!strcmp(token, "CHANNEL")){
			if(is_both_mesh == 1)
			{
			   if(band)
			   {
			      both_channel[band-1] = atoi(value);
			   }
			}
			else
			   channel  = atoi(value);				
		}else if (!strcmp(token, "MESH_ID")){
			if(is_both_mesh == 1)
			{
			   if(band)
			   {
			      strncpy(both_id[band-1], value, sizeof(both_id[band-1])-1);
			   }
			}
			else
			   strncpy(id, value, sizeof(id)-1);
		}else if (!strcmp(token, "MESH_ENCRYPT")){
			if(is_both_mesh == 1)
			{
			   if(band)
			   {
			      both_enc[band-1] = atoi(value);
			   }
			}
			else
			   enc = atoi(value);
		}else if (!strcmp(token, "MESH_WPA_AUTH")){
			if(is_both_mesh == 1)
			{
			   if(band)
			   {
			      both_auth[band-1] = atoi(value);
			   }
			}
			else
			   auth = atoi(value);
		}else if (!strcmp(token, "MESH_WPA2_CIPHER_SUITE")){
			if(is_both_mesh == 1)
			{
			   if(band)
			   {
			      both_cipher[band-1] = atoi(value);
			   }
			}
			else
			   cipher = atoi(value); 
		}else if (!strcmp(token, "MESH_WPA_PSK")){
			if(is_both_mesh == 1)
			{
			   if(band)
			   {
			      strncpy(both_network_key[band-1], value, sizeof(both_network_key[band-1])-1);
			   }
			}
			else
			   strncpy(network_key, value, sizeof(network_key)-1);
		}else if (!strcmp(token, "MESH_PSK_FORMAT")){
			if(is_both_mesh == 1)
			{
			   if(band)
			   {
			      both_psk_format[band-1] = atoi(value);
			   }
			}
			else
			   psk_format = atoi(value);
		}else if (!strcmp(token, "MESH_RESERVED")){
			if(is_both_mesh == 1)
			{
			   if(band)
			   {
			      strncpy(both_meshReserved[band-1], value, sizeof(both_meshReserved[band-1])-1);
			   }
			}
			else
			   strncpy(Entry.meshReserved, value, sizeof(Entry.meshReserved)-1);
		}else if (!strcmp(token, "MESH_WPS_MODE")){
			if(is_both_mesh == 1)
			{
			   if(band)
			   {
			      both_meshWpsMode[band-1] = atoi(value);
			   }
			}	
			else
			   Entry.meshWpsMode = atoi(value);
		} 
#endif 
		else
			printf("[%s %d]unkown mib:%s\n", __FUNCTION__,__LINE__, token);
	}
	fclose(fp);
	//printf("=============================\n\n");
	for(i=0; i<NUM_WLAN_INTERFACE; ++i)
	{
		if(set_band & BIT(i))
		{
			wlan_idx = root_idx[i];
			virtual_idx = vwlan_idx[i];
			memset(&Entry1, 0, sizeof(MIB_CE_MBSSIB_T));
			mib_chain_get(MIB_MBSSIB_TBL, virtual_idx, (void *)&Entry1);
#ifdef WLAN_MESH
			if(only_for_mesh == 0 && is_both_mesh == 0)
#endif
			{
			strncpy(Entry1.ssid, Entry.ssid, sizeof(Entry.ssid));
			Entry1.authType = Entry.authType;
			Entry1.encrypt = Entry.encrypt;
			if(Entry.encrypt==ENCRYPT_WEP)
				printf("[%s %d]remote ap encrypt is wep\n", __FUNCTION__,__LINE__);
			Entry1.wsc_auth = Entry.wsc_auth;
			Entry1.wpaAuth = Entry.wpaAuth;
			strncpy(Entry1.wpaPSK, Entry.wpaPSK, sizeof(Entry.wpaPSK));
			strncpy(Entry1.wscPsk, Entry.wscPsk, sizeof(Entry.wscPsk));
			Entry1.wpaPSKFormat = Entry.wpaPSKFormat;
			Entry1.unicastCipher = Entry.unicastCipher;
			Entry1.wpa2UnicastCipher = Entry.wpa2UnicastCipher;
			Entry1.wep = Entry.wep;
			Entry1.wsc_enc = Entry.wsc_enc;
#if defined(WLAN_CLIENT) && defined(WLAN_11W)
			if (Entry1.wlanMode == CLIENT_MODE) {
				Entry1.dotIEEE80211W = 1;
				Entry1.sha256 = 1;
			}
#endif
			}
			Entry1.wsc_configured = Entry.wsc_configured;			
#ifdef WLAN_MESH
            if(only_for_mesh == 1 || is_both_mesh == 1)
            {
			if(virtual_idx==0){		
				if(is_both_mesh == 1)
				{	
					if(is_mesh_reg == 1)
						Entry1.meshWpsMode = both_meshWpsMode[i];
					else
					{
						Entry1.meshWpsMode = both_meshWpsMode[i];	
						strncpy(Entry1.meshReserved, both_meshReserved[i], sizeof(both_meshReserved[i]));
					}
				} 
				else
				{
					if(is_mesh_reg == 1)
					Entry1.meshWpsMode = Entry.meshWpsMode;	
					else
					{
					Entry1.meshWpsMode = Entry.meshWpsMode;	
					strncpy(Entry1.meshReserved, Entry.meshReserved, sizeof(Entry.meshReserved));
					}
				}
			}
            }
#endif
			mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry1, virtual_idx);
			if(virtual_idx==0){
#ifdef WLAN_MESH
				if(only_for_mesh == 0 && is_both_mesh == 0)
#endif
				{
					mib_set(MIB_WLAN_SSID, Entry.ssid);
					mib_set(MIB_WLAN_ENCRYPT, &Entry.encrypt);
					mib_set(MIB_WLAN_AUTH_TYPE, &Entry.authType);
					mib_set(MIB_WSC_AUTH, &Entry.wsc_auth);
					mib_set(MIB_WLAN_WPA_AUTH, &Entry.wpaAuth);
					mib_set(MIB_WLAN_WPA_PSK_FORMAT, &Entry.wpaPSKFormat);
					mib_set(MIB_WLAN_WPA_PSK, Entry.wpaPSK);
					mib_set(MIB_WSC_PSK, Entry.wscPsk);
					mib_set(MIB_WLAN_WPA_CIPHER_SUITE, &Entry.unicastCipher);
					mib_set(MIB_WLAN_WPA2_CIPHER_SUITE, &Entry.wpa2UnicastCipher);								
					mib_set(MIB_WSC_ENC, &Entry.wsc_enc);
				}
				mib_set(MIB_WSC_CONFIGURED, &Entry.wsc_configured);
#ifdef WLAN_MESH
				if(only_for_mesh == 0 && is_both_mesh == 0)
#endif
				{
				if(Entry.encrypt == ENCRYPT_WEP)
				{
					mib_set(MIB_WLAN_WEP, &Entry.wep);
					mib_set(MIB_WLAN_WEP_DEFAULT_KEY, &Entry.wepDefaultKey);
					mib_set(MIB_WLAN_WEP64_KEY1, Entry.wep64Key1);
					mib_set(MIB_WLAN_WEP64_KEY2, Entry.wep64Key2);
					mib_set(MIB_WLAN_WEP64_KEY3, Entry.wep64Key3);
					mib_set(MIB_WLAN_WEP64_KEY4, Entry.wep64Key4);
					mib_set(MIB_WLAN_WEP128_KEY1, Entry.wep128Key1);
					mib_set(MIB_WLAN_WEP128_KEY2, Entry.wep128Key2);
					mib_set(MIB_WLAN_WEP128_KEY3, Entry.wep128Key3);
					mib_set(MIB_WLAN_WEP128_KEY4, Entry.wep128Key4);
					mib_set(MIB_WLAN_WEP_KEY_TYPE, &Entry.wepKeyType);		// wep Key Format
				}
				}
#ifdef WLAN_MESH
				if(only_for_mesh == 1 || is_both_mesh == 1)
				{

					if(is_both_mesh == 1 && is_mesh_reg == 0)
					{
						mib_set(MIB_WLAN_CHAN_NUM, (void *)&both_channel[i]);	
						mib_set(MIB_WLAN_MESH_ID, (void *)&both_id[i]);
						mib_set(MIB_WLAN_MESH_ENCRYPT, (void *)&both_enc[i]);
						mib_set(MIB_WLAN_MESH_WPA_AUTH, (void *)&both_auth[i]);
						mib_set(MIB_WLAN_MESH_WPA2_CIPHER_SUITE, (void *)&both_cipher[i]);
						mib_set(MIB_WLAN_MESH_WPA_PSK, (void *)&both_network_key[i]);
						mib_set(MIB_WLAN_MESH_PSK_FORMAT, (void *)&both_psk_format[i]);
					}
					else
					{
						if(is_mesh_reg == 0)
						{
							mib_set(MIB_WLAN_CHAN_NUM, (void *)&channel);	
							mib_set(MIB_WLAN_MESH_ID, (void *)&id);
							mib_set(MIB_WLAN_MESH_ENCRYPT, (void *)&enc);
							mib_set(MIB_WLAN_MESH_WPA_AUTH, (void *)&auth);
							mib_set(MIB_WLAN_MESH_WPA2_CIPHER_SUITE, (void *)&cipher);
							mib_set(MIB_WLAN_MESH_WPA_PSK, (void *)&network_key);
							mib_set(MIB_WLAN_MESH_PSK_FORMAT, (void *)&psk_format);					     
						}
					}
				}
#endif
			}
		}
	}
	wlan_idx = ori_wlan_idx;
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);	
	return;
}	
#endif

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
	mib_set(MIB_WLAN_BAND, &Entry.wlanBand);
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
				){
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

#define WRITE_WSC_PARAM(dst, dst_size, tmp, tmp_size, str, val, ret_err) {	\
	snprintf(tmp, tmp_size, str, val); \
	if ((dst_size-1) < strlen(tmp)) \
		ret_err = 1; \
	else { \
	memcpy(dst, tmp, strlen(tmp)); \
	dst += strlen(tmp); \
		dst_size -= strlen(tmp); \
		ret_err = 0; \
	} \
}
#define WRITE_WSC_PARAM2(dst, dst_size, tmp, tmp_size, str, idx, val, ret_err) {	\
	snprintf(tmp, tmp_size, str, idx, val); \
	if ((dst_size-1) < strlen(tmp)) \
		ret_err = 1; \
	else { \
	memcpy(dst, tmp, strlen(tmp)); \
	dst += strlen(tmp); \
		dst_size -= strlen(tmp); \
		ret_err = 0; \
	} \
}

static int write_wsc_int_config(char **buf, int *buf_size, char *token, int value){
	char line_buf[256];
	memset(line_buf, '\0', sizeof(line_buf));
	snprintf(line_buf, sizeof(line_buf), "%s = \"%d\"\n", token, value);
	if ((*buf_size - 1) < strlen(line_buf)) {
		goto ERROR;
	} else {
		memcpy(*buf, line_buf, strlen(line_buf));
		*buf += strlen(line_buf);
		*buf_size -= strlen(line_buf);
	}
	return 0;
ERROR:
	printf("write_config memcpy error when writing '%s'\n", token);
	return -1;
}

static int write_wsc_str_config(char **buf, int *buf_size, char *token, char *value){
	char line_buf[256];
	memset(line_buf, '\0', sizeof(line_buf));
	snprintf(line_buf, sizeof(line_buf), "%s = \"%s\"\n", token, value);
	if ((*buf_size - 1) < strlen(line_buf)) {
		goto ERROR;
	} else {
		memcpy(*buf, line_buf, strlen(line_buf));
		*buf += strlen(line_buf);
		*buf_size -= strlen(line_buf);
	}
	return 0;
ERROR:
	printf("write_config memcpy error when writing '%s'\n", token);
	return -1;
}

static void convert_hex_to_ascii(unsigned long code, char *out)
{
	*out++ = '0' + ((code / 10000000) % 10);  
	*out++ = '0' + ((code / 1000000) % 10);
	*out++ = '0' + ((code / 100000) % 10);
	*out++ = '0' + ((code / 10000) % 10);
	*out++ = '0' + ((code / 1000) % 10);
	*out++ = '0' + ((code / 100) % 10);
	*out++ = '0' + ((code / 10) % 10);
	*out++ = '0' + ((code / 1) % 10);
	*out = '\0';
}
static int compute_pin_checksum(unsigned long int PIN)
{
	unsigned long int accum = 0;
	int digit;
	PIN *= 10;
	accum += 3 * ((PIN / 10000000) % 10); 	
	accum += 1 * ((PIN / 1000000) % 10);
	accum += 3 * ((PIN / 100000) % 10);
	accum += 1 * ((PIN / 10000) % 10); 
	accum += 3 * ((PIN / 1000) % 10); 
	accum += 1 * ((PIN / 100) % 10); 
	accum += 3 * ((PIN / 10) % 10);
	digit = (accum % 10);
	return (10 - digit) % 10;
}
static void convert_bin_to_str(unsigned char *bin, int len, char *out, unsigned int out_size)
{
	int i;
	char tmpbuf[10];
	out[0] = '\0';
	for (i=0; i<len; i++) {
		snprintf(tmpbuf, sizeof(tmpbuf), "%02x", bin[i]);
		strncat(out, tmpbuf, (out_size-strlen(out)));
	}
}
#if 1 //def CONFIG_RTK_DEV_AP
int updateWscConf(char *in, char *out, int genpin, char *wlanif_name);
int start_wsc_deamon(char * wlan_interface, int mode, int WSC_UPNP_Enabled, char * bridge_iface) 
{	
#if defined(TRIBAND_WPS)
    char *cmd_opt[30]={0};
    char *token2=NULL;
    char wscFifoFile2[40];
#else
    char *cmd_opt[30]={0};
#endif /* defined(TRIBAND_WPS) */
    int cmd_cnt = 0;
    char tempbuf[40];
    //char * arg_buff[40];
    char arg_buff[40];
    char *token=NULL,*token1=NULL, *savestr1=NULL;
    int wps_debug=0, use_iwcontrol=1;
    char wsc_pin_local[16]={0},wsc_pin_peer[16]={0};
    FILE *fp;
    char wscFifoFile[40];
    char wscFifoFile1[40];
    char wscConfFile[40];    
    int wait_fifo=0;

    if(wlan_interface == NULL) {
        return -1;
    }
    
    memset(wscFifoFile, 0, sizeof(wscFifoFile));
    memset(wscFifoFile1, 0, sizeof(wscFifoFile1));
#if defined(TRIBAND_WPS)
    memset(wscFifoFile2, 0, sizeof(wscFifoFile2));
#endif
    memset(cmd_opt, 0x00, 16);
    cmd_cnt=0;
    wps_debug=0;
    use_iwcontrol=1;

    sprintf(arg_buff,"%s", wlan_interface);
    token = strtok_r(arg_buff," ", &savestr1);
    if(token)
        token1 = strtok_r(NULL," ", &savestr1);
#if defined(TRIBAND_WPS)
    if(token1)
        token2 = strtok_r(NULL," ", &savestr1);
#endif
    
    cmd_opt[cmd_cnt++] = "wscd";
    if(isFileExist("/var/wps/simplecfgservice.xml")==0){ //file is not exist
        if(isFileExist("/var/wps"))
			va_cmd("rm", 2, 1, "/var/wps", "-rf");
		va_cmd("mkdir", 1, 1, "/var/wps");
		va_cmd("/bin/sh", 2, 1, "-c", "/bin/cp /etc/simplecfg*.xml /var/wps");
    }
    
    if(mode == 1) /*client*/
    {
        WSC_UPNP_Enabled=0;
        cmd_opt[cmd_cnt++] = "-mode";
        cmd_opt[cmd_cnt++] = "2";
        
    }else{
        cmd_opt[cmd_cnt++] = "-start";
    }
    
    if(WSC_UPNP_Enabled==1){
		va_cmd("route", 6, 1, "del", "-net", "239.255.255.250", "netmask", "255.255.255.255", bridge_iface);
		va_cmd("route", 6, 1, "add", "-net", "239.255.255.250", "netmask", "255.255.255.255", bridge_iface);
    }
    
    sprintf(wscConfFile,"/var/wsc-%s", token);
    if(token1) {
        strcat(wscConfFile, "-");            
        strcat(wscConfFile, token1);
    }
#if defined(TRIBAND_WPS)
    if(token2) {
        strcat(wscConfFile, "-");            
        strcat(wscConfFile, token2);
    }
#endif /* defined(TRIBAND_WPS) */
    strcat(wscConfFile, ".conf");

	/* 98f use xml config */
    //RunSystemCmd(NULL_FILE, "flash", "upd-wsc-conf", "/etc/wscd.conf", wscConfFile, wlan_interface, NULL_STR);     
    updateWscConf("/etc/wscd.conf", wscConfFile, 0, wlan_interface);
    
    cmd_opt[cmd_cnt++] = "-c";
    cmd_opt[cmd_cnt++] = wscConfFile;
    
    if((token[4] == '0') 
#ifdef WPS_ONE_CLIENT_ONE_DAEMON
        || (mode == 1)
#endif
    )
        cmd_opt[cmd_cnt++] = "-w";    
#if defined(TRIBAND_WPS)
    else if (token[4] == '2')
        cmd_opt[cmd_cnt++] = "-w3";
#endif /* defined(TRIBAND_WPS) */
    else
        cmd_opt[cmd_cnt++] = "-w2";

    cmd_opt[cmd_cnt++] = token;

    if(token1) {
        if(token1[4] == '0')
            cmd_opt[cmd_cnt++] = "-w";       
#if defined(TRIBAND_WPS)
        else if (token1[4] == '2')
            cmd_opt[cmd_cnt++] = "-w3";
#endif /* defined(TRIBAND_WPS) */
        else
            cmd_opt[cmd_cnt++] = "-w2";
        cmd_opt[cmd_cnt++] = token1;    
    }
    
#if defined(TRIBAND_WPS)
    if(token2) {
        if(token2[4] == '0')
            cmd_opt[cmd_cnt++] = "-w";       
        else if(token2[4] == '2')
            cmd_opt[cmd_cnt++] = "-w3";
        else
            cmd_opt[cmd_cnt++] = "-w2";

        cmd_opt[cmd_cnt++] = token2;    
    }
#endif /* defined(TRIBAND_WPS) */

    if(wps_debug==1){
        /* when you would like to open debug, you should add define in wsc.h for debug mode enable*/
        cmd_opt[cmd_cnt++] = "-debug";
    }
    if(use_iwcontrol==1){
        if(token[4] == '0'
#ifdef WPS_ONE_CLIENT_ONE_DAEMON
            || (mode == 1)
#endif
        )
            cmd_opt[cmd_cnt++] = "-fi";
#if defined(TRIBAND_WPS)
        else if(token[4] == '2')
            cmd_opt[cmd_cnt++] = "-fi3";
#endif /* defined(TRIBAND_WPS) */
        else
            cmd_opt[cmd_cnt++] = "-fi2";
        sprintf(wscFifoFile,"/var/wscd-%s.fifo",token);        
        cmd_opt[cmd_cnt++] = wscFifoFile;

        if(token1) {
            if(token1[4] == '0')
                cmd_opt[cmd_cnt++] = "-fi";
#if defined(TRIBAND_WPS)
            else if(token1[4] == '2')
                cmd_opt[cmd_cnt++] = "-fi3";
#endif /* defined(TRIBAND_WPS) */
            else
                cmd_opt[cmd_cnt++] = "-fi2";
            sprintf(wscFifoFile1,"/var/wscd-%s.fifo",token1);        
            cmd_opt[cmd_cnt++] = wscFifoFile1;
        }

#if defined(TRIBAND_WPS)
        if(token2) {
            if(token2[4] == '0')
                cmd_opt[cmd_cnt++] = "-fi";
            else if(token2[4] == '2')
                cmd_opt[cmd_cnt++] = "-fi3";
            else
                cmd_opt[cmd_cnt++] = "-fi2";
            sprintf(wscFifoFile2,"/var/wscd-%s.fifo",token2);        
            cmd_opt[cmd_cnt++] = wscFifoFile2;
        }
#endif /* defined(TRIBAND_WPS) */
    }
    if(isFileExist("/var/wps_start_pbc")){
        cmd_opt[cmd_cnt++] = "-start_pbc";
        unlink("/var/wps_start_pbc");
    }
    if(isFileExist("/var/wps_start_pin")){
        cmd_opt[cmd_cnt++] = "-start";
        unlink("/var/wps_start_pin");
    }
    if(isFileExist("/var/wps_local_pin")){
        fp=fopen("/var/wps_local_pin", "r");
        if(fp != NULL){
            fscanf(fp, "%s", tempbuf);
            fclose(fp);
        }
        sprintf(wsc_pin_local, "%s", tempbuf);
        cmd_opt[cmd_cnt++] = "-local_pin";
        cmd_opt[cmd_cnt++] = wsc_pin_local;
        unlink("/var/wps_local_pin");
    }
    if(isFileExist("/var/wps_peer_pin")){
        fp=fopen("/var/wps_peer_pin", "r");
        if(fp != NULL){
            fscanf(fp, "%s", tempbuf);
            fclose(fp);
        }
        sprintf(wsc_pin_peer, "%s", tempbuf);
        cmd_opt[cmd_cnt++] = "-peer_pin";
        cmd_opt[cmd_cnt++] = wsc_pin_peer;
        unlink("/var/wps_peer_pin");
    }

#if defined(AVOID_DUAL_CLIENT_PBC_OVERLAPPING) && defined(WLAN_DUALBAND_CONCURRENT)
	if(token && token1 && mode==1){
		cmd_opt[cmd_cnt++] = "-prefer_band";
		cmd_opt[cmd_cnt++] = "1"; // prefer 2.4G
	}	
#endif

#if defined(TRIBAND_WPS)
    cmd_opt[cmd_cnt++] = "-is3";
    cmd_opt[cmd_cnt++] = "1";
#endif
    cmd_opt[cmd_cnt++] = "-daemon";
    
    cmd_opt[cmd_cnt++] = 0;

#if 0
	printf("===> cmd:");
    for (wps_debug=0; wps_debug<cmd_cnt;wps_debug++)
       printf("%s ", cmd_opt[wps_debug]);
	printf("\n");
#endif
	
	do_cmd(cmd_opt[0], cmd_opt, 1);

    if(use_iwcontrol) {
        wait_fifo=5;
        do{        
            if(isFileExist(wscFifoFile) && (wscFifoFile1[0] == 0 || isFileExist(wscFifoFile1)) 
			#if defined(TRIBAND_WPS)
              &&(wscFifoFile2[0] == 0 || isFileExist(wscFifoFile2))
			#endif
            )
            {
                wait_fifo=0;
            }else{
                wait_fifo--;
                sleep(1);
            }
            
        }while(wait_fifo !=0);
    }
    
    return use_iwcontrol;
}
int updateWscConf(char *in, char *out, int genpin, char *wlanif_name)
{	
	int fh = -1;
	struct stat status;
	char *buf, *ptr;
	int intVal, len;
	char wlan0_mode=0;
	char wlan1_mode=0;	 
	char is_config, is_registrar, is_wep=0, wep_key_type=0, wep_transmit_key=0;
	char tmpbuf[100+MAX_DEV_NAME_LEN], tmp1[100];
	unsigned int ptr_size = 0;
	int ret_err_write = 0;
	//int is_repeater_enabled=0;
	char isUpnpEnabled=0, wsc_method = 0, wsc_auth=0, wsc_enc=0;
	char wlan_network_type=0, wsc_manual_enabled=0, wlan_wep=0;
	char wlan_wep64_key1[100], wlan_wep64_key2[100], wlan_wep64_key3[100], wlan_wep64_key4[100];
	char wlan_wep128_key1[100], wlan_wep128_key2[100], wlan_wep128_key3[100], wlan_wep128_key4[100];
	char wlan_wpa_psk[100];
	char wlan_ssid[100], device_name[MAX_DEV_NAME_LEN], wsc_pin[100];
	char wlan_chan_num=0, wsc_config_by_ext_reg=0;
	char wlan0_wlan_disabled=0;	
	char wlan1_wlan_disabled=0;		
	char wlan0_wsc_disabled=0;
#ifdef WLAN_MESH
	char wlan0_mesh_enabled=0, wlan0_mesh_enc=0, wlan0_mesh_cipher=0, wlan0_mesh_mode=0;
	unsigned char wlan0_mesh_channel=0;
	unsigned char wlan0_meshid[100], wlan0_mesh_psk[100], wlan0_mesh_reserved[201];
#endif
#ifdef WLAN_DUALBAND_CONCURRENT	
	char wlan1_wsc_disabled=0;
#ifdef WLAN_MESH
	char wlan1_mesh_enabled=0, wlan1_mesh_enc=0, wlan1_mesh_cipher=0, wlan1_mesh_mode=0;
	unsigned char wlan1_mesh_channel=0;
	unsigned char wlan1_meshid[100], wlan1_mesh_psk[100], wlan1_mesh_reserved[201];
#endif
#endif
	char wlan0_encrypt=0;		
	char wlan0_wpa_cipher=0;
	char wlan0_wpa2_cipher=0;
	char wlan1_encrypt=0;		
	char wlan1_wpa_cipher=0;
	char wlan1_wpa2_cipher=0;
	//char band_select_5g2g;  // 0:2.4g  ; 1:5G   ; 2:both
	char *token=NULL, *token1=NULL, *savestr1=NULL;   
#if defined(WIFI_SIMPLE_CONFIG) && defined(CONFIG_APP_TR069)
	int auto_lockdown = 0, auto_lockdown1 = 0, auto_lockdown2 = 0;
	extRegInfoT extReg;
	int entryNum = 0, entryNum1 = 0, ii = 0, entryNum2 = 0;
#endif
#if defined(TRIBAND_WPS)
	char wlan2_mode=0, wlan2_wsc_disabled=0, wlan2_encrypt=0, wlan2_wpa_cipher=0, wlan2_wpa2_cipher=0;
	unsigned char wlan2_wlan_disabled=0;
	char *token2=NULL;
#ifdef WLAN_MESH
	char wlan2_mesh_enabled=0, wlan2_mesh_enc=0, wlan2_mesh_cipher=0, wlan2_mesh_mode=0, wlan2_mesh_channel=0;
	unsigned char wlan2_meshid[100], wlan2_mesh_psk[100], wlan2_mesh_reserved[201];
#endif
#endif /* defined(TRIBAND_WPS) */
	MIB_CE_MBSSIB_T Entry;
	int wlan_virtual_idx = 0, wlan_root_idx=0, is_cli_mode=0;
#if defined(WLAN_DUALBAND_CONCURRENT)
	int original_wlan_idx = wlan_idx;	
#endif
	if (wlanif_name != NULL) {
		token = strtok_r(wlanif_name," ", &savestr1);
		if(token)
			token1 = strtok_r(NULL," ", &savestr1);
    #if defined(TRIBAND_WPS)
		if(token1)
			token2 = strtok_r(NULL," ", &savestr1);
    #endif /* defined(TRIBAND_WPS) */
	} else {
		token = "wlan0";
	}
	//printf("===[%s %d]===wlanif_name:%s, file_in:%s, file_out:%s\n", __FUNCTION__,__LINE__,wlanif_name, in, out);
	//printf("===[%s %d]===token:%s, token1:%s, token2:%s\n", __FUNCTION__,__LINE__,token, token1, token2);	
	SetWlan_idx(token); //set global wlan_idx
	if(!is_root_iface(token))
	{
		memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));		
		get_wlan_idx_by_iface(token, &wlan_root_idx, &wlan_virtual_idx);
		mib_chain_get(MIB_MBSSIB_TBL, wlan_virtual_idx, (void *)&Entry);
	}
	mib_get_s(MIB_WSC_PIN, (void *)wsc_pin,sizeof(wsc_pin)); 
	mib_get_s(MIB_WSC_MANUAL_ENABLED, (void *)&wsc_manual_enabled,sizeof(wsc_manual_enabled));
	mib_get_s(MIB_DEVICE_NAME, (void *)&device_name,sizeof(device_name));
	//mib_get_s(MIB_WLAN_BAND2G5G_SELECT, (void *)&band_select_5g2g,sizeof(band_select_5g2g)); 
	mib_get_s(MIB_WLAN_NETWORK_TYPE, (void *)&wlan_network_type,sizeof(wlan_network_type));	 
	mib_get_s(MIB_WSC_REGISTRAR_ENABLED, (void *)&is_registrar,sizeof(is_registrar));	
	mib_get_s(MIB_WSC_METHOD, (void *)&wsc_method,sizeof(wsc_method));
	mib_get_s(MIB_WSC_CONFIG_BY_EXT_REG, (void *)&wsc_config_by_ext_reg,sizeof(wsc_config_by_ext_reg));
	if(wlan_virtual_idx){
		isUpnpEnabled = Entry.wsc_upnp_enabled;
		is_config = Entry.wsc_configured;
		wlan0_mode = Entry.wlanMode;
	}
	else{
		mib_get_s(MIB_WSC_UPNP_ENABLED, (void *)&isUpnpEnabled,sizeof(isUpnpEnabled));
		mib_get_s(MIB_WSC_CONFIGURED, (void *)&is_config,sizeof(is_config));
		mib_get_s(MIB_WLAN_MODE, (void *)&wlan0_mode,sizeof(wlan0_mode));
	}	
	if (genpin || !memcmp(wsc_pin, "\x0\x0\x0\x0\x0\x0\x0\x0", PIN_LEN)) {
		#include <sys/time.h>			
		struct timeval tod;
		unsigned long num;
		mib_get_s(MIB_ELAN_MAC_ADDR/*MIB_HW_NIC0_ADDR*/, (void *)&tmp1,sizeof(tmp1));			
		gettimeofday(&tod , NULL);
		tod.tv_sec += tmp1[4]+tmp1[5];		
		srand(tod.tv_sec);
		num = rand() % 10000000;
		num = num*10 + compute_pin_checksum(num);
		convert_hex_to_ascii((unsigned long)num, tmpbuf);
		mib_set(MIB_WSC_PIN, (void *)tmpbuf);  // wlan0 and wlan1 shares the same wsc pin mib 
		mib_update_all();//mib_update(CURRENT_SETTING);		
		printf("Generated PIN = %s\n", tmpbuf);
		if (genpin)
			return 0;
	}
	if (stat(in, &status) < 0) {
		printf("stat() error [%s]!\n", in);
		return -1;
	}
	buf = malloc(status.st_size+2048);
	if (buf == NULL) {
		printf("malloc() error [%d]!\n", (int)status.st_size+2048);
		return -1;		
	}
	ptr = buf;
	ptr_size = status.st_size + 2048;
	memset(ptr, '\0', ptr_size);
	if (wlan0_mode == WPS_CLIENT_MODE) {
		if (is_registrar) {
			if (!is_config)
				intVal = MODE_CLIENT_UNCONFIG_REGISTRAR;
			else
				intVal = MODE_CLIENT_CONFIG;			
		}
		else
			intVal = MODE_CLIENT_UNCONFIG;
		is_cli_mode = 1;
	}
	else {
		if (!is_config)
			intVal = MODE_AP_UNCONFIG;
		else
			intVal = MODE_AP_PROXY_REGISTRAR;
		is_cli_mode = 0;
	}
#ifdef CONFIG_RTL_P2P_SUPPORT
	/*for *CONFIG_RTL_P2P_SUPPORT*/
	if(wlan0_mode == WPS_P2P_SUPPORT_MODE){
		//printf("(%s %d)mode=p2p_support_mode\n\n", __FUNCTION__ , __LINE__);
		int intVal2;
		mib_get_s(MIB_WLAN_P2P_TYPE, (void *)&intVal2,sizeof(intVal2)); 	
		if(intVal2==P2P_DEVICE){
			if (write_wsc_int_config(&ptr, &ptr_size, "p2pmode", intVal2) < 0) goto ERROR;
			intVal = MODE_CLIENT_UNCONFIG;
		}
	}
#endif
	if (write_wsc_int_config(&ptr, &ptr_size, "mode", intVal) < 0) goto ERROR;

	if (wlan0_mode == WPS_CLIENT_MODE) {
#ifdef CONFIG_RTL8186_KLD_REPEATER
		if (wps_vxdAP_enabled)
			mib_get_s(MIB_WSC_UPNP_ENABLED, (void *)&intVal,sizeof(intVal));
		else
#endif
			intVal = 0;
	}
#ifdef CONFIG_RTL_P2P_SUPPORT
	else if(wlan0_mode == WPS_P2P_SUPPORT_MODE){
	/*when the board has two interfaces, system will issue two wscd then should consisder upnp conflict 20130927*/
		intVal = 0;
	}
#endif    
	else{
		intVal = isUpnpEnabled;
	}
	if (write_wsc_int_config(&ptr, &ptr_size, "upnp", intVal) < 0) goto ERROR;

	intVal = wsc_method;
#ifdef CONFIG_RTL_P2P_SUPPORT
	if(wlan0_mode == WPS_P2P_SUPPORT_MODE){
		intVal = ( CONFIG_METHOD_PIN | CONFIG_METHOD_PBC | CONFIG_METHOD_DISPLAY |CONFIG_METHOD_KEYPAD);
	}else
#endif	
	{		
		if (intVal == 1) //Pin+Ethernet
			intVal = (CONFIG_METHOD_ETH | CONFIG_METHOD_PIN);
		else if (intVal == 2) //PBC+Ethernet
			intVal = (CONFIG_METHOD_ETH | CONFIG_METHOD_PBC);
		if (intVal == 3) //Pin+PBC+Ethernet
			intVal = (CONFIG_METHOD_ETH | CONFIG_METHOD_PIN | CONFIG_METHOD_PBC);
	}
	if (write_wsc_int_config(&ptr, &ptr_size, "config_method", intVal) < 0) goto ERROR;

	if (wlan0_mode == WPS_CLIENT_MODE) 
	{
		if (wlan_network_type == 0)
			intVal = 1;
		else
			intVal = 2;
	}
	else
		intVal = 1;
	if (write_wsc_int_config(&ptr, &ptr_size, "connection_type", intVal) < 0) goto ERROR;
	if (write_wsc_int_config(&ptr, &ptr_size, "manual_config", wsc_manual_enabled) < 0) goto ERROR;
	if (write_wsc_str_config(&ptr, &ptr_size, "pin_code", wsc_pin) < 0) goto ERROR;

    wlan_virtual_idx = 0;
	if(token && (strstr(token, "wlan0")
#ifdef WPS_ONE_CLIENT_ONE_DAEMON
			|| is_cli_mode
#endif
	))
	{
		SetWlan_idx(token);	
		get_wlan_idx_by_iface(token, &wlan_root_idx, &wlan_virtual_idx);
	}
	else if(token1 && strstr(token1, "wlan0")) {
		SetWlan_idx(token1);
		get_wlan_idx_by_iface(token1, &wlan_root_idx, &wlan_virtual_idx);
	}
#if defined(TRIBAND_WPS)
	else if(token2 && strstr(token2, "wlan0")) {
		SetWlan_idx(token2);
		get_wlan_idx_by_iface(token2, &wlan_root_idx, &wlan_virtual_idx);
	}
#endif /* defined(TRIBAND_WPS) */
	else
		SetWlan_idx("wlan0");	
	mib_get_s(MIB_WLAN_CHAN_NUM, (void *)&wlan_chan_num,sizeof(wlan_chan_num));
	if (wlan_chan_num > 14)
		intVal = 2;
	else
		intVal = 1;
	if (write_wsc_int_config(&ptr, &ptr_size, "rf_band", intVal) < 0) goto ERROR;

	if(1)//(wlan_virtual_idx)
	{
		memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));		
		mib_chain_get(MIB_MBSSIB_TBL, wlan_virtual_idx, (void *)&Entry);
		wsc_auth = Entry.wsc_auth;
		wsc_enc = Entry.wsc_enc;
		strncpy(wlan_ssid, Entry.ssid, sizeof(Entry.ssid));
		wlan0_mode = Entry.wlanMode;
		wlan_wep = Entry.wep;
		wep_key_type = Entry.wepKeyType;
		wep_transmit_key = Entry.wepDefaultKey;
		memcpy(wlan_wep64_key1, Entry.wep64Key1, sizeof(Entry.wep64Key1));
		memcpy(wlan_wep64_key2, Entry.wep64Key2, sizeof(Entry.wep64Key2));
		memcpy(wlan_wep64_key3, Entry.wep64Key3, sizeof(Entry.wep64Key3));
		memcpy(wlan_wep64_key4, Entry.wep64Key4, sizeof(Entry.wep64Key4));
		memcpy(wlan_wep128_key1, Entry.wep128Key1, sizeof(Entry.wep128Key1));
		memcpy(wlan_wep128_key2, Entry.wep128Key2, sizeof(Entry.wep128Key2));
		memcpy(wlan_wep128_key3, Entry.wep128Key3, sizeof(Entry.wep128Key3));
		memcpy(wlan_wep128_key4, Entry.wep128Key4, sizeof(Entry.wep128Key4));
		strncpy(wlan_wpa_psk, Entry.wpaPSK, sizeof(Entry.wpaPSK));
		wlan0_wsc_disabled = Entry.wsc_disabled;
		wlan0_wlan_disabled = Entry.wlanDisabled;
		wlan0_encrypt = Entry.encrypt;
		wlan0_wpa_cipher = Entry.unicastCipher;
		wlan0_wpa2_cipher = Entry.wpa2UnicastCipher;		
#ifdef WLAN_MESH
		if(wlan_virtual_idx==0)
		{
			mib_get_s(MIB_WLAN_CHAN_NUM, (void *)&wlan0_mesh_channel,sizeof(wlan0_mesh_channel));
			mib_get_s(MIB_WLAN_MESH_ENABLE, (void *)&wlan0_mesh_enabled,sizeof(wlan0_mesh_enabled));			
			mib_get_s(MIB_WLAN_MESH_ID, (void *)&wlan0_meshid,sizeof(wlan0_meshid));
			mib_get_s(MIB_WLAN_MESH_ENCRYPT, (void *)&wlan0_mesh_enc,sizeof(wlan0_mesh_enc));
			mib_get_s(MIB_WLAN_MESH_WPA2_CIPHER_SUITE, (void *)&wlan0_mesh_cipher,sizeof(wlan0_mesh_cipher));
			mib_get_s(MIB_WLAN_MESH_WPA_PSK, (void *)&wlan0_mesh_psk,sizeof(wlan0_mesh_psk));
			wlan0_mesh_mode = Entry.meshWpsMode;
			strncpy(wlan0_mesh_reserved, Entry.meshReserved, sizeof(Entry.meshReserved));
		}
#endif
	}else{
		mib_get_s(MIB_WSC_AUTH, (void *)&wsc_auth,sizeof(wsc_auth));
		mib_get_s(MIB_WSC_ENC, (void *)&wsc_enc,sizeof(wsc_enc));
		mib_get_s(MIB_WLAN_SSID, (void *)&wlan_ssid,sizeof(wlan_ssid));	
		mib_get_s(MIB_WLAN_MODE, (void *)&wlan0_mode,sizeof(wlan0_mode));	
		mib_get_s(MIB_WLAN_WEP, (void *)&wlan_wep,sizeof(wlan_wep));
		mib_get_s(MIB_WLAN_WEP_KEY_TYPE, (void *)&wep_key_type,sizeof(wep_key_type));
		mib_get_s(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&wep_transmit_key,sizeof(wep_transmit_key));
		mib_get_s(MIB_WLAN_WEP64_KEY1, (void *)&wlan_wep64_key1,sizeof(wlan_wep64_key1));
		mib_get_s(MIB_WLAN_WEP64_KEY2, (void *)&wlan_wep64_key2,sizeof(wlan_wep64_key2));
		mib_get_s(MIB_WLAN_WEP64_KEY3, (void *)&wlan_wep64_key3,sizeof(wlan_wep64_key3));
		mib_get_s(MIB_WLAN_WEP64_KEY4, (void *)&wlan_wep64_key4,sizeof(wlan_wep64_key4));
		mib_get_s(MIB_WLAN_WEP128_KEY1, (void *)&wlan_wep128_key1,sizeof(wlan_wep128_key1));
		mib_get_s(MIB_WLAN_WEP128_KEY2, (void *)&wlan_wep128_key2,sizeof(wlan_wep128_key2));
		mib_get_s(MIB_WLAN_WEP128_KEY3, (void *)&wlan_wep128_key3,sizeof(wlan_wep128_key3));
		mib_get_s(MIB_WLAN_WEP128_KEY4, (void *)&wlan_wep128_key4,sizeof(wlan_wep128_key4));
		mib_get_s(MIB_WLAN_WPA_PSK, (void *)&wlan_wpa_psk,sizeof(wlan_wpa_psk));
		mib_get_s(MIB_WSC_DISABLE, (void *)&wlan0_wsc_disabled,sizeof(wlan0_wsc_disabled));	// 1104
		mib_get_s(MIB_WLAN_DISABLED, (void *)&wlan0_wlan_disabled,sizeof(wlan0_wlan_disabled));	// 0908
#if 0//defined(WIFI_SIMPLE_CONFIG) && defined(CONFIG_APP_TR069)
		mib_get_s(MIB_WLAN_WSC_AUTO_LOCK_DOWN, (void *)&auto_lockdown1,sizeof(auto_lockdown1));
		mib_get_s(MIB_WLAN_WSC_ER_NUM, (void *)&entryNum1,sizeof(entryNum1));
#endif
		mib_get_s(MIB_WLAN_ENCRYPT, (void *)&wlan0_encrypt,sizeof(wlan0_encrypt));
		mib_get_s(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&wlan0_wpa_cipher,sizeof(wlan0_wpa_cipher));
		mib_get_s(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&wlan0_wpa2_cipher,sizeof(wlan0_wpa2_cipher));
	}
	if((token == NULL || strstr(token, "wlan0") == 0) && (token1 == NULL || strstr(token1, "wlan0") == 0) 
#if defined(TRIBAND_WPS)
		&& (token2 == NULL || strstr(token2, "wlan0") == 0)
#endif
	) {
		wlan0_wsc_disabled = 1;
	}
#ifdef WPS_ONE_CLIENT_ONE_DAEMON
	if(is_cli_mode)
		wlan0_wsc_disabled = 0;
#endif
	/* for dual band*/	
	if(wlan0_wlan_disabled ){
		wlan0_wsc_disabled=1 ; // if wlan0 interface is disabled ; 
	}

#ifdef WLAN_MESH
    if(wlan0_wlan_disabled==0 && wlan0_mode==AP_MESH_MODE && wlan0_mesh_enabled) { 
    	wlan0_mesh_enabled=1;
    }else
    	wlan0_mesh_enabled=0;
#endif
	
	/* for dual band*/	
	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "#=====wlan0 start==========%d\n", wlan0_wsc_disabled, ret_err_write);
	if(ret_err_write){
		printf("WRITE_WSC_PARAM memcpy error when writing comment:[#=====wlan0 start==========%d]\n", wlan0_wsc_disabled);
		free(buf);
		return -1;
	}	

#ifdef WLAN_MESH
	if (write_wsc_int_config(&ptr, &ptr_size, "wlan0_mesh_enabled", wlan0_mesh_enabled) < 0) goto ERROR;
#endif
	
	// 1104
	if (write_wsc_int_config(&ptr, &ptr_size, "wlan0_wsc_disabled", wlan0_wsc_disabled) < 0) goto ERROR;
	if (write_wsc_int_config(&ptr, &ptr_size, "auth_type", wsc_auth) < 0) goto ERROR;
	if (write_wsc_int_config(&ptr, &ptr_size, "encrypt_type", wsc_enc) < 0) goto ERROR;
	/*for detial mixed mode info*/ 
	intVal=0;	
	if(wlan0_encrypt==6){	// mixed mode
		if(wlan0_wpa_cipher==1){
			intVal |= WSC_WPA_TKIP;
		}else if(wlan0_wpa_cipher==2){
			intVal |= WSC_WPA_AES;		
		}else if(wlan0_wpa_cipher==3){
			intVal |= (WSC_WPA_TKIP | WSC_WPA_AES); 	
		}
		if(wlan0_wpa2_cipher==1){
			intVal |= WSC_WPA2_TKIP;
		}else if(wlan0_wpa2_cipher==2){
			intVal |= WSC_WPA2_AES; 	
		}else if(wlan0_wpa2_cipher==3){
			intVal |= (WSC_WPA2_TKIP | WSC_WPA2_AES);		
		}		
	}
	if (write_wsc_int_config(&ptr, &ptr_size, "mixedmode", intVal) < 0) goto ERROR;
	//printf("[%s %d],mixedmode = 0x%02x\n", __FUNCTION__,__LINE__,intVal); 
	/*for detial mixed mode info*/ 
	if (wsc_enc == WSC_ENCRYPT_WEP)
		is_wep = 1;
	if (is_wep) { // only allow WEP in none-MANUAL mode (configured by external registrar)		
		if (wlan0_encrypt != ENCRYPT_WEP) {
			printf("WEP mismatched between WPS and host system\n");
			free(buf);
			return -1;
		}		
		if (wlan_wep <= WEP_DISABLED || wlan_wep > WEP128) {
			printf("WEP encrypt length error\n");
			free(buf);
			return -1;
		}
		wep_transmit_key++;
		if (write_wsc_int_config(&ptr, &ptr_size, "wep_transmit_key", wep_transmit_key) < 0) goto ERROR;
		/*whatever key type is ASSIC or HEX always use String-By-Hex fromat
		;2011-0419,fixed,need patch with wscd daemon , search 2011-0419*/ 
		if (wlan_wep == WEP64) {
			sprintf(tmpbuf,"%s",wlan_wep64_key1);
			convert_bin_to_str((unsigned char *)tmpbuf, 5, tmp1, sizeof(tmp1));
			tmp1[10] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key", tmp1) < 0) goto ERROR;

			sprintf(tmpbuf,"%s",wlan_wep64_key2);
			convert_bin_to_str((unsigned char *)tmpbuf, 5, tmp1, sizeof(tmp1));
			tmp1[10] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key2", tmp1) < 0) goto ERROR;

			sprintf(tmpbuf,"%s",wlan_wep64_key3);
			convert_bin_to_str((unsigned char *)tmpbuf, 5, tmp1, sizeof(tmp1));
			tmp1[10] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key3", tmp1) < 0) goto ERROR;

			sprintf(tmpbuf,"%s",wlan_wep64_key4);
			convert_bin_to_str((unsigned char *)tmpbuf, 5, tmp1, sizeof(tmp1));
			tmp1[10] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key4", tmp1) < 0) goto ERROR;
		}
		else {
			sprintf(tmpbuf,"%s",wlan_wep128_key1);
			convert_bin_to_str((unsigned char *)tmpbuf, 13, tmp1, sizeof(tmp1));
			tmp1[26] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key", tmp1) < 0) goto ERROR;

			sprintf(tmpbuf,"%s",wlan_wep128_key2);
			convert_bin_to_str((unsigned char *)tmpbuf, 13, tmp1, sizeof(tmp1));
			tmp1[26] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key2", tmp1) < 0) goto ERROR;

			sprintf(tmpbuf,"%s",wlan_wep128_key3);
			convert_bin_to_str((unsigned char *)tmpbuf, 13, tmp1, sizeof(tmp1));
			tmp1[26] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key3", tmp1) < 0) goto ERROR;

			sprintf(tmpbuf,"%s",wlan_wep128_key3);
			convert_bin_to_str((unsigned char *)tmpbuf, 13, tmp1, sizeof(tmp1));
			tmp1[26] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key4", tmp1) < 0) goto ERROR;
		}
	}
	else {
		if (write_wsc_str_config(&ptr, &ptr_size, "network_key", wlan_wpa_psk) < 0) goto ERROR;
	}
	if (write_wsc_str_config(&ptr, &ptr_size, "ssid", wlan_ssid) < 0) goto ERROR;

#ifdef WLAN_MESH
	if(wlan0_mesh_enabled) {
		if (write_wsc_int_config(&ptr, &ptr_size, "mesh_channel", wlan0_mesh_channel) < 0) goto ERROR;
		if (write_wsc_str_config(&ptr, &ptr_size, "mesh_id", wlan0_meshid) < 0) goto ERROR;
		if (write_wsc_int_config(&ptr, &ptr_size, "mesh_enc", wlan0_mesh_enc) < 0) goto ERROR;
		if (write_wsc_int_config(&ptr, &ptr_size, "mesh_cipher", wlan0_mesh_cipher) < 0) goto ERROR;
		if (write_wsc_str_config(&ptr, &ptr_size, "mesh_network_key", wlan0_mesh_psk) < 0) goto ERROR;
		if (write_wsc_int_config(&ptr, &ptr_size, "mesh_wps_mode", wlan0_mesh_mode) < 0) goto ERROR;
		if (write_wsc_str_config(&ptr, &ptr_size, "mesh_reserved", wlan0_mesh_reserved) < 0) goto ERROR;
	}
#endif
	
	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "#=====wlan0 end==========:%d\n", wlan0_wsc_disabled, ret_err_write);
	if(ret_err_write){
		printf("WRITE_WSC_PARAM memcpy error when writing comment:[#=====wlan0 end==========:%d]\n", wlan0_wsc_disabled);
		free(buf);
		return -1;
	}
#ifdef WLAN_DUALBAND_CONCURRENT
	/* switch to wlan1 */
	wlan_virtual_idx = 0;
	if(token && strstr(token, "wlan1")) {
		SetWlan_idx(token);
		get_wlan_idx_by_iface(token, &wlan_root_idx, &wlan_virtual_idx);
	}
	else if(token1 && strstr(token1, "wlan1")) {
		SetWlan_idx(token1);
		get_wlan_idx_by_iface(token1, &wlan_root_idx, &wlan_virtual_idx);
	}
#if defined(TRIBAND_WPS)
	else if(token2 && strstr(token2, "wlan1")) {
		SetWlan_idx(token2);
		get_wlan_idx_by_iface(token2, &wlan_root_idx, &wlan_virtual_idx);
	}
#endif /* defined(TRIBAND_WPS) */
	else {
		SetWlan_idx("wlan1");
	}
	//printf("===[%s %d]===wlan_virtual_idx:%d\n", __FUNCTION__,__LINE__,wlan_virtual_idx);
	//printf("======[%s %d]======wlan_idx:%d, vwlan_idx:%d\n", __FUNCTION__,__LINE__, wlan_idx, wlan_virtual_idx);
	if(1)//(wlan_virtual_idx)
	{
		memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));		
		mib_chain_get(MIB_MBSSIB_TBL, wlan_virtual_idx, (void *)&Entry);
		wsc_auth = Entry.wsc_auth;
		wsc_enc = Entry.wsc_enc;
		strncpy(wlan_ssid, Entry.ssid, sizeof(Entry.ssid));
		wlan1_mode = Entry.wlanMode;
		wlan_wep = Entry.wep;
		wep_key_type = Entry.wepKeyType;
		wep_transmit_key = Entry.wepDefaultKey;
		memcpy(wlan_wep64_key1, Entry.wep64Key1, sizeof(Entry.wep64Key1));
		memcpy(wlan_wep64_key2, Entry.wep64Key2, sizeof(Entry.wep64Key2));
		memcpy(wlan_wep64_key3, Entry.wep64Key3, sizeof(Entry.wep64Key3));
		memcpy(wlan_wep64_key4, Entry.wep64Key4, sizeof(Entry.wep64Key4));
		memcpy(wlan_wep128_key1, Entry.wep128Key1, sizeof(Entry.wep128Key1));
		memcpy(wlan_wep128_key2, Entry.wep128Key2, sizeof(Entry.wep128Key2));
		memcpy(wlan_wep128_key3, Entry.wep128Key3, sizeof(Entry.wep128Key3));
		memcpy(wlan_wep128_key4, Entry.wep128Key4, sizeof(Entry.wep128Key4));
		strncpy(wlan_wpa_psk, Entry.wpaPSK, sizeof(Entry.wpaPSK));
		wlan1_wsc_disabled = Entry.wsc_disabled;
		wlan1_wlan_disabled = Entry.wlanDisabled;
		wlan1_encrypt = Entry.encrypt;
		wlan1_wpa_cipher = Entry.unicastCipher;
		wlan1_wpa2_cipher = Entry.wpa2UnicastCipher;
#ifdef WLAN_MESH
		if(wlan_virtual_idx==0)
		{
			mib_get_s(MIB_WLAN_CHAN_NUM, (void *)&wlan1_mesh_channel,sizeof(wlan1_mesh_channel));
			mib_get_s(MIB_WLAN_MESH_ENABLE, (void *)&wlan1_mesh_enabled,sizeof(wlan1_mesh_enabled));			
			mib_get_s(MIB_WLAN_MESH_ID, (void *)&wlan1_meshid,sizeof(wlan1_meshid));
			mib_get_s(MIB_WLAN_MESH_ENCRYPT, (void *)&wlan1_mesh_enc,sizeof(wlan1_mesh_enc));
			mib_get_s(MIB_WLAN_MESH_WPA2_CIPHER_SUITE, (void *)&wlan1_mesh_cipher,sizeof(wlan1_mesh_cipher));
			mib_get_s(MIB_WLAN_MESH_WPA_PSK, (void *)&wlan1_mesh_psk,sizeof(wlan1_mesh_psk));
			wlan1_mesh_mode = Entry.meshWpsMode;
			strncpy(wlan1_mesh_reserved, Entry.meshReserved, sizeof(Entry.meshReserved));
		}
#endif
	}else{
		mib_get_s(MIB_WSC_AUTH, (void *)&wsc_auth,sizeof(wsc_auth));
		mib_get_s(MIB_WSC_ENC, (void *)&wsc_enc,sizeof(wsc_enc));
		mib_get_s(MIB_WLAN_SSID, (void *)&wlan_ssid,sizeof(wlan_ssid));	
		mib_get_s(MIB_WLAN_MODE, (void *)&wlan1_mode,sizeof(wlan1_mode));	
		mib_get_s(MIB_WLAN_WEP, (void *)&wlan_wep,sizeof(wlan_wep));
		mib_get_s(MIB_WLAN_WEP_KEY_TYPE, (void *)&wep_key_type,sizeof(wep_key_type));
		mib_get_s(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&wep_transmit_key,sizeof(wep_transmit_key));
		mib_get_s(MIB_WLAN_WEP64_KEY1, (void *)&wlan_wep64_key1,sizeof(wlan_wep64_key1));
		mib_get_s(MIB_WLAN_WEP64_KEY2, (void *)&wlan_wep64_key2,sizeof(wlan_wep64_key2));
		mib_get_s(MIB_WLAN_WEP64_KEY3, (void *)&wlan_wep64_key3,sizeof(wlan_wep64_key3));
		mib_get_s(MIB_WLAN_WEP64_KEY4, (void *)&wlan_wep64_key4,sizeof(wlan_wep64_key4));
		mib_get_s(MIB_WLAN_WEP128_KEY1, (void *)&wlan_wep128_key1,sizeof(wlan_wep128_key1));
		mib_get_s(MIB_WLAN_WEP128_KEY2, (void *)&wlan_wep128_key2,sizeof(wlan_wep128_key2));
		mib_get_s(MIB_WLAN_WEP128_KEY3, (void *)&wlan_wep128_key3,sizeof(wlan_wep128_key3));
		mib_get_s(MIB_WLAN_WEP128_KEY4, (void *)&wlan_wep128_key4,sizeof(wlan_wep128_key4));
		mib_get_s(MIB_WLAN_WPA_PSK, (void *)&wlan_wpa_psk,sizeof(wlan_wpa_psk));
		mib_get_s(MIB_WSC_DISABLE, (void *)&wlan1_wsc_disabled,sizeof(wlan1_wsc_disabled));	// 1104
		mib_get_s(MIB_WLAN_DISABLED, (void *)&wlan1_wlan_disabled,sizeof(wlan1_wlan_disabled));	// 0908 
#if 0//defined(WIFI_SIMPLE_CONFIG) && defined(CONFIG_APP_TR069)
		mib_get_s(MIB_WLAN_WSC_AUTO_LOCK_DOWN, (void *)&auto_lockdown2,sizeof(auto_lockdown2));
		mib_get_s(MIB_WLAN_WSC_ER_NUM, (void *)&entryNum2,sizeof(entryNum2));
#endif
		/*for detial mixed mode info*/ 
		mib_get_s(MIB_WLAN_ENCRYPT, (void *)&wlan1_encrypt,sizeof(wlan1_encrypt));
		mib_get_s(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&wlan1_wpa_cipher,sizeof(wlan1_wpa_cipher));
		mib_get_s(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&wlan1_wpa2_cipher,sizeof(wlan1_wpa2_cipher));
	}	
	/*if  wlanif_name doesn't include "wlan1", disable wlan1 wsc*/
	if((token == NULL || strstr(token, "wlan1") == 0) && (token1 == NULL || strstr(token1, "wlan1") == 0)
#if defined(TRIBAND_WPS)
		&& (token2 == NULL || strstr(token2, "wlan1") == 0)
#endif
	) {
		wlan1_wsc_disabled = 1;
	}
	/* for dual band*/	
	if(wlan1_wlan_disabled){
		wlan1_wsc_disabled = 1 ; // if wlan1 interface is disabled			
	}	
	/* for dual band*/		
#ifdef WPS_ONE_CLIENT_ONE_DAEMON
	if(is_cli_mode)
		wlan1_wsc_disabled = 1;
#endif
	
#ifdef WLAN_MESH
	if(wlan1_wlan_disabled==0 && wlan1_mode==AP_MESH_MODE && wlan1_mesh_enabled) { 
		wlan1_mesh_enabled=1;
	}else
		wlan1_mesh_enabled=0;
#endif

	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "#=====wlan1 start==========%d\n",wlan1_wsc_disabled, ret_err_write);
    if(ret_err_write){
		printf("WRITE_WSC_PARAM memcpy error when writing comment:[#=====wlan1 start==========%d]\n", wlan1_wsc_disabled);
		free(buf);
		return -1;
	}

#ifdef WLAN_MESH
	if (write_wsc_int_config(&ptr, &ptr_size, "wlan1_mesh_enabled", wlan1_mesh_enabled) < 0) goto ERROR;
#endif

	if (write_wsc_str_config(&ptr, &ptr_size, "ssid2", wlan_ssid) < 0) goto ERROR;
	if (write_wsc_int_config(&ptr, &ptr_size, "auth_type2", wsc_auth) < 0) goto ERROR;
	if (write_wsc_int_config(&ptr, &ptr_size, "encrypt_type2", wsc_enc) < 0) goto ERROR;
	/*for detial mixed mode info*/ 
	intVal=0;	
	if(wlan1_encrypt==6){	// mixed mode
		if(wlan1_wpa_cipher==1){
			intVal |= WSC_WPA_TKIP;
		}else if(wlan1_wpa_cipher==2){
			intVal |= WSC_WPA_AES;		
		}else if(wlan1_wpa_cipher==3){
			intVal |= (WSC_WPA_TKIP | WSC_WPA_AES); 	
		}
		if(wlan1_wpa2_cipher==1){
			intVal |= WSC_WPA2_TKIP;
		}else if(wlan1_wpa2_cipher==2){
			intVal |= WSC_WPA2_AES; 	
		}else if(wlan1_wpa2_cipher==3){
			intVal |= (WSC_WPA2_TKIP | WSC_WPA2_AES);		
		}		
	}
	if (write_wsc_int_config(&ptr, &ptr_size, "mixedmode2", intVal) < 0) goto ERROR;
	if (write_wsc_int_config(&ptr, &ptr_size, "wlan1_wsc_disabled", wlan1_wsc_disabled) < 0) goto ERROR;

	is_wep = 0;
	if (wsc_enc == WSC_ENCRYPT_WEP)
		is_wep = 1;
	if (is_wep) { // only allow WEP in none-MANUAL mode (configured by external registrar)		
		if (wlan1_encrypt != ENCRYPT_WEP) {
			printf("WEP mismatched between WPS and host system\n");
			free(buf);
			return -1;
		}		
		if (wlan_wep <= WEP_DISABLED || wlan_wep > WEP128) {
			printf("WEP encrypt length error\n");
			free(buf);
			return -1;
		}
		wep_transmit_key++;
		if (write_wsc_int_config(&ptr, &ptr_size, "wep_transmit_key2", wep_transmit_key) < 0) goto ERROR;
		/*whatever key type is ASSIC or HEX always use String-By-Hex fromat
		;2011-0419,fixed,need patch with wscd daemon , search 2011-0419*/ 
		if (wlan_wep == WEP64) {
			sprintf(tmpbuf,"%s",wlan_wep64_key1);
			convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
			tmp1[10] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key2", tmp1) < 0) goto ERROR;

			sprintf(tmpbuf,"%s",wlan_wep64_key2);
			convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
			tmp1[10] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key22", tmp1) < 0) goto ERROR;

			sprintf(tmpbuf,"%s",wlan_wep64_key3);
			convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
			tmp1[10] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key32", tmp1) < 0) goto ERROR;

			sprintf(tmpbuf,"%s",wlan_wep64_key4);
			convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
			tmp1[10] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key42", tmp1) < 0) goto ERROR;
		}
		else {
			sprintf(tmpbuf,"%s",wlan_wep128_key1);
			convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
			tmp1[26] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key2", tmp1) < 0) goto ERROR;

			sprintf(tmpbuf,"%s",wlan_wep128_key2);
			convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
			tmp1[26] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key22", tmp1) < 0) goto ERROR;

			sprintf(tmpbuf,"%s",wlan_wep128_key3);
			convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
			tmp1[26] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key32", tmp1) < 0) goto ERROR;

			sprintf(tmpbuf,"%s",wlan_wep128_key3);
			convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
			tmp1[26] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key42", tmp1) < 0) goto ERROR;
		}
	}
	else {
		if (write_wsc_str_config(&ptr, &ptr_size, "network_key2", wlan_wpa_psk) < 0) goto ERROR;
	}

#ifdef WLAN_MESH
	if(wlan1_mesh_enabled) {
		if (write_wsc_int_config(&ptr, &ptr_size, "mesh2_channel", wlan1_mesh_channel) < 0) goto ERROR;
		if (write_wsc_str_config(&ptr, &ptr_size, "mesh2_id", wlan1_meshid) < 0) goto ERROR;
		if (write_wsc_int_config(&ptr, &ptr_size, "mesh2_enc", wlan1_mesh_enc) < 0) goto ERROR;
		if (write_wsc_int_config(&ptr, &ptr_size, "mesh2_cipher", wlan1_mesh_cipher) < 0) goto ERROR;
		if (write_wsc_str_config(&ptr, &ptr_size, "mesh2_network_key", wlan1_mesh_psk) < 0) goto ERROR;
		if (write_wsc_int_config(&ptr, &ptr_size, "mesh2_wps_mode", wlan1_mesh_mode) < 0) goto ERROR;
		if (write_wsc_str_config(&ptr, &ptr_size, "mesh2_reserved", wlan1_mesh_reserved) < 0) goto ERROR;
	}
#endif
	
	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "#=====wlan1 end==========%d\n", wlan1_wsc_disabled, ret_err_write);	
	if(ret_err_write){
		printf("WRITE_WSC_PARAM memcpy error when writing comment:[#=====wlan1 end==========%d]\n", wlan1_wsc_disabled);
		free(buf);
		return -1;
	}
#endif	// END of WLAN_DUALBAND_CONCURRENT
#if defined(TRIBAND_WPS)
	wlan_virtual_idx = 0;
    /* switch to wlan2 */
    if (token && strstr(token, "wlan2")) {
        SetWlan_idx(token);
		get_wlan_idx_by_iface(token, &wlan_root_idx, &wlan_virtual_idx);
    } else if(token1 && strstr(token1, "wlan2")) {
        SetWlan_idx(token1);
		get_wlan_idx_by_iface(token1, &wlan_root_idx, &wlan_virtual_idx);
    } else if(token2 && strstr(token2, "wlan2")) {
        SetWlan_idx(token2);
		get_wlan_idx_by_iface(token2, &wlan_root_idx, &wlan_virtual_idx);
    } else {
        SetWlan_idx("wlan2");
    }
	//printf("===[%s %d]===wlan_virtual_idx:%d\n", __FUNCTION__,__LINE__,wlan_virtual_idx);
	//printf("======[%s %d]======wlan_idx:%d, vwlan_idx:%d\n", __FUNCTION__,__LINE__, wlan_idx, wlan_virtual_idx);
	
	if(1)//(wlan_virtual_idx)
	{
		memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));		
		mib_chain_get(MIB_MBSSIB_TBL, wlan_virtual_idx, (void *)&Entry);
		wsc_auth = Entry.wsc_auth;
		wsc_enc = Entry.wsc_enc;
		strncpy(wlan_ssid, Entry.ssid, sizeof(Entry.ssid));
		wlan2_mode = Entry.wlanMode;
		wlan_wep = Entry.wep;
		wep_key_type = Entry.wepKeyType;
		wep_transmit_key = Entry.wepDefaultKey;
		memcpy(wlan_wep64_key1, Entry.wep64Key1, sizeof(Entry.wep64Key1));
		memcpy(wlan_wep64_key2, Entry.wep64Key2, sizeof(Entry.wep64Key2));
		memcpy(wlan_wep64_key3, Entry.wep64Key3, sizeof(Entry.wep64Key3));
		memcpy(wlan_wep64_key4, Entry.wep64Key4, sizeof(Entry.wep64Key4));
		memcpy(wlan_wep128_key1, Entry.wep128Key1, sizeof(Entry.wep128Key1));
		memcpy(wlan_wep128_key2, Entry.wep128Key2, sizeof(Entry.wep128Key2));
		memcpy(wlan_wep128_key3, Entry.wep128Key3, sizeof(Entry.wep128Key3));
		memcpy(wlan_wep128_key4, Entry.wep128Key4, sizeof(Entry.wep128Key4));
		strncpy(wlan_wpa_psk, Entry.wpaPSK, sizeof(Entry.wpaPSK));
		wlan2_wsc_disabled = Entry.wsc_disabled;
		wlan2_wlan_disabled = Entry.wlanDisabled;
		wlan2_encrypt = Entry.encrypt;
		wlan2_wpa_cipher = Entry.unicastCipher;
		wlan2_wpa2_cipher = Entry.wpa2UnicastCipher;
#ifdef WLAN_MESH
		if(wlan_virtual_idx==0)
		{
			mib_get_s(MIB_WLAN_CHAN_NUM, (void *)&wlan2_mesh_channel,sizeof(wlan2_mesh_channel));
			mib_get_s(MIB_WLAN_MESH_ENABLE, (void *)&wlan2_mesh_enabled,sizeof(wlan2_mesh_enabled));			
			mib_get_s(MIB_WLAN_MESH_ID, (void *)&wlan2_meshid,sizeof(wlan2_meshid));
			mib_get_s(MIB_WLAN_MESH_ENCRYPT, (void *)&wlan2_mesh_enc,sizeof(wlan2_mesh_enc));
			mib_get_s(MIB_WLAN_MESH_WPA2_CIPHER_SUITE, (void *)&wlan2_mesh_cipher,sizeof(wlan2_mesh_cipher));
			mib_get_s(MIB_WLAN_MESH_WPA_PSK, (void *)&wlan2_mesh_psk,sizeof(wlan2_mesh_psk));
			wlan2_mesh_mode = Entry.meshWpsMode;
			strncpy(wlan2_mesh_reserved, Entry.meshReserved, sizeof(Entry.meshReserved));
		}
#endif			
	}else{
		mib_get_s(MIB_WSC_AUTH, (void *)&wsc_auth,sizeof(wsc_auth));
	    mib_get_s(MIB_WSC_ENC, (void *)&wsc_enc,sizeof(wsc_enc));
	    mib_get_s(MIB_WLAN_SSID, (void *)&wlan_ssid,sizeof(wlan_ssid));    
	    mib_get_s(MIB_WLAN_MODE, (void *)&wlan2_mode,sizeof(wlan2_mode));   
	    mib_get_s(MIB_WLAN_WEP, (void *)&wlan_wep,sizeof(wlan_wep));
	    mib_get_s(MIB_WLAN_WEP_KEY_TYPE, (void *)&wep_key_type,sizeof(wep_key_type));
	    mib_get_s(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&wep_transmit_key,sizeof(wep_transmit_key));
	    mib_get_s(MIB_WLAN_WEP64_KEY1, (void *)&wlan_wep64_key1,sizeof(wlan_wep64_key1));
	    mib_get_s(MIB_WLAN_WEP64_KEY2, (void *)&wlan_wep64_key2,sizeof(wlan_wep64_key2));
	    mib_get_s(MIB_WLAN_WEP64_KEY3, (void *)&wlan_wep64_key3,sizeof(wlan_wep64_key3));
	    mib_get_s(MIB_WLAN_WEP64_KEY4, (void *)&wlan_wep64_key4,sizeof(wlan_wep64_key4));
	    mib_get_s(MIB_WLAN_WEP128_KEY1, (void *)&wlan_wep128_key1,sizeof(wlan_wep128_key1));
	    mib_get_s(MIB_WLAN_WEP128_KEY2, (void *)&wlan_wep128_key2,sizeof(wlan_wep128_key2));
	    mib_get_s(MIB_WLAN_WEP128_KEY3, (void *)&wlan_wep128_key3,sizeof(wlan_wep128_key3));
	    mib_get_s(MIB_WLAN_WEP128_KEY4, (void *)&wlan_wep128_key4,sizeof(wlan_wep128_key4));
	    mib_get_s(MIB_WLAN_WPA_PSK, (void *)&wlan_wpa_psk,sizeof(wlan_wpa_psk));
	    mib_get_s(MIB_WSC_DISABLE, (void *)&wlan2_wsc_disabled,sizeof(wlan2_wsc_disabled)); // 1104
	    mib_get_s(MIB_WLAN_DISABLED, (void *)&wlan2_wlan_disabled,sizeof(wlan2_wlan_disabled));  // 0908 
#if 0//defined(WIFI_SIMPLE_CONFIG) && defined(CONFIG_APP_TR069)
	    mib_get_s(MIB_WLAN_WSC_AUTO_LOCK_DOWN, (void *)&auto_lockdown3,sizeof(auto_lockdown3));
	    mib_get_s(MIB_WLAN_WSC_ER_NUM, (void *)&entryNum3,sizeof(entryNum3));
#endif	    
	    /*for detial mixed mode info*/ 
	    mib_get_s(MIB_WLAN_ENCRYPT, (void *)&wlan2_encrypt,sizeof(wlan2_encrypt));
	    mib_get_s(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&wlan2_wpa_cipher,sizeof(wlan2_wpa_cipher));
	    mib_get_s(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&wlan2_wpa2_cipher,sizeof(wlan2_wpa2_cipher));
	}
    /*if  wlanif_name doesn't include "wlan2", disable wlan2 wsc*/
    if ((token == NULL || strstr(token, "wlan2") == 0) && (token1 == NULL || strstr(token1, "wlan2") == 0)
            && (token2 == NULL || strstr(token2, "wlan2"))) {            
        wlan2_wsc_disabled = 1;
    }
    /* for tri band*/  
    if (wlan2_wlan_disabled) {
        wlan2_wsc_disabled = 1 ; // if wlan2 interface is disabled          
    }   
    /* for tri band*/      
#ifdef WPS_ONE_CLIENT_ONE_DAEMON
    if(is_cli_mode)
        wlan2_wsc_disabled = 1;
#endif

#ifdef WLAN_MESH
	if(wlan2_wlan_disabled==0 && wlan2_mode==AP_MESH_MODE && wlan2_mesh_enabled) { 
		wlan2_mesh_enabled=1;
	}else
		wlan2_mesh_enabled=0;
#endif

    WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "#=====wlan2 start==========%d\n",wlan2_wsc_disabled, ret_err_write);
    if(ret_err_write){
        printf("WRITE_WSC_PARAM memcpy error when writing comment:[#=====wlan2 start==========%d]\n", wlan2_wsc_disabled);
        free(buf);
        return -1;
	}

#ifdef WLAN_MESH
	if (write_wsc_int_config(&ptr, &ptr_size, "wlan2_mesh_enabled", wlan2_mesh_enabled) < 0) goto ERROR;
#endif
	if (write_wsc_str_config(&ptr, &ptr_size, "ssid3", wlan_ssid) < 0) goto ERROR;
	if (write_wsc_int_config(&ptr, &ptr_size, "auth_type3", wsc_auth) < 0) goto ERROR;
	if (write_wsc_int_config(&ptr, &ptr_size, "encrypt_type3", wsc_enc) < 0) goto ERROR;
    /*for detial mixed mode info*/ 
    intVal=0;   
    if (wlan2_encrypt==6) {   // mixed mode
        if(wlan2_wpa_cipher==1) {
            intVal |= WSC_WPA_TKIP;
        } else if(wlan2_wpa_cipher==2) {
            intVal |= WSC_WPA_AES;      
        } else if(wlan2_wpa_cipher==3) {
            intVal |= (WSC_WPA_TKIP | WSC_WPA_AES);     
        }
        if (wlan2_wpa2_cipher==1) {
            intVal |= WSC_WPA2_TKIP;
        } else if (wlan2_wpa2_cipher==2) {
            intVal |= WSC_WPA2_AES;     
        } else if (wlan2_wpa2_cipher==3) {
            intVal |= (WSC_WPA2_TKIP | WSC_WPA2_AES);       
        }       
    }
	if (write_wsc_int_config(&ptr, &ptr_size, "mixedmode3", intVal) < 0) goto ERROR;
	if (write_wsc_int_config(&ptr, &ptr_size, "wlan2_wsc_disabled", wlan2_wsc_disabled) < 0) goto ERROR;

    is_wep = 0;
    if (wsc_enc == WSC_ENCRYPT_WEP)
        is_wep = 1;
    if (is_wep) { // only allow WEP in none-MANUAL mode (configured by external registrar)      
        if (wlan2_encrypt != ENCRYPT_WEP) {
            printf("WEP mismatched between WPS and host system\n");
            free(buf);
            return -1;
        }       
        if (wlan_wep <= WEP_DISABLED || wlan_wep > WEP128) {
            printf("WEP encrypt length error\n");
            free(buf);
            return -1;
        }
        wep_transmit_key++;
		if (write_wsc_int_config(&ptr, &ptr_size, "wep_transmit_key3", wep_transmit_key) < 0) goto ERROR;
        /*whatever key type is ASSIC or HEX always use String-By-Hex fromat
        ;2011-0419,fixed,need patch with wscd daemon , search 2011-0419*/ 
        if (wlan_wep == WEP64) {
            sprintf(tmpbuf,"%s",wlan_wep64_key1);
            convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
            tmp1[10] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key3", tmp1) < 0) goto ERROR;

            sprintf(tmpbuf,"%s",wlan_wep64_key2);
            convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
            tmp1[10] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key23", tmp1) < 0) goto ERROR;

            sprintf(tmpbuf,"%s",wlan_wep64_key3);
            convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
            tmp1[10] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key33", tmp1) < 0) goto ERROR;

            sprintf(tmpbuf,"%s",wlan_wep64_key4);
            convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
            tmp1[10] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key43", tmp1) < 0) goto ERROR;
        } else {
            sprintf(tmpbuf,"%s",wlan_wep128_key1);
            convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
            tmp1[26] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key3", tmp1) < 0) goto ERROR;

            sprintf(tmpbuf,"%s",wlan_wep128_key2);
            convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
            tmp1[26] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key23", tmp1) < 0) goto ERROR;

            sprintf(tmpbuf,"%s",wlan_wep128_key3);
            convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
            tmp1[26] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key33", tmp1) < 0) goto ERROR;

            sprintf(tmpbuf,"%s",wlan_wep128_key3);
            convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
            tmp1[26] = '\0';
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key43", tmp1) < 0) goto ERROR;
        }
    } else {
		if (write_wsc_str_config(&ptr, &ptr_size, "network_key3", wlan_wpa_psk) < 0) goto ERROR;
    }

#ifdef WLAN_MESH
	if(wlan2_mesh_enabled) {
		if (write_wsc_int_config(&ptr, &ptr_size, "mesh3_channel", wlan2_mesh_channel) < 0) goto ERROR;
		if (write_wsc_str_config(&ptr, &ptr_size, "mesh3_id", wlan2_meshid) < 0) goto ERROR;
		if (write_wsc_int_config(&ptr, &ptr_size, "mesh3_enc", wlan2_mesh_enc) < 0) goto ERROR;
		if (write_wsc_int_config(&ptr, &ptr_size, "mesh3_cipher", wlan2_mesh_cipher) < 0) goto ERROR;
		if (write_wsc_str_config(&ptr, &ptr_size, "mesh3_network_key", wlan2_mesh_psk) < 0) goto ERROR;
		if (write_wsc_int_config(&ptr, &ptr_size, "mesh3_wps_mode", wlan2_mesh_mode) < 0) goto ERROR;
		if (write_wsc_str_config(&ptr, &ptr_size, "mesh3_reserved", wlan2_mesh_reserved) < 0) goto ERROR;
	}
#endif
    WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "#=====wlan2 end==========%d\n", wlan2_wsc_disabled, ret_err_write);
	if(ret_err_write){
		printf("WRITE_WSC_PARAM memcpy error when writing comment:[#=====wlan2 end==========%d]\n", wlan2_wsc_disabled);
		free(buf);
		return -1;
	}
#endif /* defined(TRIBAND_WPS) */
#if defined(WIFI_SIMPLE_CONFIG) && defined(CONFIG_APP_TR069)
	/* auto lock down state */
	if( auto_lockdown1 || auto_lockdown2)
		auto_lockdown = 1;
	else
		auto_lockdown = 0;
	if (write_wsc_int_config(&ptr, &ptr_size, "auto_lockdown", auto_lockdown) < 0) goto ERROR;
	/* external registrar information 
		take example for wlan0 (wlan1 is the same as wlan0 )
	 */
	if(entryNum1 || entryNum2){
		entryNum = entryNum1;
	}
	if (write_wsc_int_config(&ptr, &ptr_size, "external_reg_num", entryNum) < 0) goto ERROR;

	if(entryNum){
		for(ii=1;ii<=entryNum;ii++){
			memset(&extReg, 0x00, sizeof(extReg));
			*((char *)&extReg) = (char)ii;
			if ( mib_get_s(MIB_WLAN_WSC_ER_TBL, (void *)&extReg,sizeof(extReg))){
				WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "ER_disable%d = \"%d\"\n", ii, extReg.disable, ret_err_write);
			    if(ret_err_write){
		            printf("WRITE_WSC_PARAM2 memcpy error when writing 'ER_disable%d'\n", ii);
					goto ERROR;
	            }
				WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "ER_uuid%d= \"%s\"\n", ii, extReg.uuid, ret_err_write);
				if(ret_err_write){
		            printf("WRITE_WSC_PARAM2 memcpy error when writing 'ER_uuid%d'\n", ii);
					goto ERROR;
	            }
				WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "ER_devname%d = \"%s\"\n", ii, extReg.devicename, ret_err_write); 
				if(ret_err_write){
		            printf("WRITE_WSC_PARAM2 memcpy error when writing 'ER_devname%d'\n", ii);
					goto ERROR;
	            }
			}
		}		
	}
#endif
#ifdef WPS_ONE_CLIENT_ONE_DAEMON
	if(is_cli_mode){
		if (write_wsc_int_config(&ptr, &ptr_size, "wps_one_cli_one_daemon", 1) < 0) goto ERROR;
	}
#endif
	//mib_get_s(gMIB_SNMP_SYS_NAME, (void *)&device_name,sizeof(device_name));	
	if (write_wsc_str_config(&ptr, &ptr_size, "device_name", device_name) < 0) goto ERROR;
	if (write_wsc_int_config(&ptr, &ptr_size, "config_by_ext_reg", wsc_config_by_ext_reg) < 0) goto ERROR;
	len = (int)(((long)ptr)-((long)buf));

	if ((fh = open(in, O_RDONLY)) < 0) {
		printf("open() error [%s]!\n", in);
		goto ERROR;
	}
	lseek(fh, 0L, SEEK_SET);
	if (read(fh, ptr, status.st_size) != status.st_size) {		
		printf("read() error [%s]!\n", in);
		goto ERROR;
	}
	close(fh);
	*(ptr+status.st_size)='\0';
	ptr = strstr(ptr, "uuid =");
	if (ptr) {
		char tmp2[100];
		mib_get_s(MIB_WLAN_MAC_ADDR, (void *)tmp1,sizeof(tmp1));
		if (!memcmp(tmp1, "\x00\x00\x00\x00\x00\x00", 6)) {
			mib_get_s(MIB_ELAN_MAC_ADDR, (void *)&tmp1,sizeof(tmp1));
		}
		convert_bin_to_str((unsigned char *)tmp1, 6, tmp2, sizeof(tmp2));
		memcpy(ptr+27, tmp2, 12);		
	}

	if ((fh = open(out, O_RDWR|O_CREAT|O_TRUNC)) < 0) {
		printf("open() error [%s]!\n", out);
		goto ERROR;
	}
	if (write(fh, buf, len+status.st_size) != len+status.st_size ) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}
	if (fh != -1) close(fh);
	if (buf) free(buf);	
#if defined(WLAN_DUALBAND_CONCURRENT)
	wlan_idx = original_wlan_idx;
#endif	
	return 0;

ERROR:
	if (fh != -1) close(fh);
	if (buf) free(buf);
#if defined(WLAN_DUALBAND_CONCURRENT)
	wlan_idx = original_wlan_idx;
#endif
	return -1;
}
#else
#ifndef WLAN_WPS_VAP
int start_wsc_deamon(char * wlan_interface, int mode)
{	
    char *cmd_opt[20]={0};
    int cmd_cnt = 0;
    char tempbuf[40];
    char arg_buff[40];
    char *token=NULL,*token1=NULL, *savestr1=NULL;
    int wps_debug=0; //, use_iwcontrol=1;
    char wsc_pin_local[16]={0},wsc_pin_peer[16]={0};
    FILE *fp;
    char wscFifoFile[40];
    char wscFifoFile1[40];
    char wscConfFile[40];    
    int wait_fifo=0;
	int status=0;
	int idx=0;
	struct stat fifo_status, fifo2_status;

    if(wlan_interface == NULL) {
        return -1;
    }
    
    memset(wscFifoFile, 0, sizeof(wscFifoFile));
    memset(wscFifoFile1, 0, sizeof(wscFifoFile1));
    memset(cmd_opt, 0x00, 16);
    cmd_cnt=0;
    wps_debug=0;
    //use_iwcontrol=1;

    sprintf(arg_buff,"%s", wlan_interface);
    token = strtok_r(arg_buff," ", &savestr1);
    if(token)
        token1 = strtok_r(NULL," ", &savestr1);
    
    cmd_opt[cmd_cnt++] = "wscd";

	#define TARGDIR "/var/wps/"
	#define SIMPLECFG "simplecfgservice.xml"
	//status |= va_cmd("/bin/flash", 3, 1, "upd-wsc-conf", "/etc/wscd.conf", "/var/wscd.conf");
	status |= va_cmd("/bin/mkdir", 2, 1, "-p", TARGDIR);
	status |= va_cmd("/bin/cp", 2, 1, "/etc/" SIMPLECFG, TARGDIR);
    
    if(mode == 1) /*cleint*/
    {
        //WSC_UPNP_Enabled=0;
        cmd_opt[cmd_cnt++] = "-mode";
        cmd_opt[cmd_cnt++] = "2";
        
    }else{
        cmd_opt[cmd_cnt++] = "-start";
		if(token && token1)
			cmd_opt[cmd_cnt++] = "-both_band_ap";
    }

#if 0
    if(WSC_UPNP_Enabled==1){
		va_cmd("route", 6, 1, "del", "-net", "239.255.255.250", "netmask", "255.255.255.255", bridge_iface);
		va_cmd("route", 6, 1, "add", "-net", "239.255.255.250", "netmask", "255.255.255.255", bridge_iface);
    }
#endif
    
    sprintf(wscConfFile,"/var/wsc-%s", token);
    if(token1) {
        strcat(wscConfFile, "-");            
        strcat(wscConfFile, token1);
    }
    strcat(wscConfFile, ".conf");

	WPS_updateWscConf("/etc/wscd.conf", wscConfFile, 0, wlan_interface);

	/* 98f use xml config */
    //RunSystemCmd(NULL_FILE, "flash", "upd-wsc-conf", "/etc/wscd.conf", wscConfFile, wlan_interface, NULL_STR);     
    
    cmd_opt[cmd_cnt++] = "-c";
    cmd_opt[cmd_cnt++] = wscConfFile;
    
    if(token && token[4] == '0')
        cmd_opt[cmd_cnt++] = "-w";    
    else{
		//if(mode == 1)
			cmd_opt[cmd_cnt++] = "-w2";
		//else
	    //    cmd_opt[cmd_cnt++] = "-w2";
    }

    cmd_opt[cmd_cnt++] = token;

    if(token1) {
        if(token1[4] == '0')
            cmd_opt[cmd_cnt++] = "-w";       
        else
            cmd_opt[cmd_cnt++] = "-w2";
        cmd_opt[cmd_cnt++] = token1;    
    }
#if 0
    if(wps_debug==1){
        /* when you would like to open debug, you should add define in wsc.h for debug mode enable*/
        cmd_opt[cmd_cnt++] = "-debug";
    }
#endif
    //if(use_iwcontrol==1)
    {
        if(token && token[4] == '0')
            cmd_opt[cmd_cnt++] = "-fi";
        else
            cmd_opt[cmd_cnt++] = "-fi2";
        sprintf(wscFifoFile,"/var/wscd-%s.fifo",token);        
        cmd_opt[cmd_cnt++] = wscFifoFile;

        if(token1) {
            if(token1[4] == '0')
                cmd_opt[cmd_cnt++] = "-fi";
            else
                cmd_opt[cmd_cnt++] = "-fi2";
            sprintf(wscFifoFile1,"/var/wscd-%s.fifo",token1);        
            cmd_opt[cmd_cnt++] = wscFifoFile1;
        }
    }
#if 0
    if(isFileExist("/var/wps_start_pbc")){
        cmd_opt[cmd_cnt++] = "-start_pbc";
        unlink("/var/wps_start_pbc");
    }
    if(isFileExist("/var/wps_start_pin")){
        cmd_opt[cmd_cnt++] = "-start";
        unlink("/var/wps_start_pin");
    }
    if(isFileExist("/var/wps_local_pin")){
        fp=fopen("/var/wps_local_pin", "r");
        if(fp != NULL){
            fscanf(fp, "%s", tempbuf);
            fclose(fp);
        }
        sprintf(wsc_pin_local, "%s", tempbuf);
        cmd_opt[cmd_cnt++] = "-local_pin";
        cmd_opt[cmd_cnt++] = wsc_pin_local;
        unlink("/var/wps_local_pin");
    }
    if(isFileExist("/var/wps_peer_pin")){
        fp=fopen("/var/wps_peer_pin", "r");
        if(fp != NULL){
            fscanf(fp, "%s", tempbuf);
            fclose(fp);
        }
        sprintf(wsc_pin_peer, "%s", tempbuf);
        cmd_opt[cmd_cnt++] = "-peer_pin";
        cmd_opt[cmd_cnt++] = wsc_pin_peer;
        unlink("/var/wps_peer_pin");
    }

#if defined(AVOID_DUAL_CLIENT_PBC_OVERLAPPING) && defined(WLAN_DUALBAND_CONCURRENT)
	if(token && token1 && mode==1){
		cmd_opt[cmd_cnt++] = "-prefer_band";
		cmd_opt[cmd_cnt++] = "1"; // prefer 2.4G
	}	
#endif

    cmd_opt[cmd_cnt++] = "-daemon";
#endif
    
    cmd_opt[cmd_cnt] = 0;

	printf("CMD ARGS: ");
	for (idx=0; idx<cmd_cnt;idx++)
		printf("%s ", cmd_opt[idx]);
	printf("\n");

	status |= do_nice_cmd(WSC_DAEMON_PROG, cmd_opt, 0);

#if 0
	printf("===> cmd:");
    for (wps_debug=0; wps_debug<cmd_cnt;wps_debug++)
       printf("%s ", cmd_opt[wps_debug]);
	printf("\n");
#endif
	
    //DoCmd(cmd_opt, NULL_FILE);

    //if(use_iwcontrol) {
        wait_fifo=5;
        do{        
            if(!(stat(wscFifoFile, &fifo_status) < 0) && (wscFifoFile1[0] == 0 || !(stat(wscFifoFile1, &fifo2_status) < 0)))
            {
                wait_fifo=0;
            }else{
                wait_fifo--;
                sleep(1);
            }
            
        }while(wait_fifo !=0);
   // }
    
    return status;
}
#endif

#ifndef CONFIG_WIFI_SIMPLE_CONFIG
enum { 

	WPS_AP_MODE=0, 
	WPS_CLIENT_MODE=1, 
	WPS_WDS_MODE=2, 
	WPS_AP_WDS_MODE=3 
};
#endif

#ifdef WLAN_WPS_VAP
int WPS_updateWscConf(char *in, char *out, int genpin, MIB_CE_MBSSIB_T *Entry, int vwlan_idx, int wlanIdx)
{
	int fh = -1;
	struct stat status;
	char *buf = NULL, *ptr;
	unsigned char intVal2, is_client, is_config, is_registrar, is_wep=0, wep_key_type=0, wep_transmit_key=0;
	unsigned char intVal, current_wps_version;
	unsigned char wlan_encrypt=0, wlan_wpa_cipher=0, wlan_wpa2_cipher=0;
	int config_method;
	unsigned char tmpbuf[100], tmp1[100];
	unsigned int ptr_size = 0;
	int len;
	int ret_err_write = 0;
#if defined(FOR_DUAL_BAND)
	unsigned char wlanBand2G5GSelect;
	//int orig_wlan_idx;
#endif

#ifdef FOR_DUAL_BAND
//	int wlan_idx_orig = wlan_idx;
#endif //FOR_DUAL_BAND
	/*
	if ( !mib_init()) {
		printf("Initialize AP MIB failed!\n");
		return -1;
	}
	*/
	fprintf(stderr, "Writing file %s...\n", out ? out : "");
	mib_get_s(MIB_WSC_PIN, (void *)tmpbuf, sizeof(tmpbuf));
	if (genpin || !strcmp(tmpbuf, "\x0")) {
		#include <sys/time.h>
		struct timeval tod;
		unsigned long num;

		mib_get_s(MIB_ELAN_MAC_ADDR/*MIB_HW_NIC0_ADDR*/, (void *)&tmp1, sizeof(tmp1));
		gettimeofday(&tod , NULL);
		tod.tv_sec += tmp1[4]+tmp1[5];
		srand(tod.tv_sec);
		num = rand() % 10000000;
		num = num*10 + compute_pin_checksum(num);
		convert_hex_to_ascii((unsigned long)num, tmpbuf);

		mib_set(MIB_WSC_PIN, (void *)tmpbuf);
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);

		printf("Generated PIN = %s\n", tmpbuf);

		if (genpin)
			return 0;
	}
#ifdef FOR_DUAL_BAND
//	wlan_idx = 0;
#endif //FOR_DUAL_BAND
#if defined(FOR_DUAL_BAND)
	//orig_wlan_idx = wlan_idx;
	//wlan_idx = 0;
#endif
	if (stat(in, &status) < 0) {
		printf("stat() error [%s]!\n", in);
		goto ERROR;
	}

	buf = malloc(status.st_size+2048);
	if (buf == NULL) {
		printf("malloc() error [%d]!\n", (int)status.st_size+2048);
		goto ERROR;
	}

	ptr = buf;
	ptr_size = status.st_size + 2048;
	memset(ptr, '\0', ptr_size);

	//mib_get_s(gMIB_WLAN_MODE, (void *)&is_client, sizeof(is_client));
	is_client = Entry->wlanMode;

	is_config = Entry->wsc_configured;

	mib_local_mapping_get(MIB_WSC_REGISTRAR_ENABLED, wlanIdx, (void *)&is_registrar); //root
	if (is_client == WPS_CLIENT_MODE) {
		if (is_registrar)
			intVal = MODE_CLIENT_CONFIG;
		else {
			if (!is_config)
				intVal = MODE_CLIENT_UNCONFIG;
			else
				intVal = MODE_CLIENT_CONFIG;
		}
	}
	else {
		is_registrar = 1; // always true in AP
		if (!is_config)
			intVal = MODE_AP_UNCONFIG;
		else {
			if (is_registrar)
				intVal = MODE_AP_PROXY_REGISTRAR;
			else
				intVal = MODE_AP_PROXY;
		}
	}

	if (write_wsc_int_config(&ptr, &ptr_size, "mode", intVal) < 0) goto ERROR;

	if (is_client)
		intVal = 0;
	else
		intVal = Entry->wsc_upnp_enabled; //vap

#ifdef WLAN_WPS_MULTI_DAEMON
	if(!is_client){
		if(wlanIdx == 0) //check upnp enabled only for ssid-1
			intVal = Entry->wsc_upnp_enabled;
		else
			intVal = 0;
	}
#endif

	if (write_wsc_int_config(&ptr, &ptr_size, "upnp", intVal) < 0) goto ERROR;

#ifdef WPS20
#ifdef WPS_VERSION_CONFIGURABLE
	if (mib_get_s(MIB_WSC_VERSION, (void *)&current_wps_version, sizeof(current_wps_version)) == 0)
#endif
		current_wps_version = WPS_VERSION_V2;
#else
	current_wps_version = WPS_VERSION_V1;
#endif

	if (write_wsc_int_config(&ptr, &ptr_size, "current_wps_version", current_wps_version) < 0) goto ERROR;

	intVal = 0;
	mib_local_mapping_get(MIB_WSC_METHOD, wlanIdx, (void *)&intVal); //root
#ifdef WPS20
	if (current_wps_version == WPS_VERSION_V2) {
		if (intVal == 1) //Pin
			config_method = CONFIG_METHOD_VIRTUAL_PIN | CONFIG_METHOD_KEYPAD;
		else if (intVal == 2) //PBC
			config_method = (CONFIG_METHOD_PHYSICAL_PBC | CONFIG_METHOD_VIRTUAL_PBC);
		else if (intVal == 3) //Pin+PBC
			config_method = (CONFIG_METHOD_VIRTUAL_PIN | CONFIG_METHOD_KEYPAD |  CONFIG_METHOD_PHYSICAL_PBC | CONFIG_METHOD_VIRTUAL_PBC);
	} else
#endif
	{
		//Ethernet(0x2)+Label(0x4)+PushButton(0x80) Bitwise OR
		if (intVal == 1) //Pin+Ethernet
			config_method = (CONFIG_METHOD_ETH | CONFIG_METHOD_PIN);
		else if (intVal == 2) //PBC+Ethernet
			config_method = (CONFIG_METHOD_ETH | CONFIG_METHOD_PBC);
		else if (intVal == 3) //Pin+PBC+Ethernet
			config_method = (CONFIG_METHOD_ETH | CONFIG_METHOD_PIN | CONFIG_METHOD_PBC);
	}

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	config_method &= ~(CONFIG_METHOD_VIRTUAL_PIN | CONFIG_METHOD_PIN );
#endif

	if (write_wsc_int_config(&ptr, &ptr_size, "config_method", config_method) < 0) goto ERROR;

#if 0 //def FOR_DUAL_BAND
	mib_get_s(gMIB_WSC_DISABLE, (void *)&intVal2, sizeof(intVal2));
	if (write_wsc_int_config(&ptr, &ptr_size, "wlan0_wsc_disabled", intVal2) < 0) goto ERROR;
#endif

	intVal2 = Entry->wsc_auth;

	if (write_wsc_int_config(&ptr, &ptr_size, "auth_type", intVal2) < 0) goto ERROR;

	intVal = Entry->wsc_enc;
	if (write_wsc_int_config(&ptr, &ptr_size, "encrypt_type", intVal) < 0) goto ERROR;

	if (intVal == WSC_ENCRYPT_WEP)
		is_wep = 1;

	/*for detial mixed mode info*/
	wlan_encrypt = Entry->encrypt;
	//mib_get_s(gMIB_WLAN_ENCRYPT, (void *)&wlan_encrypt, sizeof(wlan_encrypt));
	wlan_wpa_cipher = Entry->unicastCipher;
	//mib_get_s(gMIB_WLAN_WPA_CIPHER_SUITE, (void *)&wlan_wpa_cipher, sizeof(wlan_wpa_cipher));
	wlan_wpa2_cipher = Entry->wpa2UnicastCipher;
	//mib_get_s(gMIB_WLAN_WPA2_CIPHER_SUITE, (void *)&wlan_wpa2_cipher, sizeof(wlan_wpa2_cipher));

	intVal=0;
	if(wlan_encrypt==6){	// mixed mode
		if(wlan_wpa_cipher==1){
			intVal |= WSC_WPA_TKIP;
		}else if(wlan_wpa_cipher==2){
			intVal |= WSC_WPA_AES;
		}else if(wlan_wpa_cipher==3){
			intVal |= (WSC_WPA_TKIP | WSC_WPA_AES);
		}
		if(wlan_wpa2_cipher==1){
			intVal |= WSC_WPA2_TKIP;
		}else if(wlan_wpa2_cipher==2){
			intVal |= WSC_WPA2_AES;
		}else if(wlan_wpa2_cipher==3){
			intVal |= (WSC_WPA2_TKIP | WSC_WPA2_AES);
		}
	}
	if (write_wsc_int_config(&ptr, &ptr_size, "mixedmode", intVal) < 0) goto ERROR;
	/*for detial mixed mode info*/

	if (is_client) {
		mib_local_mapping_get(MIB_WLAN_NETWORK_TYPE, wlanIdx, (void *)&intVal); //root
		if (intVal == 0)
			intVal = 1;
		else
			intVal = 2;
	}
	else
		intVal = 1;
	if (write_wsc_int_config(&ptr, &ptr_size, "connection_type", intVal) < 0) goto ERROR;

	mib_local_mapping_get(MIB_WSC_MANUAL_ENABLED, wlanIdx, (void *)&intVal); //root
	if (write_wsc_int_config(&ptr, &ptr_size, "manual_config", intVal) < 0) goto ERROR;

	if (is_wep) { // only allow WEP in none-MANUAL mode (configured by external registrar)
		intVal = Entry->encrypt;
		//mib_get_s(gMIB_WLAN_ENCRYPT, (void *)&intVal, sizeof(intVal));
		if (intVal != WIFI_SEC_WEP) {
			printf("WEP mismatched between WPS and host system\n");
			goto ERROR;
		}
		intVal = Entry->wep;
		//mib_get_s(gMIB_WLAN_WEP, (void *)&intVal, sizeof(intVal));
		if (intVal <= WEP_DISABLED || intVal > WEP128) {
			printf("WEP encrypt length error\n");
			goto ERROR;
		}
		wep_key_type = Entry->wepKeyType;
		//mib_get_s(gMIB_WLAN_WEP_KEY_TYPE, (void *)&wep_key_type, sizeof(wep_key_type));
		//mib_get_s(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&wep_transmit_key, sizeof(wep_transmit_key));
		wep_transmit_key = Entry->wepDefaultKey;
		wep_transmit_key++;
		if (write_wsc_int_config(&ptr, &ptr_size, "wep_transmit_key", wep_transmit_key) < 0) goto ERROR;
		if (intVal == WEP64) {
			strcpy(tmpbuf,Entry->wep64Key1);
			//mib_get_s(gMIB_WLAN_WEP64_KEY1, (void *)&tmpbuf, sizeof(tmpbuf));
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			}
			else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key", tmp1) < 0) goto ERROR;

			//mib_get_s(gMIB_WLAN_WEP64_KEY2, (void *)&tmpbuf, sizeof(tmpbuf)); //vap
			strcpy(tmpbuf,Entry->wep64Key2);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			}
			else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key2", tmp1) < 0) goto ERROR;

			//mib_get_s(gMIB_WLAN_WEP64_KEY3, (void *)&tmpbuf, sizeof(tmpbuf)); //vap
			strcpy(tmpbuf,Entry->wep64Key3);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			}
			else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key3", tmp1) < 0) goto ERROR;


			//mib_get_s(gMIB_WLAN_WEP64_KEY4, (void *)&tmpbuf, sizeof(tmpbuf)); //vap
			strcpy(tmpbuf,Entry->wep64Key4);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			}
			else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key4", tmp1) < 0) goto ERROR;
		} else {
			strcpy(tmpbuf, Entry->wep128Key1);
			//mib_get_s(gMIB_WLAN_WEP128_KEY1, (void *)&tmpbuf, sizeof(tmpbuf));
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key", tmp1) < 0) goto ERROR;

			//mib_get_s(gMIB_WLAN_WEP128_KEY2, (void *)&tmpbuf, sizeof(tmpbuf)); //vap
			strcpy(tmpbuf, Entry->wep128Key2);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key2", tmp1) < 0) goto ERROR;

			//mib_get_s(gMIB_WLAN_WEP128_KEY3, (void *)&tmpbuf, sizeof(tmpbuf)); //vap
			strcpy(tmpbuf, Entry->wep128Key3);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key3", tmp1) < 0) goto ERROR;

			//mib_get_s(gMIB_WLAN_WEP128_KEY4, (void *)&tmpbuf, sizeof(tmpbuf)); //vap
			strcpy(tmpbuf, Entry->wep128Key4);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key4", tmp1) < 0) goto ERROR;
		}
	} else {
		strcpy(tmp1, Entry->wpaPSK);
		//mib_get_s(gMIB_WLAN_WPA_PSK, (void *)&tmp1, sizeof(tmp1));
		if (write_wsc_str_config(&ptr, &ptr_size, "network_key", tmp1) < 0) goto ERROR;
	}

	if (write_wsc_str_config(&ptr, &ptr_size, "ssid", Entry->ssid) < 0) goto ERROR;

	mib_get_s(MIB_WSC_PIN, (void *)&tmp1, sizeof(tmp1));
	if (write_wsc_str_config(&ptr, &ptr_size, "pin_code", tmp1) < 0) goto ERROR;

	mib_local_mapping_get(MIB_WLAN_CHAN_NUM, wlanIdx, (void *)&intVal);
	if (intVal > 14)
		intVal = 2;
	else
		intVal = 1;
	if (write_wsc_int_config(&ptr, &ptr_size, "rf_band", intVal) < 0) goto ERROR;

#ifdef FOR_DUAL_BAND
	//wlan_idx = 1;

	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "#=====wlan1 start==========%d\n", intVal, ret_err_write);
	if (ret_err_write) {
		printf("WRITE_WSC_PARAM memcpy error when writing comment:[#=====wlan1 start==========%d]\n", intVal);
		goto ERROR;
	}

	mib_local_mapping_get(MIB_WSC_DISABLE, wlanIdx, (void *)&intVal2);
	if (write_wsc_int_config(&ptr, &ptr_size, "wlan1_wsc_disabled", intVal2) < 0) goto ERROR;

	mib_local_mapping_get(MIB_WLAN_SSID, wlanIdx, (void *)&tmp1);
	if (write_wsc_str_config(&ptr, &ptr_size, "ssid2", tmp1) < 0) goto ERROR;

	mib_local_mapping_get(MIB_WSC_AUTH, wlanIdx, (void *)&intVal2);
	if (write_wsc_int_config(&ptr, &ptr_size, "auth_type2", intVal2) < 0) goto ERROR;

	mib_local_mapping_get(MIB_WSC_ENC, wlanIdx, (void *)&intVal);
	if (write_wsc_int_config(&ptr, &ptr_size, "encrypt_type2", intVal) < 0) goto ERROR;

	if (intVal == ENCRYPT_WEP)
		is_wep = 1;

	/*for detial mixed mode info*/
	mib_local_mapping_get(MIB_WLAN_ENCRYPT, wlanIdx, (void *)&wlan_encrypt);
	mib_local_mapping_get(MIB_WLAN_WPA_CIPHER_SUITE, wlanIdx, (void *)&wlan_wpa_cipher);
	mib_local_mapping_get(MIB_WLAN_WPA2_CIPHER_SUITE, wlanIdx, (void *)&wlan_wpa2_cipher);

	intVal=0;
	if(wlan_encrypt==6){	// mixed mode
		if(wlan_wpa_cipher==1){
			intVal |= WSC_WPA_TKIP;
		}else if(wlan_wpa_cipher==2){
			intVal |= WSC_WPA_AES;
		}else if(wlan_wpa_cipher==3){
			intVal |= (WSC_WPA_TKIP | WSC_WPA_AES);
		}
		if(wlan_wpa2_cipher==1){
			intVal |= WSC_WPA2_TKIP;
		}else if(wlan_wpa2_cipher==2){
			intVal |= WSC_WPA2_AES;
		}else if(wlan_wpa2_cipher==3){
			intVal |= (WSC_WPA2_TKIP | WSC_WPA2_AES);
		}
	}
	if (write_wsc_int_config(&ptr, &ptr_size, "mixedmode2", intVal) < 0) goto ERROR;
	/*for detial mixed mode info*/

/*
	mib_get_s(gMIB_WLAN_BAND2G5G_SELECT, (void *)&intVal, sizeof(intVal));	// 0:2.4g  ; 1:5G   ; 2:both(dual band)
	if(intVal != 2) {
		intVal=1;
		WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "wlan1_wsc_disabled = %d\n",intVal, ret_err_write);
		if (ret_err_write) {
			printf("WRITE_WSC_PARAM memcpy error when writing wlan1_wsc_disabled\n");
			goto ERROR;
		}
	}
	else {
		mib_get_s(gMIB_WSC_DISABLE, (void *)&intVal, sizeof(intVal));
		WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "wlan1_wsc_disabled = %d\n", intVal, ret_err_write);
		if (ret_err_write) {
			printf("WRITE_WSC_PARAM memcpy error when writing wlan1_wsc_disabled\n");
			goto ERROR;
		}
	}
*/
	if (is_wep) { // only allow WEP in none-MANUAL mode (configured by external registrar)
		mib_local_mapping_get(MIB_WLAN_ENCRYPT, wlanIdx, (void *)&intVal);
		if (intVal != _ENCRYPT_WEP_) {
			printf("WEP mismatched between WPS and host system\n");
			goto ERROR;
		}
		mib_local_mapping_get(MIB_WLAN_WEP, wlanIdx, (void *)&intVal);
		if (intVal <= WEP_DISABLED || intVal > WEP128) {
			printf("WEP encrypt length error\n");
			goto ERROR;
		}
		mib_local_mapping_get(MIB_WLAN_WEP_KEY_TYPE, wlanIdx, (void *)&wep_key_type);
		mib_local_mapping_get(MIB_WLAN_WEP_DEFAULT_KEY, wlanIdx, (void *)&wep_transmit_key);
		wep_transmit_key++;
		if (write_wsc_int_config(&ptr, &ptr_size, "wep_transmit_key2", wep_transmit_key) < 0) goto ERROR;

		if (intVal == WEP64) {
			mib_local_mapping_get(MIB_WLAN_WEP64_KEY1, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key2", tmp1) < 0) goto ERROR;

			mib_local_mapping_get(MIB_WLAN_WEP64_KEY2, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key22", tmp1) < 0) goto ERROR;

			mib_local_mapping_get(MIB_WLAN_WEP64_KEY3, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key32", tmp1) < 0) goto ERROR;

			mib_local_mapping_get(MIB_WLAN_WEP64_KEY4, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key42", tmp1) < 0) goto ERROR;
		}
		else {
			mib_local_mapping_get(MIB_WLAN_WEP128_KEY1, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			}
			else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key2", tmp1) < 0) goto ERROR;

			mib_local_mapping_get(MIB_WLAN_WEP128_KEY2, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			}
			else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key22", tmp1) < 0) goto ERROR;

			mib_local_mapping_get(MIB_WLAN_WEP128_KEY3, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			}
			else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key32", tmp1) < 0) goto ERROR;

			mib_local_mapping_get(MIB_WLAN_WEP128_KEY4, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			}
			else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key42", tmp1) < 0) goto ERROR;
		}
	}
	else {
		mib_local_mapping_get(MIB_WLAN_WPA_PSK, wlanIdx, (void *)&tmp1);
		if (write_wsc_str_config(&ptr, &ptr_size, "network_key2", tmp1) < 0) goto ERROR;
	}
	intVal =2 ;
	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "#=====wlan1 end==========%d\n",intVal, ret_err_write);
	if (ret_err_write) {
		printf("WRITE_WSC_PARAM memcpy error when writing comment:[#=====wlan1 end==========%d]\n", intVal);
		goto ERROR;
	}
//	wlan_idx = 0;
#endif //FOR_DUAL_BAND

/*
	mib_get_s(MIB_HW_MODEL_NUM, (void *)&tmp1, sizeof(tmp1));
	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "model_num = \"%s\"\n", tmp1, ret_err_write);
	if (ret_err_write) {
		printf("WRITE_WSC_PARAM memcpy error when writing model_num\n");
		goto ERROR;
	}

	mib_get_s(MIB_HW_SERIAL_NUM, (void *)&tmp1, sizeof(tmp1));
	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "serial_num = \"%s\"\n", tmp1, ret_err_write);
	if (ret_err_write) {
		printf("WRITE_WSC_PARAM memcpy error when writing serial_num\n");
		goto ERROR;
	}
*/
	mib_get_s(MIB_SNMP_SYS_NAME, (void *)&tmp1, sizeof(tmp1));
	if (write_wsc_str_config(&ptr, &ptr_size, "device_name", tmp1) < 0) goto ERROR;

	len = (int)(((long)ptr)-((long)buf));

	if ((fh = open(in, O_RDONLY)) < 0) {
		printf("open() error [%s]!\n", in);
		goto ERROR;
	}

	lseek(fh, 0L, SEEK_SET);
	if (read(fh, ptr, status.st_size) != status.st_size) {
		printf("read() error [%s]!\n", in);
		//return -1;
		goto ERROR;
	}
	close(fh);

	// search UUID field, replace last 12 char with hw mac address
	ptr = strstr(ptr, "uuid =");
	if (ptr) {
		char tmp2[100];
		mib_get_s(MIB_ELAN_MAC_ADDR/*MIB_HW_NIC0_ADDR*/, (void *)&tmp1, sizeof(tmp1));
		convert_bin_to_str(tmp1, 6, tmp2, sizeof(tmp2));
		if (strlen(ptr) < (27+12)) {
			printf("memcpy error when setting uuid: no enough space\n");
			goto ERROR;
		}
		memcpy(ptr+27, tmp2, 12);
	}
#ifdef RESTRICT_PROCESS_PERMISSION
	if ((fh = open(out, O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IWGRP|S_IWOTH)) < 0)
#else
	if ((fh = open(out, O_RDWR|O_CREAT|O_TRUNC)) < 0)
#endif
	{
		printf("open() error [%s]!\n", out);
		goto ERROR;
	}

	if (write(fh, buf, len+status.st_size) != len+status.st_size ) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}

	unsigned int wps_timeout;
	mib_get_s(MIB_WPS_TIMEOUT, &wps_timeout, sizeof(wps_timeout));
	sprintf(tmpbuf, "pbc_walk_time = %u\n", wps_timeout);
	write(fh, tmpbuf, strlen(tmpbuf));

	close(fh);
	free(buf);
#if defined(FOR_DUAL_BAND)
	//wlan_idx = orig_wlan_idx;
#endif
#ifdef FOR_DUAL_BAND
//	wlan_idx = wlan_idx_orig;
#endif //FOR_DUAL_BAND
	return 0;

ERROR:
	if (buf) free(buf);
	if (-1 != fh) close(fh);
#if defined(FOR_DUAL_BAND)
	//wlan_idx = orig_wlan_idx;
#endif
#ifdef FOR_DUAL_BAND
//	wlan_idx = wlan_idx_orig;
#endif //FOR_DUAL_BAND
	return -1;
}
#else
int WPS_updateWscConf(char *in, char *out, int genpin, char *wlanif_name)
{
	int fh = -1;
	struct stat status;
	char *buf = NULL, *ptr;
	unsigned char intVal2, is_client, is_config, is_registrar, is_wep=0, wep_key_type=0, wep_transmit_key=0;
	unsigned char intVal, current_wps_version;
	unsigned char wlan_encrypt=0, wlan_wpa_cipher=0, wlan_wpa2_cipher=0;
	int config_method;
	unsigned char tmpbuf[100], tmp1[100];
	unsigned int ptr_size = 0;
	int len;
	int ret_err_write = 0;
#ifdef WLAN_DUALBAND_CONCURRENT
	unsigned char wlanBand2G5GSelect;
	int orig_wlan_idx;
	int wlan_index = -1;
#endif
	char *token=NULL, *token1=NULL, *savestr1=NULL;
	MIB_CE_MBSSIB_T Entry;
	int i;
	int wlan_wsc_disabled=0;

#if defined(WLAN_DUALBAND_CONCURRENT)
	orig_wlan_idx = wlan_idx;
#endif

	/*
	if ( !mib_init()) {
		printf("Initialize AP MIB failed!\n");
		return -1;
	}
	*/
	if(wlanif_name != NULL) {
        token = strtok_r(wlanif_name," ", &savestr1);
        if(token)
            token1 = strtok_r(NULL," ", &savestr1);
    }
    else {
        token = (char *)WLANIF[0];
    }
#ifdef WLAN_DUALBAND_CONCURRENT
	if(token)
		wlan_index = rtk_wlan_get_mib_entry_by_ifname(&Entry, token);

	if(wlan_index<0){
		goto ERROR;
	}
	else
		wlan_idx = wlan_index;
#endif

	fprintf(stderr, "Writing file %s...\n", out ? out : "");
	mib_get_s(MIB_WSC_PIN, (void *)tmpbuf, sizeof(tmpbuf));
	if (genpin || !strcmp(tmpbuf, "\x0")) {
		#include <sys/time.h>			
		struct timeval tod;
		unsigned long num;
		
		mib_get_s(MIB_ELAN_MAC_ADDR/*MIB_HW_NIC0_ADDR*/, (void *)&tmp1, sizeof(tmp1));
		gettimeofday(&tod , NULL);
		tod.tv_sec += tmp1[4]+tmp1[5];		
		srand(tod.tv_sec);
		num = rand() % 10000000;
		num = num*10 + compute_pin_checksum(num);
		convert_hex_to_ascii((unsigned long)num, tmpbuf);

		mib_set(MIB_WSC_PIN, (void *)tmpbuf);
		mib_update_all();//mib_update(CURRENT_SETTING);		

		printf("Generated PIN = %s\n", tmpbuf);

		if (genpin)
			return 0;
	}

	if (stat(in, &status) < 0) {
		printf("stat() error [%s]!\n", in);
		//return -1;
		goto ERROR;
	}

	buf = malloc(status.st_size+2048);
	if (buf == NULL) {
		printf("malloc() error [%d]!\n", (int)status.st_size+2048);
		//return -1;
		goto ERROR;
	}
	ptr = buf;
	ptr_size = status.st_size + 2048;
	memset(ptr, '\0', ptr_size);

	//mib_get_s(gMIB_WLAN_MODE, (void *)&is_client, sizeof(is_client));
	is_client = Entry.wlanMode;
	mib_get_s(MIB_WSC_CONFIGURED, (void *)&is_config, sizeof(is_config));
	mib_get_s(MIB_WSC_REGISTRAR_ENABLED, (void *)&is_registrar, sizeof(is_registrar));
	if (is_client == WPS_CLIENT_MODE) {
		if (is_registrar)
			intVal = MODE_CLIENT_CONFIG;			
		else {
			if (!is_config)
				intVal = MODE_CLIENT_UNCONFIG;
			else
				intVal = MODE_CLIENT_CONFIG;
		}
	}
	else {
		is_registrar = 1; // always true in AP		
		if (!is_config)
			intVal = MODE_AP_UNCONFIG;
		else {
			if (is_registrar)
				intVal = MODE_AP_PROXY_REGISTRAR;
		}		
	}
	if (write_wsc_int_config(&ptr, &ptr_size, "mode", intVal) < 0) goto ERROR;

	if (is_client)
		intVal = 0;
	else
		mib_get_s(MIB_WSC_UPNP_ENABLED, (void *)&intVal, sizeof(intVal));

	if (write_wsc_int_config(&ptr, &ptr_size, "upnp", intVal) < 0) goto ERROR;

#ifdef WPS20
#ifdef WPS_VERSION_CONFIGURABLE
	if (mib_get_s(MIB_WSC_VERSION, (void *)&current_wps_version, sizeof(current_wps_version)) == 0)
#endif
		current_wps_version = WPS_VERSION_V2;
#else
	current_wps_version = WPS_VERSION_V1;
#endif
	if (write_wsc_int_config(&ptr, &ptr_size, "current_wps_version", current_wps_version) < 0) goto ERROR;

	intVal = 0;
	mib_get_s(MIB_WSC_METHOD, (void *)&intVal, sizeof(intVal));
#ifdef WPS20
#ifdef WPS_VERSION_CONFIGURABLE
	if (current_wps_version == WPS_VERSION_V2) 
#endif
	{
		if (intVal == 1) //Pin
			config_method = CONFIG_METHOD_VIRTUAL_PIN | CONFIG_METHOD_KEYPAD;
		else if (intVal == 2) //PBC
			config_method = (CONFIG_METHOD_PHYSICAL_PBC | CONFIG_METHOD_VIRTUAL_PBC);
		else if (intVal == 3) //Pin+PBC
			config_method = (CONFIG_METHOD_VIRTUAL_PIN | CONFIG_METHOD_KEYPAD |  CONFIG_METHOD_PHYSICAL_PBC | CONFIG_METHOD_VIRTUAL_PBC);
	}
#else
#ifdef WPS_VERSION_CONFIGURABLE
	if (current_wps_version == WPS_VERSION_V1)
#endif
	{
		//Ethernet(0x2)+Label(0x4)+PushButton(0x80) Bitwise OR
		if (intVal == 1) //Pin+Ethernet
			config_method = (CONFIG_METHOD_ETH | CONFIG_METHOD_PIN);
		else if (intVal == 2) //PBC+Ethernet
			config_method = (CONFIG_METHOD_ETH | CONFIG_METHOD_PBC);
		else if (intVal == 3) //Pin+PBC+Ethernet
			config_method = (CONFIG_METHOD_ETH | CONFIG_METHOD_PIN | CONFIG_METHOD_PBC);
	}
#endif
	if(intVal == 1 || intVal == 2 || intVal == 3)
		if (write_wsc_int_config(&ptr, &ptr_size, "config_method", config_method) < 0) goto ERROR;

	if (is_client) {
		mib_get_s(MIB_WLAN_NETWORK_TYPE, (void *)&intVal, sizeof(intVal));
		if (intVal == 0)
			intVal = 1;
		else
			intVal = 2;
	}
	else
		intVal = 1;
	if (write_wsc_int_config(&ptr, &ptr_size, "connection_type", intVal) < 0) goto ERROR;

	mib_get_s(MIB_WSC_MANUAL_ENABLED, (void *)&intVal, sizeof(intVal));
	if (write_wsc_int_config(&ptr, &ptr_size, "manual_config", intVal) < 0) goto ERROR;

	mib_get_s(MIB_WSC_PIN, (void *)&tmp1, sizeof(tmp1));
	if (write_wsc_str_config(&ptr, &ptr_size, "pin_code", tmp1) < 0) goto ERROR;

	//todo
	mib_get_s(MIB_WLAN_CHAN_NUM, (void *)&intVal, sizeof(intVal));
	if (intVal > 14)
		intVal = 2;
	else
		intVal = 1;
	if (write_wsc_int_config(&ptr, &ptr_size, "rf_band", intVal) < 0) goto ERROR;

	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		if(token && strstr(token, WLANIF[i]))
			rtk_wlan_get_mib_entry_by_ifname(&Entry, token);
		else if(token1 && strstr(token1, WLANIF[i]))
			rtk_wlan_get_mib_entry_by_ifname(&Entry, token1);
		else
			rtk_wlan_get_mib_entry_by_ifname(&Entry, WLANIF[i]);
	
		wlan_wsc_disabled = 0;
		/*if  wlanif_name doesn't include "wlan0", disable wlan0 wsc*/
		if((token == NULL || strstr(token, WLANIF[i]) == 0) && (token1 == NULL || strstr(token1, WLANIF[i]) == 0)) {
			wlan_wsc_disabled = 1;
		}
	
		/* for dual band*/
		if(Entry.wlanDisabled ){
			wlan_wsc_disabled=1;
		}

		WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "#=====wlan%d start==========\n", i, ret_err_write);
		if (ret_err_write) {
			printf("WRITE_WSC_PARAM memcpy error when writing comment:[#=====wlan%d start==========]\n", i);
			goto ERROR;
		}

#ifdef WLAN_DUALBAND_CONCURRENT
		WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "wlan%d_wsc_disabled = \"%d\"\n", i, wlan_wsc_disabled? 1:Entry.wsc_disabled, ret_err_write);
		if (ret_err_write) {
			printf("WRITE_WSC_PARAM2 memcpy error when writing 'wlan%d_wsc_disabled'\n", i);
			goto ERROR;
		}
#endif

		WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "auth_type%s = \"%d\"\n", i? "2":"", Entry.wsc_auth, ret_err_write);
		if (ret_err_write) {
			printf("WRITE_WSC_PARAM2 memcpy error when writing 'auth_type%s'\n", i? "2":"");
			goto ERROR;
		}

		WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "encrypt_type%s = \"%d\"\n", i? "2":"", Entry.wsc_enc, ret_err_write);
		if (ret_err_write) {
			printf("WRITE_WSC_PARAM2 memcpy error when writing 'encrypt_type%s'\n", i? "2":"");
			goto ERROR;
		}
		if (Entry.wsc_enc == WSC_ENCRYPT_WEP)
			is_wep = 1;

		/*for detial mixed mode info*/
		//mib_get_s(gMIB_WLAN_ENCRYPT, (void *)&wlan_encrypt, sizeof(wlan_encrypt));
		wlan_encrypt = Entry.encrypt;
		//mib_get_s(gMIB_WLAN_WPA_CIPHER_SUITE, (void *)&wlan_wpa_cipher, sizeof(wlan_wpa_cipher));
		wlan_wpa_cipher = Entry.unicastCipher;
		//mib_get_s(gMIB_WLAN_WPA2_CIPHER_SUITE, (void *)&wlan_wpa2_cipher, sizeof(wlan_wpa2_cipher));
		wlan_wpa2_cipher = Entry.wpa2UnicastCipher;

		intVal=0;
		if(wlan_encrypt==WIFI_SEC_WPA2_MIXED){	// mixed mode
			if(wlan_wpa_cipher==1){
				intVal |= WSC_WPA_TKIP;
			}else if(wlan_wpa_cipher==2){
				intVal |= WSC_WPA_AES;
			}else if(wlan_wpa_cipher==3){
				intVal |= (WSC_WPA_TKIP | WSC_WPA_AES);
			}
			if(wlan_wpa2_cipher==1){
				intVal |= WSC_WPA2_TKIP;
			}else if(wlan_wpa2_cipher==2){
				intVal |= WSC_WPA2_AES;
			}else if(wlan_wpa2_cipher==3){
				intVal |= (WSC_WPA2_TKIP | WSC_WPA2_AES);
			}
		}
		WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "mixedmode%s = \"%d\"\n", i? "2":"", intVal, ret_err_write);
		if (ret_err_write) {
			printf("WRITE_WSC_PARAM2 memcpy error when writing 'mixedmode%s'\n", i? "2":"");
			goto ERROR;
		}
		/*for detial mixed mode info*/

		if (is_wep) { // only allow WEP in none-MANUAL mode (configured by external registrar)
			//mib_get_s(gMIB_WLAN_ENCRYPT, (void *)&intVal, sizeof(intVal));
			intVal = Entry.encrypt;
			if (intVal != WIFI_SEC_WEP) {
				printf("WEP mismatched between WPS and host system\n");
				goto ERROR;
			}
			//mib_get_s(gMIB_WLAN_WEP, (void *)&intVal, sizeof(intVal));
			intVal = Entry.wep;
			if (intVal <= WEP_DISABLED || intVal > WEP128) {
				printf("WEP encrypt length error\n");
				goto ERROR;
			}
			//mib_get_s(gMIB_WLAN_WEP_KEY_TYPE, (void *)&wep_key_type, sizeof(wep_key_type));
			wep_key_type = Entry.wepKeyType;
			//mib_get_s(gMIB_WLAN_WEP_DEFAULT_KEY, (void *)&wep_transmit_key, sizeof(wep_transmit_key));
			//wep_transmit_key++;
			wep_transmit_key = Entry.wepDefaultKey+1;
			WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "wep_transmit_key%s = %d\n", i? "2":"", wep_transmit_key, ret_err_write);
			if (ret_err_write) {
				printf("WRITE_WSC_PARAM2 memcpy error when writing 'wep_transmit_key%s'\n", i? "2":"");
				goto ERROR;
			}
			if (intVal == WEP64) {
				//mib_get_s(gMIB_WLAN_WEP64_KEY1, (void *)&tmpbuf, sizeof(tmpbuf));
				memcpy(tmpbuf, Entry.wep64Key1, WEP64_KEY_LEN);
				if (wep_key_type == KEY_ASCII) {
					memcpy(tmp1, tmpbuf, 5);
					tmp1[5] = '\0';
				} else {
					convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
					tmp1[10] = '\0';
				}
				WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "network_key%s = %s\n", i? "2":"", tmp1, ret_err_write);
				if (ret_err_write) {
					printf("WRITE_WSC_PARAM2 memcpy error when writing 'network_key%s' (_WEP_40_PRIVACY_)\n", i? "2":"");
					goto ERROR;
				}

				//mib_get_s(gMIB_WLAN_WEP64_KEY2, (void *)&tmpbuf, sizeof(tmpbuf));
				memcpy(tmpbuf, Entry.wep64Key2, WEP64_KEY_LEN);
				if (wep_key_type == KEY_ASCII) {
					memcpy(tmp1, tmpbuf, 5);
					tmp1[5] = '\0';
				} else {
					convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
					tmp1[10] = '\0';
				}
				WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "wep_key2%s = %s\n", i? "2":"", tmp1, ret_err_write);
				if (ret_err_write) {
					printf("WRITE_WSC_PARAM2 memcpy error when writing 'wep_key2%s' (_WEP_40_PRIVACY_)\n", i? "2":"");
					goto ERROR;
				}

				//mib_get_s(gMIB_WLAN_WEP64_KEY3, (void *)&tmpbuf, sizeof(tmpbuf));
				memcpy(tmpbuf, Entry.wep64Key3, WEP64_KEY_LEN);
				if (wep_key_type == KEY_ASCII) {
					memcpy(tmp1, tmpbuf, 5);
					tmp1[5] = '\0';
				} else {
					convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
					tmp1[10] = '\0';
				}
				WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "wep_key3%s = %s\n", i? "2":"", tmp1, ret_err_write);
				if (ret_err_write) {
					printf("WRITE_WSC_PARAM2 memcpy error when writing 'wep_key3%s' (_WEP_40_PRIVACY_)\n", i? "2":"");
					goto ERROR;
				}

				//mib_get_s(gMIB_WLAN_WEP64_KEY4, (void *)&tmpbuf, sizeof(tmpbuf));
				memcpy(tmpbuf, Entry.wep64Key4, WEP64_KEY_LEN);
				if (wep_key_type == KEY_ASCII) {
					memcpy(tmp1, tmpbuf, 5);
					tmp1[5] = '\0';
				} else {
					convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
					tmp1[10] = '\0';
				}
				WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "wep_key4%s = %s\n", i? "2":"", tmp1, ret_err_write);
				if (ret_err_write) {
					printf("WRITE_WSC_PARAM2 memcpy error when writing 'wep_key4%s' (_WEP_40_PRIVACY_)\n", i? "2":"");
					goto ERROR;
				}
			} else {
				//mib_get_s(gMIB_WLAN_WEP128_KEY1, (void *)&tmpbuf, sizeof(tmpbuf));
				memcpy(tmpbuf, Entry.wep128Key1, WEP128_KEY_LEN);
				if (wep_key_type == KEY_ASCII) {
					memcpy(tmp1, tmpbuf, 13);
					tmp1[13] = '\0';
				} else {
					convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
					tmp1[26] = '\0';
				}
				WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "network_key%s = %s\n", i? "2":"", tmp1, ret_err_write);
				if (ret_err_write) {
					printf("WRITE_WSC_PARAM2 memcpy error when writing 'network_key%s' (_WEP_104_PRIVACY_)\n", i? "2":"");
					goto ERROR;
				}

				//mib_get_s(gMIB_WLAN_WEP128_KEY2, (void *)&tmpbuf, sizeof(tmpbuf));
				memcpy(tmpbuf, Entry.wep128Key2, WEP128_KEY_LEN);
				if (wep_key_type == KEY_ASCII) {
					memcpy(tmp1, tmpbuf, 13);
					tmp1[13] = '\0';
				} else {
					convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
					tmp1[26] = '\0';
				}
				WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "wep_key2%s = %s\n", i? "2":"", tmp1, ret_err_write);
				if (ret_err_write) {
					printf("WRITE_WSC_PARAM2 memcpy error when writing 'wep_key2%s' (_WEP_104_PRIVACY_)\n", i? "2":"");
					goto ERROR;
				}

				//mib_get_s(gMIB_WLAN_WEP128_KEY3, (void *)&tmpbuf, sizeof(tmpbuf));
				memcpy(tmpbuf, Entry.wep128Key3, WEP128_KEY_LEN);
				if (wep_key_type == KEY_ASCII) {
					memcpy(tmp1, tmpbuf, 13);
					tmp1[13] = '\0';
				} else {
					convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
					tmp1[26] = '\0';
				}
				WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "wep_key3%s = %s\n", i? "2":"", tmp1, ret_err_write);
				if (ret_err_write) {
					printf("WRITE_WSC_PARAM2 memcpy error when writing 'wep_key3%s' (_WEP_104_PRIVACY_)\n", i? "2":"");
					goto ERROR;
				}

				//mib_get_s(gMIB_WLAN_WEP128_KEY4, (void *)&tmpbuf, sizeof(tmpbuf));
				memcpy(tmpbuf, Entry.wep128Key4, WEP128_KEY_LEN);
				if (wep_key_type == KEY_ASCII) {
					memcpy(tmp1, tmpbuf, 13);
					tmp1[13] = '\0';
				} else {
					convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
					tmp1[26] = '\0';
				}
				WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "wep_key4%s = %s\n", i? "2":"", tmp1, ret_err_write);
				if (ret_err_write) {
					printf("WRITE_WSC_PARAM2 memcpy error when writing 'wep_key4%s' (_WEP_104_PRIVACY_)\n", i? "2":"");
					goto ERROR;
				}
			}
		} else {
			//mib_get_s(gMIB_WLAN_WPA_PSK, (void *)&tmp1, sizeof(tmp1));		
			WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "network_key%s = %s\n", i? "2":"", Entry.wpaPSK, ret_err_write);
			if (ret_err_write) {
				printf("WRITE_WSC_PARAM2 memcpy error when writing 'network_key%s'\n", i? "2":"");
				goto ERROR;
			}
		}

		//mib_get_s(gMIB_WLAN_SSID, (void *)&tmp1, sizeof(tmp1));	
		WRITE_WSC_PARAM2(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "ssid%s = \"%s\"\n", i? "2":"", Entry.ssid, ret_err_write);
		if (ret_err_write) {
			printf("WRITE_WSC_PARAM2 memcpy error when writing 'ssid%s'\n", i? "2":"");
			goto ERROR;
		}

		WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "#=====wlan%d end==========\n", i, ret_err_write);
		if (ret_err_write) {
			printf("WRITE_WSC_PARAM memcpy error when writing comment:[#=====wlan%d end==========]\n", i);
			goto ERROR;
		}
	}
#if 0	
//	}
//	else {			
		mib_get_s(MIB_WSC_PSK, (void *)&tmp1, sizeof(tmp1));
		if (write_wsc_str_config(&ptr, &ptr_size, "network_key", tmp1) < 0) goto ERROR;
		
		mib_get_s(MIB_WSC_SSID, (void *)&tmp1, sizeof(tmp1));
		if (write_wsc_str_config(&ptr, &ptr_size, "ssid", tmp1) < 0) goto ERROR;
//	}
#endif


#if 0 //def FOR_DUAL_BAND
	wlan_idx = 1;

	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "#=====wlan1 start==========%d\n", intVal, ret_err_write);
	if (ret_err_write) {
		printf("WRITE_WSC_PARAM memcpy error when writing comment:[#=====wlan1 start==========%d]\n", intVal);
		goto ERROR;
	}

	mib_get_s(gMIB_WSC_DISABLE, (void *)&intVal2, sizeof(intVal2));
	if (write_wsc_int_config(&ptr, &ptr_size, "wlan1_wsc_disabled", intVal2) < 0) goto ERROR;

	mib_get_s(gMIB_WLAN_SSID, (void *)&tmp1, sizeof(tmp1)); 
	if (write_wsc_str_config(&ptr, &ptr_size, "ssid2", tmp1) < 0) goto ERROR;

	mib_get_s(gMIB_WSC_AUTH, (void *)&intVal2, sizeof(intVal2));
	if (write_wsc_int_config(&ptr, &ptr_size, "auth_type2", intVal2) < 0) goto ERROR;

	mib_get_s(gMIB_WSC_ENC, (void *)&intVal, sizeof(intVal));
	if (write_wsc_int_config(&ptr, &ptr_size, "encrypt_type2", intVal) < 0) goto ERROR;

	if (intVal == ENCRYPT_WEP)
		is_wep = 1;
	
	/*for detial mixed mode info*/
	mib_get_s(gMIB_WLAN_ENCRYPT, (void *)&wlan_encrypt, sizeof(wlan_encrypt));
	mib_get_s(gMIB_WLAN_WPA_CIPHER_SUITE, (void *)&wlan_wpa_cipher, sizeof(wlan_wpa_cipher));
	mib_get_s(gMIB_WLAN_WPA2_CIPHER_SUITE, (void *)&wlan_wpa2_cipher, sizeof(wlan_wpa2_cipher));
	
	intVal=0;	
	if(wlan_encrypt==6){	// mixed mode
		if(wlan_wpa_cipher==1){
			intVal |= WSC_WPA_TKIP;
		}else if(wlan_wpa_cipher==2){
			intVal |= WSC_WPA_AES;		
		}else if(wlan_wpa_cipher==3){
			intVal |= (WSC_WPA_TKIP | WSC_WPA_AES);		
		}
		if(wlan_wpa2_cipher==1){
			intVal |= WSC_WPA2_TKIP;
		}else if(wlan_wpa2_cipher==2){
			intVal |= WSC_WPA2_AES;		
		}else if(wlan_wpa2_cipher==3){
			intVal |= (WSC_WPA2_TKIP | WSC_WPA2_AES);		
		}		
	}
	if (write_wsc_int_config(&ptr, &ptr_size, "mixedmode2", intVal) < 0) goto ERROR;
	/*for detial mixed mode info*/
	
/* 
	mib_get_s(gMIB_WLAN_BAND2G5G_SELECT, (void *)&intVal, sizeof(intVal));	// 0:2.4g  ; 1:5G   ; 2:both(dual band)
	if(intVal != 2) {
		intVal=1;
		WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "wlan1_wsc_disabled = %d\n",intVal, ret_err_write);
		if (ret_err_write) {
			printf("WRITE_WSC_PARAM memcpy error when writing wlan1_wsc_disabled\n");
			goto ERROR;
		}
	}
	else {
		mib_get_s(gMIB_WSC_DISABLE, (void *)&intVal, sizeof(intVal));
		WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "wlan1_wsc_disabled = %d\n", intVal, ret_err_write);
		if (ret_err_write) {
			printf("WRITE_WSC_PARAM memcpy error when writing wlan1_wsc_disabled\n");
			goto ERROR;
		}
	}
*/
	if (is_wep) { // only allow WEP in none-MANUAL mode (configured by external registrar)
		mib_get_s(gMIB_WLAN_ENCRYPT, (void *)&intVal, sizeof(intVal));
		if (intVal != _ENCRYPT_WEP_) {
			printf("WEP mismatched between WPS and host system\n");
			goto ERROR;
		}
		mib_get_s(gMIB_WLAN_WEP, (void *)&intVal, sizeof(intVal));
		if (intVal <= WEP_DISABLED || intVal > WEP128) {
			printf("WEP encrypt length error\n");
			goto ERROR;
		}
		mib_get_s(gMIB_WLAN_WEP_KEY_TYPE, (void *)&wep_key_type, sizeof(wep_key_type));
		mib_get_s(gMIB_WLAN_WEP_DEFAULT_KEY, (void *)&wep_transmit_key, sizeof(wep_transmit_key));
		wep_transmit_key++;
		if (write_wsc_int_config(&ptr, &ptr_size, "wep_transmit_key2", wep_transmit_key) < 0) goto ERROR;

		if (intVal == WEP64) {
			mib_get_s(gMIB_WLAN_WEP64_KEY1, (void *)&tmpbuf, sizeof(tmpbuf));
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key2", tmp1) < 0) goto ERROR;

			mib_get_s(gMIB_WLAN_WEP64_KEY2, (void *)&tmpbuf, sizeof(tmpbuf));
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key22", tmp1) < 0) goto ERROR;

			mib_get_s(gMIB_WLAN_WEP64_KEY3, (void *)&tmpbuf, sizeof(tmpbuf));
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			}
			else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key32", tmp1) < 0) goto ERROR;

			mib_get_s(gMIB_WLAN_WEP64_KEY4, (void *)&tmpbuf, sizeof(tmpbuf));
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key42", tmp1) < 0) goto ERROR;
		} else {
			mib_get_s(gMIB_WLAN_WEP128_KEY1, (void *)&tmpbuf, sizeof(tmpbuf));
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key2", tmp1) < 0) goto ERROR;

			mib_get_s(gMIB_WLAN_WEP128_KEY2, (void *)&tmpbuf, sizeof(tmpbuf));
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key22", tmp1) < 0) goto ERROR;

			mib_get_s(gMIB_WLAN_WEP128_KEY3, (void *)&tmpbuf, sizeof(tmpbuf));
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key32", tmp1) < 0) goto ERROR;

			mib_get_s(gMIB_WLAN_WEP128_KEY4, (void *)&tmpbuf, sizeof(tmpbuf));
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key42", tmp1) < 0) goto ERROR;
		}
	} else {
		mib_get_s(gMIB_WLAN_WPA_PSK, (void *)&tmp1, sizeof(tmp1));
		if (write_wsc_str_config(&ptr, &ptr_size, "network_key2", tmp1) < 0) goto ERROR;
	}
	intVal =2 ;
	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "#=====wlan1 end==========%d\n",intVal, ret_err_write);
	if (ret_err_write) {
		printf("WRITE_WSC_PARAM memcpy error when writing comment:[#=====wlan1 end==========%d]\n", intVal);
		goto ERROR;
	}
//	wlan_idx = 0;
#endif //FOR_DUAL_BAND

/*
	mib_get_s(MIB_HW_MODEL_NUM, (void *)&tmp1, sizeof(tmp1));	
	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "model_num = \"%s\"\n", tmp1, ret_err_write);
	if (ret_err_write) {
		printf("WRITE_WSC_PARAM memcpy error when writing model_num\n");
		goto ERROR;
	}

	mib_get_s(MIB_HW_SERIAL_NUM, (void *)&tmp1, sizeof(tmp1));	
	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "serial_num = \"%s\"\n", tmp1, ret_err_write);
	if (ret_err_write) {
		printf("WRITE_WSC_PARAM memcpy error when writing serial_num\n");
		goto ERROR;
	}
*/
	mib_get_s(MIB_SNMP_SYS_NAME, (void *)&tmp1, sizeof(tmp1));
	if (write_wsc_str_config(&ptr, &ptr_size, "device_name", tmp1) < 0) goto ERROR;

	len = (int)(((long)ptr)-((long)buf));

	if ((fh = open(in, O_RDONLY)) < 0) {
		printf("open() error [%s]!\n", in);
		goto ERROR;
	}

	lseek(fh, 0L, SEEK_SET);
	if (read(fh, ptr, status.st_size) != status.st_size) {		
		printf("read() error [%s]!\n", in);
		goto ERROR;
	}
	close(fh);

	// search UUID field, replace last 12 char with hw mac address
	ptr = strstr(ptr, "uuid =");
	if (ptr) {
		char tmp2[100];
		mib_get_s(MIB_ELAN_MAC_ADDR/*MIB_HW_NIC0_ADDR*/, (void *)&tmp1, sizeof(tmp1));	
		convert_bin_to_str(tmp1, 6, tmp2, sizeof(tmp2));
		if (strlen(ptr) < (27+12)) {
			printf("memcpy error when setting uuid: no enough space\n");
			goto ERROR_WITHOUT_CLOSE;
		}
		memcpy(ptr+27, tmp2, 12);		
	}
#ifdef RESTRICT_PROCESS_PERMISSION
	if ((fh = open(out, O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IWGRP|S_IWOTH)) < 0)
#else
	if ((fh = open(out, O_RDWR|O_CREAT|O_TRUNC)) < 0)
#endif
	{
		printf("open() error [%s]!\n", out);
		goto ERROR;
	}

	if (write(fh, buf, len+status.st_size) != len+status.st_size ) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}
	close(fh);
	free(buf);
#ifdef WLAN_DUALBAND_CONCURRENT
	wlan_idx = orig_wlan_idx;
#endif
	return 0;

ERROR:
	if (-1 != fh) close(fh);
ERROR_WITHOUT_CLOSE:
	if (buf) free(buf);
#ifdef WLAN_DUALBAND_CONCURRENT
	wlan_idx = orig_wlan_idx;
#endif
	return -1;
}
#endif
#endif

#ifdef HS2_SUPPORT
int start_hs2()
{
	int status = 0, i;
	MIB_CE_MBSSIB_T Entry;
	unsigned char wlan_hs2_enabled=0;
	int wlan_disabled=0, wlan_mode=0;
	char iface_name[16];
	char tmp_config[30]={0};
	char hs2_config[200]= {0};
	char tmpBuff[100];
#if defined(WLAN_DUALBAND_CONCURRENT)
	int original_wlan_idx = wlan_idx;
#endif

	if (isFileExist("/bin/hs2")) {
		for(i=0;i<NUM_WLAN_INTERFACE;i++){
			int isWLANEnabled=0, isAP=0, isHs2Enabled=0;
			sprintf(iface_name, "%s", WLANIF[i]);
			if(SetWlan_idx(iface_name)){
				mib_get( MIB_WLAN_DISABLED, (void *)&wlan_disabled);
				mib_get( MIB_WLAN_MODE, (void *)&wlan_mode);
				mib_get( MIB_WLAN_HS2_ENABLED, (void *)&wlan_hs2_enabled);
				if(wlan_disabled==0)
					isWLANEnabled=1;
				if(wlan_mode ==0 || wlan_mode ==3 || wlan_mode ==4 || wlan_mode ==6)
					isAP=1;
				if(wlan_hs2_enabled==1)
					isHs2Enabled=1;

				if(isWLANEnabled==1 && isAP==1 && isHs2Enabled==1){
					sprintf(tmpBuff, "/etc/hs2_%s_map.conf", iface_name);
					if(isFileExist(tmpBuff)) {
						if(hs2_config[0]==0){
							sprintf(hs2_config, "-c /etc/hs2_%s_map.conf", iface_name);
						}else{
							sprintf(tmp_config, " -c /etc/hs2_%s_map.conf", iface_name);
							strcat(hs2_config, tmp_config);
						}
					}
					else {
						printf("Wlan%d HS2 config file is missing!\n", i);
					}
				}
				// else {
				// 	printf("[wlan%d] %s (%d) - WlanEnabled: %d isAP: %d isHs2Enabled: %d\n", i, __FUNCTION__, __LINE__, isWLANEnabled, isAP, isHs2Enabled);
				// }
			}
		}

		if (hs2_config[0] != 0){
			sprintf(tmpBuff, "/bin/hs2 %s ", hs2_config);
			system(tmpBuff);
		}
	}
	else {
		printf("/bin/hs2 does not exist\n");
	}

#if defined(WLAN_DUALBAND_CONCURRENT)
	wlan_idx = original_wlan_idx;
#endif

	return status;
}
#endif

#if 0//ndef CONFIG_RTK_DEV_AP
int start_WPS(int run_daemon)
{
#ifndef WLAN_WPS_VAP
	int status=0;
	unsigned char encrypt;
	int retry;
	unsigned char wsc_disable;
	unsigned char wlan_mode;
	unsigned char wlan_nettype;
	unsigned char wpa_auth;
	char *cmd_opt[16];
	int cmd_cnt = 0; int idx;
	int wscd_pid_fd = -1;
	int i;
	unsigned int enableWscIf = 0;
	int orig_wlan_idx;
	unsigned char no_wlan;
	char fifo_buf[32], fifo_buf2[32];
#if defined(WLAN_DUALBAND_CONCURRENT)
	char wlanBand2G5GSelect;
	orig_wlan_idx = wlan_idx;
#endif
	char wsc_conf_filename[40]={0};
	char wlan_interface_name[40]={0};
	char client_interface_name[40]={0};
	char ifname[IFNAMSIZ]={0};
	MIB_CE_MBSSIB_T Entry;
	char *token=NULL, *savestr1=NULL;

//check root
	for(i = 0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		if(mib_chain_get(MIB_MBSSIB_TBL, 0, &Entry) == 0)
			printf("\n %s %d\n", __func__, __LINE__);
		wsc_disable = Entry.wsc_disabled;
		no_wlan = Entry.wlanDisabled;
		wlan_mode = Entry.wlanMode;
		wpa_auth = Entry.wpaAuth;
		encrypt = Entry.encrypt;
		
		//mib_get_s(MIB_WSC_DISABLE, (void *)&wsc_disable, sizeof(wsc_disable));
		//mib_get_s(MIB_WLAN_DISABLED, (void *)&no_wlan, sizeof(no_wlan));
		//mib_get_s(MIB_WLAN_MODE, (void *)&wlan_mode, sizeof(wlan_mode));
		mib_get_s(MIB_WLAN_NETWORK_TYPE, (void *)&wlan_nettype, sizeof(wlan_nettype));
		//mib_get_s(MIB_WLAN_WPA_AUTH, (void *)&wpa_auth, sizeof(wpa_auth));
		//mib_get_s(MIB_WLAN_ENCRYPT, (void *)&encrypt, sizeof(encrypt));

		if(no_wlan || wsc_disable || is8021xEnabled(0))
			continue;
		else if(wlan_mode == CLIENT_MODE) {
			//fixme not support on webpage
			//if(wlan_nettype != INFRASTRUCTURE)
				continue;
		}
		else if(wlan_mode == AP_MODE) {
			if((encrypt >= WIFI_SEC_WPA) && (wpa_auth == WPA_AUTH_AUTO))
				continue;
			if(wlan_interface_name[0]==0)
				snprintf(wlan_interface_name, sizeof(wlan_interface_name), "%s", WLANIF[i]);
			else{
				strcat(wlan_interface_name, " ");
				strcat(wlan_interface_name, WLANIF[i]);
			}
		}
		enableWscIf |= (1<<i);
	}
	if(enableWscIf)
		useAuth_RootIf |= enableWscIf;
//check vxd

	//check root
	for(i = 0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		wlan_getEntry(&Entry, WLAN_REPEATER_ITF_INDEX);
		wsc_disable = Entry.wsc_disabled;
		no_wlan = Entry.wlanDisabled;
		wlan_mode = Entry.wlanMode;
		wpa_auth = Entry.wpaAuth;
		encrypt = Entry.encrypt;
		
		//mib_get_s(MIB_WSC_DISABLE, (void *)&wsc_disable, sizeof(wsc_disable));
		//mib_get_s(MIB_WLAN_DISABLED, (void *)&no_wlan, sizeof(no_wlan));
		//mib_get_s(MIB_WLAN_MODE, (void *)&wlan_mode, sizeof(wlan_mode));
		mib_get_s(MIB_WLAN_NETWORK_TYPE, (void *)&wlan_nettype, sizeof(wlan_nettype));
		//mib_get_s(MIB_WLAN_WPA_AUTH, (void *)&wpa_auth, sizeof(wpa_auth));
		//mib_get_s(MIB_WLAN_ENCRYPT, (void *)&encrypt, sizeof(encrypt));

		if(no_wlan || wsc_disable)
			continue;
		else if(wlan_mode == CLIENT_MODE) {
			if(wlan_nettype != INFRASTRUCTURE)
				continue;
			snprintf(ifname, sizeof(ifname), "%s-vxd", WLANIF[i]);
			if(client_interface_name[0]==0)
				snprintf(client_interface_name, sizeof(client_interface_name), "%s-vxd", WLANIF[i]);
			else{
				strcat(client_interface_name, " ");
				strcat(client_interface_name, ifname);
			}
		}
		else if(wlan_mode == AP_MODE) {
				continue;	
		}
		strcpy(para_iwctrl[wlan_num], ifname);
		wlan_num++;
	}

	if(wlan_interface_name[0])
		start_wsc_deamon(wlan_interface_name, AP_MODE);

	if(client_interface_name[0])
	{
		token = strtok_r(client_interface_name," ", &savestr1);
	    do {
	        start_wsc_deamon(token, CLIENT_MODE);                
	        token = strtok_r(NULL," ", &savestr1);
	    }while(token != NULL);
	}

#if 0
	if(!enableWscIf)
		goto WSC_DISABLE;
	else
		useAuth_RootIf |= enableWscIf;

	if(run_daemon==0)
		goto WSC_DISABLE;

	fprintf(stderr, "START WPS SETUP!\n\n\n");
	cmd_opt[cmd_cnt++] = "";

	if (wlan_mode == CLIENT_MODE) {
		cmd_opt[cmd_cnt++] = "-mode";
		cmd_opt[cmd_cnt++] = "2";
	} else {
		cmd_opt[cmd_cnt++] = "-start";
	}

#if defined(WLAN_DUALBAND_CONCURRENT)
//	if (both_band_ap == 1)
	if(enableWscIf == 3){
		cmd_opt[cmd_cnt++] = "-both_band_ap";
		cmd_opt[cmd_cnt++] = "-w";
		cmd_opt[cmd_cnt++] = (char *)WLANIF[0];
	}
	else{
		if(enableWscIf & 1){
			cmd_opt[cmd_cnt++] = "-w";
			cmd_opt[cmd_cnt++] = (char *)WLANIF[0];
		}
		else{
			cmd_opt[cmd_cnt++] = "-w";
			cmd_opt[cmd_cnt++] = (char *)WLANIF[1];
		}
	}
#endif

	cmd_opt[cmd_cnt++] = "-c";
	cmd_opt[cmd_cnt++] = (char *)WSCD_CONF;
#if !defined(WLAN_DUALBAND_CONCURRENT)
	cmd_opt[cmd_cnt++] = "-w";
	cmd_opt[cmd_cnt++] = (char *)WLANIF[0];
#endif
	//strcat(cmd, " -c /var/wscd.conf -w wlan0");

	if (enableWscIf & 1) { // use iwcontrol
		cmd_opt[cmd_cnt++] = "-fi";
		snprintf(fifo_buf, 32, (char *)WSCD_FIFO, WLANIF[0]);
		cmd_opt[cmd_cnt++] = (char *)fifo_buf;
		//strcat(cmd, " -fi /var/wscd-wlan0.fifo");
	}
#if defined(WLAN_DUALBAND_CONCURRENT)
	if (enableWscIf & 2){
		cmd_opt[cmd_cnt++] = "-fi2";
		snprintf(fifo_buf2, 32, (char *)WSCD_FIFO, WLANIF[1]);
		cmd_opt[cmd_cnt++] = (char *)fifo_buf2;
	}
#endif

	//cmd_opt[cmd_cnt++] = "-debug";
	//strcat(cmd, " -debug");
	//strcat(cmd, " &");
	#define TARGDIR "/var/wps/"
	#define SIMPLECFG "simplecfgservice.xml"
	//status |= va_cmd("/bin/flash", 3, 1, "upd-wsc-conf", "/etc/wscd.conf", "/var/wscd.conf");
	status |= va_cmd("/bin/mkdir", 2, 1, "-p", TARGDIR);
	status |= va_cmd("/bin/cp", 2, 1, "/etc/" SIMPLECFG, TARGDIR);

	cmd_opt[cmd_cnt] = 0;
	printf("CMD ARGS: ");
	for (idx=0; idx<cmd_cnt;idx++)
		printf("%s ", cmd_opt[idx]);
	printf("\n");

	status |= do_cmd(WSC_DAEMON_PROG, cmd_opt, 0);

	if (enableWscIf & 1) {
		retry = 0;
		snprintf(fifo_buf, sizeof(fifo_buf), WSCD_FIFO, WLANIF[0]);
		while ((wscd_pid_fd = open((char *)fifo_buf, O_WRONLY|O_NONBLOCK)) == -1)
		{
			usleep(100000);
			retry ++;

			if (retry > 10) {
				printf("wscd fifo timeout, abort\n");
				break;
			}
		}

		if(wscd_pid_fd!=-1) close(wscd_pid_fd);/*jiunming, close the opened fd*/
	}

#if defined(WLAN_DUALBAND_CONCURRENT)
	if (enableWscIf & 2) {
		retry = 0;
		snprintf(fifo_buf, sizeof(fifo_buf), WSCD_FIFO, WLANIF[1]);
		while ((wscd_pid_fd = open((char *)fifo_buf, O_WRONLY|O_NONBLOCK)) == -1)
		{
			usleep(100000);
			retry ++;

			if (retry > 10) {
				printf("wscd fifo timeout, abort\n");
				break;
			}
		}

		if(wscd_pid_fd!=-1) close(wscd_pid_fd);
	}
#endif
#endif
WSC_DISABLE:
#if defined(WLAN_DUALBAND_CONCURRENT)
	wlan_idx = orig_wlan_idx;
#endif
	return status;
#else
	char wscd_pid_name[32];
	char wscd_fifo_name[32];
	char wscd_conf_name[32];
	int wsc_pid_fd=-1;

	MIB_CE_MBSSIB_T Entry;
	int orig_wlan_idx, i, j;
	char ifname[16];
	int status = 0;
	unsigned char wps_ssid;
	unsigned char vChar;
	unsigned char wsc_disable;
	unsigned char wlan_nettype;
	unsigned char wlan_mode;
#ifdef YUEME_3_0_SPEC
	unsigned char no_wlan;
#endif
	int retry=0;
#ifdef WLAN_WPS_MULTI_DAEMON
	char wscd_button_procname[32];
#endif

	//wlan_num = 0; /*reset to 0,jiunming*/
	//useAuth_RootIf = 0;  /*reset to 0 */

#ifndef WLAN_WPS_MULTI_DAEMON
	mib_get_s(MIB_WPS_ENABLE, &vChar, sizeof(vChar));
	if(vChar == 0)
		goto no_wsc;
#endif

	orig_wlan_idx = wlan_idx;

	for(j=0;j<NUM_WLAN_INTERFACE;j++){
		wlan_idx = j;
#ifdef YUEME_3_0_SPEC
		mib_get_s(MIB_WLAN_MODULE_DISABLED, (void *)&no_wlan, sizeof(no_wlan));
		if(no_wlan)
			continue;
#endif
		for ( i=0; i<=WLAN_SSID_NUM; i++) {

			mib_chain_get(MIB_MBSSIB_TBL, i, (void *)&Entry);

			rtk_wlan_get_ifname(j, i, ifname);

			wsc_disable = check_wps_enc(&Entry)? 0:1;

			mib_get_s(MIB_WLAN_MODE, (void *)&wlan_mode, sizeof(wlan_mode));
			mib_get_s(MIB_WLAN_NETWORK_TYPE, (void *)&wlan_nettype, sizeof(wlan_nettype));
			//mib_get_s(MIB_WLAN_WPA_AUTH, (void *)&wpa_auth, sizeof(wpa_auth));
			//mib_get_s(MIB_WLAN_ENCRYPT, (void *)&encrypt, sizeof(encrypt));
#ifndef WLAN_WPS_MULTI_DAEMON
			mib_get_s(MIB_WPS_SSID, &wps_ssid, sizeof(wps_ssid));
#endif

			if(Entry.wlanDisabled || wsc_disable)
				continue;
#ifdef WLAN_WPS_MULTI_DAEMON
			else if(Entry.wsc_disabled)
				continue;
#else
			else if(!check_is_wps_ssid(i, wps_ssid))
				continue;
#endif
			else if(wlan_mode == CLIENT_MODE) {
				if(wlan_nettype != INFRASTRUCTURE)
					continue;
			}
			else if(wlan_mode == AP_MODE) {
				if((Entry.encrypt >= WIFI_SEC_WPA) && (Entry.wpaAuth == WPA_AUTH_AUTO))
					continue;
			}

			if(run_daemon == 0)
				goto skip_running_wscd;

			snprintf(wscd_pid_name, 32, "/var/run/wscd-%s.pid", ifname);

			int wscd_pid = read_pid(wscd_pid_name);
			if(wscd_pid > 0 && kill(wscd_pid, 0)==0)
				goto skip_running_wscd;


			#define TARGDIR "/var/wps/"
			#define SIMPLECFG "simplecfgservice.xml"
			//status |= va_cmd("/bin/flash", 3, 1, "upd-wsc-conf", "/etc/wscd.conf", "/var/wscd.conf");
			status |= va_cmd("/bin/mkdir", 2, 1, "-p", TARGDIR);
			status |= va_cmd("/bin/cp", 2, 1, "/etc/" SIMPLECFG, TARGDIR);

			sprintf(wscd_conf_name, "/var/wscd-%s.conf", ifname);
			sprintf(wscd_fifo_name, "/var/wscd-%s.fifo", ifname);
			//status|=generateWpaConf(para_auth_conf, 0, &Entry);
			status|=WPS_updateWscConf("/etc/wscd.conf", wscd_conf_name, 0, &Entry, i, j);

#ifdef WLAN_WPS_MULTI_DAEMON
			memset(wscd_button_procname, '\0', sizeof(wscd_button_procname));

			if(j==0 && i==0) //only trigger push button with wlan0
				snprintf(wscd_button_procname, sizeof(wscd_button_procname), "/proc/gpio");

			if(wscd_button_procname[0] == '\0')
				status|=va_niced_cmd(WSC_DAEMON_PROG, 9, 0, "-start", "-c", wscd_conf_name, "-w", ifname, "-fi", wscd_fifo_name, "-proc", "-");
			else
				status|=va_niced_cmd(WSC_DAEMON_PROG, 9, 0, "-start", "-c", wscd_conf_name, "-w", ifname, "-fi", wscd_fifo_name, "-proc", wscd_button_procname);
#else
			status|=va_niced_cmd(WSC_DAEMON_PROG, 7, 0, "-start", "-c", wscd_conf_name, "-w", ifname, "-fi", wscd_fifo_name);
#endif

			// fix the depency problem
			// check fifo
			retry = 10;
			while (--retry && ((wsc_pid_fd = open(wscd_fifo_name, O_WRONLY)) == -1))
			{
				usleep(30000);
			}
			retry = 10;

			while(--retry && (read_pid(wscd_pid_name) < 0))
			{
				//printf("WSCD is not running. Please wait!\n");
				usleep(300000);
			}

			if(wsc_pid_fd!=-1) close(wsc_pid_fd);/*jiunming, close the opened fd*/

skip_running_wscd:

			if (i == WLAN_ROOT_ITF_INDEX){ // Root
					/* 2010-10-27 krammer :  use bit map to record what wlan root interface is use for auth*/

				useAuth_RootIf |= (1<<wlan_idx);//bit 0 ==> wlan0, bit 1 ==>wlan1
			}
			else {
				strcpy(para_iwctrl[wlan_num], ifname);
				wlan_num++;
			}

		}
	}
	wlan_idx = orig_wlan_idx;
no_wsc:
	if(run_daemon == 1)
		startSSDP();
	return status;

#endif
}
#else
#ifdef WLAN_WPS_VAP
int WPS_updateWscConf(char *in, char *out, int genpin, MIB_CE_MBSSIB_T *Entry, int vwlan_idx, int wlanIdx)
{
	int fh = -1;
	struct stat status;
	char *buf = NULL, *ptr;
	unsigned char intVal2, is_client, is_config, is_registrar, is_wep=0, wep_key_type=0, wep_transmit_key=0;
	unsigned char intVal, current_wps_version;
	unsigned char wlan_encrypt=0, wlan_wpa_cipher=0, wlan_wpa2_cipher=0;
	int config_method = 0;
	unsigned char tmpbuf[100], tmp1[100];
	unsigned int ptr_size = 0;
	int len;
	int ret_err_write = 0;
#if defined(FOR_DUAL_BAND)
	unsigned char wlanBand2G5GSelect;
	//int orig_wlan_idx;
#endif

#ifdef FOR_DUAL_BAND
//	int wlan_idx_orig = wlan_idx;
#endif //FOR_DUAL_BAND
	/*
	if ( !mib_init()) {
		printf("Initialize AP MIB failed!\n");
		return -1;
	}
	*/
	fprintf(stderr, "Writing file %s...\n", out ? out : "");
	mib_get_s(MIB_WSC_PIN, (void *)tmpbuf, sizeof(tmpbuf));
	if (genpin || !strcmp(tmpbuf, "\x0")) {
		#include <sys/time.h>
		struct timeval tod;
		unsigned long num;

		mib_get_s(MIB_ELAN_MAC_ADDR/*MIB_HW_NIC0_ADDR*/, (void *)&tmp1, sizeof(tmp1));
		gettimeofday(&tod , NULL);
		tod.tv_sec += tmp1[4]+tmp1[5];
		srand(tod.tv_sec);
		num = rand() % 10000000;
		num = num*10 + compute_pin_checksum(num);
		convert_hex_to_ascii((unsigned long)num, tmpbuf);

		mib_set(MIB_WSC_PIN, (void *)tmpbuf);
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);

		printf("Generated PIN = %s\n", tmpbuf);

		if (genpin)
			return 0;
	}
#ifdef FOR_DUAL_BAND
//	wlan_idx = 0;
#endif //FOR_DUAL_BAND
#if defined(FOR_DUAL_BAND)
	//orig_wlan_idx = wlan_idx;
	//wlan_idx = 0;
#endif
	if (stat(in, &status) < 0) {
		printf("stat() error [%s]!\n", in);
		goto ERROR;
	}

	buf = malloc(status.st_size+2048);
	if (buf == NULL) {
		printf("malloc() error [%d]!\n", (int)status.st_size+2048);
		goto ERROR;
	}

	ptr = buf;
	ptr_size = status.st_size + 2048;
	memset(ptr, '\0', ptr_size);

	//mib_get_s(gMIB_WLAN_MODE, (void *)&is_client, sizeof(is_client));
	is_client = Entry->wlanMode;
	is_config = Entry->wsc_configured;
	mib_local_mapping_get(MIB_WSC_REGISTRAR_ENABLED, wlanIdx, (void *)&is_registrar); //root
	if (is_client == WPS_CLIENT_MODE) {
		if (is_registrar)
			intVal = MODE_CLIENT_CONFIG;
		else {
			if (!is_config)
				intVal = MODE_CLIENT_UNCONFIG;
			else
				intVal = MODE_CLIENT_CONFIG;
		}
	} else {
		is_registrar = 1; // always true in AP
		if (!is_config)
			intVal = MODE_AP_UNCONFIG;
		else {
			if (is_registrar)
				intVal = MODE_AP_PROXY_REGISTRAR;
			else
				intVal = MODE_AP_PROXY;
		}
	}
	if (write_wsc_int_config(&ptr, &ptr_size, "mode", intVal) < 0) goto ERROR;

	if (is_client)
		intVal = 0;
	else
		intVal = Entry->wsc_upnp_enabled; //vap

#ifdef WLAN_WPS_MULTI_DAEMON
	if(!is_client){
		if(wlanIdx == 0) //check upnp enabled only for ssid-1
			intVal = Entry->wsc_upnp_enabled;
		else
			intVal = 0;
	}
#endif

	if (write_wsc_int_config(&ptr, &ptr_size, "upnp", intVal) < 0) goto ERROR;

#ifdef WPS20
#ifdef WPS_VERSION_CONFIGURABLE
	if (mib_get_s(MIB_WSC_VERSION, (void *)&current_wps_version, sizeof(current_wps_version)) == 0)
#endif
		current_wps_version = WPS_VERSION_V2;
#else
	current_wps_version = WPS_VERSION_V1;
#endif

	if (write_wsc_int_config(&ptr, &ptr_size, "current_wps_version", current_wps_version) < 0) goto ERROR;

	intVal = 0;
	mib_local_mapping_get(MIB_WSC_METHOD, wlanIdx, (void *)&intVal); //root
#ifdef WPS20
	if (current_wps_version == WPS_VERSION_V2) {
		if (intVal == 1) //Pin
			config_method = CONFIG_METHOD_VIRTUAL_PIN | CONFIG_METHOD_KEYPAD;
		else if (intVal == 2) //PBC
			config_method = (CONFIG_METHOD_PHYSICAL_PBC | CONFIG_METHOD_VIRTUAL_PBC);
		else if (intVal == 3) //Pin+PBC
			config_method = (CONFIG_METHOD_VIRTUAL_PIN | CONFIG_METHOD_KEYPAD |  CONFIG_METHOD_PHYSICAL_PBC | CONFIG_METHOD_VIRTUAL_PBC);
	} else
#endif
	{
		//Ethernet(0x2)+Label(0x4)+PushButton(0x80) Bitwise OR
		if (intVal == 1) //Pin+Ethernet
			config_method = (CONFIG_METHOD_ETH | CONFIG_METHOD_PIN);
		else if (intVal == 2) //PBC+Ethernet
			config_method = (CONFIG_METHOD_ETH | CONFIG_METHOD_PBC);
		else if (intVal == 3) //Pin+PBC+Ethernet
			config_method = (CONFIG_METHOD_ETH | CONFIG_METHOD_PIN | CONFIG_METHOD_PBC);
	}

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	config_method &= ~(CONFIG_METHOD_VIRTUAL_PIN | CONFIG_METHOD_PIN );
#endif

	if (write_wsc_int_config(&ptr, &ptr_size, "config_method", config_method) < 0) goto ERROR;

#if 0 //def FOR_DUAL_BAND
	mib_get_s(gMIB_WSC_DISABLE, (void *)&intVal2, sizeof(intVal2));
	if (write_wsc_int_config(&ptr, &ptr_size, "wlan0_wsc_disabled", intVal2) < 0) goto ERROR;
#endif

	intVal2 = Entry->wsc_auth;
	if (write_wsc_int_config(&ptr, &ptr_size, "auth_type", intVal2) < 0) goto ERROR;

	intVal = Entry->wsc_enc;
	if (write_wsc_int_config(&ptr, &ptr_size, "encrypt_type", intVal) < 0) goto ERROR;

	if (intVal == WSC_ENCRYPT_WEP)
		is_wep = 1;

	/*for detial mixed mode info*/
	wlan_encrypt = Entry->encrypt;
	//mib_get_s(gMIB_WLAN_ENCRYPT, (void *)&wlan_encrypt, sizeof(wlan_encrypt));
	wlan_wpa_cipher = Entry->unicastCipher;
	//mib_get_s(gMIB_WLAN_WPA_CIPHER_SUITE, (void *)&wlan_wpa_cipher, sizeof(wlan_wpa_cipher));
	wlan_wpa2_cipher = Entry->wpa2UnicastCipher;
	//mib_get_s(gMIB_WLAN_WPA2_CIPHER_SUITE, (void *)&wlan_wpa2_cipher, sizeof(wlan_wpa2_cipher));

	intVal=0;
	if(wlan_encrypt==6){	// mixed mode
		if(wlan_wpa_cipher==1){
			intVal |= WSC_WPA_TKIP;
		}else if(wlan_wpa_cipher==2){
			intVal |= WSC_WPA_AES;
		}else if(wlan_wpa_cipher==3){
			intVal |= (WSC_WPA_TKIP | WSC_WPA_AES);
		}
		if(wlan_wpa2_cipher==1){
			intVal |= WSC_WPA2_TKIP;
		}else if(wlan_wpa2_cipher==2){
			intVal |= WSC_WPA2_AES;
		}else if(wlan_wpa2_cipher==3){
			intVal |= (WSC_WPA2_TKIP | WSC_WPA2_AES);
		}
	}
	if (write_wsc_int_config(&ptr, &ptr_size, "mixedmode", intVal) < 0) goto ERROR;
	/*for detial mixed mode info*/

	if (is_client) {
		mib_local_mapping_get(MIB_WLAN_NETWORK_TYPE, wlanIdx, (void *)&intVal); //root
		if (intVal == 0)
			intVal = 1;
		else
			intVal = 2;
	} else
		intVal = 1;
	if (write_wsc_int_config(&ptr, &ptr_size, "connection_type", intVal) < 0) goto ERROR;

	mib_local_mapping_get(MIB_WSC_MANUAL_ENABLED, wlanIdx, (void *)&intVal); //root
	if (write_wsc_int_config(&ptr, &ptr_size, "manual_config", intVal) < 0) goto ERROR;

	if (is_wep) { // only allow WEP in none-MANUAL mode (configured by external registrar)
		intVal = Entry->encrypt;
		//mib_get_s(gMIB_WLAN_ENCRYPT, (void *)&intVal, sizeof(intVal));
		if (intVal != WIFI_SEC_WEP) {
			printf("WEP mismatched between WPS and host system\n");
			goto ERROR;
		}
		intVal = Entry->wep;
		//mib_get_s(gMIB_WLAN_WEP, (void *)&intVal, sizeof(intVal));
		if (intVal <= WEP_DISABLED || intVal > WEP128) {
			printf("WEP encrypt length error\n");
			goto ERROR;
		}
		wep_key_type = Entry->wepKeyType;
		//mib_get_s(gMIB_WLAN_WEP_KEY_TYPE, (void *)&wep_key_type, sizeof(wep_key_type));
		//mib_get_s(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&wep_transmit_key, sizeof(wep_transmit_key));
		wep_transmit_key = Entry->wepDefaultKey;
		wep_transmit_key++;
		if (write_wsc_int_config(&ptr, &ptr_size, "wep_transmit_key", wep_transmit_key) < 0) goto ERROR;

		if (intVal == WEP64) {
			strcpy(tmpbuf,Entry->wep64Key1);
			//mib_get_s(gMIB_WLAN_WEP64_KEY1, (void *)&tmpbuf, sizeof(tmpbuf));
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key", tmp1) < 0) goto ERROR;

			//mib_get_s(gMIB_WLAN_WEP64_KEY2, (void *)&tmpbuf, sizeof(tmpbuf)); //vap
			strcpy(tmpbuf,Entry->wep64Key2);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key2", tmp1) < 0) goto ERROR;

			//mib_get_s(gMIB_WLAN_WEP64_KEY3, (void *)&tmpbuf, sizeof(tmpbuf)); //vap
			strcpy(tmpbuf,Entry->wep64Key3);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key3", tmp1) < 0) goto ERROR;

			//mib_get_s(gMIB_WLAN_WEP64_KEY4, (void *)&tmpbuf, sizeof(tmpbuf)); //vap
			strcpy(tmpbuf,Entry->wep64Key4);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key4", tmp1) < 0) goto ERROR;
		} else {
			strcpy(tmpbuf, Entry->wep128Key1);
			//mib_get_s(gMIB_WLAN_WEP128_KEY1, (void *)&tmpbuf, sizeof(tmpbuf));
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key", tmp1) < 0) goto ERROR;

			//mib_get_s(gMIB_WLAN_WEP128_KEY2, (void *)&tmpbuf, sizeof(tmpbuf)); //vap
			strcpy(tmpbuf, Entry->wep128Key2);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}

			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key2", tmp1) < 0) goto ERROR;

			//mib_get_s(gMIB_WLAN_WEP128_KEY3, (void *)&tmpbuf, sizeof(tmpbuf)); //vap
			strcpy(tmpbuf, Entry->wep128Key3);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key3", tmp1) < 0) goto ERROR;

			//mib_get_s(gMIB_WLAN_WEP128_KEY4, (void *)&tmpbuf, sizeof(tmpbuf)); //vap
			strcpy(tmpbuf, Entry->wep128Key4);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key4", tmp1) < 0) goto ERROR;
		}
	} else {
		strcpy(tmp1, Entry->wpaPSK);
		//mib_get_s(gMIB_WLAN_WPA_PSK, (void *)&tmp1, sizeof(tmp1));
		if (write_wsc_str_config(&ptr, &ptr_size, "network_key", tmp1) < 0) goto ERROR;
	}

//	mib_get_s(gMIB_WLAN_SSID, (void *)&tmp1, sizeof(tmp1));
	if (write_wsc_str_config(&ptr, &ptr_size, "ssid", Entry->ssid) < 0) goto ERROR;

#if 0
//	}
//	else {
		mib_get_s(MIB_WSC_PSK, (void *)&tmp1, sizeof(tmp1));
		if (write_wsc_str_config(&ptr, &ptr_size, "network_key", tmp1) < 0) goto ERROR;

		mib_get_s(MIB_WSC_SSID, (void *)&tmp1, sizeof(tmp1));
		if (write_wsc_str_config(&ptr, &ptr_size, "ssid", tmp1) < 0) goto ERROR;
//	}
#endif

	mib_get_s(MIB_WSC_PIN, (void *)&tmp1, sizeof(tmp1));
	if (write_wsc_str_config(&ptr, &ptr_size, "pin_code", tmp1) < 0) goto ERROR;

	mib_local_mapping_get(MIB_WLAN_CHAN_NUM, wlanIdx, (void *)&intVal);
	if (intVal > 14)
		intVal = 2;
	else
		intVal = 1;
	if (write_wsc_int_config(&ptr, &ptr_size, "rf_band", intVal) < 0) goto ERROR;

#ifdef FOR_DUAL_BAND
	//wlan_idx = 1;

	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "#=====wlan1 start==========%d\n", intVal, ret_err_write);
	if (ret_err_write) {
		printf("WRITE_WSC_PARAM memcpy error when writing comment:[#=====wlan1 start==========%d]\n", intVal);
		goto ERROR;
	}

	mib_local_mapping_get(MIB_WSC_DISABLE, wlanIdx, (void *)&intVal2);
	if (write_wsc_int_config(&ptr, &ptr_size, "wlan1_wsc_disabled", intVal2) < 0) goto ERROR;

	mib_local_mapping_get(MIB_WLAN_SSID, wlanIdx, (void *)&tmp1);
	if (write_wsc_str_config(&ptr, &ptr_size, "ssid2", tmp1) < 0) goto ERROR;

	mib_local_mapping_get(MIB_WSC_AUTH, wlanIdx, (void *)&intVal2);
	if (write_wsc_int_config(&ptr, &ptr_size, "auth_type2", intVal2) < 0) goto ERROR;

	mib_local_mapping_get(MIB_WSC_ENC, wlanIdx, (void *)&intVal);
	if (write_wsc_int_config(&ptr, &ptr_size, "encrypt_type2", intVal) < 0) goto ERROR;

	if (intVal == ENCRYPT_WEP)
		is_wep = 1;

	/*for detial mixed mode info*/
	mib_local_mapping_get(MIB_WLAN_ENCRYPT, wlanIdx, (void *)&wlan_encrypt);
	mib_local_mapping_get(MIB_WLAN_WPA_CIPHER_SUITE, wlanIdx, (void *)&wlan_wpa_cipher);
	mib_local_mapping_get(MIB_WLAN_WPA2_CIPHER_SUITE, wlanIdx, (void *)&wlan_wpa2_cipher);

	intVal=0;
	if(wlan_encrypt==6){	// mixed mode
		if(wlan_wpa_cipher==1){
			intVal |= WSC_WPA_TKIP;
		}else if(wlan_wpa_cipher==2){
			intVal |= WSC_WPA_AES;
		}else if(wlan_wpa_cipher==3){
			intVal |= (WSC_WPA_TKIP | WSC_WPA_AES);
		}
		if(wlan_wpa2_cipher==1){
			intVal |= WSC_WPA2_TKIP;
		}else if(wlan_wpa2_cipher==2){
			intVal |= WSC_WPA2_AES;
		}else if(wlan_wpa2_cipher==3){
			intVal |= (WSC_WPA2_TKIP | WSC_WPA2_AES);
		}
	}
	if (write_wsc_int_config(&ptr, &ptr_size, "mixedmode2", intVal) < 0) goto ERROR;
	/*for detial mixed mode info*/

/*
	mib_get_s(gMIB_WLAN_BAND2G5G_SELECT, (void *)&intVal, sizeof(intVal));	// 0:2.4g  ; 1:5G   ; 2:both(dual band)
	if(intVal != 2) {
		intVal=1;
		WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "wlan1_wsc_disabled = %d\n",intVal, ret_err_write);
		if (ret_err_write) {
			printf("WRITE_WSC_PARAM memcpy error when writing wlan1_wsc_disabled\n");
			goto ERROR;
		}
	}
	else {
		mib_get_s(gMIB_WSC_DISABLE, (void *)&intVal, sizeof(intVal));
		WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "wlan1_wsc_disabled = %d\n", intVal, ret_err_write);
		if (ret_err_write) {
			printf("WRITE_WSC_PARAM memcpy error when writing wlan1_wsc_disabled\n");
			goto ERROR;
		}
	}
*/
	if (is_wep) { // only allow WEP in none-MANUAL mode (configured by external registrar)
		mib_local_mapping_get(MIB_WLAN_ENCRYPT, wlanIdx, (void *)&intVal);
		if (intVal != _ENCRYPT_WEP_) {
			printf("WEP mismatched between WPS and host system\n");
			goto ERROR;
		}
		mib_local_mapping_get(MIB_WLAN_WEP, wlanIdx, (void *)&intVal);
		if (intVal <= WEP_DISABLED || intVal > WEP128) {
			printf("WEP encrypt length error\n");
			goto ERROR;
		}
		mib_local_mapping_get(MIB_WLAN_WEP_KEY_TYPE, wlanIdx, (void *)&wep_key_type);
		mib_local_mapping_get(MIB_WLAN_WEP_DEFAULT_KEY, wlanIdx, (void *)&wep_transmit_key);
		wep_transmit_key++;
		if (write_wsc_int_config(&ptr, &ptr_size, "wep_transmit_key2", wep_transmit_key) < 0) goto ERROR;

		if (intVal == WEP64) {
			mib_local_mapping_get(MIB_WLAN_WEP64_KEY1, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}

			if (write_wsc_str_config(&ptr, &ptr_size, "network_key2", tmp1) < 0) goto ERROR;

			mib_local_mapping_get(MIB_WLAN_WEP64_KEY2, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key22", tmp1) < 0) goto ERROR;

			mib_local_mapping_get(MIB_WLAN_WEP64_KEY3, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key32", tmp1) < 0) goto ERROR;

			mib_local_mapping_get(MIB_WLAN_WEP64_KEY4, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 5);
				tmp1[5] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 5, tmp1, sizeof(tmp1));
				tmp1[10] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key42", tmp1) < 0) goto ERROR;
		}
		else {
			mib_local_mapping_get(MIB_WLAN_WEP128_KEY1, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "network_key2", tmp1) < 0) goto ERROR;

			mib_local_mapping_get(MIB_WLAN_WEP128_KEY2, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key22", tmp1) < 0) goto ERROR;

			mib_local_mapping_get(MIB_WLAN_WEP128_KEY3, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			}
			else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key32", tmp1) < 0) goto ERROR;

			mib_local_mapping_get(MIB_WLAN_WEP128_KEY4, wlanIdx, (void *)&tmpbuf);
			if (wep_key_type == KEY_ASCII) {
				memcpy(tmp1, tmpbuf, 13);
				tmp1[13] = '\0';
			} else {
				convert_bin_to_str(tmpbuf, 13, tmp1, sizeof(tmp1));
				tmp1[26] = '\0';
			}
			if (write_wsc_str_config(&ptr, &ptr_size, "wep_key42", tmp1) < 0) goto ERROR;
		}
	} else {
		mib_local_mapping_get(MIB_WLAN_WPA_PSK, wlanIdx, (void *)&tmp1);
		if (write_wsc_str_config(&ptr, &ptr_size, "network_key2", tmp1) < 0) goto ERROR;
	}
	intVal =2 ;
	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "#=====wlan1 end==========%d\n",intVal, ret_err_write);
	if (ret_err_write) {
		printf("WRITE_WSC_PARAM memcpy error when writing comment:[#=====wlan1 end==========%d]\n", intVal);
		goto ERROR;
	}
//	wlan_idx = 0;
#endif //FOR_DUAL_BAND

/*
	mib_get_s(MIB_HW_MODEL_NUM, (void *)&tmp1, sizeof(tmp1));
	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "model_num = \"%s\"\n", tmp1, ret_err_write);
	if (ret_err_write) {
		printf("WRITE_WSC_PARAM memcpy error when writing model_num\n");
		goto ERROR;
	}

	mib_get_s(MIB_HW_SERIAL_NUM, (void *)&tmp1, sizeof(tmp1));
	WRITE_WSC_PARAM(ptr, ptr_size, tmpbuf, sizeof(tmpbuf), "serial_num = \"%s\"\n", tmp1, ret_err_write);
	if (ret_err_write) {
		printf("WRITE_WSC_PARAM memcpy error when writing serial_num\n");
		goto ERROR;
	}
*/
	mib_get_s(MIB_SNMP_SYS_NAME, (void *)&tmp1, sizeof(tmp1));
	if (write_wsc_str_config(&ptr, &ptr_size, "device_name", tmp1) < 0) goto ERROR;

	len = (int)(((long)ptr)-((long)buf));

	if ((fh = open(in, O_RDONLY)) < 0){
		printf("open() error [%s]!\n", in);
		goto ERROR;
	}

	lseek(fh, 0L, SEEK_SET);
	if (read(fh, ptr, status.st_size) != status.st_size) {
		printf("read() error [%s]!\n", in);
		//return -1;
		goto ERROR;
	}
	close(fh);

	// search UUID field, replace last 12 char with hw mac address
	ptr = strstr(ptr, "uuid =");
	if (ptr) {
		char tmp2[100];
		mib_get_s(MIB_ELAN_MAC_ADDR/*MIB_HW_NIC0_ADDR*/, (void *)&tmp1, sizeof(tmp1));
		convert_bin_to_str(tmp1, 6, tmp2, sizeof(tmp2));
		if (strlen(ptr) < (27+12)) {
			printf("memcpy error when setting uuid: no enough space\n");
			goto ERROR;
		}
		memcpy(ptr+27, tmp2, 12);
	}
#ifdef RESTRICT_PROCESS_PERMISSION
	if ((fh = open(out, O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IWGRP|S_IWOTH)) < 0)
#else
	if ((fh = open(out, O_RDWR|O_CREAT|O_TRUNC)) < 0)
#endif
	{
		printf("open() error [%s]!\n", out);
		goto ERROR;
	}

	if (write(fh, buf, len+status.st_size) != len+status.st_size ) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}

	unsigned int wps_timeout;
	mib_get_s(MIB_WPS_TIMEOUT, &wps_timeout, sizeof(wps_timeout));
	sprintf(tmpbuf, "pbc_walk_time = %u\n", wps_timeout);
	write(fh, tmpbuf, strlen(tmpbuf));

	close(fh);
	free(buf);
#if defined(FOR_DUAL_BAND)
	//wlan_idx = orig_wlan_idx;
#endif
#ifdef FOR_DUAL_BAND
//	wlan_idx = wlan_idx_orig;
#endif //FOR_DUAL_BAND
	return 0;

ERROR:
	if (buf) free(buf);
	if (-1 != fh) close(fh);
#if defined(FOR_DUAL_BAND)
	//wlan_idx = orig_wlan_idx;
#endif
#ifdef FOR_DUAL_BAND
//	wlan_idx = wlan_idx_orig;
#endif //FOR_DUAL_BAND
	return -1;
}

int start_WPS(int run_daemon)
{

	char wscd_pid_name[32];
	char wscd_fifo_name[32];
	char wscd_conf_name[32];
	int wsc_pid_fd=-1;

	MIB_CE_MBSSIB_T Entry;
	int orig_wlan_idx, i, j;
	char ifname[16];
	int status = 0;
	unsigned char wps_ssid;
	unsigned char vChar;
	unsigned char wsc_disable;
	unsigned char wlan_nettype;
	unsigned char wlan_mode;
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
	unsigned char no_wlan;
#endif
	int retry=0;
#ifdef WLAN_WPS_MULTI_DAEMON
	char wscd_button_procname[32];
#endif

	//wlan_num = 0; /*reset to 0,jiunming*/
	//useAuth_RootIf = 0;  /*reset to 0 */

#ifndef WLAN_WPS_MULTI_DAEMON
	mib_get_s(MIB_WPS_ENABLE, &vChar, sizeof(vChar));
	if(vChar == 0)
		goto no_wsc;
#endif

	orig_wlan_idx = wlan_idx;

	for(j=0;j<NUM_WLAN_INTERFACE;j++){
		wlan_idx = j;
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
		mib_get_s(MIB_WLAN_MODULE_DISABLED, (void *)&no_wlan, sizeof(no_wlan));
		if(no_wlan)
			continue;
#endif
		for ( i=0; i<=WLAN_SSID_NUM; i++) {

			mib_chain_get(MIB_MBSSIB_TBL, i, (void *)&Entry);

			rtk_wlan_get_ifname(j, i, ifname);

			wsc_disable = check_wps_enc(&Entry)? 0:1;

			mib_get_s(MIB_WLAN_MODE, (void *)&wlan_mode, sizeof(wlan_mode));
			mib_get_s(MIB_WLAN_NETWORK_TYPE, (void *)&wlan_nettype, sizeof(wlan_nettype));
			//mib_get_s(MIB_WLAN_WPA_AUTH, (void *)&wpa_auth, sizeof(wpa_auth));
			//mib_get_s(MIB_WLAN_ENCRYPT, (void *)&encrypt, sizeof(encrypt));
#ifndef WLAN_WPS_MULTI_DAEMON
			mib_get_s(MIB_WPS_SSID, &wps_ssid, sizeof(wps_ssid));
#endif

			if(Entry.wlanDisabled || wsc_disable)
				continue;
#ifdef WLAN_WPS_MULTI_DAEMON
			else if(Entry.wsc_disabled)
				continue;
#else
			else if(!check_is_wps_ssid(i, wps_ssid))
				continue;
#endif
			else if(wlan_mode == CLIENT_MODE) {
				if(wlan_nettype != INFRASTRUCTURE)
					continue;
			}
			else if(wlan_mode == AP_MODE) {
				if((Entry.encrypt >= WIFI_SEC_WPA) && (Entry.wpaAuth == WPA_AUTH_AUTO))
					continue;
			}

			if(run_daemon == 0)
				goto skip_running_wscd;

			snprintf(wscd_pid_name, 32, "/var/run/wscd-%s.pid", ifname);

			int wscd_pid = read_pid(wscd_pid_name);
			if(wscd_pid > 0 && kill(wscd_pid, 0)==0)
				goto skip_running_wscd;


		#define TARGDIR "/var/wps/"
		#define SIMPLECFG "simplecfgservice.xml"
			//status |= va_cmd("/bin/flash", 3, 1, "upd-wsc-conf", "/etc/wscd.conf", "/var/wscd.conf");
			status |= va_cmd("/bin/mkdir", 2, 1, "-p", TARGDIR);
			status |= va_cmd("/bin/cp", 2, 1, "/etc/" SIMPLECFG, TARGDIR);

			sprintf(wscd_conf_name, "/var/wscd-%s.conf", ifname);
			sprintf(wscd_fifo_name, "/var/wscd-%s.fifo", ifname);
			//status|=generateWpaConf(para_auth_conf, 0, &Entry);
			status|=WPS_updateWscConf("/etc/wscd.conf", wscd_conf_name, 0, &Entry, i, j);

#ifdef WLAN_WPS_MULTI_DAEMON
			memset(wscd_button_procname, '\0', sizeof(wscd_button_procname));

			if(j==0 && i==0) //only trigger push button with wlan0
				snprintf(wscd_button_procname, sizeof(wscd_button_procname), "/proc/gpio");

			if(wscd_button_procname[0] == '\0')
				status|=va_niced_cmd(WSC_DAEMON_PROG, 10, 0, "-start", "-c", wscd_conf_name, "-w", ifname, "-fi", wscd_fifo_name, "-proc", "-", "-daemon");
			else
				status|=va_niced_cmd(WSC_DAEMON_PROG, 10, 0, "-start", "-c", wscd_conf_name, "-w", ifname, "-fi", wscd_fifo_name, "-proc", wscd_button_procname, "-daemon");
#else
			status|=va_niced_cmd(WSC_DAEMON_PROG, 8, 0, "-start", "-c", wscd_conf_name, "-w", ifname, "-fi", wscd_fifo_name, "-daemon");
#endif

			// fix the depency problem
			// check fifo
			retry = 10;
			while (--retry && ((wsc_pid_fd = open(wscd_fifo_name, O_WRONLY)) == -1))
			{
				usleep(30000);
			}
			retry = 10;

			while(--retry && (read_pid(wscd_pid_name) < 0))
			{
				//printf("WSCD is not running. Please wait!\n");
				usleep(300000);
			}

			if(wsc_pid_fd!=-1) close(wsc_pid_fd);/*jiunming, close the opened fd*/

skip_running_wscd:

			if (i == WLAN_ROOT_ITF_INDEX){ // Root
					/* 2010-10-27 krammer :  use bit map to record what wlan root interface is use for auth*/

				useAuth_RootIf |= (1<<wlan_idx);//bit 0 ==> wlan0, bit 1 ==>wlan1
			}
			else {
				strcpy(para_iwctrl[wlan_num], ifname);
				wlan_num++;
			}

		}
	}
	wlan_idx = orig_wlan_idx;
no_wsc:
	if(run_daemon == 1)
		startSSDP();
	return status;

}
#else
int start_WPS()
{
	int status=0;
	char iface_name[16];
	char tmpBuff[100], tmpBuff1[100], arg_buff[1000], wlan_wapi_asipaddr[100], wlan_vxd[30]={0};
	char wlan_wapi_cert_sel;
	char _enable_1x=0, _use_rs=0;
	char wlan_mode_root=0,wlan_disabled_root=1, wlan_wpa_auth_root=0, wlan1_wpa_auth_root=0;
	char wlan0_mode=1, wlan1_mode=1, both_band_ap=0;
	char wlan_wsc_disabled_root=0, wlan_network_type_root=0, wlan0_wsc_disabled_vxd=1, wlan1_wsc_disabled_vxd=1, wlan2_wsc_disabled_vxd=1;
	char wlan_1x_enabled_root=0, wlan_encrypt_root=0, wlan_mac_auth_enabled_root=0,wlan_wapi_auth=0;
	char wlan_disabled=0, wlan_mode=0, wlan_wds_enabled=0, wlan_wds_num=0;
	char wlan_encrypt=0, wlan_wds_encrypt=0;
	char wlan_wpa_auth=0;
	char wlan_1x_enabled=0,wlan_mac_auth_enabled=0;
	char wlan_root_auth_enable=0, wlan_vap_auth_enable=0;
	char wlan_network_type=0, wlan_wsc_disabled=0, wlan_hidden_ssid_enabled=0,wlan0_hidden_ssid_enabled=0,wlan1_hidden_ssid_enabled=0;
	char vap_not_in_pure_ap_mode=0, deamon_created=0;
	char isWLANEnabled=0, isAP=0, intValue=0;
	char bridge_iface[30]={0};
	char *token=NULL, *savestr1=NULL;
	char WSC=1, WSC_UPNP_Enabled=0;
	char *cmd_opt[16]={0};
	int cmd_cnt = 0;
	int check_cnt = 0;
	//Added for virtual wlan interface
	int i=0, wlan_encrypt_virtual=0;
	char wlan_vname[16];
	unsigned int enableWscIf = 0;
	MIB_CE_MBSSIB_T Entry;
#if defined(WLAN_DUALBAND_CONCURRENT)
	int original_wlan_idx = wlan_idx;
#endif

#if 1 //defined(WLAN_DUALBAND_CONCURRENT)
	char wlan_wsc1_disabled = 1 ;
	char wlan0_disabled_root=1, wlan1_disabled_root = 1, wlan2_disabled_root=1, wlan_disabled_root_tmp=1;
	char wlan0_disabled_vxd=1, wlan1_disabled_vxd=1, wlan2_disabled_vxd=1;
#endif
#if defined(CONFIG_REPEATER_WPS_SUPPORT)
	char isRptEnabled1=0, isRptEnabled2=0, isRptEnabled3=0, RptEnabled=0;
	for(i=0; i<NUM_WLAN_INTERFACE; ++i){
		snprintf(tmpBuff, sizeof(tmpBuff), "%s", WLANIF[i]);
		SetWlan_idx(tmpBuff);
		//printf("===[%s %d]===tmpBuff:%s\n", __FUNCTION__,__LINE__,tmpBuff);
		mib_get(MIB_REPEATER_ENABLED1, (void *)&RptEnabled);
		mib_get(MIB_WLAN_DISABLED, (void *)&wlan_disabled_root_tmp);
		//printf("===[%s %d]===wlan_disabled_root_tmp:%d\n", __FUNCTION__,__LINE__,wlan_disabled_root_tmp);
		memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));
		mib_chain_get(MIB_MBSSIB_TBL, WLAN_REPEATER_ITF_INDEX, (void *)&Entry);
		if(i==0){
			isRptEnabled1 = RptEnabled;
			wlan0_wsc_disabled_vxd = Entry.wsc_disabled;
			wlan0_disabled_root = wlan_disabled_root_tmp;
			wlan0_disabled_vxd = Entry.wlanDisabled;
		}else if(i==1){
			isRptEnabled2 = RptEnabled;
			wlan1_wsc_disabled_vxd = Entry.wsc_disabled;
			wlan1_disabled_root = wlan_disabled_root_tmp;
			wlan1_disabled_vxd = Entry.wlanDisabled;
		}else if(i==2){
			isRptEnabled3 = RptEnabled;
			wlan2_wsc_disabled_vxd = Entry.wsc_disabled;
			wlan2_disabled_root = wlan_disabled_root_tmp;
			wlan2_disabled_vxd = Entry.wlanDisabled;
		}
	}
#endif

    if (isFileExist((char *)WSC_DAEMON_PROG)) {
        char ap_interface[50];
        char client_interface[50];

#ifdef CONFIG_RTL_P2P_SUPPORT
        char p2p_interface[20];
        memset(p2p_interface, 0x00, sizeof(p2p_interface));
#endif
        memset(ap_interface, 0x00, sizeof(ap_interface));
        memset(client_interface, 0x00, sizeof(client_interface));
        memset(tmpBuff, 0x00, sizeof(tmpBuff));
        token=NULL;
        savestr1=NULL;

	    snprintf(arg_buff, sizeof(arg_buff), "%s", wlan_valid_interface);
		printf("====> wlan_valid_interface:%s\n", wlan_valid_interface);

        snprintf(bridge_iface, sizeof(bridge_iface), "%s", "br0"); // just for test tesia

        token = strtok_r(arg_buff," ", &savestr1);
        do{
            if (token == NULL){
                break;
            }else{
                _enable_1x=0;
                WSC=1;

                if(!strcmp(token, "wlan0") //root if
                    || !strcmp(token, "wlan1")
#if defined(CONFIG_REPEATER_WPS_SUPPORT)
                    || strstr(token, "-vxd")
#endif
#if defined(TRIBAND_SUPPORT)
                    || !strcmp(token, "wlan2")
#endif
                )
                {
                    if(is_root_iface(token)){
                    SetWlan_idx(token);
                    mib_get_s( MIB_WLAN_MODE, (void *)&wlan_mode_root, sizeof(wlan_mode_root));
                    mib_get_s( MIB_WLAN_ENABLE_1X, (void *)&wlan_1x_enabled_root, sizeof(wlan_1x_enabled_root));
                    mib_get_s( MIB_WLAN_ENCRYPT, (void *)&wlan_encrypt_root, sizeof(wlan_encrypt_root));
                    mib_get_s(MIB_WLAN_HIDDEN_SSID, (void *)&wlan_hidden_ssid_enabled, sizeof(wlan_hidden_ssid_enabled));
					
                   // mib_get_s( MIB_WLAN_MAC_AUTH_ENABLED, (void *)&wlan_mac_auth_enabled_root, sizeof(wlan_mac_auth_enabled_root));
                    mib_get_s( MIB_WLAN_NETWORK_TYPE, (void *)&wlan_network_type_root, sizeof(wlan_network_type_root));
                    mib_get_s( MIB_WLAN_WPA_AUTH, (void *)&wlan_wpa_auth_root, sizeof(wlan_wpa_auth_root));
					mib_get_s( MIB_WLAN_DISABLED, (void *)&wlan_disabled_root, sizeof(wlan_disabled_root));
					mib_get_s( MIB_WSC_DISABLE, (void *)&wlan_wsc_disabled_root, sizeof(wlan_wsc_disabled_root));

                    if(wlan_encrypt_root < 2){ //ENCRYPT_DISABLED=0, ENCRYPT_WEP=1, ENCRYPT_WPA=2, ENCRYPT_WPA2=4, ENCRYPT_WPA2_MIXED=6 ,ENCRYPT_WAPI=7
	                    if(wlan_1x_enabled_root != 0 || wlan_mac_auth_enabled_root !=0)
	                        _enable_1x=1;
                    }else{
                        if(wlan_encrypt_root != 7)	//not wapi
                            _enable_1x=1;
                    }
                    if(!strcmp(token, "wlan0") && ((wlan_wsc_disabled_root != 0) || (wlan_disabled_root != 0) || (wlan_mode_root == WDS_MODE) || (wlan_mode_root == MESH_MODE))){
                        WSC=0;
                    }
#if defined(WLAN_DUALBAND_CONCURRENT)
                    else if(!strcmp(token, "wlan1") && ((wlan_wsc_disabled_root != 0) || (wlan_disabled_root != 0) || (wlan_mode_root == WDS_MODE) || (wlan_mode_root == MESH_MODE))){
                        WSC=0;
                    }
#endif
#if defined(TRIBAND_SUPPORT)
                    else if(!strcmp(token, "wlan2") && ((wlan_wsc_disabled_root != 0) || (wlan_disabled_root != 0) || (wlan_mode_root == WDS_MODE) || (wlan_mode_root == MESH_MODE))){
                        WSC=0;
                    }
#endif
                    else{
                        if(wlan_mode_root == CLIENT_MODE){
                            if(wlan_network_type_root != 0)
                                WSC=0;
                        }
                        else if(wlan_mode_root == AP_MODE || wlan_mode_root == AP_WDS_MODE || wlan_mode_root == AP_MESH_MODE){
                            if(wlan_hidden_ssid_enabled) {
                                WSC = 0;
                            }
                            else if(wlan_encrypt_root  == 0 && _enable_1x !=0 )
                                WSC=0;
                            else if(wlan_encrypt_root == 1) /* wps do not support wep*/
                                WSC=0;
                            else if(wlan_encrypt_root >= 2 && wlan_encrypt_root != 7 && wlan_wpa_auth_root ==1 )
                                WSC=0;
#ifdef CONFIG_RTL_WAPI_SUPPORT
                            else if(wlan_encrypt_root == 7 && wlan_wapi_auth_root == 1)
                                WSC=0;
#endif
#ifdef WLAN_WPA3_NOT_SUPPORT_WPS
                            else if(wlan_encrypt_root == 16)
                                WSC=0;
#endif
                        }
                    }						
                    }
#if defined(CONFIG_REPEATER_WPS_SUPPORT)
                    else if(is_vxd_iface(token)){					
                            if(!strcmp(token, "wlan0-vxd") && (wlan0_wsc_disabled_vxd != 0 || wlan0_disabled_root != 0 || isRptEnabled1!=1 || wlan0_disabled_vxd !=0 ))
                            {
                                WSC=0;
                            }
#if defined(WLAN_DUALBAND_CONCURRENT)					
                            else if(!strcmp(token, "wlan1-vxd") && (wlan1_wsc_disabled_vxd != 0 || wlan1_disabled_root != 0 || isRptEnabled2!=1 || wlan1_disabled_vxd !=0))
                            {
                                WSC=0;	
                            }
#endif	
#if defined(TRIBAND_SUPPORT)
                            else if(!strcmp(token, "wlan2-vxd") && (wlan2_wsc_disabled_vxd != 0 || wlan2_disabled_root != 0 || isRptEnabled3!=1 || wlan2_disabled_vxd !=0))
                            {
                                WSC=0;
                            }
#endif	
                    }
#endif  

                    if(WSC==1){ //start wscd
                        if(wlan_mode_root == CLIENT_MODE
#if defined(CONFIG_REPEATER_WPS_SUPPORT)
                            || is_vxd_iface(token)
#endif
                            ){
                            if(client_interface[0] == 0) {
                                sprintf(client_interface, "%s", token);
								if(!strcmp(token, "wlan0"))
									enableWscIf |= 1;	
								else if(!strcmp(token, "wlan1"))
									enableWscIf |= 2;
                            #if defined(TRIBAND_WPS)
                                else if(!strcmp(token, "wlan2"))
                                    enableWscIf |= 4;
                            #endif /* defined(TRIBAND_WPS) */
                            #if defined(CONFIG_REPEATER_WPS_SUPPORT)
								else if(strstr(token, "-vxd")){
									strcpy(para_iwctrl[wlan_num], token);
									wlan_num++;
								}
                            #endif /* defined(TRIBAND_WPS) */
                            }
                            else {
                                strcat(client_interface, " ");                                
                                strcat(client_interface, token);
								if(!strcmp(token, "wlan0"))
									enableWscIf |= 1;	
								else if(!strcmp(token, "wlan1"))
									enableWscIf |= 2;	
                            #if defined(TRIBAND_WPS)
                                else if(!strcmp(token, "wlan2"))
                                    enableWscIf |= 4;
                            #endif /* defined(TRIBAND_WPS) */
							#if defined(CONFIG_REPEATER_WPS_SUPPORT)
								else if(strstr(token, "-vxd")){
									strcpy(para_iwctrl[wlan_num], token);
									wlan_num++;
								}
                            #endif /* defined(TRIBAND_WPS) */
                            }
                        }
#ifdef CONFIG_RTL_P2P_SUPPORT                        
                        else if(wlan_mode_root == P2P_SUPPORT_MODE) {  
                            mib_get_s( MIB_WLAN_P2P_TYPE, (void *)&p2p_mode, sizeof(p2p_mode)); 
                            sprintf(p2p_interface, "%s", token);
                        }
#endif                        
                        else { /* AP mode*/                           
                            mib_get_s( MIB_WSC_UPNP_ENABLED, (void *)&WSC_UPNP_Enabled, sizeof(WSC_UPNP_Enabled));
                            if(ap_interface[0] == 0) {								
                                sprintf(ap_interface, "%s", token);
								if(!strcmp(token, "wlan0"))
									enableWscIf |= 1;	
								else if(!strcmp(token, "wlan1"))
									enableWscIf |= 2;
                            #if defined(TRIBAND_WPS)
                                else if(!strcmp(token, "wlan2"))
                                    enableWscIf |= 4;
                            #endif /* defined(TRIBAND_WPS) */
                            }
                            else {
                                strcat(ap_interface, " ");                                
                                strcat(ap_interface, token);
								if(!strcmp(token, "wlan0"))
									enableWscIf |= 1;	
								else if(!strcmp(token, "wlan1"))
									enableWscIf |= 2;
                            #if defined(TRIBAND_WPS)
                                else if(!strcmp(token, "wlan2"))
                                    enableWscIf |= 4;
                            #endif /* defined(TRIBAND_WPS) */
                            }
                        }
                    }						
                }		
            }   
            token = strtok_r(NULL, " ", &savestr1);

        }while(token !=NULL);
	   
        /* start wsc deamon in ap mode*/
        if(ap_interface[0]) {
			useAuth_RootIf |= enableWscIf;
#if 0
			if(useAuth_RootIf==1 || useAuth_RootIf==2
            #if defined(TRIBAND_WPS)
                || useAuth_RootIf==4
            #endif
                ) {
				snprintf(useAuth_RootIfname, sizeof(useAuth_RootIfname), "%s", ap_interface);				
            }
#endif
			
            deamon_created= start_wsc_deamon(ap_interface, 0, WSC_UPNP_Enabled, bridge_iface);
        }

        /* start wsc deamon in client mode*/
        if(client_interface[0]) {
			useAuth_RootIf |= enableWscIf;
#if 0		
			if(useAuth_RootIf==1 || useAuth_RootIf==2
            #if defined(TRIBAND_WPS)
                || useAuth_RootIf==4
            #endif
			)
				snprintf(useAuth_RootIfname, sizeof(useAuth_RootIfname), "%s", client_interface);	
#endif
#ifdef CONFIG_COMBINE_TWO_WPS_DEAMON_FOR_CLIENT            
            /*start only one deamon for both clients*/
            deamon_created = start_wsc_deamon(client_interface, 1, 0, NULL);
#else
            /*start wsc deamon for every client interface*/
            token = strtok_r(client_interface," ", &savestr1);
            do {
                deamon_created = start_wsc_deamon(token, 1, 0, NULL);                
                token = strtok_r(NULL," ", &savestr1);
            }while(token != NULL);
#endif
        }
        
#ifdef CONFIG_RTL_P2P_SUPPORT                        
        /* start wsc deamon for p2p*/
        if(p2p_interface[0]) {
            if(p2p_mode == P2P_DEVICE || p2p_mode == P2P_CLIENT)
                deamon_created= start_wsc_deamon(p2p_interface, 1, 0, NULL);            
            else
                deamon_created= start_wsc_deamon(p2p_interface, 0, 0, NULL);
        }
#endif        
    }

	/* iwcontrol is started in the following start_iwcontrol */
#if 0
	if(deamon_created==1){
		if(wlan_vxd[0]){
			sprintf(tmpBuff, "iwcontrol %s %s",valid_wlan_interface, wlan_vxd);
	}else{
			sprintf(tmpBuff, "%s %s", IWCONTROL, valid_wlan_interface);
		}
		system(tmpBuff);	
	}
#endif
#if defined(WLAN_DUALBAND_CONCURRENT)
	wlan_idx = original_wlan_idx;
#endif	

	return status;
}
#endif
#endif   /*CONFIG_RTK_DEV_AP*/
#endif

#ifdef CONFIG_IAPP_SUPPORT
int start_IAPP()
{
	int status=0, i;
	int wlan_iapp_enabled=0;
	//int isWLANEnabled=0, isAP=0, isIAPPEnabled=0;
	int wlan_disabled=0, wlan_mode=0;
	char iface_name[16]; 
	char tmp_iface[30]={0};
	char iapp_interface[200]= {0};
	char bridge_iface[30]={0};
	char tmpBuff[100];

#if defined(WLAN_DUALBAND_CONCURRENT)
		int original_wlan_idx = wlan_idx;	
#endif	

	snprintf(bridge_iface, sizeof(bridge_iface), "%s", "br0");

	for(i=0;i<NUM_WLAN_INTERFACE;i++){
		int isWLANEnabled=0, isAP=0, isIAPPEnabled=0;
		sprintf(iface_name, "%s", WLANIF[i]);
		if(SetWlan_idx(iface_name)){
			mib_get_s( MIB_WLAN_DISABLED, (void *)&wlan_disabled, sizeof(wlan_disabled));
			mib_get_s( MIB_WLAN_MODE, (void *)&wlan_mode, sizeof(wlan_mode)); 
			mib_get_s( MIB_WLAN_IAPP_ENABLED, (void *)&wlan_iapp_enabled, sizeof(wlan_iapp_enabled));
			
			 if(wlan_disabled==0)
				isWLANEnabled=1;
			 if(wlan_mode ==0 || wlan_mode ==3 || wlan_mode ==4 || wlan_mode ==6)
				isAP=1; 
			 if(wlan_iapp_enabled==1)
				isIAPPEnabled=1; 

			 if(isWLANEnabled==1 && isAP==1&& isIAPPEnabled==1){
				if(iapp_interface[0]==0){
					sprintf(iapp_interface, "%s", iface_name); 
				}else{
					sprintf(tmp_iface, " %s", iface_name);
					strcat(iapp_interface, tmp_iface);
				}
			 }
		}
	}

	if (iapp_interface[0] != 0){
		//status |= va_cmd("/bin/iapp", 2, 1, bridge_iface, iapp_interface);
		//printf("\n [====%s %d====] \n status = %d", __FUNCTION__, __LINE__, status);
		sprintf(tmpBuff, "iapp %s %s",bridge_iface, iapp_interface);
		system(tmpBuff);
	}
	

#if 0	
		if(isFileExist(RESTART_IAPP))
			unlink(RESTART_IAPP);
		va_cmd_fout("echo", 1, 1, RESTART_IAPP, tmpBuff);
#endif

#if defined(WLAN_DUALBAND_CONCURRENT)
		wlan_idx = original_wlan_idx;
#endif	

	return status;
}
#endif

#ifdef _PRMT_X_WLANFORISP_
static int getWlanForISPEntry(int vwlan_idx, MIB_WLANFORISP_Tp pEntry)
{
	int i, total = mib_chain_total(MIB_WLANFORISP_TBL);
	int ssid_idx = wlan_idx*(1+WLAN_MBSSID_NUM)+(1+vwlan_idx);

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

#if defined(RTK_CROSSBAND_REPEATER)
int start_crossband(void)
{
	char repeater_enabled1 = 0, repeater_enabled2 = 0;
	char wlan_disabled1 = 1, wlan_disabled2 = 1;
	MIB_CE_MBSSIB_T Entry1, Entry2;
#if defined(WLAN_DUALBAND_CONCURRENT)
	int original_wlan_idx = wlan_idx;	
#endif
	char crossband_enabled;

	if(isFileExist("/var/run/crossband.pid"))
	{
		system("killall crossband_daemon >/dev/null 2>&1");
	}

	//mib_get(MIB_REPEATER_ENABLED1,(void *)&repeater_enabled1);
	//mib_get(MIB_REPEATER_ENABLED2,(void *)&repeater_enabled2);

	if(SetWlan_idx("wlan1")){ 
		mib_get_s( MIB_REPEATER_ENABLED1, (void *)&repeater_enabled1, sizeof(repeater_enabled1));
		if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry1))
			printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");
	}
	if(SetWlan_idx("wlan0")){ 
		mib_get_s( MIB_REPEATER_ENABLED1, (void *)&repeater_enabled2, sizeof(repeater_enabled2));
		if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry2))
			printf("Error! Get MIB_MBSSIB_TBL for root SSID error.\n");
	}
	
	mib_get_s( MIB_CROSSBAND_ENABLE, (void *)&crossband_enabled, sizeof(crossband_enabled));
	if(repeater_enabled1 && repeater_enabled2 && !Entry1.wlanDisabled && !Entry2.wlanDisabled && crossband_enabled) //dependency check
	{	
		printf("Starting crossband_daemon...\n");
		system("/bin/crossband_daemon");
	}else{
		printf( "Not starting crossband!\n");
	}

#if defined(WLAN_DUALBAND_CONCURRENT)
	wlan_idx = original_wlan_idx;	
#endif
	return 0;
}
#endif

/*
 *	vwlan_idx:
 *	0:	Root
 *	1 ~ 4:	VAP
 *	5:	Repeater
 */
static int wlanItfUpAndStartAuth(int vwlan_idx)
{
	int auth_pid_fd=-1;
	int status=0;
	char ifname[16] = {0};
	char ifname2[16] = {0};
	char para_auth_conf[30] = {0};
	char para_auth_fifo[30] = {0};
	char brname[32] = {0};
	MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_UNIVERSAL_REPEATER
	char rpt_enabled;
#endif
#ifdef WLAN_WISP
	unsigned int wan_mode;
	char wlanmode;
	int wisp_wan_id;
#endif

#ifdef _PRMT_X_WLANFORISP_
	MIB_WLANFORISP_T wlan_isp_entry;
#endif

	if(mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Entry) == 0)
		printf("\n %s %d\n", __func__, __LINE__);

	rtk_wlan_get_ifname(wlan_idx, vwlan_idx, ifname);

	if (Entry.wlanDisabled == 0
#if defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(CONFIG_YUEME)
		|| (vwlan_idx==WLAN_ROOT_ITF_INDEX)
#endif
		)	// WLAN enabled
	{
#ifdef WLAN_WISP
	mib_get_s( MIB_WAN_MODE, (void *)&wan_mode, sizeof(wan_mode));
	wlanmode = Entry.wlanMode;
#ifdef WLAN_UNIVERSAL_REPEATER
	mib_get_s( MIB_REPEATER_ENABLED1, (void *)&rpt_enabled, sizeof(rpt_enabled));
#endif
	if(!(wan_mode & MODE_Wlan)
#ifdef WLAN_UNIVERSAL_REPEATER
		|| (vwlan_idx == WLAN_REPEATER_ITF_INDEX && !getWispWanID(wlan_idx))
		|| (vwlan_idx != WLAN_REPEATER_ITF_INDEX)
		//|| (vwlan_idx != WLAN_REPEATER_ITF_INDEX && (wlanmode==AP_MODE || wlanmode==AP_WDS_MODE) && rpt_enabled) //vxd is WISP interface, add root & vap to bridge
		//|| (vwlan_idx == WLAN_REPEATER_ITF_INDEX && wlanmode==CLIENT_MODE) //root is WISP interface, add vxd to bridge
#endif
	){
#endif
		// brctl addif br0 wlan0
		strcpy(brname, BRIF);
#if defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_)
		if (get_grouping_ifname_by_groupnum(Entry.itfGroup, brname, sizeof(brname)) == NULL || !getInFlags(brname, NULL))
		{
			strcpy(brname, BRIF);
		}
#endif

#ifdef CONFIG_USER_FON
		if(vwlan_idx != WLAN_MBSSID_NUM) //last wlan vap is used for fon service
#endif
		status|=va_cmd(BRCTL, 3, 1, "addif", brname, ifname);
#ifdef WLAN_WISP
		if(vwlan_idx == WLAN_REPEATER_ITF_INDEX)
			setWlanDevFlag(ifname, 0);
#endif

#ifdef WLAN_WISP
	}else{
		if(vwlan_idx == WLAN_REPEATER_ITF_INDEX)
			setWlanDevFlag(ifname, 1);
	}
#endif
		if(vwlan_idx==WLAN_ROOT_ITF_INDEX){
#ifdef WLAN_USE_VAP_AS_SSID1
			status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=1");
#else
			if(Entry.wlanDisabled)
				status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=1");
			else{
#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_CTCAPD)
			if(Entry.func_off == 1)
				status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=1");
			else
				status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=0");
#else
				status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=0");
#endif
			}
#endif
		}

#if defined(CONFIG_CTC_SDN)
		unsigned char phyband = 0, portType;
		mib_get_s(MIB_WLAN_PHY_BAND_SELECT, &phyband, sizeof(phyband));
		if(phyband == PHYBAND_5G)
			portType = OFP_PTYPE_WLAN5G;
		else
			portType = OFP_PTYPE_WLAN2G;
		if(Entry.sdn_enable && check_ovs_enable() && check_ovs_running() == 0
			&& bind_ovs_interface_by_mibentry(portType, &Entry, vwlan_idx, wlan_idx) == 0)
		{
			printf("=====> Do bind WLAN(%s) to SDN <=====\n", ifname);
		}
		else
#endif
		// ifconfig wlan0 up
		status|=va_cmd(IFCONFIG, 2, 1, ifname, "up");
#ifdef WLAN_1x
		snprintf(para_auth_conf, sizeof(para_auth_conf), "/var/config/%s.conf", ifname);
		snprintf(para_auth_fifo, sizeof(para_auth_fifo), "/var/auth-%s.fifo", ifname);
		if (is8021xEnabled(vwlan_idx)) // 802.1x enabled, auth is only used when 802.1x is enable since encryption is driver based in 11n driver
		{ // Magician: Fixed parsing error by Source Insight
#ifdef _PRMT_X_WLANFORISP_
			//for e8, auth only enable when WlanForISP is set.
			if(getWlanForISPEntry(vwlan_idx, &wlan_isp_entry)<0){
				printf("%s %d: WLANForISP not sync, please check!!\n", __func__, __LINE__);
				return -1;
			}
			status|=generateWpaConf(para_auth_conf, 0, &Entry, &wlan_isp_entry);
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
			strcpy(ifname2, "any");
#else
			if(getWLANForISP_ifname(ifname2, &wlan_isp_entry)<0)
				strcpy(ifname2, LANIF);
#endif
#else
			status|=generateWpaConf(para_auth_conf, 0, &Entry);
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
			strcpy(ifname2, "any");
#else
			strcpy(ifname2, LANIF);
#endif
#endif //_PRMT_X_WLANFORISP_
			status|=va_niced_cmd(AUTH_DAEMON, 4, 0, ifname, ifname2, "auth", para_auth_conf);
			// fix the depency problem
			// check fifo
			int i = 0;
			while ((auth_pid_fd = open(para_auth_fifo, O_WRONLY)) == -1 && i < 10)
			{
				usleep(100000);
				i++;
			}
			if(auth_pid_fd!=-1) close(auth_pid_fd);/*jiunming, close the opened fd*/
			if (vwlan_idx == WLAN_ROOT_ITF_INDEX){ // Root
			        /* 2010-10-27 krammer :  use bit map to record what wlan root interface is use for auth*/

				useAuth_RootIf |= (1<<wlan_idx);//bit 0 ==> wlan0, bit 1 ==>wlan1
			}
			else {
				strcpy(para_iwctrl[wlan_num], ifname);
				wlan_num++;
			}

#if 0//def CONFIG_RTK_DEV_AP
			if(useAuth_RootIf==1 || useAuth_RootIf==2)
				snprintf(useAuth_RootIfname, sizeof(useAuth_RootIfname), "%s", ifname);
#endif
		
		}
#endif
		status = (status==-1)?-1:1;
	}
	else
		return 0;

	return status;

}

#ifdef WLAN_WEB_REDIRECT  //jiunming,web_redirect
int start_wlan_web_redirect(){

	int status=0;
	char tmpbuf[MAX_URL_LEN];
	char ipaddr[16], ip_port[32], redir_server[33];

	ipaddr[0]='\0'; ip_port[0]='\0';redir_server[0]='\0';
	if (mib_get_s(MIB_ADSL_LAN_IP, (void *)tmpbuf, sizeof(tmpbuf)) != 0)
	{
		strncpy(ipaddr, inet_ntoa(*((struct in_addr *)tmpbuf)), 16);
		ipaddr[15] = '\0';
		snprintf(ip_port, sizeof(ip_port), "%s:%d",ipaddr,8080);
	}//else ??

	if( mib_get_s(MIB_WLAN_WEB_REDIR_URL, (void*)tmpbuf, sizeof(tmpbuf)) )
	{
		char *p=NULL, *end=NULL;

		p = strstr( tmpbuf, "http://" );
		if(p)
			p = p + 7;
		else
			p = tmpbuf;

		end = strstr( p, "/" );
		if(end)
			*end = '\0';

		snprintf( redir_server,32,"%s",p );
		redir_server[32]='\0';
	}//else ??

	//iptables -t nat -N Web_Redirect
	status|=va_cmd(IPTABLES, 4, 1, "-t", "nat","-N","Web_Redirect");

	//iptables -t nat -A Web_Redirect -d 192.168.1.1 -j RETURN
	status|=va_cmd(IPTABLES, 8, 1, "-t", "nat","-A","Web_Redirect",
		"-d", ipaddr, "-j", (char *)FW_RETURN);

	//iptables -t nat -A Web_Redirect -d 192.168.2.11 -j RETURN
	status|=va_cmd(IPTABLES, 8, 1, "-t", "nat","-A","Web_Redirect",
		"-d", redir_server, "-j", (char *)FW_RETURN);

	//iptables -t nat -A Web_Redirect -p tcp --dport 80 -j DNAT --to-destination 192.168.1.1:8080
	status|=va_cmd(IPTABLES, 12, 1, "-t", "nat","-A","Web_Redirect",
		"-p", "tcp", "--dport", "80", "-j", "DNAT",
		"--to-destination", ip_port);

	//iptables -t nat -A PREROUTING -p tcp --dport 80 -j Web_Redirect
	status|=va_cmd(IPTABLES, 12, 1, "-t", "nat","-A","PREROUTING",
		"-i", (char *)getWlanIfName(),
		"-p", "tcp", "--dport", "80", "-j", "Web_Redirect");

	return status;
}
#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT

void wapi_cert_link_one(const char *name, const char *lnname) {
	char cmd[128];

	strcpy(cmd, "mkdir -p /var/myca/");
	system(cmd);


	strcpy(cmd, "ln -s ");
	strcat(cmd, name);
	strcat(cmd," ");
	strcat(cmd, lnname);
	system(cmd);
}

int start_aeUdpClient(void)
{
	int status=0;
	unsigned char tmpbuf[128], encrypt, wapiAuth;
	char ipaddr[16];
	extern void wapi_cert_link(void);

	mib_get_s(MIB_WLAN_ENCRYPT, (void *)&encrypt, sizeof(encrypt));
	mib_get_s(MIB_WLAN_WAPI_AUTH, (void *)&wapiAuth, sizeof(wapiAuth));
	if (encrypt != WIFI_SEC_WAPI || wapiAuth != 1) {
		goto OUT;
	}

	wapi_cert_link_one(WAPI_CA4AP_CERT_SAVE, WAPI_CA4AP_CERT);
	wapi_cert_link_one(WAPI_AP_CERT_SAVE, WAPI_AP_CERT);

	if (mib_get_s(MIB_WLAN_WAPI_ASIPADDR, (void *)tmpbuf, sizeof(tmpbuf)) == 0)
	{
		status = -1;
		goto OUT;
	}

	strncpy(ipaddr, inet_ntoa(*((struct in_addr *)tmpbuf)), 16);

	strncpy(tmpbuf, "aeUdpClient ", sizeof(tmpbuf));
	strncat(tmpbuf, "-d ", sizeof(tmpbuf));
	strncat(tmpbuf, ipaddr, sizeof(tmpbuf));
	strncat(tmpbuf, " -i wlan0 &", sizeof(tmpbuf));
	fprintf(stderr, "%s(%d): %s\n", __FUNCTION__,__LINE__, tmpbuf);
	system(tmpbuf);


OUT:
	return status;
}
#endif


int start_iwcontrol()
{
	int tmp, found = 0;
	int status = 0;
	char *argv[12];

	// When (1) WPS enabled or (2) Root AP's encryption is WPA/WPA2 without MBSSID,
	// we should start iwcontrol with wlan0 interface.
	int i = 0;
    char buf[8] = {0};

#if 1//def CONFIG_RTK_DEV_AP
	if(useAuth_RootIf){
		for(i = 0; useAuth_RootIf; i++)
		{
        	if(1)
			{
				snprintf(buf, sizeof(buf), WLANIF[i]);
	        	for (tmp=0; tmp < wlan_num; tmp++) {
	        		if (strcmp(para_iwctrl[tmp], buf)==0) {
	        			found = 1;
	        			break;
	        		}
	        	}
	        	if (!found) {
	        		strcpy(para_iwctrl[wlan_num], buf); //add root interface
	        		wlan_num++;
	        	}
	        }
	        useAuth_RootIf >>= 1;
		}
#if 0
	#if defined(TRIBAND_WPS)
        memset(useAuth_RootIfname, 0, sizeof(useAuth_RootIfname));
        for(i=0; i<NUM_WLAN_INTERFACE; ++i){    
            if (useAuth_RootIf & 0x1<<i) {        
                strcpy(para_iwctrl[wlan_num], WLANIF[i]);                
                wlan_num++;
            }
        }            
    #else /* !defined(TRIBAND_WPS) */
		if(useAuth_RootIf == 1 || useAuth_RootIf == 2)
		{			
			strcpy(para_iwctrl[0], useAuth_RootIfname);
			wlan_num = 1;
		}else {
			memset(useAuth_RootIfname, 0, sizeof(useAuth_RootIfname));
			for(i=0; i<2; ++i){				
				strcpy(para_iwctrl[wlan_num], WLANIF[i]);
				wlan_num++;
			}					
		}		
	#endif /* defined(TRIBAND_WPS) */
	#endif /* defined(TRIBAND_WPS) */
	}
#else
	for(i = 0; useAuth_RootIf; i++){
        	if ( useAuth_RootIf & 0x00000001) {
                      //snprintf(buf, sizeof(buf), "wlan%d", i);
                      snprintf(buf, sizeof(buf), WLANIF[i]);
        		for (tmp=0; tmp < wlan_num; tmp++) {
        			if (strcmp(para_iwctrl[tmp], buf)==0) {
        				found = 1;
        				break;
        			}
        		}
        		if (!found) {
        			strcpy(para_iwctrl[wlan_num], buf);
        			wlan_num++;
        		}
        	}
               useAuth_RootIf >>= 1;
	}
#endif

	printf("Total WPA/WPA2 number is %d\n", wlan_num);
	if(wlan_num>0){
		//printf("CMD ARGS: ");
		for(i=0; i<wlan_num; i++){
			argv[i+1] = para_iwctrl[i];
		//	printf("%s", argv[i+1]);
		}
		argv[i+1]=NULL;
		//printf("\n");
		status|=do_nice_cmd(IWCONTROL, argv, 1);
#if defined(CONFIG_USER_MONITORD) && (defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME))
		update_monitor_list_file("iwcontrol", 1);
#endif
	}
	return status;

}

// return value:
// 0  : success
// -1 : failed
int startWLanOneTimeDaemon()
{
	int status=0;

#ifdef HS2_SUPPORT
	status |= start_hs2();
#endif
#ifdef WLAN_11R
	status |= start_FT();
#endif
#ifdef WLAN_SMARTAPP_ENABLE
	status |= start_smart_wlanapp();
#endif
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
#ifdef WLAN_WPS_VAP
	status |= start_WPS(1);
#else
	status |= start_WPS();
#endif
#endif
#ifdef WLAN_WEB_REDIRECT
	status |= start_wlan_web_redirect();
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT
	status |= start_aeUdpClient();
#endif
#ifdef CONFIG_IAPP_SUPPORT
		status |= start_IAPP();
#endif
#ifdef WLAN_SCHEDULE_SUPPORT
		status |= start_func_wlan_schedule();
#endif
#ifdef RTK_CROSSBAND_REPEATER
		status |= start_crossband();
#endif

	return status;
}
#endif

#ifdef WLAN_ROOT_MIB_SYNC
#ifdef WLAN_SUPPORT
int check_wlan_mib()
{
	int wlan_idx_bak = wlan_idx;
	MIB_CE_MBSSIB_T Entry;
	unsigned char vChar;
	int i=0, syncRoot=0;
	unsigned char ssid[MAX_SSID_LEN]={0}, password[MAX_PSK_LEN+1]={0};
	unsigned char rsIpAddr[IP_ADDR_LEN];
	unsigned short vShort;
	unsigned char wep64Key[WEP64_KEY_LEN]={0}, wep128Key[WEP128_KEY_LEN]={0};
	unsigned int vInt;

	for(i=0; i<NUM_WLAN_INTERFACE; ++i)
	{
		memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));
		wlan_idx = i;
		mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry);

		mib_get_s(MIB_WLAN_ENCRYPT, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.encrypt){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_ENABLE_1X, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.enable1X){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WEP, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wep){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WPA_AUTH, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wpaAuth){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WPA_PSK_FORMAT, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wpaPSKFormat){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WPA_PSK, (void *)password, sizeof(password));
		if(memcmp(password, Entry.wpaPSK, sizeof(password))){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WPA_GROUP_REKEY_TIME, (void *)&vInt, sizeof(vInt));
		if(vInt!=Entry.wpaGroupRekeyTime){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_RS_IP, (void *)rsIpAddr, sizeof(rsIpAddr));
		if(memcmp(rsIpAddr, Entry.rsIpAddr, sizeof(rsIpAddr))){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_RS_PORT, (void *)&vShort, sizeof(vShort));
		if(vShort!=Entry.rsPort){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_RS_PASSWORD, (void *)password, sizeof(password));
		if(memcmp(password, Entry.rsPassword, sizeof(password))){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_DISABLED, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wlanDisabled){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_SSID, (void *)ssid, sizeof(ssid));
		if(memcmp(ssid, Entry.ssid, sizeof(ssid))){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_MODE, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wlanMode){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_AUTH_TYPE, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.authType){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.unicastCipher){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wpa2UnicastCipher){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_HIDDEN_SSID, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.hidessid){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_QoS, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wmmEnabled){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_MCAST2UCAST_DISABLE, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.mc2u_disable){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_BLOCK_RELAY, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.userisolation){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WEP_KEY_TYPE, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wepKeyType){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wepDefaultKey){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WEP64_KEY1, (void *)wep64Key, sizeof(wep64Key));
		if(memcmp(wep64Key, Entry.wep64Key1, sizeof(wep64Key))){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WEP64_KEY2, (void *)wep64Key, sizeof(wep64Key));
		if(memcmp(wep64Key, Entry.wep64Key2, sizeof(wep64Key))){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WEP64_KEY3, (void *)wep64Key, sizeof(wep64Key));
		if(memcmp(wep64Key, Entry.wep64Key3, sizeof(wep64Key))){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WEP64_KEY4, (void *)wep64Key, sizeof(wep64Key));
		if(memcmp(wep64Key, Entry.wep64Key4, sizeof(wep64Key))){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WEP128_KEY1, (void *)wep128Key, sizeof(wep128Key));
		if(memcmp(wep128Key, Entry.wep128Key1, sizeof(wep128Key))){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WEP128_KEY2, (void *)wep128Key, sizeof(wep128Key));
		if(memcmp(wep128Key, Entry.wep128Key2, sizeof(wep128Key))){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WEP128_KEY3, (void *)wep128Key, sizeof(wep128Key));
		if(memcmp(wep128Key, Entry.wep128Key3, sizeof(wep128Key))){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_WEP128_KEY4, (void *)wep128Key, sizeof(wep128Key));
		if(memcmp(wep128Key, Entry.wep128Key4, sizeof(wep128Key))){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_RATE_ADAPTIVE_ENABLED, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.rateAdaptiveEnabled){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_BAND, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wlanBand){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_FIX_RATE, (void *)&vInt, sizeof(vInt));
		if(vInt!=Entry.fixedTxRate){
			syncRoot = 1;
			break;
		}
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
		mib_get_s(MIB_WSC_DISABLE, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wsc_disabled){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WSC_CONFIGURED, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wsc_configured){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WSC_UPNP_ENABLED, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wsc_upnp_enabled){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WSC_AUTH, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wsc_auth){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WSC_ENC, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.wsc_enc){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WSC_PSK, (void *)password, sizeof(password));
		if(memcmp(password, Entry.wscPsk, sizeof(password))){
			syncRoot = 1;
			break;
		}
#endif
#ifdef WLAN_11W
		mib_get_s(MIB_WLAN_DOTIEEE80211W, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.dotIEEE80211W){
			syncRoot = 1;
			break;
		}
		mib_get_s(MIB_WLAN_SHA256, (void *)&vChar, sizeof(vChar));
		if(vChar!=Entry.sha256){
			syncRoot = 1;
			break;
		}
#endif
	}

	if(syncRoot){
		printf("[%s]Sync wlan root mib from table\n", __FUNCTION__);
		for(i=0; i<NUM_WLAN_INTERFACE; ++i)
		{
			memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));
			wlan_idx = i;
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
		base = PMAP_WLAN1;
		ssid_index -= 4;
	}
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


//--------------------------------------------------------
// Wireless LAN startup
// return value:
// 0  : not start by configuration
// 1  : successful
// -1 : failed
int startWLan(config_wlan_target target, config_wlan_ssid ssid_index)
{
	unsigned char no_wlan, wsc_disable, wlan_mode;
	int status=0, upnppid=0;
	char *argv[9];
	unsigned char phy_band_select;

#ifdef CONFIG_USER_CTCAPD
	FILE *fp;
	char line[20]={0};
	char tmpbuf;
	char intfname[16]={0}, tmp_buffer[50]={0};
	int n=0, wps_pid = -1;
#ifdef RTK_MULTI_AP
	unsigned char mode=0;
	FILE *fp2;
	int wps_status=1;
#endif
#endif

#ifdef WLAN_ROOT_MIB_SYNC
#ifdef WLAN_SUPPORT
	check_wlan_mib();
#endif
#endif

#if 0
	// Check wireless interface
	if (!getInFlags((char *)WLANIF, 0)) {
		printf("Wireless Interface Not Found !\n");
		return -1;	// interface not found
	}

	mib_get_s(MIB_WLAN_DISABLED, (void *)&no_wlan, sizeof(no_wlan));
	if (no_wlan)
		return 0;

	//1/17/06' hrchen, always start WLAN MIB, for MP test will use
	// "ifconfig wlan0 up" to start WLAN
	// config WLAN
	status|=setupWLan();
#endif
	int i = 0, active_wlan = 0, j = 0, orig_wlan_idx;
	char ifname[16];
	MIB_CE_MBSSIB_T Entry;

#if !defined(YUEME_3_0_SPEC) && !defined(CONFIG_CU_BASEON_YUEME)
	if(!check_wlan_module())
	{
#ifdef CONFIG_USER_CTCAPD		
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
				}
			}

			fclose(fp);
			fp = NULL;
			unlink("tmp/wscd_trigger");
		}
#endif
		return 1;
	}
#endif
	// Modified by Mason Yu
	wlan_num = 0; /*reset to 0,jiunming*/
	useAuth_RootIf = 0;  /*reset to 0 */
	orig_wlan_idx = wlan_idx;

#if defined(CONFIG_WLAN) /*&& defined(CONFIG_RTK_DEV_AP) */ && defined(CONFIG_WIFI_SIMPLE_CONFIG)
	int wlan_support = 0;
	memset(wlan_valid_interface, 0x00, sizeof(wlan_valid_interface));
	wlan_support = if_readlist_proc(wlan_valid_interface, "wlan", 0);
#endif	

	//process each wlan interface
	for(i = 0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		if (!getInFlags((char *)getWlanIfName(), 0)) {
			printf("Wireless Interface Not Found !\n");
			status = -1;	// interface not found
			continue;
	    }

#if defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(YUEME_3_0_SPEC)
		mib_get_s(MIB_WLAN_MODULE_DISABLED, (void *)&no_wlan, sizeof(no_wlan));
#if !defined(CONFIG_RTK_DEV_AP)
		if(no_wlan == 0)
			no_wlan = getWlanStatus(i)? 0:1;
#endif
#elif defined(CONFIG_TELMEX_DEV)
		mib_get_s(MIB_WLAN_RADIO_DISABLED, (void *)&no_wlan, sizeof(no_wlan));
		mib_chain_get(MIB_MBSSIB_TBL, WLAN_ROOT_ITF_INDEX, &Entry);
		no_wlan |= Entry.wlanDisabled;
#else
		if(mib_chain_get(MIB_MBSSIB_TBL, WLAN_ROOT_ITF_INDEX, &Entry) == 0)
			printf("\n %s %d\n", __func__, __LINE__);
		no_wlan = Entry.wlanDisabled;
#endif
		if (no_wlan){
			continue;
		}

		//interface and root is enable, now we start
		active_wlan++;

		mib_get_s( MIB_WLAN_PHY_BAND_SELECT, (void *)&phy_band_select, sizeof(phy_band_select));
		if((target == CONFIG_WLAN_2G && phy_band_select == PHYBAND_5G)
		  || (target == CONFIG_WLAN_5G && phy_band_select == PHYBAND_2G)) {
			continue;
		}

		if(ssid_index==CONFIG_SSID_ALL || ssid_index==CONFIG_SSID_ROOT){
#ifdef WLAN_FAST_INIT
			rtk_wlan_get_ifname(i, WLAN_ROOT_ITF_INDEX, ifname);
			setupWLan(ifname, WLAN_ROOT_ITF_INDEX);
#else
			status |= setupWLan(WLAN_ROOT_ITF_INDEX);
#endif
		}

		//ifconfig wlan up ,add into bridge and start [auth<-depend on interface, so run many times]
		status |= wlanItfUpAndStartAuth(WLAN_ROOT_ITF_INDEX);

		setupWlanHWRegSettings(getWlanIfName());
#if 0 // !defined(WLAN_DUALBAND_CONCURRENT)
		{
			#define RTL8185_IOCTL_GET_MIB   0x89f2
			int skfd;
			struct iwreq wrq;
			char value[16];

			//CurrentChannel
			mib_get_s(MIB_WLAN_AUTO_CHAN_ENABLED, (void *)value, sizeof(value));
			if( value[0] ){ //root ap is auto channel
				do {		//wait for selecting channel
					sleep(1);
					skfd = socket(AF_INET, SOCK_DGRAM, 0);
					strcpy(wrq.ifr_name, getWlanIfName());
					strcpy(value,"opmode");
					wrq.u.data.pointer = (caddr_t)&value;
					wrq.u.data.length = 10;
					ioctl(skfd, RTL8185_IOCTL_GET_MIB, &wrq);
					close( skfd );
				}while(value[0] == 0x04);	//WIFI_WAIT_FOR_CHANNEL_SELECT
			}
		}
#endif
		if(mib_chain_get(MIB_MBSSIB_TBL, WLAN_ROOT_ITF_INDEX, &Entry) == 0)
			printf("\n %s %d\n", __func__, __LINE__);
		wlan_mode = Entry.wlanMode;
#ifdef WLAN_MBSSID
		if (wlan_mode == AP_MODE || wlan_mode == AP_WDS_MODE
#ifdef WLAN_MESH
			|| wlan_mode ==  AP_MESH_MODE
#endif
		) {
			for (j=WLAN_VAP_ITF_INDEX; j<(WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM); j++){
				if(ssid_index!=CONFIG_SSID_ALL && ssid_index!=j) {
					continue;
				}
#ifdef WLAN_FAST_INIT
				rtk_wlan_get_ifname(i, j, ifname);
				setupWLan(ifname, j);
#else
				status |= setupWLan(j);
#endif
			    status |= wlanItfUpAndStartAuth(j);
			}
		}
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
		if (wlan_mode != WDS_MODE) {
			char rpt_enabled;
			mib_get_s(MIB_REPEATER_ENABLED1, (void *)&rpt_enabled, sizeof(rpt_enabled));
			if (rpt_enabled){
#ifdef WLAN_FAST_INIT
				rtk_wlan_get_ifname(i, WLAN_REPEATER_ITF_INDEX, ifname);
				setupWLan(ifname, WLAN_REPEATER_ITF_INDEX);
#else
				status |= setupWLan(WLAN_REPEATER_ITF_INDEX);
#endif
			    status |= wlanItfUpAndStartAuth(WLAN_REPEATER_ITF_INDEX);
			}
		}
#endif
#ifdef CONFIG_YUEME
		setupWlanVSIE(i, phy_band_select); //must set after interface up
#endif
	}
	wlan_idx = orig_wlan_idx;
	if(!active_wlan){
		return status;
	}
	else{
		check_iwcontrol_8021x();
	}

	//start wlan daemon at last due to these daemons only need one
#ifdef WLAN_MESH
	status|=setupWLanMeshDaemon();
#endif
	status|=startWLanOneTimeDaemon();
	status|=start_iwcontrol();

#ifdef RTK_SMART_ROAMING
	start_capwap();
#endif

#ifdef _PRMT_X_CMCC_WLANSHARE_
	setup_wlan_share();
#endif

#ifdef WLAN_SUPPORT
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
	orig_wlan_idx = wlan_idx;
	char wscd_pid_name[32];
	for(i = 0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		if(mib_chain_get(MIB_MBSSIB_TBL, 0, &Entry) == 0)
			printf("\n %s %d\n", __func__, __LINE__);
		no_wlan = Entry.wlanDisabled;
		wsc_disable = Entry.wsc_disabled;
		//upnppid = read_pid((char*)MINI_UPNPDPID);	//cathy
		if( !no_wlan && !wsc_disable && !is8021xEnabled(0)) {
			int retry = 10;
			getWscPidName(wscd_pid_name);
			while(--retry && (read_pid(wscd_pid_name) < 0))
			{
				//printf("WSCD is not running. Please wait!\n");
				usleep(300000);
			}
			startSSDP();
			break;
		}
	}
	wlan_idx = orig_wlan_idx;
#endif
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	setup_wlan_MAC_ACL();
#endif

#ifdef RTK_MULTI_AP
	rtk_start_multi_ap_service();
#endif

#if defined (CONFIG_ELINK_SUPPORT) || defined(CONFIG_USER_CTCAPD)
	setTelcoSelected(TELCO_CT);
#elif defined(CONFIG_ANDLINK_SUPPORT)
	setTelcoSelected(TELCO_CMCC);
#endif

#if defined(CONFIG_ANDLINK_SUPPORT)
	setupAndlink();
#endif

#if defined(CONFIG_USER_AP_CMCC) && defined(CONFIG_USER_ANDLINK_PLUGIN)
		setupAndlinkPlugin();
#endif

#ifdef CONFIG_ELINK_SUPPORT
	setupElink();
#endif

#ifdef CONFIG_USER_CTCAPD
	if((fp = fopen("tmp/wscd_trigger", "r")) != NULL)
	{
		if(fgets(line, sizeof(line), fp))
		{
			if (sscanf(line, "%c", &tmpbuf))
			{
				if (tmpbuf == '0')
				{
					system("ubus send \"propertieschanged\" \'{\"wpsswitch\":\"off\"}\'");
					//system("echo 1 > /tmp/wscd_cancel");
				}
				else if(tmpbuf == '1')	
				{
					system("ubus send \"propertieschanged\" \'{\"wpsswitch\":\"on\"}\'");
#ifdef RTK_MULTI_AP
					mib_get_s(MIB_MAP_CONTROLLER, (void *)&(mode), sizeof(mode));
					if(mode)
					{
						system("echo 1 > /tmp/virtual_push_button");
						for(i=0;i<3;i++)
						{
							wps_status = 1;
							fp2 = fopen("/tmp/wscd_status", "r");
							if(fp2){
								fscanf(fp2, "%d", &wps_status);
								fclose(fp2);
							}

							if(wps_status == 0)
								break;
							else
								system("echo 1 > /tmp/virtual_push_button");

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

#ifdef CONFIG_WIFI_SIMPLE_CONFIG
							if(Entry.wsc_disabled)
#endif
								continue;
							for(n=0; n<2; n++)
							{	
								wps_pid = findPidByName("wscd");
								if(wps_pid <= 0)
								{
									break;
								}

								sleep(1);
							}
							if(n==2 && wps_pid>0)
							{
								snprintf(tmp_buffer,sizeof(tmp_buffer),"/bin/wscd -sig_pbc %s",intfname);
								system(tmp_buffer);
							}
							if(i == 0)
								break;
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
#endif

	return status;
}

int getMiscData(char *interface, struct _misc_data_ *pData)
{

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

    return 0;
}

int getWlStaNum( char *interface, int *num )
{
    int skfd;
    unsigned short staNum;
    struct iwreq wrq;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);

    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
      close( skfd );
      /* If no wireless name : no wireless extensions */
      return -1;
    }

    wrq.u.data.pointer = (caddr_t)&staNum;
    wrq.u.data.length = sizeof(staNum);

    if (iw_get_ext(skfd, interface, SIOCGIWRTLSTANUM, &wrq) < 0) {
      close( skfd );
      return -1;
    }

    *num  = (int)staNum;

    if(skfd != -1)
        close( skfd );

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int getWlSiteSurveyResult(char *interface, SS_STATUS_Tp pStatus )
{
    int skfd;
    struct iwreq wrq;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
       close( skfd );
      /* If no wireless name : no wireless extensions */
       return -1;
    }

    wrq.u.data.pointer = (caddr_t)pStatus;

    if ( pStatus->number == 0 )
    	wrq.u.data.length = sizeof(SS_STATUS_T);
    else
        wrq.u.data.length = sizeof(pStatus->number);

    if (iw_get_ext(skfd, interface, SIOCGIWRTLGETBSSDB, &wrq) < 0) {
      close( skfd );
      return -1;
    }

    close( skfd );

    return 0;
}

#if defined(WLAN_CLIENT)
/////////////////////////////////////////////////////////////////////////////
int getWlJoinRequest(char *interface, pBssDscr pBss, unsigned char *res)
{
    int skfd;
    struct iwreq wrq;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;
    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
      close( skfd );
      /* If no wireless name : no wireless extensions */
      return -1;
    }

    wrq.u.data.pointer = (caddr_t)pBss;
    wrq.u.data.length = sizeof(BssDscr);

    if (iw_get_ext(skfd, interface, SIOCGIWRTLJOINREQ, &wrq) < 0) {
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
    int skfd;
    struct iwreq wrq;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
      close( skfd );
      /* If no wireless name : no wireless extensions */
      return -1;
    }

    wrq.u.data.pointer = (caddr_t)res;
    wrq.u.data.length = 1;

    if (iw_get_ext(skfd, interface, SIOCGIWRTLJOINREQSTATUS, &wrq) < 0) {
      close( skfd );
      return -1;
    }

    close( skfd );

    return 0;
}
#endif


/////////////////////////////////////////////////////////////////////////////
int getWlSiteSurveyRequest(char *interface, int *pStatus)
{
    int skfd;
    struct iwreq wrq;
    unsigned char result;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
      close( skfd );
      /* If no wireless name : no wireless extensions */
      return -1;
    }

    wrq.u.data.pointer = (caddr_t)&result;
    wrq.u.data.length = sizeof(result);

    if (iw_get_ext(skfd, interface, SIOCGIWRTLSCANREQ, &wrq) < 0) {
      close( skfd );
      return -1;
    }

    close( skfd );

    if ( result == 0xff )
    	*pStatus = -1;
    else
	*pStatus = (int) result;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
int getWlBssInfo(const char *interface, bss_info *pInfo)
{
    int skfd;
    struct iwreq wrq;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
      close( skfd );
      /* If no wireless name : no wireless extensions */
      return -1;
    }

    wrq.u.data.pointer = (caddr_t)pInfo;
    wrq.u.data.length = sizeof(bss_info);

    if (iw_get_ext(skfd, interface, SIOCGIWRTLGETBSSINFO, &wrq) < 0) {
      close( skfd );
      return -1;
    }

    close( skfd );

    return 0;
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

int getWlIdlePercentInfo(char *interface, unsigned int *pInfo)
{
	int ret = -1;
	FILE *fp = NULL;
	char wlan_path[128] = {0}, line[1024] = {0};
	snprintf(wlan_path, sizeof(wlan_path), "/proc/%s/ch_load", interface);

	fp = fopen(wlan_path, "r");
	if (fp) {
		while (fgets(line, sizeof(line),fp)) {
			if (strstr(line, "free_Time")) {
				if(sscanf(line,"%*[^:]: %u", pInfo))
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
	fprintf(stderr, "[%s] ifname [%s], use40M = %u, coexist = %u, *pInfo = %u\n", __func__, interface, use40M, coexist, *pInfo);
	
	return ret;
}


#ifdef WLAN_WDS
/////////////////////////////////////////////////////////////////////////////
int getWdsInfo(char *interface, char *pInfo)
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

    wrq.u.data.pointer = (caddr_t)pInfo;
    wrq.u.data.length = MAX_WDS_NUM*sizeof(WDS_INFO_T);

    if (iw_get_ext(skfd, interface, SIOCGIWRTLGETWDSINFO, &wrq) < 0) {
      close( skfd );
      return -1;
    }

    close( skfd );

    return 0;
}

#endif

int getWlVersion(const char *interface, char *verstr )
{
    int skfd;
    unsigned char vernum[4];
    struct iwreq wrq;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);

    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
      close( skfd );
      /* If no wireless name : no wireless extensions */
      return -1;
    }

    memset(vernum, 0, 4);
    wrq.u.data.pointer = (caddr_t)&vernum[0];
    wrq.u.data.length = sizeof(vernum);

    if (iw_get_ext(skfd, interface, SIOCGIWRTLDRVVERSION, &wrq) < 0) {
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
#if defined(WLAN_DUALBAND_CONCURRENT)
	else if(wlan_idx == 1)
		return (char *)WLANIF[1];
#endif
#if defined(TRIBAND_SUPPORT)
	else if(wlan_idx == 2)
		return (char *)WLANIF[2];
#endif /* defined(TRIBAND_SUPPORT) */

	printf("%s: Wrong wlan_idx!\n", __func__);

	return NULL;
}

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

void getWscPidName(char *wscd_pid_name)
{
	int orig_wlan_idx = wlan_idx;
#if defined(WLAN_DUALBAND_CONCURRENT)
	int pid;
	pid = read_pid((char *) WLAN0_WLAN1_WSCDPID);
	if (pid > 0) {
		sprintf(wscd_pid_name, "%s", WLAN0_WLAN1_WSCDPID);
		return;
	}
	wlan_idx = 1;
	sprintf(wscd_pid_name, WSCDPID, getWlanIfName());
	pid = read_pid((char *) wscd_pid_name);
	if (pid > 0) {
		wlan_idx = orig_wlan_idx;
		return;
	}
#endif
	wlan_idx = 0;
	sprintf(wscd_pid_name, WSCDPID, getWlanIfName());
	wlan_idx = orig_wlan_idx;
}
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
#ifdef WPS20
int checkWpsDisableStatus(MIB_CE_MBSSIB_Tp Entry)
{
	int disableWps = 0;
	if(Entry->encrypt ==  WIFI_SEC_WPA || Entry->encrypt == WIFI_SEC_WPA2 || Entry->encrypt == WIFI_SEC_WPA2_MIXED){
		if (((Entry->encrypt == WIFI_SEC_WPA) ||
			(Entry->encrypt == WIFI_SEC_WPA2_MIXED && Entry->unicastCipher == WPA_CIPHER_TKIP))) {	//disable wps if wpa only or tkip only
			disableWps = 1;
		}
		if (Entry->encrypt == WIFI_SEC_WPA2 || Entry->encrypt == WIFI_SEC_WPA2_MIXED) {
			if (Entry->encrypt == WIFI_SEC_WPA2) {
				if (Entry->wpa2UnicastCipher == WPA_CIPHER_TKIP)
					disableWps = 1;
			}
			else { // mixed
				if (Entry->wpa2UnicastCipher == WPA_CIPHER_TKIP && disableWps)	//disable wps if wpa2 mixed + tkip only
					disableWps = 1;
				else
					disableWps = 0;
			}
		}
	}
	if(Entry->encrypt == WIFI_SEC_WEP)
		disableWps = 1;
	return disableWps;
}
#endif //WPS20
#endif
//check flash config if wlan0 is up, called by BOA,
//0:down, 1:up
int wlan_is_up(void)
{
	int value=0;
#ifdef CONFIG_TELMEX_DEV
	char radioDisable = 0; 
#endif
	MIB_CE_MBSSIB_T Entry;

	if(mib_chain_get(MIB_MBSSIB_TBL,0,&Entry) == 0)
		printf("\n %s %d\n", __func__, __LINE__);
#ifdef CONFIG_TELMEX_DEV
	mib_get_s(MIB_WLAN_RADIO_DISABLED, (void *)&radioDisable, sizeof(radioDisable));
#endif

    if (Entry.wlanDisabled==0
#ifdef CONFIG_TELMEX_DEV
		&& (radioDisable == 0)
#endif
	) {  // wlan0 is enabled
    	value=1;
    }
	return value;
}
#ifdef WLAN_WISP
int getWispWanID(int idx)
{
	int i, mibtotal;
	MIB_CE_ATM_VC_T Entry;

	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0; i<mibtotal; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if(MEDIA_INDEX(Entry.ifIndex) == MEDIA_WLAN && ETH_INDEX(Entry.ifIndex)==idx)
			break;
	}

	return Entry.enable;
}

int checkWispAvailability()
{
	int i=0, ret=0;
	char rptEnabled;
	MIB_CE_MBSSIB_T Entry;
	int orig_idx = wlan_idx;
	for(i=0;i<NUM_WLAN_INTERFACE;i++){
		wlan_idx = i;
		wlan_getEntry(&Entry, i);
		mib_get_s(MIB_REPEATER_ENABLED1, &rptEnabled, sizeof(rptEnabled));
		if((Entry.wlanMode == AP_MODE || Entry.wlanMode == AP_WDS_MODE) && rptEnabled){
			wlan_idx = orig_idx;
			return 1;
		}
	}
	wlan_idx = orig_idx;
	return 0;
}

//return wisp wan interface name
void getWispWanName(char *name, int idx)
{
	snprintf(name, IFNAMSIZ, "%s-vxd", WLANIF[idx]);
}
void setWlanDevFlag(char *ifname, int set_wan)
{
	char cmd_str[100];

	/* echo {intf name} write {mode} > /proc/realtek/netdev_flag 
	** 	mode:	0(WISP mode)
	**		1(WLAN mode)
	**/
	if(set_wan){
		snprintf(cmd_str, sizeof(cmd_str), "echo \"%s write 0\" > /proc/realtek/netdev_flag", ifname);
		system(cmd_str);
	}
	else{
		snprintf(cmd_str, sizeof(cmd_str), "echo \"%s write 1\" > /proc/realtek/netdev_flag", ifname);
		system(cmd_str);
		snprintf(cmd_str, sizeof(cmd_str), "ifconfig %s 0.0.0.0", ifname);
		system(cmd_str);
	}
}
#endif

#ifdef CONFIG_WIFI_SIMPLE_CONFIG //WPS
// this is a workaround for wscd to get MIB id without including "mib.h" (which causes compile error), andrew
const int gMIB_WLAN_DISABLED		  = MIB_WLAN_DISABLED;
const int gMIB_WSC_DISABLE			  = MIB_WSC_DISABLE;
const int gMIB_WSC_CONFIGURED         = MIB_WSC_CONFIGURED;
const int gMIB_WLAN_SSID              = MIB_WLAN_SSID;
const int gMIB_WSC_SSID               = MIB_WSC_SSID;
const int gMIB_WLAN_AUTH_TYPE         = MIB_WLAN_AUTH_TYPE;
const int gMIB_WLAN_ENCRYPT           = MIB_WLAN_ENCRYPT;
const int gMIB_WSC_AUTH               = MIB_WSC_AUTH;
const int gMIB_WLAN_WPA_AUTH          = MIB_WLAN_WPA_AUTH;
const int gMIB_WLAN_WPA_PSK           = MIB_WLAN_WPA_PSK;
const int gMIB_WLAN_WPA_PSK_FORMAT    = MIB_WLAN_WPA_PSK_FORMAT;
const int gMIB_WSC_PSK                = MIB_WSC_PSK;
const int gMIB_WLAN_WPA_CIPHER_SUITE  = MIB_WLAN_WPA_CIPHER_SUITE;
const int gMIB_WLAN_WPA2_CIPHER_SUITE = MIB_WLAN_WPA2_CIPHER_SUITE;
const int gMIB_WLAN_WEP               = MIB_WLAN_WEP;
const int gMIB_WLAN_WEP64_KEY1        = MIB_WLAN_WEP64_KEY1;
const int gMIB_WLAN_WEP64_KEY2        = MIB_WLAN_WEP64_KEY2;
const int gMIB_WLAN_WEP64_KEY3        = MIB_WLAN_WEP64_KEY3;
const int gMIB_WLAN_WEP64_KEY4        = MIB_WLAN_WEP64_KEY4;
const int gMIB_WLAN_WEP128_KEY1       = MIB_WLAN_WEP128_KEY1;
const int gMIB_WLAN_WEP128_KEY2       = MIB_WLAN_WEP128_KEY2;
const int gMIB_WLAN_WEP128_KEY3       = MIB_WLAN_WEP128_KEY3;
const int gMIB_WLAN_WEP128_KEY4       = MIB_WLAN_WEP128_KEY4;
const int gMIB_WLAN_WEP_DEFAULT_KEY   = MIB_WLAN_WEP_DEFAULT_KEY;
const int gMIB_WLAN_WEP_KEY_TYPE      = MIB_WLAN_WEP_KEY_TYPE;
const int gMIB_WSC_ENC                = MIB_WSC_ENC;
const int gMIB_WSC_CONFIG_BY_EXT_REG  = MIB_WSC_CONFIG_BY_EXT_REG;
const int gMIB_WSC_PIN = MIB_WSC_PIN;
const int gMIB_DEVICE_NAME = MIB_DEVICE_NAME;
const int gMIB_WLAN_MAC_ADDR = MIB_WLAN_MAC_ADDR;
const int gMIB_ELAN_MAC_ADDR = MIB_ELAN_MAC_ADDR;
const int gMIB_WLAN_MODE = MIB_WLAN_MODE;
const int gMIB_WSC_REGISTRAR_ENABLED = MIB_WSC_REGISTRAR_ENABLED;
const int gMIB_WLAN_CHAN_NUM = MIB_WLAN_CHAN_NUM;
const int gMIB_WSC_UPNP_ENABLED = MIB_WSC_UPNP_ENABLED;
const int gMIB_WSC_METHOD = MIB_WSC_METHOD;
const int gMIB_WSC_MANUAL_ENABLED = MIB_WSC_MANUAL_ENABLED;
const int gMIB_SNMP_SYS_NAME = MIB_SNMP_SYS_NAME;
const int gMIB_WLAN_NETWORK_TYPE = MIB_WLAN_NETWORK_TYPE;
const int gMIB_WLAN_WSC_VERSION	= MIB_WSC_VERSION;
#ifdef CONFIG_ANDLINK_SUPPORT
const int gMIB_RTL_LINK_ENABLE	= MIB_RTL_LINK_ENABLE;
#endif



#if defined(CONFIG_RTK_DEV_AP)
const int gMIB_WLAN_BAND2G5G_SELECT				= MIB_WLAN_BAND2G5G_SELECT;
#endif

int mib_update_all()
{
	return mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
}
#endif

#ifdef CONFIG_GUEST_ACCESS_SUPPORT
int setup_wlan_guestaccess(int vwlan_idx)
{
	MIB_CE_MBSSIB_T mbssid_entry;
	char ifname[16]={0}, parm[64]={0};
	
	if(mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&mbssid_entry) == 0)
		printf("[%s %d]Error! Get MIB_MBSSIB_TBL error.\n",__FUNCTION__,__LINE__);

	rtk_wlan_get_ifname(wlan_idx, vwlan_idx, ifname);
	snprintf(parm, sizeof(parm), "guest_access=%u", mbssid_entry.guest_access);
	va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	snprintf(parm, sizeof(parm), "block_relay=%u", mbssid_entry.userisolation);
	va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	if(mbssid_entry.guest_access && mbssid_entry.userisolation){
		/*********between wlan and lan**********/
		//ebtables -A wlan_block -i wlan1-vap0 -o eth+ -j DROP
		va_cmd(EBTABLES, 8, 1, "-A", (char *)FW_FORWARD, "-i", ifname, "-o", "eth+", "-j", "DROP");
		//ebtables -A wlan_block -i wlan1-vap0 -o wlan+ -j DROP
		va_cmd(EBTABLES, 8, 1, "-A", (char *)FW_FORWARD, "-i", ifname, "-o", "wlan+", "-j", "DROP");
		/*********wlan1-vap0 sta can't acces to internet*******/
		//iptables -I INPUT -m physdev --physdev-in wlan1-vap0 -p tcp -j DROP
		va_cmd(IPTABLES, 10, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-m", "physdev", "--physdev-in", ifname, "-p", "tcp", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6		
		va_cmd(IP6TABLES, 10, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-m", "physdev", "--physdev-in", ifname, "-p", "tcp", "-j", (char *)FW_DROP);
#endif
	}
}
#endif

int setup_wlan_block(void)
{
#ifdef WLAN_MBSSID
	int i;
	char wlan_dev[16];
	unsigned char wlan_hw_diabled;
#if defined(CONFIG_E8B) || defined(CONFIG_00R0)
	int j;
	MIB_CE_MBSSIB_T Entry;
#endif
#endif
	unsigned char enable = 0;

	va_cmd(EBTABLES, 2, 1, "-F", "wlan_block");

	// Between ELAN & WIFI
	mib_get_s(MIB_WLAN_BLOCK_ETH2WIR, (void *)&enable, sizeof(enable));
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
#if defined(CONFIG_E8B) || defined(CONFIG_00R0)
		for ( j=0; j<=NUM_VWLAN_INTERFACE; j++) 
		{
			if (!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&Entry)) {
				printf("Error! Get MIB_MBSSIB_TBL error.\n");
				continue;
			}
			if(Entry.wlanDisabled)
				continue;

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

#ifdef WLAN_MESH
int setupWLanMesh(void)
{
	char ifname[16]={0};
	char parm[256]={0};
	unsigned char tmpbuf[128]= {0};
	int status=0;
	unsigned char mesh_enable=0, encrypt=0, wpaAuth=0;
	MIB_CE_MBSSIB_T Entry;
	mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry);

	strncpy(ifname, (char*)getWlanIfName(), 16);

	//status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "meshSilence=0");

	if(Entry.wlanMode == AP_MESH_MODE || Entry.wlanMode == MESH_MODE){
		mib_get_s(MIB_WLAN_MESH_ENABLE, (void *) &mesh_enable, sizeof(mesh_enable));
		snprintf(parm, sizeof(parm), "%s=%hhu", "mesh_enable", mesh_enable);
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	}
	else
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "mesh_enable=0");

	if(Entry.wlanMode == AP_MESH_MODE)
	{
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "mesh_ap_enable=1");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "mesh_portal_enable=0");
	}
	else if(Entry.wlanMode == MESH_MODE)
	{
		//if(mesh_enable)
		//	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "meshSilence=1");
		
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "mesh_ap_enable=0");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "mesh_portal_enable=0");
	}
	else{
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "mesh_ap_enable=0");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "mesh_portal_enable=0");
	}

	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "mesh_root_enable=0");
	
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "mesh_max_neightbor=16");

	mib_get_s(MIB_WLAN_MESH_ID, (void *) tmpbuf, sizeof(tmpbuf));
	snprintf(parm, sizeof(parm), "%s=%s", "mesh_id", tmpbuf);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	mib_get_s(MIB_WLAN_MESH_ENCRYPT, (void *) &encrypt, sizeof(encrypt));
	mib_get_s(MIB_WLAN_MESH_WPA_AUTH, (void *) &wpaAuth, sizeof(wpaAuth));

	if(encrypt && wpaAuth == WPA_AUTH_PSK){
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "mesh_privacy=4");
		mib_get_s(MIB_WLAN_MESH_WPA_PSK, (void *) tmpbuf, sizeof(tmpbuf));
		snprintf(parm, sizeof(parm), "%s=%s", "mesh_passphrase", tmpbuf);
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	}
	else{
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "mesh_privacy=0");
	}

	return status;
}
#ifdef WLAN_MESH_ACL_ENABLE
// return value:
// 0  : successful
// -1 : failed
int set_wlan_mesh_acl(char *ifname)
{
	unsigned char value[32];
	char parm[128];
	int num, i;
	MIB_CE_WLAN_AC_T Entry;
	int status=0;

	// aclnum=0
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "meshaclnum=0");

	// aclmode
	mib_get_s(MIB_WLAN_MESH_ACL_ENABLED, (void *)value, sizeof(value));
	snprintf(parm, sizeof(parm), "meshaclmode=%u", value[0]);
	status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

	if (value[0] == 0) // ACL disabled
		return status;

	if ((num = mib_chain_total(MIB_WLAN_MESH_ACL_TBL)) == 0)
		return status;

	for (i=0; i<num; i++) {
		if (!mib_chain_get(MIB_WLAN_MESH_ACL_TBL, i, (void *)&Entry))
			return;
		if(Entry.wlanIdx != wlan_idx)
			continue;

		// acladdr
		snprintf(parm, sizeof(parm), "meshacladdr=%.2x%.2x%.2x%.2x%.2x%.2x",
			Entry.macAddr[0], Entry.macAddr[1], Entry.macAddr[2],
			Entry.macAddr[3], Entry.macAddr[4], Entry.macAddr[5]);
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
	}
	return status;
}
#endif

int setupWLanMeshDaemon(void)
{
	int i=0, status=0;
	unsigned char mesh_enabled=0, mode=0, enabled=0;
#ifdef WLAN_DUALBAND_CONCURRENT
	int orig_wlan_idx = wlan_idx;
#endif
	MIB_CE_MBSSIB_T Entry;
	
	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		wlan_getEntry(&Entry, 0);
		mode = Entry.wlanMode;
		mib_get_s(MIB_WLAN_MESH_ENABLE,(void *)&mesh_enabled, sizeof(mesh_enabled));
	    if(mode != AP_MESH_MODE && mode != MESH_MODE) {
	        mesh_enabled = 0;
	    }
		enabled |= mesh_enabled;

    #if !defined(WLAN_MESH_SINGLE_IFACE)
        if (mesh_enabled) {
            char msh_name[16];
            sprintf(msh_name, "%s-msh", WLANIF[i]);
            status|=va_cmd(BRCTL, 3, 1, "addif", (char *)BRIF, msh_name);
            status|=va_cmd(IFCONFIG, 2, 1, msh_name, "up");
            //pathsel -i wlanX-msh -P -d
            status|=va_niced_cmd(MESH_DAEMON_PROG, 4, 0, "-i", msh_name, "-P", "-d");
        }
    #endif
	}
#ifdef WLAN_DUALBAND_CONCURRENT
	wlan_idx = orig_wlan_idx;
#endif


    #if 0// defined(CONFIG_RTL_MESH_CROSSBAND) || !defined(WLAN_MESH_SINGLE_IFACE)
    if(wlan0_mesh_enabled) {
        system("pathsel -i wlan0-msh0 -P -d");
    }

    if(wlan1_mesh_enabled) {
        system("pathsel -i wlan1-msh0 -P -d");
    }
    #elif defined(WLAN_MESH_SINGLE_IFACE)
	if(enabled){
	#if defined(TRIBAND_SUPPORT)
        system("ifconfig wlan-msh down");
        system("echo 1 > /proc/fc/ctrl/flow_flush");
	#endif /* defined(TRIBAND_SUPPORT) */

		status|=va_cmd(BRCTL, 3, 1, "addif", (char *)BRIF, "wlan-msh");
		status|=va_cmd(IFCONFIG, 2, 1, "wlan-msh", "up");
		//pathsel -i wlan-msh -P -d
        status|=va_niced_cmd(MESH_DAEMON_PROG, 4, 0, "-i", "wlan-msh", "-P", "-d");
	}
    #endif

	return 0;
}
#endif
#ifdef WLAN_BAND_STEERING
int rtk_wlan_get_band_steering_status(void)
{
#ifndef WLAN_VAP_BAND_STEERING
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
#else
	int i = 0, j = 0, check_wpa_cipher = 0;
	int orig_wlan_idx = wlan_idx;
	MIB_CE_MBSSIB_T wlan_entry[NUM_WLAN_INTERFACE][WLAN_MBSSID_NUM+1];

	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		wlan_getEntry(&wlan_entry[i][0], 0);
		for(j=1; j<WLAN_MBSSID_NUM+1; j++)
			wlan_getEntry(&wlan_entry[i][j], j);
	}
	wlan_idx = orig_wlan_idx;

	//root ssid has checked when i=0, so do not have to check again when i=1
	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		if(wlan_entry[i][0].wlanDisabled)
			continue;
		for(j=i; j<=WLAN_MBSSID_NUM; j++)
		{
			if(wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].wlanDisabled)
				continue;
			if(!strcmp(wlan_entry[i][0].ssid, wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].ssid))
			{
				if(wlan_entry[i][0].encrypt == wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].encrypt)
				{
					if(wlan_entry[i][0].encrypt == WIFI_SEC_NONE)
					{
						if(wlan_entry[i][0].enable1X == wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].enable1X)
							return 1;
					}
					else if(wlan_entry[i][0].encrypt == WIFI_SEC_WEP)
					{
						if(wlan_entry[i][0].enable1X == wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].enable1X)
						{
							if(wlan_entry[i][0].enable1X == 0)
							{
								if((wlan_entry[i][0].wep == wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].wep) 
								&& (wlan_entry[i][0].authType == wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].authType))
								{
									if(wlan_entry[i][0].wep == WEP64)
									{
										if(!strncmp(wlan_entry[i][0].wep64Key1, wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].wep64Key1, WEP64_KEY_LEN))
											return 1;
									}
									else if(wlan_entry[i][0].wep == WEP128)
									{
										if(!strncmp(wlan_entry[i][0].wep128Key1, wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].wep128Key1, WEP128_KEY_LEN))
											return 1;
									}
								}
							}
							else
							{	
								if(wlan_entry[i][0].wep == wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].wep)
									return 1;
							}
						}
					}
					else if(wlan_entry[i][0].encrypt >= WIFI_SEC_WPA)
					{
						switch(wlan_entry[i][0].encrypt)
						{
							case WIFI_SEC_WPA:
								if(wlan_entry[i][0].unicastCipher == wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].unicastCipher)
									check_wpa_cipher = 1;
								break;
							case WIFI_SEC_WPA2:
								if((wlan_entry[i][0].wpa2UnicastCipher == wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].wpa2UnicastCipher)
#ifdef WLAN_11W
								&& (wlan_entry[i][0].dotIEEE80211W == wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].dotIEEE80211W)
#endif
								)
								{
#ifdef WLAN_11W
									//1: MGMT_FRAME_PROTECTION_OPTIONAL
									if((wlan_entry[i][0].dotIEEE80211W == 1) && (wlan_entry[i][0].sha256 == wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].sha256))
										check_wpa_cipher = 1;
									else if(wlan_entry[i][0].dotIEEE80211W != 1)
#endif
										check_wpa_cipher = 1;
								}
								break;
							case WIFI_SEC_WPA2_MIXED:
								if((wlan_entry[i][0].unicastCipher == wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].unicastCipher) &&
									(wlan_entry[i][0].wpa2UnicastCipher == wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].wpa2UnicastCipher))
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
						if(wlan_entry[i][0].wpaAuth == WPA_AUTH_PSK)
						{
							if(check_wpa_cipher && (!strncmp(wlan_entry[i][0].wpaPSK, wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].wpaPSK, MAX_PSK_LEN+1)))
								return 1;
						}
						else
							if(check_wpa_cipher)
								return 1;

					}
				}
			}
		}
	}
	return 0;
#endif
}


#ifdef WLAN_VAP_BAND_STEERING
int rtk_wlan_vap_get_band_steering_status(int vapIndex)
{
	int i = 0, j = 0, check_wpa_cipher = 0;
	int orig_wlan_idx = wlan_idx;
	MIB_CE_MBSSIB_T wlan_entry[NUM_WLAN_INTERFACE][WLAN_MBSSID_NUM+1];

	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		wlan_getEntry(&wlan_entry[i][0], 0);
		for(j=1; j<WLAN_MBSSID_NUM+1; j++)
			wlan_getEntry(&wlan_entry[i][j], j);
	}
	wlan_idx = orig_wlan_idx;

	if(wlan_entry[orig_wlan_idx][vapIndex].wlanDisabled)      //vap disable
		return 0;
	else
	{
		for(i=0; i<=WLAN_MBSSID_NUM; i++){
			if(wlan_entry[1-orig_wlan_idx][i].wlanDisabled)
				continue;
			if(!strcmp(wlan_entry[orig_wlan_idx][vapIndex].ssid, wlan_entry[1-orig_wlan_idx][i].ssid))
			{
				if(wlan_entry[orig_wlan_idx][vapIndex].encrypt == wlan_entry[1-orig_wlan_idx][i].encrypt)
				{
					if(wlan_entry[orig_wlan_idx][vapIndex].encrypt == WIFI_SEC_NONE)
					{
						if(wlan_entry[orig_wlan_idx][vapIndex].enable1X == wlan_entry[1-orig_wlan_idx][i].enable1X)
								return 1;
					}
					else if(wlan_entry[orig_wlan_idx][vapIndex].encrypt == WIFI_SEC_WEP)
					{
						if(wlan_entry[orig_wlan_idx][vapIndex].enable1X == wlan_entry[1-orig_wlan_idx][i].enable1X)
						{
							if(wlan_entry[orig_wlan_idx][vapIndex].enable1X == 0)
							{
								if((wlan_entry[orig_wlan_idx][vapIndex].wep == wlan_entry[1-orig_wlan_idx][i].wep) 
								&& (wlan_entry[orig_wlan_idx][vapIndex].authType == wlan_entry[1-orig_wlan_idx][i].authType))
								{
									if(wlan_entry[orig_wlan_idx][vapIndex].wep == WEP64)
									{
										if(!strncmp(wlan_entry[orig_wlan_idx][vapIndex].wep64Key1, wlan_entry[1-orig_wlan_idx][i].wep64Key1, WEP64_KEY_LEN))
											return 1;
									}
									else if(wlan_entry[orig_wlan_idx][vapIndex].wep == WEP128)
									{
										if(!strncmp(wlan_entry[orig_wlan_idx][vapIndex].wep128Key1, wlan_entry[1-orig_wlan_idx][i].wep128Key1, WEP128_KEY_LEN))
											return 1;
									}
								}
							}
							else
							{	
								if(wlan_entry[orig_wlan_idx][vapIndex].wep == wlan_entry[1-orig_wlan_idx][i].wep)
									return 1;
							}
						}
					}
					else if(wlan_entry[orig_wlan_idx][vapIndex].encrypt >= WIFI_SEC_WPA)
					{
						switch(wlan_entry[orig_wlan_idx][vapIndex].encrypt)
						{
							case WIFI_SEC_WPA:
								if(wlan_entry[orig_wlan_idx][vapIndex].unicastCipher == wlan_entry[1-orig_wlan_idx][i].unicastCipher)
									check_wpa_cipher = 1;
								break;
							case WIFI_SEC_WPA2:
								if((wlan_entry[orig_wlan_idx][vapIndex].wpa2UnicastCipher == wlan_entry[1-orig_wlan_idx][i].wpa2UnicastCipher)
#ifdef WLAN_11W
								&& (wlan_entry[orig_wlan_idx][vapIndex].dotIEEE80211W == wlan_entry[1-orig_wlan_idx][i].dotIEEE80211W)
#endif
								)
								{
#ifdef WLAN_11W
									//1: MGMT_FRAME_PROTECTION_OPTIONAL
									if((wlan_entry[orig_wlan_idx][vapIndex].dotIEEE80211W == 1) && (wlan_entry[orig_wlan_idx][vapIndex].sha256 == wlan_entry[1-orig_wlan_idx][i].sha256))
										check_wpa_cipher = 1;
									else if(wlan_entry[orig_wlan_idx][vapIndex].dotIEEE80211W != 1)
#endif
										check_wpa_cipher = 1;
								}
								break;
							case WIFI_SEC_WPA2_MIXED:
								if((wlan_entry[orig_wlan_idx][vapIndex].unicastCipher == wlan_entry[1-orig_wlan_idx][i].unicastCipher) &&
									(wlan_entry[orig_wlan_idx][vapIndex].wpa2UnicastCipher == wlan_entry[1-orig_wlan_idx][i].wpa2UnicastCipher))
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
						if(wlan_entry[orig_wlan_idx][vapIndex].wpaAuth == WPA_AUTH_PSK)
						{
							if(check_wpa_cipher && (!strncmp(wlan_entry[orig_wlan_idx][vapIndex].wpaPSK, wlan_entry[1-orig_wlan_idx][i].wpaPSK, MAX_PSK_LEN+1)))
								return 1;
						}
						else
							if(check_wpa_cipher)
								return 1;

					}
				}
			}
		}
		return 0;
	}
	
}

void rtk_wlan_vap_set_band_steering(void)
{
#ifdef WLAN_MBSSID
	int i = 0, j = 0, orig_wlan_idx = wlan_idx;
	char ifname[16]={0};
	MIB_CE_MBSSIB_T wlan_entry[NUM_WLAN_INTERFACE][WLAN_MBSSID_NUM+1];
#endif
	char parm[64]={0};

	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		wlan_getEntry(&wlan_entry[i][0], 0);
		for(j=1; j<WLAN_MBSSID_NUM+1; j++)
			wlan_getEntry(&wlan_entry[i][j], j);
	}
	wlan_idx = orig_wlan_idx;

	for(i=1; i<=WLAN_MBSSID_NUM; i++)      //check start from wlan0-vap1
	{
		if(wlan_entry[0][i].wlanDisabled || (!wlan_entry[0][i].wlanBandSteering))
			continue;
		for(j=1; j<=WLAN_MBSSID_NUM; j++)
		{
			if(wlan_entry[NUM_WLAN_INTERFACE-1][j].wlanDisabled || (!wlan_entry[NUM_WLAN_INTERFACE-1][j].wlanBandSteering))
				continue;
			if(!strcmp(wlan_entry[0][i].ssid, wlan_entry[NUM_WLAN_INTERFACE-1][j].ssid))
			{
				printf("i %d j %d\n",i , j);
#ifdef WLAN0_5G_SUPPORT
				snprintf(ifname, sizeof(ifname), "wlan0-vap%d", i - 1);
				va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "stactrl_prefer_band=1");
				va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "stactrl_enable=1");
				snprintf(ifname, sizeof(ifname), "wlan1-vap%d", j - 1);
				va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "stactrl_prefer_band=0");
				va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "stactrl_enable=1");
#else
				snprintf(ifname, sizeof(ifname), "wlan1-vap%d", j - 1);
				va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "stactrl_prefer_band=1");
				va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "stactrl_enable=1");
				snprintf(ifname, sizeof(ifname), "wlan0-vap%d", i - 1);
				va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "stactrl_prefer_band=0");
				va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "stactrl_enable=1");
#endif
			}
		}
	}
}
#endif
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
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
void rtk_wlan_set_band_steering(void)
{
	unsigned char vuChar = 0;
	unsigned int intVal = 0;
	char parm[64]={0};
	char ifname[IFNAMSIZ]={0};
	unsigned char phyband = 0;
	rtk_wlan_get_ifname(wlan_idx, 0, ifname);
	if(mib_get_s( MIB_WIFI_STA_CONTROL, (void *)&vuChar, sizeof(vuChar)) && vuChar)
	{
		snprintf(parm, sizeof(parm), "stactrl_enable=%d", (vuChar)?1:0);
		va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

		if(mib_get_s(MIB_WLAN_BANDST_ENABLE, &vuChar, sizeof(vuChar)) && !vuChar)
		{
			vuChar = 1;
			mib_set(MIB_WLAN_BANDST_ENABLE, &vuChar);
		}

		if(mib_get_s(MIB_WLAN_BANDST_RSSTHRDHIGH, &intVal, sizeof(intVal)))
		{
			intVal = intVal+100;
			snprintf(parm, sizeof(parm), "stactrl_rssi_th_high=%d", intVal);
			va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}

		if(mib_get_s(MIB_WLAN_BANDST_RSSTHRDLOW, &intVal, sizeof(intVal)))
		{
			intVal = intVal+100;
			snprintf(parm, sizeof(parm), "stactrl_rssi_th_low=%d", intVal);
			va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}

		if(mib_get_s(MIB_WLAN_BANDST_CHANNELUSETHRD, &intVal, sizeof(intVal)))
		{
			intVal = ((intVal*255)/100);
			snprintf(parm, sizeof(parm), "stactrl_ch_util_th=%d", intVal);
			va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}

		if(mib_get_s(MIB_WLAN_BANDST_STEERINGINTERVAL, &intVal, sizeof(intVal)))
		{
			snprintf(parm, sizeof(parm), "stactrl_steer_detect=%d", intVal);
			va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}

		if(mib_get_s(MIB_WLAN_BANDST_STALOADTHRESHOLD2G, &intVal, sizeof(intVal)))
		{
			snprintf(parm, sizeof(parm), "stactrl_sta_load_th_2g=%d", intVal);
			va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
		}

		mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));
		snprintf(parm, sizeof(parm), "stactrl_prefer_band=%d", (phyband == PHYBAND_5G)?1:0);
		va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);

#ifdef WLAN_USE_VAP_AS_SSID1
		snprintf(parm, sizeof(parm), "stactrl_groupID=2");
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
#endif
	}
	else{
		va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "stactrl_enable=0");
	}
}
#else
void rtk_wlan_set_band_steering(void)
{
	unsigned char sta_control = 0, phyband = 0, stactrl_prefer_band = 0;
	char parm[64]={0};
	char ifname[IFNAMSIZ]={0};
#ifdef WLAN_VAP_BAND_STEERING
	int i = 0, j = 0, orig_wlan_idx = wlan_idx;
	char vapIfName[IFNAMSIZ]={0};
	MIB_CE_MBSSIB_T wlan_entry[NUM_WLAN_INTERFACE][WLAN_MBSSID_NUM+1];

	for(i=NUM_WLAN_INTERFACE-1; i>=0; i--)
	{
		for(j=1; j<=WLAN_MBSSID_NUM; j++)
		{
			snprintf(vapIfName, sizeof(vapIfName), "wlan%d-vap%d", i, j - WLAN_VAP_ITF_INDEX);
			va_cmd(IWPRIV, 3, 1, vapIfName, "set_mib", "stactrl_prefer_band=0");
			va_cmd(IWPRIV, 3, 1, vapIfName, "set_mib", "stactrl_enable=0");
		}
	}
#endif


	rtk_wlan_get_ifname(wlan_idx, 0, ifname); //SSID1

	mib_get_s(MIB_WIFI_STA_CONTROL, &sta_control, sizeof(sta_control));
	snprintf(parm, sizeof(parm), "stactrl_enable=%d", (sta_control & STA_CONTROL_ENABLE)?1:0);
	va_cmd(IWPRIV, 3, 1, (char *)ifname, "set_mib", parm);

	if(sta_control & STA_CONTROL_ENABLE){
		mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));
		stactrl_prefer_band = (sta_control & STA_CONTROL_PREFER_BAND)? PHYBAND_2G:PHYBAND_5G;
		if(stactrl_prefer_band == phyband)
			snprintf(parm, sizeof(parm), "stactrl_prefer_band=1");
		else
			snprintf(parm, sizeof(parm), "stactrl_prefer_band=0");
#ifdef WLAN_USE_VAP_AS_SSID1
		snprintf(parm, sizeof(parm), "stactrl_groupID=2");
		va_cmd(IWPRIV, 3, 1, (char *)ifname, "set_mib", parm);
#endif

	}
	else
		snprintf(parm, sizeof(parm), "stactrl_prefer_band=0");
	va_cmd(IWPRIV, 3, 1, (char *)ifname, "set_mib", parm);

#ifdef WLAN_VAP_BAND_STEERING
	if(sta_control & STA_CONTROL_ENABLE)
	{
		for(i=0; i<NUM_WLAN_INTERFACE; i++)
		{
			wlan_idx = i;
			wlan_getEntry(&wlan_entry[i][0], 0);
			for(j=1; j<WLAN_MBSSID_NUM+1; j++)
				wlan_getEntry(&wlan_entry[i][j], j);
		}
		wlan_idx = orig_wlan_idx;

		for(i=NUM_WLAN_INTERFACE-1; i>=0; i--)
		{
			if(wlan_entry[i][0].wlanDisabled)
				continue;
			for(j=1; j<=WLAN_MBSSID_NUM; j++)
			{
				if(wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].wlanDisabled || (!wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].wlanBandSteering))
					continue;
				if(!strcmp(wlan_entry[i][0].ssid, wlan_entry[NUM_WLAN_INTERFACE -1 - i][j].ssid))
				{
					snprintf(vapIfName, sizeof(vapIfName), "wlan0-vap%d", j - WLAN_VAP_ITF_INDEX);
					va_cmd(IWPRIV, 3, 1, vapIfName, "set_mib", "stactrl_prefer_band=1");
					va_cmd(IWPRIV, 3, 1, vapIfName, "set_mib", "stactrl_enable=1");
					snprintf(vapIfName, sizeof(vapIfName), "wlan1-vap%d", j - WLAN_VAP_ITF_INDEX);
					va_cmd(IWPRIV, 3, 1, vapIfName, "set_mib", "stactrl_prefer_band=0");
					va_cmd(IWPRIV, 3, 1, vapIfName, "set_mib", "stactrl_enable=1");
				}
			}
		}
	}
#endif

}
#endif
#endif

void rtk_wlan_wifi_button_on_off_action(void)
{
#if defined( WLAN_SUPPORT) && defined(CONFIG_WLAN_ON_OFF_BUTTON)
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
			mib_local_mapping_get(MIB_WLAN_MODULE_DISABLED, 0, (void *)&vChar);
			vChar = vChar? 0:1;
			mib_local_mapping_set(MIB_WLAN_MODULE_DISABLED, 0, (void *)&vChar);
#ifdef WLAN_DUALBAND_CONCURRENT
			mib_local_mapping_set(MIB_WLAN_MODULE_DISABLED, 1, (void *)&vChar);
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
#ifdef CONFIG_WIFI_SIMPLE_CONFIG // WPS
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

void rtk_wlan_wps_button_action(int wlan_index)
{
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
	char cmd[128]={0};
	char wscd_pid_name[64]={0};
	int pid = 0;
	if(wlan_index == -1){
		getWscPidName(wscd_pid_name);
		if(wscd_pid_name[0]){
			pid = read_pid(wscd_pid_name);
			if(pid>0){
				kill(pid, SIGUSR2);
				//printf("pid %d signal %d\n", pid, SIGUSR2);
			}
		}
	}
	else if(wlan_index>=0 && wlan_index<NUM_WLAN_INTERFACE){
#if defined(WLAN_DUALBAND_CONCURRENT) && !defined(WLAN_WPS_VAP)
		snprintf(cmd, sizeof(cmd), "echo 1 > /var/wps_start_interface%d", wlan_index);
		system(cmd);
#endif
		va_cmd(WSC_DAEMON_PROG, 2 , 1 , "-sig_pbc" , WLANIF[wlan_index]);
	}
#endif
}

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
		snprintf(cmd_str, sizeof(cmd_str), "echo %d > /proc/%s/led", led_value, WLANIF[i]);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_str);		
#endif
	}
#if defined(CONFIG_WIFI_LED_SHARED)
#if 0
	led_value = (led_mode == 2) ? ((wlSt) ? 2 : 0) : led_mode;
	snprintf(cmd_str, sizeof(cmd_str), "echo %d > /proc/%s/led", led_value, WLANIF[0]);
	va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#else
	snprintf(cmd_str, sizeof(cmd_str), "echo %d > /proc/%s/led", led_mode, WLANIF[0]);
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

#endif

#ifdef WPS_QUERY
static int getIP(char *ip, char *mac)
{
	FILE *fp;
	char strbuf[256];
	char *ptr;
	int ret = -1;

	fp = fopen("/proc/net/arp", "r");
	if (fp == NULL){
		printf("read arp file fail!\n");
		return ret;
	}
	fgets(strbuf, sizeof(strbuf), fp);
	while (fgets(strbuf, sizeof(strbuf), fp)) {
		ptr=strstr(strbuf, mac);
		if(ptr!=NULL){
			sscanf(strbuf, "%s %*s %*s %*s %*s %*s", ip);
			printf("ip %s\n", ip);
			ret = 0;
			goto end;
		}
	}
end:
	fclose(fp);
	return ret;
}
int check_wps_dev_info(char *devinfo)
{
	FILE *fp;
	//FILE *fp2;
	char wps_dev_mac[20]={0};
	int lSize;
	char wps_dev_info[256] = {0};
	char ipaddr[20] = {0};
	int ret = -1;

	fp = fopen("/tmp/wps_dev_info", "r");

	if(fp){
		fscanf(fp, "%*[^:]:%[^;]", wps_dev_mac);
		ret = getIP(ipaddr, wps_dev_mac);
		// read wps dev info
		fseek (fp , 0 , SEEK_END);
		lSize = ftell (fp);
		rewind (fp);
		fread (wps_dev_info,1,lSize,fp);
		#if 0
		fp2 = fopen("/tmp/wps_dev_qry", "w");
		if(fp2){
			fprintf(fp2,"IP:%s;%s", ipaddr, wps_dev_info);
			fclose(fp2);
		}
		#endif
		sprintf(devinfo, "IP:%s;%s", ipaddr, wps_dev_info);
		fclose(fp);
	}
	return ret;
}

void run_wps_query(char *wps_status, char *devinfo)
{

	FILE *fp;
	int status;
	fp = fopen("/tmp/wscd_status", "r");
	if(fp){
		fscanf(fp, "%d", &status);
		fclose(fp);
		if(status == PROTOCOL_TIMEOUT)
			*wps_status = 3;
		else if(status == PROTOCOL_SUCCESS)
			*wps_status = 2;
		else if(status >= WSC_EAP_FAIL && status <= PROTOCOL_PIN_NUM_ERR || status == PROTOCOL_PBC_OVERLAPPING)
			*wps_status = 1;
#ifdef COM_CUC_IGD1_WPS
		else if(status == NOT_USED)
			*wps_status = 4;//not start
#endif
		else
			*wps_status = 0;

		if(*wps_status == 2){
			if(check_wps_dev_info(devinfo)!=0){
				*wps_status = 0;
				devinfo = NULL;
			}
		}
		else
			devinfo = NULL;
	}
	else{
		*wps_status = 1; //failed
	}
}
#endif

#ifdef WLAN_WPS_VAP
void sync_wps_config_parameter_to_flash(char *filename, char *wlan_interface_name)
{
	FILE *fp;
	int vwlan_idx;
	char tmpbuf[120];
	char line[400], token[40], value[300], *ptr;
	MIB_CE_MBSSIB_T Entry;
	unsigned char vChar;
	if( wlan_interface_name [5] == '-' && wlan_interface_name [6] == 'v' && wlan_interface_name [7] == 'a') {
		vwlan_idx = wlan_interface_name[9] - '0';
		vwlan_idx ++;
#ifdef WLAN_DUALBAND_CONCURRENT
		wlan_idx = wlan_interface_name[4]- '0';
#endif
	}
	else{
		vwlan_idx = 0;
#ifdef WLAN_DUALBAND_CONCURRENT
		wlan_idx = wlan_interface_name[4]- '0';
#endif
	}
	fp = fopen(filename, "r");
	if(fp){
		wlan_getEntry(&Entry, vwlan_idx);
		while (fgets(line, 200, fp)) {
			if (line[0] == '#')
				continue;
			ptr = (char *)get_token(line, token);
			if (ptr == NULL)
				continue;
			if (get_value(ptr, value)==0){
				continue;
			}
			else if (!strcmp(token, "WSC_CONFIGURED")) {
				Entry.wsc_configured = atoi(value);
			}
			else if (!strcmp(token, "SSID")) {
				strcpy(Entry.ssid, value);
			}
			else if (!strcmp(token, "AUTH_TYPE")) {
				Entry.authType = atoi(value);
			}
			else if (!strcmp(token, "WLAN_ENCRYPT")) {
				Entry.encrypt = atoi(value);
			}
			else if (!strcmp(token, "WSC_AUTH")) {
				Entry.wsc_auth = atoi(value);
			}
			else if (!strcmp(token, "WLAN_WPA_AUTH")) {
				Entry.wpaAuth = atoi(value);
			}
			else if (!strcmp(token, "WLAN_WPA_PSK")) {
				strcpy(Entry.wpaPSK, value);
			}
			else if (!strcmp(token, "WLAN_PSK_FORMAT")) {
				Entry.wpaPSKFormat = atoi(value);
			}
			else if (!strcmp(token, "WSC_PSK")) {
				strcpy(Entry.wscPsk, value);
			}
			else if (!strcmp(token, "WPA_CIPHER_SUITE")) {
				Entry.unicastCipher = atoi(value);
			}
			else if (!strcmp(token, "WPA2_CIPHER_SUITE")) {
				Entry.wpa2UnicastCipher = atoi(value);
			}
			else if (!strcmp(token, "WEP")) {
				Entry.wep = atoi(value);
			}
			else if (!strcmp(token, "WEP64_KEY1")) {
				strcpy(Entry.wep64Key1, value);
			}
			else if (!strcmp(token, "WEP128_KEY1")) {
				strcpy(Entry.wep128Key1, value);
			}
			else if (!strcmp(token, "WEP_DEFAULT_KEY")) {
				Entry.wepDefaultKey = atoi(value);
			}
			else if (!strcmp(token, "WEP_KEY_TYPE")) {
				Entry.wepKeyType = atoi(value);
			}
			else if (!strcmp(token, "WSC_ENC")) {
				Entry.wsc_enc = atoi(value);
			}
			else if (!strcmp(token, "WSC_CONFIGBYEXTREG")) {
				if(vwlan_idx == 0){
					vChar = atoi(value);
					mib_set(MIB_WSC_CONFIG_BY_EXT_REG, &vChar);
				}
			}
			
		}
		wlan_setEntry(&Entry, vwlan_idx);
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
		fclose(fp);
	}
}
void set_wps_enable(unsigned char enable)
{
	unsigned char vChar;
	mib_get_s(MIB_WPS_ENABLE, &vChar, sizeof(vChar));
	if(vChar != enable){
		vChar = enable;
		mib_set(MIB_WPS_ENABLE, &vChar);
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
		config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
	}
}
void set_wps_ssid(unsigned char ssid_number)
{
	unsigned char vChar;
	mib_get_s(MIB_WPS_SSID, &vChar, sizeof(vChar));
	if(vChar != ssid_number){
		vChar = ssid_number;
		mib_set(MIB_WPS_SSID, &vChar);
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
		config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
	}
}
void set_wps_timeout(unsigned int wps_timeout)
{
	unsigned int intVal;
	mib_get_s(MIB_WPS_TIMEOUT, &intVal, sizeof(intVal));
	if(intVal != wps_timeout){
		intVal = wps_timeout;
		mib_set(MIB_WPS_TIMEOUT, &intVal);
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
		config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
	}
}
int check_is_wps_ssid(int vwlan_idx, unsigned char ssid_num)
{
#ifdef WLAN_DUALBAND_CONCURRENT
	if(ssid_num>WLAN_SSID_NUM)
	{
		if(wlan_idx==1){
			if(vwlan_idx == (ssid_num-(WLAN_SSID_NUM+1)))
				return 1;
		}
	}
	else{
		if(wlan_idx==0){
			if(vwlan_idx == (ssid_num-1))
				return 1;
		}
	}		
#else
	if(vwlan_idx == (ssid_num-1))
		return 1;
#endif
	return 0;
}
static int check_wps_enc(MIB_CE_MBSSIB_Tp Entry)
{
#ifdef WPS20
	unsigned char wpsUseVersion;
#ifdef WPS_VERSION_CONFIGURABLE
	if (mib_get_s(MIB_WSC_VERSION, (void *)&wpsUseVersion, sizeof(wpsUseVersion)) == 0)
#endif
		wpsUseVersion = WPS_VERSION_V2;

	if(wpsUseVersion != 0){
		if(Entry->encrypt == WIFI_SEC_WEP || Entry->encrypt == WIFI_SEC_WPA)
			return 0;
		if(Entry->encrypt == WIFI_SEC_WPA2 && Entry->wpa2UnicastCipher == WPA_CIPHER_TKIP)
			return 0;
		if(Entry->encrypt == WIFI_SEC_WPA2_MIXED && Entry->unicastCipher == WPA_CIPHER_TKIP
			&& Entry->wpa2UnicastCipher == WPA_CIPHER_TKIP)
			return 0;
		if(Entry->hidessid)
			return 0;
#ifdef WLAN_1x
		if(Entry->enable1X || (Entry->encrypt >= WIFI_SEC_WPA && Entry->wpaAuth == WPA_AUTH_AUTO))
			return 0;
#endif
#ifdef WLAN_WPA3_NOT_SUPPORT_WPS
		if(Entry->encrypt == WIFI_SEC_WPA3)
			return 0;
#endif
	}
#endif
	return 1;
}
int restartWPS(int ssid_idx)
{
	unsigned char vChar;
	int iwcontrolpid=0, wscdpid=0;
	int i,j;
	char s_ifname[16];
	char wscd_pid_name[32];
	char wscd_fifo_name[32];
	char wscd_conf_name[32];

	int wsc_pid_fd=-1;
	unsigned char encrypt;
	unsigned char wsc_disable;
	unsigned char wlan_mode;
	unsigned char wlan_nettype;
	unsigned char wpa_auth;
	unsigned char wps_ssid;
	MIB_CE_MBSSIB_T Entry;
	char ifname[16];
	int status = 0;
	int lockfd;
	int retry = 0;
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
	unsigned char no_wlan;
#endif
#ifdef WLAN_WPS_MULTI_DAEMON
	char wscd_button_procname[32];
#endif
#ifdef WLAN_WPS_MULTI_DAEMON
	int wlanIdx=-1, vwlan_idx=-1;
	int config_all=0;

#ifdef WLAN_DUALBAND_CONCURRENT
	if(ssid_idx>WLAN_SSID_NUM && ssid_idx<=WLAN_SSID_NUM*2){
		wlanIdx = 1;
		vwlan_idx = ssid_idx-(WLAN_SSID_NUM+1);
		config_all=0;
	}
	else 
#endif
	if(ssid_idx>0 && ssid_idx<=WLAN_SSID_NUM){
		wlanIdx = 0;
		vwlan_idx = ssid_idx-1;
		config_all=0;
	}
	else if(ssid_idx == 0){
		config_all = 1;
	}
	else
		return -1;
#endif
	printf("%s config_all %d  wlan_idx %d  vwlan_idx %d\n", __func__, config_all, wlanIdx, vwlan_idx);

	LOCK_WLAN();
	
	// Kill iwcontrol
#if defined(CONFIG_USER_MONITORD) && (defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME))
	update_monitor_list_file("iwcontrol", 0);
#endif
	iwcontrolpid = read_pid((char*)IWCONTROLPID);
	if(iwcontrolpid > 0){
		kill(iwcontrolpid, 9);
		unlink(IWCONTROLPID);
	}

#ifdef WLAN_WPS_VAP
	for(j=0;j<NUM_WLAN_INTERFACE;j++){
			for ( i=0; i<WLAN_SSID_NUM; i++) {
#ifdef WLAN_WPS_MULTI_DAEMON
				if(config_all==0 && (j!=wlanIdx || i!=vwlan_idx))
					continue;
#endif

				rtk_wlan_get_ifname(j, i, ifname);
				snprintf(wscd_pid_name, 32, "/var/run/wscd-%s.pid", (char *)s_ifname);
				snprintf(wscd_conf_name, 32, "/var/wscd-%s.conf", (char *)s_ifname);
				snprintf(wscd_fifo_name, 32, "/var/wscd-%s.fifo", (char *)s_ifname);

				wscdpid = read_pid(wscd_pid_name);
				if(wscdpid > 0) {
					system("/bin/echo 0 > /proc/gpio");
					kill(wscdpid, 9);
					unlink(wscd_conf_name);
					unlink(wscd_pid_name);
					unlink(wscd_fifo_name);
					printf("%s: kill wscd for ifname %s", __func__, s_ifname);
					if(i != 0 && i != WLAN_REPEATER_ITF_INDEX)
						printf("%d\n", i-WLAN_VAP_ITF_INDEX);
					else
						printf("\n");
					while(kill(wscdpid, 0)==0){
						//printf("wait wscd to be killed\n");
						usleep(300000);
					}
					startSSDP();
				}			
			}
	}
#endif

	wlan_num = 0; /*reset to 0,jiunming*/
	useAuth_RootIf = 0;  /*reset to 0 */

#ifndef WLAN_WPS_MULTI_DAEMON
	mib_get_s(MIB_WPS_ENABLE, &vChar, sizeof(vChar));
	if(vChar == 0)
		goto no_wsc;
#endif

	for(j=0;j<NUM_WLAN_INTERFACE;j++){
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
		mib_local_mapping_get(MIB_WLAN_MODULE_DISABLED, j, (void *)&no_wlan);
		if(no_wlan)
			continue;
#endif
		for ( i=0; i<WLAN_SSID_NUM; i++) { 

			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, j, i, (void *)&Entry);

			rtk_wlan_get_ifname(j, i, ifname);
						
			wsc_disable = check_wps_enc(&Entry)? 0:1;

			mib_local_mapping_get(MIB_WLAN_MODE, j, (void *)&wlan_mode);
			mib_local_mapping_get(MIB_WLAN_NETWORK_TYPE, j, (void *)&wlan_nettype);
			//mib_get_s(MIB_WLAN_WPA_AUTH, (void *)&wpa_auth, sizeof(wpa_auth));
			//mib_get_s(MIB_WLAN_ENCRYPT, (void *)&encrypt, sizeof(encrypt));
#ifndef WLAN_WPS_MULTI_DAEMON
			mib_get_s(MIB_WPS_SSID, &wps_ssid, sizeof(wps_ssid));
#endif

			if(Entry.wlanDisabled || wsc_disable)
				continue;
#ifdef WLAN_WPS_MULTI_DAEMON
			else if(Entry.wsc_disabled)
				continue;
#else
			else if(!check_is_wps_ssid(i, wps_ssid))
				continue;
#endif
			else if(wlan_mode == CLIENT_MODE) {
				if(wlan_nettype != INFRASTRUCTURE)
					continue;
			}
			else if(wlan_mode == AP_MODE) {
				if((Entry.encrypt >= WIFI_SEC_WPA) && (Entry.wpaAuth == WPA_AUTH_AUTO))
					continue;
			}
			
			snprintf(wscd_pid_name, 32, "/var/run/wscd-%s.pid", ifname);

			int wscd_pid = read_pid(wscd_pid_name);
			if(wscd_pid > 0 && kill(wscd_pid, 0)==0)
				goto skip_running_wscd;
			
			sprintf(wscd_conf_name, "/var/wscd-%s.conf", ifname);
			sprintf(wscd_fifo_name, "/var/wscd-%s.fifo", ifname);
			//status|=generateWpaConf(para_auth_conf, 0, &Entry);
			status|=WPS_updateWscConf("/etc/wscd.conf", wscd_conf_name, 0, &Entry, i, j);

			
#ifdef WLAN_WPS_MULTI_DAEMON
			memset(wscd_button_procname, '\0', sizeof(wscd_button_procname));

			if(j==0 && i==0) //only trigger push button with wlan0
				snprintf(wscd_button_procname, sizeof(wscd_button_procname), "/proc/gpio");

			if(wscd_button_procname[0] == '\0')
				status|=va_niced_cmd(WSC_DAEMON_PROG, 9, 0, "-start", "-c", wscd_conf_name, "-w", ifname, "-fi", wscd_fifo_name, "-proc", "-");
			else
				status|=va_niced_cmd(WSC_DAEMON_PROG, 9, 0, "-start", "-c", wscd_conf_name, "-w", ifname, "-fi", wscd_fifo_name, "-proc", wscd_button_procname);
#else
			status|=va_niced_cmd(WSC_DAEMON_PROG, 7, 0, "-start", "-c", wscd_conf_name, "-w", ifname, "-fi", wscd_fifo_name);
#endif

			// fix the depency problem
			// check fifo
			retry = 10;
			while (--retry && ((wsc_pid_fd = open(wscd_fifo_name, O_WRONLY)) == -1))
			{
				usleep(30000);
			}
			retry = 10;

			while(--retry && (read_pid(wscd_pid_name) < 0))
			{
				//printf("WSCD is not running. Please wait!\n");
				usleep(300000);
			}

			if(wsc_pid_fd!=-1) close(wsc_pid_fd);/*jiunming, close the opened fd*/

skip_running_wscd:

			if (i == WLAN_ROOT_ITF_INDEX){ // Root
				/* 2010-10-27 krammer :  use bit map to record what wlan root interface is use for auth*/
				useAuth_RootIf |= (1<<j);//bit 0 ==> wlan0, bit 1 ==>wlan1
			}
			else {
				strcpy(para_iwctrl[wlan_num], ifname);
				wlan_num++;
			}

		}
	}
no_wsc:
	startSSDP();
	check_iwcontrol_8021x();
	start_iwcontrol();
	UNLOCK_WLAN();
	return status;

}
#endif

#ifdef WLAN_1x
static int check_para_iwctrl(int wlan_num, char *ifname)
{
	int i = 0;
	while(i<wlan_num){
		if(!strcmp(para_iwctrl[i], ifname))
			return 1;
		i++;
	}
	return 0;
}

#endif

static int check_iwcontrol_8021x(void)
{
#ifdef WLAN_1x
	int i,j;
	char ifname[IFNAMSIZ];
	MIB_CE_MBSSIB_T Entry;
	unsigned char vChar;

	for(j=0;j<NUM_WLAN_INTERFACE;j++){
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
		if(mib_local_mapping_get(MIB_WLAN_MODULE_DISABLED, j, (void *)&vChar)==1){
			if(vChar == 1) //disabled, do nothing
				continue;
		}
#endif
		for ( i=0; i<=NUM_VWLAN_INTERFACE; i++) {
			
			if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, j, i, (void *)&Entry)==0){
				printf("Error! Get MIB_MBSSIB_TBL error.\n");
				continue;
			}

			if(Entry.wlanDisabled)
				continue;

			if(!(Entry.enable1X || (Entry.encrypt >= WIFI_SEC_WPA && WPA_AUTH_AUTO == Entry.wpaAuth))) //not 8021x enable
				continue;

			rtk_wlan_get_ifname(j, i, ifname);
		
			if (i == 0){ // Root
				/* 2010-10-27 krammer :  use bit map to record what wlan root interface is use for auth*/

				useAuth_RootIf |= (1<<j);//bit 0 ==> wlan0, bit 1 ==>wlan1
			}
			else {
				if(check_para_iwctrl(wlan_num, ifname)==0){
					strcpy(para_iwctrl[wlan_num], ifname);
					wlan_num++;
				}
			}	
		}
	}
#endif
	return 0;
}

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
void restart_iwcontrol(void)
{
	wlan_num = 0;
	useAuth_RootIf = 0;
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
	start_WPS(0);
#endif
	check_iwcontrol_8021x();
	start_iwcontrol();
}
#endif

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
			else if (!strcmp(token, "beacon_er")) {
				nds->beacon_error = strtoul(value, NULL, 10);
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

int getWlChannel( char *interface, unsigned int *num)
{
    int skfd;
    struct iwreq wrq;
	unsigned char buffer[32]={0};

    skfd = socket(AF_INET, SOCK_DGRAM, 0);

    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
    {
        close( skfd );
      /* If no wireless name : no wireless extensions */
        return -1;
    }
	strcpy(buffer,"channel");
    wrq.u.data.pointer = (caddr_t)buffer;
    wrq.u.data.length = 10;

    if (iw_get_ext(skfd, interface, RTL8192CD_IOCTL_GET_MIB, &wrq) < 0)
    {
        close( skfd );
		return -1;
    }
	*num = (unsigned int)buffer[wrq.u.data.length - 1];
    close( skfd );

    return 0;
}

int getWlMaxStaNum( char *interface, unsigned int *num)
{
    int skfd;
    struct iwreq wrq;
	unsigned char buffer[32]={0};

    skfd = socket(AF_INET, SOCK_DGRAM, 0);

    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
    {
        close( skfd );
      /* If no wireless name : no wireless extensions */
        return -1;
    }
	strcpy(buffer,"stanum");
    wrq.u.data.pointer = (caddr_t)buffer;
    wrq.u.data.length = 32;

    if (iw_get_ext(skfd, interface, RTL8192CD_IOCTL_GET_MIB, &wrq) < 0)
    {
        close( skfd );
		return -1;
    }
	
	*num = *(unsigned int*)&buffer[wrq.u.data.length - 4];
	
    close( skfd );

    return 0;
}

#ifdef CONFIG_E8B
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
	int skfd;
	struct iwreq wrq;
	int query_cnt = 0;
	
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	do {            //wait for selecting channel	
		strcpy(wrq.ifr_name, ifname);
		strcpy(buffer,"opmode");
		wrq.u.data.pointer = (caddr_t)buffer;
		wrq.u.data.length = sizeof(buffer);
		if(ioctl(skfd, RTL8192CD_IOCTL_GET_MIB, &wrq) == 0){
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
unsigned char get_wlan_phyband(void)
{
	unsigned char phyband=PHYBAND_2G;
#ifdef WLAN_DUALBAND_CONCURRENT
	mib_get_s(MIB_WLAN_PHY_BAND_SELECT, &phyband, sizeof(phyband));
#endif
	return phyband;
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

#if defined(WLAN_CLIENT)
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
#ifdef WPS_QUERY
int is_wps_running()
{

	FILE *fp;
	int status;
	fp = fopen("/tmp/wscd_status", "r");
	if(fp){
		fscanf(fp, "%d", &status);
		fclose(fp);
		if(status == PROTOCOL_TIMEOUT)
			return 0;
		else if(status == PROTOCOL_SUCCESS)
			return 0;
		else if(status >= WSC_EAP_FAIL && status <= PROTOCOL_PIN_NUM_ERR || status == PROTOCOL_PBC_OVERLAPPING)
			return 0;
		else
			return 1;//is running	
	}
	else{
		return 0;
	}
}
#endif

double rtk_wlan_get_sta_current_tx_rate(WLAN_STA_INFO_Tp pInfo)
{
	char txrate_str[20]={"0"};
	double txrate = 0.0;
	rtk_wlan_get_sta_rate(pInfo, WLAN_STA_TX_RATE, txrate_str, 20);
	txrate = atof(txrate_str);
	return txrate; //Mbps
}
#endif //CONFIG_E8B

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#if defined(CONFIG_USER_RTK_FASTPASSNF_MODULE)
#define PROC_FASTPASSNF_IGNORE_SAMBA_IF "/proc/fastPassNF/IGNORE_SAMBA_IF"
#endif

void rtk_wlan_monitor_restart_daemon(void)
{
	restart_iwcontrol();
}
int IpPortStrToSockaddr(char *str, ipPortRange *ipport)
{
	char *c, *tmp_port = NULL;
	char buf[65], *v2 = NULL;
	int port;
	if(str == NULL || ipport == NULL) return 0;

	memset(ipport, 0, sizeof(ipPortRange));
	ipport->sin_family = AF_INET;
	ipport->eth_protocol = 0x0800;
	if((c = strchr(str, ':'))){
		v2 = NULL;
		tmp_port = c+1;
		strncpy(buf, str, c-str);
		buf[c-str] = '\0';
		if((c = strchr(buf, '-'))){
			*c = '\0';
			v2 = c+1;
		}
		if(inet_aton(buf, (struct in_addr *)ipport->start_addr) == 0)
			return 0;
		if(v2){
			if(inet_aton(v2, (struct in_addr *)ipport->end_addr) == 0)
				return 0;
		}else
			memcpy(ipport->end_addr, ipport->start_addr, sizeof(ipport->end_addr));
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
				sprintf(chain_forward, "WLACL_%s_FORWARD", ifname);
				va_cmd(IPTABLES, 8, 1, "-D", "WLACL_FORWARD", "-m", "physdev", "--physdev-in", ifname, "-j", chain_forward);
				va_cmd(IPTABLES, 2, 1, "-F", chain_forward);
				va_cmd(IPTABLES, 2, 1, "-X", chain_forward);
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
	va_cmd(EBTABLES, 6, 1, "-D", "WLACL_INPUT", "-i", ifname, "-j", chain_input);
	//sprintf(cmd, "--logical-in %s -j %s", ifname, chain_forward);
	va_cmd(EBTABLES, 6, 1, "-D", "WLACL_FORWARD", "-i", ifname, "-j", chain_forward);

	va_cmd(IPTABLES, 2, 1, "-F", chain_input);
	va_cmd(IPTABLES, 2, 1, "-X", chain_input);
	va_cmd(IPTABLES, 2, 1, "-F", chain_forward);
	va_cmd(IPTABLES, 2, 1, "-X", chain_forward);
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
					}
					s = s_tmp;
				}
				free(rule);
			}

			va_cmd(EBTABLES, 4, 1, "-A", chain_input, "-j", "DROP");
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

int setup_wlan_MAC_ACL(void)
{
	char parm[64], ifname[IFNAMSIZ];
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
				rtk_wlan_get_ifname(i, j, ifname);

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
				if(wlEntry.macAccessMode == 1) //Black List
#elif defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
				if(macFilterEnable == 1) //Black List
#endif
				{
					va_cmd(IWPRIV, 2, 1, ifname, "clear_acl_table");
					//printf("====> iwpriv %s clear_acl_table\n", ifname);
					snprintf(parm, sizeof(parm), "aclmode=2");
					va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
					//printf("====> iwpriv %s set_mib %s\n", ifname, parm);
					
					tmpList = pblackList;
					while(tmpList && tmpList->tail == 0)
					{
						snprintf(parm, sizeof(parm), "%.2x%.2x%.2x%.2x%.2x%.2x",
							tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
							tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
						va_cmd(IWPRIV, 3, 1, ifname, "add_acl_table", parm);
						//printf("====> iwpriv %s add_acl_table %s\n", ifname, parm);
						tmpList = tmpList->next;
					}
					
					tmpList = phostBlockList;
					while(tmpList && tmpList->tail == 0)
					{
						if(!findMacFilterEntry(tmpList->mac, pblackList))
						{
							snprintf(parm, sizeof(parm), "%.2x%.2x%.2x%.2x%.2x%.2x",
								tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
								tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
							va_cmd(IWPRIV, 3, 1, ifname, "add_acl_table", parm);
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
					va_cmd(IWPRIV, 2, 1, ifname, "clear_acl_table");
					//printf("====> iwpriv %s clear_acl_table\n", ifname);
					snprintf(parm, sizeof(parm), "aclmode=1");
					va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
					//printf("====> iwpriv %s set_mib %s\n", ifname, parm);
					
					tmpList = pwhiteList;
					while(tmpList && tmpList->tail == 0)
					{
						if(!findMacFilterEntry(tmpList->mac, phostBlockList))
						{
							snprintf(parm, sizeof(parm), "%.2x%.2x%.2x%.2x%.2x%.2x",
								tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
								tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
							va_cmd(IWPRIV, 3, 1, ifname, "add_acl_table", parm);
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
										snprintf(parm, sizeof(parm), "%02x%02x%02x%02x%02x%02x",
											pInfo->addr[0], pInfo->addr[1], pInfo->addr[2],
											pInfo->addr[3], pInfo->addr[4], pInfo->addr[5]);
										va_cmd(IWPRIV, 3, 1, ifname, "del_sta", parm);
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
					va_cmd(IWPRIV, 2, 1, ifname, "clear_acl_table");
					//printf("====> iwpriv %s clear_acl_table\n", ifname);
					snprintf(parm, sizeof(parm), "aclmode=%d", (phostBlockList->tail) ? 0 : 2);
					va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
					//printf("====> iwpriv %s set_mib %s\n", ifname, parm);
					
					tmpList = phostBlockList;
					while(tmpList && tmpList->tail == 0)
					{
						snprintf(parm, sizeof(parm), "%.2x%.2x%.2x%.2x%.2x%.2x",
							tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
							tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
						va_cmd(IWPRIV, 3, 1, ifname, "add_acl_table", parm);
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

				sprintf(file_block, "/tmp/%s_deny_sta_%02x%02x%02x%02x%02x%02x", ifname, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				//sprintf(line, "cat /proc/%s/mib_staconfig | grep %02x%02x%02x%02x%02x%02x > %s",
				//				ifname, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], file_block);
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
	1 => not compelete,
	2 => compelete
***/
int get_wlan_channel_scan_status(int idx)
{
	char buf[256] = {0};
	FILE *fp = NULL;
	int ret = 0;
	if(idx >= 0 && idx < (sizeof(WLANIF)/sizeof(char*))){
		//printf("%s: %s, idx = %d\n", __FUNCTION__, WLANIF[idx], idx);
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
int start_wlan_channel_scan(int idx)
{
	int ret = 0, skfd = -1;
	struct iwreq wrq;
	
	if(idx >= 0 && idx < (sizeof(WLANIF)/sizeof(char*)))
	{
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
	
	if(chInfo == NULL || count == NULL)
	{
		printf("%s: Error output argument\n", __FUNCTION__);
	}
	else if(idx >= 0 && idx < (sizeof(WLANIF)/sizeof(char*)))
	{
		//printf("%s: %s, idx = %d\n", __FUNCTION__, WLANIF[idx], idx);
		if(get_wlan_channel_scan_status(idx) == 2)
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
	
	if(idx >= 0 && idx < (sizeof(WLANIF)/sizeof(char*)))
	{
		if(idx == 0) mib_get_s(MIB_WLAN_AUTO_CHAN_ENABLED, &vChar, sizeof(vChar));
		else mib_get_s(MIB_WLAN1_AUTO_CHAN_ENABLED, &vChar, sizeof(vChar));
		
		if(vChar){
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
    int sock;
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
    close(sock);
    return ret;
}

int getWlStartAPRSSIQueryResult(char *ifname, unsigned char *macaddr,
        unsigned char* measure_result, int *bss_num,
        dot11k_beacon_measurement_report *beacon_report)
{
    int sock;
    struct iwreq wrq;
    int ret = -1;
    int err;
	unsigned char buf[1+1+sizeof(dot11k_beacon_measurement_report)*MAX_BEACON_REPORT]={0};

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
out:
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

    if (iw_get_ext(skfd, interface, SIOCGIROAMINGBSSTRANSREQ, &wrq) < 0) {
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
#if defined(WLAN_ROAMING) || defined(WLAN_STA_RSSI_DIAG)
int getWlSTARSSIScanInterval( char *interface, unsigned int *num)
{
    int skfd;
    struct iwreq wrq;
	unsigned char buffer[32]={0};

    skfd = socket(AF_INET, SOCK_DGRAM, 0);

    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
    {
    	close( skfd );
      /* If no wireless name : no wireless extensions */
        return -1;
    }
	strcpy(buffer,"scan_interval");
    wrq.u.data.pointer = (caddr_t)buffer;
    wrq.u.data.length = strlen("scan_interval");

    if (iw_get_ext(skfd, interface, RTL8192CD_IOCTL_GET_MIB, &wrq) < 0)
    {
    	close( skfd );
		return -1;
    }
	
	*num = *(unsigned int*)buffer;
    close( skfd );

    return 0;
}

int doWlSTARSSIQueryRequest(char *interface, char *mac, unsigned int channel)
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

int getWlSTARSSIDETAILQueryResult(char *interface, void *sta_mac_rssi_info, unsigned char type)
{
	int skfd;
	struct iwreq wrq;
	unsigned char buf[sizeof(sta_mac_rssi_detail)*MAX_UNASSOC_STA];

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

int getWlSTARSSIQueryResult(char *interface, void *sta_mac_rssi_info)
{
    int skfd;
	//sta_mac_rssi sta_query_info[MAX_PROBE_REQ_STA];
    struct iwreq wrq;

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
	unsigned int scan_interval;

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

int rtk_wlan_set_bcnvsie( char *interface, char *data, int data_len )
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

	wrq.u.data.pointer = (caddr_t)data;
	wrq.u.data.length = (unsigned short)data_len;

	if (iw_get_ext(skfd, interface, SIOCGISETBCNVSIE, &wrq) < 0) {
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

    skfd = socket(AF_INET, SOCK_DGRAM, 0);

    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
      close( skfd );
      /* If no wireless name : no wireless extensions */
      return -1;
    }

    wrq.u.data.pointer = (caddr_t)data;
    wrq.u.data.length = (unsigned short)data_len;

    if (iw_get_ext(skfd, interface, SIOCGISETPRBVSIE, &wrq) < 0) {
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

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	wrq.u.data.pointer = (caddr_t)data;
	wrq.u.data.length = (unsigned short)data_len;

	if (iw_get_ext(skfd, interface, SIOCGISETPRBRSPVSIE, &wrq) < 0) {
		close( skfd );
		return -1;
	}

	close( skfd );
	return (int) data[0];
}
#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
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
			if(!memcmp(pInfo->addr, mac_addr, 6)){
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
			if(!memcmp(pInfo->addr, mac_addr, 6)){
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

#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
typedef struct {
	int mib_id;
	int size;
	int do_down_up;
} _WL_CHK_RADIO_TBL_;
_WL_CHK_RADIO_TBL_ wlChkRadioTbl[] = {
	{MIB_WLAN_DISABLED,				_SIZE(wlanDisabled), 			1},
	{MIB_WLAN_MODULE_DISABLED,		_SIZE(wlanModuleDisabled),		1},
	{MIB_WLAN_BASIC_RATE,			_SIZE(basicRates), 				1},
	{MIB_WLAN_SUPPORTED_RATE,		_SIZE(supportedRates), 			1},
	{MIB_WLAN_RTS_THRESHOLD,		_SIZE(rtsThreshold), 			0},
	{MIB_WLAN_FRAG_THRESHOLD,		_SIZE(fragThreshold), 			0},
	{MIB_WLAN_PREAMBLE_TYPE,		_SIZE(preambleType), 			0},
	{MIB_WLAN_INACTIVITY_TIME,		_SIZE(inactivityTime), 			0},
	{MIB_WLAN_DTIM_PERIOD,			_SIZE(dtimPeriod), 				1},
	{MIB_TX_POWER,					_SIZE(txPower), 				1},
#ifdef CONFIG_CU_BASEON_CMCC
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
#if defined(WLAN_AIRTIME_FAIRNESS)
	{MIB_WLAN_AIRTIME_ENABLE,		_SIZE(wlan_airtime_enable),		1},
#endif
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
						if(_wlIdxSsidDoChange & (1 << (0+idx*(NUM_VWLAN_INTERFACE+1))))
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

static int checkWlanHwValueIsCorrect(void) {
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

	if(strlen(ifname)<5 || strncmp(ifname, "wlan", strlen("wlan")))
		return (-1);

	idx = atoi(&ifname[4]);
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
#ifdef CONFIG_CT_AWIFI_JITUAN_FEATURE
int rtk_wlan_check_ssid_is_awifi(int wlanidx, int ssid_idx)
{
	unsigned char phyBandSelect = 0;
	int ret = 0;
	if(ssid_idx == 1)//wlan0-vap0 is awifi
	{
		mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlanidx, (void *)&phyBandSelect);
		if(phyBandSelect == PHYBAND_2G)
			ret = 1;
	}
	return ret;
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
    }

    close(skfd);
    return retVal;

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
		printf("[CMD] %s\n", cmd);
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
		printf("[CMD] %s\n", cmd);
		system(cmd);
	}else if(role==DE_ROLE_COLLECTOR)
	{
		//TODO
	}

	return 0;
}
#endif

/**
* @brief
*	   Get the encryption info of wlan interface
*
* @param[in]
*	   ifname (unsigned char *) WLAN interface name to operate
*
* @return
*	   pEncInfo (RTK_WLAN_ENCRYPT_INFO *) Pointer to struct of encryption info obtained
*
* @retval:
* - 	0 - RTK_SUCCESS
* -    -1 - RTK_FAILED
*/

int rtk_wlan_get_encryption(unsigned char *ifname, RTK_WLAN_ENCRYPT_INFO *pEncInfo)
{

	int ret = RTK_FAILED;
	int vwlan_idx;
	char def_key_idx = 0;
	MIB_CE_MBSSIB_T Wlan_Entry;
	
	ISP_API_EXIT_WITH_VAL_ON(ifname==NULL||pEncInfo==NULL, RTK_FAILED);
	
	memset(pEncInfo, 0x0, sizeof(*pEncInfo));	
	memset(&Wlan_Entry,0x0,sizeof(MIB_CE_MBSSIB_T));
	
	if((mib_chain_get_wlan(ifname,&Wlan_Entry,&vwlan_idx))!=RTK_SUCCESS){
		ISP_API_DEBUG("mib chain get error\n");
		return RTK_FAILED;
	}
	//encrypt type
	pEncInfo->encrypt_type=Wlan_Entry.encrypt;
	//if encryption is none, exit now!	
	ISP_API_EXIT_WITH_VAL_ON(pEncInfo->encrypt_type==ENCRYPT_DISABLED, RTK_SUCCESS);
	//authentication(1X)
	pEncInfo->rtk_api_enable_1x = Wlan_Entry.enable1X;
	if(pEncInfo->rtk_api_enable_1x==1){
		memcpy(pEncInfo->rtk_api_rs_addr,Wlan_Entry.rsIpAddr,IP_ADDR_LEN);//WU&
		pEncInfo->rtk_api_rs_port=Wlan_Entry.rsPort;
		memcpy(pEncInfo->rtk_api_rs_pass,Wlan_Entry.rsPassword,MAX_PSK_LEN+1);
	}
	switch(pEncInfo->encrypt_type)
	{
	case ENCRYPT_DISABLED:
		//do nothing else
		break;
	case ENCRYPT_WEP:
		//authentication(non-1X)
		if(pEncInfo->rtk_api_enable_1x==0)
		{
			pEncInfo->rtk_api_auth_type=Wlan_Entry.authType;
		}
		//encryption
		pEncInfo->rtk_api_wep_key_len=Wlan_Entry.wep;
		pEncInfo->rtk_api_wep_key_type=Wlan_Entry.wepKeyType;
		def_key_idx=Wlan_Entry.wepDefaultKey;
		if(pEncInfo->rtk_api_wep_key_len==WEP64){
			if(def_key_idx==0){
				memcpy(pEncInfo->rtk_api_wep_key64,Wlan_Entry.wep64Key1,WEP64_KEY_LEN);
			}
			else if(def_key_idx==1)
				memcpy(pEncInfo->rtk_api_wep_key64,Wlan_Entry.wep64Key2,WEP64_KEY_LEN);
			else if(def_key_idx==2)
				memcpy(pEncInfo->rtk_api_wep_key64,Wlan_Entry.wep64Key3,WEP64_KEY_LEN);
			else
				memcpy(pEncInfo->rtk_api_wep_key64,Wlan_Entry.wep64Key4,WEP64_KEY_LEN);
		}else if(pEncInfo->rtk_api_wep_key_len==WEP128){
			if(def_key_idx==0)
				memcpy(pEncInfo->rtk_api_wep_key128,Wlan_Entry.wep128Key1,WEP128_KEY_LEN);
			else if(def_key_idx==1)
				memcpy(pEncInfo->rtk_api_wep_key128,Wlan_Entry.wep128Key2,WEP128_KEY_LEN);
			else if(def_key_idx==2)
				memcpy(pEncInfo->rtk_api_wep_key128,Wlan_Entry.wep128Key3,WEP128_KEY_LEN);
			else
				memcpy(pEncInfo->rtk_api_wep_key128,Wlan_Entry.wep128Key4,WEP128_KEY_LEN);
		}else
			ISP_API_EXIT_WITH_VAL_ON(1, RTK_FAILED);
			
		break;

#ifdef CONFIG_RTL_WAPI_SUPPORT
	case ENCRYPT_WAPI:
		pEncInfo->rtk_api_wapi_auth = Wlan_Entry.wapiAuth;
		if(Wlan_Entry.wapiAuth == 1) //psk
		{
			pEncInfo->rtk_api_wapi_key_len = Wlan_Entry.wapiPskLen;
			pEncInfo->rtk_api_wapi_key_format = Wlan_Entry.wapiPskFormat;
			memcpy(pEncInfo->rtk_api_wapi_key,Wlan_Entry.wapiPsk,MAX_PSK_LEN+1);
		}
		else if((Wlan_Entry.wapiAuth == 0)
			memcpy(pEncInfo->rtk_api_wapi_as_ipaddr,Wlan_Entry.wapiAsIpAddr,IP_ADDR_LEN);
		else
			SP_API_EXIT_WITH_VAL_ON(1, RTK_FAILED);

		break;
#endif
#ifdef CONFIG_RTK_DEV_AP
	case ENCRYPT_WPA:
	case ENCRYPT_WPA2:
#endif
	case ENCRYPT_WPA2_MIXED:
#ifdef WLAN_WPA3
	case ENCRYPT_WPA3:
	case ENCRYPT_WPA3_MIXED:
#endif
		//authentication (non-1X)
		if(pEncInfo->rtk_api_enable_1x==0)
			pEncInfo->rtk_api_auth_type=Wlan_Entry.wpaAuth; 
		//encryption
		if(pEncInfo->encrypt_type==ENCRYPT_WPA2_MIXED
#ifdef CONFIG_RTK_DEV_AP
		||pEncInfo->encrypt_type==ENCRYPT_WPA
#endif
		)
			pEncInfo->rtk_api_wpa_cipher_suite=Wlan_Entry.unicastCipher;

		if(pEncInfo->encrypt_type==ENCRYPT_WPA2_MIXED
#ifdef CONFIG_RTK_DEV_AP
		||pEncInfo->encrypt_type==ENCRYPT_WPA2
#endif
#ifdef WLAN_WPA3
		||pEncInfo->encrypt_type==ENCRYPT_WPA3||pEncInfo->encrypt_type==ENCRYPT_WPA3_MIXED
#endif
		)
			pEncInfo->rtk_api_wpa2_cipher_suite=Wlan_Entry.wpa2UnicastCipher;
		
		pEncInfo->rtk_api_wpa_psk_format=Wlan_Entry.wpaPSKFormat;
		memcpy(pEncInfo->rtk_api_wpa_key,Wlan_Entry.wpaPSK,MAX_PSK_LEN+1);

		break;
		
	default:
		ISP_API_EXIT_WITH_VAL_ON(1, RTK_FAILED);
		break;
	}

	ret = RTK_SUCCESS;
	return ret; 
}

/**
* @brief
*	   Set the encryption info of wlan interface,
*	   Note that different encryption type requires different RTK_WLAN_ENCRYPT_INFO member value
*
* @param[in]
*	   ifname (unsigned char *) WLAN interface name to operate
*
* @return
*	   pEncInfo (RTK_WLAN_ENCRYPT_INFO) The struct of encryption info to be set
*
* @retval:
* - 	0 - RTK_SUCCESS
* -    -1 - RTK_FAILED
*/

int rtk_wlan_set_encryption(unsigned char *ifname, RTK_WLAN_ENCRYPT_INFO encInfo)
{
	int ret = RTK_FAILED;
	int vwlan_idx;
	char def_key_idx = 0;
	int wapi_len = 0;
	char tmp_buf[MAX_PSK_LEN+1] = {0};
	MIB_CE_MBSSIB_T Wlan_Entry;
	memset(&Wlan_Entry,0x0,sizeof(Wlan_Entry));
	ISP_API_EXIT_WITH_VAL_ON(ifname==NULL, RTK_FAILED);

	mib_save_wlanIdx();
	ret = set_wlan_idx_by_wlanIf(ifname,&vwlan_idx);
	ISP_API_GOTO_LABLE_ON(ret==RTK_FAILED, exit);
			
	if (!mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Wlan_Entry)) {
		ISP_API_DEBUG("mib_chain get error\n");
		ISP_API_GOTO_LABLE_ON(1, exit);
	}
	//encrypt type
	Wlan_Entry.encrypt=encInfo.encrypt_type;
	
	if(!mib_set(MIB_WLAN_ENCRYPT, (void *)&encInfo.encrypt_type))
		ISP_API_GOTO_LABLE_ON(1, exit);
	//if encryption is none, exit now!
	if(encInfo.encrypt_type==ENCRYPT_DISABLED){
		printf("encrypt type is ENCRYPT_DISABLED\n");
		mib_chain_update(MIB_MBSSIB_TBL, (void *)&Wlan_Entry, vwlan_idx);	
		mib_recov_wlanIdx();	
		return RTK_SUCCESS;
	}
	//authentication(1X)
	Wlan_Entry.enable1X=encInfo.rtk_api_enable_1x;
	if(!mib_set(MIB_WLAN_ENABLE_1X, (void *)&encInfo.rtk_api_enable_1x))
		ISP_API_GOTO_LABLE_ON(1, exit);
	
	if(encInfo.rtk_api_enable_1x==1)
	{
		memcpy(Wlan_Entry.rsIpAddr,encInfo.rtk_api_rs_addr,IP_ADDR_LEN);
		Wlan_Entry.rsPort=encInfo.rtk_api_rs_port;
		memcpy(Wlan_Entry.rsPassword,encInfo.rtk_api_rs_pass,MAX_PSK_LEN+1);
		
		if(!mib_set(MIB_WLAN_RS_IP, (void *)encInfo.rtk_api_rs_addr))
			ISP_API_GOTO_LABLE_ON(1, exit);

		if(!mib_set(MIB_WLAN_RS_PORT, (void *)&encInfo.rtk_api_rs_port))
			ISP_API_GOTO_LABLE_ON(1, exit);

		if(!mib_set(MIB_WLAN_RS_PASSWORD, (void *)&encInfo.rtk_api_rs_pass))
			ISP_API_GOTO_LABLE_ON(1, exit);
	}

	switch(encInfo.encrypt_type)
	{		
	case ENCRYPT_WEP:
		//authentication(non-1X)
		if(encInfo.rtk_api_enable_1x==0)
		{
			Wlan_Entry.authType=encInfo.rtk_api_auth_type;
			if(!mib_set(MIB_WLAN_AUTH_TYPE, (void *)&encInfo.rtk_api_auth_type))
				ISP_API_GOTO_LABLE_ON(1, exit);
		}
		
		//encryption
		Wlan_Entry.wep=encInfo.rtk_api_wep_key_len;
		Wlan_Entry.wepKeyType=encInfo.rtk_api_wep_key_type;
		Wlan_Entry.wepDefaultKey=def_key_idx;
		
		if(!mib_set(MIB_WLAN_WEP, (void *)&encInfo.rtk_api_wep_key_len))
			ISP_API_GOTO_LABLE_ON(1, exit);

		if(!mib_set(MIB_WLAN_WEP_KEY_TYPE, (void *)&encInfo.rtk_api_wep_key_type))
			ISP_API_GOTO_LABLE_ON(1, exit);

		if(!mib_set(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&def_key_idx))
			ISP_API_GOTO_LABLE_ON(1, exit);

		if(encInfo.rtk_api_wep_key_len==WEP64){
			if(def_key_idx==0){
				memset(Wlan_Entry.wep64Key1,0x0,WEP64_KEY_LEN); 	
				memcpy(Wlan_Entry.wep64Key1,encInfo.rtk_api_wep_key64,WEP64_KEY_LEN);
				if(! mib_set(MIB_WLAN_WEP64_KEY1, (void *)encInfo.rtk_api_wep_key64))
					ISP_API_GOTO_LABLE_ON(1, exit);
			}else if(def_key_idx==1){
				memset(Wlan_Entry.wep64Key2,0x0,WEP64_KEY_LEN); 	
				memcpy(Wlan_Entry.wep64Key2,encInfo.rtk_api_wep_key64,WEP64_KEY_LEN);
				if(!mib_set(MIB_WLAN_WEP64_KEY2, (void *)encInfo.rtk_api_wep_key64))	
					ISP_API_GOTO_LABLE_ON(1, exit);
			}else if(def_key_idx==2){
				memset(Wlan_Entry.wep64Key3,0x0,WEP64_KEY_LEN); 
				memcpy(Wlan_Entry.wep64Key3,encInfo.rtk_api_wep_key64,WEP64_KEY_LEN);
				if(!mib_set(MIB_WLAN_WEP64_KEY3, (void *)encInfo.rtk_api_wep_key64))
					ISP_API_GOTO_LABLE_ON(1, exit);
			}else{ 
				memset(Wlan_Entry.wep64Key4,0x0,WEP64_KEY_LEN);
				memcpy(Wlan_Entry.wep64Key4,encInfo.rtk_api_wep_key64,WEP64_KEY_LEN);
				if(!mib_set(MIB_WLAN_WEP64_KEY4, (void *)encInfo.rtk_api_wep_key64))
					ISP_API_GOTO_LABLE_ON(1, exit);
			}
		}else if(encInfo.rtk_api_wep_key_len==WEP128){
			if(def_key_idx==0){
				memset(Wlan_Entry.wep128Key1,0x0,WEP128_KEY_LEN);
				memcpy(Wlan_Entry.wep128Key1,encInfo.rtk_api_wep_key128,WEP128_KEY_LEN);
				if(!mib_set(MIB_WLAN_WEP128_KEY1, (void *)encInfo.rtk_api_wep_key128))
					ISP_API_GOTO_LABLE_ON(1, exit);
			}else if(def_key_idx==1){
				memset(Wlan_Entry.wep128Key2,0x0,WEP128_KEY_LEN);
				memcpy(Wlan_Entry.wep128Key2,encInfo.rtk_api_wep_key128,WEP128_KEY_LEN);
				if(!mib_set(MIB_WLAN_WEP128_KEY2, (void *)encInfo.rtk_api_wep_key128))
					ISP_API_GOTO_LABLE_ON(1, exit);
			}else if(def_key_idx==2){
				memset(Wlan_Entry.wep128Key3,0x0,WEP128_KEY_LEN);
				memcpy(Wlan_Entry.wep128Key3,encInfo.rtk_api_wep_key128,WEP128_KEY_LEN);
				if(!mib_set(MIB_WLAN_WEP128_KEY3, (void *)encInfo.rtk_api_wep_key128))			
					ISP_API_GOTO_LABLE_ON(1, exit);
			}else{ 
				memset(Wlan_Entry.wep128Key4,0x0,WEP128_KEY_LEN);
				memcpy(Wlan_Entry.wep128Key4,encInfo.rtk_api_wep_key128,WEP128_KEY_LEN);
				if(!mib_set(MIB_WLAN_WEP128_KEY4, (void *)encInfo.rtk_api_wep_key128))
					ISP_API_GOTO_LABLE_ON(1, exit);
				}
		}else
			ISP_API_GOTO_LABLE_ON(1, exit);

		break;

#ifdef CONFIG_RTL_WAPI_SUPPORT
	case ENCRYPT_WAPI:
	//authentication
	Wlan_Entry.wapiAuth = encInfo.rtk_api_wapi_auth;
	if(!mib_set(MIB_WLAN_WAPI_AUTH, (void *)encInfo.rtk_api_wapi_auth))
		ISP_API_GOTO_LABLE_ON(1, exit);

	if(Wlan_Entry.wapiAuth == 1) //psk
	{
		wapi_len = strlen(encInfo.rtk_api_wapi_key);
		Wlan_Entry.wapiPskFormat = encInfo.rtk_api_wapi_key_format;
		if(!mib_set(MIB_WLAN_WAPI_PSK_FORMAT, (void *)&encInfo.rtk_api_wapi_key_format))
			ISP_API_GOTO_LABLE_ON(1, exit);
		if(Wlan_Entry.wapiPskFormat == 1) //hex
		{
			if(rtk_string_to_hex(encInfo.rtk_api_wapi_key, tmp_buf, wapi_len) == 0)
				ISP_API_GOTO_LABLE_ON(1, exit);
			if((wapi_len & 1) || (wapi_len/2 >= MAX_PSK_LEN))
				ISP_API_GOTO_LABLE_ON(1, exit);
			wapi_len = wapi_len/2;
			Wlan_Entry.wapiPskLen = wapi_len;
			if(!mib_set(MIB_WLAN_WAPI_PSKLEN, &wapi_len))
				ISP_API_GOTO_LABLE_ON(1, exit);
			memcpy(Wlan_Entry.wapiPsk, tmp_buf, sizeof(Wlan_Entry.wapiPsk));
			if(!mib_set(MIB_WLAN_WAPI_PSK, (void *)tmp_buf))
				ISP_API_GOTO_LABLE_ON(1, exit);
		}
		else //passphrase
		{
			if(wapi_len==0 || wapi_len>(MAX_PSK_LEN-1))
				ISP_API_GOTO_LABLE_ON(1, exit);
			Wlan_Entry.wapiPskLen = wapi_len;
			if(!mib_set(MIB_WLAN_WAPI_PSKLEN, &wapi_len))
				ISP_API_GOTO_LABLE_ON(1, exit);
			memcpy(Wlan_Entry.wapiPsk, tmp_buf, sizeof(Wlan_Entry.wapiPsk));
			if(!mib_set(MIB_WLAN_WAPI_PSK, (void *)tmp_buf))
				ISP_API_GOTO_LABLE_ON(1, exit);
		}
	}
	else if(Wlan_Entry.wapiAuth == 0) //AS
	{
		Wlan_Entry.wapiAsIpAddr = encInfo.rtk_api_wapi_as_ipaddr;
		if(!mib_set(MIB_WLAN_WAPI_ASIPADDR, (void *)&encInfo.rtk_api_wapi_as_ipaddr))
			ISP_API_GOTO_LABLE_ON(1, exit);
	}
	else
		ISP_API_GOTO_LABLE_ON(1, exit);

	break;
#endif

#ifdef CONFIG_RTK_DEV_AP
	case ENCRYPT_WPA:
	case ENCRYPT_WPA2:
#endif
	case ENCRYPT_WPA2_MIXED:
#ifdef WLAN_WPA3
	case ENCRYPT_WPA3:
	case ENCRYPT_WPA3_MIXED:
#endif
		//authentication (non-1X)
		if(encInfo.rtk_api_enable_1x==0)
		{
			Wlan_Entry.wpaAuth=encInfo.rtk_api_auth_type;
			if(!mib_set(MIB_WLAN_WPA_AUTH, (void *)&encInfo.rtk_api_auth_type))
				ISP_API_GOTO_LABLE_ON(1, exit);
		}
		
		//encryption
		if(encInfo.encrypt_type==ENCRYPT_WPA2_MIXED
#ifdef CONFIG_RTK_DEV_AP
		||encInfo.encrypt_type==ENCRYPT_WPA
#endif
		)
		{
			Wlan_Entry.unicastCipher=encInfo.rtk_api_wpa_cipher_suite;
			if(!mib_set(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&encInfo.rtk_api_wpa_cipher_suite))
				ISP_API_GOTO_LABLE_ON(1, exit);
		}
		if(encInfo.encrypt_type==ENCRYPT_WPA2_MIXED
#ifdef CONFIG_RTK_DEV_AP
		||encInfo.encrypt_type==ENCRYPT_WPA2
#endif
#ifdef WLAN_WPA3
		||encInfo.encrypt_type==ENCRYPT_WPA3||encInfo.encrypt_type==ENCRYPT_WPA3_MIXED
#endif
		)
		{
			Wlan_Entry.wpa2UnicastCipher=encInfo.rtk_api_wpa2_cipher_suite;
			if(!mib_set(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&encInfo.rtk_api_wpa2_cipher_suite))
				ISP_API_GOTO_LABLE_ON(1, exit);
		}
		Wlan_Entry.wpaPSKFormat=encInfo.rtk_api_wpa_psk_format;
		memset(Wlan_Entry.wpaPSK,0x0,MAX_PSK_LEN+1);
		memcpy(Wlan_Entry.wpaPSK,encInfo.rtk_api_wpa_key,MAX_PSK_LEN+1);
		
		if(!mib_set(MIB_WLAN_WPA_PSK_FORMAT, (void *)&encInfo.rtk_api_wpa_psk_format))
			ISP_API_GOTO_LABLE_ON(1, exit);
		
		if(!mib_set(MIB_WLAN_WPA_PSK, (void *)encInfo.rtk_api_wpa_key))
			ISP_API_GOTO_LABLE_ON(1, exit);
		break;
	
	default:
		ISP_API_GOTO_LABLE_ON(1, exit);
		break;
	}
	mib_chain_update(MIB_MBSSIB_TBL, (void *)&Wlan_Entry, vwlan_idx);

	ret = RTK_SUCCESS;

exit:
	mib_recov_wlanIdx();

	return (ret==RTK_SUCCESS?RTK_SUCCESS:RTK_FAILED);
}

#ifdef CONFIG_00R0
static void set5GAutoChannelLow()
{
	char ifname[IFNAMSIZ]={0};
	int i;
	unsigned char auto_chan;

	mib_get(MIB_WLAN_AUTO_CHAN_ENABLED, (void *)&auto_chan);
	if(auto_chan)
		va_cmd(IWPRIV, 3, 1, "wlan1", "set_mib", "autoch_5gmask=0xffffff00");
#ifdef WLAN_MBSSID
	for (i=0; i<=WLAN_MBSSID_NUM; i++)
	{
		memset(ifname, 0, sizeof(ifname));
		sprintf(ifname, "wlan1-vap%d", i);
		if(auto_chan)
			va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "autoch_5gmask=0xffffff00");
	}
#endif
}

static void setSiteSurveyTime()
{
	va_cmd(IWPRIV, 3, 1, "wlan0", "set_mib", "ss_to=250");
#ifdef WLAN_DUALBAND_CONCURRENT
	va_cmd(IWPRIV, 3, 1, "wlan1", "set_mib", "ss_to=250");
#endif
}
#endif

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
#include <arpa/inet.h>
#include <pthread.h>

#include "debug.h"
#include "utility.h"
#include "subr_net.h"
#ifdef WLAN_INCLUDE_IWLIB
#include <iwlib.h>
#else
#include <linux/wireless.h>
#endif
#if defined(CONFIG_CTC_SDN)
#include "subr_ctcsdn.h"
#endif

#ifdef RTK_MULTI_AP
#include "subr_multiap.h"
#endif

#include "hapd_conf.h"

#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
static unsigned int _wlIdxRadioDoChange = 0;
static unsigned int _wlIdxSsidDoChange = 0;
static int checkWlanRootChange(int *p_action_type, config_wlan_ssid *p_ssid_index);
#endif

static int hostapd_cli_set(char *ifname, char *str, int set_cli);
#if defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
#define WPS_LED_INTERFACE "/tmp/wps_led_interface"
static int rtk_wlan_get_wps_led_interface(char *ifname);
static int writeWpsScript(const char *in, const char *out);
static int _set_wlan_wps_led_status(int led_mode);
#endif
static int writeHostapdGeneralScript(const char *in, const char *out);

static unsigned int getTxPowerPercent(int phyband, int mode);

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
const char WLAN_MONITOR_DAEMON[] = "hostapd";
#endif
const char IWPRIV[] = "/bin/iwpriv";
#define WLAN_LED_PROC PROC_WLAN_DIR_NAME"/%s/led_config"

#if defined(WLAN_WPS_HAPD)
const char HOSTAPD_WPS_SCRIPT[] = "/var/hostapd_wps.sh";
#endif
#if defined(WLAN_WPS_HAPD)
const char WPAS_WPS_SCRIPT[] = "/var/wpa_supplicant_wps.sh";
#endif
const char HOSTAPD_GENERAL_SCRIPT[] = "/var/hostapd_general_event.sh";

#define WLAN_COEX_2G_40M_PROC "echo %x > "PROC_WLAN_DIR_NAME"/%s/coex_2g_40m"

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

const unsigned char *DATA_RATEStr[12] = {"1", "2", "5.5", "11", "6", "9", "12", "18", "24", "36", "48", "54"};


const unsigned char *MCS_DATA_RATEStr[4][2][32] =
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
	  "60", "120", "180", "240", "360", "480", "540", "600"}},			// Short GI, 40MHz
	{{"29.3", "58.5", "87.8", "117", "175.5", "234", "263.3", "292.5",
	  "58.5", "117", "175.5", "234", "351", "468", "526.5", "585",
	  "87.8", "175.5", "263.3", "351", "526.5", "702", "702", "877.5",
	  "117", "234", "351", "468", "702", "936", "1053", "1170"},				// Long GI, 80MHz
	 {"32.5", "65", "97.5", "130", "195", "260", "292.5", "325",
	  "65", "130", "195", "260", "390", "520", "585", "650",
	  "97.5", "195", "292.5", "390", "585", "780", "780", "975",
	  "130", "260", "390", "520", "780", "1040", "1170", "1300"}},			// Short GI, 80MHz
	{{"58.5", "117", "175.5", "234", "351", "468", "526.5", "585",
	  "117", "234", "351", "468", "702", "936", "1053", "1170",
	  "175.5", "351", "526.5", "702", "1053", "1404", "1579.5", "1755",
	  "234", "468", "702", "936", "1404", "1872", "2106", "2340"},			// Long GI, 160MHz
	 {"65", "130", "195", "260", "390", "520", "585", "650",
	  "130", "260", "390", "520", "780", "1040", "1170", "1300",
	  "195", "390", "585", "780", "1170", "1560", "1755", "1950",
	  "260", "520", "780", "1040", "1560", "2080", "2340", "2600"}}			// Short GI, 160MHz
};


//changes in following table should be synced to VHT_MCS_DATA_RATE[] in 8812_vht_gen.c
const unsigned short VHT_MCS_DATA_RATE[4][2][40] =
        {       {       {13, 26, 39, 52, 78, 104, 117, 130, 156, 156,
                         26, 52, 78, 104, 156, 208, 234, 260, 312, 312,
                         39, 78, 117, 156, 234, 312, 351, 390, 468, 520,
                         52, 104, 156, 208, 312, 416, 468, 520, 624, 624},              // Long GI, 20MHz
                        {14, 29, 43, 58, 87, 116, 130, 144, 173, 173,
                        29, 58, 87, 116, 173, 231, 260, 289, 347, 347,
                        43, 87, 130, 173, 260, 347, 390, 433, 520, 578,
                        58, 116, 173, 231, 347, 462, 520, 578, 693, 693}},              // Short GI, 20MHz
                {       {27, 54, 81, 108, 162, 216, 243, 270, 324, 360,
                        54, 108, 162, 216, 324, 432, 486, 540, 648, 720,
                        81, 162, 243, 324, 486, 648, 729, 810, 972, 1080,
                        108, 216, 324, 432, 648, 864, 972, 1080, 1296, 1440},           // Long GI, 40MHz
                        {30, 60, 90, 120, 180, 240, 270, 300,360, 400,
                        60, 120, 180, 240, 360, 480, 540, 600, 720, 800,
                        90, 180, 270, 360, 540, 720, 810, 900, 1080, 1200,
                        120, 240, 360, 480, 720, 960, 1080, 1200, 1440, 1600}},         // Short GI, 40MHz
                {       {59, 117,  176, 234, 351, 468, 527, 585, 702, 780,
                        117, 234, 351, 468, 702, 936, 1053, 1170, 1404, 1560,
                        176, 351, 527, 702, 1053, 1404, 1404, 1755, 2106, 2340,
                        234, 468, 702, 936, 1404, 1872, 2106, 2340, 2808, 3120},        // Long GI, 80MHz
                        {65, 130, 195, 260, 390, 520, 585, 650, 780, 867,
                        130, 260, 390, 520, 780, 1040, 1170, 1300, 1560,1733,
                        195, 390, 585, 780, 1170, 1560, 1560, 1950, 2340, 2600,
                        260, 520, 780, 1040, 1560, 2080, 2340, 2600, 3120, 3467}},      // Short GI, 80MHz
                {       {117, 234, 351, 468, 702, 936, 1053, 1170, 1404, 1560,
                        234, 468, 702, 936, 1404, 1872, 2106, 2340, 2808, 3120,
                        351, 702, 1053, 1404, 2106, 2808, 3159, 3510, 4212, 0,
                        468, 936, 1404, 1872, 2808, 3744, 4212, 4680, 5616, 6240},      // Long GI, 160MHz
                        {130, 260, 390, 520, 780, 1040, 1170, 1300, 1560, 1733,
                        260, 520, 780, 1040, 1560, 2080, 2340, 2600, 3120, 3467,
                        390, 780, 1170, 1560, 2340, 3120, 3510, 3900, 4680, 0,
                        520, 1040, 1560, 2080, 3120, 4160, 4680, 5200, 6240, 6933}}     // Short GI, 160MHz
        };

const unsigned char *HE_MCS_DATA_RATE[4][3][48] =
        {       {       {"7.3", "14.6", "21.9", "29.3", "43.9", "58.5", "65.8", "73.1", "87.8", "97.5", "109.7" , "121.9",
                         "14.6", "29.3", "43.9", "58.5", "87.8", "117", "131.6", "146.3", "175.5", "195", "219.4" , "243.8",
                         "21.9", "43.9", "65.8", "87.8", "131.6", "175.5", "197.4", "219.4", "263.3", "292.5", "329.1" , "365.6",
                         "29.3", "58.5", "87.8", "117", "175.5", "234", "263.3", "292.5", "351", "390" , "438.8", "487.5" }, // 3200ns, 20MHz
                        {"8.1", "16.3", "24.4", "32.5", "48.8", "65", "73.1", "81.3", "97.5", "108.3", "121.9" , "135.4",
                         "16.3", "32.5", "48.8", "65", "97.5", "130", "146.3", "162.5", "195", "216.7", "243.8" , "270.8",
                         "24.4", "48.8", "73.1", "97.5", "146.3", "195", "219.4", "243.8", "292.5", "325", "365.6" , "406.3",
                         "32.5", "65", "97.5", "130", "195", "260", "292.5", "325", "390", "433.3" , "487.5", "541.7"}, // 1600ns, 20MHz
                        {"8.6", "17.2", "25.8", "34.4", "51.6", "68.8", "77.4", "86", "103.2", "114.7", "129" , "143.4",
                         "17.2", "34.4", "51.6", "68.8", "103.2", "137.6", "154.9", "172.1", "206.5", "229.4", "258.1" , "286.8",
                         "25.8", "51.6", "77.4", "103.2", "154.9", "206.5", "232.3", "258.1", "309.7", "344.1", "387.1" , "430.1",
                         "34.4", "68.8", "103.2", "137.6", "206.5", "275.3", "309.7", "344.1", "412.9", "458.8" , "516.2", "573.5"}}, // 800ns, 20MHz
                {       {"14.6", "29.3", "43.9", "58.5", "87.8", "117", "131.6", "146.3", "175.5", "195", "219.4" , "243.8",
                         "29.3", "58.5", "87.8", "117", "175.5", "234", "263.3", "292.5", "351", "390" , "438.8", "487.5",
                         "43.9", "87.8", "131.6", "175.5", "263.3", "351", "394.9", "438.8", "526.5", "585", "658.1" , "731.3",
                         "58.5", "117", "175.5", "234", "351", "468", "526.5", "585", "702", "780", "877.5", "975"}, // 3200ns, 40MHz
                        {"16.3", "32.5", "48.8", "65", "97.5", "130", "146.3", "162.5", "195", "216.7", "243.8" , "270.8",
                         "32.5", "65", "97.5", "130", "195", "260", "292.5", "325", "390", "433.3" , "487.5", "541.7",
                         "48.8", "97.5", "146.3", "195", "292.5", "390", "438.8", "487.5", "585", "650", "731.3" , "812.5",
                         "65", "130", "195", "260", "390", "520", "585", "650", "780", "866.7", "975", "1083.3"}, // 1600ns, 40MHz
                        {"17.2", "34.4", "51.6", "68.8", "103.2", "137.6", "154.9", "172.1", "206.5", "229.4", "258.1" , "286.8",
                         "34.4", "68.8", "103.2", "137.6", "206.5", "275.3", "309.7", "344.1", "412.9", "458.8" , "516.2", "573.5",
                         "51.6", "103.2", "154.9", "206.5", "309.7", "412.9", "464.6", "516.2", "619.4", "688.2", "774.3" , "860.3",
                         "68.8", "137.6", "206.5", "275.3", "412.9", "550.6", "619.4", "688.2", "825.9", "917.6", "1032.4", "1147.1"}}, // 800ns, 40MHz
                {       {"30.6", "61.3", "91.9", "122.5", "183.8", "245", "275.6", "306.3", "367.5", "408.3", "459.4" , "510.4",
                         "61.3", "122.5", "183.8", "245", "367.5", "490", "551.3", "612.5", "735", "816.7" , "918.8", "1020.8",
                         "91.9", "183.8", "275.6", "367.5", "551.3", "735", "826.9", "918.8", "1102.5", "1225", "1378.1" , "1531.3",
                         "122.5", "245", "367.5", "490", "735", "980", "1102.5", "1225", "1470", "1633.3", "1837.5", "2041.7"}, // 3200ns, 80MHz
                        {"34", "68.1", "102.1", "136.1", "204.2", "272.2", "306.3", "340.3", "408.3", "453.7", "510.4" , "567.1",
                         "68.1", "136.1", "204.2", "272.2", "408.3", "544.4", "612.5", "680.6", "816.7", "907.4" , "1020.8", "1134.3",
                         "102.1", "204.2", "306.3", "408.3", "612.5", "816.7", "918.8", "1020.8", "1225", "1361.1", "1531.3" , "1701.4",
                         "136.1", "272.2", "408.3", "544.4", "816.7", "1088.9", "1225", "1361.1", "1633.3", "1814.8", "2041.7", "2268.5"}, // 1600ns, 80MHz
                        {"36", "72.1", "108.1", "144.1", "216.2", "288.2", "324.3", "360.3", "432.4", "480.4", "540.4" , "600.5",
                         "72.1", "144.1", "216.2", "288.2", "432.4", "576.5", "648.5", "720.6", "864.7", "960.8" , "1080.9", "1201",
                         "108.1", "216.2", "324.3", "432.4", "648.5", "864.7", "972.8", "1080.9", "1297.1", "1441.2", "1621.3" , "1801.5",
                         "144.1", "288.2", "432.4", "576.5", "864.7", "1152.9", "1297.1", "1441.2", "1729.4", "1921.6", "2161.8", "2402"}}, // 800ns, 80MHz
                {       {"61.3", "122.5", "183.8", "245", "367.5", "490", "551.3", "612.5", "735", "816.7" , "918.8", "1020.8",
                         "122.5", "245", "367.5", "490", "735", "980", "1102.5", "1225", "1470", "1633.3", "1837.5", "2041.7",
                         "183.8","367.5", "551.3", "735", "1102.5", "1470", "1653.8", "1837.5", "2205", "2450", "2756.3", "3062.5",
                         "245", "490", "735", "980", "1470", "1960", "2205", "2450", "2940", "3266.7", "3675", "4083.3"}, // 3200ns, 160MHz
                        {"68.1", "136.1", "204.2", "272.2", "408.3", "544.4", "612.5", "680.6", "816.7", "907.4" , "1020.8", "1134.3",
                         "136.1", "272.2", "408.3", "544.4", "816.7", "1088.9", "1225", "1361.1", "1633.3", "1814.8", "2041.7", "2268.5",
                         "204.2", "408.3", "612.5", "816.7", "1225", "1633.3", "1837.5", "2041.7", "2450", "2722.2", "3062.5", "3402.8",
                         "272.2", "544.4", "816.7", "1088.9", "1633.3", "2177.8", "2450", "2722.2", "3266.7", "3629.6", "4083.3", "4537"}, // 1600ns, 160MHz
                        {"72.1", "144.1", "216.2", "288.2", "432.4", "576.5", "648.5", "720.6", "864.7", "960.8" , "1080.9", "1201",
                         "144.1", "288.2", "432.4", "576.5", "864.7", "1152.9", "1297.1", "1441.2", "1729.4", "1921.6", "2161.8", "2402",
                         "216.2", "432.4", "648.5", "864.7", "1297.1", "1729.4", "1945.6", "2161.8", "2594.1", "2882.4", "3242.6", "3602.9",
                         "288.2", "576.5", "864.7", "1152.9", "1729.4", "2305.9", "2594.1", "2882.4", "3458.8", "3843.1", "4323.5", "4803.9"}}  // 800ns, 160MHz
        };


enum {IWPRIV_GETMIB=1, IWPRIV_HS=2, IWPRIV_INT=4, IWPRIV_HW2G=8, IWPRIV_TXPOWER=16, IWPRIV_HWDPK=32, IWPRIV_AX=64};
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
			/*else
				memcpy(value, va_arg(ap, char *), MAX_5G_CHANNEL_NUM);*/
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
		else if(type & IWPRIV_HWDPK){
			for(i=0; i<dpk_len; i++) {
				len = sizeof(parm)-strlen(parm);
				if(snprintf(parm+strlen(parm), len, "%02x", dpk_value[i]) >= len){
					printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
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

static int get_WlanHWMIB(int mib_wlan0_id, int mib_wlan1_id, int wlan_hwmib_index, void *value, int value_len)
{
	int mib_id = (wlan_hwmib_index == 0)? mib_wlan0_id:mib_wlan1_id;
	return mib_get_s(mib_id, (void *)value, value_len);
}
int setupWlanHWSetting(const char *wlan_interface, int wlanIdx)
{
#if 1//def RTW_MP_CALIBRATE_DATA
	unsigned char value[64];
	unsigned char phyband=PHYBAND_2G;
	int wlan_hwmib_index = 0;

#ifdef WLAN_DUALBAND_CONCURRENT
	mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlanIdx, &phyband);
	if(phyband==PHYBAND_2G)
		wlan_hwmib_index = 1;
	else
		wlan_hwmib_index = 0;
#endif

	get_WlanHWMIB(MIB_HW_XCAP_WLAN0_AX, MIB_HW_XCAP_WLAN1_AX, wlan_hwmib_index, value, sizeof(value));
	iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "xcap", value[0]);

	get_WlanHWMIB(MIB_HW_THERMAL_A_WLAN0, MIB_HW_THERMAL_A_WLAN1, wlan_hwmib_index, value, sizeof(value));
	iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "thermalA", value[0]);

	get_WlanHWMIB(MIB_HW_THERMAL_B_WLAN0, MIB_HW_THERMAL_B_WLAN1, wlan_hwmib_index, value, sizeof(value));
	iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "thermalB", value[0]);

	get_WlanHWMIB(MIB_HW_THERMAL_C_WLAN0, MIB_HW_THERMAL_C_WLAN1, wlan_hwmib_index, value, sizeof(value));
	iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "thermalC", value[0]);

	get_WlanHWMIB(MIB_HW_THERMAL_D_WLAN0, MIB_HW_THERMAL_D_WLAN1, wlan_hwmib_index, value, sizeof(value));
	iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "thermalD", value[0]);

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

	//not sure about this mib
	//mib_get_s(MIB_HW_CHANNEL_PLAN_5G, (void *)value, sizeof(value));
	//iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "chan_plan", value[0]);

#if 0
	// already set by driver config
	get_WlanHWMIB(MIB_HW_RFE_TYPE_WLAN0, MIB_HW_RFE_TYPE_WLAN1, wlan_hwmib_index, value, sizeof(value));
	iwpriv_cmd(IWPRIV_INT, wlan_interface, "flash_set", "rfe", value[0]);
#endif

	iwpriv_cmd(IWPRIV_GETMIB, wlan_interface, "flash_set", "para_path", MIB_HW_CUSTOM_PARA_PATH);

#endif /*RTW_MP_DATA*/
	return 0;
}

void rtk_wlan_get_ifname(int wlan_index, int mib_chain_idx, char *ifname)
{
	if(mib_chain_idx==WLAN_ROOT_ITF_INDEX)
		strncpy(ifname, WLANIF[wlan_index], 16);
#ifdef WLAN_MBSSID
	else if (mib_chain_idx>=WLAN_VAP_ITF_INDEX && mib_chain_idx<(WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM))
		snprintf(ifname, 16, "%s-vap%d", (char *)WLANIF[wlan_index], mib_chain_idx-WLAN_VAP_ITF_INDEX);
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
	else if (mib_chain_idx == WLAN_REPEATER_ITF_INDEX) {
		snprintf(ifname, 16, "%s-vxd", (char *)WLANIF[wlan_index]);
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

int rtk_wlan_get_mib_index_mapping_by_ifname(const char *ifname, int *wlan_index, int *vwlan_idx)
{
	int i=0;
	for(i=0; i<NUM_WLAN_INTERFACE ; i++)
	{
		if(!strncmp(ifname, WLANIF[i], strlen(WLANIF[i])))
			break;
	}
	if(i>=NUM_WLAN_INTERFACE)
		return -1;

	*wlan_index = i;

	if(NULL != strstr(ifname, "vap"))
		*vwlan_idx = (ifname[9] - '0') + WLAN_VAP_ITF_INDEX;
	else if(NULL != strstr(ifname, "vxd"))
		*vwlan_idx = WLAN_REPEATER_ITF_INDEX;
	else
		*vwlan_idx = WLAN_ROOT_ITF_INDEX;

	return 0;
}

#if defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
#ifdef WLAN_UNIVERSAL_REPEATER
static int vxd_link_status(char *ifname)
{
	int is_link=0;
	FILE *fd;
	char buf[1024]={0}, newline[1024]={0};

	snprintf(buf, sizeof(buf), "%s status -i %s", WPA_CLI, ifname);

	fd = popen(buf, "r");
	if(fd)
	{
		if(fgets(newline, 1024, fd) != NULL){
			if(strstr(newline, "bssid="))
			{
				is_link = 1;
				printf("[%s](%d)%s is link\n",__FUNCTION__,__LINE__, ifname);
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
#ifdef RTK_MULTI_AP
#if defined(WLAN_WPS_HAPD)
	if(-1 != access("/tmp/virtual_push_button", F_OK)) {
		system("echo 1 > /var/wps_gpio");
		return;
	}
#endif
#endif
#if defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
	int i=0;
	MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_UNIVERSAL_REPEATER
	char ifname[IFNAMSIZ]={0};
	unsigned char rpt_enabled=0;
	int vxd_wps=0;
	if(wlan_index==-1){
		for(i=0; i<NUM_WLAN_INTERFACE; i++){
			mib_local_mapping_get(MIB_REPEATER_ENABLED1, i, (void *)&rpt_enabled);
			rtk_wlan_get_ifname(i, WLAN_REPEATER_ITF_INDEX, ifname);
			if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_REPEATER_ITF_INDEX, &Entry) == 0){
				printf("%s %d failed\n", __func__, __LINE__);
				continue;
			}
			if(rpt_enabled && !Entry.wsc_disabled && !vxd_link_status(ifname))
			{
				va_niced_cmd(WPA_CLI, 3 , 1 , "wps_pbc", "-i", ifname);
				vxd_wps |= 1;
			}
		}
	}
	else{
		mib_local_mapping_get(MIB_REPEATER_ENABLED1, wlan_index, (void *)&rpt_enabled);
		rtk_wlan_get_ifname(wlan_index, WLAN_REPEATER_ITF_INDEX, ifname);
		if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, WLAN_REPEATER_ITF_INDEX, &Entry) == 0){
			printf("%s %d failed\n", __func__, __LINE__);
			return;
		}
		if(rpt_enabled && !Entry.wsc_disabled && !vxd_link_status(ifname))
		{
			va_niced_cmd(WPA_CLI, 3 , 1 , "wps_pbc", "-i", ifname);
			vxd_wps = 1;
		}
	}

	if(vxd_wps == 0)
#endif
	{
		if(wlan_index==-1){
			for(i=0; i<NUM_WLAN_INTERFACE; i++){
				if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_ROOT_ITF_INDEX, &Entry) == 0){
					printf("%s %d failed\n", __func__, __LINE__);
					continue;
				}

				if(Entry.wlanDisabled || Entry.wsc_disabled)
					continue;
#ifdef WLAN_CLIENT
				if(Entry.wlanMode == AP_MODE)
					va_niced_cmd(HOSTAPD_CLI, 3 , 1 , "wps_pbc", "-i", WLANIF[i]);
				else if(Entry.wlanMode == CLIENT_MODE)
					va_niced_cmd(WPA_CLI, 3 , 1 , "wps_pbc", "-i", WLANIF[i]);
#else
				va_niced_cmd(HOSTAPD_CLI, 3 , 1 , "wps_pbc", "-i", WLANIF[i]);
#endif
			}
		}
		else{
			if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, WLAN_ROOT_ITF_INDEX, &Entry) == 0){
				printf("%s %d failed\n", __func__, __LINE__);
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

#ifndef WLAN_INCLUDE_IWLIB
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
#endif

int getWlStaInfo( char *interface,  WLAN_STA_INFO_Tp pInfo )
{
#if 1
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
#endif
	int skfd;
	struct iwreq wrq;
	struct ifreq ifr;
	union iwreq_data u;
	char buffer[sizeof(sheet_hdr_2_web)+ ((MAX_STA_NUM+1)*sizeof(WLAN_STA_INFO_T))];
	WLAN_STA_INFO_Tp result = (WLAN_STA_INFO_Tp)(buffer + sizeof(sheet_hdr_2_web));
	sheet_hdr_2_web *hdr = (sheet_hdr_2_web *) buffer;
	int ret=0;
	int number = 0, total_num = 0;
	unsigned char sheet_sequence = 0, sheet_total = 0;

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
	strcpy((char*)buffer, "sta_entry_get sheet=0");

	memset(&u, 0, sizeof(union iwreq_data));
	u.data.pointer = buffer;
	u.data.length = sizeof(buffer);

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_ifrn.ifrn_name, interface, strlen(interface));
	ifr.ifr_ifru.ifru_data = &u;

	ret = iw_get_ext(skfd, interface, SIOCDEVPRIVATE, (struct iwreq *)&ifr);

	if(ret < 0){
		close( skfd );
		if(errno == EINPROGRESS){
			return 0;
		}
		else{
			return -1;
		}
	}

	close( skfd );

	sheet_total = hdr->sheet_total;
	number = (u.data.length-sizeof(sheet_hdr_2_web))/sizeof(WLAN_STA_INFO_T);
	memcpy((pInfo+1), result, number*sizeof(WLAN_STA_INFO_T));
	total_num += number;
	sheet_sequence++;
	//printf("%s %d %d %d %d %d\n",__func__, hdr->sheet_total, hdr->sheet_sequence, u.data.length, number, sizeof(WLAN_STA_INFO_T));
	//if(number>0)
	//printf("%02x%02x%02x%02x%02x\n", ((WLAN_STA_INFO_Tp)(pInfo+1))->addr[0], ((WLAN_STA_INFO_Tp)(pInfo+1))->addr[1], ((WLAN_STA_INFO_Tp)(pInfo+1))->addr[2]
	//		, ((WLAN_STA_INFO_Tp)(pInfo+1))->addr[3], ((WLAN_STA_INFO_Tp)(pInfo+1))->addr[4], ((WLAN_STA_INFO_Tp)(pInfo+1))->addr[5]);
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
		snprintf(buffer, sizeof(buffer), "sta_entry_get sheet=%d", sheet_sequence);

		memset(&u, 0, sizeof(union iwreq_data));
		u.data.pointer = buffer;
		u.data.length = sizeof(buffer);

		memset(&ifr, 0, sizeof(struct ifreq));
		strncpy(ifr.ifr_ifrn.ifrn_name, interface, strlen(interface));
		ifr.ifr_ifru.ifru_data = &u;

		ret = iw_get_ext(skfd, interface, SIOCDEVPRIVATE, (struct iwreq *)&ifr);
		if(ret < 0){
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
		sheet_total = hdr->sheet_total;
		number = (u.data.length-sizeof(sheet_hdr_2_web))/sizeof(WLAN_STA_INFO_T);
		//printf("%s %d %d %d %d\n",__func__, hdr->sheet_total, hdr->sheet_sequence, u.data.length, number);
		if((total_num+number) > MAX_STA_NUM){
			printf("%s %d: too many bss number!\n", __func__, __LINE__);
			break;
		}
		memcpy((pInfo+1) + (total_num), result, number*sizeof(WLAN_STA_INFO_T));
		total_num += number;
		sheet_sequence++;
	}

	return 0;

#else
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
#endif
}

void rtk_wlan_wifi_button_on_off_action(void)
{
#if defined( WLAN_SUPPORT) && defined(WLAN_ON_OFF_BUTTON)
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
		case 5: //75%
			return 2;
		case 6: //25%
			return 12;
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

static unsigned int getTxPowerPercent(int phyband, int mode)
{
#ifdef WLAN_TXPOWER_HIGH
	if(phyband == PHYBAND_2G)
	{
		switch (mode)
		{
			case 0: //100% or 200%
				if(getTxPowerHigh(phyband))
					return 200; //200%
				return 100; //100%
			case 1: //80%
				return 80;
			case 2: //60%
				return 60;
			case 3: //35%
				return 35;
			case 4: //15%
				return 15;
		}
	}
	else{ //5G
		switch (mode)
		{
			case 0: //100% or 140%
				if(getTxPowerHigh(phyband))
					return 140; //140%
					//return -6; //200%
				return 100; //100%
			case 1: //80%
				return 80;
			case 2: //60%
				return 60;
			case 3: //35%
				return 35;
			case 4: //15%
				return 15;
		}
	}
#else
	switch (mode)
	{
		case 0: //100%
			return 100;
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
		case 1: //80%
			return 80;
		case 2: //60%
			return 60;
		case 3: //40%
			return 40;
		case 4: //20%
			return 20;
#else
		case 1: //70%
			return 70;
		case 2: //50%
			return 50;
		case 3: //35%
			return 35;
		case 4: //15%
			return 15;
#endif
	}
#endif
	return 0;
}

int getWlVersion(const char *interface, char *verstr )
{
	int skfd, cmd_id;
	unsigned char vernum[4];
	struct iwreq wrq;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd <0){
		printf("[%s %d] : Could not create socket \n",__func__, __LINE__);
		return -1;
	}	

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
		close( skfd );
		/* If no wireless name : no wireless extensions */
		return -1;
	}

	memset(vernum, 0, sizeof(vernum));
	wrq.u.data.pointer = (caddr_t)&vernum[0];
	wrq.u.data.length = sizeof(vernum);

	wrq.u.data.flags = SIOCGIWRTLDRVVERSION;
	cmd_id = SIOCDEVPRIVATE+2;

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
#if defined(WLAN_DUALBAND_CONCURRENT)
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

int wlan_setEntry(MIB_CE_MBSSIB_T *pEntry, int index)
{
	int ret=1;

	ret=mib_chain_update(MIB_MBSSIB_TBL, (void *)pEntry, index);

	return ret;
}

static char *get_wlan_proc_token_with_sep(char *data, char *token, char *separator)
{
	char *ptr=NULL, *data_ptr=data;
	int len=0;

	/* skip leading space */
	while(*data_ptr && *data_ptr == ' '){
		data_ptr++;
	}

	ptr = data_ptr;
	while (*ptr && *ptr != '\n' ) {
		if (*ptr == *separator) {
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

static char *get_wlan_proc_token(char *data, char *token)
{
	return get_wlan_proc_token_with_sep(data, token, ":");
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
#if 1
	snprintf(wlan_path, sizeof(wlan_path), PROC_WLAN_DIR_NAME"/%s/trx_info", ifname);
#else
	snprintf(wlan_path, sizeof(wlan_path), "/proc/%s/stats", ifname);
#endif
	//reset counter
	nds->rx_bytes = 0;
	nds->rx_packets = 0;
	nds->rx_errors = 0;
	nds->rx_dropped = 0;
	nds->tx_bytes = 0;
	nds->tx_packets = 0;
	nds->tx_errors = 0;
	nds->tx_dropped = 0;
	nds->tx_ucast_pkts = 0;
	nds->tx_mcast_pkts = 0;
	nds->tx_bcast_pkts = 0;
	nds->beacon_error = 0;

	fp = fopen(wlan_path, "r");
	if (fp) {
		while (fgets(line, sizeof(line),fp)) {
			ptr = get_wlan_proc_token_with_sep(line, token, "=");
			if (ptr == NULL)
				continue;
			if (get_wlan_proc_value(ptr, value)==0){
				continue;
			}
			//fprintf(stderr, "%s::%s:%s\n", __func__,token, value);
			if (!strcmp(token, "rx_bytes")) {
				nds->rx_bytes = strtoull(value, NULL, 10);
			}
			else if (!strcmp(token, "rx_packets")) {
				nds->rx_packets = strtoull(value, NULL, 10);
			}
			else if (!strcmp(token, "rx_errors")) {
				nds->rx_errors = strtoull(value, NULL, 10);
			}
#if 1
			else if (!strcmp(token, "rx_drops")) {
#else
			else if (!strcmp(token, "rx_data_drops")) {
#endif
				nds->rx_dropped = strtoull(value, NULL, 10);
			}
			else if (!strcmp(token, "tx_bytes")) {
				nds->tx_bytes = strtoull(value, NULL, 10);
			}
			else if (!strcmp(token, "tx_packets")) {
				nds->tx_packets = strtoull(value, NULL, 10);
			}
			//todo
			else if (!strcmp(token, "tx_fails")) {
				nds->tx_errors = strtoull(value, NULL, 10);
			}
			else if (!strcmp(token, "tx_drops")) {
				nds->tx_dropped = strtoull(value, NULL, 10);
			}
#if 1
			else if (!strcmp(token, "tx_uc_packets")) {
#else
			else if (!strcmp(token, "tx_ucast_pkts")) {
#endif
				nds->tx_ucast_pkts = strtoull(value, NULL, 10);
			}
#if 1
			else if (!strcmp(token, "tx_mc_packets")) {
#else
			else if (!strcmp(token, "tx_mcast_pkts")) {
#endif
				nds->tx_mcast_pkts = strtoull(value, NULL, 10);
			}
#if 1
			else if (!strcmp(token, "tx_bc_packets")) {
#else
			else if (!strcmp(token, "tx_bcast_pkts")) {
#endif
				nds->tx_bcast_pkts = strtoull(value, NULL, 10);
			}
		}
		fclose(fp);
	}

	snprintf(wlan_path, sizeof(wlan_path), PROC_WLAN_DIR_NAME"/%s/int_logs", ifname);
	fp = fopen(wlan_path, "r");
	if (fp) {
		while (fgets(line, sizeof(line),fp)) {
			ptr = get_wlan_proc_token_with_sep(line, token, "=");
			if (ptr == NULL)
				continue;
			if (get_wlan_proc_value(ptr, value)==0){
				continue;
			}
			if (!strcmp(token, "bcn_fail")) {
				nds->beacon_error = strtoul(value, NULL, 10);
				break;
			}
		}
		fclose(fp);
	}
#if 0
	fprintf(stderr,"ifname:%s\n", ifname);
	fprintf(stderr,"rx_bytes:%llu\n", nds->rx_bytes);
	fprintf(stderr,"rx_packets:%llu\n", nds->rx_packets);
	fprintf(stderr,"rx_errors:%llu\n", nds->rx_errors);
	fprintf(stderr,"rx_dropped:%llu\n", nds->rx_dropped);
	fprintf(stderr,"tx_bytes:%llu\n", nds->tx_bytes);
	fprintf(stderr,"tx_packets:%llu\n", nds->tx_packets);
	fprintf(stderr,"tx_errors:%llu\n", nds->tx_errors);
	fprintf(stderr,"tx_dropped:%llu\n", nds->tx_dropped);
	fprintf(stderr,"tx_ucast_pkts:%llu\n", nds->tx_ucast_pkts);
	fprintf(stderr,"tx_mcast_pkts:%llu\n", nds->tx_mcast_pkts);
	fprintf(stderr,"tx_bcast_pkts:%llu\n", nds->tx_bcast_pkts);
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
#if 1
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
#endif
}

int getWlNoiseInfo(char *interface, signed int *pInfo){
	int ret = -1;
	FILE *fp = NULL;
	char wlan_path[128] = {0}, line[1024] = {0};
	char token[512]={0}, value[512]={0};
	char *ptr = NULL;
	snprintf(wlan_path, sizeof(wlan_path), PROC_WLAN_DIR_NAME"/%s/ap_info" , interface);

	fp = fopen(wlan_path, "r");
	if(fp){
		while (fgets(line, sizeof(line),fp)) {
			ptr = get_wlan_proc_token_with_sep(line, token, "=");
			if (ptr == NULL)
				continue;
			if (get_wlan_proc_value(ptr, value)==0){
				continue;
			}
			//fprintf(stderr, "%s::%s:%s\n", __func__,token, value);
			if (!strcmp(token, "noise_level")) {
				if(sscanf(value,"%d", pInfo)){
					ret = 0;
					//printf("%s noise_level %d\n", interface, *pInfo);
					break;
				}
			}
		}
		fclose(fp);
	}
	return ret;
}


int getWlInterferencePercentInfo(char *interface, unsigned long *pInfo){
	int ret = -1;
	FILE *fp = NULL;
	char wlan_path[128] = {0}, line[1024] = {0};
	char token[512]={0}, value[512]={0};
	char *ptr = NULL;
	snprintf(wlan_path, sizeof(wlan_path), PROC_WLAN_DIR_NAME"/%s/ap_info" , interface);

	fp = fopen(wlan_path, "r");
	if(fp){
		while (fgets(line, sizeof(line),fp)) {
			ptr = get_wlan_proc_token_with_sep(line, token, "=");
			if (ptr == NULL)
				continue;
			if (get_wlan_proc_value(ptr, value)==0){
				continue;
			}
			//fprintf(stderr, "%s::%s:%s\n", __func__,token, value);
			if (!strcmp(token, "channel_loading")) {
				if(sscanf(value,"%lu", pInfo)){
					ret = 0;
					//printf("%s channel_loading %lu\n", interface, *pInfo);
					break;
				}
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
	char token[512]={0}, value[512]={0};
	char *ptr = NULL;
	snprintf(wlan_path, sizeof(wlan_path), PROC_WLAN_DIR_NAME"/%s/ap_info" , interface);

	fp = fopen(wlan_path, "r");
	if(fp){
		while (fgets(line, sizeof(line),fp)) {
			ptr = get_wlan_proc_token_with_sep(line, token, "=");
			if (ptr == NULL)
				continue;
			if (get_wlan_proc_value(ptr, value)==0){
				continue;
			}
			//fprintf(stderr, "%s::%s:%s\n", __func__,token, value);
			if (!strcmp(token, "idle_free_time")) {
				if(sscanf(value,"%u", pInfo)){
					ret = 0;
					//printf("%s idle_free_time %u\n", interface, *pInfo);
					break;
				}
			}
		}
		fclose(fp);
	}
	return ret;
}

int getWlCurrentChannelWidthInfo(char *interface, unsigned int *pInfo)
{
	int ret = -1;
	FILE *fp = NULL, *fp1 = NULL;
	char wlan_path[128] = {0}, line[1024] = {0}, wlan_path1[128] = {0};
	char token[512]={0}, value[512]={0}, token1[512]={0}, value1[512]={0};
	char *ptr = NULL, *ptr1 = NULL;
	unsigned int cur_bwmode = 0;
	unsigned int use40M = 0;
	unsigned int frequencyWidth40M = 0;
	
	snprintf(wlan_path1, sizeof(wlan_path1), "/var/hostapd.conf.%s" , interface);
	snprintf(wlan_path, sizeof(wlan_path), PROC_WLAN_DIR_NAME"/%s/ap_info" , interface);
	//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
	fp1 = fopen(wlan_path1, "r");
	if (fp1) {
		while (fgets(line, sizeof(line),fp1)) {
			//fprintf(stderr, "%s:%d  line===>%s\n", __func__,__LINE__,line);
			ptr = get_wlan_proc_token_with_sep(line, token, "=");
			if (ptr == NULL)
				continue;
			
			if (get_wlan_proc_value(ptr, value)==0){
				continue;
			}
			if (!strcmp(token, "force_2g_40m")) {
				if(sscanf(value,"%u", &use40M)){
					ret = 0;
				}
			}
			
			if (!strcmp(token, "ht_capab")) {
				if (strstr(value, "[HT40-][SHORT-GI-20][SHORT-GI-40]")) {
					//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
					frequencyWidth40M = 1;
				}
			}
			
		}
		fclose(fp1);
	}

	fp = fopen(wlan_path, "r");
	if(fp){
		while (fgets(line, sizeof(line),fp)) {
			//fprintf(stderr, "%s:%d  line===>%s\n", __func__,__LINE__,line);
			if (!strstr(line, "cur_bwmode")) {
				continue;
			}
			ptr1 = get_wlan_proc_token_with_sep(line, token1, ",");
			if (ptr1 == NULL)
				continue;
			//fprintf(stderr, "%s:%d ptr1===>%s \n", __func__,__LINE__,ptr1);
			ptr = get_wlan_proc_token_with_sep(ptr1, token1, ",");
			if (ptr == NULL)
				continue;
			//fprintf(stderr, "%s:%d token1===>%s \n", __func__,__LINE__,token1);
			ptr = get_wlan_proc_token_with_sep(token1, token, "=");
			if (ptr == NULL)
				continue;
			
			if (get_wlan_proc_value(ptr, value)==0){
				continue;
			}
			//fprintf(stderr, "%s:%d token===>%s value====>%s\n", __func__,__LINE__,token, value);
			if (!strcmp(token, "cur_bwmode")) {
				if(sscanf(value,"%u", &cur_bwmode)){
					ret = 0;
					//printf("%s cur_bwmode %u\n", interface, cur_bwmode);
					break;
				}
			}
		}
		fclose(fp);
	}

	fprintf(stderr, "%s cur_bwmode %u, use40M %u, frequencyWidth40M %u\n", interface, cur_bwmode, use40M, frequencyWidth40M);
	if(cur_bwmode==0){
		*pInfo = HT20;
		if(frequencyWidth40M ==1 && use40M ==0){
			*pInfo = HT20_40;
		}
	}
	else if(cur_bwmode==1){
		*pInfo = HT40;
	}
	else if(cur_bwmode==2){
		*pInfo = HT80;
	}
	else if(cur_bwmode==3){
		*pInfo = HT160;
	}
	else if(cur_bwmode==4){
		*pInfo = HT80_80;
	}
	return ret;
}


int getWlSiteSurveyResult(char *interface, SS_STATUS_Tp pStatus )
{
#if 1
	int skfd;
	struct iwreq wrq;
	struct ifreq ifr;
	union iwreq_data u;
	char buffer[sizeof(sheet_hdr_2_web)+ (MAX_BSS_DESC*sizeof(BssDscr))];
	bss_desc_2_web *ss_result = (bss_desc_2_web *)(buffer + sizeof(sheet_hdr_2_web));
	sheet_hdr_2_web *ss_hdr = (sheet_hdr_2_web *) buffer;
	int ret=0;
	int ss_number = 0;
	unsigned char sheet_sequence = 0, sheet_total = 0;

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
	strcpy((char*)buffer, "ss_result sheet=0");

	memset(&u, 0, sizeof(union iwreq_data));
	u.data.pointer = buffer;
	u.data.length = sizeof(buffer);

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_ifrn.ifrn_name, interface, strlen(interface));
	ifr.ifr_ifru.ifru_data = &u;

	ret = iw_get_ext(skfd, interface, SIOCDEVPRIVATE, (struct iwreq *)&ifr);

	if(ret < 0){
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
		ss_number = (u.data.length-sizeof(sheet_hdr_2_web))/sizeof(BssDscr);
		memcpy(pStatus->bssdb, ss_result, ss_number*sizeof(BssDscr));
		pStatus->number += ss_number;
		sheet_sequence++;
		//printf("%s %d %d %d %d\n",__func__, ss_hdr->sheet_total, ss_hdr->sheet_sequence, u.data.length, ss_number);
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
			snprintf(buffer, sizeof(buffer), "ss_result sheet=%d", sheet_sequence);

			memset(&u, 0, sizeof(union iwreq_data));
			u.data.pointer = buffer;
			u.data.length = sizeof(buffer);

			memset(&ifr, 0, sizeof(struct ifreq));
			strncpy(ifr.ifr_ifrn.ifrn_name, interface, strlen(interface));
			ifr.ifr_ifru.ifru_data = &u;

			ret = iw_get_ext(skfd, interface, SIOCDEVPRIVATE, (struct iwreq *)&ifr);
			if(ret < 0){
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
			ss_number = (u.data.length-sizeof(sheet_hdr_2_web))/sizeof(BssDscr);
			//printf("%s %d %d %d %d\n",__func__, ss_hdr->sheet_total, ss_hdr->sheet_sequence, u.data.length, ss_number);
			if((pStatus->number+ss_number) > MAX_BSS_DESC){
				printf("%s %d: too many bss number!\n", __func__, __LINE__);
				break;
			}
			memcpy(pStatus->bssdb + (pStatus->number), ss_result, ss_number*sizeof(BssDscr));
			pStatus->number += ss_number;
			sheet_sequence++;
		}
	}

	return 0;
#else
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
#endif
}

int getWlSiteSurveyRequest(char *interface, int *pStatus)
{
#if 1
	int skfd;
	struct iwreq wrq;
	struct ifreq ifr;
	union iwreq_data u;
	char buffer[] = "ss_start";
	int ret=0;

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
		if(errno == EALREADY){
			*pStatus  = -1; //retry later
			return 0;
		}
		else{
			return -1;
		}
	}

	close( skfd );
	*pStatus  = 0;

	return 0;
#else
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
#endif
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
#if 0
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
#else
	//hard coded for 8852AE
	memset(pData, '\0', sizeof(struct _misc_data_));
	pData->mimo_tr_used = 2;
#endif
    return 0;
}


int getWlStaNum( char *interface, int *num )
{
#if 1
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
#endif
}

int getWlEnc( char *interface , char *buffer, char *num)
{
    int skfd;
    struct iwreq wrq;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd <0){
		printf("[%s %d] : Could not create socket \n",__func__, __LINE__);
		return -1;
	}

    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
    {
	close( skfd );
      /* If no wireless name : no wireless extensions */
        return -1;
    }

    wrq.u.data.pointer = (caddr_t)buffer;
    wrq.u.data.length = strlen(buffer);
	wrq.u.data.flags = RTL8192CD_IOCTL_GET_MIB;

    if (iw_get_ext(skfd, interface, SIOCDEVPRIVATE+2, &wrq) < 0)
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
	wrq.u.data.flags = RTL8192CD_IOCTL_GET_MIB;

	if (iw_get_ext(skfd, interface, SIOCDEVPRIVATE+2, &wrq) < 0)
	{
		close( skfd );
		return -1;
	}
	memcpy(num, buffer, sizeof(int));
	if(skfd != -1)
		close( skfd );

	return 0;
}

enum rtw_data_rate{
	RTW_DATA_RATE_CCK1		= 0x0,
	RTW_DATA_RATE_OFDM54	= 0xB,
	RTW_DATA_RATE_MCS0		= 0x80,
	RTW_DATA_RATE_MCS31		= 0x9F,
	RTW_DATA_RATE_VHT_NSS1_MCS0 = 0x100,
	RTW_DATA_RATE_VHT_NSS2_MCS0 = 0x110,
	RTW_DATA_RATE_VHT_NSS3_MCS0 = 0x120,
	RTW_DATA_RATE_VHT_NSS4_MCS0	= 0x130,
	RTW_DATA_RATE_VHT_NSS4_MCS9	= 0x139,
	RTW_DATA_RATE_HE_NSS1_MCS0	= 0x180,
	RTW_DATA_RATE_HE_NSS2_MCS0	= 0x190,
	RTW_DATA_RATE_HE_NSS3_MCS0	= 0x1A0,
	RTW_DATA_RATE_HE_NSS4_MCS0	= 0x1B0,
	RTW_DATA_RATE_HE_NSS4_MCS11	= 0x1BB
};

static int _get_he_rate_index(unsigned short OperaRate)
{
	if(OperaRate>=RTW_DATA_RATE_HE_NSS4_MCS0){
		return (OperaRate-RTW_DATA_RATE_HE_NSS4_MCS0+12*3);
	}
	else if(OperaRate>=RTW_DATA_RATE_HE_NSS3_MCS0){
		return (OperaRate-RTW_DATA_RATE_HE_NSS3_MCS0+12*2);
	}
	else if(OperaRate>=RTW_DATA_RATE_HE_NSS2_MCS0){
		return (OperaRate-RTW_DATA_RATE_HE_NSS2_MCS0+12*1);
	}
	else{
		return (OperaRate-RTW_DATA_RATE_HE_NSS1_MCS0);
	}
}

static int _get_vht_rate_index(unsigned short OperaRate)
{
	if(OperaRate>=RTW_DATA_RATE_VHT_NSS4_MCS0){
		return (OperaRate-RTW_DATA_RATE_VHT_NSS4_MCS0+30);
	}
	else if(OperaRate>=RTW_DATA_RATE_VHT_NSS3_MCS0){
		return (OperaRate-RTW_DATA_RATE_VHT_NSS3_MCS0+20);
	}
	else if(OperaRate>=RTW_DATA_RATE_VHT_NSS2_MCS0){
		return (OperaRate-RTW_DATA_RATE_VHT_NSS2_MCS0+10);
	}
	else{
		return (OperaRate-RTW_DATA_RATE_VHT_NSS1_MCS0);
	}
}
void rtk_wlan_get_sta_rate(WLAN_STA_INFO_Tp pInfo, int rate_type, char *rate_str, unsigned int len)
{
	unsigned short OperaRate = 0;
	unsigned char channelWidth = 0;//20M 0,40M 1,80M 2, 160M 3
	unsigned char gi_ltf = 0;

	if(rate_type == WLAN_STA_TX_RATE){
		OperaRate = pInfo->tx_op_rate;
		gi_ltf = pInfo->tx_gi_ltf;
	}
	else if(rate_type == WLAN_STA_RX_RATE){
		OperaRate = pInfo->rx_op_rate;
		gi_ltf = pInfo->rx_gi_ltf;
	}
	channelWidth = pInfo->channel_bandwidth;
	//printf("channelWidth %d gi_ltf %d\n", channelWidth, gi_ltf);

	if(OperaRate <= RTW_DATA_RATE_HE_NSS4_MCS11 && OperaRate >= RTW_DATA_RATE_HE_NSS1_MCS0){
		if(gi_ltf!=0){
			gi_ltf = (gi_ltf&1)? 2:1; //1,3,5 ==> 800ns, 2,4 ==> 1600ns, 0 ==> 3200ns
		}
		snprintf(rate_str, len, "%s", HE_MCS_DATA_RATE[channelWidth][gi_ltf][_get_he_rate_index(OperaRate)]);
	}
	else if(OperaRate <= RTW_DATA_RATE_VHT_NSS4_MCS9 && OperaRate >= RTW_DATA_RATE_VHT_NSS1_MCS0) {
		snprintf(rate_str, len, "%d", VHT_MCS_DATA_RATE[channelWidth][gi_ltf][_get_vht_rate_index(OperaRate)]>>1);
	} else if(OperaRate <= RTW_DATA_RATE_MCS31 && OperaRate >= RTW_DATA_RATE_MCS0){
		snprintf(rate_str, len, "%s", MCS_DATA_RATEStr[channelWidth][gi_ltf][OperaRate-RTW_DATA_RATE_MCS0]);
	}
	else if(OperaRate <= RTW_DATA_RATE_OFDM54){
#if 0
		if(OperaRate % 2) {
			snprintf(rate_str, len, "%d%s",	OperaRate / 2, ".5");
		}else{
			snprintf(rate_str, len, "%d", OperaRate / 2);
		}
#else
		snprintf(rate_str, len, "%s", DATA_RATEStr[OperaRate]);
#endif
	}
	else
		rate_str[0]='\0';
	//printf("rate %x gi_ltf %d\n", OperaRate, gi_ltf);
	//printf("rate_str %s\n", rate_str);
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

unsigned int rtk_wlan_get_bss_rate(unsigned char autoRate, unsigned char wlanBand, unsigned int fixedTxRate, unsigned int curBw, unsigned char sgi)
{
	unsigned short OperaRate = fixedTxRate;
	unsigned char channelWidth = 0;//20M 0,40M 1,80M 2
	unsigned char gi_ltf = sgi;
	char rate_str[20]={0};
	unsigned int rate=0;
	int rf_num = 2;

	if(curBw == 160)
		channelWidth = 3;
	else if(curBw == 80)
		channelWidth = 2;
	else if(curBw == 40)
		channelWidth = 1;
	else
		channelWidth = 0;

	if(autoRate){
		if(wlanBand & BAND_11AX){
			OperaRate = RTW_DATA_RATE_HE_NSS1_MCS0 + (0x10)*(rf_num-1) + 12-1;
		}
		else if(wlanBand & BAND_5G_11AC){
			OperaRate = RTW_DATA_RATE_VHT_NSS1_MCS0 + (0x10)*(rf_num-1) + 10-1;
		}
		else if(wlanBand & BAND_11N){
			OperaRate = RTW_DATA_RATE_MCS0 + 8*(rf_num)-1;
		}
		else if((wlanBand & BAND_11G) || (wlanBand & BAND_11A)){
			OperaRate = 11; //54M
		}
		else if(wlanBand & BAND_11B){
			OperaRate = 3; //11M
		}
	}

	if(OperaRate <= RTW_DATA_RATE_HE_NSS4_MCS11 && OperaRate >= RTW_DATA_RATE_HE_NSS1_MCS0){
		if(gi_ltf!=0){
			gi_ltf = (gi_ltf&1)? 2:1; //1,3,5 ==> 800ns, 2,4 ==> 1600ns, 0 ==> 3200ns
		}
		snprintf(rate_str, sizeof(rate_str), "%s", HE_MCS_DATA_RATE[channelWidth][gi_ltf][_get_he_rate_index(OperaRate)]);
	}
	else if(OperaRate <= RTW_DATA_RATE_VHT_NSS4_MCS9 && OperaRate >= RTW_DATA_RATE_VHT_NSS1_MCS0) {
		snprintf(rate_str, sizeof(rate_str), "%d", VHT_MCS_DATA_RATE[channelWidth][gi_ltf][_get_vht_rate_index(OperaRate)]>>1);
	}
	else if(OperaRate <= RTW_DATA_RATE_MCS31 && OperaRate >= RTW_DATA_RATE_MCS0){
		snprintf(rate_str, sizeof(rate_str), "%s", MCS_DATA_RATEStr[channelWidth][gi_ltf][OperaRate-RTW_DATA_RATE_MCS0]);
	}
	else if(OperaRate <= RTW_DATA_RATE_OFDM54){
		snprintf(rate_str, sizeof(rate_str), "%s", DATA_RATEStr[OperaRate]);
	}
	//printf("rate %x gi_ltf %d channelWidth %d\n", OperaRate, gi_ltf, channelWidth);
	//printf("%s\n", rate_str);
	if(rate_str[0])
		rate = atof(rate_str)*1024;

	return rate;
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
			if((wlan_index >=0 && vwlan_index >=0) && !(wlan_index==i && vwlan_index==j))
				continue;

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
					//va_cmd(IWPRIV, 2, 1, ifname, "clear_acl_table");
					//va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "accept_acl", "CLEAR");
					//va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "deny_acl", "CLEAR");
					snprintf(cmd_str, sizeof(cmd_str), "echo 1 clr > " PROC_WLAN_MAC_ADDR_ACL, ifname);
					va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					//printf("====> iwpriv %s clear_acl_table\n", ifname);
					//snprintf(parm, sizeof(parm), "aclmode=2");
					//va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
					//va_cmd(HOSTAPD_CLI, 5, 1, "-i", ifname, "set", "macaddr_acl", "0");
					snprintf(cmd_str, sizeof(cmd_str), "echo 1 mode 1 > " PROC_WLAN_MAC_ADDR_ACL, ifname);
					va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					//printf("====> iwpriv %s set_mib %s\n", ifname, parm);

					tmpList = pblackList;
					while(tmpList && tmpList->tail == 0)
					{
						snprintf(parm, sizeof(parm), "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
							tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
							tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
						//va_cmd(IWPRIV, 3, 1, ifname, "add_acl_table", parm);
						//va_cmd(HOSTAPD_CLI, 5, 1, "-i", ifname, "deny_acl", "ADD_MAC", parm);
						snprintf(cmd_str, sizeof(cmd_str), "echo 1 add %s > " PROC_WLAN_MAC_ADDR_ACL, parm, ifname);
						va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
						va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "disassociate", parm);
						//printf("====> iwpriv %s add_acl_table %s\n", ifname, parm);
						tmpList = tmpList->next;
					}

					tmpList = phostBlockList;
					while(tmpList && tmpList->tail == 0)
					{
						if(!findMacFilterEntry(tmpList->mac, pblackList))
						{
							snprintf(parm, sizeof(parm), "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
								tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
								tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
							//va_cmd(IWPRIV, 3, 1, ifname, "add_acl_table", parm);
							//va_cmd(HOSTAPD_CLI, 5, 1, "-i", ifname, "deny_acl", "ADD_MAC", parm);
							snprintf(cmd_str, sizeof(cmd_str), "echo 1 add %s > " PROC_WLAN_MAC_ADDR_ACL, parm, ifname);
							va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
							va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "disassociate", parm);
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
					//va_cmd(IWPRIV, 2, 1, ifname, "clear_acl_table");
					//va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "accept_acl", "CLEAR");
					//va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "deny_acl", "CLEAR");
					snprintf(cmd_str, sizeof(cmd_str), "echo 1 clr > " PROC_WLAN_MAC_ADDR_ACL, ifname);
					va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					//printf("====> iwpriv %s clear_acl_table\n", ifname);
					//snprintf(parm, sizeof(parm), "aclmode=1");
					//va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
					//va_cmd(HOSTAPD_CLI, 5, 1, "-i", ifname, "set", "macaddr_acl", "1");
					snprintf(cmd_str, sizeof(cmd_str), "echo 1 mode 2 > " PROC_WLAN_MAC_ADDR_ACL, ifname);
					va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					//printf("====> iwpriv %s set_mib %s\n", ifname, parm);

					tmpList = pwhiteList;
					while(tmpList && tmpList->tail == 0)
					{
						if(!findMacFilterEntry(tmpList->mac, phostBlockList))
						{
							snprintf(parm, sizeof(parm), "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
								tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
								tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
							//va_cmd(IWPRIV, 3, 1, ifname, "add_acl_table", parm);
							//va_cmd(HOSTAPD_CLI, 5, 1, "-i", ifname, "accept_acl", "ADD_MAC", parm);
							snprintf(cmd_str, sizeof(cmd_str), "echo 1 add %s > " PROC_WLAN_MAC_ADDR_ACL, parm, ifname);
							va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
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
										snprintf(parm, sizeof(parm), "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
											pInfo->addr[0], pInfo->addr[1], pInfo->addr[2],
											pInfo->addr[3], pInfo->addr[4], pInfo->addr[5]);
										//va_cmd(IWPRIV, 3, 1, ifname, "del_sta", parm);
										va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "disassociate", parm);
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
					//va_cmd(IWPRIV, 2, 1, ifname, "clear_acl_table");
					//va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "accept_acl", "CLEAR");
					//va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "deny_acl", "CLEAR");
					snprintf(cmd_str, sizeof(cmd_str), "echo 1 clr > " PROC_WLAN_MAC_ADDR_ACL, ifname);
					va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					//printf("====> iwpriv %s clear_acl_table\n", ifname);
					//snprintf(parm, sizeof(parm), "aclmode=%d", (phostBlockList->tail) ? 0 : 2);
					//va_cmd(IWPRIV, 3, 1, ifname, "set_mib", parm);
					//va_cmd(HOSTAPD_CLI, 5, 1, "-i", ifname, "set", "macaddr_acl", "0");
					snprintf(cmd_str, sizeof(cmd_str), "echo 1 mode %d > " PROC_WLAN_MAC_ADDR_ACL, (phostBlockList->tail)? 0:1, ifname);
					va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					//printf("====> iwpriv %s set_mib %s\n", ifname, parm);

					tmpList = phostBlockList;
					while(tmpList && tmpList->tail == 0)
					{
						snprintf(parm, sizeof(parm), "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
							tmpList->mac[0], tmpList->mac[1], tmpList->mac[2],
							tmpList->mac[3], tmpList->mac[4], tmpList->mac[5]);
						//va_cmd(IWPRIV, 3, 1, ifname, "add_acl_table", parm);
						//va_cmd(HOSTAPD_CLI, 5, 1, "-i", ifname, "deny_acl", "ADD_MAC", parm);
						snprintf(cmd_str, sizeof(cmd_str), "echo 1 add %s > " PROC_WLAN_MAC_ADDR_ACL, parm, ifname);
						va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
						va_cmd(HOSTAPD_CLI, 4, 1, "-i", ifname, "disassociate", parm);
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
				sprintf(line, "cat "PROC_WLAN_DIR_NAME"/%s/macaddr_acl | grep %02x:%02x:%02x:%02x:%02x:%02x > %s",
								ifname, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], file_block);
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
	if(idx >= 0 && idx < (sizeof(WLANIF)/sizeof(char*))){
		//printf("%s: %s, idx = %d\n", __FUNCTION__, WLANIF[idx], idx);
#if 1
		sprintf(buf, PROC_WLAN_DIR_NAME"/%s/chan_info", WLANIF[idx]);
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
#else
		sprintf(buf, "/proc/%s/SS_Result", WLANIF[idx]);
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
#endif
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
	int ret = 0/*, skfd*/;
	//struct iwreq wrq;
	char cmd_str[256];

	if(idx >= 0 && idx < (sizeof(WLANIF)/sizeof(char*)))
	{
#if 1
		snprintf(cmd_str, sizeof(cmd_str), "echo \"acs\" > "PROC_WLAN_DIR_NAME"/%s/survey_info", WLANIF[idx]);
		if(va_cmd("/bin/sh", 2, 1, "-c", cmd_str)==0)
			ret = 1;
#else
		memset(&wrq, 0, sizeof(struct iwreq));
		skfd = socket(AF_INET, SOCK_DGRAM, 0);

		if ( iw_get_ext(skfd,(char*) WLANIF[idx], SIOCGIWNAME, &wrq) < 0)
		{
			close( skfd );
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
#endif
	}
out:
	//printf(">> %s: ret = %d\n", __FUNCTION__, ret);
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
	int /*skfd,*/i,len,chCount = 0, channel, score;
	//struct iwreq wrq;
	char buf[256] = {0}, *s1, *s2, *tmp_s;
	FILE *fp=NULL;

	if(chInfo == NULL || count == NULL)
	{
		printf("%s: Error output argument\n", __FUNCTION__);
	}
	else if(idx >= 0 && idx < (sizeof(WLANIF)/sizeof(char*)))
	{
		//printf("%s: %s, idx = %d\n", __FUNCTION__, WLANIF[idx], idx);
		if(get_wlan_channel_scan_status(idx) == 2)
		{
#if 1
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
#else
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
#endif
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
	//int skfd;
	//struct iwreq wrq;
	char cmd_str[256];

	if(idx >= 0 && idx < (sizeof(WLANIF)/sizeof(char*)))
	{
		if(idx == 0) mib_get_s(MIB_WLAN_AUTO_CHAN_ENABLED, &vChar, sizeof(vChar));
		else mib_get_s(MIB_WLAN1_AUTO_CHAN_ENABLED, &vChar, sizeof(vChar));
#if 1
		if(vChar){
			snprintf(cmd_str, sizeof(cmd_str), "echo \"switch\" > "PROC_WLAN_DIR_NAME"/%s/acs", WLANIF[idx]);
			if(va_cmd("/bin/sh", 2, 1, "-c", cmd_str)==0)
				ret = 1;
		}
#else
		if(vChar){
			memset(&wrq, 0, sizeof(struct iwreq));
			skfd = socket(AF_INET, SOCK_DGRAM, 0);

			if ( iw_get_ext(skfd, WLANIF[idx], SIOCGIWNAME, &wrq) < 0)
			{
				close( skfd );
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
#endif
		else{
			printf("%s: %s no enable auto channel function !!!\n ", __FUNCTION__, WLANIF[idx]);
		}
	}
out:
	//printf(">> %s: ret = %d\n", __FUNCTION__, ret);
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

	wrq.u.data.flags = SIOCGIROAMINGBSSTRANSREQ;

    if (iw_get_ext(skfd, interface, SIOCDEVPRIVATE+2, &wrq) < 0) {
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

#ifdef WLAN_ROAMING
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
#else
int doWlSTARSSIQueryRequest(char *interface, char *mac, unsigned int channel)
{
#if 1
	int skfd;
	struct iwreq wrq;
	struct ifreq ifr;
	union iwreq_data u;
	char buffer[] = "diag_start mode=1";
	int ret=0;

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
#else
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
#endif
}
#endif //WLAN_ROAMING
int getWlSTARSSIDETAILQueryResult(char *interface, void *sta_mac_rssi_info, unsigned char type)
{
#if 1
	int skfd;
	struct iwreq wrq;
	struct ifreq ifr;
	union iwreq_data u;
	unsigned char buf[sizeof(sta_mac_rssi_detail)*MAX_UNASSOC_STA]={0};
	int ret=0;

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

	memset(sta_mac_rssi_info, 0, sizeof(sta_mac_rssi_info));
	memcpy(sta_mac_rssi_info, buf, sizeof(buf));

	return 0;
#else
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
#endif
}

int getWlSTARSSIQueryResult(char *interface, void *sta_mac_rssi_info)
{
#if 1
	int skfd;
	struct iwreq wrq;
	struct ifreq ifr;
	union iwreq_data u;
	int ret=0;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0) {
	  close( skfd );
	  /* If no wireless name : no wireless extensions */
	  return -1;
	}

	memset(sta_mac_rssi_info, '\0', sizeof(sta_mac_rssi)*MAX_PROBE_REQ_STA);
	strcpy((char*)sta_mac_rssi_info, "diag_result mode=2");

	memset(&u, 0, sizeof(union iwreq_data));
	u.data.pointer = (caddr_t)sta_mac_rssi_info;
	u.data.length = sizeof(sta_mac_rssi)*MAX_PROBE_REQ_STA;

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
#else
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
#endif
}
int stopWlSTARSSIQueryRequest(char *interface)
{
    int skfd;
    struct iwreq wrq;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd <0){
		printf("[%s %d] : Could not create socket \n",__func__, __LINE__);
		return -1;
	}

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
	unsigned int scan_interval=6;
	if ( doWlSTARSSIQueryRequest(ifname,  NULL, channel) < 0 ) {
		printf("doWlSTARSSIQueryRequest failed!");
		ret=-1;
		goto sr_err;
	}
	printf("sleeping %ds\n",scan_interval);
	sleep(scan_interval);
	if ( getWlSTARSSIDETAILQueryResult(ifname, (sta_mac_rssi_detail *)sta_mac_rssi_result, 1) < 0 ) {
		printf("getWlSTARSSIDETAILQueryResult failed!");
		ret=-1;
		goto sr_err;
	}
sr_err:
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

	if(pLANNetInfoData == NULL){
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

	wrq.u.data.flags = SIOCGISETBCNVSIE;

	if (iw_get_ext(skfd, interface, SIOCDEVPRIVATE+2, &wrq) < 0) {
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

	wrq.u.data.flags = SIOCGISETPRBVSIE;

	if (iw_get_ext(skfd, interface, SIOCDEVPRIVATE+2, &wrq) < 0) {
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

	wrq.u.data.flags = SIOCGISETPRBRSPVSIE;

	if (iw_get_ext(skfd, interface, SIOCDEVPRIVATE+2, &wrq) < 0) {
		close( skfd );
		return -1;
	}

	close( skfd );
	return (int) data[0];
}
#endif //defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU)
#endif //CONFIG_E8B

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
#if 1
	unsigned char mac[MAC_ADDR_LEN];
	char mac_str[20]={0};
	FILE *fp=NULL;
	char buf[1024]={0}, newline[1024]={0};
	int ret = 0;

	if(rtk_string_to_hex(mac_addr, mac, 12)==0)
		return -1;

	snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	va_niced_cmd(HOSTAPD_CLI, 4, 1, "-i", interface, "disassociate", mac_str);

	snprintf(buf, sizeof(buf), "%s sta %s -i %s", HOSTAPD_CLI, mac_str, interface);
	//printf("%s\n", buf);

	fp = popen(buf, "r");
	if(fp)
	{
		if(fgets(newline, 1024, fp) != NULL){
			//printf("%s\n", newline);
			if(!strcmp(newline, mac_str))
				ret = -1;
		}
		pclose(fp);
	}
	return ret;
#else
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
#endif
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

	if(doWlKickOutDevice(interface_name, mac_addr_str) < 0)
		found = 1;
	else
		found = 0;

#if 0
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
#endif
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
	int skfd;
	struct iwreq wrq;
	int query_cnt = 0;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd <0){
		printf("[%s %d] : Could not create socket \n",__func__, __LINE__);
		return -1;
	}
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

int getWlChannel( char *interface, unsigned int *num)
{
#if 0
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
#else
#ifdef WLAN_INCLUDE_IWLIB
	int skfd;
	struct iwreq		wrq;
	struct iw_range	range;
	double		freq;
	int			channel;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Get frequency / channel */
	if(iw_get_ext(skfd, interface, SIOCGIWFREQ, &wrq) < 0){
		close( skfd );
		return(-1);
	}

	/* Convert to channel */
	if(iw_get_range_info(skfd, interface, &range) < 0){
		close( skfd );
		return(-2);
	}
	freq = iw_freq2float(&(wrq.u.freq));
	if(freq < KILO)
		channel = (int) freq;
	else{
	 channel = iw_freq_to_channel(freq, &range);
	  if(channel < 0){
		close( skfd );
		return(-3);
	  }
	}
	*num = (unsigned int) channel;
	close( skfd );
	return 0;
#else
	char wlan_path[256] = {0};
	char line[1024] = {0};
	FILE *fp = NULL;
	char *ptr = NULL;
	*num = 0;
#if 1
	snprintf(wlan_path, sizeof(wlan_path), PROC_WLAN_DIR_NAME"/%s/ap_info", interface);
#else
	snprintf(wlan_path, sizeof(wlan_path), "/proc/%s/stats", ifname);
#endif

	fp = fopen(wlan_path, "r");
	if (fp) {
		while (fgets(line, sizeof(line),fp)) {
			if((ptr = strstr(line, "cur_channel"))){
				sscanf(ptr, "%*[^=]=%d%*s", num);
				//printf("cur_channel = %d\n", *num);
				break;
			}
		}
		fclose(fp);
	}
	return 0;

#endif
#endif
}

int getWlMaxStaNum( char *interface, unsigned int *num)
{
	//TODO: WIFI6
	
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
#if defined(CONFIG_E8B) || defined(CONFIG_00R0)
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
#if defined(CONFIG_E8B) || defined(CONFIG_00R0)
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

#if defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
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
	//unsigned int wlSt = 0;

	for(i=0; i<NUM_WLAN_INTERFACE; i++)
	{
		/*if(led_mode == 2)
		{
			wlSt |= (getWlanStatus(i)) ? 1<<i : 0;
			if((wlSt&(1<<i)))
				led_value = 2;
			else
				led_value = 0; // set wifi led off when root is func_off and no vap interface up
		}
		else */ led_value = led_mode;

#if !(defined(CONFIG_WIFI_LED_USE_SOC_GPIO) || defined(CONFIG_WIFI_LED_SHARED))
		if(led_value == 2){
			snprintf(cmd_str, sizeof(cmd_str), "echo 0 > " WLAN_LED_PROC, WLANIF[i]);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
		}
		else{
			snprintf(cmd_str, sizeof(cmd_str), "echo 1 %d > " WLAN_LED_PROC, led_value, WLANIF[i]);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
		}
#endif
		printf("cmd_str: %s\n", cmd_str);
	}
#if 0 // defined(CONFIG_WIFI_LED_SHARED)
	led_value = (led_mode == 2) ? ((wlSt) ? 2 : 0) : led_mode;
	if(led_value == 2){
		snprintf(cmd_str, sizeof(cmd_str), "echo 0 > "WLAN_LED_PROC, WLANIF[0]);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
	}
	else{
		snprintf(cmd_str, sizeof(cmd_str), "echo 1 %d > "WLAN_LED_PROC, led_value, WLANIF[0]);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
	}
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

static void WRITE_HOSTAPD_FILE(int fh, char *buf)
{
	if ( write(fh, buf, strlen(buf)) != strlen(buf) ) {
		printf("Write hostapd config file error!\n");
		//exit(1);
	}
}

static int _is8021xEnabled(MIB_CE_MBSSIB_Tp pEntry) {
#ifdef WLAN_1x
	if (pEntry->enable1X) {
		return 1;
	} else {
		if (pEntry->encrypt >= WIFI_SEC_WPA) {
			if (WPA_AUTH_AUTO == pEntry->wpaAuth)
				return 1;

		}
	}
#endif
	return 0;
}

int setupWLanQos(char *argv[6])
{
	//for compile
	return 0;
}

#if 0
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
	}
#endif
	return 1;
}
#endif

#ifdef RTK_MULTI_AP
static int setupWLan_multi_ap(int wlanIdx, int vwlan_idx)
{
	char ifname[16];
	char parm[128];
	int vInt, status = 0;
	MIB_CE_MBSSIB_T Entry;
#ifdef MAP_GROUP_SUPPORT
	unsigned char map_group_ssid[MAX_SSID_LEN]={0}, map_group_wpaPsk[MAX_PSK_LEN+1]={0};
#endif

	if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIdx, vwlan_idx, (void *)&Entry)) {
		printf("Error! Get MIB_MBSSIB_TBL(setupWLan_multi_ap) error.\n");
	}

	rtk_wlan_get_ifname(wlanIdx, vwlan_idx, ifname);

	if(vwlan_idx != WLAN_ROOT_ITF_INDEX){

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
							mib_local_mapping_set( MIB_REPEATER_SSID1, wlanIdx, (void *)map_group_ssid);
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
					mib_chain_local_mapping_update(MIB_MBSSIB_TBL, wlanIdx, (void *)&Entry, vwlan_idx);
			}
		}
		#endif
	}

	{
		unsigned char map_state = 0;
		mib_get_s(MIB_MAP_CONTROLLER, (void *)&map_state, sizeof(map_state));
		if(map_state) {
			if(Entry.multiap_bss_type == 0){
				Entry.multiap_bss_type = MAP_FRONTHAUL_BSS;
				mib_chain_local_mapping_update(MIB_MBSSIB_TBL, wlanIdx, (void *)&Entry, vwlan_idx);
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


static const char HOSTAPD_CONF_NAME[]="/var/hostapd.conf";
static const char HOSTAPD_GLOBAL_CTRL_PATH[]="/var/run/hostapd/global";

static const char WPAS_CONF_NAME[]="/var/wpas.conf";

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

int stopwlan(config_wlan_target target, config_wlan_ssid ssid_index)
{
	int i = 0/*, hostapd_pid=0*/, j=0;
	unsigned char phy_band_select = 0;
	unsigned char vChar = 0;
	char pidfilename[NUM_WLAN_INTERFACE][128]={0};
	char cpidfilename[128]={0};
	char global_ctrl_path[NUM_WLAN_INTERFACE][128]={0};
	char ifname[IFNAMSIZ]={0}, wps_ifname[IFNAMSIZ]={0};

#ifdef RTK_MULTI_AP
	rtk_stop_multi_ap_service();
#endif
#if defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
	rtk_wlan_get_wps_led_interface(wps_ifname);
#endif

	for(i=0; i<NUM_WLAN_INTERFACE; i++){
			mib_local_mapping_get( MIB_WLAN_PHY_BAND_SELECT, i, (void *)&phy_band_select);
			if((target == CONFIG_WLAN_2G && phy_band_select == PHYBAND_2G) ||
				(target == CONFIG_WLAN_5G && phy_band_select == PHYBAND_5G) ||
				(target == CONFIG_WLAN_ALL)){
#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
					if(ssid_index == CONFIG_SSID_ALL /*|| (_wlIdxRadioDoChange & (1<<i))*/ /* || (_wlIdxSsidDoChange & (1<<(i*(NUM_VWLAN_INTERFACE+1)))) */
#if defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(YUEME_3_0_SPEC)
						|| !getWlanStatus(i)
#endif
					)
#endif
					{
#ifdef WLAN_CLIENT
						snprintf(pidfilename[i], sizeof(pidfilename[i]), "%s.%s", WPASPID, WLANIF[i]);
						if(read_pid(pidfilename[i])>0)
							va_cmd(BRCTL, 3, 1, "delif", BRIF, WLANIF[i]);
#ifdef WLAN_WPS_WPAS
						snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", WPACLIPID, WLANIF[i]);
						if(read_pid(cpidfilename)>0){
							kill_by_pidfile(cpidfilename, SIGTERM);
							if(!strcmp(wps_ifname, WLANIF[i])){
								va_cmd(WPAS_WPS_SCRIPT, 2, 1, WLANIF[i], "WPS-TIMEOUT");
								unlink(WPS_LED_INTERFACE);
							}
						}
#endif
						kill_by_pidfile(pidfilename[i], SIGTERM);
#ifdef WLAN_UNIVERSAL_REPEATER
						rtk_wlan_get_ifname(i, WLAN_REPEATER_ITF_INDEX, ifname);
						snprintf(pidfilename[i], sizeof(pidfilename[i]), "%s.%s", WPASPID, ifname);
						if(read_pid(pidfilename[i])>0)
							va_cmd(BRCTL, 3, 1, "delif", BRIF, ifname);
#ifdef WLAN_WPS_WPAS
						snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", WPACLIPID, ifname);
						if(read_pid(cpidfilename)>0){
							kill_by_pidfile(cpidfilename, SIGTERM);
							if(!strcmp(wps_ifname, ifname)){
								va_cmd(WPAS_WPS_SCRIPT, 2, 1, ifname, "WPS-TIMEOUT");
								unlink(WPS_LED_INTERFACE);
							}
						}
#endif
						kill_by_pidfile(pidfilename[i], SIGTERM);
#endif //WLAN_UNIVERSAL_REPEATER
#endif //WLAN_CLIENT
#if 1
						snprintf(pidfilename[i], sizeof(pidfilename[i]), "%s.%s", HOSTAPDPID, WLANIF[i]);

#ifdef CONFIG_USER_LANNETINFO
						for(j=0; j<=NUM_VWLAN_INTERFACE; j++){
							rtk_wlan_get_ifname(i, j, ifname);
							snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.lannetinfo", HOSTAPDCLIPID, ifname);
							kill_by_pidfile(cpidfilename, SIGTERM);
						}
#endif
#ifdef WLAN_WPS_HAPD
#ifdef WLAN_WPS_VAP
						for(j=0; j<=NUM_VWLAN_INTERFACE; j++)
#else
						j=0;
#endif
						{
							rtk_wlan_get_ifname(i, j, ifname);
							snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", HOSTAPDCLIPID, ifname);
							if(read_pid(cpidfilename)>0){
								kill_by_pidfile(cpidfilename, SIGTERM);
								if(!strcmp(wps_ifname, ifname)){
									va_cmd(HOSTAPD_WPS_SCRIPT, 2, 1, ifname, "WPS-TIMEOUT");
									unlink(WPS_LED_INTERFACE);
								}
							}
						}

#endif
						for(j=0; j<=NUM_VWLAN_INTERFACE; j++){
							rtk_wlan_get_ifname(i, j, ifname);
							snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.general", HOSTAPDCLIPID, ifname);
							kill_by_pidfile(cpidfilename, SIGTERM);
						}

						kill_by_pidfile(pidfilename[i], SIGTERM);
						//hostapd_pid = read_pid(pidfilename[i]);
						//kill(hostapd_pid, SIGTERM);
						//while(read_pid(pidfilename[i]) > 0)
						//	usleep(200000);
#else
						va_niced_cmd(HOSTAPD_CLI, 3, 1, "-i", WLANIF[i], "disable");
#endif
						//printf("%s %d\n", __func__, __LINE__);
					}
#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
					else //only modify mbssid
					{
						for(j=0; j<=NUM_VWLAN_INTERFACE; j++){
							if(ssid_index != CONFIG_SSID_ALL && ssid_index != j)
								continue;
							if(j!=WLAN_ROOT_ITF_INDEX){
								//if(_wlIdxSsidDoChange & (1 << (j+i*(NUM_VWLAN_INTERFACE+1))))
								{
									rtk_wlan_get_ifname(i, j, ifname);
									snprintf(global_ctrl_path[i], sizeof(global_ctrl_path[i]), "%s.%s", HOSTAPD_GLOBAL_CTRL_PATH, WLANIF[i]);

#ifdef CONFIG_USER_LANNETINFO
									snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.lannetinfo", HOSTAPDCLIPID, ifname);
									kill_by_pidfile(cpidfilename, SIGTERM);
#endif
									snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.general", HOSTAPDCLIPID, ifname);
									kill_by_pidfile(cpidfilename, SIGTERM);
#ifdef WLAN_WPS_HAPD
#ifdef WLAN_WPS_VAP
#else
									if(j==0)
#endif
									{
										snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", HOSTAPDCLIPID, ifname);
										if(read_pid(cpidfilename)>0){
											kill_by_pidfile(cpidfilename, SIGTERM);
											if(!strcmp(wps_ifname, ifname)){
												va_cmd(HOSTAPD_WPS_SCRIPT, 2, 1, ifname, "WPS-TIMEOUT");
												unlink(WPS_LED_INTERFACE);
											}
										}
									}
#endif
									//wpa_cli -g /var/run/hostapd/global.wlan0 raw REMOVE wlan0
									//printf("%s -g %s %s %s %s\n", WPA_CLI, global_ctrl_path[i], "raw", "REMOVE", ifname);
									va_niced_cmd(WPA_CLI, 5, 1, "-g", global_ctrl_path[i], "raw", "REMOVE", ifname);
								}
							}
						}
					}
#endif
			}
	}
#ifdef WLAN_BAND_STEERING
	rtk_wlan_stop_band_steering();
#endif
	return 0;
}

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#ifdef WLAN_RATE_PRIOR
#ifdef WLAN_11AX
#define RATE_PRIOR_BAND_2G (BAND_11N|BAND_11AX)
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

#define WLAN_RATE_CONTROL_PROC "echo %s > "PROC_WLAN_DIR_NAME"/%s/rate_ctl"
#define WLAN_DENY_LEGACY_CONTROL_PROC "echo %d > "PROC_WLAN_DIR_NAME"/%s/deny_legacy"

static int setupWLan(int wlanIdx, int vwlan_idx)
{
	MIB_CE_MBSSIB_T Entry, Entry_root;
	char ifname[IFNAMSIZ]={0};
	char cmd_str[256]={0};
	char rate_str[32]={0};
	unsigned char phyband=0, mode=0, wlanBand=0, vChar=0;
	int deny_legacy=0;
	unsigned int vInt=0;

	if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIdx, vwlan_idx, &Entry) == 0)
		return -1;
	if(Entry.wlanDisabled == 1)
		return 0;

#ifdef RTK_MULTI_AP
	setupWLan_multi_ap(wlanIdx, vwlan_idx);
#endif

	//set something directly to driver (not to hostapd)
	rtk_wlan_get_ifname(wlanIdx, vwlan_idx, ifname);
	mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlanIdx, &phyband);

	if(vwlan_idx==WLAN_ROOT_ITF_INDEX){
		mib_local_mapping_get(MIB_TX_POWER, wlanIdx, &mode);
		vInt = getTxPowerPercent(phyband, mode);
		iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "powerpercent", vInt);

		//coexist
		mib_local_mapping_get(MIB_WLAN_11N_COEXIST, wlanIdx, (void *)&vChar);
		if(vChar==0)
			va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "coexist=0");
		else
			va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "coexist=1");
	}

#ifdef WLAN_LIFETIME
	if(vwlan_idx==WLAN_ROOT_ITF_INDEX){
		mib_local_mapping_get(MIB_WLAN_LIFETIME, wlanIdx, (void *)&vInt);
		iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "lifetime", vInt);
	}
#endif

	if(Entry.rateAdaptiveEnabled == 1){
		snprintf(cmd_str, sizeof(cmd_str), WLAN_RATE_CONTROL_PROC, "0xffff", ifname);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
	}
	else{
		snprintf(rate_str, sizeof(rate_str), "0x%x 1", Entry.fixedTxRate);
		snprintf(cmd_str, sizeof(cmd_str), WLAN_RATE_CONTROL_PROC, rate_str, ifname);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
	}

//#ifdef WLAN_UNIVERSAL_REPEATER
//	if(vwlan_idx != WLAN_REPEATER_ITF_INDEX)
//#endif
	{
#ifdef WLAN_BAND_CONFIG_MBSSID
		wlanBand = Entry.wlanBand;
#ifdef WLAN_UNIVERSAL_REPEATER
		if(vwlan_idx == WLAN_REPEATER_ITF_INDEX){
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIdx, WLAN_ROOT_ITF_INDEX, &Entry_root);
			wlanBand = Entry_root.wlanBand;
		}
#endif
		if(vwlan_idx!=WLAN_ROOT_ITF_INDEX){
			iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "band", wlanBand);
		}
#else
		mib_local_mapping_get(MIB_WLAN_BAND, wlanIdx, (void *) &wlanBand);
#endif
#ifdef WLAN_CLIENT
		if(vwlan_idx == WLAN_ROOT_ITF_INDEX && Entry.wlanMode == CLIENT_MODE)
			iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "band", wlanBand);
#endif

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

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "force_qos", 1);
	
	// enable CTC DSCP to WIFI QoS
	iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "ctc_dscp", 1);
#endif

	return 0;
}

static int _pre_setup_wlan(int wlanIdx, int vwlan_idx)
{
	MIB_CE_MBSSIB_T Entry;
	char ifname[IFNAMSIZ]={0};
	unsigned char mode=0;

	if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIdx, vwlan_idx, &Entry) == 0)
		return -1;

	rtk_wlan_get_ifname(wlanIdx, vwlan_idx, ifname);

	if(vwlan_idx==WLAN_ROOT_ITF_INDEX){
		mib_local_mapping_get(MIB_WLAN_OFDMA_ENABLED, wlanIdx, (void *)&mode);
		iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "ofdma_enable", mode);

#ifdef WLAN_TX_BEAMFORMING
		iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "txbf", Entry.txbf);
		iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "txbf_mu", Entry.txbf? Entry.txbf_mu:0);
		iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "txbf_auto_snd", Entry.txbf? Entry.txbf_mu:0);
		iwpriv_cmd(IWPRIV_INT, ifname, "set_mib", "txbf_mu_1ss", Entry.txbf? Entry.txbf_mu:0);
#endif
	}

	return 0;
}

#define WLAN_TX_BW_MODE_PROC "echo %x > "PROC_WLAN_DIR_NAME"/%s/tx_bw_mode"

static int setupWlanRootMib(int wlanIdx)
{
	MIB_CE_MBSSIB_T Entry;
	char cmd_str[256]={0};
	char ifname[IFNAMSIZ]={0};
	int status = 0;
	unsigned char vChar = 0/*, phyband = 0*/, tx_bw_mode=0;

	if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIdx, WLAN_ROOT_ITF_INDEX, &Entry) == 0)
		return -1;

	rtk_wlan_get_ifname(wlanIdx, WLAN_ROOT_ITF_INDEX, ifname);

	if(Entry.wlanDisabled)
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=1");
	else
		status|=va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=0");

#if 0
	mib_local_mapping_get(MIB_WLAN_CHANNEL_WIDTH, wlanIdx, &vChar);
	//mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlanIdx, &phyband);
	//if(phyband == PHYBAND_5G){
		tx_bw_mode = ((vChar)<<4)|vChar; //5G: first 4 bits, 2G: last 4 bits, 0:20M, 1:40M, 2:80M

		snprintf(cmd_str, sizeof(cmd_str), WLAN_TX_BW_MODE_PROC, tx_bw_mode, ifname);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
	//}
#endif

	return status;
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
static int _check_interface_up(const char *ifname)
{
	struct timespec start, end;
	int flag, ret=0, hostapd_pid;
	char pidfilename[128];
	
	clock_gettime(CLOCK_MONOTONIC, &start);
	snprintf(pidfilename, sizeof(pidfilename), "%s.%s", HOSTAPDPID, ifname);
	while(1){
		if (getInFlags(ifname, &flag) == 1)
		{
			if((flag & IFF_UP) && (flag & IFF_RUNNING)){
				ret = 0;
				break;
			}
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		//printf("%s: wait %d sec\n", __func__, end.tv_sec - start.tv_sec);
		if(read_pid(pidfilename) <= 0 || (end.tv_sec - start.tv_sec)>=10){
			printf("%s: failed\n", __func__);
			ret = -1;
			break;
		}
		sleep(1);
	}
	return ret;
}
int startWLan(config_wlan_target target, config_wlan_ssid ssid_index)
{
	//start_hapd_process();
	//return 0;
	char filename[NUM_WLAN_INTERFACE][128]={0};
	char pidfilename[NUM_WLAN_INTERFACE][128]={0};
	char cpidfilename[128]={0};
	char global_ctrl_path[NUM_WLAN_INTERFACE][128]={0};
	char bss_config[256]={0};
	char buffer[256]={0};
	int i=0, hostapd_pid=0, hostapdcli_pid=0, j=0;
#ifdef WLAN_CLIENT
	int wpas_pid=0, wpacli_pid=0;
#ifdef WLAN_UNIVERSAL_REPEATER
	unsigned char rpt_enabled = 0;
#endif
#endif
	unsigned char phy_band_select = 0;
	unsigned char vChar = 0;
	unsigned char no_wlan = 0;
	int status = 0;
	int lannetinfo_pid = 0;
	char ifname[IFNAMSIZ]={0};
	MIB_CE_MBSSIB_T Entry;
	char cmd_str[256]={0};

#if !defined(YUEME_3_0_SPEC) && !defined(CONFIG_CU_BASEON_YUEME)
	if(!check_wlan_module()){
		return 0;
	}
#endif
#ifdef MULTI_AP_OPEN_ENCRYPT_SUPPORT
	write_backhaul_encrypt_mode_flag();
	setupMultiAPController_encrypt();
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

	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		if (!getInFlags(WLANIF[i], 0)) {
			printf("Wireless Interface Not Found !\n");
			status = -1;	// interface not found
			continue;
		}
		mib_local_mapping_get( MIB_WLAN_PHY_BAND_SELECT, i, (void *)&phy_band_select);
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
		checkWlanSetting(i);
#endif
		status |= setupWlanHWSetting(WLANIF[i], i);

		//mib_local_mapping_get(MIB_WLAN_AUTO_CHAN_ENABLED, i, (void *)&vChar);
		vChar = 1; // always echo 1 to coex_2g_40m
		snprintf(cmd_str, sizeof(cmd_str), WLAN_COEX_2G_40M_PROC, vChar?1:0, WLANIF[i]);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_str);

#if defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(YUEME_3_0_SPEC)
		mib_local_mapping_get(MIB_WLAN_MODULE_DISABLED, i, (void *)&no_wlan);
		if(no_wlan == 0)
			no_wlan = getWlanStatus(i)? 0:1;
#else
		if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_ROOT_ITF_INDEX, &Entry) == 0)
			printf("\n %s %d\n", __func__, __LINE__);
		no_wlan = Entry.wlanDisabled;
#endif
		if (no_wlan){
			continue;
		}

		_pre_setup_wlan(i, WLAN_ROOT_ITF_INDEX);

		//move to interface up
		//status |= setupWlanRootMib(i);

#ifdef WLAN_CLIENT
		if(Entry.wlanMode == CLIENT_MODE)
		{
			snprintf(pidfilename[i], sizeof(pidfilename[i]), "%s.%s", WPASPID, WLANIF[i]);
			wpas_pid = read_pid(pidfilename[i]);
			snprintf(filename[i], sizeof(filename[i]), "%s.%s", WPAS_CONF_NAME, WLANIF[i]);
			//snprintf(global_ctrl_path[i], sizeof(global_ctrl_path[i]), "%s.%s", HOSTAPD_GLOBAL_CTRL_PATH, WLANIF[i]);
			if(wpas_pid<=0)
			{
				gen_wpas_cfg(i, WLAN_ROOT_ITF_INDEX, filename[i]);
				status |= va_niced_cmd(WPAS, 11, 1, "-D", "nl80211", "-b", BRIF, "-i", WLANIF[i], "-c", filename[i], "-P", pidfilename[i], "-B");
#ifdef WLAN_WPS_WPAS
				if(Entry.wsc_disabled == 0){
					snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", WPACLIPID, WLANIF[i]);
					wpacli_pid = read_pid(cpidfilename);
					if(wpacli_pid<=0){
						status |= va_niced_cmd(WPA_CLI, 7, 0, "-a", WPAS_WPS_SCRIPT, "-B", "-P", cpidfilename, "-i", WLANIF[i]);
					}
				}
#endif
				// brctl addif br0 wlan0
				status |= va_cmd(BRCTL, 3, 1, "addif", BRIF, WLANIF[i]);
			}
			status |= setupWLan(i, WLAN_ROOT_ITF_INDEX);
		}
		else
#endif
		{
			snprintf(pidfilename[i], sizeof(pidfilename[i]), "%s.%s", HOSTAPDPID, WLANIF[i]);
			hostapd_pid = read_pid(pidfilename[i]);
			snprintf(filename[i], sizeof(filename[i]), "%s.%s", HOSTAPD_CONF_NAME, WLANIF[i]);
			snprintf(global_ctrl_path[i], sizeof(global_ctrl_path[i]), "%s.%s", HOSTAPD_GLOBAL_CTRL_PATH, WLANIF[i]);

			if(hostapd_pid<=0)
			{
				gen_hapd_cfg(i, filename[i]);
				status |= va_niced_cmd(HOSTAPD, 7, 1, filename[i], "-s", "-g", global_ctrl_path[i], "-B", "-P", pidfilename[i]);
				//printf("%s %d\n", __func__, __LINE__);
#if 0 //todo fix dualband
#ifdef TRIBAND_SUPPORT
				va_niced_cmd(HOSTAPD, 6, 0, filename[0], filename[1], filename[2], "-B", "-P", pidfilename[i]);
#elif defined(WLAN_DUALBAND_CONCURRENT)
				va_niced_cmd(HOSTAPD, 5, 0, filename[0], filename[1], "-B", "-P", pidfilename[i]);
#else
				va_niced_cmd(HOSTAPD, 4, 0, filename[0], "-B", "-P", pidfilename[i]);
#endif
#endif
			}
#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
			else{
				for(j=0; j<=NUM_VWLAN_INTERFACE; j++){
					if(ssid_index != CONFIG_SSID_ALL && ssid_index != j)
						continue;
					if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, &Entry) == 0)
						continue;
					if(((j!=WLAN_ROOT_ITF_INDEX) && (Entry.wlanDisabled == 0))
#ifdef WLAN_CLIENT
						&& (Entry.wlanMode != CLIENT_MODE)
#endif
						){
						//if(_wlIdxSsidDoChange & (1 << (j+i*(NUM_VWLAN_INTERFACE+1))))
						{
							rtk_wlan_get_ifname(i, j, ifname);
							memset(bss_config, '\0', sizeof(bss_config));
							snprintf(bss_config, sizeof(bss_config), "%s.%s", HOSTAPD_CONF_NAME, ifname);
							//wpa_cli -g /var/run/hostapd/global.wlan0 raw ADD bss_config=phy0:/var/hostapd.conf.wlan0-vap0
							//printf("%s\n", bss_config);
							gen_hapd_vif_cfg(i, j, bss_config);
							snprintf(buffer, sizeof(buffer)-strlen(bss_config), "bss_config=phy%c:%s", WLANIF[i][4], bss_config);
							//printf("%s\n", buffer);
							//printf("%s -g %s %s %s %s\n", WPA_CLI, global_ctrl_path[i], "raw", "ADD", buffer);
							status |= va_niced_cmd(WPA_CLI, 5, 1, "-g", global_ctrl_path[i], "raw", "ADD", buffer);
						}
					}
				}
			}
#endif

#ifdef WLAN_CLIENT
#ifdef WLAN_UNIVERSAL_REPEATER
			mib_local_mapping_get(MIB_REPEATER_ENABLED1, i, (void *)&rpt_enabled);
			if(rpt_enabled)
			{
				rtk_wlan_get_ifname(i, WLAN_REPEATER_ITF_INDEX, ifname);
				snprintf(pidfilename[i], sizeof(pidfilename[i]), "%s.%s", WPASPID, ifname);
				wpas_pid = read_pid(pidfilename[i]);
				snprintf(filename[i], sizeof(filename[i]), "%s.%s", WPAS_CONF_NAME, ifname);
				//snprintf(global_ctrl_path[i], sizeof(global_ctrl_path[i]), "%s.%s", HOSTAPD_GLOBAL_CTRL_PATH, ifname[i]);
				if(wpas_pid<=0)
				{
					gen_wpas_cfg(i, WLAN_REPEATER_ITF_INDEX, filename[i]);
					status |= va_niced_cmd(WPAS, 11, 1, "-D", "nl80211", "-b", BRIF, "-i", ifname, "-c", filename[i], "-P", pidfilename[i], "-B");
#ifdef WLAN_WPS_WPAS
					if(Entry.wsc_disabled == 0){
						snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", WPACLIPID, ifname);
						wpacli_pid = read_pid(cpidfilename);
						if(wpacli_pid<=0){
							status |= va_niced_cmd(WPA_CLI, 7, 0, "-a", WPAS_WPS_SCRIPT, "-B", "-P", cpidfilename, "-i", ifname);
						}
					}
#endif
					// brctl addif br0 wlan0
					status |= va_cmd(BRCTL, 3, 1, "addif", BRIF, ifname);
				}
			}
#endif //WLAN_UNIVERSAL_REPEATER
#endif //WLAN_CLIENT
			for(j=0; j<=NUM_VWLAN_INTERFACE; j++){
					if(ssid_index != CONFIG_SSID_ALL && ssid_index != j)
						continue;

					status |= setupWLan(i,j);
			}
			for(j=0; j<=NUM_VWLAN_INTERFACE; j++)
			{
				if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, &Entry) == 1){
					if(((j== WLAN_ROOT_ITF_INDEX) || (Entry.wlanDisabled == 0))
#ifdef WLAN_CLIENT
						&& (Entry.wlanMode != CLIENT_MODE)
#endif
						){
						rtk_wlan_get_ifname(i, j, ifname);
						snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.general", HOSTAPDCLIPID, ifname);
						hostapdcli_pid = read_pid(cpidfilename);
						if(hostapdcli_pid<=0){
							status |= va_niced_cmd(HOSTAPD_CLI, 7, 0, "-a", HOSTAPD_GENERAL_SCRIPT, "-B", "-P", cpidfilename, "-i", ifname);
							//printf("run general script %s\n", ifname);
						}
					}
#ifdef CONFIG_YUEME
					rtk_wlan_get_ifname(i, j, ifname);
					setup_wlan_accessRule_netfilter(ifname, &Entry);
#endif
				}
			}
#ifdef WLAN_WPS_HAPD
#ifdef WLAN_WPS_VAP
			for(j=0; j<=NUM_VWLAN_INTERFACE; j++)
#else
			j=0;
#endif
			{
				if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, &Entry) == 1){
					if((Entry.wlanDisabled == 0) && (Entry.wsc_disabled == 0)
#ifdef WLAN_CLIENT
						&& (Entry.wlanMode != CLIENT_MODE)
#endif
						){
						rtk_wlan_get_ifname(i, j, ifname);
						snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.wps", HOSTAPDCLIPID, ifname);
						hostapdcli_pid = read_pid(cpidfilename);
						if(hostapdcli_pid<=0){
							status |= va_niced_cmd(HOSTAPD_CLI, 7, 0, "-a", HOSTAPD_WPS_SCRIPT, "-B", "-P", cpidfilename, "-i", ifname);
						}
					}
				}
			}
#endif
#ifdef CONFIG_USER_LANNETINFO
#ifdef CONFIG_USER_HAPD
			for(j=0; j<=NUM_VWLAN_INTERFACE; j++){
				if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, &Entry) == 0)
					continue;

				if((Entry.wlanDisabled == 0)
#ifdef WLAN_CLIENT
					&& (Entry.wlanMode != CLIENT_MODE)
#endif
					){
					rtk_wlan_get_ifname(i, j, ifname);
					snprintf(cpidfilename, sizeof(cpidfilename), "%s.%s.lannetinfo", HOSTAPDCLIPID, ifname);
					hostapdcli_pid = read_pid(cpidfilename);
					if(hostapdcli_pid<=0){
						//lannetinfo_pid = read_pid(LANNETINFO_RUNFILE);
						//if(lannetinfo_pid>0)
						//printf("%s %s %s %s %s %s %s %s\n", HOSTAPD_CLI, "-a", LANNETINFO_ACTION, "-B", "-P", cpidfilename, "-i", ifname );
#ifdef CONFIG_USER_HAPD
						status |= va_niced_cmd(HOSTAPD_CLI, 7, 0, "-a", LANNETINFO_ACTION, "-B", "-P", cpidfilename, "-i", ifname);
#endif
						//printf("%s %d\n", __func__, __LINE__);
					}
				}
			}
#endif
#endif
			_check_interface_up(WLANIF[i]); //avoid both hostapd up at the same time cause wps upnp http init failed
#ifdef CONFIG_YUEME
			setupWlanVSIE(i, phy_band_select); //must set after interface up
#endif
		}
	}

	setup_wlan_block();

#ifdef _PRMT_X_CMCC_WLANSHARE_
	setup_wlan_share();
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	setup_wlan_MAC_ACL(-1, -1);
#endif

#ifdef RTK_MULTI_AP
	rtk_start_multi_ap_service();
#endif

#ifdef WLAN_BAND_STEERING
	rtk_wlan_setup_band_steering();
#endif

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

	UNLOCK_WLAN();
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
	{MIB_WLAN_MODULE_DISABLED,		_SIZE(wlanModuleDisabled),		1},
	{MIB_WLAN_BASIC_RATE,			_SIZE(basicRates), 				1},
	{MIB_WLAN_SUPPORTED_RATE,		_SIZE(supportedRates), 			1},
	{MIB_WLAN_RTS_THRESHOLD,		_SIZE(rtsThreshold), 			0},
	{MIB_WLAN_FRAG_THRESHOLD,		_SIZE(fragThreshold), 			0},
	{MIB_WLAN_PREAMBLE_TYPE,		_SIZE(preambleType), 			0},
	{MIB_WLAN_INACTIVITY_TIME,		_SIZE(inactivityTime), 			0},
	{MIB_WLAN_DTIM_PERIOD,			_SIZE(dtimPeriod), 				0},
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
#ifdef WLAN_RATE_PRIOR
	{MIB_WLAN_RATE_PRIOR,			_SIZE(wlan_rate_prior), 		1},
#endif
	/* not radio parameter but need to restart for both band */
#if defined(WLAN_BAND_STEERING)
	{MIB_WIFI_STA_CONTROL,				_SIZE(wifi_sta_control),		1},
#endif
#ifdef WLAN_WPS
	{MIB_WSC_PIN,					_SIZE(wscPin),					1},
#endif
#if !defined(WLAN_BAND_CONFIG_MBSSID)
	{MIB_WLAN_BAND, 				_SIZE(wlanBand),				1},
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

#define WRITE_HOSTAPD_GENERAL_SCRIPT_PARAM(dst, dst_size, tmp, tmp_size, str, val, ret_err) {	\
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

static int writeHostapdGeneralScript(const char *in, const char *out)
{
	int fh=-1;
	char tmpbuf[256]={0};
	char sysbuf[2048]={0};
	char *buf=NULL, *ptr=NULL, *sysptr=NULL;
	struct stat status;
	int ret_err_write=0, ssize, fsize;

	if (stat(in, &status) < 0) {
		printf("stat() error [%s]!\n", in);
		goto ERROR;
	}

	buf = malloc(status.st_size);
	if (buf == NULL) {
		printf("malloc() error [%d]!\n", (int)status.st_size);
		goto ERROR;
	}

	ptr = buf;
	memset(ptr, '\0', status.st_size);

	fh = open(in, O_RDONLY);
	if (fh == -1) {
		printf("open() error [%s]!\n", in);
		goto ERROR;
	}

	lseek(fh, 0L, SEEK_SET);
	if (read(fh, ptr, status.st_size) != status.st_size) {
		printf("read() error [%s]!\n", in);
		goto ERROR;
	}

	ptr = strstr(ptr, "System Settings - END");
	ptr += strlen("System Settings - END");
	fsize = ptr - buf;
	close(fh);

	sysptr = sysbuf;
	ssize = sizeof(sysbuf);

	WRITE_HOSTAPD_GENERAL_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"SYSTEMD_USOCK_SERVER=\"%s\"\n", SYSTEMD_USOCK_SERVER, ret_err_write);

	if (ret_err_write) {
		printf("WRITE_HOSTAPD_WPS_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

#ifdef RESTRICT_PROCESS_PERMISSION
	fh = open(out, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR|S_IXGRP|S_IXOTH);
#else
	fh = open(out, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
#endif
	if (fh == -1) {
		printf("Create hostapd wps file %s error!\n", out);
		goto ERROR;
	}
	snprintf(tmpbuf, sizeof(tmpbuf), "#!/bin/sh\n");
	if (write(fh, tmpbuf, strlen(tmpbuf)) != strlen(tmpbuf)) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}

	if (write(fh, sysbuf, sysptr-sysbuf) != (sysptr-sysbuf) ) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}

	if (write(fh, ptr, (status.st_size-fsize)) != (status.st_size-fsize) ) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}
	close(fh);
	free(buf);

	return 0;
ERROR:
	if (buf) free(buf);
	if (-1 != fh) close(fh);

	return -1;
}


#if defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
int rtk_wlan_get_wps_status(char *ifname)
{
	FILE *fp=NULL;
	char buf[1024]={0}, newline[1024]={0};
	int wlan_index=0, vwlan_index=0;
	MIB_CE_MBSSIB_T Entry;

	if(rtk_wlan_get_mib_index_mapping_by_ifname(ifname, &wlan_index, &vwlan_index)==0)
	{
		if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, vwlan_index, (void *)&Entry)==1)
		{
			if(Entry.wlanDisabled)
				return 0;
		}
	}

	snprintf(buf, sizeof(buf), "%s wps_get_status -i %s", HOSTAPD_CLI, ifname);

	fp = popen(buf, "r");
	if(fp)
	{
		if(fgets(newline, 1024, fp) != NULL){
			sscanf(newline, "%*[^:]:%s", buf);
			//printf("sw_commit %d\n", version);
			printf("%s\n", buf);
		}
		pclose(fp);
	}
	if(!strcmp(buf, "Active"))
		return 1;
	else
		return 0;
}

static int rtk_wlan_get_wps_led_interface(char *ifname)
{
	FILE *fp=NULL;
	char buf[32]={0};
	int ret = -1;
	fp = fopen(WPS_LED_INTERFACE, "r");
	if(fp){
		if(fscanf(fp, "%s", buf) == 1){
			strcpy(ifname, buf);
			ret = 0;
		}
		fclose(fp);
	}
	return ret;
}

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
			//printf("ip %s\n", ip);
			ret = 0;
			goto end;
		}
	}
end:
	fclose(fp);
	return ret;
}
static int check_wps_dev_info(char *mac, char *devinfo)
{
	char ipaddr[20] = {0};
	int ret = -1;
	ret = getIP(ipaddr, mac);
	sprintf(devinfo, "IP:%s;MAC:%s", ipaddr, mac);
	return ret;
}

void run_wps_query(int ssid_idx, char *wps_status, char *devinfo)
{
	FILE *fp=NULL;
	char buf[4][1024]={0}, newline[1024]={0};
	char ifname[IFNAMSIZ]={0};
	int i=0, flag=0;
	rtk_wlan_get_ifname_by_ssid_idx(ssid_idx, ifname);

	if (getInFlags(ifname, &flag) == 0 || (flag & IFF_UP) == 0 || (flag & IFF_RUNNING) == 0)
	{
		*wps_status=4;
		return;
	}

	snprintf(buf[0], sizeof(buf[0]), "%s wps_get_status -i %s", HOSTAPD_CLI, ifname);

	fp = popen(buf[0], "r");
	if(fp)
	{
		for(i=0; i<4;i++){
			if(fgets(newline, 1024, fp) != NULL){
				sscanf(newline, "%*[^:]:%s", buf[i]);
				//printf("sw_commit %d\n", version);
				//printf("%s\n", buf[i]);
			}
		}
		pclose(fp);
	}
	else{
		*wps_status=4;
		return;
	}
	if(!strcmp(buf[0], "Active")){
		*wps_status=0;
	}
	else if(!strcmp(buf[0], "Disabled") && !strcmp(buf[1], "None")){
		*wps_status=4;
	}
	else if(!strcmp(buf[0], "Timed-out")){
		*wps_status=3;
	}
	else if(!strcmp(buf[1], "Failed")){
		*wps_status=1;
	}
	else if(!strcmp(buf[1], "Success")){
		*wps_status=2;
		if(check_wps_dev_info(buf[2], devinfo) != 0)
			*wps_status=0;
	}
}

static char *_get_token(char *data, char *token)
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

static int _get_value(char *data, char *value)
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

static void *pthreadConfigWLAN(void *param)
{
	config_wlan_ssid ssid_index;
	int action_type;
	action_type = ((int *)param)[0];
	ssid_index = ((int *)param)[1];
	pthread_detach(pthread_self());
	config_WLAN(action_type, ssid_index);
	pthread_exit(NULL);
}

static void pthreadWlanConfig(int action_type, config_wlan_ssid ssid_index)
{
	int param[3]={0};
	pthread_t ntid;
	int err = 0;

	param[0] = action_type;
	param[1] = ssid_index;
	param[2] = 0;
	err = pthread_create(&ntid, NULL, pthreadConfigWLAN, param);

	if (err != 0) {
		printf("Can't create thread: %s\n", strerror(err));
	}
}
#endif

#if defined(WLAN_WPS_HAPD)
void rtk_wlan_update_wlan_wps_from_file(char *ifname)
{
	FILE *fp=NULL;
	char line[200], value[100], token[40], tmpBuf[40]={0}, *ptr;
	int set_band=0, i, tmp_wpa=0, tmp_wpa_key_mgmt=0, tmp_wpa_pairwise=0, tmp_wsc_config=0, is_restart_wlan=0;
	int /*ori_wlan_idx = wlan_idx,*/ virtual_idx=0;
	MIB_CE_MBSSIB_T Entry, Entry1;
	int vwlan_idx[NUM_WLAN_INTERFACE]={0}, root_idx[NUM_WLAN_INTERFACE]={0};
	char filename[256];

	for(i=0; i<NUM_WLAN_INTERFACE ; i++)
	{
		if(!strncmp(ifname, WLANIF[i], strlen(WLANIF[i])))
			break;
	}
	if(i>=NUM_WLAN_INTERFACE)
		return;

	//for(i=0; i<NUM_WLAN_INTERFACE ; i++)
	{
		//wlan_idx = i;
#if 0
		virtual_idx = 0;
		if(virtual_idx == 0)
		{
			if(wlan_idx == 0)
				fp = fopen("/var/run/hapd_conf_wlan0", "r");
			else if(wlan_idx == 1)
				fp = fopen("/var/run/hapd_conf_wlan1", "r");
		}
#else
#if 0 //def WLAN_WPS_VAP
		if(NULL != strstr(ifname, "vap"))
			virtual_idx = (ifname[9] - '0') + WLAN_VAP_ITF_INDEX;
#endif

		snprintf(filename, sizeof(filename), "%s.%s", HOSTAPD_CONF_NAME, WLANIF[i]);
		fp = fopen(filename, "r");
#endif
		if(!fp){
			printf("[%s]open %s fail\n", __FUNCTION__,filename);
			return;
		}
		printf("%s %s\n", __func__, ifname);
		//printf("=============================\n\n");
		memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));
		if(fgets(line, sizeof(line), fp))
		{
			if(!strstr(line,"WPS configuration - START"))
			{
				fclose(fp);
				return;
			}
			is_restart_wlan = 1;
			while ( fgets(line, sizeof(line), fp) ) {
				if(strstr(line,"WPS configuration - END"))
					break;
				ptr = (char *)_get_token(line, token);
				if (ptr == NULL){
					continue;
				}
				if (_get_value(ptr, value)==0)
					continue;
				printf("%s=%s\n", token, value);
				if(!strcmp(token, "wps_state")){
					if(atoi(value) == 2)
					{
						Entry.wsc_configured = 1;
						if(virtual_idx == 0)
							tmp_wsc_config = 1;
					}
					else
						Entry.wsc_configured = 0;
				}else if(!strcmp(token, "ssid")){
					memset(Entry.ssid, 0, sizeof(Entry.ssid));
					strncpy(Entry.ssid, value, sizeof(Entry.ssid)-1);
				}else if(!strcmp(token, "wpa")){
					tmp_wpa = atoi(value);
				}else if(!strcmp(token, "wpa_key_mgmt")){
					if(!strcmp(value, "WPA-PSK"))
					{
						tmp_wpa_key_mgmt = 1;
						Entry.wpaAuth = WPA_AUTH_PSK;
					}
					else if(!strcmp(value, "WPA-PSK-SHA256"))
					{
						Entry.encrypt = WIFI_SEC_WPA2;
#ifdef WLAN_11W
						Entry.sha256 = 1;
#endif
						Entry.wpaAuth = WPA_AUTH_PSK;
					}
				}else if(!strcmp(token, "wpa_pairwise")){
					Entry.unicastCipher = 0;
					Entry.wpa2UnicastCipher = 0;
					if(strstr(value,"TKIP") && strstr(value,"CCMP"))
						tmp_wpa_pairwise = WPA_CIPHER_MIXED;
					else if(strstr(value,"TKIP"))
						tmp_wpa_pairwise = WPA_CIPHER_TKIP;
					else if(strstr(value,"CCMP"))
						tmp_wpa_pairwise = WPA_CIPHER_AES;
				}else if(!strcmp(token, "wpa_passphrase")){
					strncpy(Entry.wpaPSK, value, sizeof(Entry.wpaPSK));
					strncpy(Entry.wscPsk, value, sizeof(Entry.wscPsk));
					Entry.wpaPSKFormat = 0;
				}else if(!strcmp(token, "wpa_psk")){
					strncpy(Entry.wpaPSK, value, sizeof(Entry.wpaPSK));
					strncpy(Entry.wscPsk, value, sizeof(Entry.wscPsk));
					Entry.wpaPSKFormat = 1;
				}else if(!strcmp(token, "auth_algs")){
					if(atoi(value) == 2)
						Entry.authType = 1;
					else if(atoi(value) == 1)
						Entry.authType = 2;
				}
				else
					printf("[%s %d]unkown mib:%s\n", __FUNCTION__,__LINE__, token);
			}
			fclose(fp);
			if(tmp_wpa_pairwise == WPA_CIPHER_MIXED)
			{
				Entry.unicastCipher = 3;
				Entry.wpa2UnicastCipher = 3;
			}
			else if(tmp_wpa_pairwise == WPA_CIPHER_AES)
			{
				Entry.wpa2UnicastCipher = 2;
			}
			else if(tmp_wpa_pairwise == WPA_CIPHER_TKIP)
			{
				Entry.unicastCipher = 1;
			}
			if(tmp_wpa_key_mgmt == 1)
			{
				if(tmp_wpa == 3)
				{
					Entry.encrypt = WIFI_SEC_WPA2_MIXED;
					Entry.wsc_auth = WSC_AUTH_WPA2PSKMIXED;
					Entry.wsc_enc = WSC_ENCRYPT_TKIPAES;
				}
				else if(tmp_wpa == 2)
				{
					Entry.encrypt = WIFI_SEC_WPA2;
					Entry.wsc_auth = WSC_AUTH_WPA2PSK;
					Entry.wsc_enc = WSC_ENCRYPT_AES;
				}
				else if(tmp_wpa == 1)
				{
					Entry.encrypt = WIFI_SEC_WPA;
					Entry.wsc_auth = WSC_AUTH_WPAPSK;
					Entry.wsc_enc = WSC_ENCRYPT_TKIP;
				}
			}
			//printf("=============================\n\n");
			for(i=0; i<NUM_WLAN_INTERFACE; i++)
			{
				memset(&Entry1, 0, sizeof(MIB_CE_MBSSIB_T));
				mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, virtual_idx, (void *)&Entry1);
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
				Entry1.wsc_configured = Entry.wsc_configured;
				mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry1, virtual_idx);
				if(virtual_idx==0){
					mib_local_mapping_set(MIB_WLAN_SSID, i, Entry.ssid);
					mib_local_mapping_set(MIB_WLAN_ENCRYPT, i, &Entry.encrypt);
					mib_local_mapping_set(MIB_WLAN_AUTH_TYPE, i, &Entry.authType);
					mib_local_mapping_set(MIB_WSC_AUTH, i, &Entry.wsc_auth);
					mib_local_mapping_set(MIB_WLAN_WPA_AUTH, i, &Entry.wpaAuth);
					mib_local_mapping_set(MIB_WLAN_WPA_PSK_FORMAT, i, &Entry.wpaPSKFormat);
					mib_local_mapping_set(MIB_WLAN_WPA_PSK, i, Entry.wpaPSK);
					mib_local_mapping_set(MIB_WSC_PSK, i, Entry.wscPsk);
					mib_local_mapping_set(MIB_WLAN_WPA_CIPHER_SUITE, i, &Entry.unicastCipher);
					mib_local_mapping_set(MIB_WLAN_WPA2_CIPHER_SUITE, i, &Entry.wpa2UnicastCipher);
					mib_local_mapping_set(MIB_WSC_ENC, i, &Entry.wsc_enc);
					mib_local_mapping_set(MIB_WSC_CONFIGURED, i, &Entry.wsc_configured);
				}
			}
		}
		else
			fclose(fp);
	}
	if(tmp_wsc_config == 1)
	{
		for(i=0; i<NUM_WLAN_INTERFACE ; i++)
		{
			//wlan_idx = i;
			memset(&Entry1, 0, sizeof(MIB_CE_MBSSIB_T));
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, 0, (void *)&Entry1);
			Entry1.wsc_configured = 1;
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry1, 0);
		}
	}
	//wlan_idx = ori_wlan_idx;
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	if(is_restart_wlan == 1)
	{
		printf("wps wlan restart\n");
		pthreadWlanConfig(ACT_RESTART, CONFIG_SSID_ALL);
		//config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
	}
	return;
}

#ifdef WLAN_WPS_VAP
enum wps_attribute {
	ATTR_AUTH_TYPE = 0x1003,
	ATTR_CRED = 0x100e,
	ATTR_ENCR_TYPE = 0x100f,
	ATTR_MAC_ADDR = 0x1020,
	ATTR_NETWORK_INDEX = 0x1026,
	ATTR_NETWORK_KEY = 0x1027,
	ATTR_SSID = 0x1045,
};
/* Authentication Type Flags */
#define WPS_AUTH_OPEN 0x0001
#define WPS_AUTH_WPAPSK 0x0002
#define WPS_AUTH_SHARED 0x0004 /* deprecated */
#define WPS_AUTH_WPA 0x0008
#define WPS_AUTH_WPA2 0x0010
#define WPS_AUTH_WPA2PSK 0x0020
#define WPS_AUTH_TYPES (WPS_AUTH_OPEN | WPS_AUTH_WPAPSK | WPS_AUTH_SHARED | \
			WPS_AUTH_WPA | WPS_AUTH_WPA2 | WPS_AUTH_WPA2PSK)

/* Encryption Type Flags */
#define WPS_ENCR_NONE 0x0001
#define WPS_ENCR_WEP 0x0002 /* deprecated */
#define WPS_ENCR_TKIP 0x0004
#define WPS_ENCR_AES 0x0008
#define WPS_ENCR_TYPES (WPS_ENCR_NONE | WPS_ENCR_WEP | WPS_ENCR_TKIP | \
			WPS_ENCR_AES)


void rtk_wlan_update_wlan_wps_from_string(char *ifname, char *hexdump){
	unsigned short uShort;
	unsigned short wps_attr=0, ssidlen=0, auth_type=0, encr_type=0, keylen=0, maclen=0;
	char *ptr = hexdump;
	unsigned char tmp[4];
	MIB_CE_MBSSIB_T Entry, Entry1;
	int i, virtual_idx = 0;
	//printf("%s %s\n", ifname, ptr);

	for(i=0; i<NUM_WLAN_INTERFACE ; i++)
	{
		if(!strncmp(ifname, WLANIF[i], strlen(WLANIF[i])))
			break;
	}
	if(i>=NUM_WLAN_INTERFACE)
		return;

	if(NULL != strstr(ifname, "vap"))
		virtual_idx = (ifname[9] - '0') + WLAN_VAP_ITF_INDEX;

	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, virtual_idx, (void *)&Entry);

	while(*ptr !='\0'){
		//printf("%s %d\n", __func__, __LINE__);
		rtk_string_to_hex(ptr, tmp, 4);
		memcpy(&uShort, tmp, sizeof(unsigned short));
		wps_attr = ntohs(uShort);
		ptr+=4;
		//printf("%s %d %x\n", __func__, __LINE__, wps_attr);
		switch (wps_attr)
		{
			case ATTR_CRED:
				ptr+=4;
				break;
			case ATTR_NETWORK_INDEX:
				ptr+=6;
				break;
			case ATTR_SSID:
				rtk_string_to_hex(ptr, tmp, 4);
				memcpy(&uShort, tmp, sizeof(unsigned short));
				ssidlen = ntohs(uShort);
				ptr+=4;
				rtk_string_to_hex(ptr, Entry.ssid, ssidlen*2);
				ptr+=(ssidlen*2);
				break;
			case ATTR_AUTH_TYPE:
				ptr+=4;
				rtk_string_to_hex(ptr, tmp, 4);
				memcpy(&uShort, tmp, sizeof(unsigned short));
				auth_type = ntohs(uShort);
				if ((auth_type & (WPS_AUTH_WPA2 | WPS_AUTH_WPA2PSK)) &&
				    (auth_type & (WPS_AUTH_WPA | WPS_AUTH_WPAPSK)))
					Entry.encrypt = WIFI_SEC_WPA2_MIXED;
				else if (auth_type & (WPS_AUTH_WPA2 | WPS_AUTH_WPA2PSK))
					Entry.encrypt = WIFI_SEC_WPA2;
				else if (auth_type & (WPS_AUTH_WPA | WPS_AUTH_WPAPSK))
					Entry.encrypt = WIFI_SEC_WPA;
				ptr+=4;
				break;
			case ATTR_ENCR_TYPE:
				ptr+=4;
				rtk_string_to_hex(ptr, tmp, 4);
				memcpy(&uShort, tmp, sizeof(unsigned short));
				encr_type = ntohs(uShort);
				if(encr_type == (WPS_ENCR_AES|WPS_ENCR_TKIP)){
					if(Entry.encrypt|WIFI_SEC_WPA)
						Entry.unicastCipher = 3;
					if(Entry.encrypt|WIFI_SEC_WPA2)
						Entry.wpa2UnicastCipher = 3;
				}
				else if(encr_type & WPS_ENCR_AES){
					if(Entry.encrypt|WIFI_SEC_WPA)
						Entry.unicastCipher = 2;
					if(Entry.encrypt|WIFI_SEC_WPA2)
						Entry.wpa2UnicastCipher = 2;
				}
				else if(encr_type & WPS_ENCR_TKIP){
					if(Entry.encrypt|WIFI_SEC_WPA)
						Entry.unicastCipher = 1;
					if(Entry.encrypt|WIFI_SEC_WPA2)
						Entry.wpa2UnicastCipher = 1;
				}
				ptr+=4;
				break;
			case ATTR_NETWORK_KEY:
				rtk_string_to_hex(ptr, tmp, 4);
				memcpy(&uShort, tmp, sizeof(unsigned short));
				keylen = ntohs(uShort);
				ptr+=4;
				rtk_string_to_hex(ptr, Entry.wpaPSK, keylen*2);
				ptr+=(keylen*2);
				break;
			case ATTR_MAC_ADDR:
				rtk_string_to_hex(ptr, tmp, 4);
				memcpy(&uShort, tmp, sizeof(unsigned short));
				maclen = ntohs(uShort);
				ptr+=4;
				ptr+=(maclen*2);
				break;
		}
	}

	//Entry.wsc_configured = 1;
	mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, virtual_idx);

	update_wps_configured(0);

	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	//if(is_restart_wlan == 1)
	{
		printf("wps wlan restart\n");
		pthreadWlanConfig(ACT_RESTART, CONFIG_SSID_ALL);
		//config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
	}

	return;
}
#else
void rtk_wlan_update_wlan_wps_from_string(char *ifname, char *hexdump)
{
	//do nothing
	return;
}
#endif
#endif

#if defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
#define WRITE_SCRIPT_PARAM(dst, dst_size, tmp, tmp_size, str, val, ret_err) {	\
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

const char *LED_WPS_G_LABEL_NAME[]={"LED_WPS_G", "LED_WPS", ""};
#define WPS_LED_CONTROL_STATUS_FILE "/tmp/wps_led_control_status"
#define WPS_LED_CONTROL_RESTORE_FILE "/tmp/wps_led_control_restore"
#define WPS_LED_TIMER_PID "/var/run/wps_timer.sh.pid"

static int writeWpsScript(const char *in, const char *out)
{
	int fh=-1;
	char tmpbuf[256]={0};
	char sysbuf[2048]={0};
	char *buf=NULL, *ptr=NULL, *sysptr=NULL;
	struct stat status;
	int ret_err_write=0, ssize, fsize;
	int index = 0;

	if (stat(in, &status) < 0) {
		printf("stat() error [%s]!\n", in);
		goto ERROR;
	}

	buf = malloc(status.st_size);
	if (buf == NULL) {
		printf("malloc() error [%d]!\n", (int)status.st_size);
		goto ERROR;
	}

	ptr = buf;
	memset(ptr, '\0', status.st_size);

	fh = open(in, O_RDONLY);
	if (fh == -1) {
		printf("open() error [%s]!\n", in);
		goto ERROR;
	}

	lseek(fh, 0L, SEEK_SET);
	if (read(fh, ptr, status.st_size) != status.st_size) {
		printf("read() error [%s]!\n", in);
		goto ERROR;
	}

	ptr = strstr(ptr, "System Settings - END");
	ptr += strlen("System Settings - END");
	fsize = ptr - buf;
	close(fh);

	sysptr = sysbuf;
	ssize = sizeof(sysbuf);

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"SYSTEMD_USOCK_SERVER=\"%s\"\n", SYSTEMD_USOCK_SERVER, ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"WPS_LED_INTERFACE=\"%s\"\n", WPS_LED_INTERFACE, ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	while(strlen(LED_WPS_G_LABEL_NAME[index]) != 0){
		snprintf(tmpbuf, sizeof(tmpbuf), "/sys/class/leds/%s/brightness", LED_WPS_G_LABEL_NAME[index]);
		if(!access(tmpbuf, F_OK))
			break;
		index++;
	}
	if(strlen(LED_WPS_G_LABEL_NAME[index]) == 0){
		printf("WPS LED proc is not found!\nPlease check if you need WPS LED!\n");
		index--;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"LED_WPS_G_LABEL=\"%s\"\n", LED_WPS_G_LABEL_NAME[index], ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"WPS_LED_CONTROL_STATUS_FILE=\"%s\"\n", WPS_LED_CONTROL_STATUS_FILE, ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"WPS_LED_CONTROL_RESTORE_FILE=\"%s\"\n", WPS_LED_CONTROL_RESTORE_FILE, ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"WPS_LED_TIMER_PID=\"%s\"\n", WPS_LED_TIMER_PID, ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

#if 0
	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"LED_WPS_G_TRIGGER=\"/sys/class/leds/%s/trigger\"\n", LED_WPS_G_LABEL_NAME[index], ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"LED_WPS_G_DELAY_ON=\"/sys/class/leds/%s/delay_on\"\n", LED_WPS_G_LABEL_NAME[index], ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"LED_WPS_G_DELAY_OFF=\"/sys/class/leds/%s/delay_off\"\n", LED_WPS_G_LABEL_NAME[index], ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"LED_WPS_G_BRIGHTNESS=\"/sys/class/leds/%s/brightness\"\n", LED_WPS_G_LABEL_NAME[index], ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"LED_WPS_G_SHOT=\"/sys/class/leds/%s/shot\"\n", LED_WPS_G_LABEL_NAME[index], ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}
#endif


#ifdef RESTRICT_PROCESS_PERMISSION
	fh = open(out, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR|S_IXGRP|S_IXOTH);
#else
	fh = open(out, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
#endif
	if (fh == -1) {
		printf("Create hostapd wps file %s error!\n", out);
		goto ERROR;
	}
	snprintf(tmpbuf, sizeof(tmpbuf), "#!/bin/sh\n");
	if (write(fh, tmpbuf, strlen(tmpbuf)) != strlen(tmpbuf)) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}

	if (write(fh, sysbuf, sysptr-sysbuf) != (sysptr-sysbuf) ) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}

	if (write(fh, ptr, (status.st_size-fsize)) != (status.st_size-fsize) ) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}
	close(fh);
	free(buf);

	return 0;
ERROR:
	if (buf) free(buf);
	if (-1 != fh) close(fh);

	return -1;
}

// led_mode: 0:led off   1:led on 2:led state restore
static int _set_wlan_wps_led_status(int led_mode)
{
	int led_index = 0;
	unsigned int max_brightness = 0;
	char buf[128]={0}, cmd[256]={0};
	FILE *fp=NULL;
	while(strlen(LED_WPS_G_LABEL_NAME[led_index]) != 0){
		snprintf(buf, sizeof(buf), "/sys/class/leds/%s/brightness", LED_WPS_G_LABEL_NAME[led_index]);
		if(!access(buf, F_OK))
			break;
		led_index++;
	}
	if(strlen(LED_WPS_G_LABEL_NAME[led_index]) == 0){
		//printf("WPS LED proc is not found!\nPlease check if you need WPS LED!\n");
		return -1;
	}
	if(led_mode != 2){
		snprintf(cmd, sizeof(cmd), "echo 1 > %s", WPS_LED_CONTROL_STATUS_FILE);
		va_niced_cmd("/bin/sh", 2, 1, "-c", cmd);
		if(led_mode == 1){
			snprintf(buf, sizeof(buf), "/sys/class/leds/%s/max_brightness", LED_WPS_G_LABEL_NAME[led_index]);
			fp=fopen(buf, "r");
			if(fp){
				fscanf(fp, "%u", &max_brightness);
				fclose(fp);
			}
		}
		snprintf(cmd, sizeof(cmd), "echo none > /sys/class/leds/%s/trigger", LED_WPS_G_LABEL_NAME[led_index]);
		va_niced_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "echo %u > /sys/class/leds/%s/brightness", max_brightness, LED_WPS_G_LABEL_NAME[led_index]);
		va_niced_cmd("/bin/sh", 2, 1, "-c", cmd);
	}
	else{ //restore
		unlink(WPS_LED_CONTROL_STATUS_FILE);
		if(!access(WPS_LED_CONTROL_RESTORE_FILE, F_OK)){
			fp = fopen(WPS_LED_CONTROL_RESTORE_FILE, "r");
			if(fp){
				while(fgets(cmd,sizeof(cmd),fp)){
					va_niced_cmd("/bin/sh", 2, 1, "-c", cmd);
					//printf("cmd: %s\n", cmd);
				}
				fclose(fp);
			}
			unlink(WPS_LED_CONTROL_RESTORE_FILE);
		}
		else {
			if(!access(WPS_LED_TIMER_PID, F_OK)){
				snprintf(buf, sizeof(buf), "/sys/class/leds/%s/max_brightness", LED_WPS_G_LABEL_NAME[led_index]);
				fp=fopen(buf, "r");
				if(fp){
					fscanf(fp, "%u", &max_brightness);
					fclose(fp);
				}
			}
			snprintf(cmd, sizeof(cmd), "echo %u > /sys/class/leds/%s/brightness", max_brightness, LED_WPS_G_LABEL_NAME[led_index]);
			va_niced_cmd("/bin/sh", 2, 1, "-c", cmd);
			//printf("cmd: %s\n", cmd);
		}
	}
	return 0;
}
#endif

#ifdef WLAN_WPS_WPAS
void rtk_wlan_update_wlan_client_wps_from_file(char *ifname)
{
#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
	FILE *fp=NULL;
	int wlanIdx=0, vwlan_idx=0, network_num=0, tmp_wpa_pairwise=0, tmp_wpa_key_mgmt=0, tmp_wpa=0, restart_flag=0;
	MIB_CE_MBSSIB_T Entry;
	char line[200]={0}, value[100], token[40], tmpBuf[40]={0}, *ptr;
#ifdef WLAN_UNIVERSAL_REPEATER
	unsigned char rpt_enabled=0;
#endif
	char filename[256]={0};

	if(!ifname)
		return;

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));

	if(rtk_wlan_get_mib_index_mapping_by_ifname(ifname, &wlanIdx, &vwlan_idx) != 0){
		printf("can not find correct wlan interface!");
		return;
	}

	if (!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIdx, vwlan_idx, (void *)&Entry)){
		printf("mib_chain get error\n");
		return;
	}

#ifdef WLAN_UNIVERSAL_REPEATER
	mib_local_mapping_get(MIB_REPEATER_ENABLED1, wlanIdx, (void *)&rpt_enabled);
#endif

	if(Entry.wlanMode == CLIENT_MODE
#ifdef WLAN_UNIVERSAL_REPEATER
		|| rpt_enabled
#endif
	)
	{
		snprintf(filename, sizeof(filename), "%s.%s", WPAS_CONF_NAME, ifname);
		fp = fopen(filename, "r");

		if(!fp){
			printf("[%s]open %s fail\n", __FUNCTION__, filename);
			return;
		}

		while (fgets(line, sizeof(line), fp))
		{
			if(strstr(line,"network={"))
				network_num++;

			if(network_num == 2)
			{
				restart_flag = 1;
				while ( fgets(line, sizeof(line), fp) )
				{
					ptr = (char *)_get_token(line, token);

					if (ptr == NULL)
						continue;
					if (_get_value(ptr, value)==0)
						continue;
					printf("%s=%s\n", token, value);
					if(!strcmp(token, "	ssid"))
					{
						memset(Entry.ssid, 0, sizeof(Entry.ssid));
						strncpy(Entry.ssid, value, sizeof(Entry.ssid)-1);
					}
					else if(!strcmp(token, "	psk"))
					{
						strncpy(Entry.wpaPSK, value, sizeof(Entry.wpaPSK));
						strncpy(Entry.wscPsk, value, sizeof(Entry.wscPsk));
						if (strlen(value) == 64)
							Entry.wpaPSKFormat = 1;
						else
							Entry.wpaPSKFormat = 0;
					}
					else if(!strcmp(token, "	pairwise"))
					{
						Entry.unicastCipher = 0;
						Entry.wpa2UnicastCipher = 0;

						if(strstr(value,"TKIP") && strstr(value,"CCMP"))
							tmp_wpa_pairwise = WPA_CIPHER_MIXED;
						else if(strstr(value,"TKIP"))
							tmp_wpa_pairwise = WPA_CIPHER_TKIP;
						else if(strstr(value,"CCMP"))
							tmp_wpa_pairwise = WPA_CIPHER_AES;
					}
					else if(!strcmp(token, "	key_mgmt"))
					{
						if(!strcmp(value, "WPA-PSK"))
						{
							tmp_wpa_key_mgmt = 1;
							Entry.wpaAuth = WPA_AUTH_PSK;
#ifdef WLAN_11W
							Entry.sha256 = 1;
							Entry.dotIEEE80211W = 1;
#endif
						}
						else if(!strcmp(value, "WPA-PSK-SHA256"))
						{
							Entry.encrypt = WIFI_SEC_WPA2;
#ifdef WLAN_11W
							Entry.sha256 = 1;
#endif
							Entry.wpaAuth = WPA_AUTH_PSK;
						}
						else if(!strcmp(value, "NONE"))
						{
							Entry.encrypt = WIFI_SEC_NONE;
							Entry.wsc_auth = WSC_AUTH_OPEN;
							Entry.wsc_enc = WSC_ENCRYPT_NONE;
						}
					}
					else if(!strcmp(token, "	proto"))
					{
						if(strstr(value,"WPA") && strstr(value,"RSN"))
							tmp_wpa = 3;
						else if(strstr(value,"RSN"))
							tmp_wpa = 2;
						else if(strstr(value,"WPA"))
							tmp_wpa = 1;
					}
					else if(!strcmp(token, "	auth_alg"))
					{
						if(strstr(value,"SHARED"))
							Entry.authType = 1;
					}
				}

				break;
			}

		}

		fclose(fp);

		if(restart_flag)
		{
			if(tmp_wpa_pairwise == WPA_CIPHER_MIXED)
			{
				Entry.unicastCipher = 3;
				Entry.wpa2UnicastCipher = 3;
			}
			else if(tmp_wpa_pairwise == WPA_CIPHER_AES)
			{
				Entry.wpa2UnicastCipher = 2;
			}
			else if(tmp_wpa_pairwise == WPA_CIPHER_TKIP)
			{
				Entry.unicastCipher = 1;
			}

			if(tmp_wpa_key_mgmt == 1)
			{
				if(tmp_wpa == 3)
				{
					Entry.encrypt = WIFI_SEC_WPA2_MIXED;
					Entry.wsc_auth = WSC_AUTH_WPA2PSKMIXED;
					Entry.wsc_enc = WSC_ENCRYPT_TKIPAES;
				}
				else if(tmp_wpa == 2)
				{
					Entry.encrypt = WIFI_SEC_WPA2;
					Entry.wsc_auth = WSC_AUTH_WPA2PSK;
					Entry.wsc_enc = WSC_ENCRYPT_AES;
				}
				else if(tmp_wpa == 1)
				{
					Entry.encrypt = WIFI_SEC_WPA;
					Entry.wsc_auth = WSC_AUTH_WPAPSK;
					Entry.wsc_enc = WSC_ENCRYPT_TKIP;
				}
			}

			Entry.wsc_configured = 1;

			if (!mib_chain_local_mapping_update(MIB_MBSSIB_TBL, wlanIdx, (void *)&Entry, vwlan_idx)){
				printf("mib_chain update error\n");
				return;
			}

			if(vwlan_idx == 0){

				mib_local_mapping_set(MIB_WLAN_SSID, wlanIdx, Entry.ssid);
				mib_local_mapping_set(MIB_WLAN_ENCRYPT, wlanIdx, &Entry.encrypt);
				mib_local_mapping_set(MIB_WLAN_AUTH_TYPE, wlanIdx, &Entry.authType);
				mib_local_mapping_set(MIB_WSC_AUTH, wlanIdx, &Entry.wsc_auth);
				mib_local_mapping_set(MIB_WLAN_WPA_AUTH, wlanIdx, &Entry.wpaAuth);
				mib_local_mapping_set(MIB_WLAN_WPA_PSK_FORMAT, wlanIdx, &Entry.wpaPSKFormat);
				mib_local_mapping_set(MIB_WLAN_WPA_PSK, wlanIdx, Entry.wpaPSK);
				mib_local_mapping_set(MIB_WSC_PSK, wlanIdx, Entry.wscPsk);
				mib_local_mapping_set(MIB_WLAN_WPA_CIPHER_SUITE, wlanIdx, &Entry.unicastCipher);
				mib_local_mapping_set(MIB_WLAN_WPA2_CIPHER_SUITE, wlanIdx, &Entry.wpa2UnicastCipher);
				mib_local_mapping_set(MIB_WSC_ENC, wlanIdx, &Entry.wsc_enc);
				mib_local_mapping_set(MIB_WSC_CONFIGURED, wlanIdx, &Entry.wsc_configured);

			}

			mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
			printf("wps wlan restart\n");
			pthreadWlanConfig(ACT_RESTART, CONFIG_SSID_ALL);
		}
	}
#endif //WLAN_CLIENT || WLAN_UNIVERSAL_REPEATER
}

/** WPS overlap detected in PBC mode */
#define WPS_EVENT_OVERLAP "WPS-OVERLAP-DETECTED"
/** WPS registration failed after M2/M2D */
#define WPS_EVENT_FAIL "WPS-FAIL"
/** WPS registration completed successfully */
#define WPS_EVENT_SUCCESS "WPS-SUCCESS"
/** WPS enrollment attempt timed out and was terminated */
#define WPS_EVENT_TIMEOUT "WPS-TIMEOUT"
/* PBC mode was activated */
#define WPS_EVENT_ACTIVE "WPS-PBC-ACTIVE"
/* PBC mode was disabled */
#define WPS_EVENT_DISABLE "WPS-PBC-DISABLE"

void rtk_wlan_store_wpas_wps_status(char *event_str, char *ifname)
{
#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
	MIB_CE_MBSSIB_T Entry;
	int wlanIdx=0, vwlan_idx=0;
	FILE *wpas_wps_stas;
#ifdef WLAN_UNIVERSAL_REPEATER
	unsigned char rpt_enabled=0;
#endif

	if(!event_str || !ifname)
		return;

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));

	if(rtk_wlan_get_mib_index_mapping_by_ifname(ifname, &wlanIdx, &vwlan_idx) != 0){
		printf("can not find correct wlan interface!");
		return;
	}

	if (!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIdx, vwlan_idx, (void *)&Entry)){
		printf("mib_chain get error\n");
		return;
	}

#ifdef WLAN_UNIVERSAL_REPEATER
	mib_local_mapping_get(MIB_REPEATER_ENABLED1, wlanIdx, (void *)&rpt_enabled);
#endif

	if(Entry.wlanMode == CLIENT_MODE
#ifdef WLAN_UNIVERSAL_REPEATER
		|| rpt_enabled
#endif
	)
	{
		wpas_wps_stas = fopen("/var/wpas_wps_stas", "w+");

		if(wpas_wps_stas == NULL){
			printf("!! Error,  [%s][%d] wpas_wps_stas = NULL \n", __FUNCTION__, __LINE__);
			return;
		}

		if(!strcmp(event_str, WPS_EVENT_ACTIVE))
		{
			fprintf(wpas_wps_stas, "%s", "PBC Status: Active\n");
			fprintf(wpas_wps_stas, "%s", "Last WPS result: None\n");
		}
		else if(!strcmp(event_str, WPS_EVENT_TIMEOUT))
		{
			fprintf(wpas_wps_stas, "%s", "PBC Status: Timed-out\n");
			fprintf(wpas_wps_stas, "%s", "Last WPS result: None\n");
		}
		else if(!strcmp(event_str, WPS_EVENT_OVERLAP))
		{
			fprintf(wpas_wps_stas, "%s", "PBC Status: Overlap\n");
			fprintf(wpas_wps_stas, "%s", "Last WPS result: None\n");
		}
		else if(!strcmp(event_str, WPS_EVENT_FAIL))
		{
			fprintf(wpas_wps_stas, "%s", "PBC Status: Disabled\n");
			fprintf(wpas_wps_stas, "%s", "Last WPS result: Failed\n");
		}
		else if(!strcmp(event_str, WPS_EVENT_SUCCESS))
		{
			fprintf(wpas_wps_stas, "%s", "PBC Status: Disabled\n");
			fprintf(wpas_wps_stas, "%s", "Last WPS result: Success\n");
		}
		else
		{
			fprintf(wpas_wps_stas, "%s", "PBC Status: Disabled\n");
			fprintf(wpas_wps_stas, "%s", "Last WPS result: None\n");
		}

		fclose(wpas_wps_stas);
	}
#endif //WLAN_CLIENT || WLAN_UNIVERSAL_REPEATER
}
#endif //WLAN_WPS_WPAS

int rtk_wlan_setup_for_driver(char *ifname, unsigned int flags)
{
	int wlan_index = 0, vwlan_idx = 0;
	int status = 0;
	static int wlan_status[NUM_WLAN_INTERFACE][NUM_VWLAN_INTERFACE+1]={0}; //status 0: never assigned, 1: up, -1: down

	if(((flags & IFF_UP) && (flags & IFF_RUNNING)) || !(flags & IFF_UP))
		;
	else
		return 0;

	if(rtk_wlan_get_mib_index_mapping_by_ifname(ifname, &wlan_index, &vwlan_idx)==0)
	{
		if((flags & IFF_UP) && (flags & IFF_RUNNING)){
			if(wlan_status[wlan_index][vwlan_idx] != 1){
				if(vwlan_idx == WLAN_ROOT_ITF_INDEX){
					setupWlanRootMib(wlan_index);
					rtk_wlan_channel_change_event(ifname);
				}
				status |= setupWLan(wlan_index, vwlan_idx);
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
				setup_wlan_MAC_ACL(wlan_index, vwlan_idx);
#endif
				wlan_status[wlan_index][vwlan_idx] = 1;
			}
		}
		else if(!(flags & IFF_UP)){
			if(wlan_status[wlan_index][vwlan_idx] != -1){
				wlan_status[wlan_index][vwlan_idx] = -1;
			}
		}
	}

	return status;
}

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

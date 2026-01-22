/*
 *      Web server handler routines for wireless status
 *
 */

#include <string.h>
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "debug.h"

static void getEncryption(MIB_CE_MBSSIB_T *Entry, char *buffer)
{
	switch (Entry->encrypt) {
		case WIFI_SEC_WEP:
			if (Entry->wep == WEP_DISABLED)
				strcpy(buffer, "Disabled");
			else if (Entry->wep == WEP64 )
				strcpy(buffer, "WEP 64bits");
			else if (Entry->wep == WEP128)
				strcpy(buffer, "WEP 128bits");
			else
				buffer[0] = '\0';
			break;
		case WIFI_SEC_NONE:
		case WIFI_SEC_WPA:
			strcpy(buffer, wlan_encrypt[Entry->encrypt]);
			break;
		case WIFI_SEC_WPA2:
			strcpy(buffer, wlan_encrypt[3]);
			break;
		case WIFI_SEC_WPA2_MIXED:
			strcpy(buffer, wlan_encrypt[4]);
			break;
#ifdef CONFIG_RTL_WAPI_SUPPORT
		case WIFI_SEC_WAPI:
			strcpy(buffer, wlan_encrypt[5]);
			break;
#endif
#ifdef WLAN_WPA3
		case WIFI_SEC_WPA3:
			strcpy(buffer, wlan_encrypt[6]);
			break;
		case WIFI_SEC_WPA2_WPA3_MIXED:
			strcpy(buffer, wlan_encrypt[7]);
			break;	
#endif
		default:
			strcpy(buffer, wlan_encrypt[0]);
	}
}

static void getTranSSID(char *buff, char *ssid)
{
	memset(buff, '\0', 200);
	memcpy(buff, ssid, MAX_SSID_LEN);
	translate_control_code(buff);
}

static void getEncryptionFromDriver(char *interface_name, MIB_CE_MBSSIB_T *Entry, char *buffer)
{
	char encmode;
	int wpa_chiper, wpa2_chiper, psk_enable;
	char mib_name[30];
	sprintf(mib_name, "encmode");
	getWlEnc(interface_name, mib_name, &encmode);
	if(encmode == WIFI_SEC_NONE)
		strcpy(buffer, wlan_encrypt[0]);
	else if(encmode == WIFI_SEC_WPA || encmode == WIFI_SEC_WPA2){
		sprintf(mib_name, "wpa_cipher");
		getWlWpaChiper(interface_name, mib_name, &wpa_chiper);
		sprintf(mib_name, "wpa2_cipher");
		getWlWpaChiper(interface_name, mib_name, &wpa2_chiper);
		if(wpa2_chiper==0)
			strcpy(buffer, wlan_encrypt[2]);
		else if(wpa_chiper==0)
			strcpy(buffer, wlan_encrypt[3]);
		else
			strcpy(buffer, wlan_encrypt[4]);
#ifdef WLAN_WPA3
		memset(mib_name,0,sizeof(mib_name));
		sprintf(mib_name, "psk_enable");
		getWlWpaChiper(interface_name, mib_name, &psk_enable);
		if(psk_enable == 10 || psk_enable == 8)
			strcpy(buffer, wlan_encrypt[6]);
#endif
	}
	else if(encmode == 1 || encmode == 5) 
		strcpy(buffer, "WEP");
	else
		buffer[0] = '\0';

}

#ifdef WLAN_QTN
int qtn_wlstatus(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0, i, k;
	MIB_CE_MBSSIB_T Entry, Entry2;
	char vChar, vChar2, vChar3, wlan_disabled;
	int vInt, vInt2;
	bss_info bss;
	unsigned char buffer[64], buffer2[64];
	unsigned char translate_ssid[200];
	int wlan_num=1;
	unsigned char wlband=0;

	for (i=0; i<wlan_num; i++) {
		mib_chain_get(MIB_MBSSIB_TBL, 0, &Entry);
#ifdef WLAN_BAND_CONFIG_MBSSID
		vChar = Entry.wlanBand;
#else
		mib_get_s(MIB_WLAN_BAND, (void *)&wlband, sizeof(wlband));
#endif
		if ( rt_qcsapi_get_bss_info(getWlanIfName(), &bss) < 0){
			//wlan_idx = orig_wlan_idx;
			return -1;
		}	
		getTranSSID(translate_ssid, bss.ssid);
		nBytesSent += boaWrite(wp,
			"wlanMode[%d]=%d;\n\tnetworkType[%d]=%d;\n"
			"\tband[%d]=%d;\n\tssid_drv[%d]='%s';\n",
			i, Entry.wlanMode, i, 0, i, wlband, i, translate_ssid);
		/* Encryption */
		if((Entry.wlanMode == AP_MODE) || (Entry.wlanMode == AP_WDS_MODE))
			getEncryption(&Entry, buffer);
		#ifdef WLAN_CLIENT
		else
		//	getEncryptionFromDriver(getWlanIfName(), &Entry, buffer);
			rt_get_SSID_encryption(getWlanIfName(), Entry.ssid, buffer);
		#endif
			
		/* WDS encryption */
		#ifdef WLAN_WDS
		mib_get_s(MIB_WLAN_WDS_ENCRYPT, (void *)&vChar, sizeof(vChar));
		#else
		vChar = WEP_DISABLED;
		#endif
		if (vChar == WEP_DISABLED)
			strcpy( buffer2, "Disabled" );
		else if (vChar == WEP64)
			strcpy( buffer2, "WEP 64bits" );
		else if (vChar == WEP128)
			strcpy( buffer2, "WEP 128bits" );
		else
			buffer2[0]='\0';
		nBytesSent += boaWrite(wp,
			"\tchannel_drv[%d]='%d';\n\twep[%d]='%s';\n"
			"\twdsEncrypt[%d]='%s';\n\tmeshEncrypt[%d]='';\n",
			i, bss.channel, i, buffer, i, buffer2, i, "Disabled");
		nBytesSent += boaWrite(wp,
			"\tbssid_drv[%d]='%02x:%02x:%02x:%02x:%02x:%02x';\n",
			i, bss.bssid[0], bss.bssid[1], bss.bssid[2],
			bss.bssid[3], bss.bssid[4], bss.bssid[5]);
		/* client number */
		wlan_disabled = Entry.wlanDisabled;
		if (wlan_disabled == 1) // disabled
			vInt = 0;
		else {
			if ( rt_qcsapi_get_sta_num(getWlanIfName(), &vInt) < 0)
				vInt = 0;
		}
		/* state */
		switch (bss.state) {
		case STATE_DISABLED:
			strcpy( buffer, "Disabled");
			break;
		case STATE_IDLE:
			strcpy( buffer, "Idle");
			break;
		case STATE_STARTED:
			strcpy( buffer, "Started");
			break;
		case STATE_CONNECTED:
			strcpy( buffer, "Connected");
			break;
		case STATE_WAITFORKEY:
			strcpy( buffer, "Waiting for keys");
			break;
		case STATE_SCANNING:
			strcpy( buffer, "Scanning");
			break;
		default:
			buffer[0]='\0';;
		}
		#ifdef WLAN_UNIVERSAL_REPEATER
		/* Is Repeater enabled ? */
		mib_get_s(MIB_REPEATER_ENABLED1, (void *)&vChar2, sizeof(vChar2));
		mib_get_s(MIB_WLAN_NETWORK_TYPE, (void *)&vChar3, sizeof(vChar3));
		if (vChar2 != 0 && Entry.wlanMode != WDS_MODE &&
			!(Entry.wlanMode==CLIENT_MODE && vChar3==ADHOC))
			vInt2 = 1;
		else
		#endif
			vInt2 = 0;
		nBytesSent += boaWrite(wp,
			"\tclientnum[%d]='%d';\n\tstate_drv[%d]='%s';\n"
			"\twlanDisabled[%d]=%d;\n\trp_enabled[%d]=%d;\n",
			i, vInt, i, buffer, i, wlan_disabled, i, vInt2);
		#ifdef WLAN_UNIVERSAL_REPEATER
		/*------- Repeater Interface -------*/
		mib_chain_get(MIB_MBSSIB_TBL, WLAN_REPEATER_ITF_INDEX, (void *)&Entry2);
		/* Repeater mode */
		if (Entry.wlanMode == AP_MODE || Entry.wlanMode == AP_WDS_MODE)
			vInt = CLIENT_MODE;
		else
			vInt = AP_MODE;
		/* Repeater encryption */
		sprintf(buffer2, VXD_IF, wlan_idx);
		if(Entry.wlanMode == CLIENT_MODE)
			getEncryption(&Entry2, buffer);
		else
			getEncryptionFromDriver(buffer2, &Entry2, buffer);
		if (rt_qcsapi_get_bss_info(buffer2, &bss)<0)
			printf("getWlBssInfo failed\n");
		getTranSSID(translate_ssid, bss.ssid);
		nBytesSent += boaWrite(wp,
			"\trp_mode[%d]=%d;\n\trp_encrypt[%d]='%s';\n"
			"\trp_ssid[%d]='%s';\n",
			i, vInt, i, buffer, i, translate_ssid);
		/* Repeater bssid */
		nBytesSent += boaWrite(wp,
			"\trp_bssid[%d]='%02x:%02x:%02x:%02x:%02x:%02x';\n",
			i, bss.bssid[0], bss.bssid[1], bss.bssid[2],
			bss.bssid[3], bss.bssid[4], bss.bssid[5]);
		/* Repeater state */
		switch (bss.state) {
		case STATE_DISABLED:
			strcpy( buffer, "Disabled");
			break;
		case STATE_IDLE:
			strcpy( buffer, "Idle");
			break;
		case STATE_STARTED:
			strcpy( buffer, "Started");
			break;
		case STATE_CONNECTED:
			strcpy( buffer, "Connected");
			break;
		case STATE_WAITFORKEY:
			strcpy( buffer, "Waiting for keys");
			break;
		case STATE_SCANNING:
			strcpy( buffer, "Scanning");
			break;
		default:
			buffer[0]='\0';;
		}
		/* Repeater client number */
		rt_qcsapi_get_sta_num(buffer2, &vInt);
		nBytesSent += boaWrite(wp,
			"\trp_state[%d]='%s';\n\trp_clientnum[%d]='%d';\n",
			i, buffer, i, vInt);
		#endif // of WLAN_UNIVERSAL_REPEATER
		#ifdef WLAN_MBSSID
		nBytesSent += boaWrite(wp,
			"\tmssid_num=%d;\n", WLAN_MBSSID_NUM);
		/*-------------- VAP Interface ------------*/
		for (k=0; k<WLAN_MBSSID_NUM; k++) {
			//wlan_idx = orig_wlan_idx;
			mib_chain_get(MIB_MBSSIB_TBL, WLAN_VAP_ITF_INDEX+k, (void *)&Entry2);
#ifdef WLAN_BAND_CONFIG_MBSSID
			wlband = Entry2.wlanBand;
#endif
			if(Entry2.wlanDisabled){
				nBytesSent += boaWrite(wp,
					"\tmssid_disable[%d]=%d;\n",
					k, Entry2.wlanDisabled);
			}
			else{
				sprintf(buffer, "%s-vap%d", getWlanIfName(), k);
				if (rt_qcsapi_get_bss_info(buffer, &bss)<0)
					printf("getWlBssInfo failed\n");
				getTranSSID(translate_ssid, bss.ssid);
				nBytesSent += boaWrite(wp,
					"\tmssid_ssid_drv[%d]='%s';\n\tmssid_band[%d]=%d;\n"
					"\tmssid_disable[%d]=%d;\n",
					k, translate_ssid, k, wlband, k, Entry2.wlanDisabled);
				nBytesSent += boaWrite(wp,
					"\tmssid_bssid_drv[%d]='%02x:%02x:%02x:%02x:%02x:%02x';\n",
					k, bss.bssid[0], bss.bssid[1], bss.bssid[2],
					bss.bssid[3], bss.bssid[4], bss.bssid[5]);
				/* VAP client number */
				rt_qcsapi_get_sta_num(buffer, &vInt);
				/* VAP encryption */
				getEncryption(&Entry2, buffer2);
				nBytesSent += boaWrite(wp,
					"\tmssid_clientnum[%d]='%d';\n\tmssid_wep[%d]='%s';\n",
					k, vInt, k, buffer2);
			}
		}
		#else // of WLAN_MBSSID
		nBytesSent += boaWrite(wp,
			"\tmssid_num=0;\n");
		#endif
		//wlan_idx = orig_wlan_idx;
#if defined(WLAN0_5G_SUPPORT) || defined(WLAN1_5G_SUPPORT)
		mib_get_s( MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
#else
		vChar = PHYBAND_2G;
#endif
		nBytesSent += boaWrite(wp, "\tBand2G5GSupport=%d;\n", vChar);
	}
//	wlan_idx = orig_wlan_idx;
	return nBytesSent;

}
#endif

int getWlMeshEncryption(char *buffer)
{
#ifdef WLAN_MESH
	unsigned char encrypt;
	if ( !mib_get_s( MIB_WLAN_MESH_ENCRYPT,  (void *)&encrypt, sizeof(encrypt)) )
		return -1;
	if ( encrypt == WIFI_SEC_NONE)
		strcpy( buffer, "Disabled");
	else if ( encrypt == WIFI_SEC_WPA2)
		strcpy( buffer, "WPA2");
	else
#endif
		buffer[0] = '\0';

	return 0;
}

int wlStatus_parm(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0, i, k;
	MIB_CE_MBSSIB_T Entry, Entry2;
	unsigned char vChar, vChar2, vChar3, wlan_disabled;
	int vInt, vInt2;
	bss_info bss;
	unsigned char buffer[64], buffer2[64], buffer3[64];
	char translate_ssid[200] = {0};
	int wlan_num=1;
#ifdef CONFIG_00R0
	unsigned char auto_channel;
	unsigned char chan;
#endif
	unsigned char wlband = 0;
	//int orig_wlan_idx;
	//orig_wlan_idx = wlan_idx;

#ifdef WLAN_QTN
#ifdef WLAN1_QTN
	if(wlan_idx==1)
#else
	if(wlan_idx==0)
#endif
		return qtn_wlstatus(eid, wp, argc, argv);
#endif
	
	for (i=0; i<wlan_num; i++) {
		if(!mib_chain_get(MIB_MBSSIB_TBL, 0, &Entry))
			printf("Error! Get MIB_MBSSIB_TBL(wlStatus_parm) error.\n");
#ifdef WLAN_BAND_CONFIG_MBSSID
		wlband = Entry.wlanBand;
#else
		mib_get_s(MIB_WLAN_BAND, (void *)&wlband, sizeof(wlband));
#endif
#ifdef CONFIG_00R0
		mib_get_s(MIB_WLAN_AUTO_CHAN_ENABLED, &auto_channel, sizeof(auto_channel));
#endif
		if ( getWlBssInfo(getWlanIfName(), &bss) < 0){
			//wlan_idx = orig_wlan_idx;
			return -1;
		}
		snprintf(translate_ssid, sizeof(translate_ssid), "%s", bss.ssid);
		translate_web_code(translate_ssid);
		nBytesSent += boaWrite(wp,
			"wlanMode[%d]=%d;\n\tnetworkType[%d]=%d;\n"
			"\tband[%d]=%d;\n\tssid_drv[%d]='%s';\n",
			i, Entry.wlanMode, i, 0, i, wlband, i, translate_ssid);

#ifdef WLAN_6G_SUPPORT
		mib_get_s(MIB_WLAN_6G_SUPPORT, (void *)&vChar, sizeof(vChar));
		nBytesSent += boaWrite(wp, "\tband6g_enable[%d]=%d;\n", i, vChar);
#else
		nBytesSent += boaWrite(wp, "\tband6g_enable[%d]=0;\n", i);
#endif
		/* Encryption */
		if((Entry.wlanMode == AP_MODE) || (Entry.wlanMode == AP_WDS_MODE))
			getEncryption(&Entry, buffer);
		else
			getEncryptionFromDriver(getWlanIfName(), &Entry, buffer);
			
		/* WDS encryption */
		#ifdef WLAN_WDS
		mib_get_s(MIB_WLAN_WDS_ENCRYPT, (void *)&vChar, sizeof(vChar));
		if (vChar == WEP_DISABLED)
			strcpy( buffer2, "Disabled" );
		else if (vChar == WEP64)
			strcpy( buffer2, "WEP 64bits" );
		else if (vChar == WEP128)
			strcpy( buffer2, "WEP 128bits" );
		#ifdef CONFIG_RTK_DEV_AP
        else if(vChar == WIFI_SEC_WPA2)
           strcpy( buffer2, "WPA2" );      
		#endif
		else
			buffer2[0]='\0';
		#else
		vChar = WEP_DISABLED;
		strcpy( buffer2, "Disabled" );
		#endif

		getWlMeshEncryption(buffer3);
#ifdef CONFIG_00R0
		if(auto_channel){
			nBytesSent += boaWrite(wp,
				"\tchannel_drv[%d]='%d';\n\twep[%d]='%s';\n"
				"\twdsEncrypt[%d]='%s';\n\tmeshEncrypt[%d]='';\n",
				i, bss.channel, i, buffer, i, buffer2, i, "Disabled");
		}
		else{
			mib_get_s(MIB_WLAN_CHAN_NUM, &chan, sizeof(chan));
			nBytesSent += boaWrite(wp,
				"\tchannel_drv[%d]='%d';\n\twep[%d]='%s';\n"
				"\twdsEncrypt[%d]='%s';\n\tmeshEncrypt[%d]='';\n",
				i, chan, i, buffer, i, buffer2, i, "Disabled");
		}
#else
		nBytesSent += boaWrite(wp,
			"\tchannel_drv[%d]='%d';\n\twep[%d]='%s';\n"
			"\twdsEncrypt[%d]='%s';\n\tmeshEncrypt[%d]='%s';\n",
			i, bss.channel, i, buffer, i, buffer2, i, buffer3);
#endif
		nBytesSent += boaWrite(wp,
			"\tbssid_drv[%d]='%02x:%02x:%02x:%02x:%02x:%02x';\n",
			i, bss.bssid[0], bss.bssid[1], bss.bssid[2],
			bss.bssid[3], bss.bssid[4], bss.bssid[5]);
		/* client number */
		wlan_disabled = Entry.wlanDisabled;
		if (wlan_disabled == 1) // disabled
			vInt = 0;
		else {
			if ( getWlStaNum(getWlanIfName(), &vInt) < 0)
				vInt = 0;
		}
		/* state */
		switch (bss.state) {
		case STATE_DISABLED:
			strcpy( buffer, "Disabled");
			break;
		case STATE_IDLE:
			strcpy( buffer, "Idle");
			break;
		case STATE_STARTED:
			strcpy( buffer, "Started");
			break;
		case STATE_CONNECTED:
			strcpy( buffer, "Connected");
			break;
		case STATE_WAITFORKEY:
			strcpy( buffer, "Waiting for keys");
			break;
		case STATE_SCANNING:
			strcpy( buffer, "Scanning");
			break;
		default:
			buffer[0]='\0';;
		}
		#ifdef WLAN_UNIVERSAL_REPEATER
		/* Is Repeater enabled ? */
		mib_get_s(MIB_REPEATER_ENABLED1, (void *)&vChar2, sizeof(vChar2));
		mib_get_s(MIB_WLAN_NETWORK_TYPE, (void *)&vChar3, sizeof(vChar3));
		if (vChar2 != 0 && Entry.wlanMode != WDS_MODE &&
			!(Entry.wlanMode==CLIENT_MODE && vChar3==ADHOC))
			vInt2 = 1;
		else
		#endif
			vInt2 = 0;
		nBytesSent += boaWrite(wp,
			"\tclientnum[%d]='%d';\n\tstate_drv[%d]='%s';\n"
			"\twlanDisabled[%d]=%d;\n\trp_enabled[%d]=%d;\n",
			i, vInt, i, buffer, i, wlan_disabled, i, vInt2);
		#ifdef WLAN_UNIVERSAL_REPEATER
		/*------- Repeater Interface -------*/
		if(!mib_chain_get(MIB_MBSSIB_TBL, WLAN_REPEATER_ITF_INDEX, (void *)&Entry2))
			printf("Error! Get MIB_MBSSIB_TBL(wlStatus_parm) error.\n");
		/* Repeater mode */
		if (Entry.wlanMode == AP_MODE || Entry.wlanMode == AP_WDS_MODE)
			vInt = CLIENT_MODE;
		else
			vInt = AP_MODE;
		/* Repeater encryption */
		sprintf(buffer2, VXD_IF, wlan_idx);
		if(Entry.wlanMode == CLIENT_MODE)
			getEncryption(&Entry2, buffer);
		else
			getEncryptionFromDriver(buffer2, &Entry2, buffer);
		if ( getInFlags(buffer2, 0) && (getWlBssInfo(buffer2, &bss)<0))
			printf("getWlBssInfo failed\n");
		strcpy(translate_ssid, bss.ssid);
		translate_web_code(translate_ssid);
		nBytesSent += boaWrite(wp,
			"\trp_mode[%d]=%d;\n\trp_encrypt[%d]='%s';\n"
			"\trp_ssid[%d]='%s';\n",
			i, vInt, i, buffer, i, translate_ssid);
		/* Repeater bssid */
		nBytesSent += boaWrite(wp,
			"\trp_bssid[%d]='%02x:%02x:%02x:%02x:%02x:%02x';\n",
			i, bss.bssid[0], bss.bssid[1], bss.bssid[2],
			bss.bssid[3], bss.bssid[4], bss.bssid[5]);
		/* Repeater state */
		switch (bss.state) {
		case STATE_DISABLED:
			strcpy( buffer, "Disabled");
			break;
		case STATE_IDLE:
			strcpy( buffer, "Idle");
			break;
		case STATE_STARTED:
			strcpy( buffer, "Started");
			break;
		case STATE_CONNECTED:
			strcpy( buffer, "Connected");
			break;
		case STATE_WAITFORKEY:
			strcpy( buffer, "Waiting for keys");
			break;
		case STATE_SCANNING:
			strcpy( buffer, "Scanning");
			break;
		default:
			buffer[0]='\0';;
		}
		/* Repeater client number */
		getWlStaNum(buffer2, &vInt);
		nBytesSent += boaWrite(wp,
			"\trp_state[%d]='%s';\n\trp_clientnum[%d]='%d';\n",
			i, buffer, i, vInt);
		#endif // of WLAN_UNIVERSAL_REPEATER
		#ifdef WLAN_MBSSID
		nBytesSent += boaWrite(wp,
			"\tmssid_num=%d;\n", WLAN_MBSSID_NUM);
		/*-------------- VAP Interface ------------*/
		for (k=0; k<WLAN_MBSSID_NUM; k++) {
			//wlan_idx = orig_wlan_idx;
			if(!mib_chain_get(MIB_MBSSIB_TBL, WLAN_VAP_ITF_INDEX+k, (void *)&Entry2))
				printf("Error! Get MIB_MBSSIB_TBL(wlStatus_parm) error.\n");
#ifdef WLAN_BAND_CONFIG_MBSSID
			wlband = Entry2.wlanBand;
#endif
			if(Entry2.wlanDisabled){
				nBytesSent += boaWrite(wp,
					"\tmssid_disable[%d]=%d;\n",
					k, Entry2.wlanDisabled);
			}
			else{
				sprintf(buffer, "%s-vap%d", getWlanIfName(), k);
			if (getInFlags(buffer, 0) && getWlBssInfo(buffer, &bss)<0)
					printf("getWlBssInfo failed\n");
				strcpy(translate_ssid, bss.ssid);
				translate_web_code(translate_ssid);
				nBytesSent += boaWrite(wp,
					"\tmssid_ssid_drv[%d]='%s';\n\tmssid_band[%d]=%d;\n"
					"\tmssid_disable[%d]=%d;\n",
					k, translate_ssid, k, wlband, k, Entry2.wlanDisabled);
				nBytesSent += boaWrite(wp,
					"\tmssid_bssid_drv[%d]='%02x:%02x:%02x:%02x:%02x:%02x';\n",
					k, bss.bssid[0], bss.bssid[1], bss.bssid[2],
					bss.bssid[3], bss.bssid[4], bss.bssid[5]);
				/* VAP client number */
			getWlStaNum(buffer, &vInt);
				/* VAP encryption */
				getEncryption(&Entry2, buffer2);
				nBytesSent += boaWrite(wp,
					"\tmssid_clientnum[%d]='%d';\n\tmssid_wep[%d]='%s';\n",
					k, vInt, k, buffer2);
			}
		}
		#else // of WLAN_MBSSID
		nBytesSent += boaWrite(wp,
			"\tmssid_num=0;\n");
		#endif
		//wlan_idx = orig_wlan_idx;
#if defined(WLAN0_5G_SUPPORT) || defined(WLAN1_5G_SUPPORT)
		mib_get_s( MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
#else
		vChar = PHYBAND_2G;
#endif
		nBytesSent += boaWrite(wp, "\tBand2G5GSupport=%d;\n", vChar);
	}
	//	wlan_idx = orig_wlan_idx;
	return nBytesSent;
}

#ifdef CONFIG_00R0
int wlanActiveConfList(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
#ifdef WLAN_MBSSID
	int i=0, j=1, orig_wlan_idx;
	unsigned int sta_num=0;
	unsigned char channel=0, chanWidth=0, wlan11nCoexist=0;
	unsigned char channelWidth[16]={0}, wlanEncryptStr[64]={0}, wlanBand[8]={0}, ifname[16]={0};
	MIB_CE_MBSSIB_T Entry;
	char *buff;
	WLAN_STA_INFO_Tp pInfo;
	bss_info info = {0};
	char buf[8] = {0};
	unsigned char auto_chan;

	nBytesSent += boaWrite(wp, "<tr>"
			"<th width=\"8%%\" align=center>%s</th>\n"
			"<th width=\"8%%\" align=center>%s</th>\n"
			"<th width=\"8%%\" align=center>%s</th>\n"
			"<th width=\"8%%\" align=center>%s</th>\n"
			"<th width=\"8%%\" align=center>%s</th>\n"
			"<th width=\"8%%\" align=center>%s</th>\n"
			"<th width=\"8%%\" align=center>%s</th>\n",
			multilang(LANG_SSID),
			multilang(LANG_BAND),
			multilang(LANG_CHANNEL),
			multilang(LANG_BANDWIDTH),
			multilang(LANG_ENCRYPTION),
			multilang(LANG_STANDARDS),
			multilang(LANG_CLIENTS));
	nBytesSent += boaWrite(wp, "<tr>\n");

	orig_wlan_idx = wlan_idx;
	wlan_idx=0;

	mib_get(MIB_WLAN_AUTO_CHAN_ENABLED, (void *)&auto_chan);
	getWlBssInfo("wlan0", &info);
	if(info.state == STATE_DISABLED || auto_chan == 0)
		mib_get(MIB_WLAN_CHAN_NUM, (void *)&channel);
	else
		channel = info.channel;

	mib_get(MIB_WLAN_CHANNEL_WIDTH, (void *)&chanWidth);
	mib_get(MIB_WLAN_11N_COEXIST, (void *)&wlan11nCoexist);

	if(chanWidth==0)
		strcpy(channelWidth, "20 MHz");
	else if(wlan11nCoexist==0)
		strcpy(channelWidth, "40 MHz");
	else
		strcpy(channelWidth, "20/40 MHz");

	buff = calloc(1, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));
	if ( buff == 0 ) {
		printf("Allocate buffer failed!\n");
		return 0;
	}

	for(i=0; i<=WLAN_MBSSID_NUM; i++)
	{
		if (!mib_chain_get(MIB_MBSSIB_TBL, i, &Entry))
			printf("get MIB chain error %s %d\n", __func__, __LINE__);

		if((i==0 && !Entry.func_off) || !Entry.wlanDisabled)
		{
			if( Entry.wlanBand==BAND_11B)
				strcpy(wlanBand, "b");
			else if (Entry.wlanBand==BAND_11G)
				strcpy(wlanBand, "g");
			else if (Entry.wlanBand==BAND_11BG)
				strcpy(wlanBand, "b/g");
			else if (Entry.wlanBand==BAND_11A)
				strcpy(wlanBand, "a");
			else if (Entry.wlanBand==BAND_11N)
				strcpy(wlanBand, "n");
			else if (Entry.wlanBand==(BAND_11N|BAND_11G))
				strcpy(wlanBand, "g/n");
			else if (Entry.wlanBand==(BAND_11B|BAND_11G|BAND_11N))
				strcpy(wlanBand, "b/g/n");
			else if (Entry.wlanBand==BAND_5G_11AN)
				strcpy(wlanBand, "a/n");
			else if (Entry.wlanBand==BAND_5G_11AC)
				strcpy(wlanBand, "ac");
			else if (Entry.wlanBand==BAND_5G_11NAC)
				strcpy(wlanBand, "n/ac");
			else if (Entry.wlanBand>=BAND_5G_11ANAC)
				strcpy(wlanBand, "a/n/ac");

			if((Entry.wlanMode == AP_MODE) || (Entry.wlanMode == AP_WDS_MODE))
				getEncryption(&Entry, wlanEncryptStr);
			else
				getEncryptionFromDriver(getWlanIfName(), &Entry, wlanEncryptStr);

			if(i) snprintf(ifname, 16, "%s-vap%d", "wlan0", i);
			else strcpy(ifname, "wlan0");

			if (getWlStaInfo(ifname, (WLAN_STA_INFO_Tp)buff) < 0)
				printf("Read wlan sta info failed!\n");
			else
			{
				for (j=1; j<=MAX_STA_NUM; j++) {
					pInfo = (WLAN_STA_INFO_Tp)&buff[j*sizeof(WLAN_STA_INFO_T)];
					if (pInfo->aid && (pInfo->flags & STA_INFO_FLAG_ASOC))
						sta_num++;
				}
			}

			nBytesSent += boaWrite(wp,
					"<td align=center width=\"8%%\">SSID%d</td>\n"
					"<td align=center width=\"8%%\">%s</td>\n"
					"<td align=center width=\"8%%\">%d</td>\n"
					"<td align=center width=\"8%%\">%s</td>\n"
					"<td align=center width=\"8%%\">%s</td>\n"
					"<td align=center width=\"8%%\">%s</td>\n"
					"<td align=center width=\"8%%\">%d\n",
					i+1,
					"2.4G",
					channel,
					channelWidth,
					wlanEncryptStr,
					wlanBand,
					sta_num);
			nBytesSent += boaWrite(wp, "</td></tr>\n");
			sta_num=0;
		}
	}

#ifdef WLAN_DUALBAND_CONCURRENT
	memset(channelWidth, 0, sizeof(channelWidth));
	memset(wlanEncryptStr, 0, sizeof(wlanEncryptStr));
	memset(wlanBand, 0, sizeof(wlanBand));

	mib_get(MIB_WLAN1_AUTO_CHAN_ENABLED, (void *)&auto_chan);
	getWlBssInfo("wlan1", &info);
	if(info.state == STATE_DISABLED || auto_chan == 0)
		mib_get(MIB_WLAN1_CHAN_NUM, (void *)&channel);
	else
		channel = info.channel;

	mib_get(MIB_WLAN1_CHANNEL_WIDTH, (void *)&chanWidth);
	mib_get(MIB_WLAN1_11N_COEXIST, (void *)&wlan11nCoexist);

	if(chanWidth==0)
		strcpy(channelWidth, "20 MHz");
	else if(chanWidth==1)
		strcpy(channelWidth, "40 MHz");
	else if(wlan11nCoexist==0)
		strcpy(channelWidth, "80 MHz");
	else
		strcpy(channelWidth, "20/40/80 MHz");

	for(i=0; i<=WLAN_MBSSID_NUM; i++)
	{
		if (!mib_chain_get(MIB_WLAN1_MBSSIB_TBL, i, &Entry))
			printf("get MIB chain error %s %d\n", __func__, __LINE__);

		if((i==0 && !Entry.func_off) || !Entry.wlanDisabled)
		{
			if( Entry.wlanBand==BAND_11B)
				strcpy(wlanBand, "b");
			else if (Entry.wlanBand==BAND_11G)
				strcpy(wlanBand, "g");
			else if (Entry.wlanBand==BAND_11BG)
				strcpy(wlanBand, "b/g");
			else if (Entry.wlanBand==BAND_11A)
				strcpy(wlanBand, "a");
			else if (Entry.wlanBand==BAND_11N)
				strcpy(wlanBand, "n");
			else if (Entry.wlanBand==(BAND_11N|BAND_11G))
				strcpy(wlanBand, "g/n");
			else if (Entry.wlanBand==(BAND_11B|BAND_11G|BAND_11N))
				strcpy(wlanBand, "b/g/n");
			else if (Entry.wlanBand==BAND_5G_11AN)
				strcpy(wlanBand, "a/n");
			else if (Entry.wlanBand==BAND_5G_11AC)
				strcpy(wlanBand, "ac");
			else if (Entry.wlanBand==BAND_5G_11NAC)
				strcpy(wlanBand, "n/ac");
			else if (Entry.wlanBand>=BAND_5G_11ANAC)
				strcpy(wlanBand, "a/n/ac");

			if((Entry.wlanMode == AP_MODE) || (Entry.wlanMode == AP_WDS_MODE))
				getEncryption(&Entry, wlanEncryptStr);
			else
				getEncryptionFromDriver(getWlanIfName(), &Entry, wlanEncryptStr);

			if(i) snprintf(ifname, 16, "%s-vap%d", "wlan1", i);
			else strcpy(ifname, "wlan1");

			if (getWlStaInfo(ifname, (WLAN_STA_INFO_Tp)buff) < 0)
				printf("Read wlan sta info failed!\n");
			else
			{
				for (j=1; j<=MAX_STA_NUM; j++) {
					pInfo = (WLAN_STA_INFO_Tp)&buff[j*sizeof(WLAN_STA_INFO_T)];
					if (pInfo->aid && (pInfo->flags & STA_INFO_FLAG_ASOC))
						sta_num++;
				}
			}

			nBytesSent += boaWrite(wp,
					"<td align=center width=\"8%%\">SSID%d</td>\n"
					"<td align=center width=\"8%%\">%s</td>\n"
					"<td align=center width=\"8%%\">%d</td>\n"
					"<td align=center width=\"8%%\">%s</td>\n"
					"<td align=center width=\"8%%\">%s</td>\n"
					"<td align=center width=\"8%%\">%s</td>\n"
					"<td align=center width=\"8%%\">%d\n",
					i+WLAN_MBSSID_NUM+2-1,           //ignore easy mesh SSID
					"5G",
					channel,
					channelWidth,
					wlanEncryptStr,
					wlanBand,
					sta_num);
			nBytesSent += boaWrite(wp, "</td></tr>\n");
			sta_num=0;
		}
	}
#endif
	if(buff)
		free(buff);
	wlan_idx=orig_wlan_idx;
#endif
	return nBytesSent;
}
#endif


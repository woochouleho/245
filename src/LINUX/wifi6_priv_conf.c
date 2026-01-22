#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "debug.h"
#include "utility.h"
#include <linux/wireless.h>

#include "wifi6_priv_conf.h"
#include "hwnat_ioctl.h"
#ifdef RTK_MULTI_AP
#include "subr_multiap.h"
#endif

// for QoS control
#define GBWC_MODE_DISABLE			0
#define GBWC_MODE_LIMIT_MAC_INNER   1 // limit bw by mac address
#define GBWC_MODE_LIMIT_MAC_OUTTER  2 // limit bw by excluding the mac
#define GBWC_MODE_LIMIT_IF_TX	   3 // limit bw by interface tx
#define GBWC_MODE_LIMIT_IF_RX	   4 // limit bw by interface rx
#define GBWC_MODE_LIMIT_IF_TRX	   5 // limit bw by interface tx/rx

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

int get_wifi_mib_priv(int skfd, char *ifname, struct wifi_mib_priv *wifi_mib)
{
	struct iwreq wrq;

	wrq.u.data.flags = SIOCMIBINIT;
	wrq.u.data.pointer = wifi_mib;
	wrq.u.data.length = (unsigned short)sizeof(struct wifi_mib_priv);

	if (iw_get_ext(skfd, ifname, SIOCDEVPRIVATE+2, &wrq) < 0) {
	  close( skfd );
	  printf("%s(%s): ioctl Error.\n", __FUNCTION__, ifname);
	  return -1;
	}

	return 0;
}

int get_wifi_mib(int skfd, char *ifname, struct wifi_mib *pmib)
{
	struct iwreq wrq;

	wrq.u.data.pointer = (caddr_t)pmib;
	wrq.u.data.length = sizeof(struct wifi_mib);

	if (iw_get_ext(skfd, ifname, SIOCMIBINIT, &wrq) < 0) {
		printf("%s(%s): ioctl Error.\n", __FUNCTION__, ifname);
		return -1;
	}
	return 0;
}

int set_wifi_mib_priv(int skfd, char *ifname, struct wifi_mib_priv *wifi_mib)
{
	struct iwreq wrq;

	wrq.u.data.flags = SIOCMIBSYNC;
	wrq.u.data.pointer = wifi_mib;
	wrq.u.data.length = (unsigned short)sizeof(struct wifi_mib_priv);

	if (iw_get_ext(skfd, ifname, SIOCDEVPRIVATE+2, &wrq) < 0) {
	  close( skfd );
	  printf("%s(%s): ioctl Error.\n", __FUNCTION__, ifname);
	  return -1;
	}

	return 0;
}

int set_wifi_mib(int skfd, char *ifname, struct wifi_mib *pmib)
{
	struct iwreq wrq;

	wrq.u.data.pointer = (caddr_t)pmib;
	wrq.u.data.length = sizeof(struct wifi_mib);

	if (iw_get_ext(skfd, ifname, SIOCMIBSYNC, &wrq) < 0)
	{
		printf("Set WLAN MIB failed!\n");
		return -1;
	}

	return 0;
}

void gen_wifi_priv_cfg(struct wifi_mib_priv *wifi_mib, int wlan_idx, int vwlan_idx)
{
	MIB_CE_MBSSIB_T Entry;
	char mode=0, wlan_mode=0;
	unsigned char value[34], phyband;
#ifdef WLAN_UNIVERSAL_REPEATER
	char rpt_enabled=0;
#endif
#ifdef WLAN_RATE_PRIOR
	unsigned char rate_prior=0;
#endif

	memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_idx, vwlan_idx, (void *)&Entry);

	//function
	if(wifi_mib->func_off != Entry.func_off)
		wifi_mib->func_off = Entry.func_off;

	//disable_protection
	*((unsigned char*)(value)) = 0;
	mib_local_mapping_get(MIB_WLAN_PROTECTION_DISABLED, wlan_idx, (void *)value);
	if(wifi_mib->disable_protection != *((unsigned char*)(value)))
		wifi_mib->disable_protection = *((unsigned char*)(value));

	//aggregation
	mib_local_mapping_get(MIB_WLAN_AGGREGATION, wlan_idx, (void *)value);
	if(wifi_mib->aggregation != value[0])
		wifi_mib->aggregation = value[0];

	//ampdu
	if((value[0]&(1<<WLAN_AGGREGATION_AMPDU))==0) {
		if(wifi_mib->ampdu != 0)
			wifi_mib->ampdu = 0;
	}
	else {
		if(wifi_mib->ampdu != 1)
			wifi_mib->ampdu = 1;
	}

	//amsdu
	if((value[0]&(1<<WLAN_AGGREGATION_AMSDU))==0) {
		if(wifi_mib->amsdu != 0)
			wifi_mib->amsdu = 0;
	}
	else{
		if(wifi_mib->amsdu != 2)
			wifi_mib->amsdu = 2;
	}

	//iapp_enable
	mib_local_mapping_get(MIB_WLAN_IAPP_ENABLED, wlan_idx, (void *)value);
	if(wifi_mib->iapp_enable != value[0])
		wifi_mib->iapp_enable = value[0];

	//a4_enable & multiap_monitor_mode_disable
#ifdef RTK_MULTI_AP

	mib_local_mapping_get(MIB_MAP_CONTROLLER, wlan_idx, (void *)value);
	if(value[0]) {

		if((Entry.multiap_bss_type & MAP_BACKHAUL_BSS) || (Entry.multiap_bss_type & MAP_BACKHAUL_STA))
			wifi_mib->a4_enable = 1;
		wifi_mib->multiap_monitor_mode_disable = 0;
		if (Entry.multiap_bss_type == 0) {  // incase controller start vaps without correct multiap_bss_type value
			Entry.multiap_bss_type = MAP_FRONTHAUL_BSS;
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, wlan_idx, (void *)&Entry, vwlan_idx);
		}
	} else {
		wifi_mib->a4_enable = 0;
		wifi_mib->multiap_monitor_mode_disable = 1;
	}
#endif

	//gbwcmode & gbwcthrd_tx & gbwcthrd_rx
	if(Entry.tx_restrict && Entry.rx_restrict == 0)
		mode = GBWC_MODE_LIMIT_IF_TX;
	else if(Entry.tx_restrict == 0 && Entry.rx_restrict)
		mode = GBWC_MODE_LIMIT_IF_RX;
    else if (Entry.tx_restrict && Entry.rx_restrict)
        mode = GBWC_MODE_LIMIT_IF_TRX;
	if(wifi_mib->gbwcmode != mode)
		wifi_mib->gbwcmode = mode;
#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_CTCAPD)
	if(wifi_mib->gbwcthrd_tx != Entry.tx_restrict)
		wifi_mib->gbwcthrd_tx = Entry.tx_restrict;
	if(wifi_mib->gbwcthrd_rx != Entry.rx_restrict)
		wifi_mib->gbwcthrd_rx = Entry.rx_restrict;
#else
	if(wifi_mib->gbwcthrd_tx != Entry.tx_restrict*1024)
		wifi_mib->gbwcthrd_tx = Entry.tx_restrict*1024;
	if(wifi_mib->gbwcthrd_rx != Entry.rx_restrict*1024)
		wifi_mib->gbwcthrd_rx = Entry.rx_restrict*1024;
#endif

	//telco_selected
	mib_local_mapping_get(MIB_TELCO_SELECTED, wlan_idx, (void *)value);
	if(wifi_mib->telco_selected != value[0])
		wifi_mib->telco_selected = value[0];

	//regdomain
	mib_local_mapping_get(MIB_HW_REG_DOMAIN, wlan_idx, (void *)value);
	if(wifi_mib->regdomain != value[0])
		wifi_mib->regdomain = value[0];

	//led_type
#ifdef CONFIG_WIFI_LED_USE_SOC_GPIO
	#define LEDTYPE 0
#elif defined(CONFIG_E8B)
	#define LEDTYPE 7
#else
	#ifdef CONFIG_00R0
		#define LEDTYPE 7
	#elif defined(CONFIG_RTL_92C_SUPPORT)
		#define LEDTYPE 11
	#else
		#define LEDTYPE 3
	#endif
#endif
	if(wifi_mib->led_type != LEDTYPE)
		wifi_mib->led_type = LEDTYPE;
#if defined(WIFI5_WIFI6_COMP) && !defined(CONFIG_RTK_DEV_AP)
	//since wifi6 only supports led_type 7, wifi5 should set to 7.
	if(wifi_mib->led_type != 7)
		wifi_mib->led_type = 7;
#endif

	//lifetime
#ifdef WLAN_LIFETIME
	*((unsigned int*)(value)) = 0;
	mib_local_mapping_get(MIB_WLAN_LIFETIME, wlan_idx, (void *)value);
	if(wifi_mib->lifetime != *((unsigned int*)(value)))
		wifi_mib->lifetime = *((unsigned int*)(value));
#endif

	//manual_priority
#ifdef CONFIG_RTK_SSID_PRIORITY
	if(wifi_mib->manual_priority != Entry.manual_priority)
		wifi_mib->manual_priority = Entry.manual_priority;
#endif

	//autorate
	value[0] = Entry.rateAdaptiveEnabled;
	if(wifi_mib->autorate != (int)value[0])
		wifi_mib->autorate = (int)value[0];

	//fixrate
	if(Entry.rateAdaptiveEnabled == 0)
	{
		wifi_mib->fixrate = Entry.fixedTxRate;
		wifi_mib->autorate = 0;
	}
	else
	{
		if(Entry.is_ax_support){
			wifi_mib->autorate = 1;
			wifi_mib->fixrate = 0;
		}
	}
	
	//deny_legacy
#ifdef WLAN_RATE_PRIOR
	mib_local_mapping_get(MIB_WLAN_RATE_PRIOR, wlan_idx, (void *)&rate_prior);
#endif
	value[0] = Entry.wlanBand;
	mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlan_idx, (void *)&phyband);
#ifdef WLAN_RATE_PRIOR
	if(rate_prior == 0){
#endif
		if (value[0] == 8) { //pure 11n
			if(phyband == PHYBAND_5G) {//5G
				value[0] += 4; // a
				mode = 4;
			}
			else{
				value[0] += 3;	//b+g+n
				mode = 3;
			}
		}
		else if (value[0] == 2) {	//pure 11g
			value[0] += 1;	//b+g
			mode = 1;
		}
		else if (value[0] == 10) {	//g+n
			value[0] += 1;	//b+g+n
			mode = 1;
		}
		else if(value[0] == 64) {	//pure 11ac
			value[0] += 12; 	//a+n
			mode = 12;
		}
		else if(value[0] == 72) {	//n+ac
			value[0] += 4; 	//a
			mode = 4;
		}
		else mode = 0;
#ifdef WLAN_RATE_PRIOR
	}
	else{
		if(phyband == PHYBAND_5G) {//5G
			value[0] = 76; //pure 11ac
			mode = 12;
		}
		else{
			value[0] = 11;	//pure 11n
			mode = 3;
		}
	}
#endif
	if(wifi_mib->deny_legacy != mode)
		wifi_mib->deny_legacy = mode;

	//lgyEncRstrct
	if(wifi_mib->lgyEncRstrct != 15)
		wifi_mib->lgyEncRstrct = 15;

	//coexist
#if 0
#if defined(CONFIG_00R0) && defined(WLAN_11N_COEXIST)
	if(phyband == PHYBAND_2G)
	{
#endif
#ifdef WLAN_RATE_PRIOR
		if(rate_prior==0){
#endif
			//11N Co-Existence
			mib_local_mapping_get(MIB_WLAN_11N_COEXIST, wlan_idx, (void *)value);
			if(wifi_mib->coexist != value[0])
				wifi_mib->coexist = value[0];
#ifdef WLAN_RATE_PRIOR
		}
		else{
			if(phyband == PHYBAND_2G) {
				if(wifi_mib->coexist != 1)
				wifi_mib->coexist = 1;
			}
		}
#endif
#if defined(CONFIG_00R0) && defined(WLAN_11N_COEXIST)
	}
#endif
#endif
	//crossband_enable
	wlan_mode=Entry.wlanMode;
	mib_local_mapping_get(MIB_CROSSBAND_ENABLE, wlan_idx, (void *)value);
	if ((wlan_mode == AP_MODE || wlan_mode == AP_MESH_MODE || wlan_mode == AP_WDS_MODE)){
		if(value[0])
			wifi_mib->crossband_enable = 1;
	}

	//monitor_sta_enabled
	//be used in rtk_isp_wlan_adapter.c

	//txbf & txbf_mu
#ifdef WLAN_TX_BEAMFORMING
		if(wifi_mib->txbf != Entry.txbf)
			wifi_mib->txbf = Entry.txbf;
		if(Entry.txbf) {
			if(wifi_mib->txbf_mu != Entry.txbf_mu)
				wifi_mib->txbf_mu = Entry.txbf_mu;
		}
		else {
			if(wifi_mib->txbf_mu != 0)
				wifi_mib->txbf_mu = 0;
		}
#endif
#ifdef WLAN_INTF_TXBF_DISABLE
		//txbf must disable if enable antenna diversity
		if(wifi_mib->txbf != 0)
			wifi_mib->txbf = 0;
		if(wifi_mib->txbf_mu != 0)
			wifi_mib->txbf_mu = 0;
#endif

	//roaming_switch
	mib_local_mapping_get(MIB_RTL_LINK_ROAMING_SWITCH, wlan_idx, (void *)value);
	if(wifi_mib->roaming_switch != value[0])
		wifi_mib->roaming_switch = value[0];

	//roaming_qos
	mib_local_mapping_get(MIB_RTL_LINK_ROAMING_QOS, wlan_idx, (void *)value);
	if(wifi_mib->roaming_qos != value[0])
		wifi_mib->roaming_qos = value[0];

	//fail_ratio
	mib_local_mapping_get(MIB_RTL_LINK_ROAMING_FAIL_RATIO, wlan_idx, (void *)value);
	if(wifi_mib->fail_ratio != value[0])
		wifi_mib->fail_ratio = value[0];
	if(!wifi_mib->fail_ratio)
		wifi_mib->fail_ratio = 100; //default

	//retry_ratio
	mib_local_mapping_get(MIB_RTL_LINK_ROAMING_RETRY_RATIO, wlan_idx, (void *)value);
	if(wifi_mib->retry_ratio != value[0])
		wifi_mib->retry_ratio = value[0];
	if(!wifi_mib->retry_ratio)
		wifi_mib->retry_ratio = 100; //default

	//ofdma
	mib_local_mapping_get(MIB_WLAN_OFDMA_ENABLED, wlan_idx, (void *)value);
	if(wifi_mib->ofdma_enable != value[0])
		wifi_mib->ofdma_enable = value[0];

	//RSSIThreshold
	if(wifi_mib->RSSIThreshold != Entry.rtl_link_rssi_th)
		wifi_mib->RSSIThreshold = Entry.rtl_link_rssi_th;

	//dfgw_mac
	mib_local_mapping_get(MIB_DEF_GW_MAC, wlan_idx, (void *)value);
	value[6] = '\0';
	//snprintf(wifi_mib->dfgw_mac, sizeof(wifi_mib->dfgw_mac), "%02X%02X%02X%02X%02X%02X", value[0], value[1], value[2], value[3], value[4], value[5]);
	memcpy(wifi_mib->dfgw_mac, value, sizeof(wifi_mib->dfgw_mac));

#ifdef WLAN_ROAMING
	unsigned char roaming_enable=0, roaming_rssi_th1=0, roaming_rssi_th2=0;
	unsigned int roaming_start_time=0, roaming_wait_time=0;
	if(phyband==PHYBAND_5G){
		mib_local_mapping_get(MIB_ROAMING5G_ENABLE, wlan_idx, (void *)&roaming_enable);
		mib_local_mapping_get(MIB_ROAMING5G_STARTTIME, wlan_idx, (void *)&roaming_start_time);
		mib_local_mapping_get(MIB_ROAMING5G_RSSI_TH1, wlan_idx, (void *)&roaming_rssi_th1);
		mib_local_mapping_get(MIB_ROAMING5G_RSSI_TH2, wlan_idx, (void *)&roaming_rssi_th2);
	}
	else{
		mib_local_mapping_get(MIB_ROAMING2G_ENABLE, wlan_idx, (void *)&roaming_enable);
		mib_local_mapping_get(MIB_ROAMING2G_STARTTIME, wlan_idx, (void *)&roaming_start_time);
		mib_local_mapping_get(MIB_ROAMING2G_RSSI_TH1, wlan_idx, (void *)&roaming_rssi_th1);
		mib_local_mapping_get(MIB_ROAMING2G_RSSI_TH2, wlan_idx, (void *)&roaming_rssi_th2);
	}
	//roaming_enable
	if(wifi_mib->roaming_enable != roaming_enable)
		wifi_mib->roaming_enable = roaming_enable;

	//roaming_start_time
	if(wifi_mib->roaming_start_time != roaming_start_time)
		wifi_mib->roaming_start_time = roaming_start_time;

	//roaming_rssi_th1
	if(wifi_mib->roaming_rssi_th1 != roaming_rssi_th1)
		wifi_mib->roaming_rssi_th1 = roaming_rssi_th1;

	//roaming_rssi_th2
	if(wifi_mib->roaming_rssi_th2 != roaming_rssi_th2)
		wifi_mib->roaming_rssi_th2 = roaming_rssi_th2;
	
	//roaming_wait_time
	if(wifi_mib->roaming_wait_time != 3)
		wifi_mib->roaming_wait_time = 3;
#endif

#ifdef RTK_HAPD_MULTI_AP
	wifi_mib->multiap_bss_type = Entry.multiap_bss_type;
#endif

	mib_local_mapping_get(MIB_WLAN_MCAST2UCAST_DISABLE, wlan_idx, (void *)value);
	if(wifi_mib->mc2u_disable != value[0])
		wifi_mib->mc2u_disable = value[0];

	// hiddenAP
	if(wifi_mib->hiddenAP != Entry.hidessid)
		wifi_mib->hiddenAP = Entry.hidessid;

}

void sync_11ac_mib(struct wifi_mib *wifiMib, int wlan_index, int vwlan_index)
{
	unsigned char value[34]={0};

#ifdef CONFIG_RTK_DEV_AP
	unsigned char phyband=0;
	mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlan_index, (void *)&phyband);

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	//stactrl_enable
	mib_local_mapping_get(MIB_WIFI_STA_CONTROL, wlan_index, (void *)value);
	value[0] = (value[0])?1:0;
	if(wifiMib->staControl.stactrl_enable != value[0])
		wifiMib->staControl.stactrl_enable = value[0];

	//stactrl_rssi_th_high
	*((unsigned int *)(value)) = 0;
	mib_local_mapping_get(MIB_WLAN_BANDST_RSSTHRDHIGH, wlan_index, (void *)value);
	if(wifiMib->staControl.stactrl_rssi_th_high != *((unsigned int *)(value)))
		wifiMib->staControl.stactrl_rssi_th_high = *((unsigned int *)(value));

	//stactrl_rssi_th_low
	*((unsigned int *)(value)) = 0;
	mib_local_mapping_get(MIB_WLAN_BANDST_RSSTHRDLOW, wlan_index, (void *)value);
	if(wifiMib->staControl.stactrl_rssi_th_low != *((unsigned int *)(value)))
		wifiMib->staControl.stactrl_rssi_th_low = *((unsigned int *)(value));

	//stactrl_ch_util_th
	*((unsigned int *)(value)) = 0;
	mib_local_mapping_get(MIB_WLAN_BANDST_CHANNELUSETHRD, wlan_index, (void *)value);
	{
		*((unsigned int *)(value)) = ((*((unsigned int *)(value))*255)/100);
		if(wifiMib->staControl.stactrl_ch_util_th != *((unsigned int *)(value)))
			wifiMib->staControl.stactrl_ch_util_th = *((unsigned int *)(value));
	}

	//stactrl_steer_detect
	*((unsigned int *)(value)) = 0;
	mib_local_mapping_get(MIB_WLAN_BANDST_STEERINGINTERVAL, wlan_index, (void *)value);
	if(wifiMib->staControl.stactrl_steer_detect != *((unsigned int *)(value)))
			wifiMib->staControl.stactrl_steer_detect = *((unsigned int *)(value));

	//stactrl_sta_load_th_2g
	*((unsigned int *)(value)) = 0;
	mib_local_mapping_get(MIB_WLAN_BANDST_STALOADTHRESHOLD2G, wlan_index, (void *)value);
	if(wifiMib->staControl.stactrl_sta_load_th_2g != *((unsigned int *)(value)))
			wifiMib->staControl.stactrl_sta_load_th_2g = *((unsigned int *)(value));

	//stactrl_prefer_band
	value[0] = (phyband == PHYBAND_5G)?1:0;
	if(wifiMib->staControl.stactrl_prefer_band != value[0])
		wifiMib->staControl.stactrl_prefer_band = value[0];
#else
	//stactrl_enable
	mib_local_mapping_get(MIB_WIFI_STA_CONTROL, wlan_index, (void *)value);
	value[0] = (value[0] & STA_CONTROL_ENABLE)?1:0;
	if(wifiMib->staControl.stactrl_enable != value[0])
		wifiMib->staControl.stactrl_enable = value[0];

	//stactrl_prefer_band
	if(wifiMib->staControl.stactrl_enable & STA_CONTROL_ENABLE){
		value[0] = (wifiMib->staControl.stactrl_enable & STA_CONTROL_PREFER_BAND)? PHYBAND_2G:PHYBAND_5G;
		if(value[0] == phyband)
			value[0] = 1;
		else
			value[0] = 0;
	}
	else
		value[0] = 0;
	if(wifiMib->staControl.stactrl_prefer_band != value[0])
		wifiMib->staControl.stactrl_prefer_band = value[0];
#endif

	//stactrl_groupID
#ifdef WLAN_USE_VAP_AS_SSID1
	if(wifiMib->staControl.stactrl_groupID != 2)
		wifiMib->staControl.stactrl_groupID = 2;
#endif
#else
	//fixme: set by wlan manager ?
#endif //CONFIG_RTK_DEV_AP

	//lifetime
#ifdef WLAN_LIFETIME
	unsigned int lifetime=0;
	mib_local_mapping_get(MIB_WLAN_LIFETIME, wlan_index, (void *)&lifetime);
	if(wifiMib->dot11OperationEntry.lifetime != lifetime)
		wifiMib->dot11OperationEntry.lifetime = lifetime;
#endif

	//opmode
	unsigned int wlan_mode=0;
#ifdef WLAN_UNIVERSAL_REPEATER
	char rpt_enabled=0;
#endif
	MIB_CE_MBSSIB_T Entry;
	memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlan_index, vwlan_index, (void *)&Entry);

#ifdef WLAN_UNIVERSAL_REPEATER
	if(vwlan_index == WLAN_REPEATER_ITF_INDEX)
		mib_local_mapping_get( MIB_REPEATER_ENABLED1, wlan_index, (void *)&rpt_enabled);
#endif
	wlan_mode = Entry.wlanMode;
	if(vwlan_index == WLAN_ROOT_ITF_INDEX){
		if (wlan_mode == CLIENT_MODE)
			value[0] = 8;	// client
		else	// 0(AP_MODE) or 3(AP_WDS_MODE)
			value[0] = 16;	// AP
		if(wifiMib->dot11OperationEntry.opmode != value[0])
			wifiMib->dot11OperationEntry.opmode = value[0];
	}
#ifdef WLAN_UNIVERSAL_REPEATER
	// Repeater opmode
	if (vwlan_index == WLAN_REPEATER_ITF_INDEX){
		if (rpt_enabled) {
			if (wlan_mode == CLIENT_MODE)
				value[0] = 8;
			else
				value[0] = 16;
			if(wifiMib->dot11OperationEntry.opmode != value[0])
				wifiMib->dot11OperationEntry.opmode = value[0];
		}
	}
#endif
}

void convert_wifi_priv_cfg(struct wifi_mib_priv *priv_mib, struct wifi_mib *wifiMib, int wlan_index, int vwlan_index)
{
	sync_11ac_mib(wifiMib, wlan_index, vwlan_index);
	//func_off
	wifiMib->miscEntry.func_off = (unsigned int)priv_mib->func_off;
	//disable_protection
	wifiMib->dot11StationConfigEntry.protectionDisabled = (int)priv_mib->disable_protection;
	//aggregation
	wifiMib->dot11nConfigEntry.dot11nAMPDU = (unsigned int)priv_mib->aggregation;
	//iapp_enable
	wifiMib->dot11OperationEntry.iapp_enable = (unsigned int)priv_mib->iapp_enable;
	//a4_enable
	wifiMib->miscEntry.a4_enable = priv_mib->a4_enable;
	//multiap_monitor_mode_disable
	wifiMib->multi_ap.multiap_monitor_mode_disable = priv_mib->multiap_monitor_mode_disable;
	//multiap_bss_type
	wifiMib->multi_ap.multiap_bss_type = priv_mib->multiap_bss_type;
	//gbwcmode
	wifiMib->gbwcEntry.GBWCMode = priv_mib->gbwcmode;
	//gbwcthrd_tx
	wifiMib->gbwcEntry.GBWCThrd_tx = priv_mib->gbwcthrd_tx;
	//gbwcthrd_rx
	wifiMib->gbwcEntry.GBWCThrd_rx = priv_mib->gbwcthrd_rx;
	//telco_selected
	wifiMib->miscEntry.telco_selected = priv_mib->telco_selected;
	//regdomain
	wifiMib->dot11StationConfigEntry.dot11RegDomain = (unsigned int)priv_mib->regdomain;
	//led_type
	wifiMib->dot11OperationEntry.ledtype = priv_mib->led_type;
	//manual_priority
	wifiMib->miscEntry.manual_priority = priv_mib->manual_priority;
	//autorate
	wifiMib->dot11StationConfigEntry.autoRate = priv_mib->autorate;
	//fixrate
	wifiMib->dot11StationConfigEntry.fixedTxRate = priv_mib->fixrate;
	//deny_legacy
	wifiMib->dot11StationConfigEntry.legacySTADeny = priv_mib->deny_legacy;
	//lgyEncRstrct
	wifiMib->dot11nConfigEntry.dot11nLgyEncRstrct = priv_mib->lgyEncRstrct;
	//cts2self
	wifiMib->dot11ErpInfo.ctsToSelf = priv_mib->lgyEncRstrct;
	//coexist
	//wifiMib->dot11nConfigEntry.dot11nCoexist = priv_mib->coexist;
	//ampdu
	wifiMib->dot11nConfigEntry.dot11nAMPDU = (unsigned int)priv_mib->ampdu;
	//amsdu
	wifiMib->dot11nConfigEntry.dot11nAMSDU = (unsigned int)priv_mib->amsdu;
	//crossband_enable
	wifiMib->crossBand.crossband_enable = priv_mib->crossband_enable;
	//monitor_sta_enabled
	wifiMib->dot11StationConfigEntry.monitor_sta_enabled = priv_mib->monitor_sta_enabled;
	//txbf
	wifiMib->dot11RFEntry.txbf = priv_mib->txbf;
	//txbf_mu
	wifiMib->dot11RFEntry.txbf_mu = priv_mib->txbf_mu;
	//roaming_switch
	wifiMib->rlr_profile.roaming_switch = priv_mib->roaming_switch;
	//roaming_qos
	wifiMib->rlr_profile.roaming_qos = priv_mib->roaming_qos;
	//fail_ratio
	wifiMib->rlr_profile.fail_ratio = priv_mib->fail_ratio;
	//retry_ratio
	wifiMib->rlr_profile.retry_ratio = priv_mib->retry_ratio;
	//RSSIThreshold
	wifiMib->rlr_profile.RSSIThreshold = (unsigned int)priv_mib->RSSIThreshold;
	//roaming_enable
	wifiMib->roamingEntry.roaming_enable = priv_mib->roaming_enable;
	//roaming_start_time
	wifiMib->roamingEntry.roaming_st_time = priv_mib->roaming_start_time;
	//roaming_rssi_th1
	wifiMib->roamingEntry.roaming_rssi_th1 = priv_mib->roaming_rssi_th1;
	//roaming_rssi_th2
	wifiMib->roamingEntry.roaming_rssi_th2 = priv_mib->roaming_rssi_th2;
	//roaming_wait_time
	wifiMib->roamingEntry.roaming_wait_time = priv_mib->roaming_wait_time;
}

void start_wifi_priv_cfg(int wlan_index, int vwlan_index)
{
	int i, j, skfd, count;
	struct iwreq wrq;
	MIB_CE_MBSSIB_T Entry;
	char ifname[16]={0};
	struct wifi_mib_priv priv_mib;
	struct wifi_mib *wifiMib;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return;

	i = wlan_index;
	j = vwlan_index;
	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
	mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&Entry);

	printf("[%s %d] %d wlanDisabled=%d \n", __FUNCTION__,__LINE__, i, Entry.wlanDisabled);

	if (Entry.wlanDisabled){
		close( skfd );
		return;
	}

	printf("[%s %d]& launch: wlan_idx = %d \n",__FUNCTION__,__LINE__,i);

	memset(ifname,0,sizeof(ifname));
#if 0 // defined(CONFIG_RTK_DEV_AP)
	if(j == 0)
		snprintf(ifname, sizeof(ifname), WLAN_IF, i);
	else
	{
#ifdef WLAN_UNIVERSAL_REPEATER
		if(j == WLAN_REPEATER_ITF_INDEX)
			snprintf(ifname, sizeof(ifname), VXD_IF, i);
		else
#endif
			snprintf(ifname, sizeof(ifname), VAP_IF, i, j-1);
	}
#else
	rtk_wlan_get_ifname(i, j, ifname);
#endif

	memset(&priv_mib,0,sizeof(struct wifi_mib_priv));

	/* Get wireless name, Do 3 times, if failed */
	if(rtk_get_interface_flag(ifname, TIMER_COUNT, IS_EXIST) == 0)
	{
		close( skfd );
		return;
	}

	if(Entry.is_ax_support)
	{
		if(get_wifi_mib_priv(skfd,ifname,&priv_mib)==0)
		{
			if(strcmp(priv_mib.rtw_mib_version, RTW_WIFI_MIB_VERSION) || (priv_mib.rtw_mib_size != sizeof(struct wifi_mib_priv))) {
				printf("%s(%s): The mib version or struct size is mismatch!!!\n", __FUNCTION__, ifname);
				return;
			}
			gen_wifi_priv_cfg(&priv_mib, i, j);

			if(set_wifi_mib_priv(skfd,ifname,&priv_mib) != 0)
			{
				printf("%s(%s): set_wifi_priv_mib fail.\n", __FUNCTION__, ifname);
				close( skfd );
				return;
			}
		}
		else
		{
			printf("%s(%s): get_wifi_priv_mib fail.\n", __FUNCTION__, ifname);
			close( skfd );
			return;
		}
	}
	else
	{
		if ((wifiMib = (struct wifi_mib *)malloc(sizeof(struct wifi_mib))) == NULL) {
			printf("MIB buffer allocation failed!\n");
			close(skfd);
			return;
		}

		if(get_wifi_mib(skfd,ifname,wifiMib)==0)
		{
			gen_wifi_priv_cfg(&priv_mib, i, j);
			convert_wifi_priv_cfg(&priv_mib, wifiMib, i, j);
			if(set_wifi_mib(skfd,ifname,wifiMib) != 0)
			{
				printf("%s(%s): set_wifi_priv_mib fail.\n", __FUNCTION__, ifname);
				if(wifiMib != NULL)
					free(wifiMib);
				close( skfd );
				return;
			}
			if(wifiMib != NULL)
				free(wifiMib);
		}
		else
		{
			printf("%s(%s): get_wifi_priv_mib fail.\n", __FUNCTION__, ifname);
			if(wifiMib != NULL)
				free(wifiMib);
			close( skfd );
			return;
		}
	}

	close( skfd );


}


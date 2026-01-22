#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "debug.h"
#include "utility.h"
#include <linux/wireless.h>

#ifdef WLAN_11R
#if 0 //def USE_LIBMD5
#include <libmd5wrapper.h>
#else
#include <openssl/md5.h>
#endif
#endif

#include "hapd_conf.h"
#include "hapd_defs.h"

#ifdef RTK_MULTI_AP
#include "subr_multiap.h"
#endif

#define HAPD_CONF_RESULT_MAX_LEN 512

static char uuid_def[]="12345678-9abc-def0-1234-56789abcdef0";


#if defined(WLAN_MBSSID_NUM) && (WLAN_MBSSID_NUM > 0)
static int get_wifi_vif_conf(struct wifi_config *vap_config, int if_idx, int index)
{
	unsigned char 	tmp8;
	unsigned short 	tmp16;
	unsigned int 	tmp32;
	unsigned char	tmpbuf[50];
	MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_ACL
#ifndef WLAN_VAP_ACL
	int i=0, j=0, entryNum=0;
	MIB_CE_WLAN_AC_T macEntry;
#endif
#endif
	//char		*name_interface;
	//struct wifi_config *vap_config = &config[index];
	//char ifname[IFNAMSIZ] = {0};
	struct sockaddr hwaddr;
//#ifdef WLAN_GEN_MBSSID_MAC
	unsigned char gen_wlan_mac[6]={0};
//#endif
#ifdef _PRMT_X_WLANFORISP_
	MIB_WLANFORISP_T wlan_isp_entry;
#endif
#ifdef RTK_MULTI_AP
	unsigned char map_mode=0;
#endif

	printf("[%s][%d][%d] ++ \n", __FUNCTION__, __LINE__, index);

	if (!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, index, (void *)&Entry)) {
		printf("Error! Get MIB_MBSSIB_TBL for VAP #%d err\n", index);
		return -1;
	}

	if (Entry.wlanDisabled) {
		vap_config->disabled = 1;
		return 0;
	}

	snprintf(vap_config->interface, sizeof(vap_config->interface) - 1,
	         "%s-" VAP_IF_ONLY, WLANIF[if_idx], index-WLAN_VAP_ITF_INDEX);
	strcpy(vap_config->ctrl_interface, NAME_CTRL_IFACE);
	strcpy(vap_config->bridge, BRIF);
	strcpy(vap_config->ssid, Entry.ssid);
#ifdef CONFIG_MBO_SUPPORT
	if(Entry.wlanMode == AP_MODE){
		vap_config->mbo_enable = Entry.mbo_enable;
	}
#endif
	mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, if_idx, &vap_config->phyband);

	//rtk_wlan_get_ifname(if_idx, index, ifname);
#if defined(WIFI5_WIFI6_COMP) && defined(CONFIG_RTK_DEV_AP)
	mib_get(MIB_ELAN_MAC_ADDR, gen_wlan_mac);
#ifdef WLAN_ETHER_MAC_ADDR_NEED_OFFSET
	setup_mac_addr(gen_wlan_mac, if_idx * (1+NUM_VWLAN_INTERFACE) + index + WLAN_ETHER_MAC_ADDR_OFFSET_VALUE);
#else
	setup_mac_addr(gen_wlan_mac, if_idx * (1+NUM_VWLAN_INTERFACE) + index);
#endif

	sprintf(vap_config->bssid, "%02x:%02x:%02x:%02x:%02x:%02x", gen_wlan_mac[0], gen_wlan_mac[1],
		gen_wlan_mac[2], gen_wlan_mac[3], gen_wlan_mac[4], gen_wlan_mac[5]);
#else
#ifdef WLAN_GEN_MBSSID_MAC
	getInAddr((char *)WLANIF[if_idx], HW_ADDR, &hwaddr);
	_gen_guest_mac(hwaddr.sa_data, MAX_WLAN_VAP+1, index, gen_wlan_mac);
	sprintf(vap_config->bssid, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned char)gen_wlan_mac[0], (unsigned char)gen_wlan_mac[1],
		(unsigned char)gen_wlan_mac[2], (unsigned char)gen_wlan_mac[3], (unsigned char)gen_wlan_mac[4], (unsigned char)gen_wlan_mac[5]);
#else
	getInAddr((char *)WLANIF[if_idx], HW_ADDR, &hwaddr);
	memcpy(gen_wlan_mac, hwaddr.sa_data, MAC_ADDR_LEN);
	setup_mac_addr(gen_wlan_mac, index-WLAN_VAP_ITF_INDEX+1);
	sprintf(vap_config->bssid, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned char)gen_wlan_mac[0], (unsigned char)gen_wlan_mac[1],
		(unsigned char)gen_wlan_mac[2], (unsigned char)gen_wlan_mac[3], (unsigned char)gen_wlan_mac[4], (unsigned char)gen_wlan_mac[5]);
	//vap_config->bssid[0]='\0';
#endif
#endif
	vap_config->encrypt = Entry.encrypt;
	if(Entry.wpaAuth == WPA_AUTH_PSK)
		vap_config->psk_enable = Entry.encrypt/2;
#ifdef WLAN_WPA3_H2E
	if(vap_config->psk_enable & PSK_WPA3)
		vap_config->sae_pwe = Entry.sae_pwe;
#endif

	strcpy(vap_config->passphrase, Entry.wpaPSK);
	vap_config->gk_rekey = Entry.wpaGroupRekeyTime;
	vap_config->authtype = Entry.authType;;
	vap_config->psk_format = Entry.wpaPSKFormat;
#ifdef WLAN_WPA3
	if(Entry.encrypt == WIFI_SEC_WPA3 || Entry.encrypt == WIFI_SEC_WPA2_WPA3_MIXED)
	{
		vap_config->wpa_cipher = 0;
		vap_config->wpa2_cipher = WPA_CIPHER_AES;
	}
	else
#endif
	{
		vap_config->wpa_cipher = Entry.unicastCipher;
		vap_config->wpa2_cipher = Entry.wpa2UnicastCipher;
	}

	if(Entry.enable1X)
		vap_config->enable_1x = Entry.enable1X;
	else if(Entry.wpaAuth == WPA_AUTH_AUTO && Entry.encrypt >= WIFI_SEC_WPA)
		vap_config->enable_1x = 1;

#ifdef WLAN_11W
	vap_config->dot11IEEE80211W = Entry.dotIEEE80211W;
	vap_config->enableSHA256 = Entry.sha256;
#endif

	if(Entry.encrypt == WIFI_SEC_WEP){
		vap_config->wepDefaultKey = Entry.wepDefaultKey;
		vap_config->wepKeyType = Entry.wepKeyType;
		if(Entry.wep == WEP64){
			vap_config->encmode = _WEP_40_PRIVACY_;
			memcpy(vap_config->wepkey0, Entry.wep64Key1, sizeof(Entry.wep64Key1));
			memcpy(vap_config->wepkey1, Entry.wep64Key2, sizeof(Entry.wep64Key2));
			memcpy(vap_config->wepkey2, Entry.wep64Key3, sizeof(Entry.wep64Key3));
			memcpy(vap_config->wepkey3, Entry.wep64Key4, sizeof(Entry.wep64Key4));
			/*
			if(Entry.wepDefaultKey == 0)
				memcpy(vap_config->wepkey0, Entry.wep64Key1, sizeof(Entry.wep64Key1));
			else if(Entry.wepDefaultKey == 1)
				memcpy(vap_config->wepkey0, Entry.wep64Key2, sizeof(Entry.wep64Key2));
			else if(Entry.wepDefaultKey == 2)
				memcpy(vap_config->wepkey0, Entry.wep64Key3, sizeof(Entry.wep64Key3));
			else if(Entry.wepDefaultKey == 3)
				memcpy(vap_config->wepkey0, Entry.wep64Key4, sizeof(Entry.wep64Key4));
			*/
	}
		else if(Entry.wep == WEP128){
			vap_config->encmode = _WEP_104_PRIVACY_;
			memcpy(vap_config->wepkey0, Entry.wep128Key1, sizeof(Entry.wep128Key1));
			memcpy(vap_config->wepkey1, Entry.wep128Key2, sizeof(Entry.wep128Key2));
			memcpy(vap_config->wepkey2, Entry.wep128Key3, sizeof(Entry.wep128Key3));
			memcpy(vap_config->wepkey3, Entry.wep128Key4, sizeof(Entry.wep128Key4));
			/*
			if(Entry.wepDefaultKey == 0)
				memcpy(vap_config->wepkey0, Entry.wep128Key1, sizeof(Entry.wep128Key1));
			else if(Entry.wepDefaultKey == 1)
				memcpy(vap_config->wepkey0, Entry.wep128Key2, sizeof(Entry.wep128Key2));
			else if(Entry.wepDefaultKey == 2)
				memcpy(vap_config->wepkey0, Entry.wep128Key3, sizeof(Entry.wep128Key3));
			else if(Entry.wepDefaultKey == 3)
				memcpy(vap_config->wepkey0, Entry.wep128Key4, sizeof(Entry.wep128Key4));
			*/
		}
	}
	else if(Entry.encrypt)
		vap_config->encmode = _IEEE8021X_PSK_;
	else
		vap_config->encmode = _NO_PRIVACY_;

#ifndef WLAN_11AX
	mib_local_mapping_get(MIB_WLAN_AUTH_TYPE, if_idx, &vap_config->authtype);
	mib_local_mapping_get(MIB_WLAN_ENCRYPT, if_idx, &vap_config->encmode);
	mib_local_mapping_get(MIB_WLAN_WPA_PSK_FORMAT, if_idx, &vap_config->psk_format);
	mib_local_mapping_get(MIB_WLAN_WPA_CIPHER_SUITE, if_idx, &vap_config->wpa_cipher);
	mib_local_mapping_get(MIB_WLAN_WPA2_CIPHER_SUITE, if_idx, &vap_config->wpa2_cipher);
	mib_local_mapping_get(MIB_WLAN_ENABLE_1X, if_idx, &vap_config->enable_1x);
#ifdef WLAN_11W
	mib_local_mapping_get(MIB_WLAN_DOTIEEE80211W, if_idx,	&vap_config->dot11IEEE80211W);
#endif
#endif

	printf("VAP%u: psk_enable=%d authtype=%d encmode=%d psk_format=%d gk_rekey=%d\n",
	       index-WLAN_VAP_ITF_INDEX,
	       vap_config->psk_enable, vap_config->authtype,
	       vap_config->encmode, vap_config->psk_format, vap_config->gk_rekey);

	printf("VAP%u: %s \n", index-WLAN_VAP_ITF_INDEX, vap_config->passphrase);

	printf("VAP%u: wpa_cipher=0x%x wpa2_cipher=0x%x enable_1x=%d\n",
	       index-WLAN_VAP_ITF_INDEX,
	       vap_config->wpa_cipher, vap_config->wpa2_cipher, vap_config->enable_1x);
#ifdef WLAN_11W
	printf("VAP%u: dot11IEEE80211W=%d enableSHA256=%d\n",
	       index-WLAN_VAP_ITF_INDEX, vap_config->dot11IEEE80211W, vap_config->enableSHA256);
#endif
#ifdef WLAN_WPA3_H2E
	printf("VAP%u: sae_pwe=%d\n", index-WLAN_VAP_ITF_INDEX, vap_config->sae_pwe);
#endif

//RADIUS
	mib_get(MIB_ADSL_LAN_IP, vap_config->lan_ip);

	vap_config->rsPort = Entry.rsPort;
	memcpy(vap_config->rsIpAddr, Entry.rsIpAddr, sizeof(Entry.rsIpAddr));
	memcpy(vap_config->rsPassword, Entry.rsPassword, sizeof(Entry.rsPassword));

	vap_config->rs2Port = Entry.rs2Port;
	memcpy(vap_config->rs2IpAddr, Entry.rs2IpAddr, sizeof(Entry.rs2IpAddr));
	memcpy(vap_config->rs2Password, Entry.rs2Password, sizeof(Entry.rs2Password));

	mib_local_mapping_get(MIB_WLAN_RS_INTERVAL_TIME, if_idx, &tmp16);
	vap_config->radius_retry_primary_interval = tmp16*60;

	mib_local_mapping_get(MIB_WLAN_ACCOUNT_RS_INTERVAL_TIME, if_idx, &tmp16);
	vap_config->radius_acct_interim_interval = tmp16*60;
	
	
#ifdef _PRMT_X_WLANFORISP_
	if(vap_config->enable_1x==1 && rtk_wlan_get_wlanforisp_entry(if_idx, index, &wlan_isp_entry)==0){
		memcpy(vap_config->nas_identifier, wlan_isp_entry.NasID, strlen(wlan_isp_entry.NasID));
		if(wlan_isp_entry.RadiusAccountEnable==0)
			vap_config->disable_acct = 1;
	}
#endif

//Advanced
	if(Entry.rateAdaptiveEnabled==1)
		vap_config->data_rate = 0;
	else
		vap_config->data_rate = Entry.fixedTxRate;

	mib_local_mapping_get(MIB_WLAN_DTIM_PERIOD, if_idx, &vap_config->dtimperiod);

	vap_config->ignore_broadcast_ssid = Entry.hidessid;
	vap_config->qos = Entry.wmmEnabled;
	vap_config->ap_isolate = Entry.userisolation;

#ifdef CONFIG_GUEST_ACCESS_SUPPORT
	if(Entry.guest_access == 0)
		vap_config->guest_access = 0;
	else
		vap_config->guest_access = 1;
#endif

//11KV
#ifdef WLAN_11K
	if(Entry.rm_activated){
		vap_config->rrm_beacon_report = 1;
		vap_config->rrm_neighbor_report = 1;
	}
#endif
#ifdef WLAN_11V
	if(Entry.BssTransEnable)
		vap_config->bss_transition = 1;
#endif

//11R
#ifdef WLAN_11R
	vap_config->ft_enable = Entry.ft_enable;
	if(Entry.ft_enable){
		memcpy(vap_config->ft_mdid, Entry.ft_mdid, sizeof(vap_config->ft_mdid));
		vap_config->ft_over_ds = Entry.ft_over_ds;
		memcpy(vap_config->nas_identifier, Entry.ft_r0kh_id, sizeof(vap_config->ft_r0kh_id));
		memcpy(vap_config->ft_r0kh_id, Entry.ft_r0kh_id, sizeof(vap_config->ft_r0kh_id));
		vap_config->ft_push = Entry.ft_push;
	}
#endif

//WPS
#ifdef WLAN_WPS
#ifdef WLAN_WPS_VAP
	//vap_config->wps_state = Entry.wsc_disabled? 0:2;
	vap_config->wps_state = Entry.wsc_disabled? 0:(Entry.wsc_configured? 2:1);

#ifdef WLAN_WPA3_NOT_SUPPORT_WPS
	if(vap_config->wps_state && Entry.encrypt == WIFI_SEC_WPA3)
		vap_config->wps_state = 0;
#endif

	if(vap_config->wps_state){
		// search UUID field, replace last 12 char with hw mac address
		//getMIB2Str(MIB_ELAN_MAC_ADDR, tmpbuf);
		//memcpy(uuid_def+strlen(uuid_def)-12, tmpbuf, 12);
		//strcpy(vap_config->uuid, uuid_def);

		mib_get_s(MIB_SNMP_SYS_NAME, (void *)vap_config->device_name, sizeof(vap_config->device_name));
		strcpy(vap_config->model_name, WPS_MODEL_NAME);
		strcpy(vap_config->model_number, WPS_MODEL_NUMBER);
		strcpy(vap_config->serial_number, WPS_SERIAL_NUMBER);
		strcpy(vap_config->device_type, WPS_DEVICE_TYPE);
		mib_get_s(MIB_WSC_METHOD, (void *)&vap_config->config_method, sizeof(vap_config->config_method));
		mib_get_s(MIB_WSC_PIN, (void *)vap_config->ap_pin, sizeof(vap_config->ap_pin));
		if(Entry.wsc_upnp_enabled)
			strcpy(vap_config->upnp_iface, BRIF);
		else
			vap_config->upnp_iface[0] = '\0';
		strcpy(vap_config->friendly_name, WPS_FRIENDLY_NAME);
		strcpy(vap_config->manufacturer_url, WPS_MANUFACTURER_URL);
		strcpy(vap_config->model_url, WPS_MODEL_URL);
		vap_config->wps_independent=1;
		if(vap_config->wps_state==1) //WPS enabled, not configured
			vap_config->wps_cred_processing=1;
	}
#endif
#endif //WLAN_WPS

//station
#if 1//defined(CONFIG_ELINK_SUPPORT) || defined(CONFIG_ANDLINK_SUPPORT) || defined(CONFIG_USER_ADAPTER_API_ISP)
	vap_config->max_num_sta = Entry.rtl_link_sta_num;
#endif
#ifdef WLAN_LIMITED_STA_NUM
	if ((vap_config->max_num_sta) && (Entry.stanum < vap_config->max_num_sta))
		vap_config->max_num_sta = Entry.stanum;
	else if (!vap_config->max_num_sta)
		vap_config->max_num_sta = Entry.stanum;
#endif

#ifdef RTK_MULTI_AP
	mib_get_s(MIB_MAP_CONTROLLER, (void *)&(map_mode), sizeof(map_mode));
	if(map_mode == MAP_CONTROLLER || map_mode == MAP_CONTROLLER_WFA || map_mode == MAP_AGENT_WFA || map_mode == MAP_CONTROLLER_WFA_R2 || map_mode == MAP_AGENT_WFA_R2 || map_mode == MAP_CONTROLLER_WFA_R3 || map_mode == MAP_AGENT_WFA_R3)
	{
		vap_config->multi_ap = 0;
		if(Entry.multiap_bss_type & MAP_FRONTHAUL_BSS){
			vap_config->multi_ap += 2;
		}
		if(Entry.multiap_bss_type & MAP_BACKHAUL_BSS)
		{
			vap_config->multi_ap += 1;
#ifdef WLAN_WPS
			snprintf(vap_config->multi_ap_backhaul_ssid, sizeof(vap_config->multi_ap_backhaul_ssid), "%s", Entry.ssid);
			strcpy(vap_config->multi_ap_backhaul_passphrase, Entry.wpaPSK);
			vap_config->multi_ap_backhaul_psk_format = Entry.wpaPSKFormat;
#endif
		}
	}
#endif

//acl
#ifdef WLAN_ACL
#ifndef WLAN_VAP_ACL
#if 0 //def WLAN_VAP_ACL
	memset(&Wlan_Entry,0,sizeof(MIB_CE_MBSSIB_T));
	mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Wlan_Entry); //vwlan_idx is the idx of vap
	vap_config->macaddr_acl = Wlan_Entry.acl_enable;
#else
	mib_local_mapping_get(MIB_WLAN_AC_ENABLED, if_idx, &vap_config->macaddr_acl);
#endif
	if(vap_config->macaddr_acl != 0){
#if 0 //def WLAN_VAP_ACL
			entryNum = mib_chain_total(MIB_WLAN_AC_TBL+vwlan_idx*WLAN_MIB_MAPPING_STEP);
#else
			entryNum = mib_chain_local_mapping_total(MIB_WLAN_AC_TBL, if_idx);
#endif
			if ( entryNum >= MAX_ACL_NUM ){
				printf("[%s][%d] entryNum overflow ! \n", __FUNCTION__, __LINE__);
				entryNum = MAX_ACL_NUM;
			}

			for (i=0; i<entryNum; i++)
			{
#if 0 //def WLAN_VAP_ACL
				if (!mib_chain_get(MIB_WLAN_AC_TBL+vwlan_idx*WLAN_MIB_MAPPING_STEP, i, (void *)&macEntry))
#else
				if (!mib_chain_local_mapping_get(MIB_WLAN_AC_TBL, if_idx, i, (void *)&macEntry))
#endif
					printf("[%s][%d] mib chain get fail ! \n", __FUNCTION__, __LINE__);

				memcpy(vap_config->mac_acl_list[j]._macaddr,macEntry.macAddr,sizeof(vap_config->mac_acl_list[j]._macaddr));
				vap_config->mac_acl_list[j].used=1;
				j++;
			}
	}
#endif
#endif

	return 0;
}
#endif /* WLAN_MBSSID_NUM */

int get_wifi_conf(struct wifi_config *config, int if_idx)
{
	unsigned char 	tmp8;
	unsigned short 	tmp16;
	unsigned int 	tmp32;
	unsigned char tmpbuf[50];
	MIB_CE_MBSSIB_T Entry;
	//char *name_interface;
	struct sockaddr hwaddr;
	int		entryNum=0, i, is_mib_change=0;
#ifdef WLAN_ACL
	int j=0;
	MIB_CE_WLAN_AC_T macEntry;

#ifdef WLAN_VAP_ACL
	int vwlan_idx = 0;
	MIB_CE_MBSSIB_T Wlan_Entry;
#endif
#endif
#ifdef RTK_MULTI_AP
	unsigned char map_mode=0;
	MIB_CE_MBSSIB_T map_entry;
#endif
#ifdef _PRMT_X_WLANFORISP_
	MIB_WLANFORISP_T wlan_isp_entry;
#endif

#ifdef WLAN_CLIENT
	MIB_CE_MBSSIB_T Vap_Entry;
#endif

	char *rates_list[12] = {"10", "20", "55", "110", "60", "90", "120", "180", "240", "360", "480", "540"};

	printf("[%s][%d][if_idx = %d] ++ \n", __FUNCTION__, __LINE__,if_idx);

#ifdef WLAN_UNIVERSAL_REPEATER
	if(config->is_repeater)
	{
		if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, WLAN_REPEATER_ITF_INDEX, (void *)&Entry)){
			printf("Error! Get MIB_MBSSIB_TBL for vxd SSID err");
			return -1;
		}
	}
	else
#endif
	{
		if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, 0, (void *)&Entry)){
			printf("Error! Get MIB_MBSSIB_TBL for root SSID err");
			return -1;
		}
	}

	strcpy(config->driver, NAME_DRIVER);

	//name_interface = NAME_INTF(if_idx);

#ifdef WLAN_UNIVERSAL_REPEATER
	if(config->is_repeater)
	{
		rtk_wlan_get_ifname(if_idx, WLAN_REPEATER_ITF_INDEX, config->interface);
		strcpy(config->ctrl_interface, WPAS_NAME_CTRL_IFACE);
	}
	else
#endif
	{
		strcpy(config->interface, WLANIF[if_idx]);
#ifdef WLAN_CLIENT
		config->wlan_mode = Entry.wlanMode;
		if(Entry.wlanMode == AP_MODE)
		{
			strcpy(config->ctrl_interface, NAME_CTRL_IFACE);
#ifdef CONFIG_MBO_SUPPORT
			config->mbo_enable = Entry.mbo_enable;
#endif
		}
		else if(Entry.wlanMode == CLIENT_MODE)
		{
			strcpy(config->ctrl_interface, WPAS_NAME_CTRL_IFACE);
		}
#else
		strcpy(config->ctrl_interface, NAME_CTRL_IFACE);
#ifdef CONFIG_MBO_SUPPORT
		config->mbo_enable = Entry.mbo_enable;
#endif
#endif
	}
	strcpy(config->bridge, BRIF);


	sprintf(config->ssid, "%s", Entry.ssid);
	mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, if_idx, 	&config->phyband);
	mib_local_mapping_get(MIB_WLAN_AUTO_CHAN_ENABLED, if_idx, &tmp8);
	if(tmp8==1)
		config->channel = 0;
	else
		mib_local_mapping_get(MIB_WLAN_CHAN_NUM, if_idx, &config->channel);
	mib_local_mapping_get(MIB_WLAN_CHANNEL_WIDTH, if_idx, &config->channel_bandwidth);
	if((config->phyband == PHYBAND_2G) && (config->channel_bandwidth == 1)){
		mib_local_mapping_get(MIB_WLAN_11N_COEXIST, if_idx, &tmp8);
		if(tmp8 == 0)
			config->force_2g_40m = 1;
	}

#ifdef WLAN_6G_SUPPORT
	mib_local_mapping_get(MIB_WLAN_6G_SUPPORT, if_idx, &config->is_6ghz_enable);
	if(config->is_6ghz_enable){
		// TODO: chanlist for 6ghz with auto channel
	}
	else
#endif
	{
		//workaround for country code CN
		if((config->channel==0) && (config->phyband == PHYBAND_5G)){
			if(config->channel_bandwidth == CHANNEL_WIDTH_160)
				snprintf(config->chanlist, sizeof(config->chanlist), "36-48");//remove dfs channel, dfs not ready
			else if(config->channel_bandwidth == CHANNEL_WIDTH_40 || config->channel_bandwidth == CHANNEL_WIDTH_80)
				snprintf(config->chanlist, sizeof(config->chanlist), "36-48 149-161");//remove dfs channel, dfs not ready
			else
				snprintf(config->chanlist, sizeof(config->chanlist), "36-48 149-165"); //remove dfs channel, dfs not ready
		}
#if 0 /* def CONFIG_YUEME */
		if((config->channel==0) && (config->phyband == PHYBAND_2G)){
			snprintf(config->chanlist, sizeof(config->chanlist), "1,6,11");
		}
#endif
	}

	mib_local_mapping_get(MIB_WLAN_CONTROL_BAND, if_idx, &config->control_band);
	mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, if_idx, &config->phy_band_select);
//	mib_local_mapping_get(MIB_TX_POWER, if_idx, &config->tx_power);
//	config->tx_restrict = Entry.tx_restrict;
//	config->rx_restrict = Entry.rx_restrict;
	mib_get(MIB_WLAN_11N_COEXIST, &config->coexist);

	config->disabled = Entry.wlanDisabled;

#if 0
//hostapd ACS chanlist
//2.4G only for channel 1,6,11
//5G only for non-DFS channel
	int radio_index_2g, radio_index_5g;
#if defined (WLAN1_5G_SUPPORT)
	radio_index_2g = 0;
	radio_index_5g = 1;
#else
	radio_index_2g = 1;
	radio_index_5g = 0;
#endif
	if (if_idx == radio_index_5g)
	{
		if(config->channel_bandwidth == CHANNEL_WIDTH_40 || config->channel_bandwidth == CHANNEL_WIDTH_80)
			strcpy(config->chanlist, "36-48 149-161");
		else
			strcpy(config->chanlist, "36-48 149-165");
	}
	else if (if_idx == radio_index_2g)
		strcpy(config->chanlist, "1 6 11");
#endif

#if 1
//Security
	config->encrypt = Entry.encrypt;
	if(Entry.wpaAuth == WPA_AUTH_PSK)
		config->psk_enable = Entry.encrypt/2;
#ifdef WLAN_WPA3_H2E
	if(config->psk_enable & PSK_WPA3)
		config->sae_pwe = Entry.sae_pwe;
#endif

	strcpy(config->passphrase, Entry.wpaPSK);
	config->gk_rekey = Entry.wpaGroupRekeyTime;
	config->authtype = Entry.authType;;
	config->psk_format = Entry.wpaPSKFormat;
#ifdef WLAN_WPA3
	if(Entry.encrypt == WIFI_SEC_WPA3 || Entry.encrypt == WIFI_SEC_WPA2_WPA3_MIXED)
	{
		config->wpa_cipher = 0;
		config->wpa2_cipher = WPA_CIPHER_AES;
	}
	else
#endif
	{
		config->wpa_cipher = Entry.unicastCipher;
		config->wpa2_cipher = Entry.wpa2UnicastCipher;
	}

#if defined(CONFIG_RTL_CFG80211_WAPI_SUPPORT)
	if (Entry.encrypt == WIFI_SEC_WAPI) {
		unsigned char certCount = 0;
		mib_get_s(MIB_WAPI_CERT_COUNT, (void *)&certCount, sizeof(certCount));
	
		config->psk_enable = PSK_CERT_WAPI;
		config->wapiAuthType = Entry.wapiAuthType;
		if (config->wapiAuthType == WAPI_AUTH_PSK) {
			snprintf(config->passphrase, sizeof(config->passphrase), "%s", Entry.wapiPsk);
		} else if (config->wapiAuthType == WAPI_AUTH_CERT) {
			config->wapiRole = Entry.wapiRole;
			if (config->wapiRole == WAPI_AE)
				memcpy(config->wapiAsuIpAddr, Entry.wapiAsuIpAddr, sizeof(Entry.wapiAsuIpAddr));			
		}
		config->wapiCryptoAlg = Entry.wapiCryptoAlg;
		config->wapiCertCount = certCount;
		config->wapiKeyUpdateTime = Entry.wapiKeyUpdateTime;
	}	
#endif

	if(Entry.enable1X)
		config->enable_1x = Entry.enable1X;
	else if(Entry.wpaAuth == WPA_AUTH_AUTO && Entry.encrypt >= WIFI_SEC_WPA)
		config->enable_1x = 1;

#ifdef WLAN_11W
	config->dot11IEEE80211W = Entry.dotIEEE80211W;
	config->enableSHA256 = Entry.sha256;
#endif

	if(Entry.encrypt == WIFI_SEC_WEP){
		config->wepDefaultKey = Entry.wepDefaultKey;
		config->wepKeyType = Entry.wepKeyType;
		if(Entry.wep == WEP64){
			config->encmode = _WEP_40_PRIVACY_;
			memcpy(config->wepkey0, Entry.wep64Key1, sizeof(Entry.wep64Key1));
			memcpy(config->wepkey1, Entry.wep64Key2, sizeof(Entry.wep64Key2));
			memcpy(config->wepkey2, Entry.wep64Key3, sizeof(Entry.wep64Key3));
			memcpy(config->wepkey3, Entry.wep64Key4, sizeof(Entry.wep64Key4));
			/*
			if(Entry.wepDefaultKey == 0)
				memcpy(config->wepkey0, Entry.wep64Key1, sizeof(Entry.wep64Key1));
			else if(Entry.wepDefaultKey == 1)
				memcpy(config->wepkey0, Entry.wep64Key2, sizeof(Entry.wep64Key2));
			else if(Entry.wepDefaultKey == 2)
				memcpy(config->wepkey0, Entry.wep64Key3, sizeof(Entry.wep64Key3));
			else if(Entry.wepDefaultKey == 3)
				memcpy(config->wepkey0, Entry.wep64Key4, sizeof(Entry.wep64Key4));
			*/
		}
		else if(Entry.wep == WEP128){
			config->encmode = _WEP_104_PRIVACY_;
			memcpy(config->wepkey0, Entry.wep128Key1, sizeof(Entry.wep128Key1));
			memcpy(config->wepkey1, Entry.wep128Key2, sizeof(Entry.wep128Key2));
			memcpy(config->wepkey2, Entry.wep128Key3, sizeof(Entry.wep128Key3));
			memcpy(config->wepkey3, Entry.wep128Key4, sizeof(Entry.wep128Key4));
			/*
			if(Entry.wepDefaultKey == 0)
				memcpy(config->wepkey0, Entry.wep128Key1, sizeof(Entry.wep128Key1));
			else if(Entry.wepDefaultKey == 1)
				memcpy(config->wepkey0, Entry.wep128Key2, sizeof(Entry.wep128Key2));
			else if(Entry.wepDefaultKey == 2)
				memcpy(config->wepkey0, Entry.wep128Key3, sizeof(Entry.wep128Key3));
			else if(Entry.wepDefaultKey == 3)
				memcpy(config->wepkey0, Entry.wep128Key4, sizeof(Entry.wep128Key4));
			*/
		}
	}
	else if(Entry.encrypt)
		config->encmode = _IEEE8021X_PSK_;
	else
		config->encmode = _NO_PRIVACY_;

#ifndef WLAN_11AX
	mib_local_mapping_get(MIB_WLAN_AUTH_TYPE, if_idx, 		&config->authtype);
	mib_local_mapping_get(MIB_WLAN_ENCRYPT, if_idx, 			&config->encmode);
	mib_local_mapping_get(MIB_WLAN_WPA_PSK_FORMAT, if_idx, 	&config->psk_format);
	mib_local_mapping_get(MIB_WLAN_WPA_CIPHER_SUITE, if_idx, 	&config->wpa_cipher);
	mib_local_mapping_get(MIB_WLAN_WPA2_CIPHER_SUITE, if_idx, &config->wpa2_cipher);
	mib_local_mapping_get(MIB_WLAN_ENABLE_1X, if_idx, 		&config->enable_1x);
#ifdef WLAN_11W
	mib_local_mapping_get(MIB_WLAN_DOTIEEE80211W, if_idx, 	&config->dot11IEEE80211W);
#endif
#endif

	printf("config->encrypt=%d psk_enable=%d authtype=%d encmode=%d psk_format=%d gk_rekey=%d\n",
		config->encrypt, config->psk_enable, config->authtype,
		config->encmode, config->psk_format, config->gk_rekey);

	printf("%s \n", config->passphrase);

	printf("wpa_cipher=0x%x wpa2_cipher=0x%x enable_1x=%d\n", config->wpa_cipher, config->wpa2_cipher, config->enable_1x);
#ifdef WLAN_11W
	printf("dot11IEEE80211W=%d enableSHA256=%d\n", config->dot11IEEE80211W, config->enableSHA256);
#endif
#ifdef WLAN_WPA3_H2E
	printf("sae_pwe=%d\n", config->sae_pwe);
#endif
#endif

//RADIUS
	mib_get(MIB_ADSL_LAN_IP, config->lan_ip);

	config->rsPort = Entry.rsPort;
	memcpy(config->rsIpAddr, Entry.rsIpAddr, sizeof(Entry.rsIpAddr));
	memcpy(config->rsPassword, Entry.rsPassword, sizeof(Entry.rsPassword));

	config->rs2Port = Entry.rs2Port;
	memcpy(config->rs2IpAddr, Entry.rs2IpAddr, sizeof(Entry.rs2IpAddr));
	memcpy(config->rs2Password, Entry.rs2Password, sizeof(Entry.rs2Password));

	mib_local_mapping_get(MIB_WLAN_RS_INTERVAL_TIME, if_idx, &tmp16);
	config->radius_retry_primary_interval = tmp16*60;

	mib_local_mapping_get(MIB_WLAN_ACCOUNT_RS_INTERVAL_TIME, if_idx, &tmp16);
	config->radius_acct_interim_interval = tmp16*60;
	
#ifdef _PRMT_X_WLANFORISP_
	if(config->enable_1x==1 && rtk_wlan_get_wlanforisp_entry(if_idx, 0, &wlan_isp_entry)==0){
		memcpy(config->nas_identifier, wlan_isp_entry.NasID, strlen(wlan_isp_entry.NasID));
		if(wlan_isp_entry.RadiusAccountEnable==0)
			config->disable_acct = 1;
	}
#endif

//Operation Mode
#ifdef WLAN_BAND_CONFIG_MBSSID
	config->wlan_band = Entry.wlanBand;
#else
	mib_local_mapping_get(MIB_WLAN_BAND, if_idx, &config->wlan_band);
#endif

//HW Setting & Features

//	if(if_idx==0){

#ifdef IS_AX_SUPPORT
	mib_get(MIB_ELAN_MAC_ADDR, tmpbuf);
#ifdef WLAN_ETHER_MAC_ADDR_NEED_OFFSET
	setup_mac_addr(tmpbuf, if_idx * (1+NUM_VWLAN_INTERFACE) + WLAN_ETHER_MAC_ADDR_OFFSET_VALUE);
#else
	setup_mac_addr(tmpbuf, if_idx * (1+NUM_VWLAN_INTERFACE));
#endif
#ifdef WLAN_UNIVERSAL_REPEATER
		if(config->is_repeater)
		setup_mac_addr(tmpbuf, NUM_VWLAN_INTERFACE);
#endif
	sprintf(config->bssid, "%02x:%02x:%02x:%02x:%02x:%02x", tmpbuf[0], tmpbuf[1],
		tmpbuf[2], tmpbuf[3], tmpbuf[4], tmpbuf[5]);
#else
	//already set in startup.c
	getInAddr((char *)WLANIF[if_idx], HW_ADDR, &hwaddr);
	memcpy(tmpbuf, hwaddr.sa_data, MAC_ADDR_LEN);
#ifdef WLAN_UNIVERSAL_REPEATER
	if(config->is_repeater){
#ifdef WLAN_GEN_MBSSID_MAC
		//getInAddr((char *)WLANIF[if_idx], HW_ADDR, &hwaddr);
		_gen_guest_mac(hwaddr.sa_data, MAX_WLAN_VAP+1, WLAN_REPEATER_ITF_INDEX, tmpbuf);
#else
		//getInAddr((char *)WLANIF[if_idx], HW_ADDR, &hwaddr);
		memcpy(tmpbuf, hwaddr.sa_data, MAC_ADDR_LEN);
		setup_mac_addr(tmpbuf, WLAN_REPEATER_ITF_INDEX);
#endif
	}
#endif
	sprintf(config->bssid, "%02x:%02x:%02x:%02x:%02x:%02x", tmpbuf[0], tmpbuf[1],
		tmpbuf[2], tmpbuf[3], tmpbuf[4], tmpbuf[5]);
#endif
/*	}else {
		mib_get(MIB_WLAN_MAC_ADDR, tmpbuf);

		sprintf(config->bssid, "%02x:%02x:%02x:%02x:%02x:%02x", tmpbuf[0], tmpbuf[1],
			tmpbuf[2], tmpbuf[3], tmpbuf[4], tmpbuf[5]);
	}*/

	mib_get(MIB_HW_WLAN0_COUNTRYSTR, &config->country);
	//mib_local_mapping_get(MIB_WLAN_QoS, if_idx, &config->qos);
	config->qos = Entry.wmmEnabled;

//Advanced
	mib_local_mapping_get(MIB_WLAN_FRAG_THRESHOLD, if_idx, (void *)tmpbuf);
	config->fragthres = (int)(*(unsigned short *)tmpbuf);

	mib_local_mapping_get(MIB_WLAN_RTS_THRESHOLD, if_idx, (void *)tmpbuf);
	config->rtsthres = (int)(*(unsigned short *)tmpbuf);

	mib_local_mapping_get(MIB_WLAN_BEACON_INTERVAL, if_idx, (void *)tmpbuf);
	config->bcnint = (int)(*(unsigned short *)tmpbuf);

	mib_local_mapping_get(MIB_WLAN_DTIM_PERIOD, if_idx, 		&config->dtimperiod);
	mib_local_mapping_get(MIB_WLAN_PREAMBLE_TYPE, if_idx, 	&config->preamble);
	mib_local_mapping_get(MIB_WLAN_SHORTGI_ENABLED, if_idx, 	&config->shortGI);
	//mib_local_mapping_get(MIB_WLAN_QoS, if_idx, 				&config->qos_enable);
	//mib_local_mapping_get(MIB_WLAN_HIDDEN_SSID, if_idx, 		&config->ignore_broadcast_ssid);
	config->ignore_broadcast_ssid = Entry.hidessid;

	//mib_local_mapping_get(MIB_WLAN_MCAST2UCAST_DISABLE, if_idx, 		&tmp8);

	config->multicast_to_unicast=Entry.mc2u_disable? 0:1;
	//mib_local_mapping_get(MIB_WLAN_BLOCK_RELAY, if_idx, 	&config->ap_isolate);
	config->ap_isolate = Entry.userisolation;

	mib_local_mapping_get(MIB_WLAN_INACTIVITY_TIME, if_idx, &tmp32);
	config->ap_max_inactivity = tmp32/100;
#ifdef HS2_SUPPORT
	mib_local_mapping_get(MIB_WLAN_HS2_ENABLED, if_idx, &config->hs20);
#endif
	//mib_local_mapping_get(MIB_WLAN_STBC_ENABLED, if_idx, &config->stbc); //TODO driver not support
	//mib_local_mapping_get(MIB_WLAN_LDPC_ENABLED, if_idx, &config->ldpc); //TODO driver not support
	mib_local_mapping_get(MIB_WLAN_TDLS_PROHIBITED, if_idx, &config->tdls_prohibit);
	mib_local_mapping_get(MIB_WLAN_TDLS_CS_PROHIBITED, if_idx, &config->tdls_prohibit_chan_switch);
#ifdef CONFIG_GUEST_ACCESS_SUPPORT
	if(Entry.guest_access == 0)
		config->guest_access = 0;
	else
		config->guest_access = 1;
#endif
	//printf("[%s][%d] -- \n", __FUNCTION__, __LINE__);

//11KV
#ifdef WLAN_11K
	if(Entry.rm_activated){
		config->rrm_beacon_report = 1;
		config->rrm_neighbor_report = 1;
	}
#endif
#ifdef WLAN_11V
	if(Entry.BssTransEnable)
		config->bss_transition = 1;
#endif
#ifdef WLAN_ROAMING
	config->rrm_beacon_report = 1;
	config->rrm_neighbor_report = 1;
	config->bss_transition = 1;
#endif

//11R
#ifdef WLAN_11R
	config->ft_enable = Entry.ft_enable;
	if(Entry.ft_enable){
		memcpy(config->ft_mdid, Entry.ft_mdid, sizeof(config->ft_mdid));
		config->ft_over_ds = Entry.ft_over_ds;
		memcpy(config->nas_identifier, Entry.ft_r0kh_id, sizeof(config->ft_r0kh_id));
		memcpy(config->ft_r0kh_id, Entry.ft_r0kh_id, sizeof(config->ft_r0kh_id));
		config->ft_push = Entry.ft_push;
		/*Key Holder Configuration*/
		//r1_key_holder
		memset(config->r1_key_holder, 0, sizeof(config->r1_key_holder));
		snprintf(config->r1_key_holder, sizeof(config->r1_key_holder), "%s", config->bssid);
		changeMacFormatWlan(config->r1_key_holder, ':');
		//r0kh & r1kh
		MIB_CE_WLAN_FTKH_T khEntry;

		entryNum = mib_chain_total(MIB_WLAN_FTKH_TBL);
		if ( entryNum >= MAX_WLFTKH_NUM ){
			printf("[%s][%d] entryNum overflow ! \n", __FUNCTION__, __LINE__);
			entryNum = MAX_WLFTKH_NUM;
		}
		for(i = 0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_WLAN_FTKH_TBL, i ,(void*)&khEntry))
				printf("[%s][%d] mib chain get fail ! \n", __FUNCTION__, __LINE__);
			memcpy(config->wlftkh_list[i].addr, khEntry.addr, sizeof(config->wlftkh_list[i].addr));
			memcpy(config->wlftkh_list[i].r0kh_id, khEntry.r0kh_id, sizeof(config->wlftkh_list[i].r0kh_id));
			memcpy(config->wlftkh_list[i].key, khEntry.key, sizeof(config->wlftkh_list[i].key));
		}
	}
#endif

//WPS
#ifdef WLAN_WPS
#ifdef WLAN_WPS_VAP
	//config->wps_state = Entry.wsc_disabled? 0:2;
	config->wps_state = Entry.wsc_disabled? 0:(Entry.wsc_configured? 2:1);
#else
	mib_local_mapping_get(MIB_WSC_DISABLE, if_idx, (void *)&tmp8);
	if(tmp8 == 0){
		mib_get_s(MIB_WSC_CONFIGURED, (void *)&tmp8, sizeof(tmp8));
		config->wps_state = (tmp8==1)? 2:1;
	}
	else
		config->wps_state = 0;
#endif
#ifdef WLAN_WPA3_NOT_SUPPORT_WPS
	if(config->wps_state && Entry.encrypt == WIFI_SEC_WPA3)
		config->wps_state = 0;
#endif

	if(config->wps_state){
		// search UUID field, replace last 12 char with hw mac address
		getMIB2Str(MIB_ELAN_MAC_ADDR, tmpbuf);
		memcpy(uuid_def+strlen(uuid_def)-12, tmpbuf, 12);
		strcpy(config->uuid, uuid_def);

		mib_get_s(MIB_SNMP_SYS_NAME, (void *)config->device_name, sizeof(config->device_name));
		strcpy(config->model_name, WPS_MODEL_NAME);
		strcpy(config->model_number, WPS_MODEL_NUMBER);
		strcpy(config->serial_number, WPS_SERIAL_NUMBER);
		strcpy(config->device_type, WPS_DEVICE_TYPE);
		mib_get_s(MIB_WSC_METHOD, (void *)&config->config_method, sizeof(config->config_method));
		mib_get_s(MIB_WSC_PIN, (void *)config->ap_pin, sizeof(config->ap_pin));
		if(Entry.wsc_upnp_enabled)
			strcpy(config->upnp_iface, BRIF);
		else
			config->upnp_iface[0] = '\0';
		strcpy(config->friendly_name, WPS_FRIENDLY_NAME);
		strcpy(config->manufacturer_url, WPS_MANUFACTURER_URL);
		strcpy(config->model_url, WPS_MODEL_URL);
		strcpy(config->manufacturer, WPS_MANUFACTURER);
#ifdef WLAN_WPS_VAP
		config->wps_independent=1;
#endif
	}
#endif //WLAN_WPS

	//multi ap backhaul bss config
#ifdef RTK_MULTI_AP
	mib_get_s(MIB_MAP_CONTROLLER, (void *)&(map_mode), sizeof(map_mode));
	if(map_mode == MAP_CONTROLLER || map_mode == MAP_CONTROLLER_WFA || map_mode == MAP_CONTROLLER_WFA_R2 || map_mode == MAP_CONTROLLER_WFA_R3)
	{
		config->multi_ap = 0;
		if(Entry.multiap_bss_type & MAP_FRONTHAUL_BSS){
			config->multi_ap += 2;
		}
		if(Entry.multiap_bss_type & MAP_BACKHAUL_BSS){
			config->multi_ap += 1;
		}
		for(i=0;i<=NUM_VWLAN_INTERFACE; i++)
		{
			memset(&map_entry, 0, sizeof(MIB_CE_MBSSIB_T));
			if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, i, (void *)&map_entry))
			{
				printf("Error! Get MIB_MBSSIB_TBL for vap SSID err");
				continue;
			}

			if(map_entry.wlanDisabled == 0)
			{
				if(map_entry.multiap_bss_type & MAP_BACKHAUL_BSS)
				{
#ifdef WLAN_WPS
					snprintf(config->multi_ap_backhaul_ssid, sizeof(config->multi_ap_backhaul_ssid), "%s", map_entry.ssid);
					strcpy(config->multi_ap_backhaul_passphrase, map_entry.wpaPSK);
					config->multi_ap_backhaul_psk_format = map_entry.wpaPSKFormat;
#endif
					break;
				}

			}
		}
	}
	else if(map_mode == MAP_AGENT || map_mode == MAP_AGENT_WFA || map_mode == MAP_AGENT_WFA_R2 || map_mode == MAP_AGENT_WFA_R3)
	{
#ifdef WLAN_UNIVERSAL_REPEATER
		if(config->is_repeater)
		{
			memset(&map_entry, 0, sizeof(MIB_CE_MBSSIB_T));
			if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, WLAN_REPEATER_ITF_INDEX, (void *)&map_entry))
			{
				printf("Error! Get MIB_MBSSIB_TBL for vxd SSID err");
				return -1;
			}

			if(map_entry.wlanDisabled == 0)
			{
				if(map_entry.multiap_bss_type == MAP_BACKHAUL_STA)
				{
					config->multi_ap_backhaul_sta = 1;
				}
				else
					config->multi_ap_backhaul_sta = 0;
			}
		}
		else
#endif
		{
			config->multi_ap = 0;
			if(Entry.multiap_bss_type & MAP_FRONTHAUL_BSS){
				config->multi_ap += 2;
			}
			if(Entry.multiap_bss_type & MAP_BACKHAUL_BSS){
				config->multi_ap += 1;
			}

			for(i=0;i<=NUM_VWLAN_INTERFACE; i++)
			{
				memset(&map_entry, 0, sizeof(MIB_CE_MBSSIB_T));
				if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, i, (void *)&map_entry))
				{
					printf("Error! Get MIB_MBSSIB_TBL for vap SSID err");
					continue;
				}

				if(map_entry.wlanDisabled == 0)
				{
					if(map_entry.multiap_bss_type & MAP_BACKHAUL_BSS)
					{
#ifdef WLAN_WPS
						snprintf(config->multi_ap_backhaul_ssid, sizeof(config->multi_ap_backhaul_ssid), "%s", map_entry.ssid);
						strcpy(config->multi_ap_backhaul_passphrase, map_entry.wpaPSK);
						config->multi_ap_backhaul_psk_format = map_entry.wpaPSKFormat;
#endif
						break;
					}

				}
			}
		}

	}
#endif //RTK_MULTI_AP


//supported_rates
	mib_local_mapping_get(MIB_WLAN_SUPPORTED_RATE, if_idx, (void *)&tmp16);
	tmp32 = (int)tmp16;
	memset(config->supported_rates, 0, sizeof(config->supported_rates));

	for(i=0; i<12; i++)
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		tmp16 = tmp32 & 0x01;
	
		if(tmp16 == 1)
		{
			snprintf(tmpbuf, sizeof(tmpbuf), "%s ", rates_list[i]);
			strncat(config->supported_rates, tmpbuf, sizeof(tmpbuf));
		}
	
		tmp32 = tmp32 >> 1;
	}
	config->supported_rates[strlen(config->supported_rates)-1] = '\0';

	//basic_rates
	mib_local_mapping_get(MIB_WLAN_BASIC_RATE, if_idx, (void *)&tmp16);
	tmp32 = (int)tmp16;
	memset(config->basic_rates, 0, sizeof(config->basic_rates));

	for(i=0; i<12; i++)
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		tmp16 = tmp32 & 0x01;
		if(tmp16 == 1)
		{
			snprintf(tmpbuf, sizeof(tmpbuf), "%s ", rates_list[i]);
			strncat(config->basic_rates, tmpbuf, strlen(tmpbuf));
		}
		tmp32 = tmp32 >> 1;
	}
	config->basic_rates[strlen(config->basic_rates)-1] = '\0';

#if defined(WLAN_MBSSID_NUM) && (WLAN_MBSSID_NUM > 0)
#ifdef WLAN_UNIVERSAL_REPEATER
	if(config->is_repeater == 0)
#endif
	{
#ifdef WLAN_CLIENT
		if(Entry.wlanMode == AP_MODE)
		{
			for (i = WLAN_VAP_ITF_INDEX; i < (WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM); i++) {
				get_wifi_vif_conf(&config[i], if_idx, i);
			}
		}
		else if(Entry.wlanMode == CLIENT_MODE)
		{
			for (i = 0; i < WLAN_MBSSID_NUM; i++) {
				memset(&Vap_Entry,0,sizeof(MIB_CE_MBSSIB_T));
				mib_chain_get(MIB_MBSSIB_TBL, i+1, (void *)&Vap_Entry); //vwlan_idx is the idx of vap
				if(Vap_Entry.wlanDisabled == 0)
				{
					Vap_Entry.wlanDisabled = 1;
					mib_chain_update(MIB_MBSSIB_TBL, (void *)&Vap_Entry, i+1);
					is_mib_change = 1;
				}
			}
		}
#else
		for (i = WLAN_VAP_ITF_INDEX; i < (WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM); i++) {
			get_wifi_vif_conf(&config[i], if_idx, i);
		}
#endif
	}
#endif

//acl
#ifdef WLAN_ACL
#ifdef WLAN_VAP_ACL
	memset(&Wlan_Entry,0,sizeof(MIB_CE_MBSSIB_T));
	mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Wlan_Entry); //vwlan_idx is the idx of vap
	config->macaddr_acl = Wlan_Entry.acl_enable;
#else
	mib_local_mapping_get(MIB_WLAN_AC_ENABLED, if_idx, &config->macaddr_acl);
#endif
	if(config->macaddr_acl != 0){
#ifdef WLAN_VAP_ACL
			entryNum = mib_chain_total(MIB_WLAN_AC_TBL+vwlan_idx*WLAN_MIB_MAPPING_STEP);
#else
			entryNum = mib_chain_local_mapping_total(MIB_WLAN_AC_TBL, if_idx);
#endif
			if ( entryNum >= MAX_ACL_NUM ){
				printf("[%s][%d] entryNum overflow ! \n", __FUNCTION__, __LINE__);
				entryNum = MAX_ACL_NUM;
			}

			for (i=0; i<entryNum; i++)
			{
#ifdef WLAN_VAP_ACL
				if (!mib_chain_get(MIB_WLAN_AC_TBL+vwlan_idx*WLAN_MIB_MAPPING_STEP, i, (void *)&macEntry))
#else
				if (!mib_chain_local_mapping_get(MIB_WLAN_AC_TBL, if_idx, i, (void *)&macEntry))
#endif
					printf("[%s][%d] mib chain get fail ! \n", __FUNCTION__, __LINE__);

				memcpy(config->mac_acl_list[j]._macaddr,macEntry.macAddr,sizeof(config->mac_acl_list[j]._macaddr));
				config->mac_acl_list[j].used=1;
				j++;
			}
	}
#endif

#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
	if(config->wlan_mode == CLIENT_MODE || config->is_repeater)
	{
		config->update_config = 1;
		config->scan_ssid = 1;
		config->pbss = 2;
	}
#endif

	if(is_mib_change)
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);

	return 0;
}

int get_wifi_wpas_conf(struct wifi_config *config, int if_idx)
{
	unsigned char 	tmp8;
	unsigned short 	tmp16;
	unsigned int 	tmp32;
	unsigned char tmpbuf[50];
	MIB_CE_MBSSIB_T Entry;
	//char *name_interface;
	struct sockaddr hwaddr;
	int		i;
#ifdef RTK_MULTI_AP
	unsigned char map_mode=0;
	MIB_CE_MBSSIB_T map_entry;
#endif
	unsigned char gen_wlan_mac[6]={0};

	printf("[%s][%d] ++ \n", __FUNCTION__, __LINE__);

#ifdef WLAN_UNIVERSAL_REPEATER
	if(config->is_repeater)
	{
		if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, WLAN_REPEATER_ITF_INDEX, (void *)&Entry)){
			printf("Error! Get MIB_MBSSIB_TBL for vxd SSID err");
			return -1;
		}
	}
	else
#endif
	{
		if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, 0, (void *)&Entry)){
			printf("Error! Get MIB_MBSSIB_TBL for root SSID err");
			return -1;
		}
	}

	//strcpy(config->driver, NAME_DRIVER);

	//name_interface = NAME_INTF(if_idx);

#ifdef WLAN_UNIVERSAL_REPEATER
	if(config->is_repeater)
		snprintf(config->interface, sizeof(config->interface), "%s-vxd", WLANIF[if_idx]);
	else
#endif
	{
#if defined(WLAN_CLIENT)
		config->wlan_mode = Entry.wlanMode;
#endif
		strcpy(config->interface, WLANIF[if_idx]);
	}

#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
	if(Entry.wlanMode == CLIENT_MODE
#if defined(WLAN_UNIVERSAL_REPEATER)
		|| config->is_repeater
#endif
	)
	{
		strcpy(config->ctrl_interface, WPAS_NAME_CTRL_IFACE);
	}
	else
#endif
		strcpy(config->ctrl_interface, NAME_CTRL_IFACE);
	//strcpy(config->bridge, BRIF);


	sprintf(config->ssid, "%s", Entry.ssid);

#if 1
//Security
	config->encrypt = Entry.encrypt;
	if(Entry.wpaAuth == WPA_AUTH_PSK)
		config->psk_enable = Entry.encrypt/2;
#ifdef WLAN_WPA3_H2E
	if(config->psk_enable & PSK_WPA3)
		config->sae_pwe = Entry.sae_pwe;
#endif

	strcpy(config->passphrase, Entry.wpaPSK);
	config->gk_rekey = Entry.wpaGroupRekeyTime;
	config->authtype = Entry.authType;;
	config->psk_format = Entry.wpaPSKFormat;
#ifdef WLAN_WPA3
	if(Entry.encrypt == WIFI_SEC_WPA3 || Entry.encrypt == WIFI_SEC_WPA2_WPA3_MIXED)
	{
		config->wpa_cipher = 0;
		config->wpa2_cipher = WPA_CIPHER_AES;
	}
	else
#endif
	{
		config->wpa_cipher = Entry.unicastCipher;
		config->wpa2_cipher = Entry.wpa2UnicastCipher;
	}

	if(Entry.enable1X)
		config->enable_1x = Entry.enable1X;
	else if(Entry.wpaAuth == WPA_AUTH_AUTO && Entry.encrypt >= WIFI_SEC_WPA)
		config->enable_1x = 1;

#ifdef WLAN_11W
	config->dot11IEEE80211W = Entry.dotIEEE80211W;
	config->enableSHA256 = Entry.sha256;
#endif

	if(Entry.encrypt == WIFI_SEC_WEP){
		config->wepDefaultKey = Entry.wepDefaultKey;
		config->wepKeyType = Entry.wepKeyType;
		if(Entry.wep == WEP64){
			config->encmode = _WEP_40_PRIVACY_;
			memcpy(config->wepkey0, Entry.wep64Key1, sizeof(Entry.wep64Key1));
			memcpy(config->wepkey1, Entry.wep64Key2, sizeof(Entry.wep64Key2));
			memcpy(config->wepkey2, Entry.wep64Key3, sizeof(Entry.wep64Key3));
			memcpy(config->wepkey3, Entry.wep64Key4, sizeof(Entry.wep64Key4));
		}
		else if(Entry.wep == WEP128){
			config->encmode = _WEP_104_PRIVACY_;
			memcpy(config->wepkey0, Entry.wep128Key1, sizeof(Entry.wep128Key1));
			memcpy(config->wepkey1, Entry.wep128Key2, sizeof(Entry.wep128Key2));
			memcpy(config->wepkey2, Entry.wep128Key3, sizeof(Entry.wep128Key3));
			memcpy(config->wepkey3, Entry.wep128Key4, sizeof(Entry.wep128Key4));
		}
	}
	else if(Entry.encrypt)
		config->encmode = _IEEE8021X_PSK_;
	else
		config->encmode = _NO_PRIVACY_;

	printf("psk_enable=%d authtype=%d encmode=%d psk_format=%d gk_rekey=%d\n",
		config->psk_enable, config->authtype,
		config->encmode, config->psk_format, config->gk_rekey);

	printf("%s \n", config->passphrase);

	printf("wpa_cipher=0x%x wpa2_cipher=0x%x enable_1x=%d dot11IEEE80211W=%d enableSHA256=%d\n",
		config->wpa_cipher, config->wpa2_cipher, config->enable_1x,
		config->dot11IEEE80211W, config->enableSHA256);
#ifdef WLAN_WPA3_H2E
	printf("sae_pwe=%d\n", config->sae_pwe);
#endif
#endif
//RADIUS
	mib_get(MIB_ADSL_LAN_IP, config->lan_ip);

	config->rsPort = Entry.rsPort;
	memcpy(config->rsIpAddr, Entry.rsIpAddr, sizeof(Entry.rsIpAddr));
	memcpy(config->rsPassword, Entry.rsPassword, sizeof(Entry.rsPassword));

	config->rs2Port = Entry.rs2Port;
	memcpy(config->rs2IpAddr, Entry.rs2IpAddr, sizeof(Entry.rs2IpAddr));
	memcpy(config->rs2Password, Entry.rs2Password, sizeof(Entry.rs2Password));


//HW Setting & Features
#ifdef WLAN_UNIVERSAL_REPEATER
	if(config->is_repeater)
	{
#ifdef WLAN_GEN_MBSSID_MAC
		getInAddr((char *)WLANIF[if_idx], HW_ADDR, &hwaddr);
		_gen_guest_mac(hwaddr.sa_data, MAX_WLAN_VAP+1, WLAN_REPEATER_ITF_INDEX, gen_wlan_mac);
		sprintf(config->bssid, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned char)gen_wlan_mac[0], (unsigned char)gen_wlan_mac[1],
			(unsigned char)gen_wlan_mac[2], (unsigned char)gen_wlan_mac[3], (unsigned char)gen_wlan_mac[4], (unsigned char)gen_wlan_mac[5]);
#else
		getInAddr((char *)WLANIF[if_idx], HW_ADDR, &hwaddr);
		memcpy(gen_wlan_mac, hwaddr.sa_data, MAC_ADDR_LEN);
		setup_mac_addr(gen_wlan_mac, WLAN_REPEATER_ITF_INDEX);
		sprintf(config->bssid, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned char)gen_wlan_mac[0], (unsigned char)gen_wlan_mac[1],
			(unsigned char)gen_wlan_mac[2], (unsigned char)gen_wlan_mac[3], (unsigned char)gen_wlan_mac[4], (unsigned char)gen_wlan_mac[5]);
		//vap_config->bssid[0]='\0';
#endif
	}
	else
#endif
	{
		getInAddr((char *)WLANIF[if_idx], HW_ADDR, &hwaddr);
		sprintf(config->bssid, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned char)hwaddr.sa_data[0], (unsigned char)hwaddr.sa_data[1],
		(unsigned char)hwaddr.sa_data[2], (unsigned char)hwaddr.sa_data[3], (unsigned char)hwaddr.sa_data[4], (unsigned char)hwaddr.sa_data[5]);
	}

	//printf("[%s][%d] -- \n", __FUNCTION__, __LINE__);

//WPS
#ifdef WLAN_WPS
#ifdef WLAN_WPS_VAP
	//config->wps_state = Entry.wsc_disabled? 0:2;
	config->wps_state = Entry.wsc_disabled? 0:(Entry.wsc_configured? 2:1);
#else
	mib_local_mapping_get(MIB_WSC_DISABLE, if_idx, (void *)&tmp8);
	if(tmp8 == 0){
		mib_get_s(MIB_WSC_CONFIGURED, (void *)&tmp8, sizeof(tmp8));
		config->wps_state = (tmp8==1)? 2:1;
	}
	else
		config->wps_state = 0;
#endif
#ifdef WLAN_WPA3_NOT_SUPPORT_WPS
	if(config->wps_state && Entry.encrypt == WIFI_SEC_WPA3)
		config->wps_state = 0;
#endif

	if(config->wps_state){
		// search UUID field, replace last 12 char with hw mac address
		//getMIB2Str(MIB_ELAN_MAC_ADDR, tmpbuf);
		//memcpy(uuid_def+strlen(uuid_def)-12, tmpbuf, 12);
		//strcpy(config->uuid, uuid_def);

		mib_get_s(MIB_SNMP_SYS_NAME, (void *)config->device_name, sizeof(config->device_name));
		strcpy(config->model_name, WPS_MODEL_NAME);
		strcpy(config->model_number, WPS_MODEL_NUMBER);
		strcpy(config->serial_number, WPS_SERIAL_NUMBER);
		strcpy(config->device_type, WPS_DEVICE_TYPE);
		mib_get_s(MIB_WSC_METHOD, (void *)&config->config_method, sizeof(config->config_method));
		mib_get_s(MIB_WSC_PIN, (void *)config->ap_pin, sizeof(config->ap_pin));
		if(Entry.wsc_upnp_enabled)
			strcpy(config->upnp_iface, BRIF);
		else
			config->upnp_iface[0] = '\0';
		strcpy(config->friendly_name, WPS_FRIENDLY_NAME);
		strcpy(config->manufacturer_url, WPS_MANUFACTURER_URL);
		strcpy(config->model_url, WPS_MODEL_URL);
		strcpy(config->manufacturer, WPS_MANUFACTURER);
#ifdef WLAN_WPS_VAP
		config->wps_independent=1;
#endif
	}
#endif //WLAN_WPS

#ifdef RTK_MULTI_AP
	mib_get_s(MIB_MAP_CONTROLLER, (void *)&(map_mode), sizeof(map_mode));
	if(map_mode == MAP_CONTROLLER || map_mode == MAP_CONTROLLER_WFA || map_mode == MAP_CONTROLLER_WFA_R2 || map_mode == MAP_CONTROLLER_WFA_R3)
	{
		config->multi_ap = 0;
		if(Entry.multiap_bss_type & MAP_FRONTHAUL_BSS){
			config->multi_ap += 2;
		}
		if(Entry.multiap_bss_type & MAP_BACKHAUL_BSS){
			config->multi_ap += 1;
		}
		for(i=0;i<=NUM_VWLAN_INTERFACE; i++)
		{
			memset(&map_entry, 0, sizeof(MIB_CE_MBSSIB_T));
			if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, i, (void *)&map_entry))
			{
				printf("Error! Get MIB_MBSSIB_TBL for vap SSID err");
				continue;
			}

			if(map_entry.wlanDisabled == 0)
			{
				if(map_entry.multiap_bss_type & MAP_BACKHAUL_BSS)
				{
#ifdef WLAN_WPS
					snprintf(config->multi_ap_backhaul_ssid, sizeof(config->multi_ap_backhaul_ssid), "%s", map_entry.ssid);
					strcpy(config->multi_ap_backhaul_passphrase, map_entry.wpaPSK);
					config->multi_ap_backhaul_psk_format = map_entry.wpaPSKFormat;
#endif
					break;
				}

			}
		}
	}
	else if(map_mode == MAP_AGENT || map_mode == MAP_AGENT_WFA || map_mode == MAP_AGENT_WFA_R2 || map_mode == MAP_AGENT_WFA_R3)
	{
#ifdef WLAN_UNIVERSAL_REPEATER
		if(config->is_repeater)
		{
			memset(&map_entry, 0, sizeof(MIB_CE_MBSSIB_T));
			if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, WLAN_REPEATER_ITF_INDEX, (void *)&map_entry))
			{
				printf("Error! Get MIB_MBSSIB_TBL for vxd SSID err");
				return -1;
			}

			if(map_entry.wlanDisabled == 0)
			{
				if(map_entry.multiap_bss_type == MAP_BACKHAUL_STA)
				{
					config->multi_ap_backhaul_sta = 1;
				}
				else
					config->multi_ap_backhaul_sta = 0;
			}
		}
		else
#endif
		{
			config->multi_ap = 0;
			if(Entry.multiap_bss_type & MAP_FRONTHAUL_BSS){
				config->multi_ap += 2;
			}
			if(Entry.multiap_bss_type & MAP_BACKHAUL_BSS){
				config->multi_ap += 1;
			}

			for(i=0;i<=NUM_VWLAN_INTERFACE; i++)
			{
				memset(&map_entry, 0, sizeof(MIB_CE_MBSSIB_T));
				if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, if_idx, i, (void *)&map_entry))
				{
					printf("Error! Get MIB_MBSSIB_TBL for vap SSID err");
					continue;
				}

				if(map_entry.wlanDisabled == 0)
				{
					if(map_entry.multiap_bss_type & MAP_BACKHAUL_BSS)
					{
#ifdef WLAN_WPS
						snprintf(config->multi_ap_backhaul_ssid, sizeof(config->multi_ap_backhaul_ssid), "%s", map_entry.ssid);
						strcpy(config->multi_ap_backhaul_passphrase, map_entry.wpaPSK);
						config->multi_ap_backhaul_psk_format = map_entry.wpaPSKFormat;
#endif
						break;
					}

				}
			}
		}

	}
#endif //RTK_MULTI_AP

//station
#if 1//defined(CONFIG_ELINK_SUPPORT) || defined(CONFIG_ANDLINK_SUPPORT) || defined(CONFIG_USER_ADAPTER_API_ISP)
	config->max_num_sta = Entry.rtl_link_sta_num;
#endif
#ifdef WLAN_LIMITED_STA_NUM
	if ((config->max_num_sta) && (Entry.stanum < config->max_num_sta))
		config->max_num_sta = Entry.stanum;
	else if (!config->max_num_sta)
		config->max_num_sta = Entry.stanum;
#endif

#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
	if(config->wlan_mode == CLIENT_MODE
#if defined(WLAN_UNIVERSAL_REPEATER)
		|| config->is_repeater
#endif
		)

	{
		config->update_config = 1;
		config->scan_ssid = 1;
		config->pbss = 2;
	}
#endif

	return 0;
}


int apply_radius(struct wifi_config *config)
{
	if(config->enable_1x){

		if(config->psk_enable)
			return NO_RADIUS;

		if(config->encmode==_WEP_40_PRIVACY_ || config->encmode==_WEP_104_PRIVACY_)
			return RADIUS_WEP;

		if(config->wpa_cipher || config->wpa2_cipher || config->wpa3_cipher)
			return RADIUS_WPA;

		return RADIUS_OPEN;
	}
	else
	return NO_RADIUS;
}

int apply_radius2(struct wifi_config *config)
{
	if( ((struct in_addr *)config->rs2IpAddr)->s_addr != INADDR_NONE && ((struct in_addr *)config->rs2IpAddr)->s_addr != 0 )
		return 1;
	else
		return 0;
}

int apply_wpa(struct wifi_config *config)
{
	if(config->psk_enable || apply_radius(config)==RADIUS_WPA)
		return 1;
	else
		return 0;
}

int apply_wps(struct wifi_config *config)
{
	if(apply_radius(config)!=NO_RADIUS) //disable wps when radius is enabled
		return 0;
	else if(config->wps_state)
		return 1;
	else
		return 0;
}

#ifdef RTK_MULTI_AP
int apply_multi_ap(struct wifi_config *config)
{
	if(config->multi_ap)
		return 1;
	else
		return 0;
}
#endif

int apply_11r(struct wifi_config *config)
{
#ifdef WLAN_11R
	if((config->encrypt==WIFI_SEC_WPA2 || config->encrypt==WIFI_SEC_WPA2_MIXED) && config->ft_enable)
		return 1;
	else
		return 0;
#else
	return 0;
#endif
}

//======Basic

int hapd_get_driver(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->driver);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_interface(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->interface);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_ctrl_interface(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->ctrl_interface);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_bridge(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->bridge);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_ssid(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
	if(config->wlan_mode == CLIENT_MODE
#if defined(WLAN_UNIVERSAL_REPEATER)
		|| config->is_repeater
#endif
		)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=\"%s\"\n", name, config->ssid);
	else
#endif
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->ssid);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}


//======Security
int hapd_get_auth_algs(struct wifi_config *config, char *name, char *result)
{
	unsigned char auth_algs = WPA_AUTH_ALG_OPEN;
	int len = 0;

	if(config->encrypt == WIFI_SEC_NONE)
		return HAPD_GET_FAIL;
	else if(apply_wpa(config))
		auth_algs = WPA_AUTH_ALG_OPEN;
	else{
		if(config->authtype == AUTH_OPEN)
			auth_algs = WPA_AUTH_ALG_OPEN;
		else if(config->authtype == AUTH_SHARED && config->encmode != _NO_PRIVACY_ && !config->psk_enable)
			auth_algs = WPA_AUTH_ALG_SHARED;
		else if(config->authtype == AUTH_BOTH && config->encmode != _NO_PRIVACY_ && !config->psk_enable)
			auth_algs = WPA_AUTH_ALG_OPEN | WPA_AUTH_ALG_SHARED;
		else
			auth_algs = WPA_AUTH_ALG_OPEN;
	}

	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, auth_algs);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_wpa(struct wifi_config *config, char *name, char *result)
{
	unsigned char wpa = 0;
	int len = 0;

	if(!apply_wpa(config))
		return HAPD_GET_FAIL;

	if(config->encrypt & WIFI_SEC_WPA)
		wpa |= WPA_PROTO_WPA;

	if((config->encrypt & WIFI_SEC_WPA2) || (config->encrypt & WIFI_SEC_WPA3))
		wpa |= WPA_PROTO_RSN;

#if defined(CONFIG_RTL_CFG80211_WAPI_SUPPORT)
	if (config->psk_enable == PSK_CERT_WAPI)
		wpa = WPA_PROTO_WAPI;
#endif

	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, wpa);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_wpa_psk(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if((config->psk_enable & (PSK_WPA|PSK_WPA2)) && config->psk_format == KEY_HEX){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->passphrase);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
	else
		return HAPD_GET_FAIL;
}
int hapd_get_wpa_passphrase(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->psk_enable && config->psk_format == KEY_ASCII){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->passphrase);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
	else
		return HAPD_GET_FAIL;
}

int hapd_get_sae_password(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->psk_enable & PSK_WPA3){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->passphrase);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
	else
		return HAPD_GET_FAIL;
}

#ifdef WLAN_WPA3_H2E
int hapd_get_sae_pwe(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->psk_enable & PSK_WPA3){
#ifdef WLAN_6G_SUPPORT
		if(config->is_6ghz_enable)
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=1\n", name);
		else
#endif
		if(config->psk_enable == (PSK_WPA2|PSK_WPA3))
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=2\n", name);
		else{
#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
			if(config->wlan_mode == CLIENT_MODE
#if defined(WLAN_UNIVERSAL_REPEATER)
				|| config->is_repeater
#endif
				)
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->sae_pwe);
			else
#endif
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=1\n", name);
		}
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
	else
		return HAPD_GET_FAIL;
}
int hapd_get_transition_disable(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->psk_enable == (PSK_WPA2|PSK_WPA3)){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=0x01\n", name);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
	else
		return HAPD_GET_FAIL;
}
#endif

int hapd_get_wpa_key_mgmt(struct wifi_config *config, char *name, char *result)
{
	enum PMF_STATUS cur_pmf_stat;

#ifdef WLAN_11W
	cur_pmf_stat = config->dot11IEEE80211W;
#endif

	int len = 0;
	if(apply_wpa(config)){

		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=", name);

		if((config->psk_enable & (PSK_WPA|PSK_WPA2)) && (cur_pmf_stat!=REQUIRED)){
			len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "WPA-PSK ");

#ifdef WLAN_11R
#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
			if(config->wlan_mode == CLIENT_MODE || config->is_repeater)
			{
				printf("[%s %d]client or vxd not support 11R!\n",__FUNCTION__,__LINE__);
			}
			else
#endif
			{
				if(config->ft_enable)
					len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "FT-PSK ");
			}
#endif
		}
		
		if(config->psk_enable == PSK_WPA2){
			if(config->dot11IEEE80211W != 2)
				len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "WPA-PSK ");
			if((config->dot11IEEE80211W == 2) || (config->enableSHA256 == 1))
				len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "WPA-PSK-SHA256 ");
		}

		if(config->psk_enable == PSK_WPA3)
			len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "SAE ");

		if(config->psk_enable == (PSK_WPA2|PSK_WPA3)){
			len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "WPA-PSK ");
			//if(config->enableSHA256 == 1)
			//	len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "WPA-PSK-SHA256 ");
			len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "SAE ");
		}

		if(apply_radius(config)==RADIUS_WPA){
			if(config->encrypt == WIFI_SEC_WPA2){
				if(config->dot11IEEE80211W != 2)
					len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "WPA-EAP ");
				if((config->dot11IEEE80211W == 2) || (config->enableSHA256 == 1))
					len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "WPA-EAP-SHA256 ");
			}
#ifdef WLAN_WPA3_ENTERPRISE
			else if(config->encrypt == WIFI_SEC_WPA3){ //wpa3 only
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s", name, "WPA-EAP-SUITE-B-192 ");
			}
#endif
			else
				len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "WPA-EAP ");
		}
#if defined(CONFIG_RTL_CFG80211_WAPI_SUPPORT)
		if (config->psk_enable == PSK_CERT_WAPI) {
			if (config->wapiAuthType == WAPI_AUTH_PSK)
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s%s", result, "WAPI-PSK ");
			if (config->wapiAuthType == WAPI_AUTH_CERT)
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s%s", result, "WAPI-CERT ");
		}
#endif

		if(apply_11r(config) && (config->psk_enable == PSK_WPA2))
			len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "FT-PSK ");

		len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "\n");
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;

		return HAPD_GET_OK;
	}
	else
	{
#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
		if(config->wlan_mode == CLIENT_MODE
#if defined(WLAN_UNIVERSAL_REPEATER)
			|| config->is_repeater
#endif
			)
		{
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=NONE\n", name);
			if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
				return HAPD_GET_FAIL;

			return HAPD_GET_OK;
		}
		else
		{
			return HAPD_GET_FAIL;
		}
#else
		return HAPD_GET_FAIL;
#endif
	}

}

int hapd_get_wpa_pairwise(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	
#if defined(CONFIG_RTL_CFG80211_WAPI_SUPPORT)
	if (config->psk_enable == PSK_CERT_WAPI)
		return HAPD_GET_OK;
#endif
	if(!(config->encrypt & WIFI_SEC_WPA))
		return HAPD_GET_FAIL;
	else if(config->wpa_cipher == WPA_CIPHER_MIXED)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "TKIP CCMP");
	else if(config->wpa_cipher == WPA_CIPHER_TKIP)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "TKIP");
	else if(config->wpa_cipher == WPA_CIPHER_AES)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "CCMP");
	else
		return HAPD_GET_FAIL;
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_rsn_pairwise(struct wifi_config *config, char *name, char *result)
{
	int len = 0;

#if defined(CONFIG_RTL_CFG80211_WAPI_SUPPORT)
	if(!((config->encrypt & WIFI_SEC_WPA2) || (config->encrypt & WIFI_SEC_WPA3) || (config->encrypt & WIFI_SEC_WAPI)))
#else
	if(!((config->encrypt & WIFI_SEC_WPA2) || (config->encrypt & WIFI_SEC_WPA3)))
#endif
		return HAPD_GET_FAIL;
	else if((config->encrypt & WIFI_SEC_WPA3))
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "CCMP");
	else if(config->wpa2_cipher == WPA_CIPHER_MIXED)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "TKIP CCMP");
	else if(config->wpa2_cipher == WPA_CIPHER_TKIP)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "TKIP");
	else if(config->wpa2_cipher == WPA_CIPHER_AES)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "CCMP");
#if defined(CONFIG_RTL_CFG80211_WAPI_SUPPORT)
	else if (config->psk_enable == PSK_CERT_WAPI) {
		if (config->wapiCryptoAlg == CIPHER_SMS4) {
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "WPI-SM4");
		} else if (config->wapiCryptoAlg == CIPHER_GCM_SM4) {
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "WPI-GCM-SM4");
		}
	}
#endif
		
	else
		return HAPD_GET_FAIL;

	if(apply_radius(config)==RADIUS_WPA){
#ifdef WLAN_WPA3_ENTERPRISE
		if(config->encrypt == WIFI_SEC_WPA3) //wpa3 only
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "GCMP-256");
#endif
	}
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_group_mgmt_cipher(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->encrypt != WIFI_SEC_WPA3)
		return HAPD_GET_OK;

	if(apply_radius(config)==RADIUS_WPA){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "BIP-GMAC-256");
	}
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_ieee80211w(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_wpa(config))
	{
		if(config->encrypt == WIFI_SEC_WPA3)
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=2\n", name);
		else if(config->encrypt == WIFI_SEC_WPA2_WPA3_MIXED)
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=1\n", name);
		else
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->dot11IEEE80211W);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}
	return HAPD_GET_OK;
}

int hapd_get_wpa_group_rekey(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_wpa(config))
	{
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->gk_rekey);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}
	return HAPD_GET_OK;
}


int hapd_get_wep_key0(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	char wep_key_type = config->wepKeyType;

	if(config->encmode == _WEP_40_PRIVACY_ || config->encmode == _WEP_104_PRIVACY_)
	{
		unsigned idx, key_len = 0;

		if(config->encmode == _WEP_40_PRIVACY_)
			key_len = 5;
		else
			key_len = 13;

		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=", name);

		if(wep_key_type == 0 && strlen(config->wepkey0) == key_len)
			len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "\"" "%s" "\"", config->wepkey0);
		else
		{
			if(wep_key_type == 0)
				memset(config->wepkey0, 0, sizeof(config->wepkey0));
			for(idx=0; idx<key_len; idx++)
				len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "%02x", config->wepkey0[idx]);
		}

		len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "\n");
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}

	return HAPD_GET_FAIL;
}
int hapd_get_wep_key1(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	char wep_key_type = config->wepKeyType;

	if(config->encmode == _WEP_40_PRIVACY_ || config->encmode == _WEP_104_PRIVACY_)
	{
		unsigned idx, key_len = 0;

		if(config->encmode == _WEP_40_PRIVACY_)
			key_len = 5;
		else
			key_len = 13;

		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=", name);

		if(wep_key_type == 0 && strlen(config->wepkey1) == key_len)
			len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "\"" "%s" "\"", config->wepkey1);
		else
		{
			if(wep_key_type == 0)
				memset(config->wepkey1, 0, sizeof(config->wepkey1));
			for(idx=0; idx<key_len; idx++){
				len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "%02x", config->wepkey1[idx]);
			}
		}

		len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "\n");
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}

	return HAPD_GET_FAIL;
}
int hapd_get_wep_key2(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	char wep_key_type = config->wepKeyType;

	if(config->encmode == _WEP_40_PRIVACY_ || config->encmode == _WEP_104_PRIVACY_)
	{
		unsigned idx, key_len = 0;

		if(config->encmode == _WEP_40_PRIVACY_)
			key_len = 5;
		else
			key_len = 13;

		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=", name);

		if(wep_key_type == 0 && strlen(config->wepkey2) == key_len)
			len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "\"" "%s" "\"", config->wepkey2);
		else
		{
			if(wep_key_type == 0)
				memset(config->wepkey2, 0, sizeof(config->wepkey2));
			for(idx=0; idx<key_len; idx++)
				len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "%02x", config->wepkey2[idx]);
		}

		len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "\n");
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}

	return HAPD_GET_FAIL;
}
int hapd_get_wep_key3(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	char wep_key_type = config->wepKeyType;

	if(config->encmode == _WEP_40_PRIVACY_ || config->encmode == _WEP_104_PRIVACY_)
	{
		unsigned idx, key_len = 0;

		if(config->encmode == _WEP_40_PRIVACY_)
			key_len = 5;
		else
			key_len = 13;

		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=", name);

		if(wep_key_type == 0 && strlen(config->wepkey3) == key_len)
			len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "\"" "%s" "\"", config->wepkey3);
		else
		{
			if(wep_key_type == 0)
				memset(config->wepkey3, 0, sizeof(config->wepkey3));
			for(idx=0; idx<key_len; idx++)
				len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "%02x", config->wepkey3[idx]);
		}

		len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "\n");
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}

	return HAPD_GET_FAIL;
}
int hapd_get_wep_default_key(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->encmode == _WEP_40_PRIVACY_ || config->encmode == _WEP_104_PRIVACY_)
	{
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->wepDefaultKey);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}

	return HAPD_GET_FAIL;
}
int hapd_get_wep_key_len_broadcast(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config) == RADIUS_WEP){
		if(config->encmode == _WEP_40_PRIVACY_)
		{
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=5\n", name);
			if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
				return HAPD_GET_FAIL;
			return HAPD_GET_OK;
		}
		else if(config->encmode == _WEP_104_PRIVACY_){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=13\n", name);
			if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
				return HAPD_GET_FAIL;
			return HAPD_GET_OK;
		}
	}

	return HAPD_GET_FAIL;
}
int hapd_get_wep_key_len_unicast(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	
	if(config->encmode == _WEP_40_PRIVACY_)
	{
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=5\n", name);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
	else if(config->encmode == _WEP_104_PRIVACY_){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=13\n", name);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}

	return HAPD_GET_FAIL;
}

int hapd_get_wep_tx_keyidx(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->encmode == _WEP_40_PRIVACY_ || config->encmode == _WEP_104_PRIVACY_)
	{
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->wepDefaultKey);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}


	return HAPD_GET_FAIL;
}
int hapd_get_sae_groups(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->psk_enable & PSK_WPA3){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=19 20 21\n", name);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}

	return HAPD_GET_FAIL;
}

int hapd_get_sae_anti_clogging_threshold(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->psk_enable & PSK_WPA3){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=5\n", name);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}

	return HAPD_GET_FAIL;
}


//======RADIUS

int hapd_get_own_ip_addr(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, inet_ntoa(*((struct in_addr *)config->lan_ip)));
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}
int hapd_get_nas_identifier(struct wifi_config *config, char *name, char *result)
{
	int len = 0;

#ifdef WLAN_11R
	//memset(result, 0, sizeof(result));
	if(config->ft_enable)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->ft_r0kh_id);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
#else
	if(apply_radius(config)==NO_RADIUS && (apply_11r(config)==0))
		return HAPD_GET_FAIL;
	else {
		if(config->nas_identifier[0])
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->nas_identifier);
		else
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "test_coding.com");
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
#endif
}

int hapd_get_ieee8021x(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
			return HAPD_GET_FAIL;
	else {
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->enable_1x);
			if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
					return HAPD_GET_FAIL;
			return HAPD_GET_OK;
	}
}

int hapd_get_auth_server_addr(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, inet_ntoa(*((struct in_addr *)config->rsIpAddr)));
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}
int hapd_get_auth_server_port(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->rsPort);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_auth_server_shared_secret(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->rsPassword);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}
int hapd_get_acct_server_addr(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
#ifdef _PRMT_X_WLANFORISP_
	else if(config->disable_acct==1)
		return HAPD_GET_FAIL;
#endif
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, inet_ntoa(*((struct in_addr *)config->rsIpAddr)));
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}
int hapd_get_acct_server_port(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
#ifdef _PRMT_X_WLANFORISP_
	else if(config->disable_acct==1)
		return HAPD_GET_FAIL;
#endif
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->rsPort);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}
int hapd_get_acct_server_shared_secret(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
#ifdef _PRMT_X_WLANFORISP_
	else if(config->disable_acct==1)
		return HAPD_GET_FAIL;
#endif
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->rsPassword);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_radius_retry_primary_interval(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->radius_retry_primary_interval);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_auth_server_addr_2(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
	else {
		if(apply_radius2(config)){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "auth_server_addr=%s\n", inet_ntoa(*((struct in_addr *)config->rs2IpAddr)));
			if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
				return HAPD_GET_FAIL;
		}
		return HAPD_GET_OK;
	}
}
int hapd_get_auth_server_port_2(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
	else {
		if(apply_radius2(config)){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "auth_server_port=%d\n", config->rs2Port);
			if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
				return HAPD_GET_FAIL;
		}
		return HAPD_GET_OK;
	}
}

int hapd_get_auth_server_shared_secret_2(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
	else {
		if(apply_radius2(config)){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "auth_server_shared_secret=%s\n", config->rs2Password);
			if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
				return HAPD_GET_FAIL;
		}
		return HAPD_GET_OK;
	}
}
int hapd_get_acct_server_addr_2(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
	else {
		if(apply_radius2(config)){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "acct_server_addr=%s\n", inet_ntoa(*((struct in_addr *)config->rs2IpAddr)));
			if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
				return HAPD_GET_FAIL;
		}
		return HAPD_GET_OK;
	}
}
int hapd_get_acct_server_port_2(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
	else {
		if(apply_radius2(config)){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "acct_server_port=%d\n", config->rs2Port);
			if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
				return HAPD_GET_FAIL;
		}
		return HAPD_GET_OK;
	}
}
int hapd_get_acct_server_shared_secret_2(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
	else {
		if(apply_radius2(config)){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "acct_server_shared_secret=%s\n", config->rs2Password);
			if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
				return HAPD_GET_FAIL;
		}
		return HAPD_GET_OK;
	}
}

int hapd_get_radius_acct_interim_interval(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_radius(config)==NO_RADIUS)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->radius_acct_interim_interval);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

//======Opeation Mode
int hapd_get_hw_mode(struct wifi_config *config, char *name, char *result)
{
	unsigned char hw_mode[20];
	int len = 0;

	if(config->phyband == PHYBAND_5G)
		strcpy(hw_mode, "a");
	else{
		if(config->wlan_band == WIRELESS_11B)
			strcpy(hw_mode, "b");
		else
			strcpy(hw_mode, "g");
	}

	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, hw_mode);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_obss_interval(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->coexist)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, 180);
	else
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->coexist);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_ieee80211n(struct wifi_config *config, char *name, char *result)
{
	unsigned char ieee80211n;
	int len = 0;

#ifdef WLAN_6G_SUPPORT
	if(config->is_6ghz_enable)
		return HAPD_GET_FAIL;
#endif

	if(config->encmode == _WEP_40_PRIVACY_ || config->encmode == _WEP_104_PRIVACY_)
		ieee80211n = 0;
	else if(config->wlan_band & WIRELESS_11N || config->wlan_band & WIRELESS_11AC || config->wlan_band & WIRELESS_11AX)
		ieee80211n = 1;	
	else
		ieee80211n = 0;

	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, ieee80211n);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}


int hapd_get_require_ht(struct wifi_config *config, char *name, char *result)
{
	unsigned char require_ht;
	int len = 0;

#ifdef WLAN_6G_SUPPORT
	if(config->is_6ghz_enable)
		return HAPD_GET_FAIL;
#endif

	if(config->wlan_band == WIRELESS_11N)
		require_ht = 1;
	else
		require_ht = 0;

	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, require_ht);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_ieee80211ac(struct wifi_config *config, char *name, char *result)
{
	unsigned char ieee80211ac;
	int len = 0;
#ifdef WLAN_6G_SUPPORT
	if(config->is_6ghz_enable)
		return HAPD_GET_FAIL;
#endif

	if(config->encmode == _WEP_40_PRIVACY_ || config->encmode == _WEP_104_PRIVACY_)
		ieee80211ac = 0;
	else if(config->wlan_band & WIRELESS_11AC || ((config->wlan_band & WIRELESS_11AX) && (config->phy_band_select==PHYBAND_5G)))
		ieee80211ac = 1;	
	else
		ieee80211ac = 0;

	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, ieee80211ac);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_ieee80211ax(struct wifi_config *config, char *name, char *result)
{
	unsigned char ieee80211ax;
	int len = 0;

	if(config->encmode == _WEP_40_PRIVACY_ || config->encmode == _WEP_104_PRIVACY_)
		ieee80211ax = 0;
	else if(config->wlan_band & WIRELESS_11AX)
		ieee80211ax = 1;	
	else
		ieee80211ax = 0;

	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, ieee80211ax);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}


int hapd_get_require_vht(struct wifi_config *config, char *name, char *result)
{
	unsigned char require_vht;
	int len = 0;

#ifdef WLAN_6G_SUPPORT
	if(config->is_6ghz_enable)
		return HAPD_GET_FAIL;
#endif

	if(config->wlan_band == WIRELESS_11AC)
		require_vht = 1;
	else
		require_vht = 0;

	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, require_vht);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_force_2g_40m(struct wifi_config *config, char *name, char *result)
{
	int len = 0;

	if(config->force_2g_40m == 1){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->force_2g_40m);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}

	return HAPD_GET_OK;
}


int hapd_get_ht_capab(struct wifi_config *config, char *name, char *result)
{
	char ht_capab[50];
	int len = 0;

#ifdef WLAN_6G_SUPPORT
	if(config->is_6ghz_enable)
		return HAPD_GET_FAIL;
#endif

	if((config->wlan_band & (WIRELESS_11N|WIRELESS_11AC|WIRELESS_11AX)) == 0)
		return HAPD_GET_FAIL;

	if(config->channel_bandwidth == CHANNEL_WIDTH_20){
		if(config->shortGI)
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=[SHORT-GI-20]", name);
		else
			return HAPD_GET_FAIL;


		len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "\n");
		return HAPD_GET_OK;
	}

	if(config->channel == 0){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=[HT40+]", name);
	}
	else if(config->channel >= 36){
		if(config->channel <= 144){
			if(config->channel%8)
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=[HT40+]", name);
			else
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=[HT40-]", name);
		}else{
			if((config->channel-1)%8)
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=[HT40+]", name);
			else
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=[HT40-]", name);
		}
	}
	else if(config->control_band == HT_CONTROL_UPPER)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=[HT40-]", name);
	else
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=[HT40+]", name);

	if(!config->coexist)
		len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "[FORCE_BW]");

	if(config->shortGI)
		len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "[SHORT-GI-20][SHORT-GI-40]");

	len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "\n");
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_vht_capab(struct wifi_config *config, char *name, char *result)
{
#if 1
	int len = 0;

#ifdef WLAN_6G_SUPPORT
	if(config->is_6ghz_enable)
		return HAPD_GET_FAIL;
#endif

	if(config->phyband == PHYBAND_2G || (config->wlan_band & (WIRELESS_11AC|WIRELESS_11AX)) == 0)
		return HAPD_GET_FAIL;

	if(config->channel_bandwidth == CHANNEL_WIDTH_160)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=[VHT160]", name);
	if(config->shortGI){
		if(config->channel_bandwidth == CHANNEL_WIDTH_80)
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=[SHORT-GI-80]", name);
		else if(config->channel_bandwidth == CHANNEL_WIDTH_160)
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=[SHORT-GI-80][SHORT-GI-160][VHT160]", name);
		else
			return HAPD_GET_FAIL;
	}

	if(len)
		len += snprintf(result + len, HAPD_CONF_RESULT_MAX_LEN - len, "\n");
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
#endif
	return HAPD_GET_OK;
}


//======Operation Channel
int hapd_get_vht_oper_chwidth(struct wifi_config *config, char *name, char *result)
{
	unsigned char oper_chwidth = CHANWIDTH_80MHZ;
	int len = 0;

#ifdef WLAN_6G_SUPPORT
	if(config->is_6ghz_enable)
		return HAPD_GET_FAIL;
#endif

	if(config->phyband == PHYBAND_2G || (config->wlan_band & (WIRELESS_11AC|WIRELESS_11AX)) == 0)
		return HAPD_GET_FAIL;

	if(config->channel_bandwidth == CHANNEL_WIDTH_80)
		oper_chwidth = CHANWIDTH_80MHZ;
	else if(config->channel_bandwidth == CHANNEL_WIDTH_160)
		oper_chwidth = CHANWIDTH_160MHZ;
	else
		oper_chwidth = CHANWIDTH_USE_HT;

	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, oper_chwidth);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}


int hapd_get_channel(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if (config->channel == 0)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\nacs_num_scans=1\n", name, config->channel);
	else
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->channel);

	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_op_class(struct wifi_config *config, char *name, char *result)
{
	int len = 0;

#ifdef WLAN_6G_SUPPORT
	if(config->is_6ghz_enable == 0)
		return HAPD_GET_FAIL;
#else
	return HAPD_GET_FAIL;
#endif

	if (config->channel_bandwidth == CHANNEL_WIDTH_20)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=131\n", name);
	else if (config->channel_bandwidth == CHANNEL_WIDTH_40)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=132\n", name);
	else if (config->channel_bandwidth == CHANNEL_WIDTH_80)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=133\n", name);
	else if (config->channel_bandwidth == CHANNEL_WIDTH_160)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=134\n", name);

	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_chanlist(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->chanlist[0]){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->chanlist);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}
	return HAPD_GET_OK;
}


int hapd_get_vht_oper_centr_freq_seg0_idx(struct wifi_config *config, char *name, char *result)
{
	unsigned char centr_freq_seg0_idx = 0;
	int len = 0;

#ifdef WLAN_6G_SUPPORT
	if(config->is_6ghz_enable)
		return HAPD_GET_FAIL;
#endif

	if(config->phyband == PHYBAND_2G || (config->wlan_band & (WIRELESS_11AC|WIRELESS_11AX)) == 0)
		return HAPD_GET_FAIL;

	if(config->channel_bandwidth == CHANNEL_WIDTH_160){
		if(config->channel >= 36 && config->channel <= 64)
			centr_freq_seg0_idx = 50;
		if(config->channel >= 100 && config->channel <= 128)
			centr_freq_seg0_idx = 114;
	}
	else if(config->channel_bandwidth == CHANNEL_WIDTH_80){
		if(config->channel <= 48)
			centr_freq_seg0_idx = 42;
		else if(config->channel <= 64)
			centr_freq_seg0_idx = 58;
		else if(config->channel <= 112)
			centr_freq_seg0_idx = 106;
		else if(config->channel <= 128)
			centr_freq_seg0_idx = 122;
		else if(config->channel <= 144)
			centr_freq_seg0_idx = 138;
		else if(config->channel <= 161)
			centr_freq_seg0_idx = 155;
		else if(config->channel <= 177)
			centr_freq_seg0_idx = 171;
	}
	else if(config->channel_bandwidth == CHANNEL_WIDTH_40){
		if(config->channel == 0)
			centr_freq_seg0_idx=0;
		else if(config->channel <= 144){
			if(config->channel%8)
				centr_freq_seg0_idx=config->channel+2;
			else
				centr_freq_seg0_idx=config->channel-2;
		}else{
			if((config->channel-1)%8)
				centr_freq_seg0_idx=config->channel+2;
			else
				centr_freq_seg0_idx=config->channel-2;
		}
	}
	else if(config->channel_bandwidth == CHANNEL_WIDTH_20){
		centr_freq_seg0_idx = config->channel;
	}

	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, centr_freq_seg0_idx);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_vht_oper_centr_freq_seg1_idx(struct wifi_config *config, char *name, char *result)
{
	unsigned char centr_freq_seg1_idx = 0;
	int len = 0;

#ifdef WLAN_6G_SUPPORT
	if(config->is_6ghz_enable)
		return HAPD_GET_FAIL;
#endif

	if(config->phyband == PHYBAND_2G || (config->wlan_band & (WIRELESS_11AC|WIRELESS_11AX)) == 0)
		return HAPD_GET_FAIL;

	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, centr_freq_seg1_idx);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_he_oper_chwidth(struct wifi_config *config, char *name, char *result)
{
	unsigned char oper_chwidth = CHANWIDTH_80MHZ;
	int len = 0;

#ifdef WLAN_6G_SUPPORT
	if(config->is_6ghz_enable)
		return HAPD_GET_FAIL;
#endif

	if(config->phyband == PHYBAND_2G
		|| (config->wlan_band & (WIRELESS_11AC|WIRELESS_11AX)) == 0 /* ieee80211ac */
		|| (config->wlan_band & WIRELESS_11AX) == 0 /* ieee80211ax */)
		return HAPD_GET_FAIL;

	if(config->channel_bandwidth == CHANNEL_WIDTH_80)
		oper_chwidth = CHANWIDTH_80MHZ;
	else if(config->channel_bandwidth == CHANNEL_WIDTH_160)
		oper_chwidth = CHANWIDTH_160MHZ;
	else
		oper_chwidth = CHANWIDTH_USE_HT;

	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, oper_chwidth);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_he_oper_centr_freq_seg0_idx(struct wifi_config *config, char *name, char *result)
{
	unsigned char centr_freq_seg0_idx = 0;
	int len = 0;

	if(config->phyband == PHYBAND_2G
		|| (config->wlan_band & (WIRELESS_11AC|WIRELESS_11AX)) == 0 /* ieee80211ac */
		|| (config->wlan_band & WIRELESS_11AX) == 0 /* ieee80211ax */)
		return HAPD_GET_FAIL;

#ifdef WLAN_6G_SUPPORT
	if(config->is_6ghz_enable){
		if(config->channel_bandwidth == CHANNEL_WIDTH_80 || config->channel_bandwidth == CHANNEL_WIDTH_160){
			if(config->channel == 0)
				centr_freq_seg0_idx=0;
			else{
				if(((config->channel-1)%16)==0)
					centr_freq_seg0_idx=config->channel+6;
				else if(((config->channel-1)%16)==4)
					centr_freq_seg0_idx=config->channel+2;
				else if(((config->channel-1)%16)==8)
					centr_freq_seg0_idx=config->channel-2;
				else
					centr_freq_seg0_idx=config->channel-6;
			}
		}
		else if(config->channel_bandwidth == CHANNEL_WIDTH_40){
			if(config->channel == 0)
				centr_freq_seg0_idx=0;
			else{
				if(((config->channel-1)%8)==0)
					centr_freq_seg0_idx=config->channel+2;
				else
					centr_freq_seg0_idx=config->channel-2;
			}
		}
		else if(config->channel_bandwidth == CHANNEL_WIDTH_20){
			centr_freq_seg0_idx = config->channel;
		}

		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, centr_freq_seg0_idx);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;

		return HAPD_GET_OK;
	}
#endif

	if(config->channel_bandwidth == CHANNEL_WIDTH_160){
		if(config->channel >= 36 && config->channel <= 64)
			centr_freq_seg0_idx = 50;
		if(config->channel >= 100 && config->channel <= 128)
                        centr_freq_seg0_idx = 114;
	}

	if(config->channel_bandwidth == CHANNEL_WIDTH_80){
		if(config->channel <= 48)
			centr_freq_seg0_idx = 42;
		else if(config->channel <= 64)
			centr_freq_seg0_idx = 58;
		else if(config->channel <= 112)
			centr_freq_seg0_idx = 106;
		else if(config->channel <= 128)
			centr_freq_seg0_idx = 122;
		else if(config->channel <= 144)
			centr_freq_seg0_idx = 138;
		else if(config->channel <= 161)
			centr_freq_seg0_idx = 155;
		else if(config->channel <= 177)
			centr_freq_seg0_idx = 171;
	}
	else if(config->channel_bandwidth == CHANNEL_WIDTH_40){
		if(config->channel == 0)
			centr_freq_seg0_idx=0;
		else if(config->channel <= 144){
			if(config->channel%8)
				centr_freq_seg0_idx=config->channel+2;
			else
				centr_freq_seg0_idx=config->channel-2;
		}else{
			if((config->channel-1)%8)
				centr_freq_seg0_idx=config->channel+2;
			else
				centr_freq_seg0_idx=config->channel-2;
		}
	}
	else if(config->channel_bandwidth == CHANNEL_WIDTH_20){
		centr_freq_seg0_idx = config->channel;
	}

	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, centr_freq_seg0_idx);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_he_oper_centr_freq_seg1_idx(struct wifi_config *config, char *name, char *result)
{
	unsigned char centr_freq_seg1_idx = 0;
	int len = 0;

	if(config->phyband == PHYBAND_2G
		|| (config->wlan_band & (WIRELESS_11AC|WIRELESS_11AX)) == 0 /* ieee80211ac */
		|| (config->wlan_band & WIRELESS_11AX) == 0 /* ieee80211ax */)
		return HAPD_GET_FAIL;

#ifdef WLAN_6G_SUPPORT
	if(config->is_6ghz_enable){
		if(config->channel_bandwidth == CHANNEL_WIDTH_160){
			if(config->channel == 0)
				centr_freq_seg1_idx=0;
			else{
				if(((config->channel-1)%32)==0)
					centr_freq_seg1_idx=config->channel+14;
				else if(((config->channel-1)%32)==4)
					centr_freq_seg1_idx=config->channel+10;
				else if(((config->channel-1)%32)==8)
					centr_freq_seg1_idx=config->channel+6;
				else if(((config->channel-1)%32)==12)
					centr_freq_seg1_idx=config->channel+2;
				else if(((config->channel-1)%32)==16)
					centr_freq_seg1_idx=config->channel-2;
				else if(((config->channel-1)%32)==20)
					centr_freq_seg1_idx=config->channel-6;
				else if(((config->channel-1)%32)==24)
					centr_freq_seg1_idx=config->channel-10;
				else
					centr_freq_seg1_idx=config->channel-14;
			}
		}

		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, centr_freq_seg1_idx);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;

		return HAPD_GET_OK;
	}
#endif

	/*
	if(config->channel_bandwidth == CHANNEL_WIDTH_160){
		if(config->channel >= 36 || config->channel <= 64)
			centr_freq_seg1_idx = 50;
	}*/

	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, 0);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

//======HW Setting, Features
int hapd_get_bssid(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->bssid[0]){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->bssid);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}
	return HAPD_GET_OK;
}


int hapd_get_country_code(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->country);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_ieee80211d(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->country[0]){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=1\n", name);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}
	return HAPD_GET_OK;
}

#ifdef CONFIG_RTL_DFS_SUPPORT
int hapd_get_ieee80211h(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if (config->phyband == PHYBAND_5G) {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=1\n", name);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}
	return HAPD_GET_OK;
}
#endif

//======Advanced
int hapd_get_fragm_threshold(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->fragthres <= 2346)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->fragthres);
	else
		return HAPD_GET_FAIL;
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;

}

int hapd_get_rts_threshold(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->rtsthres);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_beacon_int(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->bcnint);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_dtim_period(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->dtimperiod);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_preamble(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->preamble);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_ignore_broadcast_ssid(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->ignore_broadcast_ssid);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_wmm_enabled(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->qos);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_uapsd_advertisement_enabled(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->qos == 1)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->qos);
	else
		return HAPD_GET_FAIL;
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_multicast_to_unicast(struct wifi_config *config, char *name, char *result)
{

#if 0
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->multicast_to_unicast);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
#endif

	return HAPD_GET_OK;
}

int hapd_get_ap_isolate(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->ap_isolate);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_ap_max_inactivity(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->ap_max_inactivity);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

#ifdef HS2_SUPPORT
int hapd_get_hs20(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->hs20);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}
#endif

int hapd_get_tdls_prohibit(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->tdls_prohibit);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_tdls_prohibit_chan_switch(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->tdls_prohibit_chan_switch);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_data_rate(struct wifi_config *config, char *name, char *result)
{
	return HAPD_GET_OK;
}


#ifdef CONFIG_GUEST_ACCESS_SUPPORT
int hapd_get_access_network_type(struct wifi_config *config, char *name, char *result)
{
#if 0
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->guest_access);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
#endif

	return HAPD_GET_OK;
}
#endif

#define FILE_PATH_MAX_LEN 64
/*
web:	0 acl disable, 1 white, 2 black;
hostapd:	0 = accept unless in deny list, 1 = deny unless in accept list.
*/
int hapd_get_macaddr_acl(struct wifi_config *config, char *name, char *result)
{
	int i = 0;
	FILE *fp = NULL;
	char MAC_LIST_FILE_PERIF[FILE_PATH_MAX_LEN];

	if(config->macaddr_acl != 0 && config->macaddr_acl != 1 && config->macaddr_acl != 2){
		printf("macaddr_acl[%d] not support!!!\n", config->macaddr_acl);
		return HAPD_GET_OK;
	}

	if(config->macaddr_acl == 0){
		return HAPD_GET_OK;
	}else if(config->macaddr_acl == 1){
		snprintf(MAC_LIST_FILE_PERIF, FILE_PATH_MAX_LEN, "/var/run/hostapd_accept_%s", config->interface);
		if(!(fp = fopen(MAC_LIST_FILE_PERIF, "w+"))){
			fprintf(stderr, "fopen ERROR! %s\n", strerror(errno));
			return HAPD_GET_FAIL;
		}
		snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "accept_mac_file=%s\n", MAC_LIST_FILE_PERIF);
		strncat(result, "macaddr_acl=1\n", HAPD_CONF_RESULT_MAX_LEN);
	}else if(config->macaddr_acl == 2){
		snprintf(MAC_LIST_FILE_PERIF, FILE_PATH_MAX_LEN, "/var/run/hostapd_deny_%s", config->interface);
		if(!(fp = fopen(MAC_LIST_FILE_PERIF, "w+"))){
			fprintf(stderr, "fopen ERROR! %s\n", strerror(errno));
			return HAPD_GET_FAIL;
		}
		snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "deny_mac_file=%s\n", MAC_LIST_FILE_PERIF);
		strncat(result, "macaddr_acl=0\n", HAPD_CONF_RESULT_MAX_LEN);
	}

	for(i = 0; i < MAX_ACL_NUM; i++){
		if(config->mac_acl_list[i].used)
			fprintf(fp, "%02x:%02x:%02x:%02x:%02x:%02x\n", config->mac_acl_list[i]._macaddr[0],
				config->mac_acl_list[i]._macaddr[1], config->mac_acl_list[i]._macaddr[2], config->mac_acl_list[i]._macaddr[3],
				config->mac_acl_list[i]._macaddr[4], config->mac_acl_list[i]._macaddr[5]);
	}

	fclose(fp);
	return HAPD_GET_OK;
}


int hapd_get_wps_state(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else if(config->wps_state){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->wps_state);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
	else
		return HAPD_GET_FAIL;

}

int hapd_get_wps_uuid(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		if(config->uuid[0]){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->uuid);
			if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
				return HAPD_GET_FAIL;
		}
		return HAPD_GET_OK;
	}
}

int hapd_get_wps_device_name(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->device_name);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_wps_model_name(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->device_name);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_wps_model_number(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->model_number);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_wps_serial_number(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->serial_number);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_wps_device_type(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->device_type);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_wps_config_methods(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
		if(config->wlan_mode == CLIENT_MODE
#if defined(WLAN_UNIVERSAL_REPEATER)
			|| config->is_repeater
#endif
			)
		{
			if(config->config_method == 1)
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=\"%s\"\n", name, "virtual_display keypad");
			else if(config->config_method == 2)
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=\"%s\"\n", name, "physical_push_button virtual_push_button");
			else if(config->config_method == 3)
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=\"%s\"\n", name, "virtual_display keypad physical_push_button virtual_push_button");
		}
		else
#endif
		{
			if(config->config_method == 1)
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "virtual_display keypad");
			else if(config->config_method == 2)
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "physical_push_button virtual_push_button");
			else if(config->config_method == 3)
				len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "virtual_display keypad physical_push_button virtual_push_button");
		}
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_wps_pbc_in_m1(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=1\n", name);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_wps_ap_pin(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->ap_pin);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_wps_upnp_iface(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		if(config->upnp_iface[0]){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->upnp_iface);
			if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
				return HAPD_GET_FAIL;
		}
		return HAPD_GET_OK;
	}
}

int hapd_get_wps_friendly_name(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->friendly_name);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_wps_manufacturer_url(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->manufacturer_url);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_wps_model_url(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->model_url);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_wps_independent(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->wps_independent);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}
	return HAPD_GET_OK;

}

int hapd_get_wps_cred_processing(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->wps_cred_processing);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}
	return HAPD_GET_OK;

}

int hapd_get_wps_manufacturer(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_wps(config)==0)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->manufacturer);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

#ifdef RTK_MULTI_AP
int hapd_get_multi_ap(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->multi_ap){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->multi_ap);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
	else
		return HAPD_GET_FAIL;
}

int hapd_get_multi_ap_backhaul_ssid(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_multi_ap(config)==0 || config->multi_ap_backhaul_ssid[0] == 0)
		return HAPD_GET_FAIL;
	else {
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=\"%s\"\n", name, config->multi_ap_backhaul_ssid);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
}

int hapd_get_multi_ap_backhaul_wpa_psk(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_multi_ap(config)==0 || config->multi_ap_backhaul_passphrase[0] == 0)
		return HAPD_GET_FAIL;
	else {
		if(config->multi_ap_backhaul_psk_format == KEY_HEX){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->multi_ap_backhaul_passphrase);
			if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
				return HAPD_GET_FAIL;
			return HAPD_GET_OK;
		}
		else
			return HAPD_GET_FAIL;
	}
}
int hapd_get_multi_ap_backhaul_wpa_passphrase(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_multi_ap(config)==0 || config->multi_ap_backhaul_passphrase[0] == 0)
		return HAPD_GET_FAIL;
	else {
		if(config->multi_ap_backhaul_psk_format == KEY_ASCII){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->multi_ap_backhaul_passphrase);
			if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
				return HAPD_GET_FAIL;
			return HAPD_GET_OK;
		}
		else
			return HAPD_GET_FAIL;
	}
}
#endif

#if defined(CONFIG_MBO_SUPPORT)
/*=======================MBO==============================*/
int hapd_get_mbo(struct wifi_config *config, char *name, char *result)
{
	unsigned int config_setting = 1;
	if(!config->mbo_enable)
		return HAPD_GET_OK;
	if(apply_wpa(config) && config->dot11IEEE80211W==1 && (!(config->wpa_cipher && config->wpa2_cipher))
#ifdef WLAN_WPA3
		&& (config->psk_enable != PSK_WPA3)
#endif
	){
		sprintf(result, "%s=%d\n", name, config_setting);
	}
	return HAPD_GET_OK;
}

int hapd_get_oce(struct wifi_config *config, char *name, char *result)
{
	unsigned int config_setting = 4;
	if(!config->mbo_enable)
		return HAPD_GET_OK;
	if(apply_wpa(config) && config->dot11IEEE80211W==1 && (!(config->wpa_cipher && config->wpa2_cipher))
#ifdef WLAN_WPA3
		&& (config->psk_enable != PSK_WPA3)
#endif
	)
		sprintf(result, "%s=%d\n", name, config_setting);
	return HAPD_GET_OK;
}

int hapd_get_interworking(struct wifi_config *config, char *name, char *result)
{
	unsigned int config_setting = 1;
	if(!config->mbo_enable)
		return HAPD_GET_OK;
	if(apply_wpa(config) && config->dot11IEEE80211W==1 && (!(config->wpa_cipher && config->wpa2_cipher))
#ifdef WLAN_WPA3
		&& (config->psk_enable != PSK_WPA3)
#endif
	)
		sprintf(result, "%s=%d\n", name, config_setting);
	return HAPD_GET_OK;
}

int hapd_get_country3(struct wifi_config *config, char *name, char *result)
{
	unsigned int config_setting = 4;
	if(!config->mbo_enable)
		return HAPD_GET_OK;
	if(apply_wpa(config) && config->dot11IEEE80211W==1 && (!(config->wpa_cipher && config->wpa2_cipher))
#ifdef WLAN_WPA3
		&& (config->psk_enable != PSK_WPA3)
#endif
	)
		sprintf(result, "%s=0x%02x\n", name, config_setting);
	return HAPD_GET_OK;
}

int hapd_get_gas_address3(struct wifi_config *config, char *name, char *result)
{
	unsigned int config_setting = 1;
	if(!config->mbo_enable)
		return HAPD_GET_OK;
	if(apply_wpa(config) && config->dot11IEEE80211W==1 && (!(config->wpa_cipher && config->wpa2_cipher))
#ifdef WLAN_WPA3
		&& (config->psk_enable != PSK_WPA3)
#endif
	)
		sprintf(result, "%s=%d\n", name, config_setting);
	return HAPD_GET_OK;
}
#endif
int hapd_get_eap_server(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	if(apply_radius(config))
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=0\n", name);
	else
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=1\n", name);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	return HAPD_GET_OK;

}

int hapd_get_supported_rates(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->supported_rates);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}
int hapd_get_basic_rates(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->basic_rates);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_bss_transition(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->bss_transition);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

int hapd_get_rrm_neighbor_report(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->rrm_neighbor_report);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	return HAPD_GET_OK;

}

int hapd_get_rrm_beacon_report(struct wifi_config *config, char *name, char *result)
{
	int len=0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->rrm_beacon_report);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	return HAPD_GET_OK;
}

//11R
int hapd_get_mobility_domain(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_11r(config)){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->ft_mdid);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}

	return HAPD_GET_OK;
}
int hapd_get_ft_over_ds(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_11r(config)){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->ft_over_ds);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}

	return HAPD_GET_OK;
}
int hapd_get_ft_r0_key_lifetime(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_11r(config)){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->ft_r0key_to);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}

	return HAPD_GET_OK;
}
int hapd_get_reassociation_deadline(struct wifi_config *config, char *name, char *result)
{
	int len = 0;

	if(apply_11r(config)){
		if(config->ft_reasoc_to==0)
			return HAPD_GET_OK;

		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->ft_reasoc_to);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}

	return HAPD_GET_OK;
}
#ifdef WLAN_11R
static int CalcMD5(char *in, char *key)
{
    MD5_CTX mc;

    MD5_Init(&mc);
    MD5_Update(&mc, in, strlen(in));
    MD5_Final(key, &mc);

    return 0;
}
#endif
int hapd_get_r0kh_r1kh(struct wifi_config *config, char *name, char *result)
{
#ifdef WLAN_11R
	int i = 0, j = 0, entryNum = 0, ii = 0;
	char tmpbuf[200], macStr[18]={0};
	int ishex=0;
	char enckey[35]={0};
	char ifname[IFNAMSIZ]={0};
	MIB_CE_WLAN_FTKH_T ent;

	if(!apply_11r(config))
		return HAPD_GET_OK;

	if(config->fp == NULL)
		return HAPD_GET_FAIL;

	for (j=0; j<NUM_WLAN_INTERFACE; j++) {
		entryNum = mib_chain_local_mapping_total(MIB_WLAN_FTKH_TBL, j);
		for (i=0; i<entryNum; i++) {
			if (!mib_chain_local_mapping_get(MIB_WLAN_FTKH_TBL, j, i, (void *)&ent))
			{
				printf("%s: Get chain record error!\n", __func__);
				return HAPD_GET_FAIL;
			}

			rtk_wlan_get_ifname(j, ent.intfIdx, ifname);
			if(strcmp(config->interface, ifname))
				continue;

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
			if (!ishex){
				//snprintf(enckey, sizeof(enckey), "\"%s\"", ent.key);
				memset(tmpbuf, '\0', sizeof(tmpbuf));
				memset(enckey, '\0', sizeof(enckey));
				CalcMD5(ent.key, tmpbuf);
				ii=0;
				while(tmpbuf[ii]!='\0'){
					sprintf((char *)(enckey+ii*2), "%02x", (unsigned char)tmpbuf[ii]);
					ii++;
				}
			}
			else
				strcpy(enckey, ent.key);

			fprintf(config->fp, "r0kh=%s %s %s\n", macStr, ent.r0kh_id, enckey);
			fprintf(config->fp, "r1kh=%s %s %s\n", macStr, macStr, enckey);
		}
	}
#endif
	return HAPD_GET_OK;
}
int hapd_get_pmk_r1_push(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(apply_11r(config)){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->ft_push);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}

	return HAPD_GET_OK;
}

int hapd_get_r1_key_holder(struct wifi_config *config, char *name, char *result)
{
#ifdef WLAN_11R
	int len = 0;
	if(config->r1_key_holder[0])
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->r1_key_holder);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;
#endif
	return HAPD_GET_OK;
}

//station
int hapd_get_max_num_sta(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if (config->max_num_sta > 0)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->max_num_sta);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

#if defined(CONFIG_RTL_CFG80211_WAPI_SUPPORT)
int hapd_get_wapi_key_update_time(struct wifi_config *config, char *name, char *result)
{
	if (config->psk_enable != PSK_CERT_WAPI)
		return HAPD_GET_OK;
		
	int len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->wapiKeyUpdateTime);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_wapi_role(struct wifi_config *config, char *name, char *result)
{
	if (config->psk_enable != PSK_CERT_WAPI)
		return HAPD_GET_OK;
	
	int len = 0;
	if (config->wapiAuthType == WAPI_AUTH_CERT)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->wapiRole);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_wapi_ca_cert(struct wifi_config *config, char *name, char *result)
{
	if (config->psk_enable != PSK_CERT_WAPI)
		return HAPD_GET_OK;
	
	int len = 0;
	if (config->wapiAuthType == WAPI_AUTH_CERT)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, CFG80211_WAPI_CA_CERT);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_wapi_asu_cert(struct wifi_config *config, char *name, char *result)
{
	if (config->psk_enable != PSK_CERT_WAPI)
		return HAPD_GET_OK;
	
	int len = 0;
	if (config->wapiAuthType == WAPI_AUTH_CERT && config->wapiCertCount == 3)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, CFG80211_WAPI_ASU_CERT);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_wapi_ae_cert(struct wifi_config *config, char *name, char *result)
{
	if (config->psk_enable != PSK_CERT_WAPI)
		return HAPD_GET_OK;
	
	int len = 0;
	if (config->wapiAuthType == WAPI_AUTH_CERT)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, CFG80211_WAPI_USER_CERT);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}
#endif

#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
int hapd_get_wpas_network_start(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s\n", name);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_wpas_update_config(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->update_config);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_wpas_scan_ssid(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->scan_ssid);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_wpas_pbss(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->pbss);
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

//======Security
int hapd_get_wpas_auth_algs(struct wifi_config *config, char *name, char *result)
{
	unsigned char auth_algs = WPA_AUTH_ALG_OPEN;
	int len = 0;
	if(config->psk_enable & PSK_WPA3)
		return HAPD_GET_OK;

	if(config->authtype == _AUTH_ALGM_OPEN_)
		auth_algs = WPA_AUTH_ALG_OPEN;
	else if(config->authtype == _AUTH_ALGM_SHARED_)
		auth_algs = WPA_AUTH_ALG_SHARED;

	if(auth_algs == WPA_AUTH_ALG_OPEN)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=OPEN\n", name);
	else if(auth_algs == WPA_AUTH_ALG_SHARED)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=SHARED\n", name);

	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}

int hapd_get_wpas_pairwise(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->encmode==0x00)
		return HAPD_GET_OK;

	if(config->psk_enable == PSK_WPA)
	{
		if(config->wpa_cipher == CIPHER_TKIP){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "TKIP");
		}else if(config->wpa_cipher == CIPHER_AES){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "CCMP");
		}else if(config->wpa_cipher == CIPHER_MIXED){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "TKIP CCMP");
		}
	}
	
	if(config->psk_enable & (PSK_WPA2 | PSK_WPA3)){
		if(config->wpa2_cipher == CIPHER_TKIP){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "TKIP");
		}else if(config->wpa2_cipher == CIPHER_AES){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "CCMP");
		}else if(config->wpa2_cipher == CIPHER_MIXED){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "TKIP CCMP");
		}else{
			printf("!! Error,  [%s][%d] unknown wpa_cipher\n", __FUNCTION__, __LINE__);
			return HAPD_GET_FAIL;
		}
	}
		
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;

}

int hapd_get_wpas_group(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->encmode==0x00)
		return HAPD_GET_OK;

	if(config->psk_enable == (PSK_WPA|PSK_WPA2)){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "TKIP CCMP");
	}

	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;

}

int hapd_get_wpas_psk(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->psk_enable){
		if(config->psk_format == KEY_HEX){
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, config->passphrase);
		}else{
			len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=\"%s\"\n", name, config->passphrase);
		}
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
	else
		return HAPD_GET_FAIL;
}

int hapd_get_wpas_sae_password(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->psk_enable & PSK_WPA3){
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=\"%s\"\n", name, config->passphrase);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
		return HAPD_GET_OK;
	}
	else
		return HAPD_GET_FAIL;
}

int hapd_get_wpas_proto(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->encmode == _NO_PRIVACY_ || config->encmode == _WEP_40_PRIVACY_ || config->encmode == _WEP_104_PRIVACY_)
		return HAPD_GET_OK;
	else if(config->wpa_cipher && (config->wpa2_cipher==0))
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "WPA");
	else if(config->wpa_cipher && config->wpa2_cipher)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "WPA RSN");
	else if((config->wpa_cipher==0) && config->wpa2_cipher)
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%s\n", name, "RSN");
	else
		return HAPD_GET_FAIL;
	if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}
#ifdef RTK_MULTI_AP
int hapd_get_multi_ap_backhaul_sta(struct wifi_config *config, char *name, char *result)
{
	int len = 0;
	if(config->multi_ap_backhaul_sta)
	{
		len = snprintf(result, HAPD_CONF_RESULT_MAX_LEN, "%s=%d\n", name, config->multi_ap_backhaul_sta);
		if(len < 0 || len >= HAPD_CONF_RESULT_MAX_LEN)
			return HAPD_GET_FAIL;
	}
	else
		return HAPD_GET_FAIL;

	return HAPD_GET_OK;
}
#endif
#endif

struct hapd_obj	hapd_conf_table[] =
{
	{"driver",					hapd_get_driver},
	{"interface",				hapd_get_interface},
	{"ctrl_interface",			hapd_get_ctrl_interface},
	{"bridge",					hapd_get_bridge},
	{"ssid",					hapd_get_ssid},

//Security
	{"auth_algs",				hapd_get_auth_algs},
	{"wpa",						hapd_get_wpa},
	{"wpa_psk",					hapd_get_wpa_psk},
	{"wpa_passphrase",			hapd_get_wpa_passphrase},
	{"sae_password",			hapd_get_sae_password},
	{"wpa_key_mgmt",			hapd_get_wpa_key_mgmt},
	{"wpa_pairwise",			hapd_get_wpa_pairwise},
	{"rsn_pairwise",			hapd_get_rsn_pairwise},
#ifdef WLAN_WPA3_ENTERPRISE
	{"group_mgmt_cipher",		hapd_get_group_mgmt_cipher},
#endif
	{"ieee80211w",				hapd_get_ieee80211w},
	{"wpa_group_rekey",			hapd_get_wpa_group_rekey},
#ifdef WLAN_WPA3_H2E_HAPD
	{"sae_pwe", 				hapd_get_sae_pwe},
	{"transition_disable",		hapd_get_transition_disable},
#endif

	{"wep_key0",				hapd_get_wep_key0},
    {"wep_key1",                hapd_get_wep_key1},
    {"wep_key2",                hapd_get_wep_key2},
    {"wep_key3",                hapd_get_wep_key3},
	{"wep_default_key",			hapd_get_wep_default_key},
	{"wep_key_len_broadcast",	hapd_get_wep_key_len_broadcast},
	{"wep_key_len_unicast",		hapd_get_wep_key_len_unicast},
	{"sae_groups",				hapd_get_sae_groups},
	{"sae_anti_clogging_threshold",		hapd_get_sae_anti_clogging_threshold},

//RADIUS
	{"own_ip_addr",					hapd_get_own_ip_addr},
	{"nas_identifier",				hapd_get_nas_identifier},

	{"ieee8021x",					hapd_get_ieee8021x},
	{"auth_server_addr",			hapd_get_auth_server_addr},
	{"auth_server_port",			hapd_get_auth_server_port},
	{"auth_server_shared_secret",	hapd_get_auth_server_shared_secret},
	{"acct_server_addr",			hapd_get_acct_server_addr},
	{"acct_server_port",			hapd_get_acct_server_port},
	{"acct_server_shared_secret",	hapd_get_acct_server_shared_secret},
	{"radius_retry_primary_interval",	hapd_get_radius_retry_primary_interval},

	{"auth_server_addr2",			hapd_get_auth_server_addr_2},
	{"auth_server_port2",			hapd_get_auth_server_port_2},
	{"auth_server_shared_secret2",	hapd_get_auth_server_shared_secret_2},
	{"acct_server_addr2",			hapd_get_acct_server_addr_2},
	{"acct_server_port2",			hapd_get_acct_server_port_2},
	{"acct_server_shared_secret2",	hapd_get_acct_server_shared_secret_2},
	{"radius_acct_interim_interval",	hapd_get_radius_acct_interim_interval},
//
	{"hw_mode",					hapd_get_hw_mode},
	{"obss_interval",			hapd_get_obss_interval},
	{"ieee80211n",				hapd_get_ieee80211n},
	{"require_ht",				hapd_get_require_ht},
	{"ieee80211ac",				hapd_get_ieee80211ac},
	{"require_vht",				hapd_get_require_vht},
	{"ieee80211ax",				hapd_get_ieee80211ax},

	{"force_2g_40m",			hapd_get_force_2g_40m}, //need to modify hostapd

	{"ht_capab",				hapd_get_ht_capab},	//Short GI, STBC
	{"vht_capab",				hapd_get_vht_capab},

	{"vht_oper_chwidth",		hapd_get_vht_oper_chwidth},
	{"channel",					hapd_get_channel},
	{"op_class",				hapd_get_op_class},
	{"chanlist",				hapd_get_chanlist},
	{"vht_oper_centr_freq_seg0_idx",	hapd_get_vht_oper_centr_freq_seg0_idx},
#ifdef WLAN_SUPPPORT_160M
	{"vht_oper_centr_freq_seg1_idx",	hapd_get_vht_oper_centr_freq_seg1_idx},
#endif

	{"he_oper_chwidth",			hapd_get_he_oper_chwidth},
	{"he_oper_centr_freq_seg0_idx",	hapd_get_he_oper_centr_freq_seg0_idx},
#ifdef WLAN_SUPPPORT_160M
	{"he_oper_centr_freq_seg1_idx", hapd_get_he_oper_centr_freq_seg1_idx},
#endif

	{"bssid",					hapd_get_bssid},
	{"country_code",			hapd_get_country_code},
	{"ieee80211d",				hapd_get_ieee80211d},
#ifdef CONFIG_RTL_DFS_SUPPORT
	{"ieee80211h",				hapd_get_ieee80211h},
#endif

//Advanced
	{"fragm_threshold",			hapd_get_fragm_threshold},
	{"rts_threshold",			hapd_get_rts_threshold},
	{"beacon_int",				hapd_get_beacon_int},
	{"dtim_period",				hapd_get_dtim_period},
	{"preamble",				hapd_get_preamble},
	{"ignore_broadcast_ssid",	hapd_get_ignore_broadcast_ssid},
	{"wmm_enabled", 			hapd_get_wmm_enabled},
	{"uapsd_advertisement_enabled", hapd_get_uapsd_advertisement_enabled},
//
	{"multicast_to_unicast",	hapd_get_multicast_to_unicast},
	{"ap_isolate",			hapd_get_ap_isolate},
	{"data_rate",				hapd_get_data_rate},
	{"ap_max_inactivity",		hapd_get_ap_max_inactivity},
#ifdef HS2_SUPPORT
	{"hs20",					hapd_get_hs20},
#endif
	{"tdls_prohibit",			hapd_get_tdls_prohibit},
	{"tdls_prohibit_chan_switch",			hapd_get_tdls_prohibit_chan_switch},
#ifdef CONFIG_GUEST_ACCESS_SUPPORT
	{"access_network_type",		hapd_get_access_network_type},
#endif
//acl
	{"macaddr_acl",				hapd_get_macaddr_acl},

//WPS
#ifdef WLAN_WPS
	{"wps_state",				hapd_get_wps_state},
	{"wps_independent",			hapd_get_wps_independent},
	{"wps_cred_processing",		hapd_get_wps_cred_processing},
	{"uuid",					hapd_get_wps_uuid},
	{"device_name",				hapd_get_wps_device_name},
	{"model_name",				hapd_get_wps_model_name},
	{"model_number",			hapd_get_wps_model_number},
	{"serial_number",			hapd_get_wps_serial_number},
	{"device_type",				hapd_get_wps_device_type},
	{"config_methods",			hapd_get_wps_config_methods},
	{"pbc_in_m1",				hapd_get_wps_pbc_in_m1},
	{"ap_pin",					hapd_get_wps_ap_pin},
	{"upnp_iface",				hapd_get_wps_upnp_iface},
	{"friendly_name",			hapd_get_wps_friendly_name},
	{"manufacturer_url",		hapd_get_wps_manufacturer_url},
	{"model_url",				hapd_get_wps_model_url},
#endif

#if	defined(WLAN_LOGO_TEST_FOR_SD9) || defined(CONFIG_MBO_SUPPORT)
//MBO
	{"mbo",						hapd_get_mbo},
	{"oce",						hapd_get_oce},
	{"interworking",			hapd_get_interworking},
	{"country3",				hapd_get_country3},
	{"gas_address3",			hapd_get_gas_address3},
#endif

//EAP
	{"eap_server",				hapd_get_eap_server},
//rates
	{"supported_rates",			hapd_get_supported_rates}, 
	{"basic_rates",				hapd_get_basic_rates}, 
	
//11KV
	{"bss_transition",			hapd_get_bss_transition},
	{"rrm_neighbor_report",		hapd_get_rrm_neighbor_report},
	{"rrm_beacon_report", 		hapd_get_rrm_beacon_report},

//11R
	{"mobility_domain",			hapd_get_mobility_domain},
	{"ft_over_ds",				hapd_get_ft_over_ds},
	{"ft_r0_key_lifetime",		hapd_get_ft_r0_key_lifetime},
	{"reassociation_deadline",	hapd_get_reassociation_deadline},
	{"r1_key_holder",			hapd_get_r1_key_holder},
	{"r0kh",					hapd_get_r0kh_r1kh},
	{"pmk_r1_push",				hapd_get_pmk_r1_push},
#ifdef RTK_MULTI_AP
	{"multi_ap",				hapd_get_multi_ap},
#ifdef WLAN_WPS
	{"multi_ap_backhaul_ssid",	hapd_get_multi_ap_backhaul_ssid},
	{"multi_ap_backhaul_wpa_psk", hapd_get_multi_ap_backhaul_wpa_psk},
	{"multi_ap_backhaul_wpa_passphrase", hapd_get_multi_ap_backhaul_wpa_passphrase},
#endif
#endif
#if	defined(CONFIG_MBO_SUPPORT)
	//MBO
	{"mbo",						hapd_get_mbo},
	{"oce",						hapd_get_oce},
	{"interworking",			hapd_get_interworking},
	{"country3",				hapd_get_country3},
	{"gas_address3",			hapd_get_gas_address3},
#endif
	//station
	{"max_num_sta",				hapd_get_max_num_sta},
#if defined(CONFIG_RTL_CFG80211_WAPI_SUPPORT)
	{"key_update_time",			hapd_get_wapi_key_update_time},
	{"wapi_role",				hapd_get_wapi_role},
	{"wapi_ca_cert",			hapd_get_wapi_ca_cert},
	{"wapi_asu_cert",			hapd_get_wapi_asu_cert},
	{"wapi_ae_cert",			hapd_get_wapi_ae_cert},
#endif
};

struct hapd_obj	vap_hapd_conf_table[] =
{
//	{"driver",					hapd_get_driver},
	{"bss",				hapd_get_interface},
	{"ctrl_interface",			hapd_get_ctrl_interface},
	{"bridge",					hapd_get_bridge},
	{"ssid",					hapd_get_ssid},
	{"bssid",					hapd_get_bssid},

//Security
	{"auth_algs",				hapd_get_auth_algs},
	{"wpa",						hapd_get_wpa},
	{"wpa_psk",					hapd_get_wpa_psk},
	{"wpa_passphrase",			hapd_get_wpa_passphrase},
	{"sae_password",			hapd_get_sae_password},
	{"wpa_key_mgmt",			hapd_get_wpa_key_mgmt},
	{"wpa_pairwise",			hapd_get_wpa_pairwise},
	{"rsn_pairwise",			hapd_get_rsn_pairwise},
#ifdef WLAN_WPA3_ENTERPRISE
	{"group_mgmt_cipher",		hapd_get_group_mgmt_cipher},
#endif
	{"ieee80211w",				hapd_get_ieee80211w},
	{"wpa_group_rekey",			hapd_get_wpa_group_rekey},
#ifdef WLAN_WPA3_H2E_HAPD
	{"sae_pwe", 				hapd_get_sae_pwe},
	{"transition_disable",		hapd_get_transition_disable},
#endif

	{"wep_key0",			hapd_get_wep_key0},
	{"wep_key1",            hapd_get_wep_key1},
    {"wep_key2",            hapd_get_wep_key2},
    {"wep_key3",            hapd_get_wep_key3},
	{"wep_default_key",		hapd_get_wep_default_key},
	{"wep_key_len_broadcast",	hapd_get_wep_key_len_broadcast},
	{"wep_key_len_unicast",		hapd_get_wep_key_len_unicast},

//RADIUS
	{"own_ip_addr",					hapd_get_own_ip_addr},
	{"nas_identifier",				hapd_get_nas_identifier},

	{"ieee8021x",					hapd_get_ieee8021x},
	{"auth_server_addr",			hapd_get_auth_server_addr},
	{"auth_server_port",			hapd_get_auth_server_port},
	{"auth_server_shared_secret",	hapd_get_auth_server_shared_secret},
	{"acct_server_addr",			hapd_get_acct_server_addr},
	{"acct_server_port",			hapd_get_acct_server_port},
	{"acct_server_shared_secret",	hapd_get_acct_server_shared_secret},

	{"auth_server_addr2",			hapd_get_auth_server_addr_2},
	{"auth_server_port2",			hapd_get_auth_server_port_2},
	{"auth_server_shared_secret2",	hapd_get_auth_server_shared_secret_2},
	{"acct_server_addr2",			hapd_get_acct_server_addr_2},
	{"acct_server_port2",			hapd_get_acct_server_port_2},
	{"acct_server_shared_secret2",	hapd_get_acct_server_shared_secret_2},
//
#if 0
#ifdef CONFIG_MBO_SUPPORT
	{"ieee80211d",				hapd_get_ieee80211d},
#endif
#ifdef CONFIG_RTL_DFS_SUPPORT
	{"ieee80211h",				hapd_get_ieee80211h},
#endif
#endif
#if 0
	{"ht_capab",				hapd_get_ht_capab},	//Short GI, STBC
	{"vht_capab",				hapd_get_vht_capab},

	{"vht_oper_chwidth",		hapd_get_vht_oper_chwidth},
	{"channel",					hapd_get_channel},
	{"vht_oper_centr_freq_seg0_idx",	hapd_get_vht_oper_centr_freq_seg0_idx},

	{"bssid",					hapd_get_bssid},
	{"country_code",			hapd_get_country_code},
	{"ieee80211d",				hapd_get_ieee80211d},

//Advanced
	{"fragm_threshold",			hapd_get_fragm_threshold},
	{"rts_threshold",			hapd_get_rts_threshold},
	{"beacon_int",				hapd_get_beacon_int},
	
	{"preamble",				hapd_get_preamble},
#endif
	{"dtim_period",				hapd_get_dtim_period},
//
	{"data_rate",				hapd_get_data_rate},
	{"ignore_broadcast_ssid",	hapd_get_ignore_broadcast_ssid},
	{"wmm_enabled",				hapd_get_wmm_enabled},
	{"uapsd_advertisement_enabled", hapd_get_uapsd_advertisement_enabled},
	{"ap_isolate",				hapd_get_ap_isolate},
#ifdef CONFIG_GUEST_ACCESS_SUPPORT
	{"access_network_type",		hapd_get_access_network_type},
#endif
//acl
	{"macaddr_acl",				hapd_get_macaddr_acl},

//WPS
#ifdef WLAN_WPS
#ifdef WLAN_WPS_VAP
	{"wps_state",				hapd_get_wps_state},
	{"wps_independent",			hapd_get_wps_independent},
	{"wps_cred_processing",		hapd_get_wps_cred_processing},
	{"uuid",					hapd_get_wps_uuid},
	{"device_name",				hapd_get_wps_device_name},
	{"model_name",				hapd_get_wps_model_name},
	{"model_number",			hapd_get_wps_model_number},
	{"serial_number",			hapd_get_wps_serial_number},
	{"device_type",				hapd_get_wps_device_type},
	{"config_methods",			hapd_get_wps_config_methods},
	{"pbc_in_m1",				hapd_get_wps_pbc_in_m1},
	{"ap_pin",					hapd_get_wps_ap_pin},
	{"upnp_iface",				hapd_get_wps_upnp_iface},
	{"friendly_name",			hapd_get_wps_friendly_name},
	{"manufacturer_url",		hapd_get_wps_manufacturer_url},
	{"model_url",				hapd_get_wps_model_url},
#endif
#endif

//EAP
	{"eap_server",				hapd_get_eap_server},

//11KV
	{"bss_transition",			hapd_get_bss_transition},
	{"rrm_neighbor_report", 	hapd_get_rrm_neighbor_report},
	{"rrm_beacon_report",		hapd_get_rrm_beacon_report},

//11R
	{"mobility_domain", 		hapd_get_mobility_domain},
	{"ft_over_ds",				hapd_get_ft_over_ds},
	{"ft_r0_key_lifetime",		hapd_get_ft_r0_key_lifetime},
	{"reassociation_deadline",	hapd_get_reassociation_deadline},
	{"r1_key_holder",			hapd_get_r1_key_holder},
	{"r0kh",					hapd_get_r0kh_r1kh},
	{"pmk_r1_push", 			hapd_get_pmk_r1_push},

	//station
	{"max_num_sta",				hapd_get_max_num_sta},
	
#ifdef RTK_MULTI_AP
	{"multi_ap",				hapd_get_multi_ap},
#ifdef WLAN_WPS
	{"multi_ap_backhaul_ssid",	hapd_get_multi_ap_backhaul_ssid},
	{"multi_ap_backhaul_wpa_psk", hapd_get_multi_ap_backhaul_wpa_psk},
	{"multi_ap_backhaul_wpa_passphrase", hapd_get_multi_ap_backhaul_wpa_passphrase},
#endif
#endif
#ifdef CONFIG_MBO_SUPPORT
	//MBO
	{"mbo",						hapd_get_mbo},
	{"oce",						hapd_get_oce},
	{"interworking",			hapd_get_interworking},
	{"country3",				hapd_get_country3},
	{"gas_address3",			hapd_get_gas_address3},
#endif

};

#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
struct hapd_obj	wpas_conf_table[] =
{
	{"ctrl_interface",			hapd_get_ctrl_interface},
	{"update_config",			hapd_get_wpas_update_config},
#ifdef WLAN_WPS
	{"uuid",					hapd_get_wps_uuid},
	{"device_name", 			hapd_get_wps_device_name},
	{"manufacturer",			hapd_get_wps_manufacturer},
	{"model_name",				hapd_get_wps_model_name},
	{"model_number",			hapd_get_wps_model_number},
	{"serial_number",			hapd_get_wps_serial_number},
	{"device_type", 			hapd_get_wps_device_type},
	{"config_methods",			hapd_get_wps_config_methods},
#endif
#ifdef WLAN_WPA3_H2E_WPAS
	{"sae_pwe", 				hapd_get_sae_pwe},
//	{"transition_disable",		hapd_get_transition_disable},
#endif
	{"network={",				hapd_get_wpas_network_start},
	{"ssid",					hapd_get_ssid},
	{"scan_ssid",				hapd_get_wpas_scan_ssid},
	{"key_mgmt",				hapd_get_wpa_key_mgmt},
#ifdef WLAN_11W
	{"ieee80211w",			hapd_get_ieee80211w},
#endif
	{"pairwise",				hapd_get_wpas_pairwise},
	{"group",					hapd_get_wpas_group},
	{"psk",						hapd_get_wpas_psk},
	{"sae_password",			hapd_get_wpas_sae_password},
	{"pbss",					hapd_get_wpas_pbss},
	{"proto",					hapd_get_wpas_proto},
	{"auth_alg",				hapd_get_wpas_auth_algs},
	{"wep_key0",				hapd_get_wep_key0},
	{"wep_key1",				hapd_get_wep_key1},
	{"wep_key2",				hapd_get_wep_key2},
	{"wep_key3",				hapd_get_wep_key3},
	{"wep_tx_keyidx",			hapd_get_wep_tx_keyidx},
#ifdef RTK_MULTI_AP
	{"multi_ap_backhaul_sta",	hapd_get_multi_ap_backhaul_sta},
#endif
	{"}",						hapd_get_wpas_network_start},
};
#endif

void dump_tmp(unsigned char *tmp)
{
	int idx = 0;

	printf("\n tmp=");

	for(idx=0; idx<50; idx++)
	printf("%02x ",tmp[idx]);

	printf("\n\n");
}

void set_hapd_vif_cfg(FILE *hapd_cfg,struct wifi_config *wifi_cfg, int vap_idx)
{
	int idx = 0;
	char result[HAPD_CONF_RESULT_MAX_LEN]={0};
	char name[64]={0};

	printf("\nGenerating VAP#%d \"%s\" configuration\n", vap_idx,
	        wifi_cfg->interface);
#if 1
	fprintf(hapd_cfg, "\n# VAP#%d \"%s\" configuration\n\n", vap_idx,
	       wifi_cfg->interface);

#ifdef WLAN_11R
	if(wifi_cfg->fp == NULL)
		wifi_cfg->fp = hapd_cfg;
#endif

	for (idx = 0; idx < ARRAY_SIZE(vap_hapd_conf_table); idx++) {

		// printf("[%s][%d] idx=%d name(%s) \n", __FUNCTION__, __LINE__, idx, vap_hapd_conf_table[idx].name);
		memset(result, 0, sizeof(result));

		//should use interface in vif conf
		if(!strcmp(vap_hapd_conf_table[idx].name, "bss"))
			strcpy(name, "interface");
		else
			strcpy(name, vap_hapd_conf_table[idx].name);

		if (   vap_hapd_conf_table[idx].fun(wifi_cfg,
		                                    name,
		                                    result)
		    == HAPD_GET_OK) {
			if (result[0] == 0)
				continue;

			fprintf(hapd_cfg, "%s", result);

			//printf("[%s][%d] result:%s \n",
			//	__FUNCTION__, __LINE__, result);
		}
		else
			printf("[%s][%d] config get \"%s\" fail or NO need to config this item\n",
			       __FUNCTION__, __LINE__, name);
	}
#endif

}


void set_hapd_cfg(FILE *hapd_cfg,struct wifi_config *wifi_cfg)
{
	int idx=0, vap_idx=0;
	char result[HAPD_CONF_RESULT_MAX_LEN]={0};

#ifdef WLAN_11R
	wifi_cfg->fp = hapd_cfg;
#endif

	for(idx=0; idx<ARRAY_SIZE(hapd_conf_table); idx++){

		//printf("[%s][%d] idx=%d name(%s) \n", __FUNCTION__, __LINE__, idx, hapd_conf_table[idx].name);
		memset(result, 0, sizeof(result));

		if(hapd_conf_table[idx].fun(wifi_cfg, hapd_conf_table[idx].name, result) == HAPD_GET_OK){
			fprintf(hapd_cfg, "%s", result);

			//printf("[%s][%d] result:%s \n",
			//	__FUNCTION__, __LINE__, result);
		}
		else
			printf("[%s][%d] config get fail or NO need to config this item\n", __FUNCTION__, __LINE__);
	}
	for (vap_idx = WLAN_VAP_ITF_INDEX; vap_idx < (WLAN_VAP_ITF_INDEX+WLAN_VAP_USED_NUM); vap_idx++) {
		if (wifi_cfg[vap_idx].disabled) {
			printf("\n%s VAP%d disabled\n", wifi_cfg[0].interface, vap_idx);
			fprintf(hapd_cfg, "\n# %s VAP#%d disabled\n\n", wifi_cfg[0].interface, vap_idx);
			continue;
		}

		printf("\nGenerating VAP%d \"%s\" configuration\n", vap_idx,
		        wifi_cfg[vap_idx].interface);
#if 1
		fprintf(hapd_cfg, "\n# VAP#%d \"%s\" configuration\n\n", vap_idx,
		        wifi_cfg[vap_idx].interface);

		for (idx = 0; idx < ARRAY_SIZE(vap_hapd_conf_table); idx++) {

			// printf("[%s][%d] idx=%d name(%s) \n", __FUNCTION__, __LINE__, idx, vap_hapd_conf_table[idx].name);
			memset(result, 0, sizeof(result));

			if (   vap_hapd_conf_table[idx].fun(&wifi_cfg[vap_idx],
			                                    vap_hapd_conf_table[idx].name,
			                                    result)
			    == HAPD_GET_OK) {
				if (result[0] == 0)
					continue;

				fprintf(hapd_cfg, "%s", result);

				//printf("[%s][%d] result:%s \n",
				//	__FUNCTION__, __LINE__, result);
			}
			else
				printf("[%s][%d] config get \"%s\" fail or NO need to config this item\n",
				       __FUNCTION__, __LINE__, vap_hapd_conf_table[idx].name);
		}
#endif
	}

}


void gen_hapd_cfg(int if_idx, char *conf_path)
{
	FILE *hapd_cfg;
	struct wifi_config wifi_cfg[1+NUM_VWLAN_INTERFACE]={0};
	char *default_conf_path;

	if(conf_path == NULL)
		default_conf_path = CONF_PATH(if_idx);

	printf("%s \n",__FUNCTION__);

	memset(wifi_cfg, 0, sizeof(wifi_cfg));

	if(get_wifi_conf(&wifi_cfg[0],if_idx))
		return;

	if(conf_path == NULL)
		hapd_cfg = fopen(default_conf_path, "w+");
	else
		hapd_cfg = fopen(conf_path, "w+");

	if(hapd_cfg == NULL){
		printf("!! Error,  [%s][%d] conf = NULL \n", __FUNCTION__, __LINE__);
		return;
	}

	set_hapd_cfg(hapd_cfg,&wifi_cfg[0]);

	fclose(hapd_cfg);
}

int gen_hapd_vif_cfg(int if_idx, int vwlan_idx, char *conf_path)
{
	FILE *hapd_cfg=NULL;
	struct wifi_config wifi_cfg={0};
	char *default_conf_path;

	//if(conf_path == NULL)
	//	default_conf_path = CONF_PATH(if_idx);

	memset(&wifi_cfg, 0, sizeof(wifi_cfg));

#if defined(WLAN_MBSSID_NUM) && (WLAN_MBSSID_NUM > 0)
	if(get_wifi_vif_conf(&wifi_cfg,if_idx, vwlan_idx))
		return -1;
#else
	printf("!! Error,  [%s][%d] conf = NULL \n", __FUNCTION__, __LINE__);
	return -1;
#endif

	if(conf_path == NULL){
		//hapd_cfg = fopen(default_conf_path, "w+");
		printf("!! Error,  [%s][%d] conf = NULL \n", __FUNCTION__, __LINE__);
		return -1;
	}
	else
		hapd_cfg = fopen(conf_path, "w+");

	if(hapd_cfg == NULL){
		printf("!! Error,  [%s][%d] conf = NULL \n", __FUNCTION__, __LINE__);
		return -1;
	}

	set_hapd_vif_cfg(hapd_cfg,&wifi_cfg, vwlan_idx);

	fclose(hapd_cfg);

	return 0;
}


#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
void set_wpas_cfg(FILE *wpas_cfg, struct wifi_config *wifi_cfg)
{
	int idx, vap_idx;
#ifdef WLAN_UNIVERSAL_REPEATER
	if(wifi_cfg->is_repeater)
		fprintf(wpas_cfg, "# Vxd CLIENT configuration\n\n");
	else
#endif
		fprintf(wpas_cfg, "# Root CLIENT configuration\n\n");
	for(idx=0; idx<ARRAY_SIZE(wpas_conf_table); idx++){
		char result[HAPD_CONF_RESULT_MAX_LEN] = {0};

		if (wpas_conf_table[idx].fun(wifi_cfg, wpas_conf_table[idx].name, result) == HAPD_GET_OK){
			fprintf(wpas_cfg, "%s", result);
		} else
			printf("[%s][%d] config get \"%s\" fail or NO need to config this item\n",
			       __FUNCTION__, __LINE__, wpas_conf_table[idx].name);
	}
}

void gen_wpas_cfg(int if_idx, int index, char *conf_path)
{
	FILE *wpas_cfg;
	struct wifi_config wifi_cfg;
#ifdef WLAN_WPS_WPAS
	FILE *wpas_wps_stas;
#endif
	char *wpas_conf_path = conf_path;
#ifdef WLAN_UNIVERSAL_REPEATER
	char phy_name[IFNAMSIZ]={0};
	char tmp_cmd[50]={0};
	MIB_CE_MBSSIB_T Entry;
#endif
	memset(&wifi_cfg, 0, sizeof(wifi_cfg));

#ifdef WLAN_UNIVERSAL_REPEATER
	if(index == WLAN_REPEATER_ITF_INDEX)
	{
		wifi_cfg.is_repeater = 1;
	}
#endif

	if(wpas_conf_path == NULL){
#ifdef WLAN_UNIVERSAL_REPEATER
		if(index == WLAN_REPEATER_ITF_INDEX)
			wpas_conf_path = WPAS_CONF_PATH_VXD(if_idx);
		else
#endif
			wpas_conf_path = WPAS_CONF_PATH(if_idx);
	}

	printf("%s \n",__FUNCTION__);
#if defined(WIFI5_WIFI6_COMP) && defined(CONFIG_RTK_DEV_AP)
	if (get_wifi_conf(&wifi_cfg, if_idx))
		return;
#else
	if (get_wifi_wpas_conf(&wifi_cfg, if_idx))
		return;
#endif
	wpas_cfg = fopen(wpas_conf_path, "w+");

	if(wpas_cfg == NULL){
		printf("!! Error,  [%s][%d] wpas_conf = NULL \n", __FUNCTION__, __LINE__);
		return;
	}

#ifdef WLAN_UNIVERSAL_REPEATER
	if(index == WLAN_REPEATER_ITF_INDEX)
	{
		memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));
		mib_chain_get(MIB_MBSSIB_TBL, WLAN_REPEATER_ITF_INDEX, &Entry);
#ifdef WIFI5_WIFI6_COMP
		if(Entry.is_ax_support)
#endif
		{
			memset(tmp_cmd, 0, sizeof(tmp_cmd));
			snprintf(tmp_cmd, sizeof(tmp_cmd), "/var/exist_wlan%d-vxd", if_idx);
			if(-1 == access(tmp_cmd, F_OK))
			{
#ifdef BAND_2G_ON_WLAN0
				if(if_idx == 0)
					snprintf(phy_name, sizeof(phy_name), "phy1");
				else if(if_idx == 1)
					snprintf(phy_name, sizeof(phy_name), "phy0");
#else
				snprintf(phy_name, sizeof(phy_name), "phy%c", wifi_cfg.interface[4]);
#endif
				va_cmd("/bin/iw", 7, 1, "phy", phy_name, "interface", "add", wifi_cfg.interface,  "type", "station");
				memset(tmp_cmd, 0, sizeof(tmp_cmd));
				snprintf(tmp_cmd, sizeof(tmp_cmd), "echo 1 > /var/exist_wlan%d-vxd", if_idx);
				system(tmp_cmd);
			}
		}

#if defined(WIFI5_WIFI6_COMP) && !defined(WLAN_NOT_USE_MIBSYNC)
		start_wifi_priv_cfg(if_idx, index);
#endif
	}
#endif

#ifdef WIFI5_WIFI6_COMP
	if(
#ifdef WLAN_UNIVERSAL_REPEATER
		Entry.is_ax_support ||
#endif
		index != WLAN_REPEATER_ITF_INDEX)
#endif
		va_cmd(IFCONFIG, 4, 1, wifi_cfg.interface,  "hw", "ether", wifi_cfg.bssid);

	set_wpas_cfg(wpas_cfg, &wifi_cfg);

	fclose(wpas_cfg);

#ifdef WLAN_WPS_WPAS
	if(wifi_cfg.wps_state)
	{
		wpas_wps_stas = fopen("/var/wpas_wps_stas", "w+");
		if(wpas_wps_stas == NULL){
			printf("!! Error,  [%s][%d] wpas_wps_stas = NULL \n", __FUNCTION__, __LINE__);
			mib_recov_wlanIdx();
			return;
		}
		fprintf(wpas_wps_stas, "%s", "PBC Status: Disabled\n");
		fprintf(wpas_wps_stas, "%s", "Last WPS result: None\n");
		fclose(wpas_wps_stas);
	}
#endif
}
#endif

#if 0
void start_hapd_process(void)
{
	int i, orig_wlan_idx = 0;
	MIB_CE_MBSSIB_T Entry;

	orig_wlan_idx = wlan_idx;

	for(i = 0; i<NUM_WLAN_INTERFACE; i++){
		//wlan_idx = i;
		mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, 0, &Entry);

		printf("[%d] wlanDisabled=%d \n", i, Entry.wlanDisabled);

		if (Entry.wlanDisabled){
			continue;
		}

		printf("gen_hapd_cfg & launch: wlan_idx = %d \n",wlan_idx);
		gen_hapd_cfg(wlan_idx, NULL);
		//sleep(2);

		if(wlan_idx == 0){
			va_cmd("/bin/hostapd", 4, 1, CONF_PATH_WLAN0, "-P", PID_PATH_WLAN0, "-B");
			//sleep(5);
		}
		else if(wlan_idx == 1){
			va_cmd("/bin/hostapd", 4, 1, CONF_PATH_WLAN1, "-P", PID_PATH_WLAN1, "-B");
			//sleep(5);
		}
	}

	wlan_idx = orig_wlan_idx;

}

void stop_hapd_process(void)
{
	int hapd_pid;

	hapd_pid = read_pid((char*)PID_PATH_WLAN0);
	if(hapd_pid > 0){
		kill(hapd_pid, 9);
		unlink(PID_PATH_WLAN0);
	}

	hapd_pid = read_pid((char*)PID_PATH_WLAN1);
	if(hapd_pid > 0){
		kill(hapd_pid, 9);
		unlink(PID_PATH_WLAN1);
	}
}
#endif

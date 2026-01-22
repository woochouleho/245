#ifndef WIFI6_PRIV_CONF_H
#define WIFI6_PRIV_CONF_H

#ifdef CONFIG_KERNEL_5_10_x
#include "../../../../linux-5.10.x/drivers/net/wireless/realtek/g6_wifi_driver/include/rtw_mib.h"
#else
#include "../../../../linux-4.4.x/drivers/net/wireless/realtek/g6_wifi_driver/include/rtw_mib.h"
#endif
/*
struct wifi_mib_priv {
	unsigned char	func_off;
	unsigned char	func_off_prev;
	unsigned char	disable_protection;
	unsigned char	aggregation;
	unsigned char	iapp_enable;
	//unsigned int	basicrates;
	unsigned int	a4_enable;
	unsigned char	multiap_monitor_mode_disable;
	unsigned char	radio_power;
	unsigned int	gbwcmode;
	unsigned int	gbwcthrd_tx;
	unsigned int	gbwcthrd_rx;
	//unsigned int	stanum;
	unsigned char	telco_selected;
	unsigned char	regdomain;
	unsigned int	led_type;
	unsigned int	lifetime;
	unsigned int	manual_priority;
	unsigned int	opmode;
	unsigned int	autorate;
	unsigned int	fixrate;
	unsigned int	deny_legacy;
	unsigned int	lgyEncRstrct;
	unsigned int	cts2self;
	unsigned int	coexist;
	unsigned char	ampdu;
	unsigned char	amsdu;
	unsigned char	crossband_enable;
	unsigned char	stactrl_enable;
	unsigned int	stactrl_rssi_th_high;
	unsigned int	stactrl_rssi_th_low;
	unsigned int	stactrl_ch_util_th;
	unsigned int	stactrl_steer_detect;
	unsigned int	stactrl_sta_load_th_2g;
	unsigned char	stactrl_prefer_band;
	unsigned char	stactrl_groupID;
	unsigned int	monitor_sta_enabled;
	unsigned int	txbf;
	unsigned int	txbf_mu;
	unsigned int	ft_enable;
	unsigned char	ft_res_request;
	unsigned char	rm_activated;
	unsigned char	rm_link_measure;
	unsigned char	rm_beacon_passive;
	unsigned char	rm_beacon_active;
	unsigned char	rm_beacon_table;
	unsigned char	rm_neighbor_report;
	unsigned char	rm_ap_channel_report;
	unsigned char	Is11kDaemonOn;
	unsigned char	BssTransEnable;
	unsigned char	sta_roaming_time_gap;
	unsigned char	sta_roaming_rssi_gap;
	unsigned char	roaming_switch;
	unsigned char	roaming_qos;
	unsigned char	fail_ratio;
	unsigned char	retry_ratio;
	unsigned char	block_aging;
	unsigned char	RSSIThreshold;
	unsigned char	dfgw_mac[MACADDRLEN];
	unsigned char	roaming_enable;
	unsigned int	roaming_start_time;
	unsigned char	roaming_rssi_th1;
	unsigned char	roaming_rssi_th2;
	unsigned int	roaming_wait_time;
	unsigned char	set_txpwr;
	unsigned char	band;
	unsigned char	ssid[33];
	unsigned int	authtype;
	unsigned char	encmode;
	unsigned char	multiap_bss_type;
};
*/

enum { IS_WLAN_ROOT=0, IS_WLAN_VAP=1, IS_WLAN_VXD=2};

#define WIFI_PRIV_GET_OK		0
#define WIFI_PRIV_GET_FAIL		1

void start_wifi_priv_cfg(int wlan_index, int vwlan_index);
void gen_wifi_priv_cfg(struct wifi_mib_priv *wifi_mib, int wlan_idx, int vwlan_idx);

int get_wifi_mib_priv(int skfd, char *ifname, struct wifi_mib_priv *wifi_mib);
int set_wifi_mib_priv(int skfd, char *ifname, struct wifi_mib_priv *wifi_mib);

//===================================
#endif


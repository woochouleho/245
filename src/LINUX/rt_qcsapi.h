#ifndef __RT_QCAPI_H__
#define __RT_QCAPI_H__

#include <rtk/utility.h>
#ifdef WLAN_SUPPORT
#include <rtk/subr_wlan.h>

extern void startWLan_qtn();
extern int stopwlan_qtn();
extern char* rt_get_qtn_ifname(char *ifname);
extern void rt_report_qtn_button_state();
extern void rt_check_qtn_wps_config();
extern int rt_qcsapi_get_status(char *ifname, char *status);
extern int rt_qcsapi_get_sta_num(char *interface, int *number);
extern int rt_qcsapi_get_sta_mac_addr(char *ifname, unsigned int index, unsigned char *mac_addr);
extern int rt_qcsapi_get_channel_list(char *ifname, int region_by_index, unsigned int bw, char *channel_list);
extern int rt_qcsapi_get_bss_info(char *interface, bss_info *pInfo);
extern int rt_qcsapi_get_sta_tx_pkt(char* ifname, unsigned int index, unsigned int *number);
extern int rt_qcsapi_get_sta_rx_pkt(char *ifname, unsigned int index, unsigned int *number);
extern int rt_qcsapi_get_sta_tx_phy_rate(char *ifname, unsigned int index, unsigned int *tx_phy_rate);
extern int rt_qcsapi_get_sta_rssi(char *ifname, unsigned int index, unsigned int *rssi);
extern int rt_qcsapi_get_sta_snr(char *ifname, unsigned int index, unsigned int *snr);
extern void createQTN_targetIPconf();
extern int ping_qtn_check();
#endif

#endif

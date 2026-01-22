#ifndef INCLUDE_SUBR_CTCSDN_H
#define INCLUDE_SUBR_CTCSDN_H

enum{
	OFP_PTYPE_LAN = 0,
	OFP_PTYPE_WLAN2G,
	OFP_PTYPE_WLAN5G,
	OFP_PTYPE_WAN,
	OFP_PTYPE_VPN_L2TP,
	OFP_PTYPE_VPN_PPTP,
};

enum{
	// LAN
	OFP_PID_LAN_START=1,
	OFP_PID_LAN_1=OFP_PID_LAN_START,
	OFP_PID_LAN_2,
	OFP_PID_LAN_3,
	OFP_PID_LAN_4,
	OFP_PID_LAN_END=OFP_PID_LAN_4,
	// WLAN 2.4G
	OFP_PID_WLAN2G_START=5,
	OFP_PID_WLAN2G_1=OFP_PID_WLAN2G_START,
	OFP_PID_WLAN2G_2,
	OFP_PID_WLAN2G_3,
	OFP_PID_WLAN2G_4,
	OFP_PID_WLAN2G_5,
	OFP_PID_WLAN2G_6,
	OFP_PID_WLAN2G_7,
	OFP_PID_WLAN2G_8,
	OFP_PID_WLAN2G_END=OFP_PID_WLAN2G_8,
	// WLAN 5G
	OFP_PID_WLAN5G_START=13,
	OFP_PID_WLAN5G_1=OFP_PID_WLAN5G_START,
	OFP_PID_WLAN5G_2,
	OFP_PID_WLAN5G_3,
	OFP_PID_WLAN5G_4,
	OFP_PID_WLAN5G_5,
	OFP_PID_WLAN5G_6,
	OFP_PID_WLAN5G_7,
	OFP_PID_WLAN5G_8,
	OFP_PID_WLAN5G_END=OFP_PID_WLAN5G_8,
	// WAN
	OFP_PID_WAN_START=21,
	OFP_PID_WAN_1=OFP_PID_WAN_START,
	OFP_PID_WAN_2,
	OFP_PID_WAN_3,
	OFP_PID_WAN_4,
	OFP_PID_WAN_5,
	OFP_PID_WAN_6,
	OFP_PID_WAN_7,
	OFP_PID_WAN_8,
	OFP_PID_WAN_END=OFP_PID_WAN_8,
	// VPN L2TP
	OFP_PID_VPN_L2TP_START=29,
	OFP_PID_VPN_L2TP_1=OFP_PID_VPN_L2TP_START,
	OFP_PID_VPN_L2TP_2,
	OFP_PID_VPN_L2TP_3,
	OFP_PID_VPN_L2TP_4,
	OFP_PID_VPN_L2TP_END=OFP_PID_VPN_L2TP_4,
	// VPN PPTP
	OFP_PID_VPN_PPTP_START,
	OFP_PID_VPN_PPTP_1=OFP_PID_VPN_PPTP_START,
	OFP_PID_VPN_PPTP_2,
	OFP_PID_VPN_PPTP_3,
	OFP_PID_VPN_PPTP_4,
	OFP_PID_VPN_PPTP_END=OFP_PID_VPN_PPTP_4,
	OFP_PID_MAX
};

typedef struct {
	unsigned char portType;
	char name[64];
	char ifname[16];
	unsigned int ofportID;
	unsigned char sdn_enable;
}OVS_OFPPORT_INTF;

int check_ovs_running(void);
int check_ovs_enable(void);
int get_ovs_datapath_id(char *value, int size);
int get_ovs_available_interfaces(OVS_OFPPORT_INTF *list, int list_size);
int find_ovs_interface_mibentry(OVS_OFPPORT_INTF *intf, void **mib_entry, int *mib_index, int *mib_index2);
int get_ovs_phyport_by_mibentry(int portType, void *mib_entry, int mib_index, int mib_index2);
int check_ovs_interface_by_mibentry(int portType, void *mib_entry, int mib_index, int mib_index2);
int check_ovs_interface(OVS_OFPPORT_INTF *intf);
int bind_ovs_interface_by_mibentry(int portType, void *mib_entry, int mib_index, int mib_index2);
int bind_ovs_interface(OVS_OFPPORT_INTF *intf);
int unbind_ovs_interface_by_mibentry(int portType, void *mib_entry, int mib_index, int mib_index2, int dontConfig);
int unbind_ovs_interface(OVS_OFPPORT_INTF *list);
int config_ovs_port(int bind);
int stop_ovs_sdn(void);
int start_ovs_sdn(void);
int config_ovs_sdn(int configMode);
int setup_controller_route_rule(int set);
int config_ovs_controller(void);
int check_ovs_loglevel(void);
#endif

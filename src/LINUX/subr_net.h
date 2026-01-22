#ifndef INCLUDE_SUBR_NET_H
#define INCLUDE_SUBR_NET_H

#include "utility.h"

enum { ROUTE_ADD, ROUTE_DEL };

#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
#define RTK_IGMP_MLD_SNOOPING_MODULE_IPV4_PROC_CONFIG	"/proc/igmp/ctrl/igmpConfig"
#define RTK_IGMP_MLD_SNOOPING_MODULE_IPV6_PROC_CONFIG	"/proc/igmp/ctrl/igmp6Config"
#define RTK_IGMP_MLD_SNOOPING_MODULE_BR_PROC_CONFIG	"/proc/igmp/ctrl/brConfig"
#define RTK_IGMP_MLD_SNOOPING_MODULE_WHITELIST_PROC_CONFIG	"/proc/igmp/ctrl/whiteList"
#define RTK_IGMP_MLD_SNOOPING_MODULE_BLACKLIST_PROC_CONFIG	"/proc/igmp/ctrl/blackList"
void rtk_igmp_mld_snooping_module_config(const char *ifName, unsigned char *option, unsigned int value, unsigned char *procName);
int rtk_igmp_mld_snooping_module_init(const char *brName, int configAll, MIB_CE_ATM_VC_Tp pEntry);
int rtk_set_multicast_ignoreGroup_rule(int config, int *pindex, const char *start_group, const char *end_greoup, int force2ps);
void rtk_set_multicast_ignoreGroup(void);
int rtk_set_all_lan_igmp_blacklist_by_mac(int is_set, char *mac);
#endif

#ifdef CONFIG_USER_IGMPTR247
int rtk_tr247_multicast_init(void);
int rtk_tr247_multicast_silent_discarding_igmp(const char *ifNameByRule);
void rtk_tr247_multicast_profile_fw_init(unsigned int profileId);
void rtk_tr247_multicast_profile_fw_remove(unsigned int profileId);
int rtk_tr247_multicast_apply(APPLY_TR247_MC_T opt, unsigned short profileId, unsigned int portId, unsigned int aclTableEntryId);
#endif
#define WANMARK_TYPE_SYS 0x1
#define WANMARK_TYPE_APP 0x2
int rtk_wan_sockmark_remove(char *ifname, unsigned int type);
int rtk_wan_sockmark_add(char *ifname, unsigned int *mark, unsigned int *mask, unsigned int type);
int rtk_wan_sockmark_tblid_get(char *ifname, unsigned int *mark, unsigned int *mask, int *table, unsigned int *type);
int rtk_wan_sockmark_get(char *ifname, unsigned int *mark, unsigned int *mask);
int rtk_wan_tblid_get(char *ifname);
int rtk_sockmark_init(void);
int rtk_net_add_wan_policy_route_mark(char *ifname, unsigned int *mark, unsigned int *mask);
int rtk_net_remove_wan_policy_route_mark(char *ifname);
int rtk_net_get_wan_policy_route_mark(char *ifname, unsigned int *mark, unsigned int *mask);
int rtk_net_get_from_lan_with_wan_policy_route_mark(char *ifname, unsigned int *mark, unsigned int *mask);

//void check_staticRoute_change(char *ifname);
//int rtk_route_cfg_modify(MIB_CE_IP_ROUTE_T *, int, int entryID);
//void route_ppp_ifup(unsigned long pppGW, char *ifname);
#ifdef CONFIG_USER_PPPD
void connectPPP(char *inf, MIB_CE_ATM_VC_Tp pEntry, CHANNEL_MODE_T pppEncap);
void connectPPPExt(char *inf, MIB_CE_ATM_VC_Tp pEntry, CHANNEL_MODE_T pppEncap, int isSimu);
void disconnectPPP(unsigned char pppidx);
int reWriteAllpppdScript_ipup();
#if defined(CONFIG_SUPPORT_AUTO_DIAG)
void write_to_simu_ppp_options(MIB_CE_ATM_VC_Tp pEntry);
#endif
int generate_pppd_init_script(MIB_CE_ATM_VC_Tp pEntry);
int generate_pppd_ipup_script(MIB_CE_ATM_VC_Tp pEntry);
int generate_pppd_ipdown_script(MIB_CE_ATM_VC_Tp pEntry);

extern const char PPPD[];
extern const char PPPOE_PLUGIN[];
extern const char PPPOA_PLUGIN[];
#ifdef CONFIG_USER_XL2TPD
extern const char XL2TPD[];
extern const char XL2TPD_PID[];
extern const char XL2TP_PLUGIN[];
#endif
#ifdef CONFIG_USER_PPTP_SERVER
extern const char PPTPD[];
extern const char PPTPD_PID[];
#endif
#endif //CONFIG_USER_PPPD

#ifdef CONFIG_USER_PPTP_CLIENT_PPTP
extern const char NF_PPTP_ROUTE_CHAIN[];
extern const char NF_PPTP_ROUTE_COMMENT[];
#ifdef VPN_MULTIPLE_RULES_BY_FILE
extern const char NF_PPTP_IPSET_IPRANGE_CHAIN[];
extern const char NF_PPTP_IPSET_IPSUBNET_CHAIN[];
extern const char NF_PPTP_IPSET_SMAC_CHAIN[];
extern const char NF_PPTP_IPSET_RULE_INDEX_COMMENT[];
extern const char NF_PPTP_IPSET_IPRANGE_RULE_COMMENT[];
#endif
#endif

#ifdef CONFIG_USER_L2TPD_L2TPD
extern const char NF_L2TP_ROUTE_CHAIN[];
extern const char NF_L2TP_ROUTE_COMMENT[];
#ifdef VPN_MULTIPLE_RULES_BY_FILE
extern const char NF_L2TP_IPSET_IPRANGE_CHAIN[];
extern const char NF_L2TP_IPSET_IPSUBNET_CHAIN[];
extern const char NF_L2TP_IPSET_SMAC_CHAIN[];
extern const char NF_L2TP_IPSET_RULE_INDEX_COMMENT[];
extern const char NF_L2TP_IPSET_IPRANGE_RULE_COMMENT[];
#endif
#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD)
extern const char NF_VPN_ROUTE_CHAIN[];
extern const char NF_VPN_ROUTE_TABLE[];
extern const char FW_VPNGRE[];
#endif

#ifdef CONFIG_USER_RETRY_GET_FLOCK
#define MAX_READLANNETINFO_INTERVAL 50
#define MAX_GETWANCONFINFO_INTERVAL 60
#define MAX_GETPORTMAPINFO_INTERVAL 60
#define MAX_READPORTSPEEDINFO_INTERVAL 50
#endif

int rtk_wan_vpn_pptp_set_default_policy_route_rule(void);
int rtk_wan_vpn_l2tp_set_default_policy_route_rule(void);
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD)
unsigned long long rtk_wan_vpn_get_attached_pkt_count_by_ifname(VPN_TYPE_T vpn_type, unsigned char *if_name);
#endif
#ifdef CONFIG_USER_L2TPD_L2TPD
int rtk_wan_vpn_set_l2tp_policy_route_all(void);
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
int rtk_wan_vpn_set_l2tp_policy_route(unsigned char *tunnelName);
#endif
#endif
#ifdef CONFIG_USER_PPTP_CLIENT_PPTP
int rtk_wan_vpn_set_pptp_policy_route_all(void);
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
int rtk_wan_vpn_set_pptp_policy_route(unsigned char *tunnelName);
int rtk_wan_vpn_flush_pptp_dynamic_url_policy_route(unsigned char *tunnelName);
#endif
#endif

#if defined(CONFIG_USER_XL2TPD)
int connect_xl2tpd(unsigned int pppindex);
void write_to_l2tp_config(void);
#ifdef CONFIG_USER_L2TPD_LNS
void write_to_l2tp_lns_config(void);
#endif
#endif

#ifdef CONFIG_USER_PPTP_CLIENT
int connect_pptp(unsigned int pppindex);
void write_to_pptp_config(void);
#endif

#if defined(CONFIG_USER_PPTP_CLIENT) || defined(CONFIG_USER_XL2TPD)
int rtk_add_ppp_vpn_hostroute(const char *vpn_ifname, struct in_addr *remote_addr);
#endif

#ifdef CONFIG_USER_PPTP_SERVER
void write_to_pptp_server_config(void);
#endif

#if defined(CONFIG_USER_DNSMASQ_DNSMASQ)
int delete_dsldevice_on_hosts(void);
int add_dsldevice_on_hosts(void);
int reloadDnsRelay(void);
#endif
int reWriteAllDhcpcScript(void);
int get_network_dns(char *dns1, char *dns2);
int set_dhcp_port_base_filter(int enable);
void stopAddressMap(MIB_CE_ATM_VC_Tp pEntry);
int set_TR069_Firewall(int enable);
int rtk_tr069_1p_remarking(char cwmp_1p_value);
int mask2prefix(struct in_addr mask);
char *get_lan_ipnet();
int startAddressMap(MIB_CE_ATM_VC_Tp pEntry);
void WRITE_DHCPC_FILE(int fh, unsigned char *buf);
int isDhcpProcessExist(unsigned int ifIndex);

int getIPaddrInfo(MIB_CE_ATM_VC_Tp entryp, char *ipaddr, char *netmask, char *gateway);

#if defined(CONFIG_RTL_SMUX_DEV)
int rtk_smuxdev_hw_vlan_filter_init(void);
#ifdef CONFIG_USER_LAN_VLAN_TRANSLATE
int Set_LanPort_Def(int port);
#endif
int Init_LanPort_Def_Intf(int port);
int Deinit_LanPort_Def_Intf(int port);
#endif

#if defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_)
int rtk_layer2bridging_get_lan_router_groupnum(void);
int rtk_layer2bridging_set_lan_router_groupnum(int groupnum);
int rtk_layer2bridging_set_group_firewall_rule(int groupnum, int enable);
int rtk_layer2bridging_set_group_isolation_firewall_rule(int groupnum, int enable);
int rtk_layer2bridging_get_group_by_groupnum(MIB_L2BRIDGE_GROUP_T *pentry, int groupnum, unsigned int *chainid);
int rtk_layer2bridging_get_unused_groupnum();
unsigned int rtk_layer2bridging_get_new_group_instnum();
unsigned int rtk_layer2bridging_get_new_bridge_port_instnum(int groupnum);
unsigned int rtk_layer2bridging_get_new_bridge_vlan_instnum(int groupnum);
unsigned int rtk_layer2bridging_get_new_filter_instnum();
unsigned int rtk_layer2bridging_get_new_marking_instnum();
unsigned int rtk_layer2bridging_get_new_availableinterface_instnum();
int rtk_layer2bridging_get_interface_filter(unsigned int ifdomain, unsigned int ifid, int groupnum, MIB_L2BRIDGE_FILTER_T *pentry, unsigned int *chainid);
int rtk_layer2bridging_get_interface_marking(unsigned int ifdomain, unsigned int ifid, int groupnum, MIB_L2BRIDGE_MARKING_T *pentry, unsigned int *chainid);
void rtk_layer2bridging_init_mib_table();
void rtk_layer2bridging_update_wan_interface_mib_table(int action, unsigned int old_ifIndex, unsigned int new_ifIndex);
int rtk_layer2bridging_update_group_wan_interface(int groupnum);
int rtk_layer2bridging_get_interface_groupnum(unsigned int ifdomain, unsigned int ifid, int *groupnum);
int rtk_layer2bridging_set_interface_groupnum(unsigned int ifdomain, unsigned int ifid, int groupnum, int cwmp_entry_update);
int rtk_layer2bridging_get_availableinterface(struct layer2bridging_availableinterface_info *info, int max_size, unsigned int ifdomain, int groupnum);

int config_port_to_grouping(const char *port, const char *brname);
int setup_interface_grouping(void *pentry, int mode);
int setup_vlan_group(void *pentry, int mode, int logPort);
int reset_port_grouping_by_groupnum(int groupNum);
int  is_ifname_belong_interface_group(char *brname);
int get_grouping_entry_by_groupnum(int groupNum, MIB_L2BRIDGE_GROUP_T *l2br_entry, char *brname, int len);
char* get_grouping_ifname_by_groupnum(int groupNum, char *brname, int len);
int config_interface_grouping(int configMode, void *pentry);
int config_vlan_group(int configMode, void *pentry, int action, int logPort);
int clear_VLAN_grouping_by_port(int logPort);
#endif

#ifdef CONFIG_HYBRID_MODE
void rtk_pon_change_onu_devmode(void);
#endif

#ifdef CONFIG_USER_IGMPPROXY_SUPPORT_CFG_FILE
struct igmp_proxy_config_para {
    unsigned int   	 query_interval;
    unsigned int   	 query_response_interval;
    unsigned int   	 last_member_query_count;
    unsigned int   	 robust;
    unsigned int	 group_leave_delay;
    unsigned int   	 fast_leave;
    unsigned int   	 query_mode;
    unsigned int   	 query_version;
};
#endif

#ifdef CONFIG_USER_RTK_MULTICAST_VID
int rtk_iptv_multicast_vlan_config(int configAll, MIB_CE_ATM_VC_Tp pEntry);
#endif

#ifdef ROUTING
#define MAX_DYNAMIC_ROUTE_INSTNUM	10000
#define LAN_IFINDEX (DUMMY_IFINDEX - 1)	// special case for br0

int route_cfg_modify(MIB_CE_IP_ROUTE_T *, int, int entryID);
int checkRoute(MIB_CE_IP_ROUTE_T, int);
void route_ppp_ifup(unsigned long pppGW, char *ifname);
int delRoutingTable( unsigned int ifindex );
int updateRoutingTable( unsigned int old_id, unsigned int new_id );
void addStaticRoute(void);
void deleteStaticRoute(void);

typedef struct l3_forwarding_entry {
	unsigned int  InstanceNum;
	unsigned char IPVersion; /* 4 or 6 */
	unsigned char Enable;
	unsigned char Type; /* 0:network, 1:host, 2:default */
	char DestIPAddress[48];
	char DestSubnetMask[48];
	char SourceIPAddress[48];
	char SourceSubnetMask[48];
	char GatewayIPAddress[48];
	unsigned int ifIndex;
	int ForwardingMetric;
#if defined(CONFIG_IPV6) && defined(CONFIG_00R0)
	int SourcePrefixLength;
	int DestPrefixLength;
#endif
} L3_FORWARDING_ENTRY_T;

int getDynamicForwardingTotalNum(void);
int getDynamicForwardingEntryByInstNum( unsigned int instnum, L3_FORWARDING_ENTRY_T *pRoute );

int getForwardingEntryByInstNum( unsigned int instnum, MIB_CE_IP_ROUTE_T *p, unsigned int *id );
int queryRouteStatus( MIB_CE_IP_ROUTE_T *p );
#endif

#ifdef CONFIG_E8B
void reset_default_route(void);
int _check_and_update_default_route_entry(const char *ifname, void *entry, int ipv, int on_off);
#endif

int rtk_util_check_iface_ip_exist(char * ifname, int ipver);

#ifdef ROUTING
void check_staticRoute_change(char *ifname);
#endif

#ifdef CONFIG_USER_LANNETINFO
int get_lan_net_info(lanHostInfo_t **ppLANNetInfoData, unsigned int *pCount);
#ifdef CONFIG_USER_LANNETINFO_COLLECT_OFFLINE_DEVINFO
int get_offline_lan_net_info(lanHostInfo_t **ppLANNetInfoData, unsigned int *pCount, char* file, int boot);
#define OFFLINELANNETINFOFILE	"/var/lannetinfo_offline"
#define OFFLINELANNETINFOFILE_SAVE	"/var/config/lannetinfo_offline"
#endif
int sendMessageToLanNetInfo(LANNETINFO_MSG_T *msg);
int readMessageFromLanNetInfo(struct lanNetInfoMsg *qbuf);
#ifdef _PRMT_X_TRUE_LASTSERVEICE
int get_flow_info(char *pFlowInfo);
#endif
int set_lanhost_linkchange_port_status(int portIdx);
#ifdef CONFIG_TELMEX_DEV
int set_lanhost_force_recycle(unsigned int ipAddr);
#endif
int set_lanhost_dhcp_release_mac(char *mac);
int rtk_update_lan_hpc_for_mib(unsigned char *mac);
#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL
int rtk_fc_host_police_control_mib_counter_init(void);
int rtk_lan_host_trap_check(char *mac);
char rtk_lanhost_get_host_index_by_mac(char *mac);
int rtk_host_police_control_mib_counter_get(unsigned char *mac, LAN_HOST_POLICE_CTRL_MIB_TYPE_T type, unsigned long long *result);
int rtk_lanhost_limit_by_ps_check(char *mac, LAN_HOST_POLICE_CTRL_MIB_TYPE_T type);
#endif
#endif

int getInternetWANItfName(char* devname,int len);
unsigned int rtk_wan_speed_limit_get(int direction, unsigned int ifIndex);

struct dhcpOfferedAddrLease {
	u_int8_t chaddr[16];
	u_int32_t yiaddr;       /* network order */
	u_int32_t expires;      /* host order */
	u_int32_t interfaceType;
	u_int8_t hostName[64];
	u_int32_t hostType;
#ifdef GET_VENDOR_CLASS_ID
	u_int8_t vendor_class_id[64];
#endif
	u_int32_t active_time;
#ifdef _PRMT_X_CT_SUPPER_DHCP_LEASE_SC
	int category;
	int isCtcVendor;
	char szVendor[36];
	char szModel[36];
	char szUserClass[36];	//user class id in opt 77
	char szClientID[36];	//client id in opt 61
	char szFQDN[64];
#if defined(CONFIG_CU)
	char szVendorClassID[DHCP_VENDORCLASSID_LEN];
#endif
#endif
};
#ifdef EMBED
int getDhcpClientInfo(char *leaseDB, int leaseCount, unsigned char *macAddress, struct dhcpOfferedAddrLease **pClient);
int getDhcpClientHostName(char *leaseDB, int leaseCount, unsigned char *macAddress, char *hostName);
int getDhcpClientLeasesDB(char **pdb, unsigned long *pdbLen);
int getDhcpClientVendorClass(char *leaseDB, int leaseCount, unsigned char *macAddress, char *vendorClass);
int getOneDhcpClient(char **ppStart, unsigned long *size, char *ip, char *mac, char *liveTime, uint32_t *active_time);
#endif
#ifdef SUPPORT_SET_WANTYPE_FOR_STATIC_ROUTE
int getifindexByWanType(char *ifType, unsigned int *ifIndex);
#endif
int startDnsRelay(void);
#if defined(CONFIG_SECONDARY_IP)
void rtk_fw_set_lan_ip2_rule(void);
#endif
#if defined (CONFIG_IPV6) && defined(CONFIG_SECONDARY_IPV6)
void rtk_fw_set_lan_ipv6_2_rule(void);
#endif
int rtk_create_unix_domain_socket(int server, const char *path, int block);

unsigned int rtk_wan_get_max_wan_mtu(void);
unsigned int rtk_lan_get_max_lan_mtu(void);
void rtk_lan_change_mtu(void);
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
int rtk_lan_host_get_host_by_mac(unsigned char* mac);
#endif
void down_up_lan_interface(void);

int rtk_update_lan_virtual_interface(int logicPort, int action);
int rtk_is_interface_in_bridge(char *ifname, char *brname);
int rtk_check_is_lan_ethernet_interface(char *ifname);

int rtk_wan_check_Droute(int configAll, MIB_CE_ATM_VC_Tp pEntry, int *EntryID);

#ifdef CONFIG_USER_PPPD
#define PATH_IF_UPTIME          "/tmp/pppd-uptime"

int rtk_get_pppd_uptime(char *pppname, unsigned int *uptime);
#endif

#ifdef BLOCK_LAN_MAC_FROM_WAN
int set_lanhost_block_from_wan(int enable);
#endif
#ifdef BLOCK_INVALID_SRCIP_FROM_WAN
int Block_Invalid_SrcIP_From_WAN(int enable);
#endif

#ifdef CONFIG_USER_REFINE_CHECKALIVE_BY_ARP
extern int get_speed_by_portNum(int port, unsigned int *rxRate, unsigned int *txRate);
#endif

int block_br_lan_igmp_mld_rules(void);

int rtk_wan_get_ifindex_by_ifname(char *ifname);

#ifdef CONFIG_USER_IP_QOS
int setupQosRuleMainChain(unsigned int enable);
#endif
void setTmpBlockBridgeFloodPktRule(int act);

#endif

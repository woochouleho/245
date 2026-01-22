#ifndef IPV6_INFO_H
#define IPV6_INFO_H

#include<netinet/in.h>
#include "mib.h"

#define IPV6_BUF_SIZE_256 256
#define IPV6_BUF_SIZE_128 128

#define MAX_RDNSS_NUM 3
#define MAX_DNS_STRING_LEN 128

#ifdef CONFIG_IPV6
extern const char TUN6RD_IF[];
extern const char TUN6IN4_IF[];
extern const char TUN6TO4_IF[];

struct ipv6_route
{
	struct in6_addr		target;
	unsigned int		prefix_len;
	struct in6_addr		source;
	unsigned int		source_prefix_len;
	struct in6_addr		gateway;
	unsigned int		metric;
	unsigned int		flags;
	char				devname[IPV6_BUF_SIZE_128];
};

struct ipv6_ifaddr
{
	int			valid;
	struct in6_addr		addr;
	unsigned int		prefix_len;
	unsigned int		flags;
	unsigned int		scope;
};

typedef struct dnsV6Info {
	unsigned char mode;
	unsigned int wanconn;
	unsigned char nameServer[IPV6_BUF_SIZE_256];
	unsigned char leaseFile[IPV6_BUF_SIZE_128];
	unsigned int dnssl_num;
	unsigned char domainSearch[MAX_DOMAIN_NUM][MAX_DOMAIN_LENGTH];
} DNS_V6_INFO_T, *DNS_V6_INFO_Tp;


typedef struct prefixV6Info {
	unsigned int RNTime;
	unsigned int RBTime;
	unsigned int PLTime;
	unsigned int MLTime;
	unsigned char mode;
	unsigned int wanconn;
	unsigned char prefixIP[IP6_ADDR_LEN];
	unsigned char prefixLen;
	unsigned char leaseFile[IPV6_BUF_SIZE_128];
} PREFIX_V6_INFO_T, *PREFIX_V6_INFO_Tp;

typedef struct ra_v6Info {
	char ifname[IFNAMSIZ];
	unsigned int m_bit;
	unsigned int o_bit;
	unsigned char reomte_gateway[INET6_ADDRSTRLEN];
	unsigned char prefix_1[INET6_ADDRSTRLEN];
	unsigned int  prefix_len_1;
	unsigned int prefix_onlink;
	/* maybe we need to receive multiple preixes in RA message.
	unsigned char prefix_2[INET6_ADDRSTRLEN];
	unsigned int  prefix_len_2;
	*/
	unsigned int mtu;
	unsigned char rdnss[2][INET6_ADDRSTRLEN]; //we support 2 RDNSS 2019/11/20
	unsigned char dnssl[2][128];              //we support 2 DNSSL server 2019/11/20
} RAINFO_V6_T, *RAINFO_V6_Tp;

//Alan Chang 20160727
/************************************************
* Propose: ipv6_linklocal_eui64()
*    Produce Link local IPv6 address
*      e.q: MAC Address 00:E0:4C:86:53:38  ==>
			Linklocal IPv6 Address fe80::2e0:4cff:fe86:5338
* Parameter:
*	unsigned char *d      LinkLocal IPV6 Address   (output)
*     unsigned char *p      MAC Address                 (input)
* Return:
*     None
* Author:
*     Alan
*************************************************/
#define ipv6_linklocal_eui64(d, p) \
  *(d)++ = 0xFE; *(d)++ = 0x80; *(d)++ = 0; *(d)++ = 0; \
  *(d)++ = 0; *(d)++ = 0; *(d)++ = 0; *(d)++ = 0; \
  *(d)++ = ((p)[0]|0x2); *(d)++ = (p)[1]; *(d)++ = (p)[2] ;*(d)++ = 0xff; \
  *(d)++ = 0xfe; *(d)++ = (p)[3]; *(d)++ = (p)[4] ;*(d)++ = (p)[5];

/*******************************************
 *
 * Propose: ipv6_global_eui64()
 *
 * Produce global IPv6 address with EUI-64
 *
 * Ex: MAC Address 00:E0:4C:86:70:04. Prefix:2001:b011:7006:27bc::
 *     --> IPv6 global Addr: 2001:b011:7006:27bc:2e0:4cff:fe86:7004
 *
 * Parameter:
 * 	 unsigned char *d      IPV6 Preix (2001:b011:7006:27bc::)
 *	 unsigned char *p      MAC Address           
 * 
 * Return value will store at d.
 *
 * Return:
 *     None
 * Author:
 *     Rex_Chen
 *********************************************/
#define ipv6_global_eui64(d, p) \
  d += 8; *(d)++ = ((p)[0]|0x2); *(d)++ = (p)[1]; *(d)++ = (p)[2] ;*(d)++ = 0xff; \
  *(d)++ = 0xfe; *(d)++ = (p)[3]; *(d)++ = (p)[4] ;*(d)++ = (p)[5];

extern const char IP6TABLES[];
extern const char FW_IPV6FILTER[];
extern const char FW_IPV6REMOTEACC[];
extern const char ARG_ICMPV6[];
#if defined(DHCPV6_ISC_DHCP_4XX) || defined(CONFIG_USER_RADVD)
extern const char RADVD_CONF[];
extern const char RADVD_CONF_TMP[];
extern const char RADVD_PID[];
#endif
extern const char PROC_IP6FORWARD[];
extern const char PROC_MC6FORWARD[];

#if 0
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
extern const char DHCPD6S_CONF[];
extern const char DHCPD6S_CONF_TMP[];

extern const char DHCPD6S_PID[];
extern const int DHCPD6S_LAN_CONTROL_PORT[];
extern const int DHCPD6S_WLAN_CONTROL_PORT[];
#endif
#endif

//Copy from kernel linux-4.4.x/include/net/ipv6.h
/*
 *	Addr type
 *	
 *	type	-	unicast | multicast
 *	scope	-	local	| site	    | global
 *	v4	-	compat
 *	v4mapped
 *	any
 *	loopback
 */
#define IPV6_ADDR_ANY		0x0000U
#define IPV6_ADDR_UNICAST      	0x0001U	
#define IPV6_ADDR_MULTICAST    	0x0002U	
#define IPV6_ADDR_LOOPBACK	0x0010U
#define IPV6_ADDR_LINKLOCAL	0x0020U
#define IPV6_ADDR_SITELOCAL	0x0040U
#define IPV6_ADDR_COMPATv4	0x0080U
#define IPV6_ADDR_SCOPE_MASK	0x00f0U
#define IPV6_ADDR_MAPPED	0x1000U
#define IPV6_ADDR_RESERVED	0x2000U	/* reserved address space */
/*
 *	Addr scopes
 */
#define IPV6_ADDR_MC_SCOPE(a)	\
	((a)->s6_addr[1] & 0x0f)	/* nonstandard */
#define __IPV6_ADDR_SCOPE_INVALID	-1
#define IPV6_ADDR_SCOPE_NODELOCAL	0x01
#define IPV6_ADDR_SCOPE_LINKLOCAL	0x02
#define IPV6_ADDR_SCOPE_SITELOCAL	0x05
#define IPV6_ADDR_SCOPE_ORGLOCAL	0x08
#define IPV6_ADDR_SCOPE_GLOBAL		0x0e
int ipv6_addr_type(const struct in6_addr *addr);
int ipv6_addr_needs_scope_id(int type);
//Copy from kernel end

#ifdef DHCPV6_ISC_DHCP_4XX
int is_exist_pd_info(MIB_CE_ATM_VC_Tp pEntry);
int get_dhcpv6_prefixv6_info(PREFIX_V6_INFO_Tp pPrefixInfo);
int get_dhcpv6_dnsv6_info(DNS_V6_INFO_Tp pDnsV6Info);
#endif
int is_InternetWan_IPv6_single_stack(void);

int ipv6_prefix_equal(const struct in6_addr *addr1,    const struct in6_addr *addr2,  unsigned int prefixlen);
int compare_ipv6_addr(struct in6_addr *ipA, struct in6_addr *ipB);
int prefixtoIp6(void *prefix, int plen, void *ip6, int mode);
int ip6toPrefix(void *ip6, int plen, void *prefix);
int mac_meui64(char *src, char *dst);
int getifip6(const char *ifname, unsigned int addr_scope, struct ipv6_ifaddr *addr_lst, int num);
int get_network_dnsv6(char *dns1, char *dns2);
int is_rule_priority_exist_in_ip6_rule(unsigned int pref_index);
int get_wan_interface_by_pd(struct in6_addr *prefix_ip, char *wan_inf);
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
int startBR_IP_for_V6(char *br_inf);
#endif
#ifdef CONFIG_IPV6
int stop6rd_6in4_6to4(MIB_CE_ATM_VC_Tp pEntry);
int setupIPv6DNS(MIB_CE_ATM_VC_Tp pEntry);
int startIP_for_V6(MIB_CE_ATM_VC_Tp pEntry);
int startPPP_for_V6(MIB_CE_ATM_VC_Tp pEntry);
#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
int startIPv6AutoDetect(MIB_CE_ATM_VC_Tp pEntry);
#endif
#if defined(CONFIG_RTK_IPV6_LOGO_TEST)
void checkLanLinklocalIPv6Address();
#endif
#endif
#ifdef CONFIG_USER_RTK_RAMONITOR 
int del_wan_rainfo_file(MIB_CE_ATM_VC_Tp pEntry);
#endif
#ifdef CONFIG_IP6_NF_TARGET_NPT
int del_wan_npt_pd_route_rule(MIB_CE_ATM_VC_Tp pEntry);
#endif
int del_ipv6_global_lan_ip_by_wan(const char *lan_infname, MIB_CE_ATM_VC_Tp pEntry);
int set_other_delegated_default_wanconn(int old_ifIndex);
int stopIP_PPP_for_V6(MIB_CE_ATM_VC_Tp pEntry);
int set_ipv6_all_lan_phy_inf_forwarding(int set);
int config_ipv6_kernel_flag_and_static_lla_for_inf(char *infname);
#ifndef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
int setupIPV6Filter(void);
#else
int setEachIPv6FilterRuleMixed(MIB_CE_V6_IP_PORT_FILTER_Tp pIpEntry, PREFIX_V6_INFO_Tp pDLGInfo);
int setupIPV6FilterMixed(void);
#endif
int pre_setupIPV6Filter(void);
int post_setupIPV6Filter(void);
int restart_IPV6Filter(void);
int get_dnsv6_info(DNS_V6_INFO_Tp dnsV6Info); // get dns info of LAN server
unsigned int get_dnsv6_wan_delegated_mode_ifindex(void);
unsigned int get_radvd_rdnss_wan_delegated_mode_ifindex(void);

int get_dnsv6_info_by_wan(DNS_V6_INFO_Tp dnsV6Info, unsigned int ifindex); // get dns info of wan
int get_prefixv6_info(PREFIX_V6_INFO_Tp prefixInfo);
int get_prefixv6_info_by_wan(PREFIX_V6_INFO_Tp prefixInfo, int ifindex);
unsigned int get_pd_wan_delegated_mode_ifindex(void);
void rtk_ipv6_get_prefix(struct  in6_addr *ip6Addr);
void rtk_ipv6_get_prefix_len(unsigned int *prefix_len);
#ifdef ROUTING
int route_v6_cfg_modify(MIB_CE_IPV6_ROUTE_T *pRoute, int del);
void addStaticV6Route(void);
void deleteV6StaticRoute(void);
void check_IPv6_staticRoute_change(char *ifname);
#ifdef CONFIG_TELMEX_DEV
int policy_route_modify(MIB_CE_IPV6_ROUTE_T *pRoute, MIB_CE_ATM_VC_Tp pvcEntry, int del);
void set_policy_route_for_static_rt_rule(MIB_CE_ATM_VC_Tp pEntry);
void sync_v6_route_to_policy_table(MIB_CE_IPV6_ROUTE_Tp prtEntry);
#endif
#endif

#ifdef IP_ACL
void filter_set_v6_acl(int enable);
int restart_v6_acl(void);
#endif
#ifdef CONFIG_USER_RADVD
int kill_and_do_radvd(void);
void clean_radvd(void);
void restartRadvd(void);
void init_radvd_conf_mib(void);   // Added by Mason Yu for p2r_test
int setup_radvd_conf_by_lan_ifname(char* lanIfname, DNS_V6_INFO_Tp pDnsV6Info, PREFIX_V6_INFO_Tp pPrefixInfo);
int get_radvd_prefixv6_info(PREFIX_V6_INFO_Tp prefixInfo);
int get_radvd_dnsv6_info(DNS_V6_INFO_Tp pDnsV6Info); // get dns info for RADVD
int setup_radvd_conf_with_single_br(void);
#endif
int close_ipv6_kernel_flag_for_bind_inf(char *infname);
int delete_binding_lan_service_by_wan(MIB_CE_ATM_VC_Tp pEntry);
int delete_wide_dhcp6s_by_ifname(char *lan_inf);

void delOrgLanLinklocalIPv6Address(void);
void delWanGlobalIPv6Address(MIB_CE_ATM_VC_Tp pEntry);
void setLanLinkLocalIPv6Address(void);
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
void setLanIPv6Address4Readylogo(void);
#endif
void setLinklocalIPv6Address(char* ifname);
//int start_dhcpv6_by_dns_prefix(DNS_V6_INFO_Tp pDnsV6Info, PREFIX_V6_INFO_Tp pPrefixInfo);
void restartStaticPDRoute(void);
void restartLanV6Server(void);
void cleanLanV6Server(void);
int startDhcpv6Relay(void);
int restart_default_dhcpv6_server(void);
int get_if_link_local_addr6(const char *ifname, struct sockaddr_in6 *addr6);
int sendRouterSolicit(char *ifname);
void setup_disable_ipv6(const char *itf, int disable);
int set_lan_ipv6_global_address(MIB_CE_ATM_VC_Tp pEntry, PREFIX_V6_INFO_Tp prefixInfo, char *infname, int prefix_route,int set);
void save_lan_global_ip_by_lease_file(char *lease_file, char *lan_infname);
int rtk_ipv6_prefix_to_mask(unsigned int prefix, struct in6_addr *mask);
int rtk_ipv6_ip_address_range(struct in6_addr*ip6, int plen,	struct in6_addr* in6_ip_start, struct in6_addr* in6_ip_end);
int rtk_ipv6_ip_address_range_to_mask(int align_mask, struct in6_addr* in6_ip_start, struct in6_addr* in6_ip_end, struct in6_addr* ip6_mask);

//#if defined(DHCPV6_ISC_DHCP_4XX) && defined(CONFIG_USER_RADVD)
//void restart_default_LanV6Server_by_lease(char *leaseFile);
//#endif


#ifdef WLAN_SUPPORT
int is_wlan_inf_binding_v6_route_wan(char *infname);
#endif
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
int add_oif_lla_route_by_lanif(char *lan_ifname);
int del_bind_lan_oif_policy_route(char *lan_inf);
int del_all_bind_lan_oif_policy_route(void);
#if 0
int setup_radvd_conf_static(void);
int setup_radvd_conf_multiple(char* lanIfname, DNS_V6_INFO_Tp dnsV6Info, PREFIX_V6_INFO_Tp prefixInfo);
int recursive_setup_ipv6_binding_lan_services_with_single_br(void);
#ifdef CONFIG_USER_WIDE_DHCPV6
int setup_dhcp6s_conf_content(unsigned int ifIndex, char* lanIfname, PREFIX_V6_INFO_Tp pPrefixInfo);
int setup_dhcp6s_conf_pool(unsigned int ifIndex, PREFIX_V6_INFO_Tp pPrefixInfo);
int stop_multi_wide_Dhcp6sService(void);
int start_wide_Dhcp6sService_by_wan(MIB_CE_ATM_VC_Tp pEntry);
int restart_multi_wide_Dhcp6sService(int perform_directly);
int setup_wide_dhcp6s_conf_header(DNS_V6_INFO_Tp pDnsV6Info, PREFIX_V6_INFO_Tp pPrefixInfo);
int setup_binding_lan_wide_dhcp6s_config_for_wan(MIB_CE_ATM_VC_Tp pEntry, DNS_V6_INFO_Tp dnsV6Info_p, PREFIX_V6_INFO_Tp prefixInfo_p);
#ifdef CONFIG_DHCPV6_SERVER_PREFIX_DELEGATION
int setup_wide_dhcp6s_conf_for_each_lan(unsigned int ifIndex, char* lanIfname, PREFIX_V6_INFO_Tp pPrefixInfo, DNS_V6_INFO_Tp pDnsV6Info, int portNo);
#endif
#endif
#endif 
#endif

#if defined(CONFIG_USER_RTK_RAMONITOR)
int get_ra_info(char *rainfo_file, RAINFO_V6_Tp ra_v6Info);
int get_ra_mtu_by_wan_ifindex(int ifindex, int *mtu);
#endif

void get_ipv6_default_gw_inf(char *wan_inf_buf);
void get_ipv6_default_gw_inf_first(char *wan_inf_buf);
void get_ipv6_default_gw(char *dgw);
void del_static_pd_route(char *prefix_str, char *lan_infname);
int add_static_pd_route_for_default_waninf(char *prefix_str, char *lan_infname);
#ifdef CONFIG_IP6_NF_TARGET_NPT
int set_v6npt_config(char *infname);
#endif
#ifdef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
void updateIPV6FilterByPD(PREFIX_V6_INFO_Tp pDLGInfo);
#endif
#endif

#ifdef DUAL_STACK_LITE
int query_aftr(char *aftr,  char *aftr_dst, char *aftr_addr_str);
#endif

#ifdef CONFIG_USER_MAP_E
void mape_mutex_init(void);
void mape_recover_firewalls();
void mape_flush_firewalls();
#endif

#ifdef CONFIG_IPV6
int rtk_tun46_generate_static_mode_parameter(int tunType, char *ifName);
int rtk_tun46_generate_dhcp_mode_parameter(int tunType, char *ifName, char *extra_info);
int rtk_tun46_setup_tunnel(int tunType, char *ifName);
int rtk_tun46_delete_tunnel(int tunType, char *ifName);
#endif

#ifdef CONFIG_USER_NDPPD
#define NDPPD_PID_PATH "/var/run/ndppd.pid"
#define NDPPD_CONF_PATH "/var/ndppd.conf"
#define NDPPD_CMD "/bin/ndppd"
int ndp_proxy_setup_conf(void);
void ndp_proxy_clean(void);
void ndp_proxy_start(void);
#endif//endif CONFIG_USER_NDPPD

#ifdef CONFIG_RTK_IPV6_LOGO_TEST
/* [RFC 7084] CE LOGO Test CERouter 2.7.6 : Unknown Prefix 
 *  policy route mark have been defined in sockmark_define.h
 * 0x0007c000 will not match any eth port, only used by CE_Router
 */
#define MARK_CE_ROUTER "0x0007c000/0x0007c000"
#define CE_ROUTER_TABLE_ID "1350"
#endif
int rtk_ipv6_send_neigh_solicit(char *ifname, struct in6_addr *in6_dst);
int rtk_ipv6_send_ping_request(char *ifname, struct in6_addr *in6_dst);

void setup_delegated_default_wanconn(MIB_CE_ATM_VC_Tp mibentry_p);
void clear_delegated_default_wanconn(MIB_CE_ATM_VC_Tp mibentry_p);

int set_ipv6_default_policy_route(MIB_CE_ATM_VC_Tp pEntry, int set);
int set_ipv6_policy_route(MIB_CE_ATM_VC_Tp pEntry, int set);
int set_static_policy_route(PREFIX_V6_INFO_Tp prefixInf, int set, const char* brname);
int set_ipv6_lan_policy_route(char *lan_ifname, struct in6_addr *ip6addr, unsigned int ip6addr_len, int set);
//lan setting page apply will trigger br0 resetart (disable_ipv6 1->0),which will delete all route entry related with br0
//So, we need add policy route back in dhcpv6/radvd startup.
int set_lanif_PD_policy_route_for_all_wan(const char* brname);
void setup_ipv6_br_interface(const char* brname);

#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
typedef enum lan_vlan_inf_event
{
	LAN_VLAN_INTERFACE_DHCPV6S_ADD,
	LAN_VLAN_INTERFACE_DHCPV6S_CMD,
	LAN_VLAN_INTERFACE_RADVD_ADD,
	LAN_VLAN_INTERFACE_DEL
}LAN_VLAN_INF_EVENT;

struct lan_vlan_inf_msg{
	LAN_VLAN_INF_EVENT lan_vlan_inf_event;
	DNS_V6_INFO_T DnsV6Info;
	PREFIX_V6_INFO_T PrefixInfo;
	char inf_cmd[100];
};
int lan_vlan_mapping_inf_handle(struct lan_vlan_inf_msg *msg_header,MIB_CE_ATM_VC_Tp pEntry);

//in case£¬ vlan mapping lan interface(MIB_PORT_BINDING_TBL) have been deleted, the old rule will be change to
//15000:	from all oif eth0.2.700 [detached] lookup 256
//and we need clear all the "detached" rule
int del_vlan_bind_lan_detached_policy_route();

#endif
#endif

#if  defined(CONFIG_USER_RTK_MULTICAST_VID) && defined(CONFIG_RTK_SKB_MARK2)
//RA packet belong to multicast packet, we'd better not make RA packet passed for multicast rule,otherwise, wan will have two global addr
//this rule will be deleted in stopConnection
int rtk_block_multicast_ra();
#endif
#if (defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_) ) && defined(_SUPPORT_L2BRIDGING_DHCP_SERVER_)
void setup_interface_group_br_policy_route(PREFIX_V6_INFO_Tp prefix_br, char* strtbl, int set, int nptv6);
int setup_radvd_conf_with_interface_group_br(void);
#endif
int rtk_ipv6_get_route(struct ipv6_route** route_list);

#endif

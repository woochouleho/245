#ifndef SUBR_DHCPV6_H
#define SUBR_DHCPV6_H

#include "mib.h"
#include "ipv6_info.h"

#ifdef DHCPV6_ISC_DHCP_4XX
/* DHCPv6 Option codes: */
#define D6O_CLIENTID				1 /* RFC3315 */
#define D6O_SERVERID				2
#define D6O_IA_NA				3
#define D6O_IA_TA				4
#define D6O_IAADDR				5
#define D6O_ORO					6
#define D6O_PREFERENCE				7
#define D6O_ELAPSED_TIME			8
#define D6O_RELAY_MSG				9
/* Option code 10 unassigned. */
#define D6O_AUTH				11
#define D6O_UNICAST				12
#define D6O_STATUS_CODE				13
#define D6O_RAPID_COMMIT			14
#define D6O_USER_CLASS				15
#define D6O_VENDOR_CLASS			16
#define D6O_VENDOR_OPTS				17
#define D6O_INTERFACE_ID			18
#define D6O_RECONF_MSG				19
#define D6O_RECONF_ACCEPT			20
#define D6O_SIP_SERVERS_DNS			21 /* RFC3319 */
#define D6O_SIP_SERVERS_ADDR			22 /* RFC3319 */
#define D6O_NAME_SERVERS			23 /* RFC3646 */
#define D6O_DOMAIN_SEARCH			24 /* RFC3646 */
#define D6O_IA_PD				25 /* RFC3633 */
#define D6O_IAPREFIX				26 /* RFC3633 */
#define D6O_NIS_SERVERS				27 /* RFC3898 */
#define D6O_NISP_SERVERS			28 /* RFC3898 */
#define D6O_NIS_DOMAIN_NAME			29 /* RFC3898 */
#define D6O_NISP_DOMAIN_NAME			30 /* RFC3898 */
#define D6O_SNTP_SERVERS			31 /* RFC4075 */
#define D6O_INFORMATION_REFRESH_TIME		32 /* RFC4242 */
#define D6O_BCMCS_SERVER_D			33 /* RFC4280 */
#define D6O_BCMCS_SERVER_A			34 /* RFC4280 */
/* 35 is unassigned */
#define D6O_GEOCONF_CIVIC			36 /* RFC4776 */
#define D6O_REMOTE_ID				37 /* RFC4649 */
#define D6O_SUBSCRIBER_ID			38 /* RFC4580 */
#define D6O_CLIENT_FQDN				39 /* RFC4704 */
#define D6O_PANA_AGENT				40 /* paa-option */
#define D6O_NEW_POSIX_TIMEZONE			41 /* RFC4833 */
#define D6O_NEW_TZDB_TIMEZONE			42 /* RFC4833 */
#define D6O_ERO					43 /* RFC4994 */
#define D6O_LQ_QUERY				44 /* RFC5007 */
#define D6O_CLIENT_DATA				45 /* RFC5007 */
#define D6O_CLT_TIME				46 /* RFC5007 */
#define D6O_LQ_RELAY_DATA			47 /* RFC5007 */
#define D6O_LQ_CLIENT_LINK			48 /* RFC5007 */
#define D6O_MIP6_HNIDF				49 /* RFC6610 */
#define D6O_MIP6_VDINF				50 /* RFC6610 */
#define D6O_V6_LOST				51 /* RFC5223 */
#define D6O_CAPWAP_AC_V6			52 /* RFC5417 */
#define D6O_RELAY_ID				53 /* RFC5460 */
#define D6O_IPV6_ADDRESS_MOS			54 /* RFC5678 */
#define D6O_IPV6_FQDN_MOS			55 /* RFC5678 */
#define D6O_NTP_SERVER				56 /* RFC5908 */
#define D6O_V6_ACCESS_DOMAIN			57 /* RFC5986 */
#define D6O_SIP_UA_CS_LIST			58 /* RFC6011 */
#define D6O_BOOTFILE_URL			59 /* RFC5970 */
#define D6O_BOOTFILE_PARAM			60 /* RFC5970 */
#define D6O_CLIENT_ARCH_TYPE			61 /* RFC5970 */
#define D6O_NII					62 /* RFC5970 */
#define D6O_GEOLOCATION				63 /* RFC6225 */
#define D6O_AFTR_NAME				64 /* RFC6334 */
#define D6O_ERP_LOCAL_DOMAIN_NAME		65 /* RFC6440 */
#define D6O_RSOO				66 /* RFC6422 */
#define D6O_PD_EXCLUDE				67 /* RFC6603 */
#define D6O_VSS					68 /* RFC6607 */
#define D6O_MIP6_IDINF				69 /* RFC6610 */
#define D6O_MIP6_UDINF				70 /* RFC6610 */
#define D6O_MIP6_HNP				71 /* RFC6610 */
#define D6O_MIP6_HAA				72 /* RFC6610 */
#define D6O_MIP6_HAF				73 /* RFC6610 */
#define D6O_RDNSS_SELECTION			74 /* RFC6731 */
#define D6O_KRB_PRINCIPAL_NAME			75 /* RFC6784 */
#define D6O_KRB_REALM_NAME			76 /* RFC6784 */
#define D6O_KRB_DEFAULT_REALM_NAME		77 /* RFC6784 */
#define D6O_KRB_KDC				78 /* RFC6784 */
#define D6O_CLIENT_LINKLAYER_ADDR		79 /* RFC6939 */
#define D6O_LINK_ADDRESS			80 /* RFC6977 */
#define D6O_RADIUS				81 /* RFC7037 */
#define D6O_SOL_MAX_RT				82 /* RFC7083 */
#define D6O_INF_MAX_RT				83 /* RFC7083 */
#define D6O_ADDRSEL				84 /* RFC7078 */
#define D6O_ADDRSEL_TABLE			85 /* RFC7078 */
#define D6O_V6_PCP_SERVER			86 /* RFC7291 */
#define D6O_DHCPV4_MSG				87 /* RFC7341 */
#define D6O_DHCP4_O_DHCP6_SERVER		88 /* RFC7341 */
#define D6O_S46_RULE         	89 /* RFC7598 */
#define D6O_S46_BR           	90 /* RFC7598 */
#define D6O_S46_DMR          	91 /* RFC7598 */
#define D6O_S46_V4V6BIND        92 /* RFC7598 */
#define D6O_S46_PORTPARAMS   	93 /* RFC7598 */
#define D6O_S46_CONT_MAPE       94 /* RFC7598 */
#define D6O_S46_CONT_MAPT       95 /* RFC7598 */
#define D6O_S46_CONT_LW         96 /* RFC7598 */
/* not yet assigned but next free value */
#define D6O_RELAY_SOURCE_PORT			135 /* I-D */

typedef struct dhcpv6_option_tbl{
	unsigned int op_code;
	char *conf_name;
} DHCPv6_OPT_T;

extern DHCPv6_OPT_T config_tbl[];
DHCPv6_OPT_T* getDHCPv6ConfigbyOptID(const unsigned int opt_id);
DHCPv6_OPT_T* getDHCPv6ConfigbyOptName(const char *op_name);
int getLeasesInfoByOptName(const char *fname, const int opt_no, char *buff, unsigned int buff_len);
int getDHCPv6ConfigTableSize(void);

struct ipv6_lease {
	char iaaddr[INET6_ADDRSTRLEN];
	char duid[80];
	char active;
	time_t epoch;
};
int rtk_get_dhcpv6_lease_info(struct ipv6_lease *active_leases, int max_client_num, int mode,int *real_client_num);
#endif

struct ipv6_addr_str_array {
	char ipv6_addr_str[INET6_ADDRSTRLEN];
};

#if (defined(CONFIG_USER_MAP_E) || defined(CONFIG_USER_MAP_T))
/* OPTION_S46_RULE (89) */
typedef struct S46_RULE_s{
	int option_code;
	int option_len;
	char ea_flag;
	int ea_len;
	int v4_prefix_len;
	struct in_addr map_addr;
	struct in_addr map_addr_subnet;
	int v6_prefix_len;
	struct in6_addr map_addr6;
}S46_RULE;
#endif

#if (defined(CONFIG_USER_MAP_E) || defined(CONFIG_USER_LW4O6))
/* OPTION_S46_BR (90) */
typedef struct S46_BR_s{
	int option_code;
	int option_len;
	struct in6_addr mape_br_addr;
	struct in6_addr lw4o6_aftr_addr;
}S46_BR;
#endif

#ifdef CONFIG_USER_MAP_T
/* OPTION_S46_DMR (91) */
typedef struct S46_DMR_s{
	int option_code;
	int option_len;
	int dmr_prefix_len;
	struct in6_addr mapt_dmr_prefix;
}S46_DMR;
#endif

#ifdef CONFIG_USER_LW4O6
/* OPTION_S46_V4V6BIND (92) */
typedef struct S46_V4V6BIND_s{
	int option_code;
	int option_len;
	struct in_addr lw4o6_v4_addr;
	int prefix6_len;
	struct in6_addr lw4o6_v6_prefix;
}S46_V4V6BIND;
#endif

#if (defined(CONFIG_USER_MAP_E) || defined(CONFIG_USER_MAP_T) || defined(CONFIG_USER_LW4O6))
/* OPTION_S46_PORTPARAMS (93) */
typedef struct S46_PORTPARAMS_s{
	int option_code;
	int option_len;
	int psid_offset;
	int psid_len;
	int psid_val;
}S46_PORTPARAMS;
#endif

typedef struct s46_dhcp_info{
#if (defined(CONFIG_USER_MAP_E) || defined(CONFIG_USER_MAP_T))
	S46_RULE s46_rule;
#endif
#if (defined(CONFIG_USER_MAP_E) || defined(CONFIG_USER_LW4O6))
	S46_BR s46_br;
#endif
#ifdef CONFIG_USER_MAP_T
	S46_DMR	s46_dmr;
#endif
#ifdef CONFIG_USER_LW4O6
	S46_V4V6BIND s46_v4v6bind;
#endif
#if (defined(CONFIG_USER_MAP_E) || defined(CONFIG_USER_MAP_T) || defined(CONFIG_USER_LW4O6))
	S46_PORTPARAMS s46_portparams;
#endif
}S46_DHCP_INFO_T, *S46_DHCP_INFO_Tp;

typedef struct delegationInfo {
	unsigned int RNTime;
	unsigned int RBTime;
	unsigned int PLTime;
	unsigned int MLTime;
	unsigned char prefixLen;
	unsigned char prefixIP[IP6_ADDR_LEN];
	unsigned char nameServer[256];
	unsigned char wanIfname[16];
	unsigned int dnssl_num;
	unsigned char domainSearch[MAX_DOMAIN_NUM][MAX_DOMAIN_LENGTH];
	unsigned char dslite_aftr[256];
#if (defined(CONFIG_USER_MAP_E) || defined(CONFIG_USER_MAP_T)|| defined(CONFIG_USER_LW4O6))
	unsigned char s46_string[256]; //used to parse mape/mapt/lw4o6 option string (hex)
#endif
#if (defined(CONFIG_USER_MAP_E) || defined(CONFIG_USER_MAP_T))
	S46_DHCP_INFO_T map_info;
#endif
#ifdef CONFIG_USER_LW4O6
	S46_DHCP_INFO_T lw4o6_info;
#endif
} DLG_INFO_T, *DLG_INFO_Tp;

#ifdef CONFIG_GPON_FEATURE
typedef struct IANA_Info {
	int	RNTime;
	int	RBTime;
	int	PLTime;
	int MLTime;
	unsigned char ia_na[IP6_ADDR_LEN];
	unsigned char nameServer[256];
	unsigned char wanIfname[16];
} IANA_INFO_T, *IANA_INFO_Tp;
#endif // CONFIG_GPON_FEATURE

#ifdef CONFIG_IPV6
extern const char DHCP6S_LAN_PD_ROUTE_PATH[];
#endif
#if defined(DHCPV6_ISC_DHCP_4XX) || defined (CONFIG_IPV6)
//extern const char DHCPDV6_CONF_AUTO[];
extern const char DHCPDV6_CONF[];
extern const char DHCPDV6_LEASES[];
extern const char DHCPDV6_STATIC_LEASES[];
extern const char DHCPDV6[];
extern const char DHCREALYV6[];
extern const char DHCPSERVER6PID[];
extern const char DHCPRELAY6PID[];
extern const char DHCPCV6SCRIPT[];
extern const char DHCPCV6[];
extern const char PATH_DHCLIENT6_CONF[];
extern const char PATH_DHCLIENT6_LEASE[];
extern const char PATH_DHCLIENT6_PID[];
extern const char DHCPCV6STR[];

void make_dhcpcv6_conf(MIB_CE_ATM_VC_Tp pEntry);
int do_dhcpc6(MIB_CE_ATM_VC_Tp pEntry, int mode);
int del_dhcpc6(MIB_CE_ATM_VC_Tp pEntry);
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
int do_dhcpc6_br(char *br_ifname, int mode);
int del_dhcpc6_br(char *br_ifname);
#endif
int start_dhcpv6(int enable);
void clean_dhcpd6(void);
int setup_dhcpdv6_conf_manual(void);
#ifdef DHCPV6_ISC_DHCP_4XX
char *rtk_quotify_buf(const unsigned char *s, unsigned len);
int rtk_form_dhcpv6_duid(unsigned char *duid_str, int duid_type, char *macaddr);
int rtk_write_duid_to_isc_dhcp_lease_file(char *lease_file_path, char *duid_quotify_str);
int rtk_do_isc_dhcpc6_lease_with_duid_ll(MIB_CE_ATM_VC_Tp pEntry, char *ifname);
int rtk_get_duid_ll_for_isc_dhcp_lease_file(MIB_CE_ATM_VC_Tp pEntry, char *ifname, char *buf, int buf_len);
#if defined(CONFIG_USER_RTK_DHCPV6_SERVER_PD_SUPPORT) || defined(YUEME_DHCPV6_SERVER_PD_SUPPORT)
int calculate_prefix6_range(PREFIX_V6_INFO_Tp PrefixInfo, struct ipv6_addr_str_array *prefix6_subnet_start_str, struct ipv6_addr_str_array *prefix6_subnet_end_str, int*lan_pd_len);
#endif
#endif
#endif

#ifdef CONFIG_IPV6
int rtk_del_dhcp6s_pd_policy_route_and_file(MIB_CE_ATM_VC_Tp pEntry);
int rtk_dhcp6s_del_lan_pd_policy_route(char *lan_ifname, struct in6_addr *prefix_ip, unsigned int prefix_len, struct in6_addr *nexthop_addr);
int rtk_dhcp6s_setup_lan_pd_policy_route(char *lan_ifname, struct in6_addr *prefix_ip, unsigned int prefix_len, struct in6_addr *nexthop_addr);
int rtk_set_dhcp6s_lan_pd_policy_route_info(char *ifname, char *table_id);
#endif

#endif

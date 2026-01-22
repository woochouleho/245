#ifndef INCLUDE_FC_H
#define INCLUDE_FC_H

#include "mib.h"
#include "options.h"

#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#if defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_rate.h>
#include <common/util/rt_util.h>
//#include "common/rt_type.h"
#include "common/error.h"
#include "rt_acl_ext.h"

#define ACL_DEFAULT_IDX	0xFFFFFFFF
#define ACL_LAN_PORT_MASK ((1<<SW_LAN_PORT_NUM)-1)

#ifdef CONFIG_RTL_SMUX_DEV
enum acl_weight{
	WEIGHT_LOW		= 1,		//lowest priority
	WEIGHT_QOS_LOW	= 200,
	WEIGHT_NORMAL	= 250,
	WEIGHT_HW_VLAN_FILTER_LOW	= 300,
	WEIGHT_HW_VLAN_FILTER_MED	= 400,
	WEIGHT_HW_VLAN_FILTER_HIGH	= 500,
	WEIGHT_QOS_HIGH	= WEIGHT_HW_VLAN_FILTER_HIGH + 50,
	WEIGHT_HIGH		= 65535,	//highest priority
};
#else
enum acl_weight{
	WEIGHT_LOW		= 1,		//lowest priority
	WEIGHT_QOS_LOW  = 200,
	WEIGHT_NORMAL	= 300,
	WEIGHT_QOS_HIGH	= 10000,
	WEIGHT_HIGH		= 65535,	//highest priority
};
#endif

/*** set acl pattern api ***/
int rtk_acl_entry_clear(rt_acl_filterAndQos_t *entry);
int rtk_acl_entry_show(rt_acl_filterAndQos_t *entry);
int rtk_acl_set_acl_weight(rt_acl_filterAndQos_t *entry, unsigned int weight);
int rtk_acl_set_ingress_port_mask(rt_acl_filterAndQos_t *entry, unsigned int ingress_port_mask);
int rtk_acl_set_l4_dport(rt_acl_filterAndQos_t *entry, unsigned int dport_start, unsigned int dport_end);
int rtk_acl_set_action(rt_acl_filterAndQos_t *entry, unsigned int action);

/*** apply rtk acl api ***/
int rtk_acl_add(rt_acl_filterAndQos_t *entry, int mib_id);
int rtk_acl_del(int mib_id);

int RTK_FC_URL_Filter_Set(int dport);
int RTK_FC_URL_Filter_Flush(void);

typedef enum rtk_rate_application_type_e
{
	RTK_APP_TYPE_IP_QOS_RATE_SHAPING,
	RTK_APP_TYPE_DS_TCP_SYN_RATE_LIMIT,
	RTK_APP_TYPE_DS_UDP_ATTACK_RATE_LIMIT,
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	RTK_APP_TYPE_CMCC_QOS_RATE_LIMIT,
#endif
	RTK_APP_TYPE_LANHOST_US_RATE_LIMIT_BY_FC,
	RTK_APP_TYPE_LANHOST_DS_RATE_LIMIT_BY_FC,
#ifdef CONFIG_USER_RT_ACL_SYN_FLOOD_PROOF_SUPPORT
	RTK_APP_TYPE_DDOS_TCP_SYN_RATE_LIMIT,
#endif
	RTK_APP_TYPE_ACL_LIMIT_BANDWIDTH,
	RTK_APP_TYPE_MAX
} rtk_rate_application_type_t;

int rtk_qos_share_meter_set(rtk_rate_application_type_t	application_type, unsigned int index, unsigned int rate);
int rtk_qos_share_meter_get(rtk_rate_application_type_t application_type, unsigned int index, unsigned int rate);
int rtk_qos_share_meter_flush(rtk_rate_application_type_t application_type);
int rtk_qos_share_meter_delete(rtk_rate_application_type_t application_type, unsigned int index);
int rtk_qos_share_meter_update_rate(rtk_rate_application_type_t application_type, unsigned int index, unsigned int rate);


struct rtk_qos_share_meter_entry {
	char	alias_name_prefix[512];
	rtk_rate_application_type_t	application_type;
	rt_rate_meter_type_t		meter_type;
};

int rtk_fc_loop_detection_acl_set(void);
int rtk_fc_loop_detection_acl_flush(void);

#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
int rtk_fc_qos_ingress_control_packet_protection_config(void);
int rtk_fc_qos_egress_control_packet_protection_config(void);
int rtk_fc_itms_qos_ingress_control_packet_protection_config(void);
int rtk_fc_itms_qos_egress_control_packet_protection_config(void);
int rtk_fc_qos_ingress_control_packet_protection_flush(void);
int rtk_fc_qos_egress_control_packet_protection_flush(void);
int rtk_fc_qos_ingress_control_packet_protection_set(void);
int rtk_fc_qos_egress_control_packet_protection_set(void);
int rtk_fc_itms_qos_ingress_control_packet_protection_flush(void);
int rtk_fc_itms_qos_egress_control_packet_protection_flush(void);
int rtk_fc_itms_qos_ingress_control_packet_protection_set(void);
int rtk_fc_itms_qos_egress_control_packet_protection_set(void);
#endif

#endif
int rtk_fc_get_host_policer_by_mac(unsigned char *mac);
int rtk_fc_get_empty_host_policer_by_id(int hostIndex);
#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL
int rtk_fc_lanhost_rate_limit_us_trap_acl_flush(void);
int rtk_fc_lanhost_rate_limit_ds_trap_acl_flush(void);
int rtk_fc_lanhost_rate_limit_us_trap_acl_set(unsigned char *mac);
int rtk_fc_lanhost_rate_limit_ds_trap_acl_set(unsigned char *mac);
int rtk_fc_lanhost_rate_limit_us_trap_acl_setup(void);
int rtk_fc_lanhost_rate_limit_ds_trap_acl_setup(void);
void rtk_fc_lanhost_rate_limit_clear(void);
void rtk_fc_lanhost_rate_limit_apply(void);
#endif

int rtk_fc_ds_tcp_syn_rate_limit_set( void );
int rtk_fc_ds_udp_attack_rate_limit_flush( void );
int rtk_fc_ds_udp_attack_rate_limit_set( void );
int rtk_fc_ds_udp_attack_rate_limit_config( void );
int rtk_fc_ddos_smurf_attack_protect( void );

int rtk_fc_ctrl_init(void);
int rtk_fc_ctrl_init2(void);
int do_rtk_fc_flow_flush(void);
int rtk_fc_flow_flush2(int s);
int rtk_fc_flow_flush(void);

int rtk_fc_stp_setup(void);
int rtk_fc_stp_clean(void);
int rtk_fc_stp_sync_port_stat(void);
int rtk_fc_stp_check_port_stat(int portid, int state);

int rtk_fc_total_upstream_bandwidth_set(int wan_port, unsigned int bandwidth);
int rtk_fc_total_downstream_bandwidth_set(int wan_port, unsigned int bandwidth);
int rtk_fc_flow_delete(int ipv4, const char *proto, 
						const char *sip, const unsigned short sport, 
						const char *dip, const unsigned short dport);

int rtk_fc_multicast_flow_add(unsigned int group);
int rtk_fc_multicast_flow_delete(unsigned int group);
#ifdef CONFIG_IPV6
int rtk_fc_ipv6_multicast_flow_add(unsigned int* group);
int rtk_fc_ipv6_multicast_flow_delete(unsigned int* group);
#endif
int rtk_fc_multicast_ignoregroup_trap_rule(int config, int *pindex, const char *start_group, const char *end_greoup);
#ifdef CONFIG_USER_WIRED_8021X
int rtk_fc_wired_8021x_clean(int isPortNeedDownUp);
int rtk_fc_wired_8021x_setup(void);
int rtk_fc_wired_8021x_set_auth(int portid, unsigned char *mac);
int rtk_fc_wired_8021x_set_unauth(int portid, unsigned char *mac);
int rtk_fc_wired_8021x_flush_port(int portid);
#endif
#if defined(CONFIG_USER_LANNETINFO)
int rtk_fc_update_lan_hpc_for_mib(unsigned char *mac);
int rtk_fc_host_police_control_mib_counter_get(unsigned char *mac, LAN_HOST_POLICE_CTRL_MIB_TYPE_T type, unsigned long long *result);
#endif

#ifdef CONFIG_IPV6
int rtk_fc_ipv6_mvlan_loop_detection_acl_add(MIB_CE_ATM_VC_Tp atmVcEntry);
int rtk_fc_ipv6_mvlan_loop_detection_acl_delete(MIB_CE_ATM_VC_Tp atmVcEntry);
#endif
#endif

#ifdef CONFIG_USER_AVALANCH_DETECT
#ifdef CONFIG_RTL9607C_SERIES
int rtk_avalanch_07c_set(void);
int rtk_avalanch_07c_del(void);
#endif
#endif
#ifdef CONFIG_RTL9607C_SERIES
int rtk_fc_check_enanle_NIC_SRAM_accelerate(void);
#endif

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
int rtk_fc_external_switch_rgmii_port_get(void);
#endif
int rtk_fc_qos_setup_upstream_wrr(int set);
int rtk_fc_qos_setup_downstream_wrr(int set);


int rtk_fc_add_aclTrapPri_for_SWACC(unsigned char priority);
int rtk_fc_del_aclTrapPri_for_SWACC(unsigned char priority);
int rtk_fc_tcp_smallack_protect_set(int enable);

int rtk_fc_flow_delay_config(int enable);

#if defined(CONFIG_XFRM) && defined(CONFIG_USER_STRONGSWAN)
#if defined(CONFIG_RTL8277C_SERIES)
int rtk_fc_ipsec_strongswan_traffic_bypass_flush(void);
int rtk_fc_ipsec_strongswan_traffic_bypass_set(void);
int rtk_fc_ipsec_strongswan_traffic_bypass_config(void);
int rtk_fc_ipsec_strongswan_traffic_upstream_bypass_flush(void);
int rtk_fc_ipsec_strongswan_traffic_upstream_bypass_set(void);
int rtk_fc_ipsec_strongswan_traffic_upstream_bypass_config(void);
#endif
void rtk_ipsec_strongswan_iptable_rule_init(void);
void rtk_ipsec_strongswan_iptable_rule_flush(void);
int rtk_ipsec_strongswan_iptable_rule_add(MIB_IPSEC_SWAN_Tp pEntry);
#endif

#ifdef _PRMT_X_TRUE_CATV_
void rtk_fc_set_CATV(unsigned char enable, char agcoffset);
#endif
#endif

#include <netinet/in.h>
#include <linux/config.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <dirent.h>
#include "utility.h"
#include "mib.h"
#include "subr_net.h"
#include "fc_api.h"
#include "options.h"
#include "subr_net.h"
#include "sockmark_define.h"
#ifdef CONFIG_COMMON_RT_API
#include <rtk/rt/rt_rate.h>
#include <rtk/rt/rt_l2.h>
#include <rtk/rt/rt_qos.h>
#include <rtk/rt/rt_epon.h>
#include <rtk/rt/rt_gpon.h>
#include <rtk/rt/rt_switch.h>
#include <rtk/rt/rt_i2c.h>
#include "rt_rate_ext.h"
#include "rt_igmp_ext.h"
#include <rt_flow_ext.h>
#endif
#if defined(CONFIG_RTL9607C_SERIES)
#include <dal/rtl9607c/dal_rtl9607c_switch.h>
#endif
#if ! defined(CONFIG_LUNA_G3_SERIES)
#include <rtk/stp.h>
#endif
#include <sys/file.h>

//#include "debug.h"

#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
const char FC_SHARE_METER_INDEX_FILE[] = "/var/fc_share_meter_index_file";
const char FC_SHARE_METER_INDEX_TMP_FILE[] = "/var/fc_share_meter_index_file_temp";
#if defined(CONFIG_USER_RTK_LBD)
const char FC_LOOP_DETECTION_ACL_INDEX_FILE[] = "/var/fc_loop_detection_acl_index_file";
#endif
#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
const char FC_INGRESS_CONTROL_PACKET_PROTECTION_FILE[] = "/var/fc_ingress_control_packet_protection_file";
const char FC_EGRESS_CONTROL_PACKET_PROTECTION_FILE[] = "/var/fc_egress_control_packet_protection_file";
const char FC_ITMS_INGRESS_CONTROL_PACKET_PROTECTION_FILE[] = "/var/fc_itms_ingress_control_packet_protection_file";
const char FC_ITMS_EGRESS_CONTROL_PACKET_PROTECTION_FILE[] = "/var/fc_itms_egress_control_packet_protection_file";
#endif
#ifdef CONFIG_USER_AVALANCH_DETECT
const char RTK_ACL_AVLANCHE_RULE_FILE[] = "/var/rtk_avalanche_acl_index_file";
const char RTK_ACL_AVLANCHE_PERMIT_RULE_FILE[] = "/var/rtk_avalanche_acl_permit_index_file";
#endif
#ifdef CONFIG_USER_WIRED_8021X
const char FC_WIRED_8021X_DEFAULT_ACL_FILE[] = "/var/fc_wired_8021x_default_acl_file";
#endif
const char FC_BR_STP_ACL_FILE[] = "/var/fc_br_stp_acl_file";
const char FC_BR_STP_PORT_STAT_FILE[] = "/var/fc_br_stp_port_stat_file";

#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL
const char FC_US_MAC_ACL_TRAP_TO_PS_FILE[] = "/var/fc_us_mac_trap_to_ps_file";
const char FC_US_MAC_ACL_TRAP_TO_PS_FILE_TEMP[] = "/var/fc_us_mac_trap_to_ps_file_temp";
const char FC_DS_MAC_ACL_TRAP_TO_PS_FILE[] = "/var/fc_ds_mac_trap_to_ps_file";
const char FC_DS_MAC_ACL_TRAP_TO_PS_FILE_TEMP[] = "/var/fc_ds_mac_trap_to_ps_file_temp";
#endif
const char FC_TCP_SYN_RATE_LIMIT_ACL_FILE[] = "/var/fc_ds_tcp_syn_rate_limit_acl";
const char FC_UDP_ATTACK_RATE_LIMIT_ACL_FILE[] = "/var/fc_ds_udp_attack_rate_limit_acl";
const char FC_DDOS_SMURF_ATTACK_PROTECT_FILE[] = "/var/fc_ddos_smurf_attack_protect_acl";
#ifdef CONFIG_USER_RT_ACL_SYN_FLOOD_PROOF_SUPPORT
const char FC_DDOS_TCP_SYN_RATE_LIMIT_ACL_FILE[] = "/var/fc_ddos_tcp_syn_rate_limit_acl";
#endif
#ifdef CONFIG_IPV6
const char FC_ipv6_mvlan_loop_detection_ACL_FILE[] = "/var/fc_ipv6_mvlan_loop_detection_acl";
#endif
const char FC_SW_ACC_PRIO_FILE[] = "/var/fc_sw_acc_prio";
const char FC_tcp_smallack_protect_ACL_FILE[] = "/var/fc_tcp_smallack_protect_acl";

#define ConfigACLLock "/var/run/configACLLock"
#define LOCK_ACL()	\
int lockfd; \
do {	\
	if ((lockfd = open(ConfigACLLock, O_RDWR)) == -1) {	\
		perror("open ACL lockfile");	\
		return 0;	\
	}	\
	while (flock(lockfd, LOCK_EX)) { \
		if (errno != EINTR) \
			break; \
	}	\
} while (0)

#define UNLOCK_ACL()	\
do {	\
	flock(lockfd, LOCK_UN);	\
	close(lockfd);	\
} while (0)

void rtk_fc_flow_update_in_real_time_init(void);
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
int rtk_fc_external_switch_init(void);
#endif

enum {RTK_FC_SMP_DISPATCH_TX, RTK_FC_SMP_DISPATCH_RX};
#if defined(CONFIG_RTK_L34_FC_IPI_NIC_TX) || defined(CONFIG_RTK_L34_FC_IPI_NIC_RX)
#define PROC_NIC_SMP_DISPATCH_TX "/proc/fc/ctrl/smp_dispatch_nic_tx"
#define PROC_NIC_SMP_DISPATCH_RX "/proc/fc/ctrl/smp_dispatch_nic_rx"
int rtk_fc_nic_smp_dispatch(int direction)
{
#ifdef CONFIG_SMP
	char ipi_cmd[256] = "";
	int dispatch = -1;
	int mib_id = MIB_NIC_SMP_DISPATCH_TX;
	char *proc = PROC_NIC_SMP_DISPATCH_TX;

	if(direction == RTK_FC_SMP_DISPATCH_RX)
	{
		mib_id = MIB_NIC_SMP_DISPATCH_RX;
		proc = PROC_NIC_SMP_DISPATCH_RX;
	}

	mib_get_s(mib_id, &dispatch, sizeof(dispatch));
	if(dispatch == -2)
	{
		snprintf(ipi_cmd, sizeof(ipi_cmd), "echo \"mode 0 cpu 0\" > %s", proc);
		system(ipi_cmd);
	}
	else if(dispatch >= 0)
	{
		snprintf(ipi_cmd, sizeof(ipi_cmd), "echo \"mode 1 cpu %d\" > %s", dispatch % CONFIG_NR_CPUS, proc);
		system(ipi_cmd);
	}
#endif
	return 0;
}
#endif

#if defined(CONFIG_RTK_L34_FC_IPI_WIFI_TX) || defined(CONFIG_RTK_L34_FC_IPI_WIFI_RX)
#define PROC_WLAN_SMP_DISPATCH_TX "/proc/fc/ctrl/smp_dispatch_wifi_tx"
#define PROC_WLAN_SMP_DISPATCH_RX "/proc/fc/ctrl/smp_dispatch_wifi_rx"
int rtk_fc_wlan_smp_dispatch(int direction)
{
#ifdef WLAN_SUPPORT
	char ipi_cmd[256] = "";
	int dispatch = -1;
	int mib_id = MIB_WLAN_SMP_DISPATCH_TX;
	char *proc = PROC_WLAN_SMP_DISPATCH_TX;
	int i;

	if(direction == RTK_FC_SMP_DISPATCH_RX)
	{
		mib_id = MIB_WLAN_SMP_DISPATCH_RX;
		proc = PROC_WLAN_SMP_DISPATCH_RX;
	}

	for(i = 0; i<NUM_WLAN_INTERFACE; i++)
	{
		wlan_idx = i;

		mib_get_s(mib_id, &dispatch, sizeof(dispatch));
		if(dispatch == -2)
		{
			snprintf(ipi_cmd, sizeof(ipi_cmd), "echo \"wlan %d mode 0 cpu 0\" > %s", wlan_idx, proc);
			system(ipi_cmd);
		}
		else if(dispatch >= 0)
		{
			snprintf(ipi_cmd, sizeof(ipi_cmd), "echo \"wlan %d mode 1 cpu %d\" > %s", wlan_idx, dispatch % CONFIG_NR_CPUS, proc);
			system(ipi_cmd);
		}
	}
#endif
	return 0;
}
#endif

static int rtk_fc_wlan_smp_related_settings(void)
{
#if defined(WLAN_SUPPORT)
#if defined(CONFIG_RTK_SOC_RTL9607C) && defined(WIFI5_WIFI6_RUNTIME)
#if defined(CONFIG_CMCC)
	DIR* dir=NULL;
	dir = opendir(PROC_WLAN_DIR_NAME);
	if(dir){
		closedir(dir);
		va_cmd("/bin/sh", 2, 1, "-c", "echo wifi_rx_hash 1 > /proc/fc/ctrl/global_ctrl");
		va_cmd("/bin/sh", 2, 1, "-c", "echo wifi_rx_gmac_auto_sel 1 > /proc/fc/ctrl/global_ctrl");
		// modify 2.4G WIFI to CPU 1
		va_cmd("/bin/sh", 2, 1, "-c", "echo 2 > /proc/irq/73/smp_affinity");
	}
	else{
		va_cmd("/bin/sh", 2, 1, "-c", "echo wifi_rx_hash 0 > /proc/fc/ctrl/global_ctrl");
		va_cmd("/bin/sh", 2, 1, "-c", "echo wifi_rx_gmac_auto_sel 0 > /proc/fc/ctrl/global_ctrl");
		// modify 2.4G WIFI to CPU 3
		va_cmd("/bin/sh", 2, 1, "-c", "echo 8 > /proc/irq/73/smp_affinity");
	}
#endif // defined(CONFIG_CMCC)
#endif // defined(CONFIG_RTK_SOC_RTL9607C) && defined(WIFI5_WIFI6_RUNTIME)
#endif // defined(WLAN_SUPPORT)
	return 0;
}

#if defined(CONFIG_RTL9607C_SERIES)
static int rtk_qos_tick_tocken_init(void)
{
	unsigned char rateBase8192Kbps;
	int chip_version=0; // 1 -> 9603C, 2->9607C, 3-> 9603VD, 0->others
	unsigned int chipId, rev, subType;
	rtk_switch_version_get(&chipId, &rev, &subType);

	switch(chipId)
	{
		case RTL9603CVD_CHIP_ID:/*9603D*/
			chip_version = 3;
			break;
		case RTL9607C_CHIP_ID:
			switch(subType)
			{
			case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA4:
			case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA5:
			case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA6:
				chip_version = 1;
				break;
			case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA6:
			case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA7:
			default:
				chip_version = 2;
				break;
			}
			break;

		default:
			printf("The chipId is not support=%d \n", chipId);
	}

	/********************************************
	diag meter set pon-tick-token tick-period 73 token 97	, base on the clock rate of PON
	diag meter set tick-token tick-period 30 token 79			, base on the clock rate of switch
	diag register set 0x801030 0x80001e4f					, base on the clock rate of L34 , the value should be same with switch
														, 30 hex:1e,  79 hex:4f
	********************************************/
	mib_get(MIB_METER_RATEBASE_8192KBPS_DISABLE, (void *)&rateBase8192Kbps);
	if (rateBase8192Kbps == 0) //8000kbps base
	{
		system("diag meter set pon-tick-token tick-period 73 token 97");
		if (chip_version == 1 ) //9603C
		{
			printf("[%s:%d] Enable 9603C 8000Kbps base !!\n", __FUNCTION__, __LINE__);
			system("diag meter set tick-token tick-period 30 token 79");
			system("diag register set 0x801030 0x80001e4f");
		}
		else if (chip_version == 2 ){ //9607C
			printf("[%s:%d] Enable 9607C 8000Kbps base !!\n", __FUNCTION__, __LINE__);
			system("diag meter set tick-token tick-period 71 token 151");
			system("diag register set 0x801030 0x80004797");
		}
		else if (chip_version == 3 ){ //9603CVD
			printf("[%s:%d] Enable 9603C-vD 8000Kbps base !!\n", __FUNCTION__, __LINE__);
			system("diag meter set tick-token tick-period 35 token 151");
			system("diag register set 0x801030 0x80002397");
		}
	}
	else //8192kbps base
	{
		system("diag meter set pon-tick-token tick-period 110 token 149");
		if (chip_version == 1 ) //9603C
		{
			printf("[%s:%d] Enable 9603C 8000Kbps base !!\n", __FUNCTION__, __LINE__);
			system("diag meter set tick-token tick-period 22 token 60");
			system("diag register set 0x801030 0x8000163c");
		}
		else if (chip_version == 2 ){ //9607C
			printf("[%s:%d] Enable 9607C 8000Kbps base !!\n", __FUNCTION__, __LINE__);
			system("diag meter set tick-token tick-period 87 token 189");
			system("diag register set 0x801030 0x800057BD");
		}
		else if (chip_version == 3 ){ //9603CVD
			printf("[%s:%d] Enable 9603C-vD 8000Kbps base !!\n", __FUNCTION__, __LINE__);
			system("diag meter set tick-token tick-period 43 token 189");
			system("diag register set 0x801030 0x80002BBD");
		}
	}

	return 0;
}
#endif


int rtk_fc_ctrl_init(void)
{
	//reset skb mark bit definition of FC driver
	//it must sync with user space skb mark definition
	//which defined in qos file.
	//		  28	  24	  20	  16	  12	  8 	  4 	  0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// |  reserved for FC[31:19] |LAN Mark | QosEntryID|SWQID  |e| p   |
	// |		(13bits)		 |5bits    | [13:8](6b)|[7:4]  |1|[2:0]|
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	//======================================================================
	/*
	- meterIdx: 	bits[23:19]
	- meterIdx en:	-disabled-[24]
	- fwd by PS:	-disabled-[25]
	- mibIdx:		bits[30:26]
	- mibIdx en:	-disabled-[31]
	*/
	char buf[200]={0};
	unsigned char deviceType = 0;

	system("echo -1 > /proc/fc/ctrl/skbmark_qid");
	system("echo -1 > /proc/fc/ctrl/skbmark_streamId");
	system("echo -1 > /proc/fc/ctrl/skbmark_streamId_enable");
	system("echo -1 > /proc/fc/ctrl/skbmark_fwdByPs");
	system("echo -1 > /proc/fc/ctrl/skbmark_mibId_enable");
	system("echo -1 > /proc/fc/ctrl/skbmark_meterId");
	system("echo -1 > /proc/fc/ctrl/skbmark_meterId_enable");
	system("echo -1 > /proc/fc/ctrl/skbmark_swshaper_enable");
	system("echo -1 > /proc/fc/ctrl/skbmark_mibId");
	system("echo -1 > /proc/fc/ctrl/skbmark_skipFcFunc");

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "echo %d > /proc/fc/ctrl/skbmark_qid",SOCK_MARK_QOS_SWQID_START);
	system(buf);
	memset(buf, 0, sizeof(buf));
	//use FC software shaper for Flow limit instead of hw meter policing limit. It look good for TCP rate limit
	//sprintf(buf, "echo %d > /proc/fc/ctrl/meteridx_enable_bit_of_mark", SOCK_MARK_METER_INDEX_ENABLE_START);
	sprintf(buf, "echo %d > /proc/fc/ctrl/skbmark_swshaper_enable", SOCK_MARK_METER_INDEX_ENABLE_START);
	system(buf);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "echo %d > /proc/fc/ctrl/skbmark_meterId", SOCK_MARK_METER_INDEX_START);
	system(buf);
#ifdef CONFIG_RTK_SKB_MARK2
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "echo mark2 %d > /proc/fc/ctrl/skbmark_fwdByPs", SOCK_MARK2_FORWARD_BY_PS_START);
	system(buf);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "echo mark2 %d > /proc/fc/ctrl/skbmark_meterId_enable", SOCK_MARK2_METER_HW_INDEX_ENABLE_START);
	system(buf);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "echo mark2 %d > /proc/fc/ctrl/skbmark_mibId_enable", SOCK_MARK2_MIB_INDEX_ENABLE_START);
	system(buf);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "echo mark2 %d > /proc/fc/ctrl/skbmark_mibId", SOCK_MARK2_MIB_INDEX_START);
	system(buf);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "echo mark2 %d > /proc/fc/ctrl/skbmark_skipFcFunc", SOCK_MARK2_SKIP_FC_FUNC_START);
	system(buf);
#ifdef CONFIG_RTK_HOST_SPEEDUP
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "echo %d > /proc/HostSpeedUP-SkipFcBit", SOCK_MARK2_SKIP_FC_FUNC_START);
	system(buf);
#endif//endof CONFIG_RTK_HOST_SPEEDUP

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "echo mark2 %d > /proc/fc/ctrl/skbmark_streamId", SOCK_MARK2_STREAMID_START);
	system(buf);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "echo mark2 %d > /proc/fc/ctrl/skbmark_streamId_enable", SOCK_MARK2_STREAMID_ENABLE_START);
	system(buf);
#endif
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	rtk_fc_external_switch_init();
#endif

	rtk_fc_ds_tcp_syn_rate_limit_set();

	/* not use acl */
	//rtk_fc_ddos_smurf_attack_protect();

	//Kernel nf conntrack table setting
	system("echo 100000 > proc/sys/net/nf_conntrack_max");
	system("echo 10000 > proc/sys/net/netfilter/nf_conntrack_expect_max");

	// disable kernel TCP Squence check in case one direction hw forward, the other direction kernel forward
	system("echo 1 > /proc/sys/net/netfilter/nf_conntrack_tcp_be_liberal");

#ifdef USE_DEONET // kernel nf_conntrack optimization
	system("echo 86400 > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_established");
	system("echo 5 > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_syn_sent");
	system("echo 5 > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_time_wait");
	system("echo 90 > /proc/sys/net/netfilter/nf_conntrack_udp_timeout");
	system("echo 180 > /proc/sys/net/netfilter/nf_conntrack_udp_timeout_stream");
#endif

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	unsigned int pon_mode = 0;
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
	if(pon_mode == EPON_MODE){
		system("echo 1 > proc/fc/ctrl/epon_llid_format");
	}
#endif

#ifdef CONFIG_RTL9607C_SERIES
#ifdef CONFIG_WFO_VIRT
	/* Please don't reset GMAC again!!! */
#else /* !CONFIG_WFO_VIRT */
	rtk_fc_check_enanle_NIC_SRAM_accelerate();
#endif /* CONFIG_WFO_VIRT */
#endif

	rtk_fc_flow_update_in_real_time_init();

#ifdef CONFIG_RTK_L34_FC_IPI_NIC_TX
	rtk_fc_nic_smp_dispatch(RTK_FC_SMP_DISPATCH_TX);
#endif
#ifdef CONFIG_RTK_L34_FC_IPI_NIC_RX
	rtk_fc_nic_smp_dispatch(RTK_FC_SMP_DISPATCH_RX);
#endif

#if defined(WLAN_SUPPORT) && defined(CONFIG_RTK_L34_FC_IPI_WIFI_TX)
	rtk_fc_wlan_smp_dispatch(RTK_FC_SMP_DISPATCH_TX);
#endif
#if defined(WLAN_SUPPORT) && defined(CONFIG_RTK_L34_FC_IPI_WIFI_RX)
	rtk_fc_wlan_smp_dispatch(RTK_FC_SMP_DISPATCH_RX);
#endif

	rtk_fc_wlan_smp_related_settings();

	// switch FC host software MIB to payload mode for accurate host rate
#if defined(CONFIG_CMCC) || defined(CONFIG_CU) || defined(CONFIG_TRUE)
	system("echo 0 > /proc/fc/ctrl/sw_shaperMib_update_mode_flow");
#else
	system("echo 1 > /proc/fc/ctrl/sw_shaperMib_update_mode_flow");
#endif
#ifdef CONFIG_TRUE
	system("echo 0 > /proc/fc/ctrl/sw_shaperMib_update_mode_host");
#else
	system("echo 1 > /proc/fc/ctrl/sw_shaperMib_update_mode_host");
#endif

#if defined(CONFIG_RTL9607C_SERIES)
	rtk_qos_tick_tocken_init();
	unsigned char flow_mode=0;
	mib_get_s(MIB_FLOW_MODE, (void *)&flow_mode, sizeof(flow_mode));
	snprintf(buf, sizeof(buf), "echo %d > /proc/fc/ctrl/flow_mode", flow_mode);
	system(buf);

	unsigned char fc_dynamic_prehashpattern=0;
	mib_get_s(MIB_FC_DYNAMIC_PREHASHPATTERN, (void *)&fc_dynamic_prehashpattern, sizeof(fc_dynamic_prehashpattern));
	snprintf(buf, sizeof(buf), "echo %d > /proc/fc/ctrl/dynamic_prehashPattern", fc_dynamic_prehashpattern);
	system(buf);

	unsigned char shaperMib_update_mode_flow=0;
	mib_get_s(MIB_SHAPERMIB_UPDATE_MODE_FLOW, (void *)&shaperMib_update_mode_flow, sizeof(shaperMib_update_mode_flow));
	snprintf(buf, sizeof(buf), "echo %d > /proc/fc/ctrl/sw_shaperMib_update_mode_flow", shaperMib_update_mode_flow);
	system(buf);

	system("echo tx_jumbo_frame_enabled 1 > /proc/rtl8686gmac/misc");
#elif defined(CONFIG_RTL8198X_SERIES)
{
	unsigned char shaperMib_update_mode_flow=0;
	mib_get_s(MIB_SHAPERMIB_UPDATE_MODE_FLOW, (void *)&shaperMib_update_mode_flow, sizeof(shaperMib_update_mode_flow));
	snprintf(buf, sizeof(buf), "echo %d > /proc/fc/ctrl/sw_shaperMib_update_mode_flow", shaperMib_update_mode_flow);
	system(buf);
}
#endif

	mib_get_s(MIB_DEVICE_TYPE, (void *)&deviceType, sizeof(deviceType));
	// SFU need to test VLAN Aggregation mode. So need to check DMAC with different vlan id.
	if (deviceType == 0 || deviceType == 2) //hybrid and SFU mode
		system("echo 1 > proc/fc/ctrl/l2_port_moving_check_with_vlan");
#if defined(CONFIG_YUEME) && defined(CONFIG_LUNA_G3_SERIES)
	sprintf(buf, "echo lan_queue_ability_enhance 1 > /proc/fc/ctrl/global_ctrl");
	system(buf);
#endif
#if defined(CONFIG_YUEME_DPI)
#ifdef CONFIG_RTK_SKB_MARK2
	sprintf(buf, "echo MARK 2 BIT %d > /proc/ctc_dpi/dpi_fwdByPS", SOCK_MARK2_FORWARD_BY_PS_START);
	system(buf);

	//For DPI DSCP remark
	snprintf(buf, sizeof(buf), "echo DSCP_US_REAMRK_ENABLE MARK 2 BIT %d > /proc/ctc_dpi/dpi_dscpmark", SOCK_MARK2_DSCP_REMARK_ENABLE_START);
	system(buf);
	snprintf(buf, sizeof(buf), "echo DSCP_US_REAMRK_RANGE MARK 2 BIT %d %d > /proc/ctc_dpi/dpi_dscpmark", SOCK_MARK2_DSCP_START, SOCK_MARK2_DSCP_END);
	system(buf);

	snprintf(buf, sizeof(buf), "echo DSCP_DS_REAMRK_ENABLE MARK 2 BIT %d > /proc/ctc_dpi/dpi_dscpmark", SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_START);
	system(buf);
	snprintf(buf, sizeof(buf), "echo DSCP_DS_REMARK_RANGE MARK 2 BIT %d %d > /proc/ctc_dpi/dpi_dscpmark", SOCK_MARK2_DSCP_DOWN_START, SOCK_MARK2_DSCP_DOWN_END);
	system(buf);

#endif
	sprintf(buf, "echo 1 > /proc/ctc_dpi/dpi_func");
	system(buf);
#endif
	sprintf(buf, "echo cpri 1 > /proc/fc/ctrl/flow_hash_input_config");
	system(buf);
#ifdef CONFIG_CU_DPI // use rtk_fc_flow_delay_config
	unsigned int trap_first_n_pkts_to_ps = 0;
	mib_get_s(MIB_CU_TRAP_FIRST_N_PKTS_TO_PS, (void *)&trap_first_n_pkts_to_ps, sizeof(trap_first_n_pkts_to_ps));
	if(trap_first_n_pkts_to_ps > 0)
	{
		mib_set(MIB_FC_FLOW_DELAY, (void *)&trap_first_n_pkts_to_ps);
		unsigned char delay_mode=0; //kernel
		mib_set(MIB_FC_FLOW_DELAY_MODE, (void *)&delay_mode);
	}
#endif

	unsigned char accelerate_status = 0;
	mib_get_s(MIB_FC_BRIDGE_5TUPLE_FLOW_ACCELERATE_BY_2TUPLE, (void *)&accelerate_status, sizeof(accelerate_status));
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "echo %d > /proc/fc/ctrl/bridge_5tuple_flow_accelerate_by_2tuple", accelerate_status);
	system(buf);

	unsigned char flow_l2_skipPstracking = 0;
	mib_get_s(MIB_FC_FLOW_L2_SKIPPSTRACKING, (void *)&flow_l2_skipPstracking, sizeof(flow_l2_skipPstracking));
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "echo %d > /proc/fc/ctrl/fc_flow_l2_skipPsTracking", flow_l2_skipPstracking);
	system(buf);

	unsigned char flow_skipalltracking = 0;
	mib_get_s(MIB_FC_FLOW_SKIPALLPSTRACKING, (void *)&flow_skipalltracking, sizeof(flow_skipalltracking));
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "echo %d > /proc/fc/ctrl/flow_skipAllPsTracking", flow_skipalltracking);
	system(buf);

	unsigned char fc_disable_type = 0, fc_disable_action=0;
	unsigned int fc_disable_mask = 0;
	mib_get_s(MIB_FC_DISABLE_PORT_PORT_MOVING_TYPE, (void *)&fc_disable_type, sizeof(fc_disable_type));
	mib_get_s(MIB_FC_DISABLE_PORT_PORT_MOVING_ACTION, (void *)&fc_disable_action, sizeof(fc_disable_action));
	switch(fc_disable_type)
	{
		case 1: //by wan
		    	fc_disable_mask = rtk_port_get_wan_phyMask();
			break;
		case 2: //by lan
			fc_disable_mask = rtk_port_get_all_lan_phyMask();
			break;
		case 3: //by wan + lan
			fc_disable_mask = rtk_port_get_wan_phyMask() | rtk_port_get_all_lan_phyMask();
			break;
		case 4: //by mask
			mib_get_s(MIB_FC_DISABLE_PORT_PORT_MOVING_MASK, (void *)&fc_disable_mask, sizeof(fc_disable_mask));
			break;
		default:
			fc_disable_mask = 0;
	}

	int i;
	for(i=0;i<32;i++) {
	    	if((fc_disable_mask & 1<<(i))) {
			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), "echo port %d disable> /proc/fc/ctrl/disable_port_port_moving", i);
		}
	}
	system(buf);

	memset(buf, 0, sizeof(buf));
	if(fc_disable_action)
		snprintf(buf, sizeof(buf), "echo action forward> /proc/fc/ctrl/disable_port_port_moving");
	else
		snprintf(buf, sizeof(buf), "echo action drop> /proc/fc/ctrl/disable_port_port_moving");
	system(buf);

#if 0 //no use, for FC new function was not delete flow when L2 FDB timeout
	// to avoid new session cleared by arp failed.
	system("echo 1 > /proc/fc/ctrl/flow_l2_skipPsTracking");
#endif
	rtk_fc_flow_delay_config(1);

#if defined(CONFIG_GPON_FEATURE)
	// On GPON mode avoid CPU busy when OMCI not compelete.
	// Need modify FC l2_macEgrLearning to egress learning mode.
	// When OMCI compelete the systemd will change to ingress mode.
	unsigned int mode = 0;
	unsigned int portMask=0;

	mib_get_s(MIB_PON_MODE, (void *)&mode, sizeof(mode));
	if(mode == GPON_MODE) {
		portMask = (rtk_port_get_wan_phyMask()|rtk_port_get_all_lan_phyMask());
		snprintf(buf, sizeof(buf), "echo 1 0x%x > /proc/fc/ctrl/l2_macEgrLearning", portMask);
		system(buf);
	}
#endif

#if defined(WIFI6_SUPPORT) && defined(CONFIG_RTL9607C_SERIES)
	// Set min low bound criteria of the memory for limit dynamic allocate SKB to avoid system OOM when do WIFI test
	// for support wifi6, suggest set 20M as low bound crieria
	snprintf(buf, sizeof(buf), "echo %u > /sys/module/re8686_rtl9607c/parameters/min_limit_pages", (unsigned int)(20*1024));
	system(buf);
#endif

	return 0;
}

int rtk_fc_ctrl_init2(void)
{
#ifdef CONFIG_CU
	{
		unsigned int igmp_fastleave;
		mib_get_s(MIB_IGMP_FASTLEAVE_ENABLE, (void *)&igmp_fastleave, sizeof(igmp_fastleave));
		if(igmp_fastleave){
			system("echo DEVIFNAME br0 FAST_LEAVE 1 > /proc/igmp/ctrl/brConfig");
			system("echo DEVIFNAME eth0 FAST_LEAVE 1 > /proc/igmp/ctrl/igmpConfig");
			system("echo DEVIFNAME eth0 FAST_LEAVE 1 > /proc/igmp/ctrl/igmp6Config");
			system("echo DEVIFNAME eth0.2 FAST_LEAVE 1 > /proc/igmp/ctrl/igmpConfig");
			system("echo DEVIFNAME eth0.2 FAST_LEAVE 1 > /proc/igmp/ctrl/igmp6Config");
			system("echo DEVIFNAME eth0.3 FAST_LEAVE 1 > /proc/igmp/ctrl/igmpConfig");
			system("echo DEVIFNAME eth0.3 FAST_LEAVE 1 > /proc/igmp/ctrl/igmp6Config");
			system("echo DEVIFNAME eth0.4 FAST_LEAVE 1 > /proc/igmp/ctrl/igmpConfig");
			system("echo DEVIFNAME eth0.4 FAST_LEAVE 1 > /proc/igmp/ctrl/igmp6Config");
			system("echo DEVIFNAME eth0.5 FAST_LEAVE 1 > /proc/igmp/ctrl/igmpConfig");
			system("echo DEVIFNAME eth0.5 FAST_LEAVE 1 > /proc/igmp/ctrl/igmp6Config");
		}
	}
#endif
	return 0;
}


#if defined(CONFIG_COMMON_RT_API)
/**
 * Input:
 *		entry				pointer of rt_acl_filterAndQos_t which will be clear
 * Return:
 *      RT_ERR_OK			OK
 *      RT_ERR_FAILED		Failed
 */
int rtk_acl_entry_clear(rt_acl_filterAndQos_t *entry){
	memset(entry, 0x0, sizeof(rt_acl_filterAndQos_t));
	return 0;
}

int rtk_acl_entry_show(rt_acl_filterAndQos_t *entry)
{
        int i;
		printf("===========================================\n");
        printf("ACL Weight \t: %d\n", entry->acl_weight);
        printf("[ Patterns ] \n");

        if(entry->filter_fields & RT_ACL_INGRESS_PORT_MASK_BIT)
                printf("  ingress_port_mask \t\t: 0x%x\n", entry->ingress_port_mask);
        if(entry->filter_fields & RT_ACL_INGRESS_WLAN_MBSSID_MASK_BIT) {
                for(i=0; i<RT_WLAN_DEVICE_MAX; i++)
                        printf("  ingress_wlan%d_mbssid_mask \t: 0x%x\n", i, entry->ingress_wlan_mbssid_mask[i]);
        }
        if(entry->filter_fields & RT_ACL_INGRESS_SMAC_BIT) {
                printf("  ingress_smac \t\t\t: %pM\n", &(entry->ingress_smac));
                printf("  ingress_smac_mask \t\t: %pM\n", &(entry->ingress_smac_mask));
        }
        if(entry->filter_fields & RT_ACL_INGRESS_DMAC_BIT) {
                printf("  ingress_dmac \t\t\t: %pM\n", &(entry->ingress_dmac));
                printf("  ingress_dmac_mask \t\t: %pM\n", &(entry->ingress_dmac_mask));
        }
        if(entry->filter_fields & RT_ACL_INGRESS_ETHERTYPE_BIT) {
                printf("  ingress_ethertype \t\t: 0x%x\n", entry->ingress_ethertype);
                printf("  ingress_ethertype_mask \t: 0x%x\n", entry->ingress_ethertype_mask);
        }
        if(entry->filter_fields & RT_ACL_INGRESS_STAGIF_BIT)
                printf("  ingress_stagif \t\t: %s\n", entry->ingress_stagif?"Must have Stag":"Must not have Stag");
        if(entry->filter_fields & RT_ACL_INGRESS_STAG_VID_BIT)
                printf("  ingress_stag_vid \t\t: %d\n", entry->ingress_stag_vid);
        if(entry->filter_fields & RT_ACL_INGRESS_STAG_PRI_BIT)
                printf("  ingress_stag_pri \t\t: %d\n", entry->ingress_stag_pri);
        if(entry->filter_fields & RT_ACL_INGRESS_CTAGIF_BIT)
                printf("  ingress_ctagif \t\t: %s\n", entry->ingress_ctagif?"Must have Ctag":"Must not have Ctag");
        if(entry->filter_fields & RT_ACL_INGRESS_CTAG_VID_BIT)
                printf("  ingress_ctag_vid \t\t: %d\n", entry->ingress_ctag_vid);
        if(entry->filter_fields & RT_ACL_INGRESS_CTAG_PRI_BIT)
                printf("  ingress_ctag_pri \t\t: %d\n", entry->ingress_ctag_pri);

        if(entry->filter_fields & RT_ACL_INGRESS_IPV4_TAGIF_BIT)
                printf("  ingress_ipv4_tagif \t\t: %s\n", entry->ingress_ipv4_tagif?"Must be IPv4":"Must not be IPv4");
        if(entry->filter_fields & RT_ACL_INGRESS_IPV4_SIP_RANGE_BIT) {
                printf("  ingress_src_ipv4_addr_start \t: %pI4\n", &(entry->ingress_src_ipv4_addr_start));
                printf("  ingress_src_ipv4_addr_end \t: %pI4\n", &(entry->ingress_src_ipv4_addr_end));
        }
        if(entry->filter_fields & RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT) {
                printf("  ingress_dest_ipv4_addr_start \t: %pI4\n", &(entry->ingress_dest_ipv4_addr_start));
                printf("  ingress_dest_ipv4_addr_end \t: %pI4\n", &(entry->ingress_dest_ipv4_addr_end));
        }
        if(entry->filter_fields & RT_ACL_INGRESS_IPV4_DSCP_BIT)
                printf("  ingress_ipv4_dscp \t\t: %d\n", entry->ingress_ipv4_dscp);
        if(entry->filter_fields & RT_ACL_INGRESS_IPV4_TOS_BIT)
                printf("  ingress_ipv4_tos \t\t: %d\n", entry->ingress_ipv4_tos);

        if(entry->filter_fields & RT_ACL_INGRESS_IPV6_TAGIF_BIT)
                printf("  ingress_ipv6_tagif \t\t: %s\n", entry->ingress_ipv6_tagif?"Must be IPv6":"Must not be IPv6");
        if(entry->filter_fields & RT_ACL_INGRESS_IPV6_SIP_BIT) {
                printf("  ingress_src_ipv6_addr \t: %pI6\n", &(entry->ingress_src_ipv6_addr[0]));
                printf("  ingress_src_ipv6_addr_mask \t: %pI6\n", &(entry->ingress_src_ipv6_addr_mask[0]));
        }
        if(entry->filter_fields & RT_ACL_INGRESS_IPV6_DIP_BIT) {
                printf("  ingress_dest_ipv6_addr \t: %pI6\n", &(entry->ingress_dest_ipv6_addr[0]));
                printf("  ingress_dest_ipv6_addr_mask \t: %pI6\n", &(entry->ingress_dest_ipv6_addr_mask[0]));
        }
        if(entry->filter_fields & RT_ACL_INGRESS_IPV6_DSCP_BIT)
                printf("  ingress_ipv6_dscp \t\t: %d\n", entry->ingress_ipv6_dscp);
        if(entry->filter_fields & RT_ACL_INGRESS_IPV6_TC_BIT)
                printf("  ingress_ipv6_tc \t\t: %d\n", entry->ingress_ipv6_tc);

        if(entry->filter_fields & RT_ACL_INGRESS_L4_TCP_BIT) {
                printf("  ingress_l4_protocal \t\t: tcp \n");
        }else if(entry->filter_fields & RT_ACL_INGRESS_L4_UDP_BIT) {
                printf("  ingress_l4_protocal \t\t: udp \n");
        }else if(entry->filter_fields & RT_ACL_INGRESS_L4_ICMP_BIT) {
                printf("  ingress_l4_protocal \t\t: icmp \n");
        }else if(entry->filter_fields & RT_ACL_INGRESS_L4_ICMPV6_BIT) {
                printf("  ingress_l4_protocal \t\t: icmpv6 \n");
        }else if(entry->filter_fields & RT_ACL_INGRESS_L4_POROTCAL_VALUE_BIT) {
                printf("  ingress_l4_protocal \t\t: %d\n", entry->ingress_l4_protocal);
        }
        if(entry->filter_fields & RT_ACL_INGRESS_L4_SPORT_RANGE_BIT) {
                printf("  ingress_src_l4_port_start \t: %d\n", entry->ingress_src_l4_port_start);
                printf("  ingress_src_l4_port_end \t: %d\n", entry->ingress_src_l4_port_end);
        }
        if(entry->filter_fields & RT_ACL_INGRESS_L4_DPORT_RANGE_BIT) {
                printf("  ingress_dest_l4_port_start \t: %d\n", entry->ingress_dest_l4_port_start);
                printf("  ingress_dest_l4_port_end \t: %d\n", entry->ingress_dest_l4_port_end);
        }

        if(entry->filter_fields & RT_ACL_INGRESS_STREAM_ID_BIT) {
                printf("  ingress_stream_id \t\t: %d\n", entry->ingress_stream_id);
                printf("  ingress_stream_id_mask \t: 0x%x\n", entry->ingress_stream_id_mask);
        }
        if(entry->filter_fields & RT_ACL_INGRESS_TCP_FLAGS_BIT) {
                printf("  ingress_tcp_flags \t\t: 0x%x\n", entry->ingress_tcp_flags);
                printf("  ingress_tcp_flags_mask \t: 0x%x\n", entry->ingress_tcp_flags_mask);
        }
        if(entry->filter_fields & RT_ACL_INGRESS_PKT_LEN_RANGE_BIT) {
                printf("  ingress_packet_length_start \t: %d\n", entry->ingress_packet_length_start);
                printf("  ingress_packet_length_end \t: %d\n", entry->ingress_packet_length_end);
        }

        if(entry->filter_fields == 0x0)
                printf("  NULL\n");

        printf("[ Actions ] \n");
        if(entry->action_fields & RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT)
                printf("  FORWARD \t: drop \n");
        if(entry->action_fields & RT_ACL_ACTION_FORWARD_GROUP_PERMIT_BIT)
                printf("  FORWARD \t: permit \n");
        if(entry->action_fields & RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT)
                printf("  FORWARD \t: trap \n");
        if(entry->action_fields & RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT)
                printf("  PRIORITY \t: acl priority/trap priority %d \n", entry->action_priority_group_trap_priority);
        if(entry->action_fields & RT_ACL_ACTION_METER_GROUP_SHARE_METER_BIT)
                printf("  METER \t: shere meter with meter index %d \n", entry->action_meter_group_share_meter_index);

		if(entry->action_fields == 0x0)
				printf("	NULL\n");
		printf("===========================================\n\n");
		return 0;
}

/****************************************
 *	set rt_acl_filterAndQos_t API		*
 ***************************************/

/**
 * Input:
 *		entry		pointer of rt_acl_filterAndQos_t which will be add
 *		weight		acl weight(0~65536)
 * Return:
 *      0			OK
 *      1			Failed
 */
int rtk_acl_set_acl_weight(rt_acl_filterAndQos_t *entry, unsigned int weight){

	if(weight <= 65536)
		entry->acl_weight = weight;
	else{
		printf("[%s] wrong acl_weight(%d)\n", __FUNCTION__, weight);
		return 1;
	}

	return 0;
}

/**
 * Input:
 *		entry					pointer of rt_acl_filterAndQos_t which will be add
 *		ingress_port_mask		phy port mask
 * Return:
 *      0			OK
 *      1			Failed
 */
int rtk_acl_set_ingress_port_mask(rt_acl_filterAndQos_t *entry, unsigned int ingress_port_mask){
	entry->filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	entry->ingress_port_mask = ingress_port_mask;
	return 0;
}

/**
 * Input:
 *		entry				pointer of rt_acl_filterAndQos_t which will be add
 *		dport_start			start of layer 4 dest port
 *		dport_end			end of layer 4 dest port
 * Return:
 *      0			OK
 *      1			Failed
 */
int rtk_acl_set_l4_dport(rt_acl_filterAndQos_t *entry, unsigned int dport_start, unsigned int dport_end){
	// set l4 dst port
	if((dport_start < 65536) && (dport_end < 65536) && (dport_start <= dport_end)){
		entry->filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
		entry->ingress_dest_l4_port_start = dport_start;
		entry->ingress_dest_l4_port_end = dport_end;
	}else{
		printf("[%s] wrong ingress_dest_l4_port(%d to %d)\n", __FUNCTION__, dport_start, dport_end);
		return 1;
	}
	return 0;
}

/**
 * Input:
 *		entry				pointer of rt_acl_filterAndQos_t which will be add
 *		action				acl rule action
 * Return:
 *      0			OK
 *      1			Failed
 */
int rtk_acl_set_action(rt_acl_filterAndQos_t *entry, unsigned int action){
	// acl action
	if(action > ((0x1 << RT_ACL_ACTION_FIELDS_INDEX_MAX) - 1)){
		printf("[%s] invalid acl action(0x%x)\n", __FUNCTION__, action);
		return 1;
	}
	entry->action_fields |= action;
	return 0;
}

/**
 * Input:
 *		entry		pointer of rt_acl_filterAndQos_t which will be add
 *		mib_id		the mib_id of running setting which need to be add
 * Return:
 *      0			OK
 *      1			Failed
 */
int rtk_acl_add(rt_acl_filterAndQos_t *entry, int mib_id){
	unsigned int acl_idx=0;

	if(entry->filter_fields == 0x0){
		printf("[%s] there is no acl pattern\n", __FUNCTION__);
		goto err;
	}

	if(entry->action_fields == 0x0){
		printf("[%s] there is no acl action\n", __FUNCTION__);
		goto err;
	}

	// delete old acl rule
	if(mib_get_s(mib_id, &acl_idx, sizeof(acl_idx))){
		if(acl_idx != ACL_DEFAULT_IDX){
			if(RT_ERR_FAILED == rt_acl_filterAndQos_del(acl_idx)){
				printf("[%s] del urlblock rt_acl rule fail[idx=%d]\n", __FUNCTION__, acl_idx);
				goto err;
			}

			acl_idx=ACL_DEFAULT_IDX;
			if(0 == mib_set(mib_id, (void *) &acl_idx)){
				printf("[%s] mib_set(mib_id=%d) fail\n", __FUNCTION__, mib_id);
				goto err;
			}
		}
	}

	//rtk_acl_entry_show(entry);	//for debug

	if(RT_ERR_OK != rt_acl_filterAndQos_add(*entry, &acl_idx)){
		printf("[%s] add acl entry fail\n", __FUNCTION__);
		goto err;
	}

	if(0 == mib_set(mib_id, (void *) &acl_idx)){
		printf("[%s] mib_set(mib_id=%d) fail, delete fc acl rule\n", __FUNCTION__, mib_id);
		if(RT_ERR_FAILED==rt_acl_filterAndQos_del(acl_idx))		//delete acl rule
			printf("[%s] del urlblock rt_acl rule fail[idx=%d]\n", __FUNCTION__, acl_idx);
		goto err;
	}
	return 0;
err:
	return 1;
}

/**
 * Input:
 *		mib_id		the mib_id of running setting which need to be delete
 * Return:
 *      0			OK
 *      1			Failed
 */
int rtk_acl_del(int mib_id){
	unsigned int acl_idx=0;
	int ret=-1;
	if(mib_get_s(mib_id, &acl_idx, sizeof(acl_idx))){
		if(acl_idx != ACL_DEFAULT_IDX){
			if(RT_ERR_FAILED == rt_acl_filterAndQos_del(acl_idx)){
				printf("[%s] del urlblock rt_acl rule fail[idx=%d], ret=%d\n", __FUNCTION__, acl_idx, ret);
				goto err;
			}

			acl_idx=ACL_DEFAULT_IDX;
			if(0 == mib_set(mib_id, (void *) &acl_idx)){
				printf("[%s] mib_set(mib_id=%d) fail\n", __FUNCTION__, mib_id);
				goto err;
			}
		}
	}else{
		printf("[%s] get mib_idx fail.\n", __FUNCTION__);
		goto err;
	}

	return 0;
err:
	return 1;
}
#ifdef URL_BLOCKING_SUPPORT
int RTK_FC_URL_Filter_Set(int dport){
#ifdef CONFIG_RTK_SKB_MARK2
	unsigned char mark0[64], mark[64];
	char dportstr[16];
	sprintf(mark0, "0x0/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);
	sprintf(mark, "0x%llx/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK, SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);

	//DEBUGMODE(STA_INFO|STA_SCRIPT|STA_WARNING|STA_ERR);
	/*	set url block fwd rule
	 *	use connmark to trap 80/443
	 */
	va_cmd(IPTABLES, 20, 1, "-t", "mangle", (char *)FW_ADD, (char *)WEBURLPRE_CHAIN, "-i", "br0", "-p", "tcp", "-m", "multiport","--dports", "80,443",
							"-m", "conntrack", "--ctstate", "NEW", "-j", "MARK2", "--set-mark2", mark);
	va_cmd(IPTABLES, 17, 1, "-t", "mangle", (char *)FW_ADD, (char *)WEBURLPOST_CHAIN, "-p", "tcp", "-m", "multiport","--dports", "80,443",
							"-m", "conntrack", "--ctstate", "NEW", "-j", "CONNMARK2", "--save-mark2");
	va_cmd(IPTABLES, 25, 1, "-t", "mangle", (char *)FW_ADD, (char *)WEBURLPOST_CHAIN, "-p", "tcp", "-m", "multiport", "--dports", "80,443", "--tcp-flags", "PSH,ACK", "PSH,ACK",
							"-m", "conntrack", "--ctstate", "ESTABLISHED", "-m", "connmark2", "--mark2", mark, "-j", "MARK2", "--set-mark2", mark0);
	va_cmd(IPTABLES, 24, 1, "-t", "mangle", (char *)FW_ADD, (char *)WEBURLPOST_CHAIN, "-p", "tcp", "-m", "multiport", "--dports", "80,443", "--tcp-flags", "PSH,ACK", "PSH,ACK",
							"-m", "conntrack", "--ctstate", "ESTABLISHED", "-m", "connmark2", "--mark2", mark, "-j", "CONNMARK2", "--save-mark2");
	va_cmd(IPTABLES, 17, 1, "-t", "mangle", (char *)FW_ADD, (char *)WEBURLPOST_CHAIN, "-p", "tcp", "-m", "multiport", "--dports", "80,443",
							"-m", "conntrack", "--ctstate", "ESTABLISHED", "-j", "CONNMARK2", "--restore-mark2");

#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 20, 1, "-t", "mangle", (char *)FW_ADD, (char *)WEBURLPRE_CHAIN, "-i", "br0", "-p", "tcp", "-m", "multiport","--dports", "80,443",
							"-m", "conntrack", "--ctstate", "NEW", "-j", "MARK2", "--set-mark2", mark);
	va_cmd(IP6TABLES, 17, 1, "-t", "mangle", (char *)FW_ADD, (char *)WEBURLPOST_CHAIN, "-p", "tcp", "-m", "multiport","--dports", "80,443",
							"-m", "conntrack", "--ctstate", "NEW", "-j", "CONNMARK2", "--save-mark2");
	va_cmd(IP6TABLES, 25, 1, "-t", "mangle", (char *)FW_ADD, (char *)WEBURLPOST_CHAIN, "-p", "tcp", "-m", "multiport", "--dports", "80,443", "--tcp-flags", "PSH,ACK", "PSH,ACK",
							"-m", "conntrack", "--ctstate", "ESTABLISHED", "-m", "connmark2", "--mark2", mark, "-j", "MARK2", "--set-mark2", mark0);
	va_cmd(IP6TABLES, 24, 1, "-t", "mangle", (char *)FW_ADD, (char *)WEBURLPOST_CHAIN, "-p", "tcp", "-m", "multiport", "--dports", "80,443", "--tcp-flags", "PSH,ACK", "PSH,ACK",
							"-m", "conntrack", "--ctstate", "ESTABLISHED", "-m", "connmark2", "--mark2", mark, "-j", "CONNMARK2", "--save-mark2");
	va_cmd(IP6TABLES, 17, 1, "-t", "mangle", (char *)FW_ADD, (char *)WEBURLPOST_CHAIN, "-p", "tcp", "-m", "multiport", "--dports", "80,443",
							"-m", "conntrack", "--ctstate", "ESTABLISHED", "-j", "CONNMARK2", "--restore-mark2");
#endif

#endif
	return 0;
}

int RTK_FC_URL_Filter_Flush(void){
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)WEBURLPRE_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)WEBURLPOST_CHAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", (char *)WEBURLPRE_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", (char *)WEBURLPOST_CHAIN);
#endif

	return 0;
}
#endif

static struct rtk_qos_share_meter_entry rtk_qos_share_meter_table[] = {
	{"ip_QoS_rate_shaping", RTK_APP_TYPE_IP_QOS_RATE_SHAPING, RT_METER_TYPE_FLOW},
	{"DS_TCP_Syn_rate_limit", RTK_APP_TYPE_DS_TCP_SYN_RATE_LIMIT, RT_METER_TYPE_ACL},
	{"DS_UDP_attack_rate_limit", RTK_APP_TYPE_DS_UDP_ATTACK_RATE_LIMIT, RT_METER_TYPE_ACL},
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	{"cmcc_QoS_service_rate_limit", RTK_APP_TYPE_CMCC_QOS_RATE_LIMIT, RT_METER_TYPE_FLOW},
#endif
	{"lanhost_upstream_rate_limit_by_fc", RTK_APP_TYPE_LANHOST_US_RATE_LIMIT_BY_FC, RT_METER_TYPE_SW},
	{"lanhost_downstream_rate_limit_by_fc", RTK_APP_TYPE_LANHOST_DS_RATE_LIMIT_BY_FC, RT_METER_TYPE_SW},
#ifdef CONFIG_USER_RT_ACL_SYN_FLOOD_PROOF_SUPPORT
	{"DDOS_TCP_Syn_rate_limit", RTK_APP_TYPE_DDOS_TCP_SYN_RATE_LIMIT, RT_METER_TYPE_ACL},
#endif
	{"ACL_LIMIT_BANDWIDTH", RTK_APP_TYPE_ACL_LIMIT_BANDWIDTH, RT_METER_TYPE_ACL},
};

int rtk_qos_share_meter_set(rtk_rate_application_type_t application_type, unsigned int index, unsigned int rate)
{
	unsigned int meterIndex, tableIndex;
	FILE *fp;

	for(tableIndex=RTK_APP_TYPE_IP_QOS_RATE_SHAPING ; tableIndex<RTK_APP_TYPE_MAX ; tableIndex++)
	{
		if(rtk_qos_share_meter_table[tableIndex].application_type == application_type)
		{
			if(rt_rate_shareMeterType_add(rtk_qos_share_meter_table[tableIndex].meter_type, &meterIndex) == RT_ERR_OK)
			{
				if(!(fp = fopen(FC_SHARE_METER_INDEX_FILE, "a")))
				{
					fprintf(stderr, "ERROR! %s\n", strerror(errno));
					return -2;
				}
				fprintf(fp, "alis_name=%s_%d meter_index=%d rate=%u\n", rtk_qos_share_meter_table[tableIndex].alias_name_prefix, index, meterIndex, rate);
				fclose(fp);
				if(rt_rate_shareMeterRate_set(meterIndex, rate) == RT_ERR_OK)
					return meterIndex;
				else
				{
					AUG_PRT("rt_rate_shareMeterRate_set FAIL ! (meter_type=%d,meterIndex=%d,rate=%u)\n", rtk_qos_share_meter_table[tableIndex].meter_type, meterIndex, rate);
					return -1;
				}
			}
			else
			{
				AUG_PRT("rt_rate_shareMeterType_add FAIL ! (meter_type=%d)\n", rtk_qos_share_meter_table[tableIndex].meter_type);
				return -1;
			}
		}
	}

	return -1;
}

int rtk_qos_share_meter_get(rtk_rate_application_type_t application_type, unsigned int index, unsigned int rate)
{
	unsigned int meterIndex_f, tableIndex,rate_f;
	char line[1024], alisString_f[512], alisString[512];
	FILE *fp;
	for(tableIndex=RTK_APP_TYPE_IP_QOS_RATE_SHAPING ; tableIndex<RTK_APP_TYPE_MAX ; tableIndex++)
	{
		if(rtk_qos_share_meter_table[tableIndex].application_type == application_type)
		{
			if(!(fp = fopen(FC_SHARE_METER_INDEX_FILE, "r")))
				return -2;
			while(fgets(line, 1023, fp) != NULL)
			{
				sscanf(line, "alis_name=%s meter_index=%d rate=%u\n", &alisString_f[0], &meterIndex_f, &rate_f);
				sprintf(alisString, "%s_%d", rtk_qos_share_meter_table[tableIndex].alias_name_prefix, index);
				if(!strcmp(alisString, alisString_f))
				{
					AUG_PRT("rt_rate_shareMeterType_get already exist !\n");
					fclose(fp);
					return meterIndex_f;
				}
			}
			fclose(fp);
		}
	}
	return -1;
}

int rtk_qos_share_meter_delete(rtk_rate_application_type_t application_type, unsigned int index)
{
	unsigned int meterIndex_f, tableIndex, rate;
	char line[1024], alisString_f[512], alisString[512];
	FILE *fp, *fp_tmp;

	for(tableIndex=RTK_APP_TYPE_IP_QOS_RATE_SHAPING ; tableIndex<RTK_APP_TYPE_MAX ; tableIndex++)
	{
		if(rtk_qos_share_meter_table[tableIndex].application_type == application_type)
		{
			if(!(fp = fopen(FC_SHARE_METER_INDEX_FILE, "r")))
				return -2;

			if(!(fp_tmp = fopen(FC_SHARE_METER_INDEX_TMP_FILE, "w"))){
				fclose(fp);
				return -2;
			}

			while(fgets(line, 1023, fp) != NULL)
			{
				sscanf(line, "alis_name=%s meter_index=%d rate=%u\n", &alisString_f[0], &meterIndex_f, &rate);
				sprintf(alisString, "%s_%d", rtk_qos_share_meter_table[tableIndex].alias_name_prefix, index);
				if(!strcmp(alisString, alisString_f))
				{
					if(rt_rate_shareMeterType_del(meterIndex_f) != RT_ERR_OK)
					{
						AUG_PRT("rt_rate_shareMeterType_del FAIL !\n");
					}
				}
				else
				{
					fprintf(fp_tmp, "alis_name=%s meter_index=%d rate=%u\n", alisString_f, meterIndex_f, rate);
				}
			}

			fclose(fp);
			fclose(fp_tmp);
			unlink(FC_SHARE_METER_INDEX_FILE);
			if(rename(FC_SHARE_METER_INDEX_TMP_FILE, FC_SHARE_METER_INDEX_FILE) != 0)
			{
				printf("Unable to rename the file: %s %d\n", __func__, __LINE__);
			}
		}
	}
	return 0;
}

int rtk_qos_share_meter_flush(rtk_rate_application_type_t application_type)
{
	unsigned int meterIndex_f, tableIndex, rate;
	char line[1024], alisString_f[512];
	FILE *fp, *fp_tmp;

	for(tableIndex=RTK_APP_TYPE_IP_QOS_RATE_SHAPING ; tableIndex<RTK_APP_TYPE_MAX ; tableIndex++)
	{
		if(rtk_qos_share_meter_table[tableIndex].application_type == application_type)
		{
			if(!(fp = fopen(FC_SHARE_METER_INDEX_FILE, "r")))
				return -2;

			if(!(fp_tmp = fopen(FC_SHARE_METER_INDEX_TMP_FILE, "w"))){
				fclose(fp);
				return -2;
			}

			while(fgets(line, 1023, fp) != NULL)
			{
				sscanf(line, "alis_name=%s meter_index=%d rate=%u\n", &alisString_f[0], &meterIndex_f, &rate);
				if(strstr(alisString_f, rtk_qos_share_meter_table[tableIndex].alias_name_prefix))
				{
					if(rt_rate_shareMeterType_del(meterIndex_f) != RT_ERR_OK)
					{
						AUG_PRT("rt_rate_shareMeterType_del FAIL !\n");
					}
				}
				else
				{
					fprintf(fp_tmp, "alis_name=%s meter_index=%d rate=%u\n", alisString_f, meterIndex_f, rate);
				}
			}

			fclose(fp);
			fclose(fp_tmp);
			unlink(FC_SHARE_METER_INDEX_FILE);
			if(rename(FC_SHARE_METER_INDEX_TMP_FILE, FC_SHARE_METER_INDEX_FILE) != 0)
			{
				printf("Unable to rename the file: %s %d\n", __func__, __LINE__);
			}
		}
	}
	return 0;
}

int rtk_qos_share_meter_update_rate(rtk_rate_application_type_t application_type, unsigned int index, unsigned int rate)
{
	unsigned int meterIndex_f, tableIndex, rate_f;
	char line[1024], alisString_f[512], alisString[512];
	FILE *fp, *fp_tmp;

	for(tableIndex=RTK_APP_TYPE_IP_QOS_RATE_SHAPING ; tableIndex<RTK_APP_TYPE_MAX ; tableIndex++)
	{
		if(rtk_qos_share_meter_table[tableIndex].application_type == application_type)
		{
			if(!(fp = fopen(FC_SHARE_METER_INDEX_FILE, "r")))
				return -2;

			if(!(fp_tmp = fopen(FC_SHARE_METER_INDEX_TMP_FILE, "w"))){
				fclose(fp);
				return -2;
			}

			while(fgets(line, 1023, fp) != NULL)
			{
				sscanf(line, "alis_name=%s meter_index=%d rate=%u\n", &alisString_f[0], &meterIndex_f, &rate_f);
				sprintf(alisString, "%s_%d", rtk_qos_share_meter_table[tableIndex].alias_name_prefix, index);
				if(!strcmp(alisString, alisString_f))
				{
					if(rt_rate_shareMeterRate_set(meterIndex_f, rate) != RT_ERR_OK)
					{
						AUG_PRT("rt_rate_shareMeterType_del FAIL !\n");
					}
					fprintf(fp_tmp, "alis_name=%s meter_index=%d rate=%u\n", alisString_f, meterIndex_f, rate);
				}
				else
				{
					fprintf(fp_tmp, "alis_name=%s meter_index=%d rate=%u\n", alisString_f, meterIndex_f, rate_f);
				}
			}

			fclose(fp);
			fclose(fp_tmp);
			unlink(FC_SHARE_METER_INDEX_FILE);
			if(rename(FC_SHARE_METER_INDEX_TMP_FILE, FC_SHARE_METER_INDEX_FILE) != 0)
			{
				printf("Unable to rename the file: %s %d\n", __func__, __LINE__);
			}
		}
	}
	return 0;
}

#if 0
int rtk_fc_loop_detection_acl_drop_set(void)
{
#if defined(CONFIG_USER_RTK_LBD)
	rt_acl_filterAndQos_t aclRule;
	int acl_idx=0, ret=0;
	FILE *fp=NULL;
	unsigned short ether_type=0x9000;

	if(!(fp = fopen(FC_LOOP_DETECTION_ACL_INDEX_FILE, "a")))
	{
		fprintf(stderr, "ERROR! %s\n", strerror(errno));
		return -2;
	}

	/* loopback eth type = 0x9000 */
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_HIGH;
	aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	//include all lan port
	aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
	aclRule.filter_fields |= RT_ACL_INGRESS_ETHERTYPE_BIT;
	mib_get_s(MIB_LBD_ETHER_TYPE, &ether_type, sizeof(ether_type));
	aclRule.ingress_ethertype = ether_type;
	aclRule.ingress_ethertype_mask = 0xffff;
	aclRule.action_fields = RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;
	if((ret = rt_acl_filterAndQos_add(aclRule, &acl_idx)) == RT_ERR_OK) {
		//printf("%d\n", acl_idx);
		fprintf(fp,"%d\n", acl_idx);
		fclose(fp);
		return 0;
	} else {
		printf("[%s@%d] rt_acl_filterAndQos_add failed! (ret = %d)\n",__func__,__LINE__, ret);
		fclose(fp);
		return -1;
	}
#else
	return 0;
#endif
}
#endif

int rtk_fc_loop_detection_acl_set(void)
{
#if defined(CONFIG_USER_RTK_LBD)
	rt_acl_filterAndQos_t aclRule;
	int acl_idx=0, ret=0;
	FILE *fp=NULL;
	unsigned short ether_type=0x9000;

	if(!(fp = fopen(FC_LOOP_DETECTION_ACL_INDEX_FILE, "a")))
	{
		fprintf(stderr, "ERROR! %s\n", strerror(errno));
		return -2;
	}

	/* loopback eth type = 0x9000 */
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_HIGH;
	aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	//include all lan port
	aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
	aclRule.filter_fields |= RT_ACL_INGRESS_ETHERTYPE_BIT;
	mib_get_s(MIB_LBD_ETHER_TYPE, &ether_type, sizeof(ether_type));
	aclRule.ingress_ethertype = ether_type;
	aclRule.ingress_ethertype_mask = 0xffff;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 7;
	if((ret = rt_acl_filterAndQos_add(aclRule, &acl_idx)) == RT_ERR_OK) {
		//printf("%d\n", acl_idx);
		fprintf(fp,"%d\n", acl_idx);
		fclose(fp);
		return 0;
	} else {
		printf("[%s@%d] rt_acl_filterAndQos_add failed! (ret = %d)\n",__func__,__LINE__, ret);
		fclose(fp);
		return -1;
	}
#else
	return 0;
#endif
}

int rtk_fc_loop_detection_acl_flush(void)
{
#if defined(CONFIG_USER_RTK_LBD)
	FILE *fp=NULL;
	int acl_idx, ret=0;

	if(!(fp = fopen(FC_LOOP_DETECTION_ACL_INDEX_FILE, "r")))
		return -2;

	while(fscanf(fp, "%d\n", &acl_idx) != EOF)
	{
		if((ret = rt_acl_filterAndQos_del(acl_idx)) != RT_ERR_OK)
			printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, acl_idx, ret);
	}

	fclose(fp);
	unlink(FC_LOOP_DETECTION_ACL_INDEX_FILE);
#endif
	return 0;
}

#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
int rtk_fc_qos_ingress_control_packet_protection_config(void)
{
	LOCK_ACL();
	rtk_fc_qos_ingress_control_packet_protection_flush();
	rtk_fc_qos_ingress_control_packet_protection_set();
	UNLOCK_ACL();
	return 0;
}

int rtk_fc_qos_egress_control_packet_protection_config(void)
{
	LOCK_ACL();
	rtk_fc_qos_egress_control_packet_protection_flush();
	rtk_fc_qos_egress_control_packet_protection_set();
	UNLOCK_ACL();
	return 0;
}

int rtk_fc_itms_qos_ingress_control_packet_protection_config(void)
{
	LOCK_ACL();
	if(rtk_fc_itms_qos_ingress_control_packet_protection_flush() == 0)
		rtk_fc_itms_qos_ingress_control_packet_protection_set();
	UNLOCK_ACL();
	return 0;
}

int rtk_fc_itms_qos_egress_control_packet_protection_config(void)
{
	LOCK_ACL();
	rtk_fc_itms_qos_egress_control_packet_protection_flush();
	rtk_fc_itms_qos_egress_control_packet_protection_set();
	UNLOCK_ACL();
	return 0;
}

int rtk_fc_qos_ingress_control_packet_protection_flush(void)
{
	FILE *fp=NULL;
	int aclIdx, ret=0;

	if(!(fp = fopen(FC_INGRESS_CONTROL_PACKET_PROTECTION_FILE, "r")))
		return -2;

	while(fscanf(fp, "%d\n", &aclIdx) != EOF)
	{
		if((ret = rt_acl_filterAndQos_del(aclIdx)) != RT_ERR_OK)
			printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
	}

	fclose(fp);
	unlink(FC_INGRESS_CONTROL_PACKET_PROTECTION_FILE);
	return 0;
}
extern const char QOS_IPTABLE_US_ACTION_CHAIN[];
int rtk_fc_qos_egress_control_packet_protection_flush(void)
{
	FILE *fp=NULL;
	int aclIdx, ret=0;
#if defined(CONFIG_RTL_SMUX_DEV)
	char buf[256]={0};
	char smux_rule_alias[64]={0};
	int tags=0;

	strcpy(smux_rule_alias, "qos_egress_control_packet_protection_set");
	for(tags=0 ; tags<=SMUX_MAX_TAGS ; tags++) {
		sprintf(buf, "%s --if %s --tx --tags %d --rule-remove-alias %s+", SMUXCTL, ALIASNAME_NAS0, tags, smux_rule_alias);
		va_cmd("/bin/sh", 2, 1, "-c", buf);
	}
#endif
	//IGMP Local out
	unsigned int qid=0;
	char f_skb_mark_value[64]={0};
	qid = getIPQosQueueNumber()-1;
	sprintf(f_skb_mark_value, "0x%x/0x%x", (qid)<<SOCK_MARK_QOS_SWQID_START, SOCK_MARK_QOS_SWQID_BIT_MASK);
	va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_DEL, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "igmp", "-j",
			"MARK", "--set-mark",(char *)f_skb_mark_value);

#ifdef CONFIG_IPV6
	//MLD Local out
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "icmpv6",
		"--icmpv6-type", "130", "-j", "MARK", "--set-mark",(char *)f_skb_mark_value);
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "icmpv6",
		"--icmpv6-type", "131", "-j", "MARK", "--set-mark",(char *)f_skb_mark_value);
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "icmpv6",
		"--icmpv6-type", "132", "-j", "MARK", "--set-mark",(char *)f_skb_mark_value);
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "icmpv6",
		"--icmpv6-type", "143", "-j", "MARK", "--set-mark",(char *)f_skb_mark_value);
	//Router Solication
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "icmpv6",
		"--icmpv6-type", "133", "-j", "MARK", "--set-mark",(char *)f_skb_mark_value);
	//Neighbor Solication
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "icmpv6",
		"--icmpv6-type", "135", "-j", "MARK", "--set-mark",(char *)f_skb_mark_value);
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "icmpv6",
		"--icmpv6-type", "136", "-j", "MARK", "--set-mark",(char *)f_skb_mark_value);
#endif
	if(!(fp = fopen(FC_EGRESS_CONTROL_PACKET_PROTECTION_FILE, "r")))
		return 0;

	while(fscanf(fp, "%d\n", &aclIdx) != EOF)
	{
		if((ret = rt_acl_filterAndQos_del(aclIdx)) != RT_ERR_OK)
			printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
	}

	fclose(fp);
	unlink(FC_EGRESS_CONTROL_PACKET_PROTECTION_FILE);
	return 0;
}

int rtk_fc_qos_ingress_control_packet_protection_set(void)
{
	unsigned char ip6Addr[IP6_ADDR_LEN]={0}, mask[IP6_ADDR_LEN]={0};
	char MCAST_ADDR[MAC_ADDR_LEN]={0x01,0x00,0x5E,0x00,0x00,0x00};
	char MCAST_MASK[MAC_ADDR_LEN]={0xff,0xff,0xff,0x00,0x00,0x00};
	char UCAST_MASK[MAC_ADDR_LEN]={0x01,0x00,0x00,0x00,0x00,0x00};
	char BCAST_MASK[MAC_ADDR_LEN]={0xff,0xff,0xff,0xff,0xff,0xff};
	char VOIP_DROP_ADDR[MAC_ADDR_LEN]={0x00,0x00,0x00,0x00,0x00,0x00};
	char VOIP_DROP_MASK[MAC_ADDR_LEN]={0x01,0x00,0x00,0x00,0x00,0x00};
	rt_acl_filterAndQos_t aclRule;
	MIB_CE_ATM_VC_T vcEntry;
	char ifname[IFNAMSIZ];
#ifdef REMOTE_ACCESS_CTL
	MIB_CE_ACC_T accEntry;
#endif
	struct in_addr wan_ip;
	struct in_addr lan_ip;
	unsigned int aclIdx;
	int vcTotal=-1;
	FILE *fp=NULL;
	int ret, i;

	if(!(fp = fopen(FC_INGRESS_CONTROL_PACKET_PROTECTION_FILE, "a")))
	{
		fprintf(stderr, "ERROR! %s\n", strerror(errno));
		return -2;
	}

	/* ARP */
	// LAN+WAN unicast ARP Reply (Trap Priority 7)
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
	aclRule.ingress_port_mask = rtk_port_get_wan_phyMask() | rtk_port_get_all_lan_phyMask();
	// DMAC = ELAN_MAC_ADDR
	aclRule.filter_fields |= RT_ACL_INGRESS_DMAC_BIT;
	/*mib_get_s(MIB_ELAN_MAC_ADDR, (void *)&aclRule.ingress_dmac, sizeof(aclRule.ingress_dmac));
	memcpy(&aclRule.ingress_dmac_mask,UCAST_MASK,MAC_ADDR_LEN);*/;
	memcpy(&aclRule.ingress_dmac_mask,UCAST_MASK,MAC_ADDR_LEN);
	aclRule.filter_fields |= RT_ACL_INGRESS_ETHERTYPE_BIT;
	aclRule.ingress_ethertype = 0x0806;
	aclRule.ingress_ethertype_mask = 0xffff;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 7;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0) {
		fprintf(fp, "%d\n", aclIdx);
	} else {
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
	}

	// LAN ARP Request (Priority 6)
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT);
	aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
	aclRule.filter_fields |= RT_ACL_INGRESS_DMAC_BIT;
	memcpy(&aclRule.ingress_dmac,BCAST_MASK,MAC_ADDR_LEN);
	memcpy(&aclRule.ingress_dmac_mask,BCAST_MASK,MAC_ADDR_LEN);
	aclRule.filter_fields |= RT_ACL_INGRESS_ETHERTYPE_BIT;
	aclRule.ingress_ethertype = 0x0806;
	aclRule.ingress_ethertype_mask = 0xffff;
	aclRule.action_fields = (RT_ACL_ACTION_PRIORITY_GROUP_ACL_PRIORITY_BIT);
	aclRule.action_priority_group_acl_priority = 6;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0) {
		fprintf(fp, "%d\n", aclIdx);
	} else {
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
	}

	// WAN ARP Request (Priority 5)
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT);
	aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
	aclRule.filter_fields |= RT_ACL_INGRESS_DMAC_BIT;
	memcpy(&aclRule.ingress_dmac,BCAST_MASK,MAC_ADDR_LEN);
	memcpy(&aclRule.ingress_dmac_mask,BCAST_MASK,MAC_ADDR_LEN);
	aclRule.filter_fields |= RT_ACL_INGRESS_ETHERTYPE_BIT;
	aclRule.ingress_ethertype = 0x0806;
	aclRule.ingress_ethertype_mask = 0xffff;
	aclRule.action_fields = (RT_ACL_ACTION_PRIORITY_GROUP_ACL_PRIORITY_BIT);
	aclRule.action_priority_group_acl_priority = 5;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0) {
		fprintf(fp, "%d\n", aclIdx);
	} else {
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
	}
#if 0
	// WAN
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0 ; i<vcTotal ; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
			continue;

		if(vcEntry.cmode == CHANNEL_MODE_BRIDGE)
			continue;

		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_QOS_HIGH;
		aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
		aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
		aclRule.filter_fields |= RT_ACL_INGRESS_DMAC_BIT;
		memcpy(&aclRule.ingress_dmac,vcEntry.MacAddr,MAC_ADDR_LEN);
		memcpy(&aclRule.ingress_dmac_mask,UCAST_MASK,MAC_ADDR_LEN);
		aclRule.filter_fields |= RT_ACL_INGRESS_ETHERTYPE_BIT;
		aclRule.ingress_ethertype = 0x0806;
		aclRule.ingress_ethertype_mask = 0xffff;
		aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
		aclRule.action_priority_group_trap_priority = 7;
		if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0) {
			fprintf(fp, "%d\n", aclIdx);
		} else {
			printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
		}
	}
#endif
	/* DHCP */
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
	aclRule.ingress_port_mask = (rtk_port_get_wan_phyMask()|rtk_port_get_all_lan_phyMask());
	aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
	aclRule.ingress_ipv4_tagif = 1;
	aclRule.filter_fields |= RT_ACL_INGRESS_L4_SPORT_RANGE_BIT;
	aclRule.ingress_src_l4_port_start = 67;
	aclRule.ingress_src_l4_port_end = 68;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 7;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		fprintf(fp, "%d\n", aclIdx);
	else
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

	/* DHCPv6 */
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
	aclRule.ingress_port_mask = (rtk_port_get_wan_phyMask()|rtk_port_get_all_lan_phyMask());
	aclRule.filter_fields |= RT_ACL_INGRESS_ETHERTYPE_BIT;
	aclRule.ingress_ethertype = 0x86dd;//ipv6
	aclRule.filter_fields |= RT_ACL_INGRESS_IPV6_TAGIF_BIT;
	aclRule.ingress_ipv6_tagif = 1;
	aclRule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
	aclRule.ingress_dest_l4_port_start = 546;
	aclRule.ingress_dest_l4_port_end = 547;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 7;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		fprintf(fp, "%d\n", aclIdx);
	else
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	//NS ff02::1:ff00:0/104, trap to protocol stack
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
	aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
	aclRule.filter_fields |= RT_ACL_INGRESS_IPV6_TAGIF_BIT | RT_ACL_INGRESS_IPV6_DIP_BIT;
	inet_pton(PF_INET6, "ff02::1:ff00:0",(void *)ip6Addr);
	memcpy(aclRule.ingress_dest_ipv6_addr, ip6Addr, IPV6_ADDR_LEN);
	inet_pton(PF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ff00:0",(void *)mask);
	memcpy(aclRule.ingress_dest_ipv6_addr_mask, mask, IPV6_ADDR_LEN);
	aclRule.ingress_ipv6_tagif = 1;
	aclRule.filter_fields |= RT_ACL_INGRESS_L4_ICMPV6_BIT;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 7;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		fprintf(fp, "%d\n", aclIdx);
	else
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	//ff02::1 , trap to protocol stack
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
	aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
	aclRule.filter_fields |= RT_ACL_INGRESS_IPV6_DIP_BIT;
	inet_pton(PF_INET6, "ff02::1",(void *)ip6Addr);
	memcpy(aclRule.ingress_dest_ipv6_addr, ip6Addr, IPV6_ADDR_LEN);
	inet_pton(PF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",(void *)mask);
	memcpy(aclRule.ingress_dest_ipv6_addr_mask, mask, IPV6_ADDR_LEN);
	aclRule.filter_fields |= RT_ACL_INGRESS_IPV6_TAGIF_BIT;
	aclRule.ingress_ipv6_tagif = 1;
	aclRule.filter_fields |= RT_ACL_INGRESS_L4_ICMPV6_BIT;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 7;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		fprintf(fp, "%d\n", aclIdx);
	else
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

	/* PPPoE */
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
	aclRule.ingress_port_mask = (rtk_port_get_wan_phyMask()|rtk_port_get_all_lan_phyMask());
	aclRule.filter_fields |= RT_ACL_INGRESS_ETHERTYPE_BIT;
	aclRule.ingress_ethertype = 0x8863;
	aclRule.ingress_ethertype_mask = 0xffff;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 7;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		fprintf(fp, "%d\n", aclIdx);
	else
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
#if 0
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
	aclRule.ingress_port_mask = (rtk_port_get_wan_phyMask()|rtk_port_get_all_lan_phyMask());
	aclRule.filter_fields |= RT_ACL_INGRESS_ETHERTYPE_BIT;
	aclRule.ingress_ethertype = 0x8864;
	aclRule.ingress_ethertype_mask = 0xffff;
	aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
	aclRule.ingress_ipv4_tagif = 0;
	aclRule.filter_fields |= RT_ACL_INGRESS_IPV6_TAGIF_BIT;
	aclRule.ingress_ipv6_tagif = 0;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 7;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		fprintf(fp, "%d\n", aclIdx);
	else
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
#endif
	/* HTTP */
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
	aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
	aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT;
	aclRule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
	aclRule.ingress_dest_l4_port_start = 80;
	aclRule.ingress_dest_l4_port_end = 80;
	aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
	mib_get_s(MIB_ADSL_LAN_IP, (void *)&lan_ip, sizeof(lan_ip));
	aclRule.ingress_dest_ipv4_addr_start = aclRule.ingress_dest_ipv4_addr_end = ntohl(*((ipaddr_t *)&lan_ip.s_addr));
	aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
	aclRule.ingress_ipv4_tagif = 1;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 7;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		fprintf(fp, "%d\n", aclIdx);
	else
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
	aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
	aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT;
	aclRule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
	aclRule.ingress_dest_l4_port_start = 8080;
	aclRule.ingress_dest_l4_port_end = 8080;
	aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
	mib_get_s(MIB_ADSL_LAN_IP, (void *)&lan_ip, sizeof(lan_ip));
	aclRule.ingress_dest_ipv4_addr_start = aclRule.ingress_dest_ipv4_addr_end = ntohl(*((ipaddr_t *)&lan_ip.s_addr));
	aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
	aclRule.ingress_ipv4_tagif = 1;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 7;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		fprintf(fp, "%d\n", aclIdx);
	else
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

	/* WAN HTTP */
#ifdef REMOTE_ACCESS_CTL
	if (mib_chain_get(MIB_ACC_TBL, 0, (void *)&accEntry) && (accEntry.web&0x01)){
		vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
		for(i=0 ; i<vcTotal ; i++) {
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
				continue;

			if(vcEntry.cmode == CHANNEL_MODE_BRIDGE)
				continue;

			ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
			if (getInAddr(ifname, IP_ADDR, (void *)&wan_ip))
			{
				memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
				aclRule.acl_weight = WEIGHT_QOS_HIGH;
				aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
				aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
				aclRule.filter_fields |= RT_ACL_INGRESS_DMAC_BIT;
				memcpy(&aclRule.ingress_dmac,vcEntry.MacAddr,MAC_ADDR_LEN);
				memcpy(&aclRule.ingress_dmac_mask,UCAST_MASK,MAC_ADDR_LEN);
				aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT;
				aclRule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
				aclRule.ingress_dest_l4_port_start = 80;
				aclRule.ingress_dest_l4_port_end = 80;
				aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
				aclRule.ingress_dest_ipv4_addr_start = aclRule.ingress_dest_ipv4_addr_end = ntohl(*((ipaddr_t *)&wan_ip.s_addr));
				aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
				aclRule.ingress_ipv4_tagif = 1;
				aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
				aclRule.action_priority_group_trap_priority = 7;
				if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
					fprintf(fp, "%d\n", aclIdx);
				else
					printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

				memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
				aclRule.acl_weight = WEIGHT_QOS_HIGH;
				aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
				aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
				aclRule.filter_fields |= RT_ACL_INGRESS_DMAC_BIT;
				memcpy(&aclRule.ingress_dmac,vcEntry.MacAddr,MAC_ADDR_LEN);
				memcpy(&aclRule.ingress_dmac_mask,UCAST_MASK,MAC_ADDR_LEN);
				aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT;
				aclRule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
				aclRule.ingress_dest_l4_port_start = 8080;
				aclRule.ingress_dest_l4_port_end = 8080;
				aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
				aclRule.ingress_dest_ipv4_addr_start = aclRule.ingress_dest_ipv4_addr_end = ntohl(*((ipaddr_t *)&wan_ip.s_addr));
				aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
				aclRule.ingress_ipv4_tagif = 1;
				aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
				aclRule.action_priority_group_trap_priority = 7;
				if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
					fprintf(fp, "%d\n", aclIdx);
				else
					printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
			}
		}
	}
#endif

	/* IGMP */
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
	aclRule.ingress_port_mask = (rtk_port_get_wan_phyMask()|rtk_port_get_all_lan_phyMask());
	/*aclRule.filter_fields |= RT_ACL_INGRESS_DMAC_BIT;
	memcpy(&aclRule.ingress_dmac,MCAST_ADDR,MAC_ADDR_LEN);
	memcpy(&aclRule.ingress_dmac_mask,MCAST_MASK,MAC_ADDR_LEN);*/
	aclRule.filter_fields |= RT_ACL_INGRESS_L4_POROTCAL_VALUE_BIT;
	aclRule.ingress_l4_protocal = 0x2;
	aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
	aclRule.ingress_ipv4_tagif = 1;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 7;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		fprintf(fp, "%d\n", aclIdx);
	else
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

#ifdef VOIP_SUPPORT
	/* VoIP */
	unsigned int totalVoIPCfgEntry = 0;
	voipCfgParam_t VoipEntry;
	voipCfgParam_t *pCfg = NULL;
	voipCfgPortParam_t *VoIPport;
	totalVoIPCfgEntry = mib_chain_total(MIB_VOIP_CFG_TBL);
	if( totalVoIPCfgEntry > 0 ) {
		if(mib_chain_get(MIB_VOIP_CFG_TBL, 0, (void*)&VoipEntry)) {
			pCfg = &VoipEntry;
		}else {
			fprintf(stderr, "[%s %d]read voip config fail.\n",__FUNCTION__,__LINE__);
		}
	}else {
		fprintf(stderr, "[%s %d]flash do no have voip configuration.\n",__FUNCTION__,__LINE__);
	}
	if (pCfg)
	{
		VoIPport = &pCfg->ports[0];
		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_QOS_HIGH;
		aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
		aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
		aclRule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
		aclRule.ingress_dest_l4_port_start = VoIPport->sip_port;
		aclRule.ingress_dest_l4_port_end = VoIPport->sip_port;
		aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
		aclRule.ingress_ipv4_tagif = 1;
		aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
		aclRule.action_priority_group_trap_priority = 7;
		if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
			fprintf(fp, "%d\n", aclIdx);
		else
			printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_QOS_HIGH;
		aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
		aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
		aclRule.filter_fields |= RT_ACL_INGRESS_DMAC_BIT;
		memcpy(&aclRule.ingress_dmac,VOIP_DROP_ADDR,MAC_ADDR_LEN);
		memcpy(&aclRule.ingress_dmac_mask,VOIP_DROP_MASK,MAC_ADDR_LEN);
		aclRule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
		aclRule.ingress_dest_l4_port_start = VoIPport->media_port;
		aclRule.ingress_dest_l4_port_end = VoIPport->media_port+50;
		aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
		aclRule.ingress_ipv4_tagif = 1;
		aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
		aclRule.action_priority_group_trap_priority = 7;
		if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
			fprintf(fp, "%d\n", aclIdx);
		else
			printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
	}
#endif //VOIP_SUPPORT

	/* Upstream TCP SYN */
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
	aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
	aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT | RT_ACL_INGRESS_TCP_FLAGS_BIT;
	aclRule.ingress_tcp_flags = 0x2;
	aclRule.ingress_tcp_flags_mask = 0xfff;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 6;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		fprintf(fp, "%d\n", aclIdx);
	else
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

	/* Downstream TCP SYNACK */
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
	aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
	aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT | RT_ACL_INGRESS_TCP_FLAGS_BIT;
	aclRule.ingress_tcp_flags = 0x12;
	aclRule.ingress_tcp_flags_mask = 0xfff;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 6;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		fprintf(fp, "%d\n", aclIdx);
	else
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

#if defined(CONFIG_XFRM) && defined(CONFIG_USER_STRONGSWAN)
	/* IPSEC */
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
	aclRule.ingress_port_mask = (rtk_port_get_wan_phyMask());
	aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
	aclRule.ingress_ipv4_tagif = 1;
	aclRule.filter_fields |= RT_ACL_INGRESS_L4_UDP_BIT;
	aclRule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
	aclRule.ingress_dest_l4_port_start = 500;
	aclRule.ingress_dest_l4_port_end = 500;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 7;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		fprintf(fp, "%d\n", aclIdx);
	else
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
#endif

	fclose(fp);
	return 0;
}

int rtk_fc_qos_egress_control_packet_protection_set(void)
{
	//TODO: add some fine tune here if neccessary
	unsigned qid=0, mark=0;
#if defined(CONFIG_RTL_SMUX_DEV)
	//ARP Local out
	char buf[256]={0};
	char smux_rule_alias[64]={0};
	int tags=0;
	qid = getIPQosQueueNumber()-1;
	mark = ((qid)<<SOCK_MARK_QOS_SWQID_START);
	strcpy(smux_rule_alias, "qos_egress_control_packet_protection_set");
	//smuxctl --if nas0 --tx --tags 1 --filter-ethertype 0x0806 --set-skb-mark 0x70 0x70 --rule-priority 10000 --rule-append
	for(tags=0 ; tags<=SMUX_MAX_TAGS ; tags++) {
		sprintf(buf, "%s --if %s --tx --tags %d --filter-ethertype 0x0806 --set-skb-mark 0x%x 0x%x --rule-priority %d --rule-alias %s --rule-append",
		SMUXCTL, ALIASNAME_NAS0, tags, SOCK_MARK_QOS_SWQID_BIT_MASK, mark, SMUX_RULE_PRIO_QOS_HIGH, smux_rule_alias);
		va_cmd("/bin/sh", 2, 1, "-c", buf);
	}
#endif
	//IGMP Local out
	char f_skb_mark_value[64]={0};
	qid = getIPQosQueueNumber()-1;
	sprintf(f_skb_mark_value, "0x%x/0x%x", (qid)<<SOCK_MARK_QOS_SWQID_START, SOCK_MARK_QOS_SWQID_BIT_MASK);
	va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "igmp", "-j",
			"MARK", "--set-mark",(char *)f_skb_mark_value);
#ifdef CONFIG_IPV6
	//MLD Local out
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "icmpv6",
		"--icmpv6-type", "130", "-j", "MARK", "--set-mark",(char *)f_skb_mark_value);
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "icmpv6",
		"--icmpv6-type", "131", "-j", "MARK", "--set-mark",(char *)f_skb_mark_value);
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "icmpv6",
		"--icmpv6-type", "132", "-j", "MARK", "--set-mark",(char *)f_skb_mark_value);
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "icmpv6",
		"--icmpv6-type", "143", "-j", "MARK", "--set-mark",(char *)f_skb_mark_value);
	//Router Solication
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "icmpv6",
		"--icmpv6-type", "133", "-j", "MARK", "--set-mark",(char *)f_skb_mark_value);
	//Neighbor Solication
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "icmpv6",
		"--icmpv6-type", "135", "-j", "MARK", "--set-mark",(char *)f_skb_mark_value);
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-p", "icmpv6",
		"--icmpv6-type", "136", "-j", "MARK", "--set-mark",(char *)f_skb_mark_value);
#endif
	return 0;
}
extern const char QOS_IPTABLE_US_ACTION_CHAIN[];
int rtk_fc_itms_qos_ingress_control_packet_protection_flush(void)
{
	char acsurl[256+1] = {0}, ITMS_Server[256] = {0};
	unsigned int ITMS_Server_Port;
	unsigned int Conreq_Port = 0;
	struct in_addr ITMS_Server_Address;
	rt_acl_filterAndQos_t aclRule;
	struct hostent *host;
	struct in_addr acsaddr;
	FILE *fp=NULL;
	int aclIdx, ret=0;
	int aclChanged=0;

	unsigned char dynamic_acs_url_selection = 0;
	mib_get_s(CWMP_DYNAMIC_ACS_URL_SELECTION, &dynamic_acs_url_selection, sizeof(dynamic_acs_url_selection));
	int mib_id = CWMP_ACS_URL;
	if (dynamic_acs_url_selection)
	{
		mib_id = RS_CWMP_USED_ACS_URL;
	}

	if(!(fp = fopen(FC_ITMS_INGRESS_CONTROL_PACKET_PROTECTION_FILE, "r")))
		return 0;

	while(fscanf(fp, "%d\n", &aclIdx) != EOF)
	{
		if((ret = rt_acl_filterAndQos_get(aclIdx, &aclRule)) == RT_ERR_OK)
		{
			if(!mib_get_s(mib_id, (void*)acsurl, sizeof(acsurl)))
			{
				fprintf(stderr, "<%s %d> Get mib value CWMP_ACS_URL failed!\n",__func__,__LINE__);
				fclose(fp);
				return -1;
			}

			set_urlport(acsurl, ITMS_Server, &ITMS_Server_Port);

			if(!isIPAddr(ITMS_Server)) {
				if(!(host = gethostbyname(ITMS_Server)))
				{
					fprintf(stderr, "ACS URL gethostbyname failed!\n");
					fclose(fp);
					return -1;
				}

				memcpy((char *) &(acsaddr.s_addr), host->h_addr_list[0], host->h_length);
				strcpy(ITMS_Server, (char*)inet_ntoa(acsaddr));
			}

			if(inet_pton(AF_INET, ITMS_Server, &ITMS_Server_Address) != 1)
			{
				fprintf(stderr, "get ITMS_Server_Address failed!\n");
				fclose(fp);
				return -1;
			}

			if(!mib_get_s(CWMP_CONREQ_PORT, (void *)&Conreq_Port, sizeof(Conreq_Port)))
			{
				fprintf(stderr, "<%s %d> Get mib value CWMP_CONREQ_PORT failed!\n",__func__,__LINE__);
				fclose(fp);
				return -1;
			}

			if (Conreq_Port == 0)
				Conreq_Port = 7547;

			if(ntohl(ITMS_Server_Address.s_addr) != aclRule.ingress_src_ipv4_addr_start
				|| (aclRule.filter_fields&RT_ACL_INGRESS_L4_SPORT_RANGE_BIT && ITMS_Server_Port != aclRule.ingress_src_l4_port_start))
				aclChanged = 1;
			else if(ntohl(ITMS_Server_Address.s_addr) != aclRule.ingress_src_ipv4_addr_start
				|| (aclRule.filter_fields&RT_ACL_INGRESS_L4_DPORT_RANGE_BIT && Conreq_Port != aclRule.ingress_dest_l4_port_start))
				aclChanged = 1;

		}
		else
			printf("[%s@%d] rt_acl_filterAndQos_get failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
	}

	fclose(fp);

	if(aclChanged)
	{
		if(!(fp = fopen(FC_ITMS_INGRESS_CONTROL_PACKET_PROTECTION_FILE, "r")))
			return 0;

		while(fscanf(fp, "%d\n", &aclIdx) != EOF)
		{
			if((ret = rt_acl_filterAndQos_get(aclIdx, &aclRule)) == RT_ERR_OK)
			{
				if((ret = rt_acl_filterAndQos_del(aclIdx)) != RT_ERR_OK)
					printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
			}
			else
				printf("[%s@%d] rt_acl_filterAndQos_get failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
		}

		fclose(fp);
	}
	else
	{
		return -1;
	}

	unlink(FC_ITMS_INGRESS_CONTROL_PACKET_PROTECTION_FILE);
	return 0;
}

int rtk_fc_itms_qos_egress_control_packet_protection_flush(void)
{
	int atmVcEntryNum, i;
	MIB_CE_ATM_VC_T atmVcEntry;
	char cmdbuf[255]={0};
	char inf[IFNAMSIZ];
	unsigned int qid=0;
	char f_skb_mark_value[64]={0};
	qid = getIPQosQueueNumber()-1;
	sprintf(f_skb_mark_value, "0x%x/0x%x", (qid)<<SOCK_MARK_QOS_SWQID_START, SOCK_MARK_QOS_SWQID_BIT_MASK);

	atmVcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<atmVcEntryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&atmVcEntry))
		{
			printf("error get atm vc entry\n");
			continue;
		}
		if (atmVcEntry.applicationtype & X_CT_SRV_TR069)
		{
			ifGetName(atmVcEntry.ifIndex, inf, sizeof(inf));
			//bin/iptables -t mangle -A POSTROUTING -o %s  -j MARK --set-mark 0x170/0x1fff
			va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_DEL, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-o", (char*)inf, "-j",
				"MARK", "--set-mark",(char *)f_skb_mark_value);
		}
	}
	return 0;
}

int rtk_fc_itms_qos_ingress_control_packet_protection_set(void)
{
	char acsurl[256+1] = {0}, ITMS_Server[256] = {0};
	unsigned int ITMS_Server_Port;
	unsigned int Conreq_Port = 0;
	struct in_addr ITMS_Server_Address;
	rt_acl_filterAndQos_t aclRule;
	struct in_addr lan_ip;
	struct hostent *host;
	struct in_addr acsaddr;
	int i,aclIdx=0, ret;
	FILE *fp;

	unsigned char dynamic_acs_url_selection = 0;
	mib_get_s(CWMP_DYNAMIC_ACS_URL_SELECTION, &dynamic_acs_url_selection, sizeof(dynamic_acs_url_selection));
	int mib_id = CWMP_ACS_URL;
	if (dynamic_acs_url_selection)
	{
		mib_id = RS_CWMP_USED_ACS_URL;
	}
  	int aclAdded = 0;

	if(!(fp = fopen(FC_ITMS_INGRESS_CONTROL_PACKET_PROTECTION_FILE, "a")))
	{
		fprintf(stderr, "ERROR! %s\n", strerror(errno));
		return -1;
	}

	/* TR069 */
	if(!mib_get_s(mib_id, (void*)acsurl, sizeof(acsurl)))
	{
		fprintf(stderr, "<%s %d> Get mib value CWMP_ACS_URL failed!\n",__func__,__LINE__);
		fclose(fp);
		unlink(FC_ITMS_INGRESS_CONTROL_PACKET_PROTECTION_FILE);
		return -1;
	}

	set_urlport(acsurl, ITMS_Server, &ITMS_Server_Port);

	if(!isIPAddr(ITMS_Server)) {
		if(!(host = gethostbyname(ITMS_Server)))
		{
			fprintf(stderr, "ACS URL gethostbyname failed!\n");
			fclose(fp);
			unlink(FC_ITMS_INGRESS_CONTROL_PACKET_PROTECTION_FILE);
			return -1;
		}

		memcpy((char *) &(acsaddr.s_addr), host->h_addr_list[0], host->h_length);
		strcpy(ITMS_Server, (char*)inet_ntoa(acsaddr));
	}

	if(!mib_get_s(CWMP_CONREQ_PORT, (void *)&Conreq_Port, sizeof(Conreq_Port)))
	{
		fprintf(stderr, "<%s %d> Get mib value CWMP_CONREQ_PORT failed!\n",__func__,__LINE__);
		fclose(fp);
		unlink(FC_ITMS_INGRESS_CONTROL_PACKET_PROTECTION_FILE);
		return -1;
	}

	if (Conreq_Port == 0)
		Conreq_Port = 7547;

	if(isIPAddr(ITMS_Server) && (inet_pton(AF_INET, ITMS_Server, &ITMS_Server_Address) == 1))
	{
		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_QOS_HIGH;
		aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
		aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
		aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_SIP_RANGE_BIT;
		aclRule.ingress_src_ipv4_addr_start = aclRule.ingress_src_ipv4_addr_end = ntohl(ITMS_Server_Address.s_addr);
		aclRule.filter_fields |= RT_ACL_INGRESS_L4_SPORT_RANGE_BIT;
		aclRule.ingress_src_l4_port_start = aclRule.ingress_src_l4_port_end = ITMS_Server_Port;
		aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
		aclRule.ingress_ipv4_tagif = 1;
		aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
		aclRule.action_priority_group_trap_priority = 7;
		if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		{
			aclAdded = 1;
			fprintf(fp, "%d\n", aclIdx);
		}
		else
			printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_QOS_HIGH;
		aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
		aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
		//aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_SIP_RANGE_BIT;
		//aclRule.ingress_src_ipv4_addr_start = aclRule.ingress_src_ipv4_addr_end = ntohl(ITMS_Server_Address.s_addr);
		aclRule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
		aclRule.ingress_dest_l4_port_start = aclRule.ingress_dest_l4_port_end = Conreq_Port;
		aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
		aclRule.ingress_ipv4_tagif = 1;
		aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
		aclRule.action_priority_group_trap_priority = 7;
		if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		{
			aclAdded = 1;
			fprintf(fp, "%d\n", aclIdx);
		}
		else
			printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
	}

	fclose(fp);
	if (!aclAdded)
		unlink(FC_ITMS_INGRESS_CONTROL_PACKET_PROTECTION_FILE);

	return 0;
}

int rtk_fc_itms_qos_egress_control_packet_protection_set(void)
{
	//TODO: add some fine tune here if neccessary
	int atmVcEntryNum, i;
	MIB_CE_ATM_VC_T atmVcEntry;
	char cmdbuf[255]={0};
	char inf[IFNAMSIZ];
	unsigned int qid=0;
	char f_skb_mark_value[64]={0};
	qid = getIPQosQueueNumber()-1;
	sprintf(f_skb_mark_value, "0x%x/0x%x", (qid)<<SOCK_MARK_QOS_SWQID_START, SOCK_MARK_QOS_SWQID_BIT_MASK);

	atmVcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<atmVcEntryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&atmVcEntry))
		{
			printf("error get atm vc entry\n");
			continue;
		}
		if (atmVcEntry.applicationtype & X_CT_SRV_TR069)
		{
			 ifGetName(atmVcEntry.ifIndex, inf, sizeof(inf));
			//bin/iptables -t mangle -A POSTROUTING -o %s  -j MARK --set-mark 0x170/0x1fff
			va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, (char *)QOS_IPTABLE_US_ACTION_CHAIN, "-o", (char*)inf, "-j",
				"MARK", "--set-mark",(char *)f_skb_mark_value);
		}
	}
	return 0;
}
#endif

#ifdef CONFIG_USER_WIRED_8021X
int rtk_fc_wired_8021x_clean(int isPortNeedDownUp)
{
	unsigned char wired8021xEnable[MAX_LAN_PORT_NUM] = {0};
#ifdef CONFIG_COMMON_RT_API
	rt_port_linkStatus_t linkStatus;
	rt_action_t fwdAction;
	unsigned int data;
#endif
	int aclIdx, ret=0;
	FILE *fp=NULL;
	int i;

	printf("%s %d: enter\n", __func__, __LINE__);

	system("echo 0 0x0 > proc/fc/ctrl/l2_macEgrLearning");

	if(!mib_get(MIB_WIRED_8021X_ENABLE, (void *)wired8021xEnable))
	{
		printf("%s %d: get mib MIB_WIRED_8021X_ENABLE FAIL.\n", __func__, __LINE__);
		return -1;
	}

#ifdef CONFIG_COMMON_RT_API
	for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
	{
		fwdAction = ACTION_FORWARD;
		ret = rt_l2_newMacOp_set(rtk_port_get_lan_phyID(i), fwdAction);
		if(ret != RT_ERR_OK)
		{
			printf("Error! rt_l2_newMacOp_set Faile, ret=%d.\n", ret);
			return -1;
		}

		// Down up to flush L2 entries
		if(!isPortNeedDownUp || rt_port_link_get(rtk_port_get_lan_phyID(i), &linkStatus) || !wired8021xEnable[i])
			continue;

		if(linkStatus == RT_PORT_LINKUP)
		{
			if(rt_port_phyReg_get(rtk_port_get_lan_phyID(i), 0, 0, &data) == 0)
			{
				data |= (1<<11); // power down
				//diag rt_port set phy-reg port 2 page 0 register 0 data 0x1940
				//printf("%s %d: port=%d DOWN\n", __func__, __LINE__, lanport);
				rt_port_phyReg_set(rtk_port_get_lan_phyID(i), 0, 0, data);
				data &= ~(1<<11); // normal operarion
				//diag rt_port set phy-reg port 2 page 0 register 0 data 0x1140
				//printf("%s %d: port=%d UP\n", __func__, __LINE__, lanport);
				rt_port_phyReg_set(rtk_port_get_lan_phyID(i), 0, 0, data);
			}
			else
				continue;
		}
	}
#endif

	if(!(fp = fopen(FC_WIRED_8021X_DEFAULT_ACL_FILE, "r")))
		return -1;

	while(fscanf(fp, "%d\n", &aclIdx) != EOF)
	{
#ifdef CONFIG_COMMON_RT_API
		if((ret = rt_acl_filterAndQos_del(aclIdx)) != RT_ERR_OK)
			printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
#endif
	}

	fclose(fp);
	unlink(FC_WIRED_8021X_DEFAULT_ACL_FILE);
	return 0;
}

int rtk_fc_wired_8021x_setup(void)
{
	unsigned char wired8021xEnable[MAX_LAN_PORT_NUM] = {0};
	int i, ret, aclIdx=0, existWired8021xPort = 0;
	rt_acl_filterAndQos_t aclRule;
	int wired8021xPortMask = 0;
#ifdef CONFIG_COMMON_RT_API
	rt_action_t fwdAction;
#endif
	char cmd[64];
	FILE *fp;

	printf("%s %d: enter\n", __func__, __LINE__);

	if(!mib_get(MIB_WIRED_8021X_ENABLE, (void *)wired8021xEnable))
	{
		printf("%s %d: get mib MIB_WIRED_8021X_ENABLE FAIL.\n", __func__, __LINE__);
		return -1;
	}

	for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
	{
		if(wired8021xEnable[i]) {
			existWired8021xPort = 1;
			wired8021xPortMask |= (1<<i);
#ifdef CONFIG_COMMON_RT_API
			fwdAction = ACTION_DROP;
			ret = rt_l2_newMacOp_set(rtk_port_get_lan_phyID(i), fwdAction);
			if(ret != RT_ERR_OK)
			{
				printf("Error! rt_l2_newMacOp_set Faile, ret=%d.\n", ret);
				return -1;
			}
#endif
		}
	}

	if(existWired8021xPort)
	{
		if(!(fp = fopen(FC_WIRED_8021X_DEFAULT_ACL_FILE, "a")))
		{
			fprintf(stderr, "ERROR! %s\n", strerror(errno));
			return -1;
		}

#ifdef CONFIG_COMMON_RT_API
		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_QOS_HIGH;
		aclRule.filter_fields |= RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT;
		aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		aclRule.ingress_port_mask = rtk_port_get_lan_phyMask(wired8021xPortMask);
		aclRule.filter_fields |= RT_ACL_INGRESS_ETHERTYPE_BIT;
		aclRule.ingress_ethertype = 0x888e;
		aclRule.ingress_ethertype_mask = 0xffff;
		aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
		aclRule.action_priority_group_trap_priority = 7;
		if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
			fprintf(fp, "%d\n", aclIdx);
		else
			printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
#endif
		sprintf(cmd, "echo 1 0x%x > proc/fc/ctrl/l2_macEgrLearning", rtk_port_get_lan_phyMask(wired8021xPortMask));
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		sprintf(cmd, "diag l2-table set link-down-flush state enable");
		va_cmd("/bin/sh", 2, 1, "-c", cmd);

		fclose(fp);
	}
	return 0;
}

int rtk_fc_wired_8021x_set_auth(int portid, unsigned char *mac)
{
	unsigned char wired8021xMode;
	unsigned char wired8021xEnable[MAX_LAN_PORT_NUM] = {0};
	unsigned char macStr[18];
#ifdef CONFIG_COMMON_RT_API
	rt_l2_ucastAddr_t l2Addr;
	rt_action_t fwdAction;
#endif
	int ret;

	printf("%s %d: enter\n", __func__, __LINE__);

	if(portid >= SW_LAN_PORT_NUM && (mac==NULL))
	{
		printf("%s %d: invalid portid && MAC !\n", __func__, __LINE__);
		return -1;
	}

	if(!mib_get(MIB_WIRED_8021X_ENABLE, (void *)wired8021xEnable))
	{
		printf("%s %d: get mib MIB_WIRED_8021X_ENABLE FAIL.\n", __func__, __LINE__);
		return -1;
	}

	if(mib_get(MIB_WIRED_8021X_MODE, (void *)&wired8021xMode) != 0)
	{
		FILE *fp = NULL;

		if(mac==NULL)
		{
			if(wired8021xMode != WIRED_8021X_MODE_PORT_BASED)
			{
				printf("%s %d: wired 802.1x is in MAC based mode, MAC parameter should not be NULL !\n", __func__, __LINE__);
				return -1;
			}

			if(!wired8021xEnable[portid])
			{
				printf("%s %d: wired 802.1x of this port=%d was disabled.\n", __func__, __LINE__, portid);
				return -1;
			}

			printf("%s %d: portid=%d phyPortid=%d\n", __func__, __LINE__, portid, rtk_port_get_lan_phyID(portid));

#ifdef CONFIG_COMMON_RT_API
			fwdAction = ACTION_FORWARD;
			ret = rt_l2_newMacOp_set(rtk_port_get_lan_phyID(portid), fwdAction);
			if(ret != RT_ERR_OK)
			{
				printf("Error! rt_l2_newMacOp_set Faile, ret=%d.\n", ret);
				return -1;
			}
#endif
		}
		else
		{
			if(wired8021xMode != WIRED_8021X_MODE_MAC_BASED)
			{
				printf("%s %d: wired 802.1x is in port based mode, MAC parameter should be NULL !\n", __func__, __LINE__);
				return -1;
			}

			printf("%s %d: MAC=%02X:%02X:%02X:%02X:%02X:%02X\n", __func__, __LINE__
				, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			snprintf(macStr, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

#ifdef CONFIG_COMMON_RT_API
			if(portid != -1)
			{
				l2Addr.port = rtk_port_get_lan_phyID(portid);
				memcpy(l2Addr.mac.octet, mac, MAC_ADDR_LEN);
				ret = rt_l2_addr_add(&l2Addr);
				if(ret != RT_ERR_OK)
				{
					printf("Error! rt_action_t fwdAction; Faile, ret=%d.\n", ret);
					return -1;
				}
			}
#endif
		}
	}
	else
	{
		printf("%s %d: get mib MIB_WIRED_8021X_MODE FAIL.\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

int rtk_fc_wired_8021x_set_unauth(int portid, unsigned char *mac)
{
	unsigned char wired8021xMode;
	unsigned char wired8021xEnable[MAX_LAN_PORT_NUM] = {0};
	unsigned char macStr[18];
#ifdef CONFIG_COMMON_RT_API
	rt_l2_ucastAddr_t l2Addr;
	rt_action_t fwdAction;
#endif
	int ret;

	printf("%s %d: enter\n", __func__, __LINE__);

	if(portid >= SW_LAN_PORT_NUM && (mac==NULL))
	{
		printf("%s %d: invalid portid && MAC !\n", __func__, __LINE__);
		return -1;
	}

	if(!mib_get(MIB_WIRED_8021X_ENABLE, (void *)wired8021xEnable))
	{
		printf("%s %d: get mib MIB_WIRED_8021X_ENABLE FAIL.\n", __func__, __LINE__);
		return -1;
	}

	if(mib_get(MIB_WIRED_8021X_MODE, (void *)&wired8021xMode) != 0)
	{
		FILE *fp = NULL;

		if(mac==NULL)
		{
			if(wired8021xMode != WIRED_8021X_MODE_PORT_BASED)
			{
				printf("%s %d: wired 802.1x is in MAC based mode, MAC parameter should not be NULL !\n", __func__, __LINE__);
				return -1;
			}

			if(!wired8021xEnable[portid])
			{
				printf("%s %d: wired 802.1x of this port=%d was disabled.\n", __func__, __LINE__, portid);
				return -1;
			}

			printf("%s %d: portid=%d phyPortid=%d\n", __func__, __LINE__, portid, rtk_port_get_lan_phyID(portid));

#ifdef CONFIG_COMMON_RT_API
			fwdAction = ACTION_DROP;
			ret = rt_l2_newMacOp_set(rtk_port_get_lan_phyID(portid), fwdAction);
			if(ret != RT_ERR_OK)
			{
				printf("Error! rt_l2_newMacOp_set Faile, ret=%d.\n", ret);
				return -1;
			}
#endif
		}
		else
		{
			if(wired8021xMode != WIRED_8021X_MODE_MAC_BASED)
			{
				printf("%s %d: wired 802.1x is in port based mode, MAC parameter should be NULL !\n", __func__, __LINE__);
				return -1;
			}

			printf("%s %d: MAC=%02X:%02X:%02X:%02X:%02X:%02X\n", __func__, __LINE__
				, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			snprintf(macStr, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

#ifdef CONFIG_COMMON_RT_API
			if(portid != -1)
			{
				l2Addr.port = rtk_port_get_lan_phyID(portid);
				memcpy(l2Addr.mac.octet, mac, MAC_ADDR_LEN);
				ret = rt_l2_addr_del(&l2Addr);
				if(ret != RT_ERR_OK)
				{
					printf("Error! rt_l2_addr_del Faile, ret=%d.\n", ret);
					return -1;
				}
			}
#endif
		}
	}
	else
	{
		printf("%s %d: get mib MIB_WIRED_8021X_MODE FAIL.\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

int rtk_fc_wired_8021x_flush_port(int portid)
{
	unsigned char wired8021xMode;
	unsigned char wired8021xEnable[MAX_LAN_PORT_NUM] = {0};
#ifdef CONFIG_COMMON_RT_API
	rt_action_t fwdAction;
#endif
	int ret;

	printf("%s %d: enter\n", __func__, __LINE__);

	if((portid == -1 || portid >= SW_LAN_PORT_NUM))
	{
		printf("%s %d: invalid portid !\n", __func__, __LINE__);
		return -1;
	}

	if(!mib_get(MIB_WIRED_8021X_ENABLE, (void *)wired8021xEnable))
	{
		printf("%s %d: get mib MIB_WIRED_8021X_ENABLE FAIL.\n", __func__, __LINE__);
		return -1;
	}

	if(mib_get(MIB_WIRED_8021X_MODE, (void *)&wired8021xMode) != 0)
	{
		printf("%s %d: portid=%d\n", __func__, __LINE__, portid);
		if(wired8021xMode == WIRED_8021X_MODE_PORT_BASED)
		{
#ifdef CONFIG_COMMON_RT_API
			if(wired8021xEnable[portid])
			{
				fwdAction = ACTION_DROP;
				ret = rt_l2_newMacOp_set(rtk_port_get_lan_phyID(portid), fwdAction);
				if(ret != RT_ERR_OK)
				{
					printf("Error! rt_l2_newMacOp_set Faile, ret=%d.\n", ret);
					return -1;
				}
			}
#endif
		}
		else
		{
			/* No need, but need confirm following setting enabled
	   		diag l2-table get link-down-flush state */
		}
	}

	return 0;
}
#endif

#if defined(CONFIG_USER_LANNETINFO)
int rtk_fc_update_lan_hpc_for_mib(unsigned char *mac)
{
	int hostIndex, foundHostIndex=-1, nullHostIndex=-1;
	rt_rate_host_policer_control_t nullPoliceControl = {0};
	rt_rate_host_policer_control_t policeControl;

	if(mac == NULL)
		return -1;

	if(isZeroMac(mac))
		return -1;

	for(hostIndex=0 ; hostIndex<32 ; hostIndex++)
	{
		memset(&policeControl, 0, sizeof(rt_rate_host_policer_control_t));
		if(rt_rate_hostPolicerControl_get(hostIndex, &policeControl) == RT_ERR_OK)
		{
			// search for my hostindex
			if(!memcmp(&policeControl.mac_addr, mac, MAC_ADDR_LEN))
			{
				foundHostIndex = hostIndex;
				if(policeControl.mib_count_control == ENABLED)
				{
					return 0;
				}
				break;
			}
			if(!memcmp(&policeControl, &nullPoliceControl, sizeof(rt_rate_host_policer_control_t)))
			{
				if(nullHostIndex == -1)
					nullHostIndex = hostIndex;
			}
		}
	}

	hostIndex = -1;
	if(foundHostIndex != -1)
	{
		hostIndex = foundHostIndex;
	}
	else if(nullHostIndex != -1)
	{
		hostIndex = nullHostIndex;
	}

	if(hostIndex != -1)
	{
		if(hostIndex == nullHostIndex)
		{
			memset(&policeControl, 0, sizeof(rt_rate_host_policer_control_t));
			memcpy(&policeControl.mac_addr, mac, MAC_ADDR_LEN);
		}
		else if(rt_rate_hostPolicerControl_get(hostIndex, &policeControl) != RT_ERR_OK)
		{
			AUG_PRT(" rt_rate_hostPolicerControl_set fail !\n");
			return -1;
		}

		policeControl.mib_count_control = ENABLED;
#if RG_HOST_POLICE_CONTROL_DEBUG_ENABLE
		printf(" %s %d> macAddr=%02X:%02X:%02X:%02X:%02X:%02X ingress_limit_control=%d egress_limit_control=%d mib_count_control=%d ingress_limit_meter_index=%d egress_limit_meter_index=%d hostIndex=%d\n"
			, __func__, __LINE__, policeControl.mac_addr[0], policeControl.mac_addr[1]
			, policeControl.mac_addr[2], policeControl.mac_addr[3], policeControl.mac_addr[4]
			, policeControl.mac_addr[5], policeControl.ingress_limit_control, policeControl.egress_limit_control
			, policeControl.mib_count_control, policeControl.ingress_limit_meter_index, policeControl.egress_limit_meter_index, hostIndex);
#endif
		if(rt_rate_hostPolicerControl_set(hostIndex, policeControl) != RT_ERR_OK)
		{
			AUG_PRT(" rt_rate_hostPolicerControl_set fail !\n");
			return -1;
		}
	}
	else
	{
		return -1;
	}
	return 0;
}

int rtk_fc_host_police_control_mib_counter_get(unsigned char *mac, LAN_HOST_POLICE_CTRL_MIB_TYPE_T type, unsigned long long *result)
{
	int hostIndex, foundHostIndex=-1;
	rt_rate_host_policer_control_t nullPoliceControl = {0};
	rt_rate_host_policer_control_t policeControl;
	rt_rate_host_policer_mib_t policeMib;
	int lockfd;

	if(mac == NULL || result == NULL)
		return -1;

	if(isZeroMac(mac))
		return -1;

	for(hostIndex=0 ; hostIndex<32 ; hostIndex++)
	{
		memset(&policeControl, 0, sizeof(rt_rate_host_policer_control_t));
		if(rt_rate_hostPolicerControl_get(hostIndex, &policeControl) == RT_ERR_OK)
		{
			// search for my hostindex
			if(!memcmp(&policeControl.mac_addr, mac, MAC_ADDR_LEN)
				&& policeControl.mib_count_control == ENABLED)
			{
				foundHostIndex = hostIndex;
				break;
			}
		}
	}

	if(foundHostIndex != -1)
	{
		memset(&policeMib, 0, sizeof(rt_rate_host_policer_mib_t));
		if(rt_rate_hostPolicerMib_get(hostIndex, &policeMib) == RT_ERR_OK)
		{
			switch(type)
			{
				case MIB_TYPE_RX_BYTES:
					*result = policeMib.tx_count;
					break;
				case MIB_TYPE_TX_BYTES:
					*result = policeMib.rx_count;
					break;
				default:
					AUG_PRT(" Invalid MIB type = %d\n", type);
					return -1;
			}
		}
		else
		{
			AUG_PRT(" rtk_rg_hostPoliceLogging_get fail ! \n");
			return -1;
		}
	}
	else
	{
		/*AUG_PRT(" RG host police control table entry not found ! (macAddr=%02X:%02X:%02X:%02X:%02X:%02X)\n"
			, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);*/
		return -1;
	}

	return 0;
}
#endif
#endif

int rtk_fc_get_empty_host_policer_by_id(int hostIndex)
{
	rt_rate_host_policer_control_t PoliceControl = {0};

	memset(&PoliceControl, 0, sizeof(rt_rate_host_policer_control_t));
	if(rt_rate_hostPolicerControl_get(hostIndex, &PoliceControl) == RT_ERR_OK)
	{
		// search for my hostindex
		//printf("Orz [%s:%d] hostIndex =%d mac_addr=%x:%x:%x:%x:%x:%x\n",
		//__FUNCTION__, __LINE__, hostIndex, PoliceControl.mac_addr[0],PoliceControl.mac_addr[1],PoliceControl.mac_addr[2],
		//PoliceControl.mac_addr[3],PoliceControl.mac_addr[4],PoliceControl.mac_addr[5]);
		if(memcmp(&PoliceControl.mac_addr, EMPTY_MAC, MAC_ADDR_LEN) == 0)
		{
			return 1;
		}
		else {
			return 0;
		}
	}
	return 0;
}


int rtk_fc_get_host_policer_by_mac(unsigned char* mac)
{
	int hostIndex=-1;
	rt_rate_host_policer_control_t PoliceControl = {0};

	for(hostIndex=0 ; hostIndex<32 ; hostIndex++)
	{
		memset(&PoliceControl, 0, sizeof(rt_rate_host_policer_control_t));
		if(rt_rate_hostPolicerControl_get(hostIndex, &PoliceControl) == RT_ERR_OK)
		{
			// search for my hostindex
			//printf("Orz [%s:%d] hostIndex =%d mac_addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
			//__FUNCTION__, __LINE__, hostIndex, PoliceControl.mac_addr[0],PoliceControl.mac_addr[1],PoliceControl.mac_addr[2],
			//PoliceControl.mac_addr[3],PoliceControl.mac_addr[4],PoliceControl.mac_addr[5]);
			if(memcmp(&PoliceControl.mac_addr, mac, MAC_ADDR_LEN) == 0)
			{
				return hostIndex;
			}
		}
	}
	return -1;
}

#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL
int rtk_fc_lanhost_rate_limit_us_trap_acl_flush(void)
{
	//ebtables -t nat -D PREROUTING -j lanhost_ratelimit_preroute
	va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", (char *)FW_LANHOST_RATE_LIMIT_PREROUTING);
	//ebtables -t nat -X lanhost_ratelimit_preroute
	va_cmd(EBTABLES, 4, 1, "-t", "nat", "-X", (char *)FW_LANHOST_RATE_LIMIT_PREROUTING);
	return 0;
}

int rtk_fc_lanhost_rate_limit_ds_trap_acl_flush(void)
{
	//ebtables -t nat -D POSTROUTING -j lanhost_ratelimit_postroute
	va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_POSTROUTING, "-j", (char *)FW_LANHOST_RATE_LIMIT_POSTROUTING);
	//ebtables -t nat -X lanhost_ratelimit_postroute
	va_cmd(EBTABLES, 4, 1, "-t", "nat", "-X", (char *)FW_LANHOST_RATE_LIMIT_POSTROUTING);
	return 0;
}

int rtk_fc_lanhost_rate_limit_us_trap_acl_set(unsigned char *mac)
{
#ifdef CONFIG_RTK_SKB_MARK2
	unsigned char macStr[18];
	unsigned char mark[18];

	sprintf(mark, "0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);
	snprintf(macStr, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
					mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	va_cmd(EBTABLES, 10, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_LANHOST_RATE_LIMIT_PREROUTING, "-s", macStr, "-j", "mark2", "--mark2-or", mark);
#endif
	return 0;
}

int rtk_fc_lanhost_rate_limit_ds_trap_acl_set(unsigned char *mac)
{
#ifdef CONFIG_RTK_SKB_MARK2
	unsigned char macStr[18];
	unsigned char mark[18];

	sprintf(mark, "0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);
	snprintf(macStr, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
					mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	va_cmd(EBTABLES, 10, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_LANHOST_RATE_LIMIT_POSTROUTING, "-d", macStr, "-j", "mark2", "--mark2-or", mark);
#endif
	return 0;
}

int rtk_fc_lanhost_rate_limit_us_trap_acl_setup(void)
{
	//ebrables -t nat -N lanhost_ratelimit_preroute
	va_cmd(EBTABLES, 4, 1, "-t", "nat", "-N", (char *)FW_LANHOST_RATE_LIMIT_PREROUTING);
	//ebtables -t nat -P lanhost_ratelimit_preroute RETURN
	va_cmd(EBTABLES, 5, 1, "-t", "nat", "-P", (char *)FW_LANHOST_RATE_LIMIT_PREROUTING, FW_RETURN);
	//ebrables -t nat -I PREROUTING -j lanhost_ratelimit_preroute
	va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-j", (char *)FW_LANHOST_RATE_LIMIT_PREROUTING);

	return 0;
}

int rtk_fc_lanhost_rate_limit_ds_trap_acl_setup(void)
{
	//ebrables -t nat -N lanhost_ratelimit_postroute
	va_cmd(EBTABLES, 4, 1, "-t", "nat", "-N", (char *)FW_LANHOST_RATE_LIMIT_POSTROUTING);
	//ebtables -t nat -P lanhost_ratelimit_postroute RETURN
	va_cmd(EBTABLES, 5, 1, "-t", "nat", "-P", (char *)FW_LANHOST_RATE_LIMIT_POSTROUTING, FW_RETURN);
	//ebrables -t nat -I PREROUTING -j lanhost_ratelimit_postroute
	va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_POSTROUTING, "-j", (char *)FW_LANHOST_RATE_LIMIT_POSTROUTING);

	return 0;
}

int rtk_fc_lanhost_rate_limit_us_flush(void)
{
	rt_rate_host_policer_control_t hostPolicerControl;
	unsigned int hostPolicerControlIndex = 0;
	int ret;

	while(rt_rate_hostPolicerControl_get(hostPolicerControlIndex, &hostPolicerControl) == RT_ERR_OK)
	{
		if (!hostPolicerControl.ingress_limit_control
			&& !hostPolicerControl.ingress_limit_meter_index
			&& !hostPolicerControl.ingress_limit_rate)
		{
			hostPolicerControlIndex++;
			continue;
		}

		hostPolicerControl.ingress_limit_control = 0;
		hostPolicerControl.ingress_limit_meter_index = 0;
		hostPolicerControl.ingress_limit_rate = 0;

		ret = rt_rate_hostPolicerControl_set(hostPolicerControlIndex, hostPolicerControl);

		if(ret != RT_ERR_OK)
		{
			AUG_PRT("rt_rate_hostPolicerControl_set FAIL.\n");
			return -1;
		}
		hostPolicerControlIndex++;
	}

	rtk_qos_share_meter_flush(RTK_APP_TYPE_LANHOST_US_RATE_LIMIT_BY_FC);

	return 0;
}

int rtk_fc_lanhost_rate_limit_ds_flush(void)
{
	rt_rate_host_policer_control_t hostPolicerControl;
	unsigned int hostPolicerControlIndex = 0;
	int ret;

	while(rt_rate_hostPolicerControl_get(hostPolicerControlIndex, &hostPolicerControl) == RT_ERR_OK)
	{
		if (!hostPolicerControl.egress_limit_control
			&& !hostPolicerControl.egress_limit_meter_index
			&& !hostPolicerControl.egress_limit_rate)
		{
			hostPolicerControlIndex++;
			continue;
		}

		hostPolicerControl.egress_limit_control = 0;
		hostPolicerControl.egress_limit_meter_index = 0;
		hostPolicerControl.egress_limit_rate = 0;

		ret = rt_rate_hostPolicerControl_set(hostPolicerControlIndex, hostPolicerControl);

		if(ret != RT_ERR_OK)
		{
			AUG_PRT("rt_rate_hostPolicerControl_set FAIL.\n");
			return -1;
		}
		hostPolicerControlIndex++;
	}

	rtk_qos_share_meter_flush(RTK_APP_TYPE_LANHOST_DS_RATE_LIMIT_BY_FC);

	return 0;
}

int rtk_fc_lanhost_upstream_bandwidth_control_set(unsigned char *mac, unsigned int rate)
{
	rt_rate_host_policer_control_t hostPolicerControl;
	unsigned int hostPolicerControlIndex;
	int meterIndex, ret;

	hostPolicerControlIndex = rtk_lanhost_get_host_index_by_mac(mac);
	if(hostPolicerControlIndex == -1)
	{
		AUG_PRT("Invalid HOST.\n");
		return -1;
	}

	meterIndex = rtk_qos_share_meter_set(RTK_APP_TYPE_LANHOST_US_RATE_LIMIT_BY_FC, hostPolicerControlIndex, rate);
	if(meterIndex != -1)
	{
		memset(&hostPolicerControl, 0, sizeof(rt_rate_host_policer_control_t));
		ret = rt_rate_hostPolicerControl_get(hostPolicerControlIndex, &hostPolicerControl);

		if(ret != RT_ERR_OK)
		{
			AUG_PRT("rt_rate_hostPolicerControl_get FAIL.\n");
			return -1;
		}

		memcpy(&hostPolicerControl.mac_addr, mac, MAC_ADDR_LEN);
		hostPolicerControl.ingress_limit_control = 1;
		hostPolicerControl.ingress_limit_meter_index = meterIndex;
		hostPolicerControl.ingress_limit_rate = RT_RATE_HOST_METER_RATE_UNCHANGE;
		ret = rt_rate_hostPolicerControl_set(hostPolicerControlIndex, hostPolicerControl);

		if(ret != RT_ERR_OK)
		{
			printf("[%s %d]rt_rate_hostPolicerControl_set FAIL.\n", __func__, __LINE__);
			return -1;
		}
	}
	return 0;
}

int rtk_fc_lanhost_downstream_bandwidth_control_set(unsigned char *mac, unsigned int rate)
{
	rt_rate_host_policer_control_t hostPolicerControl;
	unsigned int hostPolicerControlIndex;
	int meterIndex, ret;

	hostPolicerControlIndex = rtk_lanhost_get_host_index_by_mac(mac);
	if(hostPolicerControlIndex == -1)
	{
		AUG_PRT("Invalid HOST.\n");
		return -1;
	}

	meterIndex = rtk_qos_share_meter_set(RTK_APP_TYPE_LANHOST_DS_RATE_LIMIT_BY_FC, hostPolicerControlIndex, rate);
	if(meterIndex != -1)
	{
		memset(&hostPolicerControl, 0, sizeof(rt_rate_host_policer_control_t));
		ret = rt_rate_hostPolicerControl_get(hostPolicerControlIndex, &hostPolicerControl);

		if(ret != RT_ERR_OK)
		{
			AUG_PRT("rt_rate_hostPolicerControl_get FAIL.\n");
			return -1;
		}

		memcpy(&hostPolicerControl.mac_addr, mac, MAC_ADDR_LEN);
		hostPolicerControl.egress_limit_control = 1;
		hostPolicerControl.egress_limit_meter_index = meterIndex;
		hostPolicerControl.egress_limit_rate = RT_RATE_HOST_METER_RATE_UNCHANGE;
		ret = rt_rate_hostPolicerControl_set(hostPolicerControlIndex, hostPolicerControl);

		if(ret != RT_ERR_OK)
		{
			printf("[%s %d]rt_rate_hostPolicerControl_set FAIL.\n", __func__, __LINE__);
			return -1;
		}
	}
	return 0;
}

void rtk_fc_lanhost_rate_limit_clear(void)
{
	rtk_fc_lanhost_rate_limit_us_flush();
	rtk_fc_lanhost_rate_limit_ds_flush();
	rtk_fc_lanhost_rate_limit_us_trap_acl_flush();
	rtk_fc_lanhost_rate_limit_ds_trap_acl_flush();
	clear_lanPortRateLimitInPS();
	return;
}

void rtk_fc_lanhost_rate_limit_apply(void)
{
	int ratelimit_trap_to_ps=0; //1: limit by TC otherwise:limit by FC shortcut
	MIB_LAN_HOST_BANDWIDTH_T entry;
	unsigned char lanHostBandwidthControlEnable;
	int i, totalNum;
	int ret=0, need_trapTo_ps = 0;

	if(!mib_get_s(MIB_LANPORT_RATELIMIT_TRAP_TO_PS, (void *)&ratelimit_trap_to_ps, sizeof(ratelimit_trap_to_ps)))
	{
		printf("[%s %d]Get mib value MIB_LANPORT_RATELIMIT_TRAP_TO_PS failed!\n", __func__, __LINE__);
		return;
	}

	rtk_fc_lanhost_rate_limit_us_trap_acl_setup();
	rtk_fc_lanhost_rate_limit_ds_trap_acl_setup();

	memset(&entry, 0 , sizeof(MIB_LAN_HOST_BANDWIDTH_T));
	totalNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);
	for(i=0 ; i<totalNum ; i++)
	{
		if (!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, i, (void *)&entry))
		{
			printf("[%s %d]get LAN_HOST_BANDWIDTHl chain entry error!\n", __func__, __LINE__);
			return;
		}

		if (entry.maxUsBandwidth > ratelimit_trap_to_ps)
		{
			ret = rtk_fc_lanhost_upstream_bandwidth_control_set(entry.mac, entry.maxUsBandwidth);
			if(ret == 0)
			{
				printf("[%s %d]Set rtk_fc_lanhost_upstream_bandwidth_control_set, mac=%02X:%02X:%02X:%02X:%02X:%02X, rate=%d\n", __func__, __LINE__, entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5], entry.maxUsBandwidth);
			}
			else
			{
				printf("[%s %d]Failed to Set rtk_fc_lanhost_upstream_bandwidth_control_set, mac=%02X:%02X:%02X:%02X:%02X:%02X, rate=%d\n", __func__, __LINE__, entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5], entry.maxUsBandwidth);
			}
		}
		else
		{
			if(entry.maxUsBandwidth != 0) {
				need_trapTo_ps = 1;
				rtk_fc_lanhost_rate_limit_us_trap_acl_set(entry.mac);
			}
		}

		if (entry.maxDsBandwidth > ratelimit_trap_to_ps)
		{
			ret = rtk_fc_lanhost_downstream_bandwidth_control_set(entry.mac, entry.maxDsBandwidth);
			if(ret == 0)
			{
				printf("[%s %d]Set rtk_fc_lanhost_upstream_bandwidth_control_set, mac=%02X:%02X:%02X:%02X:%02X:%02X, rate=%d\n", __func__, __LINE__, entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5], entry.maxDsBandwidth);
			}
			else
			{
				printf("[%s %d]Failed to Set rtk_fc_lanhost_upstream_bandwidth_control_set, mac=%02X:%02X:%02X:%02X:%02X:%02X, rate=%d\n", __func__, __LINE__, entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5], entry.maxDsBandwidth);
			}
		}
		else
		{
			if(entry.maxDsBandwidth != 0) {
				need_trapTo_ps = 1;
				rtk_fc_lanhost_rate_limit_ds_trap_acl_set(entry.mac);
			}
		}
#ifdef CONFIG_CMCC
        if(entry.maxUsBandwidth == 0 && entry.maxDsBandwidth == 0)
        {
            printf("both us and ds bandwidth is zero , delete the chain\n");
            mib_chain_delete(MIB_LAN_HOST_BANDWIDTH_TBL, i);
        }
#endif
	}
	if (need_trapTo_ps)
		apply_lanPortRateLimitInPS();
	rtk_fc_flow_flush();
	return;
}
#endif

int rtk_fc_ds_tcp_syn_rate_limit_set( void )
{
#if !defined(CONFIG_CA8279_SERIES)
	//ca8277 does not support sync only pattern.
	rt_acl_filterAndQos_t aclRule;
	unsigned int aclIdx=0;
	int meter_idx, ret;
	FILE *fp;

	if(!(fp = fopen(FC_TCP_SYN_RATE_LIMIT_ACL_FILE, "a")))
	{
		fprintf(stderr, "ERROR! %s\n", strerror(errno));
		return -1;
	}

	meter_idx = rtk_qos_share_meter_get(RTK_APP_TYPE_DS_TCP_SYN_RATE_LIMIT, 0, 3000);
	if (meter_idx < 0)
		meter_idx = rtk_qos_share_meter_set(RTK_APP_TYPE_DS_TCP_SYN_RATE_LIMIT, 0, 3000);
	if(meter_idx != -1)
	{
		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_QOS_HIGH;
		aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
		aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT | RT_ACL_INGRESS_TCP_FLAGS_BIT;
		aclRule.ingress_tcp_flags = 0x2;
		aclRule.ingress_tcp_flags_mask = 0xfff;
		aclRule.action_fields = RT_ACL_ACTION_METER_GROUP_SHARE_METER_BIT;
		aclRule.action_meter_group_share_meter_index = meter_idx;
		if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
			fprintf(fp, "%d\n", aclIdx);
		else
			printf("%s-%d rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

	}

	fclose(fp);
#endif

	return 0;
}

int rtk_fc_ds_udp_attack_rate_limit_flush( void )
{
	FILE *fp=NULL;
	int aclIdx, ret=0;

	if(!(fp = fopen(FC_UDP_ATTACK_RATE_LIMIT_ACL_FILE, "r")))
		return -2;

	while(fscanf(fp, "%d\n", &aclIdx) != EOF)
	{
		if((ret = rt_acl_filterAndQos_del(aclIdx)) != RT_ERR_OK)
			printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
	}

	fclose(fp);
	unlink(FC_UDP_ATTACK_RATE_LIMIT_ACL_FILE);
	rtk_qos_share_meter_flush(RTK_APP_TYPE_DS_UDP_ATTACK_RATE_LIMIT);
	return 0;
}

int rtk_fc_ds_udp_attack_rate_limit_set( void )
{
	rt_acl_filterAndQos_t aclRule;
	MIB_CE_ATM_VC_T atmVcEntry;
	unsigned int aclIdx=0;
	int meter_idx, ret;
	int atmVcEntryNum, i;
	char str_ipaddr[20] = {0};
	char str_netmask[20] = {0};
	char str_gateway[20] = {0};
	struct in_addr ipaddr;
	FILE *fp;

	if(!(fp = fopen(FC_UDP_ATTACK_RATE_LIMIT_ACL_FILE, "a")))
	{
		fprintf(stderr, "ERROR! %s\n", strerror(errno));
		return -1;
	}

	atmVcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<atmVcEntryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&atmVcEntry))
		{
			printf("error get atm vc entry\n");
			if(fp)
				fclose(fp);
			return -1;
		}

		if(atmVcEntry.applicationtype&X_CT_SRV_INTERNET && atmVcEntry.IpProtocol&IPVER_IPV4)
		{
			meter_idx = rtk_qos_share_meter_set(RTK_APP_TYPE_DS_UDP_ATTACK_RATE_LIMIT, 0, 56320);
			if(meter_idx != -1)
			{
				memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
				aclRule.acl_weight = WEIGHT_QOS_HIGH;
				aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
				aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
				aclRule.filter_fields |= RT_ACL_INGRESS_L4_UDP_BIT;
#if 1
				getIPaddrInfo(&atmVcEntry, str_ipaddr, str_netmask, str_gateway);
				if(isIPAddr(str_ipaddr) && (inet_pton(AF_INET, str_ipaddr, &ipaddr) == 1))
				{
					aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
					aclRule.ingress_dest_ipv4_addr_start = aclRule.ingress_dest_ipv4_addr_end = ntohl(ipaddr.s_addr);
					aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
					aclRule.ingress_ipv4_tagif = 1;
				}
#else
				aclRule.filter_fields |= RT_ACL_INGRESS_DMAC_BIT;
				memcpy(&aclRule.ingress_dmac[0], &atmVcEntry.MacAddr[0], MAC_ADDR_LEN);
#endif
				aclRule.filter_fields |= RT_ACL_INGRESS_PKT_LEN_RANGE_BIT;
				aclRule.ingress_packet_length_start = 124;
				aclRule.ingress_packet_length_end = 128;
				aclRule.action_fields = RT_ACL_ACTION_METER_GROUP_SHARE_METER_BIT;
				aclRule.action_meter_group_share_meter_index = meter_idx;
				if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
					fprintf(fp, "%d\n", aclIdx);
				else
					printf("%s-%d rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
			}
		}
	}

	fclose(fp);
	return 0;
}

int rtk_fc_ds_udp_attack_rate_limit_config( void )
{
	rtk_fc_ds_udp_attack_rate_limit_flush();
	rtk_fc_ds_udp_attack_rate_limit_set();
	return 0;
}

int rtk_fc_ddos_smurf_attack_protect( void )
{
	rt_acl_filterAndQos_t aclRule;
	unsigned int aclIdx=0;
	struct in_addr lan_ip;
	int ret;
	FILE *fp;

	//Yueme test plan 9.1.4.1 Ddos attack, smurf attack
	//smurf ddos attack! target ip 192.168.1.255, target dmac br0's mac
	/*
		rt_acl clear
		rt_acl set acl_weight 1000
		rt_acl set pattern ingress_dest_ipv4_addr_start 192.168.1.255 ingress_dest_ipv4_addr_end 192.168.1.255
		rt_acl set pattern ingress_dmac 00:00:00:00:00:00
		rt_acl set pattern ingress_dmac_mask 01:00:00:00:00:00
		rt_acl set pattern ingress_port_mask 0xf
		rt_acl set action FORWARD drop
		rt_acl add entry
	*/
	if(!(fp = fopen(FC_DDOS_SMURF_ATTACK_PROTECT_FILE, "a")))
	{
		fprintf(stderr, "ERROR! %s\n", strerror(errno));
		return -1;
	}

	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
	mib_get_s(MIB_ADSL_LAN_IP, (void *)&lan_ip, sizeof(lan_ip));
	aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
	aclRule.ingress_dest_ipv4_addr_start = aclRule.ingress_dest_ipv4_addr_end = (ntohl(*((ipaddr_t *)&lan_ip.s_addr)) | 0xff);
	aclRule.filter_fields |= RT_ACL_INGRESS_DMAC_BIT;
	aclRule.ingress_dmac[0] = 0;
	aclRule.ingress_dmac[1] = 0;
	aclRule.ingress_dmac[2] = 0;
	aclRule.ingress_dmac[3] = 0;
	aclRule.ingress_dmac[4] = 0;
	aclRule.ingress_dmac[5] = 0;
	aclRule.ingress_dmac_mask[0] = 0x1;
	aclRule.ingress_dmac_mask[1] = 0;
	aclRule.ingress_dmac_mask[2] = 0;
	aclRule.ingress_dmac_mask[3] = 0;
	aclRule.ingress_dmac_mask[4] = 0;
	aclRule.ingress_dmac_mask[5] = 0;
	aclRule.action_fields = RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
		fprintf(fp, "%d\n", aclIdx);
	else
		printf("%s-%d rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

	fclose(fp);
	return 0;
}

void rtk_fc_flow_update_in_real_time_init(void)
{
#if defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU)
	system("/bin/echo 1 > /proc/fc/ctrl/flow_not_update_in_real_time");
#else
	system("/bin/echo 0 > /proc/fc/ctrl/flow_not_update_in_real_time");
#endif
}

#ifdef CONFIG_USER_RT_ACL_SYN_FLOOD_PROOF_SUPPORT
int rtk_fc_ddos_tcp_syn_rate_limit_add( void )
{
	rt_acl_filterAndQos_t aclRule;
	unsigned int aclIdx=0;
	int meter_idx, ret;
	unsigned long mask, ip, subnet_start, subnet_end;
	FILE *fp;
	unsigned char ipStr[20], ipStrend[20];

	if(!(fp = fopen(FC_DDOS_TCP_SYN_RATE_LIMIT_ACL_FILE, "a")))
	{
		fprintf(stderr, "ERROR! %s\n", strerror(errno));
		return -1;
	}

	// Get LAN IP and mask
	mib_get_s(MIB_ADSL_LAN_IP, (void *)&ip, sizeof(ip));

	mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)&mask, sizeof(mask));
	subnet_start = (ip&mask);
	subnet_end = (ip|(~mask));

	meter_idx = rtk_qos_share_meter_get(RTK_APP_TYPE_DDOS_TCP_SYN_RATE_LIMIT, 0, 1000);
	if (meter_idx < 0)
		meter_idx = rtk_qos_share_meter_set(RTK_APP_TYPE_DDOS_TCP_SYN_RATE_LIMIT, 0, 1000);
	if(meter_idx != -1)
	{
#ifdef CONFIG_COMMON_RT_API
		//wan ingress SYN pkt limit 1M
		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_QOS_HIGH;
		aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
		aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT | RT_ACL_INGRESS_TCP_FLAGS_BIT;
		aclRule.ingress_tcp_flags = 0x2;
		aclRule.ingress_tcp_flags_mask = 0xfff;
		aclRule.action_fields = RT_ACL_ACTION_METER_GROUP_SHARE_METER_BIT;
		aclRule.action_meter_group_share_meter_index = meter_idx;
		if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
			fprintf(fp, "%d\n", aclIdx);
		else
			printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
#endif
	}

	meter_idx = rtk_qos_share_meter_get(RTK_APP_TYPE_DDOS_TCP_SYN_RATE_LIMIT, 1, 1000000);
	if (meter_idx < 0)
		meter_idx = rtk_qos_share_meter_set(RTK_APP_TYPE_DDOS_TCP_SYN_RATE_LIMIT, 1, 1000000);
	if(meter_idx != -1)
	{
#ifdef CONFIG_COMMON_RT_API
		//lan ingress SYN dport 53 pkt limit 1G
		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_HIGH;
		aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
		aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT | RT_ACL_INGRESS_TCP_FLAGS_BIT;
		aclRule.ingress_tcp_flags = 0x2;
		aclRule.ingress_tcp_flags_mask = 0xfff;
		aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_SIP_RANGE_BIT;
		aclRule.ingress_src_ipv4_addr_start = ntohl(subnet_start);
		aclRule.ingress_src_ipv4_addr_end = ntohl(subnet_end);
		aclRule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
		aclRule.ingress_dest_l4_port_start = 53;
		aclRule.ingress_dest_l4_port_end = 53;
		aclRule.action_fields = RT_ACL_ACTION_METER_GROUP_SHARE_METER_BIT;
		aclRule.action_meter_group_share_meter_index = meter_idx;
		if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
			fprintf(fp, "%d\n", aclIdx);
		else
			printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

		//lan ingress SYN dport 80 pkt limit 1G
		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_HIGH;
		aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
		aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT | RT_ACL_INGRESS_TCP_FLAGS_BIT;
		aclRule.ingress_tcp_flags = 0x2;
		aclRule.ingress_tcp_flags_mask = 0xfff;
		aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_SIP_RANGE_BIT;
		aclRule.ingress_src_ipv4_addr_start = ntohl(subnet_start);
		aclRule.ingress_src_ipv4_addr_end = ntohl(subnet_end);
		aclRule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
		aclRule.ingress_dest_l4_port_start = 80;
		aclRule.ingress_dest_l4_port_end = 80;
		aclRule.action_fields = RT_ACL_ACTION_METER_GROUP_SHARE_METER_BIT;
		aclRule.action_meter_group_share_meter_index = meter_idx;
		if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
			fprintf(fp, "%d\n", aclIdx);
		else
			printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

		//lan ingress SYN dport 443 pkt limit 1G
		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_HIGH;
		aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
		aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT | RT_ACL_INGRESS_TCP_FLAGS_BIT;
		aclRule.ingress_tcp_flags = 0x2;
		aclRule.ingress_tcp_flags_mask = 0xfff;
		aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_SIP_RANGE_BIT;
		aclRule.ingress_src_ipv4_addr_start = ntohl(subnet_start);
		aclRule.ingress_src_ipv4_addr_end = ntohl(subnet_end);
		aclRule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
		aclRule.ingress_dest_l4_port_start = 443;
		aclRule.ingress_dest_l4_port_end = 443;
		aclRule.action_fields = RT_ACL_ACTION_METER_GROUP_SHARE_METER_BIT;
		aclRule.action_meter_group_share_meter_index = meter_idx;
		if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
			fprintf(fp, "%d\n", aclIdx);
		else
			printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
#endif
	}

	meter_idx = rtk_qos_share_meter_get(RTK_APP_TYPE_DDOS_TCP_SYN_RATE_LIMIT, 0, 1000);
	if (meter_idx < 0)
		meter_idx = rtk_qos_share_meter_set(RTK_APP_TYPE_DDOS_TCP_SYN_RATE_LIMIT, 0, 1000);
	if(meter_idx != -1)
	{
#ifdef CONFIG_COMMON_RT_API
		//lan ingress SYN pkt default limit 1M
		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_QOS_HIGH;
		aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
		aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT | RT_ACL_INGRESS_TCP_FLAGS_BIT;
		aclRule.ingress_tcp_flags = 0x2;
		aclRule.ingress_tcp_flags_mask = 0xfff;
		aclRule.action_fields = RT_ACL_ACTION_METER_GROUP_SHARE_METER_BIT;
		aclRule.action_meter_group_share_meter_index = meter_idx;
		if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
			fprintf(fp, "%d\n", aclIdx);
		else
			printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
#endif
	}

	fclose(fp);

	rtk_lan_syn_flood_proof_fw_rules_add(ip, mask);

	return 0;
}

int rtk_fc_ddos_tcp_syn_rate_limit_delete( void )
{
	unsigned int aclIdx=0;
	int meter_idx, ret;
	char line[1024];
	FILE *fp;

	if(!(fp = fopen(FC_DDOS_TCP_SYN_RATE_LIMIT_ACL_FILE, "r")))
	{
		fprintf(stderr, "ERROR! %s\n", strerror(errno));
		return -2;
	}

	while(fgets(line, 1023, fp) != NULL)
	{
		sscanf(line, "%d\n", &aclIdx);
		if((ret = rt_acl_filterAndQos_del(aclIdx)) == RT_ERR_OK)
			printf("%s: delete syn flood acl rule %d success ! (ret = %d)\n", __FUNCTION__,aclIdx,ret);
		else
			printf("%s: delete flood acl rule %d fail ! (ret = %d)\n", __FUNCTION__,aclIdx,ret);
	}

	fclose(fp);
	unlink(FC_DDOS_TCP_SYN_RATE_LIMIT_ACL_FILE);

	meter_idx = rtk_qos_share_meter_get(RTK_APP_TYPE_DDOS_TCP_SYN_RATE_LIMIT, 0, 1000);
	if (meter_idx >= 0){
		rtk_qos_share_meter_delete(RTK_APP_TYPE_DDOS_TCP_SYN_RATE_LIMIT, 0);
	}

	meter_idx = rtk_qos_share_meter_get(RTK_APP_TYPE_DDOS_TCP_SYN_RATE_LIMIT, 1, 1000000);
	if (meter_idx >= 0){
		rtk_qos_share_meter_delete(RTK_APP_TYPE_DDOS_TCP_SYN_RATE_LIMIT, 1);
	}

	rtk_lan_syn_flood_proof_fw_rules_delete();
	return 0;
}

#endif

#ifdef STP_USE_RT_ACL_BLOCK
// RTL9607C
// don't use ACL to drop port packet when STP state to blocking
// some packet just like SSDP, L2 multicast will trap to CPU and ACL cannot drop it
#undef STP_USE_RT_ACL_BLOCK
#endif

int rtk_fc_stp_setup(void)
{
	char DMAC_MASK[MAC_ADDR_LEN]={0xff,0xff,0xff,0x0,0x0,0x0};
	char STP_DMAC[MAC_ADDR_LEN]={0x01,0x80,0xc2,0x0,0x0,0x0};
	rt_acl_filterAndQos_t aclRule;
	int ret, i, j, aclIdx;
	FILE *fp = NULL;
	unsigned int port_mask;
	char buf[256];
#if defined(CONFIG_RTL_SMUX_DEV)
	char ifname[IFNAMSIZ+1], smux_rule_alias[64];
	int logId;
#endif

	if(!(fp = fopen(FC_BR_STP_ACL_FILE, "w")))
	{
		fprintf(stderr, "[%s@%d]: ERROR! %s\n", __func__,__LINE__, strerror(errno));
		ret = -2;
		goto error;
	}

	port_mask = rtk_port_get_all_lan_phyMask();

	//STP packet trap to protocol stack
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_HIGH;
	aclRule.filter_fields |= RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT;
	aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	aclRule.ingress_port_mask = port_mask;
	aclRule.filter_fields |= RT_ACL_INGRESS_DMAC_BIT;
	memcpy(&aclRule.ingress_dmac, STP_DMAC, MAC_ADDR_LEN);
	memcpy(&aclRule.ingress_dmac_mask, DMAC_MASK, MAC_ADDR_LEN);
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
	aclRule.action_priority_group_trap_priority = 7;

	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0) {
		fprintf(fp, "%d\n", aclIdx);
	} else {
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
		goto error;
	}

#if ! defined(CONFIG_LUNA_G3_SERIES)
	rtk_stp_init();
#endif
#if defined(CONFIG_RTL_SMUX_DEV)
	strcpy(smux_rule_alias, "STP-Rule");
	for(i=0; i<32; i++)
	{
		if((port_mask & (1<<i)) && (logId = rtk_if_name_get_by_lan_phyID(i, ifname)) >= 0)
		{
			for(j=0 ; j<=SMUX_MAX_TAGS ; j++)
			{
				sprintf(buf, "%s --if %s --rx --tags %d  --filter-dmac 0180c2000000 FFFFFF000000 --set-rxif %s --target ACCEPT --rule-priority %d --rule-alias %s_def --rule-append",
					SMUXCTL, ELANRIF[logId], j, ifname, SMUX_RULE_PRIO_STP, smux_rule_alias);
				va_cmd("/bin/sh", 2, 1, "-c", buf);

				sprintf(buf, "%s --if %s --tx --tags %d  --filter-dmac 0180c2000000 FFFFFF000000 --target CONTINUE --rule-priority %d --rule-alias %s_def --rule-append",
					SMUXCTL, ELANRIF[logId], j, SMUX_RULE_PRIO_STP, smux_rule_alias);
				va_cmd("/bin/sh", 2, 1, "-c", buf);
			}
		}
	}
#endif
	rtk_fc_stp_sync_port_stat();
	ret = 0;
error:
	if(fp) fclose(fp);
	return ret;
}

int rtk_fc_stp_clean(void)
{
	FILE *fp = NULL;
	int ret, aclIdx;
	char buf[256];
#if defined(CONFIG_RTL_SMUX_DEV)
	unsigned int port_mask;
	char ifname[IFNAMSIZ+1], smux_rule_alias[64];
	int logId, i, j;
#endif

	if((fp = fopen(FC_BR_STP_ACL_FILE, "r")))
	{
		while(!feof(fp))
		{
			if(fgets(buf, sizeof(buf), fp) && sscanf(buf, "%d\n", &aclIdx) == 1)
				if(aclIdx>=0 && (ret = rt_acl_filterAndQos_del(aclIdx)) != RT_ERR_OK)
					printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
		}
		fclose(fp);
		unlink(FC_BR_STP_ACL_FILE);
	}
	else fprintf(stderr, "[%s@%d]: ERROR! %s\n", __func__,__LINE__, strerror(errno));

	if((fp = fopen(FC_BR_STP_PORT_STAT_FILE, "r")))
	{
#if defined(STP_USE_RT_ACL_BLOCK)
		fscanf(fp, "%d\n", &aclIdx);
		if(aclIdx>=0 && (ret = rt_acl_filterAndQos_del(aclIdx)) != RT_ERR_OK)
			printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
#endif
		fclose(fp);
		unlink(FC_BR_STP_PORT_STAT_FILE);
	}
	else fprintf(stderr, "[%s@%d]: ERROR! %s\n", __func__,__LINE__, strerror(errno));

#if ! defined(CONFIG_LUNA_G3_SERIES)
	rtk_stp_init();
#endif

#if defined(CONFIG_RTL_SMUX_DEV)
	port_mask = rtk_port_get_all_lan_phyMask();
	strcpy(smux_rule_alias, "STP-Rule");
	for(i=0; i<32; i++)
	{
		if((port_mask & (1<<i)) && (logId = rtk_if_name_get_by_lan_phyID(i, ifname)) >= 0)
		{
			for(j=0 ; j<=SMUX_MAX_TAGS ; j++)
			{
				sprintf(buf, "%s --if %s --rx --tags %d  --rule-remove-alias %s+",
					SMUXCTL, ELANRIF[logId], j, smux_rule_alias);
				va_cmd("/bin/sh", 2, 1, "-c", buf);

				sprintf(buf, "%s --if %s --tx --tags %d  --rule-remove-alias %s+",
					SMUXCTL, ELANRIF[logId], j, smux_rule_alias);
				va_cmd("/bin/sh", 2, 1, "-c", buf);
			}
		}
	}
#endif

	return 0;
}

int rtk_fc_stp_sync_port_stat(void)
{
	FILE *fp = NULL, *fp2 = NULL;
	int i, j, ret, aclIdx=-1, logId, state, port_stats[32];
	char port_name[32][IFNAMSIZ+1] = {0};
	unsigned int port_mask, block_port_mask;
	rt_acl_filterAndQos_t aclRule;
	char ifname[IFNAMSIZ+1], buf[256];
#if ! defined(CONFIG_LUNA_G3_SERIES)
	rtk_stp_state_t portState;
	int state2;
#endif
#if defined(CONFIG_RTL_SMUX_DEV)
	char smux_rule_alias[64];
#endif

#if defined(STP_USE_RT_ACL_BLOCK)
	if((fp = fopen(FC_BR_STP_PORT_STAT_FILE, "r")))
	{
		fscanf(fp, "%d\n", &aclIdx);
		fclose(fp);
	}
#endif
	if(!(fp = fopen(FC_BR_STP_PORT_STAT_FILE, "w")))
	{
		fprintf(stderr, "[%s@%d]: ERROR! %s\n", __func__,__LINE__, strerror(errno));
		ret = -2;
		goto error;
	}

	port_mask = rtk_port_get_all_lan_phyMask();
	block_port_mask = 0;
	for(i=0; i<32; i++)
	{
		port_stats[i] = -1;
		memset(port_name[i], 0, sizeof(port_name[i]));
		if((port_mask & (1<<i)) && (logId = rtk_if_name_get_by_lan_phyID(i, ifname)) >= 0)
		{
			strcpy(port_name[i], ELANRIF[logId]);
			state = RTK_BR_PORT_STATE_DISABLED;
			sprintf(buf, "/sys/class/net/%s/brport/state", ifname);
			if((fp2 = fopen(buf, "r")))
			{
				fscanf(fp2, "%d", &state);
				fclose(fp2);
			}
			port_stats[i] = state;
			if(state == RTK_BR_PORT_STATE_BLOCKING || state == RTK_BR_PORT_STATE_LISTENING)
				block_port_mask |= (1<<i);
#if ! defined(CONFIG_LUNA_G3_SERIES)
			if(rtk_stp_mstpState_get(0, i, &portState) == RT_ERR_OK)
			{
				switch(state)
				{
					case RTK_BR_PORT_STATE_DISABLED:	state2 = STP_STATE_DISABLED;	break;
					case RTK_BR_PORT_STATE_LISTENING:	state2 = STP_STATE_BLOCKING;	break;
					case RTK_BR_PORT_STATE_LEARNING:	state2 = STP_STATE_LEARNING; 	break;
					case RTK_BR_PORT_STATE_FORWARDING:	state2 = STP_STATE_FORWARDING;	break;
					case RTK_BR_PORT_STATE_BLOCKING:	state2 = STP_STATE_BLOCKING;	break;
					default: 							state2 = STP_STATE_FORWARDING;	break;
				}
				if(portState != state2 && (ret=rtk_stp_mstpState_set(0, i, state2) != RT_ERR_OK))
				{
					printf("[%s@%d] fail to rtk_stp_mstpState_set port %d from state %d to %d (ret = %d)\n",__func__,__LINE__, i, portState, state2, ret);
				}
			}
#endif
		}
	}
#if defined(STP_USE_RT_ACL_BLOCK)
	if(aclIdx < 0 || (ret = rt_acl_filterAndQos_get(aclIdx, &aclRule)) != RT_ERR_OK)
	{
		//STP packet trap to protocol stack
		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_HIGH-1;
		aclRule.filter_fields |= RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT;
		aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		aclRule.ingress_port_mask = 0;
		aclRule.action_fields = RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;
	}

	if(aclRule.ingress_port_mask != block_port_mask)
	{
		if(aclIdx != -1 && (ret = rt_acl_filterAndQos_del(aclIdx)) != RT_ERR_OK)
			printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
		aclIdx = -1;

		if(block_port_mask > 0)
		{
			aclRule.ingress_port_mask = block_port_mask;
			if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) != 0) {
				printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
			}
		}
	}
	fprintf(fp, "%d\n", aclIdx);
#endif

#if defined(CONFIG_RTL_SMUX_DEV)
	strcpy(smux_rule_alias, "STP-Rule");
	for(i=0; i<32; i++)
	{
		if(port_stats[i] >= 0)
		{
			for(j=0 ; j<=SMUX_MAX_TAGS ; j++)
			{
				sprintf(buf, "%s --if %s --rx --tags %d  --rule-remove-alias %s_block+",
						SMUXCTL, port_name[i], j, smux_rule_alias);
				va_cmd("/bin/sh", 2, 1, "-c", buf);

				sprintf(buf, "%s --if %s --tx --tags %d  --rule-remove-alias %s_block+",
						SMUXCTL, port_name[i], j, smux_rule_alias);
					va_cmd("/bin/sh", 2, 1, "-c", buf);

				if(block_port_mask & (1 << i))
				{
					sprintf(buf, "%s --if %s --rx --tags %d --target DROP --rule-priority %d --rule-alias %s_block --rule-append",
						SMUXCTL, port_name[i], j, SMUX_RULE_PRIO_STP-1, smux_rule_alias);
					va_cmd("/bin/sh", 2, 1, "-c", buf);

					sprintf(buf, "%s --if %s --tx --tags %d --target DROP --rule-priority %d --rule-alias %s_block --rule-append",
						SMUXCTL, port_name[i], j, SMUX_RULE_PRIO_STP-1, smux_rule_alias);
					va_cmd("/bin/sh", 2, 1, "-c", buf);
				}
			}
		}
	}
#endif
	for(i=0; i<32; i++)
	{
		if(port_stats[i] >= 0)
		{
			fprintf(fp, "port_%d=%d , IF=%s\n", i, port_stats[i], port_name[i]);
		}
	}

	ret = 0;

error:
	if(fp) fclose(fp);
	return ret;
}

int rtk_fc_stp_check_port_stat(int port, int state)
{
	int p=-1, s=-1;
	FILE *fp;
	char buf[128] = {0}, ifname[IFNAMSIZ+1];

	if((fp = fopen(FC_BR_STP_PORT_STAT_FILE, "r")))
	{
		while(!feof(fp))
		{
			if(fgets(buf, sizeof(buf), fp))
			{
				if(sscanf(buf, "port_%d=%d , IF=%s\n", &p, &s, ifname) == 3 && port == p)
					break;
			}
		}
		fclose(fp);
	}

	if(port == p && state != s)
	{
		//printf("[%s] change %s(%d) port state from %d to %d !!!\n", __FUNCTION__, ifname, port, s, state);
		rtk_fc_stp_sync_port_stat();
	}

	return 0;
}

int do_rtk_fc_flow_flush(void)
{
	//system("/bin/echo 1 > /proc/fc/ctrl/rstconntrack");
	system("/bin/echo 0 > /proc/fc/ctrl/flow_not_update_in_real_time");
	system("/bin/echo 1 > /proc/fc/ctrl/flow_flush");
	rtk_fc_flow_update_in_real_time_init();
	return 0;
}

int rtk_fc_flow_flush2(int s)
{
	char buf[32];
	if(s <= 0) return do_rtk_fc_flow_flush();
	sprintf(buf, "%d", s);
	return va_cmd("/bin/sysconf", 4, 0, "send_unix_sock_message", SYSTEMD_USOCK_SERVER, "do_flow_flush", buf);
}

int rtk_fc_flow_flush(void)
{
	unsigned char delay = 5;
	mib_get_s(MIB_FC_FLOW_FLUSH_DELAY, (void*)&delay, sizeof(delay));
	return rtk_fc_flow_flush2(delay);
}

#ifdef CONFIG_IPV6
/*
* Function:
*	rtk_fc_ipv6_mvlan_loop_detection_acl_add
*
* Propose:
*	To avoid loop back ipv6 NS packet resulting in DAD failure when onu uses the
*	unicast vlan which equals to the mvlan.
*
*   Set up an acl rule to filter ingress packet:
*	1. acl_weight
*		WEIGHT_QOS_HIGH: 550
*	2. Patterns
*		a. ingress_port_mask: wan port mask
*		b. ingress_smac: the same with wan mac
*		c. ingress_ipv6_tagif: must be IPv6
*	3. Action
*		FORWARD: drop
*
* Parameter:
*	AtmVcEntry
*
* Return:
*	0: success  -1: fail
*
* Remark:
*	This function would be called at startIP_for_V6. Since startIP_for_V6 would run
*   multiple times in startup and apply wan. Thus, this function would check the
*   rule exists or not for that wan at first in order to avoid duplicate rules.
*/
int rtk_fc_ipv6_mvlan_loop_detection_acl_add(MIB_CE_ATM_VC_Tp atmVcEntry)
{
	rt_acl_filterAndQos_t aclRule;
	unsigned int aclIdx = 0 ;
	int ret = -1;
	FILE *fp;
	char inf[IFNAMSIZ];
	char ipv6_acl_file[64] = {0};

	ifGetName(atmVcEntry->ifIndex, inf, sizeof(inf));
	snprintf(ipv6_acl_file, sizeof(ipv6_acl_file), "%s_%s", (char *)FC_ipv6_mvlan_loop_detection_ACL_FILE, inf);

	if(access(ipv6_acl_file, F_OK) == 0)
	{
		/* rule has already existed, do nothing. */
		return 0;
	}
	else
	{
		fp = fopen(ipv6_acl_file, "w");
		if(fp)
		{
			memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));

			aclRule.acl_weight = WEIGHT_QOS_HIGH;

			aclRule.filter_fields |= RT_ACL_INGRESS_IPV6_TAGIF_BIT;
			aclRule.ingress_ipv6_tagif = 1;

			aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
			aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();

			aclRule.filter_fields |= RT_ACL_INGRESS_SMAC_BIT;
			memcpy(&aclRule.ingress_smac, atmVcEntry->MacAddr, MAC_ADDR_LEN);

			aclRule.action_fields = RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;

			if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
			{
				fprintf(fp, "%d\n", aclIdx);
			}
			else
			{
				printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
			}
			fclose(fp);
		}
		return ret ;
	}
}

/*
* Function:
*	rtk_fc_ipv6_mvlan_loop_detection_acl_delete
*
* Propose:
*	delete acl loop detection rule
*
* Parameter:
*	AtmVcEntry
*
* Return:
*	0: success  -1: fail
*/
int rtk_fc_ipv6_mvlan_loop_detection_acl_delete(MIB_CE_ATM_VC_Tp atmVcEntry)
{
	unsigned int aclIdx = 0 ;
	FILE *fp;
	int ret = -1 ;
	char inf[IFNAMSIZ];
	char ipv6_acl_file[64] = {0};

	ifGetName(atmVcEntry->ifIndex, inf, sizeof(inf));
	snprintf(ipv6_acl_file, sizeof(ipv6_acl_file), "%s_%s", (char *)FC_ipv6_mvlan_loop_detection_ACL_FILE, inf);

	if(fp = fopen(ipv6_acl_file, "r"))
	{
		ret=fscanf(fp, "%d\n", &aclIdx);
		if(ret!=1)
			printf("fscanf error!\n");
		if((ret = rt_acl_filterAndQos_del(aclIdx)) != RT_ERR_OK)
		{
			printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
			fclose(fp);
			return ret;
		}
		fclose(fp);
		unlink(ipv6_acl_file);
	}
	else
	{
		fprintf(stderr, "[%s@%d]: ERROR! %s\n", __func__,__LINE__, strerror(errno));
	}
	return ret;
}
#endif
#endif

int rtk_fc_total_upstream_bandwidth_set(int wan_port, unsigned int bandwidth)
{
	AUG_PRT("bandwidth=%d\n", bandwidth);
#if (defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)) && defined(CONFIG_COMMON_RT_API)
	unsigned int pon_mode = 0, wanPhyPort=0;
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
	if(pon_mode == EPON_MODE){
		if(rt_epon_egrBandwidthCtrlRate_set(bandwidth) != RT_ERR_OK)
		{
			printf("rt_epon_egrBandwidthCtrlRate_set failed: %s %d\n", __func__, __LINE__);
			return -1;
		}
	}
	else if(pon_mode == GPON_MODE){
		if(rt_gpon_egrBandwidthCtrlRate_set(bandwidth) != RT_ERR_OK)
		{
			printf("rt_gpon_egrBandwidthCtrlRate_set failed: %s %d\n", __func__, __LINE__);
			return -1;
		}
	}
	else if (pon_mode == ETH_MODE)
	{
		mib_get_s(MIB_WAN_PHY_PORT,&wanPhyPort,sizeof(wanPhyPort));
		if(rt_rate_portEgrBandwidthCtrlRate_set(wanPhyPort, bandwidth) != RT_ERR_OK)
			printf("rt_rate_portEgrBandwidthCtrlRate_set failed: %s %d\n", __func__, __LINE__);
	}
#elif defined(CONFIG_USER_AP_HW_QOS)
#ifdef CONFIG_USER_IP_QOS
	rtk_ap_qos_bandwidth_control(wan_port, bandwidth);
#endif
#endif
	return 0;
}

int rtk_fc_total_downstream_bandwidth_set(int wan_port, unsigned int bandwidth)
{
	int portIndex;

	AUG_PRT("bandwidth=%d\n", bandwidth);
#if defined(CONFIG_COMMON_RT_API)
	for(portIndex=0 ; portIndex<ELANVIF_NUM ; portIndex++)
	{
		if(rt_rate_portEgrBandwidthCtrlRate_set(rtk_port_get_lan_phyID(portIndex), bandwidth) != RT_ERR_OK)
			printf("rt_rate_portEgrBandwidthCtrlRate_set failed: %s %d\n", __func__, __LINE__);
	}
#endif
	return 0;
}

int rtk_fc_qos_setup_upstream_wrr(int set)
{
	MIB_CE_IP_QOS_QUEUE_T ipQoSQueueTblEntry;
	int ipQoSTblEntryTotalNum;
#if defined(CONFIG_COMMON_RT_API)
	rt_qos_queue_weights_t qosWeight;
#endif
	int wanPhyPort=0, i;

	mib_get_s(MIB_WAN_PHY_PORT,&wanPhyPort,sizeof(wanPhyPort));
	AUG_PRT("wanPhyPort=%d set=%d\n", wanPhyPort, set);

#if defined(CONFIG_COMMON_RT_API)
	if(set)
	{
		memset(&ipQoSQueueTblEntry, 0, sizeof(MIB_CE_IP_QOS_QUEUE_T));
		ipQoSTblEntryTotalNum = mib_chain_total(MIB_IP_QOS_QUEUE_TBL);
		for(i=0; i<ipQoSTblEntryTotalNum; i++)
		{
			if (!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i, (void*)&ipQoSQueueTblEntry))
				continue;

			if((MAX_QOS_QUEUE_NUM-ipQoSQueueTblEntry.prior) > RTK_MAX_NUM_OF_QUEUE)
				continue;

			if(!ipQoSQueueTblEntry.enable)
			{
				qosWeight.weights[MAX_QOS_QUEUE_NUM-ipQoSQueueTblEntry.prior] = 1;
				continue;
			}
			qosWeight.weights[MAX_QOS_QUEUE_NUM-ipQoSQueueTblEntry.prior] = ipQoSQueueTblEntry.weight;
		}
	}
	else
	{
		memset(&qosWeight, 0, sizeof(rt_qos_queue_weights_t));
	}
	if(rt_qos_schedulingQueue_set(wanPhyPort, &qosWeight) != RT_ERR_OK)
		printf("rt_qos_schedulingQueue_set failed: %s %d\n", __func__, __LINE__);
#endif

	return 0;
}


int rtk_fc_qos_setup_downstream_wrr(int set)
{
	MIB_CE_IP_QOS_QUEUE_T ipQoSQueueTblEntry;
	int ipQoSTblEntryTotalNum;
#if defined(CONFIG_COMMON_RT_API)
	rt_qos_queue_weights_t qosWeight;
#endif
	int portIndex, i;

	AUG_PRT("set=%d\n", set);

#if defined(CONFIG_COMMON_RT_API)
	if(set)
	{
		memset(&ipQoSQueueTblEntry, 0, sizeof(MIB_CE_IP_QOS_QUEUE_T));
		ipQoSTblEntryTotalNum = mib_chain_total(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL);
		for(i=0; i<ipQoSTblEntryTotalNum; i++)
		{
			if (!mib_chain_get(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL, i, (void*)&ipQoSQueueTblEntry))
				continue;

			if((MAX_QOS_QUEUE_NUM-ipQoSQueueTblEntry.prior) > RTK_MAX_NUM_OF_QUEUE)
				continue;

			if(!ipQoSQueueTblEntry.enable)
			{
				qosWeight.weights[MAX_QOS_QUEUE_NUM-ipQoSQueueTblEntry.prior] = 1;
				continue;
			}

			qosWeight.weights[MAX_QOS_QUEUE_NUM-ipQoSQueueTblEntry.prior] = ipQoSQueueTblEntry.weight;
		}
	}
	else
	{
		memset(&qosWeight, 0, sizeof(rt_qos_queue_weights_t));
	}

	for(portIndex=0 ; portIndex<CONFIG_LAN_PORT_NUM ; portIndex++)
	{
		if(rt_qos_schedulingQueue_set(rtk_port_get_lan_phyID(portIndex), &qosWeight) != RT_ERR_OK)
			printf("rt_qos_schedulingQueue_set failed: %s %d\n", __func__, __LINE__);
	}
#endif

	return 0;
}

int rtk_fc_multicast_flow_add(unsigned int group)
{
	int ret = 0;
#if defined(CONFIG_COMMON_RT_API)
	char ipStr[INET6_ADDRSTRLEN] = {0};
	rt_igmp_group_devPort_cfg_t mcFlow = {0};

	memset(&mcFlow, 0, sizeof(rt_igmp_group_devPort_cfg_t));

	if(!isIgmproxyEnabled())
		return -1;

	if(isIGMPSnoopingEnabled())
		return -1;

	mcFlow.groupBehavior = RT_MC_BEHAVIOR_GROUP_AS_KNOW;
	mcFlow.is_ipv6 = 0;
	mcFlow.group_addr.ipv4 = ntohl(group);

	inet_ntop(AF_INET, &(group), ipStr, sizeof(ipStr));
	ret=rt_igmp_multicastGroupDev_set(mcFlow);
	AUG_PRT("ret=%d multicast_ipv4_addr=%s\n", ret, ipStr);
#endif
	return ret;
}

int rtk_fc_multicast_flow_delete(unsigned int group)
{
	int ret = 0;
#if defined(CONFIG_COMMON_RT_API)
	rt_igmp_group_devPort_cfg_t mcFlow = {0};
	char ipStr[INET6_ADDRSTRLEN] = {0};

	memset(&mcFlow, 0, sizeof(rt_igmp_group_devPort_cfg_t));

	if(!isIgmproxyEnabled())
		return -1;

	if(isIGMPSnoopingEnabled())
		return -1;

	mcFlow.groupBehavior = RT_MC_BEHAVIOR_GROUP_AS_UNKNOW;
	mcFlow.is_ipv6 = 0;
	mcFlow.group_addr.ipv4 = ntohl(group);

	inet_ntop(AF_INET, &(group), ipStr, sizeof(ipStr));
	ret=rt_igmp_multicastGroupDev_set(mcFlow);
	AUG_PRT("ret=%d multicast_ipv4_addr=%s\n", ret, ipStr);
#endif
	return ret;
}

#ifdef CONFIG_IPV6
int rtk_fc_ipv6_multicast_flow_add(unsigned int* group)
{
	int ret = 0;
#if defined(CONFIG_COMMON_RT_API)
	rt_igmp_group_devPort_cfg_t mcFlow = {0};
	char ipStr[INET6_ADDRSTRLEN] = {0};

	memset(&mcFlow, 0, sizeof(rt_igmp_group_devPort_cfg_t));

#ifdef CONFIG_USER_MLDPROXY
	if(!isMLDProxyEnabled())
		return -1;
#endif

	if(isMLDSnoopingEnabled())
		return -1;

	mcFlow.groupBehavior = RT_MC_BEHAVIOR_GROUP_AS_KNOW;
	mcFlow.is_ipv6 = 1;
	memcpy(mcFlow.group_addr.ipv6, group, sizeof(mcFlow.group_addr.ipv6));

	inet_ntop(PF_INET6, mcFlow.group_addr.ipv6, ipStr, sizeof(ipStr));
	ret=rt_igmp_multicastGroupDev_set(mcFlow);
	AUG_PRT("ret=%d multicast_ipv6_addr=%s\n", ret, ipStr);
#endif

	return ret;
}

int rtk_fc_ipv6_multicast_flow_delete(unsigned int* group)
{
	int ret = 0;
#if defined(CONFIG_COMMON_RT_API)
	rt_igmp_group_devPort_cfg_t mcFlow = {0};
	char ipStr[INET6_ADDRSTRLEN] = {0};

	memset(&mcFlow, 0, sizeof(rt_igmp_group_devPort_cfg_t));
#ifdef CONFIG_USER_MLDPROXY
	if(!isMLDProxyEnabled())
		return -1;
#endif
	if(isMLDSnoopingEnabled())
		return -1;

	mcFlow.groupBehavior = RT_MC_BEHAVIOR_GROUP_AS_UNKNOW;
	mcFlow.is_ipv6 = 1;
	memcpy(mcFlow.group_addr.ipv6, group, sizeof(mcFlow.group_addr.ipv6));
	inet_ntop(PF_INET6, mcFlow.group_addr.ipv6, ipStr, sizeof(ipStr));
	ret=rt_igmp_multicastGroupDev_set(mcFlow);
	AUG_PRT("ret=%d multicast_ipv6_addr=%s\n", ret, ipStr);
#endif
	return ret;
}
#endif

int rtk_fc_multicast_ignoregroup_trap_rule(int config, int *pindex, const char *start_group, const char *end_greoup)
{
#ifdef CONFIG_COMMON_RT_API
	rt_acl_filterAndQos_t aclRule;
	unsigned int ignipS[4], ignipE[4];
	unsigned char is_IPv6;
	unsigned int aclIdx = 0;
	int ret = 0;

	memset(ignipS, 0, sizeof(ignipS));
	memset(ignipE, 0, sizeof(ignipE));

	if(!start_group && !end_greoup && (config || !pindex || *pindex<0))
		return -1;
	else if(config == 0) {
		if(pindex && *pindex >= 0) {
			aclIdx = *pindex;
			ret = rt_acl_filterAndQos_del(aclIdx);
		} else
			return -1;
	}
	else {
		if((!start_group || inet_pton(AF_INET, start_group, (void *)&ignipS[0]) > 0) &&
				(!end_greoup || inet_pton(AF_INET, end_greoup, (void *)&ignipE[0]) > 0))
		{
			is_IPv6 = 0;
			ignipS[0] = ntohl(ignipS[0]);
			ignipE[0] = ntohl(ignipE[0]);
		}
		else if((!start_group || inet_pton(AF_INET6, start_group, (void *)&ignipS[0]) > 0) &&
				(!end_greoup || inet_pton(AF_INET6, end_greoup, (void *)&ignipE[0]) > 0))
		{
			is_IPv6 = 1;
		}
		else  {
			return -1;
		}

		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_NORMAL;
		aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
		aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask() | rtk_port_get_wan_phyMask();
		aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
		aclRule.action_priority_group_trap_priority = 0;
		if(!is_IPv6) {
			aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
			memcpy(&aclRule.ingress_dest_ipv4_addr_start, ignipS, sizeof(aclRule.ingress_dest_ipv4_addr_start));
			memcpy(&aclRule.ingress_dest_ipv4_addr_end, ignipE, sizeof(aclRule.ingress_dest_ipv4_addr_end));
			aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
			aclRule.ingress_ipv4_tagif = 1;
		}
#ifdef CONFIG_IPV6
		else {
			unsigned int addr_mask[4]={};
			aclRule.filter_fields |= RT_ACL_INGRESS_IPV6_DIP_BIT;
			rtk_ipv6_ip_address_range_to_mask(0, (struct in6_addr*)&ignipS, (struct in6_addr*)&ignipE, (struct in6_addr*)&addr_mask);
			memcpy(aclRule.ingress_dest_ipv6_addr, ignipS, sizeof(aclRule.ingress_dest_ipv6_addr));
			memcpy(aclRule.ingress_dest_ipv6_addr_mask, addr_mask, sizeof(aclRule.ingress_dest_ipv6_addr_mask));
			aclRule.filter_fields |= RT_ACL_INGRESS_IPV6_TAGIF_BIT;
			aclRule.ingress_ipv6_tagif = 1;
		}
#endif
		ret = rt_acl_filterAndQos_add(aclRule, &aclIdx);
		if(ret == 0 && pindex)
			*pindex = aclIdx;
	}
	return ret;
#else
	printf("[%s@%d] not support common rt api! \n",__func__,__LINE__);
	return -1;
#endif
}

#ifdef CONFIG_RTL9607C_SERIES
//Need config before NIC initial rx/tx ring describe.
int rtk_fc_check_enanle_NIC_SRAM_accelerate(void)
{
	char nic_dynamic_sram_desc_path[128] = {0};
	unsigned char enable=0;

	mib_get_s(MIB_NIC_SRAM_ACCELERATE_ENABLE, (void *)&enable, sizeof(enable));

	snprintf(nic_dynamic_sram_desc_path, sizeof(nic_dynamic_sram_desc_path), "%s", (char *)"/proc/rtl8686gmac/dynamic_sram_desc");
	if(access(nic_dynamic_sram_desc_path, F_OK) != 0)
	{
		/* path not exist, do nothing. */
		printf(">> HWNAT customize was not enabled, do nothing <<\n");
		return 0;
	}

	if(enable){
		printf(">> Enable NIC SRAM accerlate <<\n");
		if(enable==1)
		{
			va_cmd("/bin/sh", 2, 1, "-c", "echo 1 > /proc/rtl8686gmac/dynamic_sram_desc");
                        va_cmd("/bin/sh", 2, 1, "-c", "echo 1 > /proc/fc/sw_dump/nptv6_acc_mechanism");
		}
		else if(enable==3)//PON WAN two port NPTv6
                {
			va_cmd("/bin/sh", 2, 1, "-c", "echo 1 > /proc/rtl8686gmac/nptv6_two_port");
                        va_cmd("/bin/sh", 2, 1, "-c", "echo 1 > /proc/rtl8686gmac/dynamic_sram_desc");
                        va_cmd("/bin/sh", 2, 1, "-c", "echo 2 > /proc/fc/sw_dump/nptv6_acc_mechanism");
                }
                else if(enable==4)//ethernet WAN two port NPTv6
                {
                        va_cmd("/bin/sh", 2, 1, "-c", "echo 1 > /proc/rtl8686gmac/nptv6_two_port");
                        va_cmd("/bin/sh", 2, 1, "-c", "echo 1 > /proc/rtl8686gmac/dynamic_sram_desc");
                        va_cmd("/bin/sh", 2, 1, "-c", "echo 3 > /proc/fc/sw_dump/nptv6_acc_mechanism");
                }

		else
		{
			va_cmd("/bin/sh", 2, 1, "-c", "echo 2 > /proc/rtl8686gmac/dynamic_sram_desc");
                        va_cmd("/bin/sh", 2, 1, "-c", "echo 1 > /proc/fc/sw_dump/vxlan_acc_mechanism");
		}
	}
	else
	{
		printf(">> Disable NIC SRAM accerlate - DRAM mode  <<\n");
		va_cmd("/bin/sh", 2, 1, "-c", "echo 0 > /proc/rtl8686gmac/dynamic_sram_desc");
		va_cmd("/bin/sh", 2, 1, "-c", "echo reset > /proc/rtl8686gmac/hw_reg");
                va_cmd("/bin/sh", 2, 1, "-c", "echo 1 > /proc/fc/sw_dump/nptv6_acc_mechanism");
	}

	return 0;
}
#endif

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
int rtk_fc_external_switch_init(void)
{
	char pvidStartStr[32];
	char buf[200]={0};

	mib_get_s(MIB_EXTERNAL_SWITCH_PVID_START, (void *)pvidStartStr, sizeof(pvidStartStr));
	snprintf(buf, sizeof(buf), "echo svlan %s num %d, > /proc/fc/ctrl/external_switch_info", pvidStartStr, CONFIG_EXTERNAL_SWITCH_LAN_PORT_NUM);
	system(buf);
	//snprintf(buf, sizeof(buf), "echo %d > /proc/fc/ctrl/external_switch_lan_port_num", CONFIG_EXTERNAL_SWITCH_LAN_PORT_NUM);
	//system(buf);

	return 0;
}

int rtk_fc_external_switch_rgmii_port_get(void)
{
	return CONFIG_LAN_PORT_NUM;
}
#endif

int rtk_fc_add_aclTrapPri_for_SWACC(unsigned char priority)
{
	char syscmd[65];
	FILE *fp;
	unsigned int sw_priority = 0;

	fp = fopen(FC_SW_ACC_PRIO_FILE, "r");
	if(fp)
	{
		fscanf(fp, "0x%x", &sw_priority);
		fclose(fp);
	}

	if(!(sw_priority & (1<<priority)))
		sw_priority |= (1<<priority);

	snprintf(syscmd, sizeof(syscmd), "echo 0x%x > /proc/fc/ctrl/flow_swFwded_aclTrapPri_mask", sw_priority);
	va_cmd("/bin/sh", 2, 1, "-c", syscmd);

	fp = fopen(FC_SW_ACC_PRIO_FILE, "w");
	if(fp)
	{
		fprintf(fp, "0x%x", sw_priority);
		fclose(fp);
	}

	return 0;
}

int rtk_fc_del_aclTrapPri_for_SWACC(unsigned char priority)
{
	char syscmd[65];
	FILE *fp;
	unsigned int sw_priority = 0;

	fp = fopen(FC_SW_ACC_PRIO_FILE, "r");
	if(fp)
	{
		fscanf(fp, "0x%x", &sw_priority);
		fclose(fp);
	}

	if(sw_priority & (1<<priority))
		sw_priority &= ~(1<<priority);

	snprintf(syscmd, sizeof(syscmd), "echo 0x%x > /proc/fc/ctrl/flow_swFwded_aclTrapPri_mask", sw_priority);
	va_cmd("/bin/sh", 2, 1, "-c", syscmd);

	fp = fopen(FC_SW_ACC_PRIO_FILE, "w");
	if(fp)
	{
		fprintf(fp, "0x%x", sw_priority);
		fclose(fp);
	}

	return 0;
}

#if defined(CONFIG_FC_SPECIAL_FAST_FORWARD)
// for 9607C NPTv6, vxlan accelerate, use trap with priority 2 not 5
#define FC_TCP_SMALLACK_PRIO 2
#else
#define FC_TCP_SMALLACK_PRIO 5
#endif

// for ca8277X the HW acl to define packet length only has 4 rule, so let combine TCP small ACK ACL packet length
#define __COMBINE_TCP_ACK_ACL_PKT_LEN__

#ifdef __COMBINE_TCP_ACK_ACL_PKT_LEN__
#define _TCP_ACK_LEN_START_ 54
#define _TCP_ACK_LEN_END_ 126
#else
#define __TEST_CENTER__SMALL_PKT__
#endif
int rtk_fc_tcp_smallack_protect_set(int enable)
{
#ifdef CONFIG_COMMON_RT_API
	rt_acl_filterAndQos_t aclRule;
#endif
	unsigned int aclIdx = 0;
	int ret = -1;
	FILE *fp;
	unsigned char legacy = 0;

	fp = fopen(FC_tcp_smallack_protect_ACL_FILE, "r");
	if(fp)
	{
#ifdef CONFIG_COMMON_RT_API
		while(!feof(fp))
		{
			if(fscanf(fp, "%d\n", &aclIdx) && (ret = rt_acl_filterAndQos_del(aclIdx)) != RT_ERR_OK)
			{
				printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
				fclose(fp);
				return ret;
			}
		}
#endif
		fclose(fp);
		unlink(FC_tcp_smallack_protect_ACL_FILE);
	}
	rtk_fc_del_aclTrapPri_for_SWACC( FC_TCP_SMALLACK_PRIO );

	ret = rtk_fc_flow_delay_config(1);
	mib_get_s(MIB_QOS_TCP_SMALL_ACK_PROTECT_LEGACY, &legacy, sizeof(legacy));
	if(enable == 3)
	{
		enable = 1;
		legacy = 1;
	}
	if(enable && (legacy || ret <= 0))
	{
		fp = fopen(FC_tcp_smallack_protect_ACL_FILE, "w");
		if(fp)
		{
			if (enable == 1)
			{
#ifdef CONFIG_COMMON_RT_API
			// add ACL to trap tcp with ACK flag and packet size in 54 ~ 106 bytes
			// ACL trap with priority 5, and also set FC do learning software accelerate flow for priority 5

			//for LAN upstream use trap priority
			memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
			aclRule.acl_weight = WEIGHT_QOS_HIGH;
			aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
			aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
			aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT;
			aclRule.filter_fields |= RT_ACL_INGRESS_TCP_FLAGS_BIT;
			aclRule.ingress_tcp_flags = 0x10;
			aclRule.ingress_tcp_flags_mask = 0x1ff;
			aclRule.filter_fields |= RT_ACL_INGRESS_PKT_LEN_RANGE_BIT;
#ifdef __COMBINE_TCP_ACK_ACL_PKT_LEN__
			aclRule.ingress_packet_length_start = _TCP_ACK_LEN_START_;
			aclRule.ingress_packet_length_end = _TCP_ACK_LEN_END_;
#else
			aclRule.ingress_packet_length_start = 54; //L2(14)+L3(20)+L4(20)
#ifdef __TEST_CENTER__SMALL_PKT__
			aclRule.ingress_packet_length_end = 80; //for tescenter test, it will use 82 bytes to do TCP ACK test, so need ignore it
#else
			aclRule.ingress_packet_length_end = 106;  //L2(14)+VLAN1(4)+VLAN2(4)+L3(20)+L4(60)+CHKSMUM(4)
#endif
			aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
			aclRule.ingress_ipv4_tagif = 1;
#endif
			aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
			aclRule.action_priority_group_trap_priority = FC_TCP_SMALLACK_PRIO;
			if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
				fprintf(fp, "%d\n", aclIdx);
			else
				printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

#if defined(CONFIG_IPV6) && !defined(__COMBINE_TCP_ACK_ACL_PKT_LEN__)
			//for LAN upstream use trap priority
			memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
			aclRule.acl_weight = WEIGHT_QOS_HIGH;
			aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
			aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
			aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT;
			aclRule.filter_fields |= RT_ACL_INGRESS_TCP_FLAGS_BIT;
			aclRule.ingress_tcp_flags = 0x10;
			aclRule.ingress_tcp_flags_mask = 0x1ff;
			aclRule.filter_fields |= RT_ACL_INGRESS_PKT_LEN_RANGE_BIT;
#ifdef __COMBINE_TCP_ACK_ACL_PKT_LEN__
			aclRule.ingress_packet_length_start = _TCP_ACK_LEN_START_;
			aclRule.ingress_packet_length_end = _TCP_ACK_LEN_END_;
#else
			aclRule.ingress_packet_length_start = 74; //L2(14)+L3(40)+L4(20)
#ifdef __TEST_CENTER__SMALL_PKT__
			aclRule.ingress_packet_length_end = 100; //for tescenter test, it will use 102 bytes to do TCP ACK test, so need ignore it
#else
			aclRule.ingress_packet_length_end = 126;  //L2(14)+VLAN1(4)+VLAN2(4)+L3(40)+L4(60)+CHKSMUM(4)
#endif
#endif
			aclRule.filter_fields |= RT_ACL_INGRESS_IPV6_TAGIF_BIT;
			aclRule.ingress_ipv6_tagif = 1;
			aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT|RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT);
			aclRule.action_priority_group_trap_priority = FC_TCP_SMALLACK_PRIO;
			if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
				fprintf(fp, "%d\n", aclIdx);
			else
				printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
#endif
#endif
			}
			if (enable == 1 || enable == 2)
			{
			//for WAN and LAN(include option length) use ACL priority 7
			memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
			aclRule.acl_weight = WEIGHT_QOS_HIGH-1;
			aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
			aclRule.ingress_port_mask = (rtk_port_get_wan_phyMask()|rtk_port_get_all_lan_phyMask());
			aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT;
			aclRule.filter_fields |= RT_ACL_INGRESS_TCP_FLAGS_BIT;
			aclRule.ingress_tcp_flags = 0x10;
			aclRule.ingress_tcp_flags_mask = 0x1ff;
			aclRule.filter_fields |= RT_ACL_INGRESS_PKT_LEN_RANGE_BIT;
#ifdef __COMBINE_TCP_ACK_ACL_PKT_LEN__
			aclRule.ingress_packet_length_start = _TCP_ACK_LEN_START_;
			aclRule.ingress_packet_length_end = _TCP_ACK_LEN_END_;
#else
			aclRule.ingress_packet_length_start = 54; //L2(14)+L3(20)+L4(20)
			aclRule.ingress_packet_length_end = 126;  //L2(14)+VLAN1(4)+VLAN2(4)+L3(40)+L4(60)+CHKSMUM(4)
#endif
			aclRule.action_fields = (RT_ACL_ACTION_PRIORITY_GROUP_ACL_PRIORITY_BIT);
			aclRule.action_priority_group_acl_priority = 7;
			if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
				fprintf(fp, "%d\n", aclIdx);
			else
				printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
			}

			fclose(fp);

			rtk_fc_add_aclTrapPri_for_SWACC( FC_TCP_SMALLACK_PRIO );
		}
	}

	return 0;
}

#define FC_FLOW_DELAY_PATH "/proc/fc/ctrl/flow_delay"
int rtk_fc_flow_delay_config(int enable)
{
	struct stat st;
	unsigned char mode=0;
	unsigned int en=0;
	char cmd[64] = {0}, *modeStr;

	if(stat(FC_FLOW_DELAY_PATH, &st) != 0) return -1;

	mib_get_s(MIB_FC_FLOW_DELAY, &en, sizeof(en));
	mib_get_s(MIB_FC_FLOW_DELAY_MODE, &mode, sizeof(mode));

	if((en > 0 && en < 64) && enable)
	{
		if(mode == 1) modeStr = "shortcut";
		else modeStr = "kernel";
		snprintf(cmd, sizeof(cmd), "echo mode %s > %s", modeStr, FC_FLOW_DELAY_PATH);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "echo count %u > %s", en, FC_FLOW_DELAY_PATH);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);

		return 1;
	}
	else
	{
		snprintf(cmd, sizeof(cmd), "echo count 0 > %s", FC_FLOW_DELAY_PATH);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
	}

	return 0;
}

#if defined(CONFIG_XFRM) && defined(CONFIG_USER_STRONGSWAN)
const char TABLE_IPSEC_STRONGSWAN_PRI_POST[] = "ipsec_strongswan_pri";
const char TABLE_IPSEC_STRONGSWAN_POST[] = "ipsec_strongswan";
const char TABLE_IPSEC_STRONGSWAN_PRE[] = "ipsec_strongswan_pre";
#if defined(CONFIG_RTL8277C_SERIES)
const char FC_IPSEC_TRAFFIC_BYPASS_FILE[] = "/var/fc_ipsec_traffic_bypass_file";
const char FC_IPSEC_TRAFFIC_UPSTREAM_BYPASS_FILE[] = "/var/fc_ipsec_traffic_upstream_bypass_file";


int rtk_fc_ipsec_strongswan_traffic_bypass_flush(void)
{
	FILE *fp=NULL;
	int aclIdx, ret=0;

	if(!(fp = fopen(FC_IPSEC_TRAFFIC_BYPASS_FILE, "r")))
		return -2;

	while(fscanf(fp, "%d\n", &aclIdx) != EOF)
	{
		if((ret = rt_acl_filterAndQos_del(aclIdx)) != RT_ERR_OK)
			printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
	}

	fclose(fp);
	unlink(FC_IPSEC_TRAFFIC_BYPASS_FILE);
	return 0;
}

int rtk_fc_ipsec_strongswan_traffic_bypass_set(void)
{
	char UCAST_MASK[MAC_ADDR_LEN]={0x01,0x00,0x00,0x00,0x00,0x00};
	rt_acl_filterAndQos_t aclRule;
	MIB_CE_ATM_VC_T vcEntry;
	unsigned int aclIdx;
	int vcTotal=-1;
	FILE *fp=NULL;
	int ret, i;

	if(!(fp = fopen(FC_IPSEC_TRAFFIC_BYPASS_FILE, "a")))
	{
		fprintf(stderr, "ERROR! %s\n", strerror(errno));
		return -2;
	}

	/* ESP packet */
	memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
	aclRule.acl_weight = WEIGHT_QOS_HIGH;
	aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
	aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
	// DMAC = ELAN_MAC_ADDR
	aclRule.filter_fields |= RT_ACL_INGRESS_DMAC_BIT;
	/*mib_get_s(MIB_ELAN_MAC_ADDR, (void *)&aclRule.ingress_dmac, sizeof(aclRule.ingress_dmac));
	memcpy(&aclRule.ingress_dmac_mask,UCAST_MASK,MAC_ADDR_LEN);*/;
	memcpy(&aclRule.ingress_dmac_mask,UCAST_MASK,MAC_ADDR_LEN);
	aclRule.filter_fields |= RT_ACL_INGRESS_L4_POROTCAL_VALUE_BIT;
	aclRule.ingress_l4_protocal = 50;
	aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_TO_PS_BIT);
	if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0) {
		fprintf(fp, "%d\n", aclIdx);
	} else {
		printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
	}

	fclose(fp);
	return 0;
}

int rtk_fc_ipsec_strongswan_traffic_bypass_config(void)
{
	LOCK_ACL();
	rtk_fc_ipsec_strongswan_traffic_bypass_flush();
	rtk_fc_ipsec_strongswan_traffic_bypass_set();
	UNLOCK_ACL();
	return 0;
}

int rtk_fc_ipsec_strongswan_traffic_upstream_bypass_flush(void)
{
	FILE *fp=NULL;
	int aclIdx, ret=0;

	if(!(fp = fopen(FC_IPSEC_TRAFFIC_UPSTREAM_BYPASS_FILE, "r")))
		return -2;

	while(fscanf(fp, "%d\n", &aclIdx) != EOF)
	{
		if((ret = rt_acl_filterAndQos_del(aclIdx)) != RT_ERR_OK)
			printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
	}

	fclose(fp);
	unlink(FC_IPSEC_TRAFFIC_UPSTREAM_BYPASS_FILE);

	system("echo FLUSH > /proc/fastPassNF/PASS_IPSEC_IP_PAIR");
	system("echo FLUSH > /proc/fastBrPassNF/PASS_IPSEC_IP_PAIR");

	return 0;
}

int rtk_fc_ipsec_strongswan_traffic_upstream_bypass_set(void)
{
	char *ptrLocal = NULL, *splitPosLocal = NULL,  *saveptrLocal = NULL;
	char *ptrRemote = NULL, *splitPosRemote = NULL,  *saveptrRemote = NULL;
	int ipLocalLen, ipReomteLen;
	char strLocalIp[48], strRemoteIp[48], strLocalMask[8], strRemoteMask[8];
	struct in_addr instart_address, inend_address;
	ipaddr_t inmaskLocal, inmaskRemote, inAddrLocal, inAddrRemote;
	unsigned char RemoteSubnet[IPSEC_SUBNET_CNT_MAX*IPSEC_SUBNET_LEN]={0};
	unsigned char LocalSubnet[IPSEC_SUBNET_CNT_MAX*IPSEC_SUBNET_LEN]={0};
	char strStartIp[48]={0}, strEndIp[48]={0};
	char strStartIp2[48]={0}, strEndIp2[48]={0};
	char cmd[512] = {0};
	rt_acl_filterAndQos_t aclRule;
	MIB_IPSEC_SWAN_T entry;
	unsigned int aclIdx;
	int entryNum, i;
	FILE *fp=NULL;
	int ret;

	if(!(fp = fopen(FC_IPSEC_TRAFFIC_UPSTREAM_BYPASS_FILE, "a")))
	{
		fprintf(stderr, "ERROR! %s\n", strerror(errno));
		return -2;
	}

	entryNum = mib_chain_total(MIB_IPSEC_SWAN_TBL);
	for(i = 0; i<entryNum; i++){
		mib_chain_get(MIB_IPSEC_SWAN_TBL, i, (void *)&entry);
		if(entry.Enable == 1){
			memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
			aclRule.acl_weight = WEIGHT_QOS_HIGH;
			aclRule.filter_fields |= (RT_ACL_INGRESS_PORT_MASK_BIT|RT_ACL_CARE_PORT_UNITYPE_PPTP_BIT);
			aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
			aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
			aclRule.ingress_ipv4_tagif = 1;

			memcpy(LocalSubnet, entry.LocalSubnet, sizeof(LocalSubnet));
			saveptrLocal = LocalSubnet;
			ptrLocal = strtok_r(saveptrLocal, ",", &saveptrLocal);
			while( ptrLocal != NULL ) {
				memset(strLocalIp, 0, sizeof(strLocalIp));
				memset(strLocalMask, 0, sizeof(strLocalMask));
				splitPosLocal = strchr(ptrLocal, '/');
				ipLocalLen = splitPosLocal - ptrLocal;
				if(ipLocalLen > 47) goto local_next;
				strncpy(strLocalIp, ptrLocal, ipLocalLen);
				strncpy(strLocalMask, splitPosLocal+1,7);

				printf("%s:%d, parse local subnet is %s, ip is %s, mask is %s\n",
					__FUNCTION__, __LINE__, ptrLocal, strLocalIp, strLocalMask);

				if(inet_pton(AF_INET, strLocalIp, &inAddrLocal)==1){
					aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_SIP_RANGE_BIT;
					inmaskLocal = ~0 << (sizeof(ipaddr_t)*8 - atoi(strLocalMask));
					inmaskLocal = htonl(inmaskLocal);
					aclRule.ingress_src_ipv4_addr_start = ntohl(inAddrLocal & inmaskLocal);
					instart_address.s_addr = inAddrLocal & inmaskLocal;
					inet_ntop(AF_INET, &instart_address, strStartIp, INET_ADDRSTRLEN);

					aclRule.ingress_src_ipv4_addr_end = ntohl(inAddrLocal | ~ inmaskLocal);
					//inend_address.s_addr = inAddrLocal | ~ inmaskLocal;
					inet_ntop(AF_INET, &inmaskLocal, strEndIp, INET_ADDRSTRLEN);
				}

				//deal dst info
				memcpy(RemoteSubnet, entry.RemoteSubnet, sizeof(RemoteSubnet));
				saveptrRemote = RemoteSubnet;
				printf("%s:%d, pEntry->RemoteSubnet %s, saveptrRemote %s\n",
						__FUNCTION__, __LINE__, entry.RemoteSubnet, saveptrRemote);
				ptrRemote = strtok_r(saveptrRemote, ",", &saveptrRemote);
				while(ptrRemote != NULL){
					memset(strRemoteIp, 0, sizeof(strRemoteIp));
					memset(strRemoteMask, 0, sizeof(strRemoteMask));
					splitPosRemote = strchr(ptrRemote, '/');
					ipReomteLen = splitPosRemote - ptrRemote;
					if(ipReomteLen > 47) goto remote_next;
					strncpy(strRemoteIp, ptrRemote, ipReomteLen);
					strncpy(strRemoteMask, splitPosRemote+1, 7);

					printf("%s:%d, parse remote subnet is %s, ip is %s, mask is %s\n",
						__FUNCTION__, __LINE__, ptrRemote, strRemoteIp, strRemoteMask);

					if(inet_pton(AF_INET, strRemoteIp, &inAddrRemote)==1){
						aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
						inmaskRemote = ~0 << (sizeof(ipaddr_t)*8 - atoi(strRemoteMask));
						inmaskRemote = htonl(inmaskRemote);
						aclRule.ingress_dest_ipv4_addr_start = ntohl(inAddrRemote & inmaskRemote);
						instart_address.s_addr = inAddrRemote & inmaskRemote;
						inet_ntop(AF_INET, &instart_address, strStartIp2, INET_ADDRSTRLEN);

						aclRule.ingress_dest_ipv4_addr_end = ntohl(inAddrRemote | ~inmaskRemote);
						//inend_address.s_addr = inAddrRemote | ~inmaskRemote;
						inet_ntop(AF_INET, &inmaskRemote, strEndIp2, INET_ADDRSTRLEN);
					}

					snprintf(cmd, sizeof(cmd), "echo 'ADD SIP %s SIP_MASK %s DIP %s DIP_MASK %s'> /proc/fastPassNF/PASS_IPSEC_IP_PAIR",
						strStartIp, strEndIp, strStartIp2, strEndIp2);
					printf(cmd);
					system(cmd);
					snprintf(cmd, sizeof(cmd), "echo 'ADD SIP %s SIP_MASK %s DIP %s DIP_MASK %s'> /proc/fastBrPassNF/PASS_IPSEC_IP_PAIR",
						strStartIp, strEndIp, strStartIp2, strEndIp2);
					printf(cmd);
					system(cmd);

					aclRule.action_fields = (RT_ACL_ACTION_FORWARD_GROUP_TRAP_TO_PS_BIT);
					if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0) {
						fprintf(fp, "%d\n", aclIdx);
					} else {
						printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
					}

		remote_next:
					ptrRemote = strtok_r(saveptrRemote, ",", &saveptrRemote);
					printf("%s:%d, parse remote subnet is %s, saveptrRemote %s\n",
						__FUNCTION__, __LINE__, ptrRemote, saveptrRemote);
				}

		local_next:
				ptrLocal = strtok_r(saveptrLocal, ",", &saveptrLocal);
				printf("%s:%d, parse local subnet is %s\n",
						__FUNCTION__, __LINE__, ptrLocal);
			}
		}
	}

	fclose(fp);
	return 0;
}

int rtk_fc_ipsec_strongswan_traffic_upstream_bypass_config(void)
{
	LOCK_ACL();
	rtk_fc_ipsec_strongswan_traffic_upstream_bypass_flush();
	rtk_fc_ipsec_strongswan_traffic_upstream_bypass_set();
	UNLOCK_ACL();
	return 0;
}

#endif

void rtk_ipsec_strongswan_iptable_rule_init()
{
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", TABLE_IPSEC_STRONGSWAN_PRI_POST);
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", TABLE_IPSEC_STRONGSWAN_PRI_POST);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", TABLE_IPSEC_STRONGSWAN_PRI_POST);
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", TABLE_IPSEC_STRONGSWAN_PRI_POST);
#endif
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", TABLE_IPSEC_STRONGSWAN_POST);
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", TABLE_IPSEC_STRONGSWAN_POST);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", TABLE_IPSEC_STRONGSWAN_PRE);
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", TABLE_IPSEC_STRONGSWAN_PRE);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", TABLE_IPSEC_STRONGSWAN_POST);
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", TABLE_IPSEC_STRONGSWAN_POST);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", TABLE_IPSEC_STRONGSWAN_PRE);
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", TABLE_IPSEC_STRONGSWAN_PRE);
#endif
#if defined(CONFIG_RTL8277C_SERIES)
	rtk_fc_ipsec_strongswan_traffic_bypass_config();
#endif
}

void rtk_ipsec_strongswan_iptable_rule_flush()
{
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", TABLE_IPSEC_STRONGSWAN_PRI_POST);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", TABLE_IPSEC_STRONGSWAN_PRI_POST);
#endif
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", TABLE_IPSEC_STRONGSWAN_POST);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", TABLE_IPSEC_STRONGSWAN_PRE);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", TABLE_IPSEC_STRONGSWAN_POST);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", TABLE_IPSEC_STRONGSWAN_PRE);
#endif
}

int rtk_ipsec_strongswan_iptable_rule_add(MIB_IPSEC_SWAN_Tp pEntry)
{
#ifdef CONFIG_RTK_SKB_MARK2
	struct in6_addr in6AddrLocal, in6AddrRemote, in6mask;
	ipaddr_t inmask, inAddrLocal, inAddrRemote;
	char *ptrLocal = NULL, *splitPosLocal = NULL,  *saveptrLocal = NULL;
	char *ptrRemote = NULL, *splitPosRemote = NULL,  *saveptrRemote = NULL;
	int ipLocalLen, ipReomteLen;
	char strLocalIp[48], strRemoteIp[48], strLocalMask[8], strRemoteMask[8];
	int ret = 0;
	struct in_addr instart_address, inend_address;
	struct in6_addr in6start_address, in6end_address;
	char strStartIp[48]={0}, strEndIp[48]={0};
	char src_string[512] = {0};
	char src2_string[512] = {0};
	char dst_string[512] = {0};
	char cmd[512] = {0};
	unsigned char mark0[64], mark[64];
	char tmp[128] = {0};
	char ifname[IFNAMSIZ]={0};
	unsigned char RemoteSubnet[IPSEC_SUBNET_CNT_MAX*IPSEC_SUBNET_LEN]={0};
	unsigned char LocalSubnet[IPSEC_SUBNET_CNT_MAX*IPSEC_SUBNET_LEN]={0};
	unsigned int mark_value=0, mark_mask=0, pri=0;
	char f_skb_mark_mask[32]={0}, f_skb_mark_value[32]={0};
	char rule_alias_name[256]={0}, rule_priority[32]={0};
	char devname[IFNAMSIZ]={0};
	char priority_str[32]={0};

	sprintf(devname, "%s", ALIASNAME_NAS0);
	sprintf(rule_alias_name, "ipsec_strongswan_remark_8021p_us_%s", devname);
	va_cmd(SMUXCTL, 7, 1, "--if", devname, "--tx", "--tags", "1", "--rule-remove-alias", "ipsec_strongswan_remark_8021p_us_+");

	pri = pEntry->IPSec8021P-1;
	printf("%s:%d pri %d\n", __FUNCTION__, __LINE__, pri);
	if(pri!=0){
		mark_value |= ((pri) << SOCK_MARK_8021P_START) & SOCK_MARK_8021P_BIT_MASK;
		mark_value |= SOCK_MARK_8021P_ENABLE_TO_MARK(1);
		mark_mask = SOCK_MARK_8021P_BIT_MASK|SOCK_MARK_8021P_ENABLE_BIT_MASK;
	}
	sprintf(mark0, "0x0/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);
	sprintf(mark, "0x%llx/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK, SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);

	DOCMDINIT;

	/* trap all upstream packets matched with ipsec config to PS */
	/* we only need to encrpted upstream traffic, the downstream traffic will be encrypted by another host*/
	if(inet_pton(AF_INET, pEntry->RemoteIP, &inAddrRemote)==1){
		inet_ntop(AF_INET, &inAddrRemote, strStartIp, INET_ADDRSTRLEN);
		sprintf(src_string, " -d %s", strStartIp);
		sprintf(src2_string, " -s %s", strStartIp);
	}
	else if(inet_pton(AF_INET6, pEntry->RemoteIP, &in6AddrRemote)==1){
		inet_ntop(AF_INET6, &in6AddrRemote, strStartIp, INET6_ADDRSTRLEN);
		sprintf(src_string, " -d %s", strStartIp);
		sprintf(src2_string, " -s %s", strStartIp);
	}

	if(pEntry->IPSecOutInterface != 0){ //have OutInterface
		ifGetName(pEntry->IPSecOutInterface, ifname, sizeof(ifname));
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "-t mangle -A %s -p 50 -o %s %s -j MARK2 --set-mark2 %s",
			(char *)TABLE_IPSEC_STRONGSWAN_POST,
			ifname,
			src_string,
			mark);
		DOCMDARGVS(IPTABLES, DOWAIT, cmd);
#ifdef CONFIG_IPV6
		DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
#endif

		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "-t mangle -A %s -p 51 -o %s %s -j MARK2 --set-mark2 %s",
			(char *)TABLE_IPSEC_STRONGSWAN_POST,
			ifname,
			src_string,
			mark);
		DOCMDARGVS(IPTABLES, DOWAIT, cmd);
#ifdef CONFIG_IPV6
		DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
#endif

		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "-t mangle -A %s -p 50 -i %s %s -j MARK --set-mark %s",
			(char *)TABLE_IPSEC_STRONGSWAN_PRE,
			ifname,
			src2_string,
			RMACC_MARK);
		DOCMDARGVS(IPTABLES, DOWAIT, cmd);
#ifdef CONFIG_IPV6
		DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
#endif

		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "-t mangle -A %s -p 51 -i %s %s -j MARK --set-mark %s",
			(char *)TABLE_IPSEC_STRONGSWAN_PRE,
			ifname,
			src2_string,
			RMACC_MARK);
		DOCMDARGVS(IPTABLES, DOWAIT, cmd);
#ifdef CONFIG_IPV6
		DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
#endif

		if(pri!=0){
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "-t mangle -A %s -p 50 -o %s %s -j MARK --set-mark 0x%x/0x%x",
				(char *)TABLE_IPSEC_STRONGSWAN_PRI_POST,
				ifname,
				src_string,
				mark_value,
				mark_mask);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);
#ifdef CONFIG_IPV6
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
#endif

			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "-t mangle -A %s -p 51 -o %s %s -j MARK --set-mark 0x%x/0x%x",
				(char *)TABLE_IPSEC_STRONGSWAN_PRI_POST,
				ifname,
				src_string,
				mark_value,
				mark_mask);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);
#ifdef CONFIG_IPV6
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
#endif
		}
	}
	else{
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "-t mangle -A %s -p 50 %s -j MARK2 --set-mark2 %s",
			(char *)TABLE_IPSEC_STRONGSWAN_POST,
			src_string,
			mark);
		DOCMDARGVS(IPTABLES, DOWAIT, cmd);
#ifdef CONFIG_IPV6
		DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
#endif

		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "-t mangle -A %s -p 51 %s -j MARK2 --set-mark2 %s",
			(char *)TABLE_IPSEC_STRONGSWAN_POST,
			src_string,
			mark);
		DOCMDARGVS(IPTABLES, DOWAIT, cmd);
#ifdef CONFIG_IPV6
		DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
#endif

		if(pri!=0){
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "-t mangle -A %s -p 50 %s -j MARK --set-mark 0x%x/0x%x",
				(char *)TABLE_IPSEC_STRONGSWAN_PRI_POST,
				src_string,
				mark_value,
				mark_mask);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);
#ifdef CONFIG_IPV6
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
#endif

			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "-t mangle -A %s -p 51 %s -j MARK --set-mark 0x%x/0x%x",
				(char *)TABLE_IPSEC_STRONGSWAN_PRI_POST,
				src_string,
				mark_value,
				mark_mask);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);
#ifdef CONFIG_IPV6
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
#endif
		}

	}

	memcpy(LocalSubnet, pEntry->LocalSubnet, sizeof(LocalSubnet));
	saveptrLocal = LocalSubnet;
	ptrLocal = strtok_r(saveptrLocal, ",", &saveptrLocal);
	while( ptrLocal != NULL ) {
		memset(strLocalIp, 0, sizeof(strLocalIp));
		memset(strLocalMask, 0, sizeof(strLocalMask));
		splitPosLocal = strchr(ptrLocal, '/');
		ipLocalLen = splitPosLocal - ptrLocal;
		if(ipLocalLen > 47) goto localnext;
		strncpy(strLocalIp, ptrLocal, ipLocalLen);
		strncpy(strLocalMask, splitPosLocal+1,7);

		printf("%s:%d, parse local subnet is %s, ip is %s, mask is %s\n",
			__FUNCTION__, __LINE__, ptrLocal, strLocalIp, strLocalMask);

		if(inet_pton(AF_INET, strLocalIp, &inAddrLocal)==1){
			inmask = ~0 << (sizeof(ipaddr_t)*8 - atoi(strLocalMask));
			inmask = htonl(inmask);
			instart_address.s_addr = inAddrLocal & inmask;
			inet_ntop(AF_INET, &instart_address, strStartIp, INET_ADDRSTRLEN);
			inend_address.s_addr = inAddrLocal | ~ inmask;
			inet_ntop(AF_INET, &inend_address, strEndIp, INET_ADDRSTRLEN);
			sprintf(src_string, " -m iprange --src-range %s-%s", strStartIp, strEndIp);
		}
		else if(inet_pton(AF_INET6, strLocalIp, &in6AddrLocal)==1){
			rtk_ipv6_ip_address_range(&in6AddrLocal, atoi(strLocalMask), &in6start_address, &in6end_address);
			inet_ntop(AF_INET6, &in6start_address, strStartIp, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, &in6end_address, strEndIp, INET6_ADDRSTRLEN);
			sprintf(src_string, " -m iprange --src-range %s-%s", strStartIp, strEndIp);
		}

		//deal dst info
		memcpy(RemoteSubnet, pEntry->RemoteSubnet, sizeof(RemoteSubnet));
		saveptrRemote = RemoteSubnet;
		printf("%s:%d, pEntry->RemoteSubnet %s, saveptrRemote %s\n",
				__FUNCTION__, __LINE__, pEntry->RemoteSubnet, saveptrRemote);
		ptrRemote = strtok_r(saveptrRemote, ",", &saveptrRemote);
		while(ptrRemote != NULL){
			memset(strRemoteIp, 0, sizeof(strRemoteIp));
			memset(strRemoteMask, 0, sizeof(strRemoteMask));
			splitPosRemote = strchr(ptrRemote, '/');
			ipReomteLen = splitPosRemote - ptrRemote;
			if(ipReomteLen > 47) goto remotenext;
			strncpy(strRemoteIp, ptrRemote, ipReomteLen);
			strncpy(strRemoteMask, splitPosRemote+1, 7);

			printf("%s:%d, parse remote subnet is %s, ip is %s, mask is %s\n",
				__FUNCTION__, __LINE__, ptrRemote, strRemoteIp, strRemoteMask);

			if(inet_pton(AF_INET, strRemoteIp, &inAddrRemote)==1){
				inmask = ~0 << (sizeof(ipaddr_t)*8 - atoi(strRemoteMask));
				inmask = htonl(inmask);
				instart_address.s_addr = inAddrRemote & inmask;
				inet_ntop(AF_INET, &instart_address, strStartIp, INET_ADDRSTRLEN);
				inend_address.s_addr = inAddrRemote | ~inmask;
				inet_ntop(AF_INET, &inend_address, strEndIp, INET_ADDRSTRLEN);
				sprintf(dst_string, " -m iprange --dst-range %s-%s", strStartIp, strEndIp);
			}
			else if(inet_pton(AF_INET6, strRemoteIp, &in6AddrRemote)==1){
				rtk_ipv6_ip_address_range(&in6AddrRemote, atoi(strRemoteMask), &in6start_address, &in6end_address);
				inet_ntop(AF_INET6, &in6start_address, strStartIp, INET6_ADDRSTRLEN);
				inet_ntop(AF_INET6, &in6end_address, strEndIp, INET6_ADDRSTRLEN);
				sprintf(dst_string, " -m iprange --dst-range %s-%s", strStartIp, strEndIp);
			}

			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "-t mangle -A %s %s%s -j MARK2 --set-mark2 %s",
				(char *)TABLE_IPSEC_STRONGSWAN_POST,
				src_string,
				dst_string,
				mark);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);
#ifdef CONFIG_IPV6
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
#endif
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "-t mangle -A %s %s%s -j MARK2 --set-mark2 %s",
				(char *)TABLE_IPSEC_STRONGSWAN_POST,
				src_string,
				dst_string,
				mark);
			DOCMDARGVS(IPTABLES, DOWAIT, cmd);
#ifdef CONFIG_IPV6
			DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
#endif
			if(pri!=0){
				memset(cmd, 0, sizeof(cmd));
				sprintf(cmd, "-t mangle -A %s %s%s -j MARK --set-mark 0x%x/0x%x",
					(char *)TABLE_IPSEC_STRONGSWAN_PRI_POST,
					src_string,
					dst_string,
					mark_value,
					mark_mask);
				DOCMDARGVS(IPTABLES, DOWAIT, cmd);
#ifdef CONFIG_IPV6
				DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
#endif

				memset(cmd, 0, sizeof(cmd));
				sprintf(cmd, "-t mangle -A %s %s%s -j MARK --set-mark 0x%x/0x%x",
					(char *)TABLE_IPSEC_STRONGSWAN_PRI_POST,
					src_string,
					dst_string,
					mark_value,
					mark_mask);
				DOCMDARGVS(IPTABLES, DOWAIT, cmd);
#ifdef CONFIG_IPV6
				DOCMDARGVS(IP6TABLES, DOWAIT, cmd);
#endif
			}

remotenext:
			ptrRemote = strtok_r(saveptrRemote, ",", &saveptrRemote);
			printf("%s:%d, parse remote subnet is %s, saveptrRemote %s\n",
				__FUNCTION__, __LINE__, ptrRemote, saveptrRemote);
		}

localnext:
		ptrLocal = strtok_r(saveptrLocal, ",", &saveptrLocal);
		printf("%s:%d, parse local subnet is %s\n",
				__FUNCTION__, __LINE__, ptrLocal);
	}
#endif //CONFIG_RTK_SKB_MARK2
#ifdef CONFIG_RTL_SMUX_DEV
	sprintf(f_skb_mark_mask, "0x%x", SOCK_MARK_8021P_BIT_MASK|SOCK_MARK_8021P_ENABLE_BIT_MASK);
	sprintf(f_skb_mark_value, "0x%x", mark_value);
	sprintf(rule_priority, "%d", SMUX_RULE_PRIO_VEIP);
	sprintf(priority_str, "%d", pri);
	if (pri !=0)
		va_cmd(SMUXCTL, 16, 1, "--if", "nas0", "--tx", "--tags", "1", "--filter-skb-mark", f_skb_mark_mask, f_skb_mark_value, "--set-priority", priority_str, "1", "--rule-priority", rule_priority, "--rule-alias", rule_alias_name, "--rule-append");
#endif


	return 0;
}
#endif

#ifdef _PRMT_X_TRUE_CATV_
void rtk_fc_set_CATV(unsigned char enable, char agcoffset)
{
	unsigned int value;

	//port 0  dev:0x51 reg:0x6e(bit3) for Video Power Switch
	//port 0  dev:0x51 reg:0x32 for RF_OFFSET
	rt_i2c_read(0, 0x51, 0x6e, &value);
	if(enable)
		value |= (1<<3);
	else
		value &= ~(1<<3);
	rt_i2c_write(0, 0x51, 0x6e, value);
	value = agcoffset;
	rt_i2c_write(0, 0x51, 0x32, value);
}
#endif

#if defined(SUPPORT_CLOUD_VR_SERVICE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && (defined(CONFIG_EPON_FEATURE) || defined(CONFIG_GPON_FEATURE))
#if defined(CONFIG_RTL9607C_SERIES)
#include <rtk/rate.h>
#define SPECIAL_DS_QUEUE_FILE "/var/fc_special_ds_queue_meter"
#define SPECIAL_DS_QUEUE_ACL_FILE "/var/fc_special_ds_queue_meter_acl"
int rtk_clearSwitchDSQueueLimit(void)
{
	int p, q, m, ret=0;
	unsigned int r;
	FILE *fp = NULL;

	if((fp = fopen(SPECIAL_DS_QUEUE_FILE, "r")))
	{
		while(!feof(fp))
		{
			if(fscanf(fp, "%d,%d,%d,%u\n", &p, &q, &m, &r) == 4)
			{
				rtk_rate_egrQueueBwCtrlMeterIdx_set(p, q, 0);
				rtk_rate_egrQueueBwCtrlEnable_set(p, q, 0);
				rtk_rate_shareMeter_set(m, RTL9607C_METER_RATE_MAX, 1);
			}
		}
		fclose(fp);
		unlink(SPECIAL_DS_QUEUE_FILE);
	}

	if((fp = fopen(SPECIAL_DS_QUEUE_ACL_FILE, "r")))
	{
		while(!feof(fp))
		{
			if(fscanf(fp, "%d,%d,%*s\n", &p, &q) == 2)
			{
#ifdef CONFIG_COMMON_RT_API
				if((ret = rt_acl_filterAndQos_del(p)) != RT_ERR_OK)
				{
					printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, p, ret);
				}
#endif
			}
		}
		fclose(fp);
		unlink(SPECIAL_DS_QUEUE_FILE);
	}

	return 0;
}
int rtk_setupSwitchDSQueueLimit(int queue, unsigned int rate)
{
	int i, j, p, q, m, ret=0;
	unsigned int r;
	rtk_enable_t ifIclude;
	FILE *fp = NULL;

	if((fp = fopen(SPECIAL_DS_QUEUE_FILE, "w")))
	{
		ret = 0;
		for(i=0;i<ELANVIF_NUM;i++)
		{
			p = rtk_port_get_lan_phyID(i);
			if(p != -1)
			{
				for(m=(8*(p+1))-1;m>(8*(p));m--)
				{
					if(rtk_rate_shareMeter_get(m, &r, &ifIclude) == RT_ERR_OK && r == RTL9607C_METER_RATE_MAX)
					{
						if(rtk_rate_egrQueueBwCtrlMeterIdx_set(p, queue, m) == RT_ERR_OK &&
							rtk_rate_egrQueueBwCtrlEnable_set(p, queue, 1) == RT_ERR_OK &&
							rtk_rate_shareMeterMode_set(m, METER_MODE_BIT_RATE) == RT_ERR_OK &&
							rtk_rate_shareMeter_set(m, rate, 0) == RT_ERR_OK)
						{
							fprintf(fp, "%d,%d,%d,%u\n", p, queue, m, rate);
							ret++;
						}
						else
						{
							rtk_rate_egrQueueBwCtrlMeterIdx_set(p, queue, 0);
							rtk_rate_egrQueueBwCtrlEnable_set(p, queue, 0);
							rtk_rate_shareMeter_set(m, RTL9607C_METER_RATE_MAX, 1);
						}
						break;
					}
				}
			}
		}
		fclose(fp);
	}
	if(ret <= 0) unlink(SPECIAL_DS_QUEUE_FILE);
	return (ret>0)?1:-1;
}

int rtk_addSwitchDSQueueLimitACL(unsigned char *pmac, int queue)
{
#ifdef CONFIG_COMMON_RT_API
	rt_acl_filterAndQos_t aclRule;
	unsigned int aclIdx = 0;
	int ret = 0;
	FILE *fp = NULL;

	if(pmac == NULL || queue < 0 || queue > 7) return -1;

	if((fp = fopen(SPECIAL_DS_QUEUE_ACL_FILE, "a")) || (fp = fopen(SPECIAL_DS_QUEUE_ACL_FILE, "w")))
	{
		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_QOS_LOW;
		aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
		aclRule.filter_fields |= RT_ACL_INGRESS_DMAC_BIT;
		memcpy(&aclRule.ingress_dmac, pmac, MAC_ADDR_LEN);
		aclRule.action_fields = (RT_ACL_ACTION_PRIORITY_GROUP_ACL_PRIORITY_BIT);
		aclRule.action_priority_group_acl_priority = queue;
		if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
			fprintf(fp, "%d,%d,%02X-%02X-%02X-%02X-%02X-%02X\n", aclIdx, queue,
						pmac[0], pmac[1], pmac[2], pmac[3], pmac[4], pmac[5]);
		else
			printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

		fclose(fp);
	}
	return ret;
#else
	return 0;
#endif
}
#endif

int rtk_specialAdjustSMP(void)
{
	unsigned int chipId;
	unsigned int rev;
	unsigned int subType;

#ifdef CONFIG_COMMON_RT_API
	rt_switch_version_get(&chipId, &rev, &subType);
#else
	rtk_switch_version_get(&chipId, &rev, &subType);
#endif

	switch(chipId)
	{
#if defined(CONFIG_RTL9607C_SERIES)
		case RTL9603CVD_CHIP_ID:/*9603D*/
			break;
		case RTL9607C_CHIP_ID:
			switch(subType)
			{
			case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA4:
			case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA5:
			case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA6:
				break;
			case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA6:
			case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA7:
				// modify GMAC0 to CPU 1
				va_cmd("/bin/sh", 2, 1, "-c", "echo 1 > /proc/irq/59/smp_affinity");
				// modify GMAC1 to CPU 2
				va_cmd("/bin/sh", 2, 1, "-c", "echo 4 > /proc/irq/61/smp_affinity");
				// modify WIFI to CPU 3
				va_cmd("/bin/sh", 2, 1, "-c", "echo 8 > /proc/irq/73/smp_affinity");
				break;
			}
			break;
#endif
		default:
			printf("The chipId is not support=%d \n", chipId);
	}

	return 0;
}
#endif

#ifdef CONFIG_USER_AVALANCH_DETECT
#if defined(CONFIG_CMCC)
void rtk_avalanch_permit_server_acl(unsigned char mac[], unsigned char action)
{
	rt_acl_filterAndQos_t aclRule;
	int aclIdx = 0;
	FILE *fp = NULL;
	int ret;


	if(action)
	{
		if(mac==NULL)
			return;
		fp = fopen(RTK_ACL_AVLANCHE_PERMIT_RULE_FILE, "w");
		if (fp)
		{
			//permit avalanch server mac
			memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
			aclRule.acl_weight = 65536;
			aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
			aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
			memcpy(aclRule.ingress_smac, mac, ETHER_ADDR_LEN);
			aclRule.filter_fields |= RT_ACL_INGRESS_SMAC_BIT;
			aclRule.action_fields = RT_ACL_ACTION_FORWARD_GROUP_PERMIT_BIT;
			if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == RT_ERR_OK) {
				printf("Add rt acl index:%d\n", aclIdx);
				fprintf(fp, "%d\n", aclIdx);
			} else {
				printf("[%s@%d] rt_acl_filterAndQos_add failed! (ret = %d)\n",__func__,__LINE__, ret);
				return;
			}

			//drop other pkt from wan
			memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
			aclRule.acl_weight = 65536;
			aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
			aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
			aclRule.action_fields = RT_ACL_ACTION_FORWARD_GROUP_DROP_BIT;
			if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == RT_ERR_OK) {
				printf("Add rt acl index:%d\n", aclIdx);
				fprintf(fp, "%d\n", aclIdx);
			} else {
				printf("[%s@%d] rt_acl_filterAndQos_add failed! (ret = %d)\n",__func__,__LINE__, ret);
				return;
			}
			fclose(fp);
		}
	}
	else
	{
		if(!(fp = fopen(RTK_ACL_AVLANCHE_PERMIT_RULE_FILE, "r")))
		{
			printf("[%s@%d] open %s  failed!\n",__func__,__LINE__, RTK_ACL_AVLANCHE_PERMIT_RULE_FILE);
			return;
		}

		while(fscanf(fp, "%d\n", &aclIdx) != EOF)
		{
			if((ret = rt_acl_filterAndQos_del(aclIdx)) != RT_ERR_OK)
				printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, aclIdx, ret);
		}

		fclose(fp);
		unlink(RTK_ACL_AVLANCHE_PERMIT_RULE_FILE);
	}
}
#endif
#ifdef CONFIG_RTL9607C_SERIES
int rtk_avalanch_07c_set(void)
{
	rt_acl_filterAndQos_t aclRule;
	MIB_CE_ATM_VC_T vcEntry;
	int i = 0, aclIdx = 0, ret = -1, vcTotal = -1;
	FILE *fp = NULL;
	int mib_id = CWMP_ACS_URL;
	int aclAdded = 0;
	struct in_addr sin_addr_start = {0}, sin_addr_end = {0};
	char str[INET_ADDRSTRLEN];


	fp = fopen(RTK_ACL_AVLANCHE_RULE_FILE, "w");
	if (fp)
	{

		vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < vcTotal; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
				continue;
			//find first bridge wan
			if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
			{
				//Avalanche ACL rule 1
				memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
				aclRule.acl_weight = 1200/*WEIGHT_QOS_HIGH*/;
				aclRule.action_fields = RT_ACL_ACTION_FORWARD_GROUP_PERMIT_BIT;
				aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
				aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();
				if (vcEntry.itfGroup != 0) //PHYPORT(LAN port)
					aclRule.ingress_port_mask = rtk_port_get_lan_phyMask(vcEntry.itfGroup);
				else if (vcEntry.applicationtype & X_CT_SRV_INTERNET) //PHYPORT(ALL_LANPORT)
					aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
				//printf("%s: aclRule.ingress_port_mask = %d!\n", __FUNCTION__, aclRule.ingress_port_mask);

				aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_SIP_RANGE_BIT;
				inet_pton(AF_INET, "20.10.10.100", &sin_addr_start);
				aclRule.ingress_src_ipv4_addr_start = ntohl(sin_addr_start.s_addr);
				inet_pton(AF_INET, "20.10.10.103", &sin_addr_end);
				aclRule.ingress_src_ipv4_addr_end = ntohl(sin_addr_end.s_addr);

				if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
				{
					fprintf(fp, "%d\n", aclIdx);
				}
				else
					printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);

				//Avalanche ACL rule 2
				memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
				aclRule.acl_weight = 1200/*WEIGHT_QOS_HIGH*/;
				aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
				if (vcEntry.itfGroup != 0) //PHYPORT(LAN port)
					aclRule.ingress_port_mask = rtk_port_get_lan_phyMask(vcEntry.itfGroup);
				else if (vcEntry.applicationtype & X_CT_SRV_INTERNET) //PHYPORT(ALL_LANPORT)
					aclRule.ingress_port_mask = rtk_port_get_all_lan_phyMask();
				//printf("%s: aclRule.ingress_port_mask = %d!\n", __FUNCTION__, aclRule.ingress_port_mask);

				aclRule.action_fields = RT_ACL_ACTION_PRIORITY_GROUP_ACL_PRIORITY_BIT;
				aclRule.action_priority_group_acl_priority = 7;

				if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
				{
					aclAdded = 1;
					fprintf(fp, "%d\n", aclIdx);
				}
				else
					printf("[%s@%d] rt_acl_filterAndQos_add Avalanche rule failed! (ret = %d)\n",__func__,__LINE__, ret);
			}
		}
		fclose(fp);
	}

	if (!aclAdded)
		unlink(RTK_ACL_AVLANCHE_RULE_FILE);

	return 0;
}

int rtk_avalanch_07c_del(void)
{
	rt_acl_filterAndQos_t aclRule;
	int acl_idx = 0, ret = -1;
	FILE *fp = NULL;

	if(!(fp = fopen(RTK_ACL_AVLANCHE_RULE_FILE, "r")))
	{
		printf("[%s@%d] open %s  failed!\n",__func__,__LINE__, RTK_ACL_AVLANCHE_RULE_FILE);
		return -1;
	}

	while(fscanf(fp, "%d\n", &acl_idx) != EOF)
	{
		if((ret = rt_acl_filterAndQos_del(acl_idx)) != RT_ERR_OK)
			printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n",__func__,__LINE__, acl_idx, ret);
	}

	fclose(fp);
	unlink(RTK_ACL_AVLANCHE_RULE_FILE);
	return 0;
}
#endif
#endif

int rtk_fc_flow_delete(int ipv4, const char *proto,
						const char *sip, const unsigned short sport,
						const char *dip, const unsigned short dport)
{
	int ret = -1;
	rt_flow_ops_data_t key;

	//printf("===> [%s:%d] ipv4 %d\n", __FUNCTION__, __LINE__, ipv4);
#ifndef CONFIG_IPV6
	if(!ipv4) return -1;
#endif

	memset(&key, 0, sizeof(key));
	key.data_delFlow.flowKeyType = RT_FLOW_OP_FLOW_KEY_TYPE_PATTERN;

	if(proto && proto[0]) {
		key.data_delFlow.flowKey.flowPattern.l4proto_valid = 1;
		if(!strcasecmp(proto, "TCP"))
			key.data_delFlow.flowKey.flowPattern.l4proto = RT_FLOW_OP_FLOW_PATTERN_L4PROTO_TCP;
		else if(!strcasecmp(proto, "UDP"))
			key.data_delFlow.flowKey.flowPattern.l4proto = RT_FLOW_OP_FLOW_PATTERN_L4PROTO_UDP;
		else
			return -1;
	}

	if(sip && sip[0]) {
		if(ipv4) {
			struct in_addr v4addr;
			inet_pton(AF_INET, sip, &v4addr);
			key.data_delFlow.flowKey.flowPattern.sip4_valid = 1;
			key.data_delFlow.flowKey.flowPattern.sip4 = htonl(v4addr.s_addr);
		}
#ifdef CONFIG_IPV6
		else {
			struct in6_addr v6addr;
			inet_pton(AF_INET6, sip, &v6addr);
			key.data_delFlow.flowKey.flowPattern.sip6_valid = 1;
			memcpy(key.data_delFlow.flowKey.flowPattern.sip6, &v6addr, IPV6_ADDR_LEN);
		}
#endif
	}

	if(dip && dip[0]) {
		if(ipv4) {
			struct in_addr v4addr;
			inet_pton(AF_INET, dip, &v4addr);
			key.data_delFlow.flowKey.flowPattern.dip4_valid = 1;
			key.data_delFlow.flowKey.flowPattern.dip4 = htonl(v4addr.s_addr);
		}
#ifdef CONFIG_IPV6
		else {
			struct in6_addr v6addr;
			inet_pton(AF_INET6, dip, &v6addr);
			key.data_delFlow.flowKey.flowPattern.dip6_valid = 1;
			memcpy(key.data_delFlow.flowKey.flowPattern.dip6, &v6addr, IPV6_ADDR_LEN);
		}
#endif
	}

	if(sport) {
		key.data_delFlow.flowKey.flowPattern.sport_valid = 1;
		key.data_delFlow.flowKey.flowPattern.sport = sport;
	}

	if(dport) {
		key.data_delFlow.flowKey.flowPattern.dport_valid = 1;
		key.data_delFlow.flowKey.flowPattern.dport = dport;
	}

	ret = rt_flow_operation_add(RT_FLOW_OPS_DELETE_FLOW, &key);
	//printf("===> [%s:%d] ret %d\n", __FUNCTION__, __LINE__, ret);
	ret = (ret == RT_ERR_OK) ? 0 : -1;

	return ret;
}

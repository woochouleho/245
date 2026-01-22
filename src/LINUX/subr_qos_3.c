/*
 *      System routines for IP QoS
 *
 */
#define _GNU_SOURCE

#include "debug.h"
#include "utility.h"
#include <signal.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "sockmark_define.h"
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_epon.h>
#include <rtk/rt/rt_gpon.h>
#include "fc_api.h"
#endif

#ifdef CONFIG_USER_AP_HW_QOS
#include <rtk/qos.h>
#include <rtk/rate.h>
#endif

#define QOS_SETUP_DEBUG   1// for qos debug
#define MAX_PORT(val1, val2) (val1>val2?val1:val2)
#define MIN_PORT(val1, val2) (val1<val2?val1:val2)
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
static QOS_WAN_PORT_WANITF_T wan_port_itfs[SW_LAN_PORT_NUM];
static char QOS_IP_CHAIN[SW_LAN_PORT_NUM][32];
static char QOS_EB_US_CHAIN[SW_LAN_PORT_NUM][32];

static int qos_wan_ports[SW_LAN_PORT_NUM] = {0};
static int current_qos_wan_port = -1;
static char speed_limit_set_once = 0;
static unsigned short etherTypeMap[] = {0x0800, 0x0806, 0x8864, 0x8100, 0x86DD};//ipv4,arp,pppoe,802.1q,ipv6,
#else
const char QOS_IP_CHAIN[] =  "qos_ip_rules";
const char QOS_EB_US_CHAIN[] =  "qos_eb_us_rules";
#endif
const char QOS_EB_DS_CHAIN[] =  "qos_eb_ds_rules";
const char QOS_IPTABLE_US_ACTION_CHAIN[] =  "qos_iptable_action_us_rules";
const char QOS_IPTABLE_DS_ACTION_CHAIN[] =  "qos_iptable_action_ds_rules";
const char CAR_US_FILTER_CHAIN[] =  "car_filter_us";
const char CAR_DS_FILTER_CHAIN[] =  "car_filter_ds";
const char CAR_US_TRAFFIC_CHAIN[] =  "car_traffic_qos_us";
const char CAR_DS_TRAFFIC_CHAIN[] =  "car_traffic_qos_ds";
const char CAR_TRAFFIC_EBT_CHAIN[] =  "car_traffic_qos_ebt";

const char PORT_RULE_FILE[] = "/tmp/droprule";

static unsigned int qos_setup_debug = 3;

#ifdef QOS_SETUP_DEBUG
#define QOS_SETUP_PRINT_FUNCTION                    \
    do{if(qos_setup_debug&0x1) fprintf(stderr,"%s: %s  %d\n", __FILE__, __FUNCTION__,__LINE__);}while(0);

#define QOS_SETUP_PRINT(fmt, args...)        \
    do{if(qos_setup_debug&0x2) fprintf(stderr,fmt, ##args);}while(0)
#else
#define QOS_SETUP_PRINT_FUNCTION do{}while(0);
#define QOS_SETUP_PRINT(fmt, args...) do{}while(0)
#endif

#define RTP_RULE_NUM_MAX     256

#define QOS_WAN_ENABLE		0x01
#define QOS_WAN_VLAN_ENABLE	0x02
#define QOS_WAN_BRIDGE		0x04
#define QOS_WAN_TR069		0x08

#define QOS_APP_NONE		0
#define QOS_APP_VOIP		1
#define QOS_APP_TR069		2

// QoS definitions, and pass these information to linux kernel
#define QOS_8021P_OFFSET 		0	
#define QOS_8021P_MASK		(0x7<<QOS_8021P_OFFSET)

#define QOS_SWQID_OFFSET		4
#define QOS_SWQID_MASK		(0xF<<QOS_SWQID_OFFSET)

#define QOS_1P_ENABLE		(1<<3)
#define QOS_ENTRY_OFFSET 	8
#define QOS_ENTRY_MASK		(0x3F<<QOS_ENTRY_OFFSET)

#define QOS_MARK_MASK		QOS_ENTRY_MASK|QOS_1P_ENABLE|QOS_SWQID_MASK|QOS_8021P_MASK

#ifdef QOS_SUPPORT_RTP
#define RTP_PORT_FILE	"/proc/net/ip_qos_rtp"
#endif

static char* proto2str[] = {
    [0]" ",
    [1]"-p TCP",
    [2]"-p UDP",
	[3]"-p ICMP",
};

static char* proto2str2layer[] = {
    [0]" ",
    [1]"--ip-proto 6",
    [2]"--ip-proto 17",
	[3]"--ip-proto 1",
};
 
#ifdef CONFIG_IPV6
static char* proto2str_v6[] = {
    [0]" ",
    [1]"-p TCP",
    [2]"-p UDP",
	[3]"-p ICMPV6",
};

static char* proto2str2layer_v6[] = {
    [0]" ",
    [1]"--ip6-proto 6",
    [2]"--ip6-proto 17",
	[3]"--ip6-proto 58",
};
#endif
enum {
	QOS_8021P_0_MARKING = 0,
	QOS_8021P_TRANSPARENT,
	QOS_8021P_REMARKING,
};

static unsigned int current_uprate;
LANITF_MAP_T lan_itf_map;
LANITF_MAP_T lan_itf_map_vid;
int getQosEnable(void);
int getDownstreamQosEnable(void);
unsigned int getUpLinkRate(void);
unsigned int getDownLinkRate(void);


#ifdef CONFIG_RTL_SMUX_DEV
int rtk_qos_smuxdev_qos_us_rule_set(unsigned int mark, int order);
int rtk_qos_smuxdev_qos_us_rule_flush(void);
int rtk_qos_smuxdev_qos_ds_rule_set(unsigned int mark, int order);
int rtk_qos_smuxdev_qos_ds_rule_flush(void);
#endif

#if defined(WLAN_SUPPORT) && defined(CONFIG_QOS_SUPPORT_DOWNSTREAM)
void clear_downstream_queues(void);
void setup_downstream_queues(void);
#endif

#ifdef CONFIG_HWNAT
#include "hwnat_ioctl.h"

static int fill_ip_family_common_rule(struct rtl867x_hwnat_qos_rule *qos_rule, MIB_CE_IP_QOS_Tp qEntry){
	//printf("qEntry->sip %x\n", *((struct in_addr *)qEntry->sip));
	//printf("qEntry->dip %x\n", *((struct in_addr *)qEntry->dip));
	qos_rule->Format_IP_SIP = (((struct in_addr *)qEntry->sip)->s_addr);
	if(qEntry->smaskbit != 0)
		qos_rule->Format_IP_SIP_M = ~((1<<(32-qEntry->smaskbit)) - 1);
	qos_rule->Format_IP_DIP = (((struct in_addr *)qEntry->dip)->s_addr);
	if(qEntry->dmaskbit != 0)
		qos_rule->Format_IP_DIP_M = ~((1<<(32-qEntry->dmaskbit)) - 1);
	return 0;
}

static int fill_ip_rule(struct rtl867x_hwnat_qos_rule *qos_rule, MIB_CE_IP_QOS_Tp qEntry){
	fill_ip_family_common_rule(qos_rule, qEntry);
	// protocol
	if (qEntry->protoType != PROTO_NONE) {//not none
		if((qEntry->flags&EXC_PROTOCOL) != EXC_PROTOCOL){//EXC_PROTOCOL didn't set, we don't support
			qos_rule->Format_IP_Proto_M = 0xff;
			if (qEntry->protoType == PROTO_TCP)
				qos_rule->Format_IP_Proto = 6;
			else if (qEntry->protoType == PROTO_UDP)
				qos_rule->Format_IP_Proto = 17;
			else if (qEntry->protoType == PROTO_ICMP)
				qos_rule->Format_IP_Proto = 1;
			else
				qos_rule->Format_IP_Proto_M = 0x0;
		}
	}
	return 0;
}

static int fill_tcp_rule(struct rtl867x_hwnat_qos_rule *qos_rule, MIB_CE_IP_QOS_Tp qEntry){
	fill_ip_family_common_rule(qos_rule, qEntry);
	qos_rule->Format_TCP_SPORT_Sta = qEntry->sPort;
	qos_rule->Format_TCP_SPORT_End = qEntry->sPortRangeMax;
	qos_rule->Format_TCP_DPORT_Sta = qEntry->dPort;
	qos_rule->Format_TCP_DPORT_End = qEntry->dPortRangeMax;
	return 0;
}

static int fill_udp_rule(struct rtl867x_hwnat_qos_rule *qos_rule, MIB_CE_IP_QOS_Tp qEntry){
	fill_ip_family_common_rule(qos_rule, qEntry);
	qos_rule->Format_UDP_SPORT_Sta = qEntry->sPort;
	qos_rule->Format_UDP_SPORT_End = qEntry->sPortRangeMax;
	qos_rule->Format_UDP_DPORT_Sta = qEntry->dPort;
	qos_rule->Format_UDP_DPORT_End = qEntry->dPortRangeMax;
	return 0;
}

static int hwnat_qos_translate_rule(MIB_CE_IP_QOS_Tp qEntry){
	hwnat_ioctl_cmd hifr;
	struct hwnat_ioctl_qos_cmd hiqc;
	struct rtl867x_hwnat_qos_rule *qos_rule;
	MIB_CE_ATM_VC_T pvcEntry;

	if ( qEntry->outif != DUMMY_IFINDEX ) {
		if( (MEDIA_INDEX(qEntry->outif) != MEDIA_ETH)
		  #ifdef CONFIG_PTMWAN
		   && (MEDIA_INDEX(qEntry->outif) != MEDIA_PTM)
		  #endif /*CONFIG_PTMWAN*/
		   )
			return -1;
	}
	memset(&hiqc, 0, sizeof(hiqc));
	hifr.type = HWNAT_IOCTL_QOS_TYPE;
	hifr.data = &hiqc;
	hiqc.type = QOSRULE_CMD;
	qos_rule = &(hiqc.u.qos_rule.qos_rule_pattern);
	//type decide
	//Warning! now we don't support exclude....
	//and we only try IP TYPE now...
	qos_rule->rule_type = RTL867x_IPQos_Format_IP;//default
	
	switch(qEntry->ipqos_rule_type)
	{
		case IPQOS_PORT_BASE:
			strncpy(qos_rule->Format_SRCPort_NETIFNAME, ELANVIF[qEntry->phyPort-1], strlen(ELANVIF[qEntry->phyPort-1]));
			//printf("qos_rule->Format_SRCPort_NETIFNAME=%s, ALIASNAME_ELAN0=%s\n", qos_rule->Format_SRCPort_NETIFNAME, ALIASNAME_ELAN0);
			qos_rule->rule_type = RTL867x_IPQos_Format_srcPort;
			break;
		case IPQOS_MAC_BASE:
		case IPQOS_ETHERTYPE_BASE:
			qos_rule->rule_type = RTL867x_IPQos_Format_Ethernet;
			break;
		case IPQOS_IP_PROTO_BASE:
			if (qEntry->protoType == PROTO_TCP)
				qos_rule->rule_type = RTL867x_IPQos_Format_TCP;
			else if (qEntry->protoType == PROTO_UDP)
				qos_rule->rule_type = RTL867x_IPQos_Format_UDP;
			else //if (qEntry->protoType == PROTO_ICMP)
				qos_rule->rule_type = RTL867x_IPQos_Format_IP;
			break;
		case IPQOS_8021P_BASE:
			qos_rule->rule_type = RTL867x_IPQos_Format_8021p;
			break;
		case IPQOS_TYPE_AUTO:
			//for Yueme Project, we didn't setup the qos type value.
			break;
		default:
			printf("Error! No HW QoS rule type matched, should not go here!\n");
			break;
	}

RULE_TYPE_OK:

	//remark dscp
/*DSCP modify Boyce 2014-06-06*/
                hiqc.u.qos_rule.qos_rule_remark_dscp = (qEntry->m_dscp>>2) + (qEntry->m_dscp&0x1);

	//802.1p remark
	if(qEntry->m_1p != 0){
		hiqc.u.qos_rule.qos_rule_remark_8021p = qEntry->m_1p;	// 802.1p
	}

#if defined(CONFIG_RTL8685) || defined(CONFIG_RTL8685S) || defined(CONFIG_RTL8685SB)
	//vid remark
	if(qEntry->m_vid !=0){
		hiqc.u.qos_rule.qos_rule_vid = qEntry->m_vid;	// VLAN ID
	}
#endif
	//q_index
	hiqc.u.qos_rule.qos_rule_queue_index = qEntry->prior;  //(1, 2, 3, 4)
	//fill the rule
	switch(qos_rule->rule_type){
		case RTL867x_IPQos_Format_srcPort:
			break;
		case RTL867x_IPQos_Format_Ethernet:			
			{
				uint8 maskMac[MAC_ADDR_LEN]={0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
				memcpy(qos_rule->Format_Ethernet_SMAC.octet, qEntry->smac, MAC_ADDR_LEN);
				memcpy(qos_rule->Format_Ethernet_DMAC.octet, qEntry->dmac, MAC_ADDR_LEN);
				if (memcmp(qEntry->smac, EMPTY_MAC, MAC_ADDR_LEN))
					memcpy(qos_rule->Format_Ethernet_SMAC_M.octet, maskMac, MAC_ADDR_LEN);
				if (memcmp(qEntry->dmac, EMPTY_MAC, MAC_ADDR_LEN))
					memcpy(qos_rule->Format_Ethernet_DMAC_M.octet, maskMac, MAC_ADDR_LEN);

                                if(0 != qEntry->ethType) {
					qos_rule->Format_Ethernet_TYPE = qEntry->ethType;
                                        qos_rule->Format_Ethernet_TYPE_M = 0xffff;
			
			}
           }
			break;
                        
		case RTL867x_IPQos_Format_IP:
			fill_ip_rule(qos_rule, qEntry);
                        /*DSCP modify Boyce 2014-06-06
                        qEntry->qosDscp  
                        b'0=0 don't care dscp rule 
                        b'0=1 also compare dscp value  
                        */
                                if(qEntry->qosDscp & 0x1)
                                {
                                        qos_rule->Format_IP_TOS =qEntry->qosDscp-1;
                                        qos_rule->Format_IP_TOS_M= 0xfC;  //only mask dscpField
                                }else
                                {
                                        qos_rule->Format_IP_TOS_M= 0x0;
                                }
			break;

		case RTL867x_IPQos_Format_TCP:
			fill_tcp_rule(qos_rule, qEntry);
                        /*DSCP modify Boyce 2014-06-06
                        qEntry->qosDscp  
                        b'0=0 don't care dscp rule
                        b'0=1 also compare dscp value  
                        */
                                if(qEntry->qosDscp & 0x1)
                                {
                                        qos_rule->Format_IP_TOS =qEntry->qosDscp-1;
                                        qos_rule->Format_IP_TOS_M= 0xfC;  //only mask dscpField
                                }else
                                {
                                        qos_rule->Format_IP_TOS_M= 0x0;
                                }
			break;

		case RTL867x_IPQos_Format_UDP:
			fill_udp_rule(qos_rule, qEntry);
                        /*DSCP modify Boyce 2014-06-06
                        qEntry->qosDscp  
                        b'0=0 don't care dscp rule 
                        b'0=1 also compare dscp value  
                        */
                                if(qEntry->qosDscp & 0x1)
                                {
                                        qos_rule->Format_IP_TOS =qEntry->qosDscp-1;
                                        qos_rule->Format_IP_TOS_M= 0xfC;  //only mask dscpField
                                }else
                                {
                                        qos_rule->Format_IP_TOS_M= 0x0;
                                }
			break;
                        
		// Added by Mason Yu. for 802.1p match
		case RTL867x_IPQos_Format_8021p:
			//printf("***** set hwnat rule for 801.1p match(%d)\n", (qEntry->vlan1p-1));
			qos_rule->Format_8021P_PRIORITY = qEntry->vlan1p-1;
			qos_rule->Format_8021P_PRIORITY_M = 0x07;
			break;
		default:
			break;
	}
	send_to_hwnat(hifr);
	return 0;
}

static int hwnat_qos_queue_setup(void){
	hwnat_ioctl_cmd hifr;
	struct hwnat_ioctl_qos_cmd hiqc;
	struct rtl867x_hwnat_qos_queue *qos_queue;
	unsigned char policy=0;
	int qEntryNum=0, j=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	unsigned int total_bandwidth = 0;

	mib_get_s(MIB_QOS_POLICY, (void*)&policy, sizeof(policy));
	if((qEntryNum=mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		return 0;

	total_bandwidth = getUpLinkRate();

	memset(&hiqc, 0, sizeof(hiqc));
	hifr.type = HWNAT_IOCTL_QOS_TYPE;
	hifr.data = &hiqc;
	hiqc.type = OUTPUTQ_CMD;
	qos_queue = &(hiqc.u.qos_queue);
	qos_queue->action = QDISC_ENABLE;
	if(policy == PLY_PRIO){
		qos_queue->sp_num = 4;
	}else if(policy == PLY_WRR){
		qos_queue->wrr_num = 4;
		for(j=0; j<qEntryNum; j++){
			if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, j, (void*)&qEntry))
				continue;
			if ( !qEntry.enable)
				continue;				

			qos_queue->weight[j] = qEntry.weight;
			qos_queue->ceil[j] = (total_bandwidth*1024);
		}
	}
	//qos_queue->bandwidth = -1;
	qos_queue->default_queue = 3;
	send_to_hwnat(hifr);
	return 0;
}

static int hwnat_qos_bandwidth_setup(int total_tandwidth){
	hwnat_ioctl_cmd hifr;
	struct hwnat_ioctl_qos_cmd hiqc;

	memset(&hiqc, 0, sizeof(hiqc));
	hifr.type = HWNAT_IOCTL_QOS_TYPE;
	hifr.data = &hiqc;
	hiqc.type = OUTPUTQ_BWCTL_CMD;
	hiqc.qos_total_bandwidth = total_tandwidth;
	send_to_hwnat(hifr);
	return 0;
}

static int hwnat_qos_queue_stop(void){
	hwnat_ioctl_cmd hifr;
	struct hwnat_ioctl_qos_cmd hiqc;
	struct rtl867x_hwnat_qos_queue *qos_queue;

	memset(&hiqc, 0, sizeof(hiqc));
	hifr.type = HWNAT_IOCTL_QOS_TYPE;
	hifr.data = &hiqc;
	hiqc.type = OUTPUTQ_CMD;
	qos_queue = &(hiqc.u.qos_queue);
	qos_queue->action = QDISC_DISABLE;
	send_to_hwnat(hifr);
	return 0;
}

static int hwnat_wanif_rule(void)
{
	hwnat_ioctl_cmd hifr;
	struct hwnat_ioctl_qos_cmd hiqc;
	struct rtl867x_hwnat_qos_rule *qos_rule;
	MIB_CE_ATM_VC_T Entry;
	int vcTotal, i;
	char ipWanif[IFNAMSIZ];
	memset(&hiqc, 0, sizeof(hiqc));
	hifr.type = HWNAT_IOCTL_QOS_TYPE;
	hifr.data = &hiqc;
	hiqc.type = QOSRULE_CMD;
	qos_rule = &(hiqc.u.qos_rule.qos_rule_pattern);
	unsigned int portmask;


	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);

	for (i = 0; i < vcTotal; i++){

		portmask=0;
		mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry);
		if(Entry.vlan==0)
			continue;
		if(Entry.vprio==0)
			continue;
		ifGetName(Entry.ifIndex, ipWanif, sizeof(ipWanif));
		{
		int var;
		for(var = 0; var < MAX_LANIF_NUM; var ++)
		portmask|=Entry.itfGroup & (0x1 << var) ;
		}
		memcpy(qos_rule->outIfname, ipWanif, strlen(ipWanif));
		qos_rule->rule_type = RTL867x_IPQos_Format_dstIntf_1pRemark;
		hiqc.u.qos_rule.qos_rule_remark_8021p = Entry.vprio;	// 802.1p: 0 means disalbe
		//q_index
		hiqc.u.qos_rule.qos_rule_queue_index = 3;
		send_to_hwnat(hifr);
	}



}

int setWanIF1PMark(void)
{
	int vcTotal, i;

	MIB_CE_ATM_VC_T Entry;
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	int idx;
	unsigned int WanIF1PEnable=0;

	for (i = 0; i < vcTotal; i++)
	{
		mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry);

		if(Entry.vlan==0)
			continue;
		if(Entry.vprio==0)
			continue;
		WanIF1PEnable=1;
	}
	if(WanIF1PEnable)
		//AUG_PRT("HWQOSDisable\n");
		//hwnat_single_queue_setup();
		hwnat_wanif_rule();
	return 1;
}

#else
static int hwnat_qos_default_rule(void){
	return 0;
}
static int hwnat_qos_translate_rule(MIB_CE_IP_QOS_Tp qEntry){
	return 0;
}
static int hwnat_qos_queue_setup(void){
	return 0;
}
static int hwnat_qos_queue_stop(void){
	return 0;
}
static int hwnat_qos_bandwidth_setup(int total_tandwidth){
	return 0;
}
static int hwnat_wanif_rule(void) {
	return 0;
}
int setWanIF1PMark(void) {
	return 0;
}
#endif
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
// get root device like eth0.2 
static const char *getQosRootWanName(int port)
{
	if(port<0 || port>=SW_LAN_PORT_NUM)
		return NULL;
	return ELANRIF[port];
}

// init the chain name of eb/ip tables used in qos classification	
static void init_table_rule_chain(void)
{
	int i;
	memset(QOS_IP_CHAIN,0,sizeof(QOS_IP_CHAIN));
	memset(QOS_EB_US_CHAIN,0,sizeof(QOS_EB_US_CHAIN));
	for(i=0;i<SW_LAN_PORT_NUM;i++){
		snprintf(QOS_IP_CHAIN[i],sizeof(QOS_IP_CHAIN[i]),"qos_ip_rules_p%d",i);
		snprintf(QOS_EB_US_CHAIN[i],sizeof(QOS_EB_US_CHAIN[i]),"qos_eb_us_rules_p%d",i);
	}
	return;
}

// store the enable state of wan port to qos_wan_ports
static void init_qos_phy_wan_ports()
{
	int i=0;
	int wan_port = -1;
	memset(qos_wan_ports,0,sizeof(qos_wan_ports));
	if(getOpMode()==BRIDGE_MODE){
#ifdef CONFIG_USER_AP_BRIDGE_MODE_QOS
		wan_port = rtk_port_get_logPort(rtk_get_bridge_mode_uplink_port());
#endif
		if(wan_port<0)
			wan_port = getDefaultWanPort();
		qos_wan_ports[wan_port] = 1;//default logic port
	}else{
		for(i=0;i<SW_LAN_PORT_NUM;i++){
			if(rtk_port_is_wan_logic_port(i))
				qos_wan_ports[i] = 1;
		}
	}
}

// return 0 if the input ethType is not support in qos 
static int getEthTypeMapMark(unsigned short ethType)
{
	int i;
	for(i=0;i<(sizeof(etherTypeMap)/sizeof(unsigned short));i++){
		if(etherTypeMap[i]==ethType)
			return i+1;
	}
	return 0;
}

int getEthTypeNumb(void)
{
	return sizeof(etherTypeMap)/sizeof(unsigned short);
}

void getEthTypes(unsigned short *ethtypes, int numb)
{
	int i;
	if(!ethtypes)
		return;
	for(i=0;i<numb;i++){
		ethtypes[i] = etherTypeMap[i];
	}
}

int getDefaultWanPort(void)
{
	// in mib_atm_vc_default_table, the default logical port is set as CONFIG_LAN_PORT_NUM, it should be same as that one.
	return CONFIG_LAN_PORT_NUM;
}

// store info about wan itfs per wan port to wan_port_itfs
static void set_qos_wan_itfs(void)
{	
	MIB_CE_ATM_VC_T atm_entry;
	int i, j=0, k, vcEntryNum=0;
	int port;

	memset(wan_port_itfs,0,sizeof(wan_port_itfs));
	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0;i<vcEntryNum;i++){
		memset(&atm_entry,0,sizeof(MIB_CE_ATM_VC_T));
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&atm_entry))
			continue;
		if(atm_entry.logic_port < SW_LAN_PORT_NUM){
			port = atm_entry.logic_port;
			ifGetName(atm_entry.ifIndex,wan_port_itfs[port].itf[j].name,sizeof(wan_port_itfs[port].itf[j].name));
			wan_port_itfs[port].itf[j].cmode      = atm_entry.cmode;
			wan_port_itfs[port].itf[j].enable_qos = atm_entry.enableIpQos;
			printf("%s:%d port[%d].itf[%d].name is %s\n",__FUNCTION__,__LINE__,port,j,wan_port_itfs[port].itf[j].name);
			j++;
		}
	}
	return;
}
#endif

unsigned int getQoSQueueNum(void)
{
	unsigned int i = 0, entryNum = 0, num = 0, ip_qos_queue_num = 0;
	MIB_CE_IP_QOS_QUEUE_T entry;

	mib_get_s(MIB_IP_QOS_QUEUE_NUM, (void *)&ip_qos_queue_num, sizeof(ip_qos_queue_num));

	entryNum = mib_chain_total(MIB_IP_QOS_QUEUE_TBL);
	for (i = 0; i < entryNum; i++)
	{
		if (!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i, (void*)&entry))
			continue;

		if (i >= ip_qos_queue_num)
			break;

		num++;
	}
	return num;
}

/*
	index : index of the QoS rule
	action : 
		0 ==> add rule
		1 ==> update rule
		2 ==> delete rule
	oldorder : keep the order of old entry

	return :
		0 ==> update MIB failed
		1 ==> success
*/
int UpdateQoSOrder(int index, int action, int oldorder)
{
	MIB_CE_IP_QOS_T entry;
	int entryNum=0, i=0;
	int order = 0;
	
	entryNum = mib_chain_total(MIB_IP_QOS_TBL);
	if(action==0){
		/* add rule, the index value is the last one */

		if(!mib_chain_get(MIB_IP_QOS_TBL, (entryNum-1), &entry))
			return 0;
		order = entry.QoSOrder;

		for(i=0; i<(entryNum-1); i++)
		{
			//reset to zero
			memset(&entry, 0, sizeof(MIB_CE_IP_QOS_T));
			if(!mib_chain_get(MIB_IP_QOS_TBL, i, &entry))
				continue;

			if(entry.QoSOrder < order)
				continue;
			else{
				entry.QoSOrder +=1;
				if(!mib_chain_update(MIB_IP_QOS_TBL, &entry, i))
				{
					return 0;
				}
			}
		}
	}
	else if(action==1){
		/* Update rule, the index value is random */
		int vals = 0, vale=0, flag=0;
		
		if(!mib_chain_get(MIB_IP_QOS_TBL, index, &entry))
			return 0;
		order = entry.QoSOrder;

		if(order < oldorder){
			vals = order;
			vale = oldorder;
			flag=0;
		}else if(oldorder < order){
			vale = order;
			vals = oldorder;
			flag=1;
		}else{
			/* Do nothing */
			return 1;
		}

		for(i=0; i<entryNum; i++)
		{
			if(i==index)
				continue;
			
			//reset to zero
			memset(&entry, 0, sizeof(MIB_CE_IP_QOS_T));
			if(!mib_chain_get(MIB_IP_QOS_TBL, i, &entry))
				continue;

			if((entry.QoSOrder < vals) || (entry.QoSOrder > vale))
				continue;
			else{
				if(flag ==0 ){
					entry.QoSOrder +=1;
				}else{
					entry.QoSOrder -=1;
				}
				
				if(!mib_chain_update(MIB_IP_QOS_TBL, &entry, i))
				{
					return 0;
				}
			}
		}

	}
	else if(action==2)	{
		/* delete rule, the index value is random */

		if(!mib_chain_get(MIB_IP_QOS_TBL, index, &entry))
			return 0;
		order = entry.QoSOrder;

		for(i=0; i<entryNum; i++)
		{
			if(i==index)
				continue;
		
			//reset to zero
			memset(&entry, 0, sizeof(MIB_CE_IP_QOS_T));
			if(!mib_chain_get(MIB_IP_QOS_TBL, i, &entry))
				continue;

			if(entry.QoSOrder < order)
				continue;
			else{
				entry.QoSOrder -=1;
				if(!mib_chain_update(MIB_IP_QOS_TBL, &entry, i))
				{
					return 0;
				}
			}
		}

	}

	return 1;
}

// enable/disable IPQoS
void __dev_setupIPQoS(int flag)
{
	if ( flag == 0 )
		va_cmd("/bin/ethctl", 2, 1, "setipqos", "0");
	else if (flag == 1)
		va_cmd("/bin/ethctl", 2, 1, "setipqos", "1");
}

void setupLanItfMap(unsigned int enable)
{
	int index=0, j=0, k=0;
	char ifname[MAXFNAME] = {0};
#ifdef WLAN_SUPPORT
	MIB_CE_MBSSIB_T entry;
#endif
	int lan_idx=0, max_lan_port_num=0, phyPortId=0;
	int ethPhyPortId = rtk_port_get_wan_phyID();

	memset(&lan_itf_map, 0, sizeof(lan_itf_map));
	memset(&lan_itf_map_vid, 0, sizeof(lan_itf_map_vid));

	 for(lan_idx = 0; lan_idx < ELANVIF_NUM; lan_idx++){
		phyPortId = rtk_port_get_lan_phyID(lan_idx);
		if (phyPortId != -1 && phyPortId != ethPhyPortId)
		{
			max_lan_port_num++;
			continue;
		}
	}
	if(enable) {
		for (index = 1; index < (max_lan_port_num+1); index++) {
#ifdef CONFIG_RTL_SMUX_DEV
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
			if(rtk_port_is_wan_logic_port(index-1))
				continue;
#endif
			snprintf(lan_itf_map.map[index].name, MAX_NAME_LEN, "%s.0", ELANRIF[index-1]);
#else
			snprintf(lan_itf_map.map[index].name, MAX_NAME_LEN, "%s", ELANRIF[index-1]);
#endif
			snprintf(lan_itf_map_vid.map[index].name, MAX_NAME_LEN, "%s", ELANRIF[index-1]);
			printf("[%s:%d] lan_itf_map.map[%d].name:%s\n", __FUNCTION__, __LINE__, index, lan_itf_map.map[index].name);
		}
#ifdef WLAN_SUPPORT
		for(j=0;j<NUM_WLAN_INTERFACE;j++){
			for ( k=0; k<WLAN_SSID_NUM; k++) {
				mib_chain_local_mapping_get(MIB_MBSSIB_TBL, j, k, (void *)&entry);
				rtk_wlan_get_ifname(j, k, ifname);
				if(entry.wlanDisabled){
#ifdef CONFIG_CMCC_ENTERPRISE
					index++;
#endif
					continue;
				}
				snprintf(lan_itf_map.map[index].name, MAX_NAME_LEN, ifname);
				snprintf(lan_itf_map_vid.map[index].name, MAX_NAME_LEN, ifname);
				printf("[%s:%d] lan_itf_map.map[%d].name:%s\n", __FUNCTION__, __LINE__, index, lan_itf_map.map[index].name);
				index++;
			}
		}
#endif
	}
	return;
}

int getspeed()
{
	int speed = QOS_SPEED_LIMIT_UNIT;
	
#ifndef CONFIG_QOS_SPEED_LIMIT_UNIT_ONE
	unsigned char rateBase8192Kbps=1;
#if defined(CONFIG_RTL9607C_SERIES)
	mib_get(MIB_METER_RATEBASE_8192KBPS_DISABLE, (void *)&rateBase8192Kbps);
#endif
	if (rateBase8192Kbps == 0) //8000kbps base
	{
		speed = QOS_SPEED_LIMIT_UNIT_1000;
	}else
		speed = QOS_SPEED_LIMIT_UNIT;
#endif
	printf("%s:%d speed %d\n", __FUNCTION__, __LINE__, speed);
	return speed;

}

#ifdef CONFIG_USER_AP_HW_QOS
static int qos_wan_port = -1;

static int rtk_ap_qos_enable(unsigned int enable)
{
	char command[128] = {0};
	qos_wan_port = -1;

#ifdef CONFIG_USER_AP_BRIDGE_MODE_QOS
	qos_wan_port = rtk_get_bridge_mode_uplink_port();
#endif	
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	if(qos_wan_port < 0 && (qos_wan_port = rtk_port_get_wan_phyMask()) == -1) {
#else
	if(qos_wan_port < 0 && (qos_wan_port = rtk_port_get_wan_phyID()) == -1) {
#endif
		printf("[%s:%d] rtk_port_get_wan_phyID fail!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	else {
		printf("[%s:%d] qos_wan_port: %d\n", __FUNCTION__, __LINE__, qos_wan_port);
	}

#if defined(CONFIG_RTL_G3LITE_QOS_SUPPORT)
	snprintf(command, sizeof(command), "echo %d > /proc/driver/cortina/aal/qos_8198f", (enable > 0 ? 1 : 0));
	printf("[%s:%d] command: %s\n", __FUNCTION__, __LINE__, command);
	system(command);

	snprintf(command, sizeof(command), "echo \"83xxFC %d\" > /proc/driver/realtek/phyreg", (enable > 0 ? 0 : 1));
	printf("[%s:%d] command: %s\n", __FUNCTION__, __LINE__, command);
	system(command);
	
#elif defined(CONFIG_RTL_83XX_QOS_SUPPORT)
	snprintf(command, sizeof(command), "echo %d > /proc/driver/realtek/qos_83xx", (enable > 0 ? 1 : 0));
	printf("[%s:%d] command: %s\n", __FUNCTION__, __LINE__, command);
	system(command);
#endif

	return 0;
}

int rtk_ap_qos_get_wan_port(void)
{
	return qos_wan_port;
}

int rtk_ap_qos_bandwidth_control(int port, unsigned int bandwidth)
{
	int ret = -1;
	char command[64] = {0};
	QOS_SETUP_PRINT_FUNCTION;

	if(port < 0)
		return ret;

#if defined(CONFIG_RTK_SOC_RTL8198D)
	if((ret = rtk_rate_portEgrBandwidthCtrlIncludeIfg_set(port, 1)) < 0){
		printf("[%s:%d] rtk_rate_portEgrBandwidthCtrlIncludeIfg_set fails, %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}

	if((ret = rtk_rate_portEgrBandwidthCtrlRate_set(port, bandwidth)) < 0){
		printf("[%s:%d] rtk_rate_portEgrBandwidthCtrlRate_set fails, %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}
#elif defined(CONFIG_RTL_G3LITE_QOS_SUPPORT) || defined(CONFIG_RTL8198X_SERIES)
	if((ret = rtk_rate_portEgrBandwidthCtrlRate_set(port, bandwidth)) < 0){
		printf("[%s:%d] rtk_rate_portEgrBandwidthCtrlRate_set fails, %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}
#else
	snprintf(command, sizeof(command), "echo \"port %d rate %d\" > /proc/driver/realtek/qos_83xx", port, bandwidth);
	system(command);
	printf("[%s:%d] command: %s\n", __FUNCTION__, __LINE__, command);
#endif		
	
	return 0;
}

static int rtk_ap_qos_queue_set(int port, unsigned int enable)
{
	int ret = -1;
	rtk_qos_queue_weights_t qWeights;
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	unsigned char policy;
#else
	unsigned char policy[QOS_MAX_PORT_NUM] = {0};
	int logical_wan_port = 0;
#endif	
	if(port < 0)
		return ret;
		
	memset(&qWeights, 0, sizeof(rtk_qos_queue_weights_t));

	if(enable) {
#ifdef	CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(current_qos_wan_port < 0){
			printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
			return 0;
		}
		logical_wan_port = current_qos_wan_port;
#endif
		if(!mib_get_s(MIB_QOS_POLICY, (void *)&policy, sizeof(policy))){
			printf("[%s:%d] MIB get MIB_QOS_POLICY failed!\n", __FUNCTION__, __LINE__);
			return -2;
		}
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(policy == PLY_PRIO)
#else
		if(policy[logical_wan_port] == PLY_PRIO)
#endif
			{
			;	// weight 0 means SP
		}
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		else if(policy == PLY_WRR)
#else
		else if(policy[logical_wan_port] == PLY_WRR)
#endif
			{
			MIB_CE_IP_QOS_QUEUE_T qEntry;
			int qEntryNum, i;

			if((qEntryNum = mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
				return -1;

			for(i = 0; i < qEntryNum; i++){
				memset(&qEntry, 0, sizeof(MIB_CE_IP_QOS_QUEUE_T));
				if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i, (void*)&qEntry)){
					continue;
				}
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
				if(qEntry.q_logic_wan_port != logical_wan_port)
					continue;
#endif
				if(qEntry.prior > MAX_QOS_QUEUE_NUM) {
					printf("[%s:%d] invalid qEntry.prior %d.\n", __FUNCTION__, __LINE__, qEntry.prior);
					continue;
				}
				if(!qEntry.weight)
					qEntry.weight = 1;
				qWeights.weights[(MAX_QOS_QUEUE_NUM - qEntry.prior)] = qEntry.weight;
			}
		}
		else{
			printf("[%s:%d] policy=%d: Unexpected policy value! (0=PRIO, 1=WRR)\n", __FUNCTION__, __LINE__, policy);
			return -1;
		}
	}

	// uplink rule -> config wan port queue
	if((ret = rtk_qos_schedulingQueue_set(port, &qWeights)) < 0){
		printf("[%s:%d] rtk_qos_schedulingQueue_set fails, %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}

	return 0;
}

/*	mapping:
	port 		share meter index
	Port 0,6  	Meter 0~Meter   7
	Port 1,7  	Meter 8~Meter  15
	Port 2,8  	Meter 16~Meter 23
	Port 3,9  	Meter 24~Meter 31
	Port 4,10 	Meter 32~Meter 39
	Port 5    	Meter 40~Meter 47
*/
static int rtk_ap_qos_get_share_meter_index(int port, int queue, unsigned int flag)
{
	int index = -1;
#ifdef CONFIG_USER_AP_QUEUE_METER
	if(port < PHYPORT_0 || port > PHYPORT_MAX || queue < QUEUE_0 || queue > QUEUE_7)
		return -1;

	index = flag ? (port*8 + queue) : (port*8);
#endif
	return index;
}

static int rtk_ap_qos_queue_shaping_set(int port, int queue, unsigned int enable, unsigned int bandwidth)
{
	int ret = 0, meter_index = -1;
	if(port < 0 || queue < 0)
		return -1;

#ifdef CONFIG_USER_AP_QUEUE_METER
	if ((meter_index = rtk_ap_qos_get_share_meter_index(port, queue, enable)) < 0) {
		printf("[%s:%d] rtk_ap_qos_get_share_meter_index fail, port: %d, queue: %d, enable:%d\n", __FUNCTION__, __LINE__, port, queue, enable);
		return -1;
	}

	if((ret = rtk_rate_egrQueueBwCtrlEnable_set(port, queue, enable)) < 0) {
		printf("[%s:%d] rtk_rate_egrQueueBwCtrlEnable_set fails, %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}

	if((ret = rtk_rate_egrQueueBwCtrlMeterIdx_set(port, queue, meter_index)) < 0){
		printf("[%s:%d] rtk_rate_egrQueueBwCtrlMeterIdx_set fails, %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}

	// 1: ifgInclude
	if((ret = rtk_rate_shareMeter_set(meter_index, bandwidth, 1)) < 0){
		printf("[%s:%d] rtk_rate_shareMeter_set fails, %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}
#endif

	return ret;
}

static int rtk_ap_qos_dot1p_remark_set(int port, unsigned int enable, unsigned int syspri, unsigned int dot1p)
{
	int ret = -1;
	
	if(port < 0)
		return ret;
		
#if defined(CONFIG_RTL_83XX_QOS_SUPPORT) && !defined(CONFIG_RTL_G3LITE_QOS_SUPPORT)
	QOS_SETUP_PRINT_FUNCTION;
	if((ret = rtk_qos_1pRemarkEnable_set(port, enable)) < 0){
		printf("[%s:%d] rtk_qos_1pRemarkEnable_set fails, %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}

	//printf("[%s:%d] 1pRemarkEnable, port: %d\n", __FUNCTION__, __LINE__, port);

	if((ret = rtk_qos_1pRemarkGroup_set(0, syspri, 0, dot1p)) < 0){
		printf("[%s:%d] rtk_qos_1pRemarkGroup_set fails, %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}

	//printf("[%s:%d] 1pRemarkGroup, syspri:%d, dot1p: %d\n", __FUNCTION__, __LINE__, syspri, dot1p);
#endif

	return 0;
}
#endif

#ifdef CONFIG_ETH_AND_WLAN_QOS_SUPPORT
int rtk_set_wlan_mac_qos_tbl(WLAN_QOS_OPERATION_T operation, unsigned char enable)
{
	int i, total, mib_id = MIB_DATA_SPEED_LIMIT_DOWN_MAC_TBL;
	unsigned char str_mac[16] = {0};
	MIB_CE_DATA_SPEED_LIMIT_MAC_T entry;
	wifi_sbwc_req_T sbwc_req;
	unsigned char qos_enable = getQosEnable();
	int speed= getspeed();
	
	if(UPDATE_WLAN_QOS == operation) {
		total = mib_chain_total(mib_id);
		for(i = 0; i < total; i++) { 
			memset(&entry, 0, sizeof(MIB_CE_DATA_SPEED_LIMIT_MAC_T));
			memset(&sbwc_req, 0, sizeof(wifi_sbwc_req_T));
		
			if(mib_chain_get(mib_id, i, &entry) == 0)
				continue;

			snprintf(str_mac, sizeof(str_mac), "%.2x%.2x%.2x%.2x%.2x%.2x", 
						entry.mac[0],entry.mac[1],entry.mac[2],entry.mac[3],entry.mac[4],entry.mac[5]);

			snprintf(sbwc_req.clientMAC, sizeof(sbwc_req.clientMAC), "%s", str_mac);
			if(enable && qos_enable) {
				// downlink -> sta rx rate
				snprintf(sbwc_req.rx_limit, sizeof(sbwc_req.rx_limit), "%d", entry.speed_unit*speed);  // Kbps
			}
			else {
				// rx_limit/tx_limit 0 -> unlimited rule
			}

			// add sbwc_tbl based on speed_limit_down_mac_tbl
			rtk_wlan_set_sbwc_req(NULL, &sbwc_req);
		}
	}
	else if(FLUSH_WLAN_QOS == operation) {
		if(!enable) {
			rtk_wlan_delall_sbwc_tbl();
		}
	}
	else {
		printf("[%s:%d] unknown operation %d\n", __FUNCTION__, __LINE__, operation);
		return RTK_FAILED;
	}

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
	
	return RTK_SUCCESS;
}

void setup_data_speed_limit_wlan_mac_qos(unsigned char enable)
{
	rtk_wlan_set_sbwc_mode(NULL, 1);
	rtk_set_wlan_mac_qos_tbl(UPDATE_WLAN_QOS, enable);
	rtk_setup_wlan_mac_qos(enable);
	rtk_set_wlan_mac_qos_tbl(FLUSH_WLAN_QOS, enable);
}
#endif

#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
int setup_speed_limit_chain(unsigned int enable)
{
	QOS_SETUP_PRINT_FUNCTION
		
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	char up_root_chain[] = "FORWARD";
#else
	char up_root_chain[] = "PREROUTING";
#endif

	if (enable) {
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		//only set speed limit chain for once
		if(speed_limit_set_once)
			return 0;
		else
			speed_limit_set_once =1;
#endif
		// uplink works in PREROUTING
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", UP_LAN_INTF_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", up_root_chain, "-j", UP_LAN_INTF_RATE_LIMIT);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", UP_IP_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", up_root_chain, "-j", UP_IP_RATE_LIMIT);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", UP_MAC_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", up_root_chain, "-j", UP_MAC_RATE_LIMIT);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", UP_VLAN_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", up_root_chain, "-j", UP_VLAN_RATE_LIMIT);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", DOWN_IP_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", DOWN_IP_RATE_LIMIT);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", DOWN_MAC_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", DOWN_MAC_RATE_LIMIT);

	#ifndef CONFIG_USER_AP_QOS
		// downlink works in Ebtables
		// iptables "using --physdev-out in the OUTPUT, FORWARD and POSTROUTING chains
		// for non-bridged traffic is not supported anymore"
		va_cmd(EBTABLES, 2, 1, "-N", (char *)DOWN_VLAN_RATE_LIMIT);
		va_cmd(EBTABLES, 3, 1, "-P", DOWN_VLAN_RATE_LIMIT, "RETURN");
		va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-j", (char *)DOWN_VLAN_RATE_LIMIT);

		va_cmd(EBTABLES, 2, 1, "-N", (char *)DOWN_LAN_INTF_RATE_LIMIT);
		va_cmd(EBTABLES, 3, 1, "-P", DOWN_LAN_INTF_RATE_LIMIT, "RETURN");
		va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-j", (char *)DOWN_LAN_INTF_RATE_LIMIT);
	#else
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", DOWN_LAN_INTF_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", DOWN_LAN_INTF_RATE_LIMIT);
		
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", DOWN_VLAN_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", DOWN_VLAN_RATE_LIMIT);

	#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", UP_LAN_INTF_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", up_root_chain, "-j", UP_LAN_INTF_RATE_LIMIT);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", UP_IP_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", up_root_chain, "-j", UP_IP_RATE_LIMIT);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", UP_MAC_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", up_root_chain, "-j", UP_MAC_RATE_LIMIT);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", UP_VLAN_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", up_root_chain, "-j", UP_VLAN_RATE_LIMIT);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", DOWN_IP_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", DOWN_IP_RATE_LIMIT);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", DOWN_MAC_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", DOWN_MAC_RATE_LIMIT);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", DOWN_LAN_INTF_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", DOWN_LAN_INTF_RATE_LIMIT);
		
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", DOWN_VLAN_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", DOWN_VLAN_RATE_LIMIT);
	#endif
	#endif
	} else {
		// uplink works in PREROUTING
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", UP_LAN_INTF_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", up_root_chain, "-j", UP_LAN_INTF_RATE_LIMIT);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", UP_LAN_INTF_RATE_LIMIT);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", UP_IP_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", up_root_chain, "-j", UP_IP_RATE_LIMIT);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", UP_IP_RATE_LIMIT);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", UP_MAC_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", up_root_chain, "-j", UP_MAC_RATE_LIMIT);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", UP_MAC_RATE_LIMIT);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", UP_VLAN_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", up_root_chain, "-j", UP_VLAN_RATE_LIMIT);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", UP_VLAN_RATE_LIMIT);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", DOWN_IP_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", DOWN_IP_RATE_LIMIT);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", DOWN_IP_RATE_LIMIT);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", DOWN_MAC_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", DOWN_MAC_RATE_LIMIT);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", DOWN_MAC_RATE_LIMIT);

	#ifndef CONFIG_USER_AP_QOS
		// downlink works in Ebtables
		// iptables "using --physdev-out in the OUTPUT, FORWARD and POSTROUTING chains
		// for non-bridged traffic is not supported anymore"
        va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_OUTPUT, "-j", (char *)DOWN_LAN_INTF_RATE_LIMIT);
        va_cmd(EBTABLES, 2, 1, "-X", (char *)DOWN_LAN_INTF_RATE_LIMIT);

        va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_OUTPUT, "-j", (char *)DOWN_VLAN_RATE_LIMIT);
        va_cmd(EBTABLES, 2, 1, "-X", (char *)DOWN_VLAN_RATE_LIMIT);
	#else
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", DOWN_LAN_INTF_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", DOWN_LAN_INTF_RATE_LIMIT);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", DOWN_LAN_INTF_RATE_LIMIT);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", DOWN_VLAN_RATE_LIMIT);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", DOWN_VLAN_RATE_LIMIT);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", DOWN_VLAN_RATE_LIMIT);
	#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", UP_LAN_INTF_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", up_root_chain, "-j", UP_LAN_INTF_RATE_LIMIT);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", UP_LAN_INTF_RATE_LIMIT);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", UP_IP_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", up_root_chain, "-j", UP_IP_RATE_LIMIT);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", UP_IP_RATE_LIMIT);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", UP_MAC_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", up_root_chain, "-j", UP_MAC_RATE_LIMIT);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", UP_MAC_RATE_LIMIT);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", UP_VLAN_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", up_root_chain, "-j", UP_VLAN_RATE_LIMIT);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", UP_VLAN_RATE_LIMIT);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", DOWN_IP_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", DOWN_IP_RATE_LIMIT);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", DOWN_IP_RATE_LIMIT);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", DOWN_MAC_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", DOWN_MAC_RATE_LIMIT);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", DOWN_MAC_RATE_LIMIT);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", DOWN_LAN_INTF_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", DOWN_LAN_INTF_RATE_LIMIT);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", DOWN_LAN_INTF_RATE_LIMIT);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", DOWN_VLAN_RATE_LIMIT);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", DOWN_VLAN_RATE_LIMIT);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", DOWN_VLAN_RATE_LIMIT);
	#endif
	#endif
	}

	return 0;
}

int setup_speed_limit_qdisc(unsigned int enable)
{
	int port = 0;
#ifdef QOS_SETUP_IFB
	char *devname="ifb0";
#endif
	int i;
	QOS_SETUP_PRINT_FUNCTION
	if (enable) {
#if defined(QOS_SETUP_IFB)
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(!(devname=(char*)getQosRootWanName(current_qos_wan_port)))
			return 0;
#endif
		//root qdisc: tc qdisc add dev ifb0 root handle 2: htb r2q 1
		va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", devname, "root", "handle", "2:", "htb", "r2q", "1");

		//root class: tc class add dev ifb0 parent 2: classid 2:1 htb rate 1024000Kbit ceil 1024000Kbit
		va_cmd(TC, 13, 1, "class", (char *)ARG_ADD, "dev", devname, "parent", "2:", "classid", "2:1", "htb", "rate", "1024000Kbit", "ceil", "1024000Kbit");
#endif
		//root qdisc: tc qdisc add dev br0 root handle 1: htb r2q 1
		va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF, "root", "handle", "1:", "htb", "r2q", "1");

		//root class: tc class add dev br0 parent 1: classid 1:1 htb rate 1024000Kbit ceil 1024000Kbit
		va_cmd(TC, 13, 1, "class", (char *)ARG_ADD, "dev", LANIF, "parent", "1:", "classid", "1:1", "htb", "rate", "1024000Kbit", "ceil", "1024000Kbit");
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN 
		// don't need to redirect from br0 to ifb0 for multi_phy-wan 
		//tc qdisc add dev br0 ingress
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF, "ingress");
#ifdef QOS_SETUP_IFB
		//tc filter add dev br0 parent ffff: protocol ip u32 match u32 0 0 flowid 1:1 action mirred egress redirect dev ifb0
		va_cmd(TC, 21, 1, "filter", (char *)ARG_ADD, "dev", LANIF, "parent", "ffff:", "protocol", "ip", "u32", "match", "u32", "0", "0", "flowid", "1:1", "action", "mirred", "egress", "redirect", "dev", devname);		
#endif
#endif
	}
	else {
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
#if defined(QOS_SETUP_IFB)
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", devname, "root");
#endif
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", LANIF, "root");
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", LANIF, "ingress");
#else
		for(i=0;i<SW_LAN_PORT_NUM;i++){
			if(!(devname=(char*)getQosRootWanName(i)))
				return 0;
			va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", devname, "root");
		}
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", LANIF, "root");
#endif
	}

	return 0;
}

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
int setup_sharemeter_rate(int index, int speedUnit, char* setmark_str, int mask)
{
	int mark=0, mark_mask=0, meter_idx=-1;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	index += current_qos_wan_port*100;
#endif
	mark_mask = SOCK_MARK_METER_INDEX_BIT_MASK|SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK;
	meter_idx = rtk_qos_share_meter_get(RTK_APP_TYPE_IP_QOS_RATE_SHAPING, index, speedUnit);
	if (meter_idx < 0)
		meter_idx = rtk_qos_share_meter_set(RTK_APP_TYPE_IP_QOS_RATE_SHAPING, index, speedUnit);
	if(meter_idx != -1)
		mark |= SOCK_MARK_METER_INDEX_ENABLE_TO_MARK(1);
	else
	{
		printf("%s %d: rtk_qos_share_meter_set FAIL !\n", __func__, __LINE__);
		meter_idx = (index-1);
		return -1;
	}
	mark |= SOCK_MARK_METER_INDEX_TO_MARK(meter_idx);
	AUG_PRT("meter_idx = %d\n", meter_idx);
	AUG_PRT("mark = 0x%x\n", mark);
	if (mask)
		sprintf(setmark_str, "0x%x/0x%x", mark, mark_mask);
	else
		sprintf(setmark_str, "0x%x", mark);

	return 0;
}
#endif

int setup_software_upstream_if_rate_limit(int i, MIB_CE_DATA_SPEED_LIMIT_IF_Tp entry)
{
	char classid[10] = {0}, handle[32] = {0}, rate[16] = {0};
	char qdisc_handle[16] = {0}, ifname_str[MAXFNAME] = {0};
	int speed= getspeed();
#ifdef QOS_SETUP_IFB
	char *devname="ifb0";
#endif
	int j;

	QOS_SETUP_PRINT_FUNCTION
	if(entry->speed_unit <= 0 || entry->if_id > LAN_INTF_NUM){
		return -1;
	}
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(!(devname=(char*)getQosRootWanName(current_qos_wan_port)))
		return 0;
#endif
	snprintf(classid, sizeof(classid), "2:%d", i+1);
	snprintf(qdisc_handle, sizeof(qdisc_handle), "%d:", 2*100+i+1);
	snprintf(rate, sizeof(rate), "%dkbit", (entry->speed_unit*speed));
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
	setup_sharemeter_rate(((MIB_DATA_SPEED_LIMIT_UP_IF_TBL*10)+i), entry->speed_unit*speed, handle, 1);
#else
	snprintf(handle, sizeof(handle), "%d/0x%x", ((i << SOCK_MARK_QOS_SWQID_START) & SOCK_MARK_QOS_SWQID_BIT_MASK), SOCK_MARK_QOS_MASK_ALL);
#endif
#ifdef QOS_SETUP_IFB
	//tc class add dev ifb0 parent 2:1 classid 2:2 htb rate 1kbit ceil 512kbit
	va_cmd(TC, 13, 1, "class", (char *)ARG_ADD, "dev", devname, "parent", "2:1", "classid", classid, "htb", "rate", "1kbit", "ceil", rate);

	// tc qdisc add dev ifb0 parent 2:2 handle 202: pfifo limit 100
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname, "parent", classid, "handle", qdisc_handle, "pfifo", "limit", "100");

	//tc filter add dev ifb0 parent 2: protocol ip prio 1 handle 2001 fw classid 2:1
	va_cmd(TC, 15, 1, "filter", (char *)ARG_ADD, "dev", devname, "parent", "2:", "protocol", "all", "prio", "1", "handle", handle, "fw", "classid", classid);
#endif
#ifndef CONFIG_USER_AP_QOS
	if(strlen(lan_itf_map.map[entry->if_id].name)) {
		snprintf(ifname_str, sizeof(ifname_str), "%s", lan_itf_map.map[entry->if_id].name);
#else
	if(strlen(lan_itf_map_vid.map[entry->if_id].name)) {
		snprintf(ifname_str, sizeof(ifname_str), "%s+", lan_itf_map_vid.map[entry->if_id].name);
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(getOpMode()!=BRIDGE_MODE){
			for(j=0; j<MAX_VC_NUM; j++){
				if(!wan_port_itfs[current_qos_wan_port].itf[j].name[0])
					continue;
				if(wan_port_itfs[current_qos_wan_port].itf[j].cmode==CHANNEL_MODE_BRIDGE){
					va_cmd(IPTABLES, 16, 1, "-t", "mangle", "-A", UP_LAN_INTF_RATE_LIMIT,"-m","outphysdev","--physdev",wan_port_itfs[current_qos_wan_port].itf[j].name,"-m", "physdev", "--physdev-in", ifname_str, "-j", "MARK", "--set-mark", handle);
#if defined(CONFIG_IPV6)
					va_cmd(IP6TABLES, 16, 1, "-t", "mangle", "-A", UP_LAN_INTF_RATE_LIMIT, "-m","outphysdev","--physdev",wan_port_itfs[current_qos_wan_port].itf[j].name,"-m", "physdev", "--physdev-in", ifname_str, "-j", "MARK", "--set-mark", handle);
#endif
				}
				else{
					//iptables -t mangle -A up_lan_intf_rate_limit -o nas0_0  -m physdev --physdev-in eth0.2+  -j MARK --set-mark 0x/0x
					va_cmd(IPTABLES, 14, 1, "-t", "mangle", "-A", UP_LAN_INTF_RATE_LIMIT,"-o",wan_port_itfs[current_qos_wan_port].itf[j].name,"-m", "physdev", "--physdev-in", ifname_str, "-j", "MARK", "--set-mark", handle);
#if defined(CONFIG_IPV6)
					va_cmd(IP6TABLES, 14, 1, "-t", "mangle", "-A", UP_LAN_INTF_RATE_LIMIT,"-o",wan_port_itfs[current_qos_wan_port].itf[j].name,"-m", "physdev", "--physdev-in", ifname_str, "-j", "MARK", "--set-mark", handle);
#endif
				}
			}
		}
		else
#endif
#endif
		{
			//iptables -t mangle -A PREROUTING -m physdev --physdev-in eth0.2  -j MARK --set-mark 2001
			va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", UP_LAN_INTF_RATE_LIMIT, "-m", "physdev", "--physdev-in", ifname_str, "-j", "MARK", "--set-mark", handle);
#if defined(CONFIG_IPV6)
			va_cmd(IP6TABLES, 12, 1, "-t", "mangle", "-A", UP_LAN_INTF_RATE_LIMIT, "-m", "physdev", "--physdev-in", ifname_str, "-j", "MARK", "--set-mark", handle);
#endif
		}
	}
	else{
		printf("[%s:%d] lan_itf_map error! if_id:%d.\n", __FUNCTION__, __LINE__, entry->if_id);
	}

	return 0;
}

int setup_software_downstream_if_rate_limit(int i, MIB_CE_DATA_SPEED_LIMIT_IF_Tp entry)
{
	char classid[10] = {0}, handle[32] = {0}, rate[16] = {0};
	char qdisc_handle[16] = {0}, ifname_str[MAXFNAME] = {0};
	int port = 0;
	int speed= getspeed();
	int j;

	QOS_SETUP_PRINT_FUNCTION
	if(entry->speed_unit <= 0 || entry->if_id > LAN_INTF_NUM){
		return -1;
	}
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	snprintf(classid, sizeof(classid), "1:%d", i+1);
	snprintf(qdisc_handle, sizeof(qdisc_handle), "%d:", 1*100+i+1);
#else
	snprintf(classid, sizeof(classid), "1:%d", current_qos_wan_port*10+i+1);
	snprintf(qdisc_handle, sizeof(qdisc_handle), "%d:", 1*100+current_qos_wan_port*10+i+1);
#endif
	snprintf(rate, sizeof(rate), "%dkbit", (entry->speed_unit*speed));
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
#ifndef CONFIG_USER_AP_QOS
	setup_sharemeter_rate(((MIB_DATA_SPEED_LIMIT_DOWN_IF_TBL*10)+i), entry->speed_unit*speed, handle, 0);
#else
	setup_sharemeter_rate(((MIB_DATA_SPEED_LIMIT_DOWN_IF_TBL*10)+i), entry->speed_unit*speed, handle, 1);
#endif
#else
	snprintf(handle, sizeof(handle), "%d/0x%x", ((i << SOCK_MARK_QOS_SWQID_START) & SOCK_MARK_QOS_SWQID_BIT_MASK), SOCK_MARK_QOS_MASK_ALL);
#endif
	//tc class add dev br0 parent 1:1 classid 1:2 htb rate 1kbit ceil 512kbit
	va_cmd(TC, 13, 1, "class", (char *)ARG_ADD, "dev", LANIF, "parent", "1:1", "classid", classid, "htb", "rate", "1kbit", "ceil", rate);

	// tc qdisc add dev br0 parent 1:2 handle 102: pfifo limit 100
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF, "parent", classid, "handle", qdisc_handle, "pfifo", "limit", "100");

	//tc filter add dev br0 parent 1: protocol ip prio 1 handle 1001 fw classid 1:11
	va_cmd(TC, 15, 1, "filter", (char *)ARG_ADD, "dev", LANIF, "parent", "1:", "protocol", "all", "prio", "1", "handle", handle, "fw", "classid", classid);

#ifndef CONFIG_USER_AP_QOS
	if(strlen(lan_itf_map.map[entry->if_id].name)) {
		snprintf(ifname_str, sizeof(ifname_str), "%s", lan_itf_map.map[entry->if_id].name);
		//ebtables -A down_lan_intf_rate_limit -o eth0.2.0 -j mark --mark-or 0x4000000
		va_cmd(EBTABLES, 8, 1,  "-A", DOWN_LAN_INTF_RATE_LIMIT, "-o", ifname_str, "-j", "mark", "--mark-or", handle);
#else
	if(strlen(lan_itf_map_vid.map[entry->if_id].name)) {
		snprintf(ifname_str, sizeof(ifname_str), "%s+", lan_itf_map_vid.map[entry->if_id].name);
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(getOpMode()!=BRIDGE_MODE){
			for(j=0; j<MAX_VC_NUM; j++){
				if(!wan_port_itfs[current_qos_wan_port].itf[j].name[0])
					continue;
				if(wan_port_itfs[current_qos_wan_port].itf[j].cmode==CHANNEL_MODE_BRIDGE){
					va_cmd(IPTABLES, 16, 1, "-t", "mangle", "-A", DOWN_LAN_INTF_RATE_LIMIT,"-m", "physdev", "--physdev-in",wan_port_itfs[current_qos_wan_port].itf[j].name,"-m", "outphysdev", "--physdev", ifname_str, "-j", "MARK", "--set-mark", handle);
#ifdef CONFIG_IPV6
					va_cmd(IP6TABLES, 16, 1, "-t", "mangle", "-A", DOWN_LAN_INTF_RATE_LIMIT,"-m", "physdev", "--physdev-in",wan_port_itfs[current_qos_wan_port].itf[j].name, "-m", "outphysdev", "--physdev", ifname_str, "-j", "MARK", "--set-mark", handle);
#endif
				}	
				else{
					//iptables -t mangle -A down_lan_intf_rate_limit -i nas0_0	-m outphysdev --physdev eth0.2+  -j MARK --set-mark 0x/0x
					va_cmd(IPTABLES, 14, 1, "-t", "mangle", "-A", DOWN_LAN_INTF_RATE_LIMIT,"-i",wan_port_itfs[current_qos_wan_port].itf[j].name,"-m", "outphysdev", "--physdev", ifname_str, "-j", "MARK", "--set-mark", handle);
#ifdef CONFIG_IPV6
					va_cmd(IP6TABLES, 14, 1, "-t", "mangle", "-A", DOWN_LAN_INTF_RATE_LIMIT,"-i",wan_port_itfs[current_qos_wan_port].itf[j].name, "-m", "outphysdev", "--physdev", ifname_str, "-j", "MARK", "--set-mark", handle);
#endif
				}
			}
		}
		else
#endif
		{
			va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", DOWN_LAN_INTF_RATE_LIMIT, "-m", "outphysdev", "--physdev", ifname_str, "-j", "MARK", "--set-mark", handle);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 12, 1, "-t", "mangle", "-A", DOWN_LAN_INTF_RATE_LIMIT, "-m", "outphysdev", "--physdev", ifname_str, "-j", "MARK", "--set-mark", handle);
#endif
		}
#endif
	}
	else {
		printf("[%s:%d] lan_itf_map error! if_id:%d.\n", __FUNCTION__, __LINE__, entry->if_id);
	}

	return 0;
}

int setup_software_upstream_ip_rate_limit(int i, MIB_CE_DATA_SPEED_LIMIT_IP_Tp entry)
{
	char classid[10] = {0}, handle[32] = {0}, rate[16] = {0}, iprange[128] = {0};
	char qdisc_handle[16] = {0};
	int speed= getspeed();
#ifdef QOS_SETUP_IFB
	char *devname="ifb0";
#endif
	int j;

	if(entry->speed_unit <= 0){
		return -1;
	}
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(!(devname=(char*)getQosRootWanName(current_qos_wan_port)))
		return 0;
#endif
	snprintf(classid, sizeof(classid), "2:%d", i+1);
	snprintf(qdisc_handle, sizeof(qdisc_handle), "%d:", 2*100+i+1);
	snprintf(rate, sizeof(rate), "%dkbit", (entry->speed_unit*speed));
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
	setup_sharemeter_rate(((MIB_DATA_SPEED_LIMIT_UP_IP_TBL*10)+i), entry->speed_unit*speed, handle, 1);
#else
	snprintf(handle, sizeof(handle), "%d/0x%x", ((i << SOCK_MARK_QOS_SWQID_START) & SOCK_MARK_QOS_SWQID_BIT_MASK), SOCK_MARK_QOS_MASK_ALL);
#endif
	snprintf(iprange, sizeof(iprange), "%s-%s", entry->ip_start, entry->ip_end);
#ifdef QOS_SETUP_IFB
	//tc class add dev ifb0 parent 2:1 classid 2:2 htb rate 1kbit ceil 512kbit
	va_cmd(TC, 13, 1, "class", (char *)ARG_ADD, "dev", devname, "parent", "2:1", "classid", classid, "htb", "rate", "1kbit", "ceil", rate);

	// tc qdisc add dev ifb0 parent 2:2 handle 202: pfifo limit 100
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname, "parent", classid, "handle", qdisc_handle, "pfifo", "limit", "100");

	//tc filter add dev ifb0 parent 2: protocol ip prio 1 handle 2001 fw classid 2:1
	va_cmd(TC, 15, 1, "filter", (char *)ARG_ADD, "dev", devname, "parent", "2:", "protocol", "all", "prio", "1", "handle", handle, "fw", "classid", classid);
#endif
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(getOpMode()!=BRIDGE_MODE){
		for(j=0; j<MAX_VC_NUM; j++){
			if(!wan_port_itfs[current_qos_wan_port].itf[j].name[0])
				continue;
			if(wan_port_itfs[current_qos_wan_port].itf[j].cmode==CHANNEL_MODE_BRIDGE){
				//iptables -t mangle -A up_ip_rate_limit -m outphysdev --physdev nas0_0 -m iprange --src-range 192.168.1.2-192.168.1.7  -j MARK --set-mark 0x/0x
				if(entry->ip_ver == IPVER_IPV4)
					va_cmd(IPTABLES, 16, 1, "-t", "mangle", "-A", UP_IP_RATE_LIMIT,"-m","outphysdev","--physdev",wan_port_itfs[current_qos_wan_port].itf[j].name,"-m", "iprange", "--src-range", iprange, "-j", "MARK", "--set-mark", handle);
#if defined(CONFIG_IPV6)
				else
					va_cmd(IP6TABLES, 16, 1, "-t", "mangle", "-A", UP_IP_RATE_LIMIT, "-m","outphysdev","--physdev",wan_port_itfs[current_qos_wan_port].itf[j].name,"-m", "iprange", "--src-range", iprange, "-j", "MARK", "--set-mark", handle);
#endif
			}
			else{
				//iptables -t mangle -A up_ip_rate_limit -o nas0_0 -m iprange --src-range 192.168.1.2-192.168.1.7  -j MARK --set-mark 0x/0x
				if(entry->ip_ver == IPVER_IPV4)
					va_cmd(IPTABLES, 14, 1, "-t", "mangle", "-A", UP_IP_RATE_LIMIT, "-o", wan_port_itfs[current_qos_wan_port].itf[j].name, "-m", "iprange", "--src-range", iprange, "-j", "MARK", "--set-mark", handle);
#if defined(CONFIG_IPV6)
				else 
					va_cmd(IP6TABLES, 14, 1, "-t", "mangle", "-A", UP_IP_RATE_LIMIT, "-o", wan_port_itfs[current_qos_wan_port].itf[j].name,  "-m", "iprange", "--src-range", iprange, "-j", "MARK", "--set-mark", handle);
#endif
			}
		}
	}
	else
#endif
	{
		//iptables -t mangle -A PREROUTING -m iprange --src-range 192.168.1.2-192.168.1.7  -j MARK --set-mark 2001
		if(entry->ip_ver == IPVER_IPV4)
			va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", UP_IP_RATE_LIMIT, "-m", "iprange", "--src-range", iprange, "-j", "MARK", "--set-mark", handle);
#if defined(CONFIG_IPV6)
		else 
			va_cmd(IP6TABLES, 12, 1, "-t", "mangle", "-A", UP_IP_RATE_LIMIT, "-m", "iprange", "--src-range", iprange, "-j", "MARK", "--set-mark", handle);
#endif
	}
	return 0;
}

int setup_software_downstream_ip_rate_limit(int i, MIB_CE_DATA_SPEED_LIMIT_IP_Tp entry)
{
	char classid[10] = {0}, handle[32] = {0}, rate[16] = {0}, iprange[128] = {0};
	char qdisc_handle[16] = {0};
	int port = 0;
	int j;
	int speed= getspeed();

	if(entry->speed_unit <= 0){
		return -1;
	}
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	snprintf(classid, sizeof(classid), "1:%d", i+1);
	snprintf(qdisc_handle, sizeof(qdisc_handle), "%d:", 1*100+i+1);
#else
	snprintf(classid, sizeof(classid), "1:%d", current_qos_wan_port*10+i+1);
	snprintf(qdisc_handle, sizeof(qdisc_handle), "%d:", 1*100+current_qos_wan_port*10+i+1);
#endif
	snprintf(rate, sizeof(rate), "%dkbit", (entry->speed_unit*speed));
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
	setup_sharemeter_rate(((MIB_DATA_SPEED_LIMIT_DOWN_IP_TBL*10)+i), entry->speed_unit*speed, handle, 1);
#else
	snprintf(handle, sizeof(handle), "%d/0x%x", ((i << SOCK_MARK_QOS_SWQID_START) & SOCK_MARK_QOS_SWQID_BIT_MASK), SOCK_MARK_QOS_MASK_ALL);
#endif
	snprintf(iprange, sizeof(iprange), "%s-%s", entry->ip_start, entry->ip_end);

	//tc class add dev br0 parent 1:1 classid 1:2 htb rate 1kbit ceil 512kbit
	va_cmd(TC, 13, 1, "class", (char *)ARG_ADD, "dev", LANIF, "parent", "1:1", "classid", classid, "htb", "rate", "1kbit", "ceil", rate);

	// tc qdisc add dev br0 parent 1:2 handle 102: pfifo limit 100
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF, "parent", classid, "handle", qdisc_handle, "pfifo", "limit", "100");

	//tc filter add dev br0 parent 1: protocol ip prio 1 handle 1001 fw classid 1:11
	va_cmd(TC, 15, 1, "filter", (char *)ARG_ADD, "dev", LANIF, "parent", "1:", "protocol", "all", "prio", "1", "handle", handle, "fw", "classid", classid);
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(getOpMode()!=BRIDGE_MODE){
		for(j=0; j<MAX_VC_NUM; j++){
			if(!wan_port_itfs[current_qos_wan_port].itf[j].name[0])
				continue;
			
			if(wan_port_itfs[current_qos_wan_port].itf[j].cmode==CHANNEL_MODE_BRIDGE){
				//iptables -t mangle -A down_ip_rate_limit -m physdev --physdev-in nas0_0 -m iprange --src-range 192.168.1.2-192.168.1.7  -j MARK --set-mark 0x/0x
				if(entry->ip_ver == IPVER_IPV4)
					va_cmd(IPTABLES, 16, 1, "-t", "mangle", "-A", DOWN_IP_RATE_LIMIT,"-m", "physdev", "--physdev-in",wan_port_itfs[current_qos_wan_port].itf[j].name,"-m", "iprange", "--dst-range", iprange, "-j", "MARK", "--set-mark", handle);
#ifdef CONFIG_IPV6
				else
					va_cmd(IP6TABLES, 16, 1, "-t", "mangle", "-A", DOWN_IP_RATE_LIMIT,"-m", "physdev", "--physdev-in",wan_port_itfs[current_qos_wan_port].itf[j].name, "-m", "iprange", "--dst-range", iprange, "-j", "MARK", "--set-mark", handle);
#endif
			}	
			else{
				//iptables -t mangle -A down_ip_rate_limit -i nas0_0 -m iprange --dst-range 192.168.1.2-192.168.1.7  -j MARK --set-mark 0x/0x
				if(entry->ip_ver == IPVER_IPV4)
					va_cmd(IPTABLES, 14, 1, "-t", "mangle", "-A", DOWN_IP_RATE_LIMIT, "-i", wan_port_itfs[current_qos_wan_port].itf[j].name, "-m", "iprange", "--dst-range", iprange, "-j", "MARK", "--set-mark", handle);
#if defined(CONFIG_IPV6)
				else
					va_cmd(IP6TABLES, 14, 1, "-t", "mangle", "-A", DOWN_IP_RATE_LIMIT, "-i", wan_port_itfs[current_qos_wan_port].itf[j].name, "-m", "iprange", "--dst-range", iprange, "-j", "MARK", "--set-mark", handle);
#endif
			}
		}
	}
	else
#endif
	{
		//iptables -t mangle -A POSTROUTING -m iprange --dst-range 192.168.1.2-192.168.1.7  -j MARK --set-mark 1001
		if(entry->ip_ver == IPVER_IPV4)
			va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", DOWN_IP_RATE_LIMIT, "-m", "iprange", "--dst-range", iprange, "-j", "MARK", "--set-mark", handle);
#if defined(CONFIG_IPV6)
		else
			va_cmd(IP6TABLES, 12, 1, "-t", "mangle", "-A", DOWN_IP_RATE_LIMIT, "-m", "iprange", "--dst-range", iprange, "-j", "MARK", "--set-mark", handle);
#endif
	}
	return 0;
}

int setup_software_upstream_mac_rate_limit(int i, MIB_CE_DATA_SPEED_LIMIT_MAC_Tp entry)
{
	char classid[10] = {0}, handle[32] = {0}, rate[16] = {0}, str_mac[32] = {0};
	char qdisc_handle[16] = {0};
	int speed= getspeed();
#ifdef QOS_SETUP_IFB
	char *devname="ifb0";
#endif
	int j;
	QOS_SETUP_PRINT_FUNCTION
	if(entry->speed_unit <= 0){
		return -1;
	}
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(!(devname=(char*)getQosRootWanName(current_qos_wan_port)))
		return 0;
#endif
	snprintf(classid, sizeof(classid), "2:%d", i+1);
	snprintf(qdisc_handle, sizeof(qdisc_handle), "%d:", 2*100+i+1);
	snprintf(rate, sizeof(rate), "%dkbit", (entry->speed_unit*speed));
#if (defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API))
	setup_sharemeter_rate(((MIB_DATA_SPEED_LIMIT_UP_MAC_TBL*10)+i), entry->speed_unit*speed, handle, 1);
#else
	snprintf(handle, sizeof(handle), "%d/0x%x", ((i << SOCK_MARK_QOS_SWQID_START) & SOCK_MARK_QOS_SWQID_BIT_MASK), SOCK_MARK_QOS_MASK_ALL);
#endif
	snprintf(str_mac, sizeof(str_mac), "%02x:%02x:%02x:%02x:%02x:%02x",
		entry->mac[0], entry->mac[1], entry->mac[2],
		entry->mac[3], entry->mac[4], entry->mac[5]);
#ifdef QOS_SETUP_IFB
	//tc class add dev ifb0 parent 2:1 classid 2:2 htb rate 1kbit ceil 512kbit
	va_cmd(TC, 13, 1, "class", (char *)ARG_ADD, "dev", devname, "parent", "2:1", "classid", classid, "htb", "rate", "1kbit", "ceil", rate);

	// tc qdisc add dev ifb0 parent 2:2 handle 202: pfifo limit 100
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname, "parent", classid, "handle", qdisc_handle, "pfifo", "limit", "100");

	//tc filter add dev ifb0 parent 2: protocol ip prio 1 handle 2001 fw classid 2:1
	va_cmd(TC, 15, 1, "filter", (char *)ARG_ADD, "dev", devname, "parent", "2:", "protocol", "all", "prio", "1", "handle", handle, "fw", "classid", classid);
#endif
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(getOpMode()!=BRIDGE_MODE){
		for(j=0; j<MAX_VC_NUM; j++){
			if(!wan_port_itfs[current_qos_wan_port].itf[j].name[0])
				continue;
			if(wan_port_itfs[current_qos_wan_port].itf[j].cmode==CHANNEL_MODE_BRIDGE){
				va_cmd(IPTABLES, 16, 1, "-t", "mangle", "-A", UP_MAC_RATE_LIMIT,"-m","outphysdev","--physdev",wan_port_itfs[current_qos_wan_port].itf[j].name,"-m", "mac", "--mac-source", str_mac, "-j", "MARK", "--set-mark", handle);
#if defined(CONFIG_IPV6)
				va_cmd(IP6TABLES, 16, 1, "-t", "mangle", "-A", UP_MAC_RATE_LIMIT, "-m","outphysdev","--physdev",wan_port_itfs[current_qos_wan_port].itf[j].name,"-m", "mac", "--mac-source", str_mac, "-j", "MARK", "--set-mark", handle);
#endif
			}
			else{
				va_cmd(IPTABLES, 14, 1, "-t", "mangle", "-A", UP_MAC_RATE_LIMIT, "-o", wan_port_itfs[current_qos_wan_port].itf[j].name,"-m", "mac", "--mac-source", str_mac, "-j", "MARK", "--set-mark", handle);
#if defined(CONFIG_IPV6)
				va_cmd(IP6TABLES, 14, 1, "-t", "mangle", "-A", UP_MAC_RATE_LIMIT, "-o", wan_port_itfs[current_qos_wan_port].itf[j].name,"-m", "mac", "--mac-source", str_mac, "-j", "MARK", "--set-mark", handle);
#endif
			}
		}
	}
	else
#endif
	{
		// iptables -t mangle -A PREROUTING -m mac --mac-source 00:00:03:00:01:00 -j MARK --set-mark 0x1/0xf000007
		va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", UP_MAC_RATE_LIMIT, "-m", "mac", "--mac-source", str_mac, "-j", "MARK", "--set-mark", handle);
#if defined(CONFIG_IPV6)
		va_cmd(IP6TABLES, 12, 1, "-t", "mangle", "-A", UP_MAC_RATE_LIMIT, "-m", "mac", "--mac-source", str_mac, "-j", "MARK", "--set-mark", handle);
#endif
	}
	return 0;
}

int setup_software_downstream_mac_rate_limit(int i, MIB_CE_DATA_SPEED_LIMIT_MAC_Tp entry)
{
	char classid[10] = {0}, handle[32] = {0}, rate[16] = {0}, str_mac[32] = {0};
	char qdisc_handle[16] = {0};
	int port = 0;
	int j;
	int speed= getspeed();

	QOS_SETUP_PRINT_FUNCTION
	if(entry->speed_unit <= 0){
		return -1;
	}
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	snprintf(classid, sizeof(classid), "1:%d", i+1);
	snprintf(qdisc_handle, sizeof(qdisc_handle), "%d:", 1*100+i+1);
#else
	snprintf(classid, sizeof(classid), "1:%d", current_qos_wan_port*10+i+1);
	snprintf(qdisc_handle, sizeof(qdisc_handle), "%d:", 1*100+current_qos_wan_port*10+i+1);
#endif

	snprintf(rate, sizeof(rate), "%dkbit", (entry->speed_unit*speed));
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
	setup_sharemeter_rate(((MIB_DATA_SPEED_LIMIT_DOWN_MAC_TBL*10)+i), entry->speed_unit*speed, handle, 1);
#else
	snprintf(handle, sizeof(handle), "%d/0x%x", ((i << SOCK_MARK_QOS_SWQID_START) & SOCK_MARK_QOS_SWQID_BIT_MASK), SOCK_MARK_QOS_MASK_ALL);
#endif
	snprintf(str_mac, sizeof(str_mac), "%02x:%02x:%02x:%02x:%02x:%02x",
		entry->mac[0], entry->mac[1], entry->mac[2],
		entry->mac[3], entry->mac[4], entry->mac[5]);

	//tc class add dev br0 parent 1:1 classid 1:2 htb rate 1kbit ceil 512kbit
	va_cmd(TC, 13, 1, "class", (char *)ARG_ADD, "dev", LANIF, "parent", "1:1", "classid", classid, "htb", "rate", "1kbit", "ceil", rate);

	// tc qdisc add dev br0 parent 1:2 handle 102: pfifo limit 100
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF, "parent", classid, "handle", qdisc_handle, "pfifo", "limit", "100");

	//tc filter add dev br0 parent 1: protocol ip prio 1 handle 1001 fw classid 1:11
	va_cmd(TC, 15, 1, "filter", (char *)ARG_ADD, "dev", LANIF, "parent", "1:", "protocol", "all", "prio", "1", "handle", handle, "fw", "classid", classid);
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(getOpMode()!=BRIDGE_MODE){
		for(j=0; j<MAX_VC_NUM; j++){
			if(!wan_port_itfs[current_qos_wan_port].itf[j].name[0])
				continue;
			if(wan_port_itfs[current_qos_wan_port].itf[j].cmode==CHANNEL_MODE_BRIDGE){
				va_cmd(IPTABLES, 16, 1, "-t", "mangle", "-A", DOWN_MAC_RATE_LIMIT,"-m", "physdev", "--physdev-in",wan_port_itfs[current_qos_wan_port].itf[j].name,"-m", "mac", "--mac-dst", str_mac, "-j", "MARK", "--set-mark", handle);
#ifdef CONFIG_IPV6
				va_cmd(IP6TABLES, 16, 1, "-t", "mangle", "-A", DOWN_MAC_RATE_LIMIT,"-m", "physdev", "--physdev-in",wan_port_itfs[current_qos_wan_port].itf[j].name, "-m", "mac", "--mac-dst", str_mac, "-j", "MARK", "--set-mark", handle);
#endif
			}	
			else{
				va_cmd(IPTABLES, 14, 1, "-t", "mangle", "-A", DOWN_MAC_RATE_LIMIT,"-i", wan_port_itfs[current_qos_wan_port].itf[j].name, "-m", "mac", "--mac-dst", str_mac, "-j", "MARK", "--set-mark", handle);
#if defined(CONFIG_IPV6)
				va_cmd(IP6TABLES, 14, 1, "-t", "mangle", "-A", DOWN_MAC_RATE_LIMIT,"-i", wan_port_itfs[current_qos_wan_port].itf[j].name, "-m", "mac", "--mac-dst", str_mac, "-j", "MARK", "--set-mark", handle);
#endif
			}
		}
	}
	else
#endif
	{
		// iptables -t mangle -A FORWARD -m mac --mac-dst 00:00:03:00:01:00 -j MARK --set-mark 0x1/0xf000007
		va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", DOWN_MAC_RATE_LIMIT, "-m", "mac", "--mac-dst", str_mac, "-j", "MARK", "--set-mark", handle);
#if defined(CONFIG_IPV6)
		va_cmd(IP6TABLES, 12, 1, "-t", "mangle", "-A", DOWN_MAC_RATE_LIMIT, "-m", "mac", "--mac-dst", str_mac, "-j", "MARK", "--set-mark", handle);
#endif
	}
	return 0;
}

int setup_software_upstream_vlan_rate_limit(int i, MIB_CE_DATA_SPEED_LIMIT_VLAN_Tp entry)
{
	char classid[10] = {0}, handle[32] = {0}, rate[16] = {0};
	char qdisc_handle[16] = {0}, vlan_ifname[MAXFNAME]={0};
	int j=0, max_lan_itf_idx=0, untag_vlan=0;
	int speed= getspeed();
#ifdef QOS_SETUP_IFB
	char *devname="ifb0";
#endif
	int lan_idx=0, max_lan_port_num=0, phyPortId=0;
	int ethPhyPortId = rtk_port_get_wan_phyID();

	QOS_SETUP_PRINT_FUNCTION
	 for(lan_idx = 0; lan_idx < ELANVIF_NUM; lan_idx++){
		phyPortId = rtk_port_get_lan_phyID(lan_idx);
		if (phyPortId != -1 && phyPortId != ethPhyPortId)
		{
			max_lan_port_num++;
			continue;
		}
	}

	if(entry->speed_unit <= 0){
		return -1;
	}

	snprintf(classid, sizeof(classid), "2:%d", i+1);
	snprintf(qdisc_handle, sizeof(qdisc_handle), "%d:", 2*100+i+1);
	snprintf(rate, sizeof(rate), "%dkbit", (entry->speed_unit*speed));
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
	setup_sharemeter_rate(((MIB_DATA_SPEED_LIMIT_UP_VLAN_TBL*10)+i), entry->speed_unit*speed, handle, 1);
#else
	snprintf(handle, sizeof(handle), "%d/0x%x", ((i << SOCK_MARK_QOS_SWQID_START) & SOCK_MARK_QOS_SWQID_BIT_MASK), SOCK_MARK_QOS_MASK_ALL);
#endif
#ifdef QOS_SETUP_IFB
	//tc class add dev ifb0 parent 2:1 classid 2:2 htb rate 1kbit ceil 512kbit
	va_cmd(TC, 13, 1, "class", (char *)ARG_ADD, "dev", devname, "parent", "2:1", "classid", classid, "htb", "rate", "1kbit", "ceil", rate);

	// tc qdisc add dev ifb0 parent 2:2 handle 202: pfifo limit 100
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname, "parent", classid, "handle", qdisc_handle, "pfifo", "limit", "100");

	//tc filter add dev ifb0 parent 2: protocol ip prio 1 handle 2001 fw classid 2:1
	va_cmd(TC, 15, 1, "filter", (char *)ARG_ADD, "dev", devname, "parent", "2:", "protocol", "all", "prio", "1", "handle", handle, "fw", "classid", classid);
#endif

	max_lan_itf_idx = max_lan_port_num;
#ifdef WLAN_SUPPORT
	max_lan_itf_idx += WLAN_MBSSID_NUM;
#ifdef WLAN_DUALBAND_CONCURRENT
	max_lan_itf_idx += WLAN_MBSSID_NUM;
#endif
#endif

	for (j=0; j<max_lan_itf_idx; j++)
	{
		if(strlen(lan_itf_map_vid.map[j].name) > 0) {
			snprintf(vlan_ifname, sizeof(vlan_ifname),"%s.%d",  lan_itf_map_vid.map[j].name, entry->vlan);
#ifdef CONFIG_USER_AP_QOS 
			//untagged pkt will pass device, for example, eth0.2.0
			if(entry->vlan == -1){
				snprintf(vlan_ifname, sizeof(vlan_ifname),"%s.%d",  lan_itf_map_vid.map[j].name, untag_vlan);
			}
#if defined(CONFIG_IPV6) 
			va_cmd(IP6TABLES, 12, 1, "-t", "mangle", "-A", UP_VLAN_RATE_LIMIT, "-m", "physdev", "--physdev-in", vlan_ifname, "-j", "MARK", "--set-mark", handle);
#endif
#endif
			//iptables -t mangle -A PREROUTING -m physdev --physdev-in eth0.2.vlan  -j MARK --or-mark 2001
			va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", UP_VLAN_RATE_LIMIT, "-m", "physdev", "--physdev-in", vlan_ifname, "-j", "MARK", "--set-mark", handle);
		}
	}

	return 0;
}

int setup_software_downstream_vlan_rate_limit(int i, MIB_CE_DATA_SPEED_LIMIT_VLAN_Tp entry)
{
	char classid[10] = {0}, handle[32] = {0}, rate[16] = {0};
	char qdisc_handle[16] = {0}, vlan_ifname[MAXFNAME]={0};
	int port = 0, j=0, max_lan_itf_idx=0, untag_vlan=0;
	int lan_idx=0, max_lan_port_num=0, phyPortId=0;
	int ethPhyPortId = rtk_port_get_wan_phyID();
	int speed= getspeed();

	QOS_SETUP_PRINT_FUNCTION
	for(lan_idx = 0; lan_idx < ELANVIF_NUM; lan_idx++){
		phyPortId = rtk_port_get_lan_phyID(lan_idx);
		if (phyPortId != -1 && phyPortId != ethPhyPortId)
		{
			max_lan_port_num++;
			continue;
		}
	}

	if(entry->speed_unit <= 0){
		return -1;
	}

	snprintf(classid, sizeof(classid), "1:%d", i+1);
	snprintf(qdisc_handle, sizeof(qdisc_handle), "%d:", 1*100+i+1);
	snprintf(rate, sizeof(rate), "%dkbit", (entry->speed_unit*speed));
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
#ifndef CONFIG_USER_AP_QOS
	setup_sharemeter_rate(((MIB_DATA_SPEED_LIMIT_DOWN_VLAN_TBL*10)+i), entry->speed_unit*speed, handle, 0);
#else
	setup_sharemeter_rate(((MIB_DATA_SPEED_LIMIT_DOWN_VLAN_TBL*10)+i), entry->speed_unit*speed, handle, 1);
#endif
#else
	snprintf(handle, sizeof(handle), "%d/0x%x", ((i << SOCK_MARK_QOS_SWQID_START) & SOCK_MARK_QOS_SWQID_BIT_MASK), SOCK_MARK_QOS_MASK_ALL);
#endif
	//tc class add dev br0 parent 1:1 classid 1:2 htb rate 1kbit ceil 512kbit
	va_cmd(TC, 13, 1, "class", (char *)ARG_ADD, "dev", LANIF, "parent", "1:1", "classid", classid, "htb", "rate", "1kbit", "ceil", rate);

	// tc qdisc add dev br0 parent 1:2 handle 102: pfifo limit 100
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF, "parent", classid, "handle", qdisc_handle, "pfifo", "limit", "100");

	//tc filter add dev br0 parent 1: protocol ip prio 1 handle 1001 fw classid 1:11
	va_cmd(TC, 15, 1, "filter", (char *)ARG_ADD, "dev", LANIF, "parent", "1:", "protocol", "all", "prio", "1", "handle", handle, "fw", "classid", classid);

	max_lan_itf_idx = max_lan_port_num;
#ifdef WLAN_SUPPORT
	max_lan_itf_idx += WLAN_MBSSID_NUM;
#ifdef WLAN_DUALBAND_CONCURRENT
	max_lan_itf_idx += WLAN_MBSSID_NUM;
#endif
#endif
	for (j=0; j<max_lan_itf_idx; j++)
	{
		if(strlen(lan_itf_map_vid.map[j].name) > 0) {
			snprintf(vlan_ifname, sizeof(vlan_ifname),"%s.%d",  lan_itf_map_vid.map[j].name, entry->vlan);
#ifndef CONFIG_USER_AP_QOS
			///ebtables -A DOWN_VLAN_RATE_LIMIT -o eth0.2.80 -j mark --mark-or 0x4000000
			va_cmd(EBTABLES, 8, 1,	"-A", DOWN_VLAN_RATE_LIMIT, "-o", vlan_ifname, "-j", "mark", "--mark-or", handle);
#else
			//untagged pkt will pass device, for example, eth0.2.0
			if(entry->vlan == -1){
				snprintf(vlan_ifname, sizeof(vlan_ifname),"%s.%d",  lan_itf_map_vid.map[j].name, untag_vlan);
			}
			// iptables -t mangle -A down_vlan_rate_limit -m outphysdev --physdev eth0.2.100 -j MARK --set-mark 0x10/0x1fff
			va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", DOWN_VLAN_RATE_LIMIT, "-m", "outphysdev", "--physdev", vlan_ifname, "-j", "MARK", "--set-mark", handle);
#if defined(CONFIG_IPV6) 
			va_cmd(IP6TABLES, 12, 1, "-t", "mangle", "-A", DOWN_VLAN_RATE_LIMIT, "-m", "outphysdev", "--physdev", vlan_ifname, "-j", "MARK", "--set-mark", handle);
#endif
#endif
		}
	}

	return 0;
}

static int setup_data_speed_limit_if(int dir)
{
	MIB_CE_DATA_SPEED_LIMIT_IF_T entry;
	int mib_id = MIB_DATA_SPEED_LIMIT_UP_IF_TBL + dir;
	int total = mib_chain_total(mib_id);
	int i;

	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(mib_id, i, &entry) == 0)
			continue;

		if(entry.if_id < 1 || entry.if_id >12)
			continue;

		if(entry.speed_unit == 0)
			continue;

		if(dir == QOS_DIRECTION_DOWNSTREAM) {
			setup_software_downstream_if_rate_limit(i+1, &entry);
		}
		else {
			setup_software_upstream_if_rate_limit(i+1, &entry);
		}
	}
	return 0;
}

static int setup_data_speed_limit_mac(int dir)
{
	MIB_CE_DATA_SPEED_LIMIT_MAC_T entry;
	int mib_id = MIB_DATA_SPEED_LIMIT_UP_MAC_TBL + dir;
	int total = mib_chain_total(mib_id);
	int i;

	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(mib_id, i, &entry) == 0)
			continue;

		if(entry.speed_unit == 0)
			continue;

		if(dir == QOS_DIRECTION_DOWNSTREAM) {
			setup_software_downstream_mac_rate_limit(i+1, &entry);
		}
		else {
			setup_software_upstream_mac_rate_limit(i+1, &entry);
		}
	}
	return 0;
}

static int setup_data_speed_limit_ip(int dir)
{
	MIB_CE_DATA_SPEED_LIMIT_IP_T entry;
	int mib_id = MIB_DATA_SPEED_LIMIT_UP_IP_TBL + dir;
	int total = mib_chain_total(mib_id);
	int i;

	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(mib_id, i, &entry) == 0)
			continue;

		if(entry.speed_unit == 0)
			continue;

		if(dir == QOS_DIRECTION_DOWNSTREAM){
			setup_software_downstream_ip_rate_limit(i+1, &entry);
		}
		else{
			setup_software_upstream_ip_rate_limit(i+1, &entry);
		}
	}
	return 0;
}

static int setup_data_speed_limit_vlan(int dir)
{
	MIB_CE_DATA_SPEED_LIMIT_VLAN_T entry;
	int mib_id = MIB_DATA_SPEED_LIMIT_UP_VLAN_TBL + dir;
	int total = mib_chain_total(mib_id);
	int i;

	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(mib_id, i, &entry) == 0)
			continue;

		if(entry.speed_unit == 0)
			continue;

		if(dir == QOS_DIRECTION_DOWNSTREAM){
			setup_software_downstream_vlan_rate_limit(i+1, &entry);
		}
		else{
			setup_software_upstream_vlan_rate_limit(i+1, &entry);
		}
	}
	return 0;
}


static int setup_data_speed_limit()
{
	unsigned char mode;

	//UP
	mib_get(MIB_DATA_SPEED_LIMIT_UP_MODE, &mode);

	switch(mode)
	{
	case DATA_SPEED_LIMIT_MODE_IF:
		setup_data_speed_limit_if(QOS_DIRECTION_UPSTREAM);
		break;
	case DATA_SPEED_LIMIT_MODE_IP:
		setup_data_speed_limit_ip(QOS_DIRECTION_UPSTREAM);
		break;
	case DATA_SPEED_LIMIT_MODE_MAC:
		setup_data_speed_limit_mac(QOS_DIRECTION_UPSTREAM);
		break;
	case DATA_SPEED_LIMIT_MODE_VLAN:
		setup_data_speed_limit_vlan(QOS_DIRECTION_UPSTREAM);
		break;
	default:
		fprintf(stderr, "<%s:%d> Unknown data speed limit mode: %d\n", __func__, __LINE__, mode);
	case DATA_SPEED_LIMIT_MODE_DISABLE:
		break;
	}

	//DOWN
	mib_get(MIB_DATA_SPEED_LIMIT_DOWN_MODE, &mode);

	switch(mode)
	{
	case DATA_SPEED_LIMIT_MODE_IF:
		setup_data_speed_limit_if(QOS_DIRECTION_DOWNSTREAM);
		break;
	case DATA_SPEED_LIMIT_MODE_IP:
		setup_data_speed_limit_ip(QOS_DIRECTION_DOWNSTREAM);
		break;
	case DATA_SPEED_LIMIT_MODE_MAC:
		setup_data_speed_limit_mac(QOS_DIRECTION_DOWNSTREAM);
#ifdef CONFIG_ETH_AND_WLAN_QOS_SUPPORT
		setup_data_speed_limit_wlan_mac_qos(1);
#endif
		break;
	case DATA_SPEED_LIMIT_MODE_VLAN:
		setup_data_speed_limit_vlan(QOS_DIRECTION_DOWNSTREAM);
		break;
	default:
		fprintf(stderr, "<%s:%d> Unknown data speed limit mode: %d\n", __func__, __LINE__, mode);
	case DATA_SPEED_LIMIT_MODE_DISABLE:
		break;
	}

	return 0;
}

#endif

int findQueueMinWeigh()
{
	int nBytes=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, i;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(current_qos_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
#endif
	if((qEntryNum=mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		return nBytes;
	for(i=0;i<qEntryNum; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i, (void*)&qEntry))
			continue;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(qEntry.q_logic_wan_port != current_qos_wan_port)
			continue;
#endif
		if ( !qEntry.enable)
			continue;

		if(nBytes==0)
			nBytes = qEntry.weight;

		if(qEntry.weight < nBytes)
			nBytes = qEntry.weight;
	}
	return nBytes;
}

int findDownstreamQueueMinWeigh()
{
	int nBytes=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, i;

	if((qEntryNum=mib_chain_total(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL)) <=0)
		return nBytes;
	for(i=0;i<qEntryNum; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL, i, (void*)&qEntry))
			continue;
		if ( !qEntry.enable || qEntry.weight == 0)
			continue;

		if(nBytes==0)
			nBytes = qEntry.weight;

		if(qEntry.weight < nBytes)
			nBytes = qEntry.weight;
	}
	return nBytes;
}

#ifdef CONFIG_00R0
unsigned int get_next_QoSclass_instnum()
{
	unsigned int num = 0, instNum = 0, i = 0, MAXinstNum = 0, used = 0;
	MIB_CE_IP_QOS_T *p, qos_entity;

	p = &qos_entity;
	num = mib_chain_total(MIB_IP_QOS_TBL);

	// find MAX instnum
	for (i = 0; i < num; i++) {
		if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)p))
			continue;

		if (p->InstanceNum > MAXinstNum) {
			MAXinstNum = p->InstanceNum;
		}
	}

	// find available instnum
	for (instNum = 1; instNum <= MAXinstNum; instNum++) {
		used = 0;
		for (i = 0; i < num; i++) {
			if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)p))
				continue;

			if (p->InstanceNum == instNum) {
				used = 1;
				break;
			}
		}

		if (used == 0) {
			fprintf(stderr, "[%s:%s@%d] get %d \n", __FILE__, __FUNCTION__, __LINE__, instNum);
			return instNum;
		}
	}

	if (used == 1) {
		fprintf(stderr, "[%s:%s@%d] get %d \n", __FILE__, __FUNCTION__, __LINE__, (MAXinstNum + 1));
		return (MAXinstNum + 1);
	}
	else {
		fprintf(stderr, "[%s:%s@%d] get %d \n", __FILE__, __FUNCTION__, __LINE__, (num + 1));
		return (num + 1);
	}
}
#endif

int ifSumWeighIs100()
{
	int sum=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, i;

	if((qEntryNum=mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		return 0;
	for(i=0;i<qEntryNum; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i, (void*)&qEntry))
			continue;
		if ( !qEntry.enable)
			continue;
		sum += qEntry.weight;
	}

	if (sum == 100)
		return 1;
	else
		return 0;
}

int isQueneEnableForRule(unsigned char prior)
{
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, j;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(current_qos_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
#endif
	if((qEntryNum=mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		return 0;
	for(j=0;j<qEntryNum; j++)
	{
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, j, (void*)&qEntry))
			continue;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(qEntry.q_logic_wan_port != current_qos_wan_port)
			continue;
		if(j == (prior-1)+current_qos_wan_port*MAX_QOS_QUEUE_NUM)
#else
		if (j == (prior-1))
#endif
		{
			if (qEntry.enable)
				return 1;
			break;
		}
	}
	return 0;
}
int SumWeightOfWRR()
{
	int sum=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, i;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(current_qos_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
#endif
	if((qEntryNum=mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		return 0;
	for(i=0;i<qEntryNum; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i, (void*)&qEntry))
			continue;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(qEntry.q_logic_wan_port != current_qos_wan_port)
			continue;
#endif
		if(!qEntry.enable)
			continue;
		sum += qEntry.weight;
	}

	return sum;
}

int ifDownstreamSumWeighIs100()
{
	int sum=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, i;

	if((qEntryNum=mib_chain_total(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL)) <=0)
		return 0;
	for(i=0;i<qEntryNum; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL, i, (void*)&qEntry))
			continue;
		if ( !qEntry.enable)
			continue;
		sum += qEntry.weight;
	}

	if (sum == 100)
		return 1;
	else
		return 0;
}

int currentDownstreamSumWeigh()
{
	int sum=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, i;

	if((qEntryNum=mib_chain_total(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL)) <=0)
		return 0;
	for(i=0;i<qEntryNum; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL, i, (void*)&qEntry))
			continue;
		if ( !qEntry.enable)
			continue;
		sum += qEntry.weight;
	}

	return sum;
}

int isDownstreamQueneEnableForRule(unsigned char prior)
{
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, j;

	if((qEntryNum=mib_chain_total(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL)) <=0)
		return 0;
	for(j=0;j<qEntryNum; j++)
	{
		if(!mib_chain_get(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL, j, (void*)&qEntry))
			continue;
		if (j == (prior-1))
		{
			if (qEntry.enable)
				return 1;
			break;
		}
	}
	return 0;
}

int DownstreamSumWeightOfWRR()
{
	int sum=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, i;

	if((qEntryNum=mib_chain_total(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL)) <=0)
		return 0;
	for(i=0;i<qEntryNum; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL, i, (void*)&qEntry))
			continue;
		if(!qEntry.enable)
			continue;
		sum += qEntry.weight;
	}

	return sum;
}

int set_wrr_class_qdisc()
{
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, minWeigh, j;
#ifdef QOS_SETUP_IFB
	char *devname="ifb0";
#else
	char *devname="imq0";
#endif
	char s_classid[16], s_handle[16], s_qlen[16];
	int totalWeight = SumWeightOfWRR();
	int logical_wan_port = 0;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(current_qos_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
	logical_wan_port = current_qos_wan_port;
	if(!(devname=(char*)getQosRootWanName(logical_wan_port)))
		return 0;
#endif
	if((qEntryNum=mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		return 0;
	for(j=0;j<qEntryNum; j++)
	{
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, j, (void*)&qEntry))
			continue;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(qEntry.q_logic_wan_port != logical_wan_port)
			continue;
#endif
		if ( !qEntry.enable
	#ifdef CONFIG_USER_AP_E8B_QOS
			&& (j != (MAX_QOS_QUEUE_NUM-1)) 	// remain lowest queue as default
	#endif
		)
			continue;

		snprintf(s_classid, 16, "1:%d00", j+1);
		snprintf(s_handle, 16, "%d00:", j+1);
		minWeigh = findQueueMinWeigh();
	#ifdef CONFIG_USER_AP_QOS
		snprintf(s_qlen, 16, "%d", 300*qEntry.weight/totalWeight);
	#else
		snprintf(s_qlen, 16, "%d", 30*qEntry.weight/totalWeight);
	#endif
		//printf("set_wrr_class_qdisc: s_qlen=%s\n", s_qlen);

		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
		"parent", s_classid, "handle", s_handle, "pfifo", "limit", s_qlen);

	}
	return 1;
}

int set_downstream_wrr_class_qdisc()
{
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, minWeigh, j;
#ifdef QOS_SETUP_IFB
	char *devname="ifb1";
#else
	char *devname="imq1";
#endif
	char s_classid[16], s_handle[16], s_qlen[16];
	int totalWeight = DownstreamSumWeightOfWRR();
	if((qEntryNum=mib_chain_total(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL)) <=0)
		return 0;
	for(j=0;j<qEntryNum; j++)
	{
		if(!mib_chain_get(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL, j, (void*)&qEntry))
			continue;
		if ( !qEntry.enable
	#ifdef CONFIG_USER_AP_E8B_QOS
			&& (j != (MAX_QOS_QUEUE_NUM-1)) 	// remain lowest queue as default
	#endif
		)
			continue;

		snprintf(s_classid, 16, "1:%d00", j+1);
		snprintf(s_handle, 16, "%d00:", j+1);
		minWeigh = findDownstreamQueueMinWeigh();
	#ifndef CONFIG_USER_AP_E8B_QOS
		snprintf(s_qlen, 16, "%d", 30*qEntry.weight/totalWeight);
	#else
		snprintf(s_qlen, 16, "%d", 300*qEntry.weight/totalWeight);
	#endif
		//printf("set_wrr_class_qdisc: s_qlen=%s\n", s_qlen);

		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
		"parent", s_classid, "handle", s_handle, "pfifo", "limit", s_qlen);

	}
	return 1;
}

//get state about qos from pvc interface, 0: false, 1: true
static unsigned int get_pvc_qos_state(unsigned int ifIndex)
{
	MIB_CE_ATM_VC_T entry;
	int i, entryNum = 0;
	unsigned int ret = 0;

	// Mason Yu.
	if ( ifIndex == DUMMY_IFINDEX )
		return QOS_WAN_VLAN_ENABLE;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i< entryNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
			continue;

		if(entry.ifIndex == ifIndex)
		{
			if(entry.enableIpQos)
				ret |= QOS_WAN_ENABLE;
			if(entry.vlan)
				ret |= QOS_WAN_VLAN_ENABLE;
			if (entry.cmode == CHANNEL_MODE_BRIDGE)
				ret |= QOS_WAN_BRIDGE;
			break;
		}
	}

	return ret;
}

/****************************************
* getUpLinkRate:
* DESC: get upstream link rate.
****************************************/
unsigned int getWanUpLinkRate(void)
{
	unsigned int total_bandwidth = 102400;//default to be 100Mbps

#ifdef CONFIG_DEV_xDSL
	Modem_LinkSpeed vLs;
	unsigned char ret = 0;

	ret = adsl_drv_get(RLCM_GET_LINK_SPEED, (void *)&vLs, RLCM_GET_LINK_SPEED_SIZE);
	if (ret) {
		if(0 != vLs.upstreamRate)
			total_bandwidth = vLs.upstreamRate;
	}else
#endif
	{
		total_bandwidth = MAX_WAN_LINK_RATE;
	}

	return total_bandwidth;
}

unsigned int getWanDownLinkRate(void)
{
	unsigned int total_bandwidth = 1024000;//default to be 1G

	return total_bandwidth;
}

unsigned int getUpLinkRate(void)
{
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	unsigned int total_bandwidth = 1024;//default to be 1Mbps
	unsigned char policy = 0;
#else
	unsigned char policy[QOS_MAX_PORT_NUM] = {0};
	unsigned int total_bandwidth[QOS_MAX_PORT_NUM] = {0};
	if(current_qos_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
	total_bandwidth[current_qos_wan_port] = 1024;//default value
#endif
	if(mib_get_s(MIB_TOTAL_BANDWIDTH_LIMIT_EN, &policy, sizeof(policy)))
	{
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(policy==0 || !getQosEnable())
#else
		if(policy[current_qos_wan_port]==0 || !getQosEnable())
#endif
		{
			/* Use auto selection */
			return getWanUpLinkRate();
		}
		else
		{
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
			//  Because we can not distinguish total bandwidth. It is for total bandwidth limit. Let user to input total bandwidth.
			if(mib_get_s(MIB_TOTAL_BANDWIDTH, &total_bandwidth, sizeof(total_bandwidth)))
			{
#if defined(CONFIG_USER_AP_E8B_QOS)
				if(total_bandwidth == 0)	// means no rate limit
					total_bandwidth = QOS_BANDWIDTH_UNLIMITED;
#endif
				return (total_bandwidth-8); // -8kbps can let 512kbps wrr test seems perfect on TC.
			}
#else
			if(mib_get_s(MIB_MULTI_TOTAL_BANDWIDTH, &total_bandwidth, sizeof(total_bandwidth)))
				return total_bandwidth[current_qos_wan_port];

#endif
		}
	}
	else
		return 1024;

	return 1024;
}

unsigned int getDownLinkRate(void)
{
	unsigned int total_bandwidth = 1024;//default to be 1Mbps
	unsigned char policy = 0;

	if(mib_get_s(MIB_TOTAL_DOWNSTREAM_BANDWIDTH_LIMIT_EN, &policy, sizeof(policy)))
	{
		if(policy==0 || !getDownstreamQosEnable())
		{
			return getWanDownLinkRate();
		}
		else
		{
			//  Because we can not distinguish total bandwidth. It is for total bandwidth limit. Let user to input total bandwidth.
			if(mib_get(MIB_TOTAL_DOWNSTREAM_BANDWIDTH, &total_bandwidth))
			{
				return total_bandwidth;
			}
			else
				return 1024;
		}
	}

	return 0;
}

int getQosEnable(void)
{
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	unsigned char qosEnable;
#else
	unsigned char qosEnable[QOS_MAX_PORT_NUM] = {0};
	if(current_qos_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
#endif
	mib_get_s(MIB_QOS_ENABLE_QOS, (void*)&qosEnable, sizeof(qosEnable));
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	return (int)qosEnable;
#else
	return (int)qosEnable[current_qos_wan_port];
#endif
}

int getDownstreamQosEnable(void)
{
	unsigned char qosEnable;

	mib_get_s(MIB_QOS_ENABLE_DOWNSTREAM_QOS, (void*)&qosEnable, sizeof(qosEnable));
	return (int)qosEnable;
}

int getQosRuleNum()
{
	MIB_CE_IP_QOS_T entry;
	int i, ruleNum;
	int sum=0;

	ruleNum = mib_chain_total(MIB_IP_QOS_TBL);
	if (0 == ruleNum)
		return 0;

	for (i=0; i<ruleNum; i++)
	{
		if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void *)&entry) || !entry.enable)
			continue;

		sum++;
	}

	return(sum);
}

static int isQosMacRule(MIB_CE_IP_QOS_Tp pEntry)
{
	if (pEntry->protoType || *((uint32_t *)pEntry->sip) || *((uint32_t *)pEntry->dip) || pEntry->sPort ||
		pEntry->dPort
		|| pEntry->qosDscp
		)
		return 0;
	return 1;
}

static int mac_hex2string(unsigned char *hex, char *str)
{
	snprintf(str, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
		hex[0], hex[1],	hex[2], hex[3],	hex[4], hex[5]);
	return 0;
}

static int setupCarChain(unsigned int enable)
{
	QOS_SETUP_PRINT_FUNCTION

	if (enable)
	{
		va_cmd(IPTABLES, 2, 1, "-N", CAR_US_FILTER_CHAIN);
		va_cmd(IPTABLES, 5, 1, "-I", "FORWARD", "1", "-j", CAR_US_FILTER_CHAIN);

		va_cmd(IPTABLES, 2, 1, "-N", CAR_DS_FILTER_CHAIN);
		va_cmd(IPTABLES, 5, 1, "-I", "FORWARD", "1", "-j", CAR_DS_FILTER_CHAIN);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", CAR_US_TRAFFIC_CHAIN);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", CAR_US_TRAFFIC_CHAIN);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", CAR_DS_TRAFFIC_CHAIN);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "PREROUTING", "-j", CAR_DS_TRAFFIC_CHAIN);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", "pvc_mark");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", "pvc_mark");

	#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 2, 1, "-N", CAR_US_FILTER_CHAIN);
		va_cmd(IP6TABLES, 5, 1, "-I", "FORWARD", "1", "-j", CAR_US_FILTER_CHAIN);

		va_cmd(IP6TABLES, 2, 1, "-N", CAR_DS_FILTER_CHAIN);
		va_cmd(IP6TABLES, 5, 1, "-I", "FORWARD", "1", "-j", CAR_DS_FILTER_CHAIN);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", CAR_US_TRAFFIC_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", CAR_US_TRAFFIC_CHAIN);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", CAR_DS_TRAFFIC_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "PREROUTING", "-j", CAR_DS_TRAFFIC_CHAIN);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", "pvc_mark");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", "pvc_mark");
	#endif

		va_cmd(EBTABLES, 2, 1, "-N", CAR_TRAFFIC_EBT_CHAIN);
		va_cmd(EBTABLES, 3, 1, "-P", CAR_TRAFFIC_EBT_CHAIN, "RETURN");
		va_cmd(EBTABLES, 5, 1, "-I", "FORWARD", "1", "-j", CAR_TRAFFIC_EBT_CHAIN);
	} else {
		va_cmd(IPTABLES, 2, 1, "-F", CAR_US_FILTER_CHAIN);
		va_cmd(IPTABLES, 2, 1, "-F", CAR_DS_FILTER_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", CAR_US_TRAFFIC_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", CAR_DS_TRAFFIC_CHAIN);

		va_cmd(IPTABLES, 4, 1, "-D", "FORWARD", "-j", CAR_US_FILTER_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-D", "FORWARD", "-j", CAR_DS_FILTER_CHAIN);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", "pvc_mark");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", CAR_US_TRAFFIC_CHAIN);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "PREROUTING", "-j", CAR_DS_TRAFFIC_CHAIN);

		va_cmd(IPTABLES, 2, 1, "-X", CAR_US_FILTER_CHAIN);
		va_cmd(IPTABLES, 2, 1, "-X", CAR_DS_FILTER_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", "pvc_mark");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", CAR_US_TRAFFIC_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", CAR_DS_TRAFFIC_CHAIN);

	#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 2, 1, "-F", CAR_US_FILTER_CHAIN);
		va_cmd(IP6TABLES, 2, 1, "-F", CAR_DS_FILTER_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", CAR_US_TRAFFIC_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", CAR_DS_TRAFFIC_CHAIN);

		va_cmd(IP6TABLES, 4, 1, "-D", "FORWARD", "-j", CAR_US_FILTER_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-D", "FORWARD", "-j", CAR_DS_FILTER_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", "pvc_mark");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", CAR_US_TRAFFIC_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "PREROUTING", "-j", CAR_DS_TRAFFIC_CHAIN);

		va_cmd(IP6TABLES, 2, 1, "-X", CAR_US_FILTER_CHAIN);
		va_cmd(IP6TABLES, 2, 1, "-X", CAR_DS_FILTER_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", "pvc_mark");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", CAR_US_TRAFFIC_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", CAR_DS_TRAFFIC_CHAIN);
	#endif

		va_cmd(EBTABLES, 2, 1, "-F", CAR_TRAFFIC_EBT_CHAIN);
		va_cmd(EBTABLES, 4, 1, "-D", "FORWARD", "-j", CAR_TRAFFIC_EBT_CHAIN);
		va_cmd(EBTABLES, 2, 1, "-X", CAR_TRAFFIC_EBT_CHAIN);
	}

	return 1;
}

/*
 *                              HTB(root qdisc, handle 10:)
 *                               |
 *                              HTB(root class, classid 10:1)
 *            ___________________|_____________________
 *            |         |        |          |          |
 *           HTB       HTB      HTB        HTB        HTB
 *(subclass id 10:10 rate Xkbit)........       (sub class id 10:N0 rate Ykbit)
 */
static int setupCarQdisc(unsigned int enable)
{
	MIB_CE_ATM_VC_T vcEntry;
	int i, vcEntryNum;
	char ifname[IFNAMSIZ];
	char s_rate[16], s_ceil[16];
	unsigned char totalBandWidthEn;
	unsigned int bandwidth;
	unsigned char totalDownstreamBandWidthEn;
	unsigned int downstreamBandwidth;
	unsigned int rate, ceil;

	QOS_SETUP_PRINT_FUNCTION

	mib_get_s(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&totalBandWidthEn, sizeof(totalBandWidthEn));

	if (totalBandWidthEn) {
		mib_get_s(MIB_TOTAL_BANDWIDTH, &bandwidth, sizeof(bandwidth));
	}
	else
		bandwidth = getUpLinkRate();

	hwnat_qos_bandwidth_setup(bandwidth*1024);
	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<vcEntryNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry)||!vcEntry.enableIpQos)
			continue;

		ifGetName(PHY_INTF(vcEntry.ifIndex), ifname, sizeof(ifname));

		if (!enable) {
			va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", ifname, "root");
			continue;
		}
		
		//patch: actual bandwidth maybe a little greater than configured limit value, so I minish 7% of the configured limit value ahead.
		if (totalBandWidthEn)
			//ceil = bandwidth/100 * 93;
			ceil = bandwidth*93/100;
		else
			ceil = bandwidth;

		// delete default prio qdisc
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", ifname, "root");
		//tc qdisc add dev $DEV root handle 1: htb default 2 r2q 1		
		va_cmd(TC, 12, 1, "qdisc", (char *)ARG_ADD, "dev", ifname,
			"root", "handle", "1:", "htb", "default", "2", "r2q", "1");

		// root class
		snprintf(s_rate, 16, "%dKbit", ceil);
		//tc class add dev $DEV parent 1: classid 1:1 htb rate $RATE ceil $CEIL
		va_cmd(TC, 17, 1, "class", (char *)ARG_ADD, "dev", ifname,
			"parent", "1:", "classid", "1:1", "htb", "rate", s_rate, "ceil", s_rate,
			"mpu", "64", "overhead", "4");

		//patch with above
		rate = (ceil>10)?10:ceil;

		// default class
		snprintf(s_rate, 16, "%dKbit", rate);
		snprintf(s_ceil, 16, "%dKbit", ceil);
		//tc class add dev $DEV parent 1:1 classid 1:2 htb rate $RATE ceil $CEIL
		va_cmd(TC, 17, 1, "class", (char *)ARG_ADD, "dev", ifname,
			"parent", "1:1", "classid", "1:2", "htb", "rate",
			s_rate, "ceil", s_ceil, "mpu", "64", "overhead", "4");

		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", ifname,
			"parent", "1:2", "handle", "2:", "pfifo", "limit", "100");
	}

	mib_get_s(MIB_TOTAL_DOWNSTREAM_BANDWIDTH_LIMIT_EN, (void *)&totalDownstreamBandWidthEn, sizeof(totalDownstreamBandWidthEn));

	if (totalDownstreamBandWidthEn) {
		mib_get_s(MIB_TOTAL_DOWNSTREAM_BANDWIDTH, &downstreamBandwidth, sizeof(downstreamBandwidth));
	}
	else
		downstreamBandwidth = getDownLinkRate();

	if (!enable) {
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", LANIF, "root");
	} else {
		if (totalDownstreamBandWidthEn)
			//ceil = bandwidth/100 * 93;
			ceil = downstreamBandwidth*93/100;
		else
			ceil = downstreamBandwidth;

		// delete default prio qdisc
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", LANIF, "root");
		//tc qdisc add dev $DEV root handle 1: htb default 2 r2q 1		
		va_cmd(TC, 12, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF,
			"root", "handle", "1:", "htb", "default", "2", "r2q", "1");

		// root class
		snprintf(s_rate, 16, "%dKbit", ceil);
		//tc class add dev $DEV parent 1: classid 1:1 htb rate $RATE ceil $CEIL
		va_cmd(TC, 17, 1, "class", (char *)ARG_ADD, "dev", LANIF,
			"parent", "1:", "classid", "1:1", "htb", "rate", s_rate, "ceil", s_rate,
			"mpu", "64", "overhead", "4");
	}
	
	return 0;
}

static int setupCarRule_one(MIB_CE_IP_TC_Tp entry)
{
	char ifname[IFNAMSIZ]={0};
	char* tc_act = NULL, *fw_act=NULL;
	char* proto1[2]={0}, *proto2[2]={0}; // [0] for IPv4, [1] for IPv6
	char wanPort[64]={0};
	char  saddr[55], daddr[55], sport[55], dport[55];
	char  saddr6[55], daddr6[55];
	char tmpstr[48], etherType[55];
	int upLinkRate=0, childRate=0, meter_idx;
	unsigned int mark=0, mark_mask=0;
	unsigned char meterOpt = 0;
	unsigned int upstreamCri = 0, downstreamCri = 0;
#ifdef CONFIG_RTK_SKB_MARK2
	unsigned long long mark2=0, mark2_mask=0;
	unsigned char use_mark2=0;
#endif
	char *iptables_cmd=NULL, *ip6tables_cmd=NULL, *tc_protocol=NULL;
	char traffic_chain[64];
	char filter_chain[64];
	int i=0, vcEntryNum=0;
	MIB_CE_ATM_VC_T vcEntry;
	
	DOCMDINIT;

	QOS_SETUP_PRINT_FUNCTION

	tc_act = (char*)ARG_ADD;
	fw_act = (char*)FW_ADD;

	if(NULL == entry) {
		printf("Null traffic contolling rule!\n");
		return 1;
	}
	
	mib_get_s(MIB_QOS_FLOW_HWMETER_OPTION, &meterOpt, sizeof(meterOpt));
	mib_get_s(MIB_QOS_FLOW_HWMETER_UPSTREAM_CRI, &upstreamCri, sizeof(upstreamCri));
	mib_get_s(MIB_QOS_FLOW_HWMETER_DOWNSTREAM_CRI, &downstreamCri, sizeof(downstreamCri));

	memset(&vcEntry, 0, sizeof(vcEntry));
	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0 ; i<vcEntryNum ; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry))
			continue;

		if (!vcEntry.enableIpQos)
			continue;

		if(!(entry->ifIndex == DUMMY_IFINDEX || !entry->ifIndex) && entry->ifIndex != vcEntry.ifIndex)
		{
			continue;
		}
		
		ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));

#if 0 //def CONFIG_USER_AP_QOS
		ifGetName(PHY_INTF(entry->ifIndex), ifname, sizeof(ifname));
		printf("[%s:%d] wanPort: %s, ifname: %s\n", __FUNCTION__, __LINE__, wanPort, ifname);
#endif

		if (ifname[0] != '\0')
		{
			if(entry->direction == QOS_DIRECTION_DOWNSTREAM) {
				if(vcEntry.cmode == CHANNEL_MODE_BRIDGE)
					snprintf(wanPort, sizeof(wanPort), "-m physdev --physdev-in %s", ifname);
				else
					snprintf(wanPort, sizeof(wanPort), "-i %s", ifname);
			} else {
				if(vcEntry.cmode == CHANNEL_MODE_BRIDGE)
					snprintf(wanPort, sizeof(wanPort), "-m physdev --physdev-out %s", ifname);
				else
					snprintf(wanPort, sizeof(wanPort), "-o %s", ifname);
			}
		}
		
		{
#ifdef CONFIG_IPV6
			// This is a IPv4 rule
			if ( entry->IpProtocol & IPVER_IPV4 ) {
				//source address and netmask
				snprintf(tmpstr, sizeof(tmpstr), "%s", inet_ntoa(*((struct in_addr *)entry->srcip)));
				if (strcmp(tmpstr, ARG_0x4)) {
					if(0 != entry->smaskbits) {
						snprintf(saddr, sizeof(saddr), "-s %s/%d", tmpstr, entry->smaskbits);
					} else {
						snprintf(saddr, sizeof(saddr), "-s %s", tmpstr);
					}
				}
				else {//if not specify the source ip
					saddr[0] = '\0';
				}
			}
			// This is a IPv6 rule
			if ( entry->IpProtocol & IPVER_IPV6 ) {
				//source address and netmask
				inet_ntop(PF_INET6, (struct in6_addr *)entry->sip6, tmpstr, sizeof(tmpstr));
				if (strcmp(tmpstr, "::")) {
					if(0 != entry->sip6PrefixLen) {
						snprintf(saddr6, sizeof(saddr6), "-s %s/%d", tmpstr, entry->sip6PrefixLen);
					} else {
						snprintf(saddr6, sizeof(saddr6), "-s %s", tmpstr);
					}
				}
				else {//if not specify the source ip
					saddr6[0] = '\0';
				}
			}
#else
			//source address and netmask
			snprintf(tmpstr, sizeof(tmpstr), "%s", inet_ntoa(*((struct in_addr *)entry->srcip)));
			if (strcmp(tmpstr, ARG_0x4)) {
				if(0 != entry->smaskbits) {
					snprintf(saddr, sizeof(saddr), "-s %s/%d", tmpstr, entry->smaskbits);
				} else {
					snprintf(saddr, sizeof(saddr), "-s %s", tmpstr);
				}
			}
			else {//if not specify the source ip
				saddr[0] = '\0';
			}
#endif

#ifdef CONFIG_IPV6
			// This is a IPv4 rule
			if ( entry->IpProtocol & IPVER_IPV4 ) {
				//destination address and netmask
				snprintf(tmpstr, sizeof(tmpstr), "%s", inet_ntoa(*((struct in_addr *)entry->dstip)));
				if (strcmp(tmpstr, ARG_0x4)) {
					if(0 != entry->dmaskbits) {
						snprintf(daddr, sizeof(daddr), "-d %s/%d", tmpstr, entry->dmaskbits);
					} else {
						snprintf(daddr, sizeof(daddr), "-d %s", tmpstr);
					}
				} else {//if not specify the dest ip
					daddr[0] = '\0';
				}
			}
			
			// This is a IPv6 rule
			if ( entry->IpProtocol & IPVER_IPV6 ) {
				//destination address and netmask
				inet_ntop(PF_INET6, (struct in6_addr *)entry->dip6, tmpstr, sizeof(tmpstr));
				if (strcmp(tmpstr, "::")) {
					if(0 != entry->dip6PrefixLen) {
						snprintf(daddr6, sizeof(daddr6), "-d %s/%d", tmpstr, entry->dip6PrefixLen);
					} else {
						snprintf(daddr6, sizeof(daddr6), "-d %s", tmpstr);
					}
				} else {//if not specify the dest ip
					daddr6[0] = '\0';
				}
			}
#else
			//destination address and netmask
			snprintf(tmpstr, sizeof(tmpstr), "%s", inet_ntoa(*((struct in_addr *)entry->dstip)));
			if (strcmp(tmpstr, ARG_0x4)) {
				if(0 != entry->dmaskbits) {
					snprintf(daddr, sizeof(daddr), "-d %s/%d", tmpstr, entry->dmaskbits);
				} else {
					snprintf(daddr, sizeof(daddr), "-d %s", tmpstr);
				}
			} else {//if not specify the dest ip
				daddr[0] = '\0';
			}
#endif

			//source port
			if(0 != entry->sport) {
				snprintf(sport, sizeof(sport), "--sport %d", entry->sport);
			} else {
				sport[0] = '\0';
			}

			//destination port
			if(0 != entry->dport) {
				snprintf(dport, sizeof(sport), "--dport %d", entry->dport);
			} else {
				dport[0] = '\0';
			}

			//protocol
			//if (((0 != entry->sport) || (0 != entry->dport)) &&
			//	(entry->protoType < 2))
			//	entry->protoType = 4;

			if(entry->protoType>4)//wrong protocol index
			{
				printf("Wrong protocol\n");
				return 1;
			} else {
				switch(entry->protoType)
				{
					case PROTO_NONE://NONE
						proto1[0] = proto1[1] = " ";
						break;
					case PROTO_ICMP://ICMP
#ifdef CONFIG_IPV6
						// This is a IPv4 rule
						if ( entry->IpProtocol & IPVER_IPV4 )
							proto1[0] = "-p ICMP";
						// This is a IPv6 rule
						if ( entry->IpProtocol & IPVER_IPV6 )
							proto1[1] = "-p ICMPV6";
#else
						proto1[0] = proto1[1] = "-p ICMP";
#endif
						break;
					case PROTO_TCP://TCP
						proto1[0] = proto1[1] = "-p TCP";
						break;
					case PROTO_UDP://UDP
						proto1[0] = proto1[1] = "-p UDP";
						break;
					case PROTO_UDPTCP://TCP/UDP
						proto1[0] = proto1[1] = "-p TCP";
						proto2[0] = proto2[1] = "-p UDP";
						break;
				}
			}

#ifdef CONFIG_IPV6
				// This is a IPv4 rule
				if ( entry->IpProtocol & IPVER_IPV4 ) {
					iptables_cmd = "/bin/iptables";
					tc_protocol = "protocol ip";
				}
				// This is a IPv6 rule
				if ( entry->IpProtocol & IPVER_IPV6 ) {
					ip6tables_cmd = "/bin/ip6tables";
					tc_protocol = "protocol ipv6";
				}
				if ( entry->IpProtocol == IPVER_IPV4_IPV6 ) {
					tc_protocol = "protocol all";
				}
#else
				// This is a IPv4 rule
				iptables_cmd = "/bin/iptables";
				tc_protocol = "protocol ip";
#endif

#if defined(CONFIG_USER_AP_QOS)
			/* 	In case WAN=PPPoE, and qdisc builds in eth interface, 
				"protocol ip" cannot match, replace it with "protocol all"; 
				iptables/ip6tables do the classifications. */
			tc_protocol = "protocol all";
#endif

			upLinkRate = entry->limitSpeed;
			if(0 != upLinkRate)
			{
				//get mark
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
				meter_idx = rtk_qos_share_meter_get(RTK_APP_TYPE_IP_QOS_RATE_SHAPING, entry->entryid, entry->limitSpeed);
				if (meter_idx < 0)
					meter_idx = rtk_qos_share_meter_set(RTK_APP_TYPE_IP_QOS_RATE_SHAPING, entry->entryid, entry->limitSpeed);
				if(meter_idx != -1)
				{
#ifdef CONFIG_RTK_SKB_MARK2
					//for downstream rate limit use hw meter
					if(((meterOpt & 0x2) && entry->direction == QOS_DIRECTION_DOWNSTREAM && entry->limitSpeed >= downstreamCri) || 
						((meterOpt & 0x1) && entry->direction == QOS_DIRECTION_UPSTREAM && entry->limitSpeed >= upstreamCri))
					{
						use_mark2 = 1;
						mark2 |= SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(1);
						mark2_mask = SOCK_MARK2_METER_HW_INDEX_ENABLE_BIT_MASK;
					}
					else
#endif
					mark |= SOCK_MARK_METER_INDEX_ENABLE_TO_MARK(1);
					mark_mask |= SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK;
				}
				else
				{
					printf("%s %d: rtk_qos_share_meter_set FAIL !\n", __func__, __LINE__);
					meter_idx = (entry->entryid);
				}
#else
				meter_idx = (entry->entryid);
#endif
				mark_mask |= SOCK_MARK_METER_INDEX_BIT_MASK;
				mark |= SOCK_MARK_METER_INDEX_TO_MARK(meter_idx);
				AUG_PRT("meter_idx = %d\n", meter_idx);
				AUG_PRT("mark = 0x%x\n", mark);
				
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_FC_SPECIAL_FAST_FORWARD) && defined(CONFIG_IPV6)
				if(entry->IpProtocol == IPVER_IPV6 && vcEntry.napt_v6)
				{
					// for 9607C, ingore limit speed if rate limit to 800M bps for IPv6 NPT flow 
					if(entry->direction == QOS_DIRECTION_DOWNSTREAM && entry->limitSpeed >= 800000)
						continue;
#ifdef CONFIG_RTK_SKB_MARK2
					if(use_mark2){
						mark2 = SOCK_MARK2_METER_HW_INDEX_SET(mark2, 0);
						mark2_mask |= SOCK_MARK2_METER_HW_INDEX_ENABLE_BIT_MASK;
						if(mark2 == 0)
							use_mark2 = 0;
					}
#endif
					mark = SOCK_MARK_METER_INDEX_ENABLE_SET(mark, 1);
					mark |= SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK;
				}
#endif

				//patch: true bandwidth will be a little greater than limit value, so I minish 7% of set limit value ahead.
				int ceil;
				//ceil = upLinkRate/100 * 93;
				ceil = upLinkRate*93/100;

				childRate = (10>ceil)?ceil:10;
				//childRate = ceil;

				DOCMDARGVS(TC, DOWAIT, "class %s dev %s parent 1:1 classid 1:%d0 htb rate %dkbit ceil %dkbit mpu 64 overhead 4",
					tc_act, ifname, entry->entryid, childRate, ceil);

				DOCMDARGVS(TC, DOWAIT, "qdisc %s dev %s parent 1:%d0 handle %d1: pfifo limit 100",
					tc_act, ifname, entry->entryid, entry->entryid);

				//DOCMDARGVS(TC, DOWAIT, "filter %s dev %s parent 1: protocol ip prio 0 handle 0x%x fw flowid 1:%d0",
				//	tc_act, ifname, mark, entry->entryid);
				DOCMDARGVS(TC, DOWAIT, "filter %s dev %s parent 1: %s prio 0 handle 0x%x/0x%x fw flowid 1:%d0",
					tc_act, ifname, tc_protocol, mark, mark_mask, entry->entryid);

				if(entry->direction == QOS_DIRECTION_DOWNSTREAM)
					sprintf(traffic_chain, CAR_DS_TRAFFIC_CHAIN);
				else
					sprintf(traffic_chain, CAR_US_TRAFFIC_CHAIN);

				if (iptables_cmd)
					DOCMDARGVS(iptables_cmd, DOWAIT,  "-t mangle %s %s %s %s %s %s %s %s -j MARK --set-mark 0x%x/0x%x",
						fw_act, traffic_chain, wanPort, proto1[0], saddr, daddr, sport, dport, mark, mark_mask);
				if (ip6tables_cmd)
					DOCMDARGVS(ip6tables_cmd, DOWAIT,  "-t mangle %s %s %s %s %s %s %s %s -j MARK --set-mark 0x%x/0x%x",
						fw_act, traffic_chain, wanPort, proto1[1], saddr6, daddr6, sport, dport, mark, mark_mask);
#ifdef CONFIG_RTK_SKB_MARK2
				if(use_mark2)
				{
					if (iptables_cmd)
						DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle %s %s %s %s %s %s %s %s -j MARK2 --set-mark2 0x%llx/0x%llx",
							fw_act, traffic_chain, wanPort, proto1[0], saddr, daddr, sport, dport, mark2, mark2_mask);
					if (ip6tables_cmd)
						DOCMDARGVS(ip6tables_cmd, DOWAIT, "-t mangle %s %s %s %s %s %s %s %s -j MARK2 --set-mark2 0x%llx/0x%llx",
							fw_act, traffic_chain, wanPort, proto1[1], saddr6, daddr6, sport, dport, mark2, mark2_mask);
				}
#endif
				/*TCP/UDP?*/
				if(proto2)//setup the other protocol
				{
					if (iptables_cmd)
						DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle %s %s %s %s %s %s %s %s -j MARK --set-mark 0x%x/0x%x",
							fw_act, traffic_chain, wanPort, proto2[0], saddr, daddr, sport, dport, mark, mark_mask);
					if (ip6tables_cmd)
						DOCMDARGVS(ip6tables_cmd, DOWAIT, "-t mangle %s %s %s %s %s %s %s %s -j MARK --set-mark 0x%x/0x%x",
							fw_act, traffic_chain, wanPort, proto2[1], saddr6, daddr6, sport, dport, mark, mark_mask);
#ifdef CONFIG_RTK_SKB_MARK2
					if(use_mark2)
					{
						if (iptables_cmd)
							DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle %s %s %s %s %s %s %s %s -j MARK2 --set-mark2 0x%llx/0x%llx",
								fw_act, traffic_chain, wanPort, proto2[0], saddr, daddr, sport, dport, mark2, mark2_mask);
						if (ip6tables_cmd)
							DOCMDARGVS(ip6tables_cmd, DOWAIT, "-t mangle %s %s %s %s %s %s %s %s -j MARK2 --set-mark2 0x%llx/0x%llx",
								fw_act, traffic_chain, wanPort, proto2[1], saddr6, daddr6, sport, dport, mark2, mark2_mask);
					}
#endif
				}
			}
#if 0
			else
			{//if uprate=0, forbid traffic matching the rules
				if(entry->direction == QOS_DIRECTION_DOWNSTREAM)
					sprintf(filter_chain, CAR_DS_FILTER_CHAIN);
				else
					sprintf(filter_chain, CAR_US_FILTER_CHAIN);

				if (iptables_cmd)
					DOCMDARGVS(iptables_cmd, DOWAIT, "-t filter %s %s %s %s %s %s %s %s -j DROP",
						fw_act, filter_chain, wanPort, proto1[0], saddr, daddr, sport, dport);
				if (ip6tables_cmd)
					DOCMDARGVS(ip6tables_cmd, DOWAIT, "-t filter %s %s %s %s %s %s %s %s -j DROP",
						fw_act, filter_chain, wanPort, proto1[1], saddr6, daddr6, sport, dport);

				/*TCP/UDP again*/
				if(proto2)
				{
					if (iptables_cmd)
						DOCMDARGVS(iptables_cmd, DOWAIT, "-t filter %s %s %s %s %s %s %s %s -j DROP",
							fw_act, filter_chain, wanPort, proto2[0], saddr, daddr, sport, dport);
					if (ip6tables_cmd)
						DOCMDARGVS(ip6tables_cmd, DOWAIT, "-t filter %s %s %s %s %s %s %s %s -j DROP",
							fw_act, filter_chain, wanPort, proto2[1], saddr6, daddr6, sport, dport);
				}
			}
#endif
		}

		if(entry->ifIndex != DUMMY_IFINDEX && entry->ifIndex)
		{
			break;
		}
	}

	return 0;
}

static int setupCarRule()
{
	int entry_num, i;
	MIB_CE_IP_TC_T  entry;

	entry_num = mib_chain_total(MIB_IP_QOS_TC_TBL);

	for(i=0; i<entry_num; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_TC_TBL, i, (void*)&entry))
			continue;
		
		if (setupCarRule_one(&entry)) {
			continue;
		}
	}

	return 0;
}

#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
#define MAX_EBT_IPT_COMMAND_NUMBER	10
#else
#define MAX_EBT_IPT_COMMAND_NUMBER	(5*MAX_VC_NUM)
#endif
#define CUR_COMMAND_PTR(cmd)	(cmd+strlen(cmd))
#define QOS_ADD_EBT_ARG(idx, f_, ...)	snprintf(CUR_COMMAND_PTR(classfRule[idx].ebtCommand), sizeof(classfRule[idx].ebtCommand)-strlen(classfRule[idx].ebtCommand), (f_), ##__VA_ARGS__)	
#define QOS_ADD_IPT_ARG(idx, f_, ...)	snprintf(CUR_COMMAND_PTR(classfRule[idx].iptCommand), sizeof(classfRule[idx].iptCommand)-strlen(classfRule[idx].iptCommand), (f_), ##__VA_ARGS__)	

static int rtk_qos_classf_rule_upstream_sock_mark_get(MIB_CE_IP_QOS_T *entry_p, int entryIndex)
{
	unsigned char enableQos1p=2;
	unsigned int mark=0;
	unsigned short qid=0;

#if defined(_PRMT_X_CT_COM_QOS_)
	mib_get_s(MIB_QOS_ENABLE_1P, (void *)&enableQos1p, sizeof(enableQos1p));
#endif

	if(entry_p->prior)
	{
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE) || defined(CONFIG_FIBER_FEATURE) || defined(CONFIG_USER_AP_QOS)
		qid = (getIPQosQueueNumber()-entry_p->prior);
#elif defined(CONFIG_RTL_ADV_FAST_PATH) || defined(SUPPORT_BYPASS_RG_FWDENGINE)
		qid = entry_p->prior;
#endif
		mark |= SOCK_MARK_QOS_SWQID_TO_MARK(qid);

	}

	if(entry_p->m_1p
#if defined(CONFIG_USER_AP_E8B_QOS)
			&& (enableQos1p == 2)
#endif
	)
	{
		mark |= SOCK_MARK_8021P_TO_MARK(entry_p->m_1p-1);
		mark |= SOCK_MARK_8021P_ENABLE_TO_MARK(1);
	}
	mark |= SOCK_MARK_QOS_ENTRY_TO_MARK(entryIndex+1);

	return mark;
}

static int rtk_qos_classf_rules_upstream_ebt_profile_get(int mibIndex)
{
	MIB_CE_IP_QOS_T thisEntry;
	MIB_CE_IP_QOS_T entry;
	int entryNum = 0;
	int i;

	if(!mib_chain_get(MIB_IP_QOS_TBL, mibIndex, (void*)&thisEntry) || !thisEntry.enable)
		return 0;
	
	entryNum = mib_chain_total(MIB_IP_QOS_TBL);
	for(i=0 ; i<entryNum ; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)&entry) || !entry.enable)
			continue;
		
		if(entry.direction == QOS_DIRECTION_UPSTREAM)
		{
			if(i == mibIndex)
				continue;

			if(entry.vlan1p != thisEntry.vlan1p)
				continue;
#ifdef CONFIG_CMCC_ENTERPRISE
			if(entry.vid != thisEntry.vid)
				continue;
#endif
			if(entry.phyPort != thisEntry.phyPort 
				|| entry.phyPort_end != thisEntry.phyPort_end)
				continue;
			
			return (mibIndex<i) ? (mibIndex+1) : (i+1);
		}
	}

	return (mibIndex+1);
}

static int rtk_qos_classf_rules_upstream_ebt_phyPortProfile_get(int mibIndex, unsigned char phyPort)
{
	MIB_CE_IP_QOS_T thisEntry;
	MIB_CE_IP_QOS_T entry;
	int entryNum = 0;
	int i;

	if(!mib_chain_get(MIB_IP_QOS_TBL, mibIndex, (void*)&thisEntry) || !thisEntry.enable)
		return 0;
	
	entryNum = mib_chain_total(MIB_IP_QOS_TBL);
	for(i=0 ; i<entryNum ; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)&entry) || !entry.enable)
			continue;
		
		if(entry.direction == QOS_DIRECTION_UPSTREAM)
		{
			if(i == mibIndex)
				continue;

			if(entry.vlan1p != thisEntry.vlan1p)
				continue;
#ifdef CONFIG_CMCC_ENTERPRISE
			if(entry.vid != thisEntry.vid)
				continue;
#endif
			if(entry.phyPort && entry.phyPort_end && (entry.phyPort<entry.phyPort_end))
			{
				if(entry.phyPort > phyPort
					|| entry.phyPort_end < phyPort)
					continue;
			}
			else if(entry.phyPort)
			{
				if(entry.phyPort != phyPort)
					continue;
			}
			else if(phyPort)
				continue;
			
			return (mibIndex<i) ? (mibIndex+1) : (i+1);
		}
	}

	return (mibIndex+1);
}

static int rtk_qos_classf_rules_upstream_setup(unsigned char policy)
{
	struct rtk_qos_classf_rule_entry	classfRule[MAX_EBT_IPT_COMMAND_NUMBER] = {0};
	unsigned int mark=0, iptableMark=0, ebtableMark=0;
	char tmpstr[48]={0}, tmpstr_end[48]={0};
	unsigned char curCmdNum=0;
	unsigned char tempCurCmdNum=0;
	unsigned char ebtChainName[128]={0};
	unsigned char iptChainName[128]={0};
	unsigned char iptTargetName[128]={0};
	unsigned int useIptable=0;
	unsigned int useEbtable=0;
	unsigned int isOutifBridgeWAN=0;
	MIB_CE_IP_QOS_T entry;
	MIB_CE_ATM_VC_T vcEntry;
	int i, j, EntryNum = 0, vcEntryNum = 0;
	int order, qosEntryMask = 0;
	unsigned char enableDscpMark=1;
	unsigned char enableQos1p=2;
	unsigned char ipProtocol;
	char f_dscp[24] = {0};
#ifdef QOS_SUPPORT_RTP
	FILE *fp;
	char buff[100];
	unsigned int rtp_dip;
	unsigned int rtp_dpt;
	unsigned int rtpCnt=0, rtpIndex=0, rtpRuleCnt=0;
	struct rtp_struct {
		unsigned int dip;
		unsigned int dport;
	} __PACK__;
	struct rtp_rule_st {
		char daddr[64];
		char dport[48];
	} __PACK__;

	struct rtp_struct rtp_entry[RTP_RULE_NUM_MAX];
	struct rtp_rule_st rtp_rule_entry[RTP_RULE_NUM_MAX];
#endif
	unsigned int wan_qos_state = 0;
	int lanIntfIndex;
	char duid_mac[20]={0};
	char ifname[IFNAMSIZ];
	char *tc_protocol=NULL;
	int index;
#ifdef QOS_SETUP_IFB
	char *devname="ifb0";
#endif
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	int eth_type_mark = 0;
	unsigned char is_bridge_mode = 0;
	unsigned int is_bridge_wan = 0;
#endif
	int logical_wan_port = 0;

	DOCMDINIT

	QOS_SETUP_PRINT_FUNCTION
	
	//get current RTP stream info
#ifdef QOS_SUPPORT_RTP
	if (!(fp=fopen(RTP_PORT_FILE, "r"))) {
		printf("no RTP connection!\n");
	} else {
		while ( fgets(buff, sizeof(buff), fp) != NULL ) {
			if(sscanf(buff, "ip=%x port=%u", &rtp_dip, &rtp_dpt)!=2) {
				printf("Unsuported rtp format\n");
				break;
			}
			else {
				rtpCnt++;
				rtp_entry[rtpCnt-1].dip = rtp_dip;
				rtp_entry[rtpCnt-1].dport = rtp_dpt;
			}
		}
		fclose(fp);
	}
#endif
#if defined(_PRMT_X_CT_COM_QOS_)
	mib_get_s(MIB_QOS_ENABLE_DSCP_MARK, (void *)&enableDscpMark, sizeof(enableDscpMark));
	mib_get_s(MIB_QOS_ENABLE_1P, (void *)&enableQos1p, sizeof(enableQos1p));
#endif
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(current_qos_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
	logical_wan_port = current_qos_wan_port;
	if(!(devname=(char*)getQosRootWanName(logical_wan_port)))
		return 0;
	if(getOpMode()==BRIDGE_MODE)
		is_bridge_mode = 1;
#endif
	EntryNum = mib_chain_total(MIB_IP_QOS_TBL);
	printf("%s:%d EntryNum=%d\n",__FUNCTION__,__LINE__,EntryNum);
	for(order=0 ; order<(EntryNum+1) ; order++)
	{
		for(i=0 ; i<EntryNum ; i++)
		{
			useIptable=0;
			useEbtable=0;
			ipProtocol=0;
			curCmdNum=0;
			ebtableMark=0;
			iptableMark=0;
			mark=0;
			memset(classfRule, 0, sizeof(classfRule));
			memset(&entry, 0, sizeof(entry));

			if(!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)&entry)||!entry.enable)
				continue;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
			if(entry.logic_wan_port != logical_wan_port)
				continue;
#endif

#ifdef CONFIG_IPV6
			if(entry.IpProtocol&IPVER_IPV4_IPV6)
			{
				if(entry.IpProtocol&IPVER_IPV4)
						ipProtocol |= IPVER_IPV4;

				if(entry.IpProtocol&IPVER_IPV6)
						ipProtocol |= IPVER_IPV6;
			}
			else if(entry.ipqos_rule_type==IPQOS_MAC_BASE)
			{
				ipProtocol = IPVER_IPV4_IPV6;
			}
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
			else if(entry.ipqos_rule_type==IPQOS_PORT_BASE)
			{// default is 0 in Retailor, same as MAC above
				ipProtocol = IPVER_IPV4_IPV6;
			}
#endif
			else
				ipProtocol = entry.IpProtocol;
#else
			ipProtocol = IPVER_IPV4;
#endif
			if((order == entry.QoSOrder) &&
				!(qosEntryMask&(1<<i)) &&
				entry.direction == QOS_DIRECTION_UPSTREAM && 
				isQueneEnableForRule(entry.prior))
			{
				qosEntryMask |= (1<<i);

				if(entry.classtype == QOS_CLASSTYPE_NORMAL)
				{
					if(entry.ipqos_rule_type==IPQOS_TYPE_AUTO ||
							entry.ipqos_rule_type==IPQOS_IP_PROTO_BASE ||
							entry.ipqos_rule_type==IPQOS_PORT_BASE ||
							entry.ipqos_rule_type==IPQOS_MAC_BASE)
					{
#ifdef CONFIG_QOS_SUPPORT_PROTOCOL_LIST
						//protocol
						if(entry.protoType > PROTO_MAX) //invalid protocol index
							return 1;

						for (j=0 ; (1<<j)&PROTO_MAX ; j++)
						{
							if (!(entry.protoType&(1<<j)))
								continue;

							useIptable = 1;
							if (((1<<j)&(PROTO_TCP|PROTO_UDP|PROTO_ICMP)))
							{

								if(ipProtocol&IPVER_IPV4)
								{
#ifdef CONFIG_CMCC_ENTERPRISE
									if(entry.vlan1p || entry.vid)
#else
									if(entry.vlan1p)
#endif
									{
										useEbtable = 1;
										QOS_ADD_EBT_ARG(curCmdNum, "-p 0x8100"); //802.1q
									}
									else
										QOS_ADD_EBT_ARG(curCmdNum, "-p 0x0800"); //IPv4

									if ((1<<j) == PROTO_TCP)
										QOS_ADD_IPT_ARG(curCmdNum, proto2str[1]); //TCP
									else if ((1<<j) == PROTO_UDP)
										QOS_ADD_IPT_ARG(curCmdNum, proto2str[2]); //UDP
									else if ((1<<j) == PROTO_ICMP)
										QOS_ADD_IPT_ARG(curCmdNum, proto2str[3]); //ICMP
									classfRule[curCmdNum].ipVersion = IPVER_IPV4;
									classfRule[curCmdNum].protocol = (1<<j);
									curCmdNum++;
								}
#ifdef CONFIG_IPV6
								if(ipProtocol&IPVER_IPV6)
								{
#ifdef CONFIG_CMCC_ENTERPRISE
									if(entry.vlan1p || entry.vid)
#else
									if(entry.vlan1p)
#endif
									{
										useEbtable = 1;
										QOS_ADD_EBT_ARG(curCmdNum, "-p 0x8100"); //802.1q
									}
									else
										QOS_ADD_EBT_ARG(curCmdNum, "-p 0x86dd"); //IPv4

									if ((1<<j) == PROTO_TCP)
										QOS_ADD_IPT_ARG(curCmdNum, proto2str_v6[1]); //TCP
									else if ((1<<j) == PROTO_UDP)
										QOS_ADD_IPT_ARG(curCmdNum, proto2str_v6[2]); //UDP
									else if ((1<<j) == PROTO_ICMP)
										QOS_ADD_IPT_ARG(curCmdNum, proto2str_v6[3]); //ICMPV6
									classfRule[curCmdNum].ipVersion = IPVER_IPV6;
									classfRule[curCmdNum].protocol = (1<<j);
									curCmdNum++;
								}
#endif
							}
							else if (((1<<j)&PROTO_RTP))
							{
#ifdef QOS_SUPPORT_RTP
								for(rtpIndex=0 ; rtpIndex<rtpCnt ; rtpIndex++)
								{
									if(ipProtocol&IPVER_IPV4)
									{
										QOS_ADD_IPT_ARG(curCmdNum, "%s --sport %d", proto2str[2], rtp_entry[rtpIndex].dport); // UDP
										classfRule[curCmdNum].ipVersion = IPVER_IPV4;
										classfRule[curCmdNum].protocol = PROTO_RTP;
										curCmdNum++;
									}
	
#ifdef CONFIG_IPV6
									if(ipProtocol&IPVER_IPV6)
									{
										QOS_ADD_IPT_ARG(curCmdNum, "%s --sport %d", proto2str_v6[2], rtp_entry[rtpIndex].dport); // UDP
										classfRule[curCmdNum].ipVersion = IPVER_IPV6;
										classfRule[curCmdNum].protocol = PROTO_RTP;
										curCmdNum++;
									}
#endif
								}
#else
								if(ipProtocol&IPVER_IPV4)
								{
									QOS_ADD_IPT_ARG(curCmdNum, "-m helper --helper sip"); // RTP
									classfRule[curCmdNum].ipVersion = IPVER_IPV4;
									classfRule[curCmdNum].protocol = PROTO_RTP;
									curCmdNum++;
								}
#ifdef CONFIG_IPV6
								if(ipProtocol&IPVER_IPV6)
								{
									QOS_ADD_IPT_ARG(curCmdNum, "-m helper --helper sip"); // RTP
									classfRule[curCmdNum].ipVersion = IPVER_IPV6;
									classfRule[curCmdNum].protocol = PROTO_RTP;
									curCmdNum++;
								}
#endif
#endif
							}
						}

						if (!curCmdNum) // non protocol case
						{
							if(entry.protoType == PROTO_NONE)
								useIptable = 1;

							if(ipProtocol&IPVER_IPV4)
							{
								QOS_ADD_IPT_ARG(curCmdNum, (" %s", proto2str[entry.protoType]));
								classfRule[curCmdNum].ipVersion = IPVER_IPV4;
								classfRule[curCmdNum].protocol = entry.protoType;
								curCmdNum++;
							}

#ifdef CONFIG_IPV6
							if(ipProtocol&IPVER_IPV6)
							{
								QOS_ADD_IPT_ARG(curCmdNum, (" %s", proto2str_v6[entry.protoType]));
								classfRule[curCmdNum].ipVersion = IPVER_IPV6;
								classfRule[curCmdNum].protocol = entry.protoType;
								curCmdNum++;
							}
#endif
						}
#else
						//protocol
						if(entry.protoType >= PROTO_MAX) //invalid protocol index
							return 1;
						if(entry.protoType == PROTO_UDPTCP)
						{
#ifdef CONFIG_CMCC_ENTERPRISE
							if(entry.vlan1p || entry.vid )
#else
							if(entry.vlan1p)
#endif
							{
								useEbtable = 1;
								useIptable = 1;
								if(ipProtocol&IPVER_IPV4)
								{
									QOS_ADD_EBT_ARG(curCmdNum, "-p 0x8100"); // 802.1q
									QOS_ADD_IPT_ARG(curCmdNum, proto2str[1]); // TCP
									classfRule[curCmdNum].ipVersion = IPVER_IPV4;
									classfRule[curCmdNum].protocol = PROTO_TCP;
									curCmdNum++;
									QOS_ADD_EBT_ARG(curCmdNum, "-p 0x8100"); // 802.1q
									QOS_ADD_IPT_ARG(curCmdNum, proto2str[2]); // UDP
									classfRule[curCmdNum].ipVersion = IPVER_IPV4;
									classfRule[curCmdNum].protocol = PROTO_UDP;
									curCmdNum++;
								}
#ifdef CONFIG_IPV6
								if(ipProtocol&IPVER_IPV6)
								{
									QOS_ADD_EBT_ARG(curCmdNum, "-p 0x8100"); // TCP
									QOS_ADD_IPT_ARG(curCmdNum, proto2str_v6[1]); // TCP
									classfRule[curCmdNum].ipVersion = IPVER_IPV6;
									classfRule[curCmdNum].protocol = PROTO_TCP;
									curCmdNum++;
									QOS_ADD_EBT_ARG(curCmdNum, "-p 0x8100"); // UDP
									QOS_ADD_IPT_ARG(curCmdNum, proto2str_v6[2]); // UDP
									classfRule[curCmdNum].ipVersion = IPVER_IPV6;
									classfRule[curCmdNum].protocol = PROTO_UDP;
									curCmdNum++;
								}
#endif
							}
							else
							{
								useIptable = 1;
								if(ipProtocol&IPVER_IPV4)
								{
									QOS_ADD_EBT_ARG(curCmdNum, "-p 0x0800 %s", proto2str2layer[1]); // TCP
									QOS_ADD_IPT_ARG(curCmdNum, proto2str[1]); // TCP
									classfRule[curCmdNum].ipVersion = IPVER_IPV4;
									classfRule[curCmdNum].protocol = PROTO_TCP;
									curCmdNum++;
									QOS_ADD_EBT_ARG(curCmdNum, "-p 0x0800 %s", proto2str2layer[2]); // UDP
									QOS_ADD_IPT_ARG(curCmdNum, proto2str[2]); // UDP
									classfRule[curCmdNum].ipVersion = IPVER_IPV4;
									classfRule[curCmdNum].protocol = PROTO_UDP;
									curCmdNum++;
								}
#ifdef CONFIG_IPV6
								if(ipProtocol&IPVER_IPV6)
								{
									QOS_ADD_EBT_ARG(curCmdNum, "-p 0x86dd %s", proto2str2layer_v6[1]); // TCP
									QOS_ADD_IPT_ARG(curCmdNum, proto2str_v6[1]); // TCP
									classfRule[curCmdNum].ipVersion = IPVER_IPV6;
									classfRule[curCmdNum].protocol = PROTO_TCP;
									curCmdNum++;
									QOS_ADD_EBT_ARG(curCmdNum, "-p 0x86dd %s", proto2str2layer_v6[2]); // UDP
									QOS_ADD_IPT_ARG(curCmdNum, proto2str_v6[2]); // UDP
									classfRule[curCmdNum].ipVersion = IPVER_IPV6;
									classfRule[curCmdNum].protocol = PROTO_UDP;
									curCmdNum++;
								}
#endif
							}
						}
						else if(entry.protoType == PROTO_RTP)
						{
							useIptable = 1;
#ifdef QOS_SUPPORT_RTP
							for(rtpIndex=0 ; rtpIndex<rtpCnt ; rtpIndex++)
							{
								if(ipProtocol&IPVER_IPV4)
								{
									QOS_ADD_IPT_ARG(curCmdNum, "%s --sport %d", proto2str[2], rtp_entry[rtpIndex].dport); // UDP
									classfRule[curCmdNum].ipVersion = IPVER_IPV4;
									classfRule[curCmdNum].protocol = PROTO_RTP;
									curCmdNum++;
								}

#ifdef CONFIG_IPV6
								if(ipProtocol&IPVER_IPV6)
								{
									QOS_ADD_IPT_ARG(curCmdNum, "%s --sport %d", proto2str_v6[2], rtp_entry[rtpIndex].dport); // UDP
									classfRule[curCmdNum].ipVersion = IPVER_IPV6;
									classfRule[curCmdNum].protocol = PROTO_RTP;
									curCmdNum++;
								}
#endif
							}
#else
							if(ipProtocol&IPVER_IPV4)
							{
								QOS_ADD_IPT_ARG(curCmdNum, "-m helper --helper sip"); // RTP
								classfRule[curCmdNum].ipVersion = IPVER_IPV4;
								classfRule[curCmdNum].protocol = PROTO_RTP;
								curCmdNum++;
							}
#ifdef CONFIG_IPV6
							if(ipProtocol&IPVER_IPV6)
							{
								QOS_ADD_IPT_ARG(curCmdNum, "-m helper --helper sip"); // RTP
								classfRule[curCmdNum].ipVersion = IPVER_IPV6;
								classfRule[curCmdNum].protocol = PROTO_RTP;
								curCmdNum++;
							}
#endif
#endif
						}
						else
						{
							if(entry.protoType != PROTO_NONE)
								useIptable = 1;

#ifdef CONFIG_CMCC_ENTERPRISE
							if(entry.vlan1p || entry.vid)
#else
							if(entry.vlan1p)
#endif
							{
								useEbtable = 1;
								if(ipProtocol&IPVER_IPV4)
								{
									QOS_ADD_EBT_ARG(curCmdNum, "-p 0x8100");
									QOS_ADD_IPT_ARG(curCmdNum, proto2str[entry.protoType]);
									classfRule[curCmdNum].ipVersion = IPVER_IPV4;
									classfRule[curCmdNum].protocol = entry.protoType;
									curCmdNum++;
								}

#ifdef CONFIG_IPV6
								if(ipProtocol&IPVER_IPV6)
								{
									QOS_ADD_EBT_ARG(curCmdNum, "-p 0x8100");
									if(entry.protoType < sizeof(proto2str_v6)/sizeof(char*))
										QOS_ADD_IPT_ARG(curCmdNum, proto2str_v6[entry.protoType]);
									classfRule[curCmdNum].ipVersion = IPVER_IPV6;
									classfRule[curCmdNum].protocol = entry.protoType;
									curCmdNum++;
								}
#endif
							}
							else
							{
								if (entry.protoType == PROTO_NONE) useIptable=1;
								if(ipProtocol&IPVER_IPV4)
								{
									QOS_ADD_EBT_ARG(curCmdNum, "-p 0x0800 %s", proto2str2layer[entry.protoType]);
									QOS_ADD_IPT_ARG(curCmdNum, proto2str[entry.protoType]);
									classfRule[curCmdNum].ipVersion = IPVER_IPV4;
									classfRule[curCmdNum].protocol = entry.protoType;
									curCmdNum++;
								}

#ifdef CONFIG_IPV6
								if(ipProtocol&IPVER_IPV6)
								{
									QOS_ADD_EBT_ARG(curCmdNum, "-p 0x86dd %s", proto2str2layer_v6[entry.protoType]);
									if(entry.protoType < sizeof(proto2str_v6)/sizeof(char*))
										QOS_ADD_IPT_ARG(curCmdNum, proto2str_v6[entry.protoType]);
									classfRule[curCmdNum].ipVersion = IPVER_IPV6;
									classfRule[curCmdNum].protocol = entry.protoType;
									curCmdNum++;
								}
#endif
							}
						}
#endif

						//LAN interface
						tempCurCmdNum = curCmdNum;
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
						if(curCmdNum)
						{
							if(entry.phyPort)
							{
								useEbtable = 1;
								if(entry.phyPort_end && (entry.phyPort<entry.phyPort_end))
								{
									for(lanIntfIndex=1 ; lanIntfIndex<=LAN_INTF_NUM ; lanIntfIndex++)
									{
#ifndef CONFIG_USER_AP_QOS
										if(strlen(lan_itf_map.map[lanIntfIndex].name) && (entry.phyPort <= lanIntfIndex) && (lanIntfIndex < entry.phyPort_end))
#else
										if(strlen(lan_itf_map_vid.map[lanIntfIndex].name) && (entry.phyPort <= lanIntfIndex) && (lanIntfIndex < entry.phyPort_end))
#endif
										{
											for(index=0 ; index<tempCurCmdNum ; index++)
											{
#ifndef CONFIG_USER_AP_QOS
												if(curCmdNum < MAX_EBT_IPT_COMMAND_NUMBER)
													QOS_ADD_EBT_ARG(curCmdNum, "%s -i %s", classfRule[index].ebtCommand, lan_itf_map.map[lanIntfIndex].name);
#else
												if(curCmdNum < MAX_EBT_IPT_COMMAND_NUMBER)
													QOS_ADD_EBT_ARG(curCmdNum, "%s -i %s+", classfRule[index].ebtCommand, lan_itf_map_vid.map[lanIntfIndex].name);
#endif
												if(curCmdNum < MAX_EBT_IPT_COMMAND_NUMBER)
												{
													sprintf(classfRule[curCmdNum].iptCommand, "%s", classfRule[index].iptCommand);
													classfRule[curCmdNum].ipVersion = classfRule[index].ipVersion;
													classfRule[curCmdNum].protocol = classfRule[index].protocol;
													classfRule[curCmdNum].profile = rtk_qos_classf_rules_upstream_ebt_phyPortProfile_get(i, lanIntfIndex);
												}
												curCmdNum++;
											}
										}
									}

									for(index=0 ; index<tempCurCmdNum ; index++)
									{
#ifndef CONFIG_USER_AP_QOS
										QOS_ADD_EBT_ARG(index, " -i %s", lan_itf_map.map[entry.phyPort_end].name);
#else
										QOS_ADD_EBT_ARG(index, " -i %s+", lan_itf_map_vid.map[entry.phyPort_end].name);
#endif
										classfRule[index].profile = rtk_qos_classf_rules_upstream_ebt_phyPortProfile_get(i, entry.phyPort_end);
									}
								}
								else
								{
									for(index=0 ; index<curCmdNum ; index++)
									{
#ifndef CONFIG_USER_AP_QOS
										QOS_ADD_EBT_ARG(index, " -i %s", lan_itf_map.map[entry.phyPort].name);
#else
										QOS_ADD_EBT_ARG(index, " -i %s+", lan_itf_map_vid.map[entry.phyPort].name);
#endif
										classfRule[index].profile = rtk_qos_classf_rules_upstream_ebt_phyPortProfile_get(i, entry.phyPort);
									}
								}							
							}
						}
						else if(entry.phyPort)
						{
							useEbtable = 1;
							if(entry.phyPort_end && (entry.phyPort<entry.phyPort_end))
							{
								for(lanIntfIndex=1 ; lanIntfIndex<=LAN_INTF_NUM ; lanIntfIndex++)
								{
#ifndef CONFIG_USER_AP_QOS
									if(strlen(lan_itf_map.map[lanIntfIndex].name) && (entry.phyPort <= lanIntfIndex) && (lanIntfIndex <= entry.phyPort_end))
#else
									if(strlen(lan_itf_map_vid.map[lanIntfIndex].name) && (entry.phyPort <= lanIntfIndex) && (lanIntfIndex <= entry.phyPort_end))
#endif
									{
#ifndef CONFIG_USER_AP_QOS	
										QOS_ADD_EBT_ARG(curCmdNum, "-i %s", lan_itf_map.map[lanIntfIndex].name);
#else
										QOS_ADD_EBT_ARG(curCmdNum, "-i %s+", lan_itf_map_vid.map[lanIntfIndex].name);
#endif
										QOS_ADD_IPT_ARG(curCmdNum, proto2str[entry.protoType]);
										classfRule[curCmdNum].ipVersion = IPVER_IPV4;
										classfRule[curCmdNum].profile = rtk_qos_classf_rules_upstream_ebt_phyPortProfile_get(i, lanIntfIndex);
										curCmdNum++;
#ifdef CONFIG_IPV6
#ifndef CONFIG_USER_AP_QOS	
										QOS_ADD_EBT_ARG(curCmdNum, "-i %s", lan_itf_map.map[lanIntfIndex].name);
#else
										QOS_ADD_EBT_ARG(curCmdNum, "-i %s+", lan_itf_map_vid.map[lanIntfIndex].name);
#endif
										if(entry.protoType < sizeof(proto2str_v6)/sizeof(char*))
											QOS_ADD_IPT_ARG(curCmdNum, proto2str_v6[entry.protoType]);
										classfRule[curCmdNum].ipVersion = IPVER_IPV6;
										classfRule[curCmdNum].profile = rtk_qos_classf_rules_upstream_ebt_phyPortProfile_get(i, lanIntfIndex);
										curCmdNum++;
#endif
									}
								}
							}
							else
							{
#ifndef CONFIG_USER_AP_QOS	
								QOS_ADD_EBT_ARG(curCmdNum, "-i %s", lan_itf_map.map[entry.phyPort].name);
#else
								QOS_ADD_EBT_ARG(curCmdNum, "-i %s+", lan_itf_map_vid.map[entry.phyPort].name);
#endif
								QOS_ADD_IPT_ARG(curCmdNum, proto2str[entry.protoType]);
								classfRule[curCmdNum].ipVersion = IPVER_IPV4;
								classfRule[curCmdNum].profile = rtk_qos_classf_rules_upstream_ebt_phyPortProfile_get(i, entry.phyPort);
								curCmdNum++;
#ifdef CONFIG_IPV6
#ifndef CONFIG_USER_AP_QOS
								QOS_ADD_EBT_ARG(curCmdNum, "-i %s", lan_itf_map.map[entry.phyPort].name);
#else
								QOS_ADD_EBT_ARG(curCmdNum, "-i %s+", lan_itf_map_vid.map[entry.phyPort].name);
#endif
								QOS_ADD_IPT_ARG(curCmdNum, proto2str_v6[entry.protoType]);
								classfRule[curCmdNum].ipVersion = IPVER_IPV6;
								classfRule[curCmdNum].profile = rtk_qos_classf_rules_upstream_ebt_phyPortProfile_get(i, entry.phyPort);
								curCmdNum++;
#endif
							}						
						}
#else

						if(curCmdNum){// always exists for port, mac, ip
							useIptable = 1;
							if(!is_bridge_mode){
								if(entry.outif && entry.outif == DUMMY_IFINDEX){// ANY WAN
									//Set classfRule[curCmdNum] based on classfRule[index]
									for(j=0; j<MAX_VC_NUM; j++){
										if(!wan_port_itfs[logical_wan_port].itf[j].name[0] || !wan_port_itfs[logical_wan_port].itf[j].enable_qos)
											continue;
										if(wan_port_itfs[logical_wan_port].itf[j].cmode==CHANNEL_MODE_BRIDGE){//bridge wan
											for(index=0 ; index<tempCurCmdNum ; index++){
												QOS_ADD_IPT_ARG(curCmdNum, "%s -m outphysdev --physdev %s", classfRule[index].iptCommand, wan_port_itfs[logical_wan_port].itf[j].name);
												snprintf(classfRule[curCmdNum].ebtCommand,sizeof(classfRule[curCmdNum].ebtCommand), "%s", classfRule[index].ebtCommand);
												classfRule[curCmdNum].ipVersion = classfRule[index].ipVersion;
												classfRule[curCmdNum].cmd_with_wan = 1;
												curCmdNum++;
											}
										}else{//ipoe wan...
											for(index=0 ; index<tempCurCmdNum ; index++){
												QOS_ADD_IPT_ARG(curCmdNum, "%s -o %s", classfRule[index].iptCommand, wan_port_itfs[logical_wan_port].itf[j].name);
												snprintf(classfRule[curCmdNum].ebtCommand,sizeof(classfRule[curCmdNum].ebtCommand), "%s", classfRule[index].ebtCommand);
												classfRule[curCmdNum].ipVersion = classfRule[index].ipVersion;
												classfRule[curCmdNum].cmd_with_wan = 1;
												curCmdNum++;
											}
										}
									}
								}else if(entry.outif && entry.outif != DUMMY_IFINDEX){// Selected WAN
									ifGetName(entry.outif, ifname, sizeof(ifname));
									wan_qos_state = get_pvc_qos_state(entry.outif);
									is_bridge_wan = (wan_qos_state&QOS_WAN_BRIDGE) ? 1:0;
									if(is_bridge_wan){
										for(index=0 ; index<tempCurCmdNum ; index++){
											QOS_ADD_IPT_ARG(index, " -m outphysdev --physdev %s", ifname);
											classfRule[index].cmd_with_wan = 1;
										}
									}else{
										for(index=0 ; index<tempCurCmdNum ; index++){
											QOS_ADD_IPT_ARG(index, " -o %s", ifname);
											classfRule[index].cmd_with_wan = 1;
										}								
									}
								}
								
							}
						}
#endif
						///-- common parameter as following
						for(index=0 ; index<curCmdNum ; index++)
						{
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
							//WAN interface
							if(entry.outif && entry.outif != DUMMY_IFINDEX) 
							{
								wan_qos_state = get_pvc_qos_state(entry.outif);
								isOutifBridgeWAN = (wan_qos_state&QOS_WAN_BRIDGE) ? 1:0;
								
								ifGetName(entry.outif, ifname, sizeof(ifname));
								if(isOutifBridgeWAN)
								{
									useEbtable = 1;
									QOS_ADD_EBT_ARG(index, " -o %s", ifname);
								}
								else
								{
									useIptable = 1;
									QOS_ADD_IPT_ARG(index, " -o %s", ifname);								
								}
							}
#endif
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
							if(!classfRule[index].cmd_with_wan && !is_bridge_mode)
								continue;
							// in Retailor, set entry.phyPort here
							if(entry.phyPort){
								useIptable = 1;
								QOS_ADD_IPT_ARG(index," -m physdev --physdev-in %s+ ", lan_itf_map_vid.map[entry.phyPort].name);
							}							
#endif 

							if(memcmp(entry.smac, EMPTY_MAC, MAC_ADDR_LEN)) 
							{
								mac_hex2string(entry.smac, tmpstr);
								if(isOutifBridgeWAN)
								{
									QOS_ADD_EBT_ARG(index, " -s %s", tmpstr);								
								}
								else
								{
									useIptable = 1;
									if(memcmp(entry.smac_end, EMPTY_MAC, MAC_ADDR_LEN) && memcmp(entry.smac, entry.smac_end, MAC_ADDR_LEN))
									{
										mac_hex2string(entry.smac_end, tmpstr_end);
										QOS_ADD_IPT_ARG(index, " -m macrange --mac-source-range %s-%s", tmpstr, tmpstr_end);
									}
									else
									{
										QOS_ADD_IPT_ARG(index, " -m mac --mac-source %s", tmpstr);
									}								
								}
							}

							if (memcmp(entry.dmac, EMPTY_MAC, MAC_ADDR_LEN))
							{
								mac_hex2string(entry.dmac, tmpstr);
								if(isOutifBridgeWAN)
								{
									QOS_ADD_EBT_ARG(index, " -d %s", tmpstr);
								}
								else
								{
									useIptable = 1;
									if(memcmp(entry.dmac_end, EMPTY_MAC, MAC_ADDR_LEN) && memcmp(entry.dmac, entry.dmac_end, MAC_ADDR_LEN))
									{
										mac_hex2string(entry.dmac_end, tmpstr_end);
										QOS_ADD_IPT_ARG(index, " -m macrange --mac-dest-range %s-%s", tmpstr, tmpstr_end);
										
									}
									else
									{
										QOS_ADD_IPT_ARG(index, " -m mac --mac-dst %s", tmpstr);
									}								
								}
							}
							//dscp match
							if(entry.qosDscp)
							{
								if(isOutifBridgeWAN)
								{
									snprintf(f_dscp, sizeof(f_dscp), "0x%x", (entry.qosDscp-1)&0xFF);
									// This is a IPv4 rule
									if ( classfRule[index].ipVersion == IPVER_IPV4 )
										QOS_ADD_EBT_ARG(index, " --ip-tos %s", f_dscp);
									// This is a IPv6 rule
									else if ( classfRule[index].ipVersion == IPVER_IPV6 )
										QOS_ADD_EBT_ARG(index, " --ip6-tclass %s", f_dscp);
								}
								else
								{
									useIptable = 1;
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_AP_E8B_QOS)
									if(entry.qosDscp_end){
										if ( classfRule[index].ipVersion == IPVER_IPV4 )
											QOS_ADD_IPT_ARG(index, " -m dscprange --dscp-range 0x%x:0x%x", entry.qosDscp, entry.qosDscp_end);
										else if ( classfRule[index].ipVersion == IPVER_IPV6 )
											QOS_ADD_IPT_ARG(index, " -m tosrange --tos-range 0x%x:0x%x", entry.qosDscp, entry.qosDscp_end);
									}
									else{
										if ( classfRule[index].ipVersion == IPVER_IPV4 )
											QOS_ADD_IPT_ARG(index, " -m dscp --dscp 0x%x", entry.qosDscp);
										else if ( classfRule[index].ipVersion == IPVER_IPV6 )
											QOS_ADD_IPT_ARG(index, " -m tos --tos 0x%x", entry.qosDscp);
									}
#else
									if(entry.qosDscp_end)
										QOS_ADD_IPT_ARG(index, " -m dscprange --dscp-range 0x%x:0x%x", (entry.qosDscp-1)>>2, (entry.qosDscp_end-1)>>2);
									else
										QOS_ADD_IPT_ARG(index, " -m dscp --dscp 0x%x", (entry.qosDscp-1)>>2);
#endif
								}
							}

							//Tos match , tos & DSCP value can't set together, using the same structure
							if(entry.tos)
							{
								snprintf(f_dscp, sizeof(f_dscp), "0x%x", (entry.tos)&0xFF);
								if(isOutifBridgeWAN)
								{
									QOS_ADD_EBT_ARG(index, " --ip-tos %s", f_dscp);
								}
								else
								{
									useIptable = 1;
									if(entry.tos_end)
										QOS_ADD_IPT_ARG(index, " -m tosrange --tos-range 0x%x:0x%x", entry.tos, entry.tos_end);
									else
										QOS_ADD_IPT_ARG(index, " -m tos --tos 0x%x", entry.tos);
								}
							}

							if((PROTO_TCP == classfRule[index].protocol) || (PROTO_UDP == classfRule[index].protocol))
							{
								if(entry.sPort)
								{
									if(isOutifBridgeWAN)
									{
										if(entry.sPortRangeMax)
										{
											if(classfRule[index].ipVersion == IPVER_IPV4)
												QOS_ADD_EBT_ARG(index, " --ip-source-port %d:%d", MIN_PORT(entry.sPort, entry.sPortRangeMax), MAX_PORT(entry.sPort, entry.sPortRangeMax));
											else if (classfRule[index].ipVersion == IPVER_IPV6)
												QOS_ADD_EBT_ARG(index, " --ip6-source-port %d:%d", MIN_PORT(entry.sPort, entry.sPortRangeMax), MAX_PORT(entry.sPort, entry.sPortRangeMax));
										}
										else
										{
											if(classfRule[index].ipVersion == IPVER_IPV4)
												QOS_ADD_EBT_ARG(index, " --ip-source-port %d", entry.sPort);
											else if (classfRule[index].ipVersion == IPVER_IPV6)
												QOS_ADD_EBT_ARG(index, " --ip6-source-port %d", entry.sPort);
										}
									}
									else
									{
										useIptable = 1;
										if(entry.sPortRangeMax)
											QOS_ADD_IPT_ARG(index, " --sport %d:%d", MIN_PORT(entry.sPort, entry.sPortRangeMax), MAX_PORT(entry.sPort, entry.sPortRangeMax));
									    else
											QOS_ADD_IPT_ARG(index, " --sport %d", entry.sPort);
									}
								}

								if(entry.dPort)
								{
									if(isOutifBridgeWAN)
									{
										if(entry.dPortRangeMax)
										{
											if(classfRule[index].ipVersion == IPVER_IPV4)
												QOS_ADD_EBT_ARG(index, " --ip-destination-port %d:%d", MIN_PORT(entry.dPort, entry.dPortRangeMax), MAX_PORT(entry.dPort, entry.dPortRangeMax));
											else if (classfRule[index].ipVersion == IPVER_IPV6)
												QOS_ADD_EBT_ARG(index, " --ip6-destination-port %d:%d", MIN_PORT(entry.dPort, entry.dPortRangeMax), MAX_PORT(entry.dPort, entry.dPortRangeMax));
										}
										else
										{
											if(classfRule[index].ipVersion == IPVER_IPV4)
												QOS_ADD_EBT_ARG(index, " --ip-destination-port %d", entry.dPort);
											else if (classfRule[index].ipVersion == IPVER_IPV6)
												QOS_ADD_EBT_ARG(index, " --ip6-destination-port %d", entry.dPort);
										}
									}
									else
									{
										useIptable = 1;
										if(entry.dPortRangeMax)
											QOS_ADD_IPT_ARG(index, " --dport %d:%d", MIN_PORT(entry.dPort, entry.dPortRangeMax), MAX_PORT(entry.dPort, entry.dPortRangeMax));
									    else
											QOS_ADD_IPT_ARG(index, " --dport %d", entry.dPort);
									}
								}
							}

							if(classfRule[index].ipVersion == IPVER_IPV4)
							{
								snprintf(tmpstr, sizeof(tmpstr), "%s", inet_ntoa(*((struct in_addr *)entry.sip)));
								if(strcmp(tmpstr, ARG_0x4))
								{
									if(isOutifBridgeWAN)
									{
										if(entry.smaskbit)
											QOS_ADD_EBT_ARG(index, " --ip-source %s/%d", tmpstr, entry.smaskbit);
										else
											QOS_ADD_EBT_ARG(index, " --ip-source %s", tmpstr);
									}
									else
									{
										useIptable = 1;
										snprintf(tmpstr_end, sizeof(tmpstr_end), "%s", inet_ntoa(*((struct in_addr *)entry.sip_end)));
										if(strcmp(tmpstr_end, ARG_0x4))
										{
											QOS_ADD_IPT_ARG(index, " -m iprange --src-range %s-%s", tmpstr, tmpstr_end);
										}
										else
										{
											if(entry.smaskbit)
												QOS_ADD_IPT_ARG(index, " -s %s/%d", tmpstr, entry.smaskbit);
											else
												QOS_ADD_IPT_ARG(index, " -s %s", tmpstr);
										}
									}
								}
							}
#ifdef CONFIG_IPV6
							else if (classfRule[index].ipVersion == IPVER_IPV6)
							{
								inet_ntop(PF_INET6, (struct in6_addr *)entry.sip6, tmpstr, sizeof(tmpstr));
								if (strcmp(tmpstr, "::"))
								{
									if(isOutifBridgeWAN)
									{
										if(entry.sip6PrefixLen)
											QOS_ADD_EBT_ARG(index, " --ip6-source %s/%d", tmpstr, entry.sip6PrefixLen);
										else
											QOS_ADD_EBT_ARG(index, " --ip6-source %s", tmpstr);
									}
									else
									{
										useIptable = 1;
										inet_ntop(PF_INET6, (struct in6_addr *)entry.sip6_end, tmpstr_end, sizeof(tmpstr_end));
										if(strcmp(tmpstr_end, "::"))
										{
											QOS_ADD_IPT_ARG(index, " -m iprange --src-range %s-%s", tmpstr, tmpstr_end);
										}
										else
										{
											if(entry.sip6PrefixLen)
												QOS_ADD_IPT_ARG(index, " -s %s/%d", tmpstr, entry.sip6PrefixLen);
											else
												QOS_ADD_IPT_ARG(index, " -s %s", tmpstr);
										}
									}
								}
							}
#endif
							
							if(classfRule[index].ipVersion == IPVER_IPV4)
							{
								snprintf(tmpstr, sizeof(tmpstr), "%s", inet_ntoa(*((struct in_addr *)entry.dip)));
								if(strcmp(tmpstr, ARG_0x4))
								{
									if(isOutifBridgeWAN)
									{
										if(entry.dmaskbit)
											QOS_ADD_EBT_ARG(index, " --ip-destination %s/%d", tmpstr, entry.dmaskbit);
										else
											QOS_ADD_EBT_ARG(index, " --ip-destination %s", tmpstr);
									}
									else
									{
										useIptable = 1;
										snprintf(tmpstr_end, sizeof(tmpstr_end), "%s", inet_ntoa(*((struct in_addr *)entry.dip_end)));
										if(strcmp(tmpstr_end, ARG_0x4))
										{
											QOS_ADD_IPT_ARG(index, " -m iprange --dst-range %s-%s", tmpstr, tmpstr_end);
										}
										else
										{
											if(entry.dmaskbit)
												QOS_ADD_IPT_ARG(index, " -d %s/%d", tmpstr, entry.dmaskbit);
											else
												QOS_ADD_IPT_ARG(index, " -d %s", tmpstr);
										}
									}
								}
							}
#ifdef CONFIG_IPV6
							else if (classfRule[index].ipVersion == IPVER_IPV6)
							{
								inet_ntop(PF_INET6, (struct in6_addr *)entry.dip6, tmpstr, sizeof(tmpstr));
								if (strcmp(tmpstr, "::"))
								{
									if(isOutifBridgeWAN)
									{
										if(entry.dip6PrefixLen)
											QOS_ADD_EBT_ARG(index, " --ip6-destination %s/%d", tmpstr, entry.dip6PrefixLen);
										else
											QOS_ADD_EBT_ARG(index, " --ip6-destination %s", tmpstr);
									}
									else
									{
										useIptable = 1;
										inet_ntop(PF_INET6, (struct in6_addr *)entry.dip6_end, tmpstr_end, sizeof(tmpstr_end));
										if(strcmp(tmpstr_end, "::"))
										{
											QOS_ADD_IPT_ARG(index, " -m iprange --dst-range %s-%s", tmpstr, tmpstr_end);
										}
										else
										{
											if(entry.dip6PrefixLen)
												QOS_ADD_IPT_ARG(index, " -d %s/%d", tmpstr, entry.dip6PrefixLen);
											else
												QOS_ADD_IPT_ARG(index, " -d %s", tmpstr);
										}
									}
								}
							}
#endif

#ifdef CONFIG_USER_AP_E8B_QOS
							if(entry.classficationType&(1<<IP_QOS_CLASSFICATIONTYPE_FlowLabel))
							{
								if(isOutifBridgeWAN)
									printf("[%s %d] Bridge WAN not support flowlbl filtering.\n", __func__, __LINE__);
								else
								{
									useIptable = 1;
									QOS_ADD_IPT_ARG(index, " -m flowlblrange --flowlbl-range 0x%x:0x%x", entry.flowlabel, entry.flowlabel_end);
								}
							}
#endif
							// Match 802.1p
							if(entry.vlan1p)
							{
								useEbtable = 1;
								QOS_ADD_EBT_ARG(index, " --vlan-prio %d", (entry.vlan1p-1)&0xff);
							}
#ifdef CONFIG_CMCC_ENTERPRISE
							if(entry.vid)
							{
								useEbtable = 1;
								QOS_ADD_EBT_ARG(index, " --vlan-id %d", entry.vid);
							}
#endif

						}
					}
					else if(entry.ipqos_rule_type==IPQOS_ETHERTYPE_BASE && entry.ethType)
					{
						useEbtable = 1;

#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
						QOS_ADD_EBT_ARG(curCmdNum, " -p 0x%x", entry.ethType);
						classfRule[curCmdNum].ipVersion = IPVER_IPV4;
						curCmdNum++;
#else
						if(!is_bridge_mode) { //gateway mode, use ip/ebtables
							useIptable = 1;
							if(entry.outif && entry.outif == DUMMY_IFINDEX){//ANY WAN
								for(j=0; j<MAX_VC_NUM; j++){
									if(!wan_port_itfs[logical_wan_port].itf[j].name[0] || !wan_port_itfs[logical_wan_port].itf[j].enable_qos)
										continue;
									//curCmdNum is 0 here
									QOS_ADD_EBT_ARG(curCmdNum, " -p 0x%x", entry.ethType);
									classfRule[curCmdNum].ipVersion = IPVER_IPV4;
									classfRule[curCmdNum].cmd_with_wan = 1;
									if(wan_port_itfs[logical_wan_port].itf[j].cmode!=CHANNEL_MODE_BRIDGE){//nat wan
										QOS_ADD_IPT_ARG(curCmdNum, " -o %s", wan_port_itfs[logical_wan_port].itf[j].name);
										curCmdNum++;
#ifdef CONFIG_IPV6
										classfRule[curCmdNum].ipVersion = IPVER_IPV6;
										QOS_ADD_IPT_ARG(curCmdNum, " -o %s", wan_port_itfs[logical_wan_port].itf[j].name);
										QOS_ADD_EBT_ARG(curCmdNum, " -p 0x%x", entry.ethType);
										classfRule[curCmdNum].cmd_with_wan = 1;
										curCmdNum++;
#endif
									}
									else{// bridge wan
										QOS_ADD_IPT_ARG(curCmdNum, " -m outphysdev --physdev %s", wan_port_itfs[logical_wan_port].itf[j].name);
										curCmdNum++;
#ifdef CONFIG_IPV6
										classfRule[curCmdNum].ipVersion = IPVER_IPV6;
										QOS_ADD_IPT_ARG(curCmdNum, " -m outphysdev --physdev %s", wan_port_itfs[logical_wan_port].itf[j].name);
										QOS_ADD_EBT_ARG(curCmdNum, " -p 0x%x", entry.ethType);
										classfRule[curCmdNum].cmd_with_wan = 1;
										curCmdNum++;
#endif
									}
								}
							}
							else if(entry.outif && entry.outif != DUMMY_IFINDEX){//SELECTED WAN
								ifGetName(entry.outif, ifname, sizeof(ifname));
								wan_qos_state = get_pvc_qos_state(entry.outif);
								is_bridge_wan = (wan_qos_state&QOS_WAN_BRIDGE) ? 1:0;
								QOS_ADD_EBT_ARG(curCmdNum, " -p 0x%x", entry.ethType);
								classfRule[curCmdNum].ipVersion = IPVER_IPV4;
								classfRule[curCmdNum].cmd_with_wan = 1;
								if(is_bridge_wan){
									QOS_ADD_IPT_ARG(curCmdNum, " -m outphysdev --physdev %s", ifname);
									curCmdNum++;
#ifdef CONFIG_IPV6
									classfRule[curCmdNum].ipVersion = IPVER_IPV6;
									QOS_ADD_IPT_ARG(curCmdNum, " -m outphysdev --physdev %s", ifname);
									QOS_ADD_EBT_ARG(curCmdNum, " -p 0x%x", entry.ethType);
									classfRule[curCmdNum].cmd_with_wan = 1;
									curCmdNum++;
#endif
								}else{
									QOS_ADD_IPT_ARG(curCmdNum, " -o %s", ifname);
									curCmdNum++;	
#ifdef CONFIG_IPV6
									classfRule[curCmdNum].ipVersion = IPVER_IPV6;
									QOS_ADD_IPT_ARG(curCmdNum, " -o %s", ifname);
									QOS_ADD_EBT_ARG(curCmdNum, " -p 0x%x", entry.ethType);
									classfRule[curCmdNum].cmd_with_wan = 1;
									curCmdNum++;
#endif
								}
							}
							// Set ebtableMark for ethType here, borrow Qos entry to set ethType 
							eth_type_mark = getEthTypeMapMark(entry.ethType);
							if(!eth_type_mark){
								printf("%s:%d The ethType 0x%x is not support\n",__FUNCTION__,__LINE__,entry.ethType);
								continue;
							}
							else{
								eth_type_mark++;// avoid confilct when eth_type_mark is 1 and the corresponding ebmark is 0x80
								ebtableMark = SOCK_MARK_QOS_ENTRY_TO_MARK(eth_type_mark);
							}
						}
						else{//bridge mode, use ebtables only
							QOS_ADD_EBT_ARG(curCmdNum, " -p 0x%x", entry.ethType);
							classfRule[curCmdNum].ipVersion = IPVER_IPV4;
							curCmdNum++;
						}
#endif
					}
#ifdef CONFIG_BRIDGE_EBT_DHCP_OPT
					else if(entry.ipqos_rule_type==IPQOS_DHCPOPT_BASE)
					{
						useEbtable = 1;
						QOS_ADD_EBT_ARG(curCmdNum, "-p 0x0800");
						classfRule[curCmdNum].ipVersion = IPVER_IPV4;
						curCmdNum++;

						snprintf(duid_mac, sizeof(duid_mac), "%02x:%02x:%02x:%02x:%02x:%02x",
							entry.duid_mac[0], entry.duid_mac[1],
							entry.duid_mac[2], entry.duid_mac[3],
							entry.duid_mac[4], entry.duid_mac[5]);
						///-- common parameter as following
						for(index=0 ; index<curCmdNum ; index++)
						{
							switch(entry.dhcpopt_type)
							{
								case IPQOS_DHCPOPT_60:
									//EX:ebtables -A FORWARD --dhcp-opt60 ABC -j DROP
									QOS_ADD_EBT_ARG(index, " --dhcp-opt60 \"%s\"", entry.opt60_vendorclass);
									break;

								case IPQOS_DHCPOPT_61:
									switch(entry.opt61_duid_type)
									{
										case 0:  //llt , EX: ebtables -A FORWARD --dhcp-opt61 1234/llt/1/1234/00:12:34:56:12:13 -j DROP
											QOS_ADD_EBT_ARG(index, " --dhcp-opt61 %d/llt/%d/%d/%s",entry.opt61_iaid,entry.duid_hw_type,entry.duid_time,duid_mac);
											break;
										case 1:  //en , EX: ebtables -A FORWARD --dhcp-opt61 1234/en/1234/ABCDE -j DROP
											QOS_ADD_EBT_ARG(index, " --dhcp-opt61 %d/en/%d/%s",entry.opt61_iaid,entry.duid_ent_num,entry.duid_ent_id);
											break;
										case 2:  //ll, EX: ebtables -A FORWARD --dhcp-opt61 1234/ll/1/00:12:34:56:12:13 -j DROP
											QOS_ADD_EBT_ARG(index, " --dhcp-opt61 %d/ll/%d/%s",entry.opt61_iaid,entry.duid_hw_type,duid_mac);
											break;
									}
									break;
								case IPQOS_DHCPOPT_125:
									//EX: ebtables -A FORWARD --dhcp-opt125 1234//DEF// -j DROP
									QOS_ADD_EBT_ARG(index, " --dhcp-opt125 %d/%s/%s/%s/%s",
											entry.opt125_ent_num,
											entry.opt125_manufacturer,
											entry.opt125_product_class,
											entry.opt125_model,
											entry.opt125_serial);
									break;
								default:
									break;
							}
						}
					}
#endif

					if(useIptable && useEbtable)
					{
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN					
						ebtableMark = SOCK_MARK_QOS_ENTRY_TO_MARK(rtk_qos_classf_rules_upstream_ebt_profile_get(i));
#endif
						iptableMark = rtk_qos_classf_rule_upstream_sock_mark_get(&entry, i);
					}
					else if(useEbtable || isOutifBridgeWAN)
					{
						ebtableMark = rtk_qos_classf_rule_upstream_sock_mark_get(&entry, i);
						iptableMark = 0;
					}
					else if(useIptable)
					{
						ebtableMark = 0;
						iptableMark = rtk_qos_classf_rule_upstream_sock_mark_get(&entry, i);					
					}
					
					mark = rtk_qos_classf_rule_upstream_sock_mark_get(&entry, i);

					if(entry.ipqos_rule_type==IPQOS_IP_PROTO_BASE || entry.ipqos_rule_type == IPQOS_TYPE_AUTO)
					{
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
						snprintf(ebtChainName, sizeof(ebtChainName), "%s",QOS_EB_US_CHAIN[current_qos_wan_port]);
#else
						snprintf(ebtChainName, sizeof(ebtChainName), QOS_EB_US_CHAIN);
#endif
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
						snprintf(iptChainName, sizeof(iptChainName),"%s", QOS_IP_CHAIN[current_qos_wan_port]);
						snprintf(iptTargetName, sizeof(iptTargetName), "%s_%d", QOS_IP_CHAIN[current_qos_wan_port], i);
#else
						snprintf(iptChainName, sizeof(iptChainName), QOS_IP_CHAIN);
						snprintf(iptTargetName, sizeof(iptChainName), "%s_%d", QOS_IP_CHAIN, i);
#endif
					}
					else
					{
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
						snprintf(ebtChainName, sizeof(ebtChainName),"%s", QOS_EB_US_CHAIN[current_qos_wan_port]);
						snprintf(iptChainName, sizeof(iptChainName),"%s",QOS_IP_CHAIN[current_qos_wan_port]);
						snprintf(iptTargetName, sizeof(iptTargetName), "%s_%d", QOS_IP_CHAIN[current_qos_wan_port], i);
#else
						snprintf(ebtChainName, sizeof(ebtChainName), QOS_EB_US_CHAIN);
						snprintf(iptChainName, sizeof(iptChainName), QOS_IP_CHAIN);
						snprintf(iptTargetName, sizeof(iptChainName), "%s_%d", QOS_IP_CHAIN, i);
#endif
					}

#ifdef _PRMT_X_CT_COM_QOS_
					/* TR069 DIP rule for QoS mode should add to OUTPUT chain */
					if (entry.modeTr69 == MODETR069 && entry.cttypemap[0] == 4)
					{
						snprintf(iptChainName, sizeof(iptChainName), "qos_rule_output");
					}
#endif

					if(useEbtable)
					{
						for(index=0 ; index<curCmdNum ; index++)
						{
							if(useIptable && entry.phyPort)
							{
								ebtableMark = SOCK_MARK_QOS_ENTRY_TO_MARK(classfRule[index].profile);
							}
							if(enableDscpMark && entry.m_dscp && (isOutifBridgeWAN || !useIptable))
							{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
								DOCMDARGVS(EBTABLES, DOWAIT, "-t %s -A %s %s  -j ftos --set-ftos 0x%x", "broute", ebtChainName, classfRule[index].ebtCommand, (entry.m_dscp&0xFF)<<2);
#else
								DOCMDARGVS(EBTABLES, DOWAIT, "-t %s -A %s %s  -j ftos --set-ftos 0x%x", "broute", ebtChainName, classfRule[index].ebtCommand, (entry.m_dscp-1)&0xFF);
#endif
							}
							DOCMDARGVS(EBTABLES, DOWAIT, "-t %s -A %s %s -j mark --mark-and 0x%x", "broute", ebtChainName, classfRule[index].ebtCommand, ~SOCK_MARK_QOS_MASK_ALL);
							DOCMDARGVS(EBTABLES, DOWAIT, "-t %s -A %s %s -j mark --mark-or 0x%x", "broute", ebtChainName, classfRule[index].ebtCommand, ebtableMark);
							DOCMDARGVS(EBTABLES, DOWAIT, "-t %s -A %s %s -j RETURN", "broute", ebtChainName, classfRule[index].ebtCommand);
						}
					}
					if(useIptable && !isOutifBridgeWAN)
					{
						for(index=0 ; index<curCmdNum ; index++)
						{
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
							if(!is_bridge_mode && !classfRule[index].cmd_with_wan)
								continue;
#endif
							if(useEbtable)
							{
								if(entry.phyPort)
								{
									ebtableMark = SOCK_MARK_QOS_ENTRY_TO_MARK(classfRule[index].profile);
								}
								QOS_ADD_IPT_ARG(index, " -m mark --mark 0x%x/0x%x", ebtableMark, SOCK_MARK_QOS_ENTRY_BIT_MASK);
							}
													
							if(classfRule[index].ipVersion == IPVER_IPV4)
							{
								if(enableDscpMark && entry.m_dscp)
								{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_AP_E8B_QOS)
									DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -A %s -j DSCP --set-dscp 0x%x", iptTargetName, entry.m_dscp);
#else
									DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -A %s -j DSCP --set-dscp 0x%x", iptTargetName, (entry.m_dscp-1)>>2);
#endif
								}
								DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -A %s -j MARK --set-mark 0x%x/0x%x", iptTargetName, iptableMark, SOCK_MARK_QOS_MASK_ALL);
								DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -A %s %s -g %s", iptChainName, classfRule[index].iptCommand, iptTargetName);
							}
#ifdef CONFIG_IPV6
							else if(classfRule[index].ipVersion == IPVER_IPV6)
							{
								if(enableDscpMark && entry.m_dscp)
								{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_AP_E8B_QOS)
									DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -A %s -j TOS --set-tos 0x%x", iptTargetName, entry.m_dscp);
#else
									DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -A %s -j DSCP --set-dscp 0x%x", iptTargetName, (entry.m_dscp-1)>>2);
#endif
								}
								DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -A %s -j MARK --set-mark 0x%x/0x%x", iptTargetName, iptableMark, SOCK_MARK_QOS_MASK_ALL);
								DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -A %s %s -g %s", iptChainName, classfRule[index].iptCommand, iptTargetName);
							}
#endif
						}
					}

					tc_protocol = "protocol all";
					vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
#ifdef QOS_SETUP_IFB
					if(PLY_PRIO==policy)//priority queue
					{
						ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
#ifndef CONFIG_USER_AP_QOS
						DOCMDARGVS(TC, DOWAIT, "filter add dev ifb0 parent 2: prio %d %s handle 0x%x/0x%x fw flowid 2:%d",
								entry.prior, tc_protocol, mark, SOCK_MARK_QOS_MASK_ALL, entry.prior);
#else
						DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio 1 %s handle 0x%x/0x%x fw flowid 1:100",
								devname,tc_protocol, mark, SOCK_MARK_QOS_MASK_ALL);
								
						DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 2: prio 1 %s handle 0x%x/0x%x fw flowid 2:%d",
								devname,tc_protocol, mark, SOCK_MARK_QOS_MASK_ALL, entry.prior);
#endif
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN					
						/* 
							Consider QoS SP mode. 
							Some packets will be dropped in VC/PTM/ETHWAN queue.
							Create a SP qdisc in VC/PTM/ETHWAN queue to avoid this problem.	
						*/
						for(j=0; j<vcEntryNum; j++)
						{
							if(!mib_chain_get(MIB_ATM_VC_TBL, j, &vcEntry) || !vcEntry.enableIpQos)
								continue;
							
							// Use "action mirred egress redirect" to do 2 layer QoS. One is on ifb0, another is on physical device(nas0_0).
							// It is that if the packet match the rule, it will demonstrate is the following sequence
							// (1) Packet is classified as class 1:%d and redirected to ifb0(the first QoS).
							// (2) Packet will come back to physical device(nas0_0) (the second QoS).
							ifGetName(PHY_INTF(vcEntry.ifIndex), ifname, sizeof(ifname));
#ifndef CONFIG_USER_AP_QOS
							DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio %d %s handle 0x%x/0x%x fw flowid 1:%d action mirred egress redirect dev ifb0",
								ifname, entry.prior, tc_protocol, mark, SOCK_MARK_QOS_MASK_ALL, entry.prior);
#else
							DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio 1 %s handle 0x%x/0x%x fw flowid 1:%d action mirred egress redirect dev %s",
								ifname, tc_protocol, mark, SOCK_MARK_QOS_MASK_ALL, entry.prior,devname);
#endif
						}
#endif
					}
					else if(PLY_WRR==policy)//weighted round robin
					{
						DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio 1 %s handle 0x%x/0x%x fw flowid 1:%d00",
							devname,tc_protocol, mark, SOCK_MARK_QOS_MASK_ALL, entry.prior);
					}
#elif defined(QOS_SETUP_IMQ)
					if(PLY_PRIO==policy)//priority queue
					{

						ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
						DOCMDARGVS(TC, DOWAIT, "filter add dev imq0 parent 2: prio %d %s handle 0x%x/0x%x fw flowid 2:%d",
								entry.prior, tc_protocol, mark, SOCK_MARK_QOS_MASK_ALL, entry.prior);

						{
							/*
								If in bridge mode, 
								need redirect packet to imq0 by tc filter rule
							*/
							for(j=0; j<vcEntryNum; j++)
							{
								if(!mib_chain_get(MIB_ATM_VC_TBL, j, &vcEntry) || !vcEntry.enableIpQos)
									continue;
									
								if (vcEntry.cmode != CHANNEL_MODE_BRIDGE)
									continue;

								ifGetName(PHY_INTF(vcEntry.ifIndex), ifname, sizeof(ifname));
									DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio %d %s handle 0x%x/0x%x fw flowid 2:%d action mirred egress redirect dev imq0",
									ifname, entry.prior, tc_protocol, mark, SOCK_MARK_QOS_MASK_ALL, entry.prior);
							}
						}

						{
							/* 	
								Consider route mode, QoS SP mode. 
								Some packets will be dropped in VC/PTM/ETHWAN queue.
								Create a SP qdisc in VC/PTM/ETHWAN queue to avoid this problem.				
							*/
							for(j=0; j<vcEntryNum; j++)
							{
								if(!mib_chain_get(MIB_ATM_VC_TBL, j, &vcEntry) || !vcEntry.enableIpQos)
									continue;
									
								if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
									continue;

								ifGetName(PHY_INTF(vcEntry.ifIndex), ifname, sizeof(ifname));
								DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio %d %s handle 0x%x/0x%x fw flowid 1:%d",
									ifname, entry.prior, tc_protocol, mark, SOCK_MARK_QOS_MASK_ALL, entry.prior);
							}
						}
					}
					else if(PLY_WRR==policy)//weighted round robin
					{
						DOCMDARGVS(TC, DOWAIT, "filter add dev imq0 parent 1: prio 1 %s handle 0x%x/0x%x fw flowid 1:%d00",
							tc_protocol, mark, SOCK_MARK_QOS_MASK_ALL, entry.prior);

						// Mason Yu.
						// If the vc is bridge mode , set tc action mirred and redirect to img0
						{				
							for(j=0; j<vcEntryNum; j++)
							{
								if(!mib_chain_get(MIB_ATM_VC_TBL, j, &vcEntry) || !vcEntry.enableIpQos)
									continue;

								if (vcEntry.cmode != CHANNEL_MODE_BRIDGE)
									continue;	
									
								ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));					
								DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio 1 %s handle 0x%x/0x%x fw flowid 1:1 action mirred egress redirect dev imq0",
									ifname, tc_protocol, mark, SOCK_MARK_QOS_MASK_ALL);					
							}
						}
					}
#else
					if(PLY_PRIO==policy)//priority queue
					{
						//DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio %d protocol ip handle 0x%x/0x%x fw flowid 1:%d",
						//	ifname, entry.prior, mark, SOCK_MARK_QOS_MASK_ALL, entry.prior);
						for(j=0; j<vcEntryNum; j++)
						{
							if(!mib_chain_get(MIB_ATM_VC_TBL, j, &vcEntry) || !vcEntry.enableIpQos )
								continue;

							ifGetName(PHY_INTF(vcEntry.ifIndex), ifname, sizeof(ifname));
							DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio %d %s handle 0x%x/0x%x fw flowid 1:%d",
								ifname, entry.prior, tc_protocol, mark, SOCK_MARK_QOS_MASK_ALL, entry.prior);
						}
					}
					else if(PLY_WRR==policy)//weighted round robin
					{
						for(j=0; j<vcEntryNum; j++)
						{
							if(!mib_chain_get(MIB_ATM_VC_TBL, j, &vcEntry) || !vcEntry.enableIpQos)
								continue;

							ifGetName(PHY_INTF(vcEntry.ifIndex), ifname, sizeof(ifname));

							DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio 1 %s handle 0x%x/0x%x fw flowid 1:%d00",
								ifname, tc_protocol, mark, SOCK_MARK_QOS_MASK_ALL, entry.prior);
						}
					}
#endif
					hwnat_qos_translate_rule(&entry);
				}
#ifdef CONFIG_RTL_SMUX_DEV
#if defined(CONFIG_USER_AP_QOS) && defined(_PRMT_X_CT_COM_QOS_)
				// enableQos1p=0: not use, enableQos1p=1: use old value, enableQos1p=2: mark new value
				if(enableQos1p == 2)
#endif
				{
					rtk_qos_smuxdev_qos_us_rule_set(mark, i);
				}
#endif
			}
		}		
	}
	
    return 0;
}

static int rtk_qos_classf_rules_downstream_ipt_profile_get(int mibIndex)
{
	MIB_CE_IP_QOS_T thisEntry;
	MIB_CE_IP_QOS_T entry;
	int entryNum = 0;
	int i;

	if(!mib_chain_get(MIB_IP_QOS_TBL, mibIndex, (void*)&thisEntry) || !thisEntry.enable)
		return 0;
	
	entryNum = mib_chain_total(MIB_IP_QOS_TBL);
	for(i=0 ; i<entryNum ; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)&entry) || !entry.enable)
			continue;
		
		if(entry.direction == QOS_DIRECTION_DOWNSTREAM)
		{
			if(i == mibIndex)
				continue;
			
#ifdef CONFIG_IPV6
			if(!(entry.IpProtocol&thisEntry.IpProtocol))
				continue;
#endif

			if(entry.protoType != thisEntry.protoType &&
				!((entry.protoType == PROTO_TCP || entry.protoType == PROTO_UDP) && thisEntry.protoType == PROTO_UDPTCP) &&
				!((thisEntry.protoType == PROTO_TCP || thisEntry.protoType == PROTO_UDP) && entry.protoType == PROTO_UDPTCP))
				continue;
			
			if(memcmp(&entry.sip[0], &thisEntry.sip[0], IP_ADDR_LEN))
				continue;

			if(entry.sPort != thisEntry.sPort)
				continue;

			if(memcmp(&entry.dip[0], &thisEntry.dip[0], IP_ADDR_LEN))
				continue;

			if(entry.dPort != thisEntry.dPort)
				continue;

			if(entry.qosDscp != thisEntry.qosDscp)
				continue;

			if(memcmp(entry.smac, thisEntry.smac, MAC_ADDR_LEN))
				continue;

			if(entry.smaskbit!=thisEntry.smaskbit)
				continue;

			if(memcmp(entry.dmac, thisEntry.dmac, MAC_ADDR_LEN))
				continue;

			if(entry.dmaskbit!=thisEntry.dmaskbit)
				continue;

			if(entry.outif != thisEntry.outif)
				continue;

#ifdef CONFIG_IPV6
			if(memcmp(&entry.sip6[0], &thisEntry.sip6[0], IP6_ADDR_LEN))
				continue;

			if(entry.sip6PrefixLen != thisEntry.sip6PrefixLen)
				continue;

			if(memcmp(&entry.dip6[0], &thisEntry.dip6[0], IP6_ADDR_LEN))
				continue;

			if(entry.dip6PrefixLen != thisEntry.dip6PrefixLen)
				continue;
#endif

			return (mibIndex<i) ? (mibIndex+1) : (i+1);
		}
	}

	return (mibIndex+1);
}

static int rtk_qos_classf_rules_downstream_setup(unsigned char policy)
{
	struct rtk_qos_classf_rule_entry	classfRule[MAX_EBT_IPT_COMMAND_NUMBER] = {0};
	unsigned char curCmdNum=0;
	unsigned char enableDscpMark=1;
	unsigned char enableQos1p=2;
	int entryNum = 0, vcEntryNum = 0;
	MIB_CE_IP_QOS_T entry;
	MIB_CE_ATM_VC_T vcEntry;
	unsigned int useIptable=0;
	unsigned int useEbtable=0;
	unsigned char ipProtocol;
	char *pCommand;
	int order, j;
#ifdef QOS_SUPPORT_RTP
	FILE *fp;
	char buff[100];
	unsigned int rtp_dip;
	unsigned int rtp_dpt;
	unsigned int rtpCnt=0, rtpIndex=0, rtpRuleCnt=0;
	struct rtp_struct {
		unsigned int dip;
		unsigned int dport;
	} __PACK__;
	struct rtp_rule_st {
		char daddr[64];
		char dport[48];
	} __PACK__;

	struct rtp_struct rtp_entry[RTP_RULE_NUM_MAX];
	struct rtp_rule_st rtp_rule_entry[RTP_RULE_NUM_MAX];
#endif

	DOCMDINIT

	QOS_SETUP_PRINT_FUNCTION

	//get current RTP stream info
#ifdef QOS_SUPPORT_RTP
	if (!(fp=fopen(RTP_PORT_FILE, "r"))) {
		printf("no RTP connection!\n");
	} else {
		while ( fgets(buff, sizeof(buff), fp) != NULL ) {
			if(sscanf(buff, "ip=%x port=%u", &rtp_dip, &rtp_dpt)!=2) {
				printf("Unsuported rtp format\n");
				break;
			}
			else {
				rtpCnt++;
				rtp_entry[rtpCnt-1].dip = rtp_dip;
				rtp_entry[rtpCnt-1].dport = rtp_dpt;
			}
		}
		fclose(fp);
	}
#endif

#if defined(_PRMT_X_CT_COM_QOS_)
	mib_get_s(MIB_QOS_ENABLE_DOWNSTREAM_DSCP_MARK, (void *)&enableDscpMark, sizeof(enableDscpMark));
	mib_get_s(MIB_QOS_ENABLE_DOWNSTREAM_1P, (void *)&enableQos1p, sizeof(enableQos1p));
#endif
	entryNum = mib_chain_total(MIB_IP_QOS_TBL);

	for(order=1 ; order<(entryNum+1) ; order++)
	{
		unsigned int mark=0, iptableMark=0, ebtableMark=0;
		char wanIfname[IFNAMSIZ];
		unsigned short qid=0;
		char tmpstr[48]={0};
		int index;

		useIptable=0;
		useEbtable=0;
		curCmdNum=0;
		memset(classfRule, 0, sizeof(classfRule));		
		
		memset(&entry, 0, sizeof(entry));
		if(!mib_chain_get(MIB_IP_QOS_TBL, (order-1), (void*)&entry) || !entry.enable)
			continue;

		if(enableDscpMark && entry.m_dscp)
		{
			useEbtable = 1;
		}

		if(entry.direction == QOS_DIRECTION_DOWNSTREAM)
		{
#ifdef CONFIG_IPV6
			if(entry.IpProtocol&IPVER_IPV4_IPV6)
			{
				if(entry.IpProtocol&IPVER_IPV4)
					ipProtocol |= IPVER_IPV4;

				if(entry.IpProtocol&IPVER_IPV6)
					ipProtocol |= IPVER_IPV6;
			}
			else if(entry.ipqos_rule_type==IPQOS_MAC_BASE)
			{
				ipProtocol = IPVER_IPV4_IPV6;
			}
#else
			ipProtocol = IPVER_IPV4;
#endif

#ifdef CONFIG_QOS_SUPPORT_PROTOCOL_LIST
			//protocol
			if(entry.protoType > PROTO_MAX) //invalid protocol index
				return 1;

			for (j=0 ; (1<<j)&PROTO_MAX ; j++)
			{
				if (!(entry.protoType&(1<<j)))
					continue;

				useIptable = 1;
				if (((1<<j)&(PROTO_TCP|PROTO_UDP|PROTO_ICMP)))
				{
					if(ipProtocol&IPVER_IPV4)
					{
						if ((1<<j) == PROTO_TCP)
							QOS_ADD_IPT_ARG(curCmdNum, proto2str[1]); //TCP
						else if ((1<<j) == PROTO_UDP)
							QOS_ADD_IPT_ARG(curCmdNum, proto2str[2]); //UDP
						else if ((1<<j) == PROTO_ICMP)
							QOS_ADD_IPT_ARG(curCmdNum, proto2str[3]); //ICMP
						classfRule[curCmdNum].ipVersion = IPVER_IPV4;
						classfRule[curCmdNum].protocol = (1<<j);
						curCmdNum++;
					}
#ifdef CONFIG_IPV6
					if(ipProtocol&IPVER_IPV6)
					{
						if ((1<<j) == PROTO_TCP)
							QOS_ADD_IPT_ARG(curCmdNum, proto2str_v6[1]); //TCP
						else if ((1<<j) == PROTO_UDP)
							QOS_ADD_IPT_ARG(curCmdNum, proto2str_v6[2]); //UDP
						else if ((1<<j) == PROTO_ICMP)
							QOS_ADD_IPT_ARG(curCmdNum, proto2str_v6[3]); //ICMPV6
						classfRule[curCmdNum].ipVersion = IPVER_IPV6;
						classfRule[curCmdNum].protocol = (1<<j);
						curCmdNum++;
					}
#endif
				}
				if (((1<<j)&PROTO_RTP))
				{
#ifdef QOS_SUPPORT_RTP
					for(rtpIndex=0 ; rtpIndex<rtpCnt ; rtpIndex++)
					{
						if(ipProtocol&IPVER_IPV4)
						{
							QOS_ADD_IPT_ARG(curCmdNum, "%s --dport %d", proto2str[2], rtp_entry[rtpIndex].dport); // UDP
							classfRule[curCmdNum].ipVersion = IPVER_IPV4;
							classfRule[curCmdNum].protocol = PROTO_RTP;
							curCmdNum++;
						}

#ifdef CONFIG_IPV6
						if(ipProtocol&IPVER_IPV6)
						{
							QOS_ADD_IPT_ARG(curCmdNum, "%s --dport %d", proto2str_v6[2], rtp_entry[rtpIndex].dport); // UDP
							classfRule[curCmdNum].ipVersion = IPVER_IPV6;
							classfRule[curCmdNum].protocol = PROTO_RTP;
							curCmdNum++;
						}
#endif
					}
#else
					if(ipProtocol&IPVER_IPV4)
					{
						QOS_ADD_IPT_ARG(curCmdNum, "-m helper --helper sip"); // RTP
						classfRule[curCmdNum].ipVersion = IPVER_IPV4;
						classfRule[curCmdNum].protocol = PROTO_RTP;
						curCmdNum++;
					}
#ifdef CONFIG_IPV6
					if(ipProtocol&IPVER_IPV6)
					{
						QOS_ADD_IPT_ARG(curCmdNum, "-m helper --helper sip"); // RTP
						classfRule[curCmdNum].ipVersion = IPVER_IPV6;
						classfRule[curCmdNum].protocol = PROTO_RTP;
						curCmdNum++;
					}
#endif
#endif
				}
			}

			if (!curCmdNum) // non protocol case
			{
				if(entry.protoType == PROTO_NONE)
					useIptable = 1;

				if(ipProtocol&IPVER_IPV4)
				{
					QOS_ADD_IPT_ARG(curCmdNum, (" %s", proto2str[entry.protoType]));
					classfRule[curCmdNum].ipVersion = IPVER_IPV4;
					classfRule[curCmdNum].protocol = entry.protoType;
					curCmdNum++;
				}

#ifdef CONFIG_IPV6
				if(ipProtocol&IPVER_IPV6)
				{
					QOS_ADD_IPT_ARG(curCmdNum, (" %s", proto2str_v6[entry.protoType]));
					classfRule[curCmdNum].ipVersion = IPVER_IPV6;
					classfRule[curCmdNum].protocol = entry.protoType;
					curCmdNum++;
				}
#endif
			}
#else
			if(entry.protoType == PROTO_UDPTCP)
			{
				useIptable = 1;
				if(ipProtocol&IPVER_IPV4)
				{
					QOS_ADD_IPT_ARG(curCmdNum, " -p TCP");
					classfRule[curCmdNum].ipVersion = IPVER_IPV4;
					classfRule[curCmdNum].protocol = PROTO_TCP;
					curCmdNum++;
					QOS_ADD_IPT_ARG(curCmdNum, " -p UDP");
					classfRule[curCmdNum].ipVersion = IPVER_IPV4;
					classfRule[curCmdNum].protocol = PROTO_UDP;
					curCmdNum++;
				}

#ifdef CONFIG_IPV6
				if(ipProtocol&IPVER_IPV6)
				{
					QOS_ADD_IPT_ARG(curCmdNum, " -p TCP");
					classfRule[curCmdNum].ipVersion = IPVER_IPV6;
					classfRule[curCmdNum].protocol = PROTO_TCP;
					curCmdNum++;
					QOS_ADD_IPT_ARG(curCmdNum, " -p UDP");
					classfRule[curCmdNum].ipVersion = IPVER_IPV6;
					classfRule[curCmdNum].protocol = PROTO_UDP;
					curCmdNum++;
				}
#endif
			}
			else
			{
				if(entry.protoType != PROTO_NONE)
					useIptable = 1;

				if(ipProtocol&IPVER_IPV4)
				{
					QOS_ADD_IPT_ARG(curCmdNum, (" %s", proto2str[entry.protoType]));
					classfRule[curCmdNum].ipVersion = IPVER_IPV4;
					classfRule[curCmdNum].protocol = entry.protoType;
					curCmdNum++;
				}

#ifdef CONFIG_IPV6
				if(ipProtocol&IPVER_IPV6)
				{
					QOS_ADD_IPT_ARG(curCmdNum, (" %s", proto2str_v6[entry.protoType]));
					classfRule[curCmdNum].ipVersion = IPVER_IPV6;
					classfRule[curCmdNum].protocol = entry.protoType;
					curCmdNum++;
				}
#endif
			}
#endif

			///-- common parameter as following
			for(index=0 ; index<curCmdNum ; index++)
			{
				if(classfRule[index].protocol==PROTO_TCP || classfRule[index].protocol==PROTO_UDP)
				{
					//source port (range)
					if((PROTO_NONE != entry.protoType)&&(PROTO_ICMP != entry.protoType)&&0 != entry.sPort)
					{
						useIptable = 1;
						if(entry.sPortRangeMax)
							QOS_ADD_IPT_ARG(index, " --sport %d:%d", MIN_PORT(entry.sPort, entry.sPortRangeMax), MAX_PORT(entry.sPort, entry.sPortRangeMax));
						else
							QOS_ADD_IPT_ARG(index, " --sport %d", entry.sPort);
					}

					//dest port (range)
					if((PROTO_NONE != entry.protoType)&&(PROTO_ICMP != entry.protoType)&&0 != entry.dPort)
					{
						useIptable = 1;
						if(entry.dPortRangeMax)
							QOS_ADD_IPT_ARG(index, " --dport %d:%d", MIN_PORT(entry.dPort, entry.dPortRangeMax), MAX_PORT(entry.dPort, entry.dPortRangeMax));
						else
							QOS_ADD_IPT_ARG(index, " --dport %d", entry.dPort);
					}
				}

				//interface
				if ( entry.outif != DUMMY_IFINDEX ) 
				{
					useIptable = 1;
					ifGetName(entry.outif, wanIfname, sizeof(wanIfname));
				
					//wan port
					QOS_ADD_IPT_ARG(index, " -i %s", wanIfname);
				}

				if (memcmp(entry.smac, EMPTY_MAC, MAC_ADDR_LEN)) 
				{
					useIptable = 1;
					mac_hex2string(entry.smac, tmpstr);
					QOS_ADD_IPT_ARG(index, " -m mac --mac-source %s", tmpstr);
				}

				//dscp match
				if(entry.qosDscp)
				{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_AP_E8B_QOS)
					QOS_ADD_IPT_ARG(index, " -m dscp --dscp 0x%x", entry.qosDscp);
#else
					QOS_ADD_IPT_ARG(index, " -m dscp --dscp 0x%x", (entry.qosDscp-1)>>2);
#endif
				}

				// This is a IPv4 rule
				if (ipProtocol&IPVER_IPV4 && classfRule[index].ipVersion&IPVER_IPV4)
				{
					//source address
					snprintf(tmpstr, 16, "%s", inet_ntoa(*((struct in_addr *)entry.sip)));
					if (strcmp(tmpstr, ARG_0x4))
					{
						useIptable = 1;
						if(entry.smaskbit)
							QOS_ADD_IPT_ARG(index, " -s %s/%d", tmpstr, entry.smaskbit);
						else
							QOS_ADD_IPT_ARG(index, " -s %s", tmpstr);
					}
					//destination address
					snprintf(tmpstr, 16, "%s", inet_ntoa(*((struct in_addr *)entry.dip)));
					if (strcmp(tmpstr, ARG_0x4))
					{
						useIptable = 1;
						if(entry.dmaskbit)
							QOS_ADD_IPT_ARG(index, " -d %s/%d", tmpstr, entry.dmaskbit);
						else
							QOS_ADD_IPT_ARG(index, " -d %s", tmpstr);
					}
				}

#ifdef CONFIG_IPV6
				// This is a IPv6 rule
				if (ipProtocol&IPVER_IPV6 && classfRule[index].ipVersion&IPVER_IPV6)
				{
					//source address
					inet_ntop(PF_INET6, (struct in6_addr *)entry.sip6, tmpstr, sizeof(tmpstr));
					if (strcmp(tmpstr, "::"))
					{
						useIptable = 1;
						if(entry.sip6PrefixLen)
							QOS_ADD_IPT_ARG(index, " -s %s/%d", tmpstr, entry.sip6PrefixLen);
						else
							QOS_ADD_IPT_ARG(index, " -s %s", tmpstr);
					}
					//destination address
					inet_ntop(PF_INET6, (struct in6_addr *)entry.dip6, tmpstr, sizeof(tmpstr));
					if (strcmp(tmpstr, "::"))
					{
						useIptable = 1;
						if(entry.dip6PrefixLen)
							QOS_ADD_IPT_ARG(index, " -d %s/%d", tmpstr, entry.dip6PrefixLen);
						else
							QOS_ADD_IPT_ARG(index, " -d %s", tmpstr);
					}
				}
#endif

				//vlan 802.1p priority, use 802.1Q ethernet protocol
				if(entry.vlan1p)
				{
					useEbtable = 1;
					//lan 802.1p mark, 0-2 bit(match)
					QOS_ADD_EBT_ARG(index, " -p 802_1Q --vlan-prio %d", (entry.vlan1p-1)&0xff);
				}
				
				//phy port, 0: none, range:1-4
				if (entry.phyPort>=1)
				{
					useEbtable = 1;
					QOS_ADD_EBT_ARG(index, " -o %s", lan_itf_map.map[entry.phyPort].name);
				}
				
				if(memcmp(entry.dmac, EMPTY_MAC, MAC_ADDR_LEN)) 
				{
					useEbtable = 1;
					mac_hex2string(entry.dmac, tmpstr);
					QOS_ADD_EBT_ARG(index, " -d %s", tmpstr);
				}
			}
			
			if(useIptable && useEbtable)
			{
				iptableMark |= SOCK_MARK_QOS_ENTRY_TO_MARK(rtk_qos_classf_rules_downstream_ipt_profile_get(order-1));
				ebtableMark |= SOCK_MARK_QOS_ENTRY_TO_MARK(order);
				mark |= SOCK_MARK_QOS_ENTRY_TO_MARK(order);
				
				if(entry.prior>0)
				{
					qid = (getIPQosQueueNumber()-entry.prior);
					ebtableMark |= (qid)<<SOCK_MARK_QOS_SWQID_START;
					mark |= (qid)<<SOCK_MARK_QOS_SWQID_START;
				}
				
				if(0 != entry.m_1p)
				{
					ebtableMark |= SOCK_MARK_8021P_ENABLE_TO_MARK(1);
					mark |= SOCK_MARK_8021P_ENABLE_TO_MARK(1);
				}
			}
			else if(useIptable)
			{
				iptableMark |= ((order)<<SOCK_MARK_QOS_ENTRY_START);
				mark |= ((order)<<SOCK_MARK_QOS_ENTRY_START);

				if(entry.prior>0)
				{
					qid = (getIPQosQueueNumber()-entry.prior);
					iptableMark |= (qid)<<SOCK_MARK_QOS_SWQID_START;
					mark |= (qid)<<SOCK_MARK_QOS_SWQID_START;
				}
				
				if(0 != entry.m_1p)
				{
					iptableMark |= SOCK_MARK_8021P_ENABLE_TO_MARK(1);
					mark |= SOCK_MARK_8021P_ENABLE_TO_MARK(1);
				}
			}
			else if(useEbtable)
			{
				ebtableMark |= ((order)<<SOCK_MARK_QOS_ENTRY_START);
				mark |= ((order)<<SOCK_MARK_QOS_ENTRY_START);

				if(entry.prior>0)
				{
					qid = (getIPQosQueueNumber()-entry.prior);
					ebtableMark |= (qid)<<SOCK_MARK_QOS_SWQID_START;
					mark |= (qid)<<SOCK_MARK_QOS_SWQID_START;
				}
				
				if(0 != entry.m_1p)
				{
					ebtableMark |= SOCK_MARK_8021P_ENABLE_TO_MARK(1);
					mark |= SOCK_MARK_8021P_ENABLE_TO_MARK(1);
				}
			}

			if(useIptable)
			{
				for(index=0 ; index<curCmdNum ; index++)
				{
					if(classfRule[index].ipVersion == IPVER_IPV4)
					{
  						DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -A %s %s -j MARK --set-mark 0x%x/0x%x", QOS_IP_CHAIN, classfRule[index].iptCommand, iptableMark, SOCK_MARK_QOS_MASK_ALL);
					}
#ifdef CONFIG_IPV6
					else if(classfRule[index].ipVersion == IPVER_IPV6)
					{
						DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -A %s %s  -j MARK --set-mark 0x%x/0x%x", QOS_IP_CHAIN, classfRule[index].iptCommand, iptableMark, SOCK_MARK_QOS_MASK_ALL);
					}
#endif
					if(classfRule[index].ipVersion == IPVER_IPV4)
					{
						DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -A %s %s -j RETURN", QOS_IP_CHAIN, classfRule[index].iptCommand);
					}
#ifdef CONFIG_IPV6
					else if(classfRule[index].ipVersion == IPVER_IPV6)
					{
						DOCMDARGVS(IP6TABLES, DOWAIT, "-t mangle -A %s %s -j RETURN", QOS_IP_CHAIN, classfRule[index].iptCommand);
					}
#endif
				}
			}

			if(useEbtable)
			{
				for(index=0 ; index<curCmdNum ; index++)
				{
					if(useIptable && classfRule[index].ipVersion)
					{
						QOS_ADD_EBT_ARG(index, " --mark 0x%x/0x%x", iptableMark, SOCK_MARK_QOS_ENTRY_BIT_MASK);
						// remark rule must have higher priority
						QOS_ADD_EBT_ARG(index, " --mark2 0x0/0x%llx", (SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_MASK|SOCK_MARK2_DSCP_DOWN_BIT_MASK));
					}

					/* Set DSCP value */
					if(enableDscpMark && entry.m_dscp)
					{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
						DOCMDARGVS(EBTABLES, DOWAIT, "-t %s -A %s %s -j ftos --set-ftos 0x%x", "nat", QOS_EB_DS_CHAIN, classfRule[index].ebtCommand, (entry.m_dscp&0xFF)<<2);
#else
						DOCMDARGVS(EBTABLES, DOWAIT, "-t %s -A %s %s -j ftos --set-ftos 0x%x", "nat", QOS_EB_DS_CHAIN, classfRule[index].ebtCommand, (entry.m_dscp-1)&0xFF);
#endif		
					}
					DOCMDARGVS(EBTABLES, DOWAIT, "-t %s -A %s %s -j mark --mark-or 0x%x", "nat", QOS_EB_DS_CHAIN, classfRule[index].ebtCommand, ebtableMark);
				}
			}
#ifdef CONFIG_RTL_SMUX_DEV
			rtk_qos_smuxdev_qos_ds_rule_set(mark,order-1);
#endif
		}
	}

    return 0;
}

/*********************************************************************
 * NAME:    setup_qos_rules
 * DESC:    Using iptables to setup rules for queue, including packet
 *          marking(dscp, 802.1p). This function doesn't affect traffic
 *          controlling, just for priority and weighted round robin
 *          queue. Suppose that the mib setting is well-formated and right.
 * ARGS:    none
 * RETURN:  0 success, others fail
 *********************************************************************/
static int setup_qos_rules(unsigned char policy)
{
	rtk_qos_classf_rules_upstream_setup(policy);
#ifdef CONFIG_QOS_SUPPORT_DOWNSTREAM
	rtk_qos_classf_rules_downstream_setup(policy);
#endif
	return 0;
}

int setupQosRuleMainChain(unsigned int enable)
{
	QOS_SETUP_PRINT_FUNCTION

	if (enable)
	{
		va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N", QOS_EB_US_CHAIN);
		va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P", QOS_EB_US_CHAIN, "RETURN");
		va_cmd(EBTABLES, 6, 1, "-t", "broute", "-A", "BROUTING", "-j", QOS_EB_US_CHAIN);

		va_cmd(EBTABLES, 4, 1, "-t", "nat", "-N", QOS_EB_DS_CHAIN);
		va_cmd(EBTABLES, 5, 1, "-t", "nat", "-P", QOS_EB_DS_CHAIN, "RETURN");
		va_cmd(EBTABLES, 6, 1, "-t", "nat", "-A", "POSTROUTING", "-j", QOS_EB_DS_CHAIN);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", QOS_IP_CHAIN);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", QOS_IP_CHAIN);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", "pvc_mark");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", "pvc_mark");

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", "qos_rule_output");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "OUTPUT", "-j", "qos_rule_output");

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", "pvc_mark_output");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "OUTPUT", "-j", "pvc_mark_output");
#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
		va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N", FW_CONTROL_PACKET_PROTECTION);
		va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P", FW_CONTROL_PACKET_PROTECTION, "RETURN");
		va_cmd(EBTABLES, 6, 1, "-t", "broute", "-A", "BROUTING", "-j", FW_CONTROL_PACKET_PROTECTION);
#endif
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", QOS_IPTABLE_US_ACTION_CHAIN);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", QOS_IPTABLE_US_ACTION_CHAIN);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", QOS_IP_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", QOS_IP_CHAIN);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", "pvc_mark");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", "pvc_mark");

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", "qos_rule_output");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "OUTPUT", "-j", "qos_rule_output");

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", "pvc_mark_output");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "OUTPUT", "-j", "pvc_mark_output");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", QOS_IPTABLE_US_ACTION_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", QOS_IPTABLE_US_ACTION_CHAIN);
#endif
	}
	else
	{
		va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", QOS_EB_US_CHAIN);
		va_cmd(EBTABLES, 6, 1, "-t", "broute", "-D", "BROUTING", "-j", QOS_EB_US_CHAIN);
		va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X", QOS_EB_US_CHAIN);

		va_cmd(EBTABLES, 4, 1, "-t", "nat", "-F", QOS_EB_DS_CHAIN);
		va_cmd(EBTABLES, 6, 1, "-t", "nat", "-D", "POSTROUTING", "-j", QOS_EB_DS_CHAIN);
		va_cmd(EBTABLES, 4, 1, "-t", "nat", "-X", QOS_EB_DS_CHAIN);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", QOS_IP_CHAIN);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark_output");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "qos_rule_output");

		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", "pvc_mark");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", QOS_IP_CHAIN);

		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "OUTPUT", "-j", "pvc_mark_output");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "OUTPUT", "-j", "qos_rule_output");

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", "pvc_mark");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", QOS_IP_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", "pvc_mark_output");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", "qos_rule_output");

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", QOS_IPTABLE_US_ACTION_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", QOS_IPTABLE_DS_ACTION_CHAIN);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", QOS_IPTABLE_US_ACTION_CHAIN);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", QOS_IPTABLE_DS_ACTION_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", QOS_IPTABLE_US_ACTION_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", QOS_IPTABLE_DS_ACTION_CHAIN);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", QOS_IPTABLE_US_ACTION_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", QOS_IPTABLE_DS_ACTION_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", QOS_IPTABLE_US_ACTION_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", QOS_IPTABLE_DS_ACTION_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", QOS_IPTABLE_US_ACTION_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", QOS_IPTABLE_DS_ACTION_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", QOS_IP_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark_output");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "qos_rule_output");

		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", "pvc_mark");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", QOS_IP_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "OUTPUT", "-j", "pvc_mark_output");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "OUTPUT", "-j", "qos_rule_output");

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", "pvc_mark");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", QOS_IP_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", "pvc_mark_output");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", "qos_rule_output");
#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
		va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", FW_CONTROL_PACKET_PROTECTION);
		va_cmd(EBTABLES, 6, 1, "-t", "broute", "-D", "BROUTING", "-j", FW_CONTROL_PACKET_PROTECTION);
		va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X", FW_CONTROL_PACKET_PROTECTION);
#endif
#endif
	}
}

static int setupQosRuleChain(unsigned int enable)
{
	MIB_CE_IP_QOS_T qosEntry;
	char chainName[64];
	int i, qosEntryNum;
	char cmd[256] = {0};
    unsigned char origVal;
    char wan_ip[16] = {0};
    FILE *fp = NULL; 
    char line[16] = {0};
    int num = 0;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	int vcEntryNum, j;
	MIB_CE_IP_QOS_T qEntry;

#endif
	QOS_SETUP_PRINT_FUNCTION

	if (enable)
	{
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		// not support	ds,rtp,pvc
		if(current_qos_wan_port < 0){
			printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
			return 0;
		}
		va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N", QOS_EB_US_CHAIN[current_qos_wan_port]);
		va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P", QOS_EB_US_CHAIN[current_qos_wan_port], "RETURN");
		va_cmd(EBTABLES, 6, 1, "-t", "broute", "-A", "BROUTING", "-j", QOS_EB_US_CHAIN[current_qos_wan_port]);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", QOS_IP_CHAIN[current_qos_wan_port]);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", QOS_IP_CHAIN[current_qos_wan_port]);
#endif	
		qosEntryNum = mib_chain_total(MIB_IP_QOS_TBL);
		for(i=0 ; i<qosEntryNum ; i++)
		{
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
			memset(&qEntry,0,sizeof(qEntry));
			if(!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)&qEntry) || qEntry.logic_wan_port != current_qos_wan_port)
				continue;
			snprintf(chainName, sizeof(chainName),"%s_%d", QOS_IP_CHAIN[current_qos_wan_port], i);
#else
			sprintf(chainName, "%s_%d", QOS_IP_CHAIN, i);
#endif
			va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", chainName);
		}

#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", QOS_IP_CHAIN[current_qos_wan_port]);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", QOS_IP_CHAIN[current_qos_wan_port]);
#endif
		for(i=0 ; i<qosEntryNum ; i++)
		{
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
			snprintf(chainName, sizeof(chainName),"%s_%d", QOS_IP_CHAIN[current_qos_wan_port], i);
#else
			sprintf(chainName, "%s_%d", QOS_IP_CHAIN, i);
#endif
			va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", chainName);
		}
#endif
	} else {
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		for(j=0;j<SW_LAN_PORT_NUM;j++){
			va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", QOS_EB_US_CHAIN[j]);
			va_cmd(EBTABLES, 6, 1, "-t", "broute", "-D", "BROUTING", "-j", QOS_EB_US_CHAIN[j]);
			va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X", QOS_EB_US_CHAIN[j]);

			va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", QOS_IP_CHAIN[j]);
			va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", QOS_IP_CHAIN[j]);
			va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", QOS_IP_CHAIN[j]);
		}
		for(j=0;j<SW_LAN_PORT_NUM;j++){
			for(i=0 ; i<MAX_QOS_RULE ; i++){
				snprintf(chainName, sizeof(chainName),"%s_%d", QOS_IP_CHAIN[j], i);
				snprintf(cmd, sizeof(cmd), "%s -t mangle -F %s > /dev/null 2>&1", IPTABLES, chainName);
				va_cmd("/bin/sh", 2, 1, "-c", cmd);
				snprintf(cmd, sizeof(cmd), "%s -t mangle -X %s > /dev/null 2>&1", IPTABLES, chainName);
				va_cmd("/bin/sh", 2, 1, "-c", cmd);
			}
		}
#else
		va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", QOS_EB_US_CHAIN);
		va_cmd(EBTABLES, 4, 1, "-t", "nat", "-F", QOS_EB_DS_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", QOS_IP_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark_output");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "qos_rule_output");

		for(i=0 ; i<MAX_QOS_RULE ; i++)
		{
			sprintf(chainName, "%s_%d", QOS_IP_CHAIN, i);
			snprintf(cmd, sizeof(cmd), "%s -t mangle -F %s > /dev/null 2>&1", IPTABLES, chainName);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
			snprintf(cmd, sizeof(cmd), "%s -t mangle -X %s > /dev/null 2>&1", IPTABLES, chainName);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
		}
#endif

#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		for(j=0;j<SW_LAN_PORT_NUM;j++){
			va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", QOS_IP_CHAIN[j]);
			va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", QOS_IP_CHAIN[j]);
			va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", QOS_IP_CHAIN[j]);
		}
		for(j=0;j<SW_LAN_PORT_NUM;j++){
			for(i=0 ; i<MAX_QOS_RULE ; i++){
				snprintf(chainName, sizeof(chainName),"%s_%d", QOS_IP_CHAIN[j], i);
				snprintf(cmd, sizeof(cmd), "%s -t mangle -F %s > /dev/null 2>&1", IP6TABLES, chainName);
				va_cmd("/bin/sh", 2, 1, "-c", cmd);
				snprintf(cmd, sizeof(cmd), "%s -t mangle -X %s > /dev/null 2>&1", IP6TABLES, chainName);
				va_cmd("/bin/sh", 2, 1, "-c", cmd);
			}
		}
#else
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", QOS_IP_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark_output");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "qos_rule_output");
		for(i=0 ; i<MAX_QOS_RULE ; i++)
		{
			sprintf(chainName, "%s_%d", QOS_IP_CHAIN, i);
			snprintf(cmd, sizeof(cmd), "%s -t mangle -F %s > /dev/null 2>&1", IP6TABLES, chainName);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
			snprintf(cmd, sizeof(cmd), "%s -t mangle -X %s > /dev/null 2>&1", IP6TABLES, chainName);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
		}
#endif
#endif
#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
		va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", FW_CONTROL_PACKET_PROTECTION);
#endif
	}

    mib_get(MIB_OP_MODE, (void *)&origVal);
    if (origVal != BRIDGE_MODE) {
        // using unique comment string
        system("iptables -nvL --line-numbers | grep 80_PORT_WAN | awk '{print $1}' > /tmp/droprule");

        if ((fp = fopen(PORT_RULE_FILE, "r")) != NULL) {
            fscanf(fp, "%d", &num);
            sprintf(line, "iptables -D INPUT %d", num);
            system(line);
            fclose(fp);
            unlink(PORT_RULE_FILE);
        }

        get_ipv4_from_interface(wan_ip, 2);

        // when add rule, use comment option "80_PORT_WAN"
        va_cmd(IPTABLES, 16, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-i", "nas0_0", "-p", "tcp", "-d", 
                wan_ip, (char *)FW_DPORT, "80", "-m", "comment", "--comment", "80_PORT_WAN", "-j", "DROP");
    }

	return 1;
}

static int delete_ALL_WAN_default_qdisc()
{
	MIB_CE_ATM_VC_T vcEntry;
	int i, vcEntryNum;
	
	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<vcEntryNum; i++)
	{
		char devname[IFNAMSIZ];
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry))
			continue;

		ifGetName(PHY_INTF(vcEntry.ifIndex), devname, sizeof(devname));
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", devname, "root");
	}
	return 0;
}

static int delete_ALL_LAN_default_qdisc()
{
	va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", LANIF, "root");
	return 0;
}

static int setup_prio_queue(int enable)
{
	// Mason Yu. Use device name not imq0
#ifdef QOS_SETUP_IFB
	char *imq0_devname="ifb0";
#else
	char *imq0_devname="imq0";
#endif
	
	//char devname[IFNAMSIZ];
	MIB_CE_ATM_VC_T vcEntry;
	int i, vcEntryNum;
	char s_rate[16], s_ceil[16];
	char tc_cmd[100];
	char tc_cmd2[200];
	unsigned int total_bandwidth = 0;

	QOS_SETUP_PRINT_FUNCTION

	if(!enable){
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN 
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", imq0_devname, "root");
#else
		// flush all root wans like eth2.0
		for(i=0;i<SW_LAN_PORT_NUM; i++){ 
			if(!(imq0_devname=(char*)getQosRootWanName(i)))
				return 0;
			va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", imq0_devname, "root");
		}
#endif
		delete_ALL_WAN_default_qdisc();
		// disable IPQoS
		__dev_setupIPQoS(0);
		hwnat_qos_bandwidth_setup(0);
		return 0;
	}
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(current_qos_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
	if(!(imq0_devname=(char*)getQosRootWanName(current_qos_wan_port)))
		return 0;
#endif
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
	va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"root", "handle", "1:", "htb", "default", "10");
#else
	//tc qdisc add dev $DEV root handle 1: htb default 400
	va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"root", "handle", "1:", "htb", "default", "100");
#endif

	//set total_bandwidth limit in imq0, and let packet be mirred to imq0 by tc filter or iptables rule	
	total_bandwidth = getUpLinkRate();
	hwnat_qos_bandwidth_setup(total_bandwidth*1024);
	snprintf(s_rate, 16, "%dKbit", total_bandwidth);
	snprintf(s_ceil, 16, "%dKbit", total_bandwidth);
	va_cmd(TC, 13, 1, "class", "add", "dev", imq0_devname,
		"parent", "1:", "classid", "1:100", "htb", "rate", s_rate, "ceil", s_ceil);

#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
	snprintf(tc_cmd, 100, "tc qdisc add dev %s parent 1:100 handle 2: prio bands 8 priomap %s", imq0_devname, "7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7");
#else
	snprintf(tc_cmd, 100, "tc qdisc add dev %s parent 1:100 handle 2: prio bands 4 priomap %s", imq0_devname, "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3");
	#endif
	
	TRACE(STA_SCRIPT,"%s\n",tc_cmd);
	system(tc_cmd);
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"parent", "2:1", "handle", "3:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"parent", "2:2", "handle", "4:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"parent", "2:3", "handle", "5:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"parent", "2:4", "handle", "6:", "pfifo", "limit", "100");
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"parent", "2:5", "handle", "7:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"parent", "2:6", "handle", "8:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"parent", "2:7", "handle", "9:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"parent", "2:8", "handle", "10:", "pfifo", "limit", "100");
	#endif
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<vcEntryNum; i++)
	{
		char devname[IFNAMSIZ];
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry))
#else
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry)||!vcEntry.enableIpQos)
	#endif
			continue;

		//if (vcEntry.cmode != CHANNEL_MODE_BRIDGE)
		//	continue;

		ifGetName(PHY_INTF(vcEntry.ifIndex), devname, sizeof(devname));

		// delete default prio qdisc
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", devname, "root");
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
		snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 8 priomap %s", devname, "7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7");
#else
		snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 4 priomap %s", devname, "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3");
	#endif
		TRACE(STA_SCRIPT,"%s\n",tc_cmd);
		system(tc_cmd);
		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
			"parent", "1:1", "handle", "2:", "pfifo", "limit", "100");
		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
			"parent", "1:2", "handle", "3:", "pfifo", "limit", "100");
		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
			"parent", "1:3", "handle", "4:", "pfifo", "limit", "100");
		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
			"parent", "1:4", "handle", "5:", "pfifo", "limit", "100");
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
			"parent", "1:5", "handle", "6:", "pfifo", "limit", "100");
		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
			"parent", "1:6", "handle", "7:", "pfifo", "limit", "100");
		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
			"parent", "1:7", "handle", "8:", "pfifo", "limit", "100");
		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
			"parent", "1:8", "handle", "9:", "pfifo", "limit", "100");
	#endif
	
#ifdef QOS_SETUP_IFB	
		
		// tc filter add dev nas0_0 parent 1: protocol ip u32 match u32 0 0 flowid 1:4 caction mirred egress redirect dev ifb0
		//Use "action mirred egress redirect" to do 2 layer QoS. One is on ifb0, another is on physical device(nas0_0).
		// This rule is for not match the other filters on the physical device(nas0_0) and will match this rule .
		// Match packets will demonstrate is the following sequence
		// (1) Packet is classified as class 1:4 and redirected to ifb0(the first QoS).
		// (2) Packet will come back to physical device(nas0_0)(the second QoS).
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
		snprintf(tc_cmd2, 200, "tc filter add dev %s parent 1: protocol all u32 match u32 0 0 flowid 1:8 action mirred egress redirect dev %s", devname, imq0_devname);
	#elif defined(CONFIG_USER_AP_QOS)
		// in case for pppoe traffic
		snprintf(tc_cmd2, 200, "tc filter add dev %s parent 1: protocol all u32 match u32 0 0 flowid 1:4 action mirred egress redirect dev %s", devname, imq0_devname);	
	#else
		snprintf(tc_cmd2, 200, "tc filter add dev %s parent 1: protocol ip u32 match u32 0 0 flowid 1:4 action mirred egress redirect dev %s", devname, imq0_devname);		
	#endif
		TRACE(STA_SCRIPT,"%s\n",tc_cmd2);
		system(tc_cmd2);
#endif
	}
#endif

	// enable IPQoS
	__dev_setupIPQoS(1);
	
	return 0;
}

static int setup_downstream_prio_queue(int enable)
{
	// Mason Yu. Use device name not imq0
#ifdef QOS_SETUP_IFB
	char *imq1_devname="ifb1";
#else
	char *imq1_devname="imq1";
#endif
	
	//char devname[IFNAMSIZ];
	char s_rate[16], s_ceil[16];
	char tc_cmd[100];
	char tc_cmd2[200];
	unsigned int total_bandwidth = 0;

	QOS_SETUP_PRINT_FUNCTION

	if(!enable){
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", imq1_devname, "root");
		delete_ALL_LAN_default_qdisc();
		// disable IPQoS
		__dev_setupIPQoS(0);
		hwnat_qos_bandwidth_setup(0);
		return 0;
	}

#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
	va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", imq1_devname,
		"root", "handle", "1:", "htb", "default", "10");
#else
	//tc qdisc add dev $DEV root handle 1: htb default 400
	va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", imq1_devname,
		"root", "handle", "1:", "htb", "default", "100");
#endif

	//set total_bandwidth limit in imq0, and let packet be mirred to imq0 by tc filter or iptables rule	
	total_bandwidth = getDownLinkRate();
	snprintf(s_rate, 16, "%dKbit", total_bandwidth);
	snprintf(s_ceil, 16, "%dKbit", total_bandwidth);
	va_cmd(TC, 13, 1, "class", "add", "dev", imq1_devname,
		"parent", "1:", "classid", "1:100", "htb", "rate", s_rate, "ceil", s_ceil);

#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
	snprintf(tc_cmd, 100, "tc qdisc add dev %s parent 1:100 handle 2: prio bands 8 priomap %s", imq1_devname, "7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7");
#else
	snprintf(tc_cmd, 100, "tc qdisc add dev %s parent 1:100 handle 2: prio bands 4 priomap %s", imq1_devname, "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3");
#endif
	
	TRACE(STA_SCRIPT,"%s\n",tc_cmd);
	system(tc_cmd);
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq1_devname,
		"parent", "2:1", "handle", "3:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq1_devname,
		"parent", "2:2", "handle", "4:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq1_devname,
		"parent", "2:3", "handle", "5:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq1_devname,
		"parent", "2:4", "handle", "6:", "pfifo", "limit", "100");
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq1_devname,
		"parent", "2:5", "handle", "7:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq1_devname,
		"parent", "2:6", "handle", "8:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq1_devname,
		"parent", "2:7", "handle", "9:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq1_devname,
		"parent", "2:8", "handle", "10:", "pfifo", "limit", "100");
#endif
	// delete default prio qdisc
	va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", LANIF, "root");
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
	snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 8 priomap %s", LANIF, "7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7");
#else
	snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 4 priomap %s", LANIF, "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3");
#endif
	TRACE(STA_SCRIPT,"%s\n",tc_cmd);
	system(tc_cmd);
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF,
		"parent", "1:1", "handle", "2:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF,
		"parent", "1:2", "handle", "3:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF,
		"parent", "1:3", "handle", "4:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF,
		"parent", "1:4", "handle", "5:", "pfifo", "limit", "100");
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF,
		"parent", "1:5", "handle", "6:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF,
		"parent", "1:6", "handle", "7:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF,
		"parent", "1:7", "handle", "8:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", LANIF,
		"parent", "1:8", "handle", "9:", "pfifo", "limit", "100");
#endif

#ifdef QOS_SETUP_IFB	
	
	// tc filter add dev nas0_0 parent 1: protocol ip u32 match u32 0 0 flowid 1:4 caction mirred egress redirect dev ifb0
	//Use "action mirred egress redirect" to do 2 layer QoS. One is on ifb0, another is on physical device(nas0_0).
	// This rule is for not match the other filters on the physical device(nas0_0) and will match this rule .
	// Match packets will demonstrate is the following sequence
	// (1) Packet is classified as class 1:4 and redirected to ifb0(the first QoS).
	// (2) Packet will come back to physical device(nas0_0)(the second QoS).
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
	snprintf(tc_cmd2, 200, "tc filter add dev %s parent 1: protocol all u32 match u32 0 0 flowid 1:8 action mirred egress redirect dev %s", LANIF, imq1_devname);
#elif defined(CONFIG_USER_AP_QOS)
	// in case for pppoe traffic
	snprintf(tc_cmd2, 200, "tc filter add dev %s parent 1: protocol all u32 match u32 0 0 flowid 1:4 action mirred egress redirect dev %s", LANIF, imq1_devname);	
#else
	snprintf(tc_cmd2, 200, "tc filter add dev %s parent 1: protocol ip u32 match u32 0 0 flowid 1:4 action mirred egress redirect dev %s", LANIF, imq1_devname);		
#endif
	TRACE(STA_SCRIPT,"%s\n",tc_cmd2);
	system(tc_cmd2);
#endif

	// enable IPQoS
	__dev_setupIPQoS(1);
	
	return 0;
}

/*
 *	action:
 *	0: Add
 *	1: Change
 */
static int setup_htb_class(int action, unsigned int upRate)
{
#ifdef QOS_SETUP_IFB
	char *devname="ifb0";
#else
	char *devname="imq0";
#endif
	char s_rate[16], s_ceil[16], s_classid[16], s_quantum[16];
	const char *pAction;
	int j;
	int rate, ceil, quantum;
	unsigned char enableForceWeight=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, minWeigh;
	int totalWeight = SumWeightOfWRR();
	int qos_queue_num = getIPQosQueueNumber();
	if (action == 0)
		pAction = ARG_ADD;
	else
		pAction = ARG_CHANGE;
	
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(current_qos_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
	if(!(devname=(char*)getQosRootWanName(current_qos_wan_port)))
		return 0;
#endif

	snprintf(s_rate, 16, "%dKbit", upRate);
	snprintf(s_ceil, 16, "%dKbit", upRate);
	//tc class add dev $DEV parent 1: classid 1:1 htb rate $RATE ceil $CEIL
	va_cmd(TC, 13, 1, "class", pAction, "dev", devname,
		"parent", "1:", "classid", "1:1", "htb", "rate", s_rate, "ceil", s_ceil);
#if defined(_PRMT_X_CT_COM_QOS_)
	mib_get_s(MIB_QOS_ENABLE_FORCE_WEIGHT, (void *)&enableForceWeight, sizeof(enableForceWeight));
#endif
	if((qEntryNum=mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		return 0;
	for(j=0;j<qEntryNum; j++)
	{
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, j, (void*)&qEntry))
			continue;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(qEntry.q_logic_wan_port != current_qos_wan_port)
			continue;
#endif
		if ( !qEntry.enable
		#ifdef CONFIG_USER_AP_E8B_QOS
			&& (j != (MAX_QOS_QUEUE_NUM-1)) 	// remain lowest queue as default
		#endif
		)
			continue;
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		snprintf(s_classid, 16, "1:%d00", j+1);
#else
		if(j-current_qos_wan_port*MAX_QOS_QUEUE_NUM>=0)
			snprintf(s_classid, 16, "1:%d00", (j-current_qos_wan_port*MAX_QOS_QUEUE_NUM)+1);
		else{
			printf("%s:%d invalid s_classid!\n",__FUNCTION__,__LINE__);
			return 0;
		}
#endif
		minWeigh = findQueueMinWeigh();
		// Quantum should be bigger then MTU (1500) so you can send the maximum packet in 1 turn and smaller then 60000
		if(minWeigh == 0)
			quantum = 0;
		else			
#ifdef CONFIG_USER_AP_QOS
			quantum = (3000*qEntry.weight)/minWeigh;
#else
			quantum = (1500*qEntry.weight)/minWeigh;
#endif
		if (quantum > 60000)
			quantum = 60000;
		snprintf(s_quantum, 16, "%d", quantum);
		//printf("s_quantum=%s\n", s_quantum);
		rate = qEntry.weight;
		//if enableForceWeight, then send rate should not larger than wrr proportion
#if defined(_PRMT_X_CT_COM_QOS_)
		ceil = enableForceWeight?(upRate*qEntry.weight/100):upRate;
#else
		ceil = upRate;
#endif
		snprintf(s_ceil, 16, "%dKbit", ceil);
		if (rate > ceil)
		{
#if defined(_PRMT_X_CT_COM_QOS_)
			if (enableForceWeight)
				rate = ceil;
			else
				rate = ceil*qEntry.weight/totalWeight;
#else
			rate = ceil*qEntry.weight/totalWeight;
#endif
		}
		snprintf(s_rate, 16, "%dKbit", rate);

		//tc class add dev $DEV parent 10:1 classid 10:$SUBID htb rate $RATE ceil $RATE prio $PRIO
		va_cmd(TC, 15, 1, "class", pAction, "dev", devname,
			"parent", "1:1", "classid", s_classid, "htb", "rate",
			s_rate, "ceil", s_ceil, "quantum", s_quantum);
			
#ifdef _PRMT_X_CT_COM_QOS_
#ifdef CONFIG_USER_AP_HW_QOS
		if(enableForceWeight && ceil) {
			rtk_ap_qos_queue_shaping_set(qos_wan_port, qos_queue_num-qEntry.prior, 1, ceil);
		}
#endif
#endif
	}
	return 0;
}

static int setup_downstream_htb_class(int action, unsigned int upRate)
{
#ifdef QOS_SETUP_IFB
	char *devname="ifb1";
#else
	char *devname="imq1";
#endif
	char s_rate[16], s_ceil[16], s_classid[16], s_quantum[16];
	const char *pAction;
	int j;
	int rate, ceil, quantum;
	unsigned char enableForceWeight=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, minWeigh;
	int totalWeight = DownstreamSumWeightOfWRR();

	if (action == 0)
		pAction = ARG_ADD;
	else
		pAction = ARG_CHANGE;
	snprintf(s_rate, 16, "%dKbit", upRate);
	snprintf(s_ceil, 16, "%dKbit", upRate);
	//tc class add dev $DEV parent 1: classid 1:1 htb rate $RATE ceil $CEIL
	va_cmd(TC, 13, 1, "class", pAction, "dev", devname,
		"parent", "1:", "classid", "1:1", "htb", "rate", s_rate, "ceil", s_ceil);
#if defined(_PRMT_X_CT_COM_QOS_)
	mib_get_s(MIB_QOS_ENABLE_FORCE_WEIGHT, (void *)&enableForceWeight, sizeof(enableForceWeight));
#endif
	if((qEntryNum=mib_chain_total(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL)) <=0)
		return 0;
	for(j=0;j<qEntryNum; j++)
	{
		if(!mib_chain_get(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL, j, (void*)&qEntry))
			continue;
		if ( !qEntry.enable
		#ifdef CONFIG_USER_AP_E8B_QOS
			&& (j != (MAX_QOS_QUEUE_NUM-1)) 	// remain lowest queue as default
		#endif
		)
			continue;

		snprintf(s_classid, 16, "1:%d00", j+1);
		minWeigh = findDownstreamQueueMinWeigh();
		// Quantum should be bigger then MTU (1500) so you can send the maximum packet in 1 turn and smaller then 60000
		if(minWeigh == 0)
			quantum = 0;
		else
			quantum = (1500*qEntry.weight)/minWeigh;
		if (quantum > 60000)
			quantum = 60000;
		snprintf(s_quantum, 16, "%d", quantum);
		//printf("s_quantum=%s\n", s_quantum);
		rate = qEntry.weight;
		//if enableForceWeight, then send rate should not larger than wrr proportion
#if defined(_PRMT_X_CT_COM_QOS_)
		ceil = enableForceWeight?(upRate*qEntry.weight/100):upRate;
#else
		ceil = upRate;
#endif
		snprintf(s_ceil, 16, "%dKbit", ceil);
		if (rate > ceil)
		{
#if defined(_PRMT_X_CT_COM_QOS_)
			if (enableForceWeight)
				rate = ceil;
			else
				rate = ceil*qEntry.weight/totalWeight;
#else
			rate = ceil*qEntry.weight/totalWeight;
#endif
		}
		snprintf(s_rate, 16, "%dKbit", rate);

		//tc class add dev $DEV parent 10:1 classid 10:$SUBID htb rate $RATE ceil $RATE prio $PRIO
		va_cmd(TC, 15, 1, "class", pAction, "dev", devname,
			"parent", "1:1", "classid", s_classid, "htb", "rate",
			s_rate, "ceil", s_ceil, "quantum", s_quantum);
	}
	return 0;
}

static int setup_wrr_queue(int enable)
{
	int j;
#ifdef QOS_SETUP_IFB
	char *imq0_devname="ifb0";
#else
	char *imq0_devname="imq0";
#endif
	char s_rate[16], s_ceil[16], s_classid[16], s_quantum[16];
	unsigned int total_bandwidth = 0;

	MIB_CE_ATM_VC_T vcEntry;
	int i, vcEntryNum;
	char tc_cmd[100];
	char tc_cmd2[200];
	char devname2[IFNAMSIZ];

	QOS_SETUP_PRINT_FUNCTION

	if (!enable) {
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", imq0_devname, "root");
#else
		// flush all root wans like eth0.2
		for(i=0;i<SW_LAN_PORT_NUM; i++){ 
			if(!(imq0_devname=(char*)getQosRootWanName(i)))
				return 0;
			va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", imq0_devname, "root");
		}
#endif
		delete_ALL_WAN_default_qdisc();
		// disable IPQoS
		__dev_setupIPQoS(0);	
		return 0;
	}
#if defined(CONFIG_E8B)
	if (!ifSumWeighIs100())
		return 0;
#endif
	total_bandwidth = getUpLinkRate();
	current_uprate = total_bandwidth;
	hwnat_qos_bandwidth_setup(total_bandwidth*1024);

	snprintf(s_rate, 16, "%dKbit", total_bandwidth);
	snprintf(s_ceil, 16, "%dKbit", total_bandwidth);
	
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(current_qos_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
	if(!(imq0_devname=(char*)getQosRootWanName(current_qos_wan_port)))
		return 0;
#endif

#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
	va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"root", "handle", "1:", "htb", "default", "800");
#else
	//tc qdisc add dev $DEV root handle 1: htb default 400
	va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"root", "handle", "1:", "htb", "default", "400");
#endif

	setup_htb_class(0, total_bandwidth);
	//set queue len
	set_wrr_class_qdisc();
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN	
#ifdef QOS_SETUP_IFB	
	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<vcEntryNum; i++)
	{
		char devname[IFNAMSIZ];
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry))
#else
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry)||!vcEntry.enableIpQos)
	#endif
			continue;

	#if defined(CONFIG_USER_AP_QOS)
		// qdisc builds in nas instead of ppp device
		ifGetName(PHY_INTF(vcEntry.ifIndex), devname, sizeof(devname));
	#else
		ifGetName(vcEntry.ifIndex, devname, sizeof(devname));
	#endif

		// delete default prio qdisc
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", devname, "root");
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
		snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 8 priomap %s", devname, "7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7");
#else
		snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 4 priomap %s", devname, "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3");
	#endif
		TRACE(STA_SCRIPT,"%s\n",tc_cmd);
		system(tc_cmd);		
	
		// tc filter add dev nas0_0 parent 1: protocol ip u32 match u32 0 0 action mirred egress redirect dev ifb0
	#if defined(CONFIG_USER_AP_QOS)
		// in case for pppoe traffic
		snprintf(tc_cmd2, 200, "tc filter add dev %s parent 1: protocol all u32 match u32 0 0 action mirred egress redirect dev %s", devname, imq0_devname);
	#else
		snprintf(tc_cmd2, 200, "tc filter add dev %s parent 1: protocol ip u32 match u32 0 0 action mirred egress redirect dev %s", devname, imq0_devname);
	#endif
		TRACE(STA_SCRIPT,"%s\n",tc_cmd2);
		system(tc_cmd2);
	}
#endif


#ifdef QOS_SETUP_IMQ
	// Mason Yu.
	// If the vc is bridge mode , set root qdisc for tc action mirred
	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<vcEntryNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry)||!vcEntry.enableIpQos)
			continue;

		if (vcEntry.cmode != CHANNEL_MODE_BRIDGE)
			continue;

		ifGetName(PHY_INTF(vcEntry.ifIndex), devname2, sizeof(devname2));

		// delete default prio qdisc
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", devname2, "root");
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
		snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 8 priomap %s", devname2, "7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7");
#else
		snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 4 priomap %s", devname2, "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3");
#endif
		TRACE(STA_SCRIPT,"%s\n",tc_cmd);
		system(tc_cmd);
	}
#endif
#endif
	// enable IPQoS
	__dev_setupIPQoS(1);	
	return 0;
}

static int setup_downstream_wrr_queue(int enable)
{
	int j;
#ifdef QOS_SETUP_IFB
	char *imq1_devname="ifb1";
#else
	char *imq1_devname="imq1";
#endif
	char s_rate[16], s_ceil[16], s_classid[16], s_quantum[16];
	unsigned int total_bandwidth = 0;
	char tc_cmd[100];
	char tc_cmd2[200];
	char devname2[IFNAMSIZ];

	QOS_SETUP_PRINT_FUNCTION

	if (!enable) {
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", imq1_devname, "root");
		delete_ALL_LAN_default_qdisc();
		// disable IPQoS
		__dev_setupIPQoS(0);	
		return 0;
	}
#if defined(CONFIG_E8B)
	if (!ifDownstreamSumWeighIs100())
		return 0;
#endif
	total_bandwidth = getDownLinkRate();
	current_uprate = total_bandwidth;
	hwnat_qos_bandwidth_setup(total_bandwidth*1024);

	snprintf(s_rate, 16, "%dKbit", total_bandwidth);
	snprintf(s_ceil, 16, "%dKbit", total_bandwidth);

#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
	va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", imq1_devname,
		"root", "handle", "1:", "htb", "default", "800");
#else
	//tc qdisc add dev $DEV root handle 1: htb default 400
	va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", imq1_devname,
		"root", "handle", "1:", "htb", "default", "400");
#endif

#ifdef CONFIG_QOS_SUPPORT_DOWNSTREAM
	setup_downstream_htb_class(0, total_bandwidth);
	//set queue len
	set_downstream_wrr_class_qdisc();
#endif
	
#ifdef QOS_SETUP_IFB	
	// delete default prio qdisc
	va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", LANIF, "root");
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
	snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 8 priomap %s", LANIF, "7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7");
#else
	snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 4 priomap %s", LANIF, "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3");
#endif
	TRACE(STA_SCRIPT,"%s\n",tc_cmd);
	system(tc_cmd);		

	// tc filter add dev nas0_0 parent 1: protocol ip u32 match u32 0 0 action mirred egress redirect dev ifb0
#if defined(CONFIG_USER_AP_QOS)
	// in case for pppoe traffic
	snprintf(tc_cmd2, 200, "tc filter add dev %s parent 1: protocol all u32 match u32 0 0 action mirred egress redirect dev %s", LANIF, imq1_devname);
#else
	snprintf(tc_cmd2, 200, "tc filter add dev %s parent 1: protocol ip u32 match u32 0 0 action mirred egress redirect dev %s", LANIF, imq1_devname);
#endif
	TRACE(STA_SCRIPT,"%s\n",tc_cmd2);
	system(tc_cmd2);
#endif


#ifdef QOS_SETUP_IMQ
	// delete default prio qdisc
	va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", LANIF, "root");
#if defined(CONFIG_QOS_SUPPORT_8_QUEUES)
		snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 8 priomap %s", devname2, "7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7");
#else
	snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 4 priomap %s", devname2, "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3");
	#endif
		TRACE(STA_SCRIPT,"%s\n",tc_cmd);
		system(tc_cmd);
#endif

	// enable IPQoS
	__dev_setupIPQoS(1);	
	return 0;
}

static int setupQdisc(int enable)
{
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	unsigned char policy;
#else
	unsigned char policy[QOS_MAX_PORT_NUM] = {0};
#endif
	QOS_SETUP_PRINT_FUNCTION;
	
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(!enable){
		//flush is not based on port
		setup_prio_queue(enable);
		setup_wrr_queue(enable);
		return 0;
	}
	else{
		if(current_qos_wan_port < 0){
			printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
			return 0;
		}
		mib_get_s(MIB_QOS_POLICY, (void*)&policy, sizeof(policy));
	}
#else
	mib_get_s(MIB_QOS_POLICY, (void*)&policy, sizeof(policy));
#endif
	// move to setupIPQ()
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if (policy == PLY_PRIO)
#else
	if (policy[current_qos_wan_port] == PLY_PRIO)
#endif
	{
		setup_prio_queue(enable);
#ifdef CONFIG_QOS_SUPPORT_DOWNSTREAM
		setup_downstream_prio_queue(enable);
#endif
	}
	else
	{
		setup_wrr_queue(enable);
#ifdef CONFIG_QOS_SUPPORT_DOWNSTREAM
		setup_downstream_wrr_queue(enable);
#endif
	}
	return 0;
}

static int setupDefaultQdisc(int enable)
{
	MIB_CE_ATM_VC_T vcEntry;
	int i, vcEntryNum;
	char tc_cmd[100];
	
	QOS_SETUP_PRINT_FUNCTION;
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<vcEntryNum; i++)
	{
		char devname[IFNAMSIZ];
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry))
			continue;

		ifGetName(PHY_INTF(vcEntry.ifIndex), devname, sizeof(devname));

		if (!enable) {
			va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", devname, "root");
			continue;
		}

		snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio", devname);
		TRACE(STA_SCRIPT,"%s\n",tc_cmd);
		system(tc_cmd);

		if (devname[0] == 'v') {
			snprintf(tc_cmd, 100, "tc qdisc add dev %s parent 1:1 handle 10: bfifo limit 6000", devname);
		        system(tc_cmd);
			snprintf(tc_cmd, 100, "tc qdisc add dev %s parent 1:2 handle 20: bfifo limit 6000", devname);
		        system(tc_cmd);
			snprintf(tc_cmd, 100, "tc qdisc add dev %s parent 1:3 handle 30: bfifo limit 6000", devname);
		        system(tc_cmd);
		}
	}
#endif
	return 0;
}

static int setupQRule()
{
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	unsigned char policy;
#else
	unsigned char policy[QOS_MAX_PORT_NUM] = {0};
#endif
	QOS_SETUP_PRINT_FUNCTION
	mib_get_s(MIB_QOS_POLICY, (void*)&policy, sizeof(policy));
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(current_qos_wan_port < 0){
			printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
			return 0;
		}
	setup_qos_rules(policy[current_qos_wan_port]);	
#else
	setup_qos_rules(policy);
#endif
	return 0;
}

void restartQRule(void)
{
	//clean rules
	setupQosRuleChain(0);

	//setup rules
	setupQosRuleChain(1);
	setupQRule();

	rtk_hwnat_flow_flush();
}

int enableIMQ(int enable)
{
	MIB_CE_ATM_VC_T vcEntry;
	int i, entryNum;
	char ifname[IFNAMSIZ];

	QOS_SETUP_PRINT_FUNCTION

	if (!enable) {
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "IMQ_CHAIN");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", "IMQ_CHAIN");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", "IMQ_CHAIN");
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "IMQ_CHAIN");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", "IMQ_CHAIN");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", "IMQ_CHAIN");
#endif
		va_cmd(IFCONFIG, 2, 1, "imq0", "down");
		va_cmd(IFCONFIG, 2, 1, "imq1", "down");
		return 0;
	}

	va_cmd(IFCONFIG, 3, 1, "imq0", "txqueuelen", "100");
	va_cmd(IFCONFIG, 2, 1, "imq0", "up");
	va_cmd(IFCONFIG, 3, 1, "imq1", "txqueuelen", "100");
	va_cmd(IFCONFIG, 2, 1, "imq1", "up");

	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", "IMQ_CHAIN");
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", "IMQ_CHAIN");
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", "IMQ_CHAIN");
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", "IMQ_CHAIN");
#endif

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);

	for(i=0; i<entryNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry) || !vcEntry.enableIpQos)
			continue;

		if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
			continue;

		ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
		va_cmd(IPTABLES, 10, 1, "-t", "mangle", "-A", "IMQ_CHAIN",
			"-o", ifname, "-j", "IMQ", "--todev", "0");
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 10, 1, "-t", "mangle", "-A", "IMQ_CHAIN",
			"-o", ifname, "-j", "IMQ", "--todev", "0");
#endif
	}

	return 0;
}

/*
 * Make sure we have 4 QoS Queues.
 */
static void checkQosQueue()
{
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int i;

	if(mib_chain_total(MIB_IP_QOS_QUEUE_TBL) >= 4)
		return;

	// Flush table
	mib_chain_clear(MIB_IP_QOS_QUEUE_TBL);

	// Add MAX_QOS_QUEUE_NUM of default Qos Queues
	for (i=0; i<4; i++) {
		qEntry.enable = 0;
		qEntry.weight = 10*(4-i);
		qEntry.QueueInstNum = i+1;
		mib_chain_add(MIB_IP_QOS_QUEUE_TBL, &qEntry);
	}
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
}

static void checkDownstreamQosQueue()
{
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int i;
	
	if(mib_chain_total(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL) >= 4)
		return;
	
	// Flush table
	mib_chain_clear(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL);

	// Add MAX_QOS_QUEUE_NUM of default Qos Queues
	for (i=0; i<4; i++) {
		qEntry.enable = 0;
		qEntry.weight = 10*(4-i);
		qEntry.QueueInstNum = i+1;
		mib_chain_add(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL, &qEntry);
	}
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
}

int setupIPQ(void)
{
	unsigned int qosEnable;
	unsigned int downstreamQosEnable;
	unsigned int qosRuleNum, carRuleNum;
	unsigned char totalBandWidthEn;
	unsigned char totalDownstreamBandWidthEn;
#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
	unsigned char up_mode = 0;
	unsigned char down_mode = 0;
#endif

	QOS_SETUP_PRINT_FUNCTION
	
	checkQosQueue();
	checkDownstreamQosQueue();
	UpdateIpFastpathStatus(); // need to disable upstream IPQoS for IPQoS

	qosEnable = getQosEnable();
	downstreamQosEnable = getDownstreamQosEnable();
	qosRuleNum = getQosRuleNum();
	
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	unsigned char totalBandWidthEn_array[QOS_MAX_PORT_NUM] = {0};
	if(current_qos_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
#endif
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	mib_get_s(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&totalBandWidthEn, sizeof(totalBandWidthEn));
#else
	mib_get_s(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&totalBandWidthEn_array, sizeof(totalBandWidthEn_array));
	totalBandWidthEn = totalBandWidthEn_array[current_qos_wan_port];
#endif
	mib_get_s(MIB_TOTAL_DOWNSTREAM_BANDWIDTH_LIMIT_EN, (void *)&totalDownstreamBandWidthEn, sizeof(totalDownstreamBandWidthEn));

#ifndef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
	carRuleNum = mib_chain_total(MIB_IP_QOS_TC_TBL);
#else
	carRuleNum = 1;
	mib_get(MIB_DATA_SPEED_LIMIT_UP_MODE, &up_mode);
	mib_get(MIB_DATA_SPEED_LIMIT_DOWN_MODE, &down_mode);
#endif
#if defined(QOS_SETUP_IFB)
	va_cmd(IFCONFIG, 2, 1, "ifb0", "up");	
#endif
	setupLanItfMap(1);
	
	// hwnat qos
	hwnat_qos_queue_setup();
	setWanIF1PMark();
	
	// Set qdisc to prio for all WAN interface. It need delete first when the WAN interface want to be added another qdisc(see the "delete default prio qdisc" string on this file).
	setupDefaultQdisc(1);

#ifdef CONFIG_USER_AP_HW_QOS
	if (qosEnable && (qosRuleNum || totalBandWidthEn || carRuleNum)) {
		rtk_ap_qos_enable(1);
	}
#endif

#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
	if(up_mode== DATA_SPEED_LIMIT_MODE_DISABLE && down_mode == DATA_SPEED_LIMIT_MODE_DISABLE) {
#endif
		if (qosEnable || downstreamQosEnable)
			setupQosRuleChain(1);
		if ((qosEnable || downstreamQosEnable) && qosRuleNum) { // ip qos
#if defined(CONFIG_USER_AP_HW_QOS)
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
			rtk_ap_qos_queue_set(qos_wan_port, 1);
#else
			rtk_ap_qos_queue_set(rtk_port_get_phyPort(current_qos_wan_port), 1);
#endif
#endif
			setupQdisc(1);
			setupQRule();
#if defined(QOS_SETUP_IMQ)
			enableIMQ(1);
#endif
		}
#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_		
	}
#endif
#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
	else if (qosEnable && (totalBandWidthEn || carRuleNum)) 
	{
		setup_speed_limit_chain(1);
		setup_speed_limit_qdisc(1);
		setup_data_speed_limit();
	}
#else
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && !defined(CONFIG_USER_AP_QOS)  //Apollo HW have rate limit and QoS marking/rules co-exist capability.
	if ((qosEnable || downstreamQosEnable) && (totalBandWidthEn || carRuleNum))			// traffic shapping
#else
	else if ((qosEnable || downstreamQosEnable) && (totalBandWidthEn || carRuleNum))		// traffic shapping
#endif
	{
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		setupCarChain(1);
		setupCarQdisc(1);
		setupCarRule();
#endif
	}
#endif

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && (defined(CONFIG_COMMON_RT_API))
	unsigned int upstreamBandwidth = getUpLinkRate();
	unsigned int downstreamBandwidth = getDownLinkRate();
	int wanPort = 0;
	AUG_PRT("Config QoS total upstream %d Kbps downstream %d Kbps\n",upstreamBandwidth, downstreamBandwidth);
#ifdef CONFIG_USER_AP_HW_QOS
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	wanPort = qos_wan_port;
#else
	wanPort = rtk_port_get_phyPort(current_qos_wan_port);
#endif

	if (qosEnable && (qosRuleNum || totalBandWidthEn || carRuleNum))
#endif		
	{
		/* 	with hw qos scheduler, actual output bandwidth maybe a little greater than configured limit value, 
					so minish 3% of the configured limit value ahead.	*/
		//upstreamBandwidth = (upstreamBandwidth*97)/100;
		rtk_fc_total_upstream_bandwidth_set(wanPort, upstreamBandwidth);
#ifdef CONFIG_QOS_SUPPORT_DOWNSTREAM
		rtk_fc_total_downstream_bandwidth_set(wanPort, downstreamBandwidth);
#endif
	}
#endif

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	setup_pon_queues();
#endif
#if defined(WLAN_SUPPORT) && defined(CONFIG_QOS_SUPPORT_DOWNSTREAM)
	setup_downstream_queues();
#endif

#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL
	init_maxBandwidth();
	clear_maxBandwidth();
	apply_maxBandwidth();
#endif
#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
	rtk_qos_control_packet_protection();
#endif

	rtk_qos_tcp_smallack_protection();
#if defined(CONFIG_YUEME) &&  defined(SUPPORT_CLOUD_VR_SERVICE) && defined(CONFIG_EPON_FEATURE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	extern int check_special_ds_ratelimit(void);
 	//special, for test case 4.1.1, to promise the VR download performance
	check_special_ds_ratelimit();
#endif

#if defined(SUPPORT_CLOUD_VR_SERVICE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && (defined(CONFIG_EPON_FEATURE) || defined(CONFIG_GPON_FEATURE))
	extern void check_special_yueme_test(void);
	check_special_yueme_test();
#endif
	rtk_hwnat_flow_flush();

	return 0;
}

int stopIPQ(void)
{
	int port = 0, pri = 0, queue = 0;
	setupQosRuleChain(0);
	setupQdisc(0);
	setupDefaultQdisc(0);
#if defined(QOS_SETUP_IMQ)
	enableIMQ(0);
#endif
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	// don't use car in multi_phy_wan
	setupCarChain(0);
	setupCarQdisc(0);
#endif
	// hwnat qos
	hwnat_qos_queue_stop();
	hwnat_qos_bandwidth_setup(0);

#if defined(CONFIG_USER_AP_HW_QOS)
	rtk_ap_qos_enable(0);
	for (port = PHYPORT_0; port <= PHYPORT_MAX; port++) {
		rtk_ap_qos_bandwidth_control(port, QOS_BANDWIDTH_UNLIMITED);
		for(pri = 0; pri < 8; pri++){
			rtk_ap_qos_dot1p_remark_set(port, 0, pri, 0);
		}
		for(queue = 0; queue < 8; queue++){
			rtk_ap_qos_queue_shaping_set(port, queue, 0, QOS_BANDWIDTH_UNLIMITED);
		}

		rtk_ap_qos_queue_set(port, 0);
	}
#endif

#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
	setup_speed_limit_chain(0);
	setup_speed_limit_qdisc(0);
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	speed_limit_set_once = 0;
#endif

#ifdef CONFIG_ETH_AND_WLAN_QOS_SUPPORT
	setup_data_speed_limit_wlan_mac_qos(0);		// set iwpriv sbwc to unlimited
#endif
#endif
	
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
	rtk_qos_share_meter_flush(RTK_APP_TYPE_IP_QOS_RATE_SHAPING);
#endif

#ifdef CONFIG_RTL_SMUX_DEV
	rtk_qos_smuxdev_qos_us_rule_flush();
#ifdef CONFIG_QOS_SUPPORT_DOWNSTREAM
	rtk_qos_smuxdev_qos_ds_rule_flush();
#endif
#endif
	setupLanItfMap(0);
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	clear_pon_queues();
#endif
#if defined(WLAN_SUPPORT) && defined(CONFIG_QOS_SUPPORT_DOWNSTREAM)
#ifdef COM_CUC_IGD1_WMMConfiguration
	if(checkWmmServiceExists() == 0)
#endif	
	clear_downstream_queues();
#endif
	rtk_hwnat_flow_flush();

	return 0;
}


void take_qos_effect(void)
{
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	int i;
	init_qos_phy_wan_ports();
	set_qos_wan_itfs();
	init_table_rule_chain();
	stopIPQ();
	for(i=0;i<SW_LAN_PORT_NUM;i++){
		if(!qos_wan_ports[i])
			continue; 
		// WAN port i is up and ready to be set
		current_qos_wan_port = i;
		setupIPQ();
	}
	current_qos_wan_port = -1; //reset 
#else
	stopIPQ();
	setupIPQ();
#endif
}

#ifdef QOS_SUPPORT_RTP
int stopIPQForRTPQos(void)
{
	int port = 0, pri = 0, queue = 0;
	setupQosRuleChain(0);
	setupQdisc(0);
	setupDefaultQdisc(0);
#if defined(QOS_SETUP_IMQ)
	enableIMQ(0);
#endif
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	// don't use car in multi_phy_wan
	setupCarChain(0);
	setupCarQdisc(0);
#endif
	// hwnat qos
	hwnat_qos_queue_stop();
	hwnat_qos_bandwidth_setup(0);

#if defined(CONFIG_USER_AP_HW_QOS)
	rtk_ap_qos_enable(0);
	for (port = PHYPORT_0; port <= PHYPORT_MAX; port++) {
		rtk_ap_qos_bandwidth_control(port, QOS_BANDWIDTH_UNLIMITED);
		for(pri = 0; pri < 8; pri++){
			rtk_ap_qos_dot1p_remark_set(port, 0, pri, 0);
		}
		for(queue = 0; queue < 8; queue++){
			rtk_ap_qos_queue_shaping_set(port, queue, 0, QOS_BANDWIDTH_UNLIMITED);
		}

		rtk_ap_qos_queue_set(port, 0);
	}
#endif

#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
	setup_speed_limit_chain(0);
	setup_speed_limit_qdisc(0);
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	speed_limit_set_once = 0;
#endif

#ifdef CONFIG_ETH_AND_WLAN_QOS_SUPPORT
	setup_data_speed_limit_wlan_mac_qos(0);		// set iwpriv sbwc to unlimited
#endif
#endif
	
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
	rtk_qos_share_meter_flush(RTK_APP_TYPE_IP_QOS_RATE_SHAPING);
#endif

#ifdef CONFIG_RTL_SMUX_DEV
	rtk_qos_smuxdev_qos_us_rule_flush();
#ifdef CONFIG_QOS_SUPPORT_DOWNSTREAM
	rtk_qos_smuxdev_qos_ds_rule_flush();
#endif
#endif
	setupLanItfMap(0);
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	clear_pon_queues();
#endif
#if defined(WLAN_SUPPORT) && defined(CONFIG_QOS_SUPPORT_DOWNSTREAM)
	clear_downstream_queues();
#endif
	//rtk_hwnat_flow_flush();  //rtp qos not need to flush flow and conntrack

	return 0;
}

int setupIPQForRTPQos(void)
{
	unsigned int qosEnable;
	unsigned int downstreamQosEnable;
	unsigned int qosRuleNum, carRuleNum;
	unsigned char totalBandWidthEn;
	unsigned char totalDownstreamBandWidthEn;
#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
	unsigned char up_mode = 0;
	unsigned char down_mode = 0;
#endif

	QOS_SETUP_PRINT_FUNCTION
	
	checkQosQueue();
	checkDownstreamQosQueue();
	UpdateIpFastpathStatus(); // need to disable upstream IPQoS for IPQoS

	qosEnable = getQosEnable();
	downstreamQosEnable = getDownstreamQosEnable();
	qosRuleNum = getQosRuleNum();
	
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	unsigned char totalBandWidthEn_array[QOS_MAX_PORT_NUM] = {0};
	if(current_qos_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
#endif
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	mib_get_s(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&totalBandWidthEn, sizeof(totalBandWidthEn));
#else
	mib_get_s(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&totalBandWidthEn_array, sizeof(totalBandWidthEn_array));
	totalBandWidthEn = totalBandWidthEn_array[current_qos_wan_port];
#endif
	mib_get_s(MIB_TOTAL_DOWNSTREAM_BANDWIDTH_LIMIT_EN, (void *)&totalDownstreamBandWidthEn, sizeof(totalDownstreamBandWidthEn));

#ifndef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
	carRuleNum = mib_chain_total(MIB_IP_QOS_TC_TBL);
#else
	carRuleNum = 1;
	mib_get(MIB_DATA_SPEED_LIMIT_UP_MODE, &up_mode);
	mib_get(MIB_DATA_SPEED_LIMIT_DOWN_MODE, &down_mode);
#endif
#if defined(QOS_SETUP_IFB)
	va_cmd(IFCONFIG, 2, 1, "ifb0", "up");	
#endif
	setupLanItfMap(1);
	
	// hwnat qos
	hwnat_qos_queue_setup();
	setWanIF1PMark();
	
	// Set qdisc to prio for all WAN interface. It need delete first when the WAN interface want to be added another qdisc(see the "delete default prio qdisc" string on this file).
	setupDefaultQdisc(1);

#ifdef CONFIG_USER_AP_HW_QOS
	if (qosEnable && (qosRuleNum || totalBandWidthEn || carRuleNum)) {
		rtk_ap_qos_enable(1);
	}
#endif

#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
	if(up_mode== DATA_SPEED_LIMIT_MODE_DISABLE && down_mode == DATA_SPEED_LIMIT_MODE_DISABLE) {
#endif
		if (qosEnable || downstreamQosEnable)
			setupQosRuleChain(1);
		if ((qosEnable || downstreamQosEnable) && qosRuleNum) { // ip qos
#if defined(CONFIG_USER_AP_HW_QOS)
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
			rtk_ap_qos_queue_set(qos_wan_port, 1);
#else
			rtk_ap_qos_queue_set(rtk_port_get_phyPort(current_qos_wan_port), 1);
#endif
#endif
			setupQdisc(1);
			setupQRule();
#if defined(QOS_SETUP_IMQ)
			enableIMQ(1);
#endif
		}
#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_		
	}
#endif
#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
	else if (qosEnable && (totalBandWidthEn || carRuleNum)) 
	{
		setup_speed_limit_chain(1);
		setup_speed_limit_qdisc(1);
		setup_data_speed_limit();
	}
#else
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && !defined(CONFIG_USER_AP_QOS)  //Apollo HW have rate limit and QoS marking/rules co-exist capability.
	if ((qosEnable || downstreamQosEnable) && (totalBandWidthEn || carRuleNum))			// traffic shapping
#else
	else if ((qosEnable || downstreamQosEnable) && (totalBandWidthEn || carRuleNum))		// traffic shapping
#endif
	{
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		setupCarChain(1);
		setupCarQdisc(1);
		setupCarRule();
#endif
	}
#endif

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && (defined(CONFIG_COMMON_RT_API))
	unsigned int upstreamBandwidth = getUpLinkRate();
	unsigned int downstreamBandwidth = getDownLinkRate();
	int wanPort = 0;
	AUG_PRT("Config QoS total upstream %d Kbps downstream %d Kbps\n",upstreamBandwidth, downstreamBandwidth);
#ifdef CONFIG_USER_AP_HW_QOS
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	wanPort = qos_wan_port;
#else
	wanPort = rtk_port_get_phyPort(current_qos_wan_port);
#endif

	if (qosEnable && (qosRuleNum || totalBandWidthEn || carRuleNum))
#endif		
	{
		/* 	with hw qos scheduler, actual output bandwidth maybe a little greater than configured limit value, 
					so minish 3% of the configured limit value ahead.	*/
		upstreamBandwidth = (upstreamBandwidth*97)/100;
		rtk_fc_total_upstream_bandwidth_set(wanPort, upstreamBandwidth);
#ifdef CONFIG_QOS_SUPPORT_DOWNSTREAM
		rtk_fc_total_downstream_bandwidth_set(wanPort, downstreamBandwidth);
#endif
	}
#endif

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	setup_pon_queues();
#endif
#if defined(WLAN_SUPPORT) && defined(CONFIG_QOS_SUPPORT_DOWNSTREAM)
	setup_downstream_queues();
#endif

#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL
	init_maxBandwidth();
	clear_maxBandwidth();
	apply_maxBandwidth();
#endif
#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
	rtk_qos_control_packet_protection();
#endif

	rtk_qos_tcp_smallack_protection();

	//rtk_hwnat_flow_flush();

	return 0;
}


void take_qos_effect_for_rtp(void)
{
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	int i;
	init_qos_phy_wan_ports();
	set_qos_wan_itfs();
	init_table_rule_chain();
	stopIPQForRTPQos();
	for(i=0;i<SW_LAN_PORT_NUM;i++){
		if(!qos_wan_ports[i])
			continue; 
		// WAN port i is up and ready to be set
		current_qos_wan_port = i;
		setupIPQForRTPQos();
	}
	current_qos_wan_port = -1; //reset 
#else
	stopIPQForRTPQos();
	setupIPQForRTPQos();
#endif
}

void qos_handler(int signum)
{
	take_qos_effect_for_rtp();
}
#endif

#ifdef _PRMT_X_CT_COM_QOS_
#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
const char *ct_typename[CT_TYPE_NUM] = {
	"SMAC", "DMAC", "8021P", "SIP", "DIP", "SPORT", "DPORT", "TOS", "DSCP",
		"SIP6","DIP6","SPORT6","DPORT6","TrafficClass", "WANInterface", "LANInterface","EtherType"
};
#else
const char *ct_typename[CT_TYPE_NUM + 1] = {
	"", "SMAC", "8021P", "SIP", "DIP", "SPORT", "DPORT", "TOS", "DSCP",
	    "WANInterface", "LANInterface",
};
#endif

const char *LANString[12] = {
	"InternetGatewayDevice.LANDevice.1.LANEthernetInterfaceConfig.1",	//eth0_sw0
	"InternetGatewayDevice.LANDevice.1.LANEthernetInterfaceConfig.2",	//eth0_sw1
	"InternetGatewayDevice.LANDevice.1.LANEthernetInterfaceConfig.3",	//eth0_sw2
	"InternetGatewayDevice.LANDevice.1.LANEthernetInterfaceConfig.4",	//eth0_sw3
	"InternetGatewayDevice.LANDevice.1.WLANConfiguration.1",
	"InternetGatewayDevice.LANDevice.1.WLANConfiguration.2",
	"InternetGatewayDevice.LANDevice.1.WLANConfiguration.3",
	"InternetGatewayDevice.LANDevice.1.WLANConfiguration.4",
	"InternetGatewayDevice.LANDevice.1.WLANConfiguration.5",
	"InternetGatewayDevice.LANDevice.1.WLANConfiguration.6",
	"InternetGatewayDevice.LANDevice.1.WLANConfiguration.7",
	"InternetGatewayDevice.LANDevice.1.WLANConfiguration.8",
};

static int findcttype(MIB_CE_IP_QOS_T *p, unsigned char typenum)
{
	int i;

	for (i = 0; i < CT_TYPE_NUM; i++) {
		if (p->cttypemap[i] == typenum)
			return i;
	}

	return -1;
}

static int gettypeinst(MIB_CE_IP_QOS_T *p)
{
	int i;

	for (i = 0; i < CT_TYPE_NUM; i++) {
		if (p->cttypemap[i] == 0 || p->cttypemap[i] == 0xf)
			return i;
	}

	return 0;
}

static unsigned int getIPTVIndex()
{
	MIB_CE_ATM_VC_T entry;
	int i, total;

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
			continue;
		if ((entry.applicationtype == X_CT_SRV_OTHER || entry.applicationtype == X_CT_SRV_INTERNET) && entry.cmode == CHANNEL_MODE_BRIDGE && entry.enable == 1)	//OTHER or INTERNET
			return entry.ifIndex;
	}

	return DUMMY_IFINDEX;
}


int updatecttypevalue(MIB_CE_IP_QOS_T *p)
{
	unsigned char i;
	unsigned char typenum = 0;
	struct in_addr addr;
	int intvalue;
	int typeinst;
	int tmpinst;

	if (p->modeTr69 != MODEIPTV && p->modeTr69 != MODEOTHER)
		return -1;

	for (i = 1; i <= CT_TYPE_NUM; i++) {
		tmpinst = findcttype(p, i);
		switch (i) {
		case 1:	//SMAC
			if (!isMacAddrInvalid(p->smac)) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
			} else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		case 2:	//8021P
			if (p->vlan1p > 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
			} else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		case 3:	//SIP
			if (p->sip[0] != 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
#ifdef CONFIG_IPV6
			}else if (p->sip6[0] != 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
#endif				
			}else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		case 4:	//DIP
			if (p->dip[0] != 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
#ifdef CONFIG_IPV6
			}else if (p->dip6[0] != 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
#endif				
			} else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		case 5:	//SPORT
			if (p->sPort != 0 || p->sPortRangeMax != 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
			} else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		case 6:	//DPORT
			if (p->dPort != 0 || p->dPortRangeMax != 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
			} else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		case 7:	//TOS
			break;
		case 8:	//DSCP
			if (p->qosDscp != 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
			} else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		case 9:	//WANInterface
			if (p->outif != DUMMY_IFINDEX)
			{
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
			}
			else
			{
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		case 10:	//LANInterface
			if (p->phyPort > 0 || p->minphyPort > 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
			} else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		default:
			break;
		}
	}
	return 0;
}

static void calculate_ip(struct in_addr *ip, unsigned char mask, int maxflag, struct in_addr *result)
{
	unsigned int addr = 0;
	int i;

	for(i = 1 ; i <= mask ;i++)
		addr |= (1 << (32-i));
	
	memcpy(result, ip, sizeof(struct in_addr));
	result->s_addr &= htonl(addr);

	if(maxflag)
	{
		addr = 0;
		for(i = 0 ; i < (32 - mask) ;i++)
			addr |= (1 << i);

		result->s_addr |= htonl(addr);	
	}
}

int getcttypevalue(char *typevalue, MIB_CE_IP_QOS_T *p, int typeinst,
		   int maxflag)
{
	unsigned char typenum;

	typenum = p->cttypemap[typeinst];
	if (typenum <= 0 || typenum > CT_TYPE_NUM)
		return -1;

	switch (typenum) {
	case 1:		//SMAC
		mac_hex2string(p->smac, typevalue);
		return 0;
	case 2:		//8021P
		sprintf(typevalue, "%hhu", p->vlan1p - 1);
		return 0;
	case 3: 	//SIP
		{
#ifdef CONFIG_IPV6
			if(p->IpProtocol ==1)
			{
				struct in_addr ip_result = {0};
				calculate_ip((struct in_addr *)p->sip, p->smaskbit, maxflag, &ip_result);
				strcpy(typevalue, inet_ntoa(ip_result));
			}else if(p->IpProtocol ==2){
				struct in6_addr ip6_result = {0};
				prefixtoIp6(p->sip6, p->sip6PrefixLen, &ip6_result, maxflag);
				inet_ntop(AF_INET6, &ip6_result, typevalue, INET6_ADDRSTRLEN);
			}
#else
			struct in_addr ip_result = {0};
			calculate_ip((struct in_addr *)p->sip, p->smaskbit, maxflag, &ip_result);
			strcpy(typevalue, inet_ntoa(ip_result));
#endif
			return 0;
		}
	case 4: 	//DIP
		{
#ifdef CONFIG_IPV6
			if(p->IpProtocol ==1)
			{
				struct in_addr ip_result = {0};
				calculate_ip((struct in_addr *)p->dip, p->dmaskbit, maxflag, &ip_result);
				strcpy(typevalue, inet_ntoa(ip_result));
			}else if(p->IpProtocol ==2){
				struct in6_addr ip6_result = {0};
				prefixtoIp6(p->dip6, p->dip6PrefixLen, &ip6_result, maxflag);
				inet_ntop(AF_INET6, &ip6_result, typevalue, INET6_ADDRSTRLEN);
			}
#else
			struct in_addr ip_result = {0};
			calculate_ip((struct in_addr *)p->dip, p->dmaskbit, maxflag, &ip_result);
			strcpy(typevalue, inet_ntoa(ip_result));
#endif
			return 0;
		}

	case 5:		//SPORT
		sprintf(typevalue, "%hu",
			maxflag ? p->sPortRangeMax : p->sPort);
		return 0;
	case 6:		//DPORT
		sprintf(typevalue, "%hu",
			maxflag ? p->dPortRangeMax : p->dPort);
		return 0;
	case 7:		//TOS
		sprintf(typevalue, "%hhu", p->tos);
		return 0;
	case 8:		//DSCP
		sprintf(typevalue, "%hhu", (p->qosDscp - 1) >> 2);
		return 0;
	case 9:		//WANInterface
		if (p->outif != DUMMY_IFINDEX)
		{
			sprintf(typevalue, "%u", p->outif);
			return 0;
		}
		else
			return -1;
	case 10:		//LANInterface
		if (maxflag == 1) {
			if (p->phyPort > 0 && p->phyPort < 12) {
				strcpy(typevalue, LANString[p->phyPort - 1]);
				return 0;
			} else
				return -1;
		} else {
			if (p->minphyPort > 0 && p->minphyPort < 12) {
				strcpy(typevalue, LANString[p->minphyPort - 1]);
				return 0;
			} else
				return -1;
		}
	default:
		return -1;
	}
}

int setcttypevalue(char *typevalue, MIB_CE_IP_QOS_T * p, int typeinst,
		   int maxflag)
{
	unsigned char typenum;
	int i;

	typenum = p->cttypemap[typeinst];
	if (typenum <= 0 || typenum > CT_TYPE_NUM)
		return -1;

	switch (typenum) {
	case 1:		//SMAC
		for(i = 0; i < ETH_ALEN; i++) {
			p->smac[i] = hex(typevalue[i * 3]) * 16 + hex(typevalue[i * 3 + 1]);
		}
		p->enable = 1;
		return 0;
	case 2:		//8021P
		sscanf(typevalue, "%hhu", &p->vlan1p);
		p->vlan1p = p->vlan1p + 1;
		p->enable = 1;
		return 0;
	case 3:		//SIP
#ifdef CONFIG_IPV6
		if (inet_aton(typevalue, (struct in_addr *)p->sip))
		{
			p->smaskbit = 32;
			p->enable = 1;
			p->IpProtocol = 1;
			return 0;
		}
		else if(inet_pton(AF_INET6, typevalue, p->sip6))
		{
			p->sip6PrefixLen = 128;
			p->enable = 1;
			p->IpProtocol = 2;
			return 0;
		}
		return -1;
#else
		if (!inet_aton(typevalue, (struct in_addr *)p->sip))
			return -1;
		p->smaskbit = 32;
		p->enable = 1;
		return 0;
#endif
	case 4:		//DIP
#ifdef CONFIG_IPV6
		if (inet_aton(typevalue, (struct in_addr *)p->dip))
		{
			p->smaskbit = 32;
			p->enable = 1;
			p->IpProtocol = 1;
			return 0;
		}
		else if(inet_pton(AF_INET6, typevalue, p->dip6))
		{
			p->dip6PrefixLen = 128;
			p->enable = 1;
			p->IpProtocol = 2;
			return 0;
		}
		return -1;
#else
		if (!inet_aton(typevalue, (struct in_addr *)p->dip))
			return -1;
		p->smaskbit = 32;
		p->enable = 1;
		return 0;
#endif
	case 5:		//SPORT
		sscanf(typevalue, "%hu", maxflag ? &p->sPortRangeMax : &p->sPort);
		p->enable = 1;
		return 0;
	case 6:		//DPORT
		sscanf(typevalue, "%hu", maxflag ? &p->dPortRangeMax : &p->dPort);
		p->enable = 1;
		return 0;
	case 7:		//TOS
		sscanf(typevalue, "%hhu", &p->tos);
		p->enable = 1;
		return 0;
	case 8:		//DSCP
		sscanf(typevalue, "%hhu", &p->qosDscp);
		p->qosDscp = p->qosDscp >> 2 + 1;
		p->enable = 1;
		return 0;
	case 9:		//WANInterface
		sscanf(typevalue, "%u", &p->outif);
		p->enable = 1;
		return 0;
	case 10:		//LANInterface
		for (i = 0; i < 12; i++) {
			if (strstr(typevalue, LANString[i]) != 0)
				break;
		}
		if (i < 12) {
			if (maxflag == 1)
				p->phyPort = i + 1;
			else
				p->minphyPort = i + 1;
		} else
			return -1;

		p->enable = 1;
		return 0;
	default:
		return 0;
	}
}

int getcttype(char *typename, MIB_CE_IP_QOS_T *p, int typeinst)
{
	unsigned char typenum;

	typenum = p->cttypemap[typeinst];
#if defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_CMCC)
	if (typenum < 0 || (typenum >= CT_TYPE_NUM && typenum != 0xff))
		return -1;
#else
	if (typenum <= 0 || typenum > CT_TYPE_NUM)
		return -1;
#endif

	if(ct_typename[typenum] != NULL)
		strcpy(typename, ct_typename[typenum]);
	else
		strcpy(typename, "");
	return 0;
}

int setcttype(char *typename, MIB_CE_IP_QOS_T *p, int typeinst)
{
	int i;

#if defined(CONFIG_CU) || defined(CONFIG_CMCC)
		for (i = 0; i < CT_TYPE_NUM; i++) {
#else
		for (i = 1; i <= CT_TYPE_NUM; i++) {
#endif
		if (!strcmp(typename, ct_typename[i])) {
			delcttypevalue(p, typeinst);
			p->cttypemap[typeinst] = i;

			return 0;
		}
	}

	return -1;
}

int delcttypevalue(MIB_CE_IP_QOS_T *p, int typeinst)
{
	unsigned char typenum;

	typenum = p->cttypemap[typeinst];
	if (typenum <= 0 || (typenum > CT_TYPE_NUM && typenum != 0xf))
		return -1;
	p->cttypemap[typeinst] = 0;

	switch (typenum) {
	case 1:		//SMAC
		memset(p->smac, 0, sizeof(p->smac));
		return 0;
	case 2:		//8021P
		p->vlan1p = 0;
		return 0;
	case 3:		//SIP
		memset(p->sip, 0, sizeof(p->sip));
#ifdef CONFIG_IPV6
		memset(p->sip6, 0, sizeof(p->sip6));
#endif
		return 0;
	case 4:		//DIP
		memset(p->dip, 0, sizeof(p->dip));
#ifdef CONFIG_IPV6
		memset(p->dip6, 0, sizeof(p->dip6));
#endif
		return 0;
	case 5:		//SPORT
		p->sPort = 0;
		p->sPortRangeMax = 0;
		return 0;
	case 6:		//DPORT
		p->dPort = 0;
		p->dPortRangeMax = 0;
		return 0;
	case 7:		//TOS
		p->tos = 0;
		return 0;
	case 8:		//DSCP
		p->qosDscp = 0;
		return 0;
	case 9:		//WANInterface
		p->outif = DUMMY_IFINDEX;
		return 0;
	case 10:		//LANInterface
		p->phyPort = 0;
		p->minphyPort = 0;
		return 0;
	default:
		return 0;
	}
}
#endif

#ifdef CONFIG_RTL_SMUX_DEV
int rtk_qos_smuxdev_qos_us_rule_set(unsigned int mark, int order)
{
	char f_skb_mark_mask[32], f_skb_mark_value[32];
	char s_skb_mark_mask[32], s_skb_mark_value[32];
	char rule_alias_name[256], tags_num[32], rule_priority[32], remark_str[32];
	MIB_CE_IP_QOS_T ip_qos_entry;
	char devname[IFNAMSIZ];
	char priority_str[32];
	unsigned char enableQos1p=2;
	int j, k;
	char *wanitf = NULL;
	char vid_str[8];
	char vlan1p_str[8];
	
	QOS_SETUP_PRINT_FUNCTION
#if defined(_PRMT_X_CT_COM_QOS_)
	mib_get_s(MIB_QOS_ENABLE_1P, (void *)&enableQos1p, sizeof(enableQos1p));
#endif
	
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(current_qos_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
#endif
	if(mib_chain_get(MIB_IP_QOS_TBL, order, (void*)&ip_qos_entry)&&ip_qos_entry.enable)
	{
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(ip_qos_entry.logic_wan_port != current_qos_wan_port)
			return 0;
#endif
		if(ip_qos_entry.classtype == QOS_CLASSTYPE_NORMAL)
		{
			sprintf(f_skb_mark_mask, "0x%x", SOCK_MARK_QOS_ENTRY_BIT_MASK);
			sprintf(f_skb_mark_value, "0x%x", (mark&SOCK_MARK_QOS_ENTRY_BIT_MASK));
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
			//get root wan device like eth0.3
			if(!(wanitf=(char*)getQosRootWanName(current_qos_wan_port)))
				return 0;
			snprintf(devname,sizeof(devname),"%s",wanitf);
#else
			sprintf(devname, "%s", ALIASNAME_NAS0);
#endif
			sprintf(rule_priority, "%d", SMUX_RULE_PRIO_QOS_HIGH);
#if defined(_PRMT_X_CT_COM_QOS_)
			if((enableQos1p == QOS_8021P_REMARKING) || (enableQos1p == QOS_8021P_0_MARKING))
#else
			if(ip_qos_entry.m_1p)
#endif
			{
#if defined(_PRMT_X_CT_COM_QOS_)
				if(enableQos1p == QOS_8021P_0_MARKING) // 0 marking
					strcpy(priority_str, "0");
				else if (enableQos1p == QOS_8021P_REMARKING) //remarking
				{
					if(ip_qos_entry.m_1p )
						sprintf(priority_str, "%d", (ip_qos_entry.m_1p-1));
					else
						strcpy(priority_str, "0");
				}
#else
				sprintf(priority_str, "%d", (ip_qos_entry.m_1p-1));
#endif
				for(j=0 ; j<=SMUX_MAX_TAGS ; j++)
				{
					sprintf(tags_num, "%d", j);
					sprintf(rule_alias_name, "ip_qos_mark_8021p_us_%s_%s_%d", devname, tags_num, order);
					if(j==0)
					{
						va_cmd(SMUXCTL, 16, 1, "--if", devname, "--tx", "--tags", tags_num, "--filter-skb-mark", f_skb_mark_mask, f_skb_mark_value, "--set-priority", priority_str, "1", "--rule-priority", rule_priority, "--rule-alias", rule_alias_name, "--rule-append");
					}
					else
					{
						va_cmd(SMUXCTL, 16, 1, "--if", devname, "--tx", "--tags", tags_num, "--filter-skb-mark", f_skb_mark_mask, f_skb_mark_value, "--set-priority", priority_str, tags_num, "--rule-priority", rule_priority, "--rule-alias", rule_alias_name, "--rule-append");
					}
				}
				
			}
#ifndef CONFIG_BRIDGE_EBT_FTOS
			if(ip_qos_entry.m_dscp)
			{
				char dscp_str[32];
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
				sprintf(dscp_str, "%d", ip_qos_entry.m_dscp);
#else
				sprintf(dscp_str, "%d", (ip_qos_entry.m_dscp-1)>>2);
#endif
				for(j=0 ; j<=SMUX_MAX_TAGS ; j++)
				{
					sprintf(tags_num, "%d", j);
					sprintf(rule_alias_name, "ip_qos_mark_dscp_us_%s_%s_%d", devname, tags_num, order);
					va_cmd(SMUXCTL, 15, 1, "--if", devname, "--tx", "--tags", tags_num, "--filter-skb-mark", f_skb_mark_mask, f_skb_mark_value, "--set-dscp", dscp_str, "--rule-priority", rule_priority, "--rule-alias", rule_alias_name, "--rule-append");
				}
			}
#endif
			if(ip_qos_entry.ipqos_rule_type == IPQOS_8021P_BASE)
			{
				char *argv[40];
				int i=1;
				
				sprintf(s_skb_mark_mask, "0x%x", SOCK_MARK_QOS_SWQID_BIT_MASK);
				sprintf(s_skb_mark_value, "0x%x", (mark&SOCK_MARK_QOS_SWQID_BIT_MASK));		
				for(j=0 ; j<=SMUX_MAX_TAGS ; j++)
				{
					i=1;
					sprintf(tags_num, "%d", j);
					sprintf(rule_alias_name, "ip_qos_vlan_base_us_%s_%s_%d", devname, tags_num, order);
					argv[i++]="--if";
					argv[i++]=devname;
					argv[i++]="--tx";
					argv[i++]="--tags";
					argv[i++]=tags_num;
					if(ip_qos_entry.vlanid > 0)
					{
						sprintf(vid_str, "%d", ip_qos_entry.vlanid);
						argv[i++]="--filter-vid";
						argv[i++]=vid_str;
						argv[i++]="0";
					}
					if(ip_qos_entry.vlan1p > 0)
					{
						sprintf(vlan1p_str, "%d", ip_qos_entry.vlan1p);
						argv[i++]="--filter-priority";
						argv[i++]=vlan1p_str;
						argv[i++]="0";
					}
					argv[i++]="--set-skb-mark";
					argv[i++]=s_skb_mark_mask;
					argv[i++]=s_skb_mark_value;
					argv[i++]="--rule-priority";
					argv[i++]=rule_priority;
					argv[i++]="--rule-alias";
					argv[i++]=rule_alias_name;
					argv[i++]="--rule-append";
					argv[i]=NULL;
					do_cmd(SMUXCTL, argv, 1);
				}
			}
		}
		else if(ip_qos_entry.classtype == QOS_CLASSTYPE_APPBASED)
		{
			sprintf(f_skb_mark_mask, "0x%x", SOCK_MARK_QOS_ENTRY_BIT_MASK|SOCK_MARK_QOS_SWQID_BIT_MASK);
			sprintf(f_skb_mark_value, "0x0");
			sprintf(s_skb_mark_mask, "0x%x", SOCK_MARK_QOS_SWQID_BIT_MASK);
			sprintf(s_skb_mark_value, "0x%x", SOCK_MARK_QOS_SWQID_TO_MARK(getIPQosQueueNumber()-ip_qos_entry.prior));
			sprintf(rule_priority, "%d", SMUX_RULE_PRIO_QOS_HIGH);
			ifGetName(PHY_INTF(ip_qos_entry.outif), devname, sizeof(devname));
			for(j=0 ; j<=SMUX_MAX_TAGS ; j++)
			{
				QOS_SETUP_PRINT_FUNCTION
				sprintf(tags_num, "%d", j);
				sprintf(rule_alias_name, "ip_qos_appbased_us_%s_%s_%d", devname, tags_num, order);
				va_cmd(SMUXCTL, 16, 1, "--if", devname, "--tx", "--tags", tags_num, "--filter-skb-mark", f_skb_mark_mask, f_skb_mark_value, "--set-skb-mark", s_skb_mark_mask, s_skb_mark_value, "--rule-priority", rule_priority, "--rule-alias", rule_alias_name, "--rule-insert");
			}
		}
	}
	return 0;
}

int rtk_qos_smuxdev_qos_us_rule_flush(void)
{
	char rule_alias_name[256], tags_num[32];
	int i, j, k, atm_vc_total;
	MIB_CE_ATM_VC_T atm_vc_entry;
	char devname[IFNAMSIZ];
	char *wanitf = NULL;
	
	QOS_SETUP_PRINT_FUNCTION
	
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	for(j=0 ; j<=SMUX_MAX_TAGS ; j++)
	{
		sprintf(tags_num, "%d", j);
		sprintf(devname, "%s", ALIASNAME_NAS0);
		sprintf(rule_alias_name, "ip_qos_mark_8021p_us_+");
		va_cmd(SMUXCTL, 7, 1, "--if", devname, "--tx", "--tags", tags_num, "--rule-remove-alias", rule_alias_name);
		sprintf(rule_alias_name, "ip_qos_mark_dscp_us_+");
		va_cmd(SMUXCTL, 7, 1, "--if", devname, "--tx", "--tags", tags_num, "--rule-remove-alias", rule_alias_name);
		sprintf(rule_alias_name, "ip_qos_vlan_base_us_+");
		va_cmd(SMUXCTL, 7, 1, "--if", devname, "--tx", "--tags", tags_num, "--rule-remove-alias", rule_alias_name);
	}

	atm_vc_total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < atm_vc_total; i++) 
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, &atm_vc_entry))
			continue;

		ifGetName(PHY_INTF(atm_vc_entry.ifIndex), devname, sizeof(devname));
		for(j=0 ; j<=SMUX_MAX_TAGS ; j++)
		{
			sprintf(tags_num, "%d", j);
			sprintf(rule_alias_name, "ip_qos_appbased_us_+");
			va_cmd(SMUXCTL, 7, 1, "--if", devname, "--tx", "--tags", tags_num, "--rule-remove-alias", rule_alias_name);
		}
	}
#else
	snprintf(rule_alias_name, sizeof(rule_alias_name),"ip_qos_mark_8021p_us_+");
	for(j=0 ; j<=SMUX_MAX_TAGS ; j++){
		snprintf(tags_num, sizeof(tags_num),"%d", j);
		for(i=0;i<SW_LAN_PORT_NUM;i++){
			if(!(wanitf=(char*)getQosRootWanName(i)))
				return 0;
			va_cmd(SMUXCTL, 7, 1, "--if", wanitf, "--tx", "--tags", tags_num, "--rule-remove-alias", rule_alias_name);
		}
	}
#endif
	return 0;
}

int rtk_qos_smuxdev_qos_ds_rule_set(unsigned int mark, int order)
{
	char skb_mark_mask[32], skb_mark_value[32];
	char skb_mark2_mask[32], skb_mark2_value[32];

	char rule_alias_name[256], tags_num[32], rule_priority[32];
	MIB_CE_IP_QOS_T ip_qos_entry;
	int i, j;	
	
	QOS_SETUP_PRINT_FUNCTION
	
	sprintf(rule_priority, "%d", SMUX_RULE_PRIO_QOS_HIGH);
	if(mib_chain_get(MIB_IP_QOS_TBL, order, (void*)&ip_qos_entry))
	{
		if (ip_qos_entry.enable&&ip_qos_entry.direction==QOS_DIRECTION_DOWNSTREAM)
		{
			for (i=0 ; i<ELANVIF_NUM ; i++)
			{
				char devname[IFNAMSIZ];

				sprintf(devname, "%s", ELANRIF[i]);
				if(ip_qos_entry.phyPort && !strstr(lan_itf_map.map[ip_qos_entry.phyPort].name, devname))
					continue;

				sprintf(skb_mark_mask, "0x%x", SOCK_MARK_QOS_ENTRY_BIT_MASK);
				sprintf(skb_mark_value, "0x%x", (mark&SOCK_MARK_QOS_ENTRY_BIT_MASK));

				if(ip_qos_entry.m_1p)
				{
					char priority_str[32];
					sprintf(priority_str, "%d", (ip_qos_entry.m_1p-1));
					for(j=0 ; j<=SMUX_MAX_TAGS ; j++)
					{
						sprintf(tags_num, "%d", j);
						sprintf(rule_alias_name, "ip_qos_mark_8021p_ds_%s_%s_%d", devname, tags_num, order);
						if(j==0)
						{
							va_cmd(SMUXCTL, 16, 1, "--if", devname, "--tx", "--tags", tags_num, "--filter-skb-mark", skb_mark_mask, skb_mark_value, "--set-priority", priority_str, "1", "--rule-priority", rule_priority, "--rule-alias", rule_alias_name, "--rule-append");
						}
						else
						{
							va_cmd(SMUXCTL, 16, 1, "--if", devname, "--tx", "--tags", tags_num, "--filter-skb-mark", skb_mark_mask, skb_mark_value, "--set-priority", priority_str, tags_num, "--rule-priority", rule_priority, "--rule-alias", rule_alias_name, "--rule-append");
						}
					}
				}

#ifndef CONFIG_BRIDGE_EBT_FTOS
				if(ip_qos_entry.m_dscp)
				{
					char dscp_str[32];
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
					sprintf(dscp_str, "%d", ip_qos_entry.m_dscp);
#else
					sprintf(dscp_str, "%d", (ip_qos_entry.m_dscp-1)>>2);
#endif
					for(j=0 ; j<=SMUX_MAX_TAGS ; j++)
					{
						sprintf(tags_num, "%d", j);
						sprintf(rule_alias_name, "ip_qos_mark_dscp_ds_%s_%s_%d", devname, tags_num, order);

						// remark rule must have higher priority
						sprintf(skb_mark2_mask, "0x%llx", (SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_MASK|SOCK_MARK2_DSCP_DOWN_BIT_MASK));
						sprintf(skb_mark2_value, "0x0");
						va_cmd(SMUXCTL, 18, 1, "--if", devname, "--tx", "--tags", tags_num, "--filter-skb-mark", skb_mark_mask, skb_mark_value, "--filter-skb-mark2", skb_mark2_mask, skb_mark2_value, "--set-dscp", dscp_str, "--rule-priority", rule_priority, "--rule-alias", rule_alias_name, "--rule-append");
					}
				}
#endif
			}
		}
	}
	return 0;
}

int rtk_qos_smuxdev_qos_ds_rule_flush(void)
{
	char rule_alias_name[256], tags_num[32];
	int i, j, EntryNum = 0;
	
	QOS_SETUP_PRINT_FUNCTION
	
	EntryNum = mib_chain_total(MIB_IP_QOS_TBL);
	for (i=0 ; i<ELANVIF_NUM ; i++) 
	{
		char devname[IFNAMSIZ];

		//ifGetName(PHY_INTF(atm_vc_entry.ifIndex), devname, sizeof(devname));
		sprintf(devname, "%s", ELANRIF[i]);
		for(j=0 ; j<=SMUX_MAX_TAGS ; j++)
		{
			sprintf(tags_num, "%d", j);
			sprintf(rule_alias_name, "ip_qos_mark_8021p_ds_+");
			va_cmd(SMUXCTL, 7, 1, "--if", devname, "--tx", "--tags", tags_num, "--rule-remove-alias", rule_alias_name);
			sprintf(rule_alias_name, "ip_qos_mark_dscp_ds_+");
			va_cmd(SMUXCTL, 7, 1, "--if", devname, "--tx", "--tags", tags_num, "--rule-remove-alias", rule_alias_name);
		}
	}
	return 0;
}
#endif

int rtk_qos_total_us_bandwidth_support(void)
{
	return 1;
}

#ifdef CONFIG_E8B
enum {
	QOS_PRIO_INTERNET = 0,
	QOS_PRIO_TR069,
	QOS_PRIO_VOIP,
	QOS_PRIO_IPTV,
	QOS_PRIO_MAX,
};

#define QOS_TYPE_NUM 5
static int qos_mode_prio[QOS_TYPE_NUM]={0};

static int parsePriorityByQosMode(unsigned char *qosMode)
{
	char *delim=",";
	char *token,*saveptr1,*str1;
	int i, prio=0,total_queue_need=0;
	char buf[MAX_QOS_MODE_LEN]={0};

	if ( !qosMode || qosMode[0]=='\0' )
		return 0;

	buf[MAX_QOS_MODE_LEN-1]='\0';
	strncpy(buf, qosMode, sizeof(buf)-1);
	for (i = 1,str1 = buf ; i<=QOS_PRIO_MAX ; i++,str1 = NULL){
		token = strtok_r(str1,delim,&saveptr1);
		if(token){
			prio = i+(MAX_QOS_QUEUE_NUM-QOS_PRIO_MAX);
			printf("%s: %d\n",token, prio);
			total_queue_need++;

			if(strcasecmp("INTERNET",token)==0)
				qos_mode_prio[QOS_PRIO_INTERNET]=prio;
			else if(strcasecmp("TR069",token)==0)
				qos_mode_prio[QOS_PRIO_TR069]=prio;
#ifdef CONFIG_CU
			else if(strcasecmp("VOICE",token)==0)
				qos_mode_prio[QOS_PRIO_VOIP]=prio;
#else
			else if(strcasecmp("VOIP",token)==0)
				qos_mode_prio[QOS_PRIO_VOIP]=prio;
#endif
			else if(strcasecmp("IPTV",token)==0)
				qos_mode_prio[QOS_PRIO_IPTV]=prio;
#ifdef SUPPORT_CLOUD_VR_SERVICE
			else if(strcasecmp("SPECIAL_SERVICE_VR",token)==0)
				qos_mode_prio[QOS_PRIO_VOIP]=prio;
#endif
		}
		else
			break;
	}

	//enable all of queues whatever qos template, other wise qos set some queue by ACS will not active
	return MAX_QOS_QUEUE_NUM;
	
	//return total_queue_need;
}

static void clear_ip_qos_app_rules()
{
	MIB_CE_IP_QOS_T entry;
	int total = mib_chain_total(MIB_IP_QOS_TBL);
	int i = (total - 1);

	for (; i >= 0 ; i--)
	{
		if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void *)&entry))
			continue;

#ifdef _PRMT_X_CT_COM_QOS_
		if (entry.modeTr69 == MODETR069 || entry.modeTr69 == MODEVOIP ||
			(entry.modeTr69 == MODEOTHER && (strcmp(entry.RuleName, "rule_IPTV") == 0 || strcmp(entry.RuleName, "rule_INTERNET") == 0)))
		{
			mib_chain_delete(MIB_IP_QOS_TBL, i);
		}
#endif
	}
	return;
}

#ifdef _CWMP_MIB_
static unsigned int find_max_CT_Class_instanNum(int direction)
{
	int total = mib_chain_total(MIB_IP_QOS_TBL);
	int i = 0, max = 0;
	MIB_CE_IP_QOS_T qos_entity;

	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_IP_QOS_TBL, i, &qos_entity))
			continue;

#ifdef _PRMT_X_CT_COM_QOS_
		if (qos_entity.modeTr69 != MODEIPTV && qos_entity.modeTr69 != MODEOTHER)
			continue;
#endif

		if (qos_entity.direction != direction)
			continue;

		if (qos_entity.InstanceNum > max)
			max = qos_entity.InstanceNum;
	}

	return max;
}
#endif

static int setup_ip_qos_app_rules(unsigned char *qosMode)
{
	MIB_CE_IP_QOS_APP_T entry;
	MIB_CE_IP_QOS_T qos_entry;
	MIB_CE_IP_QOS_Tp p = &qos_entry;
	MIB_CE_ATM_VC_T atm_entry;
	int i = 0, j = 0, total = 0, vcEntryNum = 0;
	unsigned int app_inst_num = 1;
	int lan_idx=0, max_lan_port_num=0, phyPortId=0;
	int ethPhyPortId = rtk_port_get_wan_phyID();

	clear_ip_qos_app_rules();

	mib_chain_clear(MIB_IP_QOS_APP_TBL);

	for(lan_idx = 0; lan_idx < ELANVIF_NUM; lan_idx++){
		phyPortId = rtk_port_get_lan_phyID(lan_idx);
		if (phyPortId != -1 && phyPortId != ethPhyPortId)
		{
			max_lan_port_num++;
			continue;
		}
	}

	if (!qosMode || qosMode[0] == '\0')
		return -1;
#ifdef CONFIG_CU
	if (strcasestr(qosMode,"VOICE"))
#else
	if (strcasestr(qosMode,"VOIP"))
#endif
	{
		MIB_CE_IP_QOS_APP_T app_entry;
		memset(&app_entry, 0, sizeof(MIB_CE_IP_QOS_APP_T));

		app_entry.appName = IP_QOS_APP_VOIP;
		app_entry.prior = qos_mode_prio[QOS_PRIO_VOIP];
		app_entry.InstanceNum = app_inst_num;
		app_inst_num++;

		mib_chain_add(MIB_IP_QOS_APP_TBL, &app_entry);
	}

	if (strcasestr(qosMode,"TR069"))
	{
		MIB_CE_IP_QOS_APP_T app_entry;
		memset(&app_entry, 0, sizeof(MIB_CE_IP_QOS_APP_T));

		app_entry.appName = IP_QOS_APP_TR069;
		app_entry.prior = qos_mode_prio[QOS_PRIO_TR069];
		app_entry.InstanceNum = app_inst_num;
		app_inst_num++;

		mib_chain_add(MIB_IP_QOS_APP_TBL, &app_entry);
	}

#if 0
	total = mib_chain_total(MIB_IP_QOS_APP_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_IP_QOS_APP_TBL, i, (void *)&entry))
			continue;

		if (entry.appName == IP_QOS_APP_VOIP && entry.prior)
		{
			memset(p, 0, sizeof(MIB_CE_IP_QOS_T));
			strcpy(p->RuleName, "rule_VOIP_1");
			p->enable = 1;
			p->direction = QOS_DIRECTION_UPSTREAM;
#ifdef CONFIG_IPV6
			p->IpProtocol = IPVER_IPV4; /* IPv4 */
#endif
			// local source port: 5060
			// protocol is UDP
			p->sPort = 5060;
			p->sPortRangeMax = 5060;
			p->protoType = PROTO_UDP;
			p->prior = entry.prior;
#ifdef _PRMT_X_CT_COM_QOS_
			p->cttypemap[0] = 5; //SPORT
			p->modeTr69 = MODEVOIP; //hide from web gui and TR-069
#endif
			mib_chain_add(MIB_IP_QOS_TBL, p);

			memset(p, 0, sizeof(MIB_CE_IP_QOS_T));
			strcpy(p->RuleName, "rule_VOIP_2");
			p->enable = 1;
			p->direction = QOS_DIRECTION_UPSTREAM;
#ifdef CONFIG_IPV6
			p->IpProtocol = IPVER_IPV4; /* IPv4 */
#endif
			// local source port: 9000 ~ 9010 ( rtp + t.38),
			p->sPort = 9000;
			p->sPortRangeMax = 9010;
			p->protoType = PROTO_UDP;
			p->prior = entry.prior;
#ifdef _PRMT_X_CT_COM_QOS_
			p->cttypemap[0] = 5; //SPORT
			p->modeTr69 = MODEVOIP; //hide from web gui and TR-069
#endif
			mib_chain_add(MIB_IP_QOS_TBL, p);
		}
		else if (entry.appName == IP_QOS_APP_TR069 && entry.prior)
		{
			char acsurl[256+1] = {0}, vStr[256+1] = {0};
			struct addrinfo *servinfo = NULL;
			struct sockaddr_in *sin;
			struct in_addr acsaddr = {0};

			memset(p, 0, sizeof(MIB_CE_IP_QOS_T));
			sprintf(p->RuleName, "%s", "rule_TR069");
			p->enable = 1;
			p->direction = QOS_DIRECTION_UPSTREAM;
#ifdef CONFIG_IPV6
			p->IpProtocol = IPVER_IPV4; /* IPv4 */
#endif
			p->prior = entry.prior;
#ifdef _PRMT_X_CT_COM_QOS_
			p->cttypemap[0] = 4; //DIP
			p->modeTr69 = MODETR069; //hide from web gui and TR-069
#endif

#ifdef CONFIG_USER_CWMP_TR069
			unsigned char dynamic_acs_url_selection = 0;
			mib_get_s(CWMP_DYNAMIC_ACS_URL_SELECTION, &dynamic_acs_url_selection, sizeof(dynamic_acs_url_selection));
			int acs_url_mib_id = CWMP_ACS_URL;
			if (dynamic_acs_url_selection)
			{
				acs_url_mib_id = RS_CWMP_USED_ACS_URL;
			}

			//get ACS URL, found the IP
			if (!mib_get(acs_url_mib_id, (void *)vStr))
			{
				fprintf(stderr, "Get mib value CWMP_ACS_URL failed!\n");
			}
			else
			{
				if (vStr[0])
				{
					set_endpoint(acsurl, vStr);
					servinfo = hostname_to_ip(acsurl, IPVER_IPV4);
					if (!servinfo)
					{
						fprintf(stderr, "failed to get HOST address\n");
					}
					else
					{
						sin = (struct sockaddr_in *)servinfo->ai_addr;
 						memcpy(&acsaddr, &(sin->sin_addr), sizeof(struct in_addr));
						freeaddrinfo(servinfo);

						memcpy(&(p->dip), &(acsaddr.s_addr), IP_ADDR_LEN);

						mib_chain_add(MIB_IP_QOS_TBL, p);
					}
				}
			}
#endif
		}
	}
#endif
#ifndef CONFIG_CU
	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < vcEntryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&atm_entry))
			continue;

#if 0
		if (strcasestr(qosMode, "INTERNET") && (atm_entry.applicationtype & X_CT_SRV_INTERNET))
		{
			memset(p, 0, sizeof(MIB_CE_IP_QOS_T));
			sprintf(p->RuleName, "%s", "rule_INTERNET");
			p->enable = 1;
			p->direction = QOS_DIRECTION_UPSTREAM;
#ifdef CONFIG_IPV6
			p->IpProtocol = IPVER_IPV4; /* IPv4 */
#endif
			p->prior = qos_mode_prio[QOS_PRIO_INTERNET];
#ifdef _PRMT_X_CT_COM_QOS_
			p->cttypemap[0] = 9; //WANInterface
			p->modeTr69 = MODEOTHER;
#endif
#ifdef _CWMP_MIB_
			p->InstanceNum = find_max_CT_Class_instanNum(QOS_DIRECTION_UPSTREAM) + 1;
#endif

			p->outif = atm_entry.ifIndex;
			mib_chain_add(MIB_IP_QOS_TBL, p);
		}
#endif
		if (strcasestr(qosMode, "IPTV") && (atm_entry.applicationtype & X_CT_SRV_OTHER))
		{
			memset(p, 0, sizeof(MIB_CE_IP_QOS_T));
			sprintf(p->RuleName, "%s", "rule_IPTV");
			p->enable = 1;
			p->direction = QOS_DIRECTION_UPSTREAM;
#ifdef CONFIG_IPV6
			p->IpProtocol = IPVER_IPV4; /* IPv4 */
#endif
			p->prior = qos_mode_prio[QOS_PRIO_IPTV];
#ifdef _PRMT_X_CT_COM_QOS_
			p->cttypemap[0] = 10; //LANInterface
			p->modeTr69 = MODEOTHER;
#endif
#ifdef _CWMP_MIB_
			p->InstanceNum = find_max_CT_Class_instanNum(QOS_DIRECTION_UPSTREAM) + 1;
#endif
			for (j = 0; j < max_lan_port_num; j++)
			{
				if (atm_entry.itfGroup & (0x1 << j))
				{
					if (p->phyPort == 0)
					{
						p->phyPort = (j + 1);
					}

					if (p->phyPort > 0 && (j + 1) > p->phyPort)
					{
						p->phyPort_end = (j + 1);
					}
				}
			}

			if (p->phyPort > 0)
			{
				mib_chain_add(MIB_IP_QOS_TBL, p);
			}
		}
	}
#endif
	return 0;
}

int add_appbased_qos_rule(MIB_CE_ATM_VC_Tp atm_entry)
{
	int i;
	int qEntryNum = 0;
	char apptypestr[MAX_WAN_NAME_LEN]={0};
	MIB_CE_IP_QOS_T qos_entry;


	if(!atm_entry)
		return -1;

	if((qEntryNum=mib_chain_total(MIB_IP_QOS_TBL)) < 0)
		return -1;

	generateWanName(atm_entry, apptypestr);
	for(i=0 ; i<qEntryNum ; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)&qos_entry))
			continue;

		if(qos_entry.applicationtype==atm_entry->applicationtype
			&& !memcmp(qos_entry.RuleName, apptypestr, sizeof(qos_entry.RuleName)-1))
			{
				return 0;
			}
	}

	memset(&qos_entry, 0, sizeof(MIB_CE_IP_QOS_T));
	qos_entry.enable = 1;

#ifdef CONFIG_IPV6
	switch(atm_entry->IpProtocol)
	{
		case IPVER_IPV4:
		case IPVER_IPV4_IPV6:
			qos_entry.IpProtocol = 1;
			break;
		case IPVER_IPV6:
			qos_entry.IpProtocol = 2;
			break;
		default:
			qos_entry.IpProtocol = 1;
			break;
	} 
#endif

	snprintf(qos_entry.RuleName, sizeof(qos_entry.RuleName), "%s", apptypestr);

	if(atm_entry->applicationtype & X_CT_SRV_TR069){
		qos_entry.prior = qos_mode_prio[QOS_PRIO_TR069];
	}
	else if(atm_entry->applicationtype & X_CT_SRV_VOICE){
		qos_entry.prior = qos_mode_prio[QOS_PRIO_VOIP];
	}
	else if(atm_entry->applicationtype & X_CT_SRV_OTHER){
		qos_entry.prior = qos_mode_prio[QOS_PRIO_IPTV];
	}
	else if(atm_entry->applicationtype & X_CT_SRV_SPECIAL_SERVICE_ALL){
		//No special service type for QoS template
		//FIX ME if spec is updated or clarified.
		qos_entry.prior = qos_mode_prio[QOS_PRIO_IPTV];
	}
#ifdef SUPPORT_CLOUD_VR_SERVICE
	else if(atm_entry->applicationtype & X_CT_SRV_SPECIAL_SERVICE_VR){
		qos_entry.prior = qos_mode_prio[QOS_PRIO_IPTV];
	}
#endif
	else if(atm_entry->applicationtype & X_CT_SRV_INTERNET){ // INTERNET
		qos_entry.prior = qos_mode_prio[QOS_PRIO_INTERNET];
	}
	else
	{
		printf("[%s:%d] MIB_IP_QOS_TBL does't support this template now\n",__func__, __LINE__);
		return -1;
	}

	qos_entry.enable = 1;
	qos_entry.classtype = 1;
	qos_entry.applicationtype = atm_entry->applicationtype;
	qos_entry.outif = atm_entry->ifIndex;

	if(!mib_chain_add(MIB_IP_QOS_TBL, &qos_entry))
	{
		return -1;
	}

	return 0;
}

int update_appbased_qos_tbl(unsigned char *qosMode)
{
	int i;
	int qEntryNum = 0;
	MIB_CE_IP_QOS_T qos_entry;
	MIB_CE_ATM_VC_T atm_entry;


	if((qEntryNum=mib_chain_total(MIB_IP_QOS_TBL)) <0)
		return -1;

	for (i = qEntryNum - 1; i >= 0; i--) {
		if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void *)&qos_entry))
			continue;

		if (qos_entry.classtype == 1) {
			if (1 != mib_chain_delete(MIB_IP_QOS_TBL, i)) {
				return -1;
			}
		}
	}

	setup_ip_qos_app_rules(qosMode);
	if((qEntryNum=mib_chain_total(MIB_ATM_VC_TBL)) <0)
		return -1;

	for(i=0 ; i<qEntryNum ; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&atm_entry))
			continue;

		add_appbased_qos_rule(&atm_entry);
	}
	return 0;
}

int delQoSRuleByMode(unsigned char *sub_qosMode)
{
	MIB_CE_IP_QOS_T Entry, *pEntry=&Entry;
	int  i,entryNum;

	printf("%s for mode %s\n",__func__,sub_qosMode);
	if ( !sub_qosMode || sub_qosMode[0]=='\0' )
		return -1;

	if((entryNum=mib_chain_total(MIB_IP_QOS_TBL)) <=0)
		return -1;

	for (i = entryNum - 1; i >= 0; i--) {
		if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void *)pEntry)) {
			printf("Get chain record error!\n");
			return -1;
		}

		if (strcasestr(pEntry->RuleName, sub_qosMode)) {
			printf("delete entry %s\n", pEntry->RuleName);
			mib_chain_delete(MIB_IP_QOS_TBL, i);
		}
	}

	return 0;
}

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_AP_E8B_QOS)
unsigned int getInstNum( char *name, char *objname )
{
	unsigned int num=0;

	if( (objname!=NULL)  && (name!=NULL) )
	{
		char buf[256],*tok;
		sprintf( buf, ".%s.", objname );
		tok = strstr( name, buf );
		if(tok)
		{
			tok = tok + strlen(buf);
			sscanf( tok, "%u.%*s", &num );
		}
	}

	return num;
}

int GetClsTypeValue(char *typevalue, MIB_CE_IP_QOS_CLASSFICATIONTYPE_T *p, unsigned int typenum, int maxflag)
{
	char ip6str[INET6_ADDRSTRLEN];
	int i = 0, total_wan = 0;
	MIB_CE_ATM_VC_T entry;
	
	if (typenum >= IP_QOS_CLASSFICATIONTYPE_MAX)
		return -1;

	switch (typenum) {
	case IP_QOS_CLASSFICATIONTYPE_SMAC:
		if(maxflag)
			mac_hex2string(p->smac_end, typevalue);
		else
			mac_hex2string(p->smac, typevalue);
		return 0;
#if !defined(CONFIG_USER_AP_E8B_QOS)
	case IP_QOS_CLASSFICATIONTYPE_DMAC:
		if(maxflag)
			mac_hex2string(p->dmac_end, typevalue);
		else
			mac_hex2string(p->dmac, typevalue);
		return 0;
#endif
	case IP_QOS_CLASSFICATIONTYPE_8021P:	
		if(maxflag)
			sprintf(typevalue, "%hhu", p->vlan1p_end);
		else
			sprintf(typevalue, "%hhu", p->vlan1p);
		return 0;
	case IP_QOS_CLASSFICATIONTYPE_SIP:	
		if(maxflag)
			strcpy(typevalue, inet_ntoa(*(struct in_addr *)p->sip_end));
		else
			strcpy(typevalue, inet_ntoa(*(struct in_addr *)p->sip));
		return 0;
	case IP_QOS_CLASSFICATIONTYPE_DIP:		
		if(maxflag)
			strcpy(typevalue, inet_ntoa(*(struct in_addr *)p->dip_end));
		else
			strcpy(typevalue, inet_ntoa(*(struct in_addr *)p->dip));
		return 0;
	case IP_QOS_CLASSFICATIONTYPE_SPORT:
		sprintf(typevalue, "%hu",maxflag ? p->sPortRangeMax : p->sPort);
		return 0;
	case IP_QOS_CLASSFICATIONTYPE_DPORT:		
		sprintf(typevalue, "%hu",maxflag ? p->dPortRangeMax : p->dPort);
		return 0;
	case IP_QOS_CLASSFICATIONTYPE_TOS:		
	#if defined(CONFIG_USER_AP_E8B_QOS)
		sprintf(typevalue, "%hhu",maxflag ? p->tos_end : p->tos);
	#else		
		sprintf(typevalue, "%hhu", p->tos);
	#endif
		return 0;
	case IP_QOS_CLASSFICATIONTYPE_DSCP:
		sprintf(typevalue, "%hhu",maxflag ? p->qosDscp_end : p->qosDscp);
		return 0;
#if defined(CONFIG_IPV6) && !defined(CONFIG_USER_AP_E8B_QOS)
	case IP_QOS_CLASSFICATIONTYPE_SIP6:
		inet_ntop(AF_INET6,(struct in6_addr *)&p->sip6,ip6str,INET6_ADDRSTRLEN);
		sprintf(typevalue,"%s",ip6str);
		return 0;
	case IP_QOS_CLASSFICATIONTYPE_DIP6:
		inet_ntop(AF_INET6,(struct in6_addr *)&p->dip6,ip6str,INET6_ADDRSTRLEN);
		sprintf(typevalue,"%s",ip6str);
		return 0;
      case IP_QOS_CLASSFICATIONTYPE_SPORT6:
		sprintf(typevalue, "%hu",maxflag ? p->sPort6RangeMax : p->sPort6);
	   	return 0;
      case IP_QOS_CLASSFICATIONTYPE_DPORT6:
		sprintf(typevalue, "%hu",maxflag ? p->dPort6RangeMax : p->dPort6);
	   	return 0;
	case IP_QOS_CLASSFICATIONTYPE_TrafficClass:
		sprintf(typevalue, "%hu",maxflag ? p->tc_end:p->tc);
        	return 0;
#endif
	case IP_QOS_CLASSFICATIONTYPE_WANINTERFACE:	
		total_wan = mib_chain_total(MIB_ATM_VC_TBL);
		for(i = 0; i < total_wan; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL,i,&entry))
				continue;
			if(entry.ifIndex== p->wanitf)
			{
				if(entry.ConIPInstNum)
					sprintf(typevalue,"InternetGatewayDevice.WANDevice.1.WANConnectionDevice.%u.WANIPConnection.%u.",entry.ConDevInstNum,entry.ConIPInstNum);
				else if(entry.ConPPPInstNum)
					sprintf(typevalue,"InternetGatewayDevice.WANDevice.1.WANConnectionDevice.%u.WANPPPConnection.%u.",entry.ConDevInstNum,entry.ConPPPInstNum);
				return 0;
			}
		}			
		return -1;
	case IP_QOS_CLASSFICATIONTYPE_LANINTERFACE:		
		if (maxflag) {
			if (p->phyPort_end> 0 && p->phyPort_end< 13) {
				strcpy(typevalue, LANString[p->phyPort_end- 1]);
				return 0;
			} else
				return -1;
		} else {
			if (p->phyPort > 0 && p->phyPort < 13) {
				strcpy(typevalue, LANString[p->phyPort - 1]);
				return 0;
			} else
				return -1;
		}
	case IP_QOS_CLASSFICATIONTYPE_ETHERTYPE:
		if(p->ethType == 0x0800)
			strcpy(typevalue,"IPv4");
		else if(p->ethType == 0x86dd)
			strcpy(typevalue,"IPv6");
		else 
			strcpy(typevalue,"");
		return 0;
#ifdef CONFIG_CMCC_ENTERPRISE
    case IP_QOS_CLASSFICATIONTYPE_VID:
		sprintf(typevalue, "%hu",maxflag ? p->vid_end:p->vid);
		return 0;
#endif
	default:
		return 0;
	}

}

int SetClsTypeValue(char *typevalue, MIB_CE_IP_QOS_CLASSFICATIONTYPE_T * p, int typenum, int maxflag)
{
	int i, bit1,bit0, ret, protocol=0;
	unsigned int uVal = 0;
	unsigned char	ip6[IP6_ADDR_LEN];
	unsigned char ip[IP_ADDR_LEN];
	unsigned char mac[MAC_ADDR_LEN];
	int wan_connum = 0, wan_ipnum = 0, wan_pppnum = 0, total_wan = 0;
	MIB_CE_ATM_VC_T entry;
	if (typenum < 0 || (typenum >= IP_QOS_CLASSFICATIONTYPE_MAX))
		return -1;

	protocol = getQosClassficationProtocol(p->cls_id);
	switch (typenum) {
	case IP_QOS_CLASSFICATIONTYPE_SMAC:
		for(i = 0; i < ETH_ALEN; i++) {
			bit1 = hex(typevalue[i*3]);
			bit0 = hex(typevalue[i*3+1]);
			if(bit0 == -1 || bit1 == -1)
				return -1;
			mac[i] = bit1*16+bit0;
		}

		if(maxflag)
			memcpy(p->smac_end,mac,sizeof(p->smac_end));
		else
			memcpy(p->smac,mac,sizeof(p->smac));
		return 0;
#if !defined(CONFIG_USER_AP_E8B_QOS)
	case IP_QOS_CLASSFICATIONTYPE_DMAC:
		for(i = 0; i < ETH_ALEN; i++) {
			bit1 = hex(typevalue[i*3]);
			bit0 = hex(typevalue[i*3+1]);
			if(bit0 == -1 || bit1 == -1)
				return -1;
			mac[i] = bit1*16+bit0;
		}

		if(maxflag)
			memcpy(p->dmac_end,mac,sizeof(p->dmac_end));
		else
			memcpy(p->dmac,mac,sizeof(p->dmac));
		return 0;
#endif
	case IP_QOS_CLASSFICATIONTYPE_8021P:		
		sscanf(typevalue, "%u", &uVal);
		if(uVal > 7)
			return -1;
		else
		{
			if(maxflag) 
				p->vlan1p_end = uVal;
			else
				p->vlan1p = uVal;
		}
		return 0;
	case IP_QOS_CLASSFICATIONTYPE_SIP:		
		ret = inet_aton(typevalue,(struct in_addr *)ip );
		if(ret == 0)
		{
			ret = inet_pton(AF_INET6, typevalue, ip6);
			if(ret == 0)
				return -1;
			else
			{
				memset(p->sip,0,IP_ADDR_LEN);
				memset(p->sip_end,0,IP_ADDR_LEN);
				memset(p->dip,0,IP_ADDR_LEN);
				memset(p->dip_end,0,IP_ADDR_LEN);
#if defined(CONFIG_IPV6) && !defined(CONFIG_USER_AP_E8B_QOS)
				memset(p->dip6,0,IP6_ADDR_LEN);
				p->classficationType = (1<<IP_QOS_CLASSFICATIONTYPE_SIP6);
				memcpy(p->sip6,ip6,IP6_ADDR_LEN);
#endif
				return 0;
			}
		}
		else
		{
			memset(p->dip,0,IP_ADDR_LEN);
			memset(p->dip_end,0,IP_ADDR_LEN);
#if defined(CONFIG_IPV6) && !defined(CONFIG_USER_AP_E8B_QOS)
			memset(p->sip6,0,IP6_ADDR_LEN);
			memset(p->dip6,0,IP6_ADDR_LEN);
#endif
			if(maxflag)
				memcpy((struct in_addr *)p->sip_end,ip,sizeof(struct in_addr));
			else
				memcpy((struct in_addr *)p->sip,ip,sizeof(struct in_addr));
			return 0;
		}

	case IP_QOS_CLASSFICATIONTYPE_DIP:		
		ret = inet_aton(typevalue,(struct in_addr *)ip );
		if(ret == 0)
		{
			ret = inet_pton(AF_INET6, typevalue, ip6);
			if(ret == 0)
				return -1;
			else
			{
				memset(p->sip,0,IP_ADDR_LEN);
				memset(p->sip_end,0,IP_ADDR_LEN);
				memset(p->dip,0,IP_ADDR_LEN);
				memset(p->dip_end,0,IP_ADDR_LEN);
#if defined(CONFIG_IPV6) && !defined(CONFIG_USER_AP_E8B_QOS)
				memset(p->sip6,0,IP6_ADDR_LEN);
				p->classficationType = (1<<IP_QOS_CLASSFICATIONTYPE_DIP6);
				memcpy(p->dip6,ip6,IP6_ADDR_LEN);
#endif
				return 0;
			}
		}
		else
		{
			memset(p->sip,0,IP_ADDR_LEN);
			memset(p->sip_end,0,IP_ADDR_LEN);
#if defined(CONFIG_IPV6) && !defined(CONFIG_USER_AP_E8B_QOS)
			memset(p->sip6,0,IP6_ADDR_LEN);
			memset(p->dip6,0,IP6_ADDR_LEN);
#endif
			if(maxflag)
				memcpy((struct in_addr *)p->dip_end,ip,sizeof(struct in_addr));
			else
				memcpy((struct in_addr *)p->dip,ip,sizeof(struct in_addr));
			return 0;
		}

	case IP_QOS_CLASSFICATIONTYPE_SPORT:		
		sscanf(typevalue, "%u", &uVal);
		if(uVal < 0 || uVal > 65535)
			return -1;
		else
		{
			if(protocol == IPVER_IPV4)
			{
				if(maxflag) 
					p->sPortRangeMax = uVal;
				else
					p->sPort = uVal;
			}
#if defined(CONFIG_IPV6) && !defined(CONFIG_USER_AP_E8B_QOS)
			else if(protocol == IPVER_IPV6)
			{
				p->classficationType = (1<<IP_QOS_CLASSFICATIONTYPE_SPORT6);
				if(maxflag) 
					p->sPort6RangeMax = uVal;
				else
					p->sPort6 = uVal;
			}
			else
			{
				p->classficationType = (1<<IP_QOS_CLASSFICATIONTYPE_SPORT | 1<<IP_QOS_CLASSFICATIONTYPE_SPORT6);
				if(maxflag) 
				{
					p->sPortRangeMax = uVal;
					p->sPort6RangeMax = uVal;
				}
				else
				{
					p->sPort = uVal;
					p->sPort6 = uVal;
				}
			}
#endif
		}
		return 0;
	case IP_QOS_CLASSFICATIONTYPE_DPORT:		
		sscanf(typevalue, "%u", &uVal);
		if(uVal < 0 || uVal > 65535)
			return -1;
		else
		{
			if(protocol == IPVER_IPV4)
			{
				if(maxflag) 
					p->dPortRangeMax = uVal;
				else
					p->dPort = uVal;
			}
#if defined(CONFIG_IPV6) && !defined(CONFIG_USER_AP_E8B_QOS)
			else if(protocol == IPVER_IPV6)
			{
				p->classficationType = (1<<IP_QOS_CLASSFICATIONTYPE_DPORT6);
				if(maxflag) 
					p->dPort6RangeMax = uVal;
				else
					p->dPort6 = uVal;
			}
			else
			{
				p->classficationType = (1<<IP_QOS_CLASSFICATIONTYPE_DPORT | 1<<IP_QOS_CLASSFICATIONTYPE_DPORT6);
				if(maxflag) 
				{
					p->dPortRangeMax = uVal;
					p->dPort6RangeMax = uVal;
				}
				else
				{
					p->dPort = uVal;
					p->dPort6 = uVal;
				}
			}
#endif
		}
		return 0;
	case IP_QOS_CLASSFICATIONTYPE_TOS:	
		sscanf(typevalue, "%u", &uVal);
	#if defined(CONFIG_USER_AP_E8B_QOS)
		if(uVal < 0 || uVal > 255)
			return -1;

		p->classficationType = (1<<IP_QOS_CLASSFICATIONTYPE_TOS);
		if(maxflag) 
			p->tos_end = uVal;
		else
			p->tos = uVal;
	#else
		if(uVal !=0 && uVal != 2 && uVal !=4 && uVal != 8 && uVal != 16)
			return -1;
		p->tos = uVal;
	#endif
		return 0;
	case IP_QOS_CLASSFICATIONTYPE_DSCP:		
		sscanf(typevalue, "%u", &uVal);
		if(uVal < 0 || uVal > 64)
			return -1;
		{
			if(protocol == IPVER_IPV4)
			{
				p->classficationType = (1<<IP_QOS_CLASSFICATIONTYPE_DSCP);
				if(maxflag) 
					p->qosDscp_end = uVal;
				else
					p->qosDscp = uVal;
			}
#if defined(CONFIG_IPV6) && !defined(CONFIG_USER_AP_E8B_QOS)
			else if(protocol == IPVER_IPV6)
			{
				p->classficationType = (1<<IP_QOS_CLASSFICATIONTYPE_TrafficClass);
				if(maxflag) 
					p->tc_end = uVal;
				else
					p->tc = uVal;
			}
			else
			{
				p->classficationType = (1<<IP_QOS_CLASSFICATIONTYPE_DSCP | 1<<IP_QOS_CLASSFICATIONTYPE_TrafficClass);
				if(maxflag) 
				{
					p->qosDscp_end = uVal;
					p->tc_end = uVal;
				}
				else
				{
					p->qosDscp = uVal;
					p->tc = uVal;
				}
			}
#endif
		}
		return 0;
#if defined(CONFIG_IPV6) && !defined(CONFIG_USER_AP_E8B_QOS)
	case IP_QOS_CLASSFICATIONTYPE_SIP6:
		ret = inet_pton(AF_INET6, typevalue, ip6);
		if(ret == 0)
			return -1;
		memcpy(p->sip6,ip6,IP6_ADDR_LEN);
		return 0;
	case IP_QOS_CLASSFICATIONTYPE_DIP6:
		ret = inet_pton(AF_INET6, typevalue, ip6);
		if(ret == 0)
			return -1;
		memcpy(p->dip6,ip6,IP6_ADDR_LEN);
		return 0;
      case IP_QOS_CLASSFICATIONTYPE_SPORT6:
		sscanf(typevalue, "%u", &uVal);
		if(uVal < 0 || uVal > 65535)
			return -1;
		else
		{
			if(maxflag) 
				p->sPort6RangeMax= uVal;
			else
				p->sPort6= uVal;
		} 
	   	return 0;
      case IP_QOS_CLASSFICATIONTYPE_DPORT6:
		sscanf(typevalue, "%u", &uVal);
		if(uVal < 0 || uVal > 65535)
			return -1;
		else
		{
			if(maxflag) 
				p->dPort6RangeMax = uVal;
			else
				p->dPort6 = uVal;
		}
	   	return 0;
	case IP_QOS_CLASSFICATIONTYPE_TrafficClass:
		sscanf(typevalue, "%u", &uVal);
		if(uVal < 0 || uVal > 255)
			return -1;
		if(maxflag)
			p->tc_end = uVal;
		else
			p->tc = uVal;
        	return 0;
#endif
	case IP_QOS_CLASSFICATIONTYPE_WANINTERFACE:		//WANInterface
		wan_connum=getInstNum( typevalue, "WANConnectionDevice" );
		wan_pppnum=getInstNum( typevalue, "WANPPPConnection" );
		wan_ipnum=getInstNum( typevalue, "WANIPConnection" );
		if(wan_connum == 0)
			return -1;
		if(wan_pppnum == 0 && wan_ipnum == 0)
			return -1;
		total_wan = mib_chain_total(MIB_ATM_VC_TBL);
		for(i = 0; i < total_wan; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL,i,&entry))
				continue;
			if(entry.ConDevInstNum == wan_connum)
			{
				if(entry.ConIPInstNum && entry.ConIPInstNum == wan_ipnum)
				{
					p->wanitf = entry.ifIndex;
					return 0;
				}
				else if(entry.ConPPPInstNum && entry.ConPPPInstNum == wan_pppnum)
				{
					p->wanitf = entry.ifIndex;
					return 0;
				}
			}
		}	
		return -1;
	case IP_QOS_CLASSFICATIONTYPE_LANINTERFACE:		//LANInterface
		for (i = 0; i < 12; i++) {
			if (strstr(typevalue, LANString[i]) != 0)
				break;
		}
		if (i <= 12) {
			if (maxflag == 1)
				p->phyPort_end= i + 1;
			else
				p->phyPort= i + 1;
		} else
			return -1;
		return 0;
	case IP_QOS_CLASSFICATIONTYPE_ETHERTYPE:
		if(!strcmp(typevalue,"IPv4"))
		{
			p->ethType = 0x0800;
		}
		else if(!strcmp(typevalue,"IPv6"))
		{
			p->ethType = 0x86dd;
		}
		else
			return -1;
		return 0;
#ifdef CONFIG_CMCC_ENTERPRISE
	case IP_QOS_CLASSFICATIONTYPE_VID:
		sscanf(typevalue, "%u", &uVal);
		if(uVal < 0 || uVal > 4095)
			return -1;
		if(maxflag)
			p->vid_end = uVal;
		else
			p->vid = uVal;
		return 0;
#endif
	default:
		return 0;
	}

}

#endif

int updateMIBforQosMode(unsigned char *qosMode)
{
	MIB_CE_IP_QOS_T qos_entity[4], *p=NULL;
	int need_queue_num;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, i;
	unsigned char vUChar;
	unsigned int vUInt;

	if ( !qosMode || qosMode[0]=='\0' )
		return -1;

	printf("%s: qosMode=%s\n",__func__,qosMode);
	memset(qos_mode_prio,0,sizeof(qos_mode_prio));

	//Now parsing the priority by qosMode
	need_queue_num = parsePriorityByQosMode(qosMode);
	printf("total need %d QoS queues\n",need_queue_num);

	update_appbased_qos_tbl(qosMode);
	return 0;
}

int setMIBforQosMode(unsigned char *qosMode)
{
	MIB_CE_IP_QOS_T qos_entity[4], *p=NULL;
	int need_queue_num;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, i;
	unsigned char vUChar;
	unsigned int vUInt;

	if ( !qosMode || qosMode[0]=='\0' )
		return -1;

	printf("%s: qosMode=%s\n",__func__,qosMode);
	memset(qos_mode_prio,0,sizeof(qos_mode_prio));
#if defined(_PRMT_X_CT_COM_QOS_)
	/* InternetGatewayDevice.X_CT-COM_UplinkQoS.EnableForceWeight = 0 */
	vUChar = 0;
	mib_set(MIB_QOS_ENABLE_FORCE_WEIGHT, &vUChar);

	/* InternetGatewayDevice.X_CT-COM_UplinkQoS.EnableDSCPMark = 0 */
	vUChar = 0;
	mib_set(MIB_QOS_ENABLE_DSCP_MARK, &vUChar);
#endif

	/* InternetGatewayDevice.X_CT-COM_UplinkQoS.Enable802-1_P = 0 */
	vUChar = 0;
	mib_set(MIB_QOS_ENABLE_1P, &vUChar);
	updateMIBforQosMode(qosMode);
	return 0;
}

#ifdef _PRMT_X_CT_COM_QOS_
int setup_tr069Qos(struct in_addr *qos_addr)
{
        int entry_num, i=0;
        MIB_CE_IP_QOS_T entry;

        if(qos_addr == NULL){
                printf("Error! %s: qos_addr is NULL\n", __FUNCTION__);
                return -1; //IP is invalid
        }

        entry_num = mib_chain_total(MIB_IP_QOS_TBL);

        for(i=0; i<entry_num; i++)
        {
                if(!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)&entry))
                    continue;
                if (strcmp(entry.RuleName, "rule_TR069") == 0) {
                        //printf("enter ip  %x\n", ((struct in_addr *)qos_addr->s_addr));
                        //printf("qEntry->dip %x\n", ((struct in_addr *)entry.dip)->s_addr);

                        if ((qos_addr->s_addr ==  ((struct in_addr *)entry.dip)->s_addr)) {
                                /* do nothing, ip & port is the same */
                                return 0;
                        }
                        else {
                                memcpy(&(entry.dip),&(qos_addr->s_addr),IP_ADDR_LEN);

                                if(!mib_chain_update(MIB_IP_QOS_TBL, (void *)&entry, i))
                                        printf("Error! %s: update %d failed \n", __FUNCTION__, MIB_IP_QOS_TBL);

                                stopIPQ();
                                setupIPQ();

	                            return 0;
                        }

                }
        }
        return -1; //can't found TR069 WAN
}
#endif
#endif //#ifdef CONFIG_E8B

#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
int rtk_qos_control_packet_protection(void)
{
	char cmd_str[256] = {0};

#ifdef CONFIG_RTL8686NIC
	sprintf(cmd_str, "echo rx_save_int_prio_to_skb 1 > /proc/rtl8686gmac/misc");
	va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#endif

	sprintf(cmd_str, "echo 192 > /proc/sys/net/core/netdev_hi_priomask"); // priority 7 & 6
	va_cmd("/bin/sh", 2, 1, "-c", cmd_str);

	unsigned int mark=0, qid=0;
	qid = getIPQosQueueNumber()-1;
	mark |= (qid)<<SOCK_MARK_QOS_SWQID_START;

	// Bridge WAN IGMP control packets forward to the highest queue
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", FW_CONTROL_PACKET_PROTECTION);
	//ebtables -t broute -I BROUTING -p IPv4 --ip-proto igmp -j mark --mark-or 0x70 --mark-target CONTINUE
	DOCMDINIT
	DOCMDARGVS(EBTABLES, DOWAIT, "-t %s -A %s -p IPv4 --ip-proto igmp -j mark --mark-or 0x%x --mark-target CONTINUE",
		"broute", FW_CONTROL_PACKET_PROTECTION , mark);
	
#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_qos_ingress_control_packet_protection_config();
	rtk_fc_qos_egress_control_packet_protection_config();
	//rtk_itms_qos_control_packet_protection();
#endif

	return 0;
}

int rtk_itms_qos_control_packet_protection(void)
{
#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_itms_qos_ingress_control_packet_protection_config();
	rtk_fc_itms_qos_egress_control_packet_protection_config();
#endif

	return 0;
}
#endif

#if defined(WLAN_SUPPORT) && defined(CONFIG_QOS_SUPPORT_DOWNSTREAM)
int rtk_qos_setup_downstream_wrr(int set)
{
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
	rtk_fc_qos_setup_downstream_wrr(set);
#else
	// TODO: for RG
#endif
	return 0;
}

void clear_downstream_queues(void)
{
	char sysbuf[256];
	int i;

	sprintf(sysbuf, "/bin/echo disable > /proc/fc/ctrl/wifi_tx_qos_mapping");
	printf("system(): %s\n", sysbuf);
	system(sysbuf);

	for(i=0 ; i<MAX_QOS_QUEUE_NUM ; i++)
	{
		sprintf(sysbuf, "/bin/echo %d 1 > /proc/fc/ctrl/wifi_tx_qos_mapping", i);
		printf("system(): %s\n", sysbuf);
		system(sysbuf);
	}
	rtk_qos_setup_downstream_wrr(0);
	return;
}

void setup_downstream_queues(void)
{
#define MAX_WIFI_TX_QOS_QUEUE	4
	int i, ipQoSTblEntryNum;
	int ipQoSQueueTblEntryNum;
	MIB_CE_IP_QOS_QUEUE_T ipQoSQueueTblEntry;
	MIB_CE_IP_QOS_T ipQoSTblEntry;
	int downstreamQoSQueueExist = 0;
	int wifiTxQoSQueueNum = MAX_WIFI_TX_QOS_QUEUE;
	char sysbuf[256];
	unsigned char policy;
	unsigned char wifiQueueIndex;

	if(!getDownstreamQosEnable())
		return;
	
	mib_get_s(MIB_QOS_DOWNSTREAM_POLICY, (void *)&policy, sizeof(policy));	
	if(policy == PLY_PRIO)
	{
		ipQoSQueueTblEntryNum = mib_chain_total(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL);
		for(i=0 ; i<ipQoSQueueTblEntryNum ; i++)
		{
			memset(&ipQoSQueueTblEntry, 0, sizeof(MIB_CE_IP_QOS_QUEUE_T));
			if(!mib_chain_get(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL, i, (void*)&ipQoSQueueTblEntry))
				continue;

			if(!ipQoSQueueTblEntry.enable)
				continue;
			
			if(ipQoSQueueTblEntry.wifiPrior == WIFI_QUEUE_PRIO_BACKGROUD)
				wifiQueueIndex = 0;
			else if(ipQoSQueueTblEntry.wifiPrior == WIFI_QUEUE_PRIO_BEST_EFFORT)
				wifiQueueIndex = 1;
			else if(ipQoSQueueTblEntry.wifiPrior == WIFI_QUEUE_PRIO_VEDIO)
				wifiQueueIndex = 2;
			else if(ipQoSQueueTblEntry.wifiPrior == WIFI_QUEUE_PRIO_VOICE)
				wifiQueueIndex = 3;

			sprintf(sysbuf, "/bin/echo %d %d > /proc/fc/ctrl/wifi_tx_qos_mapping", MAX_QOS_QUEUE_NUM-ipQoSQueueTblEntry.prior, wifiQueueIndex);
			printf("system(): %s\n", sysbuf);
			system(sysbuf);
		}
	
		ipQoSTblEntryNum = mib_chain_total(MIB_IP_QOS_TBL);
		for(i=0 ; i<ipQoSTblEntryNum ; i++)
		{		
			memset(&ipQoSTblEntry, 0, sizeof(ipQoSTblEntry));
			if(!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)&ipQoSTblEntry) || !ipQoSTblEntry.enable)
				continue;

			memset(&ipQoSQueueTblEntry, 0, sizeof(MIB_CE_IP_QOS_QUEUE_T));
			if (!mib_chain_get(MIB_IP_QOS_DOWNSTREAM_QUEUE_TBL, (ipQoSTblEntry.prior-1), (void*)&ipQoSQueueTblEntry))
				continue;

			if(ipQoSTblEntry.direction == QOS_DIRECTION_DOWNSTREAM
				&& ipQoSQueueTblEntry.enable)
			{
				downstreamQoSQueueExist = 1;
			}
		}

		if(downstreamQoSQueueExist)
		{
			sprintf(sysbuf, "/bin/echo enable > /proc/fc/ctrl/wifi_tx_qos_mapping");
			printf("system(): %s\n", sysbuf);
			system(sysbuf);
		}
	}
	else
	{
		rtk_qos_setup_downstream_wrr(1);
	}
}
#endif

int rtk_qos_tcp_smallack_protection(void)
{
	unsigned int qosEnable;
	unsigned char enable = 0, qid;
	char qos_mark[128];
#ifdef CONFIG_RTK_SKB_MARK2
	char qos_mark2[128];
#endif

	mib_get_s(MIB_QOS_TCP_SMALL_ACK_PROTECT, (void*)&enable, sizeof(enable));
	qosEnable = getQosEnable();
	
	va_cmd(IPTABLES, 19, 1, "-t", "mangle", "-D", FW_POSTROUTING, "-p", "tcp", "--tcp-flags", "ALL", "ACK", "-m", "length", "--length", "40:84", "-m", "conntrack", "--ctstate", "ESTABLISHED", "-j", FW_TCP_SMALLACK_PROTECTION);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", FW_TCP_SMALLACK_PROTECTION);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", FW_TCP_SMALLACK_PROTECTION);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 19, 1, "-t", "mangle", "-D", FW_POSTROUTING, "-p", "tcp", "--tcp-flags", "ALL", "ACK", "-m", "length", "--length", "60:104", "-m", "conntrack", "--ctstate", "ESTABLISHED", "-j", FW_TCP_SMALLACK_PROTECTION);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", FW_TCP_SMALLACK_PROTECTION);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", FW_TCP_SMALLACK_PROTECTION);
#endif
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_tcp_smallack_protect_set(0);
#endif
	
	if(enable && qosEnable)
	{
		qid = getIPQosQueueNumber()-1;
		// don't do sharemeter, so reset hw meter enable bit. sw meter not work for tcp ack in fc
		snprintf(qos_mark, sizeof(qos_mark), "0x%x/0x%x", SOCK_MARK_QOS_SWQID_TO_MARK(qid), SOCK_MARK_QOS_SWQID_BIT_MASK);
#ifdef CONFIG_RTK_SKB_MARK2
		snprintf(qos_mark2, sizeof(qos_mark2), "0x%x/0x%llx", 0, SOCK_MARK2_METER_HW_INDEX_ENABLE_BIT_MASK);
#endif
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", FW_TCP_SMALLACK_PROTECTION);
		va_cmd(IPTABLES, 8, 1, "-t", "mangle", "-A", FW_TCP_SMALLACK_PROTECTION, "-j", "MARK", "--set-mark", qos_mark);
#ifdef CONFIG_RTK_SKB_MARK2
		va_cmd(IPTABLES, 8, 1, "-t", "mangle", "-A", FW_TCP_SMALLACK_PROTECTION, "-j", "MARK2", "--set-mark2", qos_mark2);
#endif
		va_cmd(IPTABLES, 19, 1, "-t", "mangle", "-A", FW_POSTROUTING, "-p", "tcp", "--tcp-flags", "ALL", "ACK", "-m", "length", "--length", "40:84", "-m", "conntrack", "--ctstate", "ESTABLISHED", "-j", FW_TCP_SMALLACK_PROTECTION);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", FW_TCP_SMALLACK_PROTECTION);
		va_cmd(IP6TABLES, 8, 1, "-t", "mangle", "-A", FW_TCP_SMALLACK_PROTECTION, "-j", "MARK", "--set-mark", qos_mark);
#ifdef CONFIG_RTK_SKB_MARK2
		va_cmd(IP6TABLES, 8, 1, "-t", "mangle", "-A", FW_TCP_SMALLACK_PROTECTION, "-j", "MARK2", "--set-mark2", qos_mark2);
#endif
		va_cmd(IP6TABLES, 19, 1, "-t", "mangle", "-A", FW_POSTROUTING, "-p", "tcp", "--tcp-flags", "ALL", "ACK", "-m", "length", "--length", "60:104", "-m", "conntrack", "--ctstate", "ESTABLISHED", "-j", FW_TCP_SMALLACK_PROTECTION);
#endif
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		rtk_fc_tcp_smallack_protect_set(1);
#endif
	}
	else if(enable) //when not enable QoS, use ACL assign ACL priority
	{
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		rtk_fc_tcp_smallack_protect_set(2);
#endif
	}
	
	return 0;
}


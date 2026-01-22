/*
 *      Web server handler routines for IP QoS
 *
 */

/*-- System inlcude files --*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/wait.h>

#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "multilang.h"

#ifdef EMBED
#include <linux/config.h>
#else
#include "../../../uClibc/include/linux/autoconf.h"
#endif

#undef QOS_DEBUG
static int function_print = 1;
static int debug_print    = 1;

#define QUEUE_RULE_NUM_MAX     256
#ifdef CONFIG_USER_AP_HW_QOS
// hw qos: one shaping rule will take one hw queue
#define TRAFFICTL_RULE_NUM_MAX 7
#else
#define TRAFFICTL_RULE_NUM_MAX 256
#endif

#define DELIM      '&'
#define SUBDELIM   '|'
#define SUBDELIM1  "|"

//print debug information
#ifdef QOS_DEBUG
#define PRINT_FUNCTION do{ if(function_print) printf("%s: %s\n", __FILE__, __FUNCTION__);}while(0);
#define QOS_DEBUG(fmt, args...) do{if(debug_print) printf("QOS DEBUG: " fmt, ##args);}while(0)

#define  PRINT_QUEUE(pEntry)  \
    printf("[QUEUE]: ifIndex:%d, ifname:%s, desc:%s, prio:%d, queueKey:%d, enable:%d\n",  \
	    pEntry->ifIndex, pEntry->ifname, pEntry->desc, pEntry->prio, pEntry->queueKey, pEntry->enable);

#define PRINT_QUEUE_RULE(pEntry)  \
   printf("[QUEUE RULE]: ifidx:0x%x, name:%s, state:%d, prio:%d, mark dscp:0x%02x,\n"      \
	  "mark 8021p:%d, dscp:0x%02x, vlan 8021p:%d, phyPort:%d, prototype:%d, "    \
	  "srcip:%s, smaskbits:%d, dstip:%s, dmaskbits:%d, src port:%d-%d, dst port:%d-%d\n",               \
	   pEntry->outif, pEntry->RuleName, pEntry->enable, pEntry->prior, \
	   pEntry->m_dscp, pEntry->m_1p, pEntry->qosDscp, pEntry->vlan1p, pEntry->phyPort, pEntry->protoType,  \
	   inet_ntoa(*((struct in_addr*)&pEntry->sip)), pEntry->smaskbit, \
	   inet_ntoa(*((struct in_addr*)&pEntry->dip)), pEntry->dmaskbit, pEntry->sPort,          \
	   pEntry->sPortRangeMax, pEntry->dPort, pEntry->dPortRangeMax);

#define PRINT_TRAFFICTL_RULE(pEntry) \
    printf("[TRAFFIC CONTROL]: entryid:%d, ifIndex:%d, srcip:%s, smaskbits:%d, dstip:%s, dmaskbits:%d,"  \
	   "sport:%d, dport%d, protoType:%d, limitspeed:%d\n",                                                      \
	    pEntry->entryid, pEntry->ifIndex, inet_ntoa(*((struct in_addr*)&pEntry->srcip)),        \
	    pEntry->smaskbits, inet_ntoa(*((struct in_addr*)&pEntry->dstip)), pEntry->dmaskbits,                    \
	    pEntry->sport, pEntry->dport, pEntry->protoType, pEntry->limitSpeed);

#else

#define PRINT_FUNCTION do{}while(0);
#define QOS_DEBUG(fmt, args...) do{}while(0)
#define PRINT_QUEUE(pEntry)
#define PRINT_QUEUE_RULE(pEntry)
#define PRINT_TRAFFICTL_RULE(pEntry)

#endif

//show the wan interface list, using js code, must have waniflist array in js code
int ifWanList_tc(int eid, request * wp, int argc, char **argv)
{
	MIB_CE_ATM_VC_T entry;
	int entryNum = 0, i=0, nBytes = 0;
	char wanif[IFNAMSIZ] = {0};

	PRINT_FUNCTION

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	QOS_DEBUG("Total entry num:%d\n", entryNum);

	//default
	nBytes += boaWrite(wp, "waniflst.add(new it(\" \", \" \"));");

	for(i=0;i<entryNum;i++)
	{
		// Kaohj --- E8 don't care enableIpQos.
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &entry)||!entry.enableIpQos)
		//if(!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
		    continue;

		//getWanName(&entry, wanif);
		ifGetName(entry.ifIndex, wanif, sizeof(wanif));
		nBytes += boaWrite(wp, "waniflst.add(new it(\"%d|%s\", \"%s\"));",
			entry.ifIndex, wanif, wanif);
	}

	return nBytes;
}

int initQosLanif(int eid, request * wp, int argc, char **argv)
{
	int i, j;
	int nBytes = 0;
	
#if defined(CONFIG_QOS_SUPPORT_WLAN_INTF)
	MIB_CE_MBSSIB_T entry;
	int lanItfNum = 0;
#endif	

	for (i=0; i < SW_LAN_PORT_NUM; i++) {
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(getOpMode()!=BRIDGE_MODE){
			if(rtk_port_is_wan_logic_port(i))
				continue;
		}else{
#ifdef CONFIG_USER_AP_BRIDGE_MODE_QOS
			if(i==rtk_port_get_logPort(rtk_get_bridge_mode_uplink_port()))
				continue;
			else 
#endif
			if(i==getDefaultWanPort())
				continue;
		}
#endif
		nBytes += boaWrite(wp, "iffs.add(new it(\"%d\", \"LAN%d\"));\n", i+1, i+1);
	}
#if defined(CONFIG_QOS_SUPPORT_WLAN_INTF)
	for(i=0 ; i<NUM_WLAN_INTERFACE ; i++) {
		for (j=0 ; j<WLAN_SSID_NUM ; j++) {
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&entry);
			if(entry.wlanDisabled) {
				continue;
			}
			nBytes += boaWrite(wp, "iffs.add(new it(\"%d\", \"SSID%d\"));\n", (SW_LAN_PORT_NUM+lanItfNum+1),(WLAN_SSID_NUM*i)+(j+1));
			lanItfNum++;
		}
	}
#endif

	return nBytes;
}

#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
int initQosSpeedLimitLanif(int eid, request * wp, int argc, char **argv)
{
	int i;
	int nBytes = 0;
	int ret = 0;
#ifdef WLAN_SUPPORT
	MIB_CE_MBSSIB_T entry;
#ifdef WLAN_DUALBAND_CONCURRENT
	int orig_idx = wlan_idx;
#endif
#endif	
	
	for (i=0; i < ELANVIF_NUM; i++) {
		nBytes += boaWrite(wp, "iffs.add(new it(%d, \"LAN_%d\"));\n", i+1, i+1);
	}

#ifdef WLAN_SUPPORT
	for(i = 1; i <= (1+WLAN_MBSSID_NUM); i++){
#ifdef WLAN_DUALBAND_CONCURRENT
		wlan_idx = 0;
#endif
		ret = mib_chain_get(MIB_MBSSIB_TBL, i-1, &entry);
		if(ret == 0)
			printf("mib_chain_get  MIB_MBSSIB_TBL error!\n");
		if(entry.wlanDisabled==0)
			nBytes += boaWrite(wp, "iffs.add(new it(\"%d\", \"SSID%d\"));\n", ELANVIF_NUM+i,i);
	}
	
#ifdef WLAN_DUALBAND_CONCURRENT
	for(i=1; i<=(1+WLAN_MBSSID_NUM); i++){
		wlan_idx = 1;
		ret = mib_chain_get(MIB_MBSSIB_TBL, i-1, &entry);
		if(ret == 0)
			printf("mib_chain_get  MIB_MBSSIB_TBL error!\n");
		if(entry.wlanDisabled==0)
			nBytes += boaWrite(wp, "iffs.add(new it(\"%d\", \"SSID%d\"));\n", ELANVIF_NUM+1+WLAN_MBSSID_NUM+i ,i+1+WLAN_MBSSID_NUM);
	}
	wlan_idx = orig_idx;	
#endif
#endif

	return nBytes;
}
#endif

int initOutif(int eid, request * wp, int argc, char **argv)
{
	MIB_CE_ATM_VC_T vcEntry;
	int entryNum, i, j, nBytes = 0;
	//char wanif[MAX_WAN_NAME_LEN]={0};
	char wanif[IFNAMSIZ]={0};
	
	PRINT_FUNCTION

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);

	for(i=0; i<entryNum; i++)
	{
		// Kaohj --- E8 don't care enableIpQos.
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &vcEntry)||!vcEntry.enableIpQos)
		//if(!mib_chain_get(MIB_ATM_VC_TBL, i, &vcEntry))
			continue;

		//getWanName(&vcEntry, wanif);
		ifGetName(vcEntry.ifIndex, wanif, sizeof(wanif));
		nBytes += boaWrite(wp, "oifkeys.add(new it(\"%d\", \"%s\"));\n",
			vcEntry.ifIndex, wanif);
	}
	return nBytes;
}

int initRulePriority(int eid, request * wp, int argc, char **argv)
{
	int j, nBytes = 0;
	unsigned int queuenum = 0;

	PRINT_FUNCTION

    // only show priority. The entryNum +1 is priority.
	mib_get_s(MIB_IP_QOS_QUEUE_NUM, &queuenum, sizeof(queuenum));
	for (j=1; j<=queuenum; j++)
		nBytes += boaWrite(wp, "quekeys.add(new it(\"%d\", \"Queue %d\"));\n", j , j);

	return nBytes;
}

char* netmaskbits2str(const int netmaskbit, char* netmaskstr, int strlen)
{
	unsigned int netmaskaddr = 0, i=0;

	if(!netmaskstr || !netmaskbit)
		return NULL;
	for(i=0;i<netmaskbit;i++)
		netmaskaddr = netmaskaddr|(0x80000000>>i);
	netmaskaddr = htonl(netmaskaddr);//host byte order to network byte order
	strncpy(netmaskstr, inet_ntoa(*(struct in_addr*)&netmaskaddr), strlen );

	return netmaskstr;
}

/******************************************************************
 * NAME:    intPageQosRule
 * DESC:    initialize the qos rules by reading mib setting and
 *          format them to send them to webs to dispaly
 * ARGS:
 * RETURN:
 ******************************************************************/
int initQosRulePage(int eid, request * wp, int argc, char **argv)
{
	MIB_CE_IP_QOS_T qEntry;
	MIB_CE_ATM_VC_T       vcEntry;
	char saddr[16]={0}, daddr[16]={0}, smask[16]={0}, dmask[16]={0};
	char smacaddr[20], dmacaddr[20];
	int i=0, qEntryNum = 0, vcEntryNum = 0, nBytes = 0;
	char wanifname[16]={0};
#ifdef CONFIG_IPV6
	unsigned char 	sip6Str[48]={0}, dip6Str[48]={0};
#endif
#ifdef CONFIG_00R0
	char* conntypeStr;
	char* applicationStr;
#endif

#ifdef CONFIG_BRIDGE_EBT_DHCP_OPT
	char duid_mac[20]={0};
#endif
	int log_wan_port = 0;

	PRINT_FUNCTION
		
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(!mib_get_s(MIB_QOS_LOG_WAN_PORT, &log_wan_port, sizeof(log_wan_port))){
		printf("%s:%d Fail to get wan port\n",__FUNCTION__,__LINE__);
		return -1;
	}
	if(log_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
	nBytes += boaWrite(wp,"var WanPort = %d;\n",log_wan_port);
#endif

	//get number of  ip qos queue rules, if none, or cannot get, return
	if((qEntryNum=mib_chain_total(MIB_IP_QOS_TBL)) <=0)
		return -1;

	if((vcEntryNum=mib_chain_total(MIB_ATM_VC_TBL)) <=0)
		return -1;
	
	for(i=0;i<qEntryNum; i++)
	{
		char phyportName[8] = {0};
		if(!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)&qEntry))
			continue;
		
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(qEntry.logic_wan_port != log_wan_port)
			continue;
#endif
		//src addr
		snprintf(saddr, 16, "%s", inet_ntoa(*((struct in_addr*)&(qEntry.sip))));

		//src subnet mask
		if(!netmaskbits2str(qEntry.smaskbit, smask, 16))
		{
			smask[0] = '\0';
		}

		//dst addr
		snprintf(daddr, 16, "%s", inet_ntoa(*((struct in_addr*)&(qEntry.dip))));
		//dst subnet mask
		if(!netmaskbits2str(qEntry.dmaskbit, dmask, 16))
		{
			dmask[0] = '\0';
		}

#ifdef CONFIG_IPV6
		inet_ntop(PF_INET6, (struct in6_addr *)qEntry.sip6, sip6Str, sizeof(sip6Str));
		inet_ntop(PF_INET6, (struct in6_addr *)qEntry.dip6, dip6Str, sizeof(dip6Str));
#endif

		// src mac
		snprintf(smacaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
			qEntry.smac[0], qEntry.smac[1],
			qEntry.smac[2], qEntry.smac[3],
			qEntry.smac[4], qEntry.smac[5]);

		// dst mac
		snprintf(dmacaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
			qEntry.dmac[0], qEntry.dmac[1],
			qEntry.dmac[2], qEntry.dmac[3],
			qEntry.dmac[4], qEntry.dmac[5]);

#ifdef CONFIG_BRIDGE_EBT_DHCP_OPT
		//ifdef DHCPOPT
		// duid mac
		snprintf(duid_mac, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
			qEntry.duid_mac[0], qEntry.duid_mac[1],
			qEntry.duid_mac[2], qEntry.duid_mac[3],
			qEntry.duid_mac[4], qEntry.duid_mac[5]);
#endif

		if(qEntry.outif==65535)
		{
			wanifname[sizeof(wanifname)-1]='\0';
			strncpy(wanifname,"Any",sizeof(wanifname)-1);
		}
		else
			ifGetName(qEntry.outif,wanifname,sizeof(wanifname));

#ifdef CONFIG_00R0
		if (qEntry.applicationtype == X_CT_SRV_TR069)
			conntypeStr = "TR069";
		else if (qEntry.applicationtype == X_CT_SRV_INTERNET)
			conntypeStr = "INTERNET";
		else if (qEntry.applicationtype == X_CT_SRV_OTHER)
			conntypeStr = "OTHER";
		else if (qEntry.applicationtype == X_CT_SRV_VOICE)
			conntypeStr = "VOICE";
		else if (qEntry.applicationtype == X_CT_SRV_INTERNET+X_CT_SRV_TR069)
			conntypeStr = "INTERNET_TR069";
		else if (qEntry.applicationtype == X_CT_SRV_VOICE+X_CT_SRV_TR069)
			conntypeStr = "VOICE_TR069";
		else if (qEntry.applicationtype == X_CT_SRV_VOICE+X_CT_SRV_INTERNET)
			conntypeStr = "VOICE_INTERNET";
		else if (qEntry.applicationtype == X_CT_SRV_VOICE+X_CT_SRV_INTERNET+X_CT_SRV_TR069)
			conntypeStr = "VOICE_INTERNET_TR069";
		else
			conntypeStr = "None";

		if (qEntry.application == 21)
			applicationStr = "Telnet";
		else if (qEntry.application == 22)
			applicationStr = "SSH";
		else if (qEntry.application == 23)
			applicationStr = "FTP";
		else if (qEntry.application == 25)
			applicationStr = "SMTP";
		else if (qEntry.application == 53)
			applicationStr = "DNS";
		else if (qEntry.application == 80)
			applicationStr = "HTTP";
		else if (qEntry.application == 110)
			applicationStr = "POP3";
		else if (qEntry.application == 123)
			applicationStr = "NTP";
		else if (qEntry.application == 443)
			applicationStr = "HTTPS";
		else if (qEntry.application == 1703)
			applicationStr = "L2TP";
		else if (qEntry.application == 5060)
			applicationStr = "SIP";
		else if (qEntry.application == 7547)
			applicationStr = "TR069";		
		else
			applicationStr = "None";		
#endif

		//now write into webs using boaWrite function
		nBytes += boaWrite(wp, "rules.push(\n"
			"new it_nr(unescapeHTML(\"%s\"),    \n"  //qos queue rule name
			"new it(\"ipqos_rule_type\",%d),\n"  //qos rule type
			"new it(\"index\",%d),\n"  //index of queue rule(identifier)
			"new it(\"state\",%d),\n"  //enable or disable
			"new it(\"qos_order\",%d),\n"  //order of queue rule
			"new it(\"prio\", \"%d\"),\n"  //queue priority, queueKey
			"new it(\"outif\", \"%d\"),\n"  //queue priority, queueKey
			"new it(\"wanifname\",  \"%s\"),\n" //source ip6
#ifdef CONFIG_00R0			
			"new it(\"conntype\",  \"%d\"),\n" //Connect Type of WAN
			"new it(\"conntypeStr\",  \"%s\"),\n" //Connect Type of WAN
			"new it(\"application\",  \"%d\"),\n" //Applications
			"new it(\"applicationStr\",  \"%s\"),\n" //Applications
#endif			
#ifdef CONFIG_IPV6
			//"new it(\"ipversion\",%d),\n"  //ipv4 or ipv6
			"new it(\"IpProtocolType\",%d),\n"  //ipv4 or ipv6
			"new it(\"sip6\",  \"%s\"),\n" //source ip6
			"new it(\"dip6\",  \"%s\"),\n" //dst ip6
			"new it(\"sip6PrefixLen\",%d),\n"  //source ip6 Prefix Len
			"new it(\"dip6PrefixLen\",%d),\n"  //dst ip6 Prefix Len
#endif
			//"new it(\"mvid\",%d),\n"   //VLAN ID
			"new it(\"mdscp\",%d),\n"   //dscp mark
			"new it(\"m1p\",  %d),\n"     //802.1p mark for wan interface
			"new it(\"vlan1p\",%d),\n"    //802.1p match for packet
			"new it(\"ethType\",\"%04x\"),\n"    //Ethernet Type match for packet
			"new it(\"phypt\", %d),\n"    //Lan phy port number
			"new it(\"proto\",%d),\n"     //protocol index, reference to mib.h
			"new it(\"dscp\", %d),\n"     //dscp
			"new it(\"sip\",  \"%s\"),\n" //source ip, if stype=1, it is DHCP OPT 60
			"new it(\"smsk\", \"%s\"),\n" //source ip subnet mask
			"new it(\"dip\",  \"%s\"),\n"
			"new it(\"dmsk\", \"%s\"),\n"
			"new it(\"spts\", %d),\n"     //source port start
			"new it(\"spte\", %d),\n"     //source port end
			"new it(\"dpts\", %d),\n"
			"new it(\"dpte\", %d),\n"
			"new it(\"dhcpopt_type_select\", \"%d\"),\n"
			"new it(\"opt60_vendorclass\", \"%s\"),\n"
			"new it(\"opt61_iaid\", \"%d\"),\n"
			"new it(\"dhcpopt61_DUID_select\", \"%d\"),\n"
			"new it(\"duid_hw_type\", \"%d\"),\n"
			"new it(\"duid_mac\", \"%s\"),\n"
			"new it(\"duid_time\", \"%d\"),\n"
			"new it(\"duid_ent_num\", \"%d\"),\n"
			"new it(\"duid_ent_id\", \"%s\"),\n"
			"new it(\"opt125_ent_num\", \"%d\"),\n"
			"new it(\"opt125_manufacturer\", \"%s\"),\n"
			"new it(\"opt125_product_class\", \"%s\"),\n"
			"new it(\"opt125_model\", \"%s\"),\n"
			"new it(\"opt125_serial\", \"%s\"),\n"
			"new it(\"smac\", \"%s\"), \n"  //source mac address,now supported now, always 00:00:00:00:00:00
			"new it(\"smacw\",\"%s\"), \n"  //source mac address wildword
			"new it(\"dmac\", \"%s\"), \n"
			"new it(\"dmacw\",\"%s\")));\n",
			strValToASP(qEntry.RuleName), qEntry.ipqos_rule_type, i, !!(qEntry.enable),
			qEntry.QoSOrder,
			qEntry.prior,
			qEntry.outif,
			wanifname,
#ifdef CONFIG_00R0
			qEntry.applicationtype,
			conntypeStr,
			qEntry.application,
			applicationStr,
#endif 
#ifdef CONFIG_IPV6
			qEntry.IpProtocol, sip6Str, dip6Str, qEntry.sip6PrefixLen, qEntry.dip6PrefixLen,
#endif
			//qEntry.m_vid,qEntry.m_dscp, qEntry.m_1p, qEntry.vlan1p, *(unsigned short *)&(qEntry.ethType), qEntry.phyPort, qEntry.protoType, qEntry.qosDscp,
			qEntry.m_dscp, qEntry.m_1p, qEntry.vlan1p, *(unsigned short *)&(qEntry.ethType), qEntry.phyPort, qEntry.protoType, qEntry.qosDscp,
			saddr, smask, daddr, dmask, qEntry.sPort,
			qEntry.sPortRangeMax, qEntry.dPort, qEntry.dPortRangeMax,
#ifdef CONFIG_BRIDGE_EBT_DHCP_OPT
			qEntry.dhcpopt_type,qEntry.opt60_vendorclass,qEntry.opt61_iaid,
			qEntry.opt61_duid_type,qEntry.duid_hw_type,duid_mac,
			qEntry.duid_time,qEntry.duid_ent_num,qEntry.duid_ent_id,
			qEntry.opt125_ent_num,qEntry.opt125_manufacturer,
			qEntry.opt125_product_class,qEntry.opt125_model,qEntry.opt125_serial,
#else
			0,"",0,
			0,0,"",
			0,0,"",
			0,"",
			"","","",
#endif
			smacaddr, "00:00:00:00:00:00", dmacaddr, "00:00:00:00:00:00");
	}

	return nBytes;
}

//Used to get subnet mask bit number
static int getNetMaskBit(char* netmask)
{
	unsigned int bits = 0, mask = 0;
	int i=0, flag = 0;

	if(!netmask||strlen(netmask)>15)
		return 0;
	mask = inet_network(netmask);
	for(;i<32;i++)
	{
		if(mask&(0x80000000>>i)) {
			if(flag)
				return 0;
			else
				bits++;
		}
		else {
	    		flag = 1;
		}
	}
	return bits;
}

/*
 * Return index of this rule
 * -1 on fail
 */
static int parseRuleArgs(char* args, MIB_CE_IP_QOS_Tp pEntry)
{
	char* p=NULL, *tmp=NULL, buff[32] = {0};
	int idx = 0;
	int ret;
#ifdef CONFIG_IPV6
	char buff2[48+1] = {0};
	struct in6_addr ip6Addr;
#endif

	pEntry->m_iptos = 0xff;

	//get index
	p = strstr(args, "index=");
	p +=strlen("index=");
	if(*p == DELIM) {
		//pEntry->index = 0;
		ret = 0;
	} else {
		//pEntry->index = strtol(p, &tmp, 0);
		ret = strtol(p, &tmp, 0);
		if(*tmp != DELIM) return -1;
	}

	//get qos_rule_type
	//qos_rule_type
	p = strstr(args, "qos_rule_type=");
	p += strlen("qos_rule_type=");
	if(*p==DELIM) return 1;
	pEntry->ipqos_rule_type = strtol(p, &tmp, 0);
	if(*tmp != DELIM) return -1; 
	printf("pEntry->ipqos_rule_type = %d\n",pEntry->ipqos_rule_type);

	//get RuleName
	p = strstr(args, "name=");
	for(p+=strlen("name="); *p!=DELIM&&idx<sizeof(pEntry->RuleName)-1;p++)
		pEntry->RuleName[idx++] = *p;
	
	//state
	#if 1 // always enabled
	pEntry->enable = 1;
	#else
	p = strstr(args, "state=");
	p += strlen("state=");
	if(*p != DELIM) {
		pEntry->enable = strtol(p, &tmp, 0);
		if(*tmp != DELIM)
			pEntry->enable = 1;//default
	} else {
		pEntry->enable = 1;//default
	}
	#endif

	//get order
	p = strstr(args, "order=");
	p += strlen("order=");
	if(*p == DELIM)//default
	{
		/* w.o input value, set to zero */
		pEntry->QoSOrder = 0;
	}
	else {
		pEntry->QoSOrder = strtol(p, &tmp, 10);
		printf("[%s(%d)]pEntry->QoSOrder = %d\n",__func__,__LINE__,pEntry->QoSOrder);
		if(*tmp != DELIM) return -1;//invalid 802.1p value
	}


	//get prio
	p = strstr(args, "prio=");
	p += strlen("prio=");
	if(*p==DELIM) return 1;
	pEntry->prior = strtol(p, &tmp, 0);
	if(*tmp != DELIM) return -1;//invalid dscp value

	//get ifIndex, cmode
	p = strstr(args, "outif=");
	p += strlen("outif=");
	if(*p==SUBDELIM||*p==DELIM) return 1;
	//ifIndex
	pEntry->outif = strtol(p, &tmp, 0);
	// Mason Yu. t123
	//if(*tmp != SUBDELIM) return -1;
	if(*tmp != DELIM) return -1;

#if 0
	//get vid
	p = strstr(args, "markvid=");
	p += strlen("markvid=");
	if(*p == DELIM)//default
		pEntry->m_vid = 0;
	else {
		pEntry->m_vid = strtol(p, &tmp, 10);
		if(*tmp != DELIM) return -1;//invalid vid value
	}
#endif
	//mark 802.1p
	p = strstr(args, "mark1p=");
	p += strlen("mark1p=");
	if(*p == DELIM)//default
		pEntry->m_1p = 0;
	else {
		pEntry->m_1p = strtol(p, &tmp, 16);
		if(*tmp != DELIM) return -1;//invalid 802.1p value
	}

	//mark dscp
	p = strstr(args, "markdscp=");
	p += strlen("markdscp=");
	if(*p == DELIM)//default
		pEntry->m_dscp = 0;
	else {
		pEntry->m_dscp = strtol(p, &tmp, 0);
		if(*tmp != DELIM) return -1;//invalid dscp value
	}

	if(pEntry->ipqos_rule_type==IPQOS_PORT_BASE)
	{
		//phy port num
		p = strstr(args, "phyport=");
		p +=strlen("phyport=");
		if(*p == DELIM) {
			pEntry->phyPort = 0;//default phy port, none
		} else {
			pEntry->phyPort = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return 1;
		}

		//dscp match
		p = strstr(args, "matchdscp=");
		p += strlen("matchdscp=");
		if(*p == DELIM) {//default
			pEntry->qosDscp = 0;
		} else {
			pEntry->qosDscp = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1;
		}
	}
	else if(pEntry->ipqos_rule_type==IPQOS_IP_PROTO_BASE)
	{
		//protocol
		p = strstr(args, "proto=");
		p += strlen("proto=");
		if(*p == DELIM) {//default, none
			pEntry->protoType = PROTO_NONE;
		} else {
			pEntry->protoType = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1;
		}

		//dscp match
		p = strstr(args, "matchdscp=");
		p += strlen("matchdscp=");
		if(*p == DELIM) {//default
			pEntry->qosDscp = 0;
		} else {
			pEntry->qosDscp = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1;
		}

#ifdef CONFIG_IPV6
		//IPVersion
		p = strstr(args, "IPversion=");
		p += strlen("IPversion=");
		if(*p == DELIM)//default
			pEntry->IpProtocol = IPVER_IPV4_IPV6;
		else {
			pEntry->IpProtocol = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1;//invalid dscp value
		}

		// If this is a IPv4 rule
		if ( pEntry->IpProtocol == IPVER_IPV4) {
#endif
			//ip source address
			p = strstr(args, "sip=");
			p += strlen("sip=");
			if(*p==DELIM) {//default
				memset(pEntry->sip, 0, IP_ADDR_LEN);
			} else {
				for(idx=0; *p != DELIM&&idx<sizeof(buff)-1; ++p)
					buff[idx++] = *p;
				buff[idx] = '\0';
				{//ip address
					inet_aton(buff, (struct in_addr *)&pEntry->sip);
				}
			}

			//source ip netmaskbit
			p = strstr(args, "smask=");
			p += strlen("smask=");
			if(*p==DELIM) {//default
				pEntry->smaskbit =0;
			} else {
				for(idx=0; *p != DELIM&&idx<sizeof(buff)-1; ++p)
					buff[idx++] = *p;
				buff[idx] = '\0';
				pEntry->smaskbit = getNetMaskBit(buff);
			}

			//ip dest address
			p = strstr(args, "dip=");
			p += strlen("dip=");
			if(*p==DELIM) {//default
				memset(pEntry->dip, 0, IP_ADDR_LEN);
			} else {
				for(idx=0; *p != DELIM&&idx<sizeof(buff)-1; ++p)
					buff[idx++] = *p;
				buff[idx] = '\0';
				{//ip address
					inet_aton(buff, (struct in_addr *)&pEntry->dip);
				}
			}

			//destination ip netmaskbit
			p = strstr(args, "dmask=");
			p += strlen("dmask=");
			if(*p==DELIM) {//default
				pEntry->dmaskbit =0;
			} else {
				for(idx=0; *p != DELIM&&idx<sizeof(buff)-1; ++p)
					buff[idx++] = *p;
				buff[idx] = '\0';
				pEntry->dmaskbit = getNetMaskBit(buff);
			}
#ifdef CONFIG_IPV6
		}
#endif

#ifdef CONFIG_IPV6
		// If it is a IPv6 rule.
		if ( pEntry->IpProtocol == IPVER_IPV6 )
		{
			//ip6 source address
			p = strstr(args, "sip6=");
			p += strlen("sip6=");
			if(*p==DELIM) {//default
				memset(pEntry->sip6, 0, IP6_ADDR_LEN);
			} else {
				for(idx=0; *p != DELIM&&idx<sizeof(buff2)-1; ++p)
					buff2[idx++] = *p;
				buff2[idx] = '\0';
				{//ip address
					inet_pton(PF_INET6, buff2, &ip6Addr);
					memcpy(pEntry->sip6, &ip6Addr, sizeof(pEntry->sip6));
				}
			}

			//ip6 dest address
			p = strstr(args, "dip6=");
			p += strlen("dip6=");
			if(*p==DELIM) {//default
				memset(pEntry->dip6, 0, IP6_ADDR_LEN);
			} else {
				for(idx=0; *p != DELIM&&idx<sizeof(buff2)-1; ++p)
					buff2[idx++] = *p;
				buff2[idx] = '\0';
				{//ip address
					inet_pton(PF_INET6, buff2, &ip6Addr);
					memcpy(pEntry->dip6, &ip6Addr, sizeof(pEntry->dip6));
				}
			}

			// ip6 src IP prefix Len
			p = strstr(args, "sip6PrefixLen=");
			p += strlen("sip6PrefixLen=");
			if(*p == DELIM)//default
				pEntry->sip6PrefixLen = 0;
			else {
				pEntry->sip6PrefixLen = strtol(p, &tmp, 0);
				if(*tmp != DELIM) return -1;//invalid dscp value
			}

			// ip6 dst IP prefix Len
			p = strstr(args, "dip6PrefixLen=");
			p += strlen("dip6PrefixLen=");
			if(*p == DELIM)//default
				pEntry->dip6PrefixLen = 0;
			else {
				pEntry->dip6PrefixLen = strtol(p, &tmp, 0);
				if(*tmp != DELIM) return -1;//invalid dscp value
			}
		}
#endif

		//src port start
		p = strstr(args, "spts=");
		p += strlen("spts=");
		if(*p==DELIM) {//default
			pEntry->sPort =0;
		} else {
			pEntry->sPort = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1;
		}

		//src port end
		p = strstr(args, "spte=");
		p += strlen("spte=");
		if(*p==DELIM) {//default
			pEntry->sPortRangeMax =0;
		} else {
			pEntry->sPortRangeMax = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1;
		}

		//dst port start
		p = strstr(args, "dpts=");
		p += strlen("dpts=");
		if(*p==DELIM) {//default
			pEntry->dPort =0;
		} else {
			pEntry->dPort = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1;
		}

		//dst port end
		p = strstr(args, "dpte=");
		p += strlen("dpte=");
		if(*p==DELIM) {//default
			pEntry->dPortRangeMax =0;
		} else {
			pEntry->dPortRangeMax = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1;
		}	
	}
	else if(pEntry->ipqos_rule_type==IPQOS_MAC_BASE)
	{
	//smac
	p = strstr(args, "smac=");
	p += strlen("smac=");
	if (*p==DELIM) {//default
		memset(pEntry->smac, 0, MAC_ADDR_LEN);
	} else {
		for(idx=0; *p != DELIM&&idx<32; ++p) {
			if (*p!=':')
				buff[idx++] = *p;
		}
		rtk_string_to_hex(buff, pEntry->smac, 12);
	}

	//dmac
	p = strstr(args, "dmac=");
	p += strlen("dmac=");
	if (*p==DELIM) {//default
		memset(pEntry->dmac, 0, MAC_ADDR_LEN);
	} else {
		for(idx=0; *p != DELIM&&idx<32; ++p) {
			if (*p!=':')
				buff[idx++] = *p;
		}
		rtk_string_to_hex(buff, pEntry->dmac, 12);
	}
	}
	else if(pEntry->ipqos_rule_type==IPQOS_ETHERTYPE_BASE)
	{
	//ethType
	p = strstr(args, "ethType=");
	p += strlen("ethType=");	
	if (*p==DELIM) {//default
		memset(&(pEntry->ethType), 0, 2);
	} else {
		// Mason Yu
		rtk_string_to_hex(p, (unsigned char *)&(pEntry->ethType), 4);
		pEntry->ethType = ntohs(pEntry->ethType);
		//printf("pEntry->ethType=0x%x\n", *(unsigned short *)&(pEntry->ethType));
	}	
	}
#ifdef CONFIG_00R0	
	else if (pEntry->ipqos_rule_type==IPQOS_CONNTYPE_BASE)
	{
		//Connect Type of WAN 
		p = strstr(args, "conntype=");
		p += strlen("conntype=");
		if(*p == DELIM)//default
			pEntry->applicationtype = 0;
		else {
			pEntry->applicationtype = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1;//invalid connection Type
		}
	}
	else if (pEntry->ipqos_rule_type==IPQOS_APPLICATION_BASE)
	{
		//Applications
		p = strstr(args, "application=");
		p += strlen("application=");
		if(*p == DELIM)//default
			pEntry->application = 0;
		else {
			pEntry->application = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1;//invalid connection Type
		}
	}	
#endif		
#ifdef CONFIG_BRIDGE_EBT_DHCP_OPT
	else if(pEntry->ipqos_rule_type==IPQOS_DHCPOPT_BASE)
	{
		//dhcp opt_type
		p = strstr(args, "dhcpopt_type=");
		p += strlen("dhcpopt_type=");
		if(*p==DELIM) //default
			pEntry->dhcpopt_type = 0;
		else
			pEntry->dhcpopt_type = strtol(p, &tmp, 0);
		if(*tmp != DELIM) return -1; 

		if(pEntry->dhcpopt_type == IPQOS_DHCPOPT_60 )
		{
			//vendor_class
			p = strstr(args, "vender_class=");
			p += strlen("vender_class=");
			if (*p==DELIM) {//default
				memset(pEntry->opt60_vendorclass, 0, OPTION_LEN);
			} else {
				for(idx=0; *p != DELIM&&idx<32; ++p) {
					if (*p!=':')
						buff[idx++] = *p;
				}
				pEntry->opt60_vendorclass[sizeof(pEntry->opt60_vendorclass)-1]='\0';
				strncpy(pEntry->opt60_vendorclass,buff,sizeof(pEntry->opt60_vendorclass)-1);
			}
		}
		else if(pEntry->dhcpopt_type == IPQOS_DHCPOPT_61 )
		{
			//opt61_iaid
			p = strstr(args, "opt61_iaid=");
			p += strlen("opt61_iaid=");
			if(*p==DELIM) //default
				pEntry->opt61_iaid = 0;
			else
				pEntry->opt61_iaid = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1; 


			//dhcpopt_duid_type
			p = strstr(args, "dhcpopt_duid_type=");
			p += strlen("dhcpopt_duid_type=");
			if(*p==DELIM) //default
				pEntry->opt61_duid_type = 0;
			else
				pEntry->opt61_duid_type = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1; 

			//duid_hw_type
			p = strstr(args, "duid_hw_type=");
			p += strlen("duid_hw_type=");
			if(*p==DELIM) //default
				pEntry->duid_hw_type = 0;
			else
				pEntry->duid_hw_type = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1; 

			//duid_mac
			p = strstr(args, "duid_mac=");
			p += strlen("duid_mac=");
			if (*p==DELIM) {//default
				memset(pEntry->duid_mac, 0, MAC_ADDR_LEN);
			} else {
				for(idx=0; *p != DELIM&&idx<32; ++p) {
					if (*p!=':')
						buff[idx++] = *p;
				}
				rtk_string_to_hex(buff, pEntry->duid_mac, 12);
			}

			//duid_time
			p = strstr(args, "duid_time=");
			p += strlen("duid_time=");
			if(*p==DELIM) //default
				pEntry->duid_time = 0;
			else
				pEntry->duid_time = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1; 

			//duid_ent_num
			p = strstr(args, "duid_ent_num=");
			p += strlen("duid_ent_num=");
			if(*p==DELIM) //default
				pEntry->duid_ent_num = 0;
			else
				pEntry->duid_ent_num = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1; 

			//duid_ent_id
			p = strstr(args, "duid_ent_id=");
			p += strlen("duid_ent_id=");
			if (*p==DELIM) {//default
				memset(pEntry->duid_ent_id, 0, OPTION_LEN);
			} else {
				for(idx=0; *p != DELIM&&idx<32; ++p) {
					if (*p!=':')
						buff[idx++] = *p;
				}
				pEntry->duid_ent_id[sizeof(pEntry->duid_ent_id)-1]='\0';
				strncpy(pEntry->duid_ent_id,buff,sizeof(pEntry->duid_ent_id)-1);
			}


		}
		else if(pEntry->dhcpopt_type == IPQOS_DHCPOPT_125 )
		{
			//opt125_ent_num
			p = strstr(args, "opt125_ent_num=");
			p += strlen("opt125_ent_num=");
			if(*p==DELIM) //default
				pEntry->opt125_ent_num = 0;
			else
				pEntry->opt125_ent_num = strtol(p, &tmp, 0);
			if(*tmp != DELIM) return -1; 

			//opt125_manufacturer
			p = strstr(args, "opt125_manufacturer=");
			p += strlen("opt125_manufacturer=");
			if (*p==DELIM) {//default
				memset(pEntry->opt125_manufacturer, 0, OPTION_LEN);
			} else {
				for(idx=0; *p != DELIM&&idx<32; ++p) {
					if (*p!=':')
						buff[idx++] = *p;
				}
				pEntry->opt125_manufacturer[sizeof(pEntry->opt125_manufacturer)-1]='\0';
				strncpy(pEntry->opt125_manufacturer,buff,sizeof(pEntry->opt125_manufacturer)-1);
			}


			//opt125_product_class
			p = strstr(args, "opt125_product_class=");
			p += strlen("opt125_product_class=");
			if (*p==DELIM) {//default
				memset(pEntry->opt125_product_class, 0, OPTION_LEN);
			} else {
				for(idx=0; *p != DELIM&&idx<32; ++p) {
					if (*p!=':')
						buff[idx++] = *p;
				}
				pEntry->opt125_product_class[sizeof(pEntry->opt125_product_class)-1]='\0';
				strncpy(pEntry->opt125_product_class,buff,sizeof(pEntry->opt125_product_class)-1);
			}
	
			//opt125_model
			p = strstr(args, "opt125_model=");
			p += strlen("opt125_model=");
			if (*p==DELIM) {//default
				memset(pEntry->opt125_model, 0, OPTION_LEN);
			} else {
				for(idx=0; *p != DELIM&&idx<32; ++p) {
					if (*p!=':')
						buff[idx++] = *p;
				}
				pEntry->opt125_model[sizeof(pEntry->opt125_model)-1]='\0';
				strncpy(pEntry->opt125_model,buff,sizeof(pEntry->opt125_model)-1);
			}

			//opt125_serial
			p = strstr(args, "opt125_serial=");
			p += strlen("opt125_serial=");
			if (*p==DELIM) {//default
				memset(pEntry->opt125_serial, 0, OPTION_LEN);
			} else {
				for(idx=0; *p != DELIM&&idx<32; ++p) {
					if (*p!=':')
						buff[idx++] = *p;
				}
				pEntry->opt125_serial[sizeof(pEntry->opt125_serial)-1]='\0';
				strncpy(pEntry->opt125_serial,buff,sizeof(pEntry->opt125_serial)-1);
			}
		}
	}
#endif //CONFIG_BRIDGE_EBT_DHCP_OPT
	//vlan 802.1p match
	p = strstr(args, "vlan1p=");
	p += strlen("vlan1p=");
	if(*p=='\0') {//default
		pEntry->vlan1p =0;
	} else {
		pEntry->vlan1p = strtol(p, &tmp, 0);
		if(*tmp != '\0') return -1;
	}	
#ifdef CONFIG_00R0
	pEntry->InstanceNum = get_next_QoSclass_instnum();
#endif
	return ret;
}
//parse formated string recieved from web client:
//inf=VAL&proto=VAL&srcip=VAL&srcnetmask=VAL&dstip=VAL&dstnetmask=VAL&sport=VAL&dport=VAL&uprate=VAL(old).
//where VAL is the value of the corresponding item. if none, VAL is ignored but the item name cannot be
//The argument action is not modified by the function, so it can be a const string or non-const string.
static int parseArgs(char* action, MIB_CE_IP_TC_Tp pEntry)
{
	char* p = NULL, *tmp = NULL;
	int i = 0;
	char ipv6Enable=-1;

#ifdef CONFIG_IPV6
	int idx = 0;
	char buff2[48] = {0};
	struct in6_addr ip6Addr;
#endif

	PRINT_FUNCTION

	//ifIndex
	tmp = strstr(action, "inf=");
	tmp += strlen("inf=");
	pEntry->ifIndex = strtol(tmp, &p,0);	
	
	if(*p != DELIM){
		return 1;
	}

	//protocol
	tmp =strstr(action, "proto=");
	tmp += strlen("proto=");
	if(!tmp||*tmp == DELIM)//not set protocol, set it to default,none
		pEntry->protoType = PROTO_NONE;
	else
	{
		pEntry->protoType = strtol(tmp, &p, 0);
		if(*p != DELIM){
			return 1;
		}
	}

#ifdef CONFIG_IPV6
	//IPVersion
	
	mib_get_s(MIB_V6_IPV6_ENABLE, (void *)&ipv6Enable, sizeof(ipv6Enable));
	if(ipv6Enable == 1){
		tmp = strstr(action, "IPversion=");
		tmp += strlen("IPversion=");
		if(*tmp == DELIM){//default
			pEntry->IpProtocol = IPVER_IPV4_IPV6;
		}
		else {
			pEntry->IpProtocol = strtol(tmp, &p, 0);
			if(*p != DELIM) return -1;//invalid dscp value
		}
	}
	else
		pEntry->IpProtocol = IPVER_IPV4;

	// If this is a IPv4 rule
	if ( pEntry->IpProtocol == IPVER_IPV4) {
#endif
		//source ip
		tmp = strstr(action, "srcip=");
		tmp += strlen("srcip=");
		if(!tmp||*tmp == DELIM)//noet set, set default
			memset(pEntry->srcip, 0, IP_ADDR_LEN);
		else
		{
			char sip[16]={0};
			p = strchr(tmp, DELIM);
			if(p&&p-tmp>15){
				return 1;
			}
			strncpy(sip, tmp, p-tmp);
			inet_aton(sip, (struct in_addr *)&pEntry->srcip);
		}

		//source ip address netmask
		tmp = strstr(action, "srcnetmask=");
		tmp += strlen("srcnetmask=");
		if(!tmp||*tmp==DELIM)
			pEntry->smaskbits = 0;
		else
		{
			char smask[16]={0};
			p = strchr(tmp, DELIM);
			if(p&&p-tmp>15) return 1;
			strncpy(smask, tmp, p-tmp);
			pEntry->smaskbits = getNetMaskBit(smask);
		}

		//destination ip
		tmp = strstr(action, "dstip=");
		tmp += strlen("dstip=");
		if(!tmp||*tmp == DELIM)//noet set, set default
			memset(pEntry->dstip, 0, IP_ADDR_LEN);
		else
		{
			char dip[16]={0};
			p = strchr(tmp, DELIM);
			if(p&&p-tmp>15){
				return 1;
			}
			strncpy(dip, tmp, p-tmp);
			inet_aton(dip, (struct in_addr *)&pEntry->dstip);
		}

		//destination ip address netmask
		tmp = strstr(action, "dstnetmask=");
		tmp += strlen("dstnetmask=");
		if(!tmp||*tmp==DELIM)
			pEntry->dmaskbits = 0;
		else
		{
			char dmask[16]={0};
			p = strchr(tmp, DELIM);
			if(p&&p-tmp>15)
				return 1;
			strncpy(dmask, tmp, p-tmp);
			pEntry->dmaskbits = getNetMaskBit(dmask);

		}
#ifdef CONFIG_IPV6
	}
#endif

#ifdef CONFIG_IPV6
	// If it is a IPv6 rule.
	if ( pEntry->IpProtocol == IPVER_IPV6 )
	{
		//ip6 source address
		tmp = strstr(action, "sip6=");
		tmp += strlen("sip6=");
		if(*tmp==DELIM) {//default
			memset(pEntry->sip6, 0, IP6_ADDR_LEN);
		} else {
			for(idx=0; *tmp != DELIM&&idx<sizeof(buff2)-1; ++tmp)
				buff2[idx++] = *tmp;
			buff2[idx] = '\0';
			{//ip address
				inet_pton(PF_INET6, buff2, &ip6Addr);
				memcpy(pEntry->sip6, &ip6Addr, sizeof(pEntry->sip6));
			}
		}

		// ip6 src IP prefix Len
		tmp = strstr(action, "sip6PrefixLen=");
		tmp += strlen("sip6PrefixLen=");
		if(*tmp == DELIM)//default
			pEntry->sip6PrefixLen = 0;
		else {
			pEntry->sip6PrefixLen = strtol(tmp, &p, 0);
			if(*p != DELIM) return -1;//invalid dscp value
		}

		//ip6 dest address
		tmp = strstr(action, "dip6=");
		tmp += strlen("dip6=");
		if(*tmp==DELIM) {//default
			memset(pEntry->dip6, 0, IP6_ADDR_LEN);
		} else {
			for(idx=0; *tmp != DELIM&&idx<sizeof(buff2)-1; ++tmp)
				buff2[idx++] = *tmp;
			buff2[idx] = '\0';
			{//ip address
				inet_pton(PF_INET6, buff2, &ip6Addr);
				memcpy(pEntry->dip6, &ip6Addr, sizeof(pEntry->dip6));
			}
		}

		// ip6 dst IP prefix Len
		tmp = strstr(action, "dip6PrefixLen=");
		tmp += strlen("dip6PrefixLen=");
		if(*tmp == DELIM)//default
			pEntry->dip6PrefixLen = 0;
		else {
			pEntry->dip6PrefixLen = strtol(tmp, &p, 0);
			if(*p != DELIM) return -1;//invalid dscp value
		}
	}
#endif

	//source port
	tmp = strstr(action, "sport=");
	tmp += strlen("sport=");
	if(!tmp||*tmp==DELIM)
		pEntry->sport = 0;
	else
	{
		pEntry->sport = strtol(tmp, &p, 0);
		if(*p != DELIM)
			return 1;
	}

	//destination port
	tmp = strstr(action, "dport=");
	tmp += strlen("dport=");
	if(!tmp||*tmp==DELIM)
		pEntry->dport = 0;
	else
	{
		pEntry->dport = strtol(tmp, &p, 0);
		if(*p != DELIM)
			return 1;
	}

#ifdef QOS_TRAFFIC_SHAPING_BY_VLANID
	//vlanID
	tmp = strstr(action, "vlanID=");
	tmp += strlen("vlanID=");
	if(!tmp||*tmp==DELIM)
		pEntry->vlanID = 0;
	else
	{
		pEntry->vlanID = strtol(tmp, &p, 0);
		if(*p != DELIM)
			return 1;
	}
#endif

#ifdef QOS_TRAFFIC_SHAPING_BY_SSID
	tmp = strstr(action, "ssid=");
	tmp += strlen("ssid=");
	printf("[%s:%d]\n",__func__,__LINE__);
	printf("tmp=%s\n",tmp);
	if(!tmp||*tmp==DELIM)
		pEntry->ssid[0]=NULL;
	else
	{
		p = strchr(tmp, DELIM);
		strncpy(pEntry->ssid, tmp, p-tmp);
		if(*p != DELIM)
			return 1;
	}
#endif

	//rate limit
	tmp = strstr(action, "rate=");
	tmp += strlen("rate=");
	if(!tmp||*tmp=='\0')
		pEntry->limitSpeed = 0;
	else
	{
		pEntry->limitSpeed = strtol(tmp, &p, 0);
		if(*p != DELIM)
			return 1;
	}

	//direction limit
	tmp = strstr(action, "direction=");
	tmp += strlen("direction=");
	if(!tmp||*tmp=='\0')
		pEntry->direction = 0;
	else
	{
		pEntry->direction = strtol(tmp, &p, 0);
		if(*p != '\0')
			return 1;
	}
	return 0;
}

void formQosRuleEdit(request * wp, char* path, char* query)
{
	MIB_CE_IP_QOS_T entry;
	char* action = NULL, args[256]={0}, *p = NULL, *tmp=NULL;
	char* act1="addrule", *act2="editrule", *url = NULL;
	char action2[2048];
	int entryNum = 0, index=0,i=0;
	int j;
	int log_wan_port = 0;
	
	PRINT_FUNCTION
		
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(!mib_get_s(MIB_QOS_LOG_WAN_PORT, &log_wan_port, sizeof(log_wan_port))){
		printf("%s:%d Fail to get wan port\n",__FUNCTION__,__LINE__);
		return;
	}
	if(log_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return;
	}
#endif

	action = boaGetVar(wp, "lst", "");
	if(action[0])
	{
		rtk_util_data_base64decode(action, &action2[0], sizeof(action2));
		//AUG_PRT("@@@@ action2=%s\n", action2);
		entryNum = mib_chain_total(MIB_IP_QOS_TBL);

		if(!strncmp(action2, act1, strlen(act1)))
		{//add new one
			if( entryNum>=QUEUE_RULE_NUM_MAX)
			{
				ERR_MSG(multilang(LANG_YOU_CANNOT_ADD_ONE_NEW_RULE_WHEN_QUEUE_IS_FULL));
				return;
			}

			//reset to zero
			bzero(&entry, sizeof(MIB_CE_IP_QOS_T));

            index = parseRuleArgs(action2, &entry);
			//printf("%s %d The index is %d\n",__FUNCTION__,__LINE__,index);
			if(entry.QoSOrder==0)
				entry.QoSOrder = (entryNum+1);
			else if(entry.QoSOrder > (entryNum+1))
	  			entry.QoSOrder = (entryNum+1);
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
			entry.logic_wan_port = log_wan_port;
#endif
			if (index >= 0)
			{
				PRINT_QUEUE_RULE((&entry));

				if(!mib_chain_add(MIB_IP_QOS_TBL, &entry))
				{
					ERR_MSG(Tadd_chain_error);
					return;
				}

				if(!UpdateQoSOrder(index, 0,0))
				{	
					ERR_MSG("Update MIB Chain record error");
					return;
				}
			}
			else
			{
				ERR_MSG(multilang(LANG_ADDQOSRULEWRONG_ARGUMENT_FORMAT));
				return;
			}

		} else if(!strncmp(action2, act2, strlen(act2))) {//update old one
			MIB_CE_IP_QOS_T oldEntry;

			//reset to zero
			bzero(&entry, sizeof(MIB_CE_IP_QOS_T));

			index = parseRuleArgs(action2, &entry);
			if(index<0)
			{
				ERR_MSG(multilang(LANG_UPDATEQOSRULE_WRONG_ARGUMENT_FORMAT));
				return;
			}

			PRINT_QUEUE_RULE((&entry));

			if(!mib_chain_get(MIB_IP_QOS_TBL, index, &oldEntry)){
				ERR_MSG(Tget_mib_error);
				return;
			}
			if(entry.QoSOrder==0)
				entry.QoSOrder = oldEntry.QoSOrder;
			else if(entry.QoSOrder > entryNum)
	  			entry.QoSOrder = entryNum;
#ifdef _CWMP_MIB_
			entry.InstanceNum=oldEntry.InstanceNum;
#endif
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
			entry.logic_wan_port = oldEntry.logic_wan_port;
#endif

			if(!mib_chain_update(MIB_IP_QOS_TBL, &entry, index))
			{
				ERR_MSG(Tupdate_chain_error);
				return;
			}

			if(!UpdateQoSOrder(index, 1, oldEntry.QoSOrder))
			{	
				ERR_MSG("Updating qos rule into mib is wrong!");
				return;
			}
			
		} else {//undefined operation
			ERR_MSG(multilang(LANG_WRONG_OPERATION_HAPPENED_YOU_ONLY_AND_ADD_OR_EDIT_QOS_RULE_IN_THIS_PAGE));
			return;
		}
	}
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
	//redirect
	url = boaGetVar(wp, "submit-url","");
	if(url[0])
	{
		boaRedirect(wp, url);
	}

	return;
}

////////////////////////////////////////////////////////////////////////////////////////////
//Qos Traffic control code

//now the problem is that the ifIndex is used in c code, and ifname is used for displaying,
//how can i get the ifname or ifIndex
int initTraffictlPage(int eid, request * wp, int argc, char **argv)
{
	MIB_CE_IP_TC_T entry;
	int entryNum = 0, i=0, nBytes = 0;
	char sip[20], dip[20], wanname[IFNAMSIZ], *p = NULL;
	unsigned int total_bandwidth = 0;
	unsigned char totalBandWidthEn = 0;
#ifdef CONFIG_IPV6
	unsigned char 	sip6Str[55], dip6Str[55];
#endif

	PRINT_FUNCTION

	entryNum = mib_chain_total(MIB_IP_QOS_TC_TBL);

	if(mib_get_s(MIB_TOTAL_BANDWIDTH, &total_bandwidth, sizeof(total_bandwidth)))
	{
		nBytes += boaWrite(wp, "totalBandwidth=%u;\n", total_bandwidth);
	}
	else
	{
		nBytes += boaWrite(wp, "totalBandwidth=1024;\n");
	}

	if (mib_get_s(MIB_TOTAL_BANDWIDTH_LIMIT_EN, &totalBandWidthEn, sizeof(totalBandWidthEn)))
		nBytes += boaWrite(wp, "totalBandWidthEn=%d;\n", totalBandWidthEn);
	else
		nBytes += boaWrite(wp, "totalBandWidthEn=0;\n");
	
	nBytes = boaWrite(wp, "displayTotalUSBandwidth=%d;\n", rtk_qos_total_us_bandwidth_support());

	for(;i<entryNum; i++)
	{
		MIB_CE_ATM_VC_T       vcEntry;

		if(!mib_chain_get(MIB_IP_QOS_TC_TBL, i, &entry))
			continue;
		wanname[0]='\0';
		//if (!getWanEntrybyindex(&vcEntry, entry.ifIndex)) {
		//	getWanName(&vcEntry, wanname);
		//}
		ifGetName(entry.ifIndex, wanname, sizeof(wanname));
		strncpy(sip, inet_ntoa(*((struct in_addr*)&entry.srcip)), INET_ADDRSTRLEN);
		strncpy(dip, inet_ntoa(*((struct in_addr*)&entry.dstip)), INET_ADDRSTRLEN);
		if(entry.smaskbits)
		{
			p = sip + strlen(sip);
			snprintf(p,sizeof(sip)-strlen(sip), "/%d", entry.smaskbits );
		}

		if(entry.dmaskbits)
		{
			p = dip + strlen(dip);
			snprintf(p,sizeof(dip)-strlen(dip), "/%d", entry.dmaskbits );
		}

#ifdef CONFIG_IPV6
		inet_ntop(PF_INET6, (struct in6_addr *)entry.sip6, sip6Str, sizeof(sip6Str));
		if(entry.sip6PrefixLen)
		{
			p = sip6Str + strlen(sip6Str);
			snprintf(p,sizeof(sip6Str)-strlen(sip6Str), "/%d", entry.sip6PrefixLen );
		}

		inet_ntop(PF_INET6, (struct in6_addr *)entry.dip6, dip6Str, sizeof(dip6Str));
		if(entry.dip6PrefixLen)
		{
			p = dip6Str + strlen(dip6Str);
			snprintf(p,sizeof(dip6Str)-strlen(dip6Str), "/%d", entry.dip6PrefixLen );
		}
#endif

		nBytes += boaWrite(wp, "traffictlRules.push(new it_nr(\"\",\n"
#ifdef CONFIG_IPV6
			//"new it(\"ipversion\",%d),\n"  //ipv4 or ipv6
			"new it(\"IpProtocolType\",%d),\n"  //ipv4 or ipv6
			"new it(\"sip6\",  \"%s\"),\n" //source ip6
			"new it(\"dip6\",  \"%s\"),\n" //dst ip6
#endif
			"new it(\"id\",         %d),\n"
			"new it(\"inf\",    \"%s\"),\n"
			"new it(\"proto\",      %d),\n"
			"new it(\"sport\",      %d),\n"
			"new it(\"dport\",      %d),\n"
			"new it(\"srcip\",  \"%s\"),\n"
			"new it(\"dstip\",  \"%s\"),\n"
			"new it(\"rate\",  \"%d\"),\n"
#ifdef QOS_TRAFFIC_SHAPING_BY_VLANID
			"new it(\"vlanID\",  \"%d\"),\n"
#endif
#ifdef QOS_TRAFFIC_SHAPING_BY_SSID
			"new it(\"ssid\",  \"%s\"),\n"
#endif
			"new it(\"direction\",   %d)));\n",
#ifdef CONFIG_IPV6
			entry.IpProtocol, sip6Str, dip6Str,
#endif
			entry.entryid, wanname, entry.protoType, entry.sport,
			entry.dport, sip, dip, entry.limitSpeed, 

#ifdef QOS_TRAFFIC_SHAPING_BY_VLANID
			entry.vlanID, 
#endif
#ifdef QOS_TRAFFIC_SHAPING_BY_SSID
			entry.ssid, 
#endif
			entry.direction);
	}

	return nBytes;
}

void formQosTraffictl(request * wp, char *path, char *query)
{
	char *action = NULL, *url = NULL;
	char *act1="applybandwidth", *act2 = "applysetting";
	char *act4="cancelbandwidth";
	int entryNum = 0;

	PRINT_FUNCTION

	action = boaGetVar(wp, "lst", "");

	entryNum = mib_chain_total(MIB_IP_QOS_TC_TBL);

	if(action[0])
	{
		if( !strncmp(action, act1, strlen(act1)) )
		{//set total bandwidth
			unsigned int totalbandwidth = 0;
			unsigned char totalbandwidthEn=0;
			char* strbandwidth = NULL;
			strbandwidth = strstr(action, "bandwidth=");
			strbandwidth += strlen("bandwidth=");
			if(*(char *)strbandwidth)//found it
			{
				totalbandwidth = strtoul(strbandwidth, NULL, 0);

				totalbandwidthEn = 1;
				if (!mib_set(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&totalbandwidthEn)) {
					ERR_MSG(Tset_mib_error);
					return;
				}

				if(!mib_set(MIB_TOTAL_BANDWIDTH, (void*)&totalbandwidth))
				{
					ERR_MSG(Tset_mib_error);
					return;
				}

				//take effect
				take_qos_effect();
			}
			else {
				totalbandwidthEn = 0;
				if (!mib_set(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&totalbandwidthEn)) {
					ERR_MSG(Tset_mib_error);
					return;
				}
			}
		}
		else if ( !strncmp(action, act4, strlen(act4)) )
		{//cancel total bandwidth restrict
			unsigned char totalbandwidthEn=0;
			if (!mib_set(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&totalbandwidthEn)) {
				ERR_MSG(Tset_mib_error);
				return;
			}

			//take effect
			take_qos_effect();
		}
		else if( !strncmp(action, act2, strlen(act2)) )
		{//delete some
			int idlst[TRAFFICTL_RULE_NUM_MAX+1] = {0};
			int  i=0, j=0, index=1;
			char stridlst[256],err_msg[256], *p = NULL;
			MIB_CE_IP_TC_T entry;

			p = strstr(action, "id=");
			p += strlen("id=");
			if(*p == '\0') {//delete none
				goto done;
			}

			stridlst[0] = '\0';
			stridlst[sizeof(stridlst)-1]='\0';
			strncpy(stridlst, p, sizeof(stridlst)-1);

			//convert the id list, store them in idlst,
			//you can delete most 10 rules at one time
			p = strtok(stridlst, SUBDELIM1);
			if(p) index = atoi(p);
			if(index>0&&index<=TRAFFICTL_RULE_NUM_MAX) idlst[index]=1;
			while((p = strtok(NULL, SUBDELIM1)) != NULL)
			{
				index = atoi(p);
				idlst[index]=1;
				if(index > TRAFFICTL_RULE_NUM_MAX )
					break;
			}

			for(i=entryNum-1; i>=0; i--)
			{
				if(!mib_chain_get(MIB_IP_QOS_TC_TBL, i, &entry))
					continue;

				if( 1 == idlst[entry.entryid]) //delete it
				{
					//delete rules of  tc and iptables
					if(1 != mib_chain_delete(MIB_IP_QOS_TC_TBL, i)) {
						snprintf(err_msg, 256, multilang(LANG_ERROR_HAPPENED_WHEN_DELETING_RULE_D), entry.entryid);
						ERR_MSG(err_msg);
						return;
					}
				}
			}

done:
			take_qos_effect();
		}
	}
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	//well, go back
	url = boaGetVar(wp, "submit-url", "");
	if(url[0])
		boaRedirect(wp, url);
	return;
}

//add traffic controlling rules's main function
void formQosTraffictlEdit(request * wp, char *path, char *query)
{
	MIB_CE_IP_TC_T entry;
	char* action=NULL, *url = NULL;
	char action2[2048];
	int entryNum = 0, entryid = 1;
	unsigned char map[TRAFFICTL_RULE_NUM_MAX+1]={0};

	PRINT_FUNCTION

	entryNum = mib_chain_total(MIB_IP_QOS_TC_TBL);

	//You are allowed to have TRAFFICTL_RULE_NUM_MAX rules
	if(entryNum>=TRAFFICTL_RULE_NUM_MAX)
	{
		ERR_MSG(multilang(LANG_TRAFFIC_CONTROLLING_QUEUE_IS_FULL_YOU_MUST_DELETE_SOME_ONE));
		return;
	}

	action = boaGetVar(wp, "lst", "");

	if(action[0])
	{
		rtk_util_data_base64decode(action, &action2[0], sizeof(action2));
		//AUG_PRT("#### action2=%s\n", action2);
		//allocate a free rule id for new entry
		{
			int i = 0;
			for(;i<entryNum;i++)
			{
				if(!mib_chain_get(MIB_IP_QOS_TC_TBL, i, &entry))
					continue;

				map[entry.entryid] = 1;
			}
			for(i=1;i<=TRAFFICTL_RULE_NUM_MAX;i++)
			{
				if(!map[i])
				{
					entryid = i;
					break;
				}
				else if(i==TRAFFICTL_RULE_NUM_MAX)
				{
					ERR_MSG(multilang(LANG_TRAFFIC_CONTROLLING_QUEUE_IS_FULL_YOU_MUST_DELETE_SOME_ONE));
					return;
				}
			}
		}

		memset(&entry, 0, sizeof(MIB_CE_IP_TC_T));

		entry.entryid = entryid;

		if(parseArgs(action2, &entry))
		{//some arguments are wrong
			ERR_MSG(multilang(LANG_WRONG_SETTING_IS_FOUND));
			return;
		}

		PRINT_TRAFFICTL_RULE((&entry));

		#if 0
		/*ql:20080814 START: patch for TnW - while one acl contain other acl, we should change the rule
		* order to enable CAR function.
		*/
		if (entryNum!=0 && !isTraffictlRuleWithPort(&entry)) {
			if (!mib_chain_insert(MIB_IP_QOS_TC_TBL, 0, &entry))
			{
				ERR_MSG("Cannot insert setting into mib!");
				return;
			}
		} else
		#endif
		/*ql:20080814 END*/
		if(!mib_chain_add(MIB_IP_QOS_TC_TBL, &entry))
		{//adding mib setting is wrong
			ERR_MSG(Tadd_chain_error);
			return;
		}
	}
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
	//redirect
	url = boaGetVar(wp, "submit-url", "");
	if(url[0])
	boaRedirect(wp, url);
	return;
}

#ifdef CONFIG_00R0
void formTrafficShaping(request *wp, char *path, char *query)
{
	char *url = NULL, *p = NULL;
	int num = 0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;

	for (num = 0; num < 4; num++) {
		char shapingstr[] = "shaping0";
		if (!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, num, (void*)&qEntry))
			continue;

		shapingstr[7] += num;
		p = boaGetVar(wp, shapingstr, "");

		if (p && p[0]) {
			qEntry.shaping_rate = atoi(p);
		}
		mib_chain_update(MIB_IP_QOS_QUEUE_TBL, (void *)&qEntry, num);
	}

	//take effect
	take_qos_effect();

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	//redirect to show page
	url = boaGetVar(wp, "submit-url", "");
	if (url[0]) {
		boaRedirect(wp, url);
	}
	return;
}
#endif

/******************************************************************
 * NAME:     formQosRule
 * DESC:     main function deal with qos rules, delete and change
 * ARGS:
 * RETURN:
 ******************************************************************/
void formQosRule(request * wp, char* path, char* query)
{
	MIB_CE_IP_QOS_T entry;
	int entryNum=0, i=0, id = 0;
	unsigned char statuslist[QUEUE_RULE_NUM_MAX+1] = {0}, dellist[QUEUE_RULE_NUM_MAX+1]= {0};
	char* action = NULL, *p=NULL, *url = NULL;
	int log_wan_port = 0;
	
	PRINT_FUNCTION
		
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(!mib_get_s(MIB_QOS_LOG_WAN_PORT, &log_wan_port, sizeof(log_wan_port))){
		printf("%s:%d Fail to get wan port\n",__FUNCTION__,__LINE__);
		return;
	}
	if(log_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return;
	}
#endif
	//abstract args from string
	action = boaGetVar(wp, "lst", "");
	entryNum = mib_chain_total(MIB_IP_QOS_TBL);

	//printf("action=%s\n", action);
	p = strtok(action, ",&");
	if(p) id = atoi(p);
	if(id<0||id>QUEUE_RULE_NUM_MAX)
		goto END;
	p = strtok(NULL, ",&");
	if(p) statuslist[id] = !!atoi(p);
	p = strtok(NULL, ",&");
	if(p) dellist[id] = !!atoi(p);

	while( (p = strtok(NULL, ",&")) != NULL )
	{
		id = atoi(p);
		if(id<0||id>QUEUE_RULE_NUM_MAX) continue;
		p = strtok(NULL, ",&");
		if(p) statuslist[id] = !!atoi(p);
		p = strtok(NULL, ",&");
		if(p) dellist[id] = !!atoi(p);
	}

	//change status of rules if neccessary
	//printf("check statuslist\n");
	for(i=0;i<entryNum;i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_TBL, i, &entry))
			continue;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(entry.logic_wan_port != log_wan_port)
			continue;
#endif
		if(statuslist[i] != entry.enable)
		{
			//printf("entry %d status changed\n", i);
			entry.enable = statuslist[i];
			if(!mib_chain_update(MIB_IP_QOS_TBL, &entry, i))
			{
				ERR_MSG(Tupdate_chain_error);
				return;
			}
		}
	}

	//printf("check deletelist!\n");
	//delete some one
	for(i=entryNum-1;i>=0;i--)
	{
		if(!mib_chain_get(MIB_IP_QOS_TBL, i, &entry))
			continue;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(entry.logic_wan_port != log_wan_port)
			continue;
#endif
		if(i<=QUEUE_RULE_NUM_MAX&&dellist[i])
		{
			//printf("entry %d deleted\n", i);
			if(!UpdateQoSOrder(i,2,(entry.QoSOrder))){
				ERR_MSG("Delete rule error!");
				return;
			}
			
			if(1 != mib_chain_delete(MIB_IP_QOS_TBL, i))
			{
				ERR_MSG(Tdelete_chain_error);
				return;
			}
		}
	}

	//take effect
	take_qos_effect();

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

END:
	//redirect to show page
	url = boaGetVar(wp, "submit-url", "");
	if(url[0])
	{
		boaRedirect(wp, url);
	}
	return;
}

int initQueuePolicy(int eid, request * wp, int argc, char **argv)
{
	char wanname[32];
	int nBytes=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int i;
	unsigned int qEntryNum = 0;
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	unsigned char policy = 0;
	unsigned int total_bandwidth = 0; 
#else
	unsigned char policy[QOS_MAX_PORT_NUM] = {0};
	unsigned int total_bandwidth[QOS_MAX_PORT_NUM] = {0};
	int log_wan_port = 0;
#endif	
	PRINT_FUNCTION
		
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(getOpMode()!=BRIDGE_MODE){//gateway mode
		if(!mib_get_s(MIB_QOS_LOG_WAN_PORT, &log_wan_port, sizeof(log_wan_port))){
			printf("%s:%d Fail to get wan port\n",__FUNCTION__,__LINE__);
			return 0;
		}
		if(log_wan_port < 0){
			printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
			return 0;
		}
		for(i=0;i<SW_LAN_PORT_NUM;i++){
			if(rtk_port_is_wan_logic_port(i))
				nBytes += boaWrite(wp,"wanportlst.add(new it(\"%d\",\"Port%d\"));\n",i,i);
		}
	}
	else{//bridge mode
#ifdef CONFIG_USER_AP_BRIDGE_MODE_QOS
		if((log_wan_port=rtk_port_get_logPort(rtk_get_bridge_mode_uplink_port())) == -1)
#endif
		{
			log_wan_port = getDefaultWanPort();
		}
		nBytes += boaWrite(wp,"wanportlst.add(new it(\"%d\",\"BrPort%d\"));\n",log_wan_port,log_wan_port);
	}
	
	nBytes += boaWrite(wp,"var WanPort = %d;\n",log_wan_port);
#endif
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(!mib_get_s(MIB_TOTAL_BANDWIDTH_LIMIT_EN, &policy, sizeof(policy))|| (policy !=0&&policy!=1))
		policy = 0;
	nBytes = boaWrite(wp, "userDefinedBandwidth=%d;\n", policy);
#else
	if(!mib_get_s(MIB_TOTAL_BANDWIDTH_LIMIT_EN, &policy, sizeof(policy))|| (policy[log_wan_port] !=0&&policy[log_wan_port]!=1))
		policy[log_wan_port] = 0;
	nBytes = boaWrite(wp, "userDefinedBandwidth=%d;\n", policy[log_wan_port]);
#endif
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(mib_get_s(MIB_TOTAL_BANDWIDTH, &total_bandwidth, sizeof(total_bandwidth)))
	{
		nBytes += boaWrite(wp, "totalBandwidth=%u;\n", total_bandwidth);
	}
#else
	if(mib_get_s(MIB_MULTI_TOTAL_BANDWIDTH, &total_bandwidth, sizeof(total_bandwidth))){
		nBytes += boaWrite(wp, "totalBandwidth=%u;\n", total_bandwidth[log_wan_port]);
	}
#endif
	else
	{
		nBytes += boaWrite(wp, "totalBandwidth=1024;\n");
	}
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	if(!mib_get_s(MIB_QOS_ENABLE_QOS, &policy, sizeof(policy))|| (policy !=0&&policy!=1))
		policy = 0;
	nBytes = boaWrite(wp, "qosEnable=%d;\n", policy);

	if(!mib_get_s(MIB_QOS_POLICY, &policy, sizeof(policy))|| (policy !=0&&policy!=1))
		policy = 0;
	nBytes += boaWrite(wp, "policy=%d;\n", policy);
#else
	if(!mib_get_s(MIB_QOS_ENABLE_QOS, &policy, sizeof(policy))|| (policy[log_wan_port] !=0&&policy[log_wan_port]!=1))
			policy[log_wan_port] = 0;
		nBytes = boaWrite(wp, "qosEnable=%d;\n", policy[log_wan_port]);

	if(!mib_get_s(MIB_QOS_POLICY, &policy, sizeof(policy))|| (policy[log_wan_port] !=0&&policy[log_wan_port]!=1))
		policy[log_wan_port] = 0;
	nBytes += boaWrite(wp, "policy=%d;\n", policy[log_wan_port]);
#endif
	nBytes = boaWrite(wp, "displayTotalUSBandwidth=%d;\n", rtk_qos_total_us_bandwidth_support());

	mib_get_s(MIB_IP_QOS_QUEUE_NUM, &qEntryNum, sizeof(qEntryNum));
	if(qEntryNum <=0)
		return nBytes;
	for(i=0;i<qEntryNum; i++)
	{
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i, (void*)&qEntry))
#else
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i+log_wan_port*MAX_QOS_QUEUE_NUM, (void*)&qEntry))
#endif
			continue;
		nBytes += boaWrite(wp, "queues.push("
			"new it_nr(\"%d\","  //qos queue name
			"new it(\"qname\",\"Q%d\"),"  // name
			"new it(\"prio\",%d),"  // priority
			"new it(\"weight\", %d),"  // weight
#ifdef CONFIG_00R0
			"new it(\"shaping\", %d),"  // Shaping Rate
#endif
			"new it(\"enable\", %d)));\n",
			i, i+1, i+1, qEntry.weight, 
#ifdef CONFIG_00R0
			qEntry.shaping_rate,
#endif
			qEntry.enable);
	}
	return nBytes;
}

int initEthTypes(int eid, request * wp, int argc, char **argv)
{
	int nBytes = 0;
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	unsigned short *ethtypes;
	int i, num;
	num = getEthTypeNumb();
	int log_wan_port = 0;
	if(!mib_get_s(MIB_QOS_LOG_WAN_PORT, &log_wan_port, sizeof(log_wan_port))){
		printf("%s:%d Fail to get wan port\n",__FUNCTION__,__LINE__);
		return 0;
	}
	if(log_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}
	nBytes += boaWrite(wp,"var WanPort = %d;\n",log_wan_port);
	
	ethtypes = (unsigned short *)malloc(sizeof(unsigned short)*num);
	if(!ethtypes)
		return;
	getEthTypes(ethtypes,num);
	nBytes += boaWrite(wp,"var ethTypes = new Array(");
	for(i=0;i<num;i++){
		nBytes += boaWrite(wp,"\"%04x\",",ethtypes[i]);
	}
	nBytes += boaWrite(wp,");\n");
	free(ethtypes);
	ethtypes = NULL;
#else
	nBytes += boaWrite(wp,"var ethTypes = new Array();\n");
#endif
	return nBytes;
} 

int initProtocol(int eid, request * wp, int argc, char **argv)
{
	int nBytes = 0;
#ifdef QOS_SUPPORT_RTP
	nBytes += boaWrite(wp,"var protos = new Array(\"\", \"TCP\", \"UDP\", \"ICMP\", \"\", \"RTP\");\n");
#else
	nBytes += boaWrite(wp,"var protos = new Array(\"\", \"TCP\", \"UDP\", \"ICMP\");\n");
#endif
	return nBytes;
}

void formQosPolicy(request * wp, char *path, char *query)
{
	char *action=NULL,*url=NULL, *p=NULL, *tmp=NULL;
	unsigned char modeflag = 0, old_modeflag = 0;
	int num = 0, ret;
	unsigned  int queuenum = 0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	unsigned char policy = 0;
#else
	unsigned char policy[QOS_MAX_PORT_NUM] = {0};
	int log_wan_port=0;
#endif
	PRINT_FUNCTION
	
#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
	action = boaGetVar(wp, "wanIsChanged", "");
	if(action[0]){
		action = boaGetVar(wp, "phywanport", "");
		if (action[0]) {
			log_wan_port = atoi(action);
			if(!mib_set(MIB_QOS_LOG_WAN_PORT, (void *)&log_wan_port)){
				ERR_MSG("Fail to set phy wan port!\n");
				return;
			}else
				goto commit;
		}
	}
	if(!mib_get_s(MIB_QOS_LOG_WAN_PORT, (void *)&log_wan_port,sizeof(log_wan_port))){
		ERR_MSG("Fail to get phy wan port!\n");
		return;
	}
	if(log_wan_port < 0){
		printf("%s:%d The phy-wan-port is invalid!\n",__FUNCTION__,__LINE__);
		return;
	}
	action = boaGetVar(wp, "phywanport", "");
	if (action[0]) {
		log_wan_port = atoi(action);
		if(!mib_set(MIB_QOS_LOG_WAN_PORT, (void *)&log_wan_port)){
			ERR_MSG("Fail to set phy wan port!\n");
			return;
		}
	}
#endif

	action = boaGetVar(wp, "qosen", "");
	if (action[0]) {
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		policy = (action[0]=='0') ? 0 : 1;
#else
		if(mib_get_s(MIB_QOS_ENABLE_QOS, (void *)&policy,sizeof(policy)))
			policy[log_wan_port] = (action[0]=='0') ? 0 : 1;
#endif
		mib_set(MIB_QOS_ENABLE_QOS, &policy);
	}

	action = boaGetVar(wp, "lst", "");
	//printf("policy: %s\n", action);
	if(action[0])
	{
		//policy
		p = strstr(action, "policy=");
		if(p)
		{
			p+=strlen("policy=");
			num = strtol(p, &tmp, 0);
			//if(tmp && tmp !=p && *tmp == DELIM)
			{
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
				policy = !!num;
				// to reduce log in console, stopIPQ will be executed in take_qos_effect
				stopIPQ();
#else
				if(mib_get_s(MIB_QOS_POLICY, (void *)&policy,sizeof(policy)))
					policy[log_wan_port] = !!num;
#endif
				if(!mib_set(MIB_QOS_POLICY, &policy)) {
					ERR_MSG(Tset_mib_error);
					return;
				}
			}
		}
	}

	mib_get_s(MIB_IP_QOS_QUEUE_NUM, &queuenum, sizeof(queuenum));
	for (num=0; num<queuenum; num++) {
		char wstr[]="w0";
		char qenstr[]="qen0";
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN	
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, num, (void*)&qEntry))
			continue;
		if (policy == 1)
#else
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, num+log_wan_port*MAX_QOS_QUEUE_NUM, (void*)&qEntry))
			continue;
		if (policy[log_wan_port] == 1)
#endif
		{ // WRR
			wstr[1] += num;
			p = boaGetVar(wp, wstr, "");
			if (p && p[0])
				qEntry.weight = atoi(p);
		}
		qenstr[3] += num;
		p = boaGetVar(wp, qenstr, "");
		if (p && p[0])
			qEntry.enable = 1;
		else
			qEntry.enable = 0;
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		mib_chain_update(MIB_IP_QOS_QUEUE_TBL, (void *)&qEntry, num);
#else
		qEntry.q_logic_wan_port = log_wan_port;
		mib_chain_update(MIB_IP_QOS_QUEUE_TBL, (void *)&qEntry, num+log_wan_port*MAX_QOS_QUEUE_NUM);
#endif
	}

#ifdef CONFIG_00R0
	for (num = 0; num < 4; num++) {
		char shapingstr[] = "shaping0";
		if (!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, num, (void*)&qEntry))
			continue;

		shapingstr[7] += num;
		p = boaGetVar(wp, shapingstr, "");

		if (p && p[0]) {
			qEntry.shaping_rate = atoi(p);
		}
		mib_chain_update(MIB_IP_QOS_QUEUE_TBL, (void *)&qEntry, num);
	}
#endif

	action = boaGetVar(wp, "totalbandwidth", "");
	if (action[0]) {
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		unsigned int totalbandwidth = 0;
		totalbandwidth = strtoul(action, NULL, 0);
		if(!mib_set(MIB_TOTAL_BANDWIDTH, (void*)&totalbandwidth))
#else
		unsigned int totalbandwidth[QOS_MAX_PORT_NUM] = {0};
		if(!mib_get_s(MIB_MULTI_TOTAL_BANDWIDTH, (void *)&totalbandwidth,sizeof(totalbandwidth)))
			return;
		totalbandwidth[log_wan_port] = strtoul(action, NULL, 0);
		if(!mib_set(MIB_MULTI_TOTAL_BANDWIDTH, (void*)&totalbandwidth))
#endif
		{
			ERR_MSG(Tset_mib_error);
			return;
		}
	}

	action = boaGetVar(wp, "bandwidth_defined", "");
	if (action[0]) {
#ifndef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
		policy = (action[0]=='0') ? 0 : 1;
#else
		if(!mib_get_s(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&policy,sizeof(policy)))
			return;
		policy[log_wan_port] = (action[0]=='0') ? 0 : 1;
#endif
		mib_set(MIB_TOTAL_BANDWIDTH_LIMIT_EN, &policy);
	}

done:
	//take effect
	take_qos_effect();
commit:
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	//redirect web page
	url = boaGetVar(wp, "submit-url", "");
	if(url[0])
	{
		boaRedirect(wp, url);
	}
	return;
}

#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
int initQosSpeedLimitRule(int eid, request * wp, int argc, char **argv)
{
	int nBytes = 0;
	unsigned char mode = 0;
	int i = 0, totalNum = 0;
	MIB_CE_DATA_SPEED_LIMIT_IF_T if_entry;
	MIB_CE_DATA_SPEED_LIMIT_VLAN_T vlan_entry;
	MIB_CE_DATA_SPEED_LIMIT_IP_T ip_entry;
	MIB_CE_DATA_SPEED_LIMIT_MAC_T mac_entry;

	mib_get(MIB_DATA_SPEED_LIMIT_UP_MODE, &mode);
	nBytes += boaWrite(wp, "rule_mode_up = %u;\n", mode);

	mib_get(MIB_DATA_SPEED_LIMIT_DOWN_MODE, &mode);
	nBytes += boaWrite(wp, "rule_mode_down = %u;\n", mode);


	totalNum = mib_chain_total(MIB_DATA_SPEED_LIMIT_UP_IF_TBL);
	for (i = 0; i < totalNum; i++)
	{
		if (!mib_chain_get(MIB_DATA_SPEED_LIMIT_UP_IF_TBL, i, (void*)&if_entry))
			continue;

		nBytes += boaWrite(wp, "rule_intf_up.push("
								"new it_nr(\"%d\","
								"new it(\"idx\", %d),"
								"new it(\"if_id\", \"%d\"),"
								"new it(\"speed_unit\", %d)));\n",
								i,
								i,
								if_entry.if_id,
								if_entry.speed_unit);
	}

	totalNum = mib_chain_total(MIB_DATA_SPEED_LIMIT_DOWN_IF_TBL);
	for (i = 0; i < totalNum; i++)
	{
		if (!mib_chain_get(MIB_DATA_SPEED_LIMIT_DOWN_IF_TBL, i, (void*)&if_entry))
			continue;

		nBytes += boaWrite(wp, "rule_intf_down.push("
								"new it_nr(\"%d\","
								"new it(\"idx\", %d),"
								"new it(\"if_id\", \"%d\"),"
								"new it(\"speed_unit\", %d)));\n",
								i,
								i,
								if_entry.if_id,
								if_entry.speed_unit);
	}

	totalNum = mib_chain_total(MIB_DATA_SPEED_LIMIT_UP_VLAN_TBL);
	for (i = 0; i < totalNum; i++)
	{
		if (!mib_chain_get(MIB_DATA_SPEED_LIMIT_UP_VLAN_TBL, i, (void*)&vlan_entry))
			continue;

		if(vlan_entry.vlan == -1)
		{
			nBytes += boaWrite(wp, "rule_vlan_up.push("
								"new it_nr(\"%d\","
								"new it(\"idx\", %d),"
								"new it(\"vlan\", \"untagged\"),"
								"new it(\"speed_unit\", %d)));\n",
								i,
								i,
								vlan_entry.speed_unit);
		}
		else
		{
			nBytes += boaWrite(wp, "rule_vlan_up.push("
								"new it_nr(\"%d\","
								"new it(\"idx\", %d),"
								"new it(\"vlan\", \"%d\"),"
								"new it(\"speed_unit\", %d)));\n",
								i,
								i,
								vlan_entry.vlan,
								vlan_entry.speed_unit);
		}
	}

	totalNum = mib_chain_total(MIB_DATA_SPEED_LIMIT_DOWN_VLAN_TBL);
	for (i = 0; i < totalNum; i++)
	{
		if (!mib_chain_get(MIB_DATA_SPEED_LIMIT_DOWN_VLAN_TBL, i, (void*)&vlan_entry))
			continue;

		nBytes += boaWrite(wp, "rule_vlan_down.push("
								"new it_nr(\"%d\","
								"new it(\"idx\", %d),"
								"new it(\"vlan\", %d),"
								"new it(\"speed_unit\", %d)));\n",
								i,
								i,
								vlan_entry.vlan,
								vlan_entry.speed_unit);
	}

	totalNum = mib_chain_total(MIB_DATA_SPEED_LIMIT_UP_IP_TBL);
	for (i = 0; i < totalNum; i++)
	{
		if (!mib_chain_get(MIB_DATA_SPEED_LIMIT_UP_IP_TBL, i, (void*)&ip_entry))
			continue;

		nBytes += boaWrite(wp, "rule_ip_up.push("
								"new it_nr(\"%d\","
								"new it(\"idx\", %d),"
								"new it(\"ip_start\", \"%s\"),"
								"new it(\"ip_end\", \"%s\"),"
								"new it(\"speed_unit\", %d)));\n",
								i,
								i,
								ip_entry.ip_start,
								ip_entry.ip_end,
								ip_entry.speed_unit);
	}

	totalNum = mib_chain_total(MIB_DATA_SPEED_LIMIT_DOWN_IP_TBL);
	for (i = 0; i < totalNum; i++)
	{
		if (!mib_chain_get(MIB_DATA_SPEED_LIMIT_DOWN_IP_TBL, i, (void*)&ip_entry))
			continue;

		nBytes += boaWrite(wp, "rule_ip_down.push("
								"new it_nr(\"%d\","
								"new it(\"idx\", %d),"
								"new it(\"ip_start\", \"%s\"),"
								"new it(\"ip_end\", \"%s\"),"
								"new it(\"speed_unit\", %d)));\n",
								i,
								i,
								ip_entry.ip_start,
								ip_entry.ip_end,
								ip_entry.speed_unit);
	}

	totalNum = mib_chain_total(MIB_DATA_SPEED_LIMIT_UP_MAC_TBL);
	for (i = 0; i < totalNum; i++)
	{
		if (!mib_chain_get(MIB_DATA_SPEED_LIMIT_UP_MAC_TBL, i, (void*)&mac_entry))
			continue;

		nBytes += boaWrite(wp, "rule_mac_up.push("
								"new it_nr(\"%d\","
								"new it(\"idx\", %d),"
								"new it(\"mac\", \"%02x:%02x:%02x:%02x:%02x:%02x\"),"
								"new it(\"speed_unit\", %d)));\n",
								i,
								i,
								mac_entry.mac[0], mac_entry.mac[1], mac_entry.mac[2],
								mac_entry.mac[3], mac_entry.mac[4], mac_entry.mac[5],
								mac_entry.speed_unit);
	}

	totalNum = mib_chain_total(MIB_DATA_SPEED_LIMIT_DOWN_MAC_TBL);
	for (i = 0; i < totalNum; i++)
	{
		if (!mib_chain_get(MIB_DATA_SPEED_LIMIT_DOWN_MAC_TBL, i, (void*)&mac_entry))
			continue;

		nBytes += boaWrite(wp, "rule_mac_down.push("
								"new it_nr(\"%d\","
								"new it(\"idx\", %d),"
								"new it(\"mac\", \"%02x:%02x:%02x:%02x:%02x:%02x\"),"
								"new it(\"speed_unit\", %d)));\n",
								i,
								i,
								mac_entry.mac[0], mac_entry.mac[1], mac_entry.mac[2],
								mac_entry.mac[3], mac_entry.mac[4], mac_entry.mac[5],
								mac_entry.speed_unit);
	}

	return nBytes;
}

void formQosSpeedLimit(request * wp, char *path, char *query)
{
	char *action = NULL, *url = NULL;
	char Mode_up = 0, Mode_down = 0;
	int chain_id = 0, chain_index = -1, chain_action = 0;
	int len=0;
	char tmplogBuf[300]={0};
	char action_str[32]={0};

	enum
	{
		CHAIN_ACTION_NONE = 0,
		CHAIN_ACTION_ADD,
		CHAIN_ACTION_EDIT,
		CHAIN_ACTION_DEL
	};

	action = boaGetVar(wp, "ModeSwitch_up", "");
	if (action[0]) {
		printf("ModeSwitch_up = %s\n", action);
		Mode_up = atoi(action);
	}
	
	action = boaGetVar(wp, "ModeSwitch_down", "");
	if (action[0]) {
		printf("ModeSwitch_down = %s\n", action);
		Mode_down = atoi(action);
	}

	action = boaGetVar(wp, "submitAction", "");
	if (action[0]) {
		printf("submitAction = %s\n", action);
		if (strcmp(action, "mode") == 0) {
			mib_set(MIB_DATA_SPEED_LIMIT_UP_MODE, &Mode_up);
			mib_set(MIB_DATA_SPEED_LIMIT_DOWN_MODE, &Mode_down);
			syslog(LOG_INFO, "QosSpeedLimit: Up Mode: %d, Down Mode: %d", Mode_up, Mode_down);
		}
		else if (strcmp(action, "rule") == 0) {
			action = boaGetVar(wp, "ruleDirection", "");
			if (action[0]) {
				printf("ruleDirection = %s\n", action);
				len += snprintf(tmplogBuf+len, sizeof(tmplogBuf)-len, ", ruleDirection: %s", action);
				if (strcmp(action, "up") == 0) {
					action = boaGetVar(wp, "ruleIndex_up", "");
					if (action[0]) {
						printf("ruleIndex_up = %s\n", action);
						chain_index = atoi(action);
					}

					action = boaGetVar(wp, "ruleAction_up", "");
					if (action[0]) {
						printf("ruleAction_up = %s\n", action);
						if (strcmp(action, "add") == 0) {
							chain_action = CHAIN_ACTION_ADD;
							action_str[sizeof(action_str)-1]='\0';
							strncpy(action_str, "Add",sizeof(action_str)-1);
						}
						else if (strcmp(action, "edit") == 0) {
							chain_action = CHAIN_ACTION_EDIT;
							action_str[sizeof(action_str)-1]='\0';
							strncpy(action_str, "Update",sizeof(action_str)-1);
						}
						else if (strcmp(action, "del") == 0) {
							chain_action = CHAIN_ACTION_DEL;
							action_str[sizeof(action_str)-1]='\0';
							strncpy(action_str, "Delete",sizeof(action_str)-1);
						}
					}

					if (Mode_up == 1) { // Interface
						MIB_CE_DATA_SPEED_LIMIT_IF_T entry;
						memset(&entry, 0, sizeof(MIB_CE_DATA_SPEED_LIMIT_IF_T));
						chain_id = MIB_DATA_SPEED_LIMIT_UP_IF_TBL;

						action = boaGetVar(wp, "InterfaceName_up", "");
						if (action[0]) {
							printf("InterfaceName_up = %s\n", action);
							entry.if_id = atoi(action);
						}

						action = boaGetVar(wp, "InterfaceSpeed_up", "");
						if (action[0]) {
							printf("InterfaceSpeed_up = %s\n", action);
							entry.speed_unit = atoi(action);
						}

						switch (chain_action)
						{
							case CHAIN_ACTION_ADD:
								mib_chain_add(chain_id, (void *)&entry);
								break;

							case CHAIN_ACTION_EDIT:
								if (chain_index >= 0) {
									mib_chain_update(chain_id, (void *)&entry, chain_index);
								}
								break;

							case CHAIN_ACTION_DEL:
								if (chain_index >= 0) {
									mib_chain_delete(chain_id, chain_index);
								}
								break;

							default:
								break;
						}
						len += snprintf(tmplogBuf+len, sizeof(tmplogBuf)-len, ", %s, if_id: %d, speed_unit: %d", action_str, entry.if_id, entry.speed_unit);
					}
					else if (Mode_up == 2) { // VlanTag
						MIB_CE_DATA_SPEED_LIMIT_VLAN_T entry;
						memset(&entry, 0, sizeof(MIB_CE_DATA_SPEED_LIMIT_VLAN_T));
						chain_id = MIB_DATA_SPEED_LIMIT_UP_VLAN_TBL;

						action = boaGetVar(wp, "VlanTagValue_up", "");
						if (action[0]) {
							printf("VlanTagValue_up = %s\n", action);
							if (strcasecmp(action, "untagged") == 0) {
								entry.vlan = -1;
							}
							else {
								entry.vlan = atoi(action);
							}
						}

						action = boaGetVar(wp, "VlanTagSpeed_up", "");
						if (action[0]) {
							printf("VlanTagSpeed_up = %s\n", action);
							entry.speed_unit = atoi(action);
						}

						switch (chain_action)
						{
							case CHAIN_ACTION_ADD:
								mib_chain_add(chain_id, (void *)&entry);
								break;

							case CHAIN_ACTION_EDIT:
								if (chain_index >= 0) {
									mib_chain_update(chain_id, (void *)&entry, chain_index);
								}
								break;

							case CHAIN_ACTION_DEL:
								if (chain_index >= 0) {
									mib_chain_delete(chain_id, chain_index);
								}
								break;

							default:
								break;
						}
						len += snprintf(tmplogBuf+len, sizeof(tmplogBuf)-len, ", %s, vlan: %d, speed_unit: %d", action_str, entry.vlan, entry.speed_unit);
					}
					else if (Mode_up == 3) { // IP
						MIB_CE_DATA_SPEED_LIMIT_IP_T entry;
						memset(&entry, 0, sizeof(MIB_CE_DATA_SPEED_LIMIT_IP_T));
						chain_id = MIB_DATA_SPEED_LIMIT_UP_IP_TBL;
						char ip_start[64] = {0}, ip_end[64] = {0};
						struct addrinfo hint, *res_start = NULL, *res_end = NULL;
						int ret = 0;

						action = boaGetVar(wp, "IP_Start_up", "");
						if (action[0]) {
							printf("IP_Start_up = %s\n", action);
							strncpy(ip_start, action, sizeof(ip_start)-1);
						}

						action = boaGetVar(wp, "IP_End_up", "");
						if (action[0]) {
							printf("IP_End_up = %s\n", action);
							strncpy(ip_end, action, sizeof(ip_end)-1);
						}

						if(chain_action != CHAIN_ACTION_DEL) {
							memset(&hint, 0, sizeof(hint));

							hint.ai_family = PF_UNSPEC;
							hint.ai_flags = AI_NUMERICHOST;

							ret = getaddrinfo(ip_start, NULL, &hint, &res_start);
							if (ret) {
								printf("Invalid start address %s: %s\n", ip_start, gai_strerror(ret));
								ERR_MSG("Invalid start address");
								if(res_start)
									freeaddrinfo(res_start);
								return;
							}

							ret = getaddrinfo(ip_end, NULL, &hint, &res_end);
							if (ret) {
								printf("Invalid end address %s: %s\n", ip_end, gai_strerror(ret));
								ERR_MSG("Invalid end address");
								if(res_start)
									freeaddrinfo(res_start);
								if(res_end)
									freeaddrinfo(res_end);
								return;
							}

							if (res_start->ai_family != res_end->ai_family) {
								printf("start address family != end address family\n");
								ERR_MSG("Invalid start or end address");
								if(res_start)
									freeaddrinfo(res_start);
								if(res_end)
									freeaddrinfo(res_end);
								return;
							}

							if(res_start->ai_family == AF_INET) {
								entry.ip_ver = IPVER_IPV4;
							}
							else if (res_start->ai_family == AF_INET6) {
								entry.ip_ver = IPVER_IPV6;
							}
							else {
								printf("%s is an is unknown address format %d\n", ip_start, res_start->ai_family);
								ERR_MSG("Unknown address format");
								if(res_start)
									freeaddrinfo(res_start);
								if(res_end)
									freeaddrinfo(res_end);
								return;
							}

							entry.ip_start[sizeof(entry.ip_start)-1]='\0';
							strncpy(entry.ip_start, ip_start,sizeof(entry.ip_start)-1);
							entry.ip_end[sizeof(entry.ip_end)-1]='\0';
							strncpy(entry.ip_end, ip_end,sizeof(entry.ip_end)-1);
							freeaddrinfo(res_start);
							freeaddrinfo(res_end);
						}
						
						action = boaGetVar(wp, "IPSpeed_up", "");
						if (action[0]) {
							printf("IPSpeed_up = %s\n", action);
							entry.speed_unit = atoi(action);
						}

						switch (chain_action)
						{
							case CHAIN_ACTION_ADD:
								mib_chain_add(chain_id, (void *)&entry);
								break;

							case CHAIN_ACTION_EDIT:
								if (chain_index >= 0) {
									mib_chain_update(chain_id, (void *)&entry, chain_index);
								}
								break;

							case CHAIN_ACTION_DEL:
								if (chain_index >= 0) {
									mib_chain_delete(chain_id, chain_index);
								}
								break;

							default:
								break;
						}
						len += snprintf(tmplogBuf+len, sizeof(tmplogBuf)-len, ", %s, ip_start: %s, ip_end: %s, speed_unit: %d", action_str, ip_start, ip_end, entry.speed_unit);
					}
					else if (Mode_up == 4) { // MAC
						MIB_CE_DATA_SPEED_LIMIT_MAC_T entry;
						memset(&entry, 0, sizeof(MIB_CE_DATA_SPEED_LIMIT_MAC_T));
						chain_id = MIB_DATA_SPEED_LIMIT_UP_MAC_TBL;
						char mac[13] = {0}, *p = NULL;
						int index = 0;

						memset(&entry, 0, sizeof(MIB_CE_DATA_SPEED_LIMIT_MAC_T));
						
						action = boaGetVar(wp, "MAC_up", "");
						if (action[0]) {
							printf("MAC_up = %s\n", action);
						}

						p = action;
						for(index = 0; (*p && (index < 12)); ++p) {
							if (*p!=':')
								mac[index++] = *p;
						}
						rtk_string_to_hex(mac, entry.mac, 12);
						printf("entry.mac = %x:%x:%x:%x:%x:%x\n", 
							entry.mac[0], entry.mac[1], entry.mac[2], 
							entry.mac[3], entry.mac[4], entry.mac[5]);

						action = boaGetVar(wp, "MACSpeed_up", "");
						if (action[0]) {
							printf("MACSpeed_up = %s\n", action);
							entry.speed_unit = atoi(action);
						}

						switch (chain_action)
						{
							case CHAIN_ACTION_ADD:
								mib_chain_add(chain_id, (void *)&entry);
								break;

							case CHAIN_ACTION_EDIT:
								if (chain_index >= 0) {
									mib_chain_update(chain_id, (void *)&entry, chain_index);
								}
								break;

							case CHAIN_ACTION_DEL:
								if (chain_index >= 0) {
									mib_chain_delete(chain_id, chain_index);
								}
								break;

							default:
								break;
						}
						len += snprintf(tmplogBuf+len, sizeof(tmplogBuf)-len, ", %s, mac %02x:%02x:%02x:%02x:%02x:%02x, speed_unit: %d",
							action_str, entry.mac[0], entry.mac[1], entry.mac[2], 
							entry.mac[3], entry.mac[4], entry.mac[5], entry.speed_unit);
					}
				}
				else if (strcmp(action, "down") == 0) {
					action = boaGetVar(wp, "ruleIndex_down", "");
					if (action[0]) {
						printf("ruleIndex_down = %s\n", action);
						chain_index = atoi(action);
					}
					
					action = boaGetVar(wp, "ruleAction_down", "");
					if (action[0]) {
						printf("ruleAction_down = %s\n", action);
						if (strcmp(action, "add") == 0) {
							chain_action = CHAIN_ACTION_ADD;
							action_str[sizeof(action_str)-1]='\0';
							strncpy(action_str, "Add",sizeof(action_str)-1);
						}
						else if (strcmp(action, "edit") == 0) {
							chain_action = CHAIN_ACTION_EDIT;
							action_str[sizeof(action_str)-1]='\0';
							strncpy(action_str, "Update",sizeof(action_str)-1);
						}
						else if (strcmp(action, "del") == 0) {
							chain_action = CHAIN_ACTION_DEL;
							action_str[sizeof(action_str)-1]='\0';
							strncpy(action_str, "Delete",sizeof(action_str)-1);
						}
					}

					if (Mode_down == 1) { // Interface
						MIB_CE_DATA_SPEED_LIMIT_IF_T entry;
						memset(&entry, 0, sizeof(MIB_CE_DATA_SPEED_LIMIT_IF_T));
						chain_id = MIB_DATA_SPEED_LIMIT_DOWN_IF_TBL;

						action = boaGetVar(wp, "InterfaceName_down", "");
						if (action[0]) {
							printf("InterfaceName_down = %s\n", action);
							entry.if_id = atoi(action);
						}

						action = boaGetVar(wp, "InterfaceSpeed_down", "");
						if (action[0]) {
							printf("InterfaceSpeed_down = %s\n", action);
							entry.speed_unit = atoi(action);
						}

						switch (chain_action)
						{
							case CHAIN_ACTION_ADD:
								mib_chain_add(chain_id, (void *)&entry);
								break;

							case CHAIN_ACTION_EDIT:
								if (chain_index >= 0) {
									mib_chain_update(chain_id, (void *)&entry, chain_index);
								}
								break;

							case CHAIN_ACTION_DEL:
								if (chain_index >= 0) {
									mib_chain_delete(chain_id, chain_index);
								}
								break;

							default:
								break;
						}

						len += snprintf(tmplogBuf+len, sizeof(tmplogBuf)-len, ", %s, if_id: %d, speed_unit: %d", action_str, entry.if_id, entry.speed_unit);
					}
					else if (Mode_down == 2) { // VlanTag
						MIB_CE_DATA_SPEED_LIMIT_VLAN_T entry;
						memset(&entry, 0, sizeof(MIB_CE_DATA_SPEED_LIMIT_VLAN_T));
						chain_id = MIB_DATA_SPEED_LIMIT_DOWN_VLAN_TBL;

						action = boaGetVar(wp, "VlanTagValue_down", "");
						if (action[0]) {
							printf("VlanTagValue_down = %s\n", action);
							if (strcasecmp(action, "untagged") == 0) {
								entry.vlan = -1;
							}
							else {
								entry.vlan = atoi(action);
							}
						}

						action = boaGetVar(wp, "VlanTagSpeed_down", "");
						if (action[0]) {
							printf("VlanTagSpeed_down = %s\n", action);
							entry.speed_unit = atoi(action);
						}

						switch (chain_action)
						{
							case CHAIN_ACTION_ADD:
								mib_chain_add(chain_id, (void *)&entry);
								break;

							case CHAIN_ACTION_EDIT:
								if (chain_index >= 0) {
									mib_chain_update(chain_id, (void *)&entry, chain_index);
								}
								break;

							case CHAIN_ACTION_DEL:
								if (chain_index >= 0) {
									mib_chain_delete(chain_id, chain_index);
								}
								break;

							default:
								break;
						}

						len += snprintf(tmplogBuf+len, sizeof(tmplogBuf)-len, ", %s, vlan: %d, speed_unit: %d", action_str, entry.vlan, entry.speed_unit);
					}
					else if (Mode_down == 3) { // IP
						MIB_CE_DATA_SPEED_LIMIT_IP_T entry;
						memset(&entry, 0, sizeof(MIB_CE_DATA_SPEED_LIMIT_IP_T));
						chain_id = MIB_DATA_SPEED_LIMIT_DOWN_IP_TBL;
						char ip_start[64] = {0}, ip_end[64] = {0};
						struct addrinfo hint, *res_start = NULL, *res_end = NULL;
						int ret = 0;

						action = boaGetVar(wp, "IP_Start_down", "");
						if (action[0]) {
							printf("IP_Start_down = %s\n", action);
							strncpy(ip_start, action, sizeof(ip_start)-1);
						}

						action = boaGetVar(wp, "IP_End_down", "");
						if (action[0]) {
							printf("IP_End_down = %s\n", action);
							strncpy(ip_end, action, sizeof(ip_end)-1);
						}

						if(chain_action != CHAIN_ACTION_DEL) {
							memset(&hint, 0, sizeof(hint));
							hint.ai_family = PF_UNSPEC;
							hint.ai_flags = AI_NUMERICHOST;

							ret = getaddrinfo(ip_start, NULL, &hint, &res_start);
							if (ret) {
								printf("Invalid start address %s: %s\n", ip_start, gai_strerror(ret));
								ERR_MSG("Invalid start address");
								if(res_start)
									freeaddrinfo(res_start);
								return;
							}

							ret = getaddrinfo(ip_end, NULL, &hint, &res_end);
							if (ret) {
								printf("Invalid end address %s: %s\n", ip_end, gai_strerror(ret));
								ERR_MSG("Invalid end address");
								if(res_start)
									freeaddrinfo(res_start);
								if(res_end)
									freeaddrinfo(res_end);
								return;
							}

							if (res_start->ai_family != res_end->ai_family) {
								printf("start address family != end address family\n");
								ERR_MSG("Invalid start or end address");
								if(res_start)
									freeaddrinfo(res_start);
								if(res_end)
									freeaddrinfo(res_end);
								return;
							}

							if(res_start->ai_family == AF_INET) {
								entry.ip_ver = IPVER_IPV4;
							}
							else if (res_start->ai_family == AF_INET6) {
								entry.ip_ver = IPVER_IPV6;
							}
							else {
								printf("%s is an is unknown address format %d\n", ip_start, res_start->ai_family);
								ERR_MSG("Unknown address format");
								if(res_start)
									freeaddrinfo(res_start);
								if(res_end)
									freeaddrinfo(res_end);
								return;
							}

							entry.ip_start[sizeof(entry.ip_start)-1]='\0';
							strncpy(entry.ip_start, ip_start,sizeof(entry.ip_start)-1);
							entry.ip_end[sizeof(entry.ip_end)-1]='\0';
							strncpy(entry.ip_end, ip_end,sizeof(entry.ip_end)-1);
							freeaddrinfo(res_start);
							freeaddrinfo(res_end);
						}
						
						action = boaGetVar(wp, "IPSpeed_down", "");
						if (action[0]) {
							printf("IPSpeed_down = %s\n", action);
							entry.speed_unit = atoi(action);
						}

						switch (chain_action)
						{
							case CHAIN_ACTION_ADD:
								mib_chain_add(chain_id, (void *)&entry);
								break;

							case CHAIN_ACTION_EDIT:
								if (chain_index >= 0) {
									mib_chain_update(chain_id, (void *)&entry, chain_index);
								}
								break;

							case CHAIN_ACTION_DEL:
								if (chain_index >= 0) {
									mib_chain_delete(chain_id, chain_index);
								}
								break;

							default:
								break;
						}

						len += snprintf(tmplogBuf+len, sizeof(tmplogBuf)-len, ", %s, ip_start: %s, ip_end: %s, speed_unit: %d", action_str, ip_start, ip_end, entry.speed_unit);
					}
					else if (Mode_down == 4) { // MAC
						MIB_CE_DATA_SPEED_LIMIT_MAC_T entry;
						memset(&entry, 0, sizeof(MIB_CE_DATA_SPEED_LIMIT_MAC_T));
						chain_id = MIB_DATA_SPEED_LIMIT_DOWN_MAC_TBL;
						char mac[13] = {0}, *p = NULL;
						int index = 0;

						memset(&entry, 0, sizeof(MIB_CE_DATA_SPEED_LIMIT_MAC_T));
						
						action = boaGetVar(wp, "MAC_down", "");
						if (action[0]) {
							printf("MAC_down = %s\n", action);
						}

						p = action;
						for(index = 0; (*p && (index < 12)); ++p) {
							if (*p!=':')
								mac[index++] = *p;
						}
						rtk_string_to_hex(mac, entry.mac, 12);
						printf("entry.mac = %x:%x:%x:%x:%x:%x\n", 
							entry.mac[0], entry.mac[1], entry.mac[2], 
							entry.mac[3], entry.mac[4], entry.mac[5]);

						action = boaGetVar(wp, "MACSpeed_down", "");
						if (action[0]) {
							printf("MACSpeed_down = %s\n", action);
							entry.speed_unit = atoi(action);
						}

						switch (chain_action)
						{
							case CHAIN_ACTION_ADD:
								mib_chain_add(chain_id, (void *)&entry);
								break;

							case CHAIN_ACTION_EDIT:
								if (chain_index >= 0) {
									mib_chain_update(chain_id, (void *)&entry, chain_index);
								}
								break;

							case CHAIN_ACTION_DEL:
								if (chain_index >= 0) {
									mib_chain_delete(chain_id, chain_index);
								}
								break;

							default:
								break;
						}
						len += snprintf(tmplogBuf+len, sizeof(tmplogBuf)-len, ", %s, mac %02x:%02x:%02x:%02x:%02x:%02x, speed_unit: %d",
							action_str, entry.mac[0], entry.mac[1], entry.mac[2], 
							entry.mac[3], entry.mac[4], entry.mac[5], entry.speed_unit);
					}
				}
			}
			syslog(LOG_INFO, "QosSpeedLimit: Up Mode: %d, Down Mode: %d%s", Mode_up, Mode_down, tmplogBuf);
		}
	}

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
	//take effect
	take_qos_effect();

	//redirect web page
	url = boaGetVar(wp, "submit-url", "");
	if (url[0]) {
		boaRedirect(wp, url);
	}
	return;
}
#endif

int ipqos_dhcpopt(int eid, request * wp, int argc, char **argv)
{
	int nBytes = 0;

#ifdef CONFIG_BRIDGE_EBT_DHCP_OPT
	nBytes += boaWrite(wp, " <td><font size=2><input type=\"radio\"  name=qos_rule_type value=4 onClick=ruleType_click();> DHCP Option</td>");	
#endif

	return nBytes;
}


int ipqos_dhcpopt_getoption60(int eid, request * wp, int argc, char **argv)
{
	int nBytes = 0, index=0;
	MIB_CE_IP_QOS_T entry;
	int start=0,end=0,len=0;
	char index_buf[3]={0};

#ifdef CONFIG_BRIDGE_EBT_DHCP_OPT
	if(wp->query_string)
	{
		start = strstr(wp->query_string,"rule_index=")+sizeof("rule_index=")-1;
		end = strstr(wp->query_string,"&rule");
		len=end-start;
		if(start&&end&&(len>0))
		{
			memcpy(index_buf,start,len);

			index= atoi(index_buf);
	if(!mib_chain_get(MIB_IP_QOS_TBL, index, &entry))
			return;
	nBytes += boaWrite(wp, "%s", entry.opt60_vendorclass);	
		}
	}
#endif

	return nBytes;

}


#ifdef QOS_TRAFFIC_SHAPING_BY_SSID
int ssid_list(int eid, request * wp, int argc, char **argv)
{
	MIB_CE_MBSSIB_T Entry;
	int i,vwlan_idx;
	int nBytesSent=0;
	int ori_wlan_idx;

	ori_wlan_idx = wlan_idx;

	printf("[%s:%d]\n",__func__,__LINE__);
	nBytesSent += boaWrite(wp, "<option value=\"\"></option>\n");

	for(i = 0; i<NUM_WLAN_INTERFACE; i++) {
		wlan_idx = i;
		if (!getInFlags((char *)getWlanIfName(), 0)) {
			printf("Wireless Interface Not Found !\n");
			continue;
	    }
		
#ifdef WLAN_MBSSID
		for (vwlan_idx=0; vwlan_idx<=NUM_VWLAN_INTERFACE; vwlan_idx++) {
			wlan_getEntry(&Entry, vwlan_idx);
			if(Entry.wlanDisabled)
				continue;
		
			printf("wlan_idx=%d, vwlan_idx=%d, ssid:%s\n",wlan_idx, vwlan_idx,Entry.ssid);
			nBytesSent += boaWrite(wp, "<option value=\"%s\">%s</option>\n",
				Entry.ssid, Entry.ssid);

		}
#endif
	}
	wlan_idx = ori_wlan_idx;
	return nBytesSent;
}
#endif

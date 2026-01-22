
#include <string.h>
#include "debug.h"
#include "utility.h"
#include <string.h>
#include <arpa/inet.h>
#include "subr_net.h"

extern const char FW_PREROUTING[];
extern const char FW_POSTROUTING[];

#if defined(PORT_FORWARD_GENERAL) || defined(DMZ) || defined(VIRTUAL_SERVER_SUPPORT)
#ifdef NAT_LOOPBACK
#ifdef PORT_FORWARD_GENERAL
static int iptable_fw_natLB_dynamic( char *ifname, const char *proto, const char *extPort, const char *intPort, const char *dstIP, const char *myip, char *nat_pre_name, char *nat_post_name, int fh)
{	
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	unsigned char ipStr[20], subnetStr[20];
	unsigned long mask, mbit, ip, subnet;
	char lan_ipaddr[32], dstIP2[32], *pStr, buff[256];	
	
	// (1) Get LAN IP and mask
	mib_get_s(MIB_ADSL_LAN_IP, (void *)&ip, sizeof(ip));
	strncpy(ipStr, inet_ntoa(*((struct in_addr *)&ip)), 16);
	ipStr[15] = '\0';

	mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)&mask, sizeof(mask));
	subnet = (ip&mask);	
	strncpy(subnetStr, inet_ntoa(*((struct in_addr *)&subnet)), 16);
	subnetStr[15] = '\0';

	mbit=0;
	mask = htonl(mask);
	while (1) {
		if (mask&0x80000000) {
			mbit++;
			mask <<= 1;
		}
		else
			break;
	}
	snprintf(lan_ipaddr, sizeof(lan_ipaddr), "%s/%lu", subnetStr, mbit);
	dstIP2[sizeof(dstIP2)-1]='\0';
	strncpy(dstIP2, dstIP, sizeof(dstIP2)-1);
	pStr = strstr(dstIP2, ":");
	if (pStr != NULL)
		pStr[0] = '\0';
	
	// (2) Set rule	for spppd
	if (fh==0)	// It is for spppd connection
	{
		// (2.1) Set prerouting rule
		// iptables -t nat -A portfwPreNatLB_ppp0 -s 192.168.1.0/24 -d 192.168.8.1 -p TCP --dport 2121 -j DNAT --to-destination 192.168.1.20:21
		if (extPort) {
			va_cmd(IPTABLES, 16, 1, "-t", "nat",
				"-A", nat_pre_name,
				"-s", lan_ipaddr,
				"-d", myip,
				"-p", (char *)proto,
				(char *)FW_DPORT, extPort, "-j",
				"DNAT", "--to-destination", dstIP);
		} else {
			va_cmd(IPTABLES, 14, 1, "-t", "nat",
				"-A", nat_pre_name,
				"-s", lan_ipaddr,
				"-d", myip,
				"-p", (char *)proto,
				"-j", "DNAT", "--to-destination", dstIP);
		}
		
		// (2.2) Set postrouting rule
		//iptables -t nat -A portfwPostNatLB_ppp0 -s 192.168.1.0/24 -d 192.168.1.20 -p TCP --dport 21 -j MASQUERADE	
		if (intPort) {
			va_cmd(IPTABLES, 14, 1, "-t", "nat",
				"-A", nat_post_name,
				"-s", lan_ipaddr,
				"-d", dstIP2,
				"-p", (char *)proto,
				(char *)FW_DPORT, intPort, 				
				"-j",	"MASQUERADE");
		} else {
			va_cmd(IPTABLES, 12, 1, "-t", "nat",
				"-A", nat_post_name,
				"-s", lan_ipaddr,
				"-d", dstIP2,
				"-p", (char *)proto,				
				"-j", "MASQUERADE");
		}
	}
	
	// (3) set rule for DHCP/pppd
	else // It is for DHCP/pppd dynamic connection
	{
		// (3.1) Set prerouting rule
		// iptables -t nat -A portfwPreNatLB_ptm0_0 -s 192.168.1.0/24 -d 192.168.8.1 -p TCP --dport 2121 -j DNAT --to-destination 192.168.1.20:21
		if (extPort) {			
			snprintf(buff, sizeof(buff), "\tiptables -t nat -A %s -s %s -d %s -p %s --dport %s -j DNAT --to-destination %s\n", nat_pre_name, lan_ipaddr, myip, (char *)proto, extPort, dstIP);
			WRITE_DHCPC_FILE(fh, buff);
		} else {			
			snprintf(buff, sizeof(buff), "\tiptables -t nat -A %s -s %s -d %s -p %s -j DNAT --to-destination %s\n", nat_pre_name, lan_ipaddr, myip, (char *)proto, dstIP);
			WRITE_DHCPC_FILE(fh, buff);
		}
		
		// (3.2) Set postrouting rule
		//iptables -t nat -A portfwPostNatLB_ptm0_0 -s 192.168.1.0/24 -d 192.168.1.20 -p TCP --dport 21 -j MASQUERADE	
		if (intPort) {			
			snprintf(buff, sizeof(buff), "\tiptables -t nat -A %s -s %s -d %s -p %s --dport %s -j MASQUERADE\n", nat_post_name, lan_ipaddr, dstIP2, (char *)proto, intPort);
			WRITE_DHCPC_FILE(fh, buff);
		} else {			
			snprintf(buff, sizeof(buff), "\tiptables -t nat -A %s -s %s -d %s -p %s -j MASQUERADE\n", nat_post_name, lan_ipaddr, dstIP2, (char *)proto);
			WRITE_DHCPC_FILE(fh, buff);
		}
	}
	return 0;

}

static void portfw_NATLB_dynamic( MIB_CE_PORT_FW_T *p, char *ifname, char *myip, char *nat_pre_name, char *nat_post_name, int fh)
{
	int hasExtPort=0, hasLocalPort=0;
	char * proto = 0;
	char intPort[32], extPort[32];
	char ipaddr[32], extra[32];	
	
	if(p==NULL) return;

    if (p->fromPort)
    {
        if (p->fromPort == p->toPort)
        {
            snprintf(intPort, sizeof(intPort), "%u", p->fromPort);
        }
        else
        {
            /* "%u-%u" is used by port forwarding */
            snprintf(intPort, sizeof(intPort), "%u-%u", p->fromPort, p->toPort);
        }

        snprintf(ipaddr, sizeof(ipaddr), "%s:%s", inet_ntoa(*((struct in_addr *)p->ipAddr)), intPort);

        if (p->fromPort != p->toPort)
        {
            /* "%u:%u" is used by filter */
            snprintf(intPort, sizeof(intPort), "%u:%u", p->fromPort, p->toPort);
        }
	hasLocalPort = 1;
    }
    else
    {
        snprintf(ipaddr, sizeof(ipaddr), "%s", inet_ntoa(*((struct in_addr *)p->ipAddr)));
	hasLocalPort = 0;
    }


	if (p->externalfromport && p->externaltoport && (p->externalfromport != p->externaltoport)) {
		snprintf(extPort, sizeof(extPort), "%u:%u", p->externalfromport, p->externaltoport);
		hasExtPort = 1;
	} else if (p->externalfromport) {
		snprintf(extPort, sizeof(extPort), "%u", p->externalfromport);
		hasExtPort = 1;
	} else if (p->externaltoport) {
		snprintf(extPort, sizeof(extPort), "%u", p->externaltoport);
		hasExtPort = 1;
	} else {
		hasExtPort = 0;
	}	
	//printf( "portfw_NATLB_dynamic: extPort is %s hasExtPort=%d, myip is %s\n",  extPort, hasExtPort, myip);
	//printf( "entry.externalfromport:%d entry.externaltoport=%d\n",  p->externalfromport, p->externaltoport);	
	
	//check something
	//internalclient can't be zeroip
	if( strncmp(ipaddr,"0.0.0.0", 7)==0 ) return;

	if (p->protoType == PROTO_TCP || p->protoType == PROTO_UDPTCP)
	{		
		iptable_fw_natLB_dynamic( ifname, ARG_TCP, hasExtPort ? extPort : 0, hasLocalPort ? intPort : 0, ipaddr, myip, nat_pre_name, nat_post_name, fh);
	}

	if (p->protoType == PROTO_UDP || p->protoType == PROTO_UDPTCP)
	{	
		iptable_fw_natLB_dynamic( ifname, ARG_UDP, hasExtPort ? extPort : 0, hasLocalPort ? intPort : 0, ipaddr, myip, nat_pre_name, nat_post_name, fh);
	}
}
#endif

#ifdef VIRTUAL_SERVER_SUPPORT
static int iptable_virtual_server_natLB_dynamic( char *ifname, const char *proto, const char *extPort, const char *intPort, const char *dstIP, const char *myip, char *nat_pre_name, char *nat_post_name, int fh)
{	
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	unsigned char ipStr[20], subnetStr[20];
	unsigned long mask, mbit, ip, subnet;
	char lan_ipaddr[32], dstIP2[32], *pStr, buff[256];	
	
	// (1) Get LAN IP and mask
	mib_get_s(MIB_ADSL_LAN_IP, (void *)&ip, sizeof(ip));
	strncpy(ipStr, inet_ntoa(*((struct in_addr *)&ip)), 16);
	ipStr[15] = '\0';

	mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)&mask, sizeof(mask));
	subnet = (ip&mask);	
	strncpy(subnetStr, inet_ntoa(*((struct in_addr *)&subnet)), 16);
	subnetStr[15] = '\0';

	mbit=0;
	mask = htonl(mask);
	while (1) {
		if (mask&0x80000000) {
			mbit++;
			mask <<= 1;
		}
		else
			break;
	}
	snprintf(lan_ipaddr, sizeof(lan_ipaddr), "%s/%lu", subnetStr, mbit);
	dstIP2[sizeof(dstIP2)-1]='\0';
	strncpy(dstIP2, dstIP, sizeof(dstIP2)-1);
	pStr = strstr(dstIP2, ":");
	if (pStr != NULL)
		pStr[0] = '\0';
	
	// (2) Set rule	for spppd
	if (fh==0)	// It is for spppd connection
	{
		// (2.1) Set prerouting rule
		// iptables -t nat -A virserverPreNatLB_ppp0 -s 192.168.1.0/24 -d 192.168.8.1 -p TCP --dport 2121 -j DNAT --to-destination 192.168.1.20:21
		if (extPort) {
			va_cmd(IPTABLES, 16, 1, "-t", "nat",
				"-A", nat_pre_name,
				"-s", lan_ipaddr,
				"-d", myip,
				"-p", (char *)proto,
				(char *)FW_DPORT, extPort, "-j",
				"DNAT", "--to-destination", dstIP);
		} else {
			va_cmd(IPTABLES, 14, 1, "-t", "nat",
				"-A", nat_pre_name,
				"-s", lan_ipaddr,
				"-d", myip,
				"-p", (char *)proto,
				"-j", "DNAT", "--to-destination", dstIP);
		}
		
		// (2.2) Set postrouting rule
		//iptables -t nat -A virserverNatLB_ppp0 -s 192.168.1.0/24 -d 192.168.1.20 -p TCP --dport 21 -j SNAT --to-source 192.168.8.1
		if (intPort) {
			va_cmd(IPTABLES, 16, 1, "-t", "nat",
				"-A", nat_post_name,
				"-s", lan_ipaddr,
				"-d", dstIP2,
				"-p", (char *)proto,
				(char *)FW_DPORT, intPort, 				
				"-j",	"SNAT", "--to-source", myip);
		} else {
			va_cmd(IPTABLES, 14, 1, "-t", "nat",
				"-A", nat_post_name,
				"-s", lan_ipaddr,
				"-d", dstIP2,
				"-p", (char *)proto,				
				"-j", "SNAT", "--to-source", myip);
		}
	}

	// (3) set rule for DHCP/pppd
	else // It is for DHCP/pppd dynamic connection
	{
		// (3.1) Set prerouting rule
		// iptables -t nat -A virserverPreNatLB_ptm0_0 -s 192.168.1.0/24 -d 192.168.8.1 -p TCP --dport 2121 -j DNAT --to-destination 192.168.1.20:21
		if (extPort) {			
			snprintf(buff, sizeof(buff), "\tiptables -t nat -A %s -s %s -d %s -p %s --dport %s -j DNAT --to-destination %s\n", nat_pre_name, lan_ipaddr, myip, (char *)proto, extPort, dstIP);
			WRITE_DHCPC_FILE(fh, buff);
		} else {			
			snprintf(buff, sizeof(buff), "\tiptables -t nat -A %s -s %s -d %s -p %s -j DNAT --to-destination %s\n", nat_pre_name, lan_ipaddr, myip, (char *)proto, dstIP);
			WRITE_DHCPC_FILE(fh, buff);
		}
		
		// (3.2) Set postrouting rule
		//iptables -t nat -A virserverPostNatLB_ptm0_0 -s 192.168.1.0/24 -d 192.168.1.20 -p TCP --dport 21 -j SNAT --to-source 192.168.8.1	
		if (intPort) {			
			snprintf(buff, sizeof(buff), "\tiptables -t nat -A %s -s %s -d %s -p %s --dport %s -j SNAT --to-source %s\n", nat_post_name, lan_ipaddr, dstIP2, (char *)proto, intPort, myip);
			WRITE_DHCPC_FILE(fh, buff);
		} else {			
			snprintf(buff, sizeof(buff), "\tiptables -t nat -A %s -s %s -d %s -p %s -j SNAT --to-source %s\n", nat_post_name, lan_ipaddr, dstIP2, (char *)proto, myip);
			WRITE_DHCPC_FILE(fh, buff);
		}
	}

	return 0;

}

static void virtual_server_NATLB_dynamic( MIB_VIRTUAL_SVR_T *p, char *ifname, char *myip, char *nat_pre_name, char *nat_post_name, int fh)
{
	int hasExtPort=0, hasLocalPort=0;
	char * proto = 0;
	char intPort[32], extPort[32];
	char ipaddr[32], extra[32];	
	char lan_dport[32]={0};
	char lan_dport2[32]={0};
	
	if(p==NULL) return;

#ifdef VIRTUAL_SERVER_MODE_NTO1
	if(p->mappingType == 1){
	//N-to-1
		snprintf(lan_dport, sizeof(lan_dport), "%u", p->lanPort);
		snprintf(lan_dport2, sizeof(lan_dport2), "%u", p->lanPort);
	}else
#endif	
	{
		snprintf(lan_dport, sizeof(lan_dport), "%u-%u", p->lanPort, p->lanPort+(p->wanEndPort-p->wanStartPort));
		snprintf(lan_dport2, sizeof(lan_dport2), "%u:%u", p->lanPort, p->lanPort+(p->wanEndPort-p->wanStartPort));
	}

	if (p->lanPort)
	{	
		snprintf(ipaddr, sizeof(ipaddr), "%s:%s", inet_ntoa(*((struct in_addr *)p->serverIp)), lan_dport);
		hasLocalPort = 1;
	}
	else
	{
		snprintf(ipaddr, sizeof(ipaddr), "%s", inet_ntoa(*((struct in_addr *)p->serverIp)));
		hasLocalPort = 0;
	}
	if (p->wanStartPort && p->wanEndPort && (p->wanStartPort != p->wanEndPort)){
		snprintf(extPort, sizeof(extPort), "%u:%u", p->wanStartPort, p->wanEndPort);
		hasExtPort = 1;
	}else if (p->wanStartPort) {
		snprintf(extPort, sizeof(extPort), "%u", p->wanStartPort);
		hasExtPort = 1;
	} else if (p->wanEndPort) {
		snprintf(extPort, sizeof(extPort), "%u", p->wanEndPort);
		hasExtPort = 1;
	} else {
		hasExtPort = 0;
	}

	//printf( "portfw_NATLB_dynamic: extPort is %s hasExtPort=%d, myip is %s\n",  extPort, hasExtPort, myip);
	//printf( "entry.externalfromport:%d entry.externaltoport=%d\n",  p->externalfromport, p->externaltoport);	
	
	//check something
	//internalclient can't be zeroip
	if( strncmp(ipaddr,"0.0.0.0", 7)==0 ) return;

	if (p->protoType == PROTO_TCP || p->protoType == PROTO_UDPTCP)
	{		
		iptable_virtual_server_natLB_dynamic( ifname, ARG_TCP, hasExtPort ? extPort : 0, hasLocalPort ? lan_dport2 : 0, ipaddr, myip, nat_pre_name, nat_post_name, fh);
	}

	if (p->protoType == PROTO_UDP || p->protoType == PROTO_UDPTCP)
	{	
		iptable_virtual_server_natLB_dynamic( ifname, ARG_UDP, hasExtPort ? extPort : 0, hasLocalPort ? lan_dport2 : 0, ipaddr, myip, nat_pre_name, nat_post_name, fh);
	}
}
#endif


#ifdef DMZ
static int iptable_dmz_natLB_dynamic(char *ifname, char *dstIP, char *myip, char *nat_pre_name, char *nat_post_name, int fh)
{	
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	unsigned char ipStr[20], subnetStr[20];
	unsigned long mask, mbit, ip, subnet;
	char lan_ipaddr[32], buff[128];	
	
	// (1) Get LAN IP and mask
	mib_get_s(MIB_ADSL_LAN_IP, (void *)&ip, sizeof(ip));
	strncpy(ipStr, inet_ntoa(*((struct in_addr *)&ip)), 16);
	ipStr[15] = '\0';

	mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)&mask, sizeof(mask));
	subnet = (ip&mask);	
	strncpy(subnetStr, inet_ntoa(*((struct in_addr *)&subnet)), 16);
	subnetStr[15] = '\0';

	mbit=0;
	mask = htonl(mask);
	while (1) {
		if (mask&0x80000000) {
			mbit++;
			mask <<= 1;
		}
		else
			break;
	}
	snprintf(lan_ipaddr, sizeof(lan_ipaddr), "%s/%lu", subnetStr, mbit);
	
	// (2) Set rule	
	if (fh==0)	// It is for spppd connection
	{
		// (2.1) Set prerouting rule
		// iptables -t nat -A dmzPreNatLB_ppp0 -s 192.168.1.0/24 -d 192.168.8.1 -j DNAT --to-destination 192.168.1.20		
		va_cmd(IPTABLES, 12, 1, "-t", "nat",
			"-A", nat_pre_name,
			"-s", lan_ipaddr,
			"-d", myip,			
			"-j",	"DNAT", "--to-destination", dstIP);
			
		// (2.2) Set postrouting rule
		//iptables -t nat -A dmzPostNatLB_ppp0 -s 192.168.1.0/24 -d 192.168.1.20  -j  MASQUERADE	
		va_cmd(IPTABLES, 10, 1, "-t", "nat",
			"-A", nat_post_name,
			"-s", lan_ipaddr,
			"-d", dstIP,			
			"-j",	"MASQUERADE");	
	}
	
	// (3) set rule for DHCP/pppd
	else // It is for DHCP/pppd dynamic connection
	{
		// (3.1) Set prerouting rule
		// iptables -t nat -A dmzPreNatLB_ptm0_0 -s 192.168.1.0/24 -d 192.168.8.1 -j DNAT --to-destination 192.168.1.20		
		snprintf(buff, 128, "\tiptables -t nat -A %s -s %s -d %s -j DNAT --to-destination %s\n", nat_pre_name, lan_ipaddr, myip, dstIP);
		WRITE_DHCPC_FILE(fh, buff);
		
		// (3.2) Set postrouting rule
		//iptables -t nat -A dmzPostNatLB_ptm0_0 -s 192.168.1.0/24 -d 192.168.1.20 -j MASQUERADE		
		snprintf(buff, 128, "\tiptables -t nat -A %s -s %s -d %s -j MASQUERADE\n", nat_post_name, lan_ipaddr, dstIP);
		WRITE_DHCPC_FILE(fh, buff);
	}

	return 0;
}

static int set_dmz_natLB_dynamic(char *ifname, char *myip, char *nat_pre_name, char *nat_post_name, int fh)
{
	int vInt;
	unsigned char value[32];
	char ipaddr[32];	

	vInt = 0;
	if (mib_get_s(MIB_DMZ_ENABLE, (void *)value, sizeof(value)) != 0)
		vInt = (int)(*(unsigned char *)value);

	if (mib_get_s(MIB_DMZ_IP, (void *)value, sizeof(value)) != 0)
	{
		if (vInt == 1)
		{
			strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
			ipaddr[15] = '\0';
			iptable_dmz_natLB_dynamic(ifname, ipaddr, myip, nat_pre_name, nat_post_name, fh);
		}
	}
	return 0;
}
#endif

static int del_NATLB_dynamic(char *nat_pre_name, char *nat_post_name)
{
	// iptables -t nat -F portfwPreNatLB_ppp0
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", nat_pre_name);
	// iptables -t nat -A PREROUTING -j portfwPreNatLB_pppo
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", nat_pre_name);
	// iptables -t nat -X portfwPreNatLB_ppp0
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", nat_pre_name);
	
	// iptables -t nat -N portfwPostNatLB_ppp0
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", nat_post_name);
	// iptables -t nat -A POSTROUTING -j portfwPostNatLB_ppp0
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_POSTROUTING, "-j", nat_post_name);
	// iptables -t nat -X portfwPostNatLB_ppp0
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", nat_post_name);
	return 0;
}

int cleanOneEntry_NATLB_rule_dynamic_link(MIB_CE_ATM_VC_Tp pEntry, int mode)
{	
	char ifname[10];
	char nat_pre_name[30], nat_post_name[30];

	ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));	
	
	if ((DHCP_T)pEntry->ipDhcp == DHCP_CLIENT || pEntry->cmode == CHANNEL_MODE_PPPOE || pEntry->cmode == CHANNEL_MODE_PPPOA)
	{
#ifdef PORT_FORWARD_GENERAL
		// (1) del NATLB of portfw rule
		if ( mode == DEL_ALL_NATLB_DYNAMIC || mode == DEL_PORTFW_NATLB_DYNAMIC)
		{
			snprintf(nat_pre_name, 30, "%s_%s", PORT_FW_PRE_NAT_LB, ifname);
			snprintf(nat_post_name, 30, "%s_%s", PORT_FW_POST_NAT_LB, ifname);
			del_NATLB_dynamic(nat_pre_name, nat_post_name);
		}
#endif
		
#ifdef DMZ
		// (2) del NATLB of dmz rule
		if ( mode == DEL_ALL_NATLB_DYNAMIC || mode == DEL_DMZ_NATLB_DYNAMIC)
		{
			snprintf(nat_pre_name, 30, "%s_%s", IPTABLE_DMZ_PRE_NAT_LB, ifname);
			snprintf(nat_post_name, 30, "%s_%s", IPTABLE_DMZ_POST_NAT_LB, ifname);
			del_NATLB_dynamic(nat_pre_name, nat_post_name);
		}
#endif
	
#ifdef VIRTUAL_SERVER_SUPPORT
		// (3) del NATLB of virtual_server rule
		if ( mode == DEL_ALL_NATLB_DYNAMIC || mode == DEL_VIR_SER_NATLB_DYNAMIC)
		{
			snprintf(nat_pre_name, 30, "%s_%s", VIR_SER_PRE_NAT_LB, ifname);
			snprintf(nat_post_name, 30, "%s_%s", VIR_SER_POST_NAT_LB, ifname);
			del_NATLB_dynamic(nat_pre_name, nat_post_name);
		}
#endif

	}
	return 0;
}

int cleanALLEntry_NATLB_rule_dynamic_link(int mode)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
			printf("cleanAll_NATLB_rule_dynamic_link: cannot get ATM_VC_TBL(ch=%d) entry\n", i);
			return 0;
		}

		cleanOneEntry_NATLB_rule_dynamic_link(&Entry, mode);
	}
	return 0;
}

#ifdef PORT_FORWARD_GENERAL
int add_portfw_dynamic_NATLB_chain(void)
{
	char ifname[16] = {0};
	char nat_pre_name[30] = {0}, nat_post_name[30] = {0};
	unsigned int entryNum = 0, i = 0;
	MIB_CE_ATM_VC_T Entry;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			printf("%s: cannot get ATM_VC_TBL.%d entry\n", __FUNCTION__, i);
			continue;
		}

		if ((DHCP_T)Entry.ipDhcp == DHCP_CLIENT || Entry.cmode == CHANNEL_MODE_PPPOE || Entry.cmode == CHANNEL_MODE_PPPOA)
		{
			ifGetName(Entry.ifIndex, ifname, sizeof(ifname));
			snprintf(nat_pre_name, sizeof(nat_pre_name), "%s_%s", PORT_FW_PRE_NAT_LB, ifname);
			snprintf(nat_post_name, sizeof(nat_post_name), "%s_%s", PORT_FW_POST_NAT_LB, ifname);

			va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", nat_pre_name);
			va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", nat_pre_name);

			va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", nat_post_name);
			va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", nat_post_name);
		}
	}

	return 0;
}
#endif

static int add_dynamic_NATLB_iptable_rule(char *nat_pre_name, char *nat_post_name, char *wanip, char *dip)
{
	unsigned char ipStr[20] = {0}, subnetStr[20] = {0};
	unsigned long mask = 0, mbit = 0, ip = 0, subnet = 0;
	char lan_ipaddr[32] = {0};

	if (nat_pre_name == NULL || nat_post_name == NULL || wanip == NULL || dip == NULL)
		return -1;

	// iptables -t nat -N portfwPreNatLB_ppp0
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", nat_pre_name);
	// iptables -t nat -A PREROUTING -j portfwPreNatLB_pppo
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", nat_pre_name);

	// iptables -t nat -N portfwPostNatLB_ppp0
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", nat_post_name);
	// iptables -t nat -A POSTROUTING -j portfwPostNatLB_ppp0
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", nat_post_name);

	// (1) Get LAN IP and mask
	mib_get_s(MIB_ADSL_LAN_IP, (void *)&ip, sizeof(ip));
	strncpy(ipStr, inet_ntoa(*((struct in_addr *)&ip)), 16);
	ipStr[15] = '\0';

	mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)&mask, sizeof(mask));
	subnet = (ip&mask);
	strncpy(subnetStr, inet_ntoa(*((struct in_addr *)&subnet)), 16);
	subnetStr[15] = '\0';

	mbit = 0;
	mask = htonl(mask);
	while (1)
	{
		if (mask&0x80000000)
		{
			mbit++;
			mask <<= 1;
		}
		else
			break;
	}
	snprintf(lan_ipaddr, sizeof(lan_ipaddr), "%s/%d", subnetStr, mbit);

	//(2) Set rule
	// (2.1) Set prerouting rule
	va_cmd(IPTABLES, 12, 1, "-t", "nat",
			"-A", nat_pre_name,
			"-s", lan_ipaddr,
			"-d", wanip,
			"-j",	"DNAT", "--to-destination", dip);

	// (2.2) Set postrouting rule
	va_cmd(IPTABLES, 10, 1, "-t", "nat",
			"-A", nat_post_name,
			"-s", lan_ipaddr,
			"-d", dip,
			"-j",	"MASQUERADE");

	return 0;
}

int addOneEntry_NATLB_rule_dynamic_link(MIB_CE_ATM_VC_Tp pEntry, char *dstIP, int mode)
{
	char ifname[16] = {0};
	char nat_pre_name[30] = {0}, nat_post_name[30] = {0};
	int flags = 0;
	struct in_addr inAddr;
	char ip4addr_str[16] = {0};

	ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));

	if ((DHCP_T)pEntry->ipDhcp == DHCP_CLIENT || pEntry->cmode == CHANNEL_MODE_PPPOE || pEntry->cmode == CHANNEL_MODE_PPPOA)
	{
		if (getInFlags(ifname, &flags) == 1 && getInAddr(ifname, IP_ADDR, &inAddr) == 1 && dstIP != NULL)
		{
			inet_ntop(AF_INET, (void *)&inAddr, ip4addr_str, 16);
#ifdef DMZ
			// (2) add NATLB of dmz rule
			if ( mode == DEL_ALL_NATLB_DYNAMIC || mode == DEL_DMZ_NATLB_DYNAMIC)
			{
				snprintf(nat_pre_name, sizeof(nat_pre_name), "%s_%s", IPTABLE_DMZ_PRE_NAT_LB, ifname);
				snprintf(nat_post_name, sizeof(nat_post_name), "%s_%s", IPTABLE_DMZ_POST_NAT_LB, ifname);
				add_dynamic_NATLB_iptable_rule(nat_pre_name, nat_post_name, ip4addr_str, dstIP);
			}
#endif
		}
	}
	return 0;
}

int addALLEntry_NATLB_rule_dynamic_link(char *dstIP, int mode)
{
	unsigned int entryNum = 0, i = 0;
	MIB_CE_ATM_VC_T Entry;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			printf("%s: cannot get ATM_VC_TBL.%d entry\n", __FUNCTION__, i);
			continue;
		}

		addOneEntry_NATLB_rule_dynamic_link(&Entry, dstIP, mode);
	}
	return 0;
}

static int set_dhcp_NATLB_chain(int fh, char *nat_pre_name, char *nat_post_name)
{
	char buff[64];
	
	// iptables -t nat -F portfwPreNatLB_ptm0_0
	snprintf(buff, 64, "\tiptables -t nat -F %s\n", nat_pre_name);
	WRITE_DHCPC_FILE(fh, buff);	
	// iptables -t nat -D PREROUTING -j portfwPreNatLB_ptm0_0
	snprintf(buff, 64, "\tiptables -t nat -D PREROUTING -j %s\n", nat_pre_name);	
	WRITE_DHCPC_FILE(fh, buff);
	// iptables -t nat -X portfwPreNatLB_ptm0_0
	snprintf(buff, 64, "\tiptables -t nat -X %s\n", nat_pre_name);
	WRITE_DHCPC_FILE(fh, buff);
	
	// iptables -t nat -F portfwPostNatLB_ptm0_0	
	snprintf(buff, 64, "\tiptables -t nat -F %s\n", nat_post_name);
	WRITE_DHCPC_FILE(fh, buff);
	// iptables -t nat -D POSTROUTING -j portfwPostNatLB_ptm0_0	
	snprintf(buff, 64, "\tiptables -t nat -D POSTROUTING -j %s\n", nat_post_name);	
	WRITE_DHCPC_FILE(fh, buff);
	// iptables -t nat -X portfwPostNatLB_ptm0_0	
	snprintf(buff, 64, "\tiptables -t nat -X %s\n", nat_post_name);
	WRITE_DHCPC_FILE(fh, buff);
	
	// iptables -t nat -N portfwPreNatLB_ptm0_0	
	snprintf(buff, 64, "\tiptables -t nat -N %s\n", nat_pre_name);
	WRITE_DHCPC_FILE(fh, buff);	
	// iptables -t nat -A PREROUTING -j portfwPreNatLB_ptm0_0	
	snprintf(buff, 64, "\tiptables -t nat -A PREROUTING -j %s\n", nat_pre_name);	
	WRITE_DHCPC_FILE(fh, buff);
	
	// iptables -t nat -N portfwPostNatLB_ptm0_0	
	snprintf(buff, 64, "\tiptables -t nat -N %s\n", nat_post_name);
	WRITE_DHCPC_FILE(fh, buff);
	// iptables -t nat -A POSTROUTING -j portfwPostNatLB_ptm0_0	
	snprintf(buff, 64, "\tiptables -t nat -A POSTROUTING -j %s\n", nat_post_name);	
	WRITE_DHCPC_FILE(fh, buff);
	return 0;
}

int set_dhcp_NATLB(int fh, MIB_CE_ATM_VC_Tp pEntry)
{
	int vInt, i, total;
	unsigned char value[32];
#ifdef PORT_FORWARD_GENERAL
	MIB_CE_PORT_FW_T PfEntry;
#endif
#ifdef VIRTUAL_SERVER_SUPPORT
	MIB_VIRTUAL_SVR_T VsEntry;
#endif
	char ifname[10];
	char nat_pre_name[30], nat_post_name[30];
	
	ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));	
	snprintf(nat_pre_name, 30, "%s_%s", PORT_FW_PRE_NAT_LB, ifname);
	snprintf(nat_post_name, 30, "%s_%s", PORT_FW_POST_NAT_LB, ifname);		

#ifdef PORT_FORWARD_GENERAL
	// (1) set NATLB rule for portfw
	vInt = 0;
	if (mib_get_s(MIB_PORT_FW_ENABLE, (void *)value, sizeof(value)) != 0)
		vInt = (int)(*(unsigned char *)value);

	if (vInt == 1)
	{
		int negate=0, hasRemote=0;
		char * proto = 0;
		char intPort[32], extPort[32];		
		
		total = mib_chain_total(MIB_PORT_FW_TBL);
		
		if (total > 0)
			set_dhcp_NATLB_chain(fh, nat_pre_name, nat_post_name);
			
		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_PORT_FW_TBL, i, (void *)&PfEntry))
				return -1;

			if (PfEntry.ifIndex != DUMMY_IFINDEX && PfEntry.ifIndex != pEntry->ifIndex)
			{
				continue;
			}

			portfw_NATLB_dynamic( &PfEntry, ifname, "$ip", nat_pre_name, nat_post_name, fh);
		}
	}
#endif
	
#ifdef DMZ
	// (2) set NATLB rule for dmz
	snprintf(nat_pre_name, 30, "%s_%s", IPTABLE_DMZ_PRE_NAT_LB, ifname);
	snprintf(nat_post_name, 30, "%s_%s", IPTABLE_DMZ_POST_NAT_LB, ifname);
	
	vInt = 0;
	if (mib_get_s(MIB_DMZ_ENABLE, (void *)value, sizeof(value)) != 0)
		vInt = (int)(*(unsigned char *)value);
		
	if (mib_get_s(MIB_DMZ_IP, (void *)value, sizeof(value)) != 0)
	{
		if (vInt == 1)
		{
			set_dhcp_NATLB_chain(fh, nat_pre_name, nat_post_name);	
			set_dmz_natLB_dynamic(ifname, "$ip", nat_pre_name, nat_post_name, fh);
		}
	}	
#endif

#ifdef VIRTUAL_SERVER_SUPPORT
	// (3) set NATLB rule for virtual server
	snprintf(nat_pre_name, 30, "%s_%s", VIR_SER_PRE_NAT_LB, ifname);
	snprintf(nat_post_name, 30, "%s_%s", VIR_SER_POST_NAT_LB, ifname);

	total = mib_chain_total(MIB_VIRTUAL_SVR_TBL);
	
	if (total > 0)
		set_dhcp_NATLB_chain(fh, nat_pre_name, nat_post_name);
		
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_VIRTUAL_SVR_TBL, i, (void *)&VsEntry))
			return -1;
		
		if(VsEntry.enable == 0)
			continue;

		if(pEntry->ifIndex != VsEntry.ifIndex)
			continue;
			
		virtual_server_NATLB_dynamic( &VsEntry, ifname, "$ip", nat_pre_name, nat_post_name, fh);
	}
#endif

	return 0;
}

static int set_ppp_NATLB_chain(int fd, char *nat_pre_name, char *nat_post_name)
{
	if (fd==0) {
		del_NATLB_dynamic(nat_pre_name, nat_post_name);

		// iptables -t nat -N portfwPreNatLB_ppp0
		va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", nat_pre_name);
		// iptables -t nat -A PREROUTING -j portfwPreNatLB_pppo
		va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", nat_pre_name);
		// iptables -t nat -N portfwPostNatLB_ppp0
		va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", nat_post_name);
		// iptables -t nat -A POSTROUTING -j portfwPostNatLB_ppp0
		va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", nat_post_name);
	}
	else {
		// iptables -t nat -F portfwPreNatLB_ppp0
		dprintf(fd, "iptables -t nat -F %s\n", nat_pre_name);
		// iptables -t nat -D PREROUTING -j portfwPreNatLB_pppo
		dprintf(fd, "iptables -t nat -D PREROUTING -j %s\n", nat_pre_name);
		// iptables -t nat -X portfwPreNatLB_ppp0
		dprintf(fd, "iptables -t nat -X %s\n", nat_pre_name);

		// iptables -t nat -F portfwPostNatLB_ppp0
		dprintf(fd, "iptables -t nat -F %s\n", nat_post_name);
		// iptables -t nat -D POSTROUTING -j portfwPostNatLB_ppp0
		dprintf(fd, "iptables -t nat -D POSTROUTING -j %s\n", nat_post_name);
		// iptables -t nat -X portfwPostNatLB_ppp0
		dprintf(fd, "iptables -t nat -X %s\n", nat_post_name);

		// iptables -t nat -N portfwPreNatLB_ppp0
		dprintf(fd, "iptables -t nat -N %s\n", nat_pre_name);
		// iptables -t nat -A PREROUTING -j portfwPreNatLB_pppo
		dprintf(fd, "iptables -t nat -A PREROUTING -j %s\n", nat_pre_name);
		// iptables -t nat -N portfwPostNatLB_ppp0
		dprintf(fd, "iptables -t nat -N %s\n", nat_post_name);
		// iptables -t nat -A POSTROUTING -j portfwPostNatLB_ppp0
		dprintf(fd, "iptables -t nat -A POSTROUTING -j %s\n", nat_post_name);
	}
	return 0;
}

int set_ppp_NATLB(int fd, char *ifname, char *myip)
{
	int vInt, i, total;
	unsigned char value[32];
	MIB_CE_PORT_FW_T PfEntry;
#ifdef VIRTUAL_SERVER_SUPPORT
	MIB_VIRTUAL_SVR_T VsEntry;
	char server_ifname[30];
#endif
	char nat_pre_name[30], nat_post_name[30], pf_ifname[16] = {0};
	
	snprintf(nat_pre_name, 30, "%s_%s", PORT_FW_PRE_NAT_LB, ifname);
	snprintf(nat_post_name, 30, "%s_%s", PORT_FW_POST_NAT_LB, ifname);	
	
#ifdef PORT_FORWARD_GENERAL
	// (1) set NATLB rule for portfw
	vInt = 0;
	if (mib_get_s(MIB_PORT_FW_ENABLE, (void *)value, sizeof(value)) != 0)
		vInt = (int)(*(unsigned char *)value);

	if (vInt == 1)
	{
		int negate=0, hasRemote=0;
		char * proto = 0;
		char intPort[32], extPort[32];

		total = mib_chain_total(MIB_PORT_FW_TBL);
		
		if ( total > 0)
			set_ppp_NATLB_chain(fd, nat_pre_name, nat_post_name);
			
		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_PORT_FW_TBL, i, (void *)&PfEntry))
				return -1;

			if (PfEntry.ifIndex != DUMMY_IFINDEX)
			{
				ifGetName(PfEntry.ifIndex, pf_ifname, sizeof(pf_ifname));
				if (strcmp(ifname, pf_ifname) != 0)
					continue;
			}

			portfw_NATLB_dynamic( &PfEntry, ifname, myip, nat_pre_name, nat_post_name, fd);
		}
	}
#endif

#ifdef DMZ
	// (2) set NATLB rule for dmz
	snprintf(nat_pre_name, 30, "%s_%s", IPTABLE_DMZ_PRE_NAT_LB, ifname);
	snprintf(nat_post_name, 30, "%s_%s", IPTABLE_DMZ_POST_NAT_LB, ifname);

	vInt = 0;
	if (mib_get_s(MIB_DMZ_ENABLE, (void *)value, sizeof(value)) != 0)
		vInt = (int)(*(unsigned char *)value);
		
	if (mib_get_s(MIB_DMZ_IP, (void *)value, sizeof(value)) != 0)
	{
		if (vInt == 1)
		{
			set_ppp_NATLB_chain(fd, nat_pre_name, nat_post_name);
			set_dmz_natLB_dynamic(ifname, myip, nat_pre_name, nat_post_name, fd);
		}
	}	
#endif

#ifdef VIRTUAL_SERVER_SUPPORT
	// (3) set NATLB rule for virtual server
	snprintf(nat_pre_name, 30, "%s_%s", VIR_SER_PRE_NAT_LB, ifname);
	snprintf(nat_post_name, 30, "%s_%s", VIR_SER_POST_NAT_LB, ifname);

	total = mib_chain_total(MIB_VIRTUAL_SVR_TBL);
	
	if (total > 0)
		set_ppp_NATLB_chain(fd, nat_pre_name, nat_post_name);
		
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_VIRTUAL_SVR_TBL, i, (void *)&VsEntry))
			return -1;
		
		if(VsEntry.enable == 0)
			continue;
			
		virtual_server_NATLB_dynamic( &VsEntry, ifname, myip, nat_pre_name, nat_post_name, fd);
	}
#endif
	
	return 1;
}

int del_ppp_NATLB(FILE *fp, char *ifname)
{
#ifdef PORT_FORWARD_GENERAL
	fprintf(fp, "iptables -t nat -F %s_%s\n", PORT_FW_PRE_NAT_LB, ifname);
	fprintf(fp, "iptables -t nat -F %s_%s\n", PORT_FW_POST_NAT_LB, ifname);
#endif
#ifdef DMZ
	fprintf(fp, "iptables -t nat -F %s_%s\n", IPTABLE_DMZ_PRE_NAT_LB, ifname);
	fprintf(fp, "iptables -t nat -F %s_%s\n", IPTABLE_DMZ_POST_NAT_LB, ifname);
#endif
#ifdef VIRTUAL_SERVER_SUPPORT
	fprintf(fp, "iptables -t nat -F %s_%s\n", VIR_SER_PRE_NAT_LB, ifname);
	fprintf(fp, "iptables -t nat -F %s_%s\n", VIR_SER_POST_NAT_LB, ifname);
#endif
	return 0;
}

#ifdef DMZ
int iptable_dmz_natLB(int del, char *dstIP)
{	
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	unsigned char ipStr[20], subnetStr[20];
	unsigned long mask, mbit, ip, subnet;
	char ipaddr[32], lan_ipaddr[32];	
	char *act;
	
	if(del) act = (char *)FW_DEL;
	else act = (char *)FW_ADD;
	
	// (1) Get LAN IP and mask
	mib_get_s(MIB_ADSL_LAN_IP, (void *)&ip, sizeof(ip));
	strncpy(ipStr, inet_ntoa(*((struct in_addr *)&ip)), 16);
	ipStr[15] = '\0';

	mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)&mask, sizeof(mask));
	subnet = (ip&mask);	
	strncpy(subnetStr, inet_ntoa(*((struct in_addr *)&subnet)), 16);
	subnetStr[15] = '\0';

	mbit=0;
	mask = htonl(mask);
	while (1) {
		if (mask&0x80000000) {
			mbit++;
			mask <<= 1;
		}
		else
			break;
	}
	snprintf(lan_ipaddr, sizeof(lan_ipaddr), "%s/%lu", subnetStr, mbit);
	
	//(2) Set rule
	#ifdef CONFIG_NETFILTER_XT_TARGET_NOTRACK
	#ifdef NAT_LOOPBACK
	/* NAT-LOOPBACK needs local-in conntrack, add rule in raw table to
		bypass notrack target below. */
	if (!del) { /* add rule */
		va_cmd(IPTABLES, 8, 1, (char *)ARG_T, "raw",
			(char *)FW_ADD, (char *)DMZ_RAW_TRACK_NAT_LB,
			"-s", dstIP,
			"-j", (char *)FW_ACCEPT);
	}
	#endif
	#endif
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) 
	{
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) 
		{
			printf("setStaticNATLoopBack: cannot get ATM_VC_TBL(ch=%d) entry\n", i);
			return -1;
		}
		
		// Set rule for static link only
		if ( !Entry.enable || ((DHCP_T)Entry.ipDhcp != DHCP_DISABLED)
			|| ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_BRIDGE)
			|| ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_PPPOE)
			|| ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_PPPOA)
			|| (!Entry.napt)
			)
			continue;
			
		snprintf(ipaddr, sizeof(ipaddr), "%s", inet_ntoa(*((struct in_addr *)Entry.ipAddr)));
		
#ifdef DMZ
		// (2.1) Set prerouting rule
		// iptables -t nat -A dmzPreNatLB -s 192.168.1.0/24 -d 192.168.8.1 -j DNAT --to-destination 192.168.1.20		
		va_cmd(IPTABLES, 12, 1, "-t", "nat",
			(char *)act,	(char *)IPTABLE_DMZ_PRE_NAT_LB,
			"-s", lan_ipaddr,
			"-d", ipaddr,			
			"-j",	"DNAT", "--to-destination", dstIP);
			
		// (2.2) Set postrouting rule
		//iptables -t nat -A dmzPostNatLB -s 192.168.1.0/24 -d 192.168.1.20 -j SNAT --to-source 192.168.8.1		
		va_cmd(IPTABLES, 10, 1, "-t", "nat",
			(char *)act,	(char *)IPTABLE_DMZ_POST_NAT_LB,
			"-s", lan_ipaddr,
			"-d", dstIP,
			"-j", "MASQUERADE");
#endif
	}

	return 0;
}
#endif

#ifdef PORT_FORWARD_GENERAL
#if defined(CONFIG_RTK_DEV_AP)
int iptable_fw_natLB( int del, unsigned int ifIndex, const char *proto, const char *extPort, const char *intPort, const char *dstIP, MIB_CE_PORT_FW_T *pentry)
#else
int iptable_fw_natLB( int del, unsigned int ifIndex, const char *proto, const char *extPort, const char *intPort, const char *dstIP)
#endif
{	
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	unsigned char ipStr[20], subnetStr[20];
	unsigned long mask, mbit, ip, subnet;
	char *act;
	char ipaddr[32], lan_ipaddr[32], dstIP2[32], *pStr;
	char ifname[16] = {0}, nat_pre_name[30] = {0}, nat_post_name[30] = {0};
	int flags = 0;
	struct in_addr inAddr;

	if(del) act = (char *)FW_DEL;
	else act = (char *)FW_ADD;

	// (1) Get LAN IP and mask
	mib_get_s(MIB_ADSL_LAN_IP, (void *)&ip, sizeof(ip));
	strncpy(ipStr, inet_ntoa(*((struct in_addr *)&ip)), 16);
	ipStr[15] = '\0';

	mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)&mask, sizeof(mask));
	subnet = (ip&mask);
	strncpy(subnetStr, inet_ntoa(*((struct in_addr *)&subnet)), 16);
	subnetStr[15] = '\0';

	mbit=0;
	mask = htonl(mask);
	while (1) {
		if (mask&0x80000000) {
			mbit++;
			mask <<= 1;
		}
		else
			break;
	}
	snprintf(lan_ipaddr, sizeof(lan_ipaddr), "%s/%lu", subnetStr, mbit);
	dstIP2[sizeof(dstIP2)-1]= '\0';
	strncpy(dstIP2, dstIP, sizeof(dstIP2)-1);
	pStr = strstr(dstIP2, ":");
	if (pStr != NULL)
		pStr[0] = '\0';

	//(2) Set rule
	#ifdef CONFIG_NETFILTER_XT_TARGET_NOTRACK
	#ifdef NAT_LOOPBACK
	/* NAT-LOOPBACK needs local-in conntrack, add rule in raw table to
		bypass notrack target below. */
	if (!del) { /* add rule */
		if (intPort) {
			va_cmd(IPTABLES, 12, 1, (char *)ARG_T, "raw",
				(char *)FW_ADD, (char *)PF_RAW_TRACK_NAT_LB,
				"-s", dstIP2,
				"-p", (char *)proto,
				(char *)FW_SPORT, intPort,
				"-j", (char *)FW_ACCEPT);
		}
		else {
			va_cmd(IPTABLES, 10, 1, (char *)ARG_T, "raw",
				(char *)FW_ADD, (char *)PF_RAW_TRACK_NAT_LB,
				"-s", dstIP2,
				"-p", (char *)proto,
				"-j", (char *)FW_ACCEPT);
		}
	}
	#endif
	#endif
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++)
	{
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			printf("%s: cannot get ATM_VC_TBL.%d entry\n", __FUNCTION__, i);
			continue;
		}

		if ((!Entry.enable) || (!Entry.napt) || (Entry.cmode==CHANNEL_MODE_BRIDGE))
			continue;

		if (ifIndex != DUMMY_IFINDEX)
		{
			if (Entry.ifIndex != ifIndex)
				continue;
		}

		// Set rule for dynamic link
		if ((DHCP_T)Entry.ipDhcp == DHCP_CLIENT || Entry.cmode == CHANNEL_MODE_PPPOE || Entry.cmode == CHANNEL_MODE_PPPOA)
		{
			ifGetName(Entry.ifIndex, ifname, sizeof(ifname));
			if (getInFlags(ifname, &flags) == 1 && getInAddr(ifname, IP_ADDR, &inAddr) == 1)
			{
				memset(ipaddr, 0 , sizeof(ipaddr));
				inet_ntop(AF_INET, (void *)&inAddr, ipaddr, 16);
			}
			else
				continue;

			snprintf(nat_pre_name, sizeof(nat_pre_name), "%s_%s", PORT_FW_PRE_NAT_LB, ifname);
			snprintf(nat_post_name, sizeof(nat_post_name), "%s_%s", PORT_FW_POST_NAT_LB, ifname);

			// (2.1) Set postrouting rule
			if (intPort)
			{
				va_cmd(IPTABLES, 14, 1, "-t", "nat",
					(char *)act, nat_post_name,
					"-s", lan_ipaddr,
					"-d", dstIP2,
					"-p", (char *)proto,
					(char *)FW_DPORT, intPort,
					"-j", "MASQUERADE");
			}
			else
			{
				va_cmd(IPTABLES, 12, 1, "-t", "nat",
					(char *)act, nat_post_name,
					"-s", lan_ipaddr,
					"-d", dstIP2,
					"-p", (char *)proto,
					"-j", "MASQUERADE");
			}
		}
		// Set rule for static link
		else if ((DHCP_T)Entry.ipDhcp == DHCP_DISABLED)
		{
			memset(ipaddr, 0 , sizeof(ipaddr));
			snprintf(ipaddr, sizeof(ipaddr), "%s", inet_ntoa(*((struct in_addr *)Entry.ipAddr)));
			snprintf(nat_pre_name, sizeof(nat_pre_name), "%s", PORT_FW_PRE_NAT_LB);
			snprintf(nat_post_name, sizeof(nat_post_name), "%s", PORT_FW_POST_NAT_LB);

			// (2.1) Set postrouting rule
			//iptables -t nat -A portfwPostNatLB -s 192.168.1.0/24 -d 192.168.1.20 -p TCP --dport 21 -j SNAT --to-source 192.168.8.1
			if (intPort) {
				va_cmd(IPTABLES, 14, 1, "-t", "nat",
					(char *)act, nat_post_name,
					"-s", lan_ipaddr,
					"-d", dstIP2,
					"-p", (char *)proto,
					(char *)FW_DPORT, intPort,
					"-j", "MASQUERADE");
			} else {
				va_cmd(IPTABLES, 12, 1, "-t", "nat",
					(char *)act, nat_post_name,
					"-s", lan_ipaddr,
					"-d", dstIP2,
					"-p", (char *)proto,
					"-j", "MASQUERADE");
			}

#if defined(CONFIG_RTK_DEV_AP)
			if(pentry && (pentry->fromPort <= 1723 &&  pentry->toPort >= 1723) && !strncmp(proto, "TCP", 3)){
				va_cmd(IPTABLES, 14, 1, "-t", "nat",
				(char *)act, nat_post_name,
				"-s", lan_ipaddr,
				"-d", dstIP2,
				"-p", "47",
				"-j", "SNAT", "--to-source", ipaddr);
			}
#endif
		}
		else
			continue;

		// (2.2) Set prerouting rule
		// iptables -t nat -A portfwPreNatLB -s 192.168.1.0/24 -d 192.168.8.1 -p TCP --dport 2121 -j DNAT --to-destination 192.168.1.20:21
		if (extPort) {
			va_cmd(IPTABLES, 16, 1, "-t", "nat",
				(char *)act, nat_pre_name,
				"-s", lan_ipaddr,
				"-d", ipaddr,
				"-p", (char *)proto,
				(char *)FW_DPORT, extPort, "-j",
				"DNAT", "--to-destination", dstIP);
		} else {
			va_cmd(IPTABLES, 14, 1, "-t", "nat",
				(char *)act, nat_pre_name,
				"-s", lan_ipaddr,
				"-d", ipaddr,
				"-p", (char *)proto,
				"-j", "DNAT", "--to-destination", dstIP);
		}

		if (ifIndex != DUMMY_IFINDEX)
			break;
	}
	return 0;
}
#endif

#ifdef VIRTUAL_SERVER_SUPPORT
int iptable_virtual_server_natLB( int del, unsigned int ifIndex, const char *proto, const char *extPort, const char *intPort, const char *dstIP)
{	
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	unsigned char ipStr[20], subnetStr[20];
	unsigned long mask, mbit, ip, subnet;
	char *act;
	char ipaddr[32], lan_ipaddr[32], dstIP2[32], *pStr;
	
	if(del) act = (char *)FW_DEL;
	else act = (char *)FW_ADD;
	
	// (1) Get LAN IP and mask
	mib_get_s(MIB_ADSL_LAN_IP, (void *)&ip, sizeof(ip));
	strncpy(ipStr, inet_ntoa(*((struct in_addr *)&ip)), 16);
	ipStr[15] = '\0';

	mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)&mask, sizeof(mask));
	subnet = (ip&mask);	
	strncpy(subnetStr, inet_ntoa(*((struct in_addr *)&subnet)), 16);
	subnetStr[15] = '\0';

	mbit=0;
	mask = htonl(mask);
	while (1) {
		if (mask&0x80000000) {
			mbit++;
			mask <<= 1;
		}
		else
			break;
	}
	snprintf(lan_ipaddr, sizeof(lan_ipaddr), "%s/%lu", subnetStr, mbit);
	dstIP2[sizeof(dstIP2)-1]= '\0';
	strncpy(dstIP2, dstIP, sizeof(dstIP2)-1);
	pStr = strstr(dstIP2, ":");
	if (pStr != NULL)
		pStr[0] = '\0';
	
	//(2) Set rule
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) 
	{
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) 
		{
			printf("setStaticNATLoopBack: cannot get ATM_VC_TBL(ch=%d) entry\n", i);
			return 0;
		}
		
		// Set rule for static link only
		if ( !Entry.enable || ((DHCP_T)Entry.ipDhcp != DHCP_DISABLED)
			|| ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_BRIDGE)
			|| ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_PPPOE)
			|| ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_PPPOA)
			|| (!Entry.napt)
			)
			continue;			
		
		if (ifIndex != DUMMY_IFINDEX)
		{
			if (Entry.ifIndex != ifIndex)
				continue;
		}
		snprintf(ipaddr, sizeof(ipaddr), "%s", inet_ntoa(*((struct in_addr *)Entry.ipAddr)));		
		
		// (2.1) Set prerouting rule
		// iptables -t nat -A portfwPreNatLB -s 192.168.1.0/24 -d 192.168.8.1 -p TCP --dport 2121 -j DNAT --to-destination 192.168.1.20:21			
		if (extPort) {
			va_cmd(IPTABLES, 16, 1, "-t", "nat",
				(char *)act,	(char *)VIR_SER_PRE_NAT_LB,
				"-s", lan_ipaddr,
				"-d", ipaddr,
				"-p", (char *)proto,
				(char *)FW_DPORT, extPort, "-j",
				"DNAT", "--to-destination", dstIP);
		} else {
			va_cmd(IPTABLES, 14, 1, "-t", "nat",
				(char *)act,	(char *)VIR_SER_PRE_NAT_LB,
				"-s", lan_ipaddr,
				"-d", ipaddr,
				"-p", (char *)proto,
				"-j", "DNAT", "--to-destination", dstIP);
		}

		// (2.2) Set postrouting rule
		//iptables -t nat -A portfwPostNatLB -s 192.168.1.0/24 -d 192.168.1.20 -p TCP --dport 21 -j SNAT --to-source 192.168.8.1 
		if (intPort) {
			va_cmd(IPTABLES, 16, 1, "-t", "nat",
				(char *)act,	(char *)VIR_SER_POST_NAT_LB,
				"-s", lan_ipaddr,
				"-d", dstIP2,
				"-p", (char *)proto,
				(char *)FW_DPORT, intPort, "-j",
				"SNAT", "--to-source", ipaddr);
		} else {
			va_cmd(IPTABLES, 14, 1, "-t", "nat",
				(char *)act,	(char *)VIR_SER_POST_NAT_LB,
				"-s", lan_ipaddr,
				"-d", dstIP2,
				"-p", (char *)proto,
				"-j", "SNAT", "--to-source", ipaddr);
		}
		
		if (ifIndex != DUMMY_IFINDEX)			
			break;		
	}
	return 0;
}
#endif
#endif
#endif

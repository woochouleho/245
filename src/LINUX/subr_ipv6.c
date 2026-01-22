/*
 *      IPv6 basic routines
 *
 */

#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "utility.h"
#include "subr_net.h"
#include "ipv6_info.h"
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include "fc_api.h"
#include <linux/version.h>
#include "sockmark_define.h"

#ifdef CONFIG_IPV6
const char IP6TABLES[] = "/bin/ip6tables";
const char FW_IPV6FILTER[] = "ipv6filter";
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
const char FW_IPV6_CE_ROUTER_FILTER[] = "ipv6_CE_Router_filter";
const char FW_IPV6_CE_ROUTER_MANGLE[] = "ipv6_CE_Router_mangle";
#endif
#ifdef CONFIG_USER_RTK_IPV6_SIMPLE_SECURITY
const char FW_IPV6_SIMPLE_SECURITY_FILTER[] = "ipv6_simple_security_filter";
#endif
const char ARG_ICMPV6[] = "ICMPV6";
#if defined(DHCPV6_ISC_DHCP_4XX) || defined(CONFIG_USER_RADVD)
const char RADVD_CONF[] = "/var/radvd.conf";
const char RADVD_CONF_TMP[] = "/var/radvd.conf.tmp";
const char RADVD_PID[] = "/var/run/radvd.pid";
#endif
const char PROC_IP6FORWARD[] = "/proc/sys/net/ipv6/conf/all/forwarding";
const char PROC_MC6FORWARD[] = "/proc/sys/net/ipv6/conf/all/mc_forwarding";

#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
const char DHCPD6S_CONF[] = "/var/dhcp6s.conf";
const char DHCPD6S_CONF_TMP[] = "/var/dhcp6s.conf.tmp";
const char DHCPD6S_PID[] = "/var/run/dhcp6s.pid";
const int DHCPD6S_LAN_CONTROL_PORT[] = {8081,8082,8083,8084};
#ifdef WLAN_8_SSID_SUPPORT
const int DHCPD6S_WLAN_CONTROL_PORT[] = {8085,8086,8087,8089,8090,8091,8092,8093,
                                         8094,8095,8096,8097,8098,8099,8100,8101};
#else
const int DHCPD6S_WLAN_CONTROL_PORT[] = {8085,8086,8087,8089,8090,8091,8092,8093,8094,8095};
#endif
#endif

static unsigned char ipv6_unspec[IP6_ADDR_LEN] = {0};
extern unsigned int rtk_wan_get_max_wan_mtu(void);

//Copy from kernel linux-4.4.x/net/ipv6/addrconf_core.c
#ifndef IPV6_ADDR_SCOPE_TYPE
#define IPV6_ADDR_SCOPE_TYPE(scope)	((scope) << 16)
#endif
static inline unsigned int ipv6_addr_scope2type(unsigned int scope)
{
	switch (scope) {
	case IPV6_ADDR_SCOPE_NODELOCAL:
		return (IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_NODELOCAL) |
			IPV6_ADDR_LOOPBACK);
	case IPV6_ADDR_SCOPE_LINKLOCAL:
		return (IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_LINKLOCAL) |
			IPV6_ADDR_LINKLOCAL);
	case IPV6_ADDR_SCOPE_SITELOCAL:
		return (IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_SITELOCAL) |
			IPV6_ADDR_SITELOCAL);
	}
	return IPV6_ADDR_SCOPE_TYPE(scope);
}

int ipv6_addr_type(const struct in6_addr *addr)
{
	__be32 st;

	st = addr->s6_addr32[0];

	/* Consider all addresses with the first three bits different of
	   000 and 111 as unicasts.
	 */
	if ((st & htonl(0xE0000000)) != htonl(0x00000000) &&
	    (st & htonl(0xE0000000)) != htonl(0xE0000000))
		return (IPV6_ADDR_UNICAST |
			IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));

	if ((st & htonl(0xFF000000)) == htonl(0xFF000000)) {
		/* multicast */
		/* addr-select 3.1 */
		return (IPV6_ADDR_MULTICAST |
			ipv6_addr_scope2type(IPV6_ADDR_MC_SCOPE(addr)));
	}

	if ((st & htonl(0xFFC00000)) == htonl(0xFE800000))
		return (IPV6_ADDR_LINKLOCAL | IPV6_ADDR_UNICAST |
			IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_LINKLOCAL));		/* addr-select 3.1 */
	if ((st & htonl(0xFFC00000)) == htonl(0xFEC00000))
		return (IPV6_ADDR_SITELOCAL | IPV6_ADDR_UNICAST |
			IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_SITELOCAL));		/* addr-select 3.1 */
	if ((st & htonl(0xFE000000)) == htonl(0xFC000000))
		return (IPV6_ADDR_UNICAST |
			IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));			/* RFC 4193 */

	if ((addr->s6_addr32[0] | addr->s6_addr32[1]) == 0) {
		if (addr->s6_addr32[2] == 0) {
			if (addr->s6_addr32[3] == 0)
				return IPV6_ADDR_ANY;

			if (addr->s6_addr32[3] == htonl(0x00000001))
				return (IPV6_ADDR_LOOPBACK | IPV6_ADDR_UNICAST |
					IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_LINKLOCAL));	/* addr-select 3.4 */

			return (IPV6_ADDR_COMPATv4 | IPV6_ADDR_UNICAST |
				IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));	/* addr-select 3.3 */
		}

		if (addr->s6_addr32[2] == htonl(0x0000ffff))
			return (IPV6_ADDR_MAPPED |
				IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));	/* addr-select 3.3 */
	}

	return (IPV6_ADDR_UNICAST |
		IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));	/* addr-select 3.4 */
}
int ipv6_addr_needs_scope_id(int type)
{
	return type & IPV6_ADDR_LINKLOCAL ||
	       (type & IPV6_ADDR_MULTICAST &&
		(type & (IPV6_ADDR_LOOPBACK|IPV6_ADDR_LINKLOCAL)));
}

int ipv6_prefix_equal(const struct in6_addr *addr1,    const struct in6_addr *addr2,  unsigned int prefixlen)
{
	const __be32 *a1 = addr1->s6_addr32;
	const __be32 *a2 = addr2->s6_addr32;
	unsigned int pdw, pbi;

	/* check complete u32 in prefix */
	pdw = prefixlen >> 5;
	if (pdw && memcmp(a1, a2, pdw << 2))
		return 0;

	/* check incomplete u32 in prefix */
	pbi = prefixlen & 0x1f;
	if (pbi && ((a1[pdw] ^ a2[pdw]) & htonl((0xffffffff) << (32 - pbi))))
		return 0;

	return 1;
}
//Copy from kernel end

/* compare_ipv6_addr: Compare ipv6 IP
 * Return 0 if ipA == ipB, -1 if ipA < ipB and 1 if ipA > ipB
 */
int compare_ipv6_addr(struct in6_addr *ipA, struct in6_addr *ipB)
{
	int i = 0;
	for(i = 0; i < 16; ++i) // Don't use magic number, here just for example
	{
		if (ipA->s6_addr[i] < ipB->s6_addr[i])
			return -1;
		else if (ipA->s6_addr[i] > ipB->s6_addr[i])
			return 1;
	}
	return 0;
}

/*
 *	convert ipv6 prefix to ip6 address
 *	prefix:  ipv6 prefix
 *	ip6:	target ipv6 address
 *	plen:	prefix length
 *   mode:  0   2001:db8::/64  => 2001:db8::/64
            1   2001:db8::/64  => 2001:db8::ffff:ffff:ffff:ffff/64
 *	Return: 1 Success; 0 fail
 */
int prefixtoIp6(void *prefix, int plen, void *ip6, int mode)
{
	struct in6_addr *src, *dst;
	int current_idx=0, i=0,k=0, m=0;
	unsigned char mask=0, tmask=0;

	if (plen <=1 || plen > 128)
		return 0;

	src = (struct in6_addr *)prefix;
	dst = (struct in6_addr *)ip6;
	*dst = in6addr_any;
	k = plen/8;
	m= plen%8;
	for (current_idx=0; current_idx<k; current_idx++)
		dst->s6_addr[current_idx] = src->s6_addr[current_idx];

	if (m ==0)
	{
		if (mode ==0)
		{	
			for (;current_idx<16;current_idx++)
				dst->s6_addr[current_idx] = 0;
		} 
		else
		{
			for (;current_idx<16;current_idx++)
				dst->s6_addr[current_idx] = 0xff;
		}
	}
	else
	{
		if (mode ==0)
		{
			mask = 0;
			tmask = 0x80;
			for (i=0; i<m; i++) {
				mask |= tmask;
				tmask = tmask>>1;
			}
			dst->s6_addr[current_idx] = src->s6_addr[current_idx] & mask;
			current_idx++;
			for (;current_idx<16;current_idx++)
				dst->s6_addr[current_idx] = 0;
		}
		else
		{
			mask = 0;
			tmask = 0x01;
			for (i=0; i<(8-m); i++) {
				mask |= tmask;
				tmask = tmask<<1;
			}
			dst->s6_addr[current_idx] = src->s6_addr[current_idx] | mask;
			current_idx++;
			for (;current_idx<16;current_idx++)
				dst->s6_addr[current_idx] = 0xff;
		}
	}
	
	return 1;
}

/*
 *	convert ipv6 address to ipv6 prefix
 *	ip6:	ipv6 address
 *	plen:	prefix length
 *	prefix: target ipv6 prefix
 *	Return: 1 Success; 0 fail
 */
int ip6toPrefix(void *ip6, int plen, void *prefix)
{
	struct in6_addr *src, *dst;
	int i, k, m;
	unsigned char mask=0, tmask=0;

	if (plen <=1 || plen > 128)
		return 0;

	src = (struct in6_addr *)ip6;
	dst = (struct in6_addr *)prefix;
	*dst = in6addr_any;
	k = plen/8;
	if((m=plen%8)) k++;
	for (i=0; i<k; i++)
		dst->s6_addr[i] = src->s6_addr[i];
	if (m)
	{
		mask = 0;
		tmask = 0x80;
		for (i=0; i<m; i++) {
			mask |= tmask;
			tmask = tmask>>1;
		}
		dst->s6_addr[k-1] &= mask;
	}

	return 1;
}

/*
 *	convert Ethernet address to modified EUI-64
 *	src(6 octects):	mac address
 *	dst(8 octects):	target MEUI-64
 *	Return: 1 Success; 0 fail
 */
int mac_meui64(char *src, char *dst)
{
	int i;

	memset(dst, 0, 8);
	memcpy(dst, src, 3);
	memcpy(dst + 5, src + 3, 3);
	dst[3] = 0xff;
	dst[4] = 0xfe;
	dst[0] ^= 0x02;
	return 1;
}

/*
 *	Get IPv6 addresses of interface.
 *	addr_scope: net/ipv6.h
 *		IPV6_ADDR_ANY		0x0000U
 *		IPV6_ADDR_UNICAST      	0x0001U
 *		IPV6_ADDR_MULTICAST    	0x0002U
 *		IPV6_ADDR_LOOPBACK	0x0010U
 *		IPV6_ADDR_LINKLOCAL	0x0020U
 *		IPV6_ADDR_SITELOCAL	0x0040U
 *		IPV6_ADDR_COMPATv4	0x0080U
 *		IPV6_ADDR_SCOPE_MASK	0x00f0U
 *		IPV6_ADDR_MAPPED	0x1000U
 *		IPV6_ADDR_RESERVED	0x2000U
 *	addr_lst: address list
 *	num: max number of address
 *	Return: number of addresses
 */
int getifip6(const char *ifname, unsigned int addr_scope, struct ipv6_ifaddr *addr_lst, int num)
{
	FILE *fp;
	struct in6_addr		addr;
	unsigned int		ifindex = 0;
	unsigned int		prefixlen, scope, flags;
	unsigned char		scope_value;
	char			devname[IFNAMSIZ];
	char 			buf[1024];
	int			k = 0;

	memset(addr_lst, 0, sizeof(struct ipv6_ifaddr)*num);
	/* Get link local addresses from /proc/net/if_inet6 */
	fp = fopen("/proc/net/if_inet6", "r");
	if (!fp)
		return 0;

	scope_value = addr_scope;
	if (addr_scope == IPV6_ADDR_UNICAST)
		scope_value = IPV6_ADDR_ANY;
	/* Format "fe80000000000000029027fffe24bbab 02 0a 20 80     eth0" */
	while (fgets(buf, sizeof(buf), fp))
	{
		//printf("buf= %s\n", buf);
		if (21 != sscanf( buf,
			"%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx %x %x %x %x %8s",
			&addr.s6_addr[ 0], &addr.s6_addr[ 1], &addr.s6_addr[ 2], &addr.s6_addr[ 3],
			&addr.s6_addr[ 4], &addr.s6_addr[ 5], &addr.s6_addr[ 6], &addr.s6_addr[ 7],
			&addr.s6_addr[ 8], &addr.s6_addr[ 9], &addr.s6_addr[10], &addr.s6_addr[11],
			&addr.s6_addr[12], &addr.s6_addr[13], &addr.s6_addr[14], &addr.s6_addr[15],
			&ifindex, &prefixlen, &scope, &flags, devname))
		{
			printf("/proc/net/if_inet6 has a broken line, ignoring");
			continue;
		}
		if (!strcmp(ifname, devname) && (addr_scope == IPV6_ADDR_ANY || scope_value == scope)) {
			if (k>=num)
				break;
			else {
				addr_lst[k].valid = 1;
				memcpy(&addr_lst[k].addr, &addr, sizeof(struct in6_addr));
				addr_lst[k].prefix_len = prefixlen;
				addr_lst[k].flags = flags;
				addr_lst[k].scope = scope;
			}
			k++;
		}
		//inet_ntop(PF_INET6, &addr, buf, 1024);
		//printf("IPv6: %s scope=0x%x\n", buf, scope);
	}

	fclose(fp);
	return k;
}

int rtk_ipv6_get_route(struct ipv6_route** route_list)
{
	FILE *fp;
	struct ipv6_route* info;
	struct in6_addr		target, source, gateway;
	unsigned int		prefixlen, source_prefix_len, metric, flags, refcount, userconut;
	char			devname[IFNAMSIZ]={0};
	char 			buf[1024]={0};
	int			k = 0;
	int count=0;

	system("cat /proc/net/ipv6_route > /tmp/ipv6_route");
	system("cat /tmp/ipv6_route | wc -l > /tmp/ipv6_route_num");

	//get num of total routes
	if ((fp = fopen("/tmp/ipv6_route_num", "r")) == NULL){
		*route_list=NULL;
		return 0;
	}
	if(fscanf(fp, "%d", &count) != 1){
		count = 0;
	}
	fclose(fp);

	//printf("%s:%d ipv6_route_num %d\n", __FUNCTION__, __LINE__, count);

	if(count == 0){
		return 0;
	}
	info = (struct ipv6_route*)malloc(sizeof(struct ipv6_route)*(count));
	memset(info, 0, sizeof(struct ipv6_route)*count);
	/* Get link local addresses from /proc/net/if_inet6 */
	fp = fopen("/tmp/ipv6_route", "r");
	if (!fp)
		return 0;

	/* Format "20040000000000000000000000000000 30 20020000000000000000000000000000 40 fe800000000000000a0027fffe887340 00000200 00000001 00000000 00000003   nas0_0" */
	while (fgets(buf, sizeof(buf), fp))
	{
		//printf("buf= %s\n", buf);
		if (53 != sscanf( buf,
			"%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx %x %02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx %x %02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx %08x %*08x %*08x %08x %s",
			&target.s6_addr[ 0], &target.s6_addr[ 1], &target.s6_addr[ 2], &target.s6_addr[ 3],
			&target.s6_addr[ 4], &target.s6_addr[ 5], &target.s6_addr[ 6], &target.s6_addr[ 7],
			&target.s6_addr[ 8], &target.s6_addr[ 9], &target.s6_addr[10], &target.s6_addr[11],
			&target.s6_addr[12], &target.s6_addr[13], &target.s6_addr[14], &target.s6_addr[15],&prefixlen, 
			&source.s6_addr[ 0], &source.s6_addr[ 1], &source.s6_addr[ 2], &source.s6_addr[ 3],
			&source.s6_addr[ 4], &source.s6_addr[ 5], &source.s6_addr[ 6], &source.s6_addr[ 7],
			&source.s6_addr[ 8], &source.s6_addr[ 9], &source.s6_addr[10], &source.s6_addr[11],
			&source.s6_addr[12], &source.s6_addr[13], &source.s6_addr[14], &source.s6_addr[15], &source_prefix_len,
			&gateway.s6_addr[ 0], &gateway.s6_addr[ 1], &gateway.s6_addr[ 2], &gateway.s6_addr[ 3],
			&gateway.s6_addr[ 4], &gateway.s6_addr[ 5], &gateway.s6_addr[ 6], &gateway.s6_addr[ 7],
			&gateway.s6_addr[ 8], &gateway.s6_addr[ 9], &gateway.s6_addr[10], &gateway.s6_addr[11],
			&gateway.s6_addr[12], &gateway.s6_addr[13], &gateway.s6_addr[14], &gateway.s6_addr[15], 
			&metric,
			&flags, devname))
		{
			printf("/proc/net/ipv6_route has a broken line, ignoring\n");
			continue;
		}
		
		if (k>=count)
			break;
		else {
			memcpy(&info[k].target, &target, sizeof(struct in6_addr));
			info[k].prefix_len = prefixlen;
			memcpy(&info[k].source, &source, sizeof(struct in6_addr));
			info[k].source_prefix_len = source_prefix_len;
			memcpy(&info[k].gateway, &gateway, sizeof(struct in6_addr));
			info[k].metric = metric;
			info[k].flags = flags;
			snprintf(info[k].devname, sizeof(info[k].devname), "%s", devname);
			//printf("IPv6: metric 0x%x, flags 0x%x, devname=%s\n", info[k].metric, info[k].flags, info[k].devname);
		}
		//printf("IPv6: metric 0x%x, flags 0x%x, devname=%s\n", info[k].metric, info[k].flags, info[k].devname);
		k++;
		//inet_ntop(PF_INET6, &addr, buf, 1024);
	}

	fclose(fp);
	//printf("%s:%d ipv6_route_num %d\n", __FUNCTION__, __LINE__, count);
	*route_list = info;
	
	return k;
}


int is_InternetWan_IPv6_single_stack(void){
	MIB_CE_ATM_VC_T entry;
	int entryNum = 0, i = 0;
	int result = 0;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);

	for (i = 0; i < entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, &entry)) {
			printf("get MIB chain error\n");
			return -1;
		}

		if((entry.applicationtype & X_CT_SRV_INTERNET) == 0){
			continue;
		}

		if(entry.IpProtocol & IPVER_IPV4){
			// not ipv6 single stack, return 0 directly.
			return 0; 
		}

		if((entry.IpProtocol == IPVER_IPV6) && (entry.dgw == IPVER_IPV6)){
			result = 1;
			//do not break, need continue.
		}
			
	}

	return result;
}

#ifdef WLAN_SUPPORT
/* is_wlan_inf_binding_v6_route_wan()
 *     return 1: means this wlan inf binding on one routing IPv6 WAN.
 *     return 0: means no bind on any routing IPv6 WAN.
 */
int is_wlan_inf_binding_v6_route_wan(char *infname)
{
	unsigned int entryNum = 0, i = 0, j = 0;
	MIB_CE_ATM_VC_T Entry;

	if (!infname)
		return -1;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			printf("Get chain record error!\n");
			return -1;
		}

		if (Entry.enable == 0)
			continue;

		if(Entry.cmode == CHANNEL_MODE_BRIDGE)
			continue;

		if (!(Entry.IpProtocol & IPVER_IPV6))
			continue;

		for (j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP3; ++j)
		{
			if (BIT_IS_SET(Entry.itfGroup, j))
			{
				if (!strcmp((char *)wlan[j - PMAP_WLAN0], infname))
					return 1;
			}
		}

	}

	return 0;
}
#endif

#ifdef CONFIG_IPV6_SIT_6RD
void make6RD_prefix(MIB_CE_ATM_VC_Tp pEntry, unsigned char *ip6buf, int ip6buf_size)
{
	unsigned int ipAddr=0;
	unsigned char B1,B2,B3,B4;
	struct in6_addr ip6Addr;
	unsigned char devAddr[MAC_ADDR_LEN];
	unsigned char meui64[8];
	int i;
	int v4addr_offset = pEntry->SixrdPrefixLen/8;

	memcpy(&ip6Addr, pEntry->SixrdPrefix, sizeof(ip6Addr));
	if (pEntry->SixrdIPv4MaskLen >=0 && pEntry->SixrdIPv4MaskLen < 32) ipAddr =((pEntry->ipAddr[0]<<24) |(pEntry->ipAddr[1]<<16) |(pEntry->ipAddr[2]<<8) |(pEntry->ipAddr[3])) << (pEntry->SixrdIPv4MaskLen);

	if(( pEntry->SixrdPrefixLen % 8 ) ==0 )
	{
		B1 = ipAddr>>24;
		B2 = (ipAddr<<8)>>24;
		B3 = (ipAddr<<16)>>24;
		B4 = (ipAddr<<24)>>24;
		ip6Addr.s6_addr[v4addr_offset] = B1;
		ip6Addr.s6_addr[v4addr_offset+1] = B2;
		ip6Addr.s6_addr[v4addr_offset+2] = B3;
		ip6Addr.s6_addr[v4addr_offset+3] = B4;

	}
	else
	{   //SixrdPrefixLen is not multiple of 8, will be more complicated to handle

		int prefix_lastbits= pEntry->SixrdPrefixLen % 8;                        //Ex:41%8=1 , last bit is 1
		unsigned int v4IP_B1B2B2B4_shifted  =  ipAddr <<(8-prefix_lastbits);    //            shift left for (8-1) = 7 bits

		B1 = v4IP_B1B2B2B4_shifted>>24;
		B2 = (v4IP_B1B2B2B4_shifted<<8)>>24;
		B3 = (v4IP_B1B2B2B4_shifted<<16)>>24;
		B4 = (v4IP_B1B2B2B4_shifted<<24)>>24;

		ip6Addr.s6_addr[v4addr_offset] =  (ip6Addr.s6_addr[v4addr_offset]>>(8-prefix_lastbits)) <<(8-prefix_lastbits);
		                                  //This stip is for EX: 6rd prefix is 2001:1001:F000 but prefix lenght is 34
		                                  //Then the bit 33,34 will be recorded. others will be shifted out.
		ip6Addr.s6_addr[v4addr_offset] |= ipAddr>>(24+prefix_lastbits);
		ip6Addr.s6_addr[v4addr_offset+1] = B1;
		ip6Addr.s6_addr[v4addr_offset+2] = B2;
		ip6Addr.s6_addr[v4addr_offset+3] = B3;
		ip6Addr.s6_addr[v4addr_offset+4] = B4;
	}

	mib_get_s(MIB_ELAN_MAC_ADDR, (void *)devAddr, sizeof(devAddr));
	mac_meui64(devAddr, meui64);
	for (i=0; i<8; i++)
		ip6Addr.s6_addr[i+8] = meui64[i];

	inet_ntop(PF_INET6, &ip6Addr, ip6buf, ip6buf_size);
}
#endif

#if defined(CONFIG_USER_NDPPD)
int ndp_proxy_setup_conf()
{
	FILE *fp=NULL;
	char ifname[IFNAMSIZ] ={0};
	int ret = 0;
	int idx = 0, total = mib_chain_total(MIB_ATM_VC_TBL);
	MIB_CE_ATM_VC_T Entry;
	MIB_CE_ATM_VC_Tp pEntry = NULL;
	char tmpBuf[256] = {0};
	int wanPrefixLen = 0;
	char wanPrefix[INET6_ADDRSTRLEN] = {0},lanPrefix[INET6_ADDRSTRLEN] = {0};
	unsigned int pd_prefix_wan = 0, flags = 0;
	unsigned char leasefile[64]= {0};
	DLG_INFO_T dlgInfo = {0};
	struct ipv6_ifaddr ip6_addr = {0}, ip6_prefix = {0};
	PREFIX_V6_INFO_T lan_prefixInfo, wan_prefixInfo;
	unsigned char  dhcpv6_mode = 0, dhcpv6_type =0,radvd_mode=0;

	//ifGetName(ifindex, ifname, IFNAMSIZ);
	if (!mib_get_s(MIB_DHCPV6_MODE, (void *)&dhcpv6_mode, sizeof(dhcpv6_mode)))
	{
		AUG_PRT("Get DHCPV6 server Mode failed\n");
	}
	if (!mib_get_s(MIB_DHCPV6S_TYPE, (void *)&dhcpv6_type, sizeof(dhcpv6_type))){
		AUG_PRT("Get DHCPV6 server Type failed\n");
	}
	if ( !mib_get_s(MIB_V6_RADVD_PREFIX_MODE, (void *)&radvd_mode, sizeof(radvd_mode)))
	{
		AUG_PRT("Error!! Get Radvd IPv6 Prefix Mode fail!");
	}

	fp = fopen(NDPPD_CONF_PATH, "w");
	if(fp == NULL) {
		printf("Error open file %s\n",NDPPD_CONF_PATH);
		return -1;
	}

	fprintf(fp, "route-ttl 30000\n");
	fprintf(fp, "address-ttl 30000\n");

	//WAN Interface filter 
	for(idx = 0 ; idx < total ; idx++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, idx, (void *)&Entry))
			continue;
		pEntry = &Entry;

		if (!pEntry->enable)
			continue;

		ifGetName(pEntry->ifIndex,ifname,sizeof(ifname));

		if(getInFlags(ifname, &flags) == 0)
			continue;

		if(!((flags&IFF_UP) && (flags&IFF_RUNNING)))
			continue;

		if((pEntry->IpProtocol & IPVER_IPV6) && pEntry->ndp_proxy == 1)
		{
			wanPrefixLen = 0;
			wanPrefix[0] = 0;

			if(pEntry->Ipv6DhcpRequest & 0x2)
			{
				snprintf(leasefile, sizeof(leasefile)-1, "%s.%s", PATH_DHCLIENT6_LEASE, ifname);
				if(getLeasesInfo(leasefile,&dlgInfo))
				{
					wanPrefixLen = dlgInfo.prefixLen;
					memcpy(&ip6_addr.addr, dlgInfo.prefixIP, sizeof(ip6_addr.addr));
				}
			}
#if !defined(CONFIG_RTK_DEV_AP)
			else if(get_prefixv6_info_by_wan(&wan_prefixInfo,pEntry->ifIndex)==0){
				wanPrefixLen = wan_prefixInfo.prefixLen;
				memcpy(&ip6_addr.addr, wan_prefixInfo.prefixIP, sizeof(ip6_addr.addr));
			}
#else
			else if(getifip6(ifname, IPV6_ADDR_UNICAST, &ip6_addr, 1) > 0)
			{
				wanPrefixLen = ip6_addr.prefix_len;
			}
#endif

			if(wanPrefixLen !=64)
				wanPrefixLen = 64;

			if(ip6toPrefix(&ip6_addr.addr, wanPrefixLen, &ip6_prefix.addr))
			{
				inet_ntop(AF_INET6, &ip6_prefix.addr, wanPrefix, sizeof(wanPrefix));
			}
			//printf("[%s %d]===> WAN: %s/%d\n", __func__,__LINE__,wanPrefix, wanPrefixLen);
			if(!(wanPrefixLen > 0 && wanPrefix[0]))
				continue;

			fprintf(fp, "proxy %s {\n ", ifname);
			fprintf(fp, "	router yes\n");
			fprintf(fp, "	timeout 600\n");
			fprintf(fp, "	autowire no\n");
			fprintf(fp, "	keepalive yes\n");
			fprintf(fp, "	retries 3\n");
			fprintf(fp, "	promiscuous no\n");
			fprintf(fp, "	allmulti no\n");
			fprintf(fp, "	ttl 30000\n");
			fprintf(fp, "	rule %s/%d {\n", wanPrefix, wanPrefixLen);
			fprintf(fp, "		iface %s\n", LANIF);

			//lan interface filter by PD
			memset(&lan_prefixInfo,0,sizeof(lan_prefixInfo));
			//bind port need use binding wan's PD
			if (pEntry->itfGroup)
			{
				if(get_prefixv6_info_by_wan(&lan_prefixInfo,pEntry->ifIndex) ==0)
				{
					inet_ntop(AF_INET6, lan_prefixInfo.prefixIP, lanPrefix, INET6_ADDRSTRLEN);
					fprintf(fp, "		prefix %s/%d\n", lanPrefix, lan_prefixInfo.prefixLen);
				}
				//if can not get wan PD, make binding port to get lan static prefix if user set static prefix(e.g. NAT/NAPT6 networking)
				else
				{
					//get manual prefix from dhcpv6
					memset(&lan_prefixInfo,0,sizeof(lan_prefixInfo));
					if ((dhcpv6_mode == DHCP_LAN_SERVER) && (dhcpv6_type == DHCPV6S_TYPE_STATIC)
						&& (get_dhcpv6_prefixv6_info(&lan_prefixInfo) ==0))
					{
						inet_ntop(AF_INET6, lan_prefixInfo.prefixIP, lanPrefix, INET6_ADDRSTRLEN);
						fprintf(fp, "		prefix %s/%d\n", lanPrefix, lan_prefixInfo.prefixLen);
					}
					//get manual prefix from radvd
					memset(&lan_prefixInfo,0,sizeof(lan_prefixInfo));
					if ((radvd_mode== RADVD_MODE_MANUAL)&&(get_radvd_prefixv6_info(&lan_prefixInfo)==0))
					{
						inet_ntop(AF_INET6, lan_prefixInfo.prefixIP, lanPrefix, INET6_ADDRSTRLEN);
						fprintf(fp, "		prefix %s/%d\n", lanPrefix, lan_prefixInfo.prefixLen);
					}
				}
			}
			else
			{
				//get default prefix from br0
				memset(&lan_prefixInfo,0,sizeof(lan_prefixInfo));
				if(get_prefixv6_info(&lan_prefixInfo) ==0)
				{
					inet_ntop(AF_INET6, lan_prefixInfo.prefixIP, lanPrefix, INET6_ADDRSTRLEN);
					fprintf(fp, "		prefix %s/%d\n", lanPrefix, lan_prefixInfo.prefixLen);
				}
				//get manual prefix from dhcpv6
				memset(&lan_prefixInfo,0,sizeof(lan_prefixInfo));
				if ((dhcpv6_mode == DHCP_LAN_SERVER) && (dhcpv6_type == DHCPV6S_TYPE_STATIC)
					&& (get_dhcpv6_prefixv6_info(&lan_prefixInfo) ==0))
				{
					inet_ntop(AF_INET6, lan_prefixInfo.prefixIP, lanPrefix, INET6_ADDRSTRLEN);
					fprintf(fp, "		prefix %s/%d\n", lanPrefix, lan_prefixInfo.prefixLen);
				}
				//get manual prefix from radvd
				memset(&lan_prefixInfo,0,sizeof(lan_prefixInfo));
				if ((radvd_mode== RADVD_MODE_MANUAL)&&(get_radvd_prefixv6_info(&lan_prefixInfo)==0))
				{
					inet_ntop(AF_INET6, lan_prefixInfo.prefixIP, lanPrefix, INET6_ADDRSTRLEN);
					fprintf(fp, "		prefix %s/%d\n", lanPrefix, lan_prefixInfo.prefixLen);
				}
			}
			fprintf(fp, "	}\n}\n");
		}
	}

out:
	fclose(fp);
	return ret;
}

void ndp_proxy_clean(void)
{
	int pid;
	
	//kill_by_pidfile((const char *)NDPPD_PID_PATH, SIGTERM);	//SIGTERM to clear old setting
	/* Because ndppd process will be triggerred very fast, we use killall -9 instead of kill by pidfile.
	 * Its pidfile is created too slow. */
	va_cmd("/bin/killall", 2, 1, "-9", "ndppd");
}
void ndp_proxy_start(void)
{
	int idx = 0, total = mib_chain_total(MIB_ATM_VC_TBL);
	MIB_CE_ATM_VC_T Entry;
	MIB_CE_ATM_VC_Tp pEntry = NULL;	
	char ifname[IFNAMSIZ] ={0};
	unsigned int flags = 0;
	int found_proxy = 0;

	for(idx = 0 ; idx < total ; idx++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, idx, (void *)&Entry))
			continue;
		pEntry = &Entry;

		if (!pEntry->enable)
			continue;

		ifGetName(pEntry->ifIndex,ifname,sizeof(ifname));

		if(getInFlags(ifname, &flags) == 0)
			continue;

		if(!((flags&IFF_UP) && (flags&IFF_RUNNING)))
			continue;

		if((pEntry->IpProtocol & IPVER_IPV6) && pEntry->ndp_proxy == 1)
		{
			found_proxy++;
		}
	}

	if(found_proxy > 0){
		va_cmd(NDPPD_CMD, 5, 1, "-d", "-c", NDPPD_CONF_PATH, "-p", NDPPD_PID_PATH);
	}
}
#endif
/*
 * FUNCTION:GET_NETWORK_DNSv6
 *
 * INPUT:
 *	dns1:
 *		Fisrt DNSv6 server to set.
 *	dns2:
 *		Second DNSv6 server to set.
 *
 * RETURN:
 * 	the number of DNS server got.
 *	-1 -- Input error
 *  -2 -- INETERNET WAN not found
 *  -3 -- resolv file not found
 *  -4 -- other
 */
int get_network_dnsv6(char *dns1, char *dns2)
{
	MIB_CE_ATM_VC_T entry;
	int total, i;
	struct in6_addr sa;	// used to check IP
	int got = 0;
	char buf[256] = {0};
	char ifname[IFNAMSIZ] = {0};

	if(dns1 == NULL || dns2 == NULL)
		return -1;

	/* Find INTERNET WAN */
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, &entry) == 0)
			continue;

		if (entry.IpProtocol == IPVER_IPV4)
			continue;

		if(entry.enable && entry.applicationtype & X_CT_SRV_INTERNET && entry.cmode != CHANNEL_MODE_BRIDGE)
		{
			ifGetName(entry.ifIndex, ifname, sizeof(ifname));
			break;
		}
	}

	/* INTERNET WAN not found */
	if(i == total)
		return -2;

	/* Get config */
	switch(entry.dnsv6Mode)
	{
	case REQUEST_DNS_NONE: //manual
	case DNS_SET_BY_API: // set by API
		{
			if(inet_ntop(AF_INET6, &entry.Ipv6Dns1, buf, sizeof(buf)) != NULL)
			{
				strcpy(dns1, buf);
				got++;
			}

			if(inet_ntop(AF_INET6, &entry.Ipv6Dns2, buf, sizeof(buf)) != NULL)
			{
				strcpy(dns2, buf);
				got++;
			}
		}
		break;
	case REQUEST_DNS: //auto
		{
			FILE *fresolv = NULL;
			char *p;

			snprintf(buf, 64, "%s.%s", (char *)DNS6_RESOLV, ifname);

			fresolv = fopen(buf, "r");
			if(fresolv == NULL)
				return -3;

			while(fgets(buf,sizeof(buf), fresolv)!=NULL)
			{
				// Get DNS servers
				p = strchr(buf, '\n');
				if (p)
				{
					if(got == 0)
					{
						memcpy(dns1, buf, p-buf);
						dns1[p-buf] = '\0';
						got++;
					}
					else if(got == 1)
					{
						memcpy(dns2, buf, p-buf);
						dns2[p-buf] = '\0';
						got++;
					}
				}

				if(got == 2)
					break;
			}
			fclose(fresolv);
		}
		break;
	default:
		return -4;
	}

	return got;
}
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
int get_bridge_info(MIB_CE_ATM_VC_T *entry)
{
	int total = mib_chain_total(MIB_ATM_VC_TBL);
	int i;
	struct in_addr inAddr;
	int flags = 0;
	char ifname[IFNAMSIZ];

	memset(entry, 0, sizeof(MIB_CE_ATM_VC_T));
	for(i = 0 ; i < total ; i++)
	{   
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)entry))
			continue;
		if((entry->applicationtype & X_CT_SRV_INTERNET))
		{   
			if(ifGetName( entry->ifIndex, ifname, IFNAMSIZ) == 0)
				continue;

			if(!strncmp(ifname, "nas0_1", 6)) /* bridge */
				return 1;
		}
	}
	return 0;
}

static int set_global_ipv6_addr(char *prefix, int prefix_len)
{
	unsigned char macaddr[6] = {0};
	char ipv6Address[256];
	char cmd[256];

	mib_get(MIB_ELAN_MAC_ADDR, (void *)macaddr);

	macaddr[0] ^= 0x02;
	macaddr[0] |= 0x02;

	snprintf(ipv6Address, 256, "%s%02x%02x:%02xff:fe%02x:%02x%02x",
			prefix, macaddr[0], macaddr[1], macaddr[2],
			macaddr[3], macaddr[4], macaddr[5]);
	printf("ipv6Address:%s\n", ipv6Address);
	snprintf(cmd, 256, "ip addr add %s/%d dev br0", ipv6Address, prefix_len);
	printf("cmd:%s\n", cmd);
	system(cmd);
	return 0;
}
int startBR_IP_for_V6(char *br_inf)
{
	char vChar = 0;
	char cmd[128] = {0};
	int prefix_len = 0;
	char prefix[256] = {0,};
	int len = 0;
	
#if defined(CONFIG_USER_RTK_RAMONITOR) 
	char br_raconf_file[64] = {0};
	FILE *ra_file = NULL;
	char *pos = NULL;
	char content[256] = {0};
	int file_count = 0;
	int m_flag = -1, o_flag=-1;
#endif
	
	mib_get_s(MIB_V6_IPV6_ENABLE, (void *)&vChar, sizeof(vChar));
	if (vChar == 0)
		return -1;
	
#if defined(CONFIG_USER_RTK_RAMONITOR) 
	snprintf(br_raconf_file, sizeof(br_raconf_file), "%s_%s", (char *)RA_INFO_FILE, br_inf);
	/* Because ramonitord will notify us if RA_INFO_FILE is exist for this WAN, we just retry open file for 2 times. */
	while (file_count <= 1)
	{
		if ((ra_file = fopen(br_raconf_file, "r")) != NULL)
		{
			/* First get M O flag info. */
			while (fgets(content, sizeof(content), ra_file) != NULL)
			{
				if ((pos = strstr(content, RA_INFO_M_BIT_KEY)) != NULL)
				{
					m_flag = atoi(pos + strlen(RA_INFO_M_BIT_KEY));
					printf("%s: %s's M bit=%d\n", __func__, br_inf, m_flag);
				}
				if ((pos = strstr(content, RA_INFO_O_BIT_KEY)) != NULL)
				{
					o_flag = atoi(pos + strlen(RA_INFO_O_BIT_KEY));
					printf("%s: %s's O bit=%d\n", __func__, br_inf, o_flag);
				}
				if ((pos = strstr(content, RA_INFO_PREFIX_KEY)) != NULL)
				{
					memset(prefix, 0, 256);
					strcpy(prefix, pos + strlen(RA_INFO_PREFIX_KEY));
					len = strlen(prefix);
					if(len > 0 && len < 256) 
						prefix[len - 1] = '\0';
					printf("%s: %s's prefix=(%s)\n", __func__, br_inf, prefix);
				}
				if ((pos = strstr(content, RA_INFO_PREFIX_LEN_KEY)) != NULL)
				{
					prefix_len = atoi(pos + strlen(RA_INFO_PREFIX_LEN_KEY));
					printf("%s: %s's prefix LEN =%d\n", __func__, br_inf, prefix_len);
				}
			}
			fclose(ra_file);
			break;
		}
		else
		{
			printf("%s: open %s file error!!!\n", __func__, br_raconf_file);
		}
		file_count++;
		printf("%s: Wait inf %s RA packet count = %d\n", __func__, br_inf, file_count);
		usleep(500000);
	}
	if(m_flag ==1)
	{
		snprintf(cmd, 64, "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/autoconf", br_inf);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef DHCPV6_ISC_DHCP_4XX
		del_dhcpc6_br((char*)BRIF);
		//run dhclient on br0
		do_dhcpc6_br((char*)BRIF, STATEFUL_DHCP_MODE);
#endif
	}
	else if ((m_flag == 0) && (o_flag ==1))
	{		
		snprintf(cmd, 64, "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/autoconf", br_inf);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef DHCPV6_ISC_DHCP_4XX
		del_dhcpc6_br((char*)BRIF);
		//run dhclient on br0
		do_dhcpc6_br((char*)BRIF, STATELESS_DHCP_MODE);
#endif
		set_global_ipv6_addr(prefix, prefix_len);
	}
	else 
	{
#ifdef DHCPV6_ISC_DHCP_4XX
		del_dhcpc6_br((char*)BRIF);
#endif
		snprintf(cmd, 64, "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/autoconf", br_inf);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		set_global_ipv6_addr(prefix, prefix_len);

	}
#endif
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
	reload_dnsrelay(br_inf);
#endif

	return 0;
}
#endif
#ifdef CONFIG_IPV6
int stop6rd_6in4_6to4(MIB_CE_ATM_VC_Tp pEntry)
{
	if(pEntry->IpProtocol != IPVER_IPV4) return 0;

	switch(pEntry->v6TunnelType) {
#ifdef CONFIG_IPV6_SIT_6RD
		case V6TUNNEL_6RD:
			va_cmd("/usr/bin/ip", 4, 1, "link", "set", TUN6RD_IF, "down");
			va_cmd("/usr/bin/ip", 3, 1, "tunnel", "del", TUN6RD_IF);
			break;
#endif
#ifdef CONFIG_IPV6_SIT
		case V6TUNNEL_6IN4:
			va_cmd("/usr/bin/ip", 4, 1, "link", "set", TUN6IN4_IF, "down");
			va_cmd("/usr/bin/ip", 3, 1, "tunnel", "del", TUN6IN4_IF);
			break;
		case V6TUNNEL_6TO4:
			va_cmd("/usr/bin/ip", 4, 1, "link", "set", TUN6TO4_IF, "down");
			va_cmd("/usr/bin/ip", 3, 1, "tunnel", "del", TUN6TO4_IF);
			break;
#endif
		default:
			break;
	}
	return 0;
}

int setupIPv6DNS(MIB_CE_ATM_VC_Tp pEntry)
{
	char inf[IFNAMSIZ];
	char file[64] = {0};
	FILE *infdns = NULL;

	// Get interface name
	ifGetName(pEntry->ifIndex, inf, sizeof(inf));
	if (pEntry->IpProtocol == IPVER_IPV4 && pEntry->v6TunnelType == V6TUNNEL_6RD)
		strncpy(inf, TUN6RD_IF, sizeof(inf));
	if (pEntry->IpProtocol == IPVER_IPV4 && pEntry->v6TunnelType == V6TUNNEL_6IN4)
		strncpy(inf, TUN6IN4_IF, sizeof(inf));
	if (pEntry->IpProtocol == IPVER_IPV4 && pEntry->v6TunnelType == V6TUNNEL_6TO4)
		strncpy(inf, TUN6TO4_IF, sizeof(inf));

	// Write DNS servers to /var/resolv6.conf.{interface}
	snprintf(file, 64, "%s.%s", (char *)DNS6_RESOLV, inf);
	infdns=fopen(file,"w");

	if(infdns)
	{
		unsigned char zero_ip[IP6_ADDR_LEN] = {0};
		char dns_addr[48] = {0};

		if(memcmp(zero_ip, pEntry->Ipv6Dns1, IP6_ADDR_LEN) != 0)
		{
			inet_ntop(AF_INET6, pEntry->Ipv6Dns1, dns_addr, 48);
			fprintf(infdns, "%s\n", dns_addr);
			//Alan, fix local out can not access dns server,
			//dns server IPv6 address add to ipv6 route table will cause dns packet cannot be sent.
			//va_cmd(ROUTE, 6, 1, "-A",  "inet6", "add", dns_addr, "dev", inf);
		}
		if(memcmp(zero_ip, pEntry->Ipv6Dns2, IP6_ADDR_LEN) != 0)
		{
			inet_ntop(AF_INET6, pEntry->Ipv6Dns2, dns_addr, 48);
			fprintf(infdns, "%s\n", dns_addr);
			//Alan, fix local out can not access dns server,
			//dns server IPv6 address add to ipv6 route table will cause dns packet cannot be sent.
			//va_cmd(ROUTE, 6, 1, "-A",  "inet6", "add", dns_addr, "dev", inf);
		}

		fclose(infdns);
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
		reload_dnsrelay(inf);
#endif
	}
	return 0;
}
int deo_startIP_for_V6_static(MIB_CE_ATM_VC_Tp pEntry)
{
	unsigned char value[128];
	unsigned char 	Ipv6AddrStr[INET6_ADDRSTRLEN], RemoteIpv6AddrStr[INET6_ADDRSTRLEN];
	unsigned char ipAddrStr[INET_ADDRSTRLEN], remoteIpAddr[INET_ADDRSTRLEN];
	char *argv[20];
	int idx = 0, len;
	char inf[IFNAMSIZ], infTun[10], v6NetRoute[10];
	char vChar = -1;
	char file[64] = {0};
	FILE *infdns = NULL;
	MIB_CE_ATM_VC_T Entry;
	int entry_index = 0;

	mib_get_s(MIB_V6_IPV6_ENABLE, (void *)&vChar, sizeof(vChar));
	if (vChar == 0)
		return -1;

	if ((pEntry->IpProtocol == IPVER_IPV6 || pEntry->IpProtocol == IPVER_IPV4_IPV6)) {
		strcpy(inf, "br0");
		if (pEntry->AddrMode != IPV6_WAN_AUTO_DETECT_MODE)
		{
			setup_disable_ipv6(inf, 1);
		}
		snprintf(value, sizeof(value), "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/forwarding", inf);
		system(value);

		if ((pEntry->AddrMode & IPV6_WAN_STATIC) /* If select Static IPV6 */
		   ){
			snprintf(value, sizeof(value), "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra", inf);
		}
		else
			snprintf(value, sizeof(value), "/bin/echo 2 > /proc/sys/net/ipv6/conf/%s/accept_ra", inf);
		system(value);

		snprintf(value, sizeof(value), "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/autoconf", inf);
		system(value);

		if ((pEntry->AddrMode & IPV6_WAN_STATIC)
		   ){
			snprintf(value, sizeof(value), "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", inf);
			system(value);
		}

		if (pEntry->AddrMode != IPV6_WAN_AUTO_DETECT_MODE)
		{
			setup_disable_ipv6(inf, 0);
		}

		if (pEntry->AddrMode == IPV6_WAN_STATIC /*&& get_net_link_status(inf)*/) {

			unsigned char zeroIpv6[IP6_ADDR_LEN]={0};
			memset(zeroIpv6, 0, sizeof(zeroIpv6));

			inet_ntop(PF_INET6, (struct in6_addr *)pEntry->Ipv6Addr, Ipv6AddrStr, sizeof(Ipv6AddrStr));

			// Add WAN IP for MER
			len = sizeof(Ipv6AddrStr) - strlen(Ipv6AddrStr);
			if(snprintf(Ipv6AddrStr + strlen(Ipv6AddrStr), len, "/%d", pEntry->Ipv6AddrPrefixLen) >= len) {
				printf("warning, string truncated\n");
			}
			va_cmd(IFCONFIG, 3, 1, inf, ARG_ADD, Ipv6AddrStr);

			// Add default gw
			if(memcmp(zeroIpv6, pEntry->RemoteIpv6Addr, sizeof(zeroIpv6)))
			{
				inet_ntop(PF_INET6, (struct in6_addr *)pEntry->RemoteIpv6Addr, RemoteIpv6AddrStr, sizeof(RemoteIpv6AddrStr));
				if(RemoteIpv6AddrStr[0] /*&& isDefaultRouteWan(pEntry)*/)
				{
					snprintf(file, 64, "%s.%s", (char *)MER_GWINFO_V6, inf);
					if((infdns=fopen(file,"w"))){
						fprintf(infdns, "%s", RemoteIpv6AddrStr);
						fclose(infdns);
					}
					// Add default gw
					// route -A inet6 add ::/0 gw 3ffe::0200:00ff:fe00:0100 dev vc0
					va_cmd(ROUTE, 8, 1, FW_ADD, "inet6", ARG_ADD, "::/0", "gw", RemoteIpv6AddrStr, "dev", inf);
				}
			}

#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
			reload_dnsrelay(inf);
#endif
		}
		set_ipv6_default_policy_route(pEntry, 1);
	}

	return 1;
}
int startIP_for_V6(MIB_CE_ATM_VC_Tp pEntry)
{
	unsigned char value[128];
	unsigned char 	Ipv6AddrStr[INET6_ADDRSTRLEN], RemoteIpv6AddrStr[INET6_ADDRSTRLEN];
	unsigned char ipAddrStr[INET_ADDRSTRLEN], remoteIpAddr[INET_ADDRSTRLEN];
	char *argv[20];
	int idx = 0, len;
	char inf[IFNAMSIZ], infTun[10], v6NetRoute[10];
	char vChar = -1;
	char file[64] = {0};
	FILE *infdns = NULL;
	MIB_CE_ATM_VC_T Entry;
	int entry_index = 0;

	mib_get_s(MIB_V6_IPV6_ENABLE, (void *)&vChar, sizeof(vChar));
	if (vChar == 0)
		return -1;

	if ((pEntry->IpProtocol & IPVER_IPV6)
#ifdef CONFIG_IPV6_SIT_6RD
		|| (pEntry->IpProtocol == IPVER_IPV4 && pEntry->v6TunnelType == V6TUNNEL_6RD)
#endif
#ifdef CONFIG_IPV6_SIT
		|| (pEntry->IpProtocol == IPVER_IPV4 && (pEntry->v6TunnelType == V6TUNNEL_6IN4) || (pEntry->v6TunnelType == V6TUNNEL_6TO4))
#endif
		) {

	// Get interface name
	ifGetName(pEntry->ifIndex, inf, sizeof(inf));
    
	//Alan, fix slaac mode can not get IPv6 address, when unplug pon
	//We need to reset disable_ipv6 to trigger RS packet, we set 0 and then set 1 to trigger RS packet
#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
	/* We do this at (systemd link trigger) for IPV6_WAN_AUTO_DETECT_MODE. */
	if (pEntry->AddrMode != IPV6_WAN_AUTO_DETECT_MODE)
#endif
	{
		/* We should apply basic ipv6 proc setting before enable ipv6,
		 * otherwise it will take default ipv6 setting.
		 * Such as, it will get SLAAC ipv6 address for (AddrMode == IPV6_WAN_STATIC) */
		setup_disable_ipv6(inf, 1);
	}
#ifdef CONFIG_KERNEL_2_6_30
	//Alan, Kernel 2.6.30 would not auto-generated IPv6 link-local Address
	//We generate IPv6 link-local Address ourselves
	setLinklocalIPv6Address(inf);
#endif

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	/* add acl rule for mvlan loop detection */
	if(pEntry->cmode!=CHANNEL_MODE_BRIDGE)
		rtk_fc_ipv6_mvlan_loop_detection_acl_add(pEntry);
#endif

	//TR069 service don't need route packet.
	if (pEntry->applicationtype == X_CT_SRV_TR069) {
		snprintf(value, sizeof(value), "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/forwarding", inf);
	}
	else {
		//Note: napt_v6 must enable forwarding to be set
		snprintf(value, sizeof(value), "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/forwarding", inf);
	}
	system(value);

	/* accept_ra set to 2: Overrule forwarding behaviour. Accept Router Advertisements
	 * even if forwarding is enabled. Otherwise set forwarding = 1 will delete default GW
	 * from RA by kernel. */
	if ((pEntry->AddrMode & IPV6_WAN_STATIC) /* If select Static IPV6 */
#ifdef CONFIG_IPV6_SIT_6RD
		|| (pEntry->IpProtocol == IPVER_IPV4 && pEntry->v6TunnelType == V6TUNNEL_6RD)	//6RD WAN do not need RA as default route
#endif
#ifdef CONFIG_IPV6_SIT
		|| (pEntry->IpProtocol == IPVER_IPV4 && (pEntry->v6TunnelType == V6TUNNEL_6IN4 || pEntry->v6TunnelType == V6TUNNEL_6TO4)) //6to4/6in4 WAN do not need RA as default route
#endif
	){
		snprintf(value, sizeof(value), "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra", inf);
	}
	else
		snprintf(value, sizeof(value), "/bin/echo 2 > /proc/sys/net/ipv6/conf/%s/accept_ra", inf);
	system(value);

	if (pEntry->cmode == CHANNEL_MODE_IPOE
		|| pEntry->cmode == CHANNEL_MODE_RT1483
#ifdef CONFIG_IPV6_SIT_6RD
		|| (pEntry->IpProtocol == IPVER_IPV4 && pEntry->v6TunnelType == V6TUNNEL_6RD)
#endif
#ifdef CONFIG_IPV6_SIT
		|| (pEntry->IpProtocol == IPVER_IPV4 && (pEntry->v6TunnelType == V6TUNNEL_6IN4 || pEntry->v6TunnelType == V6TUNNEL_6TO4))
#endif
		){
		if (!pEntry->AddrMode & IPV6_WAN_STATIC){ /* If select Static IPV6 */
			/* If forwarding is enabled, RA are not accepted unless the special
			 * hybrid mode (accept_ra=2) is enabled.
			 */
			snprintf(value, sizeof(value), "/bin/echo 2 > /proc/sys/net/ipv6/conf/%s/accept_ra", inf);
			system(value);
		}

		/*  Enable autoconf when selected autoconf and accept default route in RA only when the WAN belong to default GW , IulianWu */
		if ((pEntry->AddrMode & IPV6_WAN_AUTO)
#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
			|| (pEntry->AddrMode == IPV6_WAN_AUTO_DETECT_MODE) /* IPv6 auto detect Initial state */
#endif
		){ /* If select SLAAC */
			snprintf(value, sizeof(value), "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/autoconf", inf);
		}
		else
			snprintf(value, sizeof(value), "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/autoconf", inf);
		system(value);

		if ((pEntry->AddrMode & IPV6_WAN_STATIC)
#ifdef CONFIG_IPV6_SIT_6RD
			|| (pEntry->IpProtocol == IPVER_IPV4 && pEntry->v6TunnelType == V6TUNNEL_6RD)	//6RD WAN do not need RA as default route
#endif
#ifdef CONFIG_IPV6_SIT
			|| (pEntry->IpProtocol == IPVER_IPV4 && (pEntry->v6TunnelType == V6TUNNEL_6IN4 || pEntry->v6TunnelType == V6TUNNEL_6TO4))	//6to4/6in4 WAN do not need RA as default route
#endif
		){
			snprintf(value, sizeof(value), "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", inf);
			system(value);
		}else{
			//need get gateway information from RA, so for dynamic address case always enable accept_ra_defrtr
			snprintf(value, sizeof(value), "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", inf);
			system(value);
		}
	}

	if ((pEntry->IpProtocol & IPVER_IPV6) && (pEntry->AddrMode != IPV6_WAN_STATIC))
	{
		/* Reset IPv6 global IP for non-Static case to avoid trigger IPv6 global IP
		 * have the same IP case. So we need to reset it at startIP_for_V6().
		 */
		memset(&pEntry->Ipv6Addr[0], 0, sizeof(pEntry->Ipv6Addr));
		pEntry->Ipv6AddrPrefixLen = 0;
		//Save this mib chain entry
		if (getWanEntrybyIfname(inf, &Entry, &entry_index) == 0)
			mib_chain_update(MIB_ATM_VC_TBL, (void *)pEntry, entry_index);
	}

#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
	/* We do this at (systemd link trigger) for IPV6_WAN_AUTO_DETECT_MODE. */
	if (pEntry->AddrMode != IPV6_WAN_AUTO_DETECT_MODE)
#endif
	{
		setup_disable_ipv6(inf, 0);
	}

	if (pEntry->cmode == CHANNEL_MODE_IPOE || pEntry->cmode == CHANNEL_MODE_RT1483
#ifdef CONFIG_IPV6_SIT_6RD
		|| (pEntry->IpProtocol == IPVER_IPV4 && pEntry->v6TunnelType == V6TUNNEL_6RD)
#endif
#ifdef CONFIG_IPV6_SIT
		|| (pEntry->IpProtocol == IPVER_IPV4 && (pEntry->v6TunnelType == V6TUNNEL_6IN4 || pEntry->v6TunnelType == V6TUNNEL_6TO4))
#endif
		){
#ifdef DHCPV6_ISC_DHCP_4XX
		// Start DHCPv6 client
		// dhclient -6 -sf /var/dhclient-script -lf /var/dhclient6-leases -pf /var/run/dhclient6.pid vc0 -d -q -N -P
		if ((pEntry->Ipv6Dhcp == 1)
#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
			&& (pEntry->AddrMode != IPV6_WAN_AUTO_DETECT_MODE)
#endif
			) {
			make_dhcpcv6_conf(pEntry);
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
				/* Use DUID_LL which generated by rtk to request same IA info. */
				rtk_do_isc_dhcpc6_lease_with_duid_ll(pEntry, inf);
#endif
			if ((pEntry->Ipv6DhcpRequest == M_DHCPv6_REQ_NONE) && (pEntry->dnsv6Mode == REQUEST_DNS))
			{
				printf("startIP_for_V6 do DHCPv6 for INFO_REQUEST_DHCP_MODE\n");
				do_dhcpc6(pEntry, INFO_REQUEST_DHCP_MODE);
			}
			else
			{
				printf("startIP_for_V6 do DHCPv6 for USER_DEFINE_DHCP_MODE\n");
				do_dhcpc6(pEntry, USER_DEFINE_DHCP_MODE);
			}
		}
#endif
		
		if (pEntry->AddrMode == IPV6_WAN_STATIC && get_net_link_status(inf)) {
			
			unsigned char zeroIpv6[IP6_ADDR_LEN]={0};
			memset(zeroIpv6, 0, sizeof(zeroIpv6));

			inet_ntop(PF_INET6, (struct in6_addr *)pEntry->Ipv6Addr, Ipv6AddrStr, sizeof(Ipv6AddrStr));
    
			// Add WAN IP for MER
			len = sizeof(Ipv6AddrStr) - strlen(Ipv6AddrStr);
			if(snprintf(Ipv6AddrStr + strlen(Ipv6AddrStr), len, "/%d", pEntry->Ipv6AddrPrefixLen) >= len) {
				printf("warning, string truncated\n");
			}
			va_cmd(IFCONFIG, 3, 1, inf, ARG_ADD, Ipv6AddrStr);
    
			// Add default gw
			if(memcmp(zeroIpv6, pEntry->RemoteIpv6Addr, sizeof(zeroIpv6)))
			{
				inet_ntop(PF_INET6, (struct in6_addr *)pEntry->RemoteIpv6Addr, RemoteIpv6AddrStr, sizeof(RemoteIpv6AddrStr));
				if(RemoteIpv6AddrStr[0] && isDefaultRouteWan(pEntry))
				{
					snprintf(file, 64, "%s.%s", (char *)MER_GWINFO_V6, inf);
					if((infdns=fopen(file,"w"))){
						fprintf(infdns, "%s", RemoteIpv6AddrStr);
						fclose(infdns);
					}
					// Add default gw
					// route -A inet6 add ::/0 gw 3ffe::0200:00ff:fe00:0100 dev vc0
					va_cmd(ROUTE, 8, 1, FW_ADD, "inet6", ARG_ADD, "::/0", "gw", RemoteIpv6AddrStr, "dev", inf);
				}
			}

			setupIPv6DNS(pEntry);
		}
			// Start 6rd for ipv4 static mode
#ifdef CONFIG_IPV6_SIT_6RD
			if ( pEntry->IpProtocol == IPVER_IPV4 && pEntry->v6TunnelType == V6TUNNEL_6RD ) {
				unsigned char SixrdBRv4IP[INET_ADDRSTRLEN];
				unsigned char ip6buf[48];

				//DHCP_CLIENT will setup 6rd tunnel at systemd
				if (pEntry->ipDhcp == (unsigned char) DHCP_DISABLED)
				{
					printf("Start 6rd config for ipv4 static mode\n");

					if( (pEntry->SixrdPrefixLen+(32-pEntry->SixrdIPv4MaskLen)) >64)
					{
						printf("Invalid 6RD setting, PrefixLen and IPv4 address > 64!!! Please check the setting!\n");
						return -1;
					}

					inet_ntop(PF_INET,  (struct in_addr *)pEntry->ipAddr, ipAddrStr, sizeof(ipAddrStr));
					inet_ntop(PF_INET,  (struct in_addr *)pEntry->SixrdBRv4IP, SixrdBRv4IP, sizeof(SixrdBRv4IP));
					inet_ntop(PF_INET6, pEntry->SixrdPrefix, Ipv6AddrStr, sizeof(Ipv6AddrStr));
					//Setup tunnel
					//(1) ip tunnel add tun6rd mode sit local 10.2.2.2 ttl 64
					printf("Add 6rd tunnel for ipv4 static mode\n");
					va_cmd("/usr/bin/ip", 9, 1, "tunnel", "add", TUN6RD_IF, "mode", "sit", "local", ipAddrStr, "ttl", "64");

					//(2) ip tunnel 6rd dev tun6rd 6rd-prefix 2001:db8::/32 6rd-relay_prefix 10.0.0.0/8
					printf("Setup 6rd tunnel for ipv4 static mode\n");
					if(pEntry->SixrdIPv4MaskLen)
					{
						unsigned char IPv4Mask[INET_ADDRSTRLEN];
						unsigned int relay_mask = (*(unsigned int *)(pEntry->SixrdBRv4IP))&(0xffffffff<<(32-pEntry->SixrdIPv4MaskLen));
						char buf[128];

						inet_ntop(PF_INET,  &relay_mask,  IPv4Mask, sizeof(IPv4Mask));
						snprintf(value, sizeof(value), "%s/%d", Ipv6AddrStr, pEntry->SixrdPrefixLen);
						snprintf(buf, sizeof(buf), "%s/%d", IPv4Mask,pEntry->SixrdIPv4MaskLen);
						va_cmd("/usr/bin/ip", 8, 1, "tunnel", "6rd", "dev", TUN6RD_IF, "6rd-prefix", value, "6rd-relay_prefix",buf);
					}
					else
					{
						snprintf(value, sizeof(value), "%s/%d", Ipv6AddrStr, pEntry->SixrdPrefixLen);
						va_cmd("/usr/bin/ip", 6, 1, "tunnel", "6rd", "dev", TUN6RD_IF, "6rd-prefix", value);
					}

					//(3) ip link set tun6rd up
					printf("6rd tunnel up\n");
					va_cmd("/usr/bin/ip", 4, 1, "link", "set", TUN6RD_IF, "up");
					setup_disable_ipv6(TUN6RD_IF,0);
				
					//IP address and Routing
					printf("Setup 6rd Address and routing\n");
					
					//(5) ip -6 addr add 2001:db8:a02:202:EUI64/32 dev tun6rd
					make6RD_prefix(pEntry,ip6buf,sizeof(ip6buf));
					snprintf(value, sizeof(value), "%s/%d", ip6buf,pEntry->SixrdPrefixLen);
					va_cmd("/usr/bin/ip", 6, 1, "-6", "addr", "add",  value, "dev", TUN6RD_IF);

					//(6) ip -6 route add ::/0 via ::10.1.1.1 dev tun6rd
					if (isDefaultRouteWan(pEntry)) {
						snprintf(value, sizeof(value), "::%s", SixrdBRv4IP);
						va_cmd("/usr/bin/ip", 8, 1, "-6", "route", "add", "::/0", "via", value, "dev", TUN6RD_IF);
					}

					//6rd prefix changed, set ipv6 policy route for portmapping interface
					if (pEntry->itfGroup){
						set_ipv6_policy_route(pEntry,1);
					}
					setupIPv6DNS(pEntry);
				}
			}
#endif
#ifdef CONFIG_IPV6_SIT
			if ( pEntry->IpProtocol == IPVER_IPV4 && pEntry->v6TunnelType == V6TUNNEL_6IN4 ) {
				//We only support static 6in4 tunnel
				if (pEntry->ipDhcp == (unsigned char) DHCP_DISABLED)
				{
					AUG_PRT("Start 6in4 config for ipv4 static mode\n");
					inet_ntop(PF_INET,	(struct in_addr *)pEntry->ipAddr, ipAddrStr, sizeof(ipAddrStr));
					inet_ntop(PF_INET,	(struct in_addr *)pEntry->v6TunnelRv4IP, remoteIpAddr, sizeof(remoteIpAddr));
					inet_ntop(PF_INET6, (struct in6_addr *)pEntry->Ipv6Addr, Ipv6AddrStr, sizeof(Ipv6AddrStr));
					inet_ntop(PF_INET6, (struct in6_addr *)pEntry->RemoteIpv6Addr, RemoteIpv6AddrStr, sizeof(RemoteIpv6AddrStr));

					//Setup tunnel
					//(1) ip tunnel add tun6in4 mode sit remote 10.0.0.1 local 10.0.0.2 ttl 255
					va_cmd("/usr/bin/ip", 11, 1, "tunnel", "add", TUN6IN4_IF, "mode", "sit", "remote", remoteIpAddr, "local", ipAddrStr, "ttl", "64");
					//(2) ip link set tun6in4 up
					va_cmd("/usr/bin/ip", 4, 1, "link", "set", TUN6IN4_IF, "up");
					setup_disable_ipv6(TUN6IN4_IF,0);
					
					//(3) ip -6 addr add 2001::2/64 dev tun6in4
					snprintf(value, sizeof(value), "%s/%d", Ipv6AddrStr,pEntry->Ipv6AddrPrefixLen);
					va_cmd("/usr/bin/ip", 6, 1, "-6", "addr", "add",  value, "dev", TUN6IN4_IF);
			
					//(4) ip -6 route add default via 2001::1 dev tun6in4
					if (isDefaultRouteWan(pEntry)) {
						va_cmd("/usr/bin/ip", 8, 1, "-6", "route", "add", "default", "via", RemoteIpv6AddrStr, "dev", TUN6IN4_IF);
					}
			
					//IPV6 static address changed, set ipv6 policy route for portmapping interface
					if (pEntry->itfGroup){
						set_ipv6_policy_route(pEntry,1);
					}
					setupIPv6DNS(pEntry);
				}
			}
			if ( pEntry->IpProtocol == IPVER_IPV4 && pEntry->v6TunnelType == V6TUNNEL_6TO4 ) {
				struct in6_addr targetIp;
				
				//We only support static 6to4 tunnel
				if (pEntry->ipDhcp == (unsigned char) DHCP_DISABLED)
				{
					AUG_PRT("Start 6to4 config for ipv4 static mode\n");
					inet_ntop(PF_INET,	(struct in_addr *)pEntry->ipAddr, ipAddrStr, sizeof(ipAddrStr));
					inet_ntop(PF_INET,	(struct in_addr *)pEntry->v6TunnelRv4IP, remoteIpAddr, sizeof(remoteIpAddr));

					//Setup tunnel
					//(1) ip tunnel add tun6to4 mode sit remote any local 10.0.0.2 ttl 255
					va_cmd("/usr/bin/ip", 11, 1, "tunnel", "add", TUN6TO4_IF, "mode", "sit", "remote", "any", "local", ipAddrStr, "ttl", "64");
					//(2) ip link set tun6to4 up
					va_cmd("/usr/bin/ip", 4, 1, "link", "set", TUN6TO4_IF, "up");
					setup_disable_ipv6(TUN6TO4_IF,0);
					
					//(3) ip -6 addr add 2002:0a00:0002::1/16 dev tun6to4
					memset(&targetIp,0,sizeof(struct in6_addr));
					targetIp.s6_addr16[0] = 0x2002;
					targetIp.s6_addr[2] = pEntry->ipAddr[0];
					targetIp.s6_addr[3] = pEntry->ipAddr[1];
					targetIp.s6_addr[4] = pEntry->ipAddr[2];
					targetIp.s6_addr[5] = pEntry->ipAddr[3];
					inet_ntop(PF_INET6, &targetIp, Ipv6AddrStr, sizeof(Ipv6AddrStr));

					snprintf(value, sizeof(value), "%s1/16", Ipv6AddrStr);
					va_cmd("/usr/bin/ip", 6, 1, "-6", "addr", "add",  value, "dev", TUN6TO4_IF);

					//(4) ip -6 route add default via ::10.0.0.1 dev tun6to4
					if (isDefaultRouteWan(pEntry)) {
						snprintf(value, sizeof(value), "::%s", remoteIpAddr);
						va_cmd("/usr/bin/ip", 8, 1, "-6", "route", "add", "default", "via", value, "dev", TUN6TO4_IF);
					}
			
					//IPV6 static address changed, set ipv6 policy route for portmapping interface
					if (pEntry->itfGroup){
						set_ipv6_policy_route(pEntry,1);
					}
					setupIPv6DNS(pEntry);
				}
			}
#endif
		}
		set_ipv6_default_policy_route(pEntry, 1);
	}
	
	return 1;
}

int startPPP_for_V6(MIB_CE_ATM_VC_Tp pEntry)
{

	char dev[IFNAMSIZ] = {0}, phy_dev[IFNAMSIZ] = {0};
	char cmdBuf[128] = {0};
	char ipv6Enable;
	MIB_CE_ATM_VC_T Entry;
	int entry_index = 0;

	if(!pEntry){
		fprintf(stderr, "[%s] null ATM_VC_TBL\n", __FUNCTION__);
		return -1;
	}

	ifGetName(pEntry->ifIndex, dev, sizeof(dev));
	ifGetName(PHY_INTF(pEntry->ifIndex), phy_dev, sizeof(phy_dev));

	/*	Configure Initial IPv6 setting.
	 *	For ppp daemon, ppp interface isn't created yet. So, it move to ppp-init script */
#ifdef CONFIG_USER_SPPPD
	mib_get(MIB_V6_IPV6_ENABLE, (void *)&ipv6Enable);
	if (!(pEntry->IpProtocol & IPVER_IPV6) || !ipv6Enable) 
	{
		setup_disable_ipv6(dev, 1);
		return 0; /* Because maybe now WAN mode is IPv4 only, we can't return -1 (-1 means error)-1  */
	}

	if (pEntry->cmode == CHANNEL_MODE_PPPOE) {
		// disable IPv6 for ethernet device
		setup_disable_ipv6(phy_dev, 1);
	}

	/* We should apply basic ipv6 proc setting before enable ipv6,
	 * otherwise it will take default ipv6 setting.
	 * Such as, it will get SLAAC ipv6 address for (AddrMode == IPV6_WAN_STATIC) */
	setup_disable_ipv6(dev, 1);

	//TR069 service don't need route packet.
	if (pEntry->applicationtype == X_CT_SRV_TR069) {
		snprintf(cmdBuf, sizeof(cmdBuf), "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/forwarding", dev);
	}
	else {
		//Note: napt_v6 must enable forwarding to be set
		snprintf(cmdBuf, sizeof(cmdBuf), "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/forwarding", dev);
	}
	system(cmdBuf);

	/* If forwarding is enabled, RA are not accepted unless the special
	 * hybrid mode (accept_ra=2) is enabled.
	 */
	snprintf(cmdBuf, sizeof(cmdBuf), "/bin/echo 2 > /proc/sys/net/ipv6/conf/%s/accept_ra", dev);
	system(cmdBuf);

	/* Enable autoconf when selected autoconf and accept default route in RA only when the WAN belong to default GW , IulianWu */
	
	/* If select Static IPV6 */
	if (pEntry->AddrMode & IPV6_WAN_STATIC) 
	{
		snprintf(cmdBuf, sizeof(cmdBuf), "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", dev);
		system(cmdBuf);
	}
	else /* If select SLAAC mode, stateless and stateful DHCP mode. */
	{
		//need get gateway information from RA, so for dynamic address case always enable accept_ra_defrtr
		snprintf(cmdBuf, sizeof(cmdBuf), "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", dev);
		system(cmdBuf);
	}

	/* Set kernel IPv6 autoconf for WAN interface with IPv6 mode */
	if ((pEntry->AddrMode & IPV6_WAN_AUTO)
#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
		|| (pEntry->AddrMode == IPV6_WAN_AUTO_DETECT_MODE) /* IPv6 auto detect Initial state */
#endif
	){
		snprintf(cmdBuf, sizeof(cmdBuf), "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/autoconf", dev);
		system(cmdBuf);
	}else{
		snprintf(cmdBuf, sizeof(cmdBuf), "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/autoconf", dev);
		system(cmdBuf);
	}

	if ((pEntry->IpProtocol & IPVER_IPV6) && (pEntry->AddrMode != IPV6_WAN_STATIC))
	{
		/* Reset IPv6 global IP for non-Static case to avoid trigger IPv6 global IP
		 * have the same IP case. So we need to reset it at startIP_for_V6().
		 */
		memset(&pEntry->Ipv6Addr[0], 0, sizeof(pEntry->Ipv6Addr));
		pEntry->Ipv6AddrPrefixLen = 0;
		//Save this mib chain entry
		if (getWanEntrybyIfname(dev, &Entry, &entry_index) == 0)
			mib_chain_update(MIB_ATM_VC_TBL, (void *)pEntry, entry_index);
	}
	/* it should enable ipv6 before ppp ipv6cp, event auto detect mode. */
	setup_disable_ipv6(dev, 0);
#endif /* CONFIG_USER_SPPPD */

	set_ipv6_default_policy_route(pEntry, 1);

	return 1;
		
}

#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
/*	RA monitor handler, it auto detect MO bit of RA to config IPv6 Service
 *	Return Value:
 *	  0 if Success
 *	  -1 if Error
 */
int startIPv6AutoDetect(MIB_CE_ATM_VC_Tp pEntry)
{
	char dev[IFNAMSIZ] = {0};
	char cmdBuf[128] = {0};
	FILE *ra_file_p = NULL;
	char *ra_file_pos = NULL;
	char ra_file_path[64] = {0};
	char ra_file_content[256] = {0};
	char ipv6_prefixaddr[INET6_ADDRSTRLEN] = {0};
	int m_flag = -1, o_flag = -1;

	if(!pEntry){
		fprintf(stderr, "[%s] null ATM_VC_TBL\n", __FUNCTION__);
		return -1;
	}

	/*	RA monitor will auto detect change in ra.
	 *	this function shoult be called in following cases,
	 *	  case 1: Receive First RA
	 *	    In this case, IPv6 Config should be default setting.
	 *	      "IPoE" for startIP_for_V6()
	 *	      "PPPoE"
	 *	        - spppd for startPPP_for_V6()
	 *	        - pppd for ppp-init script
	 *	    than it will depends on MO bit to change IPv6 Setting.
	 *
	 *	  [TODO]case 2: Detect RA info change
	 *      In this case, it should recover setting and trigger again.
	 */
	ifGetName(pEntry->ifIndex, dev, sizeof(dev));
	if (pEntry->AddrMode == IPV6_WAN_AUTO_DETECT_MODE && get_net_link_status(dev) == 1)
	{
#ifdef DHCPV6_ISC_DHCP_4XX
        //clear old dhclient deamon and old stateful address
        del_dhcpc6(pEntry);
#endif
		/* Open /var/rainfo_xxx to know auto mode */
		snprintf(ra_file_path, sizeof(ra_file_path), "%s_%s", (char *)RA_INFO_FILE, dev);
		if ((ra_file_p = fopen(ra_file_path, "r")) != NULL)
		{
			/* First get M O flag info. */
			while (fgets(ra_file_content, sizeof(ra_file_content), ra_file_p) != NULL)
			{
				if ((ra_file_pos = strstr(ra_file_content, RA_INFO_M_BIT_KEY)) != NULL)
				{
					m_flag = atoi(ra_file_pos + strlen(RA_INFO_M_BIT_KEY));
					continue;
				}
				if ((ra_file_pos = strstr(ra_file_content, RA_INFO_O_BIT_KEY)) != NULL)
				{
					o_flag = atoi(ra_file_pos + strlen(RA_INFO_O_BIT_KEY));
					if (m_flag != 1 && m_flag != -1) /* If Stateful DHCPv6, need to get prefix info. */
						break;
				}
				if (m_flag == 1 && (ra_file_pos = strstr(ra_file_content, RA_INFO_PREFIX_KEY)) != NULL)
				{
					snprintf(ipv6_prefixaddr, sizeof(ipv6_prefixaddr), "%s", ra_file_pos + strlen(RA_INFO_PREFIX_KEY));
					printf("%s: prefixaddr=%s\n", __func__, ipv6_prefixaddr);
				}
			}
			printf("%s: %s's M bit=%d, O bit=%d\n", __func__, dev, m_flag, o_flag);
			fclose(ra_file_p);

#ifdef CONFIG_USER_DHCP_ISC
			/* IPv6 auto mode do DHCP6 Client. */
			if (m_flag == 0 && o_flag == 1) /* Stateless DHCP mode (M = 0, O = 1), DNS and PD from DHCPv6 Client. */ 
			{
				make_dhcpcv6_conf(pEntry);
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
				/* Use DUID_LL which generated by rtk to request same IA info. */
				rtk_do_isc_dhcpc6_lease_with_duid_ll(pEntry, dev);
#endif
				do_dhcpc6(pEntry, STATELESS_DHCP_MODE);
			}
			else if (m_flag == 1) /* Stateful DHCP mode (M = 1, O = 0,1,) WAN IP and DNS all from DHCPv6 Client. */
			{
				/* prevent kernel generate SLAAC when receive next RA. */
				snprintf(cmdBuf, sizeof(cmdBuf), "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/autoconf", dev);
				system(cmdBuf);

				/* we need to delete original WAN global address. */
				delWanGlobalIPv6Address(pEntry);

				make_dhcpcv6_conf(pEntry);
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
				/* Use DUID_LL which generated by rtk to request same IA info. */
				rtk_do_isc_dhcpc6_lease_with_duid_ll(pEntry, dev);
#endif
				do_dhcpc6(pEntry, STATEFUL_DHCP_MODE);
			}else{
				printf("%s: unexpect MO bit for %s\n", __func__, dev);
			}
#endif // of DHCPV6_ISC_DHCP_4XX
		}
		else
		{
			printf("%s: open %s file error!!!\n", __func__, ra_file_path);
		}
	}

#ifdef CONFIG_USER_RTK_RA_DELEGATION
	if ((pEntry->AddrMode == IPV6_WAN_AUTO) && (!(pEntry->Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD))){
		unsigned char dhcpv6_type =0;

		if (!mib_get_s(MIB_DHCPV6S_TYPE, (void *)&dhcpv6_type, sizeof(dhcpv6_type)) ){
			printf("[%s:%d] Error!! Get LAN  DHCPv6 server	Type failed\n",__func__,__LINE__);
		}

		if (dhcpv6_type == DHCPV6S_TYPE_RA_DELEGATION){
			make_dhcpcv6_conf(pEntry);
			if((pEntry->applicationtype&X_CT_SRV_INTERNET) || (pEntry->applicationtype&X_CT_SRV_OTHER))
				do_dhcpc6(pEntry, STATELESS_DHCP_MODE);
			else
				do_dhcpc6(pEntry, INFO_REQUEST_DHCP_MODE);
		}
	}
#endif

	return 0;
}
#endif
#endif
/* del_ipv6_global_lan_ip_by_wan must done before dhcp6c lease file be deleted. */
int del_ipv6_global_lan_ip_by_wan(const char *lan_infname, MIB_CE_ATM_VC_Tp pEntry)
{
	char ifname[IFNAMSIZ] = {0}, leasefile[64] = {0};
	DLG_INFO_T dlg_info;
	struct in6_addr new_lan_global_ip;
	char ipv6_addr_string[INET6_ADDRSTRLEN] = {0};
	char tmp_cmd[128] = {0};
	int uInt = 0;


	if (pEntry->AddrMode != IPV6_WAN_STATIC)
	{
		ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));
		snprintf(leasefile, sizeof(leasefile), "%s.%s", PATH_DHCLIENT6_LEASE, ifname);
		if (getLeasesInfo(leasefile, &dlg_info) & LEASE_GET_IAPD)
		{
			uInt = dlg_info.prefixLen;
			if (uInt == 0) {
				printf("[%s(%d)]WARNNING! Prefix Length == %d\n", __FUNCTION__, __LINE__, uInt);
				uInt = 64;
			}
			/* maybe pd changed, so need to delete ori LAN IP and add new LAN IP. */
			if (inet_ntop(PF_INET6, (struct in6_addr *)pEntry->Ipv6LocalAdress, ipv6_addr_string, sizeof(ipv6_addr_string)))
			{
				if (strcmp(ipv6_addr_string, "::")) //:: means no IP, don't need to delete it.
				{
					snprintf(tmp_cmd, sizeof(tmp_cmd), "%s/%d", ipv6_addr_string, uInt);
					va_cmd_no_error(IFCONFIG, 3, 1, lan_infname, "del", tmp_cmd); //del old global IP with old PD.
					printf("%s: Del %s IP %s\n", __FUNCTION__, lan_infname, tmp_cmd);
					if (uInt != 64)
					{
						//now still del IP with PD length 64
						snprintf(tmp_cmd, sizeof(tmp_cmd), "%s/64", ipv6_addr_string);
						va_cmd(IFCONFIG, 3, 1, lan_infname, "del", tmp_cmd); //del old global IP with old PD.
						printf("%s: Del %s IP %s\n", __FUNCTION__, lan_infname, tmp_cmd);
					}
				}
				return 1;
			}
		}
	}
	else
		; //del static lan global IP for static wan PD

	// No IP need to deleted.
	return 0;

}

int set_other_delegated_default_wanconn(int old_ifIndex)
{
	unsigned char lanIPv6PrefixMode;
	int index = 0;;
	int mib_total = mib_chain_total(MIB_ATM_VC_TBL);
	MIB_CE_ATM_VC_T pvcEntry;

	if (!mib_get_s(MIB_PREFIXINFO_PREFIX_MODE, (void *)&lanIPv6PrefixMode, sizeof(lanIPv6PrefixMode)))
	{
		printf("Error! Fail to get MIB_PREFIXINFO_PREFIX_MODE!\n");
		return -1;
	}

	if (lanIPv6PrefixMode != IPV6_PREFIX_DELEGATION) //not WAN DELEGATION mode
		return -1;

	for (index = 0; index < mib_total; index++)
	{
		mib_chain_get(MIB_ATM_VC_TBL, index, &pvcEntry);

#if 0
		if (!(pvcEntry.IpProtocol & IPVER_IPV6))
			continue;

		if (pvcEntry.cmode == CHANNEL_MODE_BRIDGE)
			continue;
#endif
		if (!((pvcEntry.enable) && (pvcEntry.cmode != CHANNEL_MODE_BRIDGE)
			&& ((pvcEntry.IpProtocol&IPVER_IPV6)
#ifdef CONFIG_IPV6_SIT_6RD
			|| (pvcEntry.IpProtocol == IPVER_IPV4 && pvcEntry.v6TunnelType == V6TUNNEL_6RD)
#endif
#ifdef CONFIG_IPV6_SIT
			|| (pvcEntry.IpProtocol == IPVER_IPV4 && pvcEntry.v6TunnelType == V6TUNNEL_6TO4)
#endif
			)))
			continue;

		if (pvcEntry.ifIndex == old_ifIndex)
			continue; //don't set old wan ifindex
		else
		{
#ifdef CONFIG_USER_RTK_WAN_CTYPE
			if (pvcEntry.applicationtype & X_CT_SRV_INTERNET)
#endif
			{
#ifdef DHCPV6_ISC_DHCP_4XX
				if (is_exist_pd_info(&pvcEntry) > 0)
#endif
				{
					if ((pvcEntry.Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)
						|| (pvcEntry.AddrMode == IPV6_WAN_STATIC) // WAN static prefixv6 case
#ifdef  CONFIG_IPV6_SIT_6RD
						|| (pvcEntry.v6TunnelType == V6TUNNEL_6RD)
#endif
#ifdef CONFIG_IPV6_SIT
						|| (pvcEntry.v6TunnelType == V6TUNNEL_6TO4)
#endif
#ifdef CONFIG_USER_RTK_RA_DELEGATION
						|| ((pvcEntry.AddrMode & IPV6_WAN_AUTO) &&(!(pvcEntry.Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)))
#endif
						)
					{
						if (!mib_set(MIB_PREFIXINFO_DELEGATED_WANCONN, (void *)&pvcEntry.ifIndex))
						{
							printf("%s Error! Fail to set MIB_PREFIXINFO_DELEGATED_WANCONN!\n", __FUNCTION__);
							return -1;
						}
						else
						{
							printf("%s set PREFIXINFO_DELEGATED_WANCONN to %d\n", __FUNCTION__, pvcEntry.ifIndex);
							return pvcEntry.ifIndex;
						}
					}
				}
			}
		}
	}
	return -1;
}

int set_other_delegated_default_dns_wanconn(int old_ifIndex)
{
	unsigned char lanIPv6DnsMode;
	int index = 0;;
	int mib_total = mib_chain_total(MIB_ATM_VC_TBL);
	MIB_CE_ATM_VC_T pvcEntry;

	if (!mib_get_s(MIB_LAN_DNSV6_MODE, (void *)&lanIPv6DnsMode, sizeof(lanIPv6DnsMode)))
	{
		printf("Error! Fail to get MIB_LAN_DNSV6_MODE!\n");
		return -1;
	}

	if (lanIPv6DnsMode != IPV6_DNS_WANCONN)
		return -1;

	for (index = 0; index < mib_total; index++)
	{
		mib_chain_get(MIB_ATM_VC_TBL, index, &pvcEntry);

		if (!((pvcEntry.enable) && (pvcEntry.cmode != CHANNEL_MODE_BRIDGE)
					&& ((pvcEntry.IpProtocol&IPVER_IPV6)
#ifdef  CONFIG_IPV6_SIT_6RD
						|| (pvcEntry.IpProtocol == IPVER_IPV4 && pvcEntry.v6TunnelType == V6TUNNEL_6RD)
#endif
#ifdef CONFIG_IPV6_SIT
						|| (pvcEntry.IpProtocol == IPVER_IPV4 && pvcEntry.v6TunnelType == V6TUNNEL_6TO4)
#endif
					   )))
			continue;

		if (pvcEntry.ifIndex == old_ifIndex)
			continue; //don't set old wan ifindex

		else
		{
#ifdef CONFIG_USER_RTK_WAN_CTYPE
			if (pvcEntry.applicationtype & X_CT_SRV_INTERNET)
#endif
			{
				//check DNS wanconn
				{
					if (!mib_set(MIB_DNSINFO_WANCONN, (void *)&pvcEntry.ifIndex))
					{
						printf("%s Error! Fail to set MIB_DNSINFO_WANCONN!\n", __FUNCTION__);
						return -1;
					}
					else
					{
						printf("%s set MIB_DNSINFO_WANCONN to %d\n", __FUNCTION__, pvcEntry.ifIndex);
						return pvcEntry.ifIndex;
					}
				}
			}
		}
	}
	return -1;
}



int stopIP_PPP_for_V6(MIB_CE_ATM_VC_Tp pEntry)
{
	unsigned char value[64];
	unsigned char 	Ipv6AddrStr[48], RemoteIpv6AddrStr[48];
	char *argv[20];
	char inf[IFNAMSIZ], infTun[10], v6NetRoute[10];
	int len;

	if (pEntry->IpProtocol & IPVER_IPV6) {
#ifdef DHCPV6_ISC_DHCP_4XX
		//First del DHCPv6 client to let it send DHCPv6 Release packet
		del_dhcpc6(pEntry);
#endif
		// Get interface name
		if (pEntry->cmode == CHANNEL_MODE_PPPOE || pEntry->cmode == CHANNEL_MODE_PPPOA)
		{
			snprintf(inf, 6, "ppp%u", PPP_INDEX(pEntry->ifIndex));
		}
		else{
			ifGetName( PHY_INTF(pEntry->ifIndex), inf, sizeof(inf));
		}

		/*we need do it before del lease file,so move it here*/
		/* del LAN global IP of this wan's PD */
		del_ipv6_global_lan_ip_by_wan((char *)BRIF, pEntry);
		// Start Slaac
		snprintf(value, 64, "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/autoconf", inf);
		system(value);

		// Set default value (forwarding=1)
		//snprintf(value, 64, "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/forwarding", inf);
		//system(value);

		if ( ((pEntry->AddrMode & IPV6_WAN_STATIC)) == IPV6_WAN_STATIC ) {
			inet_ntop(PF_INET6, (struct in6_addr *)pEntry->Ipv6Addr, Ipv6AddrStr, sizeof(Ipv6AddrStr));
			inet_ntop(PF_INET6, (struct in6_addr *)pEntry->RemoteIpv6Addr, RemoteIpv6AddrStr, sizeof(RemoteIpv6AddrStr));

			// delete WAN IP for MER
			len = sizeof(Ipv6AddrStr) - strlen(Ipv6AddrStr);
			if(snprintf(Ipv6AddrStr + strlen(Ipv6AddrStr), len, "/%d", pEntry->Ipv6AddrPrefixLen) >= len) {
				printf("warning, string truncated\n");
			}
			va_cmd(IFCONFIG, 3, 1, inf, ARG_DEL, Ipv6AddrStr);

			// delete default gw
			if (isDefaultRouteWan(pEntry)) {
				// route -A inet6 del ::/0 gw 3ffe::0200:00ff:fe00:0100 dev vc0
				va_cmd(ROUTE, 7, 1, FW_ADD, "inet6", ARG_DEL, "::/0", "gw", RemoteIpv6AddrStr, inf);
			}
		}
		else if (pEntry->AddrMode & (IPV6_WAN_AUTO |IPV6_WAN_AUTO_DETECT_MODE |IPV6_WAN_DHCP ))
		{
			/* Due to "ifconfig xxx down" and "smuxctl --if-delete  xxx" will have timing issue in stopConnection
			 * we delete default route in main table here, to make kernel could send RTM_DELROUTE msg to sysytemd 
			 * Then make sure systemd could do handle_v6_route() reset success. 
			 */
			va_cmd(BIN_IP, 6, 1, "-6", "route", "del", "default", "dev", inf);
		}
		/* Del DHCPv6 Server LAN PD route with policy or main table. */
		rtk_del_dhcp6s_pd_policy_route_and_file(pEntry);
#ifdef CONFIG_IP6_NF_TARGET_NPT
		/* Del WAN NPT PD route with policy table. */
		del_wan_npt_pd_route_rule(pEntry);
#endif
		/* del LAN global IP of this wan's PD */
		//del_ipv6_global_lan_ip_by_wan(BRIF, pEntry);
#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
		del_wan_rainfo_file(pEntry);
#endif
#ifdef CONFIG_USER_RADVD
		clean_radvd(); //fix the DUT send RA with lifetime=0 fail, when stop wan connection(close_ipv6_kernel_flag_for_bind_inf).
#endif
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
		//del bind lan wide dhcp6s and oif policy route.
		delete_binding_lan_service_by_wan(pEntry);
#endif
#ifdef CONFIG_USER_NDPPD
		ndp_proxy_clean();
#endif
#ifdef DUAL_STACK_LITE
		// Stop DS-Lite
		if (pEntry->dslite_enable) {
			// ip -6 tunnel del dslite0
			rtk_tun46_delete_tunnel(IPV6_DSLITE, inf);
		}
#endif

//MAP-E
#ifdef CONFIG_USER_MAP_E
		if(pEntry->mape_enable){
			rtk_tun46_delete_tunnel(IPV6_MAPE, inf);
	   }
#endif
//MAP-T
#ifdef CONFIG_USER_MAP_T
		if(pEntry->mapt_enable){
			rtk_tun46_delete_tunnel(IPV6_MAPT, inf);
		}
#endif
//XLAT-464
#ifdef CONFIG_USER_XLAT464
		if(pEntry->xlat464_enable){
			rtk_tun46_delete_tunnel(IPV6_XLAT464, inf);
		}
#endif
//LW4O6
#ifdef CONFIG_USER_LW4O6
		if(pEntry->lw4o6_enable){
			rtk_tun46_delete_tunnel(IPV6_LW4O6, inf);
		}
#endif

		// Stop 6rd
#ifdef CONFIG_IPV6_SIT_6RD
		if ( pEntry->AddrMode & IPV6_WAN_6RD) {
			unsigned char ip6buf[48];
			unsigned char buf[128];

			printf("Stop 6rd config\n");

			//(1) delete tunnel : ip tunnel del tun6rd
			va_cmd("/usr/bin/ip", 3 ,1 ,"tunnel", "del", TUN6RD_IF);

			//(2) delete default route : ip route del default
			va_cmd("/usr/bin/ip", 3, 1, "route", "del", "default");

			//(3) ip -6 addr del 2001:db8:a02:202:EUI64/64 dev br0
			make6RD_prefix(pEntry,ip6buf,sizeof(ip6buf));
			snprintf(buf, sizeof(buf), "%s/64", ip6buf);
			va_cmd("/usr/bin/ip", 6, 1, "-6", "addr", "del",  buf, "dev", BRIF);

			//(4) ip -6 route del ::/0
			va_cmd("/usr/bin/ip", 4, 1, "-6", "route", "del", "::/0");
		}
#endif
#ifdef CONFIG_IPV6_SIT
		if  (pEntry->AddrMode & IPV6_WAN_6IN4 ) {
			AUG_PRT("Stop 6in4 config\n");
			//delete tunnel : ip tunnel del tun6in4
			va_cmd("/usr/bin/ip", 3 ,1 ,"tunnel", "del", TUN6IN4_IF);
		}else if  (pEntry->AddrMode & IPV6_WAN_6TO4 ){
			AUG_PRT("Stop 6to4 config\n");
			//delete tunnel : ip tunnel del tun6to4
			va_cmd("/usr/bin/ip", 3 ,1 ,"tunnel", "del", TUN6TO4_IF);
		}
#endif

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
		/* delete acl rule for mvlan loop detection */
		if(pEntry->cmode!=CHANNEL_MODE_BRIDGE)
			rtk_fc_ipv6_mvlan_loop_detection_acl_delete(pEntry);
#endif
	}
	return 1;
}

void setup_forwarding(const char *itf, int disable)
{   
	char buf[128];

	snprintf(buf, sizeof(buf), "/bin/echo %d > /proc/sys/net/ipv6/conf/%s/forwarding", disable, itf);
	system(buf);
} 

void setup_accept_ra(const char *itf, int disable)
{   
	char buf[128];

	snprintf(buf, sizeof(buf), "/bin/echo %d > /proc/sys/net/ipv6/conf/%s/accept_ra", disable, itf);
	system(buf);
} 

void setup_disable_ipv6(const char *itf, int disable)
{
	char buf[128];

	snprintf(buf, sizeof(buf), "/bin/echo %d > /proc/sys/net/ipv6/conf/%s/disable_ipv6", disable, itf);
	system(buf);
}

int setupIPV6Filter(void)
{
	char *argv[20];
	char buf[128];
	int i, total;
#ifdef CONFIG_E8B
	MIB_CE_IP_PORT_FILTER_T IpEntry;
#else
	MIB_CE_V6_IP_PORT_FILTER_T IpEntry;
#endif
	MIB_CE_ATM_VC_T vcEntry;
	char *policy, *filterSIP, *filterDIP, srcPortRange[12], dstPortRange[12];
	char  srcip[55], dstip[55], srcip2[55], dstip2[55];
	char SIPRange[110]={0};
	char DIPRange[110]={0};
	char *filterSIPRange=NULL;
	char *filterDIPRange=NULL;
#if defined(CONFIG_E8B) ||defined(CONFIG_00R0)
	unsigned char ipportfilter_state = 0;
	int mib_id = MIB_IP_PORT_FILTER_TBL;
#else
	int mib_id = MIB_V6_IP_PORT_FILTER_TBL;
#endif
	filterSIP=filterDIP=NULL;
	char wanname[IFNAMSIZ];

#if defined(CONFIG_E8B) ||defined(CONFIG_00R0)
	mib_get_s(MIB_IPFILTER_ON_OFF, (void*)&ipportfilter_state, sizeof(ipportfilter_state));

	if(ipportfilter_state == 0) return 0;
#endif

	// packet filtering
	// ip filtering
	total = mib_chain_total(mib_id);

	for (i = 0; i < total; i++)
	{
		int idx=0;
#ifdef CONFIG_E8B
		int udp_tcp_idx=-1;
#endif

		/*
		 *	srcPortRange: src port
		 *	dstPortRange: dst port
		 */
		filterSIPRange=filterDIPRange=NULL;
		filterSIP=filterDIP=NULL;
		memset(argv,0,sizeof(argv));

		if (!mib_chain_get(mib_id, i, (void *)&IpEntry))
			return -1;

#ifdef CONFIG_E8B
		if (IpEntry.IpProtocol != IPVER_IPV6) // 1: IPv4, 2:IPv6
			continue;
#endif

		if (IpEntry.action == 0)
			policy = (char *)FW_DROP;
		else
			policy = (char *)FW_RETURN;

		// source port
		if (IpEntry.srcPortFrom == 0)
			srcPortRange[0]='\0';
		else if (IpEntry.srcPortFrom == IpEntry.srcPortTo)
			snprintf(srcPortRange, 12, "%u", IpEntry.srcPortFrom);
		else
			snprintf(srcPortRange, 12, "%u:%u",
					IpEntry.srcPortFrom, IpEntry.srcPortTo);

		// destination port
		if (IpEntry.dstPortFrom == 0)
			dstPortRange[0]='\0';
		else if (IpEntry.dstPortFrom == IpEntry.dstPortTo)
			snprintf(dstPortRange, 12, "%u", IpEntry.dstPortFrom);
		else
			snprintf(dstPortRange, 12, "%u:%u",
					IpEntry.dstPortFrom, IpEntry.dstPortTo);

		// source ip, prefixLen
		if(IpEntry.sip6End[0] == 0)    // normal ip filter, no iprange supported
		{
			inet_ntop(PF_INET6, (struct in6_addr *)IpEntry.sip6Start, srcip, 48);
			if (strcmp(srcip, "::") == 0)
				filterSIP = 0;
			else
			{
				if (IpEntry.sip6PrefixLen!=0){
					snprintf(buf, sizeof(buf), "/%d", IpEntry.sip6PrefixLen);
					strcat(srcip, buf);
					//snprintf(srcip, sizeof(srcip), "%s/%d", srcip, IpEntry.sip6PrefixLen);
				}

				filterSIP = srcip;
			}
		}
		else
		{
			inet_ntop(PF_INET6, (struct in6_addr *)IpEntry.sip6Start, srcip, 48);
			inet_ntop(PF_INET6, (struct in6_addr *)IpEntry.sip6End, srcip2, 48);

			if(strcmp(srcip, "::") ==0 || strcmp(srcip2, "::") ==0)
				filterSIPRange=0;
			else
			{
				snprintf(SIPRange, sizeof(SIPRange), "%s-%s", srcip, srcip2);
				filterSIPRange=SIPRange;
			}
		}

		// destination ip, mask
		if(IpEntry.dip6End[0] == 0)    // normal ip filter, no iprange supported
		{
			inet_ntop(PF_INET6, (struct in6_addr *)IpEntry.dip6Start, dstip, 48);
			if (strcmp(dstip, "::") == 0)
				filterDIP = 0;
			else
			{
				if (IpEntry.dip6PrefixLen!=0){
					snprintf(buf, sizeof(buf), "/%d", IpEntry.dip6PrefixLen);
					strcat(dstip, buf);
					//snprintf(dstip, sizeof(dstip), "%s/%d", dstip, IpEntry.dip6PrefixLen);
				}

				filterDIP = dstip;
			}
		}
		else
		{
			inet_ntop(PF_INET6, (struct in6_addr *)IpEntry.dip6Start, dstip, 48);
			inet_ntop(PF_INET6, (struct in6_addr *)IpEntry.dip6End, dstip2, 48);

			if(strcmp(dstip, "::") ==0 || strcmp(dstip2, "::") ==0)
				filterDIPRange=0;
			else
			{
				snprintf(DIPRange, sizeof(DIPRange), "%s-%s", dstip, dstip2);
				filterDIPRange=DIPRange;
			}
		}

#ifdef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
		int numOfIpv6, index=0;
		struct ipv6_ifaddr ip6_addr;
		char srcIpStart[IP6_ADDR_LEN]={0},srcIpEnd[IP6_ADDR_LEN]={0},dstIpStart[IP6_ADDR_LEN]={0},dstIpEnd[IP6_ADDR_LEN]={0};
		numOfIpv6 = getifip6("br0", IPV6_ADDR_UNICAST, &ip6_addr, 1);

		if (numOfIpv6 !=0 )
		{
			if ( (memcmp(IpEntry.sIfId6Start, (char[IP6_ADDR_LEN]){ 0 }, IP6_ADDR_LEN) != 0) ){
				if(memcmp(IpEntry.sIfId6End, (char[IP6_ADDR_LEN]){ 0 }, IP6_ADDR_LEN) != 0) {
					//Have start and end
					memcpy(srcIpStart, &ip6_addr.addr, IP6_ADDR_LEN);
					memcpy(srcIpEnd, &ip6_addr.addr, IP6_ADDR_LEN);
					for (index=0; index<8; index++){
						srcIpStart[index+8] = IpEntry.sIfId6Start[index+8];
						srcIpEnd[index+8] = IpEntry.sIfId6End[index+8];
					}
					inet_ntop(PF_INET6, (struct in6_addr *)srcIpStart, srcip, 48);
					inet_ntop(PF_INET6, (struct in6_addr *)srcIpEnd, srcip2, 48);

					if(strcmp(srcip, "::") ==0 || strcmp(srcip2, "::") ==0)
						filterSIPRange=0;
					else
					{
						snprintf(SIPRange, sizeof(SIPRange), "%s-%s", srcip, srcip2);
						filterSIPRange=SIPRange;
					}
				}else{	//Only have Start
					memcpy(srcIpStart, &ip6_addr.addr, IP6_ADDR_LEN);
					for (index=0; index<8; index++){
						srcIpStart[index+8] = IpEntry.sIfId6Start[index+8];
					}
					inet_ntop(PF_INET6, (struct in6_addr *)srcIpStart, srcip, 48);

					filterSIP = srcip;
				}
			}

		if ( (memcmp(IpEntry.dIfId6Start, (char[IP6_ADDR_LEN]){ 0 }, IP6_ADDR_LEN) != 0) ){
				if(memcmp(IpEntry.dIfId6End, (char[IP6_ADDR_LEN]){ 0 }, IP6_ADDR_LEN) != 0) {
					//Have start and end
					memcpy(dstIpStart, &ip6_addr.addr, IP6_ADDR_LEN);
					memcpy(dstIpEnd, &ip6_addr.addr, IP6_ADDR_LEN);
					for (index=0; index<8; index++){
						dstIpStart[index+8] = IpEntry.dIfId6Start[index+8];
						dstIpEnd[index+8] = IpEntry.dIfId6End[index+8];
					}
					inet_ntop(PF_INET6, (struct in6_addr *)dstIpStart, dstip, 48);
					inet_ntop(PF_INET6, (struct in6_addr *)dstIpEnd, dstip2, 48);
					if(strcmp(dstip, "::") ==0 || strcmp(dstip2, "::") ==0)
						filterDIPRange=0;
					else
					{
						snprintf(DIPRange, sizeof(DIPRange), "%s-%s", dstip, dstip2);
						filterDIPRange=DIPRange;
					}
				}
				else{ //Only have start
					memcpy(dstIpStart, &ip6_addr.addr, IP6_ADDR_LEN);
					for (index=0; index<8; index++){
						dstIpStart[index+8] = IpEntry.dIfId6Start[index+8];
					}
					inet_ntop(PF_INET6, (struct in6_addr *)dstIpStart, dstip, 48);
					filterDIP = dstip;
				}
			}
		}
#endif

		// interface
		argv[1] = (char *)FW_ADD;
		argv[2] = (char *)FW_IPV6FILTER;
		idx = 3;

		if (IpEntry.dir == DIR_IN){
			if(IpEntry.ifIndex == DUMMY_IFINDEX){
				argv[idx++] = "!";
				argv[idx++] = (char *)ARG_I;
				argv[idx++] = (char *)LANIF_ALL;
			}
			else{
				getWanEntrybyindex(&vcEntry,IpEntry.ifIndex);
				ifGetName(IpEntry.ifIndex, wanname, sizeof(wanname));
				if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
				{
				    argv[idx++] = "-m";
				    argv[idx++] = "physdev";
				    argv[idx++] = "--physdev-in";
				}
				else
				{
				    argv[idx++] = (char *)ARG_I;
				}
				argv[idx++] = wanname;
			}
		}
		else{
			argv[idx++] = (char *)ARG_I;
			argv[idx++] = (char *)LANIF;
		}

		// protocol
		if (IpEntry.protoType != PROTO_NONE) {
			argv[idx++] = "-p";
			if (IpEntry.protoType == PROTO_TCP)
				argv[idx++] = (char *)ARG_TCP;
			else if (IpEntry.protoType == PROTO_UDP)
				argv[idx++] = (char *)ARG_UDP;
#ifdef CONFIG_E8B
			else if(IpEntry.protoType == PROTO_UDPTCP){
				//add udp rule first for udp/tcp protocol
				udp_tcp_idx = idx;
				argv[idx++] = (char *)ARG_UDP;
			}
#endif
			else //if (IpEntry.protoType == PROTO_ICMPV6)
				argv[idx++] = (char *)ARG_ICMPV6;
		}

		// src ip
		if (filterSIP != 0)
		{
			argv[idx++] = "-s";
			argv[idx++] = filterSIP;

		}

		// src port
		if ((IpEntry.protoType==PROTO_TCP ||
					IpEntry.protoType==PROTO_UDP
#ifdef CONFIG_E8B
			|| IpEntry.protoType==PROTO_UDPTCP
#endif
			) && srcPortRange[0] != 0) {
			argv[idx++] = (char *)FW_SPORT;
			argv[idx++] = srcPortRange;
		}

		// dst ip
		if (filterDIP != 0)
		{
			argv[idx++] = "-d";
			argv[idx++] = filterDIP;
		}

		// iprange
		if(filterSIPRange || filterDIPRange)
		{
			argv[idx++] = "-m";
			argv[idx++] = "iprange";
			if(filterSIPRange)
			{
				argv[idx++] = "--src-range";
				argv[idx++] = filterSIPRange;
			}
			if(filterDIPRange)
			{
				argv[idx++] = "--dst-range";
				argv[idx++] = filterDIPRange;
			}
		}

		// dst port
		if ((IpEntry.protoType==PROTO_TCP ||
					IpEntry.protoType==PROTO_UDP
#ifdef CONFIG_E8B
			|| IpEntry.protoType==PROTO_UDPTCP
#endif
			) && dstPortRange[0] != 0) {
			argv[idx++] = (char *)FW_DPORT;
			argv[idx++] = dstPortRange;
		}

		// target/jump
		argv[idx++] = "-j";
		argv[idx++] = policy;
		argv[idx++] = NULL;

		//printf("idx=%d\n", idx);
		TRACE(STA_SCRIPT, "%s %s %s %s %s %s %s ...\n", IP6TABLES, argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
		do_cmd(IP6TABLES, argv, 1);

#ifdef CONFIG_E8B
		if(udp_tcp_idx!=-1){
			//add tcp rule for udp/tcp protocol
			argv[udp_tcp_idx] = (char *)ARG_TCP;
			TRACE(STA_SCRIPT, "%s %s %s %s %s %s %s ...\n", IP6TABLES, argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
			do_cmd(IP6TABLES, argv, 1);
		}
#endif
	}

	return 1;
}
#ifdef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
int setEachIPv6FilterRuleMixed(MIB_CE_V6_IP_PORT_FILTER_Tp pIpEntry, PREFIX_V6_INFO_Tp pDLGInfo)
{
	int idx=0,index=0, len;
	char srcIpStart[IP6_ADDR_LEN]={0},srcIpEnd[IP6_ADDR_LEN]={0},dstIpStart[IP6_ADDR_LEN]={0},dstIpEnd[IP6_ADDR_LEN]={0};
	char srcip[55]={0}, dstip[55]={0}, srcip2[55]={0}, dstip2[55]={0};
	char *argv[20]={0};
	char *filterSIPRange=NULL;
	char *filterDIPRange=NULL;
	char *policy=NULL, *filterSIP=NULL, *filterDIP=NULL, srcPortRange[12]={0}, dstPortRange[12]={0};
	char SIPRange[110]={0};
	char DIPRange[110]={0};
	unsigned char empty_ipv6[IP6_ADDR_LEN] = {0};
	char wanname[IFNAMSIZ];
	MIB_CE_ATM_VC_T vcEntry;

	if(!pIpEntry){
		printf("Error! Incorrect Parameter!\n");
		return 0;
	}


	//Set by PD+InterfaceID, Need Check PD not empty!
	if ( (memcmp(pIpEntry->sIfId6Start, (char[IP6_ADDR_LEN]){ 0 }, IP6_ADDR_LEN) != 0) 
		|| (memcmp(pIpEntry->dIfId6Start, (char[IP6_ADDR_LEN]){ 0 }, IP6_ADDR_LEN) != 0) )
	{
		if(!pDLGInfo || memcmp(pDLGInfo->prefixIP,empty_ipv6,IP6_ADDR_LEN) == 0 )
			return 0;
	}

	
	if (pIpEntry->action == 0)
		policy = (char *)FW_DROP;
	else
		policy = (char *)FW_RETURN;

	// source port
	if (pIpEntry->srcPortFrom == 0)
		srcPortRange[0]='\0';
	else if (pIpEntry->srcPortFrom == pIpEntry->srcPortTo)
		snprintf(srcPortRange, 12, "%u", pIpEntry->srcPortFrom);
	else
		snprintf(srcPortRange, 12, "%u:%u",
				pIpEntry->srcPortFrom, pIpEntry->srcPortTo);

	// destination port
	if (pIpEntry->dstPortFrom == 0)
		dstPortRange[0]='\0';
	else if (pIpEntry->dstPortFrom == pIpEntry->dstPortTo)
		snprintf(dstPortRange, 12, "%u", pIpEntry->dstPortFrom);
	else
		snprintf(dstPortRange, 12, "%u:%u",
				pIpEntry->dstPortFrom, pIpEntry->dstPortTo);

	// source ip, prefixLen
	if(pIpEntry->sip6End[0] == 0)    // normal ip filter, no iprange supported
	{
		inet_ntop(PF_INET6, (struct in6_addr *)pIpEntry->sip6Start, srcip, 48);
		if (strcmp(srcip, "::") != 0)
		{
			if (pIpEntry->sip6PrefixLen!=0) {
				len = sizeof(srcip) - strlen(srcip);
				if(snprintf(srcip + strlen(srcip), len, "/%d", pIpEntry->sip6PrefixLen) >= len) {
					printf("warning, string truncated\n");
				}
			}

			filterSIP = srcip;
		}
	}
	else
	{
		inet_ntop(PF_INET6, (struct in6_addr *)pIpEntry->sip6Start, srcip, 48);
		inet_ntop(PF_INET6, (struct in6_addr *)pIpEntry->sip6End, srcip2, 48);

		if(strcmp(srcip, "::") ==0 || strcmp(srcip2, "::") ==0)
			filterSIPRange=0;
		else
		{
			snprintf(SIPRange, sizeof(SIPRange), "%s-%s", srcip, srcip2);
			filterSIPRange=SIPRange;
		}
	}

	// destination ip, mask
	if(pIpEntry->dip6End[0] == 0)    // normal ip filter, no iprange supported
	{
		inet_ntop(PF_INET6, (struct in6_addr *)pIpEntry->dip6Start, dstip, 48);
		if (strcmp(dstip, "::") != 0)
		{
			if (pIpEntry->dip6PrefixLen!=0) {
				len = sizeof(dstip) - strlen(dstip);
				if(snprintf(dstip + strlen(dstip), len, "/%d", pIpEntry->dip6PrefixLen) >= len) {
					printf("warning, string truncated\n");
				}
			}

			filterDIP = dstip;
		}
	}
	else
	{
		inet_ntop(PF_INET6, (struct in6_addr *)pIpEntry->dip6Start, dstip, 48);
		inet_ntop(PF_INET6, (struct in6_addr *)pIpEntry->dip6End, dstip2, 48);

		if(strcmp(dstip, "::") ==0 || strcmp(dstip2, "::") ==0)
			filterDIPRange=0;
		else
		{
			snprintf(DIPRange, sizeof(DIPRange), "%s-%s", dstip, dstip2);
			filterDIPRange=DIPRange;
		}
	}


	if(pDLGInfo && memcmp(pDLGInfo->prefixIP,empty_ipv6,IP6_ADDR_LEN))
	{
		if ( (memcmp(pIpEntry->sIfId6Start, (char[IP6_ADDR_LEN]){ 0 }, IP6_ADDR_LEN) != 0) ){
			if(memcmp(pIpEntry->sIfId6End, (char[IP6_ADDR_LEN]){ 0 }, IP6_ADDR_LEN) != 0) {
				//Have start and end
				memcpy(srcIpStart, (void *) pDLGInfo->prefixIP, IP6_ADDR_LEN);
				memcpy(srcIpEnd, (void *) pDLGInfo->prefixIP, IP6_ADDR_LEN);
				for (index=0; index<8; index++){
					srcIpStart[index+8] = pIpEntry->sIfId6Start[index+8];
					srcIpEnd[index+8] = pIpEntry->sIfId6End[index+8];
				}

				inet_ntop(PF_INET6, (struct in6_addr *)srcIpStart, srcip, 48);
				inet_ntop(PF_INET6, (struct in6_addr *)srcIpEnd, srcip2, 48);

				if(strcmp(srcip, "::") ==0 || strcmp(srcip2, "::") ==0)
					filterSIPRange=0;
				else
				{
					snprintf(SIPRange, sizeof(SIPRange), "%s-%s", srcip, srcip2);
					filterSIPRange=SIPRange;
				}
			}else{  //Only have Start
				memcpy(srcIpStart, (void *) pDLGInfo->prefixIP, IP6_ADDR_LEN);
				for (index=0; index<8; index++){
					srcIpStart[index+8] = pIpEntry->sIfId6Start[index+8];
				}
				inet_ntop(PF_INET6, (struct in6_addr *)srcIpStart, srcip, 48);

				filterSIP = srcip;
			}
		}

		if ( (memcmp(pIpEntry->dIfId6Start, (char[IP6_ADDR_LEN]){ 0 }, IP6_ADDR_LEN) != 0) ){
			if(memcmp(pIpEntry->dIfId6End, (char[IP6_ADDR_LEN]){ 0 }, IP6_ADDR_LEN) != 0) {
				//Have start and end
				memcpy(dstIpStart, (void *) pDLGInfo->prefixIP, IP6_ADDR_LEN);
				memcpy(dstIpEnd, (void *) pDLGInfo->prefixIP, IP6_ADDR_LEN);
				for (index=0; index<8; index++){
					dstIpStart[index+8] = pIpEntry->dIfId6Start[index+8];
					dstIpEnd[index+8] = pIpEntry->dIfId6End[index+8];
				}

				inet_ntop(PF_INET6, (struct in6_addr *)dstIpStart, dstip, 48);
				inet_ntop(PF_INET6, (struct in6_addr *)dstIpEnd, dstip2, 48);

				if(strcmp(dstip, "::") ==0 || strcmp(dstip2, "::") ==0)
					filterDIPRange=0;
				else
				{
					snprintf(DIPRange, sizeof(DIPRange), "%s-%s", dstip, dstip2);
					filterDIPRange=DIPRange;
				}
			}else{ //Only have start
				memcpy(dstIpStart, (void *) pDLGInfo->prefixIP, IP6_ADDR_LEN);
				for (index=0; index<8; index++){
					dstIpStart[index+8] = pIpEntry->dIfId6Start[index+8];
				}

				inet_ntop(PF_INET6, (struct in6_addr *)dstIpStart, dstip, 48);
				filterDIP = dstip;
			}
		}
	}

	// interface
	argv[1] = (char *)FW_ADD;
	argv[2] = (char *)FW_IPV6FILTER;

	idx = 3;

	if (pIpEntry->dir == DIR_IN){
		if(pIpEntry->ifIndex == DUMMY_IFINDEX){
			argv[idx++] = "!";
			argv[idx++] = (char *)ARG_I;
			argv[idx++] = (char *)LANIF;
		}
		else{
			getWanEntrybyindex(&vcEntry,pIpEntry->ifIndex);
			ifGetName(pIpEntry->ifIndex, wanname, sizeof(wanname));
			if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
			{
			    argv[idx++] = "-m";
			    argv[idx++] = "physdev";
			    argv[idx++] = "--physdev-in";
			}
			else
			{
			    argv[idx++] = (char *)ARG_I;
			}
			argv[idx++] = wanname;
		}
	}
	else{
		argv[idx++] = (char *)ARG_I;
		argv[idx++] = (char *)LANIF;
	}

	// protocol
	if (pIpEntry->protoType != PROTO_NONE) {
		argv[idx++] = "-p";
		if (pIpEntry->protoType == PROTO_TCP)
			argv[idx++] = (char *)ARG_TCP;
		else if (pIpEntry->protoType == PROTO_UDP)
			argv[idx++] = (char *)ARG_UDP;
		else //if (pIpEntry->protoType == PROTO_ICMPV6)
			argv[idx++] = (char *)ARG_ICMPV6;
	}

	// src ip
	if (filterSIP != 0)
	{
		argv[idx++] = "-s";
		argv[idx++] = filterSIP;
	}

	// src port
	if ((pIpEntry->protoType==PROTO_TCP ||
				pIpEntry->protoType==PROTO_UDP) &&
			srcPortRange[0] != 0) {
		argv[idx++] = (char *)FW_SPORT;
		argv[idx++] = srcPortRange;
	}

	// dst ip
	if (filterDIP != 0)
	{
		argv[idx++] = "-d";
		argv[idx++] = filterDIP;
	}

	// iprange
	if(filterSIPRange || filterDIPRange)
	{
		argv[idx++] = "-m";
		argv[idx++] = "iprange";
		if(filterSIPRange)
		{
			argv[idx++] = "--src-range";
			argv[idx++] = filterSIPRange;
		}
		if(filterDIPRange)
		{
			argv[idx++] = "--dst-range";
			argv[idx++] = filterDIPRange;
		}
	}

	// dst port
	if ((pIpEntry->protoType==PROTO_TCP ||
				pIpEntry->protoType==PROTO_UDP) &&
			dstPortRange[0] != 0) {
		argv[idx++] = (char *)FW_DPORT;
		argv[idx++] = dstPortRange;
	}

	// target/jump
	argv[idx++] = "-j";
	argv[idx++] = policy;
	argv[idx++] = NULL;

	do_cmd(IP6TABLES, argv, 1);

	return 1;
}

int setupIPV6FilterMixed(void)
{
	unsigned char ivalue;
	int i, total, ret=0;
	MIB_CE_V6_IP_PORT_FILTER_T IPv6PortFilterEntry;
	char *policy;
	PREFIX_V6_INFO_T DLGInfo={0}, radvd_DLGInfo={0};
	unsigned char dhcp6s_mode = 0;
	unsigned char dhcp6s_mode_type = 0;
	struct in6_addr ip6Addr_start, radvd_prefix;
	unsigned char prefixLen = 0;
	struct in6_addr static_ip6Addr_prefix, static_ip6Addr_radvd_prefix;
	unsigned char ipv6RadvdPrefixMode = 0, radvd_prefixLen[MAX_RADVD_CONF_PREFIX_LEN] = {0};
	unsigned char radvd_V6prefix[MAX_RADVD_CONF_PREFIX_LEN] = {0};
	int radvd_mib_ok = 1, radvd_prefix_len = 0;

	printf("Update Firewall rule set by user.\n");

	/* need to consider static dhcpv6 by user */
	mib_get_s(MIB_DHCPV6_MODE, (void *)&dhcp6s_mode, sizeof(dhcp6s_mode));
	mib_get_s(MIB_DHCPV6S_TYPE, (void *)&dhcp6s_mode_type, sizeof(dhcp6s_mode_type));

	if ((dhcp6s_mode == DHCP_LAN_SERVER) && (dhcp6s_mode_type == DHCPV6S_TYPE_STATIC))
	{
			/* need to consider static dhcpv6 server by user */
		mib_get_s(MIB_DHCPV6S_RANGE_START, (void *)ip6Addr_start.s6_addr, sizeof(ip6Addr_start.s6_addr));
		mib_get_s(MIB_DHCPV6S_PREFIX_LENGTH, (void *)&prefixLen, sizeof(prefixLen));
		if (ip6toPrefix(&ip6Addr_start, prefixLen, &static_ip6Addr_prefix))
		{
			memcpy(&DLGInfo.prefixIP, &static_ip6Addr_prefix, sizeof(DLGInfo.prefixIP));
		}
		else
			printf("%s: static prefix error format!\n", __FUNCTION__);
	}
	else
		get_prefixv6_info(&DLGInfo);

#ifdef CONFIG_USER_RADVD
	mib_get_s(MIB_V6_RADVD_PREFIX_MODE, (void *)&ipv6RadvdPrefixMode, sizeof(ipv6RadvdPrefixMode));

	if (ipv6RadvdPrefixMode == RADVD_MODE_MANUAL)
	{
		if (!mib_get_s(MIB_V6_PREFIX_IP, (void *)radvd_V6prefix, sizeof(radvd_V6prefix))) { //STRING_T
			printf("Error!! Get MIB_V6_PREFIX_IP fail!");
			radvd_mib_ok = 0;
		}

		if (!mib_get_s(MIB_V6_PREFIX_LEN, (void *)radvd_prefixLen, sizeof(radvd_prefixLen))) { ////STRING_T
			printf("Error!! Get MIB_V6_PREFIX_LEN fail!");
			radvd_mib_ok = 0;
		}
		if (radvd_mib_ok)
		{
			if (inet_pton(PF_INET6, radvd_V6prefix, &radvd_prefix))
			{
				radvd_prefix_len = atoi(radvd_prefixLen);
				if (ip6toPrefix(&radvd_prefix, radvd_prefix_len, &static_ip6Addr_radvd_prefix))
				{
					memcpy(&radvd_DLGInfo.prefixIP, &static_ip6Addr_radvd_prefix, sizeof(radvd_DLGInfo.prefixIP));
				}
			}
		}

	}
	else
		get_radvd_prefixv6_info(&radvd_DLGInfo);
#endif


	// packet filtering
	// ip filtering
	total = mib_chain_total(MIB_V6_IP_PORT_FILTER_TBL);

	for (i = 0; i < total; i++)
	{
		if(!mib_chain_get(MIB_V6_IP_PORT_FILTER_TBL, i, (void *) &IPv6PortFilterEntry)){
			printf("Error! Get IPv6 Filter Entry fail!\n");
			return 0;
		}

		setEachIPv6FilterRuleMixed(&IPv6PortFilterEntry,&DLGInfo);
#ifdef CONFIG_USER_RADVD
		if (memcmp(&DLGInfo.prefixIP, &radvd_DLGInfo.prefixIP,IP6_ADDR_LEN))
		{
			/* Set radvd prefix v6 filter if prefix is not the same with DHCPv6s prefix */
			setEachIPv6FilterRuleMixed(&IPv6PortFilterEntry, &radvd_DLGInfo);
		}
#endif
	}
	return 1;
}
#endif

#ifdef CONFIG_RTK_IPV6_LOGO_TEST
 int clear_CERouter_firewall(void){

	va_cmd(IP6TABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)FW_IPV6_CE_ROUTER_FILTER);
	va_cmd(IP6TABLES, 6, 1,"-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)FW_IPV6_CE_ROUTER_MANGLE);
	
  	va_cmd(IP6TABLES, 2, 1, (char *)FW_FLUSH, (char *)FW_IPV6_CE_ROUTER_FILTER);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", (char *)FW_FLUSH, (char *)FW_IPV6_CE_ROUTER_MANGLE);

  	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_IPV6_CE_ROUTER_FILTER);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", (char *)FW_IPV6_CE_ROUTER_MANGLE);

 	/*Clear Policy Route for [RFC 7084] CE LOGO Test CERouter 2.7.6 : Unknown Prefix */	
	va_cmd("/bin/sh", 2, 1, "-c", "echo 0 > /proc/sys/net/ipv6/fwmark_reflect");
	va_cmd(BIN_IP, 7, 1, "-6", "rule", "del", "fwmark", MARK_CE_ROUTER, "table", CE_ROUTER_TABLE_ID);
	va_cmd(BIN_IP, 8, 1, "-6", "route", "del", "table", CE_ROUTER_TABLE_ID, "default", "dev", LANIF);
	
	return 0;
}
int setup_CERouter_firewall(void){
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ], leasefile[64];
	int vcTotal, i;
	DLG_INFO_T dlg_info={0};
	char prefix_ipv6_str[64] = {0};
	char prefix_str_plus_pdlen[64] = {0};

	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_IPV6_CE_ROUTER_FILTER);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", (char *)FW_IPV6_CE_ROUTER_MANGLE);

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;

#ifdef DHCPV6_ISC_DHCP_4XX
		/* 	CE-router Test Forwarding RFC7084 - IPv6 CE Router Requirements :Part A: LAN to WAN/Part B: WAN to LAN
		 *	The IPv6 CE router MUST NOT forward any IPv6 traffic 
		 *	between its LAN interface(s) and its WAN interface 
		 *	until the router has successfully completed the IPv6 address and the delegated prefix acquisition process.
		 */
		if (Entry.Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD) {
			if(0 != ifGetName(Entry.ifIndex, ifname, sizeof(ifname))){
				snprintf(leasefile, 64, "%s.%s", PATH_DHCLIENT6_LEASE, ifname);
				memset(&dlg_info, 0, sizeof(DLG_INFO_T));
				if (!((getLeasesInfo(leasefile, &dlg_info)&LEASE_GET_IAPD) && dlg_info.prefixLen != 0)){
					// IA_PD is NOT exist
					// upstream :   ip6tables -A FORWARD -i br0 -o "wan intf" -p icmpv6 --icmpv6-type echo-request   -j DROP
					// downstream:  ip6tables -A FORWARD -i "wan intf" -o br0 -p icmpv6 --icmpv6-type echo-request   -j DROP
					va_cmd(IP6TABLES, 12, 1, (char *)FW_ADD, (char *)FW_IPV6_CE_ROUTER_FILTER, "-i", LANIF, "-o", ifname,"-p", "icmpv6","--icmpv6-type","echo-request","-j", (char *)FW_DROP);	
					va_cmd(IP6TABLES, 12, 1, (char *)FW_ADD, (char *)FW_IPV6_CE_ROUTER_FILTER, "-i", ifname, "-o", LANIF,"-p", "icmpv6","--icmpv6-type","echo-request","-j", (char *)FW_DROP);
					
					/*Test CERouter 1.3.9 : Host Ignores Router Solicitations
			 		 * 28	Part A: All-Router Multicast Destination
		 			* ip6tables -I INPUT -i nas0_0 -p icmpv6 --icmpv6-type router-solicitation -j DROP
					 */
					va_cmd(IP6TABLES, 10, 1, (char *)FW_DEL, (char *)FW_INPUT,"-i",ifname, "-p", "icmpv6","--icmpv6-type","router-solicitation", "-j", (char *)FW_DROP);
					va_cmd(IP6TABLES, 10, 1, (char *)FW_INSERT, (char *)FW_INPUT,"-i",ifname, "-p", "icmpv6","--icmpv6-type","router-solicitation", "-j", (char *)FW_DROP);
				}
				else if (dlg_info.prefixLen > 0 && memcmp(dlg_info.prefixIP, ipv6_unspec, dlg_info.prefixLen<=128?dlg_info.prefixLen:128)){
					//get IA_PD on this WAN.
					// ip6tables -A FORWARD -i wan intf -o "br0" -d 3ffe:501:ffff:1110::/64  -j ACCEPT
					inet_ntop(PF_INET6, (struct in6_addr *)dlg_info.prefixIP, prefix_ipv6_str, sizeof(prefix_ipv6_str));
					snprintf(prefix_str_plus_pdlen, sizeof(prefix_str_plus_pdlen), "%s/%d", prefix_ipv6_str,dlg_info.prefixLen);
					snprintf(prefix_ipv6_str, sizeof(prefix_ipv6_str), "%s/64", prefix_ipv6_str);
					
					/* [RFC 7084] CE LOGO Test CERouter 2.7.6 : Unknown Prefix */
					va_cmd(IP6TABLES, 15, 1, (char *)FW_ADD, (char *)FW_IPV6_CE_ROUTER_FILTER, "!", "-s", prefix_ipv6_str, "-i", LANIF, "-p", "icmpv6", "--icmpv6-type", "echo-request",
						"-j",	(char *)FW_REJECT, "--reject-with", "icmp6-policy-fail");
					/*Blew policy route rule make the ICMPv6 err msg send back to br0, */
					va_cmd("/bin/sh", 2, 1 ,"-c", "echo 1 > /proc/sys/net/ipv6/fwmark_reflect");

					va_cmd(IP6TABLES, 17, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_IPV6_CE_ROUTER_MANGLE, "-i", LANIF, "-p", "icmpv6", "--icmpv6-type", "echo-request", "!", "-s", prefix_ipv6_str, 
						"-j", "MARK", "--set-mark",MARK_CE_ROUTER);
					/* ip -6 rule add fwmark 0x0007c000/0x0007c000 table 1350
					 * ip -6 route add table 1350 default dev br0 */
					va_cmd(BIN_IP, 7, 1, "-6", "rule", "add", "fwmark", MARK_CE_ROUTER, "table", CE_ROUTER_TABLE_ID);
					va_cmd(BIN_IP, 8, 1, "-6", "route", "add", "table", CE_ROUTER_TABLE_ID, "default", "dev", LANIF);
					
					/*FORWARDING RFC7083 Test CERouter 3.2.3 : Forwarding Loops: 6.Prevent Forwarding Loop
					 *invalid dst ipv6 is 3ffe:501:ffff:111f::aaaa
					 * For WAN side, spec not allow WAN side packet forward to LAN side with PD which is not set from our router.
					 */
					va_cmd(IP6TABLES, 15, 1, (char *)FW_ADD, (char *)FW_IPV6_CE_ROUTER_FILTER, "-i", ifname, "-p", "icmpv6", "--icmpv6-type", "echo-request", "!", "-d", prefix_ipv6_str, 
						"-j",(char *)FW_REJECT, "--reject-with", "icmp6-no-route");
					/* For LAN Side, because WAN DHCP may give us PD length < 64 to let us separate muti-LAN prefix for brX.
					 * And we can't reject the packet to google.com etc. So we need to accept the packet with
					 * DIP's PD is assigned from our router and Reject the packet only include the DIP's pd with other prefix length which is
					 * gotten from WAN DHCP Server.  [RFC7083 Section WPD-5]
					 */
					va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, (char *)FW_IPV6_CE_ROUTER_FILTER, "-i", LANIF, "-p", "icmpv6", "--icmpv6-type", "echo-request", "-d", prefix_str_plus_pdlen, 
						"-j", (char *)FW_REJECT, "--reject-with", "icmp6-no-route");

				}
			}
		}
#endif /* DHCPV6_ISC_DHCP_4XX */
	}
	
	va_cmd(IP6TABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", (char *)FW_IPV6_CE_ROUTER_FILTER);
	va_cmd(IP6TABLES, 6, 1,"-t", "mangle", (char *)FW_INSERT, (char *)FW_FORWARD, "-j", (char *)FW_IPV6_CE_ROUTER_MANGLE);

	return 0;
}
#endif /* CONFIG_RTK_IPV6_LOGO_TEST */

int pre_setupIPV6Filter(void)
{
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)FW_IPV6FILTER);
	return 0;
}

int post_setupIPV6Filter(void)
{
	// Set default action for ipv6filter
	unsigned char value;
	unsigned char v6_session_fw_enable = 1;
#if defined(CONFIG_E8B) ||defined(CONFIG_00R0)
	unsigned char ipportfilter_state;

	mib_get_s(MIB_IPFILTER_ON_OFF, (void*)&ipportfilter_state, sizeof(ipportfilter_state));

	if(ipportfilter_state == 0) return 0;
#endif

	// Kill all conntrack (to kill the established conntrack when change ip6tables rules)
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	system("/bin/echo flush > /proc/fc/ctrl/rstconntrack");
#else
#ifdef CONFIG_USER_CONNTRACK_TOOLS
	va_cmd(CONNTRACK_TOOL, 3, 1, "-D", "-f", "ipv6");	// v1.4.6 support family "ipv6"
#else
	va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
#endif
#endif

	// accept related
	// ip6tables -A ipv6filter -m state --state ESTABLISHED,RELATED -j RETURN
#if defined(CONFIG_00R0)
	char ipfilter_spi_state=1;
	mib_get_s(MIB_IPFILTER_SPI_ENABLE, (void*)&ipfilter_spi_state, sizeof(ipfilter_spi_state));
	if(ipfilter_spi_state != 0)
#endif
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_IPV6FILTER, "-m", "state",
			"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_RETURN);

	if (mib_get_s(MIB_V6_IPF_OUT_ACTION, (void *)&value, sizeof(value)) != 0)
	{
		if (value == 0)	// DROP
		{
			// ip6tables -A ipv6filter -i $LAN_IF -j DROP
			va_cmd(IP6TABLES, 6, 1, (char *)FW_ADD,
					(char *)FW_IPV6FILTER, (char *)ARG_I,
					(char *)LANIF, "-j", (char *)FW_DROP);
		}
	}

	mib_get_s(MIB_V6_SESSION_FW_ENABLE, (void *)&v6_session_fw_enable, sizeof(v6_session_fw_enable));
	if (mib_get_s(MIB_V6_IPF_IN_ACTION, (void *)&value, sizeof(value)) != 0)
	{
		if (value == 0 && v6_session_fw_enable)	// DROP
		{
			// ip6tables -A ipv6filter ! -i $LANIF_ALL -j DROP
			// Because SFU mode (PPTP WAN), we will generate br1, brX ... so we should use br+ to avoid kernel DROP
			// all IPv6 packet for ip6tables FORWARD. In bridge wan, packet still will pass ip6tables FORWARD chain. 20201104
			va_cmd(IP6TABLES, 7, 1, (char *)FW_ADD, (char *)FW_IPV6FILTER, "!", (char *)ARG_I, (char *)LANIF_ALL, "-j", (char *)FW_DROP);
		}
	}

	return 1;
}
#ifdef CONFIG_USER_RTK_IPV6_SIMPLE_SECURITY
 int clear_simpleSec_firewall(void)
 {
	unsigned char cmdbuf[128]={0};

	va_cmd(IP6TABLES, 4, 1, (char *)FW_DEL, (char *)FW_IPV6FILTER, "-j", (char *)FW_IPV6_SIMPLE_SECURITY_FILTER);
	va_cmd(IP6TABLES, 2, 1, (char *)FW_FLUSH, (char *)FW_IPV6_SIMPLE_SECURITY_FILTER);
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_IPV6_SIMPLE_SECURITY_FILTER);

	snprintf(cmdbuf,sizeof(cmdbuf),"echo 30 > /proc/sys/net/netfilter/nf_conntrack_udp_timeout");
	va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);

	return 0;
}
int setup_simpleSec_firewall(void)
{
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ];
	int vcTotal, i;
	PREFIX_V6_INFO_T prefixInfo;
	char prefix_ipv6_str[64] = {0};
	char prefix_str_plus_pdlen[64] = {0};
	unsigned char simpleSec_enable =0;
	unsigned char cmdbuf[128]={0};

	if (!mib_get_s( MIB_V6_SIMPLE_SEC_ENABLE, (void *)&simpleSec_enable, sizeof(simpleSec_enable)) )
		return -1;

	if (simpleSec_enable){
		va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_IPV6_SIMPLE_SECURITY_FILTER);

		/*RFC 6092 REC-7
		 *By DEFAULT, packets with unique local source and/or destination addresses [RFC4193] SHOULD NOT be forwarded to or from the exterior network.*/
		va_cmd(IP6TABLES, 6, 1, (char *)FW_ADD, (char *)FW_IPV6_SIMPLE_SECURITY_FILTER,  "-s", "fc00::/7", "-j", (char *)FW_DROP);
		va_cmd(IP6TABLES, 6, 1, (char *)FW_ADD, (char *)FW_IPV6_SIMPLE_SECURITY_FILTER,  "-d", "fc00::/7", "-j", (char *)FW_DROP);

		if (0 == get_prefixv6_info(&prefixInfo))
		{
			inet_ntop(PF_INET6, (struct in6_addr *)prefixInfo.prefixIP, prefix_ipv6_str, sizeof(prefix_ipv6_str));
			snprintf(prefix_str_plus_pdlen, sizeof(prefix_str_plus_pdlen), "%s/%d", prefix_ipv6_str,prefixInfo.prefixLen);

			/* RFC 6092 REC-5
			 * Outbound packets MUST NOT be forwarded if the source address in their outer IPv6 header does not have a unicast prefix configured for use by globally reachable nodes on the interior network.
			 */
			va_cmd(IP6TABLES, 9, 1, (char *)FW_ADD, (char *)FW_IPV6_SIMPLE_SECURITY_FILTER, "!", "-s", prefix_str_plus_pdlen, "-i", LANIF, "-j",(char *)FW_DROP);

			/* RFC 6092 REC-6
			 * Inbound packets MUST NOT be forwarded if the source address in their outer IPv6 header has a global unicast prefix assigned for use by globally reachable nodes on the interior network
			 */
			vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
			for (i = 0; i < vcTotal; i++)
			{
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
					continue;
				if(0 != ifGetName(Entry.ifIndex, ifname, sizeof(ifname))){
					va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_IPV6_SIMPLE_SECURITY_FILTER, "-s", prefix_str_plus_pdlen, "-i", ifname, "-j",(char *)FW_DROP);
				}
			}
		}

		/* RFC 6092 REC-10/REC-18/REC-23/REC-36
		 * IPv6 gateways SHOULD NOT forward ICMPv6 "Destination Unreachable" and "Packet Too Big" messages containing IP headers
		 * that do not match generic upper-layer/TCP/UDP/ESP transport state records.
		 * ip6tables -A ipv6filter -p icmpv6 --icmpv6-type destination-unreachable -m state --state INVALID -j DROP
		 * ip6tables -A ipv6filter -p icmpv6 --icmpv6-type packet-too-big -m state --state INVALID -j DROP
		 */
		va_cmd(IP6TABLES, 12, 1, (char *)FW_ADD, (char *)FW_IPV6_SIMPLE_SECURITY_FILTER,
			"-p", "icmpv6", "--icmpv6-type","destination-unreachable", "-m", "state",	"--state", "INVALID","-j", (char *)FW_DROP);

		va_cmd(IP6TABLES, 12, 1, (char *)FW_ADD, (char *)FW_IPV6_SIMPLE_SECURITY_FILTER,
			"-p", "icmpv6", "--icmpv6-type","packet-too-big", "-m", "state",	"--state", "INVALID","-j", (char *)FW_DROP);

		va_cmd(IP6TABLES, 4, 1, (char *)FW_INSERT, (char *)FW_IPV6FILTER, "-j", (char *)FW_IPV6_SIMPLE_SECURITY_FILTER);

		/* RFC 6092 REC-14
		 * A state record for a UDP flow where both source and destination ports are outside the well-known port range
		 * (ports 0-1023) MUST NOT expire in less than two minutes of idle time.The value of the UDP state record idle timer MAY be configurable.
		 * The DEFAULT is five minutes.
		 */
		snprintf(cmdbuf,sizeof(cmdbuf),"echo 300 > /proc/sys/net/netfilter/nf_conntrack_udp_timeout");
		va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
	}

	return 0;
}
#endif

int restart_IPV6Filter(void)
{
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	unsigned char logoTestMode = 0 ;
#endif
	printf("Restart IPv6 Filter!\n");
	pre_setupIPV6Filter();

#ifndef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
	setupIPV6Filter();
#else
	setupIPV6FilterMixed();
#endif

#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	if ( mib_get_s(MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode))){
		if(logoTestMode == 3){ // CE_ROUTER mode
			clear_CERouter_firewall();
			setup_CERouter_firewall();
		}
	}
#endif
#ifdef CONFIG_USER_RTK_IPV6_SIMPLE_SECURITY
	clear_simpleSec_firewall();
	setup_simpleSec_firewall();
#endif
	post_setupIPV6Filter();
	return 1;
}

/*	we send usock to systemd to schedule restart lan ipv6 server
 *	it support restart/clean now. */
void restartLanV6Server(void)
{
	va_cmd("/bin/sysconf", 4, 0, "send_unix_sock_message", SYSTEMD_USOCK_SERVER, "do_LanV6Server", "restart");
}

void cleanLanV6Server(void)
{
	va_cmd("/bin/sysconf", 4, 0, "send_unix_sock_message", SYSTEMD_USOCK_SERVER, "do_LanV6Server", "clean");
}

//Helper function for PrefixV6 for RADVD mode
/* Get LAN-side Prefix information for IPv6 servers.
 * 0 : successful
 * -1: failed
 */
int get_radvd_prefixv6_info(PREFIX_V6_INFO_Tp prefixInfo)
{
	unsigned char ipv6RadvdPrefixMode=0;
	unsigned char tmpBuf[100]={0}, prefixLen[15], ifname[IFNAMSIZ];
	unsigned char leasefile[64];
	unsigned int wanconn=0;
	DLG_INFO_T dlgInfo={0};
	MIB_CE_ATM_VC_T Entry;
	char staticPrefix[INET6_ADDRSTRLEN]={0};

	if(!prefixInfo){
		printf("Error! NULL input prefixV6Info\n");
		goto setErr_ipv6;
	}
	if ( !mib_get_s(MIB_V6_RADVD_PREFIX_MODE, (void *)&ipv6RadvdPrefixMode, sizeof(ipv6RadvdPrefixMode))) {
		printf("Error!! get LAN IPv6 Prefix Mode fail!");
		goto setErr_ipv6;
	}

	prefixInfo->mode = ipv6RadvdPrefixMode;

	switch(prefixInfo->mode){
		case RADVD_MODE_AUTO:
			wanconn = get_pd_wan_delegated_mode_ifindex();
			if (wanconn == 0xFFFFFFFF) {
				printf("Error!! %s get WAN Delegated PREFIXINFO fail!", __FUNCTION__);
				goto setErr_ipv6;
			}
			// Due to ATM's ifIndex can be 0
			prefixInfo->wanconn = wanconn;

			//printf("wanconn is %d=0x%x\n",wanconn,wanconn);
			if (prefixInfo->wanconn == DUMMY_IFINDEX) { // auto mode, get lease from most-recently leasefile.
				if (access("/var/prefix_info", F_OK) == -1)
					goto setErr_ipv6;
				getLeasesInfo("/var/prefix_info", &dlgInfo);
				snprintf(leasefile, 64, "%s.%s", PATH_DHCLIENT6_LEASE, dlgInfo.wanIfname);
				prefixInfo->wanconn = getIfIndexByName(dlgInfo.wanIfname);
			}
			
			if (get_prefixv6_info_by_wan(prefixInfo, prefixInfo->wanconn)==0){
				//prefer/valid lifetime from dhcpv6 client lease file has highest priority,otherwise,use RADVD static value
				if(!strstr(prefixInfo->leaseFile, PATH_DHCLIENT6_LEASE)){
					mib_get_s(MIB_V6_PREFERREDLIFETIME, (void *)&tmpBuf, sizeof(tmpBuf));
					prefixInfo->PLTime = (unsigned int)strtoul(tmpBuf, NULL, 10);
					mib_get_s(MIB_V6_VALIDLIFETIME, (void *)&tmpBuf, sizeof(tmpBuf));
					prefixInfo->MLTime = (unsigned int)strtoul(tmpBuf, NULL, 10);
				}
			}else{
					//AUG_PRT("Error! Could not get delegation info from wanconn %s, file %s!\n", tmpBuf, leasefile);
					goto setErr_ipv6;
			}
			break;
		case RADVD_MODE_MANUAL:
			if (!mib_get_s(MIB_V6_PREFIX_IP, (void *)tmpBuf, sizeof(tmpBuf))) { //STRING_T
				printf("Error!! Get MIB_V6_PREFIX_IP fail!");
				goto setErr_ipv6;
			}

			if (!mib_get_s(MIB_V6_PREFIX_LEN, (void *)prefixLen, sizeof(prefixLen))) { ////STRING_T
				printf("Error!! Get MIB_V6_PREFIX_LEN fail!");
				goto setErr_ipv6;
			}
			if(tmpBuf[0]){
				if ( !inet_pton(PF_INET6, tmpBuf, prefixInfo->prefixIP) )
					goto setErr_ipv6;
			}
			// AdvValidLifetime
			if ( !mib_get_s(MIB_V6_VALIDLIFETIME, (void *)tmpBuf, sizeof(tmpBuf))) {
				printf("Get AdvValidLifetime mib error!");
				goto setErr_ipv6;
			}

			if(tmpBuf[0])
				prefixInfo->MLTime = (unsigned int)strtoul(tmpBuf, NULL, 10);

			// AdvPreferredLifetime
			if ( !mib_get_s(MIB_V6_PREFERREDLIFETIME, (void *)tmpBuf, sizeof(tmpBuf))) {
				printf("Get AdvPreferredLifetime mib error!");
				goto setErr_ipv6;
			}
			if(tmpBuf[0])
				prefixInfo->PLTime = (unsigned int)strtoul(tmpBuf, NULL, 10);
			prefixInfo->prefixLen = atoi(prefixLen);
			
			break;
		default:
			printf("Error! Not support this mode %d!\n", prefixInfo->mode);
	}
	return 0;

setErr_ipv6:
	return -1;
}

int get_radvd_dnsv6_info(DNS_V6_INFO_Tp pDnsV6Info)
{
	unsigned char RadvdRDNSSMode=0;
	unsigned char tmpBuf[100]={0},dnsv6_1[64]={0},dnsv6_2[64]={0} ;
	unsigned char value[64];
	struct in6_addr targetIp;
	unsigned char devAddr[MAC_ADDR_LEN];
	unsigned char *p = &targetIp.s6_addr[0];
	unsigned char leasefile[64];
	unsigned int wanconn=0;
	DLG_INFO_T dlgInfo={0};
	MIB_CE_ATM_VC_T Entry;
	int prefixReady=0;


	// If RADVD is Manual mode, save the static RDDNS from MIB.
	if ( !mib_get_s(MIB_V6_RADVD_RDNSS_MODE, (void *)&RadvdRDNSSMode, sizeof(RadvdRDNSSMode))) {
		printf("Error!! get LAN IPv6 RDNSS Mode fail!");
		return -1;
	}
	pDnsV6Info->mode = RadvdRDNSSMode;

	switch(pDnsV6Info->mode)
	{
		case RADVD_RDNSS_HGWPROXY:
			if (!mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)tmpBuf, sizeof(tmpBuf))) {
				printf("Error!! Get LAN IPv6 Address fail!");
				goto setErr_ipv6;
			}
			if(tmpBuf[0])
				strcpy(pDnsV6Info->nameServer,tmpBuf);
			else {
				if (mib_get_s(MIB_ELAN_MAC_ADDR, (void *)devAddr, sizeof(devAddr)) != 0)
				{
					ipv6_linklocal_eui64(p, devAddr);
					inet_ntop(PF_INET6, &targetIp, value, sizeof(value));
					strncpy(pDnsV6Info->nameServer,value, sizeof(pDnsV6Info->nameServer));
				}
			}
			//printf("%s: RADVD_RDNSS_HGWPROXY,with nameServer %s\n", __FUNCTION__, pDnsV6Info->nameServer);
			break;
		case RADVD_RDNSS_WANCON:
			wanconn = get_radvd_rdnss_wan_delegated_mode_ifindex();
			if (wanconn == 0xFFFFFFFF) {
				printf("Error!! %s get WAN Delegated RDNSS fail!", __FUNCTION__);
				goto setErr_ipv6;
			}

			pDnsV6Info->wanconn = wanconn;

			//printf("wanconn is %d=0x%x\n",wanconn,wanconn);
			if (pDnsV6Info->wanconn == DUMMY_IFINDEX) { // auto mode, get lease from most-recently leasefile.
				if (access("/var/prefix_info", F_OK) == -1)
					goto setErr_ipv6;
				prefixReady = getLeasesInfo("/var/prefix_info", &dlgInfo);
				if (prefixReady & LEASE_GET_IFNAME)
					strncpy(tmpBuf, dlgInfo.wanIfname, 64);
			}
			else { // get lease of selected wan
				ifGetName(pDnsV6Info->wanconn, tmpBuf, sizeof(tmpBuf));
			}
			snprintf(leasefile, 64, "%s.%s", PATH_DHCLIENT6_LEASE, tmpBuf);
			if(!getATMVCEntryByIfName(tmpBuf, &Entry)){
				printf("Find ATM_VC_TBL interface %s Fail!\n",tmpBuf);
				goto setErr_ipv6;
			}

			if (get_dnsv6_info_by_wan(pDnsV6Info, Entry.ifIndex) == -1)
				goto setErr_ipv6;
			break;
		case RADVD_RDNSS_STATIC:
			if (!mib_get(MIB_V6_RDNSS1, (void *)dnsv6_1)) {
				printf("Error!! Get DNS Server Address 1 fail!");
				return -1;
			}

			if (!mib_get(MIB_V6_RDNSS2, (void *)dnsv6_2)) {
				printf("Error!! Get DNS Server Address 2 fail!");
				return -1;
			}

			if(dnsv6_1[0]&&dnsv6_2[0])
				snprintf(pDnsV6Info->nameServer,IPV6_BUF_SIZE_256,"%s,%s",dnsv6_1,dnsv6_2);
			else if(dnsv6_1[0])
				snprintf(pDnsV6Info->nameServer,IPV6_BUF_SIZE_256,"%s",dnsv6_1);

			printf("RADVD_RDNSS_STATIC,with nameServer %s\n",pDnsV6Info->nameServer);
			break;

		default:
			printf("%s Error! Not support this mode %d!\n", __FUNCTION__, pDnsV6Info->mode);
			goto setErr_ipv6;
	}

	return 0;

setErr_ipv6:
	return -1;
}

//Helper function for PrefixV6 mode
/* Get LAN-side Prefix information for IPv6 servers.
 * 0 : successful
 * -1: failed
 */
int get_prefixv6_info(PREFIX_V6_INFO_Tp prefixInfo)
{
	unsigned char ipv6PrefixMode=0, prefixLen;
	unsigned char tmpBuf[100]={0};
	unsigned char leasefile[64];
	unsigned int wanconn = DUMMY_IFINDEX;
	DLG_INFO_T dlgInfo={0};
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ] = {0};
	char staticPrefix[INET6_ADDRSTRLEN]={0};
	unsigned int PFTime = 0, RNTime = 0, RBTime = 0, MLTime = 0;
	unsigned char dhcpv6_type = 0, dhcpv6_mode=0;
	unsigned char ra_file[64]={0};
	
	if(!prefixInfo){
		printf("Error! NULL input prefixV6Info\n");
		goto setErr_ipv6;
	}

	/* E8B Web use this MIB to decide if prefix should use WAN Delegation or Static prefix on WEB.
	   But ISC-DHCPv6 server can't work if br0 have mutilple global IPv6 address, we need to avoid
	   static dhcpv6 server and radvd prefix setting br0 global IP when prefix is WAN Delegation mode.
	   TODO: Need hacking ISC-DHCPv6 to accept multiple br0 global IPv6 address. 20201228 */
	if ( !mib_get_s(MIB_PREFIXINFO_PREFIX_MODE, (void *)&ipv6PrefixMode, sizeof(ipv6PrefixMode))) {
		printf("Error!! get LAN IPv6 Prefix Mode fail!");
		goto setErr_ipv6;
	}
	if (!mib_get_s(MIB_DHCPV6_MODE, (void *)&dhcpv6_mode, sizeof(dhcpv6_mode))){
		printf("[%s:%d] Error!! Get LAN DHCPV6 server Type failed\n",__func__,__LINE__);
		goto setErr_ipv6;
	}
	if (!mib_get_s(MIB_DHCPV6S_TYPE, (void *)&dhcpv6_type, sizeof(dhcpv6_type))){
		printf("[%s:%d] Error!! Get LAN  DHCPv6 server  Type failed\n",__func__,__LINE__);
		goto setErr_ipv6;
	}

	prefixInfo->mode = ipv6PrefixMode;
	switch(prefixInfo->mode){
		case IPV6_PREFIX_DELEGATION:
			wanconn = get_pd_wan_delegated_mode_ifindex();
			if (wanconn == 0xFFFFFFFF) {
				printf("Error!! %s get WAN Delegated PREFIXINFO fail!", __FUNCTION__);
				goto setErr_ipv6;
			}
			// Due to ATM's ifIndex can be 0
			prefixInfo->wanconn = wanconn;

			//printf("wanconn is %d=0x%x\n",wanconn,wanconn);
			if (prefixInfo->wanconn == DUMMY_IFINDEX) { // auto mode, get lease from most-recently leasefile.
				if (access("/var/prefix_info", F_OK) == -1)
					goto setErr_ipv6;
				getLeasesInfo("/var/prefix_info", &dlgInfo);
				prefixInfo->wanconn = getIfIndexByName(dlgInfo.wanIfname);
			}


			if (get_prefixv6_info_by_wan(prefixInfo, prefixInfo->wanconn)!=0){
				//AUG_PRT("Error! Could not get delegation info from wanconn %d\n", prefixInfo->wanconn);
				goto setErr_ipv6;
			}

			break;
		case IPV6_PREFIX_STATIC:
			if (!mib_get_s(MIB_IPV6_LAN_PREFIX, (void *)tmpBuf, sizeof(tmpBuf))) { //STRING_T
				printf("Error!! Get MIB_IPV6_LAN_PREFIX fail!");
				goto setErr_ipv6;
			}

			if (!mib_get_s(MIB_IPV6_LAN_PREFIX_LEN, (void *)&prefixLen, sizeof(prefixLen))) {
				printf("Error!! Get MIB_IPV6_LAN_PREFIX_LEN fail!");
				goto setErr_ipv6;
			}
			if(tmpBuf[0]){
				if ( !inet_pton(PF_INET6, tmpBuf, prefixInfo->prefixIP) )
					goto setErr_ipv6;
			}
			prefixInfo->prefixLen = prefixLen;
			// default-lease-time
			mib_get_s(MIB_DHCPV6S_DEFAULT_LEASE, (void *)&MLTime, sizeof(MLTime));
			prefixInfo->MLTime = MLTime;
			// preferred-lifetime
			mib_get_s(MIB_DHCPV6S_PREFERRED_LIFETIME, (void *)&PFTime, sizeof(PFTime));
			prefixInfo->PLTime = PFTime;
			mib_get_s(MIB_DHCPV6S_RENEW_TIME, (void *)&RNTime, sizeof(RNTime));
			prefixInfo->RNTime = RNTime;
			mib_get_s(MIB_DHCPV6S_REBIND_TIME, (void *)&RBTime, sizeof(RBTime));
			prefixInfo->RBTime = RBTime;


			break;
		case IPV6_PREFIX_PPPOE:
		case IPV6_PREFIX_NONE:
		default:
			printf("Error! Not support this mode %d!\n", prefixInfo->mode);
	}
	return 0;

setErr_ipv6:
	return -1;
}

/* Get DNSv6 information from wan interface by ifIndex
 * ex. /var/resolv6.conf.nas0_0
 */
int get_dnsv6_info_by_wan(DNS_V6_INFO_Tp dnsV6Info, unsigned int ifindex)
{
	unsigned char dns6file[64];
	char ifname[IFNAMSIZ];
	char str_addr1[128] = {0};
	char str_addr2[128] = {0};
	char line[128] = {0};
	FILE* infdns;
	MIB_CE_ATM_VC_T Entry;
	int first_dns;
	char lease_file[128] = {0};
	DLG_INFO_T dlgInfo = {0};
	int i = 0;

	if(!dnsV6Info){
		printf("Error! NULL input dnsV6Info\n");
		goto setErr_ipv6;
	}

	if (!getATMVCEntryByIfIndex(ifindex, &Entry))
		goto setErr_ipv6;
	if ((Entry.IpProtocol & IPVER_IPV6) == 0)
		goto setErr_ipv6;

	ifGetName(Entry.ifIndex, ifname, sizeof(ifname));

	/* get wan dnsv6 file */
	/* Now we all use DNS6_RESOLV file to get DNS info with dynamic and static dns. so only get DNS from DNS6_RESOLV */
	snprintf(dns6file, 64, "%s.%s", (char *)DNS6_RESOLV, ifname);

	infdns = fopen(dns6file,"r");
	if (!infdns) goto setErr_ipv6;

	/* Put wan dns information into dnsV6Info */
	dnsV6Info->mode = Entry.dnsv6Mode;
	dnsV6Info->wanconn = Entry.ifIndex;
	snprintf(lease_file, sizeof(lease_file), "%s.%s", PATH_DHCLIENT6_LEASE, ifname);
	strncpy(dnsV6Info->leaseFile, lease_file, sizeof(dnsV6Info->leaseFile));

	// We need to get DNSSL info from lease file, not DNS info.
	if (getLeasesInfo(lease_file, &dlgInfo) & LEASE_GET_DNS)
	{
		// DNSSL from DHCPv6 server
		dnsV6Info->dnssl_num = dlgInfo.dnssl_num;
		for (i = 0; i < dlgInfo.dnssl_num; i++)
			snprintf(dnsV6Info->domainSearch[i], MAX_DOMAIN_LENGTH, "%s", dlgInfo.domainSearch[i]);
	}

	first_dns = 1;
	while (fgets(line, sizeof(line), infdns) != NULL) {
		char *new_line = NULL;

		new_line = strrchr(line, '\n');
		if(new_line)
			*new_line = '\0';

		if((strlen(line) == 0))
			continue;

		if (first_dns) {
			strncpy(dnsV6Info->nameServer, line, IPV6_BUF_SIZE_256);
			first_dns = 0;
		}
		else
			snprintf(dnsV6Info->nameServer+strlen(dnsV6Info->nameServer), IPV6_BUF_SIZE_256-strlen(dnsV6Info->nameServer), ",%s", line);
	}
	fclose(infdns);
	//printf("%s: mode=%d, wanconn=%d\n", __FUNCTION__, dnsV6Info->mode, dnsV6Info->wanconn);
	//printf("%s: dnsFile=%s, dns=%s\n", __FUNCTION__, dnsV6Info->leaseFile, dnsV6Info->nameServer);

	return 0;

setErr_ipv6:
	return -1;
}

/*
 * get_prefixv6_info_by_wan(): Get Prefixv6 information from wan interface by ifIndex
 *  Ex. 1.Static from pEntry->Ipv6Prefix.
 *	   2.RA delegation wan from RA info
 *	   3.6rd wan from 6rdPrefix
 *      4.6to4 wan from 6to4 prefix (2002:<ipv4 address>)
 *      5.PD wan from DHCPv6 lease file (PATH_DHCLIENT6_LEASE.xxx)
 */
int get_prefixv6_info_by_wan(PREFIX_V6_INFO_Tp prefixInfo, int ifindex)
{
	unsigned char ipv6PrefixMode = 0, prefixLen = {0};
	unsigned char leasefile[64] = {0};
	char ifname[IFNAMSIZ] = {0};
	DLG_INFO_T dlgInfo = {0};
	MIB_CE_ATM_VC_T Entry;
	int entry_index = 0;
	char staticPrefix[INET6_ADDRSTRLEN] = {0};
	unsigned int PFTime = 0, RNTime = 0, RBTime = 0, MLTime = 0;
	unsigned char dhcpv6_type = 0, dhcpv6_mode=0;
	unsigned char ra_file[64]={0};
	

	if (!prefixInfo) {
		printf("Error! NULL input prefixV6Info\n");
		goto setErr_ipv6;
	}

	ifGetName(ifindex, ifname, sizeof(ifname));
	if (ifname[0] == '\0')
	{
		printf("%s: get ifname failed\n", __FUNCTION__);
		goto setErr_ipv6;
	}

	if (!(getATMVCEntryByIfName(ifname, &Entry)))
	{
		printf("Error! Could not get MIB entry info from wanconn %s!\n", ifname);
		goto setErr_ipv6;
	}

	if ((Entry.IpProtocol & IPVER_IPV6) == 0
#ifdef CONFIG_IPV6_SIT_6RD
		&& (Entry.v6TunnelType != V6TUNNEL_6RD)
#endif
#ifdef CONFIG_IPV6_SIT
		&& (Entry.v6TunnelType != V6TUNNEL_6IN4 && Entry.v6TunnelType != V6TUNNEL_6TO4)
#endif
		)
		goto setErr_ipv6;
	
	if (!mib_get_s(MIB_DHCPV6_MODE, (void *)&dhcpv6_mode, sizeof(dhcpv6_mode))){
		printf("[%s:%d] Error!! Get LAN DHCPV6 server Type failed\n",__func__,__LINE__);
		goto setErr_ipv6;
	}
	if (!mib_get_s(MIB_DHCPV6S_TYPE, (void *)&dhcpv6_type, sizeof(dhcpv6_type))){
		printf("[%s:%d] Error!! Get LAN  DHCPv6 server  Type failed\n",__func__,__LINE__);
		goto setErr_ipv6;
	}

	/* Put wan ifindex info into prefixInfo */
	prefixInfo->wanconn = ifindex;

	if (Entry.AddrMode & IPV6_WAN_STATIC) //this wan is static IPv6 WAN
	{
		inet_ntop(AF_INET6, (const void *)&Entry.Ipv6Prefix, staticPrefix, INET6_ADDRSTRLEN);
		if (strcmp(staticPrefix,"::") && Entry.Ipv6PrefixLen) {
			prefixInfo->prefixLen =  Entry.Ipv6PrefixLen;
			/* We don't let WAN static prefix to be != 64 now*/
			if(prefixInfo->prefixLen != 64)
				prefixInfo->prefixLen = 64;
			ip6toPrefix(Entry.Ipv6Prefix, prefixInfo->prefixLen, prefixInfo->prefixIP);

			strncpy(prefixInfo->leaseFile, "staticWan", IPV6_BUF_SIZE_128);
			if (!mib_get_s(MIB_DHCPV6S_RENEW_TIME, (void *)&RNTime, sizeof(RNTime))){
				printf("Get MIB_DHCPV6S_RENEW_TIME mib error!\n");
				goto setErr_ipv6;
			}
			prefixInfo->RNTime = RNTime;
			if (!mib_get_s(MIB_DHCPV6S_REBIND_TIME, (void *)&RBTime, sizeof(RBTime))) {
				printf("Get MIB_DHCPV6S_REBIND_TIME mib error!\n");
				goto setErr_ipv6;
			}
			prefixInfo->RBTime = RBTime;
			/* If you want to get RADVD static MIB(like MIB_V6_XXX), you should add new function for
			 * get_radvd_prefixv6_info_by_wan or add a parameter to this function to tell we want to get
			 * dhcpv6s or radvd static MIB.
			 * Now default is get dhcpv6s static MIB. */
			// default-lease-time
			mib_get_s(MIB_DHCPV6S_DEFAULT_LEASE, (void *)&MLTime, sizeof(MLTime));
			prefixInfo->MLTime = MLTime;
			// preferred-lifetime
			mib_get_s(MIB_DHCPV6S_PREFERRED_LIFETIME, (void *)&PFTime, sizeof(PFTime));
			prefixInfo->PLTime = PFTime;

		}
		else
		{
			printf("[%s %d]Error! Could not get delegation info from wanconn %s, staticPrefix %s!\n", __func__, __LINE__, ifname,  staticPrefix);
			goto setErr_ipv6;
		}

	}
#if defined(CONFIG_USER_RTK_RA_DELEGATION) && defined(CONFIG_USER_RTK_RAMONITOR)
	else if ((Entry.AddrMode & IPV6_WAN_AUTO)&&(!(Entry.Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)) 
			&&(dhcpv6_mode == DHCP_LAN_SERVER)&&(dhcpv6_type == DHCPV6S_TYPE_RA_DELEGATION)){
		//get prefix info form /var/rainfo_<ifname>
		unsigned char ra_file[64] = {0};
		RAINFO_V6_T ra_v6Info;
		unsigned char devAddr[MAC_ADDR_LEN];
		int i=0;

		memset(&ra_v6Info, 0, sizeof(ra_v6Info));
		snprintf(ra_file, sizeof(ra_file), "%s_%s", (char *)RA_INFO_FILE, ifname);
		if (get_ra_info(ra_file,  &ra_v6Info)&RAINFO_GET_PREFIX){
			
			inet_pton(PF_INET6, ra_v6Info.prefix_1, prefixInfo->prefixIP);
			//RA delegation prefix format: 
			// RA prefix(64 bits) +  MAC(48bit) + 0(8bits) =120 bits
			//Maybe we need refine this to make it configurable  in future
			prefixInfo->prefixLen = 120;
			mib_get_s(MIB_ELAN_MAC_ADDR, (void *)devAddr, sizeof(devAddr));
			for (i=0;i<MAC_ADDR_LEN;i++)
				 prefixInfo->prefixIP[8+i] = devAddr[i];
		}
		else
		{
			printf("[%s %d]Error! Could not get delegation info from %s!\n", __func__, __LINE__, ra_file );
			goto setErr_ipv6;
		}
	}
#endif
#ifdef CONFIG_IPV6_SIT_6RD
	else if (Entry.IpProtocol == IPVER_IPV4 && Entry.v6TunnelType ==V6TUNNEL_6RD){
		struct in6_addr targetIp;
		struct in_addr inAddr;
		unsigned char SixrdBRv4IP[INET_ADDRSTRLEN];
		unsigned char Ipv6AddrStr[INET6_ADDRSTRLEN];

		if( (Entry.SixrdPrefixLen+(32-Entry.SixrdIPv4MaskLen)) >64){
			printf("Invalid 6RD setting, PrefixLen and IPv4 address > 64!!! Please check the setting!\n");
			goto setErr_ipv6;
		}

		//ipv4 dhcp need use dynamic address
		if (Entry.ipDhcp == (unsigned char) DHCP_CLIENT || Entry.cmode == CHANNEL_MODE_PPPOE){
			if (getInAddr(ifname, IP_ADDR, &inAddr) == 1){
				memcpy((struct in_addr *)Entry.ipAddr, &inAddr, sizeof(inAddr));
			}
			else{
				printf("Can not get dhcp ipv4 address from wan %s\n",ifname);
			 	goto setErr_ipv6;
			}
		}

		if (inet_ntop(PF_INET, (struct in_addr *)Entry.SixrdBRv4IP, SixrdBRv4IP, sizeof(SixrdBRv4IP)) == NULL ) {
			printf("Invalid 6RD setting, BRv4IP is invalid !!! Please check the setting!\n");
			goto setErr_ipv6;
		}
		if (inet_ntop(PF_INET6, Entry.SixrdPrefix, Ipv6AddrStr, sizeof(Ipv6AddrStr)) == NULL ) {
			printf("Invalid 6RD setting, Prefix is invalid !!! Please check the setting!\n");
			goto setErr_ipv6;
		}
		if (Entry.SixrdIPv4MaskLen > 32 || Entry.SixrdIPv4MaskLen < 0 ) {
			printf("Invalid 6RD setting, IPv4MaskLen is invalid !! Please check the setting!\n");
			goto setErr_ipv6;
		}
		if (Entry.SixrdPrefixLen > 64 || Entry.SixrdPrefixLen <= 0 ) {
			printf("Invalid 6RD setting, PrefixLen is invalid !!! Please check the setting!\n");
			goto setErr_ipv6;
		}

		make6RD_prefix(&Entry,staticPrefix,sizeof(staticPrefix));
		inet_pton(PF_INET6, staticPrefix, &targetIp);
		ip6toPrefix(&targetIp, 64, prefixInfo->prefixIP);
		prefixInfo->prefixLen = 64;
		strncpy(prefixInfo->leaseFile, "6rdWAN", IPV6_BUF_SIZE_128);
		if (!mib_get_s(MIB_DHCPV6S_RENEW_TIME, (void *)&RNTime, sizeof(RNTime))){
			printf("Get MIB_DHCPV6S_RENEW_TIME mib error!\n");
			goto setErr_ipv6;
		}
		prefixInfo->RNTime = RNTime;
		if (!mib_get_s(MIB_DHCPV6S_REBIND_TIME, (void *)&RBTime, sizeof(RBTime))) {
			printf("Get MIB_DHCPV6S_REBIND_TIME mib error!\n");
			goto setErr_ipv6;
		}
		prefixInfo->RBTime = RBTime;
		/* If you want to get RADVD static MIB(like MIB_V6_XXX), you should add new function for
		 * get_radvd_prefixv6_info_by_wan or add a parameter to this function to tell we want to get
		 * dhcpv6s or radvd static MIB.
		 * Now default is get dhcpv6s static MIB. */
		mib_get_s(MIB_DHCPV6S_DEFAULT_LEASE, (void *)&MLTime, sizeof(MLTime));
		prefixInfo->MLTime = MLTime;
		// preferred-lifetime
		mib_get_s(MIB_DHCPV6S_PREFERRED_LIFETIME, (void *)&PFTime, sizeof(PFTime));
		prefixInfo->PLTime = PFTime;

	}
#endif
#ifdef CONFIG_IPV6_SIT
	else if (Entry.IpProtocol == IPVER_IPV4 && Entry.v6TunnelType ==V6TUNNEL_6TO4){
		struct in6_addr targetIp;
		struct in_addr inAddr;

		if (Entry.ipDhcp == (unsigned char) DHCP_CLIENT || Entry.cmode == CHANNEL_MODE_PPPOE){
			if (getInAddr(ifname, IP_ADDR, &inAddr) == 1){
				memcpy((struct in_addr *)Entry.ipAddr, &inAddr, sizeof(inAddr));
			}
			else{
				printf("Can not get ipv4 address from wan %s\n",ifname);
				goto setErr_ipv6;
			}
		}

		targetIp.s6_addr16[0] = 0x2002;
		targetIp.s6_addr[2] = Entry.ipAddr[0];
		targetIp.s6_addr[3] = Entry.ipAddr[1];
		targetIp.s6_addr[4] = Entry.ipAddr[2];
		targetIp.s6_addr[5] = Entry.ipAddr[3];
		targetIp.s6_addr16[3] = 0x0001;

		ip6toPrefix(&targetIp, 64, prefixInfo->prefixIP);
		prefixInfo->prefixLen = 64;
		strncpy(prefixInfo->leaseFile, "6to4WAN", IPV6_BUF_SIZE_128);
		mib_get_s(MIB_DHCPV6S_RENEW_TIME, (void *)&RNTime, sizeof(RNTime));
		prefixInfo->RNTime = RNTime;
		mib_get_s(MIB_DHCPV6S_REBIND_TIME, (void *)&RBTime, sizeof(RBTime));
		prefixInfo->RBTime = RBTime;
		mib_get_s(MIB_DHCPV6S_PREFERRED_LIFETIME, (void *)&PFTime, sizeof(PFTime));
		prefixInfo->PLTime = PFTime;
		mib_get_s(MIB_DHCPV6S_DEFAULT_LEASE, (void *)&MLTime, sizeof(MLTime));
		prefixInfo->MLTime = MLTime;
	}
#endif
	else //this IPv6 wan is delegated from wan dhcpv6 server
	{
		snprintf(leasefile, sizeof(leasefile), "%s.%s", PATH_DHCLIENT6_LEASE, ifname);
		/* get wan lease file for pd info */
		if (getLeasesInfo(leasefile, &dlgInfo) & LEASE_GET_IAPD) {
			ip6toPrefix(dlgInfo.prefixIP, dlgInfo.prefixLen, prefixInfo->prefixIP);
			prefixInfo->prefixLen = dlgInfo.prefixLen;
			strncpy(prefixInfo->leaseFile, leasefile, IPV6_BUF_SIZE_128);
			prefixInfo->RNTime = dlgInfo.RNTime;
			prefixInfo->RBTime = dlgInfo.RBTime;
			prefixInfo->PLTime = dlgInfo.PLTime;
			prefixInfo->MLTime = dlgInfo.MLTime;
		}
		else
		{
			//printf("[%s %d] Error! Could not get delegation info from file %s!\n", __func__, __LINE__, leasefile);
			goto setErr_ipv6;
		}
	}
	return 0;

setErr_ipv6:
	return -1;
}

//Helper function for DNSv6 mode
/* Get LAN-side DNSv6 information for IPv6 servers.
 * 0 : successful
 * -1: failed
 */
int get_dnsv6_info(DNS_V6_INFO_Tp dnsV6Info)
{
	unsigned char ipv6DnsMode=0;
	unsigned char tmpBuf[100]={0},dnsv6_1[64]={0},dnsv6_2[64]={0} ;
	unsigned char leasefile[64];
	unsigned int wanconn=0;
	DLG_INFO_T dlgInfo={0};
	MIB_CE_ATM_VC_T Entry;
	int prefixReady=0;
	struct ipv6_ifaddr ipv6_addr = {0};
	int numOfIpv6 = 0;

	if(!dnsV6Info){
		printf("Error! NULL input dnsV6Info\n");
		goto setErr_ipv6;
	}
	if (!mib_get_s(MIB_LAN_DNSV6_MODE, (void *)&ipv6DnsMode, sizeof(ipv6DnsMode))) {
		printf("Error!! get LAN IPv6 DNS Mode fail!\n");
		goto setErr_ipv6;
	}

	dnsV6Info->mode = ipv6DnsMode;

	switch(dnsV6Info->mode)
	{
		case IPV6_DNS_HGWPROXY:
			if (!mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)tmpBuf, sizeof(tmpBuf))) {
				printf("Error!! Get LAN IPv6 Address fail!");
				goto setErr_ipv6;
			}
			if(tmpBuf[0])
				strcpy(dnsV6Info->nameServer,tmpBuf);
			else
			{
				//get system LLA IP of br0
				numOfIpv6 = getifip6((char *)LANIF, IPV6_ADDR_LINKLOCAL, &ipv6_addr, 1);
				if (numOfIpv6 > 0)
				{
					inet_ntop(AF_INET6, &ipv6_addr.addr, tmpBuf, INET6_ADDRSTRLEN);
					if (tmpBuf[0])
						strcpy(dnsV6Info->nameServer,tmpBuf);
				}
			}
			//printf("%s: IPV6_DNS_HGWPROXY,with nameServer %s\n", __FUNCTION__, dnsV6Info->nameServer);
			break;
		case IPV6_DNS_WANCONN:	
			wanconn = get_dnsv6_wan_delegated_mode_ifindex();
			if (wanconn == 0xFFFFFFFF) {
				printf("Error!! %s get WAN Delegated DNSv6 fail!", __FUNCTION__);
				goto setErr_ipv6;
			}

			dnsV6Info->wanconn = wanconn;

			//printf("wanconn is %d=0x%x\n",wanconn,wanconn);
			if (dnsV6Info->wanconn == DUMMY_IFINDEX) { // auto mode, get lease from most-recently leasefile.
				if (access("/var/prefix_info", F_OK) == -1)
					goto setErr_ipv6;
				prefixReady = getLeasesInfo("/var/prefix_info", &dlgInfo);
				if (prefixReady & LEASE_GET_IFNAME)
					strncpy(tmpBuf, dlgInfo.wanIfname, 64);
			}
			else { // get lease of selected wan
				ifGetName(dnsV6Info->wanconn, tmpBuf, sizeof(tmpBuf));
			}
			snprintf(leasefile, 64, "%s.%s", PATH_DHCLIENT6_LEASE, tmpBuf);
			if(!getATMVCEntryByIfName(tmpBuf, &Entry)){
				printf("Find ATM_VC_TBL interface %s Fail!\n",tmpBuf);
				goto setErr_ipv6;
			}

			if (get_dnsv6_info_by_wan(dnsV6Info, Entry.ifIndex) == -1)
				goto setErr_ipv6;

			break;
		case IPV6_DNS_STATIC:
			if (!mib_get_s(MIB_ADSL_WAN_DNSV61, (void *)dnsv6_1, sizeof(dnsv6_1))) {
				printf("Error!! Get DNS Server Address 1 fail!");
				goto setErr_ipv6;
			}

			//DNSV61,DNSV62 is in IA_6 format
			inet_ntop(PF_INET6,dnsv6_1, tmpBuf, sizeof(tmpBuf));
			strcpy(dnsv6_1,tmpBuf);

			if (!mib_get_s(MIB_ADSL_WAN_DNSV62, (void *)dnsv6_2, sizeof(dnsv6_2))) {
				printf("Error!! Get DNS Server Address 2 fail!");
				goto setErr_ipv6;
			}
			inet_ntop(PF_INET6,dnsv6_2, tmpBuf, sizeof(tmpBuf));
			strcpy(dnsv6_2,tmpBuf);

			if(strcmp(dnsv6_1, "::") !=0 && strcmp(dnsv6_2, "::") !=0)
				snprintf(dnsV6Info->nameServer,IPV6_BUF_SIZE_256,"%s,%s",dnsv6_1,dnsv6_2);
			else if(strcmp(dnsv6_1, "::") !=0)
				snprintf(dnsV6Info->nameServer,IPV6_BUF_SIZE_256,"%s",dnsv6_1);
			else if(strcmp(dnsv6_2, "::") !=0)
				snprintf(dnsV6Info->nameServer,IPV6_BUF_SIZE_256,"%s",dnsv6_2);

			//printf("%s: IPV6_DNS_STATIC,with nameServer %s\n", __FUNCTION__, dnsV6Info->nameServer);
			break;
		default:
			printf("Error! Should not go to here!!\n");
			goto setErr_ipv6;
	}

	return 0;

setErr_ipv6:
	return -1;
}

#ifdef DHCPV6_ISC_DHCP_4XX
/*
 * is_exist_pd_info() is to judge if this WAN have PD info.
 * return 1: PD from DHCPv6 Lease file
 *        2: PD from static setting
 *        0: no any PD info
 */
int is_exist_pd_info(MIB_CE_ATM_VC_Tp pEntry)
{
	char lease_file[128] = {0};
	char ifname[IFNAMSIZ] = {0};
	DLG_INFO_T dlg_info;
	char staticPrefix[INET6_ADDRSTRLEN]={0};

	ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));
	snprintf(lease_file, sizeof(lease_file), "%s.%s", PATH_DHCLIENT6_LEASE, ifname);

	if (getLeasesInfo(lease_file, &dlg_info) & LEASE_GET_IAPD)
		return 1;
	else if (pEntry->AddrMode & IPV6_WAN_STATIC){
		printf("[%s:%d] \n", __FUNCTION__, __LINE__);
		inet_ntop(AF_INET6, (const void *)pEntry->Ipv6Prefix, staticPrefix, INET6_ADDRSTRLEN);
		if(!strcmp(staticPrefix,"::")){ /* Static Prefix didn't exist */
			printf("[%s:%d] \n", __FUNCTION__, __LINE__);
			return 0;
		}
		else
			return 2; /* Static Prefix exist  */
	}
	return 0;
}

void save_lan_global_ip_by_lease_file(char *lease_file, char *lan_infname)
{
	unsigned int entryNum = 0, i = 0, j =0;
	char *string_p = NULL;
	MIB_CE_ATM_VC_T Entry;
	int entry_index = 0;
	char ifname[IFNAMSIZ] = {0};
	DLG_INFO_T dlg_info;
	unsigned char devAddr[MAC_ADDR_LEN] = {0};
	unsigned char meui64[8] = {0};
	struct in6_addr new_lan_global_ip;
	char ipv6_addr_string[INET6_ADDRSTRLEN] = {0}, old_ipv6_addr_string[INET6_ADDRSTRLEN] = {0};
	char tmp_cmd[128] = {0};
	int uInt = 0;

	string_p = lease_file;
	string_p = string_p + strlen(PATH_DHCLIENT6_LEASE) + 1; //dhcpcV6.leases.
	snprintf(ifname , sizeof(ifname), "%s", string_p);

	if (getWanEntrybyIfname(ifname, &Entry, &entry_index) < 0)
		return;

	if (getLeasesInfo(lease_file, &dlg_info) & LEASE_GET_IAPD)
	{
		uInt = dlg_info.prefixLen;
		if (uInt != 64) {
			printf("[%s(%d)]WARNNING! Prefix Length == %d\n", __FUNCTION__, __LINE__, uInt);
			uInt = 64;
		}
		ip6toPrefix((struct in6_addr *)&dlg_info.prefixIP, uInt, &new_lan_global_ip);
		mib_get_s(MIB_ELAN_MAC_ADDR, (void *)devAddr, sizeof(devAddr));
		mac_meui64(devAddr, meui64);
		for (j = 0; j < 8; j++)
			new_lan_global_ip.s6_addr[j + 8] = meui64[j];
		inet_ntop(PF_INET6, &new_lan_global_ip, ipv6_addr_string, sizeof(ipv6_addr_string));
		/* setup LAN v6 IP address according the prefix,then the IPv6 routing could be correct */
		if (memcmp(&new_lan_global_ip, &Entry.Ipv6LocalAdress, IP6_ADDR_LEN) != 0)
		{
			/* maybe pd changed, so need to delete ori LAN IP and add new LAN IP. */
			inet_ntop(PF_INET6, (struct in6_addr *)&Entry.Ipv6LocalAdress, old_ipv6_addr_string, sizeof(old_ipv6_addr_string));
			if (strcmp(old_ipv6_addr_string, "::")) //:: means no IP, don't need to delete it.
			{
				snprintf(tmp_cmd, sizeof(tmp_cmd), "%s/%d", old_ipv6_addr_string, uInt);
				va_cmd(IFCONFIG, 3, 1, lan_infname, "del", tmp_cmd); //del old global IP with old PD.
				printf("%s: Del %s IP %s\n", __FUNCTION__, lan_infname, tmp_cmd);
			}
			/* update LAN global IP to mib. */
			printf("%s: Update  entry_index = %d  Ipv6LocalAdress address to %s\n", __FUNCTION__, entry_index, ipv6_addr_string);
			inet_pton(PF_INET6, ipv6_addr_string, (struct in6_addr *)&Entry.Ipv6LocalAdress);
			mib_chain_update(MIB_ATM_VC_TBL, (void *)&Entry, entry_index);
		}
		else
		{
			/* old IP = new IP, don't need to do anything. */
			;
		}
		//Setup br0 global address have been moved to setup_dhcpdv6_conf_with_single_br()
	}

}
#endif

#ifdef ROUTING
int route_v6_cfg_modify(MIB_CE_IPV6_ROUTE_T *pRoute, int del)
{
	char ifname[IFNAMSIZ];
	char metric[5];
	char *argv[16];
	int i,j,k;
	int ret = -1;
	unsigned int entryNum;
	MIB_CE_ATM_VC_T vcEntry;
	unsigned char	RemoteIpv6AddrStr[48];
	char add_del[5] = {0};
	FILE *fp=NULL;
	char buf[128] = {0}, gateway[64] = {0};
	int foundIface = 0;
	char iproute_tblid[16] = {0};

	if( pRoute==NULL ) return ret;
	if(!pRoute->Enable) return 0;
	if(strlen(pRoute->Dstination) == 0) return ret;

	//printf("[%s:%d] \n", __func__, __LINE__);
	ifname[0]='\0';
	ifGetName(pRoute->DstIfIndex, ifname, sizeof(ifname));
	sprintf(metric,"%d",pRoute->FWMetric);
	snprintf(iproute_tblid, sizeof(iproute_tblid), "%d", IP_ROUTE_STATIC_TABLE);

	/* route -A inet6 add/del ip-dest gw ip-nexthop metric xx dev xxx */
	argv[1] = "-A"; argv[2] = "inet6";
	if (del>0)
		argv[3] = "del";
	else
		argv[3] = "add";
	argv[4] = pRoute->Dstination;
	i = 5;
	if (pRoute->NextHop[0])
	{
		argv[i++] = "gw";
		argv[i++] = pRoute->NextHop;
		sprintf(gateway, "%s", pRoute->NextHop);
	}
	/* If user does not set Nexthop, add gateway to this route via /tmp/MERgwV6.nas0_x 20210520*/
	else if(pRoute->DstIfIndex != DUMMY_IFINDEX)
	{
		// get Nh for static route table 257
		sprintf(buf, "%s.%s", MER_GWINFO_V6, ifname);
		if((fp = fopen(buf, "r")))
		{
			gateway[0] = '\0';
			if(fscanf(fp, "%[^\n]", gateway) != 1)
				printf("fscanf failed: %s %d\n", __func__, __LINE__);
			fclose(fp);
		}
		else{
			getATMVCEntryByIfIndex(pRoute->DstIfIndex, &vcEntry);
			if(vcEntry.AddrMode == IPV6_WAN_STATIC){
				inet_ntop(AF_INET6, vcEntry.RemoteIpv6Addr, gateway,sizeof(gateway));
			}
		}
	}

	argv[i++] = "metric";
	argv[i++] = metric;

	if(pRoute->DstIfIndex== DUMMY_IFINDEX)
	{
		//should find use which interface even NextHop is not null
		char buff[1024], iface[10];
		char naddr6[INET6_ADDRSTRLEN];
		char addr6[INET6_ADDRSTRLEN];
		int num, iflags, metric_int, refcnt, use, prefix_len, slen;
		char addr6p[8][5], saddr6p[8][5], naddr6p[8][5];
		char nextHopStr[INET6_ADDRSTRLEN];
		char tmpStr[INET6_ADDRSTRLEN];
		char routeStr[INET6_ADDRSTRLEN];
		unsigned char in6_addr_buf[sizeof(struct in6_addr)];

		if (!(fp=fopen("/proc/net/ipv6_route", "r"))) {
			printf("Error: cannot open /proc/net/ipv6_route - continuing...\n");
			return -1;
		}

		while (fgets(buff, sizeof(buff), fp)) {
			num = sscanf(buff, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %4s%4s%4s%4s%4s%4s%4s%4s %02x %4s%4s%4s%4s%4s%4s%4s%4s %08x %08x %08x %08x %s\n",
					addr6p[0], addr6p[1], addr6p[2], addr6p[3],
					addr6p[4], addr6p[5], addr6p[6], addr6p[7],
					&prefix_len,
					saddr6p[0], saddr6p[1], saddr6p[2], saddr6p[3],
					saddr6p[4], saddr6p[5], saddr6p[6], saddr6p[7],
					&slen,
					naddr6p[0], naddr6p[1], naddr6p[2], naddr6p[3],
					naddr6p[4], naddr6p[5], naddr6p[6], naddr6p[7],
					&metric_int, &use, &refcnt, &iflags, iface);

			/* Fetch and resolve the nexthop address. */
			snprintf(naddr6, sizeof(naddr6), "%s:%s:%s:%s:%s:%s:%s:%s",
					naddr6p[0], naddr6p[1], naddr6p[2], naddr6p[3],
					naddr6p[4], naddr6p[5], naddr6p[6], naddr6p[7]);

			inet_pton(AF_INET6, naddr6, in6_addr_buf);
			inet_ntop(AF_INET6, in6_addr_buf, nextHopStr,sizeof(nextHopStr));

			//printf("[%s:%d] nextHopStr=%s, pRoute->NextHop=%s, iface=%s\n", __func__, __LINE__, nextHopStr, pRoute->NextHop, iface);
			if(strcmp(nextHopStr, "") != 0){
				if(!strcmp(nextHopStr, pRoute->NextHop))
				{
					strcpy(ifname, iface);
					argv[i++] = "dev";
					argv[i++] = ifname;
					argv[i] = NULL;
					foundIface = 1;
					printf("[%s:%d] foundIface=%d, iface=%s\n", __func__, __LINE__, foundIface, iface);
					break;
				}
			}
		}

		fclose(fp);

		//find the exist routes have the same route, if exist, we use the same interface
		if(foundIface == 0){
			//printf("[%s:%d] \n", __func__, __LINE__);
			if (!(fp=fopen("/proc/net/ipv6_route", "r"))) {
				printf("Error: cannot open /proc/net/ipv6_route - continuing...\n");
				return -1;
			}
			
			while (fgets(buff, sizeof(buff), fp)) {
				num = sscanf(buff, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %4s%4s%4s%4s%4s%4s%4s%4s %02x %4s%4s%4s%4s%4s%4s%4s%4s %08x %08x %08x %08x %s\n",
						addr6p[0], addr6p[1], addr6p[2], addr6p[3],
						addr6p[4], addr6p[5], addr6p[6], addr6p[7],
						&prefix_len,
						saddr6p[0], saddr6p[1], saddr6p[2], saddr6p[3],
						saddr6p[4], saddr6p[5], saddr6p[6], saddr6p[7],
						&slen,
						naddr6p[0], naddr6p[1], naddr6p[2], naddr6p[3],
						naddr6p[4], naddr6p[5], naddr6p[6], naddr6p[7],
						&metric_int, &use, &refcnt, &iflags, iface);

				/* Fetch and resolve the nexthop address. */
				snprintf(addr6, sizeof(addr6), "%s:%s:%s:%s:%s:%s:%s:%s",
						addr6p[0], addr6p[1], addr6p[2], addr6p[3],
						addr6p[4], addr6p[5], addr6p[6], addr6p[7]);

				inet_pton(AF_INET6, addr6, in6_addr_buf);
				inet_ntop(AF_INET6, in6_addr_buf, tmpStr,sizeof(tmpStr));
				snprintf(routeStr, sizeof(routeStr), "%s/%d", tmpStr, prefix_len);

				//printf("[%s:%d] routeStr=%s, pRoute->Dstination=%s, iface=%s\n", __func__, __LINE__, routeStr, pRoute->Dstination, iface);
				if(strcmp(routeStr, "") != 0 && strcmp(iface, "br0")!=0 && strcmp(iface, "lo")!=0){
					if(!strcmp(routeStr, pRoute->Dstination))
					{
						strcpy(ifname, iface);
						argv[i++] = "dev";
						argv[i++] = ifname;
						argv[i] = NULL;
						foundIface = 1;
						break;
					}
				}
			}

			fclose(fp);
		}
	}
	else
	{
		argv[i++] = "dev";
		argv[i++] = ifname;
		argv[i] = NULL;
		foundIface = 1;
	}

	if(foundIface == 0){
		printf("[%s:%d]can not find interface!!!!!!\n", __func__, __LINE__);
		return -1;
	}
	
	printf("/bin/route ");
	for (k=1; k<i; k++)
		printf("%s ", argv[k]);
	printf("\n");
	ret = do_cmd("/bin/route", argv, 1);

#ifdef CONFIG_TELMEX_DEV
	/* add policy route rule */
	sync_v6_route_to_policy_table(pRoute);

#endif

	//ipv6 static route should before pd rule
	// "ip -6 route add/del Dest via NextHop metric xx dev xxx table 257"
	if (del>0)
		sprintf(add_del, "%s", "del");
	else
		sprintf(add_del, "%s", "add");

	/* Make sure gateway exists and static route set by user has gateway for the case that user does not set Nexthop. 20210520*/
	//if (pRoute->NextHop[0])
	if(gateway[0]) {
		if(pRoute->NextHop[0])
			va_cmd(BIN_IP, 12, 1, "-6", "route", add_del, pRoute->Dstination, "via", gateway,
				"metric", metric, "dev", ifname, "table", iproute_tblid);
		else
			va_cmd(BIN_IP, 10, 1, "-6", "route", add_del, pRoute->Dstination, "metric", metric, "dev", ifname, "table", iproute_tblid);
	}

	return ret;
}

void addStaticV6Route(void)
{
	unsigned int entryNum, i;
	MIB_CE_IPV6_ROUTE_T Entry;

	entryNum = mib_chain_total(MIB_IPV6_ROUTE_TBL);

	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_IPV6_ROUTE_TBL, i, (void *)&Entry))
			return;
		route_v6_cfg_modify(&Entry, 0);
	}
}

void deleteV6StaticRoute(void)
{
	unsigned int entryNum, i;
	MIB_CE_IPV6_ROUTE_T Entry;

	entryNum = mib_chain_total(MIB_IPV6_ROUTE_TBL);

	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_IPV6_ROUTE_TBL, i, (void *)&Entry))
			return;
		route_v6_cfg_modify(&Entry, 1);
	}
}

/* When systemd renew ip need check the static route bound to the interface too, IulianWu */
void check_IPv6_staticRoute_change(char *ifname)
{
	unsigned int totalEntry =0;
	int i=0;
	MIB_CE_IPV6_ROUTE_T Entry;
	char ifname_tmp[IFNAMSIZ];
	struct ipv6_ifaddr ipv6_addr;
	int numOfIpv6;
	char SourceRouteIp[64]={0};
	struct in6_addr prefix = {0};
	struct in6_addr nh_prefix = {0};
	char buf[64]={0};
	int nh_len=0;
	char dst_addr[MAX_V6_IP_LEN] ={0};
	unsigned char nextHop[IP6_ADDR_LEN];
	char dst[INET6_ADDRSTRLEN];
	char nh_dst[INET6_ADDRSTRLEN];

	totalEntry = mib_chain_total(MIB_IPV6_ROUTE_TBL);
	for (i=0; i< totalEntry; i++)
	{
		if(mib_chain_get(MIB_IPV6_ROUTE_TBL, i, (void *)&Entry) == 0)
		{
			printf("\n %s %d\n", __func__, __LINE__);
		}
		ifGetName(Entry.DstIfIndex, ifname_tmp, sizeof(ifname_tmp));

		if(!strcmp(ifname, ifname_tmp)&&Entry.Enable) //interface
		{
			route_v6_cfg_modify(&Entry, 0);
		}
		else if(Entry.DstIfIndex == DUMMY_IFINDEX && Entry.Enable && strcmp(Entry.NextHop, "")) //nextHop
		{
			//check current interface and static route entry is the same subnet
			numOfIpv6 = getifip6(ifname, IPV6_ADDR_UNICAST, &ipv6_addr, 1);
			if(numOfIpv6 > 0)
			{
				inet_ntop(AF_INET6, &ipv6_addr.addr, SourceRouteIp, INET6_ADDRSTRLEN);
				AUG_PRT("unicast ip=%s, prefix-len=%d\n",SourceRouteIp,ipv6_addr.prefix_len);

				ip6toPrefix(&ipv6_addr.addr, ipv6_addr.prefix_len, &prefix);
				sprintf( dst, "%s",  inet_ntop(AF_INET6, &prefix, buf, sizeof(buf)));

				sscanf(Entry.Dstination, "%[^/]/%d", dst_addr, &nh_len);
				inet_pton(AF_INET6, Entry.NextHop, (void *)nextHop);
				ip6toPrefix( nextHop, nh_len, &nh_prefix);
				sprintf( nh_dst, "%s",  inet_ntop(AF_INET6, &nh_prefix, buf, sizeof(buf)));

				AUG_PRT("dst = %s, nh_dst=%s, nh_len = %d\n",dst, nh_dst, nh_len);

				if(!strcmp(dst, nh_dst))
				{
					if(route_v6_cfg_modify(&Entry, 0)){
						printf("%s %d Add ipv6 static route error!\n", __func__, __LINE__);
					}
				}
			}
			else
				AUG_PRT("no unicast ip here\n");
		}
		else
			continue;
	}
}
#ifdef CONFIG_TELMEX_DEV
int policy_route_modify(MIB_CE_IPV6_ROUTE_T *pRoute, MIB_CE_ATM_VC_Tp pvcEntry, int del)
{
	char str_tblid[10]={0};
	char ifname[IFNAMSIZ];
	char metric[5];
	char *argv[16];
	int tbl_id;
	int i=1;//,k;
	int ret = -1;

	if( pRoute==NULL ) return ret;
	if(!pRoute->Enable) return 0;

	tbl_id = caculate_tblid(pvcEntry->ifIndex);
	snprintf(str_tblid, 8, "%d", tbl_id);

	/* ip -6 add ip-dest via gateway dev xxx metric xx table xxx */
	argv[i++] = "-6";
	argv[i++] = "route";
	if (del>0)
		argv[i++] = "del";
	else
		argv[i++] = "add";
	argv[i++] = pRoute->Dstination;
	if (pRoute->NextHop[0]) {
		argv[i++] = "via";
		argv[i++] = pRoute->NextHop;
	}

	if(pRoute->DstIfIndex != DUMMY_IFINDEX)
	{
		argv[i++] = "dev";

		ifGetName(pRoute->DstIfIndex, ifname, sizeof(ifname));
		argv[i++] = ifname;
	}

	sprintf(metric,"%d",pRoute->FWMetric);
	argv[i++] = "metric";
	argv[i++] = metric;

	argv[i++] = "table";
	argv[i++] = str_tblid;

	argv[i] = NULL;

	//printf("/bin/ip ");
	//for (k=1; k<i; k++)
	//	printf("%s ", argv[k]);
	//printf("\n");
	ret = do_cmd(BIN_IP, argv, 1);

	return ret;
}

/* add static routing rule to all policy route table, or static routing rule will out of action */
void set_policy_route_for_static_rt_rule(MIB_CE_ATM_VC_Tp pEntry)
{
	MIB_CE_IPV6_ROUTE_T rtEntry;
	unsigned int rtIdx,rtNum;

	rtNum = mib_chain_total(MIB_IPV6_ROUTE_TBL);
	for (rtIdx=0; rtIdx<rtNum; rtIdx++)
	{
		if (!mib_chain_get(MIB_IPV6_ROUTE_TBL, rtIdx, (void *)&rtEntry))
			continue;

		if (!rtEntry.Enable)
			continue;

		if (rtEntry.DstIfIndex != DUMMY_IFINDEX)
		{
			if (pEntry->ifIndex == rtEntry.DstIfIndex)
			{
				/* add policy route */
				policy_route_modify(&rtEntry, pEntry, 0);
			}
		}
		else
		{
			/* add policy route */
			policy_route_modify(&rtEntry, pEntry, 0);
		}
	}
}

void sync_v6_route_to_policy_table(MIB_CE_IPV6_ROUTE_Tp prtEntry)
{
	MIB_CE_ATM_VC_T vcEntry;
	int i, vcNum;
	char ifname[IFNAMSIZ+1] = {0};

	vcNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<vcNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
			continue;

		if (!vcEntry.enable)
			continue;
		if (!(vcEntry.applicationtype & X_CT_SRV_INTERNET))
			continue;
		if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
			continue;
		if (!(vcEntry.IpProtocol&IPVER_IPV6))
			continue;

		if (prtEntry->DstIfIndex == DUMMY_IFINDEX)
		{
			policy_route_modify(prtEntry, &vcEntry, 1);
			policy_route_modify(prtEntry, &vcEntry, 0);
		}
		else if (prtEntry->DstIfIndex == vcEntry.ifIndex)
		{
			policy_route_modify(prtEntry, &vcEntry, 1);
			policy_route_modify(prtEntry, &vcEntry, 0);
			break;
		}
	}
}
#endif
#endif

int set_ipv6_default_policy_route(MIB_CE_ATM_VC_Tp pEntry, int set)
{
	uint32_t wan_mark = 0, wan_mask = 0;
	char polmark[64] = {0}, iprule_pri[16] = {0};
	char ifName[IFNAMSIZ];

#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	//Ready && CE-router Logo do not need policy route / firewall / portmapping
	unsigned char logoTestMode=0;
	if ( mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode)) ){
		if (logoTestMode)
			return 0;
	}
#endif

	ifGetName(pEntry->ifIndex, ifName, sizeof(ifName));
	if(rtk_net_get_wan_policy_route_mark(ifName, &wan_mark, &wan_mask) == -1)
	{
		AUG_PRT("%s: Error to get WAN policy mark for ifindex(%u)", __FUNCTION__, pEntry->ifIndex);
		return 0;
	}
#ifdef CONFIG_YUEME
	//if approute bind WAN not up, dont prohibit
	wan_mask |= SOCK_MARK_APPROUTE_ACT_BIT_MASK;
#endif
	sprintf(polmark, "0x%x/0x%x", wan_mark, wan_mask);
	sprintf(iprule_pri, "%d", IP_RULE_PRI_DEFAULT);
	
	va_cmd(BIN_IP, 6, 1, "-6", "rule", "del", "fwmark", polmark, "prohibit");
	
	if(!set)
	{
		return 0;
	}
	
	va_cmd(BIN_IP, 8, 1, "-6", "rule", "add", "fwmark", polmark, "pref", iprule_pri, "prohibit");
	return 0;
}


static int del_wan_pd_route_rule_with_table_id(int tbl_id, int ip_rule_preference_num)
{
	FILE *fp = NULL;
	char tmp_cmd[128] = {0};
	char buf[256] = {0}, cmd_buf[256] = {0};

	//example: 20200:  from 2001:b011:7006:1d6f::/64 lookup 48
	//awk will cut string to be "from 2001:b011:7006:1d6f::/64 lookup 48"
	snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 rule | grep %d | grep \"lookup %d\" | awk '{ $1=\"\"; print $0}'", ip_rule_preference_num, tbl_id);

	if ((fp = popen(tmp_cmd, "r")) != NULL)
	{
		while(fgets(buf, sizeof(buf), fp))
		{
			snprintf(cmd_buf, sizeof(cmd_buf), "ip -6 rule del %s", buf);
			system(cmd_buf);
		}

		pclose(fp);
		return 1;
	}
	else
		return -1;
}

#ifdef CONFIG_IP6_NF_TARGET_NPT
int del_wan_npt_pd_route_rule(MIB_CE_ATM_VC_Tp pEntry)
{
	char wan_pd_str[INET6_ADDRSTRLEN + 6] = {0}; //ipv6 ip + ::/128
	char ds_wan_npt_pd_route_tbl[10] = {0};
	char prefix_ip[INET6_ADDRSTRLEN] = {0};
	PREFIX_V6_INFO_T prefixInfo = {0};

	//Del WAN NPT PD route per WAN
	if (get_prefixv6_info_by_wan(&prefixInfo, pEntry->ifIndex) == 0)
	{
		if (prefixInfo.prefixLen && memcmp(prefixInfo.prefixIP, ipv6_unspec, prefixInfo.prefixLen<=128?prefixInfo.prefixLen:128))
		{

			inet_ntop(PF_INET6, prefixInfo.prefixIP, prefix_ip, sizeof(prefix_ip));
			if (prefix_ip[0] != '\0')
			{
				snprintf(ds_wan_npt_pd_route_tbl, sizeof(ds_wan_npt_pd_route_tbl), "%d", DS_WAN_NPT_PD_ROUTE);
				snprintf(wan_pd_str, sizeof(wan_pd_str), "%s/%d", prefix_ip, prefixInfo.prefixLen);
				va_cmd_no_error(BIN_IP, 8, 1, "-6", "route", "del", wan_pd_str, "dev", LANIF, "table", ds_wan_npt_pd_route_tbl);
				//Because pd length maybe not 64, but br0 route usually is /64, so we also need to del it.
				if (prefixInfo.prefixLen != 64)
				{
					snprintf(wan_pd_str, sizeof(wan_pd_str), "%s/64", prefix_ip);
					va_cmd(BIN_IP, 8, 1, "-6", "route", "del", wan_pd_str, "dev", LANIF, "table", ds_wan_npt_pd_route_tbl);
				}
			}
		}
	}
	return 0;
}
#endif

//WAN global IP host route for DNS binding
static int del_wan_ip_route_rule_for_dns_binding_with_table_id(int tbl_id)
{
	FILE *fp = NULL;
	char tmp_cmd[128] = {0};
	char buf[256] = {0}, cmd_buf[256] = {0};

	//example: 20000:  from 200b:db8::2e0:4cff:fe86:7004 lookup 32
	//awk will cut string to be "from 200b:db8::2e0:4cff:fe86:7004 lookup 32"
	snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 rule | grep %d | grep \"lookup %d\" | awk '{ $1=\"\"; print $0}'", IP_RULE_PRI_SRCRT, tbl_id);

	if ((fp = popen(tmp_cmd, "r")) != NULL)
	{
		while(fgets(buf, sizeof(buf), fp))
		{
			snprintf(cmd_buf, sizeof(cmd_buf), "ip -6 rule del %s", buf);
			system(cmd_buf);
		}

		pclose(fp);
		return 1;
	}
	else
		return -1;
}

int is_rule_priority_exist_in_ip6_rule(unsigned int pref_index)
{
	FILE *fp = NULL;
	char tmp_cmd[128] = {0};
	char buf[256] = {0}, token_buf[16] = {0};

	//example: 27900:  from all lookup 257
	snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 rule | grep \"%u:\"", pref_index);
	snprintf(token_buf, sizeof(token_buf), "%u", pref_index);

	if ((fp = popen(tmp_cmd, "r")) != NULL)
	{
		while(fgets(buf, sizeof(buf), fp))
		{
			if (strstr(buf, token_buf))
			{
				pclose(fp);
				return 1;
			}
		}
		pclose(fp);
	}

	return 0;
}

int get_wan_interface_by_pd(struct in6_addr *prefix_ip, char *wan_inf)
{
	int ret = 0;
	struct in6_addr temp_prefix_ip = {0}, tmp_wan_prefix_ip = {0};
	char ifname[IFNAMSIZ] = {0}, leasefile[64] = {0};
	char prefix_str[INET6_ADDRSTRLEN] = {0}, nexthop_str[INET6_ADDRSTRLEN] = {0}, dev_str[IFNAMSIZ] = {0};
	MIB_CE_ATM_VC_T entry;
	unsigned int i = 0, tmp_wan_prefix_len = 0;
	int total = mib_chain_total(MIB_ATM_VC_TBL);
	PREFIX_V6_INFO_T prefixInfo={0};

	if (!prefix_ip || !wan_inf)
		return -1;

	memcpy(&temp_prefix_ip, prefix_ip, sizeof(temp_prefix_ip));
	if (inet_ntop(AF_INET6, &temp_prefix_ip, prefix_str, sizeof(prefix_str)) == NULL)
	{
		printf("%s: wrong IPv6 PD IP!\n", __FUNCTION__);
		return -1;
	}

	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
			continue;
		if (ifGetName(entry.ifIndex, ifname, IFNAMSIZ) == 0)
			continue;
		if (!(entry.IpProtocol & IPVER_IPV6))
			continue;

		if (entry.cmode == CHANNEL_MODE_BRIDGE) //dont send RA to port which bind bridge WAN
			continue;

		snprintf(leasefile, sizeof(leasefile), "%s.%s", PATH_DHCLIENT6_LEASE, ifname);
		/* get wan lease file for pd info */
		if(get_prefixv6_info_by_wan(&prefixInfo, entry.ifIndex) != 0)
			continue;

		/* check if prefix_ip is equal subnet with wan_prefix*/
		if (ipv6_prefix_equal(&temp_prefix_ip, (struct in6_addr *)&prefixInfo.prefixIP, prefixInfo.prefixLen))
		{
			strcpy(wan_inf, ifname);
			//printf("%s: wan_inf = %s\n", __FUNCTION__, ifname);
			return 1;
		}
	}

	return 0;
}
/*lan setting page apply will trigger br0 resetart (disable_ipv6 1->0),which will delete all route entry related with br0
 *So, we need add policy route back in dhcpv6/radvd startup.
 */
int set_lanif_PD_policy_route_for_all_wan(const char* brname)
{
	char str_tblid[10]={0};
	char tmp_cmd[256] = {0};
	unsigned char prefix_ip[MAX_V6_IP_LEN]={0};
	int idx=0, tbl_id=0;
	int total = mib_chain_total(MIB_ATM_VC_TBL);
	MIB_CE_ATM_VC_T entry;
	struct in6_addr targetIp;
	PREFIX_V6_INFO_T prefixInfo;

	for(idx = 0 ; idx < total ; idx++){
		if (!mib_chain_get(MIB_ATM_VC_TBL, idx, (void *)&entry))
			continue;
		if (!(entry.IpProtocol & IPVER_IPV6))
			continue;
		if (entry.cmode == CHANNEL_MODE_BRIDGE) //dont send RA to port which bind bridge WAN
			continue;
		if (get_prefixv6_info_by_wan(&prefixInfo,entry.ifIndex)==0){
			tbl_id = caculate_tblid(entry.ifIndex);
			ip6toPrefix(prefixInfo.prefixIP, prefixInfo.prefixLen, &targetIp);
			inet_ntop(PF_INET6, &targetIp, prefix_ip, sizeof(prefix_ip));
			snprintf(tmp_cmd, sizeof(tmp_cmd), "%s/%d", prefix_ip, prefixInfo.prefixLen);
			snprintf(str_tblid, 8, "%d", tbl_id);	
			va_cmd(BIN_IP, 8, 1, "-6", "route", "del", tmp_cmd, "dev", brname, "table", str_tblid);
			va_cmd(BIN_IP, 8, 1, "-6", "route", "add", tmp_cmd, "dev", brname, "table", str_tblid);
		}
	}

	return 0;
}

int set_static_policy_route(PREFIX_V6_INFO_Tp prefixInfo, int set, const char* brname)
{
	char str_tblid[10]={0};
	char tmp_cmd[256] = {0};
	unsigned char prefix_ip[MAX_V6_IP_LEN]={0};
	int idx=0, tbl_id=0;
	int total = mib_chain_total(MIB_ATM_VC_TBL);
	MIB_CE_ATM_VC_T entry;
	struct in6_addr targetIp;
	
	ip6toPrefix(prefixInfo->prefixIP, prefixInfo->prefixLen, &targetIp);
	inet_ntop(PF_INET6, &targetIp, prefix_ip, sizeof(prefix_ip));
	snprintf(tmp_cmd, sizeof(tmp_cmd), "%s/%d", prefix_ip, prefixInfo->prefixLen);

	for(idx = 0 ; idx < total ; idx++){
		if (!mib_chain_get(MIB_ATM_VC_TBL, idx, (void *)&entry))
			continue;
		if (!(entry.IpProtocol & IPVER_IPV6))
			continue;
		if (entry.cmode == CHANNEL_MODE_BRIDGE) //dont send RA to port which bind bridge WAN
			continue;
		
		tbl_id = caculate_tblid(entry.ifIndex);
		snprintf(str_tblid, 8, "%d", tbl_id);	
		if (set)
			va_cmd(BIN_IP, 8, 1, "-6", "route", "add", tmp_cmd, "dev", brname, "table", str_tblid);
		else		
			va_cmd(BIN_IP, 8, 1, "-6", "route", "del", tmp_cmd, "dev", brname, "table", str_tblid);
	}

	return 0;
}

int set_ipv6_lan_policy_route(char *lan_ifname, struct in6_addr *ip6addr, unsigned int ip6addr_len, int set)
{
	int flags = 0;
	char str_tblid[10] = {0}, ip6addr_str[INET6_ADDRSTRLEN] = {0}, prefix_ip_str[INET6_ADDRSTRLEN] = {0};
	char target_prefix_str[INET6_ADDRSTRLEN] = {0};
	char prefix_len = 0;
	char prefix_len_str[15]={0};
	struct in6_addr prefix_addr = {0};
	unsigned char radvdPrefixMode = 0;
	char ifname[IFNAMSIZ] = {0};
	char prefix_ip[INET6_ADDRSTRLEN] = {0};
	unsigned char ula_enable = 0;
	unsigned char  dhcpv6_mode = 0, dhcpv6_type =0;
	struct in6_addr address6 = {0};
	struct in6_addr dhcpv6_prefix = {0};

	if (lan_ifname == NULL || lan_ifname[0] == '\0')
		return -1;
	if (ip6addr == NULL || (inet_ntop(PF_INET6, ip6addr, ip6addr_str, sizeof(ip6addr_str)) <= 0))
		return -1;

#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
	snprintf(ifname, sizeof(ifname), "%s", (char *)BRIF);
#else
	snprintf(ifname, sizeof(ifname), "%s", lan_ifname);
#endif
	snprintf(str_tblid, sizeof(str_tblid), "%d", IP_ROUTE_LAN_TABLE);
	ip6toPrefix(ip6addr, ip6addr_len, &prefix_addr);
	if (inet_ntop(PF_INET6, &prefix_addr, prefix_ip_str, sizeof(prefix_ip_str)))
	{
		snprintf(target_prefix_str, sizeof(target_prefix_str), "%s/%u", prefix_ip_str, ip6addr_len);
		//printf("%s %d: ifname = %s,target_prefix_str =%s, str_tblid =%s\n", __FUNCTION__, __LINE__, ifname, target_prefix_str , str_tblid);
		va_cmd_no_error(BIN_IP, 8, 1, "-6", "route", "del", target_prefix_str, "dev", ifname, "table", str_tblid);
	}

	if (set && getInFlags(lan_ifname, &flags))
	{
		if (!((flags & IFF_UP) && (flags & IFF_RUNNING)))
		{
			AUG_PRT("lan_ifname (%s) is not running, dont't update policy route table(%s)\n", lan_ifname, str_tblid);
			return -1;
		}
		va_cmd(BIN_IP, 8, 1, "-6", "route", "add", target_prefix_str, "dev", ifname, "table", str_tblid);

		//LAN IP LLA route
		va_cmd(BIN_IP, 8, 1, "-6", "route", "append", "fe80::/64", "dev", ifname, "table", str_tblid);

		//Lan Radvd static mode Prefix route
		mib_get_s(MIB_V6_RADVD_PREFIX_MODE, (void *)&radvdPrefixMode, sizeof(radvdPrefixMode));
		if (radvdPrefixMode == RADVD_MODE_MANUAL) {
			mib_get_s(MIB_V6_PREFIX_IP, (void *)prefix_ip, sizeof(prefix_ip));
			mib_get_s(MIB_V6_PREFIX_LEN, (void *)prefix_len_str, sizeof(prefix_len_str));
			if (prefix_len <= 0 || prefix_len > 128)
				prefix_len = 64;

			snprintf(target_prefix_str, sizeof(target_prefix_str), "%s/%s", prefix_ip, prefix_len_str);
			va_cmd_no_error(BIN_IP, 8, 1, "-6", "route", "del", target_prefix_str, "dev", ifname, "table", str_tblid);
			va_cmd(BIN_IP, 8, 1, "-6", "route", "add", target_prefix_str, "dev", ifname, "table", str_tblid);
		}

		//Lan ULA prefix route
		mib_get_s (MIB_V6_ULAPREFIX_ENABLE, (void *)&ula_enable, sizeof(ula_enable));
		if (ula_enable!=0) {
			if (mib_get_s(MIB_V6_ULAPREFIX, (void *)prefix_ip, sizeof(prefix_ip))
				&&mib_get_s(MIB_V6_ULAPREFIX_LEN, (void *)prefix_len_str, sizeof(prefix_len_str))){

				snprintf(target_prefix_str, sizeof(target_prefix_str), "%s/%s", prefix_ip, prefix_len_str);
				va_cmd_no_error(BIN_IP, 8, 1, "-6", "route", "del", target_prefix_str, "dev", ifname, "table", str_tblid);
				va_cmd(BIN_IP, 8, 1, "-6", "route", "add", target_prefix_str, "dev", ifname, "table", str_tblid);
			}
		}

		//Lan DHCPv6 static mode Prefix route
		mib_get_s(MIB_DHCPV6_MODE, (void *)&dhcpv6_mode, sizeof(dhcpv6_mode));
		mib_get_s(MIB_DHCPV6S_TYPE, (void *)&dhcpv6_type, sizeof(dhcpv6_type));
		if ((dhcpv6_mode ==DHCP_LAN_SERVER) && (dhcpv6_type == DHCPV6S_TYPE_STATIC)){
			mib_get_s(MIB_DHCPV6S_PREFIX_LENGTH, (void *)&prefix_len, sizeof(prefix_len));
			mib_get_s(MIB_DHCPV6S_RANGE_START, (void *)address6.s6_addr, sizeof(address6.s6_addr));
			if (prefix_len <= 0 || prefix_len > 128)
				prefix_len = 64;
			ip6toPrefix(&address6, prefix_len, &dhcpv6_prefix);
			inet_ntop(PF_INET6, &dhcpv6_prefix, prefix_ip, sizeof(prefix_ip));

			snprintf(target_prefix_str, sizeof(target_prefix_str), "%s/%d", prefix_ip, prefix_len);
			va_cmd(BIN_IP, 8, 1, "-6", "route", "del", target_prefix_str, "dev", ifname, "table", str_tblid);
			va_cmd(BIN_IP, 8, 1, "-6", "route", "add", target_prefix_str, "dev", ifname, "table", str_tblid);
		}
	}

	return 0;
}

int set_ipv6_policy_route(MIB_CE_ATM_VC_Tp pEntry, int set)
{
	int tbl_id, numOfIpv6, i, flags = 0;
	char str_tblid[10]={0};
	//char lan_str_tblid[10]={0};
	char ifname[IFNAMSIZ+1] = {0};
	char buf[128] = {0}, gateway[64] = {0};	
	struct ipv6_ifaddr ipv6_addr[6];
	struct in6_addr prefix = {0};
	FILE *fp = NULL;
	uint32_t wan_mark = 0, wan_mask = 0;
	char polmark[64] = {0}, iprule_pri_sourcert[16] = {0}, iprule_pri_policyrt[16] = {0}, iprule_pri_prefixdelegatert[16] = {0};
	char lease_file[128] = {0}, tmp_cmd[256] = {0};
	char prefix_ip[INET6_ADDRSTRLEN] = {0};
	unsigned char prefix_len = 0;
	unsigned char prefix_len_str[15]={0};
	PREFIX_V6_INFO_T wan_prefixInfo;
	struct in6_addr address6;
	char wan_pd_str[INET6_ADDRSTRLEN + 6] = {0}; //ipv6 ip + ::/128

#if defined(CONFIG_USER_RTK_RAMONITOR)
	char rainfo_file[64] = {0};
	RAINFO_V6_T ra_v6Info;
#endif

	if(pEntry == NULL) {
		AUG_PRT("Error! NULL input!\n");
		return -1;
	}

#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	//Ready && CE-router Logo do not need policy route / firewall / portmapping
	unsigned char logoTestMode=0;
	if ( mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode)) ){
		if (logoTestMode)
			return 0;
	}
#endif
	ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));

	/*if(pEntry->dgw & IPVER_IPV6) {
		AUG_PRT("pEntry(%s) is enable default gateway, ignore it.\n", ifname);
		return 0;
	}*/

	tbl_id = caculate_tblid(pEntry->ifIndex);
	snprintf(str_tblid, 8, "%d", tbl_id);
	//snprintf(lan_str_tblid, sizeof(lan_str_tblid), "%d", IP_ROUTE_LAN_TABLE);

	if(rtk_net_get_wan_policy_route_mark(ifname, &wan_mark, &wan_mask) == -1)
	{
		AUG_PRT("%s: Error to get WAN policy mark for ifindex(%u)", __FUNCTION__, pEntry->ifIndex);
		return 0;
	}
	sprintf(polmark, "0x%x/0x%x", wan_mark, wan_mask);

	sprintf(iprule_pri_sourcert, "%d", IP_RULE_PRI_SRCRT);
	sprintf(iprule_pri_policyrt, "%d", IP_RULE_PRI_POLICYRT);
	sprintf(iprule_pri_prefixdelegatert, "%d", IP_RULE_PRI_PREFIXDELEGATERT);

	AUG_PRT("%s source routing for ifname (%s), table(%s)\n", ((!set)?"Reset":"Update"), ifname, str_tblid);

	va_cmd(BIN_IP, 7, 1, "-6", "rule", "del", "oif", ifname, "table", str_tblid);
	del_wan_ip_route_rule_for_dns_binding_with_table_id(tbl_id); //del all WAN global IP host route for DNS binding
	va_cmd(BIN_IP, 7, 1, "-6", "rule", "del", "fwmark", polmark, "table", str_tblid);
	va_cmd(BIN_IP, 7, 1, "-6", "rule", "del", "pref", iprule_pri_prefixdelegatert, "table", str_tblid);// del wan pd route
	del_wan_pd_route_rule_with_table_id(tbl_id, IP_RULE_PRI_PREFIXDELEGATERT);
#if (defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_) ) && defined(_SUPPORT_L2BRIDGING_DHCP_SERVER_)
	del_wan_pd_route_rule_with_table_id(tbl_id, IP_RULE_PRI_PREFIX_INTERFACE_GROUP);
#endif
	va_cmd(BIN_IP, 5, 1, "-6", "route", "flush", "table", str_tblid);
#ifdef CONFIG_IP6_NF_TARGET_NPT
	//if (pEntry->napt_v6 != 2)
	del_wan_npt_pd_route_rule(pEntry); // del WAN NPT PD route
#endif

	if(set && getInFlags(ifname, &flags))
	{
		if(set == 2 && !((flags & IFF_UP) && (flags & IFF_RUNNING)))
		{
			AUG_PRT("ifname (%s) is not running, dont't update policy route table(%s)\n", ifname, str_tblid);
			return 1;
		}
		
		va_cmd(BIN_IP, 9, 1, "-6", "rule", "add", "oif", ifname, "pref", iprule_pri_sourcert, "table", str_tblid);
		va_cmd(BIN_IP, 9, 1, "-6", "rule", "add", "fwmark", polmark, "pref", iprule_pri_policyrt, "table", str_tblid);
		va_cmd(BIN_IP, 8, 1, "-6", "route", "append", "fe80::/64", "dev", ifname, "table", str_tblid);
		
		memset(&ipv6_addr[0], 0, sizeof(ipv6_addr));
#if defined(CONFIG_USER_RTK_RAMONITOR)
		memset(&ra_v6Info, 0, sizeof(ra_v6Info));
#endif
		//WAN IP domain route
		numOfIpv6 = getifip6(ifname, IPV6_ADDR_UNICAST, &ipv6_addr[0], sizeof(ipv6_addr)/sizeof(struct ipv6_ifaddr));
		for(i=0;i<numOfIpv6;i++)
		{
			//WAN global IP host route for DNS binding
			if(inet_ntop(AF_INET6, &ipv6_addr[i].addr, gateway, sizeof(gateway))) {
				snprintf(buf, sizeof(buf), "%s/128", gateway);
				//va_cmd_no_error(BIN_IP, 9, 1, "-6", "rule", "del", "from", buf, "pref", iprule_pri_sourcert, "table", str_tblid);
				va_cmd(BIN_IP, 9, 1, "-6", "rule", "add", "from", buf, "pref", iprule_pri_sourcert, "table", str_tblid);
			}
			ip6toPrefix(&ipv6_addr[i].addr, ipv6_addr[i].prefix_len, &prefix);
			if(inet_ntop(AF_INET6, &prefix, gateway, sizeof(gateway))){
				sprintf(buf, "%s/%d", gateway, ipv6_addr[i].prefix_len);
				//WAN prefix route
				va_cmd(BIN_IP, 8, 1, "-6", "route", "add", buf, "dev", ifname, "table", str_tblid);
			}
			if(pEntry->dslite_enable){
				if(inet_pton(AF_INET6, pEntry->dslite_aftr_hostname, &address6) == 1)
				{
					/* The judgement only used for CMCC DS-Lite Test Center Test case.
					 * In this case, AFTR server is a independent interface and it has same ipv6 prefix with WAN ip/gw but different mac with gw interface
					 * Addition,  dhcp-441 client will add 128 prefix len address for IANA (maybe we should change to 64?)
					 * So we need add xxx::/128 policy route without gateway for AFTR server, in order that the packet could be sent to AFTR interface 
					 */
					if  ((ipv6_addr[i].prefix_len ==128) && (ipv6_prefix_equal(&address6,  &ipv6_addr[i].addr,  64)))
					{
						sprintf(buf, "%s/128", pEntry->dslite_aftr_hostname);
						//WAN Aftr host route
						va_cmd(BIN_IP, 8, 1, "-6", "route", "add", buf, "dev", ifname, "table", str_tblid);
					}
				}
			}
		}
		
#if defined(CONFIG_USER_RTK_RAMONITOR)
		//WAN RA prefix route
		snprintf(rainfo_file, sizeof(rainfo_file), "%s_%s", (char *)RA_INFO_FILE, ifname);
		if (get_ra_info(rainfo_file, &ra_v6Info) & RAINFO_GET_PREFIX){
			snprintf(buf, sizeof(buf), "%s/%d", ra_v6Info.prefix_1, ra_v6Info.prefix_len_1);
			if (ra_v6Info.prefix_onlink){
				va_cmd(BIN_IP, 8, 1, "-6", "route", "add", buf, "dev", ifname, "table", str_tblid);
			}
			else {
				va_cmd(BIN_IP, 8, 1, "-6", "route", "del", buf, "dev", ifname, "table", str_tblid);
			}
		}
#endif

#ifdef CONFIG_IPV6_SIT_6RD
		if (pEntry->IpProtocol == IPVER_IPV4 && pEntry->v6TunnelType == V6TUNNEL_6RD){
			// ip -6 route add ::/0 via ::10.1.1.1 dev tun6rd
			unsigned char SixrdBRv4IP[INET_ADDRSTRLEN];
			
			inet_ntop(PF_INET, (struct in_addr *)pEntry->SixrdBRv4IP, SixrdBRv4IP, sizeof(SixrdBRv4IP));
			snprintf(buf, sizeof(buf), "::%s", SixrdBRv4IP);
			va_cmd("/usr/bin/ip", 10, 1, "-6", "route", "add", "default", "via", buf, "dev", TUN6RD_IF, "table", str_tblid);
		}else
#endif
#ifdef CONFIG_IPV6_SIT
		if (pEntry->IpProtocol == IPVER_IPV4 && pEntry->v6TunnelType == V6TUNNEL_6IN4){
			inet_ntop(PF_INET6, (struct in6_addr *)pEntry->RemoteIpv6Addr, prefix_ip, sizeof(prefix_ip));
			va_cmd("/usr/bin/ip", 10, 1, "-6", "route", "add", "default", "via", prefix_ip, "dev", TUN6IN4_IF, "table", str_tblid);
		}else if (pEntry->IpProtocol == IPVER_IPV4 && pEntry->v6TunnelType == V6TUNNEL_6TO4){
			unsigned char remoteIpAddr[INET_ADDRSTRLEN];
			inet_ntop(PF_INET,	(struct in_addr *)pEntry->v6TunnelRv4IP, remoteIpAddr, sizeof(remoteIpAddr));
			snprintf(buf, sizeof(buf), "::%s", remoteIpAddr);
			va_cmd("/usr/bin/ip", 10, 1, "-6", "route", "add", "default", "via", buf, "dev", TUN6TO4_IF, "table", str_tblid);
			va_cmd("/usr/bin/ip", 8, 1, "-6", "route", "add", "2002::/16", "dev", TUN6TO4_IF, "table", str_tblid);
		}else
#endif

		{
			snprintf(buf, sizeof(buf), "%s.%s", MER_GWINFO_V6, ifname);
			if((fp = fopen(buf, "r")))
			{
				gateway[0] = '\0';
				if(fscanf(fp, "%[^\n]", gateway) != 1)
				{
					printf("fscanf failed: %s %d\n", __func__, __LINE__);
				}
				fclose(fp);	
				if(gateway[0]){
					va_cmd(BIN_IP, 10, 1, "-6", "route", "add", "default", "via", gateway, "dev", ifname, "table", str_tblid);
				}
			}
		}
		/* We move LAN prefix route(LAN LLA, static RADVD prefix, ULA prefix, static dhcpv6 prefix and PD route) into
		 * route table IP_ROUTE_LAN_TABLE. 2021/11/09
		 */

#if defined(ROUTING)&&defined(CONFIG_TELMEX_DEV)
		/* add static route to policy route table */
		set_policy_route_for_static_rt_rule(pEntry);
#endif

		//WAN PD route
		memset(&wan_prefixInfo,0,sizeof(wan_prefixInfo));
		if (0 == get_prefixv6_info_by_wan(&wan_prefixInfo, pEntry->ifIndex)){
			if (inet_ntop(PF_INET6, (struct in6_addr *)wan_prefixInfo.prefixIP, prefix_ip, sizeof(prefix_ip))){
				prefix_len = wan_prefixInfo.prefixLen;
				if (prefix_len <= 0 || prefix_len > 128)
					prefix_len = 64;
				snprintf(tmp_cmd, sizeof(tmp_cmd), "%s/%d", prefix_ip, prefix_len);
				snprintf(wan_pd_str, sizeof(wan_pd_str), "%s", tmp_cmd);

				va_cmd(BIN_IP, 9, 1, "-6", "rule", "add", "from", tmp_cmd, "pref", iprule_pri_prefixdelegatert, "lookup", str_tblid);
				//Maybe we don't need it.(LAN domain route) pd route for lan policy table
				//va_cmd(BIN_IP, 8, 1, "-6", "route", "add", tmp_cmd, "dev", LANIF, "table", lan_str_tblid);
			}
		}
		//DHCPv6 Server LAN Client PD route (For DHCPv6 Server offer PD to client)
		rtk_set_dhcp6s_lan_pd_policy_route_info(ifname, NULL);
		//WAN NPT PD route
		unsigned int wanconn = DUMMY_IFINDEX;
		wanconn = get_pd_wan_delegated_mode_ifindex();
		if (wanconn == 0xFFFFFFFF) {
			printf("Error!! %s get WAN Delegated PREFIXINFO fail!", __FUNCTION__);
		}

#ifdef CONFIG_IP6_NF_TARGET_NPT
		if (pEntry->napt_v6 != 2 && wan_pd_str[0] != '\0' && wanconn == pEntry->ifIndex)
		{
			char ds_wan_npt_pd_route_tbl[10] = {0};

			snprintf(ds_wan_npt_pd_route_tbl, sizeof(ds_wan_npt_pd_route_tbl), "%d", DS_WAN_NPT_PD_ROUTE);
			va_cmd_no_error(BIN_IP, 8, 1, "-6", "route", "del", wan_pd_str, "dev", LANIF, "table", ds_wan_npt_pd_route_tbl);
			va_cmd(BIN_IP, 8, 1, "-6", "route", "add", wan_pd_str, "dev", LANIF, "table", ds_wan_npt_pd_route_tbl);
		}
#endif
#if (defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_) ) && defined(_SUPPORT_L2BRIDGING_DHCP_SERVER_)
		if(pEntry->ifIndex == wanconn) {
			if ( memcmp(wan_prefixInfo.prefixIP, ipv6_unspec, wan_prefixInfo.prefixLen<=128?wan_prefixInfo.prefixLen:128) && wan_prefixInfo.prefixLen !=0)
				setup_interface_group_br_policy_route(&wan_prefixInfo,str_tblid, set, pEntry->napt_v6);
		}
#endif
	}

	return 0;
}

#ifdef IP_ACL
static void set_v6_acl_service(unsigned char service, char *act, char *strIP, char *protocol, unsigned short port, unsigned short whiteBlackRule)
{
	char strInputPort[8]="";
	char policy[32];
	
	if (whiteBlackRule == 0)
		strcpy(policy, FW_ACCEPT);
	else
		strcpy(policy, FW_DROP);
	
	snprintf(strInputPort, sizeof(strInputPort), "%d", port);
	if (service & 0x02) {	// can LAN access
		// ip6tables -A aclblock -i $LAN_IF -s 2001::100/64 -p UDP --dport 69 -j RETURN
		va_cmd(IP6TABLES, 12, 1, act, (char *)FW_ACL,
			(char *)ARG_I, (char *)LANIF_ALL, "-s", strIP, "-p", protocol,
			(char *)FW_DPORT, strInputPort, "-j", (char *)policy);
	}
	if (service & 0x01) {	// can WAN access
		// ip6tables -A aclblock -i ! $LAN_IF -s 2001::100/64 -p UDP --dport 69 -j RETURN
		va_cmd(IP6TABLES, 13, 1, act, (char *)FW_ACL,
			"!", (char *)ARG_I, (char *)LANIF_ALL, "-s", strIP, "-p", protocol,
			(char *)FW_DPORT, strInputPort, "-j", (char *)policy);
	}
}

static void filter_set_v6_acl_service(MIB_CE_V6_ACL_IP_T Entry, int enable, char *strIP)
{
	char *act;
	char strPort[8];
	int ret;
	unsigned char whiteBlackRule;
	char policy[32];


	if (enable)
		act = (char *)FW_ADD;
	else
		act = (char *)FW_DEL;

	mib_get_s(MIB_V6_ACL_CAPABILITY_TYPE, (void *)&whiteBlackRule, sizeof(whiteBlackRule));
	if (whiteBlackRule == 0)
		strcpy(policy, FW_ACCEPT);
	else
		strcpy(policy, FW_DROP);
	// telnet service: bring up by inetd
	set_v6_acl_service(Entry.telnet, act, strIP, (char *)ARG_TCP, 23, whiteBlackRule);

	#ifdef CONFIG_USER_FTPD_FTPD
	// ftp service: bring up by inetd
	set_v6_acl_service(Entry.ftp, act, strIP, (char *)ARG_TCP, 21, whiteBlackRule);
	#endif	

	#ifdef CONFIG_USER_TFTPD_TFTPD
	// tftp service: bring up by inetd
	set_v6_acl_service(Entry.tftp, act, strIP, (char *)ARG_UDP, 69, whiteBlackRule);
	#endif

	// HTTP service
	set_v6_acl_service(Entry.web, act, strIP, (char *)ARG_TCP, 80, whiteBlackRule);
	
	#ifdef CONFIG_USER_BOA_WITH_SSL
  	//HTTPS service
	set_v6_acl_service(Entry.https, act, strIP, (char *)ARG_TCP, 443, whiteBlackRule);
	#endif

	#ifdef CONFIG_USER_SSH_DROPBEAR
	// ssh service	
	set_v6_acl_service(Entry.ssh, act, strIP, (char *)ARG_TCP, 22, whiteBlackRule);
	#endif

	#ifdef CONFIG_USER_SAMBA
	set_v6_acl_service(Entry.smb, act, strIP, (char *)ARG_UDP, 137, whiteBlackRule);
	set_v6_acl_service(Entry.smb, act, strIP, (char *)ARG_UDP, 138, whiteBlackRule);
	set_v6_acl_service(Entry.smb, act, strIP, (char *)ARG_TCP, 445, whiteBlackRule);
	#endif
	
	#if defined(CONFIG_CMCC) //block wan 8080 port, java web
	set_v6_acl_service(1, act, strIP, (char *)ARG_TCP, 8080, 1);
	#endif
	
	// ping service
	if (Entry.icmp & 0x02) // can LAN access
	{
		// ip6tables -A aclblock -i $LAN_IF  -s 2001::100/64 -p ICMPV6 -m limit --limit 1/s -j RETURN
		va_cmd(IP6TABLES, 16, 1, act, (char *)FW_ACL,
			(char *)ARG_I, (char *)LANIF_ALL, "-s", strIP, "-p", "ICMPV6",
			"--icmpv6-type","echo-request",
			"-m", "limit",
			"--limit", "1/s", "-j", (char *)policy);
	}
	if (Entry.icmp & 0x01) // can WAN access
	{
		// ip6tables -A aclblock -i ! $LAN_IF  -s 2001::100/64 -p ICMPV6 -m limit --limit 1/s -j RETURN
		if (whiteBlackRule == 0) {		
			va_cmd(IP6TABLES, 19, 1, act, (char *)FW_ACL,
			"!", (char *)ARG_I, (char *)LANIF_ALL, "-s", strIP, "-p", "ICMPV6",
				"--icmpv6-type","echo-request",
			"-m", "limit",
				"--limit", "1/s", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
		}
		else{
			va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_ACL,
				(char *)ARG_I, "lo","-p", "ICMPV6", 
				"--icmpv6-type","echo-request","-j", (char *)FW_ACCEPT);
			
			va_cmd(IP6TABLES, 17, 1, act, (char *)FW_ACL,
				"!", (char *)ARG_I, (char *)LANIF_ALL, "-s", strIP, "-p", "ICMPV6",
				"--icmpv6-type","echo-request",
				"-m", "limit",
				"--limit", "1/s", "-j", (char *)policy);
		}
	}	
}

void filter_set_v6_acl(int enable)
{
	int i, total, len;
	char ssrc[55];
	MIB_CE_V6_ACL_IP_T Entry;
	unsigned char dhcpvalue[32];
	unsigned char vChar;
	int dhcpmode;
	unsigned char whiteBlackRule;
	char policy[32], policy_end[32];
	
	// check if ACL Capability is enabled ?
	if (!enable) {
		va_cmd(IP6TABLES, 2, 1, "-F", (char *)FW_ACL);
		va_cmd(IP6TABLES, 3, 1, "-P", (char *)FW_INPUT, FW_DROP);
	}
	else {
		mib_get_s(MIB_V6_ACL_CAPABILITY_TYPE, (void *)&whiteBlackRule, sizeof(whiteBlackRule));
		if (whiteBlackRule == 0 ) { /* white List */
			strcpy(policy, FW_RETURN);
			strcpy(policy_end, FW_DROP);
		}
		else { /* black list */
			strcpy(policy, FW_DROP);
			strcpy(policy_end, FW_RETURN);
		}
		va_cmd(IP6TABLES, 3, 1, "-P", (char *)FW_INPUT, FW_ACCEPT);
		// Add policy to aclblock chain
		total = mib_chain_total(MIB_V6_ACL_IP_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_V6_ACL_IP_TBL, i, (void *)&Entry))
			{
				return;
			}

			// Check if this entry is enabled
			if ( Entry.Enabled == 1 ) {
				inet_ntop(PF_INET6, (struct in6_addr *)Entry.sip6Start, ssrc, 48);
				len = sizeof(ssrc) - strlen(ssrc);
				if(snprintf(ssrc + strlen(ssrc), len, "/%d", Entry.sip6PrefixLen) >= len) {
					printf("warning, string truncated\n");
				}
				
				if ( Entry.Interface == IF_DOMAIN_LAN ) {
					if (Entry.any == 0x01)  	// service = any(0x01)
						va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-i", LANIF_ALL, "-s", ssrc, "-j", (char *)policy);
					else
						filter_set_v6_acl_service(Entry, enable, ssrc);
				} 
				else
					filter_set_v6_acl_service(Entry, enable, ssrc);
			}
		}

		// (1) allow for LAN
		// allowing DNS request during ACL enabled
		// ip6tables -A aclblock -p udp -i br0 --dport 53 -j RETURN		
		va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "udp", "-i", LANIF_ALL,(char*)FW_DPORT, (char *)PORT_DNS, "-j", (char *)FW_RETURN);
		va_cmd(IP6TABLES, 11, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "udp","!", "-i", LANIF_ALL,(char*)FW_DPORT, (char *)PORT_DNS, "-j", (char *)FW_DROP);
		//allow dhcpv6 packets
		va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "udp", "-i", LANIF_ALL, (char*)FW_DPORT, "547", "-j", (char *)FW_RETURN);
		// ip6tables -A aclblock -i lo -j RETURN
		// Local Out DNS query will refer the /etc/resolv.conf(DNS server is 127.0.0.1). We should accept this DNS query when enable ACL.
		va_cmd(IP6TABLES, 6, 1, (char *)FW_ADD, (char *)FW_ACL, "-i", "lo", "-j", (char *)FW_RETURN);
		
		// (2) allow for WAN
		// (2.1) Added by Mason Yu for dhcp Relay. Open DHCP Relay Port for Incoming Packets.
#ifdef DHCPV6_ISC_DHCP_4XX
		if (mib_get_s(MIB_DHCPV6_MODE, (void *)dhcpvalue, sizeof(dhcpvalue)) != 0)
		{
			dhcpmode = (unsigned int)(*(unsigned char *)dhcpvalue);
			if (dhcpmode == 1 || dhcpmode == 2 ){
				// ip6tables -A aclblock -i ! br0 -p udp --dport 67 -j ACCEPT
				va_cmd(IP6TABLES, 11, 1, (char *)FW_ADD, (char *)FW_ACL, "!", (char *)ARG_I, LANIF_ALL, "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_ACCEPT);
			}
		}
#endif
		/* allow icmpv6 rs, ra, ns, na */
		// ip6tables -A aclblock -p icmpv6 --icmpv6-type router-solicitation -j RETURN
		va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "icmpv6",
			"--icmpv6-type","router-solicitation","-j", (char *)FW_RETURN);

		// ip6tables -A aclblock -p icmpv6 --icmpv6-type router-advertisement -j RETURN
		va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "icmpv6",
			"--icmpv6-type","router-advertisement","-j", (char *)FW_RETURN);
	
		// ip6tables -A aclblock -p icmpv6 --icmpv6-type neighbour-solicitation -j RETURN
		va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "icmpv6",
			"--icmpv6-type","neighbour-solicitation","-j", (char *)FW_RETURN);
	
		// ip6tables -A aclblock -p icmpv6 --icmpv6-type neighbour-advertisement -j RETURN
		va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "icmpv6",
			"--icmpv6-type","neighbour-advertisement","-j", (char *)FW_RETURN);

		// ip6tables -A aclblock -p icmpv6 --icmpv6-type echo-request -j RETURN
		va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "icmpv6",
			"--icmpv6-type","echo-request","-j", (char *)FW_RETURN);

		/* allow dhcpv6 (client) */
		// ip6tables  -A aclblock -p udp --dport  546 -j RETURN
		va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "udp",
			"--dport", "546", "-j", (char *)FW_RETURN);
		
		/* allow dhcpv6 (server) */
		// ip6tables  -A aclblock -p udp --dport  547 -j RETURN
		va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "udp",
			"--dport", "547", "-j", (char *)FW_RETURN);
		
		// accept related
		// ip6tables -A aclblock -m state --state ESTABLISHED,RELATED -j RETURN
#if defined(CONFIG_00R0)
		char ipfilter_spi_state=1;
		mib_get_s(MIB_IPFILTER_SPI_ENABLE, (void*)&ipfilter_spi_state, sizeof(ipfilter_spi_state));
		if(ipfilter_spi_state != 0)
#endif
		va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-m", "state",
			"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);

		// (1) allow service with the same mark value(0x1000) from WAN
		va_cmd(IP6TABLES, 11, 1, (char *)FW_ADD, (char *)FW_ACL,
			"!", (char *)ARG_I, LANIF_ALL, "-m", "mark", "--mark", RMACC_MARK, "-j",
			(whiteBlackRule==0)? FW_ACCEPT:FW_DROP);

		// ip6tables -A aclblock -j DROP
		va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_ACL, "-j", (char *)policy_end);
	}
}

int restart_v6_acl(void)
{
	unsigned char aclEnable;
	
	mib_get_s(MIB_V6_ACL_CAPABILITY, (void *)&aclEnable, sizeof(aclEnable));
	if (aclEnable == 1)  // ACL Capability is enabled
	{
		filter_set_v6_acl(0);
		filter_set_v6_acl(1);
	}
	else
		filter_set_v6_acl(0);

	return 0;
}
#endif

#ifdef CONFIG_USER_RADVD

// Added by Mason Yu for p2r_test
void init_radvd_conf_mib(void)
{
	unsigned char vChar;

	// MaxRtrAdvIntervalAct
	mib_set(MIB_V6_MAXRTRADVINTERVAL, "");

	// MinRtrAdvIntervalAct
	mib_set(MIB_V6_MINRTRADVINTERVAL, "");

	// AdvCurHopLimitAct
	mib_set(MIB_V6_ADVCURHOPLIMIT, "");

	// AdvDefaultLifetime
	mib_set(MIB_V6_ADVDEFAULTLIFETIME, "");

	// AdvReachableTime
	mib_set(MIB_V6_ADVREACHABLETIME, "");

	// AdvRetransTimer
	mib_set(MIB_V6_ADVRETRANSTIMER, "");

	// AdvLinkMTU
	mib_set(MIB_V6_ADVLINKMTU, "");

	// AdvSendAdvert
	vChar = 1;
	mib_set( MIB_V6_SENDADVERT, (void *)&vChar);      // on

	// AdvManagedFlag
	vChar = 2;
	mib_set( MIB_V6_MANAGEDFLAG, (void *)&vChar );     // ignore

	// AdvOtherConfigFlag
	vChar = 2;
	mib_set( MIB_V6_OTHERCONFIGFLAG, (void *)&vChar ); // ignore

    // RDNSS
	mib_set(MIB_V6_RDNSS1, "");
	mib_set(MIB_V6_RDNSS2, "");

	// Prefix
	vChar = 1;
	mib_set( MIB_V6_RADVD_PREFIX_MODE, &vChar);

	mib_set(MIB_V6_PREFIX_IP, "3ffe:501:ffff:100::");
	mib_set(MIB_V6_PREFIX_LEN, "64");

	// AdvValidLifetime
	mib_set(MIB_V6_VALIDLIFETIME, "2592000");

	// AdvPreferredLifetime
	mib_set(MIB_V6_PREFERREDLIFETIME, "604800");

	// AdvOnLink
	vChar = 2;
	mib_set( MIB_V6_ONLINK, (void *)&vChar );    // ignore

	// AdvAutonomous
	vChar = 2;
	mib_set( MIB_V6_AUTONOMOUS, (void *)&vChar ); // ignore
}
#endif

/************************************************
* Propose: delOrgLanLinklocalIPv6Address
*
*    delete the original Link local IPv6 address
*      e.q: ifconfig br0 del fe80::2e0:4cff:fe86:5338/64 >/dev/null 2>&1
*
*    When modify the function, please also modify _delOrgLanLinklocalIPv6Address()
*    in src/linux/msgparser.c
* Parameter:
*	None
* Return:
*     None
* Author:
*     Alan
*************************************************/
void delOrgLanLinklocalIPv6Address(void)
{
	unsigned char value[64];
	unsigned char ip_str[64] = {0};

	struct in6_addr ip6Addr, targetIp;
	unsigned char devAddr[MAC_ADDR_LEN];
	unsigned char *p = &targetIp.s6_addr[0];
	int i=0;
	char cmdBuf[256]={0};

	if (mib_get_s(MIB_ELAN_MAC_ADDR, (void *)devAddr, sizeof(devAddr)) != 0)
	{
		ipv6_linklocal_eui64(p, devAddr);
		inet_ntop(PF_INET6, &targetIp, value, sizeof(value));

		if (mib_get(MIB_IPV6_LAN_IP_ADDR, (void *)ip_str) != 0)
		{
			//if MIB_IPV6_LAN_IP_ADDR is same as ipv6_linklocal_eui64, do not delete.
			if(0 == strcmp(value, ip_str)){
				return;
			}
		}

		sprintf(cmdBuf, "%s %s %s %s/%d %s %s", IFCONFIG, LANIF, ARG_DEL, value, 64, hideErrMsg1, hideErrMsg2);
		//use system to prevent error message
		system(cmdBuf);
		//va_cmd(IFCONFIG, 5, 1, LANIF, ARG_DEL, value, hideErrMsg1, hideErrMsg2);
	}
}

/************************************************
* delWanGlobalIPv6Address()
*
* Propose:
* 	Delete the original WAN global IPv6 address with RA prefix and EUI-64
*          e.q: ifconfig nas0_0 del 2001:b011:7006:27bc:2e0:4cff:fe86:7004/64 >/dev/null 2>&1
*
*   When modify the function, please also modify _delWanGlobalIPv6Address()
*   in src/linux/msgparser.c
*   
* Paremeter:
*    MIB_CE_ATM_VC_Tp pEntry: pointer to the WAN entry.
*    char *prefix_info: need to be prefix address.
*
* Author:
*    Rex
*************************************************/
#define IPV6_PREFIX_LEN 64
void delWanGlobalIPv6Address(MIB_CE_ATM_VC_Tp pEntry)
{
	char wan_inf[IFNAMSIZ]={0};
	struct ipv6_ifaddr ipv6_addr[6];
	unsigned int total=0, idx=0;
	char globalAddr[INET6_ADDRSTRLEN]={0};
	char cmdBuf[256]={0};

	if(!pEntry){
		fprintf(stderr, "[%s] null ATM_VC_TBL\b", __FUNCTION__);
		return;
	}

	if(ifGetName(pEntry->ifIndex, wan_inf, sizeof(wan_inf)) == NULL)
		return;

	total = getifip6(wan_inf, IPV6_ADDR_UNICAST, &ipv6_addr[0], sizeof(ipv6_addr)/sizeof(struct ipv6_ifaddr));

	for( idx = 0; idx < total; idx++ )
	{
		//delete WAN global IP
		if(inet_ntop(AF_INET6, &ipv6_addr[idx].addr, globalAddr, sizeof(globalAddr))) {
			sprintf(cmdBuf, "%s %s %s %s/%d %s %s", IFCONFIG, wan_inf, ARG_DEL, globalAddr, IPV6_PREFIX_LEN, hideErrMsg1, hideErrMsg2);
			system(cmdBuf);
		}
	}
	return;
}

/************************************************
* Propose: setLanLinkLocalIPv6Address(void)
*    set the Link local IPv6 address
*      e.q: ifconfig br0 add fe80::1/64 >/dev/null 2>&1
*
*    When modify the function, please also modify _setLanLinkLocalIPv6Address()
*    in src/linux/msgparser.c
* Parameter:
*	None
* Return:
*     None
* Author:
*     Alan
*************************************************/
void setLanLinkLocalIPv6Address(void)
{
	unsigned char value[64];
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	unsigned char logoTestMode = 0;
#endif

	if (mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)value, sizeof(value)) != 0)
	{
		char cmdBuf[256]={0};

		if (!strcmp(value, ""))  // It is Auto config LLA IP.
			return;

		sprintf(cmdBuf, "%s %s %s %s/%d %s %s", IFCONFIG, LANIF, ARG_ADD, value, 64, hideErrMsg1, hideErrMsg2);
		//use system to prevent error message
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
		/*CE_ROUTER lan_rfc4862 Case: Gloabl Address Autoconfiguration and DAD
		* Test Bed expect recv Global address's DAD_NS, So dodelegation() can not re-config EUI64 address
		*/
		if( mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode))){
			if(logoTestMode != 3) //NOT CE_ROUTER mode
				system(cmdBuf);
		}
#else
		system(cmdBuf);
#endif
		//va_cmd(IFCONFIG, 5, 1, LANIF, ARG_ADD, cmdBuf, hideErrMsg1, hideErrMsg2);
	}
}

/************************************************
* Propose: setLinklocalIPv6Address
*
*    set thel Link local IPv6 address
*      e.q: ifconfig br0 add fe80::2e0:4cff:fe86:5338/64 >/dev/null 2>&1
*
* Parameter:
*	char* ifname              interface name
* Return:
*     None
* Author:
*     Alan
*************************************************/
void setLinklocalIPv6Address(char* ifname)
{
	unsigned char value[64];
	struct in6_addr ip6Addr, targetIp;
	unsigned char devAddr[MAC_ADDR_LEN];
	unsigned char *p = &targetIp.s6_addr[0];
	int i=0;
	char cmdBuf[256]={0};

	if (getMacAddr(ifname, devAddr) != 0)
	{
		ipv6_linklocal_eui64(p, devAddr);
		inet_ntop(PF_INET6, &targetIp, value, sizeof(value));
		sprintf(cmdBuf, "%s %s %s %s/%d %s %s", IFCONFIG, ifname, ARG_ADD, value, 64, hideErrMsg1, hideErrMsg2);
		//use system to prevent error message
		system(cmdBuf);
		//va_cmd(IFCONFIG, 5, 1, ifname, ARG_ADD, value, hideErrMsg1, hideErrMsg2);
	}
}

#ifdef CONFIG_RTK_IPV6_LOGO_TEST
void setLanIPv6Address4Readylogo()
{
	unsigned char tmpBuf[MAX_V6_IP_LEN]={0};
	PREFIX_V6_INFO_T prefixInfo = {0};
	int ret = 0;
	unsigned char devAddr[MAC_ADDR_LEN];
	struct in6_addr targetIp = {0};
	unsigned char *p = &targetIp.s6_addr[0];
	char cmdBuf[256]={0};
	int skipllaaction = 0;
	
	mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)tmpBuf, sizeof(tmpBuf));
	mib_get_s(MIB_ELAN_MAC_ADDR, (void *)devAddr, sizeof(devAddr));
	
	if(!strcmp(tmpBuf,""))
	{
		ipv6_linklocal_eui64(p, devAddr);
		inet_ntop(PF_INET6, &targetIp, tmpBuf, sizeof(tmpBuf));
	}
	else if (!inet_pton(AF_INET6, tmpBuf, &targetIp) || !IN6_IS_ADDR_LINKLOCAL(&targetIp))
	{
		skipllaaction = 1;
		printf("[%s]Invalid Link-Local Address for %s\n", __func__,LANIF);
	}

	snprintf(cmdBuf, sizeof(cmdBuf), "echo 2 > /proc/sys/net/ipv6/conf/%s/dad_transmits", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", cmdBuf);
	
	if(skipllaaction == 0)
	{
		sprintf(cmdBuf, "%s %s %s %s/%d %s %s", IFCONFIG, LANIF, ARG_DEL, tmpBuf, 64, hideErrMsg1, hideErrMsg2);
		//use system to prevent error message
		va_cmd("/bin/sh", 2, 1, "-c", cmdBuf);
		sprintf(cmdBuf, "%s %s %s %s/%d %s %s", IFCONFIG, LANIF, ARG_ADD, tmpBuf, 64, hideErrMsg1, hideErrMsg2);
		//use system to prevent error message
		va_cmd("/bin/sh", 2, 1, "-c", cmdBuf);
	}
	
	ret = get_radvd_prefixv6_info(&prefixInfo);
	if(!ret){
		p = &targetIp.s6_addr[0];
		memset(cmdBuf, 0, sizeof(cmdBuf));
		memset(&targetIp, 0, sizeof(targetIp));
		memcpy(targetIp.s6_addr,prefixInfo.prefixIP,8);

		ipv6_global_eui64(p, devAddr);
		inet_ntop(PF_INET6, &targetIp, tmpBuf, sizeof(tmpBuf));

		snprintf(cmdBuf, sizeof(cmdBuf), "%s/%d", tmpBuf,prefixInfo.prefixLen);
		va_cmd(IFCONFIG, 3, 1, LANIF, "del", cmdBuf);
		va_cmd(IFCONFIG, 3, 1, LANIF, "add", cmdBuf);
		printf("[%s] cmd = ifconfig %s add %s\n", __func__, LANIF,cmdBuf);
	}
	else
		printf("[%s]get global prefixv6 info fail!\n",__func__);

	//snprintf(cmdBuf, sizeof(cmdBuf), "echo 1 > /proc/sys/net/ipv6/conf/%s/dad_transmits", (char*)BRIF);
	//va_cmd("/bin/sh", 2, 1, "-c", cmdBuf);
}
/*
 *  auto config MIB_IPV6_LAN_LLA_IP_ADDR for logo test
 */
void checkLanLinklocalIPv6Address()
{
	unsigned char value[MAX_V6_IP_LEN];
	struct in6_addr ip6Addr, targetIp;
	unsigned char devAddr[MAC_ADDR_LEN];
	unsigned char *p = &targetIp.s6_addr[0];
	int i=0;
	char cmdBuf[256]={0};
	char tmpBuf[MAX_V6_IP_LEN];

	if (mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)tmpBuf, sizeof(tmpBuf)) != 0){
		if (mib_get_s(MIB_ELAN_MAC_ADDR, (void *)devAddr, sizeof(devAddr)) != 0)
		{
			ipv6_linklocal_eui64(p, devAddr);
			inet_ntop(PF_INET6, &targetIp, value, sizeof(value));
			if(strcmp(value,tmpBuf)){
				printf("change lan link local addr to %s\n",value);
				mib_set(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)value);
			}
		}
	}
}
#endif

#ifdef CONFIG_USER_RTK_RAMONITOR 
/*
 *  del_wan_rainfo_file():
 *  	To delete the last RA info file "/var/rainfo_xxx"
 */
int del_wan_rainfo_file(MIB_CE_ATM_VC_Tp pEntry)
{
	char ifname[IFNAMSIZ] = {0};
	char rainfo_file[64] = {0};
	char cmd[64] = {0};
	
	// Stop DHCPv6 client
	if(ifGetName(pEntry->ifIndex, ifname, sizeof(ifname)) == NULL)
		return -1;

	snprintf(rainfo_file, sizeof(rainfo_file), "%s_%s", (char *)RA_INFO_FILE, ifname);
	if (access(rainfo_file, 0) < 0)
	{
		return 0;
	}
	snprintf(cmd, sizeof(cmd), "rm %s", rainfo_file);
	printf("%s: cmd = %s\n", __func__, cmd);
	system(cmd);
	return 0;
}
#endif 

int checkIPv6Route(MIB_CE_IPV6_ROUTE_Tp new_entry)
{
	MIB_CE_IPV6_ROUTE_T Entry;
	unsigned int totalEntry = mib_chain_total(MIB_IPV6_ROUTE_TBL);
    int i;

	for (i=0; i<totalEntry; i++) {
		if (mib_chain_get(MIB_IPV6_ROUTE_TBL, i, (void *)&Entry)) {
			if( (strcmp(new_entry->Dstination,Entry.Dstination)==0 ) &&
                (strcmp(new_entry->NextHop,Entry.NextHop)==0 ) &&
                (new_entry->DstIfIndex == Entry.DstIfIndex ) &&
                (new_entry->Enable == Entry.Enable) &&
                (new_entry->FWMetric == Entry.FWMetric))
			return 0;
		}
	}

	return 1;
}

int getIPv6addrInfo(MIB_CE_ATM_VC_Tp entryp, char *ipaddr, char *netmask, char *gateway)
{
	char ifname[IFNAMSIZ];
	int flags, k, flags_found, isPPP;
	struct ipv6_ifaddr ipv6_addr[6];
	unsigned char zero[IP6_ADDR_LEN] = {0};

	strcpy(ipaddr, "");
	strcpy(netmask, "");
	strcpy(gateway, "");
	if ((entryp->IpProtocol & IPVER_IPV6) == 0)
		return -1;

	ifGetName(entryp->ifIndex, ifname, sizeof(ifname));
	flags_found = getInFlags(ifname, &flags);

	switch (entryp->cmode) {
		case CHANNEL_MODE_BRIDGE:
			isPPP = 0;
			break;
		case CHANNEL_MODE_IPOE:
			isPPP = 0;
			break;
		case CHANNEL_MODE_PPPOE:	//patch for pppoe proxy
			isPPP = 1;
			break;
		case CHANNEL_MODE_PPPOA:
			isPPP = 1;
			break;
		default:
			isPPP = 0;
			break;
	}

	k = getifip6(ifname, IPV6_ADDR_UNICAST, ipv6_addr, 6);
	if(k > 0){
		inet_ntop(AF_INET6, &ipv6_addr[0].addr, ipaddr, INET6_ADDRSTRLEN);
		sprintf(netmask, "%d\0", ipv6_addr[0].prefix_len);
	}

	if(entryp->cmode != CHANNEL_MODE_BRIDGE && entryp->IpProtocol & IPVER_IPV6)
	{
		if(memcmp(entryp->RemoteIpv6Addr, zero, IP6_ADDR_LEN)){
			inet_ntop(AF_INET6, &entryp->RemoteIpv6Addr, gateway, INET6_ADDRSTRLEN);
		}
	}

	return 0;
}

void get_dns6_by_wan(MIB_CE_ATM_VC_T *pEntry, char *dns1, char *dns2)
{
	if ((pEntry->Ipv6Dhcp == 1) || (pEntry->Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)
			|| pEntry->AddrMode == IPV6_WAN_STATIC)
	{
		FILE* infdns;
		char file[64] = {0};
		char line[128] = {0};
		char ifname[IFNAMSIZ] = {0};

		ifGetName(pEntry->ifIndex,ifname,sizeof(ifname));

		/* Now we all use DNS6_RESOLV file to get DNS info with dynamic and static dns. so only get DNS from DNS6_RESOLV */
		snprintf(file, 64, "%s.%s", (char *)DNS6_RESOLV, ifname);

		infdns=fopen(file,"r");
		if(infdns)
		{
			int cnt = 0;

			while(fgets(line,sizeof(line),infdns) != NULL)
			{
				char *new_line = NULL;

				new_line = strrchr(line, '\n');
				if(new_line)
					*new_line = '\0';

				if((strlen(line)==0))
					continue;

				if(cnt == 0)
					strcpy(dns1, line);
				else
				{
					strcpy(dns2, line);
					break;
				}

				cnt++;
			}
			fclose(infdns);
		}
	}
}

#include <ifaddrs.h>
#include <netinet/icmp6.h>
typedef struct
{
    struct nd_router_solicit hdr;
    struct nd_opt_hdr opt;
    uint8_t hw_addr[6];
} solicit_packet;

int get_if_link_local_addr6(const char *ifname, struct sockaddr_in6 *addr6)
{   
    struct ifaddrs *ifaddr, *ifa;   
    char ip_str[64];
    struct in6_addr *tmp;
    int ok = 0;

    if(!ifname || !addr6)
        return 0;

    if (getifaddrs(&ifaddr) < 0)
    {
        perror("getifaddrs");
        return -1; 
    }

    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {       
        if(ifa->ifa_name && !strcmp(ifa->ifa_name, ifname) && 
            ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET6)
        {   
            tmp = &((struct sockaddr_in6 *)(ifa->ifa_addr))->sin6_addr;
            if(IN6_IS_ADDR_LINKLOCAL(tmp))
            {
                memcpy(addr6, ifa->ifa_addr, sizeof(struct sockaddr_in6)); 
                ok = 1;
                break;
            }
        }
    }   
    freeifaddrs(ifaddr);  
    if(!ok)
    {
        printf("Not find the interface(%s) address\n", ifname);
        return -1;
    }    
    return 0;
}

int sendRouterSolicit(char *ifname)
{
	int s = -1, hopLimit=0;
	struct sockaddr_in6 src;
	solicit_packet packet;
	struct sockaddr_in6 dst;
	ssize_t plen;
	char str[INET6_ADDRSTRLEN+1] = {0};

	if(!ifname) return -1;

	s = socket(PF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if(s < 0)
	{
		perror("socket create");
		return -1;
	}

	memset(&src, 0, sizeof(src));
	if(get_if_link_local_addr6(ifname, &src) < 0) //don't use getifip6, because bind socket will return error 
	{
		close(s);
		perror("get ipv6 link local address");
		return -1;
	}

	if(bind(s, (struct sockaddr *)&src, sizeof(src)) < 0)
	{
		perror("bind");
		close(s);
		return -1;
	}

	hopLimit = 255;
	if(setsockopt (s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hopLimit, sizeof (hopLimit)) != 0)
	{
		printf("setsocketopt failed: %s %d\n", __func__, __LINE__);
	}

	memcpy(&dst, &src, sizeof (dst));  /* copy some address info */
	memcpy(dst.sin6_addr.s6_addr, "\xff\x02\x00\x00\x00\x00\x00\x00"
					"\x00\x00\x00\x00\x00\x00\x00\x01", 16);

	/* builds ICMPv6 Router Solicitation packet */
	packet.hdr.nd_rs_type = ND_ROUTER_SOLICIT;
	packet.hdr.nd_rs_code = 0;
	packet.hdr.nd_rs_cksum = 0; /* computed by the kernel */
	packet.hdr.nd_rs_reserved = 0;

	/* gets our own interface's link-layer address (MAC) */
	if (getMacAddr(ifname, packet.hw_addr)){
		plen = sizeof(packet.hdr);
	}
	else{
		packet.opt.nd_opt_type = ND_OPT_SOURCE_LINKADDR;
		packet.opt.nd_opt_len = 1; /* 8 bytes */
		plen = sizeof(packet);
	}

	if (sendto(s, &packet, plen, 0, (const struct sockaddr *)&dst, sizeof(dst)) != plen)
	{
		perror("Sending ICMPv6 packet");
		close(s);
		return -1;
	}
	close(s);

	return 0;
}

typedef struct
{
	struct nd_neighbor_solicit hdr;
	struct nd_opt_hdr opt;
	uint8_t hw_addr[6];
} solicit_n_packet;

#include <sys/ioctl.h>
static int get_ipv6_byname(const char *name, const char *ifname, int numeric, struct sockaddr_in6 *addr)
{
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET6;
	hints.ai_socktype = SOCK_DGRAM; /* dummy */
	hints.ai_flags = numeric ? AI_NUMERICHOST : 0;

	int val = getaddrinfo(name, NULL, &hints, &res);
	if(val) {
		fprintf(stderr, "%s: %s\n", name, gai_strerror(val));
		return -1;
	}
	memcpy(addr, res->ai_addr, sizeof(struct sockaddr_in6));
	freeaddrinfo(res);

	uint8_t *ip = addr->sin6_addr.s6_addr;
	//printf("node: %s\n", name);
	//printf("ipv6: %x%x:%x%x:%x%x:%x%x:%x%x:%x%x:%x%x:%x%x\n",
	//		  ip[0], ip[1], ip[2], ip[3], ip[4], ip[5], ip[6], ip[7],
	//		  ip[8], ip[9], ip[10], ip[11], ip[12], ip[13], ip[14], ip[15]);

	val = if_nametoindex(ifname);
	if(val == 0) {
		perror(ifname);
		return -1;
	}
	addr->sin6_scope_id = val;

	return 0;
}

void rtk_ipv6_get_prefix(struct  in6_addr *ip6Addr)
{
	PREFIX_V6_INFO_T prefixInfo={0};

	get_prefixv6_info(&prefixInfo);
	memcpy(ip6Addr->s6_addr,prefixInfo.prefixIP,8);
}

void rtk_ipv6_get_prefix_len(unsigned int *prefix_len)
{
	PREFIX_V6_INFO_T prefixInfo={0};

	get_prefixv6_info(&prefixInfo);
	*prefix_len = (unsigned int)prefixInfo.prefixLen;
}

int rtk_ipv6_send_neigh_solicit(char *ifname, struct in6_addr *in6_dst)
{
	int s = -1, hopLimit=0;
	struct sockaddr_in6 src, dst;
	solicit_n_packet packet;
	ssize_t plen=0;
	char s_br[INET6_ADDRSTRLEN] = {0};
	struct ipv6_ifaddr ip6_addr[6];
	socklen_t addr_len=0;

	if(!ifname) return -1;

	s = socket(PF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if(s < 0)
	{
		perror("socket create");
		return -1;
	}

	memset(&src, 0, sizeof(src));
	getifip6((char *)ifname, IPV6_ADDR_UNICAST, ip6_addr, 6);
	inet_ntop(AF_INET6, &ip6_addr[0].addr, s_br, sizeof (s_br));
	//printf("%s src ip: %s\n", __func__, s_br);
	if (get_ipv6_byname(s_br, ifname, 1, &src))
	{
		perror("get br0 ip failed");
		close(s);
		return -1;
	}
	/*
	if(get_if_link_local_addr6(ifname, &src) < 0) //don't use getifip6, because bind socket will return error
	{
		close(s);
		perror("get ipv6 link local address");
		return -1;
	}
	*/

	addr_len = sizeof(src);
	if(bind(s, (struct sockaddr *)&src, addr_len) < 0)
	{
		perror("bind");
		close(s);
		return -1;
	}

	hopLimit = 255;
	if(setsockopt (s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hopLimit, sizeof (hopLimit)) != 0)
	{
		printf("setsocketopt failed: %s %d\n", __func__, __LINE__);
	}

	memcpy(&dst, &src, sizeof (struct sockaddr_in6));  /* copy some address info */
	memcpy(&dst.sin6_addr, in6_dst, sizeof(struct in6_addr));

	/* builds ICMPv6 Neighbor Solicitation packet */
	packet.hdr.nd_ns_type = ND_NEIGHBOR_SOLICIT;
	packet.hdr.nd_ns_code = 0;
	packet.hdr.nd_ns_cksum = 0; /* computed by the kernel */
	packet.hdr.nd_ns_reserved = 0;
	memcpy(&packet.hdr.nd_ns_target, &dst.sin6_addr, 16);

	/* determines actual multicast destination address */
	memcpy(dst.sin6_addr.s6_addr, "\xff\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\xff", 13);

	/* gets our own interface's link-layer address (MAC) */
	if (getMacAddr(ifname, packet.hw_addr)){
		plen = sizeof(packet.hdr);
	}
	else{
		packet.opt.nd_opt_type = ND_OPT_SOURCE_LINKADDR;
		packet.opt.nd_opt_len = 1; /* 8 bytes */
		plen = sizeof(packet);
	}

	addr_len = sizeof(dst);
	if (sendto(s, &packet, plen, 0, (const struct sockaddr *)&dst, addr_len) != plen)
	{
		//perror("Sending ICMPv6 packet");
		close(s);
		return -1;
	}
	close(s);

	return 0;
}

#define DEFAULT_PING_DATA_SIZE	56
int  rtk_ipv6_send_ping_request(char *ifname, struct in6_addr *in6_dst)
{
	char buffer[DEFAULT_PING_DATA_SIZE + sizeof(struct icmp6_hdr)] = {0};
	struct icmp6_hdr *pkt;
	int s = -1, hopLimit=0;
	struct sockaddr_in6 src, dst;
	char s_br[INET6_ADDRSTRLEN+1] = {0};
	struct ipv6_ifaddr ip6_addr[6];
	ssize_t plen=0;
	socklen_t addr_len=0;

	s = socket(PF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if(s < 0)
	{
		perror("socket create");
		return -1;
	}

	memset(&src, 0, sizeof(src));
	getifip6((char *)ifname, IPV6_ADDR_UNICAST, ip6_addr, 6);
	inet_ntop(AF_INET6, &ip6_addr[0].addr, s_br, sizeof (s_br));
	//printf("%s src ip: %s\n", __func__, s_br);
	if (get_ipv6_byname(s_br, ifname, 1, &src))
	{
		perror("get br0 ip failed");
		close(s);
		return -1;
	}

	/*
	if(get_if_link_local_addr6(ifname, &src) < 0) //don't use getifip6, because bind socket will return error
	{
		close(s);
		perror("get ipv6 link local address");
		return -1;
	}
	*/
	src.sin6_family = AF_INET6;
	addr_len = sizeof(src);

	if(bind(s, (struct sockaddr *)&src, addr_len) < 0)
	{
		perror("bind");
		close(s);
		return -1;
	}

	hopLimit = 255;
	if(setsockopt (s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hopLimit, sizeof (hopLimit)) != 0)
	{
		printf("setsocketopt failed: %s %d\n", __func__, __LINE__);
	}

	memset(&dst, 0, sizeof(dst));
	memcpy(&dst.sin6_addr, in6_dst, sizeof(struct in6_addr));
	dst.sin6_family = AF_INET6;

	pkt = (struct icmp6_hdr *)buffer;
	pkt->icmp6_type = ICMP6_ECHO_REQUEST;
	pkt->icmp6_id = getpid();
	plen = DEFAULT_PING_DATA_SIZE + sizeof(struct icmp6_hdr);

	addr_len = sizeof(dst);
	if (sendto(s, buffer, plen, 0, (struct sockaddr *)&dst, addr_len) != plen)
	{
		//perror("Sending ICMPv6 ping packet");
		close(s);
		return -1;
	}
	close(s);

	return 0;
}

int set_lan_ipv6_global_address(MIB_CE_ATM_VC_Tp pEntry, PREFIX_V6_INFO_Tp prefixInfo, char *infname, int prefix_route,int set)
{
	struct	in6_addr ip6Addr_local;
	unsigned char devAddr[MAC_ADDR_LEN];
	unsigned char meui64[8];
	unsigned char value[64];
	int i=0, len;
	struct in6_addr targetIp;

	if (prefixInfo == NULL ||infname == NULL )
	{
		printf("[%s:%d] Parameter Err!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if (prefixInfo->prefixLen ==0 || !memcmp(prefixInfo->prefixIP, ipv6_unspec, IP6_ADDR_LEN))
	{
		printf("[%s:%d] prefix info is NULL, set %s global_address err!\n", __FUNCTION__, __LINE__, infname);
		return -1;
	}
	ip6toPrefix(prefixInfo->prefixIP, prefixInfo->prefixLen, &targetIp);

	if ((prefixInfo->prefixLen >0) && (prefixInfo->prefixLen <=64))  {
		memcpy(ip6Addr_local.s6_addr,&targetIp,8);
		mib_get(MIB_ELAN_MAC_ADDR, (void *)devAddr);
		mac_meui64(devAddr, meui64);
		for (i=0; i<8; i++)
			ip6Addr_local.s6_addr[i+8] = meui64[i];
		inet_ntop(PF_INET6, &ip6Addr_local, value, sizeof(value));
		len=sizeof(value)-strlen(value);
		snprintf(value+strlen(value), len, "/%d", 64);
	}else if ((prefixInfo->prefixLen >64)&& (prefixInfo->prefixLen <=128)){
		inet_ntop(PF_INET6, &targetIp, value, sizeof(value));
		len = strlen(value);
		snprintf(value+len, sizeof(value)-len, "1/%d", prefixInfo->prefixLen);
	}
	else{
		printf("[%s:%d] prefix len err, set %s global_address err!\n", __FUNCTION__, __LINE__, infname);
		return -1;
	}

	if (set){
		//lan phy port need not add prefix route in main table
		if (prefix_route){
			va_cmd_no_error(BIN_IP, 6, 1, "-6", "addr", "del", value, "dev", infname);
			va_cmd(BIN_IP, 6, 1, "-6", "addr", "add", value, "dev", infname);
		}
		else{
			va_cmd_no_error(BIN_IP, 7, 1, "-6", "addr", "del", value, "dev", infname, "noprefixroute");
			va_cmd(BIN_IP, 7, 1, "-6", "addr", "add", value, "dev", infname, "noprefixroute");
		}
	}
	else{
		va_cmd_no_error(BIN_IP, 6, 1, "-6", "addr", "del", value, "dev", infname);
	}

	return 0;
}

#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
int set_ipv6_all_lan_phy_inf_forwarding(int set)
{
	char tmp_cmd[256] = {0};
	int j = 0;

	for (j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j)
	{
		snprintf(tmp_cmd, sizeof(tmp_cmd), "/bin/echo %d > /proc/sys/net/ipv6/conf/%s/forwarding", set, SW_LAN_PORT_IF[j]);
		system(tmp_cmd);
	}

#ifdef WLAN_SUPPORT
	for (j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)
	{
		if(wlan_en[j-PMAP_WLAN0] == 0)
			continue;
		snprintf(tmp_cmd, sizeof(tmp_cmd), "/bin/echo %d > /proc/sys/net/ipv6/conf/%s/forwarding", set, wlan[j-PMAP_WLAN0]);
		system(tmp_cmd);
	}
#endif
	return 1;
}

int config_ipv6_kernel_flag_and_static_lla_for_inf(char *infname)
{
	char tmp_cmd[256] = {0};
	unsigned char ipv6_lan_ip[INET6_ADDRSTRLEN] = {0};
	struct ipv6_ifaddr ipv6_addr = {0};
	int numOfIpv6 = 0;
	
	snprintf(tmp_cmd, sizeof(tmp_cmd), "/bin/echo %d > /proc/sys/net/ipv6/conf/%s/accept_ra", 0, infname);
	system(tmp_cmd);
	snprintf(tmp_cmd, sizeof(tmp_cmd), "/bin/echo %d > /proc/sys/net/ipv6/conf/%s/addr_gen_mode", 1, infname);
	system(tmp_cmd);
	/* forwarding node will make kernel learn neighbor when receive first packet (ex:RS packet), otherwise, phy
	* port eth0.x.0 won't send solicited RA packet because of destination unreachable with radvd. */
	snprintf(tmp_cmd, sizeof(tmp_cmd), "/bin/echo %d > /proc/sys/net/ipv6/conf/%s/forwarding", 1, infname);
	system(tmp_cmd);
	snprintf(tmp_cmd, sizeof(tmp_cmd), "/bin/echo %d > /proc/sys/net/ipv6/conf/%s/disable_ipv6", 0, infname);
	system(tmp_cmd);

	//echo 0 to disable_ipv6 first, and then add ipv6 address,otherwise it will prompt "Permission denied"
	mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)ipv6_lan_ip, sizeof(ipv6_lan_ip));
	if (ipv6_lan_ip[0] == '\0')
	{
		//get system LLA IP of br0
		numOfIpv6 = getifip6((char *)LANIF, IPV6_ADDR_LINKLOCAL, &ipv6_addr, 1);
		if (numOfIpv6 > 0)
		{
			inet_ntop(AF_INET6, &ipv6_addr.addr, ipv6_lan_ip, INET6_ADDRSTRLEN);
			if (ipv6_lan_ip[0] == '\0')
				printf("%s: Can't get %s LLA!\n", __FUNCTION__, LANIF);
			else
				//Need not add fe80::1/128 for lan phy port in main route table
				va_cmd(BIN_IP, 7, 1, "-6", "addr", "add", ipv6_lan_ip, "dev", infname, "noprefixroute");
		}
	}
	else
		va_cmd(BIN_IP, 7, 1, "-6", "addr", "add", ipv6_lan_ip, "dev", infname, "noprefixroute");

	return 1;
}

int add_oif_lla_route_by_lanif(char *lan_ifname)
{
	char tbl_id[10] = {0};
	char lan_phy_port_oif_pri_policyrt[16] = {0};

	if (lan_ifname == NULL)
	{
		printf("%s: NULL lan_ifname!\n", __FUNCTION__);
		return -1;
	}

	snprintf(tbl_id, sizeof(tbl_id), "%d", LAN_PHY_PORT_LLA_ROUTE);
	snprintf(lan_phy_port_oif_pri_policyrt, sizeof(lan_phy_port_oif_pri_policyrt), "%d", LAN_PHY_PORT_OIF_PRI_POLICYRT);
	//first del rule for avoiding add repeately ip rules.
	va_cmd_no_error(BIN_IP, 9, 1, "-6", "rule", "del", "oif", lan_ifname , "pref", lan_phy_port_oif_pri_policyrt, "table", tbl_id);
	va_cmd(BIN_IP, 9, 1, "-6", "rule", "add", "oif", lan_ifname , "pref", lan_phy_port_oif_pri_policyrt, "table", tbl_id);
	va_cmd(BIN_IP, 8, 1, "-6", "route", "append", "fe80::/64", "dev", lan_ifname, "table", tbl_id);

	return 1;
}

int del_bind_lan_oif_policy_route(char *lan_inf)
{
	FILE *fp = NULL;
	char tmp_cmd[128] = {0};
	char buf[256] = {0}, cmd_buf[256] = {0};
	char tbl_id[10] = {0};

	if (lan_inf[0] == '\0')
		return -1;
	//example: 15000:  from from all oif eth0.3.0 lookup 257
	//awk will cut string to be "from all oif eth0.3.0 lookup 257"
	snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 rule | grep %d | grep oif | grep %s |grep \"lookup %d\" | awk '{ $1=\"\"; print $0}'", LAN_PHY_PORT_OIF_PRI_POLICYRT, lan_inf, LAN_PHY_PORT_LLA_ROUTE);
	snprintf(tbl_id, sizeof(tbl_id), "%d", LAN_PHY_PORT_LLA_ROUTE);

	va_cmd(BIN_IP, 8, 1, "-6", "route", "del", "fe80::/64", "dev", lan_inf, "table", tbl_id);

	if ((fp = popen(tmp_cmd, "r")) != NULL)
	{
		while(fgets(buf, sizeof(buf), fp))
		{
			snprintf(cmd_buf, sizeof(cmd_buf), "ip -6 rule del %s", buf);
			system(cmd_buf);
		}

		pclose(fp);
		return 1;
	}
	else
		return -1;
}
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
//in case vlan mapping lan interface(MIB_PORT_BINDING_TBL) have been deleted, the old rule will be change to
//15000:	from all oif eth0.2.700 [detached] lookup 256
//and we need clear all the "detached" rule
int del_vlan_bind_lan_detached_policy_route()
{
	FILE *fp = NULL;
	char tmp_cmd[128] = {0};
	char buf[256] = {0}, cmd_buf[256] = {0};

	snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 rule | grep %d | grep oif | grep detached |grep \"lookup %d\" | awk '{print  $2,$3,$4,$5}'", 
			LAN_PHY_PORT_OIF_PRI_POLICYRT, LAN_PHY_PORT_LLA_ROUTE);
	if ((fp = popen(tmp_cmd, "r")) != NULL)
	{
		while(fgets(buf, sizeof(buf), fp))
		{
			snprintf(cmd_buf, sizeof(cmd_buf), "ip -6 rule del %s", buf);
			system(cmd_buf);
		}
		pclose(fp);
	}
	return 0;
}
#endif

int del_all_bind_lan_oif_policy_route(void)
{
	FILE *fp = NULL;
	char tmp_cmd[128] = {0};
	char buf[256] = {0}, cmd_buf[256] = {0};
	char tbl_id[10] = {0};

	//example: 15000:  from from all oif eth0.3.0 lookup 257
	//awk will cut string to be "from all oif eth0.3.0 lookup 257"
	snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 rule | grep %d | grep oif | grep \"lookup %d\" | awk '{ $1=\"\"; print $0}'", LAN_PHY_PORT_OIF_PRI_POLICYRT, LAN_PHY_PORT_LLA_ROUTE);
	snprintf(tbl_id, sizeof(tbl_id), "%d", LAN_PHY_PORT_LLA_ROUTE);

	va_cmd(BIN_IP, 5, 1, "-6", "route", "flush", "table", tbl_id);

	if ((fp = popen(tmp_cmd, "r")) != NULL)
	{
		while(fgets(buf, sizeof(buf), fp))
		{
			snprintf(cmd_buf, sizeof(cmd_buf), "ip -6 rule del %s", buf);
			system(cmd_buf);
		}

		pclose(fp);
		return 1;
	}
	else
		return -1;
}

int close_ipv6_kernel_flag_for_bind_inf(char *infname)
{
	char tmp_cmd[256] = {0};

	snprintf(tmp_cmd, sizeof(tmp_cmd), "/bin/echo %d > /proc/sys/net/ipv6/conf/%s/accept_ra", 0, infname);
	system(tmp_cmd);
	snprintf(tmp_cmd, sizeof(tmp_cmd), "/bin/echo %d > /proc/sys/net/ipv6/conf/%s/addr_gen_mode", 0, infname);
	system(tmp_cmd);
	snprintf(tmp_cmd, sizeof(tmp_cmd), "/bin/echo %d > /proc/sys/net/ipv6/conf/%s/forwarding", 0, infname);
	system(tmp_cmd);
	snprintf(tmp_cmd, sizeof(tmp_cmd), "/bin/echo %d > /proc/sys/net/ipv6/conf/%s/disable_ipv6", 1, infname);
	system(tmp_cmd);

	return 1;
}

#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
int lan_vlan_mapping_inf_handle(struct lan_vlan_inf_msg *msg_header,MIB_CE_ATM_VC_Tp pEntry)
{
	int i=0;
	const char *rifName = NULL;
	uint32_t logPort;
	int32_t totalPortbd ,k;
	MIB_CE_PORT_BINDING_T portBindEntry;
	struct vlan_pair *vid_pair;
	char ifname[IFNAMSIZ+1]={0};
#ifdef WLAN_SUPPORT
	int ssidIdx = -1;
#endif

	if((msg_header == NULL) ||(pEntry==NULL)){
		printf("[%s:%d] parameter err !\n", __FUNCTION__, __LINE__);
		return -1;
	}
	
	if (!pEntry->vlan)
		return -1;
	if (!(pEntry->IpProtocol & IPVER_IPV6))
		return -1;
	if (pEntry->cmode == CHANNEL_MODE_BRIDGE) //dont send RA to port which bind bridge WAN
		return -1;

	totalPortbd = mib_chain_total(MIB_PORT_BINDING_TBL);
	//polling every lanport vlan-mapping entry
	for (i = 0; i < totalPortbd; ++i){
		if(!mib_chain_get(MIB_PORT_BINDING_TBL, i, (void*)&portBindEntry))
			continue;
		logPort = portBindEntry.port;
		if((unsigned char)VLAN_BASED_MODE == portBindEntry.pb_mode){	
			vid_pair = (struct vlan_pair *)&portBindEntry.pb_vlan0_a;
			for (k=0; k<4; k++)				{
				//support lan/wlan vlan bind untag or tag wan 
				if(!(vid_pair[k].vid_a > 0 && pEntry->vid == vid_pair[k].vid_b)){
					continue;
				}
				
				if(logPort < PMAP_WLAN0 && logPort < ELANVIF_NUM)
					rifName = ELANRIF[logPort];
#ifdef WLAN_SUPPORT
				else if((ssidIdx = (logPort-PMAP_WLAN0)) < (WLAN_SSID_NUM*NUM_WLAN_INTERFACE) && ssidIdx >= 0)
					rifName = wlan[ssidIdx];
#endif
				else 
					rifName = NULL;
		
				if(rifName == NULL || !getInFlags(rifName, NULL))
					continue;
				
				ifname[0] = '\0';
				snprintf(ifname, IFNAMSIZ , "%s.%d", rifName, vid_pair[k].vid_a);
				if(!getInFlags(ifname, NULL))
					continue;
		
				printf("[%s:%d] interface:%s is vlan binding to %d \n", __FUNCTION__, __LINE__, ifname,pEntry->ifIndex );

				switch(msg_header->lan_vlan_inf_event) {
					case LAN_VLAN_INTERFACE_DHCPV6S_ADD: 
						config_ipv6_kernel_flag_and_static_lla_for_inf(ifname);
						set_lan_ipv6_global_address(NULL,  &msg_header->PrefixInfo, ifname,0,1);
						add_oif_lla_route_by_lanif(ifname);
						break;
					case LAN_VLAN_INTERFACE_DHCPV6S_CMD:
						snprintf(msg_header->inf_cmd+strlen(msg_header->inf_cmd), sizeof(msg_header->inf_cmd), " %s",ifname);
						break;
					case LAN_VLAN_INTERFACE_RADVD_ADD: 
						setup_radvd_conf_by_lan_ifname(ifname,&msg_header->DnsV6Info, &msg_header->PrefixInfo);
						add_oif_lla_route_by_lanif(ifname);
						config_ipv6_kernel_flag_and_static_lla_for_inf(ifname);
						break;
					case LAN_VLAN_INTERFACE_DEL:
						va_cmd("/usr/bin/ip",7 , 1, "-6", "addr", "flush", "dev",  ifname, "scope", "global");
						del_bind_lan_oif_policy_route(ifname);
						close_ipv6_kernel_flag_for_bind_inf(ifname);
						break;
					default:
					break;
				}
			}
		}
	}
	return 0;
}
#endif

/*
int delete_wide_dhcp6s_by_ifname(char *lan_inf)
{
	char pid_str[64] = {0};
	int dhcp6s_pid = -1, retry = 0;
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
	snprintf(pid_str, sizeof(pid_str), "%s_%s", DHCPD6S_PID, lan_inf);
#else
	snprintf(pid_str, sizeof(pid_str), "/var/run/dhcp6s.pid_%s", lan_inf);
#endif

	kill_by_pidfile(pid_str, SIGTERM);

	return 1;
}
*/


/*
 * Now delete_binding_lan_service_by_wan do 2 things.
 *  1.Del this wan's bind lan oif policy route
 *  2.Del this wan's bind lan wide DHCP6 Server program
 * */
int delete_binding_lan_service_by_wan(MIB_CE_ATM_VC_Tp pEntry)
{
	int j = 0;

	for (j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < SW_LAN_PORT_NUM; ++j){
		if (BIT_IS_SET(pEntry->itfGroup, j)){
			printf("[%s:%d] delete lan service interface:%s is binding to %d \n", __FUNCTION__, __LINE__, SW_LAN_PORT_IF[j], pEntry->ifIndex);
			del_bind_lan_oif_policy_route((char *)SW_LAN_PORT_IF[j]);
			//delete_wide_dhcp6s_by_ifname((char *)SW_LAN_PORT_IF[j]);
			close_ipv6_kernel_flag_for_bind_inf((char *)SW_LAN_PORT_IF[j]);
		}
	}
#ifdef WLAN_SUPPORT
	for (j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)	{
		if (BIT_IS_SET(pEntry->itfGroup, j)){
			if (wlan_en[j-PMAP_WLAN0] == 0)
				continue;
			printf("[%s:%d] delete lan service interface:%s is binding to %d \n", __FUNCTION__, __LINE__, wlan[j - PMAP_WLAN0], pEntry->ifIndex );
			del_bind_lan_oif_policy_route((char *)wlan[j - PMAP_WLAN0]);
			//delete_wide_dhcp6s_by_ifname((char *)wlan[j - PMAP_WLAN0]);
			close_ipv6_kernel_flag_for_bind_inf((char *)wlan[j - PMAP_WLAN0]);
		}
	}
#endif
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
	//vlan mapping interface
	struct lan_vlan_inf_msg msg_header;
	memset(&msg_header, 0, sizeof(msg_header));
	msg_header.lan_vlan_inf_event = LAN_VLAN_INTERFACE_DEL;
	lan_vlan_mapping_inf_handle(&msg_header,pEntry);
	//in case vlan mapping lan interface(MIB_PORT_BINDING_TBL) have been deleted, the old rule will be change to
	//15000:	from all oif eth0.2.700 [detached] lookup 256
	//and we need clear all the "detached" rule
	del_vlan_bind_lan_detached_policy_route();
#endif
	return 0;
}
#endif

#ifdef CONFIG_USER_RADVD
int setup_radvd_conf_by_lan_ifname(char* lanIfname, DNS_V6_INFO_Tp pDnsV6Info, PREFIX_V6_INFO_Tp pPrefixInfo)
{
	FILE *fp = NULL;
	unsigned char str[MAX_RADVD_CONF_PREFIX_LEN] = {0};
	unsigned char vChar = 0, vChar2 = 0, m_flag=0, o_flag=0,ipv6_lan_mtu_mode=0, mInterleave=0;
	int radvdpid = 0;
	unsigned char Ipv6AddrStr[48] = {0};
	unsigned char mode = 0;
	unsigned char prefix_enable = 0;
	int mtu = 0, tmp_wan_mtu = 0;
	char wan_ifname[IFNAMSIZ] = {0};
	int idx=0;
	unsigned char domainSearch[MAX_DOMAIN_NUM*MAX_DOMAIN_LENGTH] = {0};
	unsigned char dhcpv6_mode = 0 ;
	unsigned char ula_mode = 0 ;
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	unsigned char logoTestMode = 0;
#endif

	if(!pDnsV6Info || !pPrefixInfo) {
		printf("Error! NULL parameter input %s\n", __FUNCTION__);
		return -1;
	 }
#ifdef CONFIG_CMCC_IPV6_SECURITY_SUPPORT
	flush_old_ip6sec_prefix(RADVD_CONF_TMP, pPrefixInfo);
#endif
	if ((fp = fopen(RADVD_CONF_TMP, "a")) == NULL)
	{
		printf("Open file %s failed !\n", RADVD_CONF_TMP);
		return -1;
	}

	//should be accept brX or eth.x or eth0.x.0
	fprintf(fp, "interface %s\n", lanIfname);
	fprintf(fp, "{\n");

	// AdvSendAdvert
	if ( !mib_get_s( MIB_V6_SENDADVERT, (void *)&vChar, sizeof(vChar)) )
		printf("Get MIB_V6_SENDADVERT error!");
	if (0 == vChar)
		fprintf(fp, "\tAdvSendAdvert off;\n");
	else if (1 == vChar)
		fprintf(fp, "\tAdvSendAdvert on;\n");

	/* For LAN-RFC4861 Test CERouter 2.4.7, we have already modified NCE state of kernel.
	   Now kernel will set NCE REACHABLE if ont receives RS from test node in CE ROUTER logotest mode. 20220602 */
#if 0
#if defined (CONFIG_RTK_IPV6_LOGO_TEST) && defined(CONFIG_USER_RADVD_2_17)
 	/*CE router LAN-RFC4861 Test CERouter 2.4.7 : Neighbor Solicitation Processing, NCE State REACHABLE
	* Test bed will check NCE state , if radvd use unicast , the NCE state will mismatch with test script
	* About these case , MAX_INITIAL_RTR_ADVERT_INTERVAL in radvd-2.17/defaults.h aslo need be reduce to 5 (default is 16),
	*Otherwise, Test bed send RS , dut sometime can not send RA in time.
	*/
	if( mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode))){
		/*
		 * Set RASolicitedUnicast "ON" will effect LAN-RFC4861 Test CERouter 2.4.7 failure due to kernel will learn A5 node as
	     * neighbor after receiving A5 RS and sending unicast RA. Kernel will set this NCE to STALE, and it will set NUD state
	     * to DELAY after 5 seconds. Finally this test case will fail as it receives any Unicast NS(Probe NS to A5). So we can't
	     * send Unicast Package(Like icmpv6 reply or RA, etc.) to this source node(A5 node). 20220520
		 */
		if(logoTestMode == 3)
			fprintf(fp, "\tAdvRASolicitedUnicast off;\n");
	}
#endif
#endif
	// MaxRtrAdvIntervalAct
	if ( !mib_get_s(MIB_V6_MAXRTRADVINTERVAL, (void *)str, sizeof(str))) {
		printf("Get MaxRtrAdvIntervalAct mib error!");
	}
	if (str[0]) {
		fprintf(fp, "\tMaxRtrAdvInterval %s;\n", str);
	}

	// MinRtrAdvIntervalAct
	if ( !mib_get_s(MIB_V6_MINRTRADVINTERVAL, (void *)str, sizeof(str))) {
		printf("Get MinRtrAdvIntervalAct mib error!");
	}
	if (str[0]) {
		fprintf(fp, "\tMinRtrAdvInterval %s;\n", str);
	}

	// AdvCurHopLimitAct
	if ( !mib_get_s(MIB_V6_ADVCURHOPLIMIT, (void *)str, sizeof(str))) {
		printf("Get AdvCurHopLimitAct mib error!");
	}
	if (str[0]) {
		fprintf(fp, "\tAdvCurHopLimit %s;\n", str);
	}
	
	// ULAPREFIX_ENABLE
	if ( !mib_get_s (MIB_V6_ULAPREFIX_ENABLE, (void *)&ula_mode, sizeof(ula_mode))) {
		printf("Get ULAPREFIX_ENABLE mib error!");
	}
	
#ifdef CONFIG_YUEME
	if(ula_mode && mib_get_s(MIB_V6_ADVDEFAULTLIFETIME, (void *)str, sizeof(str))) {
		fprintf(fp, "\tAdvDefaultLifetime %s;\n", str);
	}
#else
	// AdvDefaultLifetime
	/* CE-ROUTER Test case 
	 * LAN-RFC7084	Part C : Preferred lifetime greater then valid lifetime
	 * if WAN IA_PD is invailed, the LAN RA router lifetime should be 0;
	 * WAN-RFC3633 Test CERouter 1.2.9 : Reception of A Reply Message for Prefix Delegation Part C/D
	 * if WAN IA_PD option lifetime is 0, the LAN RA router lifetime should be 0 and RA can not carry Prefix option
	 */
	if((pPrefixInfo->prefixLen && compare_prefixip_with_unspecip_by_bit(pPrefixInfo->prefixIP, ipv6_unspec, pPrefixInfo->prefixLen)) && (pPrefixInfo->MLTime !=0)){
		if ( !mib_get_s(MIB_V6_ADVDEFAULTLIFETIME, (void *)str, sizeof(str))) {
			printf("Get AdvDefaultLifetime mib error!");
		}
		if (str[0]) {
			fprintf(fp, "\tAdvDefaultLifetime %s;\n", str);
		}
	}
	else	{
		fprintf(fp, "\tAdvDefaultLifetime 0;\n");
	}
#endif
	// AdvReachableTime
	if ( !mib_get_s(MIB_V6_ADVREACHABLETIME, (void *)str, sizeof(str))) {
		printf("Get AdvReachableTime mib error!");
	}
	if (str[0]) {
		fprintf(fp, "\tAdvReachableTime %s;\n", str);
	}

	// AdvRetransTimer
	if ( !mib_get_s(MIB_V6_ADVRETRANSTIMER, (void *)str, sizeof(str))) {
		printf("Get AdvRetransTimer mib error!");
	}
	if (str[0]) {
		fprintf(fp, "\tAdvRetransTimer %s;\n", str);
	}

	if ( !mib_get_s( MIB_IPV6_LAN_MTU_MODE, (void *)&ipv6_lan_mtu_mode, sizeof(ipv6_lan_mtu_mode)) )
		printf("Get MIB_IPV6_LAN_MTU_MODE error!");
#ifdef CONFIG_USER_RTK_RAMONITOR
	if (get_ra_mtu_by_wan_ifindex(pPrefixInfo->wanconn, &mtu) && ipv6_lan_mtu_mode == 0) {
		ifGetName(pPrefixInfo->wanconn, wan_ifname, sizeof(wan_ifname));
		if (getInMTU((const char *)wan_ifname, &tmp_wan_mtu))
		{
			/* check if WAN RA MTU info is bigger than that WAN interface's MTU. If it is, we need to set MTU to be WAN interface's MTU. */
			if (tmp_wan_mtu > 0 && (tmp_wan_mtu < mtu || mtu == 0))
				mtu = tmp_wan_mtu;
		}
		snprintf(str, sizeof(str), "%d", mtu);
	}
	else if(ipv6_lan_mtu_mode == 1){ //get maximux wan mtu
		mtu = (int)rtk_wan_get_max_wan_mtu();
		snprintf(str, sizeof(str), "%d", mtu);
	}
	else 
#endif
	// AdvLinkMTU
	if ( !mib_get_s(MIB_V6_ADVLINKMTU, (void *)str, sizeof(str))) {
		printf("Get AdvLinkMTU mib error!");
	}
	if (str[0]) {
		mtu = atoi(str);
		if (mtu > MINIMUM_IPV6_MTU)
			fprintf(fp, "\tAdvLinkMTU %s;\n", str);
		else
			fprintf(fp, "\tAdvLinkMTU %d;\n", MINIMUM_IPV6_MTU);
	}
	else if (mtu <= MINIMUM_IPV6_MTU)
		fprintf(fp, "\tAdvLinkMTU %d;\n", MINIMUM_IPV6_MTU);

	// AdvManagedFlag
	if ( !mib_get_s( MIB_V6_MANAGEDFLAG, (void *)&vChar, sizeof(vChar)) )
		printf("Get MIB_V6_MANAGEDFLAG error!");
	if (0 == vChar)
		fprintf(fp, "\tAdvManagedFlag off;\n");
	else if (1 == vChar)
		fprintf(fp, "\tAdvManagedFlag on;\n");
	m_flag = vChar;

	mib_get_s( MIB_V6_MFLAGINTERLEAVE, (void *)&mInterleave, sizeof(mInterleave));
	
	// AdvOtherConfigFlag
	if ( !mib_get_s( MIB_V6_OTHERCONFIGFLAG, (void *)&vChar, sizeof(vChar)) )
		printf("Get MIB_V6_OTHERCONFIGFLAG error!");
	if (0 == vChar)
		fprintf(fp, "\tAdvOtherConfigFlag off;\n");
	else if (1 == vChar)
		fprintf(fp, "\tAdvOtherConfigFlag on;\n");
	o_flag = vChar;

	//NOTE: in radvd.conf
	//      Prefix/clients/route/RDNSS configurations must be given in exactly this order.

	// ULA Prefix
	if (ula_mode) {
		unsigned char ulaPrefix[MAX_RADVD_CONF_PREFIX_LEN]={0};
		unsigned char ulaPrefixlen[MAX_RADVD_CONF_LEN]={0};
		unsigned char validtime[MAX_RADVD_CONF_LEN]={0};
		unsigned char preferedtime[MAX_RADVD_CONF_LEN]={0};

		if ( !mib_get_s(MIB_V6_ULAPREFIX, (void *)ulaPrefix, sizeof(ulaPrefix))  |
			!mib_get_s(MIB_V6_ULAPREFIX_LEN, (void *)ulaPrefixlen, sizeof(ulaPrefixlen))  ||
			!mib_get_s(MIB_V6_ULAPREFIX_VALID_TIME, (void *)validtime, sizeof(validtime))  ||
			!mib_get_s(MIB_V6_ULAPREFIX_PREFER_TIME, (void *)preferedtime, sizeof(preferedtime))
		   )
		{
			printf("Get ULAPREFIX mib error!");
		}
		else	{
			fprintf(fp, "\t\n");
			fprintf(fp, "\tprefix %s/%s\n", ulaPrefix, ulaPrefixlen);
			fprintf(fp, "\t{\n");
			fprintf(fp, "\t\tAdvOnLink on;\n");
			fprintf(fp, "\t\tAdvAutonomous on;\n");
			fprintf(fp, "\t\tAdvValidLifetime %s;\n",validtime);
			fprintf(fp, "\t\tAdvPreferredLifetime %s;\n",preferedtime);
			fprintf(fp, "\t};\n");
			fprintf(fp, "\troute %s/%s\n", ulaPrefix, ulaPrefixlen);
			fprintf(fp, "\t{\n");
			fprintf(fp, "\t\tAdvRoutePreference high;\n");

#ifdef CONFIG_YUEME
			fprintf(fp, "\t\tAdvRouteLifetime %s;\n",validtime);
#else
			/*ULA-5:  An IPv6 CE router MUST NOT advertise itself as a default
			*router with a Router Lifetime greater than zero whenever all
			*of its configured and delegated prefixes are ULA prefixes.*/
			if ((pPrefixInfo->prefixLen && memcmp(pPrefixInfo->prefixIP, ipv6_unspec, pPrefixInfo->prefixLen<=128?pPrefixInfo->prefixLen:128))&& (pPrefixInfo->MLTime !=0)){
				fprintf(fp, "\t\tAdvRouteLifetime %s;\n",validtime);
			}else{
				fprintf(fp, "\t\tAdvRouteLifetime 0;\n");
			}
#endif
			fprintf(fp, "\t};\n");
#ifdef CONFIG_CMCC_IPV6_SECURITY_SUPPORT
			va_cmd(IP6SECCTRL,7,1,"-A","-a",ulaPrefix,"-l",ulaPrefixlen,"-t",preferedtime);
#endif
		}
	}

	// Prefix
	/* WAN-RFC3633 Test CERouter 1.2.9 : Reception of A Reply Message for Prefix Delegation Part C/D
	 * if WAN IA_PD option lifetime is 0, the LAN RA router lifetime should be 0 and RA can not carry Prefix option
	 */
	if ((memcmp(pPrefixInfo->prefixIP, ipv6_unspec, pPrefixInfo->prefixLen<=128?pPrefixInfo->prefixLen:128)  && pPrefixInfo->prefixLen)&& (pPrefixInfo->MLTime !=0)){
		struct  in6_addr ip6Addr;
		unsigned char devAddr[MAC_ADDR_LEN];
		unsigned char meui64[8];
		unsigned char value[64];
		int i;

		inet_ntop(PF_INET6,pPrefixInfo->prefixIP, Ipv6AddrStr, sizeof(Ipv6AddrStr));
		fprintf(fp, "\t\n");
		fprintf(fp, "\tprefix %s/%d\n", Ipv6AddrStr, (pPrefixInfo->prefixLen < 64)?64:pPrefixInfo->prefixLen);
		fprintf(fp, "\t{\n");

		// AdvOnLink
		if ( !mib_get_s( MIB_V6_ONLINK, (void *)&vChar, sizeof(vChar)) )
			printf("Get MIB_V6_ONLINK error!");
		if (0 == vChar)
			fprintf(fp, "\t\tAdvOnLink off;\n");
		else if (1 == vChar)
			fprintf(fp, "\t\tAdvOnLink on;\n");

		// AdvAutonomous
		if ( !mib_get_s( MIB_V6_AUTONOMOUS, (void *)&vChar, sizeof(vChar)) )
			printf("Get MIB_V6_AUTONOMOUS error!");
		if (m_flag == 1)
			fprintf(fp, "\t\tAdvAutonomous off;\n");
		else {
			if (0 == vChar)
				fprintf(fp, "\t\tAdvAutonomous off;\n");
			else if (1 == vChar)
				fprintf(fp, "\t\tAdvAutonomous on;\n");
		}

		// AdvValidLifetime
			fprintf(fp, "\t\tAdvValidLifetime %u;\n", pPrefixInfo->MLTime);

		// AdvPreferredLifetime
			fprintf(fp, "\t\tAdvPreferredLifetime %u;\n", pPrefixInfo->PLTime);

		fprintf(fp, "\t\tDeprecatePrefix on;\n");
		fprintf(fp, "\t};\n");
#ifdef CONFIG_CMCC_IPV6_SECURITY_SUPPORT
		sprintf(value,"%d",pPrefixInfo->prefixLen);
		sprintf(str,"%u",pPrefixInfo->PLTime);
		va_cmd(IP6SECCTRL,7,1,"-A","-a",Ipv6AddrStr,"-l", value ,"-t", str);
#endif
		//CE-ROUTER Test case : LAN_RFC7084 - IPv6 CE Router Requirements  Route Information Option Part B: /60 prefix
		//need use prefixlen in dhcpv6 lease file
		//icmpv6 option: Route Information
		fprintf(fp, "\troute %s/%d\n", Ipv6AddrStr, pPrefixInfo->prefixLen);
		fprintf(fp, "\t{\n");
		fprintf(fp, "\t\tAdvRoutePreference high;\n");
		fprintf(fp, "\t\tAdvRouteLifetime %u;\n",pPrefixInfo->MLTime);
		fprintf(fp, "\t};\n");
	}

	//Add global address for br0
	/* ISC-DHCPv6 server can't work if br0 have mutilple global IPv6 address, we need to avoid
	   dhcpv6 server and radvd prefix simultaneously setting br0 global IP when prefix is not
	   the same between dhcpv6s and radvd mode. So now we will avoid setting br0 global IP with
	   radvd setting, we only set br0 global IP when dhcpv6 server not up.
	   TODO: Need hacking ISC-DHCPv6 to accept multiple br0 global IPv6 address. 20201228 */
	if (!mib_get_s(MIB_DHCPV6_MODE, (void *)&dhcpv6_mode, sizeof(dhcpv6_mode))){
		if(fp) fclose(fp);
		printf("[%s:%d] Get DHCPV6 server Mode failed\n",__func__,__LINE__);
		return 0;
	}
	if (pPrefixInfo->prefixLen && memcmp(pPrefixInfo->prefixIP, ipv6_unspec, pPrefixInfo->prefixLen<=128?pPrefixInfo->prefixLen:128)){
		if (strcmp(lanIfname, BRIF) || dhcpv6_mode == DHCP_LAN_NONE)
			set_lan_ipv6_global_address(NULL, pPrefixInfo, (char *)lanIfname,1,1);
	}

	//set RDNSS according to DNSv6 server setting
	if (m_flag == 0 && o_flag == 0 || mInterleave == 1) {
		if (pDnsV6Info && (*(unsigned char *)pDnsV6Info->nameServer)) {
			char *ptr=NULL;
			unsigned char nameServer[IPV6_BUF_SIZE_256] = {0};

			memcpy(nameServer, pDnsV6Info->nameServer, sizeof(nameServer));

			// Alan, Modify all ',' in string for RADVD CONF format
			//  Replace ',' in string to meet RADVD CONF format
			//	RDNSS ip [ip] [ip] {    list of rdnss specific options
			//	};
			ptr = nameServer;
			while (ptr = strchr(ptr,',')) {
				*ptr++=' ';
			}

			fprintf(fp, "\n\tRDNSS %s\n", nameServer);
			fprintf(fp, "\t{\n");
			fprintf(fp, "\t\tAdvRDNSSPreference 8;\n");
			fprintf(fp, "\t\tAdvRDNSSOpen off;\n");
			fprintf(fp, "\t};\n");
		}
	}

	// Set DNSSL (DNS Search List) according to DNS info
	if (pDnsV6Info->dnssl_num >0){
		for (idx=0; idx <pDnsV6Info->dnssl_num; idx++){
			snprintf(domainSearch+strlen(domainSearch), sizeof(domainSearch)-strlen(domainSearch), "%s ", pDnsV6Info->domainSearch[idx]);
		}
		printf("%s  : DNSSL have %d entry %s\n",__func__, pDnsV6Info->dnssl_num , domainSearch);
		fprintf(fp, "\n\tDNSSL %s\n", domainSearch);
		fprintf(fp,"\t{\n");
		fprintf(fp,"\t\tAdvDNSSLLifetime 600;\n");
		fprintf(fp,"\t};\n");
	}

	fprintf(fp, "};\n");
	fclose(fp);

	return 0;

}
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
static int setup_binding_lan_radvd_config_for_wan(MIB_CE_ATM_VC_Tp pEntry, DNS_V6_INFO_Tp dnsV6Info_p, PREFIX_V6_INFO_Tp prefixInfo_p)
{
	int j = 0 ;

	for (j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j)
	{
		if (BIT_IS_SET(pEntry->itfGroup, j))
		{
			printf("[%s:%d] interface:%s is binding to %d \n", __FUNCTION__, __LINE__, SW_LAN_PORT_IF[j], pEntry->ifIndex);
			setup_radvd_conf_by_lan_ifname((char *)SW_LAN_PORT_IF[j], dnsV6Info_p, prefixInfo_p);
			config_ipv6_kernel_flag_and_static_lla_for_inf((char *)SW_LAN_PORT_IF[j]);
			add_oif_lla_route_by_lanif((char *)SW_LAN_PORT_IF[j]);
//#ifdef CONFIG_NETFILTER_XT_TARGET_TEE
			//ip6tables -o eth0.x.0  -p ipv6-icmp -m icmp6 --icmpv6-type 134 -j TEE --oif br0
			//This rule is necessary for LAN RADVD downstream.
			//Because lan phy port can not learn NS/NA for host mac, but br0 , TEE target will dup RA packet and modify oif to research route(to br0)
			//So that, RADVD DS could send out from br0 to host.
			//this rule will be cleaned in clean_radvd();
			//va_cmd(IP6TABLES, 16, 1, "-t","mangle",(char *)FW_ADD, "multi_lan_radvd_ds_TEE", "-o", (char *)SW_LAN_PORT_IF[j], "-p","ipv6-icmp",
			//	"-m","icmp6", "--icmpv6-type", "134", "-j", "TEE","--oif", (char*)BRIF);
//#endif
		}
	}
#ifdef WLAN_SUPPORT
	for (j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)
	{
		if (BIT_IS_SET(pEntry->itfGroup, j))
		{
			if (wlan_en[j-PMAP_WLAN0] == 0)
				continue;
			printf("[%s:%d] interface:%s is binding to %d \n", __FUNCTION__, __LINE__, wlan[j - PMAP_WLAN0], pEntry->ifIndex );
			setup_radvd_conf_by_lan_ifname((char *)wlan[j - PMAP_WLAN0], dnsV6Info_p, prefixInfo_p);
			config_ipv6_kernel_flag_and_static_lla_for_inf((char *)wlan[j - PMAP_WLAN0]);
			add_oif_lla_route_by_lanif((char *)wlan[j-PMAP_WLAN0]);
//#ifdef CONFIG_NETFILTER_XT_TARGET_TEE
//			va_cmd(IP6TABLES, 16, 1, "-t","mangle",(char *)FW_ADD, "multi_lan_radvd_ds_TEE", "-o", (char *)wlan[j - PMAP_WLAN0], "-p","ipv6-icmp",
//				"-m","icmp6", "--icmpv6-type", "134", "-j", "TEE","--oif", (char*)BRIF);
//#endif

		}
	}
#endif

#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
	//vlan mapping interface
	struct lan_vlan_inf_msg msg_header;
	memset(&msg_header, 0, sizeof(msg_header));
	msg_header.lan_vlan_inf_event = LAN_VLAN_INTERFACE_RADVD_ADD;
	memcpy(&msg_header.DnsV6Info,dnsV6Info_p, sizeof(DNS_V6_INFO_T));
	memcpy(&msg_header.PrefixInfo,prefixInfo_p, sizeof(PREFIX_V6_INFO_T));
	lan_vlan_mapping_inf_handle(&msg_header, pEntry);
#endif

	return 1;
}

static int setup_binding_lan_radvd_conf(void)
{
	int total = mib_chain_total(MIB_ATM_VC_TBL);
	int i = 0, j = 0;
	MIB_CE_ATM_VC_T entry;
	char ifname[IFNAMSIZ] = {0}, lan_ifname[IFNAMSIZ] = {0}, lease_file[128] = {0};
	char radvdEnable = 0;
	int radvdpid, ifindex=0,retry=0;
	unsigned short itfGroup_total = 0, itfGroup_route_total = 0, itfGroup_bridge = 0;
	DNS_V6_INFO_T dnsV6Info = {0};
	PREFIX_V6_INFO_T prefixInfo = {0};
	char tmp_cmd[256] = {0};
	unsigned char lan_prefix_str[INET6_ADDRSTRLEN] = {0};
	unsigned int PFTime = 0, RNTime = 0, RBTime = 0, MLTime = 0;
	unsigned char ipv6PrefixMode=0, prefixLen[15]={0};

	//unlink(RADVD_CONF_TMP); /* now we write radvd config in radvd.conf.tmp at begin. */
	/* Initial LAN phy port interface forwarding. */
	set_ipv6_all_lan_phy_inf_forwarding(0);
//#ifdef CONFIG_NETFILTER_XT_TARGET_TEE
	//va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", "multi_lan_radvd_ds_TEE");
	//va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "OUTPUT", "-j","multi_lan_radvd_ds_TEE");
//#endif
	for(i = 0 ; i < total ; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
			continue;
		if (ifGetName(entry.ifIndex, ifname, IFNAMSIZ) == 0)
			continue;
		if (!(entry.IpProtocol & IPVER_IPV6))
			continue;

		if (entry.cmode == CHANNEL_MODE_BRIDGE) //dont send RA to port which bind bridge WAN
			continue;

		//dont send DHCPv6 packet to port which bind bridge WAN
		if (ROUTE_CHECK_BRIDGE_MIX_ENABLE(&entry))
			continue;

		memset(&prefixInfo,0,sizeof(prefixInfo));
		memset(&dnsV6Info,0,sizeof(dnsV6Info));
		//radvd conf can have no dns and no prefix
		if(get_prefixv6_info_by_wan(&prefixInfo, entry.ifIndex) != 0){
			printf("[%s:%d] get_prefixv6_info_by_wan failed wan index = %d!\n", __FUNCTION__, __LINE__, entry.ifIndex);

			//if can not get WAN PD, make binding port to get lan static prefix if user set static prefix(e.g. NAT/NAPT6 networking) 
			mib_get_s(MIB_V6_RADVD_PREFIX_MODE, (void *)&ipv6PrefixMode, sizeof(ipv6PrefixMode));
			if (ipv6PrefixMode == IPV6_PREFIX_STATIC){
				mib_get_s(MIB_V6_PREFIX_IP, (void *)lan_prefix_str, sizeof(lan_prefix_str));
				mib_get_s(MIB_V6_PREFIX_LEN, (void *)prefixLen, sizeof(prefixLen)); ////STRING_T
				if(lan_prefix_str[0]){
					inet_pton(PF_INET6, lan_prefix_str, prefixInfo.prefixIP);
					prefixInfo.prefixLen = atoi(prefixLen);
					// default-lease-time
					mib_get_s(MIB_DHCPV6S_DEFAULT_LEASE, (void *)&MLTime, sizeof(MLTime));
					prefixInfo.MLTime = MLTime;
					// preferred-lifetime
					mib_get_s(MIB_DHCPV6S_PREFERRED_LIFETIME, (void *)&PFTime, sizeof(PFTime));
					prefixInfo.PLTime = PFTime;
					mib_get_s(MIB_DHCPV6S_RENEW_TIME, (void *)&RNTime, sizeof(RNTime));
					prefixInfo.RNTime = RNTime;
					mib_get_s(MIB_DHCPV6S_REBIND_TIME, (void *)&RBTime, sizeof(RBTime));
					prefixInfo.RBTime = RBTime;
				}
			}
		}

		if(get_dnsv6_info_by_wan(&dnsV6Info, entry.ifIndex) != 0)
		{
			printf("[%s:%d] get_dnsv6_info_by_wan failed for wan index = %d!\n", __FUNCTION__, __LINE__, entry.ifIndex);
		}

		setup_binding_lan_radvd_config_for_wan(&entry, &dnsV6Info, &prefixInfo);
	}

	return 0;
}
#endif

#if (defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_) ) && defined(_SUPPORT_L2BRIDGING_DHCP_SERVER_)
void setup_interface_group_br_policy_route(PREFIX_V6_INFO_Tp prefixInfo, char* strtbl, int set, int napt_v6)
{
	PREFIX_V6_INFO_T prefixInfo_br = {0};
	int ret = 0;
	int i = 0, total = 0, prefix_len=0, flags=0;
	MIB_L2BRIDGE_GROUP_T entry = {0};
	MIB_L2BRIDGE_GROUP_Tp pEntry = &entry;
	char brname[16] = {0}, tmp_cmd[256];
	char prefix_ip[INET6_ADDRSTRLEN] = {0};
	char iprule_pri_prefixdelegatert[16] = {0};; //ipv6 ip + ::/128

	sprintf(iprule_pri_prefixdelegatert, "%d", IP_RULE_PRI_PREFIX_INTERFACE_GROUP);

	total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
	for (i = 0; i < total; i++) {
		if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, (void *)pEntry))
			continue;
		if (pEntry->enable ==0 ) continue;
		if (pEntry->groupnum !=0 ) {
			memcpy(&prefixInfo_br, prefixInfo, sizeof(PREFIX_V6_INFO_T));
			prefixInfo_br.prefixIP[7] += (pEntry->groupnum) ;
			if (prefixInfo_br.prefixLen<64)  prefixInfo_br.prefixLen=64;
			prefix_len = prefixInfo_br.prefixLen;
			if (inet_ntop(PF_INET6, (struct in6_addr *)prefixInfo_br.prefixIP, prefix_ip, sizeof(prefix_ip)))
			{
				//Since prefix provided by radvd may be different from dhcpv6, we add brX global address here.
				snprintf(brname, sizeof(brname), "br%u", pEntry->groupnum);
				snprintf(tmp_cmd, sizeof(tmp_cmd), "%s/%d", prefix_ip, prefix_len);
				if(set == 0)
				{
					AUG_PRT("[%s:%d]set (%d) is not running, dont't update policy route table(%s)\n", __FUNCTION__, __LINE__, set, strtbl);
					return;
				}
				va_cmd(BIN_IP, 9, 1, "-6", "rule", "add", "from", tmp_cmd, "pref", iprule_pri_prefixdelegatert, "lookup", strtbl);
				va_cmd(BIN_IP, 8, 1, "-6", "route", "add", tmp_cmd, "dev", brname, "table", strtbl);
#ifdef CONFIG_IP6_NF_TARGET_NPT
				if (napt_v6 != 2)
				{
					//write prefix route to DS_WAN_NPT_PD_ROUTE 259, otherwise will hist br0's rule in 259
					char ds_wan_npt_pd_route_tbl[10] = {0};
					snprintf(ds_wan_npt_pd_route_tbl, sizeof(ds_wan_npt_pd_route_tbl), "%d", DS_WAN_NPT_PD_ROUTE);
					va_cmd_no_error(BIN_IP, 8, 1, "-6", "route", "del", tmp_cmd, "dev", brname, "table", ds_wan_npt_pd_route_tbl);
					va_cmd(BIN_IP, 8, 1, "-6", "route", "add", tmp_cmd, "dev", brname, "table", ds_wan_npt_pd_route_tbl);
				}
#endif

			}
		}
	}
}

int setup_radvd_conf_with_interface_group_br(void)
{
	unsigned char radvdEnable = 0;
	DNS_V6_INFO_T dnsV6Info = {0};
	PREFIX_V6_INFO_T prefixInfo = {0},  prefixInfo_br = {0};
	int ret = 0;
	int i = 0, total = 0;
	MIB_L2BRIDGE_GROUP_T entry = {0};
	MIB_L2BRIDGE_GROUP_Tp pEntry = &entry;
	char brname[16] = {0};
	MIB_CE_ATM_VC_T Entry;
	unsigned int wanconn=0, entry_index=0;

	mib_get_s(MIB_V6_RADVD_ENABLE, (void *)&radvdEnable, sizeof(radvdEnable));
	if (radvdEnable) {
		//Prepare DNSv6 and Prefixv6 info for default brX radvd conf.
		get_radvd_dnsv6_info(&dnsV6Info);
		get_radvd_prefixv6_info(&prefixInfo);
		total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
		for (i = 0; i < total; i++) {
			if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, (void *)pEntry))
				continue;
			if (pEntry->enable ==0 ) continue;
			if (pEntry->groupnum !=0 ) {
				memcpy(&prefixInfo_br, &prefixInfo, sizeof(PREFIX_V6_INFO_T));
				prefixInfo_br.prefixIP[7] += (pEntry->groupnum) ;
				if (prefixInfo_br.prefixLen<64)  prefixInfo_br.prefixLen=64;
				//Since prefix provided by radvd may be different from dhcpv6, we add brX global address here.
				snprintf(brname, sizeof(brname), "br%u", pEntry->groupnum);
				setup_radvd_conf_by_lan_ifname((char *)brname, &dnsV6Info, &prefixInfo_br);

				if (prefixInfo_br.mode == RADVD_MODE_MANUAL){
					//add radvd static prefix route for all wan policy route table
					set_static_policy_route(&prefixInfo_br,1, brname);
				}else{
					//add radvd PD route for all wan policy route table
					set_lanif_PD_policy_route_for_all_wan(brname);
				}

			}
		}
		wanconn = get_pd_wan_delegated_mode_ifindex();
		if(getWanEntrybyIfIndex(wanconn, &Entry, &entry_index) != 0)
		{
			printf("[%s:%d] can't find ATM_VC_TBL for MIB_PREFIXINFO_DELEGATED_WANCONN=%d\n", __FUNCTION__, __LINE__, wanconn);
			return 0;
		}

		set_ipv6_policy_route(&Entry, 2);
		rename(RADVD_CONF_TMP, RADVD_CONF);
		return 1;
	}
	return 0;
}

#endif
/* setup_radvd_conf_with_single_br: In single br0 structure, radvd also need to handle other
 *    interface config, so reconfig other interface and br0 config, then restart or reload radvd.
 */
int setup_radvd_conf_with_single_br(void)
{
	unsigned char radvdEnable = 0;
	DNS_V6_INFO_T dnsV6Info = {0};
	PREFIX_V6_INFO_T prefixInfo = {0};
	unsigned char oriULAPrefixEnable=0;
	char ulaPrefixIp_str[INET6_ADDRSTRLEN] = {0}, ulaPrefixLen_str[32]={0};
	int ret = 0;

	mib_get_s(MIB_V6_RADVD_ENABLE, (void *)&radvdEnable, sizeof(radvdEnable));
	if (radvdEnable) {
		unlink(RADVD_CONF_TMP);
		//first to write radvd conf to RADVD_CONF_TMP for bind lan interface.
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
		setup_binding_lan_radvd_conf();
#endif
		//Prepare DNSv6 and Prefixv6 info for default br0 radvd conf.
		get_radvd_dnsv6_info(&dnsV6Info);
		get_radvd_prefixv6_info(&prefixInfo);
		//Since prefix provided by radvd may be different from dhcpv6, we add br0 global address here.
		if(setup_radvd_conf_by_lan_ifname((char *)BRIF, &dnsV6Info, &prefixInfo) == -1)
				printf("setup_radvd_conf_by_lan_ifname failed: %s %d\n", __func__, __LINE__);
		// continue to write RADVD_CONF_TMP if multple brX exist.
		/* Use /bin/mv instead of /bin/cp to avoid double br0 in radvd.conf which occurs radvd crash 20220616.*/
		va_cmd("/bin/mv", 2, 1, (char*)RADVD_CONF_TMP, (char *)RADVD_CONF);

		mib_get_s( MIB_V6_ULAPREFIX_ENABLE, (void *)&oriULAPrefixEnable, sizeof(oriULAPrefixEnable));
		if(oriULAPrefixEnable){
			//read and delete all static ipv6 address
			memset(&prefixInfo, 0 ,sizeof(PREFIX_V6_INFO_T));
			mib_get_s(MIB_V6_ULAPREFIX, (void *)ulaPrefixIp_str, sizeof(ulaPrefixIp_str));
			mib_get_s(MIB_V6_ULAPREFIX_LEN, (void *)ulaPrefixLen_str, sizeof(ulaPrefixLen_str));
			if(ulaPrefixIp_str[0]){
				inet_pton(AF_INET6, ulaPrefixIp_str, prefixInfo.prefixIP);
				prefixInfo.prefixLen = atoi(ulaPrefixLen_str);
				set_lan_ipv6_global_address(NULL,&prefixInfo,(char*)LANIF,1,1);
				set_static_policy_route(&prefixInfo, 1, LANIF);
			}
		}

		return 1;
	}

	return 0;
}

/* Must not use kill_and_do_radvd directly to avoiding multiple performing radvd timing issue
 * Please use sysconf command to trigger systemd to start radvd all the time.
 */
int kill_and_do_radvd(void)
{
	char radvdEnable = 0;
	char mFlagInterleave = 0;
	kill_by_pidfile((const char *)RADVD_PID, SIGTERM);	//SIGTERM to clear old setting
	mib_get_s(MIB_V6_RADVD_ENABLE, (void *)&radvdEnable, sizeof(radvdEnable));
	mib_get_s(MIB_V6_MFLAGINTERLEAVE, (void *)&mFlagInterleave, sizeof(mFlagInterleave));
	if (radvdEnable) {
#ifdef CONFIG_RTK_RADVD_MFLAG_AUTO
		if(mFlagInterleave){
			va_cmd("/bin/radvd", 4, 1, "-C", (char *)RADVD_CONF, "-M", "1");
			//va_cmd("/bin/radvd", 9, 1, "-C", (char *)RADVD_CONF, "-n","-d","5","-m", "stderr", "-M", "1");  //debug mode
		}
		else{
			va_cmd("/bin/radvd", 2, 1, "-C", (char *)RADVD_CONF);
			//va_cmd("/bin/radvd", 7, 1, "-C", (char *)RADVD_CONF, "-n","-d","5","-m", "stderr");  //debug mode
		}
#else

#if defined(CONFIG_RTK_IPV6_LOGO_TEST)
		unsigned char logoTestMode = 0;
		if ( mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode)) ){
			/* Since we have already modified NCE state of kernel, we do not need to hint radvd that now is logotest mode. 20220602 */
#if 0
			/* CE router LAN-RFC4861 Test CERouter 2.4.7: Neighbor Solicitation Processing, NCE State REACHABLE
			 * Since Test bed will check NCE state. If radvd uses unicast RA (RASolicitedUnicast ON) the NCE state will
			 * mismatch with test script. After changing the RA to multicast, MAX_INITIAL_RTR_ADVERT_INTERVAL in radvd-2.17/defaults.h
			 * also needs to be set to reduce to 5 (default is 16). The reason is that when radvd uses multicast RA, it will refer
			 * MAX_INITIAL_RTR_ADVERT_INTERVAL to send RA as rate-limiting. Upon test bed sends RS, DUT always CANNOT send RA in time in
			 * radvd-2.17. Thus we use argument "-L" to hint radvd. 20220520
			 */
			if(logoTestMode == 3)
				va_cmd("/bin/radvd", 4, 1, "-C", (char *)RADVD_CONF, "-L", "3");
				//va_cmd("/bin/radvd", 8, 1, "-d", "5", "-m", "logfile", "-C", (char *)RADVD_CONF, "-L", "3"); //debug mode
			else
#endif
				va_cmd("/bin/radvd", 2, 1, "-C", (char *)RADVD_CONF);
		}
#else
		va_cmd("/bin/radvd", 2, 1, "-C", (char *)RADVD_CONF);
		//va_cmd("/bin/radvd", 7, 1, "-C", (char *)RADVD_CONF, "-n","-d","5","-m", "stderr");  //debug mode
#endif
#endif
	}

	return 1;
}

void  clean_radvd()
{
	kill_by_pidfile((const char *)RADVD_PID, SIGTERM);	//SIGTERM to clear old setting
//#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
//#ifdef CONFIG_NETFILTER_XT_TARGET_TEE
	//va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "multi_lan_radvd_ds_TEE");
	//va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "OUTPUT", "-j","multi_lan_radvd_ds_TEE");
	//va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", "multi_lan_radvd_ds_TEE");
//#endif
//#endif
}
void restartRadvd()
{
	unsigned char IPv6Enable=0,radvdEnable=0;
	int ret;
	unsigned char value[64]={0};
	
	clean_radvd();
	mib_get_s(MIB_V6_IPV6_ENABLE, (void *)&IPv6Enable, sizeof(IPv6Enable));
	if(IPv6Enable){
		mib_get_s(MIB_V6_RADVD_ENABLE, (void *)&radvdEnable, sizeof(radvdEnable));

		if(radvdEnable) {
			setup_radvd_conf_with_single_br();
#if (defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_) ) && defined(_SUPPORT_L2BRIDGING_DHCP_SERVER_)
			setup_radvd_conf_with_interface_group_br();
#endif
		}
		va_cmd("/bin/sysconf", 3, 1, "send_unix_sock_message", SYSTEMD_USOCK_SERVER, "do_ipv6_radvd");
	}
}
#endif

int rtk_ipv6_prefix_to_mask(unsigned int prefix, struct in6_addr *mask)
{
	struct in6_addr in6;
	int i, j;

	if (prefix > 128)
		return -1;

	memset(&in6, 0x0, sizeof(in6));
	for (i = prefix, j = 0; i > 0; i -= 8, j++) {
		if (i >= 8) {
			in6.s6_addr[j] = 0xff;
		} else {
			in6.s6_addr[j] = (unsigned long)(0xffU << (8 - i));
		}
	}

	memcpy(mask, &in6, sizeof(*mask));
	return 0;
}

int rtk_ipv6_ip_address_range(struct in6_addr*ip6, int plen,	struct in6_addr* in6_ip_start, struct in6_addr* in6_ip_end)
{
	struct in6_addr mask = {0};
	int i =0;
	char tmp[48]={0};

	inet_ntop(AF_INET6, ip6, tmp, INET6_ADDRSTRLEN);
	inet_pton(PF_INET6, tmp, in6_ip_start);
	inet_pton(PF_INET6, tmp, in6_ip_end);
	rtk_ipv6_prefix_to_mask(plen, &mask);
	for (i = 0; i < sizeof(struct in6_addr); i++){
		in6_ip_end->s6_addr[i] |= ~mask.s6_addr[i];
	}
	inet_ntop(AF_INET6, in6_ip_end, tmp, INET6_ADDRSTRLEN);
	return 0;
}

int rtk_ipv6_ip_address_range_to_mask(int align_mask, struct in6_addr* in6_ip_start, struct in6_addr* in6_ip_end, struct in6_addr* ip6_mask)
{
	int i =0;
	struct in6_addr in6;

	memset(&in6, 0x0, sizeof(in6));
	for (i = 0; i < IP6_ADDR_LEN; i++) {
		if (in6_ip_start->s6_addr[i] == in6_ip_end->s6_addr[i]) {
			in6.s6_addr[i] = 0xff;
		} else {
			if(align_mask)
			{
//				HW ACL not support RT_ACL_INGRESS_IPV6_DIP_BIT with mask not aligned
				in6.s6_addr[i]=~(int)(in6_ip_end->s6_addr[i] - in6_ip_start->s6_addr[i]);
			}
			break;
		}
	}

	memcpy(ip6_mask, &in6, sizeof(*ip6_mask));

	return 0;
}


#if defined(CONFIG_USER_RTK_RAMONITOR)
int dumpRAInfo(RAINFO_V6_Tp ra_v6Info)
{
	int i=0;
	
	printf("ra_info.ifname=%s\n",ra_v6Info->ifname);
	printf("ra_info.M=%d\n",ra_v6Info->m_bit);
	printf("ra_info.O=%d\n",ra_v6Info->o_bit);
	printf("ra_info.reomte_gateway=%s\n",ra_v6Info->reomte_gateway);
	printf("ra_info.prefix=%s\n",ra_v6Info->prefix_1);
	printf("ra_info.prefixlen=%d\n",ra_v6Info->prefix_len_1);	
	printf("ra_info.prefix_onlink=%d\n",ra_v6Info->prefix_onlink);
	printf("ra_info.mtu=%d\n",ra_v6Info->mtu);
	for (i=0; i<2 ;i++) {
		printf("ra_info.rdnss[%d]=%s\n",i, ra_v6Info->rdnss[i]);
	}
	for (i=0; i<2 ;i++) {
		printf("ra_info.dnssl[%d]=%s\n",i, ra_v6Info->dnssl[i]);
	}

	return 0;
}

/*
 * return value: If you want to add return value for parsing parameter you want, please return
 *               value for bitwise. We will use bitwise & to be decision.
 *
 * return 16(RAINFO_GET_)   :  for future use.
 * return 8(RAINFO_GET_MTU) :  find MTU.
 * return 4(RAINFO_GET_RDNSS):  find RDNSS(TODO).
 * return 2(RAINFO_GET_GW)   :  find Gateway IP (TODO).
 * return 1(RAINFO_GET_M_O)  :  find M O bit(TODO).
 * return 0(RAINFO_NONE)     :  find Nothing info we care about.
 *
 */
int get_ra_info(char *rainfo_file, RAINFO_V6_Tp ra_v6Info)
{
	FILE *fp;
	char tmp_buf[256] = {0};
	RAINFO_V6_T temp_rainfo;
	char *str = NULL;
	int ret = RAINFO_NONE;

	if ((fp = fopen(rainfo_file, "r")) == NULL)
	{
		//printf("%s Open file %s fail !\n", __FUNCTION__, rainfo_file);
		return 0;
	}

	memset(&temp_rainfo, 0, sizeof(RAINFO_V6_T));
	while (fgets(tmp_buf, sizeof(tmp_buf), fp))
	{
		if ((str = strstr(tmp_buf, RA_INFO_M_BIT_KEY)) != NULL)	{
			temp_rainfo.m_bit= atoi(str + strlen(RA_INFO_M_BIT_KEY));
			ret |= RAINFO_GET_M_O; 
		}else if ((str = strstr(tmp_buf, RA_INFO_O_BIT_KEY)) != NULL){
			temp_rainfo.o_bit= atoi(str + strlen(RA_INFO_O_BIT_KEY));
			ret |= RAINFO_GET_M_O; 
		}else if ((str = strstr(tmp_buf, RA_INFO_PREFIX_KEY)) != NULL){
			// find prefix
			snprintf(temp_rainfo.prefix_1, INET6_ADDRSTRLEN , "%s", str + strlen(RA_INFO_PREFIX_KEY));
			if (temp_rainfo.prefix_1[strlen(temp_rainfo.prefix_1)-1] == '\n')
				temp_rainfo.prefix_1[strlen(temp_rainfo.prefix_1)-1] = '\0';
			ret |= RAINFO_GET_PREFIX; 
		}else if ((str = strstr(tmp_buf, RA_INFO_PREFIX_LEN_KEY)) != NULL){
			// find prefix len
			temp_rainfo.prefix_len_1= atoi(str + strlen(RA_INFO_PREFIX_LEN_KEY));
			ret |= RAINFO_GET_PREFIX; 
		}else if ((str = strstr(tmp_buf, RA_INFO_PREFIX_ONLINK_KEY)) !=NULL) {
			// find prefix onlink flag
			temp_rainfo.prefix_onlink= atoi(str + strlen(RA_INFO_PREFIX_ONLINK_KEY));
			ret |= RAINFO_GET_PREFIX;
		}else if ((str = strstr(tmp_buf, RA_INFO_PREFIX_IFNAME_KEY)) !=NULL) {
			// find interface name
			snprintf(temp_rainfo.ifname,IFNAMSIZ, "%s", str + strlen(RA_INFO_PREFIX_IFNAME_KEY));
			if (temp_rainfo.ifname[strlen(temp_rainfo.ifname)-1] == '\n')
				temp_rainfo.ifname[strlen(temp_rainfo.ifname)-1] = '\0';
			ret |= RAINFO_GET_IFNAME; 
		}else if ((str = strstr(tmp_buf, RA_INFO_PREFIX_MTU_KEY)) !=NULL) {
			// find mtu
			temp_rainfo.mtu = atoi(str + strlen(RA_INFO_PREFIX_MTU_KEY));
			ret |= RAINFO_GET_MTU; //initial it.
		}else if ((str = strstr(tmp_buf, RA_INFO_PREFIX_GW_KEY)) !=NULL) {
			// find GW
			snprintf(temp_rainfo.reomte_gateway, sizeof(temp_rainfo.reomte_gateway), "%s", str + strlen(RA_INFO_PREFIX_GW_KEY));
			if (temp_rainfo.reomte_gateway[strlen(temp_rainfo.reomte_gateway) - 1] == '\n')
				temp_rainfo.reomte_gateway[strlen(temp_rainfo.reomte_gateway) - 1] = '\0';
			ret |= RAINFO_GET_GW;
		}
	}
	if (ret != RAINFO_NONE)
		memcpy(ra_v6Info, &temp_rainfo, sizeof(RAINFO_V6_T));

	fclose(fp);
	
	//dumpRAInfo(ra_v6Info);
	
	return ret;
}

int get_ra_mtu_by_wan_ifindex(int ifindex, int *mtu)
{
	char ifname[IFNAMSIZ] = {0};
	char rainfo_file[64] = {0};
	RAINFO_V6_T ra_v6Info;

	ifGetName(ifindex, ifname, sizeof(ifname));
	snprintf(rainfo_file, sizeof(rainfo_file), "%s_%s", (char *)RA_INFO_FILE, ifname);

	if (get_ra_info(rainfo_file, &ra_v6Info) & RAINFO_GET_MTU)
	{
		*mtu = ra_v6Info.mtu;
		return 1;
	}

	return 0;
}

#endif

#ifdef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
/*
 * Function updateIPV6FilterByPD
 *
 * Here will read MIB setting to setup IPv6 firewall rules.
 * 1. Flush all rules.
 * 2. Setup rules according to user setting with Delegated Prefix
 * 3. Setup default incoming/outgoing rules.
 *
 */
void updateIPV6FilterByPD(PREFIX_V6_INFO_Tp pDLGInfo)
{
	char *argv[20];
	unsigned char value[32], byte;
	unsigned char ivalue;
	int vInt, i, total;
	//MIB_CE_V6_IP_PORT_FILTER_Tp pIpEntry;
	MIB_CE_V6_IP_PORT_FILTER_T IpEntry;
	char *policy, *filterSIP, *filterDIP, srcPortRange[12], dstPortRange[12];
	char SIPRange[110]={0};
	char DIPRange[110]={0};
	unsigned char prefixIp6Addr[IP6_ADDR_LEN];
	unsigned char newPrefixStr[MAX_V6_IP_LEN]={0};
	PREFIX_V6_INFO_T DLGInfo, radvd_DLGInfo = {0};
	struct in6_addr ip6Addr_start, radvd_prefix;
	unsigned char prefixLen = 0;
	struct in6_addr static_ip6Addr_prefix, static_ip6Addr_radvd_prefix;
	unsigned char ipv6RadvdPrefixMode = 0, radvd_prefixLen[MAX_RADVD_CONF_PREFIX_LEN] = {0};
	unsigned char radvd_V6prefix[MAX_RADVD_CONF_PREFIX_LEN] = {0};
	int radvd_mib_ok = 1, radvd_prefix_len = 0;
	unsigned char v6_session_fw_enable = 1;


	if(!pDLGInfo)
	{
		printf("Error! Invalid parameter pDLGInfo in %s\n",__func__);
		return;
	}

	memcpy(&DLGInfo, pDLGInfo, sizeof(PREFIX_V6_INFO_T));

	printf("Update Firewall rule set by user.\n");
	// Delete ipfilter rule
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)FW_IPV6FILTER);

	// packet filtering
	// ip filtering
	total = mib_chain_total(MIB_V6_IP_PORT_FILTER_TBL);

#ifdef CONFIG_USER_RADVD
	mib_get_s(MIB_V6_RADVD_PREFIX_MODE, (void *)&ipv6RadvdPrefixMode, sizeof(ipv6RadvdPrefixMode));

	if (ipv6RadvdPrefixMode == RADVD_MODE_MANUAL)
	{
		if (!mib_get_s(MIB_V6_PREFIX_IP, (void *)radvd_V6prefix, sizeof(radvd_V6prefix))) { //STRING_T
			printf("Error!! Get MIB_V6_PREFIX_IP fail!");
			radvd_mib_ok = 0;
		}

		if (!mib_get_s(MIB_V6_PREFIX_LEN, (void *)radvd_prefixLen, sizeof(radvd_prefixLen))) { ////STRING_T
			printf("Error!! Get MIB_V6_PREFIX_LEN fail!");
			radvd_mib_ok = 0;
		}
		if (radvd_mib_ok)
		{
			if (inet_pton(PF_INET6, radvd_V6prefix, &radvd_prefix))
			{
				radvd_prefix_len = atoi(radvd_prefixLen);
				if (ip6toPrefix(&radvd_prefix, radvd_prefix_len, &static_ip6Addr_radvd_prefix))
				{
					memcpy(&radvd_DLGInfo.prefixIP, &static_ip6Addr_radvd_prefix, sizeof(radvd_DLGInfo.prefixIP));
				}
			}
		}

	}
	else
		get_radvd_prefixv6_info(&radvd_DLGInfo);
#endif

	// ip6tables -A ipv6filter -d ff00::/8 -j RETURN
	//va_cmd(IP6TABLES, 6, 1, (char *)FW_ADD, (char *)FW_IPV6FILTER, "-d",
	//	"ff00::/8", "-j", (char *)FW_RETURN);

	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_V6_IP_PORT_FILTER_TBL, i, (void *)&IpEntry))
		{
			printf("mib chain MIB_V6_IP_PORT_FILTER_TB get fail!\n");
			continue;
		}

		setEachIPv6FilterRuleMixed(&IpEntry, &DLGInfo);
#ifdef CONFIG_USER_RADVD
		if (memcmp(&DLGInfo.prefixIP, &radvd_DLGInfo.prefixIP,IP6_ADDR_LEN))
		{
			/* Set radvd prefix v6 filter if prefix is not the same with DHCPv6s prefix */
			setEachIPv6FilterRuleMixed(&IpEntry, &radvd_DLGInfo);
		}
#endif
	}

	// Kill all conntrack (to kill the established conntrack when change ip6tables rules)
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	system("/bin/echo flush > /proc/fc/ctrl/rstconntrack");
#else
#ifdef CONFIG_USER_CONNTRACK_TOOLS
	va_cmd(CONNTRACK_TOOL, 3, 1, "-D", "-f", "ipv6");	// v1.4.6 support family "ipv6"
#else
	va_cmd("/bin/ethctl", 2, 0, "conntrack", "killall");
#endif
#endif

	// accept related
	// ip6tables -A ipv6filter -m state --state ESTABLISHED,RELATED -j RETURN
#if defined(CONFIG_00R0)
	char ipfilter_spi_state=1;
	mib_get_s(MIB_IPFILTER_SPI_ENABLE, (void*)&ipfilter_spi_state, sizeof(ipfilter_spi_state));
	if(ipfilter_spi_state != 0)
#endif
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_IPV6FILTER, "-m", "state",
			"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_RETURN);

	//----------------------------------------------
	//   Now setup incoming/outgoing default rule
	//----------------------------------------------
	printf("Setup default Firewall rule.\n");
	if (mib_get_s(MIB_V6_IPF_OUT_ACTION, (void *)&ivalue, sizeof(ivalue)))
	{
		if (ivalue == 0)	// DROP
		{
			// ip6tables -A ipv6filter -i $LAN_IF -j DROP
			va_cmd(IP6TABLES, 6, 1, (char *)FW_ADD,
					(char *)FW_IPV6FILTER, (char *)ARG_I,
					(char *)LANIF, "-j", (char *)FW_DROP);
		}
	}

	mib_get_s(MIB_V6_SESSION_FW_ENABLE, (void *)&v6_session_fw_enable, sizeof(v6_session_fw_enable));
	if (mib_get_s(MIB_V6_IPF_IN_ACTION, (void *)&ivalue, sizeof(ivalue)))
	{
		if (ivalue == 0 && v6_session_fw_enable)	// DROP
		{
			// ip6tables -A ipv6filter ! -i $LANIF_ALL -j DROP
			// Because SFU mode (PPTP WAN), we will generate br1, brX ... so we should use br+ to avoid kernel DROP
			// all IPv6 packet for ip6tables FORWARD. In bridge wan, packet still will pass ip6tables FORWARD chain. 20201104
			va_cmd(IP6TABLES, 7, 1, (char *)FW_ADD, (char *)FW_IPV6FILTER, "!", (char *)ARG_I, (char *)LANIF_ALL, "-j", (char *)FW_DROP);
		}
	}
}

#endif
void get_ipv6_default_gw_inf(char *wan_inf_buf)
{
	FILE *fp = NULL;
	char tmp_cmd[128] = {0};
	char buf[IFNAMSIZ] = {0};

	snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 route | grep default | awk -F \"dev \" '{print $2}' | awk -F \" \" '{print $1}'");
	if ((fp = popen(tmp_cmd, "r")) != NULL)
	{
		while(fgets(buf, sizeof(buf), fp))
		{
			buf[strlen(buf) - 1] = '\0';
			snprintf(wan_inf_buf, IFNAMSIZ, "%s", buf);
		}

		pclose(fp);
	}
}

void get_ipv6_default_gw_inf_first(char *wan_inf_buf)
{
	FILE *fp = NULL;
	char tmp_cmd[128] = {0};
	char buf[IFNAMSIZ] = {0};

	snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 route | grep default | awk -F \"dev \" '{print $2}' | awk -F \" \" '{print $1}'");
	if ((fp = popen(tmp_cmd, "r")) != NULL)
	{
		while(fgets(buf, sizeof(buf), fp))
		{
			buf[strlen(buf) - 1] = '\0';
			snprintf(wan_inf_buf, IFNAMSIZ, "%s", buf);
			break;
		}

		pclose(fp);
	}
}

void get_ipv6_default_gw(char *dgw)
{
	FILE *fp = NULL;
	char tmp_cmd[128] = {0};
	char buf[INET6_ADDRSTRLEN] = {0};

	snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 route | grep default | awk -F \" \" '{print $3}'");
	if ((fp = popen(tmp_cmd, "r")) != NULL)
	{
		while(fgets(buf, sizeof(buf), fp))
		{
			buf[strlen(buf) - 1] = '\0';
			snprintf(dgw, INET6_ADDRSTRLEN, "%s", buf);
			break;
		}

		pclose(fp);
	}
}

void restartStaticPDRoute()
{
	unsigned char prefixStr[INET6_ADDRSTRLEN]={0};
	unsigned char len = 0;
	char lanIPv6PrefixMode;
	char static_prefix_str[INET6_ADDRSTRLEN] = {0};
	
	if(!mib_get(MIB_PREFIXINFO_PREFIX_MODE, (void *)&lanIPv6PrefixMode))
		return;
	if(lanIPv6PrefixMode == IPV6_PREFIX_STATIC)
	{
		if(!mib_get_s(MIB_IPV6_LAN_PREFIX, (void *)prefixStr, sizeof(prefixStr)))
			return;
		if(!mib_get_s(MIB_IPV6_LAN_PREFIX_LEN, (void *)&len, sizeof(len)))
			return;
	
		snprintf(static_prefix_str, sizeof(static_prefix_str), "%s/%d", prefixStr, len);
		add_static_pd_route_for_default_waninf(static_prefix_str, (char *)BRIF);
	}
}

/* prefix_str should be xxxx::/XX format, like 2003::/64 */
int add_static_pd_route_for_default_waninf(char *prefix_str, char *lan_infname)
{
	char default_gw_wan[IFNAMSIZ] = {0};
	int tbl_id = 0, wan_ifIndex = 0, ret = 0;
	MIB_CE_ATM_VC_T Entry;
	char tmp_cmd[256] = {0};

	if (!prefix_str || !lan_infname)
	{
		printf("%s: Error input! !\n", __FUNCTION__);
		return -1;
	}

	get_ipv6_default_gw_inf(default_gw_wan);

	if (default_gw_wan[0]  != '\0')
	{
		printf("%s: default_gw_wan = %s\n", __FUNCTION__, default_gw_wan);

		if (!(getATMVCEntryByIfName(default_gw_wan, &Entry)))
		{
			printf("Error! Could not get MIB entry info from wan %s!\n", default_gw_wan);
			return -1;
		}
		tbl_id = caculate_tblid(Entry.ifIndex);

		snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 route del %s dev %s table %d > /dev/null 2>&1", prefix_str, lan_infname, tbl_id);
		system(tmp_cmd);

		snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 route add %s dev %s table %d", prefix_str, lan_infname, tbl_id);
		system(tmp_cmd);

		return 1;
	}

	return -1;
}

void del_static_pd_route(char *prefix_str, char *lan_infname)
{
	char default_gw_wan[IFNAMSIZ] = {0};
	int tbl_id = 0, wan_ifIndex = 0, ret = 0;
	MIB_CE_ATM_VC_T Entry;
	char tmp_cmd[256] = {0};

	if (!prefix_str || !lan_infname)
	{
		printf("%s: Error input! !\n", __FUNCTION__);
		return;
	}

	get_ipv6_default_gw_inf(default_gw_wan);

	if (default_gw_wan[0]  != '\0')
	{
		printf("%s: default_gw_wan = %s prefix_str=%s\n", __FUNCTION__, default_gw_wan, prefix_str);

		if (!(getATMVCEntryByIfName(default_gw_wan, &Entry)))
		{
			printf("Error! Could not get MIB entry info from wan %s!\n", default_gw_wan);
			return;
		}
		tbl_id = caculate_tblid(Entry.ifIndex);

		snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 route del %s dev %s table %d > /dev/null 2>&1", prefix_str, lan_infname, tbl_id);
		system(tmp_cmd);
	}
}
#ifdef CONFIG_IP6_NF_TARGET_NPT
#define NPT_POSTROUTING "NPT_POSTROUTING_TBL"
#define NPT_PREROUTING "NPT_PREROUTING_TBL"

static int set_v6npt_rule(PREFIX_V6_INFO_T *pLanInfo, PREFIX_V6_INFO_T *pWanInfo)
{
	char wanPrefix[INET6_ADDRSTRLEN] = {0}, lanPrefix[INET6_ADDRSTRLEN] = {0};
	char ifname[IFNAMSIZ] = {0}, tmpBuf[256] = {0};
	int wanPrefixLen, lanPrefixLen;

	if (!(pLanInfo->prefixLen && compare_prefixip_with_unspecip_by_bit(pLanInfo->prefixIP, ipv6_unspec, pLanInfo->prefixLen)
		&& pWanInfo->prefixLen && compare_prefixip_with_unspecip_by_bit(pWanInfo->prefixIP, ipv6_unspec, pWanInfo->prefixLen)))
	{
		AUG_PRT(" Parameter err!\n");
		return -1;
	}

	ifGetName(pWanInfo->wanconn,ifname,sizeof(ifname));
	inet_ntop(AF_INET6, &pLanInfo->prefixIP, lanPrefix, INET6_ADDRSTRLEN);
	lanPrefixLen = pLanInfo->prefixLen;
	inet_ntop(AF_INET6, &pWanInfo->prefixIP, wanPrefix, INET6_ADDRSTRLEN);
	wanPrefixLen = pWanInfo->prefixLen;

	//AUG_PRT("LAN: %s/%d\n",lanPrefix, lan_prefixInfo.prefixLen);
	//AUG_PRT("WAN: %s/%d\n",wanPrefix, wan_prefixInfo.prefixLen);

	//Aovid duplicate rule
	sprintf(tmpBuf, "/bin/ip6tables -t mangle -D %s  -i %s -d %s/%d -j DNPT --src-pfx %s/%d --dst-pfx %s/%d",
				NPT_PREROUTING, ifname, wanPrefix, wanPrefixLen,
				wanPrefix, wanPrefixLen, lanPrefix, lanPrefixLen);
	va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);
	sprintf(tmpBuf, "/bin/ip6tables -t mangle -A %s  -i %s -d %s/%d -j DNPT --src-pfx %s/%d --dst-pfx %s/%d",
				NPT_PREROUTING, ifname, wanPrefix, wanPrefixLen,
				wanPrefix, wanPrefixLen, lanPrefix, lanPrefixLen);
	va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);

	//Aovid duplicate rule
	sprintf(tmpBuf, "/bin/ip6tables -t mangle -D %s  -o %s -s %s/%d -j SNPT --src-pfx %s/%d --dst-pfx %s/%d",
				NPT_POSTROUTING, ifname, lanPrefix, lanPrefixLen,
				lanPrefix, lanPrefixLen, wanPrefix, wanPrefixLen);
	va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);
	sprintf(tmpBuf, "/bin/ip6tables -t mangle -A %s  -o %s -s %s/%d -j SNPT --src-pfx %s/%d --dst-pfx %s/%d",
				NPT_POSTROUTING, ifname, lanPrefix,lanPrefixLen,
				lanPrefix, lanPrefixLen, wanPrefix, wanPrefixLen);
	va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);

	return 0;
}
/*
 * FUCTION: set_v6npt_config
 * return -1: Fail
 *         0: Sucess
 */
int set_v6npt_config(char *infname)
{
	int ret = 0;
	int idx = 0, total = mib_chain_total(MIB_ATM_VC_TBL);
	MIB_CE_ATM_VC_T Entry;
	MIB_CE_ATM_VC_Tp pEntry = NULL;
	char ifname[IFNAMSIZ] = {0}, tmpBuf[256] = {0};
	int found_proxy = 0;
	unsigned int  flags = 0;
	unsigned char leasefile[64]= {0};
	DLG_INFO_T dlgInfo = {0};
	struct ipv6_ifaddr ip6_addr = {0};
	PREFIX_V6_INFO_T lan_prefixInfo, wan_prefixInfo, ula_prefixInfo;
	unsigned char dhcpv6_mode=0, dhcpv6_type=0, radvd_mode=0, ula_enable=0;
	char ulaPrefix[MAX_RADVD_CONF_PREFIX_LEN];

	if (!mib_get_s(MIB_DHCPV6_MODE, (void *)&dhcpv6_mode, sizeof(dhcpv6_mode)))
	{
		AUG_PRT("Get DHCPV6 server Mode failed\n");
	}
	if (!mib_get_s(MIB_DHCPV6S_TYPE, (void *)&dhcpv6_type, sizeof(dhcpv6_type)))
	{
		AUG_PRT("Get DHCPV6 server Type failed\n");
	}
	if ( !mib_get_s(MIB_V6_RADVD_PREFIX_MODE, (void *)&radvd_mode, sizeof(radvd_mode)))
	{
		AUG_PRT("Error!! Get Radvd IPv6 Prefix Mode fail!");
	}
	if (mib_get_s(MIB_V6_ULAPREFIX_ENABLE, (void *)&ula_enable, sizeof(ula_enable)) && ula_enable)
	{
		memset(&ula_prefixInfo,0,sizeof(ula_prefixInfo));
		if(mib_get_s(MIB_V6_ULAPREFIX, (void *)tmpBuf, sizeof(tmpBuf))) {
			if(inet_pton(AF_INET6, tmpBuf, ula_prefixInfo.prefixIP) == 1)
				if(mib_get_s(MIB_V6_ULAPREFIX_LEN, (void *)tmpBuf, sizeof(tmpBuf))) 
					ula_prefixInfo.prefixLen = atoi(tmpBuf);
		}
		if(!(ula_prefixInfo.prefixLen >= 64 && ula_prefixInfo.prefixLen <= 64))
			ula_enable = 0;
	}

	sprintf(tmpBuf, "/bin/ip6tables -t mangle -D PREROUTING -j %s", NPT_PREROUTING);
	va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);
	sprintf(tmpBuf, "/bin/ip6tables -t mangle -F %s", NPT_PREROUTING);
	va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);
	sprintf(tmpBuf, "/bin/ip6tables -t mangle -X %s", NPT_PREROUTING);
	va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);

	sprintf(tmpBuf, "/bin/ip6tables -t mangle -D POSTROUTING -j %s", NPT_POSTROUTING);
	va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);
	sprintf(tmpBuf, "/bin/ip6tables -t mangle -F %s", NPT_POSTROUTING);
	va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);
	sprintf(tmpBuf, "/bin/ip6tables -t mangle -X %s", NPT_POSTROUTING);
	va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);

	sprintf(tmpBuf, "/bin/ip6tables -t mangle -N %s", NPT_PREROUTING);
	va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);
	sprintf(tmpBuf, "/bin/ip6tables -t mangle -N %s", NPT_POSTROUTING);
	va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);

	for(idx = 0 ; idx < total ; idx++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, idx, (void *)&Entry))
			continue;
		pEntry = &Entry;

		if (!pEntry->enable)
			continue;

		ifGetName(pEntry->ifIndex,ifname,sizeof(ifname));

		if(getInFlags(ifname, &flags) == 0)
			continue;

		if(!((flags&IFF_UP) && (flags&IFF_RUNNING)))
			continue;

		if((pEntry->IpProtocol & IPVER_IPV6) && pEntry->napt_v6 == 2)
		{
			memset(&wan_prefixInfo, 0, sizeof(wan_prefixInfo));
			wan_prefixInfo.wanconn = pEntry->ifIndex;
			//TBD:Now we just support 64 bit for npt6 mode
			wan_prefixInfo.prefixLen = 64;

			if(pEntry->Ipv6DhcpRequest & 0x2)
			{
				snprintf(leasefile, sizeof(leasefile)-1, "%s.%s", PATH_DHCLIENT6_LEASE, ifname);
				if(getLeasesInfo(leasefile,&dlgInfo)&LEASE_GET_IAPD)
				{
					ip6toPrefix(&dlgInfo.prefixIP,wan_prefixInfo.prefixLen,&wan_prefixInfo.prefixIP);
				}
			}
#if !defined(CONFIG_RTK_DEV_AP)
			else if(get_prefixv6_info_by_wan(&wan_prefixInfo,pEntry->ifIndex)==0){
				//do nothing
			}
#else
			else if(getifip6(ifname, IPV6_ADDR_UNICAST, &ip6_addr, 1) > 0)
			{
				ip6toPrefix(&ip6_addr.addr, wan_prefixInfo.prefixLen, &wan_prefixInfo.prefixIP);
			}
#endif
			if(!(wan_prefixInfo.prefixLen>0 && memcmp(wan_prefixInfo.prefixIP, ipv6_unspec, wan_prefixInfo.prefixLen<=128?wan_prefixInfo.prefixLen:128)))
				continue;

			found_proxy++;

			memset(&lan_prefixInfo,0,sizeof(lan_prefixInfo));
			//get default prefix from br0
			if(get_prefixv6_info(&lan_prefixInfo) ==0)
			{
				set_v6npt_rule(&lan_prefixInfo, &wan_prefixInfo);
			}
			else if ((dhcpv6_mode == DHCP_LAN_SERVER) && (dhcpv6_type == DHCPV6S_TYPE_STATIC)
				&& (get_dhcpv6_prefixv6_info(&lan_prefixInfo) ==0))
			{
				set_v6npt_rule(&lan_prefixInfo, &wan_prefixInfo);
			}
			else if ((radvd_mode == RADVD_MODE_MANUAL)&&(get_radvd_prefixv6_info(&lan_prefixInfo)==0))
			{
				set_v6npt_rule(&lan_prefixInfo, &wan_prefixInfo);
			}
			else if(ula_enable)
			{
				set_v6npt_rule(&ula_prefixInfo, &wan_prefixInfo);
			}
		}
	}

out:
	if(found_proxy){
		sprintf(tmpBuf, "/bin/ip6tables -t mangle -A PREROUTING -j %s", NPT_PREROUTING);
		va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);
		sprintf(tmpBuf, "/bin/ip6tables -t mangle -A POSTROUTING -j %s", NPT_POSTROUTING);
		va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);
	}
	else{
		sprintf(tmpBuf, "/bin/ip6tables -t mangle -F %s", NPT_PREROUTING);
		va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);
		sprintf(tmpBuf, "/bin/ip6tables -t mangle -X %s", NPT_PREROUTING);
		va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);
		sprintf(tmpBuf, "/bin/ip6tables -t mangle -F %s", NPT_POSTROUTING);
		va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);
		sprintf(tmpBuf, "/bin/ip6tables -t mangle -X %s", NPT_POSTROUTING);
		va_cmd("/bin/sh", 2, 1, "-c", tmpBuf);
	}


	return 0;
}
#endif

/*
 * Clear LAN-side delegated-wan and set to dummy if this wan is delegated-wan.
 */
void clear_delegated_default_wanconn(MIB_CE_ATM_VC_Tp mibentry_p)
{
	//If use this WAN as default conn in prefix delegated, clear this value.
	unsigned char lanIPv6PrefixMode = IPV6_PREFIX_NONE, lanIPv6DnsMode = IPv6_DNS_NONE;
	unsigned char update_config=0;
	unsigned int old_wan_conn = DUMMY_IFINDEX, new_wan_conn = DUMMY_IFINDEX;
	unsigned char pd_wan_delegated_mode = PD_DELEGATED_NONE;
	unsigned char lan_IPv6_dns_wan_conn_mode = IPv6_DNS_DELEGATED_NONE;
	unsigned char radvd_rdnss_mode = RADVD_MODE_NONE;
	unsigned char lan_rdnns_wan_conn_mode = IPv6_RDNSS_DELEGATED_NONE;

	if (((mibentry_p->cmode!=CHANNEL_MODE_BRIDGE)
		&& ((mibentry_p->IpProtocol&IPVER_IPV6)
#ifdef  CONFIG_IPV6_SIT_6RD
		|| (mibentry_p->IpProtocol == IPVER_IPV4 && mibentry_p->v6TunnelType == V6TUNNEL_6RD)
#endif
#ifdef CONFIG_IPV6_SIT
		|| (mibentry_p->IpProtocol == IPVER_IPV4 && ((mibentry_p->v6TunnelType == V6TUNNEL_6TO4) || (mibentry_p->v6TunnelType == V6TUNNEL_6IN4)))
#endif
	  ))
		){
		//clear PD wanconn
		if ((mibentry_p->Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)
			|| (mibentry_p->AddrMode == IPV6_WAN_STATIC) // WAN static prefixv6 case
#ifdef  CONFIG_IPV6_SIT_6RD
			|| (mibentry_p->v6TunnelType == V6TUNNEL_6RD)
#endif
#ifdef CONFIG_IPV6_SIT
			|| (mibentry_p->v6TunnelType == V6TUNNEL_6TO4)
#endif
#ifdef CONFIG_USER_RTK_RA_DELEGATION
			|| ((mibentry_p->AddrMode & IPV6_WAN_AUTO) &&(!(mibentry_p->Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)))
#endif
		){
			if (!mib_get_s(MIB_PREFIXINFO_PREFIX_MODE, (void *)&lanIPv6PrefixMode, sizeof(lanIPv6PrefixMode)))
				printf("Error! %s Fail to get MIB_PREFIXINFO_PREFIX_MODE!\n", __FUNCTION__);
			if (lanIPv6PrefixMode == IPV6_PREFIX_DELEGATION) {


				if (mib_get_s(MIB_PD_WAN_DELEGATED_MODE, (void *)&pd_wan_delegated_mode, sizeof(pd_wan_delegated_mode)))
				{
					if (pd_wan_delegated_mode == PD_DELEGATED_STATIC) {
						if (mib_get_s(MIB_PREFIXINFO_DELEGATED_WANCONN, (void *)&old_wan_conn, sizeof(old_wan_conn)))
						{
							/* Delete static wan delegated pd, we should set wan_delegated_mode to be AUTO
							   and let MIB_PREFIXINFO_DELEGATED_WANCONN to be DUMMY_IFINDEX.
							 */
							if (old_wan_conn == mibentry_p->ifIndex) {
								pd_wan_delegated_mode = PD_DELEGATED_AUTO;
								new_wan_conn == DUMMY_IFINDEX;
								if ((!mib_set(MIB_PD_WAN_DELEGATED_MODE, (void *)&pd_wan_delegated_mode))
									|| (!mib_set(MIB_PREFIXINFO_DELEGATED_WANCONN, (void *)&new_wan_conn)))
									printf("Error! %s Fail to set MIB_PREFIXINFO_DELEGATED_WANCONN!\n", __FUNCTION__);
								update_config++;
							}

						}
						else
							printf("Error! %s Fail to get MIB_PREFIXINFO_DELEGATED_WANCONN!\n", __FUNCTION__);
					}
					else {
						new_wan_conn = get_pd_wan_delegated_mode_ifindex();
						if ((new_wan_conn == mibentry_p->ifIndex) || (new_wan_conn == 0xFFFFFFFF)) {
							new_wan_conn = DUMMY_IFINDEX;
							if (!mib_set(MIB_PREFIXINFO_DELEGATED_WANCONN, (void *)&new_wan_conn))
								printf("Error! Fail to set MIB_PREFIXINFO_DELEGATED_WANCONN!\n");
							else update_config++;
						}
						else if (pd_wan_delegated_mode != PD_DELEGATED_AUTO) {
						//PD_DELEGATED_AUTO doesn't use MIB_PREFIXINFO_DELEGATED_WANCONN
							if (!mib_set(MIB_PREFIXINFO_DELEGATED_WANCONN, (void *)&new_wan_conn))
								printf("Error! Fail to set MIB_PREFIXINFO_DELEGATED_WANCONN!\n");
							else update_config++;
						}
					}

				}
				else {
					printf("Error! get Fail to get MIB_PD_WAN_DELEGATED_MODE!\n", __FUNCTION__);
				}
			}
		}

		//clear DHCPv6DNS wanconn
		{
			if (!mib_get_s(MIB_LAN_DNSV6_MODE, (void *)&lanIPv6DnsMode, sizeof(lanIPv6DnsMode)))
				printf("Error! Fail to et MIB_LAN_DNSV6_MODE!\n");
			if (lanIPv6DnsMode == IPV6_DNS_WANCONN) {
				if (mib_get_s(MIB_DNSV6INFO_WANCONN_MODE, (void *)&lan_IPv6_dns_wan_conn_mode, sizeof(lan_IPv6_dns_wan_conn_mode))) {
					if (lan_IPv6_dns_wan_conn_mode == IPv6_DNS_DELEGATED_STATIC) {
						if (mib_get_s(MIB_DNSINFO_WANCONN, (void *)&old_wan_conn, sizeof(old_wan_conn)))
						{
							/* Delete static wan delegated pd, we should set wan_delegated_mode to be AUTO
							   and let MIB_DNSINFO_WANCONN to be DUMMY_IFINDEX.
							 */
							if (old_wan_conn == mibentry_p->ifIndex) {
								lan_IPv6_dns_wan_conn_mode = IPv6_DNS_DELEGATED_AUTO;
								new_wan_conn == DUMMY_IFINDEX;
								if ((!mib_set(MIB_DNSV6INFO_WANCONN_MODE, (void *)&lan_IPv6_dns_wan_conn_mode))
									|| (!mib_set(MIB_DNSINFO_WANCONN, (void *)&new_wan_conn)))
									printf("Error! %s Fail to set MIB_DNSINFO_WANCONN!\n", __FUNCTION__);
								update_config++;
							}

						}
						else
							printf("Error! %s Fail to get MIB_DNSINFO_WANCONN!\n", __FUNCTION__);
					}
					else {
						new_wan_conn = get_dnsv6_wan_delegated_mode_ifindex();
						if ((new_wan_conn == mibentry_p->ifIndex) || (new_wan_conn == 0xFFFFFFFF)) {
							new_wan_conn = DUMMY_IFINDEX;
							if (!mib_set(MIB_DNSINFO_WANCONN, (void *)&new_wan_conn))
								printf("Error! Fail to set MIB_DNSINFO_WANCONN!\n");
							else update_config++;
						}
						else if (lan_IPv6_dns_wan_conn_mode != IPv6_DNS_DELEGATED_AUTO) {
						//IPv6_DNS_DELEGATED_AUTO doesn't use MIB_DNSINFO_WANCONN
							if (!mib_set(MIB_DNSINFO_WANCONN, (void *)&new_wan_conn))
								printf("Error! Fail to set MIB_DNSINFO_WANCONN!\n");
							else update_config++;
						}
					}

				}
				else {
					printf("Error! get Fail to get MIB_DNSV6INFO_WANCONN_MODE!\n", __FUNCTION__);
				}
			}
		}
		//clear RA RDNSS wanconn
		{
			if (mib_get_s(MIB_V6_RADVD_RDNSS_MODE, (void *)&radvd_rdnss_mode, sizeof(radvd_rdnss_mode)))
				printf("Error!! get LAN IPv6 RDNSS Mode fail!");

			if (radvd_rdnss_mode == RADVD_RDNSS_WANCON) {
				if (mib_get_s(MIB_V6_RADVD_RDNSSINFO_WANCONN_MODE, (void *)&lan_rdnns_wan_conn_mode, sizeof(lan_rdnns_wan_conn_mode))) {
					if (lan_rdnns_wan_conn_mode == IPv6_RDNSS_DELEGATED_STATIC) {
						if (mib_get_s(MIB_V6_RADVD_DNSINFO_WANCONN, (void *)&old_wan_conn, sizeof(old_wan_conn))) {
							/* Delete static wan delegated rdnss, we should set rdnss_wan_delegated_mode to be AUTO
							   and let MIB_V6_RADVD_DNSINFO_WANCONN to be DUMMY_IFINDEX.
							 */
							if (old_wan_conn == mibentry_p->ifIndex) {
								lan_rdnns_wan_conn_mode = IPv6_RDNSS_DELEGATED_AUTO;
								new_wan_conn == DUMMY_IFINDEX;
								if ((!mib_set(MIB_V6_RADVD_RDNSSINFO_WANCONN_MODE, (void *)&lan_rdnns_wan_conn_mode))
									|| (!mib_set(MIB_V6_RADVD_DNSINFO_WANCONN, (void *)&new_wan_conn)))
									printf("Error! %s Fail to set MIB_V6_RADVD_DNSINFO_WANCONN!\n", __FUNCTION__);
								update_config++;
							}

						}
						else
							printf("Error! %s Fail to get MIB_V6_RADVD_DNSINFO_WANCONN!\n", __FUNCTION__);
					}
					else {
						new_wan_conn = get_radvd_rdnss_wan_delegated_mode_ifindex();
						if ((new_wan_conn == mibentry_p->ifIndex) || (new_wan_conn == 0xFFFFFFFF)) {
							new_wan_conn = DUMMY_IFINDEX;
							if (!mib_set(MIB_V6_RADVD_DNSINFO_WANCONN, (void *)&new_wan_conn))
								printf("Error! Fail to set MIB_V6_RADVD_DNSINFO_WANCONN!\n");
							else update_config++;
						}
						else if (lan_rdnns_wan_conn_mode != IPv6_RDNSS_DELEGATED_AUTO) {
							//IPv6_RDNSS_DELEGATED_AUTO doesn't use MIB_V6_RADVD_DNSINFO_WANCONN
							if (!mib_set(MIB_V6_RADVD_DNSINFO_WANCONN, (void *)&new_wan_conn))
								printf("Error! Fail to set MIB_V6_RADVD_DNSINFO_WANCONN!\n");
							else update_config++;
						}
					}

				}
				else {
					printf("Error! get Fail to get MIB_V6_RADVD_RDNSSINFO_WANCONN_MODE!\n", __FUNCTION__);
				}
			}
		}
	}
#ifdef CONFIG_USER_CONF_ON_XMLFILE
	//Bug fix: WAN from IPoE+DHCPv6+pd change to PPPoE+DHCPv6+pd, the LAN page WANDelegated interface empty 
	//RootCause:spppd ETH_PPPOE_DISCOVERY -> CMD_MIB_SAVE_PPPOE will reload xml file before Formeth function Commit() 
	//So, update MIB_PREFIXINFO_DELEGATED_WANCONN and MIB_DNSINFO_WANCONN to /var/config/config.xml immediately
	if (update_config && mib_update_ex(CURRENT_SETTING, CONFIG_MIB_ALL, 1) == 0)
		printf("CS Flash error! \n");
#endif
}

/*
 * Set this wan as LAN-side delegated-wan if applicable.
 */
void setup_delegated_default_wanconn(MIB_CE_ATM_VC_Tp mibentry_p)
{
	//In Spec , if prefix mode WANDelegated, and the default WANN conn
	//			is the one have INTERNET connection type
#ifdef CONFIG_TELMEX_DEV
	unsigned char ipv6RadvdPrefixMode=0, pdAutoGetFlag=0;
#endif
	unsigned char lanIPv6PrefixMode, lanIPv6DnsMode = IPv6_DNS_NONE;
	unsigned char update_config=0;
	unsigned int old_wan_conn = DUMMY_IFINDEX, auto_wan_conn = DUMMY_IFINDEX;
	unsigned char pd_wan_delegated_mode = PD_DELEGATED_NONE;
	unsigned char lan_IPv6_dns_wan_conn_mode = IPv6_DNS_DELEGATED_NONE;
	unsigned char radvd_rdnss_mode = RADVD_MODE_NONE;
	unsigned char lan_rdnns_wan_conn_mode = IPv6_RDNSS_DELEGATED_NONE;

#ifdef CONFIG_TELMEX_DEV
	if (mibentry_p->Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)
		pdAutoGetFlag = 1;
#endif

	if(((mibentry_p->enable) && (mibentry_p->cmode!=CHANNEL_MODE_BRIDGE)
		&& ((mibentry_p->IpProtocol&IPVER_IPV6)
#ifdef	CONFIG_IPV6_SIT_6RD
		|| (mibentry_p->IpProtocol == IPVER_IPV4 && mibentry_p->v6TunnelType == V6TUNNEL_6RD)
#endif
#ifdef CONFIG_IPV6_SIT
		|| (mibentry_p->IpProtocol == IPVER_IPV4 && ((mibentry_p->v6TunnelType == V6TUNNEL_6TO4) || (mibentry_p->v6TunnelType == V6TUNNEL_6IN4)))
#endif
		) )
#ifdef CONFIG_USER_RTK_WAN_CTYPE 
		&& (mibentry_p->applicationtype & X_CT_SRV_INTERNET)
#endif
		){
			//check PD wanconn
			if  ((mibentry_p->Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)
				|| (mibentry_p->AddrMode == IPV6_WAN_STATIC) // WAN static prefixv6 case
#ifdef	CONFIG_IPV6_SIT_6RD
				|| (mibentry_p->v6TunnelType == V6TUNNEL_6RD)
#endif
#ifdef CONFIG_IPV6_SIT
				|| (mibentry_p->v6TunnelType == V6TUNNEL_6TO4)
#endif
#ifdef CONFIG_USER_RTK_RA_DELEGATION
				||  ((mibentry_p->AddrMode & IPV6_WAN_AUTO) &&(!(mibentry_p->Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)))
#endif
		){
			if (mib_get_s(MIB_PREFIXINFO_PREFIX_MODE, (void *)&lanIPv6PrefixMode, sizeof(lanIPv6PrefixMode))) {
				if (lanIPv6PrefixMode == IPV6_PREFIX_DELEGATION) {
					if (mib_get_s(MIB_PD_WAN_DELEGATED_MODE, (void *)&pd_wan_delegated_mode, sizeof(pd_wan_delegated_mode))) {
						if (pd_wan_delegated_mode == PD_DELEGATED_STATIC) {
							if (mib_get_s(MIB_PREFIXINFO_DELEGATED_WANCONN, (void *)&old_wan_conn, sizeof(old_wan_conn)))
							{
								//Selected for WAN modified
								if ((old_wan_conn != mibentry_p->ifIndex) /*|| (old_ifIndex == DUMMY_IFINDEX)*/) {
									printf("Prefix Mode is WANDelegated, now change to this WAN, %x\n", mibentry_p->ifIndex);
									if (!mib_set(MIB_PREFIXINFO_DELEGATED_WANCONN, (void *)&mibentry_p->ifIndex))
										printf("Error! %s Fail to set MIB_PREFIXINFO_DELEGATED_WANCONN!\n", __FUNCTION__);
									else update_config++;
								}
							}
							else
								printf("Error! %s Fail to get MIB_PREFIXINFO_DELEGATED_WANCONN!\n", __FUNCTION__);
						}
						else if (pd_wan_delegated_mode != PD_DELEGATED_NONE) {
							//PD_DELEGATED_AUTO should get system default gw wan instead of mibentry_p->ifIndex
							auto_wan_conn = get_pd_wan_delegated_mode_ifindex();
							printf("Prefix Mode is WANDelegated, now change to this WAN, %x\n", auto_wan_conn);
							if (!mib_set(MIB_PREFIXINFO_DELEGATED_WANCONN, (void *)&auto_wan_conn))
								printf("Error! %s Fail to set MIB_PREFIXINFO_DELEGATED_WANCONN!\n", __FUNCTION__);
							else update_config++;
						}
					}
					else
						printf("Error! %s Fail to get MIB_PD_WAN_DELEGATED_MODE!\n", __FUNCTION__);
				}
			}
			else
				printf("Error! %s Fail to get MIB_PREFIXINFO_PREFIX_MODE!\n", __FUNCTION__);
		}
		
		//check DHCPv6 DNS wanconn
		{
			if (mib_get_s(MIB_LAN_DNSV6_MODE, (void *)&lanIPv6DnsMode, sizeof(lanIPv6DnsMode))) {
				if (lanIPv6DnsMode == IPV6_DNS_WANCONN) {
					if (mib_get_s(MIB_DNSV6INFO_WANCONN_MODE, (void *)&lan_IPv6_dns_wan_conn_mode, sizeof(lan_IPv6_dns_wan_conn_mode))) {
						if (lan_IPv6_dns_wan_conn_mode == IPv6_DNS_DELEGATED_STATIC) {
							if (mib_get_s(MIB_DNSINFO_WANCONN, (void *)&old_wan_conn, sizeof(old_wan_conn)))
							{
								//Selected for WAN modified
								if ((old_wan_conn != mibentry_p->ifIndex) /*|| (old_ifIndex == DUMMY_IFINDEX)*/) {
									printf("DNSv6 Mode is WANDelegated, now change to this WAN, %x\n", mibentry_p->ifIndex);
									if (!mib_set(MIB_DNSINFO_WANCONN, (void *)&mibentry_p->ifIndex))
										printf("Error! %s Fail to set MIB_DNSINFO_WANCONN!\n", __FUNCTION__);
									else update_config++;
								}
							}
							else
								printf("Error! %s Fail to get MIB_DNSINFO_WANCONN!\n", __FUNCTION__);
						}
						else if (lan_IPv6_dns_wan_conn_mode != IPv6_DNS_DELEGATED_NONE) {
							//IPv6_DNS_DELEGATED_AUTO should get system default gw wan instead of mibentry_p->ifIndex
							auto_wan_conn = get_dnsv6_wan_delegated_mode_ifindex();
							printf("DNSv6 Mode is WANDelegated, now change to this WAN, %x\n", auto_wan_conn);
							if (!mib_set(MIB_DNSINFO_WANCONN, (void *)&auto_wan_conn))
								printf("Error! %s Fail to set MIB_DNSINFO_WANCONN!\n", __FUNCTION__);
							else update_config++;
						}
					}
					else
						printf("Error! %s Fail to get MIB_DNSV6INFO_WANCONN_MODE!\n", __FUNCTION__);
				}
			}
			else
				printf("Error! %s Fail to get MIB_LAN_DNSV6_MODE!\n", __FUNCTION__);
		}
		//check RA RDNSS wanconn
		{
			if (mib_get_s(MIB_V6_RADVD_RDNSS_MODE, (void *)&radvd_rdnss_mode, sizeof(radvd_rdnss_mode))) {
				if (radvd_rdnss_mode == RADVD_RDNSS_WANCON) {
					if (mib_get_s(MIB_V6_RADVD_RDNSSINFO_WANCONN_MODE, (void *)&lan_rdnns_wan_conn_mode, sizeof(lan_rdnns_wan_conn_mode))) {
						if (lan_rdnns_wan_conn_mode == IPv6_RDNSS_DELEGATED_STATIC) {
							if (mib_get_s(MIB_V6_RADVD_DNSINFO_WANCONN, (void *)&old_wan_conn, sizeof(old_wan_conn)))
							{
								//Selected for WAN modified
								if ((old_wan_conn != mibentry_p->ifIndex) /*|| (old_ifIndex == DUMMY_IFINDEX)*/) {
									printf("RDNSS Mode is WANDelegated, now change to this WAN, %x\n", mibentry_p->ifIndex);
									if (!mib_set(MIB_V6_RADVD_DNSINFO_WANCONN, (void *)&mibentry_p->ifIndex))
										printf("Error! %s Fail to set MIB_V6_RADVD_DNSINFO_WANCONN!\n", __FUNCTION__);
									else update_config++;
								}
							}
							else
								printf("Error! %s Fail to get MIB_V6_RADVD_DNSINFO_WANCONN!\n", __FUNCTION__);
						}
						else if (lan_rdnns_wan_conn_mode != IPv6_RDNSS_DELEGATED_NONE) {
							//IPv6_RDNSS_DELEGATED_AUTO should get system default gw wan instead of mibentry_p->ifIndex
							auto_wan_conn = get_radvd_rdnss_wan_delegated_mode_ifindex();
							printf("RDNSS Mode is WANDelegated, now change to this WAN, %x\n", auto_wan_conn);
							if (!mib_set(MIB_V6_RADVD_DNSINFO_WANCONN, (void *)&auto_wan_conn))
								printf("Error! %s Fail to set MIB_V6_RADVD_DNSINFO_WANCONN!\n", __FUNCTION__);
							else update_config++;
						}
					}
					else
						printf("Error! %s Fail to get MIB_V6_RADVD_DNSINFO_WANCONN!\n", __FUNCTION__);
				}
			}
			else
				printf("Error! %s Fail to get MIB_V6_RADVD_RDNSS_MODE!\n", __FUNCTION__);
		}
	}

	// Handle bridge and non-Internet case
	else if ((mibentry_p->cmode == CHANNEL_MODE_BRIDGE) || !(mibentry_p->applicationtype & X_CT_SRV_INTERNET))
	{
		if (mib_get_s(MIB_PD_WAN_DELEGATED_MODE, (void *)&pd_wan_delegated_mode, sizeof(pd_wan_delegated_mode))) {
			if (pd_wan_delegated_mode == PD_DELEGATED_STATIC) {
				//Selected for WAN modified
				if ((old_wan_conn == mibentry_p->ifIndex)) { //Change to pd_wan_delegated auto
					auto_wan_conn = PD_DELEGATED_AUTO;
					if (!mib_set(MIB_PD_WAN_DELEGATED_MODE, (void *)&auto_wan_conn))
						printf("Error! %s Fail to set MIB_PD_WAN_DELEGATED_MODE!\n", __FUNCTION__);
					else update_config++;
				}
			}
		}

		//check DNS wanconn
		if (mib_get_s(MIB_DNSV6INFO_WANCONN_MODE, (void *)&lan_IPv6_dns_wan_conn_mode, sizeof(lan_IPv6_dns_wan_conn_mode))) {
			if (lan_IPv6_dns_wan_conn_mode == IPv6_DNS_DELEGATED_STATIC) {
				//Selected for WAN modified
				if ((old_wan_conn == mibentry_p->ifIndex)) { //Change to pd_wan_delegated auto
					auto_wan_conn = IPv6_DNS_DELEGATED_AUTO;
					if (!mib_set(MIB_DNSV6INFO_WANCONN_MODE, (void *)&auto_wan_conn))
						printf("Error! %s Fail to set MIB_DNSV6INFO_WANCONN_MODE!\n", __FUNCTION__);
					else update_config++;
				}
			}
		}
		//check RA RDNSS wanconn
		if (mib_get_s(MIB_V6_RADVD_RDNSSINFO_WANCONN_MODE, (void *)&lan_rdnns_wan_conn_mode, sizeof(lan_rdnns_wan_conn_mode))) {
			if (lan_rdnns_wan_conn_mode == IPv6_RDNSS_DELEGATED_STATIC) {
				//Selected for WAN modified
				if ((old_wan_conn == mibentry_p->ifIndex)) { //Change to pd_wan_delegated auto
					auto_wan_conn = IPv6_RDNSS_DELEGATED_AUTO;
					if (!mib_set(MIB_V6_RADVD_RDNSSINFO_WANCONN_MODE, (void *)&auto_wan_conn))
						printf("Error! %s Fail to set MIB_V6_RADVD_RDNSSINFO_WANCONN_MODE!\n", __FUNCTION__);
					else update_config++;
				}
			}
		}
	}

#ifdef CONFIG_USER_CONF_ON_XMLFILE
	//Bug fix: WAN from IPoE+DHCPv6+pd change to PPPoE+DHCPv6+pd, the LAN page WANDelegated interface empty 
	//RootCause:spppd ETH_PPPOE_DISCOVERY -> CMD_MIB_SAVE_PPPOE will reload xml file before Formeth function Commit() 
	//So, update MIB_PREFIXINFO_DELEGATED_WANCONN and MIB_DNSINFO_WANCONN to /var/config/config.xml immediately
	if (update_config && mib_update_ex(CURRENT_SETTING, CONFIG_MIB_ALL, 1) == 0)
		printf("CS Flash error! \n");
#endif
#ifdef CONFIG_TELMEX_DEV
	if (pdAutoGetFlag)
		ipv6RadvdPrefixMode = RADVD_MODE_AUTO;
	else
		ipv6RadvdPrefixMode = RADVD_MODE_MANUAL;
	mib_set(MIB_V6_RADVD_PREFIX_MODE, (void *)&ipv6RadvdPrefixMode);
#endif
}

void setup_ipv6_br_interface(const char* brname)
{
	char cmdBuf[100] = {0};
	char ipv6addr[INET6_ADDRSTRLEN] = {0};
	struct in6_addr ipv6_addr_n = {0};
	char accept_dad[8] = {0};
	char tmpBuf[64]={0};

	if( brname ==NULL)
	{
		printf("Invalid bridge group name !\n");
		return;
	}
	snprintf(cmdBuf, sizeof(cmdBuf), "echo 1 > /proc/sys/net/ipv6/conf/%s/forwarding", brname);
	system(cmdBuf);

	/* Bridge is Router behavior, it don't care Other RA */
	snprintf(cmdBuf, sizeof(cmdBuf), "echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra", brname);
	system(cmdBuf);

#if defined(CONFIG_RTK_IPV6_LOGO_TEST)
	/*if for logo test , auto config lan link-local MIB_IPV6_LAN_LLA_IP_ADDR to eui64*/
	unsigned char logoTestMode=0;
	if ( mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode)) ){
		/*
		 * Since the link-local mode of br0 should be auto mode in logo test, there is unnecessary to check LLA here.
		 * Otherwise, MIB_IPV6_LAN_LLA_IP_ADDR would be set which links the link-local mode would be set to static. 20220519
		 */
		if(logoTestMode == 1)
			checkLanLinklocalIPv6Address();
	}
#endif
	
	if (mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)tmpBuf, sizeof(tmpBuf)) != 0)
	{
#if defined(WEB_HTTP_SERVER_BIND_IP)
		FILE *fp = NULL;
		// backup old value
		snprintf(cmdBuf, sizeof(cmdBuf), "cat /proc/sys/net/ipv6/conf/%s/accept_dad", brname);
		fp = popen(cmdBuf, "r");
		if (fp != NULL)
		{
			if (fgets(accept_dad, sizeof(accept_dad), fp) != 0)
			{
				char *p = NULL;
				p = strchr(accept_dad, '\n');
				if (p) {
					*p = '\0';
				}
			}
			pclose(fp);
		}

		if (strcmp(accept_dad, "0") != 0)
		{
			snprintf(cmdBuf, sizeof(cmdBuf), "echo 0 > /proc/sys/net/ipv6/conf/%s/accept_dad", brname);
			system(cmdBuf);
		}
#endif

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 18, 20))
		if (!strcmp(tmpBuf, ""))
		{
			snprintf(cmdBuf, sizeof(cmdBuf), "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/addr_gen_mode", brname);
			system(cmdBuf);
			printf("Auto Link-Local Address for %s \n", brname);
		}
		else if (!inet_pton(AF_INET6, tmpBuf, &ipv6_addr_n) || !IN6_IS_ADDR_LINKLOCAL(&ipv6_addr_n))
		{
			/* If IP is not v6 IP or LLA IP, we don't set IPv6 IP for LANIF */
			printf("Invalid Link-Local Address for %s\n", brname);
			snprintf(cmdBuf, sizeof(cmdBuf), "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/addr_gen_mode", brname);
			system(cmdBuf);
		}
		else
		{
			snprintf(cmdBuf, sizeof(cmdBuf), "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/addr_gen_mode", brname);
			system(cmdBuf);
			setup_disable_ipv6(brname, 0); /* Because you need to open IPv6 proc before adding IP. */
			setup_disable_ipv6(LOCALHOST, 0);
			snprintf(cmdBuf, sizeof(cmdBuf), "%s/%d", tmpBuf, sizeof(tmpBuf));
			va_cmd(IFCONFIG, 3, 1, brname, ARG_ADD, cmdBuf);
			printf("Set Link-Local Address for %s\n", brname);
		}
#else
		if (!strcmp(tmpBuf, ""))
		{
			printf("Auto Link-Local Address for %s \n", brname);
		}
		else if (!inet_pton(AF_INET6, tmpBuf, &ipv6_addr_n) || !IN6_IS_ADDR_LINKLOCAL(&ipv6_addr_n))
		{
			/* If IP is not v6 IP or LLA IP, we don't set IPv6 IP for LANIF */
			printf("Invalid Link-Local Address for %s\n", brname);
		}
		else
		{
			setup_disable_ipv6(brname, 0); /* Because you need to open IPv6 proc before adding IP. */
			setup_disable_ipv6(LOCALHOST, 0);
			snprintf(cmdBuf, sizeof(cmdBuf), "%s/%d", tmpBuf, sizeof(tmpBuf));
			va_cmd(IFCONFIG, 3, 1, brname, ARG_ADD, cmdBuf);
			printf("Set Link-Local Address for %s\n", brname);
		}
#endif
	}
	setup_disable_ipv6(brname, 0);
	setup_disable_ipv6(LOCALHOST, 0);
#if defined(WEB_HTTP_SERVER_BIND_IP)
		if (strcmp(accept_dad, "0") != 0)
		{
			snprintf(cmdBuf, sizeof(cmdBuf), "echo %s > /proc/sys/net/ipv6/conf/%s/accept_dad", accept_dad, LANIF);
			system(cmdBuf);
		}
#endif
#ifdef CONFIG_SECONDARY_IPV6
	if (mib_get_s(MIB_IPV6_LAN_IP_ADDR2, (void *)tmpBuf, sizeof(tmpBuf)) != 0)
	{
		char cmdBuf[100]={0};
		snprintf(cmdBuf, sizeof(cmdBuf), "%s/%d", tmpBuf, 64);
		//dhclient will do PREINIT6 in /etc/dhclient-script "
		//# ${ip} -6 addr flush dev ${interface} scope global permanent"
		//So we add secondart IPv6 for a long lifetime
		va_cmd(BIN_IP, 10, 1,"-6",	"addr", ARG_ADD, cmdBuf, "dev", LANIF, "valid_lft", "0xfffffffe", "preferred_lft" ,"0xfffffffe");
	}
#endif
#ifdef CONFIG_00R0 //enable anycast address fe80:: for source address
	system("/bin/echo 1 > /proc/sys/net/ipv6/anycast_src_echo_reply");
#endif
}

unsigned int get_pd_wan_delegated_mode_ifindex(void)
{
	unsigned int wanconn = DUMMY_IFINDEX;
	unsigned char pd_wan_delegated_mode = PD_DELEGATED_NONE;
	MIB_CE_ATM_VC_T entry = {0};
	unsigned int entryNum = 0, i = 0;

	if (!mib_get_s(MIB_PD_WAN_DELEGATED_MODE, (void *)&pd_wan_delegated_mode, sizeof(pd_wan_delegated_mode))) {
		goto get_mib_fail;
	}

	if (pd_wan_delegated_mode == PD_DELEGATED_AUTO) {
		wanconn = getDefaultGWIPv6WANIfindex();
		if (wanconn == DUMMY_IFINDEX) {
			entryNum = mib_chain_total(MIB_ATM_VC_TBL);
			for (i = 0; i < entryNum; i++) {
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
					continue;

				if (entry.enable == 0)
					continue;
				if (entry.cmode == CHANNEL_MODE_BRIDGE)
					continue;
#ifdef CONFIG_USER_RTK_WAN_CTYPE
				if (!(entry.applicationtype & X_CT_SRV_INTERNET))
					continue;
#endif
#ifdef  CONFIG_IPV6_SIT_6RD
				if ((entry.IpProtocol == IPVER_IPV4 && entry.v6TunnelType == V6TUNNEL_6RD))
				{
					wanconn = entry.ifIndex;
					break;
				}
#endif
#ifdef CONFIG_IPV6_SIT
				if ((entry.IpProtocol == IPVER_IPV4) && ((entry.v6TunnelType == V6TUNNEL_6TO4) || (entry.v6TunnelType == V6TUNNEL_6IN4)))
				{
					wanconn = entry.ifIndex;
					break;
				}
#endif
#ifdef CONFIG_USER_RTK_RA_DELEGATION
				if ((entry.AddrMode & IPV6_WAN_AUTO) &&(!(entry.Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)))
				{
					wanconn = entry.ifIndex;
					break;
				}
#endif
			}
		}
	}
	else {
		if (!mib_get_s(MIB_PREFIXINFO_DELEGATED_WANCONN, (void *)&wanconn, sizeof(wanconn))) {
			goto get_mib_fail;
		}
	}
	return wanconn;
get_mib_fail:
	return 0xFFFFFFFF; //means can't get wanconn
}

unsigned int get_dnsv6_wan_delegated_mode_ifindex(void)
{
	unsigned int wanconn = DUMMY_IFINDEX;
	unsigned char dnsv6_wan_delegated_mode = IPv6_DNS_DELEGATED_NONE;
	MIB_CE_ATM_VC_T entry = {0};
	unsigned int entryNum = 0, i = 0;
	
	//typedef enum { IPv6_DNS_DELEGATED_AUTO = 0, IPv6_DNS_DELEGATED_STATIC = 1, IPv6_DNS_DELEGATED_NONE = 2 } IPv6_DNS_WAN_DELEGATED_MODE;

	if (!mib_get_s(MIB_DNSV6INFO_WANCONN_MODE, (void *)&dnsv6_wan_delegated_mode, sizeof(dnsv6_wan_delegated_mode))) {
		goto get_mib_fail;
	}

	if (dnsv6_wan_delegated_mode == PD_DELEGATED_AUTO) {
		wanconn = getDefaultGWIPv6WANIfindex();
		if (wanconn == DUMMY_IFINDEX) {
			entryNum = mib_chain_total(MIB_ATM_VC_TBL);
			for (i = 0; i < entryNum; i++) {
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
					continue;

				if (entry.enable == 0)
					continue;
				if (entry.cmode == CHANNEL_MODE_BRIDGE)
					continue;
#ifdef CONFIG_USER_RTK_WAN_CTYPE
				if (!(entry.applicationtype & X_CT_SRV_INTERNET))
					continue;
#endif
#ifdef  CONFIG_IPV6_SIT_6RD
				if ((entry.IpProtocol == IPVER_IPV4 && entry.v6TunnelType == V6TUNNEL_6RD))
				{
					wanconn = entry.ifIndex;
					break;
				}
#endif
#ifdef CONFIG_IPV6_SIT
				if ((entry.IpProtocol == IPVER_IPV4) && ((entry.v6TunnelType == V6TUNNEL_6TO4) || (entry.v6TunnelType == V6TUNNEL_6IN4)))
				{
					wanconn = entry.ifIndex;
					break;
				}
#endif
#ifdef CONFIG_USER_RTK_RA_DELEGATION
				if ((entry.AddrMode & IPV6_WAN_AUTO) &&(!(entry.Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)))
				{
					wanconn = entry.ifIndex;
					break;
				}
#endif
			}
		}
	}
	else {
		if (!mib_get_s(MIB_DNSINFO_WANCONN, (void *)&wanconn, sizeof(wanconn))) {
			goto get_mib_fail;
		}
	}
	return wanconn;
get_mib_fail:
	return 0xFFFFFFFF; //means can't get wanconn
}

unsigned int get_radvd_rdnss_wan_delegated_mode_ifindex(void)
{
	unsigned int wanconn = DUMMY_IFINDEX;
	unsigned char rdnss_wan_delegated_mode = IPv6_RDNSS_DELEGATED_NONE;
	MIB_CE_ATM_VC_T entry = {0};
	unsigned int entryNum = 0, i = 0;

	if (!mib_get_s(MIB_V6_RADVD_RDNSSINFO_WANCONN_MODE, (void *)&rdnss_wan_delegated_mode, sizeof(rdnss_wan_delegated_mode))) {
		printf("Error!! %s Get MIB_V6_RADVD_RDNSSINFO_WANCONN_MODE fail!", __FUNCTION__);
		goto get_mib_fail;
	}

	if (rdnss_wan_delegated_mode == IPv6_RDNSS_DELEGATED_AUTO) {
		wanconn = getDefaultGWIPv6WANIfindex();
		if (wanconn == DUMMY_IFINDEX) {
			entryNum = mib_chain_total(MIB_ATM_VC_TBL);
			for (i = 0; i < entryNum; i++) {
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
					continue;

				if (entry.enable == 0)
					continue;
				if (entry.cmode == CHANNEL_MODE_BRIDGE)
					continue;
#ifdef CONFIG_USER_RTK_WAN_CTYPE
				if (!(entry.applicationtype & X_CT_SRV_INTERNET))
					continue;
#endif
#ifdef  CONFIG_IPV6_SIT_6RD
				if ((entry.IpProtocol == IPVER_IPV4 && entry.v6TunnelType == V6TUNNEL_6RD))
				{
					wanconn = entry.ifIndex;
					break;
				}
#endif
#ifdef CONFIG_IPV6_SIT
				if ((entry.IpProtocol == IPVER_IPV4) && ((entry.v6TunnelType == V6TUNNEL_6TO4) || (entry.v6TunnelType == V6TUNNEL_6IN4)))
				{
					wanconn = entry.ifIndex;
					break;
				}
#endif
#ifdef CONFIG_USER_RTK_RA_DELEGATION
				if ((entry.AddrMode & IPV6_WAN_AUTO) &&(!(entry.Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)))
				{
					wanconn = entry.ifIndex;
					break;
				}
#endif
			}
		}
	}
	else {
		if (!mib_get_s(MIB_V6_RADVD_DNSINFO_WANCONN, (void *)&wanconn, sizeof(wanconn))) {
			goto get_mib_fail;
		}
	}
	return wanconn;
get_mib_fail:
	return 0xFFFFFFFF; //means can't get wanconn
}
#endif //#ifdef CONFIG_IPV6

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include "mib.h"
#include "utility.h"
 
#ifdef EMBED
#include <linux/config.h>
#include <config/autoconf.h>
#else
#include "../../../../include/linux/autoconf.h"
#include "../../../../config/autoconf.h"
#endif

#if defined(DHCPV6_ISC_DHCP_4XX) || defined (CONFIG_IPV6)
//const char DHCPDV6_CONF_AUTO[]="/var/dhcpd6_auto.conf";
const char DHCPDV6_CONF[]="/var/dhcpd6.conf";         //ISC-DHCPv6 Server config
const char DHCPDV6_LEASES[]="/var/dhcpd6.leases";
const char DHCPDV6_STATIC_LEASES[]="/tmp/dhcpd6.static.leases";
const char DHCPDV6[]="/bin/dhcpd";
const char DHCREALYV6[]="/bin/dhcrelayV6";
const char DHCPSERVER6PID[]="/var/run/dhcpd6.pid";
const char DHCPRELAY6PID[]="/var/run/dhcrelay6.pid";
const char DHCPCV6SCRIPT[]="/etc/dhclient-script";
const char DHCPCV6[]="/bin/dhclient";
const char PATH_DHCLIENT6_CONF[]="/var/dhcpcV6.conf";
/* E8B fc yueme use new our own DUID_LL for ISC-DHCPv6, so we don't need to remember our lease IA info.
 * This mechanism only use in ISC-DHCPv6 4.1.1, because 4.4.1 have option to accept DUID type for user.
 */
#ifdef CONFIG_CMCC
const char PATH_DHCLIENT6_LEASE[]="/var/config/dhcpcV6.leases";
#else
 #ifndef CONFIG_TELMEX
const char PATH_DHCLIENT6_LEASE[]="/var/run/dhcpcV6.leases";
 #else
const char PATH_DHCLIENT6_LEASE[]="/var/config/dhcpcV6.leases"; //If you want to store lease info in flash, use this.
#endif
#endif
const char PATH_DHCLIENT6_PID[]="/var/run/dhcpcV6.pid";
const char DHCPCV6STR[]="dhcpcV6";
#endif
#ifdef CONFIG_IPV6
const char DHCP6S_LAN_PD_ROUTE_PATH[] = "/var/run/dhcp6s_lan_pd_route";
#endif

static unsigned char ipv6_unspec[IP6_ADDR_LEN] = {0};
extern char *strptime(const char *s, const char *format, struct tm *tm);

#ifdef CONFIG_IPV6
#ifdef DHCPV6_ISC_DHCP_4XX
int option_name_server(FILE *fp)
{
	unsigned int entryNum, i;
	MIB_DHCPV6S_NAME_SERVER_T Entry;
	unsigned char strAll[(MAX_V6_IP_LEN+2)*MAX_DHCPV6_CHAIN_ENTRY]="";
	
	entryNum = mib_chain_total(MIB_DHCPV6S_NAME_SERVER_TBL);

	for (i=0; i<entryNum; i++) {
		unsigned char buf[MAX_V6_IP_LEN+2]="";
		
		if (!mib_chain_get(MIB_DHCPV6S_NAME_SERVER_TBL, i, (void *)&Entry))
		{
  			printf("option_name_server: Get Name Server chain record error!\n");
			return -1;
		}
		
		if ( i< (entryNum-1) )
		{
			sprintf(buf, "%s, ", Entry.nameServer);
		} else
			sprintf(buf, "%s", Entry.nameServer);
		strcat(strAll, buf);		
	}
	
	if ( entryNum > 0 )
	{
		if(strAll[0]) {
			//printf("strAll=%s\n", strAll);
			fprintf(fp, "option dhcp6.name-servers %s;\n", strAll);
		}
	}
	
	return 0;
}

int option_domain_search(FILE *fp)
{
	unsigned int entryNum, i;
	MIB_DHCPV6S_DOMAIN_SEARCH_T Entry;
	unsigned char strAll[(MAX_DOMAIN_LENGTH+4)*MAX_DHCPV6_CHAIN_ENTRY]="";
	
	entryNum = mib_chain_total(MIB_DHCPV6S_DOMAIN_SEARCH_TBL);

	for (i=0; i<entryNum; i++) {
		unsigned char buf[MAX_DOMAIN_LENGTH+4]="";
		
		if (!mib_chain_get(MIB_DHCPV6S_DOMAIN_SEARCH_TBL, i, (void *)&Entry))
		{
  			printf("option_name_server: Get domain search chain record error!\n");
			return -1;
		}
		
		if ( i< (entryNum-1) )
		{
			sprintf(buf, "\"%s\", ", Entry.domain);
		} else
			sprintf(buf, "\"%s\"", Entry.domain);
		strcat(strAll, buf);		
	}
	
	if ( entryNum > 0 )
	{
		//printf("strAll(domain)=%s\n", strAll);
		fprintf(fp, "option dhcp6.domain-search %s;\n", strAll);
	}
	
	return 0;
}

int option_ntp_server(FILE *fp)
{
	unsigned int entryNum, i;
	MIB_DHCPV6S_NTP_SERVER_T Entry;
	unsigned char strAll[(MAX_V6_IP_LEN+2)*MAX_DHCPV6_CHAIN_ENTRY]="";

	entryNum = mib_chain_total(MIB_DHCPV6S_NTP_SERVER_TBL);

	for (i=0; i<entryNum; i++) {
		unsigned char buf[MAX_V6_IP_LEN+2]="";

		if (!mib_chain_get(MIB_DHCPV6S_NTP_SERVER_TBL, i, (void *)&Entry))
		{
  			printf("option_ntp_server: Get ntp server chain record error!\n");
			return -1;
		}

		if ( i< (entryNum-1) )
		{
			sprintf(buf, "%s, ", Entry.ntpServer);
		} else
			sprintf(buf, "%s", Entry.ntpServer);
		strcat(strAll, buf);
	}

	if ( entryNum > 0 )
	{
		//printf("strAll(domain)=%s\n", strAll);
		fprintf(fp, "option dhcp6.sntp-servers %s;\n", strAll);
	}

	return 0;
}

int option_mac_binding(FILE *fp)
{
	unsigned int entryNum, i;
	MIB_DHCPV6S_MAC_BINDING_T Entry;

	entryNum = mib_chain_total(MIB_DHCPV6S_MAC_BINDING_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_DHCPV6S_MAC_BINDING_TBL, i, (void *)&Entry))
		{
  			printf("option_mac_binding: Get mac binding chain record error!\n");
			return -1;
		}
		fprintf(fp, "host %s {\n", Entry.hostName);
		fprintf(fp, "    hardware ethernet %02x:%02x:%02x:%02x:%02x:%02x ;\n", 
			Entry.macAddr[0],Entry.macAddr[1],Entry.macAddr[2],Entry.macAddr[3],Entry.macAddr[4],Entry.macAddr[5]);
		fprintf(fp, "    fixed-address6 %s ;\n", Entry.ipAddress);
		fprintf(fp, "}\n");
	}
	return 0;
}
//Helper function for PrefixV6 for dhcpv6 mode
/* Get dhcpv6 Prefix information for IPv6 servers.
 * 0 : successful
 * -1: failed
 */
int get_dhcpv6_prefixv6_info(PREFIX_V6_INFO_Tp pPrefixInfo)
{
	unsigned int wanconn = DUMMY_IFINDEX;
	DLG_INFO_T dlgInfo={0};
	unsigned char dhcpv6_type = 0;
	unsigned char  ip6Addr[IP6_ADDR_LEN]={0};
	unsigned int PFTime = 0, RNTime = 0, RBTime = 0, MLTime = 0;
	unsigned char prefix_len=0;

	if(!pPrefixInfo){
		printf("Error! NULL input prefixV6Info\n");
		goto setErr_ipv6;
	}

	if (!mib_get_s(MIB_DHCPV6S_TYPE, (void *)&dhcpv6_type, sizeof(dhcpv6_type))){
		printf("[%s:%d] Error!! Get LAN  DHCPv6 server  Type failed\n",__func__,__LINE__);
		goto setErr_ipv6;
	}

	pPrefixInfo->mode = dhcpv6_type;

	switch(dhcpv6_type){
		case DHCPV6S_TYPE_AUTO:
			wanconn = get_pd_wan_delegated_mode_ifindex();
			if (wanconn == 0xFFFFFFFF) {
				printf("Error!! %s get WAN Delegated PREFIXINFO fail!", __FUNCTION__);
				goto setErr_ipv6;
			}

			// Due to ATM's ifIndex can be 0
			pPrefixInfo->wanconn = wanconn;
			if (pPrefixInfo->wanconn == DUMMY_IFINDEX) { // auto mode, get lease from most-recently leasefile.
				if (access("/var/prefix_info", F_OK) == -1)
					goto setErr_ipv6;
				getLeasesInfo("/var/prefix_info", &dlgInfo);
				pPrefixInfo->wanconn = getIfIndexByName(dlgInfo.wanIfname);
			}

			if (get_prefixv6_info_by_wan(pPrefixInfo, pPrefixInfo->wanconn)!=0){
				//AUG_PRT("Error! Could not get delegation info from wanconn %d\n", prefixInfo->wanconn);
				goto setErr_ipv6;
			}

			break;
		case DHCPV6S_TYPE_STATIC:
			mib_get_s(MIB_DHCPV6S_RANGE_START, (void *)ip6Addr, sizeof(ip6Addr));
			mib_get_s(MIB_DHCPV6S_PREFIX_LENGTH, (void *)&prefix_len, sizeof(prefix_len));
			if (prefix_len<=0 || prefix_len > 128) {
				printf("WARNNING! Prefix Length == %d\n", prefix_len);
				prefix_len = 64;
			}
			ip6toPrefix(&ip6Addr, prefix_len, pPrefixInfo->prefixIP);
			pPrefixInfo->prefixLen = prefix_len;
			// default-lease-time
			mib_get_s(MIB_DHCPV6S_DEFAULT_LEASE, (void *)&MLTime, sizeof(MLTime));
			pPrefixInfo->MLTime = MLTime;
			// preferred-lifetime
			mib_get_s(MIB_DHCPV6S_PREFERRED_LIFETIME, (void *)&PFTime, sizeof(PFTime));
			pPrefixInfo->PLTime = PFTime;
			mib_get_s(MIB_DHCPV6S_RENEW_TIME, (void *)&RNTime, sizeof(RNTime));
			pPrefixInfo->RNTime = RNTime;
			mib_get_s(MIB_DHCPV6S_REBIND_TIME, (void *)&RBTime, sizeof(RBTime));
			pPrefixInfo->RBTime = RBTime;

			break;
#if defined(CONFIG_USER_RTK_RA_DELEGATION) && defined(CONFIG_USER_RTK_RAMONITOR)
		case DHCPV6S_TYPE_RA_DELEGATION:
			wanconn = get_pd_wan_delegated_mode_ifindex();
			if (wanconn == 0xFFFFFFFF) {
				printf("Error!! %s get WAN Delegated PREFIXINFO fail!", __FUNCTION__);
				goto setErr_ipv6;
			}
			pPrefixInfo->wanconn = wanconn;
			if (wanconn!=DUMMY_IFINDEX && get_prefixv6_info_by_wan(pPrefixInfo, pPrefixInfo->wanconn)!=0){
				AUG_PRT("Error! Could not get delegation info from wanconn %d\n", pPrefixInfo->wanconn);
				goto setErr_ipv6;
			}

#endif
		default:
			AUG_PRT("Error! Not support this mode %d!\n", pPrefixInfo->mode);
	}
	return 0;

setErr_ipv6:
	return -1;
}

#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
extern int get_prefixv6_info_by_wan(PREFIX_V6_INFO_Tp prefixInfo, int ifindex);
static int setup_dhcpv6_bind_subnet_dns(DNS_V6_INFO_Tp pDnsV6Info, PREFIX_V6_INFO_Tp pPrefixInfo)
{
	unsigned dhcpv6_type =0;
	unsigned int tmpint=0;
	unsigned char tmpstr[256]={0}, prefix[MAX_V6_IP_LEN]={0},min_addr[MAX_V6_IP_LEN]={0}, max_addr[MAX_V6_IP_LEN]={0};
	FILE *fp=NULL;
	unsigned char domainSearch[MAX_DOMAIN_NUM*MAX_DOMAIN_LENGTH] = {0};
	struct in6_addr ip6Addr, ip6Prefix;
	unsigned char ip6Addr_min[IP6_ADDR_LEN]={0},ip6Addr_max[IP6_ADDR_LEN]={0};
#if defined(CONFIG_USER_RTK_DHCPV6_SERVER_PD_SUPPORT) || defined(YUEME_DHCPV6_SERVER_PD_SUPPORT)
	struct ipv6_addr_str_array prefix6_subnet_start = {0}, prefix6_subnet_end = {0};
	int lan_pd_len=0;
#endif
	unsigned char mFlag=0;
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	unsigned char logoTestMode = 0 ;
#endif
	
	int idx=0;
	MIB_CE_ATM_VC_T entry;

	if ((fp = fopen(DHCPDV6_CONF, "a")) == NULL) {
		printf("Open file %s failed !\n", DHCPDV6_CONF);
		return -1;
	}

	if (!mib_get_s(MIB_DHCPV6S_TYPE, (void *)&dhcpv6_type, sizeof(dhcpv6_type))){
		printf("[%s:%d] Get DHCPV6 server Type failed\n",__func__,__LINE__);
		fclose(fp);
		return 0;
	}

	if ( !mib_get_s( MIB_V6_AUTONOMOUS, (void *)&mFlag, sizeof(mFlag)) )
		printf("Get MIB_V6_AUTONOMOUS error!");
	//subnet6 2001:b010:7030:1a01::/64 {
	//        range6 2001:b010:7030:1a01::/64 or range6 low-address high-address;
	//}	
	if (memcmp(pPrefixInfo->prefixIP, ipv6_unspec, pPrefixInfo->prefixLen<=128?pPrefixInfo->prefixLen:128) && pPrefixInfo->prefixLen) {
		struct  in6_addr ip6Addr, ip6Addr2;
		unsigned char minaddr[IP6_ADDR_LEN]={0},maxaddr[IP6_ADDR_LEN]={0};
		unsigned char prefixBuf[100]={0},tmpBuf[100]={0},tmpBuf2[100]={0};
		int i;
	
		inet_ntop(PF_INET6,pPrefixInfo->prefixIP, prefixBuf, sizeof(prefixBuf));
		printf("prefix is %s/%d\n",prefixBuf, pPrefixInfo->prefixLen);
#ifdef CONFIG_USER_RTK_RA_DELEGATION
		//get prefix info form /var/rainfo_<ifname>
		//RA delegation prefix format: 
		//RA prefix(64 bits) +  MAC(48bit) + 0(8bits) =120 bits
		if ((dhcpv6_type ==DHCPV6S_TYPE_RA_DELEGATION) && (pPrefixInfo->prefixLen ==120)){
			fprintf(fp, "subnet6 %s/120 {\n",prefixBuf);
			snprintf(min_addr, INET6_ADDRSTRLEN, "%s10",prefixBuf);
			snprintf(max_addr,INET6_ADDRSTRLEN, "%sff",prefixBuf);
			printf("range6 %s - %s;\n",min_addr,max_addr);
			if(!mFlag)
				fprintf(fp, "\trange6 %s %s;\n", min_addr,max_addr);
		}
		else
#endif
		{
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
			if (mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode))){
				if(logoTestMode == 3) //CE_ROUTER mode
					fprintf(fp, "subnet6 %s/64 {\n", prefixBuf);
			}
#else
			fprintf(fp, "subnet6 %s/%d {\n", prefixBuf, (pPrefixInfo->prefixLen<64)?64:pPrefixInfo->prefixLen);
#endif
			//Merge the prefix & min/max address 
			mib_get_s(MIB_DHCPV6S_MIN_ADDRESS, (void *)&tmpstr, sizeof(tmpstr));
			sprintf(tmpBuf,"::%s",tmpstr);
			inet_pton(PF_INET6,tmpBuf,&ip6Addr_min);

			mib_get_s(MIB_DHCPV6S_MAX_ADDRESS, (void *)&tmpstr, sizeof(tmpstr));
			sprintf(tmpBuf,"::%s",tmpstr);
			inet_pton(PF_INET6,tmpBuf, &ip6Addr_max);
			
			for (i=0; i<8; i++){
				ip6Addr_min[i] = pPrefixInfo->prefixIP[i];
				ip6Addr_max[i] = pPrefixInfo->prefixIP[i];
			}

			if (dhcpv6_type == DHCPV6S_TYPE_AUTO_DNS_ONLY ) {
				strcpy(min_addr, "::");
				strcpy(max_addr, "::");
			}
			else {
				inet_ntop(PF_INET6, &ip6Addr_min, min_addr, sizeof(min_addr));
				inet_ntop(PF_INET6, &ip6Addr_max, max_addr, sizeof(max_addr));
			}
			printf("range6 %s - %s;\n",min_addr,max_addr);
			if(!mFlag)
				fprintf(fp, "\trange6 %s %s;\n", min_addr,max_addr);
		}
		if (pDnsV6Info->nameServer[0]){
			fprintf(fp, "\toption dhcp6.name-servers %s;\n", pDnsV6Info->nameServer);
		}
	
		if (pDnsV6Info->dnssl_num >0){
			for (idx=0; idx <pDnsV6Info->dnssl_num; idx++){
				if(idx == pDnsV6Info->dnssl_num-1)
					snprintf(domainSearch+strlen(domainSearch),sizeof(domainSearch)-strlen(domainSearch), "\"%s\" ", pDnsV6Info->domainSearch[idx]);
				else
					snprintf(domainSearch+strlen(domainSearch),sizeof(domainSearch)-strlen(domainSearch), "\"%s\",", pDnsV6Info->domainSearch[idx]);
			}
			printf("%s  : DNSSL have %d entry %s\n",__func__, pDnsV6Info->dnssl_num , domainSearch);
			fprintf(fp, "\toption dhcp6.domain-search %s;\n", domainSearch);
		}
#if defined(CONFIG_USER_RTK_DHCPV6_SERVER_PD_SUPPORT) || defined(YUEME_DHCPV6_SERVER_PD_SUPPORT)
		if (calculate_prefix6_range(pPrefixInfo, &prefix6_subnet_start, &prefix6_subnet_end,&lan_pd_len) > 0)
		{
			printf("%s: dynamic assigned prefix6 %s %s /%d\n", __FUNCTION__, prefix6_subnet_start.ipv6_addr_str, prefix6_subnet_end.ipv6_addr_str,lan_pd_len);
			fprintf(fp, "\tprefix6 %s %s /%d;\n", prefix6_subnet_start.ipv6_addr_str, prefix6_subnet_end.ipv6_addr_str,lan_pd_len);
		}
#endif
		fprintf(fp, "\twbindintf %s;\n", BRIF);

		fprintf(fp, "}\n");
	}
	fclose(fp);
	
	return 1;
}

static int setup_binding_lan_dhcpv6_conf(void)
{
	int total = mib_chain_total(MIB_ATM_VC_TBL);
	int i = 0, j = 0;
	MIB_CE_ATM_VC_T entry;
	char ifname[IFNAMSIZ] = {0};
	DNS_V6_INFO_T dnsV6Info = {0};
	DNS_V6_INFO_T dnsV6Info_tmp={0};
	PREFIX_V6_INFO_T prefixInfo = {0};
	PREFIX_V6_INFO_T prefixInfo_default_wan = {0};
	unsigned char dhcpv6_type =0, dhcpv6_mode=0;
	unsigned char  ip6Addr[IP6_ADDR_LEN]={0}, ip6Prefix[IP6_ADDR_LEN]={0};
	unsigned int PFTime = 0, RNTime = 0, RBTime = 0, MLTime = 0;
	unsigned int prefix_len=0;
	unsigned char cmd[256]={0};
	char prefix_str[INET6_ADDRSTRLEN]={0};

	// Initial LAN phy port interface forwarding.
	set_ipv6_all_lan_phy_inf_forwarding(0);
//#ifdef CONFIG_NETFILTER_XT_TARGET_TEE
	//va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", "multi_lan_dhcpv6_ds_TEE");
	//va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "OUTPUT", "-j","multi_lan_dhcpv6_ds_TEE");
//#endif
	mib_get_s(MIB_DHCPV6S_TYPE, (void *)&dhcpv6_type, sizeof(dhcpv6_type));

	if (!mib_get_s(MIB_DHCPV6_MODE, (void *)&dhcpv6_mode, sizeof(dhcpv6_mode))){
		printf("[%s:%d] Get DHCPV6 server Mode failed\n",__func__,__LINE__);
		return -1;
	}

	for(i = 0 ; i < total ; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
			continue;
		if (ifGetName(entry.ifIndex, ifname, IFNAMSIZ) == 0)
			continue;
		if (!(entry.IpProtocol & IPVER_IPV6))
			continue;

		if (entry.cmode == CHANNEL_MODE_BRIDGE) //dont send DHCPv6 packet to port which bind bridge WAN
			continue;
		//dont send DHCPv6 packet to port which bind bridge WAN
		if(!ROUTE_CHECK_BRIDGE_MIX_ENABLE(&entry))
			continue;

		if ((entry.itfGroup ==0)
		#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
			&& (entry.vlan == 0 || (entry.vlan && !isVlanMappingWAN(entry.vid)))
		#endif
			)
			continue;

		//dhcpv6 server can not startup without prefix
		if(get_prefixv6_info_by_wan(&prefixInfo, entry.ifIndex) != 0){
			printf("[%s:%d] get_prefixv6_info_by_wan failed wan index = %d!\n", __FUNCTION__, __LINE__, entry.ifIndex);

			//if can not get wan PD, make binding port to get lan static prefix if user set static prefix(e.g. NAT/NAPT6 networking)
			if (dhcpv6_type == DHCPV6S_TYPE_STATIC){
				mib_get_s(MIB_DHCPV6S_RANGE_START, (void *)ip6Addr, sizeof(ip6Addr));
				mib_get_s(MIB_DHCPV6S_PREFIX_LENGTH, (void *)&prefix_len, sizeof(prefix_len));
				if (prefix_len<=0 || prefix_len > 128) {
					printf("WARNNING! Prefix Length == %d\n", prefix_len);
					prefix_len = 64;
				}
				ip6toPrefix(&ip6Addr, prefix_len, prefixInfo.prefixIP);
				prefixInfo.prefixLen = prefix_len;
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
			}else{
				memset(&prefixInfo, 0, sizeof(prefixInfo));
			}
		}

		if (prefixInfo.prefixLen && memcmp(prefixInfo.prefixIP, ipv6_unspec, prefixInfo.prefixLen<=128?prefixInfo.prefixLen:128))
		{
			if(get_dnsv6_info_by_wan(&dnsV6Info, entry.ifIndex) != 0){
				printf("[%s:%d] get_dnsv6_info_by_wan failed for wan index = %d!\n", __FUNCTION__, __LINE__, entry.ifIndex);
				//subnet conf can have no dns info but can not have no prefix
				//continue;
			}

			//make sure dhcpd6.conf has no duplicate subnet
			if (dhcpv6_type == DHCPV6S_TYPE_STATIC){
				mib_get_s(MIB_DHCPV6S_RANGE_START, (void *)ip6Addr, sizeof(ip6Addr));
				mib_get_s(MIB_DHCPV6S_PREFIX_LENGTH, (void *)&prefix_len, sizeof(prefix_len));
				if (prefix_len<=0 || prefix_len > 128) {
					printf("WARNNING! Prefix Length == %d\n", prefix_len);
					prefix_len = 64;
				}
				ip6toPrefix(&ip6Addr, prefix_len, &ip6Prefix);
			}else{
				if (get_prefixv6_info(&prefixInfo_default_wan) !=0){
					printf("[%s:%d] get_prefixv6_info failed for default wan !\n", __FUNCTION__, __LINE__);
				}
				ip6toPrefix(&prefixInfo_default_wan.prefixIP, 64, &ip6Prefix);
			}

			/* Compare prefix is for avoiding repeated subnet6 config with default PD subnet6 config.
			 * like:
			 * subnet6 8022:2018:50:10::/64 {
			 *         range6 8022:2018:50:10:1:1:1:1 8022:2018:50:10:2:2:2:2;
			 * }
			 * subnet6 8022:2018:50:10::/64 { -- repeated subnet6
			 *         range6 8022:2018:50:10:1:1:1:1 8022:2018:50:10:2:2:2:2;
			 * }
			 */
			if (dhcpv6_mode == DHCP_LAN_SERVER){//DHCP_LAN_SERVER mode, update the dhcpd6.conf with dns and prefix
				if (memcmp(prefixInfo.prefixIP, ip6Prefix,IP6_ADDR_LEN)){
					setup_dhcpv6_bind_subnet_dns(&dnsV6Info, &prefixInfo);
				}
#if 0			//Now we only set DNS info with DNS proxy fe80::1, no WAN connection DNS info
				else {
					unsigned char ipv6DnsMode=0;
					/* Binding wan is also default wan situation, so unbinding/binding lan will use the same prefix(subnet)
					* but isc-dhcp support only one dns server option in same subnet,
					* we should set static or proxy dns server(for unbinding) + wan dns server(for binding), and static/proxy dns have higher priority
					* like:
					* option dhcp6.name-servers <proxy dns> or <static dns>,<dns from wan>;
					*/
					mib_get_s(MIB_LAN_DNSV6_MODE, (void *)&ipv6DnsMode, sizeof(ipv6DnsMode));
					if (ipv6DnsMode != IPV6_DNS_WANCONN){
						if(get_dnsv6_info(&dnsV6Info_tmp)==0){
							if (dnsV6Info_tmp.nameServer[0] && dnsV6Info.nameServer[0]){
								snprintf(cmd,sizeof(cmd),"sed -i \'s/option dhcp6.name-servers %s/option dhcp6.name-servers %s,%s/g\' %s",
									dnsV6Info_tmp.nameServer, dnsV6Info_tmp.nameServer,dnsV6Info.nameServer, DHCPDV6_CONF);
								//sed -i 's/option dhcp6.name-servers  <proxy dns> or <static dns>/option dhcp6.name-servers <proxy dns> or <static dns>,<dns from wan>/g' /var/dhcpd6.conf
								va_cmd("/bin/sh",2,1,"-c", cmd);
							}
						}
					}
				}
#endif
			}
		}

		//Invalied PD also need add broute rule for phy port, in case bind port will get address from br0 by default PD
		for (j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j){
			if (BIT_IS_SET(entry.itfGroup, j))	{
				printf("[%s:%d] interface:%s is binding to %d \n", __FUNCTION__, __LINE__, SW_LAN_PORT_IF[j], entry.ifIndex);
				//lan phy port global address need not save in ATM_VC_TBL, it will be flush in clean_dhcpd6
				config_ipv6_kernel_flag_and_static_lla_for_inf((char *)SW_LAN_PORT_IF[j]);
				set_lan_ipv6_global_address(NULL, &prefixInfo, (char *)SW_LAN_PORT_IF[j], 0,1);
				add_oif_lla_route_by_lanif((char *)SW_LAN_PORT_IF[j]);
				va_cmd(EBTABLES, 16, 1, "-t", "broute", "-D", PORTMAP_BRTBL, "-i", SW_LAN_PORT_IF[j], "-p", "IPV6", 
						"--ip6-protocol", "udp", "--ip6-sport", "546", "--ip6-dport", "547", "-j", FW_DROP);
				va_cmd(EBTABLES, 16, 1, "-t", "broute", "-I", PORTMAP_BRTBL, "-i", SW_LAN_PORT_IF[j], "-p", "IPV6", 
						"--ip6-protocol", "udp", "--ip6-sport", "546", "--ip6-dport", "547", "-j", FW_DROP);
//#ifdef CONFIG_NETFILTER_XT_TARGET_TEE
				//ip6tables -t mangle -A OUTPUT -o eth0.4.0 -p udp --sport  547 --dport 546  -j TEE --oif br0
				//This rule is necessary for LAN DHCPv6 downstream.
				//Because lan phy port can not learn NS/NA for host mac, but br0 , TEE target will dup DHCPv6 packet and modify oif to research route(to br0)
				//So that, DHCPv6 DS could send out from br0 to host.
				//this rule will be cleaned in delete_binding_lan_service_by_wan();
//				va_cmd(IP6TABLES, 16, 1, "-t","mangle",(char *)FW_ADD, "multi_lan_dhcpv6_ds_TEE", "-o", (char *)SW_LAN_PORT_IF[j], "-p","udp",
//					(char *)FW_SPORT, "547",(char *)FW_DPORT, "546", "-j", "TEE","--oif", (char*)BRIF);
//#endif
			}
		}
#ifdef WLAN_SUPPORT
		for (j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)	{
			if (BIT_IS_SET(entry.itfGroup, j))	{
				if (wlan_en[j-PMAP_WLAN0] == 0)
					continue;
				printf("[%s:%d] interface:%s is binding to %d \n", __FUNCTION__, __LINE__, wlan[j - PMAP_WLAN0], entry.ifIndex );
				config_ipv6_kernel_flag_and_static_lla_for_inf((char *)wlan[j - PMAP_WLAN0]);
				set_lan_ipv6_global_address(NULL, &prefixInfo, (char *)wlan[j - PMAP_WLAN0],0,1);
				add_oif_lla_route_by_lanif((char *)wlan[j-PMAP_WLAN0]);
				va_cmd(EBTABLES, 16, 1, "-t", "broute", "-D", PORTMAP_BRTBL, "-i", wlan[j - PMAP_WLAN0], "-p", "IPV6", 
						"--ip6-protocol", "udp", "--ip6-sport", "546", "--ip6-dport", "547", "-j", FW_DROP);
				va_cmd(EBTABLES, 16, 1, "-t", "broute", "-I", PORTMAP_BRTBL, "-i", wlan[j - PMAP_WLAN0], "-p", "IPV6", 
						"--ip6-protocol", "udp", "--ip6-sport", "546", "--ip6-dport", "547", "-j", FW_DROP);
//#ifdef CONFIG_NETFILTER_XT_TARGET_TEE
				//ip6tables -t mangle -A OUTPUT -o wlan0 -p udp --sport  547 --dport 546  -j TEE --oif br0
//				va_cmd(IP6TABLES, 16, 1, "-t","mangle",(char *)FW_ADD, "multi_lan_dhcpv6_ds_TEE", "-o",  (char *)wlan[j - PMAP_WLAN0], "-p","udp",
//					(char *)FW_SPORT, "547",(char *)FW_DPORT, "546", "-j", "TEE","--oif", (char*)BRIF);
//#endif
			}
		}
#endif
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
		//vlan mapping interface
		struct lan_vlan_inf_msg msg_header;
		memset(&msg_header, 0, sizeof(msg_header));
		msg_header.lan_vlan_inf_event = LAN_VLAN_INTERFACE_DHCPV6S_ADD;
		memcpy(&msg_header.PrefixInfo,&prefixInfo, sizeof(PREFIX_V6_INFO_T));
		lan_vlan_mapping_inf_handle(&msg_header,&entry);
#endif
	}

	return 0;
}
#endif

#if (defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_) ) && defined(_SUPPORT_L2BRIDGING_DHCP_SERVER_)
int setup_dhcpdv6_brname_with_multiple_br(char* brname_str)
{
	int i = 0, total = 0;
	MIB_L2BRIDGE_GROUP_T entry = {0};
	MIB_L2BRIDGE_GROUP_Tp pEntry = &entry;

	total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
	for (i = 0; i < total; i++) {
		if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, (void *)pEntry))
			continue;
		if (pEntry->enable ==0 ) continue;
		if (pEntry->groupnum !=0 ) {
			snprintf(brname_str+strlen(brname_str), sizeof(brname_str), " br%u", pEntry->groupnum);
		}
	}
	return 1;
}

static int setup_dhcpdv6_conf_with_multiple_br()
{
	int i = 0,j=0, total = 0;
	unsigned char dhcpv6_type =0;
	unsigned char prefix_len=0, mFlag=0;
	unsigned char tmpstr[256]={0}, prefix[MAX_V6_IP_LEN]={0},min_addr[MAX_V6_IP_LEN]={0}, max_addr[MAX_V6_IP_LEN]={0};
	unsigned char ip6Addr_min[IP6_ADDR_LEN]={0},ip6Addr_max[IP6_ADDR_LEN]={0};
	PREFIX_V6_INFO_T PrefixInfo={0}, PrefixInfo_br={0};
	MIB_L2BRIDGE_GROUP_T entry = {0};
	MIB_L2BRIDGE_GROUP_Tp pEntry = &entry;
	struct in6_addr ip6Addr={0}, ip6Prefix={0};
	unsigned char tmpBuf[100]={0};
	FILE *fp=NULL;
#ifdef CONFIG_USER_DHCPV6_EUI64_POOL_TYPE
	unsigned char ipv6PoolFormat= IPV6_POOL_FORMAT_STANDARD;
#endif

	printf("[%s:%d] \n",__func__,__LINE__);
	if ((fp = fopen(DHCPDV6_CONF, "a+")) == NULL) {
		printf("Open file %s failed !\n", DHCPDV6_CONF);
		return -1;
	}
	if (!mib_get_s(MIB_DHCPV6S_TYPE, (void *)&dhcpv6_type, sizeof(dhcpv6_type))){
		printf("[%s:%d] Get DHCPV6 server Type failed\n",__func__,__LINE__);
		fclose(fp);
		return 0;
	}
	if ( !mib_get_s( MIB_V6_AUTONOMOUS, (void *)&mFlag, sizeof(mFlag)) )
		printf("Get MIB_V6_AUTONOMOUS error!");

	total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
	if (dhcpv6_type == DHCPV6S_TYPE_STATIC){
		mib_get_s(MIB_DHCPV6S_PREFIX_LENGTH, (void *)&prefix_len, sizeof(prefix_len));
		mib_get_s(MIB_DHCPV6S_RANGE_START, (void *)ip6Addr.s6_addr, sizeof(ip6Addr.s6_addr));
		if (prefix_len<64 || prefix_len > 128) {
			printf("WARNNING! Prefix Length == %d\n", prefix_len);
			prefix_len = 64;
		}
		ip6toPrefix(&ip6Addr, prefix_len, &ip6Prefix);
		for (i = 0; i < total; i++) {
			if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, (void *)pEntry))
				continue;
			if (pEntry->enable ==0 ) continue;
			if (pEntry->groupnum !=0 )
			{
				//Prepare for brX global IP and lan PD
				memcpy(PrefixInfo.prefixIP,&ip6Prefix, IP6_ADDR_LEN);
				PrefixInfo.prefixLen = prefix_len;
				PrefixInfo.prefixIP[7] += (pEntry->groupnum) ;
				inet_ntop(PF_INET6,PrefixInfo.prefixIP, tmpBuf, sizeof(tmpBuf));
				printf("[%s:%d] br%u prefix is %s/%d\n",__FUNCTION__, __LINE__, pEntry->groupnum, tmpBuf, PrefixInfo.prefixLen);
				fprintf(fp, "subnet6 %s/%d {\n", tmpBuf,  PrefixInfo.prefixLen);
#ifdef CONFIG_USER_DHCPV6_EUI64_POOL_TYPE
				mib_get_s(MIB_DHCPV6S_POOL_ADDR_FORMAT, (void *)&ipv6PoolFormat, sizeof(ipv6PoolFormat));
				if(ipv6PoolFormat==IPV6_POOL_FORMAT_EUI64){
					fprintf(fp, "\tuse-eui-64 true;\n");
					if(!mFlag)
						fprintf(fp, "\trange6 %s/%d;\n", tmpBuf, prefix_len);
				}
				else
#endif
				{
					//Merge the prefix & min/max address
					mib_get_s(MIB_DHCPV6S_MIN_ADDRESS, (void *)&tmpstr, sizeof(tmpstr));
					sprintf(tmpBuf,"::%s",tmpstr);
					inet_pton(PF_INET6,tmpBuf, &ip6Addr_min);
					mib_get_s(MIB_DHCPV6S_MAX_ADDRESS, (void *)&tmpstr, sizeof(tmpstr));
					sprintf(tmpBuf,"::%s",tmpstr);
					inet_pton(PF_INET6,tmpBuf, &ip6Addr_max);
					for (j=0; j<8; j++){
						ip6Addr_min[j] = PrefixInfo.prefixIP[j];
						ip6Addr_max[j] = PrefixInfo.prefixIP[j];
					}
					if (dhcpv6_type == DHCPV6S_TYPE_AUTO_DNS_ONLY ) {
						strcpy(min_addr, "::");
						strcpy(max_addr, "::");
					}
					else {
						inet_ntop(PF_INET6, &ip6Addr_min, min_addr, sizeof(min_addr));
						inet_ntop(PF_INET6, &ip6Addr_max, max_addr, sizeof(max_addr));
					}
					printf("range6 %s - %s;\n",min_addr,max_addr);
					if(!mFlag)
						fprintf(fp, "\trange6 %s %s;\n", min_addr,max_addr);
				}
				fprintf(fp, "}\n");
			}
		}
	}
	else
	{
		get_prefixv6_info(&PrefixInfo);
		for (i = 0; i < total; i++) {
			if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, (void *)pEntry))
				continue;
			if (pEntry->enable ==0 ) continue;
			if (pEntry->groupnum !=0 )
			{
				memcpy(&PrefixInfo_br,&PrefixInfo, sizeof(PrefixInfo));
				PrefixInfo_br.prefixLen = prefix_len;
				PrefixInfo_br.prefixIP[7] += (pEntry->groupnum) ;
				if (PrefixInfo_br.prefixLen <64)
					PrefixInfo_br.prefixLen = 64;
				if (PrefixInfo_br.prefixLen && memcmp(PrefixInfo_br.prefixIP, ipv6_unspec, PrefixInfo_br.prefixLen<=128?PrefixInfo_br.prefixLen:128) ) {
					inet_ntop(PF_INET6,PrefixInfo_br.prefixIP, tmpBuf, sizeof(tmpBuf));
					printf("[%s:%d] br%u prefix is %s/%d\n",__FUNCTION__, __LINE__, pEntry->groupnum, tmpBuf, PrefixInfo_br.prefixLen);
#ifdef CONFIG_USER_RTK_RA_DELEGATION
					//get prefix info form /var/rainfo_<ifname>
					//RA delegation prefix format: 
					//RA prefix(64 bits) +  MAC(48bit) + 0(8bits) =120 bits
					if ((dhcpv6_type ==DHCPV6S_TYPE_RA_DELEGATION) && (PrefixInfo_br.prefixLen ==120)){
						fprintf(fp, "subnet6 %s/120 {\n",tmpBuf);
						snprintf(min_addr, INET6_ADDRSTRLEN, "%s10",tmpBuf);
						snprintf(max_addr,INET6_ADDRSTRLEN, "%sff",tmpBuf);
						printf("range6 %s - %s;\n",min_addr,max_addr);
						if(!mFlag)
							fprintf(fp, "\trange6 %s %s;\n", min_addr,max_addr);
						fprintf(fp, "}\n");
					}
					else
#endif
					{
						fprintf(fp, "subnet6 %s/%d {\n", tmpBuf, (PrefixInfo_br.prefixLen<64)?64:PrefixInfo_br.prefixLen);
						//Merge the prefix & min/max address 
						mib_get_s(MIB_DHCPV6S_MIN_ADDRESS, (void *)&tmpstr, sizeof(tmpstr));
						sprintf(tmpBuf,"::%s",tmpstr);
						inet_pton(PF_INET6,tmpBuf, &ip6Addr_min);

						mib_get_s(MIB_DHCPV6S_MAX_ADDRESS, (void *)&tmpstr, sizeof(tmpstr));
						sprintf(tmpBuf,"::%s",tmpstr);
						inet_pton(PF_INET6,tmpBuf, &ip6Addr_max);
						for (j=0; j<8; j++){
							ip6Addr_min[j] = PrefixInfo_br.prefixIP[j];
							ip6Addr_max[j] = PrefixInfo_br.prefixIP[j];
						}
						if (dhcpv6_type == DHCPV6S_TYPE_AUTO_DNS_ONLY ) {
							strcpy(min_addr, "::");
							strcpy(max_addr, "::");
						}
						else {
							inet_ntop(PF_INET6, &ip6Addr_min, min_addr, sizeof(min_addr));
							inet_ntop(PF_INET6, &ip6Addr_max, max_addr, sizeof(max_addr));
						}
						printf("range6 %s - %s;\n",min_addr,max_addr);
						if(!mFlag)
							fprintf(fp, "\trange6 %s %s;\n", min_addr,max_addr);
						fprintf(fp, "}\n");
					}
				}
			}
		}
	}
	fclose(fp);
	return 1;
}
#endif

static int setup_dhcpdv6_conf_with_single_br()
{
	unsigned char  dhcpv6_mode = 0, dhcpv6_type =0;
#ifdef CONFIG_USER_DHCPV6_EUI64_POOL_TYPE
	unsigned char ipv6PoolFormat= IPV6_POOL_FORMAT_STANDARD;
#endif
	unsigned int tmpint=0;
	unsigned char prefix_len=0, mFlag=0, mFlagInterleave = 0;
	unsigned char tmpstr[256]={0}, prefix[MAX_V6_IP_LEN]={0},min_addr[MAX_V6_IP_LEN]={0}, max_addr[MAX_V6_IP_LEN]={0};
	FILE *fp=NULL;
	DNS_V6_INFO_T dnsV6Info={0};
	PREFIX_V6_INFO_T PrefixInfo={0};
	unsigned char domainSearch[MAX_DOMAIN_NUM*MAX_DOMAIN_LENGTH] = {0};
	struct in6_addr ip6Addr, ip6Prefix;
	unsigned char ip6Addr_min[IP6_ADDR_LEN]={0},ip6Addr_max[IP6_ADDR_LEN]={0};
	int idx=0;
#if defined(CONFIG_USER_RTK_DHCPV6_SERVER_PD_SUPPORT) || defined(YUEME_DHCPV6_SERVER_PD_SUPPORT)
	struct ipv6_addr_str_array prefix6_subnet_start = {0}, prefix6_subnet_end = {0};
	int lan_pd_len=0;
#endif
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	unsigned char logoTestMode = 0 ;
#endif

	printf("[%s:%d] \n",__func__,__LINE__);
	if ((fp = fopen(DHCPDV6_CONF, "w")) == NULL) {
		printf("Open file %s failed !\n", DHCPDV6_CONF);
		return -1;
	}

	if (!mib_get_s(MIB_DHCPV6_MODE, (void *)&dhcpv6_mode, sizeof(dhcpv6_mode))){
		printf("[%s:%d] Get DHCPV6 server Mode failed\n",__func__,__LINE__);
		fclose(fp);
		return 0;
	}

	if (!mib_get_s(MIB_DHCPV6S_TYPE, (void *)&dhcpv6_type, sizeof(dhcpv6_type))){
		printf("[%s:%d] Get DHCPV6 server Type failed\n",__func__,__LINE__);
		fclose(fp);
		return 0;
	}
	if ( !mib_get_s( MIB_V6_AUTONOMOUS, (void *)&mFlag, sizeof(mFlag)) )
		printf("Get MIB_V6_AUTONOMOUS error!");
	if ( !mib_get_s(MIB_V6_MFLAGINTERLEAVE, (void *)&mFlagInterleave, sizeof(mFlagInterleave)) )
		printf("Get MIB_V6_MFLAGINTERLEAVE error!");
	//Common conf
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	if (mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode))){
		if(logoTestMode == 3)
			/* CE router lan_rfc3315 case 19 Part A : DUID Format
			 * server duid type need set to ll (default is llt);
			 * if use LLT, must ensure dhclient/dhcpd startup time is same in every reboot
			 */
			fprintf(fp, "server-duid ll;\n");
		}
#endif
	// option dhcp6.client-id 00:01:00:01:00:04:93:e0:00:00:00:00:a2:a2;
	if (mib_get_s(MIB_DHCPV6S_CLIENT_DUID, (void *)tmpstr, sizeof(tmpstr))!=0) {
		if (tmpstr[0]) {
			fprintf(fp, "option dhcp6.client-id %s;\n", tmpstr);
		}
	}
		
	// Option host
	option_mac_binding(fp);
	// Option NTP
	option_ntp_server(fp);
	
	//br0 conf Option DNS/DNSSL/subnet6
	if (dhcpv6_type == DHCPV6S_TYPE_STATIC){
		// default-lease-time
		if (mib_get_s(MIB_DHCPV6S_DEFAULT_LEASE, (void *)&tmpint, sizeof(tmpint)) != 0){
			fprintf(fp, "default-lease-time %u;\n", tmpint);
		}
		// preferred-lifetime
		if (mib_get_s(MIB_DHCPV6S_PREFERRED_LIFETIME, (void *)&tmpint, sizeof(tmpint)) != 0){
			fprintf(fp, "preferred-lifetime %u;\n", tmpint);
		}
		// option dhcp-renewal-time
		if (mib_get_s(MIB_DHCPV6S_RENEW_TIME, (void *)&tmpint, sizeof(tmpint)) != 0){
			fprintf(fp, "option dhcp-renewal-time %u;\n", tmpint);
		}
		// option dhcp-rebinding-time
		if (mib_get_s(MIB_DHCPV6S_REBIND_TIME, (void *)&tmpint, sizeof(tmpint)) != 0){
			fprintf(fp, "option dhcp-rebinding-time %u;\n", tmpint);
		}
		get_dnsv6_info(&dnsV6Info);
		if (dnsV6Info.nameServer[0]){
			fprintf(fp, "option dhcp6.name-servers %s;\n", dnsV6Info.nameServer);
		}
		option_domain_search(fp);
		get_dnsv6_info(&dnsV6Info);
		if (dnsV6Info.nameServer[0]){
			fprintf(fp, "option dhcp6.name-servers %s;\n", dnsV6Info.nameServer);
		}
		fprintf(fp, "db-time-format local;\n");

		// subnet6 3ffe:501:ffff:100::/64 {
		mib_get_s(MIB_DHCPV6S_PREFIX_LENGTH, (void *)&prefix_len, sizeof(prefix_len));
		mib_get_s(MIB_DHCPV6S_RANGE_START, (void *)ip6Addr.s6_addr, sizeof(ip6Addr.s6_addr));
		if (prefix_len<=0 || prefix_len > 128) {
			printf("WARNNING! Prefix Length == %d\n", prefix_len);
			prefix_len = 64;
		}
		ip6toPrefix(&ip6Addr, prefix_len, &ip6Prefix);
		inet_ntop(PF_INET6, &ip6Prefix, prefix, sizeof(prefix));

		//Prepare for br0 global IP and lan PD
		memcpy(PrefixInfo.prefixIP,&ip6Prefix, IP6_ADDR_LEN);
		PrefixInfo.prefixLen = prefix_len;
		fprintf(fp, "subnet6 %s/%d {\n", prefix, prefix_len);
#ifdef CONFIG_USER_DHCPV6_EUI64_POOL_TYPE
		mib_get_s(MIB_DHCPV6S_POOL_ADDR_FORMAT, (void *)&ipv6PoolFormat, sizeof(ipv6PoolFormat));
		if(ipv6PoolFormat==IPV6_POOL_FORMAT_EUI64){ 
			fprintf(fp, "\tuse-eui-64 true;\n");
			if(!mFlag || mFlagInterleave)
				fprintf(fp, "\trange6 %s/%d;\n", prefix, prefix_len);
		}
		else
#endif
		{
			getMIB2Str(MIB_DHCPV6S_RANGE_START, min_addr);
			getMIB2Str(MIB_DHCPV6S_RANGE_END, max_addr);
			if (min_addr[0] && max_addr[0] && (!mFlag || mFlagInterleave)) {
				fprintf(fp, "\trange6 %s %s;\n", min_addr, max_addr);
			}
		}
#if defined(CONFIG_USER_RTK_DHCPV6_SERVER_PD_SUPPORT) || defined(YUEME_DHCPV6_SERVER_PD_SUPPORT)
		if (calculate_prefix6_range(&PrefixInfo, &prefix6_subnet_start, &prefix6_subnet_end, &lan_pd_len) > 0)
		{
			printf("%s: assigned prefix6 %s %s /%d\n", __FUNCTION__, prefix6_subnet_start.ipv6_addr_str, prefix6_subnet_end.ipv6_addr_str,lan_pd_len);
			fprintf(fp, "\tprefix6 %s %s /%d;\n", prefix6_subnet_start.ipv6_addr_str, prefix6_subnet_end.ipv6_addr_str,lan_pd_len);
		}
#endif
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
		/*dhcpv6 static mode still need dhcpv6 packet downstream send from br0, not phy port */
		fprintf(fp, "\twbindintf %s;\n", BRIF);
#endif
		fprintf(fp, "}\n");
	}else{
		//Get Default WAN  DNS info and PD info
		get_dnsv6_info(&dnsV6Info);
		get_prefixv6_info(&PrefixInfo);

		// default-lease-time
		if (PrefixInfo.MLTime)
			fprintf(fp, "default-lease-time %u;\n", PrefixInfo.MLTime);
		// preferred-lifetime
		if (PrefixInfo.PLTime)
			fprintf(fp, "preferred-lifetime %u;\n", PrefixInfo.PLTime);
		// option dhcp-renewal-time
		if (PrefixInfo.RNTime)
			fprintf(fp, "option dhcp-renewal-time %u;\n", PrefixInfo.RNTime);
		// option dhcp-rebinding-time
		if (PrefixInfo.RBTime)
			fprintf(fp, "option dhcp-rebinding-time %u;\n", PrefixInfo.RBTime);

		if (dnsV6Info.nameServer[0]){
			fprintf(fp, "option dhcp6.name-servers %s;\n", dnsV6Info.nameServer);
		}

		/* Lease times are specified in Universal Coordinated Time (UTC), not in the local time zone. There is
		probably nowhere in the world where the times recorded on a lease are always the same as wall clock
		times. Thus, we use "local" here to make cltt and ends time present in seconds-since-epoch as according
		to the system's local clock. Otherwise, the time of leasefile which does not concern time zone would
		lead to expired time error. 20210407 */
		fprintf(fp, "db-time-format local;\n");

		if (dnsV6Info.dnssl_num >0){
			for (idx=0; idx <dnsV6Info.dnssl_num; idx++){
				if(idx == dnsV6Info.dnssl_num-1)
					snprintf(domainSearch+strlen(domainSearch),sizeof(domainSearch), "\"%s\" ", dnsV6Info.domainSearch[idx]);
				else
					snprintf(domainSearch+strlen(domainSearch),sizeof(domainSearch), "\"%s\",", dnsV6Info.domainSearch[idx]);
			}
			printf("%s  : DNSSL have %d entry %s\n",__func__, dnsV6Info.dnssl_num , domainSearch);
			fprintf(fp, "option dhcp6.domain-search %s;\n", domainSearch);
		}

		//subnet6 2001:b010:7030:1a01::/64 {
		//        range6 2001:b010:7030:1a01::/64 or range6 low-address high-address;
		//}	
		if (PrefixInfo.prefixLen && memcmp(PrefixInfo.prefixIP, ipv6_unspec, PrefixInfo.prefixLen<=128?PrefixInfo.prefixLen:128)) {
			struct  in6_addr ip6Addr, ip6Addr2;
			unsigned char minaddr[IP6_ADDR_LEN]={0},maxaddr[IP6_ADDR_LEN]={0};
			unsigned char prefixBuf[100]={0},tmpBuf[100]={0},tmpBuf2[100]={0};
			int i=0;
		
			inet_ntop(PF_INET6,PrefixInfo.prefixIP, prefixBuf, sizeof(prefixBuf));
			printf("prefix is %s/%d\n",prefixBuf, PrefixInfo.prefixLen);
#ifdef CONFIG_USER_RTK_RA_DELEGATION
			//get prefix info form /var/rainfo_<ifname>
			//RA delegation prefix format: 
			//RA prefix(64 bits) +  MAC(48bit) + 0(8bits) =120 bits
			if ((dhcpv6_type ==DHCPV6S_TYPE_RA_DELEGATION) && (PrefixInfo.prefixLen ==120)){
				fprintf(fp, "subnet6 %s/120 {\n",prefixBuf);
				snprintf(min_addr, INET6_ADDRSTRLEN, "%s10",prefixBuf);
				snprintf(max_addr,INET6_ADDRSTRLEN, "%sff",prefixBuf);
				printf("range6 %s - %s;\n",min_addr,max_addr);
				if(!mFlag || mFlagInterleave)
					fprintf(fp, "\trange6 %s %s;\n", min_addr,max_addr);
				fprintf(fp, "}\n");
			}
			else
#endif
			{
				fprintf(fp, "subnet6 %s/%d {\n", prefixBuf, (PrefixInfo.prefixLen<64)?64:PrefixInfo.prefixLen);
				//Merge the prefix & min/max address 
				mib_get_s(MIB_DHCPV6S_MIN_ADDRESS, (void *)&tmpstr, sizeof(tmpstr));
				sprintf(tmpBuf,"::%s",tmpstr);
				inet_pton(PF_INET6,tmpBuf, &ip6Addr_min);

				mib_get_s(MIB_DHCPV6S_MAX_ADDRESS, (void *)&tmpstr, sizeof(tmpstr));
				sprintf(tmpBuf,"::%s",tmpstr);
				inet_pton(PF_INET6,tmpBuf, &ip6Addr_max);
			
				for (i=0; i<8; i++){
					ip6Addr_min[i] = PrefixInfo.prefixIP[i];
					ip6Addr_max[i] = PrefixInfo.prefixIP[i];
				}

				if (dhcpv6_type == DHCPV6S_TYPE_AUTO_DNS_ONLY ) {
					strcpy(min_addr, "::");
					strcpy(max_addr, "::");
				}
				else {
					inet_ntop(PF_INET6, &ip6Addr_min, min_addr, sizeof(min_addr));
					inet_ntop(PF_INET6, &ip6Addr_max, max_addr, sizeof(max_addr));
				}
				printf("range6 %s - %s;\n",min_addr,max_addr);
				if(!mFlag || mFlagInterleave)
					fprintf(fp, "\trange6 %s %s;\n", min_addr,max_addr);
#if defined(CONFIG_USER_RTK_DHCPV6_SERVER_PD_SUPPORT) || defined(YUEME_DHCPV6_SERVER_PD_SUPPORT)
				if (calculate_prefix6_range(&PrefixInfo, &prefix6_subnet_start, &prefix6_subnet_end, &lan_pd_len) > 0)
				{
					printf("%s: dynamic assigned prefix6 %s %s /%d\n", __FUNCTION__, prefix6_subnet_start.ipv6_addr_str, prefix6_subnet_end.ipv6_addr_str, lan_pd_len);
					fprintf(fp, "\tprefix6 %s %s /%d;\n", prefix6_subnet_start.ipv6_addr_str, prefix6_subnet_end.ipv6_addr_str,lan_pd_len);
				}
#endif
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
				/* This is for case that default PD also be used for portmapping */
				fprintf(fp, "\twbindintf %s;\n", BRIF);
#endif
				fprintf(fp, "}\n");
			}
		}
	}

	//Add global address for br0
	if (PrefixInfo.prefixLen && memcmp(PrefixInfo.prefixIP, ipv6_unspec, PrefixInfo.prefixLen<=128?PrefixInfo.prefixLen:128)) {
		set_lan_ipv6_global_address(NULL, &PrefixInfo, (char *)BRIF,1,1);
	}
	fclose(fp);

	//bindindg lan port conf
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
	setup_binding_lan_dhcpv6_conf();
#endif

	/* Create lease file and don't delete lease for known client,
	 * if you don't have lease file, it will reply OK(on-link) after
	 * receiving CONFIRM message(it is tricky).
	 */
	if ((fp = fopen(DHCPDV6_LEASES, "a")) == NULL)
	{
		printf("Open file %s failed !\n", DHCPDV6_LEASES);
		return -1;
	}
	fclose(fp);
	if ((fp = fopen(DHCPDV6_STATIC_LEASES, "a")) == NULL)
	{
		printf("Open file %s failed !\n", DHCPDV6_STATIC_LEASES);
		return -1;
	}
	fclose(fp);
	
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(DHCPDV6_CONF, 0666);
	chmod(DHCPDV6_LEASES, 0666);
	chmod(DHCPDV6_STATIC_LEASES, 0666);
#endif

	return 1;
}

int startDhcpv6Relay()
{	
	char upper_ifname[8];
	unsigned int upper_if;
	PREFIX_V6_INFO_T prefixInfo;
	MIB_CE_ATM_VC_T Entry;
	
	kill_by_pidfile((const char*)DHCPRELAY6PID, SIGTERM);
	if ( !mib_get_s(MIB_DHCPV6R_UPPER_IFINDEX, (void *)&upper_if, sizeof(upper_if))) {
		AUG_PRT("Get MIB_DHCPV6R_UPPER_IFINDEX mib error!");
		return -1;
	}		
	
	if ( upper_if != DUMMY_IFINDEX )
	{
		ifGetName(upper_if, upper_ifname, sizeof(upper_ifname));
	}
	else
	{
		AUG_PRT("Error: The upper interface of dhcrelayV6 not set !\n");
		return -1;
	}
	
	if (!(getATMVCEntryByIfIndex(upper_if, &Entry)))
	{
		AUG_PRT("Error! Could not get MIB entry info from wanconn %s!\n", upper_ifname);
		return -1;
	}
	
	if (get_prefixv6_info_by_wan(&prefixInfo, upper_if) !=0){
		AUG_PRT("Error:get prefix info from wan %d failed\n",upper_if);
		return -1;
	}
	if(set_lan_ipv6_global_address(NULL,&prefixInfo,(char *)LANIF,1,1)!=0){
		AUG_PRT("Error:Set br0 global address failed\n");
		return -1;
	}
	
	// dhcrelayV6 -6 -l br0 -u vc0
	va_niced_cmd(DHCREALYV6, 5, 0, "-6", "-l", BRIF, "-u", upper_ifname);
	return 0;
}

int restart_default_dhcpv6_server(void)
{
	unsigned char value[64];
	unsigned char vChar, ipv6PoolFormat;
	int tmp_status, status=0;
	unsigned int uInt, i;
	unsigned int len=0;
	struct in6_addr ip6Addr, targetIp;
	unsigned char devAddr[MAC_ADDR_LEN];
	unsigned char meui64[8];
	unsigned char dhcpv6_type = 0, dhcpv6_mode=0;
	unsigned char cmd_buf[256]={0};
	unsigned char cmd_lan_inf[100]={0};

	//printf("[%s:%d] \n",__func__,__LINE__);
	
	//Clear old deamon and addr in 
	clean_dhcpd6();

	mib_get_s(MIB_V6_IPV6_ENABLE, (void *)&vChar, sizeof(vChar));
	if (vChar == 0)
		return 0;
	
	if (!mib_get_s(MIB_DHCPV6_MODE, (void *)&dhcpv6_mode, sizeof(dhcpv6_mode))){
		printf("[%s:%d] Get DHCPV6 server Mode failed\n",__func__,__LINE__);
		return 0;
	}

	if (!mib_get_s(MIB_DHCPV6S_TYPE, (void *)&dhcpv6_type, sizeof(dhcpv6_type))){
		printf("[%s:%d] Get DHCPV6 server Type failed\n",__func__,__LINE__);
		return 0;
	}
	
	if (dhcpv6_mode == DHCP_LAN_SERVER)	{
		//Now Setup new dhcp6d.conf and add br0 global IP
		setup_dhcpdv6_conf_with_single_br();
		snprintf(cmd_lan_inf, sizeof(cmd_lan_inf), "%s",(char *)BRIF);
#if (defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_) ) && defined(_SUPPORT_L2BRIDGING_DHCP_SERVER_)
		char br_name[100]={0};
		setup_dhcpdv6_conf_with_multiple_br();
		setup_dhcpdv6_brname_with_multiple_br(br_name);
		snprintf(cmd_lan_inf+strlen(cmd_lan_inf), sizeof(cmd_lan_inf), "%s",(char *)br_name);
#endif
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
		//Find binding lan phy port name
		int total = mib_chain_total(MIB_ATM_VC_TBL);
		int i = 0, j = 0;
		MIB_CE_ATM_VC_T entry;
		char ifname[IFNAMSIZ] = {0};

		for(i = 0 ; i < total ; i++){
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
				continue;
			if (ifGetName(entry.ifIndex, ifname, IFNAMSIZ) == 0)
				continue;
			if (!(entry.IpProtocol & IPVER_IPV6))
				continue;
			if (entry.cmode == CHANNEL_MODE_BRIDGE) //dont send RA to port which bind bridge WAN
				continue;
		
			for (j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j){
				if (BIT_IS_SET(entry.itfGroup, j)){
					snprintf(cmd_lan_inf+strlen(cmd_lan_inf), sizeof(cmd_lan_inf)-strlen(cmd_lan_inf), " %s",(char *)SW_LAN_PORT_IF[j]);
				}
			}
#ifdef WLAN_SUPPORT
			for (j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)	{
				if (BIT_IS_SET(entry.itfGroup, j)){
					if (wlan_en[j-PMAP_WLAN0] == 0)
						continue;
					snprintf(cmd_lan_inf+strlen(cmd_lan_inf), sizeof(cmd_lan_inf)-strlen(cmd_lan_inf), " %s",(char *)wlan[j - PMAP_WLAN0]);
				}			
			}
#endif
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
			//vlan mapping interface
			struct lan_vlan_inf_msg msg_header;
			memset(&msg_header, 0, sizeof(msg_header));
			msg_header.lan_vlan_inf_event = LAN_VLAN_INTERFACE_DHCPV6S_CMD;
			lan_vlan_mapping_inf_handle(&msg_header,&entry);
			snprintf(cmd_lan_inf+strlen(cmd_lan_inf), sizeof(cmd_lan_inf)-strlen(cmd_lan_inf), " %s",msg_header.inf_cmd);
#endif
		}
		
		printf("[%s:%d] cmd_lanif=%s\n",__func__,__LINE__, cmd_lan_inf);
#endif
		//When AUTO mode, should wait until Configuration file is ready then start the DHCPv6 Server.
		//(After connection is ready and getting prefix delegation.)
		if(access(DHCPDV6_CONF, F_OK)==0){
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
			mib_get(MIB_DHCPV6S_POOL_ADDR_FORMAT, (void *)&ipv6PoolFormat);
			if((dhcpv6_type == DHCPV6S_TYPE_STATIC) &&ipv6PoolFormat==IPV6_POOL_FORMAT_EUI64)  {
				snprintf(cmd_buf, sizeof(cmd_buf), "%s -6 --pool6type 1 -cf %s -lf %s %s", (char*)DHCPDV6,(char*)DHCPDV6_CONF, (char*)DHCPDV6_LEASES, cmd_lan_inf);
			}else
#endif
			{
				//dhcpd -6 -cf /var/dhcpd6.conf -lf /var/dhcpd6.leases br0 eth0.x.0(if have binding )
				snprintf(cmd_buf, sizeof(cmd_buf), "%s -6 -cf %s -lf %s %s", (char*)DHCPDV6,(char*)DHCPDV6_CONF, (char*)DHCPDV6_LEASES, cmd_lan_inf);
			}
			va_cmd("/bin/sh", 2, 1, "-c", cmd_buf);
		}
	}
	else if(dhcpv6_mode == DHCP_LAN_RELAY)
	{
	
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
		setup_binding_lan_dhcpv6_conf();//set port/vlan binding info 
#endif
		startDhcpv6Relay();
		//status=(status==-1)?-1:0;
	}
	else
		status = 0;

#ifdef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
	/* Some model have IPv6 port filter, we should update IPv6 port filter's prefix.  */
	PREFIX_V6_INFO_T DLGInfo;
	struct in6_addr static_ip6Addr;
	struct in6_addr ip6Addr_start;
	unsigned char prefixLen = 0;
	struct in6_addr static_ip6Addr_prefix;

	if (dhcpv6_mode == DHCP_LAN_SERVER)
	{
		if ((dhcpv6_type == DHCPV6S_TYPE_AUTO) || (dhcpv6_type == DHCPV6S_TYPE_AUTO_DNS_ONLY))
		{
			get_prefixv6_info(&DLGInfo);
		}
		else if (dhcpv6_type == DHCPV6S_TYPE_STATIC)
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
		updateIPV6FilterByPD(&DLGInfo);
	}

#endif

	return status;
}

void clean_dhcpd6(void)
{
	kill_by_pidfile((const char*)DHCPSERVER6PID, SIGTERM);
	kill_by_pidfile((const char*)DHCPRELAY6PID, SIGTERM);

#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
	int j;
	for (j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j){
		//Clean binding phy port ipv6 global address		
		va_cmd("/usr/bin/ip",7 , 1, "-6", "addr", "flush", "dev", (char*)SW_LAN_PORT_IF[j], "scope", "global");		

		//Clean binding phy port policy route
		del_bind_lan_oif_policy_route((char *)SW_LAN_PORT_IF[j]);

		//Clean binding phy port ebtables/ip6tables rule
		//DHCPv6 Relay daemon still use br0 (not phy port) to recv upstream dhcpv6 packet, BTW NS still learn @ br0
		//So,we need clean broute rule in clean flow
		va_cmd(EBTABLES, 16, 1, "-t", "broute", "-D", PORTMAP_BRTBL, "-i", SW_LAN_PORT_IF[j], "-p", "IPV6", 
				"--ip6-protocol", "udp", "--ip6-sport", "546", "--ip6-dport", "547", "-j", FW_DROP);
	}
#ifdef WLAN_SUPPORT
	for (j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j){
		if (wlan_en[j-PMAP_WLAN0] == 0)
			continue;
		va_cmd("/usr/bin/ip",7 , 1, "-6", "addr", "flush", "dev",  (char*)wlan[j - PMAP_WLAN0], "scope", "global");
		del_bind_lan_oif_policy_route((char *)wlan[j-PMAP_WLAN0]);
		va_cmd(EBTABLES, 16, 1, "-t", "broute", "-D", PORTMAP_BRTBL, "-i", (char*)wlan[j - PMAP_WLAN0], "-p", "IPV6", 
				"--ip6-protocol", "udp", "--ip6-sport", "546", "--ip6-dport", "547", "-j", FW_DROP);
	}
#endif
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
	//vlan mapping interface
	struct lan_vlan_inf_msg msg_header;
	int total = mib_chain_total(MIB_ATM_VC_TBL);
	int i = 0;
	MIB_CE_ATM_VC_T entry;

	memset(&msg_header, 0, sizeof(msg_header));
	msg_header.lan_vlan_inf_event = LAN_VLAN_INTERFACE_DEL;
	
	for(i = 0 ; i < total ; i++){
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
			continue;
		lan_vlan_mapping_inf_handle(&msg_header,&entry);
	}
	//in case, vlan mapping lan interface(MIB_PORT_BINDING_TBL) have been deleted, the old rule will be change to
	//15000:	from all oif eth0.2.700 [detached] lookup 256
	//and we need clear all the "detached" rule
	del_vlan_bind_lan_detached_policy_route();
#endif
//#ifdef CONFIG_NETFILTER_XT_TARGET_TEE
	//va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "multi_lan_dhcpv6_ds_TEE");
	//va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D","OUTPUT","-j", "multi_lan_dhcpv6_ds_TEE");	
	//va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", "multi_lan_dhcpv6_ds_TEE");
//#endif
#endif
}

// table to mapping dhcpv6 option and dhclient config
DHCPv6_OPT_T config_tbl[]={
{D6O_CLIENTID, "client-id"},
{D6O_SERVERID, "server-id"},
{D6O_IA_NA, "ia-na"},
{D6O_IA_TA, "ia-ta"},
{D6O_IAADDR, "name-servers"},
{D6O_ORO, "oro"},
{D6O_PREFERENCE, "preference"},
{D6O_ELAPSED_TIME, "elapsed-time"},
{D6O_RELAY_MSG, "relay-msg"},
/* Option code 10 unassigned. */
//{D6O_AUTH, "dhcp6.name-servers"},
{D6O_UNICAST, "unicast"},
{D6O_STATUS_CODE, "status-code"},
{D6O_RAPID_COMMIT, "rapid-commit"},
//{D6O_USER_CLASS, ""},
/* option 16, we use option_vendor_class_identify(). */
//{D6O_VENDOR_CLASS, ""},
{D6O_VENDOR_OPTS, "vendor-opts"},
{D6O_INTERFACE_ID, "interface-id"},
{D6O_RECONF_MSG, "reconf-msg"},
{D6O_RECONF_ACCEPT, "reconf-accept"},
{D6O_SIP_SERVERS_DNS, "sip-servers-names"},
{D6O_SIP_SERVERS_ADDR, "sip-servers-addresses"},
{D6O_NAME_SERVERS, "name-servers"},
{D6O_DOMAIN_SEARCH, "domain-search"},
{D6O_IA_PD, "ia-pd"},
{D6O_IAPREFIX, "ia-prefix"},
{D6O_NIS_SERVERS, "nis-servers"},
{D6O_NISP_SERVERS, "nisp-servers"},
{D6O_NIS_DOMAIN_NAME, "nis-domain-name"},
{D6O_NISP_DOMAIN_NAME, "nisp-domain-name"},
{D6O_SNTP_SERVERS, "sntp-servers"},
{D6O_INFORMATION_REFRESH_TIME, "info-refresh-time"},
{D6O_BCMCS_SERVER_D, "bcms-server-d"},
{D6O_BCMCS_SERVER_A, "bcms-server-a"},
/* 35 is unassigned */
//{D6O_GEOCONF_CIVIC, ""},
{D6O_REMOTE_ID, "remote-id"},
{D6O_SUBSCRIBER_ID, "subscriber-id"},
{D6O_CLIENT_FQDN, "fqdn"},
//{D6O_PANA_AGENT, ""},
//{D6O_NEW_POSIX_TIMEZONE, ""},
//{D6O_NEW_TZDB_TIMEZONE, ""},
//{D6O_ERO, ""},
{D6O_LQ_QUERY, "lq-query"},
{D6O_CLIENT_DATA, "client-data"},
{D6O_CLT_TIME, "clt-time"},
{D6O_LQ_RELAY_DATA, "lq-relay-data"},
{D6O_LQ_CLIENT_LINK, "lq-client-link"},
//{D6O_MIP6_HNIDF, ""},
//{D6O_MIP6_VDINF, ""},
//{D6O_V6_LOST, ""},
//{D6O_CAPWAP_AC_V6, ""},
//{D6O_RELAY_ID, ""},
//{D6O_IPV6_ADDRESS_MOS, ""},
//{D6O_IPV6_FQDN_MOS, ""},
//{D6O_NTP_SERVER, ""},
//{D6O_V6_ACCESS_DOMAIN, ""},
//{D6O_SIP_UA_CS_LIST, ""},
{D6O_BOOTFILE_URL, "bootfile-url"},
//{D6O_BOOTFILE_PARAM, ""},
{D6O_CLIENT_ARCH_TYPE, "arch-type"},
//{D6O_NII, ""},
//{D6O_GEOLOCATION, ""},
//{D6O_GEOCONF_CIVIC, ""},
{D6O_AFTR_NAME, "dslite"},	// user define option
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
{D6O_SOL_MAX_RT, "solmax-rt"},
#endif
{D6O_S46_CONT_MAPE, "MAP-E"},
{D6O_S46_CONT_MAPT, "MAP-T"},
{D6O_S46_CONT_LW, "lw4o6"},
/*
{D6O_ERP_LOCAL_DOMAIN_NAME, ""},
{D6O_RSOO, ""},
{D6O_PD_EXCLUDE, ""},
{D6O_VSS, ""},
{D6O_MIP6_IDINF, ""},
{D6O_MIP6_UDINF, ""},
{D6O_MIP6_HNP, ""},
{D6O_MIP6_HAA, ""},
{D6O_MIP6_HAF, ""},
{D6O_RDNSS_SELECTION, ""},
{D6O_KRB_PRINCIPAL_NAME, ""},
{D6O_KRB_REALM_NAME, ""},
{D6O_KRB_DEFAULT_REALM_NAME, ""},
{D6O_KRB_KDC, ""},
{D6O_CLIENT_LINKLAYER_ADDR, ""},
{D6O_LINK_ADDRESS, ""},
{D6O_RADIUS, ""},
{D6O_SOL_MAX_RT, ""},
{D6O_INF_MAX_RT, ""},
{D6O_ADDRSEL, ""},
{D6O_ADDRSEL_TABLE, ""},
{D6O_V6_PCP_SERVER, ""},
{D6O_DHCPV4_MSG, ""},
{D6O_DHCP4_O_DHCP6_SERVER, ""},
{D6O_RELAY_SOURCE_PORT, ""},
*/
//{0, NULL}
};

#define ORO_ENTRY_NUM (sizeof(config_tbl)/sizeof(DHCPv6_OPT_T))
int getDHCPv6ConfigTableSize(void)
{
	return ORO_ENTRY_NUM;
}

DHCPv6_OPT_T* getDHCPv6ConfigbyOptID(const unsigned int opt_id){
	int i=0;
	for(i=0; i<ORO_ENTRY_NUM;i++){
		if(opt_id == config_tbl[i].op_code){
			return &config_tbl[i];
		}
	}

	return NULL;
}

DHCPv6_OPT_T* getDHCPv6ConfigbyOptName(const char *op_name){
	int i=0;
	for(i=0; i<ORO_ENTRY_NUM;i++){
		if(!strcmp(op_name, config_tbl[i].conf_name)){
			return &config_tbl[i];
		}
	}

	return NULL;
}

int getLeasesInfoByOptName(const char *fname, const int opt_no, char *buff, unsigned int buff_len)
{
	FILE *fp;
	char temps[0x100];
	char *str, *endStr;
	unsigned int offset, RNTime, RBTime, PLTime, MLTime;
	struct in6_addr addr6;
	int in_lease6=0, in_iana=0, in_iapd=0, in_iaaddr=0, in_iaPrefix=0;
	DLG_INFO_T tempInfo;
	int ifIndex;
	MIB_CE_ATM_VC_T vcEntry;
	int found_pd=0, ret=0;
	DHCPv6_OPT_T *target_opt=getDHCPv6ConfigbyOptID(opt_no);
	unsigned int is_IAXX=0;
	char buf[INET6_ADDRSTRLEN] = {0};

	if(!buff || buff_len == 0 || !target_opt)
		return 0;

	memset(buff, 0x0, buff_len);
	if(opt_no == D6O_IA_NA || opt_no == D6O_IA_PD){
		is_IAXX = 1;
	}

	if ((fp = fopen(fname, "r")) == NULL)
	{
		printf("Open file %s fail !\n", fname);
		return 0;
	}

	while (fgets(temps,0x100,fp))
	{
		if (temps[strlen(temps)-1]=='\n')
			temps[strlen(temps)-1] = 0;

		if (str=strstr(temps, "lease6")) {
			// new lease6 declaration, reset tempInfo
			memset(&tempInfo, 0, sizeof(DLG_INFO_T));
			in_lease6 = 1;
			found_pd = 0;
			fgets(temps,0x100,fp);
		}
		
		if (!in_lease6)
			continue;

		if ( str=strstr(temps, target_opt->conf_name)) {
			if(!is_IAXX){
				/* option dhcp6.<opt name> xxxxxxxxx;
				 * Ex: option dhcp6.client-id 0:1:0:1:c7:92:bc:a0:0:e0:4c:86:70:b;
				 */
				offset = strlen(target_opt->conf_name)+1;
				if ( endStr=strchr(str+offset, ';')) {
					*endStr=0;
					memcpy(buff, (unsigned char *)(str+offset),  buff_len);
					ret=1;
				}
			}else{
				// IA_NA or IA_PD
				fgets(temps,0x100,fp);
				// Get renew
				fgets(temps,0x100,fp);
				if ( str=strstr(temps, "renew") ) {
					offset = strlen("renew")+1;
					if ( endStr=strchr(str+offset, ';')) {
						*endStr=0;
						sscanf(str+offset, "%u", &RNTime);
						tempInfo.RNTime = RNTime;
					}
				}

				// Get rebind
				fgets(temps,0x100,fp);
				if ( str=strstr(temps, "rebind")) {
					offset = strlen("rebind")+1;
					if ( endStr=strchr(str+offset, ';')) {
						*endStr=0;
						sscanf(str+offset, "%u", &RBTime);
						tempInfo.RBTime = RBTime;
					}
				}
				fgets(temps,0x100,fp);

				char sub_conf_name[16]={0};
				if(opt_no == D6O_IA_NA)
					snprintf(sub_conf_name, sizeof(sub_conf_name), "iaaddr");
				else if(opt_no == D6O_IA_PD)
					snprintf(sub_conf_name, sizeof(sub_conf_name), "iaprefix");
				while(str=strstr(temps, sub_conf_name))
				{
					// Get prefix
					if ( str=strstr(temps, sub_conf_name)) {
						offset = strlen(sub_conf_name)+1;
						if ( endStr=strchr(str+offset, ' ')) {
							*endStr=0;
							if(opt_no == D6O_IA_NA){
								inet_pton(PF_INET6, (str+offset), &addr6);
								memcpy(tempInfo.prefixIP, &addr6, IP6_ADDR_LEN);
							}else if(opt_no == D6O_IA_PD){
								endStr=strchr(str+offset, '/');
								*endStr=0;

								// PrefixIP
								inet_pton(PF_INET6, (str+offset), &addr6);
								memcpy(tempInfo.prefixIP, &addr6, IP6_ADDR_LEN);

								// Prefix Length
								//sscanf((endStr+1), "%d", &(Info.prefixLen));
								tempInfo.prefixLen = (char)atoi((endStr+1));
							}
						}
					}

					//skip starts
					fgets(temps,0x100,fp);

					// Get preferred-life
					fgets(temps,0x100,fp);
					if ( str=strstr(temps, "preferred-life")) {
						offset = strlen("preferred-life")+1;
						if ( endStr=strchr(str+offset, ';') ) {
							*endStr=0;
							sscanf(str+offset, "%u", &PLTime);
							tempInfo.PLTime = PLTime;
						}
					}

					// Get max-life
					fgets(temps,0x100,fp);
					if( str=strstr(temps, "max-life")) {
						offset = strlen("max-life")+1;
						if ( endStr=strchr(str+offset, ';')) {
							*endStr=0;
							sscanf(str+offset, "%u", &MLTime);
							tempInfo.MLTime = MLTime;
						}
					}

					memcpy((DLG_INFO_T*)buff, &tempInfo, sizeof(tempInfo));
					ret=1;

					/* jump to next iaprefix or option.
					 * because iaprefix format is
					 *
					 * iaprefix 2001:db8:0:200::/64 {
					 * starts 1558080573;
					 * preferred-life 7200;
					 * max-life 7500;
					 * }
					 */
					fgets(temps,0x100,fp);
					fgets(temps,0x100,fp);
				}
			}
		}

		if (strchr(temps, '{')) { // bypass all others declarations
			bypassBlock(fp);
		}
	}

	fclose(fp);

	return ret;
}

#ifdef _PRMT_X_CT_COM_DHCP_
static inline void to_hex(unsigned char *buf, unsigned char *value, size_t size)
{
	int i;

	for(i = 0 ; i < size ; i++)
	{
		if(i != 0)
		{
			buf[0] = ':';
			buf++;
		}
		sprintf(buf, "%02x", value[i]);
		buf += 2;
	}
}


int option_vendor_class_identify(FILE *fp, MIB_CE_ATM_VC_Tp pEntry)
{
	unsigned int i;

	/**
	 * opt16 is an encoded hex string, format: "aa:bb:cc:dd....."
	 * Start with CTCOM enterprise number 0 (4 bytes in DHCPv6) temprarily.
	 * Value 0 is specified by spec.
	 * max size is 253 * 3 bytes.
	 */
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	char opt16[1024] = "00:00:00:02:";
#else
	char opt16[1024] = "00:00:00:00:";
#endif
#ifdef SUPPORT_NON_SESSION_IPOE
	char opt17[1024] = "00:00:00:00:";
	unsigned char opt17_len=0;
#endif

	char opt16_data[512] = {0};	// option16 binary data
	char *cur = opt16_data;
	unsigned short total_len = 0;

	int idx, total = mib_chain_total(MIB_ATM_VC_TBL);
	MIB_CE_ATM_VC_T entry;

	if(pEntry == NULL)
	{
		fprintf(stderr, "<%s:%d> pEntry is NULL\n", __func__, __LINE__);
		return -1;
	}

	for(idx = 0 ; idx < total ; idx++)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, idx, &entry) == 0)
			continue;

		if(entry.ifIndex == pEntry->ifIndex)
			break;
	}

	if(idx >= total)
	{
		fprintf(stderr, "option_vendor_class_identify: 0x%x is not a WAN interface. Do not append CT-COM option 16", pEntry->ifIndex);
		return -1;
	}

	//define option 16
	fprintf(fp, "option dhcp6.vendor-class code 16 = string;\n");
#ifdef SUPPORT_NON_SESSION_IPOE
	for (i = 0; i < 4; i++)
	{
		if(pEntry->dhcpv6_opt17_enable[i])
		{
			if(pEntry->dhcpv6_opt17_type[i] == 3)//sharekey
			{
				if((opt17_len = get_dhcp_reply_auth_opt_serverid(opt16_data, sizeof(opt16_data), pEntry->dhcpv6_opt17_ServerID[i], pEntry->dhcpv6_opt17_SharedKey[i]))>0)
				{
					sprintf(opt17+strlen(opt17), "%02x:", opt17_len);
					to_hex(opt17+strlen(opt17), opt16_data, opt17_len);
				}
			}
		}
	}
#endif

	// create binary option 16 data
	for (i = 0; i < 4; i++)
	{
		if(pEntry->dhcpv6_opt16_enable[i])
		{
			unsigned char field_len = 0;

			if(pEntry->dhcpv6_opt16_type[i] == 34 && pEntry->dhcpv6_opt16_value_mode[i] != 2)
				continue;

			if(pEntry->dhcpv6_opt16_type[i] == 32 && pEntry->dhcpv6_opt16_value_mode[i] != 3)
				continue;

#ifdef SUPPORT_CLOUD_VR_SERVICE
			if(pEntry->dhcp_opt60_type[i] == 35 && pEntry->dhcp_opt60_value_mode[i] != 2)
				continue;
#endif

			if(pEntry->dhcpv6_opt16_value_mode[i] == 0)
			{
				field_len = (unsigned char) strlen(pEntry->dhcpv6_opt16_value[i]);

				if(field_len > 0)
				{
					// (2) 1 bytes for Field type
					memcpy(cur, &pEntry->dhcpv6_opt16_type[i], 1);
					total_len +=1;
					cur +=1;

					// (3) 1 bytes for Field Length
					memcpy(cur, &field_len, 1);
					total_len +=1;
					cur +=1;

					// (4) n bytes for Field Value
					memcpy(cur, pEntry->dhcpv6_opt16_value[i], field_len);
					total_len += field_len;
					cur += field_len;
				}
			}
			else if((pEntry->dhcpv6_opt16_type[i] == 34 && pEntry->dhcpv6_opt16_value_mode[i] == 2) ||
				(pEntry->dhcpv6_opt16_type[i] == 32 && pEntry->dhcpv6_opt16_value_mode[i] == 3) ||
				(pEntry->dhcpv6_opt16_type[i] == 35 && pEntry->dhcpv6_opt16_value_mode[i] == 2) ||
				(pEntry->dhcpv6_opt16_type[i] != 34 && pEntry->dhcpv6_opt16_type[i] != 32 && pEntry->dhcpv6_opt16_type[i] != 35 && pEntry->dhcpv6_opt16_value_mode[i] == 1))
			{
				char buf[100] = {0};
				int ret;

				ret = gen_ctcom_dhcp_opt(pEntry->dhcpv6_opt16_type[i], buf, 100, NULL, NULL);

				if(ret > 0)
				{
					field_len = ret;
					// (2) 1 bytes for Field type
					memcpy(cur, &pEntry->dhcpv6_opt16_type[i], 1);
					total_len +=1;
					cur +=1;

					// (3) 1 bytes for Field Length
					memcpy(cur, &field_len, 1);
					total_len +=1;
					cur +=1;

					// (4) n bytes for Field Value
					memcpy(cur, buf, field_len);
					total_len += field_len;
					cur += field_len;

					memcpy(pEntry->dhcpv6_opt16_value[i], buf, 100);
					mib_chain_update(MIB_ATM_VC_TBL, pEntry, idx);
				}
			}
		}
	}

	if(total_len > 0)
	{		
		unsigned char buf[1024] = {0};
		unsigned short total_len_network = htons(total_len);
		char *tmp = (char *)&total_len_network;

		// append data
		sprintf(buf, "%02x:%02x:", tmp[0], tmp[1]);
		strcat(opt16, buf);
		printf("total data len=%d\n", total_len);
		to_hex(buf, opt16_data, total_len);
		strcat(opt16, buf);
		printf("DHCPv6 opt16=%s\n", opt16);
	}
	if(total_len > 0 
#ifdef SUPPORT_NON_SESSION_IPOE
		|| opt17_len>0
#endif
		)
	{
		char inf[IFNAMSIZ] = {0};
		// Get interface nname
		if (pEntry->cmode == CHANNEL_MODE_PPPOE || pEntry->cmode == CHANNEL_MODE_PPPOA)
		{
			snprintf(inf, 6, "ppp%u", PPP_INDEX(pEntry->ifIndex));
		}
		else{
			ifGetName( PHY_INTF(pEntry->ifIndex), inf, sizeof(inf));
		}
		fprintf(fp, "interface \"%s\" {\n",inf);
		if(total_len > 0 )//send option 16			
			fprintf(fp,"	send dhcp6.vendor-class %s;\n",opt16);
#ifdef SUPPORT_NON_SESSION_IPOE
		if(opt17_len>0)
			fprintf(fp,"	send dhcp6.vendor-opts %s;\n",opt17);
#endif
		fprintf(fp,"}\n\n");
	}

	return 0;
}
#endif	//_PRMT_X_CT_COM_DHCP_

void make_dhcpcv6_conf(MIB_CE_ATM_VC_Tp pEntry)
{
	struct sockaddr hwaddr;
	FILE *conf_fp;
	char wanif[16], wandev[16];
	char dhcpc_cf_buf[64];
	DHCPv6_OPT_T *target_opt=NULL;
#ifdef CONFIG_CWMP_TR181_SUPPORT
	char oro_buf[64]={0}, *pch=NULL;
	int i=0;
#endif
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	unsigned char logoTestMode = 0 ;
#endif
	
	ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));
	snprintf(dhcpc_cf_buf, 64,"%s.%s", PATH_DHCLIENT6_CONF, wanif);
	if ((conf_fp = fopen(dhcpc_cf_buf, "w")) == NULL)
	{
		printf("Open file %s failed !\n", dhcpc_cf_buf);
		return;
	}
	
	fprintf(conf_fp, "#\n# dhclient Configuration\n#\n\n");
	fprintf(conf_fp, "script \"%s\";\n\n", DHCPCV6SCRIPT);

	fprintf(conf_fp, "request;\n"); //force clean OPTION_ORO

	if(pEntry->dnsv6Mode){
		if(NULL != (target_opt=getDHCPv6ConfigbyOptID(D6O_NAME_SERVERS)))
			fprintf(conf_fp, "also request dhcp6.%s;\n", target_opt->conf_name);
		if(NULL != (target_opt=getDHCPv6ConfigbyOptID(D6O_DOMAIN_SEARCH)))
			fprintf(conf_fp, "also request dhcp6.%s;\n", target_opt->conf_name);
	}
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	if( !mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode)))
		printf("Get V6_LOGO_TEST_MODE mib error!");

	if(logoTestMode == 3){ //CE_ROUTER mode
		if(NULL != (target_opt=getDHCPv6ConfigbyOptID(D6O_SOL_MAX_RT))){
			fprintf(conf_fp, "also request dhcp6.%s;\n", target_opt->conf_name);
		}
	}
#endif
#if defined (_PRMT_X_CT_COM_DHCP_) && defined (CONFIG_E8B)
	//send option 16, now only for e8 customization
	option_vendor_class_identify(conf_fp, pEntry);
#endif

#ifdef CONFIG_CWMP_TR181_SUPPORT
	if(pEntry->Ipv6DhcpRequest & M_DHCPv6_REQ_RC){
		if(NULL != (target_opt=getDHCPv6ConfigbyOptID(D6O_RAPID_COMMIT)))
			fprintf(conf_fp, "also request dhcp6.%s;\n", target_opt->conf_name);
	}

	strncpy(oro_buf, pEntry->requestedOptions, strlen(pEntry->requestedOptions)+1);
	pch=strtok(oro_buf, ",");
	while (pch != NULL){
		if(NULL != (target_opt=getDHCPv6ConfigbyOptID(atoi(pch)))){
			// user define option should declare data type????
			if(target_opt->op_code == D6O_AFTR_NAME){
				fprintf(conf_fp, "option dhcp6.%s code %d = domain-list;\n", target_opt->conf_name, target_opt->op_code);
			}
			fprintf(conf_fp, "also request dhcp6.%s;\n", target_opt->conf_name);
		}
		pch=strtok(NULL, ",");
	}
#endif

#if defined(DUAL_STACK_LITE)
	if((pEntry->dslite_enable == 1) && (pEntry->dslite_aftr_mode == IPV6_DSLITE_MODE_AUTO)){
		if(NULL != (target_opt=getDHCPv6ConfigbyOptID(D6O_AFTR_NAME))){
			fprintf(conf_fp, "option dhcp6.%s code %d = domain-list;\n", target_opt->conf_name, target_opt->op_code);
			fprintf(conf_fp, "also request dhcp6.%s;\n", target_opt->conf_name);
		}
	}
#endif
#ifdef CONFIG_USER_MAP_E
	//MAP-E
	if((pEntry->mape_enable == 1) && (pEntry->mape_mode == IPV6_MAPE_MODE_DHCP)){
		if(NULL != (target_opt=getDHCPv6ConfigbyOptID(D6O_S46_CONT_MAPE))){
			fprintf(conf_fp, "also request dhcp6.%s;\n", target_opt->conf_name);
		}
	}
#endif
#ifdef CONFIG_USER_MAP_T
	//MAP-T
	if((pEntry->mapt_enable == 1) && (pEntry->mapt_mode == IPV6_MAPT_MODE_DHCP)){
		if(NULL != (target_opt=getDHCPv6ConfigbyOptID(D6O_S46_CONT_MAPT))){
			fprintf(conf_fp, "also request dhcp6.%s;\n", target_opt->conf_name);
		}
	}
#endif
#ifdef CONFIG_USER_LW4O6
	//lw4o6
	if((pEntry->lw4o6_enable == 1) && (pEntry->lw4o6_mode == IPV6_LW4O6_MODE_AUTO)){
		if(NULL != (target_opt=getDHCPv6ConfigbyOptID(D6O_S46_CONT_LW))){
			fprintf(conf_fp, "also request dhcp6.%s;\n", target_opt->conf_name);
		}
	}
#endif
#ifdef CONFIG_USER_DHCPV6_RECONF_MSG
	// send reconf-accept option in solicit
	if(NULL != (target_opt=getDHCPv6ConfigbyOptID(D6O_RECONF_ACCEPT))){
		fprintf(conf_fp, "send dhcp6.%s;\n", target_opt->conf_name);
	}
#endif

	fclose(conf_fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(dhcpc_cf_buf, 0666);
#endif

}

/* script to wait for IPv6 DAD to complete */
#define SCRIPT_WAIT_DAD "\
ip=/usr/bin/ip\n\
dad_wait_time=3\n\n\
${ip} addr show ${interface} | grep inet6 | grep tentative \\\n\
    &> /dev/null \n\
if [ $? -eq 0 ]; then \n\
    # Wait for duplicate address detection to complete or for \n\
    # the timeout specified as --dad-wait-time. \n\
    for i in $(seq 0 $dad_wait_time) \n\
    do \n\
        # We're going to poll for the tentative flag every second. \n\
        sleep 1 \n\
        ${ip} addr show ${interface} | grep inet6 | grep tentative \\\n\
            &> /dev/null \n\
        if [ $? -ne 0 ]; then \n\
            break; \n\
        fi \n\
    done \n\
fi"

/*
 * Add DHCPCv6 for the specified IPv6 interface.
 */
int do_dhcpc6(MIB_CE_ATM_VC_Tp pEntry, int mode)
{
	char *argv[32];
	char ifname[IFNAMSIZ];
	unsigned char pidfile[64], leasefile[64], cffile[64];
	unsigned char duid_mode=(DHCPV6_DUID_TYPE_T)DHCPV6_DUID_LLT;
	int idx = 1;
	char DHCPC6_SCRIPT[]="/var/run/dhcpc6.sh";
	FILE *scriptfp;
	
	mib_get_s(MIB_DHCPV6_CLIENT_DUID_MODE, (void *)&duid_mode, sizeof(duid_mode));
	// dhclient -6 ppp0 -d -q -lf /var/config/dhcpcV6.leases.ppp0 -pf /var/run/dhcpcV6.pid.ppp0 -cf /var/dhcpcV6.conf.ppp0 -N -P
	ifGetName(pEntry->ifIndex,ifname,sizeof(ifname));
	snprintf(leasefile, sizeof(leasefile), "%s.%s", PATH_DHCLIENT6_LEASE, ifname);
	snprintf(pidfile, sizeof(pidfile), "%s.%s", PATH_DHCLIENT6_PID, ifname);
	snprintf(cffile, sizeof(cffile), "%s.%s", PATH_DHCLIENT6_CONF, ifname);
	argv[idx++] = "-6";
	argv[idx++] = ifname;
	argv[idx++] = "-d";
	argv[idx++] = "-q";
	argv[idx++] = "-lf";
	argv[idx++] = leasefile;
	argv[idx++] = "-pf";
	argv[idx++] = pidfile;
	argv[idx++] = "-cf";
	argv[idx++] = cffile;
	// for DHCP441 or later
	// Now DHCP411 already support --dad-wait-time
	argv[idx++] = "--dad-wait-time";
	argv[idx++] = "5";
	argv[idx++] = "-D";
	if (duid_mode == DHCPV6_DUID_LL)
		argv[idx++] = "LL";
	else
		argv[idx++] = "LLT";
	if (mode == INFO_REQUEST_DHCP_MODE) { /* This mode doesn't have any MIB to know, so all IPv6 mode should bring "int mode" into this function.  */
		argv[idx++] = "-S";
	}
	else {
#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
		if (pEntry->AddrMode == IPV6_WAN_AUTO_DETECT_MODE) {
			if (mode == STATEFUL_DHCP_MODE) {
				/* Request Address */
				// for DHCP441 or later
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP441
				/* DHCPv6 IA_NA is a host address and does not include any on-link information. */
#endif
				argv[idx++] = "-N";
			}
		}
		else
#endif
		{ /* User-Defined: Stateless DHCPv6(Our Web SLAAC mode) or Stateful DHCPv6 */
			// Request Address
			if (pEntry->Ipv6DhcpRequest & M_DHCPv6_REQ_IANA) {
				// for DHCP441 or later
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP441
				/* DHCPv6 IA_NA is a host address and does not include any on-link information. */
#endif
				argv[idx++] = "-N";
			}
		}
		/* Request Prefix; todo -- IAPD acquisition should be configurable */
		if ((pEntry->Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)) {
			argv[idx++] = "-P";
		}
	}
	argv[idx] = NULL;

	kill_by_pidfile(pidfile, SIGTERM);
	/* Run script since we don't want to block web process.
	 In dhclient staless mode(-S), it is not run as daemon so we have to wait
	 for link-local DAD process before running it. */
	if ((scriptfp=fopen(DHCPC6_SCRIPT, "w")) == NULL)
		return 0;
	fprintf(scriptfp, "#!/bin/sh\n\n");
	/* If you want  boa  do startIP_for_V6, It usually cause some IPv6 daemon config or lease file failed.
	 * The workaround way is 'fprintf(scriptfp,"sleep 1\n")'  20201016 */
	if (mode == INFO_REQUEST_DHCP_MODE)
	{
		fprintf(scriptfp, "interface=%s\n", ifname);
		fprintf(scriptfp, "%s\n\n", SCRIPT_WAIT_DAD);
	}

	fprintf(scriptfp, "#Due to some server not reply rebind packet.\n#So remove old lease to prevent dhclient send rebind.\n");
	fprintf(scriptfp, "if [ -f %s ]; then\n", leasefile);
	fprintf(scriptfp, "\t%s -6 -r -1 -lf %s -pf %s %s -N -P", DHCPCV6, leasefile, pidfile, ifname);
	if (duid_mode == DHCPV6_DUID_LL)
		fprintf(scriptfp, " -D LL\n");
	else
		fprintf(scriptfp, " -D LLT\n");
	fprintf(scriptfp, "fi\n\n", leasefile);

	fprintf(scriptfp, "%s", DHCPCV6);
	idx = 1;
	while (argv[idx] != NULL) {
		fprintf(scriptfp, " %s", argv[idx]);
		idx++;
	}
	fprintf(scriptfp, "&\n");
	fclose(scriptfp);
	if(chmod(DHCPC6_SCRIPT, 484) != 0)
		printf("chmod failed: %s %d\n", __func__, __LINE__);
	/* run script to launch dhcpc client */
	va_niced_cmd(DHCPC6_SCRIPT, 0, 0);

	return 1;
}

/*
 * Delete DHCPCv6 for the specified IPv6 interface.
 * Kill dhclient, remove lease file, pid file and cf file.
 */
int del_dhcpc6(MIB_CE_ATM_VC_Tp pEntry)
{
	char ifname[IFNAMSIZ] = {0};
	unsigned char pidfile[64] = {0}, cffile[64] = {0};
	int dhcpcpid = -1;
	unsigned char leasefile[64] = {0};
	unsigned char duid_mode=(DHCPV6_DUID_TYPE_T)DHCPV6_DUID_LLT;

	// Stop DHCPv6 client
#ifdef CONFIG_USER_RTK_RAMONITOR 
	if (pEntry->Ipv6Dhcp == 1 || pEntry->AddrMode == IPV6_WAN_AUTO_DETECT_MODE) 
#else
	if (pEntry->Ipv6Dhcp == 1) 
#endif
	{
		ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));
		snprintf(pidfile, 64, "%s.%s", PATH_DHCLIENT6_PID, ifname);
		snprintf(cffile, 64, "%s.%s", PATH_DHCLIENT6_CONF, ifname);
		//snprintf(leasefile, 64, "%s.%s", PATH_DHCLIENT6_LEASE, ifname);
		dhcpcpid = read_pid(pidfile);
		
		/* delete DHCPcv6 lease file for this interface */
		snprintf(leasefile, 64, "%s.%s", PATH_DHCLIENT6_LEASE, ifname);
		/* release dhcpv6 , and kill previous dhclient after release finished.
		 * Use -l to try to get lease once to avoid long duration. */
		mib_get_s(MIB_DHCPV6_CLIENT_DUID_MODE, (void *)&duid_mode, sizeof(duid_mode));
		if (duid_mode == DHCPV6_DUID_LL)
			va_cmd(DHCPCV6, 14, 1, "-6", "-r", "-1", "-cf", cffile, "-lf", leasefile, "-pf", pidfile, ifname, "-N", "-P", "-D", "LL");
		else
			va_cmd(DHCPCV6, 14, 1, "-6", "-r", "-1", "-cf", cffile, "-lf", leasefile, "-pf", pidfile, ifname, "-N", "-P", "-D", "LLT");

		/* If you want to delete DHCPv6 leasefile in this API. please see below #else log */
#ifdef CONFIG_CMCC
        unsigned char dhcpv6_force_release = 0;
        mib_get(CWMP_DHCPV6_FORCE_RELEASE, &dhcpv6_force_release);
        if(dhcpv6_force_release && (pEntry->applicationtype & X_CT_SRV_TR069) )
        {
            printf("cmcc gd: unlink lease %s\n",leasefile);
		    unlink(leasefile);
        }
#else
		/* Because now we will send DHCPv6 RELEASE message before killing process, wes will send Solict Message next.
		 * And ISC-DHCPv6 will delete leasefile after sending DHCPv6 RELEASE message. So we don't need to delete it.
		 * If no leasefile, will let DHCPv6 first send Solicit Message instead of Rebind Message
		 *
		 * Note: Because leasefile is deleted if you want to use leasefile info (like IAPD or IANA info) to delete
		 *       Route or do other thing, you should do it before call this API. Otherwise, you can't get into to
		 *       do what you want. 20220114.
		 */
		//unlink(leasefile); //Don't delete leasefile by ourselves, unless you want to send REBIND message not SOLICIT.
#endif
		if(dhcpcpid > 0) {
			unlink(cffile);
		}
	}
	return 0;
}
	
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
//bridge mode br0 request IANA is enough
int do_dhcpc6_br(char *br_ifname, int mode)
{
	char *argv[32];
	unsigned char pidfile[64]={0}, leasefile[64]={0},scriptfile[64]={0};
	int idx = 1;
	char DHCPC6_SCRIPT[]="/var/run/dhcpc6.sh";
	FILE *scriptfp;
	
	snprintf(leasefile, 64, "%s.%s", PATH_DHCLIENT6_LEASE, br_ifname);
	snprintf(pidfile, 64, "%s.%s", PATH_DHCLIENT6_PID, br_ifname);
	snprintf(scriptfile, 64, "%s", DHCPCV6SCRIPT);
	argv[idx++] = "-6";
	argv[idx++] = br_ifname;
	argv[idx++] = "-d";
	argv[idx++] = "-q";
	argv[idx++] = "-lf";
	argv[idx++] = leasefile;
	argv[idx++] = "-pf";
	argv[idx++] = pidfile;
	argv[idx++] = "-sf";
	argv[idx++] = scriptfile;
	argv[idx++] = "--dad-wait-time";
	argv[idx++] = "5";
	if (mode == STATELESS_DHCP_MODE) 
	{ 
		argv[idx++] = "-S";
	}
	argv[idx] = NULL;

	kill_by_pidfile(pidfile, SIGTERM);
	/* Run script since we don't want to block web process.
	 In dhclient staless mode(-S), it is not run as daemon so we have to wait
	 for link-local DAD process before running it. */
	if ((scriptfp=fopen(DHCPC6_SCRIPT, "w")) == NULL)
		return 0;
	fprintf(scriptfp, "#!/bin/sh\n\n");
	fprintf(scriptfp, "%s", DHCPCV6);
	idx = 1;
	while (argv[idx] != NULL) {
		fprintf(scriptfp, " %s", argv[idx]);
		idx++;
	}
	fprintf(scriptfp, "&\n");
	fclose(scriptfp);
	if(chmod(DHCPC6_SCRIPT, 484) != 0)
		printf("chmod failed: %s %d\n", __func__, __LINE__);
	/* run script to launch dhcpc client */
	va_niced_cmd(DHCPC6_SCRIPT, 0, 0);

	return 0;
}
int del_dhcpc6_br(char *br_ifname)
{		
	unsigned char pidfile[64] = {0};
	int dhcpcpid = -1;
	unsigned char leasefile[64] = {0};
	char cmdBuf[128]={0};

	snprintf(cmdBuf, 128, "ip -6 addr flush scope global dev %s\n", br_ifname);
	va_cmd("/bin/sh", 2, 1, "-c", cmdBuf);

	// Stop DHCPv6 client
	snprintf(pidfile, 64, "%s.%s", PATH_DHCLIENT6_PID, br_ifname);
	dhcpcpid = read_pid(pidfile);

	if(dhcpcpid > 0) {
		kill(dhcpcpid, 15);
		unlink(pidfile);
		/* delete DHCPcv6 lease file for this interface */
		va_cmd("/bin/rm", 1, 1, leasefile);
	}
	return 0;
}
#endif
/*
 * rtk_quotify_buf() is reference from ISC-DHCPv6-4.1.1,
 * Note: have some change, not all be the same.
 *       And remember to free buf memory after calling this function.
 */
char *rtk_quotify_buf(const unsigned char *s, unsigned len)
{
	unsigned nulen = 0;
	char *buf = NULL, *nsp = NULL;
	int i = 0;

	for (i = 0; i < len; i++) {
		if (s [i] == ' ')
			nulen++;
		else if (!isascii (s [i]) || !isprint (s [i]))
			nulen += 4;
		else if (s [i] == '"' || s [i] == '\\')
			nulen += 2;
		else
			nulen++;
	}

	buf = (char *)calloc(nulen + 1, sizeof(char));
	if (buf) {
		nsp = buf;
		for (i = 0; i < len; i++) {
			if (s [i] == ' ')
				*nsp++ = ' ';
			else if (!isascii (s [i]) || !isprint (s [i])) {
				sprintf (nsp, "\\%03o", s [i]);
				nsp += 4;
			} else if (s [i] == '"' || s [i] == '\\') {
				*nsp++ = '\\';
				*nsp++ = s [i];
			} else
				*nsp++ = s [i];
		}
		*nsp++ = 0;
	}
	return buf;
}

/*
 * duid_type: DUID_LLT(1), DUID_EN(2), DUID_LL(3).
 *            DUID_LLT len = 14,
 *            DUID_EN  len =  x,
 *            DUID_LL  len = 10,
 * 			  enum { DHCPV6_DUID_LLT = 1, DHCPV6_DUID_EN, DHCPV6_DUID_LL } DHCPV6_DUID_TYPE_T;
 *
 * Now only support DUID_LL mode.
 *
 * TODO: DUID_LLT and DUID_EN
 */
int rtk_form_dhcpv6_duid(unsigned char *duid_str, int duid_type, char *macaddr)
{
	unsigned char *duid_tmp = NULL;
	int duid_len = 0;
	int duid_len_i = 0;

	if (duid_type == DHCPV6_DUID_LLT) /* MAC + time */
	{
		/* TODO */
		printf("%s: we don't have support DUID_LLT now!\n", __FUNCTION__);
		return -1;
	}
	else if (duid_type == DHCPV6_DUID_EN)
	{
		/* TODO */
		printf("%s: we don't have support DUID_LLT now!\n", __FUNCTION__);
		return -1;
	}
	else if (duid_type == DHCPV6_DUID_LL) /* MAC */
	{
		duid_len = 10;
		duid_tmp = (unsigned char *) calloc(duid_len, sizeof(unsigned char));
		if (duid_tmp != NULL)
		{
			putUShort(duid_tmp, DHCPV6_DUID_LL);
			putUShort(duid_tmp + 2, 0x01); /* Ethernet Hardware Type */

			//rtk_wan_get_mac_by_ifname(ifname, ip->hw_address.hbuf+1);
			memcpy(duid_tmp + 4, macaddr, MAC_ADDR_LEN); /* Must only be 6 bytes for MAC Address */
			//for debug
			/*
			printf("DUID_LL mac:");
			for (duid_len_i = 0; duid_len_i < duid_len; duid_len_i++)
				printf("%02x", duid_tmp[duid_len_i]);
			printf("\n");
			*/
			memcpy(duid_str , duid_tmp, duid_len);
			free(duid_tmp);
		}
		else
			printf("%s: memory alloc for duid failed!\n", __FUNCTION__);
	}
	else
	{
		printf("%s: we don't have this type num %d for DUID\n", __FUNCTION__, duid_type);
		return -1;
	}

	return 1;
}

int rtk_write_duid_to_isc_dhcp_lease_file(char *lease_file_path, char *duid_quotify_str)
{
	char duid_buf[256] = {0};
	FILE *isc_dhcpv6_lease = NULL;

	if (duid_quotify_str == NULL)
	{
		printf("%s: DUID string is NULL!\n", __FUNCTION__);
		return -1;
	}

	isc_dhcpv6_lease = fopen(lease_file_path, "w");
	if (isc_dhcpv6_lease == NULL)
	{
		printf("%s: Open file %s fail!\n", __FUNCTION__, lease_file_path);
		return -1;
	}

	/*
	 * isc-dhcpv6 duid format: default-duid "\000\003\000\001\001\036L\007h\003";
	 *                        default-duid "\000\001\000\001$\345K3\000\032+3\352\023";
	 */
	snprintf(duid_buf, sizeof(duid_buf), "default-duid \"%s\";", duid_quotify_str);
	fprintf(isc_dhcpv6_lease, "%s\n", duid_buf);
	fclose(isc_dhcpv6_lease);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(lease_file_path, 0666);
#endif
	return 1;
}

/* rtk_do_isc_dhcpc6_lease_with_duid_ll: generate ISC DHCPv6 DUID-LL to lease file. */
int rtk_do_isc_dhcpc6_lease_with_duid_ll(MIB_CE_ATM_VC_Tp pEntry, char *ifname)
{
	unsigned char duid_hex_str[10] = {0};
	char *duid_quotify_str = NULL;
	int ret = -1;
	char lease_file_path[64] = {0};

	if (pEntry == NULL || ifname == NULL)
	{
		printf("%s: pEntry or ifname is empty!\n", __FUNCTION__);
		return ret;
	}
	ret = rtk_form_dhcpv6_duid(duid_hex_str ,DHCPV6_DUID_LL, pEntry->MacAddr); /* MacAddr is hex, not string. */

	if (ret == 1)
		duid_quotify_str = rtk_quotify_buf(duid_hex_str, sizeof(duid_hex_str));
	//printf("duid_quotify_str = %s\n", duid_quotify_str);
	snprintf(lease_file_path, sizeof(lease_file_path), "%s.%s", PATH_DHCLIENT6_LEASE, ifname);
	ret = rtk_write_duid_to_isc_dhcp_lease_file(lease_file_path, duid_quotify_str);
	free(duid_quotify_str);
	return ret;
}

/* rtk_do_isc_dhcpc6_lease_with_duid_ll: generate ISC DHCPv6 DUID-LL to lease file. */
int rtk_get_duid_ll_for_isc_dhcp_lease_file(MIB_CE_ATM_VC_Tp pEntry, char *ifname, char *buf, int buf_len)
{
	unsigned char duid_hex_str[10] = {0};
	char *duid_quotify_str = NULL;
	int ret = -1;
	char lease_file_path[64] = {0};

	if (pEntry == NULL || ifname == NULL)
	{
		printf("%s: pEntry or ifname is empty!\n", __FUNCTION__);
		return ret;
	}
	ret = rtk_form_dhcpv6_duid(duid_hex_str ,DHCPV6_DUID_LL, pEntry->MacAddr); /* MacAddr is hex, not string. */

	if (ret == 1)
		duid_quotify_str = rtk_quotify_buf(duid_hex_str, sizeof(duid_hex_str));
	//printf("%s: Get duid_quotify_str = %s\n", __FUNCTION__, duid_quotify_str);
	snprintf(buf, buf_len, "%s", duid_quotify_str);
	free(duid_quotify_str);

	return ret;
}

static int unquotify_and_to_hex(const unsigned char *s, unsigned char *duid)
{
	char duid_temp[80]={0};
	int i, cur_index;
	unsigned char ch;

	for (i = 0, cur_index = 0; s[i];) {
		if (s[i] == '\\' && isdigit(s[i + 1]) && isdigit(s[i + 2]) && isdigit(s[i + 3])) {
			ch = strtoul(s + i + 1, NULL, 8);
			i += 4;
		} else if (s[i] == '\\' && (s[i + 1] == '"' || s[i + 1] == '\\')) {
			ch = s[i + 1];
			i += 2;
		} else if (s[i]=='"'){	
		/*Check for DHCP-4.4.1 duid format
		 * LLT:2bytes+2bytes+4bytes(Time)+6bytes(MAC)
 		 * DHCP441  will add " as enclose_char  
		 * e.g ia-na "\023\000\340L\000\001\000\001\"\365\265\302\214\354K\204\224D"
		 * DHCP411 will not 
		 * e.g ia-na \023\000\340L\000\001\000\001\"\365\265\302\214\354K\204\224D
		 * tranlate to Hex ==>
		 * DHCP 411 String format e.g	 00:01:00:01:22:F5:B5:C2:8C:EC:4B:84:94:44
		 * DHCP 441 String format e.g 22-00:01:00:01:22:F5:B5:C2:8C:EC:4B:84:94:44:22
		 */
			i+=1;
			continue;
		} else {
			ch = s[i];
			i += 1;
		}

		sprintf(duid_temp + cur_index, "%02X:", ch);
		cur_index += 3;
	}

	/* remove the trailing ':' */
	duid_temp[cur_index - 1] = '\0';
	
	/* skip the preceding unknown number */
	snprintf(duid, strlen(duid_temp), "%s", duid_temp+12);
	
	return 0;
}

int rtk_get_dhcpv6_lease_info(struct ipv6_lease *active_leases, int max_client_num, int mode,int *real_client_num)
{
	char buf[256]={0}, *str=NULL, unlimit_str[16]="4294967295";
	char iaaddr[INET6_ADDRSTRLEN]={0}, duid[80]={0};
	char ifname[IFNAMSIZ]={0};
	FILE *fp=NULL,*pp=NULL;
	int i, sum = 0;
	time_t epoch;
	struct tm expired_time;
	char active = 0;
	struct in6_addr in6_dst;
	char cmd[100]={0}, cmd_result[100]={0};

	if (mode==0){
		fp = fopen(DHCPDV6_LEASES, "r");
	}
	else {
		fp = fopen(DHCPDV6_STATIC_LEASES, "r");
	}

	if (fp == NULL)
		return -1;

	/* init */
	memset(active_leases, 0, max_client_num * sizeof(struct ipv6_lease));
	memset(&expired_time, 0 ,sizeof(struct tm));
	
	while (sum<max_client_num) {
		fgets(buf, sizeof(buf), fp);
		if (feof(fp) || ferror(fp)){			
			break;
		}
		str = strtok(buf, " \t");
		if (str == NULL){
			continue;
		}
		if (!strcmp(str, "iaaddr")) {
			/* got the IPv6 address */
			str = strtok(NULL, " \t");
			iaaddr[INET6_ADDRSTRLEN-1]='\0';
			strncpy(iaaddr, str,sizeof(iaaddr)-1);
		} else if (!strcmp(str, "ia-na")) {
			/* got the DUID */
			str = strtok(NULL, " \t");
			unquotify_and_to_hex(str,duid);
		} else if (!strcmp(str, "binding")) {
			/* check if binding state is active */
			strtok(NULL, " \t");
			str = strtok(NULL, ";");
			if (!strcmp(str, "active")) {
				active = 1;
			}
			else	if (!strcmp(str, "static")) {
				active=2;
				memset(&in6_dst, 0, sizeof(struct in6_addr));				
				inet_pton(PF_INET6, iaaddr, &in6_dst);

				//static lease active status, need check address nud state later
				snprintf(ifname,IFNAMSIZ, "%s", BRIF);
				rtk_ipv6_send_ping_request(ifname, &in6_dst);
			}
		} else if ( !strcmp(str, "ends")) {
			strtok(NULL, " \t");
			str = strtok(NULL, ";");
			if(str == NULL) //never lease timeout
				epoch = strtoll(unlimit_str, NULL, 10) - time(NULL);
			else
				epoch = strtoll(str, NULL, 10) - time(NULL);

			//strptime(str, "%Y/%m/%d %H:%M:%S", &expired_time);
			//epoch=mktime(&expired_time) - time(NULL);
	
			/* find duplicate and old records */
			for (i = 0; i < sum; i++) {
				if ((!strcmp(active_leases[i].iaaddr, iaaddr))||(!strcmp(active_leases[i].duid, duid))){
					/*if dup, update struct lease*/
					if (active) {
				        snprintf(active_leases[i].iaaddr,INET6_ADDRSTRLEN, "%s", iaaddr);
						snprintf(active_leases[i].duid, sizeof(duid),"%s", duid);
						active_leases[i].epoch = epoch;
						active_leases[i].active = active;
					}
					else
						active_leases[i].active = 0;
					break;
				}
			}
			if (i == sum) {
				/* save new leases */
				snprintf(active_leases[i].iaaddr,INET6_ADDRSTRLEN, "%s", iaaddr);
				snprintf(active_leases[i].duid, sizeof(duid),"%s", duid);
				active_leases[i].epoch = epoch;
				active_leases[i].active = active;
				sum++;
			}
			active = 0;
		}
	}
	fclose(fp);
	
	//check static address nud state in the end
	for (i =0; i<sum;i++){
		if (active_leases[i].active == 2){
			snprintf(cmd, sizeof(cmd), "ip -6 neigh | grep %s ",active_leases[i].iaaddr);
			pp = popen(cmd, "r");
			if(pp==NULL)
			{
				printf("popen cmd:%s error!\n",cmd);
				return 1;
			}
			fgets(cmd_result, sizeof(cmd_result), pp);
			if (strstr(cmd_result, "FAILED") !=NULL){
				active_leases[i].active=0;
			}						
			pclose(pp);
		}
	}
	*real_client_num = sum;
	return 0;
}

#if defined(CONFIG_USER_RTK_DHCPV6_SERVER_PD_SUPPORT) || defined(YUEME_DHCPV6_SERVER_PD_SUPPORT)
/* calculate_prefix6_range: Calculate prefix6 range to LAN client and void use first prefix,
 * first prefix is assigned for br0.
 * output prefix6_subnet_start_str and prefix6_subnet_end_str, and LAN PD len.
 * It will return start and end prefix6 string into prefix6_subnet_start_str and prefix6_subnet_end_str
 */
int calculate_prefix6_range(PREFIX_V6_INFO_Tp PrefixInfo, struct ipv6_addr_str_array *prefix6_subnet_start_str, struct ipv6_addr_str_array *prefix6_subnet_end_str, int*lan_pd_len)
{
	char ipv6_tmp_str[INET6_ADDRSTRLEN] = {0};
	int valid_pd_bit_range=0;
	struct in6_addr ipv6_addr1 = {0}, ipv6_addr2 = {0};
	struct in6_addr ipv6_addr1_final = {0}, ipv6_addr2_final  = {0};
	int m=0,n=0;

	//check Parameter
	if (!memcmp(PrefixInfo->prefixIP, ipv6_unspec, PrefixInfo->prefixLen<=128?PrefixInfo->prefixLen:128) || (PrefixInfo->prefixLen<= 0) || (PrefixInfo->prefixLen>= 64) || !prefix6_subnet_start_str || !prefix6_subnet_end_str)
	{
		printf("%s: Invalid input arguments!\n", __FUNCTION__);
		return -1;
	}

	memcpy(&ipv6_addr1, PrefixInfo->prefixIP, sizeof(ipv6_addr1));
	memcpy(&ipv6_addr2, PrefixInfo->prefixIP, sizeof(ipv6_addr2));

	if (PrefixInfo->prefixLen <= 56)
		valid_pd_bit_range = PrefixInfo->prefixLen  + 4;
	else if (PrefixInfo->prefixLen <= 59)
		valid_pd_bit_range = PrefixInfo->prefixLen  + 3;
	else if (PrefixInfo->prefixLen <= 61)
		valid_pd_bit_range = PrefixInfo->prefixLen  + 2;
	else if (PrefixInfo->prefixLen == 62)
		valid_pd_bit_range = 63;
	else{
		printf("%s: Invalid Prefix Len for Lan Server PD !\n", __FUNCTION__);
		return -1;
	}

	prefixtoIp6(PrefixInfo->prefixIP,PrefixInfo->prefixLen,&ipv6_addr1,0);
	prefixtoIp6(PrefixInfo->prefixIP,PrefixInfo->prefixLen,&ipv6_addr2,1);
	ip6toPrefix(&ipv6_addr1,valid_pd_bit_range,&ipv6_addr1_final);
	ip6toPrefix(&ipv6_addr2,valid_pd_bit_range,&ipv6_addr2_final);

	/* Count start Prefix IP avoid PD which is used to LAN interface.
	 * e.g WAN PrefixInfo = 2001:db8:0:1200::/56    LAN interface address 2001:db8:0:1200::<EUI-64>/56
	 * ==> LAN server PD=>
	 * prefix6 2001:db8:0:1210:: 2001:db8:0:12f0:: /60;
	 */
	m=valid_pd_bit_range/8;
	n=valid_pd_bit_range%8;
	if (n==0){
		ipv6_addr1_final.s6_addr[m-1] +=1;
	}else{
		ipv6_addr1_final.s6_addr[m] |=(0x01<<(8-n)) ;
	}

	inet_ntop(AF_INET6, &ipv6_addr1_final, ipv6_tmp_str, sizeof(ipv6_tmp_str));
	snprintf(prefix6_subnet_start_str->ipv6_addr_str, sizeof(struct ipv6_addr_str_array), "%s", ipv6_tmp_str);
	//printf("Subnet startIP = %s!\n", prefix6_subnet_start_str->ipv6_addr_str);
	inet_ntop(AF_INET6, &ipv6_addr2_final, ipv6_tmp_str, sizeof(ipv6_tmp_str));
	snprintf(prefix6_subnet_end_str->ipv6_addr_str, sizeof(struct ipv6_addr_str_array), "%s", ipv6_tmp_str);
	//printf("Subnet End IP = %s!\n", prefix6_subnet_end_str->ipv6_addr_str);

	if (compare_ipv6_addr(&ipv6_addr1, &ipv6_addr2) > 0) //start address > end address
	{
		printf("%s: Doesn't have free Subnet prefix for %s!\n", __FUNCTION__, prefix6_subnet_start_str->ipv6_addr_str);
		memset(prefix6_subnet_start_str, 0 , sizeof(struct ipv6_addr_str_array));
		memset(prefix6_subnet_end_str, 0 , sizeof(struct ipv6_addr_str_array));
		*lan_pd_len = 0;
		return -1;
	}

	*lan_pd_len = valid_pd_bit_range;
	return 1;
}
#endif // end of #if defined(CONFIG_USER_RTK_DHCPV6_SERVER_PD_SUPPORT) || defined(YUEME_DHCPV6_SERVER_PD_SUPPORT)
#endif //end of DHCPV6_ISC_DHCP_4XX

/*
 * FUNC: rtk_del_dhcp6s_pd_policy_route_and_file
 * DESC: Delete prefix delegation route file if del that WAN. It will Del LAN's PD from this MIB_CE_ATM_VC_Tp WAN
 *       in route table DS_DHCPV6S_LAN_PD_ROUTE(258).
 */
int rtk_del_dhcp6s_pd_policy_route_and_file(MIB_CE_ATM_VC_Tp pEntry)
{
	char ifname[IFNAMSIZ] = {0}, dhcp6s_lan_pd_mapping_wan_path[64] = {0};
	char tmp_cmd[128] = {0}, tmp_buf[256] = {0};
	FILE *dhcp6s_lan_pd_route_file_p = NULL;
	char prefix_str[INET6_ADDRSTRLEN + 4/*::/64*/] = {0}, nexthop_str[INET6_ADDRSTRLEN] = {0}, dev_str[IFNAMSIZ] = {0};
	char *str_p1 = NULL;
	char *token = NULL, *saveptr = NULL;

	if (ifGetName(pEntry->ifIndex, ifname, sizeof(ifname)) == NULL)
		return -1;

	snprintf(dhcp6s_lan_pd_mapping_wan_path, sizeof(dhcp6s_lan_pd_mapping_wan_path), "%s_%s", DHCP6S_LAN_PD_ROUTE_PATH, ifname);

	/* Del DHCPv6 LAN PD route rule for this WAN. */
	if ((dhcp6s_lan_pd_route_file_p = fopen(dhcp6s_lan_pd_mapping_wan_path, "r")) != NULL)
	{
		while (fgets(tmp_buf, sizeof(tmp_buf), dhcp6s_lan_pd_route_file_p))
		{
			str_p1 = &tmp_buf[0];
			//format : 8022:2018:103:a7::/64 fe80::2 br0 nas0_0
			if ((token = strtok_r(str_p1, " ", &saveptr))) {
				snprintf(prefix_str, sizeof(prefix_str), "%s", token);
			}
			if ((token = strtok_r(NULL, " ", &saveptr))) {
				snprintf(nexthop_str, sizeof(nexthop_str), "%s", token);
			}
			if ((token = strtok_r(NULL, " ", &saveptr))) {
				snprintf(dev_str, sizeof(dev_str), "%s", token);
			}
			if (token != NULL)
			{
				snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 route del %s via %s dev %s table %u", prefix_str, nexthop_str, dev_str, DS_DHCPV6S_LAN_PD_ROUTE);
				va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);
				printf("%s: %s\n",__FUNCTION__, tmp_cmd);
			}
		}
		fclose(dhcp6s_lan_pd_route_file_p);
	}

	/* Del DHCPv6 LAN PD route file for this WAN. */
	snprintf(tmp_cmd, sizeof(tmp_cmd), "rm %s", dhcp6s_lan_pd_mapping_wan_path);
	printf("%s: %s\n",__FUNCTION__, tmp_cmd);
	va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);

	return 0;
}

/*
 * FUNC: rtk_dhcp6s_del_lan_pd_policy_route
 * DESC: Delete prefix delegation route in route table DS_DHCPV6S_LAN_PD_ROUTE(258) if kill wide-dhcp6s(SIG_TERM) or
 *       receive Release Message with PD from client(Only with ISC-DHCPv6S)
 *       when CONFIG_USER_RTK_DHCPV6_SERVER_PD_SUPPORT is enabled
 * NOTE: No support kill dhcpd(SIG_TERM) for ISC-DHCP6S, only support receive Release Message with PD
 */
int rtk_dhcp6s_del_lan_pd_policy_route(char *lan_ifname, struct in6_addr *prefix_ip, unsigned int prefix_len, struct in6_addr *nexthop_addr)
{
	char ifname[IFNAMSIZ] = {0}, dhcp6s_lan_pd_mapping_wan_path[64] = {0};
	char prefix_str[INET6_ADDRSTRLEN] = {0}, nexthop_str[INET6_ADDRSTRLEN] = {0};
	char tmp_cmd[256] = {0}, sed_cmd[256] = {0};
	MIB_CE_ATM_VC_T vcEntry;

	if (!lan_ifname || !prefix_ip) //we support delete no nexthop_addr route for wide-dhcpv6s.
		return -1;

	inet_ntop(AF_INET6, prefix_ip, prefix_str, sizeof(prefix_str));
	if (nexthop_addr != NULL)
		inet_ntop(AF_INET6, nexthop_addr, nexthop_str, sizeof(nexthop_str));

	if ((prefix_str[0] == '\0') || (nexthop_addr != NULL && (nexthop_str[0] == '\0')))
	{
		printf("%s: wrong IPv6 PD IP or Nexthop with %s!\n", __FUNCTION__, lan_ifname);
		return -1;
	}

	if (rtk_get_lan_bind_wan_ifname(lan_ifname, ifname))
	{
		if (!get_wan_interface_by_pd(prefix_ip, ifname))
			printf("%s:Can't get mapping LAN PD %s/%u with wan interface\n", __FUNCTION__, prefix_str, prefix_len);
	}

	snprintf(dhcp6s_lan_pd_mapping_wan_path, sizeof(dhcp6s_lan_pd_mapping_wan_path), "%s_%s", DHCP6S_LAN_PD_ROUTE_PATH, ifname);

	/* Delete routing record
	 * sed string: sed -i '/2001:2018:103:9b::\/64/d' /var/run/dhcp6s_lan_pd_route_nas0_1
	 * Now dev is always be LANIF, we should change it for brX in the future. */
	snprintf(sed_cmd, sizeof(sed_cmd), "sed -i '/%s\\/%u %s %s/d' %s", prefix_str, prefix_len, nexthop_str, LANIF, dhcp6s_lan_pd_mapping_wan_path);
	va_cmd_no_error("/bin/sh", 2, 1, "-c", sed_cmd);

	//del DHCPv6S LAN PD route
	//ip -6 route del 2001:2018:50:5b::/64 via fe80::2 dev br0 table 258
	snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 route del %s/%u via %s dev %s table %u", prefix_str, prefix_len, nexthop_str, LANIF, DS_DHCPV6S_LAN_PD_ROUTE);
	va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);
	printf("%s: %s\n",__FUNCTION__, tmp_cmd);

	return 0;
}

/*
 * FUNC: rtk_dhcp6s_setup_lan_pd_policy_route
 * DESC: Setup prefix delegation route in route table DS_DHCPV6S_LAN_PD_ROUTE(258) if wide-dhcp6s or
 *       ISC-dhcp6s send Reply Message with PD to client when CONFIG_USER_RTK_DHCPV6_SERVER_PD_SUPPORT
 *       is enabled. We write PD/PD len, Nexthop, dev brX, from_wan_inf in
 *       DHCP6S_LAN_PD_ROUTE_PATH_"inf(brX or nas0_X)", then add route for main or policy routing table.
 */
int rtk_dhcp6s_setup_lan_pd_policy_route(char *lan_ifname, struct in6_addr *prefix_ip, unsigned int prefix_len, struct in6_addr *nexthop_addr)
{
	char ifname[IFNAMSIZ] = {0}, dhcp6s_lan_pd_mapping_wan_path[64] = {0}, from_wan_ifname[IFNAMSIZ] = {0};
	char prefix_str[INET6_ADDRSTRLEN] = {0}, nexthop_str[INET6_ADDRSTRLEN] = {0};
	char tmp_cmd[256] = {0}, sed_cmd[256] = {0};
	MIB_CE_ATM_VC_T vcEntry;
	unsigned char lanIPv6PrefixMode = 0;
	unsigned int wan_conn = DUMMY_IFINDEX;
	DLG_INFO_T dlgInfo = {0};

	if (!lan_ifname || !prefix_ip || !nexthop_addr)
		return -1;

	inet_ntop(AF_INET6, prefix_ip, prefix_str, sizeof(prefix_str));
	inet_ntop(AF_INET6, nexthop_addr, nexthop_str, sizeof(nexthop_str));

	if ((prefix_str[0] == '\0') || (nexthop_str[0] == '\0'))
	{
		printf("%s: wrong IPv6 PD IP or Nexthop with %s!\n", __FUNCTION__, ifname);
		return -1;
	}

	if (rtk_get_lan_bind_wan_ifname(lan_ifname, ifname))
	{
		/* Can't find wan ifname, let ifname to be lan_ifname */
		if (lan_ifname[0] != '\0')
			snprintf(ifname, sizeof(ifname), "%s", lan_ifname);
		else
		{
			printf("%s: All infname is empty!\n",__FUNCTION__);
			return -1;
		}
	}

	if (strstr(ifname, "br"))
	{
		/* We don't support this feature for WAN Static PD, because Static PD length is always 64 in our designed
		 * now. So it's impossible to assign any PD to LAN client. 20200921*/
		if (!get_wan_interface_by_pd(prefix_ip, from_wan_ifname))
			printf("%s:Can't get mapping LAN PD %s/%u with  wan interface\n", __FUNCTION__, prefix_str, prefix_len);
	}
	else
		snprintf(from_wan_ifname, sizeof(from_wan_ifname), "%s", ifname);

	snprintf(dhcp6s_lan_pd_mapping_wan_path, sizeof(dhcp6s_lan_pd_mapping_wan_path), "%s_%s", DHCP6S_LAN_PD_ROUTE_PATH, from_wan_ifname);
	/* dhcp6s_lan_pd_mapping_wan_path maybe /var/run/dhcp6s_lan_pd_route_nas0_X
	 * /var/run/dhcp6s_lan_pd_route_nas0_X is for WAN policy route
	 *
	 * format: PD/PD length          Nexthop                  LAN_BR  PD's WAN
	 *         2001:2018:103:9b::/64 fe80::25a:25ff:feaa:bb51 brX     nas0_X(Empty mean unknown WAN inf)
	 *
	 * sed string sed -i '/2001:2018:103:9b::\/64/d' /var/run/dhcp6s_lan_pd_route_nas0_1
	 */
	snprintf(tmp_cmd, sizeof(tmp_cmd), "echo \"%s/%u %s %s %s\" >> %s", prefix_str, prefix_len, nexthop_str, LANIF, from_wan_ifname, dhcp6s_lan_pd_mapping_wan_path);
	snprintf(sed_cmd, sizeof(sed_cmd), "sed -i '/%s\\/%u %s %s/d' %s", prefix_str, prefix_len, nexthop_str, LANIF, dhcp6s_lan_pd_mapping_wan_path);

	va_cmd_no_error("/bin/sh", 2, 1, "-c", sed_cmd); //Del PD list in DHCP6S_LAN_PD_ROUTE_PATH file.
	printf("%s: %s\n", __FUNCTION__, tmp_cmd);
	va_cmd("/bin/sh", 2, 1, "-c", tmp_cmd);

	rtk_set_dhcp6s_lan_pd_policy_route_info(from_wan_ifname, NULL);

	return 0;
}

/*
 * FUNC: rtk_set_dhcp6s_lan_pd_policy_route_info
 * DESC: Add prefix delegation route in route table DS_DHCPV6S_LAN_PD_ROUTE(258)
 */
int rtk_set_dhcp6s_lan_pd_policy_route_info(char *ifname, char *table_id)
{
	FILE *dhcp6s_lan_pd_route_file_p = NULL;
	char dhcp6s_lan_pd_route_file_path[64] = {0};
	char tmp_buf[256] = {0}, tmp_cmd[256] = {0};
	char *str_p1 = NULL;
	char prefix_str[INET6_ADDRSTRLEN + 4/* ::/64 */] = {0}, nexthop_str[INET6_ADDRSTRLEN] = {0}, dev_str[IFNAMSIZ] = {0};
	char *token = NULL, *saveptr = NULL;

	if (!ifname || ifname[0] == '\0')
		return -1;

	snprintf(dhcp6s_lan_pd_route_file_path, sizeof(dhcp6s_lan_pd_route_file_path), "%s_%s", DHCP6S_LAN_PD_ROUTE_PATH, ifname);
	if ((dhcp6s_lan_pd_route_file_p = fopen(dhcp6s_lan_pd_route_file_path, "r")) != NULL)
	{
		while (fgets(tmp_buf, sizeof(tmp_buf), dhcp6s_lan_pd_route_file_p))
		{
			str_p1 = &tmp_buf[0];
			//format : 8022:2018:103:a7::/64 fe80::2 br0 nas0_0
			if ((token = strtok_r(str_p1, " ", &saveptr))) {
				snprintf(prefix_str, sizeof(prefix_str), "%s", token);
			}
			if ((token = strtok_r(NULL, " ", &saveptr))) {
				snprintf(nexthop_str, sizeof(nexthop_str), "%s", token);
			}
			if ((token = strtok_r(NULL, " ", &saveptr))) {
				snprintf(dev_str, sizeof(dev_str), "%s", token);
			}
			//if you need to parse WAN inf, you can strtok_r again.
			if (token != NULL)
			{
				//ip -6 route add 2001:2018:50:5b::/64 via fe80::2 dev br0 table 258
				snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 route add %s via %s dev %s table %u", prefix_str, nexthop_str, dev_str, DS_DHCPV6S_LAN_PD_ROUTE);
				va_cmd("/bin/sh", 2, 1, "-c", tmp_cmd);
				printf("%s: %s\n",__FUNCTION__, tmp_cmd);
			}
		}
		fclose(dhcp6s_lan_pd_route_file_p);
	}
	else
		return -1;

#if 0
	//Add DS_DHCPV6S_LAN_PD_ROUTE in ip6 rule.
	if (!is_rule_priority_exist_in_ip6_rule(LAN_DHCP6S_PD_ROUTE))
	{
		//ip -6 rule add from all pref 19950 table 258
		snprintf(tmp_cmd, sizeof(tmp_cmd), "ip -6 rule add from all pref %u table %u", LAN_DHCP6S_PD_ROUTE, DS_DHCPV6S_LAN_PD_ROUTE);
		va_cmd("/bin/sh", 2, 1, "-c", tmp_cmd);
		printf("%s: %s\n",__FUNCTION__, tmp_cmd);
	}
#endif

	return 0;
}

#endif // #ifdef CONFIG_IPV6

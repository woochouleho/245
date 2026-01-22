/*
 *      Web server handler routines for System IPv6 status
 *
 */

#include <string.h>
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "debug.h"
#include "multilang.h"

#ifdef CONFIG_IPV6
void formStatus_ipv6(request * wp, char *path, char *query)
{
	char *submitUrl;

	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
  	return;
}


int wanip6ConfList(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	int in_turn=0, ifcount=0;
	int i, j, k;
	struct ipv6_ifaddr ip6_addr[6];
	char buf[256], str_ip6[INET6_ADDRSTRLEN];
	struct wstatus_info sEntry[MAX_VC_NUM+MAX_PPP_NUM];

	ifcount = getWanStatus(sEntry, MAX_VC_NUM+MAX_PPP_NUM);
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr bgcolor=\"#808080\">"
	"<td width=\"8%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"22%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td></tr>\n",
#else
	nBytesSent += boaWrite(wp, "<tr>"
	"<th width=\"8%%\">%s</th>\n"
	"<th width=\"12%%\">%s</th>\n"
	"<th width=\"12%%\">%s</th>\n"
	"<th width=\"12%%\">%s</th>\n"
	"<th width=\"22%%\">%s</th>\n"
	"<th width=\"12%%\">%s</th></tr>\n",
#endif
	multilang(LANG_INTERFACE), 
#if defined(CONFIG_LUNA)
	multilang(LANG_VLAN_ID), multilang(LANG_CONNECTION_TYPE),
#else
	multilang(LANG_VPI_VCI), multilang(LANG_ENCAPSULATION),
#endif
	multilang(LANG_PROTOCOL), multilang(LANG_IP_ADDRESS), multilang(LANG_STATUS));
	in_turn = 0;
	for (i=0; i<ifcount; i++) {
		if (sEntry[i].ipver == IPVER_IPV4)
			continue; // not IPv6 capable
#ifndef CONFIG_GENERAL_WEB
		if (in_turn == 0)
			nBytesSent += boaWrite(wp, "<tr bgcolor=\"#EEEEEE\">\n");
		else
			nBytesSent += boaWrite(wp, "<tr bgcolor=\"#DDDDDD\">\n");
#else
		if (in_turn == 0)
			nBytesSent += boaWrite(wp, "<tr>\n");
#endif

		in_turn ^= 0x01;
		if(!strcmp(sEntry[i].ifname, "nas0_1"))
		{
			strcpy(sEntry[i].ifname, "br0");
			strcpy(sEntry[i].ifDisplayName, "br0");
		}
		k=getifip6(sEntry[i].ifname, IPV6_ADDR_UNICAST, ip6_addr, 6);
		buf[0]=0;
		if (k) {
			for (j=0; j<k; j++) {
				inet_ntop(PF_INET6, &ip6_addr[j].addr, str_ip6, INET6_ADDRSTRLEN);

				if (j == 0)
#ifdef CONFIG_00R0 //for rostelecom is mask bit not prefix len
					sprintf(buf, "%s/%d", str_ip6, 128);
#else
					sprintf(buf, "%s/%d", str_ip6, ip6_addr[j].prefix_len);
#endif
				else
#ifdef CONFIG_00R0
					sprintf(buf + strlen(buf), ", %s/%d%s", str_ip6, 128);
#else
					sprintf(buf + strlen(buf), ", %s/%d%s", str_ip6, ip6_addr[j].prefix_len);
#endif
			}
		}
		nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
		"<td align=center width=\"5%%\"><font size=2>%s</td>\n"
#if defined(CONFIG_LUNA)
		"<td align=center width=\"1%%\"><font size=2>%d</td>\n"
		"<td align=center width=\"9%%\"><font size=2>%s</td>\n"
#else
		"<td align=center width=\"5%%\"><font size=2>%s</td>\n"
		"<td align=center width=\"5%%\"><font size=2>%s</td>\n"
#endif
		"<td align=center width=\"5%%\"><font size=2>%s</td>\n"
		"<td align=center width=\"10%%\"><font size=2>%s</td>\n"
		"<td align=center width=\"23%%\"><font size=2>%s\n",
#else	//CONFIG_GENERAL_WEB
		"<td width=\"5%%\">%s</th>\n"
#if defined(CONFIG_LUNA)
		"<td width=\"1%%\">%d</th>\n"
		"<td width=\"9%%\">%s</th>\n"
#else
		"<td width=\"5%%\">%s</th>\n"
		"<td width=\"5%%\">%s</th>\n"
#endif
		"<td width=\"5%%\">%s</th>\n"
		"<td width=\"10%%\">%s</th>\n"
		"<td width=\"23%%\">%s\n",
#endif	//CONFIG_GENERAL_WEB
		sEntry[i].ifDisplayName, 
#if defined(CONFIG_LUNA)
		sEntry[i].vid, sEntry[i].serviceType,
#else
		sEntry[i].vpivci, sEntry[i].encaps,
#endif
		sEntry[i].protocol, buf, sEntry[i].strStatus_IPv6);
		nBytesSent += boaWrite(wp, "</td></tr>\n");
	}
	return nBytesSent;
}

#define MAX_LINE_LENGTH 256
int neighborList(int eid, request * wp, int argc, char **argv){
	FILE	*fp = NULL;
	int		nBytesSent = 0;
	char	buf[MAX_LINE_LENGTH] = {0,};
	
	fp = popen("/usr/bin/ip -6 neigh", "r");
	if (fp == NULL)
	{
		return nBytesSent;
	}

	nBytesSent += boaWrite(wp, "<textarea style='color:#ffffff; background-color:black; width:100%; resize: none; min-height: 100px;'>\n");
	while (fgets(buf, MAX_LINE_LENGTH, fp) != NULL)
	{
		nBytesSent += boaWrite(wp, "%s", buf);
	}
	nBytesSent += boaWrite(wp, "</textarea>\n");

	pclose(fp);

	return nBytesSent;
}

int routeip6ConfList(int eid, request * wp, int argc, char **argv){
	FILE *fp = NULL;
	int nBytesSent = 0, route_count=0, i=0;
	char tmp_cmd[16] = {0};
	char route_line[256]={0}, tmp_line[256]={0}, dest_ip[64]={0}, dest_ip_str[64]={0}, gw_ip[64]={0}, source_ip[64]={0};
	char *ret=NULL,*via=NULL,*dev=NULL,*metric=NULL; 
	struct ipv6_route* route_info=NULL;
	
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr bgcolor=\"#808080\">"
	"<td width=\"12%%\" align=center><font size=2><b>%s IP</b></font></td>\n"
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"8%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"8%%\" align=center><font size=2><b>%s</b></font></td></tr>\n",
#else
	nBytesSent += boaWrite(wp, "<tr>"
	"<th width=\"12%%\">%s IP</th>\n"
	"<th width=\"12%%\">%s</th>\n"
	"<th width=\"12%%\">%s</th>\n"
	"<th width=\"8%%\">%s</th>\n"
	"<th width=\"8%%\">%s</th></tr>\n",
#endif
	multilang(LANG_DESTINATION), multilang(LANG_SOURCE), multilang(LANG_GATEWAY), 
	multilang(LANG_METRIC), multilang(LANG_INTERFACE));

	route_count = rtk_ipv6_get_route(&route_info);
	//printf("%s:%d ipv6_route_num %d\n", __FUNCTION__, __LINE__, route_count);
	for(i=0; i < route_count; i++){
		inet_ntop(PF_INET6, &route_info[i].target, dest_ip_str, sizeof(dest_ip_str));
		//skip ff00 route
		if(strncmp(dest_ip_str, "ff00::", INET6_ADDRSTRLEN)==0){
			continue;
		}
		if(strncmp(route_info[i].devname, "lo", sizeof(route_info[i].devname))==0){
			continue;
		}
		//target
		snprintf(dest_ip, sizeof(dest_ip), "%s/%d", dest_ip_str, route_info[i].prefix_len);
			
		//source
		inet_ntop(PF_INET6, &route_info[i].source, route_line, sizeof(route_line));
		if(strncmp(route_line, "::", INET6_ADDRSTRLEN)==0){
			
			if(strncmp(dest_ip_str, "::", INET6_ADDRSTRLEN)==0){
				snprintf(dest_ip, sizeof(dest_ip), "%s", "default");
			}
		}
		snprintf(source_ip, sizeof(source_ip), "%s/%d", route_line, route_info[i].source_prefix_len);

		//gateway
		inet_ntop(PF_INET6, &route_info[i].gateway, gw_ip, sizeof(gw_ip));

			nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
			"<td align=center width=\"12%%\"><font size=2>%s</td>\n"
			"<td align=center width=\"12%%\"><font size=2>%s</td>\n"
		"<td align=center width=\"12%%\"><font size=2>%s</td>\n"
		"<td align=center width=\"8%%\"><font size=2>%u</td>\n"
			"<td align=center width=\"8%%\"><font size=2>%s</td>\n",
#else
			"<td width=\"12%%\">%s</td>\n"
			"<td width=\"12%%\">%s</td>\n"
		"<td width=\"12%%\">%s</td>\n"
		"<td width=\"8%%\">%u</td>\n"
			"<td width=\"8%%\">%s</td>\n",
#endif
		dest_ip, source_ip, gw_ip, route_info[i].metric, route_info[i].devname);
			nBytesSent += boaWrite(wp, "</tr>\n");

		//printf("%s:%d: target %s, source %s, gateway %s, metric %d, devname %s\n", __FUNCTION__, __LINE__, dest_ip, source_ip, gw_ip, route_info[i].metric, route_info[i].devname);
		}
	if(route_info)
		free(route_info);
		
	return nBytesSent;
}

int dsliteConfList(int eid, request * wp, int argc, char **argv){
	MIB_CE_ATM_VC_T Entry;
	unsigned int entry_Num;
	char ifname[IFNAMSIZ];
	DLG_INFO_T tempInfo = {0};
	char inf[IFNAMSIZ];
	char leasefile[64] = {0};
	char dslite_dhcpv6_option[16];
	int in_turn, ifcount=0;
	int totalNum, nBytesSent=0;
	int i;

	entry_Num = mib_chain_total(MIB_ATM_VC_TBL);
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr bgcolor=\"#808080\">"
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"22%%\" align=center><font size=2><b>%s</b></font></td></tr>\n",
#else
	nBytesSent += boaWrite(wp, "<tr>"
	"<th width=\"12%%\">%s</th>\n"
	"<th width=\"12%%\">%s</th>\n"
	"<th width=\"12%%\">%s</th>\n"
	"<th width=\"22%%\">%s</th></tr>\n",
#endif
	multilang(LANG_INTERFACE), multilang(LANG_AFTR_NAME), multilang(LANG_AFTR_ADDRESS), multilang(LANG_DS_LITE_DHCPV6_OPTION));
	in_turn = 0;
	for (i=0; i<entry_Num; i++) {		
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&Entry)){
			printf("get chain failed,id=%d,recordNum=%d,%s:%d\n",MIB_ATM_VC_TBL,i,__func__,__LINE__);
			continue;
		}
		if (Entry.cmode == CHANNEL_MODE_BRIDGE || (Entry.IpProtocol != IPVER_IPV6))
			continue;
		//ifGetName(Entry.ifIndex, ifname, sizeof(ifname));
		getDisplayWanName(&Entry, ifname);
		if(Entry.dslite_enable){
#ifndef CONFIG_GENERAL_WEB
			if (in_turn == 0)
				nBytesSent += boaWrite(wp, "<tr bgcolor=\"#EEEEEE\">\n");
			else
				nBytesSent += boaWrite(wp, "<tr bgcolor=\"#DDDDDD\">\n");
#else
			if (in_turn == 0)
				nBytesSent += boaWrite(wp, "<tr>\n");
#endif
			
			memset(dslite_dhcpv6_option, 0, sizeof(dslite_dhcpv6_option));
			if(0==Entry.dslite_aftr_mode){
				snprintf(dslite_dhcpv6_option, sizeof(dslite_dhcpv6_option), "%s", "enable");

				ifGetName(Entry.ifIndex, inf, sizeof(inf));
				snprintf(leasefile, sizeof(leasefile), "%s.%s", (char *)PATH_DHCLIENT6_LEASE, inf);
				if (getLeasesInfo(leasefile, &tempInfo) && LEASE_GET_AFTR==0)
					snprintf(tempInfo.dslite_aftr, sizeof(tempInfo.dslite_aftr),"%s","");
			}	
			else{
				snprintf(tempInfo.dslite_aftr, sizeof(tempInfo.dslite_aftr),"%s","");
				snprintf(dslite_dhcpv6_option, sizeof(dslite_dhcpv6_option), "%s", "disable");
			}
				
			//printf("[%s:%d]interfacenname=%s,ip=%s,option=%s\n",__func__,__LINE__,sEntry[i].ifDisplayName, Entry.dslite_aftr_hostname, dslite_dhcpv6_option);
			in_turn ^= 0x01;
			nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
			"<td align=center width=\"2%%\"><font size=2>%s</td>\n"
			"<td align=center width=\"23%%\"><font size=2>%s</td>\n"
			"<td align=center width=\"23%%\"><font size=2>%s</td>\n"
			"<td align=center width=\"23%%\"><font size=2>%s</td>\n",
#else	//CONFIG_GENERAL_WEB
			"<td width=\"2%%\">%s</td>\n"
			"<td width=\"23%%\">%s</td>\n"
			"<td width=\"23%%\">%s</td>\n"
			"<td width=\"2%%\">%s</td>\n",
#endif	//CONFIG_GENERAL_WEB
			ifname, tempInfo.dslite_aftr, strValToASP(Entry.dslite_aftr_hostname), dslite_dhcpv6_option);
			nBytesSent += boaWrite(wp, "</tr>\n");

		}

	}
	return 0;
}

#endif //#ifdef CONFIG_IPV6

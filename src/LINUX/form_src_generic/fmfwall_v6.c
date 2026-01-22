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
#include <config/autoconf.h>
#else
#include "../../../../include/linux/autoconf.h"
#include "../../../../config/autoconf.h"
#endif

#if defined(CONFIG_IPV6) && defined(IP_PORT_FILTER)
int ipPortFilterListV6(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;

	unsigned int entryNum, i;
	MIB_CE_V6_IP_PORT_FILTER_T Entry;
	const char *dir, *ract;
	char	*type, *ip;
	unsigned char ip6StartStr[INET6_ADDRSTRLEN], ip6EndStr[INET6_ADDRSTRLEN];
	char	ipaddr[100], portRange[20];
	char wanname[IFNAMSIZ];

	entryNum = mib_chain_total(MIB_V6_IP_PORT_FILTER_TBL);

	nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
    "<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
    "<td align=center width=\"15%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
    "<td align=center width=\"15%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
#ifndef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
    "<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s %s</b></font></td>\n"
#else
    "<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s %s/%s</b></font></td>\n"
#endif
    "<td align=center width=\"15%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
#ifndef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
    "<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s %s</b></font></td>\n"
#else
    "<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s %s/%s</b></font></td>\n"
#endif
    "<td align=center width=\"15%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
    "<td align=center width=\"15%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td></tr>\n",
#else
    "<th align=center width=\"10%%\">%s</th>\n"
    "<th align=center width=\"15%%\">%s</th>\n"
    "<th align=center width=\"15%%\">%s</th>\n"
#ifndef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
    "<th align=center width=\"20%%\">%s %s</th>\n"
#else
    "<th align=center width=\"20%%\">%s %s/%s</th>\n"
#endif
    "<th align=center width=\"15%%\">%s</th>\n"
#ifndef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
    "<th align=center width=\"20%%\">%s %s</th>\n"
#else
    "<th align=center width=\"20%%\">%s %s/%s</th>\n"
#endif
    "<th align=center width=\"15%%\">%s</th>\n"
	"<th align=center width=\"15%%\">%s</th>\n"
    "<th align=center width=\"15%%\">%s</th></tr>\n",
#endif
	multilang(LANG_SELECT), multilang(LANG_DIRECTION), multilang(LANG_PROTOCOL),

#ifndef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
	multilang(LANG_SOURCE), multilang(LANG_IP_ADDRESS), 
#else
	multilang(LANG_SOURCE), multilang(LANG_IP_ADDRESS), multilang(LANG_INTERFACE_ID), 
#endif
	multilang(LANG_SOURCE_PORT),
#ifndef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
	multilang(LANG_DESTINATION), multilang(LANG_IP_ADDRESS), 
#else
	multilang(LANG_DESTINATION), multilang(LANG_IP_ADDRESS), multilang(LANG_INTERFACE_ID), 
#endif
	multilang(LANG_DESTINATION_PORT),
#ifdef CONFIG_GENERAL_WEB
	multilang(LANG_INTERFACE),
#endif
	multilang(LANG_RULE_ACTION));
	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_V6_IP_PORT_FILTER_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

		if (Entry.dir == DIR_OUT)
			dir = Toutgoing_ippfilter;
		else
			dir = Tincoming_ippfilter;

		// Modified by Mason Yu for Block ICMP packet
		if ( Entry.protoType == PROTO_ICMP )
		{
			type = (char *)ARG_ICMPV6;
		}
		else if ( Entry.protoType == PROTO_TCP )
			type = (char *)ARG_TCP;
		else
			type = (char *)ARG_UDP;
#if 0
		ip = inet_ntoa(*((struct in_addr *)Entry.srcIp));
		if ( !strcmp(ip, "0.0.0.0"))
			ip = (char *)BLANK;
		else {
			if (Entry.smaskbit==0)
				snprintf(ipaddr, 20, "%s", ip);
			else
				snprintf(ipaddr, 20, "%s/%d", ip, Entry.smaskbit);
			ip = ipaddr;
		}
#endif

#ifndef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
		inet_ntop(PF_INET6, (struct in6_addr *)Entry.sip6Start, ip6StartStr, sizeof(ip6StartStr));
		if ( !strcmp(ip6StartStr, "::"))
			ip = (char *)BLANK;
		else {
			inet_ntop(PF_INET6, (struct in6_addr *)Entry.sip6End, ip6EndStr, sizeof(ip6EndStr));
			if ( !strcmp(ip6EndStr, "::")) {
				snprintf(ipaddr, 100, "%s/%d", ip6StartStr, Entry.sip6PrefixLen);
			}
			else {
				snprintf(ipaddr, 100, "%s-%s/%d", ip6StartStr, ip6EndStr, Entry.sip6PrefixLen);
			}
			ip = ipaddr;
		}
#else
		inet_ntop(PF_INET6, (struct in6_addr *)Entry.sIfId6Start, ip6StartStr, sizeof(ip6StartStr));
		if ( !strcmp(ip6StartStr, "::"))
		{
			ip = (char *)BLANK;

			inet_ntop(PF_INET6, (struct in6_addr *)Entry.sip6Start, ip6StartStr, sizeof(ip6StartStr));
			if ( !strcmp(ip6StartStr, "::"))
				ip = (char *)BLANK;
			else {
				inet_ntop(PF_INET6, (struct in6_addr *)Entry.sip6End, ip6EndStr, sizeof(ip6EndStr));
				if ( !strcmp(ip6EndStr, "::")) {
					if(Entry.sip6PrefixLen != 0)
						snprintf(ipaddr, 100, "%s/%d", ip6StartStr, Entry.sip6PrefixLen);
					else
						snprintf(ipaddr, 100, "%s", ip6StartStr);
				}
				else {
					if(Entry.sip6PrefixLen != 0)
						snprintf(ipaddr, 100, "%s-%s/%d", ip6StartStr, ip6EndStr, Entry.sip6PrefixLen);
					else
						snprintf(ipaddr, 100, "%s-%s", ip6StartStr,ip6EndStr);
				}
				ip = ipaddr;
			}
		}
		else {
			inet_ntop(PF_INET6, (struct in6_addr *)Entry.sIfId6End, ip6EndStr, sizeof(ip6EndStr));
			if ( !strcmp(ip6EndStr, "::")) {
				snprintf(ipaddr, 100, "%s", ip6StartStr+2);
			}
			else {
				snprintf(ipaddr, 100, "%s-%s", ip6StartStr+2, ip6EndStr+2);
			}
			ip = ipaddr;
		}

#endif
		
		if ( Entry.srcPortFrom == 0)
		{
			portRange[sizeof(portRange)-1]='\0';
			strncpy(portRange, BLANK,sizeof(portRange)-1);
		}
		else if ( Entry.srcPortFrom == Entry.srcPortTo )
			snprintf(portRange, 20, "%d", Entry.srcPortFrom);
		else
			snprintf(portRange, 20, "%d-%d", Entry.srcPortFrom, Entry.srcPortTo);

		nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>\n"
			"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n",
#else
			"<td align=center width=\"10%%\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>\n"
			"<td align=center width=\"15%%\">%s</td>\n"
      			"<td align=center width=\"15%%\">%s</td>\n"
      			"<td align=center width=\"20%%\">%s</td>\n"
      			"<td align=center width=\"15%%\">%s</td>\n",
#endif
      			i, dir, type, ip, portRange);
		
#ifndef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
		inet_ntop(PF_INET6, (struct in6_addr *)Entry.dip6Start, ip6StartStr, sizeof(ip6StartStr));
		if ( !strcmp(ip6StartStr, "::"))
			ip = (char *)BLANK;
		else {
			inet_ntop(PF_INET6, (struct in6_addr *)Entry.dip6End, ip6EndStr, sizeof(ip6EndStr));
			if ( !strcmp(ip6EndStr, "::")) {
				snprintf(ipaddr, 100, "%s/%d", ip6StartStr, Entry.dip6PrefixLen);
			}
			else {
				snprintf(ipaddr, 100, "%s-%s/%d", ip6StartStr, ip6EndStr, Entry.dip6PrefixLen);
			}
			ip = ipaddr;
		}
#else
		inet_ntop(PF_INET6, (struct in6_addr *)Entry.dIfId6Start, ip6StartStr, sizeof(ip6StartStr));
		if ( !strcmp(ip6StartStr, "::"))
		{
			ip = (char *)BLANK;

			inet_ntop(PF_INET6, (struct in6_addr *)Entry.dip6Start, ip6StartStr, sizeof(ip6StartStr));
			if ( !strcmp(ip6StartStr, "::"))
				ip = (char *)BLANK;
			else {
				inet_ntop(PF_INET6, (struct in6_addr *)Entry.dip6End, ip6EndStr, sizeof(ip6EndStr));
				if ( !strcmp(ip6EndStr, "::")) {
					if(Entry.dip6PrefixLen != 0)
						snprintf(ipaddr, 100, "%s/%d", ip6StartStr, Entry.dip6PrefixLen);
					else
						snprintf(ipaddr, 100, "%s", ip6StartStr);
				}
				else {
					if(Entry.dip6PrefixLen != 0)
						snprintf(ipaddr, 100, "%s-%s/%d", ip6StartStr, ip6EndStr, Entry.dip6PrefixLen);
					else
						snprintf(ipaddr, 100, "%s-%s", ip6StartStr, ip6EndStr);
				}
				ip = ipaddr;
			}
		}
		else {
			inet_ntop(PF_INET6, (struct in6_addr *)Entry.dIfId6End, ip6EndStr, sizeof(ip6EndStr));
			if ( !strcmp(ip6EndStr, "::")) {
				snprintf(ipaddr, 100, "%s", ip6StartStr+2);
			}
			else {
				snprintf(ipaddr, 100, "%s-%s", ip6StartStr+2, ip6EndStr+2);
			}
			ip = ipaddr;
		}

#endif
		
		if ( Entry.dstPortFrom == 0)
		{
			portRange[sizeof(portRange)-1]='\0';
			strncpy(portRange, BLANK,sizeof(portRange)-1);
		}
		else if ( Entry.dstPortFrom == Entry.dstPortTo )
			snprintf(portRange, 20, "%d", Entry.dstPortFrom);
		else
			snprintf(portRange, 20, "%d-%d", Entry.dstPortFrom, Entry.dstPortTo);

		if(Entry.ifIndex==DUMMY_IFINDEX){
			wanname[sizeof(wanname)-1]='\0';
			strncpy(wanname, "any",sizeof(wanname)-1);
		}
		else{
			ifGetName(Entry.ifIndex, wanname, sizeof(wanname));
		}
		
		if ( Entry.action == 0 )
			ract = Tdeny_ippfilter;
		else
			ract = Tallow_ippfilter;
		nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
			"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td></tr>\n",
#else
			"<td align=center width=\"20%%\">%s</td>\n"
      			"<td align=center width=\"15%%\">%s</td>\n"
      			"<td align=center width=\"15%%\">%s</td>\n"
      			"<td align=center width=\"15%%\">%s</td></tr>\n",
#endif
//      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				ip, portRange, 
#ifdef CONFIG_GENERAL_WEB
				wanname,
#endif
				ract);
	}

	return nBytesSent;
}

void formFilterV6(request * wp, char *path, char *query)
{	
	char *strSetDefaultAction;
	char *strAddIpPort, *strDelIpPort;
	char *strDelAllIpPort, *strVal, *submitUrl, *strComment;	
	unsigned char vChar;
	char tmpBuf[100];	
	unsigned int totalEntry;
	
	memset(tmpBuf,0x00,100);
	
	strSetDefaultAction = boaGetVar(wp, "setDefaultAction", "");
	strAddIpPort = boaGetVar(wp, "addFilterIpPort", "");
	strDelIpPort = boaGetVar(wp, "deleteSelFilterIpPort", "");
	strDelAllIpPort = boaGetVar(wp, "deleteAllFilterIpPort", "");

	totalEntry = mib_chain_total(MIB_V6_IP_PORT_FILTER_TBL); /* get chain record size */

	// delete ALL
	if(strDelAllIpPort[0])
	{
		mib_chain_clear(MIB_V6_IP_PORT_FILTER_TBL); /* clear chain record */
		goto setOk_filter;
	}

	// delete select
	if(strDelIpPort[0])
	{
		unsigned int i;
		unsigned int idx;
		unsigned int deleted = 0;
		for (i=0; i<totalEntry; i++) {

			idx = totalEntry-i-1;
			snprintf(tmpBuf, 20, "select%d", idx);
			strVal = boaGetVar(wp, tmpBuf, "");

			if ( !gstrcmp(strVal, "ON") ) {
				deleted ++;
				if(mib_chain_delete(MIB_V6_IP_PORT_FILTER_TBL, idx) != 1) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
					goto setErr_filter;
				}
			}
		}
		if (deleted <= 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strNoItemSelectedToDelete,sizeof(tmpBuf)-1);
			goto setErr_filter;
		}
		goto setOk_filter;
	}
	
	// Set IP filtering default action
	if (strSetDefaultAction[0])
	{
		strVal = boaGetVar(wp, "outAct", "");
		if ( strVal[0] ) {
			vChar = strVal[0] - '0';
			mib_set( MIB_V6_IPF_OUT_ACTION, (void *)&vChar);
		}

		strVal = boaGetVar(wp, "inAct", "");
		if ( strVal[0] ) {
			vChar = strVal[0] - '0';
			mib_set( MIB_V6_IPF_IN_ACTION, (void *)&vChar);
		}
		goto setOk_filter;
	}
	
	if (totalEntry >= MAX_FILTER_NUM)
	{
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
		goto setErr_filter;
	}

	// Add a new rule
	if (strAddIpPort[0] ) {		// IP/Port FILTER
		MIB_CE_V6_IP_PORT_FILTER_T filterEntry;		
		char *strFrom, *strTo;
		int intVal;
		unsigned int totalEntry;
		MIB_CE_V6_IP_PORT_FILTER_T Entry;
		int i;		

		memset(&filterEntry, 0x00, sizeof(filterEntry));	

		// protocol
		strVal = boaGetVar(wp, "protocol", "");

		if (!strVal[0]) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tprotocol_empty,sizeof(tmpBuf)-1);
			goto setErr_filter;
		}

		filterEntry.protoType = strVal[0] - '0';
		
		// source port
		strFrom = boaGetVar(wp, "sfromPort", "");
		strTo = boaGetVar(wp, "stoPort", "");		
		
		if (filterEntry.protoType != PROTO_TCP && filterEntry.protoType != PROTO_UDP){
			strFrom = 0;
		}
		
		if(strFrom!= NULL && strFrom[0])
		{
			int intVal;
			if ( !string_to_dec(strFrom, &intVal) || intVal<1 || intVal>65535) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tinvalid_source_port,sizeof(tmpBuf)-1);
				goto setErr_filter;
			}
			filterEntry.srcPortFrom = (unsigned short)intVal;


			if ( !strTo[0] )
				filterEntry.srcPortTo = filterEntry.srcPortFrom;
			else {
				if ( !string_to_dec(strTo, &intVal) || intVal<1 || intVal>65535) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tinvalid_source_port,sizeof(tmpBuf)-1);
					goto setErr_filter;
				}
				filterEntry.srcPortTo = (unsigned short)intVal;
			}

			if ( filterEntry.srcPortFrom  > filterEntry.srcPortTo ) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tinvalid_port_range,sizeof(tmpBuf)-1);
				goto setErr_filter;
			}
		}
		
		// destination port
		strFrom = boaGetVar(wp, "dfromPort", "");
		strTo = boaGetVar(wp, "dtoPort", "");
		
		if (filterEntry.protoType != PROTO_TCP && filterEntry.protoType != PROTO_UDP){
			strFrom = 0;
		}
		
		if(strFrom!= NULL && strFrom[0])
		{
			int intVal;
			if ( !string_to_dec(strFrom, &intVal) || intVal<1 || intVal>65535) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tinvalid_destination_port,sizeof(tmpBuf)-1);
				goto setErr_filter;
			}
			filterEntry.dstPortFrom = (unsigned short)intVal;


			if ( !strTo[0] )
				filterEntry.dstPortTo = filterEntry.dstPortFrom;
			else {
				if ( !string_to_dec(strTo, &intVal) || intVal<1 || intVal>65535) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tinvalid_destination_port,sizeof(tmpBuf)-1);
					goto setErr_filter;
				}
				filterEntry.dstPortTo = (unsigned short)intVal;
			}

			if ( filterEntry.dstPortFrom  > filterEntry.dstPortTo ) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tinvalid_port_range,sizeof(tmpBuf)-1);
				goto setErr_filter;
			}
		}
		

		// Source IP
		strVal = boaGetVar(wp, "sip6Start", "");
		if (strVal[0]) {
			if (!inet_pton(PF_INET6, strVal, &filterEntry.sip6Start)) {
				sprintf(tmpBuf, "%s (sip6start)", strInvalidValue);
				goto setErr_filter;
			}
		}
		
		strVal = boaGetVar(wp, "sip6End", "");
		if (strVal[0]) {
			if (!inet_pton(PF_INET6, strVal, &filterEntry.sip6End)) {
				sprintf(tmpBuf, "%s (sip6end)", strInvalidValue);
				goto setErr_filter;
			}
		}
		
		// destination IP
		strVal = boaGetVar(wp, "dip6Start", "");
		if (strVal[0]) {
			if (!inet_pton(PF_INET6, strVal, &filterEntry.dip6Start)) {
				sprintf(tmpBuf, "%s (dip6start)", strInvalidValue);
				goto setErr_filter;
			}
		}
		
		strVal = boaGetVar(wp, "dip6End", "");
		if (strVal[0]) {
			if (!inet_pton(PF_INET6, strVal, &filterEntry.dip6End)) {
				sprintf(tmpBuf, "%s (dip6end)", strInvalidValue);
				goto setErr_filter;
			}
		}
		
		// source PrefixLen
		strVal = boaGetVar(wp, "sip6PrefixLen", "");
		filterEntry.sip6PrefixLen = (char)atoi(strVal);
		
		// destination PrefixLen
		strVal = boaGetVar(wp, "dip6PrefixLen", "");
		filterEntry.dip6PrefixLen = (char)atoi(strVal);
#ifdef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER

		// Source IfId
		strVal = boaGetVar(wp, "sIfId6Start", "");
		if (strVal[0]) {
			snprintf(tmpBuf,sizeof(tmpBuf),"::%s",strVal);
			if (!inet_pton(PF_INET6, tmpBuf, &filterEntry.sIfId6Start)) {
				sprintf(tmpBuf, "%s (dip6start)", strInvalidValue);
				goto setErr_filter;
			}
		}
		
		strVal = boaGetVar(wp, "sIfId6End", "");
		if (strVal[0]) {
			snprintf(tmpBuf,sizeof(tmpBuf),"::%s",strVal);
			if (!inet_pton(PF_INET6, tmpBuf, &filterEntry.sIfId6End)) {
				sprintf(tmpBuf, "%s (dip6end)", strInvalidValue);
				goto setErr_filter;
			}
		}
		
		// destination IfId
		strVal = boaGetVar(wp, "dIfId6Start", "");
		if (strVal[0]) {
			snprintf(tmpBuf,sizeof(tmpBuf),"::%s",strVal);
			if (!inet_pton(PF_INET6, tmpBuf, &filterEntry.dIfId6Start)) {
				sprintf(tmpBuf, "%s (dip6start)", strInvalidValue);
				goto setErr_filter;
			}
		}
		
		strVal = boaGetVar(wp, "dIfId6End", "");
		if (strVal[0]) {
			snprintf(tmpBuf,sizeof(tmpBuf),"::%s",strVal);
			if (!inet_pton(PF_INET6, tmpBuf, &filterEntry.dIfId6End)) {
				sprintf(tmpBuf, "%s (dip6end)", strInvalidValue);
				goto setErr_filter;
			}
		}
#endif
				
		strVal = boaGetVar(wp, "filterMode", "");
		if ( strVal[0] ) {
			if (!strcmp(strVal, "Deny"))
				filterEntry.action = 0;
			else if (!strcmp(strVal, "Allow"))
				filterEntry.action = 1;
			else {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tinvalid_rule_action,sizeof(tmpBuf)-1);
				goto setErr_filter;
			}
		}

		strVal = boaGetVar(wp, "dir", "");
		if(strVal[0])
			filterEntry.dir = strVal[0]-'0';		

		strVal = boaGetVar(wp, "wanif", "");
		if(strVal[0]){
			filterEntry.ifIndex = atoi(strVal);
		}
		else{
			filterEntry.ifIndex = DUMMY_IFINDEX;
		}
		
		intVal = mib_chain_add(MIB_V6_IP_PORT_FILTER_TBL, (unsigned char*)&filterEntry);
		if (intVal == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tadd_chain_error,sizeof(tmpBuf)-1);
			goto setErr_filter;
		}
		else if (intVal == -1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
			goto setErr_filter;
		}

	}
	
setOk_filter:
//	Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif	
	
	restart_IPFilter_DMZ_MACFilter();
	
#ifndef NO_ACTION
	pid = fork();
	if (pid) {
		waitpid(pid, NULL, 0);
	}
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
		exit(1);
	}
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
  return;

setErr_filter:
	ERR_MSG(tmpBuf);
}

#endif

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

#ifdef CONFIG_IPV6
#ifdef DHCPV6_ISC_DHCP_4XX

/*	Return Value:
 *		-1:	setting fail
 *		 0:	NONE
 *		 1:	setting success
 */
int setNtpServer(request * wp, char *tmpBuf)
{
	char *str, *strVal;
	// Delete all NTP Server
	str = boaGetVar(wp, "delAllNTPServer", "");
	if (str[0]) {
		mib_chain_clear(MIB_DHCPV6S_NTP_SERVER_TBL); /* clear chain record */
		return 1;
	}

	/* Delete selected NTP Server */
	str = boaGetVar(wp, "delNTPServer", "");
	if (str[0]) {
		unsigned int i;
		unsigned int idx;
		unsigned int totalEntry = mib_chain_total(MIB_DHCPV6S_NTP_SERVER_TBL); /* get chain record size */
		unsigned int deleted = 0;

		for (i=0; i<totalEntry; i++) {

			idx = totalEntry-i-1;
			snprintf(tmpBuf, 20, "select%d", idx);
			strVal = boaGetVar(wp, tmpBuf, "");

			if ( !gstrcmp(strVal, "ON") ) {
				deleted ++;
				if(mib_chain_delete(MIB_DHCPV6S_NTP_SERVER_TBL, idx) != 1) {
					strcpy(tmpBuf, Tdelete_chain_error);
					return -1;
				}
			}
		}
		if (deleted <= 0) {
			sprintf(tmpBuf,"%s (MIB_DHCPV6S_NTP_SERVER_TBL)",strNoItemSelectedToDelete);
			return -1;
		}

		return 1;
	}

	// Add Name server
	str = boaGetVar(wp, "addNTPServer", "");
	if (str[0]) {
		MIB_DHCPV6S_NTP_SERVER_T entry;
		int i, intVal;
		unsigned int totalEntry = mib_chain_total(MIB_DHCPV6S_NTP_SERVER_TBL); /* get chain record size */

		str = boaGetVar(wp, "NTPServerIP", "");
		//printf("formDOMAINBLK:(Add) str = %s\n", str);
		// Jenny, check duplicated rule
		for (i = 0; i< totalEntry; i++) {
			if (!mib_chain_get(MIB_DHCPV6S_NTP_SERVER_TBL, i, (void *)&entry)) {
				strcpy(tmpBuf, errGetEntry);
				return -1;
			}
			if (!strcmp(entry.ntpServer, str)) {
				strcpy(tmpBuf, strMACInList );
				return -1;
			}
		}

		// add into configuration (chain record)
		entry.ntpServer[sizeof(entry.ntpServer)-1]='\0';
		strncpy(entry.ntpServer, str,sizeof(entry.ntpServer)-1);

		intVal = mib_chain_add(MIB_DHCPV6S_NTP_SERVER_TBL, (unsigned char*)&entry);
		if (intVal == 0) {
			sprintf(tmpBuf, "%s (NTP Server)",Tadd_chain_error);
			return -1;
		}
		else if (intVal == -1) {
			strcpy(tmpBuf, strTableFull);
			return -1;
		}
	}

	return 0;
}

/*	Return Value:
 *		-1:	setting fail
 *		 0:	NONE
 *		 1:	setting success
 */
int setMacBinding(request * wp, char *tmpBuf)
{
	char *str, *strVal;
	// Delete all mac binding
	str = boaGetVar(wp, "delAllMacBinding", "");
	if (str[0]) {
		mib_chain_clear(MIB_DHCPV6S_MAC_BINDING_TBL); /* clear chain record */
		return 1;
	}

	/* Delete selected mac binding */
	str = boaGetVar(wp, "delMacBinding", "");
	if (str[0]) {
		unsigned int i;
		unsigned int idx;
		unsigned int totalEntry = mib_chain_total(MIB_DHCPV6S_MAC_BINDING_TBL); /* get chain record size */
		unsigned int deleted = 0;

		for (i=0; i<totalEntry; i++) {

			idx = totalEntry-i-1;
			snprintf(tmpBuf, 20, "select%d", idx);
			strVal = boaGetVar(wp, tmpBuf, "");

			if ( !gstrcmp(strVal, "ON") ) {
				deleted ++;
				if(mib_chain_delete(MIB_DHCPV6S_MAC_BINDING_TBL, idx) != 1) {
					strcpy(tmpBuf, Tdelete_chain_error);
					return -1;
				}
			}
		}
		if (deleted <= 0) {
			sprintf(tmpBuf,"%s (MIB_DHCPV6S_MAC_BINDING_TBL)",strNoItemSelectedToDelete);
			return -1;
		}

		return 1;
	}

	// Add Name server
	str = boaGetVar(wp, "addMacBinding", "");
	if (str[0]) {
		MIB_DHCPV6S_MAC_BINDING_T entry;
		MIB_DHCPV6S_MAC_BINDING_T entry_tmp;
		int i, intVal;
		unsigned int totalEntry = mib_chain_total(MIB_DHCPV6S_MAC_BINDING_TBL); /* get chain record size */

		str = boaGetVar(wp, "hostName", "");
		if (!str[0]) {
			strcpy(tmpBuf, strEmptyHostName);
			return -1;
		}
		//printf("formDOMAINBLK:(Add) str = %s\n", str);
		// Jenny, check duplicated rule
		for (i = 0; i< totalEntry; i++) {
			if (!mib_chain_get(MIB_DHCPV6S_MAC_BINDING_TBL, i, (void *)&entry_tmp)) {
				strcpy(tmpBuf, errGetEntry);
				return -1;
			}
			if (!strcmp(entry_tmp.hostName, str)) {
				strcpy(tmpBuf, strMACInList );
				return -1;
			}
		}
		// add into configuration (chain record)
		entry.hostName[sizeof(entry.hostName)-1]='\0';
		strncpy(entry.hostName, str,sizeof(entry.hostName)-1);
		
		str = boaGetVar(wp, "macAddress", "");
		if (str[0]) {
			if(!isValidMacString(str)){
				strcpy(tmpBuf, strInvaidMacAddress);
				return -1;
			}

			rtk_string_to_hex(str, entry.macAddr, 12);
			if (!isValidMacAddr(entry.macAddr)) {
				strcpy(tmpBuf, strInvaidMacAddress);
				return -1;
			}
		}
		else{
			strcpy(tmpBuf, strInvaidMacAddress);
			return -1;
		}
		for (i = 0; i< totalEntry; i++) {
			if (!mib_chain_get(MIB_DHCPV6S_MAC_BINDING_TBL, i, (void *)&entry_tmp)) {
				strcpy(tmpBuf, errGetEntry);
				return -1;
			}
			if (!memcmp(entry_tmp.macAddr, entry.macAddr, 6)) {
				strcpy(tmpBuf, strMACInList );
				return -1;
			}
		}

		str = boaGetVar(wp, "ipAddress", "");
		if(str[0]){
			if(rtk_util_ip_version(str)!=6){
				strcpy(tmpBuf, strInvalidIpv6Addr);
				return -1;
			}
			strcpy(entry.ipAddress, str);
		}
		else{
			strcpy(tmpBuf, strInvalidIpv6Addr);
			return -1;
		}
		intVal = mib_chain_add(MIB_DHCPV6S_MAC_BINDING_TBL, (unsigned char*)&entry);
		if (intVal == 0) {
			sprintf(tmpBuf, "%s (MAC Binding)",Tadd_chain_error);
			return -1;
		}
		else if (intVal == -1) {
			strcpy(tmpBuf, strTableFull);
			return -1;
		}
	}

	return 0;
}


void formDhcpv6(request * wp, char *path, char *query)
{
	char *submitUrl, *str, *strVal;
	static char tmpBuf[100];
	unsigned char origvChar;
	unsigned int origInt;
	int dhcpd_changed_flag=0;
	char *strdhcpenable;
	char origDhcpv6Type=0, newDhcpv6Type=0, *strdhcpetype=NULL;
	unsigned char mode;
	DHCP_TYPE_T curDhcp;
	DHCPV6S_TYPE_T curDhcpType;
	unsigned char prefixLen;
	struct in6_addr ip6Addr_start, ip6Addr_end,ip6Addr;
	int ret = 0;
	PREFIX_V6_INFO_T prefixInfo;
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
	PREFIX_V6_INFO_T lan_prefixInfo = {0};
#endif
	
	//Init
	memset(&ip6Addr_start, 0, sizeof(struct in6_addr));
	memset(&ip6Addr_end, 0, sizeof(struct in6_addr));
	memset(&ip6Addr, 0 ,sizeof(struct in6_addr));
	memset(&prefixInfo, 0 ,sizeof(PREFIX_V6_INFO_T));

	//Clear old static IPv6 info in br0
	if ( !mib_get_s( MIB_DHCPV6_MODE, (void *)&origvChar, sizeof(origvChar)) ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, strGetDhcpModeerror,sizeof(tmpBuf)-1);
		goto setErr_dhcpv6;
	}
	if ( !mib_get_s( MIB_DHCPV6S_TYPE, (void *)&origDhcpv6Type, sizeof(origDhcpv6Type)) ) {
		snprintf(tmpBuf,sizeof(tmpBuf), "Get dhcpv6 type err!");
		goto setErr_dhcpv6;
	}

	if ((origvChar == DHCP_LAN_SERVER) && (origDhcpv6Type == DHCPV6S_TYPE_STATIC)){
		mib_get_s(MIB_DHCPV6S_RANGE_START, (void *)ip6Addr.s6_addr, sizeof(ip6Addr.s6_addr));
		mib_get_s(MIB_DHCPV6S_PREFIX_LENGTH, (void *)&prefixLen, sizeof(prefixLen));
		memcpy(prefixInfo.prefixIP, &ip6Addr, IP6_ADDR_LEN);
		prefixInfo.prefixLen = prefixLen;
		//del dhcpv6 static prefix address
		set_lan_ipv6_global_address(NULL,&prefixInfo,(char*)LANIF,0,0);
		//del dhcpv6 static prefix route for LAN route table(IP_ROUTE_LAN_TABLE)
		set_ipv6_lan_policy_route((char *)LANIF, (struct in6_addr *)&prefixInfo.prefixIP, prefixInfo.prefixLen, 0);
	}else if (origvChar == DHCP_LAN_RELAY){
		char upper_ifname[8];
		unsigned int upper_if;
		MIB_CE_ATM_VC_T Entry;
		if ( !mib_get_s(MIB_DHCPV6R_UPPER_IFINDEX, (void *)&upper_if, sizeof(upper_if))) {
			snprintf(tmpBuf, sizeof(tmpBuf),"Get MIB_DHCPV6R_UPPER_IFINDEX mib error!");
			goto setErr_dhcpv6;
		}		
	
		if ( upper_if != DUMMY_IFINDEX ){
			ifGetName(upper_if, upper_ifname, sizeof(upper_ifname));
		}
		else{
			printf("Error: The upper interface of dhcrelayV6 not set !");
		}
	
		if (!(getATMVCEntryByIfIndex(upper_if, &Entry))){
			printf("Error! Could not get MIB entry info from wanconn\n");
		}
	
		if (get_prefixv6_info_by_wan(&prefixInfo, upper_if)!=0){
			printf("Error:get prefix info from wan %d failed\n",upper_if);
		}
		if(set_lan_ipv6_global_address(NULL,&prefixInfo,(char*)LANIF,0,0)!=0){
			printf("Error:del br0 global address failed\n");
		}
	}
	//Set New
	strdhcpenable = boaGetVar(wp, "dhcpdenable", "");

	if(strdhcpenable[0])
	{
		sscanf(strdhcpenable, "%u", &origInt);
		mode = (unsigned char)origInt;
		if(mode!=origvChar)
			dhcpd_changed_flag = 1;
		if ( !mib_set(MIB_DHCPV6_MODE, (void *)&mode)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
  			strncpy(tmpBuf, strSetDhcpModeerror,sizeof(tmpBuf)-1);
			goto setErr_dhcpv6;
		}

		mib_get_s( MIB_DHCPV6S_TYPE, (void *)&origDhcpv6Type, sizeof(origDhcpv6Type));
		strdhcpetype = boaGetVar(wp, "dhcpdv6Type", "");
		if(strdhcpetype[0])
		{
			newDhcpv6Type = (*strdhcpetype - '0');
			if(origDhcpv6Type != newDhcpv6Type)
				dhcpd_changed_flag = 1;

			if ( !mib_set(MIB_DHCPV6S_TYPE, (void *)&newDhcpv6Type)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetDhcpModeerror,sizeof(tmpBuf)-1);
				goto setErr_dhcpv6;
			}
		}
	}

	if ( !mib_get_s( MIB_DHCPV6_MODE, (void *)&origvChar, sizeof(origvChar)) ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, strGetDhcpModeerror,sizeof(tmpBuf)-1);
		goto setErr_dhcpv6;
	}
	curDhcp = (DHCP_TYPE_T) origvChar;

	if ( !mib_get_s( MIB_DHCPV6S_TYPE, (void *)&origDhcpv6Type, sizeof(origDhcpv6Type))){
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, strGetDhcpModeerror,sizeof(tmpBuf)-1);
		goto setErr_dhcpv6;
	}

	if ( curDhcp == DHCP_LAN_SERVER ) {
		if(origDhcpv6Type == DHCPV6S_TYPE_AUTO){
			ret = setNtpServer(wp, tmpBuf);
			if(ret==-1){
				goto setErr_dhcpv6;
			}
			else if(ret==1){
				goto setOk_dhcpv6;
			}

			ret = setMacBinding(wp, tmpBuf);
			if(ret==-1){
				goto setErr_dhcpv6;
			}
			else if(ret==1){
				goto setOk_dhcpv6;
			}
		}else if(origDhcpv6Type == DHCPV6S_TYPE_STATIC){
			unsigned int DLTime, PFTime, RNTime, RBTime;
			
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
			//single br should delete default prefix from br0, br0 only accept 1 global IP.
			if(get_prefixv6_info(&lan_prefixInfo) == 0)
				set_lan_ipv6_global_address(NULL, &lan_prefixInfo, (char *)BRIF, 0, 0);
#endif
			str = boaGetVar(wp, "save", "");
			if (str[0]) {
				str = boaGetVar(wp, "dhcpRangeStart", "");
				if (!inet_pton(PF_INET6, str, &ip6Addr_start)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tinvalid_start_ip,sizeof(tmpBuf)-1);
					goto setErr_dhcpv6;
				}

				str = boaGetVar(wp, "dhcpRangeEnd", "");
				if (!inet_pton(PF_INET6, str, &ip6Addr_end)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tinvalid_end_ip,sizeof(tmpBuf)-1);
					goto setErr_dhcpv6;
				}

				str = boaGetVar(wp, "prefix_len", "");
				prefixLen = (char)atoi(str);

				str = boaGetVar(wp, "Dltime", "");
				sscanf(str, "%u", &DLTime);

				str = boaGetVar(wp, "PFtime", "");
				sscanf(str, "%u", &PFTime);

				str = boaGetVar(wp, "RNtime", "");
				sscanf(str, "%u", &RNTime);

				str = boaGetVar(wp, "RBtime", "");
				sscanf(str, "%u", &RBTime);

				str = boaGetVar(wp, "clientID", "");
				if (strContainXSSChar(str)){
					printf("%s:%d String %s contains invalid words causing XSS attack!\n",__FUNCTION__,__LINE__,str);
					snprintf(tmpBuf, sizeof(tmpBuf),"invalid string %s (XSS attack)", str);
					goto setErr_dhcpv6;
				}

				// Everything is ok, so set it.
				mib_set(MIB_DHCPV6S_RANGE_START, (void *)&ip6Addr_start);
				mib_set(MIB_DHCPV6S_RANGE_END, (void *)&ip6Addr_end);

				mib_set(MIB_DHCPV6S_PREFIX_LENGTH, (char *)&prefixLen);
				mib_set(MIB_DHCPV6S_DEFAULT_LEASE, (void *)&DLTime);
				mib_set(MIB_DHCPV6S_PREFERRED_LIFETIME, (void *)&PFTime);
				mib_set(MIB_DHCPV6S_RENEW_TIME, (void *)&RNTime);
				mib_set(MIB_DHCPV6S_REBIND_TIME, (void *)&RBTime);
				mib_set(MIB_DHCPV6S_CLIENT_DUID, (void *)str);
			} // of save

			// Delete all Name Server
			str = boaGetVar(wp, "delAllNameServer", "");
			if (str[0]) {
				mib_chain_clear(MIB_DHCPV6S_NAME_SERVER_TBL); /* clear chain record */
				goto setOk_dhcpv6;
			}

			/* Delete selected Name Server */
			str = boaGetVar(wp, "delNameServer", "");
			if (str[0]) {
				unsigned int i;
				unsigned int idx;
				unsigned int totalEntry = mib_chain_total(MIB_DHCPV6S_NAME_SERVER_TBL); /* get chain record size */
				unsigned int deleted = 0;

				for (i=0; i<totalEntry; i++) {

					idx = totalEntry-i-1;
					snprintf(tmpBuf, 20, "select%d", idx);
					strVal = boaGetVar(wp, tmpBuf, "");

					if ( !gstrcmp(strVal, "ON") ) {
						deleted ++;
						if(mib_chain_delete(MIB_DHCPV6S_NAME_SERVER_TBL, idx) != 1) {
							tmpBuf[sizeof(tmpBuf)-1]='\0';
							strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
							goto setErr_dhcpv6;
						}
					}
				}
				if (deleted <= 0) {
					sprintf(tmpBuf,"%s (MIB_DHCPV6S_NAME_SERVER_TBL)",strNoItemSelectedToDelete);
					goto setErr_dhcpv6;
				}

				goto setOk_dhcpv6;
			}

			// Add Name server
			str = boaGetVar(wp, "addNameServer", "");
			if (str[0]) {
				MIB_DHCPV6S_NAME_SERVER_T entry;
				int i, intVal;
				unsigned int totalEntry = mib_chain_total(MIB_DHCPV6S_NAME_SERVER_TBL); /* get chain record size */

				str = boaGetVar(wp, "nameServerIP", "");
				//printf("formDOMAINBLK:(Add) str = %s\n", str);
				// Jenny, check duplicated rule
				for (i = 0; i< totalEntry; i++) {
					if (!mib_chain_get(MIB_DHCPV6S_NAME_SERVER_TBL, i, (void *)&entry)) {
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, errGetEntry,sizeof(tmpBuf)-1);
						goto setErr_dhcpv6;
					}
					if (!strcmp(entry.nameServer, str)) {
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, strMACInList ,sizeof(tmpBuf)-1);
						goto setErr_dhcpv6;
					}
				}

				// add into configuration (chain record)
				entry.nameServer[sizeof(entry.nameServer)-1]='\0';
				strncpy(entry.nameServer, str,sizeof(entry.nameServer)-1);

				intVal = mib_chain_add(MIB_DHCPV6S_NAME_SERVER_TBL, (unsigned char*)&entry);
				if (intVal == 0) {
					//boaWrite(wp, "%s", "Error: Add Domain Blocking chain record.");
					//return;
					sprintf(tmpBuf, "%s (Name Server)",Tadd_chain_error);
					goto setErr_dhcpv6;
				}
				else if (intVal == -1) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
					goto setErr_dhcpv6;
				}

			}

			// Delete all Domain
			str = boaGetVar(wp, "delAllDomain", "");
			if (str[0]) {
				mib_chain_clear(MIB_DHCPV6S_DOMAIN_SEARCH_TBL); /* clear chain record */
				goto setOk_dhcpv6;
			}

			/* Delete selected Domain */
			str = boaGetVar(wp, "delDomain", "");
			if (str[0]) {
				unsigned int i;
				unsigned int idx;
				unsigned int totalEntry = mib_chain_total(MIB_DHCPV6S_DOMAIN_SEARCH_TBL); /* get chain record size */
				unsigned int deleted = 0;

				for (i=0; i<totalEntry; i++) {

					idx = totalEntry-i-1;
					snprintf(tmpBuf, 20, "select%d", idx);
					strVal = boaGetVar(wp, tmpBuf, "");

					if ( !gstrcmp(strVal, "ON") ) {
						deleted ++;
						if(mib_chain_delete(MIB_DHCPV6S_DOMAIN_SEARCH_TBL, idx) != 1) {
							tmpBuf[sizeof(tmpBuf)-1]='\0';
							strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
							goto setErr_dhcpv6;
						}
					}
				}
				if (deleted <= 0) {
					sprintf(tmpBuf,"%s (MIB_DHCPV6S_DOMAIN_SEARCH_TBL)",strNoItemSelectedToDelete);
					goto setErr_dhcpv6;
				}

				goto setOk_dhcpv6;
			}

			// Add doamin
			str = boaGetVar(wp, "addDomain", "");
			if (str[0]) {
				MIB_DHCPV6S_DOMAIN_SEARCH_T entry;
				int i, intVal;
				unsigned int totalEntry = mib_chain_total(MIB_DHCPV6S_DOMAIN_SEARCH_TBL); /* get chain record size */

				str = boaGetVar(wp, "domainStr", "");

				if (strContainXSSChar(str)){
					printf("%s:%d String %s contains invalid words causing XSS attack!\n",__FUNCTION__,__LINE__,str);
					snprintf(tmpBuf, sizeof(tmpBuf),"invalid string %s (XSS attack)", str);
					goto setErr_dhcpv6;
				}
				//printf("formDOMAINBLK:(Add) str = %s\n", str);
				// Jenny, check duplicated rule
				for (i = 0; i< totalEntry; i++) {
					if (!mib_chain_get(MIB_DHCPV6S_DOMAIN_SEARCH_TBL, i, (void *)&entry)) {
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, errGetEntry,sizeof(tmpBuf)-1);
						goto setErr_dhcpv6;
					}
					if (!strcmp(entry.domain, str)) {
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, strMACInList ,sizeof(tmpBuf)-1);
						goto setErr_dhcpv6;
					}
				}

				// add into configuration (chain record)
				entry.domain[sizeof(entry.domain)-1]='\0';
				strncpy(entry.domain, str,sizeof(entry.domain)-1);

				intVal = mib_chain_add(MIB_DHCPV6S_DOMAIN_SEARCH_TBL, (unsigned char*)&entry);
				if (intVal == 0) {
					//boaWrite(wp, "%s", "Error: Add Domain Blocking chain record.");
					//return;
					sprintf(tmpBuf, "%s (Domain chain)",Tadd_chain_error);
					goto setErr_dhcpv6;
				}
				else if (intVal == -1) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
					goto setErr_dhcpv6;
				}

			}

			ret = setNtpServer(wp, tmpBuf);
			if(ret==-1){
							goto setErr_dhcpv6;
						}
			else if(ret==1){
				goto setOk_dhcpv6;
					}
			ret = setMacBinding(wp, tmpBuf);
			if(ret==-1){
					goto setErr_dhcpv6;
				}
			else if(ret==1){
				goto setOk_dhcpv6;
			}
		}
	}
	else if( curDhcp == DHCP_LAN_RELAY ){
		unsigned int upper_if;

		str = boaGetVar(wp, "upper_if", "");
		upper_if = (unsigned int)atoi(str);

		if ( !mib_set(MIB_DHCPV6R_UPPER_IFINDEX, (void *)&upper_if)) {
			sprintf(tmpBuf, " %s (MIB_DHCPV6R_UPPER_IFINDEX).",Tset_mib_error);
			goto setErr_dhcpv6;
		}

	}
setOk_dhcpv6:
	// start dhcpd
	restart_default_dhcpv6_server();
// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

#ifndef NO_ACTION
	pid = fork();
	if (pid)
		waitpid(pid, NULL, 0);
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _CONFIG_SCRIPT_PROG);
#ifdef HOME_GATEWAY
		execl( tmpBuf, _CONFIG_SCRIPT_PROG, "gw", "bridge", NULL);
#else
		execl( tmpBuf, _CONFIG_SCRIPT_PROG, "ap", "bridge", NULL);
#endif
		exit(1);
	}
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
  return;

setErr_dhcpv6:
	// start dhcpd
	restart_default_dhcpv6_server();
	ERR_MSG(tmpBuf);
}


int showDhcpv6SNameServerTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;

	unsigned int entryNum, i;
	MIB_DHCPV6S_NAME_SERVER_T Entry;
	unsigned long int d,g,m;
	unsigned char sdest[MAX_V6_IP_LEN];

	entryNum = mib_chain_total(MIB_DHCPV6S_NAME_SERVER_TBL);

	nBytesSent += boaWrite(wp, "'<tr><font size=1>'+"
	"'<td align=center width=\"5%%\" bgcolor=\"#808080\">Select</td>'+\n"
	"'<td align=center width=\"35%%\" bgcolor=\"#808080\">Name Server</td></font></tr>'+\n");


	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_DHCPV6S_NAME_SERVER_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, "Get Name Server chain record error!\n");
			return -1;
		}

		strncpy(sdest, Entry.nameServer, sizeof(sdest));
		sdest[strlen(Entry.nameServer)] = '\0';

		nBytesSent += boaWrite(wp, "'<tr>'+"
//		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"radio\" name=\"select\""
//		" value=\"s%d\"></td>\n"
		"'<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>'+\n"
		"'<td align=center width=\"35%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td></tr>'+\n",
		i, strValToASP(sdest));
	}

	return 0;
}

int showDhcpv6SDOMAINTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;

	unsigned int entryNum, i;
	MIB_DHCPV6S_DOMAIN_SEARCH_T Entry;
	unsigned long int d,g,m;
	unsigned char sdest[MAX_DOMAIN_LENGTH];

	entryNum = mib_chain_total(MIB_DHCPV6S_DOMAIN_SEARCH_TBL);

	nBytesSent += boaWrite(wp, "'<tr><font size=1>'+"
	"'<td align=center width=\"5%%\" bgcolor=\"#808080\">Select</td>'+\n"
	"'<td align=center width=\"35%%\" bgcolor=\"#808080\">Domain</td></font></tr>'+\n");


	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_DHCPV6S_DOMAIN_SEARCH_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, "Get Domain search chain record error!\n");
			return -1;
		}

		strncpy(sdest, Entry.domain, sizeof(sdest));
		sdest[strlen(Entry.domain)] = '\0';

		nBytesSent += boaWrite(wp, "'<tr>'+"
//		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"radio\" name=\"select\""
//		" value=\"s%d\"></td>\n"
		"'<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>'+\n"
		"'<td align=center width=\"35%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td></tr>'+\n",
		i, strValToASP(sdest));
	}

	return 0;
}

int showDhcpv6SNTPServerTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;

	unsigned int entryNum, i;
	MIB_DHCPV6S_NTP_SERVER_T Entry;
	unsigned long int d,g,m;
	unsigned char sdest[MAX_V6_IP_LEN];

	entryNum = mib_chain_total(MIB_DHCPV6S_NTP_SERVER_TBL);

#ifndef CONFIG_00R0
	nBytesSent += boaWrite(wp, "<tr><font size=1>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">Select</td>\n"
	"<td align=center width=\"35%%\" bgcolor=\"#808080\">NTP Server</td></font></tr>\n");
#else
	nBytesSent += boaWrite(wp, "<tr><font size=1>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"35%%\" bgcolor=\"#808080\">%s</td></font></tr>\n",
	multilang(LANG_SELECT), multilang(LANG_NTP_SERVER));
#endif


	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_DHCPV6S_NTP_SERVER_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, "Get NTP server chain record error!\n");
			return -1;
		}

		strncpy(sdest, Entry.ntpServer, sizeof(sdest));
		sdest[strlen(Entry.ntpServer)] = '\0';

		nBytesSent += boaWrite(wp, "<tr>"
//		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"radio\" name=\"select\""
//		" value=\"s%d\"></td>\n"
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>\n"
		"<td align=center width=\"35%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td></tr>\n",
		i, strValToASP(sdest));
	}

	return 0;
}

int showDhcpv6SMacBindingTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;

	unsigned int entryNum, i;
	MIB_DHCPV6S_MAC_BINDING_T Entry;
	unsigned long int d,g,m;
	unsigned char host[MAX_V6_IP_LEN];
	unsigned char mac[MAX_V6_IP_LEN];
	unsigned char sdest[MAX_V6_IP_LEN];

	entryNum = mib_chain_total(MIB_DHCPV6S_MAC_BINDING_TBL);

#ifdef CONFIG_00R0
	nBytesSent += boaWrite(wp, "<tr><font size=1>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td></font>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td></font>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td></font></tr>\n"
	,multilang(LANG_SELECT), multilang(LANG_HOST_NAME), multilang(LANG_MAC_ADDRESS), multilang(LANG_IP_ADDRESS));

#else
	nBytesSent += boaWrite(wp, "<tr><font size=1>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">Select</td>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">Host Name</td></font>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">MAC Address</td></font>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">IP Address</td></font></tr>\n");
#endif


	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_DHCPV6S_MAC_BINDING_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, "Get MAC Binding chain record error!\n");
			return -1;
		}

		strncpy(host, Entry.hostName, sizeof(host));
		host[strlen(Entry.hostName)] = '\0';

		sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", Entry.macAddr[0],Entry.macAddr[1],Entry.macAddr[2],Entry.macAddr[3],Entry.macAddr[4],Entry.macAddr[5]);

		strncpy(sdest, Entry.ipAddress, sizeof(sdest));
		sdest[strlen(Entry.ipAddress)] = '\0';

		nBytesSent += boaWrite(wp, "<tr>"
//		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"radio\" name=\"select\""
//		" value=\"s%d\"></td>\n"
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>\n"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>\n"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>\n"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td></tr>\n",
		i, strValToASP(host), mac, sdest);
	}

	return 0;
}

int dhcpClientListv6(int eid, request * wp, int argc, char **argv)
{
#ifdef EMBED
	int nBytesSent = 0;
	int leases_size = 10;
	struct ipv6_lease active_leases[leases_size];
	int client_num=0;	
	int i;
	int ret=0;
	
	memset(&active_leases, 0, leases_size * sizeof(struct ipv6_lease));
	ret =	rtk_get_dhcpv6_lease_info(active_leases, leases_size, 0 ,&client_num);
	if ((client_num != 0) &&(ret ==0)){
		for (i = 0; i < client_num; i++) {
			if (active_leases[i].active) {
				nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
					"<tr bgcolor=#b7b7b7><font size=2><td>%s</td>"
					"<td>%s</td><td>%ld</td></font></tr>",
#else
					"<tr><td>%s</td>"
					"<td>%s</td><td>%ld</td></tr>",
#endif
					active_leases[i].iaaddr, active_leases[i].duid, active_leases[i].epoch);
			}
		}
	}

	int static_client_num=0;
	memset(&active_leases, 0, leases_size * sizeof(struct ipv6_lease));
	ret = rtk_get_dhcpv6_lease_info(active_leases, leases_size, 1 ,&static_client_num);
	if ((static_client_num != 0) &&(ret ==0)) {
		for (i = 0; i < static_client_num; i++) {
			if (active_leases[i].active) {
				nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
					"<tr bgcolor=#b7b7b7><font size=2><td>Static %s</td>"
					"<td>%s</td><td>Static</td></font></tr>",
#else
					"<tr><td>%s</td>"
					"<td>%s</td><td>Static</td></tr>",
#endif
					active_leases[i].iaaddr, active_leases[i].duid);
			}
		}
	}

	return nBytesSent;
#else
	return 0;
#endif
}
#endif
#endif  //#ifdef CONFIG_IPV6


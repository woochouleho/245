#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef EMBED
#include <linux/config.h>
#else
#include "../../../../include/linux/autoconf.h"
#endif

#include "webform.h"
#include "../webs.h"
#include "mib.h"
#include "utility.h"
#include "../defs.h"
#include "multilang.h"
#include "subr_net.h"
#include "sysconfig.h"

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN	48
#endif

#ifndef IFNAMSIZ
#define	IFNAMSIZ	16
#endif

#define _PTI			", new it(\"%s\", %d)"
#define _PTS			", new it(\"%s\", \"%s\")"
#define _PME(name)		#name, Entry.name


//IPv6 MAP-E
#ifdef CONFIG_USER_MAP_E
void initMAPE(request * wp, MIB_CE_ATM_VC_Tp entryP){
	MIB_CE_ATM_VC_T Entry;
	unsigned char	Ipv6AddrStr[INET6_ADDRSTRLEN];
	char *ipAddr;	
	char psid_str[8]={0};
	
	memcpy(&Entry, entryP, sizeof(Entry));

	boaWrite(wp, _PTI _PTI, _PME(mape_enable),  _PME(mape_mode));

#ifdef CONFIG_MAPE_ROUTING_MODE
	if (Entry.mape_forward_mode == IPV6_MAPE_NAT_MODE)
		boaWrite(wp, _PTI, "mape_forward_mode", 0);
	else if (Entry.mape_forward_mode == IPV6_MAPE_ROUTE_MODE)
		boaWrite(wp, _PTI, "mape_forward_mode", 1);
#endif	

	if (Entry.mape_enable ==1 && Entry.mape_mode == IPV6_MAPE_MODE_STATIC){
		/*DMR*/
		inet_ntop(PF_INET6, Entry.mape_remote_v6Addr, Ipv6AddrStr, sizeof(Ipv6AddrStr));
		boaWrite(wp, _PTS, "mape_remote_v6Addr", Ipv6AddrStr);

		/*BMR*/
		inet_ntop(PF_INET6, Entry.mape_local_v6Prefix, Ipv6AddrStr, sizeof(Ipv6AddrStr));
		boaWrite(wp, _PTS _PTI, "mape_local_v6Prefix", Ipv6AddrStr, "mape_local_v6PrefixLen", Entry.mape_local_v6PrefixLen);

		ipAddr = inet_ntoa(*(struct in_addr *)(&Entry.mape_local_v4Addr));
		boaWrite(wp, _PTS _PTI, "mape_local_v4Addr", ipAddr, "mape_local_v4MaskLen", Entry.mape_local_v4MaskLen);
	
		boaWrite(wp, _PTI _PTI, "mape_psid_offset", Entry.mape_psid_offset, "mape_psid_len", Entry.mape_psid_len);

		snprintf(psid_str, sizeof(psid_str), "%x", Entry.mape_psid);
		boaWrite(wp, ", new it(\"%s\", \"%s\") ", "mape_psid", psid_str);

		boaWrite(wp, _PTI, "mape_fmr_enabled", Entry.mape_fmr_enabled);
	}	
}


/*DMR: br*/
static void show_MAPE_DMR(request *wp){
	boaWrite(wp, "		<tr>\n"
			"			<td><font color=\"#A3238E\">%s</font></td>\n"
			"		</tr>\n"				
			"		<tr>\n"
			"			<th width=30%%>%s:</th>\n"
			"			<td width=70%%><input type=text name=\"mape_remote_v6Addr\" size=39 maxlength=39>(eg. 2001:0db8:ffff::1)</td>\n"
			"		</tr>\n", multilang(LANG_MAPE_DMR_SETTING),multilang(LANG_MAPE_BR_ADDRESS));	
}

/*BMR*/
static void show_MAPE_BMR(request *wp){
	boaWrite(wp, "		<tr>\n"
			"			<td><font color=\"#A3238E\">%s</font></td>\n"
			"		</tr>\n"
			"		<tr>\n"
			"			<th width=30%%>%s:</th>\n"
			"			<td width=70%%><input type=text name=\"mape_local_v6Prefix\" size=39 maxlength=39>\n"
			"				<input type=text name=\"mape_local_v6PrefixLen\" size=3 maxlength=3> (eg. 2001:0db8::/40)</td>\n"
			"		</tr>\n"
			"		<tr>\n"
			"			<th width=30%%>%s:</th>\n"
			"			<td width=70%%><input type=text name=\"mape_local_v4Addr\" size=14 maxlength=15>\n"
			"				<input type=text name=\"mape_local_v4MaskLen\" size=2 maxlength=2> (eg. 192.0.2.18/24)</td>\n"
			"		</tr>\n"													
			"		<tr>\n"
			"			<th width=30%%>%s:</th>\n"
			"			<td width=70%%><input type=text name=\"mape_psid_offset\" size=2 maxlength=2> (0-16)</td>\n"
			"		</tr>\n"			
			"		<tr>\n"
			"			<th width=30%%>%s:</th>\n"
			"			<td width=70%%><input type=text name=\"mape_psid_len\" size=2 maxlength=2> (0-16)</td>\n"
			"		</tr>\n"				
			"		<tr>\n"
			"			<th width=30%%>%s:</th>\n"
			"			<td width=70%%><input type=text name=\"mape_psid\" size=6 maxlength=6> (eg. 0x34)</td>\n"
			"		</tr>\n",
			multilang(LANG_MAPE_BMR_SETTING),multilang(LANG_MAPE_BMR_LOCAL_IPV6_PREFIX),multilang(LANG_MAPE_BMR_LOCAL_IPV4_PREFIX),
			multilang(LANG_MAPE_BMR_PSID_OFFSET),multilang(LANG_MAPE_BMR_PSID_LENGTH),multilang(LANG_MAPE_BMR_PSID_VAL));
}

/*FMR*/
static void show_MAPE_FMR(request *wp){
	boaWrite(wp,"		<tr>\n"
				"			<td width=30%%><font color=\"#A3238E\">%s</font></td>\n"
				"			<td width=70%%><input type=checkbox value=ON name=\"mape_fmr_enabled\" id=\"mape_fmr_enabled\" onClick=mapeEnableFMR()>&nbsp;&nbsp;"
				"				<input type=submit class=\"inner_btn\" value=\"Edit FMRs\" name=\"mape_fmr_list_show\" id=\"mape_fmr_list_show\" onClick=\"return mapeFmrListShow(this)\">\n"
				"			</td>\n"
				"		</tr>\n", multilang(LANG_MAPE_FMR_SETTING));
}

static void show_MAPE_FMR_edit(request *wp){
	boaWrite(wp,"<div id=mape_fmr_rule_div class=\"data_common data_common_notitle\">\n"
				"	<table>\n"
				"		<tr>\n"
				"			<th width=30%%>%s:</th>\n"
				"			<td width=70%%><input type=text name=\"mape_fmr_v6Prefix\" size=39 maxlength=39>\n "
				"				<input type=text name=\"mape_fmr_v6PrefixLen\" size=3 maxlength=3> (eg. 2003:0db8::/40)\n"
				"			</td>\n"
				"		</tr>\n"
				"		<tr>\n"
				"			<th width=30%%>%s:</th>\n"
				"			<td width=70%%><input type=text name=\"mape_fmr_v4Prefix\" size=15 maxlength=15>\n "
				"				<input type=text name=\"mape_fmr_v4MaskLen\" size=2 maxlength=2> (eg. 192.0.3.0/24)\n"
				" 			</td>\n"
				"		</tr>\n"
				"		<tr>\n"
				"			<th width=30%%>%s:</th>\n"
				"			<td width=70%%><input type=text name=\"mape_fmr_eaLen\" size=2 maxlength=2> (0-48)</td>\n"
				"		</tr>\n"			
				"		<tr>\n"
				"			<th width=30%%>%s:</th>\n"
				"			<td width=70%%><input type=text name=\"mape_fmr_psidOffset\" size=2 maxlength=2> (0-16)</td>\n"
				"		</tr>\n"
				"</table>\n"
				"</div>\n",
				multilang(LANG_MAPE_FMR_IPV6_PREFIX),multilang(LANG_MAPE_FMR_IPV4_PREFIX),
				multilang(LANG_MAPE_FMR_EA_LENGTH),multilang(LANG_MAPE_FMR_PSID_OFFSET));
}

/*FMR table*/
void show_MAPE_FMR_list(request *wp){
	int fmrsNum, fmrIdx;
	MIB_CE_MAPE_FMRS_T fmrEntry;
	unsigned char Ipv6AddrStr[INET6_ADDRSTRLEN];
	char ifname[IFNAMSIZ]={0};
	unsigned int wan_ifIndex;
	
	show_MAPE_FMR_edit(wp);
	
	//1. add, modify, delete button
	boaWrite(wp, "<div id=mape_fmr_tbl_div>\n");	
	boaWrite(wp, "<div class=\"btn_ctl\" id=fmr_edit>\n"
				"	<input type=\"submit\" value=\"%s\" name=\"addFMR\" onClick=\"return mapeAddFmrClick(this)\" class=\"link_bg\">\n"
				"	<input type=\"submit\" value=\"%s\" name=\"modifyFMR\" onClick=\"return mapeModifyFmrClick(this)\" class=\"link_bg\">\n"
				"	<input type=\"submit\" value=\"%s\" name=\"delFMR\" onClick=\"return mapeRemoveFmrClick(this)\" class=\"link_bg\">\n"
				"</div>\n",
				multilang(LANG_MAPE_FMR_ADD),multilang(LANG_MAPE_FMR_MODIFY),multilang(LANG_MAPE_FMR_DEL));	

	//2. current FMR table title
	if (!mib_get_s(MIB_MAPE_FMR_LIST_CUR_WAN_IFINDEX,(void *)&wan_ifIndex, sizeof(wan_ifIndex))){	
		boaError(wp, 400, "Get MIB_MAPE_FMR_LIST_CUR_WAN_IFINDEX error!\n");
		return;
	}
	ifGetName(wan_ifIndex, ifname, sizeof(ifname));
	
	boaWrite(wp, "<div class=\"column\">\n"
				"	<div class=\"column_title\">\n"
				"		<div class=\"column_title_left\"></div>\n"
				"			<p>%s (%s)</p>\n"
				"		<div class=\"column_title_right\"></div>\n"
				"	</div>\n", 
				multilang(LANG_MAPE_FMR_TABLE), ifname);

	//3. current FMR table list
	boaWrite(wp, "	<div class=\"data_common data_vertical\">\n"
				"		<table>\n");
	boaWrite(wp, "		<tr>\n"
				"			<th align=center width=\"5%%\">%s</th>\n"
				"			<th align=center width=\"20%%\">%s</th>\n"
				"			<th align=center width=\"10%%\">%s</th>\n"
				"			<th align=center width=\"5%%\">%s</th>\n"
				"			<th align=center width=\"5%%\">%s</th>\n"
				"		</tr>\n",
				multilang(LANG_MAPE_FMR_SELECT),multilang(LANG_MAPE_FMR_IPV6_PREFIX),
				multilang(LANG_MAPE_FMR_IPV4_PREFIX),multilang(LANG_MAPE_FMR_EA_LENGTH),multilang(LANG_MAPE_FMR_PSID_OFFSET));

	/*FMR entry*/
	fmrsNum = mib_chain_total(MIB_MAPE_FMRS_TBL);	
	for(fmrIdx=0; fmrIdx < fmrsNum; fmrIdx++){
		if (!mib_chain_get(MIB_MAPE_FMRS_TBL, fmrIdx, (void *)&fmrEntry)){
			boaError(wp, 400, "Get chain record error!\n");
			return;
		}

		if(fmrEntry.mape_fmr_extIf != wan_ifIndex)
			continue;
		
		inet_ntop(PF_INET6, fmrEntry.mape_fmr_v6Prefix, Ipv6AddrStr, sizeof(Ipv6AddrStr));
		boaWrite(wp, "		<tr>\n");
		boaWrite(wp, "			<td align=center width=\"5%%\"><input type=\"radio\" name=\"fmrSelect\" value=\"s%d\" onClick=\"mapeFmrPostEntry('%s', %d, '%s', %d, %d, %d)\"></td>\n",
					fmrIdx, Ipv6AddrStr, fmrEntry.mape_fmr_v6PrefixLen, 
					inet_ntoa(*((struct in_addr *)&fmrEntry.mape_fmr_v4Prefix)), fmrEntry.mape_fmr_v4MaskLen,
					fmrEntry.mape_fmr_eaLen, fmrEntry.mape_fmr_psidOffset);
		boaWrite(wp, "			<td align=center width=\"20%%\">%s/%d</td>\n"
					"			<td align=center width=\"10%%\">%s/%d</td>\n"				
					"			<td align=center width=\"5%%\">%d</td>\n"				
					"			<td align=center width=\"5%%\">%d</td>\n"
					"		</tr>\n",
					Ipv6AddrStr, fmrEntry.mape_fmr_v6PrefixLen, 
					inet_ntoa(*((struct in_addr *)&fmrEntry.mape_fmr_v4Prefix)), fmrEntry.mape_fmr_v4MaskLen,
					fmrEntry.mape_fmr_eaLen, fmrEntry.mape_fmr_psidOffset);
	}

	boaWrite(wp,"		</table>\n"
				"	</div>\n"
				"</div>\n"
				"</div>\n");		

}

void show_MAPE_settings(request *wp){
#ifndef CONFIG_GENERAL_WEB
#else
	boaWrite(wp, "<div id=mape_div style=\"display:none\">\n");
	
	/*MAP-E address mode */
	boaWrite(wp, "<div id=\"mape_mode_div\" style=\"display:none\">\n"
				"	<table>\n"
				"		<tr>\n"
				"			<th width=30%%>%s:</th>\n"
				"			<td width=70%%><select name=\"mape_mode\"	onChange=mapeModeChange()>\n"
				"				<option value=0>%s</option>\n"
				"				<option value=1>%s</option>\n"
				"			</select></td> \n"
				"		</tr>\n"
				"	</table>\n"
				"</div>\n", multilang(LANG_MAPE_SETTING_MODE),multilang(LANG_MAPE_SETTING_DHCP),multilang(LANG_MAPE_SETTING_STATIC));

	/*MAP-E forward mode*/
	#ifdef CONFIG_MAPE_ROUTING_MODE
	boaWrite(wp, "<div id=\"mape_forward_mode_div\" style=\"display:none\">\n"
				"	<table>\n"
				"	<tr><th width=30%%>%s:</th>\n"
				"		<td width=70%%>\n"
				"			<input type=\"radio\" value=0 name=\"mape_forward_mode\" id=\"mape_forward_mode\" onClick=mapeForwardModeChange()> %s \n"
				"			<input type=\"radio\" value=1 name=\"mape_forward_mode\" id=\"mape_forward_mode\" onClick=mapeForwardModeChange()> %s \n"
				"		</td>\n"
				"	</tr>\n"
				"	</table>\n"
				"</div>\n", multilang(LANG_MAPE_FORWARD_MODE),multilang(LANG_MAPE_FORWARD_NAT),multilang(LANG_MAPE_FORWARD_ROUTING));
	#endif

	boaWrite(wp, "<div id=mape_static_div style=\"display:none\">\n");
	boaWrite(wp, "	<table>\n");

	show_MAPE_DMR(wp);
	show_MAPE_BMR(wp);
	show_MAPE_FMR(wp);

	boaWrite(wp,  " </table>\n");
	boaWrite(wp, "	</div>\n"); //id=mape_static_div
	boaWrite(wp, "</div>\n"); //id=mape_div
#endif
}

int retrieve_MAPE_setting(request *wp, MIB_CE_ATM_VC_Tp pEntry, char *errBuf, int errBufLen){
	char *strValue1,*strValue2;
	int val, len;
	char s46type[16]={0};
	struct in_addr addr4;
	struct in6_addr addrIPv6;
	int i, fmrNums;
	MIB_CE_MAPE_FMRS_T fmrEntry;

	if(pEntry->IpProtocol==IPVER_IPV6){
		strValue1 = boaGetVar(wp, "s4over6Type", "");
		snprintf(s46type,sizeof(s46type),"%d",IPV6_MAPE);
		if ( !gstrcmp(strValue1, s46type)){
			pEntry->mape_enable = 1;

			//Routing mode
#ifdef CONFIG_MAPE_ROUTING_MODE
			strValue1 = boaGetVar(wp, "mape_forward_mode", "");
			if ( !gstrcmp(strValue1, "1")){
				pEntry->mape_forward_mode = IPV6_MAPE_ROUTE_MODE; //Routing mode
			}else
				pEntry->mape_forward_mode = IPV6_MAPE_NAT_MODE; // NAT mode
#endif

			/*mape mode*/
			strValue1 = boaGetVar(wp, "mape_mode", "");
			if(strValue1[0])
				pEntry->mape_mode = strValue1[0] - '0';	

			if(pEntry->mape_mode == IPV6_MAPE_MODE_STATIC){
				/*DMR: BR IPv6 preifx*/
				strValue1 = boaGetVar(wp, "mape_remote_v6Addr", "");
				if(!strValue1[0]){
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_DMR_SETTING), multilang(LANG_MAPE_BR_ADDRESS_NULL));
					goto SET_FAIL;
				}
				if(strlen(strValue1) > 39){
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_DMR_SETTING), multilang(LANG_MAPE_BR_ADDRESS_TOO_LONG));
					goto SET_FAIL;
				}	
				if(inet_pton(PF_INET6, strValue1, &addrIPv6) == 0){
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_DMR_SETTING), multilang(LANG_MAPE_BR_ADDRESS_INVALID));
					goto SET_FAIL;
				}
				memcpy(pEntry->mape_remote_v6Addr,&addrIPv6,sizeof(addrIPv6));

				/*BMR*/
				/*Local IPv6 preifx*/
				strValue1 = boaGetVar(wp, "mape_local_v6Prefix", "");
				strValue2 = boaGetVar(wp, "mape_local_v6PrefixLen", "");
				if(!strValue1[0] || !strValue2[0]){
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_BMR_SETTING), multilang(LANG_MAPE_IPV6_PREFIX_NULL));
					goto SET_FAIL;
				}		
				if(strlen(strValue1) > 39){
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_BMR_SETTING), multilang(LANG_MAPE_IPV6_PREFIX_TOO_LONG));
					goto SET_FAIL;
				}
				
				len = atoi(strValue2);
				if(len < 0 || len > 64){
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_BMR_SETTING), multilang(LANG_MAPE_IPV6_PREFIX_INVALID_LEN));
					goto SET_FAIL;
				}	

				//printf("[%s:%d] localV6Prefix=%s, prefixLen=%d\n",__FUNCTION__,__LINE__,strValue1,len);		
				if(inet_pton(PF_INET6, strValue1, &addrIPv6) == 0){			
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_BMR_SETTING), multilang(LANG_MAPE_IPV6_PREFIX_INVALID_FORMAT));
					goto SET_FAIL;
				}
				memcpy(pEntry->mape_local_v6Prefix,&addrIPv6,sizeof(addrIPv6));				
				pEntry->mape_local_v6PrefixLen = len;		
				
				/*Local IPv4 preifx*/
				strValue1 = boaGetVar(wp, "mape_local_v4Addr", "");
				strValue2 = boaGetVar(wp, "mape_local_v4MaskLen", "");
				if(!strValue1[0] || !strValue2[0]){		
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_BMR_SETTING), multilang(LANG_MAPE_IPV4_PREFIX_NULL));
					goto SET_FAIL;
				}
				if(strlen(strValue1) > 15){			
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_BMR_SETTING), multilang(LANG_MAPE_IPV4_PREFIX_TOO_LONG));
					goto SET_FAIL;
				}
				
				len = atoi(strValue2);
				if(len < 0 || len > 32){			
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_BMR_SETTING), multilang(LANG_MAPE_IPV4_PREFIX_INVALID_LEN));
					goto SET_FAIL;
				}
				//printf("[%s:%d] localV4Addr=%s, maskLen=%d\n",__FUNCTION__,__LINE__,strValue1, len);		
				if(inet_aton(strValue1, &addr4) == 0){			
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_BMR_SETTING), multilang(LANG_MAPE_IPV4_PREFIX_INVALID_FORMAT));
					goto SET_FAIL;
				}
				memcpy(pEntry->mape_local_v4Addr, &addr4, sizeof(addr4));
				pEntry->mape_local_v4MaskLen = len;							
				
				//PSID offset
				strValue1 = boaGetVar(wp, "mape_psid_offset", "");
				val = atoi(strValue1);
				if(val < 0 || val > 16){			
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_BMR_SETTING), multilang(LANG_MAPE_PSID_OFFSET_INVALID));
					goto SET_FAIL;
				}
				pEntry->mape_psid_offset = val;

				//PSID length			
				strValue1 = boaGetVar(wp, "mape_psid_len", "");
				val = atoi(strValue1);
				if(val < 0 || val > 16){			
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_BMR_SETTING), multilang(LANG_MAPE_PSID_LEN_INVALID));
					goto SET_FAIL;
				}
				pEntry->mape_psid_len = val;

				if(pEntry->mape_psid_offset + pEntry->mape_psid_len > 16){				
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_BMR_SETTING), multilang(LANG_MAPE_PSID_SUM_OF_LEN_OFFSET_VALID));
					goto SET_FAIL;
				}
				
				//PSID
				strValue1 = boaGetVar(wp, "mape_psid", "");
				if(strlen(strValue1) <= 2 || (strValue1[0] != '0' && strValue1[1] != 'x')){
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_BMR_SETTING), multilang(LANG_MAPE_PSID_HEX));
					goto SET_FAIL;
				}
				val = strtol(strValue1, NULL, 16);
				if(val > ( (1<<pEntry->mape_psid_len) -1)){
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_BMR_SETTING), multilang(LANG_MAPE_PSID_ERROR));
					goto SET_FAIL;
				}
				pEntry->mape_psid = val;

				if((pEntry->mape_local_v6PrefixLen+(32-pEntry->mape_local_v4MaskLen)+pEntry->mape_psid_len) > 64){				
					snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_BMR_SETTING), multilang(LANG_MAPE_BMR_END_USER_IPV6_LEN_LONG));
					goto SET_FAIL;
				}
				
				//FMR
				strValue1 = boaGetVar(wp, "mape_fmr_enabled", "");
				if ( !gstrcmp(strValue1, "1")){
					pEntry->mape_fmr_enabled = 1;
				}			
			}
		}
	}
	
	return 0;
SET_FAIL:
	return -1;
}

static int mape_fmr_rule_entry_generate(request *wp, char *errBuf, int errBufLen, MIB_CE_MAPE_FMRS_Tp fmrEntryP){
	char *strValue1, *strValue2;
	struct in_addr addr4;
	struct in6_addr addr6;
	int addr6_preixLen, addr4_prefixLen;
	int eaLen, psid_offset;
	int val, len;
	unsigned int wan_ifIndex;
	
	/*check IPv6 prefix*/
	strValue1 = boaGetVar(wp, "mape_fmr_v6Prefix", "");
	strValue2 = boaGetVar(wp, "mape_fmr_v6PrefixLen", "");
	if(!strValue1[0] || !strValue2[0]){
		snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_FMR_SETTING), multilang(LANG_MAPE_IPV6_PREFIX_NULL));
		goto SET_FAIL;
	}	
	if(strlen(strValue1) > 39){
		snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_FMR_SETTING), multilang(LANG_MAPE_IPV6_PREFIX_TOO_LONG));
		goto SET_FAIL;
	}	
	len = atoi(strValue2);
	if(len < 0 || len > 64){	
		snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_FMR_SETTING), multilang(LANG_MAPE_IPV6_PREFIX_INVALID_LEN));
		goto SET_FAIL;
	}
	
	//printf("[%s:%d] fmrV6Prefix=%s, prefixLen=%d\n",__FUNCTION__,__LINE__,strValue1,len);		
	if(inet_pton(PF_INET6, strValue1, &addr6) == 0){	
		snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_FMR_SETTING), multilang(LANG_MAPE_IPV6_PREFIX_INVALID_FORMAT));
		goto SET_FAIL;
	}
	addr6_preixLen = len;	

	/*check IPv4 prefix*/	
	strValue1 = boaGetVar(wp, "mape_fmr_v4Prefix", "");
	strValue2 = boaGetVar(wp, "mape_fmr_v4MaskLen", "");
	if(!strValue1[0] || !strValue2[0]){	
		snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_FMR_SETTING), multilang(LANG_MAPE_IPV4_PREFIX_NULL));
		goto SET_FAIL;
	}
	
	if(strlen(strValue1) > 15){	
		snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_FMR_SETTING), multilang(LANG_MAPE_IPV4_PREFIX_TOO_LONG));
		goto SET_FAIL;
	}
	len = atoi(strValue2);
	if(len < 0 || len > 32){	
		snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_FMR_SETTING), multilang(LANG_MAPE_IPV4_PREFIX_INVALID_LEN));
		goto SET_FAIL;
	}

	//printf("[%s:%d] fmrV4Addr=%s, maskLen=%d\n",__FUNCTION__,__LINE__,strValue1, len);		
	if(inet_aton(strValue1, &addr4) == 0){	
		snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_FMR_SETTING), multilang(LANG_MAPE_IPV4_PREFIX_INVALID_FORMAT));
		goto SET_FAIL;
	}
	addr4_prefixLen = len;

	/*check EA length*/
	strValue1 = boaGetVar(wp, "mape_fmr_eaLen", "");
	eaLen = atoi(strValue1);
	if(eaLen < 0 || eaLen < (32-addr4_prefixLen) || eaLen > (addr4_prefixLen+16)){
		snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_FMR_SETTING), multilang(LANG_MAPE_FMR_EA_LEN_INVALID));
		goto SET_FAIL;
	}
	
	/*check PSID offset*/
	strValue1 = boaGetVar(wp, "mape_fmr_psidOffset", "");
	psid_offset = atoi(strValue1);
	if(psid_offset < 0 || psid_offset > 16){	
		snprintf(errBuf, errBufLen, "MAP-E %s: %s", multilang(LANG_MAPE_FMR_SETTING), multilang(LANG_MAPE_PSID_OFFSET_INVALID));
		goto SET_FAIL;
	}

	if (!mib_get_s(MIB_MAPE_FMR_LIST_CUR_WAN_IFINDEX,(void *)&wan_ifIndex, sizeof(wan_ifIndex))){	
		snprintf(errBuf, errBufLen, "%s (MIB_MAPE_FMR_LIST_CUR_WAN_IFINDEX)", Tget_mib_error);		
		goto SET_FAIL;
	}
	
	fmrEntryP->mape_fmr_extIf = wan_ifIndex;
	memcpy(&fmrEntryP->mape_fmr_v6Prefix, &addr6, sizeof(addr6));
	fmrEntryP->mape_fmr_v6PrefixLen = addr6_preixLen;
	memcpy(&fmrEntryP->mape_fmr_v4Prefix, &addr4, sizeof(addr4));
	fmrEntryP->mape_fmr_v4MaskLen =addr4_prefixLen;
	fmrEntryP->mape_fmr_eaLen = eaLen;
	fmrEntryP->mape_fmr_psidOffset = psid_offset;

	return 0;	
SET_FAIL:
		return -1;

}

static int mape_fmr_rule_add(request *wp, char *errBuf, int errBufLen){
	int i, fmrNums, cur_mape_fmr_num=0;
	MIB_CE_MAPE_FMRS_T fmrEntry, fmrTmpEntry;
	unsigned int network_id1, network_id2;
	int ret;

	char *ip4Addr;

	if (mape_fmr_rule_entry_generate(wp, errBuf, errBufLen, &fmrEntry) < 0){
		goto SET_FAIL;
	}
	network_id1 = (((struct in_addr *)&fmrEntry.mape_fmr_v4Prefix)->s_addr)>>(32-fmrEntry.mape_fmr_v4MaskLen);

	fmrNums = mib_chain_total(MIB_MAPE_FMRS_TBL);
//	printf("[%s:%d] fmrNums=%d \n", __FUNCTION__,__LINE__, fmrNums);
	for(i = 0; i < fmrNums; i++){
		if (!mib_chain_get(MIB_MAPE_FMRS_TBL, i, (void *)&fmrTmpEntry)){
			snprintf(errBuf, errBufLen, "%s (MIB_MAPE_FMRS_TBL)", Tget_mib_error);			
			goto SET_FAIL;
		}

		if (fmrEntry.mape_fmr_extIf == fmrTmpEntry.mape_fmr_extIf){
			cur_mape_fmr_num++;
			
			network_id2 = (((struct in_addr *)&fmrTmpEntry.mape_fmr_v4Prefix)->s_addr)>>(32-fmrTmpEntry.mape_fmr_v4MaskLen);		
		//	printf("[%s:%d] (entry): entry_network_id=%u, web_network_id=%u\n",__FUNCTION__,__LINE__, network_id2, network_id1);		
			if (network_id1 == network_id2){
				snprintf(errBuf, errBufLen, "MAP-E %s: %s (same IPv4 prefix rule)", multilang(LANG_MAPE_FMR_SETTING), multilang(LANG_MAPE_FMR_RULE_EXIST));
				goto SET_FAIL;
			}
		}
	}

	if (cur_mape_fmr_num >= MAX_PER_MAPE_FMRS_NUM){
		snprintf(errBuf, errBufLen, "MAP-E FMRs(%d) is full!\n", cur_mape_fmr_num);
		goto SET_FAIL;
	}
	
	ret = mib_chain_add(MIB_MAPE_FMRS_TBL, (unsigned char*)&fmrEntry);
	if (ret == 0) {
		snprintf(errBuf, errBufLen, "%s (MIB_MAPE_FMRS_TBL)", Tadd_chain_error);
		goto SET_FAIL;
	}else if (ret == -1) {
		snprintf(errBuf, errBufLen, "%s (MIB_MAPE_FMRS_TBL)", strTableFull);
		goto SET_FAIL;
	}

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	return 0;
SET_FAIL:
	return -1;
}

static int mape_fmr_rule_modify(request *wp, char *errBuf, int errBufLen){
	char *strValue;
	char tmpstr[64]={0};
	int idx, fmrNums;
	MIB_CE_MAPE_FMRS_T fmrEntry, fmrTmpEntry;

	if (mape_fmr_rule_entry_generate(wp, errBuf, errBufLen, &fmrEntry) < 0){
		goto SET_FAIL;
	}

	strValue =  boaGetVar(wp, "fmrSelect", "");
	if (strValue[0]){	
		sscanf(strValue, "s%d", &idx);
		printf("[%s:%d] (mod) idx=%d\n",__FUNCTION__,__LINE__,strValue);
		
		fmrNums = mib_chain_total(MIB_MAPE_FMRS_TBL);
		if (fmrNums < idx){
			snprintf(errBuf, errBufLen, "Modify MIB_MAPE_FMRS_TBL failed! (fmrNums=%d, del_idx=%d)", fmrNums, idx);
			goto SET_FAIL;
		}
		
		if (mib_chain_update(MIB_MAPE_FMRS_TBL, (void *)&fmrEntry, idx) != 1){		
			snprintf(errBuf, errBufLen, "%s (MIB_MAPE_FMRS_TBL.%d)", Tupdate_chain_error, idx);
			goto SET_FAIL;
		}
		
	#ifdef COMMIT_IMMEDIATELY
		Commit();
	#endif
	}
	return 0;
SET_FAIL:
	return -1;
}

static int mape_fmr_rule_delete(request *wp, char *errBuf, int errBufLen){
	char *strValue;
	char tmpstr[8]={0};
	int idx, fmrNums;
	MIB_CE_MAPE_FMRS_T fmrEntry;
	
	strValue =  boaGetVar(wp, "fmrSelect", "");
	if (strValue[0]){		
		sscanf(strValue, "s%d", &idx);
		printf("[%s:%d] (del) idx=%d\n",__FUNCTION__,__LINE__,idx);
		
		fmrNums = mib_chain_total(MIB_MAPE_FMRS_TBL);
		if (fmrNums < idx){
			snprintf(errBuf, errBufLen, "Delete MIB_MAPE_FMRS_TBL failed! (fmrNums=%d, del_idx=%d)", fmrNums, idx);
			goto SET_FAIL;
		}
		
		if (mib_chain_delete(MIB_MAPE_FMRS_TBL, idx) != 1) {
			snprintf(errBuf, errBufLen, "%s (MIB_MAPE_FMRS_TBL.%d)", Tdelete_chain_error, idx);
			goto SET_FAIL;
		}
		
	#ifdef COMMIT_IMMEDIATELY
		Commit();
	#endif
	}	
	return 0;
SET_FAIL:
	return -1;	
}

/*FMRs*/
void formMAPE_FMR_WAN(request * wp, char *path, char *query){
	char *value;
	unsigned int ifIndex;
	char errBuf[256];
	
	value = boaGetVar(wp, "lkname", "");
	printf("[%s:%s:%d] lkname=%s\n",__FILE__,__FUNCTION__,__LINE__,value);
	if (value[0]){
		if (strcmp(value, "new") == 0){
			snprintf(errBuf, sizeof(errBuf), "This is a new link, please add the link first!\n");
			goto SET_FAIL;
		}else{
			value = boaGetVar(wp, "lst", "");
			if (value[0]){
				ifIndex = getIfIndexByName(value);
				if (ifIndex != DUMMY_IFINDEX){
					if (mib_set(MIB_MAPE_FMR_LIST_CUR_WAN_IFINDEX, (void *)&ifIndex) == 0){
						snprintf(errBuf, sizeof(errBuf), "Set MIB_MAPE_FMR_LIST_CUR_WAN_IFINDEX failed!\n");
						goto SET_FAIL;
					}else{ //redirect to /admin/mape_fmr_lists.asp
					#ifdef COMMIT_IMMEDIATELY
						Commit();
					#endif
						boaRedirect(wp, "/admin/mape_fmr_lists.asp");
						return;
					}
				}else{
					snprintf(errBuf, sizeof(errBuf), "getIfIndexByName(%s) failed!\n", value);
					goto SET_FAIL;
				}
				
			}else{
				snprintf(errBuf, sizeof(errBuf), "lst is not set!\n");
				goto SET_FAIL;
			}
		}		
	}

	return;
	
SET_FAIL:
	ERR_MSG(errBuf);
	return;
}

void formMAPE_FMR_Edit(request * wp, char *path, char *query){
	char *strValue;
	char errBuf[256];
	unsigned int wan_ifIndex, entry_index;
	MIB_CE_ATM_VC_T entry;
	char ifname[IFNAMSIZ];
	char *submitUrl;
	int change = 0;
	
	/*add fmr*/
	strValue = boaGetVar(wp, "addFMR", "");
	if(strValue[0]){
		if(mape_fmr_rule_add(wp, errBuf, sizeof(errBuf)) < 0)
			goto SET_FAIL;
		change = 1;
	}
	
	/*modify fmr*/
	strValue = boaGetVar(wp, "modifyFMR", "");
	if(strValue[0]){
		if(mape_fmr_rule_modify(wp, errBuf, sizeof(errBuf)) < 0)
			goto SET_FAIL;
		change = 1;
	}
	
	/*delete fmr*/
	strValue = boaGetVar(wp, "delFMR", "");
	if(strValue[0]){
		if(mape_fmr_rule_delete(wp, errBuf, sizeof(errBuf)) < 0)
			goto SET_FAIL;
		change = 1;
	}

	if (change == 1){//re-setup MAP-E interface	
		if (!mib_get_s(MIB_MAPE_FMR_LIST_CUR_WAN_IFINDEX,(void *)&wan_ifIndex, sizeof(wan_ifIndex))){
			snprintf(errBuf, sizeof(errBuf), "%s (MIB_MAPE_FMR_LIST_CUR_WAN_IFINDEX)", Tget_mib_error);			
			goto SET_FAIL;
		}

		if (getWanEntrybyIfIndex(wan_ifIndex, &entry, &entry_index) <0){
			snprintf(errBuf, sizeof(errBuf), "%s (MIB_ATM_VC_TBL)", Tget_mib_error);			
			goto SET_FAIL;
		}


		if (entry.mape_fmr_enabled == 0){
			entry.mape_fmr_enabled = 1;
			mib_chain_update(MIB_ATM_VC_TBL, &entry, entry_index);
			
		#ifdef COMMIT_IMMEDIATELY
			Commit();
		#endif
		}

		ifGetName(wan_ifIndex, ifname, sizeof(ifname));
		//printf("[%s:%d] wan_ifIndex=%d, ifname=%s\n",__FUNCTION__,__LINE__,wan_ifIndex,ifname);
		
		//re-setup MAP-E interface
		cmd_mape_static_mode(ifname);
	}

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);

	return;
	
SET_FAIL:
	ERR_MSG(errBuf);
	return;
}

#endif

#ifdef CONFIG_USER_MAP_T
void initMAPT(request * wp, MIB_CE_ATM_VC_Tp entryP){
	unsigned char	Ipv6AddrStr[INET6_ADDRSTRLEN];

	boaWrite(wp, _PTI, "mapt_enable", entryP->mapt_enable);
	if (entryP->mapt_enable){
		boaWrite(wp, _PTI, "mapt_mode", entryP->mapt_mode);
		if(entryP->mapt_mode == IPV6_MAPT_MODE_STATIC){
			/*DMR*/
			inet_ntop(PF_INET6, entryP->mapt_remote_v6Prefix, Ipv6AddrStr, sizeof(Ipv6AddrStr));
			boaWrite(wp, _PTS _PTI, "mapt_remote_v6Prefix", Ipv6AddrStr, "mapt_remote_v6PrefixLen", entryP->mapt_remote_v6PrefixLen);

			inet_ntop(PF_INET6, entryP->mapt_local_v6Prefix, Ipv6AddrStr, sizeof(Ipv6AddrStr));
			boaWrite(wp, _PTS _PTI, "mapt_local_v6Prefix", Ipv6AddrStr, "mapt_local_v6PrefixLen", entryP->mapt_local_v6PrefixLen);
		
			boaWrite(wp, _PTS _PTI, "mapt_local_v4Addr", inet_ntoa(*((struct in_addr *)&entryP->mapt_local_v4Addr)),
					"mapt_local_v4MaskLen", entryP->mapt_local_v4MaskLen);

			boaWrite(wp, _PTI _PTI, "mapt_psid_offset", entryP->mapt_psid_offset, "mapt_psid_len", entryP->mapt_psid_len);

			boaWrite(wp, ", new it(\"%s\", 0x%x)", "mapt_psid", entryP->mapt_psid);
		}		
	}
}

int retrieve_MAPT_setting(request *wp, MIB_CE_ATM_VC_Tp pEntry, char *errBuf, int errBufLen){
	char *strValue1,*strValue2;
	int val, len;
	char s46type[16]={0};
	struct in_addr addr4;
	struct in6_addr addrIPv6;
	int i;

	if(pEntry->IpProtocol==IPVER_IPV6){
		strValue1 = boaGetVar(wp, "s4over6Type", "");
		snprintf(s46type,sizeof(s46type),"%d",IPV6_MAPT);
		if ( !gstrcmp(strValue1, s46type)){
			pEntry->mapt_enable = 1;

			/*mapt mode*/
			strValue1 = boaGetVar(wp, "mapt_mode", "");
			if(strValue1[0])
				pEntry->mapt_mode = strValue1[0] - '0';
			if(pEntry->mapt_mode == IPV6_MAPT_MODE_STATIC){
				/*DMR: BR IPv6 preifx*/
				strValue1 = boaGetVar(wp, "mapt_remote_v6Prefix", "");
				strValue2 = boaGetVar(wp, "mapt_remote_v6PrefixLen", "");
				if(!strValue1[0]|| !strValue2[0]){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_DMR_SETTING), multilang(LANG_MAPT_BR_PREFIX_NULL));
					goto SET_FAIL;
				}
				if(strlen(strValue1) > 39){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_DMR_SETTING), multilang(LANG_MAPT_BR_PREFIX_TOO_LONG));
					goto SET_FAIL;
				}
				len = atoi(strValue2);
				if(len < 0 || len > 128){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_DMR_SETTING), multilang(LANG_MAPT_IPV6_PREFIX_INVALID_LEN));
					goto SET_FAIL;
				}
				if(inet_pton(PF_INET6, strValue1, &addrIPv6) == 0){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_DMR_SETTING), multilang(LANG_MAPT_BR_PREFIX_INVALID));
					goto SET_FAIL;
				}
				memcpy(pEntry->mapt_remote_v6Prefix,&addrIPv6,sizeof(addrIPv6));
				pEntry->mapt_remote_v6PrefixLen = len;

				/*BMR*/
				/*Local IPv6 preifx*/
				strValue1 = boaGetVar(wp, "mapt_local_v6Prefix", "");
				strValue2 = boaGetVar(wp, "mapt_local_v6PrefixLen", "");
				if(!strValue1[0] || !strValue2[0]){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_BMR_SETTING), multilang(LANG_MAPT_IPV6_PREFIX_NULL));
					goto SET_FAIL;
				}
				if(strlen(strValue1) > 39){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_BMR_SETTING), multilang(LANG_MAPT_IPV6_PREFIX_TOO_LONG));
					goto SET_FAIL;
				}

				len = atoi(strValue2);
				if(len < 0 || len > 64){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_BMR_SETTING), multilang(LANG_MAPT_IPV6_PREFIX_INVALID_LEN));
					goto SET_FAIL;
				}

				//printf("[%s:%d] localV6Prefix=%s, prefixLen=%d\n",__FUNCTION__,__LINE__,strValue1,len);
				if(inet_pton(PF_INET6, strValue1, &addrIPv6) == 0){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_BMR_SETTING), multilang(LANG_MAPT_IPV6_PREFIX_INVALID_FORMAT));
					goto SET_FAIL;
				}
				memcpy(pEntry->mapt_local_v6Prefix,&addrIPv6,sizeof(addrIPv6));
				pEntry->mapt_local_v6PrefixLen = len;

				/*Local IPv4 preifx*/
				strValue1 = boaGetVar(wp, "mapt_local_v4Addr", "");
				strValue2 = boaGetVar(wp, "mapt_local_v4MaskLen", "");
				if(!strValue1[0] || !strValue2[0]){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_BMR_SETTING), multilang(LANG_MAPT_IPV4_PREFIX_NULL));
					goto SET_FAIL;
				}
				if(strlen(strValue1) > 15){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_BMR_SETTING), multilang(LANG_MAPT_IPV4_PREFIX_TOO_LONG));
					goto SET_FAIL;
				}

				len = atoi(strValue2);
				if(len < 0 || len > 32){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_BMR_SETTING), multilang(LANG_MAPT_IPV4_PREFIX_INVALID_LEN));
					goto SET_FAIL;
				}
				//printf("[%s:%d] localV4Addr=%s, maskLen=%d\n",__FUNCTION__,__LINE__,strValue1, len);
				if(inet_aton(strValue1, &addr4) == 0){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_BMR_SETTING), multilang(LANG_MAPT_IPV4_PREFIX_INVALID_FORMAT));
					goto SET_FAIL;
				}
				memcpy(pEntry->mapt_local_v4Addr, &addr4, sizeof(addr4));
				pEntry->mapt_local_v4MaskLen = len;

				//PSID offset
				strValue1 = boaGetVar(wp, "mapt_psid_offset", "");
				val = atoi(strValue1);
				if(val < 0 || val > 16){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_BMR_SETTING), multilang(LANG_MAPT_PSID_OFFSET_INVALID));
					goto SET_FAIL;
				}
				pEntry->mapt_psid_offset = val;

				//PSID length
				strValue1 = boaGetVar(wp, "mapt_psid_len", "");
				val = atoi(strValue1);
				if(val < 0 || val > 16){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_BMR_SETTING), multilang(LANG_MAPT_PSID_LEN_INVALID));
					goto SET_FAIL;
				}
				pEntry->mapt_psid_len = val;

				if(pEntry->mapt_psid_offset + pEntry->mapt_psid_len > 16){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_BMR_SETTING), multilang(LANG_MAPT_PSID_SUM_OF_LEN_OFFSET_VALID));
					goto SET_FAIL;
				}

				//PSID
				strValue1 = boaGetVar(wp, "mapt_psid", "");
				if(strlen(strValue1) <= 2 || (strValue1[0] != '0' && strValue1[1] != 'x')){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_BMR_SETTING), multilang(LANG_MAPT_PSID_HEX));
					goto SET_FAIL;
				}
				val = strtol(strValue1, NULL, 16);
				if(val > ( (1<<pEntry->mapt_psid_len) -1)){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_BMR_SETTING), multilang(LANG_MAPT_PSID_ERROR));
					goto SET_FAIL;
				}
				pEntry->mapt_psid = val;

				if((pEntry->mapt_local_v6PrefixLen+(32-pEntry->mapt_local_v4MaskLen)+pEntry->mapt_psid_len) > 64){
					snprintf(errBuf, errBufLen, "MAP-T %s: %s", multilang(LANG_MAPT_BMR_SETTING), multilang(LANG_MAPT_BMR_END_USER_IPV6_LEN_LONG));
					goto SET_FAIL;
				}

				/* MAP-T FMRs to do*/
			}
		}
	}

	return 0;
SET_FAIL:
	return -1;
}

static void showMAPTDMR(request *wp){
	boaWrite(wp, "<tr>\n"
			"			<td>%s</td>\n"
			"		</tr>\n"
			"		<tr>\n"
			"			<th width=30%%>%s:</th>\n"
			"			<td width=70%%><input type=text name=\"mapt_remote_v6Prefix\" size=39 maxlength=39>\n"
			"			\n <input type=text name=\"mapt_remote_v6PrefixLen\" size=3 maxlength=3>"
			"			(eg. 2001:0db8:1::/64)</td>\n"
			"		</tr>\n", multilang(LANG_MAPT_DMR_SETTING),multilang(LANG_MAPT_DMR_PREFIX));
}
static void showMAPTBMR(request *wp){
	boaWrite(wp, "<tr>\n"
			"			<td>%s</td>\n"
			"		</tr>\n"
			"		<tr>\n"
			"			<th width=30%%>%s:</th>\n"
			"			<td width=70%%><input type=text name=\"mapt_local_v6Prefix\" size=39 maxlength=39>\n"
			"			\n <input type=text name=\"mapt_local_v6PrefixLen\" size=3 maxlength=3>"
			"			(eg. 2001:0db8::/40)</td>\n"
			"		</tr>\n"
			"		<tr>\n"
			"			<th width=30%%>%s:</th>\n"
			"			<td width=70%%><input type=text name=\"mapt_local_v4Addr\" size=14 maxlength=15>\n"
			"			\n <input type=text name=\"mapt_local_v4MaskLen\" size=2 maxlength=2>"
			"			(eg. 192.0.2.18/24)</td>\n"
			"		</tr>\n"
			"		<tr>\n"
			"			<th width=30%%>%s:</th>\n"
			"			<td width=70%%><input type=text name=\"mapt_psid_offset\" size=2 maxlength=2> (0-16)</td>\n"
			"		</tr>\n"
			"		<tr>\n"
			"			<th width=30%%>%s:</th>\n"
			"			<td width=70%%><input type=text name=\"mapt_psid_len\" size=2 maxlength=2> (0-16)</td>\n"
			"		</tr>\n"
			"		<tr>\n"
			"			<th width=30%%>%s:</th>\n"
			"			<td width=70%%><input type=text name=\"mapt_psid\" size=6 maxlength=6> (eg. 0x34)</td>\n"
			"		</tr>\n",
			multilang(LANG_MAPT_BMR_SETTING),multilang(LANG_MAPT_BMR_LOCAL_IPV6_PREFIX),multilang(LANG_MAPT_BMR_LOCAL_IPV4_PREFIX),
			multilang(LANG_MAPT_BMR_PSID_OFFSET),multilang(LANG_MAPT_BMR_PSID_LENGTH),multilang(LANG_MAPT_BMR_PSID_VAL));
}

void showMAPTSettings(request *wp){
#ifndef CONFIG_GENERAL_WEB
#else
	boaWrite(wp, "<div id=mapt_div style=\"display:none\">\n");

	/*MAP-T mode */
	boaWrite(wp, "<div id=\"mapt_mode_div\" style=\"display:none\">\n"
			"	<table>\n"
			"		<tr>\n"
			"			<th width=30%%>%s:</th>\n"
			"			<td width=70%%><select name=\"mapt_mode\"	onChange=maptModeChange()>\n"
			"				<option value=\"0\" >%s</option>\n"
			"				<option value=\"1\" >%s</option>\n"
			"			</select></td> \n"
			"		</tr>\n"
			"	</table>\n"
			"</div>\n", multilang(LANG_MAPT_SETTING_MODE),multilang(LANG_MAPT_SETTING_DHCP),multilang(LANG_MAPT_SETTING_STATIC));

	boaWrite(wp, "<div id=mapt_static_div style=\"display:none\">\n");
	boaWrite(wp, "	<table>\n");

	showMAPTDMR(wp);
	showMAPTBMR(wp);

	//FMR: todo

	boaWrite(wp,  " </table>\n");
	boaWrite(wp, "	</div>\n"); //id=mapt_static_div

	boaWrite(wp, "</div>\n"); //id=mapt_div
#endif
}



#endif

#ifdef CONFIG_USER_XLAT464
void initXLAT464(request * wp, MIB_CE_ATM_VC_Tp entryP){
	MIB_CE_ATM_VC_T Entry;
	unsigned char clatprefix6[INET6_ADDRSTRLEN]={0};
	unsigned char platprefix6[INET6_ADDRSTRLEN]={0};

	memcpy(&Entry, entryP, sizeof(Entry));

	//xlat464 on/off, mode init
	boaWrite(wp, _PTI _PTI, _PME(xlat464_enable), _PME(xlat464_clat_mode));
	//clat
	if(Entry.xlat464_enable == 1 && Entry.xlat464_clat_mode == IPV6_XLAT464_CLAT_MODE_STATIC){
		inet_ntop(PF_INET6, Entry.xlat464_clat_prefix6, clatprefix6, sizeof(clatprefix6));
		boaWrite(wp, _PTS, "xlat464_clat_prefix6", clatprefix6);
	}
	//plat
	inet_ntop(PF_INET6, Entry.xlat464_plat_prefix6, platprefix6, sizeof(platprefix6));
	boaWrite(wp, _PTS, "xlat464_plat_prefix6", platprefix6);
}
void showXLAT464Settings(request *wp){
#ifndef CONFIG_GENERAL_WEB
#else
	boaWrite(wp, "<div id=\"XLAT464Div\" style=\"display:none\">\n");

	boaWrite(wp,
			"<div id=\"xlat464_clatmode_div\" style=\"display:none\">\n"
			"<table>\n"
			"	<tr>\n"
			"		<th width=30%%>%s:</th>\n"
			"		<td width=70%%>\n"
			"			<input type=\"radio\" value=0 name=\"xlat464_clat_mode\" id=\"send7\" onclick=xlat464ClatPrefix6ModeChange()>%s\n"
			"			<input type=\"radio\" value=1 name=\"xlat464_clat_mode\" id=\"send8\" onclick=xlat464ClatPrefix6ModeChange()>%s\n"
			"		</td>\n"
			"	</tr>\n"
			"</table>\n"
			"</div>\n",multilang(LANG_XLAT464_SETTING_CLAT_MODE), multilang(LANG_XLAT464_SETTING_CLAT_DHCP), multilang(LANG_XLAT464_SETTING_CLAT_STATIC));
	boaWrite(wp,
			"<div id=\"xlat464_clatprefix6_div\" style=\"display:none\">\n"
			"<table>\n"
			"	<tr>\n"
			"		<th width=30%%>%s:</th>\n"
			"		<td width=70%%><input id=\"xlat464_clat_prefix6\" maxlength=39 size=36 name=\"xlat464_clat_prefix6\"></td>\n"
			"	</tr>\n"
			"</table>\n"
			"</div>\n",multilang(LANG_XLAT464_CLAT_SETTING));
	boaWrite(wp,
			"<div id=\"xlat464_platprefix6_div\" style=\"display:none\">\n"
			"<table>\n"
			"	<tr>\n"
			"		<th width=30%%>%s:</th>\n"
			"		<td width=70%%><input id=\"xlat464_plat_prefix6\" maxlength=39 size=36 name=\"xlat464_plat_prefix6\"></td>\n"
			"	</tr>\n"
			"</table>\n"
			"</div>\n"
			"</div>\n",multilang(LANG_XLAT464_PLAT_SETTING));
#endif
}
int retrieve_XLAT464_setting(request *wp, MIB_CE_ATM_VC_Tp pEntry, char *errBuf, int errBufLen){
	char *strValue=NULL;
	char s46type[16]={0};
	struct in6_addr clatprefix6={0};
	struct in6_addr platprefix6={0};

	unsigned char dbg[INET6_ADDRSTRLEN];
	if(pEntry->IpProtocol==IPVER_IPV6){
		strValue = boaGetVar(wp, "s4over6Type", "");
		snprintf(s46type,sizeof(s46type),"%d",IPV6_XLAT464);
		if(!gstrcmp(strValue,s46type)){
			pEntry->xlat464_enable = 1;
			//clat prefix6
			strValue = boaGetVar(wp, "xlat464_clat_mode", "");
			if(strValue[0])
				pEntry->xlat464_clat_mode = strValue[0] - '0';

			if(pEntry->xlat464_clat_mode == IPV6_XLAT464_CLAT_MODE_STATIC){
				//clat static
				strValue = boaGetVar(wp, "xlat464_clat_prefix6", "");
				if(!strValue[0]){
					snprintf(errBuf, errBufLen, "%s: %s", multilang(LANG_XLAT464_SETTING), multilang(LANG_XLAT464_CLAT_PREFIX_NULL));
					goto SET_FAIL;
				}
				if(strlen(strValue)>39){
					snprintf(errBuf, errBufLen, "%s: %s", multilang(LANG_XLAT464_SETTING), multilang(LANG_XLAT464_CLAT_PREFIX_TOOLONG));
					goto SET_FAIL;
				}
				if(inet_pton(PF_INET6, strValue, &clatprefix6)==0){
					snprintf(errBuf, errBufLen, "%s: %s", multilang(LANG_XLAT464_SETTING), multilang(LANG_XLAT464_CLAT_PREFIX_INVALID));
					goto SET_FAIL;
				}
				memcpy(pEntry->xlat464_clat_prefix6, &clatprefix6, sizeof(clatprefix6));
			}
			else{
				//clat dhcpv6pd:
				// 1. Cannot use wan static mode
				if(pEntry->AddrMode == IPV6_WAN_STATIC){
					snprintf(errBuf, errBufLen, "%s: %s", multilang(LANG_XLAT464_SETTING), multilang(LANG_XLAT464_CLAT_MODE_WAN_INVALID));
					goto SET_FAIL;
				}
				// 2. Request Prefix is needed
				strValue = boaGetVar(wp, "iapd", "");
				if (gstrcmp(strValue, "ON")){
					snprintf(errBuf, errBufLen, "%s: %s", multilang(LANG_XLAT464_SETTING), multilang(LANG_XLAT464_CLAT_MODE_REQ_OPTIONS_INVALID));
					goto SET_FAIL;
				}
			}

			//plat prefix6
			strValue = boaGetVar(wp, "xlat464_plat_prefix6", "");
			if(!strValue[0]){
				snprintf(errBuf, errBufLen, "%s: %s", multilang(LANG_XLAT464_SETTING), multilang(LANG_XLAT464_PLAT_PREFIX_NULL));
				goto SET_FAIL;
			}
			if(strlen(strValue)>39){
				snprintf(errBuf, errBufLen, "%s: %s", multilang(LANG_XLAT464_SETTING), multilang(LANG_XLAT464_PLAT_PREFIX_TOOLONG));
				goto SET_FAIL;
			}
			if(inet_pton(PF_INET6, strValue, &platprefix6)==0){
				snprintf(errBuf, errBufLen, "%s: %s", multilang(LANG_XLAT464_SETTING), multilang(LANG_XLAT464_PLAT_PREFIX_INVALID));
				goto SET_FAIL;
			}
			memcpy(pEntry->xlat464_plat_prefix6, &platprefix6, sizeof(platprefix6));
		}
	}
	return 0;
SET_FAIL:
	return -1;
}
#endif

#ifdef CONFIG_USER_LW4O6
void initLW4O6(request * wp, MIB_CE_ATM_VC_Tp entryP){
	MIB_CE_ATM_VC_T Entry;
	unsigned char aftr_addr[INET6_ADDRSTRLEN]={0};
	unsigned char addr6_prefix[INET6_ADDRSTRLEN]={0};
	unsigned char ipv4_addr[INET_ADDRSTRLEN]={0};

	memcpy(&Entry, entryP, sizeof(Entry));

	boaWrite(wp, _PTI _PTI, _PME(lw4o6_enable), _PME(lw4o6_mode));

	if(Entry.lw4o6_enable == 1 && Entry.lw4o6_mode == IPV6_LW4O6_MODE_STATIC){
		//Aftr address
		inet_ntop(PF_INET6, Entry.lw4o6_aftr_addr, aftr_addr, sizeof(aftr_addr));
		boaWrite(wp, _PTS, "lw4o6_aftr_addr", aftr_addr);
		//IPv6 prefix
		inet_ntop(PF_INET6, Entry.lw4o6_local_v6Prefix, addr6_prefix, sizeof(addr6_prefix));
		boaWrite(wp, _PTS, "lw4o6_local_v6Prefix", addr6_prefix);
		//IPv6 prefix length
		boaWrite(wp, _PTI, "lw4o6_local_v6PrefixLen", Entry.lw4o6_local_v6PrefixLen);
		//IPv4 address
		inet_ntop(PF_INET, Entry.lw4o6_v4addr, ipv4_addr, sizeof(ipv4_addr));
		boaWrite(wp, _PTS, "lw4o6_v4addr", ipv4_addr);
		//PSID length
		boaWrite(wp, _PTI, "lw4o6_psid_len", Entry.lw4o6_psid_len);
		//PSID
		boaWrite(wp, ", new it(\"%s\", 0x%x)", "lw4o6_psid", Entry.lw4o6_psid);
	}
}
int retrieve_LW4O6_setting(request *wp, MIB_CE_ATM_VC_Tp pEntry, char *errBuf, int errBufLen){
	char *strValue=NULL;
	int val=0, len=0;
	char s46type[16]={0};
	struct in_addr addr4;
	struct in6_addr addrIPv6;

	if(pEntry->IpProtocol==IPVER_IPV6){
		strValue = boaGetVar(wp, "s4over6Type", "");
		snprintf(s46type, sizeof(s46type), "%d", IPV6_LW4O6);
		if (!gstrcmp(strValue, s46type)){
			//lw4o6 enable
			pEntry->lw4o6_enable = 1;
			//lw4o6 mode
			strValue = boaGetVar(wp, "lw4o6_mode", "");
			if(strValue[0])
				pEntry->lw4o6_mode = strValue[0] - '0';

			if(pEntry->lw4o6_mode == IPV6_LW4O6_MODE_STATIC){
				//Aftr address
				strValue = boaGetVar(wp, "lw4o6_aftr_addr", "");
				if(!strValue[0]){
					snprintf(errBuf, errBufLen, "LW4o6 %s: %s", multilang(LANG_LW4O6_STATIC_MODE), multilang(LANG_LW4O6_AFTR_ADDR_NULL));
					goto SET_FAIL;
				}
				if(strlen(strValue) > 39){
					snprintf(errBuf, errBufLen, "LW4o6 %s: %s", multilang(LANG_LW4O6_STATIC_MODE), multilang(LANG_LW4O6_AFTR_ADDR_INVALID_LEN));
					goto SET_FAIL;
				}
				if(inet_pton(PF_INET6, strValue, &addrIPv6) == 0){
					snprintf(errBuf, errBufLen, "LW4o6 %s: %s", multilang(LANG_LW4O6_STATIC_MODE), multilang(LANG_LW4O6_AFTR_ADDR_INVALID));
					goto SET_FAIL;
				}
				memcpy(pEntry->lw4o6_aftr_addr,&addrIPv6,sizeof(addrIPv6));

				//IPv6 prefix
				strValue = boaGetVar(wp, "lw4o6_local_v6Prefix", "");
				if(!strValue[0]){
					snprintf(errBuf, errBufLen, "LW4o6 %s: %s", multilang(LANG_LW4O6_STATIC_MODE), multilang(LANG_LW4O6_LOCAL_IPV6PREFIX_NULL));
					goto SET_FAIL;
				}
				if(strlen(strValue) > 39){
					snprintf(errBuf, errBufLen, "LW4o6 %s: %s", multilang(LANG_LW4O6_STATIC_MODE), multilang(LANG_LW4O6_LOCAL_IPV6PREFIX_INVALID_LEN));
					goto SET_FAIL;
				}
				if(inet_pton(PF_INET6, strValue, &addrIPv6) == 0){
					snprintf(errBuf, errBufLen, "LW4o6 %s: %s", multilang(LANG_LW4O6_STATIC_MODE), multilang(LANG_LW4O6_LOCAL_IPV6PREFIX_INVALID));
					goto SET_FAIL;
				}
				memcpy(pEntry->lw4o6_local_v6Prefix, &addrIPv6,sizeof(addrIPv6));

				//IPv6 prefix length
				strValue = boaGetVar(wp, "lw4o6_local_v6PrefixLen", "");
				if(!strValue[0]){
					snprintf(errBuf, errBufLen, "LW4o6 %s: %s", multilang(LANG_LW4O6_STATIC_MODE), multilang(LANG_LW4O6_LOCAL_IPV6PREFIXLEN_NULL));
					goto SET_FAIL;
				}
				len = atoi(strValue);
				if(len < 0 || len > 64){
					snprintf(errBuf, errBufLen, "LW4o6 %s: %s", multilang(LANG_LW4O6_STATIC_MODE), multilang(LANG_LW4O6_LOCAL_IPV6PREFIXLEN_INVALID_LEN));
					goto SET_FAIL;
				}
				pEntry->lw4o6_local_v6PrefixLen = len;

				//IPv4 address
				strValue = boaGetVar(wp, "lw4o6_v4addr", "");
				if(!strValue[0]){
					snprintf(errBuf, errBufLen, "LW4o6 %s: %s", multilang(LANG_LW4O6_STATIC_MODE), multilang(LANG_LW4O6_IPV4_ADDR_NULL));
					goto SET_FAIL;
				}
				if(strlen(strValue) > 15){
					snprintf(errBuf, errBufLen, "LW4o6 %s: %s", multilang(LANG_LW4O6_STATIC_MODE), multilang(LANG_LW4O6_IPV4_ADDR_INVALID_LEN));
					goto SET_FAIL;
				}
				if(inet_aton(strValue, &addr4) == 0){
					snprintf(errBuf, errBufLen, "LW4o6 %s: %s", multilang(LANG_LW4O6_STATIC_MODE), multilang(LANG_LW4O6_IPV4_ADDR_INVALID));
					goto SET_FAIL;
				}
				memcpy(pEntry->lw4o6_v4addr, &addr4, sizeof(addr4));

				//PSID length
				//Since offset has a fixed length 4, PSID length should be 0~12
				strValue = boaGetVar(wp, "lw4o6_psid_len", "");
				val = atoi(strValue);
				if(val < 0 || val > 12){
					snprintf(errBuf, errBufLen, "LW4o6 %s: %s", multilang(LANG_LW4O6_STATIC_MODE), multilang(LANG_LW4O6_PSID_LEN_INVALID));
					goto SET_FAIL;
				}
				pEntry->lw4o6_psid_len = val;

				//PSID
				strValue = boaGetVar(wp, "lw4o6_psid", "");
				if(strlen(strValue) <= 2 || (strValue[0] != '0' && strValue[1] != 'x')){
					snprintf(errBuf, errBufLen, "LW4o6 %s: %s", multilang(LANG_LW4O6_STATIC_MODE), multilang(LANG_LW4O6_PSID_INVALID));
					goto SET_FAIL;
				}
				val = strtol(strValue, NULL, 16);
				if(val > ( (1<<pEntry->lw4o6_psid_len) -1)){
					snprintf(errBuf, errBufLen, "LW4o6 %s: %s", multilang(LANG_LW4O6_STATIC_MODE), multilang(LANG_LW4O6_PSID_ERROR));
					goto SET_FAIL;
				}
				pEntry->lw4o6_psid = val;
			}
		}
	}

	return 0;
SET_FAIL:
	return -1;
}

void showLW4O6Settings(request *wp){
#ifndef CONFIG_GENERAL_WEB
#else
	boaWrite(wp, "<div id=\"LW4O6Div\" style=\"display:none\">\n");

	boaWrite(wp,
			"<div id=\"lw4o6_mode_div\" style=\"display:none\">\n"
			"<table>\n"
			"	<tr>\n"
			"		<th width=30%%>%s:</th>\n"
			"		<td width=70%%>\n"
			"			<input type=\"radio\" value=0 name=\"lw4o6_mode\" id=\"send9\" onclick=lw4o6ModeChange()>%s\n"
			"			<input type=\"radio\" value=1 name=\"lw4o6_mode\" id=\"send10\" onclick=lw4o6ModeChange()>%s\n"
			"		</td>\n"
			"	</tr>\n"
			"</table>\n"
			"</div>\n",multilang(LANG_LW4O6_SETTING_MODE), multilang(LANG_LW4O6_DHCP), multilang(LANG_LW4O6_STATIC));
	boaWrite(wp,
			"<div id=\"lw4o6_static_div\" style=\"display:none\">\n"
			"<table>\n"
			"	<tr>\n"
			"		<th width=30%%>%s:</th>\n"
			"		<td width=70%%><input type=text name=\"lw4o6_aftr_addr\" maxLength=39 size=36></td>\n"
			"	</tr>\n"
			"	<tr>\n"
			"		<th width=30%%>%s:</th>\n"
			"		<td width=70%%><input type=text name=\"lw4o6_local_v6Prefix\" maxlength=39 size=36>\n"
			"		\n <input type=text name=\"lw4o6_local_v6PrefixLen\" size=3 maxlength=3></td>\n"
			"	</tr>\n"
			"		<th width=30%%>%s:</th>\n"
			"		<td width=70%%><input type=text name=\"lw4o6_v4addr\" size=14 maxlength=15></td>\n"
			"		<tr>\n"
			"			<th width=30%%>%s:</th>\n"
			"			<td width=70%%><input type=text name=\"lw4o6_psid_len\" size=2 maxlength=2> (0-12)</td>\n"
			"		</tr>\n"
			"	</tr>\n"
			"		<th width=30%%>%s:</th>\n"
			"		<td width=70%%><input type=text name=\"lw4o6_psid\" size=6 maxlength=6> (eg. 0x34)</td>\n"
			"	</tr>\n"
			"</table>\n"
			"</div>\n"
			"</div>\n",multilang(LANG_LW4O6_AFTR_ADDRESS),multilang(LANG_LW4O6_LOCAL_IPV6PREFIX),multilang(LANG_LW4O6_IPV4_ADDRESS),multilang(LANG_LW4O6_PSID_LENGTH),multilang(LANG_LW4O6_PSID));
#endif
}

#endif


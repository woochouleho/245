#include "webform.h"
#include "../asp_page.h"
#include "multilang.h"

void formAWiFi(request * wp, char *path, char *query)
{
	char *enableStr, *wanIfStr;
	char enable, preEnable;
	int wanIfIndex, preWanIfIdx;
	char *submitUrl;
	char ext_ifname[32],tmpBuf[64];

	enableStr = boaGetVar(wp, "aWiFiCap", "");
	wanIfStr = boaGetVar(wp, "ext_if", "");	
	if (enableStr[0]) {
		enable = enableStr[0] - '0';	
		if(wanIfStr[0])
			wanIfIndex = (int)atoi(wanIfStr);
		else
			wanIfIndex = DUMMY_IFINDEX;  // No interface selected.
		
		if(mib_get_s(MIB_AWIFI_ENABLE, (void *)&preEnable, sizeof(preEnable)) && mib_get_s(MIB_AWIFI_EXT_ITF, (void *)&preWanIfIdx, sizeof(preWanIfIdx))){
			if( (enable == preEnable) && ((wanIfIndex == preWanIfIdx) || (preEnable == 0)))/* No change, So no need to restart wifidog or set mib*/
				goto formAWiFi_redirect;
			
			if( !mib_set(MIB_AWIFI_ENABLE, (void *)&enable ) || !mib_set(MIB_AWIFI_EXT_ITF, (void *)&wanIfIndex )){
				strcpy(tmpBuf, Tset_mib_error);
				goto formAWiFi_err;
			}
			
		}else{
			strcpy(tmpBuf, Tget_mib_error);
			goto formAWiFi_err;
		}	
	}

formAWiFi_end:	
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

#ifdef APPLY_CHANGE
	//printf("[%s:%d] enable=%d(preEnable=%d), wanIfIndex=%d(preWanIfIdx=%d)\n",__FUNCTION__,__LINE__,enable,preEnable,wanIfIndex,preWanIfIdx);
	rtk_wan_aWiFi_wifidog_stop();
	
	if(enable){
		ifGetName(wanIfIndex, ext_ifname, sizeof(ext_ifname));
		rtk_wan_aWiFi_wifidog_start(ext_ifname);
	}
#endif

formAWiFi_redirect:
	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG(submitUrl);
	return;

formAWiFi_err:
	ERR_MSG(tmpBuf);
}

void formAWiFiMAC(request * wp, char *path, char *query)
{
	char *strMacAdd, *strMacDelSel, *strMacDelAll;
	char *strMac, *strCom, *strDelSelect;
	char *submitUrl;
	unsigned int entryNum, i, intVal, delNum = 0;
	MIB_CE_AWIFI_MAC_T Entry, macEntry;
	char tmpBuf[100];

	strMacAdd = boaGetVar(wp, "apply", "");
	strMacDelSel = boaGetVar(wp, "deleteSelAWiFiMac", "");
	strMacDelAll = boaGetVar(wp, "deleteAllAWiFiMac", "");

	entryNum = mib_chain_total(MIB_AWIFI_MAC_TBL);

	// Add Mac
	if(strMacAdd[0]){
		strMac = boaGetVar(wp, "macAddress", "");
		strCom = boaGetVar(wp, "comment", "");
		if (strMac[0]) {
			if ( strlen(strMac) != 12 || !rtk_string_to_hex(strMac, Entry.macAddr, 12)) {
				strcpy(tmpBuf, Tinvalid_source_mac);
				goto setErr_mac;
			}
			if (!isValidMacAddr(Entry.macAddr)) {
				strcpy(tmpBuf, Tinvalid_source_mac);
				goto setErr_mac;
			}
			strncpy(Entry.comment, strCom, sizeof(Entry.comment));
		}else
			return;
			
		if(entryNum >= MAX_AWIFI_MAC_LIST_NUM){
			strcpy(tmpBuf, Texceed_max_rules);
			goto setErr_mac;
		}

		for(i = 0; i < entryNum; i++){
			if (!mib_chain_get(MIB_AWIFI_MAC_TBL, i, (void *)&macEntry)){
	  			boaError(wp, 400, "Get chain record error!\n");
				return;
			}

			if(!memcmp(Entry.macAddr, macEntry.macAddr, 6)){
				strcpy(tmpBuf, "Same MAC Address rule has exist!");
				goto setErr_mac;
			}
		}
		intVal = mib_chain_add(MIB_AWIFI_MAC_TBL, (unsigned char*)&Entry);
		if (intVal == 0) {
			strcpy(tmpBuf, Tadd_chain_error);
			goto setErr_mac;
		}
		else if (intVal == -1) {
			strcpy(tmpBuf, strTableFull);
			goto setErr_mac;
		}
	}

	//Delete select index
	if(strMacDelSel[0]){
		for(i = 0; i < entryNum; i++){
			snprintf(tmpBuf, sizeof(tmpBuf), "select%d", i);
			strDelSelect = boaGetVar(wp, tmpBuf, "");
			if(!strcmp(strDelSelect, "ON")){
				delNum++;
				if(mib_chain_delete(MIB_AWIFI_MAC_TBL, i) != 1) {
					strcpy(tmpBuf, Tdelete_chain_error);
					goto setErr_mac;
				}
			}
		}
		
		if(delNum == 0)
			return;
	}

	//Delect all
	if(strMacDelAll[0]){
		if(entryNum <= 0)
			return;	
		mib_chain_clear(MIB_AWIFI_MAC_TBL); /* clear chain record */
	}

setOK_mac:	
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

#ifdef APPLY_CHANGE
	rtk_wan_aWiFi_stop(-1);
	rtk_wan_aWiFi_start(-1);
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG(submitUrl);
	return;	
	
setErr_mac:
	ERR_MSG(tmpBuf);
}

void formAWiFiURL(request * wp, char *path, char *query)
{
	char *strUrlAdd, *strUrlDelSel, *strUrlDelAll;
	char *strUrl, *strCom, *strDelSelect;
	char *submitUrl;
	unsigned int entryNum, i, intVal, delNum = 0;
	MIB_CE_AWIFI_URL_T Entry, urlEntry;
	char tmpBuf[100];

	strUrlAdd = boaGetVar(wp, "apply", "");
	strUrlDelSel = boaGetVar(wp, "deleteSelAWiFiUrl", "");
	strUrlDelAll = boaGetVar(wp, "deleteAllAWiFiUrl", "");

	entryNum = mib_chain_total(MIB_AWIFI_URL_TBL);

	// Add Url
	if(strUrlAdd[0]){
		strUrl = boaGetVar(wp, "url", "");
		strCom = boaGetVar(wp, "comment", "");
		if (strUrl[0]) {
			strncpy(Entry.url, strUrl, sizeof(Entry.url));
			strncpy(Entry.comment, strCom, sizeof(Entry.comment));
		}else
			return;
			
		if(entryNum >= MAX_AWIFI_URL_LIST_NUM){
			strcpy(tmpBuf, Texceed_max_rules);
			goto setErr_url;
		}

		for(i = 0; i < entryNum; i++){
			if (!mib_chain_get(MIB_AWIFI_URL_TBL, i, (void *)&urlEntry)){
				boaError(wp, 400, "Get chain record error!\n");
				return;
			}

			if(!strcmp(Entry.url, urlEntry.url)){
				strcpy(tmpBuf, "Same URL rule has exist!");
				goto setErr_url;
			}
		}
		intVal = mib_chain_add(MIB_AWIFI_URL_TBL, (unsigned char*)&Entry);
		if (intVal == 0) {
			strcpy(tmpBuf, Tadd_chain_error);
			goto setErr_url;
		}
		else if (intVal == -1) {
			strcpy(tmpBuf, strTableFull);
			goto setErr_url;
		}
	}

	//Delete select index
	if(strUrlDelSel[0]){
		for(i = 0; i < entryNum; i++){
			snprintf(tmpBuf, sizeof(tmpBuf), "select%d", i);
			strDelSelect = boaGetVar(wp, tmpBuf, "");
			if(!strcmp(strDelSelect, "ON")){
				delNum++;
				if(mib_chain_delete(MIB_AWIFI_URL_TBL, i) != 1) {
					strcpy(tmpBuf, Tdelete_chain_error);
					goto setErr_url;
				}
			}
		}
		
		if(delNum == 0)
			return;
	}

	//Delect all
	if(strUrlDelAll[0]){
		if(entryNum <= 0)
			return; 
		mib_chain_clear(MIB_AWIFI_URL_TBL); /* clear chain record */
	}

setOK_url:
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

#ifdef APPLY_CHANGE
	rtk_wan_aWiFi_stop(-1);
	rtk_wan_aWiFi_start(-1);
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG(submitUrl);
	return; 
	
setErr_url:
	ERR_MSG(tmpBuf);
}


#ifdef CONFIG_USER_AWIFI_AUDIT_SUPPORT
void formAWiFiAudit(request * wp, char *path, char *query)
{
	char *str, *submitUrl;
	unsigned char awifi_audit_type;
	char tmpBuf[64];

	str = boaGetVar(wp, "AWiFiAuditCap", "");
	if (str[0]) {
		awifi_audit_type = str[0] - '0';
		if (!mib_set(MIB_AWIFI_AUDIT_TYPE, &awifi_audit_type)) {
			strcpy(tmpBuf, Tset_mib_error);
			goto formAWiFiAudit_err;
		}
	}

formAWiFiAudit_end:
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

#ifdef APPLY_CHANGE
	rtk_wan_aWiFi_stop(-1);
	rtk_wan_aWiFi_start(-1);
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG(submitUrl);
	return;

formAWiFiAudit_err:
	ERR_MSG(tmpBuf);
}
#endif

int showAWiFiMACTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	unsigned int entryNum, i;
	MIB_CE_AWIFI_MAC_T Entry;
	char tmpBuf[20];

	nBytesSent += boaWrite(wp, "<tr>"
		"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
		"<td align=center width=\"40%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
		"<td align=center width=\"40%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td></tr>\n",
		multilang(LANG_SELECT), multilang(LANG_AWIFI_MAC_ADDRESS),multilang(LANG_AWIFI_COMMENT));

	entryNum = mib_chain_total(MIB_AWIFI_MAC_TBL);
	for(i = 0; i < entryNum; i++){
		if (!mib_chain_get(MIB_AWIFI_MAC_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

		if ( Entry.macAddr[0]==0 && Entry.macAddr[1]==0
		    && Entry.macAddr[2]==0 && Entry.macAddr[3]==0
		    && Entry.macAddr[4]==0 && Entry.macAddr[5]==0 ) {
			strcpy(tmpBuf, "------");
		}else {
			snprintf(tmpBuf, 100, "%02x:%02x:%02x:%02x:%02x:%02x",
				Entry.macAddr[0], Entry.macAddr[1], Entry.macAddr[2],
				Entry.macAddr[3], Entry.macAddr[4], Entry.macAddr[5]);
		}

		nBytesSent += boaWrite(wp, "<tr>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>\n"
		"<td align=center width=\"40%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
		"<td align=center width=\"40%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td></tr>\n",
		i, tmpBuf, Entry.comment);
	}
	return nBytesSent;
}

int showAWiFiURLTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	unsigned int entryNum, i;
	MIB_CE_AWIFI_URL_T Entry;
	
	nBytesSent += boaWrite(wp, "<tr>"
		"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
		"<td align=center width=\"40%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
		"<td align=center width=\"40%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td></tr>\n",
		multilang(LANG_SELECT), multilang(LANG_AWIFI_URL),multilang(LANG_AWIFI_COMMENT));

	entryNum = mib_chain_total(MIB_AWIFI_URL_TBL);
	for(i = 0; i < entryNum; i++){
		if (!mib_chain_get(MIB_AWIFI_URL_TBL, i, (void *)&Entry))
		{
			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

		nBytesSent += boaWrite(wp, "<tr>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>\n"
		"<td align=center width=\"40%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
		"<td align=center width=\"40%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td></tr>\n",
		i, Entry.url, Entry.comment);
	}
	return nBytesSent;
}



/*
 *      Web server handler routines for VPN passthorugh
 *
 */

#include <string.h>
#include <signal.h>
/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"


#ifdef CONFIG_USER_BOA_PRO_PASSTHROUGH  //VPN_passthrough_enable

int VpnPassThrulist(int eid, request * wp, int argc, char **argv)
{
   	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char wanname[IFNAMSIZ];
	int nBytesSent=0;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	boaWrite(wp,
		"<div class=\"data_common data_common_notitle\">"
		"<table><tr><td width\"100%%\"><input type=\"hidden\" value=%d name=\"totalNUM\"></td></tr>"
		"</table></div>",entryNum);

    for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

		if(!isInterfaceMatch(Entry.ifIndex))
			continue;

		if (Entry.enable == 0 || !isValidMedia(Entry.ifIndex))
			continue;

 		ifGetName(Entry.ifIndex, wanname, sizeof(wanname));
#if 0
		nBytesSent += boaWrite(wp,
			"<tr><td width=\"10%\">%s</td>"
			"<td width=\"30%\"><input type=\"checkbox\" name=\"mutli_PassThru1WAN_%d\" id=\"mutli_PassThru1WAN_%d\" value=\"ON\" onclick=\"updateIpsecState(%d)\"</td>"
			"<td width=\"30%\"><input type=\"checkbox\" name=\"mutli_PassThru2WAN_%d\" id=\"mutli_PassThru2WAN_%d\" value=\"ON\" onclick=\"updatePptpState(%d)\"</td>"
			"<td width=\"30%\"><input type=\"checkbox\" name=\"mutli_PassThru3WAN_%d\" id=\"mutli_PassThru3WAN_%d\" value=\"ON\" onclick=\"updateL2tpState(%d)\"</td></tr>",
			wanname,i,i,i, i,i,i, i,i,i);
#else

if(0){
	nBytesSent += boaWrite(wp,
		"<div class=\"data_common data_common_notitle\">"
		"<table>"
		"<tr><td width=\"10%\" align=\"center\">%s</td>"
		"<td width=\"25%\" align=\"center\"><input type=\"checkbox\" name=\"mutli_PassThru1WAN_%d\" value=\"ON\" onclick=\"updateIpsecState()\"</td>"
		"<td width=\"25%\" align=\"center\"><input type=\"checkbox\" name=\"mutli_PassThru2WAN_%d\" value=\"ON\" onclick=\"updatePptpState()\"</td>"
		"<td width=\"25%\" align=\"center\"><input type=\"checkbox\" name=\"mutli_PassThru3WAN_%d\" value=\"ON\" onclick=\"updateL2tpState()\"</td></tr>"
		"</table></div>",
		wanname,i,i,i);

}else{
		nBytesSent += boaWrite(wp,
        "<div class=\"data_common data_common_notitle\">"
	    "<table>"
		"<tr>"
		"<td width=10%% align=center>%s</td>"
		"<td width=\"25%%\" align=center><input type=\"checkbox\" name=\"mutli_PassThru1WAN_%d\" value=\"ON\" onclick=\"updateIpsecState()\"</td>"
		"<td width=\"25%%\" align=center><input type=\"checkbox\" name=\"mutli_PassThru2WAN_%d\" value=\"ON\" onclick=\"updatePptpState()\"</td>"
		"<td width=\"25%%\" align=center><input type=\"checkbox\" name=\"mutli_PassThru3WAN_%d\" value=\"ON\" onclick=\"updateL2tpState()\"</td>"
		"</tr>"
		"</table></div>",
		wanname,i,i,i);
}


#endif

	}

	return nBytesSent;

}

void formVpnPassThru(request * wp, char *path, char *query)
{
    char *strVal,*submitUrl;
	unsigned int entryNum;
	MIB_CE_ATM_VC_T Entry;
	char buf[256] ={0};

	int index = 0;

    entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	mib_set(TOTAL_WAN_CHAIN_NUM, (void *)&entryNum);

	for(index=0;index<entryNum;index++){

	    if (!mib_chain_get(MIB_ATM_VC_TBL, index, (void *)&Entry))
		{
  			boaError(wp, 400, "Get chain record error!\n");
			continue;
		}

		snprintf(buf, sizeof(buf), "mutli_PassThru1WAN_%d", index);
		strVal = boaGetVar(wp, buf, "");
		if(!gstrcmp(strVal, "ON"))
		  Entry.vpnpassthru_ipsec_enable = 1;
	    else
		  Entry.vpnpassthru_ipsec_enable = 0;

		snprintf(buf, sizeof(buf), "mutli_PassThru2WAN_%d", index);
		strVal = boaGetVar(wp, buf, "");
		if(!gstrcmp(strVal, "ON"))
		  Entry.vpnpassthru_pptp_enable = 1;
	    else
		  Entry.vpnpassthru_pptp_enable = 0;

		snprintf(buf, sizeof(buf), "mutli_PassThru3WAN_%d", index);
		strVal = boaGetVar(wp, buf, "");
		if(!gstrcmp(strVal, "ON"))
		  Entry.vpnpassthru_l2tp_enable = 1;
	    else
		  Entry.vpnpassthru_l2tp_enable = 0;

		mib_chain_update(MIB_ATM_VC_TBL, (void *)&Entry, index);
	}


#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	OK_MSG(submitUrl);

#if defined(APPLY_CHANGE)
	Setup_VpnPassThrough();
#endif

}

#endif/*CONFIG_VPNPASSTHRU*/



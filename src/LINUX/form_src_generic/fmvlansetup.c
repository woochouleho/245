/*
 *      Web server handler routines for NET
 *
 */

/*-- System inlcude files --*/
#include <config/autoconf.h>
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include <stdlib.h>
#define LANIF_NUM	PMAP_ITF_END

int initPageLanVlan(int eid, request * wp, int argc, char ** argv)
{
	int total, i, j;
	MIB_VLAN_GROUP_T group_Entry;
	MIB_CE_SW_PORT_T port_Entry;
	char vlan_str[40]={0};
	char vlan_str_tmp[40]={0};
	char vlanlist[64] = {0};

	total = mib_chain_total(MIB_VLAN_GROUP_TBL);

	for (i = 0; i < SW_LAN_PORT_NUM; i++)
	{
		mib_chain_get(MIB_SW_PORT_TBL, i, (void*)&port_Entry);
		memcpy(vlanlist, port_Entry.trunkVlanlist, sizeof(vlanlist)-1);
		boaWrite(wp, "setValue('Trunk%d', '%s');\n\t", i, vlanlist);
		boaWrite(wp, "setValue('Mode%d', %d);\n\t", i, port_Entry.pb_mode);
		for (j=0; j<total; j++)
		{
			mib_chain_get(MIB_VLAN_GROUP_TBL, j, (void*)&group_Entry);
			if(group_Entry.portMask & 1<<i)
			{
				if (strlen(vlan_str) == 0)
				{
					sprintf(vlan_str, "%d/%d", group_Entry.vid, group_Entry.vprio);
				}
				else
				{
					sprintf(vlan_str, "%s,%d/%d", vlan_str_tmp, group_Entry.vid, group_Entry.vprio);
				}
				strncpy(vlan_str_tmp,vlan_str,sizeof(vlan_str));
			}

		}
		boaWrite(wp, "setValue('VLAN%d', '%s');\n\t", i, vlan_str);
		memset(vlan_str, 0, sizeof(vlan_str));
		memset(vlan_str_tmp, 0, sizeof(vlan_str_tmp));
	}
	return 0;
}

void formLanVlanSetup(request * wp, char *path, char *query)
{
	char *strData, *token;
	char *submitUrl;
	char vlanlist[64] = {0};
	int total, i, ifidx, nVal, vid, priority, hit = 0;
	char tmpBuf[100] = {0};
	MIB_VLAN_GROUP_T group_Entry;
	MIB_CE_SW_PORT_T port_Entry;

	strData = boaGetVar(wp, "if_index", "");
	ifidx = atoi(strData);
	if (ifidx < 0 || ifidx >= LANIF_NUM) {
		strcpy(tmpBuf, strModChainerror);
		goto setErr_vmap;
	}
	mib_chain_get(MIB_SW_PORT_TBL, ifidx, (void*)&port_Entry);
	strData = boaGetVar(wp, "Frm_Mode", "");
	nVal = atoi(strData);
	port_Entry.pb_mode = nVal;
	mib_chain_update(MIB_SW_PORT_TBL, (void *)&port_Entry,ifidx);	
	config_vlan_group(CONFIGALL,NULL,0,ifidx);
	clear_VLAN_grouping_by_port(ifidx);
	clear_trunkSetting(ifidx);
	
	strData = boaGetVar(wp, "Frm_VLANPRI", "");

	if ( port_Entry.pb_mode == LAN_TRUNK_BASED_MODE )
	{
		if ( strlen(strData) >= sizeof(vlanlist) )
			strncpy(vlanlist, strData, sizeof(vlanlist) - 1);
		else
			strncpy(vlanlist, strData, strlen(strData));
		strcpy(port_Entry.trunkVlanlist, vlanlist);
		mib_chain_update(MIB_SW_PORT_TBL, (void *)&port_Entry, ifidx);
	}
	else if(port_Entry.pb_mode == LAN_VLAN_BASED_MODE)
	{
		total = mib_chain_total(MIB_VLAN_GROUP_TBL);
		
		token=strtok(strData,",");

		while (token!=NULL) {
			hit = 0;
			sscanf(token,"%d/%d", &vid, &priority);
			for (i=0;i<total;i++) {
				mib_chain_get(MIB_VLAN_GROUP_TBL, i, (void*)&group_Entry);
				if(group_Entry.vid == vid && group_Entry.vprio == priority){
					group_Entry.portMask |= 1<<ifidx;
					mib_chain_update(MIB_VLAN_GROUP_TBL, (void *)&group_Entry,i);
					setup_vlan_group(&group_Entry, 1, ifidx);
					hit = 1;
					break;
				}
			}
			if(hit == 0)
			{
				group_Entry.enable = 1;
				group_Entry.itfGroup = 0;
				group_Entry.vid = vid;
				group_Entry.vprio = priority;
				group_Entry.portMask = 1<<ifidx;
				group_Entry.txTagging = 1;
				group_Entry.instnum = 0;
				mib_chain_add(MIB_VLAN_GROUP_TBL, (void *)&group_Entry);
				setup_vlan_group(&group_Entry, 1, ifidx);
			}
			token = strtok(NULL, ",");
		}
	}
	Set_LanPort_Def(ifidx);	
	// sync with port-based mapping
	//if (pbEntry.pb_mode!=0 && org_mode != pbEntry.pb_mode)
	//	sync_itfGroup(ifidx);
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	//setupnewEth2pvc();

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page

	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;
setErr_vmap:
	ERR_MSG(tmpBuf);

}

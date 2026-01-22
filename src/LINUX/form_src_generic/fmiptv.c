
#include <config/autoconf.h>
#include "../webs.h"
//#include "fmdefs.h"
#include "mib.h"
#include "utility.h"
#include "subr_net.h"
#include <stdlib.h>
#include "../../port.h"

//#ifdef CONFIG_MCAST_VLAN
void formMcastVlanMapping(request * wp, char *path, char *query)
{
	char *strData;
	char *submitUrl;
	char *sWanName;
	int IgmpVlan;
	int IfIndex;
	int ifidx, entryNum, i, chainNum=-1;
	MIB_CE_ATM_VC_T entry;
	
	strData = boaGetVar(wp, "if_index", "");
	ifidx = atoi(strData);
	strData = boaGetVar(wp, "mVlan", "");
	IgmpVlan = atoi(strData);
	if(ifidx == -1 || IgmpVlan ==-1)
		goto back2add;
	sWanName = boaGetVar(wp, "WanName", "");
	IfIndex = getIfIndexByName(sWanName);

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, &entry)) {
			printf("get MIB chain error\n");
			return;
		}
		if(IfIndex == entry.ifIndex){
			printf("%s-%d i=%x\n",__func__,__LINE__,i);
			chainNum = i;
			break;
		}
	}
	
	if(chainNum != -1 && entry.mVid != IgmpVlan)
	{
		entry.mVid = IgmpVlan;
		//printf("%s-%d IgmpVlan=%d entry.mVid=%d chainNum=%d\n",__func__,__LINE__,IgmpVlan, entry.mVid,chainNum);
		if(chainNum != -1)
			mib_chain_update(MIB_ATM_VC_TBL, (void *)&entry, chainNum);
		
		deleteConnection(CONFIGONE, &entry);
		restartWAN(CONFIGONE, &entry); //need reconfig WAN
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
	}
	back2add: /*mean user cancel modify, refresh web page again!*/
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;


}
//#endif


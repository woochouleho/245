#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <time.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/ioctl.h>

/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "utility.h"
#include "../defs.h"
#include "debug.h"
#include "multilang.h"

#ifdef CONFIG_USER_RTK_BRIDGE_MODE
void formInternetMode(request * wp, char *path, char *query)
{	
	char *strOpMode=NULL, *submitUrl=NULL;
	char tmpBuf[64]={0};
	int opmode_changed_flag=0;

	int tmpVal;
	unsigned char origVal, curVal;
	mib_get(MIB_OP_MODE, (void *)&origVal);
	
	strOpMode = boaGetVar(wp, "opmode", "");

#if defined(WLAN_UNIVERSAL_REPEATER) && defined(CONFIG_USER_RTK_REPEATER_MODE)	
	int rptmode_changed_flag=0;
	MIB_CE_MBSSIB_T Entry;
	int BUF_SIZE_TEMP = 20;
	char wlan_vxd_if[BUF_SIZE_TEMP];
	int i = 0;
	unsigned char curOpmode=0, origRptmode=0;

	mib_get(MIB_REPEATER_MODE, (void *)&origRptmode);
	if(strOpMode[0])
	{
		sscanf(strOpMode, "%d", &tmpVal);
		curVal=(unsigned char)tmpVal;
		if(curVal == 0){        //GATEWAY
			curOpmode = 0;
			if(origRptmode == 1)
				rptmode_changed_flag = 1;
		}else if(curVal == 1){  //BRIDGE
			curOpmode = 1;
			if(origRptmode == 1)
				rptmode_changed_flag = 1;
			}
		else{                  //REPEATER
			curOpmode = 1;
			if(origRptmode == 0)
				rptmode_changed_flag = 1;
			}
		if((curOpmode != origVal) || rptmode_changed_flag)
		{
			opmode_changed_flag = 1;
		}
	}
	if(opmode_changed_flag == 1)
	{
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
		cleanSystem();
#endif
		
		if (!mib_set(MIB_OP_MODE, (void *)&curOpmode)) 
		{
			strcpy(tmpBuf, "multilang(LANG_SET_MIB_OP_MODE_FAIL)");
			goto setErr_opmode;
		}
		if(curVal==3) // repeater mode
		{
			char rptr_enabled = 1;
			
#ifdef CONFIG_ANDLINK_SUPPORT		
			mib_set(MIB_RTL_LINK_MODE, (void *)&rptr_enabled);
#endif		

			curVal = 1;
			mib_set(MIB_OP_MODE, (void *)&curVal);
			mib_set(MIB_REPEATER_MODE, (void *)&rptr_enabled);

			mib_save_wlanIdx();
			for(i=0; i<NUM_WLAN_INTERFACE;++i){
				snprintf(wlan_vxd_if, BUF_SIZE, VXD_IF, i);
				SetWlan_idx(wlan_vxd_if);
				mib_set(MIB_REPEATER_ENABLED1, (void *)&rptr_enabled);
				
				mib_chain_get(MIB_MBSSIB_TBL, WLAN_REPEATER_ITF_INDEX, (void *)&Entry);
				Entry.wlanDisabled = 0;
				Entry.wlanMode=CLIENT_MODE;
				mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, WLAN_REPEATER_ITF_INDEX);
			}	
			mib_recov_wlanIdx();		
		}else //opmode is gw or bridge
		{
			char rpt_enabled=0;
			
#ifdef CONFIG_ANDLINK_SUPPORT
			char rtl_link_mode;
	        if(curVal == BRIDGE_MODE){
	            rtl_link_mode = 0;
	            mib_set(MIB_RTL_LINK_MODE, (void *)&rtl_link_mode);
	        }else{
	            rtl_link_mode = 2;
	            mib_set(MIB_RTL_LINK_MODE, (void *)&rtl_link_mode);
	        }
#endif
			if(rptmode_changed_flag){
				mib_set(MIB_REPEATER_MODE, (void *)&rpt_enabled);
				mib_save_wlanIdx();
				for(i=0; i<NUM_WLAN_INTERFACE;++i){
					snprintf(wlan_vxd_if, BUF_SIZE, VXD_IF, i);
					SetWlan_idx(wlan_vxd_if);
					mib_set(MIB_REPEATER_ENABLED1, (void *)&rpt_enabled);
					
					mib_chain_get(MIB_MBSSIB_TBL, WLAN_REPEATER_ITF_INDEX, (void *)&Entry);
					Entry.wlanDisabled = 1;
					mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, WLAN_REPEATER_ITF_INDEX);
				}	
				mib_recov_wlanIdx();
			}
		}
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
		reStartSystem();	
#endif

		submitUrl = boaGetVar(wp, "submit-url", "");
		//OK_MSG(submitUrl);
	}
#else
	
	if(strOpMode[0])
	{
		sscanf(strOpMode, "%d", &tmpVal);
		curVal=(unsigned char)tmpVal;		
		
		if(curVal != origVal)
		{
			opmode_changed_flag = 1;
		}
	}
	if(opmode_changed_flag == 1)
	{
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
		cleanSystem();
#endif		
		if (!mib_set(MIB_OP_MODE, (void *)&curVal)) 
		{
			strcpy(tmpBuf, "multilang(LANG_SET_MIB_OP_MODE_FAIL)");
			goto setErr_opmode;
		}
	
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
		reStartSystem();
#endif

		submitUrl = boaGetVar(wp, "submit-url", "");
		//OK_MSG(submitUrl);
	}
#endif
	else
	{
		submitUrl = boaGetVar(wp, "submit-url", "");
		if (submitUrl[0])
			boaRedirect(wp, submitUrl);
		else
			boaDone(wp, 200);
	}
	
	return;
	
setErr_opmode:
	ERR_MSG(tmpBuf);
	
}
#endif


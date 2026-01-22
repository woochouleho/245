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
#include "options.h"
#if defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_gpon.h>
#else
#include "rtk/gpon.h"
#endif
#include "sysconfig.h"

/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"

int fmgpon_checkWrite(int eid, request * wp, int argc, char **argv)
{
	char *name;
	char tmpBuf[100];
	
	if (boaArgs(argc, argv, "%s", &name) < 1) 
	{
   		boaError(wp, 400, "Insufficient args\n");
   		return -1;
   	}

	if(!strcmp(name, "fmgpon_loid")) 	
	{			
		if(!mib_get_s(MIB_LOID,  (void *)tmpBuf, sizeof(tmpBuf)))		
		{	  		
			sprintf(tmpBuf, "%s (GPON LOID)",Tget_mib_error);			
			goto setErr;		
		}		
		boaWrite(wp, "%s", tmpBuf);		
		return 0;	
	}
	
	if(!strcmp(name, "fmgpon_loid_password")) 
	{
		if(!mib_get_s(MIB_LOID_PASSWD,  (void *)tmpBuf, sizeof(tmpBuf)))
		{
			sprintf(tmpBuf, "%s (GPON LOID Password)",Tget_mib_error);			
			goto setErr;
		}

		boaWrite(wp, "%s", tmpBuf);
		return 0;
	}

	if(!strcmp(name, "fmgpon_ploam_password")) 
	{
		if(!mib_get_s(MIB_GPON_PLOAM_PASSWD,  (void *)tmpBuf, sizeof(tmpBuf)))
		{
			sprintf(tmpBuf, "%s (GPON PLOAM Password)",Tget_mib_error);			
			goto setErr;
		}

		boaWrite(wp, "%s", tmpBuf);
		return 0;
	}

	if(!strcmp(name, "fmgpon_sn")) 
	{
		char sn[64] = {0};
		if(!mib_get_s(MIB_GPON_SN,  (void *)sn, sizeof(sn)))
		{
			sprintf(tmpBuf, "%s (GPON Serial Number)",Tget_mib_error);			
			goto setErr;
		}
#ifdef CONFIG_00R0
		sprintf(tmpBuf, "%02X%02X%02X%02X%s", sn[0], sn[1], sn[2], sn[3], &sn[4]);
#else
		sprintf(tmpBuf, "%s", sn);
#endif
		boaWrite(wp, "%s", tmpBuf);
		return 0;
	}

	if( !strcmp(name, "ploam_pw_length") )
	{
		unsigned int gpon_speed=0;
		if(!mib_get_s(MIB_PON_SPEED, (void *)&gpon_speed, sizeof(gpon_speed)))
		{
			sprintf(tmpBuf, "%s (GPON Serial Number)",Tget_mib_error);			
			goto setErr;
		}

		if(gpon_speed==0){
			boaWrite(wp, "%d", GPON_PLOAM_PASSWORD_LENGTH);
		}
		else{
			boaWrite(wp, "%d", NGPON_PLOAM_PASSWORD_LENGTH);
		}
		return 0;
	}

#if defined(CONFIG_00R0)
	if(!strcmp(name, "omci_mc_mode"))
	{
		int mcast = 0;

		mib_get_s(MIB_OMCI_CUSTOM_MCAST, &mcast, sizeof(mcast));
		if(mcast & 1)
			return boaWrite(wp, "1");
		else
			return boaWrite(wp, "0");
	}

#if defined(CONFIG_TR142_MODULE)
	/* Page GPON Type */
	if(!strcmp(name, "fmgpon_type_init"))
	{
		unsigned char type;
		unsigned int portmask;

		if(!mib_get_s(MIB_DEVICE_TYPE,  &type, sizeof(type)))
		{
			sprintf(tmpBuf, "%s (Device Type)",Tget_mib_error);
			goto setErr;
		}

		if(!mib_get_s(MIB_GPON_OMCI_PORTMASK,  &portmask, sizeof(portmask)))
		{
			sprintf(tmpBuf, "%s (GPON OMCI portmask)",Tget_mib_error);
			goto setErr;
		}


		boaWrite(wp, "var dev_type=%d\n", type);
		boaWrite(wp, "var pmask=%d\n", portmask);
		return 0;
	}
#endif	// CONFIG_TR142_MODULE
#endif	// CONFIG_00R0

setErr:
	ERR_MSG(tmpBuf);
	return -1;
}

///////////////////////////////////////////////////////////////////
void formgponConf(request * wp, char *path, char *query)
{
	char	*strData,*strLoid,*strLoidPasswd;
	char tmpBuf[100];
	//char omcicli[128];
	int retVal=0;

	strLoid = boaGetVar(wp, "fmgpon_loid", "");	
	if ( strLoid[0] )	
	{		
		//printf("===>[%s:%d] fmgpon_loid=%s\n",__func__,__LINE__,strLoid);		
		sprintf(tmpBuf, strLoid,strlen(strLoid));
		if(!mib_set(MIB_LOID, strLoid))		
		{
			sprintf(tmpBuf, " %s (GPON LOID).",Tset_mib_error);			
			goto setErr;		
		}	
		mib_set(MIB_LOID_OLD, tmpBuf);
	}
	else
	{
		mib_set(MIB_LOID, strLoid);
	}

	strLoidPasswd = boaGetVar(wp, "fmgpon_loid_password", "");	
	if ( strLoidPasswd[0] )
	{		
		//printf("===>[%s:%d] fmgpon_loid_password=%s\n",__func__,__LINE__,strLoidPasswd);		
		sprintf(tmpBuf, strLoidPasswd,strlen(strLoidPasswd));
		if(!mib_set(MIB_LOID_PASSWD, strLoidPasswd))		
		{			
			sprintf(tmpBuf, " %s (GPON LOID Password).",Tset_mib_error);			
			goto setErr;		
		}	
		mib_set(MIB_LOID_PASSWD_OLD, tmpBuf);
	}
	else
	{
		mib_set(MIB_LOID_PASSWD, strLoidPasswd);
	}
	
	if(strLoid[0] && strLoidPasswd[0])
	{
		va_cmd("/bin/omcicli", 4, 1, "set", "loid", strLoid,strLoidPasswd);
		//sprintf(omcicli, "/bin/omcicli set loid %s %s", strLoid,strLoidPasswd);
		//fprintf(stderr, "OMCICLI : %s \n" , omcicli);
		//system(omcicli);
	}else if ( strLoid[0] && !strLoidPasswd[0] )
	{
		va_cmd("/bin/omcicli", 3, 1, "set", "loid", strLoid);
		//sprintf(omcicli, "/bin/omcicli set loid %s", strLoid);
		//fprintf(stderr, "OMCICLI : %s \n" , omcicli);
		//system(omcicli);
	}

	strData = boaGetVar(wp, "fmgpon_ploam_password", "");
	if ( strData[0] )
	{
		//Password: 10 characters.
		unsigned int gpon_speed=0;
		int ploam_pw_length=GPON_PLOAM_PASSWORD_LENGTH;
		unsigned char password_hex[MAX_NAME_LEN]={0};
		char oamcli_cmd[128]={0};
										
		mib_get(MIB_PON_SPEED, (void *)&gpon_speed);
		if(gpon_speed==0){
			ploam_pw_length=GPON_PLOAM_PASSWORD_LENGTH;
		}
		else{
			ploam_pw_length=NGPON_PLOAM_PASSWORD_LENGTH;
		}
		formatPloamPasswordToHex(strData, password_hex, ploam_pw_length);

		//Since OMCI has already active this, so need to deactive
		printf("GPON deActivate.\n");
#ifdef CONFIG_COMMON_RT_API
		if(rt_gpon_deactivate() != RT_ERR_OK)
			printf("rt_gpon_deactivate failed: %s %d\n", __func__, __LINE__);
		sprintf(oamcli_cmd , "/sbin/diag rt_gpon set registrationId-hex %s", password_hex);
#else
		rtk_gpon_deActivate();
		sprintf(oamcli_cmd , "/sbin/diag gpon set password-hex %s", password_hex);
#endif
		system(oamcli_cmd);

		if(!mib_set(MIB_GPON_PLOAM_PASSWD, strData))
		{
			sprintf(tmpBuf, " %s (GPON PLOAM Password).",Tset_mib_error);			
			goto setErr;
		}
#ifdef CONFIG_00R0
		if(!mib_set(MIB_GPON_PLOAM_PASSWD_BACKUP, strData))
		{
			sprintf(tmpBuf, " %s (GPON PLOAM Password backup).",Tset_mib_error);
			goto setErr;
		}
#endif

		//Active GPON again
		printf("GPON Activate again.\n");
#ifdef CONFIG_COMMON_RT_API
		rt_gpon_activate(RT_GPON_ONU_INIT_STATE_O1);
#else
		rtk_gpon_activate(RTK_GPONMAC_INIT_STATE_O1);
#endif
	}
	else{
		mib_set(MIB_GPON_PLOAM_PASSWD, strData);
	}

#ifdef CONFIG_GPON_FEATURE
	char vChar;
	strData = boaGetVar(wp, "omci_olt_mode", "");
	if ( strData[0] )
	{
		vChar = strData[0] - '0';
		if(checkOMCImib(vChar)){
			if(!mib_set(MIB_OMCI_OLT_MODE, (void *)&vChar))
			{
				strcpy(tmpBuf, "Save OMCI OLT MODE Error");
				goto setErr;
			}
			restartOMCIsettings();
		}
	}
#endif
	
	strData = boaGetVar(wp, "submit-url", "");

	OK_MSG(strData);
#ifdef COMMIT_IMMEDIATELY
	Commit();
	mib_update(HW_SETTING, CONFIG_MIB_ALL);
#endif
	return;

setErr:
	ERR_MSG(tmpBuf);
}

#if defined(CONFIG_00R0)
#if defined(CONFIG_TR142_MODULE)
int showPonLanPorts(int eid, request * wp, int argc, char **argv)
{
	int i;
	int cnt = 0;

	cnt += boaWrite(wp, "<table id=tbl_lan border=0 width=800 cellspacing=4 cellpadding=0>\n"
			"<tr><td colspan=5><hr size=2 align=top></td></tr>\n"
			"<tr nowrap><td width=150px><font size=2><b>%s</b></font>"
			"</td><td>&nbsp;</td></tr>\n", multilang(LANG_PORTS_MANAGED_BY_OMCI));
	cnt += boaWrite(wp, "<tr nowrap>");
	for (i=PMAP_ETH0_SW0; i < PMAP_WLAN0; i++) {
		if (i < SW_LAN_PORT_NUM) {
			if (!(i&0x1))
				cnt += boaWrite(wp, "<tr nowrap>");
			cnt += boaWrite(wp, "<td><font size=2><input type=checkbox name=chkpt>LAN_%d</font></td><tr>\n", i+1);
		}
		else
			cnt += boaWrite(wp, "<input type=hidden name=chkpt>\n");
	}

	cnt += boaWrite(wp, "</table>\n");

	return cnt;
}

void fmgponType(request * wp, char *path, char *query)
{
	char *buf = {0};
	unsigned char dev_type = 1;
	unsigned int portmask = 0;

	buf = boaGetVar(wp, "device_type", "");
	if(!buf[0])
		return;

	dev_type = buf[0] - '0';
	mib_set(MIB_DEVICE_TYPE, &dev_type);

	if(dev_type == 2)
	{
		buf = boaGetVar(wp, "portmask", "");
		if(buf[0])
		{
			portmask = atoi(buf);
			mib_set(MIB_GPON_OMCI_PORTMASK, &portmask);
		}
	}

	/* formReboot will also commit changes. */
	formReboot(wp, path, query);
}
#endif

void formOmciMcMode(request * wp, char *path, char *query)
{
	char *buf = {0};
	int mc_mode = 0;
	int value = 0;

	buf = boaGetVar(wp, "omci_mc_mode", "");
	if(!buf[0])
		return;

	mc_mode = buf[0] - '0';
	mib_get_s(MIB_OMCI_CUSTOM_MCAST, &value, sizeof(value));

	if(mc_mode == 1)
		value |= 1;
	else
		value &= ~1;

	mib_set(MIB_OMCI_CUSTOM_MCAST, &value);

	/* formReboot will also commit changes. */
	formReboot(wp, path, query);
}

#endif	//CONFIG_00R0

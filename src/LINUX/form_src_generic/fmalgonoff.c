/*
 * =====================================================================================
 *
 *       Filename:  fmalgonoff.c
 *
 *    Description:  control the on-off of ALG by the web
 *
 *        Version:  1.0
 *        Created:  08/16/07 15:54:05
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ramen.Shen (Mr), ramen_shen@realsil.com.cn
 *        Company:  REALSIL Microelectronics Inc
 *
 * =====================================================================================
 */

/*-- System inlcude files --*/
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/route.h>

/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#ifdef EMBED
#include <linux/config.h>
#else
#include "../../../../include/linux/autoconf.h"
#endif

#ifdef USE_LOGINWEB_OF_SERVER
extern unsigned char g_login_username[];
#endif

#ifdef CONFIG_USER_RTK_NAT_ALG_PASS_THROUGH
enum algtype{
#ifdef CONFIG_NF_CONNTRACK_FTP_MODULE
    ALG_TYPE_FTP,
#endif
#ifdef CONFIG_NF_CONNTRACK_TFTP_MODULE
    ALG_TYPE_TFTP,
#endif
#ifdef CONFIG_NF_CONNTRACK_H323_MODULE
    ALG_TYPE_H323,
#endif
#ifdef CONFIG_NF_CONNTRACK_IRC_MODULE
    ALG_TYPE_IRC,
 #endif
 #ifdef CONFIG_NF_CONNTRACK_RTSP_MODULE
    ALG_TYPE_RTSP,
 #endif
 #ifdef CONFIG_NF_CONNTRACK_QUAKE3_MODULE
    ALG_TYPE_QUAKE3,
 #endif
 #ifdef CONFIG_NF_CONNTRACK_CUSEEME_MODULE
    ALG_TYPE_CUSEEME,
 #endif
 #ifdef CONFIG_NF_CONNTRACK_L2TP_MODULE
    ALG_TYPE_L2TP,
 #endif
 #ifdef CONFIG_NF_CONNTRACK_IPSEC_MODULE
    ALG_TYPE_IPSEC,
 #endif
#if defined(CONFIG_NF_CONNTRACK_SIP_MODULE)
    ALG_TYPE_SIP,
#endif
 #ifdef CONFIG_NF_CONNTRACK_PPTP_MODULE
    ALG_TYPE_PPTP,
#endif
    ALG_TYPE_MAX
};
struct {
	unsigned char id;
	unsigned int mibalgid;
	char* name;
}algTypeName[]={
#ifdef CONFIG_NF_CONNTRACK_FTP_MODULE
{ALG_TYPE_FTP,MIB_IP_ALG_FTP,"FTP"},
#endif
#ifdef CONFIG_NF_CONNTRACK_TFTP_MODULE
{ALG_TYPE_TFTP,MIB_IP_ALG_TFTP,"TFTP"},
#endif
#ifdef CONFIG_NF_CONNTRACK_H323_MODULE
{ALG_TYPE_H323,MIB_IP_ALG_H323,"H323"},
#endif
#ifdef CONFIG_NF_CONNTRACK_IRC_MODULE
{ALG_TYPE_IRC,MIB_IP_ALG_IRC,"IRC"},
#endif
#ifdef CONFIG_NF_CONNTRACK_RTSP_MODULE
{ALG_TYPE_RTSP,MIB_IP_ALG_RTSP,"RTSP"},
#endif
#ifdef CONFIG_NF_CONNTRACK_QUAKE3_MODULE
{ALG_TYPE_QUAKE3,MIB_IP_ALG_QUAKE3,"quake3"},
#endif
#ifdef CONFIG_NF_CONNTRACK_CUSEEME_MODULE
{ALG_TYPE_CUSEEME,MIB_IP_ALG_CUSEEME,"cuseeme"},
#endif
#ifdef CONFIG_NF_CONNTRACK_L2TP_MODULE
{ALG_TYPE_L2TP,MIB_IP_ALG_L2TP,"L2TP"},
#endif
#ifdef CONFIG_NF_CONNTRACK_IPSEC_MODULE
{ALG_TYPE_IPSEC,MIB_IP_ALG_IPSEC,"IPSec"},
#endif
#if defined(CONFIG_NF_CONNTRACK_SIP_MODULE)
{ALG_TYPE_SIP,MIB_IP_ALG_SIP,"SIP"},
 #endif
 #ifdef CONFIG_NF_CONNTRACK_PPTP_MODULE
{ALG_TYPE_PPTP, MIB_IP_ALG_PPTP,"PPTP"},
 #endif
{ALG_TYPE_MAX,0,NULL}
};
void formALGOnOff(request * wp, char *path, char *query)
{
	char	*str, *submitUrl;
	char tmpBuf[100];
	char cmdstr[128]={0};
	//char cmdbuf[8]={0};
	unsigned char  algenable=0;
	char message[1024];
	char syslog_msg[128] = {0, };
	char alglist[128] = {0, };

	memset(message,0,sizeof(message));
#ifdef USE_LOGINWEB_OF_SERVER
	sprintf(message, "NAT: %s change ALG setting: ", g_login_username);
#else
	sprintf(message, "NAT: change ALG setting: ");
#endif
	
	str = boaGetVar(wp, "apply", "");
	if (str[0]) {
		int i=0;
		for(i=0;i<ALG_TYPE_MAX;i++)
		{

			char algTypeStr[32]={0};
			snprintf(algTypeStr,sizeof(algTypeStr),"%s_algonoff",algTypeName[i].name);
			str=boaGetVar(wp, algTypeStr, "");
			if(str[0])
			{
				algenable=str[0]-'0';
				snprintf(cmdstr,sizeof(cmdstr),"/proc/%s_algonoff",algTypeName[i].name);
				FILE *fp=fopen(cmdstr,"w");
				if(fp)
				{
					fwrite(str,sizeof(char),1,fp);
					fclose(fp);
				}
				if(!mib_set(algTypeName[i].mibalgid,(void*)&algenable))
					printf("%s mib set %d error!\n",__FUNCTION__,algTypeName[i].mibalgid);
			}
			sprintf(tmpBuf, "%s:%s ", algTypeName[i].name, algenable==1?"Enable":"Disable");
			strcat(message, tmpBuf);

			if (algenable == 1)
			{
				if (alglist[0] == 0)	
					strcat(alglist, algTypeName[i].name);
				else
					sprintf(alglist, "%s, %s", alglist, algTypeName[i].name);
			}
		}
		syslog(LOG_INFO, "%s\n", message);
 	}

	// Mason Yu. alg_onoff_20101023
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
	setupAlgOnOff();

	if (alglist[0] != 0)
	{
		sprintf(syslog_msg, "Set ALG(%s) Enable", alglist);
		syslog2(LOG_DM_HTTP, LOG_DM_HTTP_ALG_SET, syslog_msg);
	}

	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;
}
void GetAlgTypes(request * wp)
{
int i = 0;
for(i=0;i<ALG_TYPE_MAX;i++)
{
#if defined(CONFIG_00R0) && defined(CONFIG_NF_CONNTRACK_RTSP_MODULE)
#ifndef CONFIG_GENERAL_WEB
	if(i == ALG_TYPE_RTSP)
	{
		boaWrite(wp,"<tr>\n <td ><font size=2>%s</td>\n"
		"  <td ><font size=2><input type=\"radio\" name=%s_algonoff value=1 >%s</font></td>\n"
		"     <td ><font size=2><input type=\"radio\" name=%s_algonoff value=0 >%s</font> </td>\n "
		"</tr>\n","rtsp/rtcp",algTypeName[i].name, multilang(LANG_ENABLE),algTypeName[i].name,multilang(LANG_DISABLE));
	}
	else
	{
		boaWrite(wp,"<tr>\n <td ><font size=2>%s</td>\n"
		"  <td ><font size=2><input type=\"radio\" name=%s_algonoff value=1 >%s</font></td>\n"
		"     <td ><font size=2><input type=\"radio\" name=%s_algonoff value=0 >%s</font> </td>\n "
		"</tr>\n",algTypeName[i].name,algTypeName[i].name,multilang(LANG_ENABLE), algTypeName[i].name,multilang(LANG_DISABLE));
	}
#else
	if(i == ALG_TYPE_RTSP)
	{
		boaWrite(wp,"<tr>\n <th>%s</th>\n"
		"  <td ><input type=\"radio\" name=%s_algonoff value=1 >%s &nbsp;&nbsp;\n"
		"     <input type=\"radio\" name=%s_algonoff value=0 >%s </td>\n "
		"</tr>\n","rtsp/rtcp",algTypeName[i].name, multilang(LANG_ENABLE),algTypeName[i].name,multilang(LANG_DISABLE));
	}
	else
	{
		boaWrite(wp,"<tr>\n <th>%s</th>\n"
		"  <td><input type=\"radio\" name=%s_algonoff value=1 >%s &nbsp;&nbsp;\n"
		"     <input type=\"radio\" name=%s_algonoff value=0 >%s </td>\n "
		"</tr>\n",algTypeName[i].name,algTypeName[i].name,multilang(LANG_ENABLE), algTypeName[i].name,multilang(LANG_DISABLE));
	}
#endif
#else
#ifndef CONFIG_GENERAL_WEB
	boaWrite(wp,"<tr>\n <td ><font size=2>%s</td>\n"
	"  <td ><font size=2><input type=\"radio\" name=%s_algonoff value=1 >Enable</font></td>\n"
	"     <td ><font size=2><input type=\"radio\" name=%s_algonoff value=0 >Disable</font> </td>\n "
	"</tr>\n",algTypeName[i].name,algTypeName[i].name,algTypeName[i].name);
#else
	boaWrite(wp,"<tr>\n <td >%s</td>\n"
	"  <td ><input type=\"radio\" name=%s_algonoff value=1 >Enable &nbsp;&nbsp;\n"
	"     <input type=\"radio\" name=%s_algonoff value=0 >Disable </td>\n "
	"</tr>\n",algTypeName[i].name,algTypeName[i].name,algTypeName[i].name);
#endif
#endif
}
return;
}
void CreatejsAlgTypeStatus(request * wp)
{
unsigned char  value=0;
int i =0;
for(i=0;i<ALG_TYPE_MAX;i++)
{

	mib_get_s(algTypeName[i].mibalgid,&value, sizeof(value));
	boaWrite(wp,"document.algof.%s_algonoff[%d].checked=true;\n",algTypeName[i].name,!value&0x01);
}
return;

}
void initAlgOnOff(request * wp)
{
	return;
}
#endif

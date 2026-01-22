/*
 *      Include file of form handler
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *      Authors: Dick Tam	<dicktam@realtek.com.tw>
 *
 */


#ifndef _INCLUDE_WEBFORM_H
#define _INCLUDE_WEBFORM_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../globals.h"
#include "../../port.h"
#if HAVE_STDBOOL_H
# include <stdbool.h>
#else
typedef enum {false = 0, true = 1} bool;
#endif

#ifdef EMBED
#include <linux/config.h>
#else
#include "../../../include/linux/autoconf.h"
#endif
#include "options.h"
#include "mib.h"
#include "multilang.h"

#include "../defs.h"

#include "boa.h"

#ifdef __i386__
  #define _CONFIG_SCRIPT_PATH "."
  #define _LITTLE_ENDIAN_
#else
  #define _CONFIG_SCRIPT_PATH "/bin"
#endif

//#define _UPMIBFILE_SCRIPT_PROG  "mibfile_run.sh"
#define _CONFIG_SCRIPT_PROG "init.sh"
#define _WLAN_SCRIPT_PROG "wlan.sh"
#define _PPPOE_SCRIPT_PROG "pppoe.sh"
#define _FIREWALL_SCRIPT_PROG	"firewall.sh"
#define _PPPOE_DC_SCRIPT_PROG	"disconnect.sh"

//added by xl_yue
#ifdef USE_LOGINWEB_OF_SERVER
#define ERR_MSG2(msg) { \
	boaWrite(wp,"<!DOCTYPE html>\n"); \
	boaHeader(wp); \
	boaWrite(wp, "<head><meta http-equiv=\"Content-Type\" content=\"text/html\" charset=\"utf-8\">");\
	boaWrite(wp, "<script>function go_back_referrer(){if(document.referrer&&document.referrer!=\"\") window.location=document.referrer; else history.go(-1); return 0;}</script></head>\n");\
	boaWrite(wp, "<body><blockquote>\n"); \
	boaWrite(wp, "<TABLE width=\"100%%\">\n"); \
	boaWrite(wp, "<TR><TD align = middle><h4>%s</h4></TD></TR>\n",msg); \
	boaWrite(wp, "<TR><TD align = middle><input type=\"button\" onclick=\"go_back_referrer()\" value=\"&nbsp;&nbsp;OK&nbsp;&nbsp\" name=\"OK\"></TD></TR>"); \
	boaWrite(wp, "</TABLE></blockquote></body>\n"); \
	boaFooter(wp); \
	boaDone(wp, 200); \
}
#endif

#define PASSWORD_MODIFY_SUCCESS_MSG_REDIRCET_LOGIN(msg) { \
	boaHeader(wp); \
	boaWrite(wp, "<head><META http-equiv=content-type content=\"text/html; charset=utf-8\">");\
	boaWrite(wp, "<script> \n"); \
	boaWrite(wp, "alert(\"%s\"); \n", msg); \
	boaWrite(wp, "top.location.reload(); \n"); \
	boaWrite(wp, "</script> \n"); \
	boaWrite(wp, "</head>\n"); \
	boaWrite(wp, "<body><blockquote>\n"); \
	boaWrite(wp, "</blockquote></body>\n"); \
	boaFooter(wp); \
	boaDone(wp, 200); \
}

#define ERR_MSG(msg) { \
	boaWrite(wp,"<!DOCTYPE html>\n"); \
	boaHeader(wp); \
	boaWrite(wp, "<head><meta http-equiv=\"Content-Type\" content=\"text/html\" charset=\"utf-8\">");\
	boaWrite(wp, "<script>function go_back_referrer(){if(document.referrer&&document.referrer!=\"\") window.location=document.referrer; else history.go(-1); return 0;}</script></head>\n");\
	boaWrite(wp, "<body><blockquote><h4>%s</h4>\n", msg); \
	boaWrite(wp, "<input type=\"button\" onclick=\"go_back_referrer()\" value=\"&nbsp;&nbsp;OK&nbsp;&nbsp\" name=\"OK\"></blockquote></body>"); \
	boaFooter(wp); \
	boaDone(wp, 200); \
}

#define OK_MSG(url) { \
	boaWrite(wp,"<!DOCTYPE html>\n"); \
	boaHeader(wp); \
	boaWrite(wp, "<head><meta http-equiv=Content-Type content=text/html charset=utf-8></head>\n");\
	boaWrite(wp, "<body><blockquote><h4>%s</h4>\n", multilang(LANG_CHANGE_SETTING_SUCCESSFULLY)); \
	if (url[0]) {\
		boaWrite(wp, "<input type=button value=\"  OK  \" OnClick=window.location.replace(\"%s\")></blockquote></body>", escape_url(url));\
	} else {\
		boaWrite(wp, "<input type=button value=\"  OK  \" OnClick=window.close()></blockquote></body>");\
	}\
	boaFooter(wp); \
	boaDone(wp, 200); \
}

#define OK_MSG1(msg, url) { \
	boaWrite(wp,"<!DOCTYPE html>\n"); \
	boaHeader(wp); \
	boaWrite(wp, "<head><meta http-equiv=Content-Type content=text/html charset=utf-8></head>\n"); \
	boaWrite(wp, "<body><blockquote><h4>%s</h4>\n", msg); \
	if (url) {\
		boaWrite(wp, "<input type=button value=\"  OK  \" OnClick=window.location.replace(\"%s\")></blockquote></body>", escape_url(url));\
	} else {\
		boaWrite(wp, "<input type=button value=\"  OK  \" OnClick=window.close()></blockquote></body>");\
	}\
	boaFooter(wp); \
	boaDone(wp, 200); \
}

#define OK_MSG2() { \
	boaWrite(wp,"<!DOCTYPE html>\n"); \
	boaHeader(wp); \
	boaWrite(wp, "<head><meta http-equiv=Content-Type content=text/html charset=utf-8></head>\n");\
	boaWrite(wp, "<body><blockquote><h4>%s</h4>\n", multilang(LANG_CHANGE_SETTING_SUCCESSFULLY)); \
	boaWrite(wp, "<input type=button value=\"  OK  \" OnClick=\"parent.document.getElementById('시스템 로그').click();\">");\
	boaFooter(wp); \
	boaDone(wp, 200); \
}

#define APPLY_COUNTDOWN_TIME 5
#define OK_MSG_FW(msg, url, c, ip) { \
	boaWrite(wp,"<!DOCTYPE html>\n"); \
	boaHeader(wp); \
	boaWrite(wp, "<head><script language=JavaScript><!--\n");\
	boaWrite(wp, "<meta http-equiv=Content-Type content=text/html charset=utf-8>\n"); \
	boaWrite(wp, "var count = %d;function get_by_id(id){with(document){return getElementById(id);}}\n", c);\
	boaWrite(wp, "function do_count_down(){get_by_id(\"show_sec\").innerHTML = count\n");\
	boaWrite(wp, "if(count == 0) {parent.location.href='http://%s/'; return false;}\n", ip);\
	boaWrite(wp, "if (count > 0) {count--;setTimeout('do_count_down()',1000);}}");\
	boaWrite(wp, "//-->\n");\
	boaWrite(wp,"</script></head>");\
	boaWrite(wp, "<body onload=\"do_count_down();\"><blockquote><h4>%s</h4>\n", msg);\
	boaWrite(wp, "<P align=left><h4>Please wait <B><SPAN id=show_sec></SPAN></B>&nbsp;seconds ...</h4></P>");\
	boaWrite(wp, "</blockquote></body>");\
	boaFooter(wp); \
	boaDone(wp, 200); \
}

#define MSG_COUNTDOWN(msg, sec, url) { \
	boaWrite(wp, "<head><style>\n" \
	"#cntdwn{ border-color: white;border-width: 0px;font-size: 12pt;color: red;text-align:left; font-weight:bold; font-family: Courier;}\n" \
	"</style><script language=javascript>\n" \
	"var h=%d;\n" \
	"function stop() { clearTimeout(id); }\n"\
	"function start() { h--; if (h >= 0) { frm.time.value = h; frm.textname.value='%s'; id=setTimeout(\"start()\",1000); }\n", sec, msg); \
	{ \
		char	*new_page; struct in_addr lanIp; \
		if (url) new_page = url; \
		else if (strstr(wp->host, "8080") != NULL) new_page = wp->host; \
		else { \
			mib_get_s(MIB_ADSL_LAN_IP, &lanIp, sizeof(lanIp)); \
			new_page = inet_ntoa(lanIp); \
		} \
		boaWrite(wp, "if (h == 0) { window.open(\"/\", \"_top\"); top.window.location.assign(\"http://%s/admin/login.asp\"); }}\n", new_page); \
	} \
	boaWrite(wp, "</script></head>"); \
	boaWrite(wp, \
	"<body bgcolor=white  onLoad=\"start();\" onUnload=\"stop();\"><blockquote>" \
	"<form name=frm><B><font color=red><INPUT TYPE=text NAME=textname size=40 id=\"cntdwn\">\n" \
	"<INPUT TYPE=text NAME=time size=5 id=\"cntdwn\"></font></form></blockquote></body>" ); \
}
// Added by davian kuo
extern void langSel(request * wp, char *path, char *query);

/* Routines exported in fmget.c */
extern int check_display(int eid, request * wp, int argc, char **argv);
// Kaohj
extern int checkWrite(int eid, request * wp, int argc, char **argv);
extern int initPage(int eid, request * wp, int argc, char **argv);

extern int getReplacePwPage(int eid, request * wp, int argc, char **argv);

#ifdef CONFIG_GENERAL_WEB
extern int SendWebHeadStr(int eid, request * wp, int argc, char **argv);
extern int CheckMenuDisplay(int eid, request * wp, int argc, char **argv);
#endif

#ifdef WLAN_QoS
extern int ShowWmm(int eid, request * wp, int argc, char **argv);
#endif
#ifdef WLAN_11K
extern int ShowDot11k_v(int eid, request * wp, int argc, char **argv);
#endif
extern int write_wladvanced(int eid, request * wp, int argc, char **argv);
extern int getInfo(int eid, request * wp, int argc, char **argv);
extern int getNameServer(int eid, request * wp, int argc, char **argv);
extern int getDefaultGW(int eid, request * wp, int argc, char **argv);

#if 1//defined(WLAN_SUPPORT) && defined(CONFIG_ARCH_RTL8198F)
extern int extra_wlbasic(int eid, request * wp, int argc, char **argv);
extern int extra_wladvanced(int eid, request * wp, int argc, char **argv);
#endif //defined(WLAN_SUPPORT) && defined(CONFIG_ARCH_RTL8198F)

#ifdef CONFIG_IPV6
extern int getDefaultGW_ipv6(int eid, request * wp, int argc, char **argv);
#endif

extern int srvMenu(int eid, request * wp, int argc, char **argv);

extern void formUploadWapiCert1(request * wp, char * path, char * query);
extern void formUploadWapiCert2(request * wp, char * path, char * query);
extern void formWapiReKey(request * wp, char * path, char * query);


/* Routines exported in fmtcpip.c */
extern void formTcpipLanSetup(request * wp, char *path, char *query);
#ifdef CONFIG_USER_WIRED_8021X
extern void formConfig_802_1x(request * wp, char *path, char *query);
extern int getPortStatus(int eid, request * wp, int argc, char **argv);
#endif
#ifdef CONFIG_USER_VLAN_ON_LAN
extern void formVLANonLAN(request * wp, char *path, char *query);
#endif
extern int lan_setting(int eid, request * wp, int argc, char **argv);
extern int checkIP2(int eid, request * wp, int argc, char **argv);
extern int lan_script(int eid, request * wp, int argc, char **argv);
extern int lan_port_mask(int eid, request * wp, int argc, char **argv);
#ifdef WEB_REDIRECT_BY_MAC
void formLanding(request * wp, char *path, char *query);
#endif

extern int cpuUtility(int eid, request * wp, int argc, char **argv);
extern int memUtility(int eid, request * wp, int argc, char **argv);

//#ifdef CONFIG_USER_PPPOMODEM
extern int wan3GTable(int eid, request * wp, int argc, char **argv);
extern int fm3g_checkWrite(int eid, request * wp, int argc, char **argv);
extern void form3GConf(request * wp, char *path, char *query);

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
extern void formStatus_pon(request * wp, char *path, char *query);
extern int ponGetStatus(int eid, request * wp, int argc, char **argv);
extern int ponflowGetStatus(int eid, request * wp, int argc, char **argv);
extern void formPonStats(request * wp, char *path, char *query);
#ifdef CONFIG_GPON_FEATURE
extern int showgpon_status(int eid, request * wp, int argc, char **argv);
extern int fmgpon_checkWrite(int eid, request * wp, int argc, char **argv);
extern void formgponConf(request * wp, char *path, char *query);
#ifdef CONFIG_00R0
extern void formOmciMcMode(request * wp, char *path, char *query);
extern int showPonLanPorts(int eid, request * wp, int argc, char **argv);
extern void fmgponType(request * wp, char *path, char *query);
#endif
#endif

#ifdef CONFIG_EPON_FEATURE
extern int fmepon_checkWrite(int eid, request * wp, int argc, char **argv);
extern void formeponConf(request * wp, char *path, char *query);
extern int showepon_LLID_status(int eid, request * wp, int argc, char **argv);
extern int showepon_LLID_MAC(int eid, request * wp, int argc, char **argv);
extern void formepon_llid_mac_mapping(request * wp, char *path, char *query);
#endif
#endif
#ifdef CONFIG_GPON_FEATURE
extern int fmvlan_checkWrite(int eid, request * wp, int argc, char **argv);
extern void formVlan(request * wp, char *path, char *query);
extern int omciVlanInfo(int eid, request * wp, int argc, char **argv);
extern int showOMCI_OLT_mode(int eid, request * wp, int argc, char **argv);
extern int fmOmciInfo_checkWrite(int eid, request * wp, int argc, char **argv);
extern void formOmciInfo(request * wp, char *path, char *query);
#endif

//#endif //CONFIG_USER_PPPOMODEM
extern int wanPPTPTable(int eid, request * wp, int argc, char **argv);
extern int wanL2TPTable(int eid, request * wp, int argc, char **argv);
extern int wanIPIPTable(int eid, request * wp, int argc, char **argv);
#ifdef CONFIG_USER_CUPS
int printerList(int eid, request * wp, int argc, char **argv);
#endif
//#ifdef _CWMP_MIB_
extern void formTR069Config(request * wp, char *path, char *query);
extern void formTR069CPECert(request * wp, char *path, char *query);
extern void formTR069CACert(request * wp, char *path, char *query);
extern void formVersionMod(request * wp, char *path, char *query);
extern void formExportOMCIlog(request * wp, char *path, char *query);
extern void formImportOMCIShell(request *wp, char *path, char *query);
extern int TR069ConPageShow(int eid, request * wp, int argc, char **argv);
extern int portFwTR069(int eid, request * wp, int argc, char **argv);
extern int SupportTR111(int eid, request * wp, int argc, char **argv);
//#endif

/* Routines exported in fmfwall.c */
#ifdef PORT_FORWARD_GENERAL
extern void formPortFw(request * wp, char *path, char *query);
#endif
extern void formStaticMapping(request * wp, char *path, char *query);
#ifdef NATIP_FORWARDING
extern void formIPFw(request * wp, char *path, char *query);
#endif
#ifdef PORT_TRIGGERING_STATIC
extern void formGaming(request * wp, char *path, char *query);
#endif
#ifdef PORT_TRIGGERING_DYNAMIC
extern void formPortTrigger(request * wp, char *path, char *query);
extern int portTriggerList(int eid, request * wp, int argc, char **argv);
#endif
#if defined(IP_PORT_FILTER) || defined(MAC_FILTER)
extern void formFilter(request * wp, char *path, char *query);
#endif
#ifdef LAYER7_FILTER_SUPPORT
extern int AppFilterList(int eid, request * wp, int argc, char **argv);
extern void formLayer7(request * wp, char *path, char *query);
#endif
#ifdef PARENTAL_CTRL
extern void formParentCtrl(request * wp, char *path, char *query);
extern int parentalCtrlList(int eid, request * wp, int argc, char **argv);
#endif
#ifdef DMZ
extern void formDMZ(request * wp, char *path, char *query);
#endif
#ifdef PORT_FORWARD_GENERAL
extern int portFwList(int eid, request * wp, int argc, char **argv);
#endif
extern int portStaticMappingList(int eid, request * wp, int argc, char **argv);
#ifdef NATIP_FORWARDING
extern int ipFwList(int eid, request * wp, int argc, char **argv);
#endif
#ifdef IP_PORT_FILTER
extern int ipPortFilterList(int eid, request * wp, int argc, char **argv);
#endif
#ifdef MAC_FILTER
extern int macFilterList(int eid, request * wp, int argc, char **argv);
#endif

/* Routines exported in fmmgmt.c */
extern void formPasswordSetup(request * wp, char *path, char *query);
extern void formUserPasswordSetup(request * wp, char *path, char *query);
#ifdef ACCOUNT_CONFIG
extern void formAccountConfig(request * wp, char *path, char *query);
extern int accountList(int eid, request * wp, int argc, char **argv);
#endif
#ifdef WEB_UPGRADE
extern void formUpload(request * wp, char * path, char * query);
#ifdef CONFIG_DOUBLE_IMAGE
extern void formStopUpload(request * wp, char * path, char * query);
#endif
#endif
extern void formSaveConfig(request * wp, char *path, char *query);
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
extern void formSnmpConfig(request * wp, char *path, char *query);
#endif
#ifdef CONFIG_USER_SNMPD_SNMPD_V3
extern int initSnmpConfig(int eid, request * wp, int argc, char **argv);
#endif
#ifdef CONFIG_USER_RADVD
extern void formRadvdSetup(request * wp, char *path, char *query);
#endif
extern void formAdslDrv(request * wp, char *path, char *query);
//#ifdef CONFIG_DSL_VTUO
extern void formSetVTUO(request * wp, char *path, char *query);
extern int vtuo_checkWrite(int eid, request * wp, int argc, char **argv);
//#endif /*CONFIG_DSL_VTUO*/
extern void formSetAdsl(request * wp, char *path, char *query);
#ifdef FIELD_TRY_SAFE_MODE
extern void formAdslSafeMode(request * wp, char *path, char *query);
#endif
extern void formDiagAdsl(request * wp, char *path, char *query);
extern void formSetAdslTone(request * wp, char *path, char *query);
extern void formSetAdslPSD(request * wp, char *path, char *query);
extern void formSetAdslPSD(request * wp, char *path, char *query);
extern void formStatAdsl(request * wp, char *path, char *query);
extern void formStats(request * wp, char *path, char *query);
extern void formRconfig(request * wp, char *path, char *query);
extern void formSysCmd(request * wp, char *path, char *query);
extern int sysCmdLog(int eid, request * wp, int argc, char **argv);
#ifdef CONFIG_USER_RTK_SYSLOG
extern void formSysLog(request * wp, char *path, char *query);
extern int sysLogList(int eid, request * wp, int argc, char **argv);
extern int RemoteSyslog(int eid, request * wp, int argc, char **argv);
extern int FileList(int eid, request * wp, int argc, char **argv);
#endif
extern void formWanApConnection(request * wp, char *path, char *query);
extern int CheckApConnection(int eid, request * wp, int argc, char **argv);
extern int holePunghing(int eid, request * wp, int argc, char **argv);
extern void formHolepunching(request * wp, char *path, char *query);
extern void formLdap(request * wp, char *path, char *query);
extern void formBankStatus(request * wp, char *path, char *query);
extern void formUpload1(request * wp, char *path, char *query);
extern void formUpload2(request * wp, char *path, char *query);
extern void formUpload3(request * wp, char *path, char *query);
extern void formUpload4(request * wp, char *path, char *query);
extern void formUpload5(request * wp, char *path, char *query);
extern void formUpload6(request * wp, char *path, char *query);
#ifdef DOS_SUPPORT
extern void formDosCfg(request * wp, char *path, char *query);
#endif
#if 0
extern int adslDrvSnrTblGraph(int eid, request * wp, int argc, char **argv);
extern int adslDrvSnrTblList(int eid, request * wp, int argc, char **argv);
extern int adslDrvBitloadTblGraph(int eid, request * wp, int argc, char **argv);
extern int adslDrvBitloadTblList(int eid, request * wp, int argc, char **argv);
#endif
//#ifdef CONFIG_VDSL
int vdslBandStatusTbl(int eid, request * wp, int argc, char **argv);
//#endif /*CONFIG_VDSL*/
extern int adslToneDiagTbl(int eid, request * wp, int argc, char **argv);
extern int adslToneDiagList(int eid, request * wp, int argc, char **argv);
extern int adslToneConfDiagList(int eid, request * wp, int argc, char **argv);
extern int adslPSDMaskTbl(int eid, request * wp, int argc, char **argv);
extern int adslPSDMeasure(int eid, request * wp, int argc, char **argv);
extern int pktStatsList(int eid, request * wp, int argc, char **argv);


/* Routines exported in fmatm.c */
extern int atmVcList(int eid, request * wp, int argc, char **argv);
extern int atmVcList2(int eid, request * wp, int argc, char **argv);
extern int wanConfList(int eid, request * wp, int argc, char **argv);
extern int lanConfList(int eid, request * wp, int argc, char **argv);
extern int DHCPSrvStatus(int eid, request *wp, int argc, char **argv);
extern int DSLStatus(int eid, request * wp, int argc, char **argv);
extern int DSLVer(int eid, request * wp, int argc, char **argv);
#ifdef CONFIG_00R0
extern int BootLoaderVersion(int eid, request * wp, int argc, char **argv);
extern int GPONSerialNumber(int eid, request * wp, int argc, char **argv);
#ifdef CONFIG_WLAN
extern int wlanActiveConfList(int eid, request * wp, int argc, char **argv);
#endif
#endif
#ifdef CONFIG_IPV6
extern int wanip6ConfList(int eid, request * wp, int argc, char **argv);
extern int routeip6ConfList(int eid, request * wp, int argc, char **argv);
extern int dsliteConfList(int eid, request * wp, int argc, char **argv);
extern int neighborList(int eid, request * wp, int argc, char **argv);
#endif
extern void formWanMode(request * wp, char *path, char *query);
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN) || defined(WLAN_WISP)
extern void formEth(request * wp, char *path, char *query);
#ifdef CONFIG_USER_WAN_PORT_AUTO_SELECTION
extern void formWanPortSet(request *wp, char *path, char *query);
#endif
#ifdef CONFIG_00R0
extern void formEthAdmin(request * wp, char *path, char *query);
#endif
#endif
#ifdef CONFIG_DEV_xDSL
extern void formAtmVc(request * wp, char *path, char *query);
extern void formAdsl(request * wp, char *path, char *query);
#endif
extern void formPPPEdit(request * wp, char *path, char *query);
extern void formIPEdit(request * wp, char *path, char *query);
extern void formBrEdit(request * wp, char *path, char *query);
extern void formStatus(request * wp, char *path, char *query);
//#if defined(CONFIG_RTL_8676HWNAT)
extern void formLANPortStatus(request * wp, char *path, char *query);
extern int showLANPortStatus(int eid, request * wp, int argc, char **argv);
//#endif
#ifdef CONFIG_IPV6
extern void formStatus_ipv6(request * wp, char *path, char *query);
#endif
extern void formDate(request * wp, char *path, char *query);
/* Routines exported in fmbridge.c */
extern void formBridge(request * wp, char *path, char *query);
extern void formRefleshFdbTbl(request * wp, char *path, char *query);
#ifdef CONFIG_USER_RTK_LAN_USERLIST
extern void formRefleshLanUserTbl(request * wp, char *path, char *query);
extern int LanUserTableList(int eid, request * wp, int argc, char **argv);
#endif
extern int bridgeFdbList(int eid, request * wp, int argc, char **argv);
extern int ARPTableList(int eid, request * wp, int argc, char **argv);

/* Routines exported in fmroute.c */
#ifdef ROUTING
extern void formRoute(request * wp, char *path, char *query);
extern int showStaticRoute(int eid, request * wp, int argc, char **argv);
#endif
#ifdef CONFIG_IPV6
extern void formIPv6EnableDisable(request * wp, char *path, char *query);
extern void formIPv6Route(request * wp, char *path, char *query);
extern void formFilterV6(request * wp, char *path, char *query);
extern int ipPortFilterListV6(int eid, request * wp, int argc, char **argv);
#endif
#if defined(CONFIG_USER_ROUTED_ROUTED) || defined(CONFIG_USER_ZEBRA_OSPFD_OSPFD)
extern void formRip(request * wp, char *path, char *query);
#endif
#ifdef CONFIG_USER_ROUTED_ROUTED
extern int showRipIf(int eid, request * wp, int argc, char **argv);
#endif
#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
extern int showOspfIf(int eid, request * wp, int argc, char **argv);
#endif
extern int ShowDefaultGateway(int eid, request * wp, int argc, char **argv);
extern int GetDefaultGateway(int eid, request * wp, int argc, char **argv);
extern int DisplayDGW(int eid, request * wp, int argc, char **argv);
extern int DisplayTR069WAN(int eid, request * wp, int argc, char **argv);
#ifdef CONFIG_IPV6
extern int showIPv6StaticRoute(int eid, request * wp, int argc, char **argv);
extern void formIPv6RefleshRouteTbl(request * wp, char *path, char *query);
extern int routeIPv6List(int eid, request * wp, int argc, char **argv);
#endif
extern void formRefleshRouteTbl(request * wp, char *path, char *query);
extern int routeList(int eid, request * wp, int argc, char **argv);
#ifdef CONFIG_USER_RTK_WAN_CTYPE
extern int ShowConnectionType(int eid, request * wp, int argc, char **argv);
extern int ShowConnectionTypeForBridge(int eid, request * wp, int argc, char **argv);
extern int ShowConnectionTypeForUser(int eid, request * wp, int argc, char **argv);
#endif
extern int ShowMacClone(int eid, request * wp, int argc, char **argv);
extern int ShowIpProtocolType(int eid, request * wp, int argc, char **argv);
extern int ShowIPV6Settings(int eid, request * wp, int argc, char **argv);
extern int ShowPortMapping(int eid, request * wp, int argc, char **argv);
extern int ShowPortBaseFiltering(int eid, request * wp, int argc, char **argv);
extern int Show6rdSetting(int eid, request * wp, int argc, char **argv);
extern int Show6in4Setting(int eid, request * wp, int argc, char **argv);
extern int Show6to4Setting(int eid, request * wp, int argc, char **argv);
extern int Showv6inv4TunnelSetting(int eid, request * wp, int argc, char **argv);
#ifdef WLAN_WISP
extern void ShowWispWanItf(int eid, request * wp, int argc, char **argv);
extern void initWispWanItfStatus(int eid, request * wp, int argc, char **argv);
#endif

/* Routines exported in fmdhcpd.c */
extern void formDhcpd(request * wp, char *path, char *query);
extern void formReflashClientTbl(request * wp, char *path, char *query);
extern int dhcpClientList(int eid, request * wp, int argc, char **argv);
extern int dhcpClientListWithNickname(int eid, request * wp, int argc, char **argv);

#ifdef CONFIG_USER_DHCPCLIENT_MODE
extern int dhcpc_script(int eid, request * wp, int argc, char **argv);
extern int dhcpc_clicksetup(int eid, request * wp, int argc, char **argv);
#endif
extern int tcpip_lan_oninit(int eid, request * wp, int argc, char **argv);

#ifdef DHCPV6_ISC_DHCP_4XX
extern void formDhcpv6(request * wp, char *path, char *query);
extern int showDhcpv6SNameServerTable(int eid, request * wp, int argc, char **argv);
extern int showDhcpv6SDOMAINTable(int eid, request * wp, int argc, char **argv);
extern int dhcpClientListv6(int eid, request * wp, int argc, char **argv);
extern int showDhcpv6SNTPServerTable(int eid, request * wp, int argc, char **argv);
extern int showDhcpv6SMacBindingTable(int eid, request * wp, int argc, char **argv);
#endif

/* Routines exported in fmDNS.c */
extern void formDns(request * wp, char *path, char *query);

/* Routines exported in fmDDNS.c */
#ifdef CONFIG_USER_DDNS
extern void formDDNS(request * wp, char *path, char *query);
extern int showDNSTable(int eid, request * wp, int argc, char **argv);
extern int ddnsServicePort(int eid, request * wp, int argc, char **argv);
#endif

/* Routines exported in fmDNS.c */
extern void formDhcrelay(request * wp, char *path, char *query);

#ifdef ADDRESS_MAPPING
extern void formAddressMap(request * wp, char *path, char *query);
#ifdef MULTI_ADDRESS_MAPPING
extern int showMultAddrMappingTable(int eid, request * wp, int argc, char **argv);
#endif // MULTI_ADDRESS_MAPPING
#endif

/* Routines exported in fmping.c */
extern void formPing(request * wp, char *path, char *query);
extern void formTracert(request * wp, char *path, char *query);
extern void formTracertResult(request * wp, char *path, char *query);
extern void formPingResult(request * wp, char *path, char *query);
extern int clearPingResult(int eid, request * wp, int argc, char **argv);
extern int clearTracertResult(int eid, request * wp, int argc, char **argv);
#ifdef CONFIG_IPV6
extern void formPing6(request * wp, char *path, char *query);
extern void formTracert6(request * wp, char *path, char *query);
#endif
extern void formGroupTable(request * wp, char *path, char *query);
extern void formGroupTableResult(request * wp, char *path, char *query);

extern void formSsh(request * wp, char *path, char *query);
extern void formSshResult(request * wp, char *path, char *query);

extern void formNetstat(request * wp, char *path, char *query);
extern void formNetstatResult(request * wp, char *path, char *query);
extern void formPortMirror(request * wp, char *path, char *query);
extern void formPortMirrorResult(request * wp, char *path, char *query);
extern int clearPortMirrorResult(int eid, request * wp, int argc, char **argv);

/* Routines exported in fmcapture.c */
extern void formCapture(request * wp, char *path, char *query);

/* Routines exported in fmoamlb.c */
extern void formOamLb(request * wp, char *path, char *query);

/* Routines exported in fmreboot.c */
extern void formReboot(request * wp, char *path, char *query);
#ifdef FINISH_MAINTENANCE_SUPPORT
//xl_yue added,inform itms that maintenance finished
extern void formFinishMaintenance(request * wp, char *path, char *query);
#endif

#ifdef USE_LOGINWEB_OF_SERVER
//xl_yue added
extern void userLogin(request * wp, char *path, char *query);
extern void formLogout(request * wp, char *path, char *query);
// Kaohj
extern int passwd2xmit(int eid, request * wp, int argc, char **argv);
// davian_kuo
extern void userLoginMultilang(request * wp, char *path, char *query);
#ifdef USE_CAPCTHA_OF_LOGINWEB
extern int getCaptchastring(int eid, request * wp, int argc, char **argv);
#endif
#endif


#ifdef ACCOUNT_LOGIN_CONTROL
//xl_yue add
extern void formAdminLogout(request * wp, char *path, char *query);
extern void formUserLogout(request * wp, char *path, char *query);
#endif

/* Routines exported in fmdhcpmode.c */
extern void formDhcpMode(request * wp, char *path, char *query);

/* Routines exported in fmupnp.c */
extern void formUpnp(request * wp, char *path, char *query);
extern int upnpPortFwList(int eid, request * wp, int argc, char **argv);
extern int ifwanList_upnp(int eid, request * wp, int argc, char **argv);
extern void formMLDProxy(request * wp, char *path, char *query);				// Mason Yu. MLD Proxy
extern int mldproxyinit(int eid, request * wp, int argc, char **argv);
extern void formMLDSnooping(request * wp, char *path, char *query);    		// Mason Yu. MLD snooping

/* Routines exported in fmdms.c */
#ifdef CONFIG_USER_MINIDLNA
extern int fmDMS_checkWrite(int eid, request * wp, int argc, char **argv);
extern void formDMSConf(request * wp, char *path, char *query);
#endif

extern int portmirrorinit(int eid, request * wp, int argc, char **argv);
/* Routines exported in fmigmproxy.c */
#ifdef CONFIG_USER_IGMPPROXY
extern void formIgmproxy(request * wp, char *path, char *query);
extern int igmpproxyinit(int eid, request * wp, int argc, char **argv);
#endif
extern int user_confirm(int eid, request * wp, int argc, char **argv);
extern int ifwanList(int eid, request * wp, int argc, char **argv);
#ifdef QOS_TRAFFIC_SHAPING_BY_SSID
extern int ssid_list(int eid, request * wp, int argc, char **argv);
#endif
/* Routines exported in fmothers.c */
#ifdef IP_PASSTHROUGH
extern void formOthers(request * wp, char *path, char *query);
#endif
extern int diagMenu(int eid, request * wp, int argc, char **argv);
extern int adminMenu(int eid, request * wp, int argc, char **argv);

//xl_yue
extern int userAddAdminMenu(int eid, request * wp, int argc, char **argv);

/* Routines exported in fmsyslog.c*/
//extern void formSysLog(request * wp, char *path, char *query);

#ifdef IP_ACL
/* Routines exported in fmacl.c */
extern void formACL(request * wp, char *path, char *query);
extern int showACLTable(int eid, request * wp, int argc, char **argv);
extern int LANACLItem(int eid, request * wp, int argc, char **argv);
extern int WANACLItem(int eid, request * wp, int argc, char **argv);
#ifdef CONFIG_GENERAL_WEB
extern int checkAclItems(int eid, request * wp, int argc, char **argv);
#endif
#ifdef CONFIG_IPV6
extern void formV6ACL(request * wp, char *path, char *query);
extern int showV6ACLTable(int eid, request * wp, int argc, char **argv);
extern int LANV6ACLItem(int eid, request * wp, int argc, char **argv);
extern int WANV6ACLItem(int eid, request * wp, int argc, char **argv);
#endif
#endif
#ifdef _CWMP_MIB_
extern int showCWMPACLTable(int eid, request * wp, int argc, char **argv);
#endif
#ifdef NAT_CONN_LIMIT
extern int showConnLimitTable(int eid, request * wp, int argc, char **argv);
extern void formConnlimit(request * wp, char *path, char *query);
#endif
#ifdef TCP_UDP_CONN_LIMIT
extern int showConnLimitTable(int eid, request * wp, int argc, char **argv);
extern void formConnlimit(request * wp, char *path, char *query);
#endif

extern void formmacBase(request * wp, char *path, char *query);
#ifdef IMAGENIO_IPTV_SUPPORT
extern void formIpRange(request * wp, char *path, char *query);
#endif
extern int showMACBaseTable(int eid, request * wp, int argc, char **argv);
#ifdef IMAGENIO_IPTV_SUPPORT
extern int showDeviceIpTable(int eid, request * wp, int argc, char **argv);
#endif //#ifdef IMAGENIO_IPTV_SUPPORT

#ifdef URL_BLOCKING_SUPPORT
/* Routines exported in fmurl.c */
extern void formURL(request * wp, char *path, char *query);
extern int showURLTable(int eid, request * wp, int argc, char **argv);
extern int showKeywdTable(int eid, request * wp, int argc, char **argv);
#endif
#ifdef URL_ALLOWING_SUPPORT
extern int showURLALLOWTable(int eid, request * wp, int argc, char **argv);
#endif



#ifdef DOMAIN_BLOCKING_SUPPORT
/* Routines exported in fmdomainblk.c */
extern void formDOMAINBLK(request * wp, char *path, char *query);
extern int showDOMAINBLKTable(int eid, request * wp, int argc, char **argv);
#endif

#ifdef TIME_ZONE
extern void formNtp(request * wp, char *path, char *query);
extern int timeZoneList(int eid, request * wp, int argc, char **argv);
#endif
#ifdef CONFIG_USER_BRIDGE_GROUPING
extern void formBridgeGrouping(request * wp, char *path, char *query);
extern int ifGroupList(int eid, request * wp, int argc, char **argv);
#endif

#if defined(CONFIG_USER_INTERFACE_GROUPING)
extern void formInterfaceGrouping(request * wp, char *path, char *query);
#endif

#if defined(CONFIG_USER_USP_TR369)
extern void formTR369(request * wp, char *path, char *query);
extern int showTR369List(int eid, request * wp, int argc, char **argv);
#endif

#ifdef CONFIG_USER_LAN_VLAN_TRANSLATE
extern int initPageLanVlan(int eid, request * wp, int argc, char ** argv);
extern void formLanVlanSetup(request * wp, char *path, char *query);
#endif

#if defined(CONFIG_RTL_MULTI_LAN_DEV)
#ifdef ELAN_LINK_MODE
extern void formLink(request * wp, char *path, char *query);
extern int show_lanport(int eid, request * wp, int argc, char **argv);
#endif
#else // of CONFIG_RTL_MULTI_LAN_DEV
#ifdef ELAN_LINK_MODE_INTRENAL_PHY
extern void formLink(request * wp, char *path, char *query);
#endif
#endif	// of CONFIG_RTL_MULTI_LAN_DEV
extern int iflanList(int eid, request * wp, int argc, char **argv);
extern int policy_route_outif(int eid, request * wp, int argc, char **argv);

#ifdef CONFIG_8021P_PRIO
extern int setting_1ppriority(int eid, request * wp, int argc, char **argv);
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
extern int pptpList(int eid, request * wp, int argc, char **argv);
extern void formPPtP(request * wp, char *path, char *query);
#endif //end of CONFIG_USER_PPTP_CLIENT_PPTP
#if defined(CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_PPTP_SERVER)
extern int pptpServerList(int eid, request * wp, int argc, char **argv);
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
extern void formL2TP(request * wp, char *path, char *query);
extern int lns_name_list(int eid, request * wp, int argc, char **argv);
extern int l2tpList(int eid, request * wp, int argc, char **argv);
extern int l2tpWithIPSecLAC(int eid, request * wp, int argc, char **argv);
extern int l2tpWithIPSecLNS(int eid, request * wp, int argc, char **argv);
extern int l2tpWithIPSecLACTable(int eid, request * wp, int argc, char **argv);
#endif //end of CONFIG_USER_L2TPD_L2TPD
#ifdef CONFIG_USER_L2TPD_LNS
extern int l2tpServerList(int eid, request * wp, int argc, char **argv);
#endif

#ifdef CONFIG_XFRM
#ifdef CONFIG_USER_STRONGSWAN
extern void formIPsecStrongswan(request * wp, char *path, char *query);
extern void formIPSecSwanCert(request * wp, char * path, char * query);
extern void formIPSecSwanKey(request * wp, char * path, char * query);
extern void formIPSecSwanGenCert(request * wp, char * path, char * query);
extern int ipsecSwanConfList(int eid, request * wp, int argc, char **argv);
extern int ipsecSwanWanList(int eid, request * wp, int argc, char **argv);
extern int ipsecSwanConfDetail(int eid, request * wp, int argc, char **argv);

#else
extern void formIPsec(request * wp, char *path, char *query);
extern void formIPSecCert(request * wp, char *path, char *query);
extern void formIPSecKey(request * wp, char *path, char *query);
extern int ipsec_wanList(int eid, request * wp, int argc, char **argv);
extern int ipsec_ikePropList(int eid, request * wp, int argc, char **argv);
extern int ipsec_encrypt_p2List(int eid, request * wp, int argc, char **argv);
extern int ipsec_auth_p2List(int eid, request * wp, int argc, char **argv);
extern int ipsecAlgoScripts(int eid, request * wp, int argc, char **argv);
extern int ipsec_infoList(int eid, request * wp, int argc, char **argv);
#endif
#endif

#ifdef CONFIG_NET_IPIP
extern void formIPIP(request * wp, char *path, char *query);
extern int ipipList(int eid, request * wp, int argc, char **argv);
#endif //end of CONFIG_NET_IPIP

#ifdef CONFIG_NET_IPGRE
extern void formGRE(request * wp, char *path, char *query);
extern int showGRETable(int eid, request * wp, int argc, char **argv);
#endif

#ifdef CONFIG_USER_IP_QOS
extern void formQosPolicy(request * wp, char *path, char *query);
extern int  initQueuePolicy(int eid, request * wp, int argc, char **argv);
extern void formQosRule(request * wp, char* path, char* query);
extern void formQosRuleEdit(request * wp, char* path, char* query);
extern int  initQosRulePage(int eid, request * wp, int argc, char **argv);
extern int  initRulePriority(int eid, request * wp, int argc, char **argv);
extern int  initQosLanif(int eid, request * wp, int argc, char **argv);
extern int  initOutif(int eid, request * wp, int argc, char **argv);
extern int  initTraffictlPage(int eid, request * wp, int argc, char **argv);
extern void formQosTraffictl(request * wp, char *path, char *query);
extern void formQosTraffictlEdit(request * wp, char *path, char *query);
extern int  ifWanList_tc(int eid, request * wp, int argc, char **argv);
extern int ipqos_dhcpopt(int eid, request * wp, int argc, char **argv);
extern int ipqos_dhcpopt_getoption60(int eid, request * wp, int argc, char **argv);
extern int initEthTypes(int eid, request * wp, int argc, char **argv);
extern int initProtocol(int eid, request * wp, int argc, char **argv);

#ifdef CONFIG_00R0
extern void formTrafficShaping(request * wp, char *path, char *query);
#endif
#endif

#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
extern int initQosSpeedLimitLanif(int eid, request * wp, int argc, char **argv);
extern int initQosSpeedLimitRule(int eid, request * wp, int argc, char **argv);
extern void formQosSpeedLimit(request * wp, char *path, char *query);
#endif

extern void formAcc(request * wp, char *path, char *query);
extern int accPost(int eid, request * wp, int argc, char **argv);
extern int accItem(int eid, request * wp, int argc, char **argv);
extern int ShowAutoPVC(int eid, request * wp, int argc, char **argv);	// auto-pvc-search

extern int ShowChannelMode(int eid, request * wp, int argc, char **argv); // China telecom e8-a
extern int ShowBridgeMode(int eid, request * wp, int argc, char **argv); // For PPPoE pass through
extern int ShowPPPIPSettings(int eid, request * wp, int argc, char **argv); // China telecom e8-a
#ifdef CONFIG_00R0
extern int ShowPPPIPSettings_admin(int eid, request * wp, int argc, char **argv);
#endif
#ifdef CONFIG_USER_BOA_PRO_PASSTHROUGH
extern void formVpnPassThru(request * wp, char *path, char *query);
extern int VpnPassThrulist(int eid, request * wp, int argc, char **argv);
#endif
extern int ShowWanPortSetting(int eid, request * wp, int argc, char **argv);
extern int ShowNAPTSetting(int eid, request * wp, int argc, char **argv); // China telecom e8-a
//#ifdef CONFIG_RTL_MULTI_ETH_WAN
extern int initPageWaneth(int eid, request * wp, int argc, char **argv);
//#endif

//#ifdef CONFIG_MCAST_VLAN
extern int listWanName(int eid, request * wp, int argc, char ** argv);
extern void formMcastVlanMapping(request * wp, char *path, char *query);
//#endif


extern int ShowIGMPSetting(int eid, request * wp, int argc, char **argv); // Telefonica
extern int ShowMLDSetting(int eid, request * wp, int argc, char **argv);

#ifdef CONFIG_00R0
extern int ShowRIPv2Setting(int eid, request * wp, int argc, char **argv);
#endif
extern void formWanRedirect(request * wp, char *path, char *query);
extern int getWanIfDisplay(int eid, request * wp, int argc, char **argv);
#ifdef WLAN_SUPPORT
extern int wlanMenu(int eid, request * wp, int argc, char **argv);
extern int wlanStatus(int eid, request * wp, int argc, char **argv);
extern int wlan_ssid_select(int eid, request * wp, int argc, char **argv);
extern int getwlanencrypt(int eid, request * wp, int argc, char **argv);

#if defined(WLAN_DUALBAND_CONCURRENT)
extern void formWlanRedirect(request * wp, char *path, char *query);
#endif
/* Routines exported in fmwlan.c */
extern void formWlanSetup(request * wp, char *path, char *query);
#ifdef WLAN_ACL
extern int wlAcList(int eid, request * wp, int argc, char **argv);
extern void formWlAc(request * wp, char *path, char *query);
#endif
extern void formAdvanceSetup(request * wp, char *path, char *query);
#ifdef WLAN_RTIC_SUPPORT
extern void formWlRtic(request * wp, char *path, char *query);
#endif
extern int wirelessClientList(int eid, request * wp, int argc, char **argv);
extern void formWirelessTbl(request * wp, char *path, char *query);

#ifdef WLAN_WPA
extern void formWlEncrypt(request * wp, char *path, char *query);
#endif
//WDS
#ifdef WLAN_WDS
extern void formWlWds(request * wp, char *path, char *query);
extern void formWdsEncrypt(request * wp, char *path, char *query);
extern int wlWdsList(int eid, request * wp, int argc, char **argv);
extern int wdsList(int eid, request * wp, int argc, char **argv);
#endif
#if defined(WLAN_CLIENT) || defined(WLAN_SITESURVEY)
extern void formWlSiteSurvey(request * wp, char *path, char *query);
extern int wlSiteSurveyTbl(int eid, request * wp, int argc, char **argv);
#endif

#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS) //WPS
extern void formWsc(request * wp, char *path, char *query);
#endif
#ifdef WLAN_11R
extern void formFt(request * wp, char *path, char *query);
extern int wlFtKhList(int eid, request * wp, int argc, char **argv);
#endif
extern int wlStatus_parm(int eid, request * wp, int argc, char **argv);
#ifdef WLAN_MESH
extern void formMeshSetup(request * wp, char *path, char *query);
#ifdef WLAN_MESH_ACL_ENABLE
extern int wlMeshAcList(int eid, request * wp, int argc, char **argv);
extern void formMeshACLSetup(request * wp, char *path, char *query);
#endif
extern int wlMeshNeighborTable(int eid, request * wp, int argc, char **argv);
extern int wlMeshRoutingTable(int eid, request * wp, int argc, char **argv);
extern int wlMeshPortalTable(int eid, request * wp, int argc, char **argv);
extern int wlMeshProxyTable(int eid, request * wp, int argc, char **argv);
extern int wlMeshRootInfo(int eid, request * wp, int argc, char **argv);
#endif
#ifdef SUPPORT_FON_GRE
extern void formFonGre(request * wp, char *path, char *query);
#endif
#ifdef RTK_MULTI_AP
extern void formMultiAP(request *wp, char *path, char *query);
extern int showBackhaulSelection(int eid, request * wp, int argc, char **argv);
#ifdef RTK_MULTI_AP_R2
extern void formMultiAPVLAN(request *wp, char *path, char *query);
extern void formChannelScan(request *wp, char *path, char *query);
extern int showChannelScanResults(int eid, request *wp, int argc, char **argv);
#endif
#endif
extern void translate_web_code(char *buffer);
#ifdef WLAN_SCHEDULE_SUPPORT
extern void formNewSchedule(request *wp, char *path, char *query);
extern int wlSchList(int eid,request *wp, int argc, char **argv);
#endif
#endif // of WLAN_SUPPORT

extern int oamSelectList(int eid, request * wp, int argc, char **argv);
#ifdef DIAGNOSTIC_TEST
extern void formDiagTest(request * wp, char *path, char *query);	// Diagnostic test
extern int lanTest(int eid, request * wp, int argc, char **argv);	// Ethernet LAN connection test
extern int adslTest(int eid, request * wp, int argc, char **argv);	// ADSL service provider connection test
extern int internetTest(int eid, request * wp, int argc, char **argv);	// Internet service provider connection test
#endif
#ifdef WLAN_MBSSID
extern int getVirtualIndex(int eid, request * wp, int argc, char **argv);
extern void formWlanMultipleAP(request * wp, char *path, char *query);
extern int wirelessVAPClientList(int eid, request * wp, int argc, char **argv);
extern void formWirelessVAPTbl(request * wp, char *path, char *query);
#if 0
extern int postSSID(int eid, request * wp, int argc, char **argv);
extern int postSSIDWEP(int eid, request * wp, int argc, char **argv);
#endif
extern int SSIDStr(int eid, request * wp, int argc, char **argv);
#endif

#ifdef PORT_FORWARD_ADVANCE
extern void formPFWAdvance(request * wp, char *path, char *query);
extern int showPFWAdvTable(int eid, request * wp, int argc, char **argv);
#endif
#ifdef PORT_FORWARD_GENERAL
extern int showPFWAdvForm(int eid, request * wp, int argc, char **argv);
#endif
#ifdef WEB_ENABLE_PPP_DEBUG
extern void ShowPPPSyslog(int eid, request * wp, int argc, char **argv);
#endif
#ifdef USER_WEB_WIZARD
extern void formWebWizardMenu(request * wp, char *path, char *query);
extern void formWebWizard1(request * wp, char *path, char *query);
extern void formWebWizard4(request * wp, char *path, char *query);
extern void formWebWizard5(request * wp, char *path, char *query);
extern void formWebWizard6(request * wp, char *path, char *query);
extern int ShowWebWizardPage(int eid, request * wp, int argc, char **argv);
#endif

#ifdef CONFIG_00R0
/* siyuan 2016-01-25 add wizard screen functions */
extern int getInitUrl(int eid, request * wp, int argc, char **argv);
extern void formWizardScreen0_1(request * wp, char *path, char *query);
extern int initWizardScreen1_1(int eid, request * wp, int argc, char **argv);
extern void formWizardScreen1_1(request * wp, char *path, char *query);
extern void formWizardScreen1_2(request * wp, char *path, char *query);
extern int initWizardScreen4_1(int eid, request * wp, int argc, char **argv);
extern int initWizardScreen4_1_1(int eid, request * wp, int argc, char **argv);
extern void formWizardScreen4_1_1(request * wp, char *path, char *query);
extern int initWizardScreen4_2(int eid, request * wp, int argc, char **argv);
extern int postWizardScreen4_2(int eid, request * wp, int argc, char **argv);
extern void formWizardScreen4_2(request * wp, char *path, char *query);
extern void formWizardScreen5_1(request * wp, char *path, char *query);
extern int initWizardScreen5_2(int eid, request * wp, int argc, char **argv);
extern void formWizardScreen5_2(request * wp, char *path, char *query);
extern int getTroubleInitUrl(int eid, request * wp, int argc, char **argv);
extern int getInitUrl_2019(int eid, request * wp, int argc, char **argv);
extern int initWizardScreen1_2019(int eid, request * wp, int argc, char **argv);
extern int initWizardScreen1_1_2019(int eid, request * wp, int argc, char **argv);
extern int initWizardScreen2_2_2019(int eid, request * wp, int argc, char **argv);
extern void formWizardScreen0_1_2019(request * wp, char *path, char *query);
extern void formWizardScreen1_2019(request * wp, char *path, char *query);
extern void formWizardScreen1_1_2019(request * wp, char *path, char *query);
extern void formWizardScreen2_1_2019(request * wp, char *path, char *query);
extern void formWizardScreen2_2_2019(request * wp, char *path, char *query);
#endif
#ifdef _PRMT_X_CT_COM_MWBAND_
extern int initClientLimit(int eid, request * wp, int argc, char **argv);
extern void formClientLimit(request * wp, char *path, char *query);
#endif
extern int DSLuptime(int eid, request * wp, int argc, char **argv);
//#ifdef CONFIG_USER_XDSL_SLAVE
extern int DSLSlvuptime(int eid, request * wp, int argc, char **argv);
//#endif /*CONFIG_USER_XDSL_SLAVE*/
extern int multilang_asp(int eid, request * wp, int argc, char **argv);
extern int WANConditions(int eid, request * wp, int argc, char **argv);
extern int ShowWanMode(int eid, request * wp, int argc, char **argv);
extern void set_user_profile(void);
// Kaohj -- been move to utility.c from fmigmproxy.c
//extern int ifWanNum(char *type);
#ifdef CONFIG_USER_ROUTED_ROUTED
extern int ifRipNum(); // fmroute.c
#endif

extern const char * const BRIDGE_IF;
extern const char * const ELAN_IF;
//extern const char * const WLAN_IF;

extern int g_remoteConfig;
extern int g_remoteAccessPort;
extern int g_rexpire;
extern short g_rSessionStart;

/* constant string */
static const char DOCUMENT[] = "document";
extern int lanSetting(int eid, request * wp, int argc, char **argv);
extern int getPortMappingInfo(int eid, request * wp, int argc, char **argv);
//cathy, MENU_MEMBER_T should be updated if RootMenu structure is modified
typedef enum {
	MEM_NAME,
	MEM_TYPE,
	MEM_U,
	MEM_TIP,
	MEM_CHILDRENNUMS,
	MEM_EOL,
	MEM_HIDDEN
} MENU_MEMBER_T;

typedef enum {
	MENU_DISPLAY,
	MENU_HIDDEN
} MENU_HIDDEN_T;

typedef enum {
	MENU_FOLDER,
	MENU_URL
} MENU_T;

struct RootMenu{
	char  name[128];
	MENU_T type;
	union {
		void *addr;
		void *url;
	} u;
	char tip[128];
	int childrennums;
	int eol;	// end of layer
	int hidden;
	int lang_tag; // Added by davian_kuo, for multi-lingual.
};

//add by ramen for ALG ON-OFF
#ifdef CONFIG_USER_RTK_NAT_ALG_PASS_THROUGH
void formALGOnOff(request * wp, char *path, char *query);
void GetAlgTypes(request * wp);
void initAlgOnOff(request * wp);
void CreatejsAlgTypeStatus(request * wp);
#endif

#ifdef CONFIG_USER_SAMBA
void formSamba(request * wp, char *path, char *query);
void formSambaAccount(request * wp, char *path, char *query);
int showSambaAccount(int eid, request * wp, int argc, char **argv);
#endif
#ifdef CONFIG_USB_SUPPORT
extern int listUsbDevices(int eid, request * wp, int argc, char ** argv);
#endif
#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_AWIFI_SUPPORT)
extern void formAWiFi(request * wp, char *path, char *query);
extern void formAWiFiMAC(request * wp, char *path, char *query);
extern void formAWiFiURL(request * wp, char *path, char *query);
#ifdef CONFIG_USER_AWIFI_AUDIT_SUPPORT
extern void formAWiFiAudit(request * wp, char *path, char *query);
#endif

extern int showAWiFiMACTable(int eid, request * wp, int argc, char **argv);
extern int showAWiFiURLTable(int eid, request * wp, int argc, char **argv);
#endif

#ifdef CONFIG_USER_RTK_LBD
extern void formLBD(request * wp, char *path, char *query);
extern int initLBDPage(int eid, request * wp, int argc, char **argv);
#endif

#ifdef CONFIG_INIT_SCRIPTS
extern void formInitStartScript(request * wp, char *path, char *query);
extern void formInitStartScriptDel(request * wp, char *path, char *query);
extern void formInitEndScript(request * wp, char *path, char *query);
extern void formInitEndScriptDel(request * wp, char *path, char *query);
#endif

#if defined(CONFIG_USER_Y1731) || defined(CONFIG_USER_8023AH)
extern void formY1731(request * wp, char *path, char *query);
extern int getLinktraceMac(int eid, request * wp, int argc, char **argv);
#endif


#ifdef CONFIG_USER_DOT1AG_UTILS
extern int dot1ag_init(int eid, request * wp, int argc, char **argv);
extern void formDot1agConf(request * wp, char *path, char *query);
extern void formDot1agAction(request * wp, char *path, char *query);
extern void dot1agActionRefresh(request * wp, char *path, char *query);
extern int dot1ag_status_init(int eid, request * wp, int argc, char **argv);
#endif

#ifdef CONFIG_APACHE_FELIX_FRAMEWORK
extern int getOSGIInfo(int eid, request * wp, int argc, char **argv);
extern int getOSGIBundleList(int eid, request * wp, int argc, char **argv);
extern void formOsgiUpload(request * wp, char *path, char *query);
extern void formOsgiMgt(request * wp, char *path, char *query);

#endif

extern void formY1731(request * wp, char *path, char *query);

#ifdef CONFIG_USER_RTK_BRIDGE_MODE
extern void formInternetMode(request * wp, char *path, char *query);
#endif

#ifdef CONFIG_ELINK_SUPPORT
extern void formElinkSetup(request *wp, char *path, char *query);
#ifdef CONFIG_ELINKSDK_SUPPORT
extern void formElinkSDKUpload(request *wp, char *path, char *query);
#endif
#endif

#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
extern void formStaticBalance(request * wp, char *path, char *query);
extern void formDynmaicBalance(request * wp, char *path, char *query);
extern int sta_balance_list(int eid, request * wp, int argc, char **argv);
extern int rtk_set_lan_sta_load_balance(int eid, request * wp, int argc, char **argv);
extern int rtk_set_connect_load_balance(int eid, request * wp, int argc, char **argv);
extern int rtk_set_bandwidth_load_balance(int eid, request * wp, int argc, char **argv);
extern int LoadBalanceStatsList(int eid, request * wp, int argc, char **argv);
extern void formLdStats(request * wp, char *path, char *query);
extern int wan_stats_show(int eid, request * wp, int argc, char **argv);

extern int rtk_load_balance_wan_nums(int eid, request * wp, int argc, char **argv);
extern int rtk_load_balance_actual_wans(int eid, request * wp, int argc, char **argv);

extern int rtk_load_balance_wan_info(int eid, request * wp, int argc, char **argv);
extern int rtk_load_balance_chart_header_info(int eid, request * wp, int argc, char **argv);

extern void formLdStatsChart_demo(request * wp, char *path, char *query);
#endif
extern int ShowWanPortRateSetting(int eid, request * wp, int argc, char **argv);

#ifdef CONFIG_USER_MAP_E
extern void show_MAPE_settings(request *wp);
extern void show_MAPE_FMR_list(request *wp);
extern void formMAPE_FMR_WAN(request * wp, char *path, char *query);
extern void formMAPE_FMR_Edit(request * wp, char *path, char *query);
#endif

#if defined(CONFIG_USER_MAP_E) || defined(CONFIG_USER_MAP_T)
extern void showMAPTSettings(request *wp);
#endif

#ifdef USE_DEONET /* sghong, 20230511 */
extern void formAccessLimit(request * wp, char *path, char *query);
extern int showCurrentTime(int eid, request * wp, int argc, char **argv);
extern int hourTimeSet(int eid, request * wp, int argc, char **argv);
extern int minTimeSet(int eid, request * wp, int argc, char **argv);
extern int getConnectHostList(int eid, request * wp, int argc, char **argv);
extern int showCurrentAccessList(int eid, request * wp, int argc, char **argv);

extern void formAutoReboot(request * wp, char *path, char *query);
extern int showCurrentAutoRebootConfigInfo(int eid, request * wp, int argc, char **argv);
extern int ShowManualSetting(int eid, request * wp, int argc, char **argv);
extern int ShowAutoWanPortIdleValue(int eid, request * wp, int argc, char **argv);
extern int ShowAutoSetting(int eid, request * wp, int argc, char **argv);
extern int ShowAutoRebootDay(int eid, request * wp, int argc, char **argv);
extern int ShowRebootTargetTime(int eid, request * wp, int argc, char **argv);

extern void formNatSessionSetup(request * wp, char *path, char *query);
extern int ShowNatSessionCount(int eid, request * wp, int argc, char **argv);
extern int ShowNatSessionThreshold(int eid, request * wp, int argc, char **argv);

extern void formUseageSetup(request * wp, char *path, char *query);
extern int ShowUseageThreshold(int eid, request * wp, int argc, char **argv);

extern int nvramGet(int eid, request * wp, int argc, char **argv);
extern int portCrcCounterGet(int eid, request * wp, int argc, char **argv);

extern void formSelfCheck(request * wp, char *path, char *query);
extern int ShowPortStatus(int eid, request * wp, int argc, char **argv);
extern int ShowDaemonCheckStatus(int eid, request * wp, int argc, char **argv);
extern int ShowGatewayDnsCheckStatus(int eid, request * wp, int argc, char **argv);
extern int pwmacinit(int eid, request * wp, int argc, char **argv);
extern void formNickname(request * wp, char *path, char *query);
extern int ShowNicknameTable(int eid, request * wp, int argc, char **argv);
extern int ShowAutoRebootTargetTime(int eid, request * wp, int argc, char **argv);

extern int sshPortGet(int eid, request * wp, int argc, char **argv);
#endif

#endif // _INCLUDE_APFORM_H

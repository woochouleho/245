/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Authorization "module" (c) 1998,99 Martin Hinner <martin@tdp.cz>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

extern char *mgmtUserName(void);
extern char *mgmtPassword(void);

#include <stdio.h>
#include <fcntl.h>
#ifdef __UC_LIBC__
#include <unistd.h>
#else
#include <crypt.h>
#endif
#include "syslog.h"
#ifdef USE_LIBMD5
#include <libmd5wrapper.h>
#else
#include "md5.h"
#endif //USE_LIBMD5
#include "boa.h"
#include "port.h"
#ifdef SHADOW_AUTH
#include <shadow.h>
#endif
#ifdef OLD_CONFIG_PASSWORDS
#include <crypt_old.h>
#endif
#ifdef EMBED
#include <sys/types.h>
#include <pwd.h>
#include <config/autoconf.h>
#else
#include "../../../config/autoconf.h"
#endif

#include "LINUX/sysconfig.h"
#include <ctype.h>

#ifdef SECURITY_COUNTS
#include "../../login/logcnt.c"
#endif

#ifdef EMBED
// Added by Mason Yu for 2 level web page
#include "./LINUX/mib.h"

// Added by Mason Yu
//extern char usName[MAX_NAME_LEN];
#endif
#include "./LINUX/utility.h"

// Added by davian_kuo.
#include "./LINUX/form_src/multilang.h"

#include "asp_page.h"
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
#include "web_psd_encrypt.h"
#endif

#ifdef CONFIG_USER_LANNETINFO
#include "./LINUX/subr_net.h"
#endif


//add by xl_yue
#ifdef ACCOUNT_LOGIN_CONTROL
/*
extern char suName[MAX_NAME_LEN];
extern struct account_info su_account;
extern struct account_info us_account;
extern time_t time_counter;
extern struct errlogin_entry * errlogin_list;
*/
#define MAX_LOG_NUM 3
#endif

#ifdef USE_AUTH

#define DBG_DIGEST 1

struct _auth_dir_ {
	char *directory;
	FILE *authfile;
	int dir_len;
#if SUPPORT_AUTH_DIGEST
	int authtype; // 0:basic, 1:digest
	char *realm;
#endif
	struct _auth_dir_ *next;
};

typedef struct _auth_dir_ auth_dir;

static auth_dir *auth_hashtable [AUTH_HASHTABLE_SIZE];
#ifdef OLD_CONFIG_PASSWORDS
char auth_old_password[16];
#endif

#ifdef VOIP_SUPPORT
extern char *rcm_dbg_on;
#endif

#ifdef CONFIG_CU
extern unsigned char appscan_flag;
#endif

#ifdef WEB_AUTH_PRIVILEGE
char * userAuthPage[] = {
	"/admin/login.asp",
	"/admin/index_user.html",
#ifdef CONFIG_TRUE
	"status.asp",
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	"status_pon.asp",
#endif
#ifdef CONFIG_IPV6
	"status_ipv6.asp",
#endif
#if defined(CONFIG_USB_SUPPORT)
	"status_usb.asp",
#endif
	"tcpiplan.asp",
#ifdef WLAN_SUPPORT
	"wlbasic.asp",
	"wladvanced.asp",
	"wlwpa.asp",
	"wlactrl.asp",
	"wlsurvey.asp",
	"wlwps.asp",
#endif
#ifdef CONFIG_USER_DHCP_SERVER
	"dhcpd.asp",
#endif
#ifdef CONFIG_USER_DDNS
	"ddns.asp",
#endif
#if defined(CONFIG_USER_IGMPPROXY)
	"igmproxy.asp",
#endif
#if defined(CONFIG_USER_UPNPD)||defined(CONFIG_USER_MINIUPNPD)
	"upnp.asp",
#endif
#ifdef CONFIG_USER_ROUTED_ROUTED
	"rip.asp",
#endif
#ifdef CONFIG_USER_SAMBA
	"samba.asp",
#endif
	"fw-ipportfilter.asp",
#ifdef MAC_FILTER
	"fw-macfilter.asp",
#endif
#ifdef PORT_FORWARD_GENERAL
	"fw-portfw.asp",
#endif
	"static-mapping.asp",
#ifdef URL_BLOCKING_SUPPORT
	"url_blocking.asp",
#endif
#ifdef DOMAIN_BLOCKING_SUPPORT
	"domainblk.asp",
#endif
#ifdef DMZ
	"fw-dmz.asp",
#endif
	"fw-enable.asp",
	"remote_management.asp",
	"arptable.asp",
	"bridging.asp",
#ifdef ROUTING
	"routing.asp",
#endif
#ifdef CONFIG_USER_CUPS
	"printServer.asp",
#endif
#ifdef CONFIG_USER_IP_QOS
	"net_qos_imq_policy.asp",
	"net_qos_cls.asp",
	"net_qos_traffictl.asp",
#endif
#ifdef CONFIG_IPV6
	"ipv6_enabledisable.asp",
#ifdef CONFIG_USER_RADVD
	"radvdconf.asp",
#endif
	"routing_ipv6.asp",
#ifdef IP_PORT_FILTER
	"fw-ipportfilter-v6_IfId.asp",
#endif
#ifdef IP_ACL
	"aclv6.asp",
#endif
#endif//CONFIG_IPV6
	"reboot.asp",
#ifdef CONFIG_SAVE_RESTORE
	"saveconf.asp",
#endif
#ifdef CONFIG_USER_RTK_SYSLOG
	"syslog.asp",
#endif
	"user_syslog.asp",
	"holepunching.asp",
	"ldap.asp",
	"lanApConnection.asp",
	"wanApConnection.asp",
	"user-password.asp",
#ifdef WEB_UPGRADE
	"upgrade.asp",
#endif
#ifdef IP_ACL
	"acl.html",
#endif
#ifdef TIME_ZONE
	"tz.asp",
#endif
	"stats.asp",
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	"pon-stats.asp",
	"pon-flow-stats.asp",
#endif
#endif//CONFIG_TRUE
#ifdef CONFIG_CU
	"/admin/login_admin.asp",
	"/admin/login_others.asp",
	"/cu.html",
	"/CU.HTML",
	"/CU.html",
	"/cu.HTML",
	"/top_cmcc_yo.asp",
	"/usereg_inside_loid_cmcc_ln.asp",
	"/usereg_ln.asp",
	"/admin/index_user_ln.html",
	"/admin/index_ln.html",
	"/admin/login_yo_admin.asp",
	"/admin/index_yo_user.asp",
	"/main_user.asp",
	"/top_cmcc.asp",
#ifdef CONFIG_USER_WAN_MODE_SWITCH
	"/switch_wan_mode.asp",
#endif
	"usereg_inside_menu.asp",
	"status_xpon_register.asp",
	"net_wlan_empty.asp",
#endif
	"/index.html",
	"/top.asp",
	"/left.html",
#ifdef CONFIG_CT_AWIFI_JITUAN_FEATURE
	"/top_awifi.asp",
#endif
	"/left.asp",
	"/diag_dev_basic_info.asp",
	"/diag_net_connect_info.asp",
	"/diag_net_dsl_info.asp",
	"/diag_ethernet_info.asp",
	"/diag_usb_info.asp",
	"/status_tr069_info.asp",
	"/status_tr069_config.asp",
	"/diag_ping.asp",
	"/diag_tracert.asp",
	"/diagnose_tr069.asp",
	"/status_device_basic_info.asp",
	"/status_user_net_connet_info.asp",
	"/status_user_net_connet_info_ipv6.asp",
	"/status_user_net_dsl_info.asp",
	"/status_epon.asp",
	"/status_gpon.asp",
	"/status_ethernet_info.asp",
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_AP_CMCC)
	"/index_cmcc.html",
	"/top_cmcc.asp",
	"/status_ethernet_info_cmcc.asp",
	"/status_wlan_info_11n_24g_cmcc.asp",
	"/status_wlan_info_11n_5g_cmcc.asp",
	"/usereg_inside_loid_cmcc.asp",
#endif
#ifdef CONFIG_USER_LANNETINFO
	"/status_lan_net_info.asp",
#endif
#ifdef CONFIG_USER_LAN_BANDWIDTH_MONITOR
	"/status_lan_bandwidth_monitor.asp",
#endif
	"/status_usb_info.asp",
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	"/net_dhcpd_cmcc.asp",
	"/net_ipv6_cmcc.asp",
	"/algonoff.asp",
	"/fw-dmz.asp",
	"/app_nat_vrtsvr_cfg.asp",
#else
	"/net_dhcpd.asp",
	"/ipv6.asp",
	"/dhcpdv6.asp",
#endif
#ifdef WLAN_SUPPORT
	"/diag_wlan_info.asp",
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	"/status_wlan_info_11n_cmcc.asp",
#else
	"/status_wlan_info_11n.asp",
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_AP_CMCC)
	"/net_wlan_basic_11n_cmcc.asp",
#else
	"/net_wlan_basic_user_11n_e8.asp",
#endif
#endif
#ifdef CONFIG_CT_AWIFI_JITUAN_FEATURE
	"/awifi_unique_station.asp",
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	"/usereg_inside_menu_cmcc.asp",
#endif
	"/secu_firewall_level.asp",
	"/secu_macfilter_bridge.asp",
	"/secu_macfilter_router.asp",
	"/app_storage.asp",
	"/app_samba_account.asp",
#ifdef CONFIG_MCAST_VLAN
	"/app_iptv.asp",
#endif
#if defined(CONFIG_USER_RTK_VLAN_PASSTHROUGH)
    "/app_vlan_passthrough.asp",
#endif
	"/mgm_usr_user.asp",
	"/mgm_dev_reboot.asp",
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	"/mgm_dev_reset.asp",
	"/mgm_dev_reset_user_cmcc.asp",
#endif
	"/mgm_dev_usbbak.asp",
	"/mgm_dev_usb_umount.asp",
#ifdef SUPPORT_URL_FILTER
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
	"/secu_urlfilter_cfg_dbus_yueme4.asp",
	"/secu_urlfilter_add_dbus_yueme4.asp",
#else
	"/secu_urlfilter_cfg_dbus.asp",
	"/secu_urlfilter_add_dbus.asp",
#endif
#else
	"/secu_urlfilter_cfg.asp",
	"/secu_urlfilter_add.asp",
#endif
#ifdef SUPPORT_DNS_FILTER
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
	"/secu_dnsfilter_cfg_yueme4.asp",
	"/secu_dnsfilter_add_yueme4.asp",
#else
	"/secu_dnsfilter_cfg.asp",
	"/secu_dnsfilter_add.asp",
#endif
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	"/secu_firewall_level_cmcc.asp",
#else
	"/secu_firewall_level.asp",
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	"/secu_firewall_dosprev.asp",
	"/secu_firewall_ipv6.asp",
#endif
	"/secu_macfilter_bridge.asp",
	"/secu_macfilter_bridge_add.asp",
	"/secu_macfilter_router.asp",
	"/secu_macfilter_router_add.asp",
	"/secu_macfilter_src.asp",
	"/secu_macfilter_src_add.asp",
#ifdef WLAN_SEC_WEB_ENABLE
	"/wlan_macfilter_src.asp",
	"/wlan_macfilter_src_add.asp",
#endif
	"/net_mopreipaddr.asp",
	"/net_addbindmac.asp",
	"/net_dhcpdevice.asp",
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	"/help_cmcc/help_status_device.html",
	"/help_cmcc/help_status_net.html",
	"/help_cmcc/help_status_user.html",
	"/help_cmcc/help_status_tr069.html",
	"/help_cmcc/help_net_wlan.asp",
	"/help_cmcc/help_net_wlan5G.asp",
	"/help_cmcc/help_security_wanaccess.html",
	"/help_cmcc/help_security_firewall.html",
	"/help_cmcc/help_security_macfilter.html",
	"/help_cmcc/help_apply_familymemory.html",
	"/help_cmcc/help_manage_user.html",
	"/help_cmcc/help_manage_device.asp",
	"/help_cmcc/help_status_net.asp",
	"/help_cmcc/help_net_pon.html",
	"/help_cmcc/help_net_dhcp.html",
	"/help_cmcc/help_net_qos.html",
	"/help_cmcc/help_net_time.html",
	"/help_cmcc/help_net_remote.asp",
	"/help_cmcc/help_net_route.html",
	"/help_cmcc/help_apply_ddns.html",
	"/help_cmcc/help_apply_nat.html",
	"/help_cmcc/help_apply_upnp.html",
	"/help_cmcc/help_apply_igmp.html",
#else
	"/help/help_status_device.html",
	"/help/help_status_net.html",
	"/help/help_status_user.html",
	"/help/help_net_wlan.html",
	"/help/help_security_wanaccess.html",
	"/help/help_security_firewall.html",
	"/help/help_security_macfilter.html",
	"/help/help_apply_familymemory.html",
	"/help/help_manage_user.html",
	"/help/help_manage_device.html",
	"/help/help_status_net.asp",
	"/help/help_net_pon.html",
	"/help/help_net_dhcp.html",
	"/help/help_net_qos.html",
	"/help/help_net_time.html",
	"/help/help_net_remote.html",
	"/help/help_net_route.html",
	"/help/help_apply_ddns.html",
	"/help/help_apply_nat.html",
	"/help/help_apply_upnp.html",
	"/help/help_apply_igmp.html",
#endif
#ifdef VOIP_SUPPORT
	"/help/help_apply_voip.html",
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	"/cmcc_status_voip_info.asp",
#else
	"/status_voip_info.asp",
#endif
	"/help/help_status_voip.html",
	"/help/help_diag_voip.html",
#endif
#ifdef _PRMT_X_CT_COM_USERINFO_
	"/usereg.asp",
	"/useregresult.asp",
#endif
    "/dl_notify.asp",
#ifdef CONFIG_SUPPORT_PON_LINK_DOWN_PROMPT
    "/conn-fail.asp",
#endif
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
	"/opmode.asp",
#endif
#if defined(WLAN_SITE_SURVEY_SUPPORT)
	"/net_wlan_survey.asp",
#endif
#ifdef CONFIG_CMCC_ENTERPRISE
	"/network.asp",
	"/tr069-stun.asp",
	"/app_download_gatewayapp.asp",
	"/floid.asp",
	"/certification.asp",
	"/cetifization_pwd.asp",
	"/cetifization_SN.asp",
	"/useregresult_pwd.asp",
	"/cmcc_voip_quick_setup.asp",
	"/regstatus.asp",
	"/devinfo.asp",
	"/wanstatus.asp",
	"/laninfo.asp",
	"/voipinfo.asp",
	"/quickwifi.asp",
	"/dhcp.asp",
	"/wifi.asp",
	"/wifi5g.asp",
	"/ip_qos.asp",
	"/vpn.asp",
	"/url_filter.asp",
	"/firewall.asp",
	"/mac_filter.asp",
	"/ip_filter.asp",
	"/nat.asp",
	"/ddns.asp",
	"/upnp.asp",
	"/igmp.asp",
	"/mld.asp",
	"/user.asp",
	"/device.asp",
	"/ntp.asp",
	"/language.asp",
	"/diag.asp",
	"/help_status.asp",
	"/help_network.asp",
	"/help_security.asp",
	"/help_admin.asp",
	"/pluginmanager.asp",
	"/logout.asp",
	"/logout.html",
	"/login.asp",
#endif
	0
};

#ifdef VOIP_SUPPORT
/* SD6, bohungwu, RCM voip engineering web page */
char *suserAuthPageVoIP[] = {
	"/voip_index.asp",
	"/voip_config.asp",
	"/voip_general.asp",
	"/voip_other.asp",
	"/voip_ring.asp",
	"/voip_script.js",
	"/voip_sip_status.asp",
	"/voip_tone.asp",
	0
};
#endif
#endif // #ifdef WEB_AUTH_PRIVILEGE

// simple function to strip leading space and ending space/LF/CR
// Magician (2007.12.27): Modify to trim all white spaces, and solve some possible secure problems.
char *trim(char *input)
{
	char *tmp, *ret;
	int i, len;

	tmp = input;

	len = strlen(input);
	for( i = 0; i < len; i++ )  // Trim leading spaces.
	{
		if(isspace(*tmp))
			tmp++;
		else
			break;
	}

	len = strlen(tmp);
	for( i = len - 1; i >= 0; i-- )  // Trim trailing spaces.
	{
		if( isspace(tmp[i]) )
			tmp[i] = '\0';
		else
			break;
	}

	return tmp;
}

int istrimed(char chr, char *trimchr)
{
	int i, len;

	if(isspace(chr))
		return 1;

	len = strlen(trimchr);
	for( i = 0; i < len; i++ )
		if( trimchr[i] == chr )
			return 1;

	return 0;
}

//Magician 2007/12/27: Extended for trim function.
char *trimEx(char *input, char *tmchr)
{
	char *tmp, *ret;
	int i, len;

	tmp = input;

	len = strlen(input);
	for( i = 0; i < len; i++ )  // Trim leading spaces.
	{
		if(istrimed(*tmp, tmchr))
			tmp++;
		else
			break;
	}

	len = strlen(tmp);
	for( i = len - 1; i >= 0; i-- )  // Trim trailing spaces.
	{
		if( istrimed(tmp[i], tmchr) )
			tmp[i] = '\0';
		else
			break;
	}

	return tmp;
}

#if SUPPORT_AUTH_DIGEST

//static struct http_session *digest_session, digest_session0;
static struct http_session session_array[HTTP_SESSION_MAX]; // support 2 session only

static struct http_session * http_session_get() {
	int i;
	for (i=0; i< HTTP_SESSION_MAX; i++) {
		if (session_array[i].in_use)
			continue;

		session_array[i].in_use = 1;
		return &session_array[i];
	}
	return 0;
}

static void http_session_free(struct http_session *s) {
	s->in_use = 0;
}

#define soap_random rand()
static void http_da_calc_nonce(char nonce[HTTP_DA_NONCELEN])
{
  static short count = 0xCA53;
  sprintf(nonce, "%8.8x%4.4hx%8.8x", (int)time(NULL), count++, soap_random);
}

static void http_da_calc_opaque(char opaque[HTTP_DA_OPAQUELEN])
{
  sprintf(opaque, "%8.8x", soap_random);
}

static void s2hex(const unsigned char *src, char *dst, int len) {
	int i;
	for (i=0; i < len; i++) {
		sprintf(dst, "%02x", src[i]);
		dst += 2;
	}
	*dst = 0;
}

static void http_da_calc_HA1(struct MD5Context *pCtx, char *alg, char *userid, char *realm, char *passwd, char *nonce, char *cnonce, char HA1hex[33])
{

	char HA1[16];

	MD5Init(pCtx);
	MD5Update(pCtx, userid, strlen(userid));
	MD5Update(pCtx, ":", 1);
	MD5Update(pCtx, realm, strlen(realm));
 	MD5Update(pCtx, ":", 1);
	MD5Update(pCtx, passwd, strlen(passwd));
 	MD5Final(HA1, pCtx);

	if (alg && strncasecmp(alg, "MD5-session", 11)) {
		#if DBG_DIGEST
		fprintf(stderr, "alg = %s\n", alg);
		#endif
		MD5Init(pCtx);
		MD5Update(pCtx, HA1, 16);
		MD5Update(pCtx, ":", 1);
		MD5Update(pCtx, nonce, strlen(nonce));
 		MD5Update(pCtx, ":", 1);
		MD5Update(pCtx, cnonce, strlen(cnonce));
 		MD5Final(HA1, pCtx);
	}

	s2hex(HA1, HA1hex, 16);
	#if DBG_DIGEST
	fprintf(stderr, "HA1: MD5(%s:%s:%s)=%s\n", userid, realm, passwd, HA1hex);
	#endif
};

static void http_da_calc_response(struct MD5Context *pCtx, char HA1hex[33], char *nonce, char *ncount, char *cnonce, char *qop, char *method, char *uri, char entityHAhex[33], char response[33])
{
	char HA2[16], HA2hex[33], responseHA[16];

	MD5Init(pCtx);
	MD5Update(pCtx, method, strlen(method));
	MD5Update(pCtx, ":", 1);
	MD5Update(pCtx, uri, strlen(uri));
	if (!strncasecmp(qop, "auth-int", 8))
	{
		MD5Update(pCtx, ":", 1);
		MD5Update(pCtx, entityHAhex, 32);
	}
 	MD5Final(HA2, pCtx);
	s2hex(HA2, HA2hex, 16);

	MD5Init(pCtx);
	MD5Update(pCtx, HA1hex, 32);
	MD5Update(pCtx, ":", 1);
	MD5Update(pCtx, nonce, strlen(nonce));
	MD5Update(pCtx, ":", 1);

  	if (qop && *qop)
  	{
  		MD5Update(pCtx, ncount, strlen(ncount));
		MD5Update(pCtx, ":", 1);
		MD5Update(pCtx, cnonce, strlen(cnonce));
		MD5Update(pCtx, ":", 1);
		MD5Update(pCtx, qop, strlen(qop));
		MD5Update(pCtx, ":", 1);
    	}
	MD5Update(pCtx, HA2hex, 32);
	MD5Final(responseHA, pCtx);



  	s2hex(responseHA, response, 16);
	#if DBG_DIGEST
	fprintf(stderr, "HA2: MD5(%s:%s)=%s\n", method, uri, HA2hex);
	fprintf(stderr, "Response: MD5(%s:%s:%s:%s:%s:%s)=%s\n", HA1hex, nonce, ncount, cnonce, qop, HA2hex, response);
	#endif
}



static void http_da_session_cleanup()
{
	struct http_session *s;
	time_t now = time(NULL);

	//MUTEX_LOCK(http_da_session_lock);
	int i;
	for (i = 0; i < HTTP_SESSION_MAX; i++) {
		s = &session_array[i];

		if (!s->in_use)
			continue;

		// not expired yet.
		if (s->modified + HTTP_DA_EXPIRY_TIME > now)
			continue;

		http_session_free(s);
	}
  //MUTEX_UNLOCK(http_da_session_lock);
}

#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
static struct http_session * http_da_session_start(const char *realm, const char *nonce, const char *opaque, request *req)
#else
static struct http_session * http_da_session_start(const char *realm, const char *nonce, const char *opaque)
#endif
{

	struct http_session *session;
	time_t now = time(NULL);
	static int count = 0;

	if((count++ % 10) == 0) /* don't do this all the time to improve efficiency */
 		http_da_session_cleanup();

  	//MUTEX_LOCK(http_da_session_lock);

  	session = http_session_get();
	if (session)
  {
  	//session->next = http_da_session;
		session->modified = now;
		if (nonce)
			strncpy(session->nonce, nonce, sizeof(session->nonce));
		else
			http_da_calc_nonce(session->nonce);

#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
		if(req->remote_ip_addr)
			strncpy(session->remote_ip_addr, req->remote_ip_addr, sizeof(session->remote_ip_addr));
#endif


		if (opaque)
			strncpy(session->opaque, opaque, sizeof(session->opaque));
		else
			http_da_calc_opaque(session->opaque);

		strncpy(session->realm,realm, sizeof(session->realm));
    		session->ncount = 0;
    		//http_da_session = session;
  	}

	return session;
  	//MUTEX_UNLOCK(http_da_session_lock);
}

#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
static struct http_session * http_da_session_update(const char *realm, const char *nonce, const char *opaque, const char *cnonce, const char *ncount, request *req)
#else
static struct http_session * http_da_session_update(const char *realm, const char *nonce, const char *opaque, const char *cnonce, const char *ncount)
#endif
{
	int i;
	struct http_session *s;
#if VERIFY_OPAQUE
  	if (!realm || !nonce || !opaque || !cnonce || !ncount)
    		return 0;
#else
  	if (!realm || !nonce || !cnonce || !ncount)
    		return 0;
#endif
  //MUTEX_LOCK(http_da_session_lock);
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
	http_da_session_cleanup();
#endif
	for (i = 0; i < HTTP_SESSION_MAX; i++)
	{
		s = &session_array[i];
    if (!s->in_use)
      continue;

		#if DBG_DIGEST
    	fprintf(stderr, "session nonce=%s; client resend nonce=%s\n", s->nonce, nonce);
		#endif

		#if VERIFY_OPAQUE
		#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
		if (!strcmp(s->realm, realm) && !strcmp(s->nonce, nonce) && !strcmp(s->opaque, opaque) && !strcmp(s->remote_ip_addr, req->remote_ip_addr))
		#else
   		if (!strcmp(s->realm, realm) && !strcmp(s->nonce, nonce) && !strcmp(s->opaque, opaque))
		#endif
   			break;
 		#else
		#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
		if (!strcmp(s->realm, realm) && !strcmp(s->nonce, nonce) && !strcmp(s->remote_ip_addr, req->remote_ip_addr))
		#else
   		if (!strcmp(s->realm, realm) && !strcmp(s->nonce, nonce))
		#endif
			break;
 		#endif
	}

	#if DBG_DIGEST
		fprintf(stderr, "session ncount=%d; client ncount=%d\n", s->ncount, strtoul(ncount, NULL, 16));
	#endif

	if (i < HTTP_SESSION_MAX)
	{
		unsigned long nc = strtoul(ncount, NULL, 16);

// Magician 01/14/2008
/* For non-sequential sending by FireFox, ignore ncount number comparison.
		if (s->ncount >= nc)
		{
			s->modified = 0;
			http_session_free(s);
		}
		else
		{
*/
			s->ncount = nc;
			s->modified = time(NULL);
//		}
	}
	else
		return 0;

	return s;
}

static void _auth_find_value(const char *input, const char * const token, char **ppVal, int *pSize)
{
	char del1[] = ",", del2[] = "=", *tmp, *tok, *value, *toks[11];
	char tmchr[] = "\"';=-()[]*&^%$#@!~`{}[]?,.+";
	//int i = 0;
	int i = 0, offset, valLen;

	tmp = strdup(strstr(input, "username"));

	for( tok = strtok(tmp, del1); tok != NULL && i < 10; tok = strtok(NULL, del1), i++ )
		toks[i] = tok;

	toks[i] = NULL;

	for( i = 0; i < 11; i++ )
	{
		if( !toks[i] )
			break;
#if 0
		if((tok = strtok(toks[i], del2)) && (value = strtok(NULL, del2)) && !strcmp(trim(tok), token))
		{
			value = trimEx(value, tmchr);
			*ppVal = value;
			*pSize = strlen(value);
			break;
		}
#endif
		char *buf = strdup(toks[i]);
		if((tok = strtok(toks[i], del2)) && !strcmp(trim(tok), token)){
			if(!memcmp(buf+strlen(tok), "=\"", 2) && buf[strlen(buf)-1]=='\"'){/*string type*/
				offset = strlen(tok)+2;
				valLen = strlen(buf)-offset-1;
			}else{
				offset = strlen(tok)+1;
				valLen = strlen(buf)-offset;
			}
			value = (char *)malloc(valLen+1);
			strncpy(value, buf+offset, valLen);
			value[valLen] = '\0';

			*ppVal = value;
			*pSize = strlen(value);
			free(buf);
			break;
		}

		free(buf);
	}

	free(tmp);
}


// return length of the value. -1 if token is not in input.
static int auth_find_value(const char *input, const char * const token, char *buf, int len) {
	char *pc=NULL;
	int tmp;
	memset(buf, 0, len);
	_auth_find_value(input, token, &pc, &tmp);
	if (pc) {
		strncpy(buf, pc, tmp);
		free(pc);
		return tmp;
	}
	return -1;
}


#endif

/*
 * Name: get_auth_hash_value
 *
 * Description: adds the ASCII values of the file letters
 * and mods by the hashtable size to get the hash value
 */
static inline int get_auth_hash_value(char *dir)
{
#ifdef EMBED
	return 0;
#else
	unsigned int hash = 0;
	unsigned int index = 0;
	unsigned char c;

	hash = dir[index++];
	while ((c = dir[index++]) && c != '/')
		hash += (unsigned int) c;

	return hash % AUTH_HASHTABLE_SIZE;
#endif
}

/*
 * Name: auth_add
 *
 * Description: adds
 */
void * auth_add(char *directory, char *file)
{
	auth_dir *new_a, *old;

	old = auth_hashtable[get_auth_hash_value(directory)];
	while (old)
	{
		if (!strcmp(directory, old->directory))
			return 0;
		old = old->next;
	}

	new_a = (auth_dir *)malloc(sizeof(auth_dir));
	/* success of this call will be checked later... */
	new_a->authfile = fopen(file, "rt");
	new_a->directory = strdup(directory);
	new_a->dir_len = strlen(directory);
	new_a->next = auth_hashtable [get_auth_hash_value(directory)];
	auth_hashtable [get_auth_hash_value(directory)] = new_a;

	return (void *)new_a;
}

#if SUPPORT_AUTH_DIGEST
void auth_add_digest(char *directory, char *file, char *realm) {

	auth_dir *dir;
	dir = (auth_dir *)auth_add(directory,file);
	if (dir) {
		dir->authtype = HTTP_AUTH_DIGEST;
		dir->realm = strdup(realm);

		fprintf(stderr, "Added Digest: %s, %s, %s\n", directory, file, realm);
	}
}
#endif

void auth_check()
{
	int hash;
	auth_dir *cur;

	for (hash=0;hash<AUTH_HASHTABLE_SIZE;hash++)
	{
  	cur = auth_hashtable [hash];
	  while (cur)
		{
			if (!cur->authfile)
			{
				log_error_time();
				fprintf(stderr,"Authentication password file for %s not found!\n",
						cur->directory);
			}
			cur = cur->next;
		}
	}
}

#ifdef LDAP_HACK
#include <lber.h>
#include <ldap.h>
#include <sg_configdd.h>
#include <sg_confighelper.h>
#include <sg_users.h>

/* Return a positive, negative or not possible result to the LDAP auth for
 * the specified user.
 */
static int ldap_auth(const char *const user, const char *const pswd) {
	static time_t last;
	static char *prev_user;
	LDAP *ld;
	int ldap_ver, r;
	char f[256];
	ConfigHandleType *c = config_load(amazon_ldap_config_dd);

	/* Don't repeat query too often if the user name hasn't changed */
	if (last && prev_user &&
			time(NULL) < (last + config_get_int(c, AMAZON_LDAP_CACHET)) &&
			strcmp(prev_user, user) == 0) {
		config_free(c);
		last = time(NULL);
		return 1;
	}
	if (prev_user != NULL)   { free(prev_user);	prev_user = NULL;   }
	last = 0;

	if ((ld = ldap_init(config_get_string(c, AMAZON_LDAP_HOST),
			config_get_int(c, AMAZON_LDAP_PORT))) == NULL) {
		syslog(LOG_ERR, "unable to initialise LDAP");
		config_free(c);
		return 0;
	}
	if (ldap_set_option(ld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF) != LDAP_SUCCESS) {
		syslog(LOG_ERR, "unable to set LDAP referrals off");
		config_free(c);
		ldap_unbind(ld);
		return 0;
	}
	ldap_ver = config_get_int(c, AMAZON_LDAP_VERSION);
	if (ldap_ver > 0 && ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &ldap_ver) != LDAP_SUCCESS) {
		syslog(LOG_ERR, "unable to set LDAP version %d", ldap_ver);
		config_free(c);
		ldap_unbind(ld);
		return 0;
	}
	snprintf(f, sizeof f, config_get_string(c, AMAZON_LDAP_BIND_DN), user);
	r = ldap_simple_bind_s(ld, f, pswd);
	if (r != LDAP_SUCCESS) {
		syslog(LOG_ERR, "unable to connect to LDAP (%s)", ldap_err2string(r));
		config_free(c);
		ldap_unbind(ld);
		return (r==LDAP_INVALID_CREDENTIALS)?-1:0;
	}
	config_free(c);
	ldap_unbind(ld);
	/* Caching timing details for next time through */
	prev_user = strdup(user);
	last = time(NULL);
	return 1;
}
static unsigned char ldap_succ;
#endif
#ifdef CONFIG_CT_AWIFI_JITUAN_FEATURE
static int is_host_self_2ndip(const char *host)
{
	unsigned char lan_ip[IP_ADDR_LEN] = {0};
	char lan_ip_str[INET_ADDRSTRLEN] = {0};

	if(host == NULL)
		return 0;

#ifdef CONFIG_SECONDARY_IP
	mib_get(MIB_ADSL_LAN_IP2, lan_ip);
	inet_ntop(AF_INET, lan_ip, lan_ip_str, INET_ADDRSTRLEN);
	if(strstr(host, lan_ip_str))
		return 1;
#endif

	return 0;
}
#endif

static int is_host_self(const char *host)
{
	unsigned char lan_ip[IP_ADDR_LEN] = {0};
	char lan_ip_str[INET_ADDRSTRLEN] = {0};

	if(host == NULL)
		return 0;

	mib_get_s(MIB_ADSL_LAN_IP, lan_ip, sizeof(lan_ip));
	inet_ntop(AF_INET, lan_ip, lan_ip_str, INET_ADDRSTRLEN);

	if(strstr(host, lan_ip_str))
		return 1;

#ifdef CONFIG_IPV6
	{
		char lan_ip6_str[INET6_ADDRSTRLEN] = {0};
		unsigned char host_ip6[IP6_ADDR_LEN] = {0};
		unsigned char lan_ip6[IP6_ADDR_LEN] = {0};


		mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, lan_ip6_str, sizeof(lan_ip6_str));
		inet_pton(AF_INET6, lan_ip6_str, lan_ip6);

		if(host[0] == '[')
		{
			char host_ip6_str[INET6_ADDRSTRLEN] = {0};
			char *find = NULL;

			find = strchr(host, ']');
			if(find)
			{

				strncpy(host_ip6_str, host+1, (size_t)(find - (intptr_t)host - 1));
				//fprintf(stderr, "host_ip6_str=%s\n", host_ip6_str);
				if(inet_pton(AF_INET6, host_ip6_str, host_ip6) == 1 && memcmp(lan_ip6, host_ip6, IP6_ADDR_LEN) == 0)
					return 1;
			}
		}
	}
#endif

	return 0;
}

int checkIsFromWifi(request * req)
{
#ifdef CONFIG_USER_LANNETINFO
	int ret=0, entryNum=0, i, connType=0;
	lanHostInfo_t *pLanNetInfo=NULL;
#ifdef EMBED
        char ipAddr[INET_ADDRSTRLEN]={0};
#endif

	ret = get_lan_net_info(&pLanNetInfo, &entryNum);
	if(ret<0)
		goto err;

	for(i=0; i < entryNum; i++)
	{
		inet_ntop(AF_INET, &(pLanNetInfo[i].ip), ipAddr, INET_ADDRSTRLEN);
		connType =  pLanNetInfo[i].connectionType;   //0:Ethernet 1:802.11
		if((strstr(req->remote_ip_addr, ipAddr)!=NULL) && (connType==1))
		{
			if(pLanNetInfo)
				free(pLanNetInfo);
			return 1;
		}
	}

err:
	if(pLanNetInfo)
		free(pLanNetInfo);
#endif
	return 0;
}

/*
 * Name: auth_check_userpass
 *
 * Description: Checks user's password. Returns 0 when sucessful and password
 * 	is ok, else returns nonzero; As one-way function is used RSA's MD5 w/
 *  BASE64 encoding.
#ifdef EMBED
 * On embedded environments we use crypt(), instead of MD5.
#endif
 */
int auth_check_userpass(char *user, char *pass, FILE *authfile, request * req)
{
#ifdef LDAP_HACK

	/* Yeah, code before declarations will fail on older compilers... */
	switch (ldap_auth(user, pass)) {
	case -1:	ldap_succ = 0;				return 1;
	case 0:		ldap_succ = strcmp(user, "root")?0:1;	break;
	case 1:
		ldap_succ = 1;
		if (start_user_update(0) == 0)
			done_user_update(set_user_password(user, pass, 0)==0?1:0);
		return 0;
	}
#endif
#ifdef SHADOW_AUTH
	struct spwd *sp;

	sp = getspnam(user);
	if (!sp)
		return 2;

	if (!strcmp(crypt(pass, sp->sp_pwdp), sp->sp_pwdp))
		return 0;

#else

//#ifndef EMBED
#if 1

#ifdef EMBED
	char temps[0x100],*pwd;
	//struct MD5Context mc;
 	unsigned char final[16];
	char encoded_passwd[0x40];
	int userflag = 0;
	// Added by Mason Yu (2 leval)
	FILE *fp=NULL;
	char disabled;

#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
	char HA1hex[33]={0},password_HA1[MAX_WEB_PASSWORD_LEN]={0},realm[MAX_REALM_LEN]={0};
	//struct MD5Context md5ctx;
	int userflag_login=0;
#endif


   	/* Encode password ('pass') using one-way function and then use base64
	encoding. */

	// Added by Mason Yu(2 level)
	//if ( strcmp("user", user)==0 && strcmp(req->request_uri, "/")==0 ) {

	// Added by Mason Yu for boa memory leak
	// User access web every time, the boa will auth once. And use strdup to allocate memory for directory_index.
	// So We should free the memory space of older directory_index to avoid memory leak.
#ifndef MULTI_USER_PRIV
	if (directory_index) free(directory_index);

#ifndef ACCOUNT_CONFIG
	if (strcmp(usName, user)==0)
#else
	if (getAccPriv(user) == (int)PRIV_USER)
#endif
	{
		directory_index = strdup("index_user.html");
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
		userflag_login=1;
#endif
		if (strcmp(req->request_uri, "/")==0)
		{
			fp = fopen("/var/boaUser.passwd", "r");
			// Jenny
			//authfile = fp;
			userflag = 1;
		}
	}
	else
#ifdef CONFIG_VDSL
#ifdef CONFIG_GENERAL_WEB
		directory_index = strdup("index.html");
#else
		directory_index = strdup("index_vdsl.html");
#endif
#else
		directory_index = strdup("index.html");
#endif

#endif //#ifndef MULTI_USER_PRIV

#if 0
	MD5Init(&mc);
	{
	//char *pass="admin";
	MD5Update(&mc, pass, strlen(pass));
	}
	MD5Final(final, &mc);
	strcpy(encoded_passwd,"$1$");
	base64encode(final, encoded_passwd+3, 16);
#endif
	char *xpass;
	char salt[128];
	rtk_get_crypt_salt(salt, sizeof(salt));
	xpass = crypt(pass, salt);

	DBG(printf("auth_check_userpass(%s,%s,...);\n",user,pass);)

#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
	if(userflag_login){
		mib_get_s(MIB_WEB_USER_REALM, (void *)realm, sizeof(realm));
		mib_get_s(MIB_WEB_USER_PASSWORD, (void *)password_HA1, sizeof(password_HA1));
		web_auth_digest_encrypt(user, realm, pass, HA1hex);
		if (!strcmp(password_HA1,HA1hex)) {
			//printf("auth_check_userpass1(%s,%s,...);\n",user,pass);
			if(fp)
				fclose(fp);
			userflag = 0;
			return 0;
		}
	}
#else
	if (userflag) {
		fseek(fp, 0, SEEK_SET);
		while (fgets(temps, 0x100, fp)) {
			if (temps[strlen(temps)-1]=='\n')
				temps[strlen(temps)-1] = 0;
			pwd = strchr(temps,':');
			if (pwd) {
				*pwd++=0;
				if (!strcmp(temps,user)) {
					if (!strcmp(pwd,xpass)) {
						fclose(fp);
						userflag = 0;
						return 0;
					}
				} else
					continue;
			}
		}
	}
#endif

	else {
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
	mib_get_s(MIB_WEB_USER_REALM, (void *)realm, sizeof(realm));
	mib_get_s(MIB_WEB_SUSER_PASSWORD, (void *)password_HA1, sizeof(password_HA1));
	web_auth_digest_encrypt(user, realm, pass, HA1hex);
	if (!strcmp(password_HA1,HA1hex)) {
		//printf("auth_check_userpass1(%s,%s,...);\n",user,pass);
		return 0;
	}
#else
	fseek(authfile, 0, SEEK_SET);
	while (fgets(temps,0x100,authfile))
	{
		if (temps[strlen(temps)-1]=='\n')
			temps[strlen(temps)-1] = 0;
		pwd = strchr(temps,':');
		if (pwd)
		{
			*pwd++=0;
			if (!strcmp(temps,user))
			{
				if (!strcmp(pwd,xpass)) {
					/*
					// Added by Mason Yu for web page via serverhost
					if ((strcmp(req->request_uri, "/")==0) && (!strcmp(usName, user)
					#ifdef ACCOUNT_CONFIG
						|| getAccPriv(user) == (int)PRIV_USER
					#endif
					))
						fclose(fp);
					*/
					return 0;
				}
			} else {
				// Modified by Mason Yu for multi user with passwd file.
				continue;
				//return 2;
			}
		}
	}
#endif
	}

	if (userflag) {
		fclose(fp);
		userflag = 0;
	}
	// Added by Mason Yu for web page via serverhost
	/*
	if ((strcmp(req->request_uri, "/")==0) && (!strcmp(usName, user)
	#ifdef ACCOUNT_CONFIG
		|| getAccPriv(user) == (int)PRIV_USER
	#endif
	))
		fclose(fp);
	*/
#else
//printf("user=%s, pass=%s name=%s pass2=%s\n",user,pass,mgmtUserName(),mgmtPassword());
	//if((strcmp(mgmtUserName(),user)==0)&&(strcmp(mgmtPassword(),pass)==0)) return 0;
	//else return 2;
	return 0;
#endif

#else
	struct passwd *pwp;

	pwp = getpwnam(user);
	if (pwp != NULL) {
		if (strcmp(crypt(pass, pwp->pw_passwd), pwp->pw_passwd) == 0)
			return 0;
	} else
#ifdef OLD_CONFIG_PASSWORDS
	/* For backwards compatibility we allow the global root password to work too */
	if ((auth_old_password[0] != '\0') &&
			((*user == '\0') || (strcmp(user, "root") == 0))) {
		if (strcmp(crypt_old(pass,auth_old_password),auth_old_password) == 0 ||
				strcmp(crypt(pass,auth_old_password),auth_old_password) == 0) {
			strcpy(user, "root");
			return 0;
		}
	} else
#endif	/* OLD_CONFIG_PASSWORDS */
		return 2;

#endif	/* ! EMBED */
#endif	/* SHADOW_AUTH */
	return 1;
}

#ifdef ACCOUNT_LOGIN_CONTROL
//added by xl_yue
struct errlogin_entry * scan_errlog_list(request * req)
{
	struct errlogin_entry * perrlog;

	for(perrlog = errlogin_list;perrlog;perrlog = perrlog->next){
		if(!strcmp(perrlog->remote_ip_addr, req->remote_ip_addr))
			break;
	}

	return perrlog;
}
//added by xl_yue
int check_errlogin_list(request * req)
{
	struct errlogin_entry * perrlog;
	struct errlogin_entry * berrlog = NULL;

	for(perrlog = errlogin_list;perrlog;berrlog = perrlog,perrlog = perrlog->next){
		if(!strcmp(perrlog->remote_ip_addr, req->remote_ip_addr))
			break;
	}

	if(!perrlog)
		return 0;

	if(perrlog->login_count >= MAX_LOG_NUM){
		return -1;
	}
	else{//unlist and free
		if(perrlog == errlogin_list)
			errlogin_list = perrlog->next;
		else
			berrlog->next = perrlog->next;

		free(perrlog);
	}
	return 0;
}
#endif

static void Send_Unauthorized(auth_dir *current, request *req) {
#if SUPPORT_AUTH_DIGEST
	if (current->authtype == HTTP_AUTH_DIGEST) {
		struct http_session *s;
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
		char realm[MAX_REALM_LEN]={0};
		mib_get_s(MIB_WEB_USER_REALM, (void *)realm, sizeof(realm));
		if ((s = http_da_session_start(realm, 0, 0, req)) != NULL) {
#else
		if ((s = http_da_session_start(current->realm, 0, 0)) != NULL) {
#endif

			send_r_unauthorized_digest(req, s);
			return;
		}
		// no more session available
		fprintf(stderr, "No more session available\n");
		send_r_bad_request(req);
	}
	else
#endif
	send_r_unauthorized(req,server_name);
}

extern int getSYSInfoTimer();

static int is_except_page(request *req)
{
	if(strncmp(req->request_uri, "./", 2))
		if(strlen(req->request_uri) == 1 && req->referer != NULL)
			return 1;
	return 0;
}
int auth_authorize(request * req)
{
	int i, len, denied = 1;
	auth_dir *current;
 	int hash;
	char *pwd, *method;
#ifndef ACCOUNT_LOGIN_CONTROL
	static char current_client[20];
#endif
	char auth_userpass[0x80];
#ifdef WEB_AUTH_PRIVILEGE
	char **pAuthPage;
	int idx;
	int authen=0;
#endif
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
	int is_super_user=-1;   //0: user  1:super_user 2:adsl
	char password_HA1[MAX_WEB_PASSWORD_LEN]={0};
#endif

	//added by xl_yue
#ifdef ACCOUNT_LOGIN_CONTROL
	struct account_info * current_account_info = NULL;
	struct errlogin_entry * err_login;
	int do_succeed_log = 0;
#endif

//xl_yue
#ifdef USE_LOGINWEB_OF_SERVER
	struct user_info * pUser_info;
#ifdef LOGIN_ERR_TIMES_LIMITED
	struct errlogin_entry * pErrlog_entry = NULL;
#endif
#endif

	DBG(printf("auth_authorize\n");)
#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_AP_CMCC)
	if (req->is_cgi == JSON){
		printf("[%s %d] request_uri=%s, is_cgi=%d, bypass auth temporary\n",__FUNCTION__,__LINE__,req->request_uri,req->is_cgi);
		return 1;
	}
#endif

	hash = get_auth_hash_value(req->request_uri);
	current = auth_hashtable[hash];
		while (current) {
  		if (!memcmp(req->request_uri, current->directory,
								current->dir_len)) {
			if (current->directory[current->dir_len - 1] != '/' &&
				        req->request_uri[current->dir_len] != '/' &&
								req->request_uri[current->dir_len] != '\0') {
				break;
			}
//xl_yue add
#ifndef USE_LOGINWEB_OF_SERVER
			if (req->authorization)
			{
				if (current->authfile==0)
				{
					send_r_error(req);
					return 0;
				}

			#if SUPPORT_AUTH_DIGEST
				if (!strncasecmp(req->authorization,"Digest ",7))
				{
					struct MD5Context md5ctx;
//					char username[33],realm[34],nonce[33],qop[33];
					char realm[34],nonce[33],qop[33];
					char cnonce[33],response[34],nc[33],opaque[33];
					char HA1hex[33], entityHAhex[33], myresponse[33];
					char uri[256];
					struct http_session *session;

//					auth_find_value(req->authorization + 7, "username", username, sizeof(username));
					auth_find_value(req->authorization + 7, "username", auth_userpass, sizeof(auth_userpass));
					auth_find_value(req->authorization + 7, "realm", realm, sizeof(realm));
					auth_find_value(req->authorization + 7, "nonce", nonce, sizeof(nonce));
					auth_find_value(req->authorization + 7, "qop", qop, sizeof(qop));
					auth_find_value(req->authorization + 7, "cnonce", cnonce, sizeof(cnonce));
					auth_find_value(req->authorization + 7, "response", response, sizeof(response));
					auth_find_value(req->authorization + 7, "opaque", opaque, sizeof(opaque));
					auth_find_value(req->authorization + 7, "nc", nc, sizeof(nc));
					auth_find_value(req->authorization + 7, "uri", uri, sizeof(uri));

					#if DBG_DIGEST
						//char uri[256];
						//auth_find_value(req->authorization + 7, "uri", uri, sizeof(uri));
						fprintf(stderr,"Client inputs:\n");
//						fprintf(stderr,"  username=%s\n", username);
						fprintf(stderr,"  auth_userpass=%s\n", auth_userpass);
						fprintf(stderr,"  realm=%s\n", realm);
						fprintf(stderr,"  nonce=%s\n", nonce);
						fprintf(stderr,"  qop=%s\n", qop);
						fprintf(stderr,"  cnonce=%s\n", cnonce);
						fprintf(stderr,"  response=%s\n", response);
						fprintf(stderr,"  opaque=%s\n", opaque);
						fprintf(stderr,"  uri=%s\n", uri);
						fprintf(stderr,"  nc=%s\n", nc);
					#endif
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
					if((session = http_da_session_update(realm, nonce, opaque, cnonce, nc, req)))
#else
					if((session = http_da_session_update(realm, nonce, opaque, cnonce, nc)))
#endif
					{
						char tmpStr[64];
						char *str;

#ifndef MULTI_USER_PRIV
//						if(!strcmp(username, usName))
						if(!strcmp(auth_userpass, usName))
						{
							fclose(current->authfile);
							current->authfile = fopen("/var/DigestUser.passwd", "r");
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
							is_super_user=0;
#endif

							if (directory_index)
								free(directory_index);

							directory_index = strdup("index_user.html");
						}
//						else if(!strcmp(username, suName))
						else if(!strcmp(auth_userpass, suName))
						{
							fclose(current->authfile);
							current->authfile = fopen("/var/DigestSuper.passwd", "r");
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
							is_super_user=1;
#endif
							if (directory_index)
								free(directory_index);

#ifdef CONFIG_VDSL
#ifdef CONFIG_GENERAL_WEB
							directory_index = strdup("index.html");
#else
							directory_index = strdup("index_vdsl.html");
#endif
#else
							directory_index = strdup("index.html");
#endif
						}
//						else if(!strcmp(username, "adsl"))
						else if(!strcmp(auth_userpass, "adsl"))
						{
							fclose(current->authfile);
							current->authfile = fopen("/var/DigestSuper.passwd", "r");
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
							is_super_user=2;
#endif

							if (directory_index)
								free(directory_index);
#ifdef CONFIG_VDSL
#ifdef CONFIG_GENERAL_WEB
							directory_index = strdup("index.html");
#else
							directory_index = strdup("index_vdsl.html");
#endif
#else
							directory_index = strdup("index.html");
#endif
						}
#endif

						#if DBG_DIGEST
							fprintf(stderr, "DEBUG: enter http_da_session_update session\n");
						#endif

						fseek(current->authfile, 0, SEEK_SET);
						while (fgets(tmpStr, sizeof(tmpStr), current->authfile))
						{
							str = trim(tmpStr);

							#if DBG_DIGEST
//								fprintf(stderr, "DEBUG: tmpStr(username)=%s\n", tmpStr);
								fprintf(stderr, "DEBUG: tmpStr(auth_userpass)=%s\n", tmpStr);
							#endif

//							if (!strcmp(username, tmpStr) || !strcmp(username, "adsl"))
							if (!strcmp(auth_userpass, tmpStr) || !strcmp(auth_userpass, "adsl"))
							{
								fgets(tmpStr, sizeof(tmpStr),current->authfile);
								str = trim(tmpStr);

//								if(!strcmp(username, "adsl"))
								if(!strcmp(auth_userpass, "adsl"))
									str = strdup("realtek");

								#if DBG_DIGEST
									fprintf(stderr, "DEBUG: tmpStr(password)=%s\n", tmpStr);
								#endif

								switch(req->method)
								{
									case M_GET:
										method = strdup("GET");
										break;
									case M_HEAD:
										method = strdup("HEAD");
										break;
									case M_PUT:
										method = strdup("PUT");
										break;
									case M_POST:
										method = strdup("POST");
										break;
									case M_DELETE:
										method = strdup("DELETE");
										break;
									case M_LINK:
										method = strdup("LINK");
										break;
									case M_UNLINK:
										method = strdup("UNLINK");
										break;
									default:
										method = strdup("");
								}

//								http_da_calc_HA1(&md5ctx, NULL, username, session->realm, str, session->nonce, cnonce, HA1hex);
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
								if(0 ==is_super_user){
									mib_get_s(MIB_WEB_USER_PASSWORD, (void *)password_HA1, sizeof(password_HA1));
									strncpy(HA1hex, password_HA1, sizeof(HA1hex)-1);
								}
								else if(1== is_super_user){
									mib_get_s(MIB_WEB_SUSER_PASSWORD, (void *)password_HA1, sizeof(password_HA1));
									strncpy(HA1hex, password_HA1, sizeof(HA1hex)-1);
								}
								else if(2== is_super_user){
									http_da_calc_HA1(&md5ctx, NULL, auth_userpass, session->realm, str, session->nonce, cnonce, HA1hex);
								}
#else
								http_da_calc_HA1(&md5ctx, NULL, auth_userpass, session->realm, str, session->nonce, cnonce, HA1hex);
#endif
								//http_da_calc_response(&md5ctx, HA1hex, session->nonce, nc, cnonce, qop, method, req->request_uri, entityHAhex, myresponse);
								http_da_calc_response(&md5ctx, HA1hex, session->nonce, nc, cnonce, qop, method, uri, entityHAhex, myresponse);

								#if DBG_DIGEST
									fprintf(stderr, "DEBUG:   reponse=%s\n", response);
									fprintf(stderr, "DEBUG: myreponse=%s\n\n", myresponse);
								#endif

								free(method);

								if(!strcmp(response, myresponse))
								{
									denied = 0;
									break;
								}
							}
							else
								fgets(tmpStr, sizeof(tmpStr),current->authfile);
						}
					}
				}
				else
				{
			#endif  // SUPPORT_AUTH_DIGEST

					if (strncasecmp(req->authorization,"Basic ",6))
					{
						syslog(LOG_ERR, "Can only handle Basic auth\n");
						send_r_bad_request(req);
						return 0;
					}

					#if SUPPORT_AUTH_DIGEST
					if (current->authtype == HTTP_AUTH_DIGEST)
					{
						Send_Unauthorized(current, req);
						return 0;
					}
					#endif

					base64decode(auth_userpass, req->authorization+6, sizeof(auth_userpass));

					if ( (pwd = strchr(auth_userpass,':')) == 0 )
					{
						syslog(LOG_ERR, "No user:pass in Basic auth\n");
						send_r_bad_request(req);
						return 0;
					}

					*pwd++=0;
					// Modified by Mason Yu
					//denied = auth_check_userpass(auth_userpass,pwd,current->authfile);
					denied = auth_check_userpass(auth_userpass,pwd,current->authfile, req);
			#if SUPPORT_AUTH_DIGEST
				} // digest check
			#endif
#ifdef SECURITY_COUNTS
				if (strncmp(get_mime_type(req->request_uri),"image/",6))
					access__attempted(denied, auth_userpass);
#endif

#ifdef ACCOUNT_LOGIN_CONTROL
				//added by xl_yue
				if(!denied){
					//check if this ip in errlogin list
					if(check_errlogin_list(req)){
						denied = 5;
						goto pass_check;
					}

					//for user
					if (strcmp(usName, auth_userpass)==0){
						current_account_info = &us_account;
//						printf("select us_account\n");
					}
					//for super user
					else if(strcmp(suName, auth_userpass)==0){
						current_account_info = &su_account;
//						printf("select su_account\n");
					}
					else{
						current_account_info = NULL;
//						printf("select none account\n");
					}

					if(current_account_info){
						if (strcmp(current_account_info->remote_ip_addr, req->remote_ip_addr) == 0){
//							printf("the same ip\n");
							if(current_account_info->account_timeout){			//need login again for no acction for 5 minutes
								current_account_info->account_timeout = 0;
								current_account_info->last_time = time_counter;
								denied = 3;
//								printf("account timeout\n");
							}else{
//								printf("account not timeout\n");
								current_account_info->last_time = time_counter;
								current_account_info->account_busy = 1;
								//req->paccount_info = current_account_info;
								//current_account_info->refcnt++;
							}
						}else{
//							printf("not same ip\n");
							//only one user can login using the same account at the same time
							if(current_account_info->account_busy){
//								printf("account busy\n");
								denied = 4;
							}else{
//								printf("account not busy\n");
								current_account_info->account_timeout = 0;
								current_account_info->last_time = time_counter;
								current_account_info->account_busy = 1;
								do_succeed_log = 1;
								strncpy(current_account_info->remote_ip_addr, req->remote_ip_addr, sizeof(current_account_info->remote_ip_addr));
								//req->paccount_info = current_account_info;
								//current_account_info->refcnt++;
							}
						}
					}
				}else{
					/*login error three times,should wait 1 minutes */
					if((err_login = scan_errlog_list(req)) == NULL){
						err_login = (struct errlogin_entry*) malloc(sizeof(struct errlogin_entry));
						if (!err_login)
							die(OUT_OF_MEMORY);
						err_login->login_count = 0;
						strncpy(err_login->remote_ip_addr, req->remote_ip_addr, sizeof(err_login->remote_ip_addr));
						//list err_login to errlogin_list
						err_login->next = errlogin_list;
						errlogin_list = err_login;
					}
					if((++(err_login->login_count)) >= MAX_LOG_NUM){
						//if have login,then exit
						if(!strcmp(su_account.remote_ip_addr, req->remote_ip_addr)){
							su_account.account_timeout = 0;
							su_account.account_busy = 0;
							su_account.remote_ip_addr[0] = '\0';
						}
						if(!strcmp(us_account.remote_ip_addr, req->remote_ip_addr)){
							us_account.account_timeout = 0;
							us_account.account_busy = 0;
							us_account.remote_ip_addr[0] = '\0';
						}

						if(err_login->login_count == MAX_LOG_NUM)
							err_login->last_time = time_counter;

						denied = 5;
					}else{
						err_login->last_time = time_counter;
					}
				}
#endif

pass_check:
				if (denied) {
					switch (denied) {
						case 1:
							syslog(LOG_ERR, "Authentication attempt failed for %s from %s because: Bad Password\n", auth_userpass, req->remote_ip_addr);
							#if DBG_DIGEST
								fprintf(stderr, "DEBUG %d: Authentication attempt failed for %s from %s because: Bad Password\n", denied, auth_userpass, req->remote_ip_addr);
							#endif
							break;
						case 2:
							syslog(LOG_ERR, "Authentication attempt failed for %s from %s because: Invalid Username\n",	auth_userpass, req->remote_ip_addr);
							#if DBG_DIGEST
								fprintf(stderr, "DEBUG %d: Authentication attempt failed for %s from %s because: Invalid Username\n",	denied, auth_userpass, req->remote_ip_addr);
							#endif
							break;
#ifdef ACCOUNT_LOGIN_CONTROL
						//added by xl_yue
						case 3:
							syslog(LOG_ERR, "Authentication attempt failed for %s from %s because:login has timeouted \n", auth_userpass, req->remote_ip_addr);
							#if DBG_DIGEST
								fprintf(stderr, "DEBUG %d: Authentication attempt failed for %s from %s because:login has timeouted \n", denied, auth_userpass, req->remote_ip_addr);
							#endif
							break;
						//added by xl_yue
						case 4:
							syslog(LOG_ERR, "Authentication attempt failed for %s from %s because: another has logined\n", auth_userpass, req->remote_ip_addr);
							#if DBG_DIGEST
								fprintf(stderr, "DEBUG %d: Authentication attempt failed for %s from %s because: another has logined\n", denied, auth_userpass, req->remote_ip_addr);
							#endif
							send_r_forbidden(req);
							return 0;
							break;
						//added by xl_yue
						case 5:
							syslog(LOG_ERR, "Authentication attempt failed for %s from %s because: have logined error for three times\n", auth_userpass, req->remote_ip_addr);
							#if DBG_DIGEST
								fprintf(stderr, "DEBUG %d: Authentication attempt failed for %s from %s because: have logined error for three times\n", denied, auth_userpass, req->remote_ip_addr);
							#endif
							send_r_forbidden(req);
							return 0;
							break;
#endif
						}
#ifndef ACCOUNT_LOGIN_CONTROL
					bzero(current_client, sizeof(current_client));
#endif
					Send_Unauthorized(current, req);
					return 0;


				}
#ifdef ACCOUNT_LOGIN_CONTROL
				if(do_succeed_log){
					syslog(LOG_INFO, "Authentication successful for %s from %s\n", auth_userpass, req->remote_ip_addr);
				}
#else
				if (strcmp(current_client, req->remote_ip_addr) != 0) {
					len= sizeof(current_client);
					memset(current_client, 0, len);
					strncpy(current_client, req->remote_ip_addr, len -1);
					syslog(LOG_INFO, "Authentication successful for %s from %s\n", auth_userpass, req->remote_ip_addr);
				}
#endif
				/* Copy user's name to request structure */
#ifdef LDAP_HACK
				if (!ldap_succ) {
					strcpy(req->user, "noldap");
					syslog(LOG_INFO, "Access granted as noldap");
				} else
#endif
				strncpy(req->user, auth_userpass, 15);
				req->user[15] = '\0';
				return 1;
			}else
			{
				/* No credentials were supplied. Tell them that some are required */
#ifdef ACCOUNT_LOGIN_CONTROL
				//added by xl_yue:if have login error for three times, forbid logining until 1 minute later
				if((err_login = scan_errlog_list(req)) != NULL){
					if(err_login->login_count >= MAX_LOG_NUM){
						send_r_forbidden(req);
						return 0;
					}
				}
#endif
				Send_Unauthorized(current, req);
				return 0;
			}
#else
//xl_yue
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
			/*For opmode changed, the logged Lan host may get different IP, so clear the logged info to let the same host can login again*/
			if(isFileExist(RTK_CLEAR_BOA_LOGIN_INFO_FILE)){
				free_login_list();
				unlink(RTK_CLEAR_BOA_LOGIN_INFO_FILE);
			}
#endif

				pUser_info = search_login_list(req);

				if(pUser_info 
					&& 
					(req->cookie || is_except_page(req))){
					//user account can not access admin account directory
#ifdef CONFIG_CMCC_ENTERPRISE
/*					unsigned int reset_flag, pon_mode;
					mib_get(MIB_WEB_REDIRECT_BY_RESET, &reset_flag);
					mib_get(MIB_PON_MODE, &pon_mode);
					if(!strcmp(req->request_uri, "/quickregister.html"))
					{
						if(reset_flag)
							return 0;
						char url[64] = {0};
						if(pon_mode)
							strcpy(url, "/certification.asp");
						else
							strcpy(url, "/network.asp");
						boaRedirectTemp(req, url);
						return 0;
					}
*/
#endif

#ifdef CONFIG_VDSL
					if((strcmp(req->request_uri,"/")) && (!strcmp(current->directory,"/")) && (!strcmp(pUser_info->directory,"index_user.html")))
					{
						send_r_forbidden(req);
						return 0;
					}
#endif

#ifdef CONFIG_CT_AWIFI_JITUAN_FEATURE
					unsigned char functype=0;
					mib_get_s(AWIFI_PROVINCE_CODE, &functype, sizeof(functype));
					if(functype == AWIFI_ZJ){
						if(is_host_self_2ndip(req->host)){
							send_r_forbidden(req);
							return 0;
						}
					}
#endif
#ifdef WEB_AUTH_PRIVILEGE
#ifdef CONFIG_CMCC_ENTERPRISE
					if ((strcmp(req->request_uri,"/") &&    (strstr(req->request_uri, ".asp") || strstr(req->request_uri, ".html"))) && ((pUser_info->priv==1 && !(strncmp(req->request_uri,"/bd/",4)) && (strstr(req->request_uri, ".asp") || strstr(req->request_uri, ".html")))))
#else
					//ql_xu: check the privilege of submitted page according to userAuthPage list and suserAuthPage list.
					if ((strcmp(req->request_uri,"/") &&	(strstr(req->request_uri, ".asp") || strstr(req->request_uri, ".html")|| strstr(req->request_uri, ".HTML"))) && (!pUser_info->priv || (pUser_info->priv==1 && !(strncmp(req->request_uri,"/bd/",4)) && (strstr(req->request_uri, ".asp") || strstr(req->request_uri, ".html")))))
#endif
					{
						authen = 0;
						pAuthPage = userAuthPage;

						for (idx=0; pAuthPage[idx]; idx++)
						{
							if (strstr(req->request_uri, pAuthPage[idx])) {
								authen = 1;
								break;
							}
						}
#ifdef VOIP_SUPPORT
						/* SD6, bohungwu, add engineering web page for VoIP */
						if ((rcm_dbg_on!=NULL) && (!authen))
						{
							//printf("RCM_DBG_ON env var is SET, start checking\n");
							for (idx=0; suserAuthPageVoIP[idx]; idx++)
							{
								if (strstr(req->request_uri, suserAuthPageVoIP[idx])) {
									authen = 1;
									break;
								}
							}
						}
						else
						{
							//printf("RCM_DBG_ON env var is NOT set, skip check\n");
						}
#endif /* #ifdef VOIP_SUPPORT */

						if (!authen)
						{
							send_r_not_found(req);
							return 0;
						}
					}
#endif
					directory_index = pUser_info->directory;
					time_counter = getSYSInfoTimer();
					pUser_info->last_time = time_counter;
					return 1;
				}else{
#ifdef LOGIN_ERR_TIMES_LIMITED
#ifdef CONFIG_CU
			if(appscan_flag == 0)
#endif
 			{
					pErrlog_entry = (struct errlogin_entry*)search_errlog_list(req);
					if(pErrlog_entry){
						time_counter = getSYSInfoTimer();
						if(pErrlog_entry->login_count % MAX_LOGIN_NUM == 0 && (time_counter - pErrlog_entry->last_time) <= LOGIN_ERR_WAIT_TIME){
							int urilen = strlen(req->request_uri);
							//printf("time %ld\n", LOGIN_ERR_WAIT_TIME - (time_counter - pErrlog_entry->last_time));
							if((urilen > 1) && (req->request_uri[urilen-1] == '/'))
								send_r_not_found(req); /* return 404-not found when path is a dir to pass appscan check */
							else
#ifdef CONFIG_USER_BOA_WITH_MULTILANG
								send_r_forbidden_timeout(req, LOGIN_ERR_WAIT_TIME - (time_counter - pErrlog_entry->last_time));
#else
#ifdef CONFIG_E8B
								send_r_forbidden3(req, LOGIN_ERR_WAIT_TIME - (time_counter - pErrlog_entry->last_time));
#else
								send_r_forbidden(req);
#endif

#endif
							return 0;
						}
					}
				}
#endif
#ifdef CONFIG_CT_AWIFI_JITUAN_FEATURE
					unsigned char functype=0;
					mib_get_s(AWIFI_PROVINCE_CODE, &functype, sizeof(functype));
					if(functype == AWIFI_ZJ){
						if(is_host_self_2ndip(req->host)){
							send_r_forbidden(req);
							return 0;
						}
					}
#endif
#ifdef CONFIG_CMCC_ENTERPRISE
					if(strcmp(req->request_uri, "/certification.asp")!=0
						&& strcmp(req->request_uri, "/cetifization_SN.asp")!=0
						&& strcmp(req->request_uri, "/network.asp")!=0
						&& strcmp(req->request_uri, "/app_download_gatewayapp.asp")!=0
						&& strcmp(req->request_uri, "/cetifization_pwd.asp")!=0
						&& strcmp(req->request_uri, "/regresult.asp")!=0
						&& strstr(req->request_uri, ".asp")
						|| (strstr(req->request_uri, ".html")))
					{
						if(checkIsFromWifi(req)&&!check_user_is_registered())
						{
							boaRedirectTemp(req, "/certification.asp");
							return 0;
						}
					}
#endif
#ifdef CONFIG_CU
					if(!strcasecmp(req->request_uri,"/cu.html")){
						strcpy(req->request_uri,"/cu.html");
						strtolower(req->pathname,strlen(req->pathname));
					}
#endif
					if(
#ifdef CONFIG_CU_BASEON_CMCC
						(!strcmp(req->request_uri,"/admin/login.asp") && web_style_ln !=1)
#else
						!strcmp(req->request_uri,"/admin/login.asp")
#endif
#ifdef CONFIG_CU_BASEON_YUEME
						|| !strcmp(req->request_uri,"/admin/login_admin.asp")
						|| !strcmp(req->request_uri,"/admin/login_others.asp")
						|| !strcmp(req->request_uri,"/main_admin.htm")
						|| !strcmp(req->request_uri,"/main_user.htm")
						|| !strcmp(req->request_uri,"/cu.html")
#elif defined CONFIG_CU_BASEON_CMCC
						|| (!strcmp(req->request_uri,"/admin/login_admin.asp")&& web_style_ln !=1)
						|| (!strcmp(req->request_uri,"/admin/login_others.asp")&& web_style_ln !=1)
						|| !strcmp(req->request_uri,"/main_admin.htm")
						|| !strcmp(req->request_uri,"/main_user.htm")

						|| (!strcmp(req->request_uri,"/cu.html") && web_style_ln !=1)
#endif

#ifdef CONFIG_GENERAL_WEB
						|| !strncmp(req->request_uri,"/graphics/", 10)
						|| !strcmp(req->request_uri,"/admin/reset.css")
						|| !strcmp(req->request_uri,"/admin/sk_reset.css")
						|| !strcmp(req->request_uri,"/admin/common.css")
						|| !strcmp(req->request_uri,"/admin/base.css")
						|| !strcmp(req->request_uri,"/admin/style.css")
						|| !strcmp(req->request_uri,"/admin/bootstrap.min.css")
						|| !strcmp(req->request_uri,"/admin/LoginFiles/logo.png")
						|| !strcmp(req->request_uri,"/admin/LoginFiles/user.png")
						|| !strcmp(req->request_uri,"/admin/LoginFiles/key.png")
#endif
						|| !strcmp(req->request_uri,"/admin/code.asp")
						|| !strcmp(req->request_uri,"/admin/md5.js")
						|| !strcmp(req->request_uri,"/admin/rollups/md5.js")
						|| !strcmp(req->request_uri,"/admin/php-crypt-md5.js")
						|| !strcmp(req->request_uri,"/common.js")
						|| !strcmp(req->request_uri,"/base64_code.js")
						|| !strcmp(req->request_uri,"/sha256_code.js")
						|| !strcmp(req->request_uri,"/admin/code_user.asp")
						|| !strcmp(req->request_uri,"/boaform/admin/userLogin")
						|| !strcmp(req->request_uri,"/boaform/admin/userLoginMultilang") // added by davian_kuo
						|| !strcmp(req->request_uri,"/admin/LoginFiles/logo.jpg")
#ifdef CONFIG_TELMEX_DEV
						|| !strcmp(req->request_uri,"/cookie.js")
						|| !strcmp(req->request_uri,"/form_submit_handler.js")
						|| !strcmp(req->request_uri,"/modal.js")
						|| !strcmp(req->request_uri,"/versions.js")
						|| !strcmp(req->request_uri,"/style/modal.css")
						|| !strcmp(req->request_uri,"/admin/footer.asp")
						|| !strcmp(req->request_uri,"/boaform/isUserConnected")
						|| !strcmp(req->request_uri,"/boaform/ConnectedUser")
#endif
#ifdef CONFIG_CMCC_ENTERPRISE
						|| !strcmp(req->request_uri,"/js/menu_dir.js")
						|| !strcmp(req->request_uri,"/devinfo.asp")
#endif
#if defined(CONFIG_00R0)
						|| !strcmp(req->request_uri,"/admin/LoginFiles/logo_RT_h_new.png")
						|| !strcmp(req->request_uri,"/admin/trouble_wizard_index.asp")
						|| !strcmp(req->request_uri,"/admin/wizard_screen_5_1.asp")
						|| !strcmp(req->request_uri,"/admin/wizard_screen_5_2.asp")
						|| !strcmp(req->request_uri,"/admin/wizard_screen_4_1_2.asp")
						|| !strcmp(req->request_uri,"/boaform/admin/formWizardScreen5_1")
						|| !strcmp(req->request_uri,"/boaform/admin/formWizardScreen5_2")
						|| !strcmp(req->request_uri,"/boaform/admin/formWizardScreen4_2")
						|| !strcmp(req->request_uri,"/admin/wizard_screen_0_1_2019.asp")
						|| !strcmp(req->request_uri,"/admin/wizard_screen_1_2019.asp")
						|| !strcmp(req->request_uri,"/admin/wizard_screen_1_1_2019.asp")
						|| !strcmp(req->request_uri,"/admin/wizard_screen_2_1_2019.asp")
						|| !strcmp(req->request_uri,"/admin/wizard_screen_2_2_2019.asp")
						|| !strcmp(req->request_uri,"/admin/wizard_screen_index.asp")
						|| !strcmp(req->request_uri,"/boaform/admin/getInitUrl_2019")
						|| !strcmp(req->request_uri,"/boaform/admin/formWizardScreen0_1_2019")
						|| !strcmp(req->request_uri,"/boaform/admin/formWizardScreen1_2019")
						|| !strcmp(req->request_uri,"/boaform/admin/formWizardScreen1_1_2019")
						|| !strcmp(req->request_uri,"/boaform/admin/formWizardScreen2_1_2019")
						|| !strcmp(req->request_uri,"/boaform/admin/formWizardScreen2_2_2019")

						|| !strcmp(req->request_uri,"/admin/wizard.jpg")
						|| !strcmp(req->request_uri,"/admin/reset.css")
						|| !strcmp(req->request_uri,"/admin/sk_reset.css")
						|| !strcmp(req->request_uri,"/admin/style.css")
						|| !strcmp(req->request_uri,"/admin/base.css")
						//TODO: modify the exactly files need for wizard and trouble wizard
						|| strstr(req->request_uri,".css")
						|| strstr(req->request_uri,".js")
						|| strstr(req->request_uri,".jpg")
						|| strstr(req->request_uri,".gif")
						|| strstr(req->request_uri,".png")
#endif
						|| !strcmp(req->request_uri,"/admin/LoginFiles/locker.gif")
						|| !strcmp(req->request_uri,"/favicon.ico")
#if defined(CONFIG_E8B)
						|| !strcmp(req->request_uri,"/login.cgi")
						|| !strcmp(req->request_uri,"/boaform/formUserReg")
						|| !strcmp(req->request_uri,"/image/logo.gif")
						|| !strcmp(req->request_uri,"/image/loid_register.gif")
						|| !strcmp(req->request_uri,"/image/bg_ah_login.gif")
						|| !strcmp(req->request_uri,"/image/bg_ah_login_01.gif")
						|| !strcmp(req->request_uri,"/image/useregresult_ah.gif")
						|| !strcmp(req->request_uri,"/image/loidreg_ah.gif")
						|| !strcmp(req->request_uri,"/image/loidreg_ah_01.gif")
						|| !strcmp(req->request_uri,"/nprogress.js")
						|| !strcmp(req->request_uri,"/style/default.css")
						|| !strcmp(req->request_uri,"/style/nprogress.css")
#ifdef SUPPORT_USEREG_FEATURE
						|| !strcmp(req->request_uri,"/usereg.asp")
#endif
#ifdef CONFIG_SUPPORT_PON_LINK_DOWN_PROMPT
						|| !strcmp(req->request_uri,"/conn-fail.asp")
#endif
						|| !strcmp(req->request_uri,"/useregresult.asp")
#ifdef BOOT_SELF_CHECK
						|| !strcmp(req->request_uri,"/selfcheckfail.asp")
#endif
#ifdef SUPPORT_WEB_PUSHUP
						|| !strcmp(req->request_uri, "/upgrade_pop.asp")
						|| !strcmp(req->request_uri, "/boaform/admin/formUpgradePop")
						|| !strcmp(req->request_uri, "/boaform/formUpgradeRedirect")
#endif
#ifdef SUPPORT_LOID_BURNING
						|| !strcmp(req->request_uri, "/boaform/form_loid_burning")
#endif
#ifdef SUPPORT_ZERO_CONFIG
						|| !strcmp(req->request_uri, "/itms")
#endif
#ifdef CONFIG_VIR_BOA_JEMBENCHTEST
						|| !strcmp(req->request_uri, "/jembenchtest")
#endif
#ifdef TERMINAL_INSPECTION_SC
						|| !strcmp(req->request_uri, "/terminal_inspec.asp")
#endif
#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
						|| !strcmp(req->request_uri, "/dl_notify.asp")
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
						|| !strcmp(req->request_uri,"/style/backgroup_style.css")
						|| !strcmp(req->request_uri,"/image/qrcode.png")
						|| !strcmp(req->request_uri,"/image/mobile2.png")
						|| !strcmp(req->request_uri,"/jquery-3.2.1.min.js")
						|| !strcmp(req->request_uri,"/image/cancel_cmcc.gif")
						|| !strcmp(req->request_uri,"/boaform/admin/formBundleInstall")
						|| !strcmp(req->request_uri,"/style/loading.css")
#ifdef CONFIG_USER_ANDLINK_FST_BOOT_MONITOR
						|| !strcmp(req->request_uri,"/admin/fstBootChecking_status.asp")
#endif
#endif
#if defined(CONFIG_CMCC_ENTERPRISE)
						|| !strcmp(req->request_uri,"/network.asp")
						|| !strcmp(req->request_uri,"/image/arrow.png")
						|| !strcmp(req->request_uri,"/image/favicon.png")
						|| !strcmp(req->request_uri,"/images/cmcc_login.png")
						|| !strcmp(req->request_uri,"/images/loginUsername.png")
						|| !strcmp(req->request_uri,"/images/loginPassword.png")
						|| !strcmp(req->request_uri,"/floid.asp")
						|| !strcmp(req->request_uri,"/certification.asp")
						|| !strcmp(req->request_uri,"/cetifization_pwd.asp")
						|| !strcmp(req->request_uri,"/cetifization_SN.asp")
						|| !strcmp(req->request_uri,"/useregresult_pwd.asp")
						|| !strcmp(req->request_uri,"/boaform/formUserReg_inside_menu_pwd")
						|| !strcmp(req->request_uri,"/boaform/formUserReg_inside_menu_h5")
						|| !strcmp(req->request_uri,"/image/welcome.png")
						|| !strcmp(req->request_uri,"/image/welcometwo.png")
						|| !strcmp(req->request_uri,"/image/welcomethree.png")
						|| !strcmp(req->request_uri,"/image/welcomefour.png")
						|| !strcmp(req->request_uri,"/image/welcomefive.png")
						|| !strcmp(req->request_uri,"/image/welcomesix.png")
						|| !strcmp(req->request_uri,"/image/welcomeseven.png")
						|| !strcmp(req->request_uri,"/image/gatewayapp.png")
						|| !strcmp(req->request_uri,"/image/quick_body.png")
						|| !strcmp(req->request_uri,"/image/quick_body@2x.png")
						|| !strcmp(req->request_uri,"/image/quick_body@05x.png")
						|| !strcmp(req->request_uri,"/image/quick_ok.png")
						|| !strcmp(req->request_uri,"/image/quick_error.png")
						|| !strcmp(req->request_uri,"/style/ont_quick.css")
						|| !strcmp(req->request_uri,"/style/amazeui.min.css")
						|| !strcmp(req->request_uri,"/style/common.css")
						|| !strcmp(req->request_uri,"/style/ont.css")
						|| !strcmp(req->request_uri,"/style/login.css")
						|| !strcmp(req->request_uri,"/style/floid.css")
						|| !strcmp(req->request_uri,"/style/ip_qos.css")
						|| !strcmp(req->request_uri,"/style/log.css")
						|| !strcmp(req->request_uri,"/style/logout.css")
						|| !strcmp(req->request_uri,"/style/logout_pop.css")
						|| !strcmp(req->request_uri,"/style/quick_reg.css")
						|| !strcmp(req->request_uri,"/style/regresult.css")
						|| !strcmp(req->request_uri,"/style/regstatus.css")
						|| !strcmp(req->request_uri,"/style/useregresult.css")
						|| !strcmp(req->request_uri,"/style/useregresult_pwd.css")
						|| !strcmp(req->request_uri,"/js/jquery.js")
						|| !strcmp(req->request_uri,"/js/common.js")
						|| !strcmp(req->request_uri,"/js/url.js")
						|| !strcmp(req->request_uri,"/js/md5.js")
						|| !strcmp(req->request_uri,"/style/certification.css")
						|| !strcmp(req->request_uri,"/js/amazeui.min.js")
						|| !strcmp(req->request_uri,"/js/jquery.min.js")
						|| !strcmp(req->request_uri,"/js/base64_code.js")
						|| !strcmp(req->request_uri,"/base64_code.js")
						|| !strcmp(req->request_uri,"/js/sha256_code.js")
						|| !strcmp(req->request_uri,"/sha256_code.js")
						|| !strcmp(req->request_uri,"/h5.js")
						|| !strcmp(req->request_uri,"/fonts/fontawesome-webfont.ttf")
						|| !strcmp(req->request_uri,"/regstatus.asp")
						|| !strcmp(req->request_uri,"/regresult.asp")
						|| !strcmp(req->request_uri,"/cmcc_voip_quick_setup.asp")
						|| !strcmp(req->request_uri,"/app_download_gatewayapp.asp")
						|| !strcmp(req->request_uri,"/logout_pop.asp")
						|| !strcmp(req->request_uri,"/boaform/admin/formEthernet_cmcc")
						|| !strcmp(req->request_uri,"/boaform/admin/VoipQuickConfig_cmcc")
#endif
#ifdef CONFIG_CU
						|| !strcmp(req->request_uri,"/image/Unicom_bg.jpg")
						|| !strcmp(req->request_uri,"/image/logo_cu.png")
						|| !strcmp(req->request_uri,"/image/bottom.png")

						|| !strcmp(req->request_uri,"/image/others.png")
						|| !strcmp(req->request_uri,"/image/admin.png")
						|| !strcmp(req->request_uri,"/image/idreg.png")
						|| !strcmp(req->request_uri,"/image/pass_go.png")
						|| !strcmp(req->request_uri,"/image/pass_input.png")
						|| !strcmp(req->request_uri,"/image/loid.png")
						|| !strcmp(req->request_uri,"/image/error_icon.png")
						|| !strcmp(req->request_uri,"/image/right_icon.png")
						|| !strcmp(req->request_uri,"/image/progress_bar.png")
						|| !strcmp(req->request_uri,"/image/adv.png")
						|| !strcmp(req->request_uri,"/image/advance_cfg.png")
						|| !strcmp(req->request_uri,"/image/basic_cfg.png")
						|| !strcmp(req->request_uri,"/image/basic.png")
						|| !strcmp(req->request_uri,"/image/equipment.png")
						|| !strcmp(req->request_uri,"/image/exit.png")
						|| !strcmp(req->request_uri,"/image/help.png")
						|| !strcmp(req->request_uri,"/image/manage.png")
						|| !strcmp(req->request_uri,"/image/quick_profile.png")
						|| !strcmp(req->request_uri,"/image/status.png")
						|| !strcmp(req->request_uri,"/image/regdevicebtn.gif")
#ifdef CONFIG_CU_BASEON_CMCC
						|| !strcmp(req->request_uri,"/image/lanup.jpg")
						|| !strcmp(req->request_uri,"/image/landown.jpg")
						|| !strcmp(req->request_uri,"/style/default_yo.css")
						|| !strcmp(req->request_uri,"/image/bgLN.jpg")
						|| !strcmp(req->request_uri,"/image/bgreg.jpg")
						|| !strcmp(req->request_uri,"/admin/index_yo_user.asp")
						|| !strcmp(req->request_uri,"/admin/login_yo_admin.asp")

						|| !strcmp(req->request_uri,"/usereg_ln.asp")
						|| !strcmp(req->request_uri,"/useregresult_yo.asp")
						|| !strcmp(req->request_uri,"/image/bggreen.jpg")
						|| !strcmp(req->request_uri,"/image/bgreging.jpg")
						|| !strcmp(req->request_uri,"/image/zc0.PNG")
						|| !strcmp(req->request_uri,"/image/zc1.PNG")
						|| !strcmp(req->request_uri,"/image/zc2.PNG")
						|| !strcmp(req->request_uri,"/top_cmcc_yo.asp")
#endif
#endif
#endif
						)
					{
#ifdef CONFIG_VIR_BOA_JEMBENCHTEST
						if(!strcmp(req->request_uri, "/jembenchtest")){
							req->is_cgi = 1;
							printf("##xxxx##Access %s\n",req->request_uri);
							return 1;
						}else
#endif
						return 1;
					}
#ifdef CONFIG_VIR_BOA_JEMBENCHTEST
#ifdef CONFIG_E8B
extern void freeRequestSocket(request * req);
#endif
					if(strstr(req->request_uri,"/jembenchtest")){
#ifdef CONFIG_E8B
						freeRequestSocket(req);
#endif
						return 0;
					}
#endif
					directory_index = NULL;
#if defined(CONFIG_E8B)
					//get LAN IP
					struct in_addr lan_ip;
					char lan_ip_str[INET_ADDRSTRLEN] = {0};

					if(getInAddr("br0",IP_ADDR,(void *)&lan_ip) != 1){
						mib_get_s(MIB_ADSL_LAN_IP, (void *)&lan_ip, sizeof(lan_ip));
					}
					inet_ntop(AF_INET, (void *)&lan_ip, lan_ip_str, INET_ADDRSTRLEN);

#ifdef SUPPORT_USEREG_FEATURE
					if(is_host_self(req->host)
						|| check_user_is_registered()
#ifdef CONFIG_SUPPORT_PON_LINK_DOWN_PROMPT
							&& getPONLinkState()
#endif
#ifdef COM_CUC_IGD1_AUTODIAG
							&& (!needAutoDiagRedirect())
#endif
#ifdef CONFIG_CU
						|| appscan_flag
#endif
					){
#ifdef CONFIG_CMCC
					//for CMCC DPI bundle webTesting local access
					{
						char *ipstr;
						if(ipstr = strstr(req->remote_ip_addr, lan_ip_str)) //pkt is from 192.168.1.1 to 192.168.1.1 on intf lo
						{
							if(strcmp(ipstr,lan_ip_str) == 0)
							{
								printf("[boa]auth_authorize, find access from system %s!\n", req->remote_ip_addr);
								req->buffer_end=0;
								send_r_request_ok(req);
								req_flush(req);
								return 0;
							}
						}
					}
#endif

#ifdef CONFIG_CU_BASEON_CMCC
					if(web_style_ln ==1)
						boaRedirectTemp(req, "/admin/login_yo_admin.asp");
					else
#endif
						boaRedirectTemp(req, "/admin/login.asp");
#ifdef CONFIG_CMCC_ENTERPRISE
						if(checkIsFromWifi(req) && !check_user_is_registered())
							boaRedirectTemp(req, "certification.asp");
#endif
					}
					else
					{
						//If LOID is not registered
						char url[256] = {0};
#ifdef CONFIG_YUEME
						snprintf(url, sizeof(url), "http://%s:8080/usereg.asp", lan_ip_str);
#else
#ifdef CONFIG_SUPPORT_PON_LINK_DOWN_PROMPT
						if(getPONLinkState() == 0)
							snprintf(url, sizeof(url), "http://%s/conn-fail.asp", lan_ip_str);
						else
#endif
						snprintf(url, sizeof(url), "http://%s/usereg.asp", lan_ip_str);
#ifdef CONFIG_CMCC_ENTERPRISE
						snprintf(url, sizeof(url), "http://%s/admin/login.asp", lan_ip_str);
						if(checkIsFromWifi(req) && !check_user_is_registered())
						{
							snprintf(url, sizeof(url), "http://%s/certification.asp", lan_ip_str);
						}
#endif
#endif
#ifdef COM_CUC_IGD1_AUTODIAG
						if(needAutoDiagRedirect()){
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
							char auto_redirect_url[MAX_URL_LEN] = {0};
							unsigned char urlredirect_enable=0;
							mib_get(MIB_AUTODIAG_REDIRECT_ENABLE, (void *)&urlredirect_enable);
							mib_get(MIB_AUTODIAG_REDIRECT_URL,auto_redirect_url);
							if(urlredirect_enable && auto_redirect_url[0] != '\0'){
								memset(url,0,sizeof(url));
								strncpy(url,auto_redirect_url,sizeof(auto_redirect_url));
							}
#endif
						}
#endif
						//printf("%s-%d url=%s\n",__func__,__LINE__,url);
						boaRedirectTemp(req, url);
					}
#else
				boaRedirectTemp(req, "/admin/login.asp");
#endif
#else
					if(strcmp(req->request_uri,"/")){
/*
						req->response_status = R_FORBIDDEN;
						req_write(req, "<HTML><HEAD><TITLE>Forbidden</TITLE></HEAD>\n <BODY><H1>Forbidden </H1>\n");
						req_write(req, " Your requested URL is forbidden to access, maybe because you have not logined.\n");
 						req_write(req, " Please close this window and reopen your web browser to login.\n</BODY></HTML>\n");
						req_flush(req);
*/
						req->buffer_end=0;
						req->response_status = R_MOVED_PERM;
//						req_write(req, "<HTML><HEAD><TITLE>Login</TITLE></HEAD>\n"
//									 "<BODY><BLOCKQUOTE>\n <h2><font color=\"#0000FF\">Login</font></h2>"
//									 "You have not logined,please\n");
#if !defined(CONFIG_00R0) && !defined(USE_DEONET)
						//boaWrite(req, "<HTML><HEAD><TITLE>Login</TITLE></HEAD>\n"
						//			 "<BODY><BLOCKQUOTE>\n <h2><font color=\"#0000FF\">%s</font></h2>"
						//			 "%s\n", multilang(LANG_LOGIN), multilang(LANG_RE_LOGIN_NOTICE));
						//req_write(req, "<A HREF=\"/admin/login.asp\" target=_top>login</A>.\n</BLOCKQUOTE></BODY></HTML>\n");
						boaRedirectTemp(req, "/admin/login.asp");
#else
						boaWrite(req, "HTTP/1.0 302 Moved Temporarily\r\n");
						boaWrite(req, "Content-Type: text/html; charset=UTF-8\r\n");
						boaWrite(req, "Set-Cookie: sessionid=0;  Max-Age=0; Path=/; \r\n");
						//boaWrite(req, "Connection: close\r\n");
						boaWrite(req, "\r\n");
						boaWrite(req, "<html><head>\r\n"
						 "<style>#cntdwn{ border-color: white;border-width: 0px;font-size: 12pt;color: red;text-align:center; font-weight:bold; font-family: Courier;}\r\n"
						 "body { background-repeat: no-repeat; background-attachment: fixed;	 background-position: center;	text-align:left;}\r\n"
						 "div.msg { font-size: 20px; top: 100px; width: 470px; text-align:center;}\r\n"
						 "#progress-boader{ border: 2px #ccc solid;border-radius: 10px;height: 25; top: 10px; width :420px; text-align:center ;left: 30px}\r\n"
						 "</style>\r\n"
						 "<script language=javascript>\r\n"
#ifdef CONFIG_TRUE
						 "window.open(\"/admin/login.asp\",target=\"_top\"); \r\n"
#else
						 "window.open(\"/admin/login.asp\",target=\"_top\"); \r\n"
#endif
						 "</script></head>\r\n"
						 "<body>");
						req_write(req, "</body></html>\n");
#endif
						req_flush(req);
						free_from_login_list_by_ip(req);
						return 0;
					}
					//boaRedirect(req, "/admin/login.asp");
					boaRedirectTemp(req,"/admin/login.asp"); //avoid caching
#endif
					return 0;
				}
#endif
		}
	  current = current->next;
  }

	return 1;
}

void dump_auth(void)
{
	int i;
	auth_dir *temp;

	for (i = 0; i < AUTH_HASHTABLE_SIZE; ++i) {
		if (auth_hashtable[i]) {
			temp = auth_hashtable[i];
			while (temp) {
				auth_dir *temp_next;

				if (temp->directory)
					free(temp->directory);
				if (temp->authfile)
					fclose(temp->authfile);
				temp_next = temp->next;
				free(temp);
				temp = temp_next;
			}
			auth_hashtable[i] = NULL;
		}
	}
}

#endif

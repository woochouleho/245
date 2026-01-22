/*
 *      Web server handler routines for management (password, save config, f/w update)
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *      Authors: Dick Tam	<dicktam@realtek.com.tw>
 *
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <crypt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <signal.h>
#include <stdint.h>
#include <linux/atm.h>
#include <linux/atmdev.h>
#include <libgen.h>
#include <sys/stat.h>
#include <setjmp.h>
#include <signal.h>

#include "../webs.h"
#include "../um.h"
#include "mib.h"
#include "webform.h"
#include "adsl_drv.h"
#include "utility.h"
#include "rtl_flashdrv.h"
#include <time.h>
#include <math.h>
#include "subr_net.h"

//xl_yue
#ifdef USE_LOGINWEB_OF_SERVER
#include <syslog.h>
#include "boa.h"
#endif

//ql_xu add
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

// Mason Yu
#ifdef EMBED
#include <linux/config.h>
#include <linux/sockios.h>	//cathy
#else
#include "../../../../include/linux/autoconf.h"
#endif
#ifdef CONFIG_LIB_LIBRTK_LED
#include "librtk_led/librtk_led.h"
#endif

//xl_yue
#ifdef ACCOUNT_LOGIN_CONTROL
#include <syslog.h>
#include "boa.h"
#endif
#include "../defs.h"
#include "multilang.h"
#include "multilang_set.h" // Added by davian_kuo.
#ifdef TIME_ZONE
#include "tz.h"
#endif

#ifdef CONFIG_SFU
#include <rtk/stat.h>
#endif

#ifdef CONFIG_00R0
#include "rtk/gpon.h"
#include "rtk/ponmac.h"
#ifdef CONFIG_COMMON_RT_API
#include <rtk/rt/rt_gpon.h>
#endif
#include <omci_api.h>
#endif
#ifdef CONFIG_DEV_xDSL
#include "subr_dsl.h"
#endif
#if defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_stat.h>
#include <rtk/rt/rt_port.h>
#endif
#include "syslog2d.h"
#include "libupg.h"
#include "../../sha256.h"

#define DNS_TIMEOUT 2 /* Set here the timeout for the gethostbyname call */
#define DNS_GOOGLE_SERVER  "8.8.4.4"
#define RTK_WAN_PORT	5

#define DEFAULT_GROUP		"administrators"
#define ACCESS_URL			"/"
#define _PATH_PROCNET_DEV	"/proc/net/dev"
#define MAX_DSL_TONE		512
#ifdef USE_LOGINWEB_OF_SERVER
#ifdef USE_CAPCTHA_OF_LOGINWEB
#define VERIFICATION_CODE_BASE		"0123456789abcdefghijkmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
// #define VERIFICATION_CODE_BASE		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@!#$%^&*"
#define CAPTCHA_LENGTH		6

#endif
#endif
static int srandomCalled = 0;
char g_rUserName[MAX_NAME_LEN];
char g_rUserPass[MAX_NAME_LEN];
bool_t g_remoteUpdate;

short *snr;
short *qln;
short *hlog;
static int diagflag=1;

//Added by Andrew
static unsigned char psd_bit_en;
static unsigned char psd_tone[8];
static float psd_us[8];
static char psd_measure;

// Added by Mason Yu
extern int g_filesize;
#ifdef ENABLE_SIGNATURE_ADV
extern int upgrade;
#endif
extern int g_upgrade_firmware;
#ifdef USE_LOGINWEB_OF_SERVER
// Mason Yu on True
unsigned char g_login_username[MAX_NAME_LEN];
#ifdef USE_CAPCTHA_OF_LOGINWEB
unsigned char g_captcha[MAX_NAME_LEN];
#endif
#endif

#ifdef CONFIG_USER_XMLCONFIG
extern const char *shell_name;
#endif

int replace_pw_page = 0;

#ifdef ACCOUNT_CONFIG
extern char suName[MAX_NAME_LEN];
extern char usName[MAX_NAME_LEN];
// Jenny, user account configuration
/////////////////////////////////////////////////////////////////////////////
int accountList(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;

	unsigned int entryNum, i;
	MIB_CE_ACCOUNT_CONFIG_T Entry;
	char	*priv;
	//char upasswd[MAX_NAME_LEN];

#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr><font size=2>"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=2>%s</td>\n"
	"<td align=center width=\"50%%\" bgcolor=\"#808080\"><font size=2>%s%s</td>\n"
	"<td align=center width=\"30%%\" bgcolor=\"#808080\"><font size=2>%s</td></font></tr>\n",
#else
	nBytesSent += boaWrite(wp, "<tr>"
	"<th align=center width=\"20%%\">%s</th>\n"
	"<th align=center width=\"50%%\">%s%s</th>\n"
	"<th align=center width=\"30%%\">%s</th></tr>\n",
#endif
	multilang(LANG_SELECT), multilang(LANG_USER), multilang(LANG_NAME_2), multilang(LANG_PRIVILEGE));

	/*if (!mib_get_s(MIB_SUSER_PASSWORD, (void *)upasswd, sizeof(upasswd))) {
		printf("ERROR: Get superuser password from MIB database failed.\n");
		return;
	}*/

	nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
	"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><input type=\"radio\" name=\"select\""
#else
	"<td align=center width=\"20%%\"><input type=\"radio\" name=\"select\""
#endif
//	" value=\"s0\" onClick=\"postEntry('%s', %d, '%s')\"></td>\n"),
//	suName, PRIV_ROOT, upasswd);
	" value=\"s0\" onClick=\"postEntry('%s', %d)\"></td>\n",
	suName, PRIV_ROOT);
	nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
	"<td align=center width=\"50%%\">%s</td>\n"
	"<td align=center width=\"30%%\">Admin</td></tr>\n",
#else
	"<td align=center width=\"50%%\">%s</td>\n"
	"<td align=center width=\"30%%\">Admin</td></tr>\n",
#endif
	suName);

	/*if (!mib_get_s(MIB_USER_PASSWORD, (void *)upasswd, sizeof(upasswd))) {
		printf("ERROR: Get user password from MIB database failed.\n");
		return;
	}*/
	nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
	"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><input type=\"radio\" name=\"select\""
#else
	"<td align=center width=\"20%%\"><input type=\"radio\" name=\"select\""
#endif
//	" value=\"s1\" onClick=\"postEntry('%s', %d, '%s')\"></td>\n"),
//	usName, PRIV_USER, upasswd);
	" value=\"s1\" onClick=\"postEntry('%s', %d)\"></td>\n",
	usName, PRIV_USER);
	nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
	"<td align=center width=\"50%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>\n"
	"<td align=center width=\"30%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>User</b></font></td></tr>\n",
#else
	"<td align=center width=\"50%%\">%s</td>\n"
	"<td align=center width=\"30%%\">User</td></tr>\n",
#endif
	usName);

	entryNum = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&Entry)) {
  			boaError(wp, 400, strGetChainerror);
			return -1;
		}

		priv = 0;
		if (Entry.privilege == PRIV_ROOT)
			priv = multilang(LANG_ADMIN);
		else if (Entry.privilege == PRIV_ENG)
			priv = multilang(LANG_SUPPORT);
		else if (Entry.privilege == PRIV_USER)
			priv = multilang(LANG_USER);

		nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><input type=\"radio\" name=\"select\""
#else
		"<td align=center width=\"20%%\"><input type=\"radio\" name=\"select\""
#endif
//		" value=\"s%d\" onClick=\"postEntry('%s', %d, '%s')\"></td>\n"),
//		i+2, Entry.userName, Entry.privilege, Entry.userPassword);
		" value=\"s%d\" onClick=\"postEntry('%s', %d)\"></td>\n",
		i+2, Entry.userName, Entry.privilege);
		nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
		"<td align=center width=\"50%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>\n"
		"<td align=center width=\"30%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td></tr>\n",
#else
		"<td align=center width=\"50%%\">%s</td>\n"
		"<td align=center width=\"30%%\">%s</td></tr>\n",
#endif
		Entry.userName, priv);
	}

	return nBytesSent;
}

void formAccountConfig(request * wp, char *path, char *query)
{
	char *str, *strUser, *submitUrl, *strOldPassword, *strPassword, *strConfPassword, *strPriv;
	MIB_CE_ACCOUNT_CONFIG_T entry, Entry;
	char tmpBuf[100];
	strUser = boaGetVar(wp, "username", "");
	strPriv = boaGetVar(wp, "privilege", "");
	strOldPassword = boaGetVar(wp, "oldpass", "");
	strPassword = boaGetVar(wp, "newpass", "");
	strConfPassword = boaGetVar(wp, "confpass", "");
	/* Retrieve next page URL */
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	int totalEntry, i, selected;

	// Delete
	str = boaGetVar(wp, "deluser", "");
	if (str[0]) {
		unsigned int i;
		unsigned int idx;
		totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL); /* get chain record size */
		str = boaGetVar(wp, "select", "");

		if (str[0]) {
			if (!strncmp(str, "s0", 2) || !strncmp(str, "s1", 2)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, multilang(LANG_SORRY_THE_ACCOUNT_CANNOT_BE_DELETED),sizeof(tmpBuf)-1);
				goto setErr_user;
			}
			for (i=0; i<totalEntry; i++) {
				idx = totalEntry - i + 1;
				snprintf(tmpBuf, 4, "s%d", idx);
				if (!gstrcmp(str, tmpBuf)) {
					/* get the specified chain record */
					if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, idx - 2, (void *)&Entry)) {
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, errGetEntry,sizeof(tmpBuf)-1);
						goto setErr_user;
					}
					// delete from chain record
					if(mib_chain_delete(MIB_ACCOUNT_CONFIG_TBL, idx - 2) != 1) {
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
						goto setErr_user;
					}
					goto setOk_user;
				}
			}
		}
		else {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_THERE_IS_NO_ITEM_SELECTED_TO_DELETE),sizeof(tmpBuf)-1);
			goto setErr_user;
		}
	}

	if (!strUser[0]) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, strUserNameempty,sizeof(tmpBuf)-1);
		goto setErr_user;
	}
	else {
		strncpy(entry.userName, strUser, MAX_NAME_LEN-1);
		entry.userName[MAX_NAME_LEN-1] = '\0';
		//entry.userName[MAX_NAME_LEN] = '\0';
	}

	if (!strPassword[0]) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_EMPTY_NEW_PASSWORD,sizeof(tmpBuf)-1);
		goto setErr_user;
	}
	else {
		strncpy(entry.userPassword, strPassword, MAX_PASSWD_LEN-1);
		entry.userPassword[MAX_PASSWD_LEN-1] = '\0';
		//entry.userPassword[MAX_NAME_LEN] = '\0';
	}

	if (!strConfPassword[0]) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_EMPTY_CONFIRMED_PASSWORD,sizeof(tmpBuf)-1);
		goto setErr_user;
	}

	if (strcmp(strPassword, strConfPassword) != 0 ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_UNMATCHED_PASSWORD,sizeof(tmpBuf)-1);
		goto setErr_user;
	}

	if (strPriv[0])
		entry.privilege = strPriv[0] - '0';

	totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL); /* get chain record size */
	// Add
	str = boaGetVar(wp, "adduser", "");
	if (str[0]) {
		int intVal;
		/* Check if user name exists */
		if (strcmp(suName, strUser) == 0 || strcmp(usName, strUser) == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_ERROR_USER_ALREADY_EXISTS),sizeof(tmpBuf)-1);
			goto setErr_user;
		}
		for (i=0; i<totalEntry; i++) {
			if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&Entry)) {
  				boaError(wp, 400, strGetChainerror);
				return;
			}

			if (strcmp(Entry.userName, strUser) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, multilang(LANG_ERROR_USER_ALREADY_EXISTS),sizeof(tmpBuf)-1);
				goto setErr_user;
			}
		}

		intVal = mib_chain_add(MIB_ACCOUNT_CONFIG_TBL, (unsigned char*)&entry);
		if (intVal == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strAddChainerror);
			goto setErr_user;
		}
		else if (intVal == -1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
			goto setErr_user;
		}
	}

	// Modify
	str = boaGetVar(wp, "modify", "");
	if (str[0]) {
		selected = -1;
		str = boaGetVar(wp, "select", "");
		if (str[0]) {
			for (i=0; i<totalEntry+2; i++) {
				snprintf(tmpBuf, 4, "s%d", i);
				if (!gstrcmp(str, tmpBuf)) {
					selected = i;
					break;
				}
			}
			if (selected >= 2) {
				if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, selected - 2, (void *)&Entry)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, strGetChainerror,sizeof(tmpBuf)-1);
					goto setErr_user;
				}
				if (strcmp(Entry.userPassword, strOldPassword) != 0) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, WARNING_WRONG_PASSWORD,sizeof(tmpBuf)-1);
					goto setErr_user;
				}
				mib_chain_update(MIB_ACCOUNT_CONFIG_TBL, (void *)&entry, selected - 2);
			}
			else if (selected == 0) {
				if (!mib_get_s(MIB_SUSER_PASSWORD, (void *)tmpBuf, sizeof(tmpBuf))) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, WARNING_GET_PASSWORD,sizeof(tmpBuf)-1);
					goto setErr_user;
				}
				if (strcmp(tmpBuf, strOldPassword) != 0) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, WARNING_WRONG_PASSWORD,sizeof(tmpBuf)-1);
					goto setErr_user;
				} else if (!mib_set(MIB_SUSER_PASSWORD, (void *)strPassword)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, WARNING_SET_PASSWORD,sizeof(tmpBuf)-1);
					goto setErr_user;
				}
				if (!mib_set(MIB_SUSER_NAME, (void *)strUser)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
					goto setErr_user;
				}
				mib_get_s(MIB_SUSER_NAME, (void *)suName, sizeof(suName));
			}
			else if (selected == 1) {
				if (!mib_get_s(MIB_USER_PASSWORD, (void *)tmpBuf, sizeof(tmpBuf))) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, WARNING_GET_PASSWORD,sizeof(tmpBuf)-1);
					goto setErr_user;
				}
				if (strcmp(tmpBuf, strOldPassword) != 0) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, WARNING_WRONG_PASSWORD,sizeof(tmpBuf)-1);
					goto setErr_user;
				} else if (!mib_set(MIB_USER_PASSWORD, (void *)strPassword)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, WARNING_SET_PASSWORD,sizeof(tmpBuf)-1);
					goto setErr_user;
				}
				if (!mib_set(MIB_USER_NAME, (void *)strUser)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
					goto setErr_user;
				}
				mib_get_s(MIB_USER_NAME, (void *)usName, sizeof(usName));
			}
		}
	}

setOk_user:
#ifdef EMBED
	// for take effect on real time
	mib_get_s(MIB_SUSER_NAME, (void *)suName, sizeof(suName));
	mib_get_s(MIB_USER_NAME, (void *)usName, sizeof(usName));
	rtk_util_update_boa_user_account();
	rtk_util_update_user_account();	// Jenny
#endif


	OK_MSG(submitUrl);
	return;

setErr_user:
	OK_MSG1(tmpBuf, submitUrl);
}
#endif

/////////////////////////////////////////////////////////////////////////////
void formPasswordSetup(request * wp, char *path, char *query)
{
	char *str, *submitUrl, *strPassword, *strOldPassword, *strConfPassword;
	char tmpBuf[100];
	char userName[MAX_NAME_LEN];
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
	char realm[MAX_REALM_LEN]={0};
#endif
	str = boaGetVar(wp, "userMode", "");
	//strUser = boaGetVar(wp, "username", "");
	strOldPassword = boaGetVar(wp, "oldpass", "");
	strPassword = boaGetVar(wp, "newpass", "");
	strConfPassword = boaGetVar(wp, "confpass", "");

	if ( !strOldPassword[0] ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_EMPTY_OLD_PASSWORD,sizeof(tmpBuf)-1);
		goto setErr_pass;
	}

	if ( !strPassword[0] ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_EMPTY_NEW_PASSWORD,sizeof(sizeof(tmpBuf)-1));
		goto setErr_pass;
	}

	if ( !strConfPassword[0] ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_EMPTY_CONFIRMED_PASSWORD,sizeof(tmpBuf)-1);
		goto setErr_pass;
	}

	if (strcmp(strPassword, strConfPassword) != 0 ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_UNMATCHED_PASSWORD,sizeof(tmpBuf)-1);
		goto setErr_pass;
	}

	if (str[0]) {
		if ( str[0] == '0' ) {       // superuser ==> cht
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
			if ( !mib_get_s(MIB_WEB_SUSER_PASSWORD, (void *)tmpBuf, sizeof(tmpBuf))) {
#else
			if ( !mib_get_s(MIB_SUSER_PASSWORD, (void *)tmpBuf, sizeof(tmpBuf))) {
#endif
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, WARNING_GET_PASSWORD,sizeof(tmpBuf)-1);
				goto setErr_pass;
			}

			if ( strcmp(tmpBuf, strOldPassword) != 0 ) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, WARNING_WRONG_PASSWORD,sizeof(tmpBuf)-1);
				goto setErr_pass;
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
			}else if ( !mib_set(MIB_WEB_SUSER_PASSWORD, (void *)strPassword) ) {
#else
			}else if ( !mib_set(MIB_SUSER_PASSWORD, (void *)strPassword) ) {
#endif
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, WARNING_SET_PASSWORD,sizeof(tmpBuf)-1);
				goto setErr_pass;
			}
		}
		else if ( str[0] == '1' ) {  // normal user ==> user
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
			if ( !mib_get_s(MIB_WEB_USER_PASSWORD, (void *)tmpBuf, sizeof(tmpBuf))) {
#else
			if ( !mib_get_s(MIB_USER_PASSWORD, (void *)tmpBuf, sizeof(tmpBuf))) {
#endif
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, WARNING_GET_PASSWORD,sizeof(tmpBuf)-1);
				goto setErr_pass;
			}

			if ( strcmp(tmpBuf, strOldPassword) != 0 ) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, WARNING_WRONG_PASSWORD,sizeof(tmpBuf)-1);
				goto setErr_pass;
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
			}else if ( !mib_set(MIB_WEB_USER_PASSWORD, (void *)strPassword) ) {
#else
			}else if ( !mib_set(MIB_USER_PASSWORD, (void *)strPassword) ) {
#endif
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, WARNING_SET_PASSWORD,sizeof(tmpBuf)-1);
				goto setErr_pass;
			}
		}
		else {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, WARNING_WRONG_USER,sizeof(tmpBuf)-1);
			goto setErr_pass;
		}
	}

#ifdef EMBED
	// Added by Mason Yu for take effect on real time
	rtk_util_update_boa_user_account();
	rtk_util_update_user_account();	// Jenny
#endif

	/*
	if (mib_update(HW_SETTING) == 0) {
		printf("Warning : Commit hs fail(formPasswordSetup()) !\n");
	}
	*/

// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#ifdef USE_LOGINWEB_OF_SERVER
	if(str[0] == '0')
		mib_get_s(MIB_SUSER_NAME, (void *)userName, sizeof(userName));
	else
		mib_get_s(MIB_USER_NAME, (void *)userName, sizeof(userName));

	syslog(LOG_INFO, "Change password successful for %s change %s password via Web\n", g_login_username, userName);
	strcpy(tmpBuf, "Change password successfully! Please login with new password.");
	PASSWORD_MODIFY_SUCCESS_MSG_REDIRCET_LOGIN(tmpBuf);

	if(str[0] == '0')
	{
		if (!free_from_login_list(wp)) {
			syslog(LOG_ERR, "logout error from %s\n", wp->remote_ip_addr);
			printf("logout error\n");
		} else {
			syslog(LOG_CRIT, "logout successful from %s\n",
					wp->remote_ip_addr);
			printf("logout\n");
		}

		/* if changing success, goto main page */
		//boaRedirect(wp, "/");
	}
	else
#endif
	{
		/* Init user management */
		// Commented By Mason Yu
		//set_user_profile();

		/* when change password, go to main page */
		//boaRedirect(wp, "/");

		/* Retrieve next page URL */
		/*
		submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
		OK_MSG(submitUrl);
		*/
	}

	syslog2(LOG_DM_HTTP, LOG_DM_HTTP_PASSWD_CHANGE, "Set Web UI Password Configured (Success)");

	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG(submitUrl);
	return;

setErr_pass:
	syslog2(LOG_DM_HTTP, LOG_DM_HTTP_PASSWD_CHANGE, "Set Web UI Password Configured (Fail)");
	ERR_MSG(tmpBuf);
}

int ShowNatSessionCount(int eid, request * wp, int argc, char **argv)
{
	boaWrite(wp, "<tr><th>%s :</th>\n", multilang(LANG_NATSESSION_COUNT));
	boaWrite(wp, "<td><input type='text' name='natsessioncount' value=\"%d\" size='20' maxlength='30' readonly></td></tr>\n", get_ConntrackCnt());

	boaWrite(wp, "<tr><th>%s :</th>\n", multilang(LANG_NATSESSION_HW_COUNT));
	boaWrite(wp, "<td><input type='text' name='natsessionhwcount' value=\"%d\" size='20' maxlength='30' readonly></td></tr>\n", get_ConntrackHwCnt());

	return 0;
}

int ShowNatSessionThreshold(int eid, request * wp, int argc, char **argv)
{
	unsigned int threshold;

	if (!mib_get_s(MIB_NAT_SESSION_THRESHOLD, (void *)&threshold, 4))
		return -1;

	boaWrite(wp, "<tr><th>%s :</th>\n", multilang(LANG_NATSESSION_THRESHOLD));
	boaWrite(wp, "<td><input type='text' name='natsessionthreshold' value=\"%d\" size='20' maxlength='30'></td></tr>\n", threshold);

	return 0;
}

void formNatSessionSetup(request * wp, char *path, char *query)
{
	char *strVal, *submitUrl;
	char tmpBuf[100];
	unsigned int threshold;

	strVal = boaGetVar(wp, "natsessionreset", "");
	if (strVal[0])
	{
		va_niced_cmd("/bin/conntrack", 1, 0, "-F");

		threshold = 8000;
		if (!mib_set(MIB_NAT_SESSION_THRESHOLD, (void *)&threshold))
		{
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_pass;
		}
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
		goto setOk_pass;
	}

	strVal = boaGetVar(wp, "nat_refresh", "");
	if (strVal[0])
		goto setOk_refresh;

	strVal = boaGetVar(wp, "apply", "");
	if (strVal[0])
	{
		strVal = boaGetVar(wp, "natsessionthreshold", "");
		if (strVal && strVal[0] )
			threshold = atoi(strVal);

		if (!mib_set(MIB_NAT_SESSION_THRESHOLD, (void *)&threshold))
		{
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_pass;

		}
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
	}

setOk_refresh:
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
	{
		boaWrite(wp, "<script>window.location.href = '%s';</script>", submitUrl);
		return;
	}

setOk_pass:
	/* Retrieve next page URL */
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	OK_MSG(submitUrl);
	return;

setErr_pass:
	ERR_MSG(tmpBuf);
	return;
}

int ShowUseageThreshold(int eid, request * wp, int argc, char **argv)
{
	int cputh, memth, flashth;

	if (!mib_get_s(MIB_SYSMON_CPU_USEAGE_THRESHOLD, (void *)&cputh, 4))
		return -1;

	if (!mib_get_s(MIB_SYSMON_MEM_USEAGE_THRESHOLD, (void *)&memth, 4))
		return -1;

	if (!mib_get_s(MIB_SYSMON_FLASH_USEAGE_THRESHOLD, (void *)&flashth, 4))
		return -1;

	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<th>%s :</th>\n", multilang(LANG_CPU_USEAGE_THRESHOLD));
	boaWrite(wp, "<td><input type='text' name='cpuusagethreshold' value=\"%d\" size='20' maxlength='30'></td>\n", cputh);
	boaWrite(wp, "</tr>\n");

	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<th>%s :</th>\n", multilang(LANG_MEM_USEAGE_THRESHOLD));
	boaWrite(wp, "<td><input type='text' name='memusagethreshold' value=\"%d\" size='20' maxlength='30'></td>\n", memth);
	boaWrite(wp, "</tr>\n");

	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<th>%s :</th>\n", multilang(LANG_FLASH_USEAGE_THRESHOLD));
	boaWrite(wp, "<td><input type='text' name='flashusagethreshold' value=\"%d\" size='20' maxlength='30'></td>\n", flashth);
	boaWrite(wp, "</tr>\n");

	return 0;
}

void formUseageSetup(request * wp, char *path, char *query)
{
	char *strVal, *submitUrl;
	char tmpBuf[100];
	int threshold;

	strVal = boaGetVar(wp, "apply", "");
	if (strVal[0])
	{
		strVal = boaGetVar(wp, "cpuusagethreshold", "");
		if (strVal && strVal[0] )
			threshold = atoi(strVal);

		if (!mib_set(MIB_SYSMON_CPU_USEAGE_THRESHOLD, (void *)&threshold))
		{
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_pass;

		}

		strVal = boaGetVar(wp, "memusagethreshold", "");
		if (strVal && strVal[0] )
			threshold = atoi(strVal);

		if (!mib_set(MIB_SYSMON_MEM_USEAGE_THRESHOLD, (void *)&threshold))
		{
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_pass;
		}

		strVal = boaGetVar(wp, "flashusagethreshold", "");
		if (strVal && strVal[0] )
			threshold = atoi(strVal);

		if (!mib_set(MIB_SYSMON_FLASH_USEAGE_THRESHOLD, (void *)&threshold))
		{
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_pass;
		}

		Commit();
	}

	/* Retrieve next page URL */
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	OK_MSG(submitUrl);
	return;

setErr_pass:
	ERR_MSG(tmpBuf);
	return;
}

// Added by Mason Yu for 2 level web page
/////////////////////////////////////////////////////////////////////////////
void formUserPasswordSetup(request * wp, char *path, char *query)
{
	char *str, *submitUrl, *strPassword, *strOldPassword, *strConfPassword;
	char tmpBuf[100];
	char userName[MAX_NAME_LEN];
#ifdef ACCOUNT_CONFIG
	MIB_CE_ACCOUNT_CONFIG_T Entry;
	int totalEntry, i, selected = -1;
#endif
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
		char realm[MAX_REALM_LEN]={0};
#endif

	//strUser = boaGetVar(wp, "username", "");
	strOldPassword = boaGetVar(wp, "oldpass", "");
	strPassword = boaGetVar(wp, "newpass", "");
	strConfPassword = boaGetVar(wp, "confpass", "");

	if ( !strOldPassword[0] ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, multilang(LANG_WARNING_EMPTY_OLD_PASSWORD),sizeof(tmpBuf)-1);
		goto setErr_pass;
	}

	if ( !strPassword[0] ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, multilang(LANG_WARNING_EMPTY_NEW_PASSWORD),sizeof(tmpBuf)-1);
		goto setErr_pass;
	}

	if ( !strConfPassword[0] ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, multilang(LANG_WARNING_EMPTY_CONFIRMED_PASSWORD),sizeof(tmpBuf)-1);
		goto setErr_pass;
	}

	if (strcmp(strPassword, strConfPassword) != 0 ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, multilang(LANG_WARNING_UNMATCHED_PASSWORD),sizeof(tmpBuf)-1);
		goto setErr_pass;
	}


#ifdef ACCOUNT_CONFIG
	totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL);
	for (i=0; i<totalEntry; i++) {
		if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&Entry))
			continue;
		if (Entry.privilege == (unsigned char)PRIV_ROOT)
			continue;
		#ifdef USE_LOGINWEB_OF_SERVER
		if(!strcmp(g_login_username, Entry.userName))
		#else
		if (strcmp(wp->user, Entry.userName) == 0)
		#endif
		{
			selected = i;
			break;
		}
	}
	if (selected != -1) {
		if (strcmp(Entry.userPassword, strOldPassword) != 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, WARNING_WRONG_PASSWORD,sizeof(tmpBuf)-1);
			goto setErr_pass;
		} else {
			strncpy(Entry.userPassword, strPassword, MAX_PASSWD_LEN-1);
			Entry.userPassword[MAX_PASSWD_LEN-1] = '\0';
			//Entry.userPassword[MAX_NAME_LEN] = '\0';
		}
		Entry.privilege = (unsigned char)getAccPriv(Entry.userName);
		mib_chain_update(MIB_ACCOUNT_CONFIG_TBL, (void *)&Entry, selected);
	}
	else {
#endif
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
	if ( !mib_get_s(MIB_WEB_USER_PASSWORD, (void *)tmpBuf, sizeof(tmpBuf))) {
#else
	if ( !mib_get_s(MIB_USER_PASSWORD, (void *)tmpBuf, sizeof(tmpBuf))) {
#endif
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, Tget_mib_error,sizeof(tmpBuf)-1);
		goto setErr_pass;
	}

	if ( strcmp(tmpBuf, strOldPassword) != 0 ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, multilang(LANG_ERROR_INPUT_OLD_USER_PASSWORD_ERROR),sizeof(tmpBuf)-1);
		goto setErr_pass;
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
	}else if ( !mib_set(MIB_WEB_USER_PASSWORD, (void *)strPassword) ) {
#else
	}else if ( !mib_set(MIB_USER_PASSWORD, (void *)strPassword) ) {
#endif
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
		goto setErr_pass;
	}
#ifdef ACCOUNT_CONFIG
	}
#endif

#ifdef EMBED
	// Added by Mason Yu for take effect on real time
	rtk_util_update_boa_user_account();
	rtk_util_update_user_account();	// Jenny
#endif
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#ifdef USE_LOGINWEB_OF_SERVER
	mib_get_s(MIB_USER_NAME, (void *)userName, sizeof(userName));

	syslog(LOG_INFO, "Change password successful for %s change %s password via Web\n", g_login_username, userName);

	if (!free_from_login_list(wp)) {
		syslog(LOG_ERR, "logout error from %s\n", wp->remote_ip_addr);
		printf("logout error\n");
	} else {
		syslog(LOG_CRIT, "logout successful from %s\n",
				wp->remote_ip_addr);
		printf("logout\n");
	}
	strcpy(tmpBuf, "Change setting successfully! Plese login with new setting.");
	PASSWORD_MODIFY_SUCCESS_MSG_REDIRCET_LOGIN(tmpBuf);

	syslog2(LOG_DM_HTTP, LOG_DM_HTTP_PASSWD_CHANGE, "Set Web UI Password Configured (Success)");
#else

	/* Retrieve next page URL */
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page

	OK_MSG(submitUrl);
#endif
	return;

setErr_pass:
	syslog2(LOG_DM_HTTP, LOG_DM_HTTP_PASSWD_CHANGE, "Set Web UI Password Configured (Fail)");
	ERR_MSG(tmpBuf);
}

////////////////////////////////////////////////////////////////////
void set_user_profile(void)
{
	char superName[MAX_NAME_LEN], superPass[MAX_NAME_LEN];
	char userName[MAX_NAME_LEN], userPass[MAX_PASSWD_LEN];
	char dport[10];
//	char *user, *nextUser, *group;

	/* first time load, get mib */
	if ( !mib_get_s( MIB_SUSER_NAME, (void *)superName, sizeof(superName) ) ||
		!mib_get_s( MIB_SUSER_PASSWORD, (void *)superPass, sizeof(superPass) ) ||
			!mib_get_s( MIB_USER_NAME, (void *)userName, sizeof(userName) ) ||
				!mib_get_s( MIB_USER_PASSWORD, (void *)userPass, sizeof(userPass) ) ) {
		error(E_L, E_LOG, "Get user account MIB failed");
		return;
	}

	/* Delete all user account belonging to DEFAULT_GROUP */
	/*
	user = umGetFirstUser();
	while (user) {
//		printf("boaDeleteUser (user=%s).\n", user);
		nextUser = umGetNextUser(user);
		group = umGetUserGroup(user);
		if (gstrcmp(DEFAULT_GROUP, group) == 0) {
			if ( boaDeleteUser(user) ) {
				printf("ERROR: Unable to delete user account (user=%s).\n", user);
				return;
			}
		}

		user = nextUser;
		//user = umGetFirstUser();
	}
	*/

	boaDeleteAccessLimit(ACCESS_URL);
	boaDeleteGroup(DEFAULT_GROUP);

	if ( userName[0] ) {
		/* Create supervisor */
		if ( !boaGroupExists(DEFAULT_GROUP) )
			if ( boaAddGroup(DEFAULT_GROUP, (short)PRIV_ADMIN, (void*)(intptr_t)AM_BASIC, FALSE, FALSE) ) {
				error(E_L, E_LOG, "ERROR: Unable to add group.");
				return;
			}
		if ( !boaAccessLimitExists(ACCESS_URL) ) {
			if ( boaAddAccessLimit(ACCESS_URL, (void*)(intptr_t)AM_FULL, (short)0, DEFAULT_GROUP) ) {
				error(E_L, E_LOG, "ERROR: Unable to add access limit.");
				return;
			}
		}

		/* Create user */
		if ( boaAddUser(superName, superPass, DEFAULT_GROUP, FALSE, FALSE) ) {
			error(E_L, E_LOG, "ERROR: Unable to add supervisor account.");
			return;
		}

		/* Create user */
		if ( boaAddUser(userName, userPass, DEFAULT_GROUP, FALSE, FALSE) ) {
			error(E_L, E_LOG, "ERROR: Unable to add user account.");
			return;
		}
	}
	else {
		if (g_remoteConfig) {	// remote config not allowed
			char ipaddr[20], tmpStr[5];
			int len;

			if (g_rUserName[0]) {
				if ( boaDeleteUser(g_rUserName) ) {
					printf("ERROR: Unable to delete user account (user=%s).\n", g_rUserName);
					return;
				}
				g_rUserName[0] = '\0';
			}

			mib_get_s(MIB_ADSL_LAN_IP, (void *)tmpStr, sizeof(tmpStr));
			strncpy(ipaddr, inet_ntoa(*((struct in_addr *)tmpStr)), 16);
			ipaddr[15] = '\0';
			len = sizeof(ipaddr) - strlen(ipaddr);
			if(snprintf(ipaddr + strlen(ipaddr), len, ":80") >= len) {
				printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
			}
			sprintf(dport, "%d", g_remoteAccessPort);
			// iptables -D INPUT -i ! br0 -p TCP --dport 80 -j ACCEPT
			va_cmd(IPTABLES, 11, 1, (char *)FW_DEL, (char *)FW_INPUT, "!", ARG_I, LANIF, "-p", ARG_TCP, FW_DPORT, "80", "-j", (char *)FW_ACCEPT);
			// iptables -t nat -D PREROUTING -i ! $LAN_IF -p TCP --dport 51003 -j DNAT --to-destination ipaddr:80
			va_cmd(IPTABLES, 15, 1, "-t", "nat", (char *)FW_DEL,	"PREROUTING", "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP, (char *)FW_DPORT,
			dport, "-j", "DNAT", "--to-destination", ipaddr);
			g_remoteConfig = 0;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// search token szKey from string szString
// if find, return its value, else return null
#if 0
char* SearchKeyValue(char* szString, char* szKey)
{
	char *szDuplicate;
	char *key, *lp, *cp, *value;

	//duplicate the string, avoid the original string to be modefied
	szDuplicate = strdup(szString);

	for (lp = szDuplicate ; lp && *lp; )
	{
		cp = lp;
		if ((lp = gstrchr(lp, ';')) != NULL)
		{
			lp++;
		}

		if ((key = gstrtok(cp, "= \t;")) == NULL)
		{
			continue;
		}

		if ((value = gstrtok(NULL, ";")) == NULL)
		{
			value = "";
		}

		while (gisspace(*value))
		{
			value++;
		}

		if(strcmp(key, szKey) == 0)
		{
			return value;
		}
	}

	return NULL;
}
#endif
///////////////////////////////////////////////////////////////////////////////
// find key szKey form string szString
// start from nStart to nEnd of szString
// if found, return the first index of the matched string
// if not found, return -1
static int FindKeyIndex(char *szKey, char * szString, int nStart, int nEnd)
{
	int nKeyLen = strlen(szKey);
	char *szSearch = szString + nStart;
	char *szSearchEnd = szString + nEnd;
	int nIndex = 0;
	bool bFind = false;
	while(szSearch != szSearchEnd)
	{
		if(memcmp(szSearch, szKey, nKeyLen) ==0)
		{
			bFind = true;
			break;
		}
		else
		{
			nIndex++;
			szSearch++;
		}
	}

	if(bFind == true)
		return (nIndex + nStart);
	else
		return -1;

}

/*
 * Strip head and tail of http file to form a real content file.
 */
static int strip_http(const char *fname, unsigned int offset, unsigned int nLen)
{
	FILE *src_fp, *dst_fp;
	char buf[64];
	size_t count, ret;
	unsigned int len;

	src_fp=fopen(fname, "rb");
	if(src_fp==NULL)
		return 0;
	if(fseek(src_fp, offset, SEEK_SET) != 0)
		printf("fseek failed: %s %d\n", __func__, __LINE__);
	dst_fp=fopen(fname, "r+b");
	if(dst_fp==NULL) {
		fclose(src_fp);
		return 0;
	}

	len = nLen;
	// Move data content to head of file
	while (len > 0) {
		count = (sizeof(buf) < len) ? sizeof(buf) : len;
		ret = fread(buf, 1, count, src_fp);

		count = ret;
		ret = fwrite(buf, 1, count, dst_fp);

		len -= ret;
	}

	fclose(src_fp);
	fclose(dst_fp);
	// shrink the size of file to content size
	truncate(fname, nLen);
	return 1;
}

#ifdef CONFIG_LUNA_FIRMWARE_UPGRADE_SUPPORT
#ifdef FWU_VERIFY_CHECKSUM
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/sha.h>

#define FWU_SIG_SIZE 32
#define  FWU_BUF_SIZE 65536

static int rtk_fwu_verify_checksum(const char *fname, size_t size)
{
	FILE *f_img = NULL;
	unsigned char *buffer = NULL;
	int bytesRead = 0;
	int rc = -1;
	int result = -1;
	unsigned char sig[FWU_SIG_SIZE] = {0};
	unsigned char checksum[FWU_SIG_SIZE] = {0};
	unsigned int checksum_size = 0;

	EVP_MD_CTX *ctx = NULL;

	if(fname == NULL)
		return -1;

	OpenSSL_add_all_digests();

	f_img = fopen(fname, "r");
	if(f_img == NULL)
	{
		fprintf(stderr, "Open %s failed\n", fname);
		return -1;
	}

	// tail FWU_SIG_SIZE bytes is signature
	size -= FWU_SIG_SIZE;

	buffer = malloc(FWU_BUF_SIZE);
	if(!buffer)
	{
		if(f_img) fclose(f_img);
		return ENOMEM;
	}

	ctx = EVP_MD_CTX_create();
	if(ctx == NULL) {
		printf("EVP_MD_CTX_create failed, error 0x%lx\n", ERR_get_error());
		goto verify_failed;
	}

	rc = EVP_DigestInit(ctx, EVP_sha256());
	if(rc != 1) {
		printf("EVP_DigestVerifyInit failed, error 0x%lx\n", ERR_get_error());
		goto verify_failed;
	}

	printf("file size is %lu\n", size);
	do
	{
		bytesRead = fread(buffer, 1, (FWU_BUF_SIZE < size) ? FWU_BUF_SIZE : size, f_img);
		rc = EVP_DigestVerifyUpdate(ctx, buffer, bytesRead);
		if(rc != 1) {
			printf("EVP_DigestVerifyUpdate failed, error 0x%lx\n", ERR_get_error());
			goto verify_failed;
		}
		size -= bytesRead;
	}while(size > 0);

	bytesRead = fread(sig, 1, FWU_SIG_SIZE, f_img);
	if(bytesRead != FWU_SIG_SIZE)
	{
		printf("<%s:%d> Read signature failed\n", __func__, __LINE__);
		goto verify_failed;
	}

	rc = EVP_DigestFinal(ctx, checksum, &checksum_size);
	if(rc != 1) {
		printf("EVP_DigestVerifyFinal failed, error 0x%lx\n", ERR_get_error());
		goto verify_failed;
	}

	fprintf(stderr, "[Koala] Checksum size=%d\n", checksum_size);

	if(memcmp(checksum, sig, FWU_SIG_SIZE) != 0)
	{
		printf("Incorrect checksum.\n");
		goto verify_failed;
	}
	result = 0;

verify_failed:
	if(ctx) {
		EVP_MD_CTX_destroy(ctx);
		ctx = NULL;
	}
	EVP_cleanup();
	if(buffer)
		free(buffer);
	if(f_img)
		fclose(f_img);
	return result;
}
#endif

#ifdef FWU_VERIFY_HWVER
static int rtk_fwu_verify_hwver(const char *fname)
{
	FILE *file = NULL;
	char buf[256]="";
	char hwver[32]="";
	ssize_t len = 0;

	snprintf(buf, sizeof(buf), "/bin/tar tOf %s hw_ver", fname);
	file = popen(buf, "r");
	if(file == NULL)
		return -1;

	mib_get_s(MIB_HW_HWVER, hwver, sizeof(hwver));
	fgets(buf, sizeof(buf), file);

	len = strlen(buf);
	if(len <=0)
		return -1;

	if(buf[len-1] == '\n')
		buf[len-1] = '\0';

	printf("HW_HWVER=%s, hw_ver=%s....", hwver, buf);
	if(strcmp(buf, "skip") == 0 || strcmp(buf, hwver) == 0)
	{
		printf("OK\n");
		return 0;
	}

	printf("NOK\n");
	return -1;
}
#endif

static int isValidImageFile(const char *fname)
{
	int ret;
	char buf[256];
	__sighandler_t save_alarm;

	// todo: validate the image file
	snprintf(buf, sizeof(buf), "/bin/tar tf %s md5.txt > /dev/null", fname);
	save_alarm = signal(SIGALRM, SIG_IGN); //ignore SIGALRM to prevent interrupted system call
	ret = system(buf);
	signal(SIGALRM, save_alarm);

	return !ret;
}
#else
static int isValidImageFile(const char *fname) {
	IMGHDR imgHdr;
	unsigned int csum;
	int size, remain, nRead, block;
	unsigned char buf[64];
	FILE *fp=NULL;
#ifdef CONFIG_RTL8686
	int err=-1;
#endif
#ifdef ENABLE_SIGNATURE
	SIGHDR sigHdr;
	unsigned int hdrChksum;
	int i;
#endif

	fp=fopen(fname, "rb");
	if(fp==NULL)
		goto ERROR1;
#if defined(ENABLE_SIGNATURE)
	//ql_xu add: check if the img signature is right
	memset(&sigHdr, 0, sizeof(SIGHDR));
	if (1 != fread(&sigHdr, sizeof(sigHdr), 1, fp)) {
		printf("failed to read signature header\n");
		goto ERROR1;
	}
#endif
	if (1!=fread(&imgHdr, sizeof(imgHdr), 1, fp)) {
		printf("Failed to read header\n");
		goto ERROR1;
	}
#ifndef ENABLE_SIGNATURE_ADV
#ifdef ENABLE_SIGNATURE
	printf("sig len: %d\n", sigHdr.sigLen);
	if (sigHdr.sigLen > SIG_LEN) {
		printf("signature length error\n");
		goto ERROR1;
	}
	for (i=0; i<sigHdr.sigLen; i++)
		sigHdr.sigStr[i] = sigHdr.sigStr[i] - 10;
	if (strcmp(sigHdr.sigStr, SIGNATURE)) {
		printf("signature error\n");
		goto ERROR1;
	}

	hdrChksum = sigHdr.chksum;
	hdrChksum = ipchksum(&imgHdr, sizeof(imgHdr), hdrChksum);
	if (hdrChksum) {
		printf("Checksum failed(fmmgmt isValidImageFile), size=%d, csum=%04xh\n", sigHdr.sigLen, hdrChksum);
		goto ERROR1;
	}
#endif
#endif

#ifdef CONFIG_RTL8686
	switch(imgHdr.key){
		case APPLICATION_UBOOT:
		case APPLICATION_UIMAGE:
		case APPLICATION_ROOTFS:
			printf("%s-%d, got header::%x\n",__func__,__LINE__,imgHdr.key);
			err = 0;
			break;
		default:
			printf("%s-%d, Unknown header::%x\n",__func__,__LINE__,imgHdr.key);
			err = 1;
			break;
	}
	if(err)
		goto ERROR1;
#else
	if (imgHdr.key != APPLICATION_IMAGE) {
		printf("Unknown header\n");
		goto ERROR1;
	}
#endif

	csum = imgHdr.chksum;
	size = imgHdr.length;
	remain = size;

	while (remain > 0) {
		block = (remain > sizeof(buf)) ? sizeof(buf) : remain;
		nRead = fread(buf, 1, block, fp);
		if (nRead <= 0) {
			printf("read too short (remain=%d, block=%d)\n", remain, block);
			goto ERROR1;
		}
		remain -= nRead;
		csum = ipchksum(buf, nRead,csum);
	}

	if (csum) {
		printf("Checksum failed(fmmgmt isValidImageFIle2), size=%d, csum=%04xh\n", size, csum);
		goto ERROR1;
	}
	fclose(fp);
	return 1;
ERROR1:
	if(fp!=NULL)
		fclose(fp);
	return 0;
}
#endif

// find the start and end of the upload file.
FILE * _uploadGet(request *wp, unsigned int *startPos, unsigned *endPos) {

	FILE *fp=NULL;
	struct stat statbuf;
	unsigned char c, *buf;
	char boundary[80]={0};

	if (wp->method == M_POST)
	{
		int i=0;

		fstat(wp->post_data_fd, &statbuf);
		lseek(wp->post_data_fd, 0, SEEK_SET);

		//printf("file size=%d\n",statbuf.st_size);
		fp=fopen(wp->post_file_name,"rb");
		if(fp==NULL) goto error;

		memset( boundary, 0, sizeof( boundary ) );
		if( fgets( boundary,80,fp )==NULL ) goto error;
		if( boundary[0]!='-' || boundary[1]!='-')
		{
			goto error;
		}

		i= strlen( boundary ) - 1;
		while( boundary[i]=='\r' || boundary[i]=='\n' )
		{
			boundary[i]='\0';
			i--;
		}
		printf( "boundary=%s\n", boundary );
	}
	else goto error;

	do
	{
		if(feof(fp))
		{
			printf("Cannot find start of file\n");
			goto error;
		}
		c= fgetc(fp);
		if (c!=0xd)
			continue;
		c= fgetc(fp);
		if (c!=0xa)
			continue;
		c= fgetc(fp);
		if (c!=0xd)
			continue;
		c= fgetc(fp);
		if (c!=0xa)
			continue;
		break;
	}while(1);
	(*startPos)=ftell(fp);

	do
	{
		if(feof(fp))
		{
			printf("Cannot find the end of the file!\n");
			goto error;
		}
		c= fgetc(fp);
		if (c!=0xd)
			continue;
		c= fgetc(fp);
		if (c!=0xa)
			continue;
		{
			int i, blen;

			blen= strlen( boundary );
			for( i=0; i<blen; i++)
			{
				c= fgetc(fp);
				//printf("%c(%u)", c, c);
				if (c!=boundary[i])
				{
					ungetc( c, fp );
					break;
				}
			}
			//printf("\r\n");
			if( i!=blen ) continue;
		}

		break;
	}while(1);
	(*endPos)=ftell(fp)-strlen(boundary)-2;

	return fp;
error:
	if(fp)
		fclose(fp);
	return NULL;
}

#ifdef WEB_UPGRADE
// Added by Mason Yu
void displayUploadMessage(request * wp, int status)
{
	//printf("Popout web page\n");
	boaHeader(wp);
	boaWrite(wp, "<body><blockquote><h4>\n");
	switch (status) {
#if 0
		case ERROR_FILESIZE:
			boaWrite(wp, "(file size exceeded)");
			break;
		case ERROR_FORMAT:
			boaWrite(wp, "(file format error)");
			break;
#endif
		case ERROR_NO:
			boaWrite(wp, multilang(LANG_UPGRADE_SUCCESS));
			break;
		case ERROR_ALREADY:
			boaWrite(wp, "(이미 업그레이드가 진행 )");
			break;
		case ERROR_INVALID:
		default:
			boaWrite(wp, multilang(LANG_UPDATE_FIRMWARE_FAILED));
			boaWrite(wp, "(잘못된 파일)");
			break;
	}
	boaWrite(wp, "</h4>\n");

	boaWrite(wp, "<button onclick=window.location.replace(\"%s\")>이전</button>", "/admin/upgrade.asp");

#ifndef FWU_VERIFY_CHECKSUM
	//MSG_COUNTDOWN(multilang(LANG_REBOOT_WORD0), 10 /*REBOOT_TIME*/, NULL);
	//boaWrite(wp, "%s\n", multilang(LANG_REBOOT_WORD2));
#endif
	boaWrite(wp, "</blockquote></body>");
	boaFooter(wp);
	boaDone(wp, 200);
}

#ifdef UPGRADE_V1
///////////////////////////////////////////////////////////////////////////////
static int gIsReboot = 0, gIsCommit = 0;
extern int lockUpgrade(void);
extern void unlockUpgrade(void);
void formUpload(request * wp, char *path, char *query)
{
	unsigned int startPos, endPos, nLen;
	char tmpBuf[100] /*, *submitUrl */ ;
	FILE *fp = NULL;
	char *strVal;
    int isCommit = gIsCommit, isReboot = gIsReboot;

	gIsCommit = 0;
	gIsReboot = 0;
	/* find the start and end positive of run time image */
	tmpBuf[0] = '\0';
	//printf("\nTry to get file size of new firmware\n");

#ifdef CONFIG_LUNA_FIRMWARE_UPGRADE_SUPPORT
	/* Toggle the LED for firmupgarde */

#ifdef CONFIG_00R0 //change the internet LED to status LED indicator, trigger LED in msgparser.c
	system("/bin/echo 4 > /proc/internet_flag");
#else
#ifdef CONFIG_LIB_LIBRTK_LED
	rtk_setLed(LED_PWR_G, RTK_LED_OFF, NULL);
	rtk_setLed(LED_PWR_R, RTK_LED_ON, NULL);
#else
	system("echo 3 > /proc/power_flag");
#endif	/* CONFIG_LIB_LIBRTK_LED */
#endif
#endif

#ifdef ENABLE_SIGNATURE_ADV
	if (upgrade != 2) {	//signature Err
		displayUploadMessage(wp, ERROR_INVALID);
		goto end;
	}
#endif

	if (g_filesize >= g_max_upload_size) {
		displayUploadMessage(wp, ERROR_FILESIZE);
		goto end;
	}

	if ((fp = _uploadGet(wp, &startPos, &endPos)) == NULL) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, FILEOPENFAILED,sizeof(tmpBuf)-1);
		//fclose(fp);
		goto fail;
	}
	fclose(fp);

	/* check header and checksum of this image */
	printf("endPos=%u startPos=%u\n", endPos, startPos);
	nLen = endPos - startPos;

#ifdef EMBED
	// write to flash
	{
		int writeflashtime;
		int ret;
		unsigned int	int_version;
		char			activeBank, commitBank;

		strip_http(wp->post_file_name, startPos, nLen);

		syslog2(LOG_DM_HTTP, LOG_DM_HTTP_START, "Upgrade Start by WEB");

		if(lockUpgrade() < 0)
		{
			displayUploadMessage(wp, ERROR_ALREADY);
			syslog2(LOG_DM_HTTP, LOG_DM_HTTP_FAIL, "FW Upgrade Fail (reason : upgradeLock)");
			goto end;
		}

		activeBank = get_active_info();
		if(activeBank < 0 )
		{
			displayUploadMessage(wp, ERROR_INVALID);
			syslog2(LOG_DM_HTTP, LOG_DM_HTTP_FAIL, "FW Upgrade Fail (reason : GetActive)");
			unlockUpgrade();
			goto end;
		}

		commitBank = get_commit_info();
		if(commitBank < 0 )
		{
			displayUploadMessage(wp, ERROR_INVALID);
			syslog2(LOG_DM_HTTP, LOG_DM_HTTP_FAIL, "FW Upgrade Fail (reason : GetCommit)");
			unlockUpgrade();
			goto end;
		}

		if(activeBank != commitBank)
		{
			displayUploadMessage(wp, ERROR_INVALID);
			syslog2(LOG_DM_HTTP, LOG_DM_HTTP_FAIL, "FW Upgrade Fail (reason : ActiveChange)");
			unlockUpgrade();
			goto end;
		}

		/* header 제거, decryption */
		printf("===========================\n");
		printf("pathname %s\n", wp->pathname);
		printf("Parse %s\n", wp->post_file_name);
		extern int parse_img(char * file_name, unsigned int *nLen, unsigned int *int_version);
		if((ret = parse_img(wp->post_file_name, &nLen, &int_version)) != DEO_LIBUPG_ERR_NONE)
		{
			printf("Incorrect image version\n");
			displayUploadMessage(wp, ERROR_INVALID);
			if(ret == DEO_LIBUPG_VER_MISMATCH)
				syslog2(LOG_DM_HTTP, LOG_DM_HTTP_FAIL, "FW Upgrade Fail (reason : VersionMismatch)");
			else if(ret == DEO_LIBUPG_CRC_MISMATCH)
				syslog2(LOG_DM_HTTP, LOG_DM_HTTP_FAIL, "FW Upgrade Fail (reason : CRCMismatch)");
			else if(ret == DEO_LIBUPG_INVALID_IMG)
				syslog2(LOG_DM_HTTP, LOG_DM_HTTP_FAIL, "FW Upgrade Fail (reason : InvalidImage)");
			else
				syslog2(LOG_DM_HTTP, LOG_DM_HTTP_FAIL, "FW Upgrade Fail (reason : ParseImage)");
			unlockUpgrade();
			goto end;
		}
		printf("===========================\n");

		if (!isValidImageFile(wp->post_file_name)) {
			printf("Incorrect image file\n");
			displayUploadMessage(wp, ERROR_FORMAT);
			syslog2(LOG_DM_HTTP, LOG_DM_HTTP_FAIL, "FW Upgrade Fail (reason : InvalidImage)");
			unlockUpgrade();
			goto end;
		}

#ifdef FWU_VERIFY_CHECKSUM
		if (rtk_fwu_verify_checksum(wp->post_file_name, nLen) != 0) {
			printf("Incorrect FW checksum.\n");
			displayUploadMessage(wp, ERROR_INVALID);
			syslog2(LOG_DM_HTTP, LOG_DM_HTTP_FAIL, "FW Upgrade Fail (reason : InvalidImage)");
			unlockUpgrade();
			goto end;
		}
#endif

#ifdef FWU_VERIFY_HWVER
		if (rtk_fwu_verify_hwver(wp->post_file_name) != 0) {
			printf("Incorrect image hardware version.\n");
			displayUploadMessage(wp, ERROR_INVALID);
			syslog2(LOG_DM_HTTP, LOG_DM_HTTP_FAIL, "FW Upgrade Fail (reason : InvalidImage)");
			unlockUpgrade();
			goto end;
		}
#endif

		// Save file for upgrade Firmware
		g_upgrade_firmware = TRUE;
#ifdef USE_DEONET
		cmd_upload(wp->post_file_name, 0, nLen, isReboot, isCommit, int_version);
#else
		cmd_upload(wp->post_file_name, 0, nLen);
#endif

#ifdef CONFIG_DEV_xDSL
		Modem_LinkSpeed vLs;
		vLs.upstreamRate = 0;

		if (adsl_drv_get
		    (RLCM_GET_LINK_SPEED, (void *)&vLs,
		     RLCM_GET_LINK_SPEED_SIZE) && vLs.upstreamRate != 0)
			writeflashtime = g_filesize / 17400 / 3;	//star: flash can wirte about 17k in 1 sec with the adsl line up
		else
#endif
#ifdef CONFIG_LUNA_G3_SERIES
			writeflashtime = g_filesize / 100000 / 3;	//star: flash can wirte about 21k in 1 sec
#else
			writeflashtime = g_filesize / 60000 / 3;	//star: flash can wirte about 21k in 1 sec
#endif
#ifdef CONFIG_LUNA_FIRMWARE_UPGRADE_SUPPORT
		writeflashtime += 10;
#ifdef CONFIG_00R0
#ifdef CONFIG_MTD_LUNA_NOR_SPI //the writeflashtime is match, but need add the rebooting time
		writeflashtime += 20;
#ifdef WLAN_SUPPORT
		writeflashtime += 35;
#endif
#endif
#endif //CONFIG_00R0
#endif

#ifdef CONFIG_00R0
#ifdef CONFIG_MTD_NAND
		writeflashtime = 90;
#endif
#endif //CONFIG_00R0
		printf("writeflashtime=%d\n",writeflashtime);
#if 0
		unlockUpgrade();
#endif
		// Added by Mason Yu
		if(isReboot)
		{
			boaHeader(wp);
			//--- Add timer countdown
			MSG_COUNTDOWN(multilang(LANG_FIRMWARE_UPGRADING_PLEASE_WAIT), REBOOT_TIME, NULL);
			//--- End of timer countdown
			boaWrite(wp, "<body><blockquote><h4>\n");
			boaWrite(wp, "%s<br><br>\n", multilang(LANG_REBOOT_WORD1));
			boaWrite(wp, "%s\n", multilang(LANG_REBOOT_WORD2));
			boaWrite(wp, "</blockquote></body>");
			boaFooter(wp);
			boaDone(wp, 200);
		}
		else
		{
			unlockUpgrade();
			displayUploadMessage(wp, ERROR_NO);
			goto end;
		}
	}
#if defined(CONFIG_00R0)
	rtk_env_set("sw_updater", "web"); //Change the string to tr069 accordingly
#endif

	return;
#endif


fail:
#ifdef CONFIG_LUNA_FIRMWARE_UPGRADE_SUPPORT
	/* Toggle the LED for firmupgarde */

#ifdef CONFIG_00R0 //change the internet LED to status LED indicator, trigger LED in msgparser.c
	system("/bin/echo 1 > /proc/internet_flag");
#else
	system("echo 1 > /proc/power_flag");
#endif
#endif

	OK_MSG1(tmpBuf, "/upgrade.asp");
	return;
end:
#ifdef CONFIG_LUNA_FIRMWARE_UPGRADE_SUPPORT
#ifdef CONFIG_00R0 //change the internet LED to status LED indicator, trigger LED in msgparser.c
	system("/bin/echo 1 > /proc/internet_flag");
#else
	system("echo 1 > /proc/power_flag");
#endif
#endif

#if defined(EMBED) && !defined(FWU_VERIFY_CHECKSUM)
#ifndef USE_DEONET
	syslog2(LOG_DM_HTTP, LOG_DM_HTTP_RESET, "Reboot by Web (Upgrade)");
	cmd_reboot();
#endif
#endif

	return;
}
#endif // of UPGRADE_V1
#ifdef CONFIG_DOUBLE_IMAGE
void formStopUpload(request * wp, char * path, char * query)
{
	formReboot(wp, path, query);
#ifdef USE_DEONET
	cmd_upload(NULL, 0, 0, 1, 0);  //stop fw_upload
#else
	cmd_upload(NULL, 0, 0);  //stop fw_upload
#endif
}
#endif
#endif // of WEB_UPGRADE

///////////////////////////////////////////////////////////////////////////////
/*
 *	Tag: load, Value: Upload - upload configuration file
 *	Tag: save, Value: Save... - save configuration file
 *	Tag: reset, Value: Rest - reset configuration to default
 */

void formSaveConfig(request * wp, char *path, char *query)
{
	char *strRequest;
	const char *config_filename;
	char tmpBuf[100], *submitUrl;
#ifdef CONFIG_00R0
	int isencrypted = 0;
#endif

	CONFIG_DATA_T action_type = UNKNOWN_SETTING;

   	tmpBuf[0] = '\0';

	if (g_filesize > MIN_UPLOAD_FILESIZE) {
		boaHeader(wp);
		boaWrite(wp, "<META HTTP-EQUIV=Refresh CONTENT=\"60; URL=/status.asp\">\n");
		boaWrite(wp, "<body><blockquote><h4>\n");
		boaWrite(wp, "%s<br>",multilang(LANG_RESTORE_FAIL));
		boaWrite(wp, "%s</h4>\n", multilang(LANG_REBOOT_WORD0));
		boaWrite(wp, "<br>%s\n", multilang(LANG_REBOOT_WORD2));
		boaWrite(wp, "</blockquote></body>");
		boaFooter(wp);
		boaDone(wp, 200);
		goto fail_without_open_file;
	}
	else if (g_filesize >= MAX_CONFIG_FILESIZE) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, multilang(LANG_ERROR_RESTORE_CONFIG_FILE_FAILED_UPLOADED_FILE_SIZE_OUT_OF_CONSTRAINT),sizeof(tmpBuf)-1);
		goto fail_without_open_file;
	}

#if defined(CONFIG_USER_XMLCONFIG) || defined(CONFIG_USE_XML)
	config_filename = CONFIG_XMLFILE;
#else
	config_filename = CONFIG_RAWFILE;
#endif

	strRequest = boaGetVar(wp, "save_cs", "");
	if (strRequest[0])
	{
		action_type = CURRENT_SETTING;
	}
#ifdef CONFIG_00R0
	//XOR_ENCRYPT will be enabled, so default opteration is encrypted
	strRequest = boaGetVar(wp, "save_cs_encrypt", "");
	if (strRequest[0])
	{
		isencrypted = 1;
		action_type = CURRENT_SETTING;
	}
#endif

	strRequest = boaGetVar(wp, "save_hs", "");
	if (strRequest[0])
	{
		action_type = HW_SETTING;
	}

	/* Backup Settings to File */
	if (action_type == CURRENT_SETTING) {	// save configuration file
		unsigned int fileSize, fileSector, maxFileSector;
		unsigned char *buf;
		FILE *fp;
		struct stat st;
		size_t nRead;
		char *basec, *bname;

#ifdef CONFIG_00R0
		unlink("/tmp/skip_config");
#ifdef CONFIG_USER_CONF_ON_XMLFILE
		const char* skipconf[]={
			"IP_QOS_TBL",
			"IP_QOS_TC_TBL",
			"IP_QOS_QUEUE_TBL",
			"QOS_POLICY",
			"QOS_MODE",
			"ENABLE_QOS",
			"CWMP_ACS_URL",
			"CWMP_ACS_URL_OLD",
			"CWMP_ACS_USERNAME",
			"CWMP_ACS_PASSWORD",
			"CWMP_INFORM_ENABLE",
			"CWMP_INFORM_INTERVAL",
			"CWMP_CONREQ_PASSWORD",
			"CWMP_CONREQ_USERNAME",
			"CWMP_CONREQ_PATH",
			"CWMP_CONREQ_PORT",
			"CWMP_CERT_PASSWORD",
			"CWMP_ACL_ENABLE",
			"CWMP_WAN_INTERFACE",
			"CWMP_ACL_IP_TBL",
			"CWMP_FLAG",
			"CWMP_FLAG2",
			"VOIP_TBL",
			NULL,
		};

		int i = 0;
		if(isencrypted==0){
			fp = fopen("/tmp/skip_config", "w");
			if(fp != NULL) {
				while(skipconf[i] != NULL){
					fprintf(fp, "%s\n",skipconf[i]);
					i++;
				}
				fclose(fp);
			}
		}
#endif
#endif

		before_upload(config_filename);
#ifdef CONFIG_00R0
		//decrypted config file for decrypted config button
		if(isencrypted==0){
#ifdef XOR_ENCRYPT
			char tempFile[32]={0};
			xor_encrypt((char*)config_filename, tempFile);
			rename(tempFile, config_filename);
#endif
			unlink("/tmp/skip_config");
		}
#endif

		wp->buffer_end=0; // clear header
		boaWrite(wp, "HTTP/1.0 200 OK\n");
		boaWrite(wp, "Content-Type: application/octet-stream;\n");

		//boaWrite(wp, "Content-Disposition: attachment;filename=\"%s\" \n", basename((char *)config_filename));
		basec = strdup(config_filename);
		bname = basename(basec);
		boaWrite(wp, "Content-Disposition: attachment;filename=\"%s\" \n", bname);
		free(basec);
#ifdef 	SERVER_SSL
		// IE bug, we can't sent file with no-cache through https
#else
		boaWrite(wp, "Pragma: no-cache\n");
		boaWrite(wp, "Cache-Control: no-cache\n");
#endif
		boaWrite(wp, "\n");

		if (stat(config_filename, &st)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_FILEOPENFAILED),sizeof(tmpBuf)-1);
			goto fail_without_reboot;
		}
		fileSize = st.st_size;

		fp = fopen(config_filename, "r");
		if (fp == NULL) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_FILEOPENFAILED),sizeof(tmpBuf)-1);
			goto fail_without_reboot;
		}

		maxFileSector = 0x1000;
		buf = malloc(maxFileSector);
		if (buf == NULL) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_ALLOCATE_BUFFER_FAILED),sizeof(tmpBuf)-1);
			fclose(fp);
			goto fail_without_reboot;
		}
		while (fileSize > 0) {
			fileSector = (fileSize > maxFileSector) ? maxFileSector : fileSize;
			nRead = fread(buf, 1, fileSector, fp);
			boaWriteDataNonBlock(wp, buf, nRead);

			fileSize -= fileSector;
		}
		free(buf);
		fclose(fp);
		unlink(config_filename);

		return;
	}

	/* Reset Settings to Default */
	strRequest = boaGetVar(wp, "reset", "");
	if (strRequest[0]) {		// reset configuration to default

		boaHeader(wp);
		//--- Add timer countdown
		MSG_COUNTDOWN(multilang(LANG_REBOOT_WORD0), REBOOT_TIME, NULL);
		//--- End of timer countdown
		boaWrite(wp, "<body><blockquote><h4>\n");

		boaWrite(wp, "%s<br><br>\n", multilang(LANG_REBOOT_WORD1));
		boaWrite(wp, "%s\n", multilang(LANG_REBOOT_WORD2));
		boaWrite(wp, "</blockquote></body>");
		boaFooter(wp);
		boaDone(wp, 200);

#ifdef EMBED
		syslog2(LOG_DM_HTTP, LOG_DM_HTTP_RESET, "Reboot by Web (Factory reset)");
		//Mason Yu,  LED flash while factory reset
		system("echo 2 > /proc/load_default");
		reset_cs_to_default(1);
		cmd_reboot();
#endif
		return;
	}

	/* Restore Settings from File */
	{
		FILE *fp = NULL;
		unsigned char *buf;
		unsigned int startPos, endPos, nLen, nRead;
		int ret = -1;
		CONFIG_DATA_T dtype;

		if ((fp = _uploadGet(wp, &startPos, &endPos)) == NULL) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_ERROR_FIND_THE_START_AND_END_OF_THE_UPLOAD_FILE_FAILED),sizeof(tmpBuf)-1);
			goto fail;
		}

		/* check header and checksum of this image */
		nLen = endPos - startPos;
		printf("Config file size is %d\n", nLen);
		buf = malloc(nLen);
		if (!buf) {
			fclose(fp);
			goto fail;
		}

		if(fseek(fp, startPos, SEEK_SET) != 0)
			printf("fseek failed: %s %d\n", __func__, __LINE__);
		nRead = fread((void *)buf, 1, nLen, fp);
		fclose(fp);
		if (nRead != nLen)
			printf("Read %d bytes, expect %d bytes\n", nRead, nLen);

		fp = fopen(config_filename, "w");
		if (!fp) {
			printf("Get config file fail!\n");
			free(buf);
			goto fail;
		}
		fwrite((void *)buf, 1, nLen, fp);
		printf("create file %s\n", config_filename);
		free(buf);
		fclose(fp);

		ret = after_download(config_filename);

		if (ret == 0) {
			boaHeader(wp);
			MSG_COUNTDOWN(multilang(LANG_REBOOT_WORD0), REBOOT_TIME, NULL);
			boaWrite(wp, "<body><blockquote><h4>\n");
			boaWrite(wp, "%s\n<br>",multilang(LANG_RESTORE_SUCCESS));
			boaWrite(wp, "%s<br><br>\n", multilang(LANG_REBOOT_WORD1));
			boaWrite(wp, "%s\n", multilang(LANG_REBOOT_WORD2));
			boaWrite(wp, "</blockquote></body>");
			boaFooter(wp);
			boaDone(wp, 200);
		} else {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_ERROR_RESTORE_CONFIG_FILE_FAILED_INVALID_CONFIG_FILE),sizeof(tmpBuf)-1);
			goto fail_without_reboot;
		}
	}

fail:
	cmd_reboot();

fail_without_reboot:
	unlink(config_filename);
fail_without_open_file:
	OK_MSG1(tmpBuf, "/admin/saveconf.asp");
}

void formImportOMCIShell(request * wp, char *path, char *query)
{
	char *strRequest;
	unsigned int maxFileSector;
	char tmpBuf[100], *submitUrl;
	struct stat statbuf;

	wp->buffer_end=0; // clear header
   	tmpBuf[0] = '\0';
	FILE	*fp=NULL,*fp_input;
	unsigned char *buf;
	int c;
	unsigned int endPos,nLen,nRead;
	int startPos;
	int ret=-1;

	if (wp->method == M_POST)
	{
		if(fstat(wp->post_data_fd, &statbuf) != 0)
			printf("fstat failed: %s %d\n", __func__, __LINE__);
		lseek(wp->post_data_fd, 0, SEEK_SET);
		fp=fopen(wp->post_file_name,"rb");
		if(fp==NULL) goto fail;
	}
	else goto fail;

	do
	{
		if(feof(fp))
		{
			printf("Cannot find start of file\n");
			goto fail;
		}
		c= fgetc(fp);
		if (c!=0xd) continue;
		c= fgetc(fp);
		if (c!=0xa) continue;
		c= fgetc(fp);
		if (c!=0xd) continue;
		c= fgetc(fp);
		if (c!=0xa) continue;
		break;
	}while(1);
	startPos=ftell(fp);
	if(fseek(fp,statbuf.st_size-0x100,SEEK_SET)<0)
		goto fail;
	do
	{
		if(feof(fp))
		{
			printf("Cannot find the end of the file!\n");
			goto fail;
		}
		c= fgetc(fp);
		if (c!='-') continue;
		c= fgetc(fp);
		if (c!='-') continue;
		c= fgetc(fp);
		if (c!='-') continue;
		c= fgetc(fp);
		if (c!='-') continue;
		break;
	}while(1);
	endPos=ftell(fp);
	endPos -= 6;  // Magician

	nLen = endPos - startPos;
	buf = malloc(nLen);
	if (!buf)
	{
		fclose(fp);
		goto end;
	}

	if(startPos != -1L) {
		if(fseek(fp, startPos, SEEK_SET) != 0)
			printf("fseek failed: %s %d\n", __func__, __LINE__);
	}
	nRead = fread((void *)buf, 1, nLen, fp);
	fclose(fp);
	if (nRead != nLen)
		printf("Read %d bytes, expect %d bytes\n", nRead, nLen);

	fp_input=fopen("/tmp/omcishell","w");
	if (!fp_input)
		printf("Get config file fail!\n");
	else {
		fwrite((void *)buf, 1, nLen, fp_input);
		printf("create file omcishell\n");
		fclose(fp_input);
	}
	free(buf);
	system("/bin/sh /tmp/omcishell");
	tmpBuf[sizeof(tmpBuf)-1]='\0';
	strncpy(tmpBuf, multilang(LANG_OK),sizeof(tmpBuf)-1);
	OK_MSG1(tmpBuf, "/vermod.asp");
	return;
fail:
	if(fp!=NULL)
		fclose(fp);
	OK_MSG1(tmpBuf, "/vermod.asp");
end:

 	return;
}


void formExportOMCIlog(request * wp, char *path, char *query)
{
	char *strRequest;

	char tmpBuf[100], *submitUrl;
	PARAM_HEADER_T header;
	unsigned int maxFileSector = 524288;
	FILE *fp;

	//unsigned char *ptr;
	unsigned int fileSize,filelen;
	unsigned int fileSector;
	unsigned char *buf;
	int ret;


	wp->buffer_end=0; // clear header
   	tmpBuf[0] = '\0';

	system("/bin/diag gpon deactivate");
	sleep(1);
	system("/bin/omcicli mib_reset");
	sleep(1);
	system("/bin/omcicli set logfile 3 ffffffff");
	system("/bin/diag gpon activate init-state o1");

	ret = sleep(60);
	do
	{
		ret = sleep(ret);
	}while(ret > 0);

	system("/bin/omcicli set logfile 0");
	system("/bin/tar -cf /tmp/omcilog.tar /tmp/omcilog /tmp/omcilog.par");

	ret=-1;

	boaWrite(wp, "HTTP/1.0 200 OK\n");
	boaWrite(wp, "Content-Type: application/octet-stream;\n");

	boaWrite(wp, "Content-Disposition: attachment;filename=\"omcilog.tar\" \n");
#ifdef 	SERVER_SSL
	// IE bug, we can't sent file with no-cache through https
#else
	boaWrite(wp, "Pragma: no-cache\n");
	boaWrite(wp, "Cache-Control: no-cache\n");
#endif
	boaWrite(wp, "\n");


	fp=fopen("/tmp/omcilog.tar","r");
	if(fp==NULL)
		printf("Error opening  file /tmp/omcilog.tar \n");
	//decide the file size
	fseek(fp, 0, SEEK_END);
	filelen = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fileSize=filelen;
	buf = malloc(0x1000);
	if ( buf == NULL ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, multilang(LANG_ALLOCATE_BUFFER_FAILED),sizeof(tmpBuf)-1);
		fclose(fp);
		return;
	}
	while(fileSize>0)
	{
		int nRead;
		fileSector = (fileSize>maxFileSector)?maxFileSector:fileSize;
		nRead = fread((void *)buf, 1, fileSector, fp);
		boaWriteDataNonBlock(wp, buf, nRead);

		fileSize -= fileSector;
		//ptr += fileSector;
	}

	free(buf);
	fclose(fp);
 	return;
}



#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP)
void formSnmpConfig(request * wp, char *path, char *query)
{
	char *str, *submitUrl;
	struct in_addr trap_ip;
	static char tmpBuf[100];

	//star: for take effect
	unsigned int snmpchangeflag=0;
	unsigned char vChar;
	char origstr[256];
	unsigned char origip[16];
	unsigned char snmpVal, oid_snmpVal;

	// Enable/Disable SNMPD
	str = boaGetVar(wp, "save", "");
	if (str[0]) {
		str = boaGetVar(wp, "snmp_enable", "");
		if (str[0]) {
			if (str[0] == '0')
				snmpVal = 0;
			else
				snmpVal = 1;

			mib_get_s(MIB_SNMPD_ENABLE, (void *)&oid_snmpVal, sizeof(oid_snmpVal))
				;
			if ( oid_snmpVal != snmpVal ) {
				snmpchangeflag = 1;
			}

			if ( !mib_set(MIB_SNMPD_ENABLE, (void *)&snmpVal)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto setErr_pass;
			}
		}
	}
	str = boaGetVar(wp, "snmpSysDescr", "");
	if (str[0]) {
		mib_get_s(MIB_SNMP_SYS_DESCR, (void*)origstr, sizeof(origstr));
		if(strcmp(origstr,str)!=0)
			snmpchangeflag = 1;
		if ( !mib_set(MIB_SNMP_SYS_DESCR, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_pass;
		}
	}

	str = boaGetVar(wp, "snmpSysContact", "");
	if (str[0]) {
		mib_get_s(MIB_SNMP_SYS_CONTACT, (void*)origstr, sizeof(origstr));
		if(strcmp(origstr,str)!=0)
			snmpchangeflag = 1;
		if ( !mib_set(MIB_SNMP_SYS_CONTACT, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_pass;
		}
	}

	str = boaGetVar(wp, "snmpSysName", "");
	if (str[0]) {
		mib_get_s(MIB_SNMP_SYS_NAME, (void*)origstr, sizeof(origstr));
		if(strcmp(origstr,str)!=0)
			snmpchangeflag = 1;
		if ( !mib_set(MIB_SNMP_SYS_NAME, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_pass;
		}
	}

	str = boaGetVar(wp, "snmpSysLocation", "");
	if (str[0]) {
		mib_get_s(MIB_SNMP_SYS_LOCATION, (void*)origstr, sizeof(origstr));
		if(strcmp(origstr,str)!=0)
			snmpchangeflag = 1;

		if ( !mib_set(MIB_SNMP_SYS_LOCATION, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_pass;
		}
	}

	str = boaGetVar(wp, "snmpSysObjectID", "");
	if (str[0]) {
		mib_get_s(MIB_SNMP_SYS_OID, (void*)origstr, sizeof(origstr));
		if(strcmp(origstr,str)!=0)
			snmpchangeflag = 1;
		if ( !mib_set(MIB_SNMP_SYS_OID, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_pass;
		}
	}

	str = boaGetVar(wp, "snmpCommunityRO", "");
	if (str[0]) {
		mib_get_s(MIB_SNMP_COMM_RO_ENC, (void*)origstr, sizeof(origstr));

		if(strcmp(origstr,str)!=0)
			snmpchangeflag = 1;
		if ( !mib_set(MIB_SNMP_COMM_RO_ENC, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetcommunityROerror,sizeof(tmpBuf)-1);
			goto setErr_pass;
		}
	}
	str = boaGetVar(wp, "snmpCommunityRW", "");
	if (str[0]) {
		mib_get_s(MIB_SNMP_COMM_RW_ENC, (void*)origstr, sizeof(origstr));
		if(strcmp(origstr,str)!=0)
			snmpchangeflag = 1;
		if ( !mib_set(MIB_SNMP_COMM_RW_ENC, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetcommunityRWerror,sizeof(tmpBuf)-1);
			goto setErr_pass;
		}
	}
	str = boaGetVar(wp, "snmpTrapIpAddr", "");
	if ( str[0] ) {
		if ( !inet_aton(str, &trap_ip) ) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strInvalTrapIp,sizeof(tmpBuf)-1);
			goto setErr_pass;
		}
		mib_get_s(MIB_SNMP_TRAP_IP, (void*)&origip, sizeof(origip));
		if(((struct in_addr*)origip)->s_addr != trap_ip.s_addr)
			snmpchangeflag = 1;
		if ( !mib_set(MIB_SNMP_TRAP_IP, (void *)&trap_ip)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetTrapIperror,sizeof(tmpBuf)-1);
			goto setErr_pass;
		}
	}

	/* upgdate to flash */
	//  mib_update(CURRENT_SETTING);
	if(snmpchangeflag == 1)
	{
		// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif

		//      RECONNECT_MSG(strIp);
		//      req_flush(wp);
#if defined(APPLY_CHANGE)
		if ( snmpVal == 1 ) // on
			restart_snmp(1);
		else
			restart_snmp(0);  // off
#endif
		submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
		OK_MSG(submitUrl);
		return;
	}
	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;

setErr_pass:
	ERR_MSG(tmpBuf);
}
#elif defined(CONFIG_USER_SNMPD_SNMPD_V3)
void formSnmpConfig(request * wp, char *path, char *query)
{
	char *str, *submitUrl;
	struct in_addr trap_ip;
	static char tmpBuf[100];

//star: for take effect
	unsigned int snmpchangeflag=0;
	unsigned char vChar;
	char origstr[256];
	unsigned char origip[16];
	unsigned char snmpVal, snmpEnbl, oid_snmpVal;
	unsigned char trapOn = 1;

	// Enable/Disable SNMPD
	str = boaGetVar(wp, "save", "");
	if (str[0]) {
		str = boaGetVar(wp, "snmp_enable", "");
		if (str[0]) {
			if (str[0] == '0')
				snmpEnbl = 0;
			else
				snmpEnbl = 1;

			mib_get_s(MIB_SNMPD_ENABLE, (void *)&oid_snmpVal, sizeof(oid_snmpVal));
			if ( oid_snmpVal != snmpEnbl ) {
				snmpchangeflag = 1;
			}

			if ( !mib_set(MIB_SNMPD_ENABLE, (void *)&snmpEnbl)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto setErr_pass;
			}
		}
		str = boaGetVar(wp, "snmpCommunityRO_enable", "");
		if (str[0]) {
			if (str[0] == '0')
				snmpVal = 0;
			else
				snmpVal = 1;

			mib_get_s(MIB_SNMP_COMM_RO_ON, (void *)&oid_snmpVal, sizeof(oid_snmpVal));
			if ( oid_snmpVal != snmpVal ) {
				snmpchangeflag = 1;
			}

			if ( !mib_set(MIB_SNMP_COMM_RO_ON, (void *)&snmpVal)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto setErr_pass;
			}
		}
		str = boaGetVar(wp, "snmpCommunityRW_enable", "");
		if (str[0]) {
			if (str[0] == '0')
				snmpVal = 0;
			else
				snmpVal = 1;

			mib_get_s(MIB_SNMP_COMM_RW_ON, (void *)&oid_snmpVal, sizeof(oid_snmpVal));
			if ( oid_snmpVal != snmpVal ) {
				snmpchangeflag = 1;
			}

			if ( !mib_set(MIB_SNMP_COMM_RW_ON, (void *)&snmpVal)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto setErr_pass;
			}
		}


		str = boaGetVar(wp, "snmpTrapIpAddr", "");
		if (str[0]) {
			mib_get_s(MIB_SNMP_TRAP_IP, (void*)origstr, sizeof(origstr));
			if(strcmp(origstr,str)!=0)
				snmpchangeflag = 1;
			if ( !mib_set(MIB_SNMP_TRAP_IP, (void *)str)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto setErr_pass;
			}

			trapOn = 1;
			if ( !mib_set(MIB_SNMP_COMM_TRAP_ON, (void *)&trapOn)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto setErr_pass;
			}
		}
		else
		{
			if ( !mib_set(MIB_SNMP_TRAP_IP, (void *)"")) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto setErr_pass;
			}

			trapOn = 0;
			if ( !mib_set(MIB_SNMP_COMM_TRAP_ON, (void *)&trapOn)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto setErr_pass;
			}
		}

	}
	/* upgdate to flash */
//	mib_update(CURRENT_SETTING);
	if(snmpchangeflag == 1)
	{
// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif

//		RECONNECT_MSG(strIp);
//		req_flush(wp);
#if defined(APPLY_CHANGE)
		if ( snmpEnbl == 1 )	// on
			restart_snmp(1);
		else
			restart_snmp(0);  // off
#endif
		submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
		OK_MSG(submitUrl);
		return;
	}

	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
  	return;

 setErr_pass:
	ERR_MSG(tmpBuf);
}

int initSnmpConfig(int eid, request * wp, int argc, char **argv)
{
	int nBytes = 0;
	return nBytes;
}
#endif


#ifdef CONFIG_DEV_xDSL
void formSetAdslTone(request * wp, char *path, char *query)
{
	char *submitUrl;
	short mode;
	int i, chan;
	char tmpBuf[100];
	char *strVal;
	unsigned char tone[64];	  // Added by Mason Yu for correct Tone Mib Type
	char *strApply, *strMaskAll, *strUnmaskAll;

	memset(tone, 0, sizeof(tone));

	strApply = boaGetVar(wp, "apply", "");
	strMaskAll = boaGetVar(wp, "maskAll", "");
	strUnmaskAll = boaGetVar(wp, "unmaskAll", "");


	// get the channel number
	mib_get_s(MIB_ADSL_MODE, (void *)&mode, sizeof(mode));
	if (mode & ADSL_MODE_ADSL2P)
		chan = 512;	// ADSL2+
	else
		chan = 256;	// ADSL, ADSL2

	if (strApply[0]) {
		for (i=0; i<chan; i++) {
			snprintf(tmpBuf, 20, "select%d", i);
			strVal = boaGetVar(wp, tmpBuf, "");

				if ( !gstrcmp(strVal, "ON") ) {
					//tone[i/8] = tone[i/8] | (1 << (i%8));
					tone[i/8] = (tone[i/8] << 1) | 1 ;
				}else {
					//tone[i/8] = tone[i/8] | (0 << (i%8));
					tone[i/8] = (tone[i/8] << 1) | 0 ;
				}
		}
//#ifdef APPLY_CHANGE
		// set Tone mask
		adsl_drv_get(RLCM_LOADCARRIERMASK, (void *)tone, GET_LOADCARRIERMASK_SIZE);
//#endif
		goto setOk_tone;
	}



	if (strMaskAll[0]) {
		for (i=0; i<chan; i++) {
			//tone[i/8] = tone[i/8] | (1 << (i%8));
			tone[i/8] = (tone[i/8] << 1) | 1 ;
		}
		goto setOk_tone;
	}


	if (strUnmaskAll[0]) {
		for (i=0; i<chan; i++) {
			//tone[i/8] = tone[i/8] | (0 << (i%8));
			tone[i/8] = (tone[i/8] << 1) | 0 ;
		}
		goto setOk_tone;
	}

setOk_tone:

	if ( !mib_set(MIB_ADSL_TONE, (void *)tone) ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
		goto setErr_tone;
	}

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page

	OK_MSG(submitUrl);

  	return;

setErr_tone:
	ERR_MSG(tmpBuf);

}
#endif

#ifdef CONFIG_DEV_xDSL
#ifdef CONFIG_DSL_VTUO
#if 0
#define VTUO_PRT printf
#else
#define VTUO_PRT(...) while(0){}
#endif

static int FTOI(double d)
{
	char stri[33];
	snprintf( stri, 32, "%.0f", d );
	stri[32]=0;
	return atoi(stri);
}

static void vtuo_chan_init( request * wp )
{
	char *p_page="document.set_vtuo_chan";
	char *sGinpStat[]={"Forbidden", "Preferred", "Forced", "Test" };
	unsigned int tmpUInt;
	unsigned char tmpUChar;
	boaWrite(wp, "<script>\n");

	mib_get_s(VTUO_CHAN_DS_NDR_MAX, (void *)&tmpUInt, sizeof(tmpUInt));
	boaWrite(wp, "%s.DSRateMax.value=%u;\n", p_page, tmpUInt);
	mib_get_s(VTUO_CHAN_DS_NDR_MIN, (void *)&tmpUInt, sizeof(tmpUInt));
	boaWrite(wp, "%s.DSRateMin.value=%u;\n", p_page, tmpUInt);
	mib_get_s(VTUO_CHAN_DS_MAX_DELAY, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSDelay.value=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_CHAN_DS_MIN_INP, (void *)&tmpUChar, sizeof(tmpUChar));
	if(tmpUChar==0) tmpUInt=0;
	else if(tmpUChar==17) tmpUInt=1;
	else tmpUInt= tmpUChar+1;
	boaWrite(wp, "%s.DSINP.selectedIndex=%u;\n", p_page, tmpUInt);
	mib_get_s(VTUO_CHAN_DS_MIN_INP8, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSINP8.selectedIndex=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_CHAN_DS_SOS_MDR, (void *)&tmpUInt, sizeof(tmpUInt));
	boaWrite(wp, "%s.DSSOSRate.value=%u;\n", p_page, tmpUInt);

	mib_get_s(VTUO_CHAN_US_NDR_MAX, (void *)&tmpUInt, sizeof(tmpUInt));
	boaWrite(wp, "%s.USRateMax.value=%u;\n", p_page, tmpUInt);
	mib_get_s(VTUO_CHAN_US_NDR_MIN, (void *)&tmpUInt, sizeof(tmpUInt));
	boaWrite(wp, "%s.USRateMin.value=%u;\n", p_page, tmpUInt);
	mib_get_s(VTUO_CHAN_US_MAX_DELAY, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USDelay.value=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_CHAN_US_MIN_INP, (void *)&tmpUChar, sizeof(tmpUChar));
	if(tmpUChar==0) tmpUInt=0;
	else if(tmpUChar==17) tmpUInt=1;
	else tmpUInt= tmpUChar+1;
	boaWrite(wp, "%s.USINP.selectedIndex=%u;\n", p_page, tmpUInt);
	mib_get_s(VTUO_CHAN_US_MIN_INP8, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USINP8.selectedIndex=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_CHAN_US_SOS_MDR, (void *)&tmpUInt, sizeof(tmpUInt));
	boaWrite(wp, "%s.USSOSRate.value=%d;\n", p_page, tmpUInt);

	mib_get_s(VTUO_GINP_DS_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "document.getElementById('ChanGinpDs').innerHTML='%s';\n", sGinpStat[tmpUChar]);
	mib_get_s(VTUO_GINP_US_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "document.getElementById('ChanGinpUs').innerHTML='%s';\n", sGinpStat[tmpUChar]);

	boaWrite(wp, "</script>\n");
	return;
}

static void vtuo_ginp_init( request * wp )
{
	char *p_page="document.set_vtuo_ginp";
	unsigned char tmpUChar;
	unsigned int tmpUInt;
	unsigned short tmpUShort;
	boaWrite(wp, "<script>\n");

	mib_get_s(VTUO_GINP_DS_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSGinpMode.selectedIndex=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_GINP_DS_ET_MAX, (void *)&tmpUInt, sizeof(tmpUInt));
	boaWrite(wp, "%s.DSThroMax.value=%u;\n", p_page, tmpUInt);
	mib_get_s(VTUO_GINP_DS_ET_MIN, (void *)&tmpUInt, sizeof(tmpUInt));
	boaWrite(wp, "%s.DSThroMin.value=%u;\n", p_page, tmpUInt);
	mib_get_s(VTUO_GINP_DS_NDR_MAX, (void *)&tmpUInt, sizeof(tmpUInt));
	boaWrite(wp, "%s.DSNDRMax.value=%u;\n", p_page, tmpUInt);
	mib_get_s(VTUO_GINP_DS_SHINE_RATIO, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.DSRatio.value=%u;\n", p_page, tmpUShort);
	mib_get_s(VTUO_GINP_DS_LEFTR_THRD, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSThres.value=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_GINP_DS_MAX_DELAY, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSMaxDelay.value=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_GINP_DS_MIN_DELAY, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSMinDelay.value=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_GINP_DS_MIN_INP, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSINP.selectedIndex=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_GINP_DS_REIN_SYM, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSReinSym.selectedIndex=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_GINP_DS_REIN_FREQ, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSReinFreq.selectedIndex=%u;\n", p_page, tmpUChar);

	mib_get_s(VTUO_GINP_US_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USGinpMode.selectedIndex=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_GINP_US_ET_MAX, (void *)&tmpUInt, sizeof(tmpUInt));
	boaWrite(wp, "%s.USThroMax.value=%u;\n", p_page, tmpUInt);
	mib_get_s(VTUO_GINP_US_ET_MIN, (void *)&tmpUInt, sizeof(tmpUInt));
	boaWrite(wp, "%s.USThroMin.value=%u;\n", p_page, tmpUInt);
	mib_get_s(VTUO_GINP_US_NDR_MAX, (void *)&tmpUInt, sizeof(tmpUInt));
	boaWrite(wp, "%s.USNDRMax.value=%u;\n", p_page, tmpUInt);
	mib_get_s(VTUO_GINP_US_SHINE_RATIO, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.USRatio.value=%u;\n", p_page, tmpUShort);
	mib_get_s(VTUO_GINP_US_LEFTR_THRD, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USThres.value=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_GINP_US_MAX_DELAY, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USMaxDelay.value=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_GINP_US_MIN_DELAY, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USMinDelay.value=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_GINP_US_MIN_INP, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USINP.selectedIndex=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_GINP_US_REIN_SYM, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USReinSym.selectedIndex=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_GINP_US_REIN_FREQ, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USReinFreq.selectedIndex=%u;\n", p_page, tmpUChar);

	boaWrite(wp, "</script>\n");
	return;
}

static void vtuo_line_init( request * wp )
{
	char *p_page="document.set_vtuo_line";
	char *sra_mode[4]={"Manual","AdaptInit","Dynamic","SOS"};
	unsigned char tmpUChar;
	unsigned int tmpUInt;
	unsigned short tmpUShort;
	short tmpShort;
	double tmpD;
	int num;
	boaWrite(wp, "<script>\n");

	mib_get_s(VTUO_LINE_VDSL2_PROFILE, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.Vd2Prof35b.checked=%s;\n", p_page, (tmpUShort&VDSL2_PROFILE_35B)?"true":"false" );
	boaWrite(wp, "%s.Vd2Prof30a.checked=%s;\n", p_page, (tmpUShort&VDSL2_PROFILE_30A)?"true":"false" );
	boaWrite(wp, "%s.Vd2Prof17a.checked=%s;\n", p_page, (tmpUShort&VDSL2_PROFILE_17A)?"true":"false" );
	boaWrite(wp, "%s.Vd2Prof12a.checked=%s;\n", p_page, (tmpUShort&VDSL2_PROFILE_12A)?"true":"false" );
	boaWrite(wp, "%s.Vd2Prof12b.checked=%s;\n", p_page, (tmpUShort&VDSL2_PROFILE_12B)?"true":"false" );
	boaWrite(wp, "%s.Vd2Prof8a.checked=%s;\n", p_page, (tmpUShort&VDSL2_PROFILE_8A)?"true":"false" );
	boaWrite(wp, "%s.Vd2Prof8b.checked=%s;\n", p_page, (tmpUShort&VDSL2_PROFILE_8B)?"true":"false" );
	boaWrite(wp, "%s.Vd2Prof8c.checked=%s;\n", p_page, (tmpUShort&VDSL2_PROFILE_8C)?"true":"false" );
	boaWrite(wp, "%s.Vd2Prof8d.checked=%s;\n", p_page, (tmpUShort&VDSL2_PROFILE_8D)?"true":"false" );
	/*
	mib_get_s(VTUO_LINE_DS_MAX_SNR_NOLMT, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSMaxSNRNoLmt.checked=%s;\n", p_page, tmpUChar?"true":"false" );
	*/
	mib_get_s(VTUO_LINE_DS_MAX_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.DSMaxSNR.value='%.1f';\n", p_page, tmpD );
 	mib_get_s(VTUO_LINE_DS_TARGET_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.DSTargetSNR.value='%.1f';\n", p_page, tmpD );
 	mib_get_s(VTUO_LINE_DS_MIN_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.DSMinSNR.value='%.1f';\n", p_page, tmpD );
 	mib_get_s(VTUO_LINE_DS_BITSWAP, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSBitswap[%u].checked=true;\n", p_page, tmpUChar);
	/*
	mib_get_s(VTUO_LINE_DS_MAX_TXPWR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.DSMaxTxPwr.value='%.1f';\n", p_page, tmpD );

 	mib_get_s(VTUO_LINE_DS_MIN_OH_RATE, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.DSMinOhRate.value=%u;\n", p_page, tmpUShort );

	mib_get_s(VTUO_LINE_US_MAX_SNR_NOLMT, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USMaxSNRNoLmt.checked=%s;\n", p_page, tmpUChar?"true":"false");
	*/
	mib_get_s(VTUO_LINE_US_MAX_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.USMaxSNR.value='%.1f';\n", p_page, tmpD );
 	mib_get_s(VTUO_LINE_US_TARGET_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.USTargetSNR.value='%.1f';\n", p_page, tmpD );
 	mib_get_s(VTUO_LINE_US_MIN_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.USMinSNR.value='%.1f';\n", p_page, tmpD );
 	mib_get_s(VTUO_LINE_US_BITSWAP, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USBitswap[%u].checked=true;\n", p_page, tmpUChar);
	/*
	mib_get_s(VTUO_LINE_US_MAX_RXPWR_NOLMT, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USMaxRxPwrNoLmt.checked=%s;\n", p_page, tmpUChar?"true":"false");
	mib_get_s(VTUO_LINE_US_MAX_RXPWR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.USMaxRxPwr.value='%.1f';\n", p_page, tmpD );
 	mib_get_s(VTUO_LINE_US_MAX_TXPWR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.USMaxTxPwr.value='%.1f';\n", p_page, tmpD );

 	mib_get_s(VTUO_LINE_US_MIN_OH_RATE, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.USMinOhRate.value=%u;\n", p_page, tmpUShort );
	mib_get_s(VTUO_LINE_TRANS_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.TransMode.selectedIndex=%u;\n", p_page, tmpUChar);
	*/
	mib_get_s(VTUO_LINE_ADSL_PROTOCOL, (void *)&tmpUInt, sizeof(tmpUInt));
	boaWrite(wp, "%s.AdProtANSI.checked=%s;\n", p_page, (tmpUInt&MODE_ANSI)?"true":"false" );
	boaWrite(wp, "%s.AdProtETSI.checked=%s;\n", p_page, (tmpUInt&MODE_ETSI)?"true":"false" );
	boaWrite(wp, "%s.AdProtG9921.checked=%s;\n", p_page, (tmpUInt&MODE_GDMT)?"true":"false" );
	boaWrite(wp, "%s.AdProtG9922.checked=%s;\n", p_page, (tmpUInt&MODE_GLITE)?"true":"false" );
	boaWrite(wp, "%s.AdProtG9923.checked=%s;\n", p_page, (tmpUInt&MODE_ADSL2)?"true":"false" );
	boaWrite(wp, "%s.AdProtG9925.checked=%s;\n", p_page, (tmpUInt&MODE_ADSL2PLUS)?"true":"false" );
	/*
	mib_get_s(VTUO_LINE_CLASS_MASK, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.ClassMask.selectedIndex=%u;\n", p_page, tmpUChar);
	*/
	mib_get_s(VTUO_LINE_LIMIT_MASK, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.LimitMask.selectedIndex=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_LINE_US0_MASK, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.US0Mask.selectedIndex=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_LINE_UPBO_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.UPBO[%u].checked=true;\n", p_page, tmpUChar);
	mib_get_s(VTUO_LINE_UPBOKL, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.UPBOKL.value='%.1f';\n", p_page, tmpD );

	mib_get_s(VTUO_LINE_UPBO_1A, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/100;
	boaWrite(wp, "%s.USBand1a.value='%.2f';\n", p_page, tmpD );
 	mib_get_s(VTUO_LINE_UPBO_2A, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/100;
	boaWrite(wp, "%s.USBand2a.value='%.2f';\n", p_page, tmpD );
 	mib_get_s(VTUO_LINE_UPBO_3A, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/100;
	boaWrite(wp, "%s.USBand3a.value='%.2f';\n", p_page, tmpD );
 	mib_get_s(VTUO_LINE_UPBO_4A, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/100;
	boaWrite(wp, "%s.USBand4a.value='%.2f';\n", p_page, tmpD );
 	mib_get_s(VTUO_LINE_UPBO_1B, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/100;
	boaWrite(wp, "%s.USBand1b.value='%.2f';\n", p_page, tmpD );
 	mib_get_s(VTUO_LINE_UPBO_2B, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/100;
	boaWrite(wp, "%s.USBand2b.value='%.2f';\n", p_page, tmpD );
 	mib_get_s(VTUO_LINE_UPBO_3B, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/100;
	boaWrite(wp, "%s.USBand3b.value='%.2f';\n", p_page, tmpD );
 	mib_get_s(VTUO_LINE_UPBO_4B, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/100;
	boaWrite(wp, "%s.USBand4b.value='%.2f';\n", p_page, tmpD );
	/*
 	mib_get_s(VTUO_LINE_RT_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.RTMode.checked=%s;\n", p_page, tmpUChar?"true":"false" );
	*/
	mib_get_s(VTUO_LINE_US0_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.US0[%u].checked=true;\n", p_page, tmpUChar);

	mib_get_s(VTUO_SRA_DS_RA_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "document.getElementById('LineSRAModeDs').innerHTML='DS: %s';\n", sra_mode[tmpUChar]);
	mib_get_s(VTUO_SRA_US_RA_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "document.getElementById('LineSRAModeUs').innerHTML='US: %s';\n", sra_mode[tmpUChar]);

	num=mib_chain_total( MIB_VTUO_PSD_DS_TBL );
	boaWrite(wp, "document.getElementById('LineMibPsdDs').innerHTML='DS: %d Break Point';\n", num);
	num=mib_chain_total( MIB_VTUO_PSD_US_TBL );
	boaWrite(wp, "document.getElementById('LineMibPsdUs').innerHTML='US: %d Break Point';\n", num);
	/*
	mib_get_s(VTUO_VN_DS_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "document.getElementById('LineVNDs').innerHTML='DS: %s';\n", tmpUChar?"Enable":"Disable");
	mib_get_s(VTUO_VN_US_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "document.getElementById('LineVNUs').innerHTML='US: %s';\n", tmpUChar?"Enable":"Disable");
	*/
	mib_get_s(VTUO_DPBO_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "document.getElementById('LineDpboEnable').innerHTML='%s';\n", tmpUChar?"Enable":"Disable");
	/*
	num=mib_chain_total( MIB_VTUO_RFI_TBL );
	boaWrite(wp, "document.getElementById('LineRfi').innerHTML='%d band';\n", num);
	*/
	boaWrite(wp, "</script>\n");
	return;
}

static void vtuo_inm_init( request * wp )
{
	char *p_page="document.set_vtuo_inm";
	unsigned char tmpUChar;
	short tmpShort;
	double tmpD;
	boaWrite(wp, "<script>\n");

	mib_get_s(VTUO_INM_NE_INP_EQ_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.NEInpEqMode.selectedIndex=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_INM_NE_INMCC, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.NEInmCc.value=%u;\n", p_page, tmpUChar );
	mib_get_s(VTUO_INM_NE_IAT_OFFSET, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.NEIatOff.value=%u;\n", p_page, tmpUChar );
	mib_get_s(VTUO_INM_NE_IAT_SETUP, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.NEIatSet.value=%u;\n", p_page, tmpUChar );
	/*
	mib_get_s(VTUO_INM_NE_ISDD_SEN, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.NEIsddSen.value='%.1f';\n", p_page, tmpD );
	*/
	mib_get_s(VTUO_INM_FE_INP_EQ_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.FEInpEqMode.selectedIndex=%u;\n", p_page, tmpUChar);
	mib_get_s(VTUO_INM_FE_INMCC, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.FEInmCc.value=%u;\n", p_page, tmpUChar );
	mib_get_s(VTUO_INM_FE_IAT_OFFSET, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.FEIatOff.value=%u;\n", p_page, tmpUChar );
	mib_get_s(VTUO_INM_FE_IAT_SETUP, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.FEIatSet.value=%u;\n", p_page, tmpUChar );
	/*
	mib_get_s(VTUO_INM_FE_ISDD_SEN, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.FEIsddSen.value='%.1f';\n", p_page, tmpD );
	*/
	boaWrite(wp, "</script>\n");
	return;
}

static void vtuo_sra_init( request * wp )
{
	char *p_page="document.set_vtuo_sra";
	unsigned char tmpUChar;
	unsigned short tmpUShort;
	short tmpShort;
	double tmpD;
	boaWrite(wp, "<script>\n");

	mib_get_s(VTUO_SRA_DS_RA_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSRateAdapt[%u].checked=true;\n", p_page, tmpUChar);
	mib_get_s(VTUO_SRA_DS_DYNAMIC_DEPTH, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSDynDep[%u].checked=true;\n", p_page, tmpUChar);
	mib_get_s(VTUO_SRA_DS_USHIFT_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.DSUpShiftSNR.value='%.1f';\n", p_page, tmpD );
	mib_get_s(VTUO_SRA_DS_USHIFT_TIME, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.DSUpShiftTime.value=%u;\n", p_page, tmpUShort );
	mib_get_s(VTUO_SRA_DS_DSHIFT_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.DSDownShiftSNR.value='%.1f';\n", p_page, tmpD );
	mib_get_s(VTUO_SRA_DS_DSHIFT_TIME, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.DSDownShiftTime.value=%u;\n", p_page, tmpUShort );

	mib_get_s(VTUO_SRA_DS_SOS_TIME, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.DSSosTime.value=%u;\n", p_page, tmpUShort );
	mib_get_s(VTUO_SRA_DS_SOS_CRC, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.DSSosCrc.value=%u;\n", p_page, tmpUShort );
	mib_get_s(VTUO_SRA_DS_SOS_NTONE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSSosnTones.value=%u;\n", p_page, tmpUChar );
	mib_get_s(VTUO_SRA_DS_SOS_MAX, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSSosMax.value=%u;\n", p_page, tmpUChar );
	/*
	mib_get_s(VTUO_SRA_DS_SOS_MSTEP_TONE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSSosMultiStep.selectedIndex=%u;\n", p_page, tmpUChar);
	*/
	mib_get_s(VTUO_SRA_DS_ROC_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSRocEnable[%u].checked=true;\n", p_page, tmpUChar);
	mib_get_s(VTUO_SRA_DS_ROC_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.DSRocSNR.value='%.1f';\n", p_page, tmpD );
	mib_get_s(VTUO_SRA_DS_ROC_MIN_INP, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSRocMinINP.selectedIndex=%u;\n", p_page, tmpUChar);



	mib_get_s(VTUO_SRA_US_RA_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USRateAdapt[%u].checked=true;\n", p_page, tmpUChar);
	mib_get_s(VTUO_SRA_US_DYNAMIC_DEPTH, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USDynDep[%u].checked=true;\n", p_page, tmpUChar);
	mib_get_s(VTUO_SRA_US_USHIFT_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.USUpShiftSNR.value='%.1f';\n", p_page, tmpD );
	mib_get_s(VTUO_SRA_US_USHIFT_TIME, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.USUpShiftTime.value=%u;\n", p_page, tmpUShort );
	mib_get_s(VTUO_SRA_US_DSHIFT_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.USDownShiftSNR.value='%.1f';\n", p_page, tmpD );
	mib_get_s(VTUO_SRA_US_DSHIFT_TIME, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.USDownShiftTime.value=%u;\n", p_page, tmpUShort );

	mib_get_s(VTUO_SRA_US_SOS_TIME, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.USSosTime.value=%u;\n", p_page, tmpUShort );
	mib_get_s(VTUO_SRA_US_SOS_CRC, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.USSosCrc.value=%u;\n", p_page, tmpUShort );
	mib_get_s(VTUO_SRA_US_SOS_NTONE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USSosnTones.value=%u;\n", p_page, tmpUChar );
	mib_get_s(VTUO_SRA_US_SOS_MAX, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USSosMax.value=%u;\n", p_page, tmpUChar );
	/*
	mib_get_s(VTUO_SRA_US_SOS_MSTEP_TONE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USSosMultiStep.selectedIndex=%u;\n", p_page, tmpUChar);
	*/
	mib_get_s(VTUO_SRA_US_ROC_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USRocEnable[%u].checked=true;\n", p_page, tmpUChar);
	mib_get_s(VTUO_SRA_US_ROC_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.USRocSNR.value='%.1f';\n", p_page, tmpD );
	mib_get_s(VTUO_SRA_US_ROC_MIN_INP, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USRocMinINP.selectedIndex=%u;\n", p_page, tmpUChar);


	boaWrite(wp, "</script>\n");
	return;
}

static void vtuo_dpbo_init( request * wp )
{
	char *p_page="document.set_vtuo_dpbo";
	unsigned char tmpUChar;
	unsigned short tmpUShort;
	short tmpShort;
	int tmpInt;
	double tmpD;

	mib_get_s(VTUO_DPBO_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DpboEnable[%u].checked=true;\n", p_page, tmpUChar);
	mib_get_s(VTUO_DPBO_ESEL, (void *)&tmpShort, sizeof(tmpShort));
	tmpD= (double)tmpShort/10;
	boaWrite(wp, "%s.DpboESel.value='%.1f';\n", p_page, tmpD );
	mib_get_s(VTUO_DPBO_ESCMA, (void *)&tmpInt, sizeof(tmpInt));
	tmpD=(double)tmpInt/10000;
	boaWrite(wp, "%s.DpboEScma.value='%.4f';\n", p_page, tmpD );
	mib_get_s(VTUO_DPBO_ESCMB, (void *)&tmpInt, sizeof(tmpInt));
	tmpD=(double)tmpInt/10000;
	boaWrite(wp, "%s.DpboEScmb.value='%.4f';\n", p_page, tmpD );
	mib_get_s(VTUO_DPBO_ESCMC, (void *)&tmpInt, sizeof(tmpInt));
	tmpD=(double)tmpInt/10000;
	boaWrite(wp, "%s.DpboEScmc.value='%.4f';\n", p_page, tmpD );
	mib_get_s(VTUO_DPBO_MUS, (void *)&tmpShort, sizeof(tmpShort));
	tmpD=(double)tmpShort/10;
	boaWrite(wp, "%s.DpboMus.value='%.1f';\n", p_page, tmpD );
	mib_get_s(VTUO_DPBO_FMIN, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.DpboFMin.value='%u';\n", p_page, tmpUShort );
	mib_get_s(VTUO_DPBO_FMAX, (void *)&tmpUShort, sizeof(tmpUShort));
	boaWrite(wp, "%s.DpboFMax.value='%u';\n", p_page, tmpUShort );

	return;
}

static void vtuo_dpbo_init_tbl( request * wp )
{
	char *p_page="document.set_vtuo_dpbo";
	MIB_CE_VTUO_DPBO_T entry, *p=&entry;
	int num, i;
	double tmpD;

	num=mib_chain_total( MIB_VTUO_DPBO_TBL );
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_DPBO_TBL, i, p))
			continue;

		boaWrite(wp, "%s.DpboToneId%d.value='%u';\n", p_page, i+1, p->ToneId );
		tmpD=(double)p->PsdLevel/10;
		boaWrite(wp, "%s.DpboPsd%d.value='%.1f';\n", p_page, i+1, tmpD );
	}

	return;
}

static void vtuo_psd_init_tbl( request * wp )
{
	char *p_page="document.set_vtuo_psd";
	MIB_CE_VTUO_PSD_T entry, *p=&entry;
	int num, i;
	double tmpD;

	num=mib_chain_total( MIB_VTUO_PSD_DS_TBL );
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_PSD_DS_TBL, i, p))
			continue;

		boaWrite(wp, "%s.ToneIdDs%d.value='%u';\n", p_page, i+1, p->ToneId );
		tmpD=(double)p->PsdLevel/10;
		boaWrite(wp, "%s.PsdDs%d.value='%.1f';\n", p_page, i+1, tmpD );
	}

	num=mib_chain_total( MIB_VTUO_PSD_US_TBL );
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_PSD_US_TBL, i, p))
			continue;

		boaWrite(wp, "%s.ToneIdUs%d.value='%u';\n", p_page, i+1, p->ToneId );
		tmpD=(double)p->PsdLevel/10;
		boaWrite(wp, "%s.PsdUs%d.value='%.1f';\n", p_page, i+1, tmpD );
	}

	return;
}
/*
static void vtuo_vn_init_tbl( request * wp )
{
	char *p_page="document.set_vtuo_psd";
	MIB_CE_VTUO_VN_T entry, *p=&entry;
	int num, i;
	double tmpD;

	num=mib_chain_total( MIB_VTUO_VN_DS_TBL );
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_VN_DS_TBL, i, p))
			continue;

		boaWrite(wp, "%s.ToneIdDs%d.value='%u';\n", p_page, i+1, p->ToneId );
		tmpD=(double)p->NoiseLevel/10;
		boaWrite(wp, "%s.PsdDs%d.value='%.1f';\n", p_page, i+1, tmpD );
	}

	num=mib_chain_total( MIB_VTUO_VN_US_TBL );
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_VN_US_TBL, i, p))
			continue;

		boaWrite(wp, "%s.ToneIdUs%d.value='%u';\n", p_page, i+1, p->ToneId );
		tmpD=(double)p->NoiseLevel/10;
		boaWrite(wp, "%s.PsdUs%d.value='%.1f';\n", p_page, i+1, tmpD );
	}

	return;
}

static void vtuo_vn_init( request * wp )
{
	char *p_page="document.set_vtuo_psd";
	unsigned char tmpUChar;

	mib_get_s(VTUO_VN_DS_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.DSVnEnable[%u].checked=true;\n", p_page, tmpUChar);

	mib_get_s(VTUO_VN_US_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	boaWrite(wp, "%s.USVnEnable[%u].checked=true;\n", p_page, tmpUChar);

	return;
}

static void vtuo_rfi_init_tbl( request * wp )
{
	char *p_page="document.set_vtuo_rfi";
	MIB_CE_VTUO_RFI_T entry, *p=&entry;
	int num, i;

	num=mib_chain_total( MIB_VTUO_RFI_TBL );
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_RFI_TBL, i, p))
			continue;

		boaWrite(wp, "%s.ToneId%d.value='%u';\n", p_page, i+1, p->ToneId );
		boaWrite(wp, "%s.ToneIdEnd%d.value='%u';\n", p_page, i+1, p->ToneIdEnd );
	}

	return;
}
*/
static void vtuo_stat_band( request * wp )
{
#ifndef CONFIG_GENERAL_WEB
#define FMT_STAT_BAND_START "<tr align=center bgcolor=#f0f0f0>\n<th align=left bgcolor=#c0c0c0><font size=2>%s</th>\n"
#else
#define FMT_STAT_BAND_START "<tr align=center>\n<th align=left>%s</th>\n"
#endif
#define FMT_STAT_BAND_END "\n</tr>\n"

	XDSL_OP *d=xdsl_get_op(0);
	int msgval[9], i;
	double tmpD;

	memset( msgval, 0, sizeof(msgval) );
	d->xdsl_msg_get_array(GetSNRpb, msgval);
	boaWrite(wp, FMT_STAT_BAND_START, "SNRM (dB)" );
	for(i=0; i<9; i++)
	{
		if(msgval[i]==-512)
		{
			boaWrite(wp, "<td>n/a</td>");
		}else{
			tmpD=(double)msgval[i]/10;
			boaWrite(wp, "<td>%.1f</td>", tmpD);
		}
	}
	boaWrite(wp, FMT_STAT_BAND_END);

	memset( msgval, 0, sizeof(msgval) );
	d->xdsl_msg_get_array(GetSATNpb, msgval);
	boaWrite(wp, FMT_STAT_BAND_START, "SATN (dB)" );
	for(i=0; i<9; i++)
	{
		if(msgval[i]==1023)
		{
			boaWrite(wp, "<td>n/a</td>");
		}else{
			tmpD=(double)msgval[i]/10;
			boaWrite(wp, "<td>%.1f</td>", tmpD);
		}
	}
	boaWrite(wp, FMT_STAT_BAND_END);

	memset( msgval, 0, sizeof(msgval) );
	d->xdsl_msg_get_array(GetLATNpb, msgval);
	boaWrite(wp, FMT_STAT_BAND_START, "LATN (dB)" );
	for(i=0; i<9; i++)
	{
		if(msgval[i]==1023)
		{
			boaWrite(wp, "<td>n/a</td>");
		}else{
			tmpD=(double)msgval[i]/10;
			boaWrite(wp, "<td>%.1f</td>", tmpD);
		}
	}
	boaWrite(wp, FMT_STAT_BAND_END);
}

#if 1

static int vtuo_stat_time_fmt(unsigned int uptime, char *buf, int webfmt)
{
	unsigned int updays, upsec, upminutes, uphours;

	if(buf)
	{
		updays = uptime / (60*60*24);
		if (updays)
			sprintf(buf, "%u day%s,%s", updays, (updays != 1) ? "s" : "", webfmt?"&nbsp;":" ");
		upsec = uptime % 60;
		upminutes = uptime / 60;
		uphours = (upminutes / 60) % 24;
		upminutes %= 60;
		sprintf(buf, "%02u:%02u:%02u\n", uphours, upminutes, upsec);

		return 0;
	}

	return -1;
}

static void vtuo_stat_perform( request * wp )
{
#ifndef CONFIG_GENERAL_WEB
#define FMT_STAT_PFM_START "<tr bgcolor=#f0f0f0>\n<th align=left bgcolor=#c0c0c0><font size=2>%s</th>\n"
#else
#define FMT_STAT_PFM_START "<tr>\n<th align=left>%s</th>\n"
#endif
#define FMT_STAT_PFM_END "\n</tr>\n"
	int i,j;
	XDSL_OP *d=xdsl_get_op(0);
	int tmp2dsl[6][27];//sincelink:0=ds/1=us, 15min:2=ds/3=us, 1day:4=ds/5=us
	char *tbuf[]={"ES", "SES", "LOSs", "LOFs", "UAS",
			"CV/CRC", "Corrected", "Uncorrected", "Rtx", "RtxCorrected",
			"RtxUncorrected", "LEFTRs", "MinEFTR", "ErrFreeBits", "LOL",
			"LPR", "Time Elapsed" };
	char buf[32];

	memset(tmp2dsl, 0, sizeof(tmp2dsl) );
	tmp2dsl[0][0]=0;
	d->xdsl_msg_get_array( GetEvCntDs_e127, &tmp2dsl[0][0] );
	tmp2dsl[1][0]=0;
	d->xdsl_msg_get_array( GetEvCntUs_e127, &tmp2dsl[1][0] );
	tmp2dsl[2][0]=0;
	d->xdsl_msg_get_array( GetEvCnt15MinDs_e127, &tmp2dsl[2][0] );
	tmp2dsl[3][0]=0;
	d->xdsl_msg_get_array( GetEvCnt15MinUs_e127, &tmp2dsl[3][0] );
	tmp2dsl[4][0]=0;
	d->xdsl_msg_get_array( GetEvCnt1DayDs_e127, &tmp2dsl[4][0] );
	tmp2dsl[5][0]=0;
	d->xdsl_msg_get_array( GetEvCnt1DayUs_e127, &tmp2dsl[5][0] );

#ifndef CONFIG_GENERAL_WEB
	boaWrite(wp, "<tr bgcolor=#f0f0f0>\n");
	boaWrite(wp, "<th align=left bgcolor=#c0c0c0></th>\n");
#else
	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<th align=left></th>\n");
#endif
	boaWrite(wp, "<th colspan=2>Since Link</th>\n");
	boaWrite(wp, "<th colspan=2>Curr 15 Min</th>\n");
	boaWrite(wp, "<th colspan=2>Curr 1 Day</th>\n");
	boaWrite(wp, "</tr>\n");

#ifndef CONFIG_GENERAL_WEB
	boaWrite(wp, "<tr bgcolor=#f0f0f0>\n");
	boaWrite(wp, "<th align=left bgcolor=#c0c0c0><font size=2>Full Inits</th>\n");
#else
	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<th align=left>Full Inits</th>\n");
#endif
	boaWrite(wp, "<td colspan=2>n/a</td>\n");
	boaWrite(wp, "<td colspan=2>%u</td>\n", tmp2dsl[2][1] );
	boaWrite(wp, "<td colspan=2>%u</td>\n", tmp2dsl[4][1] );
	boaWrite(wp, "</tr>\n");

#ifndef CONFIG_GENERAL_WEB
	boaWrite(wp, "<tr bgcolor=#f0f0f0>\n");
	boaWrite(wp, "<th align=left bgcolor=#c0c0c0><font size=2>Failed Full Inits</th>\n");
#else
	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<th align=left>Failed Full Inits</th>\n");
#endif
	boaWrite(wp, "<td colspan=2>n/a</td>\n");
	boaWrite(wp, "<td colspan=2>%u</td>\n", tmp2dsl[2][2] );
	boaWrite(wp, "<td colspan=2>%u</td>\n", tmp2dsl[4][2] );
	boaWrite(wp, "</tr>\n");

#ifndef CONFIG_GENERAL_WEB
	boaWrite(wp, "<tr align=center bgcolor=#f0f0f0>\n");
	boaWrite(wp, "<th bgcolor=#c0c0c0></th>\n");
#else
	boaWrite(wp, "<tr align=center>\n");
	boaWrite(wp, "<th></th>\n");
#endif
	for(j=0; j<6; j++)
		boaWrite(wp, "<td>VTU%s</td>", (j&0x1)?"O":"R" );
	boaWrite(wp, "</tr>\n");

	for(i=0; i<14; i++)
	{
		/* skip some elements */
		if (i >= 6 && i <= 10)
			continue;
		boaWrite(wp, FMT_STAT_PFM_START, tbuf[i] );
		for(j=0; j<6; j++)
			boaWrite(wp, "<td>%u</td>", tmp2dsl[j][i+4] );
		boaWrite(wp, FMT_STAT_PFM_END);
	}

	/*LOL*/
	boaWrite(wp, FMT_STAT_PFM_START, tbuf[14] );
	for(j=0; j<6; j++)
		boaWrite(wp, "<td>%u</td>", tmp2dsl[j][19]);
 	boaWrite(wp, FMT_STAT_PFM_END);

	/*LPR*/
	boaWrite(wp, FMT_STAT_PFM_START, tbuf[15] );
	for(j=0; j<6; j++)
		boaWrite(wp, "<td>%u</td>", tmp2dsl[j][21]);
	boaWrite(wp, FMT_STAT_PFM_END);

	/*TimeElapsed*/
	boaWrite(wp, FMT_STAT_PFM_START, tbuf[16] );
	for(j=0; j<3; j++)
	{
		vtuo_stat_time_fmt( tmp2dsl[j * 2][22], buf, 1 );
		boaWrite(wp, "<td colspan=2>%s</td>", buf);
	}
	boaWrite(wp, FMT_STAT_PFM_END);

	/*boaWrite(wp, "<h4>* Impulse Noise Moniter Current Counter</h4>\n");*/

	memset(tmp2dsl, 0, sizeof(tmp2dsl) );
	tmp2dsl[0][0]=0;
	d->xdsl_msg_get_array( GetInmCntDs_e127, &tmp2dsl[0][0] );
	tmp2dsl[1][0]=0;
	d->xdsl_msg_get_array( GetInmCntUs_e127, &tmp2dsl[1][0] );
	tmp2dsl[2][0]=0;
	d->xdsl_msg_get_array( GetInmCnt15MinDs_e127, &tmp2dsl[2][0] );
	tmp2dsl[3][0]=0;
	d->xdsl_msg_get_array( GetInmCnt15MinUs_e127, &tmp2dsl[3][0] );
	tmp2dsl[4][0]=0;
	d->xdsl_msg_get_array( GetInmCnt1DayDs_e127, &tmp2dsl[4][0] );
	tmp2dsl[5][0]=0;
	d->xdsl_msg_get_array( GetInmCnt1DayUs_e127, &tmp2dsl[5][0] );

	for (i = 1; i <= 26; i++) {
		if (i < 18)
			sprintf(buf, "INMEQ%u", i);
		else if (i < 26)
			sprintf(buf, "IAT%u", i - 18);
		else
			sprintf(buf, "Symbol");
		boaWrite(wp, FMT_STAT_PFM_START, buf);
		for (j = 0; j < 6; j++)
			boaWrite(wp, "<td>%u</td>", tmp2dsl[j][i]);
		boaWrite(wp, FMT_STAT_PFM_END);
	}

	return;
}


#else
static void vtuo_stat_perform( request * wp )
{
#define FMT_STAT_PFM_START "<tr bgcolor=#f0f0f0>\n<th align=left bgcolor=#c0c0c0><font size=2>%s</th>\n"
#define FMT_STAT_PFM_END "\n</tr>\n"

	XDSL_OP *d=xdsl_get_op(0);
	int msgval[4], tmpval[4];
	char buf[32];

	/*FECS*/
	boaWrite(wp, FMT_STAT_PFM_START, "FECS" );
	d->xdsl_msg_get_array(GetFECS, msgval);
	boaWrite(wp, "<td>%u</td>", msgval[0]);
	boaWrite(wp, "<td>%u</td>", msgval[1]);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*ES*/
	boaWrite(wp, FMT_STAT_PFM_START, "ES" );
	getAdslDrvInfo( "es-ds", buf, sizeof(buf) );
	boaWrite(wp, "<td>%s</td>", buf);
	getAdslDrvInfo( "es-us", buf, sizeof(buf) );
	boaWrite(wp, "<td>%s</td>", buf);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*SES*/
	boaWrite(wp, FMT_STAT_PFM_START, "SES" );
	getAdslDrvInfo( "ses-ds", buf, sizeof(buf) );
	boaWrite(wp, "<td>%s</td>", buf);
	getAdslDrvInfo( "ses-us", buf, sizeof(buf) );
	boaWrite(wp, "<td>%s</td>", buf);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*LOSs*/
	boaWrite(wp, FMT_STAT_PFM_START, "LOSs" );
	d->xdsl_msg_get_array(GetLOSs, msgval);
	boaWrite(wp, "<td>%u</td>", msgval[0]);
	boaWrite(wp, "<td>%u</td>", msgval[1]);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*LOFs*/
	boaWrite(wp, FMT_STAT_PFM_START, "LOFs" );
	d->xdsl_msg_get_array(GetLOFs, msgval);
	boaWrite(wp, "<td>%u</td>", msgval[0]);
	boaWrite(wp, "<td>%u</td>", msgval[1]);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*UAS*/
	boaWrite(wp, FMT_STAT_PFM_START, "UAS" );
	getAdslDrvInfo( "uas-ds", buf, sizeof(buf) );
	boaWrite(wp, "<td>%s</td>", buf);
	getAdslDrvInfo( "uas-us", buf, sizeof(buf) );
	boaWrite(wp, "<td>%s</td>", buf);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*Code Violation*/
	boaWrite(wp, FMT_STAT_PFM_START, "Code Violation" );
	getAdslDrvInfo( "crc-ds", buf, sizeof(buf) );
	boaWrite(wp, "<td>%s</td>", buf);
	getAdslDrvInfo( "crc-us", buf, sizeof(buf) );
	boaWrite(wp, "<td>%s</td>", buf);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*Corrected*/
	boaWrite(wp, FMT_STAT_PFM_START, "Corrected" );
	d->xdsl_msg_get_array(GetRSCorr, msgval);
	boaWrite(wp, "<td>%u</td>", msgval[0]);
	boaWrite(wp, "<td>%u</td>", msgval[1]);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*Uncorrected*/
	boaWrite(wp, FMT_STAT_PFM_START, "Uncorrected" );
	d->xdsl_msg_get_array(GetRSUnCorr, msgval);
	boaWrite(wp, "<td>%u</td>", msgval[0]);
	boaWrite(wp, "<td>%u</td>", msgval[1]);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*Rtx*/
	boaWrite(wp, FMT_STAT_PFM_START, "Rtx" );
	d->xdsl_msg_get_array(GetReTxRtx, msgval);
	boaWrite(wp, "<td>%u</td>", msgval[0]);
	boaWrite(wp, "<td>%u</td>", msgval[1]);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*RtxCorrected*/
	boaWrite(wp, FMT_STAT_PFM_START, "RtxCorrected" );
	d->xdsl_msg_get_array(GetReTxRtxCorr, msgval);
	boaWrite(wp, "<td>%u</td>", msgval[0]);
	boaWrite(wp, "<td>%u</td>", msgval[1]);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*RtxUncorrected*/
	boaWrite(wp, FMT_STAT_PFM_START, "RtxUncorrected" );
	d->xdsl_msg_get_array(GetReTxRtxUnCorr, msgval);
	boaWrite(wp, "<td>%u</td>", msgval[0]);
	boaWrite(wp, "<td>%u</td>", msgval[1]);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*LEFTRs*/
	boaWrite(wp, FMT_STAT_PFM_START, "LEFTRs" );
	d->xdsl_msg_get_array(GetReTxLEFTRs, msgval);
	boaWrite(wp, "<td>%u</td>", msgval[0]);
	boaWrite(wp, "<td>%u</td>", msgval[1]);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*MinEFTR*/
	boaWrite(wp, FMT_STAT_PFM_START, "MinEFTR" );
	d->xdsl_msg_get_array(GetReTxMinEFTR, msgval);
	boaWrite(wp, "<td>%u</td>", msgval[0]);
	boaWrite(wp, "<td>%u</td>", msgval[1]);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*ErrFreeBits*/
	boaWrite(wp, FMT_STAT_PFM_START, "ErrFreeBits" );
	d->xdsl_msg_get_array(GetReTxErrFreeBits, msgval);
	boaWrite(wp, "<td>%u</td>", msgval[0]);
	boaWrite(wp, "<td>%u</td>", msgval[1]);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*LOLs/LOL*/
	boaWrite(wp, FMT_STAT_PFM_START, "LOLs/LOL" );
	d->xdsl_msg_get_array(GetLOLs, msgval);
	d->xdsl_msg_get_array(GetLOL, tmpval);
	boaWrite(wp, "<td>%u/%u</td>", msgval[0], tmpval[0]);
	boaWrite(wp, "<td>%u/%u</td>", msgval[1], tmpval[1]);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	/*LPRs/LPR*/
	boaWrite(wp, FMT_STAT_PFM_START, "LPRs/LPR" );
	d->xdsl_msg_get_array(GetLPRs, msgval);
	d->xdsl_msg_get_array(GetLPR, tmpval);
	boaWrite(wp, "<td>%u/%u</td>", msgval[0], tmpval[0]);
	boaWrite(wp, "<td>%u/%u</td>", msgval[1], tmpval[1]);
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, "<td></td><td></td>");
	boaWrite(wp, FMT_STAT_PFM_END);

	return;
}
#endif
static void vtuo_stat_medley_psd( request * wp )
{
#define FMT_STAT_PSD_START "<tr align=center bgcolor=#f0f0f0>\n"
#define FMT_STAT_PSD_MID_1 "<th align=center bgcolor=#c0c0c0><font size=2>"
#define FMT_STAT_PSD_MID_2 "</th>\n"
#define FMT_STAT_PSD_END "\n</tr>\n"
#define VTUO_MEDLEY_PSD_DS_MAXSIZE	48
#define VTUO_MEDLEY_PSD_US_MAXSIZE	32
	XDSL_OP *d=xdsl_get_op(0);
	int ds_msg[1+2*VTUO_MEDLEY_PSD_DS_MAXSIZE], us_msg[1+2*VTUO_MEDLEY_PSD_US_MAXSIZE];
	int i, isShowtime;
	double tmpD;
	char buf[64];

	isShowtime=0;
	memset( buf, 0, sizeof(buf) );
	d->xdsl_drv_get(RLCM_REPORT_MODEM_STATE, (void *)buf, sizeof(buf));
	if( strncmp(buf, "SHOWTIME", 8)==0 )	isShowtime=1;

	if(isShowtime)
	{
		memset( ds_msg, 0, sizeof(ds_msg) );
		ds_msg[0]=VTUO_MEDLEY_PSD_DS_MAXSIZE;
		d->xdsl_msg_get_array(GetPSD_MD_Ds, ds_msg);
		memset( us_msg, 0, sizeof(us_msg) );
		us_msg[0]=VTUO_MEDLEY_PSD_US_MAXSIZE;
		d->xdsl_msg_get_array(GetPSD_MD_Us, us_msg);
	}

	i=0;
	while( (i<VTUO_MEDLEY_PSD_DS_MAXSIZE)||(i<VTUO_MEDLEY_PSD_US_MAXSIZE) )
	{
		boaWrite(wp, FMT_STAT_PSD_START, i );

		if(i<VTUO_MEDLEY_PSD_DS_MAXSIZE)
		{
			boaWrite(wp, FMT_STAT_PSD_MID_1);
			boaWrite(wp, "%d", i+1);
			boaWrite(wp, FMT_STAT_PSD_MID_2);
			if( (isShowtime) && (i<ds_msg[0]) )
			{
				tmpD=(double)ds_msg[2+i*2]/10;
				boaWrite(wp, "<td>%u</td><td>%.1f</td>", ds_msg[1+i*2], tmpD);
			}else{
				boaWrite(wp, "<td>---</td><td>---</td>");
			}
		}else{
			boaWrite(wp, FMT_STAT_PSD_MID_1);
			boaWrite(wp, FMT_STAT_PSD_MID_2);
			boaWrite(wp, "<td></td><td></td>");
		}

		if(i<VTUO_MEDLEY_PSD_US_MAXSIZE)
		{
			boaWrite(wp, FMT_STAT_PSD_MID_1);
			boaWrite(wp, "%d", i+1);
			boaWrite(wp, FMT_STAT_PSD_MID_2);
			if( (isShowtime) && (i<us_msg[0]) )
			{
				tmpD=(double)us_msg[2+i*2]/10;
				boaWrite(wp, "<td>%u</td><td>%.1f</td>", us_msg[1+i*2], tmpD);
			}else{
				boaWrite(wp, "<td>---</td><td>---</td>");
			}
		}else{
			boaWrite(wp, FMT_STAT_PSD_MID_1);
			boaWrite(wp, FMT_STAT_PSD_MID_2);
			boaWrite(wp, "<td></td><td></td>");
		}

		boaWrite(wp, FMT_STAT_PSD_END);

		i++;
	}

	return;
}

int vtuo_checkWrite(int eid, request * wp, int argc, char **argv)
{
	char *name;
	unsigned char tmpUChar;

	//printf( "%s: enter\n", __FUNCTION__ );
   	if (boaArgs(argc, argv, "%s", &name) < 1) {
   		boaError(wp, 400, "Insufficient args\n");
		printf( "%s: error, line=%d\n", __FUNCTION__, __LINE__ );
   		return -1;
   	}
	//printf( "%s: name=%s\n", __FUNCTION__, name );

	if(!strcmp(name, "chan-init")) {
		vtuo_chan_init(wp);
		return 0;
	}else if(!strcmp(name, "ginp-init")) {
		vtuo_ginp_init(wp);
		return 0;
	}else if(!strcmp(name, "line-init")) {
		vtuo_line_init(wp);
		return 0;
	}else if(!strcmp(name, "inm-init")) {
		vtuo_inm_init(wp);
		return 0;
	}else if(!strcmp(name, "sra-init")) {
		vtuo_sra_init(wp);
		return 0;
	}else if(!strcmp(name, "dpbo-init")) {
		vtuo_dpbo_init(wp);
		return 0;
	}else if(!strcmp(name, "dpbo-init-tbl")) {
		vtuo_dpbo_init_tbl(wp);
		return 0;
	}else if(!strcmp(name, "psd-init-tbl")) {
		vtuo_psd_init_tbl(wp);
		return 0;
	/*
	}else if(!strcmp(name, "vn-init-tbl")) {
		vtuo_vn_init_tbl(wp);
		return 0;
	}else if(!strcmp(name, "vn-init")) {
		vtuo_vn_init(wp);
		return 0;
	}else if(!strcmp(name, "rfi-init-tbl")) {
		vtuo_rfi_init_tbl(wp);
		return 0;
	*/
	}else if(!strcmp(name, "stat-band")) {
		vtuo_stat_band(wp);
		return 0;
	}else if(!strcmp(name, "stat-perform")) {
		vtuo_stat_perform(wp);
		return 0;
	}else if(!strcmp(name, "stat-medley-psd")) {
		vtuo_stat_medley_psd(wp);
		return 0;
	}else{
		printf( "%s: error, line=%d, name=%s\n", __FUNCTION__, __LINE__, name );
		return -1;
	}

	return 0;
}

static void formSetVTUO_Chan(request * wp, char *path, char *query)
{
	char *submitUrl;
	char *strVal;
	unsigned int tmpUInt;
	unsigned char tmpUChar;

	VTUO_PRT( "%s> enter\n", __FUNCTION__ );

	/*downstream*/
	strVal = boaGetVar(wp, "DSRateMax", "");
	if (strVal[0])
	{
		tmpUInt=atoi( strVal );
		VTUO_PRT( "DSRateMax=%u\n", tmpUInt );
		mib_set(VTUO_CHAN_DS_NDR_MAX, (void *)&tmpUInt);
	}

	strVal = boaGetVar(wp, "DSRateMin", "");
	if (strVal[0])
	{
		tmpUInt=atoi( strVal );
		VTUO_PRT( "DSRateMin=%u\n", tmpUInt );
		mib_set(VTUO_CHAN_DS_NDR_MIN, (void *)&tmpUInt);
	}

	strVal = boaGetVar(wp, "DSDelay", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "DSDelay=%u\n", tmpUChar );
		mib_set(VTUO_CHAN_DS_MAX_DELAY, (void *)&tmpUChar);
	}

	strVal = boaGetVar(wp, "DSINP", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "DSINP=%u\n", tmpUChar );
		mib_set(VTUO_CHAN_DS_MIN_INP, (void *)&tmpUChar);
	}

	strVal = boaGetVar(wp, "DSINP8", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "DSINP8=%u\n", tmpUChar );
		mib_set(VTUO_CHAN_DS_MIN_INP8, (void *)&tmpUChar);
	}

	strVal = boaGetVar(wp, "DSSOSRate", "");
	if (strVal[0])
	{
		tmpUInt=atoi( strVal );
		VTUO_PRT( "DSSOSRate=%u\n", tmpUInt );
		mib_set(VTUO_CHAN_DS_SOS_MDR, (void *)&tmpUInt);
	}


	/*upstream*/
	strVal = boaGetVar(wp, "USRateMax", "");
	if (strVal[0])
	{
		tmpUInt=atoi( strVal );
		VTUO_PRT( "USRateMax=%u\n", tmpUInt );
		mib_set(VTUO_CHAN_US_NDR_MAX, (void *)&tmpUInt);
	}

	strVal = boaGetVar(wp, "USRateMin", "");
	if (strVal[0])
	{
		tmpUInt=atoi( strVal );
		VTUO_PRT( "USRateMin=%u\n", tmpUInt );
		mib_set(VTUO_CHAN_US_NDR_MIN, (void *)&tmpUInt);
	}

	strVal = boaGetVar(wp, "USDelay", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "USDelay=%u\n", tmpUChar );
		mib_set(VTUO_CHAN_US_MAX_DELAY, (void *)&tmpUChar);
	}

	strVal = boaGetVar(wp, "USINP", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "USINP=%u\n", tmpUChar );
		mib_set(VTUO_CHAN_US_MIN_INP, (void *)&tmpUChar);
	}

	strVal = boaGetVar(wp, "USINP8", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "USINP8=%u\n", tmpUChar );
		mib_set(VTUO_CHAN_US_MIN_INP8, (void *)&tmpUChar);
	}

	strVal = boaGetVar(wp, "USSOSRate", "");
	if (strVal[0])
	{
		tmpUInt=atoi( strVal );
		VTUO_PRT( "USSOSRate=%u\n", tmpUInt );
		mib_set(VTUO_CHAN_US_SOS_MDR, (void *)&tmpUInt);
	}

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#ifdef APPLY_CHANGE
	VTUOSetupChan();
	adsl_drv_get(RLCM_MODEM_RETRAIN, NULL, 0);
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);

}

static void formSetVTUO_Ginp(request * wp, char *path, char *query)
{
	char *submitUrl;
	char *strVal;
	unsigned int tmpUInt;
	unsigned short tmpUShort;
	unsigned char tmpUChar;

	VTUO_PRT( "%s> enter\n", __FUNCTION__ );

	/*downstream*/
	strVal = boaGetVar(wp, "DSGinpMode", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "DSGinpMode=%u\n", tmpUChar );
		mib_set(VTUO_GINP_DS_MODE, (void *)&tmpUChar);
	}else
		tmpUChar=0;

	if(tmpUChar!=0) //enable
	{
		strVal = boaGetVar(wp, "DSThroMax", "");
		if (strVal[0])
		{
			tmpUInt=atoi( strVal );
			VTUO_PRT( "DSThroMax=%u\n", tmpUInt );
			mib_set(VTUO_GINP_DS_ET_MAX, (void *)&tmpUInt);
		}
		strVal = boaGetVar(wp, "DSThroMin", "");
		if (strVal[0])
		{
			tmpUInt=atoi( strVal );
			VTUO_PRT( "DSThroMin=%u\n", tmpUInt );
			mib_set(VTUO_GINP_DS_ET_MIN, (void *)&tmpUInt);
		}
		strVal = boaGetVar(wp, "DSNDRMax", "");
		if (strVal[0])
		{
			tmpUInt=atoi( strVal );
			VTUO_PRT( "DSNDRMax=%u\n", tmpUInt );
			mib_set(VTUO_GINP_DS_NDR_MAX, (void *)&tmpUInt);
		}
		strVal = boaGetVar(wp, "DSRatio", "");
		if (strVal[0])
		{
			tmpUShort=atoi( strVal );
			VTUO_PRT( "DSRatio=%u\n", tmpUShort );
			mib_set(VTUO_GINP_DS_SHINE_RATIO, (void *)&tmpUShort);
		}
		strVal = boaGetVar(wp, "DSThres", "");
		if (strVal[0])
		{
			tmpUChar=atoi( strVal );
			VTUO_PRT( "DSThres=%u\n", tmpUChar );
			mib_set(VTUO_GINP_DS_LEFTR_THRD, (void *)&tmpUChar);
		}
		strVal = boaGetVar(wp, "DSMaxDelay", "");
		if (strVal[0])
		{
			tmpUChar=atoi( strVal );
			VTUO_PRT( "DSMaxDelay=%u\n", tmpUChar );
			mib_set(VTUO_GINP_DS_MAX_DELAY, (void *)&tmpUChar);
		}
		strVal = boaGetVar(wp, "DSMinDelay", "");
		if (strVal[0])
		{
			tmpUChar=atoi( strVal );
			VTUO_PRT( "DSMinDelay=%u\n", tmpUChar );
			mib_set(VTUO_GINP_DS_MIN_DELAY, (void *)&tmpUChar);
		}
		strVal = boaGetVar(wp, "DSINP", "");
		if (strVal[0])
		{
			tmpUChar=atoi( strVal );
			VTUO_PRT( "DSINP=%u\n", tmpUChar );
			mib_set(VTUO_GINP_DS_MIN_INP, (void *)&tmpUChar);
		}
		strVal = boaGetVar(wp, "DSReinSym", "");
		if (strVal[0])
		{
			tmpUChar=atoi( strVal );
			VTUO_PRT( "DSReinSym=%u\n", tmpUChar );
			mib_set(VTUO_GINP_DS_REIN_SYM, (void *)&tmpUChar);
		}
		strVal = boaGetVar(wp, "DSReinFreq", "");
		if (strVal[0])
		{
			tmpUChar=atoi( strVal );
			VTUO_PRT( "DSReinFreq=%u\n", tmpUChar );
			mib_set(VTUO_GINP_DS_REIN_FREQ, (void *)&tmpUChar);
		}
	}

	/*Upstream*/
	strVal = boaGetVar(wp, "USGinpMode", "");
	if (strVal[0])
	{
		tmpUChar=(char)atoi( strVal );
		VTUO_PRT( "USGinpMode=%u\n", tmpUChar );
		mib_set(VTUO_GINP_US_MODE, (void *)&tmpUChar);
	}else
		tmpUChar=0;

	if(tmpUChar!=0) //enable
	{
		strVal = boaGetVar(wp, "USThroMax", "");
		if (strVal[0])
		{
			tmpUInt=atoi( strVal );
			VTUO_PRT( "USThroMax=%u\n", tmpUInt );
			mib_set(VTUO_GINP_US_ET_MAX, (void *)&tmpUInt);
		}
		strVal = boaGetVar(wp, "USThroMin", "");
		if (strVal[0])
		{
			tmpUInt=atoi( strVal );
			VTUO_PRT( "USThroMin=%u\n", tmpUInt );
			mib_set(VTUO_GINP_US_ET_MIN, (void *)&tmpUInt);
		}
		strVal = boaGetVar(wp, "USNDRMax", "");
		if (strVal[0])
		{
			tmpUInt=atoi( strVal );
			VTUO_PRT( "USNDRMax=%u\n", tmpUInt );
			mib_set(VTUO_GINP_US_NDR_MAX, (void *)&tmpUInt);
		}
		strVal = boaGetVar(wp, "USRatio", "");
		if (strVal[0])
		{
			tmpUShort=atoi( strVal );
			VTUO_PRT( "USRatio=%u\n", tmpUShort );
			mib_set(VTUO_GINP_US_SHINE_RATIO, (void *)&tmpUShort);
		}
		strVal = boaGetVar(wp, "USThres", "");
		if (strVal[0])
		{
			tmpUChar=atoi( strVal );
			VTUO_PRT( "USThres=%u\n", tmpUChar );
			mib_set(VTUO_GINP_US_LEFTR_THRD, (void *)&tmpUChar);
		}
		strVal = boaGetVar(wp, "USMaxDelay", "");
		if (strVal[0])
		{
			tmpUChar=atoi( strVal );
			VTUO_PRT( "USMaxDelay=%u\n", tmpUChar );
			mib_set(VTUO_GINP_US_MAX_DELAY, (void *)&tmpUChar);
		}
		strVal = boaGetVar(wp, "USMinDelay", "");
		if (strVal[0])
		{
			tmpUChar=atoi( strVal );
			VTUO_PRT( "USMinDelay=%u\n", tmpUChar );
			mib_set(VTUO_GINP_US_MIN_DELAY, (void *)&tmpUChar);
		}
		strVal = boaGetVar(wp, "USINP", "");
		if (strVal[0])
		{
			tmpUChar=atoi( strVal );
			VTUO_PRT( "USINP=%u\n", tmpUChar );
			mib_set(VTUO_GINP_US_MIN_INP, (void *)&tmpUChar);
		}
		strVal = boaGetVar(wp, "USReinSym", "");
		if (strVal[0])
		{
			tmpUChar=atoi( strVal );
			VTUO_PRT( "USReinSym=%u\n", tmpUChar );
			mib_set(VTUO_GINP_US_REIN_SYM, (void *)&tmpUChar);
		}
		strVal = boaGetVar(wp, "USReinFreq", "");
		if (strVal[0])
		{
			tmpUChar=atoi( strVal );
			VTUO_PRT( "USReinFreq=%u\n", tmpUChar );
			mib_set(VTUO_GINP_US_REIN_FREQ, (void *)&tmpUChar);
		}
	}

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#ifdef APPLY_CHANGE
	VTUOSetupGinp();
	adsl_drv_get(RLCM_MODEM_RETRAIN, NULL, 0);
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);

}

static void formSetVTUO_Line(request * wp, char *path, char *query)
{
	char *submitUrl;
	char *strVal;
	unsigned char tmpUChar;
	unsigned int tmpUInt;
	unsigned short tmpUShort;
	short tmpShort;
	double tmpD;

	VTUO_PRT( "%s> enter\n", __FUNCTION__ );

	tmpUShort=0;
	strVal = boaGetVar(wp, "Vd2Prof35b", "");
	if (strVal[0]) tmpUShort|=VDSL2_PROFILE_35B;
	strVal = boaGetVar(wp, "Vd2Prof30a", "");
	if (strVal[0]) tmpUShort|=VDSL2_PROFILE_30A;
	strVal = boaGetVar(wp, "Vd2Prof17a", "");
	if (strVal[0]) tmpUShort|=VDSL2_PROFILE_17A;
	strVal = boaGetVar(wp, "Vd2Prof12a", "");
	if (strVal[0]) tmpUShort|=VDSL2_PROFILE_12A;
	strVal = boaGetVar(wp, "Vd2Prof12b", "");
	if (strVal[0]) tmpUShort|=VDSL2_PROFILE_12B;
	strVal = boaGetVar(wp, "Vd2Prof8a", "");
	if (strVal[0]) tmpUShort|=VDSL2_PROFILE_8A;
	strVal = boaGetVar(wp, "Vd2Prof8b", "");
	if (strVal[0]) tmpUShort|=VDSL2_PROFILE_8B;
	strVal = boaGetVar(wp, "Vd2Prof8c", "");
	if (strVal[0]) tmpUShort|=VDSL2_PROFILE_8C;
	strVal = boaGetVar(wp, "Vd2Prof8d", "");
	if (strVal[0]) tmpUShort|=VDSL2_PROFILE_8D;
	VTUO_PRT( "Vd2Prof=0x%04x\n", tmpUShort );
	mib_set(VTUO_LINE_VDSL2_PROFILE, (void *)&tmpUShort);
	/*
	strVal = boaGetVar(wp, "DSMaxSNRNoLmt", "");
	if (strVal[0]) tmpUChar=1;
	else tmpUChar=0;
	VTUO_PRT( "DSMaxSNRNoLmt=%u\n", tmpUChar );
	mib_set(VTUO_LINE_DS_MAX_SNR_NOLMT, (void *)&tmpUChar);
	*/
	strVal = boaGetVar(wp, "DSMaxSNR", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "DSMaxSNR=%d\n", tmpShort );
		mib_set(VTUO_LINE_DS_MAX_SNR, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "DSTargetSNR", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "DSTargetSNR=%d\n", tmpShort );
		mib_set(VTUO_LINE_DS_TARGET_SNR, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "DSMinSNR", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "DSMinSNR=%d\n", tmpShort );
		mib_set(VTUO_LINE_DS_MIN_SNR, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "DSBitswap", "");
	if (strVal[0])
	{
		tmpUChar=atoi(strVal);
		VTUO_PRT( "DSBitswap=%d\n", tmpUChar );
		mib_set(VTUO_LINE_DS_BITSWAP, (void *)&tmpUChar);
	}
	/*
	strVal = boaGetVar(wp, "DSMaxTxPwr", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "DSMaxTxPwr=%d\n", tmpShort );
		mib_set(VTUO_LINE_DS_MAX_TXPWR, (void *)&tmpShort);
	}

	strVal = boaGetVar(wp, "DSMinOhRate", "");
	if (strVal[0])
	{
		tmpUShort=atoi(strVal);
		VTUO_PRT( "DSMinOhRate=%d\n", tmpUShort );
		mib_set(VTUO_LINE_DS_MIN_OH_RATE, (void *)&tmpUShort);
	}

	strVal = boaGetVar(wp, "USMaxSNRNoLmt", "");
	if (strVal[0]) tmpUChar=1;
	else tmpUChar=0;
	VTUO_PRT( "USMaxSNRNoLmt=%u\n", tmpUChar );
	mib_set(VTUO_LINE_US_MAX_SNR_NOLMT, (void *)&tmpUChar);
	*/
	strVal = boaGetVar(wp, "USMaxSNR", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "USMaxSNR=%d\n", tmpShort );
		mib_set(VTUO_LINE_US_MAX_SNR, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "USTargetSNR", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "USTargetSNR=%d\n", tmpShort );
		mib_set(VTUO_LINE_US_TARGET_SNR, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "USMinSNR", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "USMinSNR=%d\n", tmpShort );
		mib_set(VTUO_LINE_US_MIN_SNR, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "USBitswap", "");
	if (strVal[0])
	{
		tmpUChar=atoi(strVal);
		VTUO_PRT( "USBitswap=%d\n", tmpUChar );
		mib_set(VTUO_LINE_US_BITSWAP, (void *)&tmpUChar);
	}
	/*
	strVal = boaGetVar(wp, "USMaxRxPwrNoLmt", "");
	if (strVal[0]) tmpUChar=1;
	else tmpUChar=0;
	VTUO_PRT( "USMaxRxPwrNoLmt=%u\n", tmpUChar );
	mib_set(VTUO_LINE_US_MAX_RXPWR_NOLMT, (void *)&tmpUChar);
	strVal = boaGetVar(wp, "USMaxRxPwr", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "USMaxRxPwr=%d\n", tmpShort );
		mib_set(VTUO_LINE_US_MAX_RXPWR, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "USMaxTxPwr", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "USMaxTxPwr=%d\n", tmpShort );
		mib_set(VTUO_LINE_US_MAX_TXPWR, (void *)&tmpShort);
	}

	strVal = boaGetVar(wp, "USMinOhRate", "");
	if (strVal[0])
	{
		tmpUShort=atoi(strVal);
		VTUO_PRT( "USMinOhRate=%d\n", tmpUShort );
		mib_set(VTUO_LINE_US_MIN_OH_RATE, (void *)&tmpUShort);
	}
	strVal = boaGetVar(wp, "TransMode", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "TransMode=%u\n", tmpUChar );
		mib_set(VTUO_LINE_TRANS_MODE, (void *)&tmpUChar);
	}
	*/

	tmpUInt=0;
	strVal = boaGetVar(wp, "AdProtANSI", "");
	if (strVal[0]) tmpUInt|=MODE_ANSI;
	strVal = boaGetVar(wp, "AdProtETSI", "");
	if (strVal[0]) tmpUInt|=MODE_ETSI;
	strVal = boaGetVar(wp, "AdProtG9921", "");
	if (strVal[0]) tmpUInt|=MODE_GDMT;
	strVal = boaGetVar(wp, "AdProtG9922", "");
	if (strVal[0]) tmpUInt|=MODE_GLITE;
	strVal = boaGetVar(wp, "AdProtG9923", "");
	if (strVal[0]) tmpUInt|=MODE_ADSL2;
	strVal = boaGetVar(wp, "AdProtG9925", "");
	if (strVal[0]) tmpUInt|=MODE_ADSL2PLUS;
	VTUO_PRT( "AdProt=0x%08x\n", tmpUInt );
	mib_set(VTUO_LINE_ADSL_PROTOCOL, (void *)&tmpUInt);

	/*
	strVal = boaGetVar(wp, "ClassMask", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "ClassMask=%u\n", tmpUChar );
		mib_set(VTUO_LINE_CLASS_MASK, (void *)&tmpUChar);
	}
	*/
	strVal = boaGetVar(wp, "LimitMask", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "LimitMask=%u\n", tmpUChar );
		mib_set(VTUO_LINE_LIMIT_MASK, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "US0Mask", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "US0Mask=%u\n", tmpUChar );
		mib_set(VTUO_LINE_US0_MASK, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "UPBO", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "UPBO=%u\n", tmpUChar );
		mib_set(VTUO_LINE_UPBO_ENABLE, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "UPBOKL", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "UPBOKL=%d\n", tmpShort );
		mib_set(VTUO_LINE_UPBOKL, (void *)&tmpShort);
	}

	strVal = boaGetVar(wp, "USBand1a", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*100);
		VTUO_PRT( "USBand1a=%d\n", tmpShort );
		mib_set(VTUO_LINE_UPBO_1A, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "USBand2a", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*100);
		VTUO_PRT( "USBand2a=%d\n", tmpShort );
		mib_set(VTUO_LINE_UPBO_2A, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "USBand3a", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*100);
		VTUO_PRT( "USBand3a=%d\n", tmpShort );
		mib_set(VTUO_LINE_UPBO_3A, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "USBand4a", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*100);
		VTUO_PRT( "USBand4a=%d\n", tmpShort );
		mib_set(VTUO_LINE_UPBO_4A, (void *)&tmpShort);
	}

	strVal = boaGetVar(wp, "USBand1b", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*100);
		VTUO_PRT( "USBand1b=%d\n", tmpShort );
		mib_set(VTUO_LINE_UPBO_1B, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "USBand2b", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*100);
		VTUO_PRT( "USBand2b=%d\n", tmpShort );
		mib_set(VTUO_LINE_UPBO_2B, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "USBand3b", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*100);
		VTUO_PRT( "USBand3b=%d\n", tmpShort );
		mib_set(VTUO_LINE_UPBO_3B, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "USBand4b", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*100);
		VTUO_PRT( "USBand4b=%d\n", tmpShort );
		mib_set(VTUO_LINE_UPBO_4B, (void *)&tmpShort);
	}
	/*
	strVal = boaGetVar(wp, "RTMode", "");
	if (strVal[0]) tmpUChar=1;
	else tmpUChar=0;
	VTUO_PRT( "RTMode=%u\n", tmpUChar );
	mib_set(VTUO_LINE_RT_MODE, (void *)&tmpUChar);
	*/
	strVal = boaGetVar(wp, "US0", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "US0=%u\n", tmpUChar );
		mib_set(VTUO_LINE_US0_ENABLE, (void *)&tmpUChar);
	}

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#ifdef APPLY_CHANGE
	VTUOSetupLine();
	adsl_drv_get(RLCM_MODEM_RETRAIN, NULL, 0);
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);

}

static void formSetVTUO_Inm(request * wp, char *path, char *query)
{
	char *submitUrl;
	char *strVal;
	unsigned char tmpUChar;
	short tmpShort;
	double tmpD;

	VTUO_PRT( "%s> enter\n", __FUNCTION__ );

	strVal = boaGetVar(wp, "NEInpEqMode", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "NEInpEqMode=%u\n", tmpUChar );
		mib_set(VTUO_INM_NE_INP_EQ_MODE, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "NEInmCc", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "NEInmCc=%u\n", tmpUChar );
		mib_set(VTUO_INM_NE_INMCC, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "NEIatOff", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "NEIatOff=%u\n", tmpUChar );
		mib_set(VTUO_INM_NE_IAT_OFFSET, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "NEIatSet", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "NEIatSet=%u\n", tmpUChar );
		mib_set(VTUO_INM_NE_IAT_SETUP, (void *)&tmpUChar);
	}
	/*
	strVal = boaGetVar(wp, "NEIsddSen", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "NEIsddSen=%d\n", tmpShort );
		mib_set(VTUO_INM_NE_ISDD_SEN, (void *)&tmpShort);
	}
	*/
	strVal = boaGetVar(wp, "FEInpEqMode", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "FEInpEqMode=%u\n", tmpUChar );
		mib_set(VTUO_INM_FE_INP_EQ_MODE, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "FEInmCc", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "FEInmCc=%u\n", tmpUChar );
		mib_set(VTUO_INM_FE_INMCC, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "FEIatOff", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "FEIatOff=%u\n", tmpUChar );
		mib_set(VTUO_INM_FE_IAT_OFFSET, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "FEIatSet", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "FEIatSet=%u\n", tmpUChar );
		mib_set(VTUO_INM_FE_IAT_SETUP, (void *)&tmpUChar);
	}
	/*
	strVal = boaGetVar(wp, "FEIsddSen", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "FEIsddSen=%d\n", tmpShort );
		mib_set(VTUO_INM_FE_ISDD_SEN, (void *)&tmpShort);
	}
	*/
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#ifdef APPLY_CHANGE
	VTUOSetupInm();
	adsl_drv_get(RLCM_MODEM_RETRAIN, NULL, 0);
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);

}

static void formSetVTUO_Sra(request * wp, char *path, char *query)
{
	char *submitUrl;
	char *strVal;
	unsigned char tmpUChar;
	unsigned short tmpUShort;
	short tmpShort;
	double tmpD;

	VTUO_PRT( "%s> enter\n", __FUNCTION__ );

	strVal = boaGetVar(wp, "DSRateAdapt", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "DSRateAdapt=%u\n", tmpUChar );
		mib_set(VTUO_SRA_DS_RA_MODE, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "DSDynDep", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "DSDynDep=%u\n", tmpUChar );
		mib_set(VTUO_SRA_DS_DYNAMIC_DEPTH, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "DSUpShiftSNR", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "DSUpShiftSNR=%d\n", tmpShort );
		mib_set(VTUO_SRA_DS_USHIFT_SNR, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "DSUpShiftTime", "");
	if (strVal[0])
	{
		tmpUShort=atoi( strVal );
		VTUO_PRT( "DSUpShiftTime=%u\n", tmpUShort );
		mib_set(VTUO_SRA_DS_USHIFT_TIME, (void *)&tmpUShort);
	}
	strVal = boaGetVar(wp, "DSDownShiftSNR", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "DSDownShiftSNR=%d\n", tmpShort );
		mib_set(VTUO_SRA_DS_DSHIFT_SNR, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "DSDownShiftTime", "");
	if (strVal[0])
	{
		tmpUShort=atoi( strVal );
		VTUO_PRT( "DSDownShiftTime=%u\n", tmpUShort );
		mib_set(VTUO_SRA_DS_DSHIFT_TIME, (void *)&tmpUShort);
	}

	strVal = boaGetVar(wp, "DSSosTime", "");
	if (strVal[0])
	{
		tmpUShort=atoi( strVal );
		VTUO_PRT( "DSSosTime=%u\n", tmpUShort );
		mib_set(VTUO_SRA_DS_SOS_TIME, (void *)&tmpUShort);
	}
	strVal = boaGetVar(wp, "DSSosCrc", "");
	if (strVal[0])
	{
		tmpUShort=atoi( strVal );
		VTUO_PRT( "DSSosCrc=%u\n", tmpUShort );
		mib_set(VTUO_SRA_DS_SOS_CRC, (void *)&tmpUShort);
	}
	strVal = boaGetVar(wp, "DSSosnTones", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "DSSosnTones=%u\n", tmpUChar );
		mib_set(VTUO_SRA_DS_SOS_NTONE, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "DSSosMax", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "DSSosMax=%u\n", tmpUChar );
		mib_set(VTUO_SRA_DS_SOS_MAX, (void *)&tmpUChar);
	}
	/*
	strVal = boaGetVar(wp, "DSSosMultiStep", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "DSSosMultiStep=%u\n", tmpUChar );
		mib_set(VTUO_SRA_DS_SOS_MSTEP_TONE, (void *)&tmpUChar);
	}
	*/
	strVal = boaGetVar(wp, "DSRocEnable", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "DSRocEnable=%u\n", tmpUChar );
		mib_set(VTUO_SRA_DS_ROC_ENABLE, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "DSRocSNR", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "DSRocSNR=%d\n", tmpShort );
		mib_set(VTUO_SRA_DS_ROC_SNR, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "DSRocMinINP", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "DSRocMinINP=%u\n", tmpUChar );
		mib_set(VTUO_SRA_DS_ROC_MIN_INP, (void *)&tmpUChar);
	}



	strVal = boaGetVar(wp, "USRateAdapt", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "USRateAdapt=%u\n", tmpUChar );
		mib_set(VTUO_SRA_US_RA_MODE, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "USDynDep", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "USDynDep=%u\n", tmpUChar );
		mib_set(VTUO_SRA_US_DYNAMIC_DEPTH, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "USUpShiftSNR", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "USUpShiftSNR=%d\n", tmpShort );
		mib_set(VTUO_SRA_US_USHIFT_SNR, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "USUpShiftTime", "");
	if (strVal[0])
	{
		tmpUShort=atoi( strVal );
		VTUO_PRT( "USUpShiftTime=%u\n", tmpUShort );
		mib_set(VTUO_SRA_US_USHIFT_TIME, (void *)&tmpUShort);
	}
	strVal = boaGetVar(wp, "USDownShiftSNR", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "USDownShiftSNR=%d\n", tmpShort );
		mib_set(VTUO_SRA_US_DSHIFT_SNR, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "USDownShiftTime", "");
	if (strVal[0])
	{
		tmpUShort=atoi( strVal );
		VTUO_PRT( "USDownShiftTime=%u\n", tmpUShort );
		mib_set(VTUO_SRA_US_DSHIFT_TIME, (void *)&tmpUShort);
	}

	strVal = boaGetVar(wp, "USSosTime", "");
	if (strVal[0])
	{
		tmpUShort=atoi( strVal );
		VTUO_PRT( "USSosTime=%u\n", tmpUShort );
		mib_set(VTUO_SRA_US_SOS_TIME, (void *)&tmpUShort);
	}
	strVal = boaGetVar(wp, "USSosCrc", "");
	if (strVal[0])
	{
		tmpUShort=atoi( strVal );
		VTUO_PRT( "USSosCrc=%u\n", tmpUShort );
		mib_set(VTUO_SRA_US_SOS_CRC, (void *)&tmpUShort);
	}
	strVal = boaGetVar(wp, "USSosnTones", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "USSosnTones=%u\n", tmpUChar );
		mib_set(VTUO_SRA_US_SOS_NTONE, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "USSosMax", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "USSosMax=%u\n", tmpUChar );
		mib_set(VTUO_SRA_US_SOS_MAX, (void *)&tmpUChar);
	}
	/*
	strVal = boaGetVar(wp, "USSosMultiStep", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "USSosMultiStep=%u\n", tmpUChar );
		mib_set(VTUO_SRA_US_SOS_MSTEP_TONE, (void *)&tmpUChar);
	}
	*/
	strVal = boaGetVar(wp, "USRocEnable", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "USRocEnable=%u\n", tmpUChar );
		mib_set(VTUO_SRA_US_ROC_ENABLE, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "USRocSNR", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "USRocSNR=%d\n", tmpShort );
		mib_set(VTUO_SRA_US_ROC_SNR, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "USRocMinINP", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "USRocMinINP=%u\n", tmpUChar );
		mib_set(VTUO_SRA_US_ROC_MIN_INP, (void *)&tmpUChar);
	}

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#ifdef APPLY_CHANGE
	VTUOSetupSra();
	adsl_drv_get(RLCM_MODEM_RETRAIN, NULL, 0);
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);

}


static void formSetVTUO_Dpbo(request * wp, char *path, char *query)
{
	char *submitUrl;
	char *strVal;
	unsigned char tmpUChar;
	unsigned short tmpUShort;
	short tmpShort;
	int tmpInt;
	double tmpD;

	VTUO_PRT( "%s> enter\n", __FUNCTION__ );

	strVal = boaGetVar(wp, "DpboEnable", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "DpboEnable=%u\n", tmpUChar );
		mib_set(VTUO_DPBO_ENABLE, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "DpboESel", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "DpboESel=%d\n", tmpShort );
		mib_set(VTUO_DPBO_ESEL, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "DpboEScma", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpInt= FTOI(tmpD*10000);
		VTUO_PRT( "DpboEScma=%d\n", tmpInt );
		mib_set(VTUO_DPBO_ESCMA, (void *)&tmpInt);
	}
	strVal = boaGetVar(wp, "DpboEScmb", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpInt= FTOI(tmpD*10000);
		VTUO_PRT( "DpboEScmb=%d\n", tmpInt );
		mib_set(VTUO_DPBO_ESCMB, (void *)&tmpInt);
	}
	strVal = boaGetVar(wp, "DpboEScmc", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpInt= FTOI(tmpD*10000);
		VTUO_PRT( "DpboEScmc=%d\n", tmpInt );
		mib_set(VTUO_DPBO_ESCMC, (void *)&tmpInt);
	}
	strVal = boaGetVar(wp, "DpboMus", "");
	if (strVal[0])
	{
		tmpD=atof(strVal);
		tmpShort= FTOI(tmpD*10);
		VTUO_PRT( "DpboMus=%d\n", tmpShort );
		mib_set(VTUO_DPBO_MUS, (void *)&tmpShort);
	}
	strVal = boaGetVar(wp, "DpboFMin", "");
	if (strVal[0])
	{
		tmpUShort=atoi( strVal );
		VTUO_PRT( "DpboFMin=%u\n", tmpUShort );
		mib_set(VTUO_DPBO_FMIN, (void *)&tmpUShort);
	}
	strVal = boaGetVar(wp, "DpboFMax", "");
	if (strVal[0])
	{
		tmpUShort=atoi( strVal );
		VTUO_PRT( "DpboFMax=%u\n", tmpUShort );
		mib_set(VTUO_DPBO_FMAX, (void *)&tmpUShort);
	}


{
	int	i, max=16;
	MIB_CE_VTUO_DPBO_T entry, *p=&entry;
	unsigned short pre=0;

	mib_chain_clear(MIB_VTUO_DPBO_TBL);
	for(i=0; i<max; i++)
	{
		char name[16];

		sprintf( name, "DpboToneId%d", i+1 );
		strVal = boaGetVar(wp, name, "");
		p->ToneId=atoi( strVal );
		VTUO_PRT( "%s=%u\n", name, p->ToneId );

		sprintf( name, "DpboPsd%d", i+1 );
		strVal = boaGetVar(wp, name, "");
		tmpD=atof(strVal);
		p->PsdLevel= FTOI(tmpD*10);
		VTUO_PRT( "%s=%d\n", name, p->PsdLevel );

		if( (p->ToneId==0) && (p->PsdLevel==0) )
			break;
		if( (i!=0) && (pre>=p->ToneId) )
			break;
		mib_chain_add(MIB_VTUO_DPBO_TBL, p);
		pre=p->ToneId;
	}

}


#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#ifdef APPLY_CHANGE
	VTUOSetupDpbo();
	adsl_drv_get(RLCM_MODEM_RETRAIN, NULL, 0);
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);

}


static void formSetVTUO_Psd(request * wp, char *path, char *query)
{
	char *submitUrl;
	char *strVal;
	double tmpD;
	int	i, max;
	MIB_CE_VTUO_PSD_T entry, *p=&entry;
	unsigned short pre=0;

	mib_chain_clear(MIB_VTUO_PSD_DS_TBL);
	max=32;
	for(i=0; i<max; i++)
	{
		char name[16];

		sprintf( name, "ToneIdDs%d", i+1 );
		strVal = boaGetVar(wp, name, "");
		p->ToneId=atoi( strVal );
		VTUO_PRT( "%s=%u\n", name, p->ToneId );

		sprintf( name, "PsdDs%d", i+1 );
		strVal = boaGetVar(wp, name, "");
		tmpD=atof(strVal);
		p->PsdLevel= FTOI(tmpD*10);
		VTUO_PRT( "%s=%d\n", name, p->PsdLevel );

		if( (p->ToneId==0) && (p->PsdLevel==0) )
			break;
		if( (i!=0) && (pre>=p->ToneId) )
			break;
		mib_chain_add(MIB_VTUO_PSD_DS_TBL, p);
		pre=p->ToneId;
	}

	mib_chain_clear(MIB_VTUO_PSD_US_TBL);
	max=16;
	for(i=0; i<max; i++)
	{
		char name[16];

		sprintf( name, "ToneIdUs%d", i+1 );
		strVal = boaGetVar(wp, name, "");
		p->ToneId=atoi( strVal );
		VTUO_PRT( "%s=%u\n", name, p->ToneId );

		sprintf( name, "PsdUs%d", i+1 );
		strVal = boaGetVar(wp, name, "");
		tmpD=atof(strVal);
		p->PsdLevel= FTOI(tmpD*10);
		VTUO_PRT( "%s=%d\n", name, p->PsdLevel );

		if( (p->ToneId==0) && (p->PsdLevel==0) )
			break;
		if( (i!=0) && (pre>=p->ToneId) )
			break;
		mib_chain_add(MIB_VTUO_PSD_US_TBL, p);
		pre=p->ToneId;
	}

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#ifdef APPLY_CHANGE
	VTUOSetupPsd();
	adsl_drv_get(RLCM_MODEM_RETRAIN, NULL, 0);
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);

}

/*
static void formSetVTUO_Vn(request * wp, char *path, char *query)
{
	char *submitUrl;
	char *strVal;
	unsigned char tmpUChar;
	double tmpD;
	int	i, max;
	MIB_CE_VTUO_VN_T entry, *p=&entry;
	unsigned short pre=0;

	strVal = boaGetVar(wp, "DSVnEnable", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "DSVnEnable=%u\n", tmpUChar );
		mib_set(VTUO_VN_DS_ENABLE, (void *)&tmpUChar);
	}
	strVal = boaGetVar(wp, "USVnEnable", "");
	if (strVal[0])
	{
		tmpUChar=atoi( strVal );
		VTUO_PRT( "USVnEnable=%u\n", tmpUChar );
		mib_set(VTUO_VN_US_ENABLE, (void *)&tmpUChar);
	}

	mib_chain_clear(MIB_VTUO_VN_DS_TBL);
	max=32;
	for(i=0; i<max; i++)
	{
		char name[16];

		sprintf( name, "ToneIdDs%d", i+1 );
		strVal = boaGetVar(wp, name, "");
		p->ToneId=atoi( strVal );
		VTUO_PRT( "%s=%u\n", name, p->ToneId );

		sprintf( name, "PsdDs%d", i+1 );
		strVal = boaGetVar(wp, name, "");
		tmpD=atof(strVal);
		p->NoiseLevel= FTOI(tmpD*10);
		VTUO_PRT( "%s=%d\n", name, p->NoiseLevel );

		if( (p->ToneId==0) && (p->NoiseLevel==0) )
			break;
		if( (i!=0) && (pre>=p->ToneId) )
			break;
		mib_chain_add(MIB_VTUO_VN_DS_TBL, p);
		pre=p->ToneId;
	}

	mib_chain_clear(MIB_VTUO_VN_US_TBL);
	max=16;
	for(i=0; i<max; i++)
	{
		char name[16];

		sprintf( name, "ToneIdUs%d", i+1 );
		strVal = boaGetVar(wp, name, "");
		p->ToneId=atoi( strVal );
		VTUO_PRT( "%s=%u\n", name, p->ToneId );

		sprintf( name, "PsdUs%d", i+1 );
		strVal = boaGetVar(wp, name, "");
		tmpD=atof(strVal);
		p->NoiseLevel= FTOI(tmpD*10);
		VTUO_PRT( "%s=%d\n", name, p->NoiseLevel );

		if( (p->ToneId==0) && (p->NoiseLevel==0) )
			break;
		if( (i!=0) && (pre>=p->ToneId) )
			break;
		mib_chain_add(MIB_VTUO_VN_US_TBL, p);
		pre=p->ToneId;
	}

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#ifdef APPLY_CHANGE
	VTUOSetupVn();
	adsl_drv_get(RLCM_MODEM_RETRAIN, NULL, 0);
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);

}

static void formSetVTUO_Rfi(request * wp, char *path, char *query)
{
	char *submitUrl;
	char *strVal;
	double tmpD;
	int	i, max;
	MIB_CE_VTUO_RFI_T entry, *p=&entry;
	unsigned short pre=0;

	mib_chain_clear(MIB_VTUO_RFI_TBL);
	max=16;
	for(i=0; i<max; i++)
	{
		char name[16];

		sprintf( name, "ToneId%d", i+1 );
		strVal = boaGetVar(wp, name, "");
		p->ToneId=atoi( strVal );
		VTUO_PRT( "%s=%u\n", name, p->ToneId );

		sprintf( name, "ToneIdEnd%d", i+1 );
		strVal = boaGetVar(wp, name, "");
		p->ToneIdEnd=atoi( strVal );
		VTUO_PRT( "%s=%d\n", name, p->ToneIdEnd );

		if( (p->ToneId==0) && (p->ToneIdEnd==0) )
			break;
		if( p->ToneId > p->ToneIdEnd )
			break;
		if( (i!=0) && (pre>=p->ToneId) )
			break;
		mib_chain_add(MIB_VTUO_RFI_TBL, p);
		pre=p->ToneIdEnd;
	}

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#ifdef APPLY_CHANGE
	VTUOSetupRfi();
	adsl_drv_get(RLCM_MODEM_RETRAIN, NULL, 0);
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);

}
*/

#define FMT_VTUO_HEAD "<head><title>VTU-O DSL Statistics</title></head>\n"
#define FMT_VTUO_BODY "<body>\n"
#define FMT_VTUO_BODY_END "</body>\n"
static void formSetVTUO_Status_History(request * wp, int oneday)
{
	int i,j,i_loop;
	XDSL_OP *d=xdsl_get_op(0);
	int tmp2dsl[27];

	boaHeader(wp);
	boaWrite(wp, FMT_VTUO_HEAD);
	boaWrite(wp, FMT_VTUO_BODY);
	boaWrite(wp, "<h4>* %s Interval</h4>\n", oneday?"One-Day":"15-Min");

	boaWrite(wp, "<table border=0 cellspacing=4 cellpadding=0>\n");
	boaWrite(wp, "<tr align=center bgcolor=#c0c0c0>\n");
	boaWrite(wp, "<th colspan=3></th>\n");
	boaWrite(wp, "<th bgcolor=#f0f0f0>&nbsp;&nbsp;</th>\n");
	boaWrite(wp, "<th colspan=11>VTUR</th>\n");
	boaWrite(wp, "<th bgcolor=#f0f0f0>&nbsp;&nbsp;</th>\n");
	boaWrite(wp, "<th colspan=11>VTUO</th>\n");
	boaWrite(wp, "<th bgcolor=#f0f0f0>&nbsp;&nbsp;</th>\n");
	boaWrite(wp, "<th></th>\n");
	boaWrite(wp, "</tr>\n\n");

	boaWrite(wp, "<tr align=center bgcolor=#c0c0c0>\n");
	boaWrite(wp, "<th>Index</th>\n");
	boaWrite(wp, "<th>Full Inits</th>\n");
	boaWrite(wp, "<th>Failed Full<br>Inits</th>\n");
	for(i=0;i<2;i++)
	{
		char *tbuf[]={"ES", "SES", "LOSs", "LOFs", "UAS",
				"CV/CRC", "LEFTRs", "MinEFTR", "ErrFreeBits", "LOL",
				"LPR", NULL };
		boaWrite(wp, "<th bgcolor=#f0f0f0>&nbsp;&nbsp;</th>\n");
		for(j=0;tbuf[j]!=NULL;j++)
			boaWrite(wp, "<th>%s</th>\n", tbuf[j]);
	}
	boaWrite(wp, "<th bgcolor=#f0f0f0>&nbsp;&nbsp;</th>\n");
	boaWrite(wp, "<th>Index</th>\n");
	boaWrite(wp, "</tr>\n\n");

	if(oneday) i_loop=7;
	else i_loop=96;
	for(i=0;i<i_loop;i++)
	{
		boaWrite(wp, "<tr align=center bgcolor=#f0f0f0>\n");
		boaWrite(wp, "<td>%d</td>\n", i+1);

		//ds
		memset(tmp2dsl, 0, sizeof(tmp2dsl) );
		tmp2dsl[0]=i;
		if(oneday) d->xdsl_msg_get_array( GetEvCnt1DayDs_e127, tmp2dsl );
		else d->xdsl_msg_get_array( GetEvCnt15MinDs_e127, tmp2dsl );
		boaWrite(wp, "<td>%u</td>\n", tmp2dsl[1]);
		boaWrite(wp, "<td>%u</td>\n", tmp2dsl[2]);
		boaWrite(wp, "<td></td>\n");
		for(j=4;j<18;j++) {
			/* skip some elements */
			if (j >= 10 && j <= 14)
				continue;
			boaWrite(wp, "<td>%u</td>\n", tmp2dsl[j]);
		}
		boaWrite(wp, "<td>%u</td>\n", tmp2dsl[19]);
		boaWrite(wp, "<td>%u</td>\n", tmp2dsl[21]);

		//up
		memset(tmp2dsl, 0, sizeof(tmp2dsl) );
		tmp2dsl[0]=i;
		if(oneday) d->xdsl_msg_get_array( GetEvCnt1DayUs_e127, tmp2dsl );
		else d->xdsl_msg_get_array( GetEvCnt15MinUs_e127, tmp2dsl );
		boaWrite(wp, "<td></td>\n");
		for(j=4;j<18;j++) {
			/* skip some elements */
			if (j >= 10 && j <= 14)
				continue;
			boaWrite(wp, "<td>%u</td>\n", tmp2dsl[j]);
		}
		boaWrite(wp, "<td>%u</td>\n", tmp2dsl[19]);
		boaWrite(wp, "<td>%u</td>\n", tmp2dsl[21]);

		boaWrite(wp, "<td></td>\n");
		boaWrite(wp, "<td>%d</td>\n", i+1);
		boaWrite(wp, "</tr>\n\n");
	}

	boaWrite(wp, "</table>\n");
	boaWrite(wp, "<br><br>\n");

	boaWrite(wp, "<h4>* Impulse Noise Moniter %s Interval</h4>\n", oneday?"One-Day":"15-Min");
	boaWrite(wp, "<table border=0 cellspacing=4 cellpadding=0>\n");
	boaWrite(wp, "<tr align=center bgcolor=#c0c0c0>\n");
	boaWrite(wp, "<th bgcolor=#f0f0f0></th>\n");
	boaWrite(wp, "<th colspan=26>VTUR</th>\n");
	boaWrite(wp, "<th bgcolor=#f0f0f0></th>\n");
	boaWrite(wp, "<th colspan=26>VTUO</th>\n");
	boaWrite(wp, "<th bgcolor=#f0f0f0></th>\n");
	boaWrite(wp, "</tr>\n\n");

	boaWrite(wp, "<tr align=center bgcolor=#c0c0c0>\n");
	for(i=0;i<2;i++)
	{
		boaWrite(wp, "<th bgcolor=#f0f0f0>Index</th>\n");
		for(j=1;j<18;j++) boaWrite(wp, "<th>INMEQ%d</th>\n", j);
		for(j=0;j<8;j++) boaWrite(wp, "<th>IAT%d</th>\n", j);
		boaWrite(wp, "<th>Symbol</th>\n");
	}
	boaWrite(wp, "<th bgcolor=#f0f0f0>Index</th>\n");
	boaWrite(wp, "</tr>\n\n");

	if(oneday) i_loop=7;
	else i_loop=96;
	for(i=0;i<i_loop;i++)
	{
		boaWrite(wp, "<tr align=center bgcolor=#f0f0f0>\n");

		boaWrite(wp, "<td>%d</td>\n", i+1);
		memset(tmp2dsl, 0, sizeof(tmp2dsl) );
		tmp2dsl[0]=i;
		if(oneday) d->xdsl_msg_get_array( GetInmCnt1DayDs_e127, tmp2dsl );
		else d->xdsl_msg_get_array( GetInmCnt15MinDs_e127, tmp2dsl );
		for(j=1;j<27;j++) boaWrite(wp, "<td>%u</td>\n", tmp2dsl[j]);

		boaWrite(wp, "<td>%d</td>\n", i+1);
		memset(tmp2dsl, 0, sizeof(tmp2dsl) );
		tmp2dsl[0]=i;
		if(oneday) d->xdsl_msg_get_array( GetInmCnt1DayUs_e127, tmp2dsl );
		else d->xdsl_msg_get_array( GetInmCnt15MinUs_e127, tmp2dsl );
		for(j=1;j<27;j++) boaWrite(wp, "<td>%u</td>\n", tmp2dsl[j]);

		boaWrite(wp, "<td>%d</td>\n", i+1);
		boaWrite(wp, "</tr>\n\n");
	}

	boaWrite(wp, "</table>\n");

	boaWrite(wp, FMT_VTUO_BODY_END);
	boaFooter(wp);
	boaDone(wp, 200);
}
/*
static void formSetVTUO_Status_SubCarrier(request * wp, int item, int ds )
{
	XDSL_OP *d=xdsl_get_op(0);
	int chan, i, isVDSL2, ret, offset;
	int start_tone, end_tone;
	int mval=0, okflag=1;
	short *sbuf;
	int sbuf_size;

	// get the channel number
	chan=256;
	isVDSL2=0;
	if(d->xdsl_msg_get(GetPmdMode,&mval))
	{
		if(mval&MODE_VDSL2)
			isVDSL2=1;
		else
			isVDSL2=0;

		if(mval<MODE_ADSL2PLUS)
			chan=256;
		else
			chan=512;
	}

	if(isVDSL2==0)
	{
		int tmp2dsl[11];
		memset( tmp2dsl, 0, sizeof(tmp2dsl) );
		if(ds)
			d->xdsl_msg_get_array( GetDsBand, tmp2dsl );
		else
			d->xdsl_msg_get_array( GetUsBand, tmp2dsl );

		start_tone= tmp2dsl[1];
		end_tone= tmp2dsl[2];
		printf( "%s: n=%d, start=%d, end=%d\n", __FUNCTION__, tmp2dsl[0], start_tone, end_tone );
	}

	sbuf_size=sizeof(short)*MAX_DSL_TONE*2;
	sbuf = malloc(sbuf_size);
	if(sbuf)
	{
		memset( sbuf, 0,  sbuf_size );
		ret=0;
		if(item==0)
			ret = d->xdsl_drv_get(RLCM_GET_VDSL2_DIAG_HLOG, (void *)sbuf, sbuf_size);
		else if(item==1)
			ret = d->xdsl_drv_get(RLCM_GET_VDSL2_DIAG_QLN, (void *)sbuf, sbuf_size);
		else if(item==2)
			ret = d->xdsl_drv_get(RLCM_GET_VDSL2_DIAG_SNR, (void *)sbuf, sbuf_size);
		if(!ret)
		{
			printf( "%s: RLCM_GET_VDSL2_DIAG_HLOG/QLN/SNR failed\n", __FUNCTION__ );
			okflag=0;
		}
	}else{
		printf( "%s: malloc failed\n", __FUNCTION__ );
		okflag=0;
	}

	if(ds)
		offset=MAX_DSL_TONE;
	else
		offset=0;

	boaHeader(wp);
	boaWrite(wp, FMT_VTUO_HEAD);
	boaWrite(wp, FMT_VTUO_BODY);
	boaWrite(wp, "<h4>* ");
	if(item==0) boaWrite(wp, "Hlog ");
	else if(item==1) boaWrite(wp, "QLN ");
	else if(item==2) boaWrite(wp, "SNR ");
	if(ds) boaWrite(wp, "Downstream");
	else boaWrite(wp, "Upstream");
	boaWrite(wp, "</h4>\n");
	boaWrite(wp, "<table border=0 cellspacing=4 cellpadding=0>\n");

	i=0;
	while(i<chan)
	{
		int j;
		boaWrite(wp, "<tr align=center bgcolor=#f0f0f0>\n");
		boaWrite(wp, "<th width=10%% align=right bgcolor=#c0c0c0>(%d)</th>\n", i+1);

		for(j=i; j<(i+10); j++)
		{
			double tmpD;

			boaWrite(wp, "<td width=9%%>");
			if( okflag&&(j<chan) )
			{
				tmpD=0;
				if(isVDSL2==0)
				{
					if( (j>=start_tone)&&(j<=end_tone) )
						tmpD=(double)sbuf[j]/10;
				}else
					tmpD=(double)sbuf[offset+j]/10;

				boaWrite(wp, "%.1f", tmpD);
			}
			boaWrite(wp, "</td>");
		}
		boaWrite(wp, "</tr>\n\n");

		i=i+10;
	}


	boaWrite(wp, "</table>\n");
	boaWrite(wp, "<br><br>\n");
	boaWrite(wp, FMT_VTUO_BODY_END);
	boaFooter(wp);
	boaDone(wp, 200);

	if(sbuf) free(sbuf);
}
*/
static void formSetVTUO_Status_BitAlloc(request * wp, int ds)
{
	int i, ret;
	XDSL_OP *d=xdsl_get_op(0);
	int okflag=1, maxtone=4096;
	unsigned char *cbuf;
	int cbuf_size;

	cbuf_size=sizeof(char)*maxtone;
	cbuf = malloc(cbuf_size);
	if(cbuf)
	{
		memset( cbuf, 0,  cbuf_size );
		ret=0;
		if(ds)
			ret = d->xdsl_msg_get_array(GetBitPerToneDs, (int*)cbuf);
		else
			ret = d->xdsl_msg_get_array(GetBitPerToneUs, (int*)cbuf);
		if(!ret)
		{
			printf( "%s: GetBitPerToneDs/Us failed\n", __FUNCTION__ );
			okflag=0;
		}
	}else{
		printf( "%s: malloc failed\n", __FUNCTION__ );
		okflag=0;
	}

	boaHeader(wp);
	boaWrite(wp, FMT_VTUO_HEAD);
	boaWrite(wp, FMT_VTUO_BODY);
	boaWrite(wp, "<h4>* BitAlloc %s</h4>\n", ds?"Downstream":"Upstream");
	boaWrite(wp, "<table border=0 cellspacing=4 cellpadding=0>\n");

	i=0;
	while(i<maxtone)
	{
		int j;
		boaWrite(wp, "<tr align=center bgcolor=#f0f0f0>\n");
		boaWrite(wp, "<th width=10%% align=right bgcolor=#c0c0c0>(%d)</th>\n", i+1);

		for(j=i; j<i+10; j++)
		{
			boaWrite(wp, "<td width=9%%>");
			if(okflag&&(j<maxtone)) boaWrite(wp, "%u", cbuf[j] );
			boaWrite(wp, "</td>");
		}

		boaWrite(wp, "</tr>\n\n");
		i=i+10;
	}


	boaWrite(wp, "</table>\n");
	boaWrite(wp, "<br><br>\n");
	boaWrite(wp, FMT_VTUO_BODY_END);
	boaFooter(wp);
	boaDone(wp, 200);

	if(cbuf) free(cbuf);
}

static void formSetVTUO_Status_GainAlloc(request * wp, int ds)
{
	int i, ret=0;
	XDSL_OP *d=xdsl_get_op(0);
	int okflag=1, maxtone=4096;
	unsigned short *sbuf;
	int sbuf_size;

	sbuf_size=sizeof(short)*maxtone;
	sbuf = malloc(sbuf_size);
	if(sbuf)
	{
		memset( sbuf, 0,  sbuf_size );
		if(ds)
			ret = d->xdsl_msg_get_array(GetGainPerToneDs, (int*)sbuf);
		else
			ret = d->xdsl_msg_get_array(GetGainPerToneUs, (int*)sbuf);
		if(!ret)
		{
			printf( "%s: GetGainPerToneDs/Us failed\n", __FUNCTION__ );
			okflag=0;
		}
	}else{
		printf( "%s: malloc failed\n", __FUNCTION__ );
		okflag=0;
	}

	boaHeader(wp);
	boaWrite(wp, FMT_VTUO_HEAD);
	boaWrite(wp, FMT_VTUO_BODY);
	boaWrite(wp, "<h4>* GainAlloc %s</h4>\n", ds?"Downstream":"Upstream");
	boaWrite(wp, "<table border=0 cellspacing=4 cellpadding=0>\n");

	i=0;
	while(i<maxtone)
	{
		int j;
		boaWrite(wp, "<tr align=center bgcolor=#f0f0f0>\n");
		boaWrite(wp, "<th width=10%% align=right bgcolor=#c0c0c0>(%d)</th>\n", i+1);

		for(j=i; j<i+10; j++)
		{
			boaWrite(wp, "<td width=9%%>");
			if(okflag&&(j<maxtone)) boaWrite(wp, "%u", sbuf[j] );
			boaWrite(wp, "</td>");
		}

		boaWrite(wp, "</tr>\n\n");
		i=i+10;
	}

	boaWrite(wp, "</table>\n");
	boaWrite(wp, "<br><br>\n");
	boaWrite(wp, FMT_VTUO_BODY_END);
	boaFooter(wp);
	boaDone(wp, 200);

	if(sbuf) free(sbuf);
}

static void formSetVTUO_Status(request * wp, char *path, char *query)
{
	char *strApply;

	strApply = boaGetVar(wp, "StatusPage", "");
	VTUO_PRT( "StatusPage=%s\n", strApply);

	if(!strcmp(strApply, "15min")) {
		formSetVTUO_Status_History(wp, 0);
	}else if(!strcmp(strApply, "oneday")) {
		formSetVTUO_Status_History(wp, 1);
	/*
	}else if(!strcmp(strApply, "hlog_ds")) {
		formSetVTUO_Status_SubCarrier(wp, 0, 1);
	}else if(!strcmp(strApply, "hlog_us")) {
		formSetVTUO_Status_SubCarrier(wp, 0, 0);
	}else if(!strcmp(strApply, "qln_ds")) {
		formSetVTUO_Status_SubCarrier(wp, 1, 1);
	}else if(!strcmp(strApply, "qln_us")) {
		formSetVTUO_Status_SubCarrier(wp, 1, 0);
	}else if(!strcmp(strApply, "snr_ds")) {
		formSetVTUO_Status_SubCarrier(wp, 2, 1);
	}else if(!strcmp(strApply, "snr_us")) {
		formSetVTUO_Status_SubCarrier(wp, 2, 0);
	*/
	}else if(!strcmp(strApply, "bit_ds")) {
		formSetVTUO_Status_BitAlloc(wp, 1);
	}else if(!strcmp(strApply, "bit_us")) {
		formSetVTUO_Status_BitAlloc(wp, 0);
	}else if(!strcmp(strApply, "gain_ds")) {
		formSetVTUO_Status_GainAlloc(wp, 1);
	}else if(!strcmp(strApply, "gain_us")) {
		formSetVTUO_Status_GainAlloc(wp, 0);
	}else{
		boaHeader(wp);
		boaWrite(wp, "<body><h4>StatusPage=%s is not ready!!!</body></h4>\n", strApply);
		boaFooter(wp);
		boaDone(wp, 200);
	}
}

void formSetVTUO(request * wp, char *path, char *query)
{
	char *strApply;

	strApply = boaGetVar(wp, "ChanProfile", "");
	if (strApply[0]) {
		formSetVTUO_Chan( wp, path, query );
	}

	strApply = boaGetVar(wp, "GinpSetup", "");
	if (strApply[0]) {
		formSetVTUO_Ginp( wp, path, query );
	}

	strApply = boaGetVar(wp, "LineProfile", "");
	if (strApply[0]) {
		formSetVTUO_Line( wp, path, query );
	}

	strApply = boaGetVar(wp, "InmSetup", "");
	if (strApply[0]) {
		formSetVTUO_Inm( wp, path, query );
	}

	strApply = boaGetVar(wp, "SraSetup", "");
	if (strApply[0]) {
		formSetVTUO_Sra( wp, path, query );
	}

	strApply = boaGetVar(wp, "DpboSetup", "");
	if (strApply[0]) {
		formSetVTUO_Dpbo( wp, path, query );
	}

	strApply = boaGetVar(wp, "PsdSetup", "");
	if (strApply[0]) {
		formSetVTUO_Psd( wp, path, query );
	}
	/*
	strApply = boaGetVar(wp, "VnSetup", "");
	if (strApply[0]) {
		formSetVTUO_Vn( wp, path, query );
	}

	strApply = boaGetVar(wp, "RfiSetup", "");
	if (strApply[0]) {
		formSetVTUO_Rfi( wp, path, query );
	}
	*/
	strApply = boaGetVar(wp, "StatusPage", "");
	if (strApply[0]) {
		formSetVTUO_Status( wp, path, query );
	}

	return;
}
#endif /*CONFIG_DSL_VTUO*/


void formSetAdsl(request * wp, char *path, char *query)
{
	char *submitUrl;
	char *strVal, *strApply;
	char olr;
#ifdef CONFIG_VDSL
	char gvector;
#endif
	short mode;
	int xmode;


	strApply = boaGetVar(wp, "psdm", "");
	if (strApply[0]) {
		#if SUPPORT_TR105
		int id;
		if (!gstrcmp(strApply, "Enable")) {
			psd_measure = 1;
			id = RLCM_ENABLE_NODROPLINEFLAG;
		} else if (!gstrcmp(strApply, "Disable")) {
			psd_measure = 0;
			id = RLCM_DISABLE_NODROPLINEFLAG;
		} else
			goto done;

		adsl_drv_get(id, 0, 0);
		#ifdef CONFIG_USER_XDSL_SLAVE
		adsl_slv_drv_get(id, 0, 0);
		#endif /*CONFIG_USER_XDSL_SLAVE*/
		#endif
		goto done;
	}

	mib_get_s(MIB_ADSL_MODE, (void *)&mode, sizeof(mode));
	//mode &= ADSL_MODE_ANXB;
	mode = 0;

	strVal = boaGetVar(wp, "glite", "");
	if (strVal[0]=='1')
		mode |= ADSL_MODE_GLITE;
	strVal = boaGetVar(wp, "t1413", "");
	if (strVal[0]=='1')
		mode |= ADSL_MODE_T1413;
	strVal = boaGetVar(wp, "gdmt", "");
	if (strVal[0]=='1')
		mode |= ADSL_MODE_GDMT;
	strVal = boaGetVar(wp, "adsl2", "");
	if (strVal[0]=='1')
		mode |= ADSL_MODE_ADSL2;
	strVal = boaGetVar(wp, "anxj", "");
	if (strVal[0]=='1')
		mode |= ADSL_MODE_ANXJ;
	strVal = boaGetVar(wp, "anxl", "");
	if (strVal[0]=='1')
		mode |= ADSL_MODE_ANXL;
	strVal = boaGetVar(wp, "anxm", "");
	if (strVal[0]=='1')
		mode |= ADSL_MODE_ANXM;
	strVal = boaGetVar(wp, "adsl2p", "");
	if (strVal[0]=='1')
		mode |= ADSL_MODE_ADSL2P;
#ifdef ENABLE_ADSL_MODE_GINP
	strVal = boaGetVar(wp, "ginp", "");
	if (strVal[0]=='1')
		mode |= ADSL_MODE_GINP;
#endif
#ifdef CONFIG_VDSL
	strVal = boaGetVar(wp, "vdsl2", "");
	if (strVal[0]=='1')
		mode |= ADSL_MODE_VDSL2;
#endif /*CONFIG_VDSL*/
#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
	strVal = boaGetVar(wp, "gfast", "");
	if (strVal[0]=='1')
		mode |= ADSL_MODE_GFAST;
#endif

	mib_set(MIB_ADSL_MODE, (void *)&mode);

#ifdef CONFIG_VDSL
	// Set G.Vector
	strVal = boaGetVar(wp, "gvec", "");
	if (strVal[0]=='1')
		gvector = 3; // conform to DSL lib
	else
		gvector = 0;
	mib_set(MIB_DSL_G_VECTOR, (void *)&gvector);

	//VDSL2 profile
	strVal = boaGetVar(wp, "vdsl2", "");
	if (strVal[0]=='1')
	{
		unsigned short vd2p=0;

		strVal = boaGetVar(wp, "vdsl2p8a", "");
		if (strVal[0]=='1')
			vd2p |= VDSL2_PROFILE_8A;
		strVal = boaGetVar(wp, "vdsl2p8b", "");
		if (strVal[0]=='1')
			vd2p |= VDSL2_PROFILE_8B;
		strVal = boaGetVar(wp, "vdsl2p8c", "");
		if (strVal[0]=='1')
			vd2p |= VDSL2_PROFILE_8C;
		strVal = boaGetVar(wp, "vdsl2p8d", "");
		if (strVal[0]=='1')
			vd2p |= VDSL2_PROFILE_8D;
		strVal = boaGetVar(wp, "vdsl2p12a", "");
		if (strVal[0]=='1')
			vd2p |= VDSL2_PROFILE_12A;
		strVal = boaGetVar(wp, "vdsl2p12b", "");
		if (strVal[0]=='1')
			vd2p |= VDSL2_PROFILE_12B;
		strVal = boaGetVar(wp, "vdsl2p17a", "");
		if (strVal[0]=='1')
			vd2p |= VDSL2_PROFILE_17A;
		strVal = boaGetVar(wp, "vdsl2p30a", "");
		if (strVal[0]=='1')
			vd2p |= VDSL2_PROFILE_30A;
		strVal = boaGetVar(wp, "vdsl2p35b", "");
		if (strVal[0]=='1')
			vd2p |= VDSL2_PROFILE_35B;

		mib_set(MIB_VDSL2_PROFILE, (void *)&vd2p);
	}
#endif /*CONFIG_VDSL*/
#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
	strVal = boaGetVar(wp, "gfast", "");
	if (strVal[0]=='1')
	{
		unsigned short gfast=0;

		strVal = boaGetVar(wp, "GFast_106a", "");
		if (strVal[0]=='1')
			gfast |= GFAST_PROFILE_106A;
		strVal = boaGetVar(wp, "GFast_212a", "");
		if (strVal[0]=='1')
			gfast |= GFAST_PROFILE_212A;
		strVal = boaGetVar(wp, "GFast_106b", "");
		if (strVal[0]=='1')
			gfast |= GFAST_PROFILE_106B;
		strVal = boaGetVar(wp, "GFast_106c", "");
		if (strVal[0]=='1')
			gfast |= GFAST_PROFILE_106C;
		strVal = boaGetVar(wp, "GFast_212c", "");
		if (strVal[0]=='1')
			gfast |= GFAST_PROFILE_212C;
		strVal = boaGetVar(wp, "GFast_212b", "");
		if (strVal[0]=='1')
			gfast |= GFAST_PROFILE_212B;

		mib_set(MIB_GFAST_PROFILE, (void *)&gfast);
	}
#endif /*CONFIG_USER_CMD_SUPPORT_GFAST*/

	// OLR type
	olr = 0;
	strVal = boaGetVar(wp, "bswap", "");
	if (strVal[0]=='1')
		olr |= 1;

	strVal = boaGetVar(wp, "sra", "");
	if (strVal[0]=='1')
		olr |= 2;

	mib_set(MIB_ADSL_OLR, (void *)&olr);

// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	//added by xlyue
	//syslog(LOG_INFO, "ADSL Setting -- ADSL_MODE: 0x%x, ADSL_ORL: 0x%x\n",mode,olr);

#ifdef APPLY_CHANGE
	#ifdef CONFIG_USER_XDSL_SLAVE
//	mib_slv_sync_dsl();
//	sys_slv_init( UC_STR_DSL_SETUP );
	setupSlvDsl();
	#endif /*CONFIG_USER_XDSL_SLAVE*/

	setupDsl();

#ifdef CONFIG_VDSL
	/*when ad<->vd, need retrain*/
	adsl_drv_get(RLCM_MODEM_RETRAIN, NULL, 0);

	#ifdef CONFIG_USER_XDSL_SLAVE
	adsl_slv_drv_get(RLCM_MODEM_RETRAIN, NULL, 0);
	#endif

#endif /*CONFIG_VDSL*/
#endif
done:
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page

	OK_MSG(submitUrl);

	return;

}

static void _formDiagAdsl(request * wp, XDSL_OP *d)
{
	char *str, *submitUrl;
	int mode;

//	str = boaGetVar(wp, "start", "");
	// Display diagnostic messages
	boaHeader(wp);
	submitUrl = boaGetVar(wp, "submit-url", "");
	boaWrite(wp, "<META HTTP-EQUIV=Refresh CONTENT=\"180; URL=%s?act=1\">\n", submitUrl );
   	boaWrite(wp, "<body><blockquote><br><br>\n");
   	boaWrite(wp, "%s<br>\n", Tadsl_diag_wait);
   	boaWrite(wp, "</blockquote></body>");
   	boaFooter(wp);
	boaDone(wp, 200);

//	if (str[0]) {
		// start diagnose here
#ifdef _USE_NEW_IOCTL_FOR_DSLDIAG_
		//fprintf( stderr, "use RLCM_ENABLE_DIAGNOSTIC to start dsldiag\n" );
		mode=0;
		d->xdsl_drv_get(RLCM_ENABLE_DIAGNOSTIC, (void *)&mode, sizeof(int));//Lupin
#else
		mode = 41;
		d->xdsl_drv_get(RLCM_DEBUG_MODE, (void *)&mode, sizeof(int));
#endif
		d->xdsl_drv_get(RLCM_MODEM_RETRAIN, NULL, 0);
//	}
	//submitUrl = boaGetVar(wp, "submit-url", "");
	//if (submitUrl[0])
	//	boaRedirect(wp, submitUrl);
}

void formDiagAdsl(request * wp, char *path, char *query)
{
	XDSL_OP *d;
#ifdef CONFIG_USER_XDSL_SLAVE
	char *id;
	id = boaGetVar(wp, "slaveid", "");
	//printf( "%s: id=%s\n", __FUNCTION__, id );
	if(id[0])
	{
		d=xdsl_get_op(1);
	}else
#endif /*CONFIG_USER_XDSL_SLAVE*/
	{
		d=xdsl_get_op(0);
	}

	_formDiagAdsl(wp, d);
}


void formStatAdsl(request * wp, char *path, char *query)
{
	char *submitUrl;

	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
}
#endif

#ifdef CONFIG_DEV_xDSL
static int _adslToneDiagTbl(request * wp, XDSL_OP *d)
{
	char mode;
	static int chan=256;
	int nBytesSent=0;
	char str[16], hlin_ds[32], hlin_us[32];
	char latt_ds[16], latt_us[16], satt_ds[16], satt_us[16];
	char snr_ds[16], snr_us[16], attr_ds[16], attr_us[16];
	char txpw_ds[16], txpw_us[16];
	char *act;
	int ldstate;
#ifdef CONFIG_VDSL
	int mval=0;
#endif /*CONFIG_VDSL*/

	act = boaGetVar(wp, "act", "");
	if (act && act[0]=='1') {
		d->xdsl_drv_get(RLCM_GET_LD_STATE, (void *)&ldstate, 4);
#ifndef CONFIG_GENERAL_WEB
		if (ldstate != 0)
			nBytesSent += boaWrite(wp, "<tr>\n<b><font color='green'>%s</b></tr>\n", Tadsl_diag_suc);
		else
			nBytesSent += boaWrite(wp, "<tr>\n<b><font color='red'>%s</b></tr>\n", Tadsl_diag_fail);
#else
		nBytesSent += boaWrite(wp,"<div class=\"data_vertical data_common_notitle\">\n<table>\n");
		if (ldstate != 0)
			nBytesSent += boaWrite(wp, "<tr>\n%s</tr>\n", Tadsl_diag_suc);
		else
			nBytesSent += boaWrite(wp, "<tr>\n%s</tr>\n", Tadsl_diag_fail);
#endif
	}
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr bgcolor=#f0f0f0>\n<th align=left bgcolor=#c0c0c0 width=\"120\"></th>\n");
	nBytesSent += boaWrite(wp, "<th width=\"100\"><font size=2>%s</th><th width=\"100\"><font size=2>%s</th>\n</tr>\n", multilang(LANG_DOWNSTREAM), multilang(LANG_UPSTREAM));
#else
	nBytesSent += boaWrite(wp, "<tr>\n<th></th>\n");
	nBytesSent += boaWrite(wp, "<th>%s</th><th>%s</th>\n</tr>\n", multilang(LANG_DOWNSTREAM), multilang(LANG_UPSTREAM));
#endif
#ifdef CONFIG_VDSL
	//reset
	hlin_ds[0] = hlin_us[0] = latt_ds[0] = latt_us[0] = 0;
	satt_ds[0] = satt_us[0] = snr_ds[0] = snr_us[0] = 0;
	attr_ds[0] = attr_us[0] = txpw_ds[0] = txpw_us[0] = 0;

	// get the channel number
	if(d->xdsl_msg_get(GetPmdMode,&mval))
	{
		short *pother;
		VDSL2DiagOthers vd2other;

		pother=(short*)&vd2other;
		memset( &vd2other, 0, sizeof(vd2other) );
		if( d->xdsl_drv_get(RLCM_GET_VDSL2_DIAG_OTHER, (void *)&vd2other, sizeof(vd2other)) )
		{
			if(mval<MODE_VDSL1)
			{
				// Hlinear scale
				snprintf(hlin_ds, 16, "%d", (unsigned short)pother[1]);
				snprintf(hlin_us, 16, "%d", (unsigned short)pother[0]);
				// loop Attenuation
				snprintf(latt_ds, 16, "%d.%d", pother[3]/10, abs(pother[3]%10));
				snprintf(latt_us, 16, "%d.%d", pother[2]/10, abs(pother[2]%10));
				// signal Attenuation
				snprintf(satt_ds, 16, "%d.%d", pother[5]/10, abs(pother[5]%10));
				snprintf(satt_us, 16, "%d.%d", pother[4]/10, abs(pother[4]%10));
				// SNR Margin
				snprintf(snr_ds, 16, "%d.%d", pother[7]/10, abs(pother[7]%10));
				snprintf(snr_us, 16, "%d.%d", pother[6]/10, abs(pother[6]%10));
				// Attainable Rate
				snprintf(attr_ds, 16, "%d", pother[9]);
				snprintf(attr_us, 16, "%d", pother[8]);
				// tx power
				snprintf(txpw_ds, 16, "%d.%d", pother[11]/10, abs(pother[11]%10));
				snprintf(txpw_us, 16, "%d.%d", pother[10]/10, abs(pother[10]%10));
			}else if(mval&MODE_VDSL2){
				//printf( "%s:%d> it's VDSL2\n", __FUNCTION__, __LINE__ );
				// Hlinear scale
				snprintf(hlin_ds, 16, "%d", (unsigned short)vd2other.HlinScale_ds);
				snprintf(hlin_us, 16, "%d", (unsigned short)vd2other.HlinScale_us);
				#if 0
				// loop Attenuation
				snprintf(latt_ds, 16, "%d.%d", 0, 0);
				snprintf(latt_us, 16, "%d.%d", 0, 0);
				// signal Attenuation
				snprintf(satt_ds, 16, "%d.%d", 0, 0);
				snprintf(satt_us, 16, "%d.%d", 0, 0);
				#endif
				// SNR Margin
				snprintf(snr_ds, 16, "%d.%d", vd2other.SNRMds/10, abs(vd2other.SNRMds%10));
				snprintf(snr_us, 16, "%d.%d", vd2other.SNRMus/10, abs(vd2other.SNRMus%10));
				// Attainable Rate
				snprintf(attr_ds, 16, "%d", vd2other.ATTNDRds);
				snprintf(attr_us, 16, "%d", vd2other.ATTNDRus);
				// tx power
				snprintf(txpw_ds, 16, "%d.%d", vd2other.ACTATPds/10, abs(vd2other.ACTATPds%10));
				snprintf(txpw_us, 16, "%d.%d", vd2other.ACTATPus/10, abs(vd2other.ACTATPus%10));
			}
		}else{
			//printf( "%s:%d> RLCM_GET_VDSL2_DIAG_OTHER failed\n", __FUNCTION__, __LINE__ );
		}
	}else{
		//printf( "%s:%d> GetPmdMode failed\n", __FUNCTION__, __LINE__ );
	}

#else /*CONFIG_VDSL*/

	// get the channel number
	if(d->xdsl_drv_get(RLCM_GET_SHOWTIME_XDSL_MODE, (void *)&mode, 1)) {
		//ramen to clear the first 3 bit
		mode&=0x1F;
		if (mode < 5) //adsl1/adsl2
			chan = 256;
		else
			chan = 512;
	}

	//hlog = malloc(sizeof(short)*(chan*3+HLOG_ADDITIONAL_SIZE));
	hlog = malloc(sizeof(short)*(MAX_DSL_TONE*3+HLOG_ADDITIONAL_SIZE));
	//if (d->xdsl_drv_get(RLCM_GET_DIAG_HLOG, (void *)hlog, sizeof(short)*(chan*3+HLOG_ADDITIONAL_SIZE))) {
	if (d->xdsl_drv_get(RLCM_GET_DIAG_HLOG, (void *)hlog, sizeof(short)*(MAX_DSL_TONE*3+HLOG_ADDITIONAL_SIZE))) {
		// Hlinear scale
		snprintf(hlin_ds, 16, "%d", (unsigned short)hlog[chan*3+1]);
		snprintf(hlin_us, 16, "%d", (unsigned short)hlog[chan*3]);
		// loop Attenuation
		snprintf(latt_ds, 16, "%d.%d", hlog[chan*3+3]/10, abs(hlog[chan*3+3]%10));
		snprintf(latt_us, 16, "%d.%d", hlog[chan*3+2]/10, abs(hlog[chan*3+2]%10));
		// signal Attenuation
		snprintf(satt_ds, 16, "%d.%d", hlog[chan*3+5]/10, abs(hlog[chan*3+5]%10));
		snprintf(satt_us, 16, "%d.%d", hlog[chan*3+4]/10, abs(hlog[chan*3+4]%10));
		// SNR Margin
		snprintf(snr_ds, 16, "%d.%d", hlog[chan*3+7]/10, abs(hlog[chan*3+7]%10));
		snprintf(snr_us, 16, "%d.%d", hlog[chan*3+6]/10, abs(hlog[chan*3+6]%10));
		// Attainable Rate
		snprintf(attr_ds, 16, "%d", hlog[chan*3+9]);
		snprintf(attr_us, 16, "%d", hlog[chan*3+8]);
		// tx power
		snprintf(txpw_ds, 16, "%d.%d", hlog[chan*3+11]/10, abs(hlog[chan*3+11]%10));
		snprintf(txpw_us, 16, "%d.%d", hlog[chan*3+10]/10, abs(hlog[chan*3+10]%10));
	}
	else {
		hlin_ds[0] = hlin_us[0] = latt_ds[0] = latt_us[0] = 0;
		satt_ds[0] = satt_us[0] = snr_ds[0] = snr_us[0] = 0;
		attr_ds[0] = attr_us[0] = txpw_ds[0] = txpw_us[0] = 0;
	}
	if(hlog) free(hlog);
#endif /*CONFIG_VDSL*/

#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr bgcolor=#f0f0f0>\n<th align=left bgcolor=#c0c0c0><font size=2>%s</th>\n", multilang(LANG_HLIN_SCALE));
	nBytesSent += boaWrite(wp, "<td align=center><font size=2>%s</font></td>\n", hlin_ds);
	nBytesSent += boaWrite(wp, "<td align=center><font size=2>%s</font></td>\n</tr>\n", hlin_us);
#else
	nBytesSent += boaWrite(wp, "<tr>\n<th>%s</th>\n", multilang(LANG_HLIN_SCALE));
	nBytesSent += boaWrite(wp, "<td>%s</td>\n", hlin_ds);
	nBytesSent += boaWrite(wp, "<td>%s</td>\n</tr>\n", hlin_us);
#endif

#ifdef CONFIG_VDSL
if(mval<MODE_VDSL1)
{
#endif /*CONFIG_VDSL*/

#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr bgcolor=#f0f0f0>\n<th align=left bgcolor=#c0c0c0><font size=2>%s(dB)</th>\n", multilang(LANG_LOOP_ATTENUATION));
	nBytesSent += boaWrite(wp, "<td align=center><font size=2>%s</font></td>\n", latt_ds);
	nBytesSent += boaWrite(wp, "<td align=center><font size=2>%s</font></td>\n</tr>\n", latt_us);

	nBytesSent += boaWrite(wp, "<tr bgcolor=#f0f0f0>\n<th align=left bgcolor=#c0c0c0><font size=2>%s(dB)</th>\n", multilang(LANG_SIGNAL_ATTENUATION));
	nBytesSent += boaWrite(wp, "<td align=center><font size=2>%s</font></td>\n", satt_ds);
	nBytesSent += boaWrite(wp, "<td align=center><font size=2>%s</font></td>\n</tr>\n", satt_us);
#else
	nBytesSent += boaWrite(wp, "<tr>\n<th>%s(dB)</th>\n", multilang(LANG_LOOP_ATTENUATION));
	nBytesSent += boaWrite(wp, "<td>%s</td>\n", latt_ds);
	nBytesSent += boaWrite(wp, "<td>%s</td>\n</tr>\n", latt_us);

	nBytesSent += boaWrite(wp, "<tr>\n<th>%s(dB)</th>\n", multilang(LANG_SIGNAL_ATTENUATION));
	nBytesSent += boaWrite(wp, "<td>%s</td>\n", satt_ds);
	nBytesSent += boaWrite(wp, "<td>%s</td>\n</tr>\n", satt_us);
#endif
#ifdef CONFIG_VDSL
}
#endif /*CONFIG_VDSL*/

#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr bgcolor=#f0f0f0>\n<th align=left bgcolor=#c0c0c0><font size=2>%s(dB)</th>\n", multilang(LANG_SNR_MARGIN));
	nBytesSent += boaWrite(wp, "<td align=center><font size=2>%s</font></td>\n", snr_ds);
	nBytesSent += boaWrite(wp, "<td align=center><font size=2>%s</font></td>\n</tr>\n", snr_us);

	nBytesSent += boaWrite(wp, "<tr bgcolor=#f0f0f0>\n<th align=left bgcolor=#c0c0c0><font size=2>%s(Kbps)</th>\n", multilang(LANG_ATTAINABLE_RATE));
	nBytesSent += boaWrite(wp, "<td align=center><font size=2>%s</font></td>\n", attr_ds);
	nBytesSent += boaWrite(wp, "<td align=center><font size=2>%s</font></td>\n</tr>\n", attr_us);

	nBytesSent += boaWrite(wp, "<tr bgcolor=#f0f0f0>\n<th align=left bgcolor=#c0c0c0><font size=2>%s(dBm)</th>\n", multilang(LANG_OUTPUT_POWER));
	nBytesSent += boaWrite(wp, "<td align=center><font size=2>%s</font></td>\n", txpw_ds);
	nBytesSent += boaWrite(wp, "<td align=center><font size=2>%s</font></td>\n</tr>\n", txpw_us);
#else

	nBytesSent += boaWrite(wp, "<tr>\n<th>%s(dB)</th>\n", multilang(LANG_SNR_MARGIN));
	nBytesSent += boaWrite(wp, "<td>%s</td>\n", snr_ds);
	nBytesSent += boaWrite(wp, "<td>%s</td>\n</tr>\n", snr_us);

	nBytesSent += boaWrite(wp, "<tr>\n<th>%s(Kbps)</th>\n", multilang(LANG_ATTAINABLE_RATE));
	nBytesSent += boaWrite(wp, "<td>%s</td>\n", attr_ds);
	nBytesSent += boaWrite(wp, "<td>%s</td>\n</tr>\n", attr_us);

	nBytesSent += boaWrite(wp, "<tr>\n<th>%s(dBm)</th>\n", multilang(LANG_OUTPUT_POWER));
	nBytesSent += boaWrite(wp, "<td>%s</td>\n", txpw_ds);
	nBytesSent += boaWrite(wp, "<td>%s</td>\n</tr>\n", txpw_us);
	nBytesSent += boaWrite(wp, "</table>\n</div>");
#endif
	return nBytesSent;
}

int adslToneDiagTbl(int eid, request * wp, int argc, char **argv)
{
	XDSL_OP *d;
#ifdef CONFIG_USER_XDSL_SLAVE
	//printf( "\n%s: %s\n", __FUNCTION__, argc?"slave":"" );
	if(argc)
	{
		d=xdsl_get_op(1);
	}else
#endif /*CONFIG_USER_XDSL_SLAVE*/
	{
		d=xdsl_get_op(0);
	}

	return _adslToneDiagTbl(wp, d);
}


static int _vdslBandStatusTbl(request * wp, XDSL_OP *d)
{
	int nBytesSent=0;
#ifdef CONFIG_VDSL
	int mval=0;

	if(d->xdsl_msg_get(GetPmdMode,&mval))
	{
		if(mval&MODE_VDSL2)
		{
			int flag,i;
			VDSL2DiagOthers vd2other;

			memset( &vd2other, 0, sizeof(vd2other) );
			if( d->xdsl_drv_get(RLCM_GET_VDSL2_DIAG_OTHER, (void *)&vd2other, sizeof(vd2other))==0 )
			{
				flag=0;
				//printf( "%s:%d> RLCM_GET_VDSL2_DIAG_OTHER VDSL2 failed\n", __FUNCTION__, __LINE__ );
			}else{
				flag=1;
			}

#ifndef CONFIG_GENERAL_WEB
#define FMT_BANDSTAT0 "<th width=9%% bgcolor=#c0c0c0><font size=2>%s%d</font></th>\n"
#define FMT_BANDSTAT1 "<th bgcolor=#c0c0c0><font size=2>%s</font></th>\n"
#define FMT_BANDSTAT2 "<td align=center bgcolor=#f0f0f0><font size=2>%g</font></td>\n"
#define FMT_BANDSTAT3 "<td align=center bgcolor=#f0f0f0></td>\n"

			nBytesSent += boaWrite(wp, "<table border=0 width=500 cellspacing=4 cellpadding=0>\n");
			nBytesSent += boaWrite(wp, "<tr>\n");
			nBytesSent += boaWrite(wp, "<th width=19%% bgcolor=#c0c0c0><font size=2>Band Status</font></th>\n");
#else
#define FMT_BANDSTAT0 "<th>%s%d</th>\n"
#define FMT_BANDSTAT1 "<th>%s</th>\n"
#define FMT_BANDSTAT2 "<td>%g</td>\n"
#define FMT_BANDSTAT3 "<td></td>\n"
			nBytesSent += boaWrite(wp, "<div class=\"data_vertical data_common_notitle\">\n");
			nBytesSent += boaWrite(wp, "<table>\n");
			nBytesSent += boaWrite(wp, "<tr>\n");
			nBytesSent += boaWrite(wp, "<th>Band Status</th>\n");

#endif
			for(i=0;i<5;i++)
				nBytesSent += boaWrite(wp, FMT_BANDSTAT0, "U", i);
			for(i=0;i<4;i++)
				nBytesSent += boaWrite(wp, FMT_BANDSTAT0, "D", i+1);
			nBytesSent += boaWrite(wp, "</tr>\n");

			//LATN
			nBytesSent += boaWrite(wp, "<tr>\n");
			nBytesSent += boaWrite(wp, FMT_BANDSTAT1, "LATN");
			if(flag)
			{
				for(i=0;i<5;i++)
					nBytesSent += boaWrite(wp, FMT_BANDSTAT2, ((float)vd2other.LATNpbus[i])/10 );
				for(i=0;i<4;i++)
					nBytesSent += boaWrite(wp, FMT_BANDSTAT2, ((float)vd2other.LATNpbds[i])/10 );
			}else{
				for(i=0;i<9;i++)
					nBytesSent += boaWrite(wp, FMT_BANDSTAT3);
			}
			nBytesSent += boaWrite(wp, "</tr>\n");

			//SATN
			nBytesSent += boaWrite(wp, "<tr>\n");
			nBytesSent += boaWrite(wp, FMT_BANDSTAT1, "SATN");
			if(flag)
			{
				for(i=0;i<5;i++)
					nBytesSent += boaWrite(wp, FMT_BANDSTAT2, ((float)vd2other.SATNpbus[i])/10 );
				for(i=0;i<4;i++)
					nBytesSent += boaWrite(wp, FMT_BANDSTAT2, ((float)vd2other.SATNpbds[i])/10 );
			}else{
				for(i=0;i<9;i++)
					nBytesSent += boaWrite(wp, FMT_BANDSTAT3);
			}
			nBytesSent += boaWrite(wp, "</tr>\n");

			//SNRM
			nBytesSent += boaWrite(wp, "<tr>\n");
			nBytesSent += boaWrite(wp, FMT_BANDSTAT1, "SNRM");
			if(flag)
			{
				for(i=0;i<5;i++)
					nBytesSent += boaWrite(wp, FMT_BANDSTAT2, ((float)vd2other.SNRMpbus[i])/10 );
				for(i=0;i<4;i++)
					nBytesSent += boaWrite(wp, FMT_BANDSTAT2, ((float)vd2other.SNRMpbds[i])/10 );
			}else{
				for(i=0;i<9;i++)
					nBytesSent += boaWrite(wp, FMT_BANDSTAT3);
			}
			nBytesSent += boaWrite(wp, "</tr>\n");


			nBytesSent += boaWrite(wp, "</table>\n");
#ifdef CONFIG_GENERAL_WEB
			nBytesSent += boaWrite(wp, "</div>\n");
#endif
			nBytesSent += boaWrite(wp, "<p>\n");
		}
	}
#endif /*CONFIG_VDSL*/
	return nBytesSent;
}

int vdslBandStatusTbl(int eid, request * wp, int argc, char **argv)
{
	XDSL_OP *d;
#ifdef CONFIG_USER_XDSL_SLAVE
	//printf( "\n%s: %s\n", __FUNCTION__, argc?"slave":"" );
	if(argc)
	{
		d=xdsl_get_op(1);
	}else
#endif /*CONFIG_USER_XDSL_SLAVE*/
	{
		d=xdsl_get_op(0);
	}

	return _vdslBandStatusTbl(wp, d);
}

#ifdef CONFIG_VDSL
static int _adslToneDiagList(request * wp, XDSL_OP *d)
{
	char mode;
	int chan;
	int i;
	int nBytesSent=0;
	int ival, intp, fp;
	char str[16];
	int mval=0;
	int isVDSL2, vd2loop;
	ComplexShort *hlin;
	VDSL2DiagOthers vd2other;

	// get the channel number
	chan=256;
	isVDSL2=0;
	if(d->xdsl_msg_get(GetPmdMode,&mval))
	{
		if(mval&MODE_VDSL2)
			isVDSL2=1;
		else
			isVDSL2=0;

		if(mval<MODE_ADSL2PLUS)
			chan=256;
		else
			chan=512;
	}

	intp=fp=0;
	str[0] = 0;

	snr = malloc(sizeof(short)*MAX_DSL_TONE*2);
	qln = malloc(sizeof(short)*MAX_DSL_TONE*2);
	hlog = malloc(sizeof(short)*MAX_DSL_TONE*2);
	hlin = malloc(sizeof(ComplexShort)*MAX_DSL_TONE*2);
	if( !snr || !qln || !hlog || !hlin )
	{
		printf( "%s:%d>malloc failed, snr=%x, qln=%x, hlog=%x, hlin=%x\n",
			__FUNCTION__, __LINE__, snr, qln, hlog, hlin );
		diagflag = 0;
	}else{
		//reset
		memset( hlog, 0,  sizeof(short)*MAX_DSL_TONE*2 );
		memset( snr, 0,  sizeof(short)*MAX_DSL_TONE*2 );
		memset( qln, 0,  sizeof(short)*MAX_DSL_TONE*2 );
		memset( hlin, 0,  sizeof(ComplexShort)*MAX_DSL_TONE*2 );
		memset( &vd2other, 0, sizeof(vd2other) );

		if( d->xdsl_drv_get(RLCM_GET_VDSL2_DIAG_HLOG, (void *)hlog, sizeof(short)*MAX_DSL_TONE*2))
		{
			d->xdsl_drv_get(RLCM_GET_VDSL2_DIAG_SNR, (void *)snr, sizeof(short)*MAX_DSL_TONE*2);
			d->xdsl_drv_get(RLCM_GET_VDSL2_DIAG_QLN, (void *)qln, sizeof(short)*MAX_DSL_TONE*2);
			d->xdsl_drv_get(RLCM_GET_VDSL2_DIAG_HLIN, (void *)hlin, sizeof(ComplexShort)*MAX_DSL_TONE*2);
			d->xdsl_drv_get(RLCM_GET_VDSL2_DIAG_OTHER, (void *)&vd2other, sizeof(vd2other));
			diagflag = 1;
		}else{
			diagflag = 0;
			//printf( "%s:%d> RLCM_GET_VDSL2_DIAG_HLOG VDSL2 failed\n", __FUNCTION__, __LINE__ );
		}
	}
#ifdef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<div class=\"data_vertical data_common_notitle\">\n<table>\n");
#endif
	for(vd2loop=0; vd2loop<=isVDSL2; vd2loop++)
	{
		int offset;

#ifndef CONFIG_GENERAL_WEB
		if(vd2loop==1)
		{
			offset=MAX_DSL_TONE;
			nBytesSent += boaWrite(wp, "<tr></tr><tr></tr>\n");
			nBytesSent += boaWrite(wp, "<tr><th bgcolor=#c0c0c0 colspan=6><font size=2>Downstream (Group Number=%u)</font></th></tr>\n", vd2other.SNRGds);
		}else{ //vd2loop=0
			offset=0;
			if(isVDSL2) nBytesSent += boaWrite(wp, "<tr><th bgcolor=#c0c0c0 colspan=6><font size=2>Upstream (Group Number=%u)</font></th></tr>\n", vd2other.SNRGus);
		}

		nBytesSent += boaWrite(wp, "<tr>\n<th width=15%% bgcolor=#c0c0c0><font size=2>%s</font></th>\n", multilang(LANG_TONE_NUMBER));
		nBytesSent += boaWrite(wp, "<th width=15%% bgcolor=#c0c0c0><font size=2>%s</font></th>\n", multilang(LANG_H_REAL));
		nBytesSent += boaWrite(wp, "<th width=15%% bgcolor=#c0c0c0><font size=2>%s</font></th>\n", multilang(LANG_H_IMAGE));
		nBytesSent += boaWrite(wp, "<th width=15%% bgcolor=#c0c0c0><font size=2>%s</font></th>\n", multilang(LANG_SNR));
		nBytesSent += boaWrite(wp, "<th width=15%% bgcolor=#c0c0c0><font size=2>%s</font></th>\n", multilang(LANG_QLN));
		nBytesSent += boaWrite(wp, "<th width=15%% bgcolor=#c0c0c0><font size=2>%s</font></th>\n</tr>\n", multilang(LANG_HLOG));
#else
		if(vd2loop==1)
		{
			offset=MAX_DSL_TONE;
			nBytesSent += boaWrite(wp, "<tr><th colspan=6>Downstream (Group Number=%u)</th></tr>\n", vd2other.SNRGds);
		}else{ //vd2loop=0
			offset=0;
			if(isVDSL2) nBytesSent += boaWrite(wp, "<tr><th colspan=6>Upstream (Group Number=%u)</th></tr>\n", vd2other.SNRGus);
		}

		nBytesSent += boaWrite(wp, "<tr>\n<th width=15%%>%s</th>\n", multilang(LANG_TONE_NUMBER));
		nBytesSent += boaWrite(wp, "<th width=15%%>%s</th>\n", multilang(LANG_H_REAL));
		nBytesSent += boaWrite(wp, "<th width=15%%>%s</th>\n", multilang(LANG_H_IMAGE));
		nBytesSent += boaWrite(wp, "<th width=15%%>%s</th>\n", multilang(LANG_SNR));
		nBytesSent += boaWrite(wp, "<th width=15%%>%s</th>\n", multilang(LANG_QLN));
		nBytesSent += boaWrite(wp, "<th width=15%%>%s</th>\n</tr>\n", multilang(LANG_HLOG));
#endif
		for (i = 0; i < chan; i++)
		{
#ifndef CONFIG_GENERAL_WEB
			nBytesSent += boaWrite(wp, "<tr>\n<th bgcolor=#c0c0c0><font size=2>%d</font></th>\n", i);
#else
			nBytesSent += boaWrite(wp, "<tr align=center>\n<th>%d</th>\n", i);
#endif
			// H.Real
			if (diagflag) {
				intp = hlin[offset+i].real/1000;
				fp = hlin[offset+i].real%1000;
				if (fp<0) {
					fp = -fp;
					if (intp == 0)
						snprintf(str, 16, "-0.%03d", fp);
					else
						snprintf(str, 16, "%d.%03d", intp, fp);
				}
				else
					snprintf(str, 16, "%d.%03d", intp, fp);
			}
#ifndef CONFIG_GENERAL_WEB
			nBytesSent += boaWrite(wp, "<td align=center bgcolor=#f0f0f0><font size=2>%s</font></td>\n", str);
#else
			nBytesSent += boaWrite(wp, "<td align=center>%s</td>\n", str);
#endif
			// H.Image
			if (diagflag) {
				intp = hlin[offset+i].imag/1000;
				fp = hlin[offset+i].imag%1000;
				if (fp<0) {
					fp = -fp;
					if (intp == 0)
						snprintf(str, 16, "-0.%03d", fp);
					else
						snprintf(str, 16, "%d.%03d", intp, fp);
				}
				else
					snprintf(str, 16, "%d.%03d", intp, fp);
			}
#ifndef CONFIG_GENERAL_WEB
			nBytesSent += boaWrite(wp, "<td align=center bgcolor=#f0f0f0><font size=2>%s</font></td>\n", str);
#else
			nBytesSent += boaWrite(wp, "<td align=center>%s</td>\n", str);
#endif
			// snr
			if (diagflag) {
				intp = snr[offset+i]/10;
				fp = abs(snr[offset+i]%10);
				snprintf(str, 16, "%d.%d", intp, fp);
			}
#ifndef CONFIG_GENERAL_WEB
			nBytesSent += boaWrite(wp, "<td align=center bgcolor=#f0f0f0><font size=2>%s</font></td>\n", str);
#else
			nBytesSent += boaWrite(wp, "<td align=center>%s</td>\n", str);
#endif
			// qln
			if (diagflag) {
				intp = qln[offset+i]/10;
				fp = qln[offset+i]%10;
				if (fp<0) {
					if (intp != 0)
						snprintf(str, 16, "%d.%d", intp, -fp);
					else
						snprintf(str, 16, "-%d.%d", intp, -fp);
				}
				else
					snprintf(str, 16, "%d.%d", intp, fp);
			}
#ifndef CONFIG_GENERAL_WEB
			nBytesSent += boaWrite(wp, "<td align=center bgcolor=#f0f0f0><font size=2>%s</font></td>\n", str);
#else
			nBytesSent += boaWrite(wp, "<td align=center>%s</td>\n", str);
#endif
			// hlog
			if (diagflag) {
				intp = hlog[offset+i]/10;
				fp = hlog[offset+i]%10;
				if (fp<0) {
					if (intp != 0)
						snprintf(str, 16, "%d.%d", intp, -fp);
					else
						snprintf(str, 16, "-%d.%d", intp, -fp);
				}
				else
					snprintf(str, 16, "%d.%d", intp, fp);
			}
#ifndef CONFIG_GENERAL_WEB
			nBytesSent += boaWrite(wp, "<td align=center bgcolor=#f0f0f0><font size=2>%s</font></td>\n</tr>\n", str);
#else
			nBytesSent += boaWrite(wp, "<td align=center>%s</td>\n</tr>\n", str);
#endif
		}
	}
#ifdef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "</table>\n</div>\n");
#endif
	if(snr) free(snr);
	if(qln) free(qln);
	if(hlog) free(hlog);
	if(hlin) free(hlin);

	return nBytesSent;
}
#else
static int _adslToneDiagList(request * wp, XDSL_OP *d)
{
	char mode;
	static int chan=256;
	int i;
	int nBytesSent=0;
	int ival, intp, fp;
	char str[16];

	// get the channel number
	if(d->xdsl_drv_get(RLCM_GET_SHOWTIME_XDSL_MODE, (void *)&mode, 1)) {
		//ramen to clear the first 3 bit.
		mode&=0x1f;
		if (mode < 5) //adsl1/adsl2
			chan = 256;
		else
			chan = 512;
	}

	intp=fp=0;
	str[0] = 0;

	/*
	snr = malloc(sizeof(short)*chan);
	qln = malloc(sizeof(short)*chan);
	hlog = malloc(sizeof(short)*(chan*3+HLOG_ADDITIONAL_SIZE));
	*/
	snr = malloc(sizeof(short)*MAX_DSL_TONE);
	qln = malloc(sizeof(short)*MAX_DSL_TONE);
	hlog = malloc(sizeof(short)*(MAX_DSL_TONE*3+HLOG_ADDITIONAL_SIZE));

	/*
	if (adsl_drv_get(RLCM_GET_DIAG_HLOG, (void *)hlog, sizeof(short)*(chan*3+HLOG_ADDITIONAL_SIZE))) {
		adsl_drv_get(RLCM_GET_DIAG_SNR, (void *)snr, sizeof(short)*chan);
		adsl_drv_get(RLCM_GET_DIAG_QLN, (void *)qln, sizeof(short)*chan);
		diagflag = 1;
	}
	*/
	if (d->xdsl_drv_get(RLCM_GET_DIAG_HLOG, (void *)hlog, sizeof(short)*(MAX_DSL_TONE*3+HLOG_ADDITIONAL_SIZE))) {
		d->xdsl_drv_get(RLCM_GET_DIAG_SNR, (void *)snr, sizeof(short)*MAX_DSL_TONE);
		d->xdsl_drv_get(RLCM_GET_DIAG_QLN, (void *)qln, sizeof(short)*MAX_DSL_TONE);
		diagflag = 1;
	}
	else
		diagflag = 0;
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr>\n<th width=15%% bgcolor=#c0c0c0><font size=2>%s</font></th>\n", multilang(LANG_TONE_NUMBER));
	nBytesSent += boaWrite(wp, "<th width=15%% bgcolor=#c0c0c0><font size=2>%s</font></th>\n", multilang(LANG_H_REAL));
	nBytesSent += boaWrite(wp, "<th width=15%% bgcolor=#c0c0c0><font size=2>%s</font></th>\n", multilang(LANG_H_IMAGE));
	nBytesSent += boaWrite(wp, "<th width=15%% bgcolor=#c0c0c0><font size=2>%s</font></th>\n", multilang(LANG_SNR));
	nBytesSent += boaWrite(wp, "<th width=15%% bgcolor=#c0c0c0><font size=2>%s</font></th>\n", multilang(LANG_QLN));
	nBytesSent += boaWrite(wp, "<th width=15%% bgcolor=#c0c0c0><font size=2>%s</font></th>\n</tr>\n", multilang(LANG_HLOG));
#else
	nBytesSent += boaWrite(wp, "<div class=\"data_vertical data_common_notitle\">\n<table>\n");
	nBytesSent += boaWrite(wp, "<tr>\n<th width=15%%>%s</th>\n", multilang(LANG_TONE_NUMBER));
	nBytesSent += boaWrite(wp, "<th width=15%%>%s</th>\n", multilang(LANG_H_REAL));
	nBytesSent += boaWrite(wp, "<th width=15%%>%s</th>\n", multilang(LANG_H_IMAGE));
	nBytesSent += boaWrite(wp, "<th width=15%%>%s</th>\n", multilang(LANG_SNR));
	nBytesSent += boaWrite(wp, "<th width=15%%>%s</th>\n", multilang(LANG_QLN));
	nBytesSent += boaWrite(wp, "<th width=15%%>%s</th>\n</tr>\n", multilang(LANG_HLOG));
#endif
	for (i = 0; i < chan; i++) {
#ifndef CONFIG_GENERAL_WEB
		nBytesSent += boaWrite(wp, "<tr>\n<th bgcolor=#c0c0c0><font size=2>%d</font></th>\n", i);
#else
		nBytesSent += boaWrite(wp, "<tr>\n<th>%d</th>\n", i);
#endif
		// H.Real = H.Real*Hlin.Scale/32768
		if (diagflag) {
			/*
			if (i <= 31) { // upstream
				fp=(hlog[i+chan]*hlog[chan*3]*100)/32768;
			}
			else { // downstream
				fp=(hlog[i+chan]*hlog[chan*3+1]*100)/32768;
			}
			*/
			intp = hlog[i+chan]/1000;
			fp = hlog[i+chan]%1000;
			if (fp<0) {
				fp = -fp;
				if (intp == 0)
					snprintf(str, 16, "-0.%03d", fp);
				else
					snprintf(str, 16, "%d.%03d", intp, fp);
			}
			else
				snprintf(str, 16, "%d.%03d", intp, fp);
		}
#ifndef CONFIG_GENERAL_WEB
		nBytesSent += boaWrite(wp, "<td align=center bgcolor=#f0f0f0><font size=2>%s</font></td>\n", str);
#else
		nBytesSent += boaWrite(wp, "<td align=center>%s</td>\n", str);
#endif
		// H.Image = H.Image*Hlin.Scale/32768
		if (diagflag) {
			/*
			if (i <= 31) { // upstream
				fp=(hlog[i+chan*2]*hlog[chan*3]*100)/32768;
			}
			else { // downstream
				fp=(hlog[i+chan*2]*hlog[chan*3+1]*100)/32768;
			}
			*/
			intp = hlog[i+chan*2]/1000;
			fp = hlog[i+chan*2]%1000;
			if (fp<0) {
				fp = -fp;
				if (intp == 0)
					snprintf(str, 16, "-0.%03d", fp);
				else
					snprintf(str, 16, "%d.%03d", intp, fp);
			}
			else
				snprintf(str, 16, "%d.%03d", intp, fp);
		}
#ifndef CONFIG_GENERAL_WEB
		nBytesSent += boaWrite(wp, "<td align=center bgcolor=#f0f0f0><font size=2>%s</font></td>\n", str);
#else
		nBytesSent += boaWrite(wp, "<td align=center>%s</td>\n", str);
#endif
		//nBytesSent += boaWrite(wp, "<td align=center bgcolor=#f0f0f0>%d</td>\n", 0);
		// snr = -32+(snr/2)
		if (diagflag) {
			/*
			ival = snr[i] * 5; // *10/2
			intp = ival/10-32;
			fp = ival % 10;
			*/
			intp = snr[i]/10;
			fp = abs(snr[i]%10);
			snprintf(str, 16, "%d.%d", intp, fp);
		}
#ifndef CONFIG_GENERAL_WEB
		nBytesSent += boaWrite(wp, "<td align=center bgcolor=#f0f0f0><font size=2>%s</font></td>\n", str);
#else
		nBytesSent += boaWrite(wp, "<td align=center>%s</td>\n", str);
#endif
		// qln = -23-(qln/2)
		if (diagflag) {
			/*
			ival = qln[i] * 5;
			intp = ival/10+23;
			fp = ival % 10;
			*/
			intp = qln[i]/10;
			fp = qln[i]%10;
			if (fp<0) {
				if (intp != 0)
					snprintf(str, 16, "%d.%d", intp, -fp);
				else
					snprintf(str, 16, "-%d.%d", intp, -fp);
			}
			else
				snprintf(str, 16, "%d.%d", intp, fp);
		}
#ifndef CONFIG_GENERAL_WEB
		nBytesSent += boaWrite(wp, "<td align=center bgcolor=#f0f0f0><font size=2>%s</font></td>\n", str);
#else
		nBytesSent += boaWrite(wp, "<td align=center>%s</td>\n", str);
#endif
		// hlog = 6-(hlog/10)
		if (diagflag) {
			/*
			ival = hlog[i]/10;
			if (ival >= 6) {// negative value
				intp = ival - 6;
				fp = hlog[i] % 10;
				nBytesSent += boaWrite(wp, "<td align=center bgcolor=#f0f0f0>-%d.%d</td>\n</tr>\n", intp, fp);
			}
			else { //positive value
				intp = 6- ival;
				ival = hlog[i] % 10;
				if (ival != 0)
					intp--;
				fp = 10 - ival;
				nBytesSent += boaWrite(wp, "<td align=center bgcolor=#f0f0f0>%d.%d</td>\n</tr>\n", intp, fp);
			}
			*/
			intp = hlog[i]/10;
			fp = hlog[i]%10;
			if (fp<0) {
				if (intp != 0)
					snprintf(str, 16, "%d.%d", intp, -fp);
				else
					snprintf(str, 16, "-%d.%d", intp, -fp);
			}
			else
				snprintf(str, 16, "%d.%d", intp, fp);
		}
		/*
		else
			nBytesSent += boaWrite(wp, "<td align=center bgcolor=#f0f0f0>%d.%d</td>\n</tr>\n", intp, fp);
		*/
#ifndef CONFIG_GENERAL_WEB
		nBytesSent += boaWrite(wp, "<td align=center bgcolor=#f0f0f0><font size=2>%s</font></td>\n</tr>\n", str);
#else
		nBytesSent += boaWrite(wp, "<td align=center>%s</td>\n</tr>\n", str);
#endif
	}
#ifdef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "</table>\n</div>\n");
#endif
	free(snr);
	free(qln);
	free(hlog);

	return nBytesSent;
}
#endif /*CONFIG_VDSL*/

int adslToneDiagList(int eid, request * wp, int argc, char **argv)
{
	XDSL_OP *d;
#ifdef CONFIG_USER_XDSL_SLAVE
	//printf( "\n%s: %s\n", __FUNCTION__, argc?"slave":"" );
	if(argc)
	{
		d=xdsl_get_op(1);
	}else
#endif /*CONFIG_USER_XDSL_SLAVE*/
	{
		d=xdsl_get_op(0);
	}

	return _adslToneDiagList(wp, d);
}

int adslToneConfDiagList(int eid, request * wp, int argc, char **argv)
{
	short mode;
	int i, chan;
	int nBytesSent=0;
	unsigned char tone[64];    // Added by Mason Yu for correct Tone Mib Type
	int onbit;


	memset(tone, 0, sizeof(tone));

	// get the channel number
	mib_get_s(MIB_ADSL_MODE, (void *)&mode, sizeof(mode));
	if (mode & ADSL_MODE_ADSL2P)
		chan = 512;	// ADSL2+
	else
		chan = 256;	// ADSL, ADSL2

	if ( !mib_get_s(MIB_ADSL_TONE, (void *)tone, sizeof(tone))) {
		printf(strGetToneerror);
	}

#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr>\n<th width=25%% bgcolor=#c0c0c0>%s</th>\n",
			multilang(LANG_TONE_NUMBER));
	nBytesSent += boaWrite(wp, "<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td></tr>\n",
			multilang(LANG_SELECT));
#else
	nBytesSent += boaWrite(wp, "<div class=\"data_common data_common_notitle\">\n<table>\n");
	nBytesSent += boaWrite(wp, "<tr>\n<th width=25%%>%s</th>\n",
			multilang(LANG_TONE_NUMBER));
	nBytesSent += boaWrite(wp, "<td align=center width=\"20%%\">%s</td></tr>\n",
			multilang(LANG_SELECT));
#endif
	for (i = 0; i < chan; i++) {
		//onbit =(tone[i/8] >> (i%8) ) & 0x01;
		onbit =(tone[i/8] >> (7-(i%8)) ) & 0x01;
#ifndef CONFIG_GENERAL_WEB
		nBytesSent += boaWrite(wp, "<tr>\n<th bgcolor=#c0c0c0>%d</th>\n", i);
		if (onbit == 1)
			nBytesSent += boaWrite(wp, "<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\" checked></td></tr>\n", i);
		else
			nBytesSent += boaWrite(wp, "<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n", i);
#else
		nBytesSent += boaWrite(wp, "<tr>\n<th>%d</th>\n", i);
		if (onbit == 1)
			nBytesSent += boaWrite(wp, "<td align=center width=\"20%%\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\" checked></td></tr>\n", i);
		else
			nBytesSent += boaWrite(wp, "<td align=center width=\"20%%\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n", i);
#endif
	}
#ifdef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "</table></div>");
#endif
	return nBytesSent;
}

int adslPSDMaskTbl(int eid, request * wp, int argc, char **argv)
{
	int i;
	int nBytesSent=0;
	char *strChecked;
	char strTone[16];
	char strUS[16];


	/* send header */
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr>\n<th align=center width=\"10%%\" bgcolor=\"#B0B0B0\">%s</th>\n", multilang(LANG_ENABLE));
	nBytesSent += boaWrite(wp, "<th align=center width=\"10%%\" bgcolor=\"#B0B0B0\">%s (0-63)</th>\n", multilang(LANG_TONE));
	nBytesSent += boaWrite(wp, "<th align=center width=\"70%%\" bgcolor=\"#B0B0B0\">dBm/Hz</th></tr>\n");
#else
	nBytesSent += boaWrite(wp,"<div class=\"data_common data_common_notitle\">\n<table>");
	nBytesSent += boaWrite(wp, "<tr>\n<th align=center width=\"10%%\">%s</th>\n", multilang(LANG_ENABLE));
	nBytesSent += boaWrite(wp, "<th align=center width=\"10%%\">%s (0-63)</th>\n", multilang(LANG_TONE));
	nBytesSent += boaWrite(wp, "<th align=center width=\"70%%\">dBm/Hz</th></tr>\n");
#endif
	for (i = 0; i < 8; i++) {

		if (psd_bit_en & (1<<i)) {
			strChecked = "checked";
		} else {
			strChecked = "";
		}
		snprintf(strTone, sizeof(strTone), "value=%d", psd_tone[i]);
		snprintf(strUS, sizeof(strUS), "value=%.1f", psd_us[i]);
#ifndef CONFIG_GENERAL_WEB
		nBytesSent += boaWrite(wp, "<tr><td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"s%d\" value=\"1\" %s></td>\n",  i, strChecked);
		nBytesSent += boaWrite(wp, "<td align=center width=\"30%%\" bgcolor=\"#C0C0C0\"><input type=\"text\" maxLength=2 size=\"4\" name=\"t%d\" %s></td>\n", i, strTone);
		nBytesSent += boaWrite(wp, "<td align=center width=\"60%%\" bgcolor=\"#C0C0C0\"><input type=\"text\" size=\"8\" name=\"f%d\" %s></td></tr>\n", i, strUS);
#else
		nBytesSent += boaWrite(wp, "<tr><td align=center width=\"10%%\"><input type=\"checkbox\" name=\"s%d\" value=\"1\" %s></td>\n",  i, strChecked);
		nBytesSent += boaWrite(wp, "<td align=center width=\"30%%\"><input type=\"text\" maxLength=2 size=\"4\" name=\"t%d\" %s></td>\n", i, strTone);
		nBytesSent += boaWrite(wp, "<td align=center width=\"60%%\"><input type=\"text\" size=\"8\" name=\"f%d\" %s></td></tr>\n", i, strUS);
		nBytesSent += boaWrite(wp, "</table>\n</div>\n");
#endif
	}

	return nBytesSent;
}

void formSetAdslPSD(request * wp, char *path, char *query)
{
	char *submitUrl;
	int i;
	char tmpBuf[100];
	unsigned char bits = 0;
	int tone = 0;
	float us;
	UsPSDData PSD;

	char *strVal=0, *strApply;

	strApply = boaGetVar(wp, "apply", "");

	/* user clicked apply*/
	if (strApply[0]) {
		for (i=0; i<8; i++) {
			snprintf(tmpBuf, sizeof(tmpBuf), "s%d", i);
			strVal = boaGetVar(wp, tmpBuf, "");

			if (strVal && !gstrcmp(strVal, "1") ) {
				if (tone < 0) {
					snprintf(tmpBuf, sizeof(tmpBuf), multilang(LANG_S_SHOULD_BE_0_63), strVal);
					goto setErr_tone;
				}

				bits |= (1<<i);
			}

			tone = -1;

			snprintf(tmpBuf, sizeof(tmpBuf), "t%d", i);
			strVal = boaGetVar(wp, tmpBuf, "");
			if ((bits & (1 << i)) && strVal) {
				if (1 != sscanf(strVal, "%d", &tone)) {
					snprintf(tmpBuf, sizeof(tmpBuf), multilang(LANG_TD_S_NOT_A_NUMBER), i, strVal);
					goto setErr_tone;
				}

				if ((tone > 63) || (tone < 0)) {
					snprintf(tmpBuf, sizeof(tmpBuf), multilang(LANG_S_SHOULD_BE_0_63), strVal);
					goto setErr_tone;
				}

				psd_tone[i] = tone;
			}

			if (!(bits & (1 << i)))
				continue;

			snprintf(tmpBuf, sizeof(tmpBuf), "f%d", i);
			strVal = boaGetVar(wp, tmpBuf, "");
			if (strVal && (sscanf(strVal, "%f", &us)==1)) {
				psd_us[i] = us;
			}

		}

		psd_bit_en = bits;
		goto setOk_tone;
	}

setOk_tone:

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	/*
	fprintf(stderr, "psd_bit_en: %x  psd_measure: %d\n", psd_bit_en, psd_measure);
	for (i=0;i<8;i++) {
		fprintf(stderr, "psd_tone[%d]=%d, psd_us[%d]=%.1f\n", i,psd_tone[i],i,psd_us[i]);
	}
	*/
	#if SUPPORT_TR105
	for (i=0;i<8;i++) {
		if (bits & (1 << i)) {
			PSD.breakFreq_array_us[i] = psd_tone[i];
			PSD.MIB_PSD_us[i] = psd_us[i];
		} else {
			PSD.breakFreq_array_us[i] = 513; // means disabled.
		}
	}
	if (adsl_drv_get(RLCM_WEB_SET_USPSD, (void *)&PSD, sizeof(PSD))) {
	}
	#endif

	OK_MSG(submitUrl);

  	return;

setErr_tone:
	ERR_MSG(tmpBuf);
}

int adslPSDMeasure(int eid, request * wp, int argc, char **argv) {
	int nBytesSent=0;
	const char *Value;

	Value = multilang((psd_measure) ? LANG_DISABLE : LANG_ENABLE);

	nBytesSent += boaWrite(wp, "<input type=submit value=\"%s\" name=\"psdm\">", Value);
	return nBytesSent;
}


//cathy
static void _DSLuptime(request * wp, XDSL_OP *d)
{
	Modem_LinkSpeed vLs;
	unsigned char adslflag;
	struct sysinfo info, *up;
	int updays, uphours, upminutes, upsec;
	unsigned int synchronized_time[3]={0};

	// check for xDSL link
	if (!d->xdsl_drv_get(RLCM_GET_LINK_SPEED, (void *)&vLs, RLCM_GET_LINK_SPEED_SIZE) || vLs.upstreamRate == 0)
		adslflag = 0;
	else
		adslflag = 1;

	if( adslflag ) {
		d->xdsl_drv_get(RLCM_GET_DSL_ORHERS, synchronized_time, 3*sizeof(int));
		updays = (int) synchronized_time[0] / (60*60*24);
		if (updays)
			boaWrite (wp, "%d day%s,&nbsp;", updays, (updays != 1) ? "s" : "");
		upsec = (int) synchronized_time[0] % 60;
		upminutes = (int) synchronized_time[0] / 60;
		uphours = (upminutes / 60) % 24;
		upminutes %= 60;
		boaWrite (wp, "%02d:%02d:%02d\n", uphours, upminutes, upsec);
	}
	else {
		boaWrite(wp, "&nbsp;");
	}
}

int DSLuptime(int eid, request * wp, int argc, char **argv)
{
	_DSLuptime(wp, xdsl_get_op(0) );
}

#ifdef CONFIG_USER_XDSL_SLAVE
int DSLSlvuptime(int eid, request * wp, int argc, char **argv)
{
	_DSLuptime(wp, xdsl_get_op(1) );
}
#endif /*CONFIG_USER_XDSL_SLAVE*/
#endif // of CONFIG_DEV_xDSL


/////////////////////////////////////////////////////////////////////////////
void formStats(request * wp, char *path, char *query)
{
	char *strValue, *submitUrl, ret=0;
#ifdef WLAN_SUPPORT
	int i, j;
	char str_tmp[64];
#endif
	strValue = boaGetVar(wp, "reset", "");	//cathy, reset stats
	if(strValue[0]) {
		if(strValue[0] - '0') {	//reset
		set_wan_crc_counter(0);
#ifdef EMBED
		if (access("/proc/realtek/netdev_reset", F_OK) == 0)
		{
			system("echo all > /proc/realtek/netdev_reset");

			system("diag mib reset counter port all");
		}
		/* reset statistics for wifi devices */
#ifdef WLAN_SUPPORT
		for (i=0; i<NUM_WLAN_INTERFACE; i++) {
			/* wifi root */
			sprintf(str_tmp, "/proc/wlan%d/stats", i);
			if (access(str_tmp, F_OK) == 0) {
				sprintf(str_tmp, "echo 0 > /proc/wlan%d/stats", i);
				system(str_tmp);
			}
			/* wifi vap */
#ifdef WLAN_MBSSID
			for (j=0; j<WLAN_MBSSID_NUM; j++)
			{
				sprintf(str_tmp, "/proc/wlan%d-vap%d/stats", i, j);
				if (access(str_tmp, F_OK) == 0) {
					sprintf(str_tmp, "echo 0 > /proc/wlan%d-vap%d/stats", i, j);
					system(str_tmp);
				}
			}
#endif
		}
#endif
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
		rtk_if_statistic_reset();
		ret = rt_stat_init();
#endif
#ifdef WLAN_QTN
		va_cmd("/bin/qcsapi_sockrpc", 4, 1, "reset_all_stats", "wifi0", "0", "0");
#endif
#endif
#ifdef CONFIG_SFU
#ifdef CONFIG_COMMON_RT_API
		ret = rt_stat_init();
#else
		ret = rtk_stat_init();
#endif
		if(ret != 0)
#ifdef CONFIG_COMMON_RT_API
			printf("%s-%d rt_stat_init error %d\n",__func__,__LINE__,ret);
#else
			printf("%s-%d rtk_stat_init error %d\n",__func__,__LINE__,ret);
#endif
#endif
		}
	}

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page

	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
}

// List the packet statistics for all interfaces at web page.
int pktStatsList(int eid, request * wp, int argc, char **argv)
{
	//ql_xu add: for auto generate wan name
	int entryNum;
	int mibcnt;
	MIB_CE_ATM_VC_T Entry;
	int i;
	int ret=0, logID=0;
	const char *names[16];
	char ifname[IFNAMSIZ];
	char ifDisplayName[IFNAMSIZ];
	int num_itf;
	struct net_device_stats nds;
	int change_idx;
#ifdef WLAN_SUPPORT
	MIB_CE_MBSSIB_T wlan_entry;
	int orig_wlan_idx;
#endif
	char tx_str[32];
	char rx_str[32];
	char tx_bpsstr[32];
	char rx_bpsstr[32];

	num_itf=0;
	for (i=0; i<ELANVIF_NUM; i++) {
		names[i] = (char *)ELANRIF[i];
		num_itf++;
	}
#ifdef CONFIG_USB_ETH
	names[num_itf++] = (char *)USBETHIF;
#endif
#ifdef WLAN_SUPPORT
{
    int k;
    orig_wlan_idx = wlan_idx;
    for (k=0; k<NUM_WLAN_INTERFACE; k++) {
        wlan_idx = k;
        names[num_itf++] = WLANIF[k];
#ifdef WLAN_MBSSID
        for(mibcnt=1;mibcnt<=WLAN_MBSSID_NUM;mibcnt++){
            if (!mib_chain_get(MIB_MBSSIB_TBL, mibcnt, &wlan_entry)){
                printf("MIB_MBSSIB_TBL get failed\n");
                continue;
            }
            if(wlan_entry.wlanDisabled==0)
                names[num_itf++] = wlan[mibcnt + (k*(1+MAX_WLAN_VAP))];
        }
#endif
    }
    wlan_idx = orig_wlan_idx;
}
#endif

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
	"<td align=center width=\"8%%\" bgcolor=\"#808080\" class=\"table_item\">%s</td>\n"
	"<td align=center width=\"10%%\" bgcolor=\"#808080\" class=\"table_item\">%s</td>\n"
	"<td align=center width=\"9%%\" bgcolor=\"#808080\" class=\"table_item\">%s</td>\n"
	"<td align=center width=\"6%%\" bgcolor=\"#808080\" class=\"table_item\">%s</td>\n"
	"<td align=center width=\"6%%\" bgcolor=\"#808080\" class=\"table_item\">%s</td>\n"
	"<td align=center width=\"10%%\" bgcolor=\"#808080\" class=\"table_item\">%s</td>\n"
	"<td align=center width=\"9%%\" bgcolor=\"#808080\" class=\"table_item\">%s</td>\n"
	"<td align=center width=\"6%%\" bgcolor=\"#808080\" class=\"table_item\">%s</td>\n"
	"<td align=center width=\"6%%\" bgcolor=\"#808080\" class=\"table_item\">%s</td></tr>\n",
#else
	"<th align=center width=\"8%%\">%s</th>\n"
	"<th align=center width=\"10%%\">%s</th>\n"
	"<th align=center width=\"9%%\">%s</th>\n"
	"<th align=center width=\"6%%\">%s</th>\n"
	"<th align=center width=\"6%%\">%s</th>\n"
	"<th align=center width=\"10%%\">%s</th>\n"
	"<th align=center width=\"9%%\">%s</th>\n"
	"<th align=center width=\"6%%\">%s</th>\n"
	"<th align=center width=\"6%%\">%s</th></tr>\n",
#endif
	multilang(LANG_INTERFACE), multilang(LANG_RX_BYTES), multilang(LANG_RX_BPS), multilang(LANG_RX_ERR), multilang(LANG_RX_DROP),
	multilang(LANG_TX_BYTES), multilang(LANG_TX_BPS), multilang(LANG_TX_ERR), multilang(LANG_TX_DROP));

	// LAN statistics
	#ifdef CONFIG_SFU
#ifdef CONFIG_COMMON_RT_API
	rt_stat_port_cntr_t portcnt;
	ret = rt_stat_port_getAll(0, &portcnt);
#else
	rtk_stat_port_cntr_t portcnt;
	ret = rtk_stat_port_getAll(0, &portcnt);
#endif
	if(ret)
#ifdef CONFIG_COMMON_RT_API
		printf("%s-%d rt_stat_port_getAll error %d\n",__func__,__LINE__,ret);
#else
		printf("%s-%d rtk_stat_port_getAll error %d\n",__func__,__LINE__,ret);
#endif
	boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
			"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%lu</td>\n"
			"<td align=center width=\"9%%\" bgcolor=\"#C0C0C0\">%lu</td>\n"
			"<td align=center width=\"6%%\" bgcolor=\"#C0C0C0\">%lu</td>\n"
			"<td align=center width=\"6%%\" bgcolor=\"#C0C0C0\">%lu</td>\n"
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%lu</td>\n"
			"<td align=center width=\"9%%\" bgcolor=\"#C0C0C0\">%lu</td>\n"
			"<td align=center width=\"6%%\" bgcolor=\"#C0C0C0\">%lu</td>\n"
			"<td align=center width=\"6%%\" bgcolor=\"#C0C0C0\">%lu</td></tr>\n",
#else
			"<td align=center width=\"8%%\">%s</td>\n"
			"<td align=center width=\"15%%\">%lu</td>\n"
			"<td align=center width=\"8%%\">%lu</td>\n"
			"<td align=center width=\"8%%\">%lu</td>\n"
			"<td align=center width=\"15%%\">%lu</td>\n"
			"<td align=center width=\"8%%\">%lu</td>\n"
			"<td align=center width=\"8%%\">%lu</td></tr>\n",
#endif
			"LAN",
			 portcnt.ifInOctets, 0,
			 (portcnt.dot3StatsSymbolErrors + portcnt.dot3ControlInUnknownOpcodes),
			 portcnt.dot1dTpPortInDiscards,
			portcnt.ifOutOctets, 0,
			0,
			portcnt.ifOutDiscards);

	#else
	for (i = 0; i < num_itf; i++) {
#ifdef USE_DEONET /* sghong, 20230906 */
		rt_stat_port_cntr_t portcnt;
		struct port_statistics_t bps_portcnt;

		if (i == 0) change_idx = 3;
		else if (i == 1) change_idx = 2;
		else if (i == 2) change_idx = 1;
		else if (i == 3) change_idx = 0;
		else change_idx = i;

		memset(&portcnt, 0, sizeof(portcnt));
		memset(tx_str, 0, sizeof(tx_str));
		memset(rx_str, 0, sizeof(rx_str));
		memset(tx_bpsstr, 0, sizeof(tx_bpsstr));
		memset(rx_bpsstr, 0, sizeof(rx_bpsstr));

		ret = rt_stat_port_getAll(change_idx, &portcnt);

		deo_port_statistics_get(change_idx, &bps_portcnt);
		// DBG_ARG("\n%s %d LAN:%d, %d  RXbyte[%lu] TXByte[%lu] ", __func__, __LINE__, i+1, change_idx, bps_portcnt.bytesReceived, bps_portcnt.bytesSent);
#else
		get_net_device_stats(names[i], &nds);
#endif
		snprintf(ifname, sizeof(ifname), "%s", names[i]);
		logID = rtk_lan_LogID_get_by_if_name(ifname);
		if (logID >=0 )
			sprintf(ifDisplayName, "LAN%d", virt2user[logID]);
		else
			sprintf(ifDisplayName, "%s", names[i]);

		deo_change_with_commas(portcnt.ifInOctets, rx_str);
		deo_change_with_commas(((unsigned long long)(bps_portcnt.bytesReceived * 8)), rx_bpsstr);
		deo_change_with_commas(portcnt.ifOutOctets, tx_str);
		deo_change_with_commas(((unsigned long long)(bps_portcnt.bytesSent * 8)), tx_bpsstr);

		boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
			"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"9%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"6%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"6%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"9%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"6%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"6%%\" bgcolor=\"#C0C0C0\">%llu</td></tr>\n",
#else
#ifdef CONFIG_00R0
			"<td align=center width=\"8%%\">%s%d</td>\n"
#else
			"<td align=center width=\"8%%\">%s</td>\n"
#endif
			"<td align=center width=\"13%%\">%s</td>\n"
			"<td align=center width=\"13%%\">%s</td>\n"
			"<td align=center width=\"8%%\">%lu</td>\n"
			"<td align=center width=\"8%%\">%lu</td>\n"
			"<td align=center width=\"13%%\">%s</td>\n"
			"<td align=center width=\"13%%\">%s</td>\n"
			"<td align=center width=\"8%%\">%lu</td>\n"
			"<td align=center width=\"8%%\">%lu</td></tr>\n",
#endif
#ifdef CONFIG_00R0
			"LAN",i+1,
#else
			ifDisplayName,
#endif
#ifdef USE_DEONET /* sghong, 20230906 */
			rx_str, rx_bpsstr,
			// portcnt.ifInOctets, (bps_portcnt.bytesReceived * 8),
			(portcnt.dot3StatsSymbolErrors + portcnt.dot3ControlInUnknownOpcodes + portcnt.etherStatsCRCAlignErrors),
			portcnt.dot1dTpPortInDiscards,
			tx_str, tx_bpsstr,
			// portcnt.ifOutOctets, (bps_portcnt.bytesSent * 8),
			0, portcnt.ifOutDiscards);
#else
			nds.rx_bytes, nds.rx_errors, nds.rx_dropped,
			nds.tx_bytes, nds.tx_errors, nds.tx_dropped);
#endif
	}
	#endif//!CONFIG_SFU

#ifdef EMBED
#ifdef CONFIG_DEV_xDSL
if(WAN_MODE & MODE_ATM)
{
	for (mibcnt = 0; mibcnt < entryNum; mibcnt++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, mibcnt, &Entry)) {
			printf("MULTI_ATM_WAN: get MIB chain error\n");
			continue;
		}

		if (Entry.enable == 0)
			continue;

		if(MEDIA_INDEX(Entry.ifIndex) != MEDIA_ATM)
			continue;

		//generate wan name
		ifGetName(Entry.ifIndex, ifname, sizeof(ifname));

		get_net_device_stats(ifname, &nds);
		getDisplayWanName(&Entry, ifDisplayName);

		boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
			"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
			"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td></tr>\n",
#else
			"<td align=center width=\"8%%\">%s</td>\n"
			"<td align=center width=\"15%%\">%llu</td>\n"
			"<td align=center width=\"8%%\">%llu</td>\n"
			"<td align=center width=\"8%%\">%llu</td>\n"
			"<td align=center width=\"15%%\">%llu</td>\n"
			"<td align=center width=\"8%%\">%llu</td>\n"
			"<td align=center width=\"8%%\">%llu</td></tr>\n",
#endif
			ifDisplayName, nds.rx_packets,
			nds.rx_errors, nds.rx_dropped,
			nds.tx_packets, nds.tx_errors,
			nds.tx_dropped);
	}
}
#endif	// CONFIG_DEV_xDSL

#ifdef CONFIG_PTMWAN
if(WAN_MODE & MODE_PTM)
{
	for (mibcnt = 0; mibcnt < entryNum; mibcnt++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, mibcnt, &Entry)) {
			printf("MULTI_PTM_WAN: get MIB chain error\n");
			continue;
		}

		if (Entry.enable == 0)
			continue;

		if(MEDIA_INDEX(Entry.ifIndex) != MEDIA_PTM)
			continue;

		//generate wan name
		ifGetName(Entry.ifIndex, ifname, sizeof(ifname));

		get_net_device_stats(ifname, &nds);
		getDisplayWanName(&Entry, ifDisplayName);

		boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
			"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
			"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td></tr>\n",
#else
			"<td align=center width=\"8%%\">%s</td>\n"
			"<td align=center width=\"15%%\">%llu</td>\n"
			"<td align=center width=\"8%%\">%llu</td>\n"
			"<td align=center width=\"8%%\">%llu</td>\n"
			"<td align=center width=\"15%%\">%llu</td>\n"
			"<td align=center width=\"8%%\">%llu</td>\n"
			"<td align=center width=\"8%%\">%llu</td></tr>\n",
#endif
			ifDisplayName, nds.rx_packets,
			nds.rx_errors, nds.rx_dropped,
			nds.tx_packets, nds.tx_errors,
			nds.tx_dropped);
	}
}
#endif	// CONFIG_PTMWAN


#if defined(CONFIG_ETHWAN) && !defined(CONFIG_SFU)
if(WAN_MODE & MODE_Ethernet)
{
	for (mibcnt = 0; mibcnt < entryNum; mibcnt++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, mibcnt, &Entry)) {
			printf("MULTI_ETH_WAN: get MIB chain error\n");
			continue;
		}

		if (deo_wan_nat_mode_get() != DEO_BRIDGE_MODE) {
			if (mibcnt >= 1)
				continue;
		}

		if (Entry.enable == 0)
			continue;

		if(MEDIA_INDEX(Entry.ifIndex) != MEDIA_ETH)
			continue;

		//generate wan name
		ifGetName(Entry.ifIndex, ifname, sizeof(ifname));

#ifdef USE_DEONET /* sghong, 20230906 */
		rt_stat_port_cntr_t portcnt;
		struct port_statistics_t bps_portcnt;

		memset(&portcnt, 0, sizeof(portcnt));
		memset(tx_str, 0, sizeof(tx_str));
		memset(rx_str, 0, sizeof(rx_str));
		memset(tx_bpsstr, 0, sizeof(tx_bpsstr));
		memset(rx_bpsstr, 0, sizeof(rx_bpsstr));

		ret = rt_stat_port_getAll(RTK_WAN_PORT, &portcnt); /* RTK_WAN_PORT:5 */

		deo_port_statistics_get(4, &bps_portcnt); /* idx : 0 ~ 4 */
		// DBG_ARG("\n%s %d WAN  RXbyte[%lu] TXByte[%lu] CRC[%lu] ", __func__, __LINE__, bps_portcnt.bytesReceived, bps_portcnt.bytesSent, bps_portcnt.crc);
#else
		get_net_device_stats(ifname, &nds);
#endif
		getDisplayWanName(&Entry, ifDisplayName);

		if (strncmp(ifDisplayName, "nas0", 4) == 0)
			sprintf(ifDisplayName, "%s", "WAN");

		deo_change_with_commas(portcnt.ifInOctets, rx_str);
		deo_change_with_commas(((unsigned long long)(bps_portcnt.bytesReceived * 8)), rx_bpsstr);
		deo_change_with_commas(portcnt.ifOutOctets, tx_str);
		deo_change_with_commas(((unsigned long long)(bps_portcnt.bytesSent * 8)), tx_bpsstr);

		boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
			"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"9%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"6%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"6%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"9%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"6%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
			"<td align=center width=\"6%%\" bgcolor=\"#C0C0C0\">%llu</td></tr>\n",
#else
			"<td align=center width=\"8%%\">%s</td>\n"
			"<td align=center width=\"13%%\">%s</td>\n"
			"<td align=center width=\"13%%\">%s</td>\n"
			"<td align=center width=\"8%%\">%lu</td>\n"
			"<td align=center width=\"8%%\">%lu</td>\n"
			"<td align=center width=\"13%%\">%s</td>\n"
			"<td align=center width=\"13%%\">%s</td>\n"
			"<td align=center width=\"8%%\">%lu</td>\n"
			"<td align=center width=\"8%%\">%lu</td></tr>\n",
#endif
			ifDisplayName,
#ifdef USE_DEONET /* sghong, 20230906 */
			rx_str, rx_bpsstr,
			// portcnt.ifInOctets, (bps_portcnt.bytesReceived * 8),
#if 0
			(portcnt.dot3StatsSymbolErrors + portcnt.dot3ControlInUnknownOpcodes),
#else
			bps_portcnt.crc,
#endif
			portcnt.dot1dTpPortInDiscards,
			tx_str, tx_bpsstr,
			// portcnt.ifOutOctets, (bps_portcnt.bytesSent * 8),
			0, portcnt.ifOutDiscards);
#else
			nds.rx_bytes,
			nds.rx_errors, nds.rx_dropped,
			nds.tx_bytes, nds.tx_errors,
			nds.tx_dropped);
#endif
	}
}
#endif	// CONFIG_ETHWAN
#endif	// EMBED

#ifdef CONFIG_USER_PPPOMODEM
	{
		MIB_WAN_3G_T entry, *p;

		p = &entry;
		if (mib_chain_get(MIB_WAN_3G_TBL, 0, p)) {
			if (p->enable) {
				sprintf(ifname, "ppp%d", MODEM_PPPIDX_FROM);
				get_net_device_stats(ifname, &nds);

				boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
					"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">3G_%s</td>\n"
					"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
					"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
					"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
					"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
					"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
					"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td></tr>\n",
#else
					"<td align=center width=\"8%%\">3G_%s</td>\n"
					"<td align=center width=\"15%%\">%llu</td>\n"
					"<td align=center width=\"8%%\">%llu</td>\n"
					"<td align=center width=\"8%%\">%llu</td>\n"
					"<td align=center width=\"15%%\">%llu</td>\n"
					"<td align=center width=\"8%%\">%llu</td>\n"
					"<td align=center width=\"8%%\">%llu</td></tr>\n",
#endif
					ifname, nds.rx_packets,
					nds.rx_errors, nds.rx_dropped,
					nds.tx_packets, nds.tx_errors,
					nds.tx_dropped);
			}
		}
	}
#endif //CONFIG_USER_PPPOMODEM

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	{
		MIB_PPTP_T Entry;
		unsigned int entryNum, i;

		entryNum = mib_chain_total(MIB_PPTP_TBL);
		for (i=0; i<entryNum; i++)
		{
			char mppp_ifname[IFNAMSIZ];

			if (!mib_chain_get(MIB_PPTP_TBL, i, (void *)&Entry))
			{
				printf("Get MIB_PPTP_TBL chain record error!\n");
				continue;
			}

			ifGetName(Entry.ifIndex, mppp_ifname, sizeof(mppp_ifname));
			snprintf(ifname, 15, "%s_pptp%d", mppp_ifname, Entry.idx);
			get_net_device_stats(mppp_ifname, &nds);

			boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
				"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td></tr>\n",
#else
				"<td align=center width=\"8%%\">%s</td>\n"
				"<td align=center width=\"15%%\">%llu</td>\n"
				"<td align=center width=\"8%%\">%llu</td>\n"
				"<td align=center width=\"8%%\">%llu</td>\n"
				"<td align=center width=\"15%%\">%llu</td>\n"
				"<td align=center width=\"8%%\">%llu</td>\n"
				"<td align=center width=\"8%%\">%llu</td></tr>\n",
#endif
				ifname, nds.rx_packets,
				nds.rx_errors, nds.rx_dropped,
				nds.tx_packets, nds.tx_errors,
				nds.tx_dropped);
		}
	}
#endif

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	{
		MIB_L2TP_T Entry;
		unsigned int entryNum, i;

		entryNum = mib_chain_total(MIB_L2TP_TBL);
		for (i=0; i<entryNum; i++)
		{
			char mppp_ifname[IFNAMSIZ];

			if (!mib_chain_get(MIB_L2TP_TBL, i, (void *)&Entry))
			{
				printf("Get MIB_L2TP_TBL chain record error!\n");
				continue;
			}

			ifGetName(Entry.ifIndex, mppp_ifname, sizeof(mppp_ifname));
			snprintf(ifname, 15, "%s_l2tp%d", mppp_ifname, Entry.idx);
			get_net_device_stats(mppp_ifname, &nds);

			boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
				"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td></tr>\n",
#else
				"<td align=center width=\"8%%\">%s</td>\n"
				"<td align=center width=\"15%%\">%llu</td>\n"
				"<td align=center width=\"8%%\">%llu</td>\n"
				"<td align=center width=\"8%%\">%llu</td>\n"
				"<td align=center width=\"15%%\">%llu</td>\n"
				"<td align=center width=\"8%%\">%llu</td>\n"
				"<td align=center width=\"8%%\">%llu</td></tr>\n",
#endif
				ifname, nds.rx_packets,
				nds.rx_errors, nds.rx_dropped,
				nds.tx_packets, nds.tx_errors,
				nds.tx_dropped);
		}
	}
#endif

#ifdef CONFIG_NET_IPIP
	{
		MIB_IPIP_T Entry;
		unsigned int entryNum, i;

		entryNum = mib_chain_total(MIB_IPIP_TBL);
		for (i=0; i<entryNum; i++)
		{
			char ifname[IFNAMSIZ];

			if (!mib_chain_get(MIB_IPIP_TBL, i, (void *)&Entry))
			{
				printf("Get MIB_IPIP_TBL chain record error!\n");
				continue;
			}

			ifGetName(Entry.ifIndex, ifname, sizeof(ifname));
			get_net_device_stats(ifname, &nds);

			boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
				"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td>\n"
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%llu</td></tr>\n",
#else
	`			"<td align=center width=\"8%%\">%s</td>\n"
				"<td align=center width=\"15%%\">%llu</td>\n"
				"<td align=center width=\"8%%\">%llu</td>\n"
				"<td align=center width=\"8%%\">%llu</td>\n"
				"<td align=center width=\"15%%\">%llu</td>\n"
				"<td align=center width=\"8%%\">%llu</td>\n"
				"<td align=center width=\"8%%\">%llu</td></tr>\n",
#endif
				ifname, nds.rx_packets,
				nds.rx_errors, nds.rx_dropped,
				nds.tx_packets, nds.tx_errors,
				nds.tx_dropped);
		}
	}
#endif

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
void formRconfig(request * wp, char *path, char *query)
{
	char *strValue, *strSubmit, *uid, *upwd;
	char tmpBuf[100];
	char ipaddr[20], dport[10];
	char userName[MAX_NAME_LEN];
	int len;

	strSubmit = boaGetVar(wp, "yes", "");
	mib_get_s(MIB_ADSL_LAN_IP, (void *)tmpBuf, sizeof(tmpBuf));
	strncpy(ipaddr, inet_ntoa(*((struct in_addr *)tmpBuf)), 16);
	ipaddr[15] = '\0';
	len = sizeof(ipaddr) - strlen(ipaddr);
	if(snprintf(ipaddr + strlen(ipaddr), len, ":80") >= len) {
		printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
	}

	// enable
	if (strSubmit[0]) {
		mib_get_s( MIB_USER_NAME, (void *)userName, sizeof(userName) );
		if (userName[0] == '\0') {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_PROHIBIT_ADMINISTRATORS_PASSWORD_NOT_SET),sizeof(tmpBuf)-1);
			goto setErr_rconf;
		}

		strValue = boaGetVar(wp, "writable", "");
		if (strValue[0])	// allow update
			g_remoteUpdate = TRUE;
		else	// read only
			g_remoteUpdate = FALSE;

		strValue = boaGetVar(wp, "portFlag", "");
		if (strValue[0]) {	// default port 51003
			g_remoteAccessPort = 51003;
		}
		else {	// use randomly selected port
			if (!srandomCalled) {
				srandom(time(0));
				srandomCalled = 1;
			}
			g_remoteAccessPort = 50000 + (random()&0x00000fff);
		}

		sprintf(dport, "%d", g_remoteAccessPort);

		uid = boaGetVar(wp, "uid", "");
		upwd = boaGetVar(wp, "pwd", "");
		if (uid[0] != '\0' && upwd[0] != '\0') {
			/* Create user */
			if ( boaAddUser(uid, upwd, DEFAULT_GROUP, FALSE, FALSE) ) {
				error(E_L, E_LOG, "ERROR: Unable to add user account.");
				return;
			}
			else {
				g_rUserName[sizeof(g_rUserName)-1]='\0';
				strncpy(g_rUserName, uid,sizeof(g_rUserName)-1);
				g_rUserPass[sizeof(g_rUserPass)-1]='\0';
				strncpy(g_rUserPass, upwd,sizeof(g_rUserName)-1);
				// The remote access session MUST be started
				// within REMOTE_PASS_LIFE seconds.
				g_rexpire = time(0) + REMOTE_PASS_LIFE;
				g_rSessionStart = FALSE;
			}
		}
		else {
			g_rUserName[0] = '\0';
			g_rUserPass[0] = '\0';
		}

		// iptables -D INPUT -j block
		va_cmd(IPTABLES, 4, 1, (char *)FW_DEL, (char *)FW_INPUT, "-j", "block");
		// iptables -A INPUT -i ! br0 -p TCP --dport 80 -j ACCEPT
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_INPUT, "!", ARG_I, LANIF, "-p", ARG_TCP, FW_DPORT, "80", "-j", (char *)FW_ACCEPT);
		// iptables -A INPUT -j block
		va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", "block");

		// iptables -t nat -A PREROUTING -i ! $LAN_IF -p TCP --dport 51003 -j DNAT --to-destination ipaddr:80
		va_cmd(IPTABLES, 15, 1, "-t", "nat", (char *)FW_ADD,	"PREROUTING", "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP, (char *)FW_DPORT,
		dport, "-j", "DNAT", "--to-destination", ipaddr);
		g_remoteConfig = 1;
	}

	strSubmit = boaGetVar(wp, "no", "");
	// disable
	if (strSubmit[0]) {
		sprintf(dport, "%d", g_remoteAccessPort);

		// delete original user
		if (g_rUserName[0]) {
			if ( boaDeleteUser(g_rUserName) ) {
				printf("ERROR: Unable to delete user account (user=%s).\n", g_rUserName);
				return;
			}
			g_rUserName[0] = '\0';
		}

		// iptables -D INPUT -i ! br0 -p TCP --dport 80 -j ACCEPT
		va_cmd(IPTABLES, 11, 1, (char *)FW_DEL, (char *)FW_INPUT, "!", ARG_I, LANIF, "-p", ARG_TCP, FW_DPORT, "80", "-j", (char *)FW_ACCEPT);
		// iptables -t nat -D PREROUTING -i ! $LAN_IF -p TCP --dport 51003 -j DNAT --to-destination ipaddr:80
		va_cmd(IPTABLES, 15, 1, "-t", "nat", (char *)FW_DEL, "PREROUTING", "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP, (char *)FW_DPORT,
		dport, "-j", "DNAT", "--to-destination", ipaddr);
		g_remoteConfig = 0;
		g_remoteUpdate = FALSE;
	}

	strSubmit = boaGetVar(wp, "submit-url", "");   // hidden page
	boaRedirect(wp, strSubmit);
	return;

setErr_rconf:
	ERR_MSG(tmpBuf);
}

//for rc4 encryption
/****************************************/
/* rc4_encrypt()                        */
/****************************************/
#define RC4_INT unsigned int
#define MAX_MESSAGE_LENGTH 2048
unsigned char en_cipherstream[MAX_MESSAGE_LENGTH+1];
void xor_block(int length, unsigned char *a, unsigned char *b, unsigned char *out)
{
    int i;
    for (i=0;i<length; i++)
    {
        out[i] = a[i] ^ b[i];
    }
}
static __inline__ void swap(unsigned char *a, unsigned char *b)
{
    unsigned char tmp;

    tmp = *a;
    *a = *b;
    *b = tmp;
}
void rc4(
            unsigned char *key,
            int key_length,
            int cipherstream_length,
            unsigned char *cipherstream)
{
    int i, j, x;

    unsigned char s[256];
    unsigned char k[256];

    /* Create Key Stream */
    for (i=0; i<256; i++)
        k[i] = key[i % key_length];

    /* Initialize SBOX */
    for (j=0; j<256; j++)
        s[j] = j;

    /* Seed the SBOX */
    i = 0;
    for (j=0; j<256; j++)
    {
        i = (i + s[j] + k[j]) & 255;
        swap(&s[j], &s[i]);
    }

    /* Generate the cipherstream */
    j = 0;
    i = 0;

    for (x=0; x<cipherstream_length; x++)
    {
        j = (j + 1) & 255;
        i = (i + s[j]) & 255;
        swap(&s[j], &s[i]);
        cipherstream[x] = s[(s[j] + s[i]) & 255];
    };
}

void rc4_encrypt(
            unsigned char *key,
            int key_length,
            unsigned char *data,
            int data_length,
            unsigned char *ciphertext)
{

    rc4(key, key_length, data_length, en_cipherstream);
    xor_block(data_length, en_cipherstream, data, ciphertext);
}

int rc4_encry()
{
	// for rc4 ecncryption
	FILE *fp_in, *fp_out;
	char ciphertext[256];
	unsigned char rc4_key[256];
	int key_length;
	char LINE[256];
	int data_len;

	rc4_key[0] = 0;
	rc4_key[sizeof(rc4_key)-1]='\0';
	strncpy(rc4_key, "realteksd5adsl",sizeof(rc4_key)-1);
	key_length = strlen(rc4_key);
	rc4_key[key_length] = '\0';

	fp_in = fopen("/var/log/messages", "r");
	if(fp_in==NULL)
		printf("Error opening  file /var/log/messages \n");
	fp_out = fopen("/tmp/log.txt", "w");
	if(fp_out==NULL)
		printf("Error opening  file /tmp/log.txt \n");
	while (!feof(fp_in)) {
		data_len = fread(LINE, sizeof(char), 255, fp_in);
		LINE[data_len] = 0;
		//encryption
		rc4_encrypt(rc4_key, key_length, LINE, data_len, ciphertext);
		ciphertext[data_len] = 0;
		fwrite(ciphertext, sizeof(char), data_len, fp_out);
	}
	fclose(fp_out);
	fclose(fp_in);

	return 0;
}


#define _PATH_SYSCMD_LOG "/tmp/syscmd.log"
unsigned char first_time=1;
unsigned char dbg_enable=0;
void formSysCmd(request * wp, char *path, char *query)
{
	char  *submitUrl, *sysCmd,*strVal,*strRequest;
	unsigned char adsldbg;
	unsigned int maxFileSector;
//#ifndef NO_ACTION
	char tmpBuf[100];
//#endif

	if(first_time==1){
		mib_get_s(MIB_ADSL_DEBUG, (void *)&adsldbg, sizeof(adsldbg));
		if(adsldbg==1)
			dbg_enable=1;
		first_time=0;
	}
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	sysCmd = boaGetVar(wp, "sysCmd", "");   // hidden page

	strRequest = boaGetVar(wp, "save", "");
	if (strRequest[0])
	{
		PARAM_HEADER_T header;
//		unsigned char *ptr;
		unsigned int fileSize,filelen;
		unsigned int fileSector;

		wp->buffer_end=0; // clear header
		FILE *fp;
		//create config file
		rc4_encry();
		boaWrite(wp, "HTTP/1.0 200 OK\n");
		boaWrite(wp, "Content-Type: application/octet-stream;\n");

		boaWrite(wp, "Content-Disposition: attachment;filename=\"log\" \n");


		boaWrite(wp, "Pragma: no-cache\n");
		boaWrite(wp, "Cache-Control: no-cache\n");
		boaWrite(wp, "\n");

		fp=fopen("/tmp/log.txt","r");
		if(fp==NULL)
			printf("Error opening  file /tmp/log.txt \n");
		//decide the file size
		fseek(fp, 0, SEEK_END);
		filelen = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		fileSize=filelen;

		while(fileSize>0)
		{
			char buf[0x1000];
			int nRead;
			maxFileSector=0x500;
//			printf("write %d %d %08x\n",maxFileSector, fileSize, (unsigned int )ptr);
			fileSector = (fileSize>maxFileSector)?maxFileSector:fileSize;
			nRead = fread((void *)buf, 1, fileSector, fp);
			buf[nRead]=0;
			boaWriteDataNonBlock(wp, buf, nRead);
			//printf("%s",buf);
			fileSize -= fileSector;
//			ptr += fileSector;
 			//sleep(1);
		}
		fclose(fp);
		//boaDone(wp, 200);
		//OK_MSG("/saveconf.asp");
		return;
	}
//#ifndef NO_ACTION
	if(sysCmd[0]){
		snprintf(tmpBuf, 100, "%s 2>&1 > %s",sysCmd,  _PATH_SYSCMD_LOG);
		system(tmpBuf);
	}
//#endif


	strVal = boaGetVar(wp, "adsldbg", "");
	if(strVal[0]!=0){
		if (strVal[0]) {
			if (strVal[0] == '0'){
				adsldbg = 0;
			}
			else{
				adsldbg = 1;
			}
			if ( !mib_set(MIB_ADSL_DEBUG, (void *)&adsldbg)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			}
		}
	}

	boaRedirect(wp, submitUrl);
	return;
}

int sysCmdLog(int eid, request * wp, int argc, char **argv)
{
        FILE *fp;
	char  buf[150];
	int nBytesSent=0;

        fp = fopen(_PATH_SYSCMD_LOG, "r");
        if ( fp == NULL )
                goto err1;
        while(fgets(buf,150,fp)){
		nBytesSent += boaWrite(wp, "%s", buf);
        }
	fclose(fp);
	unlink(_PATH_SYSCMD_LOG);
err1:
	return nBytesSent;
}
int lanSetting(int eid, request * wp, int argc, char **argv)
{
char *parm;

	if (boaArgs(argc, argv, "%s", &parm) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}
 if( parm[0] == '1' ){
 	boaWrite(wp, "menu.addItem(\"Lan Setting\");"
			"lansetting = new MTMenu();"
			"lansetting.addItem(\"LAN Interface\", \"tcpiplan.asp\", \"\", \"LAN Interface Configuration\");"
			"lansetting.addItem(\"DHCP Mode\", \"dhcpmode.asp\", \"\", \"DHCP Mode Configuration\");"
			"lansetting.addItem(\"DHCP Server\", \"dhcpd.asp\", \"\", \"DHCP Server Configuration\");");
	boaWrite(wp, "menu.makeLastSubmenu(lansetting);");
 	}
 else
  {
  boaWrite(wp, "<tr><td><b>Lan Setting</b></td></tr>\n"
                           "<tr><td>&nbsp;&nbsp;<a href=\"tcpiplan.asp\" target=\"view\">LAN Interface</a></td></tr>\n"
			"<tr><td>&nbsp;&nbsp;<a href=\"dhcpmode.asp\" target=\"view\">DHCP Mode</a></td></tr>\n"
			"<tr><td>&nbsp;&nbsp;<a href=\"dhcpd.asp\" target=\"view\">DHCP Server</a></td></tr>\n");
 	}

   return 0;
}
#if 0
static int process_msg(char *msg, int is_wlan_only)
{
	char *p1, *p2;

	p2 = strstr(p1, "wlan");
	if (p2 && p2[5]==':')
		memcpy(p1, p2, strlen(p2)+1);
	else {
		if (is_wlan_only)
			return 0;

		p2 = strstr(p1, "klogd: ");
		if (p2 == NULL)
			return 0;
		memcpy(p1, p2+7, strlen(p2)-7+1);
	}
	return 1;
}
#endif
#ifdef FINISH_MAINTENANCE_SUPPORT
//added by xl_yue for supporting inform ITMS after finishing maintenance
void formFinishMaintenance(request * wp, char *path, char *query)
{
	char	*str, *submitUrl;
	pid_t tr069_pid;

	str = boaGetVar(wp, "finish", "");
	if (str[0]) {
		// signal tr069 to inform ITMS that maintenance is finished
		tr069_pid = read_pid("/var/run/cwmp.pid");
		if ( tr069_pid > 0)
			kill(tr069_pid, SIGUSR1);
		else
			goto setErr_Signal;
	}

	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG1(multilang(LANG_OK_START_TO_INFORM_ITMS_THAT_MAINTENANCE_IS_OVER),submitUrl);
  	return;

setErr_Signal:
	ERR_MSG(multilang(LANG_ERROR_CAN_NOT_FIND_TR069_PCOCESS));

}
#endif

//added by xl_yue
#ifdef USE_LOGINWEB_OF_SERVER
#ifdef USE_BASE64_MD5_PASSWD
void calPasswdMD5(char *pass, char *passMD5);
#endif

static unsigned char sha256_admin[32] = {
	0x8c, 0x69, 0x76, 0xe5, 0xb5, 0x41, 0x04, 0x15,
	0xbd, 0xe9, 0x08, 0xbd, 0x4d, 0xee, 0x15, 0xdf,
	0xb1, 0x67, 0xa9, 0xc8, 0x73, 0xfc, 0x4b, 0xb8,
	0xa8, 0x1f, 0x6f, 0x2a, 0xb4, 0x48, 0xa9, 0x18
};
static unsigned char sha256_root[32] = {
	0x48, 0x13, 0x49, 0x4d, 0x13, 0x7e, 0x16, 0x31,
	0xbb, 0xa3, 0x01, 0xd5, 0xac, 0xab, 0x6e, 0x7b,
	0xb7, 0xaa, 0x74, 0xce, 0x11, 0x85, 0xd4, 0x56,
	0x56, 0x5e, 0xf5, 0x1d, 0x73, 0x76, 0x77, 0xb2
};


int hexStringToBinaryBuf(const char *hexStr, unsigned char *binaryBuf, unsigned int *binaryBufLen)
{
	unsigned int len;
	unsigned int val;
	unsigned int i, j;
	char tmpbuf[3];

	if (hexStr == NULL)
		return -1;

	len = strlen(hexStr);
	if (len % 2 != 0)
		return -1;

	for (i=0, j=0; j < len; i++, j+=2)
	{
		tmpbuf[0] = hexStr[j];
		tmpbuf[1] = hexStr[j+1];
		tmpbuf[2] = 0;

		val = strtoul(tmpbuf, NULL, 16);
		binaryBuf[i] = (unsigned char) val;
	}

	*binaryBufLen = len / 2;

	return 0;
}

static int sha256_encrypt(const char *data, unsigned char *output)
{
	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, (const unsigned char*)data, strlen(data));
	sha256_final(&ctx, output);
	return 0;
}
void userLogin(request * wp, char *path, char *query)
{
	char	*str, *submitUrl, *strPasswd, *strUsername;
#ifdef USE_CAPCTHA_OF_LOGINWEB
	char *captcha;
#endif
	char	suPasswd[MAX_NAME_LEN], usPasswd[MAX_PASSWD_LEN], password[MAX_NAME_LEN];
	char	username[MAX_NAME_LEN];
#ifdef USE_BASE64_MD5_PASSWD
	char md5_pass[100];
#endif
	struct user_info * pUser_info;
#ifdef LOGIN_ERR_TIMES_LIMITED
	struct errlogin_entry * pErrlog_entry = NULL;
#endif
	char usrName[MAX_NAME_LEN];
	char supName[MAX_NAME_LEN];
	//xl_yue:1:bad password;2:invalid user;3:login error for three times,forbidden;4:other has login;
	int denied = 1;
#ifdef ACCOUNT_CONFIG
	MIB_CE_ACCOUNT_CONFIG_T Entry;
	int totalEntry, i;
#endif
	int connectedInterface = 0;
	char remote_ip_addr[INET6_ADDRSTRLEN] = {0};
	unsigned char lan_ip[IP_ADDR_LEN] = {0};
	char lan_ip_str[INET6_ADDRSTRLEN] = {0};
	struct timeval tv;
	char buffer[26];
	char tmppw[MAX_NAME_LEN];
	char timestr[30] = {0};
	struct tm* tm_info;
	int millisec;
	int len = 0;
	int mode = 0;
	int change_pw = 0;
	char *change = NULL;
	int num;
#ifdef CONFIG_DEONET_SESSION
	char *p_session = NULL;
#endif
	unsigned int	nameLen;

	gettimeofday(&tv, NULL);
	millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
	if (millisec>=1000) { // Allow for rounding up to nearest second
		millisec -=1000;
		tv.tv_sec++;
	}

	tm_info = localtime(&tv.tv_sec);
#ifdef CONFIG_00R0
	strftime(buffer, sizeof(buffer), "%d/%m %H:%M:%S", tm_info);
#else
	strftime(buffer, sizeof(buffer), "%m/%d %H:%M:%S", tm_info);
#endif
	snprintf(timestr, sizeof(timestr), "%s.%d", buffer, millisec);

	mib_get_s(MIB_ADSL_LAN_IP, lan_ip, sizeof(lan_ip));
	inet_ntop(AF_INET, lan_ip, lan_ip_str, INET_ADDRSTRLEN);

	strcpy(remote_ip_addr, wp->remote_ip_addr);
	rtk_util_get_ipv4addr_from_ipv6addr(remote_ip_addr);
	connectedInterface = rtk_intf_get_intf_by_ip(remote_ip_addr);

	struct sockaddr_in localAddress;
	socklen_t addressLength = sizeof(localAddress);
	getsockname(wp->fd, (struct sockaddr*)&localAddress, &addressLength);
	//printf("local address: %s\n", inet_ntoa( localAddress.sin_addr));
	//printf("local port: %d\n", (int) ntohs(localAddress.sin_port));
	rtk_util_get_wanaddr_from_nf_conntrack(remote_ip_addr, (int) ntohs(localAddress.sin_port), lan_ip_str);
	//printf("lan_ip_str: %s\n", lan_ip_str);

	mode = deo_wan_nat_mode_get();

	str = boaGetVar(wp, "save", "");
	if (str[0]) {
#ifndef CONFIG_DEONET_SESSION
		pUser_info = search_login_list(wp);
#if !(defined(CONFIG_00R0))
		if(pUser_info){
			denied = 5;
			goto setErr_Signal;
		}
#endif
#endif
		strUsername = boaGetVar(wp, "encodeUsername", "");
		if (!strUsername[0] ) {
			denied = 2;
			goto setErr_Signal;
		}

		// save user name for web control
		memset(username, 0, sizeof(username));
		memset(global_access_name, 0, sizeof(global_access_name));
#ifndef USE_DEONET /* login : sha256 */
		rtk_util_data_base64decode(strUsername, username, sizeof(username));
		username[sizeof(username) - 1] = '\0';
		username[MAX_NAME_LEN - 1] = '\0';
		sprintf(global_access_name, "%s", username);
#else
		unsigned char   sha256Hex_UserName[64];

		memset(sha256Hex_UserName, 0, 64);
		hexStringToBinaryBuf(strUsername, sha256Hex_UserName, &nameLen);

		if(!memcmp(sha256_root, sha256Hex_UserName, SHA256_BLOCK_SIZE))
			sprintf(username, "%s", "root");
		else if(!memcmp(sha256_admin, sha256Hex_UserName, SHA256_BLOCK_SIZE))
			sprintf(username, "%s", "admin");
		sprintf(global_access_name, "%s", username);
#endif

		// for system acount, lan access ipv4
		len = strlen(remote_ip_addr);

		if (!strcmp(username, "root") && connectedInterface == 0) {
			if (!strstr(remote_ip_addr + (len - 3), "254")) {
				denied = 8;
				goto setErr_Signal;
			}
		}

#if 0 // when modify root & admin account, open code
		else if (!strcmp(username, "admin") && connectedInterface == 0) {
				denied = 9;
				goto setErr_Signal;
		}
#endif

		// Mason Yu on True
		strncpy(g_login_username, username, sizeof(g_login_username));
		g_login_username[sizeof(g_login_username)-1] = '\0';
		if (!username[0] ) {
			denied = 2;
			goto setErr_Signal;
		}

		//strPasswd = boaGetVar(wp, "password", "");
		strPasswd = boaGetVar(wp, "encodePassword", "");
		if (!strPasswd[0] ) {
			denied = 1;
			goto setErr_Signal;
		}
		memset(password, 0, sizeof(password));
#ifndef USE_DEONET /* login : sha256 */
		rtk_util_data_base64decode(strPasswd, password, sizeof(password));
		password[sizeof(password)-1] = '\0';
		password[MAX_NAME_LEN-1]='\0';
#else
		int				id = -1;
		char			tmpPasswd[MAX_NAME_LEN];
		unsigned char	sha256Hex_temp[32];
		unsigned char   sha256Hex_UserPassWD[64];

		memset(sha256Hex_UserPassWD, 0, 64);
		memset(sha256Hex_temp, 0, 32);
		memset(suPasswd, 0, MAX_NAME_LEN);
		hexStringToBinaryBuf(strPasswd, sha256Hex_UserPassWD, &nameLen);

		if(!strcmp(username, "admin"))
			id = MIB_USER_PASSWORD;
		else if(!strcmp(username, "root"))
			id = MIB_SUSER_PASSWORD;

		if ( !mib_get_s(id, (void *)suPasswd, sizeof(suPasswd)) ){
			denied = 10;
			goto setErr_Signal;
		}

		sha256_encrypt(suPasswd, sha256Hex_temp);

		if(!memcmp(sha256Hex_temp, sha256Hex_UserPassWD, SHA256_BLOCK_SIZE))
			sprintf(password, "%s", suPasswd);

#endif

		/* compare default pow(ex:333334_admin), except mac OUI address */
		if (!strcmp(username, "admin") && strstr(password, "_admin")) {
			memset(tmppw, 0, sizeof(tmppw));

			// get current admin password
			mib_get_s(MIB_USER_PASSWORD, (void *)tmppw, sizeof(tmppw));
			if (!strcmp(password, tmppw))
				change_pw = 1;
		}

#ifdef USE_CAPCTHA_OF_LOGINWEB
		captcha = boaGetVar(wp, "captchaTextBox", "");
		if(strcmp(g_captcha, captcha)) {
			denied = 6;
			goto setErr_Signal;
		}
#endif
	}else{
		denied = 10;
		goto setErr_Signal;
	}

#ifdef ACCOUNT_CONFIG
	totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL);
	for (i=0; i<totalEntry; i++) {
		if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&Entry)) {
			denied = 10;
			goto setErr_Signal;
		}
		if (Entry.privilege == (unsigned char)PRIV_ROOT)
		{
			supName[sizeof(supName)-1]='\0';
			strncpy(supName, Entry.userName,sizeof(supName)-1);
		}
		else
		{
			usrName[sizeof(usrName)-1]='\0';
			strncpy(usrName, Entry.userName,sizeof(usrName)-1);
		}
		if (strcmp(username, Entry.userName) == 0) {
#ifdef USE_BASE64_MD5_PASSWD
			calPasswdMD5(Entry.userPassword, md5_pass);
			if (strcmp(password, md5_pass))
#else
			if (strcmp(password,Entry.userPassword))
#endif
			{
				denied = 1;
				goto setErr_Signal;
			}
			denied = 0;
			goto pass_check;
		}
	}
#endif
	if ( !mib_get_s(MIB_USER_NAME, (void *)usrName, sizeof(usrName)) ) {
		denied = 10;
		goto setErr_Signal;
	}

	if(strcmp(usrName, username)==0){
		if ( !mib_get_s(MIB_USER_PASSWORD, (void *)usPasswd, sizeof(usPasswd)) ) {
			denied = 10;
			goto setErr_Signal;
		}
#ifdef USE_BASE64_MD5_PASSWD
		calPasswdMD5(usPasswd, md5_pass);
		if(strcmp(password,md5_pass))
#else
		if(strcmp(password,usPasswd))
#endif
		{
			denied = 1;
			goto setErr_Signal;
		}
		denied = 0;
		goto pass_check;
	}

	if ( !mib_get_s(MIB_SUSER_NAME, (void *)supName, sizeof(supName)) ) {
		denied = 10;
		goto setErr_Signal;
	}

	if(strcmp(supName, username)==0){
		if ( !mib_get_s(MIB_SUSER_PASSWORD, (void *)suPasswd, sizeof(suPasswd)) ){
			denied = 10;
			goto setErr_Signal;
		}
#ifdef USE_BASE64_MD5_PASSWD
		calPasswdMD5(suPasswd, md5_pass);
		if(strcmp(password,md5_pass))
#else
		if(strcmp(password,suPasswd))
#endif
		{
			denied = 1;
			goto setErr_Signal;
		}
		denied = 0;
		goto pass_check;
	}

	if(denied){
		denied = 2;
		goto setErr_Signal;
	}

pass_check:
#ifdef ONE_USER_LIMITED
	if(!strcmp(usrName, username) && usStatus.busy){
		if(strcmp(usStatus.remote_ip_addr, wp->remote_ip_addr)){
			denied = 4;
			goto setErr_Signal;
		}
	}else if(!strcmp(supName, username) && suStatus.busy){
		if(strcmp(suStatus.remote_ip_addr, wp->remote_ip_addr)){
			denied = 4;
			goto setErr_Signal;
		}
	}
#endif

	p_session = boaGetVar(wp, "web_cookie", "");
	wp->cookie = p_session;
	pUser_info = search_login_list(wp);
#ifdef CONFIG_DEONET_SESSION
	if (getNumLoginUserInfo() >= 10) {
			denied = 7;
			goto setErr_Signal;
	}
#endif
	if(!pUser_info){
		pUser_info = malloc(sizeof(struct user_info));
		pUser_info->last_time = time_counter;
//		pUser_info->login_status = STATUS_LOGIN;
		strncpy(pUser_info->remote_ip_addr, wp->remote_ip_addr, sizeof(pUser_info->remote_ip_addr));
		memset(pUser_info->session_id, 0, sizeof(pUser_info->session_id));
		if(strcmp(usrName, username)==0){
			pUser_info->directory = strdup("/index_user.html");
			pUser_info->priv = 0;
#ifdef ONE_USER_LIMITED
			pUser_info->paccount = &usStatus;
			pUser_info->paccount->busy = 1;
			strncpy(pUser_info->paccount->remote_ip_addr, wp->remote_ip_addr, sizeof(pUser_info->paccount->remote_ip_addr));
#endif
		}
		else{
			pUser_info->priv = 1;
#ifdef CONFIG_VDSL
#ifdef CONFIG_GENERAL_WEB
			pUser_info->directory = strdup("/index.html");
#else
			pUser_info->directory = strdup("/index_vdsl.html");
#endif
#else
			pUser_info->directory = strdup("/index.html");
#endif
#ifdef ONE_USER_LIMITED
			pUser_info->paccount = &suStatus;
			pUser_info->paccount->busy = 1;
			strncpy(pUser_info->paccount->remote_ip_addr, wp->remote_ip_addr, sizeof(pUser_info->paccount->remote_ip_addr));
#endif
		}
		//list it to user_login_list
		pUser_info->next = user_login_list;
		user_login_list = pUser_info;

		if (change_pw)
			replace_pw_page = 1;

		syslog(LOG_INFO, "login successful for %s from %s(%s) via web %s %s\n",
			username, remote_ip_addr, CONNECION_INTERFACE[connectedInterface], lan_ip_str, timestr);
	}else{
//		if(pUser_info->login_status != STATUS_FORBIDDEN){
			pUser_info->last_time = time_counter;
//			pUser_info->login_status = STATUS_LOGIN;
//		}
	}

#ifdef LOGIN_ERR_TIMES_LIMITED
	free_from_errlog_list(wp);
#endif
#ifdef CONFIG_00R0
        unsigned char redirectTroubleWizard=0;
        mib_get_s(RS_WEB_WIZARD_REDIRECTTROUBLEWIZARD, &redirectTroubleWizard, sizeof(redirectTroubleWizard));
        if(redirectTroubleWizard == 2)
                boaRedirect(wp, "/index.asp");
        else
#endif
		submitUrl = boaGetVar(wp, "submit-url", "");
		boaRedirect(wp, submitUrl);

		//submitUrl = boaGetVar(wp, "submit-url", "");
		//OK_MSG1(multilang(LANG_OK_LOGIN_SUCCESSFULLY),submitUrl);
		if (connectedInterface != 0) {
		syslog2(LOG_DM_HTTP, LOG_DM_HTTP_LOGIN,
				"HTTP login successfully from Remote");
		} else {
			syslog2(LOG_DM_HTTP, LOG_DM_HTTP_LOGIN,
				"HTTP login successfully from Local %s",
				strip_ffff_in_ip(remote_ip_addr));
		}

  	return;

setErr_Signal:

#ifdef LOGIN_ERR_TIMES_LIMITED
	if(denied == 1 || denied == 2){
		pErrlog_entry = (struct errlogin_entry*)search_errlog_list(wp);
		if(pErrlog_entry){
			pErrlog_entry->last_time = time_counter;
			pErrlog_entry->login_count++;
			if(pErrlog_entry->login_count >= MAX_LOGIN_NUM)
				denied = 3;
		}else{
			pErrlog_entry = malloc(sizeof(struct errlogin_entry));
			pErrlog_entry->last_time = time_counter;
			pErrlog_entry->login_count = 1;
			strncpy(pErrlog_entry->remote_ip_addr, wp->remote_ip_addr, sizeof(pErrlog_entry->remote_ip_addr));
			pErrlog_entry->next = errlogin_list;
			errlogin_list = pErrlog_entry;
		}
	}
#endif

	switch(denied){
		case 1:
			// ERR_MSG2(multilang(LANG_ERROR_BAD_PASSWORD));
			ERR_MSG2("Connection failed.");
			syslog(LOG_ERR, "login error(bad password) for %s from %s(%s) via web %s %s\n",
				username?username:"", remote_ip_addr, CONNECION_INTERFACE[connectedInterface], lan_ip_str, timestr);
			break;
		case 2:
			// ERR_MSG2(multilang(LANG_ERROR_INVALID_USERNAME));
			ERR_MSG2("Connection failed.");
			syslog(LOG_ERR, "login error(invalid username) for %s from %s(%s) via web %s %s\n",
				username?username:"", remote_ip_addr, CONNECION_INTERFACE[connectedInterface], lan_ip_str, timestr);
			break;
#ifdef LOGIN_ERR_TIMES_LIMITED
		case 3:
			ERR_MSG2(multilang(LANG_ERROR_YOU_HAVE_LOGINED_ERROR_THREE_TIMES_PLEASE_RELOGIN_1_MINUTE_LATER));
			syslog(LOG_ERR, "login error(having logined error three times) for %s from %s(%s) via web %s %s\n",
				username?username:"", remote_ip_addr, CONNECION_INTERFACE[connectedInterface], lan_ip_str, timestr);
			break;
#endif
#ifdef ONE_USER_LIMITED
		case 4:
			ERR_MSG2(multilang(LANG_ERROR_ANOTHER_USER_HAVE_LOGINED_IN_USING_THIS_ACCOUNTONLY_ONE_USER_CAN_LOGIN_USING_THIS_ACCOUNT_AT_THE_SAME_TIME));
			syslog(LOG_ERR, "login error(using the same account with another user at the same time) for %s from %s(%s) via web %s %s\n",
				username?username:"", remote_ip_addr, CONNECION_INTERFACE[connectedInterface], lan_ip_str, timestr);
			break;
#endif
		case 5:
			ERR_MSG2(multilang(LANG_ERROR_YOU_HAVE_LOGINED_PLEASE_LOGOUT_AT_FIRST_AND_THEN_LOGIN));
			syslog(LOG_ERR, "login error(having logined) for %s from %s(%s) via web %s %s\n",
				username?username:"", remote_ip_addr, CONNECION_INTERFACE[connectedInterface], lan_ip_str, timestr);
			break;
#ifdef USE_CAPCTHA_OF_LOGINWEB
		case 6:
			ERR_MSG2(multilang(LANG_ERROR_BAD_VERIFICTION_CODE));
			syslog(LOG_ERR, "login error(bad verification code) for %s from %s(%s) via web %s %s\n",
				username?username:"", remote_ip_addr, CONNECION_INTERFACE[connectedInterface], lan_ip_str, timestr);
			break;
#endif
#ifdef CONFIG_DEONET_SESSION
		case 7:
			// ERR_MSG2("The maximum number of allowed sessions has been exceeded.");
			ERR_MSG2("Connection failed.");
			syslog(LOG_ERR, "login error(full sessions) for %s from %s(%s) via web %s %s\n",
				username?username:"", remote_ip_addr, CONNECION_INTERFACE[connectedInterface], lan_ip_str, timestr);
			break;
#endif
		case 8:
			// ERR_MSG2(multilang(LANG_ERROR_AUTHENTICATION_IP));
			ERR_MSG2("Connection failed.");
			syslog(LOG_ERR, "login error(bad access ip for system account) for %s from %s(%s) via web %s %s\n",
				username?username:"", remote_ip_addr, CONNECION_INTERFACE[connectedInterface], lan_ip_str, timestr);
			break;

		case 9:
			ERR_MSG2(multilang(LANG_ERROR_WEB_AUTHENTICATION_ERRORPLEASE_CLOSE_THIS_WINDOW_AND_REOPEN_YOUR_WEB_BROWSER_TO_LOGIN));
			syslog(LOG_ERR, "login error(web authentication error) for %s from %s(%s) via web %s %s\n",
				username?username:"", remote_ip_addr, CONNECION_INTERFACE[connectedInterface], lan_ip_str, timestr);
			break;

		default:
			ERR_MSG2(multilang(LANG_ERROR_WEB_AUTHENTICATION_ERRORPLEASE_CLOSE_THIS_WINDOW_AND_REOPEN_YOUR_WEB_BROWSER_TO_LOGIN));
			syslog(LOG_ERR, "login error(web authentication error) for %s from %s(%s) via web %s %s\n",
				username?username:"", remote_ip_addr, CONNECION_INTERFACE[connectedInterface], lan_ip_str, timestr);
			break;
		}

	if(denied) {
		if (connectedInterface != 0) {
		syslog2(LOG_DM_HTTP, LOG_DM_HTTP_LOGIN,
				"HTTP login failed from Remote");
		} else {
			syslog2(LOG_DM_HTTP, LOG_DM_HTTP_LOGIN,
				"HTTP login failed from Local %s",
				strip_ffff_in_ip(remote_ip_addr));
		}
	}
}

void formLogout(request * wp, char *path, char *query)
{
	char	*str, *submitUrl, *p_session = NULL;
	struct user_info * pUser_info;
	char remote_ip_addr[INET6_ADDRSTRLEN];
	int connectedInterface = 0;

	remote_ip_addr[sizeof(remote_ip_addr)-1]='\0';
	strncpy(remote_ip_addr, wp->remote_ip_addr,sizeof(remote_ip_addr)-1);
	rtk_util_get_ipv4addr_from_ipv6addr(remote_ip_addr);
	connectedInterface = rtk_intf_get_intf_by_ip(remote_ip_addr);

	str = boaGetVar(wp, "save", "");
	p_session = boaGetVar(wp, "web_cookie", "");
	if (str[0]) {
		wp->cookie = p_session;
		if(!free_from_login_list(wp)){
			// syslog(LOG_ERR, "logout error from %s\n",remote_ip_addr);
			boaRedirect(wp, "/admin/login.asp");
		}
#ifdef USE_LOGINWEB_OF_SERVER
		syslog(LOG_INFO, "logout successful for %s from %s(%s) on web\n", g_login_username, remote_ip_addr, CONNECION_INTERFACE[connectedInterface]);
#else
		syslog(LOG_INFO, "logout successful from %s(%s) on web\n", remote_ip_addr, CONNECION_INTERFACE[connectedInterface]);
#endif
	}

#ifdef CONFIG_00R0
	boaWrite(wp, "<html><head>\n"
		 "</style>\n"
		 "<script language=javascript>\n"
		 "window.open(\"/admin/login.asp\",target=\"_top\"); \n"
		 "</script></head>\n"
		 "<body>");
		req_write(wp, "</BODY></HTML>\n");
		req_flush(wp);
		return;
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG1(multilang(LANG_OK_LOGOUT_SUCCESSFULLY),submitUrl);
  	return;
//setErr_Signal:
//	ERR_MSG(multilang(LANG_ERROR_LOGOUT_FAILEDPERHAPS_YOU_HAVE_LOGOUTED));
}

int passwd2xmit(int eid, request * wp, int argc, char **argv)
{
#ifdef USE_BASE64_MD5_PASSWD
	boaWrite(wp, "document.cmlogin.password.value = CryptoJS.PHP_CRYPT_MD5(document.cmlogin.password.value, \"$1$\");");
#endif
}
#ifdef USE_CAPCTHA_OF_LOGINWEB
int getCaptchastring(int eid, request * wp, int argc, char **argv)
{
	char charsArray[256] = VERIFICATION_CODE_BASE;
	int lengthOtp = CAPTCHA_LENGTH, index = 0, i = 0;
	char cmd[64] ={0,};
	memset(g_captcha,0,MAX_NAME_LEN);
	srand(time(NULL));
	for(i = 0; i < lengthOtp; i++)
	{
		index = rand()%strlen(charsArray);
		if(charsArray[index] == 'M' && i == (lengthOtp -1))
			g_captcha[i] = 'm';
		else
			g_captcha[i] = charsArray[index];
	}
	boaWrite(wp, "\"%s\"", "OK");

	sprintf(cmd, "/bin/c_captcha %s", g_captcha);
	system(cmd);
}
#endif

// Multi-lingual for login, added by davian_kuo.
void userLoginMultilang(request *wp, char *path, char *query) {
	char *strVal;
	char *submitUrl;

	strVal = (char *)boaGetVar (wp, "loginSelinit", "");
	if (strVal) {
		g_language_state = atoi (strVal);
		if (g_language_state<LANG_MAX && g_language_state != g_language_state_prev) {
			//printf ("Switch to lang_%d\n", g_language_state);
			g_language_state_prev = g_language_state;

#if MULTI_LANG_DL == 1
			if (dl_handle != NULL) dlclose (dl_handle);

			char *lib_name = (char *) malloc (sizeof(char) * 25);
			if (lib_name == NULL) {
				fprintf (stderr, "lib_name malloc failed!\n"); return;
			}
			char *strtbl_name = (char *) malloc (sizeof(char) * 15);
			if (strtbl_name == NULL) {
				fprintf (stderr, "strtbl_name malloc failed!\n"); return;
			}
			snprintf (lib_name, 25, "libmultilang_%s.so", lang_set[g_language_state].langType);
			snprintf (strtbl_name, 15, "strtbl_%s", lang_set[g_language_state].langType);

			dl_handle = dlopen (lib_name, RTLD_LAZY);
			strtbl = (const char **) dlsym (dl_handle, strtbl_name);

			free (lib_name);
			free (strtbl_name);
#else
			strtbl = strtbl_name[g_language_state];
#endif
			// Save mib to xmlconfig.
			if (!mib_set (MIB_MULTI_LINGUAL, (void *)lang_set[g_language_state].langType)) {
				ERR_MSG (strSetMultiLangError);
				return;
			}
			#ifdef COMMIT_IMMEDIATELY
			Commit ();
			#endif
		}
	}
	boaRedirect(wp, "/");
}

#endif


#ifdef ACCOUNT_LOGIN_CONTROL
//added by xl_yue
/*
extern struct account_info su_account;
extern char suName[MAX_NAME_LEN];
*/
void formAdminLogout(request * wp, char *path, char *query)
{
	char	*str, *submitUrl;

	str = boaGetVar(wp, "adminlogout", "");
	if (str[0]) {
		su_account.account_busy = 0;
		su_account.account_timeout = 1;
		syslog(LOG_INFO, "Account: %s logout from %s\n", suName, su_account.remote_ip_addr);
	}
	else
		goto err_logout;

	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG1(multilang(LANG_LOGOUT_SUCCESSFULLY),submitUrl);
  	return;

err_logout:
	ERR_MSG(multilang(LANG_LOGOUT_ERROR));
}

//added by xl_yue
/*
extern struct account_info us_account;
extern char usName[MAX_NAME_LEN];
*/
void formUserLogout(request * wp, char *path, char *query)
{
	char	*str, *submitUrl;

	str = boaGetVar(wp, "userlogout", "");
	if (str[0]) {
		us_account.account_busy = 0;
		us_account.account_timeout = 1;
		syslog(LOG_INFO, "Account: %s logout from %s\n", usName, us_account.remote_ip_addr);
	}
	else
		goto err_logout;

	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG1(multilang(LANG_LOGOUT_SUCCESSFULLY),submitUrl);
  	return;

err_logout:
	ERR_MSG(multilang(LANG_LOGOUT_ERROR));
}

#endif


#ifdef CONFIG_USER_RTK_SYSLOG
int sysLogList(int eid, request * wp, int argc, char **argv)
{
#ifndef USE_DEONET
	FILE *fp;
	char buff[256], *facstr, *pristr, *timestr, *tmp, *msgstr;
	int nBytesSent=0;
	int enabled;
	unsigned char vChar;
	int pri;
	char tmpbuf_target[1024]={0};

	if (first_time==1) {
		mib_get_s(MIB_ADSL_DEBUG, (void *)&vChar, sizeof(vChar));
		if(vChar==1)
			dbg_enable=1;
		first_time=0;
	}

	if (dbg_enable==0) {
		nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
			"<td align=center bgcolor=\"#808080\"><font size=\"2\"><b>%s/%s</b></font></td>\n"
			"<td align=center bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
			"<td align=center bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
			"<td align=center bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td></tr>\n",
#else
			"<th align=center>%s/%s</th>\n"
			"<th align=center>%s</th>\n"
			"<th align=center>%s</th>\n"
			"<th align=center>%s</th></tr>\n",
#endif
		multilang(LANG_DATE), multilang(LANG_TIME), multilang(LANG_FACILITY),
		multilang(LANG_LEVEL), multilang(LANG_MESSAGE));

#ifndef BB_FEATURE_SAVE_LOG
		fp = fopen("/var/log/messages.old", "r");
		if (fp) {
			fgets(buff, sizeof(buff), fp);
			while (fgets(buff, sizeof(buff), fp) != NULL) {
				tmp = strtok(buff, "|");
				timestr = strtok(NULL, "|");
				facstr = strtok(NULL, ".");
				pristr = strtok(NULL, "|");
				msgstr = strtok(NULL, "\n");
				mib_get_s(MIB_SYSLOG_DISPLAY_LEVEL, (void *)&vChar, sizeof(vChar));
				pri = atoi(tmp);
				if((pri & 0x07) <= vChar)
					nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
						"<td bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
						"<td bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
						"<td bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
						"<td bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td></tr>\n", timestr, facstr, pristr, escape_msg(msgstr));
#else
						"<td>%s</td>\n"
						"<td>%s</td>\n"
						"<td>%s</td>\n"
						"<td>%s</td></tr>\n", timestr, facstr, pristr, escape_msg(msgstr));
#endif
			}
			fclose(fp);
		}
#endif

#ifdef BB_FEATURE_SAVE_LOG
		if (!(fp = fopen("/var/run/syslogd.txt", "r")))
#else
		if (!(fp = fopen("/var/log/messages", "r")))
#endif
		{

			//printf("Error: cannot open /var/log/messages - continuing...\n");
			goto err1;
		}
#ifndef BB_FEATURE_SAVE_LOG
		fgets(buff, sizeof(buff), fp);
#endif
		while (fgets(buff, sizeof(buff), fp) != NULL) {
			memset(tmpbuf_target, 0, sizeof(tmpbuf_target));
			escape(tmpbuf_target,buff);
			tmp = strtok(tmpbuf_target, "|");
			timestr = strtok(NULL, "|");
			facstr = strtok(NULL, ".");
			pristr = strtok(NULL, "|");
			msgstr = strtok(NULL, "\n");
			mib_get_s(MIB_SYSLOG_DISPLAY_LEVEL, (void *)&vChar, sizeof(vChar));
			pri = atoi(tmp);
			if((pri & 0x07) <= vChar){
				if(timestr == NULL){
					continue;
				}
				else{
					nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
						"<td bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
						"<td bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
						"<td bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
						"<td bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td></tr>\n", timestr, facstr, pristr, msgstr);
#else
						"<td>%s</td>\n"
						"<td>%s</td>\n"
						"<td>%s</td>\n"
						"<td>%s</td></tr>\n", timestr, facstr, pristr, escape_msg(msgstr));
#endif
				}
			}
		}
		fclose(fp);
	}
err1:
	return nBytesSent;
#else
	return 0;
#endif
}

static void saveLogFile(request * wp, FILE *fp)
{
	//unsigned char *ptr;
	unsigned int fileSize,filelen;
	unsigned int fileSector;
	unsigned int maxFileSector;

	//decide the file size
	fseek(fp, 0, SEEK_END);
	filelen = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fileSize=filelen;

	while (fileSize>0) {
		char buf[0x100];
		maxFileSector = 0x50;
		int nRead;

		fileSector = (fileSize > maxFileSector) ? maxFileSector : fileSize;
		nRead = fread((void *)buf, 1, fileSector, fp);

		boaWriteDataNonBlock(wp, buf, nRead);

		fileSize -= fileSector;
		//ptr += fileSector;
	}
}

void formUpload1(request * wp, char *path, char *query)
{
	return;
}
void formUpload2(request * wp, char *path, char *query)
{
	return;
}
void formUpload3(request * wp, char *path, char *query){
	gIsCommit = 1;
	return;
}
void formUpload4(request * wp, char *path, char *query){
	gIsCommit = 0;
	return;
}
void formUpload5(request * wp, char *path, char *query)
{
	gIsReboot = 1;
	return;
}
void formUpload6(request * wp, char *path, char *query){
	gIsReboot = 0;
	return;
}
void formBankStatus(request * wp, char *path, char *query)
{
	char *strVal, *submitUrl;
    int isCommit, isReboot, newCommit, isUpgrade;
    int isCommit_tmp = 0, isReboot_tmp = 0, isUpgrade_tmp = 0;

	strVal = boaGetVar(wp, "commit_cbx", "");
	if(strVal[0])
		isCommit = 1;
	else
		isCommit = 0;

	strVal = boaGetVar(wp, "reboot_cbx", "");
	if(strVal[0])
		isReboot = 1;
	else
		isReboot = 0;

	if(isCommit)
	{
		newCommit = get_commit_info() == 0 ? 1 : 0;
		set_commit_info(newCommit);
	}

	if(isReboot)
	{
		boaHeader(wp);
		MSG_COUNTDOWN(multilang(LANG_REBOOT_WORD0), REBOOT_TIME, NULL);
		boaWrite(wp, "<body><blockquote>\n");
		boaWrite(wp, "%s<br><br>\n", multilang(LANG_REBOOT_WORD1));
		boaWrite(wp, "%s\n", multilang(LANG_REBOOT_WORD2));
		boaWrite(wp, "</blockquote></body>");
		boaFooter(wp);
		boaDone(wp, 200);
#ifdef EMBED
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
#endif
		syslog2(LOG_DM_HTTP, LOG_DM_HTTP_RESET, "Reboot by Web");
		cmd_reboot();
		return;
	}

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	OK_MSG(submitUrl);
	return;
}

void formLdap(request * wp, char *path, char *query)
{
    char tmpBuf[100];
	char *strVal, *submitUrl;
    char *ldapUrl = NULL, *ldapFile = NULL, *ldapPath;
	unsigned char ldapcap = -1;

	strVal = boaGetVar(wp, "apply", "");
	if (strVal[0]) {
		strVal = boaGetVar(wp, "ldapcap", "");
		if (strVal[0]) {
			if (strVal[0] == '0')
				ldapcap = 0;
			else
				ldapcap = 1;
		}

		ldapUrl = boaGetVar(wp, "ldap_url", "");
		ldapFile = boaGetVar(wp, "ldap_file", "");
		ldapPath = boaGetVar(wp, "ldap_relativepath", "");

        if(ldap_mib_set(ldapcap, ldapUrl, ldapFile, ldapPath) < 0)
		{
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto formHolepunching_err;

		}
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif

	}

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	OK_MSG(submitUrl);
	return;
formHolepunching_err:
	ERR_MSG(tmpBuf);
	return;

}

void formHolepunching(request * wp, char *path, char *query)
{
	char *strVal, *submitUrl, *holeServer = NULL;
	char tmpBuf[100];
	unsigned char holecap = -1;
	unsigned short holePort = -1, holeInterval = -1;

	strVal = boaGetVar(wp, "apply", "");
	if (strVal[0]) {
		strVal = boaGetVar(wp, "holecap", "");
		if (strVal[0]) {
			if (strVal[0] == '0')
				holecap = 0;
			else
				holecap = 1;
		}

		holeServer = boaGetVar(wp, "hole_server", "");

		strVal = boaGetVar(wp, "hole_server_port", "");
		if (strVal && strVal[0] )
			holePort = atoi(strVal);

		strVal = boaGetVar(wp, "hole_interval", "");
		if (strVal && strVal[0])
			holeInterval  = atoi(strVal);

		if(holepunching_mib_set(holecap, holeServer,
					holePort, holeInterval) < 0)
		{
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto formHolepunching_err;

		}
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
#if 0 /* start/stop는 holeponching monitor에서 관리 */
		stopHolepunching();
		if(holecap == 1)
#endif
			startHolepunching();

	}

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	OK_MSG(submitUrl);
	return;
formHolepunching_err:
	ERR_MSG(tmpBuf);
	return;
}

void formWanApConnection(request * wp, char *path, char *query)
{
	char  *submitUrl, *ap_ip, *str;
	unsigned int port;

	str = boaGetVar(wp, "ip_address", "");
	if (str[0]) {
		ap_ip = boaGetVar(wp, "ap_ip_address", "");
		if (ap_ip[0]) {

			port = add_ap_connection(str);
			if(port)
				boaWrite(wp,"<meta charset='UTF-8'><script>var strUrl='http://%s:%d';var win=window.open(strUrl,'_blank'); if (win) {win.focus();} else {alert('팝업이 차단되었습니다. 팝업 차단을 해제해주세요.');}</script>", ap_ip, port);
			else
				boaWrite(wp, "<script>alert('%s');</script>", multilang(LANG_AP_CONNECTION_MAX_CONNECTION));

		}
	}

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page

	if (submitUrl[0])
	{
		boaWrite(wp, "<script>window.location.href = '%s';</script>", submitUrl);
	}
	return;
}

void formSysLog(request * wp, char *path, char *query)
{
	char  *submitUrl,*strVal,*str;
	unsigned char logcap;
	char tmpBuf[100];
	struct in_addr inIp;

#ifndef USE_DEONET
	//max msg. length
	str = boaGetVar(wp, "maxloglen", "");
	if (str[0]) {
		unsigned int len;
		len = (unsigned int) strtol(str, (char**)NULL, 10);
		if ( mib_set(MIB_MAXLOGLEN, (void *)&len) == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto formSysLog_err;
		}
	}
#else
	str = boaGetVar(wp, "logline", "");
	if (str[0]) {
		unsigned int len;
		len = (unsigned int) strtol(str, (char**)NULL, 10);
		if ( mib_set(MIB_SYSLOG_LINE, (void *)&len) == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto formSysLog_err;
		}
		syslog2_conf(SLOG_CONF_ROTATE_FILELINE, (char*)&len);
	}
#endif

	str = boaGetVar(wp, "refresh", "");
	if (str[0]) {

		submitUrl = boaGetVar(wp, "submit-url", "");
		if (submitUrl[0])
			boaRedirect(wp, submitUrl);
		return;
	}
	// Set ACL Capability
	strVal = boaGetVar(wp, "apply", "");
	if (strVal[0]) {
		struct in_addr inIpAddr;
		strVal = boaGetVar(wp, "logcap", "");
		if (strVal[0]) {
			if (strVal[0] == '0')
				logcap = 0;
			else
				logcap = 1;
			if ( !mib_set(MIB_SYSLOG, (void *)&logcap)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
#ifdef USE_DEONET
			int tmp = logcap;
			syslog2_conf(SLOG_CONF_ENABLE, (char*)&tmp);
#endif
		}

#ifdef WEB_ENABLE_PPP_DEBUG
		strVal = boaGetVar(wp, "pppcap", "");
		if (strVal[0]) {
			unsigned char pppcap;
			struct data_to_pass_st msg;
			pppcap = strVal[0] - '0';
			snprintf(msg.data, BUF_SIZE, "spppctl syslog %d", pppcap);
			write_to_pppd(&msg);
		}
#endif

		strVal = boaGetVar(wp, "levelLog", "");
		if (strVal[0]) {
			logcap = strVal[0] - '0' ;
			if (mib_set(MIB_SYSLOG_LOG_LEVEL, (void *)&logcap) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
#ifdef USE_DEONET
			int tmp = logcap;
			syslog2_conf(SLOG_CONF_LOCAL_LEVEL, (char*)&tmp);
#endif
		}
#ifndef USE_DEONET
		strVal = boaGetVar(wp, "levelDisplay", "");
		if (strVal[0]) {
			logcap = strVal[0] - '0' ;
			if (mib_set(MIB_SYSLOG_DISPLAY_LEVEL, (void *)&logcap) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
		}
#else
		strVal = boaGetVar(wp, "levelRemote", "");
		if (strVal[0]) {
			int tmp;
			char log[32] = {0,};
			char strLevel[8][6] = {"Emerg", "Alert", "Crit", "Err", "Warn", "Notify", "Info", "Debug" };
			logcap = strVal[0] - '0' ;
			tmp = logcap;
			if (mib_set(MIB_SYSLOG_REMOTE_LEVEL, (void *)&logcap) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			syslog2_conf(SLOG_CONF_REMOTE_LEVEL, (char*)&tmp);

			if(tmp >= 0 && tmp <= 7)
			{
				sprintf(log, "Log Level=%s", strLevel[tmp]);
				syslog2(LOG_DM_HTTP, LOG_DM_HTTP_LOG_REMOTE_LEVEL, log);
			}
		}

#endif
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
		strVal = boaGetVar(wp, "logMode", "");
		if (strVal[0]) {
			logcap = strVal[0] - '0' ;
			if (mib_set(MIB_SYSLOG_MODE, (void *)&logcap) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
#ifdef USE_DEONET
			int enblRemote, enblLocal;
			if(logcap == 1){
				enblRemote = 0;	enblLocal = 1;
			}else if(logcap == 2){
				enblRemote = 1;	enblLocal = 0;
			}else if(logcap == 3){
				enblRemote = 1;	enblLocal = 1;
			}
			syslog2_conf(SLOG_CONF_REMOTE_ENABLE, (char*)&enblRemote);
			syslog2_conf(SLOG_CONF_LOCAL_ENABLE, (char*)&enblLocal);
#endif
		}
#ifdef CONFIG_00R0
		strVal = boaGetVar(wp, "remoteLevel", "");
		if (strVal[0]) {
			logcap = strVal[0] - '0' ;
			if (mib_set(MIB_SYSLOG_REMOTE_LEVEL, (void *)&logcap) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
		}
#endif
#ifndef USE_DEONET
		strVal = boaGetVar(wp, "logAddr", "");
		if (strVal[0]) {
			if (!inet_aton(strVal, &inIpAddr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strWrongIP,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			if (!mib_set(MIB_SYSLOG_SERVER_IP, (void *)&inIpAddr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetIPerror,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
		}
		strVal = boaGetVar(wp, "logPort", "");
		if (strVal && strVal[0] ) {
			unsigned short intVal = atoi(strVal);
			if (!mib_set(MIB_SYSLOG_SERVER_PORT, (void *)&intVal)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tsvr_port,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
		}
#else
		char			ServerIP[MAX_REMOTE_SERVER][IP_LEN] = {0,};
		unsigned short	ServerPort[MAX_REMOTE_SERVER] = {0,};
		char			ServerUrl[URL_LEN] = {0,};
		unsigned short  ServerUrlPort;

		memset(ServerUrl, 0, URL_LEN);

		strVal = boaGetVar(wp, "logAddr1", "");
		if (strVal[0]) {
			if (!inet_aton(strVal, &inIpAddr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strWrongIP,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			if (!mib_set(MIB_SYSLOG_SERVER_IP1, (void *)&inIpAddr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetIPerror,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			strcpy(ServerIP[0], strVal);
		}
		strVal = boaGetVar(wp, "logPort1", "");
		if (strVal && strVal[0] ) {
			unsigned short intVal = atoi(strVal);
			if (!mib_set(MIB_SYSLOG_SERVER_PORT1, (void *)&intVal)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tsvr_port,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			ServerPort[0] = intVal;
		}
		strVal = boaGetVar(wp, "logAddr2", "");
		if (strVal[0]) {
			if (!inet_aton(strVal, &inIpAddr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strWrongIP,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			if (!mib_set(MIB_SYSLOG_SERVER_IP2, (void *)&inIpAddr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetIPerror,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			strcpy(ServerIP[1], strVal);
		}
		strVal = boaGetVar(wp, "logPort2", "");
		if (strVal && strVal[0] ) {
			unsigned short intVal = atoi(strVal);
			if (!mib_set(MIB_SYSLOG_SERVER_PORT2, (void *)&intVal)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tsvr_port,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			ServerPort[1] = intVal;
		}
		strVal = boaGetVar(wp, "logAddr3", "");
		if (strVal[0]) {
			if (!inet_aton(strVal, &inIpAddr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strWrongIP,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			if (!mib_set(MIB_SYSLOG_SERVER_IP3, (void *)&inIpAddr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetIPerror,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			strcpy(ServerIP[2], strVal);
		}
		strVal = boaGetVar(wp, "logPort3", "");
		if (strVal && strVal[0] ) {
			unsigned short intVal = atoi(strVal);
			if (!mib_set(MIB_SYSLOG_SERVER_PORT3, (void *)&intVal)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tsvr_port,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			ServerPort[2] = intVal;
		}
		strVal = boaGetVar(wp, "logAddr4", "");
		if (strVal[0]) {
			if (!inet_aton(strVal, &inIpAddr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strWrongIP,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			if (!mib_set(MIB_SYSLOG_SERVER_IP4, (void *)&inIpAddr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetIPerror,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			strcpy(ServerIP[3], strVal);
		}
		strVal = boaGetVar(wp, "logPort4", "");
		if (strVal && strVal[0] ) {
			unsigned short intVal = atoi(strVal);
			if (!mib_set(MIB_SYSLOG_SERVER_PORT4, (void *)&intVal)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tsvr_port,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			ServerPort[3] = intVal;
		}
		strVal = boaGetVar(wp, "logAddr5", "");
		if (strVal[0]) {
			if (!inet_aton(strVal, &inIpAddr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strWrongIP,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			if (!mib_set(MIB_SYSLOG_SERVER_IP5, (void *)&inIpAddr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetIPerror,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			strcpy(ServerIP[4], strVal);
		}
		strVal = boaGetVar(wp, "logPort5", "");
		if (strVal && strVal[0] ) {
			unsigned short intVal = atoi(strVal);
			if (!mib_set(MIB_SYSLOG_SERVER_PORT5, (void *)&intVal)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tsvr_port,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			ServerPort[4] = intVal;
		}

		strVal = boaGetVar(wp, "logUrl", "");
		if (strVal[0]) {
			if (!mib_set(MIB_SYSLOG_SERVER_URL, (void *)strVal)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetUrlerror,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			strcpy(ServerUrl, strVal);
		}
		else{
			if (!mib_set(MIB_SYSLOG_SERVER_URL, (void *)"")) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetUrlerror,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
		}

		strVal = boaGetVar(wp, "logUrlPort", "");
		if (strVal && strVal[0] ) {
			unsigned short intVal = atoi(strVal);
			if (!mib_set(MIB_SYSLOG_SERVER_URL_PORT, (void *)&intVal)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tsvr_port,sizeof(tmpBuf)-1);
				goto formSysLog_err;
			}
			ServerUrlPort = intVal;
		}

		syslog2_conf(SLOG_CONF_REMOTE_SERVER, (char*)ServerIP);
		syslog2_conf(SLOG_CONF_REMOTE_PORT, (char*)ServerPort);
		syslog2_conf(SLOG_CONF_URL, (char*)ServerUrl);
		syslog2_conf(SLOG_CONF_URL_PORT, (char*)&ServerUrlPort);

#endif
#endif
 	}

	// Set Log Server IP
#ifdef SEND_LOG
	strVal = boaGetVar(wp, "ip", "");
	if (strVal[0]) {
		if ( !inet_aton(strVal, &inIp) ) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strWrongIP,sizeof(tmpBuf)-1);
			goto formSysLog_err;
		}

		if ( !mib_set( MIB_LOG_SERVER_IP, (void *)&inIp)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto formSysLog_err;
		}
 	}

	// Set User name for Log Server
	strVal = boaGetVar(wp, "username", "");
	if (strVal[0]) {
		if ( !mib_set(MIB_LOG_SERVER_NAME, (void *)strVal)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto formSysLog_err;
		}
	}

	// Set Passeord for Log Server
	strVal = boaGetVar(wp, "passwd", "");
	if (strVal[0]) {
		if ( !mib_set(MIB_LOG_SERVER_PASSWORD, (void *)strVal)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto formSysLog_err;
		}
	}
#endif

	// Save log File
	strVal = boaGetVar(wp, "save_log", "");
	if (strVal[0])
	{
#ifndef USE_DEONET
		/*unsigned char *ptr;
		unsigned int fileSize,filelen;
		unsigned int fileSector;
		unsigned int maxFileSector;*/
		FILE *fp, *fp2;

#ifdef BB_FEATURE_SAVE_LOG
		fp = fopen("/var/run/syslogd.txt", "r");
#else
		fp=fopen("/var/log/messages","r");
#endif

		if ( fp == NULL ) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_SYSTEM_LOG_NOT_EXISTS),sizeof(tmpBuf)-1);
			goto formSysLog_err;
		}

		wp->buffer_end=0; // clear header
		boaWrite(wp, "HTTP/1.0 200 OK\n");
		boaWrite(wp, "Content-Type: application/octet-stream;\n");
		boaWrite(wp, "Content-Disposition: attachment;filename=\"messages.txt\" \n");
		boaWrite(wp, "Pragma: no-cache\n");
		boaWrite(wp, "Cache-Control: no-cache\n");
		boaWrite(wp, "\n");
#ifndef BB_FEATURE_SAVE_LOG
		fp2 = fopen("/var/log/messages.old", "r");
		if (fp2) {
			saveLogFile(wp, fp2);
			fclose(fp2);
		}
#endif
		/*
		//decide the file size
		fseek(fp, 0, SEEK_END);
		filelen = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		fileSize=filelen;

		while(fileSize>0)
		{
			char buf[0x100];
			maxFileSector=0x50;
			int nRead;

//			printf("write %d %d %08x\n",maxFileSector, fileSize, (unsigned int )ptr);
			fileSector = (fileSize>maxFileSector)?maxFileSector:fileSize;
			//fileSector = fileSize;
			nRead = fread((void *)buf, 1, fileSector, fp);

			boaWriteDataNonBlock(wp, buf, nRead);

			fileSize -= fileSector;
			ptr += fileSector;
			//wrong free....
			//free(buf);
 			//sleep(1);
		}
		*/
		saveLogFile(wp, fp);
		fclose(fp);
		return;

#else
		FILE *fp;

		system("/sbin/rm /tmp/messages.txt");
		system("/sbin/cp /var/config/log/messages /tmp/messages.txt");
		sleep(2);

		fp = fopen("/tmp/messages.txt", "r");
		if ( fp == NULL ) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_SYSTEM_LOG_NOT_EXISTS),sizeof(tmpBuf)-1);
			goto formSysLog_err;
		}

		wp->buffer_end=0; // clear header
		boaWrite(wp, "HTTP/1.0 200 OK\n");
		boaWrite(wp, "Content-Type: application/octet-stream;\n");
		boaWrite(wp, "Content-Disposition: attachment;filename=\"messages.txt\" \n");
		boaWrite(wp, "Pragma: no-cache\n");
		boaWrite(wp, "Cache-Control: no-cache\n");
		boaWrite(wp, "\n");
		saveLogFile(wp, fp);
		fclose(fp);
		return;
#endif
	}

	// Clear log file
	str = boaGetVar(wp, "clear_log", "");
	if (str[0]) {
#ifndef USE_DEONET
		unlink("/var/log/messages.old");
		unlink("/var/log/messages");
#ifdef BB_FEATURE_SAVE_LOG
		unlink("/var/config/syslogd.txt");
		unlink("/var/run/syslogd.txt");
#endif
#else
		system("/sbin/rm /var/config/log/*");
		system("/sbin/rm /var/log/*");
#endif

#ifndef USE_DEONET
		submitUrl = boaGetVar(wp, "submit-url", "");
		if (submitUrl[0])
			boaRedirect(wp, submitUrl);
		else
			boaDone(wp, 200);
#else
		wp->buffer_end=0; // clear header
		boaWrite(wp, "HTTP/1.0 200 OK\n");
		boaWrite(wp, "Content-Type: text/html; charset=utf-8\n");
		boaWrite(wp, "Pragma: no-cache\n");
		boaWrite(wp, "Cache-Control: no-cache\n");
		boaWrite(wp, "\n");
		boaWrite(wp, "<script>\n");
		boaWrite(wp, "parent.document.getElementById('시스템 로그').click();\n");
		boaWrite(wp, "</script>\n");
#endif
	  	return;
	}
#if 1
	char file_path[256] = {0, };
	char buff[256] = {0, };
	char buffLen;
	int isFirst = 1;
	FILE *fp;

	strVal = boaGetVar(wp, "print_log", "");
	if (strVal[0]) {
		strVal = boaGetVar(wp, "folder_Select", "");
		if (strVal[0])
		{
			if(!strncmp(strVal, "ram", 3))
				sprintf(file_path, "/var/log/");
			else
				sprintf(file_path, "/var/config/log/");
		}
		strVal = boaGetVar(wp, "file_List", "");
		if (strVal[0])
			sprintf(file_path, "%s%s", file_path, strVal);

		if ((fp = fopen(file_path, "r")))
		{
			while(fgets(buff, sizeof(buff), fp) != NULL){
				buffLen = strlen(buff);
				buff[buffLen - 1] = '\0';
				if(isFirst){
					isFirst = 0;
					boaWrite(wp, "<script>eval(document.getElementById('fileContent').textContent = \"%s\");</script>", buff);
				}
				else
					boaWrite(wp, "<script>eval(document.getElementById('fileContent').textContent += \"%s\");</script>", buff);
			}
		}
		fclose(fp);
#if 0
		wp->buffer_end = 0; // clear header
		boaWrite(wp, "HTTP/1.0 204 No Content\n");
		boaWrite(wp, "\n");
#endif
		return ;
	}
#endif

formSysLog_end:
#if defined(APPLY_CHANGE)
	// Mason Yu. Take effect in real time.
	stopLog();
	startLog();
#endif

// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

#ifndef USE_DEONET
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	OK_MSG(submitUrl);
#else
	OK_MSG2();
#endif
	return;
formSysLog_err:
	ERR_MSG(tmpBuf);
}

#ifdef WEB_ENABLE_PPP_DEBUG
void ShowPPPSyslog(int eid, request * wp, int argc, char **argv)
{
#ifndef CONFIG_GENERAL_WEB
	boaWrite(wp, "<tr>\n\t<td width=\"25%%\"><font size=2><b>Show PPP Debug Message&nbsp;:</b></td>\n");
	boaWrite(wp, "\t<td width=\"30%%\"><font size=2>\n");
#else
	boaWrite(wp, "<tr>\n\t<th>Show PPP Debug Message:&nbsp;</th>\n");
	boaWrite(wp, "\t<td>\n");
#endif
	boaWrite(wp, "\t\t<input type=\"radio\" value=\"0\" name=\"pppcap\">Disable&nbsp;&nbsp;");
	boaWrite(wp, "\n\t\t<input type=\"radio\" value=\"1\" name=\"pppcap\">Enable");
	boaWrite(wp, "\n\t</td>\n</tr>\n");
}
#endif

int FileList(int eid, request * wp, int argc, char **argv)
{
	char *name;
	DIR *dir;
	struct dirent *entry;
	char *folderPath = NULL;
	int isFirst = 1, isMessages = 0;

	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if (!strncmp(name, "ram", 3))
		folderPath = "/var/log";
	else
		folderPath = "/var/config/log";

	dir = opendir(folderPath);
	if (dir == NULL) {
		boaError(wp, 400, "Insufficient args\n");
		return 1;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type == DT_REG) {
			if(!strcmp(entry->d_name, "messages"))
			{
				boaWrite(wp, "\t\tfileList.innerHTML %s '<option value=\"%s\" selected >%s</option>';\n", isFirst ? "=" : "+=", entry->d_name, entry->d_name);
			}
			else
				boaWrite(wp, "\t\tfileList.innerHTML %s '<option value=\"%s\">%s</option>';\n", isFirst ? "=" : "+=", entry->d_name, entry->d_name);
			isFirst = 0;
		}
	}
	closedir(dir);

	return 0;
}

static int get_br0_info(char* ip_addr, char* subnet_mask) {
	FILE* fp;
	char buffer[128];

	fp = popen("ifconfig br0 | grep 'inet addr:'", "r");
	if (fp == NULL) {
		return -1;
	}

	if (fgets(buffer, sizeof(buffer), fp) != NULL) {
		char* token = strstr(buffer, "inet addr:");
		if (token != NULL) {
			sscanf(token, "inet addr:%s", ip_addr);
		}

		token = strstr(buffer, "Mask:");
		if (token != NULL) {
			sscanf(token, "Mask:%s", subnet_mask);
		}
	}

	pclose(fp);
	return 0;
}

static int isLansideIpaddr(request *wp)
{
	char br0IP[16] ={0,};
	char br0Mask[16] ={0,};
	char *clientIp;
	struct in_addr ip, br0, mask;

	unsigned int networkAddr, clientAddr;

	if (get_br0_info(br0IP, br0Mask) < 0)
		return 1;

	if(strncmp(wp->remote_ip_addr, "::ffff:", 7) == 0)
		clientIp = wp->remote_ip_addr + 7; /* sizeof("::ffff:") */
	else
		clientIp = wp->remote_ip_addr;

	inet_pton(AF_INET, clientIp, &ip);
	inet_pton(AF_INET, br0IP, &br0);
	inet_pton(AF_INET, br0Mask, &mask);

	networkAddr = ntohl(br0.s_addr) & ntohl(mask.s_addr);
	clientAddr = ntohl(ip.s_addr) & ntohl(mask.s_addr);

	if(networkAddr == clientAddr)
		return 1;
	return 0;
}

#define MAX_SSID_STR_LEN    30
#define AP_HOST_FILE        "/var/tmp/ap_ssid"
#define BUFLEN_32			32
struct ap_ssid_t {
	char mac[BUFLEN_32];  /* dhcp chaddr */
	char ssid[BUFLEN_32];
};

int deo_ssid_read(char *mac, char *ssid)
{
    FILE *fp;
    struct ap_ssid_t ap_ssid;

    memset(&ap_ssid, 0, sizeof(ap_ssid));
    memset(ssid, 0, BUFLEN_32);

    if (!(fp = fopen(AP_HOST_FILE, "r")))
    {
        printf("\nUnable to open %s for reading", AP_HOST_FILE);
        return -1;
    }

    while ((fread(&ap_ssid, sizeof(ap_ssid), 1, fp) == 1))
    {
        if (memcmp(mac, ap_ssid.mac, strlen(mac)) == 0)
        {
            memcpy(ssid, ap_ssid.ssid, BUFLEN_32);
            fclose(fp);
            return 0;
        }
		memset(&ap_ssid, 0, sizeof(ap_ssid));
    }

    fclose(fp);
    return 0;
}

static int getConnectHosts(request *wp, int isLanSide)
{
    int nBytesSent=0;
    int i=0, logID=0, if_total=1, br_found=0;
    int element=0, ret;
    char ipAddr[40], macAddr[40], entryMac[40], liveTime[80], *buf=NULL, *ptr=NULL
        , ifname[IFNAMSIZ]={0}, ifname_str[IFNAMSIZ]={0};
    char br_ifname[IFNAMSIZ]={0}, hostName[80], ssid[40] = {0,};
    unsigned long fsize;
    unsigned char mac_addr[6];
	char truncatedHostName[33];

    if(getDhcpClientLeasesDB(&buf, &fsize) <= 0)
        goto err;

    ptr = buf;
    while (1) {
        ret = getOneDhcpClientIncludeHostname
            (&ptr, &fsize, ipAddr, macAddr, liveTime, hostName);
        sscanf(macAddr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                &mac_addr[0], &mac_addr[1], &mac_addr[2],
                &mac_addr[3], &mac_addr[4], &mac_addr[5]);
        if_total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
        for (i=0 ; i < if_total; i++ )
        {
            sprintf(br_ifname, "br%d", i);
            if (get_host_connected_interface(br_ifname, mac_addr, ifname) == 0)
            {
                br_found=1;
                break;
            }
        }

        if (br_found)
        {
            logID = rtk_lan_LogID_get_by_if_name(ifname);
            if (logID >=0 )
                sprintf(ifname_str, "LAN%d", virt2user[logID]);
            else
                sprintf(ifname_str, "%s", ifname);
        }
        else
            memset(ifname_str, 0, sizeof(ifname_str));
        if (ret < 0)
            break;
        if (ret == 0)
            continue;
		if(!strstr(hostName, "/NAT/") && !strstr(hostName, "/BRI/"))
			continue;

        deo_ssid_read(macAddr, ssid);
		ssid[BUFLEN_32] = '\0';

        // WAN: hostname, ssid, ip, mac, port, time, button
        // LAN: button, ssid, hostname
        if (isLanSide)
        {
            if ((strstr(hostName, "NAT/AA") == NULL)
                    && (strstr(hostName, "BRI/AA") == NULL))
            {
				hostName[33] = '\0';
                nBytesSent += boaWrite(wp,
                        "<tr>\n\t<td>\n\t\t<center>\n\t\t\t<input type='button' onClick=\"apconClickLogin('%s')\" value='%s' class='basic-btn01 btn-orange2-bg' />\n\t\t</center>\n\t</td>\n\t<td><center>%s</center></td>\n\t<td><center>%s</center></td>\n</tr>\n",
                        ipAddr, multilang(LANG_CONNECT), ssid, strtok(hostName, "/"));
            }
        }
        else
		{
			hostName[33] = '\0';
			nBytesSent += boaWrite(wp,
					"<tr>\n\t<td><center>%s</center></td>\n\t<td><center>%s</center></td>\n\t<td><center>%s</center></td>\n\t<td><center>%s</center></td>\n\t<td><center>%s</center></td>\n\t<td><center>%s</center></td>\n\t<td>\n\t\t<center>\n\t\t\t<input type='submit' onClick=\"apconClickLogin('%s')\" value='%s' class='basic-btn01 btn-orange2-bg' />\n\t\t</center>\n\t</td>\n</tr>\n",
					strtok(hostName, "/"), ssid, ipAddr,
					macAddr, ifname_str, liveTime, ipAddr,
					multilang(LANG_CONNECT));
		}
    }
err:
    if (buf)
        free(buf);
    return nBytesSent;
}


int CheckApConnection(int eid, request *wp, int argc, char **argv)
{
	char *name;

	if (boaArgs(argc, argv, "%s", &name) < 1)
	{
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}
	if (!strncmp(name, "check-ip", 8))
	{
		if(isLansideIpaddr(wp))
			boaWrite(wp, "\twindow.open(\"lanApConnection.asp\", \"_blank\");");
		else
			boaWrite(wp, "\twindow.open(\"wanApConnection.asp\", \"_blank\");");
	}
    if (!strncmp(name, "load-AP-wan", 11))
    {
        getConnectHosts(wp, 0);
    }
    if (!strncmp(name, "load-AP-lan", 11))
    {
        getConnectHosts(wp, 1);
    }
	return 0;
}


int RemoteSyslog(int eid, request * wp, int argc, char **argv)
{
	char *name;

	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if (!strncmp(name, "syslog-line", 11)) {
		unsigned int line = 0;
		mib_get_s(MIB_SYSLOG_LINE, (void *)&line, sizeof(line));
		boaWrite(wp, "<tr>\n\t<th>%s:&nbsp;</th>\n", multilang(LANG_SYSLOG_LINE));
		boaWrite(wp, "\t<td><input type='text' name='logline' maxlength=\"15\" value='%d'></td>\n", line);
	}

	if (!strncmp(name, "syslog-mode", 11)) {
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
#ifndef CONFIG_GENERAL_WEB
		boaWrite(wp, "<tr>\n\t<td><font size=2><b>%s&nbsp;:</b></td>\n", multilang(LANG_MODE));
#else
		boaWrite(wp, "<tr>\n\t<th>%s:&nbsp;</th>\n", multilang(LANG_MODE));
#endif
		boaWrite(wp, "\t<td><select name='logMode' size=\"1\" onChange='cbClick(this)'>\n");
		checkWrite(eid, wp, argc, argv);
#ifdef CONFIG_00R0
		boaWrite(wp, "\n\t</select></td>\n</tr>\n");
#endif
#else
		boaWrite(wp, "<input type=\"hidden\" name=\"logMode\">\n");
#endif
	}

	if (!strncmp(name, "server-info", 11)) {
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
#ifdef CONFIG_00R0
		char *SYSLOGLEVEL[] = {"Emergency", "Alert", "Critical", "Error", "Warning", "Notice", "Infomational", "Debugging"};
		unsigned char remote_level = 0;
		int i;

		mib_get_s(MIB_SYSLOG_REMOTE_LEVEL, (void *)&remote_level, sizeof(remote_level));
#ifndef CONFIG_GENERAL_WEB
		boaWrite(wp, "<tr><td><font size=2><b>%s&nbsp;:</b></td>\n"
			"\t<td><select name='remoteLevel' size=\"1\">\n", multilang(LANG_REMOTE_LOG_LEVEL));
#else
		boaWrite(wp, "<tr><th>%s:&nbsp;</th>\n"
			"\t<td><select name='remoteLevel' size=\"1\">\n", multilang(LANG_REMOTE_LOG_LEVEL));
#endif
		for (i=0; i<8; i++) {
			if (i == remote_level)
				boaWrite(wp,"<option selected value=\"%d\">%s</option>\n", i, SYSLOGLEVEL[i]);
			else
				boaWrite(wp,"<option value=\"%d\">%s</option>\n", i, SYSLOGLEVEL[i]);
		}
		boaWrite(wp, "\t</select></td></tr>\n");

#ifndef CONFIG_GENERAL_WEB
		boaWrite(wp,	"\t<tr><td><font size=2><b>%s&nbsp;:</b></td>\n"
				"\t<td><input type='text' name='logAddr' maxlength=\"15\"></td>\n"
				"</tr>\n<tr>\n"
				"\t<td><font size=2><b>%s&nbsp;:</b></td>\n"
#else
		boaWrite(wp,	"\t<tr><th>%s:&nbsp;</th>\n"
				"\t<td><input type='text' name='logAddr' maxlength=\"15\"></td>\n"
				"</tr>\n<tr>\n"
				"\t<th>%s:&nbsp;</th>\n"
#endif
				"\t<td><input type='text' name='logPort' maxlength=\"15\"></td>\n"
				"</tr>\n", multilang(LANG_SERVER_IP_ADDR), multilang(LANG_SERVER_UDP_PORT));
#else
#ifndef USE_DEONET
		boaWrite(wp, "\n\t</select></td>\n</tr>\n"
#ifndef CONFIG_GENERAL_WEB
				"\t<td><font size=2><b>Server IP Address&nbsp;:</b></td>\n"
				"\t<td><input type='text' name='logAddr' maxlength=\"15\"></td>\n"
				"</tr>\n<tr>\n"
				"\t<td><font size=2><b>Server UDP Port&nbsp;:</b></td>\n"
#else
				"\t<th>Server IP Address:&nbsp;</th>\n"
				"\t<td><input type='text' name='logAddr' maxlength=\"15\"></td>\n"
				"</tr>\n<tr>\n"
				"\t<th>Server UDP Port:&nbsp;</th>\n"
#endif
				"\t<td><input type='text' name='logPort' maxlength=\"15\"></td>\n"
				"</tr>\n");
#else
		boaWrite(wp, "\n\t</select></td>\n<tr>\n"
			"\t<th>Server IP Address1:&nbsp;</th>\n"
			"\t<td><input type='text' name='logAddr1' maxlength=\"15\"></td>\n"
			"</tr>\n<tr>\n"
			"\t<th>Server UDP Port1:&nbsp;</th>\n"
			"\t<td><input type='text' name='logPort1' maxlength=\"15\"></td>\n"
			"</tr>\n<tr>\n"
			"\t<th>Server IP Address2:&nbsp;</th>\n"
			"\t<td><input type='text' name='logAddr2' maxlength=\"15\"></td>\n"
			"</tr>\n<tr>\n"
			"\t<th>Server UDP Port2:&nbsp;</th>\n"
			"\t<td><input type='text' name='logPort2' maxlength=\"15\"></td>\n"
			"</tr>\n<tr>\n"
			"\t<th>Server IP Address3:&nbsp;</th>\n"
			"\t<td><input type='text' name='logAddr3' maxlength=\"15\"></td>\n"
			"</tr>\n<tr>\n"
			"\t<th>Server UDP Port3:&nbsp;</th>\n"
			"\t<td><input type='text' name='logPort3' maxlength=\"15\"></td>\n"
			"</tr>\n<tr>\n"
			"\t<th>Server IP Address4:&nbsp;</th>\n"
			"\t<td><input type='text' name='logAddr4' maxlength=\"15\"></td>\n"
			"</tr>\n<tr>\n"
			"\t<th>Server UDP Port4:&nbsp;</th>\n"
			"\t<td><input type='text' name='logPort4' maxlength=\"15\"></td>\n"
			"</tr>\n<tr>\n"
			"\t<th>Server IP Address5:&nbsp;</th>\n"
			"\t<td><input type='text' name='logAddr5' maxlength=\"15\"></td>\n"
			"</tr>\n<tr>\n"
			"\t<th>Server UDP Port5:&nbsp;</th>\n"
			"\t<td><input type='text' name='logPort5' maxlength=\"15\"></td>\n"
			"</tr>\n<tr>\n"
			"\t<th>Server URL:&nbsp;</th>\n"
			"\t<td><input type='text' name='logUrl' maxlength=\"40\"></td>\n"
			"</tr>\n<tr>\n"
			"\t<th>Server URL Port:&nbsp;</th>\n"
			"\t<td><input type='text' name='logUrlPort' maxlength=\"15\"></td>\n"
			"</tr>\n"
			);
#endif
#endif
#else
		boaWrite(wp, "<input type=\"hidden\" name=\"logAddr\">\n");
		boaWrite(wp, "<input type=\"hidden\" name=\"logPort\">\n");
#endif
	}

	if (!strncmp(name, "check-ip", 8)) {
#ifndef USE_DEONET
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
		boaWrite(wp, "\tif (document.forms[0].logAddr.disabled == false && !checkIP(document.formSysLog.logAddr))\n");
		boaWrite(wp, "\t\treturn false;\n");
#endif
#else
		boaWrite(wp, "\tif (document.forms[0].logAddr1.disabled == false && !checkDeoIP(document.formSysLog.logAddr1))\n");
		boaWrite(wp, "\t\treturn false;\n");
		boaWrite(wp, "\tif (document.forms[0].logAddr2.disabled == false && !checkDeoIP(document.formSysLog.logAddr2))\n");
		boaWrite(wp, "\t\treturn false;\n");
		boaWrite(wp, "\tif (document.forms[0].logAddr3.disabled == false && !checkDeoIP(document.formSysLog.logAddr3))\n");
		boaWrite(wp, "\t\treturn false;\n");
		boaWrite(wp, "\tif (document.forms[0].logAddr4.disabled == false && !checkDeoIP(document.formSysLog.logAddr4))\n");
		boaWrite(wp, "\t\treturn false;\n");
		boaWrite(wp, "\tif (document.forms[0].logAddr5.disabled == false && !checkDeoIP(document.formSysLog.logAddr5))\n");
		boaWrite(wp, "\t\treturn false;\n");
#endif
	}
	if (!strncmp(name, "check-port", 10)) {
		boaWrite(wp, "\tif (document.forms[0].logPort1.disabled == false && !checkDeoDigitRange(document.formSysLog.logPort1, 1, 1, 65535))\n");
		boaWrite(wp, "\t\treturn false;\n");
		boaWrite(wp, "\tif (document.forms[0].logPort2.disabled == false && !checkDeoDigitRange(document.formSysLog.logPort2, 1, 1, 65535))\n");
		boaWrite(wp, "\t\treturn false;\n");
		boaWrite(wp, "\tif (document.forms[0].logPort3.disabled == false && !checkDeoDigitRange(document.formSysLog.logPort3, 1, 1, 65535))\n");
		boaWrite(wp, "\t\treturn false;\n");
		boaWrite(wp, "\tif (document.forms[0].logPort4.disabled == false && !checkDeoDigitRange(document.formSysLog.logPort4, 1, 1, 65535))\n");
		boaWrite(wp, "\t\treturn false;\n");
		boaWrite(wp, "\tif (document.forms[0].logPort5.disabled == false && !checkDeoDigitRange(document.formSysLog.logPort5, 1, 1, 65535))\n");
		boaWrite(wp, "\t\treturn false;\n");
		boaWrite(wp, "\tif (document.forms[0].logUrlPort.disabled == false && !checkDeoDigitRange(document.formSysLog.logUrlPort, 1, 1, 65535))\n");
		boaWrite(wp, "\t\treturn false;\n");
	}
	if (!strncmp(name, "check-url", 9)) {
		boaWrite(wp, "\tif (document.forms[0].logUrl.disabled == false && !isURLWithoutProtocol(document.formSysLog.logUrl))\n");
		boaWrite(wp, "\t\treturn false;\n");
	}
	if (!strncmp(name, "check-logline", 10)) {
		boaWrite(wp, "\tif (document.forms[0].logline.disabled == false && !checkDeoShortRange(document.formSysLog.logline, 1, 1, 65535, \"6000\"))\n");
		boaWrite(wp, "\t\treturn false;\n");
	}
	return 0;
}
#endif	// of CONFIG_USER_RTK_SYSLOG

int holePunghing(int eid, request * wp, int argc, char **argv)
{
	char *name;

	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if (!strncmp(name, "hole-server", 11)) {
		boaWrite(wp, "<tr>\n\t<th>%s:&nbsp;</th>\n", multilang(LANG_HOLEPUNCHING_SERVER));
		boaWrite(wp, "\t<td><input type='text' name='hole_server' maxlength=\"30\"></td></tr>\n");
	}

	if (!strncmp(name, "hole-port", 15)) {
		boaWrite(wp, "<tr>\n\t<th>%s:&nbsp;</th>\n", multilang(LANG_HOLEPUNCHING_SERVER_PORT));
		boaWrite(wp, "\t<td><input type='text' name='hole_server_port' maxlength=\"15\"></td></tr>\n");
	}

	if (!strncmp(name, "hole-interval", 13)) {
		boaWrite(wp, "<tr>\n\t<th>%s:&nbsp;</th>\n", multilang(LANG_HOLEPUNCHING_INTERVAL));
		boaWrite(wp, "\t<td><input type='text' name='hole_interval' maxlength=\"15\"></td></tr>\n");
	}

	if (!strncmp(name, "check-url", 9)) {
		boaWrite(wp, "\tif (document.forms[0].hole_server.disabled == false && !isURLWithoutProtocol(document.formHolepunching.hole_server))\n");
		boaWrite(wp, "\t\treturn false;\n");
	}
	if (!strncmp(name, "check-port", 10)) {
		boaWrite(wp, "\tif (document.forms[0].hole_server_port.disabled == false && !checkDeoShortRange(document.formHolepunching.hole_server_port, 1, 1, 65535, \"10219\"))\n");
		boaWrite(wp, "\t\treturn false;\n");
	}

	if (!strncmp(name, "check-interval", 14)) {
		boaWrite(wp, "\tif (document.forms[0].hole_interval.disabled == false && !checkDeoShortRange(document.formHolepunching.hole_interval, 1, 1, 65535, \"60\"))\n");
		boaWrite(wp, "\t\treturn false;\n");
	}
	return 0;
}

#ifdef TIME_ZONE
void formNtp(request * wp, char *path, char *query)
{
	char *submitUrl,*strVal, *tmpStr;
	char tmpBuf[100];
	char enabled=0, ntpServerIdx;
	unsigned int ext_if, interval;
#ifdef CONFIG_00R0
	int pri = 0;
#endif

	strVal = boaGetVar(wp, "save", "");

	if(strVal[0]){
		struct tm tm_time;
		time_t tm;
		struct timespec ts;
		unsigned char dst_enabled = 1, adjust_current_time = 1;

		tmpStr = boaGetVar(wp, "adjust_time", "");
		if(!strcmp(tmpStr, "ADJUST_ON"))
			adjust_current_time = 1 ;
		else
			adjust_current_time = 0 ;

		if(adjust_current_time){
			tm_time.tm_isdst = -1;  /* Be sure to recheck dst. */
			strVal = boaGetVar(wp, "year", "");
			tm_time.tm_year = atoi(strVal) - 1900;
			strVal = boaGetVar(wp, "month", "");
			tm_time.tm_mon = atoi(strVal)-1;
			strVal = boaGetVar(wp, "day", "");
			tm_time.tm_mday = atoi(strVal);
			strVal = boaGetVar(wp, "hour", "");
			tm_time.tm_hour = atoi(strVal);
			strVal = boaGetVar(wp, "minute", "");
			tm_time.tm_min = atoi(strVal);
			strVal = boaGetVar(wp, "second", "");
			tm_time.tm_sec = atoi(strVal);
			tm = mktime(&tm_time);
			if(tm < 0){
				sprintf(tmpBuf, multilang(LANG_SET_TIME_ERROR));
				goto setErr_end;
			}
			ts.tv_sec = tm;
			ts.tv_nsec = 0;
			if (clock_settime(CLOCK_REALTIME, &ts) < 0) {
				FILE *fp = NULL;
				char should_err = 1, line[32]={0};
				float uptime = 0;
#ifdef CONFIG_RTK_DEV_AP
				/*
				 * eglibc stime() can't set tm within (EPOCH+uptime).
				 * silently discard this issue for webpage user.
				 */
				fp=fopen("/proc/uptime", "r");
				if(fp)
				{
					fgets(line, sizeof(line), fp);
					if(1==sscanf(line, "%f %*f", &uptime) && tm<(int)uptime )
						should_err = 0;
					fclose(fp);
				}
#endif
				if(should_err)
				{
					sprintf(tmpBuf, multilang(LANG_CLOCK_SETTIME_ERROR));
					goto setErr_end;
				}
			}
		}

		tmpStr = boaGetVar(wp, "dst_enabled", "");
		if (!strcmp(tmpStr, "ON"))
			dst_enabled = 1;
		else
			dst_enabled = 0;
		if (mib_set(MIB_DST_ENABLED, &dst_enabled) == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_end;
		}

		tmpStr = boaGetVar(wp, "timeZone", "");
		if (tmpStr[0]) {
			FILE *fp;
			unsigned int index;

			index = strtoul(tmpStr, NULL, 0);

			if (mib_set(MIB_NTP_TIMEZONE_DB_INDEX, &index) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto setErr_end;
			}
#ifdef __UCLIBC__
			if ((fp = fopen("/etc/TZ", "w")) != NULL) {
				fprintf(fp, "%s\n", get_tz_string(index, dst_enabled));
				fclose(fp);
			}
#else
	#ifdef CONFIG_USER_GLIBC_TIMEZONE
			char cmd[128];
			char tz_location[64]={0};

			system("rm /var/localtime");
			if (dst_enabled || !is_tz_dst_exist(index)) {
				snprintf(tz_location,sizeof(tz_location),"%s",get_tz_location(index, FOR_CLI));
				format_tz_location(tz_location);

				snprintf(cmd, sizeof(cmd), "ln -s /usr/share/zoneinfo/%s /var/localtime",tz_location);
			} else {
				const char * tz_offset;

				tz_offset = get_format_tz_utc_offset(index);

				snprintf(cmd, sizeof(cmd), "ln -s /usr/share/zoneinfo/Etc/GMT%s /var/localtime",tz_offset);
			}

			//fprintf(stderr,cmd);
			system(cmd);
	#endif
#endif
		}

		tmpStr = boaGetVar(wp, "enabled", "");
		if(!strcmp(tmpStr, "ON"))
			enabled = 1 ;
		else
			enabled = 0 ;
		if ( mib_set( MIB_NTP_ENABLED, (void *)&enabled) == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_end;
		}

		tmpStr = boaGetVar(wp, "ext_if", "");
		if(tmpStr[0])
			ext_if = (unsigned int)atoi(tmpStr);
		else
			ext_if = DUMMY_IFINDEX;  // No interface selected.
		if(!mib_set(MIB_NTP_EXT_ITF, (void *)&ext_if))
		{
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_end;
		}

		tmpStr = boaGetVar(wp, "ntpServerId", "");
		if(tmpStr[0]){
			ntpServerIdx = tmpStr[0] - '0' ;
			if ( mib_set(MIB_NTP_SERVER_ID, (void *)&ntpServerIdx) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto setErr_end;
			}
		}

		tmpStr = boaGetVar(wp, "ntpServerHost1", "");
		if(tmpStr[0]){
			if ( mib_set(MIB_NTP_SERVER_HOST1, (void *)tmpStr) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto setErr_end;
			}
		}

		tmpStr = boaGetVar(wp, "ntpServerHost2", "");
		if(tmpStr[0]){
			if(strContainXSSChar(tmpStr)){
				tmpBuf[sizeof(tmpBuf)-1]='\0';
		        strncpy(tmpBuf, "Invalid ntpServerHost2!",sizeof(tmpBuf)-1);
		        goto setErr_end;
	      	}
/*ping_zhang:20081217 START:patch from telefonica branch to support WT-107*/
			if ( mib_set(MIB_NTP_SERVER_HOST2, (void *)tmpStr) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto setErr_end;
			}
/*ping_zhang:20081217 END*/

		}

#ifdef USE_DEONET /* sghong, 20230407 */
		tmpStr = boaGetVar(wp, "ntpServerHost3", "");
		if(tmpStr[0]){
			if(strContainXSSChar(tmpStr)){
				tmpBuf[sizeof(tmpBuf)-1]='\0';
		        strncpy(tmpBuf, "Invalid ntpServerHost3!",sizeof(tmpBuf)-1);
		        goto setErr_end;
	      	}
			if ( mib_set(MIB_NTP_SERVER_HOST3, (void *)tmpStr) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto setErr_end;
			}
		}
#endif

		tmpStr = boaGetVar(wp, "ntpInterval", "");
		if(tmpStr[0]){
			interval = atoi(tmpStr);
			if ( mib_set(MIB_NTP_INTERVAL, (void *)&interval) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
				goto setErr_end;
			}
		}
	}
	if (enabled == 0)
		goto  set_ntp_end;

set_ntp_end:
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ)
	restart_dnsrelay_ex("all", 0);
	/* To avoid race condition between dnsmasq and vsntp 20210510*/
	sleep(1);
#endif
#ifdef CONFIG_00R0
	mib_get_s(MIB_NTP_EXT_ITF_PRI, (void *)&pri, sizeof(pri));

	if ( pri <= NTP_EXT_ITF_PRI_LOW) {
		pri = NTP_EXT_ITF_PRI_LOW;
		mib_set(MIB_NTP_EXT_ITF_PRI, &pri);
#endif
#if defined(APPLY_CHANGE)
	if (enabled) {
		stopNTP();
		startNTP();
	} else {
		stopNTP();
	}
#endif
#ifdef CONFIG_00R0
	}
#endif

// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page

	OK_MSG(submitUrl);
	return;

setErr_end:
	ERR_MSG(tmpBuf);
}

int timeZoneList(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
	unsigned int i, selected = 0;

	mib_get_s(MIB_NTP_TIMEZONE_DB_INDEX, &selected, sizeof(selected));

	for (i = 0; i < nr_tz; i++) {
		nBytesSent += boaWrite(wp, "<option value=\"%u\"%s>%s (UTC%s)</option>",
				i, (i == selected) ? " selected" : "",
				get_tz_location(i, FOR_WEB), get_tz_utc_offset(i));
	}

	return nBytesSent;
}
#endif

#ifdef DOS_SUPPORT
void formDosCfg(request * wp, char *path, char *query)
{
	char	*submitUrl, *tmpStr;
	char	tmpBuf[100];
	char	cmd[128];
	unsigned int	floodCount=0, blockTimer=0, snmpcount=0;
	unsigned int	icmpignore=0, ntpblock=0, arpspoof=0;
	unsigned int	icmpcount = 0;
	unsigned int	enabled = 0;
	unsigned int	icmp_enable;

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page

	mib_get_s(MIB_DOS_ENABLED, (void *)&enabled, sizeof(enabled));

	tmpStr = boaGetVar(wp, "dosEnabled", "");
	if(!strcmp(tmpStr, "ON")) {
		enabled |= 1;

#if 0
		tmpStr = boaGetVar(wp, "sysfloodSYN", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 2;
			tmpStr = boaGetVar(wp, "sysfloodSYNcount", "");
			string_to_dec(tmpStr,&floodCount);
			if ( mib_set(MIB_DOS_SYSSYN_FLOOD, (void *)&floodCount) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetDosSYSSYNFLOODErr,sizeof(tmpBuf)-1);
				goto setErr;
			}
		}
		else{
			enabled &= ~2;
		}
		tmpStr = boaGetVar(wp, "sysfloodFIN", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 4;
			tmpStr = boaGetVar(wp, "sysfloodFINcount", "");
			string_to_dec(tmpStr,&floodCount);
			if ( mib_set(MIB_DOS_SYSFIN_FLOOD, (void *)&floodCount) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetDosSYSFINFLOODErr,sizeof(tmpBuf)-1);
				goto setErr;
			}
		}
		else{
			enabled &= ~4;
		}
		tmpStr = boaGetVar(wp, "sysfloodUDP", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 8;
			tmpStr = boaGetVar(wp, "sysfloodUDPcount", "");
			string_to_dec(tmpStr,&floodCount);
			if ( mib_set(MIB_DOS_SYSUDP_FLOOD, (void *)&floodCount) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetDosSYSUDPFLOODErr,sizeof(tmpBuf)-1);
				goto setErr;
			}
		}
		else{
			enabled &= ~8;
		}
		tmpStr = boaGetVar(wp, "sysfloodICMP", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x10;
			tmpStr = boaGetVar(wp, "sysfloodICMPcount", "");
			string_to_dec(tmpStr,&floodCount);
			if ( mib_set(MIB_DOS_SYSICMP_FLOOD, (void *)&floodCount) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetDosSYSICMPFLOODErr,sizeof(tmpBuf)-1);
				goto setErr;
			}
		}
		else{
			enabled &= ~0x10;
		}
#endif
		tmpStr = boaGetVar(wp, "ipfloodSYN", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x20;
			tmpStr = boaGetVar(wp, "ipfloodSYNcount", "");
			string_to_dec(tmpStr,&floodCount);
			if ( mib_set(MIB_DOS_PIPSYN_FLOOD, (void *)&floodCount) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetDosPIPSYNFLOODErr,sizeof(tmpBuf)-1);
				goto setErr;
			}
		}
		else{
			enabled &= ~0x20;
		}
		tmpStr = boaGetVar(wp, "ipfloodFIN", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x40;
			tmpStr = boaGetVar(wp, "ipfloodFINcount", "");
			string_to_dec(tmpStr,&floodCount);
			if ( mib_set(MIB_DOS_PIPFIN_FLOOD, (void *)&floodCount) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetDosPIPFINFLOODErr,sizeof(tmpBuf)-1);
				goto setErr;
			}
		}
		else{
			enabled &= ~0x40;
		}
		tmpStr = boaGetVar(wp, "ipfloodUDP", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x80;
			tmpStr = boaGetVar(wp, "ipfloodUDPcount", "");
			string_to_dec(tmpStr,&floodCount);
			if ( mib_set(MIB_DOS_PIPUDP_FLOOD, (void *)&floodCount) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetDosPIPUDPFLOODErr,sizeof(tmpBuf)-1);
				goto setErr;
			}
		}
		else{
			enabled &= ~0x80;
		}
		tmpStr = boaGetVar(wp, "portfloodUDP", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x2000000;
			tmpStr = boaGetVar(wp, "portfloodUDPcount", "");
			string_to_dec(tmpStr,&snmpcount);
			if ( mib_set(MIB_DOS_SNMP_FLOOD, (void *)&snmpcount) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetDosSNMPFLOODErr,sizeof(tmpBuf)-1);
				goto setErr;
			}
		}
		else{
			enabled &= ~0x2000000;
		}
		tmpStr = boaGetVar(wp, "ipfloodICMP", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x100;
			tmpStr = boaGetVar(wp, "ipfloodICMPcount", "");
			string_to_dec(tmpStr,&floodCount);
			if ( mib_set(MIB_DOS_PIPICMP_FLOOD, (void *)&floodCount) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetDosPIPICMPFLOODErr,sizeof(tmpBuf)-1);
				goto setErr;
			}
		}
		else{
			enabled &= ~0x100;
		}

		tmpStr = boaGetVar(wp, "ratelimitICMP", "");
		if(!strcmp(tmpStr, "ON")) {
			icmp_enable = 1;
			mib_set(MIB_DOS_RATELIMIT_ICMP_ENABLED, (void *)&icmp_enable);

			tmpStr = boaGetVar(wp, "ratelimitICMPcount", "");
			string_to_dec(tmpStr, &icmpcount);
			if (mib_set(MIB_DOS_RATELIMIT_ICMP_FLOOD, (void *)&icmpcount) == 0) {
				tmpBuf[sizeof(tmpBuf) - 1] = '\0';
				strncpy(tmpBuf, strSetDosRATELIMITICMPErr, sizeof(tmpBuf) - 1);
				goto setErr;
			}

			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd , "echo %d > /proc/sys/net/ipv4/icmp_ratelimit ", icmpcount);
			system(cmd);
			system("echo 6169 > /proc/sys/net/ipv4/icmp_ratemask");
		} else {
			icmp_enable = 0;
			icmpcount = 10;

			mib_set(MIB_DOS_RATELIMIT_ICMP_ENABLED, (void *)&icmp_enable);
			mib_set(MIB_DOS_RATELIMIT_ICMP_FLOOD, (void *)&icmpcount);
			system("echo 10 > /proc/sys/net/ipv4/icmp_ratelimit");
			system("echo 6169 > /proc/sys/net/ipv4/icmp_ratemask");
		}

		tmpStr = boaGetVar(wp, "TCPUDPPortScan", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x200;

			tmpStr = boaGetVar(wp, "portscanSensi", "");
			if( tmpStr[0]=='1' ) {
				enabled |= 0x800000;
			}
			else{
				enabled &= ~0x800000;
			}
		}
		else{
			enabled &= ~0x200;
		}
		tmpStr = boaGetVar(wp, "ICMPSmurfEnabled", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x400;
		}
		else{
			enabled &= ~0x400;
		}
		tmpStr = boaGetVar(wp, "IPLandEnabled", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x800;
		}
		else{
			enabled &= ~0x800;
		}
		tmpStr = boaGetVar(wp, "IPSpoofEnabled", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x1000;
		}
		else{
			enabled &= ~0x1000;
		}
		tmpStr = boaGetVar(wp, "IPTearDropEnabled", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x2000;
		}
		else{
			enabled &= ~0x2000;
		}
		tmpStr = boaGetVar(wp, "PingOfDeathEnabled", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x4000;
		}
		else{
			enabled &= ~0x4000;
		}
		tmpStr = boaGetVar(wp, "TCPScanEnabled", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x8000;
		}
		else{
			enabled &= ~0x8000;
		}
		tmpStr = boaGetVar(wp, "TCPSynWithDataEnabled", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x10000;
		}
		else{
			enabled &= ~0x10000;
		}
		tmpStr = boaGetVar(wp, "UDPBombEnabled", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x20000;
		}
		else{
			enabled &= ~0x20000;
		}
		tmpStr = boaGetVar(wp, "UDPEchoChargenEnabled", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x40000;
		}
		else{
			enabled &= ~0x40000;
		}
		tmpStr = boaGetVar(wp, "sourceIPblock", "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x400000;
			tmpStr = boaGetVar(wp, "IPblockTime", "");
			string_to_dec(tmpStr,&blockTimer);
			if ( mib_set(MIB_DOS_BLOCK_TIME, (void *)&blockTimer) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetDosIPBlockTimeErr,sizeof(tmpBuf)-1);
				goto setErr;
			}
		}
		else{
			enabled &= ~0x400000;
		}

		tmpStr = boaGetVar(wp, "IcmpEchoIgnoreEnabled", "");
		if (!strcmp(tmpStr, "ON")) {
			icmpignore = 1;
			enabled |= 0x4000000;
			mib_set(MIB_DOS_ICMP_IGNORE, (void *)&icmpignore);
		} else {
			enabled &= ~0x4000000;
			mib_set(MIB_DOS_ICMP_IGNORE, (void *)&icmpignore);
		}

		tmpStr = boaGetVar(wp, "NtpAttackBlockEnabled", "");
		if (!strcmp(tmpStr, "ON")) {
			ntpblock = 1;
			enabled |= 0x8000000;
			mib_set(MIB_DOS_NTP_BLOCK, (void *)&ntpblock);
		} else {
			enabled &= ~0x8000000;
			mib_set(MIB_DOS_NTP_BLOCK, (void *)&ntpblock);
		}

		tmpStr = boaGetVar(wp, "ARPSpoofEnabled", "");

		if(!strcmp(tmpStr, "ON")) {
			system("echo start > /proc/arp_spoof");
			arpspoof = 1;
			enabled |= 0x10000000;
			mib_set(MIB_DOS_ARP_SPOOF, (void *)&arpspoof);
		}
		else{
			system("echo end > /proc/arp_spoof");
			arpspoof = 0;
			enabled &= ~0x10000000;
			mib_set(MIB_DOS_ARP_SPOOF, (void *)&arpspoof);
		}
	} else {
		enabled = 0;
		// arp spoof do not use enable value, so set proc
		system("echo end > /proc/arp_spoof");

		icmp_enable = 0;
		icmpcount = 10;

		mib_set(MIB_DOS_RATELIMIT_ICMP_ENABLED, (void *)&icmp_enable);
		mib_set(MIB_DOS_RATELIMIT_ICMP_FLOOD, (void *)&icmpcount);
		system("echo 10 > /proc/sys/net/ipv4/icmp_ratelimit");
		system("echo 6168 > /proc/sys/net/ipv4/icmp_ratemask");
	}

	if ( mib_set(MIB_DOS_ENABLED, (void *)&enabled) == 0) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, strSetDosEnableErr,sizeof(tmpBuf)-1);
		goto setErr;
	}

	//apmib_update(CURRENT_SETTING);
#if defined(APPLY_CHANGE)
	setup_dos_protection();
#endif

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#ifdef CONFIG_RTK_DEV_AP
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
#endif

#ifndef NO_ACTION
	run_init_script("all");
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	OK_MSG(submitUrl);

	return;
setErr:
	ERR_MSG(tmpBuf);
}
#endif

#ifdef WEB_REDIRECT_BY_MAC
void formLanding(request * wp, char *path, char *query)
{
	char *submitUrl, *strLTime;
	unsigned int uLTime;

	strLTime = boaGetVar(wp, "interval", "");
	if ( strLTime[0] ) {
		sscanf(strLTime, "%u", &uLTime);
	}

	mib_set(MIB_WEB_REDIR_BY_MAC_INTERVAL, (void *)&uLTime);

	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		//boaRedirect(wp, submitUrl);
		OK_MSG(submitUrl);
	return;
}
#endif
void formVersionMod(request *wp, char *path, char *query)
{
	char *strData;

	// manufacturer
	strData = boaGetVar(wp,"txt_mft","");
	if (strData[0])
	{
		if (!mib_set(RTK_DEVID_MANUFACTURER, strData)) {
			goto setErr_tr069;
		}
		else printf("Update Manufacturer to %s \n" , strData);
	}

	// OUI
	strData = boaGetVar(wp,"txt_oui","");
 	if (strData[0])
	{
		if (!mib_set(RTK_DEVID_OUI, strData)) {
			goto setErr_tr069;
		}
		else printf("Update OUI to %s \n" , strData);
	}

	// Product Class
	strData = boaGetVar(wp,"txt_proclass","");
	if (strData[0])
	{
		if (!mib_set( RTK_DEVID_PRODUCTCLASS, strData)) {
			goto setErr_tr069;
		}
		else printf("Update Product Class to %s \n" , strData);
	}

	// HW Serial Number
	strData = boaGetVar(wp,"txt_serialno","");
	if (strData[0])
	{
		if (!mib_set(MIB_HW_SERIAL_NUMBER, (void *)strData)) {
			goto setErr_tr069;
		}
		else printf("Update Serial Number to %s \n" , strData);
	}

	// Provisioning Code
#ifdef CONFIG_USER_CWMP_TR069
	strData = boaGetVar(wp,"txt_provisioningcode","");
	if (strData[0])
	{
		if (!mib_set(CWMP_PROVISIONINGCODE, (void *)strData)) {
			goto setErr_tr069;
		}
		else printf("Update Provisioning Code to %s \n" , strData);
	}
#endif

	// Spec. Version
	strData = boaGetVar(wp,"txt_specver","");
	if (strData[0])
	{
		if (!mib_set(RTK_DEVINFO_SPECVER, (void *)strData)) {
			goto setErr_tr069;
		}
		else printf("Update Spec. Version to %s \n" , strData);
	}

	// Software Version
	strData = boaGetVar(wp,"txt_swver","");
	if (strData[0])
	{
		if (!mib_set(RTK_DEVINFO_SWVER, (void *)strData)) {
			goto setErr_tr069;
		}
		else printf("Update Software Version to %s \n" , strData);
	}

	// Hardware Version
	strData = boaGetVar(wp,"txt_hwver","");
	if (strData[0])
	{
		if (!mib_set(RTK_DEVINFO_HWVER, (void *)strData)) {
			goto setErr_tr069;
		}
		else printf("Update Hardware Version to %s \n" , strData);
	}
#if defined(CONFIG_GPON_FEATURE)
	//GPON SN
	strData = boaGetVar(wp,"txt_gponsn","");
	if (strData[0])
	{
		if (!mib_set(MIB_GPON_SN, (void *)strData)) {
			goto setErr_tr069;
		}
		else printf("Update GPON SN to %s \n" , strData);
	}
#endif
	// ELAN MAC Address
	strData = boaGetVar(wp,"txt_elanmac","");
	if (strData[0])
	{
		unsigned char mac[6];
		if ( !rtk_string_to_hex(strData, mac, 12)) {
			goto setErr_tr069;
		}
		if (!mib_set(MIB_ELAN_MAC_ADDR, (void *)mac)) {
			goto setErr_tr069;
		}
		else printf("Update ELAN MAC Address to %s \n" , strData);
	}


	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	mib_update(HW_SETTING, CONFIG_MIB_ALL);


	strData = boaGetVar(wp, "submit-url", "");
	if (strData[0])
		boaRedirect(wp, strData);

	return;

setErr_tr069:
	ERR_MSG(multilang(LANG_ERROR_SETTING));
}

#ifdef USER_WEB_WIZARD
void formWebWizardMenu(request * wp, char *path, char *query)
{
	char *submitUrl;

	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
		//OK_MSG(submitUrl);
	return;
}

void formWebWizard1(request * wp, char *path, char *query)
{
	char *submitUrl, *strPassword, *strConfPassword;
	char tmpBuf[100];

	strPassword = boaGetVar(wp, "newpass", "");
	strConfPassword = boaGetVar(wp, "confirmpass", "");

	if ( !strPassword[0] ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_EMPTY_NEW_PASSWORD,sizeof(tmpBuf)-1);
		goto setErr_pass;
	}

	if ( !strConfPassword[0] ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_EMPTY_CONFIRMED_PASSWORD,sizeof(tmpBuf)-1);
		goto setErr_pass;
	}

	if (strcmp(strPassword, strConfPassword) != 0 ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_UNMATCHED_PASSWORD,sizeof(tmpBuf)-1);
		goto setErr_pass;
	}

	if (!mib_set(MIB_SUSER_PASSWORD, (void *)strPassword)) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_SET_PASSWORD,sizeof(tmpBuf)-1);
		goto setErr_pass;
	}

#ifdef EMBED
	// Added by Mason Yu for take effect on real time
	mib_get_s(MIB_SUSER_NAME, (void *)suName, sizeof(suName));
	mib_get_s(MIB_USER_NAME, (void *)usName, sizeof(usName));
	rtk_util_update_boa_user_account();
	rtk_util_update_user_account();	// Jenny
#endif

// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	/* Init user management */
	// Commented By Mason Yu
	//set_user_profile();

	/* Retrieve next page URL */
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page

	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	//OK_MSG(submitUrl);
	return;

setErr_pass:
	ERR_MSG(tmpBuf);
}

void formWebWizard4(request * wp, char *path, char *query)
{
	char *username, *passwd, *submitUrl, *strValue;
	char tmpBuf[100];
	MIB_CE_ATM_VC_T Entry;
	unsigned int totalEntry;
	int i;

	// user name
	username = boaGetVar(wp, "pppusername", "");
	if ( username[0] ) {
		if (strlen(username) > MAX_PPP_NAME_LEN) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strUserNametoolong,sizeof(tmpBuf)-1);
			goto setErr_ppp;
		}
	}
	else {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, strUserNameempty,sizeof(tmpBuf)-1);
		goto setErr_ppp;
	}

	// password
	passwd = boaGetVar(wp, "ppppassword", "");
	if ( passwd[0] ) {
		if (strlen(passwd) > MAX_NAME_LEN-1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strPasstoolong,sizeof(tmpBuf)-1);
			goto setErr_ppp;
		}
	}
	else {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, strPassempty,sizeof(tmpBuf)-1);
		goto setErr_ppp;
	}

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<totalEntry; i++) {
		struct data_to_pass_st msg;
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
  			printf(strGetChainerror);
			return;
		}
		if (Entry.enable == 0)
			continue;
#ifdef CONFIG_00R0
		stopConnection(&Entry);
#endif
		if (Entry.cmode == CHANNEL_MODE_PPPOE) {
			strncpy(Entry.pppUsername, username, MAX_PPP_NAME_LEN);
			Entry.pppUsername[strlen(username)]='\0';
			strncpy(Entry.pppPassword, passwd, MAX_NAME_LEN-1);
			Entry.pppPassword[strlen(passwd)]='\0';
			strValue = boaGetVar(wp, "ppp_vlan", "");
			Entry.vid = (unsigned int) strtol(strValue, (char**)NULL, 10);
			if (Entry.vid > 0)
				Entry.vlan = 1;
			mib_chain_update(MIB_ATM_VC_TBL, (void *)&Entry, i);
		}
		else
			continue;
	}
#ifdef CONFIG_00R0
	restartWAN(CONFIGALL, NULL);
#endif
	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
  	return;

setErr_ppp:
	ERR_MSG(tmpBuf);
}

int ShowWebWizardPage(int eid, request * wp, int argc, char **argv)
{
#ifdef WLAN_SUPPORT
	boaWrite(wp, "<input type=\"hidden\" value=\"/web_wizard_5.asp\" name=\"submit-url\">\n");
#else
	boaWrite(wp, "<input type=\"hidden\" value=\"/web_wizard_6.asp\" name=\"submit-url\">\n");
#endif
return 0;
}

#ifdef WLAN_SUPPORT
static void setWlan(request * wp,char disable,char* ssid,WIFI_SECURITY_T encryption,char* key)
{
	char curssid[32],tmpBuf[100];
	MIB_CE_MBSSIB_T Entry;

	if (disable == 1)
		return;

	wlan_getEntry(&Entry, 0);
	Entry.wlanDisabled = disable;
	Entry.ssid[sizeof(Entry.ssid)-1]='\0';
	strncpy(Entry.ssid, ssid,sizeof(Entry.ssid)-1);
	Entry.encrypt = encryption;

	Entry.wpaAuth = WPA_AUTH_PSK;
	Entry.unicastCipher = WPA_CIPHER_TKIP;
	Entry.wpa2UnicastCipher = WPA_CIPHER_AES;
	Entry.wpaPSKFormat = 0;//passphrase
	Entry.wpaPSK[sizeof(Entry.wpaPSK)-1]='\0';
	strncpy(Entry.wpaPSK, key,sizeof(Entry.wpaPSK)-1);
	wlan_setEntry(&Entry, 0);
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
}
#endif

void formWebWizard5(request * wp, char *path, char *query)
{
	char *strValue,*ssid;
	char *strSubmit;
	char tmpBuf[100], disable;
	unsigned char encryption;
	int tempWlanIdx;

	strValue = boaGetVar(wp,"wlanDisabled","");
	if (!gstrcmp(strValue, "ON"))
		disable = 1;
	else
		disable = 0;

#ifdef WLAN_SUPPORT
	strValue = boaGetVar(wp,"psk","");
	encryption = WIFI_SEC_WPA2_MIXED;

	tempWlanIdx = wlan_idx;

	wlan_idx = 0;
	ssid = boaGetVar(wp,"ssid5g","");
	setWlan(wp, disable, ssid, (WIFI_SECURITY_T)encryption, strValue);

	wlan_idx = 1;
	ssid = boaGetVar(wp,"ssid2g","");
	setWlan(wp, disable, ssid, (WIFI_SECURITY_T)encryption, strValue);

	wlan_idx = tempWlanIdx;
#endif

setOK:
	strSubmit = boaGetVar(wp,"submit-url","");
	if(strSubmit[0])
		boaRedirect(wp, strSubmit);
	else
		boaDone(wp, 200);
	return;
}

void formWebWizard6(request * wp, char *path, char *query)
{
	char *strSubmit;
	unsigned char webwizard_flag = 0;

	mib_set(MIB_USER_WEB_WIZARD_FLAG, (void *)&webwizard_flag);

#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
	// webwizard_flag from 1 to 0, update redirect rule depend on optical link
	update_redirect_by_ponStatus();
#endif

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#ifdef WLAN_SUPPORT
	config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
#endif

setOK:
	//printf("[%s:%d] webwizard_flag=%d\n", __func__, __LINE__, webwizard_flag);
	strSubmit=boaGetVar(wp,"submit-url","");
	//printf("[%s:%d] strSubmit=%s\n", __func__, __LINE__, strSubmit);
	if(strSubmit[0])
		boaRedirect(wp, strSubmit);
	else
		boaDone(wp, 200);
	return;
}
#endif

#ifdef CONFIG_00R0
/* siyuan 2016-01-25 add wizard screen functions */
enum {
	WIZARD_SETUP_SIGNAL_1490 = 0,   /* Is signal on 1490 */
	WIZARD_SETUP_OMCI_SETTING,      /* Got OMCI settings */
	WIZARD_SETUP_ACCESS_ACS,      	/* Got access to ACS */
	WIZARD_SETUP_ONT_LOAD,  		/* Was derective ONT-LOAD */
	WIZARD_SETUP_PLOAM_ENTER,   	/* Was PLOAM entered */
	WIZARD_SETUP_DEFAULT_ROUTE,		/* Check connection of default route */
};

enum {
	WEB_DEFAULT_URL = 0,
	WEB_DEFAULT_RT,
	WIZARD_URL_0_1,
	WIZARD_URL_1_1,
	WIZARD_URL_1_2,
	WIZARD_URL_4_1,
	WIZARD_URL_4_1_1,
	WIZARD_URL_4_2,
	WIZARD_URL_5_1,
	WIZARD_URL_5_2,
	WIZARD_URL_4_1_2,
	WIZARD_URL_0_1_2019,
	WIZARD_URL_1_2019,
	WIZARD_URL_1_1_2019,
	WIZARD_URL_2_1_2019,
	WIZARD_URL_2_2_2019,
};

enum {
	WIZARD_STATE_1490_SIGNAL = 0,
	WIZARD_STATE_OMCI_CONFIG,
	WIZARD_STATE_ACS_CONFIG,
	WIZARD_STATE_PLOAM_SET,
	WIZARD_STATE_DEFAULT_ROUTE,
};

enum {
	SERVICE_INFO_WRONG_PPPOE_PWD = 0,
	SERVICE_INFO_NO_PPPOE_CONNECT = 1,
	SERVICE_INFO_NO_IPOE_CONNECT = 2,
	SERVICE_INFO_NO_WAN_CONNECT = 3,
	SERVICE_INFO_NOT_REGISTER = 4,
	SERVICE_INFO_CONNECTED = 5,
};

#define SETUP_WIZARD_TRY_NUM 3
#define TROUBLE_WIZARD_TRY_NUM 2

#define SETUP_WIZARD_TYPE 1
#define TROUBLE_WIZARD_TYPE 2

#define WIZARD_4_1_1_INTERVAL 5
#define WIZARD_4_1_1_TIME 60

static char * wizardUrl[] = {
	"http://192.168.0.1/",
	"http://rt.ru/",
	"/admin/wizard_screen_0_1.asp",
	"/admin/wizard_screen_1_1.asp",
	"/admin/wizard_screen_1_2.asp",
	"/admin/wizard_screen_4_1.asp",
	"/admin/wizard_screen_4_1_1.asp",
	"/admin/wizard_screen_4_2.asp",
	"/admin/wizard_screen_5_1.asp",
	"/admin/wizard_screen_5_2.asp",
	"/admin/wizard_screen_4_1_2.asp",
	"/admin/wizard_screen_0_1_2019.asp",
	"/admin/wizard_screen_1_2019.asp",
	"/admin/wizard_screen_1_1_2019.asp",
	"/admin/wizard_screen_2_1_2019.asp",
	"/admin/wizard_screen_2_2_2019.asp",
};

/* to webUI If arrived screen4.2 or screen5.2 */
static int wizard1_1_to_web = 0;
static int wizard1_1_state = 0;

static int wizard5_1_times = 0;
static int wizard5_1_try_num = 0;

/* Setup wizard or Trouble wizard*/
static int wizard5_type;

static int wizard4_1_1_state = 0;
static int wizard4_1_1_EndTime = 0;
static int wizard4_1_1_remainTime = 0;
static int into_PLOAM = 0;

/* 	Timer change for RTC request, there have 3 timer for 60 secs,
	1. signal_to_1490_timer : from 60 to 0 , can be end if omci configured is finished & timer >=0
	2. invitation_to_ploam_timer : from 60 to 0, shoud be count from 60 to 0,
	3. acs_connection_timer : from 60 to 0, shoud be count from 60 to 0,					*/

/* 	Flag of signal 1490 timer, it should be count from receieving the signal 1490 to OMCI configuration is done,
	According to the RTC, it can be end if OMCI was configured and the remaining time > 0  			*/
static int signal_to_1490_timer_flag = 1;

/* 	When receieve the pon signal, we shoud start to enter the process of wizard,
	The procedure of wizard should be end in 60secs*3 = 180 secs.			*/
static long signal_to_1490_to_end_timer = 0;

/*	Flag of ploam timer,  once the OMCI configuration failed, it should be input the PLOAM password ,
	This timer should be count from 60 to 0 for RTC request.							*/
static int invitation_to_ploam_timer_flag = 1;

/*	Flag of ACS timer, Once the OMCI configuration sucessed, it shoud be enter the procedure of ACS connection,
	This timer shoud be count from 60 to 0 for RTC  request									*/
static int acs_connection_timer_flag = 1;

/*	Flag of Default Route timer, Once the OMCI configuration and  ACS connection, shoud be test internetwan connection
	This timer shoud be count from 60 to 0 for RTC  request									*/
static int default_route_connection_timer_flag = 1;

/* means if there was a successful internet connection */
int connectOnceUp = 0;

/* Redirect to 1.1 screen if it's first start and not in Trouble wizard */
static int wizard_first_time = 1;

static char rthomehtml[] = {
	"<html><body>\n<script>\n"
	"top.location.href='http://rt.ru';\n"
	"</script>\n</body></html>"
};

static char backhtml[] = {
	"<html><body>\n<script>\n"
	"top.location.href='/';\n"
	"</script>\n</body></html>"
};

static char backHomehtml[] = {
	"<html><body>\n<script>\n"
	"window.open(\"/admin/login.asp\",target=\"_top\"); \n"
	"</script>\n</body></html>"
};

static char * serviceInfo[] = {
	"PPPoE connection - Wrong password and/or PPPoE login (if PPPoE authorization fail)",
	"PPPoE connection - No PPPoE connection(PADI do not respond)",
	"IPoE connection - No IPoE connection (in case if DHCP server do not respond)",
	"Optic cable is not connected",
	"cable is connected but device not registered(or registration fail) - Device is not registered",
	"Connected",
};

static int isSignalOn1490()
{
	int ret = 0;
	int state;

	/* gpon get onu-state: check if state is not RTK_GPONMAC_FSM_STATE_O1 */
	state = getGponONUState();
	if(state > RTK_GPONMAC_FSM_STATE_O1)
		ret = 1;

	printf("%s ret[%d]\n", __func__, ret);
	return ret;
}

static int getOmciSetting()
{
	int ret = 0;
	unsigned char status;

	if(mib_get_s(RS_OMCI_CONFIG_STATUS, &status, sizeof(status)))
	{
		if(status == 1)
			ret = 1;
	}

	printf("%s ret[%d]\n", __func__, ret);
	return ret;
}

static int getAccessToACS()
{
	int ret = 0;
	unsigned char status;

	if(mib_get_s(RS_CWMP_BOOTSTRAP_STATUS, &status, sizeof(status)))
	{
		if(status == CWMP_STATUS_SUCCESS)
			ret = 1;
	}

	printf("%s ret[%d]\n", __func__, ret);
	return ret;
}

static int isPloamEntered()
{
	int ret = 0;
	char tmpBuf[100], tmpBuf2[100];

	mib_get_s(MIB_GPON_PLOAM_PASSWD,  (void *)tmpBuf, sizeof(tmpBuf));
	mib_get_s(MIB_GPON_PLOAM_PASSWD_BACKUP,  (void *)tmpBuf2, sizeof(tmpBuf2));

	if(!tmpBuf[0])
		ret = 0;
	else if (strcmp(tmpBuf, tmpBuf2) == 0 ){
		ret = 1;
	}

	printf("%s ret[%d]\n", __func__, ret);
	return ret;
}

static int isConfigurationOmciSuccess()
{
	int ret = 0;

	ret = getOmciSetting();

	printf("%s ret[%d]\n", __func__, ret);
	return ret;
}

static int isACSConnectionSuccess()
{
	int ret = 0;

	ret = getAccessToACS();

	printf("%s ret[%d]\n", __func__, ret);
	return ret;
}

static int isPloamCodeSetSuccess()
{
	int ret = 0;

	ret = isPloamEntered();

	printf("%s ret[%d]\n", __func__, ret);
	return ret;
}

static int isDerectiveOntLoad()
{
	int ret = 0;

	/* TODO: check if receives ME305 information (configuration file) */

	printf("%s ret[%d]\n", __func__, ret);
	return ret;
}

static int isInternetConnectionUp()
{
	int i=0, ret = 0, atmVcEntryNum=0;
	MIB_CE_ATM_VC_T atmVcEntry;
	char ifname[IFNAMSIZ]={0}, gw_addr[32]={0};
	struct in_addr inAddr;

	atmVcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<atmVcEntryNum; i++)
	{
		if (mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&atmVcEntry))
		{
			if((atmVcEntry.applicationtype & (X_CT_SRV_INTERNET)) &&
				(atmVcEntry.cmode != CHANNEL_MODE_BRIDGE) &&
				(atmVcEntry.dgw != 0))
			{
				ifGetName(atmVcEntry.ifIndex,ifname,sizeof(ifname));
				if (getInAddr(ifname, DST_IP_ADDR, (void *)&inAddr) == 1)
				{
					//strcpy(gw_addr, inet_ntoa(inAddr));
					//printf(" [%s:%d] ifname=%s gw_addr=%s\n", __FUNCTION__, __LINE__, ifname, gw_addr);
					ret = 1;
					break;
				}
				else
					continue;
			}
		}
	}
	printf("%s ret[%d]\n", __func__, ret);
	return ret;
}

static int getNextUrl(int step)
{
	int urlIdx = WEB_DEFAULT_URL;
	int finish = 0;
	int stepNum;
	struct sysinfo info;
	sysinfo(&info);
	long endTime =0;

	stepNum = step;

	while(finish == 0)
	{
		switch(stepNum)
		{
		case WIZARD_SETUP_SIGNAL_1490:
			if(isSignalOn1490())
			{
				/* Initail the 1490 timer for 60 seconds */
				if (signal_to_1490_timer_flag){
					endTime = getSignal1490_end_timer();
					wizard4_1_1_EndTime = endTime + WIZARD_4_1_1_TIME;
					wizard4_1_1_remainTime = wizard4_1_1_EndTime - info.uptime;
					signal_to_1490_timer_flag=0;
				}

				urlIdx = WIZARD_URL_4_1_1;
				finish = 1;
			}
			else
			{
				urlIdx = WIZARD_URL_5_1;
				finish = 1;
			}
			break;
		case WIZARD_SETUP_OMCI_SETTING:
			if (wizard4_1_1_remainTime > 0 )
			{
				urlIdx = WIZARD_URL_4_1_1;
				finish = 1;
			}
			else if(getOmciSetting())
			{
				urlIdx = WIZARD_URL_4_1_1;
				finish = 1;
				wizard4_1_1_state = WIZARD_STATE_ACS_CONFIG;
			}
			else
			{
				stepNum = WIZARD_SETUP_PLOAM_ENTER;
				wizard4_1_1_state = WIZARD_STATE_PLOAM_SET;
			}
			break;
		case WIZARD_SETUP_ACCESS_ACS:
			/*	If ACS is work and the timer is expired, it should be go to the result page 	*/
			if(getAccessToACS() && (info.uptime > signal_to_1490_to_end_timer ))
			{
				urlIdx = WIZARD_URL_1_1;
				finish = 1;
			}
			else if ( wizard4_1_1_remainTime > 0 )
			{
				urlIdx = WIZARD_URL_4_1_1;
				finish = 1;
			}
			else if(getAccessToACS())
			{
				urlIdx = WIZARD_URL_1_1;
				finish = 1;
			}
			else
			{
				stepNum = WIZARD_SETUP_ONT_LOAD;
			}
			break;
		case WIZARD_SETUP_ONT_LOAD:
			if(isDerectiveOntLoad())
			{
				urlIdx = WIZARD_URL_1_1;
				finish = 1;
			}
			else
			{
				urlIdx = WIZARD_URL_4_2;
				finish = 1;
			}
			break;
		case WIZARD_SETUP_PLOAM_ENTER:
			if(isPloamEntered())
			{
				urlIdx = WIZARD_URL_5_2;
				finish = 1;
			}
			else
			{
				urlIdx = WIZARD_URL_0_1;
				finish = 1;
			}
			break;
		default:
			urlIdx = WEB_DEFAULT_URL;
			finish = 1;
			printf("%s error step[%d]\n", __func__, stepNum);
			break;
		}
	}

	return urlIdx;
}

int updateWizard4_1_1_Timer(void)
{
	struct sysinfo info;
	sysinfo(&info);
	fprintf(stderr, "\033[1;31m[%s:%s@%d]\033[0m info.uptime=%d, wizard4_1_1_state=%d wizard4_1_1_EndTime=%d\n",
		__FILE__, __FUNCTION__, __LINE__, info.uptime, wizard4_1_1_state, wizard4_1_1_EndTime);

	if ((wizard4_1_1_EndTime - info.uptime)  > 0) {
		wizard4_1_1_remainTime = wizard4_1_1_EndTime - info.uptime;
	}
	else {
		wizard4_1_1_remainTime = -1;
	}

	/*
	if (wizard4_1_1_state == WIZARD_STATE_OMCI_CONFIG) {
	else if (wizard4_1_1_state == WIZARD_STATE_PLOAM_SET)
	else if (wizard4_1_1_state == WIZARD_STATE_ACS_CONFIG)
	*/
	fprintf(stderr, "\033[1;31m[%s:%s@%d]\033[0m wizard4_1_1_EndTime= %d wizard4_1_1_remainTime=%d\n", __FILE__, __FUNCTION__, __LINE__, wizard4_1_1_EndTime, wizard4_1_1_remainTime);

	return 0;
}

int updateWizardState(void)
{
	int idx = WEB_DEFAULT_URL;
	struct sysinfo info;
	sysinfo(&info);

	if (isConfigurationOmciSuccess()) {
		if (getOmciSetting()) {
			wizard4_1_1_state = WIZARD_STATE_ACS_CONFIG;

			/* Initial of ACS timer, it's should be count from 60 to 0 */
			if (acs_connection_timer_flag){
				wizard4_1_1_EndTime = info.uptime + WIZARD_4_1_1_TIME;
				acs_connection_timer_flag=0;
			}
		}
		else {
				into_PLOAM = 1;
				wizard4_1_1_state = WIZARD_STATE_PLOAM_SET;
		}
	}
	else {
		if ( (wizard4_1_1_EndTime - info.uptime) > 0) {
			fprintf(stderr, "\033[1;31m[%s:%s@%d]\033[0m wizard4_1_1_EndTime= %d wizard4_1_1_remainTime=%d\n", __FILE__, __FUNCTION__, __LINE__, wizard4_1_1_EndTime, wizard4_1_1_remainTime);
			wizard4_1_1_state = WIZARD_STATE_OMCI_CONFIG;
		}
		else {
			if (getOmciSetting()) {
				wizard4_1_1_state = WIZARD_STATE_ACS_CONFIG;

				/* Initial of ACS timer, it's should be count from 60 to 0 */
				if (acs_connection_timer_flag){
					wizard4_1_1_EndTime = info.uptime + WIZARD_4_1_1_TIME;
					fprintf(stderr, "\033[1;31m[%s:%s@%d]\033[0m update wizard4_1_1_EndTime= %d wizard4_1_1_remainTime=%d\n", __FILE__, __FUNCTION__, __LINE__, wizard4_1_1_EndTime, wizard4_1_1_remainTime);
					acs_connection_timer_flag=0;
				}
			}
			else {
				into_PLOAM = 1;
				wizard4_1_1_state = WIZARD_STATE_PLOAM_SET;
			}
		}
	}

	updateWizard4_1_1_Timer();
	// check current wizard page
	if (wizard4_1_1_state == WIZARD_STATE_OMCI_CONFIG) {
		if (isConfigurationOmciSuccess()) {
			idx = getNextUrl(WIZARD_SETUP_OMCI_SETTING);
		}
		else {
			idx = WIZARD_URL_4_1_1;
		}
	}
	else if (wizard4_1_1_state == WIZARD_STATE_ACS_CONFIG) {
		if (isACSConnectionSuccess()) {
			idx = getNextUrl(WIZARD_SETUP_ACCESS_ACS);
		}
		else {
			idx = WIZARD_URL_4_1_1;
		}
	}
	else if (wizard4_1_1_state == WIZARD_STATE_PLOAM_SET) {
		if (isPloamCodeSetSuccess()) {
			idx = getNextUrl(WIZARD_SETUP_OMCI_SETTING);
		}
		else {
			idx = WIZARD_URL_4_1_1;
		}
	}

	if (idx == WIZARD_URL_4_1_1) {
		updateWizard4_1_1_Timer();
	}

	return idx;
}

int getInitUrl(int eid, request * wp, int argc, char **argv)
{
	int idx;
	int nBytesSent = 0;

	//wizard1_1_to_web = 0; //should not re-init here, refresh web page will set to 0 again, IulianWu
	//wizard4_1_1_state = WIZARD_STATE_OMCI_CONFIG;
	wizard5_1_times = 0;
	wizard5_1_try_num = SETUP_WIZARD_TRY_NUM;
	wizard5_type = SETUP_WIZARD_TYPE;
	wizard4_1_1_remainTime = WIZARD_4_1_1_TIME;

	idx = getNextUrl(WIZARD_SETUP_SIGNAL_1490);

	if (idx == WIZARD_URL_4_1_1) {
		idx = updateWizardState();
	}

	/* if signal 1490 was receieved , if user didn't click the web, it should be show the page of results after 180 seconds */
	signal_to_1490_to_end_timer = getSignal1490_end_timer() + 3*WIZARD_4_1_1_TIME;

	if(idx >= 0)
	{
		nBytesSent = boaWrite(wp, "\"%s\"", wizardUrl[idx]);
	}
	return nBytesSent;
}

void formWizardScreen0_1(request * wp, char *path, char *query)
{
	int idx;
	char * strData;
	struct sysinfo info;
	sysinfo(&info);

	strData = boaGetVar(wp,"setmanually","");
	if ( strData[0] )
	{
		idx = WIZARD_URL_1_1;
		wizard1_1_to_web = 1;
		boaRedirect(wp,  wizardUrl[idx]);
		return;
	}

	strData = boaGetVar(wp,"ploamcode","");
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
		rt_gpon_deactivate();
#else
		rtk_gpon_deActivate();
#endif
		sprintf(oamcli_cmd , "/sbin/diag gpon set password-hex %s", password_hex);
		system(oamcli_cmd);

		mib_set(MIB_GPON_PLOAM_PASSWD, strData);
		mib_set(MIB_GPON_PLOAM_PASSWD_BACKUP, strData);

		//Active GPON again
		printf("GPON Activate again.\n");
#ifdef CONFIG_COMMON_RT_API
		rt_gpon_activate(RT_GPON_ONU_INIT_STATE_O1);
#else
		rtk_gpon_activate(RTK_GPONMAC_INIT_STATE_O1);
#endif
	}

	/* Initail the PLOAM timer for 60 seconds, it should be count from 60 to 0  */
	if (invitation_to_ploam_timer_flag){
		wizard4_1_1_EndTime = info.uptime + WIZARD_4_1_1_TIME;
		fprintf(stderr, "\033[1;31m[%s:%s@%d]\033[0m update wizard4_1_1_EndTime= %d wizard4_1_1_remainTime=%d\n", __FILE__, __FUNCTION__, __LINE__, wizard4_1_1_EndTime, wizard4_1_1_remainTime);
		invitation_to_ploam_timer_flag = 0;
		wizard4_1_1_state == WIZARD_STATE_PLOAM_SET;
		updateWizard4_1_1_Timer();
	}

	idx = WIZARD_URL_4_1_1;
	boaRedirect(wp,  wizardUrl[idx]);
}

int initWizardScreen1_1(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
	unsigned int selected = 0;

	mib_get_s(MIB_NTP_TIMEZONE_DB_INDEX, &selected, sizeof(selected));
	if(wizard1_1_to_web){
#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
		// webwizard_flag from 1 to 0, update redirect rule depend on optical link
		update_redirect_by_ponStatus();
#endif
	}

	nBytesSent += boaWrite(wp, "to_web = %d;", wizard1_1_to_web);
	nBytesSent += boaWrite(wp, "time_select_val = '%u';", selected);

	return nBytesSent;
}

void formWizardScreen1_1(request * wp, char *path, char *query)
{
	char  *tmpStr, *strPassword, *strConfPassword;
	char tmpBuf[100];
	unsigned char ntpEnabled = 0;
	unsigned char webwizard_flag = 0;

	tmpStr = boaGetVar(wp, "timeZone", "");
	if (tmpStr[0]) {
		FILE *fp;
		unsigned int index;
		unsigned char dst_enabled = 1;

		if ( mib_get_s( MIB_NTP_ENABLED, (void *)&ntpEnabled, sizeof(ntpEnabled)) == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tget_mib_error,sizeof(tmpBuf)-1);
			goto setErr;
		}

		index = strtoul(tmpStr, NULL, 0);

		if (mib_set(MIB_NTP_TIMEZONE_DB_INDEX, &index) == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr;
		}

		if (mib_get_s(MIB_DST_ENABLED, &dst_enabled, sizeof(dst_enabled)) == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tget_mib_error,sizeof(tmpBuf)-1);
			goto setErr;
		}

		if ((fp = fopen("/etc/TZ", "w")) != NULL) {
			fprintf(fp, "%s\n", get_tz_string(index, dst_enabled));
			fclose(fp);
		}
	}

	strPassword = boaGetVar(wp, "newpass", "");
	strConfPassword = boaGetVar(wp, "confirmpass", "");

	if ( !strPassword[0] ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_EMPTY_NEW_PASSWORD,sizeof(tmpBuf)-1);
		goto setErr;
	}

	if ( !strConfPassword[0] ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_EMPTY_CONFIRMED_PASSWORD,sizeof(tmpBuf)-1);
		goto setErr;
	}

	if (strcmp(strPassword, strConfPassword) != 0 ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_UNMATCHED_PASSWORD,sizeof(tmpBuf)-1);
		goto setErr;
	}

	if (!mib_set(MIB_USER_PASSWORD, (void *)strPassword)) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, WARNING_SET_PASSWORD,sizeof(tmpBuf)-1);
		goto setErr;
	}

#if defined(CONFIG_USER_DNSMASQ_DNSMASQ)
	restart_dnsrelay_ex("all", 0);
	sleep(1);
#endif
#if defined(APPLY_CHANGE)
	if (ntpEnabled) {
		stopNTP();
		startNTP();
	} else {
		stopNTP();
	}
#endif

#ifdef EMBED
	// Added by Mason Yu for take effect on real time
	mib_get_s(MIB_SUSER_NAME, (void *)suName, sizeof(suName));
	mib_get_s(MIB_USER_NAME, (void *)usName, sizeof(usName));
	rtk_util_update_boa_user_account();
	rtk_util_update_user_account();	// Jenny
#endif
	//Or to webUI If arrived 4.2 or 5.2 screen, so MIB_USER_WEB_WIZARD_FLAG
	//Should be disable
	if(wizard1_1_to_web) {
#ifdef USER_WEB_WIZARD
		mib_set(MIB_USER_WEB_WIZARD_FLAG, (void *)&webwizard_flag);
#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
		update_redirect_by_ponStatus();
#endif
#endif
	}
// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
	wizard_first_time = 0;
	wizard1_1_state = 1;

	if(wizard1_1_to_web){
		free_from_login_list(wp);
		boaWrite(wp, backHomehtml);

	}else
		boaRedirect(wp,  wizardUrl[WIZARD_URL_1_2]);

	return;

setErr:
	ERR_MSG(tmpBuf);
}

int initWizardScreen4_1(int eid, request * wp, int argc, char **argv)
{
	if(connectOnceUp == 0)
		connectOnceUp = 1;

	/* FIXME: moving to screen4.1 page means that we finish setup wizard */
#ifdef USER_WEB_WIZARD
	unsigned char webwizard_flag = 0;
	mib_set(MIB_USER_WEB_WIZARD_FLAG, (void *)&webwizard_flag);
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
	// webwizard_flag from 1 to 0, update redirect rule depend on optical link
	update_redirect_by_ponStatus();
#endif
#endif
}

int initWizardScreen4_2(int eid, request * wp, int argc, char **argv)
{
	wizard1_1_to_web = 1;
}

int postWizardScreen4_2(int eid, request * wp, int argc, char **argv)
{
#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
	// webwizard_flag from 1 to 0, update redirect rule depend on optical link
	update_redirect_by_ponStatus();
#endif
	free_from_login_list(wp);
}
void formWizardScreen5_1(request * wp, char *path, char *query)
{
	int idx;
	char * strData;
	unsigned char redirectTroubleWizard=0;

	strData = boaGetVar(wp,"continue","");

	if ( strData[0] )
	{
		/* TODO: in trouble wizard do check: requirement is not clear */
		if(++wizard5_1_times > wizard5_1_try_num)
		{
			//Go to 5_2, free the trouble wizard redirect status.
			printf("[%s:%d] set RS_WEB_WIZARD_REDIRECTTROUBLEWIZARD to %d\n",__func__,__LINE__,redirectTroubleWizard);
			redirectTroubleWizard=0;
#ifdef USER_WEB_WIZARD
			mib_set(RS_WEB_WIZARD_REDIRECTTROUBLEWIZARD, &redirectTroubleWizard);
#endif
			idx = WIZARD_URL_5_2;
			wizard5_1_times = 0;
		}
		else
		{
			if (isInternetConnectionUp() && isSignalOn1490()) { //PON connected and ever conection to internet
				idx = WIZARD_URL_4_1_2;
				free_from_login_list(wp);
			}
			else
				idx = getNextUrl(WIZARD_SETUP_SIGNAL_1490);
		}
	}
	else
	{
		if((wizard5_type == SETUP_WIZARD_TYPE) && wizard_first_time)
		{
			idx = WIZARD_URL_1_1;
			//wizard_first_time = 0;
			wizard1_1_to_web=1;
		}
		else
		{
			//Go to Manual Setup, free the trouble wizard redirect status.
			printf("[%s:%d] set RS_WEB_WIZARD_REDIRECTTROUBLEWIZARD to %d\n",__func__,__LINE__,redirectTroubleWizard);
			redirectTroubleWizard=0;
#ifdef USER_WEB_WIZARD
			mib_set(RS_WEB_WIZARD_REDIRECTTROUBLEWIZARD, &redirectTroubleWizard);
#endif
			free_from_login_list(wp);
			boaWrite(wp, backhtml);
			return;
		}
	}

	boaRedirect(wp, wizardUrl[idx]);
}

/*covert value of power Watt to value of dbm*/
static void power_to_dbm(rtk_transceiver_data_t *pSrcData, char * buf)
{
    double tmp = 0;

	if(pSrcData->buf[0] == 0 && pSrcData->buf[1] == 0)
	{
		sprintf(buf, "-inf");
	}
	else
	{
   		tmp = __log10(((double)((pSrcData->buf[0] << 8) | pSrcData->buf[1])*1/10000))*10;
		sprintf(buf, "%f", tmp);
	}
}

static int isPPPoeAuthFail()
{
	int ret = 0;
	FILE *fp;
	char buf[128];
	int error;

	if ((fp = fopen("/tmp/ppp_auth_log", "r")) != NULL) {
		if(fgets(buf, 128, fp) != NULL){
			sscanf(buf, "%*d:%d", &error);
			printf("ppp_auth_log:%s\n", buf);
			if(error == 3)
				ret = 1;
		}
		fclose(fp);
	}
	return ret;
}

static void getServiceInfo(char * buf, int size)
{
	int idx = SERVICE_INFO_NO_WAN_CONNECT;
	int i, entryNum;
	MIB_CE_ATM_VC_T tEntry;
	struct in_addr inAddr;
	int flags;
	char ifname[IFNAMSIZ];

	if(isSignalOn1490() == 0)
	{
		idx = SERVICE_INFO_NO_WAN_CONNECT;
	}
	else if(getOmciSetting() == 0)
	{
		idx = SERVICE_INFO_NOT_REGISTER;
	}
	else
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
				continue;

			ifGetName(tEntry.ifIndex, ifname, IFNAMSIZ);

			if(tEntry.cmode == CHANNEL_MODE_PPPOE)
			{
				if (getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 0)
				{
					if(isPPPoeAuthFail())
						idx = SERVICE_INFO_WRONG_PPPOE_PWD;
					else
						idx = SERVICE_INFO_NO_PPPOE_CONNECT;

					break;
				}
				else {
					idx = SERVICE_INFO_CONNECTED;
				}
			}
			else if (tEntry.cmode == CHANNEL_MODE_IPOE)
			{
				if (getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 0)
				{
					idx = SERVICE_INFO_NO_IPOE_CONNECT;
				}
				else {
					idx = SERVICE_INFO_CONNECTED;
				}
			}
		}
	}

	snprintf(buf, size, "%s", multilang(LANG_SERVICE_INFO0+idx));
}

int initWizardScreen5_2(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
	char tmpBuf[128];
	int i, entryNum;
	MIB_CE_ATM_VC_T tEntry;
	rtk_transceiver_data_t dataCfg, readableCfg;
	char manufacturer[64];
	char product[64];
	FILE *fp;
	char newline[256];
	int version = 0;

	wizard1_1_to_web = 1;

	mib_get_s(MIB_HW_HWVER, (void *)tmpBuf, sizeof(tmpBuf));
	nBytesSent += boaWrite(wp, "devVer = '%s';", tmpBuf);

	fp = popen("nv getenv sw_commit", "r");
	if(fp)
	{
		if(fgets(newline, 256, fp) != NULL){
			sscanf(newline, "%*[^=]=%d", &version);
			//printf("sw_commit %d\n", version);
		}
		pclose(fp);
	}

	sprintf(tmpBuf, "nv getenv sw_version%d",version);
	fp = popen(tmpBuf, "r");
	if(fp)
	{
		if(fgets(newline, 256, fp) != NULL){
			sscanf(newline, "%*[^=]=%s", tmpBuf);
			//printf("sw version %d %s\n", version, tmpBuf);
		}
		pclose(fp);
	}
	nBytesSent += boaWrite(wp, "swVer = '%s';", tmpBuf);

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
			continue;

		if(tEntry.cmode == CHANNEL_MODE_PPPOE)
		{
			sprintf(tmpBuf,"%02X%02X%02X%02X%02X%02X",tEntry.MacAddr[0],tEntry.MacAddr[1],
				    tEntry.MacAddr[2],tEntry.MacAddr[3],tEntry.MacAddr[4],tEntry.MacAddr[5]);

			nBytesSent += boaWrite(wp, "macPppoe = '%s';", tmpBuf);
		}
	}

	if(rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_RX_POWER, &dataCfg) == RT_ERR_OK)
	{
		_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_RX_POWER, &dataCfg, &readableCfg);

		if(strstr(readableCfg.buf, "-inf"))
			nBytesSent += boaWrite(wp, "power = '%s';", multilang(LANG_NO_SIGNAL));
		else
			nBytesSent += boaWrite(wp, "power = '%s';", readableCfg.buf);
	}

	mib_get_s(MIB_HW_CWMP_PRODUCTCLASS, (void *)product, sizeof(product));
	nBytesSent += boaWrite(wp, "devModel = '%s';", product);

	/* get Service information*/
	getServiceInfo(tmpBuf, sizeof(tmpBuf));
	nBytesSent += boaWrite(wp, "servInfo = '%s';", tmpBuf);

	/* GPON identification number means serial number */
	// Use hex number to display first 4 bytes of serial number.
	char sn[64] = {0};
	mib_get_s(MIB_GPON_SN, (void *)sn, sizeof(sn));
	sprintf(tmpBuf, "%02X%02X%02X%02X%s", sn[0], sn[1], sn[2], sn[3], &sn[4]);
	nBytesSent += boaWrite(wp, "gponSn = '%s';", tmpBuf);

	if(getOmciSetting())
		nBytesSent += boaWrite(wp, "omciStatus = '%s';",multilang(LANG_WIZARD_REGISTERED));
	else
		nBytesSent += boaWrite(wp, "omciStatus = '%s';",multilang(LANG_WIZARD_UNREGISTERED));

	return nBytesSent;
}

void formWizardScreen5_2(request * wp, char *path, char *query)
{
	int idx;

	if((wizard5_type == SETUP_WIZARD_TYPE) && wizard_first_time)
	{
		idx = WIZARD_URL_1_1;
		//wizard_first_time = 0;
	}
	else
	{
		free_from_login_list(wp);
		boaWrite(wp, backhtml);
		return;
	}

	boaRedirect(wp, wizardUrl[idx]);
}

int initWizardScreen4_1_1(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;

	nBytesSent += boaWrite(wp, "interval = %d;", WIZARD_4_1_1_INTERVAL);
	nBytesSent += boaWrite(wp, "remainTime = %d;", wizard4_1_1_remainTime);

	return nBytesSent;
}

void formWizardScreen4_1_1(request * wp, char *path, char *query)
{
	int idx = WEB_DEFAULT_URL;

	if(wizard4_1_1_state == WIZARD_STATE_OMCI_CONFIG)
	{
		if(wizard4_1_1_remainTime <= 0)
		{
		 	if(isConfigurationOmciSuccess())
				idx = getNextUrl(WIZARD_SETUP_OMCI_SETTING);
			else
			idx = getNextUrl(WIZARD_SETUP_PLOAM_ENTER);
		}
		else if(isConfigurationOmciSuccess())
		{
			//idx = getNextUrl(WIZARD_SETUP_OMCI_SETTING);
			idx = updateWizardState();
		}
		else
		{
			idx = WIZARD_URL_4_1_1;
			printf("Enter screen4.1.1:check omci configuration\n");
			/* TODO: do Configuration Omci again */
		}
	}
	else if(wizard4_1_1_state == WIZARD_STATE_ACS_CONFIG)
	{
		if(wizard4_1_1_remainTime <= 0)
		{
			if (isACSConnectionSuccess())
			idx = getNextUrl(WIZARD_SETUP_ACCESS_ACS);
			else
				idx = getNextUrl(WIZARD_SETUP_ONT_LOAD);
		}
		else
		{
			idx = WIZARD_URL_4_1_1;
			printf("Enter screen4.1.1:check ACS configuration\n");
			/* TODO: do ACS configuration again */
		}
	}
	else if(wizard4_1_1_state == WIZARD_STATE_PLOAM_SET)
	{
		if(wizard4_1_1_remainTime <= 0)
		{
			idx = getNextUrl(WIZARD_SETUP_OMCI_SETTING);
		}
		else if(isPloamCodeSetSuccess())
		{
			idx = getNextUrl(WIZARD_SETUP_OMCI_SETTING);
		}
		else
		{
			/* Wait PloamCode set succefully */
			printf("Enter screen0.1:check Ploam code set\n");
			idx = WIZARD_URL_0_1;
		}
	}

	if (idx == WIZARD_URL_4_1_1) {
		updateWizard4_1_1_Timer();
	}

	boaRedirect(wp,  wizardUrl[idx]);
}

void formWizardScreen1_2(request * wp, char *path, char *query)
{
	int idx;
	char * strData;

	strData = boaGetVar(wp,"continue","");

	if ( strData[0] )
	{
		if(isInternetConnectionUp())
		{
			idx = WIZARD_URL_4_1;
		}
		else {
			idx = WIZARD_URL_4_2;
		}
	}
	else
	{
		/* back to web and clear flag */
		unsigned char webwizard_flag = 0;
#ifdef USER_WEB_WIZARD
		mib_set(MIB_USER_WEB_WIZARD_FLAG, (void *)&webwizard_flag);
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
		// webwizard_flag from 1 to 0, update redirect rule depend on optical link
		update_redirect_by_ponStatus();
#endif
#endif

		boaWrite(wp, backhtml);
		return;
	}

	boaRedirect(wp,  wizardUrl[idx]);
}

void formWizardScreen4_2(request * wp, char *path, char *query)
{
	unsigned char webwizard_flag = 0;
	if (wizard1_1_state) {
#ifdef USER_WEB_WIZARD
		mib_set(MIB_USER_WEB_WIZARD_FLAG, (void *)&webwizard_flag);
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
		// webwizard_flag from 1 to 0, update redirect rule depend on optical link
		update_redirect_by_ponStatus();
#endif
#endif

		printf("[%s:%d] redirect to login page\n",__func__,__LINE__);
		boaWrite(wp, backhtml);
	}
	else {
		printf("[%s:%d] redirect to %s\n",__func__,__LINE__,wizardUrl[WIZARD_URL_1_1]);
		boaRedirect(wp,  wizardUrl[WIZARD_URL_1_1]);
	}
}

int updateWizard1_2019_Timer(void)
{
	struct sysinfo info;
	sysinfo(&info);
	fprintf(stderr, "\033[1;31m[%s:%s@%d]\033[0m info.uptime=%d, wizard1_state=%d wizard1_EndTime=%d\n",
		__FILE__, __FUNCTION__, __LINE__, info.uptime, wizard4_1_1_state, wizard4_1_1_EndTime);
	if ((wizard4_1_1_EndTime - info.uptime)  > 0) {
		wizard4_1_1_remainTime = wizard4_1_1_EndTime - info.uptime;
	}
	else {
		wizard4_1_1_remainTime = -1;
	}

	fprintf(stderr, "\033[1;31m[%s:%s@%d]\033[0m wizard1_EndTime= %d wizard1_remainTime=%d\n", __FILE__, __FUNCTION__, __LINE__, wizard4_1_1_EndTime, wizard4_1_1_remainTime);
	return 0;
}

static int getNextUrl_2019(int step)
{
	int urlIdx = WEB_DEFAULT_URL;
	int finish = 0;
	int stepNum=0;
	struct sysinfo info;
	sysinfo(&info);
	long endTime =0;
	unsigned char webwizard_flag = 0, trouble_wizard_flag = 1;
	stepNum = step;

	//For trouble wizard behavior
	unsigned char webwizard_flag2 = 0, trouble_wizard_flag2=0;
	mib_get(MIB_USER_WEB_WIZARD_FLAG, (void *)&webwizard_flag2);
	mib_get(MIB_USER_TROUBLE_WIZARD_INTERNET_STATUS, (void *)&trouble_wizard_flag2);
	if (webwizard_flag2 ==0 && trouble_wizard_flag2 ==1)
	{
		stepNum = WIZARD_STATE_DEFAULT_ROUTE;
		/* Initial of trouble wizard timer, it's should be count from 60 to 0  */
		if (wizard4_1_1_state != WIZARD_STATE_DEFAULT_ROUTE){
			wizard4_1_1_EndTime = info.uptime + WIZARD_4_1_1_TIME;
			wizard4_1_1_remainTime = wizard4_1_1_EndTime - info.uptime;
			fprintf(stderr, "\033[1;31m[%s:%s@%d]\033[0m update wizard1_EndTime= %d \n", __FILE__, __FUNCTION__, __LINE__, wizard4_1_1_EndTime );
		}
		wizard4_1_1_state = WIZARD_STATE_DEFAULT_ROUTE;
	}

	while(finish == 0)
	{
		switch(stepNum)
		{
			case WIZARD_STATE_1490_SIGNAL:
				if(isSignalOn1490())
				{
					stepNum = WIZARD_STATE_OMCI_CONFIG;
					wizard4_1_1_state = WIZARD_STATE_OMCI_CONFIG;
					/* Initail the 1490 timer for 60 seconds */
					if (signal_to_1490_timer_flag){
						endTime = getSignal1490_end_timer();
						wizard4_1_1_EndTime = endTime + WIZARD_4_1_1_TIME;
						wizard4_1_1_remainTime = wizard4_1_1_EndTime - info.uptime;
						signal_to_1490_timer_flag=0;
					}
				}
				else
				{
					urlIdx = WIZARD_URL_2_1_2019;
					finish = 1;
				}
				break;
			case WIZARD_STATE_OMCI_CONFIG:
				if(getOmciSetting())
				{
					stepNum = WIZARD_STATE_ACS_CONFIG;
					wizard4_1_1_state = WIZARD_STATE_ACS_CONFIG;
					/* Initial of ACS timer, it's should be count from 60 to 0 */
					if (acs_connection_timer_flag){
						wizard4_1_1_EndTime = info.uptime + WIZARD_4_1_1_TIME;
						wizard4_1_1_remainTime = wizard4_1_1_EndTime - info.uptime;
						fprintf(stderr, "\033[1;31m[%s:%s@%d]\033[0m update wizard1_EndTime= %d \n", __FILE__, __FUNCTION__, __LINE__, wizard4_1_1_EndTime);
						acs_connection_timer_flag=0;
					}
				}
				else
				{
					if(isPloamEntered())
					{
						if(wizard4_1_1_remainTime > 0)
						{
							urlIdx = WIZARD_URL_1_2019;
							finish = 1;
						}
						else
						{
							urlIdx = WIZARD_URL_2_2_2019;
							finish = 1;
						}
					}
					else
					{
						if (wizard4_1_1_remainTime > 0 )
						{
							urlIdx = WIZARD_URL_1_2019;
							finish = 1;
						}
						else
						{
							stepNum = WIZARD_STATE_PLOAM_SET;
							wizard4_1_1_state = WIZARD_STATE_PLOAM_SET;
						}
					}
				}
				break;
			case WIZARD_STATE_ACS_CONFIG:
				if (!getAccessToACS())
				{
					if (info.uptime > ( getSignal1490_end_timer() + 3*WIZARD_4_1_1_TIME))
						urlIdx = WIZARD_URL_2_2_2019;
					else if (wizard4_1_1_remainTime > 0 )
						urlIdx = WIZARD_URL_1_2019;
					else
						urlIdx = WIZARD_URL_2_2_2019;
					finish = 1;
				}
				else
				{
					stepNum = WIZARD_STATE_DEFAULT_ROUTE;
					wizard4_1_1_state = WIZARD_STATE_DEFAULT_ROUTE;
					/* Initial of internet default route timer, it's should be count from 60 to 0 */
					if (default_route_connection_timer_flag){
						wizard4_1_1_EndTime = info.uptime + WIZARD_4_1_1_TIME;
						wizard4_1_1_remainTime = wizard4_1_1_EndTime - info.uptime;
						fprintf(stderr, "\033[1;31m[%s:%s@%d]\033[0m update wizard1_EndTime= %d \n", __FILE__, __FUNCTION__, __LINE__, wizard4_1_1_EndTime );
						default_route_connection_timer_flag=0;
					}
				}
				break;
			case WIZARD_STATE_DEFAULT_ROUTE:
				if (isInternetConnectionUp())
				{
					urlIdx = WEB_DEFAULT_RT;
					mib_set(MIB_USER_WEB_WIZARD_FLAG, (void *)&webwizard_flag);
					mib_set(MIB_USER_TROUBLE_WIZARD_INTERNET_STATUS, (void *)&trouble_wizard_flag);
#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
					update_redirect_by_ponStatus();
#endif
#ifdef COMMIT_IMMEDIATELY
					Commit();
#endif
#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
					unlink(WIZARD_IN_PROCESS);
#endif
					finish = 1;
				}
				else
				{
					if(info.uptime >  ( getSignal1490_end_timer() + 4*WIZARD_4_1_1_TIME) )
						urlIdx = WIZARD_URL_2_2_2019;
					else if ( wizard4_1_1_remainTime > 0 )
						urlIdx = WIZARD_URL_1_1_2019;
					else
						urlIdx = WIZARD_URL_2_2_2019;
					finish = 1;
				}
				break;
			case WIZARD_STATE_PLOAM_SET:
				if(isPloamEntered())
				{
					/* Initail the PLOAM timer for 60 seconds, it should be count from 60 to 0  */
					if (invitation_to_ploam_timer_flag){
						wizard4_1_1_EndTime = info.uptime + WIZARD_4_1_1_TIME;
						wizard4_1_1_remainTime = wizard4_1_1_EndTime - info.uptime;
						fprintf(stderr, "\033[1;31m[%s:%s@%d]\033[0m update wizard1_EndTime= %d wizard1_remainTime=%d\n", __FILE__, __FUNCTION__, __LINE__, wizard4_1_1_EndTime, wizard4_1_1_remainTime);
						invitation_to_ploam_timer_flag = 0;
					}
					if ( wizard4_1_1_remainTime > 0 ) {
						stepNum = WIZARD_STATE_OMCI_CONFIG;
						wizard4_1_1_state = WIZARD_STATE_OMCI_CONFIG;
					}
					else
					{
						urlIdx = WIZARD_URL_2_2_2019;
						finish = 1;
					}
				}
				else {
					if(getOmciSetting())
					{
						stepNum = WIZARD_STATE_OMCI_CONFIG;
						wizard4_1_1_state = WIZARD_STATE_OMCI_CONFIG;
					}
					else
					{
						urlIdx = WIZARD_URL_0_1_2019;
						finish = 1;
					}
				}
				break;
			default:
				urlIdx = WEB_DEFAULT_URL;
				finish = 1;
				printf("%s error step[%d]\n", __func__, stepNum);
				break;
		}
	}
	return urlIdx;
}

int getInitUrl_2019(int eid, request * wp, int argc, char **argv)
{
	int idx;
	int nBytesSent = 0;
	char strbuf[128]={0};
	char remote_ip_addr[INET6_ADDRSTRLEN] = {0};
	strcpy(remote_ip_addr, wp->remote_ip_addr);
	rtk_util_get_ipv4addr_from_ipv6addr(remote_ip_addr);

	wizard5_1_try_num = SETUP_WIZARD_TRY_NUM;
	wizard5_type = SETUP_WIZARD_TYPE;
	wizard4_1_1_remainTime = WIZARD_4_1_1_TIME;
#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
	sprintf(strbuf, "echo %s > %s",  remote_ip_addr, WIZARD_IN_PROCESS);
#endif
	va_cmd("/bin/sh", 2, 0, "-c", strbuf);

	idx = getNextUrl_2019(wizard4_1_1_state);
	updateWizard1_2019_Timer();

	/* if signal 1490 was receieved , if user didn't click the web, it should be show the page of results after 180 seconds */
	signal_to_1490_to_end_timer = getSignal1490_end_timer() + 4*WIZARD_4_1_1_TIME;

	if(idx == WEB_DEFAULT_RT) //redirect to url 1-1
		nBytesSent = boaWrite(wp, "\"%s\"", wizardUrl[WIZARD_URL_1_1_2019]);
	else if(idx >= 0)
	{
		nBytesSent = boaWrite(wp, "\"%s\"", wizardUrl[idx]);
	}
	return nBytesSent;
}

int initWizardScreen1_2019(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
	nBytesSent += boaWrite(wp, "interval = %d;", WIZARD_4_1_1_INTERVAL);
	nBytesSent += boaWrite(wp, "remainTime = %d;", wizard4_1_1_remainTime);
	return nBytesSent;
}

int initWizardScreen1_1_2019(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
	nBytesSent += boaWrite(wp, "interval = %d;", WIZARD_4_1_1_INTERVAL);
	nBytesSent += boaWrite(wp, "remainTime = %d;", wizard4_1_1_remainTime);
	return nBytesSent;
}

int initWizardScreen2_2_2019(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
	char tmpBuf[128];
	int i, entryNum, retry = 0;
	MIB_CE_ATM_VC_T tEntry;
	rtk_transceiver_data_t dataCfg, readableCfg;
	char manufacturer[64];
	char product[64];
	FILE *fp;
	char newline[256];
	int version = 0;

	wizard1_1_to_web = 1;

	mib_get_s(MIB_HW_HWVER, (void *)tmpBuf, sizeof(tmpBuf));
	nBytesSent += boaWrite(wp, "devVer = '%s';", tmpBuf);

	fp = popen("nv getenv sw_commit", "r");
	retry = 0;
	while (fp == NULL && retry < 5)
	{
		usleep(100000); // 100ms
		fp = popen("nv getenv sw_commit", "r");
		retry++;
	}
	if(fp)
	{
		if(fgets(newline, 256, fp) != NULL){
			sscanf(newline, "%*[^=]=%d", &version);
			//printf("sw_commit %d\n", version);
		}
		pclose(fp);
	}

	sprintf(tmpBuf, "nv getenv sw_version%d",version);
	fp = popen(tmpBuf, "r");
	retry = 0;
	while (fp == NULL && retry < 5)
	{
		usleep(100000); // 100ms
		fp = popen(tmpBuf, "r");
		retry++;
	}
	if(fp)
	{
		if(fgets(newline, 256, fp) != NULL){
			sscanf(newline, "%*[^=]=%s", tmpBuf);
			//printf("sw version %d %s\n", version, tmpBuf);
		}
		pclose(fp);
	}
	else
	{
		memset(tmpBuf, 0, sizeof(tmpBuf));
	}
	nBytesSent += boaWrite(wp, "swVer = '%s';", tmpBuf);

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
			continue;

		if(tEntry.cmode == CHANNEL_MODE_PPPOE)
		{
			sprintf(tmpBuf,"%02X%02X%02X%02X%02X%02X",tEntry.MacAddr[0],tEntry.MacAddr[1],
				    tEntry.MacAddr[2],tEntry.MacAddr[3],tEntry.MacAddr[4],tEntry.MacAddr[5]);

			nBytesSent += boaWrite(wp, "macPppoe = '%s';", tmpBuf);
		}
	}

	if(rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_RX_POWER, &dataCfg) == RT_ERR_OK)
	{
		_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_RX_POWER, &dataCfg, &readableCfg);


		if(strstr(readableCfg.buf, "-inf"))
			nBytesSent += boaWrite(wp, "power = '%s';", multilang(LANG_NO_SIGNAL));
		else
			nBytesSent += boaWrite(wp, "power = '%s';", readableCfg.buf);
	}

	mib_get_s(MIB_HW_CWMP_PRODUCTCLASS, (void *)product, sizeof(product));
	nBytesSent += boaWrite(wp, "devModel = '%s';", product);

	/* get Service information*/
	getServiceInfo(tmpBuf, sizeof(tmpBuf));
	nBytesSent += boaWrite(wp, "servInfo = '%s';", tmpBuf);

	/* GPON identification number means serial number */
	// Use hex number to display first 4 bytes of serial number.
	char sn[64] = {0};
	mib_get_s(MIB_GPON_SN, (void *)sn, sizeof(sn));
	sprintf(tmpBuf, "%02X%02X%02X%02X%s", sn[0], sn[1], sn[2], sn[3], &sn[4]);
	nBytesSent += boaWrite(wp, "gponSn = '%s';", tmpBuf);

	if(getOmciSetting())
		nBytesSent += boaWrite(wp, "omciStatus = '%s';",multilang(LANG_WIZARD_REGISTERED));
	else
		nBytesSent += boaWrite(wp, "omciStatus = '%s';",multilang(LANG_WIZARD_UNREGISTERED));

	return nBytesSent;
}

void formWizardScreen0_1_2019(request * wp, char *path, char *query)
{
	int idx;
	char * strData;

	strData = boaGetVar(wp,"ploamcode","");
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
		rtk_gpon_deActivate();
		sprintf(oamcli_cmd , "/sbin/diag gpon set password-hex %s", password_hex);
		system(oamcli_cmd);

		mib_set(MIB_GPON_PLOAM_PASSWD, strData);
		mib_set(MIB_GPON_PLOAM_PASSWD_BACKUP, strData);

		//Active GPON again
		printf("GPON Activate again.\n");
		rtk_gpon_activate(RTK_GPONMAC_INIT_STATE_O1);
	}

	idx = getNextUrl_2019(wizard4_1_1_state);
	updateWizard1_2019_Timer();
	boaRedirect(wp,  wizardUrl[idx]);
}

void formWizardScreen1_2019(request * wp, char *path, char *query)
{
	int idx = WEB_DEFAULT_URL;
	idx = getNextUrl_2019(wizard4_1_1_state);
	updateWizard1_2019_Timer();
	if(idx == WEB_DEFAULT_RT) {
		free_from_login_list(wp);
		boaWrite(wp, rthomehtml);
		return;
	}
	boaRedirect(wp,  wizardUrl[idx]);
}

void formWizardScreen1_1_2019(request * wp, char *path, char *query)
{
	int idx = WEB_DEFAULT_URL;
	idx = getNextUrl_2019(wizard4_1_1_state);

	updateWizard1_2019_Timer();
	if(idx == WEB_DEFAULT_RT) {
		free_from_login_list(wp);
		boaWrite(wp, rthomehtml);
		return;
	}
	boaRedirect(wp,  wizardUrl[idx]);
}


void formWizardScreen2_1_2019(request * wp, char *path, char *query)
{
	int idx=0;
	char * strData;
	strData = boaGetVar(wp,"exit","");
	unsigned char redirectTroubleWizard=2;

	if ( strData[0] )
	{	idx = WIZARD_URL_2_2_2019;
		boaRedirect(wp, wizardUrl[idx]);
		return;
	}

	mib_get_s(RS_WEB_TROUBLE_WIZARD_TRYCOUNT, (void *)&wizard5_1_times, sizeof(wizard5_1_times));
	strData = boaGetVar(wp,"continue","");
	if ( strData[0] )
	{
		if (isSignalOn1490()){
			signal_to_1490_timer_flag = 1;
			wizard5_1_times = 0;
			idx = getNextUrl_2019(WIZARD_STATE_1490_SIGNAL);
		}
		else
		{
			if(++wizard5_1_times > wizard5_1_try_num)
			{
				//Go to 2_2, free the trouble wizard redirect status.
				idx = WIZARD_URL_2_2_2019;
				wizard5_1_times = 0;
				mib_set(RS_WEB_WIZARD_REDIRECTTROUBLEWIZARD, &redirectTroubleWizard);
			}
			else
			{
				idx = WIZARD_URL_2_1_2019;
			}
		}
	}
	else
	{
		wizard5_1_times = 0;
		mib_set(RS_WEB_TROUBLE_WIZARD_TRYCOUNT, &wizard5_1_times);
		free_from_login_list(wp);
		boaWrite(wp, backhtml);
		return;
	}
	printf("[%s:%d] wizard5_1_times=%d wizard5_1_try_num=%d redirect to idx=%d\n",
		__FUNCTION__, __LINE__, wizard5_1_times, wizard5_1_try_num, idx);
	mib_set(RS_WEB_TROUBLE_WIZARD_TRYCOUNT, &wizard5_1_times);
	if(idx == WEB_DEFAULT_RT) {
		boaWrite(wp, rthomehtml);
		return;
	}
	boaRedirect(wp, wizardUrl[idx]);
}

void formWizardScreen2_2_2019(request * wp, char *path, char *query)
{
	unsigned char webwizard_flag = 0;
	mib_set(MIB_USER_WEB_WIZARD_FLAG, (void *)&webwizard_flag);
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
	unlink(WIZARD_IN_PROCESS);
#endif
	free_from_login_list(wp);
	boaWrite(wp, backHomehtml);
	return;
}


int getTroubleInitUrl(int eid, request * wp, int argc, char **argv)
{
	int idx;
	int nBytesSent = 0;

	wizard1_1_to_web = 0;
	wizard5_1_try_num = TROUBLE_WIZARD_TRY_NUM;
	wizard5_type = TROUBLE_WIZARD_TYPE;

	if(isSignalOn1490())
		idx = WIZARD_URL_5_2;
	else
		idx = WIZARD_URL_5_1;

	nBytesSent = boaWrite(wp, "\"%s\"", wizardUrl[idx]);
	return nBytesSent;
}
#endif

#ifdef _PRMT_X_CT_COM_MWBAND_
int initClientLimit(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
	int clientlimit=0;
	mib_get_s(CWMP_CT_MWBAND_MODE, (void *)&clientlimit, sizeof(clientlimit));
	if(clientlimit == 1)
	{
		mib_get_s(CWMP_CT_MWBAND_NUMBER, (void *)&clientlimit, sizeof(clientlimit));
		nBytesSent += boaWrite(wp, "client_limit = %d;", clientlimit);
	}
	return nBytesSent;
}

void formClientLimit(request * wp, char *path, char *query)
{
	char *strVal;
	int clientlimit=0;

	strVal = boaGetVar(wp, "clientlimitenable", "");
	if(strVal[0])
	{
		clientlimit = atoi(strVal);
		mib_set(CWMP_CT_MWBAND_MODE, (void *)&clientlimit);
		strVal = boaGetVar(wp, "clientlimitnum", "");
		if(strVal[0])
		{
			clientlimit = atoi(strVal);
			mib_set(CWMP_CT_MWBAND_NUMBER, (void *)&clientlimit);
		}
	}
	strVal = boaGetVar(wp, "submit-url", "");   // hidden page
	boaRedirect(wp, strVal);
}
#endif

#ifdef USE_DEONET /* sghong, 20230919 */
void formSelfCheck(request * wp, char *path, char *query)
{
	char *strValue, *submitUrl, ret=0;
#ifdef WLAN_SUPPORT
	int i, j;
	char str_tmp[64];
#endif
	strValue = boaGetVar(wp, "reset", "");	//cathy, reset stats
	if(strValue[0]) {
		if(strValue[0] - '0') {	//reset
#ifdef EMBED
		if (access("/proc/realtek/netdev_reset", F_OK) == 0)
		{
			system("echo all > /proc/realtek/netdev_reset");

			system("diag mib reset counter port all");
		}
		/* reset statistics for wifi devices */
#ifdef WLAN_SUPPORT
		for (i=0; i<NUM_WLAN_INTERFACE; i++) {
			/* wifi root */
			sprintf(str_tmp, "/proc/wlan%d/stats", i);
			if (access(str_tmp, F_OK) == 0) {
				sprintf(str_tmp, "echo 0 > /proc/wlan%d/stats", i);
				system(str_tmp);
			}
			/* wifi vap */
#ifdef WLAN_MBSSID
			for (j=0; j<WLAN_MBSSID_NUM; j++)
			{
				sprintf(str_tmp, "/proc/wlan%d-vap%d/stats", i, j);
				if (access(str_tmp, F_OK) == 0) {
					sprintf(str_tmp, "echo 0 > /proc/wlan%d-vap%d/stats", i, j);
					system(str_tmp);
				}
			}
#endif
		}
#endif
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
		rtk_if_statistic_reset();
		ret = rt_stat_init();
#endif
#ifdef WLAN_QTN
		va_cmd("/bin/qcsapi_sockrpc", 4, 1, "reset_all_stats", "wifi0", "0", "0");
#endif
#endif
#ifdef CONFIG_SFU
#ifdef CONFIG_COMMON_RT_API
		ret = rt_stat_init();
#else
		ret = rtk_stat_init();
#endif
		if(ret != 0)
#ifdef CONFIG_COMMON_RT_API
			printf("%s-%d rt_stat_init error %d\n",__func__,__LINE__,ret);
#else
			printf("%s-%d rtk_stat_init error %d\n",__func__,__LINE__,ret);
#endif
#endif
		}
	}

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page

	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
}

int ShowPortStatus(int eid, request * wp, int argc, char **argv)
{
	int ret;
	int portNum;
	char *name;
	char buff[30];
	rtk_enable_t state;
	rt_port_linkStatus_t link;

	memset(buff, 0, sizeof(buff));

	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if (!strcmp(name, "lan1"))
		portNum = 3;
	else if (!strcmp(name, "lan2"))
		portNum = 2;
	else if (!strcmp(name, "lan3"))
		portNum = 1;
	else if (!strcmp(name, "lan4"))
		portNum = 0;

	rt_port_adminEnable_get(portNum, &state);
	rt_port_link_get(portNum, &link);

	if (link == 0)
		sprintf(buff, "DisConnected (%s)", ((state == 0) ? "port disable":"port enable"));
	else
		sprintf(buff, "Connected (%s)", ((state == 0) ? "port disable":"port enable"));

	return boaWrite(wp, "%s", buff);
}

int DaemonCheck(char *DaemonName)
{
	char cmd[40];
	char tmpbuf[20];
	FILE *fp;
	char buf[256] = {0,};
	int ret = -1;

	memset(cmd, 0, sizeof(cmd));

	printf("0\n");
	sprintf(cmd, "ps -aef | grep %s | grep -v grep", DaemonName);
	fp = popen(cmd, "r");
	if (fp == NULL)
		return -1;

	while (fgets(buf, 256, fp) != NULL)
	{
		int nPid;

		if(strstr(buf, DaemonName) !=NULL)
		{
			memset(cmd, 0, sizeof(cmd));
			sscanf(buf, "%s %d ", tmpbuf, &nPid);
			sprintf(cmd, "kill -0 %d", nPid);

			ret = system(cmd);
			if(errno == 0 && errno == 0)
			{
				pclose(fp);
				return 0;
			}
		}
	}

	pclose(fp);

	return ret;
}

int ShowDaemonCheckStatus(int eid, request * wp, int argc, char **argv)
{
	int ret;
	char *name;
	char buff[30];

	memset(buff, 0, sizeof(buff));

	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}

	ret = DaemonCheck(name);
	if (ret == 0)
		strcpy(buff, "Success");
	else
		strcpy(buff, "Fail");

	return boaWrite(wp, "%s", buff);
}


static int getgatewayandiface(in_addr_t * addr)
{
	long destination, gateway;
	char iface[16];
	char buf[BUFFER_SIZE];
	FILE * file;

	memset(iface, 0, sizeof(iface));
	memset(buf, 0, sizeof(buf));

	file = fopen("/proc/net/route", "r");
	if (!file)
		return -1;

	while (fgets(buf, sizeof(buf), file))
	{
		if (sscanf(buf, "%s %lx %lx", iface, &destination, &gateway) == 3)
		{
			if (destination == 0)
			{ /* default */
				*addr = gateway;
				fclose(file);
				return 0;
			}
		}
	}

	/* default route not found */
	if (file)
		fclose(file);
	return -1;
}

static int deo_defaultGW_get(char *gateway)
{
	in_addr_t addr = 0;

	getgatewayandiface(&addr);

	strcpy(gateway, inet_ntoa(*(struct in_addr *) &addr));

	return 0;
}


int PingGatewayCheck()
{
	int ret;
	char cmd[30];
	char defGateway[20]= {'\0', };

	deo_defaultGW_get(defGateway);
	sprintf(cmd, "ping -c 3 %s", defGateway);
	ret = system(cmd);

	return ret;
}

static int dnslookup (char *request)
{
	int ret;
	char cmd[30];

	sprintf(cmd, "ping -c 2 %s", DNS_GOOGLE_SERVER);
	ret = system(cmd);

	return ret;
}

int ShowGatewayDnsCheckStatus(int eid, request * wp, int argc, char **argv)
{
	int ret;
	char *name;
	char buff[30];

	memset(buff, 0, sizeof(buff));

	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if (!strcmp(name, "dnscheck"))
	{
		ret = dnslookup ("dns.google");
		if (ret == 0)
			strcpy(buff, "Success");
		else
			strcpy(buff, "Fail");
	}
	else if (!strcmp(name, "gatewaycheck"))
	{
		ret = PingGatewayCheck();
		if (ret == 0)
			strcpy(buff, "Success");
		else
			strcpy(buff, "Fail");
	}

	return boaWrite(wp, "%s", buff);
}

int pwmacinit(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
	char mac_str[14];
	char tmppw[20];
	int mode;

	mode = deo_wan_nat_mode_get();
	memset(mac_str, 0, sizeof(mac_str));
	memset(tmppw, 0, sizeof(tmppw));

	if (mode == DEO_NAT_MODE)
		get_mac_from_interface("nas0_0", mac_str);
	else
		get_mac_from_interface("br0", mac_str);

	sprintf(tmppw, "%s_admin", mac_str);
	nBytesSent += boaWrite(wp,"comparebase = '%s';", tmppw);

	return nBytesSent;
}
#endif

#ifdef CONFIG_USB_SUPPORT
/// This setting should sync with usbmount user tool
#define _PTS			", new it(\"%s\", \"%s\")"

int listUsbDevices(int eid, request *wp, int argc, char ** argv)
{
	int errcode = 1, lineno = __LINE__;
	struct dirent **namelist;
	int i, n;

	n = scandir("/mnt", &namelist, usb_filter, alphasort);

	/* no match */
	if (n < 0)
		goto check_err;

	for (i = 0; i < n; i++) {
		boaWrite(wp, "push(new it_nr(\"%c\"" _PTS "));\n",
				namelist[i]->d_name[3],
				"path", namelist[i]->d_name);
		free(namelist[i]);
	}
	free(namelist);

check_err:
	return 0;
}
#endif

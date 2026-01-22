#include "webform.h"
#include "../asp_page.h"
#include "multilang.h"
#include "sysconfig.h"
#include "utility.h"

void formSamba(request * wp, char *path, char *query)
{
	char *str, *submitUrl;
	unsigned char samba_enable;
	char tmpBuf[64];
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
	unsigned char smb_security_enable;
#endif

	str = boaGetVar(wp, "sambaCap", "");
	if (str[0]) {
		samba_enable = str[0] - '0';
		if (!mib_set(MIB_SAMBA_ENABLE, &samba_enable)) {
			strcpy(tmpBuf, Tset_mib_error);
			goto formSamba_err;
		}
	}

#ifdef CONFIG_USER_NMBD
	str = boaGetVar(wp, "netBIOSName", "");
	if (str[0]) {
		if (!mib_set(MIB_SAMBA_NETBIOS_NAME, str)) {
			strcpy(tmpBuf, Tset_mib_error);
			goto formSamba_err;
		}
	}
#endif

	str = boaGetVar(wp, "serverString", "");
	if (str[0]) {
		if (!mib_set(MIB_SAMBA_SERVER_STRING, str)) {
			strcpy(tmpBuf, Tset_mib_error);
			goto formSamba_err;
		}
	}

#ifdef CONFIG_MULTI_SMBD_ACCOUNT
	str = boaGetVar(wp, "smbSecurityCap", "");
	if (str[0]) {
		smb_security_enable = str[0] - '0';
		
		if(smb_security_enable == 0) smb_security_enable=1;
		else smb_security_enable=0;
		
		if (!mib_set(MIB_SMB_ANNONYMOUS, &smb_security_enable)) {
			strcpy(tmpBuf, Tset_mib_error);
			goto formSamba_err;
		}
	}
#endif
formSamba_end:
#ifdef APPLY_CHANGE
	stopSamba();
	startSamba();
#endif

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG(submitUrl);

	return;

formSamba_err:
	ERR_MSG(tmpBuf);
}

void formSambaAccount(request * wp, char *path, char *query)
{
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
	char	*str, *submitUrl;
	char tmpBuf[100];
	//struct rtentry rt;
	MIB_CE_SMB_ACCOUNT_T entry;
	int xflag, isnet, ret;
	int skfd;
	int intVal;
	unsigned int totalEntry;

	memset( &entry, 0, sizeof(MIB_CE_SMB_ACCOUNT_T));

	// Delete
	str = boaGetVar(wp, "delSamba", "");
	if (str[0]) {
		unsigned int i;
		unsigned int idx;
		MIB_CE_SMB_ACCOUNT_T Entry;
		unsigned int totalEntry = mib_chain_total(MIB_SMBD_ACCOUNT_TBL); /* get chain record size */
		str = boaGetVar(wp, "select", "");

		if (str[0]) {
			for (i=0; i<totalEntry; i++) {
				idx = totalEntry-i-1;

				if ( idx == atoi(str) ) {
					/* get the specified chain record */
					if (!mib_chain_get(MIB_SMBD_ACCOUNT_TBL, idx, (void *)&Entry)) {
						strcpy(tmpBuf, errGetEntry);
						goto setErr_samba;
					}

					rtk_app_del_smbd_account(Entry.username);

					syslog(LOG_INFO, "formSambaAccount: Delete Entry");

					goto setOk_samba;
				}
			} // end of for
		}
		else {
			strcpy(tmpBuf, multilang(LANG_PLEASE_SELECT_AN_ENTRY_TO_DELETE));
			goto setErr_samba;
		}

		goto setOk_samba;
	}

	// parse input
	str = boaGetVar(wp, "username", "");
	if (str[0]){
	    strcpy(entry.username,str);
		printf("username:%s\n", entry.username);
	}

	str = boaGetVar(wp, "password", "");
	if (str[0]){
	    strcpy(entry.password,str);
		printf("password:%s\n", entry.password);
	}

#ifndef CONFIG_SMBD_SHARE_USB_ONLY
	str = boaGetVar(wp, "saveDir", "");
	if (str[0]){
	    sprintf(entry.path,"/mnt/%s",str);
		printf("path:%s\n", entry.path);
	}
	else{
		sprintf(entry.path,"/mnt");
		printf("path:%s\n", entry.path);
	}
#endif

	str = boaGetVar(wp, "writeable", "");
	if ( str && str[0] ) {
		entry.writable=atoi(str);
	}

	// Add
	str = boaGetVar(wp, "addSamba", "");
	if (str && str[0]) {
		intVal = add_smbd_account(entry.username, entry.password, entry.path, entry.writable);
		if (intVal == -1) {
			strcpy(tmpBuf, multilang(LANG_ERROR_USER_ALREADY_EXISTS));
			goto setErr_samba;
		}

		if (intVal == 0) {
			strcpy(tmpBuf, Tadd_chain_error);
			goto setErr_samba;
		}

		syslog(LOG_INFO, "Samba Account: Add Entry");
	}

setOk_samba:
// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;

setErr_samba:
	ERR_MSG(tmpBuf);
#endif
}

#ifdef CONFIG_SMBD_SHARE_USB_ONLY
int showSambaAccount(int eid, request * wp, int argc, char **argv)
{
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
	int nBytesSent=0;

	unsigned int entryNum, i;
	MIB_CE_SMB_ACCOUNT_T Entry;

	entryNum = mib_chain_total(MIB_SMBD_ACCOUNT_TBL);

	nBytesSent += boaWrite(wp, "<tr>"
	"<td align=center width=\"8%%\">%s</td>\n"
	"<td align=center width=\"8%%\">%s</td>\n"
	"<td align=center width=\"8%%\">%s</td>\n"
	"<td align=center width=\"5%%\">%s</td>\n"
	"</tr>\n", multilang(LANG_USERNAME), multilang(LANG_PASSWORD), multilang(LANG_PERMISSIONS), multilang(LANG_DELETE_SELECTED));


	for (i=0; i<entryNum; i++) {

		char sWritable[16];

		if (!mib_chain_get(MIB_SMBD_ACCOUNT_TBL, i, (void *)&Entry))
		{
			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}
		memset(sWritable, 0, sizeof(sWritable));
		sprintf(sWritable, "%s", Entry.writable==1 ? "Write":"read");

		nBytesSent += boaWrite(wp, "<tr>"

		"<td align=center width=\"8%%\"> %s</td>\n"
		"<td align=center width=\"8%%\"> %s</td>\n"
		"<td align=center width=\"8%%\"> %s</td>"
		"<td align=center width=\"5%%\"><input type=\"radio\" name=\"select\""
		" value=\"%d\" "
		"onClick=\"postSamba('%s','%s',%d,'select%d' )\""
		"></td>\n"
		"</tr>\n",
		Entry.username, Entry.password, sWritable,
		i, Entry.username, Entry.password, Entry.writable, i);

	}
#endif

	return 0;
}
#else
int showSambaAccount(int eid, request * wp, int argc, char **argv)
{
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
	int nBytesSent=0;

	unsigned int entryNum, i;
	MIB_CE_SMB_ACCOUNT_T Entry;

	entryNum = mib_chain_total(MIB_SMBD_ACCOUNT_TBL);

	nBytesSent += boaWrite(wp, "<tr>"
	"<td align=center width=\"8%%\">%s</td>\n"
	"<td align=center width=\"8%%\">%s</td>\n"
	"<td align=center width=\"8%%\">%s</td>\n"
	"<td align=center width=\"8%%\">%s</td>\n"
	"<td align=center width=\"5%%\">%s</td>\n"
	"</tr>\n", multilang(LANG_USERNAME), multilang(LANG_PASSWORD), multilang(LANG_PATH), multilang(LANG_PERMISSIONS), multilang(LANG_DELETE_SELECTED));


	for (i=0; i<entryNum; i++) {

		char sWritable[16];

		if (!mib_chain_get(MIB_SMBD_ACCOUNT_TBL, i, (void *)&Entry))
		{
			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}
		memset(sWritable, 0, sizeof(sWritable));
		sprintf(sWritable, "%s", Entry.writable==1 ? "Write":"read");

		nBytesSent += boaWrite(wp, "<tr>"

		"<td align=center width=\"8%%\"> %s</td>\n"
		"<td align=center width=\"8%%\"> %s</td>\n"
		"<td align=center width=\"8%%\"> %s</td>"
		"<td align=center width=\"8%%\"> %s</td>"
		"<td align=center width=\"5%%\"><input type=\"radio\" name=\"select\""
		" value=\"%d\" "
		"onClick=\"postSamba('%s','%s','%s',%d,'select%d' )\""
		"></td>\n"
		"</tr>\n",
		Entry.username, Entry.password, Entry.path, sWritable,
		i, Entry.username, Entry.password, Entry.path, Entry.writable, i);

	}
#endif

	return 0;
}
#endif

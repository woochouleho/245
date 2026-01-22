
/*
 *      Web server handler routines for Domain Blocking stuffs
 *
 */


/*-- System inlcude files --*/
#include <string.h>
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
#include "multilang.h"

#ifdef USE_LOGINWEB_OF_SERVER
extern unsigned char g_login_username[];
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
///////////////////////////////////////////////////////////////////
void formDOMAINBLK(request * wp, char *path, char *query)
{
	char	*str, *submitUrl, *strVal;
	char tmpBuf[100];
	unsigned char domainblkcap;
#ifndef NO_ACTION
	int pid;
#endif

	// Set domain blocking Capability
	str = boaGetVar(wp, "apply", "");
	if (str[0]) {
		str = boaGetVar(wp, "domainblkcap", "");
		if (str[0]) {
			if (str[0] == '0')
				domainblkcap = 0;
			else
				domainblkcap = 1;
			if ( !mib_set(MIB_DOMAINBLK_CAPABILITY, (void *)&domainblkcap)) {
				sprintf(tmpBuf, " %s (Domain Blocking Capability).",Tset_mib_error);
				goto setErr_route;
			}
		}
#ifdef USE_LOGINWEB_OF_SERVER
		int url_enable;
		if ( domainblkcap == 0 )
			url_enable = LANG_DISABLE;
		else
			url_enable = LANG_ENABLE;
		syslog(LOG_INFO, "FW: %s %s Domain Blocking.\n", g_login_username, multilang(url_enable));
#endif
 	}

	// Delete all Domain
	str = boaGetVar(wp, "delAllDomain", "");
	if (str[0]) {
#ifdef USE_LOGINWEB_OF_SERVER
		syslog(LOG_INFO, "FW: %s del All Domain Blocking FQDN rule.\n", g_login_username);
#endif
		mib_chain_clear(MIB_DOMAIN_BLOCKING_TBL); /* clear chain record */
		goto setOk_route;
	}

	/* Delete selected Domain */
	str = boaGetVar(wp, "delDomain", "");
	if (str[0]) {
		unsigned int i;
		unsigned int idx;
		unsigned int totalEntry = mib_chain_total(MIB_DOMAIN_BLOCKING_TBL); /* get chain record size */
		unsigned int deleted = 0;

		for (i=0; i<totalEntry; i++) {

			idx = totalEntry-i-1;
			snprintf(tmpBuf, 20, "select%d", idx);
			strVal = boaGetVar(wp, tmpBuf, "");

			if ( !gstrcmp(strVal, "ON") ) {
				deleted ++;
#ifdef USE_LOGINWEB_OF_SERVER
				MIB_CE_DOMAIN_BLOCKING_T Entry;
				mib_chain_get(MIB_DOMAIN_BLOCKING_TBL, idx, (void *)&Entry);
				syslogDomainBlockEntry(Entry, 0, g_login_username);
#endif
				if(mib_chain_delete(MIB_DOMAIN_BLOCKING_TBL, idx) != 1) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
					goto setErr_route;
				}
			}
		}
		if (deleted <= 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strNoItemSelectedToDelete,sizeof(tmpBuf)-1);
			goto setErr_route;
		}

		goto setOk_route;
	}
#if 0
	// Delete
	str = boaGetVar(wp, "delDomain", "");
	if (str[0]) {
		unsigned int i;
		unsigned int idx;
		MIB_CE_DOMAIN_BLOCKING_T Entry;
		unsigned int totalEntry = mib_chain_total(MIB_DOMAIN_BLOCKING_TBL); /* get chain record size */

		str = boaGetVar(wp, "select", "");

		if (str[0]) {
			for (i=0; i<totalEntry; i++) {
				idx = totalEntry-i-1;
				snprintf(tmpBuf, 4, "s%d", idx);

				if ( !gstrcmp(str, tmpBuf) ) {
					// delete from chain record
					if(mib_chain_delete(MIB_DOMAIN_BLOCKING_TBL, idx) != 1) {
						sprintf(tmpBuf, " %s (Domain Blocking chain)",Tdelete_chain_error);
						goto setErr_route;
					}

				}
			} // end of for
		}

		goto setOk_route;
	}
#endif

	// Add
	str = boaGetVar(wp, "addDomain", "");
	if (str[0]) {
		MIB_CE_DOMAIN_BLOCKING_T entry;
		int i, intVal;
		unsigned int totalEntry = mib_chain_total(MIB_DOMAIN_BLOCKING_TBL); /* get chain record size */

		str = boaGetVar(wp, "blkDomain", "");
		//printf("formDOMAINBLK:(Add) str = %s\n", str);
		// Jenny, check duplicated rule
		for (i = 0; i< totalEntry; i++) {
			if (!mib_chain_get(MIB_DOMAIN_BLOCKING_TBL, i, (void *)&entry)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, errGetEntry,sizeof(tmpBuf)-1);
				goto setErr_route;
			}
			if (!strcmp(entry.domain, str)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strMACInList ,sizeof(tmpBuf)-1);
				goto setErr_route;
			}
		}

		// add into configuration (chain record)
		entry.domain[sizeof(entry.domain)-1]='\0';
		strncpy(entry.domain, str,sizeof(entry.domain)-1);
		
		// Check XSS attack
		if(strContainXSSChar(entry.domain)){
			printf("%s:%d String %s contains invalid words causing XSS attack!\n",__FUNCTION__,__LINE__,entry.domain);
			snprintf(tmpBuf, sizeof(tmpBuf),"invalid string %s (XSS attack)", entry.domain);
			goto setErr_route;
		}

		intVal = mib_chain_add(MIB_DOMAIN_BLOCKING_TBL, (unsigned char*)&entry);
		if (intVal == 0) {
			//boaWrite(wp, "%s", "Error: Add Domain Blocking chain record.");
			//return;
			sprintf(tmpBuf, "%s (Domain Blocking chain)",Tadd_chain_error);
			goto setErr_route;
		}
		else if (intVal == -1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
			goto setErr_route;
		}
#ifdef USE_LOGINWEB_OF_SERVER
		syslogDomainBlockEntry(entry, 1, g_login_username);
#endif

	}

setOk_route:
// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#if defined(APPLY_CHANGE)
	restart_domainBLK();
#endif

#ifndef NO_ACTION
	pid = fork();
	if (pid)
		waitpid(pid, NULL, 0);
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _CONFIG_SCRIPT_PROG);
#ifdef HOME_GATEWAY
		execl( tmpBuf, _CONFIG_SCRIPT_PROG, "gw", "bridge", NULL);
#else
		execl( tmpBuf, _CONFIG_SCRIPT_PROG, "ap", "bridge", NULL);
#endif
		exit(1);
	}
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
 	return;

setErr_route:
	ERR_MSG(tmpBuf);
}

int showDOMAINBLKTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;

	unsigned int entryNum, i;
	MIB_CE_DOMAIN_BLOCKING_T Entry;
	unsigned long int d,g,m;
	unsigned char sdest[MAX_DOMAIN_LENGTH];

	entryNum = mib_chain_total(MIB_DOMAIN_BLOCKING_TBL);

	nBytesSent += boaWrite(wp, "<tr><font size=1>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"35%%\" bgcolor=\"#808080\">%s</td></font></tr>\n",
	multilang(LANG_SELECT), multilang(LANG_DOMAIN));


	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_DOMAIN_BLOCKING_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, "Get Domain Blocking chain record error!\n");
			return -1;
		}

		strncpy(sdest, Entry.domain, sizeof(sdest));
		sdest[strlen(Entry.domain)] = '\0';

		nBytesSent += boaWrite(wp, "<tr>"
//		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"radio\" name=\"select\""
//		" value=\"s%d\"></td>\n"
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>\n"
		"<td align=center width=\"35%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td></tr>\n",
		i, strValToASP(sdest));
	}

	return 0;
}
#endif

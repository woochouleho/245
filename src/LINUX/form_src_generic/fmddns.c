/*
 *      Web server handler routines for ACL stuffs
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

#define	DDNS_ADD	0
#define DDNS_MODIFY	1

#ifdef CONFIG_USER_DDNS
int ddnsServicePort(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
#ifdef CONFIG_USER_DDNS_LIBSSL
	nBytesSent += boaWrite(wp,	"<tr>\n"
									"<th width=\"30%\">%s %s:</th>\n"
									"<td width=\"70%\">\n"
										"<select size=\"1\" name=\"port\">\n"
											"<option selected value=\"80\">HTTP</option>\n"
											"<option value=\"443\">HTTPS</option>\n"
										"</select>\n"
									"</td>\n"
								"</tr>\n", multilang(LANG_PROVIDER), multilang(LANG_SERVICES));
#else
	nBytesSent += boaWrite(wp,	"<input type=\"hidden\" name=\"port\" value=\"\">\n");
#endif
	return nBytesSent;
}

static void getEntry(request * wp, MIB_CE_DDNS_Tp pEntry)
{
	char	*str, *tmp=NULL;;
	unsigned long ddns_if, if_num;	// Mason Yu. Specify IP Address
	char ifname[IFNAMSIZ];
	
	memset(pEntry, 0, sizeof(MIB_CE_DDNS_T));

	str = boaGetVar(wp, "ddnsProv", "");
	if ( str[0]=='0' ) {
		pEntry->provider[sizeof(pEntry->provider)-1]='\0';
		strncpy(pEntry->provider, "dyndns",sizeof(pEntry->provider)-1);
	} else if ( str[0]=='1') {
		pEntry->provider[sizeof(pEntry->provider)-1]='\0';
		strncpy(pEntry->provider, "tzo",sizeof(pEntry->provider)-1);
	} else if ( str[0]=='2') {
		pEntry->provider[sizeof(pEntry->provider)-1]='\0';
		strncpy(pEntry->provider, "noip",sizeof(pEntry->provider)-1);
	} else
		printf("Updatedd not support this provider!!\n");
	//printf("pEntry->provider = %s\n",pEntry->provider);

	str = boaGetVar(wp, "hostname", "");
	if (str[0]) {
		pEntry->hostname[sizeof(pEntry->hostname)-1]='\0';
		strncpy(pEntry->hostname, str,sizeof(pEntry->hostname)-1);
	}
	//printf("pEntry->hostname = %s\n", pEntry->hostname);

	str = boaGetVar(wp, "interface", "");
	if (str[0]) {
		// Mason Yu. Specify IP Address
		ddns_if = (unsigned long)atoi(str);

		if ( ddns_if == 100 ) {
			pEntry->interface[sizeof(pEntry->interface)-1]='\0';
			strncpy(pEntry->interface, BRIF,sizeof(pEntry->interface)-1);
		} else if ( ddns_if == DUMMY_IFINDEX ) {
			pEntry->interface[sizeof(pEntry->interface)-1]='\0';
			strncpy(pEntry->interface, "Any",sizeof(pEntry->interface)-1);
		} else {			
			ifGetName( ddns_if, ifname, sizeof(ifname));
			pEntry->interface[sizeof(pEntry->interface)-1]='\0';
			strncpy(pEntry->interface, ifname,sizeof(pEntry->interface)-1);			
		}

		// Mason Yu. Specify IP Address
		//strcpy(pEntry->interface, str);
		//printf("pEntry->interface= %s\n", pEntry->interface);
	}

	if ( strcmp(pEntry->provider, "dyndns") == 0 || strcmp(pEntry->provider, "noip") == 0 ) {
		str = boaGetVar(wp, "username", "");
		if (str[0]) {
			pEntry->username[sizeof(pEntry->username)-1]='\0';
			strncpy(pEntry->username, str,sizeof(pEntry->username)-1);
		}
		//printf("pEntry->username = %s\n", pEntry->username);

		str = boaGetVar(wp, "password", "");
		if (str[0]) {
			pEntry->password[sizeof(pEntry->password)-1]='\0';
			strncpy(pEntry->password, str,sizeof(pEntry->password)-1);
		}
		//printf("pEntry->password = %s\n", pEntry->password);

		str = boaGetVar(wp, "port", "");
		if(str[0]) {
			pEntry->ServicePort = strtol(str, &tmp, 0);
		}
		else
			pEntry->ServicePort = DDNS_HTTP;

	} else if ( strcmp(pEntry->provider, "tzo") == 0 ) {
		str = boaGetVar(wp, "email", "");
		if (str[0]) {
			//strcpy(pEntry->email, str);
			pEntry->username[sizeof(pEntry->username)-1]='\0';
			strncpy(pEntry->username, str,sizeof(pEntry->username)-1);
		}
		//printf("email = %s\n", pEntry->username);

		str = boaGetVar(wp, "key", "");
		if (str[0]) {
			//strcpy(pEntry->key, str);
			pEntry->password[sizeof(pEntry->password)-1]='\0';
			strncpy(pEntry->password, str,sizeof(pEntry->password)-1);
		}
		//printf("key = %s\n", pEntry->password);

	} else
		printf("Please choose the correct provider!!!\n");

	str = boaGetVar(wp, "enable", "");
	if ( str && str[0] ) {
		pEntry->Enabled = 1;
	}
}

/*
 *	type:
 *		DDNS_ADD(0):	entry to add
 *		DDNS_MODIFY(1):	entry to modify
 *	Return value:
 *	-1	: fail
 *	0	: successful
 *	pMsg	: error message
 */
static int checkEntry(MIB_CE_DDNS_Tp pEntry, int type, char *pMsg, int selected)
{
	MIB_CE_DDNS_T tmpEntry;
	int num, i;

	num = mib_chain_total(MIB_DDNS_TBL); /* get chain record size */
	// check duplication
	for (i=0; i<num; i++) {
		if(!mib_chain_get(MIB_DDNS_TBL, i, (void *)&tmpEntry))
			continue;
		if (type == DDNS_MODIFY) { // modify
			if (selected == i)
				continue;
		}
		// Mason Yu
		//if (gstrcmp(pEntry->provider, tmpEntry.provider))
		//	continue;
		//if (gstrcmp(pEntry->interface, tmpEntry.interface))
		//	continue;
		if (gstrcmp(pEntry->hostname, tmpEntry.hostname))
			continue;
		
		// entry duplication
		strcpy(pMsg, multilang(LANG_ENTRY_ALREADY_EXISTS));
		return -1;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////
void formDDNS(request * wp, char *path, char *query)
{
	char	*str, *submitUrl;
	char tmpBuf[100], ifname[6];
	unsigned int totalEntry, i, idx;
	int selected=0;
	MIB_CE_DDNS_T entry;
//	unsigned char ddns_if, if_num;
#ifndef NO_ACTION
	int pid;
#endif

	// Mason Yu. Support ddns status file.
	remove_ddns_status();
	stop_all_ddns();
	
	totalEntry = mib_chain_total(MIB_DDNS_TBL); /* get chain record size */

	// Remove
	str = boaGetVar(wp, "delacc", "");
	if (str[0]) {
		str = boaGetVar(wp, "select", "");
		if (str[0]) {
			for (i=0; i<totalEntry; i++) {
				idx = totalEntry-i-1;
				snprintf(tmpBuf, 4, "s%d", idx);
				//printf("tmpBuf(select) = %s idx=%d\n", tmpBuf, idx);

				if ( !gstrcmp(str, tmpBuf) ) {
					// delete from chain record
					if(mib_chain_delete(MIB_DDNS_TBL, idx) != 1) {
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
						goto setErr_route;
					}
				}
			} // end of for
		}
		goto setOk_route;
	}

	// Add
	str = boaGetVar(wp, "addacc", "");
	if (str[0]) {
		MIB_CE_DDNS_T tmpEntry;
		int intVal;

		getEntry(wp, &entry);
		if (checkEntry(&entry, DDNS_ADD, &tmpBuf[0], 0xffff) == -1)
			goto setErr_route;

		intVal = mib_chain_add(MIB_DDNS_TBL, (unsigned char*)&entry);
		if (intVal == 0) {
			//boaWrite(wp, "%s", "Error: Add chain(MIB_DDNS_TBL) record.");
			//return;
			sprintf(tmpBuf, "%s (MIB_DDNS_TBL)",Tadd_chain_error);
			goto setErr_route;
		}
		else if (intVal == -1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
			goto setErr_route;
		}

	}

	// Modify
	str = boaGetVar(wp, "modify", "");
	if (str[0]) {
		selected = -1;
		str = boaGetVar(wp, "select", "");
		if (str[0]) {
			for (i=0; i<totalEntry; i++) {
				idx = totalEntry-i-1;
				snprintf(tmpBuf, 4, "s%d", idx);

				if ( !gstrcmp(str, tmpBuf) ) {
					selected = idx;
					break;
				}
			}
			if (selected >= 0) {
				getEntry(wp, &entry);
				if (checkEntry(&entry, DDNS_MODIFY, &tmpBuf[0], selected) == -1)
					goto setErr_route;

				mib_chain_update(MIB_DDNS_TBL, (void *)&entry, selected);
			}
		}
	}

	str = boaGetVar(wp, "update", "");
	if (str[0]) goto restart;

setOk_route:

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

restart:
	restart_ddns();

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

int showDNSTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	unsigned int entryNum, i;
	MIB_CE_DDNS_T Entry;
	unsigned long int d,g,m;
	struct in_addr dest;
	struct in_addr gw;
	struct in_addr mask;
	char sdest[16], sgw[16];
	int status;
	char *statusStr=NULL;
	FILE *fp;
	unsigned char filename[100];
	unsigned int ifIndex;
#ifdef CONFIG_USER_DDNS_LIBSSL
	char *portStr=NULL;
#endif
	
	entryNum = mib_chain_total(MIB_DDNS_TBL);
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr><font size=1>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"30%%\" bgcolor=\"#808080\">%s</td>"
//	"<td align=center width=\"20%%\" bgcolor=\"#808080\">Interface</td>"
	"</font></tr>\n", multilang(LANG_SELECT), multilang(LANG_STATE),
#else
	nBytesSent += boaWrite(wp, "<tr>"
	"<th align=center width=\"5%%\">%s</th>\n"
	"<th align=center width=\"5%%\">%s</th>"
	"<th align=center width=\"20%%\">%s</th>"
	"<th align=center width=\"20%%\">%s</th>"
	"<th align=center width=\"10%%\">%s</th>"
#ifdef CONFIG_USER_DDNS_LIBSSL
	"<th align=center width=\"10%%\">%s</th>"
#endif
	"<th align=center width=\"30%%\">%s</th>"
//	"<td align=center width=\"20%%\">Interface</td>"
	"</tr>\n", multilang(LANG_SELECT), multilang(LANG_STATE),
#endif
	multilang(LANG_HOSTNAME), multilang(LANG_USERNAME), multilang(LANG_SERVICE),
#ifdef CONFIG_USER_DDNS_LIBSSL
	multilang(LANG_PORT),
#endif
	multilang(LANG_STATUS));

	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_DDNS_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

		// open a status file
		status = LINK_DOWN;
		sprintf(filename, "/var/%s.txt", Entry.hostname);
		fp=fopen(filename,"r"); 
		if (fp) {
			if(fscanf(fp,"%d\n",&status) == -1)
				printf("%s-%d fscanf fail!\n",__func__,__LINE__);
			fclose(fp);
		}
		
		if (status == SUCCESSFULLY_UPDATED)
			statusStr = "Successfully updated";
		else if (status == CONNECTION_ERROR)
			statusStr = "Connection error";
		else if (status == AUTH_FAILURE)
			statusStr = "Authentication failure";
		else if (status == WRONG_OPTION)
			statusStr = "Wrong option";			
		else if (status == HANDLING)
			statusStr = "Handling DDNS request packet";
		else if (status == LINK_DOWN)
			statusStr = "Cannot connecting to provider";		

#ifdef CONFIG_USER_DDNS_LIBSSL
		if (Entry.ServicePort == DDNS_HTTP)
			portStr = "HTTP";
		if (Entry.ServicePort == DDNS_HTTPS)
			portStr = "HTTPS";
#endif

		ifIndex = getIfIndexByName(Entry.interface);
		if (ifIndex == -1)
			ifIndex = DUMMY_IFINDEX;
		nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"radio\" name=\"select\""
		" value=\"s%d\" onClick=postEntry(%d,'%s','%s','%s','%s',%d)></td>\n",
		i, Entry.Enabled, Entry.provider, strValToASP(Entry.hostname), strValToASP(Entry.username), Entry.password, ifIndex);
		nBytesSent += boaWrite(wp,
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"30%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
#else
		"<td align=center width=\"5%%\"><input type=\"radio\" name=\"select\""
		" value=\"s%d\" onClick=postEntry(%d,'%s','%s','%s','%s',%d,%d)></td>\n",
		i, Entry.Enabled, Entry.provider, strValToASP(Entry.hostname), strValToASP(Entry.username), strValToASP(Entry.password), ifIndex, Entry.ServicePort);
		nBytesSent += boaWrite(wp,
		"<td align=center width=\"5%%\">%s</b></td>"
		"<td align=center width=\"20%%\">%s</b></td>"
		"<td align=center width=\"20%%\">%s</b></td>"
		"<td align=center width=\"10%%\">%s</b></td>"
#ifdef CONFIG_USER_DDNS_LIBSSL
		"<td align=center width=\"10%%\">%s</b></td>"
#endif
		"<td align=center width=\"30%%\">%s</b></td>"
#endif
//		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"</tr>\n",
		//Entry.hostname, strcmp(Entry.username, "")==0?Entry.email:Entry.username, Entry.provider, Entry.interface);
//		Entry.Enabled ? "Enable" : "Disable", Entry.hostname, Entry.username, Entry.provider, Entry.interface);
		multilang(Entry.Enabled ? LANG_ENABLE : LANG_DISABLE), strValToASP(Entry.hostname), strValToASP(Entry.username), Entry.provider,
#ifdef CONFIG_USER_DDNS_LIBSSL
		portStr,
#endif
		statusStr);

	}
	return 0;
}
#endif

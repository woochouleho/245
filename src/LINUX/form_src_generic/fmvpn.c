/*
 *      Web server handler routines for IP QoS
 *
 */

/*-- System inlcude files --*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/wait.h>
#include <memory.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/sysinfo.h>


#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "multilang.h"
#include "subr_net.h"
#ifdef EMBED
#include <linux/config.h>
#else
#include "../../../../include/linux/autoconf.h"
#endif


#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
#if 0
void startPPtP(MIB_PPTP_T *pentry)
{
	char *argv[20];
	int i=0;

	argv[i++] = "/bin/pptp";
	argv[i++] = pentry->server;
	argv[i++] = "-detach";
	switch (pentry->authtype)
	{
	case 1://pap
		argv[i++] = "+pap";
		break;
	case 2://chap
		argv[i++] = "+chap";
		break;
	case 3://chapmsv2
		argv[i++] = "+chapms-v2";
		break;
	default:
		break;
	}
	argv[i++] = "noaccomp";
	argv[i++] = "novj";
	argv[i++] = "novjccomp";
	argv[i++] = "default-asyncmap";
	argv[i++] = "noauth";
	argv[i++] = "user";
	argv[i++] = pentry->username;
	argv[i++] = "password";
	argv[i++] = pentry->password;
	if (pentry->dgw)
		argv[i++] = "defaultroute";
	argv[i++] = NULL;

	do_cmd("/bin/pptp", argv, 0);
}
#endif

void formPPtP(request * wp, char *path, char *query)
{
#if defined(CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_PPTP_SERVER)
	char *strAddPptpClient, *strDelPptpClient;
	char *strAddPptpAccount, *strDelPptpAccount, *strSavePptpAccount;
	char *strSetPptpServer;
#endif
	char *submitUrl, *strVal, *strAddPPtP, *strDelPPtP;

	char tmpBuf[100];
	int intVal;
	int pptpEntryNum, i;
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	int l2tpEntryNum;
	MIB_L2TP_T l2tpEntry;
#endif
	MIB_CE_ATM_VC_T vcEntry;
	int wanNum;
	MIB_PPTP_T entry, tmpEntry;
#if defined(CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_PPTP_SERVER)
	MIB_VPN_ACCOUNT_T account, tmpAccount;
	int accountNum;
#endif
	int deleted = 0;
	int enable;
	int pid;
	unsigned int map=0;//maximum rule num is MAX_PPTP_NUM
#ifdef CONFIG_USER_PPTP_SERVER
	int pptpType;
#endif
	int ret=0;
#ifdef CONFIG_USER_PPTP_SERVER
	strVal = boaGetVar(wp, "ltype", "");
	mib_get_s(MIB_PPTP_MODE, (void *)&pptpType, sizeof(pptpType));
	if ((strVal[0]-'0') != pptpType) {
		snprintf(tmpBuf, sizeof(tmpBuf), "%s", PPTPD_PID);
		kill_by_pidfile(tmpBuf, SIGTERM);
		pptpType = (int)(strVal[0]-'0');
		mib_set(MIB_PPTP_MODE, (void *)&pptpType);
	}
#endif
	strVal = boaGetVar(wp, "lst", "");

	// enable/disable PPtP
	if (strVal[0]) {
		strVal = boaGetVar(wp, "pptpen", "");

		if ( strVal[0] == '1' ) {//enable
			enable = 1;
		}
		else//disable
			enable = 0;

		mib_set(MIB_PPTP_ENABLE, (void *)&enable);

		pptp_take_effect();
#ifdef CONFIG_USER_PPTPD_PPTPD
		pptpd_take_effect();
#endif
#ifdef CONFIG_USER_PPTP_SERVER
		pptp_server_take_effect();
#endif
		usleep(1000);
		goto setOK_pptp;
	}

#if defined(CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_PPTP_SERVER)
	strAddPptpClient = boaGetVar(wp, "addClient", "");
	strDelPptpClient = boaGetVar(wp, "delSelClient", "");
	strAddPptpAccount = boaGetVar(wp, "addAccount", "");
	strDelPptpAccount = boaGetVar(wp, "delSelAccount", "");
	strSavePptpAccount = boaGetVar(wp, "saveAccount", "");
	strSetPptpServer = boaGetVar(wp, "addServer", "");
#endif
	strAddPPtP = boaGetVar(wp, "addPPtP", "");
	strDelPPtP = boaGetVar(wp, "delSel", "");

	memset(&entry, 0, sizeof(entry));
	pptpEntryNum = mib_chain_total(MIB_PPTP_TBL); /* get chain record size */
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	l2tpEntryNum = mib_chain_total(MIB_L2TP_TBL);
#endif
	wanNum = mib_chain_total(MIB_ATM_VC_TBL);
#if defined(CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_PPTP_SERVER)
	accountNum = mib_chain_total(MIB_VPN_ACCOUNT_TBL);
#endif

#ifdef CONFIG_USER_PPTPD_PPTPD
	/* Add new pptp client entry */
	if (strAddPptpClient[0]) {
		printf("add pptp client entry.\n");

		strVal = boaGetVar(wp, "c_name", "");
		entry.name[sizeof(entry.name)-1]='\0';
		strncpy(entry.name, strVal,sizeof(entry.name)-1);

		strVal = boaGetVar(wp, "c_addr", "");
		entry.server[sizeof(entry.server)-1]='\0';
		strncpy(entry.server, strVal,sizeof(entry.server)-1);

		for (i=0; i<pptpEntryNum; i++)
		{
			if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&tmpEntry) )
				continue;

			if ( !strcmp(tmpEntry.server, entry.server) || !strcmp(tmpEntry.name, entry.name))
			{
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, multilang(LANG_PPTP_VPN_INTERFACE_HAS_ALREADY_EXIST),sizeof(tmpBuf)-1);
				goto setErr_pptp;
			}

			map |= (1<< tmpEntry.idx);
		}
// Mason Yu. Add VPN ifIndex
#if 0
#ifdef CONFIG_USER_L2TPD_L2TPD
		for (i=0; i<l2tpEntryNum; i++)
		{
			if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tpEntry) )
				continue;
			map |= (1<<l2tpEntry.idx);
		}
#endif//endof CONFIG_USER_L2TPD_L2TPD
#endif
		strVal = boaGetVar(wp, "c_username", "");
		entry.username[sizeof(entry.username)-1]='\0';
		strncpy(entry.username, strVal,sizeof(entry.username)-1);

		strVal = boaGetVar(wp, "c_password", "");
		entry.password[sizeof(entry.password)-1]='\0';
		strncpy(entry.password, strVal,sizeof(entry.password)-1);

		strVal = boaGetVar(wp, "c_auth", "");
		entry.authtype = strVal[0] - '0';

		if (entry.authtype == 3) {
	        strVal = boaGetVar(wp, "c_enctype", "");
			entry.enctype = strVal[0] - '0';
		}

		strVal = boaGetVar(wp, "defaultgw", "");
		if (strVal[0]) {
			entry.dgw = 1;
			for (i=0; i<pptpEntryNum; i++)
			{
				if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&tmpEntry) )
					continue;

				if (tmpEntry.dgw)
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, multilang(LANG_DEFAULT_GW_HAS_ALREADY_EXIST_FOR_PPTP_VPN),sizeof(tmpBuf)-1);
					goto setErr_pptp;
				}
			}
#ifdef CONFIG_USER_SELECT_DEFAULT_GW_MANUALLY
			//check if conflicts with wan interface setting
			for (i=0; i<wanNum; i++)
			{
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
					continue;
				if (vcEntry.dgw)
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, multilang(LANG_DEFAULT_GW_HAS_ALREADY_EXIST),sizeof(tmpBuf)-1);
					goto setErr_pptp;
				}
			}
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
			for (i=0; i<l2tpEntryNum; i++)
			{
				if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tpEntry) )
					continue;
				if (l2tpEntry.dgw)
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, multilang(LANG_DEFAULT_GW_HAS_ALREADY_EXIST_FOR_L2TP_VPN),sizeof(tmpBuf)-1);
					goto setErr_pptp;
				}
			}
#endif//end of CONFIG_USER_L2TPD_L2TPD
#ifdef CONFIG_NET_IPIP
			{
				MIB_IPIP_T ipEntry;
				int ipEntryNum;
				ipEntryNum = mib_chain_total(MIB_IPIP_TBL);
				for (i=0; i<ipEntryNum; i++) {
					if (!mib_chain_get(MIB_IPIP_TBL, i, (void *)&ipEntry))
						continue;
					if (ipEntry.dgw)
					{
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, multilang(LANG_DEFAULT_GW_HAS_ALREADY_EXIST_FOR_IPIP_VPN),sizeof(tmpBuf)-1);
						goto setErr_pptp;
					}
				}
			}
#endif//end of CONFIG_NET_IPIP
		}
#ifdef CONFIG_IPV6_VPN
		strVal = boaGetVar(wp, "IpProtocolType", "");
		if (strVal[0]) {
			entry.IpProtocol = strVal[0] - '0';
		}
#endif
		for (i=0; i<MAX_PPTP_NUM; i++) {
			if (!(map & (1<<i))) {
				entry.idx = i;
				break;
			}
		}
		// Mason Yu. Add VPN ifIndex
		// unit declarations for ppp  on if_sppp.h
		// (1) 0 ~ 7: pppoe/pppoa, (2) 8: 3G, (3) 9 ~ 10: PPTP, (4) 11 ~12: L2TP
		entry.ifIndex = TO_IFINDEX(MEDIA_PPTP, (entry.idx+PPTP_VPN_STRAT), PPTP_INDEX(entry.idx));
		//printf("***** PPTP: entry.ifIndex=0x%x\n", entry.ifIndex);

		intVal = mib_chain_add(MIB_PPTP_TBL, (void *)&entry);
		if (intVal == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tadd_chain_error,sizeof(tmpBuf)-1);
			goto setErr_pptp;
		}
		else if (intVal == -1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
			goto setErr_pptp;
		}
		pptpEntryNum = mib_chain_total(MIB_PPTP_TBL);
		//startPPtP(&entry);
		entry.vpn_enable = 1;
		applyPPtP(&entry, 1, pptpEntryNum-1);
		mib_chain_update(MIB_PPTP_TBL, (void *)&entry, pptpEntryNum-1);

		goto setOK_pptp;
	}
#else
	/* Add new pptp entry */
	if (strAddPPtP[0]) {
		printf("add pptp entry.\n");

		strVal = boaGetVar(wp, "server", "");
		if(strContainXSSChar(strVal)){
			tmpBuf[sizeof(tmpBuf)-1]='\0';
	        strncpy(tmpBuf, "Invalid server!",sizeof(tmpBuf)-1);
	        goto setErr_pptp;
      	}
		entry.server[sizeof(entry.server)-1]='\0';
		strncpy(entry.server, strVal,sizeof(entry.server)-1);

		for (i=0; i<pptpEntryNum; i++)
		{
			if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&tmpEntry) )
				continue;

			if ( !strcmp(tmpEntry.server, entry.server))
			{
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, multilang(LANG_PPTP_VPN_INTERFACE_HAS_ALREADY_EXIST),sizeof(tmpBuf)-1);
				goto setErr_pptp;
			}

			map |= (1<< tmpEntry.idx);
		}
// Mason Yu. Add VPN ifIndex
#if 0
#ifdef CONFIG_USER_L2TPD_L2TPD
		for (i=0; i<l2tpEntryNum; i++)
		{
			if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tpEntry) )
				continue;
			map |= (1<<l2tpEntry.idx);
		}
#endif//endof CONFIG_USER_L2TPD_L2TPD
#endif
		strVal = boaGetVar(wp, "username", "");
		entry.username[sizeof(entry.username)-1]='\0';
		strncpy(entry.username, strVal,sizeof(entry.username)-1);

		strVal = boaGetVar(wp, "encodePassword", "");
		if ( strVal[0] ) {
			rtk_util_data_base64decode(strVal, entry.password, sizeof(entry.password));
			entry.password[sizeof(entry.password)-1] = '\0';
			if ( strlen(entry.password) > MAX_NAME_LEN-1 ) {
				strcpy(tmpBuf, strPasstoolong);
				goto setErr_pptp;
			}
			entry.password[MAX_NAME_LEN-1]='\0';
		}
		else {
			strVal = boaGetVar(wp, "password", "");
			if ( strVal[0] ) {
				if ( strlen(strVal) > MAX_NAME_LEN-1 ) {
					strcpy(tmpBuf, strPasstoolong);
					goto setErr_pptp;
				}
				strncpy(entry.password, strVal, MAX_NAME_LEN-1);
				entry.password[MAX_NAME_LEN-1]='\0';
			}
		}

		strVal = boaGetVar(wp, "auth", "");
		entry.authtype = strVal[0] - '0';

		if (entry.authtype == 3) {
	        strVal = boaGetVar(wp, "enctype", "");
			entry.enctype = strVal[0] - '0';
		}

		strVal = boaGetVar(wp, "defaultgw", "");
		if (strVal[0]) {
			entry.dgw = 1;
			for (i=0; i<pptpEntryNum; i++)
			{
				if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&tmpEntry) )
					continue;

				if (tmpEntry.dgw)
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, multilang(LANG_DEFAULT_GW_HAS_ALREADY_EXIST),sizeof(tmpBuf)-1);
					goto setErr_pptp;
				}
			}
#ifdef CONFIG_USER_SELECT_DEFAULT_GW_MANUALLY
			//check if conflicts with wan interface setting
			for (i=0; i<wanNum; i++)
			{
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
					continue;
				if (vcEntry.dgw)
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, multilang(LANG_DEFAULT_GW_HAS_ALREADY_EXIST),sizeof(tmpBuf)-1);
					goto setErr_pptp;
				}
			}
#endif
		}
#ifdef CONFIG_IPV6_VPN
		strVal = boaGetVar(wp, "IpProtocolType", "");
		if (strVal[0]) {
			entry.IpProtocol = strVal[0] - '0';
		}
#endif
		for (i=0; i<MAX_PPTP_NUM; i++) {
			if (!(map & (1<<i))) {
				entry.idx = i;
				break;
			}
		}
		// Mason Yu. Add VPN ifIndex
		// unit declarations for ppp  on if_sppp.h
		// (1) 0 ~ 7: pppoe/pppoa, (2) 8: 3G, (3) 9 ~ 10: PPTP, (4) 11 ~12: L2TP
		entry.ifIndex = TO_IFINDEX(MEDIA_PPTP, (entry.idx+PPTP_VPN_STRAT), PPTP_INDEX(entry.idx));
		//printf("***** PPTP: entry.ifIndex=0x%x\n", entry.ifIndex);

		intVal = mib_chain_add(MIB_PPTP_TBL, (void *)&entry);
		if (intVal == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tadd_chain_error,sizeof(tmpBuf)-1);
			goto setErr_pptp;
		}
		else if (intVal == -1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
			goto setErr_pptp;
		}
		pptpEntryNum = mib_chain_total(MIB_PPTP_TBL);
		//startPPtP(&entry);
		entry.vpn_enable = 1;
		applyPPtP(&entry, 1, pptpEntryNum-1);
		mib_chain_update(MIB_PPTP_TBL, (void *)&entry, pptpEntryNum-1);

		goto setOK_pptp;
	}
#endif

	/* Delete client */
#ifdef CONFIG_USER_PPTPD_PPTPD
	if (strDelPptpClient[0])
#else
	if (strDelPPtP[0])
#endif
	{
		printf("delete pptp client entry(total %d).\n", pptpEntryNum);
		for (i=pptpEntryNum-1; i>=0; i--) {
			ret = mib_chain_get(MIB_PPTP_TBL, i, (void *)&tmpEntry);
			if(ret == 0)
				printf("mib_chain_get  MIB_PPTP_TBL error!\n");
			snprintf(tmpBuf, 20, "s%d", tmpEntry.idx);
			strVal = boaGetVar(wp, tmpBuf, "");

			if (strVal[0] == '1') {
				deleted ++;
				#if 0
				sprintf(tmpBuf, "/var/run/pptp.pid.%s", tmpEntry.server);
				pid = read_pid(tmpBuf);
				if (pid>0)
					kill(pid, SIGTERM);
				#else
				tmpEntry.vpn_enable = 1; /* Because applyPPtP will check vpn_enable, we need to set it be 1 temporarily.*/
				applyPPtP(&tmpEntry, 0, i);
				tmpEntry.vpn_enable = 0;
				#endif
				if(mib_chain_delete(MIB_PPTP_TBL, i) != 1) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
					goto setErr_pptp;
				}
			}
		}

		if (deleted <= 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_THERE_IS_NO_ITEM_SELECTED_TO_DELETE),sizeof(tmpBuf)-1);
			goto setErr_pptp;
		}

		goto setOK_pptp;
	}

#if defined(CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_PPTP_SERVER)
	/* configure pptp server */
	if (strSetPptpServer[0]) {
		MIB_VPND_T server, pre_server;
		int servernum;
		struct in_addr addr;

		memset(&server, 0, sizeof(MIB_VPND_T));

		server.type = VPN_PPTP;
#if defined(CONFIG_USER_PPTP_SERVER)
		strVal = boaGetVar(wp, "s_name", "");
		strcpy(server.name, strVal);
#endif
		strVal = boaGetVar(wp, "s_auth", "");
		server.authtype = strVal[0] - '0';

		if (server.authtype == 3) {
	        strVal = boaGetVar(wp, "s_enctype", "");
			server.enctype = strVal[0] - '0';
		}

		strVal = boaGetVar(wp, "peeraddr", "");
		inet_aton(strVal, &addr);
		server.peeraddr = addr.s_addr;
		printf("%s peeraddr=%x\n", __func__, server.peeraddr);

#if defined(CONFIG_USER_PPTPD_PPTPD)
		strVal = boaGetVar(wp, "localaddr", "");
		inet_aton(strVal, &addr);
		server.localaddr = addr.s_addr;
		printf("%s localaddr=%x\n", __func__, server.localaddr);
#endif

		/* check if pptp server is modified */
		servernum = mib_chain_total(MIB_VPN_SERVER_TBL);
		for (i=0; i<servernum; i++) {
			if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, &pre_server))
				continue;
			if (VPN_PPTP == pre_server.type)
				break;
		}
		if (i < servernum) {//we are to modify pptp server
			if ((pre_server.authtype != server.authtype) ||
				(pre_server.enctype != server.enctype) ||
				(pre_server.localaddr != server.localaddr) ||
				(pre_server.peeraddr != server.peeraddr))
			{
				printf("pptp server modified, all pptpd account should be reenabled.\n");
				mib_chain_update(MIB_VPN_SERVER_TBL, &server, i);
#ifdef CONFIG_USER_PPTPD_PPTPD
				pptpd_take_effect();
#endif
#ifdef CONFIG_USER_PPTP_SERVER
				pptp_server_take_effect();
#endif
			}
		}
		else {//add pptp server
			mib_chain_add(MIB_VPN_SERVER_TBL, &server);
		}

		goto setOK_pptp;
	}

	/* Add new pptp account entry */
	if (strAddPptpAccount[0]) {
		printf("add pptp account.\n");

		memset(&account, 0, sizeof(MIB_VPN_ACCOUNT_T));
		map = 0;

		account.type = VPN_PPTP;
		strVal = boaGetVar(wp, "s_name", "");
#if defined(CONFIG_USER_PPTPD_PPTPD)
		account.name[sizeof(account.name)-1]='\0';
		strncpy(account.name, "0",sizeof(account.name)-1);
#endif
		strcat(account.name, strVal);

		strVal = boaGetVar(wp, "tunnelen", "");
		account.enable = strVal[0] - '0';

		/*
		for (i=0; i<accountNum; i++)
		{
			if ( !mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&tmpAccount) )
				continue;

			if (VPN_L2TP == tmpAccount.type)//
				continue;

			if (!strcmp(tmpAccount.name, account.name))
			{
				strcpy(tmpBuf, multilang(LANG_PPTP_VPN_ACCOUNT_HAS_ALREADY_EXIST));
				goto setErr_pptp;
			}

			map |= (1<< tmpAccount.idx);
		}
		*/
		strVal = boaGetVar(wp, "s_username", "");
		strcpy(account.username, strVal);

		strVal = boaGetVar(wp, "encodeSPassword", "");
		if ( strVal[0] ) {
			rtk_util_data_base64decode(strVal, account.password, sizeof(account.password));
			account.password[sizeof(account.password)-1] = '\0';
			if ( strlen(account.password) > MAX_NAME_LEN-1 ) {
				strcpy(tmpBuf, strPasstoolong);
				goto setErr_pptp;
			}
			account.password[MAX_NAME_LEN-1]='\0';
		}
		else {
			strVal = boaGetVar(wp, "s_password", "");
			if ( strVal[0] ) {
				if ( strlen(strVal) > MAX_NAME_LEN-1 ) {
					strcpy(tmpBuf, strPasstoolong);
					goto setErr_pptp;
				}
				strncpy(account.password, strVal, MAX_NAME_LEN-1);
				account.password[MAX_NAME_LEN-1]='\0';
			}
		}

		/*
		for (i=0; i<MAX_PPTP_NUM; i++) {
			if (!(map & (1<<i))) {
				account.idx = i;
				break;
			}
		}
		*/
		account.idx = accountNum;

		intVal = mib_chain_add(MIB_VPN_ACCOUNT_TBL, (void *)&account);
		if (intVal == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tadd_chain_error,sizeof(tmpBuf)-1);
			goto setErr_pptp;
		}
		else if (intVal == -1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
			goto setErr_pptp;
		}

		applyPptpAccount(&account, 1);

		goto setOK_pptp;
	}

	if (strDelPptpAccount[0]) {
		unsigned int deleted = 0;

		printf("delete pptp account(total %d).\n", accountNum);
		for (i=accountNum-1; i>=0; i--) {
			mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&tmpAccount);
			if (VPN_L2TP == tmpAccount.type)
				continue;
			snprintf(tmpBuf, 100, "sel%d", tmpAccount.idx);
			strVal = boaGetVar(wp, tmpBuf, "");
			if ( strVal[0] ) {
				deleted++;

				applyPptpAccount(&tmpAccount, 0);

				if(mib_chain_delete(MIB_VPN_ACCOUNT_TBL, i) != 1) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
					goto setErr_pptp;
				}
			}
		}

		if (deleted <= 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_THERE_IS_NO_ITEM_SELECTED_TO_DELETE),sizeof(tmpBuf)-1);
			goto setErr_pptp;
		}

		goto setOK_pptp;
	}

	if (strSavePptpAccount[0]) {
		int enaAccount;
		for (i=0; i<accountNum; i++) {
			mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&tmpAccount);
			if (VPN_L2TP == tmpAccount.type)
				continue;
			snprintf(tmpBuf, 100, "en%d", tmpAccount.idx);
			enaAccount = 0;
			strVal = boaGetVar(wp, tmpBuf, "");
			if ( strVal[0] ) {
				enaAccount = 1;
			}

			if (enaAccount == tmpAccount.enable)
				continue;
			else {
				tmpAccount.enable = enaAccount;
				mib_chain_update(MIB_VPN_ACCOUNT_TBL, (void *)&tmpAccount, i);
				if (enaAccount)
					applyPptpAccount(&tmpAccount, 1);
				else
					applyPptpAccount(&tmpAccount, 0);
			}
		}

		goto setOK_pptp;
	}
#endif

	/* pptp client */
	for (i=0; i<pptpEntryNum; i++) {
		ret = mib_chain_get(MIB_PPTP_TBL, i, (void *)&tmpEntry);
		if(ret == 0)
			printf("mib_chain_get  MIB_PPTP_TBL error!\n");
		snprintf(tmpBuf, 100, "submitpptp%d", tmpEntry.idx);
		strVal = boaGetVar(wp, tmpBuf, "");
		if ( strVal[0] ) {
			#if 0
			sprintf(tmpBuf, "/var/run/pptp.pid.%s", tmpEntry.server);
			pid = read_pid(tmpBuf);
			if (pid>0)
				kill(pid, SIGTERM);

			if ( !strcmp(strVal, "Connect") ) {
				startPPtP(&tmpEntry);
			}
			#else
			applyPPtP(&tmpEntry, 0, i);
			tmpEntry.vpn_enable = 0;
			if ( !strcmp(strVal, "Connect") )
			{
				tmpEntry.vpn_enable = 1;
				applyPPtP(&tmpEntry, 1, i);
			}

			mib_chain_update(MIB_PPTP_TBL, (void *)&tmpEntry, i);
			#endif
		}
	}

setOK_pptp:

// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

#ifndef NO_ACTION
	pid = fork();
	if (pid) {
		waitpid(pid, NULL, 0);
	}
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
		exit(1);
	}
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;

setErr_pptp:
	ERR_MSG(tmpBuf);
}

#define PPTP_CONF	"/var/ppp/pptp.conf"
#define FILE_LOCK
/*
 * FILE FORMAT
 * state(established/closed)
 */
int pptpList(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;

	MIB_PPTP_T Entry;
	unsigned int entryNum, i;
	unsigned int pptpEnable, opened=0;
	FILE *fp;
	#ifdef FILE_LOCK
	struct flock flptp;
	int fdptp;
	#endif
	char buff[256];
	char devname[10];
	char status[20];
	//char filename[150];
	char ifname[10];
	char state[20]="Dead";
	char dev_ifname[IFNAMSIZ], web_ifname[15];
	char *web_state[3]={"Disconnected", "Connecting", "Connected"};

	mib_get_s(MIB_PPTP_ENABLE, (void *)&pptpEnable, sizeof(pptpEnable));
	//if (0 == pptpEnable)//pptp vpn is disable, so no activited pptp itf exists.
	//	return 0;

	entryNum = mib_chain_total(MIB_PPTP_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_PPTP_TBL, i, (void *)&Entry))
		{
			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

		snprintf(ifname, 10, "pptp%d", Entry.idx);
		ifGetName(Entry.ifIndex, dev_ifname, sizeof(dev_ifname));

		//init state[]
		snprintf(state, 20, "Dead");

#if 0
		snprintf(filename, 150, PPTP_STATUS, Entry.server);

		fp = fopen(filename, "r");
		if (NULL == fp) {
			strcpy(state, "closed");
		}
		else {
			if (fscanf(fp, "%s", state) != 1)
				strcpy(state, "closed");
			fclose(fp);
		}
#else
	#ifdef FILE_LOCK
		// file locking
		fdptp = open(PPTP_CONF, O_RDWR);
		if (fdptp != -1) {
			flptp.l_type = F_RDLCK;
			flptp.l_whence = SEEK_SET;
			flptp.l_start = 0;
			flptp.l_len = 0;
			flptp.l_pid = getpid();
			if (fcntl(fdptp, F_SETLKW, &flptp) == -1) {
				printf("pptp read lock failed\n");
				close(fdptp);
				fdptp = -1;
			}
		}

		if (-1 != fdptp) {
	#endif
			if (!(fp=fopen(PPTP_CONF, "r")))
				printf("%s not exists.\n", PPTP_CONF);
			else {
				fgets(buff, sizeof(buff), fp);
				while ( fgets(buff, sizeof(buff), fp) != NULL ) {
					//"if", "dev", "uptime", "totaluptime", "status");
					if(sscanf(buff, "%s%*s%*s%*s%s\n", devname, status) != 2) {
						printf("Unsuported pptp configuration format\n");
						break;
					}
					else {
						if (!strcmp(dev_ifname, devname)) {
							state[sizeof(state)-1]='\0';
							strncpy(state, status,sizeof(state)-1);
							break;
						}
					}
				}
				fclose(fp);
			}
	#ifdef FILE_LOCK
		}

		// file unlocking
		if (fdptp != -1) {
			flptp.l_type = F_UNLCK;
			if (fcntl(fdptp, F_SETLK, &flptp) == -1)
				printf("pptp read unlock failed\n");
			close(fdptp);
		}
	#endif
#endif

#ifdef CONFIG_USER_PPTP_CLIENT_PPTP
		if (!strncmp(state, "Call_Establish", strlen("Call_Establish"))) {
			// <input type="submit" id="ppp0" value="Disconnect" name="submitppp0" onClick="disButton('ppp0')">
			opened = 2;
		}
		else if (!strncmp(state, "Dead", strlen("Dead")))
			opened = 0;
		else
			opened = 1;
#elif defined(CONFIG_USER_PPTP_CLIENT)
		struct in_addr inAddr;
		ifGetName(Entry.ifIndex, dev_ifname, sizeof(dev_ifname));
		if (getInAddr(dev_ifname, IP_ADDR, (void *)&inAddr) == 1)
			opened = 2;
		else
			opened = 0;
#endif

		//snprintf(web_ifname, 15, "pptp%d(%s)", Entry.idx, dev_ifname);
		snprintf(web_ifname, 15, "%s_pptp%d", dev_ifname, Entry.idx);

		if ((1 == opened) || (0 == pptpEnable)) {
			nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
				"<td align=center width=\"3%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><input type=\"checkbox\" name=\"s%d\" value=1></td>\n"
				"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
#else
				"<td align=center width=\"3%%\"><input type=\"checkbox\" name=\"s%d\" value=1></td>\n"
				"<td align=center width=\"5%%\">%s</td>\n"
				"<td align=center width=\"5%%\">%s</td>\n"
				"<td align=center width=\"8%%\">%s</td>\n"
#endif
				"</tr>",
				Entry.idx, web_ifname, strValToASP(Entry.server), web_state[opened]);
		}
		else {
			nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
				"<td align=center width=\"3%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><input type=\"checkbox\" name=\"s%d\" value=1></td>\n"
				"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				//"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n")
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"> "
				"  <input type=\"hidden\" name=\"submit%s\" value=\"%s\"/>"
				"  <input type=\"submit\" id=\"btn_%s\" value=\"%s\" onClick=on_submit(this)>"
				"</td>\n"
#else
				"<td align=center width=\"3%%\"><input type=\"checkbox\" name=\"s%d\" value=1></td>\n"
				"<td align=center width=\"5%%\">%s</td>\n"
				"<td align=center width=\"5%%\">%s</td>\n"
				//"<th align=center width=\"8%%\">%s ")
				"<td align=center width=\"8%%\">"
				"  <input type=\"hidden\" name=\"submit%s\" value=\"%s\"/>"
				"  <input type=\"submit\" id=\"btn_%s\" value=\"%s\" onClick=\"disButton('btn_%s'); return on_submit(this)\">"
				"</td>\n"
#endif
				"</tr>",
				Entry.idx,
				web_ifname, strValToASP(Entry.server),
				//web_state[opened],
				ifname, ((2 == opened) ? "Disconnect" : "Connect"),
				ifname, multilang(((2 == opened)?LANG_DISCONNECT:LANG_CONNECT))
			#ifdef CONFIG_GENERAL_WEB
				,ifname
			#endif
			);// 
		}
	}
	return nBytesSent;
}

#if defined(CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_PPTP_SERVER)
int pptpServerList(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	MIB_VPN_ACCOUNT_T Entry;
	unsigned int entryNum, i;
	unsigned char password[MAX_NAME_LEN];

	entryNum = mib_chain_total(MIB_VPN_ACCOUNT_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&Entry))
		{
			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

		if (VPN_L2TP == Entry.type)
			continue;

		memset(password, 0, sizeof(password));
		rtk_util_convert_to_star_string(password,strlen(Entry.password));
		password[MAX_NAME_LEN-1]='\0';

		nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
			"<td align=center width=\"3%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><input type=\"checkbox\" name=\"sel%d\" value=1></td>\n"
			"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
			"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><input type=\"checkbox\" name=\"en%d\" %s value=1></td>\n"
			"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
			"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
#else
			"<td align=center><input type=\"checkbox\" name=\"sel%d\" value=1></td>\n"
			"<td align=center>%s</td>\n"
			"<td align=center><input type=\"checkbox\" name=\"en%d\" %s value=1></td>\n"
			"<td align=center>%s</td>\n"
			"<td align=center>%s</td>\n"
#endif
			"</tr>",
			Entry.idx,
#if defined(CONFIG_USER_PPTPD_PPTPD)
			Entry.name+1,
#else
			Entry.name,
#endif
			Entry.idx, Entry.enable?"checked":"",
			Entry.username, password);
	}
}
#endif
#endif

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
//#define L2TP_PID_FILENAME	"/var/run/openl2tpd.pid"
//#define L2TP_CONF			"/var/openl2tpd.conf"
#define L2TP_CONF				"/var/ppp/l2tp.conf"

void formL2TP(request * wp, char *path, char *query)
{
		char *submitUrl, *strAddL2tpClient, *strDelL2tpClient, *strVal;
#ifdef CONFIG_USER_L2TPD_LNS
		char *strAddL2tpAccount, *strDelL2tpAccount, *strSaveL2tpAccount;
		char *strSetL2tpServer;
#endif
	char tmpBuf[100];
	int intVal, i;
	unsigned int entryNum, l2tpEntryNum;
	MIB_CE_ATM_VC_T vcEntry;
	unsigned int wanNum;
	MIB_L2TP_T entry, tmpEntry;
#ifdef CONFIG_USER_L2TPD_LNS
	MIB_VPN_ACCOUNT_T account, tmpAccount;
	int accountNum;
#endif
	int enable;
	int pid;
	unsigned int map=0;//maximum rule num is MAX_L2TP_NUM
#ifdef CONFIG_USER_L2TPD_LNS
	int l2tpType;
#endif
	int ret = 0;
#ifdef CONFIG_USER_L2TPD_LNS
	strVal = boaGetVar(wp, "ltype", "");
	if (strVal[0]) {
		mib_get_s(MIB_L2TP_MODE, (void *)&l2tpType, sizeof(l2tpType));
		if ((strVal[0]-'0') != l2tpType) {
			snprintf(tmpBuf, sizeof(tmpBuf), "%s", XL2TPD_PID);
			kill_by_pidfile(tmpBuf, SIGTERM);
			l2tpType = (int)(strVal[0]-'0');
			mib_set(MIB_L2TP_MODE, (void *)&l2tpType);
		}
	}
#endif
	strVal = boaGetVar(wp, "lst", "");

	// enable/disable L2TP
	if (strVal[0]) {
		mib_get_s(MIB_L2TP_ENABLE, (void *)&enable, sizeof(enable));
		strVal = boaGetVar(wp, "l2tpen", "");
		if ((strVal[0]-'0') != enable) {
			if ( strVal[0] == '1' ) {//enable
				enable = 1;
			}
			else//disable
				enable = 0;

			mib_set(MIB_L2TP_ENABLE, (void *)&enable);
			l2tp_take_effect();
		}

		goto setOK_l2tp;
	}

	strAddL2tpClient = boaGetVar(wp, "addClient", "");
	strDelL2tpClient = boaGetVar(wp, "delSelClient", "");
#ifdef CONFIG_USER_L2TPD_LNS
	strAddL2tpAccount = boaGetVar(wp, "addAccount", "");
	strDelL2tpAccount = boaGetVar(wp, "delSelAccount", "");
	strSaveL2tpAccount = boaGetVar(wp, "saveAccount", "");
	strSetL2tpServer = boaGetVar(wp, "addServer", "");
#endif

	memset(&entry, 0, sizeof(entry));
	l2tpEntryNum = mib_chain_total(MIB_L2TP_TBL); /* get chain record size */
	wanNum = mib_chain_total(MIB_ATM_VC_TBL);
#ifdef CONFIG_USER_L2TPD_LNS
	accountNum = mib_chain_total(MIB_VPN_ACCOUNT_TBL);
#endif

	/* Add new l2tp entry */
	if (strAddL2tpClient[0])
	{
		strVal = boaGetVar(wp, "c_name", "");
		entry.name[sizeof(entry.name)-1]='\0';
		strncpy(entry.name, strVal,sizeof(entry.name)-1);
		strVal = boaGetVar(wp, "server", "");
		if(strContainXSSChar(strVal)){
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, "Invalid server!",sizeof(tmpBuf)-1);
			goto setErr_l2tp;
		}
		entry.server[sizeof(entry.server)-1]='\0';
		strncpy(entry.server, strVal,sizeof(entry.server)-1);

		for (i=0; i<l2tpEntryNum; i++)
		{
			if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&tmpEntry) )
				continue;

			if ( !strcmp(tmpEntry.server, entry.server)
				|| !strcmp(tmpEntry.name, entry.name)
				)
			{
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, multilang(LANG_L2TP_VPN_INTERFACE_HAS_ALREADY_EXIST),sizeof(tmpBuf)-1);
				goto setErr_l2tp;
			}

			map |= (1<< tmpEntry.idx);
		}
		strVal = boaGetVar(wp, "tunnel_auth", "");
		if (strVal[0])
			entry.tunnel_auth = 1;

		if (entry.tunnel_auth) {
			strVal = boaGetVar(wp, "tunnel_secret", "");
			entry.secret[sizeof(entry.secret)-1]='\0';
			strncpy(entry.secret, strVal,sizeof(entry.secret)-1);
		}

		strVal = boaGetVar(wp, "auth", "");
		entry.authtype = strVal[0] - '0';

		if (entry.authtype == 3) {
	        strVal = boaGetVar(wp, "enctype", "");
			entry.enctype = strVal[0] - '0';
		}

		strVal = boaGetVar(wp, "username", "");
		entry.username[sizeof(entry.username)-1]='\0';
		strncpy(entry.username, strVal,sizeof(entry.username)-1);

		strVal = boaGetVar(wp, "encodePassword", "");
		if ( strVal[0] ) {
			rtk_util_data_base64decode(strVal, entry.password, sizeof(entry.password));
			entry.password[sizeof(entry.password)-1] = '\0';
			if ( strlen(entry.password) > MAX_NAME_LEN-1 ) {
				strcpy(tmpBuf, strPasstoolong);
				goto setErr_l2tp;
			}
			entry.password[MAX_NAME_LEN-1]='\0';
		}
		else {
			strVal = boaGetVar(wp, "password", "");
			if ( strVal[0] ) {
				if ( strlen(strVal) > MAX_NAME_LEN-1 ) {
					strcpy(tmpBuf, strPasstoolong);
					goto setErr_l2tp;
				}
				strncpy(entry.password, strVal, MAX_NAME_LEN-1);
				entry.password[MAX_NAME_LEN-1]='\0';
			}
		}

		strVal = boaGetVar(wp, "pppconntype", "");
		entry.conntype = strVal[0] - '0';

		if (entry.conntype == 1)
		{
			unsigned int idletime;
			// Mason Yu. If the connection is dial-on-demand, set dgw=1.
			entry.dgw = 1;
			strVal = boaGetVar(wp, "idletime", "");
			if (strVal[0]) {
				sscanf(strVal, "%u", &entry.idletime);
				//sscanf(strVal, "%u", &idletime);
				//printf("idletime is %d\n", idletime);
				//entry.idletime = idletime;
			}
		}

		strVal = boaGetVar(wp, "mtu", "");
		if (strVal[0]) {
			unsigned int mtu;
			sscanf(strVal, "%u", &entry.mtu);
			//sscanf(strVal, "%u", &mtu);
			//printf("mtu is %d\n", mtu);
			//entry.mtu = mtu;
		}

		strVal = boaGetVar(wp, "defaultgw", "");
		if (strVal[0]) {
			entry.dgw = 1;
		}

		if (entry.dgw == 1) {
			for (i=0; i<l2tpEntryNum; i++)
			{
				if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&tmpEntry) )
					continue;

				if (tmpEntry.dgw)
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, multilang(LANG_DEFAULT_GW_HAS_ALREADY_EXIST),sizeof(tmpBuf)-1);
					goto setErr_l2tp;
				}
			}
#ifdef CONFIG_USER_SELECT_DEFAULT_GW_MANUALLY
			//check if conflicts with wan interface setting
			for (i=0; i<wanNum; i++)
			{
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
					continue;
				if (vcEntry.dgw)
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, multilang(LANG_DEFAULT_GW_HAS_ALREADY_EXIST),sizeof(tmpBuf)-1);
					goto setErr_l2tp;
				}
			}
#endif
		}

#ifdef CONFIG_IPV6_VPN
		strVal = boaGetVar(wp, "IpProtocolType", "");
		if (strVal[0]) {
			entry.IpProtocol = strVal[0] - '0';
		}
#endif

		for (i=0; i<MAX_L2TP_NUM; i++) {
			if (!(map & (1<<i))) {
				entry.idx = i;
				break;
			}
		}

#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
		strVal = boaGetVar(wp, "overipsecLAC", "");
		if ( strVal[0] )
			entry.overIPSec = 1;

		if(entry.overIPSec == 1){
			strVal = boaGetVar(wp, "encodepskValueLAC", "");
			if (strVal[0]) {
				if(strVal[0]){
					rtk_util_data_base64decode(strVal, entry.IKEPreshareKey, sizeof(entry.IKEPreshareKey));
					entry.IKEPreshareKey[sizeof(entry.IKEPreshareKey)-1] = '\0';
				}
			}
			else
			{
				strVal = boaGetVar(wp, "IKEPreshareKeyLAC", "");
				if(strVal[0]){
					strncpy(entry.IKEPreshareKey, strVal, IPSEC_NAME_LEN);
				}
			}
		}
#endif

		// Mason Yu. Add VPN ifIndex
		// unit declarations for ppp  on if_sppp.h
		// (1) 0 ~ 7: pppoe/pppoa, (2) 8: 3G, (3) 9 ~ 10: PPTP, (4) 11 ~12: L2TP
		entry.ifIndex = TO_IFINDEX(MEDIA_L2TP, (entry.idx+L2TP_VPN_STRAT), L2TP_INDEX(entry.idx));
		//printf("***** L2TP: entry.ifIndex=0x%x\n", entry.ifIndex);

		intVal = mib_chain_add(MIB_L2TP_TBL, (void *)&entry);
		if (intVal == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tadd_chain_error,sizeof(tmpBuf)-1);
			goto setErr_l2tp;
		}
		else if (intVal == -1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
			goto setErr_l2tp;
		}
		l2tpEntryNum = mib_chain_total(MIB_L2TP_TBL); /* get chain record size */
		
		entry.vpn_enable = 1;

#ifndef CONFIG_USER_XL2TPD
		applyL2TP(&entry, 1, l2tpEntryNum-1);
#endif
		mib_chain_update(MIB_L2TP_TBL, (void *)&entry, l2tpEntryNum-1);
#ifdef CONFIG_USER_XL2TPD
		l2tp_take_effect();
#endif

		goto setOK_l2tp;
	}

	/* Delete entry */
	if (strDelL2tpClient[0])
	{
		int i;
		unsigned int deleted = 0;

		for (i=l2tpEntryNum-1; i>=0; i--) {
			ret= mib_chain_get(MIB_L2TP_TBL, i, (void *)&tmpEntry);
			if(ret == 0)
				printf("mib_chain_get  MIB_L2TP_TBL error!\n");
			snprintf(tmpBuf, 20, "s%d", tmpEntry.idx);
			strVal = boaGetVar(wp, tmpBuf, "");

			if (strVal[0] == '1') {
				deleted ++;
				entry.vpn_enable = 1; /* Because applyL2TP will check vpn_enable, we need to set it be 1 temporarily.*/
				applyL2TP(&tmpEntry, 0, i);
				entry.vpn_enable = 0;
				if(mib_chain_delete(MIB_L2TP_TBL, i) != 1) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
					goto setErr_l2tp;
				}
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
				l2tpIPSec_LAC_strongswan_take_effect();
#endif
			}
		}

		if (deleted <= 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_THERE_IS_NO_ITEM_SELECTED_TO_DELETE),sizeof(tmpBuf)-1);
			goto setErr_l2tp;
		}

#ifdef CONFIG_USER_XL2TPD
		l2tp_take_effect();
#endif
		goto setOK_l2tp;
	}

#ifdef CONFIG_USER_L2TPD_LNS
	/* configure l2tp server */
	if (strSetL2tpServer[0]) {
		MIB_VPND_T server, pre_server;
		int servernum;
		struct in_addr addr;

		memset(&server, 0, sizeof(MIB_VPND_T));

		server.type = VPN_L2TP;

		strVal = boaGetVar(wp, "s_name", "");
		strcpy(server.name, strVal);

		strVal = boaGetVar(wp, "s_auth", "");
		server.authtype = strVal[0] - '0';

		if (server.authtype == 3) {
			strVal = boaGetVar(wp, "s_enctype", "");
			server.enctype = strVal[0] - '0';
		}

		strVal = boaGetVar(wp, "s_tunnelAuth", "");
		if ( strVal[0] )
			server.tunnel_auth = 1;

		if(server.tunnel_auth == 1){
			strVal = boaGetVar(wp, "s_authKey", "");
			server.tunnel_key[sizeof(server.tunnel_key)-1]='\0';
			strncpy(server.tunnel_key, strVal,sizeof(server.tunnel_key)-1);
		}

		strVal = boaGetVar(wp, "peeraddr", "");
		inet_aton(strVal, &addr);
		server.peeraddr = addr.s_addr;

		strVal = boaGetVar(wp, "localaddr", "");
		inet_aton(strVal, &addr);
		server.localaddr = addr.s_addr;

#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
		strVal = boaGetVar(wp, "overipsecLNS", "");
		if ( strVal[0] )
			server.overIPSec = 1;

		if(server.overIPSec == 1){
			strVal = boaGetVar(wp, "encodepskValueLNS", "");
			if (strVal[0]) {
				if(strVal[0]){
					rtk_util_data_base64decode(strVal, server.IKEPreshareKey, sizeof(server.IKEPreshareKey));
					server.IKEPreshareKey[sizeof(server.IKEPreshareKey)-1] = '\0';
				}
			}
			else
			{
				strVal = boaGetVar(wp, "IKEPreshareKeyLNS", "");
				if(strVal[0]){
					strncpy(server.IKEPreshareKey, strVal, IPSEC_NAME_LEN);
				}
			}
		}
#endif

		/* check if l2tp server is modified */
		servernum = mib_chain_total(MIB_VPN_SERVER_TBL);
		for (i=0; i<servernum; i++) {
			if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, &pre_server))
				continue;
			if (VPN_L2TP == pre_server.type)
				break;
		}
		if (i < servernum) {//we are to modify l2tp server
			if ((pre_server.authtype != server.authtype) ||
				(pre_server.enctype != server.enctype) ||
				(pre_server.tunnel_auth != server.tunnel_auth) ||
				(pre_server.tunnel_key != server.tunnel_key) ||
				(pre_server.localaddr != server.localaddr) ||
				(pre_server.peeraddr != server.peeraddr))
			{
				printf("l2tp server modified, all l2tpd account should be reenabled.\n");
				mib_chain_update(MIB_VPN_SERVER_TBL, &server, i);
				l2tp_take_effect();
			}
		}
		else {//add l2tp server
			mib_chain_add(MIB_VPN_SERVER_TBL, &server);
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
			l2tpIPSec_LNS_strongswan_take_effect();
#endif
		}

		goto setOK_l2tp;
	}
	/* Add new l2tp account entry */
	if (strAddL2tpAccount[0]) {
		memset(&account, 0, sizeof(MIB_VPN_ACCOUNT_T));
		map = 0;

		account.type = VPN_L2TP;

		strVal = boaGetVar(wp, "s_accout_name", "");
		account.name[sizeof(account.name)-1]='\0';
		strncpy(account.name, "1",sizeof(account.name)-1);

		strVal = boaGetVar(wp, "s_username", "");
		account.name[sizeof(account.name)-1]='\0';
		strncpy(account.username, strVal,sizeof(account.username)-1);

		strVal = boaGetVar(wp, "encodeSPassword", "");
		if ( strVal[0] ) {
			rtk_util_data_base64decode(strVal, account.password, sizeof(account.password));
			account.password[sizeof(account.password)-1] = '\0';
			if ( strlen(account.password) > MAX_NAME_LEN-1 ) {
				strcpy(tmpBuf, strPasstoolong);
				goto setErr_l2tp;
			}
			account.password[MAX_NAME_LEN-1]='\0';
		}
		else {
			strVal = boaGetVar(wp, "s_password", "");
			if ( strVal[0] ) {
				if ( strlen(strVal) > MAX_NAME_LEN-1 ) {
					strcpy(tmpBuf, strPasstoolong);
					goto setErr_l2tp;
				}
				strncpy(account.password, strVal, MAX_NAME_LEN-1);
				account.password[MAX_NAME_LEN-1]='\0';
			}
		}

		strVal = boaGetVar(wp, "tunnelen", "");
		account.enable = strVal[0] - '0';

		for (i=0; i<accountNum; i++)
		{
			if ( !mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&tmpAccount) )
				continue;

			if (VPN_PPTP == tmpAccount.type)//
				continue;

			if (!strncmp(tmpAccount.name, account.name, strlen(account.name)+1)
				&& !strncmp(tmpAccount.username, account.username, strlen(account.username)+1)
				&& !strncmp(tmpAccount.password, account.password, strlen(account.password)+1))
			{
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, multilang(LANG_L2TP_VPN_ACCOUNT_HAS_ALREADY_EXIST),sizeof(tmpBuf)-1);
				goto setErr_l2tp;
			}

			map |= (1<< tmpAccount.idx);
		}

		for (i=0; i<MAX_L2TP_NUM; i++) {
			if (!(map & (1<<i))) {
				account.idx = i;
				break;
			}
		}

		intVal = mib_chain_add(MIB_VPN_ACCOUNT_TBL, (void *)&account);
		if (intVal == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tadd_chain_error,sizeof(tmpBuf)-1);
			goto setErr_l2tp;
		}
		else if (intVal == -1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
			goto setErr_l2tp;
		}

		if(account.enable == 1)
			applyL2tpAccount(&account, 1);

		goto setOK_l2tp;
	}

	if (strDelL2tpAccount[0]) {
		unsigned int deleted = 0;

		for (i=accountNum-1; i>=0; i--) {
			mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&tmpAccount);
			if (VPN_PPTP == tmpAccount.type)
				continue;
			snprintf(tmpBuf, 100, "sel%d", tmpAccount.idx);
			strVal = boaGetVar(wp, tmpBuf, "");
			if ( strVal[0] ) {
				deleted++;

				if(tmpAccount.enable == 1)
					applyL2tpAccount(&tmpAccount, 0);
				if(mib_chain_delete(MIB_VPN_ACCOUNT_TBL, i) != 1) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
					goto setErr_l2tp;
				}
			}
		}

		if (deleted <= 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_THERE_IS_NO_ITEM_SELECTED_TO_DELETE),sizeof(tmpBuf)-1);
			goto setErr_l2tp;
		}

		goto setOK_l2tp;
	}

	if (strSaveL2tpAccount[0]) {
		int enaAccount;
		for (i=0; i<accountNum; i++) {
			mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&tmpAccount);
			if (VPN_PPTP == tmpAccount.type)
				continue;
			snprintf(tmpBuf, 100, "en%d", tmpAccount.idx);
			enaAccount = 0;
			strVal = boaGetVar(wp, tmpBuf, "");
			if ( strVal[0] ) {
				enaAccount = 1;
			}

			if (enaAccount == tmpAccount.enable)
				continue;
			else {
				tmpAccount.enable = enaAccount;
				mib_chain_update(MIB_VPN_ACCOUNT_TBL, (void *)&tmpAccount, i);
				if (enaAccount)
					applyL2tpAccount(&tmpAccount, 1);
				else
					applyL2tpAccount(&tmpAccount, 0);
			}
		}

		goto setOK_l2tp;
	}
#endif

	for (i=0; i<l2tpEntryNum; i++) {
		ret = mib_chain_get(MIB_L2TP_TBL, i, (void *)&tmpEntry);
		if(ret == 0)
			printf("mib_chain_get  MIB_L2TP_TBL error!\n");
		snprintf(tmpBuf, 100, "submitl2tp%d", tmpEntry.idx);
		strVal = boaGetVar(wp, tmpBuf, "");
#ifdef CONFIG_USER_L2TPD_L2TPD
		if ( strVal[0] ) {
			if (tmpEntry.conntype != MANUAL) {
				applyL2TP(&tmpEntry, 0, i);		// delete ppp interface
				tmpEntry.vpn_enable = 0;
				if ( !strcmp(strVal, "Connect") )
				{
					tmpEntry.vpn_enable = 1;
					applyL2TP(&tmpEntry, 1, i);	// add	a ppp interface
				}
			}
			else if (tmpEntry.conntype == MANUAL) {
				if ( !strcmp(strVal, "Disconnect") )
				{
					tmpEntry.vpn_enable = 1;
					applyL2TP(&tmpEntry, 3, i);	// disconnect(down) for dial on demand
					tmpEntry.vpn_enable = 0;
				}
				if ( !strcmp(strVal, "Connect") ) {
					tmpEntry.vpn_enable = 1;
					applyL2TP(&tmpEntry, 0, i);    // delete
					applyL2TP(&tmpEntry, 1, i);   	// new
					applyL2TP(&tmpEntry, 2, i);   	// connect(up) for dial on demand
				}
			}
			mib_chain_update(MIB_L2TP_TBL, (void *)&tmpEntry, i);
		}
#elif defined(CONFIG_USER_XL2TPD)
		if ( strVal[0] ) {
			if ( !strcmp(strVal, "Connect") && tmpEntry.conntype != MANUAL) {
				applyL2TP(&tmpEntry, 1, i); // add	a ppp interface
			}
			else if (!strcmp(strVal, "Disconnect") && tmpEntry.conntype != MANUAL) {
				applyL2TP(&tmpEntry, 0, i);		// delete ppp interface
			}
			else if (tmpEntry.conntype == MANUAL) {
				if ( !strcmp(strVal, "Disconnect") )
					applyL2TP(&tmpEntry, 3, i);	// disconnect(down) for dial on demand
				if ( !strcmp(strVal, "Connect") ) {
					applyL2TP(&tmpEntry, 0, i);    // delete
					applyL2TP(&tmpEntry, 1, i);   	// new
					applyL2TP(&tmpEntry, 2, i);   	// connect(up) for dial on demand
				}
			}
		}
#endif
	}

setOK_l2tp:

// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

#ifndef NO_ACTION
	pid = fork();
	if (pid) {
		waitpid(pid, NULL, 0);
	}
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
		exit(1);
	}
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;

setErr_l2tp:
	ERR_MSG(tmpBuf);
}

int lns_name_list(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
#if defined(CONFIG_USER_L2TPD_LNS)
	MIB_VPND_T server;
	int total=0, i=0;

	total = mib_chain_total(MIB_VPN_SERVER_TBL);
	for (i=0 ; i<total ; i++) {
		if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, (void *)&server))
			continue;

		if(server.type != VPN_L2TP)
			continue;

		nBytesSent += boaWrite(wp, "<option value=\"%s\">%s</option>\n", server.name, server.name);
	}
#endif
	return nBytesSent;
}

int l2tpList(int eid, request * wp, int argc, char **argv)
{
	MIB_L2TP_T Entry;
	unsigned int entryNum, i;
	int l2tpEnable, l2tp_phase=0;
	char ifname[10], l2tp_status[20], devname[10], buff[256];
	char *state[3]={"Disconnected", "Connecting", "Connected"};
	FILE *fp;
	struct flock fl;
	int fd;
	int nBytesSent=0;
	char dev_ifname[IFNAMSIZ], web_ifname[15], action_str[15];
	int action_int;

	mib_get_s(MIB_L2TP_ENABLE, (void *)&l2tpEnable, sizeof(l2tpEnable));

	entryNum = mib_chain_total(MIB_L2TP_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_L2TP_TBL, i, (void *)&Entry))
		{
			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

		snprintf(ifname, 10, "l2tp%d", Entry.idx);
		ifGetName(Entry.ifIndex, dev_ifname, sizeof(dev_ifname));

		l2tp_phase = 0;

		// file locking
		fd = open(L2TP_CONF, O_RDWR);
		if (fd != -1) {
			fl.l_type = F_RDLCK;
			fl.l_whence = SEEK_SET;
			fl.l_start = 0;
			fl.l_len = 0;
			fl.l_pid = getpid();
			if (fcntl(fd, F_SETLKW, &fl) == -1) {
				printf("l2tp read lock failed\n");
				close(fd);
				fd = -1;
			}
		}

		if (-1 != fd) {
			if (!(fp=fopen(L2TP_CONF, "r")))
				printf("%s not exists.\n", L2TP_CONF);
			else {
				fgets(buff, sizeof(buff), fp);
				while ( fgets(buff, sizeof(buff), fp) != NULL ) {
					//"if", "dev", "uptime", "totaluptime", "status");
					if(sscanf(buff, "%s%*s%*s%*s%s\n", devname, l2tp_status) != 2) {
						printf("Unsuported l2tp configuration format\n");
						break;
					}
					else {
						// Mason Yu
						//printf("ifname=%s, devname=%s, l2tp_status=%s\n", ifname, devname, l2tp_status);
						if (!strcmp(dev_ifname, devname)) {
							if (!strncmp(l2tp_status, "session_establish", strlen("session_establish")))
								l2tp_phase = 2;
							else if (!strncmp(l2tp_status, "dead", strlen("dead")))
								l2tp_phase = 0;
							else
								l2tp_phase = 1;
							break;
						}
					}
				}
				fclose(fp);
			}
		}

		// file unlocking
		if (fd != -1) {
			fl.l_type = F_UNLCK;
			if (fcntl(fd, F_SETLK, &fl) == -1)
				printf("l2tp read unlock failed\n");
			close(fd);
		}

		//snprintf(web_ifname, 15, "l2tp%d(%s)", Entry.idx, dev_ifname);
		snprintf(web_ifname, 15, "%s_l2tp%d", dev_ifname, Entry.idx);

#ifdef CONFIG_USER_L2TPD_L2TPD
		//if (Entry.conntype == MANUAL) {
			if( 2 == l2tp_phase)
				//snprintf(action_str, 15, "Disconnect");
				action_int =LANG_DISCONNECT;
			else
				//snprintf(action_str, 15, "Connect");
				action_int =LANG_CONNECT;
		//}
		//else {
		//	if( 2 == l2tp_phase)
		//		snprintf(action_str, 15, "Disable");
		//	else
		//		snprintf(action_str, 15, "Enable");
		//}
#elif defined(CONFIG_USER_XL2TPD)
		struct in_addr inAddr;
		if (getInAddr(dev_ifname, IP_ADDR, (void *)&inAddr) == 1)
			action_int = LANG_DISCONNECT;
		else
			action_int = LANG_CONNECT;
#endif

		if ((1 == l2tp_phase) || (0 == l2tpEnable)) {
			nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
				"<td align=center width=\"3%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><input type=\"checkbox\" name=\"s%d\" value=1></td>\n"
				"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				  "<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				  "<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				  "<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				  "<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n"
				  "<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				  "<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
#else
				"<td align=center width=\"3%%\"><input type=\"checkbox\" name=\"s%d\" value=1></td>\n"
				"<td align=center width=\"5%%\">%s</td>\n"
				  "<td align=center width=\"5%%\">%s</td>\n"
				  "<td align=center width=\"5%%\">%s</td>\n"
				  "<td align=center width=\"5%%\">%s</td>\n"
				  "<td align=center width=\"5%%\">%d</td>\n"
				  "<td align=center width=\"5%%\">%s</td>\n"
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
				  "<td align=center width=\"5%%\">%s</td>\n"
#endif
				  "<td align=center width=\"8%%\">%s</td>\n"
#endif
				"</tr>",
				Entry.idx,
//#ifdef CONFIG_USER_L2TPD_LNS
#if 1
				strValToASP(Entry.name),
#else
				web_ifname,
#endif
				strValToASP(Entry.server),
				multilang(Entry.tunnel_auth ? LANG_CHALLENGE : LANG_NONE),
				vpn_auth[Entry.authtype], Entry.mtu,
				multilang(Entry.dgw ? LANG_ON : LANG_OFF),
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
				(Entry.overIPSec ? "Yes" : "No"),
#endif
				state[l2tp_phase]);
		}
		else {
			nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
				"<td align=center width=\"3%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><input type=\"checkbox\" name=\"s%d\" value=1></td>\n"
				"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				  "<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				  "<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				  "<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				  "<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n"
				  "<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				  //"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s ")
				  "<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"> "
				"<input type=\"submit\" id=\"%s\" value=\"%s\" name=\"submit%s\"></td>\n"
#else
				"<td align=center width=\"3%%\"><input type=\"checkbox\" name=\"s%d\" value=1></td>\n"
				"<td align=center width=\"5%%\">%s</td>\n"
				  "<td align=center width=\"5%%\">%s</td>\n"
				  "<td align=center width=\"5%%\">%s</td>\n"
				  "<td align=center width=\"5%%\">%s</td>\n"
				  "<td align=center width=\"5%%\">%d</td>\n"
				  "<td align=center width=\"5%%\">%s</td>\n"
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
				  "<td align=center width=\"5%%\">%s</td>\n"
#endif
				  //"<td align=center width=\"8%%\">%s ")
				  "<td align=center width=\"8%%\"> "
				  "  <input type=\"hidden\" name=\"submit%s\" value=\"%s\"/>"
				  "  <input type=\"submit\" id=\"btn_%s\" value=\"%s\" onClick=\"return on_submit(this)\">"
				  "</td>\n"
#endif
				"</tr>",
				Entry.idx,
//#ifdef CONFIG_USER_L2TPD_LNS
#if 1
				strValToASP(Entry.name),
#else
				web_ifname,
#endif
				strValToASP(Entry.server),
				multilang(Entry.tunnel_auth ? LANG_CHALLENGE : LANG_NONE),
				vpn_auth[Entry.authtype], Entry.mtu,
				multilang(Entry.dgw ? LANG_ON : LANG_OFF),
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
				(Entry.overIPSec ? "Yes" : "No"),
#endif
				//state[l2tp_phase],
				ifname, ((action_int == LANG_CONNECT) ? "Connect" : "Disconnect"), 
				//multilang((2 == l2tp_phase) ? "Disconnect" : "Connect"), ifname);
				ifname, multilang(action_int));
		}
	}
	return nBytesSent;
}

int l2tpWithIPSecLAC(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
	nBytesSent += boaWrite(wp,	"<tr>\n"
								"	<th>over IPSec:</th>\n"
								"	<td><input type=\"checkbox\" name=\"overipsecLAC\" onClick=\"onClickLacOverIPSec()\"></td>\n"
								"</tr>\n"
								"<tr>\n"
								"	<th>IKEPreshareKey:</th>\n"
								"	<td><input name=\"IKEPreshareKeyLAC\" type=\"password\" size=\"32\" maxlength=\"256\" disabled>\n"
								"	    <input type=\"checkbox\" onClick=\"show_password(3)\">Show Password</td>\n"
								"</tr>\n");
#endif
	return nBytesSent;
}

int l2tpWithIPSecLNS(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_L2TPD_LNS) && defined(CONFIG_USER_STRONGSWAN)
	int servernum=0, i=0;
	MIB_VPND_T server;
	servernum = mib_chain_total(MIB_VPN_SERVER_TBL);
	for (i=0; i<servernum; i++) {
		if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, &server))
			continue;
		if (VPN_L2TP == server.type)
			break;
	}

	nBytesSent += boaWrite(wp,	"<tr>\n"
								"	<th>over IPSec:</th>\n"
								"	<td><input type=\"checkbox\" name=\"overipsecLNS\" onClick=\"onClickLnsOverIPSec()\"></td>\n"
								"</tr>\n"
								"<tr>\n"
								"	<th>IKEPreshareKey:</th>\n"
								"	<td><input name=\"IKEPreshareKeyLNS\" type=\"password\" size=\"32\" maxlength=\"256\" %s>\n"
								"	    <input type=\"checkbox\" onClick=\"show_password(4)\">Show Password</td>\n"
								"</tr>\n", ((server.overIPSec)?"":"disabled"));
#endif
	return nBytesSent;
}

int l2tpWithIPSecLACTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
	nBytesSent += boaWrite(wp,	"<th align=center width=\"5%\">overIPSec</th>\n");
#endif
	return nBytesSent;
}

#ifdef CONFIG_USER_L2TPD_LNS
int l2tpServerList(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	MIB_VPN_ACCOUNT_T Entry;
	unsigned int entryNum, i;
	unsigned char password[MAX_NAME_LEN];

	entryNum = mib_chain_total(MIB_VPN_ACCOUNT_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&Entry))
		{
			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

		if (VPN_PPTP == Entry.type)
			continue;

		memset(password, 0, sizeof(password));
		rtk_util_convert_to_star_string(password,strlen(Entry.password));
		password[MAX_NAME_LEN-1]='\0';

		nBytesSent += boaWrite(wp, "<tr>"
			"<td align=center width=\"3%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><input type=\"checkbox\" name=\"sel%d\" value=1></td>\n"
			"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
			"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><input type=\"checkbox\" name=\"en%d\" %s value=1></td>\n"
			"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
			"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
			"</tr>",
			Entry.idx,
			Entry.name, Entry.idx, Entry.enable?"checked":"",
			Entry.username, password);
	}
	return nBytesSent;
}
#endif
#endif //end of CONFIG_USER_L2TPD_L2TPD

#ifdef CONFIG_XFRM
#define	CONFIG_DIR	"/var/config"
#define IPSECCERT_FNAME	CONFIG_DIR"/cert.pem"
#define IPSECKEY_FNAME	CONFIG_DIR"/privKey.pem"

#define UPLOAD_MSG(url) { \
	boaHeader(wp); \
	boaWrite(wp, "<body><blockquote><h4>Upload a file successfully!" \
                "<form><input type=button value=\"  OK  \" OnClick=window.location.replace(\"%s\")></form></blockquote></body>", url);\
	boaFooter(wp); \
	boaDone(wp, 200); \
}

//copy from fmmgmt.c
//find the start and end of the upload file.
FILE *uploadGetCertFile(request *wp, unsigned int *startPos, unsigned *endPos)
{
	FILE *fp=NULL;
	struct stat statbuf;
	unsigned char *buf;
	int c;
	char boundary[80];


	if (wp->method == M_POST)
	{
		int i;

		if(fstat(wp->post_data_fd, &statbuf) != 0)
			printf("fstat failed: %s %d\n", __func__, __LINE__);
		lseek(wp->post_data_fd, 0, SEEK_SET);

		printf("file size=%ld\n", statbuf.st_size);
		fp=fopen(wp->post_file_name, "rb");
		if(fp==NULL) goto error_without_close;

		memset(boundary, 0, sizeof(boundary));
		if( fgets(boundary, 80, fp)==NULL ) goto error;
		if( boundary[0]!='-' || boundary[1]!='-') goto error;

		i= strlen( boundary ) - 1;
		while( boundary[i]=='\r' || boundary[i]=='\n' )
		{
			boundary[i]='\0';
			i--;
		}
		printf( "boundary=%s\n", boundary );
	}
	else goto error_without_close;

   	//printf("_uploadGet\n");
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

	if(fseek(fp,statbuf.st_size-0x200,SEEK_SET)<0)
		goto error;

	do
	{
		if(feof(fp))
		{
			printf("Cannot find end of file\n");
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
        fclose(fp);
error_without_close:
   	return NULL;
}

#ifdef CONFIG_USER_STRONGSWAN
#include <pthread.h>
#define CONFIG_SWAN_DIR         "/var/config"
#define CONFIG_SWAN_KEY_DIR     CONFIG_SWAN_DIR"/ca.pem"
#define CONFIG_SWAN_CA_DIR	    CONFIG_SWAN_DIR"/ca.cert.pem"
#define IPSECCERT_SWAN_FNAME	"/var/swanctl/x509ca/cacert.pem"
#define IPSECKEY_SWAN_FNAME	    "/var/swanctl/private/ca.pem"

void formIPSecSwanCert(request * wp, char *path, char *query)
{
    char *strData;
    char tmpBuf[100];
    FILE *fp=NULL,*fp_input;
    unsigned char *buf;
    unsigned int startPos,endPos,nLen,nRead;
    if ((fp = uploadGetCertFile(wp, &startPos, &endPos)) == NULL)
    {
        strcpy(tmpBuf, "Upload error!");
        goto setErr_ipseccert;
    }

    nLen = endPos - startPos;
    //printf("filesize is %d\n", nLen);
    buf = malloc(nLen+1);
    if (!buf)
    {
        strcpy(tmpBuf, "Malloc failed!");
        goto setErr_ipseccert;
    }

    fseek(fp, startPos, SEEK_SET);
    nRead = fread((void *)buf, 1, nLen, fp);
    buf[nRead]=0;
    if (nRead != nLen)
        printf("Read %d bytes, expect %d bytes\n", nRead, nLen);

    //printf("write to %d bytes from %08x\n", nLen, buf);

    fp_input=fopen(CONFIG_SWAN_CA_DIR,"w");
    if (!fp_input)
    {
        strcpy(tmpBuf, "create cert file fail!");
        goto setErr_ipseccert;
    }

    fprintf(fp_input,buf);
    printf("create file %s\n", CONFIG_SWAN_CA_DIR);
    free(buf);
    fclose(fp_input);

    strData = boaGetVar(wp, "submit-url", "/ipsec_swan.asp");
    UPLOAD_MSG(strData);// display reconnect msg to remote
    return;

setErr_ipseccert:
    ERR_MSG(tmpBuf);
}

void formIPSecSwanKey(request * wp, char *path, char *query)
{
    char *strData;
    char tmpBuf[100];
    FILE *fp=NULL,*fp_input;
    unsigned char *buf;
    unsigned int startPos,endPos,nLen,nRead;
    if ((fp = uploadGetCertFile(wp, &startPos, &endPos)) == NULL)
    {
        strcpy(tmpBuf, "Upload error!");
        goto setErr_ipseckey;
    }

    nLen = endPos - startPos;
    //printf("filesize is %d\n", nLen);
    buf = malloc(nLen+1);
    if (!buf)
    {
        strcpy(tmpBuf, "Malloc failed!");
        goto setErr_ipseckey;
    }

    fseek(fp, startPos, SEEK_SET);
    nRead = fread((void *)buf, 1, nLen, fp);
    buf[nRead]=0;
    if (nRead != nLen)
        printf("Read %d bytes, expect %d bytes\n", nRead, nLen);

    //printf("write to %d bytes from %08x\n", nLen, buf);

    fp_input=fopen(CONFIG_SWAN_KEY_DIR,"w");
    if (!fp_input)
    {
        strcpy(tmpBuf, "create key file fail!");
        goto setErr_ipseckey;
    }
    fprintf(fp_input,buf);
    printf("create file %s\n", CONFIG_SWAN_KEY_DIR);
    free(buf);
    fclose(fp_input);

    strData = boaGetVar(wp, "submit-url", "/ipsec_swan.asp");
    UPLOAD_MSG(strData);// display reconnect msg to remote
    return;

setErr_ipseckey:
    ERR_MSG(tmpBuf);
}

void formIPSecSwanGenCert(request * wp, char *path, char *query)
{

	char *strVal = NULL;
	char *submitUrl = NULL;
	char tmpcmd[256];
	char CountryCA[32];
	char OrganizationCA[32];

	strVal = boaGetVar(wp, "Country", "");
	if(strVal[0]){
		memset(CountryCA, 0, sizeof(CountryCA));
		strncpy(CountryCA, strVal, sizeof(CountryCA) -1);
	}

	strVal = boaGetVar(wp, "Organization", "");
	if(strVal[0]){
		memset(OrganizationCA, 0, sizeof(OrganizationCA));
		strncpy(OrganizationCA, strVal, sizeof(OrganizationCA) -1);
	}

	system("cp -f /var/config/ca.pem /var/swanctl/private");
	system("cp -f /var/config/ca.cert.pem /var/swanctl/x509ca");

	//first generate local.priv.pem,then use local.priv.pem to generate local.cacert.pem
	system("rm -f /var/swanctl/private/local.priv.pem");
	system("rm -f /var/swanctl/x509/local.cacert.pem");
	system("ipsec pki --gen --outform pem > /var/swanctl/private/local.priv.pem");
	sprintf(tmpcmd,"ipsec pki --pub --in /var/swanctl/private/local.priv.pem | ipsec pki --issue --cacert /var/config/ca.cert.pem --cakey /var/config/ca.pem --dn \"C=%s, O=%s, CN=client\" --outform pem > /var/swanctl/x509/local.cacert.pem", CountryCA, OrganizationCA);
	system(tmpcmd);

	ipsec_strongswan_take_effect();
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;
}
static void _fillIPsecEntryInfo(request * wp, MIB_IPSEC_SWAN_Tp pEntry)
{
	char *strVal;

	strVal = boaGetVar(wp, "Enable", "");
	if(strVal[0]){
		pEntry->Enable = 1;
	}
	else{
		pEntry->Enable = 0;
	}

	strVal = boaGetVar(wp, "encapl2tp", "");
	if(strVal[0]){
		pEntry->encapl2tp = 1;
	}
	else{
		pEntry->encapl2tp = 0;
	}

	strVal = boaGetVar(wp, "IPSecType", "");
	if(strVal[0]){
		pEntry->IPSecType = strVal[0]-'0';
	}

	strVal = boaGetVar(wp, "RemoteSubnet", "");
	if(strVal[0]){
		strncpy(pEntry->RemoteSubnet, strVal, IPSEC_SUBNET_CNT_MAX*IPSEC_SUBNET_LEN -1);
	}

	strVal = boaGetVar(wp, "LocalSubnet", "");
	if(strVal[0]){
		strncpy(pEntry->LocalSubnet, strVal, IPSEC_SUBNET_CNT_MAX*IPSEC_SUBNET_LEN -1);
	}

	strVal = boaGetVar(wp, "RemoteIP", "");
	if(strVal[0]){
		strncpy(pEntry->RemoteIP, strVal, IPSEC_IP_LEN -1);
	}

	strVal = boaGetVar(wp, "RemoteDomain", "");
	if(strVal[0]){
		strncpy(pEntry->RemoteDomain, strVal, IPSEC_NAME_LEN -1);
	}

	strVal = boaGetVar(wp, "IPSecOutInterface", "");
	if(strVal[0]){
		pEntry->IPSecOutInterface = atoi(strVal);
	}

	strVal = boaGetVar(wp, "IPSecEncapsulationMode", "");
	if(strVal[0]){
		pEntry->IPSecEncapsulationMode = strVal[0]-'0';
	}

	//IKE Phase 1
	strVal = boaGetVar(wp, "IKEVersion", "");
	if(strVal[0]){
		pEntry->IKEVersion = strVal[0]-'0';
	}

	strVal = boaGetVar(wp, "ExchangeMode", "");
	if(strVal[0]){
		pEntry->ExchangeMode = strVal[0]-'0';
	}

	strVal = boaGetVar(wp, "IKEAuthenticationAlgorithm", "");
	if(strVal[0]){
		pEntry->IKEAuthenticationAlgorithm = strVal[0]-'0';
	}

	strVal = boaGetVar(wp, "IKEAuthenticationMethod", "");
	if(strVal[0]){
		pEntry->IKEAuthenticationMethod = strVal[0]-'0';
	}

	strVal = boaGetVar(wp, "IKEEncryptionAlgorithm", "");
	if(strVal[0]){
		pEntry->IKEEncryptionAlgorithm = strVal[0]-'0';
	}

	strVal = boaGetVar(wp, "IKEDHGroup", "");
	if(strVal[0]){
		pEntry->IKEDHGroup = strVal[0]-'0';
	}

	strVal = boaGetVar(wp, "IKEIDType", "");
	if(strVal[0]){
		pEntry->IKEIDType = strVal[0]-'0';
	}

	strVal = boaGetVar(wp, "IKELocalName", "");
	if(strVal[0]){
		strncpy(pEntry->IKELocalName, strVal, IPSEC_NAME_LEN -1);
	}

	strVal = boaGetVar(wp, "IKERemoteName", "");
	if(strVal[0]){
		strncpy(pEntry->IKERemoteName, strVal, IPSEC_NAME_LEN -1);
	}

	strVal = boaGetVar(wp, "encodepskValue", "");
	if (strVal[0]) {
			if(strVal[0]){
				rtk_util_data_base64decode(strVal, pEntry->IKEPreshareKey, sizeof(pEntry->IKEPreshareKey));
				pEntry->IKEPreshareKey[sizeof(pEntry->IKEPreshareKey)-1] = '\0';
			}
	}

	strVal = boaGetVar(wp, "IKESAPeriod", "");
	if(strVal[0]){
		pEntry->IKESAPeriod = atoi(strVal);
	}

	//IKE Phase 2
	strVal = boaGetVar(wp, "IPSecTransform", "");
	if(strVal[0]){
		pEntry->IPSecTransform = strVal[0]-'0';
	}

	strVal = boaGetVar(wp, "ESPAuthenticationAlgorithm", "");
	if(strVal[0]){
		pEntry->ESPAuthenticationAlgorithm = strVal[0]-'0';
	}

	strVal = boaGetVar(wp, "ESPEncryptionAlgorithm", "");
	if(strVal[0]){
		pEntry->ESPEncryptionAlgorithm = strVal[0]-'0';
	}

	strVal = boaGetVar(wp, "AHAuthenticationAlgorithm", "");
	if(strVal[0]){
		pEntry->AHAuthenticationAlgorithm = strVal[0]-'0';
	}

	strVal = boaGetVar(wp, "IPSecPFS", "");
	if(strVal[0]){
		pEntry->IPSecPFS = strVal[0]-'0';
	}

	strVal = boaGetVar(wp, "IPSecSATimePeriod", "");
	if(strVal[0]){
		pEntry->IPSecSATimePeriod = atoi(strVal);
	}

	strVal = boaGetVar(wp, "IPSecSATrafficPeriod", "");
	if(strVal[0]){
		pEntry->IPSecSATrafficPeriod = atoi(strVal);
	}

	//DPD
	strVal = boaGetVar(wp, "DPDEnable", "");
	if(strVal[0]){
		pEntry->DPDEnable = 1;
	}
	else{
		pEntry->DPDEnable = 0;
	}

	strVal = boaGetVar(wp, "DPDThreshold", "");
	if(strVal[0]){
		pEntry->DPDThreshold = atoi(strVal);
	}

	strVal = boaGetVar(wp, "DPDRetry", "");
	if(strVal[0]){
		pEntry->DPDRetry = atoi(strVal);
	}

	pEntry->ConnectionStatus = IPSEC_CONN_STATUS_DISCONNECTED;

	return;
}

void formIPsecStrongswan(request * wp, char *path, char *query)
{
	char *strVal;
	char *oprate;
	char *submitUrl;
	int negoType = 0, intVal;
	long netmask;
	char tmpBuf[100];
	struct in_addr ipAddr;
	int i, index, entryNum;
	MIB_IPSEC_SWAN_T entry, orgEntry;
    int found = 0;

	oprate = boaGetVar(wp, "OperatorStyle", "");
	memset(&entry, 0, sizeof(MIB_IPSEC_SWAN_T));

	strVal = boaGetVar(wp, "entrys", "");
	index = atoi(strVal);

	if(0 == strcmp(oprate, "add")){
		_fillIPsecEntryInfo(wp, &entry);
		mib_chain_add(MIB_IPSEC_SWAN_TBL, (void *)&entry);
		goto setOK_IPsec;
	}
	else if(0 == strcmp(oprate, "modify")){
		if(index >= MAX_IPSEC_NUM) goto setErr_IPsec;
		mib_chain_get(MIB_IPSEC_SWAN_TBL, index, (void *)&orgEntry);
		strncpy(entry.IKEPreshareKey, orgEntry.IKEPreshareKey, sizeof(entry.IKEPreshareKey));
		_fillIPsecEntryInfo(wp, &entry);
		mib_chain_update(MIB_IPSEC_SWAN_TBL, &entry, index);
		goto setOK_IPsec;
	}
	else if(0 == strcmp(oprate, "delete")){
		if(index >= MAX_IPSEC_NUM) goto setErr_IPsec;
		if(mib_chain_delete(MIB_IPSEC_SWAN_TBL, index) != 1) {
			strcpy(tmpBuf, Tdelete_chain_error);
			goto setErr_IPsec;
		}
		goto setOK_IPsec;
	}

setOK_IPsec:

	ipsec_strongswan_take_effect();

// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;

setErr_IPsec:
	ERR_MSG(tmpBuf);
}

int ipsecSwanConfList(int eid, request * wp, int argc, char **argv){
	int i, entryNum;

	entryNum = mib_chain_total(MIB_IPSEC_SWAN_TBL);

	boaWrite(wp, "<option value=\"999\" selected>Add IPSec VPN</option>\n", i, i);

	for (i=0; i<entryNum; i++){
		boaWrite(wp, "<option value=\"%d\">entry_%d</option>\n", i, i);
	}

	return 0;
}

int ipsecSwanWanList(int eid, request * wp, int argc, char **argv){
	int i, entryNum;
	char wanname[MAX_WAN_NAME_LEN];
	MIB_CE_ATM_VC_T entry;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);

	for(i = 0; i < entryNum; i++){
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry)){
			continue;
		}

		getWanName(&entry, wanname);

		boaWrite(wp, "<option value=\"%d\" %s>%s</option>\n", entry.ifIndex, entry.dgw?"selected":"", wanname);
	}

	return 0;
}

#define _PTS			", new it(\"%s\", \"%s\")"
#define _PTI			", new it(\"%s\", %d)"
#define _PME(name)		#name, entry.name
int ipsecSwanConfDetail(int eid, request * wp, int argc, char **argv){
	MIB_IPSEC_SWAN_T entry;
	int i, entryNum;
	unsigned char IKEPreshareKey[IPSEC_NAME_LEN+1];

	//update status first
	update_ipsec_swan_status();

	entryNum = mib_chain_total(MIB_IPSEC_SWAN_TBL);

	for(i = 0; i < entryNum; i++){
		if (!mib_chain_get(MIB_IPSEC_SWAN_TBL, i, (void *)&entry)){
			goto end;
		}

		memset(IKEPreshareKey, 0, sizeof(IKEPreshareKey));
		rtk_util_convert_to_star_string(IKEPreshareKey,strlen(entry.IKEPreshareKey));
		IKEPreshareKey[IPSEC_NAME_LEN]='\0';

		boaWrite(wp, "push(new it_nr(\"entry_%d\""\
				_PTI _PTI _PTS _PTS\
				_PTS _PTS _PTI \
				_PTI _PTI _PTI _PTI \
				_PTI _PTI _PTI \
				_PTI _PTS _PTS _PTS\
				_PTI _PTI _PTI \
				_PTI _PTI _PTI \
				_PTI _PTI _PTI \
				_PTI _PTI _PTI _PTI\
				"));\n",
				i, _PME(Enable),_PME(IPSecType),_PME(RemoteSubnet),_PME(LocalSubnet),
				_PME(RemoteIP),_PME(RemoteDomain),_PME(IPSecOutInterface),
				_PME(IPSecEncapsulationMode),_PME(IKEVersion),_PME(ExchangeMode),_PME(IKEAuthenticationAlgorithm),
				_PME(IKEAuthenticationMethod),_PME(IKEEncryptionAlgorithm),_PME(IKEDHGroup),
				_PME(IKEIDType),_PME(IKELocalName),_PME(IKERemoteName),"IKEPreshareKey", IKEPreshareKey,
				_PME(IKESAPeriod),_PME(IPSecTransform),_PME(ESPAuthenticationAlgorithm),
				_PME(ESPEncryptionAlgorithm),_PME(AHAuthenticationAlgorithm),_PME(IPSecPFS),
				_PME(IPSecSATimePeriod),_PME(IPSecSATrafficPeriod),_PME(DPDEnable),
				_PME(DPDThreshold),_PME(DPDRetry),_PME(ConnectionStatus), _PME(encapl2tp));
	}

end:
	return 0;
}

#else
void formIPsec(request * wp, char *path, char *query)
{
	char *strVal;
	char *addConf, *deleteConf, *enable, *disable, *refresh;
	char *submitUrl;
	int negoType = 0, intVal;
	long netmask;
	char tmpBuf[100];
	int i, entryNum;
	MIB_IPSEC_T Entry;
	int found = 0;
	
	addConf = boaGetVar(wp, "saveConf", "");
	deleteConf = boaGetVar(wp, "delConf", "");
	enable = boaGetVar(wp, "enableConf", "");
	disable = boaGetVar(wp, "disableConf", "");
	memset(&Entry, 0, sizeof(MIB_IPSEC_T));
	if(addConf[0]){
		strVal = boaGetVar(wp, "negoType", "");
		if(strVal[0] == '1')
			Entry.negotiationType= 1;  //0:IKE; 1:Manual

		strVal = boaGetVar(wp, "transMode", ""); //0-tunnel; 1-transport;
		Entry.transportMode= strVal[0] - '0';

#ifdef CONFIG_IPV6_VPN
		strVal = boaGetVar(wp, "IpProtocolType", "");
		if (strVal[0]) {
			Entry.IpProtocol = strVal[0] - '0';
		}
#endif

		strVal = boaGetVar(wp, "rtunnelAddr", "");
		if(strVal[0]) {
			snprintf(Entry.remoteTunnel, sizeof(Entry.remoteTunnel), "%s", strVal);
		}

		strVal = boaGetVar(wp, "remoteip", "");
		if (strVal[0]) {
			snprintf(Entry.remoteIP, sizeof(Entry.remoteIP), "%s", strVal);
		}
		
		strVal = boaGetVar(wp, "remotemask", "");
		if (strVal[0]) {
			Entry.remoteMaskLen=atoi(strVal);
		}
		
		strVal = boaGetVar(wp, "ltunnelAddr", "");
		if (strVal[0]) {
			snprintf(Entry.localTunnel, sizeof(Entry.localTunnel), "%s", strVal);
		}

		strVal = boaGetVar(wp, "localip", "");
		if (strVal[0]) {
			snprintf(Entry.localIP, sizeof(Entry.localIP), "%s", strVal);
		}

		strVal = boaGetVar(wp, "localmask", "");
		if (strVal[0]) {
			Entry.localMaskLen=atoi(strVal);
		}

		strVal = boaGetVar(wp, "encapsType", "");
			Entry.encapMode = strVal[0] - '0';

		strVal = boaGetVar(wp, "filterProtocol", "");
			Entry.filterProtocol= strVal[0] - '0';

		if(Entry.filterProtocol==1 || Entry.filterProtocol==2){
			strVal = boaGetVar(wp, "filterPort", "");
			if (strVal[0]) {		
				sscanf(strVal, "%d", &Entry.filterPort);
			}
		}

		if(Entry.negotiationType ==0){
			//IKE mode
			strVal = boaGetVar(wp, "ikeAuthMethod", "");
			Entry.auth_method = strVal[0] - '0';

			if(0 == Entry.auth_method)
			{
				strVal = boaGetVar(wp, "psk", "");
				if(strlen(strVal)>128){
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, multilang(LANG_PSK_LENGTH_ERROR_TOO_LONG),sizeof(tmpBuf)-1); //length error, too long!
					goto setErr_IPsec;
				}
				strncpy(Entry.psk, strVal, 130);
			}

			strVal = boaGetVar(wp, "negoMode", "");
			Entry.ikeMode = strVal[0] - '0';

			strVal = boaGetVar(wp, "ikeAliveTime", "");
			if (strVal[0]) {		
				sscanf(strVal, "%d", &Entry.ikeAliveTime);
			}

			strVal = boaGetVar(wp, "ikeProposal0", "");
			Entry.ikeProposal[0]= strVal[0] - '0';

			strVal = boaGetVar(wp, "ikeProposal1", "");
			Entry.ikeProposal[1]= strVal[0] - '0';
			
			strVal = boaGetVar(wp, "ikeProposal2", "");
			Entry.ikeProposal[2]= strVal[0] - '0';
			
			strVal = boaGetVar(wp, "ikeProposal3", "");
			Entry.ikeProposal[3]= strVal[0] - '0';

			strVal = boaGetVar(wp, "dhArray_p2", "");			
			Entry.pfs_group = 1<<(strVal[0] - '0');
			
			if(Entry.encapMode != 2)        //ESP or ESP+AH
			{
				strVal = boaGetVar(wp, "encrypt_p2", "");
				if (strVal[0]) {		
					sscanf(strVal, "%d", &Entry.encrypt_group);
				}
			}
			else
			{
				Entry.encrypt_group = 1;    //none_enc for AH only
			}
			
			strVal = boaGetVar(wp, "auth_p2", "");
			if (strVal[0]) {		
				sscanf(strVal, "%d", &Entry.auth_group);
			}

			strVal = boaGetVar(wp, "saAliveTime", "");
			if (strVal[0]) {		
				sscanf(strVal, "%d", &Entry.saAliveTime);
			}

			strVal = boaGetVar(wp, "saAliveByte", "");
			if (strVal[0]) {		
				sscanf(strVal, "%d", &Entry.saAliveByte);
			}
		}else{
			//manual mode	
			if(Entry.encapMode%2 == 1){
				strVal = boaGetVar(wp, "esp_e_algo", "");
				Entry.espEncrypt = strVal[0] - '0';
				
				strVal = boaGetVar(wp, "esp_ekey", "");
				Entry.espEncryptKey[sizeof(Entry.espEncryptKey)-1]='\0';
				strncpy(Entry.espEncryptKey, strVal,sizeof(Entry.espEncryptKey)-1);

				strVal = boaGetVar(wp, "esp_a_algo", "");
				Entry.espAuth= strVal[0] - '0';

				strVal = boaGetVar(wp, "esp_akey", "");
				Entry.espAuthKey[sizeof(Entry.espAuthKey)-1]='\0';
				strncpy(Entry.espAuthKey, strVal,sizeof(Entry.espAuthKey)-1);

				strVal = boaGetVar(wp, "spi_out_esp", "");
				if (strVal[0]) {		
					sscanf(strVal, "%d", &Entry.espOUTSPI);
				}

				strVal = boaGetVar(wp, "spi_in_esp", "");
				if (strVal[0]) {		
					sscanf(strVal, "%d", &Entry.espINSPI);
				}
			}
			
			if(Entry.encapMode/2 == 1){
				strVal = boaGetVar(wp, "ah_algo", "");
				Entry.ahAuth= strVal[0] - '0';

				strVal = boaGetVar(wp, "ah_key", "");
				Entry.ahAuthKey[sizeof(Entry.ahAuthKey)-1]='\0';
				strncpy(Entry.ahAuthKey, strVal,sizeof(Entry.ahAuthKey)-1);

				strVal = boaGetVar(wp, "spi_out_ah", "");
				if (strVal[0]) {		
					sscanf(strVal, "%d", &Entry.ahOUTSPI);
				}

				strVal = boaGetVar(wp, "spi_in_ah", "");
				if (strVal[0]) {		
					sscanf(strVal, "%d", &Entry.ahINSPI);
				}
			}
		}

		Entry.enable = Entry.state = 1;
		intVal = mib_chain_add(MIB_IPSEC_TBL, (void *)&Entry);
		if (intVal == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tadd_chain_error,sizeof(tmpBuf)-1);
			goto setErr_IPsec;
		}
		else if (intVal == -1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
			goto setErr_IPsec;
		}

		ipsec_take_effect();
		goto setOK_IPsec;
	}
	if(deleteConf[0]){
		//delete a configuration
		entryNum = mib_chain_total(MIB_IPSEC_TBL);	
		for (i=entryNum-1; i>=0; i--) {
			if(mib_chain_get(MIB_IPSEC_TBL, i, (void *)&Entry) == 0)
				printf("\n %s %d\n", __func__, __LINE__);
			snprintf(tmpBuf, 100, "row_%d", i);
			strVal = boaGetVar(wp, tmpBuf, "");

			if (strVal[0] == '1') {
                found = 1;
                if(Entry.state)
                {
                    Entry.state = 0;
                    apply_nat_rule(&Entry);
                }
				if(mib_chain_delete(MIB_IPSEC_TBL, i) != 1) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
					goto setErr_IPsec;
				}
			}
		}
        if(found)
		ipsec_take_effect();
		goto setOK_IPsec;
	}
	if(enable[0]){
		//enable a configuration
		entryNum = mib_chain_total(MIB_IPSEC_TBL);	
		for (i=0; i<entryNum; i++) {
			if(mib_chain_get(MIB_IPSEC_TBL, i, (void *)&Entry) == 0)
				printf("\n %s %d\n", __func__, __LINE__);
			snprintf(tmpBuf, 100, "row_%d", i);
			strVal = boaGetVar(wp, tmpBuf, "");

			if (strVal[0] == '1') {
				if(Entry.enable == 0){
                    found = 1;
					Entry.enable = 1;
					mib_chain_update(MIB_IPSEC_TBL, &Entry, i);
				}
			}
		}
        if(found)
		ipsec_take_effect();
		goto setOK_IPsec;
	}
	if(disable[0]){
		//disable a configuration
		entryNum = mib_chain_total(MIB_IPSEC_TBL);	
		for (i=entryNum-1; i>=0; i--) {
			if(mib_chain_get(MIB_IPSEC_TBL, i, (void *)&Entry) == 0)
				printf("\n %s %d\n", __func__, __LINE__);
			snprintf(tmpBuf, 100, "row_%d", i);
			strVal = boaGetVar(wp, tmpBuf, "");

			if (strVal[0] == '1') {
				if(Entry.enable == 1){
                    found = 1;
					Entry.state = Entry.enable = 0;
					mib_chain_update(MIB_IPSEC_TBL, &Entry, i);
				}
			}
		}
        if(found)
		ipsec_take_effect();
		goto setOK_IPsec;
	}

setOK_IPsec:

// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;

setErr_IPsec:
	ERR_MSG(tmpBuf);
}

void formIPSecCert(request * wp, char *path, char *query)
{
    char *strData;
    char tmpBuf[100];
    FILE *fp=NULL,*fp_input;
    unsigned char *buf;
    unsigned int startPos,endPos,nLen,nRead;
    if ((fp = uploadGetCertFile(wp, &startPos, &endPos)) == NULL)
    {
    	tmpBuf[sizeof(tmpBuf)-1]='\0';
        strncpy(tmpBuf, "Upload error!",sizeof(tmpBuf)-1);
        goto setErr_ipseccert;
    }

    nLen = endPos - startPos;
    //printf("filesize is %d\n", nLen);
    buf = malloc(nLen+1);
    if (!buf)
    {
    	tmpBuf[sizeof(tmpBuf)-1]='\0';
        strncpy(tmpBuf, "Malloc failed!",sizeof(tmpBuf)-1);
        fclose(fp);
        goto setErr_ipseccert;
    }

    if(fseek(fp, startPos, SEEK_SET) != 0)
        printf("fseek failed: %s %d\n", __func__, __LINE__);
    nRead = fread((void *)buf, 1, nLen, fp);
    buf[nRead]=0;
    if (nRead != nLen)
        printf("Read %d bytes, expect %d bytes\n", nRead, nLen);

    //printf("write to %d bytes from %08x\n", nLen, buf);

    fp_input=fopen(IPSECCERT_FNAME,"w");
    if (!fp_input)
    {
    	tmpBuf[sizeof(tmpBuf)-1]='\0';
        strncpy(tmpBuf, "create cert file fail!",sizeof(tmpBuf)-1);
        free(buf);
        fclose(fp);
        goto setErr_ipseccert;
    }

    fprintf(fp_input,buf);
    printf("create file %s\n", IPSECCERT_FNAME);
    free(buf);
    fclose(fp_input);

#ifdef CONFIG_USER_FLATFSD_XXX
    if( va_cmd( "/bin/flatfsd",1,1,"-s" ) )
        printf( "[%d %d]:exec 'flatfsd -s' error!",__FILE__, __LINE__);
#endif

    strData = boaGetVar(wp, "submit-url", "/ipsec.asp");
    UPLOAD_MSG(strData);// display reconnect msg to remote
    fclose(fp);
    return;

setErr_ipseccert:
    ERR_MSG(tmpBuf);
}


void formIPSecKey(request * wp, char *path, char *query)
{
    char *strData;
    char tmpBuf[100];
    FILE *fp=NULL,*fp_input;
    unsigned char *buf;
    unsigned int startPos,endPos,nLen,nRead;
    if ((fp = uploadGetCertFile(wp, &startPos, &endPos)) == NULL)
    {
    	tmpBuf[sizeof(tmpBuf)-1]='\0';
        strncpy(tmpBuf, "Upload error!",sizeof(tmpBuf)-1);
        goto setErr_ipseckey;
    }

    nLen = endPos - startPos;
    //printf("filesize is %d\n", nLen);
    buf = malloc(nLen+1);
    if (!buf)
    {
    	tmpBuf[sizeof(tmpBuf)-1]='\0';
        strncpy(tmpBuf, "Malloc failed!",sizeof(tmpBuf)-1);
        fclose(fp);
        goto setErr_ipseckey;
    }

    if(fseek(fp, startPos, SEEK_SET) != 0)
        printf("fseek failed: %s %d\n", __func__, __LINE__);
    nRead = fread((void *)buf, 1, nLen, fp);
    buf[nRead]=0;
    if (nRead != nLen)
        printf("Read %d bytes, expect %d bytes\n", nRead, nLen);

    //printf("write to %d bytes from %08x\n", nLen, buf);

    fp_input=fopen(IPSECKEY_FNAME,"w");
    if (!fp_input)
    {
    	tmpBuf[sizeof(tmpBuf)-1]='\0';
        strncpy(tmpBuf, "create key file fail!",sizeof(tmpBuf)-1);
        free(buf);
        fclose(fp);
        goto setErr_ipseckey;
    }
    fprintf(fp_input,buf);
    printf("create file %s\n", IPSECKEY_FNAME);
    free(buf);
    fclose(fp_input);

#ifdef CONFIG_USER_FLATFSD_XXX
    if( va_cmd( "/bin/flatfsd",1,1,"-s" ) )
        printf( "[%d]:exec 'flatfsd -s' error!",__FILE__ );
#endif

    strData = boaGetVar(wp, "submit-url", "/ipsec.asp");
    UPLOAD_MSG(strData);// display reconnect msg to remote
    fclose(fp);
    return;

setErr_ipseckey:
    ERR_MSG(tmpBuf);
}


int ipsec_wanList(int eid, request * wp, int argc, char **argv){
	int nBytesSent=0;
	int wanMode;
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char ifName[16];
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	wanMode = GetWanMode();
	
	for(i=0; i<entryNum; i++){
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)){
			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}
		if(((!(wanMode&1))&&MEDIA_INDEX(Entry.ifIndex)==MEDIA_ATM)
		  #ifdef CONFIG_PTMWAN
			||((!(wanMode&1))&&MEDIA_INDEX(Entry.ifIndex)==MEDIA_PTM)
		  #endif /*CONFIG_PTMWAN*/
			||((!(wanMode&2))&&MEDIA_INDEX(Entry.ifIndex)==MEDIA_ETH))
			continue;
		getDisplayWanName((void *)&Entry, ifName);
		nBytesSent += boaWrite(wp,
			"<option value=\"%s\">%s</option>",
			Entry.WanName, ifName);
	}
	return nBytesSent;
}

int ipsec_ikePropList(int eid, request * wp, int argc, char **argv){
	int nBytesSent=0;
	int i;

	for(i=0; ikeProps[i].valid!=0; i++){
		nBytesSent += boaWrite(wp,	
			"<option value=\"%d\">%s</option>",
			i, ikeProps[i].name);
	}
	return 0;
}

int ipsec_encrypt_p2List(int eid, request * wp, int argc, char **argv){
	int nBytesSent=0;
	int i;

	for(i=0; curr_encryptAlgm[i]!=NULL; i++){
		nBytesSent += boaWrite(wp,			
			"<input type=\"checkbox\" name=\"%s\" id=\"%s\" value=\"1\" %s onClick=\"encrypt_p2_change('%s', %d)\"/>%s&nbsp;",			
			curr_encryptAlgm[i], curr_encryptAlgm[i], i!=0?"checked":"", curr_encryptAlgm[i], i, curr_encryptAlgm[i]);	
	}
	return 0;
}

int ipsec_auth_p2List(int eid, request * wp, int argc, char **argv){
	int nBytesSent=0;
	int i;

	for(i=0; curr_authAlgm[i]!=NULL; i++){
		nBytesSent += boaWrite(wp,			
			"<input type=\"checkbox\" name=\"%s\" id=\"%s\" value=\"1\" %s onClick=\"auth_p2_change('%s', %d)\"/>%s&nbsp;",			
			curr_authAlgm[i], curr_authAlgm[i], i!=0?"checked":"", curr_authAlgm[i], i, curr_authAlgm[i]);	
	}
	return 0;
}

int ipsecAlgoScripts(int eid, request * wp, int argc, char **argv){
	int nBytesSent=0;
	int i;

	int dh_all=0, encrypt_all=0, auth_all=0;

	for(i=1; curr_dhArray[i]!=NULL; i++){
		dh_all |= (1<<i);
	}

	for(i=1; curr_encryptAlgm[i]!=NULL; i++){
		encrypt_all |= (1<<i);
	}

	for(i=1; curr_authAlgm[i]!=NULL; i++){
		auth_all |= (1<<i);
	}

	nBytesSent += boaWrite(wp, "dh_algo_p2 = %d;\n", dh_all);
	nBytesSent += boaWrite(wp, "encrypt_algo_p2 = %d;\n", encrypt_all);
	nBytesSent += boaWrite(wp, "auth_algo_p2 = %d;\n", auth_all);

	return 0;
}


int ipsec_infoList(int eid, request * wp, int argc, char **argv){
	int nBytesSent=0;
	char *strVal;
	MIB_IPSEC_T Entry;
	int i, intVal, entryNum, encap = 0;
	char *enableStr[2]={"Disable", "Enable"};
	char *stateStr[2]={"Not Apply", "Applyed"};
	char *negoType[2]={"AUTO", "Manual"};
	char *encapStr[3]={"ESP", "AH", "ESP+AH"};
	char *transportMode[2]={"tunnel", "transport"};
	char *filterProtocol[4]={"any", "tcp", "udp", "icmp"};
	char filterPort[10];

	entryNum = mib_chain_total(MIB_IPSEC_TBL);
	for (i=0; i<entryNum; i++){
		if (!mib_chain_get(MIB_IPSEC_TBL, i, (void *)&Entry)){
			boaError(wp, 400, "Get chain record error!\n");	
			return -1;
		}
		
		if(Entry.filterPort != 0)
			snprintf(filterPort, 10, "%d", Entry.filterPort);
		else
			snprintf(filterPort, 10, "%s", "any");

		nBytesSent = boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
			"<td class=\"infoContent\"><input type=\"checkbox\" name=\"row_%d\" value=1></td>\n"
			"<td class=\"infoContent\">%s</td>\n"
			"<td class=\"infoContent\">%s</td>\n"
			"<td class=\"infoContent\">%s</td>\n"
			"<td class=\"infoContent\">%s</td>\n"
			"<td class=\"infoContent\">%s</td>\n"
			"<td class=\"infoContent\">%s</td>\n"
			"<td class=\"infoContent\">%s</td>\n"
			"<td class=\"infoContent\">%s</td>\n"
			"<td class=\"infoContent\">%s</td>\n"
			"<td class=\"infoContent\">%s</td>\n"
			"</tr>",
#else
			"<td><input type=\"checkbox\" name=\"row_%d\" value=1></td>\n"
			"<td>%s</td>\n"
			"<td>%s</td>\n"
			"<td>%s</td>\n"
			"<td>%s</td>\n"
			"<td>%s</td>\n"
			"<td>%s</td>\n"
			"<td>%s</td>\n"
			"<td>%s</td>\n"
			"<td>%s</td>\n"
			"<td>%s</td>\n"
			"</tr>",
#endif
			i, enableStr[Entry.enable], stateStr[Entry.state], negoType[Entry.negotiationType], Entry.remoteTunnel, Entry.remoteIP, Entry.localTunnel, Entry.localIP,
			encapStr[Entry.encapMode-1], filterProtocol[Entry.filterProtocol], filterPort);
	}
	return 0;
}

#endif
#endif

#ifdef CONFIG_NET_IPIP
//max MAX_IPIP_NUM IPIP tunnels.
static unsigned char IPIP_STATUS[MAX_IPIP_NUM]={0};
int applyIPIP(MIB_IPIP_T *pentry, int enable);

void ipip_take_effect()
{
	MIB_IPIP_T entry;
	unsigned int entrynum, i;//, j;
	int enable;

	if ( !mib_get_s(MIB_IPIP_ENABLE, (void *)&enable, sizeof(enable)) )
		return;

	entrynum = mib_chain_total(MIB_IPIP_TBL);

	//delete all firstly
	for (i=0; i<entrynum; i++)
	{
		if ( !mib_chain_get(MIB_IPIP_TBL, i, (void *)&entry) )
			return;

		applyIPIP(&entry, 0);
	}

	if (enable) {
		for (i=0; i<entrynum; i++)
		{
			if ( !mib_chain_get(MIB_IPIP_TBL, i, (void *)&entry) )
				return;

			applyIPIP(&entry, 1);
		}
	}
}

int applyIPIP(MIB_IPIP_T *pentry, int enable)
{
	char *action;
	char local[20];
	char remote[20];
	char ifname[IFNAMSIZ];
	char nat_cmd[100];

	if (enable) {
		IPIP_STATUS[pentry->idx] = 1;
		action = "add";
	}
	else {
		IPIP_STATUS[pentry->idx] = 0;
		action = "del";
	}

	snprintf(local, 20, "%s", inet_ntoa(*((struct in_addr *)&pentry->saddr)));
	snprintf(remote, 20, "%s", inet_ntoa(*((struct in_addr *)&pentry->daddr)));
	ifGetName(pentry->ifIndex, ifname, sizeof(ifname));	// Mason Yu. Add VPN ifIndex

	if (!enable) {
		printf("%s:ifconfig %s down\n", __func__, ifname);
		va_cmd("/bin/ifconfig", 2, 1, ifname, "down");
	}

	printf("%s:ip tunnel %s %s mode ipip remote %s local %s\n", __func__, action, ifname, remote, local);
	va_cmd("/bin/ip", 9, 1, "tunnel", action, ifname, "mode", "ipip", "remote", remote, "local", local);

	if (enable) {
		printf("%s:ifconfig %s up\n", __func__, ifname);
		va_cmd("/bin/ifconfig", 2, 1, ifname, "up");

		/*if def gw enabled, then add new default route now. don't do SNAT for original packet.*/
		if (pentry->dgw) {
			printf("%s:route del default.\n", __func__);
			va_cmd("/bin/route", 2, 1, "del", "default");

			printf("%s:route add default %s\n", __func__, ifname);
			va_cmd("/bin/route", 3, 1, "add", "default", ifname);

#ifdef NEW_PORTMAPPING
			extern void modPolicyRouteTable(const char *pptp_ifname, struct in_addr *real_addr);
			modPolicyRouteTable(ifname, (struct in_addr *)&pentry->saddr);
#endif//endof NEW_PORTMAPPING

			sprintf(nat_cmd, "iptables -D FORWARD -o %s -p tcp --syn -j TCPMSS --set-mss 1480", ifname);
			printf("%s:%s\n", __func__, nat_cmd);
			system(nat_cmd);

			sprintf(nat_cmd, "iptables -I FORWARD 1 -o %sd -p tcp --syn -j TCPMSS --set-mss 1480", ifname);
			printf("%s:%s\n", __func__, nat_cmd);
			system(nat_cmd);
		}
	}

	return 1;
}

void formIPIP(request * wp, char *path, char *query)
{
	char *submitUrl, *strAddIPIP, *strDelIPIP, *strVal;
	char tmpBuf[100];
	int intVal;
	unsigned int ipEntryNum, i;
	MIB_IPIP_T entry, tmpEntry;
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	unsigned int l2tpEntryNum;
	MIB_L2TP_T l2tpEntry;
#endif//end of CONFIG_USER_L2TPD_L2TPD
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	unsigned int pptpEntryNum;
	MIB_PPTP_T pptpEntry;
#endif//end of CONFIG_USER_PPTP_CLIENT_PPTP
	MIB_CE_ATM_VC_T vcEntry;
	unsigned int wanNum;
	int enable;
	int pid;
	unsigned int map=0;//maximum rule num is MAX_IPIP_NUM
	char ifname[IFNAMSIZ];

	strVal = boaGetVar(wp, "lst", "");

	// enable/disable IPIP
	if (strVal[0]) {
		strVal = boaGetVar(wp, "ipipen", "");

		if ( strVal[0] == '1' ) {//enable
			enable = 1;
		}
		else//disable
			enable = 0;

		mib_set(MIB_IPIP_ENABLE, (void *)&enable);

		ipip_take_effect();

		goto setOK_ipip;
	}

	strAddIPIP = boaGetVar(wp, "addIPIP", "");
	strDelIPIP = boaGetVar(wp, "delSel", "");

	memset(&entry, 0, sizeof(entry));
	ipEntryNum = mib_chain_total(MIB_IPIP_TBL); /* get chain record size */
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	l2tpEntryNum = mib_chain_total(MIB_L2TP_TBL); /* get chain record size */
#endif//end of CONFIG_USER_L2TPD_L2TPD
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	pptpEntryNum = mib_chain_total(MIB_PPTP_TBL); /* get chain record size */
#endif//end of CONFIG_USER_PPTP_CLIENT_PPTP
	wanNum = mib_chain_total(MIB_ATM_VC_TBL);

	/* Add new ipip entry */
	if (strAddIPIP[0]) {
		printf("add ipip entry.\n");

		for (i=0; i<ipEntryNum; i++)
		{
			if ( !mib_chain_get(MIB_IPIP_TBL, i, (void *)&tmpEntry) )
				continue;

			/*
			if ( !strcmp(tmpEntry.tunnel_name, entry.tunnel_name))
			{
				strcpy(tmpBuf, "ipip vpn interface has already exist!");
				goto setErr_ipip;
			}
			*/

			map |= (1<< tmpEntry.idx);
		}

		// Mason Yu. Add VPN ifIndex
		for (i=0; i<MAX_IPIP_NUM; i++) {
			if (!(map & (1<<i))) {
				entry.idx = i;
				break;
			}
		}
		entry.ifIndex = TO_IFINDEX(MEDIA_IPIP, 0xff, IPIP_INDEX(entry.idx));
		//printf("***** IPIP: entry.ifIndex=0x%x\n", entry.ifIndex);

		strVal = boaGetVar(wp, "remote", "");
		inet_aton(strVal, &entry.daddr);

		strVal = boaGetVar(wp, "local", "");
		inet_aton(strVal, &entry.saddr);

		strVal = boaGetVar(wp, "defaultgw", "");
		if (strVal[0]) {
			entry.dgw = 1;
			for (i=0; i<ipEntryNum; i++)
			{
				if ( !mib_chain_get(MIB_IPIP_TBL, i, (void *)&tmpEntry) )
					continue;

				if (tmpEntry.dgw)
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, multilang(LANG_DEFAULT_GW_HAS_ALREADY_EXIST),sizeof(tmpBuf)-1);
					goto setErr_ipip;
				}
			}

			//check if conflicts with wan interface setting
			for (i=0; i<wanNum; i++)
			{
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
					continue;
				if (vcEntry.dgw)
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, multilang(LANG_DEFAULT_GW_HAS_ALREADY_EXIST),sizeof(tmpBuf)-1);
					goto setErr_ipip;
				}
			}

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
			for (i=0; i<l2tpEntryNum; i++)
			{
				if (!mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tpEntry))
					continue;
				if (l2tpEntry.dgw)
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, multilang(LANG_DEFAULT_GW_HAS_ALREADY_EXIST_FOR_L2TP_VPN),sizeof(tmpBuf)-1);
					goto setErr_ipip;
				}
			}
#endif//end of CONFIG_USER_L2TPD_L2TPD
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
			for (i=0; i<pptpEntryNum; i++)
			{
				if (!mib_chain_get(MIB_PPTP_TBL, i, (void *)&pptpEntry))
					continue;
				if (pptpEntry.dgw)
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, multilang(LANG_DEFAULT_GW_HAS_ALREADY_EXIST_FOR_PPTP_VPN),sizeof(tmpBuf)-1);
					goto setErr_ipip;
				}
			}
#endif//end of CONFIG_USER_PPTP_CLIENT_PPTP
		}

		intVal = mib_chain_add(MIB_IPIP_TBL, (void *)&entry);
		if (intVal == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tadd_chain_error,sizeof(tmpBuf)-1);
			goto setErr_ipip;
		}
		else if (intVal == -1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
			goto setErr_ipip;
		}

		applyIPIP(&entry, 1);

		goto setOK_ipip;
	}

	/* Delete entry */
	if (strDelIPIP[0])
	{
		int i;
		unsigned int deleted = 0;

		printf("delete ipip entry(total %d).\n", ipEntryNum);
		for (i=ipEntryNum-1; i>=0; i--) {
			mib_chain_get(MIB_IPIP_TBL, i, (void *)&tmpEntry);
			snprintf(tmpBuf, 20, "s%d", tmpEntry.idx);
			strVal = boaGetVar(wp, tmpBuf, "");

			if (strVal[0] == '1') {
				deleted ++;
				applyIPIP(&tmpEntry, 0);
				if(mib_chain_delete(MIB_IPIP_TBL, i) != 1) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
					goto setErr_ipip;
				}
			}
		}

		if (deleted <= 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_THERE_IS_NO_ITEM_SELECTED_TO_DELETE),sizeof(tmpBuf)-1);
			goto setErr_ipip;
		}

		goto setOK_ipip;
	}

	for (i=0; i<ipEntryNum; i++) {
		mib_chain_get(MIB_IPIP_TBL, i, (void *)&tmpEntry);
		ifGetName(tmpEntry.ifIndex, ifname, sizeof(ifname));	// Mason Yu. Add VPN ifIndex
		snprintf(tmpBuf, 100, "submit%s", ifname);
		strVal = boaGetVar(wp, tmpBuf, "");
		if ( strVal[0] ) {
			applyIPIP(&tmpEntry, 0);
			if ( !strcmp(strVal, "Connect") )
				applyIPIP(&tmpEntry, 1);
		}
	}

setOK_ipip:

// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

#ifndef NO_ACTION
	pid = fork();
	if (pid) {
		waitpid(pid, NULL, 0);
	}
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
		exit(1);
	}
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;

setErr_ipip:
	ERR_MSG(tmpBuf);
}


int ipipList(int eid, request * wp, int argc, char **argv)
{
	MIB_IPIP_T Entry;
	unsigned int entryNum, i;
	char remote[20], local[20];
	int enable;
	int nBytesSent=0;
	char ifname[IFNAMSIZ];
	unsigned int ipipEnable;

	mib_get_s(MIB_IPIP_ENABLE, (void *)&ipipEnable, sizeof(ipipEnable));
	entryNum = mib_chain_total(MIB_IPIP_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_IPIP_TBL, i, (void *)&Entry))
		{
			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

		snprintf(local, 20, "%s", inet_ntoa(*((struct in_addr *)&Entry.saddr)));
		snprintf(remote, 20, "%s", inet_ntoa(*((struct in_addr *)&Entry.daddr)));
		ifGetName(Entry.ifIndex, ifname, sizeof(ifname));   // Mason Yu. Add VPN ifIndex

		if (ipipEnable) {
			nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
				"<td bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"s%d\" value=1></td>\n"
				"<td bgcolor=\"#C0C0C0\">%s</td>\n"
				"<td bgcolor=\"#C0C0C0\">%s</td>\n"
				"<td bgcolor=\"#C0C0C0\">%s</td>\n"
				"<td bgcolor=\"#C0C0C0\">%s</td>\n"
				//"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s ")
				"<td bgcolor=\"#C0C0C0\"> "
				"<input type=\"submit\" id=\"%s\" value=\"%s\" name=\"submit%s\"></td>\n"
				"</tr>",
#else
				"<td><input type=\"checkbox\" name=\"s%d\" value=1></td>\n"
				"<td>%s</td>\n"
				"<td>%s</td>\n"
				"<td>%s</td>\n"
				"<td>%s</td>\n"
				//"<td align=center width=\"5%%\"><font size=\"2\">%s ")
				"<td> "
				"<input class=\"inner_btn\" type=\"submit\" id=\"%s\" value=\"%s\" name=\"submit%s\"></td>\n"
				"</tr>",
#endif
				Entry.idx, ifname, local, remote,
				multilang(Entry.dgw ? LANG_ON : LANG_OFF), ifname,
				//IPIP_STATUS[Entry.idx]?"ON":"OFF",
				multilang(IPIP_STATUS[Entry.idx] ? LANG_DISCONNECT : LANG_CONNECT),
				ifname);
		}
		else {
			nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
				"<td bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"s%d\" value=1></td>\n"
				"<td bgcolor=\"#C0C0C0\">%s</td>\n"
				"<td bgcolor=\"#C0C0C0\">%s</td>\n"
				"<td bgcolor=\"#C0C0C0\">%s</td>\n"
				"<td bgcolor=\"#C0C0C0\">%s</td>\n"
				"<td bgcolor=\"#C0C0C0\">%s "
#else
				"<td><input type=\"checkbox\" name=\"s%d\" value=1></td>\n"
				"<td>%s</td>\n"
				"<td>%s</td>\n"
				"<td>%s</td>\n"
				"<td>%s</td>\n"
				"<td>%s "
#endif
				"</tr>",
				Entry.idx, ifname, local, remote,
				multilang(Entry.dgw ? LANG_ON : LANG_OFF), multilang(LANG_OFF));
		}

	}

	return nBytesSent;
}
#endif //end of CONFIG_NET_IPIP

#ifdef CONFIG_NET_IPGRE
int showGRETable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	unsigned int entryNum, i;
	MIB_GRE_T Entry;
	unsigned long int d,g,m;
	struct in_addr dest;
	struct in_addr gw;
	struct in_addr mask;
	char sdest[16], sgw[16];
	int status;
	char *statusStr=NULL;
	char VidStr[8]="";
	char upRateStr[8]="";
	char downRateStr[8]="";
	char keyStr[16]="";
	char dscpStr[5]="";
	
	entryNum = mib_chain_total(MIB_GRE_TBL);

#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr><font size=1>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"10%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>"
	"</font></tr>\n", multilang(LANG_SELECT), multilang(LANG_STATE), multilang(LANG_NAME), "EndPoint", "Back EndPoint",
	"Checksum", "Sequence", "DSCP", "Key", "Key value",
	"VLAN ID", "UP Rate", "Down Rate");
#else
	nBytesSent += boaWrite(wp, "<tr>"
	"<th align=center width=\"5%%\">%s</th>\n"
	"<th align=center width=\"5%%\">%s</th>"
	"<th align=center width=\"10%%\">%s</th>"
	"<th align=center width=\"20%%\">%s</th>"
	"<th align=center width=\"20%%\">%s</th>"
	"<th align=center width=\"5%%\">%s</th>"
	"<th align=center width=\"5%%\">%s</th>"
	"<th align=center width=\"5%%\">%s</th>"
	"<th align=center width=\"5%%\">%s</th>"
	"<th align=center width=\"5%%\">%s</th>"
	"<th align=center width=\"5%%\">%s</th>"
	"<th align=center width=\"5%%\">%s</th>"
	"<th align=center width=\"5%%\">%s</th>"
	"</tr>\n", multilang(LANG_SELECT), multilang(LANG_STATE), multilang(LANG_NAME), "EndPoint", "Back EndPoint",
	"Checksum", "Sequence", "DSCP", "Key", "Key value",
	"VLAN ID", "UP Rate", "Down Rate");
#endif

	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_GRE_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}
		strncpy(VidStr, "", sizeof(VidStr));
		strncpy(upRateStr, "", sizeof(upRateStr));
		strncpy(downRateStr, "", sizeof(downRateStr));
		
		nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"radio\" name=\"select\""
		" value=\"s%d\" onClick=postEntry(%d,'%s','%s','%s',%d,%d,%d,%d,%d,%d,%d,%d)></td>\n",
#else
		
		"<td align=center width=\"5%%\"><input type=\"radio\" name=\"select\""
		" value=\"s%d\" onClick=postEntry(%d,'%s','%s','%s',%d,%d,%d,%d,%d,%d,%d,%d)></td>\n",
#endif
		i, Entry.enable, Entry.name, Entry.greIpAddr1, Entry.greIpAddr2,
		Entry.csum_enable, Entry.seq_enable, Entry.key_enable, Entry.key_value, Entry.m_dscp,
		Entry.vlanid, Entry.upBandWidth, Entry.downBandWidth );
		
		if (Entry.vlanid)
			snprintf(VidStr, sizeof(VidStr), "%u", Entry.vlanid);
		
		if (Entry.upBandWidth)
			snprintf(upRateStr, sizeof(upRateStr), "%u", Entry.upBandWidth);
		
		if (Entry.downBandWidth)
			snprintf(downRateStr, sizeof(downRateStr), "%u", Entry.downBandWidth);
		
		if (Entry.key_value)
			snprintf(keyStr, sizeof(keyStr), "%u", Entry.key_value);
		
		if (Entry.m_dscp)
			snprintf(dscpStr, sizeof(dscpStr), "0x%x", Entry.m_dscp-1);
		
		nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
#else	
		"<td align=center width=\"5%%\">%s</td>"
		"<td align=center width=\"10%%\">%s</td>"
		"<td align=center width=\"20%%\">%s</td>"
		"<td align=center width=\"20%%\">%s</td>"
		"<td align=center width=\"5%%\">%s</td>"
		"<td align=center width=\"5%%\">%s</td>"
		"<td align=center width=\"5%%\">%s</td>"
		"<td align=center width=\"5%%\">%s</td>"
		"<td align=center width=\"5%%\">%s</td>"
		"<td align=center width=\"5%%\">%s</td>"
		"<td align=center width=\"5%%\">%s</td>"
		"<td align=center width=\"5%%\">%s</td>"
#endif
		"</tr>\n",		
		multilang(Entry.enable ? LANG_ENABLE : LANG_DISABLE), Entry.name, Entry.greIpAddr1, Entry.greIpAddr2, 
		multilang(Entry.csum_enable ? LANG_ENABLE : LANG_DISABLE), multilang(Entry.seq_enable ? LANG_ENABLE : LANG_DISABLE),
		dscpStr, multilang(Entry.key_enable ? LANG_ENABLE : LANG_DISABLE), keyStr,
		VidStr, upRateStr, downRateStr );		
	}
	
	return 0;
}

static void getGreEntry(request * wp, MIB_GRE_Tp pEntry)
{
	char *strVal, *saveConf, *tmp=NULL;	
	
	memset(pEntry, 0, sizeof(MIB_GRE_T));

	strVal = boaGetVar(wp, "grename", "");
	strncpy(pEntry->name, strVal, IFNAMSIZ);
	
	strVal = boaGetVar(wp, "tunnelEn", "");
	pEntry->enable = strVal[0] - '0';		
	
	strVal = boaGetVar(wp, "csumEn", "");
	pEntry->csum_enable = strVal[0] - '0';	
	
	strVal = boaGetVar(wp, "seqEn", "");
	pEntry->seq_enable = strVal[0] - '0';		
	
	strVal = boaGetVar(wp, "keyEn", "");
	pEntry->key_enable = strVal[0] - '0';	
	
	strVal = boaGetVar(wp, "keyValue", "");
	if (strVal[0]) {		
		sscanf(strVal, "%d", &pEntry->key_value);
	}
	
	strVal = boaGetVar(wp, "mdscp", "");
	if(strVal[0]) 
		pEntry->m_dscp = strtol(strVal, &tmp, 0);
	else
		pEntry->m_dscp = 0;
		
	strVal = boaGetVar(wp, "greIP1", "");
	if(strVal[0])
	{
		pEntry->greIpAddr1[sizeof(pEntry->greIpAddr1)-1]='\0';
		strncpy(pEntry->greIpAddr1, strVal,sizeof(pEntry->greIpAddr1)-1);
	}

	strVal = boaGetVar(wp, "greIP2", "");
	if (strVal[0])
	{
		pEntry->greIpAddr2[sizeof(pEntry->greIpAddr2)-1]='\0';
		strncpy(pEntry->greIpAddr2, strVal,sizeof(pEntry->greIpAddr2)-1);
	}
	
	strVal = boaGetVar(wp, "grevid", "");
	if (strVal[0]) {		
		sscanf(strVal, "%d", &pEntry->vlanid);
		pEntry->itfGroup = IFGROUP_NUM;		// If this is a VLAN GRE, not use untag_gre interface on br.
	}
	
	strVal = boaGetVar(wp, "uprate", "");
	if (strVal[0]) {		
		sscanf(strVal, "%d", &pEntry->upBandWidth);
	}
	
	strVal = boaGetVar(wp, "downrate", "");
	if (strVal[0]) {		
		sscanf(strVal, "%d", &pEntry->downBandWidth);
	}

}

void formGRE(request * wp, char *path, char *query)
{
	char *str, *submitUrl;
	char tmpBuf[100];		
	MIB_GRE_T Entry, orgEntry;	
	int intVal;	
	unsigned int totalEntry, i, idx;
	int selected=0;
	unsigned char totalbandwidthEn=0;	
	
	stopGreQoS();
	
	//totalbandwidthEn = 1;
	//if (!mib_set(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&totalbandwidthEn))
	//	goto setErr_gre;	
				
	totalEntry = mib_chain_total(MIB_GRE_TBL); /* get chain record size */
	// Remove
	str = boaGetVar(wp, "delgre", "");
	if (str[0]) {
		str = boaGetVar(wp, "select", "");
		if (str[0]) {
			for (i=0; i<totalEntry; i++) {
				idx = totalEntry-i-1;
				snprintf(tmpBuf, 4, "s%d", idx);
				//printf("tmpBuf(select) = %s idx=%d\n", tmpBuf, idx);

				if ( !gstrcmp(str, tmpBuf) ) {
					// delete old GRE Tunnel
					if (!mib_chain_get(MIB_GRE_TBL, idx, (void *)&orgEntry)) {
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, "Get GRE TBL error(Remove)",sizeof(tmpBuf)-1);
						goto setErr_gre;
					}			
					applyGRE(0, idx, 0);
					
					// delete from chain record
					if(mib_chain_delete(MIB_GRE_TBL, idx) != 1) {
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
						goto setErr_gre;
					}
				}
			} // end of for
		}		
		goto setOK_gre;
	}
	
	// Add
	str = boaGetVar(wp, "add", "");
	if (str[0]) {
		
		getGreEntry(wp, &Entry);		
		
		intVal = mib_chain_add(MIB_GRE_TBL, (unsigned char*)&Entry);
		if (intVal == 0) {			
			sprintf(tmpBuf, "%s (MIB_GRE_TBL)",Tadd_chain_error);
			goto setErr_gre;
		}
		else if (intVal == -1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
			goto setErr_gre;
		}
		applyGRE(1, totalEntry, 0);		
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
			// delete old GRE Tunnel
			if (!mib_chain_get(MIB_GRE_TBL, selected, (void *)&orgEntry)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, "Get GRE TBL error",sizeof(tmpBuf)-1);
				goto setErr_gre;
			}
			
			applyGRE(0, selected, 0);
			
			if (selected >= 0) {
				getGreEntry(wp, &Entry);
				if (Entry.vlanid != 0)
					Entry.itfGroup = IFGROUP_NUM;		// If this is a VLAN GRE, not use untag_gre interface on br.
				else if (Entry.vlanid == 0 && orgEntry.vlanid == 0)
					Entry.itfGroup = orgEntry.itfGroup;
				else if (Entry.vlanid == 0 && orgEntry.vlanid != 0)
					Entry.itfGroup = orgEntry.itfGroupVlan;
				
				Entry.itfGroupVlan = orgEntry.itfGroupVlan;
				strncpy(Entry.ssid, orgEntry.ssid, strlen(orgEntry.ssid));
				mib_chain_update(MIB_GRE_TBL, (void *)&Entry, selected);
				// Create modofied GRE Tunnel
				applyGRE(1, selected, 0);
			}
		}
	}
	
	// GRE setting
	str = boaGetVar(wp, "greSet", "");
	if (str[0]) {
		unsigned char greVal;

		str = boaGetVar(wp, "greEn", "");
		if (str[0] == '1')
			greVal = 1;
		else
			greVal = 0;	// default "off"
		if (!mib_set(MIB_GRE_ENABLE, (void *)&greVal)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, "SET_RIP_ERROR",sizeof(tmpBuf)-1);
			goto setErr_gre;
		}
		gre_take_effect(0);
	}

setOK_gre:	
	
// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

#ifndef NO_ACTION
	pid = fork();
	if (pid) {
		waitpid(pid, NULL, 0);
	}
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
		exit(1);
	}
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;

setErr_gre:
	ERR_MSG(tmpBuf);

}

#endif



/*-- System inlcude files --*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/wait.h>

#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "multilang.h"

#ifdef CONFIG_IPV6
static const char RADVD_NEW_CONF[] ="/var/radvd2.conf";
static const char KEYWORD1[]= "AdvManagedFlag";
static const char KEYWORD2[]= "AdvOtherConfigFlag";
static const char KEYWORD3[]= "MaxRtrAdvInterval";
static const char KEYWORD4[]= "MinRtrAdvInterval";

void formRadvdSetup(request * wp, char *path, char *query)
{
	char *str, *submitUrl, *strVal, *pStr;
	static char tmpBuf[100];
	unsigned char vChar;
	char mode;
	int radvdpid;
	unsigned char oriPrefixMode=0;
	unsigned char oriULAPrefixEnable=0;
	unsigned char pre_static_prefix_len[32]={0},pre_static_ipaddr[64]={0};
	PREFIX_V6_INFO_T prefixInfo;
	unsigned char ulaPrefixIp[IP6_ADDR_LEN]={0};
	unsigned char ip6Addr[IP6_ADDR_LEN]={0};
	char ulaPrefixIp_str[INET6_ADDRSTRLEN] = {0}, ulaPrefixLen_str[32]={0};
	unsigned int ulaPrefixLen=0;
	int idx = 0;

#ifndef NO_ACTION
	int pid;
#endif
	//clear old static ipv6
	mib_get_s( MIB_V6_RADVD_PREFIX_MODE, (void *)&oriPrefixMode, sizeof(oriPrefixMode));
	if(oriPrefixMode == RADVD_MODE_MANUAL){
		//read and delete all static ipv6 address
		memset(&prefixInfo, 0 ,sizeof(PREFIX_V6_INFO_T));
		mib_get_s(MIB_V6_PREFIX_IP, (void *)pre_static_ipaddr, sizeof(pre_static_ipaddr));
		mib_get_s(MIB_V6_PREFIX_LEN, (void *)pre_static_prefix_len, sizeof(pre_static_prefix_len));
		if(pre_static_ipaddr[0]){
			inet_pton(AF_INET6, pre_static_ipaddr, prefixInfo.prefixIP);
			prefixInfo.prefixLen = atoi(pre_static_prefix_len);

			//delete ipv6 radvd static address
			set_lan_ipv6_global_address(NULL,&prefixInfo,(char*)LANIF,0,0);
			//del radvd static prefix route for LAN route table(IP_ROUTE_LAN_TABLE)
			set_ipv6_lan_policy_route((char *)LANIF, (struct in6_addr *)&prefixInfo.prefixIP, prefixInfo.prefixLen, 0);
		}
	}
	
	//clear old ula
	mib_get_s( MIB_V6_ULAPREFIX_ENABLE, (void *)&oriULAPrefixEnable, sizeof(oriULAPrefixEnable));
	if(oriULAPrefixEnable){
		//read and delete all static ipv6 address
		memset(&prefixInfo, 0 ,sizeof(PREFIX_V6_INFO_T));
		mib_get_s(MIB_V6_ULAPREFIX, (void *)ulaPrefixIp_str, sizeof(ulaPrefixIp_str));
		mib_get_s(MIB_V6_ULAPREFIX_LEN, (void *)ulaPrefixLen_str, sizeof(ulaPrefixLen_str));
		if(ulaPrefixIp_str[0]){
			inet_pton(AF_INET6, ulaPrefixIp_str, prefixInfo.prefixIP);
			prefixInfo.prefixLen = atoi(ulaPrefixLen_str);
			//delete ipv6 radvd ula address
			set_lan_ipv6_global_address(NULL,&prefixInfo,(char*)LANIF,0,0);
			//del radvd ula prefix route for LAN route table(IP_ROUTE_LAN_TABLE)
			set_ipv6_lan_policy_route((char *)LANIF, (struct in6_addr *)&prefixInfo.prefixIP, prefixInfo.prefixLen, 0);
		}
	}

	strVal = boaGetVar(wp, "radvd_enable", "");
	if ( strVal[0] ) {
		vChar = strVal[0] - '0';
		mib_set( MIB_V6_RADVD_ENABLE, (void *)&vChar);
	}
	str = boaGetVar(wp, "MaxRtrAdvIntervalAct", "");
	//if (str[0]) {
		if ( !mib_set(MIB_V6_MAXRTRADVINTERVAL, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_radvd;
		}
	//}

	str = boaGetVar(wp, "MinRtrAdvIntervalAct", "");
	//if (str[0]) {
		if ( !mib_set(MIB_V6_MINRTRADVINTERVAL, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_radvd;
		}
	//}

	str = boaGetVar(wp, "AdvCurHopLimitAct", "");
	//if (str[0]) {
		if ( !mib_set(MIB_V6_ADVCURHOPLIMIT, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_radvd;
		}
	//}

	str = boaGetVar(wp, "AdvDefaultLifetimeAct", "");
	//if (str[0]) {
		if ( !mib_set(MIB_V6_ADVDEFAULTLIFETIME, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_radvd;
		}
	//}

	str = boaGetVar(wp, "AdvReachableTimeAct", "");
	//if (str[0]) {
		if ( !mib_set(MIB_V6_ADVREACHABLETIME, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_radvd;
		}
	//}

	str = boaGetVar(wp, "AdvRetransTimerAct", "");
	//if (str[0]) {
		if ( !mib_set(MIB_V6_ADVRETRANSTIMER, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_radvd;
		}
	//}

	str = boaGetVar(wp, "AdvLinkMTUAct", "");
	//if (str[0]) {
		if ( !mib_set(MIB_V6_ADVLINKMTU, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_radvd;
		}
	//}

	strVal = boaGetVar(wp, "AdvSendAdvertAct", "");
	if ( strVal[0] ) {
		vChar = strVal[0] - '0';
		mib_set( MIB_V6_SENDADVERT, (void *)&vChar);
	}
	strVal = boaGetVar(wp, "AdvManagedFlagAct", "");
	if ( strVal[0] ) {
		vChar = strVal[0] - '0';
		mib_set( MIB_V6_MANAGEDFLAG, (void *)&vChar);
		if(vChar) { //enable , M flag, disable autonomous
			vChar = 0;
			mib_set(MIB_V6_AUTONOMOUS, &vChar);
		} 
		else {
			vChar = 1;
			mib_set(MIB_V6_AUTONOMOUS, &vChar);
		}
	}

	strVal = boaGetVar(wp, "AdvOtherConfigFlagAct", "");
	if ( strVal[0] ) {
		vChar = strVal[0] - '0';
		mib_set( MIB_V6_OTHERCONFIGFLAG, (void *)&vChar);
	}

	strVal = boaGetVar(wp, "EnableULA", "");
	if ( strVal[0] ) {
		vChar = strVal[0] - '0';
		mib_set( MIB_V6_ULAPREFIX_ENABLE, (void *)&vChar);
	}

	str = boaGetVar(wp, "ULAPrefixlen", "");
	if (str[0]) {
		if ( !mib_set(MIB_V6_ULAPREFIX_LEN, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_radvd;
		}
		ulaPrefixLen=atoi(str);
	}

	str = boaGetVar(wp, "ULAPrefixRandom", "");
	if ( !gstrcmp(str, "ON"))
		vChar = 1;
	else
		vChar = 0;

	if ( !mib_set(MIB_V6_ULAPREFIX_RANDOM, (void *)&vChar)) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
		goto setErr_radvd;
	}

	if (vChar){
		ulaPrefixIp[0]=0xfd;
		//ULA prefix use pseudo-random algorithm
		for (idx=1;idx<=5;idx++){
			struct timeval tpstart;

			gettimeofday(&tpstart,NULL);
			srand(tpstart.tv_usec);
			ulaPrefixIp[idx] = rand() & 0x000000ff;
		}
	}else{
		str = boaGetVar(wp, "ULAPrefix", "");
		if ((str[0] && tolower(str[0])=='f')
		    &&(str[1] && tolower(str[1])=='d')){
			inet_pton(PF_INET6, str, ip6Addr);
			ip6toPrefix(ip6Addr,ulaPrefixLen,ulaPrefixIp);
		}else{
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_radvd;
		}
	}
	inet_ntop(PF_INET6,ulaPrefixIp, ulaPrefixIp_str, sizeof(ulaPrefixIp_str));

	if ( !mib_set(MIB_V6_ULAPREFIX, (void *)ulaPrefixIp_str)) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
		goto setErr_radvd;
	}

	str = boaGetVar(wp, "ULAPrefixValidTime", "");
	if (str[0]) {
		if ( !mib_set(MIB_V6_ULAPREFIX_VALID_TIME, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_radvd;
		}
	}

	str = boaGetVar(wp, "ULAPrefixPreferedTime", "");
	if (str[0]) {
		if ( !mib_set(MIB_V6_ULAPREFIX_PREFER_TIME, (void *)str)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
			goto setErr_radvd;
		}
	}

	strVal = boaGetVar(wp, "RDNSS1", "");
	mib_set( MIB_V6_RDNSS1, (void *)strVal);

	strVal = boaGetVar(wp, "RDNSS2", "");
	mib_set( MIB_V6_RDNSS2, (void *)strVal);

	strVal = boaGetVar(wp, "PrefixEnable", "");
	if ( strVal[0] ) {
		vChar = strVal[0] - '0';
		mib_set( MIB_V6_PREFIX_ENABLE, (void *)&vChar);
	}
	pStr = boaGetVar(wp, "PrefixMode", "");
	if (pStr[0]) {
		if (pStr[0] == '1') {
			mode = 1;    // Manual

			str = boaGetVar(wp, "prefix_ip", "");
			if (str[0]) {
				if ( !mib_set(MIB_V6_PREFIX_IP, (void *)str)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
					goto setErr_radvd;
				}
			}

			str = boaGetVar(wp, "prefix_len", "");
			if (str[0]) {
				if ( !mib_set(MIB_V6_PREFIX_LEN, (void *)str)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
					goto setErr_radvd;
				}
			}

			str = boaGetVar(wp, "AdvValidLifetimeAct", "");
			//if (str[0]) {
				if ( !mib_set(MIB_V6_VALIDLIFETIME, (void *)str)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
					goto setErr_radvd;
				}
			//}

			str = boaGetVar(wp, "AdvPreferredLifetimeAct", "");
			//if (str[0]) {
				if ( !mib_set(MIB_V6_PREFERREDLIFETIME, (void *)str)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tset_mib_error,sizeof(tmpBuf)-1);
					goto setErr_radvd;
				}
			//}

			strVal = boaGetVar(wp, "AdvOnLinkAct", "");
			if ( strVal[0] ) {
				vChar = strVal[0] - '0';
				mib_set( MIB_V6_ONLINK, (void *)&vChar);
			}

			strVal = boaGetVar(wp, "AdvAutonomousAct", "");
			if ( strVal[0] ) {
				vChar = strVal[0] - '0';
				mib_get( MIB_V6_MANAGEDFLAG, (void *)&vChar);
				if(vChar) { //enable , M flag, disable autonomous
					vChar = 0;
					mib_set(MIB_V6_AUTONOMOUS, &vChar);
				} 
				else {
					vChar = 1;
					mib_set(MIB_V6_AUTONOMOUS, &vChar);
				}
			}

	   	} else {
	   		mode = 0;     // Auto
	   	}
	} else {
		mode = 0;            // Auto
	}
	mib_set(MIB_V6_RADVD_PREFIX_MODE, (void *)&mode);

	/* retrieve RDNSS mode */
	if(mode != RADVD_MODE_MANUAL){
		/* RDNSS mode can be RADVD_RDNSS_HGWPROXY(0) or RADVD_RDNSS_WANCON(1) if prefix mode is auto. */
		str = boaGetVar(wp, "RDNSSMode", "");
		if (str[0]){
			vChar = str[0] - '0';
			mib_set( MIB_V6_RADVD_RDNSS_MODE, (void *)&vChar);
		}
	}
	else{
		/* RDNSS mode is RADVD_RDNSS_STATIC(2) if prefix mode is static. */
		mode = 2;
		mib_set(MIB_V6_RADVD_RDNSS_MODE, (void *)&mode);
	}

	// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	radvdpid = read_pid((char *)RADVD_PID);
	restartRadvd();
	/* Reset ipv6filter chain for IP/Port filter */
	restart_IPV6Filter();
	restart_default_dhcpv6_server();
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

setErr_radvd:
	ERR_MSG(tmpBuf);
}
#endif // #ifdef CONFIG_IPV6

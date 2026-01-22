/*
 *      Web server handler routines for DHCP Server stuffs
 *      Authors: Kaohj	<kaohj@realtek.com.tw>
 *
 */


/*-- System inlcude files --*/
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

/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "multilang.h"
#include <rtk/utility.h>
#include "subr_net.h"
#ifdef CONFIG_RTK_DNS_TRAP
#include "sockmark_define.h"
#endif
#ifdef __i386__
#define _LITTLE_ENDIAN_
#endif


/*-- Macro declarations --*/
#ifdef _LITTLE_ENDIAN_
#define ntohdw(v) ( ((v&0xff)<<24) | (((v>>8)&0xff)<<16) | (((v>>16)&0xff)<<8) | ((v>>24)&0xff) )

#else
#define ntohdw(v) (v)
#endif

void formDhcpd(request * wp, char *path, char *query)
{
	char	*strDhcp, *submitUrl, *strIp;
	struct in_addr inIp, inMask, inGatewayIp;
#ifdef DHCPS_POOL_COMPLETE_IP
	struct in_addr inPoolStart, inPoolEnd, dhcpmask, ori_dhcpmask;
	char *str_dhcpmask;
#endif
	DHCPV4_TYPE_T dhcp, curDhcp;
	char tmpBuf[100];
#ifndef NO_ACTION
	int pid;
#endif
	unsigned char vChar;
//star: for dhcp change
	unsigned char origvChar;
	unsigned int origInt;
	char origstr[30];
	// char *origstrDomain=origstr;
	struct in_addr origGatewayIp;
	int dhcpd_changed_flag=0;
	int dhcpd_poll_change_flag = 0;
	char *strdhcpenable;
	unsigned char mode;
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	unsigned char value[64];
	int dhcpc_pid;
#endif
#ifdef RTK_SMART_ROAMING
	FILE *fp=NULL;
#endif
	int needReboot = 0;
	int change_dhcpd_mode;

	char	*strdhcpRangeStart, *strdhcpRangeEnd, *strLTime, *strDomain;
	memset(&inPoolStart,0,sizeof(struct in_addr));
	strdhcpenable = boaGetVar(wp, "dhcpdenable", "");
	mib_get_s( MIB_DHCP_MODE, (void *)&origvChar, sizeof(origvChar));

	if(strdhcpenable[0])
	{
		sscanf(strdhcpenable, "%u", &origInt);
		mode = (unsigned char)origInt;
		if(mode!=origvChar)
			dhcpd_changed_flag = 1;
		if ( !mib_set(MIB_DHCP_MODE, (void *)&mode)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
  			strncpy(tmpBuf, strSetDhcpModeerror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}
		change_dhcpd_mode = 1;
	}

	// Read current DHCP setting for reference later
	// Modified by Mason Yu for dhcpmode
	//if ( !mib_get_s( MIB_ADSL_LAN_DHCP, (void *)&vChar, sizeof(vChar)) ) {
	if ( !mib_get_s( MIB_DHCP_MODE, (void *)&vChar, sizeof(vChar)) ) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, strGetDhcpModeerror,sizeof(tmpBuf)-1);
		goto setErr_dhcpd;
	}
	curDhcp = (DHCPV4_TYPE_T) vChar;

	dhcp = curDhcp;

	if ( dhcp == DHCP_LAN_SERVER ) {
		// Get/Set DHCP client range
		unsigned int uVal, uLTime;
		unsigned char uStart, uEnd;

		// Kaohj
		#ifndef DHCPS_POOL_COMPLETE_IP
		strdhcpRangeStart = boaGetVar(wp, "dhcpRangeStart", "");
		if ( strdhcpRangeStart[0] ) {
			sscanf(strdhcpRangeStart, "%u", &uVal);
			uStart = (unsigned char)uVal;
		}
		strdhcpRangeEnd = boaGetVar(wp, "dhcpRangeEnd", "");
		if ( strdhcpRangeEnd[0] ) {
			sscanf(strdhcpRangeEnd, "%u", &uVal);
			uEnd = (unsigned char)uVal;
		}
		#else
		strdhcpRangeStart = boaGetVar(wp, "dhcpRangeStart", "");
		if ( strdhcpRangeStart[0] ) {
			if ( !inet_aton(strdhcpRangeStart, &inPoolStart) ) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetStarIperror,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
			}
		}
		strdhcpRangeEnd = boaGetVar(wp, "dhcpRangeEnd", "");
		if ( strdhcpRangeEnd[0] ) {
			if ( !inet_aton(strdhcpRangeEnd, &inPoolEnd) ) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetEndIperror,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
			}
		}
		#endif

		strLTime = boaGetVar(wp, "ltime", "");
		if ( strLTime[0] ) {
			sscanf(strLTime, "%u", &uLTime);
		}

#ifdef USE_DEONET /* sghong, 20230602 */
		uLTime *= 60;  /* from minutes to secs */
		if ((uLTime < 0) || (uLTime > (24 * 60 * 60)))
		{
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetLeaseTimeerror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}
#endif
		strDomain = boaGetVar(wp, "dname", "");

		if(!mib_get_s( MIB_ADSL_LAN_IP,  (void *)&inIp, sizeof(inIp))) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strGetIperror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

		if(!mib_get_s( MIB_ADSL_LAN_SUBNET,  (void *)&inMask, sizeof(inMask))) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strGetMaskerror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

		// Kaohj
		#ifndef DHCPS_POOL_COMPLETE_IP
		// update DHCP server config file
		if ( strdhcpRangeStart[0] && strdhcpRangeEnd[0] ) {
			unsigned char *ip, *mask;
			int diff;

			diff = (int) ( uEnd - uStart );
			ip = (unsigned char *)&inIp;
			mask = (unsigned char *)&inMask;
			if (diff <= 0 ||
				(ip[3]&mask[3]) != (uStart&mask[3]) ||
				(ip[3]&mask[3]) != (uEnd&mask[3]) ) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strInvalidRange,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
			}
		}
		#else
		// check the pool range
		if ( strdhcpRangeStart[0] && strdhcpRangeEnd[0] ) {
			//tylo, for single-PC DHCP
			//if (inPoolStart.s_addr >= inPoolEnd.s_addr) {
			if (ntohl(inPoolStart.s_addr) > ntohl(inPoolEnd.s_addr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strInvalidRange,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
			}
		}

		// Magician: Subnet mask for DHCP.
		str_dhcpmask = boaGetVar(wp, "dhcpSubnetMask", "");

		if(!inet_aton(str_dhcpmask, &dhcpmask))
		{
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_INVALID_SUBNET_MASK_VALUE),sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

		if (!isValidHostID(strdhcpRangeStart, str_dhcpmask)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_INVALID_IP_SUBNET_MASK_COMBINATION),sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

		if((inPoolStart.s_addr & dhcpmask.s_addr) != (inPoolEnd.s_addr & dhcpmask.s_addr))
		{
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, multilang(LANG_INVALID_DHCP_CLIENT_END_ADDRESSIT_SHOULD_BE_LOCATED_IN_THE_SAME_SUBNET_OF_CURRENT_IP_ADDRESS),sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}
		#endif

#ifdef IP_BASED_CLIENT_TYPE
		unsigned char pcstart,pcend,cmrstart,cmrend,stbstart,stbend,phnstart,phnend;

		//PC
		strdhcpRangeStart = boaGetVar(wp, "dhcppcRangeStart", "");
		if ( strdhcpRangeStart[0] ) {
			sscanf(strdhcpRangeStart, "%u", &uVal);
			pcstart = (unsigned char)uVal;
		}
		strdhcpRangeEnd = boaGetVar(wp, "dhcppcRangeEnd", "");
		if ( strdhcpRangeEnd[0] ) {
			sscanf(strdhcpRangeEnd, "%u", &uVal);
			pcend = (unsigned char)uVal;
		}
		if ( strdhcpRangeStart[0] && strdhcpRangeEnd[0] ) {
			unsigned char *ip, *mask;
			int diff;

			diff = (int) ( pcend - pcstart );
			ip = (unsigned char *)&inIp;
			mask = (unsigned char *)&inMask;
			if (diff <= 0 ||
				(ip[3]&mask[3]) != (pcstart&mask[3]) ||
				(ip[3]&mask[3]) != (pcend&mask[3]) ) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strInvalidRangepc,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
			}
		}
		//CMR
		strdhcpRangeStart = boaGetVar(wp, "dhcpcmrRangeStart", "");
		if ( strdhcpRangeStart[0] ) {
			sscanf(strdhcpRangeStart, "%u", &uVal);
			cmrstart = (unsigned char)uVal;
		}
		strdhcpRangeEnd = boaGetVar(wp, "dhcpcmrRangeEnd", "");
		if ( strdhcpRangeEnd[0] ) {
			sscanf(strdhcpRangeEnd, "%u", &uVal);
			cmrend = (unsigned char)uVal;
		}
		if ( strdhcpRangeStart[0] && strdhcpRangeEnd[0] ) {
			unsigned char *ip, *mask;
			int diff;

			diff = (int) ( cmrend - cmrstart );
			ip = (unsigned char *)&inIp;
			mask = (unsigned char *)&inMask;
			if (diff <= 0 ||
				(ip[3]&mask[3]) != (cmrstart&mask[3]) ||
				(ip[3]&mask[3]) != (cmrend&mask[3]) ) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strInvalidRangecmr,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
			}
		}
		//STB
		strdhcpRangeStart = boaGetVar(wp, "dhcpstbRangeStart", "");
		if ( strdhcpRangeStart[0] ) {
			sscanf(strdhcpRangeStart, "%u", &uVal);
			stbstart = (unsigned char)uVal;
		}
		strdhcpRangeEnd = boaGetVar(wp, "dhcpstbRangeEnd", "");
		if ( strdhcpRangeEnd[0] ) {
			sscanf(strdhcpRangeEnd, "%u", &uVal);
			stbend = (unsigned char)uVal;
		}
		if ( strdhcpRangeStart[0] && strdhcpRangeEnd[0] ) {
			unsigned char *ip, *mask;
			int diff;

			diff = (int) ( stbend - stbstart );
			ip = (unsigned char *)&inIp;
			mask = (unsigned char *)&inMask;
			if (diff <= 0 ||
				(ip[3]&mask[3]) != (stbstart&mask[3]) ||
				(ip[3]&mask[3]) != (stbend&mask[3]) ) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strInvalidRangestb,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
			}
		}
		//PHN
		strdhcpRangeStart = boaGetVar(wp, "dhcpphnRangeStart", "");
		if ( strdhcpRangeStart[0] ) {
			sscanf(strdhcpRangeStart, "%u", &uVal);
			phnstart = (unsigned char)uVal;
		}
		strdhcpRangeEnd = boaGetVar(wp, "dhcpphnRangeEnd", "");
		if ( strdhcpRangeEnd[0] ) {
			sscanf(strdhcpRangeEnd, "%u", &uVal);
			phnend = (unsigned char)uVal;
		}
		if ( strdhcpRangeStart[0] && strdhcpRangeEnd[0] ) {
			unsigned char *ip, *mask;
			int diff;

			diff = (int) ( phnend - phnstart );
			ip = (unsigned char *)&inIp;
			mask = (unsigned char *)&inMask;
			if (diff <= 0 ||
				(ip[3]&mask[3]) != (phnstart&mask[3]) ||
				(ip[3]&mask[3]) != (phnend&mask[3]) ) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strInvalidRangephn,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
			}
		}
		//check if the type ip pool out of ip pool range
		if((pcstart<uStart)||(cmrstart<uStart)||(stbstart<uStart)||(phnstart<uStart)
			||(pcend>uEnd)||(cmrend>uEnd)||(stbend>uEnd)||(phnend>uEnd)){
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strInvalidTypeRange,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
		}
		//check if the type ip pool overlap
		unsigned char ippool[4][2]={{pcstart,pcend},{cmrstart,cmrend},{stbstart,stbend},{phnstart,phnend}};
		unsigned char tmp1,tmp2;
		int i,j,min;
		for(i=0;i<4;i++)
		{
			min = i;
			for(j=i;j<4;j++)
			{
				if(ippool[j][0] < ippool[min][0])
					min = j;
			}
			if(min!=i){
				tmp1=ippool[i][0];
				tmp2=ippool[i][1];
				ippool[i][0]=ippool[min][0];
				ippool[i][1]=ippool[min][1];
				ippool[min][0]=tmp1;
				ippool[min][1]=tmp2;
			}
		}

		for(i=0;i<3;i++)
		{
			if(ippool[i][1]>=ippool[i+1][0]){
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strOverlapRange,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
			}
		}

		//set the type ip pool
		mib_get_s(CWMP_CT_PC_MINADDR, (void *)&origvChar, sizeof(origvChar));
		if(origvChar != pcstart)
			dhcpd_changed_flag = 1;
		if ( !mib_set(CWMP_CT_PC_MINADDR, (void *)&pcstart)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetPcStartIperror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

		mib_get_s(CWMP_CT_PC_MAXADDR, (void *)&origvChar, sizeof(origvChar));
		if(origvChar != pcend)
			dhcpd_changed_flag = 1;
		if ( !mib_set(CWMP_CT_PC_MAXADDR, (void *)&pcend)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetPcEndIperror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

		mib_get_s(CWMP_CT_CMR_MINADDR, (void *)&origvChar, sizeof(origvChar));
		if(origvChar != cmrstart)
			dhcpd_changed_flag = 1;
		if ( !mib_set(CWMP_CT_CMR_MINADDR, (void *)&cmrstart)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetCmrStartIperror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

		mib_get_s(CWMP_CT_CMR_MAXADDR, (void *)&origvChar, sizeof(origvChar));
		if(origvChar != cmrend)
			dhcpd_changed_flag = 1;
		if ( !mib_set(CWMP_CT_CMR_MAXADDR, (void *)&cmrend)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetCmrEndIperror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

		mib_get_s(CWMP_CT_STB_MINADDR, (void *)&origvChar, sizeof(origvChar));
		if(origvChar != stbstart)
			dhcpd_changed_flag = 1;
		if ( !mib_set(CWMP_CT_STB_MINADDR, (void *)&stbstart)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetStbStartIperror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

		mib_get_s(CWMP_CT_STB_MAXADDR, (void *)&origvChar, sizeof(origvChar));
		if(origvChar != stbend)
			dhcpd_changed_flag = 1;
		if ( !mib_set(CWMP_CT_STB_MAXADDR, (void *)&stbend)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetStbEndIperror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

		mib_get_s(CWMP_CT_PHN_MINADDR, (void *)&origvChar, sizeof(origvChar));
		if(origvChar != phnstart)
			dhcpd_changed_flag = 1;
		if ( !mib_set(CWMP_CT_PHN_MINADDR, (void *)&phnstart)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetPhnStartIperror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

		mib_get_s(CWMP_CT_PHN_MAXADDR, (void *)&origvChar, sizeof(origvChar));
		if(origvChar != phnend)
			dhcpd_changed_flag = 1;
		if ( !mib_set(CWMP_CT_PHN_MAXADDR, (void *)&phnend)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetPhnEndIperror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

#endif

		// Kaohj
		#ifndef DHCPS_POOL_COMPLETE_IP
		mib_get_s(MIB_ADSL_LAN_CLIENT_START, (void *)&origvChar, sizeof(origvChar));
		if(origvChar != uStart)
			dhcpd_changed_flag = 1;
		if ( !mib_set(MIB_ADSL_LAN_CLIENT_START, (void *)&uStart)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetStarIperror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

		mib_get_s(MIB_ADSL_LAN_CLIENT_END, (void *)&origvChar, sizeof(origvChar));
		if(origvChar != uEnd)
			dhcpd_changed_flag = 1;
		if ( !mib_set(MIB_ADSL_LAN_CLIENT_END, (void *)&uEnd)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetEndIperror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}
		#else
		mib_get_s(MIB_DHCP_POOL_START, (void *)&inIp, sizeof(inIp));
		if(inIp.s_addr != inPoolStart.s_addr)
		{
			dhcpd_changed_flag = 1;
			dhcpd_poll_change_flag = 1;
		}
		if ( !mib_set( MIB_DHCP_POOL_START, (void *)&inPoolStart)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetStarIperror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}
		mib_get_s(MIB_DHCP_POOL_END, (void *)&inIp, sizeof(inIp));
		if(inIp.s_addr != inPoolEnd.s_addr)
		{
			dhcpd_changed_flag = 1;
			dhcpd_poll_change_flag = 1;
		}
		if ( !mib_set( MIB_DHCP_POOL_END, (void *)&inPoolEnd)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetEndIperror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

		// Magician: Subnet mask for DHCP.
		mib_get_s(MIB_DHCP_SUBNET_MASK, (void *)&ori_dhcpmask, sizeof(ori_dhcpmask));
		if( ori_dhcpmask.s_addr != dhcpmask.s_addr )
			dhcpd_changed_flag = 1;

		if( !mib_set(MIB_DHCP_SUBNET_MASK, (void *)&dhcpmask))
		{
			sprintf(tmpBuf, " %s (DHCP subnetmask).",Tset_mib_error);
			goto setErr_dhcpd;
		}
		#endif

		mib_get_s(MIB_ADSL_LAN_DHCP_LEASE, (void *)&origInt, sizeof(origInt));
		if(origInt != uLTime)
		{
			dhcpd_changed_flag = 1;
			dhcpd_poll_change_flag = 1;
			needReboot = 1;
		}
		if ( !mib_set(MIB_ADSL_LAN_DHCP_LEASE, (void *)&uLTime)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetLeaseTimeerror,sizeof(tmpBuf)-1);
			goto setErr_dhcpd;
		}

		mib_get_s(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)origstr, sizeof(origstr));

		if ( !mib_set(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)strDomain)) {
						tmpBuf[sizeof(tmpBuf)-1]='\0';
                        strncpy(tmpBuf, strSetDomainNameerror,sizeof(tmpBuf)-1);
                        goto setErr_dhcpd;
                }

		if(strcmp(origstr, strDomain)!=0)
		{
			dhcpd_changed_flag = 1;

	#ifdef CONFIG_RTK_DNS_TRAP
			char cmdbuf[128] = {0},strDomain_tmp[MAX_NAME_LEN]={0};
			snprintf(strDomain_tmp,sizeof(strDomain_tmp), "%s",strDomain);
			strtolower(strDomain_tmp,strlen(strDomain_tmp));
			if(strchr(strDomain_tmp, '.')==NULL)
				strcat(strDomain_tmp,".com");
			create_hosts_file(strDomain_tmp);
			snprintf(cmdbuf, sizeof(cmdbuf), "echo \"%s\" > /proc/driver/realtek/domain_name",strDomain_tmp);
			system(cmdbuf);
	#ifndef CONFIG_RTK_SKB_MARK2
			snprintf(cmdbuf,sizeof(cmdbuf),"echo %d > /proc/fc/ctrl/skbmark_fwdByPs",SOCK_MARK_METER_INDEX_END);
			system(cmdbuf);
	#endif
	#endif

	#ifdef CONFIG_USER_LLMNR
			restartLlmnr();
	#endif
	#ifdef CONFIG_USER_MDNS
			restartMdns();
	#endif
	
		}

		// Added by Mason Yu for DHCP Server Gateway Address
		// Set Gateway address
		strIp = boaGetVar(wp, "ip", "");
		if ( strIp[0] ) {

	#ifndef DHCPS_POOL_COMPLETE_IP	
			if ( !inet_aton(strIp, &inGatewayIp) || !isValidHostID(strIp, inet_ntoa(inMask))) {
	#else
			if ( !inet_aton(strIp, &inGatewayIp) || !isValidHostID(strIp, str_dhcpmask)) {
	#endif
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strInvalidGatewayerror,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
			}
	
			mib_get_s(MIB_ADSL_LAN_DHCP_GATEWAY,(void*)&origGatewayIp, sizeof(origGatewayIp));
			if(origGatewayIp.s_addr != inGatewayIp.s_addr)
				dhcpd_changed_flag = 1;
			if ( !mib_set( MIB_ADSL_LAN_DHCP_GATEWAY, (void *)&inGatewayIp)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetGatewayerror,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
			}
		}
		// Kaohj
#ifdef DHCPS_DNS_OPTIONS /* sghong, 20231020 */
#if 0
		strDhcp[0] = 0;
		mib_get_s(MIB_DHCP_DNS_OPTION, (void *)&origvChar, sizeof(origvChar));
		if (origvChar != strDhcp[0])
			dhcpd_changed_flag = 1;
		mib_set(MIB_DHCP_DNS_OPTION, (void *)strDhcp);
#endif
#else
		strDhcp = boaGetVar(wp, "dhcpdns", "");
		strDhcp[0] -= '0';
		mib_get_s(MIB_DHCP_DNS_OPTION, (void *)&origvChar, sizeof(origvChar));
		if (origvChar != strDhcp[0])
			dhcpd_changed_flag = 1;
		mib_set(MIB_DHCP_DNS_OPTION, (void *)strDhcp);
		if (strDhcp[0] == 1) { // set manually
			strIp = boaGetVar(wp, "dns1", "");
			if ( !inet_aton(strIp, &inIp) ) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, Tinvalid_dns,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
			}
			mib_get_s(MIB_DHCPS_DNS1, (void *)&origGatewayIp, sizeof(origGatewayIp));
			if (origGatewayIp.s_addr != inIp.s_addr)
				dhcpd_changed_flag = 1;
			mib_set(MIB_DHCPS_DNS1, (void *)&inIp);
			inIp.s_addr = INADDR_NONE;
			strIp = boaGetVar(wp, "dns2", "");
			if (strIp[0]) {
				if ( !inet_aton(strIp, &inIp) ) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tinvalid_dns,sizeof(tmpBuf)-1);
					goto setErr_dhcpd;
				}
				mib_get_s(MIB_DHCPS_DNS2, (void *)&origGatewayIp, sizeof(origGatewayIp));
				if (origGatewayIp.s_addr != inIp.s_addr)
					dhcpd_changed_flag = 1;
				mib_set(MIB_DHCPS_DNS2, (void *)&inIp);
				inIp.s_addr = INADDR_NONE;
				strIp = boaGetVar(wp, "dns3", "");
				if (strIp[0]) {
					if ( !inet_aton(strIp, &inIp) ) {
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, Tinvalid_dns,sizeof(tmpBuf)-1);
						goto setErr_dhcpd;
					}
				}
				mib_get_s(MIB_DHCPS_DNS3, (void *)&origGatewayIp, sizeof(origGatewayIp));
				if (origGatewayIp.s_addr != inIp.s_addr)
					dhcpd_changed_flag = 1;
				mib_set(MIB_DHCPS_DNS3, (void *)&inIp);
			}
			else {
				mib_get_s(MIB_DHCPS_DNS2, (void *)&origGatewayIp, sizeof(origGatewayIp));
				if (origGatewayIp.s_addr != inIp.s_addr)
					dhcpd_changed_flag = 1;
				mib_set(MIB_DHCPS_DNS2, (void *)&inIp);
				mib_set(MIB_DHCPS_DNS3, (void *)&inIp);
			}
		}
#endif // of DHCPS_DNS_OPTIONS
	}
	else if( dhcp == DHCPV4_LAN_RELAY ){
		struct in_addr dhcps,origdhcps;
		char *str;

		str = boaGetVar(wp, "dhcps", "");
		if ( str[0] ) {
			if ( !inet_aton(str, &dhcps) ) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strInvalDhcpsAddress,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
			}
			mib_get_s(MIB_ADSL_WAN_DHCPS, (void*)&origdhcps, sizeof(origdhcps));
			if(origdhcps.s_addr != dhcps.s_addr)
				dhcpd_changed_flag = 1;
			if ( !mib_set(MIB_ADSL_WAN_DHCPS, (void *)&dhcps)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
	  			strncpy(tmpBuf, strSetDhcpserror,sizeof(tmpBuf)-1);
				goto setErr_dhcpd;
			}
		}
	}
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	snprintf(value, 64, "%s.%s", (char*)DHCPC_PID, ALIASNAME_BR0);
	dhcpc_pid = read_pid((char*)value);
	if (dhcpc_pid > 0)
		kill(dhcpc_pid, SIGTERM);

	if( dhcp == DHCPV4_LAN_CLIENT ){
		setupDHCPClient();
	}else{
		/* recover LAN IP address */
		restart_lanip();
	}
#endif
#ifdef RTK_SMART_ROAMING
	if(!(fp = fopen(CAPWAP_APP_DHCP_CONFIG, "w"))){
		sprintf(tmpBuf, "open %s file fail\n",CAPWAP_APP_DHCP_CONFIG);
		goto setErr_dhcpd;
	}

	if(mib_get_s( MIB_DHCP_MODE, (void *)&vChar, sizeof(vChar))){
		fprintf(fp, "%d", vChar);
	}

	fclose(fp);
#endif

	vChar = (unsigned char) dhcp;
	// Modify by Mason Yu for dhcpmode
	//if ( !mib_set(MIB_ADSL_LAN_DHCP, (void *)&vChar)) {
	if ( !mib_set(MIB_DHCP_MODE, (void *)&vChar)) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
  		strncpy(tmpBuf, strSetDhcpModeerror,sizeof(tmpBuf)-1);
		goto setErr_dhcpd;
	}
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

	if(dhcpd_changed_flag == 1)
	{
		char syslog_msg[128] = {0, };

		if (change_dhcpd_mode == 1)
		{
			if (mode == 0)
				syslog2(LOG_DM_HTTP, LOG_DM_HTTP_LAN_DHCP_CONF, "Network/LAN is configured (DHCP: Disable)");
			else
				syslog2(LOG_DM_HTTP, LOG_DM_HTTP_LAN_DHCP_CONF, "Network/LAN is configured (DHCP: Enable)");
		}

		if (dhcpd_poll_change_flag == 1)
		{
			sprintf(syslog_msg, "Network/LAN is configured (Start IP: %s End IP: %s) Leasetime %s", 
					strdhcpRangeStart, strdhcpRangeEnd, strLTime);

			syslog2(LOG_DM_HTTP, LOG_DM_HTTP_LAN_DHCP_IP_POOL_CONF, syslog_msg);
		}

		if(needReboot == 1)
		{
			syslog2(LOG_DM_HTTP, LOG_DM_HTTP_RESET, "Reboot by Web");
			SaveAndReboot(wp);
			return;
		}
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif

		restart_dhcp();
		submitUrl = boaGetVar(wp, "submit-url", "");
		OK_MSG(submitUrl);
	}
	else
	{
		submitUrl = boaGetVar(wp, "submit-url", "");
		if (submitUrl[0])
			boaRedirect(wp, submitUrl);
		else
			boaDone(wp, 200);
	}

	return;

setErr_dhcpd:
	ERR_MSG(tmpBuf);
}

/////////////////////////////////////////////////////////////////////////////
int dhcpClientList(int eid, request * wp, int argc, char **argv)
{
#ifdef EMBED
	char *name;

	struct stat status;
	int nBytesSent=0, isOnlyAp = 0;
	int entryNum=0,i=0, logID=0, if_total=1, br_found=0;
	MIB_CE_MAC_BASE_DHCP_T Entry;
	int element=0, ret;
	char ipAddr[40], macAddr[40], entryMac[40], liveTime[80], *buf=NULL, *ptr=NULL, ifname[IFNAMSIZ]={0}, ifname_str[IFNAMSIZ]={0};
	char br_ifname[IFNAMSIZ]={0}, hostName[80], ssid[40] = {0,};
	unsigned long fsize;
	unsigned char mac_addr[6];
	
	// lan
	// only ap		: hostname, ssid, ip, mac, port, time, button
	// all equip	:mac, ip, hostname, port, time
	
	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if ( !strcmp(name, "onlyAP") )
		isOnlyAp = 1;

	if(getDhcpClientLeasesDB(&buf, &fsize) <= 0)
		goto err;

	ptr = buf;
	entryNum = mib_chain_total(MIB_MAC_BASE_DHCP_TBL);
	while (1) {
		ret = getOneDhcpClientIncludeHostname(&ptr, &fsize, ipAddr, macAddr, liveTime, hostName);
		sscanf(macAddr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]);
#if defined(_SUPPORT_L2BRIDGING_PROFILE_) || defined(_SUPPORT_INTFGRPING_PROFILE_)
		if_total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
#endif
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

		if(isOnlyAp && !strstr(hostName, "/NAT/") && !strstr(hostName, "/BRI/"))
			continue;

		deo_ssid_read(macAddr, ssid);
		ssid[32] = '\0';
#if defined(CONFIG_RTK_DEV_AP)
		//check MIB_MAC_BASE_DHCP_TBL, set liveTime to -1(infinity) if mac matches.
		if(entryNum)
			for(i=0; i<entryNum; i++)
			{
				if (!mib_chain_get(MIB_MAC_BASE_DHCP_TBL, i, (void *)&Entry))
				{
  					printf("%s:Get chain(MIB_MAC_BASE_DHCP_TBL) record error!\n", __FUNCTION__);
					free(buf);
					return 0;
				}
				snprintf(entryMac, 20, "%02x:%02x:%02x:%02x:%02x:%02x",
					Entry.macAddr_Dhcp[0],Entry.macAddr_Dhcp[1],Entry.macAddr_Dhcp[2],
					Entry.macAddr_Dhcp[3],Entry.macAddr_Dhcp[4], Entry.macAddr_Dhcp[5]);
				if((strcmp(macAddr, entryMac)==0)&&(Entry.Enabled == 1))
				{
					//match
					snprintf(liveTime, 10, "%s", "--");
				}
					
			}
			printf("liveTime=%s\n", liveTime);
			if (strcmp(liveTime, "4294967295") ==0)
				snprintf(liveTime, 10, "%s", "--");
#endif

		hostName[33] = '\0';
		if(isOnlyAp)
			nBytesSent += boaWrite(wp,
					"<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td><center><input type='button' onClick=\"apconClickLogin('%s')\" value='%s' class='basic-btn01 btn-orange2-bg' /></center></td></tr>",
					hostName, ssid, ipAddr, macAddr,
					ifname_str, liveTime, ipAddr,multilang(LANG_CONNECT));
		else
			nBytesSent += boaWrite(wp,
					"<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",
					macAddr, ipAddr, hostName, ifname_str, liveTime);

		element++;
	}

err:
	if (element == 0) {
		nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
			"<tr bgcolor=#b7b7b7><td><font size=2>%s</td><td><font size=2>----</td><td><font size=2>----</td></tr>", multilang(LANG_NONE));
#else
			"<tr><td>%s</td><td>----</td><td>----</td></tr>", multilang(LANG_NONE));
#endif
	}
	if (buf)
		free(buf);

	return nBytesSent;
#else
	return 0;
#endif
}

int dhcpNicknameGet(char *mac, char *nickname)
{
	int i;
	int entryNum;
	MIB_NICKNAME_T Entry;

	entryNum = mib_chain_total(MIB_NICKNAME_TBL); /* get chain record size */

	for (i=0; i<entryNum; i++) 
	{
		if (!mib_chain_get(MIB_NICKNAME_TBL, i, (void *)&Entry))
		{
			printf("%s Get chain record error!\n", __func__);
			return -1;
		}

		if (strcmp(mac, Entry.clientMac) == 0)
		{
			strcpy(nickname, Entry.nickname);
			return 0;
		}
	}

	return 0;
}

int dhcpClientListWithNickname(int eid, request * wp, int argc, char **argv)
{
#ifdef EMBED
	char *name;

	struct stat status;
	int nBytesSent=0, isOnlyAp = 0;
	int entryNum=0,i=0, logID=0, if_total=1, br_found=0;
	MIB_CE_MAC_BASE_DHCP_T Entry;
	int element=0, ret;
	char ipAddr[40], macAddr[40], entryMac[40], liveTime[80], *buf=NULL, *ptr=NULL, ifname[IFNAMSIZ]={0}, ifname_str[IFNAMSIZ]={0};
	char br_ifname[IFNAMSIZ]={0}, hostName[80], ssid[40] = {0,};
	unsigned long fsize;
	unsigned char mac_addr[6];
	char nickname[60] = {0,};
	char *client_ip = NULL;
	
	// lan
	// only ap		: hostname, ssid, ip, mac, port, time, button
	// all equip	:mac, ip, hostname, port, time
	
	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if ( !strcmp(name, "onlyAP") )
		isOnlyAp = 1;

	if(getDhcpClientLeasesDB(&buf, &fsize) <= 0)
		goto err;

	ptr = buf;
	entryNum = mib_chain_total(MIB_MAC_BASE_DHCP_TBL);
	while (1) {
		ret = getOneDhcpClientIncludeHostname(&ptr, &fsize, ipAddr, macAddr, liveTime, hostName);
		sscanf(macAddr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]);
#if defined(_SUPPORT_L2BRIDGING_PROFILE_) || defined(_SUPPORT_INTFGRPING_PROFILE_)
		if_total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
#endif
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

		if(isOnlyAp && !strstr(hostName, "/NAT/") && !strstr(hostName, "/BRI/"))
			continue;

		deo_ssid_read(macAddr, ssid);
		ssid[32] = '\0';

#if defined(CONFIG_RTK_DEV_AP)
		//check MIB_MAC_BASE_DHCP_TBL, set liveTime to -1(infinity) if mac matches.
		if(entryNum)
			for(i=0; i<entryNum; i++)
			{
				if (!mib_chain_get(MIB_MAC_BASE_DHCP_TBL, i, (void *)&Entry))
				{
  					printf("%s:Get chain(MIB_MAC_BASE_DHCP_TBL) record error!\n", __FUNCTION__);
					free(buf);
					return 0;
				}
				snprintf(entryMac, 20, "%02x:%02x:%02x:%02x:%02x:%02x",
					Entry.macAddr_Dhcp[0],Entry.macAddr_Dhcp[1],Entry.macAddr_Dhcp[2],
					Entry.macAddr_Dhcp[3],Entry.macAddr_Dhcp[4], Entry.macAddr_Dhcp[5]);
				if((strcmp(macAddr, entryMac)==0)&&(Entry.Enabled == 1))
				{
					//match
					snprintf(liveTime, 10, "%s", "--");
				}
					
			}
			printf("liveTime=%s\n", liveTime);
			if (strcmp(liveTime, "4294967295") ==0)
				snprintf(liveTime, 10, "%s", "--");
#endif
			hostName[33] = '\0';

		if(isOnlyAp != 1)
		{
			memset(nickname, 0, sizeof(nickname));
			dhcpNicknameGet(macAddr, nickname);
		}

		if(isOnlyAp) {
			if (!strncmp(wp->remote_ip_addr, "::ffff:", 7))
				client_ip = wp->remote_ip_addr + 7;
			else
				client_ip = wp->remote_ip_addr;
			nBytesSent += boaWrite(wp,
                    "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td><center><input type='button' onClick=\"apconClickLogin('%s')\" value='%s' class='basic-btn01 btn-orange2-bg' /></center></td></tr>",
					hostName, ssid, ipAddr, macAddr,
					ifname_str, liveTime, ipAddr, multilang(LANG_CONNECT));
		} else
			nBytesSent += boaWrite(wp,
					"<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",
					macAddr, nickname, ipAddr, hostName, ifname_str, liveTime);
		element++;
	}

err:
	if (element == 0) {
		nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
			"<tr bgcolor=#b7b7b7><td><font size=2>%s</td><td><font size=2>----</td><td><font size=2>----</td></tr>", multilang(LANG_NONE));
#else
			"<tr><td>%s</td><td>----</td><td>----</td></tr>", multilang(LANG_NONE));
#endif
	}
	if (buf)
		free(buf);

	return nBytesSent;
#else
	return 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////
void formReflashClientTbl(request * wp, char *path, char *query)
{
	char *submitUrl, *str, *ap_ip;
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

	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
	{
		boaWrite(wp, "<script>window.location.href = '%s';</script>", submitUrl);
	}
}

//////////////////////////////////////////////////////////////////////////////
int isDhcpClientExist(char *name)
{
/*
	char tmpBuf[100];
	struct in_addr intaddr;

	if ( getInAddr(name, IP_ADDR, (void *)&intaddr ) ) {
		snprintf(tmpBuf, 100, "%s/%s-%s.pid", _DHCPC_PID_PATH, _DHCPC_PROG_NAME, name);
		if ( getPid(tmpBuf) > 0)
			return 1;
	}
*/
	return 0;
}

#ifdef USE_DEONET /* sghong, 20230926 */
void formNickname(request * wp, char *path, char *query)
{
	char *str, *submitUrl;
	char *mac, *desc;
	int  i, idx, intVal;
	int  totalEntry;
	char tmpBuf[100];
	MIB_NICKNAME_T Entry;

	str = boaGetVar(wp, "addNickname", "");
	if (str[0]) {
		mac = boaGetVar(wp, "mac", "");
		desc = boaGetVar(wp, "nickname", "");

		memset(&Entry, 0, sizeof(MIB_NICKNAME_T));

		totalEntry = mib_chain_total(MIB_NICKNAME_TBL); /* get chain record size */
		for (idx = 0; idx < totalEntry; idx++) 
		{
			if (mib_chain_get(MIB_NICKNAME_TBL, idx, (void *)&Entry)) 
			{
				if (strcmp(mac, Entry.clientMac) == 0)
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, strAddListErr,sizeof(tmpBuf)-1);
					goto setErr_al;
				}
			}
		}

		strcpy(Entry.clientMac, mac);
		strcpy(Entry.nickname, desc);

		intVal = mib_chain_add(MIB_NICKNAME_TBL, (unsigned char*)&Entry);
		if (intVal == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strAddListErr,sizeof(tmpBuf)-1);
			goto setErr_al;
		}
		else if (intVal == -1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
			goto setErr_al;
		}
	}

	str = boaGetVar(wp, "delNickname", "");
	if (str[0])
	{
		totalEntry = mib_chain_total(MIB_NICKNAME_TBL); /* get chain record size */

		str = boaGetVar(wp, "select", "");
		if (str[0])
		{
			for (i=0; i<totalEntry; i++) {
				idx = totalEntry-i-1;
				memset(tmpBuf, 0, sizeof(tmpBuf));
				snprintf(tmpBuf, 4, "s%d", idx);

				if ( !gstrcmp(str, tmpBuf) ) {
					/* get the specified chain record */
					if (!mib_chain_get(MIB_NICKNAME_TBL, idx, (void *)&Entry)) {
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, errGetEntry,sizeof(tmpBuf)-1);
						goto setErr_al;
					}

					// delete from chain record
					if(mib_chain_delete(MIB_NICKNAME_TBL, idx) != 1) {
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
						goto setErr_al;
					}
				}
			} // end of for
		}
	}

	str = boaGetVar(wp, "modNickname", "");
	if (str[0])
	{
		mac = boaGetVar(wp, "mac", "");
		desc = boaGetVar(wp, "nickname", "");

		totalEntry = mib_chain_total(MIB_NICKNAME_TBL); /* get chain record size */
		for (idx = 0; idx < totalEntry; idx++) 
		{
			if (mib_chain_get(MIB_NICKNAME_TBL, idx, (void *)&Entry)) 
			{
				if (strcmp(mac, Entry.clientMac) == 0)
				{
					memset(&Entry, 0, sizeof(MIB_NICKNAME_T));

					strcpy(Entry.clientMac, mac);
					strcpy(Entry.nickname, desc);
					mib_chain_update(MIB_NICKNAME_TBL, (unsigned char*)&Entry, idx); 
					break;
				}
			}
		}
	}

	Commit();

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);

	return;

setErr_al:
	ERR_MSG(tmpBuf);

	return;
}

int ShowNicknameTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	unsigned int entryNum, i;
	MIB_NICKNAME_T Entry;

	entryNum = mib_chain_total(MIB_NICKNAME_TBL);
	nBytesSent += boaWrite(wp, "<tr>"
			"<th align=center>%s</th>\n"
			"<th align=center>%s</th>\n"
			"<th align=center>%s</th>\n"
			"</tr>\n", 
			multilang(LANG_INDEX), multilang(LANG_MAC), multilang(LANG_NICKNAME));

	for (i=0; i<entryNum; i++) 
	{
		if (!mib_chain_get(MIB_NICKNAME_TBL, i, (void *)&Entry))
		{
			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

		nBytesSent += boaWrite(wp, "<tr>"
				"<td align=center width=\"5%%\"><input type=\"radio\" name=\"select\""
				" value=\"s%d\" "
				"></td>\n"
				"<td align=center width=\"8%%\">%s</td>\n"
				"<td align=center width=\"8%%\">%s</td>\n"
				"</tr>\n",
				i, Entry.clientMac, Entry.nickname);
	}

	return 0;
}
#endif

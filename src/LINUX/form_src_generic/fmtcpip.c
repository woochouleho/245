/*
 *      Web server handler routines for TCP/IP stuffs
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *      Authors: Dick Tam	<dicktam@realtek.com.tw>
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
#include "utility.h"
#include "../defs.h"
#include "debug.h"
#include "multilang.h"
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ)
#include "subr_net.h"
#endif

#include <linux/version.h>

#ifdef __i386__
#define _LITTLE_ENDIAN_
#endif

/*-- Macro declarations --*/
#ifdef _LITTLE_ENDIAN_
#define ntohdw(v) ( ((v&0xff)<<24) | (((v>>8)&0xff)<<16) | (((v>>16)&0xff)<<8) | ((v>>24)&0xff) )

#else
#define ntohdw(v) (v)
#endif

#ifdef CONFIG_USER_VLAN_ON_LAN
void formVLANonLAN(request * wp, char *path, char *query)
{
	char *str, *submitUrl;
	char tmpBuf[100];
	unsigned short value;
	unsigned char vUChar, changed, total_changed;
	MIB_CE_SW_PORT_T sw_entry;

	// (1) Clean all rules and setting 
#ifdef CONFIG_USER_BRIDGE_GROUPING
	setup_bridge_grouping(DEL_RULE);
#endif
	setup_VLANonLAN(DEL_RULE);

	// Set LAN1 VID
	if (mib_chain_get(MIB_SW_PORT_TBL, 0, &sw_entry) == 0) {
		sprintf(tmpBuf, "%s:%s", Tget_mib_error, MIB_SW_PORT_TBL);
		goto setErr_vlan_on_lan;
	}
	changed = 0;
	str = boaGetVar(wp, "lan1_vid", "");
	if (str[0]) {
		value = strtoul(str, NULL, 10);
		if (sw_entry.vid != value) {
			sw_entry.vid = value;
			changed++;
		}
	}
	str = boaGetVar(wp, "lan1_vid_cap", "");
	if (str[0]) {
		vUChar = strtoul(str, NULL, 10);
		if (sw_entry.vlan_on_lan_enabled != vUChar) {
			sw_entry.vlan_on_lan_enabled = vUChar;
			changed++;
		}
	}
	if (changed && mib_chain_update(MIB_SW_PORT_TBL, &sw_entry, 0) == 0) {
		sprintf(tmpBuf, "%s:%s", Tupdate_chain_error, MIB_SW_PORT_TBL);
		goto setErr_vlan_on_lan;
	}
	total_changed = changed;

	// Set LAN2 VID
	if (mib_chain_get(MIB_SW_PORT_TBL, 1, &sw_entry) == 0) {
		sprintf(tmpBuf, "%s:%s", Tget_mib_error, MIB_SW_PORT_TBL);
		goto setErr_vlan_on_lan;
	}
	changed = 0;
	str = boaGetVar(wp, "lan2_vid", "");
	if (str[0]) {
		value = strtoul(str, NULL, 10);
		if (sw_entry.vid != value) {
			sw_entry.vid = value;
			changed++;
		}
	}
	str = boaGetVar(wp, "lan2_vid_cap", "");
	if (str[0]) {
		vUChar = strtoul(str, NULL, 10);
		if (sw_entry.vlan_on_lan_enabled != vUChar) {
			sw_entry.vlan_on_lan_enabled = vUChar;
			changed++;
		}
	}
	if (changed && mib_chain_update(MIB_SW_PORT_TBL, &sw_entry, 1) == 0) {
		sprintf(tmpBuf, "%s:%s", Tupdate_chain_error, MIB_SW_PORT_TBL);
		goto setErr_vlan_on_lan;
	}
	total_changed += changed;

	// Set LAN3 VID
	if (mib_chain_get(MIB_SW_PORT_TBL, 2, &sw_entry) == 0) {
		sprintf(tmpBuf, "%s:%s", Tget_mib_error, MIB_SW_PORT_TBL);
		goto setErr_vlan_on_lan;
	}
	changed = 0;
	str = boaGetVar(wp, "lan3_vid", "");
	if (str[0]) {
		value = strtoul(str, NULL, 10);
		if (sw_entry.vid != value) {
			sw_entry.vid = value;
			changed++;
		}
	}
	str = boaGetVar(wp, "lan3_vid_cap", "");
	if (str[0]) {
		vUChar = strtoul(str, NULL, 10);
		if (sw_entry.vlan_on_lan_enabled != vUChar) {
			sw_entry.vlan_on_lan_enabled = vUChar;
			changed++;
		}
	}
	if (changed && mib_chain_update(MIB_SW_PORT_TBL, &sw_entry, 2) == 0) {
		sprintf(tmpBuf, "%s:%s", Tupdate_chain_error, MIB_SW_PORT_TBL);
		goto setErr_vlan_on_lan;
	}
	total_changed += changed;

	// Set LAN4 VID
	if (mib_chain_get(MIB_SW_PORT_TBL, 3, &sw_entry) == 0) {
		sprintf(tmpBuf, "%s:%s", Tget_mib_error, MIB_SW_PORT_TBL);
		goto setErr_vlan_on_lan;
	}
	changed = 0;
	str = boaGetVar(wp, "lan4_vid", "");
	if (str[0]) {
		value = strtoul(str, NULL, 10);
		if (sw_entry.vid != value) {
			sw_entry.vid = value;
			changed++;
		}
	}
	str = boaGetVar(wp, "lan4_vid_cap", "");
	if (str[0]) {
		vUChar = strtoul(str, NULL, 10);
		if (sw_entry.vlan_on_lan_enabled != vUChar) {
			sw_entry.vlan_on_lan_enabled = vUChar;
			changed++;
		}
	}
	if (changed && mib_chain_update(MIB_SW_PORT_TBL, &sw_entry, 3) == 0) {
		sprintf(tmpBuf, "%s:%s", Tupdate_chain_error, MIB_SW_PORT_TBL);
		goto setErr_vlan_on_lan;
	}
	total_changed += changed;

	// (2) setup all configuration and rules
	setup_VLANonLAN(ADD_RULE);
#ifdef CONFIG_USER_BRIDGE_GROUPING
	setup_bridge_grouping(ADD_RULE);
#endif

#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rtk_igmp_mld_snooping_module_init(BRIF, CONFIGALL, NULL);
#endif

// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	if (total_changed)
		Commit();
#endif

#ifndef NO_ACTION
	pid = fork();
	if (pid)
		waitpid(pid, NULL, 0);
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH,
			 _CONFIG_SCRIPT_PROG);
#ifdef HOME_GATEWAY
		execl(tmpBuf, _CONFIG_SCRIPT_PROG, "gw", "bridge", NULL);
#else
		execl(tmpBuf, _CONFIG_SCRIPT_PROG, "ap", "bridge", NULL);
#endif
		exit(1);
	}
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG(submitUrl);
	return;

setErr_vlan_on_lan:
	ERR_MSG(tmpBuf);
}
#endif

///////////////////////////////////////////////////////////////////
void formTcpipLanSetup(request * wp, char *path, char *query)
{
	char	*pStr, *strIp, *strMask, *strSnoop, *submitUrl, *strBlock;
	struct in_addr inIp, inMask;
	char tmpBuf[100], mode;
#if defined(CONFIG_SECONDARY_IP) && !defined(DHCPS_POOL_COMPLETE_IP)
	char dhcp_pool;
#endif
#ifndef NO_ACTION
	int pid;
#endif
#ifdef CONFIG_IGMP_FORBID
	char *str_igmpforbid;

#endif
	unsigned char vChar;

//star:for ip change
	struct in_addr origIp,origMask;
	int ip_mask_changed_flag = 0;
	int lanip_changed_flag = 0;
	int igmp_changed_flag = 0;
	int ip2_changed_flag = 0;
	int llmnr_changed_flag = 0;
	int mdns_changed_flag = 0;
#ifdef WLAN_SUPPORT
	int wireless_block_flag = 0;
#endif

#if defined(CONFIG_USER_UPNPD) || defined(CONFIG_USER_MINIUPNPD)
	unsigned char upnpdEnable;
	unsigned int upnpItf;
	char ext_ifname[IFNAMSIZ];
#endif
#ifdef CONFIG_IPV6
	char *lanIPv6AddrStr=NULL;
	char *str_extif,*str_dns1,*str_dns2;
	static char cmdBuf[100];
	char *lanIPv6DnsModeStr=NULL;
	unsigned char ipv6DnsMode=0;
	unsigned int ext_if=DUMMY_IFINDEX;
	struct in6_addr dnsv61, dnsv62, dnsv63;
	char *lanIPv6PrefixStr=NULL,*lanIPv6PrefixModeStr=NULL;
	char *tok=NULL, *prefixStr=NULL, *saveptr1=NULL;
	char len=0;
	char lanIPv6PrefixMode;
#endif
	memset(tmpBuf, 0, sizeof(tmpBuf));
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	unsigned char tmp_vChar;
	mib_get_s( MIB_DHCP_MODE, (void *)&tmp_vChar, sizeof(tmp_vChar));

	/* DHCP Client Mode don't need config ip/mask */
	if(tmp_vChar != DHCPV4_LAN_CLIENT){
#endif
	// Set clone MAC address
	strIp = boaGetVar(wp, "ip", "");
	strMask = boaGetVar(wp, "mask", "");

	if (!isValidHostID(strIp, strMask)) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, multilang(LANG_INVALID_IP_SUBNET_MASK_COMBINATION),sizeof(tmpBuf)-1);
		goto setErr_tcpip;
	}

	if ( strIp[0] ) {
		if ( !inet_aton(strIp, &inIp) ) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strWrongIP,sizeof(tmpBuf)-1);
			goto setErr_tcpip;
		}

		mib_get_s(MIB_ADSL_LAN_IP, (void *)&origIp, sizeof(origIp));

		if(origIp.s_addr != inIp.s_addr)		
		{
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ)
			delete_dsldevice_on_hosts();
#endif
			ip_mask_changed_flag = 1;
		/*lan ip & dhcp gateway setting should be set independently*/
		#if 1
			{
				struct in_addr dhcp_gw;
				struct in_addr tmp_dhcp_start, dhcp_start, dhcp_end;

				mib_get_s(MIB_ADSL_LAN_DHCP_GATEWAY, (void *)&dhcp_gw, sizeof(dhcp_gw));
				if(dhcp_gw.s_addr==origIp.s_addr)
					mib_set(MIB_ADSL_LAN_DHCP_GATEWAY, (void *)&inIp);

				mib_get_s(MIB_DHCP_POOL_START, (void *)&tmp_dhcp_start, sizeof(dhcp_start));

				/* LAN pool set */
				dhcp_start.s_addr = (inIp.s_addr & 0xFFFFFF00) | 2;
				dhcp_end.s_addr = (inIp.s_addr & 0xFFFFFF00) | 253;

				if ((tmp_dhcp_start.s_addr & 0xFFFFFF00) !=  (dhcp_start.s_addr & 0xFFFFFF00))
				{
					mib_set(MIB_DHCP_POOL_START, (void *)&dhcp_start);
					mib_set(MIB_DHCP_POOL_END, (void *)&dhcp_end);
					lanip_changed_flag = 1;
				}
			}
		#else
			mib_set(MIB_ADSL_LAN_DHCP_GATEWAY, (void *)&inIp);
		#endif

		// Magician: UPnP Daemon Start
		#if defined(CONFIG_USER_UPNPD)||defined(CONFIG_USER_MINIUPNPD)
			restart_upnp(CONFIGALL, 0, 0, 0);
		#endif
		// The end of UPnP Daemon Start

			if (!mib_set(MIB_ADSL_LAN_IP, (void *)&inIp)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, strSetIPerror,sizeof(tmpBuf)-1);
				goto setErr_tcpip;
			}
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ)
			add_dsldevice_on_hosts();
#endif
		}
	}
	else { // get current used IP
//		if ( !getInAddr(BRIDGE_IF, IP_ADDR, (void *)&inIp) ) {
//			strcpy(tmpBuf, "Get IP-address error!");
//			goto setErr_tcpip;
//		}
	}

//	strMask = boaGetVar(wp, "mask", "");
	if ( strMask[0] ) {
		if (!isValidNetmask(strMask, 1)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strWrongMask,sizeof(tmpBuf)-1);
			goto setErr_tcpip;
		}
		if ( !inet_aton(strMask, &inMask) ) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strWrongMask,sizeof(tmpBuf)-1);
			goto setErr_tcpip;
		}
		mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)&origMask, sizeof(origMask));
		if(origMask.s_addr!= inMask.s_addr)
			ip_mask_changed_flag = 1;
		if ( !mib_set(MIB_ADSL_LAN_SUBNET, (void *)&inMask)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetMaskerror,sizeof(tmpBuf)-1);
			goto setErr_tcpip;
		}
	}
	else { // get current used netmask
//		if ( !getInAddr(BRIDGE_IF, SUBNET_MASK, (void *)&inMask )) {
//			strcpy(tmpBuf, "Get subnet-mask error!");
//			goto setErr_tcpip;
//		}
	}

#ifdef ONE_USER_LIMITED
#ifdef IPV6
		char remote_ip_addr[INET6_ADDRSTRLEN] = {0};
#else
		char remote_ip_addr[20] = {0};
#endif
		remote_ip_addr[sizeof(remote_ip_addr)-1]='\0';
		strncpy(remote_ip_addr, wp->remote_ip_addr,sizeof(remote_ip_addr)-1);
		if ((origIp.s_addr & origMask.s_addr) != (inIp.s_addr & inMask.s_addr))
		{
			if (free_from_login_list(wp))
			{
				printf("logout successful from %s\n", remote_ip_addr);
			}
		}
#endif

#ifdef CONFIG_USER_DHCPCLIENT_MODE
	}
#endif

#ifdef WLAN_SUPPORT
	// set eth to wireless blocking
	strBlock = boaGetVar(wp, "BlockEth2Wir", "");
	if (strBlock[0])
	{
		unsigned char orWblock;
		if (strBlock[0] == '0')
			vChar = 0;
		else // '1'
			vChar = 1;

		mib_get_s(MIB_WLAN_BLOCK_ETH2WIR, (void *)&orWblock, sizeof(orWblock));
		if( orWblock != vChar )
			wireless_block_flag = 1;

#if defined(TRIBAND_SUPPORT)
{
        int k, orig_wlan_idx;
        orig_wlan_idx = wlan_idx;
        for (k=0; k<NUM_WLAN_INTERFACE; k++) {
            wlan_idx = k;
            if ( mib_set(MIB_WLAN_BLOCK_ETH2WIR, (void *)&vChar) == 0) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
                strncpy(tmpBuf, strSetLanWlanBlokErr,sizeof(tmpBuf)-1);
                goto setErr_tcpip;
            }
        }
        wlan_idx = orig_wlan_idx;
}
#else /* !defined(TRIBAND_SUPPORT) */
		if ( mib_set(MIB_WLAN_BLOCK_ETH2WIR, (void *)&vChar) == 0)
		{
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetLanWlanBlokErr,sizeof(tmpBuf)-1);
			goto setErr_tcpip;
		}
		#ifdef WLAN_DUALBAND_CONCURRENT
		wlan_idx = 1;
		if ( mib_set(MIB_WLAN_BLOCK_ETH2WIR, (void *)&vChar) == 0)
		{
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strSetLanWlanBlokErr,sizeof(tmpBuf)-1);
			goto setErr_tcpip;
		}
		wlan_idx = 0;
		#endif
#endif /* defined(TRIBAND_SUPPORT) */

		setup_wlan_block();

		TRACE(STA_SCRIPT,"/bin/echo 2 > /proc/fastbridge\n");
		system("/bin/echo 2 > /proc/fastbridge");
	}
#endif

#ifdef CONFIG_SECONDARY_IP
	char ip2mode;
	pStr = boaGetVar(wp, "enable_ip2", "");
	if (pStr[0]) {
		if (pStr[0] == '1') {
			mode = 1;
			strIp = boaGetVar(wp, "ip2", "");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, strWrongIP,sizeof(tmpBuf)-1);
					goto setErr_tcpip;
				}
				mib_get_s(MIB_ADSL_LAN_IP2, (void *)&origIp, sizeof(origIp));
				if(origIp.s_addr != inIp.s_addr)
				{
					ip2_changed_flag = 1;
					ip_mask_changed_flag = 1;
				}
				if ( !mib_set( MIB_ADSL_LAN_IP2, (void *)&inIp)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, strSetIPerror,sizeof(tmpBuf)-1);
					goto setErr_tcpip;
				}
			}
			strMask = boaGetVar(wp, "mask2", "");
			if ( strMask[0] ) {
				if ( !inet_aton(strMask, &inMask) ) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, strWrongMask,sizeof(tmpBuf)-1);
					goto setErr_tcpip;
				}
				mib_get_s(MIB_ADSL_LAN_SUBNET2, (void *)&origMask, sizeof(origMask));
				if(origMask.s_addr!= inMask.s_addr)
				{
					ip2_changed_flag = 1;
					ip_mask_changed_flag = 1;
				}
				if ( !mib_set(MIB_ADSL_LAN_SUBNET2, (void *)&inMask)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, strSetMaskerror,sizeof(tmpBuf)-1);
					goto setErr_tcpip;
				}
			}
#ifndef DHCPS_POOL_COMPLETE_IP
			// DHCP Server pool is on ...
			pStr = boaGetVar(wp, "dhcpuse", "");
			if ( pStr[0] == '0' ) { // Primary LAN
				dhcp_pool = 0;
			}
			else { // Secondary LAN
				dhcp_pool = 1;
			}
			mib_set(MIB_ADSL_LAN_DHCP_POOLUSE, (void *)&dhcp_pool);
#endif
		}
		else {
			mode = 0;
		}
	}
	else {
		mode = 0;
	}
	mib_get_s(MIB_ADSL_LAN_ENABLE_IP2, (void *)&ip2mode, sizeof(ip2mode));
	if(ip2mode != mode)
	{
		ip2_changed_flag = 1;
		ip_mask_changed_flag = 1;
	}
	mib_set(MIB_ADSL_LAN_ENABLE_IP2, (void *)&mode);
#endif	// of CONFIG_SECONDARY_IP

#if defined(CONFIG_RTL_IGMP_SNOOPING) || defined(CONFIG_BRIDGE_IGMP_SNOOPING)
	char origmode = 0;
	strSnoop = boaGetVar(wp, "snoop", "");
	if ( strSnoop[0] ) {
		// bitmap for virtual lan port function
		// Port Mapping: bit-0
		// QoS : bit-1
		// IGMP snooping: bit-2
		mib_get_s(MIB_MPMODE, (void *)&mode, sizeof(mode));
		origmode = mode;
		strSnoop = boaGetVar(wp, "snoop", "");
		if ( strSnoop[0] == '1' ) {
			mode |= MP_IGMP_MASK;
#if defined(CONFIG_BRIDGE_IGMP_SNOOPING)
			mode |= MP_MLD_MASK;
#endif
			if(origmode != mode)
				igmp_changed_flag = 1;
		}
		else {
			mode &= ~MP_IGMP_MASK;
#if defined(CONFIG_BRIDGE_IGMP_SNOOPING)
			mode &= ~MP_MLD_MASK;
#endif
			if(origmode != mode)
				igmp_changed_flag = 1;
		}
		mib_set(MIB_MPMODE, (void *)&mode);
		rtk_multicast_snooping();
	}

#ifdef CONFIG_IGMP_FORBID
 str_igmpforbid = boaGetVar(wp, "igmpforbid", "");
 if( str_igmpforbid[0] )
 	{
 	   if(str_igmpforbid[0]=='0')
 	   	{
		  vChar =0;
		  __dev_igmp_forbid(0);
 	   	}
	   else if (str_igmpforbid[0]=='1')
	   	{
	   	  vChar =1;
		   __dev_igmp_forbid(1);
	   	}
	   mib_set(MIB_IGMP_FORBID_ENABLE, (void *)&vChar);
 	}
#endif
#endif

#ifdef CONFIG_USER_LLMNR
	char *strLlmnr;
	char origstr[MAX_NAME_LEN];
	strLlmnr = boaGetVar(wp, "llmnr", "");
	if (strLlmnr[0])
	{
		unsigned char origllmnr;
		if (strLlmnr[0] == '0')
			vChar = 0;
		else // '1'
			vChar = 1;

		mib_get_s(MIB_LLMNR_ENABLE, (void *)&origllmnr, sizeof(origllmnr));
		if( origllmnr != vChar ){
			mib_set(MIB_LLMNR_ENABLE, (void *)&vChar);
			llmnr_changed_flag = 1;
			if(1 == vChar)
			{
				mib_get_s(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)origstr, sizeof(origstr));
#ifdef CONFIG_IPV6
				va_cmd("/bin/llmnrd", 6, 0, "-H", origstr, "-i", ALIASNAME_BR0 , "-d", "-6");
#else
				va_cmd("/bin/llmnrd", 5, 0, "-H", origstr, "-i", ALIASNAME_BR0 , "-d");	
#endif
			}
			else
				kill_by_pidfile_new((const char*)LLMNR_PID, SIGTERM);
		}
	}			

#endif

#ifdef CONFIG_USER_MDNS
		char *strMdns;
		char str_domain[MAX_NAME_LEN];
		strMdns = boaGetVar(wp, "mdns", "");
		if (strMdns[0])
		{
			unsigned char origmdns;
			if (strMdns[0] == '0')
				vChar = 0;
			else // '1'
				vChar = 1;
	
			mib_get_s(MIB_MDNS_ENABLE, (void *)&origmdns, sizeof(origmdns));
			if( origmdns != vChar ){
				mib_set(MIB_MDNS_ENABLE, (void *)&vChar);
				mdns_changed_flag = 1;
				if(1 == vChar)
				{
					mib_get_s(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)str_domain, sizeof(str_domain));
					va_cmd("/bin/mdnsd", 4, 0, "-i", ALIASNAME_BR0, "-n", str_domain);
				}
				else
					kill_by_pidfile_new((const char*)MDNS_PID, SIGTERM);
			}
		}			
	
#endif

#if defined(CONFIG_LUNA) && defined(CONFIG_RTL_MULTI_LAN_DEV)
	int i = 0;
	int ret = 0;
	char str_lan[16]={0};
	MIB_CE_SW_PORT_T lan_entry;
	for(i = 0 ; i < SW_LAN_PORT_NUM ; i++)
	{
		if(mib_chain_get(MIB_SW_PORT_TBL, i, (void *)&lan_entry) == 0)
		{
			fprintf(stderr, "<%s:%d> Get MIB_SW_PORT_TBL.%d failed\n", __func__, __LINE__, i);
			continue;
		}
		snprintf(str_lan, sizeof(str_lan), "lan%d", i);
		pStr = boaGetVar(wp, str_lan, "1");
		vChar = atoi(pStr);

		if(vChar != lan_entry.enable)
		{
			lan_entry.enable = vChar;
			mib_chain_update(MIB_SW_PORT_TBL, &lan_entry, i);
			restart_ethernet(i+1);
		}

#ifdef USE_DEONET /* sghong, 20221124 */
		snprintf(str_lan, sizeof(str_lan), "lan%d_fc", i);
		pStr = boaGetVar(wp, str_lan, "0");
		vChar = atoi(pStr);

		if(vChar != lan_entry.flowcontrol)
		{
			int rtk_port = 0;

			rtk_port = rtk_port_get_lan_phyID(i); /* port mapping */
			if (rtk_port != -1)
			{
				lan_entry.flowcontrol = vChar;
				mib_chain_update(MIB_SW_PORT_TBL, &lan_entry, i);
				ret = deo_port_autonego_fc_set(rtk_port, lan_entry.flowcontrol);
				if (ret != 0)
					fprintf(stderr, "<%s:%d> port:%d flowcontrol set failed\n", __func__, __LINE__, i+1);
			}
		}
#endif
		snprintf(str_lan, sizeof(str_lan), "lan%d_GL", i);
		pStr = boaGetVar(wp, str_lan, "0");
		vChar = atoi(pStr);

		if(vChar != lan_entry.gigalite)
		{
			int rtk_port = 0;
			lan_entry.gigalite = vChar;
			mib_chain_update(MIB_SW_PORT_TBL, &lan_entry, i);

			rtk_port = rtk_port_get_lan_phyID(i); /* port mapping */
			if(rtk_port != -1)
			{
				ret = rtk_port_gigaLiteEnable_set(rtk_port, lan_entry.gigalite);
				if (ret != 0)
					fprintf(stderr, "<%s:%d> port:%d gigalite set failed\n", __func__, __LINE__, i+1);

				ret = rtk_port_phyAutoNegoEnable_set(rtk_port, 1);
				if (ret != 0)
					fprintf(stderr, "<%s.%d> port:%d autonego set failed\n", __func__, __LINE__, i+1);
			}
		}
	}
#endif

	if(ip_mask_changed_flag == 1)
	{
		char syslog_msg[120];
#if 0 /* 변경된 IP로 웹 접속 삭제 */
		unsigned char ipaddr[32]="";

		//ql: when ip mask changed, then dhcpd should be changed simultaneously
		//applyLanChange2Dhcpd();
		strncpy(ipaddr, inet_ntoa(*((struct in_addr *)&inIp)), 16);
		ipaddr[15] = '\0';

		wp->buffer_end=0; // clear header
		boaWrite(wp, "HTTP/1.0 200 OK\n");
		boaWrite(wp, "Pragma: no-cache\n");
		boaWrite(wp, "Cache-Control: no-cache\n");
		boaWrite(wp, "\n");
		boaWrite(wp, "<HTML><HEAD><TITLE>Login</TITLE>\n"
					"<script>\nfunction loadFunc() { top.window.location.assign(\"http://%s\"); }\n</script>\n"
					"</HEAD>\n"
				 "<BODY onload=\"loadFunc()\"></BODY>\n</HTML>\n", ipaddr);

		// Boa must go through the normal free_request(free fd, session, .....) procedure for OLD LAN IP on process_requests(). Then it can reset the new LAN_IP.
#endif
		wp->status = LANIP_CHANGE;

		memset(syslog_msg, 0, sizeof(syslog_msg));
		sprintf(syslog_msg, "Network/LAN is configured (IP: %s Netmask: %s)", strIp, strMask);
		syslog2(LOG_DM_HTTP, LOG_DM_HTTP_LAN_IP_CHANGE, syslog_msg);
	}
	else if((igmp_changed_flag == 1) || (llmnr_changed_flag ==1) || (mdns_changed_flag ==1))
	{
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

#ifdef CONFIG_IPV6
	pStr = boaGetVar(wp, "ipv6LLAIPmode", "");
	if (pStr[0]) {
		char tmpBuf[100]= {0};

		if (mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)tmpBuf, sizeof(tmpBuf)) == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, "Get MIB_IPV6_LAN_LLA_IP_ADDR fail.!",sizeof(tmpBuf)-1);
			goto setErr_tcpip;
		}

		if (!strcmp(tmpBuf, ""))	// Old Setting is Auto mode
			delOrgLanLinklocalIPv6Address();
		else {						// Old Setting is Static mode
			// clean static LLA IP
			snprintf(cmdBuf, sizeof(cmdBuf), "%s/%d", tmpBuf, 64);
			va_cmd(IFCONFIG, 3, 1, LANIF, ARG_DEL, cmdBuf);
		}

		if (pStr[0] == '1') {	// New setting is Static mode
			lanIPv6AddrStr = boaGetVar(wp, "Ipv6LLAIP", "");
			if (lanIPv6AddrStr[0]) {
				if ( !mib_set(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)lanIPv6AddrStr)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, "Invalid LAN IPv6 LLA address!",sizeof(tmpBuf)-1);
					goto setErr_tcpip;
				}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 18, 20))
				snprintf(cmdBuf, sizeof(cmdBuf), "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/addr_gen_mode", LANIF);
				system(cmdBuf);
#endif
				setup_disable_ipv6(LANIF, 0); /* Because you need to open IPv6 proc before adding IP. */
				snprintf(cmdBuf, sizeof(cmdBuf), "%s/%d", lanIPv6AddrStr, 64);
				va_cmd(IFCONFIG, 3, 1, LANIF, ARG_ADD, cmdBuf);
			}
		}
		else {					// New setting is Auto Mode
			if ( !mib_set(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)"")) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, "Invalid LAN IPv6 LLA address!",sizeof(tmpBuf)-1);
				goto setErr_tcpip;
			}
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 18, 20))
			snprintf(cmdBuf, sizeof(cmdBuf), "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/addr_gen_mode", LANIF);
			system(cmdBuf);
#endif
			setup_disable_ipv6(LANIF, 1);
			setup_disable_ipv6(LANIF, 0);
		}
		ip2_changed_flag = 1;
	}
#endif

	lanIPv6AddrStr = boaGetVar(wp, "ipv6_addr", "");

	if(!lanIPv6AddrStr) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, "Invalid LAN IPv6 address!",sizeof(tmpBuf)-1);
		goto setErr_tcpip;
	}

	if ( !mib_set(MIB_IPV6_LAN_IP_ADDR, (void *)lanIPv6AddrStr)) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, "set LAN IPv6 address fail!",sizeof(tmpBuf)-1);
		goto setErr_tcpip;
	}

    lanIPv6DnsModeStr = boaGetVar(wp, "ipv6landnsmode", "");
	str_extif = boaGetVar(wp, "if_dns_wan", "");

	if(!lanIPv6DnsModeStr) {
		goto setErr_tcpip;
	}

	ipv6DnsMode = lanIPv6DnsModeStr[0]-'0';
	printf("[%s] lanIPv6DNSMode=%d\n",__func__,ipv6DnsMode);
	if ( !mib_set(MIB_LAN_DNSV6_MODE, (void *)&ipv6DnsMode)) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, "Error!! set IPv6 DNS Mode fail!",sizeof(tmpBuf)-1);
		goto setErr_tcpip;
	}

	switch(ipv6DnsMode){
		case IPV6_DNS_HGWPROXY:
			break;
		case IPV6_DNS_WANCONN:
			if(str_extif[0])
				ext_if = (unsigned int)atoi(str_extif);
			else
				ext_if = DUMMY_IFINDEX;  // No interface selected.

			printf("DNS WANConnect at 0x%x\n",ext_if);
			if ( !mib_set(MIB_DNSINFO_WANCONN, (void *)&ext_if)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, "Error!! set LAN IPv6 DNS WAN Conn fail!",sizeof(tmpBuf)-1);
				goto setErr_tcpip;
			}

			break;
		case IPV6_DNS_STATIC:
			memset(dnsv61.s6_addr, 0, 16);
			memset(dnsv62.s6_addr, 0, 16);
			str_dns1 = boaGetVar(wp, "Ipv6Dns1", "");
			if(str_dns1[0]) {
				if ( !inet_pton(PF_INET6, str_dns1, &dnsv61) ) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tinvalid_DNS_address,sizeof(tmpBuf)-1);
					goto setErr_tcpip;
				}
				if ( !mib_set(MIB_ADSL_WAN_DNSV61, (void *)&dnsv61)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
	  				strncpy(tmpBuf, TDNS_mib_set_error,sizeof(tmpBuf)-1);
					goto setErr_tcpip;
				}
			}

			str_dns2 = boaGetVar(wp, "Ipv6Dns2", "");
			if(str_dns2[0]) {
				if ( !inet_pton(PF_INET6, str_dns2, &dnsv62) ) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tinvalid_DNS_address,sizeof(tmpBuf)-1);
					goto setErr_tcpip;
				}
				if ( !mib_set(MIB_ADSL_WAN_DNSV62, (void *)&dnsv62)) {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
	  				strncpy(tmpBuf, TDNS_mib_set_error,sizeof(tmpBuf)-1);
					goto setErr_tcpip;
				}
			}

			break;
		default:
			break;
	}

	lanIPv6PrefixModeStr = boaGetVar(wp, "ipv6lanprefixmode", "");
	lanIPv6PrefixMode = lanIPv6PrefixModeStr[0]-'0';
	printf("[%s] lanIPv6PrefixMode=%d\n",__func__,lanIPv6PrefixMode);
	if ( !mib_set(MIB_PREFIXINFO_PREFIX_MODE, (void *)&lanIPv6PrefixMode)) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, "Set LAN IPv6 prefix mode fail!",sizeof(tmpBuf)-1);
		goto setErr_tcpip;
	}
#ifdef CONFIG_USER_RADVD
	if ( !mib_set(MIB_V6_RADVD_PREFIX_MODE, (void *)&lanIPv6PrefixMode)) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, "Set LAN  RADVD IPv6 prefix mode fail!",sizeof(tmpBuf)-1);
		goto setErr_tcpip;
	}
#endif
#ifdef DHCPV6_ISC_DHCP_4XX
	if ( !mib_set(MIB_DHCPV6S_TYPE, (void *)&lanIPv6PrefixMode)) {
		tmpBuf[sizeof(tmpBuf)-1]='\0';
		strncpy(tmpBuf, "Set LAN  DHCPv6 type mode fail!",sizeof(tmpBuf)-1);
		goto setErr_tcpip;
	}
#endif

	switch(lanIPv6PrefixMode){
		case IPV6_PREFIX_DELEGATION:
			//MIB_PREFIXINFO_DELEGATED_WANCONN
			str_extif = boaGetVar(wp, "if_delegated_wan", "");
    		if(str_extif[0])
	    		ext_if = (unsigned int)atoi(str_extif);
		    else
			    ext_if = DUMMY_IFINDEX;  // No interface selected.

		    printf("Prefix is delegated WAN at %d\n",ext_if);
		    if(!mib_set(MIB_PREFIXINFO_DELEGATED_WANCONN, (void *)&ext_if))
		    {
		    	tmpBuf[sizeof(tmpBuf)-1]='\0';
		    	strncpy(tmpBuf, "Set PrefixInfo Delegated wanconn error!",sizeof(tmpBuf)-1);
			    goto setErr_tcpip;
		    }
    
    		break;

		case IPV6_PREFIX_STATIC:
	        lanIPv6PrefixStr = boaGetVar(wp, "lanIpv6prefix", "");

	        if(!lanIPv6PrefixStr) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
		        strncpy(tmpBuf, "Invalid LAN IPv6 Prefix!",sizeof(tmpBuf)-1);
		        goto setErr_tcpip;
	        }    

	        //Prefix will be like 2001:2222::/64 form	
	        prefixStr = strtok_r(lanIPv6PrefixStr,"/",&saveptr1);
        	tok = strtok_r(NULL,"/",&saveptr1);
        	len = atoi(tok);
        
        	if(!prefixStr) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
        		strncpy(tmpBuf, "Invalid LAN IPv6 Prefix!",sizeof(tmpBuf)-1);
        		goto setErr_tcpip;
        	}
        	
        	if((len>64) || (len<48) ) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
        		strncpy(tmpBuf, "Invalid LAN IPv6 Prefix Len!",sizeof(tmpBuf)-1);
        		goto setErr_tcpip;
        	}
        
        	if ( !mib_set(MIB_IPV6_LAN_PREFIX, (void *)prefixStr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
        		strncpy(tmpBuf, "LAN IPv6 prefix fail!",sizeof(tmpBuf)-1);
        		goto setErr_tcpip;
        	}

        	if ( !mib_set(MIB_IPV6_LAN_PREFIX_LEN, (void *)&len )) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
        		strncpy(tmpBuf, "LAN IPv6 prefix length fail!",sizeof(tmpBuf)-1);
        		goto setErr_tcpip;
        	}
#ifdef CONFIG_USER_RADVD
		if ( !mib_set(MIB_V6_PREFIX_IP, (void *)prefixStr)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
        		strncpy(tmpBuf, "LAN RADVD IPv6 prefix fail!",sizeof(tmpBuf)-1);
        		goto setErr_tcpip;
        	}
		if ( !mib_set(MIB_V6_PREFIX_LEN, (void *)tok)) {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
        		strncpy(tmpBuf, "LAN RADVD IPv6 prefix length fail!",sizeof(tmpBuf)-1);
        		goto setErr_tcpip;
        	}
#endif
#ifdef DHCPV6_ISC_DHCP_4XX
		struct in6_addr ip6Addr,ip6Addr_start, ip6Addr_end;
		inet_pton(AF_INET6, prefixStr, &ip6Addr);
		prefixtoIp6(&ip6Addr, len, &ip6Addr_start, 0);
		prefixtoIp6(&ip6Addr, len, &ip6Addr_end, 1);
		
		if ( !mib_set(MIB_DHCPV6S_RANGE_START, (void *)&ip6Addr_start)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, "LAN IPv6 DHCPv6 Range Start fail!",sizeof(tmpBuf)-1);
			goto setErr_tcpip;
		}
		if ( !mib_set(MIB_DHCPV6S_RANGE_END, (void *)&ip6Addr_end)) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, "LAN IPv6 DHCPv6 Range End  fail!",sizeof(tmpBuf)-1);
			goto setErr_tcpip;
		}
#endif
        	break;
    }
	
	sleep(3);

#ifdef CONFIG_IPV6
	//TODO: re-modify dhcpd6, radvd conf and restart
	restartLanV6Server();
#endif

// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	if( ip_mask_changed_flag || ip2_changed_flag || igmp_changed_flag || llmnr_changed_flag || mdns_changed_flag){
#if !defined(CONFIG_E8B) && defined(IP_ACL) && defined(ACL_IP_RANGE)
		if(ip_mask_changed_flag){
			rtk_modify_ip_range_of_acl_default_rule();

			restart_acl();
		}
#endif
		Commit();
	}
#ifdef WLAN_SUPPORT
	else if(wireless_block_flag)
		Commit();
#endif
#ifdef CONFIG_RTK_DEV_AP
    else
        Commit();
#endif
#endif // of #if COMMIT_IMMEDIATELY
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

	if(ip_mask_changed_flag)
	{
		SaveAndReboot(wp);
	}

	return;
setErr_tcpip:
	ERR_MSG(tmpBuf);
}

int lan_setting(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	int i;
	struct user_info * pUser_info = NULL;
#ifdef CONFIG_SECONDARY_IP
	char buf[32];
#endif
	pUser_info = search_login_list(wp);

#ifdef CONFIG_SECONDARY_IP
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr>\n<td><font size=2>\n");
	nBytesSent += boaWrite(wp, "<input type=checkbox name=enable_ip2 value=1 ");
	nBytesSent += boaWrite(wp, "onClick=updateInput()>&nbsp;&nbsp;");
	nBytesSent += boaWrite(wp, "<b>%s</b>\n</font></td></tr>\n", multilang(LANG_SECONDARY_IP));
#else
	nBytesSent += boaWrite(wp, "<tr>\n<td>\n");
	nBytesSent += boaWrite(wp, "<input type=checkbox name=enable_ip2 value=1 ");
	nBytesSent += boaWrite(wp, "onClick=updateInput()>&nbsp;&nbsp;");
	nBytesSent += boaWrite(wp, "<b>%s</b>\n</td></tr>\n", multilang(LANG_SECONDARY_IP));
#endif
	
	getMIB2Str(MIB_ADSL_LAN_IP2, buf);
	nBytesSent += boaWrite(wp, "<div ID=\"secondIP\" style=\"display:none\">\n");
	nBytesSent += boaWrite(wp, "<table>\n");
	nBytesSent += boaWrite(wp, "<tr><th>%s:</th>\n", multilang(LANG_IP_ADDRESS));
	nBytesSent += boaWrite(wp, "<td><input type=text name=ip2 size=15 maxlength=15 ");
	nBytesSent += boaWrite(wp, "value=%s></td>\n</tr>\n", buf);

	getMIB2Str(MIB_ADSL_LAN_SUBNET2, buf);
	nBytesSent += boaWrite(wp, "<tr><th>%s:</th>\n", multilang(LANG_SUBNET_MASK));
	nBytesSent += boaWrite(wp, "<td><input type=text name=mask2 size=15 maxlength=15 ");
	nBytesSent += boaWrite(wp, "value=%s></td>\n</tr>\n", buf);

	nBytesSent += boaWrite(wp, "<input type=hidden name=chk_port_mask2>\n");

#ifndef DHCPS_POOL_COMPLETE_IP
	nBytesSent += boaWrite(wp, "<tr></tr><tr><th>DHCP %s:</th>\n", multilang(LANG_POOL));
	nBytesSent += boaWrite(wp, "<td>\n<input type=\"radio\"");
	nBytesSent += boaWrite(wp, " name=dhcpuse value=0>Primary LAN&nbsp;&nbsp;\n");
	nBytesSent += boaWrite(wp, "<input type=\"radio\" name=dhcpuse value=1>Secondary LAN</td>\n</tr>");
#endif
	nBytesSent += boaWrite(wp, "</table></div>\n");
#endif

	if (pUser_info->priv) {
#ifdef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<div class=\"data_common data_common_notitle\">");
#endif
#if (defined(CONFIG_RTL_IGMP_SNOOPING) || defined(CONFIG_BRIDGE_IGMP_SNOOPING)) && !defined(CONFIG_SFU)
	nBytesSent += boaWrite(wp, "<table>\n");
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr><th>%s:</th>\n", multilang(LANG_IGMP_SNOOPING));
	nBytesSent += boaWrite(wp, "<td>\n<input type=\"radio\"");
#else
	nBytesSent += boaWrite(wp, "<tr><th width=\"30%%\">%s:</th>\n", multilang(LANG_IGMP_SNOOPING));
	nBytesSent += boaWrite(wp, "<td width=\"70%%\">\n<input type=\"radio\"");
#endif
	nBytesSent += boaWrite(wp, " name=snoop value=0>%s&nbsp;&nbsp;\n", multilang(LANG_DISABLED));
	nBytesSent += boaWrite(wp, "<input type=\"radio\" name=snoop value=1>%s</td>\n</tr></table>\n", multilang(LANG_ENABLED));
#endif

#ifdef WLAN_SUPPORT
	nBytesSent += boaWrite(wp, "<table>\n");
#ifndef CONFIG_GENERAL_WEB
  	nBytesSent += boaWrite(wp, "<tr><th>%s:</th>\n", multilang(LANG_ETHERNET_TO_WIRELESS_BLOCKING));
  	nBytesSent += boaWrite(wp, "<td>\n");
#else
	nBytesSent += boaWrite(wp, "<tr><th width=\"30%%\">%s:</th>\n", multilang(LANG_ETHERNET_TO_WIRELESS_BLOCKING));
  	nBytesSent += boaWrite(wp, "<td width=\"70%%\">\n");
#endif
  	nBytesSent += boaWrite(wp, "<input type=\"radio\" name=BlockEth2Wir value=0>%s&nbsp;&nbsp;\n", multilang(LANG_DISABLED));
  	nBytesSent += boaWrite(wp, "<input type=\"radio\" name=BlockEth2Wir value=1>%s</td></tr></table>\n", multilang(LANG_ENABLED));
#endif

#ifdef CONFIG_USER_LLMNR 
	nBytesSent += boaWrite(wp, "<table>\n");
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr><th>%s:</th>\n", multilang(LANG_LLMNR));
  	nBytesSent += boaWrite(wp, "<td>\n");
#else	
	nBytesSent += boaWrite(wp, "<tr><th width=\"30%%\">%s:</th>\n", multilang(LANG_LLMNR));
	nBytesSent += boaWrite(wp, "<td width=\"70%%\">\n");
#endif 	
	nBytesSent += boaWrite(wp, "<input type=\"radio\" name=llmnr value=0>%s&nbsp;&nbsp;\n", multilang(LANG_DISABLED));
  	nBytesSent += boaWrite(wp, "<input type=\"radio\" name=llmnr value=1>%s</td></tr></table>\n", multilang(LANG_ENABLED));
#endif

#ifdef CONFIG_USER_MDNS 
	nBytesSent += boaWrite(wp, "<table>\n");
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr><th>%s:</th>\n", multilang(LANG_MDNS));
  	nBytesSent += boaWrite(wp, "<td>\n");
#else	
	nBytesSent += boaWrite(wp, "<tr><th width=\"30%%\">%s:</th>\n", multilang(LANG_MDNS));
	nBytesSent += boaWrite(wp, "<td width=\"70%%\">\n");
#endif 	
	nBytesSent += boaWrite(wp, "<input type=\"radio\" name=mdns value=0>%s&nbsp;&nbsp;\n", multilang(LANG_DISABLED));
  	nBytesSent += boaWrite(wp, "<input type=\"radio\" name=mdns value=1>%s</td></tr></table>\n", multilang(LANG_ENABLED));
#endif


#ifdef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "</div>\n");
#endif
	}

#if defined(CONFIG_LUNA) && defined(CONFIG_RTL_MULTI_LAN_DEV)
	nBytesSent += boaWrite(wp, "<div class=\"data_common data_common_notitle\">\n");
	nBytesSent += boaWrite(wp, "<table>\n");
	for(i = 0 ; i < SW_LAN_PORT_NUM ; i++)
	{
		nBytesSent += boaWrite(wp, "<tr><th width=\"30%%\">%s%d:</th>\n", multilang(LANG_LAN), i+1);
		nBytesSent += boaWrite(wp, "<td width=\"70%%\">\n<input type=\"radio\" name=lan%d value=0>%s&nbsp;&nbsp;\n", i, multilang(LANG_DISABLED));
		nBytesSent += boaWrite(wp, "<input type=\"radio\" name=lan%d value=1>%s</td>\n</tr>\n", i, multilang(LANG_ENABLED));
	}
	nBytesSent += boaWrite(wp, "</table>\n</div>\n");

#ifdef USE_DEONET /* sghong, 20221123 */
	nBytesSent += boaWrite(wp, "<div class=\"data_common data_common_notitle\">\n");
	nBytesSent += boaWrite(wp, "<table>\n");
	for(i = 0 ; i < SW_LAN_PORT_NUM ; i++)
	{
		nBytesSent += boaWrite(wp, "<tr><th width=\"30%%\">LAN%d FC:</th>\n", i+1);
		nBytesSent += boaWrite(wp, "<td width=\"70%%\">\n<input type=\"radio\" name=lan%d_fc value=0>%s&nbsp;&nbsp;\n", i, "Off");
		nBytesSent += boaWrite(wp, "<input type=\"radio\" name=lan%d_fc value=1>%s&nbsp;&nbsp;\n", i, "Symm");
		nBytesSent += boaWrite(wp, "<input type=\"radio\" name=lan%d_fc value=2>%s&nbsp;&nbsp;\n", i, "ASymm");
		nBytesSent += boaWrite(wp, "<input type=\"radio\" name=lan%d_fc value=3>%s</td>\n</tr>\n", i, "Both");
	}
	nBytesSent += boaWrite(wp, "</table>\n</div>\n");
#endif

	nBytesSent += boaWrite(wp, "<div class=\"data_common data_common_notitle\">\n");
	nBytesSent += boaWrite(wp, "<table>\n");
	for(i = 0 ; i < SW_LAN_PORT_NUM ; i++)
	{
		nBytesSent += boaWrite(wp, "<tr><th width=\"30%%\">LAN%d GigaLite:</th>\n", i + 1);
		nBytesSent += boaWrite(wp, "<td width=\"70%%\">\n<input type=\"radio\" name=lan%d_GL value=0>%s&nbsp;&nbsp;\n", i, "Disable");
		nBytesSent += boaWrite(wp, "<input type=\"radio\" name=lan%d_GL value=1>%s</td>\n</tr>\n", i, "Enable");
	}
	nBytesSent += boaWrite(wp, "</table>\n</div>\n");
#endif

	return nBytesSent;
}

int checkIP2(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;

#ifdef CONFIG_SECONDARY_IP
	nBytesSent += boaWrite(wp, "if (document.tcpip.enable_ip2.checked) {\n");
	nBytesSent += boaWrite(wp, "\tif (!checkIP(document.tcpip.ip2))\n");
	nBytesSent += boaWrite(wp, "\t\treturn false;\n");
	nBytesSent += boaWrite(wp, "\tif (!checkMask(document.tcpip.mask2))\n");
	nBytesSent += boaWrite(wp, "\t\treturn false;\n");
	nBytesSent += boaWrite(wp, "\t\tif (checkLan1andLan2(document.tcpip.ip, document.tcpip.mask, document.tcpip.ip2, document.tcpip.mask2) == false) {\n");
	nBytesSent += boaWrite(wp, "\t\talert(\"Network Address Conflict !\");\n");
	nBytesSent += boaWrite(wp, "\t\tdocument.tcpip.ip2.value=document.tcpip.ip2.defaultValue;\n");
	nBytesSent += boaWrite(wp, "\t\tdocument.tcpip.ip2.focus();\n");
	nBytesSent += boaWrite(wp, "\t\treturn false;}}\n");
#endif
	return nBytesSent;
}

int lan_script(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;

#ifdef CONFIG_SECONDARY_IP
	nBytesSent += boaWrite(wp, "function updateInput()\n");
	nBytesSent += boaWrite(wp, "{\n\tif (document.tcpip.enable_ip2.checked == true) {\n");
	nBytesSent += boaWrite(wp, "\t\tif (document.getElementById)  // DOM3 = IE5, NS6\n");
	nBytesSent += boaWrite(wp, "\t\t\tdocument.getElementById('secondIP').style.display = 'block';\n");
	nBytesSent += boaWrite(wp, "\t\t\telse {\n");
	nBytesSent += boaWrite(wp, "\t\t\tif (document.layers == false) // IE4\n");
	nBytesSent += boaWrite(wp, "\t\t\t\tdocument.all.secondIP.style.display = 'block';\n");
	nBytesSent += boaWrite(wp, "\t\t}\n");
	nBytesSent += boaWrite(wp, "\t} else {\n");
	nBytesSent += boaWrite(wp, "\t\tif (document.getElementById)  // DOM3 = IE5, NS6\n");
	nBytesSent += boaWrite(wp, "\t\t\tdocument.getElementById('secondIP').style.display = 'none';\n");
	nBytesSent += boaWrite(wp, "\t\telse {\n");
	nBytesSent += boaWrite(wp, "\t\t\tif (document.layers == false) // IE4\n");
	nBytesSent += boaWrite(wp, "\t\t\t\tdocument.all.secondIP.style.display = 'none';\n");
	nBytesSent += boaWrite(wp, "\t\t}\n");
	nBytesSent += boaWrite(wp, "\t}\n");
	nBytesSent += boaWrite(wp, "}");
#endif
	return nBytesSent;
}

int lan_port_mask(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
#ifdef CONFIG_SECONDARY_IP
	int i;
	nBytesSent += boaWrite(wp, "<th>%s:</th>", multilang(LANG_PORT_MASK));
	nBytesSent += boaWrite(wp, "<td>");

	for (i=PMAP_ETH0_SW0; i < PMAP_WLAN0 && i<SW_LAN_PORT_NUM; i++)
			nBytesSent += boaWrite(wp, "<input type=checkbox name=chk_port_mask1>Port %d", i);
	nBytesSent += boaWrite(wp, "</td>");
#endif
	return nBytesSent;
}

#ifdef CONFIG_USER_DHCPCLIENT_MODE
int dhcpc_script(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	unsigned char cur_vChar;

	mib_get_s(MIB_DHCP_MODE, (void *)&cur_vChar, sizeof(cur_vChar));

	nBytesSent += boaWrite(wp, "function updateDHCPClient(){\n");
	nBytesSent += boaWrite(wp, "  if(%d != %d){\n", DHCPV4_LAN_CLIENT, cur_vChar);
	nBytesSent += boaWrite(wp, "	document.getElementById(\"ip\").readOnly = false;\n");
	nBytesSent += boaWrite(wp, "	document.getElementById(\"ip\").disabled = false;\n");
	nBytesSent += boaWrite(wp, "	document.getElementById(\"mask\").readOnly = false;\n");
	nBytesSent += boaWrite(wp, "	document.getElementById(\"mask\").disabled = false;\n");
	nBytesSent += boaWrite(wp, "  }else{\n");
	nBytesSent += boaWrite(wp, "	document.getElementById(\"ip\").readOnly = true;\n");
	nBytesSent += boaWrite(wp, "	document.getElementById(\"ip\").disabled = true;\n");
	nBytesSent += boaWrite(wp, "	document.getElementById(\"mask\").readOnly = true;\n");
	nBytesSent += boaWrite(wp, "	document.getElementById(\"mask\").disabled = true;\n");
	nBytesSent += boaWrite(wp, "  }\n");
	nBytesSent += boaWrite(wp, "}\n");

	return nBytesSent;
}

int dhcpc_clicksetup(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	nBytesSent += boaWrite(wp, "if (document.dhcpd.dhcpdenable[3].checked == true){\n");
	nBytesSent += boaWrite(wp, "	document.getElementById(\'displayDhcpSvr\').innerHTML=\n");
	nBytesSent += boaWrite(wp, "		'<input type=\"submit\" value=\"%s\" name=\"save\" onClick=\"return saveClick(0)\">&nbsp;&nbsp;';\n", multilang(LANG_APPLY_CHANGES));
	nBytesSent += boaWrite(wp, "}\n");
	return nBytesSent;
}
#endif

int tcpip_lan_oninit(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
#if defined(CONFIG_IPV6)
	unsigned int prefix_delegation_wanconn=DUMMY_IFINDEX;
	unsigned char prefix_mode;
	unsigned int dns_wan_conn;
	unsigned char dns_mode;
#endif

#if defined(CONFIG_IPV6)
	mib_get_s(MIB_PREFIXINFO_DELEGATED_WANCONN, (void *)&prefix_delegation_wanconn, sizeof(prefix_delegation_wanconn));
	mib_get_s(MIB_PREFIXINFO_PREFIX_MODE, (void *)&prefix_mode,sizeof(prefix_mode));
	mib_get_s(MIB_DNSINFO_WANCONN, (void *)&dns_wan_conn, sizeof(dns_wan_conn));
	mib_get_s(MIB_LAN_DNSV6_MODE, (void *)&dns_mode, sizeof(dns_mode));
#endif

	nBytesSent += boaWrite(wp, "function on_init(){\n");
#if defined(CONFIG_IPV6)
	nBytesSent += boaWrite(wp, "   with (document.forms[0]){\n");
	//nBytesSent += boaWrite(wp, "	var ifIdx = %d;\n",prefix_delegation_wanconn);
	//nBytesSent += boaWrite(wp, "	if (ifIdx != 65535)\n");
	//nBytesSent += boaWrite(wp, "	  if_delegated_wan.value = ifIdx;\n");
	//nBytesSent += boaWrite(wp, "	else\n");
	//nBytesSent += boaWrite(wp, "      if_delegated_wan.selectedIndex = 0;\n");
	nBytesSent += boaWrite(wp, "	ipv6lanprefixmode.value = %d\n",prefix_mode);
	nBytesSent += boaWrite(wp, "	ifIdx = %d;\n",dns_wan_conn);
	nBytesSent += boaWrite(wp, "	if (ifIdx != 65535)\n");
	nBytesSent += boaWrite(wp, "	  if_dns_wan.value = ifIdx;\n");
	nBytesSent += boaWrite(wp, "    else\n");
	nBytesSent += boaWrite(wp, "      if_dns_wan.selectedIndex = 0;\n");
	nBytesSent += boaWrite(wp, "    ipv6landnsmode.value=%d;\n",dns_mode);
	nBytesSent += boaWrite(wp, "    prefixModeChange();\n");
	nBytesSent += boaWrite(wp, "    dnsModeChange();\n");
	nBytesSent += boaWrite(wp, "  }\n");
#endif

	nBytesSent += boaWrite(wp, "}\n");

	return nBytesSent;
}


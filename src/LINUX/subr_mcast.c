#include <stdlib.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "utility.h"
#include "subr_net.h"
#include "sockmark_define.h"
#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#include "fc_api.h"
#endif
#ifdef CONFIG_RTK_OMCI_V1
#include <omci_dm_sd.h>
#endif
#if defined(CONFIG_CTC_SDN)
#include "subr_ctcsdn.h"
#endif
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
#include <rt_igmpHook_ext.h>
const char WHITELIST_DEFAULT_DROP[] = "0.0.0.0";
const char WHITELIST_DEFAULT_ACCEPT_START[] = "224.0.0.0";
const char WHITELIST_DEFAULT_ACCEPT_END[] = "239.255.255.255";
#endif

int call_count = 0;

///--IGMP/MLD functions add here
int isIGMPSnoopingEnabled(void)
{
	unsigned char mode;

	mib_get_s(MIB_MPMODE, (void *)&mode, sizeof(mode));
	return ((mode&MP_IGMP_MASK)==MP_IGMP_MASK)?1:0;
}

int isMLDSnoopingEnabled(void)
{
	unsigned char mode;

	mib_get_s(MIB_MPMODE, (void *)&mode, sizeof(mode));
	return ((mode&MP_MLD_MASK)==MP_MLD_MASK)?1:0;
}


#if defined(CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE)
int rtk_igmp_mld_snooping_module_enable(char ipVersion, char enable)
{
	char ifName[IFNAMSIZ];
	int j;
	char enable_mod = enable;
	rt_igmpHook_devInfo_t devInfo = {0};
	int max_lan_port_num=0, phyPortId=0;
	int ethPhyPortId = rtk_port_get_wan_phyID();

#if defined(CONFIG_CMCC)
	unsigned char isCMCCTestMode = 0;
	mib_get_s(MIB_IGMP_MLD_SNOOPING_CMCC_TEST,  (void *)&isCMCCTestMode, sizeof(isCMCCTestMode));
	if(isCMCCTestMode == 1){
		enable_mod = 1; //cmcc always enable igmp/mld snooping
	}
#endif

	if(ipVersion & IPVER_IPV4)
	{
		 for(j = 0; j < ELANVIF_NUM; j++){
		{
			phyPortId = rtk_port_get_lan_phyID(j);
			if (phyPortId != -1 && phyPortId != ethPhyPortId)
			{
				sprintf(ifName, "%s", ELANVIF[j]);
				strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
				devInfo.devIfidx = get_dev_ifindex(ifName);
				rt_igmpHook_devConfig_set(devInfo , 0 ,RT_SNOOPING_DISABLE , !enable_mod);
			}
		 }
	}
#if 0//def WLAN_SUPPORT
		for(j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)
		{
			if(wlan_en[j-PMAP_WLAN0] == 0)
				continue;
			sprintf(ifName, "%s", wlan[j-PMAP_WLAN0]);
			strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
			devInfo.devIfidx = get_dev_ifindex(ifName);
			rt_igmpHook_devConfig_set(devInfo , 0 ,RT_SNOOPING_DISABLE , !enable);
		}
#endif
	}
	if(ipVersion & IPVER_IPV6)
	{
		for(j = 0; j < ELANVIF_NUM; j++){
		{
			phyPortId = rtk_port_get_lan_phyID(j);
			if (phyPortId != -1 && phyPortId != ethPhyPortId)
			{
				sprintf(ifName, "%s", ELANVIF[j]);
				strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
				devInfo.devIfidx = get_dev_ifindex(ifName);
				rt_igmpHook_devConfig_set(devInfo , 1 ,RT_SNOOPING_DISABLE , !enable_mod);
			}
		}
	}
#if 0//def WLAN_SUPPORT
		for(j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)
		{
			if(wlan_en[j-PMAP_WLAN0] == 0)
				continue;
			sprintf(ifName, "%s", wlan[j-PMAP_WLAN0]);
			strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
			devInfo.devIfidx = get_dev_ifindex(ifName);
			rt_igmpHook_devConfig_set(devInfo , 0 ,RT_SNOOPING_DISABLE , !enable);
		}
#endif
	}

	//set ignoreGroup rules
	rtk_set_multicast_ignoreGroup();

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	if(ipVersion & IPVER_IPV4)
	{
		if(enable_mod){
			va_cmd("/bin/sh", 2, 1 ,"-c", "echo 0 > /proc/fc/ctrl/igmp_unknown_flood");
		}
		else{
			va_cmd("/bin/sh", 2, 1 ,"-c", "echo 1 > /proc/fc/ctrl/igmp_unknown_flood");
		}
	}
#endif

	return 0;
}

int rtk_igmp_mld_snooping_module_init(const char *brName, int configAll, MIB_CE_ATM_VC_Tp pEntry)
{
	MIB_CE_ATM_VC_T atmVcEntry;
	unsigned int i, atmVcEntryNum;
	char ifname[IFNAMSIZ];
	char sysbuf[256] = {0};

	if(brName != NULL)
	{
		sprintf(sysbuf, "echo 1 > /sys/class/net/%s/bridge/multicast_snooping", brName);
		//AUG_PRT(">>>>>>>>> %s\n", sysbuf);
		va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
		sprintf(sysbuf, "echo 2 > sys/devices/virtual/net/%s/bridge/multicast_router", brName);
		va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
	}

	atmVcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<atmVcEntryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&atmVcEntry))
		{
  			printf("%s %d: mib_chain_get FAIL !\n", __func__, __LINE__);
			return -1;
		}

		if(pEntry!=NULL && configAll == CONFIGONE)
		{
			if(atmVcEntry.vlan != pEntry->vlan)
				continue;

			if(atmVcEntry.vid != pEntry->vid)
				continue;

			if(atmVcEntry.cmode != pEntry->cmode)
				continue;
		}

		if(atmVcEntry.cmode == CHANNEL_MODE_BRIDGE)
		{
			ifGetName(atmVcEntry.ifIndex, ifname, sizeof(ifname));
			sprintf(sysbuf, "echo 2 > /sys/devices/virtual/net/%s/brport/multicast_router", ifname);
			//AUG_PRT(">>>>>>>>> %s\n", sysbuf);
			va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
		}
	}

	//set ignoreGroup rules
	rtk_set_multicast_ignoreGroup();

	// echo MRRK2_EXTPORT_EN [startbit] MRRK2_EXTPORTID_START [startbit] MRRK2_EXTPORTID_LEN [portid bitLen] > proc/igmp/ctrl/skbMarkExtraSwitchPortId
	sprintf(sysbuf, "echo MRRK2_EXTPORT_EN %d MRRK2_EXTPORTID_START %d MRRK2_EXTPORTID_LEN %d > proc/igmp/ctrl/skbMarkExtraSwitchPortId"
		, SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_START
		, SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_START
		, SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_NUM);
	//AUG_PRT(">>>>>>>>> %s\n", sysbuf);
	va_cmd("/bin/sh", 2, 1, "-c", sysbuf);

	return 0;
}

int rtk_set_all_lan_igmp_blacklist_by_mac(int is_set, char *mac)
{
	int j = 0;
	char tmp_cmd[256] = {0};

	if (mac == NULL)
	{
		printf("%s error mac input!\n", __FUNCTION__);
		return -1;
	}

	if (is_set)
	{
		//printf("[%s:%d] %s igmp blacklist with mac: %s\n", __FUNCTION__, __LINE__, "set", mac);
		for (j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < SW_LAN_PORT_NUM; ++j)
		{
			snprintf(tmp_cmd, sizeof(tmp_cmd), "echo DEVIFNAME %s ISIP6 0 SMAC %s > %s", SW_LAN_PORT_IF[j], mac, RTK_IGMP_MLD_SNOOPING_MODULE_BLACKLIST_PROC_CONFIG);
			va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);
#ifdef CONFIG_IPV6
			snprintf(tmp_cmd, sizeof(tmp_cmd), "echo DEVIFNAME %s ISIP6 1 SMAC %s > %s", SW_LAN_PORT_IF[j], mac, RTK_IGMP_MLD_SNOOPING_MODULE_BLACKLIST_PROC_CONFIG);
			va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);
#endif
		}
#ifdef WLAN_SUPPORT
		for (j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)
		{
			if (wlan_en[j-PMAP_WLAN0] == 0)
				continue;

			snprintf(tmp_cmd, sizeof(tmp_cmd), "echo DEVIFNAME %s ISIP6 0 SMAC %s > %s", wlan[j - PMAP_WLAN0], mac, RTK_IGMP_MLD_SNOOPING_MODULE_BLACKLIST_PROC_CONFIG);
			va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);
#ifdef CONFIG_IPV6
			snprintf(tmp_cmd, sizeof(tmp_cmd), "echo DEVIFNAME %s ISIP6 1 SMAC %s > %s", wlan[j - PMAP_WLAN0], mac, RTK_IGMP_MLD_SNOOPING_MODULE_BLACKLIST_PROC_CONFIG);
			va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);
#endif
		}
#endif
	}
	else
	{
		//printf("[%s:%d] %s igmp blacklist with mac: %s\n", __FUNCTION__, __LINE__, "clear", mac);
		for (j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < SW_LAN_PORT_NUM; ++j)
		{
			snprintf(tmp_cmd, sizeof(tmp_cmd), "echo DELBY_PATTEN 1 DEVIFNAME %s ISIP6 0 SMAC %s > %s", SW_LAN_PORT_IF[j], mac, RTK_IGMP_MLD_SNOOPING_MODULE_BLACKLIST_PROC_CONFIG);
			va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);
#ifdef CONFIG_IPV6
			snprintf(tmp_cmd, sizeof(tmp_cmd), "echo DELBY_PATTEN 1 DEVIFNAME %s ISIP6 1 SMAC %s > %s", SW_LAN_PORT_IF[j], mac, RTK_IGMP_MLD_SNOOPING_MODULE_BLACKLIST_PROC_CONFIG);
			va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);
#endif
		}
#ifdef WLAN_SUPPORT
		for (j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)
		{
			if (wlan_en[j-PMAP_WLAN0] == 0)
				continue;

			snprintf(tmp_cmd, sizeof(tmp_cmd), "echo DELBY_PATTEN 1 DEVIFNAME %s ISIP6 0 SMAC %s > %s", wlan[j - PMAP_WLAN0], mac, RTK_IGMP_MLD_SNOOPING_MODULE_BLACKLIST_PROC_CONFIG);
			va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);
#ifdef CONFIG_IPV6
			snprintf(tmp_cmd, sizeof(tmp_cmd), "echo DELBY_PATTEN 1 DEVIFNAME %s ISIP6 1 SMAC %s > %s", wlan[j - PMAP_WLAN0], mac, RTK_IGMP_MLD_SNOOPING_MODULE_BLACKLIST_PROC_CONFIG);
			va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);
#endif
		}
#endif
	}

	return 0;
}
#endif

#if defined(CONFIG_USER_RTK_NOSTB_IPTV)
void __dev_NOSTBsetting(int flag)
{
	char nostb_cmd[120];
	int i, mibtotal;
	MIB_CE_ATM_VC_T tmpEntry;
	char upstreamwan_ifname[IFNAMSIZ], otherwan_ifname[IFNAMSIZ];

	memset(&upstreamwan_ifname,'\0',sizeof(upstreamwan_ifname));
	memset(&otherwan_ifname,'\0',sizeof(otherwan_ifname));

//	should check applicationtype to bind wan
//	search the first upstreamwan or other wan. if both does not exist, choose default wan.

	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0; i< mibtotal; i++)
	{
		mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tmpEntry);
		if(tmpEntry.UpstreamWan) //check upstreamwan
		{
			ifGetName(PHY_INTF(tmpEntry.ifIndex), upstreamwan_ifname, sizeof(upstreamwan_ifname));
			break;
		}
		if(tmpEntry.applicationtype&X_CT_SRV_OTHER)
			ifGetName(PHY_INTF(tmpEntry.ifIndex), otherwan_ifname, sizeof(otherwan_ifname));
	}

	if(flag){
		va_niced_cmd("/bin/sh", 2, 1, "-c", "touch /config/xupnpd.m3u");

		if(strlen(upstreamwan_ifname)!=0)
		{
			sprintf(nostb_cmd, "udpxy -p 9090 -m %s -c 1000 -B 65536 -H 5", upstreamwan_ifname);
			va_niced_cmd("/bin/sh" , 2, 0, "-c", nostb_cmd);

			sprintf(nostb_cmd, "echo %s > /tmp/udpxy_ifname", upstreamwan_ifname);
			va_niced_cmd("/bin/sh" , 2, 1, "-c", nostb_cmd);
		}
		else if(strlen(otherwan_ifname)!=0)
		{
			sprintf(nostb_cmd, "udpxy -p 9090 -m %s -c 1000 -B 65536 -H 5", otherwan_ifname);
			va_niced_cmd("/bin/sh" , 2, 0, "-c", nostb_cmd);

			sprintf(nostb_cmd, "echo %s > /tmp/udpxy_ifname", otherwan_ifname);
			va_niced_cmd("/bin/sh" , 2, 1, "-c", nostb_cmd);
		}
		else
			va_niced_cmd("/bin/sh", 2, 0, "-c", "udpxy -p 9090 -c 1000 -B 65536 -H 5");
			//without binding interface default use default wan

		va_niced_cmd("/bin/sh", 2, 0, "-c", "xupnpd");
	}
	else{
		va_niced_cmd("/bin/sh", 2, 1, "-c", "killall -9 udpxy");
		va_niced_cmd("/bin/sh", 2, 1, "-c", "killall -9 xupnpd");
	}

	printf("NO STB setting: %s\n", flag?"enabled":"disabled");
}

int startNOSTB(char* ifname)
{
	unsigned char mode;
	int i, mibtotal;
	MIB_CE_ATM_VC_T tmpEntry;
	char tmp_ifname[IFNAMSIZ] = "";

	printf("[%s:%d]startNOSTB process, ifname = %s\n", __func__, __LINE__, ifname);

	mib_get(CWMP_CT_IPTV_NOSTBENABLE, (void *)&mode);
	if(!mode)
	{
		printf("[%s:%d]NOSTB is disable.\n", __func__, __LINE__);
		return 0;//nostb is disable
	}

	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0; i< mibtotal; i++)
	{
		mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tmpEntry);

		ifGetName(PHY_INTF(tmpEntry.ifIndex), tmp_ifname, sizeof(tmp_ifname));
		if(!strncmp(tmp_ifname, ifname, sizeof(ifname)) || !strncmp("restart", ifname, sizeof(ifname)))// current wan change
		{
			__dev_NOSTBsetting(0);

			if (mode)
				__dev_NOSTBsetting(1);
			break;
		}
	}

	return 1;
}
#endif

#if defined(CONFIG_RTL_IGMP_SNOOPING)
// enable/disable IGMP snooping
void __dev_setupIGMPSnoop(int flag)
{
	struct ifreq ifr;
	struct ifvlan ifvl;
	unsigned char mode;

#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
		rtk_igmp_mld_snooping_module_enable(IPVER_IPV4, flag);
#else
	if(flag){
		system("/bin/echo 1 > /proc/br_igmpsnoop");
		system("/bin/echo 1 > /proc/br_igmpquery");
	}
	else{
		system("/bin/echo 0 > /proc/br_igmpsnoop");
		system("/bin/echo 0 > /proc/br_igmpquery");
	}
#endif
	printf("IGMP Snooping: %s\n", flag?"enabled":"disabled");

	block_br_lan_igmp_mld_rules();
}
#endif
// Mason Yu. MLD snooping
//#ifdef CONFIG_IPV6
#if defined(CONFIG_RTL_MLD_SNOOPING)
// enable/disable MLD snooping
void __dev_setupMLDSnoop(int flag)
{
	unsigned char mode;

#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rtk_igmp_mld_snooping_module_enable(IPVER_IPV6, flag);
#else
	if(flag){
		system("/bin/echo 1 > /proc/br_mldsnoop");
		system("/bin/echo 1 > /proc/br_mldquery");
	}
	else{
		system("/bin/echo 0 > /proc/br_mldsnoop");
		system("/bin/echo 0 > /proc/br_mldquery");
	}
#endif
	printf("MLD Snooping: %s\n", flag?"enabled":"disabled");

	block_br_lan_igmp_mld_rules();
}
#endif
#if defined(CONFIG_RTL_IGMP_SNOOPING) || defined(CONFIG_RTL_MLD_SNOOPING) || defined(CONFIG_BRIDGE_IGMP_SNOOPING)
#ifdef CONFIG_BRIDGE_IGMP_SNOOPING
void config_bridge_igmp_snooping(const char *brif, unsigned int enable){
	DIR *dir=NULL;
	struct dirent *next=NULL;
	char dir_path[128]={0}, cmd[256]={0};

	snprintf(dir_path, sizeof(dir_path), "/sys/devices/virtual/net/%s/brif/", brif);

	dir = opendir(dir_path);
	if (!dir) {
		/* Directory does not exist. */
		fprintf(stderr, "[%s] open %s fail\n", __FUNCTION__, dir_path);
		return;
	}

	while ((next = readdir(dir)) != NULL) {
		/* Must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, ".") == 0 || strcmp(next->d_name, "..") == 0)
			continue;

		sprintf(cmd, "echo %d > /sys/devices/virtual/net/%s/brif/%s/multicast_fast_leave", enable, brif, next->d_name);
		system(cmd);

#ifdef CONFIG_BOLOCK_IPTV_FROM_LAN_SERVER
		if(getOpMode() == GATEWAY_MODE)
		{
#endif
		if(
#ifdef CONFIG_RTL_MULTI_LAN_DEV
		(!strncmp(next->d_name, ALIASNAME_ELAN_PREFIX, strlen(ALIASNAME_ELAN_PREFIX)))
#else
		(!strncmp(next->d_name, ELANIF, strlen(ELANIF)))
#endif
		|| (!strncmp(next->d_name, ALIASNAME_WLAN, strlen(ALIASNAME_WLAN)))
		)
		{
			/* if LAN host receive multicast report, it will not send same report anymore.
			 * it will cause snooping function error. So, we avoid report flood to lan side. */
			sprintf(cmd, "echo 1 > /sys/devices/virtual/net/%s/brif/%s/multicast_router", brif, next->d_name);
			system(cmd);
		}else{
			sprintf(cmd, "echo %d > /sys/devices/virtual/net/%s/brif/%s/multicast_router", (enable)?2:1, brif, next->d_name);
			system(cmd);
		}
#ifdef CONFIG_BOLOCK_IPTV_FROM_LAN_SERVER
		}
#endif
	}

	if(dir)closedir(dir);
	return;
}
#endif //CONFIG_BRIDGE_IGMP_SNOOPING

int rtk_set_multicast_ignoreGroup_rule(int config, int *pindex, const char *start_group, const char *end_greoup, int force2ps)
{
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rt_igmpHook_ignoreGroup_t patten;
	int ret=0, index;
	unsigned int ignipS[4], ignipE[4];
	unsigned char is_IPv6;

	memset(ignipS, 0, sizeof(ignipS));
	memset(ignipE, 0, sizeof(ignipE));

	if(!start_group && !end_greoup && (config || !pindex || *pindex<0))
		return -1;
	else if(config == 0 && pindex && *pindex >= 0) {
		index = *pindex;
		memset(&patten,0,sizeof(patten));
		ret = rt_igmpHook_groupToPsTbl_del(&patten, index);
	} else {
		if((!start_group || inet_pton(AF_INET, start_group, (void *)&ignipS[0]) > 0) &&
			(!end_greoup || inet_pton(AF_INET, end_greoup, (void *)&ignipE[0]) > 0))
		{
			is_IPv6 = 0;
			ignipS[0] = ntohl(ignipS[0]);
			ignipE[0] = ntohl(ignipE[0]);
		}
		else if((!start_group || inet_pton(AF_INET6, start_group, (void *)&ignipS[0]) > 0) &&
				(!end_greoup || inet_pton(AF_INET6, end_greoup, (void *)&ignipE[0]) > 0))
		{
			is_IPv6 = 1;
		}
		else {
			return -1;
		}

		memset(&patten,0,sizeof(patten));
		patten.isIpv6 = is_IPv6;
		patten.isCopy2cpu = (force2ps)?0:1;
		memcpy(patten.ignGroupIpStart, ignipS, sizeof(patten.ignGroupIpStart));
		memcpy(patten.ignGroupIpEnd, ignipE, sizeof(patten.ignGroupIpEnd));
		if(config == 0)
			ret = rt_igmpHook_groupToPsTbl_del(&patten, index);
		else {
			ret = rt_igmpHook_groupToPsTbl_add(&patten, &index);
			if(ret == 0 && pindex) *pindex = index;
		}
	}
	return ret;
#else
	return -1;
#endif
}

#define RTK_IGMP_IGNORE_GROUP "/var/rtk_igmp_ignore_group_idx"
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
#define FC_MULTICAST_IGNOREGROUP_TRAP_RULE "/var/fc_multicast_ignoregroup_trap_rule"
#endif
void rtk_set_multicast_ignoreGroup(void)
{
	MIB_PROXY_IGNGROUP_T tmpEntry;
	int i=0, totalEntry, index;
	FILE *fp;
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	FILE *fp_fc;
#endif

	fp = fopen(RTK_IGMP_IGNORE_GROUP, "r");
	if(fp) {
		while(!feof(fp))
		{
			if(fscanf(fp, "%d\n", &index))
			{
				rtk_set_multicast_ignoreGroup_rule(0, &index, NULL, NULL, 0);
			}
		}
		fclose(fp);
	}
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	fp_fc = fopen(FC_MULTICAST_IGNOREGROUP_TRAP_RULE, "r");
	if(fp_fc) {
		while(!feof(fp_fc))
		{
			if(fscanf(fp_fc, "%d\n", &index))
			{
				rtk_fc_multicast_ignoregroup_trap_rule(0, &index, NULL, NULL);
			}
		}
		fclose(fp_fc);
	}
#endif

	fp = fopen(RTK_IGMP_IGNORE_GROUP, "w");
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	fp_fc = fopen(FC_MULTICAST_IGNOREGROUP_TRAP_RULE, "w");
#endif
	totalEntry = mib_chain_total(MIB_PROXY_IGNGROUP_TBL);
	for(i=0;i<totalEntry;i++)
	{
		if(mib_chain_get(MIB_PROXY_IGNGROUP_TBL, i, &tmpEntry)!=1)
			continue;

		if(fp && rtk_set_multicast_ignoreGroup_rule(1, &index, tmpEntry.ignGStart, tmpEntry.ignGEnd, 1) == 0){
			fprintf(fp, "%d\n", index);
		}
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
		if(tmpEntry.acl_set) {
			if(fp_fc && rtk_fc_multicast_ignoregroup_trap_rule(1, &index, tmpEntry.ignGStart, tmpEntry.ignGEnd) == 0) {
				fprintf(fp_fc, "%d\n", index);
			}
		}
#endif
	}
	if(fp) fclose(fp);

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	if(fp_fc) fclose(fp_fc);
#endif
}

#endif

// handle IGMP/MLD snooping function, this api should call after mib_set(MIB_MPMODE)
void rtk_multicast_snooping(void){
	int flag=0;
	unsigned char mode;
#if defined(CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE)
	char ifName[IFNAMSIZ];
	int j;
#endif
#if defined(CONFIG_BRIDGE_IGMP_SNOOPING)
	char cmd[128]={0};
#endif


	mib_get_s(MIB_MPMODE, (void *)&mode, sizeof(mode));

#if defined(CONFIG_RTL_IGMP_SNOOPING)
	// IGMP snooping
	if((mode&MP_IGMP_MASK)==MP_IGMP_MASK){
		system("/bin/echo 1 > /proc/br_igmpsnoop");
		system("/bin/echo 1 > /proc/br_igmpquery");
	}
	else{
		system("/bin/echo 0 > /proc/br_igmpsnoop");
		system("/bin/echo 0 > /proc/br_igmpquery");
	}

	// change default value for vif interface socket
	system("/bin/echo 1024 > /proc/sys/net/ipv4/igmp_max_memberships");
#endif
#if defined(CONFIG_RTL_MLD_SNOOPING)
	// MLD snooping
	if((mode&MP_MLD_MASK)==MP_MLD_MASK){
		system("/bin/echo 1 > /proc/br_mldsnoop");
		system("/bin/echo 1 > /proc/br_mldquery");
	}
	else{
		system("/bin/echo 0 > /proc/br_mldsnoop");
		system("/bin/echo 0 > /proc/br_mldquery");
	}
#endif
#if defined(CONFIG_BRIDGE_IGMP_SNOOPING)
	// IGMP/MLD snooping status should same
	flag = ((mode&MP_IGMP_MASK)==MP_IGMP_MASK) || ((mode&MP_MLD_MASK)==MP_MLD_MASK);	// enable if IGMP or MLD snooping is enable
	if(flag){
		config_bridge_igmp_snooping(BRIF, 1);

		/*	Kernel snooping module will dynamic select a multicast querier which send "general query".
		 *	In differnt Wan, we expect following case:
		 *		For Bridge Wan, querier should be multicast server in internet.
		 *		For Internet Wan, querier should be igmpproxy/mldproxy daemon. */
		snprintf(cmd, sizeof(cmd), "/bin/echo 1 > /sys/devices/virtual/net/%s/bridge/multicast_snooping", BRIF);
		system(cmd);
	}
	else{
		config_bridge_igmp_snooping(BRIF, 0);
		snprintf(cmd, sizeof(cmd), "/bin/echo 0 > /sys/devices/virtual/net/%s/bridge/multicast_snooping", BRIF);
		system(cmd);
	}
#endif

#if defined(CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE)
	rtk_igmp_mld_snooping_module_enable(IPVER_IPV4, ((mode&MP_IGMP_MASK)==MP_IGMP_MASK));
	rtk_igmp_mld_snooping_module_enable(IPVER_IPV6, ((mode&MP_MLD_MASK)==MP_MLD_MASK));
#endif

	printf("IGMP Snooping: %s\n", ((mode&MP_IGMP_MASK)==MP_IGMP_MASK)?"enabled":"disabled");
	printf("MLD Snooping: %s\n", ((mode&MP_MLD_MASK)==MP_MLD_MASK)?"enabled":"disabled");

	block_br_lan_igmp_mld_rules();
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	//set ignoreGroup rules
	rtk_set_multicast_ignoreGroup();
#endif
}

// Mason Yu. MLD Proxy
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_MLDPROXY
/*	 0: SUCCESS
 *	-1: FAIL
 *
 *	Support Parameters:
 *		"IpVersion",
 *		"ForceVersion",
 *		"FastLeave",
 *		"LanCompatible",
 *		"RobustnessValue",
 *		"GroupMemberQueryInterval",
 *		"GroupMemberQueryCount",
 *		"StartupQueryInterval",
 *		"StartupQueryCount",
 *		"LastMemberQueryInterval",
 *		"LastMemberQueryCount",
 *		"GeneralQueryInterval",
 *		"GeneralQueryCount",
 *		"QueryResponseInterval",
 *		"LinkChangeQueryInterval",
 *		"LinkChangeQueryCount",
 *		"IntervalScale",
 *		"OldVersionHostPresentInterval",
*/
int setup_mldproxy_conf(void)
{
	FILE *fp;
	int vInt;

	if ((fp = fopen(MLDPROXY_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", MLDPROXY_CONF);
		return -1;
	}

	if (mib_get_s(MIB_MLD_ROBUST_COUNT, (void *)&vInt, sizeof(vInt)) == 0){
		printf("get MIB_MLD_ROBUST_COUNT fail\n");
		goto ERR;
	}
	fprintf(fp, "RobustnessValue=%d\n", vInt);
	fprintf(fp, "StartupQueryCount=%d\n", vInt);
	fprintf(fp, "LastMemberQueryCount=%d\n", vInt);
	fprintf(fp, "LinkChangeQueryCount=%d\n", vInt);

	if (mib_get_s(MIB_MLD_QUERY_INTERVAL, (void *)&vInt, sizeof(vInt)) == 0){
		printf("get MIB_MLD_QUERY_INTERVAL fail\n");
		goto ERR;
	}
	fprintf(fp, "GeneralQueryInterval=%d\n", vInt);

	if (mib_get_s(MIB_MLD_QUERY_RESPONSE_INTERVAL, (void *)&vInt, sizeof(vInt)) == 0){
		printf("get MIB_MLD_QUERY_RESPONSE_INTERVAL fail\n");
		goto ERR;
	}
	fprintf(fp, "QueryResponseInterval=%d\n", vInt/1000);	// to sec

	if (mib_get_s(MIB_MLD_LAST_MEMBER_QUERY_RESPONSE_INTERVAL, (void *)&vInt, sizeof(vInt)) == 0){
		printf("get MIB_MLD_LAST_MEMBER_QUERY_RESPONSE_INTERVAL fail\n");
		goto ERR;
	}
	fprintf(fp, "LastMemberQueryInterval=%d\n", vInt);

	fprintf(fp, "FastLeave=1\n");	//hard code

	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(MLDPROXY_CONF, 0666);
#endif
	return 1;
ERR:
	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(MLDPROXY_CONF, 0666);
#endif
	return -1;
}

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
void checkMldProxyItf(void)
{
	unsigned int ext_if;
	MIB_CE_ATM_VC_T Entry;
	int i=0, totalEntry = mib_chain_total(MIB_ATM_VC_TBL);

	if(!mib_get_s(MIB_MLD_PROXY_EXT_ITF, (void *)&ext_if, sizeof(ext_if)))
		printf("Get MLD Proxy Binded WAN interface index error\n");
	if(ext_if!=DUMMY_IFINDEX)
	{
		for(i=0;i<totalEntry;i++)
		{
			if(mib_chain_get(MIB_ATM_VC_TBL,i,&Entry)!=1)
				continue;
			if((Entry.IpProtocol & IPVER_IPV6) && (Entry.cmode != CHANNEL_MODE_BRIDGE) && ( Entry.applicationtype & (X_CT_SRV_INTERNET|X_CT_SRV_OTHER)))
			{
				if(Entry.ifIndex==ext_if)
					break;
			}
		}

		if(i==totalEntry)
		{
			ext_if = DUMMY_IFINDEX;
			if(!mib_set(MIB_MLD_PROXY_EXT_ITF, (void *)&ext_if))
				printf("Set MLD Proxy Binded interface index error\n");
		}
	}
}
#endif

int rtk_util_check_iface_ip_exist(char * ifname, int ipver)
{
	int ret=0;
	int flags, flags_found;
	int numOfIpv6;
	struct ipv6_ifaddr ip6_addr[6];
	struct in_addr inAddr;
	char *str_ipv4;

	numOfIpv6=getifip6(ifname, IPV6_ADDR_UNICAST, ip6_addr, 6);
	flags_found = getInFlags(ifname, &flags);
	ret = getInAddr(ifname, IP_ADDR, &inAddr);
	str_ipv4 = inet_ntoa(inAddr);
	//DBG(LOG_DEBUG, "ifname %s str_ipv4 %s ret %d flags_found %d numOfIpv6 %d \n", ifname, str_ipv4, ret, flags_found, numOfIpv6);
	if(flags_found)
	{
		if(ipver==IPVER_IPV4 && ret==1){
			return 1;
		}
		if(ipver==IPVER_IPV6 && numOfIpv6>0){
			return 1;
		}
	}
	return 0;

}

// MLD proxy configuration
// return value:
// 1  : successful
// 0  : function not enable
// -1 : startup failed
int startMLDproxy(void)
{
	unsigned char mldproxyEnable=0;
	unsigned int mldproxyItf;
	char ifname[IFNAMSIZ], tmp_buf[64];
	int mldproxy_pid, wait_pid, status=0, retry=0;
	MIB_CE_ATM_VC_T Entry;
	unsigned int entryNum, i;
#ifdef CONFIG_MLDPROXY_MULTIWAN
	char mldproxy_cmd[120]={0};
	char wanif[IFNAMSIZ];
	char isFirstWan=1;
#endif
#ifdef CONFIG_00R0
	int checkIP=0, flag=0;
#endif
	int isBrmldproxyExist = 0;
	unsigned char mldproxyversion=0;

	// Kill old IGMP proxy
	mldproxy_pid = read_pid((char *)MLDPROXY_PID);
	if (mldproxy_pid >= 1) {
		// kill it
		kill_by_pidfile_new((const char*)MLDPROXY_PID , SIGTERM);
	}
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	//Ready && CE-router Logo do not need policy route / firewall / portmapping
	unsigned char logoTestMode=0;
	if ( mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode)) ){
		if (logoTestMode)
			return 0;
	}
#endif

	if(access("/proc/br_mldproxy", 0)==0)
	{
		isBrmldproxyExist = 1;
	}
#ifdef CONFIG_MLDPROXY_MULTIWAN
	unsigned char is_enabled = isMLDProxyEnabled();
	if(!is_enabled)
		return 0;

	sprintf(mldproxy_cmd, "%s", MLDPROXY);
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			printf("error get atm vc entry\n");
			return -1;
		}
		if(Entry.enable && Entry.enableMLD && (Entry.cmode != CHANNEL_MODE_BRIDGE))
		{
			mldproxyEnable = 1;
			ifGetName(Entry.ifIndex, wanif, sizeof(wanif));
#ifdef CONFIG_00R0
			flag = rtk_util_check_iface_ip_exist(wanif, IPVER_IPV6);
			checkIP |= flag;
#endif
			if(isFirstWan){
				sprintf(mldproxy_cmd + strlen(mldproxy_cmd), " -i %s", wanif);
				isFirstWan = 0;
			}else{
				sprintf(mldproxy_cmd + strlen(mldproxy_cmd), " %s", wanif);
			}
		}
	}
	sprintf(mldproxy_cmd + strlen(mldproxy_cmd), " -o %s", LANIF);
	if(mldproxyEnable)
	{
		if(-1 == setup_mldproxy_conf()){
			printf("[%s] setup_mldproxy_conf fail\n", __FUNCTION__);
			return -1;
		}

		if(isBrmldproxyExist)
		{
			system("/bin/echo 1 > /proc/br_mldproxy");
		}
		system(mldproxy_cmd);
	}
	else
	{
		unlink(MLDPROXY_CONF);
		if(isBrmldproxyExist)
		{
			system("/bin/echo 0 > /proc/br_mldproxy");
		}
	}
#else
	if (mib_get_s(MIB_MLD_PROXY_EXT_ITF, (void *)&mldproxyItf, sizeof(mldproxyItf)) != 0)
	{
		if (!ifGetName(mldproxyItf, ifname, sizeof(ifname)))
		{
#ifdef CONFIG_E8B
			entryNum = mib_chain_total(MIB_ATM_VC_TBL);
			for (i=entryNum; i>0; i--)
			{
				if (!mib_chain_get(MIB_ATM_VC_TBL, i-1, (void *)&Entry))
				{
					printf("error get atm vc entry\n");
					return -1;
				}

				if ((Entry.IpProtocol & IPVER_IPV6) && (Entry.cmode != CHANNEL_MODE_BRIDGE) && ( Entry.applicationtype & (X_CT_SRV_INTERNET | X_CT_SRV_OTHER)))
				{
					mldproxyEnable = 1;
					mib_set(MIB_MLD_PROXY_DAEMON, (void *)&mldproxyEnable);
					mldproxyItf = Entry.ifIndex;
					mib_set(MIB_MLD_PROXY_EXT_ITF, (void *)&mldproxyItf);
					ifGetName(mldproxyItf, ifname, sizeof(ifname));
					mib_update(CURRENT_SETTING, CONFIG_MIB_TABLE);

					break;
				}
			}
			printf("Error: MLD proxy interface not set ! set to another WAN %s\n", ifname);
#else
			printf("Error: MLD proxy interface not set !\n");
			return 0;
#endif
		}
	}

	// check if MLD proxy enabled ?
	if (mib_get_s(MIB_MLD_PROXY_DAEMON, (void *)&mldproxyEnable, sizeof(mldproxyEnable)) != 0)
	{
		if (mldproxyEnable != 1){
			printf("%s(%d) MLD proxy not enable\n",__func__,__LINE__);
		if(isBrmldproxyExist)
		{
			system("/bin/echo 0 > /proc/br_mldproxy");
		}
			return 0;	// MLD proxy not enable
		}
	}
	if(mldproxyEnable)
	{
		printf("%s(%d) MLD proxy enable\n",__func__,__LINE__);
		if(isBrmldproxyExist)
		{
			system("/bin/echo 1 > /proc/br_mldproxy");
		}
	}

	// check for bridge&IPv4 WAN should not start ecmh process
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			printf("error get atm vc entry\n");
			return -1;
		}
		if (mldproxyItf == Entry.ifIndex ) break;
	}

	if (Entry.IpProtocol == IPVER_IPV4 || Entry.cmode == CHANNEL_MODE_BRIDGE )
	{
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=entryNum; i>0; i--)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i-1, (void *)&Entry))
			{
				printf("error get atm vc entry\n");
				continue;
			}

			if ((Entry.IpProtocol & IPVER_IPV6) && (Entry.cmode != CHANNEL_MODE_BRIDGE) && ( Entry.applicationtype & X_CT_SRV_INTERNET))
			{
				mldproxyEnable = 1;
				mib_set(MIB_MLD_PROXY_DAEMON, (void *)&mldproxyEnable);
				mldproxyItf = Entry.ifIndex;
				mib_set(MIB_MLD_PROXY_EXT_ITF, (void *)&mldproxyItf);
				ifGetName(mldproxyItf, ifname, sizeof(ifname));
				mib_update(CURRENT_SETTING, CONFIG_MIB_TABLE);

				if(-1 == setup_mldproxy_conf()){
					printf("[%s] setup_mldproxy_conf fail\n", __FUNCTION__);
					return -1;
				}

				printf("Error: MLD proxy interface is invalid, set another WAN %s !\n",ifname);
				va_niced_cmd(MLDPROXY, 4, 0, "-i", ifname, "-o", (char *)LANIF);

				break;
			}
		}
#else
		printf("Error: MLD proxy interface is invalid, stop the %s !\n",MLDPROXY);
#endif
	} else {
#ifdef CONFIG_00R0
		if(checkIP == 0){
			printf("WAN has no IP address\n");
		}
		else
#endif
		{
			if(-1 == setup_mldproxy_conf()){
				printf("[%s] setup_mldproxy_conf fail\n", __FUNCTION__);
				return -1;
			}

			va_niced_cmd(MLDPROXY, 4, 0, "-i", ifname, "-o", (char *)LANIF);
		}
	}
#endif
	return 1;
}


int isMLDProxyEnabled(void)
{
	unsigned int mldproxyItf;
	unsigned int entryNum;
	char ifname[IFNAMSIZ];
	MIB_CE_ATM_VC_T Entry;
	int i;
	unsigned char is_enabled;
	int found = 0;

#ifdef CONFIG_MLDPROXY_MULTIWAN
	if(!mib_get_s(MIB_MLD_PROXY_DAEMON, (void *)&is_enabled, sizeof(is_enabled)))
	{
		return 0;
	}
	if(!is_enabled)
		return 0;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			printf("error get atm vc entry\n");
			continue;
		}
		if(Entry.enable && (Entry.IpProtocol & IPVER_IPV6) && Entry.enableMLD && (Entry.cmode != CHANNEL_MODE_BRIDGE))
		{
			found = 1;
			break;
		}
	}
#else
	if(!mib_get_s(MIB_MLD_PROXY_DAEMON, (void *)&is_enabled, sizeof(is_enabled)))
	{
		return 0;
	}

	if (mib_get_s(MIB_MLD_PROXY_EXT_ITF, (void *)&mldproxyItf, sizeof(mldproxyItf)) != 0)
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=entryNum; i>0; i--)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i-1, (void *)&Entry))
			{
				printf("error get atm vc entry\n");
				return 0;
			}

			if (!(Entry.IpProtocol & IPVER_IPV6) || (Entry.cmode == CHANNEL_MODE_BRIDGE))
				continue;

			if (ifGetName(mldproxyItf, ifname, sizeof(ifname)))
			{
				if(Entry.ifIndex == mldproxyItf && is_enabled)
					found = 1;
			}
		}
	}
#endif
	return found;
}

#endif	//CONFIG_USER_MLDPROXY
#endif

#ifdef CONFIG_USER_IGMPPROXY
#ifdef CONFIG_USER_IGMPPROXY_SUPPORT_CFG_FILE
int rtk_igmpproxy_update_config_file(char *path, struct igmp_proxy_config_para *para)
{
 	FILE *fp = NULL;

	if(path == NULL || para == NULL)
	{
		printf("%s %d: invalid parameter !\n", __func__, __LINE__);
		return -1;
	}

	unlink(path);

	if(!(fp = fopen(path, "a+")))
	{
		return -1;
	}

	fprintf(fp, "query_interval %d\n", para->query_interval);
	fprintf(fp, "query_response_interval %d\n", para->query_response_interval);
	fprintf(fp, "last_member_query_count %d\n", para->last_member_query_count);
	fprintf(fp, "robust %d\n", para->robust);
	fprintf(fp, "group_leave_delay %d\n", para->group_leave_delay);
	fprintf(fp, "fast_leave %d\n", para->fast_leave);
	fprintf(fp, "query_mode %d\n", para->query_mode);
	fprintf(fp, "query_version %d\n", para->query_version);

	fclose(fp);

	return 0;
}
#endif
// IGMP proxy configuration
// return value:
// 1  : successful
// 0  : function not enable
// -1 : startup failed
int startIgmproxy(void)
{
	unsigned char igmpEnable;
	unsigned char mode;
	unsigned int igmpItf;
	char ifname[IFNAMSIZ];
	int igmpproxy_pid, wait_pid, status=0, retry=0;;
	char sysbuf[256] = {0};
	int qversion;
#ifdef CONFIG_00R0
	int checkIP=0, flag=0;
#endif

	// Reset multicast_router
	sprintf(sysbuf, "echo %d > /sys/devices/virtual/net/%s/bridge/multicast_router", 1, LANIF);
	va_cmd("/bin/sh" , 2, 1, "-c", sysbuf);

	// Kill old IGMP proxy
	igmpproxy_pid = read_pid((char *)IGMPPROXY_PID);
	if (igmpproxy_pid >= 1) {
		// kill it
		kill_by_pidfile_new((const char*)IGMPPROXY_PID , SIGTERM);
	}

	// check if IGMP proxy enabled ?
	if (mib_get_s(MIB_IGMP_PROXY, (void *)&igmpEnable, sizeof(igmpEnable)) != 0)
	{
		if (igmpEnable != 1)
			return 0;	// IGMP proxy not enable
	}
#ifndef CONFIG_DEONET_IGMPPROXY
	if (mib_get_s(MIB_IGMP_PROXY_ITF, (void *)&igmpItf, sizeof(igmpItf)) != 0)
	{
		if (!ifGetName(igmpItf, ifname, sizeof(ifname)))
		{
			printf("Error: IGMP proxy interface not set !\n");
			return 0;
		}
	}
#ifdef CONFIG_00R0
	flag = rtk_util_check_iface_ip_exist(ifname, IPVER_IPV4);
	checkIP |= flag;
	if(checkIP == 0){
		printf("WAN has no IP address\n");
		return 0;
	}
#endif
#endif

	// need set multicast_router to 2, because if bridge WAN which one in br0 was receive IGMP query,
	// it will let IGMP proxy cannot receive IGMP Report
	sprintf(sysbuf, "echo %d > /sys/devices/virtual/net/%s/bridge/multicast_router", 2, LANIF);
	va_cmd("/bin/sh" , 2, 1, "-c", sysbuf);

#ifdef CONFIG_USER_IGMPPROXY_SUPPORT_CFG_FILE
	if(-1 == setup_igmpproxy_conf()){
		printf("[%s] setup_igmpproxy_conf fail\n", __FUNCTION__);
		return -1;
	}
	#ifndef CONFIG_DEONET_IGMPPROXY
		va_niced_cmd(IGMPROXY, 8, 0, "-c","1","-d", (char *)LANIF,"-u",ifname,"-f", IGMPPROXY_CONF);
	#else
		memset(ifname, 0, sizeof(ifname));
		mode = deo_wan_nat_mode_get();
		if (mode == DEO_NAT_MODE)
			sprintf(ifname, "%s", "nas0_0");
		else
			sprintf(ifname, "%s", "br0");

		mib_get(MIB_IGMP_QUERY_VERSION, (void*)&qversion);
		if (qversion == 2) // V2 control for igmpproxy
			va_niced_cmd(IGMPROXY, 6, 0, "-c", "2", "-d", (char *)LANIF, "-u", ifname);
		else if (qversion == 3) // V3 control for igmpproxy
			va_niced_cmd(IGMPROXY, 6, 0, "-c", "3", "-d", (char *)LANIF, "-u", ifname);
	#endif
#else
	va_niced_cmd(IGMPROXY, 6, 0, "-c","1","-d", (char *)LANIF,"-u",ifname);
#endif
	return 1;
}

#ifdef CONFIG_IGMPPROXY_MULTIWAN
#define IGMPPROXY_INTF "/var/run/igmpproxy_intf"
int setting_Igmproxy(void)
{
	int igmpproxy_pid, len, wait_pid, status=0, retry=0;
	MIB_CE_ATM_VC_T Entry;
	unsigned int entryNum, i;
	char igmpproxy_wan[100];
	char igmpproxy_cmd[200];
	int igmpenable =0;
	char ifname[IFNAMSIZ];
#ifdef CONFIG_00R0
	int checkIP=0, flag=0;
#endif
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rt_igmpHook_devInfo_t devInfo;
#endif
#if defined(CONFIG_CMCC)
	unsigned char ipoeIGMP;
#endif

	// Reset multicast_router
	sprintf(igmpproxy_cmd, "echo %d > /sys/devices/virtual/net/%s/bridge/multicast_router", 1, LANIF);
	va_cmd("/bin/sh" , 2, 1, "-c", igmpproxy_cmd);

	igmpproxy_wan[0]='\0';
#if defined(CONFIG_YUEME) || defined(CONFIG_RTK_DEV_AP) || defined(CONFIG_CMCC) || defined(CONFIG_CU)

       unsigned char igmpProxyEnable;
       mib_get_s(MIB_IGMP_PROXY, (void *)&igmpProxyEnable, sizeof(igmpProxyEnable));
#endif
	// Mason Yu. IPTV_Intf is set to "" for sar driver
#ifdef CONFIG_IP_NF_UDP
	va_cmd("/bin/sarctl",2,1,"iptv_intf", "");
#endif


	entryNum = mib_chain_total(MIB_ATM_VC_TBL);

	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			printf("error get atm vc entry\n");
			continue;
		}

		// check if IGMP proxy enabled ?

#if defined(CONFIG_YUEME) || defined(CONFIG_RTK_DEV_AP) || defined(CONFIG_CMCC) || defined(CONFIG_CU)
		if(Entry.enable && Entry.enableIGMP && igmpProxyEnable)
#else
		if(Entry.enable && Entry.enableIGMP)
#endif
		{
			char iptv_intf_str[10];
			igmpenable =1;
			if (ifGetName(Entry.ifIndex, ifname, sizeof(ifname))) {
#ifdef CONFIG_IGMPPROXY_SENDER_IP_ZERO
#if defined(CONFIG_CMCC)
				mib_get(MIB_PPPOE_WAN_IPOE_IGMP, (void *)&ipoeIGMP);
				if(ipoeIGMP && (Entry.applicationtype & X_CT_SRV_OTHER))
				{
#endif
				if(Entry.cmode == CHANNEL_MODE_PPPOE){
					char wanif[IFNAMSIZ];
					char if_cmd[120];
					ifGetName(PHY_INTF(Entry.ifIndex), wanif, sizeof(wanif));
					snprintf(ifname,sizeof(ifname),wanif);
#ifdef CONFIG_00R0
					sprintf(if_cmd,"ifconfig %s 169.254.129.%d netmask 255.255.255.0",ifname,i+1);
#else
					sprintf(if_cmd,"ifconfig %s 10.0.0.%d",ifname,i+1);
#endif
					system(if_cmd);
				}
				va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_INSERT, "POSTROUTING","-p","2",
				"-o", ifname, "-j", "SNAT","--to-source", "0.0.0.0");
#if defined(CONFIG_CMCC)
				}
#endif
#endif
#ifdef CONFIG_00R0
				flag = rtk_util_check_iface_ip_exist(ifname, IPVER_IPV4);
				checkIP |= flag;
#endif
				//multiple WAN interfaces, seperated by ','
				if(igmpproxy_wan[0]=='\0') {
					snprintf(igmpproxy_wan, 100, "%s",ifname);
				}
				else {
					len = sizeof(igmpproxy_wan) - strlen(igmpproxy_wan);
					if(snprintf(igmpproxy_wan + strlen(igmpproxy_wan), len, ",%s", ifname) >= len) {
						printf("warning, string truncated\n");
					}
				}
#ifdef CONFIG_IP_NF_UDP
				va_cmd("/bin/sarctl",2,1,"iptv_intf", ifname);
#endif
			}
		}
	}

#if 0 //def CONFIG_YUEME //upstreamwan no setting, find first internet wan
	if(igmpproxy_wan[0]=='\0') {
		for (i=0; i<entryNum; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			{
				printf("error get atm vc entry\n");
				return -1;
			}
			if (Entry.enable && (Entry.applicationtype & X_CT_SRV_INTERNET) && ifGetName(Entry.ifIndex, ifname, sizeof(ifname)))
			{
				snprintf(igmpproxy_wan, 100, "%s",ifname);
				if(igmpProxyEnable)
					igmpenable =1;
				break;
			}
		}
		printf("[%s:%d]igmpproxy_wan choice internet wan = %s, igmpenable=%u\n", __func__, __LINE__, igmpproxy_wan, igmpenable);
	}
#endif

	if(igmpenable) {
		int restart_proxy = 1;
		FILE *fp = NULL;

		igmpproxy_pid = read_pid((char *)IGMPPROXY_PID);
		if(igmpproxy_pid >= 1 && (fp = fopen(IGMPPROXY_INTF, "r"))) {
			memset(igmpproxy_cmd, 0, sizeof(igmpproxy_cmd));
			fread(igmpproxy_cmd, 1, sizeof(igmpproxy_cmd)-1, fp);
			if(!strcmp(igmpproxy_cmd, igmpproxy_wan))
				restart_proxy = 0;
			fclose(fp);
		}

		if(restart_proxy && (fp = fopen(IGMPPROXY_INTF, "w"))) {
			fprintf(fp, "%s", igmpproxy_wan);
			fclose(fp);
		}

		// need set multicast_router to 2, because if bridge WAN which one in br0 was receive IGMP query,
		// it will let IGMP proxy cannot receive IGMP Report
		sprintf(igmpproxy_cmd, "echo %d > /sys/devices/virtual/net/%s/bridge/multicast_router", 2, LANIF);
		va_cmd("/bin/sh" , 2, 1, "-c", igmpproxy_cmd);

#ifdef CONFIG_00R0
		if(checkIP == 0){
			printf("WAN has no IP address\n");
		}
		else
#endif
#ifdef CONFIG_USER_IGMPPROXY_SUPPORT_CFG_FILE
		if(-1 == setup_igmpproxy_conf()){
			printf("[%s] setup_igmpproxy_conf fail\n", __FUNCTION__);
			return -1;
		}
#endif

#ifdef CONFIG_DEONET_IGMPPROXY
		if(restart_proxy || igmpproxy_pid < 1 || igmpproxy_pid) {
#else
		if(restart_proxy || igmpproxy_pid < 1) {
#endif
			unsigned char force_ver=2;
			int query_ver = 2;

			mib_get_s(MIB_IGMP_FORCE_VERSION, (void *)&force_ver, sizeof(force_ver));
			mib_get(MIB_IGMP_QUERY_VERSION, (void*)&query_ver);

			if(igmpproxy_pid >= 1) kill_by_pidfile_new((const char*)IGMPPROXY_PID , SIGTERM);

			if(force_ver==3)
			{
#ifdef CONFIG_USER_IGMPPROXY_SUPPORT_CFG_FILE
				va_niced_cmd(IGMPROXY, 8, 0, "-c","3","-d", (char *)LANIF,"-u",igmpproxy_wan,"-f",IGMPPROXY_CONF);
#else
				va_niced_cmd(IGMPROXY, 6, 0, "-c","3","-d", (char *)LANIF,"-u",igmpproxy_wan);
#endif
			}
			else
			{
#ifdef CONFIG_USER_IGMPPROXY_SUPPORT_CFG_FILE
				if (query_ver == 2)
					va_niced_cmd(IGMPROXY, 6, 0, "-c", "2", "-d", (char *)LANIF, "-u", igmpproxy_wan);
				else if (query_ver == 3)
					va_niced_cmd(IGMPROXY, 6, 0, "-c", "3", "-d", (char *)LANIF, "-u", igmpproxy_wan);
#else
				va_niced_cmd(IGMPROXY, 6, 0, "-c","1","-d", (char *)LANIF,"-u",igmpproxy_wan);
#endif
			}
		}
#ifndef CONFIG_DEONET_IGMPPROXY
		else if(igmpproxy_pid >= 1){
			//when interface not change, only update igmp parameter
			kill(igmpproxy_pid , SIGHUP);
		}
#endif
	}
	else
	{
		unlink(IGMPPROXY_INTF);
		// Kill old IGMP proxy
		igmpproxy_pid = read_pid((char *)IGMPPROXY_PID);
		if (igmpproxy_pid >= 1) {
			// kill it
			kill_by_pidfile_new((const char*)IGMPPROXY_PID , SIGTERM);
		}
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
		strncpy(devInfo.devIfname, LANIF, sizeof(devInfo.devIfname));
		devInfo.devIfidx = get_dev_ifindex(LANIF);
		rt_igmpHook_brDevConfig_set(devInfo, RT_BR_FLUSH_MDB, 1);
#endif
	}

	//Kevin, tell 0412 igmp snooping that the proxy is enabled/disabled
#ifndef CONFIG_RTL8672NIC
	if(access("/proc/br_igmpProxy", 0)==0){
		if(igmpenable)
			system("/bin/echo 1 > /proc/br_igmpProxy");
		else
			system("/bin/echo 0 > /proc/br_igmpProxy");
	}
#endif

#if defined(CONFIG_USER_RTK_NOSTB_IPTV)
	if (startNOSTB("restart") == -1)//restart stb service.
		printf("[%s:%d] Restart NO STB service failed !\n", __func__, __LINE__);
#endif

	return 1;
}
#endif
#ifdef CONFIG_USER_IGMPPROXY_SUPPORT_CFG_FILE
/*	 0: SUCCESS
 *	-1: FAIL
 *
 *	Support Parameters:
 *		"query_interval",
 *		"query_response_interval",
 *		"last_member_query_count",
 *		"robust",
 *		"group_leave_delay",
 *		"fast_leave",
*/
int setup_igmpproxy_conf(void)
{
	FILE *fp = NULL;
	int vInt;

	unlink(IGMPPROXY_CONF);

	if(!(fp = fopen(IGMPPROXY_CONF, "a+")))
	{
		return -1;
	}

	if (mib_get_s(MIB_IGMP_QUERY_INTERVAL, (void *)&vInt, sizeof(vInt)) == 0){
		printf("get MIB_IGMP_QUERY_INTERVAL fail\n");
		goto ERR;
	}
	fprintf(fp, "query_interval %d\n", vInt);

	if (mib_get_s(MIB_IGMP_QUERY_RESPONSE_INTERVAL, (void *)&vInt, sizeof(vInt)) == 0){
		printf("get MIB_IGMP_QUERY_RESPONSE_INTERVAL fail\n");
		goto ERR;
	}
	fprintf(fp, "query_response_interval %d\n", vInt);

	if (mib_get_s(MIB_IGMP_LAST_MEMBER_QUERY_COUNT, (void *)&vInt, sizeof(vInt)) == 0){
		printf("get MIB_IGMP_LAST_MEMBER_QUERY_COUNT fail\n");
		goto ERR;
	}
	fprintf(fp, "last_member_query_count %d\n", vInt);

	if (mib_get_s(MIB_IGMP_ROBUST_COUNT, (void *)&vInt, sizeof(vInt)) == 0){
		printf("get MIB_IGMP_ROBUST_COUNT fail\n");
		goto ERR;
	}
	fprintf(fp, "robust %d\n", vInt);

	if (mib_get_s(MIB_IGMP_GROUTP_LEAVE_DELAY, (void *)&vInt, sizeof(vInt)) == 0){
		printf("get MIB_IGMP_GROUTP_LEAVE_DELAY fail\n");
		goto ERR;
	}
	fprintf(fp, "group_leave_delay %d\n", vInt);

	fprintf(fp, "fast_leave 1\n");	// hard code

	fclose(fp);
	return 0;
ERR:
	fclose(fp);
	return -1;
}

#endif
int isIgmproxyEnabled(void)
{
	MIB_CE_ATM_VC_T Entry;
	unsigned int entryNum, i;
	unsigned char igmpenable =0;
	char ifname[IFNAMSIZ];
	unsigned int igmpItf;
#ifdef CONFIG_IGMPPROXY_MULTIWAN
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++)
	{
		 if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		 {
  			printf("error get atm vc entry\n");
			return 0;
		 }

		// check if IGMP proxy enabled ?
		if(Entry.enable
#ifdef CONFIG_IPV6
			&& (Entry.IpProtocol & IPVER_IPV4)
#endif
			&& Entry.enableIGMP && (Entry.cmode != CHANNEL_MODE_BRIDGE))
		{
			igmpenable = 1;
			break;
		}
	}
#else
	if (mib_get_s(MIB_IGMP_PROXY, (void *)&igmpenable, sizeof(igmpenable)) != 0)
	{
		if (igmpenable != 1)
			return 0;	// IGMP proxy not enable
	}
	if (mib_get_s(MIB_IGMP_PROXY_ITF, (void *)&igmpItf, sizeof(igmpItf)) != 0)
	{
		if (!ifGetName(igmpItf, ifname, sizeof(ifname)))
		{
			printf("Error: IGMP proxy interface not set !\n");
			return 0;
		}
	}
#endif
	if(igmpenable)
		return 1;

	return 0;
}
#endif // of CONFIG_USER_IGMPPROXY

void checkIGMPMLDProxySnooping(int isIGMPenable, int isMLDenable, int isIGMPProxyEnable, int isMLDProxyEnable)
{
	AUG_PRT("isIGMPenable=%d, isMLDenable=%d, isIGMPProxyEnable=%d, isMLDProxyEnable=%d\n", isIGMPenable, isMLDenable, isIGMPProxyEnable, isMLDProxyEnable);
	if(isIGMPProxyEnable && isMLDProxyEnable)
	{
		if(isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 1 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 1 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 1 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 1 > /proc/rg/mld_trap_to_PS");
		}
	}
	else if(!isIGMPProxyEnable && isMLDProxyEnable)
	{
		if(isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 1 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 2 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 2 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 1 > /proc/rg/mld_trap_to_PS");
		}
	}
	else if(isIGMPProxyEnable && !isMLDProxyEnable)
	{
		if(isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 1 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 1 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 1 > /proc/rg/mcast_protocol");
			system("/bin/echo 1 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
	}
	else if(!isIGMPProxyEnable && !isMLDProxyEnable)
	{
		if(isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 1 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 2 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 0 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
	}
}

#ifdef CONFIG_USER_IGMPTR247
int rtk_tr247_mcast_profile_entry_find_by_profile_id(unsigned short profileId, MIB_OMCI_DM_MCAST_PROFILE_ENTRY_T *profile_t)
{
	MIB_OMCI_DM_MCAST_PROFILE_ENTRY_T entry;
	int i, entryNum;

	entryNum = mib_chain_total(MIB_OMCI_DM_MCAST_PROFILE_TBL);
	for(i=0 ; i<entryNum ; i++)
	{
		if ( !mib_chain_get(MIB_OMCI_DM_MCAST_PROFILE_TBL, i, (void *)&entry) )
			return -1;

		if(entry.profileId == profileId){
			if(profile_t) memcpy(profile_t, &entry, sizeof(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_T));
			return i;
		}
	}
	return -1;
}

int rtk_tr247_mcast_port_entry_find_by_profile_id(unsigned short profileId, MIB_OMCI_DM_MCAST_PORT_ENTRY_T *port_t)
{
	MIB_OMCI_DM_MCAST_PORT_ENTRY_T entry;
	int i, entryNum;

	entryNum = mib_chain_total(MIB_OMCI_DM_MCAST_PORT_TBL);
	for(i=0 ; i<entryNum ; i++)
	{
		if ( !mib_chain_get(MIB_OMCI_DM_MCAST_PORT_TBL, i, (void *)&entry) )
			return -1;

		if(entry.profileId == profileId){
			if(port_t) memcpy(port_t, &entry, sizeof(MIB_OMCI_DM_MCAST_PORT_ENTRY_T));
			return i;
		}
	}
	return -1;
}

int rtk_tr247_mcast_port_entry_find_by_port_id(unsigned int portId, MIB_OMCI_DM_MCAST_PORT_ENTRY_T *port_t)
{
	MIB_OMCI_DM_MCAST_PORT_ENTRY_T entry;
	int i, entryNum;

	entryNum = mib_chain_total(MIB_OMCI_DM_MCAST_PORT_TBL);
	for(i=0 ; i<entryNum ; i++)
	{
		if ( !mib_chain_get(MIB_OMCI_DM_MCAST_PORT_TBL, i, (void *)&entry) )
			return -1;

		if(entry.portId == portId){
			if(port_t) memcpy(port_t, &entry, sizeof(MIB_OMCI_DM_MCAST_PORT_ENTRY_T));
			return i;
		}
	}
	return -1;
}

int rtk_tr247_mcast_check_profile_proxy_enabled_by_profile_id(unsigned short profileId)
{
	MIB_OMCI_DM_MCAST_PROFILE_ENTRY_T entry;
	int i, entryNum;

	entryNum = mib_chain_total(MIB_OMCI_DM_MCAST_PROFILE_TBL);
	for(i=0 ; i<entryNum ; i++)
	{
		if ( !mib_chain_get(MIB_OMCI_DM_MCAST_PROFILE_TBL, i, (void *)&entry) )
			return 0;
#ifdef CONFIG_DEONET_IGMPPROXY // Deonet GNT2400R
		if (entry.profileId == profileId)
#else
		if (entry.profileId == profileId && (entry.IGMPFun==1 || entry.IGMPFun==2))
#endif
			return 1;
	}
	return 0;
}

int rtk_tr247_mcast_check_profile_proxy_enabled_exist(void)
{
	MIB_OMCI_DM_MCAST_PROFILE_ENTRY_T entry;
	int i, entryNum;

	entryNum = mib_chain_total(MIB_OMCI_DM_MCAST_PROFILE_TBL);
	for(i=0 ; i<entryNum ; i++)
	{
		if ( !mib_chain_get(MIB_OMCI_DM_MCAST_PROFILE_TBL, i, (void *)&entry) )
			return 0;

		if(entry.IGMPFun==1 || entry.IGMPFun==2)
			return 1;
	}
	return 0;
}

#if defined(CONFIG_USER_IGMPPROXY) && defined(CONFIG_USER_IGMPPROXY_SUPPORT_CFG_FILE)
int rtk_tr247_mcast_igmpproxy_setting(char *path)
{
	int igmpproxy_pid, len;
	MIB_CE_ATM_VC_T Entry;
	unsigned int entryNum, i;
	char igmpproxy_wan[100];
	char igmpproxy_cmd[120];
	int igmpenable =0;
	char ifname[IFNAMSIZ];

	// Kill old IGMP proxy
	igmpproxy_pid = read_pid((char *)IGMPPROXY_PID);
	if (igmpproxy_pid >= 1) {
		// kill it
		kill_by_pidfile_new((const char*)IGMPPROXY_PID , SIGTERM);
	}
	igmpproxy_wan[0]='\0';

	// Mason Yu. IPTV_Intf is set to "" for sar driver
#ifdef CONFIG_IP_NF_UDP
	va_cmd("/bin/sarctl",2,1,"iptv_intf", "");
#endif
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);

	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
  			printf("error get atm vc entry\n");
			return -1;
		}

		// check if IGMP proxy enabled ?
		if(Entry.enable && (Entry.cmode != CHANNEL_MODE_BRIDGE))
		{
			char iptv_intf_str[10];
			igmpenable =1;
			if (ifGetName(Entry.ifIndex, ifname, sizeof(ifname))) {
#ifdef CONFIG_IGMPPROXY_SENDER_IP_ZERO
				if(Entry.cmode == CHANNEL_MODE_PPPOE){
					char wanif[IFNAMSIZ];
					char if_cmd[120];
					ifGetName(PHY_INTF(Entry.ifIndex), wanif, sizeof(wanif));
					snprintf(ifname,sizeof(ifname),wanif);
#ifdef CONFIG_00R0
					sprintf(if_cmd,"ifconfig %s 169.254.129.%d netmask 255.255.255.0",ifname,i+1);
#else
					sprintf(if_cmd,"ifconfig %s 10.0.0.%d",ifname,i+1);
#endif
					system(if_cmd);
				}
				va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_INSERT, "POSTROUTING","-p","2",
				"-o", ifname, "-j", "SNAT","--to-source", "0.0.0.0");
#endif

				//multiple WAN interfaces, seperated by ','
				if(igmpproxy_wan[0]=='\0') {
					snprintf(igmpproxy_wan, 100, "%s",ifname);
				}
				else {
					len = sizeof(igmpproxy_wan) - strlen(igmpproxy_wan);
					if(snprintf(igmpproxy_wan + strlen(igmpproxy_wan), len, ",%s", ifname) >= len) {
						printf("warning, string truncated\n");
					}
				}
#ifdef CONFIG_IP_NF_UDP
				va_cmd("/bin/sarctl",2,1,"iptv_intf", ifname);
#endif
			}
			else
			{
				printf("Error: IGMP proxy interface not set !\n");
				return 0;
			}
		}
	}
	if(igmpenable){
		if(path != NULL)
		{
			int qversion;
			// original method
			//va_niced_cmd(IGMPROXY, 8, 0, "-c", "2", "-d", (char *)LANIF,"-u",igmpproxy_wan,"-f",path);
			//sprintf(igmpproxy_cmd,"%s -c 1 -d br0 -u %s -f %s",(char *)IGMPROXY, igmpproxy_wan, path);

			mib_get(MIB_IGMP_QUERY_VERSION, (void*)&qversion);
			if (qversion == 2)
				va_niced_cmd(IGMPROXY, 6, 0, "-c", "2", "-d", (char *)LANIF,"-u", igmpproxy_wan);
			if (qversion == 3)
				va_niced_cmd(IGMPROXY, 6, 0, "-c", "3", "-d", (char *)LANIF,"-u", igmpproxy_wan);
		}
		else
		{
			va_niced_cmd(IGMPROXY, 6, 0, "-c","1","-d", (char *)LANIF,"-u",igmpproxy_wan);
			//sprintf(igmpproxy_cmd,"%s -c 1 -d br0 -u %s",(char *)IGMPROXY, igmpproxy_wan);
		}
		//system(igmpproxy_cmd);
	}

	//Kevin, tell 0412 igmp snooping that the proxy is enabled/disabled
#ifndef CONFIG_RTL8672NIC
	if(access("/proc/br_igmpProxy", 0)==0){
		if(igmpenable)
			system("/bin/echo 1 > /proc/br_igmpProxy");
		else
			system("/bin/echo 0 > /proc/br_igmpProxy");
	}
#endif
	return 1;
}

int rtk_tr247_mcast_igmpproxy_start(char *path)
{
	unsigned char igmpEnable;
	unsigned int igmpItf;
	char ifname[IFNAMSIZ];
	int igmpproxy_pid;

	// Kill old IGMP proxy
	igmpproxy_pid = read_pid((char *)IGMPPROXY_PID);
	if (igmpproxy_pid >= 1) {
		// kill it
		kill_by_pidfile_new((const char*)IGMPPROXY_PID , SIGTERM);
	}

	// check if IGMP proxy enabled ?
	if (mib_get_s(MIB_IGMP_PROXY, (void *)&igmpEnable, sizeof(igmpEnable)) != 0)
	{
		if (igmpEnable != 1)
			return 0;	// IGMP proxy not enable
	}
	if (mib_get_s(MIB_IGMP_PROXY_ITF, (void *)&igmpItf, sizeof(igmpItf)) != 0)
	{
		if (!ifGetName(igmpItf, ifname, sizeof(ifname)))
		{
			printf("Error: IGMP proxy interface not set !\n");
			return 0;
		}
	}

	if(path != NULL)
	{
		va_niced_cmd(IGMPROXY, 8, 0, "-c","1","-d", (char *)LANIF,"-u",ifname,"-f",path);
	}
	else
	{
		va_niced_cmd(IGMPROXY, 6, 0, "-c","1","-d", (char *)LANIF,"-u",ifname);
	}

	return 1;
}

int rtk_tr247_mcast_igmpproxy_config(unsigned int profileId, unsigned char isUpdate, unsigned char isStop)
{
	MIB_OMCI_DM_MCAST_PROFILE_ENTRY_T profile_t = {0};
	struct igmp_proxy_config_para para_t = {0};
	int mibIndex, ret = -1;
	int qmode, qversion;
	char path[256] = {0};
	int change_config = 0;
	int query_interval, query_response_interval;

#define HMS_TO_MS(HMS)	(HMS*100)

	sprintf(path, "/var/config/tr247_igmpproxy_config_profile_%d", profileId);
	if((mibIndex = rtk_tr247_mcast_profile_entry_find_by_profile_id(profileId, &profile_t)) >= 0)
	{
		para_t.query_interval = profile_t.queryIntval;
		para_t.query_response_interval = profile_t.queryMaxRspTime;
		para_t.robust = profile_t.robustness;
		para_t.fast_leave = profile_t.ImmediateLeave;
		para_t.group_leave_delay = (HMS_TO_MS(profile_t.lastMbrQueryIntval))*profile_t.robustness;
		para_t.last_member_query_count = profile_t.robustness;

		mib_get_s(MIB_IGMP_QUERY_INTERVAL, (void *)&query_interval, sizeof(query_interval));
		mib_get_s(MIB_IGMP_QUERY_RESPONSE_INTERVAL, (void *)&query_response_interval, sizeof(query_response_interval));

		/*
		   printf("=> %s %d: query_interval:[%d]\n", __func__, __LINE__, profile_t.queryIntval);
		   printf("=> %s %d: MIB(query_interval):[%d]\n", __func__, __LINE__, query_interval);
		   printf("=> %s %d: query_response_interval:[%d]\n", __func__, __LINE__, profile_t.queryMaxRspTime);
		   printf("=> %s %d: MIB(query_response_interval):[%d]!\n", __func__, __LINE__, query_response_interval);
		*/

		if (profile_t.queryIntval != query_interval || profile_t.queryMaxRspTime != query_response_interval) {
			change_config = 1;

			query_interval = profile_t.queryIntval;
			mib_set(MIB_IGMP_QUERY_INTERVAL, (void *)&query_interval);

			query_response_interval = profile_t.queryMaxRspTime;
			mib_set(MIB_IGMP_QUERY_RESPONSE_INTERVAL, (void *)&query_response_interval);
		}

		// add query mode for mib information(because not standard)
		mib_get(MIB_IGMP_QUERY_MODE, (void*)&qmode);
		para_t.query_mode = qmode;

		mib_get(MIB_IGMP_QUERY_VERSION, (void*)&qversion);
		para_t.query_version = qversion;

		ret = rtk_igmpproxy_update_config_file(&path[0], &para_t);

		if (call_count == 0 || change_config == 1) {
#ifdef CONFIG_IGMPPROXY_MULTIWAN
			rtk_tr247_mcast_igmpproxy_setting(path);
#else
			rtk_tr247_mcast_igmpproxy_start(path);
#endif
			call_count = 1;
		}
	}
	else
	{
		printf("%s %d: rtk_tr247_mcast_port_entry_find_by_port_id FAIL !\n", __func__, __LINE__);
	}

	return ret;
}
#else
int rtk_tr247_mcast_igmpproxy_config(unsigned int profileId, unsigned char isUpdate, unsigned char isStop)
{
	printf("%s %d: not implement !\n", __func__, __LINE__);
	return -1;
}
#endif

int rtk_tr247_multicast_init(void)
{
	mib_chain_clear(MIB_OMCI_DM_MCAST_ACL_TBL);
	mib_chain_clear(MIB_OMCI_DM_MCAST_PROFILE_TBL);
	mib_chain_clear(MIB_OMCI_DM_MCAST_PORT_TBL);

	// Deonet: not need because default value 1 of igmphookmodule and port forwarding state time
	// rtk_tr247_multicast_silent_discarding_igmp(NULL);
	return 0;
}

int rtk_tr247_multicast_reset_all(void)
{
	MIB_OMCI_DM_MCAST_PROFILE_ENTRY_T profileEntry;
	MIB_L2BRIDGE_GROUP_T l2GroupEntry = {0};
	MIB_L2BRIDGE_GROUP_Tp pL2GroupEntry = &l2GroupEntry;
	MIB_CE_SW_PORT_T sw_port_entry = {0};
#ifdef CONFIG_RTL_SMUX_DEV
	char ifName[IFNAMSIZ], cmd_str[1024];
#endif
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	char brname[32] = {0};
	rt_igmpHook_devInfo_t devInfo = {0};
#endif
	int i, j, k, entryNum, ret;

#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	entryNum = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
	for (i=0 ; i<entryNum ; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, (void *)pL2GroupEntry))
			continue;

		if (!get_grouping_ifname_by_groupnum(pL2GroupEntry->groupnum, brname, sizeof(brname)))
			continue;

		strncpy(devInfo.devIfname, brname, sizeof(devInfo.devIfname));
		devInfo.devIfidx = get_dev_ifindex(brname);
		rt_igmpHook_brDevConfig_set(devInfo , RT_BR_FLUSH_CONF , 1);
	}
#endif
#ifdef CONFIG_RTL_SMUX_DEV
	for (i=0 ; i<SW_LAN_PORT_NUM ; i++)
	{
		if (!mib_chain_get(MIB_SW_PORT_TBL, i, &sw_port_entry))
			continue;

		for(j=0 ; j<=2 ; j++)
		{
			sprintf(ifName, "%s", ELANVIF[i]);
			sprintf(cmd_str, "%s --tx --if %s --tags %d --rule-remove-alias tr247_dynamic_acl+", SMUXCTL, ifName, j);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#ifdef CONFIG_TR142_MODULE
			for(k = 1; k<OMCI_DM_MAX_BRIDGE_NUM ; k++)
			{
				if((sw_port_entry.omci_uni_bridgeIdmask)&(1<<k))
				{
					sprintf(ifName, "%s_pptp_%d", ELANRIF[i],k);
					sprintf(cmd_str, "%s --tx --if %s --tags %d --rule-remove-alias tr247_dynamic_acl+", SMUXCTL, ifName, j);
					va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
				}
			}
#endif
		}
	}
#endif
	entryNum = mib_chain_total(MIB_OMCI_DM_MCAST_PROFILE_TBL);
	for(i=0 ; i<entryNum ; i++)
	{
		if ( !mib_chain_get(MIB_OMCI_DM_MCAST_PROFILE_TBL, i, (void *)&profileEntry) )
			return -1;

		rtk_tr247_multicast_profile_fw_remove(profileEntry.profileId);
	}

	/* There needs to restart the daemon so isStop should be set to zero. 20220505 */
	rtk_tr247_mcast_igmpproxy_config(0, 0, 0);

	mib_chain_clear(MIB_OMCI_DM_MCAST_ACL_TBL);
	mib_chain_clear(MIB_OMCI_DM_MCAST_PROFILE_TBL);
	mib_chain_clear(MIB_OMCI_DM_MCAST_PORT_TBL);
	return 0;
}
typedef int (*RTK_TR247_MCAST_PORT_CALL_FUNC_T)(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp, MIB_OMCI_DM_MCAST_PORT_ENTRY_Tp, const char *, int);

int rtk_tr247_multicast_port_snooping_enable(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p, MIB_OMCI_DM_MCAST_PORT_ENTRY_Tp port_p, const char *ifName, int action)
{
	unsigned int value;

	if(port_p->SnoopingEnable)
		value = 0;
	else
		value = 1;

#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rt_igmpHook_devInfo_t devInfo = {0};
	strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
	devInfo.devIfidx = get_dev_ifindex(ifName);
	if(!strcmp(ifName, BRIF))
	{
		rt_igmpHook_brDevConfig_set(devInfo , RT_BR_SNOOPING_DISABLE , value);
	}
	else
	{
		rt_igmpHook_devConfig_set(devInfo , 0 ,RT_SNOOPING_DISABLE , value);
		rt_igmpHook_devConfig_set(devInfo , 1 ,RT_SNOOPING_DISABLE , value);
	}
#else
	printf("%s %d: not implement !\n", __func__, __LINE__);
#endif
	return 0;
}

int rtk_tr247_multicast_port_max_group_number(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p, MIB_OMCI_DM_MCAST_PORT_ENTRY_Tp port_p, const char *ifName, int action)
{
	unsigned int value;

	value = port_p->portMaxGrpNum;

#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rt_igmpHook_devInfo_t devInfo = {0};
	strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
	devInfo.devIfidx = get_dev_ifindex(ifName);

	if(!strcmp(ifName, BRIF))
	{
		rt_igmpHook_brDevConfig_set(devInfo , RT_BR_MAX_GROUP_SIZE , value);
	}
	else
	{
		rt_igmpHook_devConfig_set(devInfo , 0 ,RT_MAX_GROUP_SIZE , value);
		rt_igmpHook_devConfig_set(devInfo , 1 ,RT_MAX_GROUP_SIZE , value);
	}
#else
	printf("%s %d: not implement !\n", __func__, __LINE__);
#endif
	system("/bin/echo 1 > /proc/fc/ctrl/flow_flush");
	return 0;
}

int rtk_tr247_multicast_port_max_bw(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p, MIB_OMCI_DM_MCAST_PORT_ENTRY_Tp port_p, const char *ifName, int action)
{
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rt_igmpHook_devInfo_t devInfo;
	strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
	devInfo.devIfidx = get_dev_ifindex(ifName);
	if(!strcmp(ifName, BRIF))
		rt_igmpHook_brDevConfig_set(devInfo ,RT_BR_MAX_IP4_WEIGHT , port_p->portMaxBw);
	else
		rt_igmpHook_devConfig_set(devInfo , 0 ,RT_MAX_WEIGHT , port_p->portMaxBw);
#else
	printf("%s %d: not implement !\n", __func__, __LINE__);
#endif
	system("/bin/echo 1 > /proc/fc/ctrl/flow_flush");
	return 0;
}

int rtk_tr247_multicast_port_bw_enforce(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p, MIB_OMCI_DM_MCAST_PORT_ENTRY_Tp port_p, const char *ifName, int action)
{
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rt_igmpHook_devInfo_t devInfo;
	strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
	devInfo.devIfidx = get_dev_ifindex(ifName);
	if(!strcmp(ifName, BRIF))
		rt_igmpHook_brDevConfig_set(devInfo ,RT_BR_DROP_IP4_OVER_WEIGHT , port_p->bwEnforceB);
	else
		rt_igmpHook_devConfig_set(devInfo , 0 ,RT_DROP_OVER_WEIGHT , port_p->bwEnforceB);
#else
	printf("%s %d: not implement !\n", __func__, __LINE__);
#endif
	return 0;
}

int rtk_tr247_multicast_port_igmp_version(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p, MIB_OMCI_DM_MCAST_PORT_ENTRY_Tp port_p, const char *ifName, int action)
{
	unsigned int igmpv1Disabled = 1, igmpv2Disabled = 1, igmpv3Disabled = 1;
	unsigned int mldv1Disabled = 1, mldv2Disabled = 1;

	switch(profile_p->IGMPVersion)
	{
		case 17:
			mldv2Disabled = 0;
			/* fall through */
		case 16:
			mldv1Disabled = 0;
			/* fall through */
		case 3:
			igmpv3Disabled = 0;
			/* fall through */
		case 2:
			igmpv2Disabled = 0;
			/* fall through */
		case 1:
			igmpv1Disabled = 0;
			break;
		default:
			printf("%s %d: reserved case.\n", __func__, __LINE__);
			return -1;
	}

#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rt_igmpHook_devInfo_t devInfo = {0};
	strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
	devInfo.devIfidx = get_dev_ifindex(ifName);
	if(!strcmp(ifName, BRIF))
	{
		rt_igmpHook_brDevConfig_set(devInfo ,RT_BR_DROP_IGMPV1 , igmpv1Disabled);
		rt_igmpHook_brDevConfig_set(devInfo ,RT_BR_DROP_IGMPV2 , igmpv2Disabled);
		rt_igmpHook_brDevConfig_set(devInfo ,RT_BR_DROP_IGMPV3 , igmpv3Disabled);
	}
	else
	{
		rt_igmpHook_devConfig_set(devInfo , 0 ,RT_DROP_IGMPV1 , igmpv1Disabled);
		rt_igmpHook_devConfig_set(devInfo , 0 ,RT_DROP_IGMPV2 , igmpv2Disabled);
		rt_igmpHook_devConfig_set(devInfo , 0 ,RT_DROP_IGMPV3 , igmpv3Disabled);
		rt_igmpHook_devConfig_set(devInfo , 1 ,RT_DROP_MLDV1 , mldv1Disabled);
		rt_igmpHook_devConfig_set(devInfo , 1 ,RT_DROP_MLDV2 , mldv2Disabled);
	}

	rtk_tr247_multicast_silent_discarding_igmp(ifName);
#else
	printf("%s %d: not implement !\n", __func__, __LINE__);
#endif
	return 0;
}

int rtk_tr247_multicast_port_igmp_function(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p, MIB_OMCI_DM_MCAST_PORT_ENTRY_Tp port_p, const char *ifName, int action)
{
	unsigned int snoopingDisabled = 1;
	unsigned int proxyDisabled = 1;

	switch(profile_p->IGMPFun)
	{
		case 1:
			proxyDisabled = 0;
			/* fall through */
		case 0:
			snoopingDisabled = 0;
			break;
		case 2:
			proxyDisabled = 0;
			break;
		default:
			printf("%s %d: invalid case.\n", __func__, __LINE__);
			return -1;
	}

#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rt_igmpHook_devInfo_t devInfo = {0};
	strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
	devInfo.devIfidx = get_dev_ifindex(ifName);
	if(!strcmp(ifName, BRIF))
	{
		rt_igmpHook_brDevConfig_set(devInfo , RT_BR_SNOOPING_DISABLE , snoopingDisabled);
	}
	else
	{
		rt_igmpHook_devConfig_set(devInfo , 0 ,RT_SNOOPING_DISABLE , snoopingDisabled);
		rt_igmpHook_devConfig_set(devInfo , 1 ,RT_SNOOPING_DISABLE , snoopingDisabled);
	}
#else
	printf("%s %d: not implement !\n", __func__, __LINE__);
#endif
	return 0;
}

int rtk_tr247_multicast_port_fast_leave(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p, MIB_OMCI_DM_MCAST_PORT_ENTRY_Tp port_p, const char *ifName, int action)
{
	unsigned int flastLeaveEnable;

	if(profile_p->ImmediateLeave)
		flastLeaveEnable = 1;
	else
		flastLeaveEnable = 0;
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rt_igmpHook_devInfo_t devInfo = {0};
	strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
	devInfo.devIfidx = get_dev_ifindex(ifName);
	if(!strcmp(ifName, BRIF))
	{
		rt_igmpHook_brDevConfig_set(devInfo , RT_BR_FAST_LEAVE , flastLeaveEnable);
	}
	else
	{
		rt_igmpHook_devConfig_set(devInfo , 0 ,RT_FAST_LEAVE , flastLeaveEnable);
		rt_igmpHook_devConfig_set(devInfo , 1 ,RT_FAST_LEAVE , flastLeaveEnable);
	}
#else
	printf("%s %d: not implement !\n", __func__, __LINE__);
#endif

	return 0;
}

int rtk_tr247_multicast_port_us_igmp_rate_limit(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p, MIB_OMCI_DM_MCAST_PORT_ENTRY_Tp port_p, const char *ifName, int action)
{
	unsigned int value;

	value = profile_p->UsIGMPRate;

	if(value > 1)
		value = value - 1;

#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rt_igmpHook_devInfo_t devInfo = {0};
	strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
	devInfo.devIfidx = get_dev_ifindex(ifName);
	if(!strcmp(ifName, BRIF))
	{
		rt_igmpHook_brDevConfig_set(devInfo , RT_BR_RATE_LIMIT , value);
	}
	else
	{
		rt_igmpHook_devConfig_set(devInfo , 0 ,RT_RATE_LIMIT , value);
		rt_igmpHook_devConfig_set(devInfo , 1 ,RT_RATE_LIMIT , value);
	}
#else
	printf("%s %d: not implement !\n", __func__, __LINE__);
#endif

	return 0;
}

int rtk_tr247_multicast_port_unauthorized_join_behavior(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p, MIB_OMCI_DM_MCAST_PORT_ENTRY_Tp port_p, const char *ifName, int action)
{
	unsigned int value;

	unsigned int tr142_customer_flag = 0;
	mib_get_s(MIB_TR142_CUSTOMER_FLAG, &tr142_customer_flag, sizeof(tr142_customer_flag));
	if (tr142_customer_flag & TR142_CUSTOMER_FLAG_IGNORE_MC_UNAUTHORIZED_JOIN_BEHAVIOR)
	{
		printf("%s %d: TR142_CUSTOMER_FLAG_IGNORE_MC_UNAUTHORIZED_JOIN_BEHAVIOR\n", __func__, __LINE__);
		return 0;
	}

	value = profile_p->unAuthJoinRqtBhvr;
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	int aclIndex = 0;
	rt_igmpHook_devInfo_t devInfo = {0};
	rt_igmpHook_whiteList_t patten;
	struct in_addr s;

	memset(&patten,0,sizeof(patten));
	strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
	devInfo.devIfidx = get_dev_ifindex(ifName);
	patten.gipChk=1;
	if (value == 0)
	{
		inet_pton(AF_INET, WHITELIST_DEFAULT_DROP, (void *)&s);
		patten.wlGroupIpStart[0]=ntohl(s.s_addr);
		inet_pton(AF_INET, WHITELIST_DEFAULT_DROP, (void *)&s);
		patten.wlGroupIpEnd[0]=ntohl(s.s_addr);
		if(!action)
		{
			rt_igmpHook_whiteList_del(&patten, devInfo , -1);
		}
		else
		{
			rt_igmpHook_whiteList_add(&patten, devInfo, &aclIndex);
		}
	}
	else if (value == 1)
	{
		inet_pton(AF_INET, WHITELIST_DEFAULT_ACCEPT_START, (void *)&s);
		patten.wlGroupIpStart[0]=ntohl(s.s_addr);
		inet_pton(AF_INET, WHITELIST_DEFAULT_ACCEPT_END, (void *)&s);
		patten.wlGroupIpEnd[0]=ntohl(s.s_addr);
		if(!action)
		{
			rt_igmpHook_whiteList_del(&patten, devInfo , -1);
		}
		else
		{
			rt_igmpHook_whiteList_add(&patten, devInfo, &aclIndex);
		}
	}
	//printf("==> %s, %s\n", ifName, whiteListStr);
#else
	printf("%s %d: not implement !\n", __func__, __LINE__);
#endif
	return 0;
}

int rtk_tr247_multicast_port_us_igmp_tag_info(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p, MIB_OMCI_DM_MCAST_PORT_ENTRY_Tp port_p, const char *ifName, int action)
{
	unsigned int tagAction;
	tagAction = profile_p->usIgmpTagCtrl;

#if defined(CONFIG_RTL_SMUX_DEV)
	char rule_alias_name[256], cmd_str[1024];
	char rule_exec[128], rule_action[256];
	int j;

	sprintf(rule_alias_name, "ME309_us_igmp_tag_action");
	if(tagAction == 0)//Pass upstream IGMP/MLD traffic transparently
	{
		for(j=0 ; j<=2 ; j++)
		{
			sprintf(rule_exec, "%s --if %s --rx --tags %d", SMUXCTL, ifName, j);
			sprintf(cmd_str, "%s --rule-remove-alias %s+", rule_exec, rule_alias_name);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
		}
	}
	else if(tagAction == 1)//Add a VLAN tag (including P bits)
	{
		for(j=0 ; j<=2 ; j++)
		{
			sprintf(rule_exec, "%s --if %s --rx --tags %d", SMUXCTL, ifName, j);
			sprintf(rule_action, "--push-tag --set-vid %d %d --set-priority %d %d", profile_p->usIgmpVlan, j+1, profile_p->usIgmpVprio, j+1);
			sprintf(cmd_str, "%s --filter-multicast %s --target ACCEPT --rule-priority %d --rule-alias %s --rule-append", rule_exec, rule_action, SMUX_RULE_PRIO_IGMP, rule_alias_name);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
		}
	}
	else if(tagAction == 2)//Replace the entire TCI (VLAN ID plus P bits)
	{
		for(j=1 ; j<=2 ; j++)
		{
			sprintf(rule_exec, "%s --if %s --rx --tags %d", SMUXCTL, ifName, j);
			sprintf(rule_action, "--set-vid %d %d --set-priority %d %d", profile_p->usIgmpVlan, j, profile_p->usIgmpVprio, j);
			sprintf(cmd_str, "%s --filter-multicast %s --target ACCEPT --rule-priority %d --rule-alias %s --rule-append", rule_exec, rule_action, SMUX_RULE_PRIO_IGMP, rule_alias_name);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
		}
	}
	else if(tagAction == 3)//Replace only the VLAN ID
	{
		for(j=1 ; j<=2 ; j++)
		{
			sprintf(rule_exec, "%s --if %s --rx --tags %d", SMUXCTL, ifName, j);
			sprintf(rule_action, "--set-vid %d %d", profile_p->usIgmpVlan, j);
			sprintf(cmd_str, "%s --filter-multicast %s --target ACCEPT --rule-priority %d --rule-alias %s --rule-append", rule_exec, rule_action, SMUX_RULE_PRIO_IGMP, rule_alias_name);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
		}
	}
#endif
	return 0;
}

typedef int (*RTK_TR247_MCAST_CALL_FUNC_T)(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp);
int rtk_tr247_multicast_apply_profile_port(unsigned short profileId, RTK_TR247_MCAST_PORT_CALL_FUNC_T func);

int rtk_tr247_multicast_igmp_version(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p)
{
	if(rtk_tr247_mcast_check_profile_proxy_enabled_by_profile_id(profile_p->profileId))
	{
		if(rtk_tr247_mcast_igmpproxy_config(profile_p->profileId, 1, 0) == -1)
		{
			printf("%s %d: rtk_tr247_mcast_igmpproxy_config FAIL !\n", __func__, __LINE__);
		}
	}

	rtk_tr247_multicast_apply_profile_port(profile_p->profileId, rtk_tr247_multicast_port_igmp_version);

	return 0;
}

int rtk_tr247_multicast_igmp_function(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p)
{
	if(rtk_tr247_mcast_check_profile_proxy_enabled_by_profile_id(profile_p->profileId))
	{
		if(rtk_tr247_mcast_igmpproxy_config(profile_p->profileId, 0, 0) == -1)
		{
			printf("%s %d: rtk_tr247_mcast_igmpproxy_config FAIL !\n", __func__, __LINE__);
		}
	}
	else if(!rtk_tr247_mcast_check_profile_proxy_enabled_exist())
	{
		if(rtk_tr247_mcast_igmpproxy_config(profile_p->profileId, 0, 1) == -1)
		{
			printf("%s %d: rtk_tr247_mcast_igmpproxy_config FAIL !\n", __func__, __LINE__);
		}
	}

	rtk_tr247_multicast_apply_profile_port(profile_p->profileId, rtk_tr247_multicast_port_igmp_function);

	return 0;
}

int rtk_tr247_multicast_fast_leave(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p)
{
	if(rtk_tr247_mcast_check_profile_proxy_enabled_by_profile_id(profile_p->profileId))
	{
		if(rtk_tr247_mcast_igmpproxy_config(profile_p->profileId, 1, 0) == -1)
		{
			printf("%s %d: rtk_tr247_mcast_igmpproxy_config FAIL !\n", __func__, __LINE__);
		}
	}

	rtk_tr247_multicast_apply_profile_port(profile_p->profileId, rtk_tr247_multicast_port_fast_leave);

	return 0;
}

int rtk_tr247_multicast_us_igmp_rate_limit(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p)
{
#if 0
	unsigned char ifName[IFNAMSIZ] = {0};
	unsigned char chainName[128] = {0};
	unsigned int i, atmVcEntryNum;
	MIB_CE_ATM_VC_T atmVcEntry;

	sprintf(chainName, "%s_%d", FW_US_IGMP_RATE_LIMIT, profile_p->profileId);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)chainName);
	atmVcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	unsigned char atmVcGrpIfName[IFNAMSIZ];
	unsigned char atmVcIfName[IFNAMSIZ];
	unsigned char limitStr[12];

	if(profile_p->UsIGMPRate)
	{
		for (i=0; i<atmVcEntryNum; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&atmVcEntry))
			{
	  			printf("%s %d: mib_chain_get FAIL !\n", __func__, __LINE__);
				return -1;
			}

			sprintf(atmVcGrpIfName, "br%d", atmVcEntry.itfGroupNum);
			if(!strcmp(atmVcGrpIfName, ifName))
			{
				ifGetName(atmVcEntry.ifIndex, atmVcIfName, sizeof(atmVcIfName));
				if(atmVcEntry.enable && (atmVcEntry.cmode != CHANNEL_MODE_BRIDGE))
				{
					sprintf(limitStr, "%d/s", profile_p->UsIGMPRate);
					va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char *)chainName, "-o", (char *)atmVcIfName, "-p", "igmp", "-m", "limit", "--limit", (char *)limitStr, "-j", "ACCEPT");
					va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)chainName, "-o", (char *)atmVcIfName, "-p", "igmp", "-j", "DROP");
				}
				//TODO: add MLD limit here
			}
		}
	}
#endif
	rtk_tr247_multicast_apply_profile_port(profile_p->profileId, rtk_tr247_multicast_port_us_igmp_rate_limit);

	return 0;
}

int rtk_tr247_multicast_robustness(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p)
{
	if(rtk_tr247_mcast_check_profile_proxy_enabled_by_profile_id(profile_p->profileId))
		return rtk_tr247_mcast_igmpproxy_config(profile_p->profileId, 1, 0);
	else
		return 0;
}

int rtk_tr247_multicast_query_interval(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p)
{
	if(rtk_tr247_mcast_check_profile_proxy_enabled_by_profile_id(profile_p->profileId))
		return rtk_tr247_mcast_igmpproxy_config(profile_p->profileId, 1, 0);
	else
		return 0;
}

int rtk_tr247_multicast_query_max_response_time(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p)
{
	if(profile_p->queryMaxRspTime > 255)
	{
		printf("%s %d: queryMaxRspTime = %d > 255.\n", __func__, __LINE__, profile_p->queryMaxRspTime);
		return -1;
	}

	if(rtk_tr247_mcast_check_profile_proxy_enabled_by_profile_id(profile_p->profileId))
		return rtk_tr247_mcast_igmpproxy_config(profile_p->profileId, 1, 0);
	else
		return 0;
}

int rtk_tr247_multicast_last_member_query_interval(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p)
{
	if(rtk_tr247_mcast_check_profile_proxy_enabled_by_profile_id(profile_p->profileId))
		return rtk_tr247_mcast_igmpproxy_config(profile_p->profileId, 1, 0);
	else
		return 0;
}

int rtk_tr247_multicast_unauthorized_join_behavior(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p)
{
	if(rtk_tr247_mcast_check_profile_proxy_enabled_by_profile_id(profile_p->profileId))
	{
		if(rtk_tr247_mcast_igmpproxy_config(profile_p->profileId, 1, 0) == -1)
		{
			printf("%s %d: rtk_tr247_mcast_igmpproxy_config FAIL !\n", __func__, __LINE__);
		}
	}
	rtk_tr247_multicast_apply_profile_port(profile_p->profileId, rtk_tr247_multicast_port_unauthorized_join_behavior);
}

int rtk_tr247_multicast_us_igmp_tag_info(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p)
{
	if(rtk_tr247_mcast_check_profile_proxy_enabled_by_profile_id(profile_p->profileId))
	{
		if(rtk_tr247_mcast_igmpproxy_config(profile_p->profileId, 1, 0) == -1)
		{
			printf("%s %d: rtk_tr247_mcast_igmpproxy_config FAIL !\n", __func__, __LINE__);
		}
	}

	rtk_tr247_multicast_apply_profile_port(profile_p->profileId, rtk_tr247_multicast_port_us_igmp_tag_info);

	return 0;
}

int rtk_tr247_multicast_silent_discarding_igmp(const char *ifNameByRule)
{
	unsigned char portDropIgmpVersionTbl[MAX_LAN_PORT_NUM];
	unsigned char ifName[IFNAMSIZ] = {0};
	int i, ret=0;

    mib_get_s(MIB_PORT_DROP_IGMP_VERSION, (void *)portDropIgmpVersionTbl, sizeof(portDropIgmpVersionTbl));
	for(i=0;i<SW_LAN_PORT_NUM;i++)
	{
		if(ifNameByRule == NULL)
		{
			if(rtk_if_name_get_by_lan_phyID(rtk_port_get_lan_phyID(i), ifName) == -1)
			{
				printf("%s %d: rtk_if_name_get_by_lan_phyID FAIL !\n", __func__, __LINE__);
				continue;
			}
		}
		else
		{
			if(!strcmp(ifNameByRule, BRIF))
			{
				strncpy(ifName, ELANVIF[i] , IFNAMSIZ);
			}
			else
			{
				if(strstr(ifNameByRule, ELANRIF[i]) == NULL)
				{
					continue;
				}
				strncpy(ifName, ifNameByRule , IFNAMSIZ);
			}
		}
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
		rt_igmpHook_devInfo_t devInfo = {0};
		strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
		devInfo.devIfidx = get_dev_ifindex(ifName);
#endif

		switch(portDropIgmpVersionTbl[i])
		{
			case 1:
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
				rt_igmpHook_devConfig_set(devInfo , 0 ,RT_DROP_IGMPV1 , 1);
#endif
				break;
			case 2:
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
				rt_igmpHook_devConfig_set(devInfo , 0 ,RT_DROP_IGMPV2 , 1);
#endif
				break;
			case 3:
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
				rt_igmpHook_devConfig_set(devInfo , 0 ,RT_DROP_IGMPV3 , 1);
#endif
				break;
			default:
				printf("%s %d: invalid case.\n", __func__, __LINE__);
				ret=-1;
				break;
		}
	}
	return ret;
}

void rtk_tr247_multicast_profile_fw_init(unsigned int profileId)
{
	unsigned char chainName[128] = {0};

	sprintf(chainName, "%s_%d", FW_US_IGMP_RATE_LIMIT, profileId);
	va_cmd(IPTABLES, 2, 1, "-N", (char *)chainName);
	va_cmd(IPTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_TR247_MULTICAST, "-j", (char *)chainName);
}

void rtk_tr247_multicast_profile_fw_remove(unsigned int profileId)
{
	unsigned char chainName[128] = {0};

	sprintf(chainName, "%s_%d", FW_US_IGMP_RATE_LIMIT, profileId);
	va_cmd(IPTABLES, 4, 1, (char *)FW_DEL, (char *)FW_TR247_MULTICAST, "-j", (char *)chainName);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)chainName);
}

int rtk_tr247_multicast_apply_profile_port(unsigned short profileId, RTK_TR247_MCAST_PORT_CALL_FUNC_T func)
{
	MIB_OMCI_DM_MCAST_PROFILE_ENTRY_T profile_t = {0};
	MIB_OMCI_DM_MCAST_PORT_ENTRY_T port_t = {0};
	MIB_CE_SW_PORT_T sw_port_entry = {0};
	int profIndex, portIndex, entryNum, ret = -1, i, j, apply = 0;
	unsigned char ifName[IFNAMSIZ+1] = {0};
	int wanPhyPort;

	if((profIndex = rtk_tr247_mcast_profile_entry_find_by_profile_id(profileId, &profile_t)) >= 0)
	{
		wanPhyPort = rtk_port_get_wan_phyID();

		entryNum = mib_chain_total(MIB_OMCI_DM_MCAST_PORT_TBL);
		for(portIndex=0 ; portIndex<entryNum ; portIndex++)
		{
			if (!mib_chain_get(MIB_OMCI_DM_MCAST_PORT_TBL, portIndex, (void *)&port_t)){
				continue;
			}
			if(profile_t.profileId == port_t.profileId)
			{
				if(port_t.portId == wanPhyPort)
				{
					for (i = 0; i < SW_LAN_PORT_NUM; i++)
					{
						strcpy(ifName, ELANVIF[i]);
						ret = func(&profile_t, &port_t, ifName, 1);
					}
				}
				else
				{
					if((i = rtk_if_name_get_by_lan_phyID(port_t.portId, NULL)) >= 0
						&& mib_chain_get(MIB_SW_PORT_TBL, i, &sw_port_entry))
					{
						for(j = 1; j<OMCI_DM_MAX_BRIDGE_NUM ; j++)
						{
#if defined(CONFIG_TR142_MODULE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
							if(sw_port_entry.omci_uni_portType == OMCI_PORT_TYPE_PPTP)
							{
								if((sw_port_entry.omci_uni_bridgeIdmask)&(1<<j))
								{
									sprintf(ifName, "%s_pptp_%d", ELANRIF[i],j);
									apply = 1;
								}
							}
							else
#endif
							{
								strcpy(ifName, ELANVIF[i]);
								j = OMCI_DM_MAX_BRIDGE_NUM;
								apply = 1;
							}
							if(apply)
								ret = func(&profile_t, &port_t, ifName, 1);
						}
					}
				}
			}
		}
	}
	else
	{
		//printf("%s %d: rtk_tr247_mcast_profile_entry_find_by_profile_id FAIL(profileId=%d) !\n", __func__, __LINE__, profileId);
		return -1;
	}
	return ret;
}

int rtk_tr247_multicast_apply_profile(unsigned short profileId, RTK_TR247_MCAST_CALL_FUNC_T func)
{
	MIB_OMCI_DM_MCAST_PROFILE_ENTRY_T profile_t = {0};
	int profIndex, ret = -1;

	if((profIndex = rtk_tr247_mcast_profile_entry_find_by_profile_id(profileId, &profile_t)) >= 0)
	{
		ret = func(&profile_t);
	}
	else
	{
		//printf("%s %d: rtk_tr247_mcast_profile_entry_find_by_profile_id FAIL(profileId=%d) !\n", __func__, __LINE__, profileId);
		return -1;
	}
	return ret;
}

int rtk_tr247_multicast_apply_port(unsigned int portId, RTK_TR247_MCAST_PORT_CALL_FUNC_T func, int action)
{
	MIB_OMCI_DM_MCAST_PROFILE_ENTRY_T profile_t = {0};
	MIB_OMCI_DM_MCAST_PORT_ENTRY_T port_t = {0};
	MIB_CE_SW_PORT_T sw_port_entry = {0};
	int profIndex, portIndex, ret = -1, i, j, apply = 0;
	unsigned char ifName[IFNAMSIZ+1] = {0};

	if((portIndex = rtk_tr247_mcast_port_entry_find_by_port_id(portId, &port_t)) >= 0)
	{
		if((profIndex = rtk_tr247_mcast_profile_entry_find_by_profile_id(port_t.profileId, &profile_t)) < 0)
		{
			//printf("%s %d: rtk_tr247_mcast_profile_entry_find_by_profile_id FAIL (profileId=%u) !\n", __func__, __LINE__, port_t.profileId);
			return -1;
		}

		if(port_t.portId == rtk_port_get_wan_phyID())
		{
			ret = func(&profile_t, &port_t, BRIF, action);
		}
		else
		{
			if((i = rtk_if_name_get_by_lan_phyID(port_t.portId, NULL)) >= 0
				&& mib_chain_get(MIB_SW_PORT_TBL, i, &sw_port_entry))
			{
				for(j = 1; j<OMCI_DM_MAX_BRIDGE_NUM ; j++)
				{
#if defined(CONFIG_TR142_MODULE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
					if(sw_port_entry.omci_uni_portType == OMCI_PORT_TYPE_PPTP)
					{
						if((sw_port_entry.omci_uni_bridgeIdmask)&(1<<j))
						{
							sprintf(ifName, "%s_pptp_%d", ELANRIF[i],j);
							apply = 1;
						}
					}
					else
#endif
					{
						strcpy(ifName, ELANVIF[i]);
						j = OMCI_DM_MAX_BRIDGE_NUM;
						apply = 1;
					}
					if(apply)
						ret = func(&profile_t, &port_t, ifName, action);
				}
			}
		}
	}
	else
	{
		//printf("%s %d: rtk_tr247_mcast_port_entry_find_by_port_id FAIL (portId=%u)!\n", __func__, __LINE__, portId);
		return -1;
	}

	return ret;
}

int rtk_tr247_dynamic_acl_find(unsigned short profileId, unsigned int aclTableEntryId, MIB_OMCI_DM_MCAST_ACL_ENTRY_T *acl_t)
{
	MIB_OMCI_DM_MCAST_ACL_ENTRY_T entry = {0};
	int i, entryNum;

	entryNum = mib_chain_total(MIB_OMCI_DM_MCAST_ACL_TBL);
	for(i=0 ; i<entryNum ; i++)
	{
		if ( !mib_chain_get(MIB_OMCI_DM_MCAST_ACL_TBL, i, (void *)&entry) )
			return -1;

		if(entry.profileId == profileId && entry.aclTableEntryId == aclTableEntryId){
			if(acl_t) memcpy(acl_t, &entry, sizeof(MIB_OMCI_DM_MCAST_ACL_ENTRY_T));
			return i;
		}
	}
	return -1;
}

int rtk_tr247_dynamic_acl_get_count_by_profile(unsigned short profileId)
{
	MIB_OMCI_DM_MCAST_ACL_ENTRY_T acl_t = {0};
	int i, entryNum, count = 0;

	entryNum = mib_chain_total(MIB_OMCI_DM_MCAST_ACL_TBL);
	for(i=0 ; i<entryNum ; i++)
	{
		if ( !mib_chain_get(MIB_OMCI_DM_MCAST_ACL_TBL, i, (void *)&acl_t) )
			return -1;

		if(acl_t.profileId == profileId )
			count++;
	}
	return count;
}

int rtk_tr247_dynamic_acl_set_rule(MIB_OMCI_DM_MCAST_ACL_ENTRY_T *pAcl, const char *ifName, int action)
{
	int acl_proto = 0, i = 0;
	char sipIpv4Str[64]={0}, dipStartIpv4Str[64]={0}, dipEndIpv4Str[64]={0};
	char sipIpv4Valid = 0, dipStartIpv4Valid = 0, dipEndIpv4StrValid = 0;

	if(pAcl == NULL || ifName == NULL)
		return -1;

	if(pAcl->ipProtocol == IPVER_IPV4)
	{
		sprintf(sipIpv4Str, "%s", inet_ntoa(*((struct in_addr *)&pAcl->sip_ipv4[0])));
		if(!isIPAddr(sipIpv4Str) || !strcmp(sipIpv4Str, "0.0.0.0"))
			sipIpv4Str[0] = '\0';

		sprintf(dipStartIpv4Str, "%s", inet_ntoa(*((struct in_addr *)&pAcl->dipStart_ipv4[0])));
		if(!isIPAddr(dipStartIpv4Str) || !strcmp(dipStartIpv4Str, "0.0.0.0"))
			dipStartIpv4Str[0] = '\0';

		sprintf(dipEndIpv4Str, "%s", inet_ntoa(*((struct in_addr *)&pAcl->dipEnd_ipv4[0])));
		if(!isIPAddr(dipEndIpv4Str) || !strcmp(dipEndIpv4Str, "0.0.0.0"))
			dipEndIpv4Str[0] = '\0';

		if(sipIpv4Str[0] || dipStartIpv4Str[0] || dipEndIpv4Str[0])
			acl_proto = IPVER_IPV4;
	}
	else
	{
		printf("%s %d: ipProtocol=%d not support now !\n", __func__, __LINE__, pAcl->ipProtocol);
		//TODO: smuxdev need support --filter-sip6 --filter-dip6-range further
		return 0;
	}


#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	int aclIndex;
	rt_igmpHook_devInfo_t devInfo = {0};
	rt_igmpHook_whiteList_t patten;
	rt_igmpHook_groupWeight_t groupWeightCfg;
	struct in_addr s;
	unsigned char uChar = 0;

	memset(&patten,0,sizeof(patten));
	strncpy(devInfo.devIfname, ifName, sizeof(devInfo.devIfname));
	devInfo.devIfidx = get_dev_ifindex(ifName);
	if (mib_get_s(MIB_TR247_UNMATCHED_VEIP_CFG, (void *)&uChar, sizeof(uChar)) != 0){
		patten.mcSipChkIgnoreNonIGMPv3MLDv2 = uChar;
	}else{
		patten.mcSipChkIgnoreNonIGMPv3MLDv2 = 0;
	}

	if(acl_proto == IPVER_IPV4)
	{
		if(sipIpv4Str[0])
		{
			patten.mcSipChk=1;
			inet_pton(AF_INET, sipIpv4Str, (void *)&s);
			patten.wlMcSip[0]=ntohl(s.s_addr);
			patten.wlMcSipEnd[0]=ntohl(s.s_addr);
		}
		if(dipStartIpv4Str[0] && dipEndIpv4Str[0])
		{
			patten.gipChk=1;
			inet_pton(AF_INET, dipStartIpv4Str, (void *)&s);
			patten.wlGroupIpStart[0]=ntohl(s.s_addr);
			inet_pton(AF_INET, dipEndIpv4Str, (void *)&s);
			patten.wlGroupIpEnd[0]=ntohl(s.s_addr);
		}
		else if(dipStartIpv4Str[0])
		{
			patten.gipChk=1;
			inet_pton(AF_INET, dipStartIpv4Str, (void *)&s);
			patten.wlGroupIpStart[0]=ntohl(s.s_addr);
			patten.wlGroupIpEnd[0]=ntohl(s.s_addr);
		}
		else if(dipEndIpv4Str[0])
		{
			patten.gipChk=1;
			inet_pton(AF_INET, dipEndIpv4Str, (void *)&s);
			patten.wlGroupIpStart[0]=ntohl(s.s_addr);
			patten.wlGroupIpEnd[0]=ntohl(s.s_addr);
		}
		inet_pton(AF_INET, dipStartIpv4Str, (void *)&s);
		groupWeightCfg.weightGroupIpStart[0] = ntohl(s.s_addr);
		inet_pton(AF_INET, dipEndIpv4Str, (void *)&s);
		groupWeightCfg.weightGroupIpEnd[0] = ntohl(s.s_addr);
		groupWeightCfg.isIpv6 = 0;
		groupWeightCfg.weight = pAcl->imputedGrpBw;
		if(!action)
		{
			rt_igmpHook_groupWeight_del(devInfo, &groupWeightCfg, pAcl->groupWeightId);
			if(!strcmp(ifName, BRIF))
			{
				for (i = 0; i < SW_LAN_PORT_NUM; i++)
				{
					memset(&devInfo,0,sizeof(devInfo));
					strncpy(devInfo.devIfname, ELANVIF[i], sizeof(devInfo.devIfname));
					devInfo.devIfidx = get_dev_ifindex(ELANVIF[i]);
					rt_igmpHook_whiteList_del(&patten, devInfo , -1);
				}
			}
			else
			{
				rt_igmpHook_whiteList_del(&patten, devInfo , -1);
			}
		}
		else
		{
			rt_igmpHook_groupWeight_add(devInfo, &groupWeightCfg, &pAcl->groupWeightId);
			if(!strcmp(ifName, BRIF))
			{
				for (i = 0; i < SW_LAN_PORT_NUM; i++)
				{
					memset(&devInfo,0,sizeof(devInfo));
					strncpy(devInfo.devIfname, ELANVIF[i], sizeof(devInfo.devIfname));
					devInfo.devIfidx = get_dev_ifindex(ELANVIF[i]);
					rt_igmpHook_whiteList_add(&patten, devInfo, &aclIndex);
				}
			}
			else
			{
				rt_igmpHook_whiteList_add(&patten, devInfo, &aclIndex);
			}
		}
		//printf("==> %s, %s\n", ifName, whiteListStr);
	}
#endif

	return 0;
}

int rtk_tr247_multicast_port_dynamic_acl(MIB_OMCI_DM_MCAST_PROFILE_ENTRY_Tp profile_p, MIB_OMCI_DM_MCAST_PORT_ENTRY_Tp port_p, const char *ifName, int action)
{
	int i, j, entryNum;
	MIB_OMCI_DM_MCAST_ACL_ENTRY_T acl_t = {0};
	unsigned char ifName_tmp[IFNAMSIZ+1] = {0};

	entryNum = mib_chain_total(MIB_OMCI_DM_MCAST_ACL_TBL);
	for(i=0 ; i<entryNum ; i++)
	{
		if ( !mib_chain_get(MIB_OMCI_DM_MCAST_ACL_TBL, i, (void *)&acl_t) )
			continue;

		if(acl_t.profileId == profile_p->profileId )
		{
			if(port_p->portId == rtk_port_get_wan_phyID())
			{
				rtk_tr247_dynamic_acl_set_rule(&acl_t, BRIF, action);
			}
			else
			{
				rtk_tr247_dynamic_acl_set_rule(&acl_t, ifName, action);
			}
		}
	}

	return 0;
}

int rtk_tr247_dynamic_acl_config(unsigned short profileId, unsigned int aclTableEntryId, int action)
{
	MIB_OMCI_DM_MCAST_ACL_ENTRY_T acl_t = {0};
	MIB_OMCI_DM_MCAST_PROFILE_ENTRY_T profile_t = {0};
	MIB_OMCI_DM_MCAST_PORT_ENTRY_T port_t;
	MIB_CE_SW_PORT_T sw_port_entry = {0};
	unsigned char ifName[IFNAMSIZ+1] = {0};
	int i, j, k, ret = -1, entryNum, portEntryIdx, profIndex, aclIndex, apply = 0;
	//AUG_PRT("==> %s: profileId=%u, aclTableEntryId=%u, action=%d\n", __FUNCTION__, profileId, aclTableEntryId, action);
	if((profIndex = rtk_tr247_mcast_profile_entry_find_by_profile_id(profileId, &profile_t)) < 0)
	{
		//printf("%s %d: rtk_tr247_mcast_profile_entry_find_by_profile_id FAIL(profileId=%d) !\n", __func__, __LINE__, profileId);
		return -1;
	}

	if((aclIndex = rtk_tr247_dynamic_acl_find(profileId, aclTableEntryId, &acl_t)) < 0)
	{
		//printf("%s %d: rtk_tr247_dynamic_acl_find FAIL !\n", __func__, __LINE__);
		return -1;
	}

	entryNum = mib_chain_total(MIB_OMCI_DM_MCAST_PORT_TBL);
	for(i=0 ; i<entryNum ; i++)
	{
		if ( !mib_chain_get(MIB_OMCI_DM_MCAST_PORT_TBL, i, (void *)&port_t) )
			continue;

		if(acl_t.profileId ==  port_t.profileId)
		{
			if(port_t.portId == rtk_port_get_wan_phyID())
			{
				rtk_tr247_dynamic_acl_set_rule(&acl_t, BRIF, action);
			}
			else
			{
				if((j = rtk_if_name_get_by_lan_phyID(port_t.portId, NULL)) >= 0
					&& mib_chain_get(MIB_SW_PORT_TBL, j, &sw_port_entry))
				{
					for(k = 1; k<OMCI_DM_MAX_BRIDGE_NUM ; k++)
					{
#if defined(CONFIG_TR142_MODULE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
						if(sw_port_entry.omci_uni_portType == OMCI_PORT_TYPE_PPTP)
						{
							if((sw_port_entry.omci_uni_bridgeIdmask)&(1<<k))
							{
								sprintf(ifName, "%s_pptp_%d", ELANRIF[j],k);
								apply = 1;
							}
						}
						else
#endif
						{
							strcpy(ifName, ELANVIF[j]);
							k = OMCI_DM_MAX_BRIDGE_NUM;
							apply = 1;
						}
						if(apply)
							rtk_tr247_dynamic_acl_set_rule(&acl_t, ifName, action);
					}
				}
			}
		}
	}

	return 0;
}

int rtk_tr247_dynamic_acl_set(unsigned short profileId, unsigned int aclTableEntryId)
{
	return rtk_tr247_dynamic_acl_config(profileId, aclTableEntryId, 1);
}

int rtk_tr247_dynamic_acl_del(unsigned short profileId, unsigned int aclTableEntryId)
{
	return rtk_tr247_dynamic_acl_config(profileId, aclTableEntryId, 0);
}

int rtk_tr247_multicast_new_port_create(unsigned int portId)
{
	MIB_OMCI_DM_MCAST_PORT_ENTRY_T port_t = {0};
	int mibIndex, mib_ret=-1, ret=-1;
	unsigned char ifName[16] = {0};

	if((mibIndex = rtk_tr247_mcast_port_entry_find_by_port_id(portId, &port_t)) >= 0)
	{
		//printf("%s %d: portId=%d existed(mibIndex=%d) !\n", __func__, __LINE__, portId, mibIndex);
		ret = 0;
	}
	else
	{
		port_t.portId= portId;
		port_t.profileId = 0xffff; // default, before assign port to profile
		port_t.SnoopingEnable = 1;
		port_t.portMaxGrpNum = 0;
		if(portId == rtk_port_get_wan_phyID())
		{
			strncpy((char *)port_t.ifName, BRIF, sizeof(port_t.ifName));
		}
		else
		{
			rtk_if_name_get_by_lan_phyID(portId,ifName);
			memcpy(port_t.ifName, ifName, sizeof(port_t.ifName));
		}
		mib_ret = mib_chain_add(MIB_OMCI_DM_MCAST_PORT_TBL, &port_t);
		if(mib_ret == 0)
		{
			//printf("%s %d: Error! MIB_OMCI_DM_MCAST_PORT_TBL Add chain record.\n", __func__, __LINE__);
		}
		else if(mib_ret == -1)
		{
			//printf("%s %d: Error! MIB_OMCI_DM_MCAST_PORT_TBL Full.\n", __func__, __LINE__);
		}
		else
		{
			//printf("%s %d: SUCESS !\n", __func__, __LINE__);
			ret = 0;
		}
	}
	return ret;
}

static void *rtk_tr247_multicast_port_add_thread(void *param)
{
	int i, mibIndex, ret=-1, action, j, apply = 0;;
	unsigned int portId, profileId;
	MIB_OMCI_DM_MCAST_PORT_ENTRY_T port_t = {0};
	MIB_CE_SW_PORT_T sw_port_entry = {0};
	unsigned char ifName[IFNAMSIZ+1] = {0};
	portId = ((unsigned int *)param)[0];
	profileId = ((unsigned int *)param)[1];
	if((mibIndex = rtk_tr247_mcast_port_entry_find_by_port_id(portId, &port_t)) >= 0)
	{
		if(port_t.profileId != 0xffff && port_t.profileId != profileId)
		{
			action = 0;
			ret = 0;
			ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_igmp_version, action);
			ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_snooping_enable, action);
			ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_max_group_number, action);
			ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_fast_leave, action);
			ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_dynamic_acl, action);
			//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_igmp_function, action);
			//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_us_igmp_rate_limit, action);
			//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_max_bw, action);
			//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_bw_enforce, action);
			//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_us_igmp_tag_info, action);
		}

		port_t.profileId = profileId;
		if(portId == rtk_port_get_wan_phyID())
		{
			strncpy((char *)port_t.ifName, BRIF, sizeof(port_t.ifName));
		}
		else
		{
			if((i = rtk_if_name_get_by_lan_phyID(portId, NULL)) >= 0
					&& mib_chain_get(MIB_SW_PORT_TBL, i, &sw_port_entry))
			{
				for(j = 1; j<OMCI_DM_MAX_BRIDGE_NUM ; j++)
				{
#if defined(CONFIG_TR142_MODULE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
					if(sw_port_entry.omci_uni_portType == OMCI_PORT_TYPE_PPTP)
					{
						if((sw_port_entry.omci_uni_bridgeIdmask)&(1<<j))
						{
							sprintf(ifName, "%s_pptp_%d", ELANRIF[i],j);
							apply = 1;
						}
					}
					else
#endif
					{
						strcpy(ifName, ELANVIF[i]);
						j = OMCI_DM_MAX_BRIDGE_NUM;
						apply = 1;
					}
					if(apply)
						memcpy(port_t.ifName, ifName, sizeof(port_t.ifName));
				}
			}
		}
		mib_chain_update(MIB_OMCI_DM_MCAST_PORT_TBL, (void *) &port_t, mibIndex);

		action = 1;
		ret = 0;
		ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_igmp_version, action);
		ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_snooping_enable, action);
		ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_max_group_number, action);
		ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_fast_leave, action);
		ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_dynamic_acl, action);
		//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_igmp_function, action);
		//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_us_igmp_rate_limit, action);
		//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_max_bw, action);
		//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_bw_enforce, action);
		//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_us_igmp_tag_info, action);
	}
	else
	{
		//printf("%s %d: portId=%d not existed !\n", __func__, __LINE__, portId);
	}
	pthread_exit(NULL);
}

int rtk_tr247_multicast_apply(APPLY_TR247_MC_T opt, unsigned short profileId, unsigned int portId, unsigned int aclTableEntryId)
{
	MIB_OMCI_DM_MCAST_ACL_ENTRY_T acl_t = {0};
	MIB_OMCI_DM_MCAST_PROFILE_ENTRY_T profile_t = {0};
	MIB_OMCI_DM_MCAST_PORT_ENTRY_T port_t = {0};
	MIB_CE_SW_PORT_T sw_port_entry = {0};
	int mibIndex, mib_ret=-1, ret=-1;
	int i, entryNum, action, j, apply = 0;
	unsigned char ifName[IFNAMSIZ+1] = {0};
	int swPortIndex, portIndex, profIndex, phyPortId;
	unsigned int param[3] = {0};

	//AUG_PRT(" >>>>> opt=%d profileId=%d portId=%d aclTableEntryId=%d\n", opt, profileId, portId, aclTableEntryId);
	switch(opt)
	{
		case TR247_MC_RESET_ALL:
			//AUG_PRT("TR247_MC_RESET_ALL\n");
			ret = rtk_tr247_multicast_reset_all();
			break;
		case TR247_MC_PROFILE_ADD:
			//AUG_PRT("TR247_MC_PROFILE_ADD\n");
			if((mibIndex = rtk_tr247_mcast_profile_entry_find_by_profile_id(profileId, NULL)) >=0)
			{
				//printf("%s %d: profileId=%d existed(mibIndex=%d) !\n", __func__, __LINE__, profileId, mibIndex);
				ret = 0;
			}
			else
			{
				profile_t.profileId = profileId;
				profile_t.IGMPFun = 0;
				profile_t.IGMPVersion = 17;
				profile_t.ImmediateLeave = 1;
				profile_t.UsIGMPRate = 0;
				profile_t.robustness = 2;
				profile_t.queryIntval = 125;
				profile_t.queryMaxRspTime = 100;
				profile_t.lastMbrQueryIntval = 10;
				profile_t.unAuthJoinRqtBhvr = 0;
				mib_ret = mib_chain_add(MIB_OMCI_DM_MCAST_PROFILE_TBL, &profile_t);
				if(mib_ret == 0)
				{
					//printf("%s %d: Error! MIB_OMCI_DM_MCAST_PROFILE_TBL Add chain record.\n", __func__, __LINE__);
				}
				else if(mib_ret == -1)
				{
					//printf("%s %d: Error! MIB_OMCI_DM_MCAST_PROFILE_TBL Full.\n", __func__, __LINE__);
				}
				else
				{
					rtk_tr247_multicast_profile_fw_init(profileId);
					if(rtk_tr247_mcast_check_profile_proxy_enabled_by_profile_id(profileId))
						rtk_tr247_mcast_igmpproxy_config(profileId, 0, 0);
					else if(!rtk_tr247_mcast_check_profile_proxy_enabled_exist())
						rtk_tr247_mcast_igmpproxy_config(profileId, 0, 1);
					//printf("%s %d: SUCESS !\n", __func__, __LINE__);
					ret = 0;
				}
			}
			break;
		case TR247_MC_PROFILE_DEL:
			//AUG_PRT("TR247_MC_PROFILE_DEL\n");
			if((mibIndex = rtk_tr247_mcast_profile_entry_find_by_profile_id(profileId, &profile_t)) >=0)
			{
				rtk_tr247_multicast_profile_fw_remove(profileId);
				if(!rtk_tr247_mcast_check_profile_proxy_enabled_exist())
					rtk_tr247_mcast_igmpproxy_config(profileId, 0, 1);

				//delete associate dynamic ACL rules
				entryNum = mib_chain_total(MIB_OMCI_DM_MCAST_ACL_TBL);
				for(i=(entryNum-1) ; i>=0 ; i--)
				{
					if ( !mib_chain_get(MIB_OMCI_DM_MCAST_ACL_TBL, i, (void *)&acl_t) )
						continue;

					if(acl_t.profileId != profileId)
						continue;

					rtk_tr247_dynamic_acl_del(profileId, acl_t.aclTableEntryId);
					mib_chain_delete(MIB_OMCI_DM_MCAST_ACL_TBL, i);
				}

				//reset profileId for associate port config
				phyPortId = rtk_port_get_wan_phyID();
				entryNum = mib_chain_total(MIB_OMCI_DM_MCAST_PORT_TBL);
				for(i=0 ; i<entryNum ; i++)
				{
					if ( !mib_chain_get(MIB_OMCI_DM_MCAST_PORT_TBL, i, (void *)&port_t) )
						return -1;

					if(port_t.profileId != profileId)
						continue;

					// delete port unauthorized join rule
					if (port_t.portId == phyPortId) //WAN port
					{
						for (swPortIndex = 0; swPortIndex < SW_LAN_PORT_NUM; swPortIndex++)
						{
							strcpy(ifName, ELANVIF[swPortIndex]);
							rtk_tr247_multicast_port_unauthorized_join_behavior(&profile_t, &port_t, ifName, 0);
						}
					}
					else
					{
						if ((swPortIndex = rtk_if_name_get_by_lan_phyID(port_t.portId, NULL)) >= 0 &&
							mib_chain_get(MIB_SW_PORT_TBL, swPortIndex, &sw_port_entry))
						{
							for(j = 1; j<OMCI_DM_MAX_BRIDGE_NUM ; j++)
							{
#if defined(CONFIG_TR142_MODULE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
								if (sw_port_entry.omci_uni_portType == OMCI_PORT_TYPE_PPTP)
								{
									if((sw_port_entry.omci_uni_bridgeIdmask)&(1<<j))
									{
										sprintf(ifName, "%s_pptp_%d", ELANRIF[swPortIndex],j);
										apply = 1;
									}
								}
								else
#endif
								{
									strcpy(ifName, ELANVIF[swPortIndex]);
									j = OMCI_DM_MAX_BRIDGE_NUM;
									apply = 1;
								}
								if(apply)
									rtk_tr247_multicast_port_unauthorized_join_behavior(&profile_t, &port_t, ifName, 0);
							}
						}
					}

					port_t.profileId = 0xffff;
					mib_chain_update(MIB_OMCI_DM_MCAST_PORT_TBL, (void *)&port_t, i);
				}
				//printf("%s %d: SUCESS !\n", __func__, __LINE__);
				mib_ret = mib_chain_delete(MIB_OMCI_DM_MCAST_PROFILE_TBL, mibIndex);
				if(mib_ret <= 0)
				{
					//printf("%s %d: Error! MIB_OMCI_DM_MCAST_PORT_TBL delete chain record.\n", __func__, __LINE__);
				}
				else
				{
					//printf("%s %d: SUCESS !\n", __func__, __LINE__);
					ret = 0;
				}
			}
			else
			{
				//printf("%s %d: profileId=%d not existed !\n", __func__, __LINE__, profileId);
			}
			break;
		case TR247_MC_PORT_DEL:
			//AUG_PRT("TR247_MC_PORT_DEL\n");
			if((mibIndex = rtk_tr247_mcast_port_entry_find_by_port_id(portId, &port_t)) >= 0)
			{
				action = 0;
				ret = 0;
				ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_igmp_version, action);
				ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_snooping_enable, action);
				ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_max_group_number, action);
				ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_fast_leave, action);
				ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_dynamic_acl, action);
				//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_us_igmp_rate_limit, action);
				//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_igmp_function, action);
				//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_max_bw, action);
				//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_bw_enforce, action);
				//ret |= rtk_tr247_multicast_apply_port(port_t.portId, &rtk_tr247_multicast_port_us_igmp_tag_info, action);

				mib_ret = mib_chain_delete(MIB_OMCI_DM_MCAST_PORT_TBL, mibIndex);
				if(mib_ret <= 0)
				{
					//printf("%s %d: Error! MIB_OMCI_DM_MCAST_PORT_TBL delete chain record.\n", __func__, __LINE__);
				}
				else
				{
					//printf("%s %d: SUCESS !\n", __func__, __LINE__);
					ret = 0;
				}
			}
			else
			{
				//printf("%s %d: portId=%d not existed !\n", __func__, __LINE__, portId);
			}
			break;
		case TR247_MC_PORT_ADD:
			//AUG_PRT("TR247_MC_PORT_ADD\n");
			/* fall through */
		case TR247_MC_PROFILE_PER_PORT_ADD:
			//AUG_PRT("TR247_MC_PROFILE_PER_PORT_ADD\n");
			param[0] = portId;
			param[1] = profileId;
			param[2] = 0;
			pthread_t tid;
			pthread_create(&tid, NULL, rtk_tr247_multicast_port_add_thread, param);
			pthread_detach(tid);
			sleep(1);
			break;
		case TR247_MC_PORT_REAPPLY:
			if((swPortIndex = rtk_if_name_get_by_lan_phyID(portId, NULL)) >= 0 &&
				mib_chain_get(MIB_SW_PORT_TBL, swPortIndex, &sw_port_entry))
			{
				for(j = 1; j<OMCI_DM_MAX_BRIDGE_NUM ; j++)
				{
#if defined(CONFIG_TR142_MODULE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
					if(sw_port_entry.omci_uni_portType == OMCI_PORT_TYPE_PPTP)
					{
						if((sw_port_entry.omci_uni_bridgeIdmask)&(1<<j))
						{
							phyPortId = portId;
							sprintf(ifName, "%s_pptp_%d", ELANRIF[swPortIndex],j);
							apply = 1;
						}
					}
					else
#endif
					{
						phyPortId = rtk_port_get_wan_phyID();
						strcpy(ifName, ELANVIF[swPortIndex]);
						j = OMCI_DM_MAX_BRIDGE_NUM;
						apply = 1;
					}
					if(apply)
					{
						if((portIndex = rtk_tr247_mcast_port_entry_find_by_port_id(phyPortId, &port_t)) >= 0 &&
							(profIndex = rtk_tr247_mcast_profile_entry_find_by_profile_id(port_t.profileId, &profile_t)) >= 0)
						{
							ret = 0;
							ret |= rtk_tr247_multicast_port_igmp_version(&profile_t, &port_t, ifName, 1);
							ret |= rtk_tr247_multicast_port_snooping_enable(&profile_t, &port_t, ifName, 1);
							ret |= rtk_tr247_multicast_port_max_group_number(&profile_t, &port_t, ifName, 1);
							ret |= rtk_tr247_multicast_port_fast_leave(&profile_t, &port_t, ifName, 1);
							ret |= rtk_tr247_multicast_port_dynamic_acl(&profile_t, &port_t, ifName, 1);
							//ret |= rtk_tr247_multicast_port_igmp_function(&profile_t, &port_t, ifName, 1);
							//ret |= rtk_tr247_multicast_port_us_igmp_rate_limit(&profile_t, &port_t, ifName, 1);
							//ret |= rtk_tr247_multicast_port_max_bw(&profile_t, &port_t, ifName, 1);
							//ret |= rtk_tr247_multicast_port_bw_enforce(&profile_t, &port_t, ifName, 1);
							//ret |= rtk_tr247_multicast_port_us_igmp_tag_info(&profile_t, &port_t, ifName, 1);
							memcpy(port_t.ifName, ifName, sizeof(port_t.ifName));
							mib_chain_update(MIB_OMCI_DM_MCAST_PORT_TBL, (void *) &port_t, portIndex);
						}
						else
						{
							//printf("%s %d: portId=%d not existed !\n", __func__, __LINE__, portId);
						}
					}
				}

			}
			break;
		case TR247_MC_PROFILE_PER_PORT_DEL:
			//AUG_PRT("TR247_MC_PROFILE_PER_PORT_DEL\n");
			break;
		case TR247_MC_SNOOP_ENABLE_SET:
			//AUG_PRT("TR247_MC_SNOOP_ENABLE_SET\n");
			if(rtk_tr247_multicast_new_port_create(portId) == 0)
			{
				ret = rtk_tr247_multicast_apply_port(portId, &rtk_tr247_multicast_port_snooping_enable, 1);
			}
			//else
			//	printf("%s %d: rtk_tr247_multicast_new_port_create FAIL !\n", __func__, __LINE__);
			break;
		case TR247_MC_IGMP_VERSION_SET:
			//AUG_PRT("TR247_MC_IGMP_VERSION_SET\n");
			ret = rtk_tr247_multicast_apply_profile(profileId, &rtk_tr247_multicast_igmp_version);
			break;
		case TR247_MC_IGMP_FUNCTION_SET:
			//AUG_PRT("TR247_MC_IGMP_FUNCTION_SET\n");
			ret = rtk_tr247_multicast_apply_profile(profileId, &rtk_tr247_multicast_igmp_function);
			break;
		case TR247_MC_FAST_LEAVE_SET:
			//AUG_PRT("TR247_MC_FAST_LEAVE_SET\n");
			ret = rtk_tr247_multicast_apply_profile(profileId, &rtk_tr247_multicast_fast_leave);
			break;
		case TR247_MC_US_IGMP_RATE_SET:
			//AUG_PRT("TR247_MC_US_IGMP_RATE_SET\n");
			ret = rtk_tr247_multicast_apply_profile(profileId, &rtk_tr247_multicast_us_igmp_rate_limit);
			break;
		case TR247_MC_ROBUSTNESS_SET:
			//AUG_PRT("TR247_MC_ROBUSTNESS_SET\n");
			ret = rtk_tr247_multicast_apply_profile(profileId, &rtk_tr247_multicast_robustness);
			break;
		case TR247_MC_QUERIER_IPADDR_SET:
			//AUG_PRT("TR247_MC_QUERIER_IPADDR_SET\n");
			break;
		case TR247_MC_QUERY_INTERVAL_SET:
			//AUG_PRT("TR247_MC_QUERY_INTERVAL_SET\n");
			ret = rtk_tr247_multicast_apply_profile(profileId, &rtk_tr247_multicast_query_interval);
			break;
		case TR247_MC_QUERY_MAX_RESPONSE_TIME_SET:
			//AUG_PRT("TR247_MC_QUERY_MAX_RESPONSE_TIME_SET\n");
			ret = rtk_tr247_multicast_apply_profile(profileId, &rtk_tr247_multicast_query_max_response_time);
			break;
		case TR247_MC_LAST_MBR_QUERY_INTERVAL_SET:
			//AUG_PRT("TR247_MC_LAST_MBR_QUERY_INTERVAL_SET\n");
			ret = rtk_tr247_multicast_apply_profile(profileId, &rtk_tr247_multicast_last_member_query_interval);
			break;
		case TR247_MC_UNAUTHORIZED_JOIN_BEHAVIOR_SET:
			//AUG_PRT("TR247_MC_UNAUTHORIZED_JOIN_BEHAVIOR_SET\n");
			break;
		case TR247_MC_US_IGMP_TAG_INFO_SET:
			ret = rtk_tr247_multicast_apply_profile(profileId, &rtk_tr247_multicast_us_igmp_tag_info);
			break;
		case TR247_MC_MAX_GRP_NUM_SET:
			//AUG_PRT("TR247_MC_MAX_GRP_NUM_SET\n");
			if(rtk_tr247_multicast_new_port_create(portId) == 0)
			{
				ret = rtk_tr247_multicast_apply_port(portId, &rtk_tr247_multicast_port_max_group_number, 1);
			}
			//else
			//	printf("%s %d: rtk_tr247_multicast_new_port_create FAIL !\n", __func__, __LINE__);
			break;
		case TR247_MC_MAX_BW_SET:
			if(rtk_tr247_multicast_new_port_create(portId) == 0)
			{
				ret = rtk_tr247_multicast_apply_port(portId, &rtk_tr247_multicast_port_max_bw, 1);
			}
			//else
			//	printf("%s %d: rtk_tr247_multicast_new_port_create FAIL !\n", __func__, __LINE__);
			//AUG_PRT("TR247_MC_MAX_BW_SET\n");
			break;
		case TR247_MC_BW_ENFORCE_SET:
			if(rtk_tr247_multicast_new_port_create(portId) == 0)
			{
				ret = rtk_tr247_multicast_apply_port(portId, &rtk_tr247_multicast_port_bw_enforce, 1);
			}
			//else
			//	printf("%s %d: rtk_tr247_multicast_new_port_create FAIL !\n", __func__, __LINE__);
			//AUG_PRT("TR247_MC_BW_ENFORCE_SET\n");
			break;
		case TR247_MC_CURRENT_BW_GET:
			//AUG_PRT("TR247_MC_CURRENT_BW_GET\n");
			break;
		case TR247_MC_JOIN_MSG_COUNT_GET:
			//AUG_PRT("TR247_MC_JOIN_MSG_COUNT_GET\n");
			break;
		case TR247_MC_BW_EXCEEDED_COUNT_GET:
			//AUG_PRT("TR247_MC_BW_EXCEEDED_COUNT_GET\n");
			break;
		case TR247_MC_IPV4_ACTIVE_GROUP_GET:
			//AUG_PRT("TR247_MC_IPV4_ACTIVE_GROUP_GET\n");
			break;
		case TR247_MC_DYNAMIC_ACL_SET:
			//AUG_PRT("TR247_MC_DYNAMIC_ACL_SET\n");
			ret = rtk_tr247_dynamic_acl_set(profileId, aclTableEntryId);
			break;
		case TR247_MC_DYNAMIC_ACL_DEL:
			//AUG_PRT("TR247_MC_DYNAMIC_ACL_DEL\n");
			ret = rtk_tr247_dynamic_acl_del(profileId, aclTableEntryId);
			break;
		case TR247_MC_STATIC_ACL_SET:
			//AUG_PRT("TR247_MC_STATIC_ACL_SET\n");
			break;
		case TR247_MC_STATIC_ACL_DEL:
			//AUG_PRT("TR247_MC_STATIC_ACL_DEL\n");
			break;
		default:
			AUG_PRT("No such case !\n");
			break;
	}

	return ret;
}
#endif
#ifdef CONFIG_USER_RTK_MULTICAST_VID
static int rtk_iptv_mvlan_config(MIB_CE_ATM_VC_Tp pEntry)
{
	unsigned char ifName[IFNAMSIZ] = {0};
	int i,j;

	if(pEntry == NULL)
		return 0;

	ifGetName(PHY_INTF(pEntry->ifIndex), ifName, sizeof(ifName));

#if defined(CONFIG_RTL_SMUX_DEV)
	char alias_name[64] = {0}, cmd[256]={0}, *pcmd;

	sprintf(alias_name, "iptv_mvlan_%s", ifName);
	for(j=1 ; j<=SMUX_MAX_TAGS ; j++)
	{
		// remove rule for mvid
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
		sprintf(cmd, "%s --if %s --rx --tags %d --rule-remove-alias %s+", SMUXCTL, ELANRIF[pEntry->logic_port], j, alias_name);
#else
		sprintf(cmd, "%s --if %s --rx --tags %d --rule-remove-alias %s+", SMUXCTL, ALIASNAME_NAS0, j, alias_name);
#endif
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		sprintf(cmd, "%s --if %s --rx --tags %d --rule-remove-alias %s+", SMUXCTL, ifName, j, alias_name);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);

		if(pEntry->mVid <= 0)
			continue;

		// first rule for filter mvid.
		{
			pcmd = cmd;
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
			pcmd += sprintf(pcmd, "%s --if %s --rx --tags %d", SMUXCTL,  ELANRIF[pEntry->logic_port], j);
#else
			pcmd += sprintf(pcmd, "%s --if %s --rx --tags %d", SMUXCTL, ALIASNAME_NAS0, j);
#endif
			pcmd += sprintf(pcmd, " --filter-vid %d %d --filter-multicast --set-rxif %s --target ACCEPT", pEntry->mVid, j, ifName);
			pcmd += sprintf(pcmd, " --rule-priority %d --rule-alias %s --rule-insert", SMUX_RULE_PRIO_IGMP, alias_name);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
		}
		// second rule for translation mvid to vid;
		{
			pcmd = cmd;
			pcmd += sprintf(pcmd, "%s --if %s --rx --tags %d", SMUXCTL, ifName, j);
			pcmd += sprintf(pcmd, " --filter-vid %d %d --filter-multicast", pEntry->mVid, j);
			if(pEntry->vlan)
				pcmd += sprintf(pcmd, " --set-vid %d %d", pEntry->vid, j);
			else
				pcmd += sprintf(pcmd, " --pop-tag");
			pcmd += sprintf(pcmd, " --set-skb-mark2 0x%llx 0x%llx", SOCK_MARK2_RTK_IGMP_MLD_VID_SET(0,1), SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK);
			// don't set target action on mvid translation
			//pcmd += sprintf(pcmd, " --target CONTINUE");
			pcmd += sprintf(pcmd, " --rule-priority %d --rule-alias %s --rule-insert", SMUX_RULE_PRIO_IGMP, alias_name);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
		}
	}

#if defined(CONFIG_RTK_SKB_MARK2) && defined(CONFIG_USER_RTK_MULTICAST_VID_US)
	//for WAN tx, translation the IGMP and MLD packet's VLAN ID to mvid
	if(pEntry->mVid > 0)
	{
		for(j=0 ; j<=SMUX_MAX_TAGS ; j++)
		{
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
			sprintf(cmd, "%s --if %s --tx --tags %d --rule-remove-alias %s+", SMUXCTL, ELANRIF[pEntry->logic_port], j, alias_name);
#else
			sprintf(cmd, "%s --if %s --tx --tags %d --rule-remove-alias %s+", SMUXCTL, ALIASNAME_NAS0, j, alias_name);
#endif
			va_cmd("/bin/sh", 2, 1, "-c", cmd);

			pcmd = cmd;
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
			pcmd += sprintf(pcmd, "%s --if %s --tx --tags %d", SMUXCTL, ELANRIF[pEntry->logic_port], j);
#else
			pcmd += sprintf(pcmd, "%s --if %s --tx --tags %d", SMUXCTL, ALIASNAME_NAS0, j);
#endif
			pcmd += sprintf(pcmd, " --filter-txif %s --filter-skb-mark2 0x%llx 0x%llx", ifName, SOCK_MARK2_RTK_IGMP_MLD_VID_SET(0,1), SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK);
			if(j == 0){
				pcmd += sprintf(pcmd, " --push-tag --set-vid %d 1", pEntry->mVid);
			}
			else
				pcmd += sprintf(pcmd, " --set-vid %d %d", pEntry->mVid, j);
			pcmd += sprintf(pcmd, " --rule-priority %d --rule-alias %s --rule-insert", SMUX_RULE_PRIO_IGMP, alias_name);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
		}
	}
#endif

#else
	char cmd[256]={0};

	sprintf(cmd, "%s setsmux %s %s mvid %d", "/bin/ethctl", ALIASNAME_NAS0, ifName, pEntry->mVid);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif

	return 0;
}

int rtk_iptv_multicast_vlan_config(int configAll, MIB_CE_ATM_VC_Tp pEntry)
{
	MIB_CE_ATM_VC_T atm_vc_t;
	int entryNum, i, j;

	if(configAll == CONFIGONE && pEntry == NULL)
	{
		// error for CONFIGONE case and the WAN entry is NULL pointer
		return 1;
	}

	if(configAll != CONFIGONE)
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for(i=0 ; i<entryNum ; i++)
		{
			if ( !mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&atm_vc_t) )
				continue;
#if defined(CONFIG_CTC_SDN)
			if((atm_vc_t.enable && atm_vc_t.sdn_enable) &&
				check_ovs_enable() && check_ovs_running() == 0)
			{
				if(check_ovs_interface_by_mibentry(OFP_PTYPE_WAN, &atm_vc_t, i, -1) == 0)
				{
					continue;
				}
			}
#endif
			rtk_iptv_mvlan_config(&atm_vc_t);
		}
	}
	else if(pEntry)
		rtk_iptv_mvlan_config(pEntry);

#if defined(CONFIG_RTK_SKB_MARK2) && defined(CONFIG_USER_RTK_MULTICAST_VID_US)
	//for WAN tx, translation the IGMP or MLD packet's VLAN ID to mvid
	//use iptables to identify IGMP and MLD packet, because the SMUX cannot support to filter IPv6 MLD packet
	char cmd[256]={0};

	sprintf(cmd, "%s -t mangle -D POSTROUTING -p 0x2 -j MARK2 --set-mark2 0x%llX/0x%llX",
				IPTABLES, SOCK_MARK2_RTK_IGMP_MLD_VID_SET(0,1), SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t mangle -A POSTROUTING -p 0x2 -j MARK2 --set-mark2 0x%llX/0x%llX",
				IPTABLES, SOCK_MARK2_RTK_IGMP_MLD_VID_SET(0,1), SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef CONFIG_IPV6
	sprintf(cmd, "%s -t mangle -D POSTROUTING -p 0x3a -m icmpv6 --icmpv6-type 143 -j MARK2 --set-mark2 0x%llX/0x%llX",
				IP6TABLES, SOCK_MARK2_RTK_IGMP_MLD_VID_SET(0,1), SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t mangle -D POSTROUTING -p 0x3a -m icmpv6 --icmpv6-type 131 -j MARK2 --set-mark2 0x%llX/0x%llX",
				IP6TABLES, SOCK_MARK2_RTK_IGMP_MLD_VID_SET(0,1), SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t mangle -D POSTROUTING -p 0x3a -m icmpv6 --icmpv6-type 132 -j MARK2 --set-mark2 0x%llX/0x%llX",
				IP6TABLES, SOCK_MARK2_RTK_IGMP_MLD_VID_SET(0,1), SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	sprintf(cmd, "%s -t mangle -A POSTROUTING -p 0x3a -m icmpv6 --icmpv6-type 143 -j MARK2 --set-mark2 0x%llX/0x%llX",
				IP6TABLES, SOCK_MARK2_RTK_IGMP_MLD_VID_SET(0,1), SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t mangle -A POSTROUTING -p 0x3a -m icmpv6 --icmpv6-type 131 -j MARK2 --set-mark2 0x%llX/0x%llX",
				IP6TABLES, SOCK_MARK2_RTK_IGMP_MLD_VID_SET(0,1), SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t mangle -A POSTROUTING -p 0x3a -m icmpv6 --icmpv6-type 132 -j MARK2 --set-mark2 0x%llX/0x%llX",
				IP6TABLES, SOCK_MARK2_RTK_IGMP_MLD_VID_SET(0,1), SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
#endif

	return 0;
}
#if defined(CONFIG_IPV6) && defined(CONFIG_RTK_SKB_MARK2)
//RA packet belong to multicast packet, we'd better not make RA packet passed for multicast rule,otherwise, wan will have two global addr
//this rule will be deleted in stopConnection
int rtk_block_multicast_ra()
{
	MIB_CE_ATM_VC_T Entry;
	int entryNum=mib_chain_total(MIB_ATM_VC_TBL);
	unsigned char ifName[IFNAMSIZ] = {0};
	int i;
	char cmd[256] ={0};

	for (i = 0; i < entryNum; i++) /* Check have any ROUTING WAN, IulianWu*/
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;
		if ((Entry.enable == 0) ||(!(Entry.IpProtocol &IPVER_IPV6))  ||  !(Entry.mVid))
			continue;

		ifGetName(PHY_INTF(Entry.ifIndex), ifName, sizeof(ifName));

		if ((Entry.vlan && (Entry.mVid != Entry.vid)) || (!Entry.vlan)){
			snprintf(cmd,sizeof(cmd) ,"%s -I INPUT -i %s -p icmpv6 --icmpv6-type router-advertisement -m mark2 --mark2 0x%08llX/0x%08llX -j DROP",
				IP6TABLES, ifName, SOCK_MARK2_RTK_IGMP_MLD_VID_SET(0,1), SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
		}
	}
	return 0;
}
#endif
#endif

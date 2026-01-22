/* startup.c - kaohj */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/route.h>
#include <linux/atm.h>
#include <linux/atmdev.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/resource.h>
#include "fc_api.h"
#include "utility.h"
#include "mib.h"
#ifdef CONFIG_RTK_DNS_TRAP
#include "sockmark_define.h"
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
#include <rtk/pon_led.h>
#endif
#include <dirent.h>
#ifdef EMBED
#include <linux/config.h>
#else
#include "../../../../include/linux/autoconf.h"
#endif
#include "../defs.h"

#include "mibtbl.h"
#include "utility.h"
#ifdef WLAN_SUPPORT

#include <linux/wireless.h>

#if defined(CONFIG_E8B) || defined(CONFIG_00R0)
#ifdef USE_LIBMD5
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#undef uint32
#endif
#include <libmd5wrapper.h>
#else
#include "../md5.h"
#endif //USE_LIBMD5
#endif

#endif

#include "debug.h"
// Mason Yu
#include "syslog.h"
#include <sys/stat.h>

#if defined(CONFIG_GPON_FEATURE)
#include "rtk/gpon.h"
#if defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_gpon.h>
#endif
#ifdef CONFIG_RTK_OMCI_V1
#include <omci_api.h>
#include <gos_type.h>
#endif
#endif
#if defined(CONFIG_EPON_FEATURE)
#include "rtk/epon.h"
#if defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_epon.h>
#endif
#endif

#include "subr_net.h"

#ifdef CONFIG_CU
#include "subr_cu.h"
#endif

#if defined(CONFIG_CTC_SDN)
#include "subr_ctcsdn.h"
#endif

#include "subr_platform.h"
#include <linux/version.h>
#ifdef CONFIG_LIB_LIBRTK_LED
#include "librtk_led/librtk_led.h"
#endif

extern void createFTPAclChain();
extern void setup_ftp_servers();

int startLANAutoSearch(const char *ipAddr, const char *subnet);
int isDuplicate(struct in_addr *ipAddr, const char *device);

#ifdef CUSTOMIZE_SSID//CONFIG_USER_AP_CMCC
static void convert_bin_to_str(unsigned char *bin, int len, char *out)
{
	int i;
	char tmpbuf[10];

	out[0] = '\0';

	for (i=0; i<len; i++) {
		sprintf(tmpbuf, "%02x", bin[i]);
		strcat(out, tmpbuf);
	}
}

#ifdef WLAN_SUPPORT
#ifdef CUSTOMIZE_SSID_GW
int set_ssid_customize_gw(int flag)
{
	unsigned char ssid[MAX_SSID_LEN] = {0};
	unsigned char mac[MAC_ADDR_LEN] = {0};
	unsigned char macstr[MAC_ADDR_LEN*2+1] = {0}, macstrtmp[MAC_ADDR_LEN+1] = {0};
	int i = 0, j=0, len=0, vwlan_idx;
	unsigned char phyBandSelect = 0, is_guest = 0, pos=0;
	MIB_CE_MBSSIB_T Wlans_Entry;

#ifdef CUSTOMER_HW_SETTING_SUPPORT
	unsigned char hw_ssid[MAX_SSID_LEN] = {0};
	int set_ssid = 0;
#endif

	mib_save_wlanIdx();

	for(j=0; j<NUM_WLAN_INTERFACE;++j)
	{
		wlan_idx = j;
		vwlan_idx = 0;
		//set_ssid = 0;
		//mib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyBandSelect);
		mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyBandSelect,sizeof(phyBandSelect));
		if(NUM_WLAN_INTERFACE==2){
			if( phyBandSelect==PHYBAND_5G ){
				#ifdef CONFIG_USER_AP_CMCC
				len = strlen(CUSTOMIZE_CMCC_SSID_PREFIX_GW_5G);
				memcpy(ssid, CUSTOMIZE_CMCC_SSID_PREFIX_GW_5G, len);
				#else
				len = strlen(CUSTOMIZE_CT_SSID_PREFIX_GW_5G);
				memcpy(ssid, CUSTOMIZE_CT_SSID_PREFIX_GW_5G, len);
				#endif
			}else if( phyBandSelect==PHYBAND_2G){
				#ifdef CONFIG_USER_AP_CMCC
				len = strlen(CUSTOMIZE_CMCC_SSID_PREFIX_GW_2G);
				memcpy(ssid, CUSTOMIZE_CMCC_SSID_PREFIX_GW_2G, len);
				#else
				len = strlen(CUSTOMIZE_CT_SSID_PREFIX_GW_2G);
				memcpy(ssid, CUSTOMIZE_CT_SSID_PREFIX_GW_2G, len);
				#endif
			}
		}else {
			#ifdef CONFIG_USER_AP_CMCC
			len = strlen("CMCC_");
			memcpy(ssid, "CMCC_", len);
			#else
			len = strlen("ChinaNet-");
			memcpy(ssid, "ChinaNet-", len);
			#endif
		}

#ifdef CUSTOMER_HW_SETTING_SUPPORT
		if(flag == HW_SSID_GEN){
			if(wlan_idx==0){
				//mib_get(MIB_CUSTOMER_HW_WLAN0_SSID, (void *)&hw_ssid);
				mib_get_s(MIB_CUSTOMER_HW_WLAN0_SSID, (void *)hw_ssid,sizeof(hw_ssid));
			}else if(wlan_idx==1){
				//mib_get(MIB_CUSTOMER_HW_WLAN1_SSID, (void *)&hw_ssid);
				mib_get_s(MIB_CUSTOMER_HW_WLAN1_SSID, (void *)hw_ssid,sizeof(hw_ssid));
			}
			if(strlen(hw_ssid)==0)
				set_ssid = 1;
		}
#endif
		for(i=0; i<=NUM_VWLAN_INTERFACE;++i)
		{
			vwlan_idx = i;
#ifdef RESERVE_KEY_SETTING
#ifdef CONFIG_E8B
#ifdef WLAN_UNIVERSAL_REPEATER
			if(vwlan_idx != 0 && vwlan_idx != NUM_VWLAN_INTERFACE)
				continue;
#else
			if(vwlan_idx != 0)
				continue;
#endif
#endif
#endif

			//mib_get(MIB_ELAN_MAC_ADDR, (void *)&mac);
			mib_get_s(MIB_ELAN_MAC_ADDR, (void *)mac,sizeof(mac));

			if(vwlan_idx==1)
				(*(char *)(mac+5)) += 0x01;
			else if(vwlan_idx==2)
				(*(char *)(mac+5)) += 0x02;
			else if(vwlan_idx==3)
				(*(char *)(mac+5)) += 0x03;
			else if(vwlan_idx==4)
				(*(char *)(mac+5)) += 0x04;
			else if(vwlan_idx==5)
				(*(char *)(mac+5)) += 0x05;

			convert_bin_to_str(mac, MAC_ADDR_LEN, macstr);
			pos = len;
			memcpy(ssid+pos, macstr+6, 6);
			pos += 6;

			//98D no guest wlan access mode
			/*
			mib_get(MIB_WLAN_ACCESS, (void *)&is_guest);
			if(is_guest){
				memcpy(ssid+pos, CMCC_GUEST_SUFFIX, strlen(CMCC_GUEST_SUFFIX));
				pos += strlen(CMCC_GUEST_SUFFIX);
			}
			*/
			ssid[pos] = '\0';

#ifndef CUSTOMER_HW_SETTING_SUPPORT
			memset(&Wlans_Entry,0,sizeof(MIB_CE_MBSSIB_T));
	        mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Wlans_Entry);
#ifdef CONFIG_DEFAULT_CONF_ON_RT_XMLFILE
#ifdef CONFIG_USER_AP_CMCC
		if(strstr(Wlans_Entry.ssid,"CMCC"))
			continue;
#endif
#endif
			strncpy(Wlans_Entry.ssid, ssid,MAX_SSID_LEN);
			mib_chain_update(MIB_MBSSIB_TBL, (void *)&Wlans_Entry, vwlan_idx);
#else

			if(set_ssid && (vwlan_idx==0)){
				mib_set(MIB_WLAN_SSID, (void *)&ssid);
			}
			if(vwlan_idx)
				mib_set(MIB_WLAN_SSID, (void *)&ssid);


#endif

		}
	}

	mib_recov_wlanIdx();
	return 0;
}
#endif
#endif

int set_ssid_customize(int flag)
{
	unsigned char op_mode = 0;

	printf("SSID Customized !\n");
	//mib_get(MIB_OP_MODE, (void *)&op_mode);
	mib_get_s(MIB_OP_MODE, (void *)&op_mode,sizeof(op_mode));

	if(op_mode==GATEWAY_MODE)
	{
#ifdef WLAN_SUPPORT
#ifdef CUSTOMIZE_SSID_GW
		set_ssid_customize_gw(flag);
#endif
#endif
	}
#ifdef CMCC_SSID_CUSTOMIZE_AP_RPT
	else if(op_mode==BRIDGE_MODE)
	{
		set_ssid_customize_ap_repeater(flag);
	}else
		printf("[%s %d]Invalid OPMODE %d\n", __FUNCTION__,__LINE__,op_mode);
#endif

	return 0;
}


#endif

#ifdef CUSTOMIZE_SSID//CONFIG_USER_AP_CMCC
int up_ssid_value()
{
    unsigned char new_ver=0;

	//mib_get(MIB_SSID_VER, (void *)&new_ver);
	mib_get_s(MIB_SSID_VER, (void *)&new_ver,sizeof(new_ver));
	//printf("##### new_ver:%d\n", new_ver);
	if( new_ver ){
		return -1;
	}else{
		return 0;
	}
}
#endif

#ifdef _PRMT_X_CMCC_LANINTERFACES_
static void check_l2filter(void)
{
	int total = mib_chain_total(MIB_L2FILTER_TBL);
	int i;
	MIB_CE_L2FILTER_T entry;

	if(total >= PMAP_ITF_END)
		return;

	memset(&entry, 0, sizeof(MIB_CE_L2FILTER_T));

	for(i = total ; i < PMAP_ITF_END ; i++)
		mib_chain_add(MIB_L2FILTER_TBL, &entry);

	return;
}

static void check_elan_conf(void)
{
	int total = mib_chain_total(MIB_ELAN_CONF_TBL);
	int i;
	MIB_CE_ELAN_CONF_T entry;

	if(total >= ELANVIF_NUM)
		return;

	memset(&entry, 0, sizeof(MIB_CE_ELAN_CONF_T));

	for(i = total ; i < ELANVIF_NUM ; i++)
		mib_chain_add(MIB_ELAN_CONF_TBL, &entry);

	return;
}
#endif

#ifdef CONFIG_DEV_xDSL
#ifdef CONFIG_USER_RTK_DSL_LOG
int startDslLog(void)
{
	int fdconf;
	char buff[64];
	const char username[] = "adsl2";
	const char password[] = "adsl2";
	const char serverip[] = "192.168.88.100";
	const char storepath[] = "/var/config";
	const char configfile[] = "/var/dsl_autolog.conf";

	fdconf = open(configfile, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
	if (fdconf != -1) {
		snprintf(buff, 64, "username %s\n", username);
		if ( write(fdconf, buff, strlen(buff)) != strlen(buff) ) {
			printf("Write dsl_autolog.conf file error(username)!\n");
		}

		snprintf(buff, 64, "password %s\n", password);
		if ( write(fdconf, buff, strlen(buff)) != strlen(buff) ) {
			printf("Write dsl_autolog.conf file error(password)!\n");
		}

		snprintf(buff, 64, "serverip %s\n", serverip);
		if ( write(fdconf, buff, strlen(buff)) != strlen(buff) ) {
			printf("Write dsl_autolog.conf file error(serverip)!\n");
		}

		snprintf(buff, 64, "storepath %s\n", storepath);
		if ( write(fdconf, buff, strlen(buff)) != strlen(buff) ) {
			printf("Write dsl_autolog.conf file error(storepath)!\n");
		}
		close(fdconf);
		va_cmd("/bin/dsl_autolog", 2 , 1, "-c", configfile);
	}
	return 0;
}
#endif

//--------------------------------------------------------
// xDSL startup
// return value:
// 0  : not start by configuration
// 1  : successful
// -1 : failed
int startDsl()
{
	unsigned char init_line;
	unsigned short dsl_mode;
	int adslmode;
	int ret;

#ifdef CONFIG_VDSL
	//enable/disable dsl log
	system("/bin/adslctrl debug 9");
#endif /*CONFIG_VDSL*/


	ret = 1;
	if (mib_get_s(MIB_INIT_LINE, (void *)&init_line, sizeof(init_line)) != 0) {
		if (init_line == 1) {
			// start adsl
		  #ifdef CONFIG_VDSL
			adslmode=0;
		  #else
			mib_get_s(MIB_ADSL_MODE, (void *)&dsl_mode, sizeof(dsl_mode));
			adslmode=(int)(dsl_mode & (ADSL_MODE_GLITE|ADSL_MODE_T1413|ADSL_MODE_GDMT));	// T1.413 & G.dmt
		  #endif /*CONFIG_VDSL*/
			adsl_drv_get(RLCM_PHY_START_MODEM, (void *)&adslmode, 4);
			ret = setupDsl();

		  #if defined(CONFIG_USER_XDSL_SLAVE)
			adsl_slv_drv_get(RLCM_PHY_START_MODEM, (void *)&adslmode, 4);
			ret = setupSlvDsl();
		  #endif

		}
		else
			ret = 0;
	}
	else
		ret = -1;
	return ret;
}
#endif


//--------------------------------------------------------
// Find the minimun WLAN-side link MRU
// It is used to set the LAN-side MTU(MRU) for the
// path-mtu problem.
// RETURN: 0: if failed
//	 : others: the minimum MRU for the WLAN link
static int get_min_wan_mru()
{
	int vcTotal, i, pmtu;
	MIB_CE_ATM_VC_T Entry;

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	pmtu = 1500;

	for (i = 0; i < vcTotal; i++)
	{
		/* get the specified chain record */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return 0;

		if (Entry.enable == 0)
			continue;

		if (Entry.mtu < pmtu)
			pmtu = Entry.mtu;
	}

	return pmtu;
}

//--------------------------------------------------------
// Ethernet LAN startup
// return value:
// 1  : successful
// -1 : failed
#define ConfigWlanLock "/var/run/configWlanLock"
#define ConfigACLLock "/var/run/configACLLock"

#if defined(CONFIG_USER_MSTPD)
#define READ_BUF_SIZE	50

int getPid_fromFile(char *file_name)
{
	FILE *fp;
	char *pidfile = file_name;
	int result = -1;

	fp= fopen(pidfile, "r");
	if (!fp) {
        	printf("can not open:%s\n", file_name);
		return -1;
   	}
	fscanf(fp,"%d",&result);
	fclose(fp);

	return result;
}

pid_t find_pid_by_name( char* pidName)
{
	DIR *dir;
	struct dirent *next;

	pid_t pid;

	if ( strcmp(pidName, "init")==0)
		return 1;

	dir = opendir("/proc");
	if (!dir) {
		printf("Cannot open /proc");
		return 0;
	}

	while ((next = readdir(dir)) != NULL) {
		FILE *status;
		char filename[READ_BUF_SIZE];
		char buffer[READ_BUF_SIZE];
		char name[READ_BUF_SIZE];

		/* Must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, "..") == 0)
			continue;

		/* If it isn't a number, we don't want it */
		if (!isdigit(*next->d_name))
			continue;

		sprintf(filename, "/proc/%s/status", next->d_name);
		if (! (status = fopen(filename, "r")) ) {
			continue;
		}
		if (fgets(buffer, READ_BUF_SIZE-1, status) == NULL) {
			fclose(status);
			continue;
		}
		fclose(status);

		/* Buffer should contain a string like "Name:   binary_name" */
		sscanf(buffer, "%*s %s", name);
		if (strcmp(name, pidName) == 0) {
		//	pidList=xrealloc( pidList, sizeof(pid_t) * (i+2));
			pid=(pid_t)strtol(next->d_name, NULL, 0);
			closedir(dir);
			return pid;
		}
	}
	closedir(dir);
	return 0;
}
#endif

int startELan()
{
	unsigned char value[6];
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	unsigned char rgmii_port[32];
	int rgmii_idx;
	char tmpStr[128];
	char mcIfname[IFNAMSIZ] = {0};
#endif
#ifdef WLAN_GEN_MBSSID_MAC
	unsigned char gen_wlan_mac[MAC_ADDR_LEN];
#endif
	int vInt;
	char macaddr[13];
	char ipaddr[16];
	char subnet[16];
	char timeOut[6];
	int status=0;
	int i;
#if defined(CONFIG_IPV6)
	char tmpBuf[64];
#endif
#ifdef WLAN_SUPPORT
	char para2[20];
	int k, orig_wlan_idx;
#endif
	FILE *f;
	int portid;
	char sysbuf[128];
#ifndef CONFIG_RTL_MULTI_PHY_ETH_WAN
	int ethPhyPortId = rtk_port_get_wan_phyID();
#endif

#ifdef CONFIG_USER_DHCPCLIENT_MODE
	unsigned char cur_vChar;
#endif
	unsigned int mtu;
	char mtuStr[10];
	mtu=rtk_lan_get_max_lan_mtu();
	snprintf(mtuStr, sizeof(mtuStr), "%u", mtu);

#ifdef CONFIG_RTL8672NIC
#ifdef WIFI_TEST
	//for wifi test
	mib_get_s(MIB_WLAN_BAND, (void *)value, sizeof(value));
	if(value[0]==4 || value[0]==5){//wifi
		status|=va_cmd("/bin/ethctl",2,1,"wifi","1");
	}
	else
#endif
	{
#ifdef WLAN_SUPPORT
	// to support WIFI logo test mode.....
	mib_get_s(MIB_WIFI_SUPPORT, (void*)value, sizeof(value));
	if(value[0]==1)
	{
		MIB_CE_MBSSIB_T mEntry;
		wlan_getEntry(&mEntry, 0);
		if(mEntry.wlanBand==2 || mEntry.wlanBand==3)
			status|=va_cmd("/bin/ethctl",2,1,"wifi","1");
		else
			status|=va_cmd("/bin/ethctl",2,1,"wifi","0");
	}
	else
		status|=va_cmd("/bin/ethctl",2,1,"wifi","0");
#endif
	}
#endif

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
#if defined(CONFIG_LAN_SDS_FEATURE)
	system("echo 5 > /proc/lan_sds/lan_sds_cfg");
#endif
#if defined(CONFIG_RTL8367_SUPPORT) && defined(CONFIG_LIB_8367)
	rtk_switch_init();
#endif
#endif

	if (mib_get_s(MIB_ELAN_MAC_ADDR, (void *)value, sizeof(value)) != 0)
	{
		unsigned char base_mac[6];
		memcpy(base_mac, value, sizeof(base_mac));
#ifdef WLAN_SUPPORT
		if((f = fopen(ConfigWlanLock, "w")) == NULL)
			return -1;
		fclose(f);
#ifdef RESTRICT_PROCESS_PERMISSION
	 	chmod (ConfigWlanLock, 0666);
#endif
#endif
#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		if((f = fopen(ConfigACLLock, "w")) == NULL)
			return -1;
		fclose(f);
#ifdef RESTRICT_PROCESS_PERMISSION
		chmod (ConfigACLLock, 0666);
#endif
#endif
		rtk_sockmark_init();
#if defined(CONFIG_YUEME)
		rtk_init_interface_packet_statistics_log_file();
#endif

		snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
			value[0], value[1], value[2], value[3], value[4], value[5]);
		for(i=0;i<ELANVIF_NUM;i++){
			status|=va_cmd(IFCONFIG, 4, 1, ELANRIF[i], "hw", "ether", macaddr);
		}
#if defined(CONFIG_RTL8681_PTM)
		status|=va_cmd(IFCONFIG, 4, 1, PTMIF, "hw", "ether", macaddr);
#endif
#ifdef CONFIG_USB_ETH
		status|=va_cmd(IFCONFIG, 4, 1, USBETHIF, "hw", "ether", macaddr);
#endif //CONFIG_USB_ETH
#if defined(WLAN_SUPPORT) && !(IS_AX_SUPPORT)
		orig_wlan_idx = wlan_idx;
		for (k=0; k<NUM_WLAN_INTERFACE; k++) {
			wlan_idx = k;
#if !defined(WLAN_RTK_MULTI_AP)
			//if enable mesh, let wlan MAC address different with br0, because the mesh error connect type issue
			if (k>0)
#endif
			{
				setup_mac_addr(value, 1);
				snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
                    value[0], value[1], value[2], value[3], value[4], value[5]);
			}
			rtk_wlan_get_ifname(k, 0, para2);
			status|=va_cmd(IFCONFIG, 4, 1, para2, "hw", "ether", macaddr);
			status|=va_cmd(IFCONFIG, 3, 1, para2, "mtu", mtuStr);

#ifdef WLAN_MBSSID
			// Set macaddr for VAP
			for (i=1; i<=WLAN_MBSSID_NUM; i++) {
#ifdef WLAN_GEN_MBSSID_MAC
				_gen_guest_mac(value, MAX_WLAN_VAP+1, i, gen_wlan_mac);
				snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
					gen_wlan_mac[0], gen_wlan_mac[1], gen_wlan_mac[2], gen_wlan_mac[3], gen_wlan_mac[4], gen_wlan_mac[5]);
#else
				setup_mac_addr(value, 1);
				snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
					value[0], value[1], value[2], value[3], value[4], value[5]);
#endif
				rtk_wlan_get_ifname(k, i, para2);
				status|=va_cmd(IFCONFIG, 4, 1, para2, "hw", "ether", macaddr);
				status|=va_cmd(IFCONFIG, 3, 1, para2, "mtu", mtuStr);
			}
#endif
#if defined(WLAN_UNIVERSAL_REPEATER) && defined(WLAN_11AX)
			/* reserve mac address for wifi6 driver repeater interface */
#ifdef WLAN_GEN_MBSSID_MAC
			_gen_guest_mac(value, MAX_WLAN_VAP+1, WLAN_REPEATER_ITF_INDEX, gen_wlan_mac);
#else
			setup_mac_addr(value, 1);
#endif
#endif // WLAN_UNIVERSAL_REPEATER && WLAN_11AX
		}
		wlan_idx = orig_wlan_idx;
#endif // WLAN_SUPPORT
	}

#if defined(CONFIG_USER_VXLAN)
#define ConfigVxlanLock "/var/run/configVxlanLock"
	if((f = fopen(ConfigVxlanLock, "w")) == NULL)
		return -1;
	fclose(f);
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_LANNETINFO)
	#define NETDEV_PORT_MAPPING "/var/dev_port_mapping"
	unlink(NETDEV_PORT_MAPPING);
#endif

	va_cmd(IFCONFIG, 2, 1, ELANIF, "up");
	va_cmd(IFCONFIG, 3, 1, ELANIF, "mtu", mtuStr);

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	mib_get_s(MIB_EXTERNAL_SWITCH_RGMII_PORT, (void *)rgmii_port, sizeof(rgmii_port));
	rgmii_idx = rtk_fc_external_switch_rgmii_port_get();
	rtk_port_set_dev_port_mapping(atoi(rgmii_port), (char *)ELANRIF[rgmii_idx]);
	va_cmd(IFCONFIG, 2, 1, ELANRIF[rgmii_idx], "up");
	va_cmd(IFCONFIG, 3, 1, ELANRIF[rgmii_idx], "mtu", mtuStr);
	snprintf(tmpStr, sizeof(tmpStr), "echo carrier %s on > /proc/rtk_smuxdev/interface", ELANRIF[rgmii_idx]);
	system(tmpStr);
#endif

	for(i=0;i<ELANVIF_NUM;i++)
	{
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
		if(rtk_port_check_external_port(i))
		{
#if defined(CONFIG_RTL_MULTI_LAN_DEV) && defined(CONFIG_RTL_SMUX_DEV)
			rtk_external_port_set_dev_port_mapping(rtk_external_port_get_lan_phyID(i), (char *)EXTELANRIF[i]);
			va_cmd(IFCONFIG, 2, 1, EXTELANRIF[i], "up");
			va_cmd(IFCONFIG, 3, 1, EXTELANRIF[i], "mtu", mtuStr);
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_LANNETINFO) //because the /proc/rtl8686gmac/dev_port_mapping can't read. so record here
			sprintf(sysbuf, "echo %s %d >> %s", ELANVIF[i], rtk_port_get_lan_phyID(i), NETDEV_PORT_MAPPING);
			va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
#endif
#endif
		}
		else
#endif
		{
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
			if (rtk_port_is_wan_logic_port(i))
				continue;
#endif

			portid = rtk_port_get_lan_phyID(i);
			if (portid != -1)
			{
#ifndef CONFIG_RTL_MULTI_PHY_ETH_WAN
				if (portid == ethPhyPortId)
					continue;
#endif

				rtk_port_set_dev_port_mapping(portid, (char *)ELANRIF[i]);

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_LANNETINFO) //because the /proc/rtl8686gmac/dev_port_mapping can't read. so record here
				sprintf(sysbuf, "echo %s %d >> %s", ELANVIF[i], portid, NETDEV_PORT_MAPPING);
				va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
#endif
			}
			va_cmd(IFCONFIG, 2, 1, ELANRIF[i], "up");
			va_cmd(IFCONFIG, 3, 1, ELANRIF[i], "mtu", mtuStr);
		}
#if defined(CONFIG_RTL_SMUX_DEV)
		Init_LanPort_Def_Intf(i);
#endif
	}

	// ifconfig eth0 up
	//va_cmd(IFCONFIG, 2, 1, "eth0", "up");

	// brctl addbr br0
	status|=va_cmd(BRCTL, 2, 1, "addbr", (char*)BRIF);

	// ifconfig br0 hw ether
	if (mib_get_s(MIB_ELAN_MAC_ADDR, (void *)value, sizeof(value)) != 0)
	{
		snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
		value[0], value[1], value[2], value[3], value[4], value[5]);
		va_cmd(IFCONFIG, 4, 1, BRIF, "hw", "ether", macaddr);
		va_cmd(IFCONFIG, 3, 1, BRIF, "mtu", mtuStr);
	}

#if defined(WLAN_SUPPORT) && defined(CONFIG_LUNA) && defined(CONFIG_RTL_MULTI_LAN_DEV)
	va_cmd(IFCONFIG, 4, 1, ELANIF, "hw", "ether", macaddr);
#endif

	// enable Reverse Path Filtering for ip spoofing (IPv4 Only)
	snprintf(sysbuf, sizeof(sysbuf),"echo 1 > /proc/sys/net/ipv4/conf/%s/rp_filter", BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", sysbuf);

#if !defined(CONFIG_LUNA) && !defined(CONFIG_DSL_VTUO)
	//setup WAN to WAN blocking
	system("/bin/echo 1 > /proc/br_wanblocking");
#endif

#if defined(CONFIG_USER_MSTPD)
{
	int pid = find_pid_by_name("mstpd");
	int stp_state = getPid_fromFile("/sys/class/net/br0/bridge/stp_state");

	if (pid <= 0 || stp_state != 2 || (pid > 0 && stp_state == 2))
		system("brctl stp br0 off;sleep 1");

	if (pid <= 0) {
		system("mstpd;sleep 2");
	}
}
#endif

	if (mib_get_s(MIB_BRCTL_STP, (void *)value, sizeof(value)) != 0)
	{
		vInt = (int)(*(unsigned char *)value);
		if (vInt == 0)	// stp off
		{
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
			rtk_fc_stp_clean();
#endif
			// brctl stp br0 off
			status|=va_cmd(BRCTL, 3, 1, "stp", (char *)BRIF, "off");

			// brctl setfd br0 1
			//if forwarding_delay=0, fdb_get may fail in serveral seconds after booting
			status|=va_cmd(BRCTL, 3, 1, "setfd", (char *)BRIF, "1");
		}
		else		// stp on
		{
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
			rtk_fc_stp_setup();
#endif
			// brctl stp br0 on
			status|=va_cmd(BRCTL, 3, 1, "stp", (char *)BRIF, "on");
		}
	}

	// brctl setageing br0 ageingtime
	if (mib_get_s(MIB_BRCTL_AGEINGTIME, (void *)value, sizeof(value)) != 0)
	{
		vInt = (int)(*(unsigned short *)value);
		snprintf(timeOut, 6, "%u", vInt);
		status|=va_cmd(BRCTL, 3, 1, "setageing", (char *)BRIF, timeOut);
	}

#ifdef CONFIG_UNI_CAPABILITY
	setupUniPortCapability();
#endif
	for(i=0;i<ELANVIF_NUM;i++)
	{
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
		if (rtk_port_is_wan_logic_port(i))
			continue;
#else
		portid = rtk_port_get_lan_phyID(i);
		if (portid != -1)
		{
			if (portid == ethPhyPortId)
				continue;
		}
#endif
		status|=va_cmd(IFCONFIG, 3, 1, ELANVIF[i], "mtu", mtuStr);
		status|=va_cmd(BRCTL, 3, 1, "addif", (char *)BRIF, ELANVIF[i]);
#ifdef CONFIG_RTK_DEV_AP
#else
// Mason Yu
#ifdef NAT_LOOPBACK
		// Use hairpin_mode with caution, ex. flooding traffic would be looped
		// forever between two connected device where they are both in hairpin_mode.
		#if 0
		sprintf(sysbuf, "/bin/echo 1 >  /sys/class/net/br0/brif/%s/hairpin_mode", ELANVIF[i]);
		printf("system(): %s\n", sysbuf);
		system(sysbuf);
		#endif
#endif
#endif

		va_cmd(IFCONFIG, 2, 1, ELANVIF[i], "down");
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
		if(rtk_port_check_external_port(i))
		{
			va_cmd(IFCONFIG, 2, 1, EXTELANRIF[i], "down");
		}
		else
#endif
		{
			va_cmd(IFCONFIG, 2, 1, ELANRIF[i], "down");
		}
		va_cmd(IFCONFIG, 2, 1, ELANVIF[i], "up");
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
		if(rtk_port_check_external_port(i))
		{
			va_cmd(IFCONFIG, 2, 1, EXTELANRIF[i], "up");
		}
		else
#endif
		{
			va_cmd(IFCONFIG, 2, 1, ELANRIF[i], "up");
		}
		restart_ethernet((i + 1));
	}

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	///-- Add interface eth0.x.igmp into bridge to handle all IGMP in our SoC
	sprintf(mcIfname, "%s.%s", ELANRIF[rgmii_idx], "igmp");
	status|=va_cmd(IFCONFIG, 3, 1, mcIfname, "mtu", mtuStr);
	status|=va_cmd(BRCTL, 3, 1, "addif", (char *)BRIF, mcIfname);
	va_cmd(IFCONFIG, 2, 1, mcIfname, "up");
	va_cmd(IFCONFIG, 3, 1, mcIfname, "mtu", mtuStr);
#endif

#if defined(CONFIG_RTL8681_PTM)
	status|=va_cmd(BRCTL, 3, 1, "addif", (char *)BRIF, PTMIF);
#endif
#ifdef CONFIG_USB_ETH
	status|=va_cmd(BRCTL, 3, 1, "addif", (char *)BRIF, USBETHIF);
#endif //CONFIG_USB_ETH

	/* Mason Yu. 2011/04/12
	 * In order to wait if the ALL LAN bridge ports is ready or not. Set dad probes amount to 4 for br0.
	 */
    /*
     * 2012/8/15
     * Since the eth0.2~5 up timing is tuned to must more later, so 4 is not enough, the first 5 NS
     * could not be send out until eth0.2~5 are up.
     */

	/* 2015/8/7 4 is enough now, change for IPv6 ready log test*/
	snprintf(sysbuf, sizeof(sysbuf), "echo 4 > /proc/sys/net/ipv6/conf/%s/dad_transmits", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", sysbuf);

#ifndef CONFIG_RTK_DEV_AP
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	mib_get_s(MIB_DHCP_MODE, (void *)&cur_vChar, sizeof(cur_vChar));

	/* OTHER : assign static LAN ip
	** DHCPV4_LAN_CLIENT : dynamic get LAN ip, so postpone to "/bin/diag port set phy-force-power-down port all state disable"
	*/
	if(cur_vChar != DHCPV4_LAN_CLIENT)
#endif
#endif
	{
		// ifconfig LANIF LAN_IP netmask LAN_SUBNET
		if (mib_get_s(MIB_ADSL_LAN_IP, (void *)value, sizeof(value)) != 0)
		{
			strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
			ipaddr[15] = '\0';
		}
		if (mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)value, sizeof(value)) != 0)
		{
			strncpy(subnet, inet_ntoa(*((struct in_addr *)value)), 16);
			subnet[15] = '\0';
		}

		/* To reduce unnecessary nf_conntrack, use iptables notrack target for the following traffic:
		 * - LAN-to-LAN traffic
		 * - LAN side local in/out traffic
		 */
		#ifdef CONFIG_NETFILTER_XT_TARGET_NOTRACK
		#ifdef NAT_LOOPBACK
		/* NAT-LOOPBACK needs local-in conntrack, so we add rawTrackNatLB chain
			in raw table to bypass LAN side local-in notrack target below. */
		va_cmd(IPTABLES, 4, 1, (char *)ARG_T, "raw", "-N", (char *)PF_RAW_TRACK_NAT_LB);
		va_cmd(IPTABLES, 6, 1, (char *)ARG_T, "raw", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)PF_RAW_TRACK_NAT_LB);
		va_cmd(IPTABLES, 4, 1, (char *)ARG_T, "raw", "-N", (char *)DMZ_RAW_TRACK_NAT_LB);
		va_cmd(IPTABLES, 6, 1, (char *)ARG_T, "raw", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)DMZ_RAW_TRACK_NAT_LB);
		#endif
		va_cmd(IPTABLES, 11, 1, (char *)ARG_T, "raw", (char *)FW_ADD, (char *)FW_PREROUTING, "-s", get_lan_ipnet(), "-d", get_lan_ipnet(), "-j", "CT", "--notrack");
		va_cmd(IPTABLES, 11, 1, (char *)ARG_T, "raw", (char *)FW_ADD, (char *)FW_PREROUTING, "-s", get_lan_ipnet(), "-d", "224.0.0.0/4", "-j", "CT", "--notrack");
		va_cmd(IPTABLES, 9, 1, (char *)ARG_T, "raw", (char *)FW_ADD, (char *)FW_OUTPUT, "-s", get_lan_ipnet(), "-j", "CT", "--notrack");
		#endif
		// get the minumum MRU for all WLAN-side link
		/* marked by Jenny
		vInt = get_min_wan_mru();
		if (vInt==0) */
			//vInt = 1500;
		//snprintf(value, 6, "%d", vInt);
		// set LAN-side MRU
		va_cmd(IFCONFIG, 2, 1, LANIF, "up");
		status|=va_cmd(IFCONFIG, 6, 1, (char*)LANIF, ipaddr, "netmask", subnet, "mtu", mtuStr);
	}

#ifdef CONFIG_SECONDARY_IP
	rtk_set_secondary_ip();
#endif


#if defined(CONFIG_SECONDARY_IP) && defined(WLAN_QTN)
	//  ifconfig LANIF LAN_IP netmask LAN_SUBNET
	if (mib_get_s(MIB_LAN_RGMII_IP, (void *)value, sizeof(value)) != 0)
	{
		strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
		ipaddr[15] = '\0';

		//snprintf(value, 6, "%d", vInt);
		// set LAN-side MRU
		status|=va_cmd(IFCONFIG, 6, 1, (char*)LAN_RGMII, ipaddr, "netmask", "255.255.255.0", "mtu", mtuStr);
	}
#endif

#ifdef CONFIG_USER_DHCP_SERVER
	if (mib_get_s(MIB_ADSL_LAN_AUTOSEARCH, (void *)value, sizeof(value)) != 0)
	{
		if (value[0] == 1)	// enable LAN ip autosearch
		{
			// check if dhcp server on ? per TR-068, I-190
			// Modified by Mason Yu for dhcpmode
			// if (mib_get_s(MIB_ADSL_LAN_DHCP, (void *)value, sizeof(value)) != 0)
			if (mib_get_s(MIB_DHCP_MODE, (void *)value, sizeof(value)) != 0)
			{
				if (value[0] != DHCPV4_LAN_SERVER)	// dhcp server is disabled
				{
					usleep(2000000); // wait 2 sec for br0 ready
					startLANAutoSearch(ipaddr, subnet);
				}
			}
		}
	}
#endif

#if defined(CONFIG_IPV6)
	if(deo_ipv6_status_get() != 0)
		setup_ipv6_br_interface(LANIF);
	else
		setup_disable_ipv6(LOCALHOST, 1);
#endif

#ifdef CONFIG_USER_WIRED_8021X
	rtk_wired_8021x_take_effect();
#endif

	snprintf(sysbuf, sizeof(sysbuf),"echo 0 > /proc/sys/net/bridge/bridge-nf-call-arptables");
	va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
	snprintf(sysbuf, sizeof(sysbuf),"echo 0 > /proc/sys/net/bridge/bridge-nf-call-iptables");
	va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
	snprintf(sysbuf, sizeof(sysbuf),"echo 0 > /proc/sys/net/bridge/bridge-nf-call-ip6tables");
	va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
	snprintf(sysbuf, sizeof(sysbuf),"echo 1 > /sys/class/net/%s/bridge/nf_call_arptables", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
	snprintf(sysbuf, sizeof(sysbuf),"echo 1 > /sys/class/net/%s/bridge/nf_call_iptables", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
	snprintf(sysbuf, sizeof(sysbuf),"echo 1 > /sys/class/net/%s/bridge/nf_call_ip6tables", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", sysbuf);

#ifdef _PRMT_X_CT_COM_ALARM_MONITOR_
	CWMP_CT_ALARM_T alarm;
	int alarm_idx = 0, flags = 0;
	int portSucc = 1;
    char failedLan[64] = {0};
    strcat(failedLan, "²»¿ÉÓÃ¶Ë¿ÚºÅ");

	for (i = 0; i < ELANVIF_NUM; i++)
	{
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
		if (rtk_port_is_wan_logic_port(i))
			continue;
#else
		portid = rtk_port_get_lan_phyID(i);
		if (portid != -1)
		{
			if (portid == ethPhyPortId)
				continue;
		}
#endif
		if (getInFlags(ELANRIF[i], &flags) == 1 && !(flags & IFF_UP))
		{
            strcat(failedLan, ELANRIF[i]);
            strcat(failedLan, ",");
			portSucc = 0;
		}
	}

	//fprintf(stderr, "check LAN port status after setup is complete: portSucc = %d\n", portSucc);
	if (portSucc == 0)
	{
#ifdef AUTO_SAVE_ALARM_CODE
        auto_save_alarm_code_add(CTCOM_ALARM_PORT_FAILED, failedLan);
#endif
		set_ctcom_alarm(CTCOM_ALARM_PORT_FAILED);
	}
	else
	{
		if (get_alarm_by_alarm_num(CTCOM_ALARM_PORT_FAILED, &alarm, &alarm_idx) == 0)
		{
			mib_chain_delete(CWMP_CT_ALARM_TBL, alarm_idx);
		}
	}
#endif

	if (status)  //start fail
	    return -1;

	//start success
    return 1;
}

// return value:
// 0  : not active
// 1  : successful
// -1 : startup failed
int setupService(void)
{
#ifdef REMOTE_ACCESS_CTL
	MIB_CE_ACC_T Entry;
#endif
	char *argv[15];
	int status=0;

#ifdef REMOTE_ACCESS_CTL
	if (!mib_chain_get(MIB_ACC_TBL, 0, (void *)&Entry))
		return 0;
#endif

	/* run as console web
	if (pEntry->web !=0) {
		// start webs
		va_cmd(WEBSERVER, 0, 0);
	}
	*/
	//if (pEntry->snmp !=0) {
		// start snmpd
		// Commented by Mason Yu
		// We use new version
		//va_cmd(SNMPD, 0, 0);
		// Add by Mason Yu for start SnmpV2Trap
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	char vChar;
	mib_get_s(MIB_SNMPD_ENABLE, (void *)&vChar, sizeof(vChar));
	if(vChar==1)
		status = startSnmp();
#endif
	//}
	return status;
}

//--------------------------------------------------------
// Daemon startup
// return value:
// 1  : successful
// -1 : failed

#define WAIT_UNTIL_READY

int startDaemon(void)
{
	int pppd_fifo_fd=-1;
	int mpoad_fifo_fd=-1;
	int status=0, tmp_status;
	int k;
#ifndef WAIT_UNTIL_READY
	int mpoa_retry=0;
	int ppp_retry=0;
	int retry_limit = 5;
#endif /* WAIT_UNTIL_READY */

//#ifdef CONFIG_USER_XDSL_SLAVE
//	if( startSlv()<0 )
//		status=-1;
//#endif /*CONFIG_USER_XDSL_SLAVE*/

	//#ifndef CONFIG_ETHWAN
	#ifdef CONFIG_DEV_xDSL
	// start mpoad
	status|=va_niced_cmd(MPOAD, 0, 0);

	// check if mpoad ready to serve
#ifdef WAIT_UNTIL_READY
	/* Wait until mpoad is ready */
	while ((mpoad_fifo_fd = open(MPOAD_FIFO, O_WRONLY)) == -1){
		;
	}
	close(mpoad_fifo_fd);
#else /*else WAIT_UNTIL_READY */

	/* Wait until timeout */
	tmp_status = status;
retry_mpoad:
	printf("[%s(%d)]status=%d, mpoa_retry=%d\n",__func__,__LINE__,status,mpoa_retry);
	for (k=0; k<=100; k++)
	{
		if ((mpoad_fifo_fd = open(MPOAD_FIFO, O_WRONLY))!=-1)
			break;
		usleep(30000);
	}

	if (mpoad_fifo_fd == -1)
	{
		mpoa_retry++;
		printf("open mpoad fifo failed !\n");
		status = -1;
		printf("[%s(%d)]status=%d, mpoa_retry=%d\n",__func__,__LINE__,status,mpoa_retry);
		if(mpoa_retry< retry_limit)
			goto retry_mpoad;
	}
	else{
		status = tmp_status;
		close(mpoad_fifo_fd);
	}
#endif /* endif WAIT_UNTIL_READY */
	#endif // CONFIG_DEV_xDSL

#ifdef CONFIG_PPP
#ifdef CONFIG_USER_SPPPD
	// start spppd
	status|=va_niced_cmd(SPPPD, 0, 0);

	// check if spppd ready to serve
#ifdef WAIT_UNTIL_READY
	/* Wait until spppd is ready */
	while ((pppd_fifo_fd = open(PPPD_FIFO, O_WRONLY)) == -1){
		;
	}
	close(pppd_fifo_fd);
#else	/* else WAIT_UNTIL_READY*/

	/* Wait until timeout */
	tmp_status = status;
retry_pppd:
	printf("[%s(%d)]status=%d, ppp_retry=%d\n",__func__,__LINE__,status,ppp_retry);
	for (k=0; k<=100; k++)
	{
		if ((pppd_fifo_fd = open(PPPD_FIFO, O_WRONLY))!=-1)
			break;
		usleep(30000);
	}

	if (pppd_fifo_fd == -1){
		ppp_retry++;
		status = -1;
		printf("[%s(%d)]status=%d, ppp_retry=%d\n",__func__,__LINE__,status,ppp_retry);
		if(ppp_retry < retry_limit)
			goto retry_pppd;
	}
	else{
		status = tmp_status;
		close(pppd_fifo_fd);
	}
#endif /* endif WAIT_UNTIL_READY */
#endif // CONFIG_USER_SPPPD
#ifdef CONFIG_USER_PPPD
	generate_pppd_core_script();
#endif // CONFIG_USER_PPPD
#endif

#ifdef CONFIG_SUPPORT_AUTO_DIAG
#define AUTOSIMU_DAEMON "/bin/autoSimu"

	status|=va_niced_cmd(AUTOSIMU_DAEMON, 0, 0);

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DevicePacketLossEnable_
	va_niced_cmd("/bin/autoPing", 0, 0);
#endif
#endif

#ifdef CONFIG_USER_WT_146
#define BFD_DAEMON "/bin/bfdmain"
#define BFD_SERVER_FIFO_NAME "/tmp/bfd_serv_fifo"
{
	int bfdmain_fifo_fd=-1;

	// start bfdmain
	status|=va_niced_cmd(BFD_DAEMON, 0, 0);

	// check if bfdmain ready to serve
	//while ((bfdmain_fifo_fd = open(BFD_SERVER_FIFO_NAME, O_WRONLY)) == -1)
	for (k=0; k<=100; k++)
	{
		if ((bfdmain_fifo_fd = open(BFD_SERVER_FIFO_NAME, O_WRONLY))!=-1)
			break;
		usleep(30000);
	}

	if (bfdmain_fifo_fd == -1)
		status = -1;
	else
		close(bfdmain_fifo_fd);
}
#endif //CONFIG_USER_WT_146

	// Kaohj -- move from startRest().

#ifdef CONFIG_PPPOE_MONITOR
	va_niced_cmd("/bin/ppp-monitor", 0, 0);
#endif

	return status;
}

int startDaemonLast(void)
{
	int status=0;
#ifdef CONFIG_USER_BOA_WITH_SSL
	int https_status=0;
#endif

#ifdef TIME_ZONE
#if 0//#ifdef CONFIG_E8B
	status|=setupNtp(SNTP_ENABLED);
#else
	status|=startNTP();
#endif
#endif

	status|=setupService();

	//run dhcpd or dhcpc on br0
	startDhcp();

	// start webs
	if(read_pid("/var/run/boa.pid") < 0)
	{
#ifdef CONFIG_USER_BOA_WITH_SSL
		if (mib_get(MIB_WEB_BOA_ENABLE_HTTPS, (void *)&https_status) != 0)
		{
			if(https_status == 1) /* only https port */
				status|=va_niced_cmd(WEBSERVER, 1, 0, "-n");
			else if(https_status == 2) /* only normal port*/
				status|=va_niced_cmd(WEBSERVER, 1, 0, "-s");
			else /* both normal and https port */
				status|=va_niced_cmd(WEBSERVER, 0, 0);
		}
		else
			status|=va_niced_cmd(WEBSERVER, 0, 0);
#else
		status|=va_niced_cmd(WEBSERVER, 0, 0);
#endif
	}else{
		#ifdef RTK_WEB_HTTP_SERVER_BIND_DEV
		va_cmd("/bin/touch",1,0,RTK_TRIGGLE_BOA_REBIND_SOCK_FILE);
		#endif
	}

#ifdef CONFIG_USER_LANNETINFO
	restartlanNetInfo();
#else
	status|=va_niced_cmd(ARP_MONITOR, 10, 1, "-F", "-d", "-t", "3", "-nf", "3", "-nt", "7", "-i", "br+");
#endif
#if defined(CONFIG_USER_P0F)
	restart_p0f();
#endif

#ifdef CONFIG_USER_MDNS
	restartMdns();
#endif

	return 0;
}

#ifdef ELAN_LINK_MODE_INTRENAL_PHY
// Added by Mason Yu
int setupLinkMode_internalPHY()
{
	restart_ethernet(1);
	return 1;

}
#endif

//--------------------------------------------------------
// LAN side IP autosearch using ARP
// Input: current IP address
// return value:
// 1  : successful
// -1 : failed
int startLANAutoSearch(const char *ipAddr, const char *subnet)
{
	unsigned char netip[4];
	struct in_addr *dst;
	int k, found;

	TRACE(STA_INFO, "Start LAN IP autosearch\n");
	dst = (struct in_addr *)netip;

	if (!inet_aton(ipAddr, dst)) {
		printf("invalid or unknown target %s", ipAddr);
		return -1;
	}

	if (isDuplicate(dst, LANIF)) {
		TRACE(STA_INFO, "Duplicate LAN IP found !\n");
		found = 0;
		inet_aton("192.168.1.254", dst);
		if (isDuplicate(dst, LANIF)) {
			netip[3] = 63;	// 192.168.1.63
			if (isDuplicate(dst, LANIF)) {
				// start from 192.168.1.253 and descending
				for (k=253; k>=1; k--) {
					netip[3] = k;
					if (!isDuplicate(dst, LANIF)) {
						// found it
						found = 1;
						TRACE(STA_INFO, "Change LAN ip to %s\n", inet_ntoa(*dst));
						break;
					}
				}
			}
			else {
				// found 192.168.1.63
				found = 1;
				TRACE(STA_INFO, "Change LAN ip to %s\n", inet_ntoa(*dst));
			}
		}
		else {
			// found 192.168.1.254
			found = 1;
			TRACE(STA_INFO, "Change LAN ip to %s\n", inet_ntoa(*dst));
		}

		if (!found) {
			printf("not available LAN IP !\n");
			return -1;
		}

		// ifconfig LANIF LAN_IP netmask LAN_SUBNET
		va_cmd(IFCONFIG, 4, 1, (char*)LANIF, inet_ntoa(*dst), "netmask", subnet);
	}

	return 1;
}

#if defined(CONFIG_RTL_IGMP_SNOOPING)
int setupIGMPSnoop()
{
	unsigned char mode;
	mib_get(MIB_MPMODE, (void *)&mode);
	if (mode&MP_IGMP_MASK){
		__dev_setupIGMPSnoop(1);
	} else {
		__dev_setupIGMPSnoop(0);
	}
	return 1;
}
#endif
#if defined(CONFIG_RTL_MLD_SNOOPING)
int setupMLDSnoop()
{
	unsigned char mode;
	mib_get(MIB_MPMODE, (void *)&mode);
	if (mode&MP_MLD_MASK){
		__dev_setupMLDSnoop(1);
	} else {
		__dev_setupMLDSnoop(0);
	}
	return 1;
}
#endif

//--------------------------------------------------------
// Final startup
// return value:
// 1  : successful
// -1 : failed
int startRest(void)
{
	int vcTotal, i;
	unsigned char autosearch, mode;
	MIB_CE_ATM_VC_T Entry;
	unsigned char tr247_unmatched_veip_cfg = 0;
	char sysbuf[128];

#ifdef CONFIG_IPV6
	char ipv6Enable =-1;
#endif

	mib_get_s(MIB_TR247_UNMATCHED_VEIP_CFG, (void *)&tr247_unmatched_veip_cfg, sizeof(tr247_unmatched_veip_cfg));
	// Kaohj -- move to startDaemon().
	//	When ppp up, it will send command to boa message queue,
	//	so we needs boa msgq to be enabled earlier.
/*
	// start snmpd
	va_cmd(SNMPD, 0, 0);
*/

	// Add static routes
	// Mason Yu. Init hash table for all routes on RIP
	// Move to startWan()
	//addStaticRoute();



#if defined(CONFIG_RTL_IGMP_SNOOPING) || defined(CONFIG_RTL_MLD_SNOOPING) || defined(CONFIG_BRIDGE_IGMP_SNOOPING)
	rtk_multicast_snooping();
#endif

#if defined(CONFIG_USER_PORT_ISOLATION)
	checkPortIsolationEntry();
#endif

#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	rtk_del_manual_firewall();
	rtk_init_manual_firewall();
	set_manual_load_balance_rules();
#endif

#ifdef CONFIG_USER_IP_QOS
#ifdef CONFIG_USER_AP_QOS
	take_qos_effect();
#else
	setupIPQ();
#endif
#ifdef CONFIG_HWNAT
extern int setWanIF1PMark(void);
	setWanIF1PMark();
#endif
#endif

	// ioctl for direct bridge mode, jiunming
	{
		unsigned char  drtbr_mode;
		if (mib_get_s(MIB_DIRECT_BRIDGE_MODE, (void *)&drtbr_mode, sizeof(drtbr_mode)) != 0)
		{
			__dev_setupDirectBridge( (int) drtbr_mode );
		}
	}

#ifdef DOS_SUPPORT
	// for DOS support
	setup_dos_protection();
#endif

     #ifdef CONFIG_IGMP_FORBID

       unsigned char igmpforbid_mode;

	 if (!mib_get_s( MIB_IGMP_FORBID_ENABLE,  (void *)&igmpforbid_mode, sizeof(igmpforbid_mode))){
		printf("igmp forbid  parameter failed!\n");
	}
	 if(1==igmpforbid_mode){
             __dev_igmp_forbid(1);
	 }
     #endif

#ifdef CONFIG_USER_SAMBA
	if(!tr247_unmatched_veip_cfg)
		startSamba();
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	smartHGU_Samba_Initialize();
#endif // CONFIG_YUEME
#endif // CONFIG_USER_SAMBA

	// E8B forceportal
#ifdef _PRMT_X_CT_COM_PORTALMNT_
	setPortalMNT();
#endif

//for MP mode
#if defined(WLAN_SUPPORT) && (defined(CONFIG_USER_WIRELESS_MP_MODE) || !defined(CONFIG_RTK_DEV_AP))
	unsigned char mp_mode = 0;
	mib_get_s(MIB_WIFI_TEST, (void *)&mp_mode, sizeof(mp_mode));
	if (mp_mode == 1) {
		va_niced_cmd("/bin/sh", 1, 0, "/etc/scripts/mp_start.sh");
	}
#endif

#ifdef CONFIG_USER_Y1731
	Y1731_start(0);
#endif
#ifdef CONFIG_USER_8023AH
	EFM_8023AH_start(0);
#endif

#if defined(CONFIG_USER_RTK_LBD)
	setupLBD();
#endif
#ifdef CONFIG_USER_MINIUPNPD
	restart_upnp(CONFIGALL, 0, 0, 0);
#endif

#ifdef COM_CUC_IGD1_WMMConfiguration
	setup_wmmservice_rules();
	setup_WmmRule_startup();
#endif

#if defined(WIFI_TIMER_SCHEDULE) || defined(LED_TIMER) || defined(SLEEP_TIMER) || defined(SUPPORT_TIME_SCHEDULE) || defined(BB_FEATURE_SAVE_LOG)
#ifdef CONFIG_RTL8367_SUPPORT
	/* TODO: updateScheduleCrondFile will cause MDC/MDIO access Fail (unknown reason) */
        printf("%s(%d): Not execute updateScheduleCrondFile", __FUNCTION__, __LINE__);
#else
	updateScheduleCrondFile("/var/spool/cron/crontabs", 1);
#endif
#endif

	if(tr247_unmatched_veip_cfg)
	{
		for (i = 1; i < ELANVIF_NUM; i++)
		{
			snprintf(sysbuf, sizeof(sysbuf), "echo 0 > /sys/class/net/%s/brport/unicast_flood", ELANVIF[i]);
			va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
		}
#ifdef WLAN_SUPPORT
		for (i=0; i<NUM_WLAN_INTERFACE; i++)
		{
			snprintf(sysbuf, sizeof(sysbuf), "echo 0 > /sys/class/net/%s%d/brport/unicast_flood", ALIASNAME_WLAN,i);
			va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
		}
#endif
	}


	return 1;
}

#ifdef CONFIG_E8B
static int start_e8_rest(void)
{
#ifdef _PRMT_X_CT_COM_PING_
#ifdef _PRMT_SC_CT_COM_Ping_Enable_
	unsigned int function_mask = 0;
	mib_get_s(PROVINCE_SICHUAN_FUNCTION_MASK, &function_mask, sizeof(function_mask));
	if ((function_mask & PROVINCE_SICHUAN_PING_ENABLE) != 0)
	{
		unsigned char vChar = 0;
		vChar = 0;
		mib_set(CWMP_CT_PING_ENABLE, &vChar);
		rtk_cwmp_set_sc_ct_ping_enable(0);
	}
#endif
#endif
	return 0;
}
#endif

#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
// Added by Mason Yu
static int getSnmpConfig(void)
{

	//char *str1, *str2, *str3, *str4, *str5;
	unsigned char str[256];
	FILE *fp;

	fp=fopen("/tmp/snmp", "w+");


	mib_get_s(MIB_SNMP_SYS_DESCR, (void *)str, sizeof(str));
	fprintf(fp, "%s\n", str);


	mib_get_s( MIB_SNMP_SYS_CONTACT,  (void *)str, sizeof(str));
	fprintf(fp, "%s\n", str);


	mib_get_s( MIB_SNMP_SYS_NAME,  (void *)str, sizeof(str));
	fprintf(fp, "%s\n", str);


	mib_get_s( MIB_SNMP_SYS_LOCATION,  (void *)str, sizeof(str));
	fprintf(fp, "%s\n", str);


	mib_get_s( MIB_SNMP_SYS_OID,  (void *)str, sizeof(str));
	fprintf(fp, "%s\n", str);


  	fclose(fp);
	return 0;
}
#endif

static int check_wan_mac()
{
	//sync wan mac address from ELAN_MAC_ADDR
	int ret=0;
	int i, vcTotal;
	MIB_CE_ATM_VC_T Entry;
	char macaddr[MAC_ADDR_LEN], gen_macaddr[MAC_ADDR_LEN];

	mib_get_s(MIB_ELAN_MAC_ADDR, (void *)macaddr, sizeof(macaddr));

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);

	for (i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				return -1;

		if(MEDIA_INDEX(Entry.ifIndex) == MEDIA_ETH
#ifdef CONFIG_DEV_xDSL
			|| MEDIA_INDEX(Entry.ifIndex) == MEDIA_ATM
			|| MEDIA_INDEX(Entry.ifIndex) == MEDIA_PTM
#endif
			){

			memcpy(gen_macaddr, macaddr, MAC_ADDR_LEN);
#if defined(CONFIG_LUNA) && defined(GEN_WAN_MAC)
			setup_mac_addr(gen_macaddr,WAN_HW_ETHER_START_BASE + ETH_INDEX(Entry.ifIndex));
#else
			setup_mac_addr(gen_macaddr,1);
#endif
			//gen_macaddr[MAC_ADDR_LEN-1]+= (WAN_HW_ETHER_START_BASE + ETH_INDEX(Entry.ifIndex));
			if(memcmp(gen_macaddr, Entry.MacAddr, MAC_ADDR_LEN)){
				memcpy(Entry.MacAddr, gen_macaddr, MAC_ADDR_LEN);
				mib_chain_update(MIB_ATM_VC_TBL, (void *)&Entry, i);
				ret++;
			}
		}
	}
	if(!ret)
		return 0;

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
	return 0;
}

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
static void syncLOID()
{
#ifdef _PRMT_X_CT_COM_USERINFO_
        unsigned char loid[MAX_NAME_LEN];
        unsigned char password[MAX_LOID_PW_LEN] = {0};
        unsigned char old_loid[MAX_NAME_LEN];
        unsigned char old_password[MAX_LOID_PW_LEN]= {0};
        int changed = 0;
        unsigned int pon_mode;
        unsigned char reg_type;
        int entryNum =4;
        int index = 0;
        char oamcli_cmd[128]={0};
#if defined(CONFIG_GPON_FEATURE)
		unsigned char password_hex[MAX_LOID_PW_LEN]={0};
		unsigned char old_password_hex[MAX_LOID_PW_LEN]={0};
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
		unsigned char pon_reg_mode=0;
#endif
#if defined(CONFIG_GPON_FEATURE)
		unsigned int gpon_speed=0;
		int ploam_pw_length=GPON_PLOAM_PASSWORD_LENGTH;

		mib_get_s(MIB_PON_SPEED, (void *)&gpon_speed, sizeof(gpon_speed));
		if(gpon_speed==0){
			ploam_pw_length=GPON_PLOAM_PASSWORD_LENGTH;
		}
		else{
			ploam_pw_length=NGPON_PLOAM_PASSWORD_LENGTH;
		}
#endif

        mib_get_s(PROVINCE_DEV_REG_TYPE, &reg_type, sizeof(reg_type));
        mib_get_s(MIB_LOID, loid, sizeof(loid));
        mib_get_s(MIB_LOID_OLD, old_loid, sizeof(old_loid));
        if(strcmp(loid, old_loid) != 0)
                changed = 1;

        mib_get_s(MIB_LOID_PASSWD, password, sizeof(password));
        mib_get_s(MIB_LOID_PASSWD_OLD, old_password, sizeof(old_password));
        if(strcmp(password, old_password) != 0)
                changed = 1;

#if defined(CONFIG_GPON_FEATURE)
		formatPloamPasswordToHex(password, password_hex, ploam_pw_length);
		formatPloamPasswordToHex(old_password, old_password_hex, ploam_pw_length);
#endif
        mib_get_s(MIB_PON_MODE, (void *)&pon_mode, sizeof(pon_mode));
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
		mib_get_s(MIB_PON_REG_MODE, (void *) &pon_reg_mode, sizeof(pon_reg_mode));
		if(pon_reg_mode==1 || pon_reg_mode==2)
		{
			if(reg_type != DEV_REG_TYPE_DEFAULT && strlen(loid)==0 )
				return;
			if(reg_type == DEV_REG_TYPE_DEFAULT && strlen(old_loid)==0 )
				return;
		}
#endif

        if(reg_type != DEV_REG_TYPE_DEFAULT)
        {
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
		if (pon_mode == EPON_MODE)
		{
			index = 0;
			{
				va_cmd("/bin/oamcli", 6, 1, "set", "ctc", "loid", "0", loid, password); /* "0" means index = 0 */
				va_cmd("/bin/oamcli", 3, 1, "trigger", "register", "0");
			}
		}
#ifdef CONFIG_RTK_OMCI_V1
		else if(pon_mode == GPON_MODE)
		{
			if(strlen(loid))
			{
				va_cmd("/bin/omcicli", 4, 1, "set", "loid", loid, password);
			}
			PON_OMCI_CMD_T msg;
			memset(&msg, 0, sizeof(msg));
			msg.cmd = PON_OMCI_CMD_LOIDAUTH_GET_RSP;
			if(omci_SendCmdAndGet(&msg) == GOS_OK)
				printf("OMCI APP LoID: %s\n", msg.value);
			if(strcmp(msg.value, loid) != 0)
			{
				va_cmd("/bin/omcicli", 4, 1, "set", "loid", loid, password);
				system("/bin/diag gpon deactivate");
				system("/bin/diag gpon activate init-state o1");
			}
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
			system("/bin/diag gpon deactivate");
			memset(oamcli_cmd, 0, 128);
			sprintf(oamcli_cmd , "/sbin/diag gpon set password-hex %s", password_hex);
			system(oamcli_cmd);
			system("/bin/diag gpon activate init-state o1");
#endif
		}
		#endif
#endif
                return;
        }

        if(changed)
        {
                mib_set(MIB_LOID, old_loid);
                mib_set(MIB_LOID_PASSWD, old_password);
#ifdef COMMIT_IMMEDIATELY
                Commit();
#endif
                if (mib_get_s(MIB_PON_MODE, (void *)&pon_mode, sizeof(pon_mode)) != 0)
                {
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
                        if (pon_mode == EPON_MODE)
                        {
				index = 0;
				{
					if(reg_type == DEV_REG_TYPE_DEFAULT)
						va_cmd("/bin/oamcli", 6, 1, "set", "ctc", "loid", "0", old_loid, old_password); /* "0" means index = 0 */
					else
						va_cmd("/bin/oamcli", 6, 1, "set", "ctc", "loid", "0", loid, password);
					va_cmd("/bin/oamcli", 3, 1, "trigger", "register", "0");
				}
                        }
#ifdef CONFIG_RTK_OMCI_V1
                        else if(pon_mode == GPON_MODE)
                        {
                                if(reg_type == DEV_REG_TYPE_DEFAULT)
                                {
                                	if(strlen(old_loid))
					{
							va_cmd("/bin/omcicli", 4, 1, "set", "loid", old_loid, old_password);
					}

				}
				else
				{
					if(strlen(loid))
					{
						va_cmd("/bin/omcicli", 4, 1, "set", "loid", loid, password);
					}
				}
				PON_OMCI_CMD_T msg;
				memset(&msg, 0, sizeof(msg));
				msg.cmd = PON_OMCI_CMD_LOIDAUTH_GET_RSP;
				if(omci_SendCmdAndGet(&msg) == GOS_OK)
		        		printf("OMCI APP LoID: %s\n", msg.value);
				if(strcmp(msg.value, loid) != 0)
				{
					if(reg_type == DEV_REG_TYPE_DEFAULT && old_loid[0] != '\0')
						va_cmd("/bin/omcicli", 4, 1, "set", "loid", old_loid, old_password);
					else
						va_cmd("/bin/omcicli", 4, 1, "set", "loid", loid, password);
					system("/bin/diag gpon deactivate");
					system("/bin/diag gpon activate init-state o1");
				}
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
				system("/bin/diag gpon deactivate");
				memset(oamcli_cmd, 0, 128);
				if(reg_type == DEV_REG_TYPE_DEFAULT)
					snprintf(oamcli_cmd , sizeof(oamcli_cmd), "/sbin/diag gpon set password-hex %s", old_password_hex);
				else
					snprintf(oamcli_cmd , sizeof(oamcli_cmd), "/sbin/diag gpon set password-hex %s", password_hex);
				system("/bin/diag gpon activate init-state o1");
#endif
                        }
			#endif
#endif
                }
        }
#endif
}
#endif

static void check_user_password(void)
{
	unsigned char userPass[MAX_PASSWD_LEN], default_userPass[MAX_PASSWD_LEN];
	unsigned char changed = 0;

	mib_get_s(MIB_USER_PASSWORD,(void *)userPass, sizeof(userPass) );
	if(!strcmp(userPass, "0") || !strcmp(userPass,"useradmin")){
		mib_get_s(MIB_DEFAULT_USER_PASSWORD, (void *)default_userPass, sizeof(default_userPass));
		mib_set(MIB_USER_PASSWORD, (void *)default_userPass);
		changed = 1;
	}

#ifdef ACCOUNT_CONFIG
	MIB_CE_ACCOUNT_CONFIG_T Entry;
	char susName[MAX_NAME_LEN] = {0};
	int totalEntry, i;
		totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL);
		for (i=0; i<totalEntry; i++) {
			if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&Entry)) {
				continue;
			}
			if(!strcmp(Entry.userName, "useradmin")){
				if(!strcmp(Entry.userPassword, "0")){
					mib_get(MIB_DEFAULT_USER_PASSWORD, (void *)default_userPass);
					strcpy(Entry.userPassword, default_userPass);
				}
			}
			mib_chain_update(MIB_ACCOUNT_CONFIG_TBL, (void *)&Entry, i);
			changed = 1;
		}
#endif


#ifdef TELNET_ACCOUNT_INDEPENDENT
	mib_get_s(MIB_TELNET_USER, userPass, sizeof(userPass));
	if(userPass[0] == '\0')
	{
		mib_get_s(MIB_HW_TELNET_USER, userPass, sizeof(userPass));
		mib_set(MIB_TELNET_USER, userPass);
		mib_get_s(MIB_HW_TELNET_PASSWD, userPass, sizeof(userPass));
		mib_set(MIB_TELNET_PASSWD, userPass);
		changed = 1;
	}
#endif

	if(changed)
		Commit();
}

static void check_suser_password(void)
{
#ifdef CONFIG_TELMEX_DEV
	unsigned char default_suserPass[MAX_PSK_LEN+1]={0};
	char suserPass[MAX_PASSWD_LEN];
	unsigned char passwdMod = 0;
	int changed = 0;

	mib_get_s(MIB_SUSER_PASSWORD,(void *)suserPass, sizeof(suserPass));
	if(!strcmp(suserPass, "0"))
	{
		mib_get_s(MIB_DEFAULT_WLAN_WPAKEY, (void *)default_suserPass, sizeof(default_suserPass));
		if(default_suserPass[0]== 0)
			snprintf(default_suserPass, sizeof(default_suserPass), "password");
		mib_set(MIB_SUSER_PASSWORD, (void *)default_suserPass);
		changed = 1;
	}
	if (changed)
		Commit();
#else
	unsigned char suserPass[MAX_PASSWD_LEN], default_suserPass[MAX_PASSWD_LEN];

	mib_get_s(MIB_SUSER_PASSWORD,(void *)suserPass, sizeof(suserPass));
	if(!strcmp(suserPass, "0"))
	{
		mib_get(MIB_DEFAULT_SUSER_PASSWORD, (void *)default_suserPass);
		if(default_suserPass[0])	// default password is configured
			mib_set(MIB_SUSER_PASSWORD, (void *)default_suserPass);
		else
			mib_set(MIB_SUSER_PASSWORD, "system");
	}
#endif
}

#ifdef CONFIG_USER_RTK_RECOVER_SETTING
char xml_line[1024];
mib_table_entry_T *mib_info;
int info_total;
static int get_line(char *s, int size, FILE *fp)
{
	char *pstr;

	while (1) {
		if (!fgets(s,size,fp))
			return -1;
		pstr = trim_white_space(s);
		if (strlen(pstr))
			break;
	}

	//printf("get line: %s\n", s);
	return 0;
}

/*
 *	0: not consist
 *	1: consist with system mib
 *	-1: error
 */
static int xml_table_check(char *line, CONFIG_DATA_T cnf_type)
{
	int i;
	char *ptoken;
	char *pname, *pvalue;
	const char empty_str[]="";
	mib_table_entry_T info_entry;
	unsigned char mibvalue[1024], svalue[2048];

	// get name
	ptoken = strtok(line, "\"");
	ptoken = strtok(0, "\"");
	pname = ptoken;
	//printf("name=%s\n", ptoken);

	for(i=0; i<info_total; i++){
		if (((mib_table_entry_T*)(mib_info+i))->mib_type != cnf_type)
			continue;
		if(!strcmp(((mib_table_entry_T*)(mib_info+i))->name, ptoken)){
			memcpy(&info_entry,(mib_table_entry_T*)(mib_info+i),sizeof(mib_table_entry_T));
			break;
		}
	}

	if(i>=info_total) {
		printf("%s: Invalid table entry name: %s\n", __FUNCTION__, ptoken);
		return -1;
	}

	// get value
	ptoken = strtok(0, "\"");
	ptoken = strtok(0, "\"");
	if (strtok(0, "\"")==NULL)
		ptoken = (char *)empty_str;
	pvalue = ptoken;
	//printf("xml value=%s\n", ptoken);
	mib_get_s(info_entry.id, (void *)mibvalue, sizeof(mibvalue));

	mib_to_string(svalue, mibvalue, info_entry.type, info_entry.size);
	//printf("sys value=%s\n", svalue);
	if (!strncmp(pvalue, svalue, 512))
		return 1;
	else {
		printf("name=%s	value= [%s(xml), %s(sys)]\n", pname, pvalue, svalue);
		return 0;
	}
	return 0;
}

/*
 *	0: not consist
 *	1: consist with system mib
 *	-1: error
 */
static int xml_chain_check_real(FILE *fp)
{
	char *pstr, *ptoken;
	const char empty_str[]="";
	char *pname, *pvalue;

	while(!feof(fp)) {
		get_line(xml_line, sizeof(xml_line), fp);
		// remove leading space
		pstr = trim_white_space(xml_line);
		if (!strncmp(pstr, "</chain", 7)) {
			break; // end of chain object
		}
		// check OBJECT_T
		if (!strncmp(pstr, "<chain", 6)) {
			// get Object name
			ptoken = strtok(pstr, "\"");
			ptoken = strtok(0, "\"");
			//printf("obj_name=%s\n", ptoken);
			xml_chain_check_real(fp);
		}
		else {
			// get name
			ptoken = strtok(pstr, "\"");
			ptoken = strtok(0, "\"");
			pname = ptoken;
			//printf("name=%s\t\t", ptoken);

			// get value
			ptoken = strtok(0, "\"");
			ptoken = strtok(0, "\"");
			if (strtok(0, "\"")==NULL)
				ptoken = (char *)empty_str;
			pvalue = ptoken;
			//printf("value=%s\n", ptoken);
		}
	}
	return 1;
}

/*
 *	0: not consist
 *	1: consist with system mib
 *	-1: error
 */
static int xml_chain_check(char *line, FILE *fp)
{
	char *ptoken;

	// get chain name
	ptoken = strtok(line, "\"");
	ptoken = strtok(0, "\"");
	//printf("Chain name=%s\n", ptoken);
	xml_chain_check_real(fp);
	return 1;
}

/*
 *	0: not consist
 *	1: consist with system mib
 *	-1: error
 */
static int check_xml_value(char *line, CONFIG_DATA_T cnf_type, FILE *fp)
{
	int i, k;
	char str[32];
	int ret=0;

	// remove leading space
	i = 0; k = 0;
	while (line[i++]==' ')
		k++;
	sscanf(line, "%s", str);
	//printf("str=%s\n", str);
	if (!strcmp(str, "<Value")) {
		ret = xml_table_check(&line[k], cnf_type);
	}
	else if (!strcmp(str, "<chain")) {
		ret = xml_chain_check(&line[k], fp);
	}
	else {
		printf("Unknown statement: %s\n", line);
		ret = -1;
	}

	return ret;
}

/*	check consistency between xml file and system mib
 *
 *	0: not consistent
 *	1: consistent with system mib
 *	-1: error
 */
int check_xml_file(char *fname, CONFIG_DATA_T dtype)
{
	FILE *fp;
	int i, ret=0;
	char *pstr;
	CONFIG_DATA_T ftype;
#ifdef XOR_ENCRYPT
	char tempFile[32];
#endif

#ifdef XOR_ENCRYPT
	xor_encrypt(fname, tempFile);
	rename(tempFile, fname);
#endif

	if (!(fp = fopen(fname, "r"))) {
		return -1;
	}
	get_line(xml_line, sizeof(xml_line), fp);
	pstr = trim_white_space(xml_line);
	// Check for configuration type (cs or hs?).
	if(!strcmp(pstr, CONFIG_HEADER))
		ftype = CURRENT_SETTING;
	else if(!strcmp(pstr, CONFIG_HEADER_HS))
		ftype = HW_SETTING;
	else {
		printf("%s: Invalid config file(%s)!\n", __FUNCTION__, fname);
		fclose(fp);
		return -1;
	}
	if (ftype != dtype) {
		printf("%s: %s not in correct type.\n", __FUNCTION__, fname);
		fclose(fp);
		return -1;
	}

	info_total = mib_info_total();
	mib_info=(mib_table_entry_T *)malloc(sizeof(mib_table_entry_T)*info_total);

	for(i=0;i<info_total;i++){
		if(!mib_info_index(i,mib_info+i))
			break;
	}

	if(i<info_total){
		free(mib_info);
		fclose(fp);
		printf("%s: get mib info total entry error!\n", __FUNCTION__);
		return -1;
	}

	while(!feof(fp)) {
		get_line(xml_line, sizeof(xml_line), fp);//get one line from the file
		pstr = trim_white_space(xml_line);
		if(!strcmp(pstr, CONFIG_TRAILER) || !strcmp(pstr, CONFIG_TRAILER_HS))
			break; // end of configuration

		if ((ret=check_xml_value(pstr, dtype, fp)) != 1)
			break;
	}

	free(mib_info);
	fclose(fp);
	return ret;
}

/* Return backup xml file if not consistent with system mib */
unsigned int check_xml_backup()
{
	int iVal;
	unsigned int ret=0;
	char cmd_str[128];

	// check hs
	#ifndef FORCE_HS
	printf("Check HS XML backup ...\n");
	snprintf(cmd_str, 128, "/bin/gunzip -c %s > %s", OLD_SETTING_FILE_HS_GZ, TEMP_XML_FILE_HS);
	//printf("cmd: %s\n", cmd_str);
	system(cmd_str);
	iVal = check_xml_file(TEMP_XML_FILE_HS, HW_SETTING);
	unlink(TEMP_XML_FILE_HS);
	if (iVal != 1)
		ret |= HW_SETTING;
	#else // FORCE_HS
	printf("Skip checking HS XML backup ...\n");
	#endif

	// check cs
	printf("Check CS XML backup ...\n");
	snprintf(cmd_str, 128, "/bin/gunzip -c %s > %s", OLD_SETTING_FILE_GZ, TEMP_XML_FILE);
	//printf("cmd: %s\n", cmd_str);
	system(cmd_str);
	iVal = check_xml_file(TEMP_XML_FILE, CURRENT_SETTING);
	unlink(TEMP_XML_FILE);
	if (iVal != 1)
		ret |= CURRENT_SETTING;
	return ret;
}
#endif // of CONFIG_USER_RTK_RECOVER_SETTING

#ifdef CONFIG_TRUE
void checkDefaultPppAccount(void)
{
	MIB_CE_ATM_VC_T Entry;
	int total_entry=0, i;

	total_entry = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0; i<total_entry; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if (Entry.cmode != CHANNEL_MODE_PPPOE)
			continue;

		if ('\0' == Entry.pppUsername[0])
		{
			unsigned char macAddr[MAC_ADDR_LEN];

			mib_get(MIB_ELAN_MAC_ADDR, macAddr);
			snprintf(Entry.pppUsername, MAX_NAME_LEN, "%02x%02x%02x%02x%02x%02x@truehisp",
					macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
			strcpy(Entry.pppPassword, "password");
			mib_chain_update(MIB_ATM_VC_TBL,(void *)&Entry, i);
		}
	}
}

void checkDefaultUserPassword(void)
{
	char userPass[MAX_NAME_LEN]="\0";

	mib_get(MIB_USER_PASSWORD, (void *)userPass);
	if ('\0' == userPass[0])
	{
		mib_get(MIB_HW_USER_PASSWORD, (void *)userPass);
		mib_set(MIB_USER_PASSWORD, (void *)userPass);
	}
}
#endif

/*
 *	Disable PVCs before doing auto-pvc search.
 */
static void mibchain_clearPVC()
{
	unsigned int entryNum;
	int i;
	MIB_CE_ATM_VC_T tEntry;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);

	// clear atm pvc
	for (i=entryNum-1; i>=0; i--) {
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
			continue;

		if(MEDIA_INDEX(tEntry.ifIndex) != MEDIA_ATM)
			continue;
		mib_chain_delete(MIB_ATM_VC_TBL, i);
	}
	return;
}

/*
 * system initialization, checking, setup, etc.
 */
static int sys_setup()
{
	key_t key;
	int qid, vInt, activePVC, ret;
	int i;
	MIB_CE_ATM_VC_T Entry;
	unsigned char value[32];
	FILE *fp;
	char userName[MAX_NAME_LEN], userPass[MAX_NAME_LEN];
	char *xpass;
#ifdef ACCOUNT_CONFIG
	MIB_CE_ACCOUNT_CONFIG_T entry;
	unsigned int totalEntry;
#endif
#ifdef CONFIG_USER_RTK_RECOVER_SETTING
	unsigned int dtype;
#endif
	unsigned char autosearch;
	char cmd[512];
	char devName[MAX_DEV_NAME_LEN]={0};

	ret = 0;
	//----------------- check if configd is ready -----------------------
	key = ftok("/bin/init", 'r');
	for (i=0; i<=100; i++) {
		if (i==100) {
			printf("\033[0;31mError: configd not started !!\033[0m\n");
			return 0;
		}
		if ((qid = msgget( key, 0660 )) == -1)
			usleep(30000);
		else
			break;
	}

	// Kaohj -- check consistency between MIB chain definition and descriptor.
	// startup process would be ceased if checking failed, programmer must review
	// all MIB chain descriptors in problem.
	if (mib_check_desc()==-1) {
		printf("Please check MIB chain descriptors !!\n");
		return -1;
	}
#ifdef CONFIG_USER_RTK_RECOVER_SETTING
	// Delay before writing to flash for system stability
	sleep(1);
	fp = fopen(FLASH_CHECK_FAIL, "r"); //only when current setting check is fail, sys restore to oldconfig
	if (fp) {
		fscanf(fp, "%d\n", &dtype);
		fclose(fp);
		unlink(FLASH_CHECK_FAIL);
		//printf("dtype=%d\n", dtype);
		if (dtype & CURRENT_SETTING) {
			// gzip: decompress file
			va_cmd("/bin/gunzip", 2, 1, "-f", OLD_SETTING_FILE_GZ);
			va_cmd("/bin/loadconfig", 2, 1, "-f", OLD_SETTING_FILE);
		}
		if (dtype & HW_SETTING) {
			// gzip: decompress file
			va_cmd("/bin/gunzip", 2, 1, "-f", OLD_SETTING_FILE_HS_GZ);
			va_cmd("/bin/loadconfig", 3, 1, "-f", OLD_SETTING_FILE_HS, "hs");
		}
	}

	if ((dtype=check_xml_backup()) != 0) {
		if (dtype & HW_SETTING) { //update hs setting in backup file
			printf("%s: hs xml not consistent, generate a new one.\n", __FUNCTION__);
			// generate a new backup xml
			va_cmd("/bin/saveconfig", 2, 1, "-s", "hs");
		}

		if (dtype & CURRENT_SETTING) { //update cs setting in backup file
			printf("%s: cs xml not consistent, generate a new one.\n", __FUNCTION__);
			// generate a new backup xml
			va_cmd("/bin/saveconfig", 2, 1, "-s", "cs");
		}
	}
	else
		printf("%s: xml check ok.\n", __FUNCTION__);
#endif

#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	if (rtk_check_port_remapping() == RTK_FAILED) {
		printf("Please check value of MIB_PORT_REMAPPING!!!\n");
		return -1;
	}
#endif

#if 0
	usbRestore();
#endif

#ifdef CONFIG_TRUE
	checkDefaultUserPassword();
#endif

#ifdef _PRMT_X_CMCC_LANINTERFACES_
	check_l2filter();
	check_elan_conf();
#endif

	// Clear atm pvcs before doing auto-pvc.
	autosearch = 0;
	mib_get_s(MIB_ATM_VC_AUTOSEARCH, (void *)&autosearch, sizeof(autosearch));
	if (autosearch == 1)
		mibchain_clearPVC();

	//----------------
	// Mason Yu
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	getSnmpConfig();
#endif
	check_user_password();
	check_suser_password();
	// ftpd: /etc/passwd & /tmp (as home dir)
	rtk_util_update_user_account();
	// Kaohj --- force kernel(linux-2.6) igmp version to 2
#ifdef FORCE_IGMP_V2
	fp = fopen((const char *)PROC_FORCE_IGMP_VERSION,"w");
	if(fp)
	{
		fprintf(fp, "%d", 2);
		fclose(fp);
	}
#endif

#ifdef CONFIG_MULTI_FTPD_ACCOUNT
	ftpd_account_change();
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
	smbd_account_change();
#endif
	smartHGU_ftpserver_init_api();
#endif

#ifdef CONFIG_CU
#ifdef _PRMT_C_CU_FTPSERVICE_
	/*
		For ChinaUnicom:
		Resync "/var/ftp_userList" with CWMP_FTP_SERVER
		CWMP_FTP_SERVER is defined for "InternetGatewayDevice.Services.X_CU_FTPService.FTPServer."
	*/
	createFTPAclChain();
	setup_ftp_servers();
#endif
#endif

#ifdef _PRMT_X_CT_COM_ALARM_MONITOR_
	init_alarm_numbers();
#endif

	check_wan_mac();
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	syncLOID();
#endif
#ifdef WLAN_QTN
	createQTN_targetIPconf();
#endif

	/* pass device name to kernel */
	mib_get_s(MIB_DEVICE_NAME,(void *)devName, sizeof(devName));
	if(devName[0] == '\0')
	{
		strcpy(devName, DEFAULT_DEVICE_NAME); //if devName is NULL
	}
	//DEBUG("*****devName=%s*****\n",devName);
	sprintf(cmd,"echo \"%s\" > /proc/sys/kernel/hostname",devName);
	system(cmd);

	if(1){
#ifdef CUSTOMIZE_SSID//CONFIG_USER_AP_CMCC
		if(up_ssid_value()==0){
			unsigned char retValue = 0, kk;
			CUST_SET_SSID_T set_type;
#ifdef CUSTOMER_HW_SETTING_SUPPORT
			for(kk=0; kk<NUM_WLAN_INTERFACE; ++kk){
				retValue = up_cust_hw_value(kk);
				if( retValue==1 ){
					set_type = HW_SSID_GEN;
					set_ssid_customize(set_type);
				}
			}
#else
					set_type = NO_HW_SSID_GEN;
					set_ssid_customize(NO_HW_SSID_GEN);//SSID set
#endif
					retValue = 1;
					mib_set(MIB_SSID_VER, (void *)&retValue);
					mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
				}
#endif
	}

	generate_ifupdown_core_script();
	return ret;
}

#ifdef CONFIG_USER_XDSL_SLAVE
int startSlv(void)
{
	int  sysret, ret=0;
	char sysbuf[128];

	sprintf( sysbuf, "/bin/ucrelay" );
	printf( "system(): %s\n", sysbuf );
	sysret=system( sysbuf );
	if( WEXITSTATUS(sysret)!=0 )
	{
		printf( "exec ucrelay failed!\n" );
		ret=-1;
	}

	if(ret==0)
	{
#if 1
		int i=3;
		while(i--)
		{
			sprintf( sysbuf, "/bin/ucstartslv" );
			printf( "system(): %s\n", sysbuf );
			sysret=system( sysbuf );
			if( WEXITSTATUS(sysret)!=0 )
			{
				printf( "call /bin/ucstartslv to init slave firmware failed!\n" );
				ret=-1;
			}else{
				ret=0;
				break;
			}
		}
#endif
	}
	return ret;
}
#endif /*CONFIG_USER_XDSL_SLAVE*/

#ifdef CONFIG_KEEP_BOOTCODE_CONFIG
#define BOOTCONF_START 0xbfc07f80
#define BOOTCONF_SIZE  0x40
#define BOOTCONF_MAGIC (('b'<<24) | ('t'<<16) | ('c'<<8) | ('f')) //0x62746366
#define BOOTCONF_PROCNAME	"/proc/bootconf"
struct bootconf
{
	unsigned long	magic;
	unsigned char	mac[6];
	unsigned short	flag;
	unsigned long	ip;
	unsigned long	ipmask;
	unsigned long	serverip;
	unsigned char	filename[24];
	unsigned char	res[16];
};
typedef struct bootconf bootconf_t, *bootconf_p;

static int bootconf_get_from_procfile(bootconf_t *p)
{
	FILE *f;
	int ret=-1;

	if(p==NULL) return ret;
	f=fopen( BOOTCONF_PROCNAME, "r" );
	if(f)
	{
		if( fread(p, 1, BOOTCONF_SIZE, f)==BOOTCONF_SIZE )
		{
			if(p->magic==BOOTCONF_MAGIC)
				ret=0;
			else
				printf( "%s: magic not match %08x\n", __FUNCTION__, p->magic );
		}else{
			printf( "%s: fread errno=%d\n", __FUNCTION__, errno );
		}
		fclose(f);
	}else{
		printf( "%s: can't open %s\n", __FUNCTION__, BOOTCONF_PROCNAME );
	}

	return ret;
}
static void bootconf_updatemib(void)
{
	bootconf_t bc;
	if( bootconf_get_from_procfile(&bc)==0 )
	{
		mib_set( MIB_ELAN_MAC_ADDR, bc.mac );
		mib_set( MIB_ADSL_LAN_IP, &bc.ip );
		mib_set( MIB_ADSL_LAN_SUBNET, &bc.ipmask );
	}else
		printf( "%s: call bootconf_getdata() failed!\n", __FUNCTION__);

	return;
}
#endif /*CONFIG_KEEP_BOOTCODE_CONFIG*/


#ifdef CONFIG_RTL8685_PTM_MII
const char PTMCTL[]="/bin/ptmctl";
int startPTM(void)
{
	int ret=-1;
	if (WAN_MODE & MODE_BOND){
		printf("PTM Bonding Mode!\n");
		if( va_cmd(PTMCTL, 1, 1, "set_sys") )
			goto ptmfail;
		if( va_cmd(PTMCTL, 3, 1, "set_hw", "bonding", "2") )
			goto ptmfail;
	} else {
		printf("PTM Non-Bonding Mode!\n");
		if( va_cmd(PTMCTL, 1, 1, "set_hw") )
			goto ptmfail;
	}

	//default fast path
	if( va_cmd(PTMCTL, 3, 1, "set_qmap", "7", "44444444") )
		goto ptmfail;

	ret=0;
ptmfail:
	return ret;
}

#if defined(CONFIG_USER_XDSL_SLAVE)
int startSlvPTM(void)
{
	int ret=-1;
	if (WAN_MODE & MODE_BOND){
		printf("PTM Bonding Mode!\n");
		if( va_cmd(PTMCTL, 3, 1, "-d", "/dev/ptm1", "set_sys") )
			goto ptmfail;
		if( va_cmd(PTMCTL, 5, 1, "-d", "/dev/ptm1", "set_hw", "bonding", "2") )
			goto ptmfail;
	} else {
		if( va_cmd(PTMCTL, 3, 1, "-d", "/dev/ptm1", "set_hw") )
			goto ptmfail;
	}

	//default fast path
	if( va_cmd(PTMCTL, 5, 1, "-d", "/dev/ptm1", "set_qmap", "7", "44444444") )
		goto ptmfail;

	ret=0;
ptmfail:
	return ret;
}
#endif /*CONFIG_USER_XDSL_SLAVE*/
#endif /*CONFIG_RTL8685_PTM_MII*/

#if defined(CONFIG_EPON_FEATURE)
int startEPON(void)
{
	int entryNum=4;
	unsigned int totalEntry;
	int index;
	int retVal;
	MIB_CE_MIB_EPON_LLID_T mib_llidEntry;
#ifdef CONFIG_COMMON_RT_API
	rt_epon_llid_entry_t llid_entry;
#else
	rtk_epon_llid_entry_t llid_entry;
#endif
	char loid[100]={0};
	char passwd[100]={0};
	char oamcli_cmd[128]={0};

	if(rtk_epon_llidEntryNum_get(&entryNum) != RT_ERR_OK)
		printf("rtk_epon_llidEntryNum_get failed: %s %d\n", __func__, __LINE__);

	totalEntry = mib_chain_total(MIB_EPON_LLID_TBL); /* get chain record size */
	if(totalEntry!= entryNum)
	{
		printf("Error! %s: Chip support LLID entry %d is not the same ad MIB entries nubmer %d\n",
			__func__,entryNum, totalEntry);
		return -1;
	}

	//EPON_LLID MIB Table is not empty, read from it and set to driver
	for(index=0;index<totalEntry;index++)
	{
		int i;
		if (mib_chain_get(MIB_EPON_LLID_TBL, index, (void *)&mib_llidEntry))
		{
			memset(&llid_entry,0,sizeof(llid_entry));
			llid_entry.llidIdx = index;
#ifdef CONFIG_COMMON_RT_API
			rt_epon_llid_entry_get(&llid_entry);
#else
			rtk_epon_llid_entry_get(&llid_entry);
#endif
			for(i=0;i<MAC_ADDR_LEN;i++)
				llid_entry.mac.octet[i] = (unsigned char) mib_llidEntry.macAddr[i];
#ifdef CONFIG_COMMON_RT_API
			rt_epon_llid_entry_set(&llid_entry);
#else
			rtk_epon_llid_entry_set(&llid_entry);
#endif
		}
		else
		{
			printf("Error: %s mib chain get error for index %d\n",__func__,index);
		}
	}

	config_oam_vlancfg();

	return 0;

}
#endif

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
int startPON(void)
{
	unsigned int pon_mode;
	unsigned int pon_led_spec_type;
	int ret;

	if (mib_get_s(MIB_PON_MODE, (void *)&pon_mode, sizeof(pon_mode)) != 0)
	{
		if (pon_mode == EPON_MODE)
		{
#if defined(CONFIG_EPON_FEATURE)
			printf("set config for EPON\n");
#ifdef CONFIG_CU
			/*CU if in silent mode, led need blinking*/
			va_cmd("/bin/oamcli", 4, 1, "set", "ctc", "ponLedInSilent", "1");
#endif
			startEPON();
#endif
		}
#if defined(CONFIG_GPON_FEATURE) && defined(CONFIG_TR142_MODULE)
		else if(pon_mode==GPON_MODE)
		{
			set_wan_ponmac_qos_queue_num();
		}
#endif
	}

	if(mib_get_s(MIB_PON_LED_SPEC, (void *)&pon_led_spec_type, sizeof(pon_led_spec_type)) != 0){
		if((ret = rtk_pon_led_SpecType_set(pon_led_spec_type)) != 0)
			printf("rtk_pon_led_SpecType_set failed, ret = %d\n", ret);
		else
			printf("rtk_pon_led_SpecType_set %d\n", pon_led_spec_type);
	}
	else
		printf("MIB_PON_LED_SPEC get failed\n");

	return 0;
}
#endif


#if defined(CONFIG_USER_JAMVM) && defined (CONFIG_APACHE_FELIX_FRAMEWORK)
int startOsgi(void)
{
	FILE *fp_blist = NULL;
	FILE *fp_cnt = NULL;
	MIB_CE_OSGI_BUNDLE_T entry;
	int entry_cnt;
	int bundle_cnt;
	int idx, i , ignore;
	char osgi_cmd[512];
	char *ignore_bundle[] =
	{
		"System Bundle",
		"RealtekTCPSocketListener",
		"Apache Felix Bundle Repository",
		"Apache Felix Gogo Command",
		"Apache Felix Gogo Runtime",
		"Apache Felix Gogo Shell",
		"osgi.cmpn",
		"Apache Felix Declarative Services",
		"Apache Felix Http Jetty"
	};

	snprintf(osgi_cmd,512, "ls /usr/local/class/felix/bundle/*.jar | wc -l > /tmp/bundle_cnt\n");
	system(osgi_cmd);


	if (!(fp_cnt=fopen("/tmp/bundle_cnt", "r")))
	{
		return 0;
	}

	fscanf(fp_cnt, "%d\n", &bundle_cnt);

	if (!(fp_blist=fopen("/tmp/OSGI_STARTUP", "w")))
	{
		return 0;
	}

	entry_cnt = mib_chain_total(MIB_OSGI_BUNDLE_TBL);

	for(idx = 0 ; idx < entry_cnt; idx++)
	{
		if (!mib_chain_get(MIB_OSGI_BUNDLE_TBL, idx, (void *)&entry))
		{
			fclose(fp_blist);
			return 0;
		}
		for(i = 0 ; i < sizeof(ignore_bundle) / sizeof(ignore_bundle[0]); i++)
		{
			if(strcmp(ignore_bundle[i] , entry.bundle_name) == 0 )
			{
				ignore = 1;
				break;
			}
		}
		if(ignore == 1)
		{
			ignore = 0;
			continue;
		}
		else // write file
		{
			fprintf(fp_blist, "/var/config_osgi/%s,%d,%d\n", entry.bundle_file, entry.bundle_action,++bundle_cnt);
		}
	}

	unlink("/tmp/bundle_cnt");
	fclose(fp_blist);
	fclose(fp_cnt);

	return 1;
}
#endif

#if defined(WLAN_SUPPORT)
#if defined(WLAN_CTC_MULTI_AP)
int checkDefaultBackhaulBss(void)
{
	MIB_CE_MBSSIB_T Entry;
	unsigned char phy_band_select = 0;
	unsigned char mac[6];
	int i = 0;

	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		mib_local_mapping_get( MIB_WLAN_PHY_BAND_SELECT, i, (void *)&phy_band_select);
		if(phy_band_select == PHYBAND_5G){
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_VAP_ITF_INDEX, &Entry);
			if(strlen(Entry.ssid) == 0){
				if (!mib_get_s(MIB_ELAN_MAC_ADDR, (void *)mac, sizeof(mac)))
					return -1;
				snprintf(Entry.ssid, sizeof(Entry.ssid), "ChinaNet-Mesh%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
				snprintf(Entry.wpaPSK, sizeof(Entry.wpaPSK),"!@#$%%12345");
				Entry.hidessid = 1;
#ifdef WLAN_11AX
				Entry.encrypt = WIFI_SEC_WPA2_WPA3_MIXED;
				Entry.unicastCipher = 0;
				Entry.wpa2UnicastCipher = 2;
#else
				Entry.encrypt = WIFI_SEC_WPA2;
				Entry.unicastCipher = 1;
				Entry.wpa2UnicastCipher = 2;
#endif
				mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, &Entry, WLAN_VAP_ITF_INDEX);
#ifdef COMMIT_IMMEDIATELY
				Commit();
#endif
			}
			break;
		}
	}
	return 0;
}
#endif
#if defined(WLAN_WPA)
#ifdef CONFIG_CU
void setVAPDefaultWPAKey(char *key)
{
	int idx=0;
	MIB_CE_MBSSIB_T Entry;

	for(idx = 1; idx<= WLAN_MBSSID_NUM; idx++){
		wlan_getEntry(&Entry, idx);
		strcpy(Entry.wpaPSK, key);
		wlan_setEntry(&Entry, idx);
	}
}
#endif

int checkDefaultWPAKey(void)
{
	int status=0,i;
	unsigned char default_wpakey[MAX_NAME_LEN]={0};
	MIB_CE_MBSSIB_T Entry;

	wlan_getEntry(&Entry, 0);

	if((!strcmp(Entry.wpaPSK,"0"))){
		if(!mib_get_s(MIB_DEFAULT_WLAN_WPAKEY, (void *)default_wpakey, sizeof(default_wpakey)))
			return -1;
		if(strlen(default_wpakey) > 0){
			strcpy(Entry.wpaPSK, default_wpakey);
			wlan_setEntry(&Entry, 0);
#ifdef CONFIG_CU
			setVAPDefaultWPAKey(default_wpakey);
#endif

#ifdef CONFIG_USER_CUMANAGEDEAMON
			mib_set(CU_SRVMGT_ORIGINAL_PSK,(void*)default_wpakey);
#endif
			printf("Set Default WPA Key to %s \n" , default_wpakey);
			status|=1;
		}
	}

	return status;
}
#endif

#ifdef CONFIG_00R0
int checkDefaultSSID(void)
{
	int status = 0;
	MIB_CE_MBSSIB_T Entry;
	char default_ssid[MAX_NAME_LEN];
	unsigned char mac[6];
	char end_mac[5]="";
	unsigned char vChar = 1;
	wlan_getEntry(&Entry, 0);

	memset(default_ssid, 0, sizeof(default_ssid));

	if(!strcmp(Entry.ssid,"0"))
	{
		if (!mib_get_s(MIB_ELAN_MAC_ADDR, (void *)mac, sizeof(mac)))
			return -1;

		if(!mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar)))
			return -1;

		sprintf(end_mac,"%02X%02X",mac[4],mac[5]);
		if(vChar==PHYBAND_5G)
			sprintf(default_ssid,"RT-5GPON-%s", end_mac);
		else
			sprintf(default_ssid,"RT-GPON-%s", end_mac);

		printf("set default SSID %s\n",default_ssid);
		strncpy(Entry.ssid, default_ssid, sizeof(Entry.ssid));
		Entry.wsc_configured = 1;
		wlan_setEntry(&Entry, 0);
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	}

	return 0;
}
#else
int checkDefaultSSID(void)
{
	int status=0;
	char current_SSID[256];
	unsigned char default_ssid[MAX_NAME_LEN]={0};
	unsigned char vChar = PHYBAND_2G;
	MIB_CE_MBSSIB_T Entry;
#ifdef CONFIG_CU
	unsigned char prefix_exist = 0;
	char root_default_ssid[MAX_NAME_LEN] = {0};
#endif
#if defined(CONFIG_TRUE) || defined(CONFIG_TLKM)
	unsigned char mac[MAC_ADDR_LEN];
#endif
#ifdef CONFIG_TELMEX_DEV
	int len_sn;
	int needUpdate = 0;
	char serial_no[64]={0};
	unsigned char default_ssid_prefix[MAX_NAME_LEN]={0};
	int i;
#endif

#ifdef CONFIG_TELMEX_DEV
	mib_get_s( MIB_HW_SERIAL_NUMBER,  (void *)serial_no, sizeof(serial_no));
	len_sn = strlen(serial_no);
	if(len_sn > 4)
	{
		snprintf(default_ssid_prefix, sizeof(default_ssid_prefix), "%s%s", "INFINITUM", &serial_no[len_sn-4]);
	}
	else
	{
		snprintf(default_ssid_prefix, sizeof(default_ssid_prefix), "%s%s", "INFINITUM", serial_no);
	}

	for(i=0; i<=NUM_VWLAN_INTERFACE;++i)
	{
		needUpdate = 0;
		if(!mib_chain_get(MIB_MBSSIB_TBL, i, (void *)&Entry))
			continue;
		if(!strcmp(Entry.ssid,"0"))
		{
			unsigned char default_ssid[MAX_SSID_LEN]={0};

			mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
			if(vChar==PHYBAND_5G)
				snprintf(default_ssid, sizeof(default_ssid), "%s_5", default_ssid_prefix);
			else
				snprintf(default_ssid, sizeof(default_ssid), "%s_2.4", default_ssid_prefix);

			if (i != 0)
				snprintf(default_ssid, sizeof(default_ssid), "%s_%d", default_ssid, i+1);

			if(strcmp(Entry.ssid, default_ssid))
			{
				snprintf(Entry.ssid, sizeof(Entry.ssid), "%s", default_ssid);
				needUpdate = 1;
			}
		}

		if(needUpdate)
		{
			mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, i);
			status|=1;
		}
	}
#else
	wlan_getEntry(&Entry, 0);

	if(!strcmp(Entry.ssid,"0")){
		if (!mib_get_s(MIB_DEFAULT_WLAN_SSID, (void *)default_ssid, sizeof(default_ssid)))
			return -1;
        if(strlen(default_ssid) > 0){
#ifdef CONFIG_CU
			if(!strncmp(default_ssid,"CU_",3))
				prefix_exist = 1;
			if(wlan_idx == 0){
				snprintf(root_default_ssid,sizeof(root_default_ssid),"CU_%s",prefix_exist?(&default_ssid[3]):default_ssid);
				printf("Set default 2G SSID %s\n",root_default_ssid);
			}
			else{
				snprintf(root_default_ssid,sizeof(root_default_ssid),"CU_%s_5G",prefix_exist?(&default_ssid[3]):default_ssid);
				printf("Set default 5G SSID %s\n",root_default_ssid);
			}
			strncpy(Entry.ssid,root_default_ssid,sizeof(Entry.ssid));
#elif defined(CONFIG_TRUE) || defined(CONFIG_TLKM)
			mib_get(MIB_ELAN_MAC_ADDR, (void *)mac);
			if (vChar == PHYBAND_5G)
				snprintf(default_ssid, sizeof(default_ssid), "%s_home5G_%1x%02x", default_ssid, mac[4]&0x0f, mac[5]);
			else
				snprintf(default_ssid, sizeof(default_ssid), "%s_home2G_%1x%02x", default_ssid, mac[4]&0x0f, mac[5]);
#else
			mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
			if(vChar==PHYBAND_5G)
				strcat(default_ssid, "-5G");
            strcpy(Entry.ssid, default_ssid);
            printf("set default SSID %s\n",default_ssid);
#endif
            wlan_setEntry(&Entry, 0);
			status|=1;
		}
#if defined(CONFIG_CT_AWIFI_JITUAN_FEATURE)
        unsigned char functype=0;
        mib_get_s(AWIFI_PROVINCE_CODE, &functype, sizeof(functype));
        if(functype == AWIFI_ZJ){
			wlan_getEntry(&Entry, 1);
			if(strncmp(Entry.ssid, "aWiFi", 5) != 0){
				strcpy(Entry.ssid, "aWiFi");
				printf("set aWiFi default SSID aWiFi\n");
				wlan_setEntry(&Entry, 1);
				mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
			}
        }
#endif
	}
#endif
	return status;
}
#endif	// CONFIG_00R0
#endif	// WLAN_SUPPORT

#ifdef CONFIG_TELMEX_DEV
void checkDefaultDomainName(void)
{
	char domain[MAX_NAME_LEN];

	mib_get(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)domain);
	if(strcmp(domain, "domain.name") == 0)
	{
		strcpy(domain, "domain.local");
		mib_set(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)domain);
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	}
}
#endif

#if (defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE))
#if defined(_SUPPORT_L2BRIDGING_PROFILE_) || defined(_SUPPORT_INTFGRPING_PROFILE_)
static void clear_l2bridge_created_by_omci()
{
	int total, i;
	MIB_L2BRIDGE_GROUP_T entry;

	total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);

	for(i = total -1 ; i >= 0 ; i--)
	{
		if(mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, &entry)== 0)
			continue;

		if(entry.omci_configured == 1)
		{
			/* Update Layer2Bridging tree */
			int j = 0, k = 0;
			MIB_L2BRIDGE_FILTER_T l2br_filter_entry = {0};
			MIB_L2BRIDGE_MARKING_T l2br_marking_entry = {0};
			k = mib_chain_total(MIB_L2BRIDGING_FILTER_TBL);
			for (j = (k - 1); j >= 0; j--)
			{
				if (!mib_chain_get(MIB_L2BRIDGING_FILTER_TBL, j, (void *)&l2br_filter_entry))
					continue;

				if (l2br_filter_entry.groupnum == entry.groupnum)
				{
					mib_chain_delete(MIB_L2BRIDGING_FILTER_TBL, j);
				}
			}

			k = mib_chain_total(MIB_L2BRIDGING_MARKING_TBL);
			for (j = (k - 1); j >= 0; j--)
			{
				if (!mib_chain_get(MIB_L2BRIDGING_MARKING_TBL, j, (void *)&l2br_marking_entry))
					continue;

				if (l2br_marking_entry.groupnum == entry.groupnum)
				{
					mib_chain_delete(MIB_L2BRIDGING_MARKING_TBL, j);
				}
			}

			mib_chain_delete(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i);
		}
	}
}
#endif

static void clear_wan_created_by_omci()
{
	int total, i;
	MIB_CE_ATM_VC_T entry;

	total = mib_chain_total(MIB_ATM_VC_TBL);

	for(i = total -1 ; i >= 0 ; i--)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, &entry)== 0)
			continue;

		if(entry.omci_configured == 1)
			mib_chain_delete(MIB_ATM_VC_TBL, i);
	}
}

static void init_uni_port_type()
{
	MIB_CE_SW_PORT_T sw_port_entry = {0};
	int i = 0, pon_mode = 0;
	unsigned char deviceType = 0;

	mib_get_s(MIB_PON_MODE, (void *)&pon_mode, sizeof(pon_mode));
	mib_get_s(MIB_DEVICE_TYPE, (void *)&deviceType, sizeof(deviceType));

	for (i = 0; i < ELANVIF_NUM; i++)
	{
		if (!mib_chain_get(MIB_SW_PORT_TBL, i, (void *)&sw_port_entry))
			continue;

		sw_port_entry.omci_uni_portType = 1;
		sw_port_entry.omci_uni_bridgeIdmask= 0;
		sw_port_entry.omci_uni_learning_limit = 0;
		sw_port_entry.omci_uni_mtu = 0;

		if(sw_port_entry.omci_configured)
		{
			sw_port_entry.omci_configured = 0;
			sw_port_entry.itfGroup = 0;
		}

		//for EPON SFU config LAN port to PPTP type
		if(pon_mode == EPON_MODE && (deviceType == 0 || deviceType == 2))
		{
			sw_port_entry.omci_uni_portType = 0;
			sw_port_entry.omci_configured = 1;
		}

		mib_chain_update(MIB_SW_PORT_TBL, (void *)&sw_port_entry, i);
	}
	return;
}

#if defined(CONFIG_TR142_MODULE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
static void rtk_tr142_init_options()
{
	char cmd[1024] = {0};
	unsigned char uChar = 0;
	unsigned char deviceType = 0;
	unsigned char tr247_unmatched_veip_cfg = 0;
	unsigned char tr142_log_cfg = 0;
	int tr142_customer_flag = 0;
	unsigned char rule_type_for_pptp = 3;
	unsigned char specific_rule_for_unmatched_veip = 0;
	unsigned char tr142_ds_ignore_pbit = 0;
	int pon_mode = 0;

	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
	if(pon_mode != GPON_MODE)
		return;

	if (mib_get_s(MIB_DEVICE_TYPE, (void *)&uChar, sizeof(uChar)) != 0)
		deviceType = uChar;
	else
		deviceType = 2;
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "echo \"device_type %u\" > /proc/rtk_tr142/options", deviceType);
	system(cmd);

	if (mib_get_s(MIB_TR247_UNMATCHED_VEIP_CFG, (void *)&uChar, sizeof(uChar)) != 0)
		tr247_unmatched_veip_cfg = uChar;
	else
		tr247_unmatched_veip_cfg = 0;
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "echo \"tr247_set_rule_for_unmatched_veip %u\" > /proc/rtk_tr142/options", tr247_unmatched_veip_cfg);
	system(cmd);

	if(tr247_unmatched_veip_cfg)
	{
		system("echo \"tr247_rule_type_for_unmatched_veip 3\" > /proc/rtk_tr142/options");
		system("echo \"vlan_deicfi 1\" > /proc/fc/ctrl/flow_hash_input_config ");
		system("echo 1 > /proc/fc/ctrl/flow_l2_skipPsTracking");
		adjustSwitchQueue();
		disableStormControl();
		system("echo 1 > /proc/learn_unknown_unicast");
	}

	if (mib_get_s(MIB_TR142_LOG_CFG, (void *)&uChar, sizeof(uChar)) != 0)
		tr142_log_cfg = uChar;
	else
		tr142_log_cfg = 0;
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "echo \"log_level %u\" > /proc/rtk_tr142/options", tr142_log_cfg);
	system(cmd);

	mib_get_s(MIB_TR142_CUSTOMER_FLAG, &tr142_customer_flag, sizeof(tr142_customer_flag));
	if(tr142_customer_flag & TR142_CUSTOMER_FLAG_L2RULE_TYPE_SMUX_ONLY)
	{
		rule_type_for_pptp = 1;
	}
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "echo \"rule_type_for_pptp %u\" > /proc/rtk_tr142/options", rule_type_for_pptp);
	system(cmd);

	if(tr142_customer_flag & TR142_CUSTOMER_FLAG_UNMATCH_VEIP_SPECIFIC_FLOW)
	{
		specific_rule_for_unmatched_veip = 1;
	}
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "echo \"specific_rule_for_unmatched_veip %u\" > /proc/rtk_tr142/options", specific_rule_for_unmatched_veip);
	system(cmd);

	if(tr142_customer_flag & TR142_CUSTOMER_FLAG_IGNORE_DS_PBIT)
	{
		tr142_ds_ignore_pbit = 1;
	}
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "echo \"ds_ignore_pbit %u\" > /proc/rtk_tr142/options", tr142_ds_ignore_pbit);
	system(cmd);
	return;
}
#endif
#endif

void DisableDevConsoleFlowCtrl();
#ifdef CONFIG_RTK_DEV_AP
#ifdef CONFIG_BRIDGE_IGMP_SNOOPING
static void igmpsnooping_enable_fast_leave(int enable)
{
	char cmdBuff[128] = {0};
	char dirBuff[64] = {0};
	DIR *dir;
	struct dirent *ptr;

	snprintf(dirBuff,sizeof(dirBuff),"/sys/devices/virtual/net/%s/brif",BRIF);

    if ((dir=opendir(dirBuff)) == NULL)
    {
        return;
    }

    while ((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)
			continue;

        {
			snprintf(cmdBuff,sizeof(cmdBuff),"/bin/echo %d >%s/%s/multicast_fast_leave",enable,dirBuff,ptr->d_name);
			system(cmdBuff);
        }
    }
    closedir(dir);
}
#endif
#endif

#ifdef SLEEP_TIMER
static void _sleep_timer_startup_update(void)
{
	//must before startwlan
	int totalnum, i;
	unsigned char enable=0;
	MIB_CE_SLEEPMODE_SCHED_T sleepEntry;
	//in startup, we must reset SLEEPMODE,
	//it will affect internet led
	//mib_set(MIB_RG_SLEEPMODE_ENABLE, (void *)&enable);

	totalnum = mib_chain_total(MIB_SLEEP_MODE_SCHED_TBL);
	for(i=totalnum-1; i>=0; i--)
	{
		mib_chain_get(MIB_SLEEP_MODE_SCHED_TBL, i, &sleepEntry);
		if(1 == sleepEntry.day)
		{
			//AUG_PRT("mib del SLEEP_MODE_SCHED_TBL id=%d\n",i);
			mib_chain_delete(MIB_SLEEP_MODE_SCHED_TBL, i);
#ifdef WLAN_SUPPORT
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
			mib_local_mapping_set(MIB_WLAN_MODULE_DISABLED, 0, (void *)&enable);
#ifdef WLAN_DUALBAND_CONCURRENT
			mib_local_mapping_set(MIB_WLAN_MODULE_DISABLED, 1, (void *)&enable);
#endif
#else
			mib_set(MIB_WIFI_MODULE_DISABLED, (void *)&enable);
#endif //YUEME_3_0_SPEC
#endif //WLAN_SUPPORT
		}
	}
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
}
#endif
#if defined(CONFIG_USER_BLUEZ)&& defined (CONFIG_USER_RTK_BLUETOOTH_FIRMWARE)
#define BT_ADDR_FILE        "/var/firmware/rtlbt/bdaddr"
#define EXTRA_CONFIG_FILE  "/var/firmware/rtlbt/rtk_btconfig"
#define BUF_LEN 128
#if defined(CONFIG_RTK_BLUETOOTH_HW_RTL8822B_S)
#define RTK_BT_CFG_FILE 		 "rtl8822b_config"
#elif defined(CONFIG_RTK_BLUETOOTH_HW_RTL8761A_T)
#define RTK_BT_CFG_FILE			 "rtl8761at_config"
#elif defined(CONFIG_RTK_BLUETOOTH_HW_RTL8761A_TV)
#define RTK_BT_CFG_FILE			 "rtl8761a_config"
#elif defined(CONFIG_RTK_BLUETOOTH_HW_RTL8761B_T)
#define RTK_BT_CFG_FILE			 "rtl8761bt_config"
#elif defined(CONFIG_RTK_BLUETOOTH_HW_RTL8761B_TV)
#define RTK_BT_CFG_FILE			 "rtl8761b_config"
#endif

static void gen_rtlbt_fw_config(void)
{
	FILE *fd;
	int count;
	int i,j;
	unsigned char hw_btaddr[6] = {0};
	unsigned char hw_txpwr_idx[6] = {0};
	unsigned char hw_xtal_cap_val = 0;
	unsigned char hw_tx_dac_val = 0;

	system("mkdir /var/firmware");
#if defined(CONFIG_BT_HCIBTUSB_RTLBTUSB)
	system("cp /etc/rtlbt/* /var/firmware/");
#elif defined(CONFIG_BT_HCIUART_RTL3WIRE)
	system("mkdir /var/firmware/rtlbt");
	system("cp /etc/rtlbt/* /var/firmware/rtlbt/");

	//BT mac address need set to /var/firmware/rtlbt/bdaddr, need not save in config file
	mib_get_s(MIB_BLUETOOTH_HW_BT_ADDR, (void*)&hw_btaddr, sizeof(hw_btaddr));
	fd=fopen(BT_ADDR_FILE, "w");
	if(fd != NULL)
	{
		count = fprintf(fd,"%02x:%02x:%02x:%02x:%02x:%02x\n",hw_btaddr[0],hw_btaddr[1],hw_btaddr[2],hw_btaddr[3],hw_btaddr[4],hw_btaddr[5]);
		if(count < 0)
		{
			printf("Failed to write BT mac file!\n");
		}
		fclose(fd);
	}
	else
	{
		printf("Failed to open BT addr file :%s\n",strerror(errno));
	}

#if defined(CONFIG_RTK_BLUETOOTH_CONFIGURATION_IN_EFUSE)
    //MP parameter will save in chip efuse, do not generate extra config file
    return;
#elif defined(CONFIG_RTK_BLUETOOTH_CONFIGURATION_IN_FLASH)
	mib_get_s(MIB_BLUETOOTH_HW_TX_POWER_IDX,(void*)&hw_txpwr_idx, sizeof(hw_txpwr_idx));
	mib_get_s(MIB_BLUETOOTH_HW_XTAL_CAP_VAL,(void*)&hw_xtal_cap_val, sizeof(hw_xtal_cap_val));
	mib_get_s(MIB_BLUETOOTH_HW_TX_DAC_VAL,(void*)&hw_tx_dac_val, sizeof(hw_tx_dac_val));

	/*****here can 0x15b+4(len)+1M+2M+3M+LE or 0x15a+6+Max+1/2/3/le+0x1*****/
	/*
#if defined(CONFIG_RTK_BLUETOOTH_HW_RTL8822B_S)
	unsigned char txpwr_idx[9] = {0x5a,0x01,0x06,0x23,0x18,0x18,0x18,0x18,0x01};
#else
	unsigned char txpwr_idx[9] = {0x5a,0x01,0x06,0x0e,0x0b,0x0b,0x0b,0xa,0x01};
#endif
	for(i=0;i<5;i++)
	{
		if(hw_txpwr_idx[i] != 0)
		{
			txpwr_idx[3+i] = hw_txpwr_idx[i];
		}
	}
	*/
#if defined(CONFIG_RTK_BLUETOOTH_HW_RTL8822B_S)
	unsigned char txpwr_idx[7] = {0x5b,0x01,0x04,0x18,0x18,0x18,0x18};
#else
	unsigned char txpwr_idx[7] = {0x5b,0x01,0x04,0x0b,0x0b,0x0b,0xa};
#endif
	for(i=0;i<4;i++)
	{
		if(hw_txpwr_idx[i] != 0)
		{
			txpwr_idx[3+i] = hw_txpwr_idx[i];
		}
	}

	unsigned char xtal_cap_val[4] = {0xe6,0x01,0x01,0x00};
	unsigned char tx_dac_val[4] = {0x61,0x01,0x01,0x00};

	xtal_cap_val[3] = hw_xtal_cap_val;
	tx_dac_val[3]  = hw_tx_dac_val;

	fd =fopen(EXTRA_CONFIG_FILE, "w");
	if(fd != NULL)
	{
		if (xtal_cap_val[3] != 0)
		{
			count = fprintf(fd,"0x%02x 0x%02x 0x%02x 0x%02x\n", xtal_cap_val[0],xtal_cap_val[1],xtal_cap_val[2],xtal_cap_val[3]);
			if(count < 0)
			{
				printf("Failed to write bt config file xtal_cap_val!\n");
			}
		}
		count = fprintf(fd,"0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
                            txpwr_idx[0],txpwr_idx[1],txpwr_idx[2],txpwr_idx[3],txpwr_idx[4],txpwr_idx[5],txpwr_idx[6]);
		if(count < 0)
		{
			printf("Failed to write bt config file txpwr_idx!\n");
		}
#if defined(CONFIG_RTK_BLUETOOTH_HW_RTL8761A_TV)
		if (tx_dac_val[3] != 0)
		{
			count = fprintf(fd,"0x%02x 0x%02x 0x%02x 0x%02x\n", tx_dac_val[0],tx_dac_val[1],tx_dac_val[2],tx_dac_val[3]);
			if(count < 0)
			{
				printf("Failed to write bt config file tx_dac_val!\n");
			}
		}
		fclose(fd);
#endif
	}
	else
	{
		printf("Failed to open BT extra file :%s\n",strerror(errno));
	}
#endif
#endif
    return;
}
#endif

#if defined(CONFIG_RTK_BOARDID)
void sync_board_id(){
	/*
		env bootargs_boardid must be same with hw setting HW_BOARD_ID
	*/
	unsigned int hw_board_id = 0;
	if(mib_get(MIB_HW_BOARD_ID, (void *)&hw_board_id)){
		unsigned int bootargs_boardid=0;
		char bootargs_boardid_buf[64]={0};
		rtk_env_get("bootargs_boardid", bootargs_boardid_buf, sizeof(bootargs_boardid_buf));
		sscanf(bootargs_boardid_buf, "bootargs_boardid=%u", &bootargs_boardid);
		printf("bootargs_boardid=0x%08x hw_board_id=0x%08x\n",bootargs_boardid,hw_board_id);
		if(hw_board_id != bootargs_boardid){
			sprintf(bootargs_boardid_buf,"%u",hw_board_id);
			rtk_env_set("bootargs_boardid", bootargs_boardid_buf);
			printf("reboot system because bootargs_boardid isn't equal to hw_board_id\n");
			cmd_reboot();
		}
	}

}
#endif

#ifdef CONFIG_DEONET_DATAPATH // GNT2400R
static void setup_CPU_Parameter()
{
	//unsigned int wan_mode = 0;
	uint32 acl_idx = 0;
	rt_acl_filterAndQos_t rt_acl_entry = {0,};

	//mib_get_s(MIB_WAN_NAT_MODE, (void *)&wan_mode, sizeof(wan_mode));
	// CPU Protection (rate limit) when NAT
	//if (wan_mode == 1)
	//	system("/bin/diag rt_rate set egress port 9 rate 40000");

	// CPU(P9) Q0 drop enable
	// Important pkt is in Q3
	//system("/bin/diag flowctrl set ingress egress-drop port 9 queue-id 0 drop enable");

	/* for always sending all arp to cpu
	// ARP
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rt_acl_entry.ingress_port_mask = 0x2f;
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_ETHERTYPE_BIT;
	rt_acl_entry.ingress_ethertype = 0x0806;
	rt_acl_entry.ingress_ethertype_mask = 0xffff;
	rt_acl_entry.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT;
	rt_acl_entry.action_fields |= RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT;
	rt_acl_entry.acl_weight = 400;
	rt_acl_entry.action_priority_group_trap_priority = 3;
	rt_acl_filterAndQos_add(rt_acl_entry, &acl_idx);
	*/

	// DHCP v4
	memset(&rt_acl_entry, 0, sizeof(rt_acl_entry));
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rt_acl_entry.ingress_port_mask = 0x2f;
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
	rt_acl_entry.ingress_ipv4_tagif = 1;
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_L4_UDP_BIT;
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
	rt_acl_entry.ingress_dest_l4_port_start = 67;
	rt_acl_entry.ingress_dest_l4_port_end = 68;
	rt_acl_entry.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT;
	rt_acl_entry.action_fields |= RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT;
	rt_acl_entry.acl_weight = 400;
	rt_acl_entry.action_priority_group_trap_priority = 3;
	rt_acl_filterAndQos_add(rt_acl_entry, &acl_idx);

	// DHCP v6
	memset(&rt_acl_entry, 0, sizeof(rt_acl_entry));
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rt_acl_entry.ingress_port_mask = 0x2f;
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_IPV6_TAGIF_BIT;
	rt_acl_entry.ingress_ipv6_tagif = 1;
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_L4_UDP_BIT;
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
	rt_acl_entry.ingress_dest_l4_port_start = 546;
	rt_acl_entry.ingress_dest_l4_port_end = 547;
	rt_acl_entry.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT;
	rt_acl_entry.action_fields |= RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT;
	rt_acl_entry.acl_weight = 400;
	rt_acl_entry.action_priority_group_trap_priority = 3;
	rt_acl_filterAndQos_add(rt_acl_entry, &acl_idx);

	// ICMP v4
	/*
	memset(&rt_acl_entry, 0, sizeof(rt_acl_entry));
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rt_acl_entry.ingress_port_mask = 0x2f;
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_L4_ICMP_BIT;
	rt_acl_entry.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT;
	rt_acl_entry.action_fields |= RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT;
	rt_acl_entry.action_priority_group_trap_priority = 3;
	rt_acl_entry.acl_weight = 400;
	rt_acl_filterAndQos_add(rt_acl_entry, &acl_idx);
	*/

	// ICMP v6
	memset(&rt_acl_entry, 0, sizeof(rt_acl_entry));
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rt_acl_entry.ingress_port_mask = 0x2f;
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_L4_ICMPV6_BIT;
	rt_acl_entry.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT;
	rt_acl_entry.action_fields |= RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT;
	rt_acl_entry.action_priority_group_trap_priority = 3;
	rt_acl_entry.acl_weight = 400;
	rt_acl_filterAndQos_add(rt_acl_entry, &acl_idx);

	// IGMP v4
	/*
	memset(&rt_acl_entry, 0, sizeof(rt_acl_entry));
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
	rt_acl_entry.ingress_port_mask = 0x2f;
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_IPV4_TAGIF_BIT;
	rt_acl_entry.ingress_ipv4_tagif = 1;
	//rt_acl_entry.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
	//rt_acl_entry.ingress_dest_ipv4_addr_start = 0xE0000001;
	//rt_acl_entry.ingress_dest_ipv4_addr_end = 0xEFFFFFFF;
	rt_acl_entry.filter_fields |= RT_ACL_INGRESS_L4_POROTCAL_VALUE_BIT;
	rt_acl_entry.ingress_l4_protocal = 0x02;
	rt_acl_entry.action_fields |= RT_ACL_ACTION_FORWARD_GROUP_TRAP_BIT;
	rt_acl_entry.action_fields |= RT_ACL_ACTION_PRIORITY_GROUP_TRAP_PRIORITY_BIT;
	rt_acl_entry.action_priority_group_trap_priority = 3;
	rt_acl_entry.acl_weight = 400;
	rt_acl_filterAndQos_add(rt_acl_entry, &acl_idx);
	*/
}
static void setup_Datapath_Parameter()
{
	// accelerate by "what" : 2tuple or 5tuple
	system("echo 0 > /proc/fc/ctrl/bridge_5tuple_flow_accelerate_by_2tuple");

	// p7 enable
	system("echo pon_ds_p7_loopback 1 > proc/fc/ctrl/global_ctrl");

	// * DS threshold
	// 1) fc-ignore (ds latency)
	system("/bin/diag pbo set downstream fc-ignore start threshold 2312");
	system("/bin/diag pbo set downstream fc-ignore stop threshold 2300");

	// 2) sync p7 with p5
	system("/bin/diag port set mac-force port 7 ability 2g5 flow-control disable");
	system("/bin/diag port set serdes port 7 hsgmii-mac n-way force");

	// 3) adjusting ds qos threshold (mcast + uni)
	system("/bin/diag flowctrl set ingress egress-drop port 0,1,2,3 threshold 530");
	system("/bin/diag flowctrl set ingress egress-drop port 5 threshold 200");
	system("/bin/diag flowctrl set ingress egress-drop port 7 threshold 510");
	system("/bin/diag flowctrl set ingress egress-drop port 9 threshold 400");
	system("/bin/diag flowctrl set ingress egress-drop queue-id 0,1,2,3 threshold 200");

	// 4) adjusting flowctrl port/system high/low threshold
	system("/bin/diag flowctrl set ingress system drop-threshold high-on threshold 712");
	system("/bin/diag flowctrl set ingress system drop-threshold high-off threshold 700");
	system("/bin/diag flowctrl set ingress system drop-threshold low-on threshold 486");
	system("/bin/diag flowctrl set ingress system drop-threshold low-off threshold 471");

	system("/bin/diag flowctrl set ingress system flowctrl-threshold high-on threshold 712");
	system("/bin/diag flowctrl set ingress system flowctrl-threshold high-off threshold 700");
	system("/bin/diag flowctrl set ingress system flowctrl-threshold low-on threshold 486");
	system("/bin/diag flowctrl set ingress system flowctrl-threshold low-off threshold 471");

	system("/bin/diag flowctrl set ingress port drop-threshold high-on threshold 712");
	system("/bin/diag flowctrl set ingress port drop-threshold high-off threshold 700");
	system("/bin/diag flowctrl set ingress port drop-threshold low-on threshold 486");
	system("/bin/diag flowctrl set ingress port drop-threshold low-off threshold 471");

	system("/bin/diag flowctrl set ingress port flowctrl-threshold high-on threshold 712");
	system("/bin/diag flowctrl set ingress port flowctrl-threshold high-off threshold 700");
	system("/bin/diag flowctrl set ingress port flowctrl-threshold low-on threshold 486");
	system("/bin/diag flowctrl set ingress port flowctrl-threshold low-off threshold 471");

	// * US threshold
	// 1) pbo set sid
	system("/bin/diag pbo set sid 0 flowctrl-threshold on threshold 150");
	system("/bin/diag pbo set sid 0 flowctrl-threshold off threshold 130");
	system("/bin/diag pbo set sid 1 flowctrl-threshold on threshold 500");
	system("/bin/diag pbo set sid 1 flowctrl-threshold off threshold 480");
	system("/bin/diag pbo set sid 2 flowctrl-threshold on threshold 1150");
	system("/bin/diag pbo set sid 2 flowctrl-threshold off threshold 1110");
	system("/bin/diag pbo set sid 3 flowctrl-threshold on threshold 1350");
	system("/bin/diag pbo set sid 3 flowctrl-threshold off threshold 1310");

	// 2) To prevent all SID flowcontrol states from being latched to 'O'
	//		- stop-all threshold is 7984
	//		- The larger the deviation between the "stop-all threshold" value and
	//		  the "flowctrl-threshold on" value, the more stable it becomes.
	system("/bin/diag pbo set system flowctrl-threshold on threshold 6814");
	system("/bin/diag pbo set system flowctrl-threshold off threshold 6694");
}
#endif

static void startTelnet(void)
{
    unsigned char	manufacturing_mode;

	if (mib_get(MIB_MANUFACTURING_MODE, &manufacturing_mode))
    {
        if(manufacturing_mode)
		{
            system("/usr/bin/telnetd &");
			rtk_gpon_rogueOnt_interrupt_set(0);
		}
    }
}

#ifdef USE_DEONET /* sghong, 20240618 */
void deo_accesslimit_syslog_set(void)
{   
	int idx;
	int totalEntry;
	int syslogid;
	char mode_str[10];
	char syslog_msg[128]; 
	MIB_ACCESSLIMIT_T entry;

	totalEntry = mib_chain_total(MIB_ACCESSLIMIT_TBL); /* get chain record size */

	for (idx = 0; idx < totalEntry; idx++)
	{
		memset(syslog_msg, 0, sizeof(syslog_msg));
		mib_chain_get(MIB_ACCESSLIMIT_TBL, idx, (void *)&entry);

		if (entry.used == 1)
		{
			if (entry.accessBlock == 0)
			{
				syslogid = LOG_DM_INTERNET_LIMIT_ALLOW;
				strcpy(mode_str, "Allow");
			}
			else
			{
				syslogid = LOG_DM_INTERNET_LIMIT_RESTRICT;
				strcpy(mode_str, "Restrict");
			}

			sprintf(syslog_msg, "[INTERNET LIMIT] Connect %s CPE Start %s (%02d:%02d ~ %02d:%02d, %s)",
					mode_str,
					entry.accessMac,
					entry.fromTimeHour, entry.fromTimeMin,
					entry.toTimeHour, entry.toTimeMin,
					entry.weekdays, entry.weekdays);

			syslog2(LOG_DM_SNTP, syslogid, syslog_msg);
		}
	}

	return;
}
#endif

int main(int argc, char *argv[])
{
	unsigned char value[32];
	int vInt;
	unsigned int debug_mode = 0;
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	int snmpd_pid;
#endif
	FILE *fp = NULL;
	int i;
#ifdef TIME_ZONE
	int my_pid;
#endif
#ifdef WLAN_COUNTRY_SETTING
	char strCurrCountry[10], strHwCountry[10];
	unsigned char regDomain;
#endif
#if defined(WLAN_SUPPORT) && (defined(CONFIG_E8B) ||defined(CONFIG_00R0)|| defined(CONFIG_TRUE)|| defined(CONFIG_TELMEX_DEV)||defined(CONFIG_TLKM))
	char wlan_failed = 0;
#endif
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	unsigned char cur_vChar;
#endif
#if defined(CONFIG_RTK_SOC_RTL8198D)
	char sysbuf[128];
#endif
#if defined(WLAN_COUNTRY_SETTING) && defined(WLAN_DUALBAND_CONCURRENT)
	unsigned char phyband=PHYBAND_2G;
#endif

#ifdef CONFIG_HYBRID_MODE
	rtk_pon_change_onu_devmode();
#endif

#ifdef CONFIG_USER_WAN_PORT_AUTO_SELECTION
	va_cmd("/bin/touch", 1, 1, RTL_SYSTEM_REINIT_FILE);
#endif

	int portid;
#ifndef CONFIG_RTL_MULTI_PHY_ETH_WAN
	int wanPhyPort = rtk_port_get_wan_phyID();
#endif
	unsigned char status=0;
#ifdef WLAN_RTIC_SUPPORT
		unsigned char rtic_mode =0;
#endif

	char cmdbuf[128] = {0};
#ifdef CONFIG_RTK_DNS_TRAP
	char domain_name[64]={0};
#endif
	char hw_province_name[32];

#ifdef CONFIG_USER_BOA_WITH_SSL
	int https_status=0;
#endif
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	unsigned char logoTestMode=0;
#endif

	setpriority(PRIO_PROCESS, getpid(), -18);

#if defined(SWAP_WLAN_MIB)
	int needReboot_cs=0, needReboot_hs=0, max_wait=60, loop=0, syncFlash=0;
	CPC_info cpc_info, cpc_info1;
	#define CONFIG_PARTITION_PATH "/var/run/.config_partition_write_count"
	#define SWAP_HS_DONE "/tmp/syc_flash_hs_done.txt"

	memset(&cpc_info, 0, sizeof(CPC_info));
	memset(&cpc_info1, 0, sizeof(CPC_info));

	printf("**********************************************\n");
	// === swap wlan cs mib ===
	/*if config.xml not exist, then cpc file exist here after generate config.xml
	  if config.xml exist, then cpc file not exist here
	*/
	if(isFileExist((char *)CONFIG_PARTITION_PATH)){
		getConfigPartitionCount(&cpc_info);
	}

	if(needSwapWlanMib(CURRENT_SETTING))
	{
#ifdef SWAP_WLAN_MIB_AUTO
		printf("\033[0;31mSwap wlan cs mib automaticlly !!! \033[m\n");
		syncFlash = 1;
		snprintf(cmdbuf, sizeof(cmdbuf), "flash swap wlan cs 1");
		system(cmdbuf);
#else
		printf("\033[0;31mPlease swap wlan cs first, and do reboot !!! \033[m\n");
#endif
	}

#ifdef SWAP_WLAN_MIB_AUTO
	if(syncFlash)
	{
		printf("before sync flash, hs_count:%d, cs_count:%d\n", cpc_info.hs_count, cpc_info.cs_count);
		loop = max_wait;
		while(loop--)
		{
			if(isFileExist((char *)CONFIG_PARTITION_PATH))
			{
				getConfigPartitionCount(&cpc_info1);
				if(cpc_info1.cs_count-cpc_info.cs_count==1){
					printf("\033[0;31msync flash cs done !!!\033[m\n");
					needReboot_cs = 1;
					break;
				}else{
					printf("[1]sync flash wlan cs doing...\n");
					sleep(1);
				}
			}else
			{
				printf("[2]sync flash wlan cs doing...\n");
				sleep(1);
			}
		}
		if(!needReboot_cs){
			printf("\033[0;31msync wlan cs to flash fail !!!!!!!\033[m\n");
			goto startup_fail;
		}
	}
#endif

	// === swap wlan hs mib ===
	syncFlash = 0;
	if(needSwapWlanMib(HW_SETTING))
	{
#ifdef SWAP_WLAN_MIB_AUTO
		printf("\033[0;31mSwap wlan hs mib automaticlly !!! \033[m\n");
		if(isFileExist((char *)SWAP_HS_DONE))
			unlink(SWAP_HS_DONE);
		syncFlash = 1;
		snprintf(cmdbuf, sizeof(cmdbuf), "flash swap wlan hs 1");
		system(cmdbuf);

#else
		printf("\033[0;31mPlease swap wlan hs first, and do reboot !!! \033[m\n");
#endif
	}

#ifdef SWAP_WLAN_MIB_AUTO
	if(syncFlash){
		loop = max_wait;
		while(loop--)
		{
			if(isFileExist((char *)SWAP_HS_DONE))
			{
				printf("\033[0;31msync flash hs done !!!\033[m\n");
				needReboot_hs = 1;
				unlink(SWAP_HS_DONE);
				break;
			}else
			{
				printf("sync flash wlan hs doing...\n");
				sleep(1);
			}
		}
		if(!needReboot_hs){
			printf("\033[0;31msync wlan hs to flash fail !!!!!!!\033[m\n");
			goto startup_fail;
		}
	}

	if(needReboot_hs || needReboot_cs){
		printf("\033[0;31mAfter swap wlan mib automaticlly, Reboot now......\033[m\n");
		system("reboot");
	}
#endif

	printf("**********************************************\n");
#endif

#ifdef WLAN_SUPPORT
//#if defined(CONFIG_RTL8852AE_BACKPORTS) && defined(CPTCFG_RTL8192CD_MODULE)
#if defined(WIFI5_WIFI6_COMP)
	extern int check_wlan_interface(void);
	check_wlan_interface();
#endif
//#endif
#endif
#ifdef CONFIG_CU
	setTmpDropBridgeDhcpPktRule(1);
#endif

#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_ctrl_init();
#endif
#if defined(CONFIG_LUNA) && defined(CONFIG_COMMON_RT_API)
	rtk_fw_setup_storm_control();
	rtk_fw_setup_arp_storm_control();
#endif
#ifdef CONFIG_USER_AVALANCH_DETECT
	init_avalanch_detect_fw();
	setup_avalanch_detect_fw();
#endif
#ifdef CONFIG_USER_RTK_FASTPASSNF_MODULE
	rtk_switch_fastpass_nf_modules(1);
#endif
#ifdef CONFIG_USER_RTK_FAST_BR_PASSNF_MODULE
	rtk_switch_fastpass_br_modules(1);
#endif


	debug_mode = 0;
	while ((i = getopt(argc, argv, "d")) != -1){
		switch (i) {
			case 'd':
				debug_mode |= (STA_INFO|STA_SCRIPT|STA_WARNING);
				break;
			default:
				break;
		}
	}

	// set debug mode
	//DEBUGMODE(STA_INFO|STA_SCRIPT|STA_WARNING|STA_ERR);
	DEBUGMODE(debug_mode|STA_ERR);

#ifdef CONFIG_KEEP_BOOTCODE_CONFIG
	bootconf_updatemib();
#endif /*CONFIG_KEEP_BOOTCODE_CONFIG*/

#if (defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE))
#if defined(_SUPPORT_L2BRIDGING_PROFILE_) || defined(_SUPPORT_INTFGRPING_PROFILE_)
	clear_l2bridge_created_by_omci();
#endif
	clear_wan_created_by_omci();
	init_uni_port_type();

#ifdef CONFIG_USER_IGMPTR247
	rtk_tr247_multicast_init();
#endif

#if defined(CONFIG_TR142_MODULE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_tr142_init_options();
#endif
#endif

#if ! defined(_LINUX_2_6_) && defined(CONFIG_RTL_MULTI_WAN)
	initWANMode();
#endif
	setup_wan_port();

#if defined(CONFIG_DSL_ON_SLAVE)
	system("/bin/adslctrl InitSAR 1");
	printf("/bin/adslctrl InitSAR 1\n");
	sleep(1);
	system("/bin/adslctrl InitPTM 1");
	printf("/bin/adslctrl InitPTM 1\n");
	sleep(1);
#endif

#ifdef CONFIG_RTL_SMUX_DEV
#ifndef CONFIG_RTL_MULTI_PHY_ETH_WAN
	va_cmd(SMUXCTL, 11, 1, "--if", ALIASNAME_NAS0, "--set-if-rsmux", "--set-if-rx-policy", "DROP",
							"--set-if-tx-policy", "CONTINUE", "--set-if-rxmc-policy", "DROP", "--set-if-rx-multi", "0");
#endif
#if 0 //defined(CONFIG_RTL_SMUX_DEV) && ! defined(CONFIG_LUNA_G3_SERIES)  //don't enable hw vlan filter, because VLAN transparent case
	rtk_smuxdev_hw_vlan_filter_init();
#endif
	snprintf(cmdbuf, sizeof(cmdbuf), "echo auto_reserve_vlan 1 > proc/rtk_smuxdev/configure");
	va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
#endif

	if (sys_setup() == -1)
		goto startup_fail;

#ifdef CONFIG_TRUE
	checkDefaultPppAccount();
#endif

	/*
     * Do not call DisableDevConsoleFlowCtrl at background to avoid to be
     * stopped by job signal
     * rtk_is_inforeGroundpgrp will return -1 if startup call from RC22
	 */
	if(0 != rtk_is_inforeGroundpgrp()) {
		DisableDevConsoleFlowCtrl();
	}

#ifdef CONFIG_INIT_SCRIPTS
	printf("========== Initiating Starting Script =============\n");
	system("sh /var/config/start_script");
	printf("========== End Initiating Starting Script =============\n");
#endif

#if defined(CONFIG_RTK_BOARDID) && defined(CONFIG_CMCC)
	sync_board_id();
#endif
#ifdef CONFIG_USER_RTK_SYSLOG
	stopLog();
	if(-1==startLog())
		goto startup_fail;
#endif
	startTelnet();

	stopAutoReboot();
	startAutoReboot();

	stopConntrack_mon();
	startConntrack_mon();

	startLdap();

	stopUseage();
	startUseage();

	if (deo_wan_nat_mode_get() == DEO_BRIDGE_MODE)
	{
		stopDhcpRelay();
		deo_startDhcpRelay();
	}

	if (deo_ipv6_status_get() == 1)
	{
		stopDhcpRelay6();
		deo_startDhcpRelay6();
	}

	if (-1==startELan())
		printf("startELan fail, plz check!\n");
#ifdef CONFIG_RTK_DNS_TRAP
	if (mib_get(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)domain_name) != 0)
	{
		strtolower(domain_name,strlen(domain_name));
		if(strchr(domain_name, '.')==NULL)
			strcat(domain_name,".com");
		create_hosts_file(domain_name);
		snprintf(cmdbuf, sizeof(cmdbuf), "echo \"%s\" > /proc/driver/realtek/domain_name",domain_name);
		system(cmdbuf);

#ifdef CONFIG_RTK_SKB_MARK2
		snprintf(cmdbuf,sizeof(cmdbuf),"echo %d > /proc/driver/realtek/dns_skb_mark",SOCK_MARK2_FORWARD_BY_PS_START);
		system(cmdbuf);
#else
		snprintf(cmdbuf,sizeof(cmdbuf),"echo %d > /proc/fc/ctrl/skbmark_fwdByPs",SOCK_MARK_METER_INDEX_END);
		system(cmdbuf);

		snprintf(cmdbuf,sizeof(cmdbuf),"echo %d > /proc/driver/realtek/dns_skb_mark",SOCK_MARK_METER_INDEX_END);
		system(cmdbuf);
#endif
	}
#endif
#if defined(CONFIG_CMCC)
		mib_get_s(MIB_HW_PROVINCE_NAME, (void *)hw_province_name, sizeof(hw_province_name));
		if(!strncmp(hw_province_name, "factory", strlen("factory")))
		{
			//For MP, need ping 1.1 success eariler
			set_all_port_powerdown_state(0);
		}
#endif

#ifdef CONFIG_USER_LXC
        va_cmd(EBTABLES, 7, 1, "-I", "FORWARD", "1", "-i", "veth+", "-j" ,"ACCEPT");
	va_cmd(EBTABLES, 7, 1, "-I", "FORWARD", "1", "-o", "veth+", "-j" ,"ACCEPT");
#endif
#ifdef CONFIG_RTK_MAC_AUTOBINDING
	do{
		unsigned char flag=0;
		if(mib_get(MIB_AUTO_MAC_BINDING_IPTV_BRIDGE_WAN,&flag)&&flag){
				char cmd_str[128]={0};
				sprintf(cmd_str,"echo %d > /proc/mac_autobinding/mode",flag-1);//0--for ShanDong 1--for GuiZhou
				va_cmd("/bin/sh",2,1,"-c",cmd_str);
		}
	}while(0);
#endif



#ifdef CONFIG_RTK_DEV_AP
	//start timelycheck
	startTimelycheck();
#endif

#ifdef CONFIG_USER_LLMNR
	restartLlmnr();

#endif
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	if ( mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode)) ){
		//This proc is used for kernel path
		snprintf(cmdbuf, sizeof(cmdbuf), "echo %d > /proc/sys/net/ipv6/rtk_ip6_logo_test", logoTestMode);
		va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
#ifndef CONFIG_RTK_DEV_AP
		if(logoTestMode == 1){
			/* disable acceleration, for Selftest_5_0_7 router spec 28, may hit flow result in wrong next hop */
			snprintf(cmdbuf, sizeof(cmdbuf), "echo %d > /proc/fc/ctrl/hwnat", 0);
			va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
		}
#endif
	}
#endif


#ifdef CONFIG_USER_RTK_BRIDGE_MODE
		if(getOpMode()==BRIDGE_MODE)
		{
			//set external switch vlan group, bridge mode
			system("echo op_mode 1 > /proc/driver/realtek/rtk_op_mode");

#if defined(CONFIG_LUNA) && defined(CONFIG_RTL8686_SWITCH)
			set_all_port_powerdown_state(0);
#endif

			startBridge();

#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
			int logic_port = rtk_port_get_default_wan_logicPort();
#endif
#ifdef CONFIG_RTK_DEV_AP
#ifdef CONFIG_BRIDGE_IGMP_SNOOPING
			igmpsnooping_enable_fast_leave(1);
			//  add nas0 to multicast_router
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
			if (logic_port >= 0)
				snprintf(cmdbuf,sizeof(cmdbuf),"echo 2 > /sys/devices/virtual/net/%s/brif/%s/multicast_router",BRIF,ELANRIF[logic_port]);
#else
			snprintf(cmdbuf,sizeof(cmdbuf),"echo 2 > /sys/devices/virtual/net/%s/brif/%s/multicast_router",BRIF,ALIASNAME_NAS0);
#endif
			system(cmdbuf);
#endif
#endif

#ifdef CONFIG_USER_ANDLINK_FST_BOOT_MONITOR
			start_fstBootMonitor();
#endif

#ifdef CONFIG_USER_ANDLINK_PLUGIN
			start_andlink_plugin();
#endif
#ifdef CONFIG_USER_GREEN_PLUGIN
			start_secRouter_plugin();
#endif
			if(mib_get_s(MIB_LED_STATUS, (void *)&status, sizeof(status))!=0)
			{
			   if(status == 0)
				   system("/bin/mpctl led off");
			   else
				   system("/bin/mpctl led restore");
			}
#if defined(CONFIG_USER_RTK_MULTI_BRIDGE_WAN)
			//add bridge wan setting
			if (-1==startBrWan(CONFIGALL, NULL))
				goto startup_fail;
			//set firewall rule and ebtables rule for bridge mode
			setBrWanFirewall();
			setupBrWanMap();
#endif

#if defined(CONFIG_USER_RTK_VLAN_PASSTHROUGH)
			if(is_tagged_br_wan_exist() == 1)
				set_vlan_passthrough(0);
			else
				set_vlan_passthrough(1);
#endif
#ifdef CONFIG_USER_WAN_PORT_AUTO_SELECTION
			kill_by_pidfile_new((const char *)WAN_PORT_AUTO_SELECTION_RUNFILE, SIGTERM);

			if(access(RTL_SYSTEM_REINIT_FILE, F_OK)==0)
				unlink(RTL_SYSTEM_REINIT_FILE);
#endif
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
			if (logoTestMode==2){
				snprintf(cmdbuf, sizeof(cmdbuf), "echo 0 >  /proc/sys/kernel/printk");
				va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
				snprintf(cmdbuf, sizeof(cmdbuf), "echo 0 > /sys/devices/virtual/net/%s/bridge/multicast_snooping", (char*)BRIF);
				va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
				//Readylogo host  need regard dut as a client
				/* ND Case Default Router Switch
				 * Test v6LC.2.2.12: Router Advertisement Processing, Validity (Hosts Only)
				 * Default route list have A and B two entry, spec expect that DUT should use more suitable dst_entry
				 * pls reference RFC 4861 Section 6-3-6  and net/ipv6/route.c  ip6_pol_route func
				 *	  if (net->ipv6.devconf_all->forwarding == 0)
				 *	     strict |= RT6_LOOKUP_F_REACHABLE;
				 * e.g. A is FAILED B is REACHABLE,  DUT should chose B*/
				snprintf(cmdbuf, sizeof(cmdbuf),"echo 0 > /proc/sys/net/ipv6/conf/all/forwarding");
				va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
				snprintf(cmdbuf, sizeof(cmdbuf),"echo 0 > /proc/sys/net/ipv6/conf/%s/forwarding", (char*)BRIF);
				va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
				snprintf(cmdbuf, sizeof(cmdbuf),"echo 1 > /proc/sys/net/ipv6/conf/%s/accept_ra", (char*)BRIF);
				va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
				snprintf(cmdbuf, sizeof(cmdbuf),"echo 1 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", (char*)BRIF);
				va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
				snprintf(cmdbuf, sizeof(cmdbuf),"echo 1 > /proc/sys/net/ipv6/conf/%s/autoconf", (char*)BRIF);
				va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);

				/*Readylogo Test v6LC.2.2.23: Processing Router Advertisment with Route Information Option (Hosts Only)
				 *Please reference ndisc_router_discovery() in ndisc.c
				 */
				snprintf(cmdbuf, sizeof(cmdbuf),"echo 96 > /proc/sys/net/ipv6/conf/%s/accept_ra_rt_info_max_plen", (char*)BRIF);
				va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);

				/* Readylogo Test 3.2.6: Stable addresses (Host Only)
				 * It looks like Test case is not mature, this case intend to test stable_secret(IN6_ADDR_GEN_MODE_STABLE_PRIVACY addr_gen_mode)
				 * But when enable this addr_gen_mode, it will result in other case which need check EUI64 address failed.
				 * So store the proc conf here tentatively
				 */
				snprintf(cmdbuf, sizeof(cmdbuf),"echo fe80::1 > /proc/sys/net/ipv6/conf/%s/stable_secret", (char*)BRIF);
				va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);

				//[ND] Section 6.3.7 case 128 Router Solicitations
				snprintf(cmdbuf, sizeof(cmdbuf),"echo 4 > /proc/sys/net/ipv6/conf/%s/router_solicitation_interval", (char*)BRIF);
				va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
				snprintf(cmdbuf, sizeof(cmdbuf),"echo 3 > /proc/sys/net/ipv6/conf/%s/router_solicitations", (char*)BRIF);
				va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
				//Check Tag for LOGO test bed, when get this message, it means DUT bootup completely
				printf("Init System OK for IPV6\n");
				//trigger NS packet
				snprintf(cmdbuf, sizeof(cmdbuf), "echo 6 > /proc/sys/net/ipv6/conf/%s/dad_transmits", (char*)BRIF);
				va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
				setup_disable_ipv6((char*)BRIF, 1);
				setup_disable_ipv6((char*)BRIF, 0);
				snprintf(cmdbuf, sizeof(cmdbuf), "echo 1 > /proc/sys/net/ipv6/conf/%s/dad_transmits", (char*)BRIF);
				va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
			}
#endif
			return 0;
		}
#endif

	//set external switch vlan group, gateway mode
	system("echo op_mode 0 > /proc/driver/realtek/rtk_op_mode");

	if (deo_wan_nat_mode_get() == DEO_NAT_MODE)
	{
		system("echo deonet_op_mode 0 > /proc/driver/realtek/deonet_op_mode");
		stopDnsProxy();
		deo_startDnsProxy();
	}
	else
	{
		int vInt;
		char buf[128];

		mib_get_s(MIB_IGMP_QUERY_MODE, (void *)&vInt, sizeof(vInt));
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "echo query_mode %d > /proc/driver/realtek/bridge_query_mode", vInt);
		system(buf);

		system("echo deonet_op_mode 1 > /proc/driver/realtek/deonet_op_mode");
	}

#if defined(CONFIG_RTK_SOC_RTL8198D)
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	for (i = 0; i < SW_PORT_NUM; i++) {
		if (!rtk_port_is_wan_logic_port(i))
			continue;
		portid = rtk_port_get_phyPort(i);
		rtk_port_set_dev_port_mapping(portid, (char *)ELANRIF[i]);
		va_cmd(IFCONFIG, 2, 1, ELANRIF[i], "down");
	}
#else
	if(wanPhyPort!=-1){
		rtk_port_set_dev_port_mapping(wanPhyPort, ALIASNAME_NAS0);
	}
	va_cmd(IFCONFIG, 2, 1, ALIASNAME_NAS0, "down");
#endif
#endif

#if defined(CONFIG_LUNA)
#if defined(CONFIG_RTL_MULTI_LAN_DEV) && defined(CONFIG_RTL8686)
	//without RG, default let switch forward packet.
	system("/bin/echo normal > /proc/rtl8686gmac/switch_mode");
#endif
#endif

	// check INIT_SCRIPT
	if (mib_get_s(MIB_INIT_SCRIPT, (void *)value, sizeof(value)) != 0)
	{
		vInt = (int)(*(unsigned char *)value);
	}
	else
		vInt = 1;

	if (vInt == 0)
	{
		 for(i=0;i<ELANVIF_NUM;i++){
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
			if (rtk_port_is_wan_logic_port(i))
				continue;
#else
			portid = rtk_port_get_lan_phyID(i);
			if (portid != -1 && portid == wanPhyPort)
				 continue;
#endif

			va_cmd(IFCONFIG, 2, 1, ELANVIF[i], "up");
		}
#if defined(CONFIG_RTL8681_PTM)
		va_cmd(IFCONFIG, 2, 1, PTMIF, "up");
#endif
#ifdef CONFIG_USB_ETH
		va_cmd(IFCONFIG, 2, 1, USBETHIF, "up");
#endif //CONFIG_USB_ETH
		if(read_pid("/var/run/boa.pid") < 0)
		{
#ifdef CONFIG_USER_BOA_WITH_SSL
			if (mib_get(MIB_WEB_BOA_ENABLE_HTTPS, (void *)&https_status) != 0)
			{
				if(https_status == 1) /* only https port */
					va_niced_cmd(WEBSERVER, 1, 0, "-n");
				else if(https_status == 2) /* only normal port */
					va_niced_cmd(WEBSERVER, 1, 0, "-s");
				else /* both normal and https port */
					va_niced_cmd(WEBSERVER, 0, 0);
			}
			else
				va_niced_cmd(WEBSERVER, 0, 0);
#else
			va_niced_cmd(WEBSERVER, 0, 0);
#endif
		}
		return 0;	// stop here
	}

#ifdef E8B_NEW_DIAGNOSE
	fp = fopen(INFORM_STATUS_FILE, "w");
	if (fp) {
		fprintf(fp, "%d:%s", NO_INFORM, E8B_START_STR);
		fclose(fp);
	}
#endif
#ifdef SUPPORT_WEB_PUSHUP
	//just need to deal status of PROGGRESSING after boot
	unsigned int fw_status = firmwareUpgradeConfigStatus();
	if(fw_status == FW_UPGRADE_STATUS_PROGGRESSING)
		firmwareUpgradeConfigStatusSet(FW_UPGRADE_STATUS_OTHERS);
#endif

#ifdef CONFIG_TELMEX_DEV
	checkDefaultDomainName();
#endif
#ifdef NEED_CHECK_DHCP_SERVER
	//check if need close dhcp server
	if((isOnlyOneBridgeWan()>0)
#ifdef CONFIG_GPON_FEATURE
		||(isHybridGponmode()>0)
#endif
	)
	{
		if(needModifyDhcpMode()>0)
		{
#ifdef COMMIT_IMMEDIATELY
			Commit();
#endif
		}
	}
#endif

#if defined(SUPPORT_WEB_REDIRECT) && defined(CONFIG_E8B)
	if(!check_user_is_registered())
		enable_http_redirect2register_page(1);
#endif

	if (-1==startDaemon())
		goto startup_fail;

	//root interface should be up first
#ifndef CONFIG_RTL_MULTI_LAN_DEV
	if (va_cmd(IFCONFIG, 2, 1, ELANIF, "up"))
		goto startup_fail;
#endif
#ifdef CONFIG_DEV_xDSL
	// Create in ra8670.c, dsl link status
	va_cmd(IFCONFIG, 2, 1, "atm0", "up");
#if defined(CONFIG_USER_CMD_SERVER_SIDE) && defined(CONFIG_DEV_xDSL)
	// Create in rtk_atm.c, ptm link status
	va_cmd(IFCONFIG, 2, 1, "ptm0", "up");
#endif
#endif

#if defined(CONFIG_USER_CMD_SERVER_SIDE) && defined(CONFIG_DEV_xDSL)
	dsl_msg_set(SetDslBond, (WAN_MODE & MODE_BOND));
#else
#ifdef CONFIG_RTL8685_PTM_MII
	if( startPTM()<0 )
		goto startup_fail;

#if defined(CONFIG_USER_XDSL_SLAVE)
	if( startSlvPTM()<0 )
		goto startup_fail;
#endif /*CONFIG_USER_XDSL_SLAVE*/

#endif /*CONFIG_RTL8685_PTM_MII*/
#endif

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	if( startPON()<0 )
		goto startup_fail;
#endif

#ifdef PORT_FORWARD_GENERAL
	clear_dynamic_port_fw(NULL);
#endif

#ifdef CONFIG_E8B
	reset_default_route();
#endif
	//Init IP rule
	setup_policy_route_rule(1);
#if defined(CONFIG_APACHE_FELIX_FRAMEWORK) && (defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC))
	rtk_osgi_setup_osgi_permissions(NULL);
#endif
#ifdef CONFIG_YUEME
#ifdef YUEME_4_0_SPEC
	dbus_do_flash2ExtraMIB();
#endif
#ifdef CONFIG_USER_DBUS_PROXY
	va_niced_cmd("/bin/appctrld", 0, 0);
#endif
#endif

#if defined(CONFIG_XDSL_CTRL_PHY_IS_SOC)
	va_cmd(IFCONFIG, 2, 1, ELANIF, "up");
	for(i=0;i<ELANVIF_NUM;i++){
		if(va_cmd(IFCONFIG, 2, 1, ELANVIF[i], "up")){
			goto startup_fail;
		}
	}
	if (-1==startWan(CONFIGALL, NULL, IS_BOOT))
		goto startup_fail;
#else
	if (-1==startWan(CONFIGALL, NULL, IS_BOOT))
		goto startup_fail;
#endif

	char adpass[MAX_NAME_LEN];
	mib_get_s(MIB_USER_PASSWORD, (void*)adpass, sizeof(adpass));

	/* change admin default password */
	if (!strcmp(adpass, "system") || !strcmp(adpass, "000001_admin")) {
		char macaddr[MAX_NAME_LEN];
		char passwd[20] = {0};
		char tmppw[7] = {0};
		int num = 0;

		getMIB2Str(MIB_ELAN_MAC_ADDR, macaddr);

		// wan is lan mac + 1
		num = (int)strtol(macaddr + 6, NULL, 16);
		snprintf(tmppw, sizeof(tmppw), "%06x", num + 1);

		//change to upper
		strup(tmppw);

		snprintf(passwd, sizeof(passwd), "%s_admin", tmppw);
		if (!mib_set(MIB_USER_PASSWORD, (void *)passwd))
			printf("> Set Super user password to MIB database failed.\n");
		else
			printf("> Update default password(%s).\n", passwd);
	}

#if defined(CONFIG_DEV_xDSL) && defined(CONFIG_USER_CMD_SERVER_SIDE)
	startCommserv();
#endif

#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
			//check MIB_PORT_BINDING_TBL before do setupnewEth2pvc
			checkPortBindEntry();
#endif

#ifdef NEW_PORTMAPPING
			setupnewEth2pvc();
#endif

	if (-1==startDaemonLast())
		goto startup_fail;

	applyLanPortSetting();

#ifdef CONFIG_E8B
	 // Set MAC filter
	setupMacFilterTables();
#endif
#ifdef CONFIG_USER_VLAN_ON_LAN
	setup_VLANonLAN(ADD_RULE);
#endif
#ifdef CONFIG_USER_BRIDGE_GROUPING
	setup_bridge_grouping(ADD_RULE);
#endif
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rtk_igmp_mld_snooping_module_init(BRIF, CONFIGALL, NULL);
#endif
#ifdef _PRMT_X_CMCC_LANINTERFACES_
	setupL2Filter();
	setupMACLimit();
#if defined(DHCP_ARP_IGMP_RATE_LIMIT)
	initDhcpIgmpArpLimit();
#endif
#endif

#if defined(CONFIG_LUNA) && defined(CONFIG_RTL8686_SWITCH)
		set_all_port_powerdown_state(0);
#if defined(CONFIG_RTL9607C_SERIES)
		// set phy parameter again to handle LED
		for(i = 1 ; i <= SW_LAN_PORT_NUM ; i++)
			restart_ethernet(i);
#endif
#endif

#ifdef CONFIG_CU
	 unsigned char powertest_flag = 0;
	 if(mib_get_s(MIB_POWER_TEST_START, &powertest_flag, sizeof(powertest_flag))){
		 if(powertest_flag == 1){
#if defined(CONFIG_RTL9607C_SERIES)
			 va_cmd("/bin/sh", 2, 1, "-c", "/etc/scripts/set_lan_ge_to_fe.sh 1");
#endif
		 }
	 }
#endif


#if defined(CONFIG_RTL8681_PTM)
	if (va_cmd(IFCONFIG, 2, 1, PTMIF, "up"))
		goto startup_fail;
#endif
#ifdef CONFIG_USB_ETH
	if (va_cmd(IFCONFIG, 2, 1, USBETHIF, "up"))
		goto startup_fail;
#endif //CONFIG_USB_ETH

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	startHomeNas();
#endif

	// restart USB to trigger hotplug add uevent
	/*  this method should enable "find" feature in busybox
	**  sd[a-z][0-9],sr[0-9],*lp[0-9] should same with mdev.conf
	 */
	system("find /sys/devices/platform -type d -name sd[a-z] -exec sh -c 'echo \"add\" >> \"$0\"/uevent' {} \\;");
	system("find /sys/devices/platform -type d -name sd[a-z][0-9] -exec sh -c 'echo \"add\" >> \"$0\"/uevent' {} \\;");	//for partition device
	system("find /sys/devices/platform -type d -name sr[0-9] -exec sh -c 'echo \"add\" >> \"$0\"/uevent' {} \\;");
	system("find /sys/devices/platform -type d -name *lp[0-9] -exec sh -c 'echo \"add\" >> \"$0\"/uevent' {} \\;");
//#ifndef CONFIG_ETHWAN
#ifdef CONFIG_DEV_xDSL
	if (-1==startDsl())
		goto startup_fail;
#endif

#ifdef ELAN_LINK_MODE_INTRENAL_PHY
	setupLinkMode_internalPHY();
#endif

#ifdef SLEEP_TIMER
	_sleep_timer_startup_update();
#endif

#ifdef WLAN_SUPPORT
#if defined(WLAN_CTC_MULTI_AP)
	checkDefaultBackhaulBss();
#endif
#if defined(CONFIG_E8B) || defined(CONFIG_00R0)|| defined(CONFIG_TRUE)||defined(CONFIG_TLKM)|| defined(CONFIG_TELMEX_DEV) ||defined(WLAN_USE_DEFAULT_SSID_PSK)
	//check default SSID and WPA key
	int orig_wlan_idx;
	orig_wlan_idx = wlan_idx;

#ifdef NEED_CEHCK_WLAN_INTERFACE
	int only_one_bridge_wan=0;
#ifdef CONFIG_GPON_FEATURE
	int is_hybrid_gpon_mode=0;

	is_hybrid_gpon_mode = isHybridGponmode();
#endif
	only_one_bridge_wan=isOnlyOneBridgeWan();
#endif

	//process each wlan interface
	for(i = 0; i<NUM_WLAN_INTERFACE; i++){
		int check_status=0;
		wlan_idx = i;
#if !defined(RTK_MULTI_AP_ETH_ONLY)
		if (!getInFlags((char *)getWlanIfName(), 0)) {
			printf("Wireless Interface Not Found !\n");
			continue;
	    }
#endif

		if(checkDefaultSSID()>0)
			check_status |= 1;

#if defined(WLAN_WPA)
		if(checkDefaultWPAKey()>0)
			check_status |= 1;
#endif

#ifdef NEED_CEHCK_WLAN_INTERFACE
		//for telmex:if only one bridge wan,turn down wlan
		if(only_one_bridge_wan
#ifdef CONFIG_GPON_FEATURE
			||is_hybrid_gpon_mode
#endif
		)
		{
			if(needModifyWlanInterface()>0)
			{
				check_status |= 1;
			}
		}
#endif

#if defined(WLAN_COUNTRY_SETTING) && defined(WLAN_DUALBAND_CONCURRENT)
		mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, wlan_idx, &phyband);
		if(phyband==PHYBAND_2G)
		{
			mib_get( MIB_WLAN_COUNTRYSTR, (void *)strCurrCountry);
			mib_get( MIB_HW_WLAN1_COUNTRYSTR, (void *)strHwCountry);
			if(strcmp(strCurrCountry, strHwCountry))
			{
				if(strcmp(strCurrCountry, "US") == 0)
				{
					regDomain = 1;
					mib_set(MIB_HW_WLAN1_COUNTRYSTR, (void *)strCurrCountry);
					mib_set(MIB_HW_WLAN1_REG_DOMAIN, (void *)&regDomain);
					mib_update(HW_SETTING, CONFIG_MIB_ALL);
				}
				else if(strcmp(strCurrCountry, "TH") == 0)
				{
					regDomain = 3;
					mib_set(MIB_HW_WLAN1_COUNTRYSTR, (void *)strCurrCountry);
					mib_set(MIB_HW_WLAN1_REG_DOMAIN, (void *)&regDomain);
					mib_update(HW_SETTING, CONFIG_MIB_ALL);
				}
			}
		}
#endif
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
		if(check_status){
			update_wps_configured(0);
			Commit();
		}
#endif
	}
	wlan_idx = orig_wlan_idx;

	//check default SSID and WPA key end

	//if (-1==startWLan())
	//	goto startup_fail;
	extern int startWLan_with_lock(config_wlan_target target, config_wlan_ssid ssid_index);
	wlan_failed = startWLan_with_lock(CONFIG_WLAN_ALL, CONFIG_SSID_ALL);
#else
	//if (-1==startWLan())
	//	goto startup_fail;
	extern int startWLan_with_lock(config_wlan_target target, config_wlan_ssid ssid_index);
	startWLan_with_lock(CONFIG_WLAN_ALL, CONFIG_SSID_ALL);
#endif
#ifdef CONFIG_USER_THERMAL_CONTROL
	va_niced_cmd("/bin/sh", 1, 0, "/etc/scripts/wlan_thermal.sh");
#endif
#ifdef WLAN_QTN
	if(ping_qtn_check()==0)
		startWLan_qtn();
#endif
#ifdef WLAN_CHECK_MIB_BEFORE_RESTART
	mib_backup(CONFIG_MIB_ALL);
#endif
#endif

#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_ctrl_init2();
#endif

#if defined(_SUPPORT_INTFGRPING_PROFILE_)
	rtk_layer2bridging_init_mib_table();

	config_vlan_group(CONFIGALL, NULL, 1, -1);

	config_interface_grouping(CONFIGALL, NULL);
#endif

#ifdef CONFIG_RTK_DEV_AP
#ifdef CONFIG_BRIDGE_IGMP_SNOOPING
	igmpsnooping_enable_fast_leave(1);
#endif
#endif

#if (defined(CONFIG_RTL867X_NETLOG)  && defined (CONFIG_USER_NETLOGGER_SUPPORT))
        va_niced_cmd(NETLOGGER,0,1);
#endif

#if defined(CONFIG_AIRTEL_EMBEDUR)
	write_omd_reboot_log(REBOOT_FLAG);
	get_reboot_info();
	get_reboot_log();
#endif

#ifdef CONFIG_USER_CLUSTER_MANAGE
    va_cmd( "/bin/cluster_server", 0, 0 );
    if (mib_get(MIB_CLUSTER_MANAGE_ENABLE, (void *)value) != 0)
    {
        vInt = (int)(*(unsigned char *)value);
        if(1 == vInt)
            va_cmd( "/bin/cluster_client", 0, 0 );
    }
#endif

#ifdef _CWMP_MIB_ /*jiunming, mib for cwmp-tr069*/
	if (-1==startCWMP())
		goto startup_fail;

#ifdef _PRMT_TR143_
	struct TR143_UDPEchoConfig echoconfig;
	UDPEchoConfigSave( &echoconfig );
	UDPEchoConfigStart( &echoconfig );
#endif //_PRMT_TR143_
#endif	//_CWMP_MIB_

#ifdef CONFIG_USER_DM
	restart_dm();
#endif

#ifdef CONFIG_USER_RTK_OMD
	// no need log reboot infomation by hardware reboot
	//if(is_terminal_reboot() )
	//	write_omd_reboot_log(TERMINAL_REBOOT);
	write_omd_reboot_log(REBOOT_FLAG);
	va_niced_cmd( "/bin/omd_main", 0, 0 );
#endif
#ifdef CONFIG_USER_DBUS
	struct stat sb;
	if (stat(CONFIG_DBUS_FRAMEWORK_PATH"/apps/etc/dbus-1/system.conf", &sb) == -1)
	{
		printf("*** DBUS: USE default system.conf *** \n");
		system("cp /etc/dbus-1/system.conf.def "CONFIG_DBUS_FRAMEWORK_PATH"/apps/etc/dbus-1/system.conf");
#ifdef CONFIG_CU
		system("cp /etc/dbus-1/session.conf.def "CONFIG_DBUS_FRAMEWORK_PATH"/apps/etc/dbus-1/session.conf");
#endif
	}
	va_niced_cmd("/usr/sbin/dbus-daemon", 1, 0, "--system");
#endif
#ifdef CONFIG_USER_DBUS_PROXY
	if(!access("/tmp/.runRestore", F_OK)) unlink("/tmp/.runRestore");
	va_niced_cmd( "/bin/proxyDaemon", 0, 0 );
	va_niced_cmd( "/bin/reg_server", 0, 0 );
#ifdef WLAN_CTC_MULTI_AP
	va_niced_cmd( "/bin/monitor_easymesh", 0, 0 );
#endif
#ifdef WLAN_11AX
	va_niced_cmd( "/bin/smart_wlanapp", 0, 0 );
#endif
#endif

#if defined(WLAN_SUPPORT) && defined(CONFIG_E8B)
	if(wlan_failed)
		syslog(LOG_ERR, "104012 WLAN start failed.");
#endif
	if (-1==startRest())
		goto startup_fail;

#ifdef CONFIG_USER_WATCHDOG_WDG
    //start watchdog & kick every 5 seconds silently
	//va_cmd_no_echo("/bin/wdg", 2, 1, "timeout", "10");
	//va_cmd_no_echo("/bin/wdg", 2, 1, "start", "5");
#endif
#if 0
#ifdef CONFIG_USER_PPPOE_PROXY
       va_cmd("/bin/pppoe-server",0,1);
#endif
#endif
	//take effect
#if defined(CONFIG_XFRM)
#if defined(CONFIG_USER_STRONGSWAN)
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
	rtk_ipsec_strongswan_iptable_rule_init();
#endif
	ipsec_strongswan_take_effect();
#else
	ipsec_take_effect();
#endif
#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) && defined(CONFIG_USER_L2TPD_L2TPD)
	FILE *vpn_lock_f;
	if((vpn_lock_f = fopen(ConfigVpnLock, "w")) == NULL)
	{
		printf("Open %s file failed!!\n", ConfigVpnLock);
	}
	else fclose(vpn_lock_f);
#endif

//move pptp_take_effect() and l2tp_take_effect() to systemd, because it should start vpn after wan up(get IP). 2019/09/25
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP)
	rtk_wan_vpn_set_attach_rules_all(VPN_TYPE_PPTP, VPN_AC_ALL);
	//pptp_take_effect();
#endif
#ifdef CONFIG_USER_PPTPD_PPTPD
	pptpd_take_effect();
#endif

#ifdef CONFIG_USER_PPTP_SERVER
	pptp_server_take_effect();
#endif

#if defined(CONFIG_USER_L2TPD_L2TPD)
	rtk_wan_vpn_set_attach_rules_all(VPN_TYPE_L2TP, VPN_AC_ALL);
	//l2tp_take_effect();
#endif
#ifdef CONFIG_USER_L2TPD_LNS
	l2tp_lns_take_effect();
#endif

#ifdef CONFIG_LIB_LIBRTK_LED
	rtk_setLed(LED_PWR_G, RTK_LED_ON, NULL);
#else
//star add: for ZTE LED request
// Kaohj --- TR068 Power LED
	unsigned char power_flag;
	fp = fopen("/proc/power_flag","w");
	if(fp)
	{
		power_flag = '0';
		fwrite(&power_flag,1,1,fp);
		fclose(fp);
	}
#endif /* CONFIG_LIB_LIBRTK_LED */

#if 0//defined(CONFIG_CU_BASEON_YUEME)
	if(access("/opt/cu/apps/tt",F_OK)<0){
		printf("\ncreate /opt/cu/apps/tt folder\n");
		system("mkdir /opt/cu/apps/tt");
		system("cp /usr/sbin/mgufw /opt/cu/apps/tt/ufwmg");
		/* use sync() to make os to write file currently to avoid file content lost*/
		sync();
	}
#endif

//#ifdef _CWMP_MIB_ /*jiunming, mib for cwmp-tr069*/
	/*when those processes created by startup are killed,
	  they will be zombie processes,jiunming*/
	signal(SIGCHLD, SIG_IGN);//add by star
//#endif

#if defined(CONFIG_CTC_SDN)
	if(check_ovs_enable())
	{
		start_ovs_sdn();
	}
#endif

#if 0
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	// Mason Yu. System init status
	snmpd_pid = read_pid("/var/run/snmpd.pid");
	if (snmpd_pid > 0) {
		printf("Send signal to snmpd.\n");
		kill(snmpd_pid, SIGUSR1);
	}
#endif
#endif

#if defined(CONFIG_TELMEX_DEV) && defined(FIREWALL_SECURITY)
	startFirewall();
#endif

#ifdef CONFIG_IPV6
	restartStaticPDRoute();
	/* Lan Server should be setupped after firewall/portmapping rule, such as
	 * ebtales -p ipv6 -o eth0.3.0 --ip6-proto ipv6-icmp --ip6-icmp-type router-advertisement -j DROP
	 * ip6tables -A INPUT -m physdev --physdev-in eth0.3.0 -p udp -dport 547 -j DROP
	 */
	restartLanV6Server();
//#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
//#ifdef CONFIG_USER_WIDE_DHCPV6
	//modify/add WAN need to restart multi-dhcp6s service.
	//restart_multi_wide_Dhcp6sService(0);
//#endif
//#endif
#endif
#ifdef CONFIG_USER_FON
	extern void setupFonEnv(void);
	setupFonEnv();
	startFonSpot();
#endif
#ifdef CONFIG_INIT_SCRIPTS
	printf("========== Initiating Ending Script =============\n");
	system("sh /var/config/end_script");
	printf("========== End Initiating Ending Script =============\n");
#endif

#if defined(CONFIG_USER_JAMVM) && defined (CONFIG_APACHE_FELIX_FRAMEWORK)
	if(startOsgi() == 0)
		printf("OSGi Start Error!!!\n");
#endif
#ifdef CONFIG_USER_OPENJDK8
#ifdef CONFIG_CMCC_OSGIMANAGE
	if (startOsgiManage() == -1)
	{
		printf("start OsgiManage failed !\n");
	}
#endif
#endif

#if defined(CONFIG_CMCC_WIFI_PORTAL)
	clearWhiteListMib();
	setupTrustedMacConf();
	setupMacWhitelist();
	setupTrustedUrlConf();
#endif

#ifdef TIME_ZONE
	/********************** Important **************************************************
	/  If wan channel is ETHWAN, it will get ip address before system initation finished,
	/  we  kick sntpc to sync the time again
	/********************************************************************************/
	// kick sntpc to sync the time
	my_pid = read_pid(SNTPC_PID);
	if ( my_pid > 0) {
		kill(my_pid, SIGUSR1);
	}
#endif

	#ifdef CONFIG_RPS
	system("sh /etc/scripts/rps.sh on");
	#endif

#if defined(TRIBAND_SUPPORT)
    system("iwpriv wlan0 set_mib amsdu=0");
    system("echo 1 > /proc/fc/ctrl/disableWifiTxDistributed");
    system("echo 2 > /proc/irq/59/smp_affinity");
    system("echo 1 > /proc/irq/61/smp_affinity");
    system("echo 8 > /proc/irq/63/smp_affinity");
    system("echo 8 > /proc/irq/72/smp_affinity");
    system("echo 4 > /proc/irq/73/smp_affinity");
    system("echo 1 > /proc/irq/74/smp_affinity");
#endif

#ifdef CONFIG_TELMEX
	system("echo \"wlan 1 mode 1 cpu 0\" > /proc/fc/ctrl/smp_dispatch_wifi_tx");
#endif

#if defined(CONFIG_RTK_SOC_RTL8198D) || defined(CONFIG_RTK_SOC_RTL9607C)
	system("echo \"wlan 0 mode 1 cpu 0\" > /proc/fc/ctrl/smp_dispatch_wifi_tx");
#endif

#if defined(CONFIG_RTK_SOC_RTL8198D)
	#if defined(CPTCFG_RTL8852AE_BACKPORTS) && defined(CPTCFG_WLAN_8192FE)
		system("echo 4 > /proc/irq/61/smp_affinity");
		system("echo 8 > /proc/irq/73/smp_affinity");
	#elif defined(CPTCFG_RTL8852AE_BACKPORTS) && !defined(CPTCFG_WLAN_8192FE)
		system("echo 1 > /proc/irq/61/smp_affinity");
		system("echo 8 > /proc/irq/73/smp_affinity");
	#else
	#endif
#endif

#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
	setWanPortPowerState();
#endif

#ifdef CONFIG_CTC_E8_CLIENT_LIMIT
	rtk_mwband_set();
#endif

// start andlink(start andlink after dhcpc start)
#ifdef CONFIG_ANDLINK_SUPPORT
	startAndlink();
#endif
#ifdef CONFIG_ELINK_SUPPORT
	startElink();
#endif

#ifdef CONFIG_USER_CTCAPD
	rtk_send_signal_to_daemon(CTCAPD_RUNFILE, SIGUSR1);
#endif

#ifdef CONFIG_USER_CTC_EOS_MIDDLEWARE
	start_eos_middleware();
#endif

#ifdef CONFIG_TR142_MODULE
	set_dot1p_value_byCF();
#endif

#ifdef CONFIG_RTK_SMB_SPEEDUP
	rtk_smb_speedup();
#endif

#ifdef CONFIG_USER_RTK_FASTPASSNF_MODULE
	rtk_fastpassnf_smbspeedup(1);
#if defined(CONFIG_XFRM) && defined(CONFIG_USER_STRONGSWAN)
	rtk_fastpassnf_ipsecspeedup(1);
#endif
#endif
#ifdef CONFIG_USER_RTK_FAST_BR_PASSNF_MODULE
#if defined(CONFIG_XFRM) && defined(CONFIG_USER_STRONGSWAN)
	rtk_fastpassnf_br_ipsecspeedup(1);
#endif
#endif


#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	//Disable snooping of kernel bridge
	system("echo 0 > /sys/class/net/br0/bridge/multicast_snooping");
#endif

#if defined(CONFIG_USER_BLUEZ)&& defined (CONFIG_USER_RTK_BLUETOOTH_FIRMWARE)
	gen_rtlbt_fw_config();
#endif
#ifdef CONFIG_E8B
	start_e8_rest();
#endif

#if defined(CONFIG_CU_BASEON_YUEME)
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_dbus_init_traffic_monitor_rule();
#endif
#endif
#ifdef WLAN_RTIC_SUPPORT
	if(!mib_get_s(MIB_RTIC_MODE, (void *)&rtic_mode, sizeof(rtic_mode)))
		printf("<%s>%d: Error! Get MIB_RTIC_MODE error.\n",__FUNCTION__,__LINE__);
	printf("<%s>%d: init is here!\n",__FUNCTION__,__LINE__);
	if (rtic_mode)
		system("echo 1 > /proc/net/rtic/wlan2/rtic_mode");
	else
		system("echo 0 > /proc/net/rtic/wlan2/rtic_mode");
#endif
#ifdef CONFIG_IPV6
	/*
 	* 2019/12/02
 	* Since now eth0.2~5 and wlan interface are up, change NS number to default number 1.
	 */
	snprintf(cmdbuf, sizeof(cmdbuf), "echo 1 > /proc/sys/net/ipv6/conf/%s/dad_transmits", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);

#if defined(CONFIG_RTK_IPV6_LOGO_TEST)
	if (logoTestMode){
		snprintf(cmdbuf, sizeof(cmdbuf), "echo 0 >  /proc/sys/kernel/printk");
		va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
		snprintf(cmdbuf, sizeof(cmdbuf), "echo 0 > /sys/devices/virtual/net/%s/bridge/multicast_snooping", (char*)BRIF);
		va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);

		if (logoTestMode==3){
			MIB_CE_ATM_VC_T Entry;
			char wan_if[IFNAMSIZ];
			int vcTotal=0;

			vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
			for (i = 0; i < vcTotal; i++){
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
					continue;
				if(0 != ifGetName(Entry.ifIndex, wan_if, sizeof(wan_if))){
					snprintf(cmdbuf, sizeof(cmdbuf), "echo 1 > /proc/sys/net/ipv6/conf/%s/dad_transmits", wan_if);
					va_cmd("/bin/sh", 2, 1, "-c", cmdbuf);
				}
			}
		}
		//Check Tag for LOGO test bed, when get this message, it means DUT bootup completely
		printf("Init System OK for IPV6\n");
		if (logoTestMode == 1 || logoTestMode == 2)
			setLanIPv6Address4Readylogo();
	}
#endif
#endif
#ifdef CONFIG_CT_AWIFI_JITUAN_SMARTWIFI
	{
		unsigned char functype=0;
		mib_get(AWIFI_PROVINCE_CODE, &functype);
		if(functype == AWIFI_ZJ){
			system("ln -s /var/config/awifi/libhttpd.so /var/config/awifi/libhttpd.so.0");
			system("echo 3 > /proc/sys/net/ipv4/tcp_syn_retries");
			system("mkdir -p /var/spool/cron/crontabs");
			if(access("/var/config/awifi", F_OK) != 0)
				system("mkdir -p  /var/config/awifi");
		}
	}
#endif

#ifdef CONFIG_CT_AWIFI_UPGRADE
	{
		unsigned char functype=0;
		mib_get(AWIFI_PROVINCE_CODE, &functype);
		if(functype == AWIFI_ZJ){
	//	  system("/bin/crond&");
		system("/bin/awifi_upgrade&");
		}
	}
#endif

#if defined(INETD_BIND_IP)
	update_inetd_conf();
#endif
#if defined(PROCESS_PERMISSION_LIMIT)
	char permission_ctrl[64] = {0};
#ifdef WLAN_SUPPORT
	unsigned char mode = 0;
	mib_get_s(MIB_WIFI_TEST, (void *)&mode, sizeof(mode));
	if(mode == 1) //mp mode
		snprintf(permission_ctrl, sizeof(permission_ctrl), "echo 1 > %s", PROCESS_PERMISSION_FILE);
	else
#endif
	{
		mib_get_s(MIB_HW_PROVINCE_NAME, (void *)&hw_province_name, sizeof(hw_province_name));
		if(strlen(hw_province_name)==0 || !strcmp(hw_province_name, "factory") || !strcmp(hw_province_name, "default"))
			snprintf(permission_ctrl, sizeof(permission_ctrl), "echo 1 > %s", PROCESS_PERMISSION_FILE);
		else
			snprintf(permission_ctrl, sizeof(permission_ctrl), "echo 0 > %s", PROCESS_PERMISSION_FILE);
	}
	va_cmd("/bin/sh", 2, 1, "-c", permission_ctrl);
	chmod(PROCESS_PERMISSION_FILE, 0666);
#endif

#ifdef CONFIG_USER_WAN_PORT_AUTO_SELECTION
	if(access(RTL_SYSTEM_REINIT_FILE, F_OK)==0)
		unlink(RTL_SYSTEM_REINIT_FILE);

	start_wan_port_auto_selection();
#endif

#ifdef CONFIG_USER_SC_API
	va_cmd("/bin/sc_isp_init",0,1);
#endif

#ifdef CONFIG_USER_ANDLINK_FST_BOOT_MONITOR
		start_fstBootMonitor();
#endif

#ifdef CONFIG_USER_ANDLINK_PLUGIN
		start_andlink_plugin();
#endif
#ifdef CONFIG_USER_GREEN_PLUGIN
		start_secRouter_plugin();
#endif
#ifdef LED_TIMER
		if(mib_get_s(MIB_LED_STATUS, (void *)&status, sizeof(status))!=0)
		{
			if(status == 0)
				system("/bin/mpctl led off");
			else
				system("/bin/mpctl led restore");
		}
#endif
#ifdef _PRMT_X_TRUE_CATV_
		{
			unsigned char catv_enable=0;
			char catv_agcoffset=0;

			mib_get(MIB_CATV_ENABLE,&catv_enable);
			mib_get(MIB_CATV_AGCOFFSET,&catv_agcoffset);
			rtk_fc_set_CATV(catv_enable, catv_agcoffset);
		}
#endif
#if defined(CONFIG_USER_DATA_ELEMENTS_AGENT) || defined(CONFIG_USER_DATA_ELEMENTS_COLLECTOR)
	start_data_element();
#endif
#ifdef CONFIG_USER_WSDD2
	char wsdd2_ctrl[64] = {0};
	snprintf(wsdd2_ctrl, sizeof(wsdd2_ctrl), "/bin/wsdd2 -L -l -i %s &", (char*)BRIF);
	va_cmd("/bin/sh", 2, 1, "-c", wsdd2_ctrl);
#endif
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	//let port counter sync between linux and external switch
	char extsw_cmd[256]={0};
	for(i=0;i<ELANVIF_NUM;i++)
	{
		if(rtk_port_check_external_port(i))
		{
#if defined(CONFIG_RTL_MULTI_LAN_DEV) && defined(CONFIG_RTL_SMUX_DEV)
			rtk_external_port_set_dev_port_mapping(rtk_external_port_get_lan_phyID(i), (char *)EXTELANRIF[i]);
#endif
		}
	}
#endif

#ifdef CONFIG_DEV_xDSL
#ifdef CONFIG_USER_RTK_DSL_LOG
	startDslLog();
#endif
#endif

#ifdef CONFIG_USB_SUPPORT
	resetUsbDevice();
#endif

#ifdef CONFIG_USER_MONITORD
	startMonitord(0);
#endif


#if CONFIG_SND_RTL8277C_SOC_I2S_MODULE
	char mod_inspath[128] = {0};
	snprintf(mod_inspath, sizeof(mod_inspath), "modprobe snd-soc-rtl8277c.ko bypass_clk_init=1");
	system(mod_inspath);

	#ifdef CONFIG_SND_RTL8277C_SOC_IIS_TEST_MODULE
	snprintf(mod_inspath, sizeof(mod_inspath), "modprobe  snd-soc-i2s-rtktest.ko");
	system(mod_inspath);
	#endif

#endif

#ifdef CONFIG_RTL8277C_SERIES
	system("rtl8277c_config");
#endif

#if defined(CONFIG_CU)
	unsigned char nessus_test_start = 0;
	mib_get_s(MIB_CU_NESSUS_TEST_START, &nessus_test_start, sizeof(nessus_test_start));
	if(nessus_test_start)
	{
		printf("CU_NESSUS_TEST_START:%d!\n",nessus_test_start);
		system("echo 0 > /proc/sys/net/ipv4/icmp_ratelimit");
		system("echo 0 > /proc/sys/net/ipv4/icmp_ratemask");
		system("echo 0 > /proc/sys/net/ipv6/icmp/ratelimit");
	}
#endif

#ifdef CONFIG_DEONET_RULE_REFRESH
	// refresh for reserved multicast rule
	unsigned char mode4;
	unsigned char mode6;

	mode4 = deo_wan_nat_mode_get();
	if (mode4 == 1 || mode4 == 2) {
		if (mode4 == 1)
			system("echo deonet_op_mode 0 > /proc/driver/realtek/deonet_op_mode");
		else if (mode4 == 2) {
			system("echo deonet_op_mode 1 > /proc/driver/realtek/deonet_op_mode");

			multicast_drop_specific_port(1);
			// drop default ingress management ip range
			rt_acl_drop_managemet_ip_rule(1);
		}
	}

	mode6 = deo_ipv6_status_get();
	if (mode6 == 0 || mode6 == 1) {
		if (mode6 == 0)
			system("echo ipv6_enable 0 > /proc/driver/realtek/deonet_ipv6_enable");
		else if (mode6 == 1)
			system("echo ipv6_enable 1 > /proc/driver/realtek/deonet_ipv6_enable");
	}

	multicast_forward_shared_specific_address(mode4, mode6);
	multicast_drop_specific_address(mode4, mode6);
	multicast_trap_IEEE_1905_acl();

	// if receive ont source mac based ingress packet, drop
	// mode4: 1-NAT 2-BRIDGE
	// mode6: 0-disable 1-enable
	source_mac_drop(mode4);
#endif

#ifdef CONFIG_DEONET_DATAPATH
	setup_CPU_Parameter();
	setup_Datapath_Parameter();
#endif

#ifdef USE_DEONET
	/* control dropbear for ssh */
	start_ssh();

	deo_accesslimit_syslog_set();

	/* access limit set */
	deo_accesslimit_set();

	// ethertype 0x893a drop control
	va_cmd(SMUXCTL, 12, 1, "--if", "nas0", "--tx", "--tags", "0", "--filter-ethertype", "0x893a", "--target", "DROP", "--rule-priority", "1100", "--rule-append");
	va_cmd(SMUXCTL, 12, 1, "--if", "nas0", "--rx", "--tags", "1", "--filter-ethertype", "0x893a", "--target", "DROP", "--rule-priority", "1100", "--rule-append");

	// ethertype 0x9000 drop control
	va_cmd(SMUXCTL, 12, 1, "--if", "nas0", "--tx", "--tags", "0", "--filter-ethertype", "0x9000", "--target", "DROP", "--rule-priority", "1100", "--rule-append");

	// request key to ADAMS
	system("/bin/requestkey 0 0 &");
#endif

	if (mode4 == DEO_BRIDGE_MODE)
	{
		/* br0 ip 충돌, HFR 10G olt gpon connect br0:1 ip 미설정 이슈 수정 */
#ifdef USE_DEONET /* sghong, 20251201 */
		va_cmd("/usr/bin/ip", 5, 1, "addr", "del", "192.168.75.1/24", "dev", "br0");
#else
		va_cmd("/bin/ifconfig", 2, 1, "br0", "192.168.75.2");
#endif
		/* br0:1 ip set */
		va_cmd("/bin/ifconfig", 2, 1, "br0:1", "192.168.75.1");
	}

	/** To allow other threads to continue execution, the main thread should
	 ** terminate by calling pthread_exit() rather than exit(3). */
	printf("System startup success !\n");
	pthread_exit(NULL);
	//return 0;	// child thread will exit with main thread exit, even child thread detech
startup_fail:
	va_niced_cmd(WEBSERVER,0,1);
	printf("\033[0;31mSystem startup failed !\033[0m\n");
	return -1;
}

#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
void DisableDevConsoleFlowCtrl()
{
	int fd;
	struct termios terminalAttributes;

	if((fd = open("/dev/console", O_RDONLY)))
	{
		memset(&terminalAttributes, 0, sizeof(struct termios));
		if(tcgetattr(fd, &terminalAttributes) == 0)
		{
			printf(">>> Disable /dev/console XON XOFF !!!\n");
			terminalAttributes.c_iflag = (terminalAttributes.c_iflag & ~(IXON | IXOFF));
			if(tcsetattr(fd, TCSADRAIN, &terminalAttributes) != 0){
				printf(">>> Fail disable /dev/console XON XOFF !!!\n");
			}
		}
		if(fd != -1)
			close(fd);
	}
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "mib.h"
#include "utility.h"
#include "subr_net.h"
#include "subr_ctcsdn.h"
#include "debug.h"

#define SDNCMD "/usr/bin/sdn_cmd"

int check_ovs_running(void)
{
	int ret=1;
	
	ret = va_cmd(SDNCMD, 1, 1, "check");
	
	return ret;
}

int check_ovs_enable(void)
{
	unsigned char enable = 0;
	mib_get_s(MIB_CTCSDN_ENABLE, &enable, sizeof(enable));
	if(enable) return 1;
	return 0;
}

int check_ovs_loglevel(void)
{
	unsigned char loglevel = 0;
	FILE *fp = NULL;
	
	mib_get_s(MIB_CTCSDN_LOGLEVEL, &loglevel, sizeof(loglevel));
	
	fp = fopen("/var/run/SDN_LOG_LEVEL", "w");
	if(fp)
	{
		fprintf(fp, "%hhu", loglevel);
		fclose(fp);
	}
	
	return 0;
}

typedef void (*sighandler_t)(int);
int get_ovs_datapath_id(char *value, int size)
{
	int ret = -1;
	FILE *fp;
	char buf[128] = {0}, *p;
	sighandler_t old_handler;
	
	if(value == NULL || size == 0)
		return -1;
	
	if(check_ovs_running() != 0)
		return -1;
	
	old_handler = signal(SIGCHLD, SIG_DFL);
	sprintf(buf, "%s get dpid", SDNCMD);
	fp = popen(buf, "r");
	if(fp)
	{
		fgets(value, size, fp);
		*(value+size-1) = '\0';
		p = strchr(value, '\n');
		if(p) *p = '\0';
		ret = pclose(fp);
		if(ret != 0) *value = '\0';
	}
	signal(SIGCHLD, old_handler);
	return ret;
}

int get_ovs_available_interfaces(OVS_OFPPORT_INTF *list, int list_size)
{
	int i, j, port_id, size=0;
	OVS_OFPPORT_INTF *intf;
	
	if(list_size <= 0 || list == NULL)
		return 0;
	
	intf = list;
	
	MIB_CE_SW_PORT_T portEntry;
	for(i=0; i<ELANVIF_NUM; i++)
	{
		if(!mib_chain_get(MIB_SW_PORT_TBL, i, (void *)&portEntry))
			continue;
		if(portEntry.enable == 0)
			continue;
		port_id = get_ovs_phyport_by_mibentry(OFP_PTYPE_LAN, &portEntry, i, 0);
		if(port_id > 0)
		{
			intf->portType = OFP_PTYPE_LAN;
			sprintf(intf->name, "LAN%d", i+1);
			strncpy(intf->ifname, ELANRIF[i], sizeof(intf->ifname));
			intf->ifname[sizeof(intf->ifname)-1] = '\0';
			intf->ofportID = port_id;
			intf->sdn_enable = portEntry.sdn_enable;
			if((++size) >= list_size) goto done;
			intf++;
		}
	}
#ifdef WLAN_SUPPORT	
	unsigned char phyband = 0, portType, wlanDisabled=0;
	char *alias_format=NULL;
	MIB_CE_MBSSIB_T wlanEntry;
	for(i=0; i<NUM_WLAN_INTERFACE; i++)
	{
		mib_local_mapping_get(MIB_WLAN_MODULE_DISABLED, i, (void *)&wlanDisabled);
		if(wlanDisabled) continue;
		
		mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, i, &phyband);
		if(phyband == PHYBAND_5G)
		{
			portType = OFP_PTYPE_WLAN5G;
			alias_format="5G-%d";
		}
		else
		{
			portType = OFP_PTYPE_WLAN2G;
			alias_format="2.4G-%d";
		}
		
		for(j=0; j<WLAN_SSID_NUM; j++) 
		{
			if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&wlanEntry))
				continue;
			if(wlanEntry.wlanDisabled)
				continue;
			port_id = get_ovs_phyport_by_mibentry(portType, &wlanEntry, j, i);
			if(port_id > 0)
			{
				intf->portType = portType;
				sprintf(intf->name, alias_format, j+1);
				rtk_wlan_get_ifname(i, j, intf->ifname);
				intf->ofportID = port_id;
				intf->sdn_enable = wlanEntry.sdn_enable;
				if((++size) >= list_size) goto done;
				intf++;
			}
		}	
	}
#endif
	int vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	MIB_CE_ATM_VC_T vcEntry;
	for(i=0; i<vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
			continue;
		if(vcEntry.enable == 0)
			continue;
		
		port_id = get_ovs_phyport_by_mibentry(OFP_PTYPE_WAN, &vcEntry, i, 0);
		if(port_id > 0)
		{
			intf->portType = OFP_PTYPE_WAN;
			getWanName(&vcEntry, intf->name);
			ifGetName(vcEntry.ifIndex,intf->ifname,sizeof(intf->ifname));
			intf->ofportID = port_id;
			intf->sdn_enable = vcEntry.sdn_enable;
			if((++size) >= list_size) goto done;
			intf++;
		}
	}
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	int l2tpTotal = mib_chain_total(MIB_L2TP_TBL);
    MIB_L2TP_T l2tpEntry;
	for(i=0; i<l2tpTotal; i++)
	{
		if (!mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tpEntry))
			continue;
		if(l2tpEntry.vpn_enable == VPN_DISABLE)
			continue;
		port_id = get_ovs_phyport_by_mibentry(OFP_PTYPE_VPN_L2TP, &l2tpEntry, i, 0);
		if(port_id > 0)
		{
			intf->portType = OFP_PTYPE_VPN_L2TP;
			strncpy(intf->name, l2tpEntry.tunnelName, sizeof(intf->name));
			intf->name[sizeof(intf->name)-1] = '\0';
			ifGetName(l2tpEntry.ifIndex,intf->ifname,sizeof(intf->ifname));
			intf->ofportID = port_id;
			intf->sdn_enable = l2tpEntry.sdn_enable;
			if((++size) >= list_size) goto done;
			intf++;
		}
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	int pptpTotal = mib_chain_total(MIB_PPTP_TBL);
    MIB_PPTP_T pptpEntry;
	for(i=0; i<pptpTotal; i++)
	{
		if (!mib_chain_get(MIB_PPTP_TBL, i, (void *)&pptpEntry))
			continue;
		if(pptpEntry.vpn_enable == VPN_DISABLE)
			continue;
		
		port_id = get_ovs_phyport_by_mibentry(OFP_PTYPE_VPN_PPTP, &pptpEntry, i, 0);
		if(port_id > 0)
		{
			intf->portType = OFP_PTYPE_VPN_PPTP;
			strncpy(intf->name, pptpEntry.tunnelName, sizeof(intf->name));
			intf->name[sizeof(intf->name)-1] = '\0';
			ifGetName(pptpEntry.ifIndex,intf->ifname,sizeof(intf->ifname));
			intf->ofportID = port_id;
			intf->sdn_enable = pptpEntry.sdn_enable;
			if((++size) >= list_size) goto done;
			intf++;
		}
	}
#endif

done:
	return size;
}

int get_ovs_phyport_by_mibentry(int portType, void *mib_entry, int mib_index, int mib_index2)
{
	int port_id = -1;
	
	if(mib_entry == NULL)
		return -1;
	
	switch(portType)
	{
		case OFP_PTYPE_LAN:
		{
			MIB_CE_SW_PORT_T *portEntry = (MIB_CE_SW_PORT_T *)mib_entry;
			port_id = mib_index+OFP_PID_LAN_START;
			if(port_id > OFP_PID_LAN_END)
				port_id = -1;
			break;
		}
#ifdef WLAN_SUPPORT	
		case OFP_PTYPE_WLAN2G:
		{
			MIB_CE_MBSSIB_T *wlanEntry = (MIB_CE_MBSSIB_T *)mib_entry;;
			port_id = mib_index+OFP_PID_WLAN2G_START;
			if(port_id > OFP_PID_WLAN2G_END)
				port_id = -1;
			break;
		}
		case OFP_PTYPE_WLAN5G:
		{
			MIB_CE_MBSSIB_T *wlanEntry = (MIB_CE_MBSSIB_T *)mib_entry;;
			port_id = mib_index+OFP_PID_WLAN5G_START;
			if(port_id > OFP_PID_WLAN5G_END)
				port_id = -1;
			break;
		}
#endif
		case OFP_PTYPE_WAN:
		{
			MIB_CE_ATM_VC_T *vcEntry = (MIB_CE_ATM_VC_T *)mib_entry;
			port_id = ETH_INDEX(vcEntry->ifIndex)+OFP_PID_WAN_START;
			if(port_id > OFP_PID_WAN_END)
				port_id = -1;
			break;
		}
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)		
		case OFP_PTYPE_VPN_L2TP:
		{
			MIB_L2TP_T *l2tpEntry = (MIB_L2TP_T *)mib_entry;
			port_id = l2tpEntry->idx+OFP_PID_VPN_L2TP_START;
			if(port_id > OFP_PID_VPN_L2TP_END)
				port_id = -1;
			break;
		}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)	
		case OFP_PTYPE_VPN_PPTP:
		{
			MIB_PPTP_T *pptpEntry = (MIB_PPTP_T *)mib_entry;
			port_id = pptpEntry->idx+OFP_PID_VPN_PPTP_START;
			if(port_id > OFP_PID_VPN_PPTP_END)
				port_id = -1;
			break;
		}
#endif
	}
	
	return port_id;
}

int find_ovs_interface_mibentry(OVS_OFPPORT_INTF *intf, void **mib_entry, int *mib_index, int *mib_index2)
{
	int i, j;
	if(intf == NULL || mib_entry == NULL)
		return -1;
	
	switch(intf->portType)
	{
		case OFP_PTYPE_LAN:
		{
			MIB_CE_SW_PORT_T portEntry;
			for(i=0; i<ELANVIF_NUM; i++)
			{
				if(strcmp(ELANRIF[i], intf->ifname))
					continue;
				if(mib_chain_get(MIB_SW_PORT_TBL, i, (void *)&portEntry) && portEntry.enable != 0)
				{
					*mib_entry = malloc(sizeof(MIB_CE_SW_PORT_T));
					if(*mib_entry)
					{
						memcpy(*mib_entry, &portEntry, sizeof(MIB_CE_SW_PORT_T));
						if(mib_index) *mib_index = i;
						return 0;
					}
				}
				return -1;
			}
			break;
		}
#ifdef WLAN_SUPPORT	
		case OFP_PTYPE_WLAN2G:
		case OFP_PTYPE_WLAN5G:
		{
			MIB_CE_MBSSIB_T wlanEntry;
			unsigned char phyband = 0, wlanDisabled = 0;
			char ifname[16] = {0};
			for(i=0; i<NUM_WLAN_INTERFACE; i++)
			{
				mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, i, &phyband);
				if(intf->portType == OFP_PTYPE_WLAN2G && phyband != PHYBAND_2G)
					continue;
				else if(intf->portType == OFP_PTYPE_WLAN5G && phyband != PHYBAND_5G)
					continue;
				mib_local_mapping_get(MIB_WLAN_MODULE_DISABLED, i, (void *)&wlanDisabled);
				if(wlanDisabled) continue;
				
				for(j=0; j<WLAN_SSID_NUM; j++) 
				{
					rtk_wlan_get_ifname(i, j, ifname);
					if(strcmp(ifname, intf->ifname))
						continue;
					if(mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&wlanEntry)
						&& wlanEntry.wlanDisabled == 0)
					{
						*mib_entry = malloc(sizeof(MIB_CE_MBSSIB_T));
						if(*mib_entry)
						{
							memcpy(*mib_entry, &wlanEntry, sizeof(MIB_CE_MBSSIB_T));
							if(mib_index) *mib_index = j;
							if(mib_index2) *mib_index2 = i;
							return 0;
						}
					}
					return -1;
				}
			}
			break;
		}
#endif
		case OFP_PTYPE_WAN:
		{
			int vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
			MIB_CE_ATM_VC_T vcEntry;
			char ifname[16] = {0};
			for(i=0; i<vcTotal; i++)
			{
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
					continue;
				
				ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
				if(strcmp(ifname, intf->ifname))
					continue;
				
				if(vcEntry.enable != 0)
				{
					*mib_entry = malloc(sizeof(MIB_CE_ATM_VC_T));
					if(*mib_entry)
					{
						memcpy(*mib_entry, &vcEntry, sizeof(MIB_CE_ATM_VC_T));
						if(mib_index) *mib_index = i;
						return 0;
					}
				}
				return -1;
			}
			break;
		}
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)		
		case OFP_PTYPE_VPN_L2TP:
		{
			int l2tpTotal = mib_chain_total(MIB_L2TP_TBL);
			MIB_L2TP_T l2tpEntry;
			char ifname[16] = {0};
			for(i=0; i<l2tpTotal; i++)
			{
				if (!mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tpEntry))
					continue;
				
				ifGetName(l2tpEntry.ifIndex, ifname, sizeof(intf->ifname));
				if(strcmp(ifname, intf->ifname))
					continue;
				
				if(l2tpEntry.vpn_enable != VPN_DISABLE)
				{
					*mib_entry = malloc(sizeof(MIB_L2TP_T));
					if(*mib_entry)
					{
						memcpy(*mib_entry, &l2tpEntry, sizeof(MIB_L2TP_T));
						if(mib_index) *mib_index = i;
						return 0;
					}
				}
				return -1;
			}
			break;
		}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)	
		case OFP_PTYPE_VPN_PPTP:
		{
			int pptpTotal = mib_chain_total(MIB_PPTP_TBL);
			MIB_PPTP_T pptpEntry;
			char ifname[16] = {0};
			for(i=0; i<pptpTotal; i++)
			{
				if (!mib_chain_get(MIB_PPTP_TBL, i, (void *)&pptpEntry))
					continue;
				
				ifGetName(pptpEntry.ifIndex, ifname, sizeof(intf->ifname));
				if(strcmp(ifname, intf->ifname))
					continue;
				
				if(pptpEntry.vpn_enable != VPN_DISABLE)
				{
					*mib_entry = malloc(sizeof(MIB_PPTP_T));
					if(*mib_entry)
					{
						memcpy(*mib_entry, &pptpEntry, sizeof(MIB_PPTP_T));
						if(mib_index) *mib_index = i;
						return 0;
					}
				}
				return -1;
			}
			break;
		}
#endif
	}
	return -1;
}

int check_ovs_interface_by_mibentry(int portType, void *mib_entry, int mib_index, int mib_index2)
{
	int port_id = -1, ret = -1;
	char ifname[16] = {0};
	char buf[256] = {0};
	
	if(mib_entry == NULL)
		return -1;
	
	port_id = get_ovs_phyport_by_mibentry(portType, mib_entry, mib_index, mib_index2);
	if(port_id <= 0) return -1;
	
	switch(portType)
	{
		case OFP_PTYPE_LAN:
		{
			MIB_CE_SW_PORT_T *portEntry = (MIB_CE_SW_PORT_T *)mib_entry;
			strcpy(ifname, ELANRIF[mib_index]);
			sprintf(buf, "%s checkport %s %d", SDNCMD, ifname, port_id);
			ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
			break;
		}
#ifdef WLAN_SUPPORT	
		case OFP_PTYPE_WLAN2G:
		case OFP_PTYPE_WLAN5G:
		{
			MIB_CE_MBSSIB_T *wlanEntry = (MIB_CE_MBSSIB_T *)mib_entry;;
			rtk_wlan_get_ifname(mib_index2, mib_index, ifname);
			sprintf(buf, "%s checkport %s %d", SDNCMD, ifname, port_id);
			ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
			break;
		}
#endif
		case OFP_PTYPE_WAN:
		{
			MIB_CE_ATM_VC_T *vcEntry = (MIB_CE_ATM_VC_T *)mib_entry;
			ifGetName(PHY_INTF(vcEntry->ifIndex), ifname, sizeof(ifname));
			sprintf(buf, "%s checkport %s %d", SDNCMD, ifname, port_id);
			ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
			break;
		}
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)		
		case OFP_PTYPE_VPN_L2TP:
		{
			MIB_L2TP_T *l2tpEntry = (MIB_L2TP_T *)mib_entry;
			break;
		}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)	
		case OFP_PTYPE_VPN_PPTP:
		{
			MIB_PPTP_T *pptpEntry = (MIB_PPTP_T *)mib_entry;
			break;
		}
#endif
	}
	return ret;
}

int check_ovs_interface(OVS_OFPPORT_INTF *intf)
{
	int mib_index, mib_index2, do_mib_update=0;
	void *mib_entry = NULL;
	int ret = -1;
	
	if(find_ovs_interface_mibentry(intf, &mib_entry, &mib_index, &mib_index2) == 0)
	{
		ret = check_ovs_interface_by_mibentry(intf->portType, mib_entry, mib_index, mib_index2) ;
	}

done:
	if(mib_entry != NULL)
		free(mib_entry);
	
	return ret;
}

int bind_ovs_interface_by_mibentry(int portType, void *mib_entry, int mib_index, int mib_index2)
{
	int port_id = -1, ret = -1;
	char ifname[16] = {0};
	char buf[1024] = {0}, *pbuf=NULL;
	
	if(mib_entry == NULL)
		return -1;
	
	port_id = get_ovs_phyport_by_mibentry(portType, mib_entry, mib_index, mib_index2);
	if(port_id <= 0) return -1;
	
	switch(portType)
	{
		case OFP_PTYPE_LAN:
		{
			MIB_CE_SW_PORT_T *portEntry = (MIB_CE_SW_PORT_T *)mib_entry;
			strcpy(ifname, ELANRIF[mib_index]);
			sprintf(buf, "%s checkport %s %d", SDNCMD, ifname, port_id);
			ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
			if(ret == 3)
			{
				va_cmd(IFCONFIG, 2, 1, ifname, "down");
#if defined(CONFIG_RTL_SMUX_DEV)
				va_cmd(IFCONFIG, 2, 1, ELANVIF[mib_index], "down");
				va_cmd(BRCTL, 2, 1, "delif", (char *)BRIF, ELANVIF[mib_index]);
				Deinit_LanPort_Def_Intf(mib_index);
#else
				va_cmd(IFCONFIG, 2, 1, ELANVIF[mib_index], "down");
				va_cmd(BRCTL, 2, 1, "delif", (char *)BRIF, ELANVIF[mib_index]);
#endif
				sprintf(buf, "%s bind LAN %s %d", SDNCMD, ifname, port_id);
				ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
				va_cmd(IFCONFIG, 2, 1, ifname, "up");
			}
			break;
		}
#ifdef WLAN_SUPPORT	
		case OFP_PTYPE_WLAN2G:
		case OFP_PTYPE_WLAN5G:
		{
			MIB_CE_MBSSIB_T *wlanEntry = (MIB_CE_MBSSIB_T *)mib_entry;;
			rtk_wlan_get_ifname(mib_index2, mib_index, ifname);
			sprintf(buf, "%s checkport %s %d", SDNCMD, ifname, port_id);
			ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
			if(ret == 3)
			{
				va_cmd(IFCONFIG, 2, 1, ifname, "down");
				va_cmd(BRCTL, 2, 1, "delif", (char *)BRIF, ifname);
				sprintf(buf, "%s bind WLAN %s %d", SDNCMD, ifname, port_id);
				ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
				va_cmd(IFCONFIG, 2, 1, ifname, "up");
			}
			break;
		}
#endif
		case OFP_PTYPE_WAN:
		{
			char *wanType;
			char ipaddr[64] = {0};
			MIB_CE_ATM_VC_T *vcEntry = (MIB_CE_ATM_VC_T *)mib_entry;
			ifGetName(PHY_INTF(vcEntry->ifIndex), ifname, sizeof(ifname));
			sprintf(buf, "%s checkport %s %d", SDNCMD, ifname, port_id);
			ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
			if(ret == 3)
			{
				deleteConnection(CONFIGONE, vcEntry);
				addEthWANdev(vcEntry);
				setup_ethernet_config(vcEntry, ifname);
#if defined(CONFIG_GPON_FEATURE) && defined (CONFIG_TR142_MODULE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
				rtk_tr142_sync_wan_info(vcEntry, 1);
#endif
#ifdef CONFIG_USER_RTK_MULTICAST_VID
				rtk_iptv_multicast_vlan_config(CONFIGONE, vcEntry);
#endif
				if(vcEntry->cmode == CHANNEL_MODE_BRIDGE)
					wanType = "bridge";
				else
					wanType = "routing";
				sprintf(buf, "%s bind WAN %s %d %s", SDNCMD, ifname, port_id, wanType);
				ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
				if(ret == 0)
				{
					if(vcEntry->cmode == CHANNEL_MODE_BRIDGE)
					{
						 ; //nothing to do, maybe in furture 
					}
					else if(vcEntry->cmode == CHANNEL_MODE_IPOE)
					{
						buf[0] = '\0';
						pbuf = buf;
						pbuf += sprintf(pbuf, "%s config WAN %s ipoe", SDNCMD, ifname);
						if(vcEntry->IpProtocol == IPVER_IPV4_IPV6) 
							pbuf += sprintf(pbuf, " proto ip46");
						else if(vcEntry->IpProtocol == IPVER_IPV6)
							pbuf += sprintf(pbuf, " proto ip6");
						else
							pbuf += sprintf(pbuf, " proto ip4");
						
						if(vcEntry->IpProtocol & IPVER_IPV4)
						{
							if(vcEntry->ipDhcp == DHCP_DISABLED)
							{
								pbuf += sprintf(pbuf, " ipmode static");
								ipaddr[0] = '\0';
								strncpy(ipaddr, (char*)inet_ntoa(*((struct in_addr *)vcEntry->ipAddr)), sizeof(ipaddr));
								pbuf += sprintf(pbuf, " ipaddr %s", ipaddr);
								ipaddr[0] = '\0';
								strncpy(ipaddr, (char*)inet_ntoa(*((struct in_addr *)vcEntry->remoteIpAddr)), sizeof(ipaddr));
								pbuf += sprintf(pbuf, " ipmask %s", ipaddr);
								ipaddr[0] = '\0';
								strncpy(ipaddr, (char*)inet_ntoa(*((struct in_addr *)vcEntry->netMask)), sizeof(ipaddr));
								pbuf += sprintf(pbuf, " ipgw %s", ipaddr);
								
								pbuf += sprintf(pbuf, " ipdns static");
								ipaddr[0] = '\0';
								strncpy(ipaddr, (char*)inet_ntoa(*((struct in_addr *)vcEntry->v4dns1)), sizeof(ipaddr));
								pbuf += sprintf(pbuf, " ipdns1 %s", ipaddr);
								ipaddr[0] = '\0';
								strncpy(ipaddr, (char*)inet_ntoa(*((struct in_addr *)vcEntry->v4dns2)), sizeof(ipaddr));
								pbuf += sprintf(pbuf, " ipdns2 %s", ipaddr);
							}
							else
							{
								pbuf += sprintf(pbuf, " ipmode dhcp");
								if(vcEntry->dnsMode)
									pbuf += sprintf(pbuf, " ipdns auto");
								else
								{
									pbuf += sprintf(pbuf, " ipdns static");
									ipaddr[0] = '\0';
									strncpy(ipaddr, (char*)inet_ntoa(*((struct in_addr *)vcEntry->v4dns1)), sizeof(ipaddr));
									pbuf += sprintf(pbuf, " ipdns1 %s", ipaddr);
									ipaddr[0] = '\0';
									strncpy(ipaddr, (char*)inet_ntoa(*((struct in_addr *)vcEntry->v4dns2)), sizeof(ipaddr));
									pbuf += sprintf(pbuf, " ipdns2 %s", ipaddr);
									
								}
							}
							
							pbuf += sprintf(pbuf, " ipnat %s", (vcEntry->napt == 1)?"on":"off");
						}
						
						if(vcEntry->IpProtocol & IPVER_IPV6)
						{
							;// no support
						}
						
						va_cmd("/bin/sh", 2, 1, "-c", buf);
					}
					else if(vcEntry->cmode == CHANNEL_MODE_PPPOE)
					{
						buf[0] = '\0';
						pbuf = buf;
						pbuf += sprintf(pbuf, "%s config WAN %s pppoe", SDNCMD, ifname);
						if(vcEntry->IpProtocol == IPVER_IPV4_IPV6) 
							pbuf += sprintf(pbuf, " proto ip46");
						else if(vcEntry->IpProtocol == IPVER_IPV6)
							pbuf += sprintf(pbuf, " proto ip6");
						else
							pbuf += sprintf(pbuf, " proto ip4");
						
						pbuf += sprintf(pbuf, " unit %d", PPP_INDEX(vcEntry->ifIndex));
						
						if(vcEntry->pppUsername[0])
							pbuf += sprintf(pbuf, " user %s", vcEntry->pppUsername);
						if(vcEntry->pppPassword[0])
							pbuf += sprintf(pbuf, " passwd %s", vcEntry->pppPassword);
						if(vcEntry->pppACName[0])
							pbuf += sprintf(pbuf, " acname %s", vcEntry->pppACName);
#ifdef CONFIG_SPPPD_STATICIP
						if (vcEntry->pppIp){
							pbuf += sprintf(pbuf, " ipmode static");
							ipaddr[0] = '\0';
							strncpy(ipaddr, (char*)inet_ntoa(*((struct in_addr *)vcEntry->ipAddr)), sizeof(ipaddr));
							pbuf += sprintf(pbuf, " ipaddr %s", ipaddr);
						}
						else
#endif
						{
							pbuf += sprintf(pbuf, " ipmode auto");
						}
						
						if(vcEntry->dnsMode)
							pbuf += sprintf(pbuf, " ipdns auto");
						else
						{
							pbuf += sprintf(pbuf, " ipdns static");
							ipaddr[0] = '\0';
							strncpy(ipaddr, (char*)inet_ntoa(*((struct in_addr *)vcEntry->v4dns1)), sizeof(ipaddr));
							pbuf += sprintf(pbuf, " ipdns1 %s", ipaddr);
							ipaddr[0] = '\0';
							strncpy(ipaddr, (char*)inet_ntoa(*((struct in_addr *)vcEntry->v4dns2)), sizeof(ipaddr));
							pbuf += sprintf(pbuf, " ipdns2 %s", ipaddr);
						}
						
						pbuf += sprintf(pbuf, " mtu %d", vcEntry->mtu);
						pbuf += sprintf(pbuf, " mru %d", vcEntry->mtu);
						
						if (vcEntry->pppCtype == CONNECT_ON_DEMAND)
						{
							pbuf += sprintf(pbuf, " conntype demand");
							pbuf += sprintf(pbuf, " idle %u", vcEntry->pppIdleTime);
						}
						else if (vcEntry->pppCtype == CONTINUOUS)	
						{
							pbuf += sprintf(pbuf, " conntype continous");
						}

						pbuf += sprintf(pbuf, " ipnat %s", (vcEntry->napt == 1)?"on":"off");
						
						va_cmd("/bin/sh", 2, 1, "-c", buf);
					}

				}
			}
			break;
		}
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)		
		case OFP_PTYPE_VPN_L2TP:
		{
			MIB_L2TP_T *l2tpEntry = (MIB_L2TP_T *)mib_entry;
			ifGetName(l2tpEntry->ifIndex,ifname,sizeof(ifname));
			sprintf(buf, "%s checkport %s %d", SDNCMD, ifname, port_id);
			ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
			if(ret == 3)
			{
				l2tpEntry->sdn_enable = 0; //to avoid call ovs unbind
				applyL2TP(l2tpEntry, 3, mib_index);
				l2tpEntry->sdn_enable = 1;
				
				sprintf(buf, "%s bind VPN %s %d l2tp", SDNCMD, ifname, port_id);
				ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
				if(ret == 0)
				{
					buf[0] = '\0';
					pbuf = buf;
					pbuf += sprintf(pbuf, "%s config VPN %s", SDNCMD, ifname);
					pbuf += sprintf(pbuf, " tunnel %s", l2tpEntry->tunnelName);
					pbuf += sprintf(pbuf, " server %s port %d", l2tpEntry->server, l2tpEntry->vpn_port);
					if(l2tpEntry->username[0])
					{
						pbuf += sprintf(pbuf, " user %s", l2tpEntry->username);
						if(l2tpEntry->password[0])
							pbuf += sprintf(pbuf, " passwd %s", l2tpEntry->password);
					}
					pbuf += sprintf(pbuf, " gw on ipnat on");
					pbuf += sprintf(pbuf, " mtu %d mru %d", l2tpEntry->mtu, l2tpEntry->mtu);
					if((l2tpEntry->conntype == CONNECT_ON_DEMAND || l2tpEntry->conntype == CONNECT_ON_DEMAND_PKT_COUNT) && l2tpEntry->idletime > 0)
					{
						pbuf += sprintf(pbuf, " conntype demand idle %d", l2tpEntry->idletime);
					}
					else
						pbuf += sprintf(pbuf, " conntype continous");
					
					va_cmd("/bin/sh", 2, 1, "-c", buf);
				}
			}
			break;
		}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)	
		case OFP_PTYPE_VPN_PPTP:
		{
			MIB_PPTP_T *pptpEntry = (MIB_PPTP_T *)mib_entry;
			ifGetName(pptpEntry->ifIndex,ifname,sizeof(ifname));
			sprintf(buf, "%s checkport %s %d", SDNCMD, ifname, port_id);
			ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
			if(ret == 3)
			{
				pptpEntry->sdn_enable = 0; //to avoid call ovs unbind
				applyPPtP(pptpEntry, 3, mib_index);
				pptpEntry->sdn_enable = 1;
				
				sprintf(buf, "%s bind VPN %s %d pptp", SDNCMD, ifname, port_id);
				ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
				if(ret == 0)
				{
					buf[0] = '\0';
					pbuf = buf;
					pbuf += sprintf(pbuf, "%s config VPN %s", SDNCMD, ifname);
					pbuf += sprintf(pbuf, " tunnel %s", pptpEntry->tunnelName);
					pbuf += sprintf(pbuf, " server %s port %d", pptpEntry->server, pptpEntry->vpn_port);
					if(pptpEntry->username[0])
					{
						pbuf += sprintf(pbuf, " user %s", pptpEntry->username);
						if(pptpEntry->password[0])
							pbuf += sprintf(pbuf, " passwd %s", pptpEntry->password);
					}
					pbuf += sprintf(pbuf, " gw on ipnat on");
					pbuf += sprintf(pbuf, " mtu 1420 mru 1420");
					if((pptpEntry->conntype == CONNECT_ON_DEMAND || pptpEntry->conntype == CONNECT_ON_DEMAND_PKT_COUNT) && pptpEntry->idletime > 0)
					{
						pbuf += sprintf(pbuf, " conntype demand idle %d", pptpEntry->idletime);
					}
					else
						pbuf += sprintf(pbuf, " conntype continous");
					
					va_cmd("/bin/sh", 2, 1, "-c", buf);
				}
			}
			break;
		}
#endif
	}
	
	return ret;
}

int bind_ovs_interface(OVS_OFPPORT_INTF *intf)
{
	int mib_index, mib_index2, do_mib_update=0;
	void *mib_entry = NULL;
	int ret = -1;
	
	if(find_ovs_interface_mibentry(intf, &mib_entry, &mib_index, &mib_index2) == 0)
	{
		switch(intf->portType)
		{
			case OFP_PTYPE_LAN:
			{
				MIB_CE_SW_PORT_T *portEntry = (MIB_CE_SW_PORT_T *)mib_entry;
				if(!portEntry->sdn_enable)
				{
					portEntry->sdn_enable = 1;
					if(mib_chain_update(MIB_SW_PORT_TBL, portEntry, mib_index))
						do_mib_update = 1;
					else 
						goto done;
				}
				
				if(bind_ovs_interface_by_mibentry(OFP_PTYPE_LAN, portEntry, mib_index, 0) == 0)
					ret = 0;
				else
				{
					if(do_mib_update)
					{
						portEntry->sdn_enable = 0;
						mib_chain_update(MIB_SW_PORT_TBL, portEntry, mib_index);
					}
					ret = -1;
				}
				break;
			}
#ifdef WLAN_SUPPORT	
			case OFP_PTYPE_WLAN2G:
			case OFP_PTYPE_WLAN5G:
			{
				MIB_CE_MBSSIB_T *wlanEntry = (MIB_CE_MBSSIB_T *)mib_entry;;
				if(!wlanEntry->sdn_enable)
				{
					wlanEntry->sdn_enable = 1;
					if(mib_chain_local_mapping_update(MIB_MBSSIB_TBL, mib_index2, wlanEntry, mib_index))
						do_mib_update = 1;
					else 
						goto done;
				}
				
				if(bind_ovs_interface_by_mibentry(intf->portType, wlanEntry, mib_index, mib_index2) == 0)
					ret = 0;
				else
				{
					if(do_mib_update)
					{
						wlanEntry->sdn_enable = 0;
						mib_chain_local_mapping_update(MIB_MBSSIB_TBL, mib_index2, wlanEntry, mib_index);
					}
					ret = -1;
				}
				break;
			}
#endif
			case OFP_PTYPE_WAN:
			{
				MIB_CE_ATM_VC_T *vcEntry = (MIB_CE_ATM_VC_T *)mib_entry;
				if(!vcEntry->sdn_enable)
				{
					vcEntry->sdn_enable = 1;
					if(mib_chain_update(MIB_ATM_VC_TBL, vcEntry, mib_index))
						do_mib_update = 1;
					else 
						goto done;
				}
				
				if(bind_ovs_interface_by_mibentry(OFP_PTYPE_WAN, vcEntry, mib_index, 0) == 0)
					ret = 0;
				else
				{
					if(do_mib_update)
					{
						vcEntry->sdn_enable = 0;
						mib_chain_update(MIB_ATM_VC_TBL, vcEntry, mib_index);
					}
					ret = -1;
				}
				break;
			}
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)		
			case OFP_PTYPE_VPN_L2TP:
			{
				MIB_L2TP_T *l2tpEntry = (MIB_L2TP_T *)mib_entry;
				if(!l2tpEntry->sdn_enable)
				{
					l2tpEntry->sdn_enable = 1;
					if(mib_chain_update(MIB_L2TP_TBL, l2tpEntry, mib_index))
						do_mib_update = 1;
					else 
						goto done;
				}
				
				if(bind_ovs_interface_by_mibentry(OFP_PTYPE_VPN_L2TP, l2tpEntry, mib_index, 0) == 0)
					ret = 0;
				else
				{
					if(do_mib_update)
					{
						l2tpEntry->sdn_enable = 0;
						mib_chain_update(MIB_L2TP_TBL, l2tpEntry, mib_index);
					}
					ret = -1;
				}
				break;
			}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)	
			case OFP_PTYPE_VPN_PPTP:
			{
				MIB_PPTP_T *pptpEntry = (MIB_PPTP_T *)mib_entry;
				if(!pptpEntry->sdn_enable)
				{
					pptpEntry->sdn_enable = 1;
					if(mib_chain_update(MIB_PPTP_TBL, pptpEntry, mib_index))
						do_mib_update = 1;
					else 
						goto done;
				}
				
				if(bind_ovs_interface_by_mibentry(OFP_PTYPE_VPN_PPTP, pptpEntry, mib_index, 0) == 0)
					ret = 0;
				else
				{
					if(do_mib_update)
					{
						pptpEntry->sdn_enable = 0;
						mib_chain_update(MIB_PPTP_TBL, pptpEntry, mib_index);
					}
					ret = -1;
				}
				break;
			}
#endif
		}
	}

done:
	if(mib_entry != NULL)
		free(mib_entry);
	
	return ret;
}

int unbind_ovs_interface_by_mibentry(int portType, void *mib_entry, int mib_index, int mib_index2, int dontConfig)
{
	int port_id = -1, ret = -1;
	char ifname[16] = {0};
	char buf[256] = {0};
	
	if(mib_entry == NULL)
		return -1;
	
	port_id = get_ovs_phyport_by_mibentry(portType, mib_entry, mib_index, mib_index2);
	if(port_id <= 0) return -1;
	
	switch(portType)
	{
		case OFP_PTYPE_LAN:
		{
			MIB_CE_SW_PORT_T *portEntry = (MIB_CE_SW_PORT_T *)mib_entry;
			strcpy(ifname, ELANRIF[mib_index]);
			sprintf(buf, "%s checkport %s %d", SDNCMD, ifname, port_id);
			ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
			if(ret == 0)
			{
				sprintf(buf, "%s unbind LAN %s %d", SDNCMD, ifname, port_id);
				ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
				if(ret == 0 && !dontConfig)
				{
					va_cmd(IFCONFIG, 2, 1, ifname, "down");
#if defined(CONFIG_RTL_SMUX_DEV)
					Init_LanPort_Def_Intf(mib_index);
					va_cmd(BRCTL, 3, 1, "addif", (char *)BRIF, ELANVIF[mib_index]);
					va_cmd(IFCONFIG, 2, 1, ELANVIF[mib_index], "up");
#else
					va_cmd(BRCTL, 3, 1, "addif", (char *)BRIF, ELANVIF[mib_index]);
					va_cmd(IFCONFIG, 2, 1, ELANVIF[mib_index], "up");
#endif
					va_cmd(IFCONFIG, 2, 1, ifname, "up");
				}
			}
			break;
		}
#ifdef WLAN_SUPPORT	
		case OFP_PTYPE_WLAN2G:
		case OFP_PTYPE_WLAN5G:
		{
			MIB_CE_MBSSIB_T *wlanEntry = (MIB_CE_MBSSIB_T *)mib_entry;;
			rtk_wlan_get_ifname(mib_index2, mib_index, ifname);
			sprintf(buf, "%s checkport %s %d", SDNCMD, ifname, port_id);
			ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
			if(ret == 0 )
			{
				sprintf(buf, "%s unbind WLAN %s %d", SDNCMD, ifname, port_id);
				ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
				if(ret == 0 && !dontConfig)
				{
					va_cmd(IFCONFIG, 2, 1, ifname, "down");
					va_cmd(BRCTL, 3, 1, "addif", (char *)BRIF, ifname);
					va_cmd(IFCONFIG, 2, 1, ifname, "up");
				}
			}
			break;
		}
#endif
		case OFP_PTYPE_WAN:
		{
			MIB_CE_ATM_VC_T *vcEntry = (MIB_CE_ATM_VC_T *)mib_entry;
			ifGetName(PHY_INTF(vcEntry->ifIndex), ifname, sizeof(ifname));
			sprintf(buf, "%s checkport %s %d", SDNCMD, ifname, port_id);
			ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
			if(ret == 0)
			{
				sprintf(buf, "%s unbind WAN %s %d", SDNCMD, ifname, port_id);
				ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
				if(ret == 0 && !dontConfig)
				{
					deleteConnection(CONFIGONE, vcEntry);
					restartWAN(CONFIGONE, vcEntry);
				}
			}
			break;
		}
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)		
		case OFP_PTYPE_VPN_L2TP:
		{
			MIB_L2TP_T *l2tpEntry = (MIB_L2TP_T *)mib_entry;
			int enable = 0;
			ifGetName(l2tpEntry->ifIndex, ifname, sizeof(ifname));
			sprintf(buf, "%s checkport %s %d", SDNCMD, ifname, port_id);
			ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
			if(ret == 0)
			{
				sprintf(buf, "%s unbind VPN %s %d", SDNCMD, ifname, port_id);
				ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
				if(ret == 0 && !dontConfig)
				{
					mib_get(MIB_L2TP_ENABLE, (void *)&enable);
					if (enable)
					{
						applyL2TP(l2tpEntry, 2, mib_index);
					}
				}
			}
			break;
		}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)	
		case OFP_PTYPE_VPN_PPTP:
		{
			MIB_PPTP_T *pptpEntry = (MIB_PPTP_T *)mib_entry;
			int enable = 0;
			ifGetName(pptpEntry->ifIndex, ifname, sizeof(ifname));
			sprintf(buf, "%s checkport %s %d", SDNCMD, ifname, port_id);
			ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
			if(ret == 0)
			{
				sprintf(buf, "%s unbind VPN %s %d", SDNCMD, ifname, port_id);
				ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
				if(ret == 0 && !dontConfig)
				{
					mib_get(MIB_PPTP_ENABLE, (void *)&enable);
					if (enable)
					{
						applyPPtP(pptpEntry, 2, mib_index);
					}
				}
			}
			break;
		}
#endif
	}
	
	return ret;
}

int unbind_ovs_interface(OVS_OFPPORT_INTF *intf)
{
	int mib_index, mib_index2, do_mib_update=0;
	void *mib_entry = NULL;
	int ret = -1;
	
	if(find_ovs_interface_mibentry(intf, &mib_entry, &mib_index, &mib_index2) == 0)
	{
		switch(intf->portType)
		{
			case OFP_PTYPE_LAN:
			{
				MIB_CE_SW_PORT_T *portEntry = (MIB_CE_SW_PORT_T *)mib_entry;
				if(portEntry->sdn_enable)
				{
					portEntry->sdn_enable = 0;
					if(mib_chain_update(MIB_SW_PORT_TBL, portEntry, mib_index))
						do_mib_update = 1;
					else 
						goto done;
				}
				
				if(unbind_ovs_interface_by_mibentry(OFP_PTYPE_LAN, portEntry, mib_index, 0, 0) == 0)
					ret = 0;
				else
				{
					if(do_mib_update)
					{
						portEntry->sdn_enable = 1;
						mib_chain_update(MIB_SW_PORT_TBL, portEntry, mib_index);
					}
					ret = -1;
				}
				break;
			}
#ifdef WLAN_SUPPORT	
			case OFP_PTYPE_WLAN2G:
			case OFP_PTYPE_WLAN5G:
			{
				MIB_CE_MBSSIB_T *wlanEntry = (MIB_CE_MBSSIB_T *)mib_entry;;
				if(wlanEntry->sdn_enable)
				{
					wlanEntry->sdn_enable = 0;
					if(mib_chain_local_mapping_update(MIB_MBSSIB_TBL, mib_index2, wlanEntry, mib_index))
						do_mib_update = 1;
					else 
						goto done;
				}
				
				if(unbind_ovs_interface_by_mibentry(intf->portType, wlanEntry, mib_index, mib_index2, 0) == 0)
					ret = 0;
				else
				{
					if(do_mib_update)
					{
						wlanEntry->sdn_enable = 1;
						mib_chain_local_mapping_update(MIB_MBSSIB_TBL, mib_index2, wlanEntry, mib_index);
					}
					ret = -1;
				}
				break;
			}
#endif
			case OFP_PTYPE_WAN:
			{
				MIB_CE_ATM_VC_T *vcEntry = (MIB_CE_ATM_VC_T *)mib_entry;
				if(vcEntry->sdn_enable)
				{
					vcEntry->sdn_enable = 0;
					if(mib_chain_update(MIB_ATM_VC_TBL, vcEntry, mib_index))
						do_mib_update = 1;
					else 
						goto done;
				}
				
				if(unbind_ovs_interface_by_mibentry(OFP_PTYPE_WAN, vcEntry, mib_index, 0, 0) == 0)
					ret = 0;
				else
				{
					if(do_mib_update)
					{
						vcEntry->sdn_enable = 1;
						mib_chain_update(MIB_ATM_VC_TBL, vcEntry, mib_index);
					}
					ret = -1;
				}
				break;
			}
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)		
			case OFP_PTYPE_VPN_L2TP:
			{
				MIB_L2TP_T *l2tpEntry = (MIB_L2TP_T *)mib_entry;
				if(l2tpEntry->sdn_enable)
				{
					l2tpEntry->sdn_enable = 0;
					if(mib_chain_update(MIB_L2TP_TBL, l2tpEntry, mib_index))
						do_mib_update = 1;
					else 
						goto done;
				}
				
				if(unbind_ovs_interface_by_mibentry(OFP_PTYPE_VPN_L2TP, l2tpEntry, mib_index, 0, 0) == 0)
					ret = 0;
				else
				{
					if(do_mib_update)
					{
						l2tpEntry->sdn_enable = 1;
						mib_chain_update(MIB_L2TP_TBL, l2tpEntry, mib_index);
					}
					ret = -1;
				}
				break;
			}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)	
			case OFP_PTYPE_VPN_PPTP:
			{
				MIB_PPTP_T *pptpEntry = (MIB_PPTP_T *)mib_entry;
				if(pptpEntry->sdn_enable)
				{
					pptpEntry->sdn_enable = 0;
					if(mib_chain_update(MIB_PPTP_TBL, pptpEntry, mib_index))
						do_mib_update = 1;
					else 
						goto done;
				}
				
				if(unbind_ovs_interface_by_mibentry(OFP_PTYPE_VPN_PPTP, pptpEntry, mib_index, 0, 0) == 0)
					ret = 0;
				else
				{
					if(do_mib_update)
					{
						pptpEntry->sdn_enable = 1;
						mib_chain_update(MIB_PPTP_TBL, pptpEntry, mib_index);
					}
					ret = -1;
				}
				break;
			}
#endif
		}
	}

done:
	if(mib_entry != NULL)
		free(mib_entry);
	
	return ret;
}

int config_ovs_port(int bind)
{
	int i, j;
	// LAN PORT
	MIB_CE_SW_PORT_T portEntry;
	for(i=0; i<ELANVIF_NUM; i++)
	{
		if(!mib_chain_get(MIB_SW_PORT_TBL, i, (void *)&portEntry))
			continue;
		if(portEntry.enable == 0 || portEntry.sdn_enable == 0)
			continue;

		if(bind)
			bind_ovs_interface_by_mibentry(OFP_PTYPE_LAN, &portEntry, i, 0);
		else
			unbind_ovs_interface_by_mibentry(OFP_PTYPE_LAN, &portEntry, i, 0, 0);
	}
#ifdef WLAN_SUPPORT
	// WLAN PORT
	unsigned char phyband = 0, portType, wlanDisabled=0;
	MIB_CE_MBSSIB_T wlanEntry;
	for(i=0; i<NUM_WLAN_INTERFACE; i++)
	{
		mib_local_mapping_get(MIB_WLAN_MODULE_DISABLED, i, (void *)&wlanDisabled);
		if(wlanDisabled) continue;
		mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, i, &phyband);
		if(phyband == PHYBAND_5G)
			portType = OFP_PTYPE_WLAN5G;
		else
			portType = OFP_PTYPE_WLAN2G;
		
		for(j=0; j<WLAN_SSID_NUM; j++) 
		{
			if(!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&wlanEntry))
				continue;
			if(wlanEntry.wlanDisabled || wlanEntry.sdn_enable == 0)
				continue;

			if(bind)
				bind_ovs_interface_by_mibentry(portType, &wlanEntry, j, i);
			else
				unbind_ovs_interface_by_mibentry(portType, &wlanEntry, j, i, 0);
		}	
	}
#endif
	// WAN PORT
	int vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	MIB_CE_ATM_VC_T vcEntry;
	for(i=0; i<vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
			continue;
		if(vcEntry.enable == 0 || vcEntry.sdn_enable == 0)
			continue;

		if(bind)
			bind_ovs_interface_by_mibentry(OFP_PTYPE_WAN, &vcEntry, i, 0);
		else
			unbind_ovs_interface_by_mibentry(OFP_PTYPE_WAN, &vcEntry, i, 0, 0);
	}
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	// VPN L2TP
	int l2tpTotal = mib_chain_total(MIB_L2TP_TBL);
	MIB_L2TP_T l2tpEntry;
	for(i=0; i<l2tpTotal; i++)
	{
		if (!mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tpEntry))
			continue;
		if(l2tpEntry.vpn_enable == VPN_DISABLE || l2tpEntry.sdn_enable == 0)
			continue;

		if(bind)
			bind_ovs_interface_by_mibentry(OFP_PTYPE_VPN_L2TP, &l2tpEntry, i, 0);
		else
			unbind_ovs_interface_by_mibentry(OFP_PTYPE_VPN_L2TP, &l2tpEntry, i, 0, 0);
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	// VPN PPTP
	int pptpTotal = mib_chain_total(MIB_PPTP_TBL);
	MIB_PPTP_T pptpEntry;
	for(i=0; i<pptpTotal; i++)
	{
		if(!mib_chain_get(MIB_PPTP_TBL, i, (void *)&pptpEntry))
			continue;
		if(pptpEntry.vpn_enable == VPN_DISABLE || pptpEntry.sdn_enable == 0)
			continue;

		if(bind)
			bind_ovs_interface_by_mibentry(OFP_PTYPE_VPN_PPTP, &pptpEntry, i, 0);
		else
			unbind_ovs_interface_by_mibentry(OFP_PTYPE_VPN_L2TP, &l2tpEntry, i, 0, 0);
	}
#endif
	
	return 0;
}

static char OVS_OUTPUT_R[]="OVS_OUTPUT_ROUTE";
int setup_controller_route_rule(int set)
{
	char ofpusewan[32]={0}, controlAddr[128]={0}, *ip, *cmd, buf[256];
	int sp1, sp2, port, ret, i, vcTotal;
	unsigned int mark, mask;
	MIB_CE_ATM_VC_T vcEntry;
	char ifName[IFNAMSIZ];
	
	if(!set){
		va_cmd(IPTABLES, 6, 1, "-t", "filter", "-D", "OUTPUT", "-j", OVS_OUTPUT_R);
		va_cmd(IPTABLES, 4, 1, "-t", "filter", "-F", OVS_OUTPUT_R);
		va_cmd(IPTABLES, 4, 1, "-t", "filter", "-X", OVS_OUTPUT_R);
	}
	else
	{
		mib_get_s(MIB_CTCSDN_CONADDR, controlAddr, sizeof(controlAddr));
		mib_get_s(MIB_CTCSDN_OFWAN, ofpusewan, sizeof(ofpusewan));
		
		if(!strcmp(ofpusewan, "tr069") && controlAddr[0] != '\0')
		{
			vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
			for(i=0; i<vcTotal; i++)
			{
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
					continue;
				if(vcEntry.enable == 0 || !(vcEntry.applicationtype & X_CT_SRV_TR069))
					continue;
				break;
			}

			ifGetName(vcEntry.ifIndex, ifName, sizeof(ifName));
			if(i != vcTotal && !rtk_net_get_wan_policy_route_mark(ifName, &mark, &mask))
			{
				va_cmd(IPTABLES, 4, 1, "-t", "filter", "-N", OVS_OUTPUT_R);
				va_cmd(IPTABLES, 6, 1, "-t", "filter", "-A", "OUTPUT", "-j", OVS_OUTPUT_R);

				sp1=sp2=port=-1;
				ret = sscanf(controlAddr, "tcp:%n%*[^:]%n:%d", &sp1, &sp2, &port);
				if(!(sp1 != -1 && sp2 != -1))
				{
					sp1=0;
					sp2=-1;
					ret = sscanf(controlAddr, "%*[^:]%n:%d", &sp2, &port);
				}
				ip = &controlAddr[0];
				if(sp2 > 0) *(ip+sp2) = '\0';
				if(sp1 > 0) ip = ip+sp1;
				
				if(ip[0]){
					cmd = &buf[0];
					cmd+= sprintf(cmd, "%s -t filter -A %s -d %s -p tcp", IPTABLES, OVS_OUTPUT_R, ip);
					if(port>0)
						cmd+= sprintf(cmd, " --dport %d", port);
					cmd+= sprintf(cmd, " -j MARK --set-mark 0x%x/0x%x", mark, mask);
					
					va_cmd("/bin/sh", 2, 1, "-c", buf);
				}
			}
		}
	}
	return 0;
}

int config_ovs_controller(void)
{
	int ret = -1;
	char ofpusewan[32]={0},controlAddr[128]={0},buf[256]={0};
	
	setup_controller_route_rule(0);
	
	if(check_ovs_enable() && check_ovs_running() == 0)
	{
		mib_get_s(MIB_CTCSDN_CONADDR, controlAddr, sizeof(controlAddr));
		mib_get_s(MIB_CTCSDN_OFWAN, ofpusewan, sizeof(ofpusewan));
		
		if(ofpusewan[0] != '\0' && controlAddr[0] != '\0')
		{
			if(!strcmp(ofpusewan, "internet") || !strcmp(ofpusewan, "tr069"))
			{
				setup_controller_route_rule(1);
					
				if(!strncmp(controlAddr, "tcp:", 4))
					sprintf(buf, "%s set controller %s", SDNCMD, controlAddr);
				else
					sprintf(buf, "%s set controller tcp:%s", SDNCMD, controlAddr);
				
				ret = va_cmd("/bin/sh", 2, 1, "-c", buf);
			}
		}
	}
	
	return ret;
}

int stop_ovs_sdn(void)
{	
	int ret = -1;
	
	if(check_ovs_running() != 0)
		return 0;
	
	printf("==> Stop SDN <==\n");
	
	config_ovs_port(0);
	
	ret = va_cmd(SDNCMD, 1, 1, "stop");
	
	return ret;
}

int start_ovs_sdn(void)
{
	int ret = -1;

	if(check_ovs_enable())
	{
		check_ovs_loglevel();
		
		if(check_ovs_running() == 0)
		{
			ret = 0;
		}
		else
		{
			printf("==> Start SDN <==\n");
			
			ret = va_cmd(SDNCMD, 1, 1, "start");
			if(ret == 0)
			{
				config_ovs_port(1);
				
				config_ovs_controller();
			}
		}
	}

	return ret;
}

int config_ovs_sdn(int configMode)
{
	stop_ovs_sdn();
	
	start_ovs_sdn();
	
	return 0;
}


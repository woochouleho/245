#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mib.h"
#include "sysconfig.h"
#include "utility.h"
#if defined(CONFIG_LUNA) && defined(CONFIG_RTL8686_SWITCH)
#include <common/error.h>
#include <common/util/rt_util.h>
#ifdef CONFIG_COMMON_RT_API
#include <rtk/rt/rt_switch.h>
#include <rtk/rt/rt_port.h>
#elif defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
#include <rtk/ponmac.h>
#include <rtk/gpon.h>
#include <rtk/epon.h>
#include <rtk/switch.h>
#else
#include <rtk/switch.h>
#endif
#endif // defined(CONFIG_LUNA) && defined(CONFIG_RTL8686_SWITCH)

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#include "fc_api.h"
#endif

#include "subr_switch.h"

#if defined(CONFIG_EXTERNAL_PHY_POLLING)
#ifdef CONFIG_COMMON_RT_API
#include <rtk/rt/rt_mdio.h>
#else
#include <rtk/mdio.h>
#endif
#endif
#if defined(CONFIG_CMCC) && defined(CONFIG_RTL9607C_SERIES)
#include <dal/rtl9607c/dal_rtl9607c_switch.h>
#endif

#ifdef CONFIG_USER_RTK_EXTGPHY
#include <lib_extgphy.h>
#endif

#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN

/*
 *	Get a physical port by logic port.
 */
int rtk_port_get_phyPort(int logicPort) 
{
	unsigned char port_map_tbl[MAX_LAN_PORT_NUM];
	
	if (logicPort < 0 || logicPort > MAX_LAN_PORT_NUM-1) {
		printf("\n%s:%d Invalid logic port ID %d\n", __FUNCTION__, __LINE__, logicPort);
		return -1;
	}	
	
	if (!mib_get_s(MIB_PORT_REMAPPING, (void *)port_map_tbl, sizeof(port_map_tbl))) {
		printf("\n%s:%d mib_get_s MIB_PORT_REMAPPING fail!!!\n", __FUNCTION__, __LINE__);
		return -1;		
	}		

	return port_map_tbl[logicPort];
}

/*
 *	Get a physical port mask by logic port.
 */
unsigned int rtk_port_get_phyPortMask(int logicPort) 
{
	int phy_port = rtk_port_get_phyPort(logicPort);
	
	if (phy_port < 0)
		return 0;

	return (1 << phy_port);
}

/*
 *	Check a logic port whether is WAN port.
 */
int rtk_port_is_wan_logic_port(int logicPort)
{
	int vcTotal, i;
	MIB_CE_ATM_VC_T entry;

	if (getOpMode() == BRIDGE_MODE)
		return 0;

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);	
	for (i = 0; i < vcTotal; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
			continue;
		if (entry.enable == 0)
			continue;
		if (entry.logic_port == logicPort) 
			break;		
	}

	return (i < vcTotal ? 1 : 0);
}

/*
 *	Check a logic port attach WAN port number.
 */
int rtk_logic_port_attach_wan_num(int logicPort)
{
	int vcTotal, i, ret=0;
	MIB_CE_ATM_VC_T entry;

	if (getOpMode() == BRIDGE_MODE)
		return 0;

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);	
	for (i = 0; i < vcTotal; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
			continue;
		if (entry.enable == 0)
			continue;
		if (entry.logic_port == logicPort) 
			ret++;		
	}

	return ret;
}

/*
 *	Check a physical port whether is WAN port.
 */
int rtk_port_is_wan_phy_port(int phyPort)
{
	if (getOpMode() == BRIDGE_MODE)
		return 0;
	
	unsigned int phyport_mask = rtk_port_get_wan_phyMask(); 
	if ((1 << phyPort) & phyport_mask)
		return 1;
	else
		return 0;
}

/*
 *	Get a LAN physical port by logic port.
 */
int rtk_port_get_lan_phyID(int logPortId)
{
	return rtk_port_get_phyPort(logPortId);
}

unsigned int rtk_port_get_lan_phyMask(unsigned int portmask)
{
	int i, phy_port;
	unsigned int phyport_mask = 0;
	
	for (i = 0; i < SW_PORT_NUM; i++) {
		if ((portmask >> i) & 1) {
			if (rtk_port_is_wan_logic_port(i))
				continue;

			phy_port = rtk_port_get_phyPort(i);
			if (phy_port < 0)
				continue;
			
			phyport_mask |= (1 << phy_port);
		}		
	}
	return phyport_mask;
}

/*
 *	Get all physical LAN port mask.
 */
unsigned int rtk_port_get_all_lan_phyMask(void)
{
#ifdef CONFIG_LAN_PORT_NUM
	unsigned int allLanPortMask = ((1 << SW_PORT_NUM) - 1);
	return rtk_port_get_lan_phyMask(allLanPortMask);
#else
#ifdef CONFIG_RTL9602C_SERIES
	return rtk_port_get_lan_phyMask(0x3);
#else
	return rtk_port_get_lan_phyMask(0xf);
#endif
#endif
}

/*
 *	Get physical port by dev interface name.
 *	The argument ifname is eth0.x/eth0.x.0/nas0_x/pppx.
 */
int rtk_port_get_phyport_by_ifname(char *ifname)
{
	int vcTotal, i;
	char tmp_ifname[IFNAMSIZ] = {0};
	MIB_CE_ATM_VC_T entry;
	
	if (ifname == NULL)
		return -1;
	
	if (strncmp(ifname, ALIASNAME_MWNAS, strlen(ALIASNAME_MWNAS)) == 0 || 
		strncmp(ifname, ALIASNAME_PPP, strlen(ALIASNAME_PPP)) == 0) {
		
		vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < vcTotal; i++) {
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
				continue;
			
			if (entry.enable == 0)
				continue;

			ifGetName(entry.ifIndex, tmp_ifname, sizeof(tmp_ifname));
			if (strcmp(ifname, tmp_ifname) == 0) 
				break;		
		}
		if (i >= vcTotal) 
			return -1;			
		else
			return rtk_port_get_phyPort(entry.logic_port);		
	}

	for (i = 0; i < SW_PORT_NUM; i++) {
		if (strcmp(ifname, ELANRIF[i]) == 0 || strcmp(ifname, ELANVIF[i]) == 0) 
			return rtk_port_get_phyPort(i); 		
	}

	return -1;	
}

/*
 *	Check logic port whether is LAN port and have been bind to WAN.
 */
int rtk_port_is_bind_to_wan(int logicPort)
{
	int vcTotal, i;
	unsigned int itfGroup;
	MIB_CE_ATM_VC_T entry;
	
	if (rtk_port_is_wan_logic_port(logicPort))
		return 0;
	
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < vcTotal; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
			continue;		
		if (entry.enable == 0)
			continue;
		
		itfGroup = entry.itfGroup;
		if ((itfGroup & (1 << logicPort)) != 0) 
			break;		
	}

	return (i < vcTotal ? 1 : 0);	
}

/*
 *	Update WAN physical port mask when add/delete WAN.
 */
void rtk_port_update_wan_phyMask(void)
{
	int vcTotal, i, phy_port;	
	unsigned int phyport_mask = 0;
	MIB_CE_ATM_VC_T entry;	

	if (getOpMode() == GATEWAY_MODE) {	
		
		vcTotal = mib_chain_total(MIB_ATM_VC_TBL);		
		for (i = 0; i < vcTotal; i++) {
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
				continue;		
			if (entry.enable == 0)
				continue;

			phy_port = rtk_port_get_phyPort(entry.logic_port);
			if(phy_port < 0)
				continue;
			
			phyport_mask |= (1 << phy_port);
		}
	}
	
	mib_set(MIB_WAN_PHY_PORT_MASK, (void *)&phyport_mask);

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	setup_wan_port();

	return;
}

/*
 *	Update interface list of bridge (br0).
 *	When add/delete WAN, it should call this function to update bridge interface list.
 */
void rtk_update_br_interface_list(void)
{
	int i;
	unsigned char value[6] = {0};
	unsigned char macaddr[13] = {0};
	
	for (i = 0; i < SW_PORT_NUM; i++) {
		if (rtk_port_is_wan_logic_port(i)) { //wan logic port
			if (rtk_is_interface_in_bridge((char *)ELANVIF[i], (char *)BRIF)) {
				va_cmd(BRCTL, 3, 1, "delif", BRIF, ELANVIF[i]);
				
				rtk_update_lan_virtual_interface(i, 0);
			}
		} else { //lan logic port
			if (!rtk_is_interface_in_bridge((char *)ELANVIF[i], (char *)BRIF)) {					

				rtk_update_lan_virtual_interface(i, 1);

				mib_get_s(MIB_ELAN_MAC_ADDR, (void *)value, sizeof(value));
				snprintf(macaddr, sizeof(macaddr), "%02x%02x%02x%02x%02x%02x",
				value[0], value[1], value[2], value[3], value[4], value[5]);			
				va_cmd(IFCONFIG, 4, 1, ELANVIF[i], "hw", "ether", macaddr);
				va_cmd(BRCTL, 3, 1, "addif", BRIF, ELANVIF[i]);	
				va_cmd(IFCONFIG, 2, 1, ELANVIF[i], "up");
			}
		}
	}
	return;
}

/*
 *	Get default WAN physical port mask.	
 */
unsigned int rtk_port_get_default_wan_phyMask(void)
{
	unsigned int phymask = 0;	

	if(mib_get_def(MIB_WAN_PHY_PORT_MASK, (void *)&phymask, sizeof(phymask)))
		return phymask;
	else
		return 0;		
}

/*
 *	Get default WAN logic port.	
 */
int rtk_port_get_default_wan_logicPort(void)
{
	int i;
	unsigned int wanPhyPortMask = rtk_port_get_default_wan_phyMask();
	if (wanPhyPortMask == 0)
		return -1;	
		
	for (i = 0; i< SW_PORT_NUM; i++) {
		if (wanPhyPortMask == rtk_port_get_phyPortMask(i))
			break;
	}
	return (i < SW_PORT_NUM ? i : -1);		
}
/*
 *	Get a logical port by physical port.
 */
int rtk_port_get_logPort(int phyPort) 
{
	unsigned char port_map_tbl[MAX_LAN_PORT_NUM];
	int i;
	if(phyPort<0)
		return -1;
	if (!mib_get_s(MIB_PORT_REMAPPING, (void *)port_map_tbl, sizeof(port_map_tbl))) {
		printf("\n%s:%d mib_get_s MIB_PORT_REMAPPING fail!!!\n", __FUNCTION__, __LINE__);
		return -1;		
	}		
	for(i=0;i<MAX_LAN_PORT_NUM;i++){
		if(phyPort==port_map_tbl[i])
			return i;
	}
	return -1;
}

int rtk_check_port_remapping(void)
{	
	int i, def_phyport = 0;
	unsigned int def_phymask = 0;
	unsigned char def_remap_tbl[MAX_LAN_PORT_NUM];
	unsigned char remap_tbl[MAX_LAN_PORT_NUM];

	if (!mib_get_def(MIB_WAN_PHY_PORT_MASK, (void *)&def_phymask, sizeof(def_phymask))) 
		return RTK_FAILED;
	
	if (!mib_get_def(MIB_PORT_REMAPPING, (void *)def_remap_tbl, sizeof(def_remap_tbl)))
		return RTK_FAILED;

	if (!mib_get_s(MIB_PORT_REMAPPING, (void *)remap_tbl, sizeof(remap_tbl)))
		return RTK_FAILED;	

	if (def_phymask < 1)
		return RTK_FAILED;

	def_phyport = ffs(def_phymask) -1;

	for (i = 0; i < MAX_LAN_PORT_NUM; i++)
		if (remap_tbl[i] == def_phyport) 
			break;

	if (i < MAX_LAN_PORT_NUM)
		return RTK_SUCCESS;

	for (i = 0; i < CONFIG_LAN_PORT_NUM; i++)
		if (remap_tbl[i] != def_remap_tbl[i])
			break;

	if (i < CONFIG_LAN_PORT_NUM)
		return RTK_SUCCESS;	
	
	remap_tbl[CONFIG_LAN_PORT_NUM] = def_phyport;
	mib_set(MIB_PORT_REMAPPING, (void *)remap_tbl);
	mib_update_ex(HW_SETTING, CONFIG_MIB_TABLE, 1);

	return RTK_SUCCESS;	
}


/*
 *	Get a logical port by root interface name.
 *	The argument ifname is eth0.2/eth0.3/eth0.4/...
 */
int rtk_port_get_logicPort_by_root_ifname(char *ifname)
{
	int i;
	for (i = 0; i < SW_LAN_PORT_NUM; i++)
		if (strcmp(ifname, ELANRIF[i]) == 0)
			break;

	if (i >= SW_LAN_PORT_NUM)
		return -1;

	return i;
}

#else

int rtk_port_get_lan_phyID(int logPortId)
{
	int phyPortId, ret;
    unsigned char re_map_tbl[MAX_LAN_PORT_NUM];
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	unsigned char phy_pmapping[MAX_LAN_PORT_NUM];
	mib_get_s(MIB_HW_PHY_PORTMAPPING, (void *)phy_pmapping, sizeof(phy_pmapping));
#endif

    mib_get_s(MIB_PORT_REMAPPING, (void *)re_map_tbl, sizeof(re_map_tbl));

	if (logPortId < 0 || logPortId > MAX_LAN_PORT_NUM-1) {
		printf("\n%s:%d Invalid logic port ID %d\n", __FUNCTION__, __LINE__, logPortId);
		return -1;
	}
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	if(logPortId >= MAX_LAN_PORT_NUM){
		ret = -1;
	}
	else{
		ret = 0;
		phyPortId = phy_pmapping[re_map_tbl[logPortId]];
	}
#elif CONFIG_USER_RTK_EXTGPHY
	if(re_map_tbl[logPortId] != 6)	//PHY port 6 as external phy
	{
#ifdef CONFIG_COMMON_RT_API
		ret = rt_switch_phyPortId_get(re_map_tbl[logPortId], &phyPortId);
#else
		ret = -1;
#endif
	}
	else
	{
		ret = 0;
		phyPortId = re_map_tbl[logPortId];
	}
#else
#ifdef CONFIG_COMMON_RT_API
	ret = rt_switch_phyPortId_get(re_map_tbl[logPortId], &phyPortId);
#else
	ret = -1;
#endif
#endif	//CONFIG_USER_SUPPORT_EXTERNAL_SWITCH

	if(ret == 0)
		return phyPortId;
	else{
#ifdef CONFIG_COMMON_RT_API
		DBPRINT(1, "%s RT API get failed!\n", __FUNCTION__);
#endif
		return -1;
	}
}

unsigned int rtk_port_get_lan_phyMask(unsigned int portmask)
{
	int i=0, phyPortId, ret;
	unsigned int phyportmask=0;
	int ethPhyPortId = rtk_port_get_wan_phyID();

	for(i=0;i<ELANVIF_NUM;i++)
	{
		if((portmask>>i) & 1){
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
			if(rtk_port_check_external_port(i)){
				phyPortId = rtk_fc_external_switch_rgmii_port_get();
				ret == 0;
			}
			else
#endif
			{
				phyPortId = rtk_port_get_lan_phyID(i);
			}
			if(phyPortId != -1)
			{
				if(phyPortId == ethPhyPortId)
					continue;
				phyportmask |= (1 << phyPortId);
			}
			else
				DBPRINT(1, "%s RT API get failed! i=%d\n", __FUNCTION__, i);
		}
	}

	return phyportmask;
}

unsigned int rtk_port_get_all_lan_phyMask(void)
{
#ifdef CONFIG_LAN_PORT_NUM
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	unsigned int allLanPortMask = ((1 << CONFIG_LAN_PORT_NUM+CONFIG_EXTERNAL_SWITCH_LAN_PORT_NUM) - 1);
#elif CONFIG_USER_RTK_EXTGPHY
	unsigned int allLanPortMask = ((1<<CONFIG_LAN_PORT_NUM+1)-1);
#else
	unsigned int allLanPortMask = ((1<<CONFIG_LAN_PORT_NUM)-1);
#endif
	return rtk_port_get_lan_phyMask(allLanPortMask);
#else
#ifdef CONFIG_RTL9602C_SERIES
	return rtk_port_get_lan_phyMask(0x3);
#else
	return rtk_port_get_lan_phyMask(0xf);
#endif
#endif
}

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
int rtk_external_port_get_lan_phyID(int logPortId)
{
	int phyPortId = -1;
	if(rtk_port_check_external_port(logPortId)){
    	phyPortId = rtk_port_get_lan_phyID(logPortId) - EXTERNAL_SWITCH_PORT_OFFSET;
	}
	
	return phyPortId;
}

unsigned int rtk_port_get_external_lan_phyMask(unsigned int portmask)
{
	int i=0, phyPortId;
	unsigned int phyportmask=0;
	int ethPhyPortId = rtk_port_get_wan_phyID();

	for(i=0;i<ELANVIF_NUM;i++)
	{
		if(((portmask)>>i) & 1){
			if(rtk_port_check_external_port(i))
			{
				phyPortId = rtk_port_get_lan_phyID(i);
				if(phyPortId != -1)
				{
					if(phyPortId == ethPhyPortId)
						continue;
					phyportmask |= (1 << phyPortId);
				}
				else
					DBPRINT(1, "%s RT API get failed! i=%d\n", __FUNCTION__, i);
			}
		}
	}

	return phyportmask;
}

unsigned int rtk_port_get_all_external_lan_phyMask(void)
{
#if defined(CONFIG_EXTERNAL_SWITCH_LAN_PORT_NUM) && defined(CONFIG_LAN_PORT_NUM)
	unsigned int allExtLanPortMask = ((1<<CONFIG_EXTERNAL_SWITCH_LAN_PORT_NUM)-1)<<CONFIG_LAN_PORT_NUM;

	return rtk_port_get_external_lan_phyMask(allExtLanPortMask);
#else
	return 0;
#endif
}
#endif
int rtk_port_get_wan_phyID(void)
{
	int wanPhyPort = -1, logPortId=-1;
#if defined(CONFIG_LUNA)	
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	int pon_mode;
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
	if(pon_mode != ETH_MODE)
	{
#ifdef CONFIG_COMMON_RT_API
		logPortId = LOG_PORT_PON;
#endif		
	}
#elif defined(CONFIG_ETHWAN_USE_USB_SGMII)
#if defined(CONFIG_COMMON_RT_API)
	logPortId = LOG_PORT_HSG0;
#endif		
#elif defined(CONFIG_ETHWAN_USE_PCIE1_SGMII)
#if defined(CONFIG_COMMON_RT_API)
	logPortId = LOG_PORT_HSG1;
#endif	
#endif

	if(logPortId != -1)
	{
#if defined(CONFIG_COMMON_RT_API)
		if(rt_switch_phyPortId_get(logPortId, &wanPhyPort) != 0)
			wanPhyPort = -1;
#endif
	}

	if(wanPhyPort < 0 && mib_get_s(MIB_WAN_PHY_PORT,(void *)&wanPhyPort, sizeof(wanPhyPort)))
	{
		return wanPhyPort;
	}
#endif
	return wanPhyPort;
}

unsigned int rtk_port_get_wan_phyMask(void)
{
	unsigned int phyportmask=0;
	int phyPortId;	

	phyPortId = rtk_port_get_wan_phyID();

	if(phyPortId != -1)
		phyportmask |= (1 << phyPortId);
	else
		DBPRINT(1, "%s rtk_port_get_wan_phyID failed!\n", __FUNCTION__);

	return phyportmask;
}
#endif
unsigned int rtk_port_get_cpu_phyMask(void)
{
        unsigned int phyportmask=0;

#ifdef CONFIG_COMMON_RT_API
        int phyPortId, i=0, len;
        int logPortIds[] = {LOG_PORT_CPU0, LOG_PORT_CPU1, LOG_PORT_CPU2, LOG_PORT_CPU3,
                                                LOG_PORT_CPU4, LOG_PORT_CPU5, LOG_PORT_CPU6, LOG_PORT_CPU7};
        len = sizeof(logPortIds)/sizeof(int);
        for(i=0;i<len;i++)
        {
                if(rt_switch_phyPortId_get(logPortIds[i], &phyPortId) == 0)
                {
                        phyportmask |= 1<<phyPortId;
                }
        }
#endif
        return phyportmask;
}

int rtk_port_get_lan_phyStatus(int logPortId, int *status)
{
#if defined(CONFIG_COMMON_RT_API)
	rt_port_linkStatus_t rtPortStatus;
	int phyPortId;
#endif
	int ret;

	if(status == NULL)
	{
		DBPRINT(1, "%s status=NULL !\n", __FUNCTION__);
		return -1;
	}

	if(logPortId >= MAX_LAN_PORT_NUM)
	{
		DBPRINT(1, "%s invalid logPortId=%d !\n", __FUNCTION__, logPortId);
		return -1;
	}

#ifdef CONFIG_COMMON_RT_API
	phyPortId = rtk_port_get_lan_phyID(logPortId);
	if(phyPortId == -1)
	{
		*status = 0;
		DBPRINT(1, "%s rtk_port_get_lan_phyID get failed! logPortId=%d\n", __FUNCTION__, logPortId);
		return -1;
	}
	ret = rt_port_link_get(phyPortId, &rtPortStatus);
	if(rtPortStatus == RT_PORT_LINKUP)
		*status = 1;
	else
		*status = 0;
#else
	return -1;
#endif

	if(ret == 0)
	{
		return 0;
	}
	else
	{
		DBPRINT(1, "%s RG/RT API get failed! logPortId=%d\n", __FUNCTION__, logPortId);
		return -1;
	}
}

int rtk_port_get_lan_link_info(int logPortId, struct net_link_info *netlink_info)
{
#if defined(CONFIG_COMMON_RT_API)
	rt_port_speed_t  rtSpeed;
	rt_port_duplex_t rtDuplex;
	int phyPortId;
	int ret;
#endif

	if(netlink_info == NULL)
	{
		DBPRINT(1, "%s netlink_info=NULL !\n", __FUNCTION__);
		return -1;
	}

	if(logPortId >= MAX_LAN_PORT_NUM)
	{
		DBPRINT(1, "%s invalid logPortId=%d !\n", __FUNCTION__, logPortId);
		return -1;
	}

#ifdef CONFIG_COMMON_RT_API
	phyPortId = rtk_port_get_lan_phyID(logPortId);
	if(phyPortId == -1)
	{
		DBPRINT(1, "%s rtk_port_get_lan_phyID get failed! logPortId=%d\n", __FUNCTION__, logPortId);
		return -1;
	}
	rtSpeed = 0;
	rtDuplex = 0;
	if((ret = rt_port_speedDuplex_get(phyPortId, &rtSpeed, &rtDuplex))== RT_ERR_OK)
	{
		switch (rtDuplex){
			case RT_PORT_HALF_DUPLEX:
				netlink_info->duplex = 0;
				break;
			case RT_PORT_FULL_DUPLEX:
				netlink_info->duplex = 1;
				break;
		}
		switch (rtSpeed){
			case RT_PORT_SPEED_10M:
				netlink_info->speed = 10;
				break;
			case RT_PORT_SPEED_100M:
				netlink_info->speed = 100;
				break;
			case RT_PORT_SPEED_1000M:
				netlink_info->speed = 1000;
				break;
			case RT_PORT_SPEED_500M:
				netlink_info->speed = 500;
				break;
		}
		return 0;
	}
	else
	{
		DBPRINT(1, "%s RG/RT API get failed! logPortId=%d\n", __FUNCTION__, logPortId);
		return -1;
	}
#else
	return -1;
#endif
}

int rtk_if_name_get_by_lan_phyID(unsigned int portId, unsigned char *ifName)
{
	int i=0, phyPortId;

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	if(!(rtk_port_get_all_lan_phyMask()&(1<<portId)) && !(rtk_port_get_all_external_lan_phyMask()&(1<<portId)))
#else
	if(!(rtk_port_get_all_lan_phyMask()&(1<<portId)))
#endif
		return -1;

	for(i=0;i<ELANVIF_NUM;i++)
	{
		phyPortId = rtk_port_get_lan_phyID(i);
		if(phyPortId != -1 && phyPortId == portId)
		{
			if(ifName != NULL) sprintf(ifName, "%s", ELANVIF[i]);
			return i;
		}
	}
	return -1;
}

int rtk_lan_phyID_get_by_if_name(unsigned char *ifName)
{
	int i=0, j=0, phyPortId, ret;	
	unsigned char wlanIfName[16];

	if(ifName == NULL)
		return -1;

#if defined(CONFIG_USER_RTK_BRIDGE_MODE) && !defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
	if (strcmp(ifName, ALIASNAME_NAS0) == 0 && getOpMode() == BRIDGE_MODE)
		return rtk_port_get_wan_phyID();
#endif
	
	for(i=0;i<ELANVIF_NUM;i++)
	{
		phyPortId = rtk_port_get_lan_phyID(i);
		if(phyPortId != -1 && (!strcmp(ifName, ELANVIF[i]) || !strcmp(ifName, ELANRIF[i])))
		{
			return phyPortId;
		}
	}

#ifdef WLAN_SUPPORT
	for (i=0 ; i<NUM_WLAN_INTERFACE ; i++)
	{
#if defined(CONFIG_COMMON_RT_API)
		if(rt_switch_phyPortId_get(LOG_PORT_CPU0, &phyPortId) != 0)
	    {
	    	continue;
		}
#else
		phyPortId = 0xff;
#endif
		if(!strcmp(ifName, WLANIF[i]))
		{
			return phyPortId;
		}
#ifdef WLAN_MBSSID
		for(j=1 ; j<=WLAN_MBSSID_NUM ; j++)
		{
			sprintf(wlanIfName, VAP_IF, i, j-1);
			if(!strcmp(ifName, wlanIfName))
			{
				return phyPortId;
			}
		}
#endif
	}
#endif
		
	return -1;
}

int rtk_lan_LogID_get_by_if_name(unsigned char *ifName)
{
	int i=0;
	char ifname_str[IFNAMSIZ]={0};
	if(ifName == NULL)
		return -1;
	for(i=0;i<SW_LAN_PORT_NUM;i++)
	{
		if(!strcmp(ifName, ELANRIF[i]))
		{
			return i;
		}
		//untag interface eth0.2.0
		sprintf(ifname_str, "%s.0", ELANRIF[i]);
		if(!strcmp(ifName, ifname_str))
		{
			return i;
		}
	}
#ifdef WLAN_SUPPORT
	for (i = PMAP_WLAN0; i <= PMAP_WLAN1_VAP_END; ++i)
	{
		if (wlan_en[i - PMAP_WLAN0] == 0)
			continue;

		if (!strcmp(ifName,  wlan[i - PMAP_WLAN0]))
		{
			return i;
		}
	}
#endif
	return -1;
}

#ifdef CONFIG_UNI_CAPABILITY
void setupUniPortCapability()
{
	FILE *fp = NULL;
	char line[64] = {0};
	char *delim = ";";
	char *pch = NULL;
	//char cmd[128];
	int portIdx = 0;
	unsigned int chipID;
	unsigned int rev;
	unsigned int subType;
#if defined(CONFIG_LUNA)
#if defined(CONFIG_COMMON_RT_API)
	rt_port_phy_ability_t ability;
#else
	rtk_port_phy_ability_t ability;
#endif
#endif
#ifdef CONFIG_USER_RTK_EXTGPHY
	rtk_extgphy_ability_t  extgphy_ability;
	memset(&extgphy_ability, 0 , sizeof(rtk_extgphy_ability_t));
#endif

	fp = fopen(PROC_UNI_CAPABILITY, "r");
	if (fp == NULL)
		return;
#ifdef CONFIG_COMMON_RT_API
	rt_switch_version_get(&chipID, &rev, &subType);
#else
	rtk_switch_version_get(&chipID, &rev, &subType);
#endif

	while (fgets(line, 64, fp))
	{
		if (line[0]=='#')
		{
			continue;
		}

		printf ("%s:%d line is %s\n", __FUNCTION__, __LINE__, line);
		pch = strtok(line, delim);
		while (pch != NULL)
		{
			printf ("%s:%d pch is %s\n", __FUNCTION__, __LINE__, pch);
			if (atoi(pch) == UNI_PORT_NONE)
			{
#if defined(CONFIG_LUNA)
#if defined(CONFIG_COMMON_RT_API)
				if(rt_port_phyAutoNegoAbility_get(portIdx, &ability) != RT_ERR_OK)
					printf("rt_port_phyAutoNegoAbility_get failed: %s %d\n", __func__, __LINE__);
#else
				rtk_port_phyAutoNegoAbility_get(portIdx, &ability);
#endif
				ability.Half_10 = 0;
				ability.Full_10 = 0;
				ability.Half_100 = 0;
				ability.Full_100 = 0;
				ability.Half_1000 = 0;
				ability.Full_1000 = 0;
#if defined(CONFIG_COMMON_RT_API)
				if(rt_port_phyAutoNegoAbility_set(portIdx, &ability) != RT_ERR_OK)
					printf("rt_port_phyAutoNegoAbility_set failed: %s %d\n", __func__, __LINE__);
#else
				rtk_port_phyAutoNegoAbility_set(portIdx, &ability);
#endif
#endif
				printf ("%s:%d pch is %s, port %d, UNI_PORT_NONE\n", __FUNCTION__, __LINE__, pch, portIdx);
				//sprintf(cmd, "diag port set auto-nego port %d ability", portIdx);
			}
			else if (atoi(pch) == UNI_PORT_FE)
			{
#if defined(CONFIG_LUNA)
#if defined(CONFIG_COMMON_RT_API)
				if(rt_port_phyAutoNegoAbility_get(portIdx, &ability) != RT_ERR_OK)
					printf("rt_port_phyAutoNegoAbility_get failed: %s %d\n", __func__, __LINE__);
#else
				rtk_port_phyAutoNegoAbility_get(portIdx, &ability);
#endif
				ability.Half_10 = 1;
				ability.Full_10 = 1;
				ability.Half_100 = 1;
				ability.Full_100 = 1;
				ability.Half_1000 = 0;
				ability.Full_1000 = 0;
#if defined(CONFIG_COMMON_RT_API)
				if(rt_port_phyAutoNegoAbility_set(portIdx, &ability) != RT_ERR_OK)
					printf("rt_port_phyAutoNegoAbility_set failed: %s %d\n", __func__, __LINE__);
#else
				rtk_port_phyAutoNegoAbility_set(portIdx, &ability);
#endif
				// We should enable swPbo in 9603CVD because throughput of GE port download will effect by FE port download.
				// We can only enable swPbo in 9603CVD(not 9607C oi 9603C), because only 9603CVD have enhance length of egress queue
				// In 9607C or 9603C, length of egress queue is too small so that IPTV will stocked
#ifdef CONFIG_RTL9603CVD_SERIES
				if(chipID == RTL9603CVD_CHIP_ID)
					rtk_swPbo_portState_set(portIdx, 1);
#endif
#endif
				printf ("%s:%d pch is %s, port %d, UNI_PORT_FE\n", __FUNCTION__, __LINE__, pch, portIdx);
				//sprintf(cmd, "diag port set auto-nego port %d ability 10h 10f 100h 100f flow-control asy-flow-control", portIdx);
			}
			else if(atoi(pch) == UNI_PORT_GE) //UNI_PORT_GE
			{
#if defined(CONFIG_LUNA)
#if defined(CONFIG_COMMON_RT_API)
				if(rt_port_phyAutoNegoAbility_get(portIdx, &ability) != RT_ERR_OK)
					printf("rt_port_phyAutoNegoAbility_get failed: %s %d\n", __func__, __LINE__);
#else
				rtk_port_phyAutoNegoAbility_get(portIdx, &ability);
#endif
				ability.Half_10 = 1;
				ability.Full_10 = 1;
				ability.Half_100 = 1;
				ability.Full_100 = 1;
				ability.Half_1000 = 1;
				ability.Full_1000 = 1;
#if defined(CONFIG_COMMON_RT_API)
				if(rt_port_phyAutoNegoAbility_set(portIdx, &ability) != RT_ERR_OK)
					printf("rt_port_phyAutoNegoAbility_set failed: %s %d\n", __func__, __LINE__);
#else
				rtk_port_phyAutoNegoAbility_set(portIdx, &ability);
#endif
#endif
				printf ("%s:%d pch is %s, port %d, UNI_PORT_GE\n", __FUNCTION__, __LINE__, pch, portIdx);
				//sprintf(cmd, "diag port set auto-nego port %d ability 10h 10f 100h 100f 1000f flow-control asy-flow-control", portIdx);
			}
			else if(atoi(pch) == UNI_PORT_2P5GE)
			{
#if defined(CONFIG_USER_RTK_EXTGPHY)
				extgphy_ability.Half_10 = 0;
				extgphy_ability.Full_10 = 0;
				extgphy_ability.Half_100 = 1;
				extgphy_ability.Full_100 = 1;
				extgphy_ability.Half_1000 = 1;
				extgphy_ability.Full_1000 = 1;
				extgphy_ability.Full_10000 = 1;
				extgphy_ability.Full_2500 = 1;
				extgphy_ability.Full_10000 = 1;
				extgphy_ability.FC = 0;
				extgphy_ability.AsyFC = 0;
				rt_port_extgphyAutoNegoAbility_set(portIdx, &extgphy_ability);
#endif
				printf ("%s:%d To-Do: not suuport UNI_PORT_2P5GE yet\n", __FUNCTION__, __LINE__);
			}
			else if(atoi(pch) == UNI_PORT_5GE)
			{
				/*To-Do*/
				printf ("%s:%d To-Do: not suuport UNI_PORT_5GE yet\n", __FUNCTION__, __LINE__);

			}
			else if(atoi(pch) == UNI_PORT_10GE)
			{
#if defined(CONFIG_LUNA)
#if defined(CONFIG_COMMON_RT_API)
				if(rt_port_phyAutoNegoAbility_get(portIdx, &ability) != RT_ERR_OK)
					printf("rt_port_phyAutoNegoAbility_get failed: %s %d\n", __func__, __LINE__);
#else
				rtk_port_phyAutoNegoAbility_get(portIdx, &ability);
#endif
				ability.Half_10 = 1;
				ability.Full_10 = 1;
				ability.Half_100 = 1;
				ability.Full_100 = 1;
				ability.Half_1000 = 1;
				ability.Full_1000 = 1;
				ability.Full_10000 = 1;
#if defined(CONFIG_COMMON_RT_API)
				if(rt_port_phyAutoNegoAbility_set(portIdx, &ability) != RT_ERR_OK)
					printf("rt_port_phyAutoNegoAbility_set failed: %s %d\n", __func__, __LINE__);
#else
				rtk_port_phyAutoNegoAbility_set(portIdx, &ability);
#endif
#endif
				printf ("%s:%d pch is %s, port %d, UNI_PORT_GE\n", __FUNCTION__, __LINE__, pch, portIdx);
				//sprintf(cmd, "diag port set auto-nego port %d ability 10h 10f 100h 100f 1000f flow-control asy-flow-control", portIdx);
			}
			else if(atoi(pch) == UNI_PORT_25GE)
			{
				/*To-Do*/
				printf ("%s:%d To-Do: not suuport UNI_PORT_25GE yet\n", __FUNCTION__, __LINE__);
			}
			else if(atoi(pch) == UNI_PORT_40GE)
			{
				/*To-Do*/
				printf ("%s:%d To-Do: not suuport UNI_PORT_40GE yet\n", __FUNCTION__, __LINE__);
			}
			pch = strtok (NULL, delim);
			portIdx++;
		}
		break;
	}
	fclose(fp);

	return;
}

int getUniPortCapability(int logPortId)
{
	FILE *fp = NULL;
	char line[64] = {0};
	char *delim = ";";
	char *pch = NULL;
	int portIdx = 0;
	int i = 0;

	fp = fopen(PROC_UNI_CAPABILITY, "r");
	if (fp == NULL)
		return -1;

	if (logPortId < 0 || logPortId > (ELANVIF_NUM - 1) ) 
	{
		fclose(fp);
		return -1;
	}

	portIdx = rtk_port_get_lan_phyID(logPortId);

	if (portIdx == -1) {
		fclose(fp);
		return -1;
	}

	while (fgets(line, 64, fp))
	{
		if (line[0] == '#')
		{
			continue;
		}

		printf ("%s:%d line is %s\n", __FUNCTION__, __LINE__, line);
		pch = strtok(line, delim);
		while (pch != NULL)
		{
			//printf ("%s:%d pch is %s\n",__FUNCTION__, __LINE__,pch);
			if (i == portIdx)
			{
				printf("%s:%d pch is %s, port is %d\n",__FUNCTION__, __LINE__, pch, portIdx);
				fclose(fp);
				return atoi(pch);
			}

			pch = strtok(NULL, delim);
			i++;
		}
		break;
	}
	fclose(fp);
	return -1;
}
#endif

#if defined(CONFIG_LUNA) && defined(CONFIG_RTL8686_SWITCH)
void set_port_powerdown_state(int port, unsigned int state)
{
	int i=0;
	int id = rtk_port_get_lan_phyID(port);

	if(id == -1)
		return;

#ifdef CONFIG_COMMON_RT_API
	rt_port_phyPowerDown_set(id, state);
#else
	rtk_port_phyPowerDown_set(id, state);
#endif
}

void set_all_lan_port_powerdown_state(unsigned int state)
{
	int i=0;
	int port;
	for(i=0;i<SW_LAN_PORT_NUM;i++){
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
		if (rtk_port_is_wan_logic_port(i))
			continue;
#endif

#ifdef CONFIG_COMMON_RT_API
		if(rt_switch_phyPortId_get(i, &port) == RT_ERR_OK)
			if(rt_port_phyPowerDown_set(port, state) != RT_ERR_OK)
				printf("rt_port_phyPowerDown_set failed: %s %d\n", __func__, __LINE__);
#else
		if(rtk_switch_phyPortId_get(i, &port) == RT_ERR_OK)
			if(rtk_port_phyPowerDown_set(port, state) != RT_ERR_OK)
				printf("rtk_port_phyPowerDown_set failed: %s %d\n", __func__, __LINE__);
#endif

	}
}
void set_all_port_powerdown_state(unsigned int state)
{
#ifdef CONFIG_COMMON_RT_API
	rt_switch_devInfo_t devInfo;
#else
	rtk_switch_devInfo_t devInfo;
#endif
#if defined(CONFIG_CMCC) && defined(CONFIG_RTL9607C_SERIES)
	unsigned int chipId, rev, subType;
	unsigned char green=1;
#endif
	int port = 0;
	int ret;

#ifdef CONFIG_COMMON_RT_API
	if(rt_switch_deviceInfo_get(&devInfo) != RT_ERR_OK)
		printf("rt_switch_deviceInfo_get failed, %s %d\n", __func__, __LINE__);
#else
	if(rtk_switch_deviceInfo_get(&devInfo) != RT_ERR_OK)
		printf("rtk_switch_deviceInfo_get failed: %s %d\n", __func__, __LINE__);
#endif
#if defined(CONFIG_CMCC) && defined(CONFIG_RTL9607C_SERIES)
#ifdef CONFIG_COMMON_RT_API
	rt_switch_version_get(&chipId, &rev, &subType);
#else
	rtk_switch_version_get(&chipId, &rev, &subType);
#endif

	if(chipId==RTL9607C_CHIP_ID)
	{
		switch(subType)
		{
			case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA5:
			case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA6:
			case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA7:
			case RTL9607C_CHIP_SUB_TYPE_RTL9606C_VA5:
			case RTL9607C_CHIP_SUB_TYPE_RTL9606C_VA6:
			case RTL9607C_CHIP_SUB_TYPE_RTL9606C_VA7:
				green=0;
				break;
			default:
				green=1;
				break;
		}
	}
#endif

	for (port = devInfo.ether.min; port <= devInfo.ether.max; port++)
	{
		if(port>=32) break;

		if (devInfo.ether.portmask.bits[0] & 1<<port)
		{
			//All Port
#if defined(CONFIG_CMCC) && defined(CONFIG_RTL9607C_SERIES)
			rtk_port_greenEnable_set(port, green); //disable green table to fix LAN TC rx FCS error
#endif
#ifdef CONFIG_COMMON_RT_API
			if(rt_port_phyPowerDown_set(port, state) != RT_ERR_OK)
				printf("rt_port_phyPowerDown_set(%d, %d) failed: %s %d\n",port, state, __func__, __LINE__);
#else
			if(rtk_port_phyPowerDown_set(port, state) != RT_ERR_OK)
				printf("rtk_port_phyPowerDown_set failed: %s %d\n", __func__, __LINE__);
#endif
		}
	}
#if defined(CONFIG_EXTERNAL_PHY_POLLING)
	if (state == 0) { // phy power up
#ifdef CONFIG_COMMON_RT_API
		rt_mdio_c22_write(0, 0x1140);
#else
		rtk_mdio_c22_write(0, 0x1140);
#endif
	}
#endif
#if defined(CONFIG_RTL_8221B_SUPPORT)
	if (state == 0) { // phy power up
		system("/bin/echo power_up > /proc/rtl8221b/phy");
	}
#endif
}
#endif


#ifndef CONFIG_RTL_MULTI_PHY_ETH_WAN
int rtk_port_set_wan_phyID(int phyPort)
{
	int i, j;
	int tmp_val;
	int origPhyPort;
	unsigned char remap_tbl[MAX_LAN_PORT_NUM];	
	int ret=0;
	
	mib_get_s(MIB_WAN_PHY_PORT, (void *)&origPhyPort, sizeof(origPhyPort));

	if(phyPort==origPhyPort)
		return RTK_SUCCESS;
	
	mib_set(MIB_WAN_PHY_PORT, (void *)&phyPort);

	mib_get_s(MIB_PORT_REMAPPING, (void *)remap_tbl, sizeof(remap_tbl));

	for(i=0; i<ELANVIF_NUM; i++)
	{
		if(remap_tbl[i]==phyPort)
		{
			remap_tbl[i]=origPhyPort;
			break;
		}		
	}

	//sort
	for(i=0; i<ELANVIF_NUM-1; i++)
	for(j=i+1; j<ELANVIF_NUM; j++)
	if(remap_tbl[j]<remap_tbl[i])
	{
		tmp_val=remap_tbl[i];
		remap_tbl[i]=remap_tbl[j];
		remap_tbl[j]=tmp_val;
	}
	mib_set(MIB_PORT_REMAPPING, (void *)remap_tbl);	

	ret=mib_update_ex(HW_SETTING, CONFIG_MIB_ALL, 1);
	if(ret==0)
		printf("mib_update_ex error!\n");

	return RTK_SUCCESS;
}

int rtk_set_wan_phyport_and_reinit_system(int newWanPhyPort)
{
	int wanPhyPort = rtk_port_get_wan_phyID();	

	if(newWanPhyPort>=0 && wanPhyPort>=0 && newWanPhyPort!=wanPhyPort)
	{
		rtk_port_set_wan_phyID(newWanPhyPort);
		
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
		cleanSystem();				

		reStartSystem();
#endif
		return RTK_SUCCESS;
	}

	return RTK_FAILED;
}

int rtk_get_config_default_wan_port_num(int *wan_phy_port)
{
	int ret = RTK_FAILED;
	int phy_port = 0;	

	if(mib_get_def(MIB_WAN_PHY_PORT, (void *)&phy_port, sizeof(phy_port))) {
		*wan_phy_port = phy_port;
		ret = RTK_SUCCESS;
	}

	return ret;	
}
#endif

static const char * const br_port_state_names[] = {
	[RTK_BR_PORT_STATE_DISABLED] = "disabled",
	[RTK_BR_PORT_STATE_LISTENING] = "listening",
	[RTK_BR_PORT_STATE_LEARNING] = "learning",
	[RTK_BR_PORT_STATE_FORWARDING] = "forwarding",
	[RTK_BR_PORT_STATE_BLOCKING] = "blocking",
};

const char * get_br_port_state_str(int state)
{
	if(state >= RTK_BR_PORT_STATE_DISABLED && state <= RTK_BR_PORT_STATE_BLOCKING)
		return br_port_state_names[state];
	else
		return "unknown";
}

int check_ifname_br_port_state(char *ifname, int state)
{
	unsigned char stp_enable = 0;
	int i, phyPort;

	mib_get_s(MIB_BRCTL_STP, (void *)&stp_enable, sizeof(stp_enable));
	if(stp_enable)
	{
		for(i=0;i<ELANVIF_NUM;i++)
		{
			if(!strcmp(ELANVIF[i], ifname) && 
				(phyPort = rtk_port_get_lan_phyID(i)) >= 0)
			{
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) 
				rtk_fc_stp_check_port_stat(phyPort, state);
#endif
				break;
			}
		}
	}
	
	return 0;
}

int rtk_port_set_dev_port_mapping(int port_id, char *dev_name)
{
	FILE* fp;
	char sysbuf[128];

	fp = fopen("/proc/eth_nic/dev_port_mapping", "r");
	if (fp)
	{
		fclose(fp);
		sprintf(sysbuf, "/bin/echo %d %s > /proc/eth_nic/dev_port_mapping", port_id, dev_name);
	}
	else
	{
#if defined(CONFIG_LUNA_G3_SERIES)
		sprintf(sysbuf, "/bin/echo %d %s > /proc/driver/cortina/ni/dev_port_mapping", port_id, dev_name);
#else
		sprintf(sysbuf, "/bin/echo %d %s > /proc/rtl8686gmac/dev_port_mapping", port_id, dev_name);
#endif
	}
	va_cmd("/bin/sh", 2, 1, "-c", sysbuf);

	return 0;
}

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
int rtk_external_port_set_dev_port_mapping(int port_id, char *dev_name)
{
	char sysbuf[128];

#ifdef CONFIG_RTL8367_SUPPORT
	sprintf(sysbuf, "/bin/echo %d %s > /proc/rtl8367/dev_port_mapping", port_id, dev_name);
	va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
#endif

	return 0;
}

int rtk_port_check_external_port(int logPortId)
{
	int phyPortId = -1;
    phyPortId = rtk_port_get_lan_phyID(logPortId);
	if(phyPortId >=EXTERNAL_SWITCH_PORT_OFFSET){
		return 1;
	}
	return 0;
}
#endif



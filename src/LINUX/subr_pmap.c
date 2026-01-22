
/*
 *      System routines for Port-mapping
 *
 */

#include <string.h>
#include "debug.h"
#include "utility.h"
#include "subr_net.h"
#include <linux/wireless.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "sockmark_define.h"
#ifdef CONFIG_RTK_MAC_AUTOBINDING
#include "../cmcc/libnetwork_core/plugin/mac_autobinding/mac_autobinding.h"
#endif

/*
 * The bitmap for interface should follow this convention commonly shared
 * by sar driver (sar_send())
 * Bit-map:
 *  bit16|bit15|bit14|bit13|bit12|(bit11)|bit10|bit9|bit8|bit7|bit6 |bit5 |bit4  |bit3|bit2|bit1|bit0
 *  vap3 |vap2 |vap1 |vap0 |wlan1|(usb0) |vap3 |vap2|vap1|vap0|wlan0|resvd|device|lan3|lan2|lan1|lan0
 * If usb(switch port) add:
 *  bit16|bit15|bit14|bit13|bit12|(bit11)|bit10|bit9|bit8|bit7|bit6 |bit5  |bit4     |bit3|bit2|bit1|bit0
 *  vap3 |vap2 |vap1 |vap0 |wlan1|(usb0) |vap3 |vap2|vap1|vap0|wlan0|device|lan4(usb)|lan3|lan2|lan1|lan0
 */

#ifdef CONFIG_USER_VXLAN
int exec_vxlan_portmp();
#endif
#ifdef CONFIG_VIR_BOA_JEMBENCHTEST
extern void rtk_cmcc_do_jspeedup_ex();
#endif

#ifdef NEW_PORTMAPPING
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct pmap_s pmap_list[MAX_VC_NUM];

/*
 *	Get the port-mapping bitmap (fgroup); member ports(mapped+unmapped) for WAN interfaces.
 *	-1	failed
 *	0	successful
 */
int get_pmap_fgroup(struct pmap_s *pmap_p, int num)
{
	unsigned int total;
	unsigned int mapped_itfgroup, unmapped_itfgroup;
	int i, k;
	MIB_CE_ATM_VC_T Entry;

	memset(pmap_p, 0, sizeof(struct pmap_s)*num);
	total = mib_chain_total(MIB_ATM_VC_TBL);
	if (total > num)
		return -1;
	mapped_itfgroup = 0;

	for(i = 0; i < total; i++) {
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&Entry))
			printf("Error! Get MIB_ATM_VC_TBL(get_pmap_fgroup) error.\n");
		if (!Entry.enable || !isValidMedia(Entry.ifIndex))
			continue;
		pmap_p[i].ifIndex = Entry.ifIndex;
		pmap_p[i].valid = 1;
		pmap_p[i].itfGroup = Entry.itfGroup;
		mapped_itfgroup |= Entry.itfGroup;
	}
	unmapped_itfgroup = ((~mapped_itfgroup) & 0xFFFFFFFF);

	// Get fgroup
	for(i = 0; i < total; i++) {
		// mapped + unmapped
		pmap_p[i].fgroup = pmap_p[i].itfGroup | unmapped_itfgroup;
	}

	return 0;
}

#if defined(CONFIG_USER_RTK_MULTI_BRIDGE_WAN)
int check_BrWanitfGroup(MIB_CE_BR_WAN_Tp pEntry, MIB_CE_BR_WAN_Tp pOldEntry)
{
	uint32_t i;
	uint32_t cur_itfgroup;
	int32_t totalEntry;
	int ret;

	MIB_CE_BR_WAN_T temp_Entry;

	cur_itfgroup = pEntry->itfGroup;

	if(cur_itfgroup)
	{
		totalEntry = mib_chain_total(MIB_BR_WAN_TBL);
		for(i = 0; i < totalEntry; ++i)
		{
			uint32_t sameport;
			ret = mib_chain_get(MIB_BR_WAN_TBL, i, (void*)&temp_Entry);

			if(!ret)
				return -1;

			if(pOldEntry && pOldEntry->ifIndex == temp_Entry.ifIndex)
				continue;

			sameport = (uint32_t)(cur_itfgroup & temp_Entry.itfGroup);
			if(sameport) {
				temp_Entry.itfGroup &=	(uint32_t)( ~(sameport));
				mib_chain_update(MIB_BR_WAN_TBL, (void*)&temp_Entry, i);
			}
		}  //end for()
	}
	return 0;
}

static void set_br_wan_pmap_rule(MIB_CE_BR_WAN_Tp pEntry, int action)
{
	unsigned int ifIndex;
	char ifname[IFNAMSIZ+1], wan_ifname[IFNAMSIZ+1],ppp_ifname[IFNAMSIZ+1], chain_in[64], chain_out[64], polmark[64];
	int32_t j;
	uint32_t itfGroup, hit_entry = 0;
	uint32_t wan_mark = 0, wan_mask = 0;
#if defined(CONFIG_E8B)
	int32_t totalPortbd ,k;
	MIB_CE_PORT_BINDING_T portBindEntry;
	struct vlan_pair *vid_pair;
#endif
	char ifName[IFNAMSIZ];

	if((pEntry->cmode != CHANNEL_MODE_BRIDGE) || (pEntry->vlan == 0))
		return;

	itfGroup	= pEntry->itfGroup;
	ifIndex		= pEntry->ifIndex;

	ifGetName( PHY_INTF(ifIndex), wan_ifname, sizeof(wan_ifname));
	
	sprintf(chain_in, "%s_%s_in", PORTMAP_EBTBL, wan_ifname);
	sprintf(chain_out, "%s_%s_out", PORTMAP_EBTBL, wan_ifname);
	
	va_cmd(EBTABLES, 2, 1, "-F", chain_in);
	va_cmd(EBTABLES, 2, 1, "-X", chain_in);
	va_cmd(EBTABLES, 2, 1, "-F", chain_out);
	va_cmd(EBTABLES, 2, 1, "-X", chain_out);
	
	if(action)
	{
		ifGetName(pEntry->ifIndex, ifName, sizeof(ifName));
		rtk_net_add_wan_policy_route_mark(ifName, &wan_mark, &wan_mask);

		va_cmd(EBTABLES, 2, 1, "-N", chain_in);
		va_cmd(EBTABLES, 3, 1, "-P", chain_in, "DROP");
		va_cmd(EBTABLES, 2, 1, "-N", chain_out);
		va_cmd(EBTABLES, 3, 1, "-P", chain_out, "DROP");
		
		if(pEntry->applicationtype & X_BR_SRV_OTHER)
		{
			hit_entry = 1;
		}

		for(j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j)
		{
			if(BIT_IS_SET(itfGroup, j))
			{
				hit_entry = 1;
				va_cmd(EBTABLES, 6, 1, "-I", chain_in, "-o", ELANVIF[j], "-j", "RETURN");
				va_cmd(EBTABLES, 6, 1, "-I", chain_out, "-i", ELANVIF[j], "-j", "RETURN");				
#ifdef CONFIG_E8B
				if(pEntry->disableLanDhcp == LAN_DHCP_ENABLE)  //get dhcp from ONU
				{
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_INTBL, "-i", ELANVIF[j], "-j", PORTMAP_SEVTBL2);
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", ELANVIF[j], "-j", PORTMAP_SEVTBL2);
					va_cmd(EBTABLES, 12, 1, "-I", PORTMAP_EBTBL, "-i", ELANVIF[j], "-p", "ipv4",
						"--ip-proto", "udp", "--ip-dport", "67:68", "-j", "DROP");
					va_cmd(EBTABLES, 12, 1, "-I", PORTMAP_EBTBL, "-i", ELANVIF[j], "-p", "ipv6",
						"--ip6-proto", "udp", "--ip6-dport", "546:547", "-j", "DROP");
					va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, PORTMAP_EBTBL, "-o",ELANVIF[j] , "-p", "IPv6",
						"--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "router-advertisement","-j", (char *)FW_DROP);
				}
				else   //get dhcp from WAN
#endif
				{
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_INTBL, "-i", ELANVIF[j], "-j", PORTMAP_SEVTBL);
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", ELANVIF[j], "-j", PORTMAP_SEVTBL);
				}
				
				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", ELANVIF[j], "-j", chain_out);

				sprintf(polmark, "0x%x", wan_mark);
				va_cmd(EBTABLES, 12, 1, "-t", "broute", "-A", PORTMAP_BRTBL, "-i", ELANVIF[j], "-j", "mark", "--mark-or", polmark, "--mark-target", "CONTINUE");		
#if !defined(CONFIG_CMCC)
				if (pEntry->applicationtype & X_CT_SRV_OTHER)
#endif
				{
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-o", ELANVIF[j], "-j", chain_out);
				}

				//block lan->nas0 when the lan port bind with wan on web in bridge mode
				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-i", ELANVIF[j], "-j", chain_in);
				//block nas0->lan when the lan port bind with wan on web in bridge mode
				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-o", ELANVIF[j], "-j", chain_out);
			}
		}
		
		for(j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)
		{
			if(BIT_IS_SET(itfGroup, j))
			{
				hit_entry = 1;
				va_cmd(EBTABLES, 6, 1, "-I", chain_in, "-o", wlan[j-PMAP_WLAN0], "-j", "RETURN");
				va_cmd(EBTABLES, 6, 1, "-I", chain_out, "-i", wlan[j-PMAP_WLAN0], "-j", "RETURN");
#ifdef CONFIG_E8B
				if(pEntry->disableLanDhcp == LAN_DHCP_ENABLE)
				{
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_INTBL, "-i", wlan[j-PMAP_WLAN0], "-j", PORTMAP_SEVTBL2);
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", wlan[j-PMAP_WLAN0], "-j", PORTMAP_SEVTBL2);
					va_cmd(EBTABLES, 12, 1, "-I", PORTMAP_EBTBL, "-i", wlan[j-PMAP_WLAN0], "-p", "ipv4",
						"--ip-proto", "udp", "--ip-dport", "67:68", "-j", "DROP");
					va_cmd(EBTABLES, 12, 1, "-I", PORTMAP_EBTBL, "-i", wlan[j-PMAP_WLAN0], "-p", "ipv6",
						"--ip6-proto", "udp", "--ip6-dport", "546:547", "-j", "DROP");
					va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, PORTMAP_EBTBL, "-o",wlan[j-PMAP_WLAN0] , "-p", "IPv6",
						"--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "router-advertisement","-j", (char *)FW_DROP);
				}
				else 
#endif
				{
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_INTBL, "-i", wlan[j-PMAP_WLAN0], "-j", PORTMAP_SEVTBL);
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", wlan[j-PMAP_WLAN0], "-j", PORTMAP_SEVTBL);
				}

				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", wlan[j-PMAP_WLAN0], "-j", chain_out);
				
				sprintf(polmark, "0x%x", wan_mark);
				va_cmd(EBTABLES, 12, 1, "-t", "broute", "-A", PORTMAP_BRTBL, "-i", wlan[j-PMAP_WLAN0], "-j", "mark", "--mark-or", polmark, "--mark-target", "CONTINUE");

#if !defined(CONFIG_CMCC)
				if (pEntry->applicationtype & X_CT_SRV_OTHER)
#endif
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-o", wlan[j-PMAP_WLAN0], "-j", chain_out);
				//block wlan->nas0 when the wlan port bind with wan on web in bridge mode
				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-i", wlan[j-PMAP_WLAN0], "-j", chain_in);
				//block nas0->wlan when the wlan port bind with wan on web in bridge mode
				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-o", wlan[j-PMAP_WLAN0], "-j", chain_out);
			}
		}

#if defined(CONFIG_E8B)
		const char *rifName = NULL;
		uint32_t logPort;
#ifdef WLAN_SUPPORT
		int ssidIdx = -1;
#endif
		totalPortbd = mib_chain_total(MIB_PORT_BINDING_TBL);
		//polling every lanport vlan-mapping entry
		for (j = 0; j < totalPortbd; ++j)
		{
			if(!mib_chain_get(MIB_PORT_BINDING_TBL, j, (void*)&portBindEntry))
				continue;
			logPort = portBindEntry.port;
			if((unsigned char)VLAN_BASED_MODE == portBindEntry.pb_mode)
			{	
				vid_pair = (struct vlan_pair *)&portBindEntry.pb_vlan0_a;
				for (k=0; k<4; k++)
				{
					#if defined(CONFIG_RTK_DEV_AP)
					//support lan/wlan vlan bind untag wan 
					if(!(vid_pair[k].vid_a > 0 && pEntry->vid == vid_pair[k].vid_b))
					#else
					if(!(vid_pair[k].vid_a > 0 && vid_pair[k].vid_b > 0 && 
						(pEntry->vlan && pEntry->vid == vid_pair[k].vid_b))
					)
					#endif
					{
						continue;
					}
					
					if(logPort < PMAP_WLAN0 && logPort < ELANVIF_NUM)
						rifName = ELANRIF[logPort];
#ifdef WLAN_SUPPORT
					else if((ssidIdx = (logPort-PMAP_WLAN0)) < (WLAN_SSID_NUM*NUM_WLAN_INTERFACE) && ssidIdx >= 0)
						rifName = wlan[ssidIdx];
#endif
					else 
						rifName = NULL;

					if(rifName == NULL || !getInFlags(rifName, NULL))
						continue;
					
					ifname[0] = '\0';
					sprintf(ifname, "%s.%d", rifName, vid_pair[k].vid_a);
					if(!getInFlags(ifname, NULL))
						continue;
					
					hit_entry = 1;
					va_cmd(EBTABLES, 6, 1, "-I", chain_in, "-o", ifname, "-j", "RETURN");
					va_cmd(EBTABLES, 6, 1, "-I", chain_out, "-i", ifname, "-j", "RETURN");
#ifdef CONFIG_E8B
					if(pEntry->disableLanDhcp == LAN_DHCP_ENABLE)
					{
						va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_INTBL, "-i", ifname, "-j", PORTMAP_SEVTBL2);
						va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", ifname, "-j", PORTMAP_SEVTBL2);
						va_cmd(EBTABLES, 12, 1, "-I", PORTMAP_EBTBL, "-i", ifname, "-p", "ipv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "DROP");
						va_cmd(EBTABLES, 12, 1, "-I", PORTMAP_EBTBL, "-i", ifname, "-p", "ipv6", "--ip6-proto", "udp", "--ip6-dport", "546:547", "-j", "DROP");
						va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, PORTMAP_EBTBL, "-o", ifname , "-p", "IPv6","--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "router-advertisement","-j", (char *)FW_DROP);
					}
					else 
#endif
					{
						va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_INTBL, "-i", ifname, "-j", PORTMAP_SEVTBL);
						va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", ifname, "-j", PORTMAP_SEVTBL);
					}
					
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", ifname, "-j", chain_out);

					sprintf(polmark, "0x%x", wan_mark);
					va_cmd(EBTABLES, 12, 1, "-t", "broute", "-A", PORTMAP_BRTBL, "-i", ifname, "-j", "mark", "--mark-or", polmark, "--mark-target", "CONTINUE");

#if !defined(CONFIG_CMCC)
					if (pEntry->applicationtype & X_CT_SRV_OTHER)
#endif
						va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-o", ifname, "-j", chain_out);

					//block lan/wlan->nas0 when the lan/wlan vlan bind with wan on web in bridge mode
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-i", ifname, "-j", chain_in);
					//block nas0->lan/wlan when the lan/wlan vlan bind with wan on web in bridge mode
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-o", ifname, "-j", chain_out);
				}
			}
		}
#endif
		if(hit_entry)
		{
			va_cmd(EBTABLES, 6, 1, "-I", chain_out, "-i", wan_ifname, "-j", "RETURN");
			va_cmd(EBTABLES, 6, 1, "-I", chain_in, "-o", wan_ifname, "-j", "RETURN");
			va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-i", wan_ifname, "-j", chain_in);
			va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-o", wan_ifname, "-j", chain_out);
			va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_INTBL, "-i", wan_ifname, "-j", PORTMAP_SEVTBL);
			va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", wan_ifname, "-j", PORTMAP_SEVTBL);
			sprintf(polmark, "0x%x", wan_mark);
			va_cmd(EBTABLES, 12, 1, "-t", "broute", "-A", PORTMAP_BRTBL, "-i", wan_ifname, "-j", "mark", "--mark-or", polmark, "--mark-target", "CONTINUE");
			//block nas0->lan/wlan when the lan/wlan vlan bind with wan on web in bridge mode
			va_cmd(EBTABLES, 6, 1, "-I", chain_out, "-i", ALIASNAME_NAS0, "-j", "DROP");		
		
			//for binding MARK 
			sprintf(polmark, "0x%x/0x%x", wan_mark, wan_mask);
			va_cmd(EBTABLES, 6, 1, "-A", (char *)chain_out, "--mark", polmark, "-j", "RETURN");
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
			MIB_CE_BR_WAN_T entry;
			uint32_t wan_mark_2 = 0, wan_mask_2 = 0;
			int i=0, vcEntryNum = mib_chain_total(MIB_BR_WAN_TBL);
			char wan_ifname_2[IFNAMSIZ+1];

			for(i=0;i<vcEntryNum; i++)
			{
				if(!mib_chain_get(MIB_BR_WAN_TBL, i, (void*)&entry))
					continue;
				if ( entry.IpProtocol == pEntry->IpProtocol)
					continue;
				for(j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j)
				{
					if(BIT_IS_SET(itfGroup, j) && BIT_IS_SET(entry.itfGroup, j))
					{
						ifGetName( PHY_INTF(entry.ifIndex), wan_ifname_2, sizeof(wan_ifname_2));
						ifGetName(entry.ifIndex, ifName, sizeof(ifName));
						rtk_net_get_wan_policy_route_mark(ifName, &wan_mark_2, &wan_mask_2);
						//for none binding to binding LAN port
						sprintf(polmark, "0x%x/0x%x", 0, wan_mask_2);
						va_cmd(EBTABLES, 9, 1, "-A", (char *)chain_out, "-i", "!", wan_ifname_2, "--mark", polmark, "-j", "RETURN");						
						//for binding MARK
						sprintf(polmark, "0x%x/0x%x", wan_mark_2, wan_mask_2);
						va_cmd(EBTABLES, 6, 1, "-A", (char *)chain_out, "--mark", polmark, "-j", "RETURN");
						break;
					}
				}
			}
#endif
		}
		else
		{   // if no any port or vlan hit, clear ebtables rules
			va_cmd(EBTABLES, 2, 1, "-F", chain_in);
			va_cmd(EBTABLES, 2, 1, "-X", chain_in);
			va_cmd(EBTABLES, 2, 1, "-F", chain_out);
			va_cmd(EBTABLES, 2, 1, "-X", chain_out);
		}		
	}	
}

static void reset_br_wan_pmap()
{
	int32_t totalEntry, i = 0;
	MIB_CE_BR_WAN_T temp_Entry;
#if defined(CONFIG_E8B)
	MIB_CE_PORT_BINDING_T portBindEntry;
#endif

	// ebtables -D INPUT -j portmapping_input
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_INPUT, "-j", (char *)PORTMAP_INTBL);
	// ebtables -F portmapping_input
	va_cmd(EBTABLES, 2, 1, "-F", (char *)PORTMAP_INTBL);
	// ebtables -X portmapping_input
	va_cmd(EBTABLES, 2, 1, "-X", (char *)PORTMAP_INTBL);
	
	// ebtables -D OUTPUT -j portmapping_output
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_OUTPUT, "-j", (char *)PORTMAP_OUTTBL);
	// ebtables -F portmapping_input
	va_cmd(EBTABLES, 2, 1, "-F", (char *)PORTMAP_OUTTBL);
	// ebtables -X portmapping_input
	va_cmd(EBTABLES, 2, 1, "-X", (char *)PORTMAP_OUTTBL);
	
	// ebtables -F portmapping_service
	va_cmd(EBTABLES, 2, 1, "-F", (char *)PORTMAP_SEVTBL);
	// ebtables -X portmapping_service
	va_cmd(EBTABLES, 2, 1, "-X", (char *)PORTMAP_SEVTBL);
#ifdef CONFIG_E8B
	// ebtables -F portmapping_service2
	va_cmd(EBTABLES, 2, 1, "-F", (char *)PORTMAP_SEVTBL2);
	// ebtables -X portmapping_service2
	va_cmd(EBTABLES, 2, 1, "-X", (char *)PORTMAP_SEVTBL2);
#endif
	// ebtables -D FORWARD -j portmapping
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)PORTMAP_EBTBL);
	// ebtables -F portmapping
	va_cmd(EBTABLES, 2, 1, "-F", (char *)PORTMAP_EBTBL);
	// ebtables -X portmapping
	va_cmd(EBTABLES, 2, 1, "-X", (char *)PORTMAP_EBTBL);
	
	// ebtables -t broute -D BROUTING -j portmapping_broute
	va_cmd(EBTABLES, 6, 1, "-t", "broute", (char *)FW_DEL, "BROUTING", "-j", (char *)PORTMAP_BRTBL);
	// ebtables -t broute -F portmapping_broute
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", (char *)PORTMAP_BRTBL);
	// ebtables -t broute -X portmapping_broute
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X", (char *)PORTMAP_BRTBL);
	
	// iptables -t mangle -D PREROUTING -j portmapping_rt_tbl
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", (char *)FW_PREROUTING, "-j", PORTMAP_RTTBL);
	// iptables -t mangle -F portmapping_rt_tbl
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", PORTMAP_RTTBL);
	// iptables -t mangle -X portmapping_rt_tbl
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", PORTMAP_RTTBL);
#ifdef CONFIG_IPV6
	// ip6tables -t mangle -D PREROUTING -j portmapping_rt_tbl
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", (char *)FW_PREROUTING, "-j", PORTMAP_RTTBL);
	// ip6tables -t mangle -F portmapping_rt_tbl
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", PORTMAP_RTTBL);
	// ip6tables -t mangle -X portmapping_rt_tbl
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", PORTMAP_RTTBL);
#endif

#if defined(CONFIG_E8B)
	totalEntry = mib_chain_total(MIB_PORT_BINDING_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_PORT_BINDING_TBL, i, (void*)&portBindEntry))
		{
			setup_vlan_map_intf(&portBindEntry, portBindEntry.port, 0);
		} 
	}
#endif

	totalEntry = mib_chain_total(MIB_BR_WAN_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_BR_WAN_TBL, i, (void*)&temp_Entry))
		{
			char ifname[IFNAMSIZ] ="";
			set_br_wan_pmap_rule(&temp_Entry, 0);
			ifGetName(temp_Entry.ifIndex, ifname, sizeof(ifname));
			rtk_net_remove_wan_policy_route_mark(ifname);
		} 
	}	
}

int exec_br_wan_portmp()
{
	int32_t totalEntry;
	MIB_CE_BR_WAN_T temp_Entry, comp_Entry;
	int32_t i,j=0,k=0;
#if defined(CONFIG_E8B)
	MIB_CE_PORT_BINDING_T portBindEntry;
#endif
	
	//ebtables -N portmapping_input
	va_cmd(EBTABLES, 2, 1, "-N", (char *)PORTMAP_INTBL);
	//ebtables -P portmapping_input RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)PORTMAP_INTBL, "RETURN");
	//ebtables -A INPUT -j portmapping_input
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)PORTMAP_INTBL);
	
	//ebtables -N portmapping_output
	va_cmd(EBTABLES, 2, 1, "-N", (char *)PORTMAP_OUTTBL);
	//ebtables -P portmapping_output RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)PORTMAP_OUTTBL, "RETURN");
	//ebtables -A INPUT -j portmapping_output
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-j", (char *)PORTMAP_OUTTBL);
	
	//ebtables -N portmapping_service
	va_cmd(EBTABLES, 2, 1, "-N", (char *)PORTMAP_SEVTBL);
	//ebtables -P portmapping_service RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)PORTMAP_SEVTBL, "RETURN");
	//ebtables -A portmapping_service -p ipv4 --ip-proto udp --ip-dport 67:68 -j DROP ## block DHCP
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL, "-p", "ipv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "DROP");
	//ebtables -A portmapping_service -p ipv4 --ip-proto 2 -j DROP ## block IGMP
	va_cmd(EBTABLES, 8, 1, "-A", PORTMAP_SEVTBL, "-p", "ipv4", "--ip-proto", "2", "-j", "DROP");
	//ebtables -A portmapping_service -d multicast -p ipv4 --ip-proto ! icmp -j DROP ## block multicast data
	va_cmd(EBTABLES, 11, 1, "-A", PORTMAP_SEVTBL, "-d", "multicast", "-p", "ipv4", "--ip-proto", "!", "icmp", "-j", "DROP");
#ifdef CONFIG_IPV6
	//ebtables -A portmapping_service -p ipv6 --ip6-proto udp --ip6-dport 546:547 -j DROP ## block DHCPv6
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL, "-p", "ipv6", "--ip6-proto", "udp", "--ip6-dport", "546:547", "-j", "DROP");
	//ebtables -A portmapping_service -p ipv6 --ip6-proto ipv6-icmp --ip6-icmp-type 130:134 -j DROP ## block MLD, RS, RA
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL, "-p", "ipv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "130:134", "-j", "DROP");
	//ebtables -A portmapping_service -d multicast -p ipv6 --ip6-proto ! ipv6-icmp -j DROP ## block multicast data
	va_cmd(EBTABLES, 11, 1, "-A", PORTMAP_SEVTBL, "-d", "multicast", "-p", "ipv6", "--ip6-proto", "!", "ipv6-icmp", "-j", "DROP");
#endif
#ifdef CONFIG_E8B
	//accept DHCP
	//ebtables -N portmapping_service2 
	va_cmd(EBTABLES, 2, 1, "-N", (char *)PORTMAP_SEVTBL2);
	//ebtables -P portmapping_service2 RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)PORTMAP_SEVTBL2, "RETURN");
	//ebtables -A portmapping_service -p ipv4 --ip-proto udp --ip-dport 67:68 -j DROP ## block DHCP
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL2, "-p", "ipv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "RETURN");
	//ebtables -A portmapping_service2 -p ipv4 --ip-proto 2 -j DROP ## block IGMP
	va_cmd(EBTABLES, 8, 1, "-A", PORTMAP_SEVTBL2, "-p", "ipv4", "--ip-proto", "2", "-j", "DROP");
	//ebtables -A portmapping_service2 -d multicast -p ipv4 --ip-proto ! icmp -j DROP ## block multicast data
	va_cmd(EBTABLES, 11, 1, "-A", PORTMAP_SEVTBL2, "-d", "multicast", "-p", "ipv4", "--ip-proto", "!", "icmp", "-j", "DROP");
#ifdef CONFIG_IPV6
	//ebtables -A portmapping_service -p ipv6 --ip6-proto udp --ip6-dport 546:547 -j DROP ## block DHCPv6
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL2, "-p", "ipv6", "--ip6-proto", "udp", "--ip6-dport", "546:547", "-j", "RETURN");
	//ebtables -A portmapping_service2 -p ipv6 --ip6-proto ipv6-icmp --ip6-icmp-type 130:132 -j DROP ## block MLD
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL2, "-p", "ipv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "130:132", "-j", "DROP");
	//ebtables -A portmapping_service2 -d multicast -p ipv6 --ip6-proto ! ipv6-icmp -j DROP ## block multicast data
	va_cmd(EBTABLES, 11, 1, "-A", PORTMAP_SEVTBL2, "-d", "multicast", "-p", "ipv6", "--ip6-proto", "!", "ipv6-icmp", "-j", "DROP");
#endif
#endif
	//ebtables -N portmapping
	va_cmd(EBTABLES, 2, 1, "-N", (char *)PORTMAP_EBTBL);
	//ebtables -P portmapping RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)PORTMAP_EBTBL, "RETURN");
	//ebtables -A FORWARD -j portmapping
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)PORTMAP_EBTBL);

	// ebtables -t broute -N portmapping_broute
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N", (char *)PORTMAP_BRTBL);
	// ebtables -t broute -F portmapping_broute
	va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P", (char *)PORTMAP_BRTBL, "RETURN");
	// ebtables -t broute -I BROUTING -j portmapping_broute
	va_cmd(EBTABLES, 6, 1, "-t", "broute", "-I", "BROUTING", "-j", (char *)PORTMAP_BRTBL);
	
	// for filter WAN routing packet
	// iptables -t mangle -N portmapping_rt_tbl
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", PORTMAP_RTTBL);
	// iptables -t mangle -A PREROUTING -j portmapping_rt_tbl
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", (char *)FW_PREROUTING, "-j", PORTMAP_RTTBL);
#ifdef CONFIG_IPV6
	// ip6tables -t mangle -N portmapping_rt_tbl
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", PORTMAP_RTTBL);
	// ip6tables -t mangle -A PREROUTING -j portmapping_rt_tbl
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", (char *)FW_PREROUTING, "-j", PORTMAP_RTTBL);
#endif

	//block pkts between nas+ 
	va_cmd(EBTABLES, 8, 1, "-I", PORTMAP_EBTBL, "-i", "nas+", "-o", "nas+", "-j", "DROP");

#if defined(CONFIG_E8B)
	totalEntry = mib_chain_total(MIB_PORT_BINDING_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_PORT_BINDING_TBL, i, (void*)&portBindEntry))
		{
			setup_vlan_map_intf(&portBindEntry, portBindEntry.port, 1);
		} 
	}
#endif
	
	totalEntry = mib_chain_total(MIB_BR_WAN_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_BR_WAN_TBL, i, (void*)&temp_Entry))
		{
			if (!temp_Entry.enable)
				continue;
			// check port mapping rule valid or not
			for(j=totalEntry-1;j>=0; j--)
			{
				if (i ==j )
					continue;
				if(!mib_chain_get(MIB_BR_WAN_TBL, j, (void*)&comp_Entry))
					continue;
#if defined(CONFIG_E8B) && defined(CONFIG_IPV6)
				if ( temp_Entry.IpProtocol != comp_Entry.IpProtocol)
					continue;
#endif
				for(k = PMAP_ETH0_SW0; k < PMAP_WLAN0 && k < ELANVIF_NUM; k++)
				{
					if(BIT_IS_SET(temp_Entry.itfGroup, k) && BIT_IS_SET(comp_Entry.itfGroup, k))
					{
						BIT_XOR_SET(temp_Entry.itfGroup, k);
						mib_chain_update(MIB_BR_WAN_TBL, (void*)&temp_Entry, i);
					}
				}
			}

			set_br_wan_pmap_rule(&temp_Entry, 1);
		} 
	}
	
	return 0;
}
//set up port/vlan mapping dev and rules
void setupBrWanMap()
{
	reset_br_wan_pmap();	

	exec_br_wan_portmp();
}
#endif
//this func make sure that the different apptype pvcs do not share same lanports
int check_itfGroup(MIB_CE_ATM_VC_Tp pEntry, MIB_CE_ATM_VC_Tp pOldEntry)
{
	uint32_t i;
	uint32_t cur_itfgroup;
	int32_t totalEntry;
	int ret;

	MIB_CE_ATM_VC_T temp_Entry;
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	int wanIndex = 0;
	unsigned int portMask = 0;
	
	if (pEntry->applicationtype&(X_CT_SRV_INTERNET | X_CT_SRV_SPECIAL_SERVICE_ALL)) {
			if(checkIPv4_IPv6_Dual_PolicyRoute_ex(pEntry, &wanIndex, &portMask)==1)
			{
				printf("\033[1;33;40m @@@@@@@@@@@@@@@@@########### %s:%d DO NOT rest port binding\033[m\n", __FUNCTION__, __LINE__);
				return 0;
			}
	}
#endif

	//for VoIP and TR069 only WAN ignore port binding
	if(pEntry->itfGroup && !(pEntry->applicationtype & ~(X_CT_SRV_TR069|X_CT_SRV_VOICE)))
		pEntry->itfGroup = 0;

	cur_itfgroup = pEntry->itfGroup;

	if(cur_itfgroup)
	{
		totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
		for(i = 0; i < totalEntry; ++i)
		{
			uint32_t sameport;
			ret = mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&temp_Entry);

			if(!ret)
				return -1;

			//if(pEntry->ifIndex == temp_Entry.ifIndex)
			if(pOldEntry && pOldEntry->ifIndex == temp_Entry.ifIndex)
				continue;

			sameport = (uint32_t)(cur_itfgroup & temp_Entry.itfGroup);
			if(sameport) {
				temp_Entry.itfGroup &=  (uint32_t)( ~(sameport));
				mib_chain_update(MIB_ATM_VC_TBL, (void*)&temp_Entry, i);
			}
		}  //end for()
	}
	/*****  when web UI modify port-mapping,re-do load-balance on this port *****/
#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
	extern uint32_t wan_diff_itfgroup;
	if(pOldEntry && pEntry)
		wan_diff_itfgroup = (pOldEntry->itfGroup)^(pEntry->itfGroup);
	else
		wan_diff_itfgroup = 0;
#endif
	/************************************/

	return 0;
}
#if defined(CONFIG_USER_NEW_BR_FOR_BRWAN)
int reset_untag_intf_to_br0(void)
{
	int32_t i, j;
	char ifname[32] = {0};
#ifdef WLAN_SUPPORT
	MIB_CE_MBSSIB_T mbssEntry;
	int orig_wlan_idx;
#endif
	for(j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j)
	{
		config_port_to_grouping(ELANVIF[j], BRIF);
	}
#if defined(WLAN_SUPPORT)
	orig_wlan_idx = wlan_idx;
	for (i = 0; i < NUM_WLAN_INTERFACE; i++)
	{
		wlan_idx = i;
		for (j = 0; j <= WLAN_MBSSID_NUM; j++)
		{
			if (!mib_chain_get(MIB_MBSSIB_TBL, j, (void *)&mbssEntry) || mbssEntry.wlanDisabled)
				continue;
			rtk_wlan_get_ifname(i, j, ifname);
			config_port_to_grouping(ifname, BRIF);
		}
	}
	wlan_idx = orig_wlan_idx;
#endif
	return 0;
}
int find_and_add_binded_intf(MIB_CE_ATM_VC_Tp pEntry, char *brname)
{
	uint16_t  itfGroup;
	int32_t j, k, totalPortbd = 0; 
	MIB_CE_PORT_BINDING_T portBindEntry;
	struct vlan_pair *vid_pair;
	const char *rifName = NULL;
	uint32_t logPort;
	char ifname[IFNAMSIZ+1] = {0};
#ifdef WLAN_SUPPORT
	int ssidIdx = -1;
#endif
	itfGroup = pEntry->itfGroup;
	//check eth-lan port bind
	for(j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j)
	{
		if(BIT_IS_SET(itfGroup, j))
			config_port_to_grouping(ELANVIF[j], brname);
	}
	//check wlan port bind	
	for(j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)
	{
		if(BIT_IS_SET(itfGroup, j))
			config_port_to_grouping(wlan[j-PMAP_WLAN0], brname);
	}
	//check vlan bind
	totalPortbd = mib_chain_total(MIB_PORT_BINDING_TBL);
	for (j = 0; j < totalPortbd; ++j)
	{
		if(!mib_chain_get(MIB_PORT_BINDING_TBL, j, (void*)&portBindEntry))
			continue;
		logPort = portBindEntry.port;
		if((unsigned char)VLAN_BASED_MODE == portBindEntry.pb_mode)
		{	
			vid_pair = (struct vlan_pair *)&portBindEntry.pb_vlan0_a;
			for (k=0; k<4; k++)
			{
				if(vid_pair[k].vid_a > 0 && pEntry->vid == vid_pair[k].vid_b)
				{		
					if(logPort < PMAP_WLAN0 && logPort < ELANVIF_NUM)
						rifName = ELANRIF[logPort];
#ifdef WLAN_SUPPORT
					else if((ssidIdx = (logPort-PMAP_WLAN0)) < (WLAN_SSID_NUM*NUM_WLAN_INTERFACE) && ssidIdx >= 0)
						rifName = wlan[ssidIdx];
#endif
					else 
						rifName = NULL;
					if(rifName == NULL || !getInFlags(rifName, NULL))
						continue;
					sprintf(ifname, "%s.%d", rifName, vid_pair[k].vid_a);
					config_port_to_grouping(ifname, brname);
				}
			}
		}
	}
	return 0;
}
int setup_new_bridge_interface(MIB_CE_ATM_VC_Tp pEntry, int mode)
{
	char brname[16] = {0}, wan_ifname[16] = {0}, macaddr[16] = {0};
	
	snprintf(brname, sizeof(brname), "br_%d", ETH_INDEX(pEntry->ifIndex));
	
	ifGetName( PHY_INTF(pEntry->ifIndex), wan_ifname, sizeof(wan_ifname));
	
	if(mode) //add new br
	{
		if (getInFlags(brname, NULL))
			va_cmd(IFCONFIG, 2, 1, brname, "down");
		else
			va_cmd(BRCTL, 2, 1, "addbr", brname);
		
		getMIB2Str(MIB_ELAN_MAC_ADDR, macaddr);
		
		if (macaddr[0])
			va_cmd(IFCONFIG, 4, 1, brname, "hw", "ether", macaddr);
		
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
		rtk_igmp_mld_snooping_module_init(brname, CONFIGALL, NULL);
#endif

		find_and_add_binded_intf(pEntry, brname);
		config_port_to_grouping(wan_ifname, brname);
		
		va_cmd(IFCONFIG, 2, 1, brname, "up");
	}
	else
	{
		if (!getInFlags(brname, NULL))
			return 0;
		
		va_cmd(IFCONFIG, 2, 1, brname, "down");
		va_cmd(BRCTL, 2, 1, "delbr", brname);
		
		config_port_to_grouping(wan_ifname, BRIF);
	}
	return 0;
}
#endif

#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
int isVlanMappingWAN(unsigned short vid)
{
	int32_t j, k, totalPortbd = 0;
	MIB_CE_PORT_BINDING_T portBindEntry;
	struct vlan_pair *vid_pair;

	totalPortbd = mib_chain_total(MIB_PORT_BINDING_TBL);
	for (j = 0; j < totalPortbd; ++j)
	{
		if(!mib_chain_get(MIB_PORT_BINDING_TBL, j, (void*)&portBindEntry))
			continue;

#if defined(CONFIG_E8B)
		if((unsigned char)VLAN_BASED_MODE == portBindEntry.pb_mode)
#endif
		{
			vid_pair = (struct vlan_pair *)&portBindEntry.pb_vlan0_a;
			for (k=0; k<4; k++)
			{
				if(vid_pair[k].vid_a > 0 && vid == vid_pair[k].vid_b) {
					printf("This WAN is vlan mapping WAN.\n");
					return 1;
				}
			}
		}
	}
	return 0;
}

void setup_vlan_map_intf(MIB_CE_PORT_BINDING_Tp pEntry, unsigned int port, int action)
{
	int32_t j, k, phyPortId=-1, wanPhyPortId=-1;
	struct vlan_pair *vid_pair;
	char cmd_str[256] = {0}, ifName[IFNAMSIZ+1] = {0};
	const char *rifName = NULL;
	const char *rrifName = NULL;
#if defined(CONFIG_RTL_SMUX_DEV)
	char alias_name[64] = {0};
#endif
#ifdef WLAN_SUPPORT
	int ssidIdx = -1;
#endif

	if(port < PMAP_WLAN0 && port < ELANVIF_NUM)
	{
		phyPortId = rtk_port_get_lan_phyID(port);
		wanPhyPortId = rtk_port_get_wan_phyID();
		if (phyPortId != -1 && phyPortId == wanPhyPortId)
			return;
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
		if(rtk_port_check_external_port(port)){
			rifName = EXTELANRIF[port];
			rrifName = ELANRIF[port];
		}
		else
#endif
		{
			rifName = ELANRIF[port];
			rrifName = ELANRIF[port];
		}
	}
#ifdef WLAN_SUPPORT
	else if((ssidIdx = (port-PMAP_WLAN0)) < (WLAN_SSID_NUM*NUM_WLAN_INTERFACE) && ssidIdx >= 0)
	{
		rifName = wlan[ssidIdx];
		rrifName = wlan[ssidIdx];
	}
#endif
	else 
		rifName = NULL;

	if(rifName == NULL || !getInFlags(rifName, NULL))
	{
		return;
	}

#if defined(CONFIG_E8B)
	if((unsigned char)VLAN_BASED_MODE == pEntry->pb_mode)
	{
		vid_pair = (struct vlan_pair *)&(pEntry->pb_vlan0_a);
		for (k=0; k<4; k++)
		{
			if (vid_pair[k].vid_a <= 0) continue;
			
			ifName[0] = '\0';
			sprintf(ifName, "%s.%d", rrifName, vid_pair[k].vid_a);

			if(getInFlags(ifName, NULL))
			{
				sprintf(cmd_str, "%s %s down", IFCONFIG, ifName);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#if defined(CONFIG_USER_NEW_BR_FOR_BRWAN) //revert vlan-dev to br0
				config_port_to_grouping(ifName, BRIF);
#endif
				sprintf(cmd_str, "%s delif %s %s", BRCTL, BRIF, ifName);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#if defined(CONFIG_RTL_SMUX_DEV)
				sprintf(cmd_str, "%s --if-delete %s", SMUXCTL, ifName);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#else
				sprintf(cmd_str, "%s rem %s", "/bin/vconfig", ifName);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#endif
			}
		
			if(action)
			{
#if defined(CONFIG_RTL_SMUX_DEV)				
				sprintf(cmd_str, "%s --if-create-name %s %s", SMUXCTL, rifName, ifName);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
				sprintf(alias_name, "VLAN_MAP_PORT%d_%d", port, k);
				for(j = 0; j <= 2; j++)
				{
					if(j == 0)
					{
						sprintf(cmd_str, "%s --if %s --tx --tags 0 --filter-txif %s --push-tag --set-vid %d 1 --set-rxif %s --rule-alias %s --rule-priority %d --target ACCEPT --rule-insert", 
										SMUXCTL, rifName, ifName, vid_pair[k].vid_a,rifName, alias_name, SMUX_RULE_PRIO_VLAN_MAP);
						va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					}
					else
					{
						sprintf(cmd_str, "%s --if %s --rx --tags %d --filter-vid %d %d --pop-tag --set-rxif %s --rule-alias %s --rule-priority %d --target ACCEPT --rule-insert", 
											SMUXCTL, rifName, j, vid_pair[k].vid_a, j, ifName, alias_name, SMUX_RULE_PRIO_VLAN_MAP);
						va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
						sprintf(cmd_str, "%s --if %s --tx --tags %d --filter-txif %s --set-vid %d %d --set-rxif %s --rule-alias %s --rule-priority %d --target ACCEPT --rule-insert", 
										SMUXCTL, rifName, j, ifName, vid_pair[k].vid_a, j, rifName, alias_name, SMUX_RULE_PRIO_VLAN_MAP);
						va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					}
				}
#else			
				sprintf(cmd_str, "/bin/vconfig add %s %d", rifName, vid_pair[k].vid_a);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#endif
				sprintf(cmd_str, "%s addif %s %s", BRCTL, BRIF, ifName);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
				sprintf(cmd_str, "%s %s up", IFCONFIG, ifName);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
			}
		}
	}
#endif
}
#endif

#ifdef SUPPORT_BRIDGE_VLAN_FILTER_PORTBIND
#define BRIDGE_VLAN_FILTER_START 4000
#define ROUTE_MIX_DEFAULT_RULE "ROUTE_MIX_DEFAULT_RULE"
#endif

void _set_pmap_rule(int action, MIB_CE_ATM_VC_Tp pEntry, uint16_t filter_vid, uint16_t bridge_mode_wan, 
							uint32_t wan_mark, uint32_t wan_mask, char *chain_out, const char *pifname) 
{
	int i;
	char mark_str[32];
	char buf[256];
	char *acStr, *acStr_I;
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	int wanIndex = 0;
	unsigned int portMask = 0;
#endif

	acStr = (action) ? "-A":"-D";
	acStr_I = (action) ? "-I":"-D";
	snprintf(mark_str, sizeof(mark_str), "0x%x", SOCK_FROM_LAN_SET(wan_mark, 1));
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	if (pEntry->applicationtype&(X_CT_SRV_INTERNET | X_CT_SRV_SPECIAL_SERVICE_ALL) && 
			(pEntry->IpProtocol & IPVER_IPV4_IPV6) != IPVER_IPV4_IPV6 && 
			checkIPv4_IPv6_Dual_PolicyRoute_ex(pEntry, &wanIndex, &portMask) == 1) 
	{
		if(pEntry->IpProtocol == IPVER_IPV6)
			va_cmd(EBTABLES, 14, 1, "-t", "broute", acStr, PORTMAP_BRTBL, "-p", "IPv6", "-i", pifname, "-j", "mark", "--mark-or", 
							mark_str, "--mark-target", "CONTINUE");
		else
			va_cmd(EBTABLES, 15, 1, "-t", "broute", acStr, PORTMAP_BRTBL, "!", "-p", "IPv6", "-i", pifname, "-j", "mark", "--mark-or", 
							mark_str, "--mark-target", "CONTINUE");
	}
	else 
#endif
	va_cmd(EBTABLES, 12, 1, "-t", "broute", acStr, PORTMAP_BRTBL, "-i", pifname, "-j", "mark", "--mark-or", 
							mark_str, "--mark-target", "CONTINUE");
							
	va_cmd(EBTABLES, 6, 1, acStr, chain_out, "-o", pifname, "-j", "RETURN");

	if(bridge_mode_wan)
	{
#ifdef SUPPORT_BRIDGE_VLAN_FILTER_PORTBIND
		if(action && filter_vid != 1) {
			snprintf(buf, sizeof(buf), "%s vlan add vid %d dev %s untagged pvid master", BIN_BRIDGE, filter_vid, pifname);
			va_cmd("/bin/sh", 2, 1, "-c", buf);
			snprintf(buf, sizeof(buf), "%s vlan add vid %d dev %s untagged self", BIN_BRIDGE, 1, pifname);
			va_cmd("/bin/sh", 2, 1, "-c", buf);
			if(bridge_mode_wan == 2) {
				unsigned char mac[MAC_ADDR_LEN];
				mib_get_s( MIB_ELAN_MAC_ADDR, (void *)mac, sizeof(mac));
				snprintf(buf, sizeof(buf), "smuxctl --if %s --set-if-rsmux", pifname);
				va_cmd("/bin/sh", 2, 1, "-c", buf);
				for(i=0; i<2; i++) {
					snprintf(buf, sizeof(buf), "smuxctl --if %s --rx --tags %d --filter-dmac %02X:%02X:%02X:%02X:%02X:%02X --reserve-add-vlan 1 --rule-alias %s --rule-priority 65535 --target CONTINUE --rule-append", 
												pifname, i, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ROUTE_MIX_DEFAULT_RULE);
					va_cmd("/bin/sh", 2, 1, "-c", buf);
				}
			}
		} else if(filter_vid != 1){
			snprintf(buf, sizeof(buf), "%s vlan del vid %d dev %s", BIN_BRIDGE, filter_vid, pifname);
			va_cmd("/bin/sh", 2, 1, "-c", buf);
			snprintf(buf, sizeof(buf), "%s vlan add vid %d dev %s untagged pvid master", BIN_BRIDGE, 1, pifname);
			va_cmd("/bin/sh", 2, 1, "-c", buf);
			if(bridge_mode_wan == 2) {
				for(i=0; i<2; i++) {
					snprintf(buf, sizeof(buf), "smuxctl --if %s --rx --tags %d --rule-remove-alias %s+", 
												pifname, i, ROUTE_MIX_DEFAULT_RULE);
					va_cmd("/bin/sh", 2, 1, "-c", buf);
				}
			}
		}
#endif
		
		//disableLanDhcp 
#ifdef CONFIG_E8B
		if(pEntry->disableLanDhcp == LAN_DHCP_ENABLE)  //get dhcp from ONU
		{
			va_cmd(EBTABLES, 6, 1, acStr, PORTMAP_INTBL, "-i", pifname, "-j", PORTMAP_SEVTBL2);
			va_cmd(EBTABLES, 6, 1, acStr, PORTMAP_OUTTBL, "-o", pifname, "-j", PORTMAP_SEVTBL2);
		}
		else    //get dhcp from WAN
#endif
		{
			va_cmd(EBTABLES, 6, 1, acStr, PORTMAP_INTBL, "-i", pifname, "-j", PORTMAP_SEVTBL);
			va_cmd(EBTABLES, 6, 1, acStr, PORTMAP_OUTTBL, "-o", pifname, "-j", PORTMAP_SEVTBL);
		}
	}
	else
	{
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
		if(pEntry->IpProtocol & IPVER_IPV6){
			va_cmd(IP6TABLES, 12, 1, acStr_I, FW_INPUT, "-i", pifname, "-p", "udp", "--sport", "546",
				"--dport", "547", "-j", FW_ACCEPT);
			va_cmd(EBTABLES, 14, 1, "-t", "broute", acStr_I, PORTMAP_BRTBL, "-i", pifname, "-p", "IPV6",
				"--ip6-protocol", "ipv6-icmp", "--ip6-icmp-type", "133", "-j", FW_DROP);
			/* Because with no binding LAN port, we will use br0 inf in radvd.conf. So we need to avoid unsolicited RA packet send to binding LAN port. */
			va_cmd(EBTABLES, 16, 1, acStr_I, IPV6_RA_LO_FILTER, "-o", pifname, "--logical-out", BRIF, "-d" , "Multicast", "-p", "IPV6",
				"--ip6-protocol", "ipv6-icmp", "--ip6-icmp-type", "134", "-j", FW_DROP);
		}
#endif	
#ifdef CONFIG_CU
		//disableLanDhcp should work on route mode
		if(pEntry->disableLanDhcp == LAN_DHCP_ENABLE)
		{
			va_cmd(EBTABLES, 6, 1, acStr, PORTMAP_INTBL, "-i", pifname, "-j", PORTMAP_SEVTBL_RT2);
			va_cmd(EBTABLES, 6, 1, acStr, PORTMAP_OUTTBL, "-o", pifname, "-j", PORTMAP_SEVTBL_RT2);
		}	
		else
		{
			va_cmd(EBTABLES, 6, 1, acStr, PORTMAP_INTBL, "-i", pifname, "-j", PORTMAP_SEVTBL_RT);
			va_cmd(EBTABLES, 6, 1, acStr, PORTMAP_OUTTBL, "-o", pifname, "-j", PORTMAP_SEVTBL_RT);
		}
#endif
	}
}

void set_pmap_rule(MIB_CE_ATM_VC_Tp pEntry, int action)
{
	unsigned int ifIndex;
	char ifname[IFNAMSIZ+1], wan_phy_ifname[IFNAMSIZ+1], wan_ifname[IFNAMSIZ+1], wan_l3ifname[IFNAMSIZ+1];
	char chain_out[64], chain_in[64], polmark[32], polmark_mask[64], wan_type[IFNAMSIZ+1], tmp_str[64];
	const char *pifname;
	int32_t j;
	uint32_t itfGroup, hit_entry = 0;
	uint32_t wan_mark = 0, wan_mask = 0;
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
	int32_t totalPortbd ,k;
	MIB_CE_PORT_BINDING_T portBindEntry;
	struct vlan_pair *vid_pair;
#endif
    unsigned char ignore_dhcp_on_pppoe_bridge=0;
#if defined(CONFIG_IPV6)
	char chain_ipv6_ra_in[64];
#endif
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
	const char *rifName = NULL;
	uint32_t logPort;
#ifdef WLAN_SUPPORT
	int ssidIdx = -1;
#endif
#endif
	char buf[128] = {0};
	uint16_t filter_vid = 1, bridge_mode_wan = 0;
	
	if(!(pEntry->applicationtype & ~(X_CT_SRV_TR069/*|X_CT_SRV_VOICE*/)))
		return;

	memset(wan_ifname, 0, sizeof(wan_ifname));

	itfGroup	= pEntry->itfGroup;
	ifIndex		= pEntry->ifIndex;

	if(ROUTE_CHECK_BRIDGE_MIX_ENABLE(pEntry))
	{
		ifGetName(PHY_INTF(ifIndex), wan_phy_ifname, sizeof(wan_phy_ifname));
		snprintf(wan_ifname, sizeof(wan_ifname), "%s%s", wan_phy_ifname, ROUTE_BRIDGE_WAN_SUFFIX);
		bridge_mode_wan = 2;
	}
	else {
		ifGetName(PHY_INTF(ifIndex), wan_phy_ifname, sizeof(wan_phy_ifname));
		strcpy(wan_ifname, wan_phy_ifname);
		if(pEntry->cmode == CHANNEL_MODE_BRIDGE)
			bridge_mode_wan = 1;
	}
	ifGetName(pEntry->ifIndex, wan_l3ifname, sizeof(wan_l3ifname));
	rtk_net_get_wan_policy_route_mark(wan_l3ifname, &wan_mark, &wan_mask);

	if (MEDIA_INDEX(pEntry->ifIndex) == MEDIA_ATM)
		sprintf(wan_type, "%s+", ALIASNAME_VC);
	else if (MEDIA_INDEX(pEntry->ifIndex) == MEDIA_PTM)
		sprintf(wan_type, "%s+", ALIASNAME_PTM0);
	else if (MEDIA_INDEX(pEntry->ifIndex) == MEDIA_ETH)
		sprintf(wan_type, "%s+", ALIASNAME_NAS0);
	else
		sprintf(wan_type, "%s+", ALIASNAME_NAS0);

	//rtk_net_get_from_lan_with_wan_policy_route_mark(wan_l3ifname, &lan_mark, &lan_mask);
#ifdef SUPPORT_BRIDGE_VLAN_FILTER_PORTBIND
#if defined(CONFIG_YUEME)  //for internet route+bridge WAN not with special VLAN
	if((pEntry->applicationtype & X_CT_SRV_INTERNET) && bridge_mode_wan == 2)
		filter_vid = 1;
	else
#endif
	filter_vid = BRIDGE_VLAN_FILTER_START+SOCK_MARK_MARK_TO_WAN_INDEX(wan_mark);
#endif

	snprintf(polmark, sizeof(polmark), "0x%x", wan_mark);
	snprintf(polmark_mask, sizeof(polmark_mask), "0x%x/0x%x", wan_mark, wan_mask);
	snprintf(chain_out, sizeof(chain_out), "%s_%s_out", PORTMAP_EBTBL, wan_phy_ifname);
	snprintf(chain_in, sizeof(chain_in), "%s_%s_in", PORTMAP_EBTBL, wan_phy_ifname);

	va_cmd(EBTABLES, 2, 1, "-F", chain_out);
	va_cmd(EBTABLES, 2, 1, "-F", chain_in);
	
	if(action)
	{
		va_cmd(EBTABLES, 2, 1, "-N", chain_out);
		va_cmd(EBTABLES, 3, 1, "-P", chain_out, "DROP");
		va_cmd(EBTABLES, 2, 1, "-N", chain_in);
		va_cmd(EBTABLES, 3, 1, "-P", chain_in, "DROP");
		
#ifdef CONFIG_E8B
		if (bridge_mode_wan)
		{
#if defined(CONFIG_IPV6)
			if(pEntry->IpProtocol == IPVER_IPV4) { 
				va_cmd(EBTABLES, 8, 1, "-I", chain_out, "-o", wan_ifname, "-p", "ipv6", "-j", (char *)FW_DROP);
			}
			else if(pEntry->IpProtocol == IPVER_IPV6) {
				va_cmd(EBTABLES, 8, 1, "-I", chain_out, "-o", wan_ifname, "-p", "ipv4", "-j", (char *)FW_DROP);
			}
#endif
			if(pEntry->disableLanDhcp == LAN_DHCP_ENABLE) { //DROP DHCP from wan, if get IP by lan
				va_cmd(EBTABLES, 11, 1, "-I", chain_out, "1", "-p", "ipv4",
					"--ip-proto", "udp", "--ip-dport", "67:68", "-j", (char *)FW_DROP);
#if defined(CONFIG_IPV6)
				va_cmd(EBTABLES, 11, 1, "-I", chain_out, "1", "-p", "ipv6", 
					"--ip6-proto", "udp", "--ip6-dport", "546:547", "-j", (char *)FW_DROP);
				va_cmd(EBTABLES, 11, 1, "-I", chain_out, "1", "-p", "ipv6",
					"--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "router-advertisement", "-j", (char *)FW_DROP);
#endif
			}
		}
#endif

#if defined(CONFIG_USER_NEW_BR_FOR_BRWAN)
	    if(bridge_mode_wan) {
			if(pEntry->newbr == 1)
			{
				setup_new_bridge_interface(pEntry, 1);  //create new br and add interface, no need to add rules
				return;
			}
			else
				setup_new_bridge_interface(pEntry, 0);
		}
#endif
	}

	if (!(pEntry->applicationtype & X_CT_SRV_INTERNET))
	{
		hit_entry = 1;
	}

	for(j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j)
	{
		if(BIT_IS_SET(itfGroup, j))
		{
			hit_entry++;
			pifname = ELANVIF[j];
			_set_pmap_rule(action, pEntry, filter_vid, bridge_mode_wan, wan_mark, wan_mask, chain_out, pifname);
		}
	}
	
	for(j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)
	{
		if(BIT_IS_SET(itfGroup, j))
		{
			hit_entry++;
			pifname = wlan[j-PMAP_WLAN0];
			_set_pmap_rule(action, pEntry, filter_vid, bridge_mode_wan, wan_mark, wan_mask, chain_out, pifname);
		}
	}

#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
	totalPortbd = mib_chain_total(MIB_PORT_BINDING_TBL);
	//polling every lanport vlan-mapping entry
	for (j = 0; j < totalPortbd; ++j)
	{
		if(!mib_chain_get(MIB_PORT_BINDING_TBL, j, (void*)&portBindEntry))
			continue;
		logPort = portBindEntry.port;
		if((unsigned char)VLAN_BASED_MODE == portBindEntry.pb_mode)
		{
			vid_pair = (struct vlan_pair *)&portBindEntry.pb_vlan0_a;
			for (k=0; k<4; k++)
			{
				//support lan/wlan vlan bind untag or tag wan 
				if(!(vid_pair[k].vid_a > 0 && pEntry->vid == vid_pair[k].vid_b))
				{
					continue;
				}
				
				if(logPort < PMAP_WLAN0 && logPort < ELANVIF_NUM)
					rifName = ELANRIF[logPort];
#ifdef WLAN_SUPPORT
				else if((ssidIdx = (logPort-PMAP_WLAN0)) < (WLAN_SSID_NUM*NUM_WLAN_INTERFACE) && ssidIdx >= 0)
					rifName = wlan[ssidIdx];
#endif
				else 
					rifName = NULL;

				if(rifName == NULL || !getInFlags(rifName, NULL))
					continue;

				ifname[0] = '\0';
				sprintf(ifname, "%s.%d", rifName, vid_pair[k].vid_a);
				if(!getInFlags(ifname, NULL))
					continue;
				
				hit_entry = 1;
				pifname = ifname;
				_set_pmap_rule(action, pEntry, filter_vid, bridge_mode_wan, wan_mark, wan_mask, chain_out, pifname);
			}
		}
	}
#endif

	if(action)
	{
		if(hit_entry > 0)
		{
			if(bridge_mode_wan)
			{
#ifdef SUPPORT_BRIDGE_VLAN_FILTER_PORTBIND
				if(filter_vid != 1) {
					snprintf(buf, sizeof(buf), "%s vlan add vid %d dev %s untagged pvid master", BIN_BRIDGE, filter_vid, wan_ifname);
					va_cmd("/bin/sh", 2, 1, "-c", buf);
					snprintf(buf, sizeof(buf), "%s vlan del vid %d dev %s", BIN_BRIDGE, 1, wan_ifname);
					va_cmd("/bin/sh", 2, 1, "-c", buf);
					snprintf(buf, sizeof(buf), "%s vlan add vid %d dev %s untagged self", BIN_BRIDGE, filter_vid, BRIF);
					va_cmd("/bin/sh", 2, 1, "-c", buf);
				}
#endif
				va_cmd(EBTABLES, 6, 1, "-A", chain_out, "-o", wan_ifname, "-j", "RETURN");
				va_cmd(EBTABLES, 6, 1, "-A", chain_in, "--mark", polmark_mask, "-j", "RETURN");
#if defined(CONFIG_YUEME)  //for internet route+bridge WAN not with special VLAN
				if((pEntry->applicationtype & X_CT_SRV_INTERNET) && bridge_mode_wan == 2) {
					va_cmd(EBTABLES, 7, 1, "-A", chain_out, "-o", "!", wan_type, "-j", "RETURN");
					snprintf(tmp_str, sizeof(tmp_str), "0x%x/0x%x", 0, wan_mask);
					va_cmd(EBTABLES, 6, 1, "-A", chain_in, "--mark", tmp_str, "-j", "RETURN");
				}
#endif	
				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_INTBL, "-i", wan_ifname, "-j", PORTMAP_SEVTBL);
				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", wan_ifname, "-j", PORTMAP_SEVTBL);
				
				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-o", wan_ifname, "-j", chain_in);
				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "--mark", polmark_mask, "-j", chain_out);
#if 0 //FIXME, no porting 				
				mib_get(MIB_IGNORE_DHCP_ON_PPPOE_BRIDGE,&ignore_dhcp_on_pppoe_bridge);
                if(pEntry->brmode == BRIDGE_PPPOE && ignore_dhcp_on_pppoe_bridge)
                {
                    //pppoe bridge drop dhcp packet
                    va_cmd(EBTABLES, 12, 1, "-I", chain_out, "-o", wan_ifname, "-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "DROP");
                }
#endif				
			}
			else // for routing case drop to bridge WAN
			{
				va_cmd(EBTABLES, 8, 1, "-A", PORTMAP_EBTBL, "--mark", polmark_mask, "-o", wan_type, "-j", "DROP");
			}

			va_cmd(EBTABLES, 8, 1, "-A", PORTMAP_OUTTBL, "--mark", polmark_mask, "-d", "Multicast", "-j", chain_out); 
		}
		else
		{// if no any port or vlan hit, clear ebtables rules
			va_cmd(EBTABLES, 2, 1, "-F", chain_out);
			va_cmd(EBTABLES, 2, 1, "-X", chain_out);
			va_cmd(EBTABLES, 2, 1, "-F", chain_in);
			va_cmd(EBTABLES, 2, 1, "-X", chain_in);
		}
		
		if(bridge_mode_wan) {
			//for bridge WAN downstream mark
			va_cmd(EBTABLES, 12, 1, "-t", "broute", "-A", PORTMAP_BRTBL, "-i", wan_ifname, "-j", "mark", "--mark-or", polmark, "--mark-target", "CONTINUE");;
		}
		//for routing WAN downstream mark
		va_cmd(IPTABLES, 10, 1, "-t", "mangle", "-A", (char *)PORTMAP_RTTBL, "-i", wan_l3ifname, "-j", "MARK", "--set-mark", polmark_mask);
#if defined(CONFIG_IPV6)
		va_cmd(IP6TABLES, 10, 1, "-t", "mangle", "-A", (char *)PORTMAP_RTTBL, "-i", wan_l3ifname, "-j", "MARK", "--set-mark", polmark_mask);
#endif
		if(pEntry->cmode == CHANNEL_MODE_PPPOE)
		{
			//for Multicast downstream mark when packet without ppp header
			va_cmd(IPTABLES, 10, 1, "-t", "mangle", "-A", (char *)PORTMAP_RTTBL, "-i", wan_phy_ifname, "-j", "MARK", "--set-mark", polmark_mask);
#if defined(CONFIG_IPV6)
			va_cmd(IP6TABLES, 10, 1, "-t", "mangle", "-A", (char *)PORTMAP_RTTBL, "-i", wan_phy_ifname, "-j", "MARK", "--set-mark", polmark_mask);
#endif
		}
	}
	else 
	{
		if(bridge_mode_wan)
		{
#ifdef SUPPORT_BRIDGE_VLAN_FILTER_PORTBIND
			if(filter_vid != 1) {
				snprintf(buf, sizeof(buf), "%s vlan del vid %d dev %s", BIN_BRIDGE, filter_vid, wan_ifname);
				va_cmd("/bin/sh", 2, 1, "-c", buf);
				snprintf(buf, sizeof(buf), "%s vlan add vid %d dev %s untagged pvid master", BIN_BRIDGE, 1, wan_ifname);
				va_cmd("/bin/sh", 2, 1, "-c", buf);
				snprintf(buf, sizeof(buf), "%s vlan del vid %d dev %s self", BIN_BRIDGE, filter_vid, BRIF);
				va_cmd("/bin/sh", 2, 1, "-c", buf);
			}
#endif
			va_cmd(EBTABLES, 6, 1, "-D", PORTMAP_INTBL, "-i", wan_ifname, "-j", PORTMAP_SEVTBL);
			va_cmd(EBTABLES, 6, 1, "-D", PORTMAP_OUTTBL, "-o", wan_ifname, "-j", PORTMAP_SEVTBL);
			va_cmd(EBTABLES, 6, 1, "-D", PORTMAP_EBTBL, "-o", wan_ifname, "-j", chain_in);
			va_cmd(EBTABLES, 6, 1, "-D", PORTMAP_EBTBL, "--mark", polmark_mask, "-j", chain_out);
			//va_cmd(EBTABLES, 12, 1, "-D", chain_out, "-o", wan_ifname, "-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "DROP");
			va_cmd(EBTABLES, 12, 1, "-t", "broute", "-D", PORTMAP_BRTBL, "-i", wan_ifname, "-j", "mark", "--mark-or", polmark, "--mark-target", "CONTINUE");
		}
		else 
		{
			va_cmd(EBTABLES, 8, 1, "-D", PORTMAP_EBTBL, "--mark", polmark_mask, "-o", wan_type, "-j", "DROP");
		}
		va_cmd(EBTABLES, 8, 1, "-D", PORTMAP_OUTTBL, "--mark", polmark_mask, "-d", "Multicast", "-j", chain_out); 
		va_cmd(IPTABLES, 10, 1, "-t", "mangle", "-D", (char *)PORTMAP_RTTBL, "-i", wan_l3ifname, "-j", "MARK", "--set-mark", polmark_mask);
#if defined(CONFIG_IPV6)
		va_cmd(IP6TABLES, 10, 1, "-t", "mangle", "-D", (char *)PORTMAP_RTTBL, "-i", wan_l3ifname, "-j", "MARK", "--set-mark", polmark_mask);
#endif
		if(pEntry->cmode == CHANNEL_MODE_PPPOE)
		{
			//for Multicast downstream mark when packet without ppp header
			va_cmd(IPTABLES, 10, 1, "-t", "mangle", "-D", (char *)PORTMAP_RTTBL, "-i", wan_phy_ifname, "-j", "MARK", "--set-mark", polmark_mask);
#if defined(CONFIG_IPV6)
			va_cmd(IP6TABLES, 10, 1, "-t", "mangle", "-D", (char *)PORTMAP_RTTBL, "-i", wan_phy_ifname, "-j", "MARK", "--set-mark", polmark_mask);
#endif
		}
		
		va_cmd(EBTABLES, 2, 1, "-F", chain_out);
		va_cmd(EBTABLES, 2, 1, "-X", chain_out);
		va_cmd(EBTABLES, 2, 1, "-F", chain_in);
		va_cmd(EBTABLES, 2, 1, "-X", chain_in);
	}
}


#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
void rtk_pmap_filter_unexist_wan(void)
{
	MIB_CE_ATM_VC_T temp_Entry;
	MIB_CE_PORT_BINDING_T portBindEntry;
	int32_t totalPortbd ,i, k, j, totalEntry;
	uint32_t logPort;
	struct vlan_pair *vid_pair;
	int32_t isExist=0;
	char ifname[IFNAMSIZ+1];
	const char *rif = NULL;
#ifdef WLAN_SUPPORT
	int ssidIdx = -1;
#endif

	totalPortbd = mib_chain_total(MIB_PORT_BINDING_TBL);
	//polling every lanport vlan-mapping entry
	for (j = 0; j < totalPortbd; ++j)
	{
		if(!mib_chain_get(MIB_PORT_BINDING_TBL, j, (void*)&portBindEntry))
			continue;
		logPort = portBindEntry.port;
#if defined(CONFIG_E8B)
		if((unsigned char)VLAN_BASED_MODE == portBindEntry.pb_mode)
		{	
			vid_pair = (struct vlan_pair *)&portBindEntry.pb_vlan0_a;
			totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
			for (k=0; k<4; k++)
			{
				if(!(vid_pair[k].vid_a > 0 && vid_pair[k].vid_b > 0)
				){
					continue;
				}
				isExist=0;
				for(i = 0; i < totalEntry; ++i)
				{
					if(mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&temp_Entry))
					{
						if (!temp_Entry.enable)
							continue;
						if(vid_pair[k].vid_b != temp_Entry.vid)
							continue;
						
						isExist = 1;
					}
				}

				if(isExist == 0)
				{
					if(logPort < PMAP_WLAN0 && logPort < ELANVIF_NUM)
						rif = ELANRIF[logPort];
#ifdef WLAN_SUPPORT
					else if((ssidIdx = (logPort-PMAP_WLAN0)) < (WLAN_SSID_NUM*NUM_WLAN_INTERFACE) && ssidIdx >= 0)
						rif = wlan[ssidIdx];
#endif
					else 
						rif = NULL;
					
					if(rif != NULL && getInFlags(rif, NULL))
					{
						sprintf(ifname, "%s.%d", rif, vid_pair[k].vid_a);
						va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-i", ifname, "-j", "DROP");
					}
				}
			}
		} 
#endif
	}
}
#endif

//execute the port-mapping commands
int exec_portmp()
{
	int32_t totalEntry;
	MIB_CE_ATM_VC_T temp_Entry, comp_Entry;
	int32_t i,j=0,k=0;
	char buf[256] = {0};
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
	MIB_CE_PORT_BINDING_T portBindEntry;
#endif
	
	//ebtables -N portmapping_input
	va_cmd(EBTABLES, 2, 1, "-N", (char *)PORTMAP_INTBL);
	//ebtables -P portmapping_input RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)PORTMAP_INTBL, "RETURN");
	//ebtables -A INPUT -j portmapping_input
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)PORTMAP_INTBL);
	
	//ebtables -N portmapping_output
	va_cmd(EBTABLES, 2, 1, "-N", (char *)PORTMAP_OUTTBL);
	//ebtables -P portmapping_output RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)PORTMAP_OUTTBL, "RETURN");
#ifdef CONFIG_YUEME
	//ebtables -A portmapping_service2 --logical-out br+ -p ipv4 --ip-proto igmp -j RETURN ## accept igmp query
	va_cmd(EBTABLES, 13, 1, "-A", PORTMAP_OUTTBL, "--logical-out", "br+", "-o", "!", "nas0+", "-p", "ipv4", "--ip-proto", "igmp", "-j", "RETURN");
#ifdef CONFIG_IPV6 
	//ebtables -A portmapping_service2 --logical-out br+ -p ipv6 --ip6-proto ipv6-icmp --ip6-icmp-type 130 -j RETURN ## accept mld query
	va_cmd(EBTABLES, 15, 1, "-A", PORTMAP_OUTTBL, "--logical-out", "br+", "-o", "!", "nas0+", "-p", "ipv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "130", "-j", "RETURN");
#endif
#endif
	//ebtables -A INPUT -j portmapping_output
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-j", (char *)PORTMAP_OUTTBL);
	
	//ebtables -N portmapping_service
	va_cmd(EBTABLES, 2, 1, "-N", (char *)PORTMAP_SEVTBL);
	//ebtables -P portmapping_service RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)PORTMAP_SEVTBL, "RETURN");

	//ebtables -A portmapping_service -p ipv4 --ip-proto udp --ip-dport 67:68 -j DROP ## block DHCP
#if defined(CONFIG_USER_DHCPCLIENT_MODE)
	char dhcp_mode = 0;
	mib_get_s( MIB_DHCP_MODE, (void *)&dhcp_mode, sizeof(dhcp_mode));
	if (dhcp_mode != DHCPV4_LAN_CLIENT)
#endif
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL, "-p", "ipv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "DROP");
	//ebtables -A portmapping_service -p ipv4 --pkttype-type multicast -j DROP ## block IGMP 
	va_cmd(EBTABLES,  8, 1, "-A", PORTMAP_SEVTBL, "-p", "ipv4", "--ip-proto", "igmp", "-j", "DROP");
	//ebtables -A portmapping_service -p ipv4 --pkttype-type multicast -j DROP ## block multicast data
	va_cmd(EBTABLES,  8, 1, "-A", PORTMAP_SEVTBL, "-p", "ipv4", "--pkttype-type", "multicast", "-j", "DROP");
#ifdef CONFIG_IPV6
	//ebtables -A portmapping_service -p ipv6 --ip6-proto udp --ip6-dport 546:547 -j DROP ## block DHCPv6
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL, "-p", "ipv6", "--ip6-proto", "udp", "--ip6-dport", "546:547", "-j", "DROP");
	//ebtables -A portmapping_service -p ipv6 --ip6-proto ipv6-icmp --ip6-icmp-type 130:134 -j DROP ## block MLD, RS, RA
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL, "-p", "ipv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "130:134", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL, "-p", "ipv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "143", "-j", "DROP");
	//ebtables -A portmapping_service -p ipv6 --ip6-proto ! ipv6-icmp --pkttype-type multicast -j DROP ## block multicast data
	va_cmd(EBTABLES, 11, 1, "-A", PORTMAP_SEVTBL, "-p", "ipv6", "--ip6-proto", "!", "ipv6-icmp", "--pkttype-type", "multicast", "-j", "DROP");
#endif

#ifdef CONFIG_E8B
	//accept DHCP
	//ebtables -N portmapping_service2 
	va_cmd(EBTABLES, 2, 1, "-N", (char *)PORTMAP_SEVTBL2);
	//ebtables -P portmapping_service RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)PORTMAP_SEVTBL2, "RETURN");
	//ebtables -A portmapping_service2 -p ipv4 --ip-proto udp --ip-dport 67:68 -j RETURN ## accept DHCP
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL2, "-p", "ipv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "RETURN");
	//ebtables -A portmapping_service -p ipv4 --pkttype-type multicast -j DROP ## block IGMP 
	va_cmd(EBTABLES,  8, 1, "-A", PORTMAP_SEVTBL2, "-p", "ipv4", "--ip-proto", "igmp", "-j", "DROP");
	//ebtables -A portmapping_service2 -p ipv4 --pkttype-type multicast -j DROP ## block multicast data
	va_cmd(EBTABLES,  8, 1, "-A", PORTMAP_SEVTBL2, "-p", "ipv4", "--pkttype-type", "multicast", "-j", "DROP");
#ifdef CONFIG_IPV6
	//ebtables -A portmapping_service2 -p ipv6 --ip6-proto udp --ip6-dport 546:547 -j RETURN ## accept DHCPv6
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL2, "-p", "ipv6", "--ip6-proto", "udp", "--ip6-dport", "546:547", "-j", "RETURN");
	//ebtables -A portmapping_service2 -p ipv6 --ip6-proto ipv6-icmp --ip6-icmp-type 130:134 -j RETURN ## accept MLD, RS, RA
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL2, "-p", "ipv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "130:134", "-j", "RETURN");
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL2, "-p", "ipv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "143", "-j", "DROP");
	//ebtables -A portmapping_service2 -p ipv6 --ip6-proto ! ipv6-icmp --pkttype-type multicast -j DROP ## block multicast data
	va_cmd(EBTABLES, 11, 1, "-A", PORTMAP_SEVTBL2, "-p", "ipv6", "--ip6-proto", "!", "ipv6-icmp", "--pkttype-type", "multicast", "-j", "DROP");
#endif


#ifdef CONFIG_CU
	//ebtables -N portmapping_service_rt
	va_cmd(EBTABLES, 2, 1, "-N", (char *)PORTMAP_SEVTBL_RT);
	//ebtables -P portmapping_service_rt RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)PORTMAP_SEVTBL_RT, "RETURN");
	//ebtables -A portmapping_service_rt -p ipv4 --ip-proto udp --ip-dport 67:68 -j DROP ## block DHCP
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL_RT, "-p", "ipv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "DROP");
#ifdef CONFIG_IPV6
	//ebtables -A portmapping_service -p ipv6 --ip6-proto udp --ip6-dport 546:547 -j DROP ## block DHCPv6
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL_RT, "-p", "ipv6", "--ip6-proto", "udp", "--ip6-dport", "546:547", "-j", "DROP");
#endif
	//accept DHCP
	//ebtables -N portmapping_service_rt2
	va_cmd(EBTABLES, 2, 1, "-N", (char *)PORTMAP_SEVTBL_RT2);
	//ebtables -P portmapping_service_rt2 RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)PORTMAP_SEVTBL_RT2, "RETURN");
	//ebtables -A portmapping_service_rt2 -p ipv4 --ip-proto udp --ip-dport 67:68 -j DROP ## block DHCP
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL_RT2, "-p", "ipv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "RETURN");
#ifdef CONFIG_IPV6
	//ebtables -A portmapping_service_rt2 -p ipv6 --ip6-proto udp --ip6-dport 546:547 -j DROP ## block DHCPv6
	va_cmd(EBTABLES, 10, 1, "-A", PORTMAP_SEVTBL_RT2, "-p", "ipv6", "--ip6-proto", "udp", "--ip6-dport", "546:547", "-j", "RETURN");
#endif
#endif
#endif
	//ebtables -N portmapping
	va_cmd(EBTABLES, 2, 1, "-N", (char *)PORTMAP_EBTBL);
	//ebtables -P portmapping RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)PORTMAP_EBTBL, "RETURN");
	//ebtables -A FORWARD -j portmapping
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)PORTMAP_EBTBL);

	// ebtables -t broute -N portmapping_broute
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N", (char *)PORTMAP_BRTBL);
	// ebtables -t broute -F portmapping_broute
	va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P", (char *)PORTMAP_BRTBL, "RETURN");
	// clear WAN and LAN mark
	snprintf(buf, sizeof(buf), "%s -t broute -D %s -j mark --mark-and 0x%x", EBTABLES, PORTMAP_BRTBL, ~(SOCK_MARK_WAN_INDEX_BIT_MASK|SOCK_FROM_LAN_BIT_MASK));
	va_cmd("/bin/sh", 2, 1, "-c", buf);
	snprintf(buf, sizeof(buf), "%s -t broute -I %s -j mark --mark-and 0x%x", EBTABLES, PORTMAP_BRTBL, ~(SOCK_MARK_WAN_INDEX_BIT_MASK|SOCK_FROM_LAN_BIT_MASK));
	va_cmd("/bin/sh", 2, 1, "-c", buf);
	// ebtables -t broute -I BROUTING -j portmapping_broute
	va_cmd(EBTABLES, 6, 1, "-t", "broute", "-A", "BROUTING", "-j", (char *)PORTMAP_BRTBL);
	
	// for filter WAN routing packet
	// iptables -t mangle -N portmapping_rt_tbl
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", PORTMAP_RTTBL);
	// iptables -t mangle -A PREROUTING -j portmapping_rt_tbl
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", (char *)FW_PREROUTING, "-j", PORTMAP_RTTBL);
#ifdef CONFIG_IPV6
	// ip6tables -t mangle -N portmapping_rt_tbl
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", PORTMAP_RTTBL);
	// ip6tables -t mangle -A PREROUTING -j portmapping_rt_tbl
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", (char *)FW_PREROUTING, "-j", PORTMAP_RTTBL);
	//IPv6 RA filter to each LAN interface. Avoid br0 RA packet send to eth0.x.
	// ebtables -N ipv6_ra_local_out
	va_cmd(EBTABLES, 2, 1, "-N", (char *)IPV6_RA_LO_FILTER);
	// ebtables -P ipv6_ra_local_out RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)IPV6_RA_LO_FILTER, (char *)FW_RETURN);
	// ebtables -A OUTPUT-j ipv6_ra_local_out
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-j", (char *)IPV6_RA_LO_FILTER);
#endif

#ifdef SUPPORT_BRIDGE_VLAN_FILTER_PORTBIND
	{
		MIB_L2BRIDGE_GROUP_T l2br_entry = {0};
		if(get_grouping_entry_by_groupnum(0, &l2br_entry, NULL,0) == 0)
		{
			snprintf(buf, sizeof(buf), "echo %d > /sys/class/net/%s/bridge/vlan_filtering", (l2br_entry.vlan_filtering)?1:0, BRIF);
			va_cmd("/bin/sh", 2, 1, "-c", buf);
		}
	}
#endif

#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
	totalEntry = mib_chain_total(MIB_PORT_BINDING_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_PORT_BINDING_TBL, i, (void*)&portBindEntry))
		{
			setup_vlan_map_intf(&portBindEntry, portBindEntry.port, 1);
		} 
	}
	
	rtk_pmap_filter_unexist_wan();
#endif
	
	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&temp_Entry))
		{
			if (!temp_Entry.enable)
				continue;
			// check port mapping rule valid or not
			for(j=totalEntry-1;j>=0; j--)
			{
				if (i ==j )
					continue;
				if(!mib_chain_get(MIB_ATM_VC_TBL, j, (void*)&comp_Entry))
					continue;
#if defined(CONFIG_E8B) && defined(CONFIG_IPV6)
				if ( temp_Entry.IpProtocol != comp_Entry.IpProtocol)
					continue;
#endif
				for(k = PMAP_ETH0_SW0; k < PMAP_WLAN0 && k < ELANVIF_NUM; k++)
				{
					if(BIT_IS_SET(temp_Entry.itfGroup, k) && BIT_IS_SET(comp_Entry.itfGroup, k))
					{
						BIT_XOR_SET(temp_Entry.itfGroup, k);
						mib_chain_update(MIB_ATM_VC_TBL, (void*)&temp_Entry, i);
					}
				}
			}

			set_pmap_rule(&temp_Entry, 1);
		} 
	}

#ifdef  CONFIG_VIR_BOA_JEMBENCHTEST
	rtk_cmcc_do_jspeedup_ex();
#endif
	setTmpBlockBridgeFloodPktRule(0);
#ifdef CONFIG_CU
	setTmpDropBridgeDhcpPktRule(0);
#endif
#ifdef CONFIG_RTK_MAC_AUTOBINDING
		do{
			char tmp_cmd[128]={0};
			snprintf(tmp_cmd, sizeof(tmp_cmd), "/bin/echo %d  > /proc/mac_autobinding/mac_operation", PROCFS_MAC_AUTOBINDING_ENTRY_RENOTIFY);
			va_cmd("/bin/sh", 2, 1, "-c", tmp_cmd);
		}while(0);
		
#endif

	return 0;
}

#ifdef CONFIG_CU
void setTmpDropBridgeDhcpPktRule(int act)
{
	if(act == 1)
	{
		va_cmd(EBTABLES, 10, 1, "-A", FW_FORWARD, "-p", "ipv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "DROP");
		va_cmd(EBTABLES, 10, 1, "-A", FW_FORWARD, "-p", "ipv6", "--ip6-proto", "udp", "--ip6-dport", "546:547", "-j", "DROP");
		va_cmd(EBTABLES, 10, 1, "-A", FW_FORWARD, "-p", "ipv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "130:134", "-j", "DROP");
		va_cmd(EBTABLES, 10, 1, "-A", FW_FORWARD, "-p", "ipv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "143", "-j", "DROP");
		//va_cmd(EBTABLES, 1, 1, "-L");
	}
	else if(act == 0)
	{
		va_cmd(EBTABLES, 10, 1, "-D", FW_FORWARD, "-p", "ipv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "DROP");
		va_cmd(EBTABLES, 10, 1, "-D", FW_FORWARD, "-p", "ipv6", "--ip6-proto", "udp", "--ip6-dport", "546:547", "-j", "DROP");
		va_cmd(EBTABLES, 10, 1, "-D", FW_FORWARD, "-p", "ipv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "130:134", "-j", "DROP");
		va_cmd(EBTABLES, 10, 1, "-D", FW_FORWARD, "-p", "ipv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "143", "-j", "DROP");
		//va_cmd(EBTABLES, 1, 1, "-L");
	}
	else
		return;
}
#endif

int set_vc_fgroup()
{
	int i;
	int vcIdx;
	struct data_to_pass_st msg;
	char wanif[16];
	MEDIA_TYPE_T mType;

	get_pmap_fgroup(pmap_list, MAX_VC_NUM);

	for (i=0; i<MAX_VC_NUM; i++) {
		if (!pmap_list[i].valid)
			continue;
		mType = MEDIA_INDEX(pmap_list[i].ifIndex);
		if ( mType == MEDIA_ATM)
		{
			/*august:20120330
			we integrate the ethwan and adsl portmap

			the fgroup and the entry.itfGroup is always the following sequence

			bit 0 ===> frist lan device, e.g. eth0.2
			bit n ===> the num n lan or wlan device, e.g. eth0.(2 + n);
			*/
			unsigned int pass_kernel_fgroup = pmap_list[i].fgroup << 1;

			vcIdx = VC_INDEX(pmap_list[i].ifIndex);
			snprintf(wanif, 16, "vc%d", vcIdx);
			//AUG_PRT("The tmp_Entry.fgroup is 0x%x\n", pass_kernel_fgroup);
			snprintf(msg.data, BUF_SIZE, "mpoactl set %s fgroup %d", wanif, pass_kernel_fgroup);
			//AUG_PRT("%s\n", msg.data);

			write_to_mpoad(&msg);

		}
		else
			continue;
	}
	return 0;
}

void setup_vc_pmap_lanmember(int vcIndex, unsigned int fgroup)
{
	char wanif[16];
	int vcIdx;
	struct data_to_pass_st msg;

	vcIdx = VC_INDEX(vcIndex);
	snprintf(wanif, 16, "vc%d", vcIdx);

	snprintf(msg.data, BUF_SIZE, "mpoactl set %s fgroup %d", wanif, fgroup);

	//AUG_PRT("%s \n", msg.data);

	write_to_mpoad(&msg);
}

void setup_wan_pmap_lanmember(MEDIA_TYPE_T mType, unsigned int Index)
{
	int i;

	get_pmap_fgroup(pmap_list, MAX_VC_NUM);

	for(i=0; i<MAX_VC_NUM; i++)
	{
		if(pmap_list[i].ifIndex != Index)
			continue;

		if (mType == MEDIA_ETH && (WAN_MODE & MODE_Ethernet))
		{
			int tmp_group;
#ifdef CONFIG_HWNAT
			// bit-0 used for wan in driver
			tmp_group = ((pmap_list[i].fgroup)<<1)|0x1;
			//AUG_PRT("set intf:%x   member:%x!\n", Index, tmp_group);
			setup_hwnat_eth_member(Index, tmp_group, 1);
#endif
		}
#ifdef CONFIG_PTMWAN
		else if (mType == MEDIA_PTM && (WAN_MODE & MODE_PTM))
		{
			int tmp_group;
#ifdef CONFIG_HWNAT
			// bit-0 used for wan in driver
			tmp_group = ((pmap_list[i].fgroup)<<1)|0x1;
			//AUG_PRT("set intf:%x   member:%x!\n", Index, tmp_group);
			setup_hwnat_ptm_member(Index, tmp_group, 1);
#endif
		}
#endif /*CONFIG_PTMWAN*/
		else if (mType == MEDIA_ATM && (WAN_MODE & MODE_ATM))
		{
			unsigned int fgroup;
			// bit-0 is set to 0 to diff vc from nas
			fgroup = ((pmap_list[i].fgroup)<<1)|0x0;
			//AUG_PRT("set intf:%x   member:%x!\n", Index, fgroup);
			setup_vc_pmap_lanmember(Index, fgroup);
		}

	}
}

static void reset_pmap()
{
	int32_t totalEntry;
	MIB_CE_ATM_VC_T temp_Entry;
	int32_t i;
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
	MIB_CE_PORT_BINDING_T portBindEntry;
#endif
	setTmpBlockBridgeFloodPktRule(1);

	// ebtables -D INPUT -j portmapping_input
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_INPUT, "-j", (char *)PORTMAP_INTBL);
	// ebtables -F portmapping_input
	va_cmd(EBTABLES, 2, 1, "-F", (char *)PORTMAP_INTBL);
	// ebtables -X portmapping_input
	va_cmd(EBTABLES, 2, 1, "-X", (char *)PORTMAP_INTBL);
	
	// ebtables -D OUTPUT -j portmapping_output
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_OUTPUT, "-j", (char *)PORTMAP_OUTTBL);
	// ebtables -F portmapping_input
	va_cmd(EBTABLES, 2, 1, "-F", (char *)PORTMAP_OUTTBL);
	// ebtables -X portmapping_input
	va_cmd(EBTABLES, 2, 1, "-X", (char *)PORTMAP_OUTTBL);
	
	// ebtables -F portmapping_service
	va_cmd(EBTABLES, 2, 1, "-F", (char *)PORTMAP_SEVTBL);
	// ebtables -X portmapping_service
	va_cmd(EBTABLES, 2, 1, "-X", (char *)PORTMAP_SEVTBL);
#ifdef CONFIG_E8B
	// ebtables -F portmapping_service2
	va_cmd(EBTABLES, 2, 1, "-F", (char *)PORTMAP_SEVTBL2);
	// ebtables -X portmapping_service2
	va_cmd(EBTABLES, 2, 1, "-X", (char *)PORTMAP_SEVTBL2);
#ifdef CONFIG_CU
	// ebtables -F portmapping_service_rt
	va_cmd(EBTABLES, 2, 1, "-F", (char *)PORTMAP_SEVTBL_RT);
	// ebtables -X portmapping_service_rt
	va_cmd(EBTABLES, 2, 1, "-X", (char *)PORTMAP_SEVTBL_RT);
	// ebtables -F portmapping_service_rt2
	va_cmd(EBTABLES, 2, 1, "-F", (char *)PORTMAP_SEVTBL_RT2);
	// ebtables -X portmapping_service_rt2
	va_cmd(EBTABLES, 2, 1, "-X", (char *)PORTMAP_SEVTBL_RT2);
#endif
#endif
	// ebtables -D FORWARD -j portmapping
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)PORTMAP_EBTBL);
	// ebtables -F portmapping
	va_cmd(EBTABLES, 2, 1, "-F", (char *)PORTMAP_EBTBL);
	// ebtables -X portmapping
	va_cmd(EBTABLES, 2, 1, "-X", (char *)PORTMAP_EBTBL);
	
	// ebtables -t broute -D BROUTING -j portmapping_broute
	va_cmd(EBTABLES, 6, 1, "-t", "broute", (char *)FW_DEL, "BROUTING", "-j", (char *)PORTMAP_BRTBL);
	// ebtables -t broute -F portmapping_broute
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", (char *)PORTMAP_BRTBL);
	// ebtables -t broute -X portmapping_broute
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X", (char *)PORTMAP_BRTBL);
	
	// iptables -t mangle -D PREROUTING -j portmapping_rt_tbl
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", (char *)FW_PREROUTING, "-j", PORTMAP_RTTBL);
	// iptables -t mangle -F portmapping_rt_tbl
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", PORTMAP_RTTBL);
	// iptables -t mangle -X portmapping_rt_tbl
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", PORTMAP_RTTBL);

#ifdef CONFIG_VIR_BOA_JEMBENCHTEST
	rtk_cmcc_do_jspeedup_ex();
#endif

#ifdef CONFIG_IPV6
	// ip6tables -t mangle -D PREROUTING -j portmapping_rt_tbl
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", (char *)FW_PREROUTING, "-j", PORTMAP_RTTBL);
	// ip6tables -t mangle -F portmapping_rt_tbl
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", PORTMAP_RTTBL);
	// ip6tables -t mangle -X portmapping_rt_tbl
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", PORTMAP_RTTBL);	
	// ebtables -F ipv6_ra_local_out
	va_cmd(EBTABLES, 2, 1, "-F", (char *)IPV6_RA_LO_FILTER);
	// ebtables -D OUTPUT -j ipv6_ra_local_out
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_OUTPUT, "-j", (char *)IPV6_RA_LO_FILTER);
	// ebtables -X ipv6_ra_local_out
	va_cmd(EBTABLES, 2, 1, "-X", (char *)IPV6_RA_LO_FILTER);
#endif

#if defined(CONFIG_USER_NEW_BR_FOR_BRWAN)
	reset_untag_intf_to_br0();	
#endif
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
	totalEntry = mib_chain_total(MIB_PORT_BINDING_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_PORT_BINDING_TBL, i, (void*)&portBindEntry))
		{
			setup_vlan_map_intf(&portBindEntry, portBindEntry.port, 0);
		} 
	}
#endif

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&temp_Entry))
		{
			set_pmap_rule(&temp_Entry, 0);
		} 
	}
}

#ifdef CONFIG_CMCC
/* 
For CMCC test case 8.1.1.
Create two bridge internet WAN with vlan 200 and 300
LAN1 vlan binding with 200/200 
LAN2 vlan binding with 300/300
SSID1 vlan binding with 300/300
LAN1 <--> LAN2 Drop
LAN2 <--> SSID1 Forward
In this configuration, the pvid of LAN1/LAN2/SSID1 should be LAN VLAN(4200). So three port could ping each other.
 */
const char VLANBINDING_ISOLATION[] = "vlanisolation";
int rtk_cmcc_find_wan_by_vlanid(unsigned short vid, MIB_CE_ATM_VC_T *vc_Entry)
{
	int j = 0, totalEntry = 0, ifIndex = 0;

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for (j = 0; j < totalEntry; ++j)
	{
		mib_chain_get(MIB_ATM_VC_TBL, j, (void*)vc_Entry);
		if (vc_Entry->vlan && vc_Entry->vid == vid)
		{
			ifIndex	= (int)vc_Entry->ifIndex;

			return ifIndex;
		}
	}
	return -1;
}

static int rtk_cmcc_get_vlan_binding_port_name(unsigned int port, char* portname, int len)
{
	int32_t phyPortId=-1, wanPhyPortId=-1;
#ifdef WLAN_SUPPORT
	int ssidIdx = -1;
#endif

	//printf("%s:%d, port = %d\n", __FUNCTION__, __LINE__, port);
	if(port < PMAP_WLAN0 && port < ELANVIF_NUM)
	{
		phyPortId = rtk_port_get_lan_phyID(port);
		wanPhyPortId = rtk_port_get_wan_phyID();
		if (phyPortId != -1 && phyPortId == wanPhyPortId){
			return -1;
		}
		else{
			snprintf(portname, len, "%s+", ELANRIF[port]);
			//printf("%s:%d, port = %d, portname %s\n", __FUNCTION__, __LINE__, port, portname);
		}
	}
#ifdef WLAN_SUPPORT
	else if((ssidIdx = (port-PMAP_WLAN0)) <= PMAP_WLAN1_VAP_END && ssidIdx >= 0){
		snprintf(portname, len, "%s", wlan[ssidIdx]);
		//printf("%s:%d, port = %d, portname %s\n", __FUNCTION__, __LINE__, port, portname);
	}
#endif
	else 
		return -1;

	return 0;
}

static void rtk_cmcc_clear_lan_vlanbind_issolation(void)
{
	//ebtables -D FORWARD -j lanissolation
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)VLANBINDING_ISOLATION);
	// ebtables -F lanissolation
	va_cmd(EBTABLES, 2, 1, "-F", (char *)VLANBINDING_ISOLATION);
	// ebtables -X lanissolation
	va_cmd(EBTABLES, 2, 1, "-X", (char *)VLANBINDING_ISOLATION);
}

int rtk_cmcc_handle_lan_vlanbind_issolation(void)
{
	int ret=0;
	MIB_CE_PORT_BINDING_T pairEntry1, pairEntry2;
	int totalPortbd;
	int pair1, pair2;
	MIB_CE_ATM_VC_T vc_Entry;
	char portname1[20], portname2[20];
	int i, total = 0;
	unsigned int tmpItfGroup=0;
	MIB_CE_ATM_VC_T entry;
	MIB_CE_PORT_BINDING_T portBindEntry;
	char portname[20];
	
	memset(portname1, 0, sizeof(portname1));
	memset(portname2, 0, sizeof(portname2));
	rtk_cmcc_clear_lan_vlanbind_issolation();
	//ebtables -N lanissolation
	va_cmd(EBTABLES, 2, 1, "-N", (char *)VLANBINDING_ISOLATION);
	//ebtables -P lanissolation RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)VLANBINDING_ISOLATION, "RETURN");
	//ebtables -A FORWARD -j lanissolation
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)VLANBINDING_ISOLATION);

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
			return -1;
		if (entry.enable == 0)
			continue;
		tmpItfGroup |= entry.itfGroup;
	}
	
	totalPortbd = mib_chain_total(MIB_PORT_BINDING_TBL);
	for (pair1 = 0; pair1 < totalPortbd-1; ++pair1)
	{

		mib_chain_get(MIB_PORT_BINDING_TBL, pair1, (void*)&pairEntry1);
		if((unsigned char)VLAN_BASED_MODE == pairEntry1.pb_mode)
		{
			/* Only support ONE vlan binding set for each port */
			struct vlan_pair *vid_pair1;
			vid_pair1 = (struct vlan_pair *)&pairEntry1.pb_vlan0_a;
			rtk_cmcc_get_vlan_binding_port_name(pairEntry1.port, portname1, sizeof(portname1));
			
			//ebtables -A lanissolation -i eth0.x+ -o eth0.x+ -j DROP //example: eth0.6.0 send to eth0.6.102
			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char *)VLANBINDING_ISOLATION, "-i", portname1, "-o", portname1,"-j", (char *)FW_DROP); //eth0.6.0 send to eth0.6.102

			for(pair2=pair1+1; pair2<totalPortbd;++pair2)
			{
				mib_chain_get(MIB_PORT_BINDING_TBL, pair2, (void*)&pairEntry2);

				if((unsigned char)VLAN_BASED_MODE == pairEntry2.pb_mode)
				{
					struct vlan_pair *vid_pair2;
					vid_pair2 = (struct vlan_pair *)&pairEntry2.pb_vlan0_a;
					rtk_cmcc_get_vlan_binding_port_name(pairEntry2.port, portname2, sizeof(portname2));
					if(vid_pair1->vid_a!=vid_pair2->vid_a && (rtk_cmcc_find_wan_by_vlanid(vid_pair1->vid_b, &vc_Entry) > 0) && (rtk_cmcc_find_wan_by_vlanid(vid_pair2->vid_b, &vc_Entry) > 0))
					{
						//ebtables -A lanissolation -i eth0.x+ -o eth0.y+ -j DROP
						//ebtables -A lanissolation -i eth0.y+ -o eth0.x+ -j DROP
						if(portname1[0] && portname2[0])
						{
							va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char *)VLANBINDING_ISOLATION, "-i", portname1, "-o", portname2,"-j", (char *)FW_DROP);
							va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char *)VLANBINDING_ISOLATION, "-i", portname2, "-o", portname1,"-j", (char *)FW_DROP);
						}
						
					}
				}
			}	
			//drop unbind port traffic to vlan binding port
			for(i = 0; i < totalPortbd-1; ++i)
			{
				memset(portname, 0, sizeof(portname));
				if(!BIT_IS_SET(tmpItfGroup, i)){
					mib_chain_get(MIB_PORT_BINDING_TBL, i, (void*)&portBindEntry);
					if(portBindEntry.pb_mode != (unsigned char)VLAN_BASED_MODE){
						rtk_cmcc_get_vlan_binding_port_name(i, portname, sizeof(portname));
						if(portname1[0] && portname[0]){
							va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char *)VLANBINDING_ISOLATION, "-i", portname, "-o", portname1,"-j", (char *)FW_DROP);
						}
					}
				}
			}
		}
	}
	return 0;
}

int isInternetRouteWanExist(void)
{
	int total, i, found = 0;
	MIB_CE_ATM_VC_T entry;

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_ATM_VC_TBL, i, &entry) == 0)
			continue;

		if (entry.enable == 0)
			continue;

		if ((entry.cmode != CHANNEL_MODE_BRIDGE) && (entry.applicationtype & X_CT_SRV_INTERNET))
		{
			found = 1;
			break;
		}
	}
	return found;
}

/*
For CMCC
When both Internet route WAN and bridge WAN exist.
those unbind LAN ports should use route WAN.
*/
const char UNBIND_PORT_RULE[] = "unbind_port_rule";

void rtk_cmcc_clear_unbind_port_rule(void)
{
	//ebtables -D FORWARD -j unbind_port_rule
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)UNBIND_PORT_RULE);
	// ebtables -F unbind_port_rule
	va_cmd(EBTABLES, 2, 1, "-F", (char *)UNBIND_PORT_RULE);
	// ebtables -X unbind_port_rule
	va_cmd(EBTABLES, 2, 1, "-X", (char *)UNBIND_PORT_RULE);
}
int rtk_cmcc_handle_unbind_port_rule(void)
{
	int i, total = 0;
	unsigned int tmpItfGroup=0;
	MIB_CE_ATM_VC_T entry;
	MIB_CE_PORT_BINDING_T portBindEntry;
	char portname[20];

	rtk_cmcc_clear_unbind_port_rule();
	if(!isInternetRouteWanExist())
	{
		return -1;
	}

	//ebtables -N unbind_port_rule
	va_cmd(EBTABLES, 2, 1, "-N", (char *)UNBIND_PORT_RULE);
	//ebtables -P unbind_port_rule RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)UNBIND_PORT_RULE, "RETURN");
	//ebtables -A FORWARD -j unbind_port_rule
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)UNBIND_PORT_RULE);

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
			return -1;
		if (entry.enable == 0)
			continue;
		tmpItfGroup |= entry.itfGroup;
	}
	
	for(i = PMAP_ETH0_SW0; i <= PMAP_WLAN1_VAP_END; ++i)
	{
		memset(portname, 0, sizeof(portname));
		if(!BIT_IS_SET(tmpItfGroup, i)){
			mib_chain_get(MIB_PORT_BINDING_TBL, i, (void*)&portBindEntry);
			if(portBindEntry.pb_mode != (unsigned char)VLAN_BASED_MODE){
				rtk_cmcc_get_vlan_binding_port_name(i, portname, sizeof(portname));
				if(portname[0]){
					va_cmd(EBTABLES, 14, 1, "-A", UNBIND_PORT_RULE, "-i", portname, "-o", "nas0+", "-p", "ipv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "DROP");
					va_cmd(EBTABLES, 10, 1, "-A", UNBIND_PORT_RULE, "-i", portname, "-o", "nas0+", "-p", "PPP_DISC", "-j", "DROP");
					va_cmd(EBTABLES, 10, 1, "-A", UNBIND_PORT_RULE, "-i", portname, "-o", "nas0+", "-p", "PPP_SES", "-j", "DROP");
				}
			}
		}
	}

	return 0;
}
#endif

#if defined(CONFIG_USER_PORT_ISOLATION)
const char LAN_PORT_ISOLATION[] = "lan_port_isolation";
void rtk_lan_clear_lan_port_isolation(void)
{
	//ebtables -D FORWARD -j lanissolation
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)LAN_PORT_ISOLATION);
	// ebtables -F lanissolation
	va_cmd(EBTABLES, 2, 1, "-F", (char *)LAN_PORT_ISOLATION);
	// ebtables -X lanissolation
	va_cmd(EBTABLES, 2, 1, "-X", (char *)LAN_PORT_ISOLATION);
}

int rtk_lan_handle_lan_port_isolation(void)
{
	int ret=0;
	MIB_CE_PORT_ISOLATION_T pairEntry;
	int totalPortbd, pair1;
	char portname[20];

	//ebtables -N lanissolation
	va_cmd(EBTABLES, 2, 1, "-N", (char *)LAN_PORT_ISOLATION);
	//ebtables -P lanissolation RETURN
	va_cmd(EBTABLES, 3, 1, "-P", (char *)LAN_PORT_ISOLATION, "RETURN");
	//ebtables -A FORWARD -j lanissolation
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)LAN_PORT_ISOLATION);

	totalPortbd = mib_chain_total(MIB_PORT_ISOLATION_TBL);
	for (pair1 = 0; pair1 < totalPortbd; ++pair1)
	{
		mib_chain_get(MIB_PORT_ISOLATION_TBL, pair1, (void*)&pairEntry);
		if(pairEntry.isolation==1 && pairEntry.port < PMAP_ITF_END){
			if(pairEntry.port < PMAP_WLAN0)
			{
				snprintf(portname, sizeof(portname), "%s+", ELANRIF[pairEntry.port]);
			}
#ifdef WLAN_SUPPORT
			else if(pairEntry.port >= PMAP_WLAN0 && pairEntry.port <= PMAP_WLAN1_VAP_END)
			{
				snprintf(portname, sizeof(portname), "%s+", wlan[pairEntry.port-PMAP_WLAN0]);
			}
#endif //WLAN_SUPPORT
			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char *)LAN_PORT_ISOLATION, "-i", portname, "-o", "eth+","-j", (char *)FW_DROP);
			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char *)LAN_PORT_ISOLATION, "-i", portname, "-o", "wlan+","-j", (char *)FW_DROP);
		}
	}
	return 0;
}

#endif
#ifdef CONFIG_RTK_MAC_AUTOBINDING
extern char *if_indextoname (unsigned int __ifindex, char *__ifname);
//mode 0--del 1--add
extern char *if_indextoname (unsigned int __ifindex, char *__ifname);
void set_auto_mac_binding_map_rule(unsigned char *smac,unsigned itfIndex,int mode){
	char ifname[IFNAMSIZ+1]={0};
	int wanIfindex = 0;
	char chain_in[64]={0};
	char chain_out[64]={0};
	char smac_str[32]={0};
	if(!if_indextoname(itfIndex,ifname)){
		printf("%s %d get interface name error from index %d\n",__FUNCTION__,__LINE__,itfIndex);
		return;
	}
	wanIfindex=getIfIndexByName(ifname);
	if(wanIfindex==DUMMY_IFINDEX){
		printf("%s %d get ifindex error from ifname %s\n",__FUNCTION__,__LINE__,ifname);
		return;
	}
	sprintf(chain_in, "%s_%s_in", PORTMAP_EBTBL, ifname);
	sprintf(chain_out, "%s_%s_out", PORTMAP_EBTBL, ifname);
	va_cmd(EBTABLES, 2, 1, "-N", chain_in);
	va_cmd(EBTABLES, 3, 1, "-P", chain_in, "DROP");
	va_cmd(EBTABLES, 2, 1, "-N", chain_out);
	va_cmd(EBTABLES, 3, 1, "-P", chain_out, "DROP");
	sprintf(smac_str,"%02x:%02x:%02x:%02x:%02x:%02x",smac[0],smac[1],smac[2],smac[3],smac[4],smac[5]);
	va_cmd(EBTABLES, 6, 1, mode?"-I":"-D", PORTMAP_EBTBL, "-d", smac_str, "-j", chain_in);
	//va_cmd(EBTABLES, 6, 1, "-I", PORTMAP_INTBL, "-s", smac_str, "-j", PORTMAP_SEVTBL);
	va_cmd(EBTABLES, 6, 1, mode?"-I":"-D", "INPUT", "-s", smac_str, "-j", PORTMAP_SEVTBL);
	va_cmd(EBTABLES, 6, 1, mode?"-I":"-D", PORTMAP_EBTBL, "-s", smac_str, "-j", chain_out);
	va_cmd(EBTABLES, 6, 1, mode?"-I":"-D", PORTMAP_OUTTBL, "-d", smac_str, "-j", PORTMAP_SEVTBL);
	va_cmd(EBTABLES, 6, 1, mode?"-I":"-D", PORTMAP_OUTTBL, "-d", smac_str, "-j", chain_out);
	va_cmd(EBTABLES, 6, 1, mode?"-I":"-D", chain_in, "-d", smac_str, "-j", "RETURN");
	va_cmd(EBTABLES, 6, 1, mode?"-I":"-D", chain_out, "-s",smac_str, "-j", "RETURN");

	va_cmd(EBTABLES, 14, 1, mode?"-I":"-D", UNBIND_PORT_RULE, "-s", smac_str, "-o", "nas0+", "-p", "ipv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "ACCEPT");
	va_cmd(EBTABLES, 10, 1, mode?"-I":"-D", UNBIND_PORT_RULE, "-s", smac_str, "-o", "nas0+", "-p", "PPP_DISC", "-j", "ACCEPT");
	va_cmd(EBTABLES, 10, 1, mode?"-I":"-D", UNBIND_PORT_RULE, "-s", smac_str, "-o", "nas0+", "-p", "PPP_SES", "-j", "ACCEPT");
}
#endif

void setupnewEth2pvc()
{
	reset_pmap();	
	//RESET_DECONFIG_SCRIPT();	

#ifdef CONFIG_CMCC
	rtk_cmcc_handle_lan_vlanbind_issolation();
#endif

	//set_vc_fgroup();
	exec_portmp();
#if defined(CONFIG_USER_VXLAN)
	exec_vxlan_portmp();
#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD)
	rtk_wan_vpn_attach_policy_route_init();
#endif
#ifdef CONFIG_CMCC
	rtk_cmcc_handle_unbind_port_rule();
#endif
#if defined(CONFIG_USER_PORT_ISOLATION)
	rtk_lan_clear_lan_port_isolation();
	rtk_lan_handle_lan_port_isolation();
#endif
	//CLOSE_DECONFIG_SCRIPT();
}

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_NET_IPIP) || defined(CONFIG_USER_XL2TPD) || defined(CONFIG_USER_PPTP_CLIENT)
#include <linux/sockios.h>
#ifdef CONFIG_IPV6_VPN
#include <ifaddrs.h>
#endif

unsigned int get_ip(char * szIf)
{
	int sockfd;
	struct ifreq req;
	struct sockaddr_in *host;
	int ret=0;

	if(-1 == (sockfd = socket(PF_INET, SOCK_STREAM, 0)))
	{
		perror( "socket" );
		return -1;
	}

	bzero(&req, sizeof(struct ifreq));
	req.ifr_name[sizeof(req.ifr_name)-1]='\0';
	strncpy(req.ifr_name, szIf,sizeof(req.ifr_name)-1);
	ret=ioctl(sockfd, SIOCGIFADDR, &req);
	if(ret)
		printf("ioctl error!\n");
	host = (struct sockaddr_in*)&req.ifr_addr;
	close(sockfd);

	return host->sin_addr.s_addr;
}

#ifdef CONFIG_IPV6_VPN
static int ipv6_addr_equal(const struct in6_addr *a1,
				  const struct in6_addr *a2)
{
	return (((a1->s6_addr32[0] ^ a2->s6_addr32[0]) |
		 (a1->s6_addr32[1] ^ a2->s6_addr32[1]) |
		 (a1->s6_addr32[2] ^ a2->s6_addr32[2]) |
		 (a1->s6_addr32[3] ^ a2->s6_addr32[3])) == 0);
}

/**
 * If head file  is not existed in your system, you could get information of IPv6 address
 * in file /proc/net/if_inet6.
 *
 * Contents of file "/proc/net/if_inet6" like as below:
 * 00000000000000000000000000000001 01 80 10 80       lo
 * fe8000000000000032469afffe08aa0f 08 40 20 80     ath0
 * fe8000000000000032469afffe08aa0f 07 40 20 80    wifi0
 * fe8000000000000032469afffe08aa0f 05 40 20 80     eth1
 * fe8000000000000032469afffe08aa0f 03 40 20 80      br0
 * fe8000000000000032469afffe08aa10 04 40 20 80     eth0
 *
 * +------------------------------+ ++ ++ ++ ++    +---+
 * |                                |  |  |  |     |
 * 1                                2  3  4  5     6
 *
 * There are 6 row items parameters:
 * 1 => IPv6 address without ':'
 * 2 => Interface index
 * 3 => Length of prefix
 * 4 => Scope value (see kernel source "include/net/ipv6.h" and "net/ipv6/addrconf.c")
 * 5 => Interface flags (see kernel source "include/linux/rtnetlink.h" and "net/ipv6/addrconf.c")
 * 6 => Device name
 *
 * Note that all values of row 1~5 are hexadecimal string
 */
static int isIfIn6Addr(char * szIf, struct in6_addr *ifaddr6)
{
#define IF_INET6 "/proc/net/if_inet6"
	struct in6_addr addr6;
	char str[128], address[64];
	char *addr, *index, *prefix, *scope, *flags, *name;
	char *delim = " \t\n", *p, *q;
	FILE *fp;
	int count;

	if (NULL == (fp = fopen(IF_INET6, "r"))) {
		perror("fopen error");
		return -1;
	}

	while (fgets(str, sizeof(str), fp)) {
		addr = strtok(str, delim);
		index = strtok(NULL, delim);
		prefix = strtok(NULL, delim);
		scope = strtok(NULL, delim);
		flags = strtok(NULL, delim);
		name = strtok(NULL, delim);

		if (strcmp(name, szIf))
			continue;

		memset(address, 0x00, sizeof(address));
		p = addr;
		q = address;
		count = 0;
		while (*p != ' ') {
				if (count == 4) {
						*q++ = ':';
						count = 0;
				}
				*q++ = *p++;
				count++;
		}

		inet_pton(AF_INET6, address, &addr6);

		if (ipv6_addr_equal(&addr6, ifaddr6)) {
			fclose(fp);
			return 1;
		}
	}

	fclose(fp);
	return 0;
}
#endif

/*
 * Author          : QL
 * Date             : 2011-12-16
 * FUNC_NAME : modPolicyRouteTable
 * PARAMS        : pptp_ifname                pptp interface name(ppp9, ppp10, ...)
 *                        real_addr                    real interface address(vc0, nas0_0, ppp0, ...)
 * Description   : when PPtP Interface is on with def gw, maybe we should add default route to policy route table
 */
void modPolicyRouteTable(const char *pptp_ifname, struct in_addr *real_addr)
{
	MIB_CE_ATM_VC_T entry;
	int32_t totalEntry;
	uint32_t ifIndex, tbl_id, i;
	char ifname[IFNAMSIZ];
	char str_tblid[10];
	unsigned int devAddr;

	printf("%s %d Enter (pptp:%s real:%x).\n", __func__, __LINE__, pptp_ifname, real_addr->s_addr);

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&entry))
		{
#ifdef CONFIG_IPV6_VPN
			if (entry.IpProtocol == IPVER_IPV6)
				continue;
#endif

			//only care route mode interface.
			if(entry.cmode != CHANNEL_MODE_BRIDGE)
			{
				ifIndex = entry.ifIndex;
				ifGetName(ifIndex, ifname, sizeof(ifname));

				devAddr = get_ip(ifname);
				if (devAddr != real_addr->s_addr)
					continue;

				tbl_id = caculate_tblid(ifIndex);
				snprintf(str_tblid, 10, "%d", tbl_id);
				//delete original default route.
				printf("ip route del default table %d\n", tbl_id);
				va_cmd(BIN_IP, 5, 1, "route", "del", "default", "table", str_tblid);

				printf("ip route add default dev %s table %d\n", pptp_ifname, tbl_id);
				va_cmd(BIN_IP, 7, 1, "route", "add", "default", "dev", pptp_ifname, "table", str_tblid);

				break;
			}
		}
	}
}

#ifdef CONFIG_IPV6_VPN
void modIPv6PolicyRouteTable(const char *pptp_ifname, struct in6_addr *real_addr)
{
	MIB_CE_ATM_VC_T entry;
	int32_t totalEntry;
	uint32_t ifIndex, tbl_id, i;
	char ifname[IFNAMSIZ];
	char str_tblid[10];
	struct in6_addr devAddr;
	unsigned char buff[48];

	printf("%s %d Enter (pptp:%s real:%s).\n", __func__, __LINE__, pptp_ifname,
		inet_ntop(AF_INET6, &real_addr, buff, 48));

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&entry))
		{
			if (entry.IpProtocol == IPVER_IPV4)
				continue;

			//only care route mode interface.
			if(entry.cmode != CHANNEL_MODE_BRIDGE)
			{
				ifIndex = entry.ifIndex;
				ifGetName(ifIndex, ifname, sizeof(ifname));

				if (!isIfIn6Addr(ifname, real_addr))
					continue;

				tbl_id = caculate_tblid(ifIndex);
				snprintf(str_tblid, 10, "%d", tbl_id);
				//delete original default route.
				printf("ip -6 route del ::/0 table %d\n", tbl_id);
				va_cmd(BIN_IP, 6, 1, "-6", "route", "del", "::/0", "table", str_tblid);

				printf("ip -6 route add ::/0 dev %s table %d\n", pptp_ifname, tbl_id);
				va_cmd(BIN_IP, 8, 1, "-6", "route", "add", "::/0", "dev", pptp_ifname, "table", str_tblid);

				break;
			}
		}
	}
}
#endif
#endif//endof CONFIG_USER_PPTP_CLIENT_PPTP || CONFIG_USER_L2TPD_L2TPD || CONFIG_NET_IPIP

#endif//endof NEW_PORTMAPPING

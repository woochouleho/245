/*-- System inlcude files --*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "options.h"


void formInterfaceGrouping(request * wp, char *path, char *query)
{
	char *str, *submitUrl;
	int i = 0, total = 0, need_update = 0, need_update_default = 0, itfNum = 0;
	int group_number = 0, group_enable = 0, group_action = 1;
	unsigned int chainid = 0, tmp_chainid = 0;
	unsigned char current_lan_router_groupnum = 0;
	char *member_token = NULL, *member_save = NULL;
	char *interface_token = NULL, *interface_save = NULL;
	char error_msg[128] = {0};
	char group_name[64] = {0}, group_member_list[512] = {0}, group_available_list[512] = {0};
	struct layer2bridging_availableinterface_info itfList[MAX_NUM_OF_ITFS];
	int interface_index = 0, interface_ifdomain = 0, interface_ifid = 0, interface_itfGroup = 0;
	MIB_L2BRIDGE_GROUP_T *p, l2br_group_entry, l2br_group_entry_tmp;
	p = &l2br_group_entry;
	MIB_L2BRIDGE_FILTER_T l2br_filter_entry, l2br_filter_entry_tmp;
	MIB_L2BRIDGE_MARKING_T l2br_marking_entry, l2br_marking_entry_tmp;
	MIB_L2BRIDGE_PORT_T l2br_port_entry;
	MIB_L2BRIDGE_VLAN_T l2br_vlan_entry;
	MIB_CE_ATM_VC_T wanEntry;
	int wanIndex = 0;
	unsigned int show_domain = 0;
	show_domain = (INTFGRPING_DOMAIN_ELAN | INTFGRPING_DOMAIN_WLAN | INTFGRPING_DOMAIN_WANROUTERCONN);
#ifdef CONFIG_USER_LAN_VLAN_TRANSLATE
	show_domain |= (INTFGRPING_DOMAIN_ELAN_VLAN);
#endif
#if !defined(CONFIG_00R0)
	show_domain |= (INTFGRPING_DOMAIN_LANROUTERCONN);
#endif

	fprintf(stderr, "[%s:%s] Set Interface Grouping\n", __FILE__, __FUNCTION__);

	str = boaGetVar(wp, "group_number", "");
	if (str[0])
	{
		group_number = atoi(str);
		fprintf(stderr, "group_number = %d\n", group_number);
	}

	str = boaGetVar(wp, "group_action", "");
	if (str[0])
	{
		group_action = atoi(str);
		fprintf(stderr, "group_action = %d\n", group_action);
	}

	str = boaGetVar(wp, "group_enable_value", "");
	if (str[0])
	{
		group_enable = atoi(str);
		fprintf(stderr, "group_enable = %d\n", group_enable);
	}

	str = boaGetVar(wp, "group_name", "");
	if (str[0])
	{
		snprintf(group_name, sizeof(group_name), "%s", str);
		fprintf(stderr, "group_name = %s\n", group_name);
	}

	str = boaGetVar(wp, "group_member_list", "");
	if (str[0])
	{
		snprintf(group_member_list, sizeof(group_member_list), "%s", str);
		fprintf(stderr, "group_member_list = %s\n", group_member_list);
	}

	str = boaGetVar(wp, "group_available_list", "");
	if (str[0])
	{
		snprintf(group_available_list, sizeof(group_available_list), "%s", str);
		fprintf(stderr, "group_available_list = %s\n", group_available_list);
	}

	if (group_number == (-1))
	{
		total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
		if (total >= IFGROUP_NUM)
		{
			strcpy(error_msg, strTableFull);
			goto CONFIG_ERROR;
		}

		memset(p, 0, sizeof(MIB_L2BRIDGE_GROUP_T));
		p->enable = group_enable;
		p->groupnum = rtk_layer2bridging_get_unused_groupnum();
		if (strlen(group_name) == 0)
		{
			sprintf(p->name, "Group_%d", p->groupnum);
		}
		else
		{
			strcpy(p->name, group_name);
		}
		p->vlanid = 0;
#if (defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)) 
		p->omci_configured = 0;
		p->omci_bridgeId = 0;
#endif
		p->instnum = rtk_layer2bridging_get_new_group_instnum();
		mib_chain_add(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, (void *)p);

		fprintf(stderr, "Add a new group: number = %u, name = %s\n", p->groupnum, p->name);

		group_number = p->groupnum;
		need_update = 1;
	}

	if (rtk_layer2bridging_get_group_by_groupnum(p, group_number, &chainid))
	{
		if (group_action)
		{
			if (p->enable != group_enable)
			{
				p->enable = group_enable;
				need_update = 1;
			}

			if (strlen(group_name) > 0 && strcmp(p->name, group_name))
			{
				strcpy(p->name, group_name);
			}
		}

		itfNum = rtk_layer2bridging_get_availableinterface(itfList, MAX_NUM_OF_ITFS, show_domain, (-1));

		if (strlen(group_member_list))
		{
			member_token = strtok_r(group_member_list, ",", &member_save);
			while (member_token)
			{
				interface_index = 0;
				interface_ifdomain = 0;
				interface_ifid = 0;
				interface_itfGroup = 0;

				interface_token = strtok_r(member_token, "|", &interface_save);
				while (interface_token)
				{
					if (interface_index == 0)
					{
						interface_ifdomain = atoi(interface_token);
					}
					else if (interface_index == 1)
					{
						interface_ifid = atoi(interface_token);
					}
					else if (interface_index == 3)
					{
						interface_itfGroup = atoi(interface_token);
					}

					interface_token = strtok_r(NULL, "|", &interface_save);
					interface_index++;
				}

				for (i = 0; i < itfNum; i++)
				{
					if ((interface_ifdomain == itfList[i].ifdomain) && (interface_ifid == itfList[i].ifid))
					{
						if (interface_itfGroup != p->groupnum)
						{
							fprintf(stderr, "%s change group number: %d -> %d\n", itfList[i].name, interface_itfGroup, p->groupnum);
							rtk_layer2bridging_set_interface_groupnum(interface_ifdomain, interface_ifid, p->groupnum, 1);
							need_update = 1;

							if (interface_ifdomain == INTFGRPING_DOMAIN_LANROUTERCONN)
							{
								current_lan_router_groupnum = p->groupnum;
								mib_set(MIB_RS_LAN_ROUTER_GROUPNUM, (void *)&current_lan_router_groupnum);

								if (p->groupnum == 0)
								{
									need_update_default = 1;
								}
							}
#if defined(CONFIG_00R0)
							else if (interface_ifdomain == INTFGRPING_DOMAIN_WANROUTERCONN &&
										getWanEntrybyIfIndex(interface_ifid, &wanEntry, &wanIndex) == 0 &&
										(wanEntry.dgw || wanEntry.applicationtype & X_CT_SRV_INTERNET))
							{
								int lanIPNum = 0;
								struct layer2bridging_availableinterface_info lanIP[MAX_NUM_OF_ITFS];
								lanIPNum = rtk_layer2bridging_get_availableinterface(lanIP, MAX_NUM_OF_ITFS, INTFGRPING_DOMAIN_LANROUTERCONN, (-1));
								if (lanIPNum > 0)
								{
									if (lanIP[0].itfGroup != p->groupnum)
									{
										fprintf(stderr, "%s change group number: %d -> %d\n", lanIP[0].name, lanIP[0].itfGroup, p->groupnum);
										rtk_layer2bridging_set_interface_groupnum(lanIP[0].ifdomain, lanIP[0].ifid, p->groupnum, 1);
										current_lan_router_groupnum = p->groupnum;
										mib_set(MIB_RS_LAN_ROUTER_GROUPNUM, (void *)&current_lan_router_groupnum);

										if (p->groupnum == 0)
										{
											need_update_default = 1;
										}
									}
								}
							}
#endif

#if defined(_CWMP_MIB_)
							/* remove automatic build bridge WAN if this group has WANROUTERCONN, Filter/Marking entry had been updated by rtk_layer2bridging_set_interface_groupnum */
							if (interface_ifdomain == INTFGRPING_DOMAIN_WANROUTERCONN)
							{
								int j = 0;
								total = mib_chain_total(MIB_ATM_VC_TBL);
								for (j = (total - 1); j >= 0; j--)
								{
									if (!mib_chain_get(MIB_ATM_VC_TBL, j, (void *)&wanEntry))
										continue;

									if (wanEntry.itfGroupNum == p->groupnum)
									{
										if (wanEntry.connDisable == 1 && wanEntry.ConPPPInstNum == 0 && wanEntry.ConIPInstNum == 0)
										{
											deleteConnection(CONFIGONE, &wanEntry);
											mib_chain_delete(MIB_ATM_VC_TBL, j);
										}
									}
								}
							}
#endif
						}

						break;
					}
				}

				member_token = strtok_r(NULL, ",", &member_save);
			}
		}

		if (strlen(group_available_list))
		{
			member_token = strtok_r(group_available_list, ",", &member_save);
			while (member_token)
			{
				interface_index = 0;
				interface_ifdomain = 0;
				interface_ifid = 0;
				interface_itfGroup = 0;

				interface_token = strtok_r(member_token, "|", &interface_save);
				while (interface_token)
				{
					if (interface_index == 0)
					{
						interface_ifdomain = atoi(interface_token);
					}
					else if (interface_index == 1)
					{
						interface_ifid = atoi(interface_token);
					}
					else if (interface_index == 3)
					{
						interface_itfGroup = atoi(interface_token);
					}

					interface_token = strtok_r(NULL, "|", &interface_save);
					interface_index++;
				}

				for (i = 0; i < itfNum; i++)
				{
					if ((interface_ifdomain == itfList[i].ifdomain) && (interface_ifid == itfList[i].ifid))
					{
						if (interface_itfGroup != 0)
						{
							fprintf(stderr, "%s change group number: %d -> 0\n", itfList[i].name, itfList[i].itfGroup);
							rtk_layer2bridging_set_interface_groupnum(interface_ifdomain, interface_ifid, 0, 1);
							need_update_default = 1;

							if (interface_ifdomain == INTFGRPING_DOMAIN_LANROUTERCONN)
							{
								current_lan_router_groupnum = 0;
								mib_set(MIB_RS_LAN_ROUTER_GROUPNUM, (void *)&current_lan_router_groupnum);
								need_update = 1;
							}
#if defined(CONFIG_00R0)
							else if (interface_ifdomain == INTFGRPING_DOMAIN_WANROUTERCONN &&
										getWanEntrybyIfIndex(interface_ifid, &wanEntry, &wanIndex) == 0 &&
										(wanEntry.dgw || wanEntry.applicationtype & X_CT_SRV_INTERNET))
							{
								int lanIPNum = 0;
								struct layer2bridging_availableinterface_info lanIP[MAX_NUM_OF_ITFS];
								lanIPNum = rtk_layer2bridging_get_availableinterface(lanIP, MAX_NUM_OF_ITFS, INTFGRPING_DOMAIN_LANROUTERCONN, (-1));
								if (lanIPNum > 0)
								{
									if (lanIP[0].itfGroup != 0)
									{
										fprintf(stderr, "%s change group number: %d -> 0\n", lanIP[0].name, lanIP[0].itfGroup);
										rtk_layer2bridging_set_interface_groupnum(lanIP[0].ifdomain, lanIP[0].ifid, 0, 1);
										current_lan_router_groupnum = 0;
										mib_set(MIB_RS_LAN_ROUTER_GROUPNUM, (void *)&current_lan_router_groupnum);
										need_update = 1;
									}
								}
							}
#endif

#if defined(_CWMP_MIB_)
							/* remove automatic build bridge WAN if this group has WANROUTERCONN, Filter/Marking entry had been updated by rtk_layer2bridging_set_interface_groupnum */
							if (interface_ifdomain == INTFGRPING_DOMAIN_WANROUTERCONN)
							{
								int j = 0;
								total = mib_chain_total(MIB_ATM_VC_TBL);
								for (j = (total - 1); j >= 0; j--)
								{
									if (!mib_chain_get(MIB_ATM_VC_TBL, j, (void *)&wanEntry))
										continue;

									if (wanEntry.itfGroupNum == 0)
									{
										if (wanEntry.connDisable == 1 && wanEntry.ConPPPInstNum == 0 && wanEntry.ConIPInstNum == 0)
										{
											deleteConnection(CONFIGONE, &wanEntry);
											mib_chain_delete(MIB_ATM_VC_TBL, j);
										}
									}
								}
							}
#endif
						}

						break;
					}
				}

				member_token = strtok_r(NULL, ",", &member_save);
			}
		}

		if (group_action == 0)
		{
			fprintf(stderr, "Remove group %u\n", group_number);

			total = mib_chain_total(MIB_L2BRIDGING_FILTER_TBL);
			for (i = (total - 1); i >= 0; i--)
			{
				if (!mib_chain_get(MIB_L2BRIDGING_FILTER_TBL, i, (void *)&l2br_filter_entry))
					continue;

				if (l2br_filter_entry.groupnum == group_number)
				{
					if (rtk_layer2bridging_get_interface_filter(l2br_filter_entry.ifdomain, l2br_filter_entry.ifid, 0, &l2br_filter_entry_tmp, &tmp_chainid) == 0)
					{
						l2br_filter_entry.groupnum = 0;
						mib_chain_update(MIB_L2BRIDGING_FILTER_TBL, (void *)&l2br_filter_entry, i);
					}
					else
					{
						mib_chain_delete(MIB_L2BRIDGING_FILTER_TBL, i);
					}
				}
			}

			total = mib_chain_total(MIB_L2BRIDGING_MARKING_TBL);
			for (i = (total - 1); i >= 0; i--)
			{
				if (!mib_chain_get(MIB_L2BRIDGING_MARKING_TBL, i, (void *)&l2br_marking_entry))
					continue;

				if (l2br_marking_entry.groupnum == group_number)
				{
					if (rtk_layer2bridging_get_interface_marking(l2br_marking_entry.ifdomain, l2br_marking_entry.ifid, 0, &l2br_marking_entry_tmp, &tmp_chainid) == 0)
					{
						l2br_marking_entry.groupnum = 0;
						mib_chain_update(MIB_L2BRIDGING_MARKING_TBL, (void *)&l2br_marking_entry, i);
					}
					else
					{
						mib_chain_delete(MIB_L2BRIDGING_MARKING_TBL, i);
					}
				}
			}

			total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_PORT_TBL);
			for (i = (total - 1); i >= 0; i--)
			{
				if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_PORT_TBL, i, (void *)&l2br_port_entry))
					continue;

				if (l2br_port_entry.groupnum == group_number)
				{
					mib_chain_delete(MIB_L2BRIDGING_BRIDGE_PORT_TBL, i);
				}
			}

			total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_VLAN_TBL);
			for (i = (total - 1); i >= 0; i--)
			{
				if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_VLAN_TBL, i, (void *)&l2br_vlan_entry))
					continue;

				if (l2br_vlan_entry.groupnum == group_number)
				{
					mib_chain_delete(MIB_L2BRIDGING_BRIDGE_VLAN_TBL, i);
				}
			}

#if defined(_CWMP_MIB_)
			total = mib_chain_total(MIB_ATM_VC_TBL);
			for (i = (total - 1); i >= 0; i--)
			{
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&wanEntry))
					continue;

				if (wanEntry.itfGroupNum == group_number)
				{
					if (wanEntry.connDisable == 1 && wanEntry.ConPPPInstNum == 0 && wanEntry.ConIPInstNum == 0)
					{
						deleteConnection(CONFIGONE, &wanEntry);
						mib_chain_delete(MIB_ATM_VC_TBL, i);
					}
				}
			}
#endif

			setup_interface_grouping(p, 0);

			if (need_update_default)
			{
				if (rtk_layer2bridging_get_group_by_groupnum(&l2br_group_entry_tmp, 0, &tmp_chainid))
				{
					setup_interface_grouping(&l2br_group_entry_tmp, 1);
				}
			}

			mib_chain_delete(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, chainid);
		}
		else
		{
			fprintf(stderr, "Update group %u\n", group_number);
			mib_chain_update(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, (void *)p, chainid);

			if (need_update)
			{
				setup_interface_grouping(p, 1);
			}

			if (need_update_default)
			{
				if (rtk_layer2bridging_get_group_by_groupnum(&l2br_group_entry_tmp, 0, &tmp_chainid))
				{
					setup_interface_grouping(&l2br_group_entry_tmp, 1);
				}
			}
		}
	}
	else
	{
		strcpy(error_msg, strGetChainerror);
		goto CONFIG_ERROR;
	}

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#if defined(CONFIG_00R0)
	cleanAllFirewallRule();
	setupFirewall(NOT_BOOT);
#endif
	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG(submitUrl);
	return;

CONFIG_ERROR:
	ERR_MSG(error_msg);
}



/*
 *      laod balance
 *
 */

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
#include "subr_net.h"

#define MAX_PORT_NUM	11
#define MAX_TOTAL_LOGIC_PORT_NUM 6

#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
void formLdStats(request * wp, char *path, char *query)
{
	char *strValue, *submitUrl, ret=0;
	
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page

	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
}

int rtk_set_default_route_by_wan_info(MIB_CE_ATM_VC_T *wan_entry)
{
	char wan_gw_ip[16] = {0};
	char wan_name[IFNAMSIZ] = {0};
	int balance_type;
	
	if(wan_entry ==NULL)
		return -1;

	ifGetName(wan_entry->ifIndex, wan_name, sizeof(wan_name));

	if(wan_entry->balance_type == 1)
	{
		//shared network need check default route rule which is necessary or not.
		if(isDefaultRouteWan(wan_entry))
		{
			rtk_get_default_gw_ip_by_wan_info(wan_name, wan_entry->cmode, wan_gw_ip);
			if((wan_entry->cmode==CHANNEL_MODE_IPOE) && (strlen(wan_gw_ip) > 0))
			{
				va_cmd(ROUTE, 6, 1, ARG_ADD, "default", "gw", wan_gw_ip, "dev", wan_name);
			}
			else if(wan_entry->cmode==CHANNEL_MODE_PPPOE)
			{
				va_cmd(ROUTE, 3, 1, ARG_ADD, "default", wan_name);
			}
			else{
				//do nothing
			}
		}
	}
	else
	{
		//proprietary network should delete default route rule.
		va_cmd(ROUTE, 3, 1, ARG_DEL, "default", wan_name);
	}

	return 0;
}

void formDynmaicBalance(request * wp, char *path, char *query)
{
	char *addDynbalance, *strWanDevName, *dynSelectValue, *globalEnableFlag, *dynTypeValue, *submitUrl, *strVal;
	int i =0, total_wan_num = 0,j = 0 , dyn_select_value = 0, dyn_type_value = 0 , dy_enable_flag = 0, dyn_type_old = 0;
	int dy_weight_flag = 0, dy_weight_old = 0;
	MIB_CE_ATM_VC_T wan_entry;
	char wanif[IFNAMSIZ] = {0};
	char tmpBuf[100];
	char vChar = 0, vChar_type = 0,globalFlag = 0;
	char tmpbuf1[32]={0},tmpbuf2[32]={0},tmpbuf3[32]={0};
	char global_balance_enable = 0;
	int global_balance_flag = 0;
	char dy_enable = 0;
	char cmd[128] = {0};

	globalEnableFlag = boaGetVar(wp, "total_balance_enable", "");
	if (globalEnableFlag[0]) {
		if (globalEnableFlag[0] == '0')
			globalFlag = 0;
		else if(globalEnableFlag[0] == '1')
			globalFlag = 1;
	}
	if(!mib_get_s(MIB_GLOBAL_LOAD_BALANCE, (void *)&global_balance_enable,sizeof(global_balance_enable)))
	{
		snprintf(tmpBuf, sizeof(tmpBuf), "%s:%s", Tget_mib_error, MIB_GLOBAL_LOAD_BALANCE);
		goto setErr_dyn_balance;
	}
		
	if(!mib_set(MIB_GLOBAL_LOAD_BALANCE, (void *)&globalFlag))
	{
		snprintf(tmpBuf, sizeof(tmpBuf), "%s:%s", Tset_mib_error, MIB_GLOBAL_LOAD_BALANCE);
		goto setErr_dyn_balance;
	}

	if(globalFlag != global_balance_enable)
		global_balance_flag = 1;

	if(!globalFlag)
	{
		printf("[%s %d] total load balance is disabled!\n",__FUNCTION__,__LINE__);
		goto setOk_dyn_Balance;
	}
	
	strVal = boaGetVar(wp, "dynamic_balance_enable", "");
	if (strVal[0]) {
		if (strVal[0] == '0')
			vChar = 0;
		else if(strVal[0] == '1')
			vChar = 1;
	}

	if(vChar)
	{
		strVal = boaGetVar(wp, "dynamic_balance_type", "");
		if (strVal[0]) {
			if (strVal[0] == '1'){
				//boardband load balance
				vChar_type = 1;
			}
			else if(strVal[0] == '2'){
				//connect load balance
				vChar_type = 2;
			}
		}
	}

	if(!mib_get_s(MIB_DYNAMIC_LOAD_BALANCE, (void *)&dy_enable,sizeof(dy_enable)))
	{
		snprintf(tmpBuf, sizeof(tmpBuf), "%s:%s", Tget_mib_error, MIB_DYNAMIC_LOAD_BALANCE);
		goto setErr_dyn_balance;
	}

	if(vChar != dy_enable)
		dy_enable_flag = 1;

	if(!mib_set(MIB_DYNAMIC_LOAD_BALANCE, (void *)&vChar))
	{
		snprintf(tmpBuf, sizeof(tmpBuf), "%s:%s", Tset_mib_error, MIB_DYNAMIC_LOAD_BALANCE);
		goto setErr_dyn_balance;
	}

	if(vChar_type)
	{
		if(!mib_set(MIB_DYNAMIC_LOAD_BALANCE_TYPE, (void *)&vChar_type))
		{
			snprintf(tmpBuf, sizeof(tmpBuf), "%s:%s", Tset_mib_error, MIB_DYNAMIC_LOAD_BALANCE_TYPE);
			goto setErr_dyn_balance;
		}
	}

	if(!vChar){
		printf("[%s %d] dynamic load balance is disabled!\n",__FUNCTION__,__LINE__ );
		goto setOk_dyn_Balance;
	}

	addDynbalance = boaGetVar(wp, "addDynBalance", "");
	//add dynmaic load balance
	if(addDynbalance[0]){
		total_wan_num = mib_chain_total(MIB_ATM_VC_TBL);
		for(i = 0; i < total_wan_num; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, &wan_entry))
				continue;

			ifGetName(wan_entry.ifIndex, wanif, sizeof(wanif));

			dyn_type_old = wan_entry.balance_type;
			dy_weight_old = wan_entry.weight;
			
			if(vChar_type==RTK_CONNECT_TYPE){
				for(j =0; j< total_wan_num; j++){
					snprintf(tmpbuf1, 32, "wan_name_ct_%d", j);
					strWanDevName = boaGetVar(wp, tmpbuf1, "");
					if(!strncmp(strWanDevName, wanif, sizeof(wanif)))
					{
						snprintf(tmpbuf2, 32, "dyn_select_%d", j);
						dynSelectValue = boaGetVar(wp, tmpbuf2, "");
						dyn_select_value = atoi(dynSelectValue);

						snprintf(tmpbuf3, 32, "dyn_ct_type_%d", j);
						dynTypeValue = boaGetVar(wp, tmpbuf3, "");
						dyn_type_value = atoi(dynTypeValue);

						break;
					}
				}

				wan_entry.balance_type = (unsigned int)dyn_type_value;
				wan_entry.weight = (unsigned int)dyn_select_value;

				if(dy_weight_old != wan_entry.weight)
					dy_weight_flag = 1;
				
				if(!mib_chain_update(MIB_ATM_VC_TBL, (void*)&wan_entry, (unsigned int)i))
				{
					snprintf(tmpBuf, sizeof(tmpBuf), "%s", Tadd_chain_error);
					goto setErr_dyn_balance;
				}
			}
			else if(vChar_type == RTK_BANDWIDTH_TYPE){
				for(j =0; j< total_wan_num; j++){
					snprintf(tmpbuf1, 32, "wan_name_bd_%d", j);
					strWanDevName = boaGetVar(wp, tmpbuf1, "");
					if(!strncmp(strWanDevName, wanif, sizeof(wanif)))
					{
						snprintf(tmpbuf2, 32, "dyn_wan_bd_%d", j);
						dynSelectValue = boaGetVar(wp, tmpbuf2, "");
						dyn_select_value = atoi(dynSelectValue);

						snprintf(tmpbuf3, 32, "dyn_bd_type_%d", j);
						dynTypeValue = boaGetVar(wp, tmpbuf3, "");
						dyn_type_value = atoi(dynTypeValue);
						break;
					}
				}

				wan_entry.balance_type = (unsigned int)dyn_type_value;
				wan_entry.weight = (unsigned int)dyn_select_value;
				if(dy_weight_old != wan_entry.weight)
					dy_weight_flag = 1;
				if(!mib_chain_update(MIB_ATM_VC_TBL, (void*)&wan_entry, (unsigned int)i))
				{
					snprintf(tmpBuf, sizeof(tmpBuf), "%s", Tadd_chain_error);
					goto setErr_dyn_balance;
				}
			}

			if(dyn_type_old != dyn_type_value)   //wan 's balance type be changed
			{			

				rtk_set_default_route_by_wan_info(&wan_entry);
				
				if(dyn_type_value == 0)   //share -> private
				{
					memset(cmd, 0, sizeof(cmd));
					snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process type_to_private %s",wanif);
					system(cmd);
				}
				else   // private -> share
				{
					memset(cmd, 0, sizeof(cmd));
					snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process type_to_share");
					system(cmd);
				}
			}
		}

		goto setOk_dyn_Balance;
	}

setOk_dyn_Balance:
	//	Magician: Commit immediately
	
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	if(global_balance_flag)
	{
		//process manual load balance when change global balance states
		set_manual_load_balance_rules();
		
		if(globalFlag)  
		{
			if(vChar)	//global switch to enable	
			{
				memset(cmd, 0, sizeof(cmd));
				snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process dy_to_enable");
				system(cmd);
			} 
		}
		else   //global switch to disable         
		{
			memset(cmd, 0, sizeof(cmd));
			snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process dy_to_disable");
			system(cmd);
		}  
	}
	
	if(dy_enable_flag)
	{
		if(vChar)   //dynamic switch to enable
		{
			memset(cmd, 0, sizeof(cmd));
			snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process dy_to_enable");
			system(cmd);
		} 
		else       //dynamic switch to disable
		{
			memset(cmd, 0, sizeof(cmd));
			snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process dy_to_disable");
			system(cmd);
		}   
	}

	if(dy_weight_flag)
	{
		memset(cmd, 0, sizeof(cmd));
		snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process dy_mod_weight");
		system(cmd);
	}	


	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);

	return;

setErr_dyn_balance:
	ERR_MSG(tmpBuf);	
}

int rtk_get_all_static_entry(char (*mac_str)[MAC_ADDR_STRING_LEN])
{
	int i , con_cnt = 0 , totalEntry = 0;
	MIB_STATIC_LOAD_BALANCE_T sta_balance_entry;

	totalEntry = mib_chain_total(MIB_STATIC_LOAD_BALANCE_TBL);

	for (i=0; i<totalEntry; i++) {
		if(!mib_chain_get(MIB_STATIC_LOAD_BALANCE_TBL, i, &sta_balance_entry))
			continue;

		if(mac_str[con_cnt])
			memcpy(mac_str[con_cnt], sta_balance_entry.mac, MAC_ADDR_STRING_LEN);
		con_cnt++;
	}

	return con_cnt;
}

int rtk_delete_dynamic_entry_when_manual_enable(void)
{
	MIB_STATIC_LOAD_BALANCE_T sta_balance_entry;
	int i = 0, totalEntry = 0;
	char cmd[128] = {0};
	
	totalEntry = mib_chain_total(MIB_STATIC_LOAD_BALANCE_TBL);
	for (i=0; i<totalEntry; i++) {
		if(!mib_chain_get(MIB_STATIC_LOAD_BALANCE_TBL, i, &sta_balance_entry))
			continue;
		
		memset(cmd, 0, sizeof(cmd));
		snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process add_static %s",sta_balance_entry.mac);
		system(cmd);
	}

	return 1;
}

int rtk_check_manual_balance_is_exist(char *mac)
{
	MIB_STATIC_LOAD_BALANCE_T sta_balance_entry;
	int i = 0, totalEntry = 0;
	
	totalEntry = mib_chain_total(MIB_STATIC_LOAD_BALANCE_TBL);
	for (i=0; i<totalEntry; i++) {
		if(!mib_chain_get(MIB_STATIC_LOAD_BALANCE_TBL, i, &sta_balance_entry))
			continue;

		if(!strncmp(mac,sta_balance_entry.mac,MAC_ADDR_STRING_LEN-1))
		{
			return 1;
		}
	}

	return 0;
}

void formStaticBalance(request * wp, char *path, char *query)
{
	char *submitUrl, *strVal;
	char *totalNum;
	char *strDevName, *strMac, *strWanDevName,*delDevMac;
	char *addStaBalance, *delStabalance, *delAllStabalance;
	char vChar = 0, sta_enable = 0,sta_enable_flag = 0;
	char tmpBuf[100];
	MIB_CE_ATM_VC_T wan_entry;
	MIB_STATIC_LOAD_BALANCE_T sta_balance_entry;
	int intVal = 0, mibTblId = 0;
	int i =0, deleted = 0, totalEntry = 0,j = 0, con_cnt;
	int total_lan_num = 0, get_del_dev_flag = 0, total_wan_num = 0;
	char tmpbuf1[32]={0},tmpbuf2[32]={0},tmpbuf3[32]={0};
	char wanif[IFNAMSIZ] = {0};
	char save_mac_str[16][MAC_ADDR_STRING_LEN] = {0};    //16 is static ld table max num
	char cmd[128] = {0};
	
	strVal = boaGetVar(wp, "balance_enable", "");
	addStaBalance = boaGetVar(wp, "addStaBalance", "");
	delStabalance = boaGetVar(wp, "deleteSelStaBalance", "");
	delAllStabalance = boaGetVar(wp, "deleteAllStaBalance", "");

	//enable or disable manual load balance
	if (strVal[0]) {
		if (strVal[0] == '0')
			vChar = 0;
		else if(strVal[0] == '1')
			vChar = 1;

		if(!mib_get_s(MIB_STATIC_LOAD_BALANCE_ENABLE, (void *)&sta_enable,sizeof(sta_enable)))
		{
			snprintf(tmpBuf, sizeof(tmpBuf), "%s:%s", Tget_mib_error, MIB_STATIC_LOAD_BALANCE_ENABLE);
			goto setErr_sta_balance;
		}

		if(vChar != sta_enable)
			sta_enable_flag = 1;
		
		if(!mib_set(MIB_STATIC_LOAD_BALANCE_ENABLE, (void *)&vChar))
		{
			snprintf(tmpBuf, sizeof(tmpBuf), "%s:%s", Tset_mib_error, MIB_STATIC_LOAD_BALANCE_ENABLE);
			goto setErr_sta_balance;
		}

		if(!vChar){
			printf("[%s %d] static load balance is disable!\n",__FUNCTION__,__LINE__ );
			goto setOk_StaBalance;
		}
	}

	//delete all entries
	if(delAllStabalance[0]){
		con_cnt = rtk_get_all_static_entry(save_mac_str);
			
		mibTblId = MIB_STATIC_LOAD_BALANCE_TBL;
		printf("%s %d delete all static entry!\n",__FUNCTION__,__LINE__);	
		mib_chain_clear(mibTblId); /* clear chain record */
	
		for(i = 0; i < con_cnt; i++)
		{	
			memset(cmd, 0, sizeof(cmd));
			snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process del_static %s",save_mac_str[i]);
			system(cmd);
		}
	
		goto setOk_StaBalance;
	}

	//delete selected entries
	if(delStabalance[0])
	{
		totalEntry = mib_chain_total(MIB_STATIC_LOAD_BALANCE_TBL);
		for (i=0; i<totalEntry; i++) {
			get_del_dev_flag = 0;
			snprintf(tmpbuf1, 32, "delete%d", i);
			strVal = boaGetVar(wp, tmpbuf1, "");

			snprintf(tmpbuf2, 32, "del_dev_%d", i);
			delDevMac = boaGetVar(wp, tmpbuf2, "");

			if ( !strncmp(strVal, "ON",2)) {
				for(j= 0; j<totalEntry; j++)
				{
					if(!mib_chain_get(MIB_STATIC_LOAD_BALANCE_TBL, j, &sta_balance_entry))
						continue;

					if(!strncmp(delDevMac, sta_balance_entry.mac, MAC_ADDR_STRING_LEN))
					{
						get_del_dev_flag = 1;
						strWanDevName = sta_balance_entry.wan_itfname;
						break;
					}
				}

				if(get_del_dev_flag){
					if(mib_chain_delete(MIB_STATIC_LOAD_BALANCE_TBL, j) != 1) {
						snprintf(tmpBuf, sizeof(tmpBuf), "%s", Tdelete_chain_error);
						goto setErr_sta_balance;
					}
					 //when delete static entry ,the mac need add dynamic load balance ,signal example is "00:11:22:33:44:55"
					memset(cmd, 0, sizeof(cmd));
					snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process del_static %s",sta_balance_entry.mac);
					system(cmd);
				}
			}
		}
		goto setOk_StaBalance;
	}

	//add static load balance
	if(addStaBalance[0]){
		//validity check.
		totalEntry = mib_chain_total(MIB_STATIC_LOAD_BALANCE_TBL);
		if(totalEntry == RTK_MANUAL_BALANCE_TOTAL_NUM)
		{
			snprintf(tmpBuf, sizeof(tmpBuf), "%s", "MIB_STATIC_LOAD_BALANCE_TBL is full!");
				goto setErr_sta_balance;
		}

		totalNum = boaGetVar(wp, "total_lan_num", "");
		total_lan_num = atoi(totalNum);
		for(i = 0; i < total_lan_num;i++)
		{
			memset(tmpbuf1,0,sizeof(tmpbuf1));
			snprintf(tmpbuf1,sizeof(tmpbuf1),"select%d",i);
			strVal = boaGetVar(wp, tmpbuf1, "");
			
			if(strVal[0] == '1')
			{
				memset(&sta_balance_entry,0,sizeof(sta_balance_entry));
				memset(tmpbuf1,0,sizeof(tmpbuf1));
				memset(tmpbuf2,0,sizeof(tmpbuf2));
				memset(tmpbuf3,0,sizeof(tmpbuf3));
				snprintf(tmpbuf1,sizeof(tmpbuf1),"dev_name_%d",i);
				snprintf(tmpbuf2,sizeof(tmpbuf2),"mac_%d",i);
				snprintf(tmpbuf3,sizeof(tmpbuf3),"interface_%d",i);

				sta_balance_entry.enable  = 1;

				strDevName = boaGetVar(wp, tmpbuf1, "");
				strMac = boaGetVar(wp, tmpbuf2, "");
				strWanDevName = boaGetVar(wp, tmpbuf3, "");

				if(strDevName[0] && strMac[0] && strWanDevName[0])
				{
					snprintf(sta_balance_entry.dev_name, RTK_LAN_NAME_LENTH, "%s", strDevName);
					snprintf(sta_balance_entry.mac, MAC_ADDR_STRING_LEN, "%s", strMac);
					snprintf(sta_balance_entry.wan_itfname, IFNAMSIZ, "%s", strWanDevName);
				}
				else{
					snprintf(tmpBuf, sizeof(tmpBuf), "%s", "get manual load balance information fail!");
					goto setErr_sta_balance;
				}
				if(rtk_check_manual_balance_is_exist(sta_balance_entry.mac)==1){
					snprintf(tmpBuf, sizeof(tmpBuf), "%s", "manual load balance rule already exists!");
					goto setErr_sta_balance;
				}
				//printf("=====%s %s %s\n",sta_balance_entry.dev_name,sta_balance_entry.mac,sta_balance_entry.wan_itfname);

				intVal = mib_chain_add(MIB_STATIC_LOAD_BALANCE_TBL, (unsigned char*)&sta_balance_entry);
				if (intVal == 0) {
					snprintf(tmpBuf, sizeof(tmpBuf), "%s", Tadd_chain_error);
					goto setErr_sta_balance;
				}
				else if (intVal == -1) {
					snprintf(tmpBuf, sizeof(tmpBuf), "%s", strTableFull);
					goto setErr_sta_balance;
				}
				 //when add static entry ,the mac need delete dynamic load balance ,signal example is "00:11:22:33:44:55"
				memset(cmd, 0, sizeof(cmd));
				snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process add_static %s",sta_balance_entry.mac);
				system(cmd);
			}
		}
		goto setOk_StaBalance;
	}

setOk_StaBalance:
	//	Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	if(sta_enable_flag)
	{
		if(vChar)  //switch to enable
		{	
			rtk_delete_dynamic_entry_when_manual_enable();
		}
		else   //switch to disable
		{
			memset(cmd, 0, sizeof(cmd));
			snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process dy_to_enable");
			system(cmd);
		}
	}

	//set rules to manual load balance
	set_manual_load_balance_rules();

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);

	return;

setErr_sta_balance:
	ERR_MSG(tmpBuf);	
}


int sta_balance_list(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	unsigned int wan_entry_num,sta_load_entry_num;
	int i = 0, j = 0;
	MIB_STATIC_LOAD_BALANCE_T Entry;
	MIB_CE_ATM_VC_T wan_entry;
	char *start_ip, *end_ip;

	char wanif[IFNAMSIZ] = {0};
	char tmpBuf[12] = {0};
	char global_balance_enable = 0, manual_enable = 0;
	
	sta_load_entry_num = mib_chain_total(MIB_STATIC_LOAD_BALANCE_TBL);
	wan_entry_num = mib_chain_total(MIB_ATM_VC_TBL);
	if(!mib_get_s(MIB_GLOBAL_LOAD_BALANCE, (void *)&global_balance_enable,sizeof(global_balance_enable)))
	{
		return nBytesSent;
	}

	if(!mib_get_s(MIB_STATIC_LOAD_BALANCE_ENABLE, (void *)&manual_enable,sizeof(manual_enable)))
	{
		return nBytesSent;
	}

	if(manual_enable == 0 || global_balance_enable == 0)
	{
		snprintf(tmpBuf,12,"%s","disabled");
	}
	nBytesSent += boaWrite(wp, "<tr>"
		"<th align=\"left\" width=\"20%%\" bgcolor=\"#C0C0C0\">%s</th>\n"
		"<th align=\"left\" width=\"20%%\" bgcolor=\"#C0C0C0\">%s</th>\n"
		"<th align=\"left\" width=\"20%%\" bgcolor=\"#C0C0C0\">%s</th>\n"
		"<th align=\"left\" width=\"40%%\" bgcolor=\"#C0C0C0\">%s</th></tr>\n",
		multilang(LANG_ENABLE_STATIC_LOAD_BALANCE), multilang(LANG_LOAD_BALANCE_DEV_NAME), multilang(LANG_MAC_ADDRESS),multilang(LANG_WAN_INTERFACE));
	
	for(i = 0;i<wan_entry_num; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &wan_entry))
			continue;
		
		ifGetName(wan_entry.ifIndex, wanif, sizeof(wanif));


		for(j=0; j<sta_load_entry_num; j++)
		{
			if(!mib_chain_get(MIB_STATIC_LOAD_BALANCE_TBL, j, &Entry))
				continue;

			if(strncmp(Entry.wan_itfname, wanif, sizeof(wanif)))
				continue;

			nBytesSent += boaWrite(wp, "<tr>"
			"<td align=\"left\" width=\"20%%\" bgcolor=\"#808080\"><input type=\"checkbox\" name=\"delete%d\" value=\"ON\" %s></td>\n"
			"<td align=\"left\" width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\">%s<input type=\"hidden\" name=\"del_dev_%d\" value=\"%s\"></font></td>\n"
			"<td align=\"left\" width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\">%s</font></td>"
			"<td align=\"left\" width=\"40%%\" bgcolor=\"#808080\"><font size=\"2\">%s</font></td></tr>\n",
			j, tmpBuf, Entry.dev_name, j, Entry.mac, Entry.mac,wanif);
		}


	}
	
	return nBytesSent;
}

int rtk_set_connect_load_balance(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0, entry_num = 0,i = 0;
	MIB_CE_ATM_VC_T entry;
	char wanif[IFNAMSIZ] = {0};
	char tmpBuf[12] = {0};
	char global_balance_enable = 0, dyn_enable = 0;

	if(!mib_get_s(MIB_GLOBAL_LOAD_BALANCE, (void *)&global_balance_enable,sizeof(global_balance_enable)))
	{
		return nBytesSent;
	}

	if(!mib_get_s(MIB_DYNAMIC_LOAD_BALANCE, (void *)&dyn_enable,sizeof(dyn_enable)))
	{
		return nBytesSent;
	}

	if(dyn_enable == 0 || global_balance_enable == 0)
	{
		snprintf(tmpBuf,12,"%s","disabled");
	}
	
	entry_num = mib_chain_total(MIB_ATM_VC_TBL);

	for(i=0;i<entry_num;i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
			continue;

		ifGetName(entry.ifIndex, wanif, sizeof(wanif));
		
		nBytesSent += boaWrite(wp, "<tr>"
			"<td align=\"left\" width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\">%s<input type=\"hidden\" name=\"wan_name_ct_%d\" value=\"%s\"></td>\n"
			"<td align=\"left\" width=\"40%%\" bgcolor=\"#808080\"><font size=\"2\"><select id=\"ct_type_%d\" name=\"dyn_ct_type_%d\" style=\"width:170px\" %s>\n"
					"<option value=\"0\">%s</option>\n"
					"<option value=\"1\">%s</option>\n"
					"</select><input type=\"hidden\" id=\"ct_type_value_%d\" value=\"%d\"></td>\n"
			"<td align=\"left\" width=\"15%%\" bgcolor=\"#808080\"><font size=\"2\">%d</td>\n"
			"<td align=\"left\" width=\"25%%\" bgcolor=\"#808080\"><font size=\"2\"><select id=\"ct_name_%d\" name=\"dyn_select_%d\" %s>\n"
					"<option value=\"1\">1</option>\n"
					"<option value=\"2\">2</option>\n"
					"<option value=\"3\">3</option>\n"
					"<option value=\"4\">4</option>\n"
					"<option value=\"5\">5</option>\n"
					"<option value=\"6\">6</option>\n"
					"<option value=\"7\">7</option>\n"
					"<option value=\"8\">8</option>\n"
					"<option value=\"9\">9</option>\n"
					"<option value=\"10\">10</option>\n"
					"</select><input type=\"hidden\" id=\"ct_selected_value_%d\" value=\"%d\"></td></tr>\n",
			wanif, i, wanif,
			i, i, tmpBuf, multilang(LANG_DYN_PROPRIETARY_NETWORK), multilang(LANG_DYN_SHARED_NETWORK), i, entry.balance_type,
			entry.logic_port,
			i, i, tmpBuf, i, entry.weight-1);
	}

	return nBytesSent;
}

int rtk_set_bandwidth_load_balance(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0, entry_num = 0,i = 0;
	MIB_CE_ATM_VC_T entry;
	char wanif[IFNAMSIZ] = {0};
	char tmpBuf[12] = {0};
	char global_balance_enable = 0, dyn_enable = 0;

	entry_num = mib_chain_total(MIB_ATM_VC_TBL);

	if(!mib_get_s(MIB_GLOBAL_LOAD_BALANCE, (void *)&global_balance_enable,sizeof(global_balance_enable)))
	{
		return nBytesSent;
	}

	if(!mib_get_s(MIB_DYNAMIC_LOAD_BALANCE, (void *)&dyn_enable,sizeof(dyn_enable)))
	{
		return nBytesSent;
	}

	if(dyn_enable == 0 || global_balance_enable == 0)
	{
		snprintf(tmpBuf,12,"%s","disabled");
	}

	for(i=0;i<entry_num;i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
			continue;

		ifGetName(entry.ifIndex, wanif, sizeof(wanif));
		
		nBytesSent += boaWrite(wp, "<tr>"
			"<td align=\"left\" width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\">%s<input type=\"hidden\" name=\"wan_name_bd_%d\" value=\"%s\"></td>\n"
			"<td align=\"left\" width=\"40%%\" bgcolor=\"#808080\"><font size=\"2\"><select id=\"bd_type_%d\" name=\"dyn_bd_type_%d\" style=\"width:170px\" %s>\n"
					"<option value=\"0\">%s</option>\n"
					"<option value=\"1\">%s</option>\n"
					"</select><input type=\"hidden\" id=\"bd_type_value_%d\" value=\"%d\"></td>\n"
			"<td align=\"left\" width=\"15%%\" bgcolor=\"#808080\"><font size=\"2\">%d</td>\n"
			"<td align=\"left\" width=\"25%%\" bgcolor=\"#808080\"><font size=\"2\"><input type=\"text\" id =\"bd_name_%d\" name = \"dyn_wan_bd_%d\" size=\"12\" maxlength=\"12\" value = %d></td></tr>\n",
			wanif, i, wanif,
			i, i, tmpBuf, multilang(LANG_DYN_PROPRIETARY_NETWORK), multilang(LANG_DYN_SHARED_NETWORK), i, entry.balance_type,
			entry.logic_port,
			i, i, entry.weight ,tmpBuf);
	}

	return nBytesSent;
}

#if defined(CONFIG_USER_LANNETINFO) && defined(CONFIG_USER_REFINE_CHECKALIVE_BY_ARP)
int rtk_set_lan_sta_load_balance(int eid, request * wp, int argc, char **argv)
{
	int i = 0,idx = 0,entry_num = 0;
	int nBytesSent=0;
	unsigned int count = 0;
	unsigned char mac[18] = {0};
	MIB_CE_ATM_VC_T entry;
	char wanif[IFNAMSIZ] = {0};
	char *buf = NULL;
	char option_buf[RTK_SELECT_OPTION_LEN_OF_WAN_NAME] = {0};
	char dev_name[RTK_LAN_NAME_LENTH] = {0};
	char tmpBuf[12] = {0};
	char global_balance_enable = 0, manual_enable = 0;
	lanHostInfo_t *pLanNetInfo=NULL;

	if(!mib_get_s(MIB_GLOBAL_LOAD_BALANCE, (void *)&global_balance_enable,sizeof(global_balance_enable)))
	{
		return nBytesSent;
	}

	if(!mib_get_s(MIB_STATIC_LOAD_BALANCE_ENABLE, (void *)&manual_enable,sizeof(manual_enable)))
	{
		return nBytesSent;
	}

	if(manual_enable == 0 || global_balance_enable == 0)
	{
		snprintf(tmpBuf,12,"%s","disabled");
	}
	
	if(get_lan_net_info(&pLanNetInfo, &count) != 0)
	{
		goto get_info_end;
	}

	entry_num = mib_chain_total(MIB_ATM_VC_TBL);

	buf = (char *)malloc(sizeof(char)*RTK_SELECT_OPTION_LEN_OF_WAN_NAME*entry_num);

	if(buf == NULL){
		printf("%s %d buf is NULL\n",__FUNCTION__,__LINE__);
		goto get_info_end;
	}
	memset(buf,0,sizeof(buf));
	
	for(i=0;i<entry_num;i++)
	{
		memset(wanif,0,sizeof(wanif));
		memset(option_buf,0,sizeof(option_buf));
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
		    continue;

		ifGetName(entry.ifIndex, wanif, sizeof(wanif));
		snprintf(option_buf,sizeof(option_buf),"<option value=\"%s\">%s</option>",wanif,wanif);
		strncat(buf,option_buf,sizeof(option_buf));
	}

	if(count > 0)
	{
		for(i=0; (i<count) && (idx<MAX_STA_NUM*2); i++){
			memset(mac,0,sizeof(mac));
			snprintf(mac,sizeof(mac),"%02x:%02x:%02x:%02x:%02x:%02x", pLanNetInfo[idx].mac[0],pLanNetInfo[idx].mac[1],pLanNetInfo[idx].mac[2],pLanNetInfo[idx].mac[3],pLanNetInfo[idx].mac[4],pLanNetInfo[idx].mac[5]);
			if(strlen(pLanNetInfo[idx].devName)==0)
				snprintf(dev_name, RTK_LAN_NAME_LENTH, "unknown");
			else
				snprintf(dev_name, RTK_LAN_NAME_LENTH, "%s", pLanNetInfo[idx].devName);
			
			nBytesSent += boaWrite(wp, "<tr>"
			"<td align=\"left\" width=\"20%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" id=\"set%d\" name=\"select%d\" value=\"0\" %s></td>\n"
			"<td align=\"left\" width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s<input type=\"hidden\" name=\"dev_name_%d\" value=\"%s\"></td>\n"
			"<td align=\"left\" width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s<input type=\"hidden\" name=\"mac_%d\" value=\"%s\"></td>\n"
			"<td align=\"left\" width=\"15%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n"
			"<td align=\"left\" width=\"25%%\" bgcolor=\"#C0C0C0\"><select name =\"interface_%d\" %s>%s</select></td></tr>\n",
			idx,idx, tmpBuf,
			dev_name,idx,dev_name,
			mac,idx,mac,
			pLanNetInfo[idx].phy_port,
			idx,tmpBuf, buf);

			idx++;
		}
		nBytesSent += boaWrite(wp, "<tr><input type=\"hidden\" name=\"total_lan_num\" value=\"%d\"></tr>\n", idx);
	}

	if(buf)
		free(buf);
	
get_info_end:
	if(pLanNetInfo)
		free(pLanNetInfo);


	return nBytesSent;
}

int LoadBalanceStatsList(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0,i = 0, j = 0,k=0, n = 0;
	int sta_entry_num = 0, wan_entry_idx = 0, wan_mib_entry = 0;
	MIB_CE_ATM_VC_T entry;
	unsigned int count = 0, wanCount = 0, mappingCount = 0;
	char wanif[IFNAMSIZ] = {0}, cmp_wan_name[IFNAMSIZ] = {0};
	MIB_STATIC_LOAD_BALANCE_T sta_entry;
	unsigned int wan_mark = 0, wan_mask = 0;
	con_wan_info_T *pConWandata = NULL;
	lanHostInfo_t  LanNetInfo = {0};
	lanHostInfo_t *pLanNetInfo = NULL;
	unsigned char lan_mac[18] = {0};
	char LanDevName[64] = {0};
	int find_wan_flag = 0;
	char default_wan_itf[IFNAMSIZ] = {0};
	char global_balance_enable = 0, sta_enable = 0, dyn_enable = 0;
	int ret_1 = 0, ret_2 = 0;
	int wan_connect_flag = 0;
	char wan_type[16] = {0};

	ret_1 = get_lan_net_info(&pLanNetInfo, &count);
	ret_2 = get_con_wan_info(&pConWandata, &wanCount);

	if(ret_1 != 0)
		goto get_info_end;
	
	if((!mib_get_s(MIB_STATIC_LOAD_BALANCE_ENABLE, (void *)&sta_enable,sizeof(sta_enable))) ||
		(!mib_get_s(MIB_GLOBAL_LOAD_BALANCE, (void *)&global_balance_enable,sizeof(global_balance_enable))) ||
		(!mib_get_s(MIB_DYNAMIC_LOAD_BALANCE, (void *)&dyn_enable,sizeof(dyn_enable))))
	{
		goto get_info_end;
	}

	rtk_get_default_gw_wan_from_route_table(default_wan_itf);
	
	nBytesSent +=boaWrite(wp, "&nbsp;&nbsp;\n"
	"<div class=\"column\">\n"
	"<div class=\"column_title\">\n"
	"<div class=\"column_title_left\"></div>\n"
	"<p>%s</p>\n"
	"<div class=\"column_title_right\"></div>\n"
	"</div>\n"
	"<div class=\"data_common data_vertical\">\n",multilang(LANG_ASSIGNMENT_TABLE));
	
	nBytesSent +=boaWrite(wp, "<table>\n");
	nBytesSent += boaWrite(wp, "<tr>"
	"<td align=center width=\"14%%\" bgcolor=\"#808080\" class=\"table_item\"><b>%s</b></td>\n"
	"<td align=center width=\"16%%\" bgcolor=\"#808080\" class=\"table_item\"><b>%s</b></td>\n"
	"<td align=center width=\"16%%\" bgcolor=\"#808080\" class=\"table_item\"><b>%s</b></td>\n"
	"<td align=center width=\"16%%\" bgcolor=\"#808080\" class=\"table_item\"><b>%s</b></td>\n"
	"<td align=center width=\"10%%\" bgcolor=\"#808080\" class=\"table_item\"><b>%s</b></td>\n"
	"<td align=center width=\"10%%\" bgcolor=\"#808080\" class=\"table_item\"><b>%s</b></td>\n"
	"<td align=center width=\"10%%\" bgcolor=\"#808080\" class=\"table_item\"><b>%s</b></td>\n"
	"<td align=center width=\"8%%\" bgcolor=\"#808080\" class=\"table_item\"><b>%s</b></td></tr>\n",
	multilang(LANG_WAN_INTERFACE),multilang(LANG_LOAD_BALANCE_DEV_NAME),multilang(LANG_MAC_ADDRESS),multilang(LANG_IP_ADDRESS), multilang(LANG_LAN_PORT), multilang(LANG_RX_PKT), multilang(LANG_TX_PKT),multilang(LANG_TYPE));

	sta_entry_num = mib_chain_total(MIB_STATIC_LOAD_BALANCE_TBL);
	wan_mib_entry = mib_chain_total(MIB_ATM_VC_TBL);

	for(n = 0; n< wan_mib_entry; n++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, n, &entry)){
			continue;
		}
		
		if(rtk_check_wan_is_connected(&entry)){
			wan_connect_flag = 1;
			break;
		}
	}

	//when all wan are disconnected, only show lan dev without wan info.
	if(wan_connect_flag == 0 && count > 0)
	{
		memset(wanif,0,IFNAMSIZ);
		for(i = 0; i<count; i++)
		{
			snprintf(lan_mac,sizeof(lan_mac),"%02x:%02x:%02x:%02x:%02x:%02x", pLanNetInfo[i].mac[0],pLanNetInfo[i].mac[1],pLanNetInfo[i].mac[2],pLanNetInfo[i].mac[3],pLanNetInfo[i].mac[4],pLanNetInfo[i].mac[5]);
			if(strlen(pLanNetInfo[i].devName)==0)
				snprintf(LanDevName, MAX_LANNET_DEV_NAME_LENGTH, "unknown");
			else
				snprintf(LanDevName, MAX_LANNET_DEV_NAME_LENGTH, "%s", pLanNetInfo[i].devName);

			nBytesSent += boaWrite(wp, "<tr>"
				"<td align=center width=\"14%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
				"<td align=center width=\"16%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
				"<td align=center width=\"16%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
				"<td align=center width=\"16%%\" bgcolor=\"#C0C0C0\">%d.%d.%d.%d</td>\n"
				"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%d</td>\n"
				"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%d</td>\n"
				"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%d</td>\n"
				"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">N/A</td></tr>\n",
				wanif, LanDevName, lan_mac,  pLanNetInfo[i].ip>>24, ( pLanNetInfo[i].ip>>16) & 0xff, ( pLanNetInfo[i].ip>>8) & 0xff, pLanNetInfo[i].ip & 0xff, pLanNetInfo[i].phy_port, pLanNetInfo[i].rx_packets, pLanNetInfo[i].tx_packets);
		}
	}
	
	if(count>0 && wan_connect_flag == 1){
		for(n = 0; n< wan_mib_entry; n++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, n, &entry)){
				continue;
			}
			
			ifGetName(entry.ifIndex, cmp_wan_name, sizeof(cmp_wan_name));
			
			for(i = 0; i<count; i++)
			{
				find_wan_flag  = 0;
				memset(wanif,0,IFNAMSIZ);
				
				snprintf(lan_mac,sizeof(lan_mac),"%02x:%02x:%02x:%02x:%02x:%02x", pLanNetInfo[i].mac[0],pLanNetInfo[i].mac[1],pLanNetInfo[i].mac[2],pLanNetInfo[i].mac[3],pLanNetInfo[i].mac[4],pLanNetInfo[i].mac[5]);
				if(strlen(pLanNetInfo[i].devName)==0)
						snprintf(LanDevName, MAX_LANNET_DEV_NAME_LENGTH, "unknown");
					else
						snprintf(LanDevName, MAX_LANNET_DEV_NAME_LENGTH, "%s", pLanNetInfo[i].devName);

				if(sta_enable != 0 && global_balance_enable != 0)
				{
					//check munual load balance entry which could has the same mac. 
					for(j=0;j<sta_entry_num;j++)
					{
						if(!mib_chain_get(MIB_STATIC_LOAD_BALANCE_TBL, j, &sta_entry))
							continue;

						if(!strncasecmp(lan_mac,sta_entry.mac,MAC_ADDR_STRING_LEN-1))
						{
							find_wan_flag  = 1;
							snprintf(wanif, IFNAMSIZ, "%s", sta_entry.wan_itfname);
							snprintf(wan_type, 16, "manual");
							break;
						}
					}
				}
				
				//find port mapping Lan pc info from port mapping when disable auto load balance or global load balance.
				if(find_wan_flag == 0)
				{
					if(rtk_check_port_mapping_rules_exist(pLanNetInfo[i].ifName, &wan_entry_idx, &wan_mark))
					{
						if(mib_chain_get(MIB_ATM_VC_TBL, wan_entry_idx, &entry)){
							ifGetName(entry.ifIndex, wanif, sizeof(wanif));
							find_wan_flag  = 1;
							snprintf(wan_type, 16, "manual");
						}
					}
				}

				if(dyn_enable != 0 && global_balance_enable != 0)
				{
					//check auto load balance entry which could has the same mac. 
					if(wanCount > 0 && find_wan_flag == 0 && ret_2 == 0)
					{
						for(k=0;k<wanCount;k++)
						{
							if(pConWandata[k].wan_mark)
							{
								if(find_wan_flag == 1)
									break;
								
								if(rtk_get_wan_itf_by_mark(wanif,pConWandata[k].wan_mark)!=1)
								{
									//printf("this mark cant find wanitf name.\n");
									continue;
								}
								for(j=0; j<MAX_MAC_NUM; j++)
								{
									if(strlen(pConWandata[k].mac_info[j].mac_str))
									{
										if(!strncasecmp(lan_mac, pConWandata[k].mac_info[j].mac_str, MAC_ADDR_STRING_LEN-1))
										{
											find_wan_flag  = 1;
											snprintf(wan_type, 16, "auto");
											break;
										}
									}
								}
							}
						}
					}
				}

				if(find_wan_flag == 1)
				{
					if(strncmp(wanif,cmp_wan_name,IFNAMSIZ))
						continue;
						
					nBytesSent += boaWrite(wp, "<tr>"
						"<td align=center width=\"14%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
						"<td align=center width=\"16%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
						"<td align=center width=\"16%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
						"<td align=center width=\"16%%\" bgcolor=\"#C0C0C0\">%d.%d.%d.%d</td>\n"
						"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%d</td>\n"
						"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%d</td>\n"
						"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%d</td>\n"
						"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">%s</td></tr>\n",
						wanif, LanDevName, lan_mac,  pLanNetInfo[i].ip>>24, ( pLanNetInfo[i].ip>>16) & 0xff, ( pLanNetInfo[i].ip>>8) & 0xff, pLanNetInfo[i].ip & 0xff, pLanNetInfo[i].phy_port, pLanNetInfo[i].rx_packets, pLanNetInfo[i].tx_packets, wan_type);
				}
				
				if(find_wan_flag == 0){
					//lan dev<-->default wan
					if(strncmp(default_wan_itf,cmp_wan_name,IFNAMSIZ))
						continue;
					
					nBytesSent += boaWrite(wp, "<tr>"
							"<td align=center width=\"14%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
							"<td align=center width=\"16%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
							"<td align=center width=\"16%%\" bgcolor=\"#C0C0C0\">%s</td>\n"
							"<td align=center width=\"16%%\" bgcolor=\"#C0C0C0\">%d.%d.%d.%d</td>\n"
							"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%d</td>\n"
							"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%d</td>\n"
							"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\">%d</td>\n"
							"<td align=center width=\"8%%\" bgcolor=\"#C0C0C0\">default</td></tr>\n",
							default_wan_itf, LanDevName, lan_mac,  pLanNetInfo[i].ip>>24, ( pLanNetInfo[i].ip>>16) & 0xff, ( pLanNetInfo[i].ip>>8) & 0xff, pLanNetInfo[i].ip & 0xff, pLanNetInfo[i].phy_port, pLanNetInfo[i].rx_packets, pLanNetInfo[i].tx_packets);
				}
			}
		}
	}
	nBytesSent +=boaWrite(wp, "</table>\n"
	"</div>\n"
	"</div>\n");
	
get_info_end:
	if(pLanNetInfo)
		free(pLanNetInfo);
		
	if(pConWandata)
		free(pConWandata);
	
	return nBytesSent;
}

int wan_stats_show(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0,i = 0,j = 0;
	int entry_num = 0;
	MIB_CE_ATM_VC_T entry;
	char wanif[IFNAMSIZ] = {0};
	int wan_phy_port = 0;
	lan_net_port_speed_T ps[MAX_PORT_NUM] = {0};
	unsigned int up_speed = 0;
	unsigned int down_speed = 0;
	struct net_device_stats nds;
	char tmpBuf[32] = {0};
	char default_wan_itf[IFNAMSIZ]= {0};
	float total_rx_rate = 0;
	float total_tx_rate = 0;
	unsigned long long total_rx_pkt = 0;
	unsigned long long total_tx_pkt = 0;
	char total_port[12] = {0};
	int logic_port_array[MAX_TOTAL_LOGIC_PORT_NUM] = {-1,-1,-1,-1,-1,-1};
	int find_same_logic_port = 0, total_num = 0;
	char tmpbuf[6] = {0};
	char connect_status[16] = {0};

	entry_num = mib_chain_total(MIB_ATM_VC_TBL);

	#if 0
	if(get_lanNetInfo_portSpeed(ps, MAX_PORT_NUM)==RTK_FAILED)
		return nBytesSent;
	#endif

	nBytesSent +=boaWrite(wp, "<div class=\"column\">\n"
	"<div class=\"column_title\">\n"
	"<div class=\"column_title_left\"></div>\n"
	"<p>%s</p>\n"
	"<div class=\"column_title_right\"></div>\n"
	"</div>\n"
	"<div class=\"data_common data_vertical\">\n",multilang(LANG_WAN_STATS_TABLE));

	nBytesSent +=boaWrite(wp, "<table>\n");
	nBytesSent += boaWrite(wp, "<tr>"
	"<td align=center width=\"14%%\"><b>%s</b></td>\n"
	"<td align=center width=\"16%%\"><b>%s</b></td>\n"
	"<td align=center width=\"16%%\"><b>Rx Rate(Mbps)</b></td>\n"
	"<td align=center width=\"16%%\"><b>Tx Rate(Mbps)</b></td>\n"
	"<td align=center width=\"14%%\"><b>%s</b></td>\n"
	"<td align=center width=\"14%%\"><b>%s</b></td>\n"
	"<td align=center width=\"10%%\"><b>%s</b></td></tr>\n",
	multilang(LANG_WAN_INTERFACE),multilang(LANG_WAN_PORT), multilang(LANG_RX_PKT), multilang(LANG_TX_PKT), multilang(LANG_STATUS));

	rtk_get_default_gw_wan_from_route_table(default_wan_itf);
	
	for(i=0; i<entry_num; i++)
	{
		
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
			continue;

		ifGetName(entry.ifIndex, wanif, sizeof(wanif));
		
		if(!strncmp(wanif,default_wan_itf,IFNAMSIZ))
		{
			snprintf(tmpBuf, sizeof(tmpBuf), "%s(default)", wanif);
		}
		else
		{
			snprintf(tmpBuf, sizeof(tmpBuf), "%s", wanif);
		}
		wan_phy_port = rtk_port_get_phyport_by_ifname(wanif);
		if(wan_phy_port==-1){
			printf("%s %d find %s phy port fail\n",__FUNCTION__,__LINE__,wanif);
			continue;
		}

		if(get_speed_by_portNum(wan_phy_port, &down_speed, &up_speed))
		{
			printf("%s %d get speed fail\n",__FUNCTION__,__LINE__);
		}

		#if 0
		for(j=0; j< MAX_PORT_NUM; j++)
		{
			if(wan_phy_port == ps[j].portNum)
			{
				up_speed = ps[j].txRate;
				down_speed = ps[j].rxRate;
				break;
			}
		}
		#endif

		get_net_device_stats(wanif, &nds);

		if(rtk_check_wan_is_connected(&entry))
			snprintf(connect_status, sizeof(connect_status), "connected");
		else
			snprintf(connect_status, sizeof(connect_status), "disconnected");

		nBytesSent += boaWrite(wp, "<tr>"
		"<td align=center width=\"14%%\">%s</td>\n"
		"<td align=center width=\"16%%\">%d</td>\n"
		"<td align=center width=\"16%%\">%.3f</td>\n"
		"<td align=center width=\"16%%\">%.3f</td>\n"
		"<td align=center width=\"14%%\">%lld</td>\n"
		"<td align=center width=\"14%%\">%lld</td>\n"
		"<td align=center width=\"10%%\">%s</td></tr>\n",
		tmpBuf,entry.logic_port,(float)down_speed*8/1000000, (float)up_speed*8/1000000, nds.rx_packets, nds.tx_packets, connect_status);
		
		total_rx_rate += (float)down_speed*8/1000000;
		total_tx_rate += (float)up_speed*8/1000000;
		total_rx_pkt += nds.rx_packets;
		total_tx_pkt += nds.tx_packets;

		if(i < MAX_TOTAL_LOGIC_PORT_NUM){
			logic_port_array[i] = entry.logic_port;
			total_num++;
		}
	}

	for(j=0; j<6; j++)
	{
		if(logic_port_array[j] >= 0){
			if(total_num==1)
			{
				snprintf(total_port,12,"%d",logic_port_array[j]);
				break;
			}
			else{
				if(j==total_num-1){
					snprintf(tmpbuf,6,"%d",logic_port_array[j]);
				}
				else{
					snprintf(tmpbuf,6,"%d/",logic_port_array[j]);
				}
				strncat(total_port,tmpbuf,sizeof(total_port));
			}
		}
	}
	
	nBytesSent += boaWrite(wp, "<tr>"
		"<td align=center width=\"14%%\">Total</td>\n"
		"<td align=center width=\"16%%\">%s</td>\n"
		"<td align=center width=\"16%%\">%.3f</td>\n"
		"<td align=center width=\"16%%\">%.3f</td>\n"
		"<td align=center width=\"14%%\">%lld</td>\n"
		"<td align=center width=\"14%%\">%lld</td>\n"
		"<td align=center width=\"10%%\">N/A</td></tr>\n",
		total_port,total_rx_rate, total_tx_rate, total_rx_pkt, total_tx_pkt);

	nBytesSent +=boaWrite(wp, "</table>\n"
	"</div>\n"
	"</div>\n");
	
	return nBytesSent;
}

int rtk_get_wan_rx_tx_rate_by_itfIndex(unsigned int wan_itfIndex, unsigned int *bigger_rate)
{
	char wan_name[IFNAMSIZ] = {0};
	lan_net_port_speed_T ps[MAX_PORT_NUM] = {0};
	int wan_phy_port = 0;
	int i = 0;
	unsigned int up_speed = 0;
	unsigned int down_speed = 0;

	if(bigger_rate == NULL){
		printf("%s %d pointer bigger_rate is NULL\n",__FUNCTION__,__LINE__);
		return -1;
	}
	
	ifGetName(wan_itfIndex, wan_name, sizeof(wan_name));

	if(strlen(wan_name) == 0){
		printf("%s %d get wan_name fail\n",__FUNCTION__,__LINE__);
		return -1;
	}

#if 0
	if(get_lanNetInfo_portSpeed(ps, MAX_PORT_NUM)==RTK_FAILED){
		printf("%s %d get wan phy port speed fail\n",__FUNCTION__,__LINE__);
		return -1;
	}
#endif

	wan_phy_port = rtk_port_get_phyport_by_ifname(wan_name);
	if(wan_phy_port==-1){
		printf("%s %d find %s phy port fail\n",__FUNCTION__,__LINE__,wan_name);
		return -1;
	}

	if(get_speed_by_portNum(wan_phy_port, &down_speed, &up_speed))
	{
		printf("%s %d get speed fail\n",__FUNCTION__,__LINE__);
	}

	#if 0
	for(i=0; i< MAX_PORT_NUM; i++)
	{
		if(wan_phy_port == ps[i].portNum)
		{
			up_speed = ps[i].txRate;
			down_speed = ps[i].rxRate;
			break;
		}
	}
	#endif

	if(up_speed > down_speed)
		*bigger_rate = up_speed;
	else
		*bigger_rate = down_speed;

	return 0;
}

unsigned int rtk_get_lan_dev_num_by_wan_itfname(unsigned int wan_itfIndex)
{
	MIB_STATIC_LOAD_BALANCE_T sta_entry;
	unsigned int lan_num_cnt = 0;
	int i= 0, j = 0, k = 0;
	int find_wan_flag = 0;
	char wanif[IFNAMSIZ] = {0};
	int sta_entry_num = 0, wan_idx = 0;
	lanHostInfo_t *pLanNetInfo = NULL;
	int ret_1 = 0, ret_2 = 0;
	con_wan_info_T *pConWandata = NULL;
	char global_balance_enable = 0, sta_enable = 0, dyn_enable = 0;
	char default_wan_itf[IFNAMSIZ] = {0};
	unsigned char lan_mac[18] = {0};
	unsigned int wan_mark = 0;
	unsigned int count = 0, wanCount = 0;
	MIB_CE_ATM_VC_T entry;
	char wan_name[IFNAMSIZ] = {0}, cmp_wan_name[IFNAMSIZ] = {0};
	
	ret_1 = get_lan_net_info(&pLanNetInfo, &count);
	ret_2 = get_con_wan_info(&pConWandata,&wanCount);

	if(ret_1 != 0)
		goto get_info_end;
	
	if((!mib_get_s(MIB_STATIC_LOAD_BALANCE_ENABLE, (void *)&sta_enable,sizeof(sta_enable))) ||
		(!mib_get_s(MIB_GLOBAL_LOAD_BALANCE, (void *)&global_balance_enable,sizeof(global_balance_enable))) ||
		(!mib_get_s(MIB_DYNAMIC_LOAD_BALANCE, (void *)&dyn_enable,sizeof(dyn_enable))))
	{
		goto get_info_end;
	}

	ifGetName(wan_itfIndex, wan_name, sizeof(wan_name));

	if(strlen(wan_name) == 0)
		goto get_info_end;

	rtk_get_default_gw_wan_from_route_table(default_wan_itf);
	//manual load balance 
	sta_entry_num = mib_chain_total(MIB_STATIC_LOAD_BALANCE_TBL);
	
	if(count>0){
		for(i=0; i<count; i++)
		{
			//printf("%s %d wan_idx:%s\n",__FUNCTION__,__LINE__, wan_itfIndex);
			find_wan_flag  = 0;
			memset(wanif,0,IFNAMSIZ);
			
			snprintf(lan_mac,sizeof(lan_mac),"%02x:%02x:%02x:%02x:%02x:%02x", pLanNetInfo[i].mac[0],pLanNetInfo[i].mac[1],pLanNetInfo[i].mac[2],pLanNetInfo[i].mac[3],pLanNetInfo[i].mac[4],pLanNetInfo[i].mac[5]);
			if(sta_enable != 0 && global_balance_enable != 0)
			{
				//check munual load balance entry which could has the same mac. 
				for(j=0;j<sta_entry_num;j++)
				{
					if(!mib_chain_get(MIB_STATIC_LOAD_BALANCE_TBL, j, &sta_entry))
						continue;

					if(!strncasecmp(lan_mac,sta_entry.mac,MAC_ADDR_STRING_LEN-1))
					{
						find_wan_flag  = 1;
						snprintf(cmp_wan_name,IFNAMSIZ,"%s",sta_entry.wan_itfname);
						break;
					}
				}
			}
			
			//find port mapping Lan pc info from port mapping when disable auto load balance or global load balance.
			if(find_wan_flag == 0)
			{
				if(rtk_check_port_mapping_rules_exist(pLanNetInfo[i].ifName, &wan_idx, &wan_mark))
				{
					if(mib_chain_get(MIB_ATM_VC_TBL, wan_idx, &entry)){
						ifGetName(entry.ifIndex, wanif, sizeof(wanif));
						snprintf(cmp_wan_name,IFNAMSIZ,"%s",wanif);
						find_wan_flag  = 1;
					}
				}
			}

			if(dyn_enable != 0 && global_balance_enable != 0)
			{
				//check auto load balance entry which could has the same mac. 
				if(wanCount > 0 && find_wan_flag == 0 && ret_2 == 0)
				{
					for(k=0;k<wanCount;k++)
					{
						if(pConWandata[k].wan_mark)
						{
							if(find_wan_flag == 1)
								break;
							
							if(rtk_get_wan_itf_by_mark(wanif,pConWandata[k].wan_mark)!=1)
							{
								//printf("this mark cant find wanitf name.\n");
								continue;
							}
							for(j=0; j<MAX_MAC_NUM; j++)
							{
								if(strlen(pConWandata[k].mac_info[j].mac_str))
								{
									if(!strncasecmp(lan_mac, pConWandata[k].mac_info[j].mac_str, MAC_ADDR_STRING_LEN-1))
									{
										find_wan_flag  = 1;
										snprintf(cmp_wan_name,IFNAMSIZ,"%s",wanif);
										//printf("%s %d wan:%s find lan %s in auto\n",__FUNCTION__,__LINE__, wan_name, pLanNetInfo[i].ifName);
										break;
									}
								}
							}
						}
					}
				}
			}

			//check wan name whether matching or not 
			if(!strncmp(cmp_wan_name, wan_name,IFNAMSIZ) && find_wan_flag==1)
				lan_num_cnt++;
			
			if(find_wan_flag == 0){
				//lan dev<-->default wan
				if(!strncmp(default_wan_itf, wan_name,IFNAMSIZ)){
					find_wan_flag  = 1;
					lan_num_cnt++;
					printf("%s %d wan:%s == default wan:%s\n",__FUNCTION__,__LINE__, wan_name, default_wan_itf);
				}
			}
		}	
	}

get_info_end:
	if(pLanNetInfo)
		free(pLanNetInfo);
		
	if(pConWandata)
		free(pConWandata);
	
	return lan_num_cnt;
	
}

#endif

/*ASP: return each wan's info's title to web*/
int rtk_load_balance_chart_header_info(int eid, request * wp, int argc, char **argv){
	int nBytesSent = 0;
	int wan_hdr_width, connect_hdr_width, rate_width, devices_width, onoff_width;
	char onoff_display[32] = {0};
	
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN_DEMO
	wan_hdr_width = 40;
	connect_hdr_width = 155;
	rate_width = 320;
	devices_width = 105;
	onoff_width = 75;
	snprintf(onoff_display, sizeof(onoff_display), "%s", "display:table-cell;");
#else
	wan_hdr_width = 60;
	connect_hdr_width = 165;
	rate_width = 320;
	devices_width = 135;
	onoff_width = 5;
	snprintf(onoff_display, sizeof(onoff_display), "%s", "display:none;");
#endif
	nBytesSent += boaWrite(wp, "<tr>\n");
	nBytesSent += boaWrite(wp, "<th style=\"width:%dpx; text-align:center;\"> WAN </th>\n", 
								wan_hdr_width);
	nBytesSent += boaWrite(wp, "<th style=\"width:%dpx; text-align:center;\"> Status </th>\n", 
								connect_hdr_width);
	nBytesSent += boaWrite(wp, "<th style=\"width:%dpx; padding-left:20px;\"> Rate </th>\n", 
								rate_width);
	nBytesSent += boaWrite(wp, "<th style=\"width:%dpx; text-align:center;\"> Devices </th>\n", 
								devices_width);
	nBytesSent += boaWrite(wp, "<th style=\"width:%dpx; text-align:center; %s\"> On/Off </th>\n", 
								onoff_width, onoff_display);
	nBytesSent += boaWrite(wp, "</tr>\n");

	return nBytesSent;
}

/*ASP: return actual wan nums to web*/
int rtk_load_balance_wan_nums(int eid, request * wp, int argc, char **argv){
	int nBytesSent = 0;
	int wan_nums = 0;
	
	/*ToDo: get current wan nums*/
	wan_nums = mib_chain_total(MIB_ATM_VC_TBL);
	
	nBytesSent += boaWrite(wp, "%d", wan_nums);
	
	return nBytesSent;
}

/*ASP: return each wan's init configurations to web*/
#define PROCESS_SCALE_FACTOR 0.7
int rtk_load_balance_actual_wans(int eid, request * wp, int argc, char **argv){
	int nBytesSent = 0,i = 0, wan_nums = 0, web_wan_idx = 0;
	char text[32]={0};
	char img[32] = {0};
	int connect_status = 0, any_wan_connect=0;
	int speed = 0, wans_total_speed = 0;//all wans max rate sum
	float rate = 0, total_rate = 0, real_wan_rate = 0;
	int ratio = 0, total_ratio = 0;//(80 means 80%)
	int clients_num = 0, total_clients_num = 0;
	int onoff_status = 0;
	struct net_link_info netlink_info;
	MIB_CE_ATM_VC_T entry;
	unsigned int wan_rate = 0;
	
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN_DEMO
	char *onoff_display = "display:table-cell;";
#else
	char *onoff_display = "display:none;";
#endif

	wan_nums = mib_chain_total(MIB_ATM_VC_TBL);	
	
	for (i=0; i < wan_nums; i++){
		/*ToDo: get wan's max rate*/
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
			continue;

		if(rtk_port_get_lan_link_info(entry.logic_port, &netlink_info))
		{
			printf("%s %d get phy port link rate fail\n",__FUNCTION__,__LINE__);
		}

		if(netlink_info.speed)
			speed = netlink_info.speed;
		else{
			printf("%s %d get wan port phy max rate fail\n",__FUNCTION__,__LINE__);
			speed = 0;
		}
		
		wans_total_speed += speed;
	}
	
	for (i=0; i < wan_nums; i++){
		web_wan_idx = i+1;

		/*WAN port*/
		nBytesSent += boaWrite(wp, "<tr>\n");
		nBytesSent += boaWrite(wp, "<td style=\"font-weight:bold;\"> WAN%d </td>\n", 
									web_wan_idx);
		
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
			continue;
		
		/*WAN connected status*/
		/*ToDo: get connect_status*/
		if(rtk_check_wan_is_connected(&entry))
			connect_status = 1;
		else
			connect_status = 0;
		
		any_wan_connect |= connect_status;
		if (connect_status == 1){//connected
			snprintf(text, sizeof(text), "%s", "Connected");
			snprintf(img, sizeof(img), "%s", "wan_connected");
		}else{//disconnected
			snprintf(text, sizeof(text), "%s", "Disconnected");
			snprintf(img, sizeof(img), "%s", "wan_disconnected");
		}
		nBytesSent += boaWrite(wp, "<td>\n");
		nBytesSent += boaWrite(wp, "<div align=\"center\">\n");
		nBytesSent += boaWrite(wp, "<ul style=\"width:100%%\">\n");
		nBytesSent += boaWrite(wp, "<li>\n");
		nBytesSent += boaWrite(wp, "<img id=\"wan%d_state_img\" "
								   "  src=\"./graphics/icon/%s.png\" height=\"64\" width=\"64\" align=\"center\"/>\n",
								   web_wan_idx, img);
		nBytesSent += boaWrite(wp, "<p><span id=\"wan%d_state_text\" "
								   "  style=\"font-weight:bold; color:#707070;\">%s</span></p>\n", 
								   web_wan_idx, text);
		nBytesSent += boaWrite(wp, "</li>\n");
		nBytesSent += boaWrite(wp, "</ul>\n");
		nBytesSent += boaWrite(wp, "</div>\n");
		nBytesSent += boaWrite(wp, "</td>\n");

		/*WAN rate*/ 
		/*ToDo: get rate value (rate)*/
		if(rtk_get_wan_rx_tx_rate_by_itfIndex(entry.ifIndex, &wan_rate))
		{
			printf("%s %d get wan rate fail\n",__FUNCTION__,__LINE__);
		}
		rate = (float)wan_rate*8/1000000;
		if(!rtk_check_wan_is_connected(&entry))
			rate = 0;
		
		ratio = (rate*100/wans_total_speed)*PROCESS_SCALE_FACTOR;
		total_rate += rate;
		
		nBytesSent += boaWrite(wp, "<td align=\"center\" > \n");
		nBytesSent += boaWrite(wp, "<ul>\n");
		nBytesSent += boaWrite(wp, "<li>\n");	
		nBytesSent += boaWrite(wp, "<div class=\"progress_container_1\" "
								   "id=\"wan%d_rate_img_p\" style=\"overflow: hidden;\">\n",
									web_wan_idx);
		
		nBytesSent += boaWrite(wp, "<div class=\"progress\" id=\"wan%d_rate_img\" "
									"style=\"width:%d%%; color:black; float:left;\"></div>\n",
									web_wan_idx, ratio);
			
		nBytesSent += boaWrite(wp, "<span id=\"wan%d_rate_img_span\" style=\"float:left;\">%.1fMbps</span>\n",
									web_wan_idx, rate);
		nBytesSent += boaWrite(wp, "</div>\n");
		nBytesSent += boaWrite(wp, "</li>\n");
		nBytesSent += boaWrite(wp, "</ul>\n");
		nBytesSent += boaWrite(wp, "</td>\n");

		/*WAN client num*/
		/*ToDo: get clients num (clients_num)*/
#if defined(CONFIG_USER_LANNETINFO) && defined(CONFIG_USER_REFINE_CHECKALIVE_BY_ARP)
		clients_num  = rtk_get_lan_dev_num_by_wan_itfname(entry.ifIndex);
#endif
		
		total_clients_num += clients_num;
		nBytesSent += boaWrite(wp, "<td> <div align=\"center\" >\n");
		nBytesSent += boaWrite(wp, "<ul style=\"width:100%%\"> \n");
		nBytesSent += boaWrite(wp, "<li>\n");

		nBytesSent += boaWrite(wp, "<img src=\"./graphics/icon/laptop.png\" height=\"40\" width=\"64\" align=\"center\"/> \n");
		nBytesSent += boaWrite(wp, "<p> <span id=\"wan%d_dev_nums_text\" "
									"style=\"font-weight:bold; font-size:14px; color:#707070;\"> x %d </span> </p>\n",
									web_wan_idx, clients_num);
		nBytesSent += boaWrite(wp, "</li>\n");
		nBytesSent += boaWrite(wp, "</ul>\n");
		nBytesSent += boaWrite(wp, "</div>\n");
		nBytesSent += boaWrite(wp, "</td>\n");

		/*WAN on-off status*/
		onoff_status = entry.power_on;
		if (onoff_status == 1){//on status
			snprintf(text, sizeof(text), "%s", "On");
			snprintf(img, sizeof(img), "%s", "wan_on");
		}else{//off status
			snprintf(text, sizeof(text), "%s", "Off");
			snprintf(img, sizeof(img), "%s", "wan_off");
		}

		nBytesSent += boaWrite(wp, "<td style=\"%s\"><div align=\"center\">\n",
									onoff_display);
		nBytesSent += boaWrite(wp, "<ul style=\"width:100%%\">\n");
		nBytesSent += boaWrite(wp, "<li>\n");
		nBytesSent += boaWrite(wp, "<img id=\"wan%d_onoff_img\" src=\"./graphics/icon/%s.png\" height=\"32\" width=\"64\" "
								   "align=\"center\" onclick=\"changeOnOffStateEvent(this, 'wan%d')\"/>\n",
										web_wan_idx, img, web_wan_idx);
		nBytesSent += boaWrite(wp, "<p><span id=\"wan%d_onoff_text\" style=\"font-weight:bold; color:#707070;\">%s"
									"</span></p>\n",
									web_wan_idx, text);
									
		nBytesSent += boaWrite(wp, "</li>\n");
		nBytesSent += boaWrite(wp, "</ul>\n");
		nBytesSent += boaWrite(wp, "</div>\n");
		nBytesSent += boaWrite(wp, "</td>\n");
		nBytesSent += boaWrite(wp, "</tr>\n");
	}

	/*Total Wan*/
	nBytesSent += boaWrite(wp, "<tr>\n");
	nBytesSent += boaWrite(wp, "<td style=\"font-weight:bold;\"> TOTAL </td>\n");

	/*WAN connected status*/
	if (any_wan_connect == 1){//connected
		snprintf(text, sizeof(text), "%s", "Connected");
		snprintf(img, sizeof(img), "%s", "total_connected");
	}else{//disconnected
		snprintf(text, sizeof(text), "%s", "Disconnected");
		snprintf(img, sizeof(img), "%s", "total_disconnected");
	}
		
	nBytesSent += boaWrite(wp, "<td><div align=\"center\"> \n");
	nBytesSent += boaWrite(wp, "<ul style=\"width:100%%\">\n");
	nBytesSent += boaWrite(wp, "<li>\n");
	nBytesSent += boaWrite(wp, "<img id=\"total_state_img\" src=\"./graphics/icon/%s.png\" "
								"height=\"64\" width=\"64\" align=\"center\"/>\n",
								img);
	nBytesSent += boaWrite(wp, "<p><span id=\"total_state_text\" "
								"style=\"font-weight:bold; color:#707070;\">%s</span></p>\n",
								text);
	nBytesSent += boaWrite(wp, "</li>\n");
	nBytesSent += boaWrite(wp, "</ul> \n");
	nBytesSent += boaWrite(wp, "</div></td>\n");


	/*rate and radio*/
	total_ratio = (total_rate*100/wans_total_speed)*PROCESS_SCALE_FACTOR;
	nBytesSent += boaWrite(wp, "<td align=\"center\" >\n");
	nBytesSent += boaWrite(wp, "<ul><li> \n");
	nBytesSent += boaWrite(wp, "<div class=\"progress_container_1 progress_container_2\" "
								"id=\"total_rate_img_p\" style=\"overflow: hidden;\"> \n");
	nBytesSent += boaWrite(wp, "<div class=\"progress\" id=\"total_rate_img\" style=\"width:%d%%; color:black; float:left;\"> </div>\n",
								total_ratio);
	nBytesSent += boaWrite(wp, "<span id=\"total_rate_img_span\" style=\"float:left;\">%.1fMbps</span>\n",
								total_rate);
	nBytesSent += boaWrite(wp, "</div >\n");
	nBytesSent += boaWrite(wp, "</li></ul>\n");
	nBytesSent += boaWrite(wp, "</td>\n");

	/*Client nums*/
	nBytesSent += boaWrite(wp, "<td> <div align=\"center\">\n");
	nBytesSent += boaWrite(wp, "<ul style=\"width:100%%\"><li> \n");
	nBytesSent += boaWrite(wp, "<img src=\"./graphics/icon/laptop.png\" height=\"40\" width=\"64\" align=\"center\"/> \n");
	nBytesSent += boaWrite(wp, "<p> <span id=\"total_dev_nums_text\" "
								"style=\"font-weight:bold; font-size:14px; color:#707070;\"> x %d </span> </p>\n",
								total_clients_num);
	nBytesSent += boaWrite(wp, "</li></ul> \n");
	nBytesSent += boaWrite(wp, "</div></td>\n");

	/*no On/Off*/
	nBytesSent += boaWrite(wp, "<td style=\"%s\"> <p> - </p> </td>\n",
								onoff_display);
	
	nBytesSent += boaWrite(wp, "</tr>\n");	
}

/*ASP: return each wan's realtime configurations to web by Ajax request*/
int rtk_load_balance_wan_info(int eid, request * wp, int argc, char **argv){
	int nBytesSent = 0;	
	int i;
	int speed = 0, wans_total_speed =0;
	int percentage = 0;
	unsigned int clients_num = 0;
	MIB_CE_ATM_VC_T entry;
	int entry_num = 0;
	unsigned int wan_rate = 0;
	float realtime_rate = 0.0; 
	char connect_status[16] = {0};
	unsigned int wan_max_rate = 0;
	struct net_link_info netlink_info;
	
	entry_num = mib_chain_total(MIB_ATM_VC_TBL);

	for (i=0; i < entry_num; i++){
		/*ToDo: get wan's max rate*/
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
			continue;

		if(rtk_port_get_lan_link_info(entry.logic_port, &netlink_info))
		{
			printf("%s %d get phy port link rate fail\n",__FUNCTION__,__LINE__);
		}
		if(netlink_info.speed)
			speed = netlink_info.speed;
		else{
			printf("%s %d get wan port phy max rate fail\n",__FUNCTION__,__LINE__);
			speed = 0;
		}	
		wans_total_speed += speed;
	}

	for (i = 0; i < entry_num; i++){
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
			continue;
		
		//get wan rate
#if defined(CONFIG_USER_LANNETINFO) && defined(CONFIG_USER_REFINE_CHECKALIVE_BY_ARP)
		if(rtk_get_wan_rx_tx_rate_by_itfIndex(entry.ifIndex, &wan_rate))
		{
			printf("[%s %d] get wan port rate fail\n",__FUNCTION__,__LINE__);
			continue;
		}
#endif

		if(rtk_check_wan_is_connected(&entry))
			snprintf(connect_status, sizeof(connect_status), "connected");
		else
			snprintf(connect_status, sizeof(connect_status), "disconnected");

		if(!rtk_check_wan_is_connected(&entry))
			wan_rate = 0;
		
		realtime_rate = (float)wan_rate*8/1000000;

		if(wans_total_speed){
			percentage = (realtime_rate*100)/wans_total_speed*PROCESS_SCALE_FACTOR;
		}
		else
			percentage = 0;

#if defined(CONFIG_USER_LANNETINFO) && defined(CONFIG_USER_REFINE_CHECKALIVE_BY_ARP)
		clients_num  = rtk_get_lan_dev_num_by_wan_itfname(entry.ifIndex);
#endif

		//printf("%s %d wan:%d state:%s rate:%f percentage:%d dev_nums:%d entry_num:%d\n",__FUNCTION__,__LINE__,i+1, connect_status, rate, percentage, clients_num, entry_num);
		
		nBytesSent += boaWrite(wp, "wan=wan%d|",i+1);
		nBytesSent += boaWrite(wp, "state=%s|",connect_status);
		nBytesSent += boaWrite(wp, "rate=%.1f|",realtime_rate);
		nBytesSent += boaWrite(wp, "percentage=%d%%|",percentage);
		nBytesSent += boaWrite(wp, "dev_nums=%d|",clients_num);

		if(entry.power_on == 1)
		{
			nBytesSent += boaWrite(wp, "on/off=on,");
		}
		else
		{
			nBytesSent += boaWrite(wp, "on/off=off,");
		}
	}
	return nBytesSent;	
}

void formLdStatsChart_demo(request * wp, char *path, char *query){
	char *submitUrl;
	char *onoff_event_val;
	int wan_idx = -1;
	char onoff_val[8] = {0};
	MIB_CE_ATM_VC_T wan_entry;
	
//	printf("[%s:%d] formLdStatsChart_demo\n",__FUNCTION__,__LINE__);
	
	onoff_event_val = boaGetVar(wp, "onoff_change", "");
//	printf("[%s:%d] wan_onoff_val=%s\n",__FUNCTION__,__LINE__,onoff_event_val);
	if (onoff_event_val[0]){
		sscanf(onoff_event_val, "wan%d_%s", &wan_idx, onoff_val);
		printf("[%s:%d] wan_idx=%d, onoff_val=%s\n",__FUNCTION__,__LINE__,wan_idx, onoff_val);
		if (wan_idx > 0){

			if(!mib_chain_get(MIB_ATM_VC_TBL, wan_idx-1, &wan_entry))
				printf("[%s:%d]  get wan mib entry fail  \n",__FUNCTION__,__LINE__);
			
			if (strcmp(onoff_val, "on") == 0){
				
				wan_entry.power_on = 1;
				setup_wan_port_power_on_off(rtk_port_get_phyPort(wan_entry.logic_port), 1); 

			}else if(strcmp(onoff_val, "off") == 0){
				
				wan_entry.power_on = 0;
				setup_wan_port_power_on_off(rtk_port_get_phyPort(wan_entry.logic_port), 0); 
				
			}

			mib_chain_update(MIB_ATM_VC_TBL, &wan_entry, wan_idx-1);
		}
	}

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
	
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;
}

#endif

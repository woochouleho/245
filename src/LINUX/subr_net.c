#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/msg.h>
#include <net/route.h>
#include <string.h>
#include "mib.h"
#include "utility.h"
#include "subr_net.h"
#include "subr_switch.h"
#include "utility.h"
#include "form_src/multilang.h"
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#include "fc_api.h"
#endif
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_CTC_E8_CLIENT_LIMIT)
#include "rt_misc_ext.h"
#endif
#include "debug.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#ifdef CONFIG_RTK_OMCI_V1
#include <omci_dm_sd.h>
#endif
#if defined(CONFIG_EPON_FEATURE)
#if defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_epon.h>
#else
#include <rtk/epon.h>
#endif
#endif

#include "sockmark_define.h"
#include <unistd.h>

#ifdef CONFIG_USER_LANNETINFO
#include <sys/syscall.h>
#endif

#ifdef CONFIG_HWNAT
#include "hwnat_ioctl.h"
#endif

#ifdef CONFIG_PPPOE_MONITOR
#include "../msgq.h"
#include <sys/syscall.h>
#endif

#define DNS_RELAY_SHORT_LEASETIME 1*60       /*1min */

#if defined(CONFIG_CTC_SDN)
#include "subr_ctcsdn.h"
#endif
#include <netinet/if_ether.h>
#include <net/if_arp.h>

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
const char FW_IN_COMMING[] = "in_comming_filter";
#endif
#if defined(CONFIG_BOLOCK_IPTV_FROM_LAN_SERVER)
const char LAN_TO_LAN_MULTICAST_BLOCK[] = "lan_to_lan_multicast_block";
const char LAN_QUERY_BLOCK[] = "lan_query_block";
#endif
const char IPOA_IPINFO[] = "/tmp/IPoAHalfBridge";
#ifdef CONFIG_USER_PPPD
const char PPPD[] = "/bin/pppd";
const char PPPOE_PLUGIN[] = "/lib/pppd/plugins/rp-pppoe.so";
const char PPPOA_PLUGIN[] = "/lib/pppd/plugins/pppoatm.so";
#ifdef CONFIG_USER_PPTP_CLIENT
const char PPTP_PLUGIN[] = "/lib/pppd/plugins/pptp.so";
#endif // CONFIG_USER_PPTP_CLIENT
#ifdef CONFIG_USER_XL2TPD
const char XL2TPD[] = "/bin/xl2tpd";
const char XL2TPD_PID[] = "/var/run/xl2tpd.pid";
const char XL2TP_PLUGIN[] = "/lib/pppd/plugins/pppol2tp.so";
#endif
#ifdef CONFIG_USER_PPTP_SERVER
const char PPTPD[] = "/bin/pptpd";
const char PPTPD_PID[] = "/var/run/pptpd.pid";
#endif
#endif // CONFIG_USER_PPPD
#ifdef CONFIG_VIR_BOA_JEMBENCHTEST
extern void rtk_cmcc_do_jspeedup_ex();
#endif

extern const char LANIF_ALL[];

static void deo_filter_set_acl_service(MIB_CE_ACL_IP_T Entry, unsigned char Interface, int enable, char *strIP, int isIPrange);

/**
 * Name: void *read_data_from_file(int fd, unsigned char *file, unsigned int *size)
 * func: Read data from file to buf
 * param[in]
 *		fd (int) The file descriptor
 * param[in]
 *		file (unsigned char *) filen name
 * param[out]
 *		size (unsigned int *) The size of the data actually read from the file
 * return
 *		(void *) Pointer to the data,The user must free the buf
 **/
void *read_data_from_file(int fd, unsigned char *file, unsigned int *size)
{
	struct stat status;
	int read_size, read_count;
	void *dataBuf = NULL;

	if(fd < 0 || file == NULL)
	{
		return NULL;
	}

	if ( stat(file, &status) < 0 )
	{
		return NULL;
	}
	read_size = (unsigned long)(status.st_size);

	dataBuf = (void *) malloc(read_size);
	if(dataBuf == NULL)
	{
		return NULL;
	}
	memset(dataBuf, 0, read_size);

	read_count = read(fd, (void*)dataBuf, read_size);
	if (read_count != read_size)
	{
		printf("(%s:%d),Read file data failed!\n",__FUNCTION__, __LINE__);
	}

	*size = read_count;

	return dataBuf;
}

#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
#include <rtk/l2.h>
#include <sys/sysinfo.h>

con_wan_info_T con_wan[MAX_CON_WAN_NUM] = {0};
con_wan_info_T port_map_info[MAX_CON_WAN_NUM] = {0};
save_sta_info_T save_sta_info[MAX_SAVE_STA_NUM] = {0};

void swap(int *x, int *y)
{
	if(x == NULL || y == NULL)
		return;

	int t = *x;
	*x = *y;
	*y = t;

	return;
}

int rtk_calc_max_divisor(int a,int b)
{
	int tmp_a = a;
	int tmp_b = b;
	int tmp2 ,result;

	if(a <= 0 || b <= 0 )
		return 0;

	if (a < b)
	{
		swap(&tmp_a, &tmp_b);
	}

	while( tmp_b > 0)
	{
		tmp2 = tmp_a % tmp_b;
		tmp_a = tmp_b;
		tmp_b = tmp2;
	}

	result = tmp_a;

	return result;
}


int rtk_calc_max_divisor_of_multi_number(int number[],int len)
{
	int i = 0;
	int result = number[0];
	if(result <= 0)
	{
		printf("------%s---%s(%d),	trace: ERROR	bandwidth is a error number	  result = %d  -------\n",__FILE__,__FUNCTION__,__LINE__ ,result);
		return -1;
	}
	if(len > 1)
	{
		for(i = 1;i < len;i++){
			result = rtk_calc_max_divisor(result,number[i]);
			if(result <= 0)
			{
				printf("------%s---%s(%d),	trace: ERROR    bandwidth is a error number	  result = %d -------\n",__FILE__,__FUNCTION__,__LINE__ ,result);
				return -1;
			}
		}
	}

	return result;
}

int rtk_calc_wan_weight_by_real_bandwidth(int wan_idx, int wan_bandwidth)
{
	int32_t i ,totalEntry;
	MIB_CE_ATM_VC_T temp_Entry,wan_Entry;
	int bandwidth[MAX_CON_WAN_NUM] = {0};
	int bandwidth_unit , divisor_result = 0;

	if (!mib_chain_get(MIB_ATM_VC_TBL, wan_idx, (void *)&wan_Entry))
		return 0;

	if(!mib_get_s(MIB_BASIC_BANDWIDTH_UNIT, (void *)&bandwidth_unit,sizeof(bandwidth_unit)))
		return 0;

	wan_Entry.bandwidth = wan_bandwidth;
	mib_chain_update(MIB_ATM_VC_TBL, (void*)&wan_Entry, wan_idx);


	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	if(totalEntry <= 0)
	{
		printf("------%s---%s(%d),	trace: ERROR    NO WAN exist	-------\n",__FILE__,__FUNCTION__,__LINE__);
		return -1;
	}

	for(i = 0; i < totalEntry; ++i)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&temp_Entry))
			continue;

		if (temp_Entry.enable == 0)
			continue;

		bandwidth[i] = temp_Entry.bandwidth;
	}

	for(i = 0; i < totalEntry; ++i)
	{
		if(bandwidth_unit && (bandwidth[i]/bandwidth_unit) >= 1)
			bandwidth[i] = bandwidth[i]/bandwidth_unit;
		else
			bandwidth[i] = 1;
	}

	//to calc max divisor for get each wan weight
	if(totalEntry >= 1)
		divisor_result = rtk_calc_max_divisor_of_multi_number(bandwidth, totalEntry);

	if(divisor_result <= 0)
	{
		printf("------%s---%s(%d),	trace: ERROR    bandwidth divisor calc fail	-------\n",__FILE__,__FUNCTION__,__LINE__);
		return -1;
	}
	else
	{
		for(i = 0; i < totalEntry; ++i)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&temp_Entry))
				continue;

			if (temp_Entry.enable == 0)
				continue;

			temp_Entry.weight = bandwidth[i]/divisor_result;

			mib_chain_update(MIB_ATM_VC_TBL, (void*)&temp_Entry, i);
		}
	}

	return 1;
}

int rtk_get_dynamic_load_balance_enable()
{
	unsigned char dynamic_enable = 0, global_balance_enable = 0;

	if(!mib_get_s(MIB_GLOBAL_LOAD_BALANCE, (void *)&global_balance_enable,sizeof(global_balance_enable)))
	{
		printf("get global load balance enable  failed!\n");
	}

	if(!mib_get_s(MIB_DYNAMIC_LOAD_BALANCE, (void *)&dynamic_enable,sizeof(dynamic_enable)))
	{
		printf("get dynamic load balance enable failed!\n");
	}

	if(dynamic_enable && global_balance_enable)
		dynamic_enable = 1;
	else
		dynamic_enable = 0;

	return dynamic_enable;
}

int isZeroMacstr(unsigned char *Mac_str)
{
	unsigned char mac[6];
	int idx, ii = 0;

	if (Mac_str == NULL || (*Mac_str == '\0'))
		return 1;

	do {
		if(*Mac_str != '0' && *Mac_str != ':')
			return 0;
		Mac_str++;
	}while(*Mac_str != '\0');

	return 1;
}

int rtk_get_wan_mark_by_wanname(unsigned char *wan_name, unsigned int *wan_mark)
{
	int32_t i ,totalEntry, wan_mask = 0;
	MIB_CE_ATM_VC_T temp_Entry;
	unsigned char wan_ifname[IFNAMSIZ], ppp_ifname[IFNAMSIZ+1] = {0};

	if(wan_name == NULL || wan_mark == NULL)
		return 0;

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&temp_Entry))
			continue;

		if (temp_Entry.enable == 0)
			continue;

		ifGetName(temp_Entry.ifIndex, wan_ifname, sizeof(wan_ifname));

		if(temp_Entry.cmode == CHANNEL_MODE_PPPOE)
			sprintf(ppp_ifname, "%s%d", ALIASNAME_PPP, PPP_INDEX(temp_Entry.ifIndex));

		if((strncmp(wan_name, wan_ifname, IFNAMSIZ) == 0) || (strncmp(wan_name, ppp_ifname, IFNAMSIZ) == 0))
			break;
	}

	if (i >= totalEntry)
		return 0;
	else
	{
		rtk_net_get_wan_policy_route_mark(wan_name, wan_mark, &wan_mask);
		return 1;
	}
}

int rtk_get_wan_entry_by_wanmark(MIB_CE_ATM_VC_T *pEntry, unsigned int wan_mark)
{
	int32_t i , totalEntry = 0;
	unsigned int wan_mark_tmp, wan_mask = 0;
	MIB_CE_ATM_VC_T temp_Entry;
	char ifName[IFNAMSIZ];

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&temp_Entry))
		{
			ifGetName(temp_Entry.ifIndex, ifName, sizeof(ifName));
			rtk_net_get_wan_policy_route_mark(ifName, &wan_mark_tmp, &wan_mask);
			if(wan_mark == wan_mark_tmp)
				break;
		}
	}

	if(i>= totalEntry)
		return 0;

	if(pEntry)
	{
		memcpy(pEntry, (void *)&temp_Entry, sizeof(MIB_CE_ATM_VC_T));
		return 1;
	}

	return 0;
}

int rtk_get_wan_index_by_wanmark(int *wan_idx, unsigned int wan_mark)
{
	int i , totalEntry = 0;
	unsigned int wan_mark_tmp, wan_mask = 0;
	MIB_CE_ATM_VC_T temp_Entry;
	char ifName[IFNAMSIZ];

	if(wan_idx == NULL)
		return 0;

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&temp_Entry))
		{
			ifGetName(temp_Entry.ifIndex, ifName, sizeof(ifName));
			rtk_net_get_wan_policy_route_mark(ifName, &wan_mark_tmp, &wan_mask);
			if(wan_mark == wan_mark_tmp)
			{
				*wan_idx = i;
				return 1;
			}
		}
	}

	return 0;
}

int rtk_get_wan_entry_by_wanvid(int *wan_entry_idx, MIB_CE_ATM_VC_T           *pEntry, unsigned int vid)
{
	int32_t i , totalEntry = 0;
	unsigned int wan_mark_tmp, wan_mask = 0;
	MIB_CE_ATM_VC_T temp_Entry;

	if(wan_entry_idx == NULL)
		return 0;

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&temp_Entry))
		{
			if (!temp_Entry.enable)
				continue;

			if(vid == temp_Entry.vid)
			{
				*wan_entry_idx = i;
				break;
			}
		}
	}

	if(i>= totalEntry)
		return 0;

	if(pEntry)
	{
		memcpy(pEntry, (void *)&temp_Entry, sizeof(MIB_CE_ATM_VC_T));
		return 1;
	}

	return 0;
}

void rtk_get_port_map_wan_mark_by_mac(unsigned char* mac, int *wan_info_idx, unsigned int *wan_mark)
{
	int i = 0, j = 0;

	if(mac == NULL || wan_info_idx == NULL || wan_mark == NULL)
		return;

	for(i = 0; i < MAX_CON_WAN_NUM; i++)
	{
		for(j = 0; j < MAX_MAC_NUM; j++)
		{
			if(memcmp(port_map_info[i].mac_info[j].mac_str, mac, MAC_ADDR_STRING_LEN) == 0)
			{
				*wan_mark = port_map_info[i].wan_mark;
				*wan_info_idx = i;
				return;
			}
		}
	}
	return;
}

void rtk_get_load_balance_wan_mark_by_mac(unsigned char* mac, int *wan_info_idx, unsigned int *wan_mark)
{
	int i = 0, j = 0;

	if(mac == NULL || wan_info_idx == NULL || wan_mark == NULL)
		return;

	for(i = 0; i < MAX_CON_WAN_NUM; i++)
	{
		for(j = 0; j < MAX_MAC_NUM; j++)
		{
			if(memcmp(con_wan[i].mac_info[j].mac_str, mac, MAC_ADDR_STRING_LEN) == 0)
			{
				*wan_mark = con_wan[i].wan_mark;
				*wan_info_idx = i;
				return;
			}
		}
	}
	return;
}

void rtk_get_load_balance_wan_mark_by_ip(char *ip, unsigned int *wan_mark)
{
	int i = 0, j = 0;

	if(ip == NULL || wan_mark == NULL)
		return;

	for(i = 0; i < MAX_CON_WAN_NUM; i++)
	{
		for(j = 0; j < MAX_MAC_NUM; j++)
		{
			if(con_wan[i].ip_info[j].ipAddr == ntohl(inet_addr(ip)))
			{
				*wan_mark = con_wan[i].wan_mark;
				return;
			}
		}
	}
	return;
}

void rtk_process_bridge_ext_rules(unsigned int wan_mark, unsigned char *mac, char *ip, int op)
{
	MIB_CE_ATM_VC_T wan_entry;
	char polmark[64] = {0};

	if(rtk_get_wan_entry_by_wanmark(&wan_entry, wan_mark) == 1)
	{

		sprintf(polmark, "0x%x/0x%x", wan_mark, SOCK_MARK_WAN_INDEX_BIT_MASK);

		if(wan_entry.cmode == CHANNEL_MODE_BRIDGE)
		{
			if(mac && *mac && isZeroMacstr(mac) == 0)
			{
				va_cmd(EBTABLES, 12, 1, (op==1)?"-A":"-D", (char *)DYNAMIC_INPUT_LOAD_BALANCE, "-s", mac, "-p", "ipv4", "--ip-proto", "udp", "--ip-dport", "67:68",  "-j", "DROP");
				va_cmd(EBTABLES, 9, 1, (op==1)?"-A":"-D", (char *)DYNAMIC_FORWARD_LOAD_BALANCE, "-d", mac, "!", "--mark", polmark , "-j", "DROP");
			}

			if(ip && *ip)
			{
				va_cmd(EBTABLES, 12, 1, (op==1)?"-A":"-D", (char *)DYNAMIC_INPUT_LOAD_BALANCE, "--ip-src", ip, "-p", "ipv4", "--ip-proto", "udp", "--ip-dport", "67:68",  "-j", "DROP");
				va_cmd(EBTABLES, 11, 1, (op==1)?"-A":"-D", (char *)DYNAMIC_FORWARD_LOAD_BALANCE, "--ip-dst", ip, "-p", "ipv4", "!", "--mark", polmark , "-j", "DROP");
			}
		}
	}

	return;
}

void rtk_update_save_sta_info(unsigned char *mac, unsigned int wan_mark)
{
	int i, j = 0, sta_idx = -1 ,sta_cur_cnt = 0, tmp_idx = 0;
	struct sysinfo sys_info;

	if(mac == NULL)
		return;

	sysinfo(&sys_info);  //to get system time for update the time of lan connect to the wan

	for(i = 0; i < MAX_SAVE_STA_NUM; i++)
	{
		if(memcmp(save_sta_info[i].mac_str, mac, MAC_ADDR_STRING_LEN) == 0)
		{
			sta_idx = i;
			sta_cur_cnt = save_sta_info[sta_idx].save_cnt;
		}
	}

	if(sta_cur_cnt == 0)
	{
		//find empty entry to record the sta info
		for(j = 0; j < MAX_SAVE_STA_NUM; j++)
		{
			if(isZeroMacstr(save_sta_info[j].mac_str))
			{
				sta_idx = j;
				memcpy(save_sta_info[j].mac_str, mac, MAC_ADDR_STRING_LEN);
				break;
			}
		}
	}
	else if(sta_cur_cnt == SAVE_INFO_NUM)
	{
		if(save_sta_info[sta_idx].save_wan_info[sta_cur_cnt-1].save_wan_mark == wan_mark)
		{
			save_sta_info[sta_idx].save_wan_info[sta_cur_cnt-1].save_wan_time = sys_info.uptime;
			return;
		}

		for(j = 0; j < SAVE_INFO_NUM-1; j++)
		{
			memcpy(&(save_sta_info[sta_idx].save_wan_info[j]), &(save_sta_info[sta_idx].save_wan_info[j+1]),sizeof(save_wan_info_T));
		}
		memset(&(save_sta_info[sta_idx].save_wan_info[SAVE_INFO_NUM-1]), 0, sizeof(save_wan_info_T));
		save_sta_info[sta_idx].save_cnt--;
		sta_cur_cnt--;
	}
	else if(sta_cur_cnt > SAVE_INFO_NUM)
		printf("------%s---%s(%d),	trace: XXXXXXX	 ERROR  sta_cur_cnt = %d  -------\n",__FILE__,__FUNCTION__,__LINE__,sta_cur_cnt);

	if(sta_cur_cnt > 0 && save_sta_info[sta_idx].save_wan_info[sta_cur_cnt-1].save_wan_mark == wan_mark)
	{
		save_sta_info[sta_idx].save_wan_info[sta_cur_cnt-1].save_wan_time = sys_info.uptime;
		return;
	}
	else
	{
		save_sta_info[sta_idx].save_wan_info[sta_cur_cnt].save_wan_mark = wan_mark;
		save_sta_info[sta_idx].save_wan_info[sta_cur_cnt].save_wan_time = sys_info.uptime;
		save_sta_info[sta_idx].save_cnt++;
	}

	return;
}

//op  0:add; 1:delete; 2:flush
void rtk_update_port_map_info(unsigned char *ifname, int wan_info_idx, unsigned char* mac, char *ip, unsigned int wan_mark, int op, int type)
{
	int i = 0 , k = 0;
	con_wan_info_T *tmpinfo = NULL;
	MIB_CE_ATM_VC_T wan_entry;
	unsigned char wanname[IFNAMSIZ] = {0};

	if(wan_info_idx == -1)
	{
		//printf("------%s---%s(%d),  ERROR: wan_info_idx is error !!!!!!! -------\n",__FILE__,__FUNCTION__,__LINE__);
		return;
	}

	rtk_get_wan_entry_by_wanmark(&wan_entry,wan_mark);
	ifGetName(wan_entry.ifIndex, wanname, sizeof(wanname));
	memcpy(port_map_info[wan_info_idx].wan_name, wanname, IFNAMSIZ);

	if(op == 0)
	{
		port_map_info[wan_info_idx].wan_mark = wan_mark;
		if(type == MAC_TYPE)
		{
			for(k = 0; k < MAX_MAC_NUM; k++)
			{
				if(isZeroMacstr(port_map_info[wan_info_idx].mac_info[k].mac_str) && ifname && mac)
				{
					memcpy(port_map_info[wan_info_idx].mac_info[k].mac_str, mac, MAC_ADDR_STRING_LEN);
					memcpy(port_map_info[wan_info_idx].mac_info[k].ifname, ifname, IFNAMSIZ);
					port_map_info[wan_info_idx].conn_cnt++;
					break;
				}
			}
		}
		if(type == IP_TYPE)
		{
			for(k = 0; k < MAX_IP_NUM; k++)
			{
				if(port_map_info[wan_info_idx].ip_info[k].ipAddr == 0 && ifname && ip)
				{
					port_map_info[wan_info_idx].ip_info[k].ipAddr = ntohl(inet_addr(ip));
					memcpy(port_map_info[wan_info_idx].ip_info[k].ifname, ifname, IFNAMSIZ);
					port_map_info[wan_info_idx].conn_cnt++;
					break;
				}
			}
		}
	}
	else if(op == 1)
	{
		if(type == MAC_TYPE)
		{
			for(k = 0; k < MAX_MAC_NUM; k++)
			{
				if(mac && memcmp(port_map_info[wan_info_idx].mac_info[k].mac_str, mac, MAC_ADDR_STRING_LEN) == 0)
				{
					memset(port_map_info[wan_info_idx].mac_info[k].mac_str, 0, MAC_ADDR_STRING_LEN);
					memset(port_map_info[wan_info_idx].mac_info[k].ifname, 0, IFNAMSIZ);
					port_map_info[wan_info_idx].conn_cnt--;
					break;
				}
			}
		}
		if(type == IP_TYPE)
		{
			for(k = 0; k < MAX_IP_NUM; k++)
			{
				if(ip && port_map_info[wan_info_idx].ip_info[k].ipAddr == ntohl(inet_addr(ip)))
				{
					port_map_info[wan_info_idx].ip_info[k].ipAddr = 0;
					memset(port_map_info[wan_info_idx].ip_info[k].ifname, 0, IFNAMSIZ);
					port_map_info[wan_info_idx].conn_cnt--;
					break;
				}
			}
		}

		if(port_map_info[wan_info_idx].conn_cnt == 0)
		{
			memset(port_map_info[wan_info_idx].wan_name, 0, IFNAMSIZ);
			port_map_info[wan_info_idx].wan_mark = 0;
		}

	}
	else if(op == 2)
	{
		//when wan be delete or wan disconnect, clear wan's all connect info
		memset(&(port_map_info[wan_info_idx]), 0, sizeof(con_wan_info_T));
	}

	return;
}

//op  0:add; 1:delete; 2:flush
void rtk_update_load_balance_info(unsigned char *ifname, int wan_info_idx, unsigned char* mac, char *ip, unsigned int wan_mark, int op, int type)
{
	int i = 0 , k = 0;
	MIB_CE_ATM_VC_T wan_entry;
	unsigned char wanname[IFNAMSIZ] = {0};

	if(wan_info_idx == -1)
	{
		//printf("------%s---%s(%d),  ERROR: cannot find con_wan info or con_wan info has been full -------\n",__FILE__,__FUNCTION__,__LINE__);
		return;
	}
	rtk_get_wan_entry_by_wanmark(&wan_entry,wan_mark);
	ifGetName(wan_entry.ifIndex, wanname, sizeof(wanname));
	memcpy(con_wan[wan_info_idx].wan_name, wanname, IFNAMSIZ);

	if(op == 0)
	{
		con_wan[wan_info_idx].wan_mark = wan_mark;
		if(type == MAC_TYPE)
		{
			for(k = 0; k < MAX_MAC_NUM; k++)
			{
				if(isZeroMacstr(con_wan[wan_info_idx].mac_info[k].mac_str) && ifname && mac)
				{
					memcpy(con_wan[wan_info_idx].mac_info[k].mac_str, mac, MAC_ADDR_STRING_LEN);
					memcpy(con_wan[wan_info_idx].mac_info[k].ifname, ifname, IFNAMSIZ);
					con_wan[wan_info_idx].conn_cnt++;
					break;
				}
			}
		}
		if(type == IP_TYPE)
		{
			for(k = 0; k < MAX_IP_NUM; k++)
			{
				if(con_wan[wan_info_idx].ip_info[k].ipAddr == 0 && ifname && ip)
				{
					con_wan[wan_info_idx].ip_info[k].ipAddr = ntohl(inet_addr(ip));
					memcpy(con_wan[wan_info_idx].ip_info[k].ifname, ifname, IFNAMSIZ);
					con_wan[wan_info_idx].conn_cnt++;
					break;
				}
			}
		}
	}
	else if(op == 1)
	{
		if(type == MAC_TYPE)
		{
			for(k = 0; k < MAX_MAC_NUM; k++)
			{
				if(mac && memcmp(con_wan[wan_info_idx].mac_info[k].mac_str, mac, MAC_ADDR_STRING_LEN) == 0)
				{
					memset(con_wan[wan_info_idx].mac_info[k].mac_str, 0, MAC_ADDR_STRING_LEN);
					memset(con_wan[wan_info_idx].mac_info[k].ifname, 0, IFNAMSIZ);
					con_wan[wan_info_idx].conn_cnt--;
					break;
				}
			}
		}
		if(type == IP_TYPE)
		{
			for(k = 0; k < MAX_IP_NUM; k++)
			{
				if(ip && con_wan[wan_info_idx].ip_info[k].ipAddr == ntohl(inet_addr(ip)))
				{
					con_wan[wan_info_idx].ip_info[k].ipAddr = 0;
					memset(con_wan[wan_info_idx].ip_info[k].ifname, 0, IFNAMSIZ);
					con_wan[wan_info_idx].conn_cnt--;
					break;
				}
			}
		}

		if(con_wan[wan_info_idx].conn_cnt == 0)
		{
			memset(con_wan[wan_info_idx].wan_name, 0, IFNAMSIZ);
			con_wan[wan_info_idx].wan_mark = 0;
		}

	}
	else if(op == 2)
	{
		//when wan be delete or wan disconnect, clear wan's all connect info
		memset(&(con_wan[wan_info_idx]), 0, sizeof(con_wan_info_T));
	}

	//rtk_update_wan_conn_cnt(wan_mark, op);

	return;
}

void dump_conn_wan_info()
{
	int i, j = 0;

	printf("------%s---%s(%d), ------------------------- DUMP WAN info --------------------------- \n",__FILE__,__FUNCTION__,__LINE__ );
	for(i = 0;i < MAX_CON_WAN_NUM; i++)
	{
		if(con_wan[i].wan_mark)
		{
			printf("------%s---%s(%d), 	con_wan[%d].wan_name = %s   con_wan[%d].wan_mark = 0x%x	  con_wan[%d].conn_cnt = %d   con_wan[%d].adjust_cnt = %d ---------------------\n",__FILE__,__FUNCTION__,__LINE__ ,i , con_wan[i].wan_name ,i , con_wan[i].wan_mark ,i, con_wan[i].conn_cnt,i,con_wan[i].adjust_cnt);

			printf("------%s---%s(%d), ------------------------- DUMP MAC info  -------------------------	\n",__FILE__,__FUNCTION__,__LINE__ );
			for(j = 0; j < MAX_MAC_NUM; j++)
			{
				if(con_wan[i].mac_info[j].ifname[0] || con_wan[i].mac_info[j].mac_str[0])
				printf("------%s---%s(%d),   con_wan[%d].mac_info[%d].ifname = %s  ; mac_str = %s  -------\n",__FILE__,__FUNCTION__,__LINE__ ,i,j,con_wan[i].mac_info[j].ifname,con_wan[i].mac_info[j].mac_str);
			}

			printf("------%s---%s(%d), ------------------------- DUMP IP info  -------------------------	\n",__FILE__,__FUNCTION__,__LINE__ );
			for(j = 0; j < MAX_IP_NUM; j++)
			{
				if(con_wan[i].ip_info[j].ifname[0] || con_wan[i].ip_info[j].ipAddr)
				printf("------%s---%s(%d),   con_wan[%d].ip_info[%d].ifname = %s  ; ipAddr = 0x%x  -------\n",__FILE__,__FUNCTION__,__LINE__ ,i,j,con_wan[i].ip_info[j].ifname,con_wan[i].ip_info[j].ipAddr);
			}
		}
	}

	return;
}

void dump_port_map_info()
{
	int i, j = 0;

	printf("------%s---%s(%d), ------------------------- DUMP PORT MAP info --------------------------- \n",__FILE__,__FUNCTION__,__LINE__ );
	for(i = 0;i < MAX_CON_WAN_NUM; i++)
	{
		if(port_map_info[i].wan_mark)
		{
			printf("------%s---%s(%d), 	port_map_info[%d].wan_name = %s   port_map_info[%d].wan_mark = 0x%x	  coport_map_infon_wan[%d].conn_cnt = %d   port_map_info[%d].adjust_cnt = %d ---------------------\n",__FILE__,__FUNCTION__,__LINE__ ,i , port_map_info[i].wan_name ,i , port_map_info[i].wan_mark ,i, port_map_info[i].conn_cnt,i,port_map_info[i].adjust_cnt);

			printf("------%s---%s(%d), ------------------------- DUMP MAC info  -------------------------	\n",__FILE__,__FUNCTION__,__LINE__ );
			for(j = 0; j < MAX_MAC_NUM; j++)
			{
				if(port_map_info[i].mac_info[j].ifname[0] || port_map_info[i].mac_info[j].mac_str[0])
				printf("------%s---%s(%d),   port_map_info[%d].mac_info[%d].ifname = %s  ; port_map_info = %s  -------\n",__FILE__,__FUNCTION__,__LINE__ ,i,j,port_map_info[i].mac_info[j].ifname,port_map_info[i].mac_info[j].mac_str);
			}

			printf("------%s---%s(%d), ------------------------- DUMP IP info  -------------------------	\n",__FILE__,__FUNCTION__,__LINE__ );
			for(j = 0; j < MAX_IP_NUM; j++)
			{
				if(port_map_info[i].ip_info[j].ifname[0] || port_map_info[i].ip_info[j].ipAddr)
				printf("------%s---%s(%d),   port_map_info[%d].ip_info[%d].ifname = %s  ; ipAddr = 0x%x  -------\n",__FILE__,__FUNCTION__,__LINE__ ,i,j,port_map_info[i].ip_info[j].ifname,port_map_info[i].ip_info[j].ipAddr);
			}
		}
	}

	return;
}

int rtk_calc_total_load_balance_expect_cnt(int *cal_expect_cnt)
{
	int32_t i = 0, j = 0, k = 0, m = 0, ret, count = 0, totalEntry = 0, adjust_wan_num = 0, min_wan_idx = 0;
	MIB_CE_ATM_VC_T temp_Entry;
	lanHostInfo_t *pLanNetInfo = NULL;
	unsigned char wan_ifname[IFNAMSIZ] = {0};
	int total_cnt_org = 0, total_weight_org = 0 ;
	int total_cnt = 0, total_weight = 0, expect_over = 0;
	int weight[MAX_CON_WAN_NUM] = {0}, manual_cnt[MAX_CON_WAN_NUM] = {0}, dynamic_cnt[MAX_CON_WAN_NUM] = {0}, portmap_cnt[MAX_CON_WAN_NUM] = {0};
	double tmp_expect_cnt[MAX_CON_WAN_NUM] = {0}, diff_cnt[MAX_CON_WAN_NUM] = {0};
	int wan_idx_adjust[MAX_CON_WAN_NUM] = {0}, current_cnt[MAX_CON_WAN_NUM] = {0};
	double expect_cnt_org = 0;
	int private_sta_cnt = 0;

	if(cal_expect_cnt == NULL)
		return 0;

	ret = get_lan_net_info(&pLanNetInfo, &count);
	if(ret != 0)
	{
		if(pLanNetInfo)
			free(pLanNetInfo);
		return 0;
	}

	if(count != 0)
		total_cnt_org = count;
	else
	{
		if(pLanNetInfo)
			free(pLanNetInfo);
		return 0;
	}

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&temp_Entry))
			continue;

		ifGetName(temp_Entry.ifIndex, wan_ifname, sizeof(wan_ifname));

		if(temp_Entry.balance_type == 0)
		{
			private_sta_cnt = (rtk_calc_manual_load_balance_num(wan_ifname) + port_map_info[i].conn_cnt);
			total_cnt_org -= private_sta_cnt;
		}

		if (temp_Entry.enable == 0 || rtk_check_wan_is_connected(&temp_Entry) == 0 || (temp_Entry.balance_type == 0))
			continue;

		manual_cnt[i] = rtk_calc_manual_load_balance_num(wan_ifname);
		dynamic_cnt[i] = con_wan[i].conn_cnt;
		portmap_cnt[i] = port_map_info[i].conn_cnt;
		weight[i] = temp_Entry.weight;

		total_weight_org += temp_Entry.weight;
	}

	if(total_weight_org == 0)
		return 0;

	total_cnt = total_cnt_org;
	for(i = 0; i < totalEntry; ++i)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&temp_Entry))
			continue;

		if (temp_Entry.enable == 0 || rtk_check_wan_is_connected(&temp_Entry) == 0 || (temp_Entry.balance_type == 0))
			continue;

		expect_cnt_org = (double)( weight[i] * total_cnt_org )/total_weight_org;

		if((manual_cnt[i] + portmap_cnt[i]) > expect_cnt_org)    //if static entry > first_expect_cnt, mark cal_expect_cnt = -1 ,only dynamic  involved in calculation
		{
			cal_expect_cnt[i] = -1;
			total_cnt -= manual_cnt[i] + portmap_cnt[i];
			continue;
		}

		total_weight += weight[i];
	}

	if(total_weight == 0 || total_cnt == 0)
		return 0;

	j = 0;
	expect_over = total_cnt;
	for(i = 0; i < totalEntry; ++i)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&temp_Entry))
			continue;

		if (temp_Entry.enable == 0 || rtk_check_wan_is_connected(&temp_Entry) == 0 || (temp_Entry.balance_type == 0))
			continue;

		if(cal_expect_cnt[i] != -1)
		{
			tmp_expect_cnt[i] = (double)( weight[i] * total_cnt)/total_weight;
			cal_expect_cnt[i] = (weight[i] * total_cnt)/total_weight;
			diff_cnt[i] = (double)(cal_expect_cnt[i]) - tmp_expect_cnt[i];
			expect_over -= cal_expect_cnt[i];

			wan_idx_adjust[j] = i;

			j++;
		}
		else
		{
			cal_expect_cnt[i] = manual_cnt[i] + portmap_cnt[i];
		}
	}

	adjust_wan_num = j;

	while(expect_over > 0)
	{
		for(i = 0; i <= adjust_wan_num-2; i++)
		{
			for (k = i+1; k <= adjust_wan_num-1; k++)
			{
				if((diff_cnt[wan_idx_adjust[i]] - diff_cnt[wan_idx_adjust[k]])< 0.000001)
				 	min_wan_idx = wan_idx_adjust[i];
				else
					min_wan_idx = wan_idx_adjust[k];
			}
		}

		diff_cnt[min_wan_idx]++;

		cal_expect_cnt[min_wan_idx]++;
		expect_over--;
	}

	if(pLanNetInfo)
		free(pLanNetInfo);

	return 1;
}

int rtk_find_ip_by_mac_from_lannetinfo(unsigned char *ip, unsigned char *mac)
{
	unsigned int i, count = 0;
	lanHostInfo_t *pLanNetInfo=NULL;
	unsigned char lan_mac[MAC_ADDR_STRING_LEN] = {0};
	unsigned char ip_addr[32] = {0};

	if(ip == NULL || mac == NULL)
		return;

	if(get_lan_net_info(&pLanNetInfo, &count) != 0)
	{
		return 0;
	}

	if(count > 0)
	{
		for(i = 0; i<count; i++)
		{
			memset(lan_mac,0,sizeof(lan_mac));

			snprintf(lan_mac,sizeof(lan_mac),"%02x:%02x:%02x:%02x:%02x:%02x", pLanNetInfo[i].mac[0],pLanNetInfo[i].mac[1],pLanNetInfo[i].mac[2],pLanNetInfo[i].mac[3],pLanNetInfo[i].mac[4],pLanNetInfo[i].mac[5]);

			if(memcmp(lan_mac, mac, MAC_ADDR_STRING_LEN) == 0)
			{
				snprintf(ip_addr, sizeof(ip_addr), "%d.%d.%d.%d",pLanNetInfo[i].ip>>24,(pLanNetInfo[i].ip>>16)&0xff, (pLanNetInfo[i].ip>>8)&0xff, pLanNetInfo[i].ip&0xff);

				memcpy(ip,ip_addr,32);

				return 1;
			}
		}
	}

	return 1;
}

void rtk_delete_ct_flow_when_load_balance_change(unsigned char *mac)
{
	unsigned char delete_enable = 0;
	unsigned char ip[32] = {0};
	char cmdbuf[128] = {0};

	if(mac == NULL)
		return;

	if(!mib_get_s(MIB_DELETE_CT_FLOW_ENABLE, (void *)&delete_enable,sizeof(delete_enable)))
	{
		printf("get delete ct flow enable failed!\n");
	}

	if(delete_enable)
	{
		rtk_find_ip_by_mac_from_lannetinfo(ip, mac);

		if(ip[0])
		{
			//printf("------%s---%s(%d),	trace: 	     ip = %s      -------\n",__FILE__,__FUNCTION__,__LINE__, ip);

			snprintf(cmdbuf,sizeof(cmdbuf),"echo 0 SIP4 %s > /proc/fc/ctrl/flow_operation", ip);
			system(cmdbuf);

			snprintf(cmdbuf,sizeof(cmdbuf),"echo 0 IPV4 %s > /proc/fc/ctrl/ct_operation", ip);
			system(cmdbuf);
		}
	}

	return;
}

void rtk_set_load_balance_mac_iptables_2(unsigned char *ifname, int wan_entry_idx, unsigned char *mac, int wan_mark, int op)
{
	char polmark[64] = {0};

	if(ifname == NULL || mac == NULL)
		return;

	sprintf(polmark, "0x%x", wan_mark);
	//only support V4
	va_cmd(EBTABLES, 12, 1, "-t", "broute", (op==1)?"-A":"-D", (char *)DYNAMIC_MAC_LOAD_BALANCE, "-s", mac, "-p", "IPv4", "-j", "mark", "--set-mark", polmark, "--mark-target", "CONTINUE");

	if(op == 0)
		rtk_delete_ct_flow_when_load_balance_change(mac);

	rtk_process_bridge_ext_rules(wan_mark, mac, 0, op);

	rtk_update_load_balance_info(ifname,wan_entry_idx, mac, 0 , wan_mark, (op==1)?0:1 , 0);

	return;
}

void rtk_set_dynamic_load_balance_adjust_cnt(int wan_info_idx, char *wan_itf_name, int count)
{
	int i = 0 ,wan_mark ;

	if(wan_info_idx == -1 || wan_itf_name == NULL) // con_info doesn't exit,find empty entry   (for new wan ,or it has no connected lan client)
	{
		//printf("------%s---%s(%d),  ERROR: find wan  %s  dynamic info fail -------\n",__FILE__,__FUNCTION__,__LINE__,wan_itf_name);
		return;
	}

	rtk_get_wan_mark_by_wanname(wan_itf_name, &wan_mark);

	con_wan[wan_info_idx].adjust_cnt = count;
	memcpy(con_wan[wan_info_idx].wan_name, wan_itf_name, IFNAMSIZ);
	con_wan[wan_info_idx].wan_mark = wan_mark;

	return;
}

void rtk_adjust_con_stas_to_below_wan(int target_idx, int over_idx)
{
	int i, j , tmp_cnt = 0, move_cnt = 0;
	unsigned char mac_tmp[MAC_ADDR_STRING_LEN];

	if((con_wan[target_idx].adjust_cnt + con_wan[over_idx].adjust_cnt) >= 0 )
		move_cnt = -(con_wan[target_idx].adjust_cnt);
	else
		move_cnt = con_wan[over_idx].adjust_cnt;

	for(i = 0; i < MAX_MAC_NUM; i++)
	{
		if(isZeroMac(con_wan[over_idx].mac_info[i].mac_str) == 0)
		{
			memcpy(mac_tmp,con_wan[over_idx].mac_info[i].mac_str,MAC_ADDR_STRING_LEN);

			rtk_set_load_balance_mac_iptables_2(con_wan[over_idx].mac_info[i].ifname, target_idx, con_wan[over_idx].mac_info[i].mac_str,con_wan[target_idx].wan_mark, 1);
			rtk_set_load_balance_mac_iptables_2(con_wan[over_idx].mac_info[i].ifname, over_idx, con_wan[over_idx].mac_info[i].mac_str,con_wan[over_idx].wan_mark, 0);

			rtk_update_save_sta_info(mac_tmp, con_wan[target_idx].wan_mark);

			(con_wan[over_idx].adjust_cnt)--;
			(con_wan[target_idx].adjust_cnt)++;
			move_cnt-- ;

			if(move_cnt == 0)
				break;
		}
	}

	return;
}

int rtk_check_sta_is_exist(unsigned char *sta_mac)
{
	lanHostInfo_t *pLanNetInfo = NULL;
	unsigned char lan_mac[18] = {0};
	char LanDevName[64] = {0};
	int i, ret, count = 0 ,is_exist = 0;

	if(sta_mac == NULL)
		return -1;

	ret = get_lan_net_info(&pLanNetInfo, &count);
	if(ret != 0)
	{
		if(pLanNetInfo)
			free(pLanNetInfo);
		return -1;
	}

	if(count>0){
		for(i=0; i<count; i++)
		{
			snprintf(lan_mac,sizeof(lan_mac),"%02x:%02x:%02x:%02x:%02x:%02x", pLanNetInfo[i].mac[0],pLanNetInfo[i].mac[1],pLanNetInfo[i].mac[2],pLanNetInfo[i].mac[3],pLanNetInfo[i].mac[4],pLanNetInfo[i].mac[5]);
			if(memcmp(lan_mac, sta_mac, MAC_ADDR_STRING_LEN) == 0)
				is_exist = 1;

		}
	}

	if(pLanNetInfo)
		free(pLanNetInfo);

	return is_exist;
}

void rtk_check_unexist_sta_and_delete_info(void)
{
	int i, j = 0;

	for(i = 0;i < MAX_CON_WAN_NUM; i++)
	{
		if(con_wan[i].wan_mark)
		{
			for(j = 0; j < MAX_MAC_NUM; j++)
			{
				if((isZeroMac(con_wan[i].mac_info[j].mac_str) == 0) && (rtk_check_sta_is_exist(con_wan[i].mac_info[j].mac_str) == 0))
				{
					rtk_set_load_balance_mac_iptables_2(con_wan[i].mac_info[j].ifname, i, con_wan[i].mac_info[j].mac_str, con_wan[i].wan_mark, 0);
				}
			}
		}
	}

	return;
}

void rtk_get_save_wan_time_by_mac_and_wanmark(unsigned char *mac, int wan_mark, long *up_time)
{
	int i ,j = 0;

	if (mac == NULL ||(*mac == '\0') || up_time == NULL)
		return;

	//to find time of the sta last connect to the wan
	for(i = 0; i < MAX_STA_NUM; i++)
	{
		if(memcmp(save_sta_info[i].mac_str, mac, MAC_ADDR_STRING_LEN) == 0)
		{
			if(save_sta_info[i].save_cnt > 0)
			{
				for(j = save_sta_info[i].save_cnt - 1 ; j >= 0; j--)
				{
					if(save_sta_info[i].save_wan_info[j].save_wan_mark == wan_mark)
					{
						*up_time = save_sta_info[i].save_wan_info[j].save_wan_time;
						return;
					}
			}
			}
		}
	}

	return;
}

void rtk_move_sta_to_target_wan(unsigned char *mac, unsigned char *sta_ifname, int org_wan_idx, int target_wan_idx)
{
	int i, j = 0;
	long tmp_up_time, tmp_min_up_time = 0;

	if (mac == NULL || sta_ifname == NULL|| (*mac == '\0'))
		return;

	if(target_wan_idx >= 0 && org_wan_idx >= 0)
	{
		rtk_set_load_balance_mac_iptables_2(sta_ifname, target_wan_idx, mac, con_wan[target_wan_idx].wan_mark, 1);
		rtk_set_load_balance_mac_iptables_2(sta_ifname, org_wan_idx, mac, con_wan[org_wan_idx].wan_mark, 0);

		rtk_update_save_sta_info(mac, con_wan[target_wan_idx].wan_mark);

		(con_wan[org_wan_idx].adjust_cnt)--;
		(con_wan[target_wan_idx].adjust_cnt)++;
	}

	return;
}


void rtk_move_sta_by_save_info_time(sta_info_T *tmp_sta_mac_p, int len ,int org_wan_idx)
{
	int i, j , m, move_num = 0, move_sta_idx = -1;
	long tmp_time, wan_up_time = 0;

	for(i = 0; i < MAX_CON_WAN_NUM; i++)
	{
		if(con_wan[i].adjust_cnt < 0)
		{
			for(j = 0; j < len; j++)
			{
				//find matched sta ,move it to target wan
				if(isZeroMacstr(tmp_sta_mac_p[j].mac_str) == 0)
				{
					rtk_get_save_wan_time_by_mac_and_wanmark(tmp_sta_mac_p[j].mac_str, con_wan[i].wan_mark, &wan_up_time);
					if(wan_up_time && (tmp_time == 0))
						tmp_time = wan_up_time;

					if(wan_up_time && (wan_up_time < tmp_time))
					{
						tmp_time = wan_up_time;
						move_sta_idx = j;
					}
				}
			}

			if(move_sta_idx >= 0)
			{
				rtk_move_sta_to_target_wan(tmp_sta_mac_p[move_sta_idx].mac_str, tmp_sta_mac_p[move_sta_idx].sta_ifname, org_wan_idx, i);
				memset(&(tmp_sta_mac_p[move_sta_idx]), 0 ,sizeof(sta_info_T));
			}
		}
	}

	for(i = 0; i < MAX_CON_WAN_NUM; i++)
	{
		if(con_wan[i].adjust_cnt < 0)
		{
			for(j = 0; j < len; j++)
			{
				if(isZeroMacstr(tmp_sta_mac_p[j].mac_str) == 0)
				{
					rtk_move_sta_to_target_wan(tmp_sta_mac_p[j].mac_str, tmp_sta_mac_p[j].sta_ifname, org_wan_idx, i);
					memset(&(tmp_sta_mac_p[j]), 0 ,sizeof(sta_info_T));
				}
			}
		}
	}

	return;
}

void rtk_adjust_con_info_by_save_info(void)
{
	int i, j , k , m, n = 0;
	sta_info_T tmp_mac_array[MAX_SAVE_STA_NUM] = {0};
	sta_info_T tmp_mac;
	unsigned char tmp_mac_str[MAC_ADDR_STRING_LEN] = {0};
	unsigned char tmp_sta_ifname[IFNAMSIZ] = {0};
	long tmp_up_time;

	for(i = 0;i < MAX_CON_WAN_NUM; i++)
	{
		if(con_wan[i].adjust_cnt > 0)
		{
			k = 0;
			//get all sta's time of connect to the over wan
			for(j = 0; j < MAX_MAC_NUM; j++)
			{
				if(isZeroMacstr(con_wan[i].mac_info[j].mac_str) == 0)
				{
					memcpy(tmp_mac_str, con_wan[i].mac_info[j].mac_str, MAC_ADDR_STRING_LEN);
					memcpy(tmp_sta_ifname, con_wan[i].mac_info[j].ifname, IFNAMSIZ);
					rtk_get_save_wan_time_by_mac_and_wanmark(tmp_mac_str, con_wan[i].wan_mark, &tmp_up_time);

					if(tmp_up_time)
					{
						memcpy(tmp_mac_array[k].mac_str, tmp_mac_str, MAC_ADDR_STRING_LEN);
						memcpy(tmp_mac_array[k].sta_ifname, tmp_sta_ifname, MAC_ADDR_STRING_LEN);
						tmp_mac_array[k].wan_time = tmp_up_time;
						k++;
					}
				}
			}

			if(k <= 0)
				continue;

			if(k > 1)
			{
				for(m = 0; m < k-1; m++)
				{
					for(n = 0; n < k-m-1; n++)
					{
						if(tmp_mac_array[n].wan_time < tmp_mac_array[n+1].wan_time)
						{
							memcpy(&tmp_mac, &(tmp_mac_array[n]), sizeof(sta_info_T));
							memcpy(&(tmp_mac_array[n]), &(tmp_mac_array[n+1]), sizeof(sta_info_T));
							memcpy(&(tmp_mac_array[n+1]), &tmp_mac, sizeof(sta_info_T));
						}
					}
				}
			}

			for( m = 0; m < con_wan[i].adjust_cnt; m++)
			{
				rtk_move_sta_by_save_info_time(tmp_mac_array, con_wan[i].adjust_cnt, i);
				if(con_wan[i].adjust_cnt == 0)
					break;
			}

		}
	}

	return;
}

void rtk_distribute_global_load_balance(void)
{
	int manual_cnt = 0, port_map_cnt = 0, dynamic_cnt = 0, adjust_cnt = 0;
	int32_t i, j,  totalEntry = 0;
	MIB_CE_ATM_VC_T temp_Entry;
	unsigned char wan_ifname[IFNAMSIZ] = {0};
	int expect_cnt[MAX_CON_WAN_NUM] = {0};

	rtk_check_unexist_sta_and_delete_info();

	if(rtk_calc_total_load_balance_expect_cnt(expect_cnt)==0)
		return;

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&temp_Entry))
			continue;

		if (temp_Entry.enable == 0 || rtk_check_wan_is_connected(&temp_Entry) == 0 || (temp_Entry.balance_type == 0))
			continue;

		ifGetName(temp_Entry.ifIndex, wan_ifname, sizeof(wan_ifname));
		manual_cnt = rtk_calc_manual_load_balance_num(wan_ifname);
		dynamic_cnt = con_wan[i].conn_cnt;
		port_map_cnt = port_map_info[i].conn_cnt;

		adjust_cnt = ((manual_cnt + dynamic_cnt + port_map_cnt)- expect_cnt[i]);

		rtk_set_dynamic_load_balance_adjust_cnt(i, wan_ifname, adjust_cnt);
	}

	rtk_adjust_con_info_by_save_info();

	for(i = 0;i < MAX_CON_WAN_NUM; i++)
	{
		if(con_wan[i].adjust_cnt == 0)
			continue;

		if(con_wan[i].adjust_cnt < 0)
		{
			for(j = 0;j < MAX_CON_WAN_NUM; j++)
			{
				if(con_wan[j].adjust_cnt == 0)
					continue;

				if(con_wan[j].adjust_cnt > 0)
				{
					rtk_adjust_con_stas_to_below_wan(i , j);
				}

				if(con_wan[i].adjust_cnt == 0)
					break;
			}
		}
	}

	for(i = 0;i < MAX_CON_WAN_NUM; i++)
	{
		con_wan[i].adjust_cnt = 0;
	}

	return;
}

int rtk_check_wan_is_connected(MIB_CE_ATM_VC_Tp pEntry)
{
	int is_connect = 0;
	int flag = 0;
	rtk_port_linkStatus_t status;
	char wanname[32] = {0};
	struct in_addr inAddr;
#ifdef CONFIG_IPV6
	struct ipv6_ifaddr ip6_addr[6];
#endif

	ifGetName(pEntry->ifIndex, wanname, sizeof(wanname));

	if (getInFlags(wanname, &flag) == 1)
	{
		if((flag & IFF_UP) && (flag & IFF_RUNNING))
		{
			//check ipv4
			if(pEntry->IpProtocol == IPVER_IPV4)
			{
				if(getInAddr(wanname, IP_ADDR, (void *)&inAddr) == 1) // wan get IPV4 addr
					is_connect = 1;
			}
			//check ipv6
#ifdef CONFIG_IPV6
			if(pEntry->IpProtocol == IPVER_IPV6)
			{
				if(getifip6(wanname, IPV6_ADDR_UNICAST, ip6_addr, 6) > 0)
					is_connect = 1;
			}
			//check ipv4 && ipv6
			if(pEntry->IpProtocol == IPVER_IPV4_IPV6)
			{
				if((getInAddr(wanname, IP_ADDR, (void *)&inAddr) == 1)
					&&(getifip6(wanname, IPV6_ADDR_UNICAST, ip6_addr, 6) > 0))
					is_connect = 1;
			}
#endif
		}
	}


	if(pEntry->cmode == CHANNEL_MODE_BRIDGE)
	{

		if(rtk_port_link_get(rtk_port_get_phyPort(pEntry->logic_port), &status)!=0)
		{
			printf("\n port stats get error!%s %d\n",__FUNCTION__,__LINE__);
			return RTK_FAILED;
		}
		if(status==0 || status==2)
			is_connect = 0;
		else if(status == 1)
			is_connect = 1;
	}

	return is_connect;
}

int rtk_check_port_mapping_rules_exist(unsigned char *ifname, int *wan_entry_idx, unsigned int *wan_mark)
{
	int i ,j ,totalEntry = 0, is_exist = 0;
	int logic_port = -1;
	unsigned int *wan_mask;
	MIB_CE_ATM_VC_T temp_Entry;
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
	MIB_CE_PORT_BINDING_T portBindEntry;
	struct vlan_pair *vid_pair;
	unsigned char *vlan_if_tmp = NULL;
	int ssidIdx = -1;
#endif
	char ifName[IFNAMSIZ];

	if(ifname == NULL || wan_entry_idx == NULL || wan_mark == NULL)
		return 0;

	for(i = 0; i < SW_LAN_PORT_NUM; i++)
	{
		if(strncmp(ifname, ELANVIF[i], 8)==0)
		{
			logic_port = i;
			break;
		}
	}

	for(j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)
	{
		if(strncmp(ifname, wlan[j-PMAP_WLAN0], IFNAMSIZ)== 0)
		{
			logic_port = j;
			break;
		}
	}

	if(logic_port == -1) return;

	//check port-binding
	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&temp_Entry))
		{
			if (!temp_Entry.enable)
				continue;

			if(BIT_IS_SET(temp_Entry.itfGroup, logic_port))
			{
				is_exist = 1;
				*wan_entry_idx = i;
				ifGetName(temp_Entry.ifIndex, ifName, sizeof(ifName));
				rtk_net_get_wan_policy_route_mark(ifName, wan_mark, wan_mask);
			}
		}
	}

	//check vlan-binding
#if defined(CONFIG_00R0) || defined(CONFIG_E8B)
	totalEntry = mib_chain_total(MIB_PORT_BINDING_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_PORT_BINDING_TBL, i, (void*)&portBindEntry))
		{
			if(portBindEntry.port < PMAP_WLAN0 && portBindEntry.port < ELANVIF_NUM)
				vlan_if_tmp = ELANVIF[portBindEntry.port];

			if((ssidIdx = (portBindEntry.port-PMAP_WLAN0)) < (WLAN_SSID_NUM*NUM_WLAN_INTERFACE) && ssidIdx >= 0)
				vlan_if_tmp = wlan[ssidIdx];

			if(vlan_if_tmp && memcmp(ifname, vlan_if_tmp , IFNAMSIZ) == 0)
			{
				if((unsigned char)VLAN_BASED_MODE == portBindEntry.pb_mode)
				{
					vid_pair = (struct vlan_pair *)&(portBindEntry.pb_vlan0_a);
					for (j=0; j<4; j++)
					{
						if (vid_pair[j].vid_a > 0)
						{
							is_exist = 1;
							rtk_get_wan_entry_by_wanvid(wan_entry_idx, &temp_Entry ,vid_pair[j].vid_b);
							ifGetName(temp_Entry.ifIndex, ifName, sizeof(ifName));
							rtk_net_get_wan_policy_route_mark(ifName, wan_mark, wan_mask);
						}
					}
				}
			}
		}
	}
#endif

	return is_exist;
}

int rtk_check_load_balance_rules_exist(unsigned char *mac, char *ip)
{
	int i ,j ,entryNum = 0, is_exist = 0;
	unsigned char ip_tmp[32] = {0};
	MIB_STATIC_LOAD_BALANCE_T Entry_tmp;
	char manual_enable = 0;

	//check static load_balance info
	if(!mib_get_s(MIB_STATIC_LOAD_BALANCE_ENABLE, (void *)&manual_enable,sizeof(manual_enable)))
	{
		printf("------%s---%s(%d),	ERROR: MIB_STATIC_LOAD_BALANCE_ENABLE get fail -------\n",__FILE__,__FUNCTION__,__LINE__  );
	}

	if(manual_enable != 0)
	{
		entryNum = mib_chain_total(MIB_STATIC_LOAD_BALANCE_TBL);

		for (i = 0; i < entryNum; i++) {
			if (!mib_chain_get(MIB_STATIC_LOAD_BALANCE_TBL, i, (void *)&Entry_tmp))
				continue;

			if(mac && memcmp(Entry_tmp.mac, mac, MAC_ADDR_STRING_LEN) == 0)
				is_exist = 1;

		}
	}

	//check dynamic load_balance info
	for(i = 0;i < MAX_CON_WAN_NUM; i++)
	{
		if(con_wan[i].wan_mark)
		{
			if(mac && isZeroMacstr(mac) == 0)
			{
				for(j = 0; j < MAX_MAC_NUM; j++)
				{
					if((isZeroMacstr(con_wan[i].mac_info[j].mac_str)==0) && memcmp(con_wan[i].mac_info[j].mac_str, mac, MAC_ADDR_STRING_LEN) == 0)
					{
						is_exist = 1;
					}
				}
			}

			if(ip && ip[0]!= 0)
			{
				for(j = 0; j < MAX_IP_NUM; j++)
				{
					if(con_wan[i].ip_info[j].ipAddr)
					{
						snprintf(ip_tmp, sizeof(ip_tmp), "%s", (inet_ntoa(*(struct in_addr *)&(con_wan[i].ip_info[j].ipAddr))));
						if(strcmp(ip_tmp, ip) == 0)
						{
							is_exist = 1;
						}
					}
				}
			}
		}
	}

	return is_exist;
}

int rtk_check_port_map_info_exist(unsigned char *mac, char *ip)
{
	int i ,j , is_exist = 0;
	unsigned char ip_tmp[32] = {0};

	//check port map info
	for(i = 0;i < MAX_CON_WAN_NUM; i++)
	{
		if(port_map_info[i].wan_mark)
		{
			if(mac && isZeroMacstr(mac) == 0)
			{
				for(j = 0; j < MAX_MAC_NUM; j++)
				{
					if((isZeroMacstr(port_map_info[i].mac_info[j].mac_str)==0) && memcmp(port_map_info[i].mac_info[j].mac_str, mac, MAC_ADDR_STRING_LEN) == 0)
					{
						is_exist = 1;
					}
				}
			}

			if(ip && ip[0]!= 0)
			{
				for(j = 0; j < MAX_IP_NUM; j++)
				{
					if(port_map_info[i].ip_info[j].ipAddr)
					{
						snprintf(ip_tmp, sizeof(ip_tmp), "%s", (inet_ntoa(*(struct in_addr *)&(port_map_info[i].ip_info[j].ipAddr))));
						if(strcmp(ip_tmp, ip) == 0)
						{
							is_exist = 1;
						}
					}
				}
			}
		}
	}

	return is_exist;
}

int rtk_find_match_wan_mark(int *wan_info_idx, unsigned int *wan_mark)
{
	int entryNum = 0, i = 0, j = 0, wan_idx = -1;
	MIB_CE_ATM_VC_T Entry1, Entry2, wan_entry;
	unsigned int *wan_mask;
	int manual_cnt1 , dynamic_cnt1 ,port_map_cnt1 , manual_cnt2 , dynamic_cnt2 , port_map_cnt2 = 0;
	unsigned char wanname1[IFNAMSIZ], wanname2[IFNAMSIZ] = {0};
	char ifName[IFNAMSIZ];

	if(wan_info_idx == NULL || wan_mark == NULL)
		return 0 ;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry1))
			continue;

		ifGetName(Entry1.ifIndex, wanname1, sizeof(wanname1));
		manual_cnt1 = rtk_calc_manual_load_balance_num(wanname1);
		dynamic_cnt1 = con_wan[i].conn_cnt;
		port_map_cnt1 = port_map_info[i].conn_cnt;


		if(Entry1.weight > 0 && rtk_check_wan_is_connected(&Entry1) > 0 && (Entry1.balance_type != 0))
		{
			for (j = i+1; j < entryNum; j++) {
				if (!mib_chain_get(MIB_ATM_VC_TBL, j, (void *)&Entry2))
					continue;

				ifGetName(Entry2.ifIndex, wanname2, sizeof(wanname2));
				manual_cnt2 = rtk_calc_manual_load_balance_num(wanname2);
				dynamic_cnt2 = con_wan[j].conn_cnt;
				port_map_cnt2 = port_map_info[j].conn_cnt;

				if(Entry2.weight > 0 && rtk_check_wan_is_connected(&Entry2) > 0 && (Entry2.balance_type != 0))
				{
					if ((manual_cnt1 + dynamic_cnt1 + port_map_cnt1)*(Entry2.weight) > (manual_cnt2 + dynamic_cnt2 + port_map_cnt2)*(Entry1.weight))
						i = j;
				}
			}
			wan_idx = i;
			break;
		}
	}

	if ((wan_idx == -1) || (wan_idx >= entryNum))
		return 0;

	if(!mib_chain_get(MIB_ATM_VC_TBL, wan_idx, (void *)&wan_entry))
		return 0;

	*wan_info_idx = wan_idx;
	ifGetName(wan_entry.ifIndex, ifName, sizeof(ifName));
	rtk_net_get_wan_policy_route_mark(ifName, wan_mark, wan_mask);

	return 1;
}
/*
void process_dynamic_wan_con_info2(int sign_id, char *sign_str)
{
	int pid, shmid;
	char *viraddr;
	union sigval mysigval;

	sleep(1);  // wait for last process ,for release shared memory

	shmid=shmget(0x44,30,0666|IPC_CREAT);
	viraddr=(char*)shmat(shmid,0,0);
	strcat(viraddr,sign_str);

	pid = findPidByName("systemd");
	if (pid <= 0){
		printf("[%s:%d] systemd pid=%d\n", __FUNCTION__,__LINE__,pid);
		return;
	}

	mysigval.sival_int = sign_id;

	if (sigqueue(pid, SIGUSR2, mysigval) == -1)
		printf("sigqueue error");

	shmdt(viraddr);

	return ;
}
*/

/*   save wan info to file  */
int writeConWanInfo()
{
	int fd, i;
	con_wan_info_T *con_wan_data[MAX_CON_WAN_NUM];

	fd = open(CONWANFILE, O_RDWR|O_CREAT, 0644);
	if (fd < 0)
	{
		printf("------%s---%s(%d),	open error	-------\n",__FILE__,__FUNCTION__,__LINE__);
		return -1;
	}

	if( !flock_set(fd, F_WRLCK) )
	{
		/* clear /var/con_wan_info */
		if ( !truncate(CONWANFILE, 0) )
		{
			for(i = 0; i < MAX_CON_WAN_NUM; i++)
			{
				con_wan_data[i] = &con_wan[i];
				write(fd, (void *)con_wan_data[i], sizeof(con_wan_info_T));
			}
			flock_set(fd, F_UNLCK);
		}
	}

	close(fd);

	return 0;
}

int get_con_wan_info(con_wan_info_T **ppConWandata, unsigned int *pCount)
{
	struct stat status;
	int fd, read_size;
	int i , j = 0;
	char cmd[128] = {0};
	void *p = NULL;

#ifdef CONFIG_USER_RETRY_GET_FLOCK
	unsigned long long msecond = 0;
	struct timeval tv;
	struct timeval initial_tv;
#endif

	/***    send sign to systemd   *******/
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process write_dynamic_file");
	system(cmd);

	if((ppConWandata == NULL) || (pCount==NULL))
	{
		printf("------%s---%s(%d),	NULL error	-------\n",__FILE__,__FUNCTION__,__LINE__);
		return -1;
	}

	*ppConWandata = NULL;
	*pCount = 0;

	if ( stat(CONWANFILE, &status) < 0 )
	{
		printf("------%s---%s(%d),	stats error	-------\n",__FILE__,__FUNCTION__,__LINE__);
		return -3;
	}

	fd = open(CONWANFILE, O_RDONLY);
	if(fd < 0)
	{
		printf("------%s---%s(%d),	open error -------\n",__FILE__,__FUNCTION__,__LINE__);
		return -3;
	}

#ifdef CONFIG_USER_RETRY_GET_FLOCK
	gettimeofday (&tv, NULL);
	initial_tv.tv_sec = tv.tv_sec;
	initial_tv.tv_usec = tv.tv_usec;

	do {
		if (!flock_set(fd, F_RDLCK))
		{
			if ((p = (void *)read_data_from_file(fd, CONWANFILE, &read_size)) == NULL)
			{
				printf("(%s)%d read file error!\n", __FUNCTION__, __LINE__);
				flock_set(fd, F_UNLCK);
				close(fd);
		return -3;
	}

			flock_set(fd, F_UNLCK);
			break;
		}

		gettimeofday(&tv, NULL);
		msecond = (tv.tv_sec *1000 + tv.tv_usec/1000) - (initial_tv.tv_sec *1000 + initial_tv.tv_usec/1000);
	} while(msecond < MAX_GETWANCONFINFO_INTERVAL);
#else
	if(!flock_set(fd, F_RDLCK))
	{
		if ((p = (void *)read_data_from_file(fd, CONWANFILE, &read_size)) == NULL)
		{
			printf("(%s)%d read file error!\n", __FUNCTION__, __LINE__);
			flock_set(fd, F_UNLCK);
			close(fd);
			return -3;
		}

		flock_set(fd, F_UNLCK);
	}
#endif

	*ppConWandata = (con_wan_info_T *)p;
	*pCount = read_size / sizeof(con_wan_info_T);

	close(fd);
	return 0;
}

/*  save port map info to file  */
int writePortMapInfo()
{
	int fd, i;
	con_wan_info_T *port_map_data[MAX_CON_WAN_NUM];

	fd = open(PORTMAPFILE, O_RDWR|O_CREAT, 0644);
	if (fd < 0)
	{
		printf("------%s---%s(%d),	open error	-------\n",__FILE__,__FUNCTION__,__LINE__);
		return -1;
	}

	if( !flock_set(fd, F_WRLCK) )
	{
		/* clear /var/con_wan_info */
		if ( !truncate(PORTMAPFILE, 0) )
		{
			for(i = 0; i < MAX_CON_WAN_NUM; i++)
			{
				port_map_data[i] = &port_map_info[i];
				write(fd, (void *)port_map_data[i], sizeof(con_wan_info_T));
			}
			flock_set(fd, F_UNLCK);
		}
	}

	close(fd);

	return 0;
}

void deleteUnexistStaSaveInfo(void)
{
	int i = 0;

	for(i = 0;i < MAX_SAVE_STA_NUM; i++)
	{
		if(rtk_check_sta_is_exist(save_sta_info[i].mac_str) == 0)
			memset(&(save_sta_info[i]), 0, sizeof(save_sta_info_T));
	}

	return;
}

int get_port_map_info(con_wan_info_T **ppPortMapdata, unsigned int *pCount)
{
	struct stat status;
	int fd, read_size;
	int i , j = 0;
	char cmd[128] = {0};
	void *p = NULL;

#ifdef CONFIG_USER_RETRY_GET_FLOCK
	unsigned long long msecond = 0;
	struct timeval tv;
	struct timeval initial_tv;
#endif

	/***    send sign to systemd   *******/
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process write_portmap_file");
	system(cmd);

	if((ppPortMapdata == NULL) || (pCount==NULL))
	{
		printf("------%s---%s(%d),	NULL error	-------\n",__FILE__,__FUNCTION__,__LINE__);
		return (-1);
	}

	*ppPortMapdata = NULL;
	*pCount = 0;

	if ( stat(PORTMAPFILE, &status) < 0 )
	{
		printf("------%s---%s(%d),	stats error	-------\n",__FILE__,__FUNCTION__,__LINE__);
		return (-3);
	}

	fd = open(PORTMAPFILE, O_RDONLY);
	if(fd < 0)
	{
		printf("------%s---%s(%d),	open error -------\n",__FILE__,__FUNCTION__,__LINE__);
		return (-3);
	}

#ifdef CONFIG_USER_RETRY_GET_FLOCK
	gettimeofday (&tv, NULL);
	initial_tv.tv_sec = tv.tv_sec;
	initial_tv.tv_usec = tv.tv_usec;

	do {
		if (!flock_set(fd, F_RDLCK))
		{
			if ((p = (void *)read_data_from_file(fd, PORTMAPFILE, &read_size)) == NULL)
			{
				printf("(%s)%d read file error!\n", __FUNCTION__, __LINE__);
				flock_set(fd, F_UNLCK);
				close(fd);
				return (-3);
			}

			flock_set(fd, F_UNLCK);
			break;
	}

		gettimeofday(&tv, NULL);
		msecond = (tv.tv_sec *1000 + tv.tv_usec/1000) - (initial_tv.tv_sec *1000 + initial_tv.tv_usec/1000);
	} while(msecond < MAX_GETPORTMAPINFO_INTERVAL);
#else
	if(!flock_set(fd, F_RDLCK))
	{
		if ((p = (void *)read_data_from_file(fd, PORTMAPFILE, &read_size)) == NULL)
		{
			printf("(%s)%d read file error!\n", __FUNCTION__, __LINE__);
			flock_set(fd, F_UNLCK);
			close(fd);
			return -3;
		}

		flock_set(fd, F_UNLCK);
	}
#endif

	*ppPortMapdata = (con_wan_info_T *)p;
	*pCount = read_size / sizeof(con_wan_info_T);

	close(fd);
	return 0;
}

//0: del    1:add
void rtk_set_load_balance_mac_iptables(unsigned char *ifname, unsigned char *mac, int op)
{
	char polmark[64] = {0};
	uint32_t wan_mark = 0, wan_mask = 0;
	int wan_entry_idx = -1;

	if(ifname == NULL || isZeroMacstr(mac))
		return;

	if(op == 1) //add rules
	{
		if(rtk_check_load_balance_rules_exist(mac, 0) == 1)
		{
			//printf("------%s---%s(%d),	TRACE	  mac rules has been exist !!!!! -------\n",__FILE__,__FUNCTION__,__LINE__);
			return;
		}

		if(rtk_check_port_mapping_rules_exist(ifname, &wan_entry_idx, &wan_mark) == 1)
		{
			//printf("------%s---%s(%d),	TRACE	 port mapping exist !!!!!! ,don't add dynamic rule, just save port-bind info  -------\n",__FILE__,__FUNCTION__,__LINE__);
			if(rtk_check_port_map_info_exist(mac, 0) == 0)
			{
				//printf("------%s---%s(%d),	TRACE	 port mapping rules has been exist !!!!!  -------\n",__FILE__,__FUNCTION__,__LINE__);
				rtk_update_port_map_info(ifname, wan_entry_idx, mac, 0 , wan_mark, (op==1)?0:1 , 0);
			}
			return;
		}
		else if(rtk_find_save_sta_info_wan_mark(&wan_entry_idx, &wan_mark, mac) == 1)
		{
			//printf("------%s---%s(%d),	TRACE	  find save wan info    wan_entry_idx = %d    wan_mark = 0x%x -------\n",__FILE__,__FUNCTION__,__LINE__,wan_entry_idx, wan_mark);
		}
		else if(rtk_find_match_wan_mark(&wan_entry_idx, &wan_mark)==0)
		{
			//printf("------%s---%s(%d),	TRACE	  find match wan fail !!!!! -------\n",__FILE__,__FUNCTION__,__LINE__);
			return;
		}
	}

	if(op == 0)  //delete rules
	{
		rtk_get_port_map_wan_mark_by_mac(mac, &wan_entry_idx, &wan_mark);

		if(wan_mark)
		{
			rtk_update_port_map_info(ifname, wan_entry_idx, mac, 0 , wan_mark, (op==1)?0:1  , 0);
			return;
		}

		rtk_get_load_balance_wan_mark_by_mac(mac, &wan_entry_idx, &wan_mark);
	}

	if(wan_mark)
	{
		sprintf(polmark, "0x%x", wan_mark);
		//only support V4
		va_cmd(EBTABLES, 12, 1, "-t", "broute", (op==1)?"-A":"-D", (char *)DYNAMIC_MAC_LOAD_BALANCE, "-s", mac, "-p", "IPv4", "-j", "mark", "--set-mark", polmark, "--mark-target", "CONTINUE");

		if(op == 0)
		{
			rtk_delete_ct_flow_when_load_balance_change(mac);
		}

		rtk_process_bridge_ext_rules(wan_mark, mac, 0, op);

		rtk_update_load_balance_info(ifname, wan_entry_idx,  mac, 0 , wan_mark, (op==1)?0:1 , 0);

		if(op == 1)
		{
			rtk_update_save_sta_info(mac, wan_mark);
		}
	}

	return;
}

int get_eth_lan_mac_by_l2(l2_info_T *pHwL2Info, int array_num, int *real_num)
{
	int idx=0;
#if defined(CONFIG_RTK_L34_XPON_PLATFORM)
	int i=0, r=0;
	rtk_l2_addr_table_t l2table;
	do
	{
		bzero(&l2table, sizeof(rtk_l2_addr_table_t));
		i = r;
		if(0 == (rtk_l2_nextValidEntry_get(&r, &l2table)))
		{
			/* Wrap around */
			if(r < i)
			{
				break;
			}
			if(l2table.entryType==RTK_LUT_L2UC){
				if(l2table.entry.l2UcEntry.port < MAX_LAN_PORT_NUM){
					memcpy(pHwL2Info[idx].mac, l2table.entry.l2UcEntry.mac.octet, MAC_ADDR_LEN);
					pHwL2Info[idx].port_id = l2table.entry.l2UcEntry.port;
					idx++;
				}
			}
			r++;
			if(idx >= array_num)
			{
				break;
			}
		}
		else
		{
			break;
		}
	} while(1);
#endif
	if(real_num)
		*real_num=idx;
	return 0;
}

int rtk_find_save_sta_info_wan_mark(int *wan_info_idx, unsigned int *wan_mark, unsigned char *mac)
{
	int i, j = 0, sta_idx = -1 ,sta_cur_cnt = 0;
	MIB_CE_ATM_VC_T wan_entry;

	if(mac == NULL || wan_info_idx == NULL || wan_mark == NULL)
		return 0;

	for(i = 0; i < MAX_SAVE_STA_NUM; i++)
	{
		if(memcmp(save_sta_info[i].mac_str, mac, MAC_ADDR_STRING_LEN) == 0)
		{
			sta_idx = i;
			sta_cur_cnt = save_sta_info[sta_idx].save_cnt;
		}
	}

	if(sta_cur_cnt <= 0)
		return 0;

	for(j = sta_cur_cnt-1 ; j >= 0 ; j--)
	{
		if(rtk_get_wan_entry_by_wanmark(&wan_entry, save_sta_info[sta_idx].save_wan_info[j].save_wan_mark) == 1)
		{

			if(wan_entry.enable > 0 && wan_entry.weight > 0 && rtk_check_wan_is_connected(&wan_entry) && wan_entry.balance_type != 0)
			{
				if(rtk_get_wan_index_by_wanmark(wan_info_idx, save_sta_info[sta_idx].save_wan_info[j].save_wan_mark) == 1)
				{
					*wan_mark = save_sta_info[sta_idx].save_wan_info[j].save_wan_mark;
					return 1;
				}
			}
		}
	}

	return 0;
}

void rtk_process_rules_when_wan_connect()
{
	lanHostInfo_t *pLanNetInfo = NULL;
	unsigned char lan_mac[18] = {0};
	char LanDevName[64] = {0};
	int i, ret, count = 0;

	ret = get_lan_net_info(&pLanNetInfo, &count);
	if(ret != 0)
	{
		if(pLanNetInfo)
			free(pLanNetInfo);
		return;
	}

	if(count>0){
		for(i=0; i<count; i++)
		{
			snprintf(lan_mac,sizeof(lan_mac),"%02x:%02x:%02x:%02x:%02x:%02x", pLanNetInfo[i].mac[0],pLanNetInfo[i].mac[1],pLanNetInfo[i].mac[2],pLanNetInfo[i].mac[3],pLanNetInfo[i].mac[4],pLanNetInfo[i].mac[5]);
			if((strlen(pLanNetInfo[i].ifName) != 0)  &&  isZeroMacstr(lan_mac) == 0)
				rtk_set_load_balance_mac_iptables(pLanNetInfo[i].ifName, lan_mac, 1);
		}
	}

	if(pLanNetInfo)
		free(pLanNetInfo);

	return;

}

void rtk_process_rules_when_wan_link_down(unsigned int wan_mark)
{
	int i, j  = 0 , wan_info_idx = -1;
	uint32_t new_wan_mark = 0;
	unsigned char IpAddr[32] = {0};
	unsigned char lan_if_tmp[IFNAMSIZ];
	unsigned char mac_tmp[MAC_ADDR_STRING_LEN];

	for(i = 0;i < MAX_CON_WAN_NUM; i++)
	{
		if(con_wan[i].wan_mark == wan_mark)
		{
			wan_info_idx = i;
			break;
		}
	}

	if(wan_info_idx == -1)
	{
		//printf("------%s---%s(%d),	ERROR: this wan has no connect info -------\n",__FILE__,__FUNCTION__,__LINE__);
		return;
	}

	for(j = 0; j < MAX_MAC_NUM; j++)
	{
		if(isZeroMacstr(con_wan[wan_info_idx].mac_info[j].mac_str) == 0)
		{
			memcpy(lan_if_tmp, con_wan[wan_info_idx].mac_info[j].ifname, IFNAMSIZ);
			memcpy(mac_tmp, con_wan[wan_info_idx].mac_info[j].mac_str, MAC_ADDR_STRING_LEN);

			//delete current load balance info
			rtk_set_load_balance_mac_iptables(con_wan[wan_info_idx].mac_info[j].ifname, con_wan[wan_info_idx].mac_info[j].mac_str, 0);

			//to find new wan for client connect
			rtk_set_load_balance_mac_iptables(lan_if_tmp, mac_tmp, 1);
		}
	}

	return;
}

int setup_load_balance_rules_when_link_change(char *ifname, unsigned int ifi_flags)
{
	int i, j = 0, l2_mac_num = 0, port_id = -1;
	unsigned char mac_tmp[MAC_ADDR_STRING_LEN];
	l2_info_T l2_mac_info[MAX_L2_MAC_NUM];
	unsigned char ip_tmp[32] = {0}, IpAddr[32] = {0};
	unsigned int wan_port, wan_mark;

	if(ifname == NULL)
		return 0;

	memset(l2_mac_info, 0, sizeof(l2_info_T)*MAX_L2_MAC_NUM);

	for(i = 0; i < SW_LAN_PORT_NUM; i++)
	{
		if(strncmp(ifname, ELANVIF[i], 8)==0)
		{
			port_id = i;
			break;
		}
	}

	get_eth_lan_mac_by_l2(l2_mac_info, MAX_L2_MAC_NUM, &l2_mac_num);

	if((ifi_flags & IFF_UP) && (ifi_flags & IFF_RUNNING))
	{
		for(i = 0; i < l2_mac_num; i++)
		{
			if((port_id != -1) && (l2_mac_info[i].port_id == rtk_port_get_phyPort(port_id)))
			{
				sprintf(mac_tmp, "%02x:%02x:%02x:%02x:%02x:%02x", l2_mac_info[i].mac[0],l2_mac_info[i].mac[1],l2_mac_info[i].mac[2],l2_mac_info[i].mac[3],l2_mac_info[i].mac[4],l2_mac_info[i].mac[5]);

				rtk_set_load_balance_mac_iptables(ifname, mac_tmp , 1);
			}
		}
	}
	else
	{
		//clear port's all rules:include mac and ip
		for(i = 0;i < MAX_CON_WAN_NUM; i++)
		{
			if(con_wan[i].wan_mark)
			{
				for(j = 0; j < MAX_MAC_NUM; j++)
				{
					if( (memcmp(con_wan[i].mac_info[j].ifname, ifname, 8) == 0)  && (isZeroMacstr(con_wan[i].mac_info[j].mac_str) == 0))
					{
						rtk_set_load_balance_mac_iptables(ifname, con_wan[i].mac_info[j].mac_str, 0);
					}
				}
			}
		}

		for(i = 0;i < MAX_CON_WAN_NUM; i++)
		{
			if(port_map_info[i].wan_mark)
			{
				for(j = 0; j < MAX_MAC_NUM; j++)
				{
					if( (memcmp(port_map_info[i].mac_info[j].ifname, ifname, 8) == 0)  && (isZeroMacstr(port_map_info[i].mac_info[j].mac_str) == 0))
					{
						rtk_update_port_map_info(port_map_info[i].mac_info[j].ifname, i, port_map_info[i].mac_info[j].mac_str, 0 , wan_mark, 0 , 0);
					}
				}
			}
		}
	}

	//when wan port link down , update load_balance info
	if(((strncmp(ifname, ALIASNAME_MWNAS, strlen(ALIASNAME_MWNAS)) == 0)
		||(strncmp(ifname, ALIASNAME_PPP, strlen(ALIASNAME_PPP)) == 0))
		&& ((ifi_flags & IFF_UP) && (ifi_flags & IFF_RUNNING)) == 0)
	{
		if(rtk_get_wan_mark_by_wanname(ifname, &wan_mark))
		{
			rtk_process_rules_when_wan_link_down(wan_mark);
		}
	}

	return 0;
}
#endif

#ifdef IP_PASSTHROUGH
void set_IPPT_LAN_access();
#endif

#ifdef CONFIG_CU
int block_wanout_tmp()
{
	// OUTPUT
	va_cmd(EBTABLES, 2, 1, "-N", "wan_out_tmp");
	va_cmd(EBTABLES, 3, 1, "-P", "wan_out_tmp", "RETURN");

	va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "wan_out_tmp", "-o", "nas+", "-j", "DROP");

	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-j", "wan_out_tmp");

	return 0;
}

int block_wanout_tmp_clear()
{
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_OUTPUT, "-j", "wan_out_tmp");
	va_cmd(EBTABLES, 2, 1, "-X", "wan_out_tmp");

	return 0;
}
#endif

#if defined(CONFIG_BLOCK_BRIDGE_WAN_IGMP_TO_IGMPPROXY)

const char BRIDGE_WAN_TO_IGMPPROXY_IGMP_BLOCK[] = "bridge_wan_igmp_block";

int read_proc_value(const char *filename)
{
	FILE *fp;
	int value = 0;

	if ((fp = fopen(filename, "r")) == NULL)
		return -1;
	fscanf(fp, "%d", &value);

	fclose(fp);
	return value;
}


void setup_block_igmp_from_bridge_wan_to_igmpproxy_rules()
{

	int entryNum;
	int i;
	MIB_CE_ATM_VC_T Entry;
	char wanname[MAX_NAME_LEN];
	char procFile[120];

	memset(procFile,0 ,sizeof(procFile));

	va_cmd(EBTABLES, 2, 1, "-N", BRIDGE_WAN_TO_IGMPPROXY_IGMP_BLOCK);
	va_cmd(EBTABLES, 3, 1, "-P", BRIDGE_WAN_TO_IGMPPROXY_IGMP_BLOCK, "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-j", BRIDGE_WAN_TO_IGMPPROXY_IGMP_BLOCK);

	sprintf(procFile, "/sys/devices/virtual/net/%s/bridge/multicast_router",LANIF);


	//check if br0->multicast_router is 2.
	if(read_proc_value(procFile) != 2)
		return;

	//get all bridge wan interfaces
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);

	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if(!Entry.enable)
			continue;

		if(Entry.cmode == CHANNEL_MODE_BRIDGE)
		{
#ifdef CONFIG_RTL_MULTI_ETH_WAN
			snprintf(wanname, 16, "%s%u", ALIASNAME_MWNAS, ETH_INDEX(Entry.ifIndex));
#else
			snprintf(wanname, 16, "%s%u", ALIASNAME_NAS, ETH_INDEX(Entry.ifIndex));
#endif

			va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char *)BRIDGE_WAN_TO_IGMPPROXY_IGMP_BLOCK, "-i", wanname, "-p", "IPv4", "--ip-proto", "igmp", "-j", FW_DROP);

#ifdef CONFIG_IPV6
			va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)BRIDGE_WAN_TO_IGMPPROXY_IGMP_BLOCK, "-i", wanname, "-p", "IPv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "130:132/0:255", "-j", FW_DROP);
			va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)BRIDGE_WAN_TO_IGMPPROXY_IGMP_BLOCK, "-i", wanname, "-p", "IPv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "143:143/0:255", "-j", FW_DROP);
#endif
		}
	}

	return ;
}

void clean_block_igmp_from_bridge_wan_to_igmpproxy_rules(void)
{
	va_cmd(EBTABLES, 4, 1, FW_DEL, FW_FORWARD, "-j", (char *)BRIDGE_WAN_TO_IGMPPROXY_IGMP_BLOCK);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)BRIDGE_WAN_TO_IGMPPROXY_IGMP_BLOCK);
	return;
}

#endif


///--Firewall function add here
#if defined(CONFIG_BOLOCK_IPTV_FROM_LAN_SERVER)
void clean_lan_to_lan_multicast_block_rules(void)
{
	va_cmd(EBTABLES, 4, 1, FW_DEL, FW_FORWARD, "-j", (char *)LAN_TO_LAN_MULTICAST_BLOCK);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)LAN_TO_LAN_MULTICAST_BLOCK);
	va_cmd(EBTABLES, 4, 1, FW_DEL, FW_FORWARD, "-j", (char *)LAN_QUERY_BLOCK);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)LAN_QUERY_BLOCK);
	system("echo disable > /proc/driver/realtek/drop_iptv_from_lan_server");
	return;
}

void setup_lan_to_lan_multicast_block_rules(void)
{
	int total_entry = 0;
	int i, j;
	MIB_CE_ATM_VC_T Entry;
	uint16_t itfGroup;
	char cmd[128] = {0};

	//drop multicast data drom lan port
	va_cmd(EBTABLES, 2, 1, "-N", LAN_TO_LAN_MULTICAST_BLOCK);
	va_cmd(EBTABLES, 3, 1, "-P", LAN_TO_LAN_MULTICAST_BLOCK, "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", LAN_TO_LAN_MULTICAST_BLOCK);
	va_cmd(EBTABLES, 2, 1, "-N", LAN_QUERY_BLOCK);
	va_cmd(EBTABLES, 3, 1, "-P", LAN_QUERY_BLOCK, "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-j", LAN_QUERY_BLOCK);

	if(getOpMode() == GATEWAY_MODE)
	{
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,"eth+","--ip-dport","520","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,"eth+","--ip6-dport","521","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,"eth+","--ip-dport","546:547","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,"eth+","--ip6-dport","546:547","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,"eth+","--ip-dport","1900","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,"eth+","--ip6-dport","1900","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,"eth+","--ip-dport","5353","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,"eth+","--ip6-dport","5353","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,"eth+","-d","01:00:5e:00:00:00/ff:ff:ff:00:00:00","-j","DROP");
		va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,"eth+","-d","33:33:00:00:00:00/ff:ff:00:00:00:00","-j","DROP");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_QUERY_BLOCK,"-p","IPv6","-i" ,"eth+","--ip6-proto","ipv6-icmp","--ip6-icmp-type","130","-j","DROP");
		va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","igmp","-i" ,"eth+","-o","eth+","-j","DROP");

          #ifdef WLAN_SUPPORT
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,"wlan+","--ip-dport","520","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,"wlan+","--ip6-dport","521","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,"wlan+","--ip-dport","546:547","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,"wlan+","--ip6-dport","546:547","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,"wlan+","--ip-dport","1900","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,"wlan+","--ip6-dport","1900","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,"wlan+","--ip-dport","5353","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,"wlan+","--ip6-dport","5353","-j","RETURN");
		va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,"wlan+","-d","01:00:5e:00:00:00/ff:ff:ff:00:00:00","-j","DROP");
		va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,"wlan+","-d","33:33:00:00:00:00/ff:ff:00:00:00:00","-j","DROP");
		va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_QUERY_BLOCK,"-p","IPv6","-i" ,"wlan+","--ip6-proto","ipv6-icmp","--ip6-icmp-type","130","-j","DROP");
		va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","igmp","-i" ,"wlan+","-o","wlan+","-j","DROP");
		va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","igmp","-i" ,"br+","-o","wlan+","-j","DROP");
		va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","igmp","-i" ,"wlan+","-o","br+","-j","DROP");
#endif
		system("echo enable > /proc/driver/realtek/drop_iptv_from_lan_server");
	}
	else if(getOpMode() == BRIDGE_MODE)
	{
		total_entry = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < total_entry; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				continue;

			if(!Entry.enable)
				continue;

#ifdef CONFIG_USER_CTCAPD
			if(Entry.cmode == CHANNEL_MODE_BRIDGE && Entry.iptvwan == 1)
#else
			if(Entry.cmode == CHANNEL_MODE_BRIDGE)
#endif
			{
				system("echo enable > /proc/driver/realtek/drop_iptv_from_lan_server");
				itfGroup	= Entry.itfGroup;
				for(j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j)
				{
					if(BIT_IS_SET(itfGroup, j))
					{
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,ELANVIF[j],"--ip-dport","520","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,ELANVIF[j],"--ip6-dport","521","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,ELANVIF[j],"--ip-dport","546:547","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,ELANVIF[j],"--ip6-dport","546:547","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,ELANVIF[j],"--ip-dport","1900","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,ELANVIF[j],"--ip6-dport","1900","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,ELANVIF[j],"--ip-dport","5353","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,ELANVIF[j],"--ip6-dport","5353","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,ELANVIF[j],"-d","01:00:5e:00:00:00/ff:ff:ff:00:00:00","-j","DROP");
						va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,ELANVIF[j],"-d","33:33:00:00:00:00/ff:ff:00:00:00:00","-j","DROP");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_QUERY_BLOCK,"-p","IPv6","-i" ,ELANVIF[j],"--ip6-proto","ipv6-icmp","--ip6-icmp-type","130","-j","DROP");
						memset(cmd, 0, sizeof(cmd));
						snprintf(cmd, sizeof(cmd), "echo ifname %s > /proc/driver/realtek/drop_iptv_from_lan_server", ELANVIF[j]);
						system(cmd);
					}
				}
#ifdef WLAN_SUPPORT
				for(j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)
				{
					if(BIT_IS_SET(itfGroup, j))
					{
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip-dport","520","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip6-dport","521","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip-dport","546:547","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip6-dport","546:547","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip-dport","1900","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip6-dport","1900","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip-dport","5353","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip6-dport","5353","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"-d","01:00:5e:00:00:00/ff:ff:ff:00:00:00","-j","DROP");
						va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"-d","33:33:00:00:00:00/ff:ff:00:00:00:00","-j","DROP");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_QUERY_BLOCK,"-p","IPv6","-i" ,wlan[j-PMAP_WLAN0],"--ip6-proto","ipv6-icmp","--ip6-icmp-type","130","-j","DROP");
						memset(cmd, 0, sizeof(cmd));
						snprintf(cmd, sizeof(cmd), "echo ifname %s > /proc/driver/realtek/drop_iptv_from_lan_server", wlan[j-PMAP_WLAN0]);
						system(cmd);
					}
				}
#endif
			}
		}
	}
	return;
}
#endif
#ifdef CONFIG_USER_VXLAN
int setupVxLAN_FW_RULES();
#endif
#ifdef CONFIG_USER_GREEN_PLUGIN
static int _is_selfcontrol_chain_exist()
{
	FILE *fp = NULL;
	char buf[256] = {0};
	int ret = 0;

	fp = popen("iptables -nvL FORWARD | grep self_control", "r");
	if(fp==NULL)
		ret = 0;
	else{
		if(fgets(buf, sizeof(buf), fp)!=NULL)
			ret = 1;
		else
			ret = 0;
		pclose(fp);
	}

	return ret;
}

static void _recover_selfcontrol_chain()
{
	va_cmd(IPTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", (char *)SELF_CONTROL_CHAIN_NAME);
}

#ifdef CONFIG_IPV6
static int _is_selfcontrol6_chain_exist()
{
	FILE *fp = NULL;
	char buf[256] = {0};
	int ret = 0;

	fp = popen("ip6tables -nvL FORWARD | grep self_control6", "r");
	if(fp==NULL)
		ret = 0;
	else{
		if(fgets(buf, sizeof(buf), fp)!=NULL)
			ret = 1;
		else
			ret = 0;
		pclose(fp);
	}

	return ret;
}

static void _recover_selfcontrol6_chain()
{
	va_cmd(IP6TABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", (char *)SELF_CONTROL6_CHAIN_NAME);
}
#endif
#endif

void setTmpBlockBridgeFloodPktRule(int act)
{
	char act_str[64];
	if(act == 1)
	{
		snprintf(act_str, sizeof(act_str), "-I");
		printf("[%s %d]temporarily block RA in bridge fowward!\n", __FUNCTION__, __LINE__);
	}
	else if(act == 0)
	{
		snprintf(act_str, sizeof(act_str), "-D");
		printf("[%s %d]restore RA in bridge fowward!\n", __FUNCTION__, __LINE__);
	}
	else
		return;
	va_cmd(EBTABLES, 12, 1, act_str, FW_FORWARD, "-d" , "Multicast", "-p", "IPV6", "--ip6-protocol", "ipv6-icmp", "--ip6-icmp-type", "134", "-j", FW_DROP);

}

void cleanAllFirewallRule(void)
{
#ifdef CONFIG_USER_GREEN_PLUGIN
	int sc_exist = 0;
#ifdef CONFIG_IPV6
	int sc6_exist = 0;
#endif
	sc_exist = _is_selfcontrol_chain_exist();
#ifdef CONFIG_IPV6
	sc6_exist = _is_selfcontrol6_chain_exist();
#endif
#endif
	// Added by Mason Yu. Clean all Firewall rules.
	va_cmd(IPTABLES, 1, 1, "-F");
	// set INPUT policy to ACCEPT to avoid input packet drop
	va_cmd(IPTABLES, 3, 1, "-P", "INPUT", "ACCEPT");
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	va_cmd(EBTABLES, 2, 1, "-F", "INPUT");
	va_cmd(EBTABLES, 2, 1, "-F", "FORWARD");
	va_cmd(EBTABLES, 2, 1, "-F", "OUTPUT");
#else
	va_cmd(EBTABLES, 1, 1, "-F");
#endif
	setTmpBlockBridgeFloodPktRule(1);
#ifdef CONFIG_CU
	setTmpDropBridgeDhcpPktRule(1);
#endif
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", "PREROUTING");
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", "INPUT");
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", "OUTPUT");
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", "POSTROUTING");
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 1, 1, "-F");
	// set INPUT policy to ACCEPT to avoid input packet drop
	va_cmd(IP6TABLES, 3, 1, "-P", "INPUT", "ACCEPT");
	va_cmd(IP6TABLES, 4, 1, "-t", "nat", "-F", "PREROUTING");
	va_cmd(IP6TABLES, 4, 1, "-t", "nat", "-F", "INPUT");
	va_cmd(IP6TABLES, 4, 1, "-t", "nat", "-F", "OUTPUT");
	va_cmd(IP6TABLES, 4, 1, "-t", "nat", "-F", "POSTROUTING");
#endif

	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F",(char *)FW_PREROUTING);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F",(char *)FW_INPUT);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F",(char *)FW_FORWARD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F",(char *)FW_OUTPUT);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F",(char *)FW_POSTROUTING);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F",(char *)FW_PREROUTING);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F",(char *)FW_INPUT);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F",(char *)FW_FORWARD);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F",(char *)FW_OUTPUT);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F",(char *)FW_POSTROUTING);
#endif


#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
	rtk_del_dynamic_firewall();
#endif

#if defined(CONFIG_USER_PPTP_SERVER)
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_PPTP_VPN_IN);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_PPTP_VPN_FW);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)IPTABLE_PPTP_VPN_IN);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)IPTABLE_PPTP_VPN_FW);
#endif
#if defined(CONFIG_USER_L2TPD_LNS)
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_L2TP_VPN_IN);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_L2TP_VPN_FW);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)IPTABLE_L2TP_VPN_IN);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)IPTABLE_L2TP_VPN_FW);
#endif

#ifdef CONFIG_DEV_xDSL
#ifdef CONFIG_USER_IGMPPROXY
	va_cmd(IPTABLES, 2, 1, "-X", (char *)IGMP_OUTPUT_VPRIO_RESET);
#endif
#if (defined(CONFIG_IPV6) && defined(CONFIG_USER_MLDPROXY))
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)MLD_OUTPUT_VPRIO_RESET);
#endif
#endif
#ifdef CONFIG_DEONET_DOS // Deonet GNT2400R
	// iptables -X droplist
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_DROPLIST);
#endif
#ifdef IP_ACL
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_ACL);
	// delete chain(aclblock) on nat
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)FW_ACL);
	// delete chain(aclblock) on nat
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)FW_ACL);
	// iptables -t mangle -D PREROUTING -j aclblock
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", (char *)FW_ACL);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_ACL);
#endif
#endif
#ifdef _CWMP_MIB_
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_CWMP_ACL);
	// delete chain(cwmp_aclblock) on nat
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)FW_CWMP_ACL);
	// iptables -t mangle -D PREROUTING -j cwmp_aclblock
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", (char *)FW_CWMP_ACL);
#endif
#if defined(CONFIG_USER_DHCPCLIENT_MODE) && defined(CONFIG_CMCC) && defined(CONFIG_CMCC_ANDLINK_DEV_SDK)
	va_cmd(IPTABLES, 2, 1, "-X", (char *)REDIRECT_URL_TO_BR0);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)REDIRECT_URL_TO_BR0);
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", (char *)REDIRECT_URL_TO_BR0);
#endif

#ifdef CONFIG_IGMPPROXY_SENDER_IP_ZERO
	MIB_CE_ATM_VC_T Entry;
	unsigned int entryNum, i;
	char ifname[IFNAMSIZ];
#if defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU)
	unsigned char igmpProxyEnable;
	mib_get_s(MIB_IGMP_PROXY, (void *)&igmpProxyEnable, sizeof(igmpProxyEnable));
#endif
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++)	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)){
			printf("error get atm vc entry\n");
			break;
		}
#if defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU)
		if(Entry.enable && Entry.enableIGMP && igmpProxyEnable)
#else
		if(Entry.enable && Entry.enableIGMP)
#endif
		{
			if (ifGetName(Entry.ifIndex, ifname, sizeof(ifname))) {
#if defined(CONFIG_CMCC)
				unsigned char ipoeIGMP;
				mib_get(MIB_PPPOE_WAN_IPOE_IGMP, (void *)&ipoeIGMP);
				if(ipoeIGMP && (Entry.applicationtype & X_CT_SRV_OTHER))
				{
#endif
				if(Entry.cmode == CHANNEL_MODE_PPPOE){
					char wanif[IFNAMSIZ];
					char if_cmd[120];
					ifGetName(PHY_INTF(Entry.ifIndex), wanif, sizeof(wanif));
					snprintf(ifname,sizeof(ifname),wanif);
					va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_DEL, "POSTROUTING","-p","2",
								"-o", ifname, "-j", "SNAT","--to-source", "0.0.0.0");
				}
#if defined(CONFIG_CMCC)
				}
#endif
			}
		}
	}
#endif //CONFIG_IGMPPROXY_SENDER_IP_ZERO

#if defined(NAT_CONN_LIMIT) || defined(TCP_UDP_CONN_LIMIT)
	va_cmd(IPTABLES, 2, 1, "-X", "connlimit");
#endif

	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_BR_WAN);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_BR_WAN_FORWARD);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_BR_LAN_FORWARD);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_BR_WAN_OUT);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_BR_LAN_OUT);
#if defined(CONFIG_00R0) && defined(CONFIG_USER_INTERFACE_GROUPING)
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_INTERFACE_GROUP_DROP_IGMP);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_INTERFACE_GROUP_DROP_FORWARD);
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_INTERFACE_GROUP_DROP_FORWARD);
#endif
#ifdef PPPOE_PASSTHROUGH
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", FW_BR_PPPOE);
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_BR_PPPOE);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_BR_PPPOE);
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_BR_PPPOE_ACL);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_BR_PPPOE_ACL);
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
	va_cmd(IPTABLES, 2, 1, "-X", "domainblk");
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)MARK_FOR_DOMAIN);
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", (char *)MARK_FOR_DOMAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", (char *)MARK_FOR_DOMAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-X", "domainblk");
#endif
#endif
#ifdef CONFIG_ANDLINK_SUPPORT
	va_cmd(IPTABLES, 2, 1, "-X", "andlink");
#endif
#ifdef PORT_FORWARD_GENERAL
	va_cmd(IPTABLES, 2, 1, "-X", (char *)PORT_FW);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)PORT_FW);
#ifdef NAT_LOOPBACK
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)PORT_FW_PRE_NAT_LB);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)PORT_FW_POST_NAT_LB);
#endif
#endif

#ifdef DMZ
	va_cmd(IPTABLES, 2, 1, "-X", (char *)IPTABLE_DMZ);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)IPTABLE_DMZ);
#endif

#ifdef DMZ
#ifdef NAT_LOOPBACK
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)IPTABLE_DMZ_PRE_NAT_LB);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)IPTABLE_DMZ_POST_NAT_LB);
#endif
#endif

//#if defined(CONFIG_USER_BOA_PRO_PASSTHROUGH) && defined(CONFIG_RTK_DEV_AP)
#ifdef CONFIG_USER_BOA_PRO_PASSTHROUGH
    va_cmd(IPTABLES, 2, 1, "-X", (char *)L2TP_PASSTHROUGH_FORWARD);
    va_cmd(IPTABLES, 2, 1, "-X", (char *)L2TP_PASSTHROUGH_FORWARD);
    va_cmd(IPTABLES, 2, 1, "-X", (char *)L2TP_PASSTHROUGH_FORWARD);
#endif

#ifdef NATIP_FORWARDING
	va_cmd(IPTABLES, 2, 1, "-X", (char *)IPTABLE_IPFW);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)IPTABLE_IPFW);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)IPTABLE_IPFW2);
#endif

#ifdef PORT_FORWARD_ADVANCE
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_PPTP);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)FW_PPTP);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_L2TP);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)FW_L2TP);
#endif

#if defined(PORT_TRIGGERING_STATIC) || defined(PORT_TRIGGERING_DYNAMIC)
	va_cmd(IPTABLES, 2, 1, "-X", (char *)IPTABLES_PORTTRIGGER);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)IPTABLES_PORTTRIGGER);
#endif

#ifdef REMOTE_ACCESS_CTL
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_INACC);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_INACC);
#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_ICMP_FILTER);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_ICMP_FILTER);
#endif
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_IN_COMMING);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_IN_COMMING);
#endif // CONFIG_IPV6
#endif // CONFIG_YUEME
#endif // REMOTE_ACCESS_CTL

	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_DHCP_PORT_FILTER);
#ifdef IP_PORT_FILTER
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_IPFILTER_IN);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_IPFILTER_OUT);
#else
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_IPFILTER);
#endif
#endif

#ifdef CONFIG_USER_RTK_VOIP
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-p", "udp", "-j", VOIP_REMARKING_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", VOIP_REMARKING_CHAIN);
#if defined(CONFIG_IPV6)
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-p", "udp", "-j", VOIP_REMARKING_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", VOIP_REMARKING_CHAIN);
#endif
#endif

#if (defined(IP_PORT_FILTER) || defined(DMZ)) && defined(CONFIG_TELMEX_DEV)
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_DMZFILTER);
#endif

#ifdef PARENTAL_CTRL
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_PARENTAL_CTRL);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_PARENTAL_CTRL);
#endif
#endif

#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_IPV6FILTER);
#endif


#if defined(CONFIG_BOLOCK_IPTV_FROM_LAN_SERVER)
	//drop multicast data drom lan port
	clean_lan_to_lan_multicast_block_rules();
#endif
#if defined(CONFIG_BLOCK_BRIDGE_WAN_IGMP_TO_IGMPPROXY)
	// drop igmp from bridge wan when br0->multicast_router is 2.
	clean_block_igmp_from_bridge_wan_to_igmpproxy_rules();
#endif

#if defined(CONFIG_USER_PORT_ISOLATION)
	rtk_lan_clear_lan_port_isolation();
#endif

#if defined(CONFIG_CU_BASEON_YUEME) && defined(COM_CUC_IGD1_ParentalControlConfig)
    va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_INPUT, "-j", (char *)FW_PARENTAL_MAC_CTRL_LOCALIN);
    va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_PARENTAL_MAC_CTRL_LOCALIN);
    va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)FW_PARENTAL_MAC_CTRL_FWD);
    va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_PARENTAL_MAC_CTRL_FWD);
#endif

#ifdef MAC_FILTER
	//va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_MACFILTER_FWD);
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)FW_MACFILTER_FWD);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_MACFILTER_FWD);
	va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", (char *)FW_MACFILTER_MC);
	va_cmd(EBTABLES, 4, 1, "-t", "nat", "-X", (char *)FW_MACFILTER_MC);
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_INPUT, "-j", (char *)FW_MACFILTER_LI);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_MACFILTER_LI);
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_OUTPUT, "-j", (char *)FW_MACFILTER_LO);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_MACFILTER_LO);
#if defined(CONFIG_E8B)
	va_cmd(IPTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)FW_MACFILTER_FWD_L3);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_MACFILTER_FWD_L3);
#endif
#ifdef CONFIG_00R0
	va_cmd(IPTABLES, 2, 1, "-X", "macfilter");
#endif
#endif

#ifdef SUPPORT_ACCESS_RIGHT
	apply_accessRight(0);
#endif

#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", "WebRedirect_Wizard");
	unlink("/tmp/WebRedirectChain");
#endif

#if defined(URL_BLOCKING_SUPPORT) || defined(URL_ALLOWING_SUPPORT)
	va_cmd(IPTABLES, 2, 1, "-X", WEBURL_CHAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-X", WEBURL_CHAIN);
#endif
#endif

#ifdef WEB_REDIRECT_BY_MAC
	va_cmd(IPTABLES, 2, 1, "-t", "nat", "-X", "WebRedirectByMAC");
#endif
#ifdef SUPPORT_WEB_REDIRECT
	va_cmd(IPTABLES, 2, 1, "-t", "nat", "-X", "WebRedirectReg");
#endif
#if 0
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	va_cmd(IPTABLES, 2, 1, "-t", "nat", "-X", "CaptivePortal");
#endif
#endif

#ifdef CONFIG_IPV6
	do{
		unsigned char mib_auto_mac_binding_iptv_bridge_wan_mode=0;
		if(mib_get(MIB_AUTO_MAC_BINDING_IPTV_BRIDGE_WAN,&mib_auto_mac_binding_iptv_bridge_wan_mode)&&mib_auto_mac_binding_iptv_bridge_wan_mode==2){
			va_cmd(IP6TABLES, 2, 1, "-X", (char *)IPTABLE_IPV6_MAC_AUTOBINDING_RS);
		}
	}while(0);
#endif

#if defined(CONFIG_USER_CWMP_TR069)
	va_cmd(IPTABLES, 2, 1, "-X", IPTABLE_TR069);
#endif
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-X", IPTABLE_IPV6_TR069);
#endif

#if defined(CONFIG_USER_DATA_ELEMENTS_AGENT) || defined(CONFIG_USER_DATA_ELEMENTS_COLLECTOR)
	va_cmd(IPTABLES, 2, 1, "-X", IPTABLE_DATA_ELEMENT);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-X", IP6TABLE_DATA_ELEMENT);
#endif
#endif

#ifdef REMOTE_ACCESS_CTL
	filter_set_remote_access(0);
#endif

#ifdef CONFIG_SUPPORT_CAPTIVE_PORTAL
	va_cmd(IPTABLES, 2, 1, "-X", FW_CAPTIVEPORTAL_FILTER);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-X", FW_CAPTIVEPORTAL_FILTER);
#endif
#endif

#ifdef CONFIG_USER_FON
	va_cmd(IPTABLES, 2, 1, "-X", "fongw");
	va_cmd(IPTABLES, 2, 1, "-X", "fongw_fwd");
#endif

#if defined(CONFIG_USER_RTK_NAT_ALG_PASS_THROUGH) || (defined(CONFIG_TRUE) && defined(CONFIG_IP_NF_ALG_ONOFF))
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_VPNPASSTHRU);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_VPNPASSTHRU);
#endif
#endif
#if (defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	unsigned int pon_mode = 0;
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
	if(pon_mode==GPON_MODE)
	{
		va_cmd(EBTABLES, 2, 1, "-X", L2_PPTP_PORT_SERVICE);
		va_cmd(EBTABLES, 2, 1, "-X", L2_PPTP_PORT_INPUT_TBL);
		va_cmd(EBTABLES, 2, 1, "-X", L2_PPTP_PORT_OUTPUT_TBL);
		va_cmd(EBTABLES, 2, 1, "-X", L2_PPTP_PORT_FORWARD_TBL);
		va_cmd(EBTABLES, 2, 1, "-X", L2_PPTP_PORT_FORWARD);
	}
#endif
	va_cmd(EBTABLES, 2, 1, "-X", FW_ISOLATION_MAP);
	va_cmd(EBTABLES, 2, 1, "-X", FW_ISOLATION_BRPORT);
#ifdef CONFIG_USER_WIRED_8021X
	rtk_wired_8021x_clean(0);
#endif
#ifdef CONFIG_USER_CTCAPD
	va_cmd(EBTABLES, 2, 1, "-F",WIFI_PERMIT_IN_TBL);
	va_cmd(EBTABLES, 4, 1, "-D", "INPUT", "-j", WIFI_PERMIT_IN_TBL);
	va_cmd(EBTABLES, 2, 1, "-X",WIFI_PERMIT_IN_TBL);
	va_cmd(EBTABLES, 2, 1, "-F",WIFI_PERMIT_FWD_TBL);
	va_cmd(EBTABLES, 4, 1, "-D", "FORWARD", "-j", WIFI_PERMIT_FWD_TBL);
	va_cmd(EBTABLES, 2, 1, "-X",WIFI_PERMIT_FWD_TBL);
#endif

#ifdef CONFIG_RTK_DNS_TRAP
	char skbmark_buf[64]={0};

#ifdef CONFIG_RTK_SKB_MARK2

	snprintf(skbmark_buf, sizeof(skbmark_buf), "0x%llx", (1<<SOCK_MARK2_FORWARD_BY_PS_START));

	//iptables -t mangle -D PREROUTING -p udp --dport 53 -j MARK2 --or-mark 0x1
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);
	//iptables -t mangle -D PREROUTING -p udp --sport 53 -j MARK2 --or-mark 0x1
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -D PREROUTING -p udp --dport 53 -j MARK2 --or-mark 0x1
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -D PREROUTING -p udp --sport 53 -j MARK2 --or-mark 0x1
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);

#else

	snprintf(skbmark_buf, sizeof(skbmark_buf), "0x%x", (1<<SOCK_MARK_METER_INDEX_END));

	//iptables -t mangle -D PREROUTING -p udp --dport 53 -j MARK --or-mark 0x80000000
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);
	//iptables -t mangle -D PREROUTING -p udp --sport 53 -j MARK --or-mark 0x80000000
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -D PREROUTING -p udp --dport 53 -j MARK --or-mark 0x80000000
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -D PREROUTING -p udp --sport 53 -j MARK --or-mark 0x80000000
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);

#endif
#endif

#if defined(URL_BLOCKING_SUPPORT)||defined(URL_ALLOWING_SUPPORT)
	va_cmd(IPTABLES, 16, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-i", BRIF, "-p", "tcp", "--tcp-flags", "SYN,FIN,RST", "NONE", "-m", "length", "!", "--length", "0:84", "-j", WEBURL_CHAIN);
	va_cmd(IPTABLES, 2, 1, "-F", WEBURL_CHAIN);
	va_cmd(IPTABLES, 2, 1, "-X", WEBURL_CHAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 16, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-i", BRIF, "-p", "tcp", "--tcp-flags", "SYN,FIN,RST", "NONE", "-m", "length", "!", "--length", "0:104", "-j", WEBURL_CHAIN);
	va_cmd(IP6TABLES, 2, 1, "-F", WEBURL_CHAIN);
	va_cmd(IP6TABLES, 2, 1, "-X", WEBURL_CHAIN);
#endif
	va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-i", BRIF, "-p", "tcp", "-j", WEBURLPRE_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", WEBURLPRE_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", WEBURLPRE_CHAIN);
	va_cmd(IPTABLES, 11, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "!", "-o", BRIF, "-p", "tcp", "-j", WEBURLPOST_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", WEBURLPOST_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", WEBURLPOST_CHAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 10, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-i", BRIF, "-p", "tcp", "-j", WEBURLPRE_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", WEBURLPRE_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", WEBURLPRE_CHAIN);
	va_cmd(IP6TABLES, 11, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "!", "-o", BRIF, "-p", "tcp", "-j", WEBURLPOST_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", WEBURLPOST_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", WEBURLPOST_CHAIN);
#endif
#endif

#ifdef _PRMT_X_CMCC_SECURITY_
	va_cmd(IPTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", PARENTALCTRL_CHAIN);
	va_cmd(IPTABLES, 2, 1, "-F", PARENTALCTRL_CHAIN);
	va_cmd(IPTABLES, 2, 1, "-X", PARENTALCTRL_CHAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", PARENTALCTRL_CHAIN);
	va_cmd(IP6TABLES, 2, 1, "-F", PARENTALCTRL_CHAIN);
	va_cmd(IP6TABLES, 2, 1, "-X", PARENTALCTRL_CHAIN);
#endif
	va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-i", BRIF, "-p", "tcp", "-j", PARENTALCTRLPRE_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", PARENTALCTRLPRE_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", PARENTALCTRLPRE_CHAIN);
	va_cmd(IPTABLES, 11, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "!", "-o", BRIF, "-p", "tcp", "-j", PARENTALCTRLPOST_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", PARENTALCTRLPOST_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", PARENTALCTRLPOST_CHAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 10, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-i", BRIF, "-p", "tcp", "-j", PARENTALCTRLPRE_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", PARENTALCTRLPRE_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", PARENTALCTRLPRE_CHAIN);
	va_cmd(IP6TABLES, 11, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "!", "-o", BRIF, "-p", "tcp", "-j", PARENTALCTRLPOST_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", PARENTALCTRLPOST_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", PARENTALCTRLPOST_CHAIN);
#endif
#endif

#ifdef CONFIG_CU
	block_wanout_tmp();
#endif

#ifdef CONFIG_USER_LOCK_NET_FEATURE_SUPPORT
	rtk_clean_net_locked_rule();
#endif

#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	rtk_del_manual_firewall();
#endif
#if defined(BLOCK_LAN_MAC_FROM_WAN)
	set_lanhost_block_from_wan(1);
#endif
#ifdef BLOCK_INVALID_SRCIP_FROM_WAN
	Block_Invalid_SrcIP_From_WAN(1);
#endif

#ifdef CONFIG_USER_GREEN_PLUGIN
	//secure router plugin only recover the rules in its chain, and does not recover its chain in FORWARD chain.
	if(sc_exist)
		_recover_selfcontrol_chain();
#ifdef CONFIG_IPV6
	if(sc6_exist)
		_recover_selfcontrol6_chain();
#endif
#endif

#ifdef _PRMT_X_CT_COM_WANEXT_
	rtk_fw_set_ipforward(0);
#endif

#ifdef CONFIG_USER_MAP_E
	mape_flush_firewalls();
#endif

#if (defined(CONFIG_CMCC) && defined(CONFIG_APACHE_FELIX_FRAMEWORK)) || defined(CONFIG_CU)
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_cmcc_flush_traffic_mirror_rule();
#ifdef CONFIG_CMCC_TRAFFIC_PROCESS_RULE_SUPPORT
	rtk_cmcc_flush_traffic_process_rule();
#endif //CONFIG_CMCC_TRAFFIC_PROCESS_RULE_SUPPORT
	rtk_cmcc_flush_traffic_qos_rule();
#endif
#endif

#ifdef CONFIG_USER_IP_QOS
	setupQosRuleMainChain(0);
#endif

}

#if defined(CONFIG_00R0) && defined(CONFIG_USER_INTERFACE_GROUPING)
int block_interfce_group_default_rule()
{
	int total=0, itfNum = 0;;
	int i, dgw_ifindex=0, igmp_proxy_ifindex=0, mld_proxy_ifindex=0, dgw_found=0, igmp_found=0, mld_found=0;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ] = {0},  dgw_ifname[IFNAMSIZ] = {0}, igmp_ifname[IFNAMSIZ] = {0}, mld_ifname[IFNAMSIZ] = {0};
	char lan_ifname[IFNAMSIZ] = {0};
	struct layer2bridging_availableinterface_info itfList[MAX_NUM_OF_ITFS];
	MIB_CE_SW_PORT_T swEntry={0};
	unsigned int show_domain = 0;

	va_cmd(EBTABLES, 2, 1, "-N", FW_INTERFACE_GROUP_DROP_IGMP);
	va_cmd(EBTABLES, 3, 1, "-P", FW_INTERFACE_GROUP_DROP_IGMP, "RETURN");
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_INTERFACE_GROUP_DROP_FORWARD);
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_INTERFACE_GROUP_DROP_FORWARD);

	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", FW_INTERFACE_GROUP_DROP_IGMP);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_INTERFACE_GROUP_DROP_FORWARD);
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_INTERFACE_GROUP_DROP_FORWARD);

	// check all wan interfaces
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if(!Entry.enable)
			continue;

		ifGetName(Entry.ifIndex, ifname, sizeof(ifname));
		if(Entry.cmode != CHANNEL_MODE_BRIDGE)
		{
			if(Entry.dgw) {
				dgw_ifindex = Entry.ifIndex;
				strcpy(dgw_ifname, ifname);
			}
			if(Entry.enableIGMP) {
				igmp_proxy_ifindex = Entry.ifIndex;
				strcpy(igmp_ifname, ifname);
			}
			if(Entry.enableMLD) {
				mld_proxy_ifindex = Entry.ifIndex;
				strcpy(mld_ifname, ifname);
			}
		}
	}

	show_domain = INTFGRPING_DOMAIN_WANROUTERCONN;
	itfNum = rtk_layer2bridging_get_availableinterface(itfList, MAX_NUM_OF_ITFS, show_domain, (-1));
	for (i = 0; i < itfNum; i++)
	{
		if (itfList[i].itfGroup != 0) continue;
		if (itfList[i].ifid == dgw_ifindex)
			dgw_found = 1;
		if (itfList[i].ifid == igmp_proxy_ifindex)
			igmp_found = 1;
		if (itfList[i].ifid == mld_proxy_ifindex)
			mld_found = 1;
	}

	show_domain = (INTFGRPING_DOMAIN_ELAN | INTFGRPING_DOMAIN_WLAN);
	itfNum = rtk_layer2bridging_get_availableinterface(itfList, MAX_NUM_OF_ITFS, show_domain, (-1));
	for (i = 0; i < itfNum; i++)
	{
		if (itfList[i].itfGroup != 0) continue;
		if (itfList[i].ifdomain == INTFGRPING_DOMAIN_ELAN)
			snprintf(lan_ifname, sizeof(lan_ifname), "%s", ELANVIF[itfList[i].ifid]);
		else
			snprintf(lan_ifname, sizeof(lan_ifname), "%s", itfList[i].name);

		//Block routing wan access from default group without default gw
		//iptables -I if_group_drop_forward -m physdev --physdev-in eth0.5.0 -o ppp0 -j DROP
		if (!dgw_found && dgw_ifindex!=0) {
			 va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_INTERFACE_GROUP_DROP_FORWARD, "-m", "physdev", "--physdev-in",
				 lan_ifname, "-o",  (char* )dgw_ifname, "-j", (char *)FW_DROP);
			 va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_INTERFACE_GROUP_DROP_FORWARD, "-m", "physdev", "--physdev-in",
				 lan_ifname, "-o",  (char* )dgw_ifname, "-j", (char *)FW_DROP);
		}
		//Block IGMP in default group without igmpproxy's WAN
		//ebtables -I if_group_drop_igmp -p IPv4 -i eth0.5.0 --ip-proto igmp -j DROP
		if (!igmp_found && igmp_proxy_ifindex!=0)
			va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, FW_INTERFACE_GROUP_DROP_IGMP, "-p", "IPv4", "--ip-proto", "igmp",
				"-i", lan_ifname, "-j", "DROP");

		//Block MLD in default group without mldproxy's WAN
		//eebtables -I if_group_drop_igmp -p IPv6 --ip6-proto ipv6-icmp --ip6-icmp-type 130:132 -i eth0.5.0 -j DROP
		if (!mld_found && mld_proxy_ifindex!=0)
			va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, FW_INTERFACE_GROUP_DROP_IGMP, "-p", "IPv6", "--ip6-proto",
				"ipv6-icmp", "--ip6-icmp-type" , "130:132","-i", lan_ifname, "-j", "DROP");
	}

	return 0;
}
#endif

int block_br_wan()
{
	int total;
	int i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ + 2] = {0};

	// INPUT
	va_cmd(EBTABLES, 2, 1, "-N", FW_BR_WAN);
	va_cmd(EBTABLES, 3, 1, "-P", FW_BR_WAN, "RETURN");
	// OUTPUT
	va_cmd(EBTABLES, 2, 1, "-N", FW_BR_WAN_OUT);
	va_cmd(EBTABLES, 3, 1, "-P", FW_BR_WAN_OUT, "RETURN");
	va_cmd(EBTABLES, 2, 1, "-N", FW_BR_LAN_OUT);
	va_cmd(EBTABLES, 3, 1, "-P", FW_BR_LAN_OUT, "RETURN");

	// check all wan interfaces
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if(!Entry.enable)
			continue;

		ifGetName(PHY_INTF(Entry.ifIndex), ifname, sizeof(ifname));
		if (ROUTE_CHECK_BRIDGE_MIX_ENABLE(&Entry))
			strncat(ifname, ROUTE_BRIDGE_WAN_SUFFIX, sizeof(ifname) - strlen(ifname) - 1);

		if((Entry.cmode == CHANNEL_MODE_BRIDGE)
				|| ROUTE_CHECK_BRIDGE_MIX_ENABLE(&Entry)
		)
		{
#if defined(CONFIG_USER_DHCPCLIENT_MODE)
			char dhcp_mode = 0;
			mib_get_s(MIB_DHCP_MODE, (void *)&dhcp_mode, sizeof(dhcp_mode));
			if(dhcp_mode != DHCPV4_LAN_CLIENT)
#endif
			{
#ifdef USE_DEONET /* sghong, 20230621, used in BRIDGE mode. */
#if  0 /* sghong, 20231109, not used */
				/* IPv4 icmp accept */
				va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_WAN, "-i", ifname, "-p", "arp", "-j", "ACCEPT");
				va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_WAN_OUT, "-o", ifname, "-p", "arp", "-j", "ACCEPT");
				va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, FW_BR_WAN, "-i", ifname, "-p", "ipv4", "--ip-proto", "icmp", "-j", "ACCEPT");
				va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, FW_BR_WAN_OUT, "-o", ifname, "-p", "ipv4", "--ip-proto", "icmp", "-j", "ACCEPT");
				/* IPv6 icmp accept */
				va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, FW_BR_WAN, "-i", ifname, "-p", "ip6", "--ip6-protocol", "ipv6-icmp", "--ip6-icmp-type", "128:137","-j", "ACCEPT");
				va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, FW_BR_WAN_OUT, "-o", ifname, "-p", "ip6", "--ip6-protocol", "ipv6-icmp", "--ip6-icmp-type", "128:137","-j", "ACCEPT");

#endif
#else
				/*
				 * -i nas0_0 -j DROP,  INPUT  chain
				 * -o nas0_0 -j DROP,  OUTPUT chain
				 */
				va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_BR_WAN, "-i", ifname, "-j", "DROP");
				va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_BR_WAN_OUT, "-o", ifname, "-j", "DROP");
#endif
			}
		}
	}

	// add to INPUT chain
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", FW_BR_WAN);
	// add to OUTPUT chain
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-j", FW_BR_WAN_OUT);
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-j", FW_BR_LAN_OUT);

	return 0;
}

#ifdef CONFIG_IPV6
int block_br_wan_forward()
{
	int total;
	int i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ] = {0};
	int have_br_wan=0;

	// create chain
	va_cmd(EBTABLES, 2, 1, "-N", FW_BR_WAN_FORWARD);
	va_cmd(EBTABLES, 3, 1, "-P", FW_BR_WAN_FORWARD, "RETURN");

	// check all wan interfaces
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if(!Entry.enable)
			continue;

		ifGetName(PHY_INTF(Entry.ifIndex), ifname, sizeof(ifname));

		if(Entry.cmode == CHANNEL_MODE_BRIDGE && Entry.IpProtocol == IPVER_IPV4) // ipv4 only, drop ipv6
		{
			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_WAN_FORWARD, "-p", "ipv6", "-i", ifname, "-j", "DROP");
			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_WAN_FORWARD, "-p", "ipv6", "-o", ifname, "-j", "DROP");
		}
		else if(Entry.cmode == CHANNEL_MODE_BRIDGE && Entry.IpProtocol == IPVER_IPV6) // ipv6 only, drop ipv4
		{
			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_WAN_FORWARD, "-p", "ipv4", "-i", ifname, "-j", "DROP");
			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_WAN_FORWARD, "-p", "ipv4", "-o", ifname, "-j", "DROP");
		}
		else
			printf("[%s:%d] Entry.cmode =%d, Entry.IpProtocol=%d\n", __func__, __LINE__, Entry.cmode, Entry.IpProtocol);

		if(Entry.cmode == CHANNEL_MODE_BRIDGE)
		{
			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_WAN_FORWARD, "-i", ifname, "-o", "nas0+", "-j", "DROP");
			have_br_wan=1;
		}

	}
#if defined(CONFIG_YUEME) //drop bridge wan arp with lan side dst/src ip
	if(have_br_wan)
	{
		unsigned char tmpBuf[100]={0};

		mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)tmpBuf, sizeof(tmpBuf));

		//ebtables -I br_wan_forward -o nas0+ -p 0x0806 --arp-ip-dst 192.168.1.1/24 -j DROP
		va_cmd(EBTABLES, 10, 1, (char *)FW_INSERT, FW_BR_WAN_FORWARD, "-o", "nas0+", "-p", "0x0806", "--arp-ip-dst", get_lan_ipnet(), "-j", "DROP");//drop v4 lan ip from wan
		va_cmd(EBTABLES, 10, 1, (char *)FW_INSERT, FW_BR_WAN_FORWARD, "-i", "nas0+", "-p", "0x0806", "--arp-ip-src", get_lan_ipnet(), "-j", "DROP");//drop v4 lan ip from wan
		//ebtables -I br_wan_forward -i nas0+ -p ip6 --ip6-source fe80::1 --ip6-protocol ipv6-icmp --ip6-icmp-type 135:136 -j DROP
		va_cmd(EBTABLES, 14, 1, (char *)FW_INSERT, FW_BR_WAN_FORWARD, "-i", "nas0+", "-p", "ip6",  "--ip6-src", tmpBuf, "--ip6-protocol", "ipv6-icmp", "--ip6-icmp-type", "135:136","-j", "DROP");//drop v6 lan ip from wan
	}
#endif
	// add to INPUT chain
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", FW_BR_WAN_FORWARD);

	return 0;
}
#endif

#ifdef USE_DEONET /* sghong, 20230406 */
int deo_httpdRedirect(void)
{
	/* iptables -t nat -I portfw -i br0 -d 192.168.45.1 -p tcp --dport 80 -j DNAT --to 192.168.75.1   */
	va_cmd(IPTABLES, 16, 1, "-t", "nat", (char *)FW_DEL, PORT_FW, "-i", "br0", "-d", "192.168.45.1", "-p", "tcp", "--dport", "80", "-j", "DNAT", "--to", "192.168.75.1");

	va_cmd(IPTABLES, 16, 1, "-t", "nat", (char *)FW_INSERT, PORT_FW, "-i", "br0", "-d", "192.168.45.1", "-p", "tcp", "--dport", "80", "-j", "DNAT", "--to", "192.168.75.1");

	return 0;
}

int deo_dhcpLanToLanBlock(void)
{
	/*
	 * ebtables -D FORWARD -p IPv4 -i eth+ -o eth+ --ip-proto 17 --ip-dport 67 -j DROP
	 * ebtables -A FORWARD -p IPv4 -i eth+ -o eth+ --ip-proto 17 --ip-dport 67 -j DROP
	 */
	va_cmd(EBTABLES, 14, 1, (char *)FW_DEL, (char*)FW_BR_LAN_FORWARD, "-p", "IPv4", "-i", "eth+", "-o", "eth+", "--ip-proto", "udp", "--ip-dport", "67", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, (char *)FW_ADD, (char*)FW_BR_LAN_FORWARD, "-p", "IPv4", "-i", "eth+", "-o", "eth+", "--ip-proto", "udp", "--ip-dport", "67", "-j", "DROP");

	return 0;
}

int deo_privateDHCPServerPacketBlock(void)
{
	/*
	 * ebtables -D INPUT -p ipv4 --ip-proto 17 --ip-dport 68 -i eth+ -j DROP
	 * ebtables -I INPUT -p ipv4 --ip-proto 17 --ip-dport 68 -i eth+ -j DROP
	 * ebtables -D FORWARD -p ipv4 --ip-proto 17 --ip-dport 68 -i eth+ -j DROP
	 * ebtables -A FORWARD -p ipv4 --ip-proto 17 --ip-dport 68 -i eth+ -j DROP
	 */
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_PRIVATE_DHCP_SERVER_INPUT, "-p", "IPv4", "-i", "eth+", "--ip-proto", "udp", "--ip-dport", "68", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_PRIVATE_DHCP_SERVER_INPUT, "-p", "IPv4", "-i", "eth+", "--ip-proto", "udp", "--ip-dport", "68", "-j", "DROP");

	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_LAN_FORWARD, "-p", "IPv4", "-i", "eth+", "--ip-proto", "udp", "--ip-dport", "68", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_LAN_FORWARD, "-p", "IPv4", "-i", "eth+", "--ip-proto", "udp", "--ip-dport", "68", "-j", "DROP");

	return 0;
}

int deo_ebtables_dropRuleSet(int wan_mode, int ipv6_en)
{
	if (wan_mode == DEO_NAT_MODE)
	{
		/*
		   ebtables -A br_wan -p ARP -i nas0_1 -j DROP
		   ebtables -A br_wan -p IPv4 -i nas0_1 -j DROP
		   ebtables -A br_wan_out -p ARP -o nas0_1 -j DROP
		   ebtables -A br_wan_out -p IPv4 -o nas0_1 -j DROP
		*/
		va_cmd(EBTABLES, 8, 1, (char *)FW_DEL, (char*)FW_BR_WAN, "-p", "ARP", "-i", "nas0_1", "-j", "DROP");
		va_cmd(EBTABLES, 8, 1, (char *)FW_DEL, (char*)FW_BR_WAN, "-p", "IPv4", "-i", "nas0_1", "-j", "DROP");
		va_cmd(EBTABLES, 8, 1, (char *)FW_DEL, (char*)FW_BR_WAN_OUT, "-p", "ARP", "-i", "nas0_1", "-j", "DROP");
		va_cmd(EBTABLES, 8, 1, (char *)FW_DEL, (char*)FW_BR_WAN_OUT, "-p", "IPv4", "-i", "nas0_1", "-j", "DROP");

		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char*)FW_BR_WAN, "-p", "ARP", "-o", "nas0_1", "-j", "DROP");
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char*)FW_BR_WAN, "-p", "IPv4", "-o", "nas0_1", "-j", "DROP");
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char*)FW_BR_WAN_OUT, "-p", "ARP", "-o", "nas0_1", "-j", "DROP");
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char*)FW_BR_WAN_OUT, "-p", "IPv4", "-o", "nas0_1", "-j", "DROP");
	}
	else
	{
		/*
		 * ebtables -I br_wan_out -p IPv4 -o nas0+ --ip-dst 192.168.75.0/24 -j DROP
		 * ebtables -I br_wan -i nas0+ -p 0x0806 --arp-ip-src 192.168.75.0/24 -j DROP
		 * ebtables -I br_wan_forward -o nas0+ -p 0x0806 --arp-ip-dst 192.168.75.0/24 -j DROP
		 * ebtables -I br_wan_out -o nas0+ -p 0x0806 --arp-ip-dst 192.168.75.0/24 -j DROP
		 * */

		va_cmd(EBTABLES, 10, 1, (char *)FW_DEL, (char*)FW_BR_WAN_OUT, "-p", "IPv4", "-o", "nas0+", "--ip-dst", "192.168.75.0/24", "-j", "DROP");
		va_cmd(EBTABLES, 10, 1, (char *)FW_DEL, (char*)FW_BR_WAN, "-i", "nas0+", "-p", "0x0806", "--arp-ip-src", "192.168.75.0/24", "-j", "DROP");
		va_cmd(EBTABLES, 10, 1, (char *)FW_DEL, (char*)FW_BR_WAN_FORWARD, "-o", "nas0+", "-p", "0x0806", "--arp-ip-dst", "192.168.75.0/24", "-j", "DROP");
		va_cmd(EBTABLES, 10, 1, (char *)FW_DEL, (char*)FW_BR_WAN_OUT, "-o", "nas0+", "p", "0x0806", "--arp-ip-dst", "192.168.75.0/24", "-j", "DROP");

		va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char*)FW_BR_WAN_OUT, "-p", "IPv4", "-o", "nas0+", "--ip-dst", "192.168.75.0/24", "-j", "DROP");
		va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char*)FW_BR_WAN, "-i", "nas0+", "-p", "0x0806", "--arp-ip-src", "192.168.75.0/24", "-j", "DROP");
		va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char*)FW_BR_WAN_FORWARD, "-o", "nas0+", "-p", "0x0806", "--arp-ip-dst", "192.168.75.0/24", "-j", "DROP");
		va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char*)FW_BR_WAN_OUT, "-o", "nas0+", "-p", "0x0806", "--arp-ip-dst", "192.168.75.0/24", "-j", "DROP");
	}

	if (ipv6_en == 1)
	{
		/*
		   ebtables -A br_wan_out -p IPv6 -o eth+ --ip6-proto 17 --ip6-sport 546 -j DROP
		   ebtables -A br_wan_out -p IPv6 -o nas0+ --ip6-proto 17 --ip6-dport 546 -j DROP
		   ebtables -A br_wan_out -p IPv6 -o nas0+ --ip6-proto 58 --ip6-icmp-type 134 -j DROP
		 */
		va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_WAN_OUT, "-p", "IPv6", "-o", "eth+", "--ip6-proto", "17", "--ip6-dport", "546", "-j", "DROP");
		va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_WAN_OUT, "-p", "IPv6", "-o", "nas0+", "--ip6-proto", "17", "--ip6-sport", "546", "-j", "DROP");
		va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_WAN_OUT, "-p", "IPv6", "-o", "nas0+", "--ip6-proto", "58", "--ip6-icmp-type", "134", "-j", "DROP");

		va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_WAN_OUT, "-p", "IPv6", "-o", "eth+", "--ip6-proto", "17", "--ip6-sport", "546", "-j", "DROP");
		va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_WAN_OUT, "-p", "IPv6", "-o", "nas0+", "--ip6-proto", "17", "--ip6-dport", "546", "-j", "DROP");
		va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_WAN_OUT, "-p", "IPv6", "-o", "nas0+", "--ip6-proto", "58", "--ip6-icmp-type", "134", "-j", "DROP");
	}

	/*
	 * common
	 *
	ebtables -A br_wan -p IPv4 -i eth+ --ip-proto 17 --ip-dport 68 -j DROP
	ebtables -A br_wan -p IPv4 -i eth+ --ip-proto 17 --ip-sport 67 -j DROP
	ebtables -A br_wan -p IPv4 -i nas0+ --ip-proto 17 --ip-sport 68 -j DROP
	ebtables -A br_wan -p IPv4 -i eth+ --ip-dst 224.0.0.1 --ip-proto 2 -j DROP
	*/
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_WAN, "-p", "IPv4", "-i", "eth+", "--ip-proto", "17", "--ip-dport", "68", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_WAN, "-p", "IPv4", "-i", "eth+", "--ip-proto", "17", "--ip-sport", "67", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_WAN, "-p", "IPv4", "-i", "nas0+", "--ip-proto", "17", "--ip-sport", "68", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_WAN, "-p", "IPv4", "-i", "eth+", "--ip-dst", "224.0.0.1", "--ip-proto", "2", "-j", "DROP");

	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_WAN, "-p", "IPv4", "-i", "eth+", "--ip-proto", "17", "--ip-dport", "68", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_WAN, "-p", "IPv4", "-i", "eth+", "--ip-proto", "17", "--ip-sport", "67", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_WAN, "-p", "IPv4", "-i", "nas0+", "--ip-proto", "17", "--ip-sport", "68", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_WAN, "-p", "IPv4", "-i", "eth+", "--ip-dst", "224.0.0.1", "--ip-proto", "2", "-j", "DROP");

	return 0;

}

int deo_IPv6RuleSet(void)
{
	va_cmd(EBTABLES, 16, 1, (char *)FW_DEL, (char*)FW_BR_WAN, "-p", "IPv6", "-i", "nas0+", "--ip6-proto", "17",
			"--ip6-sport", "547", "--ip6-dport", "546", "-j", "ftos", "--set-ftos", "0xb8");

	va_cmd(EBTABLES, 16, 1, (char *)FW_ADD, (char*)FW_BR_WAN, "-p", "IPv6", "-i", "nas0+", "--ip6-proto", "17",
			"--ip6-sport", "547", "--ip6-dport", "546", "-j", "ftos", "--set-ftos", "0xb8");
	/*
	 * ebtables -I br_wan -p IPv6 -i br0 --ip6-proto ipv6-icmp --ip6-icmp-type 128:137/0:255 -j ACCEPT
	 * ebtables -I br_wan -p IPv6 --ip6-proto udp --ip6-dport 546:547 -j ACCEPT
	 * ebtables -I br_wan -p IPv6 -i nas0+ --ip6-proto 17 --ip6-sport 547 --ip6-dport 546  -j ftos --set-ftos 0xb8
	 * ebtables -I br_wan_out -p IPv6 -o br0 --ip6-proto ipv6-icmp --ip6-icmp-type 128:137/0:255 -j ACCEPT
	 * ebtables -I br_wan_out -p IPv6 --ip6-proto udp --ip6-dport 546:547 -j ACCEPT
	 * ebtables -I br_wan_out -p IPv6 --ip6-proto icmpv6 --ip6-icmp-type 4/1 -j ACCEPT
	 *
	 */
#if 0 /* 20231109, sghong, not used */
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_WAN, "-p", "IPv6", "-i", "br0", "--ip6-proto", "ipv6-icmp",
			"--ip6-icmp-type", "128:137/0:255", "-j", "ACCEPT");
	va_cmd(EBTABLES, 10, 1, (char *)FW_DEL, (char*)FW_BR_WAN, "-p", "IPv6", "--ip6-proto", "udp",
			"--ip6-dport", "546:547", "-j", "ACCEPT");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_WAN_OUT, "-p", "IPv6", "-o", "br0", "--ip6-proto", "ipv6-icmp",
			"--ip6-icmp-type", "128:137/0:255", "-j", "ACCEPT");
	va_cmd(EBTABLES, 10, 1, (char *)FW_DEL, (char*)FW_BR_WAN_OUT, "-p", "IPv6", "--ip6-proto", "udp",
			"--ip6-dport", "546:547", "-j", "ACCEPT");
	va_cmd(EBTABLES, 10, 1, (char *)FW_DEL, (char*)FW_BR_WAN_OUT, "-p", "IPv6", "--ip6-proto", "ipv6-icmp",
			"--ip6-icmp-type", "4", "-j", "ACCEPT");

	va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char*)FW_BR_WAN, "-p", "IPv6", "-i", "br0", "--ip6-proto", "ipv6-icmp",
			"--ip6-icmp-type", "128:137/0:255", "-j", "ACCEPT");
	va_cmd(EBTABLES, 10, 1, (char *)FW_INSERT, (char*)FW_BR_WAN, "-p", "IPv6", "--ip6-proto", "udp",
			"--ip6-dport", "546:547", "-j", "ACCEPT");
	va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char*)FW_BR_WAN_OUT, "-p", "IPv6", "-o", "br0", "--ip6-proto", "ipv6-icmp",
			"--ip6-icmp-type", "128:137/0:255", "-j", "ACCEPT");
	va_cmd(EBTABLES, 10, 1, (char *)FW_INSERT, (char*)FW_BR_WAN_OUT, "-p", "IPv6", "--ip6-proto", "udp",
			"--ip6-dport", "546:547", "-j", "ACCEPT");
	va_cmd(EBTABLES, 10, 1, (char *)FW_INSERT, (char*)FW_BR_WAN_OUT, "-p", "IPv6", "--ip6-proto", "ipv6-icmp",
			"--ip6-icmp-type", "4", "-j", "ACCEPT");
#endif

	return;
}

int deo_dhcprelayRuleSet(void)
{
	/*
	    1. WAN 으로부터의 Request, LAN 으로부터의 Reply. 차단
		DHCP relay 의 경우에는 src/dest 모두 67 이므로 주의할 것
		ebtables -A FORWARD -p IPv4 -i nas0+ --ip-proto 17 --ip-sport 68 -j DROP
		ebtables -A FORWARD -p IPv4 -i eth+ --ip-proto 17 --ip-dport 68 -j DROP

		2. CPU로 들어오는 것도 차단
		ebtables -A INPUT -p IPv4 -i nas0+ --ip-proto 17 --ip-sport 68 -j DROP
		ebtables -A INPUT -p IPv4 -i eth+ --ip-proto 17 --ip-dport 68 -j DROP

		3. LAN to LAN Request 차단
		ebtables -A FORWARD -p IPv4 -i eth+ -o eth+ --ip-proto 17 --ip-dport 67 -j DROP

		4. WAN to LAN DHCP 정상 패킷 차단 (DHCP-Relay 를 통해서만 forwarding 되어야 함)
		ebtables -A FORWARD -p IPv4 -i nas0+ -o eth+ --ip-proto 17 --ip-sport 67 -j DROP
		ebtables -A FORWARD -p IPv4 -i eth+ -o nas0+ --ip-proto 17 --ip-dport 67 -j DROP

		5. DHCP-Relay 에서 나가는 패킷에 대한 룰
		ebtables -A OUTPUT -o nas0+ -p ipv4 --ip-proto 17 --ip-dport 68 -j DROP
		ebtables -A OUTPUT -o eth+ -p ipv4 --ip-proto 17 --ip-sport 68 -j DROP

		6. DSCP 46 설정
		ebtables -A br_dhcprelay_forward -p IPv4 -i nas0+ --ip-proto 17 --ip-sport 67 -j mark --mark-set 0x0 --mark-target CONTINUE
		ebtables -A br_dhcprelay_forward -i nas0+ -p ipv4 --ip-proto 17 --ip-sport 67 -j ftos --set-ftos 0xb8
	*/

	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY_FORWARD, "-p", "IPv4", "-i", "nas0+", "--ip-proto", "udp", "--ip-sport", "68", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY_FORWARD, "-p", "IPv4", "-i", "eth+", "--ip-proto", "udp", "--ip-dport", "68", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY_INPUT, "-p", "IPv4", "-i", "nas0+", "--ip-proto", "udp", "--ip-sport", "68", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY_INPUT, "-p", "IPv4", "-i", "eth+", "--ip-proto", "udp", "--ip-dport", "68", "-j", "DROP");

	va_cmd(EBTABLES, 14, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY_FORWARD, "-p", "IPv4", "-i", "eth+", "-o", "eth+", "--ip-proto", "udp", "--ip-dport", "67", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY_FORWARD, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "udp", "--ip-sport", "67", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY_FORWARD, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "udp", "--ip-dport", "67", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY_FORWARD, "-p", "IPv4", "-i", "nas0+", "--ip-proto", "udp", "--ip-sport", "67", "-j", "mark", "--mark-set", "0x0", "--mark-target", "CONTINUE");
	va_cmd(EBTABLES, 14, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY_FORWARD, "-p", "IPv4", "-i", "nas0+", "--ip-proto", "udp", "--ip-sport", "67", "-j", "ftos", "--set-ftos", "0xb8");

	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY_OUTPUT, "-p", "IPv4", "-o", "nas0+", "--ip-proto", "udp", "--ip-dport", "68", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY_OUTPUT, "-p", "IPv4", "-o", "eth+", "--ip-proto", "udp", "--ip-sport", "68", "-j", "DROP");

	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_DHCPRELAY_FORWARD, "-p", "IPv4", "-i", "nas0+", "--ip-proto", "udp", "--ip-sport", "68", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_DHCPRELAY_FORWARD, "-p", "IPv4", "-i", "eth+", "--ip-proto", "udp", "--ip-dport", "68", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_DHCPRELAY_INPUT, "-p", "IPv4", "-i", "nas0+", "--ip-proto", "udp", "--ip-sport", "68", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_DHCPRELAY_INPUT, "-p", "IPv4", "-i", "eth+", "--ip-proto", "udp", "--ip-dport", "68", "-j", "DROP");

	va_cmd(EBTABLES, 14, 1, (char *)FW_ADD, (char*)FW_BR_DHCPRELAY_FORWARD, "-p", "IPv4", "-i", "eth+", "-o", "eth+", "--ip-proto", "udp", "--ip-dport", "67", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, (char *)FW_ADD, (char*)FW_BR_DHCPRELAY_FORWARD, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "udp", "--ip-sport", "67", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, (char *)FW_ADD, (char*)FW_BR_DHCPRELAY_FORWARD, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "udp", "--ip-dport", "67", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, (char *)FW_INSERT, (char*)FW_BR_DHCPRELAY_FORWARD, "-p", "IPv4", "-i", "nas0+", "--ip-proto", "udp", "--ip-sport", "67", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, (char *)FW_INSERT, (char*)FW_BR_DHCPRELAY_FORWARD, "-p", "IPv4", "-i", "nas0+", "--ip-proto", "udp", "--ip-sport", "67", "-j", "mark", "--mark-set", "0x0", "--mark-target", "CONTINUE");

	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_DHCPRELAY_OUTPUT, "-p", "IPv4", "-o", "nas0+", "--ip-proto", "udp", "--ip-dport", "68", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char*)FW_BR_DHCPRELAY_OUTPUT, "-p", "IPv4", "-o", "eth+", "--ip-proto", "udp", "--ip-sport", "68", "-j", "DROP");

	return 0;
}


int deo_dhcpv6relayRuleSet(void)
{
	/*
	 * ebtables -I br_dhcprelay6_forward -p IPv6 -i nas0+ --ip6-proto 17 --ip6-sport 546 -j DROP
	 * ebtables -I br_dhcprelay6_forward -p IPv6 -i eth+ --ip6-proto 17 --ip6-dport 546 -j DROP
	 * ebtables -I br_dhcprelay6_input -p IPv6 -i nas0+ --ip6-proto 17 --ip6-sport 546 -j DROP
	 * ebtables -I br_dhcprelay6_input -p IPv6 -i eth+ --ip6-proto 17 --ip6-dport 546 -j DROP
	 * ebtables -I br_dhcprelay6_forward -p IPv6 -i eth+ -o eth+ --ip6-proto 17 --ip6-dport 547 -j DROP
	 * ebtables -I br_dhcprelay6_forward -p IPv6 -i nas0+ -o eth+ --ip6-proto 17 --ip6-sport 547 -j DROP
	 * ebtables -I br_dhcprelay6_forward -p IPv6 -i eth+ -o nas0+ --ip6-proto 17 --ip6-dport 547 -j DROP
	 * ebtables -I br_dhcprelay6_output  -p ipv6 -o nas0+ --ip6-proto 17 --ip6-dport 546 -j DROP
	 * ebtables -I br_dhcprelay6_output  -p ipv6 -o eth+ --ip6-proto 17 --ip6-sport 546 -j DROP
	 */
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY6_FORWARD, "-p", "IPv6", "-i", "nas0+", "--ip6-proto", "udp", "--ip6-sport", "546", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY6_FORWARD, "-p", "IPv6", "-i", "eth+", "--ip6-proto", "udp", "--ip6-dport", "546", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY6_INPUT, "-p", "IPv6", "-i", "nas0+", "--ip6-proto", "udp", "--ip6-sport", "546", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY6_INPUT, "-p", "IPv6", "-i", "eth+", "--ip6-proto", "udp", "--ip6-dport", "546", "-j", "DROP");

	va_cmd(EBTABLES, 14, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY6_FORWARD, "-p", "IPv6", "-i", "eth+", "-o", "eth+", "--ip6-proto", "udp", "--ip6-dport", "547", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY6_FORWARD, "-p", "IPv6", "-i", "nas0+", "-o", "eth+", "--ip6-proto", "udp", "--ip6-sport", "547", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY6_FORWARD, "-p", "IPv6", "-i", "eth+", "-o", "nas0+", "--ip6-proto", "udp", "--ip6-dport", "547", "-j", "DROP");

	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY6_OUTPUT, "-p", "IPv6", "-o", "nas0+", "--ip6-proto", "udp", "--ip6-dport", "546", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char*)FW_BR_DHCPRELAY6_OUTPUT, "-p", "IPv6", "-o", "eth+", "--ip6-proto", "udp", "--ip6-sport", "546", "-j", "DROP");


	va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char*)FW_BR_DHCPRELAY6_FORWARD, "-p", "IPv6", "-i", "nas0+", "--ip6-proto", "udp", "--ip6-sport", "546", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char*)FW_BR_DHCPRELAY6_FORWARD, "-p", "IPv6", "-i", "eth+", "--ip6-proto", "udp", "--ip6-dport", "546", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char*)FW_BR_DHCPRELAY6_INPUT, "-p", "IPv6", "-i", "nas0+", "--ip6-proto", "udp", "--ip6-sport", "546", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char*)FW_BR_DHCPRELAY6_INPUT, "-p", "IPv6", "-i", "eth+", "--ip6-proto", "udp", "--ip6-dport", "546", "-j", "DROP");

	va_cmd(EBTABLES, 14, 1, (char *)FW_INSERT, (char*)FW_BR_DHCPRELAY6_FORWARD, "-p", "IPv6", "-i", "eth+", "-o", "eth+", "--ip6-proto", "udp", "--ip6-dport", "547", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, (char *)FW_INSERT, (char*)FW_BR_DHCPRELAY6_FORWARD, "-p", "IPv6", "-i", "nas0+", "-o", "eth+", "--ip6-proto", "udp", "--ip6-sport", "547", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, (char *)FW_INSERT, (char*)FW_BR_DHCPRELAY6_FORWARD, "-p", "IPv6", "-i", "eth+", "-o", "nas0+", "--ip6-proto", "udp", "--ip6-dport", "547", "-j", "DROP");

	va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char*)FW_BR_DHCPRELAY6_OUTPUT, "-p", "IPv6", "-o", "nas0+", "--ip6-proto", "udp", "--ip6-dport", "546", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char*)FW_BR_DHCPRELAY6_OUTPUT, "-p", "IPv6", "-o", "eth+", "--ip6-proto", "udp", "--ip6-sport", "546", "-j", "DROP");

	return 0;
}

int deo_icmpv6CpuTxRules(void)
{
	/* CPU TX */
	/*
	 * ip6tables -t mangle -I OUTPUT -o eth+ -p icmpv6 --icmpv6-type 131 -j DROP
	 * ip6tables -t mangle -I OUTPUT -o eth+ -p icmpv6 --icmpv6-type 132 -j DROP
	 * ip6tables -t mangle -I OUTPUT -o eth+ -p icmpv6 --icmpv6-type 143 -j DROP
	 *
	 * ip6tables -t mangle -I POSTROUTING -o nas+ -p icmpv6 --icmpv6-type 133 -j DSCP --set-dscp 46
	 * ip6tables -t mangle -I POSTROUTING -o nas+ -p icmpv6 --icmpv6-type 135 -j DSCP --set-dscp 46
	 * ip6tables -t mangle -I POSTROUTING -o nas+ -p icmpv6 --icmpv6-type 136 -j DSCP --set-dscp 46
	 * ip6tables -t mangle -I POSTROUTING -o nas+ -p icmpv6 --icmpv6-type 131 -j DSCP --set-dscp 46
	 * ip6tables -t mangle -I POSTROUTING -o nas+ -p icmpv6 --icmpv6-type 132 -j DSCP --set-dscp 46
	 * ip6tables -t mangle -I POSTROUTING -o nas+ -p icmpv6 --icmpv6-type 143 -j DSCP --set-dscp 46
	 * ip6tables -t mangle -I POSTROUTING -o eth+ -p icmpv6 --icmpv6-type 135 -j DSCP --set-dscp 46
	 *
	 * ebtables -I OUTPUT -o nas+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 134 -j DROP
	 * ebtables -I OUTPUT -o eth+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 130 -j ftos --set-ftos 0xb8
	 */
	 va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_BR_ICMPV6_TX_MANGLE_OUTPUT, "-o", "eth+", "-p", "icmpv6", "--icmpv6-type", "131", "-j", "DROP");
	 va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_BR_ICMPV6_TX_MANGLE_OUTPUT, "-o", "eth+", "-p", "icmpv6", "--icmpv6-type", "132", "-j", "DROP");
	 va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_BR_ICMPV6_TX_MANGLE_OUTPUT, "-o", "eth+", "-p", "icmpv6", "--icmpv6-type", "143", "-j", "DROP");

	 va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_BR_ICMPV6_TX_MANGLE_POSTROUTING, "-o", "br0", "-p", "icmpv6", "--icmpv6-type", "133", "-j", "DSCP", "--set-dscp", "46");
	 va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_BR_ICMPV6_TX_MANGLE_POSTROUTING, "-o", "br0", "-p", "icmpv6", "--icmpv6-type", "135", "-j", "DSCP", "--set-dscp", "46");
	 va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_BR_ICMPV6_TX_MANGLE_POSTROUTING, "-o", "br0", "-p", "icmpv6", "--icmpv6-type", "136", "-j", "DSCP", "--set-dscp", "46");
	 va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_BR_ICMPV6_TX_MANGLE_POSTROUTING, "-o", "br0", "-p", "icmpv6", "--icmpv6-type", "131", "-j", "DSCP", "--set-dscp", "46");
	 va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_BR_ICMPV6_TX_MANGLE_POSTROUTING, "-o", "br0", "-p", "icmpv6", "--icmpv6-type", "132", "-j", "DSCP", "--set-dscp", "46");
	 va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_BR_ICMPV6_TX_MANGLE_POSTROUTING, "-o", "br0", "-p", "icmpv6", "--icmpv6-type", "143", "-j", "DSCP", "--set-dscp", "46");
	 va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_BR_ICMPV6_TX_MANGLE_POSTROUTING, "-o", "eth+", "-p", "icmpv6", "--icmpv6-type", "135", "-j", "DSCP", "--set-dscp", "46");

	 va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char *)FW_BR_ICMPV6_TX_OUTPUT, "-o", "br0", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "134", "-j", "DROP");
	 va_cmd(EBTABLES, 14, 1, (char *)FW_DEL, (char *)FW_BR_ICMPV6_TX_OUTPUT, "-o", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "130", "-j", "ftos", "--set-ftos", "0xb8");


	 va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_BR_ICMPV6_TX_MANGLE_OUTPUT, "-o", "eth+", "-p", "icmpv6", "--icmpv6-type", "131", "-j", "DROP");
	 va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_BR_ICMPV6_TX_MANGLE_OUTPUT, "-o", "eth+", "-p", "icmpv6", "--icmpv6-type", "132", "-j", "DROP");
	 va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_BR_ICMPV6_TX_MANGLE_OUTPUT, "-o", "eth+", "-p", "icmpv6", "--icmpv6-type", "143", "-j", "DROP");

	 va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_BR_ICMPV6_TX_MANGLE_POSTROUTING, "-o", "br0", "-p", "icmpv6", "--icmpv6-type", "133", "-j", "DSCP", "--set-dscp", "46");
	 va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_BR_ICMPV6_TX_MANGLE_POSTROUTING, "-o", "br0", "-p", "icmpv6", "--icmpv6-type", "135", "-j", "DSCP", "--set-dscp", "46");
	 va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_BR_ICMPV6_TX_MANGLE_POSTROUTING, "-o", "br0", "-p", "icmpv6", "--icmpv6-type", "136", "-j", "DSCP", "--set-dscp", "46");
	 va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_BR_ICMPV6_TX_MANGLE_POSTROUTING, "-o", "br0", "-p", "icmpv6", "--icmpv6-type", "131", "-j", "DSCP", "--set-dscp", "46");
	 va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_BR_ICMPV6_TX_MANGLE_POSTROUTING, "-o", "br0", "-p", "icmpv6", "--icmpv6-type", "132", "-j", "DSCP", "--set-dscp", "46");
	 va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_BR_ICMPV6_TX_MANGLE_POSTROUTING, "-o", "br0", "-p", "icmpv6", "--icmpv6-type", "143", "-j", "DSCP", "--set-dscp", "46");
	 va_cmd(IP6TABLES, 14, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_BR_ICMPV6_TX_MANGLE_POSTROUTING, "-o", "eth+", "-p", "icmpv6", "--icmpv6-type", "135", "-j", "DSCP", "--set-dscp", "46");

	 va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)FW_BR_ICMPV6_TX_OUTPUT, "-o", "br0", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "134", "-j", "DROP");
	 va_cmd(EBTABLES, 14, 1, (char *)FW_INSERT, (char *)FW_BR_ICMPV6_TX_OUTPUT, "-o", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "130", "-j", "ftos", "--set-ftos", "0xb8");

	 return 0;
}

int deo_ipv6OltToLANRules(void)
{
	/* OLT <--> LAN */
#if 0
	/* RS UP / DOWN */
	ebtableS -t broute -I BROUTING -i eth+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 133 -j ftos --set-ftos 0xb8  16
	ebtables -t broute -I BROUTING -i nas+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 133 -j ftos --set-ftos 0xb8 16

	/* RA DOWN */
	ebtables -t broute -I BROUTING -i nas+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 134 -j ftos --set-ftos 0xb8

	/* NS & NA UP / DOWN */
	ebtables -t broute -I BROUTING -i eth+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 135 -j ftos --set-ftos 0xb8
	ebtables -t broute -I BROUTING -i eth+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 136 -j ftos --set-ftos 0xb8
	ebtables -t broute -I BROUTING -i nas+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 135 -j ftos --set-ftos 0xb8
	ebtables -t broute -I BROUTING -i nas+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 136 -j ftos --set-ftos 0xb8

	/* MLD Report (join/leave) & Query */
	ebtables -t broute -I BROUTING -i eth+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 131 -j ftos --set-ftos 0xb8
	ebtables -t broute -I BROUTING -i eth+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 132 -j ftos --set-ftos 0xb8
	ebtables -t broute -I BROUTING -i eth+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 143 -j ftos --set-ftos 0xb8
	ebtables -t broute -I BROUTING -i nas+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 130 -j ftos --set-ftos 0xb8

	/* MLD Query */
	ebtables -t broute -A BROUTING -p ipv6 -i eth+ --ip6-proto 58 --ip6-icmp-type 130 --ip6-src fe80::/10 -j DROP

	/* MLD Report v1 & v2 */
	ebtables -t broute -A BROUTING -p ipv6 -i nas+ --ip6-proto 58 --ip6-icmp-type 131 --ip6-src fe80::/10 -j DROP
	ebtables -t broute -A BROUTING -p ipv6 -i nas+ --ip6-proto 58 --ip6-icmp-type 143 --ip6-src fe80::/10 -j DROP

	ebtables -I FORWARD -o eth+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 131 -j DROP 12
	ebtables -I FORWARD -o eth+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 143 -j DROP
	ebtables -I FORWARD -o eth+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 132 -j DROP

	ebtables -I FORWARD -o eth+ -p ipv6 --ip6-proto 58 --ip6-icmp-type 133 -j DROP
#endif
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_OLTTOLAN_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "133", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_OLTTOLAN_BROUTE, "-i", "nas+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "133", "-j", "ftos", "--set-ftos", "0xb8");

	/* RA DOWN */
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_OLTTOLAN_BROUTE, "-i", "nas+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "134", "-j", "ftos", "--set-ftos", "0xb8");

	/* NS & NA UP / DOWN */
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_OLTTOLAN_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "135", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_OLTTOLAN_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "136", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_OLTTOLAN_BROUTE, "-i", "nas+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "135", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_OLTTOLAN_BROUTE, "-i", "nas+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "136", "-j", "ftos", "--set-ftos", "0xb8");

	/* MLD Report (join/leave) & Query */
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_OLTTOLAN_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "131", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_OLTTOLAN_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "132", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_OLTTOLAN_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "143", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_OLTTOLAN_BROUTE, "-i", "br0", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "130", "-j", "ftos", "--set-ftos", "0xb8");

	/* MLD Query */
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_OLTTOLAN_BROUTE, "-p", "ipv6", "-i", "eth+", "--ip6-proto", "58", "--ip6-icmp-type", "130", "--ip6-src", "fe80::/10", "-j", "DROP");

	/* MLD Report v1 & v2 */
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_OLTTOLAN_BROUTE, "-p", "ipv6", "-i", "nas+", "--ip6-proto", "58", "--ip6-icmp-type", "131", "--ip6-src", "fe80::/10", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, "-t", "broute", (char *)FW_DEL, FW_BR_OLTTOLAN_BROUTE, "-p", "ipv6", "-i", "nas+", "--ip6-proto", "58", "--ip6-icmp-type", "143", "--ip6-src", "fe80::/10", "-j", "DROP");

	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char *)FW_BR_OLTTOLAN_FORWARD, "-o", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "131", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char *)FW_BR_OLTTOLAN_FORWARD, "-o", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "143", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char *)FW_BR_OLTTOLAN_FORWARD, "-o", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "132", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_DEL, (char *)FW_BR_OLTTOLAN_FORWARD, "-o", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "133", "-j", "DROP");


	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_INSERT, FW_BR_OLTTOLAN_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "133", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_INSERT, FW_BR_OLTTOLAN_BROUTE, "-i", "nas+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "133", "-j", "ftos", "--set-ftos", "0xb8");

	/* RA DOWN */
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_INSERT, FW_BR_OLTTOLAN_BROUTE, "-i", "nas+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "134", "-j", "ftos", "--set-ftos", "0xb8");

	/* NS & NA UP / DOWN */
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_INSERT, FW_BR_OLTTOLAN_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "135", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_INSERT, FW_BR_OLTTOLAN_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "136", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_INSERT, FW_BR_OLTTOLAN_BROUTE, "-i", "nas+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "135", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_INSERT, FW_BR_OLTTOLAN_BROUTE, "-i", "nas+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "136", "-j", "ftos", "--set-ftos", "0xb8");

	/* MLD Report (join/leave) & Query */
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_INSERT, FW_BR_OLTTOLAN_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "131", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_INSERT, FW_BR_OLTTOLAN_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "132", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_INSERT, FW_BR_OLTTOLAN_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "143", "-j", "ftos", "--set-ftos", "0xb8");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_INSERT, FW_BR_OLTTOLAN_BROUTE, "-i", "br0", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "130", "-j", "ftos", "--set-ftos", "0xb8");

	/* MLD Query */
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_ADD, FW_BR_OLTTOLAN_BROUTE, "-p", "ipv6", "-i", "eth+", "--ip6-proto", "58", "--ip6-icmp-type", "130", "--ip6-src", "fe80::/10", "-j", "DROP");

	/* MLD Report v1 & v2 */
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_ADD, FW_BR_OLTTOLAN_BROUTE, "-p", "ipv6", "-i", "nas+", "--ip6-proto", "58", "--ip6-icmp-type", "131", "--ip6-src", "fe80::/10", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_ADD, FW_BR_OLTTOLAN_BROUTE, "-p", "ipv6", "-i", "nas+", "--ip6-proto", "58", "--ip6-icmp-type", "143", "--ip6-src", "fe80::/10", "-j", "DROP");

	va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)FW_BR_OLTTOLAN_FORWARD, "-o", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "131", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)FW_BR_OLTTOLAN_FORWARD, "-o", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "143", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)FW_BR_OLTTOLAN_FORWARD, "-o", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "132", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)FW_BR_OLTTOLAN_FORWARD, "-o", "eth+", "-p", "ipv6", "--ip6-proto", "58", "--ip6-icmp-type", "133", "-j", "DROP");

	return 0;
}

int deo_lantolanDSCPSetRules(void)
{
	/*
	 * ebtables -t broute -A BROUTING -i eth+ -j mark --mark-set 0x3 --mark-target CONTINUE
	 * ebtables -t broute -A BROUTING -i eth+ -p ipv4 --ip-dscp 60 -j mark --mark-set 0x2 --mark-target CONTINUE
	 * ebtables -t broute -A BROUTING -i eth+ -p ipv4 --ip-dscp 80 -j mark --mark-set 0x1 --mark-target CONTINUE
	 * ebtables -t broute -A BROUTING -i eth+ -p ipv4 --ip-dscp b8 -j mark --mark-set 0x0 --mark-target CONTINUE
	 * ebtables -t broute -A BROUTING -i eth+ -p ipv6 --ip6-tclass 60 -j mark --mark-set 0x2 --mark-target CONTINUE
	 * ebtables -t broute -A BROUTING -i eth+ -p ipv6 --ip6-tclass 80 -j mark --mark-set 0x1 --mark-target CONTINUE
	 * ebtables -t broute -A BROUTING -i eth+ -p ipv6 --ip6-tclass b8 -j mark --mark-set 0x0 --mark-target CONTINUE
	 */
	va_cmd(EBTABLES, 12, 1, "-t", "broute", (char *)FW_DEL, FW_BR_LANTOLAN_DSCP_BROUTE, "-i", "eth+", "-j", "mark", "--mark-set", "0x3", "--mark-target", "CONTINUE");
	 va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_LANTOLAN_DSCP_BROUTE, "-i", "eth+", "-p", "ipv4", "--ip-dscp", "60", "-j", "mark", "--mark-set", "0x2", "--mark-target", "CONTINUE");
	 va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_LANTOLAN_DSCP_BROUTE, "-i", "eth+", "-p", "ipv4", "--ip-dscp", "80", "-j", "mark", "--mark-set", "0x1", "--mark-target", "CONTINUE");
	 va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_LANTOLAN_DSCP_BROUTE, "-i", "eth+", "-p", "ipv4", "--ip-dscp", "b8", "-j", "mark", "--mark-set", "0x0", "--mark-target", "CONTINUE");
	 va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_LANTOLAN_DSCP_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-tclass", "60", "-j", "mark", "--mark-set", "0x2", "--mark-target", "CONTINUE");
	 va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_LANTOLAN_DSCP_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-tclass", "80", "-j", "mark", "--mark-set", "0x1", "--mark-target", "CONTINUE");
	 va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_DEL, FW_BR_LANTOLAN_DSCP_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-tclass", "b8", "-j", "mark", "--mark-set", "0x0", "--mark-target", "CONTINUE");

	va_cmd(EBTABLES, 12, 1, "-t", "broute", (char *)FW_ADD, FW_BR_LANTOLAN_DSCP_BROUTE, "-i", "eth+", "-j", "mark", "--mark-set", "0x3", "--mark-target", "CONTINUE");
	 va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_ADD, FW_BR_LANTOLAN_DSCP_BROUTE, "-i", "eth+", "-p", "ipv4", "--ip-dscp", "60", "-j", "mark", "--mark-set", "0x2", "--mark-target", "CONTINUE");
	 va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_ADD, FW_BR_LANTOLAN_DSCP_BROUTE, "-i", "eth+", "-p", "ipv4", "--ip-dscp", "80", "-j", "mark", "--mark-set", "0x1", "--mark-target", "CONTINUE");
	 va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_ADD, FW_BR_LANTOLAN_DSCP_BROUTE, "-i", "eth+", "-p", "ipv4", "--ip-dscp", "b8", "-j", "mark", "--mark-set", "0x0", "--mark-target", "CONTINUE");
	 va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_ADD, FW_BR_LANTOLAN_DSCP_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-tclass", "60", "-j", "mark", "--mark-set", "0x2", "--mark-target", "CONTINUE");
	 va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_ADD, FW_BR_LANTOLAN_DSCP_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-tclass", "80", "-j", "mark", "--mark-set", "0x1", "--mark-target", "CONTINUE");
	 va_cmd(EBTABLES, 16, 1, "-t", "broute", (char *)FW_ADD, FW_BR_LANTOLAN_DSCP_BROUTE, "-i", "eth+", "-p", "ipv6", "--ip6-tclass", "b8", "-j", "mark", "--mark-set", "0x0", "--mark-target", "CONTINUE");

	 return 0;
}

int deo_dhcpClientOnBridge(void)
{
	int wan_mode;
	unsigned char is_ipv6;

	/* mode check, bridge mode add */
	mib_get_s(MIB_WAN_NAT_MODE, &wan_mode, sizeof(wan_mode));
	mib_get_s(MIB_V6_IPV6_ENABLE, &is_ipv6, sizeof(is_ipv6));

	if (wan_mode == DEO_BRIDGE_MODE || is_ipv6)
	{
#if 0 /* 20231109, sghong, not used */
		/*
		   ebtables -I br_wan -p IPv4 --ip-proto udp --ip-dport 67:68 -j ACCEPT
		   ebtables -I br_wan -p ARP -i br0 -j ACCEPT
		   ebtables -I br_wan_out -p IPv4 --ip-proto udp --ip-dport 67:68 -j ACCEPT
		   ebtables -I br_wan_out -p ARP -o br0 -j ACCEPT
		*/
		va_cmd(EBTABLES, 10, 1, (char *)FW_DEL, (char*)FW_BR_WAN, "-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "ACCEPT");
		va_cmd(EBTABLES, 10, 1, (char *)FW_DEL, (char*)FW_BR_WAN, "-p", "IPv4", "--ip-proto", "udp", "--ip-sport", "53", "-j", "ACCEPT");
		va_cmd(EBTABLES, 8, 1, (char *)FW_DEL, (char*)FW_BR_WAN, "-p", "ARP", "-i", "br0", "-j", "ACCEPT");
		va_cmd(EBTABLES, 10, 1, (char *)FW_DEL, (char*)FW_BR_WAN_OUT, "-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "ACCEPT");
		va_cmd(EBTABLES, 10, 1, (char *)FW_DEL, (char*)FW_BR_WAN_OUT, "-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "53", "-j", "ACCEPT");
		va_cmd(EBTABLES, 8, 1, (char *)FW_DEL, (char*)FW_BR_WAN_OUT, "-p", "ARP", "-i", "br0", "-j", "ACCEPT");

		va_cmd(EBTABLES, 10, 1, (char *)FW_INSERT, (char*)FW_BR_WAN, "-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "ACCEPT");
		va_cmd(EBTABLES, 10, 1, (char *)FW_INSERT, (char*)FW_BR_WAN, "-p", "IPv4", "--ip-proto", "udp", "--ip-sport", "53", "-j", "ACCEPT");
		va_cmd(EBTABLES, 8, 1, (char *)FW_INSERT, (char*)FW_BR_WAN, "-p", "ARP", "-i", "br0", "-j", "ACCEPT");
		va_cmd(EBTABLES, 10, 1, (char *)FW_INSERT, (char*)FW_BR_WAN_OUT, "-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "ACCEPT");
		va_cmd(EBTABLES, 10, 1, (char *)FW_INSERT, (char*)FW_BR_WAN_OUT, "-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "53", "-j", "ACCEPT");
		va_cmd(EBTABLES, 8, 1, (char *)FW_INSERT, (char*)FW_BR_WAN_OUT, "-p", "ARP", "-i", "br0", "-j", "ACCEPT");
#endif
	}

	return 0;
}

int	deo_rateRuleSet(int wan_mode, char *ifname)
{
	/*
	 * ifname : br0 or nas0+
	   iptables -I RATE_ACL -p tcp -i %s --syn -j DROP"
	   iptables -I RATE_ACL -p tcp -i %s --syn -m limit --limit 200/sec --limit-burst 200 -j ACCEPT"
	   iptables -I RATE_ACL -p tcp -i %s --syn -m recent --name syn_rate --set
	   iptables -I RATE_ACL -p tcp -i %s --syn -m recent --name syn_rate --update --seconds 1 --hitcount 20 -j DROP"
	   iptables -I RATE_ACL -p udp -i %s --dport 123 -j DROP"
	   iptables -I RATE_ACL -p udp -i %s --dport 123 -m limit --limit %d/sec --limit-burst %d -j ACCEPT"  100/100
	   iptables -A RATE_ACL -p udp -i %s --dport 161 -m limit --limit %d/sec --limit-burst %d -j ACCEPT"  100/100
	   iptables -A RATE_ACL -p udp -i %s --dport 161 -j DROP"
	   iptables -A RATE_ACL -p tcp -i %s --dport 22 -m limit --limit 200/sec --limit-burst 200 -j ACCEPT"
	   iptables -A RATE_ACL -p tcp -i %s --dport 22 -j DROP"
	   iptables -A RATE_ACL -p tcp -i %s --dport 8080 -m limit --limit 200/sec --limit-burst 200 -j ACCEPT"
	   iptables -A RATE_ACL -p tcp -i %s --dport 8080 -j DROP"
	   iptables -A RATE_ACL -p 2 -i %s -m limit --limit 200/sec --limit-burst 200 -j ACCEPT"
	   iptables -A RATE_ACL -p icmp -i %s -m limit --limit 200/sec --limit-burst 200 -j ACCEPT"
	   iptables -A RATE_ACL -p icmp -i %s -j DROP"
	   iptables -A RATE_ACL -p ALL -i br0 -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
	*/
#if 0
	va_cmd(IPTABLES, 18, 1, FW_DEL, RATE_ACL, "-p", "tcp", "-i", ifname, "--syn", "-m", "recent", "--name", "syn_rate", "--update", "--seconds", "1", "--hitcount", "20", "-j", "DROP");
	va_cmd(IPTABLES, 12, 1, FW_DEL, RATE_ACL, "-p", "tcp", "-i", ifname, "--syn", "-m", "recent", "--name", "syn_rate", "--set");
#endif
	va_cmd(IPTABLES, 15, 1, FW_DEL, RATE_ACL, "-p", "tcp", "-i", ifname, "--syn", "-m", "limit", "--limit", "200/sec", "--limit-burst", "200", "-j", "ACCEPT");
	va_cmd(IPTABLES,  9, 1, FW_DEL, RATE_ACL, "-p", "tcp", "-i", ifname, "--syn", "-j", "DROP");
	va_cmd(IPTABLES, 16, 1, FW_DEL, RATE_ACL, "-p", "udp", "-i", ifname, "--dport", "123", "-m", "limit", "--limit", "100/sec", "--limit-burst", "100", "-j", "ACCEPT");
	va_cmd(IPTABLES, 10, 1, FW_DEL, RATE_ACL, "-p", "udp", "-i", ifname, "--dport", "123", "-j", "DROP");
	va_cmd(IPTABLES, 16, 1, FW_DEL, RATE_ACL, "-p", "udp", "-i", ifname, "--dport", "161", "-m", "limit", "--limit", "100/sec", "--limit-burst", "100", "-j", "ACCEPT");
	va_cmd(IPTABLES, 10, 1, FW_DEL, RATE_ACL, "-p", "udp", "-i", ifname, "--dport", "161", "-j", "DROP");
	va_cmd(IPTABLES, 16, 1, FW_DEL, RATE_ACL, "-p", "udp", "-i", ifname, "--dport", "30161", "-m", "limit", "--limit", "100/sec", "--limit-burst", "100", "-j", "ACCEPT");
	va_cmd(IPTABLES, 10, 1, FW_DEL, RATE_ACL, "-p", "udp", "-i", ifname, "--dport", "30161", "-j", "DROP");
	va_cmd(IPTABLES, 16, 1, FW_DEL, RATE_ACL, "-p", "tcp", "-i", ifname, "--dport", "22", "-m", "limit", "--limit", "200/sec", "--limit-burst", "200", "-j", "ACCEPT");
	va_cmd(IPTABLES, 10, 1, FW_DEL, RATE_ACL, "-p", "tcp", "-i", ifname, "--dport", "22", "-j", "DROP");
	va_cmd(IPTABLES, 16, 1, FW_DEL, RATE_ACL, "-p", "tcp", "-i", ifname, "--dport", "8080", "-m", "limit", "--limit", "200/sec", "--limit-burst", "200", "-j", "ACCEPT");
	va_cmd(IPTABLES, 10, 1, FW_DEL, RATE_ACL, "-p", "tcp", "-i", ifname, "--dport", "8080", "-j", "DROP");
	va_cmd(IPTABLES, 14, 1, FW_DEL, RATE_ACL, "-p", "2"  , "-i", ifname, "-m", "limit", "--limit", "200/sec", "--limit-burst", "200", "-j", "ACCEPT");
	va_cmd(IPTABLES, 14, 1, FW_DEL, RATE_ACL, "-p", "icmp", "-i", ifname, "-m", "limit", "--limit", "200/sec", "--limit-burst", "200", "-j", "ACCEPT");
	va_cmd(IPTABLES,  8, 1, FW_DEL, RATE_ACL, "-p", "icmp", "-i", ifname, "-j", "DROP");
	va_cmd(IPTABLES, 12, 1, FW_DEL, RATE_ACL, "-p", "ALL",  "-i", ifname, "-m", "conntrack", "--ctstate", "ESTABLISHED,RELATED", "-j", "ACCEPT");


#if 0
	va_cmd(IPTABLES, 18, 1, FW_ADD, RATE_ACL, "-p", "tcp", "-i", ifname, "--syn", "-m", "recent", "--name", "syn_rate", "--update", "--seconds", "1", "--hitcount", "20", "-j", "DROP");
	va_cmd(IPTABLES, 12, 1, FW_ADD, RATE_ACL, "-p", "tcp", "-i", ifname, "--syn", "-m", "recent", "--name", "syn_rate", "--set");
#endif
	va_cmd(IPTABLES, 15, 1, FW_ADD, RATE_ACL, "-p", "tcp", "-i", ifname, "--syn", "-m", "limit", "--limit", "200/sec", "--limit-burst", "200", "-j", "ACCEPT");
	va_cmd(IPTABLES,  9, 1, FW_ADD, RATE_ACL, "-p", "tcp", "-i", ifname, "--syn", "-j", "DROP");
	va_cmd(IPTABLES, 16, 1, FW_ADD, RATE_ACL, "-p", "udp", "-i", ifname, "--dport", "123", "-m", "limit", "--limit", "100/sec", "--limit-burst", "100", "-j", "ACCEPT");
	va_cmd(IPTABLES, 10, 1, FW_ADD, RATE_ACL, "-p", "udp", "-i", ifname, "--dport", "123", "-j", "DROP");
	va_cmd(IPTABLES, 16, 1, FW_ADD, RATE_ACL, "-p", "udp", "-i", ifname, "--dport", "161", "-m", "limit", "--limit", "100/sec", "--limit-burst", "100", "-j", "ACCEPT");
	va_cmd(IPTABLES, 10, 1, FW_ADD, RATE_ACL, "-p", "udp", "-i", ifname, "--dport", "161", "-j", "DROP");
	va_cmd(IPTABLES, 16, 1, FW_ADD, RATE_ACL, "-p", "udp", "-i", ifname, "--dport", "30161", "-m", "limit", "--limit", "100/sec", "--limit-burst", "100", "-j", "ACCEPT");
	va_cmd(IPTABLES, 10, 1, FW_ADD, RATE_ACL, "-p", "udp", "-i", ifname, "--dport", "30161", "-j", "DROP");
	va_cmd(IPTABLES, 16, 1, FW_ADD, RATE_ACL, "-p", "tcp", "-i", ifname, "--dport", "22", "-m", "limit", "--limit", "200/sec", "--limit-burst", "200", "-j", "ACCEPT");
	va_cmd(IPTABLES, 10, 1, FW_ADD, RATE_ACL, "-p", "tcp", "-i", ifname, "--dport", "22", "-j", "DROP");
	va_cmd(IPTABLES, 16, 1, FW_ADD, RATE_ACL, "-p", "tcp", "-i", ifname, "--dport", "8080", "-m", "limit", "--limit", "200/sec", "--limit-burst", "200", "-j", "ACCEPT");
	va_cmd(IPTABLES, 10, 1, FW_ADD, RATE_ACL, "-p", "tcp", "-i", ifname, "--dport", "8080", "-j", "DROP");
	va_cmd(IPTABLES, 14, 1, FW_ADD, RATE_ACL, "-p", "2",   "-i", ifname, "-m", "limit", "--limit", "200/sec", "--limit-burst", "200", "-j", "ACCEPT");
	va_cmd(IPTABLES, 14, 1, FW_ADD, RATE_ACL, "-p", "icmp", "-i", ifname, "-m", "limit", "--limit", "200/sec", "--limit-burst", "200", "-j", "ACCEPT");
	va_cmd(IPTABLES,  8, 1, FW_ADD, RATE_ACL, "-p", "icmp", "-i", ifname, "-j", "DROP");
	va_cmd(IPTABLES, 12, 1, FW_ADD, RATE_ACL, "-p", "ALL",  "-i", ifname, "-m", "conntrack", "--ctstate", "ESTABLISHED,RELATED", "-j", "ACCEPT");

	return 0;
}

int	deo_cpurateRuleSet(int wan_mode, char *ifname)
{
	/*
	 * ifname : br0 or nas0+
	 iptables -A CPU_RATE_ACL -i %s -m limit --limit 2000/sec --limit-burst 1000 -j ACCEPT
	 iptables -A CPU_RATE_ACL -i %s -j DROP
	 */
	va_cmd(IPTABLES, 6, 1, FW_DEL, CPU_RATE_ACL, "-i", ifname, "-m", "limit", "--limit", "2000/sec", "--limit-burst", "1000", "-j", "ACCEPT");
	va_cmd(IPTABLES, 6, 1, FW_DEL, CPU_RATE_ACL, "-i", ifname, "-j", "DROP");


	va_cmd(IPTABLES, 6, 1, FW_ADD, CPU_RATE_ACL, "-i", ifname, "-m", "limit", "--limit", "2000/sec", "--limit-burst", "1000", "-j", "ACCEPT");
	va_cmd(IPTABLES, 6, 1, FW_ADD, CPU_RATE_ACL, "-i", ifname, "-j", "DROP");

	return 0;
}

int	deo_upnpRuleSet(int wan_mode, int ipv6_en, char *ifname)
{
	if (wan_mode == DEO_NAT_MODE)
	{
		va_cmd(IPTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "tcp", "-m", "multiport", "--dports", "137,138,139,445", "-i", "br+", "-o", "nas0+", "-j", "DROP");
		va_cmd(IPTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "tcp", "-m", "multiport", "--dports", "137,138,139,445", "-i", "nas0+", "-o", "br+", "-j", "DROP");
		va_cmd(IPTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "udp", "-m", "multiport", "--dports", "137,138,139,445", "-i", "br+", "-o", "nas0+", "-j", "DROP");
		va_cmd(IPTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "udp", "-m", "multiport", "--dports", "137,138,139,445", "-i", "nas0+", "-o", "br+", "-j", "DROP");

		va_cmd(IPTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "tcp", "-m", "multiport", "--dports", "137,138,139,445", "-i", "br+", "-o", "nas0+", "-j", "DROP");
		va_cmd(IPTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "tcp", "-m", "multiport", "--dports", "137,138,139,445", "-i", "nas0+", "-o", "br+", "-j", "DROP");
		va_cmd(IPTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "udp", "-m", "multiport", "--dports", "137,138,139,445", "-i", "br+", "-o", "nas0+", "-j", "DROP");
		va_cmd(IPTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "udp", "-m", "multiport", "--dports", "137,138,139,445", "-i", "nas0+", "-o", "br+", "-j", "DROP");

		va_cmd(EBTABLES, 10, 1, FW_DEL, UPNP_ACL, "-d", "33:33:0:0:1:30", "-i", "eth+", "-o", "nas0+", "-j", "DROP");
		va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:1:30", "-i", "eth+", "-o", "nas0+", "--ip6-proto", "17", "--ip6-dport", "1900", "-j", "DROP");

		va_cmd(EBTABLES, 10, 1, FW_INSERT, UPNP_ACL, "-d", "33:33:0:0:1:30", "-i", "eth+", "-o", "nas0+", "-j", "DROP");
		va_cmd(EBTABLES, 16, 1, FW_INSERT, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:1:30", "-i", "eth+", "-o", "nas0+", "--ip6-proto", "17", "--ip6-dport", "1900", "-j", "DROP");
	}

	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-dst", "239.255.255.250", "--ip-proto", "17", "--ip-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-dst", "224.0.0.251", "--ip-proto", "17", "--ip-dport", "5353", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-dst", "224.0.0.252", "--ip-proto", "17", "--ip-dport", "5355", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-dst", "239.255.255.246", "--ip-proto", "17", "--ip-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+",  "-o", "nas0+", "--ip-dst", "239.255.255.250", "--ip-proto", "17", "--ip-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+",  "-o", "nas0+", "--ip-dst", "224.0.0.251", "--ip-proto", "17", "--ip-dport", "5353", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+",  "-o", "nas0+", "--ip-dst", "224.0.0.252", "--ip-proto", "17", "--ip-dport", "5355", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+",  "-o", "nas0+", "--ip-dst", "239.255.255.246", "--ip-proto", "17", "--ip-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+",  "-o", "nas0+", "--ip-dst", "224.0.1.65", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+",  "-o", "nas0+", "--ip-dst", "224.0.1.76", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+",  "-o", "nas0+", "--ip-dst", "224.0.1.178", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_DEL, UPNP_ACL, "-p", "0x88d9", "-i", "eth+", "-o", "nas0+", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-dst", "224.0.1.65", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-dst", "224.0.1.76", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-dst", "224.0.1.178", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_DEL, UPNP_ACL, "-p", "0x88d9", "-i", "nas0+", "-o", "eth+", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "17", "--ip-dport", "137", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "17", "--ip-dport", "138", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "17", "--ip-dport", "139", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "17", "--ip-dport", "445", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "17", "--ip-dport", "137", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "17", "--ip-dport", "138", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "17", "--ip-dport", "139", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "17", "--ip-dport", "445", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "6", "--ip-dport", "137", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "6", "--ip-dport", "138", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "6", "--ip-dport", "139", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "6", "--ip-dport", "445", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "6", "--ip-dport", "137", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "6", "--ip-dport", "138", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "6", "--ip-dport", "139", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_DEL, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "6", "--ip-dport", "445", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:0:c", "-i", "nas0+", "-o", "eth+", "--ip6-proto", "17", "--ip6-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:0:fb", "-i", "nas0+", "-o", "eth+", "--ip6-proto", "17", "--ip6-dport", "5353", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:1:0:3", "-i", "nas0+", "-o", "eth+", "--ip6-proto", "17", "--ip6-dport", "5355", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:0:f", "-i", "nas0+", "-o", "eth+", "--ip6-proto", "17", "--ip6-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:1:30", "-i", "nas0+", "-o", "eth+", "--ip6-proto", "17", "--ip6-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:0:c", "-i", "eth+", "-o", "nas0+", "--ip6-proto", "17", "--ip6-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:0:fb", "-i", "eth+", "-o", "nas0+", "--ip6-proto", "17", "--ip6-dport", "5353", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:1:0:3", "-i", "eth+", "-o", "nas0+", "--ip6-proto", "17", "--ip6-dport", "5355", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:0:f", "-i", "eth+", "-o", "nas0+", "--ip6-proto", "17", "--ip6-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_DEL, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:1:30", "-i", "eth+", "-o", "nas0+", "--ip6-proto", "17", "--ip6-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_DEL, UPNP_ACL, "-d", "33:33:0:0:0:c", "-i", "nas0+", "-o", "eth+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_DEL, UPNP_ACL, "-d", "33:33:0:0:0:fb", "-i", "nas0+", "-o", "eth+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_DEL, UPNP_ACL, "-d", "33:33:0:1:0:3", "-i", "nas0+", "-o", "eth+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_DEL, UPNP_ACL, "-d", "33:33:0:0:0:f", "-i", "nas0+", "-o", "eth+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_DEL, UPNP_ACL, "-d", "33:33:0:0:1:30", "-i", "nas0+", "-o", "eth+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_DEL, UPNP_ACL, "-d", "33:33:0:0:0:c", "-i", "eth+", "-o", "nas0+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_DEL, UPNP_ACL, "-d", "33:33:0:0:0:fb", "-i", "eth+", "-o", "nas0+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_DEL, UPNP_ACL, "-d", "33:33:0:1:0:3", "-i", "eth+", "-o", "nas0+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_DEL, UPNP_ACL, "-d", "33:33:0:0:0:f", "-i", "eth+", "-o", "nas0+", "-j", "DROP");

	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-dst", "239.255.255.250", "--ip-proto", "17", "--ip-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-dst", "224.0.0.251", "--ip-proto", "17", "--ip-dport", "5353", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-dst", "224.0.0.252", "--ip-proto", "17", "--ip-dport", "5355", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-dst", "239.255.255.246", "--ip-proto", "17", "--ip-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+",  "-o", "nas0+", "--ip-dst", "239.255.255.250", "--ip-proto", "17", "--ip-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+",  "-o", "nas0+", "--ip-dst", "224.0.0.251", "--ip-proto", "17", "--ip-dport", "5353", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+",  "-o", "nas0+", "--ip-dst", "224.0.0.252", "--ip-proto", "17", "--ip-dport", "5355", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+",  "-o", "nas0+", "--ip-dst", "239.255.255.246", "--ip-proto", "17", "--ip-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+",  "-o", "nas0+", "--ip-dst", "224.0.1.65", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+",  "-o", "nas0+", "--ip-dst", "224.0.1.76", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+",  "-o", "nas0+", "--ip-dst", "224.0.1.178", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_ADD, UPNP_ACL, "-p", "0x88d9", "-i", "eth+", "-o", "nas0+", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-dst", "224.0.1.65", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-dst", "224.0.1.76", "-j", "DROP");
	va_cmd(EBTABLES, 12, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-dst", "224.0.1.178", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_ADD, UPNP_ACL, "-p", "0x88d9", "-i", "nas0+", "-o", "eth+", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "17", "--ip-dport", "137", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "17", "--ip-dport", "138", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "17", "--ip-dport", "139", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "17", "--ip-dport", "445", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "17", "--ip-dport", "137", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "17", "--ip-dport", "138", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "17", "--ip-dport", "139", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "17", "--ip-dport", "445", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "6", "--ip-dport", "137", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "6", "--ip-dport", "138", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "6", "--ip-dport", "139", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "nas0+", "-o", "eth+", "--ip-proto", "6", "--ip-dport", "445", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "6", "--ip-dport", "137", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "6", "--ip-dport", "138", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "6", "--ip-dport", "139", "-j", "DROP");
	va_cmd(EBTABLES, 14, 1, FW_ADD, UPNP_ACL, "-p", "IPv4", "-i", "eth+", "-o", "nas0+", "--ip-proto", "6", "--ip-dport", "445", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:0:c", "-i", "nas0+", "-o", "eth+", "--ip6-proto", "17", "--ip6-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:0:fb", "-i", "nas0+", "-o", "eth+", "--ip6-proto", "17", "--ip6-dport", "5353", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:1:0:3", "-i", "nas0+", "-o", "eth+", "--ip6-proto", "17", "--ip6-dport", "5355", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:0:f", "-i", "nas0+", "-o", "eth+", "--ip6-proto", "17", "--ip6-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:1:30", "-i", "nas0+", "-o", "eth+", "--ip6-proto", "17", "--ip6-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:0:c", "-i", "eth+", "-o", "nas0+", "--ip6-proto", "17", "--ip6-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:0:fb", "-i", "eth+", "-o", "nas0+", "--ip6-proto", "17", "--ip6-dport", "5353", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:1:0:3", "-i", "eth+", "-o", "nas0+", "--ip6-proto", "17", "--ip6-dport", "5355", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:0:f", "-i", "eth+", "-o", "nas0+", "--ip6-proto", "17", "--ip6-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 16, 1, FW_ADD, UPNP_ACL, "-p", "IPv6", "-d", "33:33:0:0:1:30", "-i", "eth+", "-o", "nas0+", "--ip6-proto", "17", "--ip6-dport", "1900", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_ADD, UPNP_ACL, "-d", "33:33:0:0:0:c", "-i", "nas0+", "-o", "eth+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_ADD, UPNP_ACL, "-d", "33:33:0:0:0:fb", "-i", "nas0+", "-o", "eth+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_ADD, UPNP_ACL, "-d", "33:33:0:1:0:3", "-i", "nas0+", "-o", "eth+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_ADD, UPNP_ACL, "-d", "33:33:0:0:0:f", "-i", "nas0+", "-o", "eth+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_ADD, UPNP_ACL, "-d", "33:33:0:0:1:30", "-i", "nas0+", "-o", "eth+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_ADD, UPNP_ACL, "-d", "33:33:0:0:0:c", "-i", "eth+", "-o", "nas0+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_ADD, UPNP_ACL, "-d", "33:33:0:0:0:fb", "-i", "eth+", "-o", "nas0+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_ADD, UPNP_ACL, "-d", "33:33:0:1:0:3", "-i", "eth+", "-o", "nas0+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_ADD, UPNP_ACL, "-d", "33:33:0:0:0:f", "-i", "eth+", "-o", "nas0+", "-j", "DROP");
	va_cmd(EBTABLES, 10, 1, FW_ADD, UPNP_ACL, "-d", "33:33:0:0:1:30", "-i", "eth+", "-o", "nas0+", "-j", "DROP");
	return 0;
}

int	deo_skbaclRuleSet(int wan_mode, char *ifname)
{
	/*
	 * ifname : br0 or nas0+
	iptables -A SKB_ACL --ipv4 -i ifname -s 192.168.75.0/24 -j RETURN ; bridge only

	iptables -A SKB_ACL --ipv4 -i ifname -s 175.122.253.0/24 -j RETURN
	iptables -A SKB_ACL --ipv4 -i ifname -s 219.248.49.64/26 -j RETURN
	iptables -A SKB_ACL --ipv4 -i ifname -s 210.94.1.0/24 -j RETURN
	iptables -A SKB_ACL --ipv4 -i ifname -s 123.111.105.0/24 -j RETURN
	iptables -A SKB_ACL --ipv4 -i ifname -s 123.111.106.0/24 -j RETURN
	iptables -A SKB_ACL --ipv4 -i ifname -s 203.236.3.245/32 -j RETURN
	iptables -A SKB_ACL --ipv4 -i ifname -s 61.252.53.130/29 -j RETURN
	iptables -A SKB_ACL --ipv4 -i ifname -s 14.36.46.94/30 -j RETURN
	iptables -A SKB_ACL -p ALL -i ifname -m conntrack --ctstate NEW -j DROP
	*/
	if (wan_mode == DEO_BRIDGE_MODE)
	{
		va_cmd(IPTABLES, 9, 1, FW_DEL, SKB_ACL, "--ipv4", "-i", ifname, "-s", "192.168.75.0/24", "-j", "RETURN");
		va_cmd(IPTABLES, 9, 1, FW_ADD, SKB_ACL, "--ipv4", "-i", ifname, "-s", "192.168.75.0/24", "-j", "RETURN");
	}

	va_cmd(IPTABLES, 9, 1, FW_DEL, SKB_ACL, "--ipv4", "-i", ifname, "-s", "175.122.253.0/24", "-j", "RETURN");
	va_cmd(IPTABLES, 9, 1, FW_DEL, SKB_ACL, "--ipv4", "-i", ifname, "-s", "219.248.49.64/26", "-j", "RETURN");
	va_cmd(IPTABLES, 9, 1, FW_DEL, SKB_ACL, "--ipv4", "-i", ifname, "-s", "210.94.1.0/24", "-j", "RETURN");
	va_cmd(IPTABLES, 9, 1, FW_DEL, SKB_ACL, "--ipv4", "-i", ifname, "-s", "123.111.105.0/24", "-j", "RETURN");
	va_cmd(IPTABLES, 9, 1, FW_DEL, SKB_ACL, "--ipv4", "-i", ifname, "-s", "123.111.106.0/24", "-j", "RETURN");
	va_cmd(IPTABLES, 9, 1, FW_DEL, SKB_ACL, "--ipv4", "-i", ifname, "-s", "203.236.3.245/32", "-j", "RETURN");
	va_cmd(IPTABLES, 9, 1, FW_DEL, SKB_ACL, "--ipv4", "-i", ifname, "-s", "61.252.53.130/29", "-j", "RETURN");
	va_cmd(IPTABLES, 9, 1, FW_DEL, SKB_ACL, "--ipv4", "-i", ifname, "-s", "14.36.46.94/30", "-j", "RETURN");
	va_cmd(IPTABLES, 12, 1, FW_DEL, SKB_ACL, "-p", "ALL", "-i", ifname, "-m", "conntrack", "--ctstate", "NEW", "-j", "DROP");

	if ( 0 == access( "/config/wan_acl", F_OK))
	{
		va_cmd(IPTABLES, 9, 1, FW_DEL, SKB_ACL, "--ipv4", "-i", ifname, "-s", "192.168.1.0/24", "-j", "RETURN");
		va_cmd(IPTABLES, 9, 1, FW_ADD, SKB_ACL, "--ipv4", "-i", ifname, "-s", "192.168.1.0/24", "-j", "RETURN");
	}

	va_cmd(IPTABLES, 9, 1, FW_ADD, SKB_ACL, "--ipv4", "-i", ifname, "-s", "175.122.253.0/24", "-j", "RETURN");
	va_cmd(IPTABLES, 9, 1, FW_ADD, SKB_ACL, "--ipv4", "-i", ifname, "-s", "219.248.49.64/26", "-j", "RETURN");
	va_cmd(IPTABLES, 9, 1, FW_ADD, SKB_ACL, "--ipv4", "-i", ifname, "-s", "210.94.1.0/24", "-j", "RETURN");
	va_cmd(IPTABLES, 9, 1, FW_ADD, SKB_ACL, "--ipv4", "-i", ifname, "-s", "123.111.105.0/24", "-j", "RETURN");
	va_cmd(IPTABLES, 9, 1, FW_ADD, SKB_ACL, "--ipv4", "-i", ifname, "-s", "123.111.106.0/24", "-j", "RETURN");
	va_cmd(IPTABLES, 9, 1, FW_ADD, SKB_ACL, "--ipv4", "-i", ifname, "-s", "203.236.3.245/32", "-j", "RETURN");
	va_cmd(IPTABLES, 9, 1, FW_ADD, SKB_ACL, "--ipv4", "-i", ifname, "-s", "61.252.53.130/29", "-j", "RETURN");
	va_cmd(IPTABLES, 9, 1, FW_ADD, SKB_ACL, "--ipv4", "-i", ifname, "-s", "14.36.46.94/30", "-j", "RETURN");
	va_cmd(IPTABLES, 12, 1, FW_ADD, SKB_ACL, "-p", "ALL", "-i", ifname, "-m", "conntrack", "--ctstate", "NEW", "-j", "DROP");

	return 0;
}
#endif

int block_br_lan_igmp_mld_rules(void)
{
#define BR_IGMP_MLD_DROP "BR_IGMP_MLD_DROP"
	int igmpSnoopEnabled = isIGMPSnoopingEnabled() ;
#ifdef CONFIG_IPV6
	int mldSnoopEnabled = isMLDSnoopingEnabled() ;
#endif

	va_cmd(EBTABLES, 8, 1, "-D", (char*)FW_BR_LAN_FORWARD, "-p", "IPv4", "--ip-proto", "igmp", "-j", BR_IGMP_MLD_DROP);
#ifdef CONFIG_IPV6
	va_cmd(EBTABLES, 10, 1, "-D", (char*)FW_BR_LAN_FORWARD, "-p", "IPv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "143:143/0:255", "-j", BR_IGMP_MLD_DROP);
	va_cmd(EBTABLES, 10, 1, "-D", (char*)FW_BR_LAN_FORWARD, "-p", "IPv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "130:132/0:255", "-j", BR_IGMP_MLD_DROP);
#endif
	va_cmd(EBTABLES, 2, 1, "-F", BR_IGMP_MLD_DROP);
	va_cmd(EBTABLES, 2, 1, "-X", BR_IGMP_MLD_DROP);

	if(igmpSnoopEnabled
#ifdef CONFIG_IPV6
		|| mldSnoopEnabled
#endif
	)
	{
		va_cmd(EBTABLES, 2, 1, "-N", BR_IGMP_MLD_DROP);
		va_cmd(EBTABLES, 3, 1, "-P", BR_IGMP_MLD_DROP, "RETURN");
		va_cmd(EBTABLES, 8, 1, "-A", BR_IGMP_MLD_DROP, "-i", "eth+", "-o", "eth+", "-j", "DROP");
#ifdef WLAN_SUPPORT
		va_cmd(EBTABLES, 8, 1, "-A", BR_IGMP_MLD_DROP, "-i", "eth+", "-o", "wlan+", "-j", "DROP");
		va_cmd(EBTABLES, 8, 1, "-A", BR_IGMP_MLD_DROP, "-i", "wlan+", "-o", "eth+", "-j", "DROP");
		va_cmd(EBTABLES, 8, 1, "-A", BR_IGMP_MLD_DROP, "-i", "wlan+", "-o", "wlan+", "-j", "DROP");
#endif
		if(igmpSnoopEnabled)
		{
			va_cmd(EBTABLES, 8, 1, "-A", (char*)FW_BR_LAN_FORWARD, "-p", "IPv4", "--ip-proto", "igmp", "-j", BR_IGMP_MLD_DROP);
		}
#ifdef CONFIG_IPV6
		if(mldSnoopEnabled)
		{
			va_cmd(EBTABLES, 10, 1, "-A", (char*)FW_BR_LAN_FORWARD, "-p", "IPv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "143:143/0:255", "-j", BR_IGMP_MLD_DROP);
			va_cmd(EBTABLES, 10, 1, "-A", (char*)FW_BR_LAN_FORWARD, "-p", "IPv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "130:132/0:255", "-j", BR_IGMP_MLD_DROP);
		}
#endif
	}

	return 0;
}

int block_br_lan_rules()
{
#ifdef CONFIG_YUEME
	struct in_addr ip_addr;
	char gip[60] = {0};
	if (mib_get_s(MIB_ADSL_LAN_IP, (void *)&ip_addr, sizeof(ip_addr)) != 0 &&
		inet_ntop(AF_INET, &ip_addr, gip, sizeof(gip)) != NULL)
	{
		va_cmd(EBTABLES, 13, 1, "-A", (char*)FW_BR_LAN_FORWARD, "--logical-in", BRIF, "-i", "!", "nas0+", "-p", "arp", "--arp-ip-dst", gip, "-j", "DROP");
		va_cmd(EBTABLES, 13, 1, "-A", (char*)FW_BR_LAN_FORWARD, "--logical-in", BRIF, "-i", "!", "nas0+", "-p", "arp", "--arp-ip-src", gip, "-j", "DROP");
	}
	if (mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)gip, sizeof(gip)) != 0)
	{
#ifdef CONFIG_IPV6
		va_cmd(EBTABLES, 17, 1, "-A", (char*)FW_BR_LAN_FORWARD, "--logical-in", BRIF, "-i", "!", "nas0+", "-p", "ip6", "--ip6-src", gip, "--ip6-protocol", "ipv6-icmp", "--ip6-icmp-type", "135:136","-j", "DROP");//drop v6 lan ip from wan
#endif
	}
#endif

	block_br_lan_igmp_mld_rules();

	return 0;
}

#ifdef PPPOE_PASSTHROUGH
static int setupBrPppoe(void)
{
	int total;
	int i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ] = {0};

	// check all wan interfaces
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if(!Entry.enable)
			continue;

		ifGetName(PHY_INTF(Entry.ifIndex), ifname, sizeof(ifname));

#ifdef CONFIG_E8B
		if ((Entry.cmode != CHANNEL_MODE_BRIDGE) && (Entry.brmode == BRIDGE_PPPOE))
#else
		if (Entry.brmode == BRIDGE_PPPOE)
#endif
		{
			va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_BR_PPPOE, "-i", ifname,"-j", FW_BR_PPPOE_ACL);
			va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_BR_PPPOE, "-o", ifname,"-j", FW_BR_PPPOE_ACL);
		}
	}

	return 0;
}
#endif

#ifdef CONFIG_NETFILTER_XT_MATCH_PSD
int setup_psd(void)
{
	unsigned char enable = 0;

	mib_get_s(MIB_PSD_ENABLE, &enable, sizeof(enable));
	va_cmd(IPTABLES, 8, 1, FW_DEL, FW_INPUT, "-m", "psd", "-i", "nas+", "-j", FW_DROP);
	va_cmd(IPTABLES, 8, 1, FW_DEL, FW_INPUT, "-m", "psd", "-i", "ppp+", "-j", FW_DROP);


	if(enable){
		va_cmd(IPTABLES, 8, 1, FW_INSERT, FW_INPUT, "-m", "psd", "-i", "nas+", "-j", FW_DROP);
		va_cmd(IPTABLES, 8, 1, FW_INSERT, FW_INPUT, "-m", "psd", "-i", "ppp+", "-j", FW_DROP);
	}

#ifdef VIRTUAL_SERVER_SUPPORT
	setupVtlsvr(VIRTUAL_SERVER_DELETE);
	setupVtlsvr(VIRTUAL_SERVER_ADD);
#endif

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_flow_flush();
#endif
	return 0;
}
#endif

#ifdef LAYER7_FILTER_SUPPORT
int setupAppFilter(void)
{
	int entryNum,i;
	LAYER7_FILTER_T Entry;

	// Delete all Appfilter rule
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_APPFILTER);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_APP_P2PFILTER);
	va_cmd(IPTABLES, 4, 1, ARG_T, "mangle", "-F", (char *)FW_APPFILTER);

	entryNum = mib_chain_total(MIB_LAYER7_FILTER_TBL);

	for(i=0;i<entryNum;i++)
	{
		if (!mib_chain_get(MIB_LAYER7_FILTER_TBL, i, (void *)&Entry))
			return -1;

		if(!strcmp("smtp",Entry.appname)){
			//iptables -A appfilter -i br0 -p TCP --dport 25 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_APPFILTER, ARG_I, LANIF,
				"-p", ARG_TCP, FW_DPORT, "25", "-j", (char *)FW_DROP);
			continue;
		}
		else if(!strcmp("pop3",Entry.appname)){
			//iptables -A appfilter -i br0 -p TCP --dport 110 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_APPFILTER, ARG_I, LANIF,
				"-p", ARG_TCP, FW_DPORT, "110", "-j", (char *)FW_DROP);
			continue;
		}
		else if(!strcmp("bittorrent",Entry.appname)){
			//iptables -A appp2pfilter -m ipp2p --bit -j DROP
			va_cmd(IPTABLES, 7, 1, (char *)FW_ADD, (char *)FW_APP_P2PFILTER, "-m", "ipp2p",
				"--bit", "-j", (char *)FW_DROP);
			continue;
		}
		else if(!strcmp("chinagame",Entry.appname)){
			//iptables -A appfilter -i br0 -p TCP --dport 8000 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_APPFILTER, ARG_I, LANIF,
				"-p", ARG_TCP, FW_DPORT, "8000", "-j", (char *)FW_DROP);
			continue;
		}
		else if(!strcmp("gameabc",Entry.appname)){
			//iptables -A appfilter -i br0 -p TCP --dport 5100 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_APPFILTER, ARG_I, LANIF,
				"-p", ARG_TCP, FW_DPORT, "5100", "-j", (char *)FW_DROP);
			continue;
		}
		else if(!strcmp("haofang",Entry.appname)){
			//iptables -A appfilter -i br0 -p TCP --dport 1201 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_APPFILTER, ARG_I, LANIF,
				"-p", ARG_TCP, FW_DPORT, "1201", "-j", (char *)FW_DROP);
			continue;
		}
		else if(!strcmp("ourgame",Entry.appname)){
			//iptables -A appfilter -i br0 -p TCP --dport 2000 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_APPFILTER, ARG_I, LANIF,
				"-p", ARG_TCP, FW_DPORT, "2000", "-j", (char *)FW_DROP);
		}


		// iptables -t mangle -A appfilter -m layer7 --l7proto qq -j DROP
		va_cmd(IPTABLES, 10, 1, ARG_T, "mangle", (char *)FW_ADD, (char *)FW_APPFILTER, "-m", "layer7",
			"--l7proto", Entry.appname, "-j", (char *)FW_DROP);

	}

#ifdef VIRTUAL_SERVER_SUPPORT
	setupVtlsvr(VIRTUAL_SERVER_DELETE);
	setupVtlsvr(VIRTUAL_SERVER_ADD);
#endif
	return 0;
}
#endif
#ifdef PARENTAL_CTRL
//Uses Global variable for keep watching timeout
static MIB_PARENT_CTRL_T parentctrltable[MAX_PARENTCTRL_USER_NUM] = {0};
/********************************
 *
 *	Initialization. load from flash
 *
 ********************************/
//return if this mac should be filtered now!
static int parent_ctrl_check(MIB_PARENT_CTRL_T *entry)
{
	time_t tm;
	struct tm the_tm;
	int		tmp1,tmp2,tmp3;;

	time(&tm);
	memcpy(&the_tm, localtime(&tm), sizeof(the_tm));

	if (((entry->controlled_day) & (1 << the_tm.tm_wday))!=0)
	{
		tmp1 = entry->start_hr * 60 +  entry->start_min;
		tmp2 = entry->end_hr * 60 +  entry->end_min;
		tmp3 = the_tm.tm_hour *60 + the_tm.tm_min;
		if ((tmp3 >= tmp1) && (tmp3 < tmp2) )
			return 1;
	}

		return 0;

}

int parent_ctrl_table_init(void)
{
	int total,i;
	MIB_PARENT_CTRL_T	entry;

	memset(&parentctrltable[0],0, sizeof(MIB_PARENT_CTRL_T)*MAX_PARENTCTRL_USER_NUM);
	total = mib_chain_total(MIB_PARENTAL_CTRL_TBL);
	if (total >= MAX_PARENTCTRL_USER_NUM)
	{
		total = MAX_PARENTCTRL_USER_NUM -1;
		printf("ERROR, CHECK!");
	}
	for ( i=0; i<total; i++)
	{
		mib_chain_get(MIB_PARENTAL_CTRL_TBL, i, &parentctrltable[i]);
	}
}

int parent_ctrl_table_add(MIB_PARENT_CTRL_T *addedEntry)
{
	int	i;

	for (i = 0; i < MAX_PARENTCTRL_USER_NUM; i++)
	{
		if (strlen(parentctrltable[i].username) == 0)
		{
			break;
		}
	}
	addedEntry->cur_state = 0;
	memcpy (&parentctrltable[i],addedEntry, sizeof(MIB_PARENT_CTRL_T));

	parent_ctrl_table_rule_update();
}

int parent_ctrl_table_del(MIB_PARENT_CTRL_T *addedEntry)
{
	int	i;
	char macaddr[20];
	char ipRangeStr[32];
	unsigned char sipStr[16]={0};
	unsigned char eipStr[16]={0};

	for (i = 0; i < MAX_PARENTCTRL_USER_NUM; i++)
	{
		if ( !strcmp(parentctrltable[i].username, addedEntry->username))
		{
			if (parentctrltable[i].cur_state == 1)
			{
				//del the entry
				if (parentctrltable[i].specfiedPC)
				{
					snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
						parentctrltable[i].mac[0], parentctrltable[i].mac[1],
						parentctrltable[i].mac[2], parentctrltable[i].mac[3],
						parentctrltable[i].mac[4], parentctrltable[i].mac[5]);
					va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "mac",
						"--mac-source",  macaddr, "-j", "DROP");
#ifdef CONFIG_IPV6
					va_cmd(IP6TABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "mac",
						"--mac-source",  macaddr, "-j", "DROP");
#endif
				}
				else
				{
					strncpy(sipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].sip)), 16);
					sipStr[15] = '\0';
					strncpy(eipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].eip)), 16);
					eipStr[15] = '\0';
					sprintf(ipRangeStr, "%s-%s", sipStr, eipStr);
					va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "iprange",
						"--src-range",  ipRangeStr, "-j", "DROP");
#ifdef CONFIG_IPV6
					va_cmd(IP6TABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "iprange",
						"--src-range",  ipRangeStr, "-j", "DROP");
#endif
				}
			}
			//remove the iptable here if it exists
			memset(&parentctrltable[i],0, sizeof(MIB_PARENT_CTRL_T));
			break;
		}
	}
}
int parent_ctrl_table_delall()
{
	int	i;
	char macaddr[20];
	char ipRangeStr[32];
	unsigned char sipStr[16]={0};
	unsigned char eipStr[16]={0};

	for (i = 0; i < MAX_PARENTCTRL_USER_NUM; i++)
	{
		if (parentctrltable[i].cur_state == 1)
		{
			//del the entry
			if (parentctrltable[i].specfiedPC)
			{
				snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
					parentctrltable[i].mac[0], parentctrltable[i].mac[1],
					parentctrltable[i].mac[2], parentctrltable[i].mac[3],
					parentctrltable[i].mac[4], parentctrltable[i].mac[5]);
				va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
					(char *)ARG_I, (char *)LANIF, "-m", "mac",
					"--mac-source",  macaddr, "-j", "DROP");
#ifdef CONFIG_IPV6
				va_cmd(IP6TABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
					(char *)ARG_I, (char *)LANIF, "-m", "mac",
					"--mac-source",  macaddr, "-j", "DROP");
#endif
			}
			else
			{
				strncpy(sipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].sip)), 16);
				sipStr[15] = '\0';
				strncpy(eipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].eip)), 16);
				eipStr[15] = '\0';
				sprintf(ipRangeStr, "%s-%s", sipStr, eipStr);
				va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
					(char *)ARG_I, (char *)LANIF, "-m", "iprange",
					"--src-range",  ipRangeStr, "-j", "DROP");
#ifdef CONFIG_IPV6
				va_cmd(IP6TABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
					(char *)ARG_I, (char *)LANIF, "-m", "iprange",
					"--src-range",  ipRangeStr, "-j", "DROP");
#endif
			}
		}

	}
	memset(&parentctrltable[0],0, sizeof(MIB_PARENT_CTRL_T)*MAX_PARENTCTRL_USER_NUM);
}

// update the rules to iptables according to current time
int parent_ctrl_table_rule_update()
{
	int i, check;
	char macaddr[20];
	char ipRangeStr[32];
	unsigned char sipStr[16]={0};
	unsigned char eipStr[16]={0};
	unsigned char parentalCtrlOn;

	if (mib_get_s(MIB_PARENTAL_CTRL_ENABLE, (void *)&parentalCtrlOn, sizeof(parentalCtrlOn)) == 0)
		parentalCtrlOn = 0;

	if(!parentalCtrlOn)
		return 1;

	for (i = 0; i < MAX_PARENTCTRL_USER_NUM; i++)
	{
		if (strlen(parentctrltable[i].username) > 0)
		{

			check = parent_ctrl_check(&parentctrltable[i]);

			if (( check == 1) &&
				(parentctrltable[i].cur_state == 0))
			{
				parentctrltable[i].cur_state = 1;

				//add the entry
				if (parentctrltable[i].specfiedPC)
				{
					snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
					parentctrltable[i].mac[0], parentctrltable[i].mac[1],
					parentctrltable[i].mac[2], parentctrltable[i].mac[3],
					parentctrltable[i].mac[4], parentctrltable[i].mac[5]);

					//for debug
					va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "mac",
						"--mac-source",  macaddr, "-j", "DROP");
#ifdef CONFIG_IPV6
					va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "mac",
						"--mac-source",  macaddr, "-j", "DROP");
#endif
				}
				else
				{
					strncpy(sipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].sip)), 16);
					sipStr[15] = '\0';
					strncpy(eipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].eip)), 16);
					eipStr[15] = '\0';
					sprintf(ipRangeStr, "%s-%s", sipStr, eipStr);
					va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "iprange",
						"--src-range",  ipRangeStr, "-j", "DROP");
#ifdef CONFIG_IPV6
					va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "iprange",
						"--src-range",  ipRangeStr, "-j", "DROP");
#endif
				}
			}
			else if ((check == 0) &&
				(parentctrltable[i].cur_state == 1))
			{
				parentctrltable[i].cur_state = 0;
				//del the entry
				if (parentctrltable[i].specfiedPC)
				{
					snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
					parentctrltable[i].mac[0], parentctrltable[i].mac[1],
					parentctrltable[i].mac[2], parentctrltable[i].mac[3],
					parentctrltable[i].mac[4], parentctrltable[i].mac[5]);

					va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "mac",
						"--mac-source",  macaddr, "-j", "DROP");
#ifdef CONFIG_IPV6
					va_cmd(IP6TABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "mac",
						"--mac-source",  macaddr, "-j", "DROP");
#endif
				}
				else
				{
					strncpy(sipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].sip)), 16);
					sipStr[15] = '\0';
					strncpy(eipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].eip)), 16);
					eipStr[15] = '\0';
					sprintf(ipRangeStr, "%s-%s", sipStr, eipStr);
					va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "iprange",
						"--src-range",  ipRangeStr, "-j", "DROP");
#ifdef CONFIG_IPV6
					va_cmd(IP6TABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "iprange",
						"--src-range",  ipRangeStr, "-j", "DROP");
#endif
				}
			}

		}
	}
	return 1;
}
#endif

#ifdef NATIP_FORWARDING
static void fw_setupIPForwarding(void)
{
	int i, total;
	char local[16], remote[16], enable;
	MIB_CE_IP_FW_T Entry;

	// Add New chain on filter and nat
	// iptables -N ipfw
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IPTABLE_IPFW);
	// iptables -A FORWARD -j ipfw
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)IPTABLE_IPFW);
	// iptables -t nat -N ipfw
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)IPTABLE_IPFW);
	// iptables -t nat -A PREROUTING -j ipfw
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)IPTABLE_IPFW);
	// iptables -t nat -N ipfw2
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)IPTABLE_IPFW2);
	// iptables -t nat -A POSTROUTING -j ipfw2
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", (char *)IPTABLE_IPFW2);


	mib_get_s(MIB_IP_FW_ENABLE, (void *)&enable, sizeof(enable));
	if (!enable)
		return;

	total = mib_chain_total(MIB_IP_FW_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_IP_FW_TBL, i, (void *)&Entry))
			continue;
		strncpy(local, inet_ntoa(*((struct in_addr *)Entry.local_ip)), 16);
		strncpy(remote, inet_ntoa(*((struct in_addr *)Entry.remote_ip)), 16);
		// iptables -t nat -A ipfw -d remoteip ! -i $LAN_IF -j DNAT --to-destination localip
		va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_ADD,	(char *)IPTABLE_IPFW, "-d", remote, "!", (char *)ARG_I, (char *)LANIF, "-j",
			"DNAT", "--to-destination", local);
		// iptables -t nat -A ipfw2 -s localip -o ! br0 -j SNAT --to-source remoteip
		va_cmd(IPTABLES, 13, 1, "-t", "nat", FW_ADD, (char *)IPTABLE_IPFW2, "!", (char *)ARG_O, (char *)LANIF, "-s", local, "-j", "SNAT", "--to-source", remote);

		// iptables -A ipfw2 -d localip ! -i $LAN_IF -o $LAN_IF -j RETURN
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD,
			(char *)IPTABLE_IPFW, "-d", local, "!", (char *)ARG_I,
			(char *)LANIF, (char *)ARG_O,
			(char *)LANIF, "-j", (char *)FW_ACCEPT);
	}
}
#endif

#ifndef CONFIG_RTL8672NIC
void setupIPQoSflag(int flag)
{
	if(flag)
		system("ethctl setipqos 1");
	else
		system("ethctl setipqos 0");
}
#endif

#ifdef PORT_TRIGGERING_STATIC
static void parse_and_add_triggerPort(char *inRange, PROTO_TYPE_T prot, char *ip)
{
	int parseLen, j;
	char tempStr1[10]={0},tempStr2[10]={0};
	char *p, dstPortRange[32];
	int idx=0,shift=0;

	parseLen = strlen(inRange);
	if (prot == PROTO_TCP)
		p = (char *)ARG_TCP;
	else
		p = (char *)ARG_UDP;

	for(j=0;j<GAMING_MAX_RANGE;j++)
	{
		if(((inRange[j]>='0')&&(inRange[j]<='9')))
		{
			if(shift>=9) continue;
			if(idx==0)
				tempStr1[shift++]=inRange[j];
			else
				tempStr2[shift++]=inRange[j];

		}
		else if((inRange[j]==',')||
				(inRange[j]=='-')||
				(inRange[j]==0))
		{
			if(idx==0)
				tempStr1[shift]=0;
			else
				tempStr2[shift]=0;

			shift=0;
			if((inRange[j]==',')||
				(inRange[j]==0))
			{
				/*
				uint16 inStart,inFinish;
				inStart=atoi(tempStr1);
				if(idx==0)
					inFinish=inStart;
				else
					inFinish=atoi(tempStr2);
				*/
				dstPortRange[0] = '\0';
				if (idx==0) // single port number
					strncpy(dstPortRange, tempStr1, 32);
				else {
					if (strlen(tempStr1)!=0 && strlen(tempStr2)!=0)
						snprintf(dstPortRange, 32, "%s:%s", tempStr1, tempStr2);
				}

				idx=0;

				// iptables -t nat -A PREROUTING -i ! $LAN_IF -p TCP --dport dstPortRange -j DNAT --to-destination ipaddr
				va_cmd(IPTABLES, 15, 1, "-t", "nat",
					(char *)FW_ADD,	(char *)IPTABLES_PORTTRIGGER,
					 "!", (char *)ARG_I, (char *)LANIF,
					"-p", p,
					(char *)FW_DPORT, dstPortRange, "-j",
					"DNAT", "--to-destination", ip);

				// iptables -I ipfilter 3 -i ! $LAN_IF -o $LAN_IF -p TCP --dport dstPortRange -j RETURN
				va_cmd(IPTABLES, 13, 1, (char *)FW_ADD,
					(char *)IPTABLES_PORTTRIGGER, "!", (char *)ARG_I,
					(char *)LANIF, (char *)ARG_O,
					(char *)LANIF, "-p", p,
					(char *)FW_DPORT, dstPortRange, "-j",
					(char *)FW_ACCEPT);
				/*
				//make inFinish always bigger than inStart
				if(inStart>inFinish)
				{
					uint16 temp;
					temp=inFinish;
					inFinish=inStart;
					inStart=temp;
				}

				if(!((inStart==0)||(inFinish==0)))
				{
					rtl8651a_addTriggerPortRule(
					dsid, //dsid
					inType,inStart,inFinish,
					outType,
					outStart,
					outFinish,localIp);
//					printf("inRange=%d-%d\n inType=%d\n",inStart,inFinish,inType);
				}
				*/
			}
			else
			{
				idx++;
			}
			if(inRange[j]==0)
				break;
		}
	}
}

static void setupPortTriggering(void)
{
	int i, total;
	char ipaddr[16];
	MIB_CE_PORT_TRG_T Entry;

	total = mib_chain_total(MIB_PORT_TRG_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_PORT_TRG_TBL, i, (void *)&Entry))
			continue;
		if(!Entry.enable)
			continue;

		strncpy(ipaddr, inet_ntoa(*((struct in_addr *)Entry.ip)), 16);

		parse_and_add_triggerPort(Entry.tcpRange, PROTO_TCP, ipaddr);
		parse_and_add_triggerPort(Entry.udpRange, PROTO_UDP, ipaddr);
	}
}
#endif // of PORT_TRIGGERING_STATIC

/*
 *	Clamp TCPMSS in FORWARD chain
 */
static int clamp_tcpmss(MIB_CE_ATM_VC_Tp pEntry)
{
	char ifname[IFNAMSIZ];
	char mss[8], mss_s[16];

	ifGetName( pEntry->ifIndex, ifname, sizeof(ifname));
	sprintf(mss, "%d", pEntry->mtu-40); /* IP Header + TCP Header = 40 */
	sprintf(mss_s, "%d:65495", pEntry->mtu-40);
	va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char *)FW_FORWARD,
		"-p", "tcp", "-o", (char *)ifname, "--tcp-flags", "SYN,RST", "SYN", "-j",
		"TCPMSS", "--clamp-mss-to-pmtu");
	va_cmd(IPTABLES, 17, 1, (char *)FW_ADD, (char *)FW_FORWARD,
		"-p", "tcp", "-i", (char *)ifname, "--tcp-flags", "SYN,RST", "SYN", "-m" , "tcpmss", "--mss", mss_s, "-j",
		"TCPMSS", "--set-mss", mss);

#ifdef CONFIG_IPV6
	if (pEntry->IpProtocol & IPVER_IPV6) {
		sprintf(mss, "%d", pEntry->mtu-60); /* IP Header + TCP Header = 60 */
		sprintf(mss_s, "%d:65475", pEntry->mtu-60);
		va_cmd(IP6TABLES, 12, 1, (char *)FW_ADD, (char *)FW_FORWARD,
			"-p", "tcp", "-o", (char *)ifname, "--tcp-flags", "SYN,RST", "SYN", "-j",
			"TCPMSS", "--clamp-mss-to-pmtu");
		va_cmd(IP6TABLES, 17, 1, (char *)FW_ADD, (char *)FW_FORWARD,
			"-p", "tcp", "-i", (char *)ifname, "--tcp-flags", "SYN,RST", "SYN", "-m" , "tcpmss", "--mss", mss_s, "-j",
			"TCPMSS", "--set-mss", mss);
	}
#endif
	return 0;
}

#if defined(CONFIG_USER_RTK_BRIDGE_MACFILTER_SUPPORT)
// Mac Filter for bridge opmode
void setup_br_macfilter(void)
{
	unsigned char origVal;

	mib_get(MIB_OP_MODE, (void *)&origVal);

	if(origVal == BRIDGE_MODE)
	{
		va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_MACFILTER_FWD);
		#ifdef CONFIG_E8B
		setupMacFilterTables();
		#else
		setupMacFilter();
		#endif
		// ebtables -A FORWARD -j macfilter
		//add macfilter chain to forward CHAIN for bridge opmode
		va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_MACFILTER_FWD);
		va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_MACFILTER_FWD);
	}
	else{
		va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)FW_MACFILTER_FWD);
		va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_INPUT, "-j", (char *)FW_MACFILTER_FWD);
		va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_MACFILTER_FWD);
	}
}
#endif

static int filter_set_tcpmss(void)
{
	int i, total;
	MIB_CE_ATM_VC_T Entry;
#ifdef CONFIG_USER_PPPOMODEM
	MIB_WAN_3G_T Entry_3G;
#endif
	#define NET_MTU 1500
	#define PPPoE_MTU 1492

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;
		if (Entry.enable == 0 || Entry.cmode == CHANNEL_MODE_BRIDGE)
			continue;
		if (Entry.cmode == CHANNEL_MODE_PPPOE || Entry.cmode == CHANNEL_MODE_PPPOA) {
			if (Entry.mtu < PPPoE_MTU)
				clamp_tcpmss(&Entry);
		}
		else {
			if(Entry.mtu < NET_MTU)
				clamp_tcpmss(&Entry);
		}
	}

#ifdef CONFIG_USER_PPPOMODEM
	if(mib_chain_get( MIB_WAN_3G_TBL, 0, (void*)&Entry_3G) == 1){
		if (Entry_3G.enable) {
			if(Entry_3G.mtu < PPPoE_MTU) {
				// clamp_tcpmss needs ifIndex and mtu
				Entry.ifIndex = TO_IFINDEX(MEDIA_3G,  MODEM_PPPIDX_FROM, 0);
				Entry.mtu = Entry_3G.mtu;
				clamp_tcpmss(&Entry);
			}
		}
	}
#endif

#ifdef CONFIG_E8B
	// setup for all interface tcpmss for --clamp-mss-to-pmtu to match all inteface
	va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_FORWARD,
		"-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j",
		"TCPMSS", "--clamp-mss-to-pmtu");
#else
#ifndef USE_DEONET /* sghong, 20221215, not used ppp+ interface */
	// setup default PPPoE tcpmss (1492-40=1452)
	va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char *)FW_FORWARD,
		"-p", "tcp", "-o", "ppp+", "--tcp-flags", "SYN,RST", "SYN", "-j",
		"TCPMSS", "--clamp-mss-to-pmtu");
	va_cmd(IPTABLES, 17, 1, (char *)FW_ADD, (char *)FW_FORWARD,
		"-p", "tcp", "-i", "ppp+", "--tcp-flags", "SYN,RST", "SYN", "-m" , "tcpmss", "--mss", "1452:65495", "-j",
		"TCPMSS", "--set-mss", "1452");
#endif
#endif

#ifdef CONFIG_IPV6
#ifdef CONFIG_E8B
	// setup for all interface tcpmss for --clamp-mss-to-pmtu to match all inteface
	va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_FORWARD,
		"-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j",
		"TCPMSS", "--clamp-mss-to-pmtu");
#else
#ifndef USE_DEONET /* sghong, 20221215, not used ppp+ interface */
	/* It is automatically set the MSS to the proper value (PMTU minus 60 bytes,
	 which should be a reasonable value for most applications. */
	va_cmd(IP6TABLES, 12, 1, (char *)FW_ADD, (char *)FW_FORWARD,
		"-p", "tcp", "-o", "ppp+", "--tcp-flags", "SYN,RST", "SYN", "-j",
		"TCPMSS", "--clamp-mss-to-pmtu");
	va_cmd(IP6TABLES, 17, 1, (char *)FW_ADD, (char *)FW_FORWARD,
		"-p", "tcp", "-i", "ppp+", "--tcp-flags", "SYN,RST", "SYN", "-m" , "tcpmss", "--mss", "1432:65475", "-j",
		"TCPMSS", "--set-mss", "1432");
#endif
#endif
#endif
	return 0;
}

int setup_dscp_remark_rule_pre(void)
{
	char dscp_mark[64], dscp_val_str[32];
	char chain_name[64];
	int dscp_val;

	snprintf(chain_name, sizeof(chain_name), "%s_pre", FW_DSCP_REMARK);
	va_cmd_no_error(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", chain_name);
	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)chain_name);
	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-X", (char *)chain_name);
#ifdef CONFIG_IPV6
	va_cmd_no_error(IP6TABLES, 6, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", chain_name);
	va_cmd_no_error(IP6TABLES, 4, 1, "-t", "mangle", "-F", (char *)chain_name);
	va_cmd_no_error(IP6TABLES, 4, 1, "-t", "mangle", "-X", (char *)chain_name);
#endif

	snprintf(dscp_mark, sizeof(dscp_mark), "0x%llx", (SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_MASK|SOCK_MARK2_DSCP_DOWN_BIT_MASK|SOCK_MARK2_DSCP_REMARK_ENABLE_MASK|SOCK_MARK2_DSCP_BIT_MASK));
	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-N", (char *)chain_name);
	va_cmd_no_error(IPTABLES, 9, 1, "-t", "mangle", (char *)FW_ADD, chain_name, "-j", "CONNMARK2", "--restore-mark2", "--mask2", dscp_mark);
	va_cmd_no_error(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", chain_name);
#ifdef CONFIG_IPV6
	va_cmd_no_error(IP6TABLES, 4, 1, "-t", "mangle", "-N", (char *)chain_name);
	va_cmd_no_error(IP6TABLES, 9, 1, "-t", "mangle", (char *)FW_ADD, chain_name, "-j", "CONNMARK2", "--restore-mark2", "--mask2", dscp_mark);
	va_cmd_no_error(IP6TABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", chain_name);
#endif

	return 0;
}

int setup_dscp_remark_rule_post(void)
{
	char dscp_mark[64], dscp_val_str[32];
	char dscp_up_chain_name[64], dscp_down_chain_name[64];
	int dscp_val;

	snprintf(dscp_up_chain_name, sizeof(dscp_up_chain_name), "%s_UP", FW_DSCP_REMARK);
	snprintf(dscp_down_chain_name, sizeof(dscp_down_chain_name), "%s_DOWN", FW_DSCP_REMARK);

	va_cmd_no_error(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-j", FW_DSCP_REMARK);
	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)FW_DSCP_REMARK);
	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-X", (char *)FW_DSCP_REMARK);
	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)dscp_up_chain_name);
	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-X", (char *)dscp_up_chain_name);
	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)dscp_down_chain_name);
	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-X", (char *)dscp_down_chain_name);
#ifdef CONFIG_IPV6
	va_cmd_no_error(IP6TABLES, 6, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-j", FW_DSCP_REMARK);
	va_cmd_no_error(IP6TABLES, 4, 1, "-t", "mangle", "-F", (char *)FW_DSCP_REMARK);
	va_cmd_no_error(IP6TABLES, 4, 1, "-t", "mangle", "-X", (char *)FW_DSCP_REMARK);
	va_cmd_no_error(IP6TABLES, 4, 1, "-t", "mangle", "-F", (char *)dscp_up_chain_name);
	va_cmd_no_error(IP6TABLES, 4, 1, "-t", "mangle", "-X", (char *)dscp_up_chain_name);
	va_cmd_no_error(IP6TABLES, 4, 1, "-t", "mangle", "-F", (char *)dscp_down_chain_name);
	va_cmd_no_error(IP6TABLES, 4, 1, "-t", "mangle", "-X", (char *)dscp_down_chain_name);
#endif

	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-N", (char *)FW_DSCP_REMARK);
	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-N", (char *)dscp_up_chain_name);
	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-N", (char *)dscp_down_chain_name);
#ifdef CONFIG_IPV6
	va_cmd_no_error(IP6TABLES, 4, 1, "-t", "mangle", "-N", (char *)FW_DSCP_REMARK);
	va_cmd_no_error(IP6TABLES, 4, 1, "-t", "mangle", "-N", (char *)dscp_up_chain_name);
	va_cmd_no_error(IP6TABLES, 4, 1, "-t", "mangle", "-N", (char *)dscp_down_chain_name);
#endif

	for(dscp_val=0; dscp_val<64; dscp_val++)
	{
		snprintf(dscp_mark, sizeof(dscp_mark), "0x%llx/0x%llx", SOCK_MARK2_DSCP_SET(0,dscp_val),SOCK_MARK2_DSCP_BIT_MASK);
		snprintf(dscp_val_str, sizeof(dscp_val_str), "0x%x", dscp_val);
		va_cmd_no_error(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)dscp_up_chain_name, "-m", "mark2", "--mark2", dscp_mark, "-j", "DSCP", "--set-dscp", dscp_val_str);
#ifdef CONFIG_IPV6
		va_cmd_no_error(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)dscp_up_chain_name, "-m", "mark2", "--mark2", dscp_mark, "-j", "DSCP", "--set-dscp", dscp_val_str);
#endif
	}
	snprintf(dscp_mark, sizeof(dscp_mark), "0x%llx/0x%llx", SOCK_MARK2_DSCP_REMARK_ENABLE_SET(0,1), SOCK_MARK2_DSCP_REMARK_ENABLE_MASK);
	va_cmd_no_error(IPTABLES, 13, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_DSCP_REMARK, "!", "-o", LANIF_ALL, "-m", "mark2", "--mark2", dscp_mark, "-j", (char *)dscp_up_chain_name);
#ifdef CONFIG_IPV6
	va_cmd_no_error(IP6TABLES, 13, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_DSCP_REMARK, "!", "-o", LANIF_ALL, "-m", "mark2", "--mark2", dscp_mark, "-j", (char *)dscp_up_chain_name);
#endif

	for(dscp_val=0; dscp_val<64; dscp_val++)
	{
		snprintf(dscp_mark, sizeof(dscp_mark), "0x%llx/0x%llx", SOCK_MARK2_DSCP_DOWN_SET(0,dscp_val),SOCK_MARK2_DSCP_DOWN_BIT_MASK);
		snprintf(dscp_val_str, sizeof(dscp_val_str), "0x%x", dscp_val);
		va_cmd_no_error(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)dscp_down_chain_name, "-m", "mark2", "--mark2", dscp_mark, "-j", "DSCP", "--set-dscp", dscp_val_str);
#ifdef CONFIG_IPV6
		va_cmd_no_error(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)dscp_down_chain_name, "-m", "mark2", "--mark2", dscp_mark, "-j", "DSCP", "--set-dscp", dscp_val_str);
#endif
	}
	snprintf(dscp_mark, sizeof(dscp_mark), "0x%llx/0x%llx", SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_SET(0,1), SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_MASK);
	va_cmd_no_error(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_DSCP_REMARK, "-o", LANIF_ALL, "-m", "mark2", "--mark2", dscp_mark, "-j", (char *)dscp_down_chain_name);
#ifdef CONFIG_IPV6
	va_cmd_no_error(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_DSCP_REMARK, "-o", LANIF_ALL, "-m", "mark2", "--mark2", dscp_mark, "-j", (char *)dscp_down_chain_name);
#endif

	snprintf(dscp_mark, sizeof(dscp_mark), "0x%llx", (SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_MASK|SOCK_MARK2_DSCP_DOWN_BIT_MASK|SOCK_MARK2_DSCP_REMARK_ENABLE_MASK|SOCK_MARK2_DSCP_BIT_MASK));
	va_cmd_no_error(IPTABLES, 9, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_DSCP_REMARK, "-j", "CONNMARK2", "--save-mark2", "--mask2", dscp_mark);
#ifdef CONFIG_IPV6
	va_cmd_no_error(IP6TABLES, 9, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_DSCP_REMARK, "-j", "CONNMARK2", "--save-mark2", "--mask2", dscp_mark);
#endif

	va_cmd_no_error(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-j", FW_DSCP_REMARK);
#ifdef CONFIG_IPV6
	va_cmd_no_error(IP6TABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-j", FW_DSCP_REMARK);
#endif
	return 0;
}

int setup_dscp_remark_rule_normal(void)
{
	setup_dscp_remark_rule_pre();

	return 0;
}

int setup_dscp_remark_rule_high(void)
{
	setup_dscp_remark_rule_post();

	return 0;
}

#if defined(CONFIG_USER_DATA_ELEMENTS_AGENT) || defined(CONFIG_USER_DATA_ELEMENTS_COLLECTOR)
static int setup_de_firewall()
{
	unsigned char role = DE_ROLE_DISABLED;
	unsigned int port = 0;
	unsigned char port_str[32] = {0};

	mib_get_s(MIB_DATA_ELEMENT_ROLE, (void *)&role, sizeof(role));
	mib_get_s(MIB_DATA_ELEMENT_PORT, (void *)&port, sizeof(port));
	if(port==0)
		port = DATA_ELEMENT_DEF_PORT;
	sprintf(port_str, "%d", port);

	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_DATA_ELEMENT);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)IP6TABLE_DATA_ELEMENT);
#endif

	if(getOpMode()==BRIDGE_MODE)
		return 0;

	if(role==DE_ROLE_AGENT)
	{
		// iptables -A data_element -p tcp --dport port -j ACCEPT
		va_cmd(IPTABLES, 8, 1, "-A", IPTABLE_DATA_ELEMENT, "-p", "tcp", (char *)FW_DPORT, port_str, "-j", "ACCEPT");
#ifdef CONFIG_IPV6
		// ip6tables -A ipv6_data_element -p tcp --dport port -j ACCEPT
		va_cmd(IP6TABLES, 8, 1, "-A", IP6TABLE_DATA_ELEMENT, "-p", "tcp", (char *)FW_DPORT, port_str, "-j", "ACCEPT");
#endif
	}else if(role==DE_ROLE_COLLECTOR)
	{
		//TODO
	}

	return 0;
}

#endif

#if (defined(IP_PORT_FILTER) || defined(DMZ)) && defined(CONFIG_TELMEX_DEV)
void filter_patch_for_dmz(void)
{
	MIB_CE_PORT_FW_T PfEntry;
	int vInt, total, i;
	unsigned char value[32];
	char ipaddr[32];
	char intPort[16];
	char *argv[16];
	int idx=0;

	va_cmd(IPTABLES, 2, 1, "-F", FW_DMZFILTER);

	vInt = 0;
	if (mib_get_s(MIB_DMZ_ENABLE, (void *)value, sizeof(value)) != 0)
		vInt = (int)(*(unsigned char *)value);
	if (vInt == 1)
	{
		if (mib_get_s(MIB_DMZ_IP, (void *)value, sizeof(value)) != 0)
		{
			strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
			ipaddr[15] = '\0';

			//iptables -A dmz_patch -p tcp -i br0 -s xx.xx.xx.xx -m state --state NEW --tcp-flags SYN,ACK ACK -j ACCEPT
			va_cmd(IPTABLES, 17, 1, (char *)FW_ADD, (char *)FW_DMZFILTER, "-p", "tcp", "-i", (char *)LANIF_ALL, "-s", ipaddr,
				"-m", "state", "--state", "NEW", "--tcp-flags", "SYN,ACK", "ACK", "-j", (char *)FW_ACCEPT);
		}
	}

	vInt = 0;
	if (mib_get_s(MIB_PORT_FW_ENABLE, (void *)value, sizeof(value)) != 0)
		vInt = (int)(*(unsigned char *)value);
	if (vInt == 1)
	{
		//iptables -A FORWARD -p tcp -i br0 -s 192.168.1.39 --sport 1000 -j ACCEPT
		argv[1] = "-A";
		argv[2] = (char *)FW_DMZFILTER;
		argv[3] = "-p";
		argv[4] = "tcp";
		argv[5] = "-i";
		argv[6] = (char *)LANIF;

		total = mib_chain_total(MIB_PORT_FW_TBL);
		for (i=0; i<total; i++)
		{
			mib_chain_get(MIB_PORT_FW_TBL, i, (void *)&PfEntry);

			if (PfEntry.dynamic == 1)
				continue;

			idx = 7;
			if (PfEntry.protoType == PROTO_TCP || PfEntry.protoType == PROTO_UDPTCP)
			{
				snprintf(ipaddr, sizeof(ipaddr), "%s", inet_ntoa(*((struct in_addr *)PfEntry.ipAddr)));
				argv[idx++] = "-s";
				argv[idx++] = ipaddr;

				if (PfEntry.fromPort == PfEntry.toPort)
				{
					snprintf(intPort, sizeof(intPort), "%u", PfEntry.fromPort);
				}
				else
				{
					/* "%u-%u" is used by port forwarding */
					snprintf(intPort, sizeof(intPort), "%u-%u", PfEntry.fromPort, PfEntry.toPort);
				}
				argv[idx++] = "--sport";
				argv[idx++] = intPort;
				argv[idx++] = "-j";
				argv[idx++] = (char *)FW_ACCEPT;
				argv[idx++] = NULL;

				do_cmd(IPTABLES, argv, 1);
			}
		}
	}

	//iptables -A dmz_patch -p tcp -i br0 -m state --state NEW --tcp-flags SYN,ACK ACK -j DROP
	va_cmd(IPTABLES, 15, 1, (char *)FW_ADD, (char *)FW_DMZFILTER, "-p", "tcp", "-i", (char *)LANIF_ALL, "-m", "state",
		"--state", "NEW", "--tcp-flags", "SYN,ACK", "ACK", "-j", (char *)FW_DROP);
}
#endif
void mac_autobinding_setup_mac_accept_rule(unsigned char *mac){
	char macstring[32]={0};
	snprintf(macstring,sizeof(macstring)-1,"%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	va_cmd(IP6TABLES, 12, 1, (char *)FW_INSERT, (char *)IPTABLE_IPV6_MAC_AUTOBINDING_RS, "-m","mac","--mac-source",macstring,"-p", "icmpv6",
		"--icmpv6-type","router-solicitation","-j", (char *)FW_ACCEPT);
}

void mac_autobinding_delete_mac_accept_rule(unsigned char *mac){
	char macstring[32]={0};
	snprintf(macstring,sizeof(macstring)-1,"%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	va_cmd(IP6TABLES, 12, 1, (char *)FW_DEL, (char *)IPTABLE_IPV6_MAC_AUTOBINDING_RS, "-m","mac","--mac-source",macstring,"-p", "icmpv6",
		"--icmpv6-type","router-solicitation","-j", (char *)FW_ACCEPT);
}

void mac_autobinding_setup_default_rule(){
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)IPTABLE_IPV6_MAC_AUTOBINDING_RS, "-p", "icmpv6",
		"--icmpv6-type","router-solicitation","-j", (char *)FW_DROP);
}
void mac_autobinding_restore_mac_rule(){
		MIB_CE_CMCC_DHCPCLIENT_MAC_T entry;
		int i = 0;
		int total = mib_chain_total(MIB_CMCC_DHCPCLIENT_MAC_TBL);
		for(i=0;i<total;i++){
			if(mib_chain_get(MIB_CMCC_DHCPCLIENT_MAC_TBL,i,&entry)){
				printf("%s %d %02x:%02x:%02x:%02x:%02x:%02x\n",__FUNCTION__,__LINE__,
					entry.Mac[0],entry.Mac[1],entry.Mac[2],entry.Mac[3],entry.Mac[4],entry.Mac[5]);
				mac_autobinding_setup_mac_accept_rule(entry.Mac);
			}
		}
}

void mac_autobinding_del_mac_rule(unsigned char *mac){
		MIB_CE_CMCC_DHCPCLIENT_MAC_T entry;
		int i = 0;
		int total = mib_chain_total(MIB_CMCC_DHCPCLIENT_MAC_TBL);
		for(i=0;i<total;i++){
			if(mib_chain_get(MIB_CMCC_DHCPCLIENT_MAC_TBL,i,&entry)){
				if(!memcmp(mac,entry.Mac,sizeof(entry.Mac))){
					printf("%s %d %02x:%02x:%02x:%02x:%02x:%02x\n",__FUNCTION__,__LINE__,
						entry.Mac[0],entry.Mac[1],entry.Mac[2],entry.Mac[3],entry.Mac[4],entry.Mac[5]);
					mib_chain_delete(MIB_CMCC_DHCPCLIENT_MAC_TBL,i);
					mac_autobinding_delete_mac_accept_rule(mac);
					break;
				}
			}
		}
}
void mac_autobinding_del_ra_client(unsigned char *ip6addr){
		MIB_CE_CMCC_RACLIENT_ADDR_T entry;
		int i = 0;
		int total = mib_chain_total(MIB_CMCC_RACLIENT_ADDR_TBL);
		for(i=0;i<total;i++){
			if(mib_chain_get(MIB_CMCC_RACLIENT_ADDR_TBL,i,&entry)){
				char tmpbuf[128]={0};
				inet_ntop(AF_INET6, ip6addr, tmpbuf, sizeof(tmpbuf));
				if(!strcmp(tmpbuf,entry.Addr)){
					printf("%s %d ipv6 addr %s\n",__FUNCTION__,__LINE__,entry.Addr);
					mib_chain_delete(MIB_CMCC_RACLIENT_ADDR_TBL,i);
					//send signal to radvd to reload config
					int radvdpid=read_pid((char *)RADVD_PID);
					if(radvdpid>0){
						kill(radvdpid, SIGHUP);
					}
					break;
				}
			}
		}
}



// Execute firewall rules
// return value:
// 1  : successful
int setupFirewall(int isBoot)
{
	char *argv[20];
	unsigned char value[32];
	int vInt, i, total, vcNum;
	MIB_CE_ATM_VC_T Entry;
	char *policy, *filterSIP, *filterDIP, srcPortRange[12], dstPortRange[12];
	char ipaddr[32], srcip[20], dstip[20];
	char ifname[16], extra[32];
	char srcmacaddr[18], dstmacaddr[18];
#ifdef IP_PASSTHROUGH
	unsigned int ippt_itf;
#endif
	int spc_enable, spc_ip;
	// Added by Mason Yu for ACL
	unsigned char aclEnable, domainEnable, cwmp_aclEnable;
	unsigned char dhcpvalue[32];
	unsigned char vChar;
	int dhcpmode;
	int wan_mode;
	// Added by Mason Yu for URL Blocking
#if defined( URL_BLOCKING_SUPPORT)||defined( URL_ALLOWING_SUPPORT)
	unsigned char urlEnable = 0;
#endif
#ifdef NAT_CONN_LIMIT
	unsigned char connlimitEn;
#endif
	unsigned char isIpv6Enable = 0;

#if defined(CONFIG_IPV6) &&defined(CONFIG_RTK_IPV6_LOGO_TEST)
	//Ready && CE-router Logo do not need policy route / firewall / portmapping
	unsigned char logoTestMode=0;
	if ( mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode)) ){
		if (logoTestMode)
			return 0;
	}
#endif

	//-------------------------------------------------
	// Filter table
	//-------------------------------------------------
	//--------------- set policies --------------
#ifdef REMOTE_ACCESS_CTL
	// iptables -P INPUT DROP
	va_cmd(IPTABLES, 3, 1, "-P", (char *)FW_INPUT, (char *)FW_DROP);
#else // IP_ACL
	// iptables -P INPUT ACCEPT
	va_cmd(IPTABLES, 3, 1, "-P", (char *)FW_INPUT, (char *)FW_ACCEPT);
#endif

	// iptables -P FORWARD ACCEPT
	va_cmd(IPTABLES, 3, 1, "-P", (char *)FW_FORWARD, (char *)FW_ACCEPT);

#if defined(CONFIG_USER_PPTP_SERVER)
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IPTABLE_PPTP_VPN_IN);
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IPTABLE_PPTP_VPN_FW);
#endif
#if defined(CONFIG_USER_L2TPD_LNS)
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IPTABLE_L2TP_VPN_IN);
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IPTABLE_L2TP_VPN_FW);
	setup_l2tp_server_firewall();
#endif

#if defined(CONFIG_CT_AWIFI_JITUAN_SMARTWIFI)
	unsigned char functype=0;
	mib_get(AWIFI_PROVINCE_CODE, &functype);
	if(functype == AWIFI_ZJ && isBoot){
		awifi_preFirewall_enable();
	}
#endif

#ifdef CONFIG_DEV_xDSL
#ifdef CONFIG_USER_IGMPPROXY
	/* 	Due to NIC driver use bit 0~2 of skb mark to VLAN priority,
	 *	and IGMP kernel module will set reserved_tailroom in igmpv3_newpack()
	 *	but skb mark/reserved_tailroom is a union struct (include/linux/skbuff.h)
	 *	So, we weset skb mark to 0.
	 *	PS. this rule can not work on OUTPUT chain of mangle table
	 */
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IGMP_OUTPUT_VPRIO_RESET);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-j", (char *)IGMP_OUTPUT_VPRIO_RESET);
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)IGMP_OUTPUT_VPRIO_RESET, "-p", "igmp", "-j", "MARK", "--set-mark", "0x0");
//	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)IGMP_OUTPUT_VPRIO_RESET, "-p", "igmp", "-j", "MARK", "--set-mark", "0x0/0x7");
#endif
#if (defined(CONFIG_IPV6) && defined(CONFIG_USER_MLDPROXY))
	/* 	Due to NIC driver use bit 0~2 of skb mark to VLAN priority,
	 *	and IGMP kernel module will set reserved_tailroom in mld_newpack()
	 *	but skb mark/reserved_tailroom is a union struct (include/linux/skbuff.h)
	 *	So, we weset skb mark to 0.
	 *	PS. this rule can not work on OUTPUT chain of mangle table
	 */
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)MLD_OUTPUT_VPRIO_RESET);
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-j", (char *)MLD_OUTPUT_VPRIO_RESET);
	va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)MLD_OUTPUT_VPRIO_RESET, "-p", "icmpv6", "--icmpv6-type", "143", "-j", "MARK", "--set-mark", "0x0");
//	va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)MLD_OUTPUT_VPRIO_RESET, "-p", "icmpv6", "--icmpv6-type", "143", "-j", "MARK", "--set-mark", "0x0/0x7");
#endif
#endif

/* for deonet iptables mac filter on Bridge mode */
#ifdef MAC_FILTER
		//	Add Chain(macfilter)
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_MACFILTER_FWD);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_MACFILTER_FWD);
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#ifdef REMOTE_ACCESS_CTL
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_ICMP_FILTER);
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, "INPUT", "-p", "icmp", "-j", (char *)FW_ICMP_FILTER);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_ICMP_FILTER);
	va_cmd(IP6TABLES, 6, 1, (char *)FW_ADD, "INPUT", "-p", "icmpv6", "-j", (char *)FW_ICMP_FILTER);
#endif
	set_icmp_Firewall(1);
#endif
#endif

#ifdef REMOTE_ACCESS_CTL
	// accept related
	// iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
#if defined(CONFIG_00R0)
	char ipfilter_spi_state=1;
	mib_get_s(MIB_IPFILTER_SPI_ENABLE, (void*)&ipfilter_spi_state, sizeof(ipfilter_spi_state));
	if(ipfilter_spi_state != 0)
#endif
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-m", "state",
		"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);
#endif

	// It must be in the beginning of FORWARD chain.
	filter_set_tcpmss();

#if defined(CONFIG_YUEME) || defined(CONFIG_CU)
	va_cmd(IPTABLES, 4, 1, "-D", FW_FORWARD, "-j", LXC_FWD_TBL);
	va_cmd(IPTABLES, 2, 1, "-F", LXC_FWD_TBL);
	va_cmd(IPTABLES, 2, 1, "-N", LXC_FWD_TBL);
	va_cmd(IPTABLES, 4, 1, "-A", FW_FORWARD, "-j", LXC_FWD_TBL);
	va_cmd(IPTABLES, 6, 1, "-A", LXC_FWD_TBL, "-i", "veth+", "-j", "ACCEPT");
	va_cmd(IPTABLES, 6, 1, "-A", LXC_FWD_TBL, "-o", "veth+", "-j", "ACCEPT");
	va_cmd(IPTABLES, 6, 1, "-A", LXC_FWD_TBL, "-i", "lxcbr0", "-j", "ACCEPT");
	va_cmd(IPTABLES, 6, 1, "-A", LXC_FWD_TBL, "-o", "lxcbr0", "-j", "ACCEPT");

	va_cmd(IPTABLES, 4, 1, "-D", FW_INPUT, "-j", LXC_IN_TBL);
	va_cmd(IPTABLES, 2, 1, "-F", LXC_IN_TBL);
	va_cmd(IPTABLES, 2, 1, "-N", LXC_IN_TBL);
	va_cmd(IPTABLES, 4, 1, "-A", FW_INPUT, "-j", LXC_IN_TBL);
	va_cmd(IPTABLES, 6, 1, "-A", LXC_IN_TBL, "-i", "veth+", "-j", "ACCEPT");
	va_cmd(IPTABLES, 6, 1, "-A", LXC_IN_TBL, "-i", "lxcbr0", "-j", "ACCEPT");
#endif

#ifdef CONFIG_USER_FON
	setFonFirewall();
#endif
#ifdef CONFIG_USER_ROUTED_ROUTED
	// Kaohj -- allowing RIP (in case of ACL blocking)
#ifdef CONFIG_CU
	//iptables -A INPUT -i br0 -p udp --dport 520 -j ACCEPT
	va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_INPUT, "-i", LANIF_ALL, "-p",
		"udp", (char *)FW_DPORT, "520", "-j", (char *)FW_ACCEPT);
#else
	// iptables -A INPUT -p udp --dport 520 -j ACCEPT
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p",
		"udp", (char *)FW_DPORT, "520", "-j", (char *)FW_ACCEPT);
#endif
#endif

	// Drop UPnP SSDP from the WAN side
    va_cmd(IPTABLES, 9, 1, (char *)FW_ADD, (char *)FW_INPUT, "!", (char *)ARG_I, (char *)LANIF_ALL, "-d", "239.255.255.250", "-j", (char *)FW_DROP);

	// Allowing multicast access, ie. IGMP, RIPv2
	// iptables -A INPUT -d 224.0.0.0/4 -j ACCEPT
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_INPUT, "-d",
		"224.0.0.0/4", "-j", (char *)FW_ACCEPT);

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#ifdef WLAN_SUPPORT
	setup_wlan_accessRule_netfilter_iptables_init();
#endif
#endif

#ifdef CONFIG_ANDLINK_SUPPORT
	// Add chain for andlink
	va_cmd(IPTABLES, 2, 1, "-N", "andlink");
	mib_get(MIB_RTL_LINK_ENABLE, (void *) &vChar);
	if(vChar==1){
		filter_set_andlink(1);
	}
	//iptables -A INPUT -j andlink
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", "andlink");
#endif

	/* lo rule for ping our inteface IP,otherwise we can't ping our IP address on interface.
	 * iptables -A INPUT -i lo -j ACCEPT */
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_INPUT, "-i", "lo","-j", (char *)FW_ACCEPT);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 6, 1, (char *)FW_ADD, (char *)FW_INPUT, "-i", "lo","-j", (char *)FW_ACCEPT);
#endif
#if defined(CONFIG_USER_DATA_ELEMENTS_AGENT) || defined(CONFIG_USER_DATA_ELEMENTS_COLLECTOR)
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IPTABLE_DATA_ELEMENT);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, "INPUT", "-j", (char *)IPTABLE_DATA_ELEMENT);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)IP6TABLE_DATA_ELEMENT);
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, "INPUT", "-j", (char *)IP6TABLE_DATA_ELEMENT);
#endif
	setup_de_firewall();
#endif
#ifdef CONFIG_DEONET_DOS // Deonet GNT2400R
	// iptables -N droplist
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_DROPLIST);
	// Iptables -D INPUT -i nas+ -m state --state NEW -j DROP
	//va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_INPUT, (char *)ARG_I, "nas0_0", "-m", "state", "--state", "NEW", "-j", (char *)FW_DROP);
	va_cmd(IPTABLES, 2, 1, "-N", (char *)SKB_ACL);
	va_cmd(IPTABLES, 2, 1, "-N", (char *)RATE_ACL);
	va_cmd(IPTABLES, 2, 1, "-N", (char *)CPU_RATE_ACL);

	// drop for icmp unreachable
	va_cmd(IPTABLES, 8, 1, (char *)FW_INSERT, (char *)FW_OUTPUT, "-p", "icmp", "--icmp-type", "destination-unreachable", "-j", (char *)FW_DROP);
#endif
#ifdef IP_ACL
	//  Add Chain(aclblock) for ACL
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_ACL);
	// Add chain(aclblock) on nat
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)FW_ACL);
	// Add chain(aclblock) on nat
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", (char *)FW_ACL);
	restart_acl();
#ifdef CONFIG_00R0
	/* Only take effect on WAN */
	//iptables -A INPUT ! -i br+ -j aclblock
	va_cmd(IPTABLES, 7, 1, (char *)FW_ADD, (char *)FW_INPUT, "!", "-i", LANIF_ALL, "-j", (char *)FW_ACL);
	// iptables -t nat -A PREROUTING -j aclblock
	va_cmd(IPTABLES, 9, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "!", "-i", LANIF_ALL, "-j", (char *)FW_ACL);
	// iptables -t mangle -A PREROUTING -j aclblock
	va_cmd(IPTABLES, 9, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "!", "-i", LANIF_ALL, "-j", (char *)FW_ACL);
#else
#ifdef CONFIG_DEONET_DOS // Deonet GNT2400R
	wan_mode = deo_wan_nat_mode_get();

	if (wan_mode == DEO_NAT_MODE)
		strcpy(ifname, "nas0+");
	else
		strcpy(ifname, "br0");

	// iptables -A INPUT -j droplist
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_DROPLIST);

	/* "iptables -I INPUT --ipv4 -i %s -p udp --dport 68 -j ACCEPT" */
	va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, "INPUT", "--ipv4", "-i", ifname, "-p", "udp", "--dport", "68", "-j", FW_ACCEPT);

	//iptables -A INPUT -j aclblock
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_ACL);
	
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, "INPUT", "-j", (char *)SKB_ACL);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, "INPUT", "-j", (char *)RATE_ACL);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, "INPUT", "-j", (char *)CPU_RATE_ACL);
#endif

	// iptables -t nat -A PREROUTING -j aclblock
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)FW_ACL);
	// iptables -t mangle -A PREROUTING -j aclblock
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)FW_ACL);
#endif
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_ACL);
	mib_get_s(MIB_V6_ACL_CAPABILITY, (void *)&aclEnable, sizeof(aclEnable));
	restart_v6_acl();
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_ACL);
#endif
#endif


#ifdef _CWMP_MIB_
	//  Add Chain(cwmp_aclblock) for ACL
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_CWMP_ACL);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", (char *)FW_CWMP_ACL);

	mib_get_s(CWMP_ACL_ENABLE, (void *)&cwmp_aclEnable, sizeof(cwmp_aclEnable));
	//if (cwmp_aclEnable == 1)  // ACL Capability is enabled
	//	filter_set_cwmp_acl(1);
	restart_cwmp_acl();

	//iptables -A INPUT -j cwmp_aclblock
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_CWMP_ACL);
	// iptables -t mangle -A PREROUTING -j cwmp_aclblock
	va_cmd_no_error(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", (char *)FW_CWMP_ACL);
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)FW_CWMP_ACL);
#endif
#if defined(CONFIG_USER_DHCPCLIENT_MODE) && defined(CONFIG_CMCC) && defined(CONFIG_CMCC_ANDLINK_DEV_SDK)
	va_cmd(IPTABLES, 2, 1, "-N", (char *)REDIRECT_URL_TO_BR0);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)REDIRECT_URL_TO_BR0);
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)REDIRECT_URL_TO_BR0);
#endif

#ifdef CONFIG_USER_AVALANCH_DETECT
	setup_avalanch_detect_fw();
#endif

#ifdef NAT_CONN_LIMIT
	//Add Chain(connlimit) for NAT conn limit
	va_cmd(IPTABLES, 2, 1, "-N", "connlimit");

	mib_get_s(MIB_NAT_CONN_LIMIT, (void *)&connlimitEn, sizeof(connlimitEn));
	if (connlimitEn == 1)
		set_conn_limit();

	//iptables -A FORWARD -j connlimit
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", "connlimit");
#endif
#ifdef TCP_UDP_CONN_LIMIT
	//Add Chain(connlimit) for NAT conn limit
	va_cmd(IPTABLES, 2, 1, "-N", "connlimit");

	mib_get_s(MIB_CONNLIMIT_ENABLE, (void *)&vChar, sizeof(vChar));
	if (vChar == 1)
		set_conn_limit();
	//iptables -A FORWARD -j connlimit
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", "connlimit");
#endif

	// setup chain for lan forward blocking
	va_cmd(EBTABLES, 4, 1, "-N", FW_BR_LAN_FORWARD, "-P", "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, "FORWARD", "-j", FW_BR_LAN_FORWARD);
	block_br_lan_rules();

#ifdef USE_DEONET /* sghong, 20230406 */
	va_cmd(EBTABLES, 4, 1, "-N", FW_BR_PRIVATE_DHCP_SERVER_INPUT, "-P", "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, "INPUT", "-j", FW_BR_PRIVATE_DHCP_SERVER_INPUT);

	deo_dhcpLanToLanBlock();
	deo_privateDHCPServerPacketBlock();

	va_cmd(EBTABLES, 4, 1, "-N", FW_BR_DHCPRELAY6_INPUT, "-P", "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, "INPUT", "-j", FW_BR_DHCPRELAY6_INPUT);
	va_cmd(EBTABLES, 4, 1, "-N", FW_BR_DHCPRELAY6_FORWARD, "-P", "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, "FORWARD", "-j", FW_BR_DHCPRELAY6_FORWARD);
	va_cmd(EBTABLES, 4, 1, "-N", FW_BR_DHCPRELAY6_OUTPUT, "-P", "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, "OUTPUT", "-j", FW_BR_DHCPRELAY6_OUTPUT);

	va_cmd(EBTABLES, 4, 1, "-N", FW_BR_DHCPRELAY_INPUT, "-P", "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, "INPUT", "-j", FW_BR_DHCPRELAY_INPUT);
	va_cmd(EBTABLES, 4, 1, "-N", FW_BR_DHCPRELAY_FORWARD, "-P", "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, "FORWARD", "-j", FW_BR_DHCPRELAY_FORWARD);
	va_cmd(EBTABLES, 4, 1, "-N", FW_BR_DHCPRELAY_OUTPUT, "-P", "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, "OUTPUT", "-j", FW_BR_DHCPRELAY_OUTPUT);

	if(deo_wan_nat_mode_get() == DEO_BRIDGE_MODE)
		deo_dhcprelayRuleSet();

	isIpv6Enable = deo_ipv6_status_get();
	/* ipv6 enable 시 dhcp relay 관련 rule 설정 */
	if (isIpv6Enable == 1)
		deo_dhcpv6relayRuleSet();
#endif

	block_br_wan();

#ifdef USE_DEONET /* sghong, 20230705 */
	deo_IPv6RuleSet();
	deo_dhcpClientOnBridge();

	//va_cmd(EBTABLES, 6, 1, "-t", "broute", "-N", FW_BR_LANTOLAN_DSCP_BROUTE, "-P", "RETURN");
	//va_cmd(EBTABLES, 6, 1, "-t", "broute", (char *)FW_ADD, "BROUTING", "-j", FW_BR_LANTOLAN_DSCP_BROUTE);

	va_cmd(EBTABLES, 6, 1, "-t", "broute", "-N", FW_BR_OLTTOLAN_BROUTE, "-P", "RETURN");
	va_cmd(EBTABLES, 6, 1, "-t", "broute", (char *)FW_ADD, "BROUTING", "-j", FW_BR_OLTTOLAN_BROUTE);

	va_cmd(EBTABLES, 4, 1, "-N", FW_BR_OLTTOLAN_FORWARD, "-P", "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, "FORWARD", "-j", FW_BR_OLTTOLAN_FORWARD);


	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", FW_BR_ICMPV6_TX_MANGLE_OUTPUT);
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, "OUTPUT", "-j", FW_BR_ICMPV6_TX_MANGLE_OUTPUT);

	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", FW_BR_ICMPV6_TX_MANGLE_POSTROUTING);
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, "POSTROUTING", "-j", FW_BR_ICMPV6_TX_MANGLE_POSTROUTING);

	va_cmd(EBTABLES, 4, 1, "-N", FW_BR_ICMPV6_TX_OUTPUT, "-P", "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, "OUTPUT", "-j", FW_BR_ICMPV6_TX_OUTPUT);

	//deo_lantolanDSCPSetRules();
	if (isIpv6Enable == 1)
		deo_ipv6OltToLANRules();

	if ((deo_wan_nat_mode_get() == DEO_BRIDGE_MODE) || (isIpv6Enable == 1))
		deo_icmpv6CpuTxRules();

#endif

#ifdef CONFIG_IPV6
	block_br_wan_forward();
#endif
#if defined(CONFIG_00R0) && defined(CONFIG_USER_INTERFACE_GROUPING)
	block_interfce_group_default_rule();
#endif

#ifdef CONFIG_USER_RTK_VOIP
	voip_setup_iptable(0);
#endif

#ifdef WLAN_SUPPORT
	// setup chain for wlan blocking
	va_cmd(EBTABLES, 4, 1, "-N", "wlan_block", "-P", "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, "FORWARD", "-j", "wlan_block");
#endif

#if defined(CONFIG_BOLOCK_IPTV_FROM_LAN_SERVER)
	//drop multicast data drom lna port

	setup_lan_to_lan_multicast_block_rules();
#endif
#if defined(CONFIG_USER_PORT_ISOLATION)
	rtk_lan_handle_lan_port_isolation();
#endif

#ifdef PPPOE_PASSTHROUGH
	//  W.H. Hung: Add Chain br_pppoe for bridged PPPoE
	va_cmd(EBTABLES, 2, 1, "-N", FW_BR_PPPOE);
	va_cmd(EBTABLES, 3, 1, "-P", FW_BR_PPPOE, "RETURN");
	va_cmd(EBTABLES, 2, 1, "-N", FW_BR_PPPOE_ACL);
	va_cmd(EBTABLES, 3, 1, "-P", FW_BR_PPPOE_ACL, "DROP");
	// and to FORWORD chain
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", FW_BR_PPPOE);
	// and to FORWORD bridge PPPoE ACL
	va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_PPPOE_ACL,
		"--proto", "0x8100", "--vlan-encap", "0x8863", "-j", "RETURN");
	va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_PPPOE_ACL,
		"--proto", "0x8100", "--vlan-encap", "0x8864", "-j", "RETURN");
	va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_BR_PPPOE_ACL,
		"--proto", "0x8863", "-j", "RETURN");
	va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_BR_PPPOE_ACL,
		"--proto", "0x8864", "-j", "RETURN");
	va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_PPPOE_ACL,
		"-p", "IPv4", "-d", "01:00:5e:00:00:00/ff:ff:ff:00:00:00", "-j", "RETURN");
	va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_PPPOE_ACL,
		"-p", "IPv6", "-d", "33:33:00:00:00:00/ff:ff:00:00:00:00", "-j", "RETURN");
	setupBrPppoe();
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
	//  Add Chain(domainblk) for ACL
	va_cmd(IPTABLES, 2, 1, "-N", "domainblk");
	//iptables -t mangle -N mark_for_dns_filter
	va_cmd(IPTABLES, 4, 1,(char *)ARG_T, "mangle","-N", (char *)MARK_FOR_DOMAIN);
	//iptables -t mangle -I PREROUTING -j mark_for_dns_filter
	va_cmd(IPTABLES, 6, 1, (char *)ARG_T, "mangle",(char *)FW_INSERT, (char *)FW_PREROUTING, "-j", (char *)MARK_FOR_DOMAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", "domainblk");
#endif
	mib_get_s(MIB_DOMAINBLK_CAPABILITY, (void *)&domainEnable, sizeof(domainEnable));
	if (domainEnable == 1)  // Domain blocking Capability is enabled
		filter_set_domain(1);

	filter_set_domain_default();
	patch_for_office365();

	//iptables -A INPUT -j domainblk
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", "domainblk");

	//iptables -A FORWARD -j domainblk
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", "domainblk");

	//iptables -A OUTPUT -j domainblk
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, "OUTPUT", "-j", "domainblk");
#ifdef CONFIG_IPV6
	//ip6tables -A INPUT -j domainblk
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", "domainblk");
	//ip6tables -A FORWARD -j domainblk
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", "domainblk");
	//ip6tables -A OUTPUT -j domainblk
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, "OUTPUT", "-j", "domainblk");
#endif
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
#ifndef CONFIG_E8B
	/* support prohibit ntp dns request from DOD PPPoE wan */
	va_cmd(IPTABLES, 2, 1, "-N", "ntpblk");
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, "OUTPUT", "-j", "ntpblk");
#endif
#endif

#ifdef IP_PASSTHROUGH
	// IP Passthrough, LAN access
	set_IPPT_LAN_access();
#endif

#ifdef CONFIG_IPV6
	do{
		unsigned char mib_auto_mac_binding_iptv_bridge_wan_mode=0;
		if(mib_get(MIB_AUTO_MAC_BINDING_IPTV_BRIDGE_WAN,&mib_auto_mac_binding_iptv_bridge_wan_mode)&&mib_auto_mac_binding_iptv_bridge_wan_mode==2){
			va_cmd(IP6TABLES, 2, 1, "-N", (char *)IPTABLE_IPV6_MAC_AUTOBINDING_RS);
			va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, "INPUT", "-j", (char *)IPTABLE_IPV6_MAC_AUTOBINDING_RS);
			mac_autobinding_restore_mac_rule();
			mac_autobinding_setup_default_rule();
		}
	}while(0);
#endif

#ifdef CONFIG_USER_CWMP_TR069
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IPTABLE_TR069);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, "INPUT", "-j", (char *)IPTABLE_TR069);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)IPTABLE_IPV6_TR069);
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, "INPUT", "-j", (char *)IPTABLE_IPV6_TR069);
#endif
	set_TR069_Firewall(1);
#endif

#if defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_)
	rtk_layer2bridging_set_group_firewall_rule(-1, 1);
#endif

#ifdef REMOTE_ACCESS_CTL
	// Add chain for remote access
	// iptables -N inacc
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_INACC);
	va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_INACC,
		 "!", (char *)ARG_I, LANIF, "-m", "mark", "--mark", RMACC_MARK, "-j", FW_ACCEPT);

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	//va_cmd(IPTABLES, 11, 1, "-A", (char *)FW_INACC,
	//	 "!", (char *)ARG_I, LANIF, "-m", "mark", "--mark", RMACC_MARK, "-j", FW_ACCEPT);

	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_IN_COMMING);
	va_cmd(IPTABLES, 4, 1, "-A", (char *)FW_INACC, "-j", (char *)FW_IN_COMMING);
#endif
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_INACC);
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_IN_COMMING);
	va_cmd(IP6TABLES, 4, 1, "-A", (char *)FW_INPUT, "-j", (char *)FW_IN_COMMING);
#endif
#endif

	filter_set_remote_access(1);

#ifdef CONFIG_USER_DHCP_SERVER
	// Added by Mason Yu for dhcp Relay. Open DHCP Relay Port for Incoming Packets.
	#ifdef REMOTE_ACCESS_CTL
	if (mib_get_s(MIB_DHCP_MODE, (void *)dhcpvalue, sizeof(dhcpvalue)) != 0)
	{
		dhcpmode = (unsigned int)(*(unsigned char *)dhcpvalue);
		if (dhcpmode == DHCP_LAN_RELAY) {
			// iptables -A inacc -i ! br+ -p udp --sport 67 --dport 67 -j ACCEPT
			va_cmd(IPTABLES, 13, 1, (char *)FW_ADD, (char *)FW_INACC, "!", (char *)ARG_I, LANIF_ALL, "-p", "udp", (char *)FW_SPORT, (char *)PORT_DHCP, (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_ACCEPT);
		}
	}
	#endif
#endif

#ifdef CONFIG_USER_ROUTED_ROUTED
	// Added by Mason Yu. Open RIP Port for Incoming Packets.
	if (mib_get_s( MIB_RIP_ENABLE, (void *)&vChar, sizeof(vChar)) != 0)
	{
		if (1 == vChar)
		{
			// iptables -A inacc -i ! br0 -p udp --dport 520 -j ACCEPT
			va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_INACC, "!", "-i", LANIF_ALL, "-p", "udp", (char *)FW_DPORT, "520", "-j", (char *)FW_ACCEPT);
		}
	}
#endif

	// iptables -A INPUT -j inacc
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_INACC);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_INACC);
#endif
#endif // of REMOTE_ACCESS_CTL
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if (mib_get_s(MIB_PPTP_ENABLE, (void *)&vInt, sizeof(vInt)) != 0)
	{
		if (1 == vInt)
		{
			//iptables -I INPUT -p gre(47) -j ACCEPT
			va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "gre", "-j", (char *)FW_ACCEPT);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 6, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "gre", "-j", (char *)FW_ACCEPT);
#endif
		}
	}
#endif

#ifdef CONFIG_SUPPORT_CAPTIVE_PORTAL
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_CAPTIVEPORTAL_FILTER);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, "INPUT", "-j", (char *)FW_CAPTIVEPORTAL_FILTER);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_CAPTIVEPORTAL_FILTER);
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, "INPUT", "-j", (char *)FW_CAPTIVEPORTAL_FILTER);
#endif
	set_CaptivePortal_Firewall();
#endif

#ifdef CONFIG_CU
	//iptables -A INPUT -i br0 -p udp --dport 547 -j REJECT --reject-with icmp-port-unreachable
	va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char *)FW_INPUT, "-i", LANIF_ALL, "-p", "udp", (char *)FW_DPORT, "547", "-j", "REJECT", "--reject-with", "icmp-port-unreachable");
#endif

	// Add chain for dhcp Port base filter
	// iptables -N dhcp_port_filter
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_DHCP_PORT_FILTER);
	set_dhcp_port_base_filter(1);
	// iptables -A INPUT -j dhcp_port_filter
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_DHCP_PORT_FILTER);

#if 0  /* 20231013, OLT PING issue */
	if (wan_mode == DEO_NAT_MODE)
	{
		/* " iptables -D INPUT -i nas0_0 -m state --state NEW -j DROP" rule */
		va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, (char *)FW_INPUT, (char *)ARG_I, "nas0_0", "-m", "state", "--state", "NEW", "-j", (char *)FW_DROP);
		va_cmd(IPTABLES, 11, 1, (char *)FW_INSERT, (char *)FW_INPUT, "6", (char *)ARG_I, "nas0_0", "-m", "state", "--state", "NEW", "-j", (char *)FW_DROP);
	}
#endif

#ifdef REMOTE_ACCESS_CTL
	// Added by Mason Yu for accept packet with ip(127.0.0.1)
	// Magician: Modify this rule to allow to ping self. Merge 192.168.1.1-to-192.168.1.1 accepted rule and to-127.0.0.1 accpeted rule.
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_INPUT, "-i", "lo", "-j", (char *)FW_ACCEPT);
#endif

	// Single PC mode
	if (mib_get_s(MIB_SPC_ENABLE, (void *)value, sizeof(value)) != 0)
	{
		if (value[0]) // Single PC mode enabled
		{
			spc_enable = 1;
			mib_get_s(MIB_SPC_IPTYPE, (void *)value, sizeof(value));
			if (value[0] == 0) // single private IP
			{
				spc_ip = 0;
				mib_get_s(MIB_ADSL_LAN_IP, (void *)value, sizeof(value));

				// IP pool start address
				// Kaohj
				#ifndef DHCPS_POOL_COMPLETE_IP
				mib_get_s(MIB_ADSL_LAN_CLIENT_START, (void *)&value[3], sizeof(value[3]));
				strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
				#else
				mib_get_s(MIB_DHCP_POOL_START, (void *)value, sizeof(value));
				strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
				#endif
				ipaddr[15] = '\0';
				// iptables -A FORWARD -i $LANIF -s ! ipaddr -j DROP
				va_cmd(IPTABLES, 9, 1, (char *)FW_ADD,
					(char *)FW_FORWARD, (char *)ARG_I,
					(char *)LANIF_ALL, "!", "-s", ipaddr,
					"-j", (char *)FW_DROP);
			}
			else // single IP passthrough
			{
				spc_ip = 1;
			}
		}
		else
			spc_enable = 0;
	}

#ifdef IP_PASSTHROUGH
	// check vc
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;

		if (spc_enable && (spc_ip == 1)) // single IP passthrough (public IP)
		{
			if ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_RT1483 &&
				ippt_itf == Entry.ifIndex)
			{	// ippt WAN interface (1483-r)
				if ((DHCP_T)Entry.ipDhcp == DHCP_DISABLED)
				{
					strncpy(ipaddr, inet_ntoa(*((struct in_addr *)Entry.ipAddr)), 16);
					ipaddr[15] = '\0';
					// iptables -A FORWARD -i $LANIF -s ! ipaddr -j DROP
					va_cmd(IPTABLES, 9, 1, (char *)FW_ADD,
						(char *)FW_FORWARD, (char *)ARG_I,
						(char *)LANIF_ALL, "!", "-s", ipaddr,
						"-j", (char *)FW_DROP);
				}
			}
		}
	}
#endif

	setup_dscp_remark_rule_normal();

#ifdef CONFIG_YUEME
	//for framework and game plugin feature
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)FRAMEWORK_ADDR_MAP);
	va_cmd(IPTABLES, 6, 1, "-t", "nat", "-A", "POSTROUTING", "-j", (char *)FRAMEWORK_ADDR_MAP);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)FRAMEWORK_ADDR_MAP);
	//framework rule: iptables -t nat -A framework_addr_map -s 10.0.3.0/24 ! -d 10.0.3.0/24 -j MASQUERADE
	va_cmd(IPTABLES, 11, 1, "-t", "nat", FW_ADD, FRAMEWORK_ADDR_MAP, "-s", "10.0.3.0/24", "!", "-d", "10.0.3.0/24", "-j", "MASQUERADE");
	//Because game plugin will name interface to be l2tp_xxx(l2tp_1) or pptp_xxx
	va_cmd(IPTABLES, 8, 1, "-t", "nat", FW_ADD, FRAMEWORK_ADDR_MAP, "-o", "l2tp+", "-j", "MASQUERADE");
	va_cmd(IPTABLES, 8, 1, "-t", "nat", FW_ADD, FRAMEWORK_ADDR_MAP, "-o", "pptp+", "-j", "MASQUERADE");
#endif

	//for napt feature
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)NAT_ADDRESS_MAP_TBL);
	va_cmd(IPTABLES, 6, 1, "-t", "nat", "-A", "POSTROUTING", "-j", (char *)NAT_ADDRESS_MAP_TBL);
#if defined(CONFIG_IPV6)
	va_cmd(IP6TABLES, 4, 1, "-t", "nat", "-N", (char *)NAT_ADDRESS_MAP_TBL);
	va_cmd(IP6TABLES, 6, 1, "-t", "nat", "-A", "POSTROUTING", "-j", (char *)NAT_ADDRESS_MAP_TBL);
#endif

#ifdef CONFIG_USER_RTK_VOIP
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", VOIP_REMARKING_CHAIN);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-p", "udp", "-j", VOIP_REMARKING_CHAIN);
#if defined(CONFIG_IPV6)
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", VOIP_REMARKING_CHAIN);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-p", "udp", "-j", VOIP_REMARKING_CHAIN);
#endif
#endif

#ifdef YUEME_4_0_SPEC
#ifdef SUPPORT_APPFILTER
	apply_appfilter(((!isBoot)?2:isBoot));
#endif
#endif

#if defined(URL_BLOCKING_SUPPORT)||defined(URL_ALLOWING_SUPPORT)
#if defined(CONFIG_E8B) && !defined(SUPPORT_URL_FILTER)
	unsigned char urlfilter_mode = 0;
	mib_get_s(MIB_URLFILTER_MODE, (void *)&urlfilter_mode, sizeof(urlfilter_mode));
	if (urlfilter_mode == 1) /* White List */
		urlEnable = 2;
	else if (urlfilter_mode == 2) /* Black List */
		urlEnable = 1;
#else
	mib_get_s(MIB_URL_CAPABILITY, (void *)&urlEnable, sizeof(urlEnable));
#endif

	va_cmd(IPTABLES, 2, 1, "-N", WEBURL_CHAIN);
	va_cmd(IPTABLES, 16, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-i", LANIF_ALL, "-p", "tcp", "--tcp-flags", "SYN,FIN,RST", "NONE", "-m", "length", "!", "--length", "0:84", "-j", WEBURL_CHAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", WEBURL_CHAIN);
	va_cmd(IP6TABLES, 16, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-i", LANIF_ALL, "-p", "tcp", "--tcp-flags", "SYN,FIN,RST", "NONE", "-m", "length", "!", "--length", "0:104", "-j", WEBURL_CHAIN);
#endif
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", WEBURLPRE_CHAIN);
	va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "-i", LANIF_ALL, "-p", "tcp", "-j", WEBURLPRE_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", WEBURLPOST_CHAIN);
	va_cmd(IPTABLES, 11, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "!", "-o", LANIF_ALL, "-p", "tcp", "-j", WEBURLPOST_CHAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", WEBURLPRE_CHAIN);
	va_cmd(IP6TABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "-i", LANIF_ALL, "-p", "tcp", "-j", WEBURLPRE_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", WEBURLPOST_CHAIN);
	va_cmd(IP6TABLES, 11, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "!", "-o", LANIF_ALL, "-p", "tcp", "-j", WEBURLPOST_CHAIN);
#endif

#ifdef SUPPORT_URL_FILTER
	set_url_filter();
#else
	filter_set_url(urlEnable);
#endif
#endif

#ifdef _PRMT_X_CMCC_SECURITY_
	va_cmd(IPTABLES, 2, 1, "-N", PARENTALCTRL_CHAIN);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", PARENTALCTRL_CHAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", PARENTALCTRL_CHAIN);
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", PARENTALCTRL_CHAIN);
#endif

	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", PARENTALCTRLPRE_CHAIN);
	va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "-i", LANIF_ALL, "-p", "tcp", "-j", PARENTALCTRLPRE_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", PARENTALCTRLPOST_CHAIN);
	va_cmd(IPTABLES, 11, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "!", "-o", LANIF_ALL, "-p", "tcp", "-j", PARENTALCTRLPOST_CHAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", PARENTALCTRLPRE_CHAIN);
	va_cmd(IP6TABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "-i", LANIF_ALL, "-p", "tcp", "-j", PARENTALCTRLPRE_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", PARENTALCTRLPOST_CHAIN);
	va_cmd(IP6TABLES, 11, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "!", "-o", LANIF_ALL, "-p", "tcp", "-j", PARENTALCTRLPOST_CHAIN);
#endif

	unsigned char rule_reset = 1;
	mib_set(MIB_RS_PARENTALCTRL_RULE_RESET, (void *)&rule_reset);
#endif

#ifdef LAYER7_FILTER_SUPPORT
	//App Filter
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_APPFILTER);
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_APP_P2PFILTER);
	va_cmd(IPTABLES, 4, 1, ARG_T, "mangle", "-N", (char *)FW_APPFILTER);
	setupAppFilter();
	// iptables -A FORWARD -j appfilter
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_APPFILTER);
	// iptables -t mangle -A POSTROUTING -j appfilter
	va_cmd(IPTABLES, 6, 1, ARG_T, "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", (char *)FW_APPFILTER);
#endif

#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
	// iptables -t nat -N WebRedirect_Wizard
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", "WebRedirect_Wizard");
	// iptables -t nat -A PREROUTING -j WebRedirect_Wizard
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", "WebRedirect_Wizard");

	system("/bin/echo 1 > /tmp/WebRedirectChain");
#endif

#ifdef PORT_FORWARD_GENERAL
	// port forwarding
	// Add New chain on filter and nat
	// iptables -N portfw
	va_cmd(IPTABLES, 2, 1, "-N", (char *)PORT_FW);
	// iptables -A FORWARD -j portfw
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)PORT_FW);
	// iptables -t nat -N portfw
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)PORT_FW);
	// iptables -t nat -A PREROUTING -j portfw
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)PORT_FW);

#ifdef NAT_LOOPBACK
	// iptables -t nat -N portfwPreNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)PORT_FW_PRE_NAT_LB);
	// iptables -t nat -A PREROUTING -j portfwPreNatLB
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)PORT_FW_PRE_NAT_LB);
	// iptables -t nat -N portfwPostNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)PORT_FW_POST_NAT_LB);
	// iptables -t nat -A POSTROUTING -j portfwPostNatLB
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", (char *)PORT_FW_POST_NAT_LB);
#endif
	setupPortFW();
#endif

#ifdef USE_DEONET /* sghong, 20231025 */
	if (wan_mode == DEO_NAT_MODE)
		deo_httpdRedirect();
#endif

//#if defined(CONFIG_USER_BOA_PRO_PASSTHROUGH) && defined(CONFIG_RTK_DEV_AP)
#ifdef CONFIG_USER_BOA_PRO_PASSTHROUGH
   // iptables -N IPSEC_PASSTHROUGH_FORWARD
   va_cmd(IPTABLES, 2, 1, "-N", (char *)IPSEC_PASSTHROUGH_FORWARD);
   // iptables -N L2TP_PASSTHROUGH_FORWARD
   va_cmd(IPTABLES, 2, 1, "-N", (char *)L2TP_PASSTHROUGH_FORWARD);
   // iptables -N PPTP_PASSTHROUGH_FORWARD
   va_cmd(IPTABLES, 2, 1, "-N", (char *)PPTP_PASSTHROUGH_FORWARD);

   // iptables -A FORWARD -j IPSEC_PASSTHROUGH_FORWARD
   va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)IPSEC_PASSTHROUGH_FORWARD);
   // iptables -A FORWARD -j L2TP__PASSTHROUGH_FORWARD
   va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)L2TP_PASSTHROUGH_FORWARD);
   // iptables -A FORWARD -j PPTP_PASSTHROUGH_FORWARD
   va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)PPTP_PASSTHROUGH_FORWARD);

   Setup_VpnPassThrough();
#endif


#ifdef PORT_FORWARD_ADVANCE
	setupPFWAdvance();
#endif

#ifdef NATIP_FORWARDING
	fw_setupIPForwarding();
#endif

#if defined(PORT_TRIGGERING_STATIC) || defined(PORT_TRIGGERING_DYNAMIC)
	// Add New chain on filter and nat
	// iptables -N portTrigger
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IPTABLES_PORTTRIGGER);
	// iptables -A FORWARD -j portTrigger
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)IPTABLES_PORTTRIGGER);
	// iptables -t nat -N portTrigger
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)IPTABLES_PORTTRIGGER);
	// iptables -t nat -A PREROUTING -j portTrigger
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)IPTABLES_PORTTRIGGER);
	setupPortTriggering();
#endif
//add virtual_server filter before ipfilter.
#ifdef VIRTUAL_SERVER_SUPPORT
#ifdef CONFIG_E8B

	const char IPTABLE_VTLSUR[]="virtual_server";

	//execute virtual server FORWARD rules
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IPTABLE_VTLSUR);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)IPTABLE_VTLSUR);

	//execute virtual server NAT prerouting rules
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)IPTABLE_VTLSUR);
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)IPTABLE_VTLSUR);

	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", (char *)IPTABLE_VTLSUR);
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)IPTABLE_VTLSUR);

#ifdef NAT_LOOPBACK
	// iptables -t nat -N virserverPreNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)VIR_SER_PRE_NAT_LB);
	// iptables -t nat -A PREROUTING -j virserverPreNatLB
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)VIR_SER_PRE_NAT_LB);
	// iptables -t nat -N virserverPostNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)VIR_SER_POST_NAT_LB);
	// iptables -t nat -A POSTROUTING -j virserverPostNatLB
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", (char *)VIR_SER_POST_NAT_LB);
#endif

	setupVtlsvr(VIRTUAL_SERVER_ADD);
#else
	//execute virtual server rules
	setupVtlsvr();
#endif
#endif

#ifdef IP_PORT_FILTER
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_IPFILTER_IN);
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_IPFILTER_OUT);
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_FORWARD, ARG_I, LANIF_ALL, "-j", (char *)FW_IPFILTER_OUT);
	va_cmd(IPTABLES, 7, 1, (char *)FW_ADD, (char *)FW_FORWARD, ARG_NO, ARG_I, LANIF_ALL, "-j", (char *)FW_IPFILTER_IN);
#else
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_IPFILTER);
	// iptables -A FORWARD -j ipfilter
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_IPFILTER);
#endif
	setupIPFilter();
#endif

#if (defined(IP_PORT_FILTER) || defined(DMZ)) && defined(CONFIG_TELMEX_DEV)
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_DMZFILTER);
	filter_patch_for_dmz();
	//iptables -A FORWARD -j dmz_patch
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_DMZFILTER);
#endif

#ifdef PARENTAL_CTRL
	// parental_ctrl
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_PARENTAL_CTRL);
	// iptables -A FORWARD -j parental_ctrl
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_PARENTAL_CTRL);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_PARENTAL_CTRL);
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_PARENTAL_CTRL);
#endif
#endif

#ifdef DMZ
#ifdef NAT_LOOPBACK
	// iptables -t nat -N dmzPreNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)IPTABLE_DMZ_PRE_NAT_LB);
	// iptables -t nat -A PREROUTING -j dmzPreNatLB
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)IPTABLE_DMZ_PRE_NAT_LB);
	// iptables -t nat -N dmzPostNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)IPTABLE_DMZ_POST_NAT_LB);
	// iptables -t nat -A POSTROUTING -j dmzPostNatLB
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", (char *)IPTABLE_DMZ_POST_NAT_LB);
#endif

	// DMZ
	// Add New chain on filter and nat
	// iptables -N dmz
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IPTABLE_DMZ);
	// iptables -t nat -N dmz
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)IPTABLE_DMZ);
	setupDMZ();
#ifdef IP_PORT_FILTER
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	// iptables -A filter_in -j dmz
	va_cmd(IPTABLES, 4, 1, "-A", (char *)FW_IPFILTER_IN, "-j", (char *)IPTABLE_DMZ);
#else
	// iptables -A filter -j dmz
	va_cmd(IPTABLES, 4, 1, "-A", (char *)FW_IPFILTER, "-j", (char *)IPTABLE_DMZ);
#endif
#endif
	// iptables -t nat -A PREROUTING -j dmz
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)IPTABLE_DMZ);
#endif

#ifdef IP_PORT_FILTER
	// Set IP filter default action
	setup_default_IPFilter();
#endif

#if defined(CONFIG_CU_BASEON_YUEME) && defined(COM_CUC_IGD1_ParentalControlConfig)
	va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_PARENTAL_MAC_CTRL_LOCALIN);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)FW_PARENTAL_MAC_CTRL_LOCALIN, (char *)FW_RETURN);
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-j", (char *)FW_PARENTAL_MAC_CTRL_LOCALIN);
	va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_PARENTAL_MAC_CTRL_FWD);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)FW_PARENTAL_MAC_CTRL_FWD, (char *)FW_RETURN);
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", (char *)FW_PARENTAL_MAC_CTRL_FWD);
#endif


#ifdef MAC_FILTER
	// Mac Filter
	va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_MACFILTER_FWD);
	va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_MACFILTER_LI);
	va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_MACFILTER_LO);
	va_cmd(EBTABLES, 4, 1, "-t", "nat", (char *)"-N", (char *)FW_MACFILTER_MC);
	va_cmd(EBTABLES, 5, 1, "-t", "nat", (char *)"-P", (char *)FW_MACFILTER_MC, (char *)FW_RETURN);
#ifdef CONFIG_E8B
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_MACFILTER_FWD_L3);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_MACFILTER_FWD_L3);
	setupMacFilterTables();
#else
	setupMacFilter();
#endif
#ifdef	IGMPHOOK_MAC_FILTER
	reset_IgmpHook_MacFilter();
#endif

	// ebtables -A FORWARD -j macfilter
	#ifdef CONFIG_RTK_DEV_AP
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_MACFILTER_FWD);
	#if defined(CONFIG_USER_RTK_BRIDGE_MACFILTER_SUPPORT)
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_MACFILTER_FWD);
	va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", FW_MACFILTER_MC);
	#endif
	#else
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-j", (char *)FW_MACFILTER_LI);
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-j", (char *)FW_MACFILTER_LO);
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", (char *)FW_MACFILTER_FWD);
	#endif
#endif

#ifdef SUPPORT_ACCESS_RIGHT
	apply_accessRight(1);
#endif

#ifdef SUPPORT_FON_GRE
	// iptables -N dhcp_queue
	va_cmd(IPTABLES, 2, 1, "-N", "dhcp_queue");
	// iptables -A FORWARD -j dhcp_queue
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", "dhcp_queue");
	// iptables -N FON_GRE_FORWARD
	va_cmd(IPTABLES, 2, 1, "-N", "FON_GRE_FORWARD");
	// iptables -I FORWARD -j FON_GRE_FORWARD
	va_cmd(IPTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", "FON_GRE_FORWARD");
	// iptables -N FON_GRE_INPUT
	va_cmd(IPTABLES, 2, 1, "-N", "FON_GRE_INPUT");
	// iptables -I INPUT -j FON_GRE_INPUT
	va_cmd(IPTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-j", "FON_GRE_INPUT");
	// iptables -t mangle -N FON_GRE
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", "FON_GRE");
	// iptables -t mangle -A FORWARD -j FON_GRE
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-j", "FON_GRE");

	va_cmd(EBTABLES, 2, 1, "-N", "FON_GRE");
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", "FON_GRE");
	va_cmd(EBTABLES, 3, 1, "-P", "FON_GRE", (char *)FW_RETURN);
#endif

#ifdef CONFIG_XFRM
	// iptables -t mangle -N XFRM
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", "XFRM");
	// iptables -t mangle -A POSTROUTING -j XFRM
	va_cmd_no_error(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_POSTROUTING, "-j", "XFRM");
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", "XFRM");
#ifdef CONFIG_IPV6_VPN
	// iptables -t mangle -N XFRM
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", "XFRM");
	// iptables -t mangle -A POSTROUTING -j XFRM
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", "XFRM");
#endif
#endif

#ifdef LAYER7_FILTER_SUPPORT
	// iptables -A FORWARD -j appp2pfilter
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_APP_P2PFILTER);
#endif



 	// for multicast
#if 0 // move ahead to ipfilter chain
	// iptables -A FORWARD -d 224.0.0.0/4 -j ACCEPT
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-d",
		"224.0.0.0/4", "-j", (char *)FW_ACCEPT);
#endif

	// iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
	// andrew. moved to the 1st rule, or returning PING/DNS relay will be blocked.
	//va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-m", "state",
	//	"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);

#ifdef REMOTE_ACCESS_CTL
	// iptables -A INPUT -i $LAN_INTERFACE -j ACCEPT
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_INPUT, (char *)ARG_I, (char *)LANIF_ALL,
		"-j", (char *)FW_ACCEPT);
#endif

#if 0
	/*--------------------------------------------------------------------
	 * The following are the default action and should be final rules
	 -------------------------------------------------------------------*/
	// iptables -N block
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_BLOCK);

	// default action
	if (mib_get_s(MIB_IPF_OUT_ACTION, (void *)value, sizeof(value)) != 0)
	{
		vInt = (int)(*(unsigned char *)value);
		if (vInt == 1)	// ACCEPT
		{
			// iptables -A block -i $LAN_IF -j ACCEPT
			va_cmd(IPTABLES, 6, 1, (char *)FW_ADD,
				(char *)FW_BLOCK, (char *)ARG_I,
				(char *)LANIF, "-j", (char *)FW_ACCEPT);
		}
	}

	if (mib_get_s(MIB_IPF_IN_ACTION, (void *)value, sizeof(value)) != 0)
	{
		vInt = (int)(*(unsigned char *)value);
		if (vInt == 1)	// ACCEPT
		{
			// iptables -A block -i ! $LAN_IF -j ACCEPT
			va_cmd(IPTABLES, 7, 1, (char *)FW_ADD,
				(char *)FW_BLOCK, "!", (char *)ARG_I,
				(char *)LANIF, "-j", (char *)FW_ACCEPT);
		}
	}

	// iptables -A block -m state --state ESTABLISHED,RELATED -j ACCEPT
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_BLOCK, "-m", "state",
		"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);

	/*
	// iptables -A block -m state --state NEW -i $LAN_INTERFACE -j ACCEPT
	va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_BLOCK, "-m", "state",
		"--state", "NEW", (char *)ARG_I, (char *)LANIF,
		"-j", (char *)FW_ACCEPT);
	*/

	// iptables -A block -j DROP
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, "block", "-j", (char *)FW_DROP);

	/*
	// iptables -A INPUT -j block
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", "block");
	*/

	// iptables -A FORWARD -j block
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", "block");
#endif

#ifdef CONFIG_IPV6
	// INPUT default DROP , ip6table -P INPUT  DROP
	if (aclEnable == 0)
		va_cmd(IP6TABLES, 3, 1, "-P", (char *)FW_INPUT, (char *)FW_DROP);
	else
		va_cmd(IP6TABLES, 3, 1, "-P", (char *)FW_INPUT, FW_ACCEPT);

	// ip6tables -A INPUT -p icmpv6 --icmpv6-type router-solicitation -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "icmpv6",
		"--icmpv6-type","router-solicitation", "-j", (char *)FW_ACCEPT);

	// ip6tables -A INPUT -p icmpv6 --icmpv6-type router-advertisement -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "icmpv6",
		"--icmpv6-type","router-advertisement","-j", (char *)FW_ACCEPT);

	// ip6tables -I INPUT  -p icmpv6 --icmpv6-type neighbour-solicitation -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "icmpv6",
		"--icmpv6-type","neighbour-solicitation","-j", (char *)FW_ACCEPT);

	// ip6tables -I INPUT  -p icmpv6 --icmpv6-type neighbour-advertisement -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "icmpv6",
		"--icmpv6-type","neighbour-advertisement","-j", (char *)FW_ACCEPT);

	// ip6tables -I INPUT  -p icmpv6 --icmpv6-type echo-request -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "icmpv6",
		"--icmpv6-type","echo-request","-j", (char *)FW_ACCEPT);

	// Need accept MLD query, because need let kernel reply the MLD member report to the WAN
	// ip6tables -I INPUT  -p icmpv6 --icmpv6-type 130 -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "icmpv6",
		"--icmpv6-type","130","-j", (char *)FW_ACCEPT);

	// ip6tables -A INPUT -p icmpv6 -i $LAN_INTERFACE -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "icmpv6",
	(char *)ARG_I, (char *)LANIF_ALL, "-j", (char *)FW_ACCEPT);

	// ip6tables -A INPUT -i $LAN_INTERFACE -j ACCEPT
	va_cmd(IP6TABLES, 6, 1, (char *)FW_ADD, (char *)FW_INPUT, (char *)ARG_I, (char *)LANIF_ALL,
		"-j", (char *)FW_ACCEPT);

	// ip6tables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
#if defined(CONFIG_00R0)
	char ipfilter_spi_state=1;
	mib_get_s(MIB_IPFILTER_SPI_ENABLE, (void*)&ipfilter_spi_state, sizeof(ipfilter_spi_state));
	if(ipfilter_spi_state != 0)
#endif
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-m", "state",
			"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);

	// ip6tables  -A INPUT -p udp --dport  546 -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "udp",
			"--dport", "546", "-j", (char *)FW_ACCEPT);

	//  Added by Mason Yu for dhcpv6 Relay. Open DHCPv6 Relay Port for Incoming Packets.
	// ip6tables  -A INPUT ! -i $LAN_INTERFACE -p udp --dport  547 -j ACCEPT
	va_cmd(IP6TABLES, 11, 1, (char *)FW_ADD, (char *)FW_INPUT, "!", (char *)ARG_I, (char *)LANIF,
			"-p", "udp", "--dport", "547", "-j", (char *)FW_ACCEPT);

	// IPv6 Filter
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_IPV6FILTER);
	// ip6tables -A FORWARD -j ipv6filter
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_IPV6FILTER);
	restart_IPV6Filter();

#if defined(CONFIG_USER_RTK_MULTICAST_VID) && defined(CONFIG_RTK_SKB_MARK2)
	rtk_block_multicast_ra();
#endif
#endif
#if defined (CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_L2TPD_LNS)
	set_LAN_VPN_accept((char *)FW_INPUT);
#endif

#if defined(CONFIG_USER_RTK_VOIP)
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		voip_set_fc_1p_tag();
#endif
#endif

#if defined(CONFIG_USER_RTK_NAT_ALG_PASS_THROUGH) || (defined(CONFIG_TRUE) && defined(CONFIG_IP_NF_ALG_ONOFF))
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_VPNPASSTHRU);
	// iptables -A FORWARD -j vpnPassThrough
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_VPNPASSTHRU);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_VPNPASSTHRU);
	// ip6tables -A FORWARD -j vpnPassThrough
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_VPNPASSTHRU);
#endif
#endif

#ifdef CONFIG_USER_IGMPTR247
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_TR247_MULTICAST);
	va_cmd(IPTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_OUTPUT, "-j", (char *)FW_TR247_MULTICAST);
#endif
#if defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_)
	va_cmd(EBTABLES, 2, 1, "-N", FW_ISOLATION_MAP);
	va_cmd(EBTABLES, 3, 1, "-P", FW_ISOLATION_MAP, (char *)FW_RETURN);
	va_cmd(EBTABLES, 4, 1, "-I", FW_FORWARD, "-j", FW_ISOLATION_MAP);
	va_cmd(EBTABLES, 2, 1, "-N", FW_ISOLATION_BRPORT);
	va_cmd(EBTABLES, 3, 1, "-P", FW_ISOLATION_BRPORT, (char *)FW_DROP);
	va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_ISOLATION_BRPORT, "-i", "nas0+", "-j", (char *)FW_RETURN);
	va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_ISOLATION_BRPORT, "-o", "nas0+", "-j", (char *)FW_RETURN);
	va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_ISOLATION_BRPORT, "-d", "1:80:c2:0:0:0/ff:ff:ff:0:0:0", "-j", (char *)FW_RETURN);

	rtk_layer2bridging_set_group_isolation_firewall_rule(-1, 1);
#endif
#if (defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	unsigned int pon_mode = 0;
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
	if(pon_mode==GPON_MODE || pon_mode==EPON_MODE)
	{
		va_cmd(EBTABLES, 2,  1, "-N", L2_PPTP_PORT_SERVICE);
		va_cmd(EBTABLES, 3,  1, "-P", L2_PPTP_PORT_SERVICE, "RETURN");
		va_cmd(EBTABLES, 10, 1, "-A", L2_PPTP_PORT_SERVICE, "-p", "ipv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "DROP");
		va_cmd(EBTABLES, 8,  1, "-A", L2_PPTP_PORT_SERVICE, "-p", "ipv4", "--ip-proto", "2", "-j", "DROP");
		va_cmd(EBTABLES, 10, 1, "-A", L2_PPTP_PORT_SERVICE, "-p", "ipv6", "--ip6-proto", "udp", "--ip6-dport", "546:547", "-j", "DROP");
		va_cmd(EBTABLES, 10, 1, "-A", L2_PPTP_PORT_SERVICE, "-p", "ipv6", "--ip6-proto", "ipv6-icmp", "--ip6-icmp-type", "130:134", "-j", "DROP");
		va_cmd(EBTABLES, 2,  1, "-N", L2_PPTP_PORT_FORWARD);
		va_cmd(EBTABLES, 3,  1, "-P", L2_PPTP_PORT_FORWARD, "DROP");
		va_cmd(EBTABLES, 6,  1, "-A", L2_PPTP_PORT_FORWARD, "-d", "01:80:C2:00:00:00/FF:FF:FF:00:00:00", "-j", "RETURN");
		va_cmd(EBTABLES, 6,  1, "-A", L2_PPTP_PORT_FORWARD, "-i", "nas+", "-j", "RETURN");
		va_cmd(EBTABLES, 2,  1, "-N", L2_PPTP_PORT_INPUT_TBL);
		va_cmd(EBTABLES, 3,  1, "-P", L2_PPTP_PORT_INPUT_TBL, "RETURN");
		va_cmd(EBTABLES, 4,  1, "-I", FW_INPUT, "-j", L2_PPTP_PORT_INPUT_TBL);
		va_cmd(EBTABLES, 2,  1, "-N", L2_PPTP_PORT_OUTPUT_TBL);
		va_cmd(EBTABLES, 3,  1, "-P", L2_PPTP_PORT_OUTPUT_TBL, "RETURN");
		va_cmd(EBTABLES, 4,  1, "-I", FW_OUTPUT, "-j", L2_PPTP_PORT_OUTPUT_TBL);
		va_cmd(EBTABLES, 2,  1, "-N", L2_PPTP_PORT_FORWARD_TBL);
		va_cmd(EBTABLES, 3,  1, "-P", L2_PPTP_PORT_FORWARD_TBL, "RETURN");
		va_cmd(EBTABLES, 4,  1, "-I", FW_FORWARD, "-j", L2_PPTP_PORT_FORWARD_TBL);
		reconfig_port_pptp_def_rule();
	}
#endif

#ifdef CONFIG_USER_WIRED_8021X
	rtk_wired_8021x_setup();
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#ifdef WLAN_SUPPORT
	setup_wlan_accessRule_netfilter_init();
#endif
#endif

#if defined(NESSUS_FILTER)
	setup_filter_nessus();
#endif

#ifdef CONFIG_CMCC_FORWARD_RULE_SUPPORT
	rtk_cmcc_init_cmcc_forward_rule();
	setupCmccForwardRule(CMCC_FORWARDRULE_ADD);
#endif

#if defined(CONFIG_RTK_CTCAPD_FILTER_SUPPORT)
	rtk_restore_ctcapd_filter_state();
	rtk_set_dns_filter_rules_for_ubus();
#endif
#ifdef CONFIG_USER_CTCAPD
	char cmdBuf[64]={0};
	snprintf(cmdBuf, sizeof(cmdBuf), "echo 1 > %s", CTCAPD_FW_FLAG_FILE);
	system(cmdBuf);
#ifdef WLAN_SUPPORT
	rtk_set_wifi_permit_rule(); //sarah: init for wifi permit rule
#endif
#endif


#ifdef CONFIG_RTK_DNS_TRAP

	char skbmark_buf[64]={0};

#ifdef CONFIG_RTK_SKB_MARK2

	snprintf(skbmark_buf, sizeof(skbmark_buf), "0x%x", (1<<SOCK_MARK2_FORWARD_BY_PS_START));

	//iptables -t mangle -D PREROUTING -p udp --dport 53 -j MARK2 --or-mark 0x1
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);
	//iptables -t mangle -D PREROUTING -p udp --sport 53 -j MARK2 --or-mark 0x1
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -D PREROUTING -p udp --dport 53 -j MARK2 --or-mark 0x1
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -D PREROUTING -p udp --sport 53 -j MARK2 --or-mark 0x1
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);

	//iptables -t mangle -A PREROUTING -p udp --dport 53 -j MARK2 --or-mark 0x1
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);
	//iptables -t mangle -A PREROUTING -p udp --sport 53 -j MARK2 --or-mark 0x1
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -A PREROUTING -p udp --dport 53 -j MARK2 --or-mark 0x1
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -A PREROUTING -p udp --sport 53 -j MARK2 --or-mark 0x1
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK2", "--or-mark", skbmark_buf);


#else

	snprintf(skbmark_buf, sizeof(skbmark_buf), "0x%x", (1<<SOCK_MARK_METER_INDEX_END));

	//iptables -t mangle -D PREROUTING -p udp --dport 53 -j MARK --or-mark 0x80000000
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);
	//iptables -t mangle -D PREROUTING -p udp --sport 53 -j MARK --or-mark 0x80000000
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -D PREROUTING -p udp --dport 53 -j MARK --or-mark 0x80000000
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -D PREROUTING -p udp --sport 53 -j MARK --or-mark 0x80000000
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);

	//iptables -t mangle -A PREROUTING -p udp --dport 53 -j MARK --or-mark 0x80000000
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);
	//iptables -t mangle -A PREROUTING -p udp --sport 53 -j MARK --or-mark 0x80000000
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -A PREROUTING -p udp --dport 53 -j MARK --or-mark 0x80000000
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_DPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);
	//ip6tables -t mangle -A PREROUTING -p udp --sport 53 -j MARK --or-mark 0x80000000
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, FW_PREROUTING, "-p", "udp", FW_SPORT, "53", "-j", "MARK", "--or-mark", skbmark_buf);

#endif
#endif

#ifdef CONFIG_VIR_BOA_JEMBENCHTEST
	rtk_cmcc_do_jspeedup_ex();
#endif

#if defined(CONFIG_USER_DOS_ICMP_REDIRECTION)
	rtk_set_dos_icmpRedirect_rule();
#endif

#if defined(CONFIG_CTC_SDN)
	if(check_ovs_enable() && check_ovs_running() == 0)
	{
		setup_controller_route_rule(1);
	}
#endif

#ifdef CONFIG_CU
	block_wanout_tmp_clear();
#endif

#if defined(CONFIG_SECONDARY_IP)
	rtk_fw_set_lan_ip2_rule();
#endif
#if defined (CONFIG_IPV6) && defined(CONFIG_SECONDARY_IPV6)
	rtk_fw_set_lan_ipv6_2_rule();
#endif
#ifdef CONFIG_USER_VXLAN
	setupVxLAN_FW_RULES();
#endif

#ifdef CONFIG_USER_LOCK_NET_FEATURE_SUPPORT
	va_cmd(IPTABLES, 2, 1, "-N", (char *)NETLOCK_INPUT);
	// iptables -I INPUT ! -i br+ -j NetBlock_Input
	va_cmd(IPTABLES, 7, 1, (char *)FW_INSERT, (char *)FW_INPUT, "!", "-i", LANIF_ALL, "-j", (char *)NETLOCK_INPUT);
	va_cmd(IPTABLES, 2, 1, "-N", (char *)NETLOCK_FWD);
	// iptables -I FORWARD -j NetBlock_Fwd
	va_cmd(IPTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", (char *)NETLOCK_FWD);
	va_cmd(IPTABLES, 2, 1, "-N", (char *)NETLOCK_OUTPUT);
	// iptables -I OUTPUT -j NetBlock_Output
	va_cmd(IPTABLES, 7, 1, (char *)FW_INSERT, (char *)FW_OUTPUT, "!", "-o", LANIF_ALL, "-j", (char *)NETLOCK_OUTPUT);

	va_cmd(IP6TABLES, 2, 1, "-N", (char *)NETLOCK_INPUT);
	// ip6tables -I INPUT -j NetBlock_Input
	va_cmd(IP6TABLES, 7, 1, (char *)FW_INSERT, (char *)FW_INPUT, "!", "-i", LANIF_ALL, "-j", (char *)NETLOCK_INPUT);
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)NETLOCK_FWD);
	// ip6tables -I FORWARD -j NetBlock_Fwd
	va_cmd(IP6TABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", (char *)NETLOCK_FWD);
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)NETLOCK_OUTPUT);
	// ip6tables -I OUTPUT -j NetBlock_Output
	va_cmd(IP6TABLES, 7, 1, (char *)FW_INSERT, (char *)FW_OUTPUT, "!", "-o", LANIF_ALL, "-j", (char *)NETLOCK_OUTPUT);

	//flush net lock rules when switch opmode
	rtk_set_net_locked_rules(0);
#endif

#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	rtk_init_dynamic_firewall();
#endif
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ280) && defined(CONFIG_RTL_MULTI_ETH_WAN)
	rtk_fw_set_skb_mark_for_dns_pkt();
#endif

#ifdef CONFIG_NETFILTER_XT_MATCH_PSD
	setup_psd();
#endif
#ifdef CONFIG_PPPOE_MONITOR
	va_cmd(EBTABLES, 8, 1, "-A", "FORWARD", "-p", "0x8863","-d", "ff:ff:ff:ff:ff:ff", "-j", "block_pppoe");
#endif
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_flow_flush();
#endif

#ifdef _PRMT_X_CT_COM_WANEXT_
	rtk_fw_set_ipforward(1);
#endif

#ifdef CONFIG_USER_MAP_E
	if (!isBoot){
		mape_recover_firewalls();
	}
#endif
#if defined(DHCP_ARP_IGMP_RATE_LIMIT)
	initDhcpIgmpArpLimit();
#endif
#if defined(CONFIG_RTL_CFG80211_WAPI_SUPPORT)
	va_cmd(EBTABLES, 8, 1, "-t", "broute", "-D", "BROUTING", "-p", "0x88b4", "-j", "DROP");
	va_cmd(EBTABLES, 8, 1, "-t", "broute", "-I", "BROUTING", "-p", "0x88b4", "-j", "DROP");
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#ifdef CONFIG_APACHE_FELIX_FRAMEWORK
	rtk_cmcc_init_traffic_mirror_rule();
#ifdef CONFIG_CMCC_TRAFFIC_PROCESS_RULE_SUPPORT
	rtk_cmcc_init_traffic_process_rule();
#endif //CONFIG_CMCC_TRAFFIC_PROCESS_RULE_SUPPORT
	rtk_cmcc_init_traffic_qos_rule();
#endif
#endif
#endif

#ifdef CONFIG_USER_IP_QOS
	setupQosRuleMainChain(1);
#endif
	setup_dscp_remark_rule_high();
#ifdef YUEME_4_0_SPEC
	apply_approute(((!isBoot)?2:isBoot));

	apply_hwAccCancel(((!isBoot)?2:isBoot));
#endif
	setTmpBlockBridgeFloodPktRule(0);

#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", FW_PUSHWEB_FOR_UPGRADE);
	va_cmd(IPTABLES, 17, 1, "-t", "nat", "-D", "PREROUTING", "-i", BRIF, "-p", "tcp", "--dport", "80", "-m", "addrtype", "!", "--dst-typ", "LOCAL", "-j", FW_PUSHWEB_FOR_UPGRADE);
	va_cmd(IPTABLES, 17, 1, "-t", "nat", "-I", "PREROUTING", "-i", BRIF, "-p", "tcp", "--dport", "80", "-m", "addrtype", "!", "--dst-typ", "LOCAL", "-j", FW_PUSHWEB_FOR_UPGRADE);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "nat", "-N", FW_PUSHWEB_FOR_UPGRADE);
	va_cmd(IP6TABLES, 17, 1, "-t", "nat", "-D", "PREROUTING", "-i", BRIF, "-p", "tcp", "--dport", "80", "-m", "addrtype", "!", "--dst-typ", "LOCAL", "-j", FW_PUSHWEB_FOR_UPGRADE);
	va_cmd(IP6TABLES, 17, 1, "-t", "nat", "-I", "PREROUTING", "-i", BRIF, "-p", "tcp", "--dport", "80", "-m", "addrtype", "!", "--dst-typ", "LOCAL", "-j", FW_PUSHWEB_FOR_UPGRADE);
#endif

#endif

#ifdef SUPPORT_DOS_SYSLOG
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", FW_DOS);
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-I", FW_INPUT, "-j", FW_DOS);

#endif

#ifdef USE_DEONET /* sghong, 20231030 */
	va_cmd(IPTABLES, 2, 1, "-N", (char *)UPNP_ACL);
	va_cmd(IPTABLES, 4, 1, (char *)FW_INSERT, "FORWARD", "-j", (char *)UPNP_ACL);

	va_cmd(EBTABLES, 2, 1, "-N", (char *)UPNP_ACL);
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, "FORWARD", "-j", (char *)UPNP_ACL);

	/* iptables -A INPUT -i ifname -j DROP,  input chain 제일 아래에 추가 되어야 함 */
	va_cmd(IPTABLES, 6, 1, (char *)FW_DEL, "INPUT", "-i", ifname, "-j", "DROP");
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, "INPUT", "-i", ifname, "-j", "DROP");

	deo_upnpRuleSet(wan_mode, isIpv6Enable, ifname);
	deo_skbaclRuleSet(wan_mode, ifname);
	deo_rateRuleSet(wan_mode, ifname);
	deo_cpurateRuleSet(wan_mode, ifname);
	deo_ebtables_dropRuleSet(wan_mode, isIpv6Enable);

	/* iptables -I INPUT -i %s -p icmp --icmp-type 8 -j ACCEPT
	 * %s : "br0" or "nas0+"
	 */
	va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, "INPUT", "-i", ifname, "-p", "icmp", "--icmp-type", "8", "-j", "ACCEPT");
	va_cmd(IPTABLES, 10, 1, (char *)FW_INSERT, "INPUT", "-i", ifname, "-p", "icmp", "--icmp-type", "8", "-j", "ACCEPT");
#endif

	return 1;
}
#if defined(CONFIG_USER_RTK_MULTI_BRIDGE_WAN)
int block_br_mode_br_wan()
{
	int total;
	int i;
	MIB_CE_BR_WAN_T Entry;
	char ifname[IFNAMSIZ] = {0};
	// INPUT
	va_cmd(EBTABLES, 2, 1, "-N", FW_BR_WAN);
	va_cmd(EBTABLES, 3, 1, "-P", FW_BR_WAN, "RETURN");
	// OUTPUT
	va_cmd(EBTABLES, 2, 1, "-N", FW_BR_WAN_OUT);
	va_cmd(EBTABLES, 3, 1, "-P", FW_BR_WAN_OUT, "RETURN");
	va_cmd(EBTABLES, 2, 1, "-N", FW_BR_LAN_OUT);
	va_cmd(EBTABLES, 3, 1, "-P", FW_BR_LAN_OUT, "RETURN");
#ifdef CONFIG_IPV6
	// create chain
	va_cmd(EBTABLES, 2, 1, "-N", FW_BR_WAN_FORWARD);
	va_cmd(EBTABLES, 3, 1, "-P", FW_BR_WAN_FORWARD, "RETURN");
#endif
	// check all wan interfaces
	total = mib_chain_total(MIB_BR_WAN_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_BR_WAN_TBL, i, (void *)&Entry))
			continue;
		if(!Entry.enable)
			continue;
		ifGetName(PHY_INTF(Entry.ifIndex), ifname, sizeof(ifname));
		if(Entry.cmode == CHANNEL_MODE_BRIDGE)
		{
			va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_BR_WAN, "-i", ifname, "-j", "DROP");
			va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_BR_WAN_OUT, "-o", ifname, "-j", "DROP");
		}
		#ifdef CONFIG_IPV6
		if(Entry.cmode == CHANNEL_MODE_BRIDGE && Entry.IpProtocol == IPVER_IPV4) // ipv4 only, drop ipv6
		{
			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_WAN_FORWARD, "-p", "ipv6", "-i", ifname, "-j", "DROP");
			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_WAN_FORWARD, "-p", "ipv6", "-o", ifname, "-j", "DROP");
		}
		else if(Entry.cmode == CHANNEL_MODE_BRIDGE && Entry.IpProtocol == IPVER_IPV6) // ipv6 only, drop ipv4
		{
			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_WAN_FORWARD, "-p", "ipv4", "-i", ifname, "-j", "DROP");
			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_WAN_FORWARD, "-p", "ipv4", "-o", ifname, "-j", "DROP");
		}
		else
			printf("[%s:%d] Entry.cmode =%d, Entry.IpProtocol=%d\n", __func__, __LINE__, Entry.cmode, Entry.IpProtocol);
		#endif
	}

	if(total)
	{
		// create chain
		va_cmd(EBTABLES, 2, 1, "-N", FW_BR_WAN_FORWARD);
		va_cmd(EBTABLES, 3, 1, "-P", FW_BR_WAN_FORWARD, "RETURN");

#if defined(CONFIG_YUEME)
		unsigned char tmpBuf[100]={0};

		mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)tmpBuf, sizeof(tmpBuf));
		//ebtables -I br_wan_forward -o nas0+ -p 0x0806 --arp-ip-dst 192.168.1.1/24 -j DROP
		va_cmd(EBTABLES, 10, 1, (char *)FW_INSERT, FW_BR_WAN_FORWARD, "-o", "nas0+", "-p", "0x0806", "--arp-ip-dst", get_lan_ipnet(), "-j", "DROP");//drop v4 lan ip from wan
		va_cmd(EBTABLES, 10, 1, (char *)FW_INSERT, FW_BR_WAN_FORWARD, "-i", "nas0+", "-p", "0x0806", "--arp-ip-src", get_lan_ipnet(), "-j", "DROP");//drop v4 lan ip from wan
		//ebtables -I br_wan_forward -i nas0+ -p ip6 --ip6-source fe80::1 --ip6-protocol ipv6-icmp --ip6-icmp-type 135:136 -j DROP
		va_cmd(EBTABLES, 14, 1, (char *)FW_INSERT, FW_BR_WAN_FORWARD, "-i", "nas0+", "-p", "ip6",  "--ip6-src", tmpBuf, "--ip6-protocol", "ipv6-icmp", "--ip6-icmp-type", "135:136","-j", "DROP");//drop v6 lan ip from wan
#endif
	}

	// add to INPUT chain
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", FW_BR_WAN);
	// add to OUTPUT chain
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-j", FW_BR_WAN_OUT);
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-j", FW_BR_LAN_OUT);
#ifdef CONFIG_IPV6
	// add to INPUT chain
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", FW_BR_WAN_FORWARD);
#endif
	return 0;
}


#if defined(CONFIG_BOLOCK_IPTV_FROM_LAN_SERVER)
void setup_br_mode_lan_to_lan_multicast_block_rules(void)
{
	int total_entry = 0;
	int i, j;
	MIB_CE_BR_WAN_T Entry;
	uint16_t itfGroup;
	char cmd[128] = {0};
	//drop multicast data drom lan port
	va_cmd(EBTABLES, 2, 1, "-N", LAN_TO_LAN_MULTICAST_BLOCK);
	va_cmd(EBTABLES, 3, 1, "-P", LAN_TO_LAN_MULTICAST_BLOCK, "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", LAN_TO_LAN_MULTICAST_BLOCK);
	va_cmd(EBTABLES, 2, 1, "-N", LAN_QUERY_BLOCK);
	va_cmd(EBTABLES, 3, 1, "-P", LAN_QUERY_BLOCK, "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-j", LAN_QUERY_BLOCK);
	if(getOpMode() == BRIDGE_MODE)
	{
		total_entry = mib_chain_total(MIB_BR_WAN_TBL);
		for (i = 0; i < total_entry; i++)
		{
			if (!mib_chain_get(MIB_BR_WAN_TBL, i, (void *)&Entry))
				continue;
			if(!Entry.enable)
				continue;
#ifdef CONFIG_USER_CTCAPD
			if(Entry.cmode == CHANNEL_MODE_BRIDGE && Entry.iptvwan == 1)
#else
			if(Entry.cmode == CHANNEL_MODE_BRIDGE)
#endif
			{
				system("echo enable > /proc/driver/realtek/drop_igmp_query_from_lan");
				itfGroup	= Entry.itfGroup;
				for(j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j)
				{
					if(BIT_IS_SET(itfGroup, j))
					{
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,ELANVIF[j],"--ip-dport","520","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,ELANVIF[j],"--ip6-dport","521","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,ELANVIF[j],"--ip-dport","546:547","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,ELANVIF[j],"--ip6-dport","546:547","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,ELANVIF[j],"--ip-dport","1900","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,ELANVIF[j],"--ip6-dport","1900","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,ELANVIF[j],"--ip-dport","5353","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,ELANVIF[j],"--ip6-dport","5353","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,ELANVIF[j],"-d","01:00:5e:00:00:00/ff:ff:ff:00:00:00","-j","DROP");
						va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,ELANVIF[j],"-d","33:33:00:00:00:00/ff:ff:00:00:00:00","-j","DROP");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_QUERY_BLOCK,"-p","IPv6","-i" ,ELANVIF[j],"--ip6-proto","ipv6-icmp","--ip6-icmp-type","130","-j","DROP");
						memset(cmd, 0, sizeof(cmd));
						snprintf(cmd, sizeof(cmd), "echo ifname %s > /proc/driver/realtek/drop_igmp_query_from_lan", ELANVIF[j]);
						system(cmd);
					}
				}
#ifdef WLAN_SUPPORT
				for(j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)
				{
					if(BIT_IS_SET(itfGroup, j))
					{
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip-dport","520","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip6-dport","521","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip-dport","546:547","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip6-dport","546:547","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip-dport","1900","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip6-dport","1900","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip-dport","5353","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"--ip6-dport","5353","-j","RETURN");
						va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv4","--ip-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"-d","01:00:5e:00:00:00/ff:ff:ff:00:00:00","-j","DROP");
						va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)LAN_TO_LAN_MULTICAST_BLOCK,"-p","IPv6","--ip6-proto","udp","-i" ,wlan[j-PMAP_WLAN0],"-d","33:33:00:00:00:00/ff:ff:00:00:00:00","-j","DROP");
						va_cmd(EBTABLES, 12, 1, (char *)FW_INSERT, (char *)LAN_QUERY_BLOCK,"-p","IPv6","-i" ,wlan[j-PMAP_WLAN0],"--ip6-proto","ipv6-icmp","--ip6-icmp-type","130","-j","DROP");
						memset(cmd, 0, sizeof(cmd));
						snprintf(cmd, sizeof(cmd), "echo ifname %s > /proc/driver/realtek/drop_igmp_query_from_lan", wlan[j-PMAP_WLAN0]);
						system(cmd);
					}
				}
#endif
			}
		}
	}
	return;
}
#endif
int setBrWanFirewall()
{
	char *argv[20];
	unsigned char value[32];
	int vInt, i, total, vcNum;
	char *policy, *filterSIP, *filterDIP, srcPortRange[12], dstPortRange[12];
	char ipaddr[32], srcip[20], dstip[20];
	char ifname[16], extra[32];
	char srcmacaddr[18], dstmacaddr[18];
	int spc_enable, spc_ip;
	// Added by Mason Yu for ACL
	unsigned char aclEnable, domainEnable, cwmp_aclEnable;
	unsigned char dhcpvalue[32];
	unsigned char vChar;
	int dhcpmode;
#ifdef REMOTE_ACCESS_CTL
	// iptables -P INPUT DROP
	va_cmd(IPTABLES, 3, 1, "-P", (char *)FW_INPUT, (char *)FW_DROP);
#else // IP_ACL
	// iptables -P INPUT ACCEPT
	va_cmd(IPTABLES, 3, 1, "-P", (char *)FW_INPUT, (char *)FW_ACCEPT);
#endif
	// iptables -P FORWARD ACCEPT
	va_cmd(IPTABLES, 3, 1, "-P", (char *)FW_FORWARD, (char *)FW_ACCEPT);
#ifdef REMOTE_ACCESS_CTL
	// iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-m", "state","--state", "ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);
#endif
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p","udp", (char *)FW_DPORT, "520", "-j", (char *)FW_ACCEPT);
	// Drop UPnP SSDP from the WAN side
	va_cmd(IPTABLES, 9, 1, (char *)FW_ADD, (char *)FW_INPUT, "!", (char *)ARG_I, (char *)LANIF_ALL, "-d", "239.255.255.250", "-j", (char *)FW_DROP);
	// Allowing multicast access, ie. IGMP, RIPv2 // iptables -A INPUT -d 224.0.0.0/4 -j ACCEPT
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_INPUT, "-d", "224.0.0.0/4", "-j", (char *)FW_ACCEPT);
	block_br_mode_br_wan();
#if defined(CONFIG_BOLOCK_IPTV_FROM_LAN_SERVER)
	//drop multicast data drom lan port
	setup_br_mode_lan_to_lan_multicast_block_rules();
#endif

#ifdef REMOTE_ACCESS_CTL
	// iptables -A INPUT -i $LAN_INTERFACE -j ACCEPT
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_INPUT, (char *)ARG_I, (char *)LANIF_ALL,
		"-j", (char *)FW_ACCEPT);
#endif

}
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
int rtk_filter_url(char *domainStr, char *ifname)
{
#ifdef CONFIG_NETFILTER_XT_MATCH_DNS
	va_cmd(IPTABLES, 15, 1, (char *)FW_ADD, "ntpblk", "-o", ifname, "-p", "udp", "--dport", "53", "-m", "dns", "--qname", domainStr, "--rmatch", "-j", (char *)FW_DROP);
#else
	unsigned char cmpStr[MAX_DOMAIN_LENGTH]="\0";
	unsigned char seg[MAX_DOMAIN_GROUP][MAX_DOMAIN_SUB_STRING];
	int j, k;
	unsigned char *needle_tmp, *str;

	needle_tmp = domainStr;

	for (j=0; (str =strchr(needle_tmp, '.'))!= NULL; j++)
	{
		*str = '\0';

		strncpy(seg[j]+1, needle_tmp, (MAX_DOMAIN_SUB_STRING - 1));
		seg[j][MAX_DOMAIN_SUB_STRING - 1]='\0';
		//printf(" seg[%d]= %s...(1)\n", j, seg[j]);

		//seg[j][0]= len[j];
		seg[j][0] = strlen(needle_tmp);
		//printf(" seg[%d]= %s...(2)\n", j, seg[j]);

		needle_tmp = str+1;
	}

	// calculate the laster domain sub string lengh and form the laster compare sub string
	strncpy(seg[j]+1, needle_tmp, (MAX_DOMAIN_SUB_STRING - 1));
	seg[j][MAX_DOMAIN_SUB_STRING - 1]='\0';

	seg[j][0]= strlen(needle_tmp);
	//printf(" seg[%d]= %s...(3)\n", j, seg[j]);

	// Merge the all compare sub string into a final compare string.
	for ( k=0; k<=j; k++) {
		//printf(" seg[%d]= %s", k, seg[k]);
		strcat(cmpStr, seg[k]);
		//printf(" cmpStr=%s\n", cmpStr);
	}
	//printf("\n");

	// iptables -A ntpblk -o ppp0 -p udp --dport 53 -m string --domain yahoo.com -j DROP
	va_cmd(IPTABLES, 16, 1, (char *)FW_ADD, "ntpblk", "-o", ifname, "-p", "udp", "--dport", "53", "-m", "string", "--domain", cmpStr, "--algo", "bm", "-j", (char *)FW_DROP);
#endif
	return 0;
}

int rtk_pppoe_prohibit_sntp_trigger_dial_on_demand(MIB_CE_ATM_VC_Tp pEntry)
{
	struct in_addr server_addr;
	struct in_addr inAddr;
	unsigned int ntpItf;
	unsigned char enable, serverID;
	int serverIndex;
	char sVal[64], ifname[IFNAMSIZ];

	if (pEntry->cmode != CHANNEL_MODE_PPPOE)
		return -1;
	if (pEntry->pppCtype != CONNECT_ON_DEMAND)
		return -1;

	va_cmd(IPTABLES, 2, 1, "-F", "ntpblk");

	ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));
	if ((getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 1) && (inAddr.s_addr != 0x40404040))
		return 0;

	mib_get_s( MIB_NTP_ENABLED, (void *)&enable, sizeof(enable));
	if (1 == enable)
	{
		mib_get_s(MIB_NTP_EXT_ITF, (void *)&ntpItf, sizeof(ntpItf));
		if ((ntpItf != 65535) && (ntpItf != pEntry->ifIndex))
			return -1;

		mib_get_s( MIB_NTP_SERVER_ID, (void *)&serverID, sizeof(serverID));
#ifndef CONFIG_TELMEX_DEV
		if (serverID == 0) {
			mib_get_s(MIB_NTP_SERVER_HOST1, sVal, sizeof(sVal));
			if (0 == inet_aton(sVal, &server_addr))
			{
				rtk_filter_url(sVal, ifname);
			}
			mib_get_s(MIB_NTP_SERVER_HOST2, (void *)sVal, sizeof(sVal));
			if (0 == inet_aton(sVal, &server_addr))
			{
				rtk_filter_url(sVal, ifname);
			}
		}
		else
		{
			mib_get_s(MIB_NTP_SERVER_HOST5, (void *)sVal, sizeof(sVal));
			if (0 == inet_aton(sVal, &server_addr))
			{
				rtk_filter_url(sVal, ifname);
			}
			mib_get_s(MIB_NTP_SERVER_HOST4, (void *)sVal, sizeof(sVal));
			if (0 == inet_aton(sVal, &server_addr))
			{
				rtk_filter_url(sVal, ifname);
			}
			mib_get_s(MIB_NTP_SERVER_HOST3, (void *)sVal, sizeof(sVal));
			if (0 == inet_aton(sVal, &server_addr))
			{
				rtk_filter_url(sVal, ifname);
			}
			mib_get_s(MIB_NTP_SERVER_HOST2, (void *)sVal, sizeof(sVal));
			if (0 == inet_aton(sVal, &server_addr))
			{
				rtk_filter_url(sVal, ifname);
			}
			mib_get_s(MIB_NTP_SERVER_HOST1, (void *)sVal, sizeof(sVal));
			if (0 == inet_aton(sVal, &server_addr))
			{
				rtk_filter_url(sVal, ifname);
			}
		}
#else
		for(serverIndex=0; serverIndex <5; serverIndex++)
		{
			if (serverID & (1<<serverIndex))
			{
				if (serverIndex == 0)
				{
					mib_get_s(MIB_NTP_SERVER_HOST1, (void *)sVal, sizeof(sVal));
					if (0 == inet_aton(sVal, &server_addr))
					{
						rtk_filter_url(sVal, ifname);
					}
				}
				else if (serverIndex == 1)
				{
					mib_get_s(MIB_NTP_SERVER_HOST2, (void *)sVal, sizeof(sVal));
					if (0 == inet_aton(sVal, &server_addr))
					{
						rtk_filter_url(sVal, ifname);
					}
				}
				else if (serverIndex == 2)
				{
					mib_get_s(MIB_NTP_SERVER_HOST3, (void *)sVal, sizeof(sVal));
					if (0 == inet_aton(sVal, &server_addr))
					{
						rtk_filter_url(sVal, ifname);
					}
				}
				else if (serverIndex == 3)
				{
					mib_get_s(MIB_NTP_SERVER_HOST4, (void *)sVal, sizeof(sVal));
					if (0 == inet_aton(sVal, &server_addr))
					{
						rtk_filter_url(sVal, ifname);
					}
				}
				else if (serverIndex == 4)
				{
					mib_get_s(MIB_NTP_SERVER_HOST5, (void *)sVal, sizeof(sVal));
					if (0 == inet_aton(sVal, &server_addr))
					{
						rtk_filter_url(sVal, ifname);
					}
				}
			}
		}
#endif
	}

	return 0;
}
#endif

#ifdef TIME_ZONE
int rtk_sntp_is_bind_on_wan(MIB_CE_ATM_VC_Tp entry)
{
	struct in_addr server_addr;
	struct in_addr inAddr;
	unsigned char enable;
	char ifname[IFNAMSIZ];
#if defined(CONFIG_E8B)
	unsigned char if_type = 0, wan_type = 0;
#endif
	unsigned int if_wan, ifIndex = DUMMY_IFINDEX;
	int total, i;

	mib_get_s(MIB_NTP_ENABLED, (void *)&enable, sizeof(enable));
	if (!enable)
		return 0;

	if (entry->cmode == CHANNEL_MODE_BRIDGE)
		return 0;

#if defined(CONFIG_E8B)
	mib_get_s(MIB_NTP_IF_TYPE, &if_type, sizeof(if_type));
	mib_get_s(MIB_NTP_IF_WAN, &if_wan, sizeof(if_wan));
	switch(if_type)
	{
		case 0: //INTERNET
			wan_type = X_CT_SRV_INTERNET;
			break;
		case 1: //VOICE
			wan_type = X_CT_SRV_VOICE;
			break;
		case 2: //TR069
			wan_type = X_CT_SRV_TR069;
			break;
		case 3:
			wan_type = X_CT_SRV_OTHER;
			break;
	}
#else
	mib_get_s(MIB_NTP_EXT_ITF, (void *)&if_wan, sizeof(if_wan));
#endif
	if (if_wan == DUMMY_IFINDEX)
	{
		AUG_PRT("%s:NTP inf is DUMMY!\n", __FUNCTION__);
		return 0;
	}

	if ((entry->cmode == CHANNEL_MODE_PPPOE) && (entry->pppCtype == CONNECT_ON_DEMAND))
	{
		ifGetName(entry->ifIndex, ifname, sizeof(ifname));
		if ((getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 1) && (inAddr.s_addr != 0x40404040))
			return 0;
	}

#if defined(CONFIG_E8B)
	if (entry->applicationtype & wan_type)
#endif
	{
		if (entry->ifIndex == if_wan)
			return 1;
	}
	return 0;
}
#endif

#ifdef WEB_REDIRECT_BY_MAC
int start_web_redir_by_mac(void)
{
	int status=0;
	char tmpbuf[MAX_URL_LEN];
	char ipaddr[16], ip_port[32], ip_port2[32], redir_server[33];
	int  def_port=WEB_REDIR_BY_MAC_PORT;
	int  def_port2=WEB_REDIR_BY_MAC_PORT_SSL;

	ipaddr[0]='\0'; ip_port[0]='\0';redir_server[0]='\0';
	if (mib_get_s(MIB_ADSL_LAN_IP, (void *)tmpbuf, sizeof(tmpbuf)) != 0)
	{
		strncpy(ipaddr, inet_ntoa(*((struct in_addr *)tmpbuf)), 16);
		ipaddr[15] = '\0';
		sprintf(ip_port,"%s:%d",ipaddr,def_port);
		sprintf(ip_port2,"%s:%d",ipaddr,def_port2);
	}//else ??

	//iptables -t nat -N WebRedirectByMAC
	status|=va_cmd(IPTABLES, 4, 1, "-t", "nat","-N","WebRedirectByMAC");

	//iptables -t nat -A WebRedirectByMAC -d 192.168.1.1 -j RETURN
	status|=va_cmd(IPTABLES, 8, 1, "-t", "nat",(char *)FW_ADD,"WebRedirectByMAC",
		"-d", ipaddr, "-j", (char *)FW_RETURN);

	//iptables -t nat -A WebRedirectByMAC -p tcp --dport 80 -j DNAT --to-destination 192.168.1.1:18080
	status|=va_cmd(IPTABLES, 12, 1, "-t", "nat",(char *)FW_ADD,"WebRedirectByMAC",
		"-p", "tcp", "--dport", "80", "-j", "DNAT",
		"--to-destination", ip_port);

	//iptables -t nat -A PREROUTING -i eth0 -p tcp --dport 80 -j WebRedirectByMAC
	status|=va_cmd(IPTABLES, 12, 1, "-t", "nat",(char *)FW_ADD,"PREROUTING",
		"-i", LANIF,
		"-p", "tcp", "--dport", "80", "-j", "WebRedirectByMAC");

	//iptables -t nat -A WebRedirectByMAC -p tcp --dport 443 -j DNAT --to-destination 192.168.1.1:10443
	status|=va_cmd(IPTABLES, 12, 1, "-t", "nat","-A","WebRedirectByMAC",
		"-p", "tcp", "--dport", "443", "-j", "DNAT",
		"--to-destination", ip_port2);

	//iptables -t nat -A PREROUTING -i eth0 -p tcp --dport 443 -j WebRedirectByMAC
	status|=va_cmd(IPTABLES, 12, 1, "-t", "nat","-A","PREROUTING",
		"-i", LANIF,
		"-p", "tcp", "--dport", "443", "-j", "WebRedirectByMAC");


{
	int num,i;
	unsigned char tmp2[18];
	MIB_WEB_REDIR_BY_MAC_T	wrm_entry;

	num = mib_chain_total( MIB_WEB_REDIR_BY_MAC_TBL );
	//printf( "\nnum=%d\n", num );
	for(i=0;i<num;i++)
	{
		if( !mib_chain_get( MIB_WEB_REDIR_BY_MAC_TBL, i, (void*)&wrm_entry ) )
			continue;

		sprintf( tmp2, "%02X:%02X:%02X:%02X:%02X:%02X",
				wrm_entry.mac[0], wrm_entry.mac[1], wrm_entry.mac[2], wrm_entry.mac[3], wrm_entry.mac[4], wrm_entry.mac[5] );
		//printf( "add one mac: %s \n", tmp2 );
		// iptables -A macfilter -i eth0  -m mac --mac-source $MAC -j ACCEPT/DROP
		status|=va_cmd("/bin/iptables", 10, 1, "-t", "nat", (char *)FW_INSERT, "WebRedirectByMAC", "-m", "mac", "--mac-source", tmp2, "-j", "RETURN");
	}
}

	return 0;
}
#endif

#ifdef CONFIG_USER_RTK_VOIP
#include "web_voip.h"
static void voip_add_iptable_rules(int is_dmz, char *portbuff)
{
	int i = 0, total = 0;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ] = {0};

	if (is_dmz) {
		va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_DEL,
		       (char *)IPTABLE_DMZ, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_UDP, (char *)FW_DPORT, portbuff, "-j",
		       (char *)FW_ACCEPT);

		va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_ADD,
		       (char *)IPTABLE_DMZ, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_UDP, (char *)FW_DPORT, portbuff, "-j",
		       (char *)FW_ACCEPT);

	} else {
		total = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				return;
			if (Entry.cmode == CHANNEL_MODE_BRIDGE)
				continue;
			if (Entry.applicationtype & X_CT_SRV_VOICE)
			{
				ifGetName(Entry.ifIndex, ifname, sizeof(ifname));
				va_cmd(IPTABLES, 10, 1, (char *)FW_INSERT, (char *)IPTABLE_VOIP,
						(char *)ARG_I, ifname, "-p", (char *)ARG_UDP,
						(char *)FW_DPORT, portbuff, "-j", (char *)"ACCEPT");
			}
		}
	}

}

/*----------------------------------------------------------------------------
 * Name:
 *      voip_setup_iptable
 * Descriptions:
 *      Creat an iptable rule to allow incoming VoIP calls.
 * return:              none
 *---------------------------------------------------------------------------*/
void voip_setup_iptable(int is_dmz)
{
	char portbuff[16];
	int i, val;
	unsigned short sip_port[VOIP_PORTS], media_port[VOIP_PORTS], T38_port[VOIP_PORTS], h248_port[VOIP_PORTS];
	voipCfgParam_t *pCfg;
	voipCfgPortParam_t *VoIPport;

	val = voip_flash_get(&pCfg);
	if (val != 0) {
		/* default value */
		for (i = 0; i < VOIP_PORTS; i++) {
			sip_port[i] = 5060 + i;
			media_port[i] = 9000 + i * 4;
			T38_port[i] = 9008 + i * 2;
			h248_port[i] = 2944;
		}
	} else {
		for (i = 0; i < VOIP_PORTS; i++) {
			VoIPport = &pCfg->ports[i];

			sip_port[i] = VoIPport->sip_port;
			media_port[i] = VoIPport->media_port;
			T38_port[i] = VoIPport->T38_port;
			h248_port[i] = pCfg->mg_port;
		}
	}

	if(!is_dmz){
		va_cmd(IPTABLES, 4, 1, "-D",(char *)FW_INPUT,"-j", IPTABLE_VOIP);
		va_cmd(IPTABLES, 2, 1, "-F", IPTABLE_VOIP);
		va_cmd(IPTABLES, 2, 1, "-X", IPTABLE_VOIP);
		va_cmd(IPTABLES, 2, 1, "-N", IPTABLE_VOIP);
		va_cmd(IPTABLES, 4, 1, "-I" ,(char *)FW_INPUT,"-j", IPTABLE_VOIP);
	}

	for (i = 0; i < VOIP_PORTS; i++) {
		sprintf(portbuff, "%hu", sip_port[i]);
		voip_add_iptable_rules(is_dmz, portbuff);

		sprintf(portbuff, "%hu:%hu", media_port[i], media_port[i] + 3);
		voip_add_iptable_rules(is_dmz, portbuff);

		sprintf(portbuff, "%hu", T38_port[i]);
		voip_add_iptable_rules(is_dmz, portbuff);

		sprintf(portbuff, "%hu", h248_port[i]);
		voip_add_iptable_rules(is_dmz, portbuff);
	}
}
#endif

#if defined(CONFIG_USER_RTK_VOIP)
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
void voip_set_fc_1p_tag(void)
{
	voipCfgParam_t * pCfg = NULL;;
	voipCfgPortParam_t *pSip_conf;
	int i,val, vcTotal=0, haveVoipWan=0;
	unsigned int mark=0, mark_mask=0, qid=0, sip_pri=0, rtp_pri=0;
	printf("voip_set_rg_1p_tag\n");
	char *iptables_cmd=NULL;
	char f_skb_mark_mask[32]={0}, f_skb_mark_value[32]={0};
	char rule_alias_name[256]={0}, rule_priority[32]={0};
	char devname[IFNAMSIZ]={0}, ifname[IFNAMSIZ]={0};
	char priority_str[32]={0};
	MIB_CE_ATM_VC_T Entry;
	unsigned int totalEntry =0;
	voipCfgParam_t VoipEntry;
	DOCMDINIT

	totalEntry = mib_chain_total(MIB_VOIP_CFG_TBL);
	if( totalEntry > 0 ){
		if(mib_chain_get(MIB_VOIP_CFG_TBL, 0, (void*)&VoipEntry)){
			pCfg = &VoipEntry;
		}else{
			fprintf(stderr, "[%s %d]read voip config fail.\n",__FUNCTION__,__LINE__);
		}
	}else{
		fprintf(stderr, "[%s %d]flash do no have voip configuration.\n",__FUNCTION__,__LINE__);
	}

	if (!pCfg) return ;

	pSip_conf = &pCfg->ports[0];

	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", VOIP_REMARKING_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", VOIP_REMARKING_CHAIN);
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return;
		if(Entry.applicationtype & X_CT_SRV_VOICE) {
			haveVoipWan=1;
			ifGetName(Entry.ifIndex,ifname,sizeof(ifname));
		}
	}
	if (!haveVoipWan) return ;

	qid = getIPQosQueueNumber()-1;
	iptables_cmd = "/bin/iptables";
	sprintf(devname, "%s", ALIASNAME_NAS0);
	sprintf(rule_alias_name, "voip_remark_8021p_us_%s", devname);
	va_cmd(SMUXCTL, 7, 1, "--if", devname, "--tx", "--tags", "1", "--rule-remove-alias", "voip_remark_8021p_us_+");

		/* wanVlanPriorityVoice is sip packet
		wanVlanPriorityData is rtp packet */
#if defined(CONFIG_00R0)
		fprintf(stderr, "voip RTP 1p tag is wanVlanPriorityData %d\n", pCfg->wanVlanPriorityData);
		fprintf(stderr, "voip SIP 1p tag is wanVlanPriorityVoice %d\n", pCfg->wanVlanPriorityVoice);
		sip_pri = pCfg->wanVlanPriorityVoice;
		rtp_pri = pCfg->wanVlanPriorityData;
#endif
		//SIP
		if (sip_pri !=0) {
			mark |= ((sip_pri) << SOCK_MARK_8021P_START) & SOCK_MARK_8021P_BIT_MASK;
			mark |= SOCK_MARK_8021P_ENABLE_TO_MARK(1);
			mark_mask = SOCK_MARK_8021P_BIT_MASK|SOCK_MARK_8021P_ENABLE_BIT_MASK;
		}
#ifdef CONFIG_RTL_SMUX_DEV
		sprintf(f_skb_mark_mask, "0x%x", SOCK_MARK_8021P_BIT_MASK|SOCK_MARK_8021P_ENABLE_BIT_MASK);
		sprintf(f_skb_mark_value, "0x%x", mark);
		sprintf(rule_priority, "%d", SMUX_RULE_PRIO_VOIP);
		sprintf(priority_str, "%d", sip_pri);
		if (sip_pri !=0)
			va_cmd(SMUXCTL, 16, 1, "--if", "nas0", "--tx", "--tags", "1", "--filter-skb-mark", f_skb_mark_mask, f_skb_mark_value, "--set-priority", priority_str, "1", "--rule-priority", rule_priority, "--rule-alias", rule_alias_name, "--rule-append");
#endif

		mark |= (qid)<<SOCK_MARK_QOS_SWQID_START;
		mark_mask |= SOCK_MARK_QOS_SWQID_BIT_MASK;

		//iptables -t mangle -A POSTROUTING -o nas0_0 -p UDP --dport 5060 -j MARK --set-mark 0x80/0x1f80
		DOCMDARGVS(iptables_cmd, DOWAIT,  "-t mangle -A  %s -o %s -p UDP --dport %d -j MARK --set-mark 0x%x/0x%x",
			VOIP_REMARKING_CHAIN, ifname, pSip_conf->sip_port, mark, mark_mask);
		//rtp port range. reserve 10 port for rtp usage... (should modify);
		//RTP
		mark = 0;
		mark_mask=0;

		if (rtp_pri !=0) {
			mark |= (rtp_pri << SOCK_MARK_8021P_START) & SOCK_MARK_8021P_BIT_MASK;
			mark |= SOCK_MARK_8021P_ENABLE_TO_MARK(1);
			mark_mask = SOCK_MARK_8021P_BIT_MASK|SOCK_MARK_8021P_ENABLE_BIT_MASK;
		}
#ifdef CONFIG_RTL_SMUX_DEV
		sprintf(f_skb_mark_mask, "0x%x", SOCK_MARK_8021P_BIT_MASK|SOCK_MARK_8021P_ENABLE_BIT_MASK);
		sprintf(f_skb_mark_value, "0x%x", mark);
		sprintf(devname, "%s", ALIASNAME_NAS0);
		sprintf(rule_priority, "%d", SMUX_RULE_PRIO_VOIP);
		sprintf(priority_str, "%d", rtp_pri);
		if (rtp_pri !=0)
			va_cmd(SMUXCTL, 16, 1, "--if", "nas0", "--tx", "--tags", "1", "--filter-skb-mark", f_skb_mark_mask, f_skb_mark_value, "--set-priority", priority_str, "1", "--rule-priority", rule_priority, "--rule-alias", rule_alias_name, "--rule-append");
#endif
		mark |= (qid)<<SOCK_MARK_QOS_SWQID_START;
		mark_mask |= SOCK_MARK_QOS_SWQID_BIT_MASK;
		DOCMDARGVS(iptables_cmd, DOWAIT,  "-t mangle -A  %s -o %s -p UDP --sport %d:%d -j MARK --set-mark 0x%x/0x%x",
			VOIP_REMARKING_CHAIN, ifname, pSip_conf->media_port,  (pSip_conf->media_port+10), mark, mark_mask);

}
#endif
#endif

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
void set_CaptivePortal_Firewall(void)
{
	unsigned char cp_enable = 0;
	char portStr[8] = {0};

	snprintf(portStr, sizeof(portStr), "%d", CAPTIVEPORTAL_PORT);
	mib_get_s(MIB_CAPTIVEPORTAL_ENABLE, (void *)&cp_enable, sizeof(cp_enable));

	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_CAPTIVEPORTAL_FILTER);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)FW_CAPTIVEPORTAL_FILTER);
#endif

	if (cp_enable == 0)
	{
		/* for nmap port scan */
		va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char *)FW_CAPTIVEPORTAL_FILTER, (char *)ARG_I, (char *)LANIF,
				"-p", (char *)ARG_TCP, (char *)FW_DPORT, portStr,
				"-j", "REJECT", "--reject-with", "tcp-reset");
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_CAPTIVEPORTAL_FILTER, "!", (char *)ARG_I, (char *)LANIF,
				"-p", (char *)ARG_TCP, (char *)FW_DPORT, portStr,
				"-j", "DROP");
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 12, 1, (char *)FW_ADD, (char *)FW_CAPTIVEPORTAL_FILTER, (char *)ARG_I, (char *)LANIF,
				"-p", (char *)ARG_TCP, (char *)FW_DPORT, portStr,
				"-j", "REJECT", "--reject-with", "tcp-reset");
		va_cmd(IP6TABLES, 11, 1, (char *)FW_ADD, (char *)FW_CAPTIVEPORTAL_FILTER, "!", (char *)ARG_I, (char *)LANIF,
				"-p", (char *)ARG_TCP, (char *)FW_DPORT, portStr,
				"-j", "DROP");
#endif
	}

	return;
}

#if 0
int start_captiveportal(void)
{
	int status = 0;
	char tmpbuf[MAX_URL_LEN];
	char lan_ipaddr[16], lan_ip_port[32];
	char ip_mask[24];
	int  def_port = CAPTIVEPORTAL_PORT, i, num;
	FILE *fp;

	lan_ipaddr[0] = '\0';
	lan_ip_port[0] = '\0';

	set_CaptivePortal_Firewall();

	if (mib_get_s(MIB_ADSL_LAN_IP, (void *)tmpbuf, sizeof(tmpbuf)) != 0)
	{
		strncpy(lan_ipaddr, inet_ntoa(*((struct in_addr *)tmpbuf)), 16);
		lan_ipaddr[15] = '\0';
		sprintf(lan_ip_port, "%s:%d", lan_ipaddr, def_port);
	}
	else
		return -1;

	//iptables -t nat -A PREROUTING -p tcp -d 192.168.1.1 --dport 80 -j RETURN
	status |= va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_ADD, "PREROUTING",	"-p", "tcp", "-d", lan_ipaddr, "--dport", "80", "-j", (char *)FW_RETURN);

	//iptables -t nat -A PREROUTING -p tcp --dport 80 -j DNAT --to-destination 192.168.1.1:18182
	status |= va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_ADD, "PREROUTING", "-p", "tcp", "--dport", "80", "-j", "DNAT", "--to-destination", lan_ip_port);

	CWMP_CAPTIVEPORTAL_ALLOWED_LIST_T ccal_entry;

	if(!(fp = fopen("/var/cp_allow_ip", "w")))
	{
		fprintf(stderr, "Open file cp_allow_ip failed!");
		return -1;
	}

	num = mib_chain_total(CWMP_CAPTIVEPORTAL_ALLOWED_LIST);

	for( i = 0; i < num; i++ )
	{
		if(!mib_chain_get(CWMP_CAPTIVEPORTAL_ALLOWED_LIST, i, (void*)&ccal_entry))
			continue;

		if( ccal_entry.mask < 32 ) // Valid subnet mask
			sprintf(ip_mask, "%s/%d", inet_ntoa(*(struct in_addr *)&ccal_entry.ip_addr), ccal_entry.mask);
		else	// Invalid subnet mask or unset subnet mask.
			sprintf(ip_mask, "%s", inet_ntoa(*(struct in_addr *)&ccal_entry.ip_addr));

		// iptables -t nat -I PREROUTING -p tcp -d 209.85.175.104/24 --dport 80 -j RETURN
		status |= va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_INSERT, "PREROUTING", "-p", "tcp", "-d", ip_mask, "--dport", "80", "-j", "RETURN");
		fprintf(fp, "%s\n", ip_mask);
	}

	fclose(fp);

	return status;
}

int stop_captiveportal(void)
{
	int status = 0;
	char tmpbuf[MAX_URL_LEN];
	char lan_ipaddr[16], lan_ip_port[32];
	char ip_mask[24];
	int  def_port = CAPTIVEPORTAL_PORT, i;
	FILE *fp;

	set_CaptivePortal_Firewall();

	if(fp = fopen("/var/cp_allow_ip", "r"))
	{
		char *tmp;

		while(fgets(ip_mask, 24, fp))
		{
			tmp = strchr(ip_mask, '\n');
			*tmp = '\0';

			// iptables -t nat -D PREROUTING -p tcp -d 209.85.175.104/24 --dport 80 -j RETURN
			va_cmd(IPTABLES, 12, 1, "-t", "nat", "-D", "PREROUTING", "-p", "tcp", "-d", ip_mask, "--dport", "80", "-j", "RETURN");
		}

		fclose(fp);
		unlink("/var/cp_allow_ip");
	}

	lan_ipaddr[0] = '\0';
	lan_ip_port[0] = '\0';

	if (mib_get_s(MIB_ADSL_LAN_IP, (void *)tmpbuf, sizeof(tmpbuf)) != 0)
	{
		strncpy(lan_ipaddr, inet_ntoa(*((struct in_addr *)tmpbuf)), 16);
		lan_ipaddr[15] = '\0';
		sprintf(lan_ip_port, "%s:%d", lan_ipaddr, def_port);
	}
	else
		return -1;

	//iptables -t nat -D PREROUTING -p tcp -d 192.168.1.1 --dport 80 -j RETURN
	status |= va_cmd(IPTABLES, 12, 1, "-t", "nat", "-D", "PREROUTING", "-p", "tcp", "-d", lan_ipaddr, "--dport", "80", "-j", (char *)FW_RETURN);

	//iptables -t nat -D PREROUTING -p tcp --dport 80 -j DNAT --to-destination 192.168.1.1:18182
	status |= va_cmd(IPTABLES, 12, 1, "-t", "nat", "-D", "PREROUTING", "-p", "tcp", "--dport", "80", "-j", "DNAT", "--to-destination", lan_ip_port);

	return status;
}
#endif

int start_captiveportal(void)
{
	CWMP_CAPTIVEPORTAL_MAC_LIST_T cpal_entry;
	int status = 0;
	char tmpbuf[MAX_URL_LEN];
	char lan_ipaddr[16], portStr[10], lan_ip_port[32];
	int  def_port = CAPTIVEPORTAL_PORT, i, num;
	char macaddr[18];
/***
#ifdef CONFIG_RTK_L34_ENABLE
	char cmdStr[1024];
	char cp_url[MAX_URL_LEN], *pUrl;

	mib_get(MIB_CAPTIVEPORTAL_URL, (void *)cp_url);
	pUrl = cp_url;
	//trim head "http://" if existed
	if (!strncmp(pUrl, "http://", 7))
		pUrl += 7;
#endif//end of CONFIG_RTK_L34_ENABLE
*/
	set_CaptivePortal_Firewall();

	if (mib_get(MIB_ADSL_LAN_IP, (void *)tmpbuf) != 0)
	{
		snprintf(lan_ipaddr, sizeof(lan_ipaddr), "%s", inet_ntoa(*((struct in_addr *)tmpbuf)));
		snprintf(portStr, sizeof(portStr), "%d", def_port);
		snprintf(lan_ip_port, sizeof(lan_ip_port), "%s:%d", lan_ipaddr, def_port);
	}
	else
		return -1;

	//iptables -t nat -A PREROUTING -i br+ -p tcp -d 192.168.1.1 --dport 80 -j RETURN
	status |= va_cmd(IPTABLES, 14, 1, "-t", "nat", (char *)FW_ADD, "PREROUTING", (char *)ARG_I, LANIF_ALL, "-p", "tcp", "-d", lan_ipaddr,
					"--dport", "80", "-j", (char *)FW_RETURN);

	//iptables -t nat -A PREROUTING -i br+ -p tcp --dport 80 -j DNAT --to-destination 192.168.1.1:18182
	status |= va_cmd(IPTABLES, 14, 1, "-t", "nat", (char *)FW_ADD, "PREROUTING", (char *)ARG_I, LANIF_ALL, "-p", "tcp", "--dport", "80",
					"-j", "DNAT", "--to-destination", lan_ip_port);

/*
#ifdef CONFIG_RTK_L34_ENABLE
	snprintf(cmdStr, 1024, "/bin/echo a -2 %s > /proc/rg/redirect_first_http_req_set_url 2>&1", pUrl);
	va_cmd("/bin/sh", 2, 1, "-c", cmdStr);

	snprintf(cmdStr, 1024, "/bin/echo %s > /proc/rg/avoid_force_portal_set_url 2>&1", pUrl);
	va_cmd("/bin/sh", 2, 1, "-c", cmdStr);

	cmd_set_captive_portal();
#endif//end of CONFIG_RTK_L34_ENABLE
*/

	num = mib_chain_total(CWMP_CAPTIVEPORTAL_MAC_LIST);
	for( i = 0; i < num; i++ )
	{
		if(!mib_chain_get(CWMP_CAPTIVEPORTAL_MAC_LIST, i, (void*)&cpal_entry))
			continue;

		snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
			cpal_entry.macAddr[0], cpal_entry.macAddr[1],
			cpal_entry.macAddr[2], cpal_entry.macAddr[3],
			cpal_entry.macAddr[4], cpal_entry.macAddr[5]);

		if (cpal_entry.allowed)
		{
			// iptables -t nat -I PREROUTING -i br+ -p tcp --dport 80 -m mac --mac-source $MAC -j RETURN
			status |= va_cmd(IPTABLES, 16, 1, "-t", "nat", (char *)FW_INSERT, "PREROUTING", (char *)ARG_I, LANIF_ALL, "-p", "tcp", "--dport", "80",
					"-m", "mac", "--mac-source",  macaddr, "-j", (char *)FW_RETURN);
/*
#ifdef CONFIG_RTK_L34_ENABLE
			snprintf(cmdStr, 1024, "/bin/echo m 0 %s > /proc/rg/redirect_first_http_req_by_mac 2>&1", macaddr);
			va_cmd("/bin/sh", 2, 1, "-c", cmdStr);
			RTK_RG_Add_ACL_Captive_Portal_Rule(0, cpal_entry.macAddr);
#endif//end of CONFIG_RTK_L34_ENABLE
*/
		}
		else
		{
			//iptables -I INPUT -i br+ -p tcp --dport $CAPTIVEPORTAL_PORT -m mac --mac-source $MAC -j DROP
			status |= va_cmd(IPTABLES, 14, 1, (char *)FW_INSERT, "INPUT", (char *)ARG_I, LANIF_ALL, "-p", "tcp", "--dport", portStr,
					"-m", "mac", "--mac-source",  macaddr, "-j", (char *)FW_DROP);
/*
#ifdef CONFIG_RTK_L34_ENABLE
			snprintf(cmdStr, 1024, "/bin/echo m -1 %s > /proc/rg/redirect_first_http_req_by_mac 2>&1", macaddr);
			va_cmd("/bin/sh", 2, 1, "-c", cmdStr);
#endif//end of CONFIG_RTK_L34_ENABLE
*/
		}
	}

	return status;
}

int stop_captiveportal(void)
{
	CWMP_CAPTIVEPORTAL_MAC_LIST_T cpal_entry;
	int status = 0;
	char tmpbuf[MAX_URL_LEN];
	char lan_ipaddr[16], portStr[10], lan_ip_port[32];
	int  def_port = CAPTIVEPORTAL_PORT, i, num;
	char macaddr[18];
/*
#ifdef CONFIG_RTK_L34_ENABLE
	char cmdStr[1024];
	char cp_url[MAX_URL_LEN], *pUrl;

	mib_get(MIB_CAPTIVEPORTAL_URL, (void *)cp_url);
	pUrl = cp_url;
	// trim head "http://" if existed
	if (!strncmp(pUrl, "http://", 7))
		pUrl += 7;
#endif//end of CONFIG_RTK_L34_ENABLE
*/

	set_CaptivePortal_Firewall();

	num = mib_chain_total(CWMP_CAPTIVEPORTAL_MAC_LIST);
	for( i = 0; i < num; i++ )
	{
		if(!mib_chain_get(CWMP_CAPTIVEPORTAL_MAC_LIST, i, (void*)&cpal_entry))
			continue;

		snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
			cpal_entry.macAddr[0], cpal_entry.macAddr[1],
			cpal_entry.macAddr[2], cpal_entry.macAddr[3],
			cpal_entry.macAddr[4], cpal_entry.macAddr[5]);

		if (cpal_entry.allowed)
		{
			// iptables -t nat -D PREROUTING -i br+ -p tcp --dport 80 -m mac --mac-source $MAC -j RETURN
			status |= va_cmd(IPTABLES, 16, 1, "-t", "nat", (char *)FW_DEL, "PREROUTING", (char *)ARG_I, LANIF_ALL, "-p", "tcp", "--dport", "80",
					"-m", "mac", "--mac-source",  macaddr, "-j", (char *)FW_RETURN);
		}
		else
		{
			//iptables -D INPUT -i br+ -p tcp --dport $CAPTIVEPORTAL_PORT -m mac --mac-source $MAC -j DROP
			status |= va_cmd(IPTABLES, 14, 1, (char *)FW_DEL, "INPUT", (char *)ARG_I, LANIF_ALL, "-p", "tcp", "--dport", portStr,
					"-m", "mac", "--mac-source",  macaddr, "-j", (char *)FW_DROP);
		}
/*
#ifdef CONFIG_RTK_L34_ENABLE
		snprintf(cmdStr, 1024, "/bin/echo d %s > /proc/rg/redirect_first_http_req_by_mac 2>&1", macaddr);
		va_cmd("/bin/sh", 2, 1, "-c", cmdStr);
#endif//end of CONFIG_RTK_L34_ENABLE
*/
	}

	if (mib_get(MIB_ADSL_LAN_IP, (void *)tmpbuf) != 0)
	{
		snprintf(lan_ipaddr, sizeof(lan_ipaddr), "%s", inet_ntoa(*((struct in_addr *)tmpbuf)));
		snprintf(portStr, sizeof(portStr), "%d", def_port);
		snprintf(lan_ip_port, sizeof(lan_ip_port), "%s:%d", lan_ipaddr, def_port);
	}
	else
		return -1;

	//iptables -t nat -D PREROUTING -i br+ -p tcp -d 192.168.1.1 --dport 80 -j RETURN
	status |= va_cmd(IPTABLES, 14, 1, "-t", "nat", (char *)FW_DEL, "PREROUTING", (char *)ARG_I, LANIF_ALL, "-p", "tcp", "-d", lan_ipaddr,
					"--dport", "80", "-j", (char *)FW_RETURN);

	//iptables -t nat -D PREROUTING -i br+ -p tcp --dport 80 -j DNAT --to-destination 192.168.1.1:18182
	status |= va_cmd(IPTABLES, 14, 1, "-t", "nat", (char *)FW_DEL, "PREROUTING", (char *)ARG_I, LANIF_ALL, "-p", "tcp", "--dport", "80",
					"-j", "DNAT", "--to-destination", lan_ip_port);
/*
#ifdef CONFIG_RTK_L34_ENABLE
	snprintf(cmdStr, 1024, "/bin/echo d -2 > /proc/rg/redirect_first_http_req_set_url 2>&1");
	va_cmd("/bin/sh", 2, 1, "-c", cmdStr);

	snprintf(cmdStr, 1024, "/bin/echo %s > /proc/rg/avoid_force_portal_set_url 2>&1", pUrl);
	va_cmd("/bin/sh", 2, 1, "-c", cmdStr);
#endif//end of CONFIG_RTK_L34_ENABLE
*/

	return status;
}

#endif

#ifdef CONFIG_USER_CWMP_TR069
int set_TR069_Firewall(int enable)
{
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_TR069);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)IPTABLE_IPV6_TR069);
#endif

	if (enable)
	{
		unsigned int conreq_port = 0;
		char strPort[8] = {0}, ifname[IFNAMSIZ] = {0};
		mib_get_s(CWMP_CONREQ_PORT, (void *)&conreq_port, sizeof(conreq_port));
		if (conreq_port == 0)
			conreq_port = 7547;

		sprintf(strPort, "%u", conreq_port);

#ifdef CONFIG_USER_RTK_WAN_CTYPE
		int i = 0, total = mib_chain_total(MIB_ATM_VC_TBL);
		MIB_CE_ATM_VC_T entry;

		for (i = 0; i < total; i++)
		{
			if (mib_chain_get(MIB_ATM_VC_TBL, i, &entry) == 0)
				continue;

			if (entry.applicationtype & X_CT_SRV_TR069)
			{
				ifGetName(entry.ifIndex, ifname, IFNAMSIZ);
				break;
			}
		}
#endif

	#if 1 /* DROP cwmp packets for none-cwmp interfaces */
		#ifdef CONFIG_USER_RTK_WAN_CTYPE
#ifdef CONFIG_CU
		// LAN side
		// iptables -A tr069 -i br0 -p tcp --dport 7547 -j REJECT --reject-with tcp-reset
		va_cmd(IPTABLES, 12, 1, "-A", IPTABLE_TR069, (char *)ARG_I, (char *)LANIF, "-p", "tcp", (char *)FW_DPORT, strPort, "-j", "REJECT", "--reject-with", "tcp-reset");
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 12, 1, "-A", IPTABLE_IPV6_TR069, (char *)ARG_I, (char *)LANIF, "-p", "tcp", (char *)FW_DPORT, strPort, "-j", "REJECT", "--reject-with", "tcp-reset");
#endif
#endif

		if (strlen(ifname) > 0)
		{
			// iptables -A tr069 ! -i nas0_0 -p tcp --dport 7547 -j DROP
			va_cmd(IPTABLES, 11, 1, "-A", IPTABLE_TR069, "!", (char *)ARG_I, ifname, "-p", "tcp", (char *)FW_DPORT, strPort, "-j", "DROP");
			#ifdef CONFIG_IPV6
			//drop conreq from any other wan
			// ip6tables -A ipv6_tr069 ! -i nas0_0 -p tcp --dport 7547 -j DROP
			va_cmd(IP6TABLES, 11, 1, "-A", (char *)IPTABLE_IPV6_TR069, "!", (char *)ARG_I, ifname, "-p", (char *)ARG_TCP,
				(char *)FW_DPORT, strPort, "-j", (char *)FW_DROP);
			va_cmd(IP6TABLES, 10, 1, "-A", (char *)IPTABLE_IPV6_TR069, (char *)ARG_I, ifname, "-p", (char *)ARG_TCP,
				(char *)FW_DPORT, strPort, "-j", (char *)FW_ACCEPT);
			#endif
		}
		#ifdef CONFIG_CU
		else
		{
			va_cmd(IPTABLES, 8, 1, "-A", IPTABLE_TR069, "-p", "tcp", (char *)FW_DPORT, strPort, "-j", "DROP");
			#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 8, 1, "-A", (char *)IPTABLE_IPV6_TR069, "-p", (char *)ARG_TCP, (char *)FW_DPORT, strPort, "-j", "DROP");
			#endif
		}
		#endif
		#endif // of CONFIG_USER_RTK_WAN_CTYPE
	#else /* in case some may need to reply with icmp reject for cwmp packets comming from none-tr069 wan */
		// LAN side
		// iptables -A tr069 -i br0 -p tcp --dport 7547 -j REJECT --reject-with tcp-reset
		va_cmd(IPTABLES, 12, 1, "-A", IPTABLE_TR069, (char *)ARG_I, (char *)LANIF, "-p", "tcp", (char *)FW_DPORT, strPort, "-j", "REJECT", "--reject-with", "tcp-reset");
#ifdef CONFIG_IPV6
#ifdef CONFIG_CU
		va_cmd(IP6TABLES, 12, 1, "-A", IPTABLE_IPV6_TR069, (char *)ARG_I, (char *)LANIF, "-p", "tcp", (char *)FW_DPORT, strPort, "-j", "REJECT", "--reject-with", "tcp-reset");
#else
		va_cmd(IP6TABLES, 10, 1, "-A", IPTABLE_IPV6_TR069, (char *)ARG_I, (char *)LANIF, "-p", "tcp", (char *)FW_DPORT, strPort, "-j", (char *)FW_DROP);
#endif
#endif

		// WAN side
#ifdef CONFIG_USER_RTK_WAN_CTYPE
		if (strlen(ifname) > 0)
		{
			// iptables -A tr069 ! -i nas0_0 -p tcp --dport 7547 -j REJECT --reject-with tcp-reset
#ifdef CONFIG_CU
			va_cmd(IPTABLES, 11, 1, "-A", IPTABLE_TR069, "!", (char *)ARG_I, ifname, "-p", "tcp", (char *)FW_DPORT, strPort, "-j", "DROP");
#else
			va_cmd(IPTABLES, 13, 1, "-A", IPTABLE_TR069, "!", (char *)ARG_I, ifname, "-p", "tcp", (char *)FW_DPORT, strPort, "-j", "REJECT", "--reject-with", "tcp-reset");
#endif

#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 10, 1, "-A", (char *)IPTABLE_IPV6_TR069, (char *)ARG_I, ifname, "-p", (char *)ARG_TCP,
				(char *)FW_DPORT, strPort, "-j", (char *)FW_ACCEPT);
#endif
		}
#ifdef CONFIG_CU
		else
		{
			va_cmd(IPTABLES, 8, 1, "-A", IPTABLE_TR069, "-p", "tcp", (char *)FW_DPORT, strPort, "-j", "DROP");
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 8, 1, "-A", (char *)IPTABLE_IPV6_TR069, "-p", (char *)ARG_TCP, (char *)FW_DPORT, strPort, "-j", "DROP");
#endif
		}
#endif
#endif
	#endif
	}
	return 0;
}

int rtk_tr069_1p_remarking(char cwmp_1p_value)
{
	unsigned int mark=0, mark_mask=0;
	char f_skb_mark_mask[32]={0}, f_skb_mark_value[32]={0};
	char rule_alias_name[256]={0}, rule_priority[32]={0};
	char devname[IFNAMSIZ]={0};
	char priority_str[32]={0};

#ifdef CONFIG_RTL_SMUX_DEV
	sprintf(devname, "%s", ALIASNAME_NAS0);
	sprintf(rule_alias_name, "cwmp_remark_8021p_us_%s", devname);
	va_cmd(SMUXCTL, 7, 1, "--if", devname, "--tx", "--tags", "1", "--rule-remove-alias", "cwmp_remark_8021p_us_+");
#endif

	if(cwmp_1p_value == -1)
		return 0;

	// calculate skb mark
	mark |= ((cwmp_1p_value) << SOCK_MARK_8021P_START) & SOCK_MARK_8021P_BIT_MASK;
	mark |= SOCK_MARK_8021P_ENABLE_TO_MARK(1);
	mark_mask = SOCK_MARK_8021P_BIT_MASK|SOCK_MARK_8021P_ENABLE_BIT_MASK;

#ifdef CONFIG_RTL_SMUX_DEV
	sprintf(f_skb_mark_mask, "0x%x", mark_mask);
	sprintf(f_skb_mark_value, "0x%x", mark);
	sprintf(rule_priority, "%d", SMUX_RULE_PRIO_CWMP);
	sprintf(priority_str, "%d", cwmp_1p_value);
	va_cmd(SMUXCTL, 16, 1, "--if", "nas0", "--tx", "--tags", "1", "--filter-skb-mark", f_skb_mark_mask, f_skb_mark_value, "--set-priority", priority_str, "1", "--rule-priority", rule_priority, "--rule-alias", rule_alias_name, "--rule-append");
#endif

	return 0;
}
#endif

#ifdef ADDRESS_MAPPING
#ifdef MULTI_ADDRESS_MAPPING
static int GetIP_AddressMap(MIB_CE_MULTI_ADDR_MAPPING_LIMIT_T *entry, ADDRESSMAP_IP_T *ip_info)
{
	unsigned char value[32];

        // Get Local Start IP
	if (((struct in_addr *)entry->lsip)->s_addr != 0)
	{
		strncpy(ip_info->lsip, inet_ntoa(*((struct in_addr *)entry->lsip)), 16);
		ip_info->lsip[15] = '\0';
	}
	else
		ip_info->lsip[0] = '\0';


	// Get Local End IP
	if (((struct in_addr *)entry->leip)->s_addr != 0)
	{
		strncpy(ip_info->leip, inet_ntoa(*((struct in_addr *)entry->leip)), 16);
		ip_info->leip[15] = '\0';
	}
	else
		ip_info->leip[0] = '\0';

	// Get Global Start IP
	if (((struct in_addr *)entry->gsip)->s_addr != 0)
	{
		strncpy(ip_info->gsip, inet_ntoa(*((struct in_addr *)entry->gsip)), 16);
		ip_info->gsip[15] = '\0';
	}
	else
		ip_info->gsip[0] = '\0';

	// Get Global End IP
	if (((struct in_addr *)entry->geip)->s_addr != 0)
	{
		strncpy(ip_info->geip, inet_ntoa(*((struct in_addr *)entry->geip)), 16);
		ip_info->geip[15] = '\0';
	}
	else
		ip_info->geip[0] = '\0';

	sprintf(ip_info->srcRange, "%s-%s", ip_info->lsip, ip_info->leip);
	sprintf(ip_info->globalRange, "%s-%s", ip_info->gsip, ip_info->geip);
//	printf( "\r\nGetIP_AddressMap %s-%s", ip_info->lsip, ip_info->leip);
//	printf( "\r\nGetIP_AddressMap %s-%s", ip_info->gsip, ip_info->geip);
	return 1;
}
#else //!MULTI_ADDRESS_MAPPING
static int GetIP_AddressMap(ADDRESSMAP_IP_T *ip_info)
{
	unsigned char value[32];

        // Get Local Start IP
        if (mib_get_s(MIB_LOCAL_START_IP, (void *)value, sizeof(value)) != 0)
	{
		if (((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(ip_info->lsip, inet_ntoa(*((struct in_addr *)value)), 16);
			ip_info->lsip[15] = '\0';
		}
		else
			ip_info->lsip[0] = '\0';
	}

	// Get Local End IP
        if (mib_get_s(MIB_LOCAL_END_IP, (void *)value, sizeof(value)) != 0)
	{
		if (((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(ip_info->leip, inet_ntoa(*((struct in_addr *)value)), 16);
			ip_info->leip[15] = '\0';
		}
		else
			ip_info->leip[0] = '\0';
	}

	// Get Global Start IP
	if (mib_get_s(MIB_GLOBAL_START_IP, (void *)value, sizeof(value)) != 0)
	{
		if (((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(ip_info->gsip, inet_ntoa(*((struct in_addr *)value)), 16);
			ip_info->gsip[15] = '\0';
		}
		else
			ip_info->gsip[0] = '\0';
	}

	// Get Global End IP
	if (mib_get_s(MIB_GLOBAL_END_IP, (void *)value, sizeof(value)) != 0)
	{
		if (((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(ip_info->geip, inet_ntoa(*((struct in_addr *)value)), 16);
			ip_info->geip[15] = '\0';
		}
		else
			ip_info->geip[0] = '\0';
	}

	sprintf(ip_info->srcRange, "%s-%s", ip_info->lsip, ip_info->leip);
	sprintf(ip_info->globalRange, "%s-%s", ip_info->gsip, ip_info->geip);

	return 1;
}
#endif //end of !MULTI_ADDRESS_MAPPING
#endif

int mask2prefix(struct in_addr mask)
{
	unsigned int i;
	unsigned int seen_one=0;
	int c=0;

	i = mask.s_addr;

	while (i > 0) {
		if (i & 1) {
			seen_one = 1;
			c++;
		} else {
			if (seen_one) {
				return -1;
			}
		}
		i >>= 1;
	}

	return c;
}

char *get_lan_ipnet()
{
	struct in_addr inIp, inMask;
	int prefix_num;
	static char src_parm[32];

	mib_get( MIB_ADSL_LAN_IP,  (void *)&inIp);
	mib_get( MIB_ADSL_LAN_SUBNET,  (void *)&inMask);
	inIp.s_addr = inIp.s_addr&inMask.s_addr;
	prefix_num = mask2prefix(inMask);
	snprintf(src_parm, 32, "%s/%d", inet_ntoa(inIp), prefix_num);
	//printf("%s: src_parm=%s\n", __FUNCTION__, src_parm);
	return src_parm;
}

// Setup one NAT rule for a specfic interface
int startAddressMap(MIB_CE_ATM_VC_Tp pEntry)
{
	char wanif[IFNAMSIZ];
	char myip[16];
#ifdef ADDRESS_MAPPING
	ADDRESSMAP_IP_T ip_info;
#ifdef MULTI_ADDRESS_MAPPING
	MIB_CE_MULTI_ADDR_MAPPING_LIMIT_T		entry;
	int		i,entryNum;


#else // ! MULTI_ADDRESS_MAPPING
	char vChar;
	ADSMAP_T mapType;

	GetIP_AddressMap(&ip_info);

	mib_get_s( MIB_ADDRESS_MAP_TYPE,  (void *)&vChar, sizeof(vChar));
        mapType = (ADSMAP_T)vChar;
#endif  //end of !MULTI_ADDRESS_MAPPING
#endif

	if ( !pEntry->enable || ((DHCP_T)pEntry->ipDhcp != DHCP_DISABLED)
		|| ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
		|| ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
		|| ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOA)
		|| (!pEntry->napt)
		)
		return -1;

	//snprintf(wanif, 6, "vc%u", VC_INDEX(pEntry->ifIndex));
	ifGetName( PHY_INTF(pEntry->ifIndex), wanif, sizeof(wanif));
	strncpy(myip, inet_ntoa(*((struct in_addr *)pEntry->ipAddr)), 16);
	myip[15] = '\0';

#ifdef ADDRESS_MAPPING
#ifdef MULTI_ADDRESS_MAPPING
	entryNum = mib_chain_total(MULTI_ADDRESS_MAPPING_LIMIT_TBL);

	for (i = 0; i<entryNum; i++)
	{
		mib_chain_get(MULTI_ADDRESS_MAPPING_LIMIT_TBL, i, &entry);
		GetIP_AddressMap(&entry, &ip_info);

		// add customized mapping
		if ( entry.addressMapType == ADSMAP_ONE_TO_ONE ) {
			va_cmd(IPTABLES, 12, 1, "-t", "nat", FW_ADD, NAT_ADDRESS_MAP_TBL, "-s", ip_info.lsip,
				ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.gsip);

		}
		else if ( entry.addressMapType == ADSMAP_MANY_TO_ONE ) {
			va_cmd(IPTABLES, 14, 1, "-t", "nat", FW_ADD, NAT_ADDRESS_MAP_TBL, "-m", "iprange", "--src-range", ip_info.srcRange,
				ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.gsip);

		}
		else if ( entry.addressMapType == ADSMAP_MANY_TO_MANY ) {
			va_cmd(IPTABLES, 14, 1, "-t", "nat", FW_ADD, NAT_ADDRESS_MAP_TBL, "-m", "iprange", "--src-range", ip_info.srcRange,
				ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.globalRange);

		}
		else if ( entry.addressMapType == ADSMAP_ONE_TO_MANY ) {
			va_cmd(IPTABLES, 12, 1, "-t", "nat", FW_ADD, NAT_ADDRESS_MAP_TBL, "-s", ip_info.lsip,
				ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.globalRange);
		}

	}
#else //!MULTI_ADDRESS_MAPPING
	// add customized mapping
	if ( mapType == ADSMAP_ONE_TO_ONE ) {
		va_cmd(IPTABLES, 12, 1, "-t", "nat", FW_ADD, NAT_ADDRESS_MAP_TBL, "-s", ip_info.lsip,
			ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.gsip);

	}
	else if ( mapType == ADSMAP_MANY_TO_ONE ) {
		va_cmd(IPTABLES, 14, 1, "-t", "nat", FW_ADD, NAT_ADDRESS_MAP_TBL, "-m", "iprange", "--src-range", ip_info.srcRange,
			ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.gsip);

	}
	else if ( mapType == ADSMAP_MANY_TO_MANY ) {
		va_cmd(IPTABLES, 14, 1, "-t", "nat", FW_ADD, NAT_ADDRESS_MAP_TBL, "-m", "iprange", "--src-range", ip_info.srcRange,
			ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.globalRange);

	}
	// Mason Yu on True
#if 1
	else if ( mapType == ADSMAP_ONE_TO_MANY ) {
		va_cmd(IPTABLES, 12, 1, "-t", "nat", FW_ADD, NAT_ADDRESS_MAP_TBL, "-s", ip_info.lsip,
			ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.globalRange);

	}
#endif
#endif //end of !MULTI_ADDRESS_MAPPING
#endif
	// add default mapping
	#ifdef CONFIG_SNAT_SRC_FILTER
	va_cmd(IPTABLES, 12, 1, "-t", "nat", FW_ADD, NAT_ADDRESS_MAP_TBL, "-s", get_lan_ipnet(),
		ARG_O, wanif, "-j", "SNAT", "--to-source", myip);
	#else
	va_cmd(IPTABLES, 10, 1, "-t", "nat", FW_ADD, NAT_ADDRESS_MAP_TBL,
		ARG_O, wanif, "-j", "SNAT", "--to-source", myip);
	#endif

	return 0;
}

// Delete one NAT rule for a specfic interface
void stopAddressMap(MIB_CE_ATM_VC_Tp pEntry)
{
	char *argv[16];
	char wanif[IFNAMSIZ];
	char myip[16];
	int idx;

#ifdef ADDRESS_MAPPING
	ADDRESSMAP_IP_T ip_info;
#ifdef MULTI_ADDRESS_MAPPING
	MIB_CE_MULTI_ADDR_MAPPING_LIMIT_T		entry;
	int		i,entryNum;


#else //MULTI_ADDRESS_MAPPING
	char vChar;
	ADSMAP_T mapType;

	GetIP_AddressMap(&ip_info);

	mib_get_s( MIB_ADDRESS_MAP_TYPE,  (void *)&vChar, sizeof(vChar));
        mapType = (ADSMAP_T)vChar;
#endif //!MULTI_ADDRESS_MAPPING
#endif

	if ( !pEntry->enable || ((DHCP_T)pEntry->ipDhcp != DHCP_DISABLED)
		|| ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
		|| ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
		|| ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOA)
		|| (!pEntry->napt)
		)
		return;

	//snprintf(wanif, 6, "vc%u", VC_INDEX(pEntry->ifIndex));
	ifGetName( PHY_INTF(pEntry->ifIndex), wanif, sizeof(wanif));
	strncpy(myip, inet_ntoa(*((struct in_addr *)pEntry->ipAddr)), 16);
	myip[15] = '\0';

	if (pEntry->cmode != CHANNEL_MODE_BRIDGE && pEntry->napt == 1)
	{	// Enable NAPT
		argv[1] = "-t";
		argv[2] = "nat";
		argv[3] = (char *)FW_DEL;
		argv[4] = (char *)NAT_ADDRESS_MAP_TBL;

		// remove default mapping
		idx = 5;
		#ifdef CONFIG_SNAT_SRC_FILTER
		argv[5] = "-s";
		argv[6] = get_lan_ipnet();
		idx = 7;
		#else
		idx = 5;
		#endif
		argv[idx++] = "-o";
		argv[idx++] = wanif;
		argv[idx++] = "-j";
		if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
			strncpy(myip, inet_ntoa(*((struct in_addr *)pEntry->ipAddr)), 16);
			myip[15] = '\0';
			argv[idx++] = "SNAT";
			argv[idx++] = "--to-source";
			argv[idx++] = myip;
			argv[idx++] = NULL;
		}
		else {
			argv[idx++] = "MASQUERADE";
			argv[idx++] = NULL;
		}
		do_cmd(IPTABLES, argv, 1);

#ifdef ADDRESS_MAPPING
#ifdef MULTI_ADDRESS_MAPPING
		entryNum = mib_chain_total(MULTI_ADDRESS_MAPPING_LIMIT_TBL);

		for (i = 0; i<entryNum; i++)
		{
			mib_chain_get(MULTI_ADDRESS_MAPPING_LIMIT_TBL, i, &entry);
			GetIP_AddressMap(&entry, &ip_info);

			// remove customized mapping
			if ( entry.addressMapType  == ADSMAP_ONE_TO_ONE ) {
				argv[5] = "-s";
				argv[6] = ip_info.lsip;
				argv[7] = "-o";
				argv[8] = wanif;
				argv[9] = "-j";
				if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
					argv[10] = "SNAT";
					argv[11] = "--to-source";
					argv[12] = ip_info.gsip;
					argv[13] = NULL;
				}
				else {
					argv[8] = "MASQUERADE";
					argv[9] = NULL;
				}

			} else if ( entry.addressMapType  == ADSMAP_MANY_TO_ONE ) {

				argv[5] = "-m";
				argv[6] = "iprange";
				argv[7] = "--src-range";
				argv[8] = ip_info.srcRange;
				argv[9] = "-o";
				argv[10] = wanif;
				argv[11] = "-j";

				if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
					argv[12] = "SNAT";
					argv[13] = "--to-source";
					argv[14] = ip_info.gsip;
					argv[15] = NULL;
				}
				else {
					argv[8] = "MASQUERADE";
					argv[9] = NULL;
				}

			} else if ( entry.addressMapType  == ADSMAP_MANY_TO_MANY ) {
				argv[5] = "-m";
				argv[6] = "iprange";
				argv[7] = "--src-range";
				argv[8] = ip_info.srcRange;
				argv[9] = "-o";
				argv[10] = wanif;
				argv[11] = "-j";
				if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
					argv[12] = "SNAT";
					argv[13] = "--to-source";
					argv[14] = ip_info.globalRange;
					argv[15] = NULL;
				}
				else {
					argv[8] = "MASQUERADE";
					argv[9] = NULL;
				}

			}

			else if ( entry.addressMapType  == ADSMAP_ONE_TO_MANY ) {
				argv[5] = "-s";
				argv[6] = ip_info.lsip;
				argv[7] = "-o";
				argv[8] = wanif;
				argv[9] = "-j";
				if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
					argv[10] = "SNAT";
					argv[11] = "--to-source";
					argv[12] = ip_info.globalRange;
					argv[13] = NULL;
				}
				else {
					argv[8] = "MASQUERADE";
					argv[9] = NULL;
				}
			}

			else
				return;
			TRACE(STA_SCRIPT, "%s %s %s %s %s ", IPTABLES, argv[1], argv[2], argv[3], argv[4]);
			TRACE(STA_SCRIPT, "%s %s %s %s\n", argv[5], argv[6], argv[7], argv[8]);
			do_cmd(IPTABLES, argv, 1);
		}
#else //!MULTI_ADDRESS_MAPPING
		// remove customized mapping
		if ( mapType == ADSMAP_ONE_TO_ONE ) {
			argv[5] = "-s";
			argv[6] = ip_info.lsip;
			argv[7] = "-o";
			argv[8] = wanif;
			argv[9] = "-j";
			if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
				argv[10] = "SNAT";
				argv[11] = "--to-source";
				argv[12] = ip_info.gsip;
				argv[13] = NULL;
			}
			else {
				argv[8] = "MASQUERADE";
				argv[9] = NULL;
			}

		} else if ( mapType == ADSMAP_MANY_TO_ONE ) {
			argv[5] = "-m";
			argv[6] = "iprange";
			argv[7] = "--src-range";
			argv[8] = ip_info.srcRange;
			argv[9] = "-o";
			argv[10] = wanif;
			argv[11] = "-j";
			if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
				argv[12] = "SNAT";
				argv[13] = "--to-source";
				argv[14] = ip_info.gsip;
				argv[15] = NULL;
			}
			else {
				argv[8] = "MASQUERADE";
				argv[9] = NULL;
			}

		} else if ( mapType == ADSMAP_MANY_TO_MANY ) {
			argv[5] = "-m";
			argv[6] = "iprange";
			argv[7] = "--src-range";
			argv[8] = ip_info.srcRange;
			argv[9] = "-o";
			argv[10] = wanif;
			argv[11] = "-j";
			if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
				argv[12] = "SNAT";
				argv[13] = "--to-source";
				argv[14] = ip_info.globalRange;
				argv[15] = NULL;
			}
			else {
				argv[8] = "MASQUERADE";
				argv[9] = NULL;
			}

		}

		// Msason Yu on True
#if 1
		else if ( mapType == ADSMAP_ONE_TO_MANY ) {
			argv[5] = "-s";
			argv[6] = ip_info.lsip;
			argv[7] = "-o";
			argv[8] = wanif;
			argv[9] = "-j";
			if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
				argv[10] = "SNAT";
				argv[11] = "--to-source";
				argv[12] = ip_info.globalRange;
				argv[13] = NULL;
			}
			else {
				argv[8] = "MASQUERADE";
				argv[9] = NULL;
			}
		}
#endif
		else
			return;
		TRACE(STA_SCRIPT, "%s %s %s %s %s ", IPTABLES, argv[1], argv[2], argv[3], argv[4]);
		TRACE(STA_SCRIPT, "%s %s %s %s\n", argv[5], argv[6], argv[7], argv[8]);
		do_cmd(IPTABLES, argv, 1);

#endif //end of !MULTI_ADDRESS_MAPPING
#endif // of ADDRESS_MAPPING
	}

}

// Config all NAT rules.
// If action= ACT_STOP, delete all NAT rules.
// If action= ACT_START, setup all NAT rules.
void config_AddressMap(int action)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char wanif[6];
	char myip[16];

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
			printf("restartAddressMap: cannot get ATM_VC_TBL(ch=%d) entry\n", i);
			return;
		}

		if (Entry.enable == 0)
			continue;
		if ( action == ACT_STOP )
			stopAddressMap(&Entry);
		else if ( action == ACT_START) {
			startAddressMap(&Entry);
		}
	}

	if ( action == ACT_START) {
#ifdef CONFIG_USER_CONNTRACK_TOOLS
		va_cmd(CONNTRACK_TOOL, 1, 1, "-F");
#else
		va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
#endif
	}

}


#ifdef REMOTE_ACCESS_CTL
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
void remote_access_modify(MIB_CE_ACC_T accEntry, int enable)
{
	char *act;
	char strPort[8];
	int ret;
	unsigned char ftp_enable = 0;
	unsigned char smba_enable = 0;

	if (enable)
		act = (char *)"-I";
	else
		act = (char *)FW_DEL;

#ifdef SUPPORT_INCOMING_FILTER
	int totalEntry = 0, num = 0;
	char errdesc[256] = {0};
	smart_func_in_coming_val addincomingentry;
	MIB_CE_IN_COMMING_T in_coming_t;

	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_IN_COMMING);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)FW_IN_COMMING);
#endif

	totalEntry = mib_chain_total(MIB_IN_COMMING_TBL);
	for (num = 0; num < totalEntry; num++)
	{
		memset(&in_coming_t, 0, sizeof(MIB_CE_IN_COMMING_T));
		if (mib_chain_get(MIB_IN_COMMING_TBL, num, (void *)&in_coming_t))
		{
			memset(&addincomingentry, 0, sizeof(smart_func_in_coming_val));
			strcpy(addincomingentry.remoteIP, in_coming_t.remoteIP);
			addincomingentry.protocol = in_coming_t.protocol;
			addincomingentry.port = in_coming_t.port;
			addincomingentry.interface = in_coming_t.interface;
			ret = smart_func_add_in_coming_api(IN_COMING_API_ADD, &addincomingentry, errdesc);

			if (ret != IN_COMING_RET_SUCCESSFUL) {
				printf("smart_func_add_in_coming_api fail: %s\n", errdesc);
			}
		}
	}
#endif

    syslog(LOG_ERR, "iptables:in %s(%d)\r\n", __FUNCTION__, __LINE__);

	if(enable){
		//FTP
#ifdef FTP_SERVER_INTERGRATION
		mib_get_s(MIB_FTP_ENABLE, (void *)&ftp_enable, sizeof(ftp_enable));
#else
	    mib_get_s(MIB_VSFTP_ENABLE, (void *)&ftp_enable, sizeof(ftp_enable));
#endif
		/*0:ftpserver disable;1:local enbale,remote disable;2:local disable,remote enable;3:local enbale,remote enbale*/
		setFtpAccessFw(ftp_enable);

		//SAMBA
#ifdef CONFIG_USER_SAMBA
		mib_get_s(MIB_SAMBA_ENABLE, (void *)&smba_enable, sizeof(smba_enable));
		/*0:sambaserver disable;1:local enbale,remote disable;2:local disable,remote enable;3:local enbale,remote enbale*/
		setSambaAccessFw(smba_enable);
#endif

		//TELNET
		/*0:telnetserver disable;1:local enbale,remote disable;2:local disable,remote enbale;3:local enbale,remote enbale*/
		setTelnetAccessFw(accEntry.telnet);
	}
	else{
		delFtpAccessFw();

#ifdef CONFIG_USER_SAMBA
		delSambaAccessFw();
#endif

		delTelnetAccessFw();
	}
#if 0
	// telnet service: bring up by inetd
	#ifdef CONFIG_USER_TELNETD_TELNETD
	if (!(accEntry.telnet & 0x02)) {	// not LAN access
		// iptables -A inacc -i $LAN_IF -p TCP --dport 23 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "23", "-j", (char *)FW_DROP);

	#ifdef CONFIG_IPV6
		//ip6tables -I ipv6remoteacc -i br0 -p TCP --dport 23 -j DROP
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_INACC, (char *)ARG_I, (char *)LANIF
				, "-p", (char *)ARG_TCP, (char *)FW_DPORT, "23", "-j", (char *)FW_DROP);
	#endif
	}
	if (accEntry.telnet & 0x01) {	// can WAN access
		snprintf(strPort, sizeof(strPort)-1, "%u", accEntry.telnet_port);
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act, (char *)FW_PREROUTING,
		 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);

		// redirect if this is not standard port
		if (accEntry.telnet_port != 23) {
			ret = va_cmd(IPTABLES, 15, 1, "-t", "nat",
					(char *)act, (char *)FW_PREROUTING,
					 "!", (char *)ARG_I, (char *)LANIF,
					"-p", ARG_TCP,
					(char *)FW_DPORT, strPort, "-j",
					"REDIRECT", "--to-ports", "23");
		}

		// iptables -A inacc -i ! $LAN_IF -p TCP --dport 23 -j ACCEPT
		#if 0
		va_cmd(IPTABLES, 11, 1, act, (char *)FW_INACC,
		 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "23", "-j", (char *)FW_ACCEPT);

		fprintf(stderr, "telnet = %d\n", accEntry.telnet_port);
		if (accEntry.telnet_port != 23) {
			ret = va_cmd(IPTABLES, 15, 1, "-t", "nat",
					(char *)act, (char *)FW_PREROUTING,
					 "!", (char *)ARG_I, (char *)LANIF,
					"-p", ARG_TCP,
					(char *)FW_DPORT, strPort, "-j",
					"REDIRECT", "--to-ports", "23");
			fprintf(stderr, "telnet cmd = %d\n", ret);
		}
		#endif

	#ifdef CONFIG_IPV6
		//block IPv6 WAN access
		//FIXME: There's no redirect rule support for IPv6, we can only block standard port.

		//ip6tables -I ipv6remoteacc -i ! br0 -p TCP --dport 23 -j ACCEPT
		va_cmd(IP6TABLES, 11, 1, act, (char *)FW_INACC, (char *)ARG_I, "!", (char *)LANIF
				, "-p", (char *)ARG_TCP, (char *)FW_DPORT, "23", "-j", (char *)FW_ACCEPT);
	#endif

	}
	#endif	//CONFIG_USER_TELNETD_TELNETD
    #endif


	#ifdef CONFIG_USER_FTPD_FTPD
	if (!(accEntry.ftp & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
		// iptables -A inacc -i $LAN_IF -p TCP --dport 21 -j DROP
	//	va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
	//	(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
	//	(char *)FW_DPORT, "21", "-j", (char *)FW_DROP);

	#ifdef CONFIG_IPV6
		//ip6tables -I ipv6remoteacc -i br0 -p TCP --dport 21 -j DROP
	//	va_cmd(IP6TABLES, 10, 1, act, (char *)FW_INACC, (char *)ARG_I, (char *)LANIF
	//			, "-p", (char *)ARG_TCP, (char *)FW_DPORT, "21", "-j", (char *)FW_DROP);
	#endif
	}
	if (accEntry.ftp & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
		snprintf(strPort, sizeof(strPort)-1, "%u", accEntry.ftp_port);
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act, (char *)FW_PREROUTING,
		 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);

		// redirect if this is not standard port
		if (accEntry.ftp_port != 21) {
			ret = va_cmd(IPTABLES, 15, 1, "-t", "nat",
					(char *)act, (char *)FW_PREROUTING,
					 "!", (char *)ARG_I, (char *)LANIF,
					"-p", ARG_TCP,
					(char *)FW_DPORT, strPort, "-j",
					"REDIRECT", "--to-ports", "21");
		}

		#if 0
		// iptables -A inacc -i ! $LAN_IF -p TCP --dport 21 -j ACCEPT
		va_cmd(IPTABLES, 11, 1, act, (char *)FW_INACC,
		 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "21", "-j", (char *)FW_ACCEPT);
		#endif
#ifdef CONFIG_IPV6
		//block IPv6 WAN access
		//FIXME: There's no redirect rule support for IPv6, we can only block standard port.

		//ip6tables -A ipv6remoteacc -i ! br0 -p TCP --dport 21 -j ACCEPT
		va_cmd(IP6TABLES, 11, 1, act, (char *)FW_INACC, (char *)ARG_I, "!", (char *)LANIF
				, "-p", (char *)ARG_TCP, (char *)FW_DPORT, "21", "-j", (char *)FW_ACCEPT);
#endif

	}
	#endif	//CONFIG_USER_FTPD_FTPD

	#ifdef CONFIG_USER_TFTPD_TFTPD
	// tftp service: bring up by inetd
	if (!(accEntry.tftp & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
#ifdef CONFIG_CU
				// iptables -A inacc -i $LAN_IF -p UDP --dport 69 -j REJECT --reject-with icmp-port-unreachable
				va_cmd(IPTABLES, 12, 1, act, (char *)FW_INACC,
				(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
				(char *)FW_DPORT, "69", "-j", "REJECT", "--reject-with", "icmp-port-unreachable");
#ifdef CONFIG_IPV6
				va_cmd(IP6TABLES, 12, 1, act, (char *)FW_INACC,
				(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
				(char *)FW_DPORT, "69", "-j", "REJECT", "--reject-with", "icmp6-port-unreachable");
#endif
#else
		// iptables -A inacc -i $LAN_IF -p UDP --dport 69 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "69", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_INACC,
		(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "69", "-j", (char *)FW_DROP);
#endif
#endif
	}
	if (accEntry.tftp & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
		// iptables -A inacc -i ! $LAN_IF -p UDP --dport 69 -j ACCEPT
		va_cmd(IPTABLES, 11, 1, act, (char *)FW_INACC,
		 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "69", "-j", (char *)FW_ACCEPT);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 11, 1, act, (char *)FW_INACC,
		 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "69", "-j", (char *)FW_ACCEPT);
#endif
	}
	#endif

	// HTTP service
		if (!(accEntry.web & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
			// iptables -A inacc -i $LAN_IF -p TCP --dport 80 -j DROP
			va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
			(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, "80", "-j", (char *)FW_DROP);
			va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
			(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, "8080", "-j", (char *)FW_DROP);
		}
		if (accEntry.web & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
			snprintf(strPort, sizeof(strPort)-1, "%u", accEntry.web_port);
			// iptables -A inacc -i ! $LAN_IF -p TCP --dport 80 -j ACCEPT
			va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act, (char *)FW_PREROUTING,
				 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
				(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
			#if 0
			va_cmd(IPTABLES, 11, 1, act, (char *)FW_INACC,
			 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, "80", "-j", (char *)FW_ACCEPT);
			#endif
			if (accEntry.web_port != 80) {
				va_cmd(IPTABLES, 15, 1, "-t", "nat",
					(char *)act, (char *)FW_PREROUTING,
					 "!", (char *)ARG_I, (char *)LANIF,
					"-p", ARG_TCP,
					(char *)FW_DPORT, strPort, "-j",
					"REDIRECT", "--to-ports", "80");
			}
			snprintf(strPort, sizeof(strPort)-1, "8080");
			// iptables -A inacc -i ! $LAN_IF -p TCP --dport 80 -j ACCEPT
			va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act, (char *)FW_PREROUTING,
				 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
				(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
		}

  #ifdef CONFIG_USER_BOA_WITH_SSL
  	  //HTTPS service
		if (!(accEntry.https & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
			// iptables -A inacc -i $LAN_IF -p TCP --dport 443 -j DROP
			va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
			(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, "443", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 10, 1, act, (char *)FW_INACC,
			(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, "443", "-j", (char *)FW_DROP);
#endif
		}
		if (accEntry.https & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
			snprintf(strPort, sizeof(strPort)-1, "%u", accEntry.https_port);
			// iptables -t mangle  -I PREROUTING -i !$LAN_IF -p TCP --dport 443 -j MARK --set-mark 0x1000
			va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act, (char *)FW_PREROUTING,
				 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
				(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);

			if (accEntry.https_port != 443) {
				va_cmd(IPTABLES, 15, 1, "-t", "nat",
					(char *)act, (char *)FW_PREROUTING,
					 "!", (char *)ARG_I, (char *)LANIF,
					"-p", ARG_TCP,
					(char *)FW_DPORT, strPort, "-j",
					"REDIRECT", "--to-ports", "443");
			}

		}
  #endif

	#ifdef CONFIG_USER_SNMPD_SNMPD_V2CTRAP
	// snmp service
	//if (accEntry.snmp !=0) {	// have snmp server
		if (!(accEntry.snmp & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
			// iptables -A inacc -i $LAN_IF -p UDP --dport 161:162 -j DROP
			va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
			(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
			(char *)FW_DPORT, "161:162", "-j", (char *)FW_DROP);
		}
		if (accEntry.snmp & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
			// iptables -A inacc -i ! $LAN_IF -p UDP --dport 161:162 -m limit
			//  --limit 100/s --limit-burst 500 -j ACCEPT
			va_cmd(IPTABLES, 17, 1, act, (char *)FW_INACC,
				 "!", (char *)ARG_I, (char *)LANIF, "-p",
				(char *)ARG_UDP, (char *)FW_DPORT, "161:162", "-m",
				"limit", "--limit", "100/s", "--limit-burst",
				"500", "-j", (char *)FW_ACCEPT);
		}
	#endif

	#ifdef CONFIG_USER_SSH_DROPBEAR
	// ssh service
		if (!(accEntry.ssh & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
#ifdef CONFIG_CU
			// iptables -I inacc -i $LAN_IF -p TCP --dport 22 -j REJECT --reject-with tcp-reset
			va_cmd(IPTABLES, 12, 1, act, (char *)FW_INACC,
			(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, "22", "-j", "REJECT", "--reject-with", "tcp-reset");
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 12, 1, act, (char *)FW_INACC,
			(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, "22", "-j", "REJECT", "--reject-with", "tcp-reset");
#endif
#else
			// iptables -A inacc -i $LAN_IF -p TCP --dport 22 -j DROP
			va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
			(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, "22", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			//nessus scan ssh port in ipv6
			va_cmd(IP6TABLES, 10, 1, act, (char *)FW_INACC,
			(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, "22", "-j", (char *)FW_DROP);
#endif
#endif
		}
		if (accEntry.ssh & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
			// iptables -A inacc -i ! $LAN_IF -p TCP --dport 22 -j ACCEPT
			va_cmd(IPTABLES, 11, 1, act, (char *)FW_INACC,
			 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, "22", "-j", (char *)FW_ACCEPT);
			//nessus scan ssh port in ipv6
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 11, 1, act, (char *)FW_INACC,
			 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, "22", "-j", (char *)FW_ACCEPT);
#endif
		}
	#endif

#ifdef CONFIG_CU_BASEON_YUEME
	//icmp service
	if (accEntry.icmp & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) // can WAN access
	{
		va_cmd(IPTABLES, 15, 1, act, (char *)FW_ICMP_FILTER,
			"!", (char *)ARG_I, (char *)LANIF,
			"-p", "icmp", "--icmp-type", "echo-request",
			"-m", "limit", "--limit", "20/s",
			"-j", (char *)FW_ACCEPT);

#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 15, 1, act, (char *)FW_ICMP_FILTER,
			"!", (char *)ARG_I, (char *)LANIF,
			"-p", "icmpv6", "--icmpv6-type", "echo-request",
			"-m", "limit","--limit", "20/s",
			"-j", (char *)FW_ACCEPT);
#endif
	}

	if (!(accEntry.icmp & ACL_ENTRY_WAN_SIDE_PERMISSION_SET))
	{
		va_cmd(IPTABLES, 11, 1, act, (char *)FW_ICMP_FILTER,
			"!", (char *)ARG_I, (char *)LANIF,
			"-p", "icmp", "--icmp-type", "echo-request",
			"-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 11, 1, act, (char *)FW_ICMP_FILTER,
			"!", (char *)ARG_I, (char *)LANIF,
			"-p", "icmpv6", "--icmpv6-type", "echo-request",
			"-j", (char *)FW_DROP);
#endif
	}
#endif

	#ifdef CONFIG_USER_NETLOGGER_SUPPORT
	if (accEntry.netlog & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
		snprintf(strPort, sizeof(strPort)-1, "%u", 0x1234);
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act, (char *)FW_PREROUTING,
		 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
	}
	#endif //CONFIG_USER_NETLOGGER_SUPPORT

#if 0
#ifdef CONFIG_USER_SAMBA
	if (!(accEntry.smb & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
		// iptables -A inacc -i $LAN_IF -p TCP --dport 139 -j DROP
//		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
//			(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
//			(char *)FW_DPORT, "139", "-j", (char *)FW_DROP);
		// iptables -A inacc -i $LAN_IF -p TCP --dport 445 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
			(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, "445", "-j", (char *)FW_DROP);
		// iptables -A inacc -i $LAN_IF -p UDP --dport 137 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
			(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
			(char *)FW_DPORT, "137", "-j", (char *)FW_DROP);
		// iptables -A inacc -i $LAN_IF -p UDP --dport 138 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
			(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
			(char *)FW_DPORT, "138", "-j", (char *)FW_DROP);
	}
	if (accEntry.smb & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
		// iptables -A inacc -i ! $LAN_IF -p TCP --dport 139 -j ACCEPT
//		va_cmd(IPTABLES, 11, 1, act, (char *)FW_INACC,
//		 	"!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
//			(char *)FW_DPORT, "139", "-j", (char *)FW_ACCEPT);
		// iptables -A inacc -i ! $LAN_IF -p TCP --dport 445 -j ACCEPT
		va_cmd(IPTABLES, 11, 1, act, (char *)FW_INACC,
			"!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, "445", "-j", (char *)FW_ACCEPT);
		// iptables -A inacc -i ! $LAN_IF -p UDP --dport 137 -j ACCEPT
		va_cmd(IPTABLES, 11, 1, act, (char *)FW_INACC,
			"!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
			(char *)FW_DPORT, "137", "-j", (char *)FW_ACCEPT);
		// iptables -A inacc -i ! $LAN_IF -p UDP --dport 138 -j ACCEPT
		va_cmd(IPTABLES, 11, 1, act, (char *)FW_INACC,
			"!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
			(char *)FW_DPORT, "138", "-j", (char *)FW_ACCEPT);
	}
#endif
#endif

	/* For NESSUS 11612 */
#ifndef CONFIG_CU
	va_cmd(IPTABLES, 10, 1, "-I", (char *)FW_INACC, "-i", "br0", "-p", "udp", "--dport", "4011", "-j", FW_DROP);
#endif

#ifdef CONFIG_CT_AWIFI_JITUAN_SMARTWIFI

	// 1.1 10.1 no pass
	//iptables -I inacc -i br0 -p tcp -s 192.168.10.0/24 -d 192.168.1.0/24 --dport 8080 -j DROP
	//iptables -I inacc -i br0 -p tcp -s 192.168.10.0/24 -d 192.168.1.0/24 --dport 80 -j DROP

	struct in_addr lanip, lanmask, lannet;
	char ipaddr[16],subnet[16];
	char ipaddr2[16],subnet2[16];
	int maskbit=0,maskbit2=0,i=0;
	char lan_ip_buf[64] = {0};
	char lan_ip2_buf[64] = {0};

	mib_get_s(MIB_ADSL_LAN_IP2, (void *)&lanip, sizeof(lanip));
	mib_get_s(MIB_ADSL_LAN_SUBNET2, (void *)&lanmask, sizeof(lanmask));
	lannet.s_addr = lanip.s_addr & lanmask.s_addr;
	strncpy(ipaddr2, inet_ntoa(lannet), 16);
	ipaddr2[15] = '\0';

	for(i=0;i<32;i++)
	{
		if(lanmask.s_addr&(0x80000000>>i))
			maskbit2++;
	}
	sprintf(lan_ip2_buf, "%s/%d", ipaddr2,maskbit2);

	mib_get_s(MIB_ADSL_LAN_IP, (void *)&lanip, sizeof(lanip));
	mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)&lanmask, sizeof(lanmask));
	lannet.s_addr = lanip.s_addr & lanmask.s_addr;
	strncpy(ipaddr, inet_ntoa(lannet), 16);
	ipaddr[15] = '\0';

	for(i=0;i<32;i++)
	{
		if(lanmask.s_addr&(0x80000000>>i))
			maskbit++;
	}

	sprintf(lan_ip_buf, "%s/%d", ipaddr,maskbit);

	va_cmd(IPTABLES, 14, 1 ,act, (char *)FW_INACC,
		(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		"-s",lan_ip2_buf, "-d",lan_ip_buf,
		(char *)FW_DPORT, "80", "-j", (char *)FW_DROP);
	va_cmd(IPTABLES, 14, 1 ,act, (char *)FW_INACC,
		(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		"-s",lan_ip2_buf, "-d",lan_ip_buf,
		(char *)FW_DPORT, "8080", "-j", (char *)FW_DROP);

#endif
}
#else
void remote_access_modify(MIB_CE_ACC_T accEntry, int enable)
{
	char *act;
	char strPort[16];
	int ret;
#ifdef _CWMP_MIB_
	unsigned int cwmp_connreq_port = 0;
	unsigned char cwmp_flag = 0;
#endif

	if (enable)
		act = (char *)FW_INSERT;
	else
		act = (char *)FW_DEL;

#ifdef _CWMP_MIB_
	mib_get_s(CWMP_CONREQ_PORT, &cwmp_connreq_port, sizeof(cwmp_connreq_port));

	if (cwmp_connreq_port) {
		mib_get_s(CWMP_FLAG, &cwmp_flag, sizeof(cwmp_flag));

		// can WAN access
		if (cwmp_flag & CWMP_FLAG_AUTORUN) {
			snprintf(strPort, sizeof(strPort), "%u",
				 cwmp_connreq_port);
#ifndef CONFIG_TELMEX_DEV
			va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
			       (char *)FW_PREROUTING, "!", (char *)ARG_I,
			       (char *)LANIF, "-p", (char *)ARG_TCP,
			       (char *)FW_DPORT, strPort, "-j", (char *)"MARK",
			       "--set-mark", RMACC_MARK);
#endif
		}
	}
#endif

	// telnet service: bring up by inetd
#if defined(CONFIG_USER_TELNETD_TELNETD) || defined(CONFIG_CU)
	if (!(accEntry.telnet & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
#ifdef CONFIG_CU
		// iptables -A inacc -i $LAN_IF -p TCP --dport 23 -j DROP
		va_cmd(IPTABLES, 12, 1, act, (char *)FW_INACC,
			(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, "23", "-j", "REJECT", "--reject-with", "tcp-reset");

#ifdef CONFIG_IPV6
		//ip6tables -I ipv6remoteacc -i br0 -p TCP --dport 23 -j DROP
		va_cmd(IP6TABLES, 12, 1, act, (char *)FW_INACC, (char *)ARG_I, (char *)LANIF
			, "-p", (char *)ARG_TCP, (char *)FW_DPORT, "23", "-j", "REJECT", "--reject-with", "tcp-reset");
#endif
#else
		// iptables -I inacc -i $LAN_IF -p TCP --dport 23 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		       (char *)FW_DPORT, "23", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
		//ip6tables -I ipv6remoteacc -i br0 -p TCP --dport 23 -j DROP
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_INACC, (char *)ARG_I, (char *)LANIF
				, "-p", (char *)ARG_TCP, (char *)FW_DPORT, "23", "-j", (char *)FW_DROP);
#endif
#endif
	}
	if (accEntry.telnet & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
		snprintf(strPort, sizeof(strPort), "%u", accEntry.telnet_port);
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_TCP, (char *)FW_DPORT, strPort, "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);

		// redirect if this is not standard port
		if (accEntry.telnet_port != 23) {
			ret = va_cmd(IPTABLES, 15, 1, "-t", "nat",
				     (char *)act, (char *)FW_PREROUTING,
				     "!", (char *)ARG_I, (char *)LANIF,
				     "-p", ARG_TCP,
				     (char *)FW_DPORT, strPort, "-j",
				     "REDIRECT", "--to-ports", "23");
		}
	}
#endif

#ifdef CONFIG_USER_FTPD_FTPD
	// ftp service: bring up by inetd
	if (!(accEntry.ftp & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
		// iptables -I inacc -i $LAN_IF -p TCP --dport 21 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		       (char *)FW_DPORT, "21", "-j", (char *)FW_DROP);
	}
	if (accEntry.ftp & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
		snprintf(strPort, sizeof(strPort), "%u", accEntry.ftp_port);
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_TCP, (char *)FW_DPORT, strPort, "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);

		// redirect if this is not standard port
		if (accEntry.ftp_port != 21) {
			ret = va_cmd(IPTABLES, 15, 1, "-t", "nat",
				     (char *)act, (char *)FW_PREROUTING,
				     "!", (char *)ARG_I, (char *)LANIF,
				     "-p", ARG_TCP,
				     (char *)FW_DPORT, strPort, "-j",
				     "REDIRECT", "--to-ports", "21");
		}
	}
#endif

#ifdef CONFIG_USER_SAMBA
	// samba service: bring up by inetd
	if (!(accEntry.smb & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
		// iptables -I inacc -i $LAN_IF -p TCP --dport 445 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		       (char *)FW_DPORT, "445", "-j", (char *)FW_DROP);
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		       (char *)FW_DPORT, "139", "-j", (char *)FW_DROP);
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		       (char *)FW_DPORT, "137", "-j", (char *)FW_DROP);
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		       (char *)FW_DPORT, "138", "-j", (char *)FW_DROP);
	}
	if (accEntry.smb & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_TCP, (char *)FW_DPORT, "445", "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_TCP, (char *)FW_DPORT, "139", "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_UDP, (char *)FW_DPORT, "137", "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_UDP, (char *)FW_DPORT, "138", "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);
	}
#endif

#ifdef CONFIG_USER_TFTPD_TFTPD
	// tftp service: bring up by inetd
	if (!(accEntry.tftp & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
		// iptables -I inacc -i $LAN_IF -p UDP --dport 69 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		       (char *)FW_DPORT, "69", "-j", (char *)FW_DROP);
	}
	if (accEntry.tftp & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_UDP, (char *)FW_DPORT, "69", "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);
	}
#endif

	// HTTP service
	if (!(accEntry.web & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
		// iptables -I inacc -i $LAN_IF -p TCP --dport 80 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		       (char *)FW_DPORT, "80", "-j", (char *)FW_DROP);
	}
	if (accEntry.web & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
		snprintf(strPort, sizeof(strPort), "%u", accEntry.web_port);
		// iptables -t mangle -I PREROUTING ! -i $LAN_IF -p TCP --dport 80 -j MARK --set-mark 0x1000
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_TCP, (char *)FW_DPORT, strPort, "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);

		if (accEntry.web_port != 80) {
			va_cmd(IPTABLES, 15, 1, "-t", "nat",
			       (char *)act, (char *)FW_PREROUTING,
			       "!", (char *)ARG_I, (char *)LANIF,
			       "-p", ARG_TCP,
			       (char *)FW_DPORT, strPort, "-j",
			       "REDIRECT", "--to-ports", "80");
		}

	}
#ifdef CONFIG_USER_BOA_WITH_SSL
	//HTTPS service
	if (!(accEntry.https & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
		// iptables -I inacc -i $LAN_IF -p TCP --dport 443 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		       (char *)FW_DPORT, "443", "-j", (char *)FW_DROP);
	}
	if (accEntry.https & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
		snprintf(strPort, sizeof(strPort), "%u", accEntry.https_port);
		// iptables -t mangle -I PREROUTING ! -i $LAN_IF -p TCP --dport 443 -j MARK --set-mark 0x1000
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_TCP, (char *)FW_DPORT, strPort, "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);

		if (accEntry.https_port != 443) {
			va_cmd(IPTABLES, 15, 1, "-t", "nat",
			       (char *)act, (char *)FW_PREROUTING,
			       "!", (char *)ARG_I, (char *)LANIF,
			       "-p", ARG_TCP,
			       (char *)FW_DPORT, strPort, "-j",
			       "REDIRECT", "--to-ports", "443");
		}

	}
#endif

#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_NETSNMP_SNMPD)
	// snmp service
	if (!(accEntry.snmp & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
		// iptables -I inacc -i $LAN_IF -p UDP --dport 161:162 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		       (char *)FW_DPORT, "161:162", "-j", (char *)FW_DROP);
	}
	if (accEntry.snmp & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
		va_cmd(IPTABLES, 21, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_UDP, (char *)FW_DPORT, "161:162", "-m",
		       "limit", "--limit", "100/s", "--limit-burst", "500",
		       "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
	}
#endif

#if defined(CONFIG_USER_SSH_DROPBEAR)||defined(CONFIG_USER_SSH_DROPBEAR_2016_73)
	// ssh service
	if (!(accEntry.ssh & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) {	// not LAN access
#ifdef CONFIG_CU
		// iptables -I inacc -i $LAN_IF -p TCP --dport 22 -j REJECT --reject-with tcp-reset
		va_cmd(IPTABLES, 12, 1, act, (char *)FW_INACC,
				(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
				(char *)FW_DPORT, "22", "-j", "REJECT", "--reject-with", "tcp-reset");
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 12, 1, act, (char *)FW_INACC,
				(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
				(char *)FW_DPORT, "22", "-j", "REJECT", "--reject-with", "tcp-reset");
#endif
#else
		// iptables -I inacc -i $LAN_IF -p TCP --dport 22 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		       (char *)FW_DPORT, "22", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
		//nessus scan ssh port in ipv6
		va_cmd(IP6TABLES, 10, 1, act, (char *)FW_INACC,
				(char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
				(char *)FW_DPORT, "22", "-j", (char *)FW_DROP);
#endif
#endif
	}
	if (accEntry.ssh & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
		snprintf(strPort, sizeof(strPort), "%u", accEntry.ssh_port);
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_TCP, (char *)FW_DPORT, strPort, "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);

		if (accEntry.ssh_port != 22) {
			va_cmd(IPTABLES, 15, 1, "-t", "nat",
			       (char *)act, (char *)FW_PREROUTING,
			       "!", (char *)ARG_I, (char *)LANIF,
			       "-p", ARG_TCP,
			       (char *)FW_DPORT, strPort, "-j",
			       "REDIRECT", "--to-ports", "22");
		}
	}
#endif

	// ping service
	if (!(accEntry.icmp & ACL_ENTRY_LAN_SIDE_PERMISSION_SET))	// not LAN access
	{
		// iptables -I INPUT -i $LAN_IF -p ICMP --icmp-type echo-request -j ACCEPT
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", "ICMP",
		       "--icmp-type", "echo-request", "-j", (char *)FW_DROP);
	}
	if (accEntry.icmp & ACL_ENTRY_WAN_SIDE_PERMISSION_SET)	// can WAN access
	{
		va_cmd(IPTABLES, 19, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_ICMP, "--icmp-type", "echo-request",
		       "-m", "limit", "--limit", "1/s", "-j", (char *)"MARK",
		       "--set-mark", RMACC_MARK);
	}

#ifdef CONFIG_USER_NETLOGGER_SUPPORT
	// can WAN access
	snprintf(strPort, sizeof(strPort), "%u", 0x1234);
	va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act, (char *)FW_PREROUTING,
	       "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
	       (char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark",
	       RMACC_MARK);
#endif //CONFIG_USER_NETLOGGER_SUPPORT
}
#endif
void filter_set_remote_access(int enable)
{
	MIB_CE_ACC_T accEntry;
	if (!mib_chain_get(MIB_ACC_TBL, 0, (void *)&accEntry))
		return;
	remote_access_modify( accEntry, enable );

	return;

}
#endif // of REMOTE_ACCESS_CTL

#ifdef USE_DEONET /* sghong, 20240429 */
void deo_filter_acl_flush(void)
{
	int i, total;
	struct in_addr src;
	char ssrc[40], policy[32], policy_end[32];
	MIB_CE_ACL_IP_T Entry;

#ifdef ACL_IP_RANGE
	struct in_addr start,end;
	char sstart[16],send[16];
#endif
	
	total = mib_chain_total(MIB_ACL_IP_TBL);
	for (i=0; i<total; i++) {
		if (!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&Entry))
		{
			return;
		}

		// Check if this entry is enabled
		if ( Entry.Enabled == 1 ) {

			strcpy(policy, FW_RETURN);

			start.s_addr = *(unsigned long*)Entry.startipAddr;
			end.s_addr = *(unsigned long*)Entry.endipAddr;
			if (0 == start.s_addr)
			{
				sstart[0] = '\0';
				ssrc[0] = '\0';
			}
			else
			{
				strcpy(sstart, inet_ntoa(start));
				strcpy(send, inet_ntoa(end));
				strcpy(ssrc,sstart);
				strcat(ssrc,"-");
				strcat(ssrc,send);
			}

			{
				// iptables -A INPUT -s xxx.xxx.xxx.xxx
				//va_cmd(IPTABLES, 9, 1, (char *)FW_ADD, "aclblock", "!", "-i", BRIF, "-s", ssrc, "-j", (char *)FW_RETURN);
				if(*(unsigned long*)Entry.startipAddr != *(unsigned long*)Entry.endipAddr) {
					deo_filter_set_acl_service(Entry, Entry.Interface, 0, ssrc, 1);
				}
				else {
					deo_filter_set_acl_service(Entry, Entry.Interface, 0, sstart, 0);
				}

			}
		}
	}
}
#endif

#ifdef IP_ACL
static int filter_acl_flush()
{
#ifdef CONFIG_DEONET_DOS // Deonet GNT2400R
	// iptables -F droplist
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_DROPLIST);
#endif
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_ACL);
	// Flush all rule in nat table
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)FW_ACL);
	// Flush all rule in nat table
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)FW_ACL);

	return 0;
}

/*
 * In general, we don't allow device access from wan except those specified in this function.
 */
static int filter_acl_allow_wan()
{
	unsigned char dhcpMode;
	unsigned char vChar;
#if defined(IP_ACL)
	int i, entryNum;
	unsigned char whiteBlackRule;
	MIB_CE_ACL_IP_T Entry;
#endif
#ifdef CONFIG_DEONET_DOS // Deonet GNT2400R
	unsigned int dos_enable = 0;
	if (mib_get_s(MIB_DOS_ENABLED,  (void *)&dos_enable, sizeof(dos_enable))) {
		if (dos_enable & DOS_ENABLE) {
			if (dos_enable & TCPUDPPORTSCAN) {
				va_cmd(IPTABLES, 9, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-p", (char *)ARG_TCP, "--tcp-flags", "RST", "RST", "-j", (char *)FW_DROP);
				va_cmd(IPTABLES, 11, 1, (char *)FW_INSERT, (char *)FW_DROPLIST, (char *)ARG_I, "nas0_0", "-p", (char *)ARG_TCP, "--syn", "--dport", ":1024", "-j", (char *)FW_DROP);
			}
		}
	}
#endif

	// iptables -A aclblock -i lo -j RETURN
	// Local Out DNS query will refer the /etc/resolv.conf(DNS server is 127.0.0.1). We should accept this DNS query when enable ACL.
	// 20220525, do not filter any service from intf lo
	va_cmd(IPTABLES, 6, 1, (char *)FW_INSERT, (char *)FW_ACL, "-i", "lo", "-j", (char *)FW_RETURN);

	// allow for WAN
	// (1) allow service with the same mark value(0x1000) from WAN
	va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_ACL,
		"!", (char *)ARG_I, LANIF_ALL, "-m", "mark", "--mark", RMACC_MARK, "-j", FW_ACCEPT);

	// (2) Added by Mason Yu for dhcp Relay. Open DHCP Relay Port for Incoming Packets.
#ifdef CONFIG_USER_DHCP_SERVER
	if (mib_get_s(MIB_DHCP_MODE, &dhcpMode, sizeof(dhcpMode)) != 0)
	{
		if (dhcpMode == 1 || dhcpMode == 2 ){
			// iptables -A aclblock -i ! br0 -p udp --dport 67 -j ACCEPT
			va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_ACL, "!", (char *)ARG_I, LANIF_ALL, "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_ACCEPT);
		}
	}
#endif

#ifdef CONFIG_USER_ROUTED_ROUTED
	// (3) Added by Mason Yu. Open RIP Port for Incoming Packets.
	if (mib_get_s( MIB_RIP_ENABLE, (void *)&vChar, sizeof(vChar)) != 0)
	{
		if (1 == vChar)
		{
			// iptables -A aclblock -i ! br0 -p udp --dport 520 -j ACCEPT
			va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_ACL, "!", "-i", LANIF_ALL, "-p", "udp", (char *)FW_DPORT, "520", "-j", (char *)FW_ACCEPT);
		}
	}
#endif
#if defined(IP_ACL)
		mib_get_s(MIB_ACL_CAPABILITY_TYPE, (void *)&whiteBlackRule, sizeof(whiteBlackRule));
		entryNum = mib_chain_total(MIB_ACL_IP_TBL);
		for (i=0; i<entryNum; i++) {
			if (!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&Entry))
			{
				printf("[%s %d]mib_chain_get failed\n", __func__, __LINE__);
				return -1;
			}

			if (Entry.Interface == IF_DOMAIN_LAN)
				continue;

			if(whiteBlackRule==ACL_TYPE_BLACK_LIST)
			{
				if(Entry.icmp==0)
					break;
			}
			else
			{
				if(Entry.icmp==0x1)
					break;
			}
			// iptables -A aclblock -i ! $LAN_IF   -p ICMP --icmp-type echo-request -m limit
			//	 --limit 1/s -j DROP
			va_cmd(IPTABLES, 15, 1, (char *)FW_ADD, (char *)FW_ACL,
				"!", (char *)ARG_I, (char *)LANIF, "-p", "ICMP",
				"--icmp-type", "echo-request", "-m", "limit",
				"--limit", "1/s", "-j", (char *)FW_DROP);
		}
#else

	// iptables -A aclblock -i ! $LAN_IF   -p ICMP --icmp-type echo-request -m limit
	//   --limit 1/s -j DROP
#ifndef RTK_MULTI_AP_WFA
	va_cmd(IPTABLES, 15, 1, (char *)FW_ADD, (char *)FW_ACL,
		"!", (char *)ARG_I, (char *)LANIF_ALL, "-p", "ICMP",
		"--icmp-type", "echo-request", "-m", "limit",
		"--limit", "1/s", "-j", (char *)FW_DROP);
#endif
#endif

	// accept related
	// iptables -A aclblock -m state --state ESTABLISHED,RELATED -j ACCEPT
#if defined(CONFIG_00R0)
	char ipfilter_spi_state=1;
	mib_get_s(MIB_IPFILTER_SPI_ENABLE, (void*)&ipfilter_spi_state, sizeof(ipfilter_spi_state));
	if(ipfilter_spi_state != 0)
#endif

	return 0;
}

#ifdef CONFIG_00R0
static int set_acl_tcp_rules(MIB_CE_ACL_IP_Tp pEntry, unsigned short port, unsigned short redirected_port)
{
	char ifname[IFNAMSIZ] = "";
	struct in_addr src;
	char ssrc[40];
	char str_port[8] = "";
	char str_rport[8] = "";

	src.s_addr = *(unsigned long *)pEntry->ipAddr;
	snprintf(ssrc, sizeof(ssrc), "%s/%hhu", inet_ntoa(src), pEntry->maskbit);

	snprintf(str_port, sizeof(str_port), "%hu", port);
	snprintf(str_rport, sizeof(str_rport), "%hu", redirected_port);

	if(pEntry->Interface != DUMMY_IFINDEX)
	{
		if(ifGetName(pEntry->Interface, ifname, sizeof(ifname)) == 0)
			return -1;

		// iptables -A inacc -i nas0_0 -s xxx.xxx.xxx.xxx -p TCP --dport 80 -j ACCEPT
		va_cmd(IPTABLES, 12, 1, FW_ADD, (char *)FW_ACL,
			(char *)ARG_I, ifname, "-s", ssrc, "-p", ARG_TCP, (char *)FW_DPORT, str_port,
			"-j", (char *)FW_ACCEPT);

		va_cmd(IPTABLES, 16, 1, ARG_T, "mangle", FW_ADD, (char *)FW_ACL,
			(char *)ARG_I, ifname, "-s", ssrc, "-p", ARG_TCP, (char *)FW_DPORT, str_port,
			"-j", (char *)"MARK", "--set-mark", RMACC_MARK);

		// redirect if this is not standard port
		if (redirected_port != port) {
			va_cmd(IPTABLES, 16, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_ACL,
					(char *)ARG_I, ifname, "-s", ssrc,	"-p", ARG_TCP, (char *)FW_DPORT, str_port,
					"-j", "REDIRECT", "--to-ports", str_rport);
		}
	}
	else
	{
		// iptables -A inacc -i nas0_0 -s xxx.xxx.xxx.xxx -p TCP --dport 80 -j ACCEPT
		va_cmd(IPTABLES, 10, 1, FW_ADD, (char *)FW_ACL,
			"-s", ssrc, "-p", ARG_TCP, (char *)FW_DPORT, str_port,
			"-j", (char *)FW_ACCEPT);

		va_cmd(IPTABLES, 14, 1, ARG_T, "mangle", FW_ADD, (char *)FW_ACL,
			"-s", ssrc, "-p", ARG_TCP, (char *)FW_DPORT, str_port,
			"-j", (char *)"MARK", "--set-mark", RMACC_MARK);

		// redirect if this is not standard port
		if (redirected_port != port) {
			va_cmd(IPTABLES, 14, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_ACL,
				"-s", ssrc,	"-p", ARG_TCP, (char *)FW_DPORT, str_port,
				"-j", "REDIRECT", "--to-ports", str_rport);
		}
	}

	return 0;
}

int filter_set_acl()
{
	int i, total;
	MIB_CE_ACL_IP_T Entry;

	filter_acl_flush();

	// Add policy to aclblock chain
	total = mib_chain_total(MIB_ACL_IP_TBL);
	for (i=0; i<total; i++)
	{
		if (!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&Entry))
			continue;

		if(Entry.Enabled == 0)
			continue;

		if(Entry.web)
			set_acl_tcp_rules(&Entry, Entry.web_port, 80);
		else if(Entry.https)
			set_acl_tcp_rules(&Entry, Entry.https_port, 443);
		else if(Entry.ssh)
			set_acl_tcp_rules(&Entry, Entry.ssh_port, 22);
		else if(Entry.telnet)
			set_acl_tcp_rules(&Entry, Entry.telnet_port, 23);
		else if(Entry.icmp)
		{
			struct in_addr net_in, src;
			char ssrc[40];

			net_in.s_addr = *(unsigned long *)Entry.ipAddr;
			src.s_addr = ntohl(net_in.s_addr);
			snprintf(ssrc, sizeof(ssrc), "%s/%hhu", inet_ntoa(src), Entry.maskbit);


			if(Entry.Interface != DUMMY_IFINDEX)
			{
				char ifname[IFNAMSIZ] = "";

				if(ifGetName(Entry.Interface, ifname, sizeof(ifname)) > 0)
				{
					// iptables -t mangle -A aclblock -i nas0_x -s 192.168.1.0/12 -p ICMP --icmp-type echo-request -m limit
					//   --limit 1/s -j MARK --set-mark 0x1000
					va_cmd(IPTABLES, 20, 1, ARG_T, "mangle", FW_ADD, (char *)FW_ACL,
						ARG_I, ifname, "-s", ssrc, "-p", "ICMP",
						"--icmp-type", "echo-request", "-m", "limit",
						"--limit", "1/s", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
				}
				else
				{
					fprintf(stderr, "[ACL] Unkown interface: 0x%x\n", Entry.Interface);
				}
			}
			else
			{
				// iptables -t mangle -A aclblock -s 192.168.1.0/12 -p ICMP --icmp-type echo-request -m limit
				//   --limit 1/s -j MARK --set-mark 0x1000
				va_cmd(IPTABLES, 18, 1, ARG_T, "mangle", FW_ADD, (char *)FW_ACL,
					"-s", ssrc, "-p", "ICMP",
					"--icmp-type", "echo-request", "-m", "limit",
					"--limit", "1/s", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
			}
		}
		else
		{
			fprintf(stderr, "[ACL] Unknown protocol on MIB index %d\n", i);
		}
	}

	filter_acl_allow_wan();
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-m", "state",
			"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);

	// iptables -A aclblock -j DROP
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_ACL, "-j", (char *)FW_DROP);

	return 0;
}
#else
void set_acl_service(unsigned char service, char *act, char *strIP, int isIPrange, char *protocol, unsigned short port)
{
	char strInputPort[8]="";

	snprintf(strInputPort, sizeof(strInputPort), "%d", port);
	if (service & ACL_ENTRY_LAN_SIDE_PERMISSION_SET) {	// can LAN access
		if (isIPrange) {
			// iptables -A inacc -i $LAN_IF -m iprange --src-range x.x.x.x-1x.x.x.x -p UDP --dport 69 -j ACCEPT
			va_cmd(IPTABLES, 14, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF_ALL, "-m", "iprange", "--src-range", strIP, "-p", protocol,
				(char *)FW_DPORT, strInputPort, "-j", (char *)FW_ACCEPT);
		}
		else {
			// iptables -A inacc -i $LAN_IF -s xxx.xxx.xxx.xxx -p UDP --dport 69 -j ACCEPT
			va_cmd(IPTABLES, 12, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF_ALL, "-s", strIP, "-p", protocol,
				(char *)FW_DPORT, strInputPort, "-j", (char *)FW_ACCEPT);
		}
	}
	if (service & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
		if (isIPrange) {
			// iptables -A inacc -i ! $LAN_IF -m iprange --src-range x.x.x.x-1x.x.x.x -p UDP --dport 69 -j ACCEPT
			va_cmd(IPTABLES, 15, 1, act, (char *)FW_ACL,
				"!", (char *)ARG_I, (char *)LANIF_ALL, "-m", "iprange", "--src-range", strIP, "-p", protocol,
				(char *)FW_DPORT, strInputPort, "-j", (char *)FW_ACCEPT);
		}
		else {
			// iptables -A inacc -i ! $LAN_IF -s xxx.xxx.xxx.xxx -p UDP --dport 69 -j ACCEPT
			va_cmd(IPTABLES, 13, 1, act, (char *)FW_ACL,
				"!", (char *)ARG_I, (char *)LANIF_ALL, "-s", strIP, "-p", protocol,
				(char *)FW_DPORT, strInputPort, "-j", (char *)FW_ACCEPT);
		}
	}
}

static void set_acl_service_redirect(unsigned char service, unsigned char Interface, unsigned short portRedirect, char *act, char *strIP, int isIPrange, char *protocol, unsigned short port, unsigned short whiteBlackRule)
{
	char strInputPort[16];
	char strPort[16], ipRuleStr[36];
	char policy[32];
	char rtk_policy[32];

	if (whiteBlackRule == 0)
		strcpy(policy, FW_ACCEPT);
	else
		strcpy(policy, FW_DROP);

	if(strlen(strIP)>0) {
		if (isIPrange)
			snprintf(ipRuleStr, sizeof(ipRuleStr), "%s", strIP);
		else
			snprintf(ipRuleStr, sizeof(ipRuleStr), "-s %s", strIP);
	}
	else
	{
		sprintf(ipRuleStr, "");
	}

	snprintf(strInputPort, sizeof(strInputPort), "%hu", port);

	if (Interface == IF_DOMAIN_LAN && service & ACL_ENTRY_LAN_SIDE_PERMISSION_SET) {	//can  LAN access
		// iptables -A aclblock -i $LAN_IF -m iprange --src-range x.x.x.x-1x.x.x.x -p TCP --dport 23 -j ACCEPT or
		// iptables -A aclblock -i $LAN_IF -s 192.168.1.0/24 -p TCP --dport 23  -j ACCEPT

		if(isIPrange){
			if (strIP[0])
				va_cmd(IPTABLES, 14, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF_ALL, "-m", "iprange", "--src-range", (char *)ipRuleStr, "-p", protocol,
				(char *)FW_DPORT, strInputPort, "-j", (char *)policy);
			else
				va_cmd(IPTABLES, 10, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF_ALL, "-p", protocol,
				(char *)FW_DPORT, strInputPort, "-j", (char *)policy);
		}
		else{
			if (strIP[0])
				va_cmd(IPTABLES, 11, 1, act, (char *)FW_ACL,
					(char *)ARG_I, (char *)LANIF_ALL, (char *)ipRuleStr, "-p", protocol,
					(char *)FW_DPORT, strInputPort, "-j", (char *)policy);
			else
				va_cmd(IPTABLES, 10, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF_ALL, "-p", protocol,
				(char *)FW_DPORT, strInputPort, "-j", (char *)policy);
		}
	}

	else if(Interface == IF_DOMAIN_LAN && (service == ACL_ENTRY_DENY_SET)) {
		if (whiteBlackRule == 0)
			strncpy(rtk_policy, FW_DROP,sizeof(policy));
		else
			strncpy(rtk_policy, FW_ACCEPT,sizeof(policy));

		if(isIPrange){
			if (strIP[0])
				va_cmd(IPTABLES, 14, 1, act, (char *)FW_ACL,
					(char *)ARG_I, (char *)LANIF_ALL, "-m", "iprange", "--src-range", (char *)ipRuleStr, "-p", protocol,
					(char *)FW_DPORT, strInputPort, "-j", (char *)rtk_policy);
			else
				va_cmd(IPTABLES, 10, 1, act, (char *)FW_ACL,
					(char *)ARG_I, (char *)LANIF_ALL, "-p", protocol,
					(char *)FW_DPORT, strInputPort, "-j", (char *)rtk_policy);
		}
		else{
			if (strIP[0])
				va_cmd(IPTABLES, 11, 1, act, (char *)FW_ACL,
					(char *)ARG_I, (char *)LANIF_ALL, (char *)ipRuleStr, "-p", protocol,
					(char *)FW_DPORT, strInputPort, "-j", (char *)rtk_policy);
			else
				va_cmd(IPTABLES, 10, 1, act, (char *)FW_ACL,
					(char *)ARG_I, (char *)LANIF_ALL, "-p", protocol,
					(char *)FW_DPORT, strInputPort, "-j", (char *)rtk_policy);
		}
	}

	if (Interface == IF_DOMAIN_WAN && service & ACL_ENTRY_WAN_SIDE_PERMISSION_SET ) {	// can WAN access . white list
		snprintf(strPort, sizeof(strPort), "%hu", portRedirect);
		if (whiteBlackRule == 0) {
			if(isIPrange){
				if (strIP[0])
					va_cmd(IPTABLES, 19, 1, ARG_T, "mangle", act, (char *)FW_ACL,
						"!", (char *)ARG_I, (char *)LANIF_ALL,"-m", "iprange", "--src-range", (char *)ipRuleStr, "-p", protocol,
						(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
				else
					va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act, (char *)FW_ACL,
						"!", (char *)ARG_I, (char *)LANIF_ALL, "-p", protocol,
						(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
			}
			else{
				if (strIP[0])
					va_cmd(IPTABLES, 16, 1, ARG_T, "mangle", act, (char *)FW_ACL,
					"!", (char *)ARG_I, (char *)LANIF_ALL, (char *)ipRuleStr, "-p", protocol,
					(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
				else
					va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act, (char *)FW_ACL,
						"!", (char *)ARG_I, (char *)LANIF_ALL, "-p", protocol,
						(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
			}
		}
		else
		{
			//iptables -A aclblock -i ! $LAN_IF -p TCP --dport 23  -j DROP
			if(isIPrange){
				if (strIP[0])
					va_cmd(IPTABLES, 15, 1, act, (char *)FW_ACL,
						 "!", (char *)ARG_I,  (char *)LANIF_ALL, "-m", "iprange", "--src-range", (char *)ipRuleStr, "-p", protocol,
						(char *)FW_DPORT, strInputPort, "-j", (char *)policy);
				else
					va_cmd(IPTABLES, 11, 1, act, (char *)FW_ACL,
						 "!", (char *)ARG_I,  (char *)LANIF_ALL, "-p", protocol,
						(char *)FW_DPORT, strInputPort, "-j", (char *)policy);
			}
			else{
				if (strIP[0])
					va_cmd(IPTABLES, 12, 1, act, (char *)FW_ACL,
						 "!", (char *)ARG_I,  (char *)LANIF_ALL, (char *)ipRuleStr, "-p", protocol,
						(char *)FW_DPORT, strInputPort, "-j", (char *)policy);
				else
					va_cmd(IPTABLES, 11, 1, act, (char *)FW_ACL,
						 "!", (char *)ARG_I,  (char *)LANIF_ALL, "-p", protocol,
						(char *)FW_DPORT, strInputPort, "-j", (char *)policy);
			}
		}

		// redirect if this is not standard port
		if ((whiteBlackRule == 0) && (portRedirect != port)) {
			if(isIPrange){
				if (strIP[0])
					va_cmd(IPTABLES, 19, 1, "-t", "nat",
							(char *)act, (char *)FW_ACL,"!", (char *)ARG_I, (char *)LANIF_ALL, "-m", "iprange", "--src-range",(char *)ipRuleStr,
							"-p", protocol, (char *)FW_DPORT, strPort, "-j",
							"REDIRECT", "--to-ports", strInputPort);
				else
					va_cmd(IPTABLES, 15, 1, "-t", "nat",
							(char *)act, (char *)FW_ACL,"!", (char *)ARG_I, (char *)LANIF_ALL,
							"-p", protocol, (char *)FW_DPORT, strPort, "-j",
							"REDIRECT", "--to-ports", strInputPort);
			}
			else{
				if (strIP[0])
					va_cmd(IPTABLES, 16, 1, "-t", "nat",
							(char *)act, (char *)FW_ACL,"!", (char *)ARG_I, (char *)LANIF_ALL, (char *)ipRuleStr,
							"-p", protocol, (char *)FW_DPORT, strPort, "-j",
							"REDIRECT", "--to-ports", strInputPort);
				else
					va_cmd(IPTABLES, 15, 1, "-t", "nat",
							(char *)act, (char *)FW_ACL,"!", (char *)ARG_I, (char *)LANIF_ALL,
							"-p", protocol, (char *)FW_DPORT, strPort, "-j",
							"REDIRECT", "--to-ports", strInputPort);
			}
		}
	}
}

static void filter_set_acl_service(MIB_CE_ACL_IP_T Entry, unsigned char Interface, int enable, char *strIP, int isIPrange)
{
	char *act;
	char strPort[16], ipRuleStr[36], policy[32];
	unsigned char whiteBlackRule;
	int ret;

	if (enable)
		act = (char *)FW_ADD;
	else
		act = (char *)FW_DEL;

	if(strlen(strIP)>0) {
		if (isIPrange)
			snprintf(ipRuleStr, sizeof(ipRuleStr), "%s", strIP);
		else
			snprintf(ipRuleStr, sizeof(ipRuleStr), "-s %s", strIP);
	}
	else
	{
		sprintf(ipRuleStr, "-s 0.0.0.0/0");
	}

	mib_get_s(MIB_ACL_CAPABILITY_TYPE, (void *)&whiteBlackRule, sizeof(whiteBlackRule));

#ifndef USE_DEONET
	// telnet service: bring up by inetd, telnetd from user/telnetd or busybox
	set_acl_service_redirect(Entry.telnet, Interface, Entry.telnet_port, act, strIP, isIPrange, (char *)ARG_TCP, 23, whiteBlackRule);

#ifdef CONFIG_USER_FTPD_FTPD
	// ftp service: bring up by inetd
	set_acl_service_redirect(Entry.ftp, Interface, Entry.ftp_port, act, strIP, isIPrange, (char *)ARG_TCP, 21, whiteBlackRule);
#endif

#ifdef CONFIG_USER_TFTPD_TFTPD
	// tftp service: bring up by inetd
	set_acl_service_redirect(Entry.tftp, Interface, 69, act, strIP, isIPrange, (char *)ARG_UDP, 69, whiteBlackRule);
#endif

#ifdef CONFIG_USER_BOA_WITH_SSL
  	//HTTPS service
	set_acl_service_redirect(Entry.https, Interface, Entry.https_port, act, strIP, isIPrange, (char *)ARG_TCP, 443, whiteBlackRule);
#endif

#else
	// HTTP service
	set_acl_service_redirect(Entry.web, Interface, Entry.web_port, act, strIP, isIPrange, (char *)ARG_TCP, 80, whiteBlackRule);

#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	// snmp service
	if (Interface == IF_DOMAIN_LAN && Entry.snmp & ACL_ENTRY_LAN_SIDE_PERMISSION_SET) {	// can LAN access
		// iptables -A aclblock -i $LAN_IF -m iprange --src-range x.x.x.x-x.x.x.x -p UDP --dport 161:162 -j ACCEPT
		if(isIPrange){
			if (strIP[0])
				va_cmd(IPTABLES, 14, 1, act, (char *)FW_ACL,
					(char *)ARG_I, (char *)LANIF_ALL, "-m", "iprange", "--src-range", (char*)ipRuleStr, "-p", (char *)ARG_UDP,
					(char *)FW_DPORT, "161:162", "-j", (char *)FW_ACCEPT);
			else
				va_cmd(IPTABLES, 10, 1, act, (char *)FW_ACL,
					(char *)ARG_I, (char *)LANIF_ALL, "-p", (char *)ARG_UDP,
					(char *)FW_DPORT, "161:162", "-j", (char *)FW_ACCEPT);
		}
		else{
			va_cmd(IPTABLES, 11, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF_ALL, (char*)ipRuleStr, "-p", (char *)ARG_UDP,
				(char *)FW_DPORT, "161:162", "-j", (char *)FW_ACCEPT);
		}
	}
	if (Interface == IF_DOMAIN_WAN && Entry.snmp & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) {	// can WAN access
		// iptables -t mangle -A aclblock ! -i $LAN_IF -m iprange --src-range x.x.x.x-x.x.x.x -p UDP --dport 161:162 -m limit
		//  --limit 100/s --limit-burst 500 -j MARK --set-mark 0x1000
		if(isIPrange){
			if (strIP[0])
				va_cmd(IPTABLES, 25, 1, ARG_T, "mangle", act, (char *)FW_ACL,
					"!", (char *)ARG_I, (char *)LANIF_ALL, "-m", "iprange", "--src-range", (char*)ipRuleStr, "-p",
					(char *)ARG_UDP, (char *)FW_DPORT, "161:162", "-m",
					"limit", "--limit", "100/s", "--limit-burst",
					"500", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
			else
				va_cmd(IPTABLES, 21, 1, ARG_T, "mangle", act, (char *)FW_ACL,
					"!", (char *)ARG_I, (char *)LANIF_ALL, "-p",
					(char *)ARG_UDP, (char *)FW_DPORT, "161:162", "-m",
					"limit", "--limit", "100/s", "--limit-burst",
					"500", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
		}
		else{
			va_cmd(IPTABLES, 22, 1, ARG_T, "mangle", act, (char *)FW_ACL,
				"!", (char *)ARG_I, (char *)LANIF_ALL, (char*)ipRuleStr, "-p",
				(char *)ARG_UDP, (char *)FW_DPORT, "161:162", "-m",
				"limit", "--limit", "100/s", "--limit-burst",
				"500", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
		}
	}
#endif

#ifdef CONFIG_USER_SSH_DROPBEAR
	// ssh service
	set_acl_service_redirect(Entry.ssh, Interface, Entry.ssh_port, act, strIP, isIPrange, (char *)ARG_TCP, 22, whiteBlackRule);
#endif
#endif

#ifdef CONFIG_USER_SAMBA
	set_acl_service_redirect(Entry.smb, Interface, Entry.smb_udp_port1, act, strIP, isIPrange, (char *)ARG_UDP, 137, whiteBlackRule);
	set_acl_service_redirect(Entry.smb, Interface, Entry.smb_udp_port2, act, strIP, isIPrange, (char *)ARG_UDP, 138, whiteBlackRule);
	set_acl_service_redirect(Entry.smb, Interface, Entry.smb_tcp_port, act, strIP, isIPrange, (char *)ARG_TCP, 445, whiteBlackRule);
#endif

#if defined(CONFIG_CMCC) //block wan 8080 port, java web
	if (Interface == IF_DOMAIN_WAN)
		set_acl_service_redirect(1, IF_DOMAIN_WAN, 8080, act, strIP, isIPrange, (char *)ARG_TCP, 8080, 1);
#endif

	if (whiteBlackRule == 0)
		strcpy(policy, FW_ACCEPT);
	else
		strcpy(policy, FW_DROP);

	// ping service
	if (Interface == IF_DOMAIN_LAN && Entry.icmp & ACL_ENTRY_LAN_SIDE_PERMISSION_SET) // can LAN access
	{
		// iptables -A aclblock -i $LAN_IF -m iprange --src-range x.x.x.x-x.x.x.x -p ICMP --icmp-type echo-request -m limit
		//   --limit 1/s -j ACCEPT
		if(isIPrange){
			if (strIP[0])
				va_cmd(IPTABLES, 18, 1, act, (char *)FW_ACL,
					(char *)ARG_I, (char *)LANIF_ALL, "-m", "iprange", "--src-range", (char*)ipRuleStr, "-p", "ICMP",
					"--icmp-type", "echo-request", "-m", "limit",
					"--limit", "1/s", "-j", (char *)policy);
			else
				va_cmd(IPTABLES, 14, 1, act, (char *)FW_ACL,
					(char *)ARG_I, (char *)LANIF_ALL, "-p", "ICMP",
					"--icmp-type", "echo-request", "-m", "limit",
					"--limit", "1/s", "-j", (char *)policy);
		}
		else{
			va_cmd(IPTABLES, 15, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF_ALL, (char*)ipRuleStr, "-p", "ICMP",
				"--icmp-type", "echo-request", "-m", "limit",
				"--limit", "1/s", "-j", (char *)policy);
		}
	}

	if (Interface == IF_DOMAIN_WAN && Entry.icmp & ACL_ENTRY_WAN_SIDE_PERMISSION_SET) // can WAN access
	{
		// iptables -t mangle -A aclblock ! -i $LAN_IF -m iprange --src-range x.x.x.x-x.x.x.x -p ICMP --icmp-type echo-request -m limit
		//   --limit 1/s -j MARK --set-mark 0x1000
		if (whiteBlackRule == 0) {
			if(isIPrange){
				if (strIP[0])
					va_cmd(IPTABLES, 23, 1, ARG_T, "mangle", act, (char *)FW_ACL,
						"!", (char *)ARG_I, (char *)LANIF_ALL, "-m", "iprange", "--src-range", (char*)ipRuleStr, "-p", "ICMP",
						"--icmp-type", "echo-request", "-m", "limit",
						"--limit", "1/s", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
				else
					va_cmd(IPTABLES, 19, 1, ARG_T, "mangle", act, (char *)FW_ACL,
						"!", (char *)ARG_I, (char *)LANIF_ALL, "-p", "ICMP",
						"--icmp-type", "echo-request", "-m", "limit",
						"--limit", "1/s", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
			}
			else{
				va_cmd(IPTABLES, 20, 1, ARG_T, "mangle", act, (char *)FW_ACL,
					"!", (char *)ARG_I, (char *)LANIF_ALL, (char*)ipRuleStr, "-p", "ICMP",
					"--icmp-type", "echo-request", "-m", "limit",
					"--limit", "1/s", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
			}
		}
		else
		{
			// iptables -t mangle -A aclblock ! -i $LAN_IF -s 192.168.1.0/12 -p ICMP --icmp-type echo-request -m limit
			//   --limit 1/s -j MARK --set-mark 0x1000
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_ACL,
				(char *)ARG_I, "lo", "-p", "ICMP",
				"--icmp-type", "echo-request", "-j", (char *)FW_ACCEPT);

			if(isIPrange){
				if (strIP[0])
					va_cmd(IPTABLES, 19, 1, act, (char *)FW_ACL,
						"!", (char *)ARG_I, (char *)LANIF_ALL, "-m", "iprange", "--src-range", (char*)ipRuleStr, "-p", "ICMP",
						"--icmp-type", "echo-request", "-m", "limit",
						"--limit", "1/s", "-j", (char *)policy);
				else
					va_cmd(IPTABLES, 15, 1, act, (char *)FW_ACL,
						"!", (char *)ARG_I, (char *)LANIF_ALL, "-p", "ICMP",
						"--icmp-type", "echo-request", "-m", "limit",
						"--limit", "1/s", "-j", (char *)policy);
			}
			else{
				va_cmd(IPTABLES, 16, 1, act, (char *)FW_ACL,
					"!", (char *)ARG_I, (char *)LANIF_ALL, (char*)ipRuleStr, "-p", "ICMP",
					"--icmp-type", "echo-request", "-m", "limit",
					"--limit", "1/s", "-j", (char *)policy);
			}
		}
	}
}

static void deo_filter_set_acl_service(MIB_CE_ACL_IP_T Entry, unsigned char Interface, int enable, char *strIP, int isIPrange)
{
	char *act;
	char ipRuleStr[36], policy[32], ifname[10];
	int ret, wan_mode;

	if (enable)
		act = (char *)FW_INSERT;
	else
		act = (char *)FW_DEL;

	if(strlen(strIP)>0) {
		if (isIPrange)
			snprintf(ipRuleStr, sizeof(ipRuleStr), "%s", strIP);
		else
			snprintf(ipRuleStr, sizeof(ipRuleStr), "-s %s", strIP);
	}
	else
	{
		sprintf(ipRuleStr, "-s 0.0.0.0/0");
	}

	strcpy(policy, FW_RETURN);

	wan_mode = deo_wan_nat_mode_get();
	memset(ifname, 0, sizeof(ifname));

	if (wan_mode == DEO_NAT_MODE)
		strcpy(ifname, "nas0+");
	else
		strcpy(ifname, "br0");


	if (Interface == IF_DOMAIN_WAN) // can WAN access
	{
		// iptables -t mangle -A aclblock ! -i $LAN_IF -s 192.168.1.0/12 -p ICMP --icmp-type echo-request -m limit
		//   --limit 1/s -j MARK --set-mark 0x1000
		if(isIPrange){
			if (strIP[0])
			{
				va_cmd(IPTABLES, 10, 1, act, (char *)SKB_ACL,
						(char *)ARG_I, (char *)ifname, "-m", "iprange", "--src-range", (char*)ipRuleStr, 
						"-j", (char *)policy);
			}
		}
	}
}


void filter_set_acl(int enable)
{
	int i, total;
	struct in_addr src;
	char ssrc[40], policy[32], policy_end[32];
	MIB_CE_ACL_IP_T Entry;
	unsigned char whiteBlackRule;

#ifdef ACL_IP_RANGE
	struct in_addr start,end;
	char sstart[16],send[16];
#endif

#ifdef MAC_ACL
	MIB_CE_ACL_MAC_T MacEntry;
	char macaddr[18];
#endif

	// Added by Mason Yu for ACL.
	// check if ACL Capability is enabled ?
	if (!enable) {
#ifdef CONFIG_E8B
		filter_acl_flush();
#else
		filter_acl_flush();

#if defined(CONFIG_USER_PPTP_SERVER)
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_PPTP_VPN_IN);
	setup_pptp_server_firewall(PPTPS_RULE_TYPE_IN);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_ACL, "-j", (char *)IPTABLE_PPTP_VPN_IN);
#endif
#if defined(CONFIG_USER_L2TPD_LNS)
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_L2TP_VPN_IN);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_ACL, "-j", (char *)IPTABLE_L2TP_VPN_IN);
#endif

		filter_acl_allow_wan();
		va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-m", "state",
			"--state", "NEW,ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);
		// For security issue, we do NOT allow connection attemp from wan even if ACL is disabled.
		// iptables -A -i ! br0 aclblock -j DROP
#ifndef CONFIG_RTK_DEV_AP
		va_cmd(IPTABLES, 7, 1, (char *)FW_ADD, (char *)FW_ACL, "!", (char *)ARG_I, (char *)LANIF_ALL, "-j", (char *)FW_DROP);
#endif
#endif
	}
	else {
		mib_get_s(MIB_ACL_CAPABILITY_TYPE, (void *)&whiteBlackRule, sizeof(whiteBlackRule));
		filter_acl_flush();
		filter_acl_allow_wan();
		// Add policy to aclblock chain
		total = mib_chain_total(MIB_ACL_IP_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&Entry))
			{
				return;
			}

//    		printf("\nstartip=%x,endip=%x\n",*(unsigned long*)Entry.startipAddr,*(unsigned long*)Entry.endipAddr);

			// Check if this entry is enabled
			if ( Entry.Enabled == 1 ) {
				if (whiteBlackRule == ACL_TYPE_WHITE_LIST ) //white List
				{
					strcpy(policy, FW_RETURN);
					strcpy(policy_end, FW_DROP);
				}
				else //Black List CMCC
				{
					strcpy(policy, FW_DROP);
					strcpy(policy_end, FW_RETURN);
				}
#ifdef ACL_IP_RANGE
				start.s_addr = *(unsigned long*)Entry.startipAddr;
				end.s_addr = *(unsigned long*)Entry.endipAddr;
				if (0 == start.s_addr)
				{
					sstart[0] = '\0';
					ssrc[0] = '\0';
				}
				else
				{
					strcpy(sstart, inet_ntoa(start));
					strcpy(send, inet_ntoa(end));
					strcpy(ssrc,sstart);
					strcat(ssrc,"-");
					strcat(ssrc,send);
				}
#else
				src.s_addr = *(unsigned long *)Entry.ipAddr;

				// inet_ntoa is not reentrant, we have to
				// copy the static memory before reuse it
				if (0 == src.s_addr)
					ssrc[0] = '\0';
				else
					snprintf(ssrc, sizeof(ssrc), "%s/%hhu", inet_ntoa(src), Entry.maskbit);

#endif
		        	if ( Entry.Interface == IF_DOMAIN_LAN ) {
#ifdef ACL_IP_RANGE
					if(*(unsigned long*)Entry.startipAddr != *(unsigned long*)Entry.endipAddr) {
						if (Entry.any == 0x01) { 	// service = any(0x01)
					// iptables -A aclblock -m iprange --src-range x.x.x.x-1x.x.x.x -j RETURN
							if (ssrc[0])
								va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_ACL, "-i", LANIF_ALL, "-m", "iprange", "--src-range", ssrc, "-j", (char *)policy);
							else
								va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-i", LANIF_ALL, "-j", (char *)policy);
						}
						else
							filter_set_acl_service(Entry, Entry.Interface, enable, ssrc, 1);

					}
					else {
						if (Entry.any == 0x01)  	// service = any(0x01)
						{
							if (sstart[0])
								va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-i", LANIF_ALL, "-s", sstart, "-j", (char *)policy);
							else
								va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_ACL, "-i", LANIF_ALL, "-j", (char *)policy);
						}
					else
							filter_set_acl_service(Entry, Entry.Interface, enable, sstart, 0);
					}
#else
					if (Entry.any == 0x01)  	// service = any(0x01)
					{
					// iptables -A INPUT -s xxx.xxx.xxx.xxx
						if (ssrc[0])
							va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-i", LANIF_ALL, "-s", ssrc, "-j", (char *)policy);
						else
							va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_ACL, "-i", LANIF_ALL, "-j", (char *)policy);
					}
					else
						filter_set_acl_service(Entry, Entry.Interface, enable, ssrc, 0);
#endif
				} else {
					// iptables -A INPUT -s xxx.xxx.xxx.xxx
					//va_cmd(IPTABLES, 9, 1, (char *)FW_ADD, "aclblock", "!", "-i", BRIF, "-s", ssrc, "-j", (char *)FW_RETURN);
#ifdef ACL_IP_RANGE
					if(*(unsigned long*)Entry.startipAddr != *(unsigned long*)Entry.endipAddr) {
						filter_set_acl_service(Entry, Entry.Interface, enable, ssrc, 1);
					}
					else {
						filter_set_acl_service(Entry, Entry.Interface, enable, sstart, 0);
					}

#else
					filter_set_acl_service(Entry, Entry.Interface, enable, ssrc, 0);
#endif
				}
			}
		}

#ifdef MAC_ACL
		total = mib_chain_total(MIB_ACL_MAC_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_ACL_MAC_TBL, i, (void *)&MacEntry))
			{
				return;
			}

			// Check if this entry is enabled
			if ( MacEntry.Enabled == 1 ) {
				snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
					MacEntry.macAddr[0], MacEntry.macAddr[1],
					MacEntry.macAddr[2], MacEntry.macAddr[3],
					MacEntry.macAddr[4], MacEntry.macAddr[5]);

		        if ( MacEntry.Interface == IF_DOMAIN_LAN ) {
		 			// iptables -A aclblock -i br0  -m mac --mac-source $MAC -j RETURN
					va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_ACL,
							(char *)ARG_I, LANIF_ALL, "-m", "mac",
							"--mac-source",  macaddr, "-j", (char *)policy);
				} else {
					// iptables -A aclblock -i ! br0  -m mac --mac-source $MAC -j RETURN
					va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_ACL,
							 "!", (char *)ARG_I, LANIF_ALL, "-m", "mac",
							"--mac-source",  macaddr, "-j", (char *)policy);
				}
			}
		}
#endif
		// (1) allow for LAN
		// allowing DNS request during ACL enabled
		// iptables -A aclblock -p udp -i br0 --dport 53 -j RETURN
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "udp", "-i", LANIF_ALL,(char*)FW_DPORT, (char *)PORT_DNS, "-j", (char *)FW_RETURN);
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "udp","!", "-i", LANIF_ALL,(char*)FW_DPORT, (char *)PORT_DNS, "-j", (char *)FW_DROP);
#ifdef CONFIG_CMCC
		va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "tcp", (char*)FW_DPORT, (char *)PORT_DNS, "-j", (char *)FW_DROP);
#endif

		// allowing DHCP request during ACL enabled
		// iptables -A aclblock -p udp -i br0 --dport 67 -j RETURN
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "udp", "-i", LANIF_ALL, (char*)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_RETURN);

#if defined(CONFIG_USER_PPTP_SERVER)
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_ACL, "-j", (char *)IPTABLE_PPTP_VPN_IN);
#endif
#if defined(CONFIG_USER_L2TPD_LNS)
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_ACL, "-j", (char *)IPTABLE_L2TP_VPN_IN);
#endif
		va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-m", "state",
			"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);

		// iptables -A aclblock -j DROP
		va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_ACL, "-j", (char *)policy_end);
	}
}
#endif	// CONFIG_00R0
#endif	// IP_ACL

#ifdef USE_DEONET 
void deo_filter_set_acl(int enable)
{
	int i, total;
	struct in_addr src;
	char ssrc[40], policy[32], policy_end[32];
	MIB_CE_ACL_IP_T Entry;
	unsigned char whiteBlackRule;

#ifdef ACL_IP_RANGE
	struct in_addr start,end;
	char sstart[16],send[16];
#endif

#ifdef MAC_ACL
	MIB_CE_ACL_MAC_T MacEntry;
	char macaddr[18];
#endif

	// Added by Mason Yu for ACL.
	// check if ACL Capability is enabled ?
	if (!enable) {
		filter_acl_flush();
		deo_filter_acl_flush();
	}
	else {
		mib_get_s(MIB_ACL_CAPABILITY_TYPE, (void *)&whiteBlackRule, sizeof(whiteBlackRule));

		filter_acl_flush();

		// Add policy to aclblock chain
		total = mib_chain_total(MIB_ACL_IP_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&Entry))
			{
				return;
			}

//    		printf("\nstartip=%x,endip=%x\n",*(unsigned long*)Entry.startipAddr,*(unsigned long*)Entry.endipAddr);

			// Check if this entry is enabled
			if ( Entry.Enabled == 1 ) {

				strcpy(policy, FW_RETURN);

				start.s_addr = *(unsigned long*)Entry.startipAddr;
				end.s_addr = *(unsigned long*)Entry.endipAddr;
				if (0 == start.s_addr)
				{
					sstart[0] = '\0';
					ssrc[0] = '\0';
				}
				else
				{
					strcpy(sstart, inet_ntoa(start));
					strcpy(send, inet_ntoa(end));
					strcpy(ssrc,sstart);
					strcat(ssrc,"-");
					strcat(ssrc,send);
				}

				{
					// iptables -A INPUT -s xxx.xxx.xxx.xxx
					//va_cmd(IPTABLES, 9, 1, (char *)FW_ADD, "aclblock", "!", "-i", BRIF, "-s", ssrc, "-j", (char *)FW_RETURN);
					if(*(unsigned long*)Entry.startipAddr != *(unsigned long*)Entry.endipAddr) {
						deo_filter_set_acl_service(Entry, Entry.Interface, enable, ssrc, 1);
					}
					else {
						deo_filter_set_acl_service(Entry, Entry.Interface, enable, sstart, 0);
					}

				}
			}
		}
	}
}
#endif

#if defined(IP_ACL)
#ifdef CONFIG_00R0
int restart_acl(void)
{
	unsigned char telnet_enable = 0;
	mib_get_s(MIB_TELNET_ENABLE, &telnet_enable, sizeof(telnet_enable));

	// iptables -D INPUT -i br0 -p TCP --dport 23 -j DROP
	va_cmd(IPTABLES, 10, 1, FW_DEL, (char *)FW_INPUT,
		"-i", BRIF, "-p", ARG_TCP, (char *)FW_DPORT, "23",
		"-j", (char *)FW_DROP);
	if(!telnet_enable)
	{
		// iptables -I INPUT -i br0 -p TCP --dport 23 -j DROP
		va_cmd(IPTABLES, 10, 1, FW_INSERT, (char *)FW_INPUT,
			"-i", BRIF, "-p", ARG_TCP, (char *)FW_DPORT, "23",
			"-j", (char *)FW_DROP);
	}

	return filter_set_acl();
}
#else
#ifdef AUTO_LANIP_CHANGE_FOR_IP_ACL
void autoChangeLanipForIpAcl(unsigned int origip, unsigned int origmask, unsigned int newip, unsigned int newmask)
{
	int i, entryNum, changeflag=0;
	MIB_CE_ACL_IP_T Entry;
	unsigned int rangestart, rangeend, masklen;
	if((origip==0) || (newip==0) || ((origip==newip) && (origmask==newmask)))
		return;
	//printf("[%s %d]orig:%x/%x new:%x/%x\n",__func__,__LINE__,origip,origmask,newip,newmask);
	entryNum = mib_chain_total(MIB_ACL_IP_TBL);
	for (i=0; i<entryNum; i++)
	{
		if(!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&Entry))
			continue;
		if(Entry.Interface != IF_DOMAIN_LAN)
			continue;
#ifdef ACL_IP_RANGE
		if(((*((unsigned int*)Entry.startipAddr))&origmask) == (origip&origmask))
		{
			if(origmask >= newmask)
			{
				rangestart = (*((unsigned int*)Entry.startipAddr))&(~origmask);
				rangeend = (*((unsigned int*)Entry.endipAddr))&(~origmask);
			}
			else
			{
				//change to the new whole subnet
				rangestart = 1;
				rangeend = (~newmask)-1;
			}
			//printf("[%s %d]rule%u range:%u~%u\n",__func__,__LINE__,i,rangestart,rangeend);
			*(unsigned int*)Entry.startipAddr = (newip&newmask) + rangestart;
			*(unsigned int*)Entry.endipAddr = (newip&newmask) + rangeend;
			changeflag=1;
		}
#else
		if((*((unsigned int*)Entry.ipAddr)&origmask) == (origip&origmask))
		{
			masklen = 31;
			while((masklen>=0) && (newmask & (1<<masklen)))
				masklen--;

			*((unsigned int*)Entry.ipAddr) = newip;
			Entry.maskbit = 32-(masklen+1);
			changeflag=1;
		}
#endif
		if(changeflag)
			mib_chain_update(MIB_ACL_IP_TBL, (void *)&Entry, i);
	}
	if(changeflag)
		restart_acl();
}
#endif

/*
 *	Return value:
 *	0	: fail
 *	1	: successful
 *	pMsg	: error message
 */
int checkACL(char *pMsg, int idx, unsigned char aclcap)
{
	int i, entryNum;
	unsigned char whiteBlackRule;
	MIB_CE_ACL_IP_T Entry;
	// if ACL is disable, it just return success.
	if (!aclcap) return 1;
	mib_get_s(MIB_ACL_CAPABILITY_TYPE, (void *)&whiteBlackRule, sizeof(whiteBlackRule));
	entryNum = mib_chain_total(MIB_ACL_IP_TBL);

	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&Entry))
		{
			//boaError(wp, 400, "Get chain record error!\n");
			strcpy(pMsg, "Get chain record error!\n");
			return 0;
		}
		if (idx != -1) {
			if ((idx == i) && (idx != 0))
			{
				continue;
			}
		}

#if 0
		if (Entry.Interface == IF_DOMAIN_WAN)
			continue;
#endif

		if (whiteBlackRule == ACL_TYPE_WHITE_LIST ) //whiteList
		{
			if ((Entry.web & ACL_ENTRY_LAN_SIDE_PERMISSION_SET) || (Entry.any==0x01))  // can web access from LAN
			{
				return 1;
			}
		}
		else
		{
#if !defined(CONFIG_CMCC) || !defined(CONFIG_CU)
			//OSGI will close web
			if(!(Entry.web & ACL_ENTRY_LAN_SIDE_PERMISSION_SET))
#endif
			{
			return 1;
			}
		}
	}
	strcpy(pMsg, "Sorry! There must be at lease one IP from LAN that can reach WUI !!\n");
	return 0;
}

int restart_acl(void)
{
	unsigned char aclEnable;
	char msg[100];

	mib_get_s(MIB_ACL_CAPABILITY, &aclEnable, sizeof(aclEnable));

	deo_filter_set_acl(0);

	if (aclEnable == 1) {  // ACL Capability is enabled
		if (checkACL(msg, -1, 1)) {
			deo_filter_set_acl(1);
		} else {
			fputs(msg, stderr);
			aclEnable = 0;
			mib_set(MIB_ACL_CAPABILITY, &aclEnable);
		}
	}

	return 0;
}

int set_ip_acl_tbl_service(int service_type, unsigned char enable){
#ifdef ACL_IP_RANGE
	printf("[%s:%d] ACL_IP_RANGE case not support in this API!!\n", __FILE__,__LINE__);
	return RTK_FAILED;
#else
	MIB_CE_ACL_IP_T Entry, oldEntry;
	int mibtotal, i;
	unsigned char *entryService=NULL;
	struct in_addr addr;
	unsigned char type = ACL_TYPE_WHITE_LIST;	//default white

	mib_get_s(MIB_ACL_CAPABILITY_TYPE, &type, sizeof(type));

	mibtotal = mib_chain_total(MIB_ACL_IP_TBL);
	for(i=0; i<mibtotal; i++){
		mibtotal = mib_chain_total(MIB_ACL_IP_TBL);
		if (!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&oldEntry)){
			printf("[%s:%d] mib_chain_get MIB_ACL_IP_TBL.%d failed!!\n", __FILE__,__LINE__,i);
			return RTK_FAILED;
		}
		memcpy(&Entry, &oldEntry, sizeof(Entry));
		if(service_type == SAMBA_SERVICE)
			entryService = &(Entry.smb);
		else if(service_type == FTP_SERVICE)
			entryService = &(Entry.ftp);
		else if(service_type == TELNET_SERVICE){
			entryService = &(Entry.telnet);
		}
		else if(service_type == SSH_SERVICE)
			entryService = &(Entry.ssh);
		else if(service_type == ICMP_SERVICE)
			entryService = &(Entry.icmp);
		else
			return RTK_FAILED;
		addr.s_addr = *(unsigned long *)Entry.ipAddr;
#ifdef CONFIG_RTK_DEV_AP
		if(addr.s_addr == 0)
#endif
		{
			if(Entry.Interface == IF_DOMAIN_LAN){
				if(enable & RTK_SERVICE_LAN_SIDE_ALLOW){//Lan can access
					if(type == ACL_TYPE_WHITE_LIST)
						*entryService = ACL_ENTRY_LAN_SIDE_PERMISSION_SET;
					else
						*entryService = 0;
				}else{
					if(type == ACL_TYPE_WHITE_LIST)
						*entryService = 0;
					else
						*entryService = ACL_ENTRY_LAN_SIDE_PERMISSION_SET;
				}
			}else if(Entry.Interface == IF_DOMAIN_WAN){
				if(enable & RTK_SERVICE_WAN_SIDE_ALLOW){//Wan can access
					if(type == ACL_TYPE_WHITE_LIST)
						*entryService = ACL_ENTRY_WAN_SIDE_PERMISSION_SET;
					else
						*entryService = 0;
				}else{
					if(type == ACL_TYPE_WHITE_LIST)
						*entryService = 0;
					else
						*entryService = ACL_ENTRY_WAN_SIDE_PERMISSION_SET;
				}
			}
			mib_chain_update(MIB_ACL_IP_TBL, (char *)&Entry, i );
		}
	}

	restart_acl();
	return RTK_SUCCESS;
#endif
}


int get_ip_acl_tbl_service(int service_type, unsigned char *enable){
#ifdef ACL_IP_RANGE
	printf("[%s:%d] ACL_IP_RANGE case not support in this API!!\n", __FILE__,__LINE__);
	return RTK_FAILED;
#else
	MIB_CE_ACL_IP_T Entry;
	int mibtotal, i;
	unsigned char entryService=0;
	struct in_addr addr;

	unsigned char type = ACL_TYPE_WHITE_LIST;	//default white

	mib_get_s(MIB_ACL_CAPABILITY_TYPE, &type, sizeof(type));
	mibtotal = mib_chain_total(MIB_ACL_IP_TBL);
	for(i=0; i<mibtotal; i++){
		mibtotal = mib_chain_total(MIB_ACL_IP_TBL);
		if (!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&Entry)){
			printf("[%s:%d] mib_chain_get MIB_ACL_IP_TBL.%d failed!!\n", __FILE__,__LINE__,i);
			return RTK_FAILED;
		}

		if(service_type == SAMBA_SERVICE)
			entryService = Entry.smb;
		else if(service_type == FTP_SERVICE)
			entryService = Entry.ftp;
		else if(service_type == TELNET_SERVICE)
			entryService = Entry.telnet;
		else if(service_type == SSH_SERVICE)
			entryService = Entry.ssh;
		else if(service_type == ICMP_SERVICE)
			entryService = Entry.icmp;
		else
			return RTK_FAILED;

		addr.s_addr = *(unsigned long *)Entry.ipAddr;
#ifdef CONFIG_RTK_DEV_AP
		if(addr.s_addr == 0)
#endif
		{
			if(Entry.Interface == IF_DOMAIN_LAN)
			{
				if(type == ACL_TYPE_WHITE_LIST && (entryService & ACL_ENTRY_LAN_SIDE_PERMISSION_SET)) //Lan can access
					*enable |= RTK_SERVICE_LAN_SIDE_ALLOW;
				else if(type == ACL_TYPE_BLACK_LIST && !(entryService & ACL_ENTRY_LAN_SIDE_PERMISSION_SET))
					*enable |= RTK_SERVICE_LAN_SIDE_ALLOW;
			}
			else if(Entry.Interface == IF_DOMAIN_WAN)
			{
				if(type == ACL_TYPE_WHITE_LIST && (entryService & ACL_ENTRY_WAN_SIDE_PERMISSION_SET)) //Wan can access
					*enable |= RTK_SERVICE_WAN_SIDE_ALLOW;
				else if(type == ACL_TYPE_BLACK_LIST && !(entryService & ACL_ENTRY_LAN_SIDE_PERMISSION_SET))
					*enable |= RTK_SERVICE_WAN_SIDE_ALLOW;
			}
		}

	}
	return RTK_SUCCESS;
#endif
}


#endif
#endif
#ifdef CONFIG_TRUE
void rtk_fw_sync_from_wanAccess_to_rmtacc(void)
{
	int i, totalNum;
	unsigned char httpEnable=0, aclcap;
	unsigned int  lanRuleExistFlag = 0;
	unsigned int  httpPort;

	mib_get_s(MIB_RMTACC_HTTP_ENABLE, (void *)&httpEnable, sizeof(httpEnable));
	mib_get_s(MIB_RMTACC_HTTP_WAN_PORT, (void *)&httpPort, sizeof(httpPort));
#ifdef IP_ACL
	MIB_CE_ACL_IP_T Entry;
	totalNum = mib_chain_total(MIB_ACL_IP_TBL);
	for (i=totalNum-1; i>=0; i--)
	{
		if (!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&Entry))
			continue;

		if (IF_DOMAIN_LAN == Entry.Interface)
		{
			if (Entry.Enabled)
				lanRuleExistFlag = 1;

			continue;
		}

		if (Entry.web & 0x1)
		{
			Entry.web = 0;

			if (Entry.telnet || Entry.ftp || Entry.tftp || Entry.snmp || Entry.ssh || Entry.icmp)
				mib_chain_update(MIB_ACL_IP_TBL, (void *)&Entry, i);
			else
				mib_chain_delete(MIB_ACL_IP_TBL, i);
		}
	}

	if (httpEnable)
	{
		mib_get_s(MIB_ACL_CAPABILITY, (void *)&aclcap, sizeof(aclcap));
		if (0 == aclcap)
		{
			if (0 == lanRuleExistFlag)
			{
				memset(&Entry, 0, sizeof(MIB_CE_ACL_IP_T));
				Entry.Enabled = 1;
				Entry.Interface = IF_DOMAIN_LAN;
				Entry.any= 1;
				mib_chain_add(MIB_ACL_IP_TBL, (void *)&Entry);
			}
			aclcap = 1;
			mib_set(MIB_ACL_CAPABILITY, (void *)&aclcap);
		}

		memset(&Entry, 0, sizeof(MIB_CE_ACL_IP_T));
		Entry.Enabled = 1;
		Entry.Interface = IF_DOMAIN_WAN;
		Entry.web = 1;
		Entry.web_port = httpPort;
		mib_chain_add(MIB_ACL_IP_TBL, (void *)&Entry);
	}
#else
	if (httpEnable)
	{
		MIB_CE_ACC_T Entry;
		mib_chain_get(MIB_ACC_TBL, 0, (void *)&Entry);
		Entry.web |= 0x01;
		Entry.web_port = httpPort;
		mib_chain_update(MIB_ACC_TBL, (void *)&Entry, 0);
	}
	else
	{
		MIB_CE_ACC_T Entry;
		mib_chain_get(MIB_ACC_TBL, 0, (void *)&Entry);
		Entry.web_port = httpPort;
		Entry.web &= 0xFE;
		mib_chain_update(MIB_ACC_TBL, (void *)&Entry, 0);
	}
#endif
}

void rtk_fw_sync_from_rmtacc_to_wanAccess(void)
{
	int i, totalNum;
	unsigned char httpEnable=0;
	unsigned int  httpPort, sessionTime;

#ifdef IP_ACL
	MIB_CE_ACL_IP_T Entry;
	totalNum = mib_chain_total(MIB_ACL_IP_TBL);
	for (i=0; i<totalNum; i++)
	{
		if (!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&Entry))
			continue;

		if (IF_DOMAIN_LAN == Entry.Interface)
			continue;

		if (!Entry.Enabled)
			continue;

		if ((Entry.web & 0x1)
#ifdef ACL_IP_RANGE
			&& (*(unsigned int *)Entry.startipAddr == 0) && (*(unsigned int *)Entry.endipAddr == 0)
#endif
			&& (*(unsigned int *)Entry.ipAddr == 0)
			)
		{
			httpEnable = 1;
			httpPort = Entry.web_port;
			mib_set(MIB_RMTACC_HTTP_ENABLE, (void *)&httpEnable);
			mib_set(MIB_RMTACC_HTTP_WAN_PORT, (void *)&httpPort);

			break;
		}
	}
#else
	MIB_CE_ACC_T Entry;
	mib_chain_get(MIB_ACC_TBL, 0, (void *)&Entry);
	if (Entry.web & 0x1)
	{
		httpPort = Entry.web_port;
		httpEnable = 1;
		mib_set(MIB_RMTACC_HTTP_ENABLE, (void *)&httpEnable);
		mib_set(MIB_RMTACC_HTTP_WAN_PORT, (void *)&httpPort);
	}
	else
	{
		httpPort = Entry.web_port;
		httpEnable = 0;
		mib_set(MIB_RMTACC_HTTP_ENABLE, (void *)&httpEnable);
		mib_set(MIB_RMTACC_HTTP_WAN_PORT, (void *)&httpPort);
	}
#endif
}

#endif

#if !defined(CONFIG_E8B) && defined(IP_ACL) && defined(ACL_IP_RANGE)
int rtk_modify_ip_range_of_acl_default_rule()
{
	MIB_CE_ACL_IP_T entry;
	unsigned char enable_acl = 0;
	struct in_addr origIp,origMask;

	mib_get_s(MIB_ACL_CAPABILITY, (void *)&enable_acl, sizeof(enable_acl));

	if(!enable_acl){
		printf("acl is disabled!\n");
		return -1;
	}

	if(!mib_chain_total(MIB_ACL_IP_TBL)){
		printf("default acl mib entry isn't exist!\n");
		return -1;
	}

	memset(&entry, 0, sizeof(MIB_CE_ACL_IP_T));
	//get default acl rule mib entry
	mib_chain_get(MIB_ACL_IP_TBL, 0, (void *)&entry);

	if(entry.Enabled && entry.any && entry.Interface==IF_DOMAIN_LAN)
	{
		mib_get_s(MIB_ADSL_LAN_IP, (void *)&origIp, sizeof(origIp));
		mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)&origMask, sizeof(origMask));

		*(unsigned long *)entry.startipAddr = origIp.s_addr & origMask.s_addr;
		*(unsigned long *)entry.endipAddr = (origIp.s_addr & origMask.s_addr) | (0xFFFFFFFF ^ origMask.s_addr);

		mib_chain_update(MIB_ACL_IP_TBL, &entry, 0);
	}

	return 0;
}
#endif

#ifdef IP_ACL
#ifdef CONFIG_00R0
void syslogACLEntry(MIB_CE_ACL_IP_T Entry, int isAdd, char* user)
{
	char	*add;
	char message[1024];
	struct in_addr dest;
	char sdest[35];
	char service[128] ="";
	char port[64]="";
	char tmp_port[6]="";
	char ifname[IFNAMSIZ] = "";
	unsigned char testing_bit;

	memset(message,0,sizeof(message));

	if(isAdd==1)
		add =  (char *)ARG_ADD;
	else if(isAdd==2)
		add =  "update";
	else
		add =  (char *)ARG_DEL;

	dest.s_addr = *(unsigned long *)Entry.ipAddr;

	// inet_ntoa is not reentrant, we have to
	// copy the static memory before reuse it
	snprintf(sdest, sizeof(sdest), "%s/%d", inet_ntoa(dest), Entry.maskbit);

	if(Entry.Interface == DUMMY_IFINDEX)
		strcpy(ifname, "Any");
	else
		ifGetName(Entry.Interface, ifname, sizeof(ifname));

	testing_bit = 0x02;
	if (Entry.telnet & testing_bit) {
		strcat(service, "TELNET");
		snprintf(port, sizeof(port), "%hu", Entry.telnet_port);
	}
	else if (Entry.web & testing_bit) {
		strcat(service, "HTTP");
		snprintf(port, sizeof(port), "%hu", Entry.web_port);
	}
	else if (Entry.https & testing_bit) {
		strcat(service, "https,");
		snprintf(port, sizeof(port), "%hu", Entry.https_port);
	}
	else if (Entry.ssh & testing_bit) {
		strcat(service, "SSH");
		snprintf(port, sizeof(port), "%hu", Entry.ssh_port);
	}
	else if (Entry.icmp & testing_bit) {
		strcat(service, "ICMP");
		strcat(port, "N/A");
	}

	sprintf(message, "%s:%s, %s:%s, %s:%s, %s:%s, %s:%s",
		multilang(LANG_STATE), multilang(Entry.Enabled ? LANG_ENABLE : LANG_DISABLE),
		multilang(LANG_INTERFACE), ifname,
		multilang(LANG_IP_ADDRESS), sdest,
		multilang(LANG_SERVICES), service,
		multilang(LANG_PORT), port);

	syslog(LOG_INFO, "ACL: %s %s ACL rule. Rule: %s\n", user, add, message);
}

#else
void syslogACLEntry(MIB_CE_ACL_IP_T Entry, int isAdd, char* user)
{
	char	*add;
	char message[1024];
#ifndef ACL_IP_RANGE
	struct in_addr dest;
#endif
	char sdest[35];
	char service[128] ="";
	char port[64]="";
	char tmp_port[6]="";
	unsigned char testing_bit;

	memset(message,0,sizeof(message));

	if(isAdd==1)
		add =  (char *)ARG_ADD;
	else if(isAdd==2)
		add =  "update";
	else
		add =  (char *)ARG_DEL;

#ifdef ACL_IP_RANGE
	struct in_addr startip,endip;
	char sstart[16],send[16];

	startip.s_addr = *(uint32_t *)Entry.startipAddr;
	endip.s_addr = *(uint32_t *)Entry.endipAddr;

	strcpy(sstart, inet_ntoa(startip));
	strcpy(send, inet_ntoa(endip));
	strcpy(sdest, sstart);
	if(strcmp(sstart,send)){
		strcat(sdest, "-");
		strcat(sdest, send);
	}
#else
	dest.s_addr = *(in_addr_t *)Entry.ipAddr;

	// inet_ntoa is not reentrant, we have to
	// copy the static memory before reuse it
	snprintf(sdest, sizeof(sdest), "%s/%d", inet_ntoa(dest), Entry.maskbit);
#endif
	testing_bit = (Entry.Interface == IF_DOMAIN_WAN) ? 0x01 : 0x02;

	if (Entry.any == 0x01) { 	// service == any(0x01)
		strcat(service, "any");
		strcat(port, "--");
	} else {
#ifdef CONFIG_USER_TELNETD_TELNETD
		if (Entry.telnet & testing_bit) {
			strcat(service, "telnet,");
			strcat(port, "23,");
		}
#endif
#ifdef CONFIG_USER_FTPD_FTPD
		if (Entry.ftp & testing_bit) {
			strcat(service, "ftp,");
			strcat(port, "21,");
		}
#endif
#ifdef CONFIG_USER_TFTPD_TFTPD
		if (Entry.tftp & testing_bit) {
			strcat(service, "tftp,");
			strcat(port, "69,");
		}
#endif
		if (Entry.web & testing_bit) {
			strcat(service, "web,");
			strcat(port, "80,");
		}
#ifdef CONFIG_USER_BOA_WITH_SSL
		if (Entry.https & testing_bit) {
			strcat(service, "https,");
			strcat(port, "443,");
		}
#endif //end of CONFIG_USER_BOA_WITH_SSL
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
		if (Entry.snmp & testing_bit) {
			strcat(service, "snmp,");
			strcat(port, "161,162,");
		}
#endif
#ifdef CONFIG_USER_SSH_DROPBEAR
		if (Entry.ssh & testing_bit) {
			strcat(service, "ssh,");
			strcat(port, "22,");
		}
#endif
		if (Entry.icmp & testing_bit) {
			strcat(service, "ping,");
			strcat(port, "--,");
		}

		// trim ',' on service/port string.
		service[strlen(service) - 1] = '\0';
		port[strlen(port) - 1] = '\0';
	}
	sprintf(message, "%s:%s, %s:%s, %s:%s, %s:%s, %s:%s",
		multilang(LANG_STATE), multilang(Entry.Enabled ? LANG_ENABLE : LANG_DISABLE),
		multilang(LANG_INTERFACE),(Entry.Interface == IF_DOMAIN_LAN)? "LAN" : "WAN",
		multilang(LANG_IP_ADDRESS), sdest,
		multilang(LANG_SERVICES), service,
		multilang(LANG_PORT), port);

	syslog(LOG_INFO, "ACL: %s %s ACL rule. Rule: %s\n", user, add, message);
}
#endif	// CONFIG_00R0

#ifdef CONFIG_IPV6
void syslogACLv6Entry(MIB_CE_V6_ACL_IP_T Entry, int isAdd, char* user)
{
	char	*add;
	char service[128] ="";
	char port[64]="";
	char tmp_port[6]="";
	char srcip[55];
	char message[1024];
	int len;

	memset(message,0,sizeof(message));

	if(isAdd==1)
		add =  (char *)ARG_ADD;
	else if(isAdd==2)
		add =  "update";
	else
		add =  (char *)ARG_DEL;

	// source ip, prefixLen
	inet_ntop(PF_INET6, (struct in6_addr *)Entry.sip6Start, srcip, 48);
	if (Entry.sip6PrefixLen!=0) {
		len = sizeof(srcip) - strlen(srcip);
		snprintf(srcip + strlen(srcip), len, "/%d", Entry.sip6PrefixLen);
	}

	if (Entry.any == 0x01) { 	// service == any(0x01)
		strcat(service, "any");
		strcat(port, "--");
	}
	else {
#ifdef CONFIG_USER_TELNETD_TELNETD
		if ((Entry.telnet & 0x01) || (Entry.telnet & 0x02)) {
			strcat(service, "telnet,");
			strcat(port, "23,");
		}
#endif
#ifdef CONFIG_USER_FTPD_FTPD
		if ((Entry.ftp & 0x01) || (Entry.ftp & 0x02)) {
			strcat(service, "ftp,");
			strcat(port, "21,");
		}
#endif
#ifdef CONFIG_USER_TFTPD_TFTPD
		if ((Entry.tftp & 0x01) || (Entry.tftp & 0x02)) {
			strcat(service, "tftp,");
			strcat(port, "69,");
		}
#endif
		if ((Entry.web & 0x01) || (Entry.web & 0x02)) {
			strcat(service, "web,");
			strcat(port, "80,");
		}
#ifdef CONFIG_USER_BOA_WITH_SSL
		if ((Entry.https & 0x01) || (Entry.https & 0x02)) {
			strcat(service, "https,");
			strcat(port, "443,");
		}
#endif
#ifdef CONFIG_USER_SSH_DROPBEAR
		if ((Entry.ssh & 0x01) || (Entry.ssh & 0x02)) {
			strcat(service, "ssh,");
			strcat(port, "22,");
		}
#endif

		// LAN side access is always enabled
		if (Entry.Interface == IF_DOMAIN_WAN) {
			if (Entry.icmp & 0x01)
				strcat(service, "ping,");
		}
		else
			strcat(service, "ping,");

		// trim ',' on service/port string.
		service[strlen(service) - 1] = '\0';
		port[strlen(port) - 1] = '\0';
	}

	sprintf(message, "%s:%s, %s:%s, %s:%s, %s:%s, %s:%s",
		multilang(LANG_STATE), multilang(Entry.Enabled ? LANG_ENABLE : LANG_DISABLE),
		multilang(LANG_INTERFACE),(Entry.Interface == IF_DOMAIN_LAN)? "LAN" : "WAN",
		multilang(LANG_IP_ADDRESS), srcip,
		multilang(LANG_SERVICES), service,
		multilang(LANG_PORT), port);

	syslog(LOG_INFO, "ACL: %s %s ACLv6 rule. Rule: %s\n", user, add, message);
}
#endif //CONFIG_IPV6
#endif //IP_ACL


#ifdef _CWMP_MIB_
void filter_set_cwmp_acl_service(int enable, char *strIP, char *strPort)
{
	char *act;

	if (enable)
		act = (char *)FW_ADD;
	else
		act = (char *)FW_DEL;

	va_cmd(IPTABLES, 17, 1, ARG_T, "mangle", "-A", (char *)FW_CWMP_ACL,
		"!", (char *)ARG_I, (char *)LANIF, "-s", strIP, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
}

void filter_set_cwmp_acl(int enable)
{
	int i, total;
	struct in_addr src;
	char ssrc[40];
	MIB_CWMP_ACL_IP_T Entry;
	unsigned char dhcpMode;
	unsigned char vChar;
	char strPort[16];
	unsigned int conreq_port=0;

	// check if ACL Capability is enabled ?
	if (!enable) {
		va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_CWMP_ACL);
		// Flush all rule in nat table
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)FW_CWMP_ACL);
	}
	else {
		// Get TR069 port number
		mib_get_s(CWMP_CONREQ_PORT, (void *)&conreq_port, sizeof(conreq_port));
		if(conreq_port==0) conreq_port=7547;
		sprintf(strPort, "%u", conreq_port );

		// Add policy to cwmp_aclblock chain
		total = mib_chain_total(MIB_CWMP_ACL_IP_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_CWMP_ACL_IP_TBL, i, (void *)&Entry))
			{
				return;
			}

//    		printf("\nstartip=%x,endip=%x\n",*(unsigned long*)Entry.startipAddr,*(unsigned long*)Entry.endipAddr);

			src.s_addr = *(unsigned long *)Entry.ipAddr;

			// inet_ntoa is not reentrant, we have to
			// copy the static memory before reuse it
			snprintf(ssrc, sizeof(ssrc), "%s/%hhu", inet_ntoa(src), Entry.maskbit);
			filter_set_cwmp_acl_service(enable, ssrc, strPort);
		}

		// allow for WAN
		// allow packet with the same mark value(0x1000) from WAN
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_CWMP_ACL,
			"!", (char *)ARG_I, LANIF, "-m", "mark", "--mark", RMACC_MARK, "-j", FW_ACCEPT);

		// iptables -A cwmp_aclblock ! -i br0 -p tcp --dport 7547 -j DROP
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_CWMP_ACL, "!", (char *)ARG_I, (char *)LANIF,
			"-p", (char *)ARG_TCP, (char *)FW_DPORT, strPort, "-j", (char *)FW_DROP);
	}
}

int restart_cwmp_acl(void)
{
	unsigned char cwmp_aclEnable;
	char msg[100];
	char strPort[16];
	unsigned int conreq_port=0;
	int i = 0, total = 0;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ] = {0};

	mib_get_s(CWMP_ACL_ENABLE, &cwmp_aclEnable, sizeof(cwmp_aclEnable));
	filter_set_cwmp_acl(0);
	if (cwmp_aclEnable == 1)  // CWMP ACL Capability is enabled
		filter_set_cwmp_acl(1);
	else
	{
		/*add a wan port to pass */
		mib_get_s(CWMP_CONREQ_PORT, (void *)&conreq_port, sizeof(conreq_port));
		if(conreq_port==0) conreq_port=7547;
		sprintf(strPort, "%u", conreq_port );
		total = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				return -1;
			if (Entry.cmode == CHANNEL_MODE_BRIDGE)
				continue;
			if (Entry.applicationtype & X_CT_SRV_TR069)
			{
				ifGetName(Entry.ifIndex, ifname, sizeof(ifname));
				va_cmd(IPTABLES, 14, 1, ARG_T, "mangle", (char *)FW_ADD, (char *)FW_CWMP_ACL,
						(char *)ARG_I, ifname, "-p", (char *)ARG_TCP,
						(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
			}
		}
	}
	return 0;
}
#endif


#ifdef TCP_UDP_CONN_LIMIT
void set_conn_limit(void)
{
	int i, total;
	char ssrc[20];
	char connNum[10];
	MIB_CE_CONN_LIMIT_T Entry;

	// Add policy to connlimit chain
	//iptables -A connlimit -m state --state RELATED,ESTABLISHED -j RETURN
#if defined(CONFIG_00R0)
	char ipfilter_spi_state=1;
	mib_get_s(MIB_IPFILTER_SPI_ENABLE, (void*)&ipfilter_spi_state, sizeof(ipfilter_spi_state));
	if(ipfilter_spi_state != 0)
#endif
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, "connlimit", "-m", "state",
		"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_RETURN);

	total = mib_chain_total(MIB_CONN_LIMIT_TBL);
	for (i=0; i<total; i++) {
		if (!mib_chain_get(MIB_CONN_LIMIT_TBL, i, (void *)&Entry))
			continue;

		// Check if this entry is enabled
		if ( Entry.Enabled == 1 ) {
			// inet_ntoa is not reentrant, we have to
			// copy the static memory before reuse it
			strncpy(ssrc, inet_ntoa(*((struct in_addr *)&Entry.ipAddr)), 16);
			ssrc[15] = '\0';

			snprintf(connNum, 10, "%d", Entry.connNum);

			// iptables -A connlimit -i br0 -s xxx.xxx.xxx.xxx
			va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "connlimit", "-s", ssrc, "-m", "iplimit",
				"--iplimit-above", connNum, "--iplimit-mask", "255.255.255.255", "-j", "REJECT");
		}
	}
}
//#endif

int restart_connlimit(void)
{
	unsigned char connlimitEn;

	va_cmd(IPTABLES, 2, 1, "-F", "connlimit");

	mib_get_s(MIB_CONNLIMIT_ENABLE, (void *)&connlimitEn, sizeof(connlimitEn));
	if (connlimitEn == 1)
		set_conn_limit();
}

void set_conn_limit(void)
{
	int 					i, total;
	char 				ssrc[20];
	char 				connNum[10];
	MIB_CE_TCP_UDP_CONN_LIMIT_T Entry;

	// Add policy to connlimit chain, allow all established ~
	//iptables -A connlimit -m state --state RELATED,ESTABLISHED -j RETURN
#if defined(CONFIG_00R0)
	char ipfilter_spi_state=1;
	mib_get_s(MIB_IPFILTER_SPI_ENABLE, (void*)&ipfilter_spi_state, sizeof(ipfilter_spi_state));
	if(ipfilter_spi_state != 0)
#endif
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, "connlimit", "-m", "state",
		"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_RETURN);

	total = mib_chain_total(MIB_TCP_UDP_CONN_LIMIT_TBL);
	for (i=0; i<total; i++) {
		if (!mib_chain_get(MIB_TCP_UDP_CONN_LIMIT_TBL, i, (void *)&Entry))
			continue;
		// Check if this entry is enabled and protocol is TCP
		if (( Entry.Enabled == 1 ) && ( Entry.protocol == 0 )) {
			// inet_ntoa is not reentrant, we have to
			// copy the static memory before reuse it
			strncpy(ssrc, inet_ntoa(*((struct in_addr *)&Entry.ipAddr)), 16);
			ssrc[15] = '\0';
			snprintf(connNum, 10, "%d", Entry.connNum);

			// iptables -A connlimit -p tcp -s 192.168.1.99 -m iplimit --iplimit-above 2 -j REJECT
			va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "connlimit","-p","tcp", "-s", ssrc, "-m", "iplimit",
				"--iplimit-above", connNum, "-j", "REJECT");
		}
		// UDP
		else if  (( Entry.Enabled == 1 ) && ( Entry.protocol == 1 )) {
			strncpy(ssrc, inet_ntoa(*((struct in_addr *)&Entry.ipAddr)), 16);
			ssrc[15] = '\0';
			snprintf(connNum, 10, "%d", Entry.connNum);

			//iptables -A connlimit -p udp -s 192.168.1.99 -m udplimit --udplimit-above 2 -j REJECT
			va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "connlimit","-p","udp", "-s", ssrc, "-m", "udplimit",
				"--udplimit-above", connNum, "-j", "REJECT");
		}

	}

	//The Global rules goes last ~
	//TCP
	mib_get_s(MIB_CONNLIMIT_TCP, (void *)&i, sizeof(i));
	if (i >0)
	{
		snprintf(connNum, 10, "%d", i);
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, "connlimit","-p","tcp",  "-m", "iplimit",
					"--iplimit-above", connNum, "-j", "REJECT");
	}
	//UDP
	mib_get_s(MIB_CONNLIMIT_UDP, (void *)&i, sizeof(i));
	if (i >0)
	{
		snprintf(connNum, 10, "%d", i);
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, "connlimit","-p","udp",  "-m", "udplimit",
					"--udplimit-above", connNum, "-j", "REJECT");
	}

}
#endif//TCP_UDP_CONN_LIMIT

unsigned int rtk_transfer2IfIndxfromIfName( char *ifname )
{
	MIB_CE_ATM_VC_T *pEntry,vc_entity;
	int total,i;
	unsigned int ifindex=DUMMY_IFINDEX;

	if(ifname==NULL) return ifindex;
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for( i=0; i<total; i++ )
	{
		char tmp_ifname[32];
		pEntry = &vc_entity;
		if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)pEntry ) )
			continue;

		//ifGetName(tmp_ifname, pEntry->ifIndex, sizeof(tmp_ifname));
		ifGetName(pEntry->ifIndex, tmp_ifname, sizeof(tmp_ifname));

		if( strcmp(ifname, tmp_ifname)==0 )
		{
			ifindex=pEntry->ifIndex;
			break;
		}
	}

	return ifindex;
}

#ifdef CONFIG_IPV6
void ipv6_route_addr_format_transfer(char *src, char *dst)
{
	unsigned char buf[sizeof(struct in6_addr)] = {0};
	int s = 0;

	if (!src || !dst)
		return;

	s = inet_pton(AF_INET6, src, buf);
	if (s <= 0)
	{
		if (s == 0)
			fprintf(stderr, "Not in presentation format");

		return;
	}

	if (inet_ntop(AF_INET6, buf, dst, INET6_ADDRSTRLEN) == NULL)
		fprintf(stderr, "inet_ntop error");

	 return;
}
#endif

/***port forwarding APIs*******/
#ifdef PORT_FORWARD_GENERAL
/*move from startup.c:iptable_fw()*/
#if defined(CONFIG_RTK_DEV_AP)
static void iptable_fw( int del, int negate, const char *ifname, const char *proto, const char *remoteIP, const char *extPort, const char *dstIP, MIB_CE_PORT_FW_T *pentry)
#else
static void iptable_fw( int del, int negate, const char *ifname, const char *proto, const char *remoteIP, const char *extPort, const char *dstIP)
#endif
{
	char *act;

	if(del) act = (char *)FW_DEL;
	else act = (char *)FW_ADD;


//	if (negate && remoteIP) {
	if (negate && remoteIP && extPort) {
		va_cmd(IPTABLES, 17, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			 "!", (char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			"-s", (char *)remoteIP,
			(char *)FW_DPORT, extPort, "-j",
			"DNAT", "--to-destination", dstIP);
	} else if (negate && remoteIP) {
		va_cmd(IPTABLES, 15, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			 "!", (char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			"-s", (char *)remoteIP,
			"-j", "DNAT", "--to-destination", dstIP);
//	} else if (negate) {
	} else if (negate && extPort) {
		va_cmd(IPTABLES, 15, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			 "!", (char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			(char *)FW_DPORT, extPort, "-j",
			"DNAT", "--to-destination", dstIP);
//	} else if (remoteIP) {
	} else if (remoteIP && extPort) {
		va_cmd(IPTABLES, 16, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			(char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			"-s", (char *)remoteIP,
			(char *)FW_DPORT, extPort, "-j",
			"DNAT", "--to-destination", dstIP);
	} else if (negate) {
		va_cmd(IPTABLES, 13, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			 "!", (char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			"-j", "DNAT", "--to-destination", dstIP);
	} else if (remoteIP) {
		va_cmd(IPTABLES, 14, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			(char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			"-s", (char *)remoteIP,
			"-j", "DNAT", "--to-destination", dstIP);
//	} else {
	} else if (extPort) {
		va_cmd(IPTABLES, 14, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			(char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			(char *)FW_DPORT, extPort, "-j",
			"DNAT", "--to-destination", dstIP);
	} else {
		va_cmd(IPTABLES, 12, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			(char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			 "-j", "DNAT", "--to-destination", dstIP);
	}

#if defined(CONFIG_RTK_DEV_AP)
	if(pentry && (pentry->fromPort <= 1723 &&  pentry->toPort >= 1723) && !strncmp(proto, "TCP", 3)){
		if (remoteIP) {
			if (negate){
				va_cmd(IPTABLES, 15, 1, "-t", "nat",
					(char *)act,	(char *)PORT_FW,
					 "!", (char *)ARG_I, (char *)ifname,
					"-p", "47",
					"-s", (char *)remoteIP, "-j",
					"DNAT", "--to", inet_ntoa(*((struct in_addr *)pentry->ipAddr)));

			}else{
				va_cmd(IPTABLES, 14, 1, "-t", "nat",
				(char *)act,	(char *)PORT_FW,
				(char *)ARG_I, (char *)ifname,
				"-p", "47",
				"-s", (char *)remoteIP,
				"-j", "DNAT", "--to", inet_ntoa(*((struct in_addr *)pentry->ipAddr)));
				}
		}
		else{
			if (negate){
				va_cmd(IPTABLES, 13, 1, "-t", "nat",
					(char *)act,	(char *)PORT_FW,
					 "!", (char *)ARG_I, (char *)ifname,
					"-p", "47", "-j",
					"DNAT", "--to", inet_ntoa(*((struct in_addr *)pentry->ipAddr)));

			}else{
				va_cmd(IPTABLES, 12, 1, "-t", "nat",
				(char *)act,	(char *)PORT_FW,
				(char *)ARG_I, (char *)ifname,
				"-p", "47",
				"-j", "DNAT", "--to", inet_ntoa(*((struct in_addr *)pentry->ipAddr)));
				}
			}
		}

#endif

}

/*move from startup.c:iptable_filter()*/
#if defined(CONFIG_RTK_DEV_AP)
static void iptable_filter( int del, int negate, const char *ifname, const char *proto, const char *remoteIP, const char *intPort, MIB_CE_PORT_FW_T *pentry)
#else
static void iptable_filter( int del, int negate, const char *ifname, const char *proto, const char *remoteIP, const char *intPort)
#endif
{
	char *act, *rulenum;

	if(del)
	{
		act = (char *)FW_DEL;

//		if (negate && remoteIP) {
		if (negate && remoteIP && intPort) {
			va_cmd(IPTABLES, 15, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				(char *)FW_DPORT, intPort,
				"-j",(char *)FW_ACCEPT);
		} else if (negate && remoteIP) {
			va_cmd(IPTABLES, 13, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				"-j",(char *)FW_ACCEPT);
//		} else if (negate) {
		} else if (negate && intPort) {
			va_cmd(IPTABLES, 13, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				(char *)FW_DPORT, intPort,
				"-j",(char *)FW_ACCEPT);
//		} else if (remoteIP) {
		} else if (remoteIP && intPort) {
			va_cmd(IPTABLES, 14, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				(char *)FW_DPORT, intPort,
				"-j",(char *)FW_ACCEPT);
		} else if (negate) {
			va_cmd(IPTABLES, 11, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-j",(char *)FW_ACCEPT);
		} else if (remoteIP) {
			va_cmd(IPTABLES, 12, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				"-j",(char *)FW_ACCEPT);
//		} else {
		} else if (intPort) {
			va_cmd(IPTABLES, 12, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				(char *)FW_DPORT, intPort, "-j",
				(char *)FW_ACCEPT);
		} else {
			va_cmd(IPTABLES, 10, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-j", (char *)FW_ACCEPT);
		}
	}else
	{
 		act = (char *)FW_ADD;

//		if (negate && remoteIP) {
		if (negate && remoteIP && intPort) {
			va_cmd(IPTABLES, 15, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				(char *)FW_DPORT, intPort,
				"-j",(char *)FW_ACCEPT);
		} else if (negate && remoteIP) {
			va_cmd(IPTABLES, 13, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				"-j",(char *)FW_ACCEPT);
//		} else if (negate) {
		} else if (negate && intPort) {
			va_cmd(IPTABLES, 13, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				(char *)FW_DPORT, intPort,
				"-j",(char *)FW_ACCEPT);
//		} else if (remoteIP) {
		} else if (remoteIP && intPort) {
			va_cmd(IPTABLES, 14, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				(char *)FW_DPORT, intPort,
				"-j",(char *)FW_ACCEPT);
		} else if (negate) {
			va_cmd(IPTABLES, 11, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-j",(char *)FW_ACCEPT);
		} else if (remoteIP) {
			va_cmd(IPTABLES, 12, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				"-j",(char *)FW_ACCEPT);
//		} else {
		} else if (intPort) {
			va_cmd(IPTABLES, 12, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				(char *)FW_DPORT, intPort, "-j",
				(char *)FW_ACCEPT);
		} else {
			va_cmd(IPTABLES, 10, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-j", (char *)FW_ACCEPT);
		}
	}

	#if defined(CONFIG_RTK_DEV_AP)
	if(del) act = (char *)FW_DEL;
	else act = (char *)FW_ADD;
	if(pentry && (pentry->fromPort <= 1723 &&  pentry->toPort >= 1723)&&!strncmp(proto, "TCP", 3)){
		if (remoteIP) {
			if (negate){
				va_cmd(IPTABLES, 11, 1,
					(char *)act,	(char *)PORT_FW,
					 "!", (char *)ARG_I, (char *)ifname,
					"-p", "47",
					"-s", (char *)remoteIP, "-j",(char *)FW_ACCEPT);

			}else{
				va_cmd(IPTABLES, 10, 1,
				(char *)act,	(char *)PORT_FW,
				(char *)ARG_I, (char *)ifname,
				"-p", "47",
				"-s", (char *)remoteIP,
				"-j",(char *)FW_ACCEPT);
				}
		}
		else{
			if (negate){
				va_cmd(IPTABLES, 9, 1,
					(char *)act,	(char *)PORT_FW,
					 "!", (char *)ARG_I, (char *)ifname,
					"-p", "47", "-j",(char *)FW_ACCEPT);

			}else{
				va_cmd(IPTABLES, 8, 1,
				(char *)act,	(char *)PORT_FW,
				(char *)ARG_I, (char *)ifname,
				"-p", "47",
				"-j",(char *)FW_ACCEPT);
				}
			}
		}
	#endif

}

/*move from startup.c ==> part of setupFirewall()*/
void portfw_modify( MIB_CE_PORT_FW_T *p, int del)
{
	int negate=0, hasRemote=0, hasLocalPort=0, hasExtPort=0, shift_portmap_range = 0;
	char * proto = 0;
	char intPort[32], extPort[32];
	char ipaddr[32], extra[32], ifname[IFNAMSIZ], ipaddr_shift_portmap[64] = {0};

	if(p==NULL) return;

#if 0
	{
		fprintf( stderr,"<portfw_modify>\n" );
		fprintf( stderr,"\taction:%s\n", (del==0)?"ADD":"DEL" );
		fprintf( stderr,"\tifIndex:0x%x\n", p->ifIndex );
		fprintf( stderr,"\tenable:%u\n", p->enable );
		fprintf( stderr,"\tleaseduration:%u\n", p->leaseduration );
		fprintf( stderr,"\tremotehost:%s\n", inet_ntoa(*((struct in_addr *)p->remotehost))  );
		fprintf( stderr,"\texternalport:%u\n", p->externalport );
		fprintf( stderr,"\tinternalclient:%s\n", inet_ntoa(*((struct in_addr *)p->ipAddr)) );
		fprintf( stderr,"\tinternalport:%u\n", p->toPort );
		fprintf( stderr,"\tprotocol:%u\n", p->protoType ); /*PROTO_TCP=1, PROTO_UDP=2*/
		fprintf( stderr,"<end portfw_modify>\n" );
	}
#endif

	if( del==0 ) //add
	{
		char vCh=0;
		mib_get_s(MIB_PORT_FW_ENABLE, (void *)&vCh, sizeof(vCh));
		if(vCh==0) return;

		if (!p->enable) return;
	}

/*	snprintf(intPort, 12, "%u", p->fromPort);

	if (p->externalport) {
		snprintf(extPort, sizeof(extPort), "%u", p->externalport);
		snprintf(intPort, sizeof(intPort), "%u", p->fromPort);
		snprintf(ipaddr, sizeof(ipaddr), "%s:%s", inet_ntoa(*((struct in_addr *)p->ipAddr)), intPort);
	} else {
		snprintf(intPort, sizeof(extPort), "%u", p->fromPort);
		strncpy(extPort, intPort, sizeof(intPort));
		strncpy(ipaddr, inet_ntoa(*((struct in_addr *)p->ipAddr)), 16);
	}
	*/
    if (p->fromPort)
    {
        if (p->fromPort == p->toPort)
        {
            snprintf(intPort, sizeof(intPort), "%u", p->fromPort);
        }
        else
        {
            /* "%u-%u" is used by port forwarding */
            snprintf(intPort, sizeof(intPort), "%u-%u", p->fromPort, p->toPort);
        }

        snprintf(ipaddr, sizeof(ipaddr), "%s:%s", inet_ntoa(*((struct in_addr *)p->ipAddr)), intPort);

        if (p->fromPort != p->toPort)
        {
            /* "%u:%u" is used by filter */
            snprintf(intPort, sizeof(intPort), "%u:%u", p->fromPort, p->toPort);
        }
        hasLocalPort = 1;
    }
    else
    {
        snprintf(ipaddr, sizeof(ipaddr), "%s", inet_ntoa(*((struct in_addr *)p->ipAddr)));
        hasLocalPort = 0;
    }

	if (p->externalfromport && p->externaltoport && (p->externalfromport != p->externaltoport)) {
		snprintf(extPort, sizeof(extPort), "%u:%u", p->externalfromport, p->externaltoport);
		hasExtPort = 1;
	} else if (p->externalfromport) {
		snprintf(extPort, sizeof(extPort), "%u", p->externalfromport);
		hasExtPort = 1;
	} else if (p->externaltoport) {
		snprintf(extPort, sizeof(extPort), "%u", p->externaltoport);
		hasExtPort = 1;
	} else {
		hasExtPort = 0;
	}
	//printf( "extPort:%s hasExtPort=%d\n",  extPort, hasExtPort);
	//printf( "entry.externalfromport:%d entry.externaltoport=%d\n",  p->externalfromport, p->externaltoport);

#if defined(CONFIG_00R0)
	if ((hasLocalPort && (p->fromPort != p->toPort)) &&
		(hasExtPort && (p->externalfromport != p->externaltoport)) &&
		((p->toPort - p->fromPort) == (p->externaltoport - p->externalfromport)))
	{
		shift_portmap_range = 1;
		snprintf(ipaddr_shift_portmap, sizeof(ipaddr_shift_portmap), "%s/%u", ipaddr, p->externalfromport);
	}
#endif

	if (ifGetName(p->ifIndex, ifname, sizeof(ifname)))
		negate = 0;
	else {
		strcpy(ifname, LANIF);
		negate = 1;
	}

	if (p->remotehost[0]) {
		snprintf(extra, sizeof(extra), "%s", inet_ntoa(*((struct in_addr *)p->remotehost)));
		hasRemote = 1;
	} else {
		hasRemote = 0;
	}

	//fprintf( stderr, "ipaddr:%s, intPort:%s\n", ipaddr, intPort );
	//check something
	//internalclient can't be zeroip
	if( strncmp(ipaddr,"0.0.0.0", 7)==0 ) return;
	//internalport can't be zero
//	if( strcmp(intPort,"0")==0 ) return;
	//fprintf( stderr, "Pass ipaddr:%s, intPort:%s\n", ipaddr, intPort );

	if (p->protoType == PROTO_TCP || p->protoType == PROTO_UDPTCP)
	{
	#if defined(CONFIG_RTK_DEV_AP)
		iptable_fw( del, negate, ifname, ARG_TCP, hasRemote ? extra : 0, hasExtPort ? extPort : 0, ipaddr, p);
		iptable_filter( del, negate, ifname, ARG_TCP, hasRemote ? extra : 0, hasLocalPort ? intPort : 0, p);
#ifdef PORT_FORWARD_GENERAL
#ifdef NAT_LOOPBACK
		iptable_fw_natLB( del, p->ifIndex, ARG_TCP, hasExtPort ? extPort : 0, hasLocalPort ? intPort : 0, ipaddr,p);
#endif
#endif

	#else
		// iptables -t nat -A PREROUTING -i ! $LAN_IF -p TCP --dport dstPortRange -j DNAT --to-destination ipaddr
		//		iptable_fw( del, negate, ifname, ARG_TCP, hasRemote ? extra : 0, extPort, ipaddr);
		iptable_fw( del, negate, ifname, ARG_TCP, hasRemote ? extra : 0, hasExtPort ? extPort : 0, shift_portmap_range ? ipaddr_shift_portmap : ipaddr);
		// iptables -I ipfilter 3 -i ! $LAN_IF -o $LAN_IF -p TCP --dport dstPortRange -j RETURN
//		iptable_filter( del, negate, ifname, ARG_TCP, hasRemote ? extra : 0, intPort);
		iptable_filter( del, negate, ifname, ARG_TCP, hasRemote ? extra : 0, hasLocalPort ? intPort : 0);
#ifdef PORT_FORWARD_GENERAL
#ifdef NAT_LOOPBACK
		iptable_fw_natLB( del, p->ifIndex, ARG_TCP, hasExtPort ? extPort : 0, hasLocalPort ? intPort : 0, shift_portmap_range ? ipaddr_shift_portmap : ipaddr);
#endif
#endif
	#endif
	}

	if (p->protoType == PROTO_UDP || p->protoType == PROTO_UDPTCP)
	{
#if defined(CONFIG_RTK_DEV_AP)
		iptable_fw( del, negate, ifname, ARG_UDP, hasRemote ? extra : 0, hasExtPort ? extPort : 0, ipaddr, p);
		iptable_filter( del, negate, ifname, ARG_UDP, hasRemote ? extra : 0, hasLocalPort ? intPort : 0, p);
#ifdef PORT_FORWARD_GENERAL
#ifdef NAT_LOOPBACK
		iptable_fw_natLB( del, p->ifIndex, ARG_UDP, hasExtPort ? extPort : 0, hasLocalPort ? intPort : 0, ipaddr, p);
#endif
#endif
#else
//		iptable_fw( del, negate, ifname, ARG_UDP, hasRemote ? extra : 0, extPort, ipaddr);
		iptable_fw( del, negate, ifname, ARG_UDP, hasRemote ? extra : 0, hasExtPort ? extPort : 0, shift_portmap_range ? ipaddr_shift_portmap : ipaddr);
		// iptables -I ipfilter 3 -i ! $LAN_IF -o $LAN_IF -p UDP --dport dstPortRange -j RETURN
//		iptable_filter( del, negate, ifname, ARG_UDP, hasRemote ? extra : 0, intPort);
		iptable_filter( del, negate, ifname, ARG_UDP, hasRemote ? extra : 0, hasLocalPort ? intPort : 0);
#ifdef PORT_FORWARD_GENERAL
#ifdef NAT_LOOPBACK
		iptable_fw_natLB( del, p->ifIndex, ARG_UDP, hasExtPort ? extPort : 0, hasLocalPort ? intPort : 0, shift_portmap_range ? ipaddr_shift_portmap : ipaddr);
#endif
#endif
#endif
	}
}
#endif // of PORT_FORWARD_GENERAL
/***end port forwarding APIs*******/

#ifdef IP_PASSTHROUGH
void set_IPPT_LAN_access()
{
	unsigned int ippt_itf;
	unsigned char value[32];

	// IP Passthrough, LAN access
	if (mib_get_s(MIB_IPPT_ITF, (void *)&ippt_itf, sizeof(ippt_itf)) != 0)
		if (ippt_itf != DUMMY_IFINDEX) {	// IP passthrough
			mib_get_s(MIB_IPPT_LANACC, (void *)value, sizeof(value));
			if (value[0] == 0)	// disable LAN access
				// iptables -A FORWARD -i $LAN_IF -o $LAN_IF -j DROP
				va_cmd(IPTABLES, 8, 1, (char *)FW_ADD,
					(char *)FW_FORWARD, (char *)ARG_I,
					(char *)LANIF, (char *)ARG_O,
					(char *)LANIF, "-j", (char *)FW_DROP);
		}
}

void clean_IPPT_LAN_access()
{
	// iptables -D FORWARD -i $LAN_IF -o $LAN_IF -j DROP
	va_cmd(IPTABLES, 8, 1, (char *)FW_DEL,
		(char *)FW_FORWARD, (char *)ARG_I,
		(char *)LANIF, (char *)ARG_O,
		(char *)LANIF, "-j", (char *)FW_DROP);
}

// Mason Yu
void restartIPPT(struct ippt_para para)
{
	unsigned int entryNum, i, idx, vcIndex, selected;
	MIB_CE_ATM_VC_T Entry;
	FILE *fp;
#ifdef CONFIG_USER_DHCP_SERVER
	int restar_dhcpd_flag=0;
#endif
	int isPPPoE=0;
	unsigned long myipaddr, hisipaddr;
	char pppif[6], globalIP_str[16];
	MEDIA_TYPE_T mType;

	//printf("Take effect for IPPT and old_ippt_itf=%d new_ippt_itf=%d\n", para.old_ippt_itf, para.new_ippt_itf);
	//printf("Take effect for IPPT and old_ippt_lease=%d new_ippt_lease=%d\n", para.old_ippt_lease, para.new_ippt_lease);
	//printf("Take effect for IPPT and old_ippt_lanacc=%d new_ippt_lanacc=%d\n", para.old_ippt_lanacc, para.new_ippt_lanacc);

       	// Stop IPPT
       	// If old_ippt_itf != 255 and new_ippt_itf != old_ippt_itf, it is that the IPPT is enabled. We should clear some configurations.
	if ( para.old_ippt_itf != DUMMY_IFINDEX  && para.new_ippt_itf != para.old_ippt_itf) {
		#ifdef CONFIG_USER_DHCP_SERVER
		// (1)  set restart DHCP server flag with 1.
		restar_dhcpd_flag = 1;  // on restart DHCP server flag
		#endif

		// (2) Delete /tmp/PPPHalfBridge file for DHCP Server
       		fp = fopen("/tmp/PPPHalfBridge", "r");
       		if (fp) {
			if(fread(&myipaddr, 4, 1, fp) != 1)
				printf("Reading error: %s %d\n", __func__, __LINE__);
			if(fread(&hisipaddr, 4, 1, fp) != 1)
				printf("Reading error: %s %d\n", __func__, __LINE__);
       			unlink("/tmp/PPPHalfBridge");
       			fclose(fp);
       		}

       		// (3) Delete /tmp/IPoAHalfBridge file for DHCP Server
       		fp = fopen("/tmp/IPoAHalfBridge", "r");
       		if (fp) {
			if(fread(&myipaddr, 4, 1, fp) != 1)
				printf("Reading error: %s %d\n", __func__, __LINE__);
			if(fread(&hisipaddr, 4, 1, fp) != 1)
				printf("Reading error: %s %d\n", __func__, __LINE__);
       			unlink("/tmp/IPoAHalfBridge");
       			fclose(fp);
       		}

		// Change Public IP to string
		snprintf(globalIP_str, 16, "%d.%d.%d.%d", (int)(myipaddr>>24)&0xff, (int)(myipaddr>>16)&0xff, (int)(myipaddr>>8)&0xff, (int)(myipaddr)&0xff);

		// (4) Delete LAN IPPT interface
		va_cmd(IFCONFIG, 2, 1, (char*)LAN_IPPT,"down");

		// (5) Delete a public IP's route
               	va_cmd(ROUTE, 5, 1, ARG_DEL, "-host", globalIP_str, "dev", LANIF);

		// (6) Restart the previous IPPT WAN connection.
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < entryNum; i++) {
			/* Retrieve entry */
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
				printf("restartIPPT: cannot get ATM_VC_TBL-1(ch=%d) entry\n", i);
				return;
			}

			/* remove connection on driver*/
			if (para.old_ippt_itf == Entry.ifIndex) {
				stopConnection(&Entry);

				// If this connection is PPPoE/oA, we should kill the old NAPT rule in POSTROUTING chain.
				// When channel mode is rt1483, NAPT rule will be deleted by stopConnection().
				if (Entry.cmode == CHANNEL_MODE_PPPOE || Entry.cmode == CHANNEL_MODE_PPPOA) {
					snprintf(pppif, 6, "ppp%u", PPP_INDEX(Entry.ifIndex));
        				va_cmd(IPTABLES, 10, 1, "-t", "nat", FW_DEL, "POSTROUTING",
			 			"-o", pppif, "-j", "SNAT", "--to-source", globalIP_str);
				}

				if (Entry.cmode == CHANNEL_MODE_PPPOE) {
					isPPPoE = 1;
					vcIndex = VC_INDEX(Entry.ifIndex);
					mType = MEDIA_INDEX(Entry.ifIndex);
					selected = i;
				}

				restartWAN(CONFIGONE, &Entry);
				break;
			}
		}
	}

	if (isPPPoE) {
		for (i=0; i<entryNum; i++) {
			if (i == selected)
				continue;
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
				printf("restartIPPT: cannot get ATM_VC_TBL-1(ch=%d) entry\n", i);
				return;
			}
			if (mType == MEDIA_INDEX(Entry.ifIndex) && vcIndex == VC_INDEX(Entry.ifIndex)) {	// Jenny, for multisession PPPoE support
				restartWAN(CONFIGONE, &Entry);
			}
		}
	}

	// Start New IPPT
	if ( para.new_ippt_itf != DUMMY_IFINDEX && para.new_ippt_itf != para.old_ippt_itf) {
		#ifdef CONFIG_USER_DHCP_SERVER
		// (1)  set restart DHCP server flag with 1.
		restar_dhcpd_flag = 1;  // on restart DHCP server flag
		#endif

		// (2) Config WAN interface and reconnect to DSL.
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < entryNum; i++) {
			/* Retrieve entry */
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
				printf("restartIPPT: cannot get ATM_VC_TBL-2(ch=%d) entry\n", i);
				return;
			}

			if (para.new_ippt_itf == Entry.ifIndex) {
				// If this connection is PPPoE/oA, we should kill the old NAPT rule in POSTROUTING chain.
				// When channel mode is rt1483, NAPT rule will be deleted by stopConnection().
				if ( (Entry.cmode == CHANNEL_MODE_PPPOE || Entry.cmode == CHANNEL_MODE_PPPOA)) {
					snprintf(pppif, 6, "ppp%u", PPP_INDEX(Entry.ifIndex));
					va_cmd(IPTABLES, 8, 1, "-t", "nat", FW_DEL, NAT_ADDRESS_MAP_TBL,
			 			"-o", pppif, "-j", "MASQUERADE");
				}
				stopConnection(&Entry);
				if (Entry.cmode == CHANNEL_MODE_PPPOE) {
					isPPPoE = 1;
					vcIndex = VC_INDEX(Entry.ifIndex);
					mType = MEDIA_INDEX(Entry.ifIndex);
					selected = i;
				}
				restartWAN(CONFIGONE, &Entry);
				break;
			}
		}
		if (isPPPoE) {
			for (i=0; i<entryNum; i++) {
				if (i == selected)
					continue;
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
					printf("restartIPPT: cannot get ATM_VC_TBL-1(ch=%d) entry\n", i);
					return;
				}
				if (mType == MEDIA_INDEX(Entry.ifIndex) && vcIndex == VC_INDEX(Entry.ifIndex)) { // Jenny, for multisession PPPoE support
					restartWAN(CONFIGONE, &Entry);
				}
			}
		}

	}  //  if ( new_ippt_itf != 255 )

#ifdef CONFIG_USER_DHCP_SERVER
	// Check IPPT Lease Time
	// Here we just concern about IPPT is enable.
	if ( para.old_ippt_lease != para.new_ippt_lease && para.new_ippt_itf != DUMMY_IFINDEX) {
		restar_dhcpd_flag = 1;  // on restart DHCP server flag
		//printf("change IPPT Lease Time\n");
	}
#endif

	// Check IPPT LAN Access
	if ( para.new_ippt_itf != DUMMY_IFINDEX || para.old_ippt_itf != DUMMY_IFINDEX) {
		if ( para.old_ippt_lanacc == 0 && para.old_ippt_itf != DUMMY_IFINDEX)
			clean_IPPT_LAN_access();
		set_IPPT_LAN_access();
	}

#ifdef CONFIG_USER_DHCP_SERVER
	// Restart DHCP Server
	if ( restar_dhcpd_flag == 1 ) {
		restar_dhcpd_flag = 0;  // off restart DHCP server flag
		restart_dhcp();
	}
#endif

}
#endif

int caculate_tblid(uint32_t ifid)
{
	char ifname[IFNAMSIZ];
	ifGetName(ifid, ifname, sizeof(ifname));
	return rtk_wan_tblid_get(ifname);
}

int caculate_tblid_by_ifname(char *ifname)
{
	return rtk_wan_tblid_get(ifname);
}

#ifdef ROUTING
// update corresponding field of rtentry from MIB_CE_IP_ROUTE_T
static void updateRtEntry(MIB_CE_IP_ROUTE_T *pSrc, struct rtentry *pDst)
{
	struct sockaddr_in *s_in;
	struct in_addr gwAddr;

	//pDst->rt_flags = RTF_UP;
	s_in = (struct sockaddr_in *)&pDst->rt_dst;
	s_in->sin_family = AF_INET;
	s_in->sin_port = 0;
	s_in->sin_addr = *(struct in_addr *)pSrc->destID;

	s_in = (struct sockaddr_in *)&pDst->rt_genmask;
	s_in->sin_family = AF_INET;
	s_in->sin_port = 0;
	s_in->sin_addr = *(struct in_addr *)pSrc->netMask;

	//if (pSrc->nextHop[0]&&pSrc->nextHop[1]&&pSrc->nextHop[2]&&pSrc->nextHop[3]) {
	if (pSrc->nextHop[0] || pSrc->nextHop[1] || pSrc->nextHop[2] ||pSrc->nextHop[3]) {
		s_in = (struct sockaddr_in *)&pDst->rt_gateway;
		s_in->sin_family = AF_INET;
		s_in->sin_port = 0;
		s_in->sin_addr = *(struct in_addr *)pSrc->nextHop;
	}
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	if(pSrc->nextHopEnable == 0)
	{
		if(get_wan_gateway(pSrc->ifIndex, &gwAddr)!=0){
			printf("Didn't exist gatway in ifIndex=%d\n", pSrc->ifIndex);
			return;
		}
		s_in = (struct sockaddr_in *)&pDst->rt_gateway;
		s_in->sin_family = AF_INET;
		s_in->sin_port = 0;
		s_in->sin_addr = *(struct in_addr *)&gwAddr;
	}
#endif
}

/* When systemd renew ip need check the static route bound to the interface too, IulianWu */
void check_staticRoute_change(char *ifname)
{
	unsigned int totalEntry =0;
	int i=0;
	MIB_CE_IP_ROUTE_T Entry;
	char ifname_tmp[IFNAMSIZ];

	totalEntry = mib_chain_total(MIB_IP_ROUTE_TBL);
	for (i=0; i< totalEntry; i++){
		if(mib_chain_get(MIB_IP_ROUTE_TBL, i, (void *)&Entry) == 0)
			printf("\n %s %d\n", __func__, __LINE__);
		ifGetName(Entry.ifIndex, ifname_tmp, sizeof(ifname_tmp));
		if(!strcmp(ifname, ifname_tmp)&& Entry.Enable){
			route_cfg_modify(&Entry, 0, i);
		}
		else if(Entry.ifIndex == DUMMY_IFINDEX && Entry.Enable && strcmp(Entry.nextHop, "0.0.0.0")) //nextHop
		{
			struct in_addr inAddr;
			char itfIP[20], nextHop[20], netMask[20];

			if (getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 0)
			{
				printf("[%s:%d]get ip error\n", __func__, __LINE__);
				continue;
			}

			sprintf( itfIP, "%s",   inet_ntoa(inAddr) );
			sprintf( nextHop, "%s",   inet_ntoa(*((struct in_addr *)Entry.nextHop)));
			sprintf( netMask, "%s",   inet_ntoa(*((struct in_addr *)Entry.netMask)));

			if (!isSameSubnet(itfIP, nextHop, netMask))
				continue;
			route_cfg_modify(&Entry, 0, i);
		}
		else
			continue;
	}
}

/*del=>  1: delete the route entry,
         0: add the route entry(skip ppp part),
        -1: add the route entry*/
int route_cfg_modify(MIB_CE_IP_ROUTE_T *pRoute, int del, int entryID)
{
	char ifname[IFNAMSIZ];
	int ret = 0;
	MIB_CE_ATM_VC_T vc_entry = {0};
	unsigned int nh;
	int advRoute=0;
	char destIP[16]={0}, netmask[16]={0}, toDest[64]={0};
	int32_t tbl_id;
	char str_tblid[10]="";
	char metric[5];
	char rt_nH[16];
	MIB_CE_ATM_VC_T vcEntry;
	unsigned int entryNum;
	int j=0;
#ifdef SUPPORT_SET_WANTYPE_FOR_STATIC_ROUTE
	unsigned int IPTV_ifIndex;
#endif
	char iproute_tblid[16] = {0};
	char forceNH = 0;

	if (pRoute==NULL) return -1;
	if (!pRoute->Enable) return -1;

	ifGetName(pRoute->ifIndex, ifname, sizeof(ifname));

	nh = *(unsigned int *)pRoute->nextHop;
	strncpy(destIP, inet_ntoa(*((struct in_addr *)pRoute->destID)), sizeof(destIP)-1);
	strncpy(netmask, inet_ntoa(*((struct in_addr *)pRoute->netMask)), sizeof(netmask)-1);

	if (strcmp(destIP, "0.0.0.0") == 0)
	{
		printf("Didn't allow deault route setting in static route configuration!\n");
		return -1;
	}
	snprintf(toDest, 64, "%s/%s", destIP, netmask);

	if (nh == 0
#ifdef SUPPORT_SET_WANTYPE_FOR_STATIC_ROUTE
		|| pRoute->itfTypeEnable == 1
#endif
		) { // no next hop, check interface
		// If no interface specified, return failed.
#ifdef SUPPORT_SET_WANTYPE_FOR_STATIC_ROUTE
		if(pRoute->itfTypeEnable == 1)
		{
			if(getifindexByWanType(pRoute->itfType, &IPTV_ifIndex) == -1)
				return 0;
			if (!getATMVCEntryByIfIndex(IPTV_ifIndex, &vc_entry))
				return 0;
		}
		else
#endif
		if (!getATMVCEntryByIfIndex(pRoute->ifIndex, &vc_entry))
			return -1;
	}
	snprintf(iproute_tblid, sizeof(iproute_tblid), "%d", IP_ROUTE_STATIC_TABLE);

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	struct in_addr gwAddr;
	char str_ipaddr[64]={0};
	char str_netmask[64]={0};
	char str_gateway[64]={0};
	struct in_addr inAddr;
	if(pRoute->nextHopEnable == 0)
	{
#ifdef SUPPORT_SET_WANTYPE_FOR_STATIC_ROUTE
		if(pRoute->itfTypeEnable == 1)
		{
			if(get_wan_gateway(IPTV_ifIndex, &gwAddr)!=0){
				printf("Didn't exist gatway in ifIndex=%d\n", IPTV_ifIndex);
				return 0;
			}
		}
		else
#endif
		if(get_wan_gateway(pRoute->ifIndex, &gwAddr)!=0){
			printf("Didn't exist gatway in ifIndex=%d\n", pRoute->ifIndex);
			return -1;
		}
		sprintf(rt_nH, "%s", inet_ntoa(*((struct in_addr *)&gwAddr)));
		getIPaddrInfo(&vc_entry, str_ipaddr, str_netmask, str_gateway);
		getInAddr(ifname, SUBNET_MASK, &inAddr);
		strncpy(str_netmask, inet_ntoa(inAddr), sizeof(str_netmask)-1);
		if(!isSameSubnet(destIP, str_ipaddr, str_netmask))
		{
			forceNH=1;
		}
	}
	else
#endif
	{
		sprintf(rt_nH, "%s", inet_ntoa(*((struct in_addr *)pRoute->nextHop)));
	}
#ifdef SUPPORT_SET_WANTYPE_FOR_STATIC_ROUTE
	if(pRoute->itfTypeEnable == 1)
		ifGetName(IPTV_ifIndex, ifname, sizeof(ifname));
	else
#endif
	if(pRoute->ifIndex == DUMMY_IFINDEX)
	{
		//should find use which interface even NextHop is not null
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		struct in_addr inAddr;
		char str_ipv4[INET_ADDRSTRLEN] = {0};
		for (j=0; j<entryNum; j++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, j, (void *)&vcEntry))
				return 0;

			if(vcEntry.cmode != CHANNEL_MODE_BRIDGE && vcEntry.cmode != CHANNEL_MODE_IPOE) //ppp
			{
				FILE *fp;
				int flags, n;
				uint32_t mask, gateway;
				uint32_t dest, tmask, nH;
				char buf[256], tdevice[256];

				if(fp = fopen("/proc/net/route", "r"))
				{
					n = 0;
					mask = 0;
					nH = ntohl(hextol(pRoute->nextHop));
					//printf("%s-%d nH=%x\n",__func__,__LINE__,nH);
					while (fgets(buf, sizeof(buf), fp) != NULL) {
						++n;
						if (n == 1 && strncmp(buf, "Iface", 5) == 0)
							continue;
						sscanf(buf, "%255s %lx %lx %x %*s %*s %*s %x",
							tdevice, &dest, &gateway, &flags, &tmask);
						//printf("tdevice:%s, dest=%x, gateway=%x, tmask=%x\n",tdevice,dest,gateway,tmask);
						if ((nH & tmask) == dest
						 && (tmask > mask || mask == 0) && (flags & RTF_UP) && (dest != 0x7f000000) && (dest !=0)
						) {
							mask = tmask;
							strcpy(ifname, tdevice);
							//printf("[%s-%d] n:%d tdevice:%s, dest=%x, gateway=%x, tmask=%x\n",__func__,__LINE__,n,tdevice,dest,gateway,tmask);
							break;
						}
					}

					if(getInAddr(ifname, DST_IP_ADDR, &inAddr) == 1)
						strcpy(str_ipv4, inet_ntoa(inAddr));

					fclose(fp);
				}
				else
				{
					printf("[%s-%d] can not open /proc/net/route. \n", __func__, __LINE__);
					return 0;
				}
			}
			else
			{
				if(vcEntry.ipDhcp == (char)DHCP_CLIENT)
				{
					FILE *fp = NULL;
					char fname[128] = {0};

					ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
					sprintf(fname, "%s.%s", MER_GWINFO, ifname);

					if(fp = fopen(fname, "r"))
					{
						if(fscanf(fp, "%s", str_ipv4)!=1)
							printf("fscanf failed: %s %d\n", __func__, __LINE__);
						fclose(fp);
					}
				}
				else
				{
					unsigned char zero[IP_ADDR_LEN] = {0};
					if(memcmp(vcEntry.remoteIpAddr, zero, IP_ADDR_LEN))
						strcpy(str_ipv4, inet_ntoa(*((struct in_addr *)vcEntry.remoteIpAddr)));
                    if(!strcmp(str_ipv4, rt_nH))
                        ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
				}
			}
			//printf("[%s:%d] str_ipv4=%s, rt_nH=%s\n", __func__, __LINE__, str_ipv4, rt_nH);
			if(!strcmp(str_ipv4, rt_nH))
				break;

		}

	}

	if (del>0) {
		//static route add to route table IP_ROUTE_STATIC_TABLE
		sprintf(metric,"%d",pRoute->FWMetric);
		//printf("[%s:%d] nextHop = %s, toDest=%s, metric=%s, ifname=%s, forceNH=%hhu\n", __func__, __LINE__, rt_nH, toDest, metric, ifname, forceNH);
		if((vc_entry.cmode == CHANNEL_MODE_PPPOE) || (vc_entry.cmode == CHANNEL_MODE_PPPOA))
			ret = va_cmd(BIN_IP, 10, 1, "-4", "route", "del", toDest, "metric", metric, "dev", ifname, "table", iproute_tblid);
		else {
			if(pRoute->nextHop[0] || forceNH)
				ret = va_cmd(BIN_IP, 10, 1, "-4", "route", "del", toDest, "via", rt_nH, "metric", metric, "table", iproute_tblid);
			else
				ret = va_cmd(BIN_IP, 8, 1, "-4", "route", "del", toDest, "metric", metric, "table", iproute_tblid);
		}
		return ret;
	}

	/* For IPOE WAN which needs default gateway to reach the destination, add default gateway to this static route
	   even if there is no nexthop. 20210713 */
	if(vc_entry.cmode == CHANNEL_MODE_IPOE && !strcmp(rt_nH,"0.0.0.0")){
		FILE *fp = NULL;
		char fname[128] = {0};

		if(ifGetName(vc_entry.ifIndex, ifname, sizeof(ifname)))
		{
			sprintf(fname, "%s.%s", MER_GWINFO, ifname);

			if(fp = fopen(fname, "r")){
				if(fscanf(fp, "%s", rt_nH)!=1)
					printf("fscanf failed: %s %d\n", __func__, __LINE__);
				fclose(fp);
			}
		}
	}
	//static route add to route table IP_ROUTE_STATIC_TABLE
	sprintf(metric,"%d",pRoute->FWMetric);
	//printf("[%s:%d] nextHop = %s, toDest=%s, metric=%s, ifname=%s, forceNH=%hhu\n", __func__, __LINE__, rt_nH, toDest, metric, ifname, forceNH);
	if(pRoute->nextHop[0] || forceNH)
		ret = va_cmd(BIN_IP, 12, 1, "-4", "route", "add", toDest, "via", rt_nH, "metric", metric, "dev", ifname, "table", iproute_tblid);
	else
		ret = va_cmd(BIN_IP, 10, 1, "-4", "route", "add", toDest, "metric", metric, "dev", ifname, "table", iproute_tblid);

	return ret;
}

void route_ppp_ifup(unsigned long pppGW, char *ifname)
{
	unsigned int entryNum, i;
	char ifname0[IFNAMSIZ];//, *ifname;
	MIB_CE_IP_ROUTE_T Entry;
#ifdef CONFIG_USER_CWMP_TR069
	int cwmp_msgid;
	struct cwmp_message cwmpmsg;
	memset(&cwmpmsg, 0, sizeof(cwmpmsg));
#endif

	entryNum = mib_chain_total(MIB_IP_ROUTE_TBL);

	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_IP_ROUTE_TBL, i, (void *)&Entry))
		{
			continue;
		}
		if( !Entry.Enable ) continue;

#ifdef SUPPORT_SET_WANTYPE_FOR_STATIC_ROUTE
		if(Entry.itfTypeEnable == 1)
			if(getifindexByWanType(Entry.itfType, &Entry.ifIndex) == -1)
				continue;
#endif
		ifGetName(Entry.ifIndex, ifname0, sizeof(ifname0));

		if (!strncmp(ifname0, "ppp", 3)) {
			route_cfg_modify(&Entry, 0, i);
		}
		else if (Entry.ifIndex == DUMMY_IFINDEX) {	// Interface "any"
			struct in_addr *addr;
			addr = (struct in_addr *)&Entry.nextHop;
			if (addr->s_addr == pppGW) {
				route_cfg_modify(&Entry, 0, i);
			}
		}
	}

#ifdef CONFIG_USER_CWMP_TR069
#ifdef CONFIG_E8B
	int numOfIpv6 = 0;
	struct ipv6_ifaddr ipv6_addr;
	numOfIpv6 = getifip6(ifname, IPV6_ADDR_UNICAST, &ipv6_addr, 1);
	//fprintf(stderr, "ifname = %s, numOfIpv6 = %d \n", __FILE__, __FUNCTION__, __LINE__, ifname, numOfIpv6);
	if (numOfIpv6 == 0) // IPv6 not up
	{
		unsigned char InformType = 0;
		mib_get_s(PROVINCE_CWMP_INFORM_TYPE, &InformType, sizeof(InformType));

		if (InformType == CWMP_INFORM_TYPE_GUD)
		{
			unsigned int vUint = 0;
			if (mib_get_s(CWMP_INFORM_USER_EVENTCODE, &vUint, sizeof(vUint))) {
				if (!(vUint & EC_X_CT_COM_BIND_1)) {
					vUint = vUint | EC_X_CT_COM_BIND_1;
					mib_set(CWMP_INFORM_USER_EVENTCODE, &vUint);
				}
			}
		}
	}
#endif

	memset(&cwmpmsg,0,sizeof(struct cwmp_message));
	if((cwmp_msgid = msgget((key_t)1234, 0)) > 0 )
	{
		cwmpmsg.msg_type = MSG_ACTIVE_NOTIFY;
		msgsnd(cwmp_msgid, (void *)&cwmpmsg, MSG_SIZE, 0);
	}
#endif
}

void addStaticRoute(void)
{
	unsigned int entryNum, i;
	MIB_CE_IP_ROUTE_T Entry;

	entryNum = mib_chain_total(MIB_IP_ROUTE_TBL);

	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_IP_ROUTE_TBL, i, (void *)&Entry))
		{
			return;
		}

		route_cfg_modify(&Entry, 0, i);
	}
}

void deleteStaticRoute(void)
{
	unsigned int entryNum, i;
	MIB_CE_IP_ROUTE_T Entry;

	entryNum = mib_chain_total(MIB_IP_ROUTE_TBL);

	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_IP_ROUTE_TBL, i, (void *)&Entry))
		{
			return;
		}
		route_cfg_modify(&Entry, 1, i);
	}
}

int getDynamicForwardingTotalNum(void)
{
	int ret=0;
	char *routename="/proc/net/route";
	FILE *fp;

	fp=fopen( routename, "r" );
	if(fp)
	{
		char buff[256];

		fgets(buff, sizeof(buff), fp);
		while( fgets(buff, sizeof(buff), fp) != NULL )
		{
			int flgs, ref, use, metric;
			char ifname[16], ifname2[16];
			uint32_t d,g,m;

			if(sscanf(buff, "%s%x%x%X%d%d%d%x",
				ifname, &d, &g, &flgs, &ref, &use, &metric, &m)!=8)
				break;//Unsuported kernel route format

			{
				unsigned int i,num, is_match_static=0;
				MIB_CE_IP_ROUTE_T *p,entity;

				if( strncmp(ifname, "lo", sizeof(ifname)) == 0)
					continue;

				num = mib_chain_total( MIB_IP_ROUTE_TBL );
				for( i=0; i<num;i++ )
				{
					p = &entity;
					if( !mib_chain_get( MIB_IP_ROUTE_TBL, i, (void*)p ))
						continue;

					if( ifGetName( p->ifIndex, ifname2, sizeof(ifname2) )==0 ) //any interface
						ifname2[0]=0;

					if(  (p->Enable) &&
					     ( (p->ifIndex==DUMMY_IFINDEX) || (strcmp(ifname, ifname2)==0) ) && //interface: any or specific
					     (*((uint32_t*)(p->destID))==d) &&  //destIPaddress
					     (*((uint32_t*)(p->netMask))==m) && //netmask
					     (*((uint32_t*)(p->nextHop))==g) && //GatewayIPaddress
					     ( (p->FWMetric==-1 && metric==0) || (p->FWMetric==metric )	) //ForwardingMetric
					  )
					{
						is_match_static=1; //static route
						break;
					}

				}
				if(is_match_static==0) ret++; //dynamic route
			}
		}
		fclose(fp);
	}

#if defined(CONFIG_IPV6) && defined(CONFIG_00R0)
	fp = fopen("/proc/net/ipv6_route", "r");
	if (fp)
	{
		char buff[1024] = {0}, iface[10] = {0};
		int num, iflags, metric, refcnt, use, prefix_len, slen;
		char addr6p[8][5], saddr6p[8][5], naddr6p[8][5];
		char addr6[INET6_ADDRSTRLEN] = {0}, naddr6[INET6_ADDRSTRLEN] = {0};
		char dest_str[INET6_ADDRSTRLEN] = {0}, nexthop_str[INET6_ADDRSTRLEN] = {0};

		while (fgets(buff, sizeof(buff), fp))
		{
			num = sscanf(buff, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %4s%4s%4s%4s%4s%4s%4s%4s %02x %4s%4s%4s%4s%4s%4s%4s%4s %08x %08x %08x %08x %s\n",
							addr6p[0], addr6p[1], addr6p[2], addr6p[3],
							addr6p[4], addr6p[5], addr6p[6], addr6p[7],
							&prefix_len,
							saddr6p[0], saddr6p[1], saddr6p[2], saddr6p[3],
							saddr6p[4], saddr6p[5], saddr6p[6], saddr6p[7],
							&slen,
							naddr6p[0], naddr6p[1], naddr6p[2], naddr6p[3],
							naddr6p[4], naddr6p[5], naddr6p[6], naddr6p[7],
							&metric, &use, &refcnt, &iflags, iface);

			if (!(iflags & RTF_UP) || strcmp(iface, "lo") == 0 || (strncmp(addr6p[0], "ff", 2) == 0 && prefix_len == 8))
				continue;

			snprintf(addr6, sizeof(addr6), "%s:%s:%s:%s:%s:%s:%s:%s",
					addr6p[0], addr6p[1], addr6p[2], addr6p[3],
					addr6p[4], addr6p[5], addr6p[6], addr6p[7]);

			snprintf(naddr6, sizeof(naddr6), "%s:%s:%s:%s:%s:%s:%s:%s",
					naddr6p[0], naddr6p[1], naddr6p[2], naddr6p[3],
					naddr6p[4], naddr6p[5], naddr6p[6], naddr6p[7]);

			ipv6_route_addr_format_transfer(addr6, dest_str);
			ipv6_route_addr_format_transfer(naddr6, nexthop_str);

			if (metric == 1024 && strcmp(dest_str, "::") != 0 && strcmp(nexthop_str, "::") == 0)
				continue;

			ret++;
		}

		fclose(fp);
	}
#endif

	return ret;
}

int getDynamicForwardingEntryByInstNum( unsigned int instnum, L3_FORWARDING_ENTRY_T *pRoute )
{
	int ret=-1;
	int count=0;
	char *routename="/proc/net/route";
	FILE *fp;

	if( (instnum==0) || (pRoute==NULL) ) return ret;
	memset( pRoute, 0, sizeof(L3_FORWARDING_ENTRY_T) );
	pRoute->InstanceNum = instnum;
	pRoute->ForwardingMetric = -1;
	pRoute->ifIndex = DUMMY_IFINDEX;

	fp=fopen( routename, "r" );
	if(fp)
	{
		char buff[256];
		fgets(buff, sizeof(buff), fp);
		while( fgets(buff, sizeof(buff), fp) != NULL )
		{
			int flgs, ref, use, metric;
			char ifname[16], ifname2[16];
			uint32_t d,g,m;

			if(sscanf(buff, "%s%x%x%X%d%d%d%x",
				ifname, &d, &g, &flgs, &ref, &use, &metric, &m)!=8)
				break;//Unsuported kernel route format

			{
				unsigned int i,num, is_match_static=0;
				MIB_CE_IP_ROUTE_T *p,entity;

				if( strncmp(ifname, "lo", sizeof(ifname)) == 0)
					continue;

				num = mib_chain_total( MIB_IP_ROUTE_TBL );
				for( i=0; i<num;i++ )
				{
					p = &entity;
					if( !mib_chain_get( MIB_IP_ROUTE_TBL, i, (void*)p ))
						continue;

					if( ifGetName( p->ifIndex, ifname2, sizeof(ifname2) )==0 ) //any interface
						ifname2[0]=0;

					if(  (p->Enable) &&
					     ( (p->ifIndex==DUMMY_IFINDEX) || (strcmp(ifname, ifname2)==0) ) && //interface: any or specific
					     (*((uint32_t*)(p->destID))==d) &&  //destIPaddress
					     (*((uint32_t*)(p->netMask))==m) && //netmask
					     (*((uint32_t*)(p->nextHop))==g) && //GatewayIPaddress
					     ( (p->FWMetric==-1 && metric==0) || (p->FWMetric==metric )	) //ForwardingMetric
					  )
					{
						is_match_static=1; //static route
						break;
					}
				}
				if(is_match_static==0) count++; //dynamic route
			}

			if(count==instnum)
			{
				pRoute->IPVersion = 4;

				//CONFIG_PTMWAN, CONFIG_ETHWAN
				//if failed, transfer2IfIndxfromIfName returns DUMMY_IFINDEX
				//fprintf(stderr, "transfering %s to ifIndex\n", ifname);
				if(strncmp(ifname, "br", 2) == 0)
					pRoute->ifIndex = LAN_IFINDEX;
				else
					pRoute->ifIndex = getIfIndexByName(ifname);

				sprintf(pRoute->DestIPAddress, "%s", inet_ntoa(*((struct in_addr *)&d)));
				sprintf(pRoute->DestSubnetMask, "%s", inet_ntoa(*((struct in_addr *)&m)));
				sprintf(pRoute->GatewayIPAddress, "%s", inet_ntoa(*((struct in_addr *)&g)));

				pRoute->ForwardingMetric = metric;
				pRoute->Enable = (flgs&RTF_UP)?1:0;
				if(flgs&RTF_HOST)
					pRoute->Type = 1; //host
				else if( (d==0) && (m==0) )
					pRoute->Type = 2; //default
				else
					pRoute->Type = 0; //network

				ret = 0;
				break;
			}
		}
		fclose(fp);
	}

	if (ret == 0) return ret;

#if defined(CONFIG_IPV6) && defined(CONFIG_00R0)
	fp = fopen("/proc/net/ipv6_route", "r");
	if (fp)
	{
		char buff[1024] = {0}, iface[10] = {0};
		int num, iflags, metric, refcnt, use, prefix_len, slen;
		char addr6p[8][5], saddr6p[8][5], naddr6p[8][5];
		char addr6[INET6_ADDRSTRLEN] = {0}, saddr6[INET6_ADDRSTRLEN] = {0}, naddr6[INET6_ADDRSTRLEN] = {0};
		char dest_str[INET6_ADDRSTRLEN] = {0}, src_str[INET6_ADDRSTRLEN] = {0}, nexthop_str[INET6_ADDRSTRLEN] = {0};

		while (fgets(buff, sizeof(buff), fp))
		{
			num = sscanf(buff, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %4s%4s%4s%4s%4s%4s%4s%4s %02x %4s%4s%4s%4s%4s%4s%4s%4s %08x %08x %08x %08x %s\n",
							addr6p[0], addr6p[1], addr6p[2], addr6p[3],
							addr6p[4], addr6p[5], addr6p[6], addr6p[7],
							&prefix_len,
							saddr6p[0], saddr6p[1], saddr6p[2], saddr6p[3],
							saddr6p[4], saddr6p[5], saddr6p[6], saddr6p[7],
							&slen,
							naddr6p[0], naddr6p[1], naddr6p[2], naddr6p[3],
							naddr6p[4], naddr6p[5], naddr6p[6], naddr6p[7],
							&metric, &use, &refcnt, &iflags, iface);

			if (!(iflags & RTF_UP) || strcmp(iface, "lo") == 0 || (strncmp(addr6p[0], "ff", 2) == 0 && prefix_len == 8))
				continue;

			snprintf(addr6, sizeof(addr6), "%s:%s:%s:%s:%s:%s:%s:%s",
					addr6p[0], addr6p[1], addr6p[2], addr6p[3],
					addr6p[4], addr6p[5], addr6p[6], addr6p[7]);

			snprintf(saddr6, sizeof(saddr6), "%s:%s:%s:%s:%s:%s:%s:%s",
					saddr6p[0], saddr6p[1], saddr6p[2], saddr6p[3],
					saddr6p[4], saddr6p[5], saddr6p[6], saddr6p[7]);

			snprintf(naddr6, sizeof(naddr6), "%s:%s:%s:%s:%s:%s:%s:%s",
					naddr6p[0], naddr6p[1], naddr6p[2], naddr6p[3],
					naddr6p[4], naddr6p[5], naddr6p[6], naddr6p[7]);

			ipv6_route_addr_format_transfer(addr6, dest_str);
			ipv6_route_addr_format_transfer(saddr6, src_str);
			ipv6_route_addr_format_transfer(naddr6, nexthop_str);

			if (metric == 1024 && strcmp(dest_str, "::") != 0 && strcmp(nexthop_str, "::") == 0)
				continue;

			count++;

			if (count == instnum)
			{
				pRoute->IPVersion = 6;

				if (strncmp(iface, "br", 2) == 0)
					pRoute->ifIndex = LAN_IFINDEX;
				else
					pRoute->ifIndex = rtk_transfer2IfIndxfromIfName(iface);

				snprintf(pRoute->DestIPAddress, sizeof(pRoute->DestIPAddress), "%s", dest_str);
				pRoute->DestPrefixLength = prefix_len;

				if (strcmp(src_str, "::") != 0)
				{
					snprintf(pRoute->SourceIPAddress, sizeof(pRoute->SourceIPAddress), "%s", src_str);
					pRoute->SourcePrefixLength = slen;
				}

				if (strcmp(nexthop_str, "::") != 0)
				{
					snprintf(pRoute->GatewayIPAddress, sizeof(pRoute->GatewayIPAddress), "%s", nexthop_str);
				}

				pRoute->ForwardingMetric = metric;
				pRoute->Enable = (iflags&RTF_UP)?1:0;
				if(iflags&RTF_HOST)
					pRoute->Type = 1; //host
				else if (strcmp(dest_str, "::") == 0 && metric == 1024)
					pRoute->Type = 2; //default
				else
					pRoute->Type = 0; //network

				ret = 0;
				break;
			}
		}
		fclose(fp);
	}
#endif

	return ret;
}

//return -1:error,  0:not found, 1:found in /proc/net/route
//refer to user/busybox/route.c:displayroutes()
int queryRouteStatus( MIB_CE_IP_ROUTE_T *p )
{
	int ret=-1;

	//fprintf( stderr, "<%s:%d>Start\n",__FUNCTION__,__LINE__ );

	if(p)
	{
		char *routename="/proc/net/route";
		FILE *fp;

		fp=fopen( routename, "r" );
		if(fp)
		{
			char buff[256];
			int  i=0;
			while( fgets(buff, sizeof(buff), fp) != NULL )
			{
				int flgs, ref, use, metric;
				char ifname[16], ifname2[16];
				uint32_t d,g,m;

				//fprintf( stderr, "<%s:%d>%s\n",__FUNCTION__,__LINE__,buff );
				i++;
				if(i==1) continue; //skip the first line

				//Iface Destination Gateway Flags RefCnt Use Metric Mask MTU Window IRTT
				if(sscanf(buff, "%s%x%x%X%d%d%d%x",
					ifname, &d, &g, &flgs, &ref, &use, &metric, &m)!=8)
					break;//Unsuported kernel route format

				if( ifGetName( p->ifIndex, ifname2, sizeof(ifname2) )==0 ) //any interface
					ifname2[0]=0;

				//fprintf( stderr, "<%s:%d>%s==%s(%d)\n",__FUNCTION__,__LINE__,ifname,ifname2, ( (strlen(ifname2)==0) || (strcmp(ifname, ifname2)==0) ) );
				//fprintf( stderr, "<%s:%d>%06x==%06x(%d)\n",__FUNCTION__,__LINE__,*((unsigned long int*)(p->destID)),d,(*((unsigned long int*)(p->destID))==d) );
				//fprintf( stderr, "<%s:%d>%06x==%06x(%d)\n",__FUNCTION__,__LINE__,*((unsigned long int*)(p->netMask)),m,(*((unsigned long int*)(p->netMask))==m) );
				//fprintf( stderr, "<%s:%d>%06x==%06x(%d)\n",__FUNCTION__,__LINE__,*((unsigned long int*)(p->nextHop)),g,(*((unsigned long int*)(p->nextHop))==g) );
				//fprintf( stderr, "<%s:%d>%d==%d(%d)\n",__FUNCTION__,__LINE__, p->FWMetric, metric,( (p->FWMetric==-1 && metric==0) || (p->FWMetric==metric )	) );

				if(  (p->Enable) &&
				     ( (p->ifIndex==DUMMY_IFINDEX) || (strcmp(ifname, ifname2)==0) ) && //interface: any or specific
				     (*((uint32_t *)(p->destID))==d) &&  //destIPaddress
				     (*((uint32_t *)(p->netMask))==m) && //netmask
				     (*((uint32_t *)(p->nextHop))==g) && //GatewayIPaddress
				     ( (p->FWMetric==-1 && metric==0) || (p->FWMetric==metric )	) //ForwardingMetric
				  )
				{
					//fprintf( stderr, "<%s:%d>Found\n",__FUNCTION__,__LINE__ );
					ret=1;//found
					break;
				}
			}

			if( feof(fp) && (ret!=1) )
			{
					//fprintf( stderr, "<%s:%d>Not Found\n",__FUNCTION__,__LINE__ );
					ret=0; //not found
			}
			fclose(fp);
		}
	}

	//fprintf( stderr, "<%s:%d>End(ret:%d)\n",__FUNCTION__,__LINE__,ret );

	return ret;
}

int getForwardingEntryByInstNum( unsigned int instnum, MIB_CE_IP_ROUTE_T *p, unsigned int *id )
{
	int ret=-1;
	unsigned int i,num;

	if( (instnum==0) || (p==NULL) || (id==NULL) )
		return ret;

	num = mib_chain_total( MIB_IP_ROUTE_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_IP_ROUTE_TBL, i, (void*)p ) )
			continue;

		if( p->InstanceNum==instnum )
		{
			*id = i;
			ret = 0;
			break;
		}
	}
	return ret;
}
#endif

//--Default Route
#ifdef CONFIG_E8B
// When system start up need clear all WAN dgw.
// For E8 project let multiple internet WAN auto select the default route which one is first up
void reset_default_route(void)
{
	MIB_CE_ATM_VC_T vc_entry = {0};
	unsigned char ifname[IFNAMSIZ];
	char cmd[256];
	unsigned int i,num;

	num = mib_chain_total(MIB_ATM_VC_TBL);
	for( i=0; i<num; i++ )
	{
		if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vc_entry))
			continue;

		if(vc_entry.dgw != 0)
		{
			vc_entry.dgw = 0;
			mib_chain_update(MIB_ATM_VC_TBL, &vc_entry, i);
		}
	}
}

static int _clear_default_route_entry(int ipv, MIB_CE_ATM_VC_Tp pentry)
{
	char ifname[IFNAMSIZ+1]={0};
	char buf[128]={0};

	ifGetName(pentry->ifIndex, ifname, sizeof(ifname));
	if(ipv == IPVER_IPV4)
	{
		sprintf(buf, "/usr/bin/ip route del default dev %s", ifname);
		va_cmd("/bin/sh", 2, 1, "-c", buf);
		//set_ipv4_source_route(pentry, 1);
	}
#ifdef CONFIG_IPV6
	else
	{
		//if interface is not default wan, do not make kernel send RTM_NEWROUTE netlink any more
		if(pentry->AddrMode != IPV6_WAN_STATIC)
		{
			snprintf(buf, sizeof(buf), "echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", ifname);
			va_cmd("/bin/sh", 2, 1, "-c", buf);
		}
		sprintf(buf, "/usr/bin/ip -6 route del default dev %s", ifname);
		va_cmd_no_error("/bin/sh", 2, 1, "-c", buf);
		// workaround, because there was multiple default route for ppp
		sprintf(buf, "/usr/bin/ip -6 route del default dev %s", ifname);
		va_cmd_no_error("/bin/sh", 2, 1, "-c", buf);
		//set_ipv6_source_route(pentry, 1);
	}
#endif
	return 0;
}

int _check_and_update_default_route_entry(const char *ifname, void *entry, int ipv, int on_off)
{
	char new_ifname[IFNAMSIZ+1], bak_ifname[IFNAMSIZ+1];
	char buf[128]={0};
	MIB_CE_ATM_VC_T vc_entry = {0};
	MIB_CE_ATM_VC_T tmp_vc_entry = {0};
	 int i,num;
	int wan_ifIndex = -1, ret, update = 0, found_dgw_idx = -1, ifi_flags;
	MIB_CE_ATM_VC_Tp p_vc_entry = NULL;
	int entry_idx = -1;
#if defined(CONFIG_YUEME) && defined(SUPPORT_CLOUD_VR_SERVICE)
	int found_vr_dgw_idx = -1;
	MIB_CE_ATM_VC_T vr_vc_entry = {0};
#endif

	if(entry == NULL)
	{
		ret = getWanEntrybyIfname(ifname, &vc_entry, &wan_ifIndex);
		if(ret < 0){
			AUG_PRT(" Cannot get ATM_VC_TBL by ifname(%s) \n", ifname);
			return -1;
		}
		AUG_PRT("ifname = %s, ip version %d, on_off = %d\n", ifname, (ipv == IPVER_IPV4)?4:6, on_off);
	}
	else
	{
		memcpy(&vc_entry,entry, sizeof(MIB_CE_ATM_VC_T));
		AUG_PRT("entry = %p, ip version %d, on_off = %d\n", entry, (ipv == IPVER_IPV4)?4:6, on_off);
		ifGetName(vc_entry.ifIndex, bak_ifname, sizeof(bak_ifname));
		ifname = bak_ifname;
	}

#ifdef CONFIG_IPV6
	if(!(vc_entry.IpProtocol & ipv) || !vc_entry.cmode)
#else
	if(!vc_entry.cmode)
#endif
	{
		AUG_PRT(" ATM_VC_TBL.%d ifname(%s) not support ip version %d \n", wan_ifIndex, ifname, (ipv == IPVER_IPV4)?4:6);
		return -1;
	}

	if(!((vc_entry.applicationtype & X_CT_SRV_INTERNET)
#if defined(CONFIG_YUEME) && defined(SUPPORT_CLOUD_VR_SERVICE)
		|| (vc_entry.applicationtype & X_CT_SRV_SPECIAL_SERVICE_VR)
#endif
#ifdef CONFIG_SUPPORT_HQOS_APPLICATIONTYPE
		|| (vc_entry.othertype == OTHER_HQOS_TYPE)
#endif
	)) {
		AUG_PRT(" ATM_VC_TBL.%d ifname(%s) is not internet WAN \n", wan_ifIndex, ifname);
		if(vc_entry.dgw & ipv){
			vc_entry.dgw &= ~(ipv);
			if(wan_ifIndex>=0 && !mib_chain_update(MIB_ATM_VC_TBL, &vc_entry, wan_ifIndex)){
				AUG_PRT(" Update ATM_VC_TBL.%d Fail \n", wan_ifIndex);
			}
		}
		ret = 0;
		if(on_off) goto done;
		else if(entry == NULL) return 0;
	}

	if(on_off == 0)
	{
		if(vc_entry.dgw & ipv)
		{
			AUG_PRT(" ATM_VC_TBL.%d ifname(%s) disable default route for ip version %d \n", wan_ifIndex, ifname, (ipv == IPVER_IPV4)?4:6);
			vc_entry.dgw &= ~(ipv);
			if(wan_ifIndex>=0 && !mib_chain_update(MIB_ATM_VC_TBL, &vc_entry, wan_ifIndex)){
				AUG_PRT(" Update ATM_VC_TBL.%d Fail \n", wan_ifIndex);
			}
		}
		else {
			AUG_PRT(" ATM_VC_TBL.%d ifname(%s) is not enable default route for ip version %d \n", wan_ifIndex, ifname, (ipv == IPVER_IPV4)?4:6);
			ret = 0;
			if(entry == NULL) return 0;
			else goto done;
		}
	}
	else
	{
		if(vc_entry.dgw & ipv){
			AUG_PRT(" ATM_VC_TBL.%d ifname(%s) is already enable default route for ip version %d \n", wan_ifIndex, ifname, (ipv == IPVER_IPV4)?4:6);
			return 0;
		}
	}

	num = mib_chain_total(MIB_ATM_VC_TBL);
	for( i=0; i<num; i++ )
	{
		if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&tmp_vc_entry) ||
			tmp_vc_entry.ifIndex == vc_entry.ifIndex ||
			i == wan_ifIndex || !tmp_vc_entry.cmode || !(tmp_vc_entry.enable) ||
			(!((tmp_vc_entry.applicationtype & X_CT_SRV_INTERNET)
#if defined(CONFIG_YUEME) && defined(SUPPORT_CLOUD_VR_SERVICE)
				|| (tmp_vc_entry.applicationtype & X_CT_SRV_SPECIAL_SERVICE_VR)
#endif
#ifdef CONFIG_SUPPORT_HQOS_APPLICATIONTYPE
				|| (tmp_vc_entry.othertype == OTHER_HQOS_TYPE)
#endif
			))
#ifdef CONFIG_IPV6
			||!(tmp_vc_entry.IpProtocol & ipv)
#endif
		) continue;

		if(on_off)
		{
			if(tmp_vc_entry.dgw & ipv) {
#if defined(CONFIG_YUEME) && defined(SUPPORT_CLOUD_VR_SERVICE)
				if(tmp_vc_entry.applicationtype & X_CT_SRV_SPECIAL_SERVICE_VR)
				{
					found_vr_dgw_idx = i;
					memcpy(&vr_vc_entry, &tmp_vc_entry, sizeof(vr_vc_entry));
				}
				else
#endif
				{
					found_dgw_idx = i;
					break;
				}
			}
		}
		else
		{
			ifGetName(tmp_vc_entry.ifIndex, new_ifname, sizeof(new_ifname));
			if(getInFlags(new_ifname, &ifi_flags) && (ifi_flags & IFF_UP) && (ifi_flags & IFF_RUNNING))
			{
				sprintf(buf, "%s.%s", (ipv == IPVER_IPV4)?MER_GWINFO:MER_GWINFO_V6, new_ifname);
				if(/*ipv == IPVER_IPV4 && */(tmp_vc_entry.cmode == CHANNEL_MODE_PPPOE || tmp_vc_entry.cmode == CHANNEL_MODE_PPPOA) || access(buf, F_OK) == 0)
				{
#if defined(CONFIG_YUEME) && defined(SUPPORT_CLOUD_VR_SERVICE)
					if(tmp_vc_entry.applicationtype & X_CT_SRV_SPECIAL_SERVICE_VR)
					{
						found_vr_dgw_idx = i;
						memcpy(&vr_vc_entry, &tmp_vc_entry, sizeof(vr_vc_entry));
					}
					else
#endif
					{
						found_dgw_idx = i;
						break;
					}
				}
			}
		}
	}

	if(on_off)
	{
		if(found_dgw_idx < 0)
		{
#if defined(CONFIG_YUEME) && defined(SUPPORT_CLOUD_VR_SERVICE)
			if((vc_entry.applicationtype & X_CT_SRV_SPECIAL_SERVICE_VR)) {
				if(found_vr_dgw_idx < 0) {
					entry_idx = wan_ifIndex;
					p_vc_entry = &vc_entry;
				}
			}
			else
#endif
			{
				entry_idx = wan_ifIndex;
				p_vc_entry = &vc_entry;
			}
		}
		else
		{
			ret = 0;
			AUG_PRT(" Already has default route for ip version %d on ATM_VC_TBL.%d !\n", (ipv == IPVER_IPV4)?4:6, found_dgw_idx);
		}
	}
	else
	{
		if(found_dgw_idx >= 0)
		{
			entry_idx = found_dgw_idx;
			p_vc_entry = &tmp_vc_entry;
		}
#if defined(CONFIG_YUEME) && defined(SUPPORT_CLOUD_VR_SERVICE)
		else if(found_vr_dgw_idx >= 0)
		{
			entry_idx = found_vr_dgw_idx;
			p_vc_entry = &vr_vc_entry;
		}
#endif
		else
		{
			ret = 0;
			AUG_PRT(" No found ATM_VC_TBL entry as default route for ip version %d !\n", (ipv == IPVER_IPV4)?4:6);
		}
	}

	if(entry_idx >= 0)
	{
		FILE *fp = NULL;
		char gateway[64] = {0};
		ifGetName(p_vc_entry->ifIndex, new_ifname, sizeof(new_ifname));
		AUG_PRT(" ip version %d default route updated to ATM_VC_TBL.%d(%s) \n", (ipv == IPVER_IPV4)?4:6, entry_idx, new_ifname);
		p_vc_entry->dgw |= ipv;
		if(!mib_chain_update(MIB_ATM_VC_TBL, p_vc_entry, entry_idx)){
			AUG_PRT(" Update ATM_VC_TBL.%d Fail \n", entry_idx);
			return -1;
		}

		if(ipv == IPVER_IPV4)
		{
			if(p_vc_entry->cmode == CHANNEL_MODE_PPPOE || p_vc_entry->cmode == CHANNEL_MODE_PPPOA)
			{
				sprintf(buf, "/usr/bin/ip route replace default dev %s", new_ifname);
				va_cmd("/bin/sh", 2, 1, "-c", buf);
			}
			else
			{
				sprintf(buf, "%s.%s", MER_GWINFO, new_ifname);
				if((fp = fopen(buf, "r")))
				{
					fscanf(fp, "%[^\n]", gateway);
					fclose(fp);
					if(gateway[0])
					{
						sprintf(buf, "/usr/bin/ip route replace default via %s dev %s", gateway, new_ifname);
						va_cmd("/bin/sh", 2, 1, "-c", buf);
					}
				}
			}
		}
#ifdef CONFIG_IPV6
		else
		{
			char *pbuf;

			gateway[0] = '\0';
			sprintf(buf, "%s.%s", MER_GWINFO_V6, new_ifname);
			if((fp = fopen(buf, "r")))
			{
				fscanf(fp, "%[^\n]", gateway);
				fclose(fp);
			}

			pbuf = buf;
			pbuf += sprintf(pbuf, "/usr/bin/ip -6 route add default dev %s", new_ifname);

			if(gateway[0]) {
				pbuf += sprintf(pbuf, " via %s", gateway);
			}
			if(!(p_vc_entry->AddrMode & IPV6_WAN_STATIC)) {
				pbuf += sprintf(pbuf, " proto ra");
			}

			va_cmd("/bin/sh", 2, 1, "-c", buf);

			if(!(p_vc_entry->AddrMode & IPV6_WAN_STATIC)) {
				snprintf(buf, sizeof(buf), "echo 1 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", new_ifname);
				va_cmd("/bin/sh", 2, 1, "-c", buf);
				AUG_PRT(" Send Router Solicitation to %s  !\n", new_ifname);
				sendRouterSolicit(new_ifname);
			}
		}
#endif
		ret = 1;
	}

done:
	if(!(vc_entry.dgw & ipv))
	{
		if(on_off)
		{
			AUG_PRT(" Clear default route %d %s\n", (ipv == IPVER_IPV4)?4:6, ifname);
			_clear_default_route_entry(ipv, &vc_entry);
		}
	}
#if defined(CONFIG_YUEME) && defined(SUPPORT_CLOUD_VR_SERVICE)
	else if(found_vr_dgw_idx >= 0)
	{
		if(on_off)
		{
			vr_vc_entry.dgw &= ~(ipv);
			mib_chain_update(MIB_ATM_VC_TBL, &vr_vc_entry, found_vr_dgw_idx);
			ifGetName(vr_vc_entry.ifIndex, new_ifname, sizeof(new_ifname));
			AUG_PRT(" Clear default route %d %s\n", (ipv == IPVER_IPV4)?4:6, new_ifname);
			_clear_default_route_entry(ipv, &vr_vc_entry);
		}
	}
#endif
	return ret;
}
#endif

int rtk_net_get_wan_pppoe_order(MIB_CE_ATM_VC_Tp pEntry)
{
	MIB_CE_ATM_VC_T Entry;
	unsigned int totalEntry;
	int i, order=-1;

	if (pEntry->cmode != CHANNEL_MODE_PPPOE)
		return -1;

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<totalEntry; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;

		if (Entry.cmode != CHANNEL_MODE_PPPOE)
			continue;

		order = order+1;
		if (Entry.ifIndex == pEntry->ifIndex)
			return order;
	}
	return -1;
}

int rtk_net_get_wan_pppoa_order(MIB_CE_ATM_VC_Tp pEntry)
{
	MIB_CE_ATM_VC_T Entry;
	unsigned int totalEntry;
	int i, order=-1;

	if (pEntry->cmode != CHANNEL_MODE_PPPOA)
		return -1;

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<totalEntry; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;

		if (Entry.cmode != CHANNEL_MODE_PPPOA)
			continue;

		order = order+1;
		if (Entry.ifIndex == pEntry->ifIndex)
			return order;
	}
	return -1;
}


int rtk_net_add_wan_policy_route_mark(char *ifname, unsigned int *mark, unsigned int *mask)
{
	return rtk_wan_sockmark_add(ifname, mark, mask, WANMARK_TYPE_SYS);
}

int rtk_net_remove_wan_policy_route_mark(char *ifname)
{
	return rtk_wan_sockmark_remove(ifname, WANMARK_TYPE_SYS);
}

int rtk_net_get_wan_policy_route_mark(char *ifname, unsigned int *mark, unsigned int *mask)
{
	return rtk_wan_sockmark_get(ifname, mark, mask);
}

int rtk_net_get_from_lan_with_wan_policy_route_mark(char *ifname, unsigned int *mark, unsigned int *mask)
{
	unsigned int _mark=0, _mask=0;

	//combine the WAN MARK with from LAN MARK as portmapping wan upstream route
	rtk_net_get_wan_policy_route_mark(ifname, &_mark, &_mask);
	_mark = SOCK_FROM_LAN_SET(_mark, 1);
	_mask = _mask | SOCK_FROM_LAN_BIT_MASK;

	if(mark) *mark = _mark;
	if(mask) *mask = _mask;

	return 1;
}

///--Others
#ifdef CONFIG_PPP
void write_to_pppd(struct data_to_pass_st *pmsg)
{
#if defined(CONFIG_LUNA)
	//Chuck: use system call to trigger spppd main.c pause(), fit for ql_xu performance tune fix
	system(pmsg->data);
#else
	int pppd_fifo_fd=-1;

	pppd_fifo_fd = open(PPPD_FIFO, O_WRONLY);
	if (pppd_fifo_fd == -1)
		fprintf(stderr, "Sorry, no spppd server\n");
	else
	{
		write(pppd_fifo_fd, pmsg, sizeof(*pmsg));
		close(pppd_fifo_fd);
	}
#endif
}
#endif

// return value:
// 0  : successful
// -1 : failed
int write_to_mpoad(struct data_to_pass_st *pmsg)
{
	int mpoad_fifo_fd=-1;
	int status=0;
	int ret=0;
#ifdef CONFIG_DEV_xDSL
	mpoad_fifo_fd = open(MPOAD_FIFO, O_WRONLY);
	if (mpoad_fifo_fd == -1) {
		fprintf(stderr, "Sorry, no mpoad server\n");
		status = -1;
	} else
	{
		// Modified by Mason Yu
		//write(mpoad_fifo_fd, pmsg, sizeof(*pmsg));
REWRITE:
		ret = write(mpoad_fifo_fd, pmsg, sizeof(*pmsg));
		if(ret<0 && errno==EPIPE)
			 goto REWRITE;
#ifdef _LINUX_2_6_
		//sleep more time for mpoad to wake up
		usleep(100*1000);
#else
		usleep(30*1000);
#endif

		close(mpoad_fifo_fd);
		// wait server to consume it
#ifdef _LINUX_2_6_
		//sleep more time for mpoad to wake up
		usleep(100*1000);
#else
		usleep(1000);
#endif
	}
#endif // CONFIG_DEV_xDSL
	return status;
}

void WRITE_DHCPC_FILE(int fh, unsigned char *buf)
{
	if ( write(fh, buf, strlen(buf)) != strlen(buf) ) {
		printf("Write udhcpc script file error!\n");
	}
}

#ifdef CONFIG_USER_DHCPCLIENT_MODE
int setupDHCPClient(void){
	int status=0;
	unsigned char value[64];
	unsigned int count=0;

	/* run dhcpc daemon on bridge interface */
	startDhcpc(ALIASNAME_BR0, NULL, 0);

err:
	return status;
}

#ifdef USE_DEONET /* sghong, 20230710 */
#define ARP_PROBE_MIN           1   /*minimum time between Probe packets */
#define ARP_PROBE_MAX           2   /*maximum time between Probe packets */

struct arpMsg {
	struct ethhdr ethhdr;       /* Ethernet header */
	u_short htype;              /* hardware type (must be ARPHRD_ETHER) */
	u_short ptype;              /* protocol type (must be ETH_P_IP) */
	u_char  hlen;               /* hardware address length (must be 6) */
	u_char  plen;               /* protocol address length (must be 4) */
	u_short operation;          /* ARP opcode */
	u_char  sHaddr[6];          /* sender's hardware address */
	u_char  sInaddr[4];         /* sender's IP address */
	u_char  tHaddr[6];          /* target's hardware address */
	u_char  tInaddr[4];         /* target's IP address */
	u_char  pad[18];            /* pad for min. Ethernet payload (60 bytes) */
};
int do_one_arp_probe(u_int32_t yiaddr, unsigned char *mac, char *interface)
{
	int optval = 1;
	int s;              /* socket */
	int rv = 1;         /* return value */
	struct sockaddr addr;       /* for interface name */
	struct arpMsg   arp;
	fd_set      fdset;
	struct timeval  tm, orgtime, aftertime;
	u_int32_t u32_srcIP, u32_tarIP;
	const time_t MICROSEC = 1000000;
	time_t elapsed=0, remaining=0, timeout=0; /* unit: microsecond */
	time_t rand_min = MICROSEC*ARP_PROBE_MIN, rand_max = MICROSEC*ARP_PROBE_MAX; /* unit: microsecond */

	if ((s = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1)
	{
		return -1;
	}

	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1)
	{
		close(s);
		return -1;
	}

	/* send arp request */
	memset(&arp, 0, sizeof(arp));
	memcpy(arp.ethhdr.h_dest, MAC_BCAST_ADDR, 6);   /* MAC DA */
	memcpy(arp.ethhdr.h_source, mac, 6);            /* MAC SA */
	arp.ethhdr.h_proto = htons(ETH_P_ARP);          /* protocol type (Ethernet) */
	arp.htype = htons(ARPHRD_ETHER);                /* hardware type */
	arp.ptype = htons(ETH_P_IP);                    /* protocol type (ARP message)
	*/
	arp.hlen = 6;                                   /* hardware address length */
	arp.plen = 4;                                   /* protocol address length */
	arp.operation = htons(ARPOP_REQUEST);           /* ARP op code */
	memcpy(arp.sHaddr, mac, 6);                     /* sender hardware address */
	memcpy(arp.tInaddr, &yiaddr, sizeof(yiaddr));   /* target IP address */

	memset(&addr, 0, sizeof(addr));
	strcpy(addr.sa_data, interface);
	if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0)
		rv = 0;

	/* random interval in range [rand_min, rand_max] */
	srandom(time(NULL));
	timeout = random()%(rand_max+1-rand_min) + rand_min;

	/* wait arp reply, and check it */
	tm.tv_sec = timeout/MICROSEC;
	tm.tv_usec = timeout%MICROSEC;
	remaining = timeout - elapsed;
	gettimeofday(&orgtime, NULL);
	while (elapsed <= timeout)
	{
		FD_ZERO(&fdset);
		FD_SET(s, &fdset);
		tm.tv_sec = remaining/MICROSEC;
		tm.tv_usec = remaining%MICROSEC;
		if (select(s + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &tm) < 0)
		{
			if (errno != EINTR) rv = 0;
		}
		else if (FD_ISSET(s, &fdset))
		{
			if (recv(s, &arp, sizeof(arp), 0) < 0 ) rv = 0;
			memcpy(&u32_srcIP, arp.sInaddr, sizeof(u32_srcIP));
			memcpy(&u32_tarIP, arp.tInaddr, sizeof(u32_tarIP));

			if (arp.operation == htons(ARPOP_REPLY) &&
					bcmp(arp.tHaddr, mac, 6) == 0 &&
					u32_srcIP == yiaddr)
			{
				rv = 0;
				break;
			}
			else if(arp.operation == htons(ARPOP_REQUEST) &&
					bcmp(arp.sHaddr, mac, 6) != 0 &&
					u32_tarIP == yiaddr &&
					u32_srcIP == 0)
			{
				rv = 0;
				break;
			}
		}
		gettimeofday(&aftertime, NULL);
		elapsed = (aftertime.tv_sec-orgtime.tv_sec)*MICROSEC + (aftertime.tv_usec-orgtime.tv_usec);
		remaining = timeout - elapsed;
	}
	close(s);
	return rv;
}

static int deo_bridge_mode_rule_set(char *inf)
{
	va_cmd(IPTABLES, 16, 1, FW_DEL, FW_INPUT, ARG_I, inf, "-p",
			ARG_UDP, FW_DPORT, "69", "-d", ARG_255x4, "-m",
			"state", "--state", "NEW", "-j", FW_ACCEPT);
	va_cmd(IPTABLES, 16, 1, FW_ADD, FW_INPUT, ARG_I, inf, "-p",
			ARG_UDP, FW_DPORT, "69", "-d", ARG_255x4, "-m",
			"state", "--state", "NEW", "-j", FW_ACCEPT);
}
int startBridgeIP_v4(char *inf, MIB_CE_ATM_VC_Tp pEntry, CHANNEL_MODE_T ipEncap,
					int *isSyslog, char *msg)
{
	char myip[16], remoteip[16], netmask[16];
	FILE *fp;
	unsigned long broadcastIpAddr;
	char broadcast[16];

	if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED)
	{
		int probe_sent = 0;
		unsigned char conflict = 0;
		unsigned char mibConflict = 0;
		unsigned char devAddr[MAC_ADDR_LEN];

		mib_get_s(MIB_ELAN_MAC_ADDR, (void *)devAddr, sizeof(devAddr));

		while(probe_sent < 3)
		{
			if (do_one_arp_probe(((struct in_addr *)pEntry->ipAddr)->s_addr, devAddr,"br0") == 0)
			{
				conflict = 1;
				break;
			}
			probe_sent++;
		}

		strncpy(myip, inet_ntoa(*((struct in_addr *)pEntry->ipAddr)), 16);
		myip[15] = '\0';

		mib_get_s(MIB_IP_CONFLICT, (void *)&mibConflict,
				sizeof(mibConflict));
		if(conflict && (mibConflict != conflict))
		{
			*isSyslog = 1;
			sprintf(msg, "IPv4 Detect duplicate address %s\n", myip);
			mib_set(MIB_IP_CONFLICT, (void*)&conflict);
		}
		else if(!conflict && (mibConflict != conflict))
		{
			*isSyslog = 1;
			sprintf(msg, "IPv4 Undetect duplicate address %s\n", myip);
			mib_set(MIB_IP_CONFLICT, (void*)&conflict);
		}
		strncpy(remoteip, inet_ntoa(*((struct in_addr *)pEntry->remoteIpAddr)), 16);
		remoteip[15] = '\0';
		strncpy(netmask, inet_ntoa(*((struct in_addr *)pEntry->netMask)), 16);
		netmask[15] = '\0';
		broadcastIpAddr = ((struct in_addr *)pEntry->ipAddr)->s_addr | ~(((struct in_addr *)pEntry->netMask)->s_addr);
		strncpy(broadcast, inet_ntoa(*((struct in_addr *)&broadcastIpAddr)), 16);
		broadcast[15] = '\0';

		va_cmd(IFCONFIG, 6, 1, inf, myip, "netmask", netmask, "broadcast",  broadcast);

		if (isDefaultRouteWan(pEntry))
		{
#ifdef DEFAULT_GATEWAY_V2
			if (ifExistedDGW() == 1)    // Jenny, delete existed default gateway first
				va_cmd(ROUTE, 2, 1, ARG_DEL, "default");
#endif
			/*
			 route add default gw remotip dev nas0_0
			 */
			va_cmd(ROUTE, 4, 1, ARG_ADD, "default", "gw", remoteip, "dev", inf);
		}
		if (ipEncap == CHANNEL_MODE_BRIDGE) {
			unsigned char value[32];
			snprintf(value, 32, "%s.%s", (char *)MER_GWINFO, inf);
			if (fp = fopen(value, "w"))
			{
				/* fprintf(fp, "%s\n", remoteip); */
				fputs(remoteip, fp);
				fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
				chmod(value,0666);
#endif
			}
		}
	}
	else
	{
		int dhcpc_pid;
		unsigned char value[32];
		/*
		 * Enabling support for a dynamically assigned IP (ISP DHCP)...
		 */
#ifdef RESTRICT_PROCESS_PERMISSION
		system("/bin/echo 1 > /proc/sys/net/ipv4/ip_dynaddr");
#else
		if (fp = fopen(PROC_DYNADDR, "w"))
		{
			fprintf(fp, "1\n");
			fclose(fp);
		}
		else
		{
			printf("Open file %s failed !\n", PROC_DYNADDR);
		}
#endif
		snprintf(value, 32, "%s.%s", (char*)DHCPC_PID, inf);
		dhcpc_pid = read_pid((char*)value);
		if (dhcpc_pid > 0)
			kill(dhcpc_pid, SIGTERM);

		setupDHCPClient();
	}

#ifdef ROUTING
	/*
	 *  When interface IP reset, the static route will also be reseted.
	 */
	deleteStaticRoute();
	addStaticRoute();
#endif

	set_ipv4_default_policy_route(pEntry, 1);

	return 1;
}
#endif

/* /var/udhcpc/udhcpc.br0 create */
static void writeDhcpClientScript(char *fname)
{
	int fh;
	int mark;
	char buff[64];
	unsigned char lanIP[16]={0};
#ifdef RESTRICT_PROCESS_PERMISSION
	fh = open(fname, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR|S_IXGRP|S_IXOTH);
#else
	fh = open(fname, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
#endif
	if (fh == -1) {
		printf("Create udhcpc script file %s error!\n", fname);
		return;
	}

	WRITE_DHCPC_FILE(fh, "#!/bin/sh\n");
	snprintf(buff, 64, "RESOLV_CONF=\"/var/udhcpc/resolv.conf.$interface\"\n");
	WRITE_DHCPC_FILE(fh, buff);

	if (!getMIB2Str(MIB_ADSL_LAN_IP, lanIP)) {
		snprintf(buff, 64, "ORIG_LAN_IP=\"%s\"\n", lanIP);
		WRITE_DHCPC_FILE(fh, buff);
	}

	WRITE_DHCPC_FILE(fh, "[ \"$broadcast\" ] && BROADCAST=\"broadcast $broadcast\"\n");
	WRITE_DHCPC_FILE(fh, "[ \"$subnet\" ] && NETMASK=\"netmask $subnet\"\n");
	// WRITE_DHCPC_FILE(fh, "echo interface=$interface\n");
	// WRITE_DHCPC_FILE(fh, "echo ciaddr=$ciaddr\n");
	// WRITE_DHCPC_FILE(fh, "echo ip=$ip\n");
	WRITE_DHCPC_FILE(fh, "if [ \"$ip\" != \"$ciaddr\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "  echo clear_ip\n");
	WRITE_DHCPC_FILE(fh, "  ifconfig $interface 0.0.0.0\n");
	WRITE_DHCPC_FILE(fh, "fi\n");
	//WRITE_DHCPC_FILE(fh, "ifconfig $interface $ip $BROADCAST $NETMASK -pointopoint\n");

	/**************************** Important *****************************/
	/* Should set policy route before write resolv.conf, or there are   */
	/* still some packets may go to wrong pvc.                          */
	/*************************************** ****************************/
	//WRITE_DHCPC_FILE(fh, "ip ro add default via $router dev $interface\n\n");

	WRITE_DHCPC_FILE(fh, "if [ \"$dns\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "  rm $RESOLV_CONF\n");
	WRITE_DHCPC_FILE(fh, "  for i in $dns\n");
	WRITE_DHCPC_FILE(fh, "    do\n");
	WRITE_DHCPC_FILE(fh, "      if [ $i = '0.0.0.0' ]|| [ $i = '255.255.255.255' ]; then\n");
	WRITE_DHCPC_FILE(fh, "        continue\n");
	WRITE_DHCPC_FILE(fh, "      fi\n");
	// WRITE_DHCPC_FILE(fh, "      echo 'DNS=' $i\n");
	//WRITE_DHCPC_FILE(fh, "\techo nameserver $i >> $RESOLV_CONF\n");
	// echo 192.168.88.21@192.168.99.100
	snprintf(buff, 64, "      echo $i@$ip >> $RESOLV_CONF\n");

	WRITE_DHCPC_FILE(fh, buff);
	WRITE_DHCPC_FILE(fh, "    done\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

#ifndef CONFIG_USER_RTK_BRIDGE_MODE
	WRITE_DHCPC_FILE(fh, "ifconfig $interface $ip $BROADCAST $NETMASK -pointopoint\n");
	WRITE_DHCPC_FILE(fh, "ip ro del default dev $interface\n\n");
	WRITE_DHCPC_FILE(fh, "ip ro add default via $router dev $interface\n\n");

	//Microsoft AUTO IP in udhcp-0.9.9-pre2/dhcpc.c, compare first 8 char with "169.254." prefix
	WRITE_DHCPC_FILE(fh, "if [ \"${ip:0:8}\" = \"169.254.\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "  echo \"DHCP sever no response ==> recover to original LAN IP($ORIG_LAN_IP)\"\n");
	WRITE_DHCPC_FILE(fh, "  ifconfig $interface 0.0.0.0\n");
	WRITE_DHCPC_FILE(fh, "  ifconfig $interface $ORIG_LAN_IP\n");
	WRITE_DHCPC_FILE(fh, "fi\n");
#else
	//bridge mode br0 defoult start dhcpc, if br0 can't get IP, not set br0 IP
	WRITE_DHCPC_FILE(fh, "if [ \"${ip:0:8}\" != \"169.254.\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "	ifconfig $interface $ip $BROADCAST $NETMASK -pointopoint\n");
	WRITE_DHCPC_FILE(fh, "ip ro del default dev $interface\n\n");
	WRITE_DHCPC_FILE(fh, "	ip ro add default via $router dev $interface\n\n");
#if 1  /* route info 0.0.0.0 표시 문제패치,  20240207 */
	WRITE_DHCPC_FILE(fh, "\tsysconf send_unix_sock_message /var/run/systemd.usock do_default_gw_wk_set $router $interface 2\n");
#endif
	WRITE_DHCPC_FILE(fh, "fi\n");
#endif

	close(fh);
}
#endif //CONFIG_USER_DHCPCLIENT_MODE

/* /var/udhcpc/udhcpc.nas0_0 create */
static void write_to_dhcpc_script(char *fname, MIB_CE_ATM_VC_Tp pEntry)
{
	int fh;
	int mark;
	char buff[64];
#ifdef DEFAULT_GATEWAY_V2
	unsigned int dgw;
#endif
	char ifname[30];

	ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));
#ifdef RESTRICT_PROCESS_PERMISSION
	fh = open(fname, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR|S_IXGRP|S_IXOTH);
#else
	fh = open(fname, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
#endif

	if (fh == -1) {
		printf("Create udhcpc script file %s error!\n", fname);
		return;
	}

	WRITE_DHCPC_FILE(fh, "#!/bin/sh\n");
	/* Function calculates number of bit in a netmask */
	WRITE_DHCPC_FILE(fh, "mask2cidr() {\n");
	WRITE_DHCPC_FILE(fh, "	nbits=0\n");
	WRITE_DHCPC_FILE(fh, "	IFS=.\n");
	WRITE_DHCPC_FILE(fh, "	for dec in $1 ; do\n");
	WRITE_DHCPC_FILE(fh, "		case $dec in\n");
	WRITE_DHCPC_FILE(fh, "			255) let nbits+=8;;\n");
	WRITE_DHCPC_FILE(fh, "			254) let nbits+=7;;\n");
	WRITE_DHCPC_FILE(fh, "			252) let nbits+=6;;\n");
	WRITE_DHCPC_FILE(fh, "			248) let nbits+=5;;\n");
	WRITE_DHCPC_FILE(fh, "			240) let nbits+=4;;\n");
	WRITE_DHCPC_FILE(fh, "			224) let nbits+=3;;\n");
	WRITE_DHCPC_FILE(fh, "			192) let nbits+=2;;\n");
	WRITE_DHCPC_FILE(fh, "			128) let nbits+=1;;\n");
	WRITE_DHCPC_FILE(fh, "			0);;\n");
	WRITE_DHCPC_FILE(fh, "			*) echo \"Error: $dec is not recognised\"; exit 1\n");
	WRITE_DHCPC_FILE(fh, "		esac\n");
	WRITE_DHCPC_FILE(fh, "	done\n");
	WRITE_DHCPC_FILE(fh, "	 echo \"$nbits\"\n");
	WRITE_DHCPC_FILE(fh, "}\n");
	WRITE_DHCPC_FILE(fh, "numbits=$(mask2cidr $subnet)\n");
	WRITE_DHCPC_FILE(fh, "[ \"$broadcast\" ] && BROADCAST=\"broadcast $broadcast\"\n");
	WRITE_DHCPC_FILE(fh, "[ \"$subnet\" ] && NETMASK=\"netmask $subnet\"\n");

	snprintf(buff, 64, "MER_GW_INFO=\"%s.$interface\"\n", MER_GWINFO);
	WRITE_DHCPC_FILE(fh, buff);
	snprintf(buff, 64, "ROUTER_INFO=\"%s.$interface\"\n", DHCPC_ROUTERFILE);
	WRITE_DHCPC_FILE(fh, buff);
	snprintf(buff, 64, "RESOLV_CONF=\"%s.$interface\"\n", DNS_RESOLV);
	WRITE_DHCPC_FILE(fh, buff);

	WRITE_DHCPC_FILE(fh, "if [ -f \"$MER_GW_INFO\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "	orig_router=`cat $MER_GW_INFO`\n");
	WRITE_DHCPC_FILE(fh, "else\n");
	WRITE_DHCPC_FILE(fh, "	orig_router=\"\"\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

#ifdef USE_DEONET /* sghong, 20231120 */
	WRITE_DHCPC_FILE(fh, "dns1=$(mib get DHCPS_DNS1)\n");
	WRITE_DHCPC_FILE(fh, "dns2=$(mib get DHCPS_DNS2)\n");
	WRITE_DHCPC_FILE(fh, "DNS1=$(echo $dns1 | cut -d \"=\" -f 2)\n");
	WRITE_DHCPC_FILE(fh, "DNS2=$(echo $dns2 | cut -d \"=\" -f 2)\n");
#endif

#if 0
	WRITE_DHCPC_FILE(fh, "echo interface=$interface\n");
	WRITE_DHCPC_FILE(fh, "echo ciaddr=$ciaddr\n");
	WRITE_DHCPC_FILE(fh, "echo ip=$ip\n");
#endif
	WRITE_DHCPC_FILE(fh, "if [ \"$ip\" != \"$ciaddr\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "if [ \"\" == \"$ciaddr\" -o \"0.0.0.0\" == \"$ciaddr\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "ciaddr=`ifconfig $interface 2>/dev/null|awk '/inet addr:/ {print $2}'|sed 's/addr://'`\n");
	WRITE_DHCPC_FILE(fh, "echo \"update ciaddr=\"$ciaddr\n");
	WRITE_DHCPC_FILE(fh, "fi\n");
	WRITE_DHCPC_FILE(fh, "if [ \"\" != \"$ciaddr\" -a \"$ip\" != \"$ciaddr\" ]; then\n");

	WRITE_DHCPC_FILE(fh, "echo clear_ip\n");
	WRITE_DHCPC_FILE(fh, "ifconfig $interface 0.0.0.0\n");
	WRITE_DHCPC_FILE(fh, "fi\n");
	WRITE_DHCPC_FILE(fh, "else\n");
	WRITE_DHCPC_FILE(fh, "if [ \"\" != \"$orig_router\" -a \"$orig_router\" != \"$router\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "echo clear_ip\n");
	WRITE_DHCPC_FILE(fh, "ifconfig $interface 0.0.0.0\n");
	WRITE_DHCPC_FILE(fh, "fi\n");
    WRITE_DHCPC_FILE(fh, "fi\n");

#if 1 //def CONFIG_CWMP_TR181_SUPPORT
	// Collect DHCP Information for Device.DHCPv4.Client.{i}.
	WRITE_DHCPC_FILE(fh, "\nINFO_FILE=\"/tmp/udhcpc_info.$interface\"\n");
	WRITE_DHCPC_FILE(fh, "rm -f $INFO_FILE\n");
	WRITE_DHCPC_FILE(fh, "rm -f $MER_GW_INFO\n");
	WRITE_DHCPC_FILE(fh, "rm -f $ROUTER_INFO\n");

	// IP Address
	WRITE_DHCPC_FILE(fh, "if [ \"$ip\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "	echo ip=$ip >> $INFO_FILE\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

	// Netmask
	WRITE_DHCPC_FILE(fh, "if [ \"$subnet\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "	echo subnet=$subnet >> $INFO_FILE\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

	// DHCP Server
	WRITE_DHCPC_FILE(fh, "if [ \"$siaddr\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "	echo siaddr=$siaddr >> $INFO_FILE\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

	// Gateway info
	WRITE_DHCPC_FILE(fh, "if [ \"$router\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "	echo router=$router >> $INFO_FILE\n");
	WRITE_DHCPC_FILE(fh, "	echo $router > $MER_GW_INFO\n");
	WRITE_DHCPC_FILE(fh, "	echo $router > $ROUTER_INFO\n");
	// WRITE_DHCPC_FILE(fh, "	echo router=$router\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

	// DNS Servers
	WRITE_DHCPC_FILE(fh, "DNS=''\n");
	WRITE_DHCPC_FILE(fh, "rm -f $RESOLV_CONF\n");

#ifdef USE_DEONET /* sghong, 20231229 */
#if 0
	WRITE_DHCPC_FILE(fh, "if [ \"$dns\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "	for i in $dns\n");
	WRITE_DHCPC_FILE(fh, "	do\n");
	WRITE_DHCPC_FILE(fh, "      if [ $i = '0.0.0.0' ]|| [ $i = '255.255.255.255' ]; then\n");
	WRITE_DHCPC_FILE(fh, "          continue\n");
	WRITE_DHCPC_FILE(fh, "      fi\n");
	WRITE_DHCPC_FILE(fh, "		DNS=\"$DNS$i,\"\n");
	WRITE_DHCPC_FILE(fh, "	done\n");
	WRITE_DHCPC_FILE(fh, "  echo dns=$DNS >> $INFO_FILE\n");
	WRITE_DHCPC_FILE(fh, "fi\n");
#endif

	WRITE_DHCPC_FILE(fh, "for idx in 1 2\n");
	WRITE_DHCPC_FILE(fh, "	do\n");
	WRITE_DHCPC_FILE(fh, "		if [ $idx = 1 ]; then\n");
	WRITE_DHCPC_FILE(fh, "			echo $DNS1@$ip >> $RESOLV_CONF\n");
	WRITE_DHCPC_FILE(fh, "		else\n");
	WRITE_DHCPC_FILE(fh, "			echo $DNS2@$ip >> $RESOLV_CONF\n");
	WRITE_DHCPC_FILE(fh, "		fi\n");
	WRITE_DHCPC_FILE(fh, "	done\n");

	WRITE_DHCPC_FILE(fh, "	DNS=\"$DNS1,$DNS2,\"\n");
	WRITE_DHCPC_FILE(fh, "  echo dns=$DNS >> $INFO_FILE\n");
#else
	WRITE_DHCPC_FILE(fh, "if [ \"$dns\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "  for i in $dns\n");
	WRITE_DHCPC_FILE(fh, "  do\n");
	WRITE_DHCPC_FILE(fh, "      if [ $i = '0.0.0.0' ]|| [ $i = '255.255.255.255' ]; then\n");
	WRITE_DHCPC_FILE(fh, "          continue\n");
	WRITE_DHCPC_FILE(fh, "      fi\n");
	WRITE_DHCPC_FILE(fh, "      DNS=\"$DNS$i,\"\n");
	WRITE_DHCPC_FILE(fh, "      echo \"$i@$ip\" >> $RESOLV_CONF\n");
	WRITE_DHCPC_FILE(fh, "  done\n");
	WRITE_DHCPC_FILE(fh, "  echo dns=$DNS >> $INFO_FILE\n");
	// WRITE_DHCPC_FILE(fh, "	echo dns=$DNS\n");
	WRITE_DHCPC_FILE(fh, "fi\n");
#endif

	// Expire time
	WRITE_DHCPC_FILE(fh, "if [ \"$expire\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "	echo expire=$expire >> $INFO_FILE\n");
	// WRITE_DHCPC_FILE(fh, "	echo expire=$expire\n");
	WRITE_DHCPC_FILE(fh, "fi\n");
#endif

	WRITE_DHCPC_FILE(fh, "ip addr add $ip/$numbits broadcast $broadcast dev $interface\n");
#ifndef CONFIG_E8B
// for E8 case, default gateway update by systemd
	if (isDefaultRouteWan(pEntry))
	{
		WRITE_DHCPC_FILE(fh, "if [ \"$orig_router\" != \"$router\" ]; then\n");
#ifdef DEFAULT_GATEWAY_V2
		if (ifExistedDGW() == 1)	// Jenny, delete existed default gateway first
			WRITE_DHCPC_FILE(fh, "\troute del default\n");
#endif
		WRITE_DHCPC_FILE(fh, "\twhile route del -net default gw 0.0.0.0 dev $interface\n");
		WRITE_DHCPC_FILE(fh, "\tdo :\n");
		WRITE_DHCPC_FILE(fh, "\tdone\n\n");
		WRITE_DHCPC_FILE(fh, "\tfor i in $router\n");
		WRITE_DHCPC_FILE(fh, "\tdo\n");
//		WRITE_DHCPC_FILE(fh, "\tifconfig $interface pointopoint $i\n");
		WRITE_DHCPC_FILE(fh, "\troute add -net default gw $i dev $interface\n");
#if 0  /* route info 0.0.0.0 표시 문제패치,  20240207 */
		WRITE_DHCPC_FILE(fh, "\tsysconf send_unix_sock_message /var/run/systemd.usock do_default_gw_wk_set $i $interface 1\n");
#endif
		WRITE_DHCPC_FILE(fh, "\tdone\n");
//		WRITE_DHCPC_FILE(fh, "\tifconfig $interface -pointopoint\n");
		WRITE_DHCPC_FILE(fh, "fi\n");
	}
	else {
#ifdef DEFAULT_GATEWAY_V2	// Jenny, assign default gateway by remote WAN IP
		unsigned char dgwip[16];
		if (mib_get_s(MIB_ADSL_WAN_DGW_ITF, (void *)&dgw, sizeof(dgw)) != 0) {
			if (dgw == DGW_NONE && getMIB2Str(MIB_ADSL_WAN_DGW_IP, dgwip) == 0) {
				if (ifExistedDGW() == 1)
					WRITE_DHCPC_FILE(fh, "\troute del default\n");
				// route add default gw remotip
				snprintf(buff, 64, "\troute add default gw %s\n", dgwip);
				WRITE_DHCPC_FILE(fh, buff);
			}
		}
#endif
	}
#endif

#if 0 //def CONFIG_USER_RTK_WAN_CTYPE
	if (pEntry->applicationtype&X_CT_SRV_TR069)
	{
		char cmd[64];

		//snprintf(ifname,sizeof(ifname),"vc%d",VC_INDEX(pEntry->ifIndex));
		sprintf(buff,"%s.%s", DHCPC_ROUTERFILE, ifname);
		WRITE_DHCPC_FILE(fh, "if [ \"$router\" ]; then\n");
		WRITE_DHCPC_FILE(fh, "\tfor i in $router\n");
		WRITE_DHCPC_FILE(fh, "\tdo\n");
		unlink(buff);
		sprintf(cmd,"\techo $router > %s\n",buff);
		WRITE_DHCPC_FILE(fh, cmd);
		WRITE_DHCPC_FILE(fh, "\tdone\n");
		WRITE_DHCPC_FILE(fh, "fi\n");
	}
#endif

#if defined(PORT_FORWARD_GENERAL) || defined(DMZ)
#ifdef NAT_LOOPBACK
	set_dhcp_NATLB(fh, pEntry);
#endif
#endif

	close(fh);
}

int setup_policy_route_rule(int add)
{
	char cmd[128];
	const char *action = (add) ? "add" : "del";

#ifdef CONFIG_PACKAGE_netifd
	unsigned char rtk_netifd_feature_enable = 0;
	mib_get_s(MIB_RTK_NETIFD_FEATURE, (void *)&rtk_netifd_feature_enable, sizeof(rtk_netifd_feature_enable));
	if (rtk_netifd_feature_enable)
		return rtk_netifd_setup_policy_route_rule(add);
#endif
	// Static Route table
	snprintf(cmd, sizeof(cmd), "%s rule %s from all lookup %d pref %d\n", BIN_IP, action, IP_ROUTE_STATIC_TABLE, IP_RULE_STATIC_ROUTE);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	// LAN bridge Route table
	snprintf(cmd, sizeof(cmd), "%s rule %s from all lookup %d pref %d\n", BIN_IP, action, IP_ROUTE_LAN_TABLE, IP_RULE_PRI_LANRT);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef CONFIG_IPV6
	// v6 Static Route table
	snprintf(cmd, sizeof(cmd), "%s -6 rule %s from all lookup %d pref %d\n", BIN_IP, action, IP_ROUTE_STATIC_TABLE, IP_RULE_STATIC_ROUTE);
#ifdef CONFIG_UPSTREAM_ISOLATION
	uint32_t from_lan_with_wan_mark = 0, from_lan_with_wan_mask = 0;
	char polmark[64] = {0}, iprule_pri[16] = {0};

	/* This case is for portmapping LAN isolation case, Like LAN1 PC to LAN2 PC. downstream package should hit IPv6 LAN route.
	 * We don't want to downstream hit from_lan_with_wan_mark, so mark value is 0 not SOCK_FROM_LAN_BIT_MASK.
	 */
	from_lan_with_wan_mark = 0x0;
	from_lan_with_wan_mask = SOCK_FROM_LAN_BIT_MASK;
	sprintf(polmark, "0x%x/0x%x", from_lan_with_wan_mark, from_lan_with_wan_mask);

	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	// v6 LAN bridge Route table
	snprintf(cmd, sizeof(cmd), "%s -6 rule %s from all fwmark %s lookup %d pref %d\n", BIN_IP, action, polmark, IP_ROUTE_LAN_TABLE, IP_RULE_PRI_LANRT);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	// v6 NPT Route table
	snprintf(cmd, sizeof(cmd), "%s -6 rule %s from all fwmark %s lookup %d pref %d",  BIN_IP, action, polmark, IP_ROUTE_LAN_V6NPT_TABLE, IP_RULE_PRI_WAN_NPT_PD_POLICYRT);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	// v6 LAN PD Route table
	snprintf(cmd, sizeof(cmd), "%s -6 rule %s from all fwmark %s lookup %d pref %d\n", BIN_IP, action, polmark, IP_ROUTE_LAN_V6PD_TABLE, LAN_DHCP6S_PD_ROUTE);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
#else
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	// v6 LAN bridge Route table
	snprintf(cmd, sizeof(cmd), "%s -6 rule %s from all lookup %d pref %d\n", BIN_IP, action, IP_ROUTE_LAN_TABLE, IP_RULE_PRI_LANRT);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	// v6 NPT Route table
	snprintf(cmd, sizeof(cmd), "%s -6 rule %s from all lookup %d pref %d",  BIN_IP, action, IP_ROUTE_LAN_V6NPT_TABLE, IP_RULE_PRI_WAN_NPT_PD_POLICYRT);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	// v6 LAN PD Route table
	snprintf(cmd, sizeof(cmd), "%s -6 rule %s from all lookup %d pref %d\n", BIN_IP, action, IP_ROUTE_LAN_V6PD_TABLE, LAN_DHCP6S_PD_ROUTE);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
#endif
	return 0;
}

int set_ipv4_default_policy_route(MIB_CE_ATM_VC_Tp pEntry, int set)
{
	uint32_t wan_mark = 0, wan_mask = 0;
	char polmark[64] = {0}, iprule_pri[16] = {0};
	char ifName[IFNAMSIZ];

	ifGetName(pEntry->ifIndex, ifName, sizeof(ifName));
	if(rtk_net_get_wan_policy_route_mark(ifName, &wan_mark, &wan_mask) == -1)
	{
		AUG_PRT("%s: Error to get WAN policy mark for ifindex(%u)", __FUNCTION__, pEntry->ifIndex);
		return 0;
	}
#ifdef CONFIG_YUEME
	//if approute bind WAN not up, dont prohibit
	wan_mask |= SOCK_MARK_APPROUTE_ACT_BIT_MASK;
#endif
	sprintf(polmark, "0x%x/0x%x", wan_mark, wan_mask);
	sprintf(iprule_pri, "%d", IP_RULE_PRI_DEFAULT);

	va_cmd(BIN_IP, 5, 1, "rule", "del", "fwmark", polmark, "prohibit");

	if(!set)
	{
		return 0;
	}

	va_cmd(BIN_IP, 7, 1, "rule", "add", "fwmark", polmark, "pref", iprule_pri, "prohibit");

	return 0;
}

extern char *inet_ntoa_r(struct in_addr in,char* buf);
int set_ipv4_policy_route(MIB_CE_ATM_VC_Tp pEntry, int set)
{
	int tbl_id, isppp = 0, count, ret, flags = 0;
	char str_tblid[10]={0};
	char ifname[IFNAMSIZ+1] = {0};
	char buf[128] = {0};
	FILE *fp = NULL;
	char myip[16] = {0}, remoteip[16] = {0}, netmask[16] = {0};
	char *tmpc = NULL;
	struct in_addr ip, mask;
	uint32_t wan_mark = 0, wan_mask = 0;
	char polmark[64] = {0}, iprule_pri_sourcert[16] = {0}, iprule_pri_policyrt[16] = {0};
	char lanNetAddr[20];

	if(pEntry == NULL) {
		AUG_PRT("Error! NULL input!\n");
		return -1;
	}
	ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));

	/*if(pEntry->dgw & IPVER_IPV4) {
		AUG_PRT("pEntry(%s) is enable default gateway, ignore it.\n", ifname);
		return 0;
	}*/

	tbl_id = caculate_tblid(pEntry->ifIndex);

	if (set == 3 || set == 4)
	{
		tbl_id += ITF_SOURCE_ROUTE_SIMU_START;
		set -= 3;
		strcat(ifname, "_0");
	}

	snprintf(str_tblid, 8, "%d", tbl_id);
	if(rtk_net_get_wan_policy_route_mark(ifname, &wan_mark, &wan_mask) == -1)
	{
		AUG_PRT("%s: Error to get WAN policy mark for ifindex(%u)", __FUNCTION__, pEntry->ifIndex);
		return 0;
	}
	sprintf(polmark, "0x%x/0x%x", wan_mark, wan_mask);

	sprintf(iprule_pri_sourcert, "%d", IP_RULE_PRI_SRCRT);
	sprintf(iprule_pri_policyrt, "%d", IP_RULE_PRI_POLICYRT);

	//AUG_PRT("%s source rouing for ifname (%s), table(%s)\n", ((!set)?"Reset":"Update"), ifname, str_tblid);

	if(pEntry->cmode == CHANNEL_MODE_PPPOE || pEntry->cmode == CHANNEL_MODE_PPPOA)
	{
		isppp = 1;
		getIPaddrInfo(pEntry, myip, netmask, remoteip);
	}
	else
	{
		if(pEntry->ipDhcp == DHCP_DISABLED)
		{
			strncpy(myip, inet_ntoa(*((struct in_addr *)pEntry->ipAddr)), 15);
			strncpy(remoteip, inet_ntoa(*((struct in_addr *)pEntry->remoteIpAddr)), 15);
			strncpy(netmask, inet_ntoa(*((struct in_addr *)pEntry->netMask)), 15);
		}
		else
		{
			sprintf(buf, "/tmp/udhcpc_info.%s", ifname);
			fp = fopen(buf, "r");
			if(fp)
			{
				while(!feof(fp))
				{
					if(fgets(buf, sizeof(buf)-1, fp))
					{
						tmpc = strrchr(buf, '\r'); if(tmpc) *tmpc = '\0';
						tmpc = strrchr(buf, '\n'); if(tmpc) *tmpc = '\0';
						if(strncmp(buf, "ip=", 3) == 0)
							strncpy(myip, buf+3, 15);
						else if(strncmp(buf, "subnet=", 7) == 0)
							strncpy(netmask, buf+7, 15);
						else if(strncmp(buf, "router=", 7) == 0)
							strncpy(remoteip, buf+7, 15);
					}
				}
				fclose(fp);
			}
		}
	}

	if(myip[0]){
		va_cmd(BIN_IP, 7, 1, "-4", "rule", "del", "from", myip, "table", str_tblid);
	}
	va_cmd(BIN_IP, 7, 1, "-4", "rule", "del", "fwmark", polmark, "table", str_tblid);
	va_cmd(BIN_IP, 5, 1, "-4", "rule", "del", "table", str_tblid);
	va_cmd(BIN_IP, 5, 1, "-4", "route", "flush", "table", str_tblid);

	if(set && getInFlags(ifname, &flags))
	{
		if(set == 2 && !((flags & IFF_UP) && (flags & IFF_RUNNING)))
		{
			AUG_PRT("ifname (%s) is not running, dont't update policy route table(%s)\n", ifname, str_tblid);
			return 1;
		}

		if(myip[0]){
			va_cmd("/usr/bin/ip", 9, 1, "-4", "rule", "add", "from", myip, "pref", iprule_pri_sourcert, "table", str_tblid);
		}
		va_cmd("/usr/bin/ip", 9, 1, "-4", "rule", "add", "fwmark", polmark, "pref", iprule_pri_policyrt, "table", str_tblid);

		if(!isppp)
		{
			inet_aton(myip, &ip);
			inet_aton(netmask, &mask);
			ip.s_addr = ip.s_addr & mask.s_addr;
			char static_route_cmd[256] = {0};

			unsigned int i = 31;
			unsigned int slash = 0;
			unsigned int mask_val = ntohl(mask.s_addr);
			while(i && ((mask_val >> i)&1))
			{
				slash++;
				i--;
			}
			buf[0] = 0;
			if((tmpc = (char*)inet_ntoa_r(ip, buf)))
			{
				//WAN IP domain route
				//sprintf(tmpc+strlen(tmpc), "/%d", slash);
				sprintf(buf + strlen(buf), "/%d", slash);
				va_cmd(BIN_IP, 8, 1, "-4", "route", "add", buf, "dev", ifname, "table", str_tblid);
			}

			buf[0] = 0;
			snprintf(buf, sizeof(buf), "/tmp/udhcpc_static_route.%s", ifname);
			fp = fopen(buf, "r");
			if (fp)
			{
				while (!feof(fp))
				{
					if (fgets(buf, sizeof(buf)-1, fp))
					{
						tmpc = strrchr(buf, '\r'); if (tmpc) *tmpc = '\0';
						tmpc = strrchr(buf, '\n'); if (tmpc) *tmpc = '\0';
						snprintf(static_route_cmd, sizeof(static_route_cmd), "/usr/bin/ip -4 route add %s dev %s table %s", buf, ifname, str_tblid);
						va_cmd("/bin/sh", 2, 1, "-c", static_route_cmd);
					}
				}
				fclose(fp);
			}

			if(remoteip[0]){
				va_cmd(BIN_IP, 10, 1, "-4", "route", "add", "default", "via", remoteip, "dev", ifname, "table", str_tblid);
			}
		}
		else
		{
			va_cmd(BIN_IP, 8, 1, "-4", "route", "add", "default", "dev", ifname, "table", str_tblid);
		}
	}

	return 0;
}

int set_ipv4_lan_policy_route(char *lan_infname, struct in_addr *ip4addr, unsigned char mask_len, int set)
{
	char ip4addr_str[INET_ADDRSTRLEN] = {0};
	int flags = 0;
	char ipv4_ip_mask_cidr_str[INET_ADDRSTRLEN + 3] = {0};
	char str_tblid[10] = {0};
	struct in_addr route_v4_ip;
	struct in_addr mask;

	if (lan_infname == NULL || lan_infname[0] == '\0') {
		AUG_PRT("Error! NULL lan_infname input!\n");
		return -1;
	}
	if (ip4addr == NULL || (inet_ntop(PF_INET, ip4addr, ip4addr_str, sizeof(ip4addr_str)) <= 0))
		return -1;

	snprintf(str_tblid, sizeof(str_tblid), "%d", (char *)IP_ROUTE_LAN_TABLE);
	AUG_PRT("%s IPv4 LAN route for ifname (%s), table(%s)\n", ((set) ? "Set":"Del"), lan_infname, str_tblid);
	//va_cmd(BIN_IP, 5, 1, "-4", "route", "flush", "table", str_tblid);
	route_v4_ip.s_addr = ip4addr->s_addr & cidr_to_netmask(mask_len);
	if (inet_ntop(PF_INET, &route_v4_ip, ip4addr_str, sizeof(ip4addr_str)) <= 0)
		return -1;

	if (set && getInFlags(lan_infname, &flags))
	{
		if (!(flags & IFF_UP)) //IFF_RUNNING, br0 is not running if no phy port carrier on
		{
			AUG_PRT("lan_infname (%s) is not up, dont't update policy route table(%s)\n", lan_infname, str_tblid);
			return 1;
		}

		//LAN IP domain route
#if 0
		get_v4_interface_addr_net_info(ipv4_ip_mask_cidr_str, sizeof(ipv4_ip_mask_cidr_str), lan_infname);
		if (ipv4_ip_mask_cidr_str[0] != '\0')
			va_cmd(BIN_IP, 8, 1, "-4", "route", "add", ipv4_ip_mask_cidr_str, "dev", lan_infname, "table", str_tblid);
#endif
		snprintf(ipv4_ip_mask_cidr_str, sizeof(ipv4_ip_mask_cidr_str), "%s/%d", ip4addr_str, mask_len);
		va_cmd(BIN_IP, 8, 1, "-4", "route", "add", ipv4_ip_mask_cidr_str, "dev", lan_infname, "table", str_tblid);
	}
	else
	{
		snprintf(ipv4_ip_mask_cidr_str, sizeof(ipv4_ip_mask_cidr_str), "%s/%d", ip4addr_str, mask_len);
		va_cmd(BIN_IP, 8, 1, "-4", "route", "del", ipv4_ip_mask_cidr_str, "dev", lan_infname, "table", str_tblid);
	}
	return 0;
}

// DHCP client configuration
// return value:
// 1  : successful
int startDhcpc(char *inf, MIB_CE_ATM_VC_Tp pEntry, int is_diag)
{
	unsigned char value[32], value2[32];
	char devName[MAX_DEV_NAME_LEN]={0};
	FILE *fp;
	DNS_TYPE_T dnsMode;
	char * argv[16];

	int i, vcTotal, resolvopt, dhcpc_idx=1;

	mib_get_s(MIB_DEVICE_NAME, (void *)devName, sizeof(devName));
	if(devName[0] == '\0')
	{
		strcpy(devName, DEFAULT_DEVICE_NAME); //if devName is NULL
	}

	argv[1] = inf;
	argv[2] = "up";
	argv[3] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s\n", IFCONFIG, argv[1], argv[2]);
	do_cmd(IFCONFIG, argv, 1);

	// udhcpc -i vc0 -p pid -s script
	argv[dhcpc_idx++] = (char *)ARG_I;
	argv[dhcpc_idx++] = inf;
	if(is_diag)
		argv[dhcpc_idx++] = "-d";	//diagnostics
	argv[dhcpc_idx++] = "-p";
	snprintf(value2, 32, "%s.%s", (char*)DHCPC_PID, inf);
	argv[dhcpc_idx++] = (char *)value2;

#ifdef CONFIG_00R0
	// before start, wait for 10 seconds or SIGUSR1
	argv[dhcpc_idx++] = "-D";
	argv[dhcpc_idx++] = "10";
#endif

	argv[dhcpc_idx++] = "-s";
	snprintf(value, 32, "%s.%s", (char *)DHCPC_SCRIPT_NAME, inf);
#ifdef CONFIG_RTK_DEV_AP
	int pid;
	pid = read_pid(value2);
	if (pid > 0)
		return 0;
#endif
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	if(!strcmp(ALIASNAME_BR0, inf))
		writeDhcpClientScript(value);
	else
		write_to_dhcpc_script(value, pEntry);
#else
	write_to_dhcpc_script(value, pEntry);
#endif
	argv[dhcpc_idx++] = (char *)DHCPC_SCRIPT;

	// Add option 12 Host Name
	if(strlen(devName) > 0){
		argv[dhcpc_idx++] = "-H";
		argv[dhcpc_idx++] = (char *)devName;
		argv[dhcpc_idx++] = NULL;
	}
	else
		argv[dhcpc_idx++] = NULL;

#ifndef DISABLE_MICROSOFT_AUTO_IP_CONFIGURATION
	if (strcmp(inf, LANIF) == 0)
	{
		// LAN interface
		// enable Microsoft auto IP configuration
		dhcpc_idx--;
		argv[dhcpc_idx++] = "-a";
		argv[dhcpc_idx++] = NULL;
	}
#endif

	TRACE(STA_SCRIPT, "%s %s %s %s ", DHCPC, argv[1], argv[2], argv[3]);
	TRACE(STA_SCRIPT, "%s %s %s\n", argv[4], argv[5], argv[6]);
	do_nice_cmd(DHCPC, argv, 0);

#ifdef CONFIG_RTK_DEV_AP
	start_process_check_pidfile((const char*)value2);
#endif
	return 1;
}

int reWriteAllDhcpcScript()
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char wanif[IFNAMSIZ];
	unsigned char value[32];

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
			printf("reWriteAllDhcpcScript: cannot get ATM_VC_TBL(ch=%d) entry\n", i);
			return 0;
		}

		if ((DHCP_T)Entry.ipDhcp == DHCP_CLIENT)
		{
			ifGetName(PHY_INTF(Entry.ifIndex),wanif,sizeof(wanif));
			snprintf(value, 32, "%s.%s", (char *)DHCPC_SCRIPT_NAME, wanif);
			write_to_dhcpc_script(value, &Entry);
		}

	}
	return 0;
}

int isDhcpProcessExist(unsigned int ifIndex)
{
    int pid;
    unsigned char ifname[IFNAMSIZ];
    unsigned char pid_file[32];

    ifGetName(PHY_INTF(ifIndex),ifname,sizeof(ifname));
    snprintf(pid_file, 32, "%s.%s", (char*)DHCPC_PID, ifname);
    pid = read_pid((char*)pid_file);
    if(pid>0)
    {
        if(kill(pid, 0) == 0) //send a meaasge "zero" to PID, if process exist, return 0
        {
            return 1;
        }
        else
        {
            //pid file exist but process has been killed.
            return 0;
        }
    }
    else
    {
        //pid file not exist.
        return 0;
    }
}

#ifdef CONFIG_USER_DHCP_SERVER
#ifdef SUPPORT_DHCP_RESERVED_IPADDR
static const char DHCPReservedIPAddrFile[] = "/var/udhcpd/DHCPReservedIPAddr.txt";
static int setupDHCPReservedIPAddr( void )
{
	FILE *fp;

	fp=fopen( DHCPReservedIPAddrFile, "w" );
	if(!fp) return -1;

	{//default pool
		unsigned int j,num;
		num = mib_chain_total( MIB_DHCP_RESERVED_IPADDR_TBL );
		fprintf( fp, "START %u\n", 0 );
		for( j=0;j<num;j++ )
		{
			MIB_DHCP_RESERVED_IPADDR_T entryip;
			if(!mib_chain_get( MIB_DHCP_RESERVED_IPADDR_TBL, j, (void*)&entryip ))
				continue;
			if( entryip.InstanceNum==0 )
			{
				fprintf( fp, "%s\n",  inet_ntoa(*((struct in_addr *)&(entryip.IPAddr))) );
			}
		}
		fprintf( fp, "END\n" );
	}

#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
{
	unsigned int i,numpool;
	numpool = mib_chain_total( MIB_DHCPS_SERVING_POOL_TBL );
	for( i=0; i<numpool;i++ )
	{
		unsigned int j,num;
		DHCPS_SERVING_POOL_T entrypool;

		if( !mib_chain_get( MIB_DHCPS_SERVING_POOL_TBL, i, (void*)&entrypool ) )
			continue;

		//skip disable or relay pools
		if( entrypool.enable==0 || entrypool.localserved==0 )
			continue;

		num = mib_chain_total( MIB_DHCP_RESERVED_IPADDR_TBL );
		fprintf( fp, "START %u\n", entrypool.InstanceNum );
		for( j=0;j<num;j++ )
		{
			MIB_DHCP_RESERVED_IPADDR_T entryip;
			if(!mib_chain_get( MIB_DHCP_RESERVED_IPADDR_TBL, j, (void*)&entryip ))
				continue;
			if( entryip.InstanceNum==entrypool.InstanceNum )
			{
				fprintf( fp, "%s\n",  inet_ntoa(*((struct in_addr *)&(entryip.IPAddr))) );
			}
		}
		fprintf( fp, "END\n" );
	}
}
#endif //_PRMT_X_TELEFONICA_ES_DHCPOPTION_

	fclose(fp);
	return 0;
}

int clearDHCPReservedIPAddrByInstNum(unsigned int instnum)
{
	int j,num;
	num = mib_chain_total( MIB_DHCP_RESERVED_IPADDR_TBL );
	if( num>0 )
	{
		for( j=num-1;j>=0;j-- )
		{
			MIB_DHCP_RESERVED_IPADDR_T entryip;
			if(!mib_chain_get( MIB_DHCP_RESERVED_IPADDR_TBL, j, (void*)&entryip ))
				continue;
			if( entryip.InstanceNum==instnum )
			{
				mib_chain_delete( MIB_DHCP_RESERVED_IPADDR_TBL, j );
			}
		}
	}
	return 0;
}
#endif //SUPPORT_DHCP_RESERVED_IPADDR

int rtk_get_dns_from_file(char *filename, char *dns1, char *dns2 )
{
	FILE *fd = NULL;
	char line[128] = {0};
	int ret=RTK_FAILED;
	fd = fopen(filename, "r");
	if(fd)
	{
		char *p = NULL;
		int cnt = 0;
		while(fgets(line,sizeof(line),fd))
		{
			if((strlen(line)==0))
			continue;

			p = strchr(line, '@');
			if (p)
			{
				if(cnt == 0)
				{
					memcpy(dns1, line, p-line);
					dns1[p-line] = '\0';
					cnt++;
				}
				else
				{
					memcpy(dns2, line, p-line);
					dns2[p-line] = '\0';
					cnt++;
					break;
				}
			}
		}
		fclose(fd);
		ret = RTK_SUCCESS;

	}
	return ret;

}

#ifdef CONFIG_USER_DNS_RELAY_PROXY
int rtk_get_dns_by_wanifname(char *dns1, char *dns2, char *wanifname, unsigned char *dns_mode)
{
	int total, i;
	int ret=RTK_FAILED, get_entry_temp=0;
	unsigned int ifindex=0;
	MIB_CE_ATM_VC_T Entry;
	unsigned char zero[IP_ADDR_LEN] = {0};
	struct in_addr in, *pIn;
	char ifname[IFNAMSIZ]={0};

	total = mib_chain_total(MIB_ATM_VC_TBL);
	if (total > MAX_VC_NUM)
		return RTK_FAILED;

	for (i = 0; i < total; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			printf("Get chain record error!\n");
			return RTK_FAILED;
		}

		if (Entry.enable == 0)
			continue;

		ifGetName(Entry.ifIndex, ifname, sizeof(ifname));

		if(!strcmp(ifname, wanifname)) {
			get_entry_temp=1;
			break;
		}
	}

	if(get_entry_temp)
	{

		*dns_mode= Entry.dnsMode;

		if ((Entry.dnsMode == REQUEST_DNS_NONE)||(Entry.dnsMode == DNS_SET_BY_API))
		{

			if(memcmp(zero, Entry.v4dns1, IP_ADDR_LEN) != 0)
			{
				inet_ntop(AF_INET, Entry.v4dns1, dns1, INET_ADDRSTRLEN);
			}
			if(memcmp(zero, Entry.v4dns2, IP_ADDR_LEN) != 0)
			{
				inet_ntop(AF_INET, Entry.v4dns2, dns2, INET_ADDRSTRLEN);
			}

		}
	    else
		{
			//auto
			char line[128] = {0};

			if ((DHCP_T)Entry.ipDhcp == DHCP_CLIENT)
				snprintf(line, 64, "%s.%s", (char *)DNS_RESOLV, wanifname);
			if (Entry.cmode == CHANNEL_MODE_PPPOE || Entry.cmode == CHANNEL_MODE_PPPOA)
				snprintf(line, 64, "%s.%s", (char *)PPP_RESOLV, wanifname);

			rtk_get_dns_from_file(line, dns1, dns2);


	   }
	}
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD) || defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	else
	{
		//auto

		char line[128] = {0};
		*dns_mode= (int)REQUEST_DNS;
		snprintf(line, 64, "%s.%s", (char *)PPP_RESOLV, wanifname);
		rtk_get_dns_from_file(line, dns1, dns2);

	}
#endif

		ret=RTK_SUCCESS;

	return ret;
}

int rtk_get_dns_and_restart_sign(char *dns1, char *dns2, int *short_lease_flag, int *dns_relay_flag)
{
	int ret=RTK_FAILED;
	char ifname[IFNAMSIZ]={0};
	unsigned char dns_request_mode=0, landns_mode=0;
	unsigned int wan_index;
	struct in_addr dns;

	if((dns1==NULL)||(dns2==NULL)||(short_lease_flag==NULL)||(dns_relay_flag==NULL))
	{
		printf("[%s:%d] Null pointer!\n", __FUNCTION__, __LINE__);
		return RTK_FAILED;
	}


#ifdef CONFIG_USER_RTK_BRIDGE_MODE
	unsigned char op_mode;
	op_mode=getOpMode();
	if(op_mode!=GATEWAY_MODE)
	{
		*short_lease_flag=0;
		*dns_relay_flag=0;
		return RTK_SUCCESS;
	}
#endif

	if(mib_get_s(MIB_LAN_DNSV4_MODE, (void *)&landns_mode, sizeof(landns_mode))==0)
	{
		printf("[%s:%d]get mib MIB_LAN_DNSV4_MODE fail!\n", __FUNCTION__, __LINE__);
		return RTK_FAILED;
	}
	switch(landns_mode){

	case IPV4_DNS_PROXY:
		*short_lease_flag=0;
		*dns_relay_flag=0;
		ret=RTK_SUCCESS;
	break;

	case IPV4_DNS_WANCONN_SPECIFIED:

		mib_get_s(MIB_DNSINFOV4_WANCONN, (void *)&wan_index, sizeof(wan_index));

		ifGetName(wan_index, ifname, sizeof(ifname));
		if(rtk_get_dns_by_wanifname(dns1, dns2, ifname, &dns_request_mode)==RTK_SUCCESS)
		{
			if(dns_request_mode==REQUEST_DNS)
			{
				if((strlen(dns1)==0)&&(strlen(dns2)==0))
				{
					*short_lease_flag=1;
					*dns_relay_flag=0;
				}
				else
				{
					*short_lease_flag=0;
					*dns_relay_flag=1;
				}

			}
			else
			{
				if((strlen(dns1)==0)&&(strlen(dns2)==0))
				{
					*short_lease_flag=0;
					*dns_relay_flag=0;
				}
				else
				{
					*short_lease_flag=0;
					*dns_relay_flag=1;
				}

			}
			ret=RTK_SUCCESS;
		}

	break;

	case IPV4_DNS_STATIC:

		*short_lease_flag=0;

		mib_get_s(MIB_ADSL_LANV4_DNS1, (void *)&dns, sizeof(dns));
		if((unsigned int)dns.s_addr)
		{
			strncpy(dns1, inet_ntoa(dns), 16);
			dns1[15] = '\0';
		}
		mib_get_s(MIB_ADSL_LANV4_DNS2, (void *)&dns, sizeof(dns));
		if((unsigned int)dns.s_addr)
		{
			strncpy(dns2, inet_ntoa(dns), 16);
			dns2[15] = '\0';
		}
		if((strlen(dns1)==0)&&(strlen(dns2)==0))
			*dns_relay_flag=0;
		else
			*dns_relay_flag=1;

		ret=RTK_SUCCESS;

	break;

	default:
		ret=RTK_FAILED;
	break;

	}

	return ret;

}

void write_dns_to_confile(char *dns1, char *dns2, char *dns_proxy, FILE *fp, int dns_relay_flag)
{

	if(dns_relay_flag)
	{
		if(strlen(dns1))
			fprintf(fp, "opt dns %s\n", dns1);
		if(strlen(dns2))
			fprintf(fp, "opt dns %s\n", dns2);
	}
	else
	{
		fprintf(fp, "opt dns %s\n", dns_proxy);
	}


}

#endif

int get_dns_from_wan(char *dns1, int len1, char *dns2, int len2, char *dns3, int len3)
{
#ifdef CONFIG_USER_DNS_RELAY_PROXY
	int short_lease_flag = 0, dns_relay_flag = 0;
	if(rtk_get_dns_and_restart_sign(dns1, dns2, &short_lease_flag, &dns_relay_flag) == RTK_SUCCESS) {
		if(dns_relay_flag != 0) {
			if(dns1[0] || dns2[0])
				return 1;
		}
	}
#else
	MIB_CE_ATM_VC_T vcEntry;
	int i, vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	char wan_name[IFNAMSIZ+1]={0}, filepath[64]={0};
	unsigned int flags = 0;
	for(i=0;i<vcEntryNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry))
			continue;
		if (!vcEntry.enable || vcEntry.cmode == CHANNEL_MODE_BRIDGE)
			continue;
#ifdef CONFIG_IPV6
		if(!(vcEntry.IpProtocol & IPVER_IPV4))
			continue;
#endif
		if(!isDefaultRouteWanV4(&vcEntry))
			continue;

		ifGetName(vcEntry.ifIndex, wan_name, sizeof(wan_name));
		if (getInFlags(wan_name, &flags) == 1 &&
			(flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING))
		{
			if ((DHCP_T)vcEntry.ipDhcp == DHCP_CLIENT)
				snprintf(filepath, 64, "%s.%s", (char *)DNS_RESOLV, wan_name);
			else if (vcEntry.cmode == CHANNEL_MODE_PPPOE || vcEntry.cmode == CHANNEL_MODE_PPPOA)
				snprintf(filepath, 64, "%s.%s", (char *)PPP_RESOLV, wan_name);
			rtk_get_dns_from_file(filepath, dns1, dns2);
			if(dns1[0] || dns2[0])
				return 1;
		}
	}
#endif
	return 0;
}

// DHCP server configuration
// return value:
// 0  : not active
// 1  : active
// -1 : setup failed
int setupDhcpd(void)
{
	unsigned char value[64], value1[64], value3[64], value4[64], value5[64];
	unsigned int uInt, uLTime;
	unsigned char dnsMode;
	FILE *fp=NULL, *fp2=NULL;
	char ipstart[16], ipend[16], vChar, optionStr[254];
#ifdef IMAGENIO_IPTV_SUPPORT
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
	char opchaddr[16]={0}, stbdns1[16]={0}, stbdns2[16]={0};
	unsigned short opchport;
#endif
/*ping_zhang:20090930 END*/
#endif
	char subnet[16], ipaddr[16], ipaddr2[16];
	char dhcpsubnet[16];
	char dns1[16], dns2[16], dns3[16], dhcps[16];
	char domain[MAX_NAME_LEN];
	unsigned int entryNum, i, j;
	MIB_CE_MAC_BASE_DHCP_T Entry;
#ifdef IP_PASSTHROUGH
	int ippt;
	unsigned int ippt_itf;
#endif
	int spc_enable=0, spc_ip=0;
	struct in_addr myip;
	//ql: check if pool is in first IP subnet or in secondary IP subnet.
	int subnetIdx=0;
	struct in_addr lanip1, lanmask1, lanip2, lanmask2, inpoolstart;
	char serverip[16];

/*ping_zhang:20080919 START:add for new telefonica tr069 request: dhcp option*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	DHCPS_SERVING_POOL_T dhcppoolentry;
	int dhcppoolnum;
	unsigned char macempty[6]={0,0,0,0,0,0};
	char *ipempty="0.0.0.0";
#endif
	int short_lease_flag=0;
	char macaddr[20];
	unsigned char tmpdns1[4], tmpdns2[4];
	int fixed_dns;

/*ping_zhang:20080919 END*/

	// check if dhcp server on ?
	// Modified by Mason Yu for dhcpmode
	//if (mib_get_s(MIB_ADSL_LAN_DHCP, (void *)value, sizeof(value)) != 0)
	if (mib_get_s(MIB_DHCP_MODE, (void *)value, sizeof(value)) != 0)
	{
		uInt = (unsigned int)(*(unsigned char *)value);

#ifdef CONFIG_AUTO_DHCP_CHECK
		if (uInt != DHCPV4_LAN_SERVER && uInt!=AUTO_DHCPV4_BRIDGE)
#else
		if (uInt != DHCPV4_LAN_SERVER)
#endif
		{
			return 0;	// dhcp Server not on
		}
	}
#ifdef CONFIG_RTK_DEV_AP
	int pid;
	pid = read_pid(DHCPSERVERPID);
	if (pid > 0)
		return 0;
#endif
#ifdef CONFIG_SECONDARY_IP
	mib_get_s(MIB_ADSL_LAN_ENABLE_IP2, (void *)value, sizeof(value));
	if (value[0])
		mib_get_s(MIB_ADSL_LAN_DHCP_POOLUSE, (void *)value, sizeof(value));
#else
	value[0] = 0;
#endif

#ifdef DHCPS_POOL_COMPLETE_IP
	mib_get_s(MIB_DHCP_SUBNET_MASK, (void *)value, sizeof(value));
	strncpy(dhcpsubnet, inet_ntoa(*((struct in_addr *)value)), 16);
	dhcpsubnet[15] = '\0';
	// IP Pool start
	mib_get_s(MIB_DHCP_POOL_START, (void *)value, sizeof(value));
	ipstart[sizeof(ipstart)-1]='\0';
	strncpy(ipstart, inet_ntoa(*((struct in_addr *)value)), sizeof(ipstart)-1);
	// IP Pool end
	mib_get_s(MIB_DHCP_POOL_END, (void *)value, sizeof(value));
	ipend[sizeof(ipend)-1]='\0';
	strncpy(ipend, inet_ntoa(*((struct in_addr *)value)), sizeof(ipend)-1);
#else
	if (value[0] == 0) { // primary LAN
		if (mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)value, sizeof(value)) != 0)
		{
			strncpy(dhcpsubnet, inet_ntoa(*((struct in_addr *)value)), 16);
			dhcpsubnet[15] = '\0';
		}
		else
			return -1;

		mib_get_s(MIB_ADSL_LAN_IP, (void *)value, sizeof(value));
	}
	else { // secondary LAN
		if (mib_get_s(MIB_ADSL_LAN_SUBNET2, (void *)value, sizeof(value)) != 0)
		{
			strncpy(dhcpsubnet, inet_ntoa(*((struct in_addr *)value)), 16);
			dhcpsubnet[15] = '\0';
		}
		else
			return -1;

		mib_get_s(MIB_ADSL_LAN_IP2, (void *)value, sizeof(value));
	}

	// IP pool start address
	mib_get_s(MIB_ADSL_LAN_CLIENT_START, (void *)&value[3], sizeof(value[3]));
		strncpy(ipstart, inet_ntoa(*((struct in_addr *)value)), 16);
		ipstart[15] = '\0';
	// IP pool end address
	mib_get_s(MIB_ADSL_LAN_CLIENT_END, (void *)&value[3], sizeof(value[3]));
		strncpy(ipend, inet_ntoa(*((struct in_addr *)value)), 16);
		ipend[15] = '\0';
#endif

	// Added by Mason Yu for MAC base assignment. Start
	if ((fp2 = fopen("/var/dhcpdMacBase.txt", "w")) == NULL)
	{
		printf("***** Open file /var/dhcpdMacBase.txt failed !\n");
		goto dhcpConf;
	}

//star: reserve local ip for dhcp server
		MIB_CE_MAC_BASE_DHCP_T entry;

		strcpy(macaddr, "localmac");
		//printf("entry.macAddr = %s\n", entry.macAddr);
		if (mib_get_s(MIB_ADSL_LAN_IP, (void *)value, sizeof(value)) != 0)
		{
			//strncpy(entry.ipAddr, inet_ntoa(*((struct in_addr *)value)), 16);
			//entry.ipAddr[15] = '\0';
			fprintf(fp2, "%s: %s\n", macaddr, inet_ntoa(*((struct in_addr *)value)));
		}

#ifdef CONFIG_SECONDARY_IP
		mib_get_s(MIB_ADSL_LAN_ENABLE_IP2, (void *)value, sizeof(value));
		if (value[0] == 1) {
			strcpy(macaddr, "localsecmac");
			//printf("entry.macAddr = %s\n", entry.macAddr);
			if (mib_get_s(MIB_ADSL_LAN_IP2, (void *)value, sizeof(value)) != 0)
			{
				//strncpy(entry.ipAddr, inet_ntoa(*((struct in_addr *)value)), 16);
				//entry.ipAddr[15] = '\0';
				fprintf(fp2, "%s: %s\n", macaddr, inet_ntoa(*((struct in_addr *)value)) );
			}

		}

#endif

//star: check mactbl
	struct in_addr poolstart,poolend,macip;
	unsigned long v1;

	inet_aton(ipstart, &poolstart);
	inet_aton(ipend, &poolend);
check_mactbl:
	entryNum = mib_chain_total(MIB_MAC_BASE_DHCP_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_MAC_BASE_DHCP_TBL, i, (void *)&Entry))
		{
  			printf("setupDhcpd:Get chain(MIB_MAC_BASE_DHCP_TBL) record error!\n");
		}
		//inet_aton(Entry.ipAddr, &macip);
		v1 = ntohl(*((uint32_t *)Entry.ipAddr_Dhcp));

		if( v1 < ntohl(poolstart.s_addr) || v1 > ntohl(poolend.s_addr) )
		//if(macip.s_addr<poolstart.s_addr||macip.s_addr>poolend.s_addr)
		{
			if(mib_chain_delete(MIB_MAC_BASE_DHCP_TBL, i)!=1)
			{
				fclose(fp2);
#ifdef RESTRICT_PROCESS_PERMISSION
				chmod("/var/dhcpdMacBase.txt",0666);
#endif
				printf("setupDhcpd:Delete chain(MIB_MAC_BASE_DHCP_TBL) record error!\n");
				return -1;
			}
			break;
		}
	}
	if((int)i<((int)entryNum-1))
		goto check_mactbl;

	entryNum = mib_chain_total(MIB_MAC_BASE_DHCP_TBL);
	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_MAC_BASE_DHCP_TBL, i, (void *)&Entry))
		{
  			printf("setupDhcpd:Get chain(MIB_MAC_BASE_DHCP_TBL) record error!\n");
		}

		if(Entry.Enabled==0)
			continue;

		snprintf(macaddr, 18, "%02x-%02x-%02x-%02x-%02x-%02x",
				Entry.macAddr_Dhcp[0], Entry.macAddr_Dhcp[1],
				Entry.macAddr_Dhcp[2], Entry.macAddr_Dhcp[3],
				Entry.macAddr_Dhcp[4], Entry.macAddr_Dhcp[5]);

		for (j=0; j<17; j++){
			if ( macaddr[j] != '-' ) {
				macaddr[j] = tolower(macaddr[j]);
			}
		}

		//fprintf(fp2, "%s: %s\n", Entry.macAddr, Entry.ipAddr);
		fprintf(fp2, "%s: %s\n", macaddr, inet_ntoa(*((struct in_addr *)Entry.ipAddr_Dhcp)) );
	}
	fclose(fp2);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod("/var/dhcpdMacBase.txt",0666);
#endif

	// Added by Mason Yu for MAC base assignment. End

dhcpConf:

#ifdef SUPPORT_DHCP_RESERVED_IPADDR
	setupDHCPReservedIPAddr();
#endif //SUPPORT_DHCP_RESERVED_IPADDR

#ifdef IMAGENIO_IPTV_SUPPORT
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
	if (mib_get_s(MIB_OPCH_ADDRESS, (void *)&value[0], sizeof(value[0])) != 0) {
		strncpy(opchaddr, inet_ntoa(*(struct in_addr *)value), 16);
		opchaddr[15] = '\0';
	} else
		return -1;

	if (mib_get_s(MIB_IMAGENIO_DNS1, (void *)&value[0], sizeof(value[0])) != 0) {
		strncpy(stbdns1, inet_ntoa(*(struct in_addr *)value), 16);
		stbdns1[15] = '\0';
	} else
		return -1;

	mib_get_s(MIB_IMAGENIO_DNS2, (void *)&myip, sizeof(myip));
	if ((myip.s_addr != 0xffffffff) && (myip.s_addr != 0)) { // not empty
		strncpy(stbdns2, inet_ntoa(myip), 16);
		stbdns2[15] = '\0';
	}

	if (!mib_get_s(MIB_OPCH_PORT, (void *)&opchport, sizeof(opchport)))
		return -1;
#endif
/*ping_zhang:20090930 END*/
#endif

	// IP max lease time
	if (mib_get_s(MIB_ADSL_LAN_DHCP_LEASE, (void *)&uLTime, sizeof(uLTime)) == 0)
		return -1;

	if (mib_get_s(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)domain, sizeof(domain)) == 0)
		return -1;
#if 0
	// get DNS mode
	if (mib_get_s(MIB_ADSL_WAN_DNS_MODE, (void *)value, sizeof(value)) != 0)
	{
		dnsMode = (DNS_TYPE_T)(*(unsigned char *)value);
	}
	else
		return -1;
#endif

#ifdef USE_DEONET /* sghong, 20230330 */
	if (access(DHCPD_CONF, F_OK) == 0)
		goto SKIP_DNSCONF;
#endif

	// Commented by Mason Yu for LAN_IP as DNS Server
	if ((fp = fopen(DHCPD_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", DHCPD_CONF);
		return -1;
	}

	if (mib_get_s(MIB_SPC_ENABLE, (void *)value, sizeof(value)) != 0)
	{
		if (value[0])
		{
			spc_enable = 1;
			mib_get_s(MIB_SPC_IPTYPE, (void *)value, sizeof(value));
			spc_ip = (unsigned int)(*(unsigned char *)value);
		}
		else
			spc_enable = 0;
	}

/*ping_zhang:20080919 START:add for new telefonica tr069 request: dhcp option*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	fprintf(fp,"poolname default\n");
#endif
/*ping_zhang:20080919 END*/
	//ql: check if pool is in first IP subnet or in secondary IP subnet.
	mib_get_s(MIB_DHCP_POOL_START, (void *)&inpoolstart, sizeof(inpoolstart));
	mib_get_s(MIB_ADSL_LAN_IP, (void *)&lanip1, sizeof(lanip1));
	mib_get_s(MIB_ADSL_LAN_SUBNET, (void *)&lanmask1, sizeof(lanmask1));
	if ((inpoolstart.s_addr & lanmask1.s_addr) == (lanip1.s_addr & lanmask1.s_addr))
		subnetIdx = 0;
	else {
#ifdef CONFIG_SECONDARY_IP
		mib_get_s(MIB_ADSL_LAN_IP2, (void *)&lanip2, sizeof(lanip2));
		mib_get_s(MIB_ADSL_LAN_SUBNET2, (void *)&lanmask2, sizeof(lanmask2));
		if ((inpoolstart.s_addr & lanmask2.s_addr) == (lanip2.s_addr & lanmask2.s_addr))
			subnetIdx = 1;
		else
#endif //CONFIG_SECONDARY_IP
			subnetIdx = 0;
	}
	if (0 == subnetIdx)
		snprintf(serverip, 16, "%s", inet_ntoa(lanip1));
#ifdef CONFIG_SECONDARY_IP
	else
		snprintf(serverip, 16, "%s", inet_ntoa(lanip2));
#endif //CONFIG_SECONDARY_IP

	fprintf(fp, "interface %s\n", LANIF);
	//ql add
	fprintf(fp, "server %s\n", serverip);
	// Mason Yu. For Test
#if 0
	fprintf(fp, "locallyserved no\n" );
	if (mib_get_s(MIB_ADSL_WAN_DHCPS, (void *)value, sizeof(value)) != 0)
	{
		if (((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(dhcps, inet_ntoa(*((struct in_addr *)value)), 16);
			dhcps[15] = '\0';
		}
		else
			dhcps[0] = '\0';
	}
	fprintf(fp, "dhcpserver %s\n", dhcps);
#endif

	if (spc_enable)
	{
		if (spc_ip == 0) { // single private ip
			fprintf(fp, "start %s\n", ipstart);
			fprintf(fp, "end %s\n", ipstart);
		}
		//else { // single shared ip
	}
	else
	{
		fprintf(fp, "start %s\n", ipstart);
		fprintf(fp, "end %s\n", ipend);
	}
#if 0
#ifdef IP_BASED_CLIENT_TYPE
	fprintf(fp, "pcstart %s\n", pcipstart);
	fprintf(fp, "pcend %s\n", pcipend);
	fprintf(fp, "cmrstart %s\n", cmripstart);
	fprintf(fp, "cmrend %s\n", cmripend);
	fprintf(fp, "stbstart %s\n", stbipstart);
	fprintf(fp, "stbend %s\n", stbipend);
	fprintf(fp, "phnstart %s\n", phnipstart);
	fprintf(fp, "phnend %s\n", phnipend);
	fprintf(fp, "hgwstart %s\n", hgwipstart);
	fprintf(fp, "hgwend %s\n", hgwipend);
	//ql 20090122 add
	fprintf(fp, "pcopt60 %s\n", pcopt60);
	fprintf(fp, "cmropt60 %s\n", cmropt60);
	fprintf(fp, "stbopt60 %s\n", stbopt60);
	fprintf(fp, "phnopt60 %s\n", phnopt60);
#endif
#endif

#if 0
/*ping_zhang:20090319 START:replace ip range with serving pool of tr069*/
#ifndef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
#ifdef IP_BASED_CLIENT_TYPE
	{
		MIB_CE_IP_RANGE_DHCP_T Entry;
		int i, entrynum=mib_chain_total(MIB_IP_RANGE_DHCP_TBL);

		for (i=0; i<entrynum; i++)
		{
			if (!mib_chain_get(MIB_IP_RANGE_DHCP_TBL, i, (void *)&Entry))
				continue;

			strncpy(devipstart, inet_ntoa(*((struct in_addr *)Entry.startIP)), 16);
			strncpy(devipend, inet_ntoa(*((struct in_addr *)Entry.endIP)), 16);

/*ping_zhang:20090317 START:change len of option60 to 100*/
			//fprintf(fp, "range start=%s:end=%s:option=%s:devicetype=%d\n", devipstart, devipend, Entry.option60,Entry.deviceType);
			//fprintf(fp, "range start=%s:end=%s:option=%s:devicetype=%d:optCode=%d:optStr=%s\n", devipstart, devipend, Entry.option60,Entry.deviceType,Entry.optionCode,Entry.optionStr);
			//fprintf(fp, "range s=%s:e=%s:o=%s:t=%d:oC=%d:oS=%s\n",	devipstart, devipend, Entry.option60,Entry.deviceType,Entry.optionCode,Entry.optionStr);
			fprintf(fp, "range i=%d:s=%s:e=%s:o=%s:t=%d\n",i,devipstart, devipend, Entry.option60,Entry.deviceType);
			fprintf(fp, "range_optRsv i=%d:oC=%d:oS=%s\n",i,Entry.optionCode,Entry.optionStr);
/*ping_zhang:20090317 END*/
		}
	}
#endif
#endif
#endif //#if 0

/*ping_zhang:20090319 END*/
#ifdef IMAGENIO_IPTV_SUPPORT
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
	fprintf(fp, "opchaddr %s\n", opchaddr);
	fprintf(fp, "opchport %d\n", opchport);
	fprintf(fp, "stbdns1 %s\n", stbdns1);
	if (stbdns2[0])
		fprintf(fp, "stbdns2 %s\n", stbdns2);
#endif
/*ping_zhang:20090930 END*/
#endif

#ifdef IP_PASSTHROUGH
	ippt = 0;
	if (mib_get_s(MIB_IPPT_ITF, (void *)&ippt_itf, sizeof(ippt_itf)) != 0)
		if (ippt_itf != DUMMY_IFINDEX) // IP passthrough
			ippt = 1;

	if (ippt)
	{
		fprintf(fp, "ippt yes\n");
		mib_get_s(MIB_IPPT_LEASE, (void *)&uInt, sizeof(uInt));
		fprintf(fp, "ipptlt %d\n", uInt);
	}
#endif

	fprintf(fp, "opt subnet %s\n", dhcpsubnet);

	// Added by Mason Yu
	if (mib_get_s(MIB_ADSL_LAN_DHCP_GATEWAY, (void *)value, sizeof(value)) != 0)
	{
		strncpy(ipaddr2, inet_ntoa(*((struct in_addr *)value)), 16);
		ipaddr2[15] = '\0';
	}
	else{
		fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
		chmod(DHCPD_CONF,0666);
#endif
		return -1;
	}
	fprintf(fp, "opt router %s\n", ipaddr2);

	// Modified by Mason Yu for LAN_IP as DNS Server
#if 0
	if (dnsMode == DNS_AUTO)
	{
		fprintf(fp, "opt dns %s\n", ipaddr);
	}
	else	// DNS_MANUAL
	{
		if (dns1[0])
			fprintf(fp, "opt dns %s\n", dns1);
		if (dns2[0])
			fprintf(fp, "opt dns %s\n", dns2);
		if (dns3[0])
			fprintf(fp, "opt dns %s\n", dns3);
	}
#endif

#ifdef DHCPS_DNS_OPTIONS
	deo_dns_fixed_value_get(&fixed_dns, tmpdns1, tmpdns2);

	mib_get_s(MIB_DHCP_DNS_OPTION, (void *)&dnsMode, sizeof(dnsMode));
	if(fixed_dns == 0)
	{
#ifdef USE_DEONET /* sghong, 20230331,  SKB default dns value "210.220.163.82", "219.250.36.130" */
		if ((tmpdns1[0] == 0) && (tmpdns1[1] == 0) && (tmpdns1[2] == 0) && (tmpdns1[3] == 0))
		{
			fprintf(fp, "opt dns %s\n", "219.250.36.130");
			fprintf(fp, "opt dns %s\n", "210.220.163.82");
		}
		else
		{
			/* dns primary value "210.220.163.82" */
			fprintf(fp, "opt dns %d.%d.%d.%d\n", tmpdns1[0], tmpdns1[1], tmpdns1[2], tmpdns1[3]);
			fprintf(fp, "opt dns %d.%d.%d.%d\n", tmpdns2[0], tmpdns2[1], tmpdns2[2], tmpdns2[3]);
		}
#else
		fprintf(fp, "opt dns %s\n", ipaddr2);
#endif
	}
	else if(fixed_dns == 1) {
		memset(dns1, 0, sizeof(dns1));
		memset(dns2, 0, sizeof(dns2));
		memset(dns3, 0, sizeof(dns3));
		if (mib_get_s(MIB_DHCPS_DNS1, (void *)&myip, sizeof(myip)) &&
			myip.s_addr != INADDR_NONE)
		{
			strncpy(dns1, inet_ntoa(myip), sizeof(dns1));
			fprintf(fp, "opt dns %s\n", dns1);
		}

		if (mib_get_s(MIB_DHCPS_DNS2, (void *)&myip, sizeof(myip)) &&
			myip.s_addr != INADDR_NONE)
		{
			strncpy(dns2, inet_ntoa(myip), sizeof(dns2));
			fprintf(fp, "opt dns %s\n", dns2);
		}

		if (mib_get_s(MIB_DHCPS_DNS3, (void *)&myip, sizeof(myip)) &&
			myip.s_addr != INADDR_NONE)
		{
			strncpy(dns3, inet_ntoa(myip), sizeof(dns3));
			// fprintf(fp, "opt dns %s\n", dns3);
		}
	}
#if 0
	else if(fixed_dns == 2) {
		dns1[0] = dns2[0] = dns3[0] = 0;
		if(get_dns_from_wan(dns1, sizeof(dns1), dns2, sizeof(dns2), dns3, sizeof(dns3)) == 0) {
			short_lease_flag = 1;
			fprintf(fp, "opt dns %s\n", ipaddr2);
		}
		else {
			if(dns1[0]) fprintf(fp, "opt dns %s\n", dns1);
			if(dns2[0]) fprintf(fp, "opt dns %s\n", dns2);
			if(dns3[0]) fprintf(fp, "opt dns %s\n", dns3);
		}
	}
#endif
#endif

	if(short_lease_flag)
		fprintf(fp, "opt lease %u\n", DNS_RELAY_SHORT_LEASETIME);
	else
		fprintf(fp, "opt lease %u\n", uLTime);

#ifndef CONFIG_TRUE
	if (domain[0])
		fprintf(fp, "opt domain %s\n", domain);
	else
		// per TR-068, I-188
		fprintf(fp, "opt domain domain_not_set.invalid\n");
#endif
#ifdef _CWMP_MIB_ /*jiunming, for cwmp-tr069*/
{
	//format: opt venspec [enterprise-number] [sub-option code] [sub-option data] ...
	//opt vendspec: dhcp option 125 Vendor-Identifying Vendor-Specific
	//enterprise-number: 3561(ADSL Forum)
	//sub-option code: 4(GatewayManufacturerOUI)
	//sub-option code: 5(GatewaySerialNumber)
	//sub-option code: 6(GatewayProductClass)
	char opt125_sn[65];
	mib_get_s(MIB_HW_SERIAL_NUMBER, (void *)opt125_sn, sizeof(opt125_sn));
	fprintf( fp, "opt venspec 3561 4 %s 5 %s 6 %s\n", MANUFACTURER_OUI, opt125_sn, PRODUCT_CLASS );
}
#endif

#if !defined(CONFIG_00R0)
#ifndef CONFIG_TELMEX_DEV
	//Mark because project 00R0 will use ntp.local as default NTP Server and it will
	//Slow the udhcpd init time.
	/* DHCP option 42 */
	mib_get_s( MIB_NTP_SERVER_ID, (void *)&vChar, sizeof(vChar));
	if (vChar == 0) {
		mib_get_s(MIB_NTP_SERVER_HOST1, value, sizeof(value));
		fprintf(fp, "opt ntpsrv %s\n", value);
	}
	else
	{
		mib_get_s(MIB_NTP_SERVER_HOST5, (void *)value5, sizeof(value5));
		mib_get_s(MIB_NTP_SERVER_HOST4, (void *)value4, sizeof(value4));
		mib_get_s(MIB_NTP_SERVER_HOST3, (void *)value3, sizeof(value3));
		mib_get_s(MIB_NTP_SERVER_HOST2, (void *)value, sizeof(value));
		mib_get_s(MIB_NTP_SERVER_HOST1, (void *)value1, sizeof(value1));
		fprintf(fp, "opt ntpsrv %s,%s,%s,%s,%s\n", value,value1,value3,value4,value5);
	}
#else
	unsigned char serverAddr[150];
	int serverIndex, length=0;

	mib_get_s( MIB_NTP_SERVER_ID, (void *)&vChar, sizeof(vChar));
	memset(serverAddr, 0, sizeof(serverAddr));

	if (vChar & 0x01)
	{
		mib_get(MIB_NTP_SERVER_HOST1, value);
		length += sprintf(serverAddr + length, "%s", value);
	}
	if (vChar & 0x02)
	{
		mib_get(MIB_NTP_SERVER_HOST2, value);
		if (serverAddr[0])
			length += sprintf(serverAddr + length, ",%s", value);
		else
			length += sprintf(serverAddr + length, "%s", value);
	}
	if (vChar & 0x04)
	{
		mib_get(MIB_NTP_SERVER_HOST3, value);
		if (serverAddr[0])
			length += sprintf(serverAddr + length, ",%s", value);
		else
			length += sprintf(serverAddr + length, "%s", value);
	}
	if (vChar & 0x08)
	{
		mib_get(MIB_NTP_SERVER_HOST4, value);
		if (serverAddr[0])
			length += sprintf(serverAddr + length, ",%s", value);
		else
			length += sprintf(serverAddr + length, "%s", value);
	}
	if (vChar & 0x10)
	{
		mib_get(MIB_NTP_SERVER_HOST5, value);
		if (serverAddr[0])
			length += sprintf(serverAddr + length, ",%s", value);
		else
			length += sprintf(serverAddr + length, "%s", value);
	}

	fprintf(fp, "opt ntpsrv %s\n", serverAddr);
#endif
#endif

	/* DHCP option 66 */
	if (mib_get_s(MIB_TFTP_SERVER_ADDR, (void *)&optionStr, sizeof(optionStr)) != 0) { //Length max 254
		fprintf(fp, "opt tftp %s\n", optionStr);
	}

#if !defined(CONFIG_00R0)
	//Mark because project 00R0 will use ntp.local as default NTP Server and it will
	//Slow the udhcpd init time.
	/* DHCP option 100 */
	if (mib_get_s(MIB_POSIX_TZ_STRING, (void *)&optionStr, sizeof(optionStr)) != 0) { //Length max 254
		fprintf(fp, "opt tzstring %s\n", optionStr);
	}
#endif
#ifdef CONFIG_USER_RTK_VOIP //single SIP server right now /* DHCP option 120 */

#include <errno.h>
#include <fcntl.h>
#include "voip_manager.h"
#define SIP_PROXY_NUM 2

	voipCfgParam_t *voip_pVoIPCfg = NULL;
	voipCfgPortParam_t *pCfg;
	int count = 0;
	int ret;

	int enc[SIP_PROXY_NUM];
	long ipaddrs=0;

	if(voip_pVoIPCfg==NULL){
		if (voip_flash_get(&voip_pVoIPCfg) != 0){
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
			fprintf(fp, "poolend end\n");
#endif
			fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
		chmod(DHCPD_CONF,0666);
#endif
			return -1;
		}
	}

	pCfg = &voip_pVoIPCfg->ports[0];
	for( i=0 ; i<MAX_PROXY ; i++ ){
		if(*(char *)pCfg->proxies[i].addr){
			ipaddrs = inet_network(pCfg->proxies[i].addr);
			if (ipaddrs >= 0){
				enc[i] = 1;
				fprintf(fp, "opt sipsrv %d %s\n", enc[i], pCfg->proxies[i].addr);
			}
			else {
				if(strlen(pCfg->proxies[0].addr) > 5) { //Avoid to the NULL string, Iulian Wu
					enc[i] = 0;
					fprintf(fp, "opt sipsrv %d %s\n", enc[i], pCfg->proxies[i].addr);
				}
			}
			break;
		}
	}

#endif

/*write dhcp serving pool config*/
/*ping_zhang:20080919 START:add for new telefonica tr069 request: dhcp option*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	fprintf(fp, "poolend end\n");
	memset(&dhcppoolentry,0,sizeof(DHCPS_SERVING_POOL_T));
/*test code*
	dhcppoolentry.enable=1;
	dhcppoolentry.poolorder=1;
	strcpy(dhcppoolentry.poolname,"poolone");
	strcpy(dhcppoolentry.vendorclass,"MSFT 5.0");
	strcpy(dhcppoolentry.clientid,"");
	strcpy(dhcppoolentry.userclass,"");

	inet_aton("192.168.1.40",((struct in_addr *)(dhcppoolentry.startaddr)));
	inet_aton("192.168.1.50",((struct in_addr *)(dhcppoolentry.endaddr)));
	inet_aton("255.255.255.0",((struct in_addr *)(dhcppoolentry.subnetmask)));
	inet_aton("192.168.1.1",((struct in_addr *)(dhcppoolentry.iprouter)));
	inet_aton("172.29.17.10",((struct in_addr *)(dhcppoolentry.dnsserver1)));
	strcpy(dhcppoolentry.domainname,"poolone.com");
	dhcppoolentry.leasetime=86400;
	dhcppoolentry.dnsservermode=1;
	dhcppoolentry.InstanceNum=1;
	entryNum = mib_chain_total(MIB_DHCPS_SERVING_POOL_TBL);
printf("\nentryNum=%d\n",entryNum);
	if(entryNum==0)
		mib_chain_add(MIB_DHCPS_SERVING_POOL_TBL,(void*)&dhcppoolentry);
****/

	entryNum = mib_chain_total(MIB_DHCPS_SERVING_POOL_TBL);

	for(i=0;i<entryNum;i++){
		memset(&dhcppoolentry,0,sizeof(DHCPS_SERVING_POOL_T));
		if(!mib_chain_get(MIB_DHCPS_SERVING_POOL_TBL,i,(void*)&dhcppoolentry))
			continue;
		if(dhcppoolentry.enable==1){
			strncpy(ipstart, inet_ntoa(*((struct in_addr *)(dhcppoolentry.startaddr))), 16);
			strncpy(ipend, inet_ntoa(*((struct in_addr *)(dhcppoolentry.endaddr))), 16);
			strncpy(dhcpsubnet, inet_ntoa(*((struct in_addr *)(dhcppoolentry.subnetmask))), 16);
			strncpy(ipaddr2, inet_ntoa(*((struct in_addr *)(dhcppoolentry.iprouter))), 16);

			if( dhcppoolentry.localserved ) //check only for locallyserved==true
			{
				if(!strcmp(ipstart,ipempty) ||
					!strcmp(ipend,ipempty) ||
					!strcmp(dhcpsubnet,ipempty))
					continue;
			}


			fprintf(fp, "poolname %s\n",dhcppoolentry.poolname);
			fprintf(fp, "cwmpinstnum %d\n",dhcppoolentry.InstanceNum);
			fprintf(fp, "poolorder %u\n",dhcppoolentry.poolorder);
			fprintf(fp, "interface %s\n", LANIF);
			//ql add
			fprintf(fp, "server %s\n", serverip);
			fprintf(fp, "start %s\n", ipstart);
			fprintf(fp, "end %s\n", ipend);

			fprintf(fp, "sourceinterface %u\n",dhcppoolentry.sourceinterface);
			if(dhcppoolentry.vendorclass[0]!=0){
				fprintf(fp, "vendorclass %s\n",dhcppoolentry.vendorclass);
				fprintf(fp, "vendorclassflag %u\n",dhcppoolentry.vendorclassflag);
				if(dhcppoolentry.vendorclassmode[0]!=0)
					fprintf(fp, "vendorclassmode %s\n",dhcppoolentry.vendorclassmode);
			}
			if(dhcppoolentry.clientid[0]!=0){
				fprintf(fp, "clientid %s\n",dhcppoolentry.clientid);
				fprintf(fp, "clientidflag %u\n",dhcppoolentry.clientidflag);
			}
			if(dhcppoolentry.userclass[0]!=0){
				fprintf(fp, "userclass %s\n",dhcppoolentry.userclass);
				fprintf(fp, "userclassflag %u\n",dhcppoolentry.userclassflag);
			}
			if(memcmp(dhcppoolentry.chaddr,macempty,6)){
				fprintf(fp, "chaddr %02x%02x%02x%02x%02x%02x\n",dhcppoolentry.chaddr[0],dhcppoolentry.chaddr[1],dhcppoolentry.chaddr[2],
					dhcppoolentry.chaddr[3],dhcppoolentry.chaddr[4],dhcppoolentry.chaddr[5]);
				if(memcmp(dhcppoolentry.chaddrmask,macempty,6))
					fprintf(fp, "chaddrmask %02x%02x%02x%02x%02x%02x\n",dhcppoolentry.chaddrmask[0],dhcppoolentry.chaddrmask[1],dhcppoolentry.chaddrmask[2],
						dhcppoolentry.chaddrmask[3],dhcppoolentry.chaddrmask[4],dhcppoolentry.chaddrmask[5]);
				fprintf(fp, "chaddrflag %u\n",dhcppoolentry.chaddrflag);
			}

#ifdef IP_PASSTHROUGH
			ippt = 0;
			if (mib_get_s(MIB_IPPT_ITF, (void *)&ippt_itf, sizeof(ippt_itf)) != 0)
				if (ippt_itf != DUMMY_IFINDEX) // IP passthrough
					ippt = 1;

			if (ippt)
			{
				fprintf(fp, "ippt yes\n");
				mib_get_s(MIB_IPPT_LEASE, (void *)&uInt, sizeof(uInt));
				fprintf(fp, "ipptlt %d\n", uInt);
			}
#endif
#ifdef IMAGENIO_IPTV_SUPPORT
			//fprintf(fp, "pcopt60 %s\n", pcopt60);
			//fprintf(fp, "cmropt60 %s\n", cmropt60);
			//fprintf(fp, "stbopt60 %s\n", stbopt60);
			//fprintf(fp, "phnopt60 %s\n", phnopt60);
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
			fprintf(fp, "opchaddr %s\n", opchaddr);
			fprintf(fp, "opchport %d\n", opchport);
			fprintf(fp, "stbdns1 %s\n", stbdns1);
			if (stbdns2[0])
				fprintf(fp, "stbdns2 %s\n", stbdns2);
#endif
/*ping_zhang:20090930 END*/
#endif

			if(strcmp(dhcpsubnet,ipempty))
				fprintf(fp, "opt subnet %s\n", dhcpsubnet);
			if(strcmp(ipaddr2,ipempty))
				fprintf(fp, "opt router %s\n", ipaddr2);

#if 1 //def DHCPS_DNS_OPTIONS
			if (dhcppoolentry.dnsservermode == 0)
				fprintf(fp, "opt dns %s\n", ipaddr2);
			else if(dhcppoolentry.dnsservermode == 1) {
				myip = *((struct in_addr *)(dhcppoolentry.dnsserver1));
				if (myip.s_addr != INADDR_NONE) {
					strncpy(ipaddr, inet_ntoa(myip), sizeof(ipaddr));
					ipaddr[sizeof(ipaddr)-1] = '\0';
					fprintf(fp, "opt dns %s\n", dns1);
				}

				myip = *((struct in_addr *)(dhcppoolentry.dnsserver2));
				if (myip.s_addr != INADDR_NONE) {
					strncpy(ipaddr, inet_ntoa(myip), sizeof(ipaddr));
					ipaddr[sizeof(ipaddr)-1] = '\0';
					fprintf(fp, "opt dns %s\n", dns1);
				}

				myip = *((struct in_addr *)(dhcppoolentry.dnsserver3));
				if (myip.s_addr != INADDR_NONE) {
					strncpy(ipaddr, inet_ntoa(myip), sizeof(ipaddr));
					ipaddr[sizeof(ipaddr)-1] = '\0';
					fprintf(fp, "opt dns %s\n", dns1);
				}
			}
			else if(dhcppoolentry.dnsservermode == 2) {
				dns1[0] = dns2[0] = dns3[0] = 0;
				if(get_dns_from_wan(dns1, sizeof(dns1), dns2, sizeof(dns2), dns3, sizeof(dns3)) == 0) {
					short_lease_flag = 1;
					fprintf(fp, "opt dns %s\n", ipaddr2);
				}
				else {
					if(dns1[0]) fprintf(fp, "opt dns %s\n", dns1);
					if(dns2[0]) fprintf(fp, "opt dns %s\n", dns2);
					if(dns3[0]) fprintf(fp, "opt dns %s\n", dns3);
				}
			}
#endif

		if(short_lease_flag)
			fprintf(fp, "opt lease %u\n", DNS_RELAY_SHORT_LEASETIME);
		else
			fprintf(fp, "opt lease %u\n", (unsigned int)(dhcppoolentry.leasetime));

#ifndef CONFIG_TRUE
			if (dhcppoolentry.domainname[0])
				fprintf(fp, "opt domain %s\n", dhcppoolentry.domainname);
			else
				// per TR-068, I-188
				fprintf(fp, "opt domain domain_not_set.invalid\n");
#endif
#ifdef _CWMP_MIB_ /*jiunming, for cwmp-tr069*/
		{
			//format: opt venspec [enterprise-number] [sub-option code] [sub-option data] ...
			//opt vendspec: dhcp option 125 Vendor-Identifying Vendor-Specific
			//enterprise-number: 3561(ADSL Forum)
			//sub-option code: 4(GatewayManufacturerOUI)
			//sub-option code: 5(GatewaySerialNumber)
			//sub-option code: 6(GatewayProductClass)
			char opt125_sn[65];
			mib_get_s(MIB_HW_SERIAL_NUMBER, (void *)opt125_sn, sizeof(opt125_sn));
			fprintf( fp, "opt venspec 3561 4 %s 5 %s 6 %s\n", MANUFACTURER_OUI, opt125_sn, PRODUCT_CLASS );
		}
#endif
			//relayinfo
			if( dhcppoolentry.localserved==0 )
			{
				char dhcpserveripaddr[16];
				fprintf(fp, "locallyserved no\n" );
				strncpy(dhcpserveripaddr, inet_ntoa(*((struct in_addr *)(dhcppoolentry.dhcprelayip))), 16);
				dhcpserveripaddr[15]='\0';
				fprintf(fp, "dhcpserver %s\n", dhcpserveripaddr );
			}

			fprintf(fp, "poolend end\n");

		}
	}
#endif
/*ping_zhang:20080919 END*/

	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
		chmod(DHCPD_CONF,0666);
#endif

SKIP_DNSCONF:
//star: retain the lease file between during dhcdp restart
	if((fp = fopen(DHCPD_LEASE, "r")) == NULL)
	{
		if ((fp = fopen(DHCPD_LEASE, "w")) == NULL)
		{
			printf("Open file %s failed !\n", DHCPD_LEASE);
			return -1;
		}
		fprintf(fp, "\n");
		fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
		chmod(DHCPD_LEASE,0666);
#endif
	}
	else
		fclose(fp);

	return 1;
}

// Added by Mason Yu for Dhcp Relay
// return value:
// 0  : not active
// 1  : active
// -1 : setup failed
int startDhcpRelay(void)
{
	unsigned char value[32];
	unsigned int dhcpmode=2;
	char dhcps[16];
	int status=0;
#ifdef CONFIG_RTK_DEV_AP
	int pid;
	pid = read_pid(DHCPSERVERPID);
	if (pid > 0)
		return 0;
#endif
	if (mib_get_s(MIB_DHCP_MODE, (void *)value, sizeof(value)) != 0)
	{
		dhcpmode = (unsigned int)(*(unsigned char *)value);
		if (dhcpmode != 1 )
			return 0;	// dhcp Relay not on
	}

	// DHCP Relay is on
	if (dhcpmode == DHCP_LAN_RELAY) {

		//printf("DHCP Relay is on\n");

		if (mib_get_s(MIB_ADSL_WAN_DHCPS, (void *)value, sizeof(value)) != 0)
		{
			if (((struct in_addr *)value)->s_addr != 0)
			{
				strncpy(dhcps, inet_ntoa(*((struct in_addr *)value)), 16);
				dhcps[15] = '\0';
			}
			else
				dhcps[0] = '\0';
		}
		else
			return -1;

		#ifdef REMOTE_ACCESS_CTL
		// For DHCP Relay. Open DHCP Relay Port for Incoming Packets.
		// iptables -D inacc -i ! br+ -p udp --sport 67 --dport 67 -j ACCEPT
		va_cmd_no_error(IPTABLES, 13, 1, (char *)FW_DEL, (char *)FW_INACC, "!", (char *)ARG_I, LANIF_ALL, "-p", "udp", (char *)FW_SPORT, (char *)PORT_DHCP, (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_ACCEPT);
		// iptables -A inacc -i ! br+ -p udp --sport 67 --dport 67 -j ACCEPT
		va_cmd(IPTABLES, 13, 1, (char *)FW_ADD, (char *)FW_INACC, "!", (char *)ARG_I, LANIF_ALL, "-p", "udp", (char *)FW_SPORT, (char *)PORT_DHCP, (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_ACCEPT);
		#endif

		//printf("dhcps = %s\n", dhcps);
#ifndef COMBINE_DHCPD_DHCRELAY
		status=va_niced_cmd("/bin/dhcrelay", 1, 0, dhcps);
#else
		status=va_niced_cmd(DHCPD, 2, 0, "-R", dhcps);
#ifdef CONFIG_RTK_DEV_AP
		start_process_check_pidfile((const char*)DHCPSERVERPID);
#endif
#endif
		status=(status==-1)?-1:1;

		return status;

	}

	return status;

}
#endif // of CONFIG_DHCP_SERVER

#if defined(CONFIG_ANDLINK_SUPPORT) && defined(CONFIG_USER_DHCP_SERVER)
int setupDhcpd_Andlink(char *ifname, char *ip, char *ip_start, char *ip_end)
{
	unsigned char value[64], value1[64], value3[64], value4[64], value5[64];
	unsigned int uInt, uLTime;
	DNS_TYPE_T dnsMode;
	FILE *fp=NULL, *fp2=NULL;
	char ipstart[16], ipend[16], vChar, optionStr[254];
#ifdef IMAGENIO_IPTV_SUPPORT
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
	char opchaddr[16]={0}, stbdns1[16]={0}, stbdns2[16]={0};
	unsigned short opchport;
#endif
/*ping_zhang:20090930 END*/
#endif
	char subnet[16], ipaddr[16], ipaddr2[16];
	char dhcpsubnet[16];
	char dns1[16], dns2[16], dns3[16], dhcps[16];
	char domain[MAX_NAME_LEN];
	unsigned int entryNum, i, j;
	MIB_CE_MAC_BASE_DHCP_T Entry;
#ifdef IP_PASSTHROUGH
	int ippt;
	unsigned int ippt_itf;
#endif
	int spc_enable=0, spc_ip=0;
	struct in_addr myip;
	//ql: check if pool is in first IP subnet or in secondary IP subnet.
	int subnetIdx=0;
	struct in_addr lanip1, lanmask1, lanip2, lanmask2, inpoolstart;
	char serverip[16];
#ifdef CONFIG_CT_AWIFI_JITUAN_FEATURE
	char serverip_awifi[16];
#endif


/*ping_zhang:20080919 START:add for new telefonica tr069 request: dhcp option*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	DHCPS_SERVING_POOL_T dhcppoolentry;
	int dhcppoolnum;
	unsigned char macempty[6]={0,0,0,0,0,0};
	char *ipempty="0.0.0.0";
#endif
	char macaddr[20];

/*ping_zhang:20080919 END*/
	// check if dhcp server on ?
	// Modified by Mason Yu for dhcpmode
	//if (mib_get(MIB_ADSL_LAN_DHCP, (void *)value) != 0)

#if 0 // bcz dhcp mode is client now and do not need change(tesia)
	if (mib_get(MIB_DHCP_MODE, (void *)value) != 0)
	{
		uInt = (unsigned int)(*(unsigned char *)value);
		if (uInt != 2 )
		{
			return 0;	// dhcp Server not on
		}
	}
#endif

#ifdef CONFIG_SECONDARY_IP
	mib_get(MIB_ADSL_LAN_ENABLE_IP2, (void *)value);
	if (value[0])
		mib_get(MIB_ADSL_LAN_DHCP_POOLUSE, (void *)value);
#else
	value[0] = 0;
#endif

#ifdef DHCPS_POOL_COMPLETE_IP
	mib_get(MIB_DHCP_SUBNET_MASK, (void *)value);
	strncpy(dhcpsubnet, inet_ntoa(*((struct in_addr *)value)), 16);
	dhcpsubnet[15] = '\0';
	// IP Pool start
	mib_get(MIB_DHCP_POOL_START, (void *)value);
	strncpy(ipstart, inet_ntoa(*((struct in_addr *)value)), 16);
	// IP Pool end
	mib_get(MIB_DHCP_POOL_END, (void *)value);
	strncpy(ipend, inet_ntoa(*((struct in_addr *)value)), 16);
#else
	if (value[0] == 0) { // primary LAN
		if (mib_get(MIB_ADSL_LAN_SUBNET, (void *)value) != 0)
		{
			strncpy(dhcpsubnet, inet_ntoa(*((struct in_addr *)value)), 16);
			dhcpsubnet[15] = '\0';
		}
		else
			return -1;

		mib_get(MIB_ADSL_LAN_IP, (void *)value);
	}
	else { // secondary LAN
		if (mib_get(MIB_ADSL_LAN_SUBNET2, (void *)value) != 0)
		{
			strncpy(dhcpsubnet, inet_ntoa(*((struct in_addr *)value)), 16);
			dhcpsubnet[15] = '\0';
		}
		else
			return -1;

		mib_get(MIB_ADSL_LAN_IP2, (void *)value);
	}

	// IP pool start address
	mib_get(MIB_ADSL_LAN_CLIENT_START, (void *)&value[3]);
		strncpy(ipstart, inet_ntoa(*((struct in_addr *)value)), 16);
		ipstart[15] = '\0';
	// IP pool end address
	mib_get(MIB_ADSL_LAN_CLIENT_END, (void *)&value[3]);
		strncpy(ipend, inet_ntoa(*((struct in_addr *)value)), 16);
		ipend[15] = '\0';
#endif

	// Added by Mason Yu for MAC base assignment. Start
	if ((fp2 = fopen("/var/dhcpdMacBase.txt", "w")) == NULL)
	{
		printf("***** Open file /var/dhcpdMacBase.txt failed !\n");
		goto dhcpConf;
	}

//star: reserve local ip for dhcp server
		MIB_CE_MAC_BASE_DHCP_T entry;

		strcpy(macaddr, "localmac");
		//printf("entry.macAddr = %s\n", entry.macAddr);
		if (mib_get(MIB_ADSL_LAN_IP, (void *)value) != 0)
		{
			//strncpy(entry.ipAddr, inet_ntoa(*((struct in_addr *)value)), 16);
			//entry.ipAddr[15] = '\0';
			fprintf(fp2, "%s: %s\n", macaddr, inet_ntoa(*((struct in_addr *)value)));
		}

#ifdef CONFIG_SECONDARY_IP
		mib_get(MIB_ADSL_LAN_ENABLE_IP2, (void *)value);
		if (value[0] == 1) {
			strcpy(macaddr, "localsecmac");
			//printf("entry.macAddr = %s\n", entry.macAddr);
			if (mib_get(MIB_ADSL_LAN_IP2, (void *)value) != 0)
			{
				//strncpy(entry.ipAddr, inet_ntoa(*((struct in_addr *)value)), 16);
				//entry.ipAddr[15] = '\0';
				fprintf(fp2, "%s: %s\n", macaddr, inet_ntoa(*((struct in_addr *)value)) );
			}

		}

#endif

//star: check mactbl
	struct in_addr poolstart,poolend,macip;
	unsigned long v1;

	inet_aton(ipstart, &poolstart);
	inet_aton(ipend, &poolend);
check_mactbl:
	entryNum = mib_chain_total(MIB_MAC_BASE_DHCP_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_MAC_BASE_DHCP_TBL, i, (void *)&Entry))
		{
			printf("setupDhcpd:Get chain(MIB_MAC_BASE_DHCP_TBL) record error!\n");
		}
		//inet_aton(Entry.ipAddr, &macip);
		v1 = *((unsigned long *)Entry.ipAddr_Dhcp);
		if( v1 < poolstart.s_addr || v1 > poolend.s_addr )
		//if(macip.s_addr<poolstart.s_addr||macip.s_addr>poolend.s_addr)
		{
			if(mib_chain_delete(MIB_MAC_BASE_DHCP_TBL, i)!=1)
			{
				fclose(fp2);
#ifdef RESTRICT_PROCESS_PERMISSION
				chmod("/var/dhcpdMacBase.txt",0666);
#endif

				printf("setupDhcpd:Delete chain(MIB_MAC_BASE_DHCP_TBL) record error!\n");
				return -1;
			}
			break;
		}
	}
	if((int)i<((int)entryNum-1))
		goto check_mactbl;

	entryNum = mib_chain_total(MIB_MAC_BASE_DHCP_TBL);
	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_MAC_BASE_DHCP_TBL, i, (void *)&Entry))
		{
			printf("setupDhcpd:Get chain(MIB_MAC_BASE_DHCP_TBL) record error!\n");
		}

		if(Entry.Enabled==0)
			continue;

		snprintf(macaddr, 18, "%02x-%02x-%02x-%02x-%02x-%02x",
				Entry.macAddr_Dhcp[0], Entry.macAddr_Dhcp[1],
				Entry.macAddr_Dhcp[2], Entry.macAddr_Dhcp[3],
				Entry.macAddr_Dhcp[4], Entry.macAddr_Dhcp[5]);

		for (j=0; j<17; j++){
			if ( macaddr[j] != '-' ) {
				macaddr[j] = tolower(macaddr[j]);
			}
		}

		//fprintf(fp2, "%s: %s\n", Entry.macAddr, Entry.ipAddr);
		fprintf(fp2, "%s: %s\n", macaddr, inet_ntoa(*((struct in_addr *)Entry.ipAddr_Dhcp)) );
	}
	fclose(fp2);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod("/var/dhcpdMacBase.txt",0666);
#endif

	// Added by Mason Yu for MAC base assignment. End

dhcpConf:

#ifdef SUPPORT_DHCP_RESERVED_IPADDR
	setupDHCPReservedIPAddr();
#endif //SUPPORT_DHCP_RESERVED_IPADDR

#ifdef IMAGENIO_IPTV_SUPPORT
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
	if (mib_get(MIB_OPCH_ADDRESS, (void *)&value[0]) != 0) {
		strncpy(opchaddr, inet_ntoa(*(struct in_addr *)value), 16);
		opchaddr[15] = '\0';
	} else
		return -1;

	if (mib_get(MIB_IMAGENIO_DNS1, (void *)&value[0]) != 0) {
		strncpy(stbdns1, inet_ntoa(*(struct in_addr *)value), 16);
		stbdns1[15] = '\0';
	} else
		return -1;

	mib_get(MIB_IMAGENIO_DNS2, (void *)&myip);
	if ((myip.s_addr != 0xffffffff) && (myip.s_addr != 0)) { // not empty
		strncpy(stbdns2, inet_ntoa(myip), 16);
		stbdns2[15] = '\0';
	}

	if (!mib_get(MIB_OPCH_PORT, (void *)&opchport))
		return -1;
#endif
/*ping_zhang:20090930 END*/
#endif

	// IP max lease time
	if (mib_get(MIB_ADSL_LAN_DHCP_LEASE, (void *)&uLTime) == 0)
		return -1;

	if (mib_get(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)domain) == 0)
		return -1;

	// get DNS mode
	if (mib_get(MIB_ADSL_WAN_DNS_MODE, (void *)value) != 0)
	{
		dnsMode = (DNS_TYPE_T)(*(unsigned char *)value);
	}
	else
		return -1;

	// Commented by Mason Yu for LAN_IP as DNS Server
	if ((fp = fopen(DHCPD_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", DHCPD_CONF);
		return -1;
	}

	if (mib_get(MIB_SPC_ENABLE, (void *)value) != 0)
	{
		if (value[0])
		{
			spc_enable = 1;
			mib_get(MIB_SPC_IPTYPE, (void *)value);
			spc_ip = (unsigned int)(*(unsigned char *)value);
		}
		else
			spc_enable = 0;
	}

/*ping_zhang:20080919 START:add for new telefonica tr069 request: dhcp option*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	fprintf(fp,"poolname default\n");
#endif
/*ping_zhang:20080919 END*/
	//ql: check if pool is in first IP subnet or in secondary IP subnet.
	mib_get(MIB_DHCP_POOL_START, (void *)&inpoolstart);
	mib_get(MIB_ADSL_LAN_IP, (void *)&lanip1);
	mib_get(MIB_ADSL_LAN_SUBNET, (void *)&lanmask1);
	if ((inpoolstart.s_addr & lanmask1.s_addr) == (lanip1.s_addr & lanmask1.s_addr))
		subnetIdx = 0;
	else {
#ifdef CONFIG_SECONDARY_IP
		mib_get(MIB_ADSL_LAN_IP2, (void *)&lanip2);
		mib_get(MIB_ADSL_LAN_SUBNET2, (void *)&lanmask2);
		if ((inpoolstart.s_addr & lanmask2.s_addr) == (lanip2.s_addr & lanmask2.s_addr))
			subnetIdx = 1;
		else
#endif //CONFIG_SECONDARY_IP
			subnetIdx = 0;
	}
#ifdef CONFIG_SECONDARY_IP
	if (1 == subnetIdx)
		snprintf(serverip, 16, "%s", inet_ntoa(lanip2));
	else
#endif //CONFIG_SECONDARY_IP
		snprintf(serverip, 16, "%s", inet_ntoa(lanip1));

	fprintf(fp, "interface %s\n", LANIF);
	//ql add
	fprintf(fp, "server %s\n", ip); // tesia
	// Mason Yu. For Test
#if 0
	fprintf(fp, "locallyserved no\n" );
	if (mib_get(MIB_ADSL_WAN_DHCPS, (void *)value) != 0)
	{
		if (((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(dhcps, inet_ntoa(*((struct in_addr *)value)), 16);
			dhcps[15] = '\0';
		}
		else
			dhcps[0] = '\0';
	}
	fprintf(fp, "dhcpserver %s\n", dhcps);
#endif

	if (spc_enable)
	{
		if (spc_ip == 0) { // single private ip
			fprintf(fp, "start %s\n", ip_start); // tesia
			fprintf(fp, "end %s\n", ip_end); //tesia
		}
		//else { // single shared ip
	}
	else
	{
		fprintf(fp, "start %s\n", ip_start); //tesia
		fprintf(fp, "end %s\n", ip_end); //tesia
	}
#if 0
#ifdef IP_BASED_CLIENT_TYPE
	fprintf(fp, "pcstart %s\n", pcipstart);
	fprintf(fp, "pcend %s\n", pcipend);
	fprintf(fp, "cmrstart %s\n", cmripstart);
	fprintf(fp, "cmrend %s\n", cmripend);
	fprintf(fp, "stbstart %s\n", stbipstart);
	fprintf(fp, "stbend %s\n", stbipend);
	fprintf(fp, "phnstart %s\n", phnipstart);
	fprintf(fp, "phnend %s\n", phnipend);
	fprintf(fp, "hgwstart %s\n", hgwipstart);
	fprintf(fp, "hgwend %s\n", hgwipend);
	//ql 20090122 add
	fprintf(fp, "pcopt60 %s\n", pcopt60);
	fprintf(fp, "cmropt60 %s\n", cmropt60);
	fprintf(fp, "stbopt60 %s\n", stbopt60);
	fprintf(fp, "phnopt60 %s\n", phnopt60);
#endif
#endif

#if 0
/*ping_zhang:20090319 START:replace ip range with serving pool of tr069*/
#ifndef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
#ifdef IP_BASED_CLIENT_TYPE
	{
		MIB_CE_IP_RANGE_DHCP_T Entry;
		int i, entrynum=mib_chain_total(MIB_IP_RANGE_DHCP_TBL);

		for (i=0; i<entrynum; i++)
		{
			if (!mib_chain_get(MIB_IP_RANGE_DHCP_TBL, i, (void *)&Entry))
				continue;

			strncpy(devipstart, inet_ntoa(*((struct in_addr *)Entry.startIP)), 16);
			strncpy(devipend, inet_ntoa(*((struct in_addr *)Entry.endIP)), 16);

/*ping_zhang:20090317 START:change len of option60 to 100*/
			//fprintf(fp, "range start=%s:end=%s:option=%s:devicetype=%d\n", devipstart, devipend, Entry.option60,Entry.deviceType);
			//fprintf(fp, "range start=%s:end=%s:option=%s:devicetype=%d:optCode=%d:optStr=%s\n", devipstart, devipend, Entry.option60,Entry.deviceType,Entry.optionCode,Entry.optionStr);
			//fprintf(fp, "range s=%s:e=%s:o=%s:t=%d:oC=%d:oS=%s\n",	devipstart, devipend, Entry.option60,Entry.deviceType,Entry.optionCode,Entry.optionStr);
			fprintf(fp, "range i=%d:s=%s:e=%s:o=%s:t=%d\n",i,devipstart, devipend, Entry.option60,Entry.deviceType);
			fprintf(fp, "range_optRsv i=%d:oC=%d:oS=%s\n",i,Entry.optionCode,Entry.optionStr);
/*ping_zhang:20090317 END*/
		}
	}
#endif
#endif
#endif //#if 0

/*ping_zhang:20090319 END*/
#ifdef IMAGENIO_IPTV_SUPPORT
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
	fprintf(fp, "opchaddr %s\n", opchaddr);
	fprintf(fp, "opchport %d\n", opchport);
	fprintf(fp, "stbdns1 %s\n", stbdns1);
	if (stbdns2[0])
		fprintf(fp, "stbdns2 %s\n", stbdns2);
#endif
/*ping_zhang:20090930 END*/
#endif

#ifdef IP_PASSTHROUGH
	ippt = 0;
	if (mib_get(MIB_IPPT_ITF, (void *)&ippt_itf) != 0)
		if (ippt_itf != DUMMY_IFINDEX) // IP passthrough
			ippt = 1;

	if (ippt)
	{
		fprintf(fp, "ippt yes\n");
		mib_get(MIB_IPPT_LEASE, (void *)&uInt);
		fprintf(fp, "ipptlt %d\n", uInt);
	}
#endif

	fprintf(fp, "opt subnet %s\n", dhcpsubnet);

	// Added by Mason Yu
	if (mib_get(MIB_ADSL_LAN_DHCP_GATEWAY, (void *)value) != 0)
	{
		strncpy(ipaddr2, inet_ntoa(*((struct in_addr *)value)), 16);
		ipaddr2[15] = '\0';
	}
	else{
		fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
		chmod(DHCPD_CONF,0666);
#endif
		return -1;
	}

	snprintf(ipaddr2, sizeof(ipaddr2), "%s", ip); // tesia
	fprintf(fp, "opt router %s\n", ipaddr2);

	// Modified by Mason Yu for LAN_IP as DNS Server
#if 0
	if (dnsMode == DNS_AUTO)
	{
		fprintf(fp, "opt dns %s\n", ipaddr);
	}
	else	// DNS_MANUAL
	{
		if (dns1[0])
			fprintf(fp, "opt dns %s\n", dns1);
		if (dns2[0])
			fprintf(fp, "opt dns %s\n", dns2);
		if (dns3[0])
			fprintf(fp, "opt dns %s\n", dns3);
	}
#endif

	// Kaohj
#if 0
#ifdef DHCPS_DNS_OPTIONS
	mib_get(MIB_DHCP_DNS_OPTION, (void *)value);
	if (value[0] == 0)
	fprintf(fp, "opt dns %s\n", ipaddr2); // tesia
	else { // check manual setting
		mib_get(MIB_DHCPS_DNS1, (void *)&myip);
		strncpy(ipaddr, inet_ntoa(myip), 16);
		ipaddr[15] = '\0';
		fprintf(fp, "opt dns %s\n", ipaddr);
		mib_get(MIB_DHCPS_DNS2, (void *)&myip);
		if (myip.s_addr != INADDR_NONE) { // not empty
			strncpy(ipaddr, inet_ntoa(myip), 16);
			ipaddr[15] = '\0';
			fprintf(fp, "opt dns %s\n", ipaddr);
			mib_get(MIB_DHCPS_DNS3, (void *)&myip);
			if (myip.s_addr != INADDR_NONE) { // not empty
				strncpy(ipaddr, inet_ntoa(myip), 16);
				ipaddr[15] = '\0';
				fprintf(fp, "opt dns %s\n", ipaddr);
			}
		}
	}
#else
	fprintf(fp, "opt dns %s\n", ipaddr2);
#endif
#else
	fprintf(fp, "opt dns %s\n", ipaddr2); // tesia
#endif


	fprintf(fp, "opt lease %u\n", uLTime);
	if (domain[0])
		fprintf(fp, "opt domain %s\n", domain);
	else
		// per TR-068, I-188
		fprintf(fp, "opt domain domain_not_set.invalid\n");

#ifdef _CWMP_MIB_ /*jiunming, for cwmp-tr069*/
{
	//format: opt venspec [enterprise-number] [sub-option code] [sub-option data] ...
	//opt vendspec: dhcp option 125 Vendor-Identifying Vendor-Specific
	//enterprise-number: 3561(ADSL Forum)
	//sub-option code: 4(GatewayManufacturerOUI)
	//sub-option code: 5(GatewaySerialNumber)
	//sub-option code: 6(GatewayProductClass)
	char opt125_sn[65];
	mib_get(MIB_HW_SERIAL_NUMBER, (void *)opt125_sn);
	fprintf( fp, "opt venspec 3561 4 %s 5 %s 6 %s\n", MANUFACTURER_OUI, opt125_sn, PRODUCT_CLASS );
}
#endif

#if !defined(CONFIG_00R0)
#ifndef CONFIG_TELMEX_DEV
	//Mark because project 00R0 will use ntp.local as default NTP Server and it will
	//Slow the udhcpd init time.
	/* DHCP option 42 */
	mib_get( MIB_NTP_SERVER_ID, (void *)&vChar);
	if (vChar == 0) {
		mib_get(MIB_NTP_SERVER_HOST1, value);
		fprintf(fp, "opt ntpsrv %s\n", value);
	}
	else
	{
		mib_get(MIB_NTP_SERVER_HOST5, (void *)value5);
		mib_get(MIB_NTP_SERVER_HOST4, (void *)value4);
		mib_get(MIB_NTP_SERVER_HOST3, (void *)value3);
		mib_get(MIB_NTP_SERVER_HOST2, (void *)value);
		mib_get(MIB_NTP_SERVER_HOST1, (void *)value1);
		fprintf(fp, "opt ntpsrv %s,%s,%s,%s,%s\n", value,value1,value3,vlaue4,value5);
	}
#else
	unsigned char serverAddr[150];
	int serverIndex, length=0;

	mib_get_s( MIB_NTP_SERVER_ID, (void *)&vChar, sizeof(vChar));
	memset(serverAddr, 0, sizeof(serverAddr));

	if (vChar & 0x01)
	{
		mib_get(MIB_NTP_SERVER_HOST1, value);
		length += sprintf(serverAddr + length, "%s", value);
	}
	if (vChar & 0x02)
	{
		mib_get(MIB_NTP_SERVER_HOST2, value);
		if (serverAddr[0])
			length += sprintf(serverAddr + length, ",%s", value);
		else
			length += sprintf(serverAddr + length, "%s", value);
	}
	if (vChar & 0x04)
	{
		mib_get(MIB_NTP_SERVER_HOST3, value);
		if (serverAddr[0])
			length += sprintf(serverAddr + length, ",%s", value);
		else
			length += sprintf(serverAddr + length, "%s", value);
	}
	if (vChar & 0x08)
	{
		mib_get(MIB_NTP_SERVER_HOST4, value);
		if (serverAddr[0])
			length += sprintf(serverAddr + length, ",%s", value);
		else
			length += sprintf(serverAddr + length, "%s", value);
	}
	if (vChar & 0x10)
	{
		mib_get(MIB_NTP_SERVER_HOST5, value);
		if (serverAddr[0])
			length += sprintf(serverAddr + length, ",%s", value);
		else
			length += sprintf(serverAddr + length, "%s", value);
	}

	fprintf(fp, "opt ntpsrv %s\n", serverAddr);
#endif
#endif

	/* DHCP option 66 */
	if (mib_get(MIB_TFTP_SERVER_ADDR, (void *)&optionStr) != 0) { //Length max 254
		fprintf(fp, "opt tftp %s\n", optionStr);
	}

#if !defined(CONFIG_00R0)
	//Mark because project 00R0 will use ntp.local as default NTP Server and it will
	//Slow the udhcpd init time.
	/* DHCP option 100 */
	if (mib_get(MIB_POSIX_TZ_STRING, (void *)&optionStr) != 0) { //Length max 254
		fprintf(fp, "opt tzstring %s\n", optionStr);
	}
#endif
#ifdef CONFIG_USER_RTK_VOIP //single SIP server right now /* DHCP option 120 */

#include <errno.h>
#include <fcntl.h>
#include "voip_manager.h"
#define SIP_PROXY_NUM 2

	voipCfgParam_t *voip_pVoIPCfg = NULL;
	voipCfgPortParam_t *pCfg;
	int count = 0;
	int ret;

	int enc[SIP_PROXY_NUM];
	long ipaddrs=0;

	if(voip_pVoIPCfg==NULL){
		if (voip_flash_get(&voip_pVoIPCfg) != 0){
			fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
			chmod(DHCPD_CONF,0666);
#endif
			return -1;
		}
	}

	pCfg = &voip_pVoIPCfg->ports[0];
	for( i=0 ; i<MAX_PROXY ; i++ ){
		if(*(char *)pCfg->proxies[i].addr){
			ipaddrs = inet_network(pCfg->proxies[i].addr);
			if (ipaddrs >= 0){
				enc[i] = 1;
				fprintf(fp, "opt sipsrv %d %s\n", enc[i], pCfg->proxies[i].addr);
			}
			else {
				if(strlen(pCfg->proxies[0].addr) > 5) { //Avoid to the NULL string, Iulian Wu
					enc[i] = 0;
					fprintf(fp, "opt sipsrv %d %s\n", enc[i], pCfg->proxies[i].addr);
				}
			}
			break;
		}
	}

#endif

/*write dhcp serving pool config*/
/*ping_zhang:20080919 START:add for new telefonica tr069 request: dhcp option*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	fprintf(fp, "poolend end\n");
#endif

#ifdef CONFIG_CT_AWIFI_JITUAN_FEATURE
		unsigned char functype=0;
		mib_get(AWIFI_PROVINCE_CODE, &functype);
		if(functype == AWIFI_ZJ){
		memset(&dhcppoolentry,0,sizeof(DHCPS_SERVING_POOL_T));

	/*test code*
		dhcppoolentry.enable=1;
		dhcppoolentry.poolorder=1;
		strcpy(dhcppoolentry.poolname,"poolone");
		strcpy(dhcppoolentry.vendorclass,"MSFT 5.0");
		strcpy(dhcppoolentry.clientid,"");
		strcpy(dhcppoolentry.userclass,"");

		inet_aton("192.168.1.40",((struct in_addr *)(dhcppoolentry.startaddr)));
		inet_aton("192.168.1.50",((struct in_addr *)(dhcppoolentry.endaddr)));
		inet_aton("255.255.255.0",((struct in_addr *)(dhcppoolentry.subnetmask)));
		inet_aton("192.168.1.1",((struct in_addr *)(dhcppoolentry.iprouter)));
		inet_aton("172.29.17.10",((struct in_addr *)(dhcppoolentry.dnsserver1)));
		strcpy(dhcppoolentry.domainname,"poolone.com");
		dhcppoolentry.leasetime=86400;
		dhcppoolentry.dnsservermode=1;
		dhcppoolentry.InstanceNum=1;
		entryNum = mib_chain_total(MIB_DHCPS_SERVING_POOL_TBL);
	printf("\nentryNum=%d\n",entryNum);
		if(entryNum==0)
			mib_chain_add(MIB_DHCPS_SERVING_POOL_TBL,(void*)&dhcppoolentry);
	****/

		entryNum = mib_chain_total(MIB_DHCPS_SERVING_POOL_AWIFI_TBL);

		for(i=0;i<entryNum;i++){
			memset(&dhcppoolentry,0,sizeof(DHCPS_SERVING_POOL_T));
			if(!mib_chain_get(MIB_DHCPS_SERVING_POOL_AWIFI_TBL,i,(void*)&dhcppoolentry))
				continue;
			if(dhcppoolentry.enable==1){
				strncpy(ipstart, inet_ntoa(*((struct in_addr *)(dhcppoolentry.startaddr))), 16);
				strncpy(ipend, inet_ntoa(*((struct in_addr *)(dhcppoolentry.endaddr))), 16);
				strncpy(dhcpsubnet, inet_ntoa(*((struct in_addr *)(dhcppoolentry.subnetmask))), 16);
				strncpy(ipaddr2, inet_ntoa(*((struct in_addr *)(dhcppoolentry.iprouter))), 16);

				if( dhcppoolentry.localserved ) //check only for locallyserved==true
				{
					if(!strcmp(ipstart,ipempty) ||
						!strcmp(ipend,ipempty) ||
						!strcmp(dhcpsubnet,ipempty))
						continue;
				}


				fprintf(fp, "poolname %s\n",dhcppoolentry.poolname);
				fprintf(fp, "cwmpinstnum %d\n",dhcppoolentry.InstanceNum);
				fprintf(fp, "poolorder %u\n",dhcppoolentry.poolorder);
				fprintf(fp, "interface %s\n", LANIF);
				//ql add
				mib_get(MIB_ADSL_LAN_IP2, (void *)&lanip2);
				snprintf(serverip_awifi, 16, "%s", inet_ntoa(lanip2));

				fprintf(fp, "server %s\n", serverip_awifi);
				fprintf(fp, "start %s\n", ipstart);
				fprintf(fp, "end %s\n", ipend);

				fprintf(fp, "sourceinterface %u\n",dhcppoolentry.sourceinterface);
				if(dhcppoolentry.vendorclass[0]!=0){
					fprintf(fp, "vendorclass %s\n",dhcppoolentry.vendorclass);
					fprintf(fp, "vendorclassflag %u\n",dhcppoolentry.vendorclassflag);
					if(dhcppoolentry.vendorclassmode[0]!=0)
						fprintf(fp, "vendorclassmode %s\n",dhcppoolentry.vendorclassmode);
				}
				if(dhcppoolentry.clientid[0]!=0){
					fprintf(fp, "clientid %s\n",dhcppoolentry.clientid);
					fprintf(fp, "clientidflag %u\n",dhcppoolentry.clientidflag);
				}
				if(dhcppoolentry.userclass[0]!=0){
					fprintf(fp, "userclass %s\n",dhcppoolentry.userclass);
					fprintf(fp, "userclassflag %u\n",dhcppoolentry.userclassflag);
				}
				if(memcmp(dhcppoolentry.chaddr,macempty,6)){
					fprintf(fp, "chaddr %02x%02x%02x%02x%02x%02x\n",dhcppoolentry.chaddr[0],dhcppoolentry.chaddr[1],dhcppoolentry.chaddr[2],
						dhcppoolentry.chaddr[3],dhcppoolentry.chaddr[4],dhcppoolentry.chaddr[5]);
					if(memcmp(dhcppoolentry.chaddrmask,macempty,6))
						fprintf(fp, "chaddrmask %02x%02x%02x%02x%02x%02x\n",dhcppoolentry.chaddrmask[0],dhcppoolentry.chaddrmask[1],dhcppoolentry.chaddrmask[2],
							dhcppoolentry.chaddrmask[3],dhcppoolentry.chaddrmask[4],dhcppoolentry.chaddrmask[5]);
					fprintf(fp, "chaddrflag %u\n",dhcppoolentry.chaddrflag);
				}

#ifdef IP_PASSTHROUGH
				ippt = 0;
				if (mib_get(MIB_IPPT_ITF, (void *)&ippt_itf) != 0)
					if (ippt_itf != DUMMY_IFINDEX) // IP passthrough
						ippt = 1;

				if (ippt)
				{
					fprintf(fp, "ippt yes\n");
					mib_get(MIB_IPPT_LEASE, (void *)&uInt);
					fprintf(fp, "ipptlt %d\n", uInt);
				}
#endif
#ifdef IMAGENIO_IPTV_SUPPORT
				//fprintf(fp, "pcopt60 %s\n", pcopt60);
				//fprintf(fp, "cmropt60 %s\n", cmropt60);
				//fprintf(fp, "stbopt60 %s\n", stbopt60);
				//fprintf(fp, "phnopt60 %s\n", phnopt60);
	/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
				fprintf(fp, "opchaddr %s\n", opchaddr);
				fprintf(fp, "opchport %d\n", opchport);
				fprintf(fp, "stbdns1 %s\n", stbdns1);
				if (stbdns2[0])
					fprintf(fp, "stbdns2 %s\n", stbdns2);
#endif
	/*ping_zhang:20090930 END*/
#endif

				if(strcmp(dhcpsubnet,ipempty))
					fprintf(fp, "opt subnet %s\n", dhcpsubnet);
				if(strcmp(ipaddr2,ipempty))
					fprintf(fp, "opt router %s\n", ipaddr2);

				// Kaohj
#ifdef DHCPS_DNS_OPTIONS
				if (dhcppoolentry.dnsservermode == 0)
					fprintf(fp, "opt dns %s\n", ipaddr2);
				else { // check manual setting
					strncpy(ipaddr, inet_ntoa(*((struct in_addr *)(dhcppoolentry.dnsserver1))), 16);
					ipaddr[15] = '\0';
					fprintf(fp, "opt dns %s\n", ipaddr);
					strncpy(ipaddr, inet_ntoa(*((struct in_addr *)(dhcppoolentry.dnsserver2))), 16);
					ipaddr[15] = '\0';
					if(strcmp(ipaddr,ipempty)) { // not empty
						fprintf(fp, "opt dns %s\n", ipaddr);
						strncpy(ipaddr, inet_ntoa(*((struct in_addr *)(dhcppoolentry.dnsserver3))), 16);
						ipaddr[15] = '\0';
						if(strcmp(ipaddr,ipempty)) { // not empty
							fprintf(fp, "opt dns %s\n", ipaddr);
						}
					}
				}
#else
				fprintf(fp, "opt dns %s\n", ipaddr2);
#endif
				fprintf(fp, "opt lease %u\n", (unsigned int)(dhcppoolentry.leasetime));
#ifndef CTC_YUNMESTB_DHCPD_DHCPOPTION
				if (dhcppoolentry.domainname[0])
					fprintf(fp, "opt domain %s\n", dhcppoolentry.domainname);
				else
					// per TR-068, I-188
					fprintf(fp, "opt domain domain_not_set.invalid\n");
#endif
#ifdef _CWMP_MIB_ /*jiunming, for cwmp-tr069*/
#ifdef _PRMT_X_CT_COM_DHCP_
			{
				//format: opt venspec [enterprise-number] [sub-option code] [sub-option data] ...
				//opt vendspec: dhcp option 125 Vendor-Identifying Vendor-Specific
				//enterprise-number: 0(e8 temprary enterprise code)
				//sub-option code: 2(DHCP server type)
				//sub-option code: 10(Internet VID)
				//sub-option code: 11(IPTV VID)
				//sub-option code: 12(TR069 VID)
				//sub-option code: 13(VoIP VID)
				unsigned char send_opt_125;
				mib_get(CWMP_CT_DHCPS_SEND_OPT_125, &send_opt_125);
				if(send_opt_125)
					fprintf( fp, "opt venspec 0 2 %s\n", "HGW-CT");
			}
#else
			{
				//format: opt venspec [enterprise-number] [sub-option code] [sub-option data] ...
				//opt vendspec: dhcp option 125 Vendor-Identifying Vendor-Specific
				//enterprise-number: 3561(ADSL Forum)
				//sub-option code: 4(GatewayManufacturerOUI)
				//sub-option code: 5(GatewaySerialNumber)
				//sub-option code: 6(GatewayProductClass)
				char opt125_sn[65];
				mib_get(MIB_HW_SERIAL_NUMBER, (void *)opt125_sn);
				fprintf( fp, "opt venspec 3561 4 %s 5 %s 6 %s\n", MANUFACTURER_OUI, opt125_sn, PRODUCT_CLASS );
			}
#endif
#endif
				//relayinfo
				if( dhcppoolentry.localserved==0 )
				{
					char dhcpserveripaddr[16];
					fprintf(fp, "locallyserved no\n" );
					strncpy(dhcpserveripaddr, inet_ntoa(*((struct in_addr *)(dhcppoolentry.dhcprelayip))), 16);
					dhcpserveripaddr[15]='\0';
					fprintf(fp, "dhcpserver %s\n", dhcpserveripaddr );
				}

				fprintf(fp, "poolend end\n");

			}
		}
		}
#endif

#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	memset(&dhcppoolentry,0,sizeof(DHCPS_SERVING_POOL_T));

/*test code*
	dhcppoolentry.enable=1;
	dhcppoolentry.poolorder=1;
	strcpy(dhcppoolentry.poolname,"poolone");
	strcpy(dhcppoolentry.vendorclass,"MSFT 5.0");
	strcpy(dhcppoolentry.clientid,"");
	strcpy(dhcppoolentry.userclass,"");

	inet_aton("192.168.1.40",((struct in_addr *)(dhcppoolentry.startaddr)));
	inet_aton("192.168.1.50",((struct in_addr *)(dhcppoolentry.endaddr)));
	inet_aton("255.255.255.0",((struct in_addr *)(dhcppoolentry.subnetmask)));
	inet_aton("192.168.1.1",((struct in_addr *)(dhcppoolentry.iprouter)));
	inet_aton("172.29.17.10",((struct in_addr *)(dhcppoolentry.dnsserver1)));
	strcpy(dhcppoolentry.domainname,"poolone.com");
	dhcppoolentry.leasetime=86400;
	dhcppoolentry.dnsservermode=1;
	dhcppoolentry.InstanceNum=1;
	entryNum = mib_chain_total(MIB_DHCPS_SERVING_POOL_TBL);
printf("\nentryNum=%d\n",entryNum);
	if(entryNum==0)
		mib_chain_add(MIB_DHCPS_SERVING_POOL_TBL,(void*)&dhcppoolentry);
****/

	entryNum = mib_chain_total(MIB_DHCPS_SERVING_POOL_TBL);

	for(i=0;i<entryNum;i++){
		memset(&dhcppoolentry,0,sizeof(DHCPS_SERVING_POOL_T));
		if(!mib_chain_get(MIB_DHCPS_SERVING_POOL_TBL,i,(void*)&dhcppoolentry))
			continue;
		if(dhcppoolentry.enable==1){
			strncpy(ipstart, inet_ntoa(*((struct in_addr *)(dhcppoolentry.startaddr))), 16);
			strncpy(ipend, inet_ntoa(*((struct in_addr *)(dhcppoolentry.endaddr))), 16);
			strncpy(dhcpsubnet, inet_ntoa(*((struct in_addr *)(dhcppoolentry.subnetmask))), 16);
			strncpy(ipaddr2, inet_ntoa(*((struct in_addr *)(dhcppoolentry.iprouter))), 16);

			if( dhcppoolentry.localserved ) //check only for locallyserved==true
			{
				if(!strcmp(ipstart,ipempty) ||
					!strcmp(ipend,ipempty) ||
					!strcmp(dhcpsubnet,ipempty))
					continue;
			}


			fprintf(fp, "poolname %s\n",dhcppoolentry.poolname);
			fprintf(fp, "cwmpinstnum %d\n",dhcppoolentry.InstanceNum);
			fprintf(fp, "poolorder %u\n",dhcppoolentry.poolorder);
			fprintf(fp, "interface %s\n", LANIF);
			//ql add
			fprintf(fp, "server %s\n", serverip);
			fprintf(fp, "start %s\n", ipstart);
			fprintf(fp, "end %s\n", ipend);

			fprintf(fp, "sourceinterface %u\n",dhcppoolentry.sourceinterface);
			if(dhcppoolentry.vendorclass[0]!=0){
				fprintf(fp, "vendorclass %s\n",dhcppoolentry.vendorclass);
				fprintf(fp, "vendorclassflag %u\n",dhcppoolentry.vendorclassflag);
				if(dhcppoolentry.vendorclassmode[0]!=0)
					fprintf(fp, "vendorclassmode %s\n",dhcppoolentry.vendorclassmode);
			}
			if(dhcppoolentry.clientid[0]!=0){
				fprintf(fp, "clientid %s\n",dhcppoolentry.clientid);
				fprintf(fp, "clientidflag %u\n",dhcppoolentry.clientidflag);
			}
			if(dhcppoolentry.userclass[0]!=0){
				fprintf(fp, "userclass %s\n",dhcppoolentry.userclass);
				fprintf(fp, "userclassflag %u\n",dhcppoolentry.userclassflag);
			}
			if(memcmp(dhcppoolentry.chaddr,macempty,6)){
				fprintf(fp, "chaddr %02x%02x%02x%02x%02x%02x\n",dhcppoolentry.chaddr[0],dhcppoolentry.chaddr[1],dhcppoolentry.chaddr[2],
					dhcppoolentry.chaddr[3],dhcppoolentry.chaddr[4],dhcppoolentry.chaddr[5]);
				if(memcmp(dhcppoolentry.chaddrmask,macempty,6))
					fprintf(fp, "chaddrmask %02x%02x%02x%02x%02x%02x\n",dhcppoolentry.chaddrmask[0],dhcppoolentry.chaddrmask[1],dhcppoolentry.chaddrmask[2],
						dhcppoolentry.chaddrmask[3],dhcppoolentry.chaddrmask[4],dhcppoolentry.chaddrmask[5]);
				fprintf(fp, "chaddrflag %u\n",dhcppoolentry.chaddrflag);
			}

#ifdef IP_PASSTHROUGH
			ippt = 0;
			if (mib_get(MIB_IPPT_ITF, (void *)&ippt_itf) != 0)
				if (ippt_itf != DUMMY_IFINDEX) // IP passthrough
					ippt = 1;

			if (ippt)
			{
				fprintf(fp, "ippt yes\n");
				mib_get(MIB_IPPT_LEASE, (void *)&uInt);
				fprintf(fp, "ipptlt %d\n", uInt);
			}
#endif
#ifdef IMAGENIO_IPTV_SUPPORT
			//fprintf(fp, "pcopt60 %s\n", pcopt60);
			//fprintf(fp, "cmropt60 %s\n", cmropt60);
			//fprintf(fp, "stbopt60 %s\n", stbopt60);
			//fprintf(fp, "phnopt60 %s\n", phnopt60);
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
			fprintf(fp, "opchaddr %s\n", opchaddr);
			fprintf(fp, "opchport %d\n", opchport);
			fprintf(fp, "stbdns1 %s\n", stbdns1);
			if (stbdns2[0])
				fprintf(fp, "stbdns2 %s\n", stbdns2);
#endif
/*ping_zhang:20090930 END*/
#endif

			if(strcmp(dhcpsubnet,ipempty))
				fprintf(fp, "opt subnet %s\n", dhcpsubnet);
			if(strcmp(ipaddr2,ipempty))
				fprintf(fp, "opt router %s\n", ipaddr2);

			// Kaohj
#ifdef DHCPS_DNS_OPTIONS
			if (dhcppoolentry.dnsservermode == 0)
				fprintf(fp, "opt dns %s\n", ipaddr2);
			else { // check manual setting
				strncpy(ipaddr, inet_ntoa(*((struct in_addr *)(dhcppoolentry.dnsserver1))), 16);
				ipaddr[15] = '\0';
				fprintf(fp, "opt dns %s\n", ipaddr);
				strncpy(ipaddr, inet_ntoa(*((struct in_addr *)(dhcppoolentry.dnsserver2))), 16);
				ipaddr[15] = '\0';
				if(strcmp(ipaddr,ipempty)) { // not empty
					fprintf(fp, "opt dns %s\n", ipaddr);
					strncpy(ipaddr, inet_ntoa(*((struct in_addr *)(dhcppoolentry.dnsserver3))), 16);
					ipaddr[15] = '\0';
					if(strcmp(ipaddr,ipempty)) { // not empty
						fprintf(fp, "opt dns %s\n", ipaddr);
					}
				}
			}
#else
			fprintf(fp, "opt dns %s\n", ipaddr2);
#endif
			fprintf(fp, "opt lease %u\n", (unsigned int)(dhcppoolentry.leasetime));
			if (dhcppoolentry.domainname[0])
				fprintf(fp, "opt domain %s\n", dhcppoolentry.domainname);
			else
				// per TR-068, I-188
				fprintf(fp, "opt domain domain_not_set.invalid\n");

#ifdef _CWMP_MIB_ /*jiunming, for cwmp-tr069*/
		{
			//format: opt venspec [enterprise-number] [sub-option code] [sub-option data] ...
			//opt vendspec: dhcp option 125 Vendor-Identifying Vendor-Specific
			//enterprise-number: 3561(ADSL Forum)
			//sub-option code: 4(GatewayManufacturerOUI)
			//sub-option code: 5(GatewaySerialNumber)
			//sub-option code: 6(GatewayProductClass)
			char opt125_sn[65];
			mib_get(MIB_HW_SERIAL_NUMBER, (void *)opt125_sn);
			fprintf( fp, "opt venspec 3561 4 %s 5 %s 6 %s\n", MANUFACTURER_OUI, opt125_sn, PRODUCT_CLASS );
		}
#endif
			//relayinfo
			if( dhcppoolentry.localserved==0 )
			{
				char dhcpserveripaddr[16];
				fprintf(fp, "locallyserved no\n" );
				strncpy(dhcpserveripaddr, inet_ntoa(*((struct in_addr *)(dhcppoolentry.dhcprelayip))), 16);
				dhcpserveripaddr[15]='\0';
				fprintf(fp, "dhcpserver %s\n", dhcpserveripaddr );
			}

			fprintf(fp, "poolend end\n");

		}
	}
#endif
/*ping_zhang:20080919 END*/

	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(DHCPD_CONF,0666);
#endif

//star: retain the lease file between during dhcdp restart
	if((fp = fopen(DHCPD_LEASE, "r")) == NULL)
	{
		if ((fp = fopen(DHCPD_LEASE, "w")) == NULL)
		{
			printf("Open file %s failed !\n", DHCPD_LEASE);
			return -1;
		}
		fprintf(fp, "\n");
		fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
		chmod(DHCPD_LEASE, 0666);
#endif
	}
	else
		fclose(fp);

	return 1;
}
#endif

int delete_line(const char *filename, int delete_line)
{
	FILE *fileptr1, *fileptr2;
	int ch;
	int temp = 1;
	char *tmpFileName="/var/tmp/replica";

	//open file in read mode
	fileptr1 = fopen(filename, "r");
	if(fileptr1){
		fseek(fileptr1, 0, SEEK_SET);

		//open new file in write mode
		fileptr2 = fopen(tmpFileName, "w+");
		if(fileptr2){
			while ((ch = getc(fileptr1)) != EOF)
			{
				if (ch == '\n')
					temp++;

				//except the line to be deleted
				if (temp != delete_line)
				{
					//copy all lines in file replica.c
					putc(ch, fileptr2);
				}
			}
			fclose(fileptr2);
		}
		fclose(fileptr1);
	}
	if(remove(filename) != 0)
		printf("Unable to delete the file: %s %d\n", __func__, __LINE__);
	//rename the file replica.c to original name
	if(rename(tmpFileName, filename) != 0)
		printf("Unable to rename the file: %s %d\n", __func__, __LINE__);
	return 0;
}

#if defined(CONFIG_USER_DNSMASQ_DNSMASQ)
int delete_hosts(unsigned char *yiaddr)
{
	FILE *fp;
	int dnsrelaypid=0;
	char temps[256+MAX_NAME_LEN]="", *pwd;
	char tmp1[20]="";
	int tmp_line = 0;

del_again:
	if ((fp = fopen(HOSTS, "r")) == NULL)
	{
		printf("Open file %s failed !\n", HOSTS);
		return 0;
	}

	fseek(fp, 0, SEEK_SET);
	temps[0] = '\0';
	tmp1[0] = '\0';
	tmp_line = 0;

	while (fgets(temps, 256+MAX_NAME_LEN, fp)) {
		tmp_line++;
		pwd = strstr(temps, yiaddr);
		if (pwd) {
			if(sscanf(temps, "%s%*", tmp1) == 1)
			{
				if (!strcmp(tmp1, yiaddr)) {
					fclose(fp);
					delete_line(HOSTS, tmp_line);
					goto del_again;
				}
			}
		}
	}
	fclose(fp);

	dnsrelaypid = read_pid((char*)DNSRELAYPID);
	if(dnsrelaypid > 0)
		kill(dnsrelaypid, SIGHUP);
	return 1;
}

int delete_dsldevice_on_hosts()
{
	/* delete entry with deviceName */
	char devName[MAX_DEV_NAME_LEN]={0}, devName_tmp[MAX_DEV_NAME_LEN+2];
	FILE *fp, *fp1;
	char temps[256], *pwd, tmpFileName[64];
	int dnsrelaypid=0;

	mib_get_s(MIB_DEVICE_NAME, (void *)devName, sizeof(devName));
	if(devName[0] == '\0')
	{
		strcpy(devName, DEFAULT_DEVICE_NAME); //if devName is NULL
	}

	if ((fp = fopen(HOSTS, "r")) == NULL)
 	{
 		printf("Open file %s failed !\n", HOSTS);
 		return 0;
 	}

	snprintf(tmpFileName, sizeof(tmpFileName), "%s_tmp", HOSTS);
	if ((fp1 = fopen(tmpFileName, "w")) == NULL)
	{
		printf("Open file %s failed !\n", tmpFileName);
		fclose(fp);
		return 0;
	}

	snprintf(devName_tmp, sizeof(devName_tmp), " %s ", devName);
	while (fgets(temps, sizeof(temps)-1, fp)) {
		if(!strstr(temps, devName_tmp))
		{
			fprintf(fp1, "%s", temps);
		}
	}

	fclose(fp);
	fclose(fp1);

	unlink(HOSTS);
	rename(tmpFileName, HOSTS);

	dnsrelaypid = read_pid((char*)DNSRELAYPID);
	if(dnsrelaypid > 0)
		kill(dnsrelaypid, SIGHUP);

	return 1;
}

int add_dsldevice_on_hosts()
{
	unsigned char value, value2[32];
	FILE *fp;
	char domain[MAX_NAME_LEN] = {0};
	char devName[MAX_DEV_NAME_LEN+2] = {0};
	int dnsrelaypid=0;

	mib_get_s(MIB_DEVICE_NAME, (void *)devName, sizeof(devName));
	if(devName[0] == '\0')
	{
		strcpy(devName, DEFAULT_DEVICE_NAME); //if devName is NULL
	}

#ifdef CONFIG_USER_DHCP_SERVER
	mib_get_s(MIB_DHCP_MODE, (void *)&value, sizeof(value));
	if (value== DHCP_SERVER)
	{
		if ((fp = fopen(HOSTS, "a")) == NULL)
		{
			printf("Open file %s failed on DHCP_SERVER mode !\n", HOSTS);
			return -1;
		}
	}
	else
#endif
	{
		if ((fp = fopen(HOSTS, "w")) == NULL)
		{
			printf("Open file %s failed on Not DHCP_SERVER mode !\n", HOSTS);
			return -1;
		}
	}

	struct in_addr inAddr;
	if(!getInAddr(ALIASNAME_BR0, IP_ADDR, (void *)&inAddr)){
		printf("[%s] Get %s address fail !\n", __FUNCTION__, ALIASNAME_BR0);
		fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
		chmod(HOSTS,0666);
#endif
		return -1;
	}

	fprintf(fp, "%s\t", inet_ntoa(inAddr));
#ifdef CONFIG_USER_DHCP_SERVER
	if (value  == DHCP_SERVER)
	{	// add domain
		mib_get_s(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)domain, sizeof(domain));
		if (domain[0])
			fprintf(fp, "%s.%s ", devName, domain);
	}
#endif
	//need add blank to devName for filter string from HOSTS
	fprintf(fp, " %s \n", devName);
	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(HOSTS,0666);
#endif

	dnsrelaypid = read_pid((char*)DNSRELAYPID);
	if(dnsrelaypid > 0)
		kill(dnsrelaypid, SIGHUP);

	return 1;
}

/* clear dnsmasq cache
   return value: 1 success, 0 fail */
int reloadDnsRelay()
{
	int dnsrelaypid=0;
	dnsrelaypid = read_pid((char*)DNSRELAYPID);
	if(dnsrelaypid > 0)
	{
		kill(dnsrelaypid, SIGHUP);
		return 1;
	}
	return 0;
}

// DNS relay server configuration
// return value:
// 1  : successful
// -1 : startup failed
int startDnsRelay(void)
{
	unsigned char value[32];
	FILE *fp;
	DNS_TYPE_T dnsMode;
	char *str;
	unsigned int i, vcTotal, resolvopt;
	char dns1[16], dns2[16], dns3[16];
	char dnsv61[48]={0};
	char dnsv62[48]={0};
	char dnsv63[48]={0};
	char domain[MAX_NAME_LEN];

#if defined(CONFIG_USER_APP_DNS_NOT_GO_THROUGH_DNSMASQ)
	rtk_generate_resolv_file(RESOLV_BACKUP);

#else

	FILE *dnsfp=fopen(RESOLV,"w");		/* resolv.conf*/
	if(dnsfp){
		fprintf(dnsfp, "nameserver 127.0.0.1\n");
#ifdef CONFIG_IPV6
	fprintf(dnsfp, "nameserver ::1\n");
#endif

		fclose(dnsfp);
	}
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(RESOLV,0666);
#endif

#endif

#ifdef OLD_DNS_MANUALLY
	if (mib_get_s(MIB_ADSL_WAN_DNS1, (void *)value, sizeof(value)) != 0)
	{
		if (((struct in_addr *)value)->s_addr != INADDR_NONE && ((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(dns1, inet_ntoa(*((struct in_addr *)value)), 16);
			dns1[15] = '\0';
		}
		else
			dns1[0] = '\0';
	}
	else
		return -1;

	if (mib_get_s(MIB_ADSL_WAN_DNS2, (void *)value, sizeof(value)) != 0)
	{
		if (((struct in_addr *)value)->s_addr != INADDR_NONE && ((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(dns2, inet_ntoa(*((struct in_addr *)value)), 16);
			dns2[15] = '\0';
		}
		else
			dns2[0] = '\0';
	}
	else
		return -1;

	if (mib_get_s(MIB_ADSL_WAN_DNS3, (void *)value, sizeof(value)) != 0)
	{
		if (((struct in_addr *)value)->s_addr != INADDR_NONE && ((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(dns3, inet_ntoa(*((struct in_addr *)value)), 16);
			dns3[15] = '\0';
		}
		else
			dns3[0] = '\0';
	}
	else
		return -1;

#ifdef CONFIG_IPV6
	if (mib_get_s(MIB_ADSL_WAN_DNSV61, (void *)value, sizeof(value)) != 0)	{
		if (memcmp( ((struct in6_addr *)value)->s6_addr, dnsv61, 16))
		{
			inet_ntop(PF_INET6, &value, dnsv61, 48);
			dnsv61[47] = '\0';
		}
		else
			dnsv61[0] = '\0';
	}
	else
		return -1;

	if (mib_get_s(MIB_ADSL_WAN_DNSV62, (void *)value, sizeof(value)) != 0)	{
		if (memcmp( ((struct in6_addr *)value)->s6_addr, dnsv62, 16))
		{
			inet_ntop(PF_INET6, &value, dnsv62, 48);
			dnsv62[47] = '\0';
		}
		else
			dnsv62[0] = '\0';
	}
	else
		return -1;

	if (mib_get_s(MIB_ADSL_WAN_DNSV63, (void *)value, sizeof(value)) != 0)	{
		if (memcmp( ((struct in6_addr *)value)->s6_addr, dnsv63, 16))
		{
			inet_ntop(PF_INET6, &value, dnsv63, 48);
			dnsv63[47] = '\0';
		}
		else
			dnsv63[0] = '\0';
	}
	else
		return -1;
#endif

	// get DNS mode
	if (mib_get_s(MIB_ADSL_WAN_DNS_MODE, (void *)value, sizeof(value)) != 0)
	{
		dnsMode = (DNS_TYPE_T)(*(unsigned char *)value);
	}
	else
		return -1;

	if ((fp = fopen(MANUAL_RESOLV, "w")) == NULL)
	{
		printf("Open file %s failed !\n", MANUAL_RESOLV);
		return -1;
	}

	if (dns1[0])
		fprintf(fp, "nameserver %s\n", dns1);
	if (dns2[0])
		fprintf(fp, "nameserver %s\n", dns2);
	if (dns3[0])
		fprintf(fp, "nameserver %s\n", dns3);
#ifdef CONFIG_IPV6
	if (dnsv61[0])
		fprintf(fp, "nameserver %s\n", dnsv61);
	if (dnsv62[0])
		fprintf(fp, "nameserver %s\n", dnsv62);
	if (dnsv63[0])
		fprintf(fp, "nameserver %s\n", dnsv63);
#endif
	fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod(MANUAL_RESOLV,0666);
#endif

	if (dnsMode == DNS_MANUAL)
	{
		// dnsmasq -h -i LANIF -r MANUAL_RESOLV
		TRACE(STA_INFO, "get DNS from manual\n");
		str=(char *)MANUAL_RESOLV;
	}
	else	// DNS_AUTO
	{
#if 0
		MIB_CE_ATM_VC_T Entry;

		resolvopt = 0;
		vcTotal = mib_chain_total(MIB_ATM_VC_TBL);

		for (i = 0; i < vcTotal; i++)
		{
			/* get the specified chain record */
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				return -1;

			if (Entry.enable == 0)
				continue;

			if ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_PPPOE ||
				(CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_PPPOA)
				resolvopt = 1;
			else if ((DHCP_T)Entry.ipDhcp == DHCP_CLIENT)
				resolvopt = 2;
		}

#ifdef CONFIG_USER_PPPOMODEM
		{
			MIB_WAN_3G_T entry,*p;
			p=&entry;
			if(mib_chain_get( MIB_WAN_3G_TBL, 0, (void*)p))
			{
				if(p->enable) resolvopt = 1;
			}
		}
#endif //CONFIG_USER_PPPOMODEM

		if (resolvopt == 1) // get from PPP
		{
			// dnsmasq -h -i LANIF -r PPP_RESOLV
			TRACE(STA_INFO, "get DNS from PPP\n");
			str=(char *)PPP_RESOLV;
		}
		else if (resolvopt == 2) // get from DHCP client
		{
			// dnsmasq -h -i LANIF -r DNS_RESOLV
			TRACE(STA_INFO, "get DNS from DHCP client\n");
			str=(char *)DNS_RESOLV;
		}
		else	// get from manual
		{
			// dnsmasq -h -i LANIF -r MANUAL_RESOLV
			TRACE(STA_INFO, "get DNS from manual\n");
			str=(char *)MANUAL_RESOLV;
		}
#endif

		fp=fopen(AUTO_RESOLV,"r");
		if(fp)
			fclose(fp);
		else{
			fp=fopen(AUTO_RESOLV,"w");
			if(fp)
			{
				fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
				chmod(AUTO_RESOLV,0666);
#endif
			}
		}
		str=(char *)AUTO_RESOLV;
	}
#endif

	// create hosts file
	delete_dsldevice_on_hosts();
	add_dsldevice_on_hosts();

	#if 0
	//va_cmd(DNSRELAY, 4, 0, (char *)ARG_I, (char *)LANIF, "-r", str);
	if (va_cmd(DNSRELAY, 2, 0, "-r", str))
	    return -1;
	#endif

#ifdef OLD_DNS_MANUALLY
//star: for the dns restart
	unlink(RESOLV);

	if (symlink(str, RESOLV)) {
			printf("failed to link %s --> %s\n", str, RESOLV);
			return -1;
	}
#endif

#ifdef CONFIG_CU
	// fix nessus 10539 DNS Server Recursive Query Cache Poisoning Weakness
	//command: /bin/dnsmasq -c 0 -C /var/dnsmasq.conf -r /var/resolv.conf --log-facility=/dev/null
	if (va_niced_cmd(DNSRELAY, 7, 0, "-c", "0", "-C", DNSMASQ_CONF, "-r", RESOLV, "--log-facility=/dev/null"))
#else
	if (va_niced_cmd(DNSRELAY, 5, 0, "-C", DNSMASQ_CONF, "-r", RESOLV, "--log-facility=/dev/null"))
#endif
	    return -1;

	return 1;
}
#endif

#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
// write config and restart
int restart_dnsrelay(void)
{
	restart_dnsrelay_ex("all", 1); //delay dns proxy
	return 0;
}

int restart_dnsrelay_ex(char *ifname, int delay)
{
	char cmd[256];

	snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_restart_dnsrelay %s %d", ifname, delay);
	system(cmd);
	return 0;
}

int reload_dnsrelay(char *ifname)
{
	char cmd[256];

	snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_restart_dnsrelay %s 2", ifname);
	system(cmd);

	return 0;
}
#endif

#ifdef CONFIG_USER_DHCP_SERVER
int restart_dhcp(void)
{
	unsigned char value[32];
	unsigned int uInt = DHCP_LAN_NONE;
	int dhcpserverpid=0,dhcprelaypid=0;
	int tmp_status, status=0, i;

	dhcpserverpid = read_pid((char*)DHCPSERVERPID);
	dhcprelaypid = read_pid((char*)DHCPRELAYPID);


	//printf("\ndhcpserverpid=%d,dhcprelaypid=%d\n",dhcpserverpid,dhcprelaypid);

	if(dhcpserverpid > 0) kill(dhcpserverpid, SIGTERM);
	if(dhcprelaypid > 0) kill(dhcprelaypid, SIGTERM);

	if(dhcpserverpid > 0)
	{
		i = 0;
		while (kill(dhcpserverpid, 0) == 0 && i < 20) {
			usleep(100000); i++;
		}
		if(i >= 20 )
			kill(dhcpserverpid, SIGKILL);

		if(!access(DHCPSERVERPID, F_OK)) unlink(DHCPSERVERPID);
	}

	if(dhcprelaypid > 0)
	{
		i = 0;
		while (kill(dhcprelaypid, 0) == 0 && i < 20) {
			usleep(100000); i++;
		}
		if(i >= 20 )
			kill(dhcprelaypid, SIGKILL);

		if(!access(DHCPRELAYPID, F_OK)) unlink(DHCPRELAYPID);
	}

/*ping_zhang:20090319 END*/
	if(mib_get_s(MIB_DHCP_MODE, (void *)value, sizeof(value)) != 0)
	{
		uInt = (unsigned int)(*(unsigned char *)value);

		if(uInt != DHCPV4_LAN_SERVER)
		{
			FILE *fp;
			//star:clean the lease file
			if ((fp = fopen(DHCPD_LEASE, "w")) == NULL)
			{
				printf("Open file %s failed !\n", DHCPD_LEASE);
				return -1;
			}
			fprintf(fp, "\n");
			fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
			chmod(DHCPD_LEASE,0666);
#endif

		}

		if(uInt != DHCP_LAN_RELAY)
		{
			#ifdef REMOTE_ACCESS_CTL
			// For DHCP Relay. Open DHCP Relay Port for Incoming Packets.
			// iptables -D inacc -i ! br+ -p udp --sport 67 --dport 67 -j ACCEPT
			va_cmd_no_error(IPTABLES, 13, 1, (char *)FW_DEL, (char *)FW_INACC, "!", (char *)ARG_I, LANIF_ALL, "-p", "udp", (char *)FW_SPORT, (char *)PORT_DHCP, (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_ACCEPT);
			#endif
		}

		if(uInt == DHCPV4_LAN_SERVER)
		{
			tmp_status = setupDhcpd();
			if (tmp_status == 1)
			{
				//printf("\nrestart dhcpserver!\n");
#ifdef COMBINE_DHCPD_DHCRELAY
				status = va_niced_cmd(DHCPD, 2, 0, "-S", DHCPD_CONF);
#else
				status = va_niced_cmd(DHCPD, 1, 0, DHCPD_CONF);
#endif

				while(read_pid((char*)DHCPSERVERPID) < 0)
					usleep(250000);

#if defined(CONFIG_USER_DNSMASQ_DNSMASQ)
				restart_dnsrelay();
#endif
			}
			else if (tmp_status == -1)
				status = -1;
			return status;
		}
		else if(uInt == DHCPV4_LAN_RELAY)
		{
			startDhcpRelay();
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ)
			restart_dnsrelay();
#endif
			return status;
		}
		else
		{
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ)
			restart_dnsrelay();
#endif
			return 0;
		}
	}
	return -1;
}
#endif

//Ip interface
int restart_lanip(void)
{
	char vChar=0;
	unsigned char ipaddr[32]="",ipaddr2[32]="";
	unsigned char subnet[32]="",subnet2[32]="";
	unsigned char value[6];
	int vInt;
	int status=0;
	FILE * fp;
#if defined(CONFIG_IPV6)
	char ipaddr6[64] = {0};
#endif

#ifdef IP_PASSTHROUGH
	unsigned char ippt_addr[32]="";
	unsigned int ippt_itf;
	unsigned long myipaddr,hisipaddr;
	int ippt_flag = 0;
#endif

#ifdef IP_PASSTHROUGH
	if (mib_get_s(MIB_IPPT_ITF, (void *)&ippt_itf, sizeof(ippt_itf)) != 0) {
		if (ippt_itf != DUMMY_IFINDEX) {	// IP passthrough
			fp = fopen ("/tmp/PPPHalfBridge", "r");
			if (fp) {
				if(fread(&myipaddr, 4, 1, fp) != 1)
					printf("Reading error: %s %d\n", __func__, __LINE__);
				// Added by Mason Yu. Access internet fail.
				fclose(fp);
				myipaddr+=1;
				strncpy(ippt_addr, inet_ntoa(*((struct in_addr *)(&myipaddr))), 16);
				ippt_addr[15] = '\0';
				ippt_flag =1;
			}

		}
	}
#endif

#ifdef _CWMP_MIB_
	mib_get_s(CWMP_LAN_IPIFENABLE, (void *)&vChar, sizeof(vChar));
	if (vChar != 0)
#endif
	{
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

		vInt = rtk_lan_get_max_lan_mtu();
		snprintf(value, 6, "%d", vInt);
		// set LAN-side MRU

#ifdef CONFIG_SECONDARY_IP
		mib_get_s(MIB_ADSL_LAN_ENABLE_IP2, (void *)value, sizeof(value));
		if (value[0] == 1) {
			// ifconfig LANIF LAN_IP netmask LAN_SUBNET
			if (mib_get_s(MIB_ADSL_LAN_IP2, (void *)value, sizeof(value)) != 0)
			{
				strncpy(ipaddr2, inet_ntoa(*((struct in_addr *)value)), 16);
				ipaddr2[15] = '\0';
			}
			if (mib_get_s(MIB_ADSL_LAN_SUBNET2, (void *)value, sizeof(value)) != 0)
			{
				strncpy(subnet2, inet_ntoa(*((struct in_addr *)value)), 16);
				subnet2[15] = '\0';
			}
			snprintf(value, 6, "%d", vInt);

			if(!strncmp(ipaddr,ipaddr2,16))
			{
				status|=va_cmd(IFCONFIG, 2, 1, (char*)LAN_ALIAS, "down");
				status|=va_cmd(IFCONFIG, 6, 1, (char*)LANIF, ipaddr, "netmask", subnet, "mtu", value);
			}
			else
			{
				status|=va_cmd(IFCONFIG, 6, 1, (char*)LANIF, ipaddr, "netmask", subnet, "mtu", value);
				status|=va_cmd(IFCONFIG, 6, 1, (char*)LAN_ALIAS, ipaddr2, "netmask", subnet2, "mtu", value);
			}
		}
		else
		{
			snprintf(value, 6, "%d", vInt);
			status|=va_cmd(IFCONFIG, 6, 1, (char*)LANIF, ipaddr, "netmask", subnet, "mtu", value);
		}
#else
		status|=va_cmd(IFCONFIG, 6, 1, (char*)LANIF, ipaddr, "netmask", subnet, "mtu", value);
#endif


#if 0
		mib_get_s(MIB_ADSL_LAN_ENABLE_IP2, (void *)value, sizeof(value));
		if (value[0] == 1) {
			// ifconfig LANIF LAN_IP netmask LAN_SUBNET
			if (mib_get_s(MIB_ADSL_LAN_IP2, (void *)value, sizeof(value)) != 0)
			{
				strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
				ipaddr[15] = '\0';
			}
			if (mib_get_s(MIB_ADSL_LAN_SUBNET2, (void *)value, sizeof(value)) != 0)
			{
				strncpy(subnet, inet_ntoa(*((struct in_addr *)value)), 16);
				subnet[15] = '\0';
			}
			snprintf(value, 6, "%d", vInt);
			// set LAN-side MRU
			status|=va_cmd(IFCONFIG, 6, 1, (char*)LAN_ALIAS, ipaddr, "netmask", subnet, "mtu", value);
		}
#endif

#if defined(CONFIG_IPV6)
	if (mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)ipaddr6, sizeof(ipaddr6)) != 0)
	{
		char cmdBuf[100] = {0};
#if defined(WEB_HTTP_SERVER_BIND_IP)
		char accept_dad[8] = {0};
		// backup old value
		snprintf(cmdBuf, sizeof(cmdBuf), "cat /proc/sys/net/ipv6/conf/%s/accept_dad", LANIF);
		fp = popen(cmdBuf, "r");
		if (fp != NULL)
		{
			if (fgets(accept_dad, sizeof(accept_dad), fp) != 0)
			{
				char *p = NULL;
				p = strchr(accept_dad, '\n');
				if (p) {
					*p = '\0';
				}
			}
			pclose(fp);
		}

		if (strcmp(accept_dad, "0") != 0)
		{
			snprintf(cmdBuf, sizeof(cmdBuf), "echo 0 > /proc/sys/net/ipv6/conf/%s/accept_dad", LANIF);
			system(cmdBuf);
		}

		snprintf(cmdBuf, sizeof(cmdBuf), "%s/%d", ipaddr6, 64);
		va_cmd(IFCONFIG, 3, 1, LANIF, ARG_ADD, cmdBuf);

		if (strcmp(accept_dad, "0") != 0)
		{
			snprintf(cmdBuf, sizeof(cmdBuf), "echo %s > /proc/sys/net/ipv6/conf/%s/accept_dad", accept_dad, LANIF);
			system(cmdBuf);
		}
#endif
	}
#endif

#ifdef IP_PASSTHROUGH
               if(ippt_flag)
			   status|=va_cmd(IFCONFIG, 2, 1, (char*)LAN_IPPT,ippt_addr);
#endif

#ifdef CONFIG_USER_DHCP_SERVER
		status|=restart_dhcp();
#endif

// Trigger igmpproxy to update interface info.
#ifdef CONFIG_USER_IGMPPROXY
		vInt = read_pid("/var/run/igmp_pid");
		if (vInt >= 1) {
			// Kick to sync the multicast virtual interfaces
			kill(vInt, SIGUSR1);
		}
#endif
#ifdef CONFIG_CT_AWIFI_JITUAN_SMARTWIFI
		unsigned char functype=0;
		mib_get(AWIFI_PROVINCE_CODE, &functype);
		if(functype == AWIFI_ZJ){
			awifiAddFwRuleChain();
		}
#endif
#ifdef SMBD_BIND_IP
		stopSamba();
		sleep(1);
		startSamba();
#endif

#ifdef OSGIAPP_BIND_IP
		restartOsgiApp();
#endif

		// Kaohj -- Disconnect network connection when LAN IP changes
		usleep(250000);
		cmd_killproc(PID_SHIFT(PID_TELNETD));
		return status;
	}
	return -1;
}

//DHCP
#ifdef EMBED
int getDhcpClientVendorClass(char *leaseDB, int leaseCount, unsigned char *macAddress, char *vendorClass)
{
	struct dhcpOfferedAddrLease *p = NULL;
	int i;

	if(leaseDB == NULL || vendorClass == NULL || macAddress == NULL)
		return 0;

	for(i = 0 ; i < leaseCount ; i++)
	{
		p = (struct dhcpOfferedAddrLease *) (leaseDB + i * sizeof(struct dhcpOfferedAddrLease));
		if(!memcmp(macAddress, p->chaddr, 6))
		{
#ifdef GET_VENDOR_CLASS_ID
			memcpy(vendorClass, p->vendor_class_id, sizeof(p->vendor_class_id));
			return 1;
#endif
		}
	}

	return 0;
}

int getOneDhcpClient(char **ppStart, unsigned long *size, char *ip, char *mac, char *liveTime, uint32_t *active_time)
{
	struct dhcpOfferedAddrLease *entry;

	if (ppStart == NULL || *ppStart == NULL || *size < sizeof(struct dhcpOfferedAddrLease) )
		return -1;

	entry = ((struct dhcpOfferedAddrLease *)*ppStart);
	*ppStart = *ppStart + sizeof(struct dhcpOfferedAddrLease);
	*size = *size - sizeof(struct dhcpOfferedAddrLease);

	if (entry->expires == 0)
		return 0;
//star: conflict ip addr will not be displayed on web
	if(!memcmp(entry->chaddr, "\x00\x00\x00\x00\x00\x00", 6))
		return 0;

	strcpy(ip, inet_ntoa(*((struct in_addr *)&entry->yiaddr)));
	snprintf(mac, 20, "%02x:%02x:%02x:%02x:%02x:%02x",
			entry->chaddr[0], entry->chaddr[1], entry->chaddr[2],
			entry->chaddr[3], entry->chaddr[4], entry->chaddr[5]);

	snprintf(liveTime, 10, "%lu", (unsigned long)ntohl(entry->expires));
	*active_time = entry->active_time;

	return 1;
}

int getOneDhcpClientIncludeHostname(char **ppStart, unsigned long *size, char *ip, char *mac, char *liveTime, char *hostName)
{
	struct dhcpOfferedAddrLease *entry;

	if (ppStart == NULL
			|| *ppStart == NULL
			|| *size < sizeof(struct dhcpOfferedAddrLease) )
		return -1;

	entry = ((struct dhcpOfferedAddrLease *)*ppStart);
	*ppStart = *ppStart + sizeof(struct dhcpOfferedAddrLease);
	*size = *size - sizeof(struct dhcpOfferedAddrLease);

	if (entry->expires == 0)
		return 0;
	//star: conflict ip addr will not be displayed on web
	if(!memcmp(entry->chaddr, "\x00\x00\x00\x00\x00\x00", 6))
		return 0;

	strcpy(ip, inet_ntoa(*((struct in_addr *)&entry->yiaddr)));
	snprintf(mac, 20, "%02x:%02x:%02x:%02x:%02x:%02x",
			entry->chaddr[0], entry->chaddr[1], entry->chaddr[2],
			entry->chaddr[3], entry->chaddr[4], entry->chaddr[5]);

	snprintf(liveTime, 10, "%lu", (unsigned long)ntohl(entry->expires));
	sprintf(hostName, "%s", entry->hostName);

	return 1;
}

int getDhcpClientHostName(char *leaseDB, int leaseCount, unsigned char *macAddress, char *hostName)
{
	struct dhcpOfferedAddrLease *p = NULL;
	int i;

	if(leaseDB == NULL || hostName == NULL || macAddress == NULL)
		return 0;

	for(i = 0 ; i < leaseCount ; i++)
	{
		p = (struct dhcpOfferedAddrLease *) (leaseDB + i * sizeof(struct dhcpOfferedAddrLease));
		if(!memcmp(macAddress, p->chaddr, 6))
		{
			sprintf(hostName, "%s", p->hostName);
			return 1;
		}
	}

	return 0;
}

int getDhcpClientInfo(char *leaseDB, int leaseCount, unsigned char *macAddress, struct dhcpOfferedAddrLease **pClient)
{
	struct dhcpOfferedAddrLease *p = NULL;
	int i;

	if(leaseDB == NULL || pClient == NULL || macAddress == NULL)
		return 0;

	for(i = 0 ; i < leaseCount ; i++)
	{
		p = (struct dhcpOfferedAddrLease *) (leaseDB + i * sizeof(struct dhcpOfferedAddrLease));
		if(!memcmp(macAddress, p->chaddr, 6))
		{
			*pClient = p;
			return 1;
		}
	}

	return 0;
}

int getDhcpClientLeasesDB(char **pdb, unsigned long *pdbLen)
{
	int pid, len = -1;
	struct stat status;
	char *buffer = NULL;
	FILE *fp;
	int lockfd = -1;

	if(pdb == NULL) return -1;

	pid = read_pid(DHCPSERVERPID);
	if (pid > 0)	// renew lease file
		kill(pid, SIGUSR1);
	usleep(1000);

	if ((lockfd = lock_file_by_flock(DHCPSERVERPID, 0)) == -1)
	{
		printf("%s, the file have been locked\n", __FUNCTION__);
		goto err;
	}

	if (stat(DHCPD_LEASE, &status) < 0)goto err;
	len = status.st_size/sizeof(struct dhcpOfferedAddrLease);

	if(len > 0)
	{
		// read DHCP server lease file
		buffer = *pdb;
		buffer = (char*)realloc(buffer, status.st_size);
		if (buffer == NULL) goto err;

		fp = fopen(DHCPD_LEASE, "r");
		if (fp == NULL) goto err;
		fread(buffer, 1, status.st_size, fp);
		fclose(fp);

		if(pdbLen) *pdbLen=status.st_size;
		*pdb = buffer;
	}

	if(-1 != lockfd) unlock_file_by_flock(lockfd);
	return len;

err:
	if (buffer) free(buffer);
	if(-1 != lockfd) unlock_file_by_flock(lockfd);
	return -1;
}

#endif
/*
 * FUNCTION:GET_NETWORK_DNS
 *
 * INPUT:
 *	dns1:
 *		Fisrt DNS server to set.
 *	dns2:
 *		Second DNS server to set.
 *
 * RETURN:
 * 	the number of DNS server got.
 *	-1 -- Input error
 *  -2 -- INETERNET WAN not found
 *  -3 -- resolv file not found
 *  -4 -- other
 */
int get_network_dns(char *dns1, char *dns2)
{
	MIB_CE_ATM_VC_T entry;
	int total, i;
	struct sockaddr_in sa;	// used to check IP
	int got = 0;
	char buf[256] = {0};
	char ifname[IFNAMSIZ] = {0};

	if(dns1 == NULL || dns2 == NULL)
		return -1;

	/* Find INTERNET WAN */
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, &entry) == 0)
			continue;

		if(entry.enable && entry.applicationtype & X_CT_SRV_INTERNET && entry.cmode != CHANNEL_MODE_BRIDGE)
		{
			ifGetName(entry.ifIndex, ifname, sizeof(ifname));
			break;
		}
	}

	/* INTERNET WAN not found */
	if(i == total)
		return -2;

	/* Get config */
	switch(entry.dnsMode)
	{
	case REQUEST_DNS_NONE: //manual
	case DNS_SET_BY_API: // set by API
		{
			if(inet_ntop(AF_INET, &entry.v4dns1, buf, sizeof(buf)) != NULL)
			{
				strcpy(dns1, buf);
				got++;
			}

			if(inet_ntop(AF_INET, &entry.v4dns2, buf, sizeof(buf)) != NULL)
			{
				strcpy(dns2, buf);
				got++;
			}
		}
		break;
	case REQUEST_DNS: //auto
		{
			FILE *fresolv = NULL;
			char *p;

			if ((DHCP_T)entry.ipDhcp == DHCP_CLIENT)
				snprintf(buf, 64, "%s.%s", (char *)DNS_RESOLV, ifname);
			if (entry.cmode == CHANNEL_MODE_PPPOE || entry.cmode == CHANNEL_MODE_PPPOA)
				snprintf(buf, 64, "%s.%s", (char *)PPP_RESOLV, ifname);

			fresolv = fopen(buf, "r");
			if(fresolv == NULL)
				return -3;

			while(!feof(fresolv))
			{
				memset(buf,0,sizeof(buf));
				fgets(buf,sizeof(buf), fresolv);

				if((strlen(buf)==0))
					break;

				// Get DNS servers
				p = strchr(buf, '@');
				if (p)
				{
					if(got == 0)
					{
						memcpy(dns1, buf, p-buf);
						dns1[p-buf] = '\0';
						got++;
					}
					else if(got == 1)
					{
						memcpy(dns2, buf, p-buf);
						dns2[p-buf] = '\0';
						got++;
					}
				}

				if(got == 2)
					break;
			}
			fclose(fresolv);
		}
		break;
	default:
		return -4;
	}

	return got;
}

int set_dhcp_port_base_filter(int enable)
{
	int j;
	unsigned char value[32];
	unsigned int bitmap;

	if (!enable)
	{
		va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_DHCP_PORT_FILTER);
		return 1;
	}
	else
	{
		if(!mib_get_s( MIB_DHCP_PORT_FILTER,  (void *)value, sizeof(value)))
			return 0;

		bitmap = (*(unsigned int *)value);
		for(j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j)
		{
			if (bitmap & (0x1 << j))
			{
				#ifdef CONFIG_NETFILTER_XT_MATCH_PHYSDEV
				va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char *)FW_DHCP_PORT_FILTER, "-m", "physdev", "--physdev-in", (char *)ELANVIF[j], "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);
				#else
				// iptables -A dhcp_port_filter -i eth0.2 -p udp --dport 67 -j DROP
				va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_DHCP_PORT_FILTER, (char *)ARG_I, (char *)ELANVIF[j], "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);
				#endif
			}
		}
#ifdef TRIBAND_SUPPORT
#if defined(CONFIG_RTK_DEV_AP)
		for(j = PMAP_WLAN0; j <= PMAP_WLAN2_VAP3; ++j)
		{
				if (bitmap & (0x1 << j))
				{
					// iptables -A dhcp_port_filter -i wlan0 -p udp --dport 67 -j DROP
                    #if defined(_LINUX_3_18_) || defined(CONFIG_RTK_DEV_AP)
					va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char *)FW_DHCP_PORT_FILTER, "-m", "physdev", "--physdev-in", wlan[j - PMAP_WLAN0], "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);
                    #else
					va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_DHCP_PORT_FILTER, (char *)ARG_I, wlan[j - PMAP_WLAN0], "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);
                    #endif
				}
	    }
#endif
#elif WLAN_SUPPORT
		for(j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)
		{
			if (bitmap & (0x1 << j))
			{
				// iptables -A dhcp_port_filter -i wlan0 -p udp --dport 67 -j DROP
				#ifdef CONFIG_NETFILTER_XT_MATCH_PHYSDEV
				va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char *)FW_DHCP_PORT_FILTER, "-m", "physdev", "--physdev-in", wlan[j - PMAP_WLAN0], "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);
				#else
				va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_DHCP_PORT_FILTER, (char *)ARG_I, wlan[j - PMAP_WLAN0], "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);
				#endif
			}
		}
#endif
		return 1;
	}
}

void setup_ipforwarding(int enable)
{
	FILE *fp;
	int ipv6forward;
#ifdef RESTRICT_PROCESS_PERMISSION
	char sysbuf[128];
#endif

	if (enable != 0)
		enable = 1;
#ifdef RESTRICT_PROCESS_PERMISSION
	sprintf(sysbuf,"/bin/echo %d > /proc/sys/net/ipv4/ip_forward",enable);
	printf("[%s %d] system(): %s\n",__func__,__LINE__,sysbuf);
    	system(sysbuf);
#else
	fp = fopen(PROC_IPFORWARD, "w");
	if(fp)
	{
		fprintf(fp, "%d\n", enable);
		fclose(fp);
	}
#endif
#ifdef CONFIG_IPV6
	//  Because the /proc/.../conf/all/forwarding value, will affect /proc/.../conf/vc0/forwarding value.
	fp = fopen(PROC_IP6FORWARD, "r+");
	if(fp)
	{
		if(fscanf(fp,"%d",&ipv6forward) != 1)
			printf("fscanf failed: %s %d\n", __func__, __LINE__);
		if(ipv6forward !=enable)
			fprintf(fp, "%d\n", enable);
		fclose(fp);
	}
#endif
}

#if defined(CONFIG_RTL_SMUX_DEV)
int rtk_smuxdev_hw_vlan_filter_init(void)
{
	char cmd_str[1024];
	sprintf(cmd_str, "echo switch_on > /proc/rtk_smuxdev/hw_vlan_filter");
	AUG_PRT("%s\n", cmd_str);
	va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
	sprintf(cmd_str, "echo enable %s ALL > /proc/rtk_smuxdev/hw_vlan_filter", ALIASNAME_NAS0);
	AUG_PRT("%s\n", cmd_str);
	va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
	return 0;
}

#ifdef CONFIG_USER_LAN_VLAN_TRANSLATE
int Set_LanPort_Def(int port)
{
	int i = port, vid = 0;
	MIB_CE_SW_PORT_T swEntry;
	char rule_alias_name[64] = {0};
	char vlanlist[64] = {0};
	char *strData, *token;
	unsigned char tr247_unmatched_veip_cfg = 0;
	mib_get_s(MIB_TR247_UNMATCHED_VEIP_CFG, (void *)&tr247_unmatched_veip_cfg, sizeof(tr247_unmatched_veip_cfg));

	if(i >= ELANVIF_NUM || mib_chain_get(MIB_SW_PORT_TBL, i, (void *)&swEntry)<=0)		return -1;

	sprintf(rule_alias_name, "%s-rx-default",ELANRIF[i]);

	va_cmd(SMUXCTL, 7, 1, "--if", ELANRIF[i], "--rx", "--tags", "0", "--rule-remove-alias", rule_alias_name);
	va_cmd(SMUXCTL, 7, 1, "--if", ELANRIF[i], "--rx", "--tags", "1", "--rule-remove-alias", rule_alias_name);
	va_cmd(SMUXCTL, 7, 1, "--if", ELANRIF[i], "--rx", "--tags", "2", "--rule-remove-alias", rule_alias_name);

	if(swEntry.pb_mode != LAN_VLAN_BASED_MODE)
	{
		va_cmd(SMUXCTL, 10, 1, "--if", ELANRIF[i], "--rx", "--tags", "0", "--set-rxif", ELANVIF[i], "--rule-alias", rule_alias_name, "--rule-append");
#ifdef CONFIG_CMCC
		va_cmd(SMUXCTL, 13, 1, "--if", ELANRIF[i], "--rx", "--tags", "1", "--filter-vid", "0", "1", "--set-rxif", ELANVIF[i], "--rule-alias", rule_alias_name, "--rule-append");
#endif
		if(swEntry.pb_mode == LAN_TRANSPARENT_BASED_MODE || tr247_unmatched_veip_cfg == 1) {
			va_cmd(SMUXCTL, 10, 1, "--if", ELANRIF[i], "--rx", "--tags", "1", "--set-rxif", ELANVIF[i], "--rule-alias", rule_alias_name, "--rule-append");
			if(tr247_unmatched_veip_cfg == 1) {
				va_cmd(SMUXCTL, 10, 1, "--if", ELANRIF[i], "--rx", "--tags", "2", "--set-rxif", ELANVIF[i], "--rule-alias", rule_alias_name, "--rule-append");
			}
		}else if(swEntry.pb_mode == LAN_TRUNK_BASED_MODE){
			memcpy(vlanlist, swEntry.trunkVlanlist, sizeof(vlanlist)-1);
			token=strtok(vlanlist,",");

			while (token!=NULL) {
				sscanf(token, "%d", &vid);
				sprintf(rule_alias_name, "%s-rx-trunk-vid%d",ELANRIF[i], vid);
				va_cmd(SMUXCTL, 13,1, "--if", ELANRIF[i], "--rx", "--tags", "1", "--filter-vid", token, "1","--set-rxif", ELANVIF[i], "--rule-alias", rule_alias_name, "--rule-append");
				token=strtok(NULL,",");
				memset(rule_alias_name, 0x0, sizeof(rule_alias_name));
			}

		}
	}

	return 0;
}
#endif
int Init_LanPort_Def_Intf(int port)
{
	int i = port, j;
	MIB_CE_SW_PORT_T swEntry;
	char rule_alias_name[64] = {0};
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	char tags_num[32];
	char tags_increase_num[32];
	char rule_priority[32];
	char filter_vid[32];
	char skb_mark2_mask[64] = {0};
	char skb_mark2_value[64] = {0};
	int rgmii_idx;
#endif
	unsigned char tr247_unmatched_veip_cfg = 0;
#ifdef CONFIG_DEONET_DATAPATH
typedef struct {
	char filter_dscp_str[32];
	char action_pri_q_str[8];
	char rule_pri[4];
} add_rule_t;
	add_rule_t v4_add_rule[4] = {
		{ "0000000000000000", "0x00", "0"},
		{ "0000000100000000", "0x10", "1"},
		{ "0000000001000000", "0x20", "2"},
		{ "0000000000400000", "0x30", "3"}};
#endif

	mib_get_s(MIB_TR247_UNMATCHED_VEIP_CFG, (void *)&tr247_unmatched_veip_cfg, sizeof(tr247_unmatched_veip_cfg));
	if(i >= ELANVIF_NUM || mib_chain_get(MIB_SW_PORT_TBL, i, (void *)&swEntry)<=0)		return -1;

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	rgmii_idx = rtk_fc_external_switch_rgmii_port_get();
	if(rtk_port_check_external_port(i))
	{
		sprintf(rule_alias_name, "%s-rx-default", EXTELANRIF[i]);
		va_cmd(SMUXCTL, 4, 1, "--if-create-name", EXTELANRIF[i], ELANVIF[i], "--set-if-rsmux");
	}
	else
#endif
	{
#ifndef CONFIG_DEONET_DATAPATH
		sprintf(rule_alias_name, "%s-rx-default",ELANRIF[i]);
#endif
		va_cmd(SMUXCTL, 4, 1, "--if-create-name", ELANRIF[i], ELANVIF[i], "--set-if-rsmux");
		if(swEntry.pb_mode != LAN_VLAN_BASED_MODE)
		{
#ifdef CONFIG_DEONET_DATAPATH
			for (j=0; j<4; j++) {
					if (j == 0) {
						memset(rule_alias_name, 0, sizeof(rule_alias_name));
						snprintf(rule_alias_name, sizeof(rule_alias_name),
								"%s_RX_%d", ELANRIF[i], j);
						va_cmd(SMUXCTL, 17, 1, "--if", ELANRIF[i], "--rx", "--tags", "0",
								"--set-skb-mark", "0x70", v4_add_rule[j].action_pri_q_str,
								"--target", "ACCEPT",
								"--rule-priority", v4_add_rule[j].rule_pri,
								"--set-rxif", ELANVIF[i],
								"--rule-alias", rule_alias_name, "--rule-append");
					} else {
						memset(rule_alias_name, 0, sizeof(rule_alias_name));
						snprintf(rule_alias_name, sizeof(rule_alias_name),
								"%s_RX_%d", ELANRIF[i], j);
						va_cmd(SMUXCTL, 19, 1, "--if", ELANRIF[i], "--rx", "--tags", "0",
								"--filter-dscp-range", v4_add_rule[j].filter_dscp_str,
								"--set-skb-mark", "0x70", v4_add_rule[j].action_pri_q_str,
								"--target", "ACCEPT",
								"--rule-priority", v4_add_rule[j].rule_pri,
								"--set-rxif", ELANVIF[i],
								"--rule-alias", rule_alias_name, "--rule-append");
					}
			}
#else
			va_cmd(SMUXCTL, 10, 1, "--if", ELANRIF[i], "--rx", "--tags", "0", "--set-rxif", ELANVIF[i], "--rule-alias", rule_alias_name, "--rule-append");
#endif
#ifdef CONFIG_CMCC
			va_cmd(SMUXCTL, 13, 1, "--if", ELANRIF[i], "--rx", "--tags", "1", "--filter-vid", "0", "1", "--set-rxif", ELANVIF[i], "--rule-alias", rule_alias_name, "--rule-append");
#endif
			if(swEntry.pb_mode == LAN_TRANSPARENT_BASED_MODE || tr247_unmatched_veip_cfg == 1) {
				va_cmd(SMUXCTL, 10, 1, "--if", ELANRIF[i], "--rx", "--tags", "1", "--set-rxif", ELANVIF[i], "--rule-alias", rule_alias_name, "--rule-append");
				if(tr247_unmatched_veip_cfg == 1) {
					va_cmd(SMUXCTL, 10, 1, "--if", ELANRIF[i], "--rx", "--tags", "2", "--set-rxif", ELANVIF[i], "--rule-alias", rule_alias_name, "--rule-append");
				}
			}

			va_cmd(SMUXCTL, 15, 1,
					"--if", ELANRIF[i], "--tx", "--tags", "0",
					"--filter-ipl4proto", "2",
					"--set-skb-mark", "0x70", "0x30",
					"--set-dscp", "46",
					"--rule-priority", "100",
					"--rule-append");

			va_cmd(SMUXCTL, 17, 1,
					"--if", ELANRIF[i], "--tx", "--tags", "0",
					"--filter-ipl4proto", "17", "--filter-ipl4sport", "67",
					"--set-skb-mark", "0x70", "0x30",
					"--set-dscp", "46",
					"--rule-priority", "100",
					"--rule-append");
		}
	}

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	if(rtk_port_check_external_port(i)==1)
	{
		unsigned int pvid_start;
		char value[32], tpid[32];

		mib_get_s(MIB_EXTERNAL_SWITCH_PVID_START, (void *)value, sizeof(value));
		pvid_start = atoi(value);
		sprintf(filter_vid, "%d", pvid_start+(i-CONFIG_LAN_PORT_NUM));
		mib_get_s(MIB_EXTERNAL_SWITCH_TPID, (void *)tpid, sizeof(tpid));
		for(j=0 ; j<=SMUX_MAX_TAGS ; j++)
		{
			// Should be first hit rule in system
			sprintf(tags_num, "%d", j);
			sprintf(tags_increase_num, "%d", j+1);
			sprintf(rule_priority, "%d", SMUX_RULE_PRIO_RX_PHY);
			sprintf(rule_alias_name, "%s-external-switch-rx-mark-portid", ELANRIF[rgmii_idx]);
			sprintf(skb_mark2_mask, "0x%llx", (SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_MASK|SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_MASK));
			sprintf(skb_mark2_value, "0x%llx", (SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_MASK|((unsigned long long)rtk_external_port_get_lan_phyID(i)<<SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_START)));
			va_cmd(SMUXCTL, 19, 1, "--if", ELANRIF[rgmii_idx], "--rx", "--tags", tags_num, "--filter-tpid", tpid, "0", "--filter-vid", filter_vid, "0", "--set-skb-mark2", skb_mark2_mask, skb_mark2_value, "--rule-alias", rule_alias_name, "--rule-priority", rule_priority, "--rule-append");
			//sprintf(rule_alias_name, "%s-external-switch-rx-default-to-%s", EXTELANRIF[i], ELANVIF[i]);
			//va_cmd(SMUXCTL, 10, 1, "--if", EXTELANRIF[i], "--rx", "--tags", tags_num, "--set-rxif", ELANVIF[i], "--rule-alias", rule_alias_name, "--rule-append");
		}
		sprintf(rule_alias_name, "%s-external-switch-rx-default-to-%s", EXTELANRIF[i], ELANVIF[i]);
		va_cmd(SMUXCTL, 10, 1, "--if", EXTELANRIF[i], "--rx", "--tags", "0", "--set-rxif", ELANVIF[i], "--rule-alias", rule_alias_name, "--rule-append");
#ifdef CONFIG_CMCC
		va_cmd(SMUXCTL, 13, 1, "--if", EXTELANRIF[i], "--rx", "--tags", "1", "--filter-vid", "0", "1", "--set-rxif", ELANVIF[i], "--rule-alias", rule_alias_name, "--rule-append");
#endif
	}
#endif

	return 0;
}

int Deinit_LanPort_Def_Intf(int port)
{
	int i = port;
	MIB_CE_SW_PORT_T swEntry;
	if(i >= ELANVIF_NUM || mib_chain_get(MIB_SW_PORT_TBL, i, (void *)&swEntry)<=0)		return -1;
	va_cmd(IFCONFIG, 2, 1, ELANVIF[i], "down");
	va_cmd(SMUXCTL, 2, 1, "--if-delete", ELANVIF[i]);
	return 0;
}
#endif

#if defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_) || defined(CONFIG_USER_NEW_BR_FOR_BRWAN)
#include <libgen.h>
int config_port_to_grouping(const char *port, const char *brname)
{
	char buf[256] = {0}, path[256], *prv_brname = NULL;
	int retry=0;

	if(port == NULL || !getInFlags(port, NULL) ||
		(brname && *brname != 0 && !getInFlags(brname, NULL)))
		return -1;

	sprintf(path, "/sys/class/net/%s/brport/bridge", port);
	if(readlink(path, buf, sizeof(buf)) > 0 &&
		(prv_brname = basename(buf)) &&
		!getInFlags(prv_brname, NULL))
	{
		prv_brname = NULL;
	}

	if(prv_brname != NULL)
	{
		va_cmd(IFCONFIG, 2, 1, port, "down");
		va_cmd(BRCTL, 3, 1, "delif", prv_brname, port);
	}

	if(brname != NULL)
	{
		va_cmd(IFCONFIG, 2, 1, port, "down");
		va_cmd(BRCTL, 3, 1, "addif", brname, port);
		va_cmd(IFCONFIG, 2, 1, port, "up");
	}
	return 0;
}
#endif
#if defined(_SUPPORT_INTFGRPING_PROFILE_) || defined(_SUPPORT_L2BRIDGING_PROFILE_)
int setup_interface_grouping(void *pentry, int mode)
{
	char buf[256] = {0}, macaddr[16] = {0}, brname[32] = {0}, ifname[32] = {0};
	int i, j, total, idx=0;
	MIB_L2BRIDGE_GROUP_Tp pEntry = (MIB_L2BRIDGE_GROUP_Tp)pentry;
	MIB_CE_SW_PORT_T portEntry;
	MIB_CE_ATM_VC_T wanEntry;
	MIB_VLAN_GROUP_T vlanGrpEntry;
#if defined(WLAN_SUPPORT)
	MIB_CE_MBSSIB_T mbssEntry;
	int orig_wlan_idx;
#endif
#ifdef CONFIG_IPV6
	char wan_ifname[64] = {0};
	unsigned int wanconn = DUMMY_IFINDEX;
#endif
	if (mode == 0 && pEntry->groupnum != 0 && pEntry->groupnum == rtk_layer2bridging_get_lan_router_groupnum())
	{
		rtk_layer2bridging_set_lan_router_groupnum(0);
	}

	if (pEntry == NULL || get_grouping_ifname_by_groupnum(pEntry->groupnum, brname, sizeof(brname)) == NULL)
		return -1;

	if (mode)
	{
		if (getInFlags(brname, NULL))
			va_cmd(IFCONFIG, 2, 1, brname, "down");
		else
			va_cmd(BRCTL, 2, 1, "addbr", brname);
		// ifconfig br0 hw ether
		getMIB2Str(MIB_ELAN_MAC_ADDR, macaddr);
		if (macaddr[0])
			va_cmd(IFCONFIG, 4, 1, brname, "hw", "ether", macaddr);

#ifdef SUPPORT_BRIDGE_VLAN_FILTER_PORTBIND
		snprintf(buf, sizeof(buf), "echo %d > /sys/class/net/%s/bridge/vlan_filtering", (pEntry->vlan_filtering)?1:0, brname);
		va_cmd("/bin/sh", 2, 1, "-c", buf);
#endif

#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
		rtk_igmp_mld_snooping_module_init(brname, CONFIGALL, NULL);
#endif

		for (i = 0; i < ELANVIF_NUM; i++)
		{
			if (!mib_chain_get(MIB_SW_PORT_TBL, i, (void*)&portEntry))
				continue;
			if (portEntry.itfGroup == pEntry->groupnum)
			{
				config_port_to_grouping(ELANVIF[i], brname);
			}
		}

		total = mib_chain_total(MIB_VLAN_GROUP_TBL);
		for (i = 0 ; i < total ; i++)
		{
			if (!mib_chain_get(MIB_VLAN_GROUP_TBL, i, (void*)&vlanGrpEntry))
				continue;

			if (vlanGrpEntry.itfGroup == pEntry->groupnum)
			{
				setup_vlan_group(&vlanGrpEntry, 1, -1);
			}
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

				if (mbssEntry.itfGroup == pEntry->groupnum)
				{
					rtk_wlan_get_ifname(i, j, ifname);
					config_port_to_grouping(ifname, brname);
				}
			}
		}
		wlan_idx = orig_wlan_idx;
#endif

		total = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0 ; i < total ; i++)
		{
			if  (!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&wanEntry) ||
				wanEntry.itfGroupNum != pEntry->groupnum ||
				wanEntry.cmode != CHANNEL_MODE_BRIDGE ||
				!wanEntry.enable)
				continue;
			ifGetName(PHY_INTF(wanEntry.ifIndex), ifname, sizeof(ifname));
			config_port_to_grouping(ifname, brname);
		}

		rtk_layer2bridging_set_group_firewall_rule(pEntry->groupnum, 1);
		rtk_layer2bridging_set_group_isolation_firewall_rule(pEntry->groupnum, 1);

		if (pEntry->enable)
		{
#if defined(CONFIG_IPV6)
			setup_ipv6_br_interface(brname);
#endif
			va_cmd(IFCONFIG, 2, 1, brname, "up");
		}
	}
	else
	{
		if (!getInFlags(brname, NULL))
			return 0;

		va_cmd(IFCONFIG, 2, 1, brname, "down");

		for (i = 0; i < ELANVIF_NUM; i++)
		{
			if (!mib_chain_get(MIB_SW_PORT_TBL, i, (void*)&portEntry))
				continue;

			if (portEntry.itfGroup == pEntry->groupnum)
			{
				portEntry.itfGroup = 0;
				config_port_to_grouping(ELANVIF[i], BRIF);
				mib_chain_update(MIB_SW_PORT_TBL, (void *)&portEntry, i);
			}
		}

		total = mib_chain_total(MIB_VLAN_GROUP_TBL);
		for (i = 0 ; i < total ; i++)
		{
			if (!mib_chain_get(MIB_VLAN_GROUP_TBL, i, (void*)&vlanGrpEntry))
				continue;

			if (vlanGrpEntry.itfGroup == pEntry->groupnum)
			{
				vlanGrpEntry.itfGroup = 0;
				setup_vlan_group(&vlanGrpEntry, 1, -1);
				mib_chain_update(MIB_VLAN_GROUP_TBL, (void *)&vlanGrpEntry, i);
			}
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

				if (mbssEntry.itfGroup == pEntry->groupnum)
				{
					mbssEntry.itfGroup = 0;
					rtk_wlan_get_ifname(i, j, ifname);
					config_port_to_grouping(ifname, BRIF);
					mib_chain_update(MIB_MBSSIB_TBL, (void *)&mbssEntry, j);
				}
			}
		}
		wlan_idx = orig_wlan_idx;
#endif

		total = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0 ; i < total ; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&wanEntry) ||
				wanEntry.itfGroupNum != pEntry->groupnum ||
				wanEntry.cmode != CHANNEL_MODE_BRIDGE ||
				!wanEntry.enable)
				continue;
			wanEntry.itfGroupNum = 0;
			ifGetName(PHY_INTF(wanEntry.ifIndex), ifname, sizeof(ifname));
			config_port_to_grouping(ifname, BRIF);
			mib_chain_update(MIB_ATM_VC_TBL, (void *)&wanEntry, i);
		}

		rtk_layer2bridging_set_group_firewall_rule(pEntry->groupnum, 0);
		rtk_layer2bridging_set_group_isolation_firewall_rule(pEntry->groupnum, 0);

		va_cmd(BRCTL, 2, 1, "delbr", brname);
	}

#ifdef CONFIG_IPV6
	wanconn = get_pd_wan_delegated_mode_ifindex();
	if (wanconn == 0xFFFFFFFF) {
		printf("Error!! %s get WAN Delegated PREFIXINFO fail!", __FUNCTION__);
	}
	if (wanconn != DUMMY_IFINDEX && wanconn!= 0 && wanconn != 0xFFFFFFFF)
	{
		ifGetName(wanconn, wan_ifname, sizeof(wan_ifname));
		fprintf(stderr, "[%s:%d] do_ipv6_iapd wanconn=%d, ifname=%s\n", __FUNCTION__,__LINE__, wanconn, wan_ifname);
		va_cmd("/bin/sysconf", 5, 0, "send_unix_sock_message", SYSTEMD_USOCK_SERVER, "do_ipv6_iapd", wan_ifname, wan_ifname);
	}
#endif

	return 0;
}

int setup_vlan_group(void *pentry, int mode, int logPort) //mode=> del(0), add(1)
{
	int total, i, j, idx=0;
	char brname[IFNAMSIZ+1] = {0}, ifName[IFNAMSIZ+1] = {0}, *vif;
	const char *rif, *rrif;
	char alias_name[64] = {0}, cmd_str[256] = {0}, *pcmd;
	MIB_VLAN_GROUP_Tp pEntry = (MIB_VLAN_GROUP_Tp)pentry;
	MIB_CE_SW_PORT_T portEntry;
#if defined(WLAN_SUPPORT)
	MIB_CE_MBSSIB_T mbssEntry;
	int wlanIndex;
	int ssidIdx = -1;
#endif

	if(pEntry == NULL || pEntry->vid <= 0)
		return -1;
#if defined(CONFIG_RTL_SMUX_DEV)
	pcmd = alias_name;
	pcmd += sprintf(pcmd, "VLAN_GRP");
	if(pEntry->vid > 0)
		pcmd += sprintf(pcmd, "_v%d", pEntry->vid);
	else
		pcmd += sprintf(pcmd, "_vX");
	if(pEntry->vprio > 0)
		pcmd += sprintf(pcmd, "_p%d", pEntry->vprio);
	else
		pcmd += sprintf(pcmd, "_pX");
#endif

	if(get_grouping_ifname_by_groupnum(pEntry->itfGroup, brname, sizeof(brname)) == NULL || !getInFlags(brname, NULL))
		strcpy(brname, BRIF);

	for (i = 0; i < PMAP_ITF_END; i++)
	{
		if(logPort != -1 && i != logPort)
			continue;

		if(i < PMAP_WLAN0){
			if (!mib_chain_get(MIB_SW_PORT_TBL, i, (void*)&portEntry) ||
				!(pEntry->portMask & (1<<i)))
				continue;

#if defined(CONFIG_TR142_MODULE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
			if(mode && portEntry.omci_uni_portType == OMCI_PORT_TYPE_PPTP)
				continue;
#endif
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
			if(rtk_port_check_external_port(i)){
				rif = EXTELANRIF[i];
				rrif = ELANRIF[i];
			}
			else
#endif
			{
				rif = ELANRIF[i];
				rrif = ELANRIF[i];
			}
		}
#ifdef WLAN_SUPPORT
		else if((ssidIdx = (i-PMAP_WLAN0)) < (WLAN_SSID_NUM*NUM_WLAN_INTERFACE) && ssidIdx >= 0){
			if(i >=PMAP_WLAN0 && i <=PMAP_WLAN0_VAP_END){
				wlanIndex = 0;
				idx = i-PMAP_WLAN0;
			}
			else{
				wlanIndex = 1;
				idx = i-PMAP_WLAN1;
			}

			if (!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIndex, idx, (void *)&mbssEntry)||
				!(pEntry->portMask & (1<<i)))
				continue;

			rif = wlan[ssidIdx];
			rrif = wlan[ssidIdx];
		}
#endif

		vif = ifName;
		vif += sprintf(vif, "%s", rrif);
		if(pEntry->vid > 0)
			vif += sprintf(vif, ".%d", pEntry->vid);
		if(pEntry->vprio >= 0)
			vif += sprintf(vif, "_%d", pEntry->vprio);
		vif = ifName;

#if defined(CONFIG_RTL_SMUX_DEV)
		if(!getInFlags(vif, NULL))
		{
			if(mode)
			{
				sprintf(cmd_str, "%s --if-create-name %s %s", SMUXCTL, rif, vif);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
			}
			else
			{
				sprintf(cmd_str, "%s %s down", IFCONFIG, vif);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
			}

			for(j = 0; j <= 2; j++)
			{
#ifdef CONFIG_USER_LAN_VLAN_TRANSLATE
				if(pEntry->vid == 1)
				{
					pcmd = cmd_str;
					pcmd += sprintf(pcmd, "%s --if %s --rx --tags %d", SMUXCTL, rif, j);
					if(pEntry->vprio >= 0)
						pcmd += sprintf(pcmd, " --filter-priority %d %d", pEntry->vprio, j);
					pcmd += sprintf(pcmd, " --set-rxif %s --target ACCEPT --rule-alias %s", vif, alias_name);
					if(mode) pcmd += sprintf(pcmd, " --rule-priority %d --rule-insert", SMUX_RULE_PRIO_VLAN_GRP);
					else pcmd += sprintf(pcmd, " --rule-remove-alias %s+", alias_name);
					va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					break;
				}
				else
#endif
				{
					if(j > 0)
					{
						pcmd = cmd_str;
						pcmd += sprintf(pcmd, "%s --if %s --rx --tags %d", SMUXCTL, rif, j);
						if(pEntry->vid > 0)
							pcmd += sprintf(pcmd, " --filter-vid %d %d", pEntry->vid, j);
						if(pEntry->vprio >= 0)
							pcmd += sprintf(pcmd, " --filter-priority %d %d", pEntry->vprio, j);
						pcmd += sprintf(pcmd, " --set-rxif %s --target ACCEPT --rule-alias %s", vif, alias_name);
						if(mode) pcmd += sprintf(pcmd, " --rule-priority %d --rule-insert", SMUX_RULE_PRIO_VLAN_GRP);
						else pcmd += sprintf(pcmd, " --rule-remove-alias %s+", alias_name);
						va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					}

					if(pEntry->txTagging)
					{
						pcmd = cmd_str;
						pcmd += sprintf(pcmd, "%s --if %s --tx --tags %d --filter-txif %s", SMUXCTL, rif, j, vif);
						if(j == 0)
							pcmd += sprintf(pcmd, " --push-tag");
						if(pEntry->vid > 0)
							pcmd += sprintf(pcmd, " --set-vid %d %d", pEntry->vid, (j==0)?1:j);
						if(pEntry->vprio >= 0)
							pcmd += sprintf(pcmd, " --set-priority %d %d", pEntry->vprio, (j==0)?1:j);
						pcmd += sprintf(pcmd, " --rule-alias %s", alias_name);
						if(mode) sprintf(pcmd, " --rule-priority %d --rule-insert", SMUX_RULE_PRIO_VLAN_GRP);
						else pcmd += sprintf(pcmd, " --rule-remove-alias %s+", alias_name);
						va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
					}
				}
			}
		}

		if(mode){
			config_port_to_grouping(vif, brname);
		}
		else
		{
			sprintf(cmd_str, "%s --if-delete %s", SMUXCTL, vif);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
		}
#endif
	}

	return 0;
}

int reset_port_grouping_by_groupnum(int groupNum)
{
	int i, j, total, idx=0;
	MIB_CE_SW_PORT_T portEntry;
	MIB_CE_ATM_VC_T wanEntry;
	MIB_VLAN_GROUP_T vlanGrpEntry;
#if defined(WLAN_SUPPORT)
	MIB_CE_MBSSIB_T mbssEntry;
	int orig_wlan_idx;
#endif
	//total = mib_chain_total(MIB_SW_PORT_TBL);
	for (i = 0; i < ELANVIF_NUM; i++)
	{
		if(!mib_chain_get(MIB_SW_PORT_TBL, i, (void*)&portEntry) ||
			portEntry.itfGroup != groupNum)
			continue;

		portEntry.itfGroup = 0;
		mib_chain_update(MIB_SW_PORT_TBL, (void*)&portEntry, i);
	}

	total = mib_chain_total(MIB_VLAN_GROUP_TBL);
	for(i = 0 ; i < total ; i++)
	{
		if(!mib_chain_get(MIB_VLAN_GROUP_TBL, i, (void*)&vlanGrpEntry) ||
			vlanGrpEntry.itfGroup != groupNum)
			continue;

		vlanGrpEntry.itfGroup = 0;
		mib_chain_update(MIB_VLAN_GROUP_TBL, (void*)&vlanGrpEntry, i);
	}

#if defined(WLAN_SUPPORT)
	orig_wlan_idx = wlan_idx;
	for (i = 0; i < NUM_WLAN_INTERFACE; i++)
	{
		wlan_idx = i;
		for (j = 0; j <= WLAN_MBSSID_NUM; j++)
		{
			if (!mib_chain_get(MIB_MBSSIB_TBL, j, (void *)&mbssEntry) || mbssEntry.itfGroup != groupNum)
				continue;

			mbssEntry.itfGroup = 0;
			mib_chain_update(MIB_MBSSIB_TBL, (void *)&mbssEntry, j);
		}
	}
	wlan_idx = orig_wlan_idx;
#endif

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0 ; i < total ; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&wanEntry) ||
			wanEntry.itfGroupNum != groupNum)
			continue;

		wanEntry.itfGroupNum = 0;
		mib_chain_update(MIB_ATM_VC_TBL, (void*)&wanEntry, i);
	}

	return 0;
}

int  is_ifname_belong_interface_group(char *comp_brname)
{
	int i = 0, total = 0;
	MIB_L2BRIDGE_GROUP_T entry = {0};
	MIB_L2BRIDGE_GROUP_Tp pEntry = &entry;
	char brname[16] = {0};
	if (comp_brname == NULL) return -1;

	total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, (void *)pEntry))
			continue;
		if (pEntry->groupnum !=0)
		{
			snprintf(brname, sizeof(brname),"br%u", pEntry->groupnum);
			if (strcmp(comp_brname, (char *)brname) == 0) {
				return 0;
			}
		}
	}
	return -1;
}


int get_grouping_entry_by_groupnum(int groupNum, MIB_L2BRIDGE_GROUP_T *l2br_entry, char *brname, int len)
{
	int i = 0, total = 0;
	MIB_L2BRIDGE_GROUP_T entry = {0};
	MIB_L2BRIDGE_GROUP_Tp pEntry = &entry;

	if ((l2br_entry == NULL && brname == NULL) || (brname != NULL && len <= 1)) return -1;

	total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, (void *)pEntry))
			continue;

		if (groupNum == rtk_layer2bridging_get_lan_router_groupnum())
		{
			if(brname) strcpy(brname, BRIF);
			if(l2br_entry) memcpy((void*)l2br_entry, (void*)pEntry, sizeof(MIB_L2BRIDGE_GROUP_T));
			return 0;
		}
		else
		{
			if (groupNum == pEntry->groupnum)
			{
				if(brname) snprintf(brname, (len - 1), "br%u", pEntry->groupnum);
				if(l2br_entry) memcpy((void*)l2br_entry, (void*)pEntry, sizeof(MIB_L2BRIDGE_GROUP_T));
				return 0;
			}
		}
	}

	return -1;
}

char *get_grouping_ifname_by_groupnum(int groupNum, char *brname, int len)
{
	if(get_grouping_entry_by_groupnum(groupNum, NULL, brname, len) == 0)
		return brname;
	else
		return NULL;
}

int config_interface_grouping(int configMode, void *pentry)
{
	MIB_L2BRIDGE_GROUP_T entry = {0};
	int i, total;

	if(configMode == CONFIGALL)
	{
		total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
		for (i = 1; i < total; i++) //from 1 start, skip default bridge setting
		{
			if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, (void *)&entry))
				continue;
			setup_interface_grouping(&entry, 1);
		}
	}
	else if(pentry != NULL)
		setup_interface_grouping(pentry, 1);

	return 0;
}

#ifdef CONFIG_HYBRID_MODE
/*
 * RETURN VALUE: 1 - router  0 - bridge
 */
int rtk_pon_get_devmode(void)
{
	unsigned char deviceType;

	mib_get(MIB_DEVICE_TYPE, (void *)&deviceType);

	return !!deviceType;
}

void rtk_pon_change_onu_devmode(void)
{
	MIB_CE_ATM_VC_T wanEntry;
	unsigned int vcTotal, i;
	unsigned char deviceType, dhcpMode;
	unsigned int pon_mode=0;
	unsigned int currDevMode=0;	// 1 router  0 bridge
	unsigned int prevDevMode=0;

	prevDevMode = rtk_pon_get_devmode();

	mib_get(MIB_PON_MODE, (void *)&pon_mode);
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&wanEntry))
			continue;

		if (wanEntry.enable)
			break;
	}

	if (i < vcTotal)
		currDevMode = 1;

	if (currDevMode != prevDevMode)
	{
		deviceType = currDevMode;
		mib_set(MIB_DEVICE_TYPE, (void *)&deviceType);

		if (1 == currDevMode)
		{
			#if 0
			/* if wan exists, then work in router mode */
			if (GPON_MODE == pon_mode)
			{
				//omcicli set devmode router
				va_cmd("/bin/omcicli", 3, 1, "set", "devmode", "router");

#if defined(CONFIG_RTK_L34_ENABLE)
				if(rtk_rg_gpon_deActivate() != RT_ERR_OK)
					printf("%s-%d rtk_rg_gpon_deActivate error \n",__func__,__LINE__);
				rtk_rg_gpon_activate(RTK_GPONMAC_INIT_STATE_O1);
#else
				rtk_gpon_deActivate();
				rtk_gpon_activate(RTK_GPONMAC_INIT_STATE_O1);
#endif
			}
			else if (EPON_MODE == pon_mode)
			{
				va_cmd("/bin/oamcli", 3, 1, "set", "devmode", "router");

				va_cmd("/bin/oamcli", 3, 1, "trigger", "register", "0");
			}
			else
				return;
			#endif
			/* enable DHCPD */
			dhcpMode = DHCP_LAN_SERVER;
			mib_set( MIB_DHCP_MODE, (void *)&dhcpMode);
			restart_dhcp();
		}
		else
		{
			#if 0
			/* if no wan exists, then work in bridge mode */
			if (GPON_MODE == pon_mode)
			{
				//omcicli set devmode bridge
				va_cmd("/bin/omcicli", 3, 1, "set", "devmode", "bridge");
			}
			else if (EPON_MODE == pon_mode)
			{
				va_cmd("/bin/oamcli", 3, 1, "set", "devmode", "bridge");
			}
			else
				return;
			#endif
			/* disable DHCPD */
			dhcpMode = DHCP_LAN_NONE;
			mib_set( MIB_DHCP_MODE, (void *)&dhcpMode);
			restart_dhcp();
		}
		Commit();
		cmd_reboot();
	}

	if (EPON_MODE == pon_mode)
	{
		if (1 == currDevMode)
		{
			va_cmd("/bin/sh", 2, 1, "-c", "echo 1 > /proc/devmode");
		}
		else
		{
			va_cmd("/bin/sh", 2, 1, "-c", "echo 0 > /proc/devmode");
		}
	}
}
#endif

int config_vlan_group(int configMode, void *pentry, int action, int logPort)
{
	MIB_VLAN_GROUP_T entry = {0};
	int i, total;

	if(configMode == CONFIGALL)
	{
		total = mib_chain_total(MIB_VLAN_GROUP_TBL);
		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_VLAN_GROUP_TBL, i, (void *)&entry))
				continue;
			setup_vlan_group(&entry, action, logPort);
		}
	}
	else if(pentry != NULL)
		setup_vlan_group(pentry, action, logPort);

	return 0;
}
int clear_VLAN_grouping_by_port(int logPort)
{
	int i = 0,num = 0;
	MIB_VLAN_GROUP_T group_Entry;
	num = mib_chain_total( MIB_VLAN_GROUP_TBL );
	for (i=num-1;i>=0;i--) {
		if(mib_chain_get(MIB_VLAN_GROUP_TBL, i, (void*)&group_Entry) == 0)
			printf("\n %s %d\n", __func__, __LINE__);
		if(group_Entry.portMask && 1<<logPort){
			group_Entry.portMask &= ~(1<<logPort);
			if(group_Entry.portMask == 0)
			{
				mib_chain_delete(MIB_VLAN_GROUP_TBL,i);
			}
			else
			{
				mib_chain_update(MIB_VLAN_GROUP_TBL, (void *)&group_Entry,i);
			}
		}
	}
	return 0;
}

int clear_trunkSetting(int logPort)
{
	int i = logPort, vid = 0;
	MIB_CE_SW_PORT_T swEntry;
	char rule_alias_name[64] = {0};
	char vlanlist[64] = {0};
	char *strData, *token;

	if(i >= ELANVIF_NUM || mib_chain_get(MIB_SW_PORT_TBL, i, (void *)&swEntry)<=0)		return -1;

	memcpy(vlanlist, swEntry.trunkVlanlist, sizeof(vlanlist)-1);
	token=strtok(vlanlist,",");


	while (token!=NULL) {
		sscanf(token, "%d", &vid);
		sprintf(rule_alias_name, "%s-rx-trunk-vid%d",ELANRIF[i], vid);
		va_cmd(SMUXCTL, 7, 1, "--if", ELANRIF[i], "--rx", "--tags", "1", "--rule-remove-alias", rule_alias_name);
		token=strtok(NULL,",");
		memset(rule_alias_name, 0x0, sizeof(rule_alias_name));
	}


	return 0;
}

int rtk_layer2bridging_get_lan_router_groupnum(void)
{
	unsigned int i = 0, total = 0, instnum = 0;
	MIB_L2BRIDGE_FILTER_T *p, entry;
	p = &entry;

	total = mib_chain_total(MIB_L2BRIDGING_FILTER_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_FILTER_TBL, i, (void *)p))
			continue;

		if (p->ifdomain == INTFGRPING_DOMAIN_LANROUTERCONN)
		{
			if (p->groupnum == 0 || p->enable)
			{
				return p->groupnum;
			}
		}
	}

	return 0;
}

int rtk_layer2bridging_set_lan_router_groupnum(int groupnum)
{
	unsigned int i = 0, total = 0, instnum = 0;
	MIB_L2BRIDGE_FILTER_T *p, entry;
	p = &entry;

	total = mib_chain_total(MIB_L2BRIDGING_FILTER_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_FILTER_TBL, i, (void *)p))
			continue;

		if (p->ifdomain == INTFGRPING_DOMAIN_LANROUTERCONN && p->groupnum != groupnum)
		{
			p->groupnum = groupnum;
			mib_chain_update(MIB_L2BRIDGING_FILTER_TBL, (void *)p, i);
			break;
		}
	}

	return 0;
}

int rtk_layer2bridging_set_group_firewall_rule(int groupnum, int enable)
{
	int ret = 0, i = 0, j = 0, total = 0, wan_total = 0, wan_num = 0, first_wan_id = (-1), dgw_wan_id = (-1);
	unsigned int wan_mark, wan_mask;
	char brname[16] = {0}, br_chain_name[32] = {0}, str_wan_mark[64] = {0}, str_tblid[16] = {0};
	MIB_L2BRIDGE_GROUP_T *p, entry;
	p = &entry;
	MIB_CE_ATM_VC_T wanEntry;
	char ifName[IFNAMSIZ];

	total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, (void *)p))
			continue;

		if (!get_grouping_ifname_by_groupnum(p->groupnum, brname, sizeof(brname)))
			continue;

		snprintf(br_chain_name, sizeof(br_chain_name), "interface_grouping_%s", brname);

		if (p->groupnum == rtk_layer2bridging_get_lan_router_groupnum())
		{
			va_cmd(IPTABLES, 4, 1, (char *)ARG_T, "mangle", "-F", br_chain_name);
			va_cmd(IPTABLES, 8, 1, (char *)ARG_T, "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, (char *)ARG_I, brname, "-j", br_chain_name);
			va_cmd(IPTABLES, 4, 1, (char *)ARG_T, "mangle", "-X", br_chain_name);
		}
		else
		{
			if (groupnum == (-1) || p->groupnum == groupnum)
			{
				va_cmd(IPTABLES, 4, 1, (char *)ARG_T, "mangle", "-F", br_chain_name);
				va_cmd(IPTABLES, 8, 1, (char *)ARG_T, "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, (char *)ARG_I, brname, "-j", br_chain_name);
				va_cmd(IPTABLES, 4, 1, (char *)ARG_T, "mangle", "-X", br_chain_name);

				if (enable)
				{
					va_cmd(IPTABLES, 4, 1, (char *)ARG_T, "mangle", "-N", br_chain_name);
					va_cmd(IPTABLES, 8, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, (char *)ARG_I, brname, "-j", br_chain_name);

					first_wan_id = (-1);
					dgw_wan_id = (-1);
					wan_num = 0;
					ret = 0;
					wan_total = mib_chain_total(MIB_ATM_VC_TBL);
					for (j = 0; j < wan_total; j++)
					{
						if (!mib_chain_get(MIB_ATM_VC_TBL, j, (void *)&wanEntry))
							continue;

						if (wanEntry.itfGroupNum == p->groupnum && wanEntry.cmode != CHANNEL_MODE_BRIDGE)
						{
							if (first_wan_id == (-1))
							{
								first_wan_id = j;
							}

							if (isDefaultRouteWan(&wanEntry))
							{
								dgw_wan_id = j;
							}
							wan_num++;
						}
					}

					if (wan_num > 0)
					{

						if (dgw_wan_id >= 0)
						{
							if (mib_chain_get(MIB_ATM_VC_TBL, dgw_wan_id, (void *)&wanEntry))
							{
								ret = 1;
							}
						}
						else if (first_wan_id >= 0)
						{
							if (mib_chain_get(MIB_ATM_VC_TBL, first_wan_id, (void *)&wanEntry))
							{
								ret = 1;
							}
						}

						if (ret)
						{
							ifGetName(wanEntry.ifIndex, ifName, sizeof(ifName));
							if (!rtk_net_get_wan_policy_route_mark(ifName, &wan_mark, &wan_mask))
							{
								snprintf(str_wan_mark, sizeof(str_wan_mark), "0x%x/0x%x", wan_mark, wan_mask);
								va_cmd(IPTABLES, 8, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, br_chain_name, "-j", "MARK", "--set-mark", str_wan_mark);
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

int rtk_layer2bridging_set_group_isolation_firewall_rule(int groupnum, int enable)
{
	int i = 0, total = 0;
	char brname[32] = {0};
	MIB_L2BRIDGE_GROUP_T *p = NULL, entry = {0};

	p = &entry;

	total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, (void *)p))
			continue;

		if (!get_grouping_ifname_by_groupnum(p->groupnum, brname, sizeof(brname)))
			continue;

		//groupnum == -1 is for setupFirewall and we need to reset rule
		if ((p->groupnum == groupnum) || (groupnum == -1))
		{
			va_cmd(EBTABLES, 6, 1, (char *)FW_DEL, FW_ISOLATION_MAP, "--logical-out", brname, "-j", (char *)FW_ISOLATION_BRPORT);
			if (enable == 1)
			{
				if (p->isolation_enable == 1) //isolation
				{
					va_cmd(EBTABLES, 6, 1, (char *)FW_INSERT, FW_ISOLATION_MAP, "--logical-out", brname, "-j", (char *)FW_ISOLATION_BRPORT);
				}
				else // not isolation, do nothing
					;
			}
		}
	}
	return 0;
}

int rtk_layer2bridging_get_group_by_groupnum(MIB_L2BRIDGE_GROUP_T *pentry, int groupnum, unsigned int *chainid)
{
	int ret = 0;
	unsigned int i = 0, total = 0;

	if (!pentry || !chainid) return ret;

	total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, (void *)pentry))
			continue;

		if (pentry->groupnum == groupnum)
		{
			*chainid = i;
			ret = 1;
			break;
		}
	}

	return ret;
}

int rtk_layer2bridging_get_unused_groupnum()
{
	unsigned int ret = 0, i = 0, chainid = 0;
	MIB_L2BRIDGE_GROUP_T *p, entry;

	/* 0 is default group num */
	for (i = 1; i < IFGROUP_NUM; i++)
	{
		p = &entry;
		if (rtk_layer2bridging_get_group_by_groupnum(p, i, &chainid))
			continue;

		ret = i;
		break;
	}

	return ret;
}

unsigned int rtk_layer2bridging_get_new_group_instnum()
{
	unsigned int i = 0, total = 0, instnum = 0;
	MIB_L2BRIDGE_GROUP_T *p, entry;
	p = &entry;

	total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_GROUP_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, i, (void *)p))
			continue;

		if (p->instnum > instnum)
		{
			instnum = p->instnum;
		}
	}

	return (instnum + 1);
}

unsigned int rtk_layer2bridging_get_new_bridge_port_instnum(int groupnum)
{
	unsigned int i = 0, total = 0, instnum = 0;
	MIB_L2BRIDGE_PORT_T *p, entry;
	p = &entry;

	total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_PORT_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_PORT_TBL, i, (void *)p) || (p->groupnum != groupnum))
			continue;

		if (p->instnum > instnum)
		{
			instnum = p->instnum;
		}
	}

	return (instnum + 1);
}

unsigned int rtk_layer2bridging_get_new_bridge_vlan_instnum(int groupnum)
{
	unsigned int i = 0, total = 0, instnum = 0;
	MIB_L2BRIDGE_VLAN_T *p, entry;
	p = &entry;

	total = mib_chain_total(MIB_L2BRIDGING_BRIDGE_VLAN_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_BRIDGE_VLAN_TBL, i, (void *)p) || (p->groupnum != groupnum))
			continue;

		if (p->instnum > instnum)
		{
			instnum = p->instnum;
		}
	}

	return (instnum + 1);
}

unsigned int rtk_layer2bridging_get_new_filter_instnum()
{
	unsigned int i = 0, total = 0, instnum = 0;
	MIB_L2BRIDGE_FILTER_T *p, entry;
	p = &entry;

	total = mib_chain_total(MIB_L2BRIDGING_FILTER_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_FILTER_TBL, i, (void *)p))
			continue;

		if (p->instnum > instnum)
		{
			instnum = p->instnum;
		}
	}

	return (instnum + 1);
}

unsigned int rtk_layer2bridging_get_new_marking_instnum()
{
	unsigned int i = 0, total = 0, instnum = 0;
	MIB_L2BRIDGE_MARKING_T *p, entry;
	p = &entry;

	total = mib_chain_total(MIB_L2BRIDGING_MARKING_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_MARKING_TBL, i, (void *)p))
			continue;

		if (p->instnum > instnum)
		{
			instnum = p->instnum;
		}
	}

	return (instnum + 1);
}

unsigned int rtk_layer2bridging_get_new_availableinterface_instnum()
{
	unsigned int i = 0, total = 0, instnum = 0;
	MIB_L2BRIDGE_AVAILABLE_INTERFACE_T *p, entry;
	p = &entry;

	total = mib_chain_total(MIB_L2BRIDGING_AVAILABLEINTERFACE_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_AVAILABLEINTERFACE_TBL, i, (void *)p))
			continue;

		if (p->instnum > instnum)
		{
			instnum = p->instnum;
		}
	}

	return (instnum + 1);
}

int rtk_layer2bridging_get_interface_filter(unsigned int ifdomain, unsigned int ifid, int groupnum, MIB_L2BRIDGE_FILTER_T *pentry, unsigned int *chainid)
{
	int ret = 0;
	unsigned int i = 0, total = 0;

	if (!pentry || !chainid) return ret;

	total = mib_chain_total(MIB_L2BRIDGING_FILTER_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_FILTER_TBL, i, (void *)pentry))
			continue;

		if (pentry->ifdomain == ifdomain && pentry->ifid == ifid)
		{
			/* don't care groupnum if value is -1 */
			if (groupnum == (-1) || pentry->groupnum == groupnum)
			{
				*chainid = i;
				ret = 1;
				break;
			}
		}
	}

	return ret;
}

int rtk_layer2bridging_get_interface_marking(unsigned int ifdomain, unsigned int ifid, int groupnum, MIB_L2BRIDGE_MARKING_T *pentry, unsigned int *chainid)
{
	int ret = 0;
	unsigned int i = 0, total = 0;

	if (!pentry || !chainid) return ret;

	total = mib_chain_total(MIB_L2BRIDGING_MARKING_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_L2BRIDGING_MARKING_TBL, i, (void *)pentry))
			continue;

		if (pentry->ifdomain == ifdomain && pentry->ifid == ifid)
		{
			/* don't care groupnum if value is -1 */
			if (groupnum == (-1) || pentry->groupnum == groupnum)
			{
				*chainid = i;
				ret = 1;
				break;
			}
		}
	}

	return ret;
}

void rtk_layer2bridging_init_mib_table()
{
	unsigned int i = 0, j = 0, itfNum = 0, chainid = 0, need_update = 0;
	struct layer2bridging_availableinterface_info itfList[MAX_NUM_OF_ITFS];
	MIB_L2BRIDGE_GROUP_T l2br_group_entry;
	MIB_L2BRIDGE_FILTER_T l2br_filter_entry;
	MIB_L2BRIDGE_MARKING_T l2br_marking_entry;
	unsigned int show_domain = 0;
	show_domain = (INTFGRPING_DOMAIN_ELAN | INTFGRPING_DOMAIN_WLAN | INTFGRPING_DOMAIN_WANROUTERCONN);
#ifdef CONFIG_USER_LAN_VLAN_TRANSLATE
	show_domain |= (INTFGRPING_DOMAIN_ELAN_VLAN | INTFGRPING_DOMAIN_WLAN_VLAN);
#endif

	for (i = 0; i < IFGROUP_NUM; i++)
	{
		memset(itfList, 0, sizeof(itfList));
		itfNum = rtk_layer2bridging_get_availableinterface(itfList, MAX_NUM_OF_ITFS, show_domain, i);
		if (itfNum > 0)
		{
			if (rtk_layer2bridging_get_group_by_groupnum(&l2br_group_entry, i, &chainid) == 0)
			{
				memset(&l2br_group_entry, 0, sizeof(MIB_L2BRIDGE_GROUP_T));
				l2br_group_entry.enable = 1;
				if (i == 0)
				{
					l2br_group_entry.groupnum = 0;
					strcpy(l2br_group_entry.name, "Default");
				}
				else
				{
					l2br_group_entry.groupnum = rtk_layer2bridging_get_unused_groupnum();
					sprintf(l2br_group_entry.name, "Group_%u", l2br_group_entry.groupnum);
				}

				l2br_group_entry.vlanid = 0;
#if (defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE))
				l2br_group_entry.omci_configured = 0;
				l2br_group_entry.omci_bridgeId = 0;
#endif
				l2br_group_entry.instnum = rtk_layer2bridging_get_new_group_instnum();
				mib_chain_add(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, (void *)&l2br_group_entry);
				need_update = 1;
			}
			else
			{
#ifdef CONFIG_00R0 /* keep VLAN ID of default bridge group */
				if (i == 0 && l2br_group_entry.vlanid == 0)
				{
					int total = 0, default_vlanid = 0;
					MIB_CE_ATM_VC_T wanEntry;

					total = mib_chain_total(MIB_ATM_VC_TBL);
					for (j = 0; j < total; j++)
					{
						if (!mib_chain_get(MIB_ATM_VC_TBL, j, (void *)&wanEntry))
							continue;

						if (wanEntry.itfGroupNum == i && wanEntry.vlan == 1)
						{
							if (wanEntry.dgw == 1 || default_vlanid == 0)
							{
								default_vlanid = wanEntry.vid;
							}
						}
					}

					if (default_vlanid > 0)
					{
						l2br_group_entry.vlanid = default_vlanid;
						mib_chain_update(MIB_L2BRIDGING_BRIDGE_GROUP_TBL, (void *)&l2br_group_entry, chainid);
						need_update = 1;
					}
				}
#endif
			}

			for (j = 0; j < itfNum; j++)
			{
				if (rtk_layer2bridging_get_interface_filter(itfList[j].ifdomain, itfList[j].ifid, i, &l2br_filter_entry, &chainid) == 0 ||
					rtk_layer2bridging_get_interface_marking(itfList[j].ifdomain, itfList[j].ifid, i, &l2br_marking_entry, &chainid) == 0)
				{
					rtk_layer2bridging_set_interface_groupnum(itfList[j].ifdomain, itfList[j].ifid, i, 1);
					need_update = 1;
				}
			}

			if (i == 0)
			{
				if (rtk_layer2bridging_get_interface_filter(INTFGRPING_DOMAIN_LANROUTERCONN, SW_LAN_PORT_NUM, (-1), &l2br_filter_entry, &chainid) == 0)
				{
					rtk_layer2bridging_set_interface_groupnum(INTFGRPING_DOMAIN_LANROUTERCONN, SW_LAN_PORT_NUM, i, 1);
					need_update = 1;
				}
			}
		}
	}

	if (need_update)
	{
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
	}

	return;
}

/* action:
           0 -> delete
           1 -> add
           2 -> modify
*/
void rtk_layer2bridging_update_wan_interface_mib_table(int action, unsigned int old_ifIndex, unsigned int new_ifIndex)
{
	int i = 0, total = 0;
	unsigned int chainid = 0;
	MIB_L2BRIDGE_FILTER_T l2br_filter_entry;

	if (action == 1)
	{
		if (rtk_layer2bridging_get_interface_filter(INTFGRPING_DOMAIN_WANROUTERCONN, new_ifIndex, 0, &l2br_filter_entry, &chainid) == 0)
		{
			rtk_layer2bridging_set_interface_groupnum(INTFGRPING_DOMAIN_WANROUTERCONN, new_ifIndex, 0, 1);
		}
	}
	else
	{
		total = mib_chain_total(MIB_L2BRIDGING_FILTER_TBL);
		for (i = (total - 1); i >= 0; i--)
		{
			if (!mib_chain_get(MIB_L2BRIDGING_FILTER_TBL, i, (void *)&l2br_filter_entry))
				continue;

			if (l2br_filter_entry.ifdomain == INTFGRPING_DOMAIN_WANROUTERCONN)
			{
				if (l2br_filter_entry.ifid == old_ifIndex)
				{
					if (action == 0)
					{
						mib_chain_delete(MIB_L2BRIDGING_FILTER_TBL, i);
					}
					else if (action == 2 && l2br_filter_entry.ifid != new_ifIndex)
					{
						l2br_filter_entry.ifid = new_ifIndex;
						mib_chain_update(MIB_L2BRIDGING_FILTER_TBL, (void *)&l2br_filter_entry, i);
					}
				}
			}
		}
	}

	return;
}

int rtk_layer2bridging_update_group_wan_interface(int groupnum)
{
	int ret = 0, i = 0, total = 0, wan_num = 0, first_wan_id = (-1), dgw_wan_id = (-1);
	unsigned int chainid = 0;
	MIB_CE_ATM_VC_T wanEntry;
	MIB_L2BRIDGE_FILTER_T l2br_filter_entry;
	MIB_L2BRIDGE_MARKING_T l2br_marking_entry;

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&wanEntry))
			continue;

		if (wanEntry.itfGroupNum == groupnum)
		{
			if (first_wan_id == (-1))
			{
				first_wan_id = i;
			}

			if (isDefaultRouteWan(&wanEntry))
			{
				dgw_wan_id = i;
			}
			wan_num++;
		}
	}

	if (wan_num == 0)
	{
		total = mib_chain_total(MIB_L2BRIDGING_FILTER_TBL);
		for (i = (total - 1); i >= 0; i--)
		{
			if (!mib_chain_get(MIB_L2BRIDGING_FILTER_TBL, i, (void *)&l2br_filter_entry))
				continue;

			if (l2br_filter_entry.groupnum == groupnum && l2br_filter_entry.ifdomain == INTFGRPING_DOMAIN_WAN)
			{
				mib_chain_delete(MIB_L2BRIDGING_FILTER_TBL, i);
			}
		}

		total = mib_chain_total(MIB_L2BRIDGING_MARKING_TBL);
		for (i = (total - 1); i >= 0; i--)
		{
			if (!mib_chain_get(MIB_L2BRIDGING_MARKING_TBL, i, (void *)&l2br_marking_entry))
				continue;

			if (l2br_marking_entry.groupnum == groupnum && l2br_marking_entry.ifdomain == INTFGRPING_DOMAIN_WAN)
			{
				mib_chain_delete(MIB_L2BRIDGING_MARKING_TBL, i);
			}
		}
	}
	else
	{
		if (dgw_wan_id >= 0)
		{
			if (mib_chain_get(MIB_ATM_VC_TBL, dgw_wan_id, (void *)&wanEntry))
			{
				ret = 1;
			}
		}
		else if (first_wan_id >= 0)
		{
			if (mib_chain_get(MIB_ATM_VC_TBL, first_wan_id, (void *)&wanEntry))
			{
				ret = 1;
			}
		}

		if (ret)
		{
#ifdef _CWMP_MIB_
			/* Update WANConnectionDevice Filter/Marking entry of this groupnum */
			if (rtk_layer2bridging_get_interface_filter(INTFGRPING_DOMAIN_WAN, ((MEDIA_INDEX(wanEntry.ifIndex) << 8) + wanEntry.ConDevInstNum), groupnum, &l2br_filter_entry, &chainid))
			{
				l2br_filter_entry.enable = 1;
				l2br_filter_entry.VLANIDFilter = -1;
				l2br_filter_entry.AdmitOnlyVLANTagged = 0;
				mib_chain_update(MIB_L2BRIDGING_FILTER_TBL, (void *)&l2br_filter_entry, chainid);
			}
			else
			{
				memset(&l2br_filter_entry, 0, sizeof(MIB_L2BRIDGE_FILTER_T));
				l2br_filter_entry.enable = 1;
				l2br_filter_entry.groupnum = groupnum;
				l2br_filter_entry.ifdomain = INTFGRPING_DOMAIN_WAN;
				l2br_filter_entry.ifid = ((MEDIA_INDEX(wanEntry.ifIndex) << 8) + wanEntry.ConDevInstNum);
				l2br_filter_entry.VLANIDFilter = -1;
				l2br_filter_entry.AdmitOnlyVLANTagged = 0;
				l2br_filter_entry.instnum = rtk_layer2bridging_get_new_filter_instnum();
				mib_chain_add(MIB_L2BRIDGING_FILTER_TBL, (void *)&l2br_filter_entry);
			}

			if (rtk_layer2bridging_get_interface_marking(INTFGRPING_DOMAIN_WAN, ((MEDIA_INDEX(wanEntry.ifIndex) << 8) + wanEntry.ConDevInstNum), groupnum, &l2br_marking_entry, &chainid))
			{
				l2br_marking_entry.enable = 1;
				l2br_marking_entry.vid = wanEntry.vid;
				l2br_marking_entry.vprio = (wanEntry.vprio - 1);
				l2br_marking_entry.VLANIDUntag = 0;
				l2br_marking_entry.VLANIDMarkOverride = 1;
				l2br_marking_entry.EthernetPriorityOverride = 1;
				mib_chain_update(MIB_L2BRIDGING_MARKING_TBL, (void *)&l2br_marking_entry, chainid);
			}
			else
			{
				memset(&l2br_marking_entry, 0, sizeof(MIB_L2BRIDGE_MARKING_T));
				l2br_marking_entry.enable = 1;
				l2br_marking_entry.groupnum = groupnum;
				l2br_marking_entry.ifdomain = INTFGRPING_DOMAIN_WAN;
				l2br_marking_entry.ifid = ((MEDIA_INDEX(wanEntry.ifIndex) << 8) + wanEntry.ConDevInstNum);
				l2br_marking_entry.vid = wanEntry.vid;
				l2br_marking_entry.vprio = (wanEntry.vprio - 1);
				l2br_marking_entry.VLANIDUntag = 0;
				l2br_marking_entry.VLANIDMarkOverride = 1;
				l2br_marking_entry.EthernetPriorityOverride = 1;
				l2br_marking_entry.instnum = rtk_layer2bridging_get_new_marking_instnum();
				mib_chain_add(MIB_L2BRIDGING_MARKING_TBL, (void *)&l2br_marking_entry);
			}
#endif
		}
	}

	return 0;
}

int rtk_layer2bridging_get_interface_groupnum(unsigned int ifdomain, unsigned int ifid, int *groupnum)
{
	int ret = 0, i = 0, total = 0;
	unsigned int chainid = 0;
	MIB_CE_SW_PORT_T portEntry;
	MIB_VLAN_GROUP_T vlangroupEntry;
	MIB_CE_ATM_VC_T wanEntry;
#if defined(WLAN_SUPPORT)
	MIB_CE_MBSSIB_T mbssEntry;
	int orig_wlan_idx;
#endif

	if (ifdomain == INTFGRPING_DOMAIN_ELAN)
	{
		if (mib_chain_get(MIB_SW_PORT_TBL, ifid, (void *)&portEntry))
		{
			*groupnum = portEntry.itfGroup;
			ret = 1;
		}
	}
	else if (ifdomain == INTFGRPING_DOMAIN_ELAN_VLAN || ifdomain == INTFGRPING_DOMAIN_WLAN_VLAN)
	{
		if (mib_chain_get(MIB_VLAN_GROUP_TBL, ifid, (void *)&vlangroupEntry))
		{
			*groupnum = vlangroupEntry.itfGroup;
			ret = 1;
		}
	}
	else if (ifdomain == INTFGRPING_DOMAIN_WLAN)
	{
#if defined(WLAN_SUPPORT)
		orig_wlan_idx = wlan_idx;
		chainid = ifid;
		if (chainid >= (WLAN_MBSSID_NUM + 1))
		{
			wlan_idx = 1;
			chainid -= (WLAN_MBSSID_NUM + 1);
		}
		else
		{
			wlan_idx = 0;
		}

		if (mib_chain_get(MIB_MBSSIB_TBL, chainid, (void *)&mbssEntry))
		{
			*groupnum = mbssEntry.itfGroup;
			ret = 1;
		}
		wlan_idx = orig_wlan_idx;
#endif
	}
	else if (ifdomain == INTFGRPING_DOMAIN_WANROUTERCONN)
	{
		total = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&wanEntry))
				continue;

			if (wanEntry.ifIndex == ifid)
			{
				*groupnum = wanEntry.itfGroupNum;
				ret = 1;
				break;
			}
		}
	}

	return ret;
}

int rtk_layer2bridging_set_interface_groupnum(unsigned int ifdomain, unsigned int ifid, int groupnum, int cwmp_entry_update)
{
	int ret = 0, i = 0, total = 0, old_groupnum = (-1);
	unsigned int chainid = 0;
	unsigned int filter_domain = (INTFGRPING_DOMAIN_ELAN | INTFGRPING_DOMAIN_ELAN_VLAN | INTFGRPING_DOMAIN_WLAN | INTFGRPING_DOMAIN_WLAN_VLAN | INTFGRPING_DOMAIN_WAN | INTFGRPING_DOMAIN_LANROUTERCONN | INTFGRPING_DOMAIN_WANROUTERCONN);
	unsigned int marking_domain = (INTFGRPING_DOMAIN_ELAN | INTFGRPING_DOMAIN_ELAN_VLAN | INTFGRPING_DOMAIN_WLAN | INTFGRPING_DOMAIN_WLAN_VLAN | INTFGRPING_DOMAIN_WAN);
	MIB_CE_SW_PORT_T portEntry;
	MIB_VLAN_GROUP_T groupEntry;
	MIB_CE_ATM_VC_T wanEntry;
#if defined(WLAN_SUPPORT)
	MIB_CE_MBSSIB_T mbssEntry;
	int orig_wlan_idx;
#endif

	MIB_L2BRIDGE_FILTER_T l2br_filter_entry;
	MIB_L2BRIDGE_MARKING_T l2br_marking_entry;

	/* create a new entry */
	if (cwmp_entry_update)
	{
		if (ifdomain & filter_domain)
		{
			if (rtk_layer2bridging_get_interface_filter(ifdomain, ifid, (-1), &l2br_filter_entry, &chainid) == 0)
			{
				memset(&l2br_filter_entry, 0, sizeof(MIB_L2BRIDGE_FILTER_T));
				l2br_filter_entry.enable = 1;
				l2br_filter_entry.groupnum = groupnum;
				l2br_filter_entry.ifdomain = ifdomain;
				l2br_filter_entry.ifid = ifid;
				l2br_filter_entry.VLANIDFilter = -1;
				l2br_filter_entry.AdmitOnlyVLANTagged = 0;
				l2br_filter_entry.instnum = rtk_layer2bridging_get_new_filter_instnum();
				mib_chain_add(MIB_L2BRIDGING_FILTER_TBL, (void *)&l2br_filter_entry);
			}
		}

		if (ifdomain & marking_domain)
		{
			if (rtk_layer2bridging_get_interface_marking(ifdomain, ifid, (-1), &l2br_marking_entry, &chainid) == 0)
			{
				memset(&l2br_marking_entry, 0, sizeof(MIB_L2BRIDGE_MARKING_T));
				l2br_marking_entry.enable = 1;
				l2br_marking_entry.groupnum = groupnum;
				l2br_marking_entry.ifdomain = ifdomain;
				l2br_marking_entry.ifid = ifid;
				l2br_marking_entry.vid = -1;
				l2br_marking_entry.vprio = -1;
				l2br_marking_entry.VLANIDUntag = 1;
				l2br_marking_entry.VLANIDMarkOverride = 0;
				l2br_marking_entry.EthernetPriorityOverride = 0;
				l2br_marking_entry.instnum = rtk_layer2bridging_get_new_marking_instnum();
				mib_chain_add(MIB_L2BRIDGING_MARKING_TBL, (void *)&l2br_marking_entry);
			}
		}
	}

	if (ifdomain == INTFGRPING_DOMAIN_ELAN)
	{
		if (mib_chain_get(MIB_SW_PORT_TBL, ifid, (void *)&portEntry))
		{
			if (portEntry.itfGroup != groupnum)
			{
				portEntry.itfGroup = groupnum;
				mib_chain_update(MIB_SW_PORT_TBL, (void *)&portEntry, ifid);
			}

			if (cwmp_entry_update)
			{
				if (rtk_layer2bridging_get_interface_filter(ifdomain, ifid, (-1), &l2br_filter_entry, &chainid))
				{
					l2br_filter_entry.enable = 1;
					l2br_filter_entry.groupnum = groupnum;
					l2br_filter_entry.VLANIDFilter = -1;
					l2br_filter_entry.AdmitOnlyVLANTagged = 0;
					mib_chain_update(MIB_L2BRIDGING_FILTER_TBL, (void *)&l2br_filter_entry, chainid);
				}

				if (rtk_layer2bridging_get_interface_marking(ifdomain, ifid, (-1), &l2br_marking_entry, &chainid))
				{
					l2br_marking_entry.enable = 1;
					l2br_marking_entry.groupnum = groupnum;
					l2br_marking_entry.vid = -1;
					l2br_marking_entry.vprio = -1;
					l2br_marking_entry.VLANIDUntag = 1;
					l2br_marking_entry.VLANIDMarkOverride = 0;
					l2br_marking_entry.EthernetPriorityOverride = 0;
					mib_chain_update(MIB_L2BRIDGING_MARKING_TBL, (void *)&l2br_marking_entry, chainid);
				}
			}
		}
	}
	else if (ifdomain == INTFGRPING_DOMAIN_ELAN_VLAN || ifdomain == INTFGRPING_DOMAIN_WLAN_VLAN)
	{
		if (mib_chain_get(MIB_VLAN_GROUP_TBL, ifid, (void *)&groupEntry))
		{
			if (groupEntry.itfGroup != groupnum)
			{
				groupEntry.itfGroup = groupnum;
				mib_chain_update(MIB_VLAN_GROUP_TBL, (void *)&groupEntry, ifid);
			}

			if (cwmp_entry_update)
			{
				if (rtk_layer2bridging_get_interface_filter(ifdomain, ifid, (-1), &l2br_filter_entry, &chainid))
				{
					l2br_filter_entry.enable = 1;
					l2br_filter_entry.groupnum = groupnum;
					l2br_filter_entry.VLANIDFilter = groupEntry.vid;
					l2br_filter_entry.AdmitOnlyVLANTagged = 1;
					mib_chain_update(MIB_L2BRIDGING_FILTER_TBL, (void *)&l2br_filter_entry, chainid);
				}

				if (rtk_layer2bridging_get_interface_marking(ifdomain, ifid, (-1), &l2br_marking_entry, &chainid))
				{
					l2br_marking_entry.enable = 1;
					l2br_marking_entry.groupnum = groupnum;
					if (groupEntry.txTagging)
					{
						l2br_marking_entry.vid = groupEntry.vid;
						l2br_marking_entry.vprio = groupEntry.vprio;
						l2br_marking_entry.VLANIDUntag = 0;
						l2br_marking_entry.VLANIDMarkOverride = 1;
						l2br_marking_entry.EthernetPriorityOverride = 1;
					}
					else
					{
						l2br_marking_entry.vid = -1;
						l2br_marking_entry.vprio = -1;
						l2br_marking_entry.VLANIDUntag = 1;
						l2br_marking_entry.VLANIDMarkOverride = 0;
						l2br_marking_entry.EthernetPriorityOverride = 0;
					}
					mib_chain_update(MIB_L2BRIDGING_MARKING_TBL, (void *)&l2br_marking_entry, chainid);
				}
			}
		}
	}
	else if (ifdomain == INTFGRPING_DOMAIN_WLAN)
	{
#if defined(WLAN_SUPPORT)
		orig_wlan_idx = wlan_idx;

		chainid = ifid;
		if (chainid >= (WLAN_MBSSID_NUM + 1))
		{
			wlan_idx = 1;
			chainid -= (WLAN_MBSSID_NUM + 1);
		}
		else
		{
			wlan_idx = 0;
		}

		if (mib_chain_get(MIB_MBSSIB_TBL, chainid, (void *)&mbssEntry))
		{
			if (mbssEntry.itfGroup != groupnum)
			{
				mbssEntry.itfGroup = groupnum;
				mib_chain_update(MIB_MBSSIB_TBL, (void *)&mbssEntry, chainid);
			}

			if (cwmp_entry_update)
			{
				if (rtk_layer2bridging_get_interface_filter(ifdomain, ifid, (-1), &l2br_filter_entry, &chainid))
				{
					l2br_filter_entry.enable = 1;
					l2br_filter_entry.groupnum = groupnum;
					l2br_filter_entry.VLANIDFilter = -1;
					l2br_filter_entry.AdmitOnlyVLANTagged = 0;
					mib_chain_update(MIB_L2BRIDGING_FILTER_TBL, (void *)&l2br_filter_entry, chainid);
				}

				if (rtk_layer2bridging_get_interface_marking(ifdomain, ifid, (-1), &l2br_marking_entry, &chainid))
				{
					l2br_marking_entry.enable = 1;
					l2br_marking_entry.groupnum = groupnum;
					l2br_marking_entry.vid = -1;
					l2br_marking_entry.vprio = -1;
					l2br_marking_entry.VLANIDUntag = 1;
					l2br_marking_entry.VLANIDMarkOverride = 0;
					l2br_marking_entry.EthernetPriorityOverride = 0;
					mib_chain_update(MIB_L2BRIDGING_MARKING_TBL, (void *)&l2br_marking_entry, chainid);
				}
			}
		}

		wlan_idx = orig_wlan_idx;
#endif
	}
	else if (ifdomain == INTFGRPING_DOMAIN_LANROUTERCONN)
	{
		if (cwmp_entry_update)
		{
			if (rtk_layer2bridging_get_interface_filter(ifdomain, ifid, (-1), &l2br_filter_entry, &chainid))
			{
				l2br_filter_entry.enable = 1;
				l2br_filter_entry.groupnum = groupnum;
				l2br_filter_entry.VLANIDFilter = -1;
				l2br_filter_entry.AdmitOnlyVLANTagged = 0;
				mib_chain_update(MIB_L2BRIDGING_FILTER_TBL, (void *)&l2br_filter_entry, chainid);
			}
		}
	}
	else if (ifdomain == INTFGRPING_DOMAIN_WANROUTERCONN)
	{
		total = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&wanEntry))
				continue;

			if (wanEntry.ifIndex == ifid)
			{
				if (wanEntry.itfGroupNum != groupnum)
				{
					old_groupnum = wanEntry.itfGroupNum;
					wanEntry.itfGroupNum = groupnum;
					mib_chain_update(MIB_ATM_VC_TBL, (void *)&wanEntry, i);
				}

				if (cwmp_entry_update)
				{
					/* Update WANConnectionDevice.{i}.WAN(IP/PPP)Connection.{i} Filter entry */
					if (rtk_layer2bridging_get_interface_filter(ifdomain, ifid, (-1), &l2br_filter_entry, &chainid))
					{
						l2br_filter_entry.enable = 1;
						l2br_filter_entry.groupnum = groupnum;
						l2br_filter_entry.VLANIDFilter = -1;
						l2br_filter_entry.AdmitOnlyVLANTagged = 0;
						mib_chain_update(MIB_L2BRIDGING_FILTER_TBL, (void *)&l2br_filter_entry, chainid);
					}

					/* Update WANConnectionDevice Filter/Marking entry */
					if (old_groupnum >= 0)
					{
						rtk_layer2bridging_update_group_wan_interface(old_groupnum);
					}
					rtk_layer2bridging_update_group_wan_interface(groupnum);
				}

				break;
			}
		}
	}

	return ret;
}

int rtk_layer2bridging_get_availableinterface(struct layer2bridging_availableinterface_info *info, int max_size, unsigned int ifdomain, int groupnum)
{
	int i = 0, j = 0, k=0, total = 0, count = 0, idx=0;
	int conninstBit = 0, mediaBit = 0;
	char *pName = NULL;
	MIB_CE_SW_PORT_T portEntry;
	MIB_VLAN_GROUP_T groupEntry;
	MIB_CE_ATM_VC_T wanEntry;
	char buff[64] = {0};
#if defined(WLAN_SUPPORT)
	MIB_CE_MBSSIB_T mbssEntry;
	int orig_wlan_idx;
#endif
	int ethPhyPortId = rtk_port_get_wan_phyID();
	int phyPortId=-1;

	count = 0;

	/* LAN Interface */
	for (i = 0; i < ELANVIF_NUM; i++)
	{
		idx = i;
		phyPortId = rtk_port_get_lan_phyID(i);
		if (phyPortId != -1 && phyPortId == ethPhyPortId)
			continue;

		if (!mib_chain_get(MIB_SW_PORT_TBL, i, (void *)&portEntry))
			continue;

		if (ifdomain & INTFGRPING_DOMAIN_ELAN)
		{
			if (groupnum == (-1) || groupnum == portEntry.itfGroup)
			{
				info[count].ifdomain = INTFGRPING_DOMAIN_ELAN;
				info[count].ifid = idx;
				sprintf(info[count].name, "LAN%d", (i + 1));
				info[count].itfGroup = portEntry.itfGroup;
				if (++count >= max_size)
					return count;
			}
		}

		/* LAN VLAN Interface */
		total = mib_chain_total(MIB_VLAN_GROUP_TBL);
		for (j = 0; j < total; j++)
		{
			if (!mib_chain_get(MIB_VLAN_GROUP_TBL, j, (void *)&groupEntry))
				continue;

			if (groupEntry.portMask & (1 << idx))
			{
				if (groupEntry.vid > 1)
				{
					if (ifdomain & INTFGRPING_DOMAIN_ELAN_VLAN)
					{
						if (groupnum == (-1) || groupnum == groupEntry.itfGroup)
						{
							info[count].ifdomain = INTFGRPING_DOMAIN_ELAN_VLAN;
							info[count].ifid = j;
							pName = info[count].name;
							pName += sprintf(pName, "LAN%d_%d", (i + 1), groupEntry.vid);
							if (groupEntry.vprio >= 0)
							{
								pName += sprintf(pName, "_%d", groupEntry.vprio);
							}

							info[count].itfGroup = groupEntry.itfGroup;
							if (++count >= max_size)
								return count;
						}
					}
				}
			}
		}
	}

	/* InternetGatewayDevice.LANDevice.1.LANHostConfigManagement.IPInterface.1 */
	if (SW_LAN_PORT_NUM > 0)
	{
		if (ifdomain & INTFGRPING_DOMAIN_LANROUTERCONN)
		{
			info[count].ifdomain = INTFGRPING_DOMAIN_LANROUTERCONN;
			info[count].ifid = SW_LAN_PORT_NUM;
			sprintf(info[count].name, "LANIPInterface");
			info[count].itfGroup = rtk_layer2bridging_get_lan_router_groupnum();
			if (++count >= max_size)
				return count;
		}
	}

	/* WLAN Interface */
#if defined(WLAN_SUPPORT)
	orig_wlan_idx = wlan_idx;
	for (i = 0; i < NUM_WLAN_INTERFACE; i++)
	{
		wlan_idx = i;

		total = WLAN_MBSSID_NUM;
		for (j = 0; j <= total; j++)
		{
#if defined(CONFIG_00R0) && defined(CONFIG_USER_RTK_MULTI_AP)
			/* Hide Guest SSID for Mesh */
			if (j == WLAN_MBSSID_NUM)
				continue;
#endif
			if (!mib_chain_get(MIB_MBSSIB_TBL, j, (void *)&mbssEntry))
				continue;

			if (ifdomain & INTFGRPING_DOMAIN_WLAN)
			{
				if (groupnum == (-1) || groupnum == mbssEntry.itfGroup)
				{
					rtk_wlan_get_ifname(i, j, buff);
					info[count].ifdomain = INTFGRPING_DOMAIN_WLAN;
					info[count].ifid = ((i * (total + 1)) + j);
					sprintf(info[count].name, "%s", buff);
					info[count].itfGroup = mbssEntry.itfGroup;
					if (++count >= max_size)
					{
						wlan_idx = orig_wlan_idx;
						return count;
					}
				}
			}

			/* WLAN VLAN Interface */
			total = mib_chain_total(MIB_VLAN_GROUP_TBL);
			for (k = 0; k < total; k++)
			{
				if (!mib_chain_get(MIB_VLAN_GROUP_TBL, k, (void *)&groupEntry))
					continue;

				idx = ((i * (WLAN_MBSSID_NUM + 1)) + j)+PMAP_WLAN0;
				if (groupEntry.portMask & (1 << idx))
				{
					if (groupEntry.vid > 1)
					{
						if (ifdomain & INTFGRPING_DOMAIN_WLAN_VLAN)
						{
							if (groupnum == (-1) || groupnum == groupEntry.itfGroup)
							{
								rtk_wlan_get_ifname(i, j, buff);
								info[count].ifdomain = INTFGRPING_DOMAIN_WLAN_VLAN;
								info[count].ifid = k;
								pName = info[count].name;
								pName += sprintf(info[count].name, "%s", buff);
								pName += sprintf(pName, "_%d", groupEntry.vid);
								if (groupEntry.vprio >= 0)
								{
									pName += sprintf(pName, "_%d", groupEntry.vprio);
								}

								info[count].itfGroup = groupEntry.itfGroup;
								if (++count >= max_size)
									return count;
							}
						}
					}
				}
			}
		}
	}
	wlan_idx = orig_wlan_idx;
#endif

	/* WAN Interface */
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&wanEntry))
			continue;

		if (!isValidMedia(wanEntry.ifIndex))
			continue;
#ifdef _CWMP_MIB_
		/* skip the automatic build bridge WAN */
		if (wanEntry.connDisable == 1 && wanEntry.ConPPPInstNum == 0 && wanEntry.ConIPInstNum == 0)
			continue;
#endif
#if 0 // only show bridge WAN
			if (wanEntry.cmode != CHANNEL_MODE_BRIDGE && !ROUTE_CHECK_BRIDGE_MIX_ENABLE(&wanEntry))
				continue;
#endif

#ifdef _CWMP_MIB_
			if ((conninstBit & (1 << wanEntry.ConDevInstNum)) == 0 || (mediaBit & (1 << MEDIA_INDEX(wanEntry.ifIndex))) == 0)
			{
				conninstBit |= (1 << wanEntry.ConDevInstNum);
				mediaBit |= (1 << MEDIA_INDEX(wanEntry.ifIndex));
				if (ifdomain & INTFGRPING_DOMAIN_WAN)
				{
					info[count].ifdomain = INTFGRPING_DOMAIN_WAN;
					info[count].ifid = ((MEDIA_INDEX(wanEntry.ifIndex) << 8) + wanEntry.ConDevInstNum);
					sprintf(info[count].name, "WANConnectionDevice.%d", wanEntry.ConDevInstNum);
					info[count].itfGroup = 0;
					if (++count >= max_size)
						return count;
				}
			}
#endif

			if (ifdomain & INTFGRPING_DOMAIN_WANROUTERCONN)
			{
				if (groupnum == (-1) || groupnum == wanEntry.itfGroupNum)
				{
					info[count].ifdomain = INTFGRPING_DOMAIN_WANROUTERCONN;
					info[count].ifid = wanEntry.ifIndex;
					ifGetName(wanEntry.ifIndex, info[count].name, sizeof(info[count].name));
					info[count].itfGroup = wanEntry.itfGroupNum;
					if (++count >= max_size)
						return count;
				}
			}
	}

	return count;
}
#endif

int getIPaddrInfo(MIB_CE_ATM_VC_Tp entryp, char *ipaddr, char *netmask, char *gateway)
{
	int flags, flags_found, isPPP;
	struct in_addr inAddr;
	char ifname[IFNAMSIZ], *str_ipv4;
	int maskbits;

	strcpy(ipaddr, "");
	strcpy(netmask, "");
	strcpy(gateway, "");
#ifdef CONFIG_IPV6
	if ((entryp->IpProtocol & IPVER_IPV4) == 0)
		return -1;
#endif

	ifGetName(entryp->ifIndex, ifname, sizeof(ifname));
	flags_found = getInFlags(ifname, &flags);

	switch (entryp->cmode) {
		case CHANNEL_MODE_BRIDGE:
			isPPP = 0;
			break;
		case CHANNEL_MODE_IPOE:
			isPPP = 0;
			break;
		case CHANNEL_MODE_PPPOE:	//patch for pppoe proxy
			isPPP = 1;
			break;
		case CHANNEL_MODE_PPPOA:
			isPPP = 1;
			break;
		default:
			isPPP = 0;
			break;
	}
	strcpy(ipaddr, "");
#ifdef EMBED
	if (flags_found && getInAddr(ifname, IP_ADDR, &inAddr) == 1) {
		str_ipv4 = inet_ntoa(inAddr);
		// IP Passthrough or IP unnumbered
		if (flags & IFF_POINTOPOINT && (strcmp(str_ipv4, "10.0.0.1") == 0))
			strcpy(ipaddr, STR_UNNUMBERED);
		else
			strcpy(ipaddr, str_ipv4);
	}
#endif

	strcpy(netmask, "");
	if (flags_found && getInAddr(ifname, SUBNET_MASK, &inAddr) == 1) {
		for ( maskbits=32 ; ((inAddr.s_addr & (1L<<(32-maskbits))) == 0)&&(maskbits>0) ; maskbits-- ){
			//do nothing
		}
		sprintf(netmask, "%d", maskbits);
	}

	strcpy(gateway, "");
	if (flags_found)
	{
		if(isPPP)
		{
			if(getInAddr(ifname, DST_IP_ADDR, &inAddr) == 1)
				strcpy(gateway, inet_ntoa(inAddr));
		}
		else
		{
			if(entryp->ipDhcp == (char)DHCP_CLIENT)
			{
				FILE *fp = NULL;
				char fname[128] = {0};

				sprintf(fname, "%s.%s", MER_GWINFO, ifname);

				if(fp = fopen(fname, "r"))
				{
					if(fscanf(fp, "%s", gateway) != 1)
						printf("fscanf failed: %s %d\n", __func__, __LINE__);
					fclose(fp);
				}
			}
			else
			{
				unsigned char zero[IP_ADDR_LEN] = {0};
				if(memcmp(entryp->remoteIpAddr, zero, IP_ADDR_LEN))
					strcpy(gateway, inet_ntoa(*((struct in_addr *)entryp->remoteIpAddr)));
			}
		}
	}

	return 0;
}

#if (defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
int config_port_pptp_def_rule(int logPortId)
{
	int tr142_customer_flag=0, i;
	char ifname[IFNAMSIZ] = {0}, macaddr[16], smux_rule_alias[64] = {0}, smux_cmd[512] = {0};
	char realIfname[IFNAMSIZ] = {0};
	int pon_mode = 0;

	mib_get_s(MIB_PON_MODE, (void *)&pon_mode, sizeof(pon_mode));
	if(pon_mode == GPON_MODE)
	{
		va_cmd(EBTABLES, 6, 1, "-A", L2_PPTP_PORT_INPUT_TBL, "-i", ELANVIF[logPortId], "-j", L2_PPTP_PORT_SERVICE);
		va_cmd(EBTABLES, 6, 1, "-A", L2_PPTP_PORT_OUTPUT_TBL, "-o", ELANVIF[logPortId], "-j", L2_PPTP_PORT_SERVICE);
		va_cmd(EBTABLES, 6, 1, "-A", L2_PPTP_PORT_FORWARD_TBL, "-o", ELANVIF[logPortId], "-j", L2_PPTP_PORT_FORWARD);

		sprintf(ifname, "%s_pptp", ELANRIF[logPortId]);
#ifdef CONFIG_TR142_MODULE
		mib_get_s(MIB_TR142_CUSTOMER_FLAG, &tr142_customer_flag, sizeof(tr142_customer_flag));
		if((tr142_customer_flag & TR142_CUSTOMER_FLAG_ISOLATION_PER_PORT))
		{
			va_cmd(EBTABLES, 6, 1, "-A", L2_PPTP_PORT_FORWARD_TBL, "-o", ifname, "-j", L2_PPTP_PORT_FORWARD);
		}
#endif
	}

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	if(rtk_port_check_external_port(logPortId))
	{
		snprintf(realIfname, sizeof(realIfname)-1, "%s", EXTELANRIF[logPortId]);
	}
	else
#endif
	{
		snprintf(realIfname, sizeof(realIfname)-1, "%s", ELANRIF[logPortId]);
	}

#if 1
	getMIB2Str(MIB_ELAN_MAC_ADDR, macaddr);
	snprintf(smux_rule_alias, sizeof(smux_rule_alias), "pptp-l2_uni_def");
	snprintf(smux_cmd, sizeof(smux_cmd), "%s --if %s --rx --tags 0 --filter-dmac %s --set-rxif %s --target ACCEPT --rule-priority %d --rule-alias %s --rule-append",
								SMUXCTL, realIfname, macaddr, ELANVIF[logPortId], SMUX_RULE_PRIO_PPTP_LAN_UNI, smux_rule_alias);
	va_cmd("/bin/sh", 2, 1, "-c", smux_cmd);
	snprintf(smux_cmd, sizeof(smux_cmd), "%s --if %s --rx --tags 0 --filter-ethertype 0x0806 --filter-dmac ffffffffffff --set-rxif %s --duplicate-forward --rule-priority %d --rule-alias %s_ARP --rule-append",
								SMUXCTL, realIfname, ELANVIF[logPortId], SMUX_RULE_PRIO_PPTP_LAN_UNI, smux_rule_alias);
	va_cmd("/bin/sh", 2, 1, "-c", smux_cmd);
	snprintf(smux_cmd, sizeof(smux_cmd), "%s --if %s --rx --tags 0 --filter-ethertype 0x86dd --filter-multicast --filter-ipl4proto 0x3a --set-rxif %s --duplicate-forward --rule-priority %d --rule-alias %s_ARP --rule-append",
								SMUXCTL, realIfname, ELANVIF[logPortId], SMUX_RULE_PRIO_PPTP_LAN_UNI, smux_rule_alias);
	va_cmd("/bin/sh", 2, 1, "-c", smux_cmd);

	for(i = 0; i<=SMUX_MAX_TAGS;i++)
	{
		//STP to br0 force
		snprintf(smux_cmd, sizeof(smux_cmd),  "%s --if %s --rx --tags %d --filter-dmac 0180c2000000 FFFFFFFFFFFF --set-rxif %s --target ACCEPT --rule-priority %d --rule-alias %s_STP --rule-append",
									SMUXCTL, realIfname, i, ELANVIF[logPortId], SMUX_RULE_PRIO_PPTP_LAN_UNI+1, smux_rule_alias);
		va_cmd("/bin/sh", 2, 1, "-c", smux_cmd);
#if 0
		snprintf(smux_cmd, sizeof(smux_cmd), "%s --if %s --rx --tags %d --target DROP --rule-priority %d --rule-alias %s --rule-append",
								SMUXCTL, ELANRIF[logPortId], i, SMUX_RULE_PRIO_PPTP_LAN_UNI_DEF, smux_rule_alias);
		va_cmd("/bin/sh", 2, 1, "-c", smux_cmd);
#endif
	}

#endif
	return 0;
}

int reset_port_pptp_def_rule(int logPortId)
{
	int tr142_customer_flag=0, i;
	char ifname[32] = {0}, smux_rule_alias[64] = {0}, smux_cmd[512] = {0};
	char realIfname[IFNAMSIZ] = {0};
	int pon_mode = 0;

	mib_get_s(MIB_PON_MODE, (void *)&pon_mode, sizeof(pon_mode));
	if(pon_mode == GPON_MODE)
	{
		va_cmd(EBTABLES, 6, 1, "-D", L2_PPTP_PORT_INPUT_TBL, "-i", ELANVIF[logPortId], "-j", L2_PPTP_PORT_SERVICE);
		va_cmd(EBTABLES, 6, 1, "-D", L2_PPTP_PORT_OUTPUT_TBL, "-o", ELANVIF[logPortId], "-j", L2_PPTP_PORT_SERVICE);
		va_cmd(EBTABLES, 6, 1, "-D", L2_PPTP_PORT_FORWARD_TBL, "-o", ELANVIF[logPortId], "-j", L2_PPTP_PORT_FORWARD);

		sprintf(ifname, "%s_pptp", ELANRIF[logPortId]);
#ifdef CONFIG_TR142_MODULE
		mib_get_s(MIB_TR142_CUSTOMER_FLAG, &tr142_customer_flag, sizeof(tr142_customer_flag));
		if((tr142_customer_flag & TR142_CUSTOMER_FLAG_ISOLATION_PER_PORT))
		{
			va_cmd(EBTABLES, 6, 1, "-D", L2_PPTP_PORT_FORWARD_TBL, "-o", ifname, "-j", L2_PPTP_PORT_FORWARD);
		}
#endif
	}

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	if(rtk_port_check_external_port(i))
	{
		snprintf(realIfname, sizeof(realIfname)-1, "%s", EXTELANRIF[logPortId]);
	}
	else
#endif
	{
		snprintf(realIfname, sizeof(realIfname)-1, "%s", ELANRIF[logPortId]);
	}

#if 1
	snprintf(smux_rule_alias, sizeof(smux_rule_alias), "pptp-l2_uni_def");
	for(i = 0; i<=SMUX_MAX_TAGS;i++)
	{
		snprintf(smux_cmd, sizeof(smux_cmd),  "%s --if %s --rx --tags %d --rule-remove-alias %s+",
									SMUXCTL, realIfname, i, smux_rule_alias);
		va_cmd("/bin/sh", 2, 1, "-c", smux_cmd);
	}
#endif

	return 0;
}

int reconfig_port_pptp_def_rule(void)
{
#ifdef CONFIG_RTK_OMCI_V1
	int i, count=0;
	MIB_CE_SW_PORT_T portEntry;

	for(i=0;i<ELANVIF_NUM;i++)
	{
		if (!mib_chain_get(MIB_SW_PORT_TBL, i, &portEntry))
			continue;

		if(portEntry.omci_configured && portEntry.omci_uni_portType == OMCI_PORT_TYPE_PPTP)
		{
			reset_port_pptp_def_rule(i);
			config_port_pptp_def_rule(i);
		}
	}
#endif
	return 0;
}
#endif

int rtk_hwnat_flow_flush(void)
{
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	rtk_fc_flow_flush();
#endif
	return 0;
}

#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_CTC_E8_CLIENT_LIMIT)
int rtk_fc_accesswanlimit_set(void)
{
#ifdef _PRMT_X_CT_COM_MWBAND_
	rt_misc_wan_access_limit_t access_wan_info;
	int ret=0;
	unsigned int vInt;

	memset(&access_wan_info,0,sizeof(rt_misc_wan_access_limit_t));
	mib_get_s( CWMP_CT_MWBAND_MODE, (void *)&vInt, sizeof(vInt));

	switch(vInt)
	{
		case 0://unlimit
			access_wan_info.accessLimitNumber = -1; //-1 = unlimit
			ret = rt_misc_wan_access_limit_set(access_wan_info);
			if(ret){
				fprintf(stderr, "%s-%d error ret=%d\n",__func__,__LINE__,ret);
				return -1;
			}
			break;
		case 1://all port
			access_wan_info.limitField = RT_MISC_WAN_ACCESS_LIMIT_BY_IP;
			unsigned int portMask = 0;
			unsigned int phy_portmask;

			phy_portmask = rtk_port_get_all_lan_phyMask();
			access_wan_info.portMask = phy_portmask;
#ifdef WLAN_SUPPORT
#ifdef WLAN_DUALBAND_CONCURRENT
			unsigned int allWLanPortMask1 = ((1<<WLAN_SSID_NUM)-1);
			access_wan_info.wlanMask[1] = allWLanPortMask1;
#endif
			unsigned int allWLanPortMask0 = ((1<<WLAN_SSID_NUM)-1);
			access_wan_info.wlanMask[0] = allWLanPortMask0;//all master wifi port
#endif
			mib_get_s( CWMP_CT_MWBAND_NUMBER, (void *)&vInt, sizeof(vInt));
			access_wan_info.accessLimitNumber = vInt;

			ret = rt_misc_wan_access_limit_set(access_wan_info);
			if(ret){
				fprintf(stderr, "%s-%d error ret=%d\n",__func__,__LINE__,ret);
				return -1;
			}

			break;
	}
#endif
	return 0;
}
#endif //CONFIG_CTC_E8_CLIENT_LIMIT

#ifdef CONFIG_USER_LANNETINFO
int get_lan_net_info(lanHostInfo_t **ppLANNetInfoData, unsigned int *pCount)
{
	struct stat status;
	int fd, read_count;
	void *p = NULL;
#ifdef CONFIG_USER_RETRY_GET_FLOCK
	unsigned long long msecond = 0;
	struct timeval tv;
	struct timeval initial_tv;
#else
	int pid;
#endif

	if( (ppLANNetInfoData==NULL)||(pCount==NULL) )
	{
		return -1;
	}

	*ppLANNetInfoData=NULL;
	*pCount=0;

#if !defined( CONFIG_USER_RETRY_GET_FLOCK)
	pid = read_pid(LANNETINFO_RUNFILE);
	if( pid>0 )
		kill(pid, SIGUSR2);

	usleep(60000);//60ms
#endif

	if ( stat(LANNETINFOFILE, &status) < 0 )
		return -3;

	fd = open(LANNETINFOFILE, O_RDONLY);
	if(fd < 0)
		return -3;

#ifdef CONFIG_USER_RETRY_GET_FLOCK
	gettimeofday (&tv, NULL);
	initial_tv.tv_sec = tv.tv_sec;
	initial_tv.tv_usec = tv.tv_usec;

	do {
		if (!flock_set(fd, F_RDLCK))
		{
			if ((p = (void *)read_data_from_file(fd, LANNETINFOFILE, &read_count)) == NULL)
			{
				printf("(%s)%d read file error!\n", __FUNCTION__, __LINE__);
				flock_set(fd, F_UNLCK);
				close(fd);
				return -3;
			}

			*ppLANNetInfoData = (lanHostInfo_t *)p;
			*pCount = read_count / sizeof(lanHostInfo_t);

			flock_set(fd, F_UNLCK);
			break;
		}

		gettimeofday(&tv, NULL);
		msecond = ((unsigned long long)tv.tv_sec *1000 + tv.tv_usec/1000) - ((unsigned long long)initial_tv.tv_sec *1000 + initial_tv.tv_usec/1000);
	} while(msecond < MAX_READLANNETINFO_INTERVAL);
#else
	if(!flock_set(fd, F_RDLCK))
	{
		if ((p = (void *)read_data_from_file(fd, LANNETINFOFILE, &read_count)) == NULL)
		{
			printf("(%s)%d read file error!\n", __FUNCTION__, __LINE__);
			flock_set(fd, F_UNLCK);
			close(fd);
			return -3;
	}

		*ppLANNetInfoData = (lanHostInfo_t *)p;
		*pCount = read_count / sizeof(lanHostInfo_t);
		flock_set(fd, F_UNLCK);
	}
#endif

	close(fd);
	return 0;
}

#ifdef CONFIG_USER_LANNETINFO_COLLECT_OFFLINE_DEVINFO
int get_offline_lan_net_info(lanHostInfo_t **ppLANNetInfoData, unsigned int *pCount, char* file, int boot)
{
	struct stat status;
	int fd, i, read_size, pid;
	lanHostInfo_t *pLANNetInfoData = NULL;

	if( (ppLANNetInfoData==NULL)||(pCount==NULL) )
		return -1;

	//osgi api called when boot==0
	if(boot == 0){
		pid = read_pid(LANNETINFO_RUNFILE);
		if( pid>0 )
			kill(pid, SIGUSR2);

		usleep(30000);//30ms
	}

	if ( stat(file, &status) < 0 )
		return -3;

	fd = open(file, O_RDONLY);
	if(fd < 0)
		return -3;

	read_size = (unsigned long)(status.st_size);
	*pCount = read_size / sizeof(lanHostInfo_t);

	pLANNetInfoData = (lanHostInfo_t *) malloc(read_size);

	if(pLANNetInfoData == NULL)
	{
		if(fd) close(fd);
		return -3;
	}
	*ppLANNetInfoData = pLANNetInfoData;
	memset(pLANNetInfoData, 0, read_size);

	if(!flock_set(fd, F_RDLCK))
	{
		read_size = read(fd, (void*)pLANNetInfoData, read_size);
		flock_set(fd, F_UNLCK);
	}

	close(fd);
	return 0;
}
#endif
#ifdef CONFIG_USER_REFINE_CHECKALIVE_BY_ARP
int getPortSpeedInfoFromFile(lan_net_port_speed_T **ppPortSpeedData, unsigned int *pCount)
{
	struct stat status;
	int fd, read_count;
	void *p = NULL;
#ifdef CONFIG_USER_RETRY_GET_FLOCK
	unsigned long long msecond = 0;
	struct timeval tv;
	struct timeval initial_tv;
#endif

	if( (ppPortSpeedData==NULL)||(pCount==NULL) )
	{
		return -1;
	}

	*ppPortSpeedData=NULL;
	*pCount=0;

	if ( stat(PORTSPEEDFILE, &status) < 0 )
		return -3;

	fd = open(PORTSPEEDFILE, O_RDONLY);
	if(fd < 0)
		return -3;

#ifdef CONFIG_USER_RETRY_GET_FLOCK
	gettimeofday (&tv, NULL);
	initial_tv.tv_sec = tv.tv_sec;
	initial_tv.tv_usec = tv.tv_usec;

	do {
		if (!flock_set(fd, F_RDLCK))
		{
			if ((p = (void *)read_data_from_file(fd, PORTSPEEDFILE, &read_count)) == NULL)
			{
				printf("(%s)%d read file error!\n", __FUNCTION__, __LINE__);
				flock_set(fd, F_UNLCK);
				close(fd);
				return -3;
			}

			*ppPortSpeedData = (lan_net_port_speed_T *)p;
			*pCount = read_count / sizeof(lan_net_port_speed_T);

			flock_set(fd, F_UNLCK);
			break;
		}

		gettimeofday(&tv, NULL);
		msecond = ((unsigned long long)tv.tv_sec *1000 + tv.tv_usec/1000) - ((unsigned long long)initial_tv.tv_sec *1000 + initial_tv.tv_usec/1000);
	} while(msecond < MAX_READPORTSPEEDINFO_INTERVAL);
#else
	if(!flock_set(fd, F_RDLCK))
	{
		if ((p = (void *)read_data_from_file(fd, PORTSPEEDFILE, &read_count)) == NULL)
		{
			printf("(%s)%d read file error!\n", __FUNCTION__, __LINE__);
			flock_set(fd, F_UNLCK);
			close(fd);
			return -3;
		}

		*ppPortSpeedData = (lan_net_port_speed_T *)p;
		*pCount = read_count / sizeof(lan_net_port_speed_T);
		flock_set(fd, F_UNLCK);
	}
#endif

	close(fd);
	return 0;
}
int get_speed_by_portNum(int port, unsigned int *rxRate, unsigned int *txRate)
{
	int i = 0, count = 0;;
	lan_net_port_speed_T *pSpeedInfo=NULL;
	unsigned int rxSpeed = 0, txSpeed = 0;

	if (port > 11 || txRate == NULL || rxRate == NULL)
		return (-1);

	*rxRate = 0;
	*txRate = 0;

	if(getPortSpeedInfoFromFile(&pSpeedInfo, &count) != 0)
	{
		printf("(%s)%d  get port speed info faild!\n", __FUNCTION__, __LINE__);

		if(pSpeedInfo)
			free(pSpeedInfo);

		return (-1);
	}

	for(i = 0; i < count; i++)
	{
		if (pSpeedInfo[i].portNum == port)
		{
			rxSpeed = pSpeedInfo[i].rxRate;
			txSpeed = pSpeedInfo[i].txRate;
			break;
		}
	}
	*rxRate = rxSpeed;
	*txRate = txSpeed;

	if(pSpeedInfo)
		free(pSpeedInfo);

	return 0;
}
#endif

int sendMessageToLanNetInfo(LANNETINFO_MSG_T *msg)
{
	int  ret, qid, cpid, ctgid;
	struct lanNetInfoMsg qbuf;
	int lannetinfo_pid = -1;

	qid = msgget((key_t)1771,  0660);
	if(qid <= 0){
		printf("[%s %d]: get lannetinfo msgqueue error!\n", __func__, __LINE__);
		return -2;
	}

	/* get lanNetInfo process pid*/
	lannetinfo_pid = read_pid(LANNETINFO_RUNFILE);

	if( (lannetinfo_pid == -1)||(0 != kill(lannetinfo_pid, 0)) )
	{
		printf("[%s %d]: There is no lanNetInfo Process!\n", __func__, __LINE__);
		return -3;
	}

	cpid = (int)syscall(SYS_gettid);
	ctgid = (int)getpid();

	/* Send a message to the queue */
	qbuf.mtype = lannetinfo_pid;
	qbuf.request = cpid;
	qbuf.tgid = ctgid;
	qbuf.msg.cmd = msg->cmd;
	qbuf.msg.arg1 = msg->arg1;
	qbuf.msg.arg2 = msg->arg2;
	memcpy(qbuf.msg.mtext, msg->mtext, MAX_DEVICE_INFO_SIZE);

	ret = msgsnd(qid, (void *)&qbuf, sizeof(struct lanNetInfoMsg)-sizeof(long), 0);
	if( ret== -1)
	{
		printf("[%s %d]:send devcie info request to lanNetInfo process error.\n", __func__, __LINE__);
		return ret;
	}
	return 0;
}

/*
	Return Value:
	Return -1 on failure;
	otherwise return the number of bytes actually copied into the mtext array.
*/
int readMessageFromLanNetInfo(struct lanNetInfoMsg *qbuf)
{
	int ret, qid, cpid, retry_count=20;

	qid = msgget((key_t)1771,  0660);
	if(qid <= 0){
		printf("[%s %d]: get lannetinfo msgqueue error!\n", __func__, __LINE__);
		return -1;
	}

	cpid = (int)syscall(SYS_gettid);
	qbuf->mtype = cpid;

rcv_retry:
	ret=msgrcv(qid, (void *)qbuf, sizeof(struct lanNetInfoMsg)-sizeof(long), cpid, 0);
	if (ret == -1) {
		switch (errno) {
				case EINTR	 :
					printf("====>>%s %d:EINTR	 \n", __func__, __LINE__);
					goto rcv_retry;
				case EACCES :
					printf("====>>%s %d:EACCES	\n", __func__, __LINE__);
					break;
				case EFAULT   :
					printf("====>>%s %d:EFAULT	 \n", __func__, __LINE__);
					break;
				case EIDRM	:
					printf("====>>%s %d:EIDRM	 \n", __func__, __LINE__);
					break;
				case EINVAL   :
					printf("====>>%s %d:EINVAL	 \n", __func__, __LINE__);
					break;
				case ENOMSG   :
					printf("====>>%s %d:ENOMSG	 \n", __func__, __LINE__);
					break;
				case E2BIG   :
					printf("====>>%s %d:E2BIG	 \n", __func__, __LINE__);
					break;
				default:
					printf("====>>%s %d:Unknown	 \n", __func__, __LINE__);
			}
	}

	return ret;
}

int set_lanhost_linkchange_port_status(int portIdx)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;
	//unsigned char max_lanHost_number = portIdx;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_LINKCHANGE_PORT_SET;

	mymsg->arg1 = portIdx;

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}
	printf("[%s %d]: failed! portIdx=%d\n", __func__, __LINE__, portIdx);
	return -4;
}

#ifdef CONFIG_TELMEX_DEV
int set_lanhost_force_recycle(unsigned int ipAddr)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_FORCE_RECYCLE;

	mymsg->arg1 = ipAddr;

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}
#endif//end of CONFIG_TELMEX_DEV

int set_lanhost_dhcp_release_mac(char *mac)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_DHCPRELEASE_IP_SET;

	memcpy(mymsg->mtext, mac, MAC_ADDR_LEN);

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			// printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}
	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}
int rtk_update_lan_hpc_for_mib(unsigned char *mac)
{
	unsigned char macStr[64] = {0};

	if(mac == NULL)
		return -1;

	if(isZeroMac(mac))
		return -1;

	sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_HOST_MIB_POSTROUTING, "-d", macStr);
	va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_HOST_MIB_POSTROUTING, "-d", macStr);

	return 0;
}

#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL
int rtk_fc_host_police_control_mib_counter_init(void)
{
	//ebtables -t nat -D POSTROUTING -j host_mib_postroute
	va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_POSTROUTING, "-j", FW_HOST_MIB_POSTROUTING);
	//ebtables -t nat -X host_mib_postroute
	va_cmd(EBTABLES, 4, 1, "-t", "nat", (char *)"-X", (char *)FW_HOST_MIB_POSTROUTING);
	//ebtables -t nat -N host_mib_postroute
	va_cmd(EBTABLES, 4, 1, "-t", "nat", (char *)"-N", (char *)FW_HOST_MIB_POSTROUTING);
	//ebtables -t nat -P host_mib_postroute RETURN
	va_cmd(EBTABLES, 5, 1, "-t", "nat", (char *)"-P", (char *)FW_HOST_MIB_POSTROUTING, (char *)FW_RETURN);
	//ebtables -t nat -I POSTROUTING -j host_mib_postroute
	va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_POSTROUTING, "-j", FW_HOST_MIB_POSTROUTING);

	return 0;
}

char rtk_lanhost_get_host_index_by_mac(char *mac)
{
	MIB_LAN_HOST_BANDWIDTH_T entry;
	int i, totalNum;

	memset(&entry, 0 , sizeof(MIB_LAN_HOST_BANDWIDTH_T));
	totalNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);
	for(i=0; i<totalNum; i++)
	{
		if (!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, i, (void *)&entry))
		{
			printf("[%s %d]get LAN_HOST_BANDWIDTHl chain entry error!\n", __func__, __LINE__);
			continue;
		}

		if(!memcmp(mac, entry.mac, ETHER_ADDR_LEN))
		{
			return entry.hostIndex;
		}
	}

	return -1;
}

int rtk_host_police_control_mib_counter_get(unsigned char *mac, LAN_HOST_POLICE_CTRL_MIB_TYPE_T type, unsigned long long *result)
{
	char internetWanName[IFNAMSIZ]={0};
	unsigned char ebt_macStr[64], macStr[64];
	unsigned char ebt_mac[6];
	unsigned char path[64]={0};
	unsigned long long packetCount=0, byteCount=0;
	char hostIndex;
	FILE *fp = NULL;
	char line[512];
	char cmd[256];
	unsigned long long finalByteCount;

#define TX_L4_HEADER_SIZE	 54
#define RX_L4_HEADER_SIZE	 36

	sprintf(path, "/tmp/host_police_mib_counter");
	if(type == MIB_TYPE_TX_BYTES)
	{
		if(!getInternetWANItfName(internetWanName,sizeof(internetWanName)-1))
		{
			return -1;
		}

		hostIndex = rtk_lanhost_get_host_index_by_mac(mac);
		if(hostIndex == -1)
		{
			return -1;
		}
		sprintf(cmd, "%s -s class show dev %s | grep \"2:%d0 parent 2:1 leaf %d00\" -A 1  > %s", TC, internetWanName, (hostIndex+1), (hostIndex+1),  path);
	}
	else
	{
		sprintf(cmd, "%s -t nat -L %s --Lc > %s", EBTABLES, FW_HOST_MIB_POSTROUTING,  path);
	}
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	if((fp=fopen(path, "r")))
	{
		while(fgets(line, sizeof(line), fp)!=NULL)
		{
			if(type == MIB_TYPE_TX_BYTES)
			{
				if(sscanf(line,"%*s %llu %*s %llu %*s", &byteCount, &packetCount) == 2 && byteCount)
				{
					//printf("Tx ebt_macStr=%s packetCount=%d byteCount=%d\n", ebt_macStr, packetCount, byteCount);
					finalByteCount = byteCount - (TX_L4_HEADER_SIZE*packetCount);
					if(finalByteCount <= 0)
					{
						//va_cmd(IPTABLES, 4, 1, "-t", "mangle", (char *)"-Z", (char *)FW_HOST_MIB_POSTROUTING);
						*result = 0;
						fclose(fp);
						unlink(path);
						return -1;
					}
					else
					{
						*result = finalByteCount;
						fclose(fp);
						unlink(path);
						return 0;
					}
				}
			}
			else
			{
				if(sscanf(line,"-d %hhx:%hhx:%hhx:%hhx:%hhx:%hhx -j CONTINUE , pcnt = %llu -- bcnt = %llu"
					, &ebt_mac[0], &ebt_mac[1], &ebt_mac[2], &ebt_mac[3], &ebt_mac[4], &ebt_mac[5], &packetCount, &byteCount) == 8)
				{
					sprintf(ebt_macStr, "%02X:%02X:%02X:%02X:%02X:%02X", ebt_mac[0], ebt_mac[1], ebt_mac[2], ebt_mac[3], ebt_mac[4], ebt_mac[5]);
					sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
					if(!strcmp(macStr, ebt_macStr))
					{
						//printf("Rx ebt_macStr=%s packetCount=%d byteCount=%d\n", ebt_macStr, packetCount, byteCount);
						finalByteCount = byteCount - (RX_L4_HEADER_SIZE*packetCount);
						if(finalByteCount <= 0)
						{
							//sprintf(cmd, "%s %s %s %s %s %s %s %s", EBTABLES, "-t", "nat", (char *)"-L", (char *)FW_HOST_MIB_POSTROUTING, "-Z", ">", "/dev/null");
							//va_cmd("/bin/sh", 2, 1, "-c", cmd);
							*result = 0;
							fclose(fp);
							unlink(path);
							return -1;
						}
						else
						{
							*result = finalByteCount;
							fclose(fp);
							unlink(path);
							return 0;
						}
					}
				}
			}
		}
		fclose(fp);
	}

	unlink(path);
	return -1;
}

int rtk_lan_host_trap_check(char *mac)
{
	unsigned char bandcontrolEnable;
	MIB_LAN_HOST_BANDWIDTH_T entry;
	int i, totalNum;

	if(!mib_get_s(MIB_LANHOST_BANDWIDTH_CONTROL_ENABLE, (void *)&bandcontrolEnable, sizeof(bandcontrolEnable)))
	{
		printf("Get Bandwidth Control Enable error!");
	}

	if(!bandcontrolEnable)
	{
		return 0;//when disable bandwidth control, use hw counter.
	}

	memset(&entry, 0 , sizeof(MIB_LAN_HOST_BANDWIDTH_T));
	totalNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);
	for(i=0; i<totalNum; i++)
	{
		if (!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, i, (void *)&entry))
		{
			printf("[%s %d]get LAN_HOST_BANDWIDTH_TBL chain entry error!\n", __func__, __LINE__);
			return 0;
		}

		if(!entry.maxUsBandwidth && !entry.maxDsBandwidth)
			continue;

		if(!memcmp(mac, entry.mac, ETHER_ADDR_LEN))
			return 1;
	}

	return 0;
}

int rtk_lanhost_limit_by_ps_check(char *mac, LAN_HOST_POLICE_CTRL_MIB_TYPE_T type)
{
	int ratelimit_trap_to_ps=0; //1: limit by TC otherwise:limit by FC shortcut
	unsigned char bandcontrolEnable;
	MIB_LAN_HOST_BANDWIDTH_T entry;
	int i, totalNum;

	if(!mib_get_s(MIB_LANPORT_RATELIMIT_TRAP_TO_PS, (void *)&ratelimit_trap_to_ps, sizeof(ratelimit_trap_to_ps)))
	{
		printf("[%s %d]Get mib value MIB_LANPORT_RATELIMIT_TRAP_TO_PS failed!\n", __func__, __LINE__);
		return 0;
	}

	if(!mib_get_s(MIB_LANHOST_BANDWIDTH_CONTROL_ENABLE, (void *)&bandcontrolEnable, sizeof(bandcontrolEnable)))
	{
		printf("Get Bandwidth Control Enable error!");
	}

	if(!bandcontrolEnable)
	{
		return 0;//when disable bandwidth control, use hw counter.
	}

	memset(&entry, 0 , sizeof(MIB_LAN_HOST_BANDWIDTH_T));
	totalNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);
	for(i=0; i<totalNum; i++)
	{
		if (!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, i, (void *)&entry))
		{
			printf("[%s %d]get LAN_HOST_BANDWIDTH_TBL chain entry error!\n", __func__, __LINE__);
			return 0;
		}

		if (type == MIB_TYPE_TX_BYTES)
		{
			if(!entry.maxUsBandwidth)
				continue;

			if(entry.maxUsBandwidth <= ratelimit_trap_to_ps)
				return 1;
		}
		else if (type == MIB_TYPE_RX_BYTES)
		{
			if(!entry.maxDsBandwidth)
				continue;

			if(entry.maxDsBandwidth <= ratelimit_trap_to_ps)
				return 1;
		}
	}

	return 0;
}
#endif //CONFIG_USER_LAN_BANDWIDTH_CONTROL
/****************************************************************************/
#endif //#ifdef CONFIG_USER_LANNETINFO
#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL
typedef struct lanport_ctlrate_info{
	unsigned int upRate;   /* in unit of kbps */
	unsigned int downRate; /* in unit of kbps */
}LANPORT_CTLRATE_INFO;

LANPORT_CTLRATE_INFO lanPortCtlRateInfo[4];

#ifdef WLAN_SUPPORT
typedef struct wifi_ctlrate_info{
	unsigned char mac[MAC_ADDR_LEN];   /* mac address */
	unsigned int upRate;   /* in unit of kbps */
	unsigned int downRate; /* in unit of kbps */
}WIFI_CTLRATE_INFO;

WIFI_CTLRATE_INFO wifiCtlRateInfo[MAX_STA_NUM+1];

char wlan_sta_buf[sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1)];
int isWifiMacOnline(unsigned char *mac)
{
	int i;
	WLAN_STA_INFO_Tp pInfo;

	for(i=1; i<=MAX_STA_NUM; i++)
	{
		pInfo = (WLAN_STA_INFO_Tp)&wlan_sta_buf[i*sizeof(WLAN_STA_INFO_T)];
		if (pInfo->aid && (pInfo->flags & STA_INFO_FLAG_ASOC))
		{
			if( !macAddrCmp(mac, pInfo->addr) )
				return i;
		}
	}

	return 0;
}
#endif

int rtk_lan_host_index_check_exist(int hostIndex)
{
	MIB_LAN_HOST_BANDWIDTH_T entry;
	int i, totalNum;

	totalNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);
	for(i=0; i<totalNum; i++)
	{
		if(!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, i, (void*)&entry))
		{
			printf("Get lan host bandwidth table error.\n");
			return 0;
		}

		if(hostIndex == entry.hostIndex)
		{
			return 1;
		}
	}
	return 0;
}

int rtk_lan_host_get_new_host_index(void)
{
	int i, newHostIndex = -1;

	for(i=0 ; i<16 ; i++)
	{

		if(!rtk_lan_host_index_check_exist(i))
		{
			if(newHostIndex == -1) {
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
				if(rtk_fc_get_empty_host_policer_by_id(i))
#endif
				newHostIndex = i;
			}
		}
	}

	return newHostIndex;
}

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
int rtk_lan_host_get_host_by_mac(unsigned char* mac)
{
	return rtk_fc_get_host_policer_by_mac(mac);
}
#endif

int rtk_lan_host_get_mib_index_by_host_index(int hostIndex)
{
	MIB_LAN_HOST_BANDWIDTH_T entry;
	int i, totalNum;

	totalNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);
	for(i=0; i<totalNum; i++)
	{
		if(!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, i, (void*)&entry))
		{
			printf("Get lan host bandwidth table error.\n");
			return -1;
		}

		if(hostIndex == entry.hostIndex)
		{
			return i;
		}
	}

	return -1;
}

void apply_lanPortRateLimitInPS(void)
{
	MIB_LAN_HOST_BANDWIDTH_T entry;
	int i, totalNum, mibIndex;
	char s_rate[16], s_ceil[16], s_classid[16], s_quantum[16],s_handleid[16];
	char s_mac2[16], s_mac4[16],s_mac[32];
	char d_mac2[16], d_mac4[16];
	char internetWanName[IFNAMSIZ]={0};
	char tc_handle_id[128];
	char us_needed=0;
	char ds_needed=0;
	char cmd[512];
	FILE * fp;
	int ret;

#define RATE_BASE_FOR_KBPS 1024

	getInternetWANItfName(internetWanName,sizeof(internetWanName)-1);
	printf("%s %d internetWanName=%s\n",__FUNCTION__,__LINE__,internetWanName);
	memset(&entry, 0 , sizeof(MIB_LAN_HOST_BANDWIDTH_T));
	totalNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);
	for(i=0; i<totalNum; i++)
	{
		if (!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, i, (void *)&entry))
		{
			printf("[%s %d]get LAN_HOST_BANDWIDTHl chain entry error!\n", __func__, __LINE__);
				return;
		}

		if(entry.maxUsBandwidth)
			us_needed = 1;

		if(entry.maxDsBandwidth)
			ds_needed = 1;
	}
	if(totalNum)
	{
		//AUG_PRT("ADD DEFAULT!!!!!\n");
		if(ds_needed)
		{
			//AUG_PRT("FLUSH ALL DS!!!!!\n");
			va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", "br0", "root");
			//AUG_PRT("ADD DEFAULT DS!!!!!\n");
			//download
			//tc qdisc add dev br0 root handle 1: htb default 10
			va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", "br0","root", "handle", "1:", "htb", "default", "10");
			//tc class add dev br0 parent 1: classid 1:1 htb rate 100Mbit
			va_cmd(TC, 15, 1, "class", (char *)ARG_ADD, "dev", "br0","parent", "1:", "classid", "1:1", "htb", "rate", "102400kbit", "ceil", "102400kbit","quantum","1500");
			//tc class add dev br0 parent 1:1 classid 1:10 htb rate 100Mbit
			va_cmd(TC, 15, 1, "class", (char *)ARG_ADD, "dev", "br0","parent", "1:", "classid", "1:10", "htb", "rate", "102400kbit", "ceil", "102400kbit","quantum","1500");
			//tc qdisc add dev br0 parent 1:10 handle 100: pfifo limit 100
			va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", "br0","parent", "1:10", "handle", "100:", "pfifo", "limit", "100");
		}
		else
		{
			//AUG_PRT("FLUSH ALL DS!!!!!\n");
			va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", "br0", "root");
		}
		//upload
		if(strlen(internetWanName))
		{
			if(us_needed)
			{
				//AUG_PRT("FLUSH ALL US!!!!!\n");
				va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", internetWanName, "root");
				//AUG_PRT("ADD DEFAULT US!!!!!\n");
				//iptables -t mangle -N lanhost_ratelimit_preroute
				va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", (char *)FW_LANHOST_RATE_LIMIT_PREROUTING);
				//iptables -t mangle -j lanhost_ratelimit_preroute
				va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_PREROUTING, "-j", (char *)FW_LANHOST_RATE_LIMIT_PREROUTING);
				//tc qdisc add dev nas0_0 root handle 1: htb default 10
				va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", internetWanName,"root", "handle", "2:", "htb", "default", "10");
				//tc class add dev nas0_0 parent 1: classid 1:1 htb rate 100Mbit
				va_cmd(TC, 15, 1, "class", (char *)ARG_ADD, "dev", internetWanName,"parent", "2:", "classid", "2:1", "htb", "rate", "102400kbit", "ceil", "102400kbit","quantum","1500");
				//tc class add dev nas0_0 parent 1:1 classid 1:10 htb rate 100Mbit
				va_cmd(TC, 15, 1, "class", (char *)ARG_ADD, "dev", internetWanName,"parent", "2:", "classid", "2:10", "htb", "rate", "102400kbit", "ceil", "102400kbit","quantum","1500");
				//tc qdisc add dev nas0_0 parent 1:10 handle 100: pfifo limit 100
				va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", internetWanName,"parent", "2:10", "handle", "100:", "pfifo", "limit", "100");
			}
			else
			{
				//AUG_PRT("FLUSH ALL US!!!!!\n");
				va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", internetWanName, "root");
			}
		}
	}
	else
	{
		//AUG_PRT("FLUSH ALLLLLL!!!!!\n");
		//clear all the old rules
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", "br0", "root");
		if(getInternetWANItfName(internetWanName,sizeof(internetWanName)-1))
			va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", internetWanName, "root");
	}

	for(i=1; i<=16; i++)
	{
		if((mibIndex = rtk_lan_host_get_mib_index_by_host_index(i)) != -1)
		{
			if (!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, mibIndex, (void *)&entry))
			{
				printf("[%s %d]get LAN_HOST_BANDWIDTHl chain entry error!\n", __func__, __LINE__);
			}

			//AUG_PRT(" >>>>> %02x:%02x:%02x:%02x:%02x:%02x",entry.mac[0],entry.mac[1], entry.mac[2],entry.mac[3],entry.mac[4],entry.mac[5]);
			snprintf(s_classid, 16, "2:%d0", entry.hostIndex+1);
			snprintf(s_handleid, 16, "%d00", entry.hostIndex+1);
			printf("[%s %d] s_classid=%s\n", __func__, __LINE__, s_classid);
			printf("[%s %d] s_handleid=%s\n", __func__, __LINE__, s_handleid);
			snprintf(tc_handle_id, 128, "0x%x/0x%x", SOCK_MARK_METER_INDEX_TO_MARK(i+1), SOCK_MARK_METER_INDEX_BIT_MASK);
			if (entry.maxUsBandwidth != 0)
			{
				snprintf(s_mac, sizeof(s_mac), "%02x:%02x:%02x:%02x:%02x:%02x",entry.mac[0],entry.mac[1], entry.mac[2],entry.mac[3],entry.mac[4],entry.mac[5]);
				//iptables -I lanhost_ratelimit_preroute -t mangle -m mac --mac-source 30:b5:c2:04:f2:aa -j MARK --set-mark 3001
				//iptables -I lanhost_ratelimit_preroute -t mangle -m mac --mac-source 30:b5:c2:04:f2:aa -j RETURN
				va_cmd(IPTABLES, 10, 1, "-I", FW_LANHOST_RATE_LIMIT_PREROUTING, "-t", "mangle", "-m", "mac", "--mac-source", s_mac, "-j", "RETURN" );
				va_cmd(IPTABLES, 12, 1, "-I", FW_LANHOST_RATE_LIMIT_PREROUTING, "-t", "mangle", "-m", "mac", "--mac-source", s_mac, "-j", "MARK", "--set-mark", tc_handle_id);

				if(strlen(internetWanName)){
					//upload
					snprintf(s_rate, 16, "%dbit", entry.maxUsBandwidth*RATE_BASE_FOR_KBPS);
					snprintf(s_ceil, 16, "%dbit", entry.maxUsBandwidth*RATE_BASE_FOR_KBPS);
					snprintf(cmd, 512, "/bin/tc -s class show dev %s parent 2:1 classid %s", internetWanName, s_classid);
					fp = popen(cmd, "r");
					if(fp == NULL)
						ret = -1;
					else {
						cmd[0] = '\0';
						fgets(cmd, sizeof(cmd), fp);
						if(cmd[0] != '\0')
							ret = 0;
						else
							ret = 1;
						pclose(fp);
					}
					if(ret)
					{
						//AUG_PRT("US ADD!!!!!\n");
						//tc class add dev nas0_0 parent 2:1 classid 2:20 htb rate 800Kbit ceil 800Kbit
						va_cmd(TC, 15, 1, "class", (char *)ARG_ADD, "dev", internetWanName,"parent", "2:1", "classid", s_classid, "htb", "rate", s_rate, "ceil", s_ceil,"quantum","1500");
						//tc qdisc add dev nas0_0 parent 2:20 handle 200: pfifo limit 100
						va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", internetWanName,"parent", s_classid, "handle", s_handleid, "pfifo", "limit", "100");
						//tc filter add dev nas0_0 parent 2:0 protocol ip handle 3001 fw flowid 2:20
						va_cmd(TC, 15, 1, "filter", (char *)ARG_ADD, "dev", internetWanName, "pref", s_handleid,"parent", "2:0", "protocol", "ip", "handle", (char *)tc_handle_id, "fw", "flowid", s_classid);
					}
					else
					{
						//AUG_PRT("US REPLACE!!!!!\n");
						//tc class replace dev nas0_0 parent 2:1 classid 2:20 htb rate 800Kbit ceil 800Kbit
						va_cmd(TC, 15, 1, "class", (char *)ARG_REPLACE, "dev", internetWanName,"parent", "2:1", "classid", s_classid, "htb", "rate", s_rate, "ceil", s_ceil,"quantum","1500");
					}
				}
			}
			else
			{
				//AUG_PRT("US DEL SINGLE!!!!! s_classid=%s s_handleid=%s\n", s_classid, s_handleid);
				//tc qdisc del dev nas0_0 parent 2:20
				va_cmd(TC, 6, 1, "qdisc", (char *)ARG_DEL, "dev", internetWanName,"parent", s_classid);
				//tc filter del dev nas0_0 pref 0xbb9
				va_cmd(TC, 6, 1, "filter", (char *)ARG_DEL, "dev", internetWanName,"pref", s_handleid);
				//tc class del dev nas0_0 parent 2:1
				va_cmd(TC, 6, 1, "class", (char *)ARG_DEL, "dev", internetWanName,"classid", s_classid);
			}

			snprintf(s_classid, 16, "1:%d0", entry.hostIndex+1);
			snprintf(s_handleid, 16, "%d00", entry.hostIndex+1);
			printf("[%s %d] s_classid=%s\n", __func__, __LINE__, s_classid);
			printf("[%s %d] s_handleid=%s\n", __func__, __LINE__, s_handleid);
			if (entry.maxDsBandwidth != 0)
			{
				snprintf(s_rate, 16, "%dbit", entry.maxDsBandwidth*RATE_BASE_FOR_KBPS);
				snprintf(s_ceil, 16, "%dbit", entry.maxDsBandwidth*RATE_BASE_FOR_KBPS);
				snprintf(d_mac4, 16, "0x%02x%02x%02x%02x",entry.mac[2],entry.mac[3],entry.mac[4],entry.mac[5]);
				snprintf(d_mac2, 16, "0x%02x%02x",entry.mac[0],entry.mac[1]);
				snprintf(cmd, 512, "/bin/tc -s class show dev br0 parent 1:1 classid %s", s_classid);
				fp = popen(cmd, "r");
				if(fp == NULL)
					ret = -1;
				else {
					cmd[0] = '\0';
					fgets(cmd, sizeof(cmd), fp);
					if(cmd[0] != '\0')
						ret = 0;
					else
						ret = 1;
					pclose(fp);
				}
				if(ret)
				{
					//AUG_PRT("DS ADD!!!!!\n");
					//tc class add dev br0 parent 1:1 classid 1:X0 htb rate 1600kbit ceil 1600kbit
					va_cmd(TC, 15, 1, "class", (char *)ARG_ADD, "dev", "br0","parent", "1:1", "classid", s_classid, "htb", "rate", s_rate, "ceil", s_ceil,"quantum","1400");
					//tc qdisc add dev br0 parent 1:X0 handle x00: pfifo limit 100
					va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", "br0","parent", s_classid, "handle", s_handleid, "pfifo", "limit", "100");
					//tc filter add dev br0 parent 1:0 protocol ip u32 match u16 0x0800 0xffff at -2 match u32 0x00ebd99f 0xffffffff at -12 match u16 0x0857 0xffff at -14 flowid 1:10
					va_cmd(TC, 31, 1, "filter", (char *)ARG_ADD, "dev", "br0", "pref", s_handleid,"parent", "1:0", "protocol", "ip", "u32", "match", "u16",
							"0x0800","0xffff","at","-2","match","u32",d_mac4,"0xffffffff","at","-12","match","u16",d_mac2,"0xffff",
							"at","-14","flowid",s_classid);
				}
				else
				{
					//AUG_PRT("DS REPLACE!!!!!\n");
					//tc class replace dev br0 parent 1:1 classid 1:X0 htb rate 1600kbit ceil 1600kbit
					va_cmd(TC, 15, 1, "class", (char *)ARG_REPLACE, "dev", "br0","parent", "1:1", "classid", s_classid, "htb", "rate", s_rate, "ceil", s_ceil,"quantum","1400");
				}
			}
			else
			{
				//AUG_PRT("DS DEL SINGLE!!!!! s_classid=%s s_handleid=%s\n", s_classid, s_handleid);
				//tc qdisc del dev nas0_0 parent 2:20
				va_cmd(TC, 6, 1, "qdisc", (char *)ARG_DEL, "dev", "br0","parent", s_classid);
				//tc filter del dev nas0_0 pref 0xbb9
				va_cmd(TC, 6, 1, "filter", (char *)ARG_DEL, "dev", "br0","pref", s_handleid);
				//tc class del dev nas0_0 parent 2:1
				va_cmd(TC, 6, 1, "class", (char *)ARG_DEL, "dev", "br0","classid", s_classid);
			}
		}
		else
		{
			snprintf(s_classid, 16, "2:%d0", i+1);
			snprintf(s_handleid, 16, "%d00", i+1);

			//AUG_PRT("US DEL SINGLE OTHER DELETED!!!!! s_classid=%s s_handleid=%s\n", s_classid, s_handleid);
			//tc qdisc del dev nas0_0 parent 2:20
			va_cmd(TC, 6, 1, "qdisc", (char *)ARG_DEL, "dev", internetWanName,"parent", s_classid);
			//tc filter del dev nas0_0 pref 0xbb9
			va_cmd(TC, 6, 1, "filter", (char *)ARG_DEL, "dev", internetWanName,"pref", s_handleid);
			//tc class del dev nas0_0 parent 2:1
			va_cmd(TC, 6, 1, "class", (char *)ARG_DEL, "dev", internetWanName,"classid", s_classid);

			snprintf(s_classid, 16, "1:%d0", i+1);
			snprintf(s_handleid, 16, "%d00", i+1);
			//AUG_PRT("DS DEL SINGLE OTHER DELETED!!!!! s_classid=%s s_handleid=%s\n", s_classid, s_handleid);
			//tc qdisc del dev br0 parent 2:20
			va_cmd(TC, 6, 1, "qdisc", (char *)ARG_DEL, "dev", "br0","parent", s_classid);
			//tc filter del dev br0 pref 0xbb9
			va_cmd(TC, 6, 1, "filter", (char *)ARG_DEL, "dev", "br0","pref", s_handleid);
			//tc class del dev br0 parent 2:1
			va_cmd(TC, 6, 1, "class", (char *)ARG_DEL, "dev", "br0","classid", s_classid);
		}
	}
}

void clear_lanPortRateLimitInPS(void)
{
	MIB_LAN_HOST_BANDWIDTH_T entry;
	int i, totalNum;
	char s_mac[32], tc_handle_id[128];

	//iptables -t mangle -D PREROUTING -j lanhost_ratelimit_preroute
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", (char *)FW_LANHOST_RATE_LIMIT_PREROUTING);
	//iptables -t mangle -F lanhost_ratelimit_preroute
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", (char *)FW_FLUSH, (char *)FW_LANHOST_RATE_LIMIT_PREROUTING);
	//iptables -t mangle -X lanhost_ratelimit_preroute
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", (char *)FW_LANHOST_RATE_LIMIT_PREROUTING);
	return;
}

/*
Return:
	0 - no change in  lanPortCtlRateInfo.
	1 - policy changed in lanPortCtlRateInfo.
	others - error occured.
 */
int reCalculateLanPortRateLimitInfo(void)
{
	int i, totalNum, macidx, idx, ssid=-1;
	unsigned int ratelimit;
	MIB_LAN_HOST_BANDWIDTH_T entry;
	LANPORT_CTLRATE_INFO oldInfo[4];
#ifdef WLAN_SUPPORT
	WIFI_CTLRATE_INFO oldWifiInfo[MAX_STA_NUM+1];
	WLAN_STA_INFO_Tp pInfo;
	//WIFISSID_CTLRATE_INFO oldSSIDInfo;
#endif

	//LAN
	memcpy(oldInfo, lanPortCtlRateInfo , sizeof(lanPortCtlRateInfo));
	memset(lanPortCtlRateInfo, 0, sizeof(lanPortCtlRateInfo));

#ifdef WLAN_SUPPORT
	//WIFI
	memcpy(oldWifiInfo, wifiCtlRateInfo , sizeof(wifiCtlRateInfo));
	memset(wifiCtlRateInfo, 0, sizeof(wifiCtlRateInfo));

	memset(wlan_sta_buf, 0, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));
	if ( getWlStaInfo(getWlanIfName(),	(WLAN_STA_INFO_Tp)wlan_sta_buf ) < 0 ) {
		printf("Read wlan sta info failed!\n");
		return -1;
	}
#endif

	memset(&entry, 0 , sizeof(MIB_LAN_HOST_BANDWIDTH_T));
	totalNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);

	for(i=0; i<totalNum; i++)
	{
		if (!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, i, (void *)&entry))
		{
			printf("[%s %d]get LAN_HOST_BANDWIDTHl chain entry error!\n", __func__, __LINE__);
			return -1;
		}

		macidx = -1;
	}


	if( ( !memcmp(oldInfo, lanPortCtlRateInfo, sizeof(lanPortCtlRateInfo)) )
#ifdef WLAN_SUPPORT
		&&( !memcmp(oldWifiInfo, wifiCtlRateInfo, sizeof(wifiCtlRateInfo)) )
#endif
		)
	{// lanPortCtlRateInfo and wifiCtlRateInfo not change
		//printf("[%s %d]:not change\n", __func__, __LINE__);
		return 0;
	}
	else
	{
		//printf("[%s %d]:change.\n", __func__, __LINE__);
		return 1;
	}

}

#ifdef WLAN_SUPPORT
/* 9607 support wifi client rate limit now */
void apply_wifiRateLimit(void)
{
	int i = 0;
	char cmdBuf[64];
	char macString[20];
	char *ifName = getWlanIfName();

	for(i=1; i<=MAX_STA_NUM; i++)
	{
		memset(cmdBuf, 0, 64);
		memset(macString, 0, 20);
		if( !isMacAddrInvalid(wifiCtlRateInfo[i].mac) )
		{
			sprintf(macString, "%2x%2x%2x%2x%2x%2x",wifiCtlRateInfo[i].mac[0],wifiCtlRateInfo[i].mac[1],wifiCtlRateInfo[i].mac[2],wifiCtlRateInfo[i].mac[3],wifiCtlRateInfo[i].mac[4],wifiCtlRateInfo[i].mac[5]);
			fillcharZeroToMacString(macString);
			sprintf(cmdBuf, "/bin/iwpriv %s sta_bw_control %s,%d,%d\n", ifName, macString, wifiCtlRateInfo[i].downRate, wifiCtlRateInfo[i].upRate);
			//printf("[%s %d]:MAC(%s), upRate(%d), downRate(%d)\n", __func__, __LINE__, macString, wifiCtlRateInfo[i].upRate, wifiCtlRateInfo[i].downRate);
			system(cmdBuf);
		}
	}
}

void clear_wifiRateLimit(void)
{
	char cmdBuf[64];
	char macString[20];
	char *ifName = getWlanIfName();

	MIB_LAN_HOST_BANDWIDTH_T entry;
	int i, totalNum;

	memset(wlan_sta_buf, 0, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));
	if ( getWlStaInfo(getWlanIfName(),	(WLAN_STA_INFO_Tp)wlan_sta_buf ) < 0 ) {
		printf("Read wlan sta info failed!\n");
		return;
	}
	memset(&entry, 0 , sizeof(MIB_LAN_HOST_BANDWIDTH_T));
	totalNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);

	for(i=0; i<totalNum; i++)
	{
		if (!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, i, (void *)&entry))
		{
			printf("[%s %d]get LAN_HOST_BANDWIDTHl chain entry error!\n", __func__, __LINE__);
			return;
		}

		/* not wifi client, continue */
		if( !isWifiMacOnline(entry.mac) )
			continue;

		memset(cmdBuf, 0, 64);
		memset(macString, 0, 20);
		sprintf(macString, "%2x%2x%2x%2x%2x%2x",entry.mac[0],entry.mac[1],entry.mac[2],entry.mac[3],entry.mac[4],entry.mac[5]);
		fillcharZeroToMacString(macString);
		sprintf(cmdBuf, "/bin/iwpriv %s sta_bw_control %s,0,0\n", ifName, macString);
		//printf("[%s %d]:MAC(%s), upRate(%d), downRate(%d)\n", __func__, __LINE__, macString, entry.maxUsBandwidth, entry.maxDsBandwidth);
		system(cmdBuf);
	}
}
#endif

void clear_lanPortRateLimit(void)
{
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_lanhost_rate_limit_clear();
#endif
}

void apply_lanPortRateLimit(void)
{
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_lanhost_rate_limit_apply();
#endif
}

void init_maxBandwidth(void)
{
	int ratelimit_trap_to_ps=0; //1: limit by TC otherwise:limit by FC shortcut
	unsigned char lanHostBandwidthControlEnable=0;
	char internetWanName[IFNAMSIZ]={0};
	MIB_CE_ATM_VC_T vcEntry;
	int totalVcEntry, i;
	char ifname[IFNAMSIZ] = {0};

	if(!mib_get_s(MIB_LANPORT_RATELIMIT_TRAP_TO_PS, (void *)&ratelimit_trap_to_ps, sizeof(ratelimit_trap_to_ps)))
	{
		printf("[%s %d]Get mib value MIB_LANPORT_RATELIMIT_TRAP_TO_PS failed!\n", __func__, __LINE__);
		return;
	}

	if(!mib_get_s(MIB_LANHOST_BANDWIDTH_CONTROL_ENABLE, (void *)&lanHostBandwidthControlEnable, sizeof(lanHostBandwidthControlEnable)))
	{
		printf("[%s %d]Get mib value MIB_LANHOST_BANDWIDTH_CONTROL_ENABLE failed!\n", __func__, __LINE__);
		return;
	}

	if(lanHostBandwidthControlEnable)
	{

		totalVcEntry = mib_chain_total(MIB_ATM_VC_TBL);

		for (i=0; i<totalVcEntry; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
			{
				continue;
			}
			ifGetName(vcEntry.ifIndex,ifname,sizeof(ifname));
			getInternetWANItfName(internetWanName,sizeof(internetWanName)-1);
			if(!strcmp(internetWanName, ifname) && strlen(internetWanName)){
				printf("%s %d internetWanName=%s\n",__FUNCTION__,__LINE__,internetWanName);
				//upload
				//tc qdisc del dev nas0_0 root 2>/dev/null
				va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", internetWanName, "root");
			}
		}
	}
}

void apply_maxBandwidth()
{
	unsigned char lanHostBandwidthControlEnable=0;

	mib_get_s(MIB_LANHOST_BANDWIDTH_CONTROL_ENABLE, (void *)&lanHostBandwidthControlEnable, sizeof(lanHostBandwidthControlEnable));

	reCalculateLanPortRateLimitInfo();
	if(lanHostBandwidthControlEnable)
	{
		clear_lanPortRateLimit();
		apply_lanPortRateLimit();
#ifdef WLAN_SUPPORT
		clear_wifiRateLimit();
		apply_wifiRateLimit();
#endif
	}
	else
	{
		clear_lanPortRateLimit();
#ifdef WLAN_SUPPORT
		clear_wifiRateLimit();
#endif
	}
}

void clear_maxBandwidth()
{
	clear_lanPortRateLimit();
#ifdef WLAN_SUPPORT
	clear_wifiRateLimit();
#endif
}
#endif

int getInternetWANItfName(char* devname,int len)
{
		int i =0;
		MIB_CE_ATM_VC_T vcEntry;
		int vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for(i=0;i<vcEntryNum; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry))
				continue;

			if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
				continue;
			if (!vcEntry.enable)
				continue;
			if (!((vcEntry.applicationtype&X_CT_SRV_INTERNET)))
				continue;
			ifGetName(vcEntry.ifIndex, devname, len);
			return 1;
		}
		return 0;

}

/*
	WAN-based rate limit
	return value is n means limit rate is n*512kbps,
	return value 0 means unlimit.
*/
unsigned int rtk_wan_speed_limit_get(int direction, unsigned int ifIndex)
{
#if defined(CONFIG_USER_IP_QOS) && !defined(_PRMT_X_CT_COM_DATA_SPEED_LIMIT_)
	unsigned int speedLimit = 0;
	MIB_CE_IP_TC_T  entry;
	int entry_num, i;

	entry_num = mib_chain_total(MIB_IP_QOS_TC_TBL);

	for(i=0; i<entry_num; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_TC_TBL, i, (void*)&entry))
			continue;

		if(entry.direction == direction
			&& entry.ifIndex == ifIndex
			&& entry.limitSpeed)
		{
			if(entry.limitSpeed < 512)
				speedLimit = 1;
			else
				speedLimit = entry.limitSpeed/512;

			break;
		}
	}

	return speedLimit;
#else
	return 0;
#endif
}


#if defined(CONFIG_USER_IP_QOS) && !defined(_PRMT_X_CT_COM_DATA_SPEED_LIMIT_)
/*
	WAN-based rate limit(downstream),
	return value is n means limit rate is n*512kbps,
	return value 0 means unlimit.
*/
unsigned int rtk_wan_downstream_speed_limit_get(MIB_CE_ATM_VC_Tp atm_entry)
{
	return rtk_wan_speed_limit_get(QOS_DIRECTION_DOWNSTREAM, atm_entry->ifIndex);
}

/*
	WAN-based rate limit(upstream),
	return value is n means limit rate is n*512kbps,
	return value 0 means unlimit.
*/
unsigned int rtk_wan_upstream_speed_limit_get(MIB_CE_ATM_VC_Tp atm_entry)
{
	return rtk_wan_speed_limit_get(QOS_DIRECTION_UPSTREAM, atm_entry->ifIndex);
}
#endif

#ifdef SUPPORT_SET_WANTYPE_FOR_STATIC_ROUTE
int getifindexByWanType(char *ifType, unsigned int *ifIndex)
{
	int i, total;
	char ifname[IFNAMSIZ]={0};
	MIB_CE_ATM_VC_T entry;
	struct in_addr inAddr;
	int flags = 0;

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, &entry)!=1)
			continue;

		if(strcmp(ifType, "INTERNET") == 0 && ((entry.applicationtype & X_CT_SRV_INTERNET) == 0))
			continue;

		if(strcmp(ifType, "IPTV") == 0 && ((entry.applicationtype & X_CT_SRV_OTHER) == 0 || entry.othertype != OTHER_IPTV_TYPE))
			continue;

		if(ifGetName( entry.ifIndex, ifname, IFNAMSIZ) == 0)
			continue;

		if (getInFlags( ifname, &flags) == 1 && flags & IFF_UP)
		{
			if (getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 1)
			{
				*ifIndex = entry.ifIndex;
				return 0;
			}
		}
	}

	return -1;
}
/*
int FindRelatedIfindex(unsigned int ifIndex, MIB_CE_IP_ROUTE_Tp pEntry)
{
	int ifindex = -1;

	if(pEntry == NULL)
		return 0;

	if (ifIndex == pEntry->ifIndex)
	{
		ifindex = getifindexByWanType(pEntry->itfType);
		if(pEntry->itfTypeEnable && strlen(pEntry->itfType) && (ifindex != ifIndex))
			pEntry->ifIndex = ifindex;
		return 1;
	}
	else if(pEntry->itfTypeEnable && strlen(pEntry->itfType))
	{
		ifindex = getifindexByWanType(pEntry->itfType);
		if(ifindex == ifIndex || ifindex == -1)
		{
			if(ifindex == -1)
				pEntry->ifIndex = -1;
			if(pEntry->ifIndex == -1)
				pEntry->ifIndex = ifindex;
			return 1;
		}
	}

	return 0;
}*/
#endif

#ifdef CONFIG_PPP
#ifdef CONFIG_USER_PPPD
static void write_to_ppp_options(MIB_CE_ATM_VC_Tp pEntry)
{
	int fdoptions;
	char buff[64]={0};
	char option_name[32]={0}, tmp[8]={0};
	DIR* dir=NULL;
#ifdef CONFIG_IPV6
	int i;
	unsigned char meui64[8];
	unsigned char Ipv6AddrStr[INET6_ADDRSTRLEN];
	struct in6_addr ip6Addr;
#endif

	/**********************************************************************/
	/* Write PPP options file												   */
	/**********************************************************************/

	if(!pEntry){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return;
	}

	dir = opendir(PPPD_PEERS_FOLDER);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_PEERS_FOLDER);
	}

	ifGetName(pEntry->ifIndex, tmp, sizeof(tmp));
	sprintf(option_name, "%s/opts_%s", PPPD_PEERS_FOLDER, tmp);
	fdoptions = open(option_name, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);

	if (fdoptions == -1) {
		printf("Create %s file error!\n", option_name);
		return;
	}

	printf("Create %s file OK!\n", option_name);

	if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE) {
		snprintf(buff, 64, "plugin %s", PPPOE_PLUGIN);
		WRITE_PPP_FILE(fdoptions, buff);
		/* https://fossies.org/linux/rp-pppoe/doc/KERNEL-MODE-PPPOE
		 * PPPoE Kernel Mode support following plugin parameters:
		 *		ethXXX
		 *		brXXX
		 *		nic-XXXX
		 * 		rp_pppoe_service SERVICE_NAME
		 *		rp_pppoe_ac NAME
		 *		rp_pppoe_verbose 0|1
		 *		rp_pppoe_sess nnnn:aa:bb:cc:dd:ee:ff
		 *		rp_pppoe_mac aa:bb:cc:dd:ee:ff
		 */
		if(pEntry->pppServiceName[0]){
			snprintf(buff, 64, " rp_pppoe_service %s", pEntry->pppServiceName);
			WRITE_PPP_FILE(fdoptions, buff);
		}
		if(pEntry->pppACName[0]){
			snprintf(buff, 64, " rp_pppoe_ac %s", pEntry->pppACName);
			WRITE_PPP_FILE(fdoptions, buff);
		}
		snprintf(buff, 64, "\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}
#ifdef CONFIG_DEV_xDSL
	else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOA) {
		if (pEntry->encap == ENCAP_VCMUX)
			snprintf(buff, 64, "vc-encaps\n");
		else if (pEntry->encap == ENCAP_LLC)
			snprintf(buff, 64, "llc-encaps\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "plugin %s\n", PPPOA_PLUGIN);
		WRITE_PPP_FILE(fdoptions, buff);
	}
#endif
	snprintf(buff, 64, "nodetach\n");
	WRITE_PPP_FILE(fdoptions, buff);
	snprintf(buff, 64, "user %s\n", pEntry->pppUsername);
	WRITE_PPP_FILE(fdoptions, buff);

	if (pEntry->pppCtype == CONTINUOUS) // Continuous
		snprintf(buff, 64, "persist\n");
	else if (pEntry->pppCtype == CONNECT_ON_DEMAND) { // On-demand
		snprintf(buff, 64, "demand\nidle %u\n", pEntry->pppIdleTime);
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "defaultroute\n");
	}
	WRITE_PPP_FILE(fdoptions, buff);

	if (pEntry->pppCtype == CONTINUOUS || pEntry->pppCtype == CONNECT_ON_DEMAND) {
		snprintf(buff, 64, "holdoff 5\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}

#ifdef CONFIG_IPV6
	if (pEntry->IpProtocol == IPVER_IPV6)
	{
		snprintf(buff, 64, "noip\n");	// IPv6 Only
		WRITE_PPP_FILE(fdoptions, buff);
	}else if (pEntry->IpProtocol== IPVER_IPV4)
#endif
	{
		snprintf(buff, 64, "noipv6\n");	// IPv4 Only
		WRITE_PPP_FILE(fdoptions, buff);
	}

	snprintf(buff, 64, "mru %u\n", pEntry->mtu);
	WRITE_PPP_FILE(fdoptions, buff);

	if (pEntry->pppLcpEcho) {
		snprintf(buff, 64, "lcp-echo-interval %u\n", pEntry->pppLcpEcho);
		WRITE_PPP_FILE(fdoptions, buff);
	}
	if (pEntry->pppLcpEchoRetry) {
		snprintf(buff, 64, "lcp-echo-failure %u\n", pEntry->pppLcpEchoRetry);
		WRITE_PPP_FILE(fdoptions, buff);
	}
	snprintf(buff, 64, "noipdefault\n");
	WRITE_PPP_FILE(fdoptions, buff);

#ifdef CONFIG_IPV6
	if (pEntry->IpProtocol == IPVER_IPV6 || pEntry->IpProtocol == IPVER_IPV4_IPV6) {
		snprintf(buff, 64, "+ipv6\n");
		WRITE_PPP_FILE(fdoptions, buff);
		/* local interface identifier */
		mac_meui64(pEntry->MacAddr, meui64);
		memset(&ip6Addr, 0, sizeof(ip6Addr));
		for (i=0; i<8; i++)
			ip6Addr.s6_addr[i+8] = meui64[i];
		inet_ntop(PF_INET6, &ip6Addr, Ipv6AddrStr, sizeof(Ipv6AddrStr));
		if (pEntry->pppCtype == CONNECT_ON_DEMAND) { // On-demand
			/* remote LL address required for demand-dialling  */
			snprintf(buff, 64, "ipv6 %s,::123\n", Ipv6AddrStr);
			WRITE_PPP_FILE(fdoptions, buff);
		}
		else {
			/* Set the local interface identifier. */
			snprintf(buff, 64, "ipv6 %s\n", Ipv6AddrStr);
			WRITE_PPP_FILE(fdoptions, buff);
		}
	}
#endif

	if (pEntry->pppAuth == PPP_AUTH_PAP) {
		snprintf(buff, 64, "refuse-chap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-mschap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-mschap-v2\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}
	else if (pEntry->pppAuth == PPP_AUTH_CHAP) {
		snprintf(buff, 64, "refuse-pap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-mschap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-mschap-v2\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}
	else if (pEntry->pppAuth == PPP_AUTH_MSCHAP) {
		snprintf(buff, 64, "refuse-pap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-chap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-mschap-v2\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}
	else if (pEntry->pppAuth == PPP_AUTH_MSCHAPV2) {
		snprintf(buff, 64, "refuse-pap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-chap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-mschap\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}

	snprintf(buff, 64, "usepeerdns\n");
	WRITE_PPP_FILE(fdoptions, buff);

	snprintf(buff, 64, "maxfail 0\n");
	WRITE_PPP_FILE(fdoptions, buff);

	if (pEntry->pppDebug == 1) {
		snprintf(buff, 64, "debug\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "dump\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}
	if (isDefaultRouteWan(pEntry)) {
		snprintf(buff, 64, "defaultroute\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}

#ifdef CONFIG_PPP_MULTILINK
	snprintf(buff, 64, "multilink\n");
	WRITE_PPP_FILE(fdoptions, buff);
#endif

	close(fdoptions);
}

#if defined(CONFIG_SUPPORT_AUTO_DIAG)
void write_to_simu_ppp_options(MIB_CE_ATM_VC_Tp pEntry)
{
	int fdoptions;
	char buff[64]={0};
	char option_name[32]={0}, tmp[8]={0};
	DIR* dir=NULL;

	/**********************************************************************/
	/* Write PPP options file												   */
	/**********************************************************************/

	if(!pEntry){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return;
	}

	dir = opendir(PPPD_PEERS_FOLDER);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_PEERS_FOLDER);
	}

	ifGetName(pEntry->ifIndex, tmp, sizeof(tmp));
	sprintf(option_name, "%s/opts_simu_%s", PPPD_PEERS_FOLDER, tmp);
	fdoptions = open(option_name, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);

	if (fdoptions == -1) {
		printf("Create %s file error!\n", option_name);
		return;
	}

	printf("Create %s file OK!\n", option_name);
	if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE) {
		snprintf(buff, 64, "plugin %s\n", PPPOE_PLUGIN);
		WRITE_PPP_FILE(fdoptions, buff);
	}
#ifdef CONFIG_DEV_xDSL
	else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOA) {
		if (pEntry->encap == ENCAP_VCMUX)
			snprintf(buff, 64, "vc-encaps\n");
		else if (pEntry->encap == ENCAP_LLC)
			snprintf(buff, 64, "llc-encaps\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "plugin %s\n", PPPOA_PLUGIN);
		WRITE_PPP_FILE(fdoptions, buff);
	}
#endif
	snprintf(buff, 64, "nodetach\n");
	WRITE_PPP_FILE(fdoptions, buff);
	snprintf(buff, 64, "user %s\n", pEntry->pppUsername);
	WRITE_PPP_FILE(fdoptions, buff);
	snprintf(buff, 64, "password %s\n", pEntry->pppPassword);
	WRITE_PPP_FILE(fdoptions, buff);

	if (pEntry->pppCtype == CONTINUOUS) // Continuous
		snprintf(buff, 64, "persist\n");
	else if (pEntry->pppCtype == CONNECT_ON_DEMAND) { // On-demand
		snprintf(buff, 64, "demand\nidle %u\n", pEntry->pppIdleTime);
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "defaultroute\n");
	}
	WRITE_PPP_FILE(fdoptions, buff);

	if (pEntry->pppCtype == CONTINUOUS || pEntry->pppCtype == CONNECT_ON_DEMAND) {
		snprintf(buff, 64, "holdoff 5\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}

#ifdef CONFIG_IPV6
	if (pEntry->IpProtocol == IPVER_IPV6)
	{
		snprintf(buff, 64, "noip\n");	// IPv6 Only
		WRITE_PPP_FILE(fdoptions, buff);
	}else if (pEntry->IpProtocol== IPVER_IPV4)
#endif
	{
		snprintf(buff, 64, "noipv6\n");	// IPv4 Only
		WRITE_PPP_FILE(fdoptions, buff);
	}

	snprintf(buff, 64, "mru %u\n", pEntry->mtu);
	WRITE_PPP_FILE(fdoptions, buff);

	if (pEntry->pppLcpEcho) {
		snprintf(buff, 64, "lcp-echo-interval %u\n", pEntry->pppLcpEcho);
		WRITE_PPP_FILE(fdoptions, buff);
	}
	if (pEntry->pppLcpEchoRetry) {
		snprintf(buff, 64, "lcp-echo-failure %u\n", pEntry->pppLcpEchoRetry);
		WRITE_PPP_FILE(fdoptions, buff);
	}
	snprintf(buff, 64, "noipdefault\n");
	WRITE_PPP_FILE(fdoptions, buff);

#ifdef CONFIG_IPV6
	if (pEntry->IpProtocol == IPVER_IPV6 || pEntry->IpProtocol == IPVER_IPV4_IPV6) {
		snprintf(buff, 64, "+ipv6\n");
		WRITE_PPP_FILE(fdoptions, buff);
		if (pEntry->pppCtype == CONNECT_ON_DEMAND) { // On-demand
			/* remote LL address required for demand-dialling  */
			snprintf(buff, 64, "ipv6 ,::123\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}
	}
#endif

	if (pEntry->pppAuth == PPP_AUTH_PAP) {
		snprintf(buff, 64, "refuse-chap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-mschap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-mschap-v2\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}
	else if (pEntry->pppAuth == PPP_AUTH_CHAP) {
		snprintf(buff, 64, "refuse-pap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-mschap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-mschap-v2\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}
	else if (pEntry->pppAuth == PPP_AUTH_MSCHAP) {
		snprintf(buff, 64, "refuse-pap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-chap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-mschap-v2\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}
	else if (pEntry->pppAuth == PPP_AUTH_MSCHAPV2) {
		snprintf(buff, 64, "refuse-pap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-chap\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "refuse-mschap\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}

	snprintf(buff, 64, "usepeerdns\n");
	WRITE_PPP_FILE(fdoptions, buff);

	snprintf(buff, 64, "maxfail 1\n");
	WRITE_PPP_FILE(fdoptions, buff);

	if (pEntry->pppDebug == 1) {
		snprintf(buff, 64, "debug\n");
		WRITE_PPP_FILE(fdoptions, buff);
		snprintf(buff, 64, "dump\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}
	if (isDefaultRouteWan(pEntry)) {
		snprintf(buff, 64, "defaultroute\n");
		WRITE_PPP_FILE(fdoptions, buff);
	}

#ifdef CONFIG_PPP_MULTILINK
	snprintf(buff, 64, "multilink\n");
	WRITE_PPP_FILE(fdoptions, buff);
#endif

	snprintf(buff, 64, "simulation-ppp\n");
	WRITE_PPP_FILE(fdoptions, buff);

	close(fdoptions);
}
#endif //CONFIG_SUPPORT_AUTO_DIAG

#ifdef CONFIG_USER_XL2TPD
/*
 *  generate l2tp init script
 *  this script called after ppp device create, and ppp daemon will wait script finish
 *  return:
 *    -1: Fail
 *     0: Success
 */
static int generate_l2tp_init_script(MIB_L2TP_Tp pEntry)
{
	FILE *fp=NULL;
	char wanif[IFNAMSIZ]={0}, script_path[128]={0};
	DIR* dir=NULL;

	if(!pEntry){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return -1;
	}

	dir = opendir(PPPD_INIT_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_INIT_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));

	if (PPP_INDEX(pEntry->ifIndex) != DUMMY_PPP_INDEX) {
		/* ppp initial script */
		snprintf(script_path, sizeof(script_path), "%s/"PPPD_INIT_SCRIPT_NAME, PPPD_INIT_SCRIPT_DIR, wanif);
		if (fp = fopen(script_path, "w+")) {
			fprintf(fp, "#!/bin/sh\n\n");

			fprintf(fp, "######### Script Globle Variable ###########\n");
			fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
			fprintf(fp, "PIDFILE=%s/${IFNAME}.pid\n", (char *)PPPD_CONFIG_FOLDER);
			fprintf(fp, "BACKUP_PIDFILE=%s/${IFNAME}.pid_back\n", (char *)PPPD_CONFIG_FOLDER);
			fprintf(fp, "######### Script Globle Variable ###########\n\n");

			fprintf(fp, "## PID file be remove when PPP link_terminated\n");
			fprintf(fp, "## it will cause delete ppp daemon fail, So backup it.\n");
			fprintf(fp, "if [ -f ${PIDFILE} ]; then\n");
				fprintf(fp, "\tcp -af ${PIDFILE} ${BACKUP_PIDFILE}\n");
			fprintf(fp, "fi\n");

			fclose(fp);
			if(chmod(script_path, 484) != 0)
				printf("chmod failed: %s %d\n", __func__, __LINE__);
		}else{
			fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, script_path);
			goto Error;
		}
	}
	else {
		fprintf(stderr, "[%s] Error: DUMMY_PPP_INDEX\n.", __FUNCTION__);
		goto Error;
	}

	return 0;
Error:
	return -1;
}

/*
 *	generate pppd ipcp/ipcpv6 up script for WAN interface
 *  return:
 *    -1: Fail
 *     0: Success
 */
static int generate_l2tp_ipup_script(MIB_L2TP_Tp pEntry)
{
	int ret=0, len=0;
	FILE *fp=NULL;
	char wanif[IFNAMSIZ]={0}, ipup_path[128]={0};
	DIR* dir=NULL;

	if(!pEntry){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return -1;
	}

	dir = opendir(PPPD_V4_UP_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_V4_UP_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));

	if (PPP_INDEX(pEntry->ifIndex) != DUMMY_PPP_INDEX) {
		/* IPv4 script */
		snprintf(ipup_path, sizeof(ipup_path), "%s/"PPPD_V4_UP_SCRIPT_NAME, PPPD_V4_UP_SCRIPT_DIR, wanif);
		if (fp = fopen(ipup_path, "w+")) {
			fprintf(fp, "#!/bin/sh\n\n");

			fprintf(fp, "######### Script Globle Variable ###########\n");
			fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
			fprintf(fp, "######### Script Globle Variable ###########\n\n");

			// create dns resolv.conf.{ifname} file for dnsmasq
			fprintf(fp, "rm %s/resolv.conf.${IFNAME}\n", (char *)PPPD_CONFIG_FOLDER);
			fprintf(fp, "if [[ ${DNS1} != \"\" ]]; then echo \"${DNS1}@${IPLOCAL}\" >> %s/resolv.conf.${IFNAME}; fi\n", (char *)PPPD_CONFIG_FOLDER);
			fprintf(fp, "if [[ ${DNS2} != \"\" ]] && [[ ${DNS1} != ${DNS2} ]]; then echo \"${DNS2}@${IPLOCAL}\" >> %s/resolv.conf.${IFNAME}; fi\n\n", (char *)PPPD_CONFIG_FOLDER);

			//iptables -t nat -A address_map_tbl -o ppp11 -j MASQUERADE
			fprintf(fp, "iptables -t nat -A %s -o ${IFNAME} -j MASQUERADE\n\n", NAT_ADDRESS_MAP_TBL);

			if(pEntry->dgw){
				fprintf(fp, "# search route table and change default route as lower preference\n");
				fprintf(fp, "sysconf send_unix_sock_message %s do_update_vpn_def_rt ${IFNAME}\n\n", SYSTEMD_USOCK_SERVER);
			}

			fclose(fp);
			if(chmod(ipup_path, 484) != 0)
				printf("chmod failed: %s %d\n", __func__, __LINE__);
		}else{
			fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, ipup_path);
			goto Error;
		}
	}
	else {
		fprintf(stderr, "[%s] Error: DUMMY_PPP_INDEX\n.", __FUNCTION__);
		goto Error;
	}

	return ret;
Error:
	return -1;
}

/*
 *	generate pppd ipcp/ipcpv6 down script for WAN interface
 *  return:
 *    -1: Fail
 *     0: Success
 */
static int generate_l2tp_ipdown_script(MIB_L2TP_Tp pEntry)
{
	int ret=0;
	FILE *fp=NULL;
	char wanif[IFNAMSIZ]={0}, ipdown_path[128]={0};
	unsigned char vChar=0;
	DIR* dir=NULL;

	if(!pEntry){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return -1;
	}

	dir = opendir(PPPD_V4_DOWN_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_V4_DOWN_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	if (PPP_INDEX(pEntry->ifIndex) != DUMMY_PPP_INDEX) {
		/* IPv4 script */
		snprintf(wanif, sizeof(wanif), "ppp%u", PPP_INDEX(pEntry->ifIndex));
		snprintf(ipdown_path, sizeof(ipdown_path), "%s/"PPPD_V4_DOWN_SCRIPT_NAME, PPPD_V4_DOWN_SCRIPT_DIR, wanif);
		if (fp = fopen(ipdown_path, "w+")) {
			fprintf(fp, "#!/bin/sh\n\n");

			fprintf(fp, "######### Script Globle Variable ###########\n");
			fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
			fprintf(fp, "######### Script Globle Variable ###########\n\n");

			// clean dns resolv.conf.{ifname} file
			fprintf(fp, "rm %s/resolv.conf.${IFNAME}\n\n", (char *)PPPD_CONFIG_FOLDER);

			//iptables -t nat -D address_map_tbl -o ppp11 -j MASQUERADE
			fprintf(fp, "iptables -t nat -D %s -o ${IFNAME} -j MASQUERADE\n\n", NAT_ADDRESS_MAP_TBL);

			if(pEntry->dgw){
				// recover default route
				fprintf(fp, "%s/"PPPD_VPN_DOWN_SCRIPT_NAME"\n", (char *)PPPD_VPN_SCRIPT_DIR, wanif);
			}

			fclose(fp);
			if(chmod(ipdown_path, 484) != 0)
				printf("chmod failed: %s %d\n", __func__, __LINE__);
		}else{
			fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, ipdown_path);
			goto Error;
		}
	}
	else {
		fprintf(stderr, "[%s] Error: DUMMY_PPP_INDEX\n.", __FUNCTION__);
		goto Error;
	}

	return ret;
Error:
	return -1;
}

#define L2TP_PORT 1701
int connect_xl2tpd(unsigned int pppindex)
{
	char buff[64];
	char pidfile[64], back_pidfile[64];

	/* check ppp interface before start lac connection */
	snprintf(pidfile, sizeof(pidfile), "%s/ppp%u.pid", PPPD_CONFIG_FOLDER, pppindex);
	snprintf(back_pidfile, sizeof(back_pidfile), "%s/ppp%u.pid_back", PPPD_CONFIG_FOLDER, pppindex);
	fprintf(stderr, "[%s] pidfile=%s\n", __FUNCTION__, pidfile);
	if (read_pid(pidfile) <= 0){
		kill_by_pidfile(pidfile, SIGTERM);
	}else if(read_pid(back_pidfile) > 0){
		/* PID file be remove when PPP link_terminated
		 * it will cause delete ppp daemon fail, So backup it on ipcp/ipv6cp up script. */
		kill_by_pidfile(back_pidfile, SIGTERM);
	}

	sprintf(buff,"echo \"c connect%u\" > /var/run/xl2tpd/l2tp-control", pppindex);
	printf("%s\n", buff);
	system(buff);

	return 0;
}

void write_to_l2tp_config(void)
{
	int fdconf, fdl2tp, i;
	char buff[64];
	unsigned int l2tp_entry_num = 0, pppindex;
	MIB_L2TP_T l2tp_entry;
	struct in_addr server_ip;
	unsigned int l2tp_port = L2TP_PORT;
	char ifname[IFNAMSIZ];

	/**********************************************************************/
	/* Write PAP/CHAP secrets file                                        */
	/**********************************************************************/
	rtk_create_ppp_secrets();

	/**********************************************************************/
	/* Write L2TP secrets file												   */
	/**********************************************************************/
	fdl2tp = open("/etc/xl2tpd/l2tp-secrets", O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
	if (fdl2tp == -1) {
		printf("Create /etc/xl2tpd/l2tp-secrets file error!\n");
		return;
	}

	/**********************************************************************/
	/* Write L2TP config / option file												   */
	/**********************************************************************/

	fdconf = open("/etc/xl2tpd/xl2tpd.conf", O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);

	if (fdconf == -1) {
		printf("Create /etc/xl2tpd/xl2tpd.conf file error!\n");
		return;
	}
	snprintf(buff, sizeof(buff), "[global]\n");
	WRITE_PPP_FILE(fdconf, buff);
	snprintf(buff, sizeof(buff), "port = %u\n", l2tp_port);
	WRITE_PPP_FILE(fdconf, buff);

	snprintf(buff, sizeof(buff), "auth file = /etc/xl2tpd/l2tp-secrets\n");
	WRITE_PPP_FILE(fdconf, buff);

	snprintf(buff, sizeof(buff), "debug tunnel = yes\n");
	WRITE_PPP_FILE(fdconf, buff);
	snprintf(buff, sizeof(buff), "debug avp = yes\n");
	WRITE_PPP_FILE(fdconf, buff);

	l2tp_entry_num = mib_chain_total(MIB_L2TP_TBL);

	for (i=0 ; i<l2tp_entry_num ; i++) {
		int fdoptions;
		char xl2tpdopt_str[32];
		if (!mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tp_entry))
			continue;

		pppindex = PPP_INDEX(l2tp_entry.ifIndex);
		snprintf(xl2tpdopt_str, sizeof(xl2tpdopt_str), "/etc/ppp/options%u.xl2tpd", pppindex);
		fdoptions = open(xl2tpdopt_str, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
		if (fdoptions == -1) {
			printf("Create %s file error!\n", xl2tpdopt_str);
			return;
		}

		snprintf(buff, sizeof(buff), "unit %u\n", pppindex);
		WRITE_PPP_FILE(fdoptions, buff);

		//snprintf(buff, 64, "plugin %s\n", XL2TP_PLUGIN);
		//WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, sizeof(buff), "[lac connect%u]\n", pppindex);
		WRITE_PPP_FILE(fdconf, buff);

		snprintf(buff, sizeof(buff), "lns = %s\n", l2tp_entry.server);
		WRITE_PPP_FILE(fdconf, buff);

		snprintf(buff, sizeof(buff), "pppoptfile =%s\n", xl2tpdopt_str);
		WRITE_PPP_FILE(fdconf, buff);

		if (l2tp_entry.authtype == AUTH_PAP) {
			snprintf(buff, sizeof(buff), "refuse-chap\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse-mschap\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse-mschap-v2\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse chap = yes\n");
			WRITE_PPP_FILE(fdconf, buff);
		}
		else if (l2tp_entry.authtype == AUTH_CHAP) {
			snprintf(buff, sizeof(buff), "refuse-pap\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse-mschap\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse-mschap-v2\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse pap = yes\n");
			WRITE_PPP_FILE(fdconf, buff);
		}
		else if (l2tp_entry.authtype == AUTH_CHAPMSV2) {
			snprintf(buff, sizeof(buff), "refuse-pap\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse-chap\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse-mschap\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse pap = yes\n");
			WRITE_PPP_FILE(fdconf, buff);
		}

		snprintf(buff, sizeof(buff), "refuse-eap\n");
		WRITE_PPP_FILE(fdoptions, buff);

		if (l2tp_entry.enctype == VPN_ENCTYPE_MPPE) {
			snprintf(buff, sizeof(buff), "mppe required\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "-mppc\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}
		else if (l2tp_entry.enctype == VPN_ENCTYPE_MPPC) {
			snprintf(buff, sizeof(buff), "-mppe\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "+mppc\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}
		else if (l2tp_entry.enctype == VPN_ENCTYPE_BOTH) {
			snprintf(buff, sizeof(buff), "mppe required\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "+mppc\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}

		snprintf(buff, sizeof(buff), "noipdefault\n");
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, sizeof(buff), "nodetach\n");
		WRITE_PPP_FILE(fdoptions, buff);

		//snprintf(buff, 64, "ppp debug = yes\n");
		//WRITE_PPP_FILE(fdconf, buff);

		if (l2tp_entry.conntype == CONTINUOUS || l2tp_entry.conntype == CONNECT_ON_DEMAND) {
			snprintf(buff, sizeof(buff), "redial = yes\n");
			WRITE_PPP_FILE(fdconf, buff);
			snprintf(buff, sizeof(buff), "redial timeout = 1\n");
			WRITE_PPP_FILE(fdconf, buff);
		}
		if (l2tp_entry.conntype == CONNECT_ON_DEMAND) {
			snprintf(buff, sizeof(buff), "demand\nidle %u\n", l2tp_entry.idletime * 60);
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "holdoff 5\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}

		if (l2tp_entry.dgw == 1 || l2tp_entry.conntype == CONNECT_ON_DEMAND) {
			snprintf(buff, sizeof(buff), "defaultroute\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}

#ifdef CONFIG_IPV6_VPN
		if (l2tp_entry.IpProtocol == IPVER_IPV6)
		{
			snprintf(buff, sizeof(buff), "noip\n");	// IPv6 Only
			WRITE_PPP_FILE(fdoptions, buff);
		}else if (l2tp_entry.IpProtocol== IPVER_IPV4)
#endif
		{
			snprintf(buff, sizeof(buff), "noipv6\n"); // IPv4 Only
			WRITE_PPP_FILE(fdoptions, buff);
		}

		snprintf(buff, sizeof(buff), "lcp-echo-failure 10\n");
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, sizeof(buff), "lcp-echo-interval 10\n");
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, sizeof(buff), "mtu %u\n", l2tp_entry.mtu);
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, sizeof(buff), "name %s\n", l2tp_entry.username);
		WRITE_PPP_FILE(fdoptions, buff);

		ifGetName(l2tp_entry.ifIndex, ifname, IFNAMSIZ);
		snprintf(buff, sizeof(buff), "remotename %s\n", ifname);
		WRITE_PPP_FILE(fdoptions, buff);

		if (l2tp_entry.tunnel_auth) {
			snprintf(buff, sizeof(buff), "challenge = yes\n");
			WRITE_PPP_FILE(fdconf, buff);

			snprintf(buff, sizeof(buff), "* * %s\n", l2tp_entry.secret);
			WRITE_PPP_FILE(fdl2tp, buff);
		}

		//snprintf(buff, sizeof(buff), "debug\n");
		//WRITE_PPP_FILE(fdoptions, buff);
		//snprintf(buff, sizeof(buff), "logfile /dev/console\n");
		//WRITE_PPP_FILE(fdoptions, buff);
		//snprintf(buff, sizeof(buff), "dump\n");
		//WRITE_PPP_FILE(fdoptions, buff);

		close(fdoptions);

		generate_l2tp_init_script(&l2tp_entry);
		generate_l2tp_ipup_script(&l2tp_entry);
		generate_l2tp_ipdown_script(&l2tp_entry);
	}
	close(fdl2tp);
	close(fdconf);

	/**********************************************************************/
	/* Write PPP option file												   */
	/**********************************************************************/

	int fdoptions = open("/etc/ppp/options", O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
	if (fdoptions == -1) {
		printf("Create /etc/ppp/options file error!\n");
		return;
	}
	snprintf(buff, sizeof(buff), "debug\n");
	WRITE_PPP_FILE(fdoptions, buff);
	close(fdoptions);
}

#ifdef CONFIG_USER_L2TPD_LNS
void write_to_l2tp_lns_config()
{
	int fdchap, fdpap, fdconf, fdl2tp;
	char buff[64];
	MIB_VPND_T server;
	MIB_VPN_ACCOUNT_T entry;
	unsigned int l2tp_entry_num = 0, l2tp_account_entry_num=0, pppindex=L2TP_VPN_STRAT;
	int i=0, j=0;
	struct in_addr addr, endaddr;
	char str_ipaddr[32], str_endipaddr[32];
	unsigned int l2tp_port = L2TP_PORT;

	/**********************************************************************/
	/* Write PAP/CHAP secrets file                                        */
	/**********************************************************************/
	rtk_create_ppp_secrets();

	/**********************************************************************/
	/* Write L2TP secrets file												   */
	/**********************************************************************/
	fdl2tp = open("/etc/xl2tpd/l2tp-secrets", O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
	if (fdl2tp == -1) {
		printf("Create /etc/xl2tpd/l2tp-secrets file error!\n");
		return;
	}

	/**********************************************************************/
	/* Write L2TP config / option file												   */
	/**********************************************************************/

	fdconf = open("/etc/xl2tpd/xl2tpd.conf", O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);

	if (fdconf == -1) {
		printf("Create /etc/xl2tpd/xl2tpd.conf file error!\n");
		return;
	}
	snprintf(buff, 64, "[global]\n");
	WRITE_PPP_FILE(fdconf, buff);

	snprintf(buff, 64, "port = %u\n", l2tp_port);
	WRITE_PPP_FILE(fdconf, buff);

	snprintf(buff, 64, "auth file = /etc/xl2tpd/l2tp-secrets\n");
	WRITE_PPP_FILE(fdconf, buff);

	snprintf(buff, 64, "debug tunnel = yes\n");
	WRITE_PPP_FILE(fdconf, buff);
	snprintf(buff, 64, "debug avp = yes\n");
	WRITE_PPP_FILE(fdconf, buff);

	l2tp_entry_num = mib_chain_total(MIB_VPN_SERVER_TBL);

	for (i=0 ; i<l2tp_entry_num ; i++) {
		int fdoptions;
		char xl2tpdopt_str[32]={0};
		if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, (void *)&server))
			continue;

		if(server.type != VPN_L2TP)
			continue;

		snprintf(xl2tpdopt_str, 32, "/etc/ppp/options%u.xl2tpd", pppindex);
		fdoptions = open(xl2tpdopt_str, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
		if (fdoptions == -1) {
			printf("Create %s file error!\n", xl2tpdopt_str);
			return;
		}

		snprintf(buff, 64, "unit %u\n", pppindex);
		WRITE_PPP_FILE(fdoptions, buff);

		//snprintf(buff, 64, "plugin %s\n", XL2TP_PLUGIN);
		//WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, 64, "[lns default]\n");
		WRITE_PPP_FILE(fdconf, buff);
		addr.s_addr = server.peeraddr;
		snprintf(str_ipaddr, sizeof(str_ipaddr), "%s", inet_ntoa(addr));
		//if endaddr has no value, make endaddr same as peeraddr subnet
		if(server.endaddr == 0){
			server.endaddr = ntohl((htonl(server.peeraddr)|0xff)-1); //192.168.1."254"
		}

		//if peeraddr is no the same with endaddr, we reserved endaddr as LNS server local ip addres
#if !defined(CONFIG_CMCC_ENTERPRISE)
		if(server.peeraddr != server.endaddr){
			endaddr.s_addr = ntohl(htonl(server.endaddr)-1);
		}
		else
#endif
		{
			endaddr.s_addr = server.endaddr;
		}
		snprintf(str_endipaddr, sizeof(str_endipaddr), "%s", inet_ntoa(endaddr));
		snprintf(buff, 64, "ip range = %s-%s\n", str_ipaddr, str_endipaddr);
		WRITE_PPP_FILE(fdconf, buff);

		//if endaddr has no value, make endaddr same as peeraddr subnet
#if defined(CONFIG_CMCC_ENTERPRISE)
		endaddr.s_addr = server.localaddr;
#else
		endaddr.s_addr = server.endaddr;
#endif
		snprintf(str_endipaddr, sizeof(str_endipaddr), "%s", inet_ntoa(endaddr));
		snprintf(buff, 64, "local ip = %s\n", str_endipaddr);
		WRITE_PPP_FILE(fdconf, buff);

		snprintf(buff, 64, "pppoptfile =%s\n", xl2tpdopt_str);
		WRITE_PPP_FILE(fdconf, buff);

		snprintf(buff, 64, "length bit = yes\n");
		WRITE_PPP_FILE(fdconf, buff);

		if (server.authtype != 4) {
			snprintf(buff, 64, "auth\n");
			WRITE_PPP_FILE(fdoptions, buff);

			if (server.authtype == AUTH_PAP) {
				snprintf(buff, 64, "require-pap\n");
				WRITE_PPP_FILE(fdoptions, buff);
			}
			else if (server.authtype == AUTH_CHAP) {
				snprintf(buff, 64, "require-chap\n");
				WRITE_PPP_FILE(fdoptions, buff);
			}
			else if (server.authtype == AUTH_CHAPMSV2) {
				snprintf(buff, 64, "require-mschap-v2\n");
				WRITE_PPP_FILE(fdoptions, buff);
			}

			if (server.enctype == VPN_ENCTYPE_MPPE) {
				snprintf(buff, sizeof(buff), "mppe required\n");
				WRITE_PPP_FILE(fdoptions, buff);
				snprintf(buff, sizeof(buff), "-mppc\n");
				WRITE_PPP_FILE(fdoptions, buff);
			}
			else if (server.enctype == VPN_ENCTYPE_MPPC) {
				snprintf(buff, sizeof(buff), "-mppe\n");
				WRITE_PPP_FILE(fdoptions, buff);
				snprintf(buff, sizeof(buff), "+mppc\n");
				WRITE_PPP_FILE(fdoptions, buff);
			}
			else if (server.enctype == VPN_ENCTYPE_BOTH) {
				snprintf(buff, sizeof(buff), "mppe required\n");
				WRITE_PPP_FILE(fdoptions, buff);
				snprintf(buff, sizeof(buff), "+mppc\n");
				WRITE_PPP_FILE(fdoptions, buff);
			}
		}

		snprintf(buff, 64, "refuse-eap\n");
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, 64, "noipdefault\n");
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, 64, "nodetach\n");
		WRITE_PPP_FILE(fdoptions, buff);

		//snprintf(buff, 64, "ppp debug = yes\n");
		//WRITE_PPP_FILE(fdconf, buff);
/*
		if (l2tp_entry.conntype == 0 || l2tp_entry.conntype == 1) {
			snprintf(buff, 64, "redial = yes\n");
			WRITE_PPP_FILE(fdconf, buff);
			snprintf(buff, 64, "redial timeout = 1\n");
			WRITE_PPP_FILE(fdconf, buff);
		}
		if (l2tp_entry.conntype == CONNECT_ON_DEMAND) {
			snprintf(buff, 64, "demand\nidle %u\n", l2tp_entry.idletime * 60);
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, 64, "holdoff 5\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}

		if (l2tp_entry.dgw == 1 || l2tp_entry.conntype == CONNECT_ON_DEMAND) {
			snprintf(buff, 64, "defaultroute\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}

#ifdef CONFIG_IPV6_VPN
		if (l2tp_entry.IpProtocol == IPVER_IPV6)
		{
			snprintf(buff, 64, "noip\n");	// IPv6 Only
			WRITE_PPP_FILE(fdoptions, buff);
		}else if (l2tp_entry.IpProtocol== IPVER_IPV4)
#endif
		*/
		{
			snprintf(buff, 64, "noipv6\n"); // IPv4 Only
			WRITE_PPP_FILE(fdoptions, buff);
		}

		snprintf(buff, 64, "mtu 1200\n");
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, 64, "name %s\n", server.name);
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, sizeof(buff), "lcp-echo-failure 10\n");
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, sizeof(buff), "lcp-echo-interval 10\n");
		WRITE_PPP_FILE(fdoptions, buff);

		if (server.tunnel_auth) {
			snprintf(buff, 64, "challenge = yes\n");
			WRITE_PPP_FILE(fdconf, buff);

			snprintf(buff, 64, "* * %s\n", server.tunnel_key);
			WRITE_PPP_FILE(fdl2tp, buff);
		}

		//snprintf(buff, sizeof(buff), "debug\n");
		//WRITE_PPP_FILE(fdoptions, buff);
		//snprintf(buff, sizeof(buff), "logfile /dev/console\n");
		//WRITE_PPP_FILE(fdoptions, buff);
		//snprintf(buff, sizeof(buff), "dump\n");
		//WRITE_PPP_FILE(fdoptions, buff);

		close(fdoptions);
	}
	close(fdl2tp);
	close(fdconf);

	/**********************************************************************/
	/* Write PPP option file												   */
	/**********************************************************************/

	int fdoptions = open("/etc/ppp/options", O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
	if (fdoptions == -1) {
		printf("Create /etc/ppp/options file error!\n");
		return;
	}
	snprintf(buff, 64, "debug\n");
	WRITE_PPP_FILE(fdoptions, buff);
	close(fdoptions);
}
#endif //CONFIG_USER_L2TPD_LNS
#endif //CONFIG_USER_XL2TPD

#ifdef CONFIG_USER_PPTP_CLIENT
/*
 *  generate pptp init script
 *  this script called after ppp device create, and ppp daemon will wait script finish
 *  return:
 *    -1: Fail
 *     0: Success
 */
static int generate_pptp_init_script(MIB_PPTP_Tp pEntry)
{
	FILE *fp=NULL;
	char wanif[IFNAMSIZ]={0}, script_path[128]={0};
	DIR* dir=NULL;

	if(!pEntry){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return -1;
	}

	dir = opendir(PPPD_INIT_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_INIT_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));

	if (PPP_INDEX(pEntry->ifIndex) != DUMMY_PPP_INDEX) {
		/* ppp initial script */
		snprintf(script_path, sizeof(script_path), "%s/"PPPD_INIT_SCRIPT_NAME, PPPD_INIT_SCRIPT_DIR, wanif);
		if (fp = fopen(script_path, "w+")) {
			fprintf(fp, "#!/bin/sh\n\n");

			fprintf(fp, "######### Script Globle Variable ###########\n");
			fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
			fprintf(fp, "PIDFILE=%s/${IFNAME}.pid\n", (char *)PPPD_CONFIG_FOLDER);
			fprintf(fp, "BACKUP_PIDFILE=%s/${IFNAME}.pid_back\n", (char *)PPPD_CONFIG_FOLDER);
			fprintf(fp, "######### Script Globle Variable ###########\n\n");

			fprintf(fp, "## PID file be remove when PPP link_terminated\n");
			fprintf(fp, "## it will cause delete ppp daemon fail, So backup it.\n");
			fprintf(fp, "if [ -f ${PIDFILE} ]; then\n");
				fprintf(fp, "\tcp -af ${PIDFILE} ${BACKUP_PIDFILE}\n");
			fprintf(fp, "fi\n");

			fclose(fp);
			if(chmod(script_path, 484) != 0)
				printf("chmod failed: %s %d\n", __func__, __LINE__);
		}else{
			fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, script_path);
			goto Error;
		}
	}
	else {
		fprintf(stderr, "[%s] Error: DUMMY_PPP_INDEX\n.", __FUNCTION__);
		goto Error;
	}

	return 0;
Error:
	return -1;
}

/*
 *	generate pppd ipcp/ipcpv6 up script for WAN interface
 *  return:
 *    -1: Fail
 *     0: Success
 */
static int generate_pptp_ipup_script(MIB_PPTP_Tp pEntry)
{
	int ret=0, len=0;
	FILE *fp=NULL;
	char wanif[IFNAMSIZ]={0}, ipup_path[128]={0};
	DIR* dir=NULL;

	if(!pEntry){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return -1;
	}

	dir = opendir(PPPD_V4_UP_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_V4_UP_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));

	if (PPP_INDEX(pEntry->ifIndex) != DUMMY_PPP_INDEX) {
		/* IPv4 script */
		snprintf(ipup_path, sizeof(ipup_path), "%s/"PPPD_V4_UP_SCRIPT_NAME, PPPD_V4_UP_SCRIPT_DIR, wanif);
		if (fp = fopen(ipup_path, "w+")) {
			fprintf(fp, "#!/bin/sh\n\n");

			fprintf(fp, "######### Script Globle Variable ###########\n");
			fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
			fprintf(fp, "######### Script Globle Variable ###########\n\n");

			// create dns resolv.conf.{ifname} file for dnsmasq
			fprintf(fp, "rm %s/resolv.conf.${IFNAME}\n", (char *)PPPD_CONFIG_FOLDER);
			fprintf(fp, "if [[ ${DNS1} != \"\" ]]; then echo \"${DNS1}@${IPLOCAL}\" >> %s/resolv.conf.${IFNAME}; fi\n", (char *)PPPD_CONFIG_FOLDER);
			fprintf(fp, "if [[ ${DNS2} != \"\" ]] && [[ ${DNS1} != ${DNS2} ]]; then echo \"${DNS2}@${IPLOCAL}\" >> %s/resolv.conf.${IFNAME}; fi\n\n", (char *)PPPD_CONFIG_FOLDER);

			//iptables -t nat -A address_map_tbl -o ppp11 -j MASQUERADE
			fprintf(fp, "iptables -t nat -A %s -o ${IFNAME} -j MASQUERADE\n\n", NAT_ADDRESS_MAP_TBL);

			if(pEntry->dgw){
				fprintf(fp, "# search route table and change default route as lower preference\n");
				fprintf(fp, "sysconf send_unix_sock_message %s do_update_vpn_def_rt ${IFNAME}\n\n", SYSTEMD_USOCK_SERVER);
			}

			fclose(fp);
			if(chmod(ipup_path, 484) != 0)
				printf("chmod failed: %s %d\n", __func__, __LINE__);
		}else{
			fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, ipup_path);
			goto Error;
		}
	}
	else {
		fprintf(stderr, "[%s] Error: DUMMY_PPP_INDEX\n.", __FUNCTION__);
		goto Error;
	}

	return ret;
Error:
	return -1;
}

/*
 *	generate pppd ipcp/ipcpv6 down script for WAN interface
 *  return:
 *    -1: Fail
 *     0: Success
 */
static int generate_pptp_ipdown_script(MIB_PPTP_Tp pEntry)
{
	int ret=0;
	FILE *fp=NULL;
	char wanif[IFNAMSIZ]={0}, ipdown_path[128]={0};
	unsigned char vChar=0;
	DIR* dir=NULL;

	if(!pEntry){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return -1;
	}

	dir = opendir(PPPD_V4_DOWN_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_V4_DOWN_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	if (PPP_INDEX(pEntry->ifIndex) != DUMMY_PPP_INDEX) {
		/* IPv4 script */
		snprintf(wanif, sizeof(wanif), "ppp%u", PPP_INDEX(pEntry->ifIndex));
		snprintf(ipdown_path, sizeof(ipdown_path), "%s/"PPPD_V4_DOWN_SCRIPT_NAME, PPPD_V4_DOWN_SCRIPT_DIR, wanif);
		if (fp = fopen(ipdown_path, "w+")) {
			fprintf(fp, "#!/bin/sh\n\n");

			fprintf(fp, "######### Script Globle Variable ###########\n");
			fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
			fprintf(fp, "######### Script Globle Variable ###########\n\n");

			// clean dns resolv.conf.{ifname} file
			fprintf(fp, "rm %s/resolv.conf.${IFNAME}\n\n", (char *)PPPD_CONFIG_FOLDER);

			//iptables -t nat -D address_map_tbl -o ppp11 -j MASQUERADE
			fprintf(fp, "iptables -t nat -D %s -o ${IFNAME} -j MASQUERADE\n\n", NAT_ADDRESS_MAP_TBL);

			if(pEntry->dgw){
				// recover default route
				fprintf(fp, "%s/"PPPD_VPN_DOWN_SCRIPT_NAME"\n", (char *)PPPD_VPN_SCRIPT_DIR, wanif);
			}

			fclose(fp);
			if(chmod(ipdown_path, 484) != 0)
				printf("chmod failed: %s %d\n", __func__, __LINE__);
		}else{
			fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, ipdown_path);
			goto Error;
		}
	}
	else {
		fprintf(stderr, "[%s] Error: DUMMY_PPP_INDEX\n.", __FUNCTION__);
		goto Error;
	}

	return ret;
Error:
	return -1;
}

int connect_pptp(unsigned int pppindex)
{
	char ifIdx[3];
	char *argv[32], confname[8];
	int idx, i;

	idx = 0;
	argv[idx++] = (char *)PPPD;
	argv[idx++] = "call";
	snprintf(confname, 8, "pptp%u", pppindex);
	argv[idx++] = (char *)confname;
	argv[idx++] = "logfd";
	argv[idx++] = "2";
	argv[idx++] = "nodetach";
	argv[idx++] = NULL;

	printf("%s", PPPD);
	for (i=1; i<idx-1; i++)
		printf(" %s", argv[i]);
	printf("\n");
	do_nice_cmd(PPPD, argv, 0);

	return 0;
}

void write_to_pptp_config(void)
{
	int i;
	char buff[64];
	unsigned int pptp_entry_num = 0, pppIndex;
	MIB_PPTP_T pptp_entry;
	struct in_addr server_ip;
	char ifname[IFNAMSIZ];

	/**********************************************************************/
	/* Write CHAP/PAP secrets file												   */
	/**********************************************************************/
	rtk_create_ppp_secrets();

	/**********************************************************************/
	/* Write PPTP config / option file												   */
	/**********************************************************************/

	pptp_entry_num = mib_chain_total(MIB_PPTP_TBL);

	for (i=0 ; i<pptp_entry_num ; i++) {
		int fdconf, fdoptions;
		char pptpconf_str[32], pptpopt_str[32];
		if (!mib_chain_get(MIB_PPTP_TBL, i, (void *)&pptp_entry))
			continue;

		pppIndex = PPP_INDEX(pptp_entry.ifIndex);
		snprintf(pptpconf_str, sizeof(pptpconf_str), "/etc/ppp/peers/pptp%u", pppIndex);
		fdconf = open(pptpconf_str, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
		if (fdconf == -1) {
			printf("Create %s file error!\n", pptpconf_str);
			return;
		}

		snprintf(pptpopt_str, sizeof(pptpopt_str), "/etc/ppp/options%u.pptp", pppIndex);
		fdoptions = open(pptpopt_str, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
		if (fdoptions == -1) {
			printf("Create %s file error!\n", pptpopt_str);
			return;
		}

		snprintf(buff, sizeof(buff), "unit %u\n", pppIndex);
		WRITE_PPP_FILE(fdoptions, buff);

		/* Use OPENWRT PPTP plugin to increase pptp performance */
		snprintf(buff, sizeof(buff), "plugin %s\n", PPTP_PLUGIN);
		WRITE_PPP_FILE(fdconf, buff);
		snprintf(buff, sizeof(buff), "pptp_server %s\n", pptp_entry.server);
		WRITE_PPP_FILE(fdconf, buff);

		snprintf(buff, sizeof(buff), "name %s\n", pptp_entry.username);
		WRITE_PPP_FILE(fdconf, buff);

		ifGetName(pptp_entry.ifIndex, ifname, IFNAMSIZ);
		snprintf(buff, sizeof(buff), "remotename %s\n", ifname);
		WRITE_PPP_FILE(fdconf, buff);

#ifdef CONFIG_IPV6_VPN
		if (pptp_entry.IpProtocol == IPVER_IPV6)
		{
			snprintf(buff, sizeof(buff), "noip\n");	// IPv6 Only
			WRITE_PPP_FILE(fdoptions, buff);
		}else if (pptp_entry.IpProtocol== IPVER_IPV4)
#endif
		{
			snprintf(buff, sizeof(buff), "noipv6\n"); // IPv4 Only
			WRITE_PPP_FILE(fdoptions, buff);
		}

		snprintf(buff, sizeof(buff), "noipdefault\n");
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, sizeof(buff), "nodetach\n");
		WRITE_PPP_FILE(fdoptions, buff);

		if (pptp_entry.authtype == AUTH_PAP) {
			snprintf(buff, sizeof(buff), "refuse-chap\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse-mschap\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse-mschap-v2\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}
		else if (pptp_entry.authtype == AUTH_CHAP) {
			snprintf(buff, sizeof(buff), "refuse-pap\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse-mschap\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse-mschap-v2\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}
		else if (pptp_entry.authtype == AUTH_CHAPMSV2) {
			snprintf(buff, sizeof(buff), "refuse-pap\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse-chap\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "refuse-mschap\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}
		snprintf(buff, sizeof(buff), "refuse-eap\n");
		WRITE_PPP_FILE(fdoptions, buff);

		if (pptp_entry.enctype == VPN_ENCTYPE_MPPE) {
			snprintf(buff, sizeof(buff), "mppe required\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "-mppc\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}
		else if (pptp_entry.enctype == VPN_ENCTYPE_MPPC) {
			snprintf(buff, sizeof(buff), "-mppe\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "+mppc\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}
		else if (pptp_entry.enctype == VPN_ENCTYPE_BOTH) {
			snprintf(buff, sizeof(buff), "mppe required\n");
			WRITE_PPP_FILE(fdoptions, buff);
			snprintf(buff, sizeof(buff), "+mppc\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}

		snprintf(buff, sizeof(buff), "file %s\n", pptpopt_str);
		WRITE_PPP_FILE(fdconf, buff);

		snprintf(buff, sizeof(buff), "persist\n");
		WRITE_PPP_FILE(fdconf, buff);

		snprintf(buff, sizeof(buff), "ipparam vpn%u\n", pppIndex);
		WRITE_PPP_FILE(fdconf, buff);

#if 0
		/* pptp plugin not support lock?? */
		snprintf(buff, sizeof(buff), "lock\n");
		WRITE_PPP_FILE(fdoptions, buff);
#endif

		//snprintf(buff, 64, "noauth\n");
		//WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, sizeof(buff), "nobsdcomp\n");
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, sizeof(buff), "nodeflate\n");
		WRITE_PPP_FILE(fdoptions, buff);

		if (pptp_entry.dgw == 1) {
			snprintf(buff, sizeof(buff), "defaultroute\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}

		//snprintf(buff, sizeof(buff), "debug\n");
		//WRITE_PPP_FILE(fdoptions, buff);
		//snprintf(buff, sizeof(buff), "dump\n");
		//WRITE_PPP_FILE(fdoptions, buff);

		close(fdoptions);
		close(fdconf);

		generate_pptp_init_script(&pptp_entry);
		generate_pptp_ipup_script(&pptp_entry);
		generate_pptp_ipdown_script(&pptp_entry);
	}

	/**********************************************************************/
	/* Write PPP option file												   */
	/**********************************************************************/

	int fdoptions = open("/etc/ppp/options", O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
	if (fdoptions == -1) {
		printf("Create /etc/ppp/options file error!\n");
		return;
	}
	snprintf(buff, sizeof(buff), "debug\n");
	WRITE_PPP_FILE(fdoptions, buff);
	close(fdoptions);
}

#ifdef CONFIG_USER_PPTP_SERVER
void write_to_pptp_server_config()
{
	int fdconf;
	char buff[64];
	MIB_VPND_T server;
	MIB_VPN_ACCOUNT_T entry;
	unsigned int pptp_entry_num = 0, pppindex=9;
	int i=0, j=0;
	struct in_addr addr, endaddr;
	char str_ipaddr[32], str_endipaddr[32];

	/**********************************************************************/
	/* Write PAP/CHAP secrets file                                        */
	/**********************************************************************/
	rtk_create_ppp_secrets();

	/**********************************************************************/
	/* Write PPTP config / option file												   */
	/**********************************************************************/

	fdconf = open("/etc/pptpd.conf", O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);

	if (fdconf == -1) {
		printf("Create /etc/pptpd.conf file error!\n");
		return;
	}
	snprintf(buff, 64, "ppp /bin/pppd\n");
	WRITE_PPP_FILE(fdconf, buff);

	snprintf(buff, 64, "option /etc/ppp/options.pptpd\n");
	WRITE_PPP_FILE(fdconf, buff);

	pptp_entry_num = mib_chain_total(MIB_VPN_SERVER_TBL);

	for (i=0 ; i<pptp_entry_num ; i++) {
		int fdoptions;
		char pptpdopt_str[32]={0};
		if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, (void *)&server))
			continue;

		if(server.type != VPN_PPTP)
			continue;

		snprintf(pptpdopt_str, 32, "/etc/ppp/options.pptpd");
		fdoptions = open(pptpdopt_str, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
		if (fdoptions == -1) {
			printf("Create %s file error!\n", pptpdopt_str);
			return;
		}

		snprintf(buff, 64, "unit %u\n", pppindex);
		WRITE_PPP_FILE(fdoptions, buff);

		addr.s_addr = server.peeraddr;
		snprintf(str_ipaddr, sizeof(str_ipaddr), "%s", inet_ntoa(addr));
		//if endaddr has no value, make endaddr same as peeraddr subnet
		if(server.endaddr == 0){
			server.endaddr = ntohl((htonl(server.peeraddr)|0xff)-1); //192.168.1."254"
		}

		//if peeraddr is no the same with endaddr, we reserved endaddr as LNS server local ip address
		/*
		if(server.peeraddr != server.endaddr){
			endaddr.s_addr = ntohl(htonl(server.endaddr)-1);
		}
		else{
			endaddr.s_addr = server.endaddr;
		}
		snprintf(str_endipaddr, sizeof(str_endipaddr), "%s", inet_ntoa(endaddr));
		*/
		snprintf(buff, 64, "remoteip %s-253\n", str_ipaddr);
		WRITE_PPP_FILE(fdconf, buff);

		//if endaddr has no value, make endaddr same as peeraddr subnet
		endaddr.s_addr = server.endaddr;
		snprintf(str_endipaddr, sizeof(str_endipaddr), "%s", inet_ntoa(endaddr));
		snprintf(buff, 64, "localip %s\n", str_endipaddr);
		WRITE_PPP_FILE(fdconf, buff);

		if (server.authtype != 4) {
			snprintf(buff, 64, "auth\n");
			WRITE_PPP_FILE(fdoptions, buff);

		if (server.authtype == AUTH_PAP) {
				snprintf(buff, 64, "require-pap\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}
		else if (server.authtype == AUTH_CHAP) {
				snprintf(buff, 64, "require-chap\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}
		else if (server.authtype == AUTH_CHAPMSV2) {
			snprintf(buff, 64, "require-mschap-v2\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}

			if (server.enctype == VPN_ENCTYPE_MPPE) {
				snprintf(buff, 64, "mppe required\n");
				WRITE_PPP_FILE(fdoptions, buff);
				snprintf(buff, 64, "-mppc\n");
				WRITE_PPP_FILE(fdoptions, buff);
			}
			else if (server.enctype == VPN_ENCTYPE_MPPC) {
				snprintf(buff, 64, "-mppe\n");
				WRITE_PPP_FILE(fdoptions, buff);
				snprintf(buff, 64, "+mppc\n");
				WRITE_PPP_FILE(fdoptions, buff);
			}
			else if (server.enctype == VPN_ENCTYPE_BOTH) {
				snprintf(buff, 64, "mppe required\n");
				WRITE_PPP_FILE(fdoptions, buff);
				snprintf(buff, 64, "+mppc\n");
			WRITE_PPP_FILE(fdoptions, buff);
		}
		}

		snprintf(buff, 64, "noipdefault\n");
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, 64, "nodetach\n");
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, 64, "noipv6\n"); // IPv4 Only
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, 64, "mtu 1200\n");
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, 64, "name %s\n", server.name);
		WRITE_PPP_FILE(fdoptions, buff);

		snprintf(buff, 64, "ms-dns %s\n", str_endipaddr);
		WRITE_PPP_FILE(fdoptions, buff);

		//snprintf(buff, 64, "debug\n");
		//WRITE_PPP_FILE(fdoptions, buff);
		//snprintf(buff, 64, "dump\n");
		//WRITE_PPP_FILE(fdoptions, buff);

		close(fdoptions);
	}
	close(fdconf);

	/**********************************************************************/
	/* Write PPP option file												   */
	/**********************************************************************/

	int fdoptions = open("/etc/ppp/options", O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
	if (fdoptions == -1) {
		printf("Create /etc/ppp/options file error!\n");
		return;
	}
	snprintf(buff, 64, "debug\n");
	WRITE_PPP_FILE(fdoptions, buff);
	close(fdoptions);
}
#endif
#endif

//static void write_to_ppp_script(MIB_CE_ATM_VC_Tp pEntry)
static void write_to_ppp_script(void)
{
	int fdoptions;
	char buff[64];
	int vcTotal, i;
	MIB_CE_ATM_VC_T Entry;
	int ispppoe = 0, ispppoa = 0;

	/**********************************************************************/
	/* Write CHAP/PAP secrets file												   */
	/**********************************************************************/
	rtk_create_ppp_secrets();

	/**********************************************************************/
	/* Write PPP options file												   */
	/**********************************************************************/

	fdoptions = open("/etc/ppp/options", O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);

	if (fdoptions == -1) {
		printf("Create /etc/ppp/options file error!\n");
		return;
	}

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < vcTotal; i++)
	{
		/* get the specified chain record */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return;

		if (Entry.cmode != CHANNEL_MODE_PPPOE && Entry.cmode != CHANNEL_MODE_PPPOA)
			continue;

		write_to_ppp_options(&Entry);

		if ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_PPPOE && ispppoe ==0) {
			snprintf(buff, 64, "plugin %s\n", PPPOE_PLUGIN);
			ispppoe = 1;
			WRITE_PPP_FILE(fdoptions, buff);
		}
		else if ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_PPPOA && ispppoa == 0) {
			snprintf(buff, 64, "plugin %s\n", PPPOA_PLUGIN);
			ispppoa = 1;
			WRITE_PPP_FILE(fdoptions, buff);
		}
	}

	close(fdoptions);
}

#if defined(CONFIG_USER_PPTP_CLIENT) || defined(CONFIG_USER_XL2TPD)
/*
 * This function aims to solve multiple default routes problem in the condition
 * that both vpn ppp and Internet interface(ex. nas0_0) are set as default route.
 * This vpn is going to be as default route, we have to
 * - set the original default route as lower preference.
 * - add host route for the vpn server ip, to make it go the original default gw.
 */
int rtk_add_ppp_vpn_hostroute(const char *vpn_ifname, struct in_addr *remote_addr)
{
	char buff[256];
	char ifname[32];
	char metrx_str[8];
	int flgs, metrx;
	int same_subnet;
	struct in_addr dest, gw_ip, mask;
	FILE *fp;
	FILE *fp_s;
	char vpn_down_path[128], vpn_down_path_dod[128];
	DIR* dir=NULL;
	char str_rmt_addr[16];

	if(!vpn_ifname || !remote_addr){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return -1;
	}

	dir = opendir(PPPD_VPN_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_VPN_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	snprintf(vpn_down_path, sizeof(vpn_down_path), "%s/"PPPD_VPN_DOWN_SCRIPT_NAME, PPPD_VPN_SCRIPT_DIR, vpn_ifname);
	if (access(vpn_down_path, F_OK) == 0){
		unlink(vpn_down_path);
	}

	snprintf(vpn_down_path_dod, sizeof(vpn_down_path_dod), "%s/"PPPD_VPN_DOWN_SCRIPT_NAME"_dod", PPPD_VPN_SCRIPT_DIR, vpn_ifname);
	if (access(vpn_down_path_dod, F_OK) == 0){
		unlink(vpn_down_path_dod);
	}

	/* Check direct connect */
	same_subnet = 0;
	if (!(fp = fopen("/proc/net/route", "r"))) {
		printf("Error: cannot open /proc/net/route\n");
		return -1;
	}
	fgets(buff, sizeof(buff), fp);
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		if (sscanf(buff, "%s%x%*x%x%*d%*d%*d%x", ifname, &dest, &flgs, &mask) != 4) {
			printf("Unsuported kernel route format\n");
			fclose(fp);
			return -1;
		}
		if ((flgs & RTF_UP) && mask.s_addr != 0) {
			if ((dest.s_addr & mask.s_addr) == (remote_addr->s_addr & mask.s_addr)) {
				//printf("dest=0x%x, mask=0x%x\n", dest.s_addr, mask.s_addr);
				//printf("%s: Direct connect to %s\n", __FUNCTION__, ifname);
				same_subnet = 1;
				break;
			}
		}
	}

	strncpy(str_rmt_addr, inet_ntoa(*remote_addr), 16);
	fseek(fp, 0, SEEK_SET);
	fgets(buff, sizeof(buff), fp);
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		if (sscanf(buff, "%s%x%x%x%*d%*d%d%x", ifname, &dest, &gw_ip, &flgs, &metrx, &mask) != 6) {
			printf("Unsuported kernel route format\n");
			fclose(fp);
			return -1;
		}
		if ((flgs & RTF_UP) && dest.s_addr == 0 && mask.s_addr == 0) {
			/* Default gw found */
			//printf("%s: Default route on %s to %s\n", __FUNCTION__, ifname, inet_ntoa(gw_ip));

			/* Change this default route as lower preference */
			/* Remove previous modified default route. */
			va_cmd(ROUTE, 4, 1, ARG_DEL, "default", "dev", ifname);

			/* Set original default route to lower preference (metric+1) */
			snprintf(metrx_str, sizeof(metrx_str), "%d", metrx+1);
			if (flgs & RTF_GATEWAY){
				va_cmd(ROUTE, 8, 1, ARG_ADD, "default", "gw", inet_ntoa(gw_ip), "dev", ifname, "metric", metrx_str);
			}else{
				va_cmd(ROUTE, 6, 1, ARG_ADD, "default", "dev", ifname, "metric", metrx_str);
			}

			/* Not direct connect, so we set host route. */
			if (!same_subnet) {
				/* set host route for the vpn server host */
				/* route add host_ip gw gw_ip */
				if (flgs & RTF_GATEWAY){
					va_cmd(ROUTE, 4, 1, ARG_ADD, str_rmt_addr, "gw", inet_ntoa(gw_ip));
				}else{
					va_cmd(ROUTE, 4, 1, ARG_ADD, str_rmt_addr, "dev", ifname);
				}

				fp_s = NULL;
				if ((fp_s = fopen(vpn_down_path_dod, "w")) != NULL) {
					fprintf(fp_s, "#!/bin/sh\n\n");
					fprintf(fp_s, "route del %s\n", str_rmt_addr);
					fprintf(fp_s, "\n");
					fclose(fp_s);
					chmod(vpn_down_path_dod, 484);
				}
			}

			/* Setup vpn down script */
			fp_s = NULL;
			if ((fp_s=fopen(vpn_down_path, "w")) != NULL) {
				fprintf(fp_s, "#!/bin/sh\n\n");

				/* Remove previous vpn default route. */
				fprintf(fp_s, "route del default dev %s\n\n", vpn_ifname);

				/* Restore original default route (metric) */
				if (flgs & RTF_GATEWAY){
					/* Remove previous modified default route. */
					fprintf(fp_s, "route del default gw %s dev %s\n", inet_ntoa(gw_ip), ifname);
					fprintf(fp_s, "route add default gw %s dev %s metric %d\n", inet_ntoa(gw_ip), ifname, metrx);
				}else{
					/* Remove previous modified default route. */
					fprintf(fp_s, "route del default dev %s\n", ifname);
					fprintf(fp_s, "route add default dev %s metric %d\n", ifname, metrx);
				}
				if (!same_subnet) /* Remove host route for server ip */
					fprintf(fp_s, "route del %s\n", str_rmt_addr);
				fprintf(fp_s, "\n");
				fclose(fp_s);
			}
			chmod(vpn_down_path, 484);
			break;
		}
	}
	fclose(fp);

	va_cmd(ROUTE, 4, 1, ARG_ADD, "default", "dev", vpn_ifname);
	return 0;
}
#endif /* #if defined(CONFIG_USER_PPTP_CLIENT) || defined(CONFIG_USER_XL2TPD) */
#endif  //CONFIG_USER_PPPD
#endif  //CONFIG_PPP

int startIP_v4(char *inf, MIB_CE_ATM_VC_Tp pEntry, CHANNEL_MODE_T ipEncap,
					int *isSyslog, char *msg)
{
	char myip[16], remoteip[16], netmask[16];
#ifdef IP_PASSTHROUGH
	unsigned int ippt_itf;
	int ippt=0;
	struct in_addr net;
	char netip[16];
#endif
	FILE *fp;
	// Mason Yu
	unsigned long broadcastIpAddr;
	char broadcast[16];

#ifdef CONFIG_IPV6_SIT_6RD
	if (ipEncap == CHANNEL_MODE_6RD) ipEncap = CHANNEL_MODE_IPOE;
#endif
#ifdef CONFIG_IPV6_SIT
	if (ipEncap == CHANNEL_MODE_6IN4 || ipEncap == CHANNEL_MODE_6TO4) ipEncap = CHANNEL_MODE_IPOE;
#endif

#ifdef IP_PASSTHROUGH
	// check IP passthrough
	if (mib_get_s(MIB_IPPT_ITF, (void *)&ippt_itf, sizeof(ippt_itf)) != 0)
	{
		if (ippt_itf == pEntry->ifIndex)
			ippt = 1; // this interface enable the IP passthrough
	}
#endif

	if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED)
	{
		int probe_sent = 0;
		unsigned char conflict = 0;
		unsigned char mibConflict = 0;
		unsigned char devAddr[MAC_ADDR_LEN];

		mib_get_s(MIB_ELAN_MAC_ADDR, (void *)devAddr, sizeof(devAddr));
		devAddr[MAC_ADDR_LEN-1] += 1;

		while(probe_sent < 3)
		{
			if (do_one_arp_probe(((struct in_addr *)pEntry->ipAddr)->s_addr, devAddr,"nas0_0") == 0)
			{
				conflict = 1;
				break;
			}
			probe_sent++;
		}

		strncpy(myip, inet_ntoa(*((struct in_addr *)pEntry->ipAddr)), 16);
		myip[15] = '\0';

		mib_get_s(MIB_IP_CONFLICT, (void *)&mibConflict,
				sizeof(mibConflict));
		if(conflict && (mibConflict != conflict))
		{
			*isSyslog = 1;
			sprintf(msg, "IPv4 Detect duplicate address %s\n", myip);
			mib_set(MIB_IP_CONFLICT, (void*)&conflict);
		}
		else if(!conflict && (mibConflict != conflict))
		{
			*isSyslog = 1;
			sprintf(msg, "IPv4 Undetect duplicate address %s\n", myip);
			mib_set(MIB_IP_CONFLICT, (void*)&conflict);
		}
		strncpy(remoteip, inet_ntoa(*((struct in_addr *)pEntry->remoteIpAddr)), 16);
		remoteip[15] = '\0';
		strncpy(netmask, inet_ntoa(*((struct in_addr *)pEntry->netMask)), 16);
		netmask[15] = '\0';
		// Mason Yu
		broadcastIpAddr = ((struct in_addr *)pEntry->ipAddr)->s_addr | ~(((struct in_addr *)pEntry->netMask)->s_addr);
		strncpy(broadcast, inet_ntoa(*((struct in_addr *)&broadcastIpAddr)), 16);
		broadcast[15] = '\0';
#ifdef CONFIG_ATM_CLIP
		if (ipEncap == CHANNEL_MODE_RT1483 || ipEncap == CHANNEL_MODE_RT1577)
#else
		if (ipEncap == CHANNEL_MODE_RT1483)
#endif
		{
#ifdef IP_PASSTHROUGH
			if (!ippt)
			{
#endif
				if (pEntry->ipunnumbered)	// Jenny, for IP unnumbered determination temporarily
					va_cmd(IFCONFIG, 8, 1, inf, "10.0.0.1", "-arp",
						"-broadcast", "pointopoint", "10.0.0.2",
						"netmask", ARG_255x4);
				else
					// ifconfig vc0 myip -arp -broadcast pointopoint
					//   remoteip netmask 255.255.255.255
					va_cmd(IFCONFIG, 8, 1, inf, myip, "-arp",
						"-broadcast", "pointopoint", remoteip,
//						"netmask", netmask);	// Jenny, subnet mask added
						"netmask", ARG_255x4);
#ifdef IP_PASSTHROUGH
			}
			else	// IP passthrough
			{
				// ifconfig vc0 10.0.0.1 -arp -broadcast pointopoint
				//   10.0.0.2 netmask 255.255.255.255
				va_cmd(IFCONFIG, 8, 1, inf, "10.0.0.1", "-arp",
					"-broadcast", "pointopoint", "10.0.0.2",				// Jenny, for IP passthrough determination temporarily
					"netmask", netmask);
//				va_cmd(IFCONFIG, 8, 1, inf, "10.0.0.1", "-arp",
//					"-broadcast", "pointopoint", "10.0.0.2",
//					"netmask", ARG_255x4);
				// ifconfig br0:1 remoteip
				va_cmd(IFCONFIG, 2, 1, LAN_IPPT, remoteip);

				// Mason Yu. Add route for Public IP
				net.s_addr = (((struct in_addr *)pEntry->remoteIpAddr)->s_addr) & (((struct in_addr *)pEntry->netMask)->s_addr);
				strncpy(netip, inet_ntoa(net), 16);
				netip[15] = '\0';
				va_cmd(ROUTE, 7, 1, ARG_DEL, "-net", netip, "netmask", netmask, "dev", LANIF);
				va_cmd(ROUTE, 5, 1, ARG_ADD, "-host", myip, "dev", LANIF);

				// write ip to file for DHCP server
				if (fp = fopen(IPOA_IPINFO, "w"))
				{
					fwrite( pEntry->ipAddr, 4, 1, fp);
					fwrite( pEntry->remoteIpAddr, 4, 1, fp);
					fwrite( pEntry->netMask, 4, 1, fp);
					fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
					chmod(IPOA_IPINFO,0666);
#endif

				}
			}
#endif
			if (isDefaultRouteWan(pEntry))
			{
#ifdef DEFAULT_GATEWAY_V2
				if (ifExistedDGW() == 1)	// Jenny, delete existed default gateway first
					va_cmd(ROUTE, 2, 1, ARG_DEL, "default");
#endif
				// route add default vc0
				va_cmd(ROUTE, 3, 1, ARG_ADD, "default", inf);
				//va_cmd(ROUTE, 4, 1, ARG_ADD, "default", "gw", remoteip);
			}
		}
		else
		{
			// Mason Yu. Set netmask and broadcast
			// ifconfig vc0 myip
			//va_cmd(IFCONFIG, 2, 1, inf, myip);

			va_cmd(IFCONFIG, 6, 1, inf, myip, "netmask", netmask, "broadcast",  broadcast);

			if (isDefaultRouteWan(pEntry))
			{
#ifdef DEFAULT_GATEWAY_V2
					if (ifExistedDGW() == 1)	// Jenny, delete existed default gateway first
						va_cmd(ROUTE, 2, 1, ARG_DEL, "default");
#endif
				// route add default gw remotip dev nas0_0
				va_cmd(ROUTE, 4, 1, ARG_ADD, "default", "gw", remoteip, "dev", inf);
			}
			if (ipEncap == CHANNEL_MODE_IPOE) {
				unsigned char value[32];
				snprintf(value, 32, "%s.%s", (char *)MER_GWINFO, inf);
				if (fp = fopen(value, "w"))
				{
					fprintf(fp, "%s\n", remoteip);
					fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
					chmod(value,0666);
#endif
				}
			}
		}
	}
	else
	{
		int dhcpc_pid;
		unsigned char value[32];
		// Enabling support for a dynamically assigned IP (ISP DHCP)...
#ifdef RESTRICT_PROCESS_PERMISSION
			system("/bin/echo 1 > /proc/sys/net/ipv4/ip_dynaddr");
#else
		if (fp = fopen(PROC_DYNADDR, "w"))
		{
			fprintf(fp, "1\n");
			fclose(fp);
		}
		else
		{
			printf("Open file %s failed !\n", PROC_DYNADDR);
		}
#endif

		snprintf(value, 32, "%s.%s", (char*)DHCPC_PID, inf);
		dhcpc_pid = read_pid((char*)value);
		if (dhcpc_pid > 0)
			kill(dhcpc_pid, SIGTERM);
		if (startDhcpc(inf, pEntry, 0) == -1)
		{
			printf("start DHCP client failed !\n");
		}
	}

#ifdef CONFIG_HWNAT
	if (pEntry->napt == 1){
		if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED){
			send_extip_to_hwnat(ADD_EXTIP_CMD, inf, *(uint32 *)pEntry->ipAddr);
		}
		else{
			send_extip_to_hwnat(ADD_EXTIP_CMD, inf, 0);
		}
	}
#endif
#ifdef ROUTING
	// When interface IP reset, the static route will also be reseted.
	deleteStaticRoute();
	addStaticRoute();
#endif

	set_ipv4_default_policy_route(pEntry, 1);

	return 1;
}
// IP interface: 1483-r or MER
// return value:
// 1  : successful
int startIP(char *inf, MIB_CE_ATM_VC_Tp pEntry, CHANNEL_MODE_T ipEncap)
{
	unsigned char buffer[7];
	int isSyslog = 0;
	char msg[256] = {0,};

	// Move MTU setup to setup_ethernet_config()
	#if 0
	// Set MTU for 1483-r or MER
	sprintf(buffer, "%u", pEntry->mtu);
	va_cmd(IFCONFIG,3,1,inf, "mtu", buffer);
	#endif

#ifdef CONFIG_IPV6
	if (pEntry->IpProtocol & IPVER_IPV4) {
#endif
		startIP_v4(inf, pEntry, ipEncap, &isSyslog, msg);
#ifdef CONFIG_IPV6
	}
	/* We don't let boa to do startIP_for_V6, It usually cause some IPv6 daemon config or lease file failed.
	 * For IPoE systemd link up will trigger startIP_for_V6(). Maybe we can do the same thing for startIP_v4()  20201016 */
	//if (pEntry->IpProtocol & IPVER_IPV6)
	//	startIP_for_V6(pEntry);
#endif
	return 1;
}

#ifdef CONFIG_PPP
// Jenny, stop PPP
void stopPPP(void)
{
	int i;
	char tp[10];
	struct data_to_pass_st msg;

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	for (i=9; i<=10; i++)
	{
#ifdef CONFIG_USER_PPPD
		disconnectPPP(i);
#endif //CONFIG_USER_PPPD
#ifdef CONFIG_USER_SPPPD
		sprintf(tp, "ppp%d", i);
		if (find_ppp_from_conf(tp)) {
			snprintf(msg.data, BUF_SIZE, "spppctl del %d pptp 0", i);
			TRACE(STA_SCRIPT, "%s\n", msg.data);
			write_to_pppd(&msg);
		}
#endif //CONFIG_USER_SPPPD
	}
#endif

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	for (i=11; i<=12; i++)
	{
#ifdef CONFIG_USER_PPPD
		disconnectPPP(i);
#endif //CONFIG_USER_PPPD
#ifdef CONFIG_USER_SPPPD
		sprintf(tp, "ppp%d", i);
		if (find_ppp_from_conf(tp)) {
			snprintf(msg.data, BUF_SIZE, "spppctl del %u l2tp 0", i);
			TRACE(STA_SCRIPT, "%s\n", msg.data);
			write_to_pppd(&msg);
		}
#endif //CONFIG_USER_SPPPD
	}
#endif

	/* L2TP/PPTP can over other ppp interface, So it should disconnect after L2TP/PPTP */
#ifdef CONFIG_USER_PPPOMODEM
	for (i=0; i<(MAX_PPP_NUM+MAX_MODEM_PPPNUM); i++)
#else
	for (i=0; i<MAX_PPP_NUM; i++)
#endif //CONFIG_USER_PPPOMODEM
	{
#ifdef CONFIG_USER_PPPD
		disconnectPPP(i);
#endif //CONFIG_USER_PPPD
#ifdef CONFIG_USER_SPPPD
		sprintf(tp, "ppp%d", i);
		if (find_ppp_from_conf(tp)) {
			snprintf(msg.data, BUF_SIZE, "spppctl del %u", i);
			TRACE(STA_SCRIPT, "%s\n", msg.data);
			write_to_pppd(&msg);
		}
#endif //CONFIG_USER_SPPPD
	}

	usleep(2000000);

}

#ifdef CONFIG_USER_PPPD
void connectPPP(char *inf, MIB_CE_ATM_VC_Tp pEntry, CHANNEL_MODE_T pppEncap)
{
	connectPPPExt(inf, pEntry, pppEncap, 0);
}

void connectPPPExt(char *inf, MIB_CE_ATM_VC_Tp pEntry, CHANNEL_MODE_T pppEncap, int isSimu)
{
	char ifIdx[3];
	char *argv[32], value[16], mru[8], idletime[16], katimer[8], echo_retry[8], optname[32];
	int idx, i;
	char cmd[256]={0};
	char tmp[16]={0};

	if(!inf && !pEntry){
		fprintf(stderr, "[%s:%d] null parameters\n", __FUNCTION__, __LINE__);
		return;
	}

	idx = 0;
	argv[idx++] = (char *)PPPD;
	argv[idx++] = "unit";
	snprintf(ifIdx, 3, "%u", PPP_INDEX(pEntry->ifIndex));
	argv[idx++] = (char *)ifIdx;
	argv[idx++] = "file";
	if(isSimu == 1){
		ifGetName(pEntry->ifIndex, tmp, sizeof(tmp));
		snprintf(optname, sizeof(optname), "%s/opts_simu_%s", PPPD_PEERS_FOLDER, tmp);
	}
	else{
		snprintf(optname, sizeof(optname), "%s/opts_ppp%s", PPPD_PEERS_FOLDER, ifIdx);
	}
	argv[idx++] = (char *)optname;
	if (pppEncap == CHANNEL_MODE_PPPOE){		// PPPoE
		// pppd vc0 nodetach plugin /lib/plugins/rp-pppoe.so user USER
		argv[idx++] = inf;
	}
	else {	// PPPoA
		// pppd 4.35 nodetach plugin /lib/plugins/pppoatm.so user USER
		snprintf(value, 16, "%u.%u", pEntry->vpi, pEntry->vci);
		argv[idx++] = (char *)value;
	}
	argv[idx++] = NULL;

	printf("%s", PPPD);
	for (i=1; i<idx-1; i++)
		printf(" %s", argv[i]);
	printf("\n");
	do_nice_cmd(PPPD, argv, 0);
}


void disconnectPPP(unsigned char pppidx){
	char ifname[IFNAMSIZ]={0}, pidfile[64]={0};
	char back_pidfile[64]={0};
	pid_t pid;
	int entryIndex=0;
	MIB_CE_ATM_VC_T Entry;
	char buff[128];
#ifdef CONFIG_USER_XL2TPD
	MIB_L2TP_T l2tp_entry;
#endif
#if defined(CONFIG_USER_PPTP_CLIENT)
	MIB_PPTP_T pptp_entry;
#endif
#ifdef CONFIG_USER_PPPOMODEM
	MIB_WAN_3G_T pom_entry;
#endif
#ifdef CONFIG_USER_DHCP_ISC
	char dhclient_pidfile[64] = {0}, leasefile[64] = {0};
#endif
#ifdef CONFIG_IPV6
	unsigned char duid_mode=(DHCPV6_DUID_TYPE_T)DHCPV6_DUID_LLT;
#endif

	if(pppidx == DUMMY_PPP_INDEX)
		return;

	snprintf(ifname, sizeof(ifname), "ppp%u", pppidx);
	entryIndex= getATMVcEntryByName(ifname,&Entry);
	if(entryIndex != -1){
		if (!Entry.enable) return;

#ifdef CONFIG_IPV6
		if(Entry.IpProtocol & IPVER_IPV6){
#ifdef CONFIG_USER_DHCP_ISC
			if(Entry.Ipv6DhcpRequest & (M_DHCPv6_REQ_IAPD|M_DHCPv6_REQ_IANA)){
				snprintf(dhclient_pidfile, sizeof(dhclient_pidfile), "%s.%s", PATH_DHCLIENT6_PID, ifname);
				pid = read_pid(dhclient_pidfile);
				if (pid > 0){
					/* delete DHCPcv6 lease file for this interface */
					snprintf(leasefile, sizeof(leasefile), "%s.%s", PATH_DHCLIENT6_LEASE, ifname);
					/* release dhcpv6 , and kill previous dhclient after release finished.
					 * Use -l to try to get lease once to avoid long duration. */
					mib_get_s(MIB_DHCPV6_CLIENT_DUID_MODE, (void *)&duid_mode, sizeof(duid_mode));
					if (duid_mode == DHCPV6_DUID_LL)
						va_cmd(DHCPCV6, 12, 1, "-6", "-r", "-1", "-lf", leasefile, "-pf", dhclient_pidfile, ifname, "-N", "-P", "-D", "LL");
					else
						va_cmd(DHCPCV6, 12, 1, "-6", "-r", "-1", "-lf", leasefile, "-pf", dhclient_pidfile, ifname, "-N", "-P", "-D", "LLT");
				}
			}
#endif // CONFIG_USER_DHCP_ISC
		}
#endif // CONFIG_IPV6
	}
#if defined(CONFIG_USER_XL2TPD)
	else if(getL2TPEntryByIfname(ifname, &l2tp_entry, &entryIndex)){
		printf("[%s] %s is L2TP Interface.\n", __FUNCTION__, ifname);

		/* disconnect old tunnel */
		sprintf(buff,"echo \"d connect%u\" > /var/run/xl2tpd/l2tp-control", pppidx);
		printf("[%s] %s\n", __FUNCTION__, buff);
		system(buff);
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT)
	else if(getPPTPEntryByIfname(ifname, &pptp_entry, &entryIndex)){
		printf("[%s] %s is PPTP Interface.\n", __FUNCTION__, ifname);
	}
#endif
#ifdef CONFIG_SUPPORT_AUTO_DIAG
	else if(getSimuWanEntrybyIfname(ifname, &Entry, &entryIndex)==0){
		if (!Entry.enable) return;
	}
#endif
#ifdef CONFIG_USER_PPPOMODEM
	else if(getPoMEntryByIfname(ifname, &pom_entry, &entryIndex)){
		if (!pom_entry.enable) return;
	}
#endif
	else{
		printf("[%s] unknown ppp interface(%s).\n", __FUNCTION__, ifname);
	}

	snprintf(pidfile, sizeof(pidfile), "%s/%s.pid", PPPD_CONFIG_FOLDER, ifname);
	snprintf(back_pidfile, sizeof(back_pidfile), "%s/%s.pid_back", PPPD_CONFIG_FOLDER, ifname);

	/* SIGINT, SIGTERM
	 * These signals cause pppd to terminate the link (by closing LCP),
	 * restore the serial device settings, and exit.
	 * If a connector or disconnector process is currently running,
	 * pppd will send the same signal to its process group,
	 * so as to terminate the connector or disconnector process. */
	pid = read_pid(pidfile);
	if (pid <= 0){
		/* PID file be remove when PPP link_terminated
		 * it will cause delete ppp daemon fail, So backup it on ipcp/ipv6cp up script. */
		kill_by_pidfile(back_pidfile, SIGTERM);
	}else{
		kill_by_pidfile(pidfile, SIGTERM);

		/* PPP wan is deleted, it don't need backup pid file anymore, remove it. */
		snprintf(back_pidfile, sizeof(back_pidfile), "%s/%s.pid_back", PPPD_CONFIG_FOLDER, ifname);
		if (access(back_pidfile, F_OK) == 0) unlink(back_pidfile);
	}

	return;
}
#endif //CONFIG_USER_PPPD

#if defined(CONFIG_LUNA)
char *translate_special_chars(char *str)
{
	if(!str)
		return NULL;

	int i = 0;
	char *s = malloc((strlen(str) + 1) * 2);
	if(!s)
		return NULL;

	while (*str) {
		if (strchr("`\"$\\", *str))
			s[i++] = '\\';
		s[i++] = *str++;
	}
	s[i] = 0;
	return s;
}
#endif

// PPP connection
// Input: inf == "vc0","vc1", ...
// pppEncap: 0 : PPPoE, 1 : PPPoA
// return value:
// 1  : successful
int startPPP(char *inf, MIB_CE_ATM_VC_Tp pEntry, char *qos, CHANNEL_MODE_T pppEncap)
{
	char ifIdx[3], pppif[6], stimeout[7];
#ifdef IP_PASSTHROUGH
	unsigned int ippt_itf;
	int ippt=0;
#endif
	struct data_to_pass_st msg;
	int lastArg, pppinf, pppdbg = 0;
#ifdef NEW_PORTMAPPING
	int tbl_id;
	char str_tblid[16];
	char devname[IFNAMSIZ];
#endif

#if defined(CONFIG_LUNA)
	unsigned char *pppUsername=NULL;
	unsigned char *pppPassword=NULL;

	pppUsername = translate_special_chars(pEntry->pppUsername);
	pppPassword = translate_special_chars(pEntry->pppPassword);
#endif

	memset(msg.data, 0, BUF_SIZE);
#ifdef IP_PASSTHROUGH
	// check IP passthrough
	if (mib_get_s(MIB_IPPT_ITF, (void *)&ippt_itf, sizeof(ippt_itf)) != 0)
	{
		if (ippt_itf == pEntry->ifIndex)
			ippt = 1; // this interface enable the IP passthrough
	}
#endif

	snprintf(ifIdx, 3, "%u", PPP_INDEX(pEntry->ifIndex));
	snprintf(pppif, 6, "ppp%u", PPP_INDEX(pEntry->ifIndex));

	if (pppEncap == CHANNEL_MODE_PPPOE)	// PPPoE
	{
		// Kaohj -- set pppoe txqueuelen to 0 to make it no queue as the queueing is
		//		just in the underlying vc interface
		// ifconfig ppp0 txqueuelen 0
		// tc qdisc replace dev ppp0 root pfifo
		// tc qdisc del dev ppp0 root
		va_cmd(IFCONFIG, 3, 1, pppif, "txqueuelen", "1000");
		// workaround to remove default qdisc if any.
		va_cmd(TC, 6, 1, "qdisc", "replace", "dev", pppif, "root", "pfifo");
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", pppif, "root");
//#ifdef CONFIG_USER_PPPOE_PROXY
#if 0
              printf("enable pppoe proxy %d \n",pEntry->PPPoEProxyEnable);
              printf("maxuser = %d ...",pEntry->PPPoEProxyMaxUser);
             if(pEntry->PPPoEProxyEnable)
	    {
                FILE  *fp_pap;
                pppoe_proxy pp_proxy;
	         if ((fp_pap = fopen(PPPD_PAPFILE, "a+")) == NULL)
		  {
			printf("Open file %s failed !\n", PPPD_PAPFILE);
			return -1;
		  }
		  fprintf(fp_pap, "%s * \"%s\"  *\n", pEntry->pppUsername, pEntry->pppPassword);
		  fclose(fp_pap);

		  if(!has_pppoe_init){
				pp_proxy.cmd =PPPOE_PROXY_ENABLE;
				ppp_proxy_ioctl(&pp_proxy,SIOCPPPOEPROXY);
				has_pppoe_init =1;
		   }
		    pp_proxy.wan_unit =PPP_INDEX(pEntry->ifIndex) ;
		    pp_proxy.cmd =PPPOE_WAN_UNIT_SET;
		    strcpy(pp_proxy.user,pEntry->pppUsername);
		    strcpy(pp_proxy.passwd,pEntry->pppPassword);
		    pp_proxy.maxShareNum = pEntry->PPPoEProxyMaxUser;
		    ppp_proxy_ioctl(&pp_proxy,SIOCPPPOEPROXY);

			if (pEntry->napt == 1)
			{	// Enable NAPT
				va_cmd(IPTABLES, 8, 1, "-t", "nat", FW_ADD, NAT_ADDRESS_MAP_TBL,
				"-o", pppif, "-j", "MASQUERADE");
			}


			return 1;
             }
#endif

		if (pEntry->pppCtype != MANUAL){
#if CONFIG_USER_PPPD
			// pppd vc0 nodetach plugin /lib/plugins/rp-pppoe.so user USER
			connectPPP(inf, pEntry, pppEncap);
#endif //CONFIG_USER_PPPD
#ifdef CONFIG_USER_SPPPD
			// spppctl add 0 pppoe vc0 username USER password PASSWORD
			//         gw 1 mru xxxx acname xxx
#if defined(CONFIG_LUNA)
			snprintf(msg.data, BUF_SIZE,
				"spppctl add %s pppoe %s username \"%s\" password \"%s\""
				" gw %d mru %d", ifIdx,
				inf, pppUsername?pppUsername:pEntry->pppUsername, pppPassword?pppPassword:pEntry->pppPassword,
				isDefaultRouteWan(pEntry), pEntry->mtu);
#else
			snprintf(msg.data, BUF_SIZE,
				"spppctl add %s pppoe %s username %s password %s"
				" gw %d mru %d", ifIdx,
				inf, pEntry->pppUsername, pEntry->pppPassword,
				isDefaultRouteWan(pEntry), pEntry->mtu);
#endif
#endif //CONFIG_USER_SPPPD
		}
#ifdef CONFIG_USER_SPPPD
		else {
#if defined(CONFIG_LUNA)
			snprintf(msg.data, BUF_SIZE,
				"spppctl new %s pppoe %s username \"%s\" password \"%s\""
				" gw %d mru %d", ifIdx,
				inf, pppUsername?pppUsername:pEntry->pppUsername, pppPassword?pppPassword:pEntry->pppPassword,
				isDefaultRouteWan(pEntry), pEntry->mtu);
#else
			snprintf(msg.data, BUF_SIZE,
				"spppctl new %s pppoe %s username %s password %s"
				" gw %d mru %d", ifIdx,
				inf, pEntry->pppUsername, pEntry->pppPassword,
				isDefaultRouteWan(pEntry), pEntry->mtu);
#endif
		}
		if (strlen(pEntry->pppACName))
			snprintf(msg.data + strlen(msg.data), BUF_SIZE - strlen(msg.data), " acname %s", pEntry->pppACName);
		// Set Service Name
		if (strlen(pEntry->pppServiceName))
			snprintf(msg.data + strlen(msg.data), BUF_SIZE - strlen(msg.data), " servicename %s", pEntry->pppServiceName);
#ifdef CONFIG_SPPPD_STATICIP
		// Jenny, set PPPoE static IP
		if (pEntry->pppIp) {
			unsigned long addr;
			addr = *((unsigned long *)pEntry->ipAddr);
			if (addr)
				snprintf(msg.data+ strlen(msg.data), BUF_SIZE - strlen(msg.data), " staticip %x", addr);
		}
#endif
#ifdef CONFIG_USER_PPPOE_PROXY
		snprintf(msg.data+strlen(msg.data), BUF_SIZE-strlen(msg.data), " proxy %d maxuser %d itfgroup %d", pEntry->PPPoEProxyEnable, pEntry->PPPoEProxyMaxUser, pEntry->itfGroup);
#endif
#endif	//CONFIG_USER_SPPPD
	}
	#ifdef CONFIG_DEV_xDSL
	else	// PPPoA
	{
		if (pEntry->pppCtype != MANUAL)
#ifdef CONFIG_USER_PPPD
		{
			// pppd 4.35 nodetach plugin /lib/plugins/pppoatm.so user USER
			connectPPP(inf, pEntry, pppEncap);
		}
#endif //CONFIG_USER_PPPD
#ifdef CONFIG_USER_SPPPD
		{
			// spppctl add 0 pppoa vpi.vci encaps ENCAP qos xxx
			//         username USER password PASSWORD gw 1 mru xxxx
#if defined(CONFIG_LUNA)
			snprintf(msg.data, BUF_SIZE,
				"spppctl add %s pppoa %u.%u encaps %d qos %s "
				"username \"%s\" password \"%s\" gw %d mru %d",
				ifIdx, pEntry->vpi, pEntry->vci, pEntry->encap,	qos,
				pppUsername?pppUsername:pEntry->pppUsername, pppPassword?pppPassword:pEntry->pppPassword, isDefaultRouteWan(pEntry), pEntry->mtu);
#else
			snprintf(msg.data, BUF_SIZE,
				"spppctl add %s pppoa %u.%u encaps %d qos %s "
				"username %s password %s gw %d mru %d",
				ifIdx, pEntry->vpi, pEntry->vci, pEntry->encap,	qos,
				pEntry->pppUsername, pEntry->pppPassword, isDefaultRouteWan(pEntry), pEntry->mtu);
#endif
		}else{
#if defined(CONFIG_LUNA)
			snprintf(msg.data, BUF_SIZE,
				"spppctl new %s pppoa %u.%u encaps %d qos %s "
				"username \"%s\" password \"%s\" gw %d mru %d",
				ifIdx, pEntry->vpi, pEntry->vci, pEntry->encap,	qos,
				pppUsername?pppUsername:pEntry->pppUsername, pppPassword?pppPassword:pEntry->pppPassword, isDefaultRouteWan(pEntry), pEntry->mtu);
#else
			snprintf(msg.data, BUF_SIZE,
				"spppctl new %s pppoa %u.%u encaps %d qos %s "
				"username %s password %s gw %d mru %d",
				ifIdx, pEntry->vpi, pEntry->vci, pEntry->encap,	qos,
				pEntry->pppUsername, pEntry->pppPassword, isDefaultRouteWan(pEntry), pEntry->mtu);
#endif
		}
#endif //CONFIG_USER_SPPPD
	}
	#endif // CONFIG_DEV_xDSL

	// Set Authentication Method
#ifdef CONFIG_USER_SPPPD
	if ((PPP_AUTH_T)pEntry->pppAuth >= PPP_AUTH_PAP && (PPP_AUTH_T)pEntry->pppAuth <= PPP_AUTH_MSCHAPV2)
		snprintf(msg.data + strlen(msg.data), BUF_SIZE - strlen(msg.data), " auth %s", ppp_auth[pEntry->pppAuth]);

	//printf("%s:%d auth %s\n", __FUNCTION__, __LINE__,ppp_auth[pEntry->pppAuth]);
#ifdef IP_PASSTHROUGH
	// Set IP passthrough
	snprintf(msg.data + strlen(msg.data), BUF_SIZE - strlen(msg.data), " ippt %d", ippt);
#endif

	// set PPP debug
	pppdbg = pEntry->pppDebug==0?0:1;
	snprintf(msg.data + strlen(msg.data), BUF_SIZE - strlen(msg.data), " debug %d", pppdbg);

#ifdef _CWMP_MIB_
	// Set Auto Disconnect Timer
	if (pEntry->autoDisTime > 0)
		snprintf(msg.data + strlen(msg.data), BUF_SIZE - strlen(msg.data), " disctimer %d", pEntry->autoDisTime);
	// Set Warn Disconnect Delay
	if (pEntry->warnDisDelay > 0)
		snprintf(msg.data + strlen(msg.data), BUF_SIZE - strlen(msg.data), " discdelay %d", pEntry->warnDisDelay);
#endif

#ifdef CONFIG_IPV6
	snprintf(msg.data + strlen(msg.data), BUF_SIZE - strlen(msg.data), " ipt %u", pEntry->IpProtocol - 1);
#endif

	if (pEntry->pppCtype == CONTINUOUS)	// Continuous
	{
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
		// set the ppp keepalive timeout
		if(pEntry->pppLcpEcho)
			snprintf(msg.data, BUF_SIZE, "spppctl katimer %hu", pEntry->pppLcpEcho);
		else
#if defined(CONFIG_00R0)
			snprintf(msg.data, BUF_SIZE, "spppctl katimer 30");	//default
#elif defined(CONFIG_RTK_DEV_AP)
			snprintf(msg.data, BUF_SIZE, "spppctl katimer 10");//default
#elif defined(CONFIG_CMCC_ENTERPRISE)
			snprintf(msg.data, BUF_SIZE, "spppctl katimer 60");//default
#elif defined(CONFIG_CMCC)
			snprintf(msg.data, BUF_SIZE, "spppctl katimer 30"); //CMCC neeed retry ppp diag in 60 seconds.
#else
			snprintf(msg.data, BUF_SIZE, "spppctl katimer 20");//default
#endif
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);

		// set the ppp keepalive timeout
		if(pEntry->pppLcpEchoRetry)
			snprintf(msg.data, BUF_SIZE, "spppctl echo_retry %hu", pEntry->pppLcpEchoRetry);
		else
#if defined(CONFIG_00R0)
			snprintf(msg.data, BUF_SIZE, "spppctl echo_retry 3");	//default
#elif defined(CONFIG_RTK_DEV_AP)
			snprintf(msg.data, BUF_SIZE, "spppctl echo_retry 3");	//default
#elif defined(CONFIG_CMCC_ENTERPRISE)
			snprintf(msg.data, BUF_SIZE, "spppctl echo_retry 6");	//default
#elif defined(CONFIG_CMCC)
			snprintf(msg.data, BUF_SIZE, "spppctl echo_retry 3"); //CMCC neeed retry ppp diag in 60 seconds.
#else
			snprintf(msg.data, BUF_SIZE, "spppctl echo_retry 6");	//default
#endif
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
		printf("PPP Connection (Continuous)...\n");
	}
	else if (pEntry->pppCtype == CONNECT_ON_DEMAND)	// On-demand
	{
		snprintf(msg.data + strlen(msg.data), BUF_SIZE - strlen(msg.data), " timeout_en 1 timeout %u", pEntry->pppIdleTime);
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
		printf("PPP Connection (On-demand)...\n");
	}
	else if (pEntry->pppCtype == MANUAL)	// Manual
	{
		// Jenny, for PPP connecting/disconnecting manually
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
		printf("PPP Connection (Manual)...\n");
	}
#endif	//CONFIG_USER_SPPPD

#ifdef NEW_PORTMAPPING
	// Add initial default route to dev (ex. nas0_0) when ppp not ready.
	tbl_id = caculate_tblid(pEntry->ifIndex);
	snprintf(str_tblid, 16, "%d", tbl_id);
	ifGetName(PHY_INTF(pEntry->ifIndex), devname, sizeof(devname));
	printf("%s: add default dev %s table %d\n", __FUNCTION__, devname, tbl_id);
	va_cmd("/usr/bin/ip", 7, 1, "route", "add", "default", "dev", devname, "table", str_tblid);
#endif
	if (pEntry->napt == 1)
	{
#ifdef CONFIG_HWNAT
		/*QL: pppoe interface should do HW SNAT as well as mer interface*/
		send_extip_to_hwnat(ADD_EXTIP_CMD, pppif, 0);
#endif
	}

	set_ipv4_default_policy_route(pEntry, 1);

#if defined(CONFIG_LUNA)
	if(pppUsername)
		free(pppUsername);
	if(pppPassword)
		free(pppPassword);
#endif

	return 1;
}

// find if pppif exists in /var/ppp/ppp.conf
int find_ppp_from_conf(char *pppif)
{
	char buff[256];
	FILE *fp;
	char strif[6];
	int found=0;

	if (!(fp=fopen(PPP_CONF, "r"))) {
		printf("%s not exists.\n", PPP_CONF);
	}
	else {
		fgets(buff, sizeof(buff), fp);
		while( fgets(buff, sizeof(buff), fp) != NULL ) {
			if(sscanf(buff, "%s", strif)!=1) {
				found=0;
				printf("Unsuported ppp configuration format\n");
				break;
			}

			if ( !strcmp(pppif, strif) ) {
				found = 1;
				break;
			}
		}
		fclose(fp);
	}
	return found;
}
#endif

void update_wan_routing(char *ifname)
{
#ifdef NEW_PORTMAPPING
    MIB_CE_ATM_VC_T Entry, *pEntry;
	unsigned int i,num;
    unsigned int ifIndex=0;
    char wanif[8];
    char strTblID[10], strDefTblID[10];
#ifdef CONFIG_IPV6
    char ipv6Enable =-1;
#endif
	char lanNetAddr[20];

	getPrimaryLanNetAddrInfo(lanNetAddr,sizeof(lanNetAddr));

	pEntry = &Entry;
	num = mib_chain_total( MIB_ATM_VC_TBL );
	for( i=0; i<num;i++ )
	{
		if ( !mib_chain_get( MIB_ATM_VC_TBL, i, (void*)pEntry ) )
			continue;

		if (pEntry->cmode != CHANNEL_MODE_PPPOE){
			continue;
		}

        ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));
        if (!strcmp(wanif, ifname))
        {
            ifIndex = pEntry->ifIndex;
            break;
        }
	}

    if (0 == ifIndex)
        return;

    snprintf(strTblID, 10, "%d", caculate_tblid(ifIndex));
    //ip route add default dev ppp0 table 48
    va_cmd("/usr/bin/ip", 7, 1, "route", "add", "default", "dev", ifname, "table", strTblID);
	// change rule is for avoiding route to be nas0_x when reboot.
    va_cmd("/usr/bin/ip", 7, 1, "route", "change", "default", "dev", ifname, "table", strTblID);
	va_cmd("/usr/bin/ip", 7, 1, "route", "add", lanNetAddr, "dev", LANIF, "table", strTblID);
	// default table
	if (pEntry->applicationtype&X_CT_SRV_INTERNET) {
        snprintf(strDefTblID, 10, "%d", PMAP_DEFAULT_TBLID);
        //ip ro change default dev ppp0 table 252
        va_cmd("/usr/bin/ip", 7, 1, "route", "change", "default", "dev", ifname, "table", strDefTblID);
#if defined(CONFIG_USER_PPPOE_PROXY)
		if(pEntry->PPPoEProxyEnable){
            //route add default ppp0
            va_cmd(ROUTE, 3, 1, "add", "default", ifname);
            //iptables -I  FORWARD  1 -i ppp+ -o ppp0 -j ACCEPT
            //va_cmd(IPTABLES, 9, 1, "-I", "FORWARD", "1", "-i", "ppp+", "-o", ifname, "-j", "ACCEPT");
		}
#endif
	}

#ifdef CONFIG_IPV6
	mib_get_s(MIB_V6_IPV6_ENABLE, (void *)&ipv6Enable, sizeof(ipv6Enable));
	if(ipv6Enable == 1)
	{
        //ip -6 route flush table 48
        va_cmd("/usr/bin/ip", 5, 1, "-6", "route", "flush", "table", strTblID);
        //ip -6 route add default dev ppp0 table 48
        va_cmd("/usr/bin/ip", 8, 1, "-6", "route", "add", "default", "dev", ifname, "table", strTblID);

		// default table
		if (pEntry->applicationtype&X_CT_SRV_INTERNET) {
            //ip -6 route flush table 252
            va_cmd("/usr/bin/ip", 5, 1, "-6", "route", "flush", "table", strDefTblID);
            //ip -6 route add default dev ppp0 table 48
            va_cmd("/usr/bin/ip", 8, 1, "-6", "route", "add", "default", "dev", ifname, "table", strDefTblID);
		}
	}
#endif//end of CONFIG_IPV6
#endif//end of NEW_PORTMAPPING
}

/*
 *	setup the policy-routing for static link
 */
#if 0 //def IP_POLICY_ROUTING
static int setup_static_PolicyRouting(unsigned int ifIndex)
{
	int i, num, mark, ret, found;
	char str_mark[8], str_rtable[8], ipAddr[32], wanif[IFNAMSIZ];
	struct in_addr inAddr;
	MIB_CE_IP_QOS_T qEntry;

	ret = -1;

	if (ifIndex == DUMMY_IFINDEX)
		return ret;
	else {
		if (PPP_INDEX(ifIndex) != DUMMY_PPP_INDEX) {
			// PPP interface
			return ret;
		}
		else {
			// vc interface
			ifGetName(ifIndex,wanif,sizeof(wanif));
			snprintf(str_rtable, 8, "%d", VC_INDEX(ifIndex)+PR_VC_START);
			if (getInAddr( wanif, DST_IP_ADDR, (void *)&inAddr) == 1)
			{
				strcpy(ipAddr, inet_ntoa(inAddr));
			}
			else
				return ret;
		}

		if (!getInFlags(wanif, 0))
			return ret;	// interface not found
	}

	num = mib_chain_total(MIB_IP_QOS_TBL);
	found = 0;
	// set advanced-routing rule
	for (i=0; i<num; i++) {
		if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void *)&qEntry))
			continue;

		if (qEntry.outif == ifIndex) {

			found = 1;
			mark = get_classification_mark(i);
			if (mark != 0) {
				snprintf(str_mark, 8, "%x", mark);
				// Don't forget to point out that fwmark with
				// ipchains/iptables is a decimal number, but that
				// iproute2 uses hexadecimal number.
				// ip ru add fwmark xxx table 3
				va_cmd("/usr/bin/ip", 6, 1, "ru", "add", "fwmark", str_mark, "table", str_rtable);
				// ip ro add default via ipaddr dev vc0 table 3
			}
		}
	}
	if (found) {
		// ip ro add default via ipaddr dev vc0 table 3
		va_cmd("/usr/bin/ip", 9, 1, "ro", "add", "default", "via", ipAddr, "dev", wanif, "table", str_rtable);
		ret = 0;
	}

	return ret;
}
#endif

int generate_ifupdown_core_script(void){
	FILE *fp=NULL;
	DIR *dir=NULL;
	char filename[128]={0};

	dir = opendir(IFUPDOWN_CONFIG_FOLDER);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", IFUPDOWN_CONFIG_FOLDER);
	}
	if(dir)closedir(dir);

	/* generate if-up script for WAN interface */
	snprintf(filename, sizeof(filename), "%s/if-up", IFUPDOWN_CONFIG_FOLDER);
	if (fp = fopen(filename, "w+")) {
		fprintf(fp, "#!/bin/sh\n\n");

		fprintf(fp, "######### Script Globle Variable ###########\n");
		fprintf(fp, "DATE=`date`\n");
		fprintf(fp, "SCRIPT=%s/if-${1}-up\n", IFUPDOWN_UP_SCRIPT_DIR);
		fprintf(fp, "######### Script Globle Variable ###########\n\n");

		/* TODO: Part 1: Common Script, this part should independent to any interfaces. */
		/* Part 2: Call interface shell script */
		fprintf(fp, "if [ -f ${SCRIPT} ]; then\n");
			fprintf(fp, "\techo \"[${DATE}] Run ${SCRIPT}\" >> %s\n", IFUPDOWN_DEBUG_FILE);
			fprintf(fp, "\techo \"$(tail -n %d %s)\" > %s\n", IFUPDOWN_MAX_DEBUG_LINE, IFUPDOWN_DEBUG_FILE, IFUPDOWN_DEBUG_FILE);
			fprintf(fp, "\t${SCRIPT}\n");
		fprintf(fp, "fi\n");

		fclose(fp);
		if(chmod(filename, 484) != 0)
			printf("chmod failed: %s %d\n", __func__, __LINE__);
	}
	else{
		printf("[%s:%d]open %s failed\n", __func__, __LINE__, filename);
		return -1;
	}

	/* generate if-down script for WAN interface */
	snprintf(filename, sizeof(filename), "%s/if-down", IFUPDOWN_CONFIG_FOLDER);
	if (fp = fopen(filename, "w+")) {
		fprintf(fp, "#!/bin/sh\n\n");

		fprintf(fp, "######### Script Globle Variable ###########\n");
		fprintf(fp, "DATE=`date`\n");
		fprintf(fp, "SCRIPT=%s/if-${1}-down\n", IFUPDOWN_DOWN_SCRIPT_DIR);
		fprintf(fp, "######### Script Globle Variable ###########\n\n");

		/* TODO: Part 1: Common Script, this part should independent to any interfaces. */
		/* Part 2: Call interface shell script */
		fprintf(fp, "if [ -f ${SCRIPT} ]; then\n");
			fprintf(fp, "\techo \"[${DATE}] Run ${SCRIPT}\" >> %s\n", IFUPDOWN_DEBUG_FILE);
			fprintf(fp, "\techo \"$(tail -n %d %s)\" > %s\n", IFUPDOWN_MAX_DEBUG_LINE, IFUPDOWN_DEBUG_FILE, IFUPDOWN_DEBUG_FILE);
			fprintf(fp, "\t${SCRIPT}\n");
		fprintf(fp, "fi\n");

		fclose(fp);
		if(chmod(filename, 484) != 0)
			printf("chmod failed: %s %d\n", __func__, __LINE__);
	}
	else{
		printf("[%s:%d]open %s failed\n", __func__, __LINE__, filename);
		return -1;
	}

	return 0;
}

/*
 *	generate if-up script for WAN interface
 *  return:
 *    -1: Fail
 *     0: Success
 */
static int generate_ifup_script(MIB_CE_ATM_VC_Tp pEntry)
{
	FILE *fp=NULL;
	char wanif[IFNAMSIZ]={0}, phyif[IFNAMSIZ]={0}, ifup_path[256]={0}, phy_ifup_path[256]={0};
	DIR* dir=NULL;

	if(!pEntry){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return -1;
	}

	dir = opendir(IFUPDOWN_CONFIG_FOLDER);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", IFUPDOWN_CONFIG_FOLDER);
	}
	if(dir)closedir(dir);

	dir = opendir(IFUPDOWN_UP_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", IFUPDOWN_UP_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));

	/* ifup script */
	snprintf(ifup_path, sizeof(ifup_path), "%s/"IFUPDOWN_UP_SCRIPT_NAME, IFUPDOWN_UP_SCRIPT_DIR, wanif);
	fp = fopen(ifup_path, "w+");
	if (!fp) {
		fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, ifup_path);
		goto Error;
	}

	fprintf(fp, "#!/bin/sh\n\n");

	fprintf(fp, "######### Script Globle Variable ###########\n");
	fprintf(fp, "IFACE=%s\n", wanif);
	fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
	fprintf(fp, "######### Script Globle Variable ###########\n\n");

	switch(pEntry->cmode){
	case CHANNEL_MODE_PPPOA:
	case CHANNEL_MODE_PPPOE:
#ifdef CONFIG_USER_PPPD
		if (PPP_INDEX(pEntry->ifIndex) != DUMMY_PPP_INDEX) {

			/* add IPv6 DoD default route */
			if (pEntry->IpProtocol & IPVER_IPV6 &&
				pEntry->pppCtype == CONNECT_ON_DEMAND &&
				pEntry->applicationtype & X_CT_SRV_INTERNET) { // On-demand
				fprintf(fp, "/usr/bin/ip -6 route add default dev ${IFACE}\n");
			}
		}else{
			fprintf(stderr, "[%s] Error: DUMMY_PPP_INDEX\n.", __FUNCTION__);
		}
#endif
		break;
	default:
		break;
	}

	fclose(fp);
	if(chmod(ifup_path, 484) != 0)
		fprintf(stderr, "[%s %d] chmod failed: %s\n", __func__, __LINE__, ifup_path);

	/* physical ifup script for ppp device */
	switch(pEntry->cmode){
	case CHANNEL_MODE_PPPOA:
	case CHANNEL_MODE_PPPOE:
#ifdef CONFIG_USER_PPPD
		if (PPP_INDEX(pEntry->ifIndex) != DUMMY_PPP_INDEX) {

			ifGetName(PHY_INTF(pEntry->ifIndex), phyif, sizeof(phyif));
			snprintf(phy_ifup_path, sizeof(phy_ifup_path), "%s/"IFUPDOWN_UP_SCRIPT_NAME, IFUPDOWN_UP_SCRIPT_DIR, phyif);
			if (fp = fopen(phy_ifup_path, "w+")) {
				fprintf(fp, "#!/bin/sh\n\n");

				fprintf(fp, "######### Script Globle Variable ###########\n");
				fprintf(fp, "IFACE=%s\n", phyif);
				fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
				fprintf(fp, "PPPIFNAME=%s\n", wanif);
				fprintf(fp, "######### Script Globle Variable ###########\n\n");

				fprintf(fp, "#disconnect old ppp session\n");
				fprintf(fp, "sysconf send_unix_sock_message %s do_disconn_old_ppp_session ${PPPIFNAME}\n\n", SYSTEMD_USOCK_SERVER);

				fclose(fp);
				if(chmod(phy_ifup_path, 484) != 0)
					fprintf(stderr, "[%s %d] chmod failed: %s\n", __func__, __LINE__, phy_ifup_path);
			}else{
				fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, phy_ifup_path);
			}
		}
		else {
			fprintf(stderr, "[%s] Error: DUMMY_PPP_INDEX\n.", __FUNCTION__);
		}
#endif //CONFIG_USER_PPPD
		break;
	default:
		break;
	}

	return 0;
Error:
	if(fp)fclose(fp);
	return -1;
}

/*
 *	generate if-down script for WAN interface
 *  return:
 *    -1: Fail
 *     0: Success
 */
static int generate_ifdown_script(MIB_CE_ATM_VC_Tp pEntry)
{
	FILE *fp=NULL;
	char wanif[IFNAMSIZ]={0}, phyif[IFNAMSIZ]={0}, ifdown_path[128]={0}, phy_ifdown_path[128]={0};
	DIR* dir=NULL;
	unsigned char duid_mode=(DHCPV6_DUID_TYPE_T)DHCPV6_DUID_LLT;

	if(!pEntry){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return -1;
	}

	dir = opendir(IFUPDOWN_CONFIG_FOLDER);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", IFUPDOWN_CONFIG_FOLDER);
	}
	if(dir)closedir(dir);

	dir = opendir(IFUPDOWN_DOWN_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", IFUPDOWN_DOWN_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));
	mib_get_s(MIB_DHCPV6_CLIENT_DUID_MODE, (void *)&duid_mode, sizeof(duid_mode));

	/* ifdown script */
	snprintf(ifdown_path, sizeof(ifdown_path), "%s/"IFUPDOWN_DOWN_SCRIPT_NAME, IFUPDOWN_DOWN_SCRIPT_DIR, wanif);
	fp = fopen(ifdown_path, "w+");
	if (!fp) {
		fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, ifdown_path);
		goto Error;
	}

	fprintf(fp, "#!/bin/sh\n\n");

	fprintf(fp, "######### Script Globle Variable ###########\n");
	fprintf(fp, "IFACE=%s\n", wanif);
	fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
#if defined(CONFIG_USER_DHCP_ISC) && defined(CONFIG_USER_PPPD)
	fprintf(fp, "DHCPCV6_LEASEFILE=%s.%s\n", PATH_DHCLIENT6_LEASE, wanif);
	fprintf(fp, "DHCPCV6_PIDFILE=%s.%s\n", PATH_DHCLIENT6_PID, wanif);
#endif
	fprintf(fp, "######### Script Globle Variable ###########\n\n");

	switch(pEntry->cmode){
	case CHANNEL_MODE_PPPOA:
	case CHANNEL_MODE_PPPOE:
		/* restart ipv6 server on ipv6cp-down script */
		break;
	case CHANNEL_MODE_IPOE:
	case CHANNEL_MODE_RT1483:
	case CHANNEL_MODE_RT1577:
	case CHANNEL_MODE_DSLITE:
#if defined(CONFIG_USER_DHCP_ISC) && defined(CONFIG_USER_PPPD)
		fprintf(fp, "# IPv6 should not work anymore, kill dhclient then delete pid and lease file.\n");
		fprintf(fp, "if [ -f ${DHCPCV6_PIDFILE} ]; then\n");
		fprintf(fp, "\tif [ -f ${DHCPCV6_LEASEFILE} ]; then\n");
		fprintf(fp, "\t\t/bin/dhclient -6 -r -1 -lf ${DHCPCV6_LEASEFILE} -pf ${DHCPCV6_PIDFILE} %s -N -P", wanif);
		if (duid_mode == DHCPV6_DUID_LL)
			fprintf(fp, " -D LL\n");
		else
			fprintf(fp, " -D LLT\n");
		fprintf(fp, "\t\trm ${DHCPCV6_LEASEFILE}\n");
		fprintf(fp, "\telse\n");
		fprintf(fp, "\t\t[ -f ${DHCPCV6_PIDFILE} ] && kill `cat ${DHCPCV6_PIDFILE}`\n");
		fprintf(fp, "\tfi\n");
		fprintf(fp, "fi\n");
		fprintf(fp, "\n");

		if(pEntry->Ipv6DhcpRequest & (M_DHCPv6_REQ_IAPD)){
			fprintf(fp, "PREFIX_MODE=`mib get PREFIXINFO_PREFIX_MODE`\n");
			fprintf(fp, "PREFIX_WANCONN=`mib get PREFIXINFO_DELEGATED_WANCONN`\n\n");

			fprintf(fp, "# if intf is PD wan, it should clean LAN server\n");
			fprintf(fp, "if [[ ${PREFIX_MODE#*=} -eq %d ]]; then\n", IPV6_PREFIX_DELEGATION);
			fprintf(fp, "\tif [[ ${PREFIX_WANCONN#*=} -eq ${IFINDEX} ]]; then\n");
				if(pEntry->pppDebug){
					fprintf(fp, "\t\techo \"$0: restart lan server\" >> %s\n", IFUPDOWN_DEBUG_FILE);
				}

				fprintf(fp, "\t\t#Delete LAN IPv6 IP and Restart IPv6 Lan Server, unused parameter(::/64)\n");
				fprintf(fp, "\t\tsysconf send_unix_sock_message %s do_ipv6_iapd ::/64 %s\n", SYSTEMD_USOCK_SERVER, wanif);
			fprintf(fp, "\tfi\n");
			fprintf(fp, "fi\n");
			fprintf(fp, "\n");
		}
#endif
		break;
	default:
		break;
	}

	fclose(fp);
	if(chmod(ifdown_path, 484) != 0)
		printf("chmod failed: %s %d\n", __func__, __LINE__);

	/* physical ifdown script for ppp device */
	switch(pEntry->cmode){
	case CHANNEL_MODE_PPPOA:
	case CHANNEL_MODE_PPPOE:
#ifdef CONFIG_USER_PPPD
		if (PPP_INDEX(pEntry->ifIndex) != DUMMY_PPP_INDEX) {

			ifGetName(PHY_INTF(pEntry->ifIndex), phyif, sizeof(phyif));
			snprintf(phy_ifdown_path, sizeof(phy_ifdown_path), "%s/"IFUPDOWN_DOWN_SCRIPT_NAME, IFUPDOWN_DOWN_SCRIPT_DIR, phyif);
			if (fp = fopen(phy_ifdown_path, "w+")) {
				fprintf(fp, "#!/bin/sh\n\n");

				fprintf(fp, "######### Script Globle Variable ###########\n");
				fprintf(fp, "IFACE=%s\n", phyif);
				fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
				fprintf(fp, "PPP_IF=%s\n", wanif);
				fprintf(fp, "PIDFILE=%s/%s.pid\n", (char *)PPPD_CONFIG_FOLDER, wanif);
				fprintf(fp, "BACKUP_PIDFILE=%s/%s.pid_back\n", (char *)PPPD_CONFIG_FOLDER, wanif);
				fprintf(fp, "######### Script Globle Variable ###########\n\n");

				fprintf(fp, "#send SIGHUP to disconnect %s session\n", wanif);
				fprintf(fp, "if [ -f ${PIDFILE} ]; then\n");
					fprintf(fp, "\tkill -SIGHUP `cat ${PIDFILE}`\n");
				fprintf(fp, "else\n");
					fprintf(fp, "\tif [ -f ${BACKUP_PIDFILE} ]; then\n");
						fprintf(fp, "\t\tkill -SIGHUP `cat ${BACKUP_PIDFILE}`\n");
					fprintf(fp, "\tfi\n");
				fprintf(fp, "fi\n");

				fclose(fp);
				if(chmod(phy_ifdown_path, 484) != 0)
					printf("chmod failed: %s %d\n", __func__, __LINE__);
			}
			else{
				fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, phy_ifdown_path);
			}
		}
#endif
		break;
	default:
		break;
	}

	return 0;
Error:
	if(fp)fclose(fp);
	return -1;
}

/*
 *	generate spppd ipup script for WAN interface
 */
static int generate_spppd_ipup_script(MIB_CE_ATM_VC_Tp pEntry)
{
	int mark, ret, len;
	FILE *fp;
	char wanif[8], ifup_path[32];
	char ipv6Enable =-1;


	ret = 0;

	ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));

	if (PPP_INDEX(pEntry->ifIndex) != DUMMY_PPP_INDEX) {
		// PPP interface
		/* IPv4 script */
		snprintf(ifup_path, 32, "/var/ppp/ifup_%s", wanif);
		if (fp = fopen(ifup_path, "w+")) {
			fprintf(fp, "#!/bin/sh\n\n");

			fclose(fp);
			if(chmod(ifup_path, 484) != 0)
				printf("chmod failed: %s %d\n", __func__, __LINE__);
		}
		else
			ret = -1;

		// Added by Mason Yu. For IPV6
#ifdef CONFIG_IPV6
		/* IPv6 script */
		mib_get_s(MIB_V6_IPV6_ENABLE, (void *)&ipv6Enable, sizeof(ipv6Enable));
		if(ipv6Enable == 1)
		{
			snprintf(ifup_path, 32, "/var/ppp/ifupv6_%s", wanif);
			if (fp = fopen(ifup_path, "w+")) {
				unsigned char Ipv6AddrStr[48], RemoteIpv6AddrStr[48];
				unsigned char pidfile[64], leasefile[64], cffile[64];
				unsigned char value[128];

				fprintf(fp, "#!/bin/sh\n\n");
				if (((pEntry->AddrMode & IPV6_WAN_STATIC)) == IPV6_WAN_STATIC) {
					inet_ntop(PF_INET6, (struct in6_addr *)pEntry->Ipv6Addr, Ipv6AddrStr, sizeof(Ipv6AddrStr));
					inet_ntop(PF_INET6, (struct in6_addr *)pEntry->RemoteIpv6Addr, RemoteIpv6AddrStr, sizeof(RemoteIpv6AddrStr));

					// Add WAN static IP
					len = sizeof(Ipv6AddrStr) - strlen(Ipv6AddrStr);
					snprintf(Ipv6AddrStr + strlen(Ipv6AddrStr), len, "/%d", pEntry->Ipv6AddrPrefixLen);
					fprintf(fp, "/bin/ifconfig %s add %s\n", wanif, Ipv6AddrStr);

					// Add default gw
					if (isDefaultRouteWan(pEntry)) {
						// route -A inet6 add ::/0 gw 3ffe::0200:00ff:fe00:0100 dev ppp0
						fprintf(fp, "/bin/route -A inet6  add ::/0 gw %s dev %s\n", RemoteIpv6AddrStr, wanif);
					}
				}
				else
					/* need get gateway information from RA, so for dynamic address case always enable accept_ra_defrtr */
					fprintf(fp, "echo 1 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr\n", wanif);
#ifdef DHCPV6_ISC_DHCP_4XX
				// Start DHCPv6 client
				// dhclient -6 ppp0 -d -q -lf /var/dhcpcV6.leases.ppp0 -pf /var/run/dhcpcV6.pid.ppp0 -cf /var/dhcpcV6.conf.ppp0 -N -P
#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
				/* Auto mode PPPoE IPv6 will do at systemd trigger, so we don't do it at ppp ipcpv6 up. */
				if (pEntry->AddrMode == IPV6_WAN_AUTO_DETECT_MODE)
					printf("IPV6_WAN_AUTO_DETECT_MODE doesn't need to start DHCPv6 client in file %s\n", ifup_path);
				else
#endif
				 if (pEntry->Ipv6Dhcp == 1) {
					snprintf(leasefile, 64, "%s.%s", PATH_DHCLIENT6_LEASE, wanif);
					snprintf(pidfile, 64, "%s.%s", PATH_DHCLIENT6_PID, wanif);
					snprintf(cffile, 64, "%s.%s", PATH_DHCLIENT6_CONF, wanif);
					make_dhcpcv6_conf(pEntry);

					//fix bug :mul-wan, pppoe+dhcpv6 ->reboot->ppp0 has two ipv6 address dhcp from server
		                        //when restart dhclient , it should clean the old dhcpv6 address,please do not user 'flush',casue SLAAC address may have timing issue.
		                        //The prefix len depends on dhcp-4.x.x/client/dhc6.c dhc6_marshall_values fun $(new_ip6_prefixlen)
					snprintf(value, 128, "ip -6 addr del $(cat %s  | awk '/iaaddr/' | awk '{print $2}' | awk 'END {print}')/128 dev %s\n",leasefile,wanif);
		                        fprintf(fp, value);

					//when start dhclient, it should clean leasefile which maybe save previous address info and will lead it can't get new IP this time.
					snprintf(value, 128, "echo \" \" > %s\n", leasefile);
					fprintf(fp, value);
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
					/* Use DUID_LL which generated by rtk to request same IA info.
					 * But PPP may not get same IA info because session ID won't be same and PPP will release every time */
					char duid_buf[128] = {0}; //If 128 is not enough, you can add this size.
					char tmp_cmd[256] = {0};
					rtk_get_duid_ll_for_isc_dhcp_lease_file(pEntry, wanif, duid_buf, sizeof(duid_buf));
					/*
					 * isc-dhcpv6 duid format: default-duid "\000\003\000\001\001\036L\007h\003";
					 *                        default-duid "\000\001\000\001$\345K3\000\032+3\352\023";
					 */
					/* use ' to wrap the string which have ". */
					snprintf(tmp_cmd, sizeof(tmp_cmd), "echo 'default-duid \"%s\";' > %s\n", duid_buf, leasefile);
					fprintf(fp, tmp_cmd);
#endif

					snprintf(value, 128, "/bin/dhclient -6 %s -d -q -lf %s -pf %s -cf %s ", wanif, leasefile, pidfile, cffile);
					fprintf(fp, value);
					// for DHCP441 or later
					#ifdef CONFIG_USER_DHCPV6_ISC_DHCP441
					fprintf(fp, "--dad-wait-time 5 ");
					fprintf(fp, "-D LL ");
					#endif

					/* Only request other information(like DNS, DNSSL etc...) not include any IA info.
					 * Note: After receive server REPLY, DHCP6c daemon will exit and write DNS info to lease file.
					 *       Now we use it at TR069, VOIP and TR069_VOICE only .
					 */
					if ((pEntry->dnsv6Mode == 1) && (pEntry->Ipv6DhcpRequest == M_DHCPv6_REQ_NONE)) {
						fprintf(fp, "-S ");
					}
					else {
						// Request Address
						if (pEntry->Ipv6DhcpRequest & M_DHCPv6_REQ_IANA) {
							// for DHCP441 or later
							fprintf(fp, "-N ");
						}

						if ((pEntry->Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)) {
							fprintf(fp, "-P ");
						}
					}
					fprintf(fp, "&\n");
				}
#endif
				fclose(fp);
				if(chmod(ifup_path, 484) != 0)
					printf("chmod failed: %s %d\n", __func__, __LINE__);
			}
		}  //End of if(ipv6Enable == 1)
#endif
	}
	else {
		// not supported till now
		return -1;
	}

	return ret;
}

/*
 *	generate spppd ipdown script for WAN interface
 */
static int generate_spppd_ipdown_script(MIB_CE_ATM_VC_Tp pEntry)
{
	int mark, ret;
	FILE *fp;
	char wanif[6], ifdown_path[32];
	char devname[IFNAMSIZ];
	unsigned char vChar;
	unsigned char pidfile[64], leasefile[64];
	unsigned char duid_mode=(DHCPV6_DUID_TYPE_T)DHCPV6_DUID_LLT;

	ret = 0;

	if (PPP_INDEX(pEntry->ifIndex) != DUMMY_PPP_INDEX) {
		// PPP interface
		snprintf(wanif, 6, "ppp%u", PPP_INDEX(pEntry->ifIndex));
		snprintf(ifdown_path, sizeof(ifdown_path), "/var/ppp/ifdown_%s", wanif);
		if (fp = fopen(ifdown_path, "w+")) {
			fprintf(fp, "#!/bin/sh\n\n");

			fclose(fp);
			if(chmod(ifdown_path, 484) != 0)
				printf("chmod failed: %s %d\n", __func__, __LINE__);
		}
		else
			ret = -1;

#ifdef CONFIG_IPV6
		snprintf(ifdown_path, sizeof(ifdown_path), "/var/ppp/ifdownv6_%s", wanif);
		if (fp = fopen(ifdown_path, "w+")) {
			fprintf(fp, "#!/bin/sh\n\n");
#ifdef DHCPV6_ISC_DHCP_4XX
#ifdef CONFIG_BHS
			/* For Telefonica IPv6 Test Plan, Test Case ID 20178, if PPPoE session is down,			   *
			* LAN PC global IPv6 address must change the state from "Prefered" to "Obsolete".		   *
			* So here, if PPPoE connection is down, and DHCPv6 is auto mode, kill the DHCPv6 process  */

			mib_get_s(MIB_DHCPV6_CLIENT_DUID_MODE, (void *)&duid_mode, sizeof(duid_mode));
			mib_get_s(MIB_DHCPV6_MODE, (void *)&vChar, sizeof(vChar));
			if (vChar == 3) {  // 3:auto mode
				fprintf(fp, "kill `cat /var/run/dhcpd6.pid`\n");
			}
#endif
			if (pEntry->Ipv6Dhcp == 1) {
				snprintf(leasefile, sizeof(leasefile), "%s.%s", PATH_DHCLIENT6_LEASE, wanif);
				snprintf(pidfile, sizeof(pidfile), "%s.%s", PATH_DHCLIENT6_PID, wanif);
				/* release dhcpv6 client if exist.
				 * Use -l to try to get lease once to avoid long duration. */
				fprintf(fp, "/bin/dhclient -6 -r -1 -lf %s -pf %s %s -N -P", leasefile, pidfile, wanif);
				if (duid_mode == DHCPV6_DUID_LL)
					fprintf(fp, " -D LL\n");
				else
					fprintf(fp, " -D LLT\n");
			}
			else if (pEntry->AddrMode == IPV6_WAN_AUTO_DETECT_MODE && pEntry->cmode == CHANNEL_MODE_PPPOE)
			{
				fprintf(fp, "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/autoconf\n", wanif);
				/* release dhcpv6 client if exist.
				 * Use -l to try to get lease once to avoid long duration. */
				snprintf(leasefile, sizeof(leasefile), "%s.%s", PATH_DHCLIENT6_LEASE, wanif);
				snprintf(pidfile, sizeof(pidfile), "%s.%s", PATH_DHCLIENT6_PID, wanif);
				fprintf(fp, "[ -f %s ] && /bin/dhclient -6 -r -1 -lf %s -pf %s %s -N -P", pidfile, leasefile, pidfile, wanif);
				if (duid_mode == DHCPV6_DUID_LL)
					fprintf(fp, " -D LL\n");
				else
					fprintf(fp, " -D LLT\n");
			}

			fprintf(fp, "\n");
			fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
			fprintf(fp, "DHCPCV6_LEASEFILE=%s.%s\n", PATH_DHCLIENT6_LEASE, wanif);
			fprintf(fp, "DHCPCV6_PIDFILE=%s.%s\n", PATH_DHCLIENT6_PID, wanif);
			fprintf(fp, "\n");

			if(pEntry->Ipv6DhcpRequest & (M_DHCPv6_REQ_IAPD)){
				fprintf(fp, "PREFIX_MODE=`mib get PREFIXINFO_PREFIX_MODE`\n");
				fprintf(fp, "PREFIX_WANCONN=`mib get PREFIXINFO_DELEGATED_WANCONN`\n\n");

				fprintf(fp, "# if intf is PD wan, it should clean LAN server\n");
				fprintf(fp, "if [[ ${PREFIX_MODE#*=} -eq %d ]]; then\n", IPV6_PREFIX_DELEGATION);
				fprintf(fp, "\tif [[ ${PREFIX_WANCONN#*=} -eq ${IFINDEX} ]]; then\n");
					fprintf(fp, "\t\t#Delete LAN IPv6 IP and Restart IPv6 Lan Server, unused parameter(::/64)\n");
					fprintf(fp, "\t\tsysconf send_unix_sock_message %s do_ipv6_iapd ::/64 %s\n", SYSTEMD_USOCK_SERVER, wanif);
				fprintf(fp, "\tfi\n");
				fprintf(fp, "fi\n");
				fprintf(fp, "\n");
			}
#endif
#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
			snprintf(pidfile, sizeof(pidfile), "%s_%s", (char *)RA_INFO_FILE, wanif);
			fprintf(fp, "rm %s > /dev/null 2>&1\n", pidfile);
#endif
			fclose(fp);
			if(chmod(ifdown_path, 484) != 0)
				printf("chmod failed: %s %d\n", __func__, __LINE__);
		}
		else
			ret = -1;
#endif
	}
	else {
		// not supported till now
		return -1;
	}

	return ret;
}

#ifdef CONFIG_USER_PPPD
int generate_pppd_core_script(void){
	FILE *fp=NULL;
	DIR *dir=NULL;
	char filename[128]={0};

	dir = opendir(PPPD_CONFIG_FOLDER);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_CONFIG_FOLDER);
	}
	if(dir)closedir(dir);

	/* This Script is RTK hack, A program or script which is executed when the ppp device is created
	 * and ppp daemon will wait script finish. */
	snprintf(filename, sizeof(filename), "%s/ppp-init", PPPD_CONFIG_FOLDER);
	if (fp = fopen(filename, "w+")) {
		fprintf(fp, "#!/bin/sh\n\n");
		fprintf(fp, "######### for debug###########\n");
		fprintf(fp, "#echo \"$0: DEVICE=${DEVICE} IFNAME=${IFNAME} SESSION=${SESSION}\" >> %s\n", PPPD_DEBUG_FILE);
		fprintf(fp, "######### for debug###########\n\n");

		fprintf(fp, "######### Script Globle Variable ###########\n");
		fprintf(fp, "DATE=`date`\n");
		fprintf(fp, "SCRIPT=%s/init-${IFNAME}\n", PPPD_INIT_SCRIPT_DIR);
		fprintf(fp, "######### Script Globle Variable ###########\n\n");

		/* TODO: Part 1: Common Script, this part should independent to any ppp interfaces. */
		/* Part 2: Call interface shell script */
		fprintf(fp, "if [ -f ${SCRIPT} ]; then\n");
			fprintf(fp, "\techo \"[${DATE}] Run ${SCRIPT}\" >> %s\n", PPPD_DEBUG_FILE);
			fprintf(fp, "\techo \"$(tail -n %d %s)\" > %s\n", PPPD_MAX_DEBUG_LINE, PPPD_DEBUG_FILE, PPPD_DEBUG_FILE);
			fprintf(fp, "\t${SCRIPT}\n");
		fprintf(fp, "fi\n");

		fclose(fp);
		if(chmod(filename, 484) != 0)
			printf("chmod failed: %s %d\n", __func__, __LINE__);
	}
	else{
		printf("[%s:%d]open %s failed\n", __func__, __LINE__, filename);
		return -1;
	}

	/*  A program or script which is executed just before the ppp network interface is brought up.
	 *  It is executed with the same parameters as the ip-up script (below).
	 *  At this point the interface exists and has IP addresses assigned but is still down.
	 *  This can be used to add firewall rules before any IP traffic can pass through the interface.
	 *  Pppd will wait for this script to finish before bringing the interface up,
	 *  so this script should run quickly. */
	snprintf(filename, sizeof(filename), "%s/ip-pre-up", PPPD_CONFIG_FOLDER);
	if (fp = fopen(filename, "w+")) {
		fprintf(fp, "#!/bin/sh\n\n");
		fprintf(fp, "######### for debug###########\n");
		fprintf(fp, "#echo \"$0: DEVICE=${DEVICE} IFNAME=${IFNAME} IPLOCAL=${IPLOCAL}\" >> %s\n", PPPD_DEBUG_FILE);
		fprintf(fp, "######### for debug###########\n\n");

		fclose(fp);
		if(chmod(filename, 484) != 0)
			printf("chmod failed: %s %d\n", __func__, __LINE__);
	}
	else{
		printf("[%s:%d]open %s failed\n", __func__, __LINE__, filename);
		return -1;
	}

	/* A program or script which is executed when the link is available for sending and receiving IP packets
	 * (that is, IPCP has come up). It is executed with the parameters */
	snprintf(filename, sizeof(filename), "%s/ip-up", PPPD_CONFIG_FOLDER);
	if (fp = fopen(filename, "w+")) {
		fprintf(fp, "#!/bin/sh\n\n");
		fprintf(fp, "######### for debug###########\n");
		fprintf(fp, "#echo \"$0: DEVICE=${DEVICE} IFNAME=${IFNAME} IPLOCAL=${IPLOCAL}\" >> %s\n", PPPD_DEBUG_FILE);
		fprintf(fp, "######### for debug###########\n\n");

		fprintf(fp, "######### Script Globle Variable ###########\n");
		fprintf(fp, "DATE=`date`\n");
		fprintf(fp, "SCRIPT=%s/ip-${IFNAME}-up\n", PPPD_V4_UP_SCRIPT_DIR);
		fprintf(fp, "######### Script Globle Variable ###########\n\n");

		/* TODO: Part 1: Common Script, this part should independent to any ppp interfaces. */
		/* Part 2: Call interface shell script */
		fprintf(fp, "if [ -f ${SCRIPT} ]; then\n");
			fprintf(fp, "\techo \"[${DATE}] Run ${SCRIPT}\" >> %s\n", PPPD_DEBUG_FILE);
			fprintf(fp, "\techo \"$(tail -n %d %s)\" > %s\n", PPPD_MAX_DEBUG_LINE, PPPD_DEBUG_FILE, PPPD_DEBUG_FILE);
			fprintf(fp, "\t${SCRIPT}\n");
		fprintf(fp, "fi\n");

		fclose(fp);
		if(chmod(filename, 484) != 0)
			printf("chmod failed: %s %d\n", __func__, __LINE__);
	}
	else{
		printf("[%s:%d]open %s failed\n", __func__, __LINE__, filename);
		return -1;
	}

	/* A program or script which is executed when the link is no longer available
	 * for sending and receiving IP packets.
	 * This script can be used for undoing the effects of the /etc/ppp/ip-up and /etc/ppp/ip-pre-up scripts.
	 * It is invoked in the same manner and with the same parameters as the ip-up script. */
	snprintf(filename, sizeof(filename), "%s/ip-down", PPPD_CONFIG_FOLDER);
	if (fp = fopen(filename, "w+")) {
		fprintf(fp, "#!/bin/sh\n\n");
		fprintf(fp, "######### for debug###########\n");
		fprintf(fp, "#echo \"$0: DEVICE=${DEVICE} IFNAME=${IFNAME} IPLOCAL=${IPLOCAL}\" >> %s\n", PPPD_DEBUG_FILE);
		fprintf(fp, "######### for debug###########\n\n");

		fprintf(fp, "######### Script Globle Variable ###########\n");
		fprintf(fp, "DATE=`date`\n");
		fprintf(fp, "SCRIPT=%s/ip-${IFNAME}-down\n", PPPD_V4_DOWN_SCRIPT_DIR);
		fprintf(fp, "######### Script Globle Variable ###########\n\n");

		/* TODO: Part 1: Common Script, this part should independent to any ppp interfaces. */
		/* Part 2: Call interface shell script */
		fprintf(fp, "if [ -f ${SCRIPT} ]; then\n");
			fprintf(fp, "\techo \"[${DATE}] Run ${SCRIPT}\" >> %s\n", PPPD_DEBUG_FILE);
			fprintf(fp, "\techo \"$(tail -n %d %s)\" > %s\n", PPPD_MAX_DEBUG_LINE, PPPD_DEBUG_FILE, PPPD_DEBUG_FILE);
			fprintf(fp, "\t${SCRIPT}\n");
		fprintf(fp, "fi\n");

		fclose(fp);
		if(chmod(filename, 484) != 0)
			printf("chmod failed: %s %d\n", __func__, __LINE__);
	}
	else{
		printf("[%s:%d]open %s failed\n", __func__, __LINE__, filename);
		return -1;
	}

#ifdef CONFIG_IPV6
	/* Like /etc/ppp/ip-up, except that it is executed when the link is available
	 * for sending and receiving IPv6 packets. It is executed with the parameters */
	snprintf(filename, sizeof(filename), "%s/ipv6-up", PPPD_CONFIG_FOLDER);
	if (fp = fopen(filename, "w+")) {
		fprintf(fp, "#!/bin/sh\n\n");
		fprintf(fp, "######### for debug###########\n");
		fprintf(fp, "#echo \"$0: DEVICE=${DEVICE} IFNAME=${IFNAME} IPLOCAL=${IPLOCAL}\" >> %s\n", PPPD_DEBUG_FILE);
		fprintf(fp, "######### for debug###########\n\n");

		fprintf(fp, "######### Script Globle Variable ###########\n");
		fprintf(fp, "DATE=`date`\n");
		fprintf(fp, "SCRIPT=%s/ipv6-${IFNAME}-up\n", PPPD_V6_UP_SCRIPT_DIR);
		fprintf(fp, "######### Script Globle Variable ###########\n\n");

		/* TODO: Part 1: Common Script, this part should independent to any ppp interfaces. */
		/* Part 2: Call interface shell script */
		fprintf(fp, "if [ -f ${SCRIPT} ]; then\n");
			fprintf(fp, "\techo \"[${DATE}] Run ${SCRIPT}\" >> %s\n", PPPD_DEBUG_FILE);
			fprintf(fp, "\techo \"$(tail -n %d %s)\" > %s\n", PPPD_MAX_DEBUG_LINE, PPPD_DEBUG_FILE, PPPD_DEBUG_FILE);
			fprintf(fp, "\t${SCRIPT}\n");
		fprintf(fp, "fi\n");

		fclose(fp);
		if(chmod(filename, 484) != 0)
			printf("chmod failed: %s %d\n", __func__, __LINE__);
	}
	else{
		printf("[%s:%d]open %s failed\n", __func__, __LINE__, filename);
		return -1;
	}

	/* Similar to /etc/ppp/ip-down, but it is executed when IPv6 packets can no longer be transmitted
	 * on the link. It is executed with the same parameters as the ipv6-up script. */
	snprintf(filename, sizeof(filename), "%s/ipv6-down", PPPD_CONFIG_FOLDER);
	if (fp = fopen(filename, "w+")) {
		fprintf(fp, "#!/bin/sh\n\n");
		fprintf(fp, "######### for debug###########\n");
		fprintf(fp, "#echo \"$0: DEVICE=${DEVICE} IFNAME=${IFNAME} IPLOCAL=${IPLOCAL}\" >> %s\n", PPPD_DEBUG_FILE);
		fprintf(fp, "######### for debug###########\n\n");

		fprintf(fp, "######### Script Globle Variable ###########\n");
		fprintf(fp, "DATE=`date`\n");
		fprintf(fp, "SCRIPT=%s/ipv6-${IFNAME}-down\n", PPPD_V6_DOWN_SCRIPT_DIR);
		fprintf(fp, "######### Script Globle Variable ###########\n\n");

		/* TODO: Part 1: Common Script, this part should independent to any ppp interfaces. */
		/* Part 2: Call interface shell script */
		fprintf(fp, "if [ -f ${SCRIPT} ]; then\n");
			fprintf(fp, "\techo \"[${DATE}] Run ${SCRIPT}\" >> %s\n", PPPD_DEBUG_FILE);
			fprintf(fp, "\techo \"$(tail -n %d %s)\" > %s\n", PPPD_MAX_DEBUG_LINE, PPPD_DEBUG_FILE, PPPD_DEBUG_FILE);
			fprintf(fp, "\t${SCRIPT}\n");
		fprintf(fp, "fi\n");

		fclose(fp);
		if(chmod(filename, 484) != 0)
			printf("chmod failed: %s %d\n", __func__, __LINE__);
	}
	else{
		printf("[%s:%d]open %s failed\n", __func__, __LINE__, filename);
		return -1;
	}
#endif

	return 0;
}

/*
 *  generate pppd init script
 *  this script called after ppp device create, and ppp daemon will wait script finish
 *  return:
 *    -1: Fail
 *     0: Success
 */
int generate_pppd_init_script(MIB_CE_ATM_VC_Tp pEntry)
{
	FILE *fp=NULL;
	char wanif[IFNAMSIZ]={0}, script_path[128]={0};
#ifdef CONFIG_IPV6
	char ipv6Enable =-1;
#endif
	DIR* dir=NULL;

	if(!pEntry){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return -1;
	}

	dir = opendir(PPPD_INIT_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_INIT_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));

	if (PPP_INDEX(pEntry->ifIndex) != DUMMY_PPP_INDEX) {
		/* ppp initial script */
		snprintf(script_path, sizeof(script_path), "%s/"PPPD_INIT_SCRIPT_NAME, PPPD_INIT_SCRIPT_DIR, wanif);
		if (fp = fopen(script_path, "w+")) {
			fprintf(fp, "#!/bin/sh\n\n");

			if(pEntry->pppDebug){
				fprintf(fp, "######### for debug###########\n");
				fprintf(fp, "echo \"$0: DEVICE=${DEVICE} IFNAME=${IFNAME} SESSION=${SESSION}\" >> %s\n", PPPD_DEBUG_FILE);
				fprintf(fp, "######### for debug###########\n\n");
			}

			fprintf(fp, "######### Script Globle Variable ###########\n");
			fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
			fprintf(fp, "PIDFILE=%s/${IFNAME}.pid\n", (char *)PPPD_CONFIG_FOLDER);
			fprintf(fp, "BACKUP_PIDFILE=%s/${IFNAME}.pid_back\n", (char *)PPPD_CONFIG_FOLDER);
			fprintf(fp, "######### Script Globle Variable ###########\n\n");

			fprintf(fp, "## PID file be remove when PPP link_terminated\n");
			fprintf(fp, "## it will cause delete ppp daemon fail, So backup it.\n");
			fprintf(fp, "if [ -f ${PIDFILE} ]; then\n");
				fprintf(fp, "\tcp -af ${PIDFILE} ${BACKUP_PIDFILE}\n");
			fprintf(fp, "fi\n");

			fprintf(fp, "#update ppp session id\n");
			fprintf(fp, "sysconf send_unix_sock_message %s do_update_ppp_session_id update ${IFNAME}\n\n", SYSTEMD_USOCK_SERVER);

#ifdef CONFIG_IPV6
			mib_get(MIB_V6_IPV6_ENABLE, (void *)&ipv6Enable);
			if (!(pEntry->IpProtocol & IPVER_IPV6) || !ipv6Enable) {
				fprintf(fp, "/bin/echo 1 > /proc/sys/net/ipv6/conf/${DEVICE}/disable_ipv6\n");
				fprintf(fp, "/bin/echo 1 > /proc/sys/net/ipv6/conf/${IFNAME}/disable_ipv6\n");
			}else{
				if (pEntry->cmode == CHANNEL_MODE_PPPOE) {
					// disable IPv6 for ethernet device
					fprintf(fp, "#disable IPv6 for ethernet device\n");
					fprintf(fp, "/bin/echo 1 > /proc/sys/net/ipv6/conf/${DEVICE}/disable_ipv6\n");
				}

				fprintf(fp, "#We should apply basic ipv6 proc setting before enable ipv6,\n");
				fprintf(fp, "#otherwise it will take default ipv6 setting.\n");
				fprintf(fp, "/bin/echo 1 > /proc/sys/net/ipv6/conf/${IFNAME}/disable_ipv6\n");

				if(pEntry->napt_v6)
				{
					fprintf(fp, "/bin/echo 1 > /proc/sys/net/ipv6/conf/${IFNAME}/forwarding\n");
				}else{
					fprintf(fp, "/bin/echo 0 > /proc/sys/net/ipv6/conf/${IFNAME}/forwarding\n");
				}

				/* If forwarding is enabled, RA are not accepted unless the special
				 * hybrid mode (accept_ra=2) is enabled.
				 */
				fprintf(fp, "/bin/echo 2 > /proc/sys/net/ipv6/conf/${IFNAME}/accept_ra\n");

				/* Enable autoconf when selected autoconf and accept default route in RA only when the WAN belong to default GW , IulianWu */
				/* If select Static IPV6 */
				if (pEntry->AddrMode & IPV6_WAN_STATIC)
				{
					fprintf(fp, "/bin/echo 0 > /proc/sys/net/ipv6/conf/${IFNAME}/accept_ra_defrtr\n");
				}
				else /* If select SLAAC mode, stateless and stateful DHCP mode. */
				{
					//need get gateway information from RA, so for dynamic address case always enable accept_ra_defrtr
					fprintf(fp, "/bin/echo 1 > /proc/sys/net/ipv6/conf/${IFNAME}/accept_ra_defrtr\n");
				}

				/* Set kernel IPv6 autoconf for WAN interface with IPv6 mode */
				if (pEntry->AddrMode & IPV6_WAN_AUTO){ /* Stateless DHCP mode = WEB SLAAC mode (M = 0, O = 1) */
					fprintf(fp, "/bin/echo 1 > /proc/sys/net/ipv6/conf/${IFNAME}/autoconf\n");
#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
				}else if (pEntry->AddrMode == IPV6_WAN_AUTO_DETECT_MODE){ /* IPv6 auto detect Initial state */
					fprintf(fp, "/bin/echo 1 > /proc/sys/net/ipv6/conf/${IFNAME}/autoconf\n");
#endif
				}else /* Stateful DHCP mode (M = 1), be careful for IPv6 server which doesn't give IA_NA.
					  * It may cause WAN inf doesn't have globalIP. */
				{
					fprintf(fp, "/bin/echo 0 > /proc/sys/net/ipv6/conf/${IFNAME}/autoconf\n");
				}

				fprintf(fp, "/bin/echo 0 > /proc/sys/net/ipv6/conf/${IFNAME}/disable_ipv6\n");
			}
#endif //CONFIG_IPV6

			fclose(fp);
			if(chmod(script_path, 484) != 0)
				printf("chmod failed: %s %d\n", __func__, __LINE__);
		}else{
			fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, script_path);
			goto Error;
		}
	}
	else {
		fprintf(stderr, "[%s] Error: DUMMY_PPP_INDEX\n.", __FUNCTION__);
		goto Error;
	}

	return 0;
Error:
	return -1;
}


/*
 *	generate pppd ipcp/ipcpv6 up script for WAN interface
 *  return:
 *    -1: Fail
 *     0: Success
 */
int generate_pppd_ipup_script(MIB_CE_ATM_VC_Tp pEntry)
{
	int ret=0, len=0;
	FILE *fp=NULL;
	char wanif[IFNAMSIZ]={0}, ipup_path[128]={0};
#ifdef CONFIG_IPV6
	char ipv6Enable =-1;
	unsigned char duid_mode=(DHCPV6_DUID_TYPE_T)DHCPV6_DUID_LLT;
#endif
	DIR* dir=NULL;
#ifdef NAT_LOOPBACK
	int fh;
#endif

	if(!pEntry){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return -1;
	}

	dir = opendir(PPPD_V4_UP_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_V4_UP_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));

	if (PPP_INDEX(pEntry->ifIndex) != DUMMY_PPP_INDEX) {
		/* IPv4 script */
		snprintf(ipup_path, sizeof(ipup_path), "%s/"PPPD_V4_UP_SCRIPT_NAME, PPPD_V4_UP_SCRIPT_DIR, wanif);
		if (fp = fopen(ipup_path, "w+")) {
			fprintf(fp, "#!/bin/sh\n\n");

			if(pEntry->pppDebug){
				fprintf(fp, "######### for debug###########\n");
				fprintf(fp, "echo \"$0: DEVICE=${DEVICE} IFNAME=${IFNAME} IPLOCAL=${IPLOCAL} SESSION=${SESSION} IPREMOTE=${IPREMOTE} DNS1=${DNS1}\" >> %s\n", PPPD_DEBUG_FILE);
				fprintf(fp, "######### for debug###########\n\n");
			}
#if defined(CONFIG_SUPPORT_AUTO_DIAG)
			if(pEntry->mbs ==1 || pEntry->mbs ==2){
				fprintf(fp, "#update ppp simu parameters\n");
				fprintf(fp, "touch /tmp/ppp7Finish\n");
				fprintf(fp, "sysconf send_unix_sock_message %s do_update_simu_result_file \\\"${IFNAME}\\\" \\\"${SESSION}\\\" \\\"${IPLOCAL}\\\" \\\"${DNS1}\\\" \\\"${IPREMOTE}\\\" \n\n", SYSTEMD_USOCK_SERVER);
			}
#endif
			fprintf(fp, "######### Script Globle Variable ###########\n");
			fprintf(fp, "SESSION=${SESSION}\n");
			fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
			fprintf(fp, "######### Script Globle Variable ###########\n\n");

			// create dns resolv.conf.{ifname} file for dnsmasq
			fprintf(fp, "rm %s/resolv.conf.${IFNAME}\n", (char *)PPPD_CONFIG_FOLDER);
			fprintf(fp, "if [[ ${DNS1} != \"\" ]]; then echo \"${DNS1}@${IPLOCAL}\" >> %s/resolv.conf.${IFNAME}; fi\n", (char *)PPPD_CONFIG_FOLDER);
			fprintf(fp, "if [[ ${DNS2} != \"\" ]] && [[ ${DNS1} != ${DNS2} ]]; then echo \"${DNS2}@${IPLOCAL}\" >> %s/resolv.conf.${IFNAME}; fi\n", (char *)PPPD_CONFIG_FOLDER);

#ifdef NAT_LOOPBACK
			fprintf(fp, "# NAT Loopback hook Begin\n");
			fflush(fp); /* fp stream has buffer to write, if we need to write by underlying file descriptor we should flush buffer first. */
			fh = fileno(fp);
			set_ppp_NATLB(fh, wanif, "$IPLOCAL");
			fprintf(fp, "# NAT Loopback hook End\n\n");
#endif
			fclose(fp);
			if(chmod(ipup_path, 484) != 0)
				printf("chmod failed: %s %d\n", __func__, __LINE__);
		}else{
			fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, ipup_path);
			goto Error;
		}

#ifdef CONFIG_IPV6
		/* IPv6 script */
		mib_get_s(MIB_DHCPV6_CLIENT_DUID_MODE, (void *)&duid_mode, sizeof(duid_mode));
		mib_get_s(MIB_V6_IPV6_ENABLE, (void *)&ipv6Enable, sizeof(ipv6Enable));
		if(ipv6Enable == 1)
		{
			dir = opendir(PPPD_V6_UP_SCRIPT_DIR);
			if (ENOENT == errno) {
				/* Directory does not exist. */
				va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_V6_UP_SCRIPT_DIR);
			}
			if(dir)closedir(dir);

			snprintf(ipup_path, sizeof(ipup_path), "%s/"PPPD_V6_UP_SCRIPT_NAME, PPPD_V6_UP_SCRIPT_DIR, wanif);
			if (fp = fopen(ipup_path, "w+")) {
				unsigned char Ipv6AddrStr[48], RemoteIpv6AddrStr[48];
				unsigned char value[128];

				fprintf(fp, "#!/bin/sh\n\n");

				if(pEntry->pppDebug){
					fprintf(fp, "######### for debug###########\n");
					fprintf(fp, "echo \"$0: DEVICE=${DEVICE} IFNAME=${IFNAME} IPLOCAL=${IPLOCAL} SESSION=${SESSION}\" >> %s\n", PPPD_DEBUG_FILE);
					fprintf(fp, "######### for debug###########\n\n");
				}

#if defined(CONFIG_SUPPORT_AUTO_DIAG)
				if(pEntry->mbs ==1 || pEntry->mbs ==2){
					fprintf(fp, "#update ppp simu parameters\n");
					fprintf(fp, "touch /tmp/ppp7Finish\n");
					fprintf(fp, "sysconf send_unix_sock_message %s do_update_simu_result_file_ipv6 \\\"${IFNAME}\\\" \\\"${SESSION}\\\" \\\"${LLREMOTE}\\\" \n\n", SYSTEMD_USOCK_SERVER);
				}
#endif

				fprintf(fp, "######### Script Globle Variable ###########\n");
				fprintf(fp, "SESSION=${SESSION}\n");
				fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
#ifdef CONFIG_USER_DHCP_ISC
				fprintf(fp, "DHCPCV6_LEASEFILE=%s.${IFNAME}\n", PATH_DHCLIENT6_LEASE);
				fprintf(fp, "DHCPCV6_PIDFILE=%s.${IFNAME}\n", PATH_DHCLIENT6_PID);
				fprintf(fp, "DHCPCV6_CFFILE=%s.${IFNAME}\n", PATH_DHCLIENT6_CONF);
#endif
				fprintf(fp, "######### Script Globle Variable ###########\n\n");

				if (((pEntry->AddrMode & IPV6_WAN_STATIC)) == IPV6_WAN_STATIC) {
					inet_ntop(PF_INET6, (struct in6_addr *)pEntry->Ipv6Addr, Ipv6AddrStr, sizeof(Ipv6AddrStr));
					inet_ntop(PF_INET6, (struct in6_addr *)pEntry->RemoteIpv6Addr, RemoteIpv6AddrStr, sizeof(RemoteIpv6AddrStr));

					// Add WAN static IP
					len = sizeof(Ipv6AddrStr) - strlen(Ipv6AddrStr);
					snprintf(Ipv6AddrStr + strlen(Ipv6AddrStr), len, "/%d", pEntry->Ipv6AddrPrefixLen);
					fprintf(fp, "/bin/ifconfig ${IFNAME} add %s\n", Ipv6AddrStr);

					// Add default gw
					if (isDefaultRouteWan(pEntry)) {
						// route -A inet6 add ::/0 gw 3ffe::0200:00ff:fe00:0100 dev ppp0
						fprintf(fp, "/bin/route -A inet6  add ::/0 gw %s dev ${IFNAME}\n", RemoteIpv6AddrStr);
					}
				}
				else {
					/* need get gateway information from RA, so for dynamic address case always enable accept_ra_defrtr */
					fprintf(fp, "echo 1 > /proc/sys/net/ipv6/conf/${IFNAME}/accept_ra_defrtr\n");
					/* accept_ra set to 2: Overrule forwarding behaviour. Accept Router Advertisements
					 * even if forwarding is enabled. Otherwise set forwarding = 1 will delete default GW
					 * from RA by kernel. */
					fprintf(fp, "echo 2 > /proc/sys/net/ipv6/conf/${IFNAME}/accept_ra\n");
				}
#ifdef DHCPV6_ISC_DHCP_4XX
				// Start DHCPv6 client
				// dhclient -6 ppp0 -d -q -lf /var/dhcpcV6.leases.ppp0 -pf /var/run/dhcpcV6.pid.ppp0 -cf /var/dhcpcV6.conf.ppp0 -N -P
#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
				/* Auto mode PPPoE IPv6 will do at systemd trigger, so we don't do it at ppp ipcpv6 up. */
				if (pEntry->AddrMode == IPV6_WAN_AUTO_DETECT_MODE)
					printf("IPV6_WAN_AUTO_DETECT_MODE doesn't need to start DHCPv6 client in file %s\n", ipup_path);
				else
#endif
				 if (pEntry->Ipv6Dhcp == 1) {
					make_dhcpcv6_conf(pEntry);

					//fix bug :mul-wan, pppoe+dhcpv6 ->reboot->ppp0 has two ipv6 address dhcp from server
		                        //when restart dhclient , it should clean the old dhcpv6 address,please do not user 'flush',casue SLAAC address may have timing issue.
		                        //The prefix len depends on dhcp-4.x.x/client/dhc6.c dhc6_marshall_values fun $(new_ip6_prefixlen)
					snprintf(value, sizeof(value), "ip -6 addr del $(cat ${DHCPCV6_LEASEFILE}  | awk '/iaaddr/' | awk '{print $2}' | awk 'END {print}')/128 dev ${IFNAME}\n");
					fprintf(fp, value);

					//when start dhclient, it should clean leasefile which maybe save previous address info and will lead it can't get new IP this time.
					snprintf(value, sizeof(value), "echo \" \" > ${DHCPCV6_LEASEFILE}\n");
					fprintf(fp, value);
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
					/* Use DUID_LL which generated by rtk to request same IA info.
					 * But PPP may not get same IA info because session ID won't be same and PPP will release every time */
					char duid_buf[128] = {0}; //If 128 is not enough, you can add this size.
					char tmp_cmd[256] = {0};
					rtk_get_duid_ll_for_isc_dhcp_lease_file(pEntry, wanif, duid_buf, sizeof(duid_buf));
					/*
					 * isc-dhcpv6 duid format: default-duid "\000\003\000\001\001\036L\007h\003";
					 *                        default-duid "\000\001\000\001$\345K3\000\032+3\352\023";
					 */
					/* use ' to wrap the string which have ". */
					snprintf(tmp_cmd, sizeof(tmp_cmd), "echo 'default-duid \"%s\";' > ${DHCPCV6_LEASEFILE}\n", duid_buf);
					fprintf(fp, tmp_cmd);
#endif

					snprintf(value, sizeof(value), "/bin/dhclient -6 ${IFNAME} -d -q -lf ${DHCPCV6_LEASEFILE} -pf ${DHCPCV6_PIDFILE} -cf ${DHCPCV6_CFFILE} ");
					fprintf(fp, value);
					// for DHCP441 or later
					fprintf(fp, "--dad-wait-time 5 ");
					if (duid_mode == DHCPV6_DUID_LL)
						fprintf(fp, "-D LL ");
					else
						fprintf(fp, "-D LLT ");

					/* Only request other information(like DNS, DNSSL etc...) not include any IA info.
					 * Note: After receive server REPLY, DHCP6c daemon will exit and write DNS info to lease file.
					 *       Now we use it at TR069, VOIP and TR069_VOICE only .
					 */
					if ((pEntry->dnsv6Mode == 1) && (pEntry->Ipv6DhcpRequest == M_DHCPv6_REQ_NONE)) {
						fprintf(fp, "-S ");
					}
					else {
						// Request Address
						if (pEntry->Ipv6DhcpRequest & M_DHCPv6_REQ_IANA) {
							// for DHCP441 or later
							fprintf(fp, "-N ");
						}

						if ((pEntry->Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)) {
							fprintf(fp, "-P ");
						}
					}
					fprintf(fp, "&\n");
				}
#endif
				fclose(fp);
				if(chmod(ipup_path, 484) != 0)
					printf("chmod failed: %s %d\n", __func__, __LINE__);
			}else{
				fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, ipup_path);
				goto Error;
			}
		}  //End of if(ipv6Enable == 1)
#endif
	}
	else {
		fprintf(stderr, "[%s] Error: DUMMY_PPP_INDEX\n.", __FUNCTION__);
		goto Error;
	}

	return ret;
Error:
	return -1;
}

/*
 *	generate pppd ipcp/ipcpv6 down script for WAN interface
 *  return:
 *    -1: Fail
 *     0: Success
 */
int generate_pppd_ipdown_script(MIB_CE_ATM_VC_Tp pEntry)
{
	int ret=0;
	FILE *fp=NULL;
	char wanif[IFNAMSIZ]={0}, ipdown_path[128]={0};
	unsigned char vChar=0;
#ifdef CONFIG_IPV6
	char ipv6Enable =-1;
	unsigned char duid_mode=(DHCPV6_DUID_TYPE_T)DHCPV6_DUID_LLT;
#endif
	DIR* dir=NULL;

	if(!pEntry){
		fprintf(stderr, "[%s] null parameter\n", __FUNCTION__);
		return -1;
	}

	dir = opendir(PPPD_V4_DOWN_SCRIPT_DIR);
	if (ENOENT == errno) {
		/* Directory does not exist. */
		va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_V4_DOWN_SCRIPT_DIR);
	}
	if(dir)closedir(dir);

	if (PPP_INDEX(pEntry->ifIndex) != DUMMY_PPP_INDEX) {
		/* IPv4 script */
		snprintf(wanif, sizeof(wanif), "ppp%u", PPP_INDEX(pEntry->ifIndex));
		snprintf(ipdown_path, sizeof(ipdown_path), "%s/"PPPD_V4_DOWN_SCRIPT_NAME, PPPD_V4_DOWN_SCRIPT_DIR, wanif);
		if (fp = fopen(ipdown_path, "w+")) {
			fprintf(fp, "#!/bin/sh\n\n");

			if(pEntry->pppDebug){
				fprintf(fp, "######### for debug###########\n");
				fprintf(fp, "echo \"$0: DEVICE=${DEVICE} IFNAME=${IFNAME} IPLOCAL=${IPLOCAL}\" >> %s\n", PPPD_DEBUG_FILE);
				fprintf(fp, "######### for debug###########\n\n");
			}

			fprintf(fp, "######### Script Globle Variable ###########\n");
			fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
			fprintf(fp, "######### Script Globle Variable ###########\n\n");

			// clean dns resolv.conf.{ifname} file
			fprintf(fp, "rm %s/resolv.conf.${IFNAME}\n", (char *)PPPD_CONFIG_FOLDER);
#ifdef NAT_LOOPBACK
			fprintf(fp, "# NAT Loopback hook Begin\n");
			del_ppp_NATLB(fp, wanif);
			fprintf(fp, "# NAT Loopback hook End\n\n");
#endif

			fclose(fp);
			if(chmod(ipdown_path, 484) != 0)
				printf("chmod failed: %s %d\n", __func__, __LINE__);
		}else{
			fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, ipdown_path);
			goto Error;
		}

#ifdef CONFIG_IPV6
		mib_get_s(MIB_V6_IPV6_ENABLE, (void *)&ipv6Enable, sizeof(ipv6Enable));
		if(ipv6Enable == 1)
		{
			dir = opendir(PPPD_V6_DOWN_SCRIPT_DIR);
			if (ENOENT == errno) {
				/* Directory does not exist. */
				va_cmd("/bin/mkdir", 2, 1, "-p", PPPD_V6_DOWN_SCRIPT_DIR);
			}
			if(dir)closedir(dir);

			/* IPv6 script */
			snprintf(ipdown_path, sizeof(ipdown_path), "%s/"PPPD_V6_DOWN_SCRIPT_NAME, PPPD_V6_DOWN_SCRIPT_DIR, wanif);
			if (fp = fopen(ipdown_path, "w+")) {
				fprintf(fp, "#!/bin/sh\n\n");

				if(pEntry->pppDebug){
					fprintf(fp, "######### for debug###########\n");
					fprintf(fp, "echo \"$0: DEVICE=${DEVICE} IFNAME=${IFNAME} IPLOCAL=${IPLOCAL}\" >> %s\n", PPPD_DEBUG_FILE);
					fprintf(fp, "######### for debug###########\n\n");
				}

				fprintf(fp, "######### Script Globle Variable ###########\n");
				fprintf(fp, "IFINDEX=%u\n", pEntry->ifIndex);
#ifdef CONFIG_USER_DHCP_ISC
				fprintf(fp, "DHCPCV6_LEASEFILE=%s.${IFNAME}\n", PATH_DHCLIENT6_LEASE);
				fprintf(fp, "DHCPCV6_PIDFILE=%s.${IFNAME}\n", PATH_DHCLIENT6_PID);
#endif
#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
				fprintf(fp, "RA_INFO_FILE=%s_${IFNAME}\n", (char *)RA_INFO_FILE);
#endif
				fprintf(fp, "######### Script Globle Variable ###########\n\n");

#ifdef CONFIG_USER_DHCP_ISC
#ifdef CONFIG_BHS
				/* For Telefonica IPv6 Test Plan, Test Case ID 20178, if PPPoE session is down,			   *
				* LAN PC global IPv6 address must change the state from "Prefered" to "Obsolete".		   *
				* So here, if PPPoE connection is down, and DHCPv6 is auto mode, kill the DHCPv6 process  */

				mib_get_s(MIB_DHCPV6_MODE, (void *)&vChar, sizeof(vChar));
				if (vChar == 3) {  // 3:auto mode
					fprintf(fp, "kill `cat /var/run/dhcpd6.pid`\n");
				}
#endif

				mib_get_s(MIB_DHCPV6_CLIENT_DUID_MODE, (void *)&duid_mode, sizeof(duid_mode));
				fprintf(fp, "\n");
				fprintf(fp, "# IPv6 should not work anymore, kill dhclient then delete pid and lease file.\n");
				fprintf(fp, "if [ -f ${DHCPCV6_PIDFILE} ]; then\n");
				fprintf(fp, "\tif [ -f ${DHCPCV6_LEASEFILE} ]; then\n");
				fprintf(fp, "\t\t/bin/dhclient -6 -r -1 -lf ${DHCPCV6_LEASEFILE} -pf ${DHCPCV6_PIDFILE} %s -N -P", wanif);
				if (duid_mode == DHCPV6_DUID_LL)
					fprintf(fp, " -D LL\n");
				else
					fprintf(fp, " -D LLT\n");
				fprintf(fp, "\t\trm ${DHCPCV6_LEASEFILE}\n");
				fprintf(fp, "\telse\n");
				fprintf(fp, "\t\t[ -f ${DHCPCV6_PIDFILE} ] && kill `cat ${DHCPCV6_PIDFILE}`\n");
				fprintf(fp, "\tfi\n");
				fprintf(fp, "fi\n");
				fprintf(fp, "\n");

				if (pEntry->AddrMode == IPV6_WAN_AUTO_DETECT_MODE && pEntry->cmode == CHANNEL_MODE_PPPOE){
					fprintf(fp, "/bin/echo 1 > /proc/sys/net/ipv6/conf/${IFNAME}/autoconf\n");
					fprintf(fp, "\n");
				}

				if(pEntry->Ipv6DhcpRequest & (M_DHCPv6_REQ_IAPD)){
					fprintf(fp, "PREFIX_MODE=`mib get PREFIXINFO_PREFIX_MODE`\n");
					fprintf(fp, "PREFIX_WANCONN=`mib get PREFIXINFO_DELEGATED_WANCONN`\n\n");

					fprintf(fp, "# if intf is PD wan, it should clean LAN server\n");
					fprintf(fp, "if [[ ${PREFIX_MODE#*=} -eq %d ]]; then\n", IPV6_PREFIX_DELEGATION);
					fprintf(fp, "\tif [[ ${PREFIX_WANCONN#*=} -eq ${IFINDEX} ]]; then\n");
						if(pEntry->pppDebug){
							fprintf(fp, "\t\techo \"$0: restart lan server\" >> %s\n", PPPD_DEBUG_FILE);
						}
						fprintf(fp, "\t\t#Delete LAN IPv6 IP and Restart IPv6 Lan Server, unused parameter(::/64)\n");
						fprintf(fp, "\t\tsysconf send_unix_sock_message %s do_ipv6_iapd ::/64 %s\n", SYSTEMD_USOCK_SERVER, wanif);
					fprintf(fp, "\tfi\n");
					fprintf(fp, "fi\n");
					fprintf(fp, "\n");
				}
#endif
#if defined(CONFIG_USER_RTK_RAMONITOR) && defined(CONFIG_USER_RTK_IPV6_WAN_AUTO_DETECT)
				fprintf(fp, "rm ${RA_INFO_FILE} > /dev/null 2>&1\n");
#endif
				fclose(fp);
				if(chmod(ipdown_path, 484) != 0)
					printf("chmod failed: %s %d\n", __func__, __LINE__);
			}else{
				fprintf(stderr, "[%s] Error: create %s fail\n.", __FUNCTION__, ipdown_path);
				goto Error;
			}
		}
#endif
	}
	else {
		fprintf(stderr, "[%s] Error: DUMMY_PPP_INDEX\n.", __FUNCTION__);
		goto Error;
	}

	return ret;
Error:
	return -1;
}

int reWriteAllpppdScript_ipup()
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
			printf("reWriteAllpppdScript_ipup: cannot get ATM_VC_TBL(ch=%d) entry\n", i);
			return 0;
		}

		if ((!Entry.enable) || ((Entry.cmode!=CHANNEL_MODE_PPPOE) && (Entry.cmode!=CHANNEL_MODE_PPPOA)))
			continue;
		generate_pppd_ipup_script(&Entry);
	}
	return 0;
}
#endif //CONFIG_USER_PPPD

/*
 *	generate the pppx initial script for WAN interface
 */
static int generate_init_script(MIB_CE_ATM_VC_Tp pEntry){
#ifdef CONFIG_USER_PPPD
	generate_pppd_init_script(pEntry);
#endif //CONFIG_USER_PPPD
	return 0;
}

/*
 *	generate the ipup_ppp(vc)x script for WAN interface
 */
static int generate_ipup_script(MIB_CE_ATM_VC_Tp pEntry){
#ifdef CONFIG_USER_PPPD
	generate_pppd_ipup_script(pEntry);
#endif //CONFIG_USER_PPPD
#ifdef CONFIG_USER_SPPPD
	generate_spppd_ipup_script(pEntry);
#endif //CONFIG_USER_SPPPD
	return 0;
}

/*
 *	generate the ipdown_ppp(vc)x script for WAN interface
 */
static int generate_ipdown_script(MIB_CE_ATM_VC_Tp pEntry){
#ifdef CONFIG_USER_PPPD
	generate_pppd_ipdown_script(pEntry);
#endif //CONFIG_USER_PPPD
#ifdef CONFIG_USER_SPPPD
	generate_spppd_ipdown_script(pEntry);
#endif //CONFIG_USER_SPPPD
	return 0;
}

//Inform kernel the number of routing interface.
// flag = 1 to increase 1
// flag = 0 to decrease 1
static void internetLed_route_if(int flag)
{
	FILE *fp;

	fp = fopen ("/proc/IntnetLED", "w");
	if (fp)
	{
		if (flag)
			fprintf(fp,"+");
		else
			fprintf(fp,"-");
		fclose(fp);
	}
}

int setup_ethernet_config(MIB_CE_ATM_VC_Tp pEntry, char *wanif)
{
	char *argv[8];
	int status=0;
	unsigned char devAddr[MAC_ADDR_LEN];
	char macaddr[MAC_ADDR_LEN*2+1];
	char sValue[8];
#ifdef CONFIG_IPV6
	char ipv6Enable;
#endif
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	unsigned char logoTestMode = 0 ;
#endif

	// ifconfig vc0 down
	va_cmd(IFCONFIG, 2, 1, wanif, "down");
#ifdef WLAN_WISP
	if(MEDIA_INDEX(pEntry->ifIndex) != MEDIA_WLAN)
	{
#endif
#ifdef CONFIG_ATM_CLIP
		if(pEntry->cmode != CHANNEL_MODE_RT1577)
#endif
		{
#ifdef CONFIG_USER_MAC_CLONE
                        if(pEntry->macclone_enable == 0){
                        snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
                                pEntry->MacAddr[0], pEntry->MacAddr[1], pEntry->MacAddr[2], pEntry->MacAddr[3], pEntry->MacAddr[4], pEntry->MacAddr[5]);
                        }
                        else{
                                snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
                                        pEntry->macclone[0], pEntry->macclone[1], pEntry->macclone[2], pEntry->macclone[3], pEntry->macclone[4], pEntry->macclone[5]);
                        }
#else
                        snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
                                        pEntry->MacAddr[0], pEntry->MacAddr[1], pEntry->MacAddr[2], pEntry->MacAddr[3], pEntry->MacAddr[4], pEntry->MacAddr[5]);
#endif

			argv[1]=wanif;
			argv[2]="down";
			argv[3]="hw";
			argv[4]="ether";
			argv[5]=macaddr;
			argv[6]=NULL;
			TRACE(STA_SCRIPT, "%s %s %s %s %s\n", IFCONFIG, argv[1], argv[2], argv[3], argv[4]);
			status|=do_cmd(IFCONFIG, argv, 1);
		}
#ifdef WLAN_WISP
	}
#endif

		// ifconfig vc0 mtu 1500
		if (pEntry->cmode != CHANNEL_MODE_BRIDGE && pEntry->cmode != CHANNEL_MODE_PPPOE
			&& pEntry->cmode != CHANNEL_MODE_PPPOA) {
			sprintf(sValue, "%u", pEntry->mtu);
			argv[1] = wanif;
			argv[2] = "mtu";
			argv[3] = &sValue[0];
			argv[4] = NULL;
			TRACE(STA_SCRIPT, "%s %s %s %s\n", IFCONFIG, argv[1], argv[2], argv[3]);
			status|=do_cmd(IFCONFIG, argv, 1);
		}

		// ifconfig vc0 txqueuelen 10
		argv[1] = wanif;
		argv[2] = "txqueuelen";
                argv[3] = "10";
		argv[4] = NULL;
		TRACE(STA_SCRIPT, "%s %s %s %s\n", IFCONFIG, argv[1], argv[2], argv[3]);
		status|=do_cmd(IFCONFIG, argv, 1);
#ifdef CONFIG_IPV6
		mib_get_s(MIB_V6_IPV6_ENABLE, (void *)&ipv6Enable, sizeof(ipv6Enable));
		// Disable ipv6 in bridge
		if (pEntry->cmode == CHANNEL_MODE_BRIDGE)
			setup_disable_ipv6(wanif, 1);
		else { // enable ipv6 if applicable
			if ((pEntry->IpProtocol & IPVER_IPV6) && ipv6Enable)
				setup_disable_ipv6(wanif, 0);
			else
				setup_disable_ipv6(wanif, 1);
		}
#endif

	 /* This part is for 98D switch port startup issue. These procs cannot be recovered in applywan flow so we use compiler flag
		to avoid logo test failure with dad_transmit 17. 20210528 */
#ifdef CONFIG_RTK_SOC_RTL8198D
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
		unsigned char ce_router_cmd[256]={0};

		if( !mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&logoTestMode, sizeof(logoTestMode)))
			printf("Get V6_LOGO_TEST_MODE mib error!");

		if(logoTestMode ==3 ){ //CE_ROUTER mode
			//[ND]	Section 6.3.7 case 128 Router Solicitations
			snprintf(ce_router_cmd, sizeof(ce_router_cmd), "echo 4 > /proc/sys/net/ipv6/conf/%s/router_solicitation_interval", wanif);
			va_cmd("/bin/sh", 2, 1, "-c", ce_router_cmd);
			snprintf(ce_router_cmd, sizeof(ce_router_cmd), "echo 3 > /proc/sys/net/ipv6/conf/%s/router_solicitations", wanif);
			va_cmd("/bin/sh", 2, 1, "-c", ce_router_cmd);
			//CE ROUTER : wan-rfc3633 Test CERouter 1.2.2 : Basic Message Exchanges
			snprintf(ce_router_cmd,  sizeof(ce_router_cmd), "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", wanif);
			va_cmd("/bin/sh", 2, 1, "-c", ce_router_cmd);
			//WAN_RFC4862 request WAN interface send DAD for Linklocal, bug 98D switch port startup too late(after startup),
			//this proc will be set to 1 after startup
			snprintf(ce_router_cmd, sizeof(ce_router_cmd), "echo 17 > /proc/sys/net/ipv6/conf/%s/dad_transmits", wanif);
			va_cmd("/bin/sh", 2, 1, "-c", ce_router_cmd);

			/* accept_ra set to 2: Overrule forwarding behaviour. Accept Router Advertisements
			 * even if forwarding is enabled. Otherwise set forwarding = 1 will delete default GW
			 * from RA by kernel. */
			snprintf(ce_router_cmd,  sizeof(ce_router_cmd), "/bin/echo 2 > /proc/sys/net/ipv6/conf/%s/accept_ra", wanif);
			va_cmd("/bin/sh", 2, 1, "-c", ce_router_cmd);
		}
#endif
#endif
		// ifconfig vc0 up
		argv[2] = "up";
		argv[3] = NULL;
		TRACE(STA_SCRIPT, "%s %s %s\n", IFCONFIG, argv[1], argv[2]);
		status|=do_cmd(IFCONFIG, argv, 1);

		return status;
}

// return value:
// 0  : successful
// -1 : failed
int startConnection(MIB_CE_ATM_VC_Tp pEntry, int mib_vc_idx)
{
	struct data_to_pass_st msg;
	char *aal5Encap, qosParms[64], wanif[IFNAMSIZ];
	char wandev_br[IFNAMSIZ + 1];
	int brpppoe;
	int pcreg, screg;
	int status=0;
	MEDIA_TYPE_T mType;
	char vChar=-1;
	#if defined(CONFIG_RTL_MULTI_ETH_WAN) && !defined(CONFIG_RTL_SMUX_DEV)
	char cmd_buff[64] = {'\0'};
	#endif

	if(pEntry == NULL || pEntry->enable == 0)
	{
		return 0;
	}

	mType = MEDIA_INDEX(pEntry->ifIndex);
#ifdef CONFIG_DEV_xDSL
	if (mType == MEDIA_ATM)
		setup_ATM_channel(pEntry);
#else
	if (mType == MEDIA_ATM)
		return 0;
#endif
	if (pEntry->cmode != CHANNEL_MODE_BRIDGE)
		internetLed_route_if(1);//+1

#ifdef NEW_PORTMAPPING
		get_pmap_fgroup(pmap_list, MAX_VC_NUM);
		setup_wan_pmap_lanmember(mType, pEntry->ifIndex);
#endif

	//snprintf(wanif, 5, "vc%d", VC_INDEX(pEntry->ifIndex));
	ifGetName(PHY_INTF(pEntry->ifIndex),wanif,sizeof(wanif));
	//printf("%s true_%s, major dev=%s\n", __func__, MEDIA_INDEX(pEntry->ifIndex)?"ETH":"ATM", wanif);

	if (pEntry->cmode != CHANNEL_MODE_PPPOA)
	{
		setup_ethernet_config(pEntry, wanif);
	}

	generate_ifup_script(pEntry);
	generate_ifdown_script(pEntry);

// Mason Yu. enable_802_1p_090722
#ifdef CONFIG_DEV_xDSL
#ifdef ENABLE_802_1Q
	// This vlan tag function is mutual exclusive with multi-wan pvc.
#ifndef CONFIG_RTL_MULTI_PVC_WAN
	// mpoactl set vc0 vlan 1 pvid 1
	snprintf(msg.data, BUF_SIZE,
		"mpoactl set %s vlan %d vid %d vprio %d", wanif,
		pEntry->vlan, pEntry->vid, pEntry->vprio);
	if (pEntry->cmode == CHANNEL_MODE_BRIDGE || pEntry->cmode == CHANNEL_MODE_IPOE ||
		pEntry->cmode == CHANNEL_MODE_PPPOE || pEntry->cmode == CHANNEL_MODE_6RD || pEntry->cmode == CHANNEL_MODE_6IN4) {
		if (mType == MEDIA_ATM) {
			// mpoactl set vc0 vlan 1 pvid 1
			TRACE(STA_SCRIPT, "%s\n", msg.data);
			status |= write_to_mpoad(&msg);
		}
	}
#endif
#endif
#endif
	printf("[%s@%d] pEntry->ifIndex = %d, pEntry->cmode = %d \n", __FUNCTION__, __LINE__, pEntry->ifIndex, pEntry->cmode);
	#if defined(CONFIG_RTL_MULTI_ETH_WAN) && !defined(CONFIG_RTL_SMUX_DEV) && defined(SUPPORT_PORT_BINDING)
	snprintf(cmd_buff, sizeof(cmd_buff), "/bin/ethctl port_mapping %s %x 1", wanif, pEntry->itfGroup);
	system(cmd_buff);
	//printf("[%s@%d] pEntry->ifIndex = %d, pEntry->cmode = %d pEntry->itfGroup=0x%x wanif=%s cmd_buff=%s \n", __FUNCTION__, __LINE__, pEntry->ifIndex, pEntry->cmode, pEntry->itfGroup, wanif, cmd_buff);
	#endif

	if (pEntry->cmode == CHANNEL_MODE_BRIDGE)
	{
		// rfc1483-bridged
		// brctl addif br0 vc0
		printf("1483 bridged\n");
		// Mason Yu
		//cxy 2016-12-22: CHANNEL_MODE_BRIDGE always add wanif to br0
		//if ( pEntry->brmode != BRIDGE_DISABLE )
#if defined(CONFIG_PPPOE_MONITOR)
		add_pppoe_monitor_wan_interface(wanif);
#endif
#if defined(_SUPPORT_INTFGRPING_PROFILE_)
		char brname[32] = {0};
		if(pEntry->itfGroupNum != 0 &&
			get_grouping_ifname_by_groupnum(pEntry->itfGroupNum, brname, sizeof(brname)) &&
			getInFlags(brname, NULL))
		{
			status|=va_cmd(BRCTL, 3, 1, "addif", brname, wanif);
		}
		else
#endif
		status|=va_cmd(BRCTL, 3, 1, "addif", BRIF, wanif);

		set_ipv4_default_policy_route(pEntry, 1);
#ifdef CONFIG_IPV6
		set_ipv6_default_policy_route(pEntry, 1);
#endif
#ifdef CONFIG_BRIDGE_IGMP_SNOOPING
		/* only if enable CONFIG_BRIDGE_IGMP_SNOOPING, it should config snooping proc setting */
		rtk_multicast_snooping();
#endif
	}
	else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_IPOE)
	{
		// MAC Encapsulated Routing
		printf("1483 MER\n");

		// ROUTE_BRIDGE_MIXED use smux_dev nas0_x_1 for bridge not nas0_x.
		if (ROUTE_CHECK_BRIDGE_MIX_ENABLE(pEntry))	// enable bridge
		{
			snprintf(wandev_br, sizeof(wandev_br), "%s%s", wanif, ROUTE_BRIDGE_WAN_SUFFIX);
			va_cmd(BRCTL, 3, 1, "addif", BRIF, wandev_br);
		}
	}
#ifdef CONFIG_PPP
	else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
	{
		// PPPoE
		printf("PPPoE\n");
#ifdef CONFIG_00R0
		if(!strcmp(pEntry->pppUsername,"") && !strcmp(pEntry->pppPassword,""))
		{
			printf("PPPoE UserName and PassWord is empty!\n");
			return -1;
		}
#endif
		// ROUTE_BRIDGE_MIXED use smux_dev nas0_x_1 for bridge not nas0_x.
		if (ROUTE_CHECK_BRIDGE_MIX_ENABLE(pEntry))	// enable bridge
		{
			snprintf(wandev_br, sizeof(wandev_br), "%s%s", wanif, ROUTE_BRIDGE_WAN_SUFFIX);
			va_cmd(BRCTL, 3, 1, "addif", BRIF, wandev_br);
#if defined(CONFIG_PPPOE_MONITOR)
			add_pppoe_monitor_wan_interface(wanif);
#endif
		}

		generate_init_script(pEntry);
		generate_ipup_script(pEntry);
		generate_ipdown_script(pEntry);
	}
#endif

	setup_NAT(pEntry, 1);

// UPnP Daemon Start
#if defined(CONFIG_USER_MINIUPNPD)
	restart_upnp(CONFIGONE, 1, pEntry->ifIndex, 0);
#endif
// The end of UPnP Daemon Start

#ifdef CONFIG_USER_WT_146
	wt146_create_wan(pEntry, 0);
#endif //CONFIG_USER_WT_146

#if 0//defined (CONFIG_EPON_FEATURE) && defined(CONFIG_RTK_L34_ENABLE)
//cxy 2016-6-6: oam need to add iptv wan vid as default multicast vlan
	if(pEntry->applicationtype == X_CT_SRV_OTHER)
	{
		int pon_mode=0;
		int i;
		mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
		if(pon_mode == EPON_MODE)
		{
			char cmdstr[128];
			for(i=0; i<SW_LAN_PORT_NUM; i++)
			{
				if((pEntry->itfGroup)&(1<<i))
				{
					snprintf(cmdstr, sizeof(cmdstr), "oamcli set igmp mcvlan %d %d 1", i, pEntry->vid);
					system(cmdstr);
				}
			}
		}
	}
#endif
#ifdef CONFIG_RTK_MAC_AUTOBINDING
	do{
		unsigned char flag=0;
		if(mib_get(MIB_AUTO_MAC_BINDING_IPTV_BRIDGE_WAN,&flag)&&flag){
			if (pEntry->cmode == CHANNEL_MODE_BRIDGE&&pEntry->applicationtype==X_CT_SRV_OTHER){
#ifdef CONFIG_SUPPORT_IPTV_APPLICATIONTYPE
					//if(pEntry->othertype==OTHER_IPTV_TYPE)
					{
						int ifIndex = if_nametoindex(wanif);
						char cmd_str[128]={0};
						if(ifIndex){
							sprintf(cmd_str,"echo %d > /proc/mac_autobinding/iptv_bridge_wan",ifIndex);
							va_cmd("/bin/sh",2,1,"-c",cmd_str);
						}
					}

#endif
			}
		}
	}while(0);
#endif

	return status;
}

#if defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN)
void addEthWANdev(MIB_CE_ATM_VC_Tp pEntry)
{
	MEDIA_TYPE_T mType;
	char ifname[IFNAMSIZ];
	int flag=0, tag=0;
	unsigned int nas0_mtu;
	char mtuStr[10];
	int vpass = pEntry->vpass;
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	unsigned int pon_mode = 0;
	unsigned char tr247_unmatched_veip_cfg = 0;

	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
	mib_get_s(MIB_TR247_UNMATCHED_VEIP_CFG, (void *)&tr247_unmatched_veip_cfg, sizeof(tr247_unmatched_veip_cfg));
	if(tr247_unmatched_veip_cfg) vpass = 1;
#endif

	if(vpass && (pEntry->cmode != CHANNEL_MODE_BRIDGE || pEntry->vlan)) vpass = 0;

	nas0_mtu=rtk_wan_get_max_wan_mtu();
	snprintf(mtuStr, sizeof(mtuStr), "%u", nas0_mtu);

	ifGetName(PHY_INTF(pEntry->ifIndex), ifname, sizeof(ifname));
	mType = MEDIA_INDEX(pEntry->ifIndex);

	//add new ethwan dev here.
	if( ((mType == MEDIA_ETH) && (WAN_MODE & MODE_Ethernet))
	     #ifdef CONFIG_PTMWAN
	     ||((mType == MEDIA_PTM) && (WAN_MODE & MODE_PTM))
	     #endif /*CONFIG_PTMWAN*/
	){
		unsigned char *rootdev=NULL;

		#ifdef CONFIG_PTMWAN
		if(mType == MEDIA_PTM)
			rootdev=ALIASNAME_PTM0;
		else
		#endif /*CONFIG_PTMWAN*/
			rootdev=ALIASNAME_NAS0;

#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
		rootdev=(unsigned char *)ELANRIF[pEntry->logic_port];
		va_cmd(IFCONFIG, 2, 1, rootdev, "up");
#endif
		va_cmd(IFCONFIG, 3, 1, rootdev, "mtu", mtuStr);

#ifdef CONFIG_RTL_MULTI_WAN
#if defined(CONFIG_RTL_SMUX_DEV)
		char cmd_str[256]={0}, *pcmd_str = NULL;
		unsigned int vlantag;

		sprintf(cmd_str, "/bin/smuxctl --if-create-name %s %s --set-if-rsmux", rootdev, ifname);
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
		if(pon_mode == GPON_MODE && !(pEntry->applicationtype & X_CT_SRV_PON_PPTP))
		{//for GPON WAN, carrier up by tr142
			sprintf(cmd_str+strlen(cmd_str), " --carrier MANUAL");
		}
#endif
		va_cmd("/bin/sh", 2, 1, "-c", cmd_str);

		//RX
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
		if(pon_mode!=GPON_MODE)
#endif
		{
			int rule_priority;
			for (tag = 0; tag <= SMUX_MAX_TAGS; tag++) {
				if((tag == 0 && pEntry->vlan) || (tag && !pEntry->vlan && !vpass))
					continue;
				pcmd_str = cmd_str;
				pcmd_str += sprintf(pcmd_str, "%s --if %s --rx", SMUXCTL, rootdev);
				pcmd_str += sprintf(pcmd_str, " --tags %d", tag);
				if (pEntry->vlan)
					pcmd_str += sprintf(pcmd_str, " --filter-vid 0x%x 1", pEntry->vid);

				if(!(((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
					|| ROUTE_CHECK_BRIDGE_MIX_ENABLE(pEntry)
				))
				{// filter dest addr on unicast
					pcmd_str += sprintf(pcmd_str, " --filter-unicast-mac 1");
					rule_priority = SMUX_RULE_PRIO_VEIP;
				}
				else rule_priority = SMUX_RULE_PRIO_BCMC;

				if(vpass) rule_priority = rule_priority-1;
				pcmd_str += sprintf(pcmd_str, " --rule-priority %d", rule_priority);

				pcmd_str += sprintf(pcmd_str, " --set-rxif %s --rule-alias %s-rx-default-root --target ACCEPT --rule-append", ifname, ifname);
				printf("==> [%s] %s\n", __FUNCTION__, cmd_str);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
			}
		}

		if (pEntry->vlan)
		{
			for (tag = 1; tag <= SMUX_MAX_TAGS; tag++) {
				sprintf(cmd_str, "%s --if %s --rx --tags %d --filter-vid 0x%x %d --pop-tag --rule-priority %d --rule-alias %s-rx-default-leaf --rule-append",
						SMUXCTL, ifname, tag, pEntry->vid, tag, SMUX_RULE_PRIO_VEIP, ifname);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
			}
		}

		//TX
		if (pEntry->vlan
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
			|| pon_mode == EPON_MODE
#endif
			)
		{
			vlantag = pEntry->vid;
			for (tag = 0; tag <= SMUX_MAX_TAGS; tag++) {
				if((tag && !pEntry->vlan && !vpass))
					continue;
				cmd_str[0] = '\0';
				pcmd_str = cmd_str;
				pcmd_str += sprintf(pcmd_str, "%s --if %s --tx", SMUXCTL, ifname);
				pcmd_str += sprintf(pcmd_str, " --tags %d",tag);
				if(pEntry->vlan)
				{
					if(tag == 0)
						pcmd_str += sprintf(pcmd_str, " --push-tag");
					if(pEntry->vprio)
						pcmd_str += sprintf(pcmd_str, " --set-priority %d %d", (pEntry->vprio-1), (tag)?tag:1);
					pcmd_str += sprintf(pcmd_str, " --set-vid 0x%x %d", vlantag, (tag)?tag:1);
				}
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
#ifdef CONFIG_RTK_SKB_MARK2
				if(pon_mode == EPON_MODE){
					//Need enable streamId enable bit on EPON, for GPON set stream ID by TR142
					pcmd_str += sprintf(pcmd_str, " --set-skb-mark2 0x%llx 0x%llx", SOCK_MARK2_STREAMID_ENABLE_MASK, SOCK_MARK2_STREAMID_ENABLE_MASK);
				}
#else
				#error "please enable CONFIG_RTK_SKB_MARK2"
#endif
#endif
				pcmd_str += sprintf(pcmd_str, " --target ACCEPT --rule-alias %s-tx-default --rule-append", ifname);
				printf("==> [%s] %s\n", __FUNCTION__, cmd_str);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
			}
		}
		if (ROUTE_CHECK_BRIDGE_MIX_ENABLE(pEntry))
		{
			int ipoe_pass_rule_pri = SMUX_RULE_PRIO_PPTP_LAN_UNI_DEF;
			char wandev_br[IFNAMSIZ+1], wan_br_alias[64];

			snprintf(wandev_br, sizeof(wandev_br), "%s%s", ifname, ROUTE_BRIDGE_WAN_SUFFIX);
			snprintf(cmd_str, sizeof(cmd_str), "/bin/smuxctl --if-create-name %s %s", ifname, wandev_br);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);

			snprintf(wan_br_alias, sizeof(wan_br_alias), "bridge_passth_%s", ifname);
			for (tag = 0; tag <= SMUX_MAX_TAGS; tag++) {
				snprintf(cmd_str, sizeof(cmd_str), "%s --if %s --rx --tags %d --set-rxif %s --target ACCEPT --rule-priority %d --rule-alias %s --rule-append",
									SMUXCTL, ifname, tag, wandev_br, ipoe_pass_rule_pri, wan_br_alias);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);

				snprintf(cmd_str, sizeof(cmd_str), "%s --if %s --rx --tags %d --filter-unicast-mac 1 --set-rxif %s --target ACCEPT --rule-priority %d --rule-alias %s --rule-append",
									SMUXCTL, ifname, tag, ifname, ipoe_pass_rule_pri + 1, wan_br_alias);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);

				snprintf(cmd_str, sizeof(cmd_str), "%s --if %s --rx --tags %d --filter-dmac 010000000000 010000000000 --duplicate-forward --set-rxif %s --target ACCEPT --rule-priority %d --rule-alias %s  --rule-append",
									SMUXCTL, ifname, tag, wandev_br, ipoe_pass_rule_pri + 2, wan_br_alias);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
			}

			va_cmd(IFCONFIG, 2, 1, wandev_br, "up");
		}
#else
		int tmp_group;
		const char smux_brg_cmd[]="/bin/ethctl addsmux bridge %s %s";
		const char smux_ipoe_cmd[]="/bin/ethctl addsmux ipoe %s %s";
		const char smux_pppoe_cmd[]="/bin/ethctl addsmux pppoe %s %s";

		char cmd_str[128]={0};

		// Mason Yu
		//va_cmd(IFCONFIG, 2, 1, rootdev, "up");

		if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
			snprintf(cmd_str, 100, smux_brg_cmd, rootdev, ifname);
		else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_IPOE || (CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_6RD
			|| (CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_6IN4 || (CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_6TO4 )
			snprintf(cmd_str, 100, smux_ipoe_cmd, rootdev, ifname);
		else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
			snprintf(cmd_str, 100, smux_pppoe_cmd, rootdev, ifname);

		if (pEntry->napt)
			strncat(cmd_str, " napt", 100);

		if (ROUTE_CHECK_BRIDGE_MIX_ENABLE(pEntry))
			strncat(cmd_str, " brpppoe", 100);

		if (pEntry->vlan) {
			unsigned int vlantag;
			vlantag = (pEntry->vid|((pEntry->vprio) << 13));
			snprintf(cmd_str, 100, "%s vlan %d", cmd_str, vlantag );
		}
		printf("TRACE: %s\n", cmd_str);
		system(cmd_str);
		#if !defined(CONFIG_RTL9601B_SERIES) && !defined(CONFIG_SFU)
		while(getInFlags(ifname, &flag)==0) { //wait the device created successly, Iulian Wu
			printf("WAN %s is not ready!\n", ifname);
			sleep(2);
		}
		#endif
#endif
#endif // of CONFIG_RTL_MULTI_WAN
		if(getInFlags(ifname, &flag)!=0){
			if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE){
				snprintf(mtuStr, sizeof(mtuStr), "%u", pEntry->mtu+8);
			}
			else{
				snprintf(mtuStr, sizeof(mtuStr), "%u", pEntry->mtu);
			}
			//wan bridge into br0 will cause br0 MTU changed(bridge MTU only can be the smallest of the members)
			if (pEntry->cmode != CHANNEL_MODE_BRIDGE)
			{
				va_cmd(IFCONFIG, 3, 1, ifname, "mtu", mtuStr);
			}
		}
	}
}
#endif

int underlying_pppoe_exist(MIB_CE_ATM_VC_Tp pEntry)
{
	int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char wandev[IFNAMSIZ];

	entryNum =  mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if (Entry.cmode != CHANNEL_MODE_PPPOE)
			continue;
		if (VC_INDEX(Entry.ifIndex) != VC_INDEX(pEntry->ifIndex))
			continue;
		if (MEDIA_INDEX(pEntry->ifIndex) != MEDIA_INDEX(Entry.ifIndex))
			continue;
		if (PPP_INDEX(pEntry->ifIndex) == PPP_INDEX(Entry.ifIndex))
			continue;
		// use the same underlying VC/nas/ptm
		ifGetName( PHY_INTF(pEntry->ifIndex), wandev, sizeof(wandev));
		printf("%s:%d:use the same underlying %s interface.\n", __FUNCTION__, __LINE__, wandev);
		return 1;
	}
	return 0;

}
#if defined(CONFIG_USER_RTK_MULTI_BRIDGE_WAN)
void set_vlan_passthrough(unsigned int enable);
void setBrWangroup(char *list, unsigned char grpnum)
{
	int ifdomain, ifid;
	int i, num;
	char *arg0, *token;
	MIB_CE_BR_WAN_T Entry;
	arg0 = list;
	while ((token = strtok(arg0, ",")) != NULL) {
		ifid = atoi(token);
		ifdomain = IF_DOMAIN(ifid);
		ifid = IF_INDEX(ifid);
#if defined(ITF_GROUP_4P) || defined(CONFIG_RTL_8676HWNAT) || defined(CONFIG_APOLLO_ROMEDRIVER) || defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		if (ifdomain == DOMAIN_ELAN) {
			MIB_CE_SW_PORT_T Entry;
			num = mib_chain_total(MIB_SW_PORT_TBL);
			if (ifid < num) {
				if (!mib_chain_get(MIB_SW_PORT_TBL, ifid, &Entry))
					return;
				Entry.itfGroup = grpnum;
				mib_chain_update(MIB_SW_PORT_TBL, &Entry, ifid);
			} else if (ifid >= num && ifid < num * 2) {
				if (!mib_chain_get(MIB_SW_PORT_TBL, ifid - num, &Entry))
					return;
				Entry.vlan_on_lan_itfGroup = grpnum;
				mib_chain_update(MIB_SW_PORT_TBL, &Entry, ifid - num);
			}
		}
#endif
#ifdef WLAN_SUPPORT
		else if (ifdomain == DOMAIN_WLAN) {
			int ori_wlan_idx = wlan_idx;
			wlan_idx = 0;
			if (ifid == 0) {
				mib_set(MIB_WLAN_ITF_GROUP, &grpnum);
				goto next_token;
			}
#if defined(CONFIG_RTL_92D_DMDP) || defined(WLAN_DUALBAND_CONCURRENT)
			if (ifid == 10) {
				mib_set(MIB_WLAN1_ITF_GROUP, &grpnum);
				goto next_token;
			}
#endif
			// Added by Mason Yu
#ifdef WLAN_MBSSID
			// wlan0_vap0
			if (ifid >= 1 && ifid < IFGROUP_NUM) {
				mib_set(MIB_WLAN_VAP0_ITF_GROUP + ((ifid - 1) << 1), &grpnum);
				goto next_token;
			}
#if defined(CONFIG_RTL_92D_DMDP) || defined(WLAN_DUALBAND_CONCURRENT)
			// wlan1_vap0
			if (ifid >= 11 && ifid < IFGROUP_NUM + 10) {
				mib_set(MIB_WLAN1_VAP0_ITF_GROUP + ((ifid - 11) << 1), &grpnum);
				goto next_token;
			}
#endif
#endif // WLAN_MBSSID
			wlan_idx = ori_wlan_idx;
		}
#endif // WLAN_SUPPORT
		else if (ifdomain == DOMAIN_WAN) {
			num = mib_chain_total(MIB_BR_WAN_TBL);
			for (i = 0; i < num; i++) {
				if (!mib_chain_get(MIB_BR_WAN_TBL, i, (void *)&Entry))
					return;
				if (Entry.enable && isValidMedia(Entry.ifIndex)
					&& Entry.ifIndex == ifid) {
					Entry.itfGroupNum = grpnum;
					mib_chain_update(MIB_BR_WAN_TBL,(void *)&Entry, i);
				}
			}
		}
next_token:
		arg0 = NULL;
	}
}
int generateBrWanName(MIB_CE_BR_WAN_T *entry, char* wanname)
{
#ifdef CONFIG_E8B
	char vpistr[6];
	char vcistr[6];
	char vid[16];
	int i, mibtotal;
	MIB_CE_BR_WAN_T tmpEntry;
	MEDIA_TYPE_T mType;

	if(entry==NULL || wanname ==NULL){
		printf("[%s %d] : arg is null \n",__func__, __LINE__);
		return -1;
	}

	mibtotal = mib_chain_total(MIB_BR_WAN_TBL);
	for(i=0; i< mibtotal; i++)
	{
		mib_chain_get(MIB_BR_WAN_TBL, i, &tmpEntry);
		if(tmpEntry.ifIndex == entry->ifIndex
#ifdef BR_ROUTE_ONEPVC
		&&tmpEntry.cmode == entry->cmode
#endif
		)
		break;
	}
	if(i==mibtotal)
		return -1;
	i++;
	mType = MEDIA_INDEX(entry->ifIndex);
	sprintf(wanname, "%d_", i);

	memset(vpistr, 0, sizeof(vpistr));
	memset(vcistr, 0, sizeof(vcistr));

	if (entry->applicationtype == X_BR_SRV_OTHER)
	{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
			strcat(wanname, "OTHER_");
#else
			strcat(wanname, "Other_");
#endif
	}
	if(entry->cmode == CHANNEL_MODE_BRIDGE)
		strcat(wanname, "B_");
	if (mType == MEDIA_ETH) {
		strcat(wanname, "VID_");
		if(entry->vlan==1){
			sprintf(vid, "%d", entry->vid);
			strcat(wanname, vid);
		}
	}
	return 0;
#endif
}
int getBrWanName(MIB_CE_BR_WAN_T * pEntry, char *name)
{
	if (pEntry == NULL || name == NULL)
		return 0;
#ifdef _CWMP_MIB_
	if (*(pEntry->WanName))
		strcpy(name, pEntry->WanName);
	else
#endif
	{			//if not set by ACS. then generate automaticly.
		generateBrWanName(pEntry, name);
	}
	return 1;
}

static int setup_br_ethernet_config(MIB_CE_BR_WAN_Tp pEntry, char *wanif)
{
	char *argv[8];
	int status=0;
	unsigned char devAddr[MAC_ADDR_LEN];
	char macaddr[MAC_ADDR_LEN*2+1];
	char sValue[8];
#ifdef CONFIG_IPV6
	char ipv6Enable;
#endif
	char mtuStr[10];
	unsigned int mtu;

	va_cmd(IFCONFIG, 2, 1, wanif, "down");
#ifdef WLAN_WISP
	if(MEDIA_INDEX(pEntry->ifIndex) != MEDIA_WLAN)
#endif
	{
		snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
				pEntry->MacAddr[0], pEntry->MacAddr[1], pEntry->MacAddr[2], pEntry->MacAddr[3], pEntry->MacAddr[4], pEntry->MacAddr[5]);
		argv[1]=wanif;
		argv[2]="down";
		argv[3]="hw";
		argv[4]="ether";
		argv[5]=macaddr;
		argv[6]=NULL;
		TRACE(STA_SCRIPT, "%s %s %s %s %s\n", IFCONFIG, argv[1], argv[2], argv[3], argv[4]);
		status|=do_cmd(IFCONFIG, argv, 1);
	}

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	char sysbuf[128];
	int wanPhyPort;

	wanPhyPort = rtk_port_get_wan_phyID();

	if(wanPhyPort!=-1){
		rtk_port_set_dev_port_mapping(wanPhyPort, ALIASNAME_NAS0);
	}
	printf( "system(): %s\n", sysbuf );
	system(sysbuf);
	if (wanPhyPort!=-1) {
		sprintf(sysbuf, "echo 0x%x > /proc/fc/ctrl/wan_port_mask", (1<<wanPhyPort));		// change fc wan port mask
		printf( "system(): %s\n", sysbuf );
		system(sysbuf);		// change fc wan port mask
	}
	sleep(1);
#endif

	mtu = rtk_wan_get_max_wan_mtu();
	snprintf(mtuStr, sizeof(mtuStr), "%u", mtu);
	va_cmd(IFCONFIG, 3, 1, wanif, "mtu", mtuStr);

	// ifconfig vc0 txqueuelen 10
	argv[1] = wanif;
	argv[2] = "txqueuelen";
    argv[3] = "10";
	argv[4] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s %s\n", IFCONFIG, argv[1], argv[2], argv[3]);
	status|=do_cmd(IFCONFIG, argv, 1);
#ifdef CONFIG_IPV6
	mib_get_s(MIB_V6_IPV6_ENABLE, (void *)&ipv6Enable, sizeof(ipv6Enable));
	// Disable ipv6 in bridge
	if (pEntry->cmode == CHANNEL_MODE_BRIDGE)
		setup_disable_ipv6(wanif, 1);
#endif
	argv[2] = "up";
	argv[3] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s\n", IFCONFIG, argv[1], argv[2]);
	status|=do_cmd(IFCONFIG, argv, 1);

		return status;
}

int startBrConnection(MIB_CE_BR_WAN_Tp pEntry, int mib_vc_idx)
{
	struct data_to_pass_st msg;
	char *aal5Encap, qosParms[64], wanif[IFNAMSIZ];
	int brpppoe;
	int pcreg, screg;
	int status=0;
	MEDIA_TYPE_T mType;
	char vChar=-1;
	#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_RTL_MULTI_ETH_WAN)
	char cmd_buff[64] = {'\0'};
	#endif

	if(pEntry == NULL || pEntry->enable == 0)
	{
		return 0;
	}

	mType = MEDIA_INDEX(pEntry->ifIndex);

	ifGetName(PHY_INTF(pEntry->ifIndex),wanif,sizeof(wanif));

	if (pEntry->cmode == CHANNEL_MODE_BRIDGE)
	{
		setup_br_ethernet_config(pEntry, wanif);
	}

	if (pEntry->cmode == CHANNEL_MODE_BRIDGE)
	{
#if defined(_SUPPORT_INTFGRPING_PROFILE_)
		char brname[32] = {0};
		if(pEntry->itfGroupNum != 0 &&
			get_grouping_ifname_by_groupnum(pEntry->itfGroupNum, brname, sizeof(brname)) &&
			getInFlags(brname, NULL))
		{
			status|=va_cmd(BRCTL, 3, 1, "addif", brname, wanif);
		}
		else
#endif
		status|=va_cmd(BRCTL, 3, 1, "addif", BRIF, wanif);

	}

	return status;
}

void stopBrWanConnection(MIB_CE_BR_WAN_Tp pEntry)
{
	struct data_to_pass_st msg;
	char wandev[IFNAMSIZ];
	char wanif[IFNAMSIZ];
	char myip[16];
	int i;
	char cmd_buff[64] = {'\0'};
	ifGetName( PHY_INTF(pEntry->ifIndex), wanif, sizeof(wanif));
	ifGetName( PHY_INTF(pEntry->ifIndex), wandev, sizeof(wandev));
	va_cmd(BRCTL, 3, 1, "delif", BRIF, wandev);
	/* Kevin, stop root Qdisc before ifdown interface, for 0412 hw closing IPQoS */
	va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", wandev, "root");
	if (pEntry->cmode != CHANNEL_MODE_PPPOA) {
		va_cmd(IFCONFIG, 2, 1, wanif, "0.0.0.0");
		va_cmd(IFCONFIG, 2, 1, wanif, "down");
		// wait for sar to process the queueing packets
		for (i=0; i<10000000; i++);
#if defined(CONFIG_RTL_MULTI_ETH_WAN)
		//del ethwan dev here.
		if( MEDIA_INDEX(pEntry->ifIndex) == MEDIA_ETH){
			unsigned char *rootdev;
			rootdev=ALIASNAME_NAS0;
			/* patch for unregister_device fail: unregister_netdevice: waiting for nas0_3 to become free. Usage count = 1*/
#ifdef CONFIG_IPV6
			if (pEntry->IpProtocol & IPVER_IPV4)
#endif
			if ((DHCP_T)pEntry->ipDhcp != DHCP_DISABLED)
			{
				int dhcpc_pid = 0;
				char filename[100] = {0};
				sprintf(filename, "%s.%s", (char*)DHCPC_PID, wandev);
				dhcpc_pid = read_pid(filename);
				if (dhcpc_pid > 0)
					kill(dhcpc_pid, SIGTERM);
			}
#if defined(CONFIG_RTL_MULTI_ETH_WAN)
			printf("%s remove smux device %s\n", __func__, wandev);
#if defined(CONFIG_RTL_SMUX_DEV)
			char cmd_str[256]={0};
			sprintf(cmd_str, "/bin/smuxctl --if-delete %s", wandev);
			printf("==> [%s] %s\n", __FUNCTION__, cmd_str);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#endif
#endif
		}
#endif
	}
}
int deleteBrWanConnection(int configAll, MIB_CE_BR_WAN_Tp pEntry)
{
	char action_type_str[128] = {0};
	unsigned int entryNum, i, idx;
	MIB_CE_BR_WAN_T Entry;
	//close all tunnel before delete wan interface.
	entryNum = mib_chain_total(MIB_BR_WAN_TBL);
	for (i = 0; i < entryNum; i++) {
		/* Retrieve entry */
		if (!mib_chain_get(MIB_BR_WAN_TBL, i, (void *)&Entry)) {
			printf("deleteConnection: cannot get BR_WAN_TBL(ch=%d) entry\n", i);
			return 0;
		}
		if (configAll == CONFIGALL)
		{
			if(Entry.enable) {
				/* remove connection on driver*/
				stopBrWanConnection(&Entry);
			}
		}
	}
	if (configAll == CONFIGONE) {
		if (pEntry) {
			if (pEntry->enable) {
				/* remove connection on driver*/
				stopBrWanConnection(pEntry);
			}
		}
	}
	return 0;
}

int is_tagged_br_wan_exist(void)
{
	int is_exist = 0 ,i = 0 ,vcTotal = 0;
	MIB_CE_BR_WAN_T Entry;

	vcTotal = mib_chain_total(MIB_BR_WAN_TBL);
	for (i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_BR_WAN_TBL, i, (void *)&Entry))
				return -1;
		if (Entry.enable == 1 && Entry.vlan != 0)
		{
			is_exist = 1;
			break;
		}
	}

	return is_exist;
}

void restartBrWAN(int configAll, MIB_CE_BR_WAN_Tp pEntry)
{
	int dhcrelay_pid, i;
	if (configAll == CONFIGALL)
		startBrWan(CONFIGALL, NULL);
	else if (configAll == CONFIGONE)
		startBrWan(CONFIGONE, pEntry);
	//else if (configAll == CONFIGCWMP)
	//	startBrWan(CONFIGCWMP, NULL);
	//execute firewall rules
	setBridgeFirewall();
	setBrWanFirewall();
	//set port/vlan mapping rules
#ifdef NEW_PORTMAPPING
	setupBrWanMap();
#endif
// Mason Yu. SIGRTMIN for DHCP Relay.catch SIGRTMIN(090605)
#ifdef COMBINE_DHCPD_DHCRELAY
	dhcrelay_pid = read_pid("/var/run/udhcpd.pid");
#else
	dhcrelay_pid = read_pid("/var/run/dhcrelay.pid");
#endif
	if (dhcrelay_pid > 0) {
		printf("restartWAN1: kick dhcrelay(%d) to Re-discover all network interface\n", dhcrelay_pid);
		// Mason Yu. catch SIGRTMIN(It is a real time signal and number is 32) for re-sync all interface on DHCP Relay
		// Kaohj -- SIGRTMIN is not constant, so we use 32.
		//kill(dhcrelay_pid, SIGRTMIN);
		kill(dhcrelay_pid, 32);
	}

#if defined(CONFIG_USER_RTK_VLAN_PASSTHROUGH)
	if(is_tagged_br_wan_exist() == 1)
		set_vlan_passthrough(0);
	else
		set_vlan_passthrough(1);
#endif

	return;
}
void addEthBrWANdev(MIB_CE_BR_WAN_Tp pEntry)
{
	MEDIA_TYPE_T mType;
	char ifname[IFNAMSIZ];
	char macaddr[MAC_ADDR_LEN*2+1];
	int flag=0, tag=0;

	ifGetName(PHY_INTF(pEntry->ifIndex), ifname, sizeof(ifname));
	mType = MEDIA_INDEX(pEntry->ifIndex);

	//add new ethwan dev here.
	if((mType == MEDIA_ETH) && (WAN_MODE & MODE_Ethernet)){
#if defined(CONFIG_RTL_SMUX_DEV)
		char cmd_str[256]={0}, *pcmd_str = NULL;
		unsigned char *rootdev=NULL;
		unsigned int vlantag;
		rootdev=ALIASNAME_NAS0;
		sprintf(cmd_str, "/bin/smuxctl --if-create-name %s %s --set-if-rsmux", rootdev, ifname);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
		//RX
		{
			pcmd_str = cmd_str;
			pcmd_str += sprintf(pcmd_str, "%s --if %s --rx", SMUXCTL, rootdev);
			if (pEntry->vlan)
			{// filter vlan ID on RX
				//vlantag = (pEntry->vid|((pEntry->vprio) << 13));
				vlantag = pEntry->vid;
				pcmd_str += sprintf(pcmd_str, " --tags 1 --filter-vid 0x%x 1", pEntry->vid);
			}
			else {
				pcmd_str += sprintf(pcmd_str, " --tags 0");
			}
			pcmd_str += sprintf(pcmd_str, " --rule-priority %d", SMUX_RULE_PRIO_BCMC);
			pcmd_str += sprintf(pcmd_str, " --set-rxif %s --rule-alias %s-rx-default-root --target ACCEPT --rule-append", ifname, ifname);
			printf("==> [%s] %s\n", __FUNCTION__, cmd_str);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
		}
		if (pEntry->vlan)
		{
			sprintf(cmd_str, "%s --if %s --rx --tags 1 --filter-vid 0x%x 1 --pop-tag --rule-alias %s-rx-default-leaf --rule-append", SMUXCTL, ifname, pEntry->vid, ifname);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
		}
		//TX
		if (pEntry->vlan)
		{
			vlantag = pEntry->vid;
			for (tag = 0; tag <= SMUX_MAX_TAGS; tag++) {
				cmd_str[0] = '\0';
				pcmd_str = cmd_str;
				pcmd_str += sprintf(pcmd_str, "%s --if %s --tx", SMUXCTL, ifname);
				pcmd_str += sprintf(pcmd_str, " --tags %d",tag);
				if(pEntry->vlan)
				{
					if(pEntry->vprio)
					{
						if(tag == 0)
							pcmd_str += sprintf(pcmd_str, " --push-tag --set-vid 0x%x 1 --set-priority %d 1", vlantag, (pEntry->vprio-1));
						else
							pcmd_str += sprintf(pcmd_str, " --set-vid 0x%x %d --set-priority %d %d", vlantag, tag, (pEntry->vprio-1), tag);
					}
					else
					{
						if(tag == 0)
							pcmd_str += sprintf(pcmd_str, " --push-tag --set-vid 0x%x 1", vlantag);
						else
							pcmd_str += sprintf(pcmd_str, " --set-vid 0x%x %d", vlantag, tag);
					}
				}
				if(pEntry->vprio)
					pcmd_str += sprintf(pcmd_str, " --target ACCEPT --rule-alias %s-tx-default --rule-append", ifname);
				else
					pcmd_str += sprintf(pcmd_str, " --rule-alias %s-tx-default --rule-append", ifname);
				printf("==> [%s] %s\n", __FUNCTION__, cmd_str);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
			}
		}
#endif

	}
}
int startBrWan(int configAll, MIB_CE_BR_WAN_Tp pEntry)
{
	int vcTotal, i, ret;
	MIB_CE_BR_WAN_T Entry;
	FILE *fp;
	char vcNum[16];
	char ifname[IFNAMSIZ];
	char tmpBuf[128] = {0};
	char vChar=-1;
	unsigned char *rootdev=NULL;
	unsigned char devAddr[MAC_ADDR_LEN];
	char macaddr[MAC_ADDR_LEN*2+1];
	unsigned int mtu;
	char mtuStr[10];
	ret = 1;
	vcTotal = mib_chain_total(MIB_BR_WAN_TBL);
#if defined(CONFIG_RTL_MULTI_ETH_WAN)
	mib_get_s(MIB_ELAN_MAC_ADDR, (void *)devAddr, sizeof(devAddr));
	snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",devAddr[0], devAddr[1], devAddr[2],devAddr[3], devAddr[4], devAddr[5]);
#if defined(CONFIG_RTL_MULTI_ETH_WAN)
	rootdev=ALIASNAME_NAS0;
	mtu = rtk_wan_get_max_wan_mtu();
	snprintf(mtuStr, sizeof(mtuStr), "%u", mtu);
#ifndef CONFIG_ARCH_RTL8198F
	va_cmd(IFCONFIG, 4, 1, rootdev, "hw", "ether", macaddr);
#endif
	va_cmd(IFCONFIG, 2, 1, rootdev, "up");
	va_cmd(IFCONFIG, 3, 1, rootdev, "mtu", mtuStr);
#endif
	for (i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_BR_WAN_TBL, i, (void *)&Entry))
				return -1;
		if (Entry.enable == 0 || Entry.vlan == 0)
			continue;
		if(configAll == CONFIGALL)
			addEthBrWANdev(&Entry);
		else if(configAll == CONFIGONE)
		{
			if(pEntry && Entry.ifIndex == pEntry->ifIndex)
			{
				addEthBrWANdev(&Entry);
				break;
			 }
		}
	}
#endif

	for (i = 0; i < vcTotal; i++)
	{
		/* get the specified chain record */
		if (!mib_chain_get(MIB_BR_WAN_TBL, i, (void *)&Entry))
			return -1;

		if(!isInterfaceMatch(Entry.ifIndex))  // Magician: Only raise interfaces that was set by wan mode.
			continue;

		if (Entry.enable == 0 || Entry.vlan == 0)
			continue;

		if (configAll == CONFIGALL) 		// config for ALL
			ret |= startBrConnection(&Entry, i);
		else if (configAll == CONFIGONE)	// config for one
		{
			if (pEntry != NULL) {
				if (Entry.ifIndex == pEntry->ifIndex) {
					ret |= startBrConnection(&Entry, i);
				}
				else
					continue;
			}
		}

	}

	for (i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_BR_WAN_TBL, i, (void *)&Entry))
			return -1;
		if (Entry.enable == 0 || Entry.vlan == 0)
			continue;

#ifdef CONFIG_BRIDGE_IGMP_SNOOPING
		if ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_BRIDGE){
			ifGetName(Entry.ifIndex,ifname,sizeof(ifname));
			snprintf(tmpBuf,sizeof(tmpBuf),"echo 2 > /sys/devices/virtual/net/%s/brif/%s/multicast_router",BRIF,ifname);
			system(tmpBuf);

			snprintf(tmpBuf,sizeof(tmpBuf),"echo 1 > /sys/devices/virtual/net/%s/brif/%s/multicast_fast_leave",BRIF,ifname);
			system(tmpBuf);
		}
#endif
	}

/* Start IPv6 LAN servers */
#ifdef CONFIG_IPV6
	if (pEntry && (pEntry->IpProtocol & IPVER_IPV6))
	restartLanV6Server();
#endif

	return ret;
}
#endif

int rtk_wan_is_link_status_change(char *ifname, unsigned int ifi_flags)
{
	FILE *fp=NULL;
	char fname[64];
	unsigned int flags;

	snprintf(fname, 64, "/tmp/linkStatus.%s", ifname);
	fp = fopen(fname, "r");
	if (fp)
	{
		if(fscanf(fp, "%x", &flags) != 1)
		{
			goto changed;
		}

		if (!((ifi_flags ^ flags) & (IFF_UP | IFF_RUNNING)))
		{
			fclose(fp);
			return 0;
		}
	}

changed:
	if (fp) fclose(fp);
	return 1;
}

void rtk_wan_log_link_status(char *ifname, unsigned int ifi_flags)
{
	FILE *fp=NULL;
	char fname[64];

	snprintf(fname, 64, "/tmp/linkStatus.%s", ifname);
	fp = fopen(fname, "w");
	if (fp)
	{
		fprintf(fp, "%x", ifi_flags);
		fclose(fp);
	}
}

void rtk_wan_clear_link_status(char *ifname)
{
	FILE *fp=NULL;
	char fname[64];

	snprintf(fname, 64, "/tmp/linkStatus.%s", ifname);
	unlink(fname);
}

void stopConnection(MIB_CE_ATM_VC_Tp pEntry)
{
	struct data_to_pass_st msg;
	char wandev[IFNAMSIZ];
	char wanif[IFNAMSIZ];
	char myip[16];
	int i;
#ifdef CONFIG_RTL_MULTI_PVC_WAN
	char ifname[IFNAMSIZ];   // major vc device name
	int found_another=0;
#elif defined(CONFIG_IPV6)
	char ifname[IFNAMSIZ];
#endif
#ifdef CONFIG_USER_CMD_SERVER_SIDE
	struct rtk_xdsl_atm_channel_info data;
        int ch_no=-1;
#endif
	char cmd_buff[64] = {'\0'};
	char wandev_br[IFNAMSIZ + 1];

	//clean_SourceRoute(pEntry);

#if defined(PORT_FORWARD_GENERAL) || defined(DMZ)
#ifdef NAT_LOOPBACK
	cleanOneEntry_NATLB_rule_dynamic_link(pEntry, DEL_ALL_NATLB_DYNAMIC);
#endif
#endif

#ifdef CONFIG_USER_WT_146
	wt146_del_wan(pEntry);
#endif //CONFIG_USER_WT_146

//  UPnP Daemon Stop
#if defined(CONFIG_USER_MINIUPNPD)
	restart_upnp(CONFIGONE, 0, pEntry->ifIndex, 0);
#endif
// The end of UPnP Daemon Stop

#ifdef CONFIG_PPP
	// delete this vc
	if (pEntry->cmode == CHANNEL_MODE_PPPOE || pEntry->cmode == CHANNEL_MODE_PPPOA)
	{
		int i,entryNum;
		MIB_CE_ATM_VC_T Entry;

#ifdef CONFIG_USER_PPPD
		disconnectPPP(PPP_INDEX(pEntry->ifIndex));
#endif //CONFIG_USER_PPPD
#ifdef CONFIG_USER_SPPPD
		// spppctl down 0, to release dhcpv6 address if exist
		snprintf(msg.data, BUF_SIZE,
			"spppctl down %u", PPP_INDEX(pEntry->ifIndex));
		snprintf(wanif, 6, "ppp%u", PPP_INDEX(pEntry->ifIndex));
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
		// spppctl del 0
		snprintf(msg.data, BUF_SIZE,
			"spppctl del %u", PPP_INDEX(pEntry->ifIndex));
		snprintf(wanif, 6, "ppp%u", PPP_INDEX(pEntry->ifIndex));
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
#endif //CONFIG_USER_SPPPD
#if 0
		/* Andrew, need to remove all PPP using the same underlying VC or kernel will have NULL references */
		entryNum =  mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < entryNum; i++) {
			/* Retrieve entry */
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				continue;

			if ((Entry.cmode != CHANNEL_MODE_PPPOE) && (Entry.cmode != CHANNEL_MODE_PPPOA))
				continue;
			if (VC_INDEX(Entry.ifIndex) != VC_INDEX(pEntry->ifIndex))
				continue;
			if (MEDIA_INDEX(pEntry->ifIndex) != MEDIA_INDEX(Entry.ifIndex))
				continue;

			snprintf(msg.data, BUF_SIZE, "spppctl del %u", PPP_INDEX(Entry.ifIndex));
			TRACE(STA_SCRIPT, "%s\n", msg.data);
			write_to_pppd(&msg);
		}
#endif
	}
	else{
#endif
		//snprintf(wanif, 6, "vc%u", VC_INDEX(pEntry->ifIndex));
		ifGetName( PHY_INTF(pEntry->ifIndex), wanif, sizeof(wanif));
#ifdef CONFIG_PPP
	}
#endif

	ifGetName( PHY_INTF(pEntry->ifIndex), wandev, sizeof(wandev));
#ifdef CONFIG_IPV6
	if(pEntry->IpProtocol & IPVER_IPV4)
#endif
	{
		set_ipv4_policy_route(pEntry, 0);
		set_ipv4_default_policy_route(pEntry, 0);
		sprintf(cmd_buff, "%s.%s", DNS_RESOLV, wandev);
		if (access(cmd_buff, F_OK) == 0) unlink(cmd_buff);
		sprintf(cmd_buff, "%s.%s", (char *)MER_GWINFO, wandev);
		if (access(cmd_buff, F_OK) == 0) unlink(cmd_buff);
	}
#ifdef CONFIG_IPV6
	if(pEntry->IpProtocol & IPVER_IPV6)
	{
#ifdef DUAL_STACK_LITE
		// reset dslite policy route rule
		if((pEntry->IpProtocol == IPVER_IPV6) && pEntry->dslite_enable){
			set_ipv4_policy_route(pEntry, 0);
			set_ipv4_default_policy_route(pEntry, 0);
		}
#endif
		set_ipv6_policy_route(pEntry, 0);
		set_ipv6_default_policy_route(pEntry, 0);
		sprintf(cmd_buff, "%s.%s", DNS6_RESOLV, wandev);
		if (access(cmd_buff, F_OK) == 0) unlink(cmd_buff);
		sprintf(cmd_buff, "%s.%s", (char *)MER_GWINFO_V6, wandev);
		if (access(cmd_buff, F_OK) == 0) unlink(cmd_buff);
		ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));
#ifndef CONFIG_TELMEX
		snprintf(cmd_buff, 64, "%s.%s", PATH_DHCLIENT6_LEASE, ifname);
		if (access(cmd_buff, F_OK) == 0) unlink(cmd_buff);
#endif
#if defined(CONFIG_USER_RTK_MULTICAST_VID) && defined(CONFIG_RTK_SKB_MARK2)
		//This rule add in rtk_iptv_mvlan_config()
		snprintf(cmd_buff, sizeof(cmd_buff),"%s -D INPUT -i %s -p icmpv6 --icmpv6-type router-advertisement -m mark2 --mark2 0x%llX/0x%llX -j DROP",
				IP6TABLES, wandev, SOCK_MARK2_RTK_IGMP_MLD_VID_SET(0,1), SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_buff);
#endif
	}
#endif
	#if defined(CONFIG_RTL_MULTI_ETH_WAN) && !defined(CONFIG_RTL_SMUX_DEV) && defined(SUPPORT_PORT_BINDING)
	//remove port-mapping before remove wan, /bin/ethctl port_mapping nas0_1 0x2 1
	snprintf(cmd_buff, sizeof(cmd_buff), "/bin/ethctl port_mapping %s %x 0", wandev, pEntry->itfGroup);
	system(cmd_buff);
	printf("%s %d wandev=%s Entry.itfGroup=0x%x Entry.ifIndex=0x%x buff=%s \n", __func__, __LINE__, wandev, pEntry->itfGroup, pEntry->ifIndex, cmd_buff);
	#endif
#ifdef CONFIG_IPV6
	if (pEntry->IpProtocol & IPVER_IPV6) {
		stopIP_PPP_for_V6(pEntry);
	}
#endif
#ifdef CONFIG_PPPOE_MONITOR
	if (
		((pEntry->cmode == CHANNEL_MODE_PPPOE)&&ROUTE_CHECK_BRIDGE_MIX_ENABLE(pEntry)) ||
		(pEntry->cmode == CHANNEL_MODE_BRIDGE))
	{
		del_pppoe_monitor_wan_interface(wandev);
	}
#endif

	sleep(1);
#ifdef NEW_PORTMAPPING
	set_pmap_rule(pEntry, 0);
#endif
	if(pEntry->cmode == CHANNEL_MODE_BRIDGE)
		va_cmd(BRCTL, 3, 1, "delif", BRIF, wandev);

	if (ROUTE_CHECK_BRIDGE_MIX_ENABLE(pEntry))
	{
		snprintf(wandev_br, sizeof(wandev_br), "%s%s", wandev, ROUTE_BRIDGE_WAN_SUFFIX);
		va_cmd(BRCTL, 3, 1, "delif", BRIF, wandev_br);
		va_cmd(IFCONFIG, 2, 1, wandev_br, "down");
#if defined(CONFIG_RTL_SMUX_DEV)
		snprintf(cmd_buff, sizeof(cmd_buff), "/bin/smuxctl --if-delete %s", wandev_br);
		printf("==> [%s] %s\n", __FUNCTION__, cmd_buff);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_buff);
#endif
	}
	if (pEntry->cmode != CHANNEL_MODE_BRIDGE)
		internetLed_route_if(0);//-1

	/* Kevin, stop root Qdisc before ifdown interface, for 0412 hw closing IPQoS */
	va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", wandev, "root");

	if (pEntry->cmode != CHANNEL_MODE_PPPOA) {
		//ifconfig vc0 0.0.0.0
		va_cmd(IFCONFIG, 2, 1, wanif, "0.0.0.0");
		// ifconfig vc0 down
		va_cmd(IFCONFIG, 2, 1, wanif, "down");
		// wait for sar to process the queueing packets
		//usleep(1000);
		for (i=0; i<10000000; i++);

#ifdef CONFIG_IPV6
		if (pEntry->IpProtocol & IPVER_IPV4){
#endif
		if ((DHCP_T)pEntry->ipDhcp != DHCP_DISABLED)
		{
			int dhcpc_pid = 0;
			char filename[100] = {0};

			sprintf(filename, "%s.%s", (char*)DHCPC_PID, wandev);
			dhcpc_pid = read_pid(filename);
			if (dhcpc_pid > 0)
#ifdef CONFIG_RTK_DEV_AP
				kill(dhcpc_pid, SIGTERM);
#else
				kill(dhcpc_pid, SIGHUP);
#endif
			fprintf(stderr, "[%s] kill %s\n", __FUNCTION__, filename);
		}
#ifdef CONFIG_IPV6
		}
#endif

		#ifdef CONFIG_DEV_xDSL
		if (MEDIA_INDEX(pEntry->ifIndex) == MEDIA_ATM) {
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			// Get major vc device name
			ifGetName(MAJOR_VC_INTF(pEntry->ifIndex), ifname, sizeof(ifname));
			// ethctl remsumx vcX vcX_X
			//printf("** Remove smux device %s(root %s)\n", wanif, ifname);
#if defined(CONFIG_RTL_SMUX_DEV)
			char cmd_str[256]={0};
			sprintf(cmd_str, "/bin/smuxctl --if-delete %s", wandev);
			printf("==> [%s] %s\n", __FUNCTION__, cmd_str);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#else
			if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
			{
				va_cmd("/bin/ethctl", 4, 1, "remsmux", "bridge", ifname, wandev);
			}
			else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_IPOE)
			{
				va_cmd("/bin/ethctl", 4, 1, "remsmux", "ipoe", ifname, wandev);
			}
			else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
			{
				va_cmd("/bin/ethctl", 4, 1, "remsmux", "pppoe", ifname, wandev);
			}
#endif
#endif

			// mpoactl del vc0
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			found_another = find_latest_used_vc(pEntry);
			if (!found_another) {
				// delete major vc name
				//printf("**stopConnection: delete major vc name=%s\n", ifname);
				//ifconfig vc0 0.0.0.0
				va_cmd(IFCONFIG, 2, 1, ifname, "0.0.0.0");
				// ifconfig vc0 down
				va_cmd(IFCONFIG, 2, 1, ifname, "down");
				// wait for sar to process the queueing packets
				//usleep(1000);
				for (i=0; i<10000000; i++);

				snprintf(msg.data, BUF_SIZE, "mpoactl del %s", ifname);
				TRACE(STA_SCRIPT, "%s\n", msg.data);
				write_to_mpoad(&msg);

				// make sure this vc been deleted completely
				while (find_pvc_from_running(pEntry->vpi, pEntry->vci));
			}
#else
			if (((CHANNEL_MODE_T)pEntry->cmode != CHANNEL_MODE_PPPOE) || (!underlying_pppoe_exist(pEntry)))
			{
				snprintf(msg.data, BUF_SIZE, "mpoactl del %s", wandev);
				TRACE(STA_SCRIPT, "%s\n", msg.data);
				write_to_mpoad(&msg);

				// make sure this vc been deleted completely
				while (find_pvc_from_running(pEntry->vpi, pEntry->vci));
			}
#endif

#ifdef CONFIG_USER_CMD_SERVER_SIDE
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			if (VC_MINOR_INDEX(MAJOR_VC_INTF(pEntry->ifIndex)) == DUMMY_VC_MINOR_INDEX) // major vc devname
				ch_no = VC_MAJOR_INDEX(MAJOR_VC_INTF(pEntry->ifIndex));
			else{
				//ch_no = VC_MAJOR_INDEX(MAJOR_VC_INTF(pEntry->ifIndex));
			}
#else
			ch_no = VC_INDEX(PHY_INTF(pEntry->ifIndex));
#endif
			data.vpi = pEntry->vpi;
			data.vci = pEntry->vci;

			commserv_atmSetup(CMD_DEL, &data);
#endif
		}
		#endif
#if defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN) || defined(CONFIG_RTK_DEV_AP)
		//del ethwan dev here.
		if( (MEDIA_INDEX(pEntry->ifIndex) == MEDIA_ETH)
		     #ifdef CONFIG_PTMWAN
		     || (MEDIA_INDEX(pEntry->ifIndex) == MEDIA_PTM)
		     #endif /*CONFIG_PTMWAN*/
		){
			unsigned char *rootdev;

			#ifdef CONFIG_PTMWAN
			if(MEDIA_INDEX(pEntry->ifIndex) == MEDIA_PTM)
				rootdev=ALIASNAME_PTM0;
			else
			#endif /*CONFIG_PTMWAN*/
				rootdev=ALIASNAME_NAS0;


			/* patch for unregister_device fail: unregister_netdevice: waiting for nas0_3 to become free. Usage count = 1*/

#if defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN)
			printf("%s remove smux device %s\n", __func__, wandev);
#if defined(CONFIG_RTL_SMUX_DEV)
			char cmd_str[256]={0};
			sprintf(cmd_str, "/bin/smuxctl --if-delete %s", wandev);
			printf("==> [%s] %s\n", __FUNCTION__, cmd_str);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
#else
			if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
			{
				va_cmd("/bin/ethctl", 4, 1, "remsmux", "bridge", rootdev, wandev);
			}
			else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_IPOE || (CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_6RD
					||(CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_6IN4 ||(CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_6TO4)
			{
				va_cmd("/bin/ethctl", 4, 1, "remsmux", "ipoe", rootdev, wandev);
			}
			else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
			{
				if (!underlying_pppoe_exist(pEntry))
					va_cmd("/bin/ethctl", 4, 1, "remsmux", "pppoe", rootdev, wandev);
			}
#endif
#endif
		}
#endif
#ifdef WLAN_WISP
		if(MEDIA_INDEX(pEntry->ifIndex) == MEDIA_WLAN){
			int dhcpc_pid;
			unsigned char value[32];
			snprintf(value, 32, "%s.%s", (char*)DHCPC_PID, wanif);
			dhcpc_pid = read_pid((char*)value);
			if (dhcpc_pid > 0)
				kill(dhcpc_pid, SIGTERM);
		}
#endif
	}
	#ifdef CONFIG_DEV_xDSL
	else {
#ifdef CONFIG_USER_CMD_SERVER_SIDE
		data.vpi = pEntry->vpi;
		data.vci = pEntry->vci;
		commserv_atmSetup(CMD_DEL, &data);
#endif

		int repeat_i=0;
		//ifconfig vc0 0.0.0.0
		va_cmd(IFCONFIG, 2, 1, wanif, "0.0.0.0");
		va_cmd(IFCONFIG, 2, 1, wanif, "down");
		for (i=0; i<10000000; i++);
		while( (repeat_i<10) && (find_pvc_from_running(pEntry->vpi, pEntry->vci)) )
		{
			usleep(50000);
			repeat_i++;
		}
	}
	#endif

	// delete one NAT rule for the specific interface
	setup_NAT(pEntry, 0);

#ifdef CONFIG_HWNAT
	if (pEntry->napt == 1){
		if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
			send_extip_to_hwnat(DEL_EXTIP_CMD, wandev, *(uint32 *)pEntry->ipAddr);
		}
		else{
			send_extip_to_hwnat(DEL_EXTIP_CMD, wandev, 0);
		}
	}
#endif

#ifdef CONFIG_IPV6
	//for RFC6296 NPT feature, need reconfig proxy ndp when ip change
	if(pEntry->IpProtocol & IPVER_IPV6)
	{
#ifdef CONFIG_IP6_NF_TARGET_NPT
		if(pEntry->napt_v6 == 2)
		{
			set_v6npt_config(NULL);
		}
#endif
#ifdef CONFIG_USER_NDPPD
		if (pEntry->ndp_proxy == 1)
		{
			ndp_proxy_clean();
			ndp_proxy_setup_conf();
			ndp_proxy_start();
		}
#endif
	}
#endif
#if defined(CONFIG_GPON_FEATURE) && defined (CONFIG_TR142_MODULE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_tr142_sync_wan_info(pEntry, 0);
#endif

	ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));
		rtk_wan_clear_link_status(ifname);
#ifdef CONFIG_RTK_MAC_AUTOBINDING
	do{
			unsigned char flag=0;
			if(mib_get(MIB_AUTO_MAC_BINDING_IPTV_BRIDGE_WAN,&flag)&&flag){
				if (pEntry->cmode == CHANNEL_MODE_BRIDGE&&pEntry->applicationtype==X_CT_SRV_OTHER){
#ifdef CONFIG_SUPPORT_IPTV_APPLICATIONTYPE
						//if(pEntry->othertype==OTHER_IPTV_TYPE)
						{
							int ifIndex = if_nametoindex(wanif);
							char cmd_str[128]={0};
							if(ifIndex){
								sprintf(cmd_str,"echo %d > /proc/mac_autobinding/iptv_bridge_wan",-1);
								va_cmd("/bin/sh",2,1,"-c",cmd_str);
							}
						}

#endif
				}
			}
	}while(0);
#endif

}


char* config_WAN_action_type_to_str( int configAll, MIB_CE_ATM_VC_Tp pEntry, int isStart, char* ret )
{
	char action_type_str[128] = {0};
	char wan_info_str[128] = {0};

	sprintf(action_type_str, "%s", isStart?"Start":"Stop");

	switch( configAll )
	{
		case CONFIGONE:
			if(pEntry) sprintf(wan_info_str, "%s WAN VID=%d", (pEntry->cmode==CHANNEL_MODE_BRIDGE)?"BRIDGE":((pEntry->cmode==CHANNEL_MODE_IPOE)?"IPoE":"PPPoE"), pEntry->vid);
			else sprintf(wan_info_str, "WAN");
			break;
		case CONFIGALL:
			sprintf(wan_info_str, "all WAN");
			break;
		case CONFIGCWMP:
			if(pEntry) sprintf(wan_info_str, "CWMP Config %s WAN VID=%d", (pEntry->cmode==CHANNEL_MODE_BRIDGE)?"BRIDGE":((pEntry->cmode==CHANNEL_MODE_IPOE)?"IPoE":"PPPoE"), pEntry->vid);
			else sprintf(wan_info_str, "CWMP Config WAN");
			break;
		default:
			sprintf(ret, "Invalid action_type !");
			return ret;
	}

	sprintf(ret, "%s %s", action_type_str, wan_info_str);
	return ret;

}

// Added by Mason Yu
// configAll = CONFIGALL,  pEntry = NULL  : delete all WAN connections(include VC, ETHWAN, PTMWAN, VPN, 3g).
//									  It means that we want to restart all WAN channel.
// configAll = CONFIGONE, pEntry != NULL : delete specified VC, ETHWAN, PTMWAN connection and VPN, 3g connections.
// 									  It means that we delete or modify an old VC, ETHWAN, PTMWAN channel.
// configAll = CONFIGONE, pEntry = NULL  : delete VPN, 3g connections. It means that we add a new VC, ETHWAN, PTMWAN channel.
int deleteConnection(int configAll, MIB_CE_ATM_VC_Tp pEntry)
{
	char action_type_str[128] = {0};
	unsigned int entryNum, i, idx;
	MIB_CE_ATM_VC_T Entry;
	char ifName[IFNAMSIZ];
#ifdef CONFIG_USER_LANNETINFO
	int lannetinfo_pid = 0;
#endif

	AUG_PRT("%s %s\n", __func__, config_WAN_action_type_to_str(configAll, pEntry, 0, action_type_str));
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	syslog(LOG_CRIT, "%s", config_WAN_action_type_to_str(configAll, pEntry, 0, action_type_str));
#else
	syslog(LOG_INFO, "%s", config_WAN_action_type_to_str(configAll, pEntry, 0, action_type_str));
#endif

	//close all tunnel before delete wan interface.
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	MIB_PPTP_T pptp;

	entryNum = mib_chain_total(MIB_PPTP_TBL);
	for (i=0; i<entryNum; i++)
	{
		if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&pptp) )
			return -1;

		if(configAll == CONFIGALL)
		{
			applyPPtP(&pptp, 0, i);
		}
		else if(configAll == CONFIGONE)
		{
			if(pEntry && (pEntry->cmode != CHANNEL_MODE_BRIDGE) && (pEntry->applicationtype & (X_CT_SRV_INTERNET | X_CT_SRV_OTHER)))
			{
				applyPPtP(&pptp, 0, i);
			}
			else
				break;
		}
	}
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	MIB_L2TP_T l2tp;
	entryNum = mib_chain_total(MIB_L2TP_TBL);
	for (i=0; i<entryNum; i++)
	{
		if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tp) )
			return -1;

		if(configAll == CONFIGALL)
		{
			applyL2TP(&l2tp, 0, i);
		}
		else if(configAll == CONFIGONE)
		{
			if(pEntry && (pEntry->cmode != CHANNEL_MODE_BRIDGE) && (pEntry->applicationtype & (X_CT_SRV_INTERNET | X_CT_SRV_OTHER)))
			{
				applyL2TP(&l2tp, 0, i);
			}
			else
				break;
		}
	}
#endif

	if (configAll == CONFIGALL)
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < entryNum; i++) {
			/* Retrieve entry */
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
				printf("deleteConnection: cannot get ATM_VC_TBL(ch=%d) entry\n", i);
				continue;
			}
#if defined(CONFIG_CTC_SDN)
			if((Entry.enable && Entry.sdn_enable) &&
				check_ovs_enable() && check_ovs_running() == 0)
			{
				if(unbind_ovs_interface_by_mibentry(OFP_PTYPE_WAN, &Entry, -1, -1, 1) == 0)
				{
					printf("=====> Do unbind WAN(ifIndex=%x) to SDN <=====\n", Entry.ifIndex);
				}
			}
#endif
			if(Entry.enable) {
				/* remove connection on driver*/
#ifdef CONFIG_IPV6
				stop6rd_6in4_6to4(&Entry);
#endif
				stopConnection(&Entry);
			}
#ifdef _PRMT_X_CT_COM_WANEXT_
			clean_IPForward_by_WAN_entry(&Entry);
#endif
			ifGetName(Entry.ifIndex, ifName, sizeof(ifName));
			rtk_net_remove_wan_policy_route_mark(ifName);

#ifdef CONFIG_E8B
			if(Entry.dgw) {
				if(Entry.dgw & IPVER_IPV4) _check_and_update_default_route_entry(ifName, &Entry, IPVER_IPV4, 0);
				if(Entry.dgw & IPVER_IPV6) _check_and_update_default_route_entry(ifName, &Entry, IPVER_IPV6, 0);
			}
#endif
#ifdef CONFIG_IPV6
			clear_delegated_default_wanconn(&Entry);
#endif
		}
	}
	else if (configAll == CONFIGONE)
	{
		if (pEntry) {
#if defined(CONFIG_CTC_SDN)
			if(pEntry->enable && pEntry->sdn_enable &&
				check_ovs_enable() && check_ovs_running() == 0)
			{
				if(unbind_ovs_interface_by_mibentry(OFP_PTYPE_WAN, pEntry, -1, -1, 1) == 0)
				{
					printf("=====> Do unbind WAN(ifIndex=%x) to SDN <=====\n", pEntry->ifIndex);
				}
			}
#endif
			if (pEntry->enable) {
				/* remove connection on driver*/
#ifdef CONFIG_IPV6
				stop6rd_6in4_6to4(pEntry);
#endif
				stopConnection(pEntry);
			}
#ifdef _PRMT_X_CT_COM_WANEXT_
			clean_IPForward_by_WAN_entry(pEntry);
#endif
			ifGetName(pEntry->ifIndex, ifName, sizeof(ifName));
			rtk_net_remove_wan_policy_route_mark(ifName);

#ifdef CONFIG_E8B
			if(pEntry->dgw) {
				if(pEntry->dgw & IPVER_IPV4) _check_and_update_default_route_entry(ifName, pEntry, IPVER_IPV4, 0);
				if(pEntry->dgw & IPVER_IPV6) _check_and_update_default_route_entry(ifName, pEntry, IPVER_IPV6, 0);
			}
#endif
#ifdef CONFIG_IPV6
			clear_delegated_default_wanconn(pEntry);
#endif
		}
	}
#ifdef CONFIG_USER_LANNETINFO
		lannetinfo_pid = read_pid(LANNETINFO_RUNFILE);
		if (lannetinfo_pid > 0)
			kill(lannetinfo_pid, SIGUSR1);
#endif

#ifdef CONFIG_USER_PPPOMODEM
	if((configAll == CONFIGALL) || (pEntry && (TO_IFINDEX(MEDIA_3G,  MODEM_PPPIDX_FROM, 0) == pEntry->ifIndex)))
	{
		wan3g_stop();
	}
#endif //CONFIG_USER_PPPOMODEM

#if defined(CONFIG_CT_AWIFI_JITUAN_SMARTWIFI)
	unsigned char functype=0;
	mib_get(AWIFI_PROVINCE_CODE, &functype);
	if(functype == AWIFI_ZJ){
		g_wan_modify=1;
	}
#endif
#if defined(CONFIG_CMCC_WIFI_PORTAL)
	g_wan_modify=1;
#endif

	return 0;
}

static int startWanServiceDependency(int ipEnabled, int isBoot)
{
	char vChar=-1;
	int ret=0;
	int rc=0;

#ifdef TIME_ZONE
		unsigned int if_wan=DUMMY_IFINDEX;
#endif

#ifdef DEFAULT_GATEWAY_V2
	unsigned char dgwip[16];
	unsigned int dgw;
	if (mib_get_s(MIB_ADSL_WAN_DGW_ITF, (void *)&dgw, sizeof(dgw)) != 0) {
		if (dgw == DGW_NONE && getMIB2Str(MIB_ADSL_WAN_DGW_IP, dgwip) == 0) {
			if (ifExistedDGW() == 1)
				va_cmd(ROUTE, 2, 1, ARG_DEL, "default");
			// route add default gw remotip
			va_cmd(ROUTE, 4, 1, ARG_ADD, "default", "gw", dgwip);
		}
	}
#endif

	// Add static routes
	// Mason Yu. Init hash table for all routes on RIP
#ifdef ROUTING
	addStaticRoute();
#endif
#if defined(CONFIG_IPV6) && defined(ROUTING)
	deleteV6StaticRoute(); //if WAN exist, just change configure
	addStaticV6Route();
#endif
/*
#if defined(CONFIG_IPV6) && defined(CONFIG_USER_RADVD)
	//Kill originally running RADVD daemon.
	//Alan
	//1. fix IPv4 routing WAN only, the radvd process still exist
	//2. configd and boa both will restart radvd, we leave configd to start radvd
	printf("radvd mode is %d \n", rc);
	if (rc==RADVD_RUNNING_MODE_DISABLE)
	{
		int pid = read_pid((char *)RADVD_PID);
		if ( pid > 0)
		{
			printf("Bridge mode only, kill the original radvd deamon.\n");
			kill(pid,SIGTERM);
			//radvd will remove pid file, we do not remove pid file
			//va_cmd("/bin/rm", 1, 0, (char *)RADVD_PID);
		}
	}
#endif
*/
//IulianWu. move the proc operation to /etc/init.d/rcX
/*
	if (ipEnabled!=1)
	{
		setup_ipforwarding(0);
	}
*/

#ifdef CONFIG_USER_ROUTED_ROUTED
	if (startRip() == -1)
	{
		printf("start RIP failed !\n");
		ret = -1;
	}
#endif

#ifdef CONFIG_USER_QUAGGA_RIPNGD
	if (startRipng() == -1)
	{
		printf("start RIPNG failed !\n");
		ret = -1;
	}
#endif

#ifdef CONFIG_USER_QUAGGA
	rtk_restart_igp();
#endif

#if defined(CONFIG_E8B) && defined(TIME_ZONE)
	if_wan = check_ntp_if();
	if (if_wan == DUMMY_IFINDEX) {
		stopNTP();
		printf("kill SNTP process, because can't find the ntp wan!\n");
	}
	if (startNTP())
	{
		printf("start SNTP failed !\n");
		ret = -1;
	}
#endif

#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
	if (startOspf() == -1)
	{
		printf("start OSPF failed !\n");
		ret = -1;
	}
#endif

#ifndef CONFIG_00R0
#ifdef CONFIG_USER_IGMPPROXY
#ifdef USE_DEONET
		if (startIgmproxy() == -1)
		{
			printf("start IGMP proxy failed !\n");
			ret = -1;
		}
#else
#ifdef CONFIG_IGMPPROXY_MULTIWAN
		if (setting_Igmproxy() == -1)
		{
			printf("start IGMP proxy failed !\n");
			ret = -1;
		}
#endif // of CONFIG_IGMPPROXY_MULTIWAN
#endif // of USE_DEONET
#endif // of CONFIG_USER_IGMPPROXY

#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_MLDPROXY
		if (startMLDproxy() == -1)
		{
			printf("start MLD proxy failed !\n");
			ret = -1;
		}
#endif // of CONFIG_USER_MLDPROXY
#endif
#endif //CONFIG_00R0

#ifdef CONFIG_USER_DOT1AG_UTILS
		startDot1ag();
#endif

#if defined(CONFIG_USER_RTK_NAT_ALG_PASS_THROUGH) || (defined(CONFIG_TRUE) && defined(CONFIG_IP_NF_ALG_ONOFF))
	if(setupAlgOnOff()==-1)
	{
		printf("execute ALG on-off failed!\n");
	}
#endif

#if 0 //eason path for pvc add/del and conntrack killall
	// Kaohj --- remove all conntracks
	va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
#endif

#ifdef WEB_REDIRECT_BY_MAC
	if( -1==start_web_redir_by_mac() )
		ret=-1;
#endif

#if defined(SUPPORT_WEB_REDIRECT) && defined(CONFIG_E8B)
	if(!check_user_is_registered())
		enable_http_redirect2register_page(1);
#endif


#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	unsigned char cp_enable;
	char cp_url[MAX_URL_LEN];

	mib_get_s(MIB_CAPTIVEPORTAL_ENABLE, (void *)&cp_enable, sizeof(cp_enable));
	mib_get_s(MIB_CAPTIVEPORTAL_URL, (void *)cp_url, sizeof(cp_url));

	if(cp_enable && cp_url[0])
		if( -1 == start_captiveportal() )
			ret = -1;
#endif

#ifdef CONFIG_USER_Y1731
	Y1731_start(0);
#endif

#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
	rtk_qos_control_packet_protection();
#endif

	return ret;
}

#if ! defined(_LINUX_2_6_) && defined(CONFIG_RTL_MULTI_WAN)
int initWANMode()
{
#if !defined(CONFIG_RTL_SMUX_DEV)
	FILE *fp = NULL;
#endif
	int wan_mode=0;
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE) || defined(CONFIG_FIBER_FEATURE)
	mib_get_s(MIB_PON_MODE, &wan_mode, sizeof(wan_mode));
#endif
#if !defined(CONFIG_RTL_SMUX_DEV)
	fp = fopen("/proc/rtk_smux/wan_mode", "w");
	if(fp){
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		fprintf(fp, "%d", 0); //force wan_mode to ETHMODE, For FC no implementation for carrier interface
#else
		fprintf(fp, "%d", wan_mode);
#endif
		fclose(fp);
	}
#endif
#if defined(CONFIG_FIBER_FEATURE)
	if(wan_mode == FIBER_MODE){
		system("/bin/ethctl enable_nas0_wan 1");
	}
	else
#endif
	if(get_net_link_status(ALIASNAME_NAS0)){
		system("/bin/ethctl enable_nas0_wan 1");
	}
	return 1;
}
static void checkWanModeStatus()
{
	int wan_mode=0, ret = 0;
	char cmd[64] = {0};

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE) || defined(CONFIG_FIBER_FEATURE)
	mib_get_s(MIB_PON_MODE, &wan_mode, sizeof(wan_mode));
#endif
#if defined(CONFIG_EPON_FEATURE)
	if(wan_mode == EPON_MODE)
	{
		int eonu = 0;

		eonu = getEponONUState(0);
		if(eonu == 5){
			ret = epon_getAuthState(0);
		}
	}
#endif
	if (ret == 1){
		printf(">>>> Checked WAN Mode is connected, recover SMUX interface status ......\n");
		snprintf(cmd, sizeof(cmd)-1, "/bin/ethctl setsmux %s \"*\" carrier 1", ALIASNAME_NAS0);
		system (cmd);
	}
}
#endif
#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
int setup_wan_port_power_on_off(int phy_port, int power_on)
{
	char cmd[128] = {0};

	memset(cmd, 0, sizeof(cmd));

	if(phy_port < 0)
		return 0;

	if(phy_port >= 0 && phy_port < SW_PORT_NUM)
	{
		if(power_on == 1)  //link up wan port
			snprintf(cmd, sizeof(cmd), "diag port set phy-force-power-down port %d state disable", phy_port);
		else
			snprintf(cmd, sizeof(cmd), "diag port set phy-force-power-down port %d state enable", phy_port);
	}

	if(phy_port >= SW_PORT_NUM)
	{
		if(power_on == 1)  //just used on ext-port 5, for test
			snprintf(cmd, sizeof(cmd), "diag debug ext-mdio c22 set 0 0x1140");
		else
			snprintf(cmd, sizeof(cmd), "diag debug ext-mdio c22 set 0 0x1940");
	}

	system(cmd);

	return 1;
}
#endif

#if defined(CONFIG_USER_MULTI_PHY_ETH_WAN_RATE)
int setup_wan_phy_rate(int phy_port, int phy_rate)
{
	char cmd[128] = {0};

	memset(cmd, 0, sizeof(cmd));

	if(phy_port < 0 || phy_rate < 0)
		return 0;

	if(phy_port >= 0 && phy_port < SW_PORT_NUM)
	{
		if(phy_rate == 0 || phy_rate == 1000)  //auto
		{
			snprintf(cmd, sizeof(cmd), "diag port set auto-nego port %d ability 10h 10f 100h 100f 1000f flow-control", phy_port);
		}
		else if(phy_rate == 100)
		{
			snprintf(cmd, sizeof(cmd), "diag port set auto-nego port %d ability 10h 10f 100h 100f flow-control", phy_port);
		}
		else if(phy_rate == 10)
		{
			snprintf(cmd, sizeof(cmd), "diag port set auto-nego port %d ability 10h 10f flow-control", phy_port);
		}
	}

	if(phy_port >= SW_PORT_NUM)
	{
		if(phy_rate == 0)
		{
			snprintf(cmd, sizeof(cmd), "echo phy_ability auto > /proc/realtek/ext_phy_polling");
		}
		else
		{
			snprintf(cmd, sizeof(cmd), "echo phy_ability %dM > /proc/realtek/ext_phy_polling", phy_rate);
		}
	}

	system(cmd);

	return 1;
}
#endif

int setup_wan_port(void)
{
	char sysbuf[128];
	int wanPhyPort;
	unsigned int wanPhyPortMask=0;
	char realIfname[IFNAMSIZ] = {0};

#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	int logic_port;
	wanPhyPortMask=rtk_port_get_wan_phyMask();
	if (wanPhyPortMask == 0) { //bridge mode
		wanPhyPortMask = rtk_port_get_default_wan_phyMask();
		if (wanPhyPortMask) {
			wanPhyPort = ffs(wanPhyPortMask)-1;
			logic_port=rtk_port_get_default_wan_logicPort();
			if (logic_port >= 0) {
				rtk_port_set_dev_port_mapping(wanPhyPort, ELANRIF[logic_port]);
			}
		}
	} else {
		int i;
		for (i = 0; i < SW_PORT_NUM; i++) {
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
			if(rtk_port_check_external_port(i))
			{
				snprintf(realIfname, sizeof(realIfname)-1, "%s", EXTELANRIF[i]);
			}
			else
#endif
			{
				snprintf(realIfname, sizeof(realIfname)-1, "%s", ELANRIF[i]);
			}
			if (!rtk_port_is_wan_logic_port(i)) {
#ifdef CONFIG_RTL_SMUX_DEV
				va_cmd(SMUXCTL, 11, 1, "--if", realIfname, "--set-if-rsmux", "--set-if-rx-policy", "CONTINUE",
				"--set-if-tx-policy", "CONTINUE", "--set-if-rxmc-policy", "CONTINUE", "--set-if-rx-multi", "0");
#endif
				continue;
			}
			wanPhyPort = rtk_port_get_phyPort(i);
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
			if(rtk_port_check_external_port(i))
			{
				rtk_external_port_set_dev_port_mapping(rtk_external_port_get_lan_phyID(i), (char *)realIfname);
			}
			else
#endif
			{
				rtk_port_set_dev_port_mapping(wanPhyPort, realIfname);
			}

#ifdef CONFIG_RTL_SMUX_DEV
			va_cmd(SMUXCTL, 11, 1, "--if", realIfname, "--set-if-rsmux", "--set-if-rx-policy", "DROP",
				"--set-if-tx-policy", "CONTINUE", "--set-if-rxmc-policy", "DROP", "--set-if-rx-multi", "1");
#endif
		}
	}

#else
	wanPhyPort = rtk_port_get_wan_phyID();

	if(wanPhyPort!=-1)
	{
		rtk_port_set_dev_port_mapping(wanPhyPort, ALIASNAME_NAS0);
		wanPhyPortMask |= (1 << wanPhyPort);
	}
#endif

#if !defined(CONFIG_LUNA_G3_SERIES)
#if defined(CONFIG_USER_CMD_SERVER_SIDE)
/*	this is not final version,
 *	- change PTM root device to nas0 for G.fast
 *	- reserve last LAN port for remote phy communication
 *		=> 9607CP original LAN port 0~3, so reserve port 3
 *		=> should change user config CONFIG_LAN_PORT_NUM to 3
 *	- change ethernet wan to another one. */
	rtk_port_set_dev_port_mapping(SWITCH_WAN_PORT, ALIASNAME_PTM0);
	wanPhyPortMask |= (1 << SWITCH_WAN_PORT);
#endif
#endif

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	if (wanPhyPortMask!=0) {
		sprintf(sysbuf, "echo 0x%x > /proc/fc/ctrl/wan_port_mask", wanPhyPortMask);		// change fc wan port mask
		va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
	}
#endif
	return 0;
}

#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
int setWanPortPowerState(void)
{
	int vcTotal, i;
	MIB_CE_ATM_VC_T Entry;

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				return -1;

		if (Entry.enable == 0)
			continue;

		setup_wan_port_power_on_off(rtk_port_get_phyPort(Entry.logic_port), Entry.power_on);
	}

	return 0;
}
#endif

//--------------------------------------------------------
// WAN startup
// return value:
// 1  : successful
// -1 : failed
// configAll = CONFIGALL,  pEntry = NULL  : start all WAN connections(include VC, ETHWAN, PTMWAN, VPN, 3g).
// configAll = CONFIGONE, pEntry != NULL : start specified VC, ETHWAN, PTMWAN connection and VPN, 3g connections.
// configAll = CONFIGONE, pEntry = NULL  : start VPN, 3g connections.
int startWan(int configAll, MIB_CE_ATM_VC_Tp pEntry, int isBoot)
{
	char action_type_str[128] = {0};
	int vcTotal, i, j, ret;
	MIB_CE_ATM_VC_T Entry;
	int ipEnabled;
	FILE *fp;
	char vcNum[16];
	char ifname[IFNAMSIZ];
	char tmpBuf[128] = {0};
	char vChar=-1;
#ifdef CONFIG_DEV_xDSL
	Modem_LinkSpeed vLs;
	vLs.upstreamRate=0;
#endif
	// Mason Yu
	unsigned char *rootdev=NULL;
	unsigned char devAddr[MAC_ADDR_LEN];
	char macaddr[MAC_ADDR_LEN*2+1];
	unsigned int mtu;
	char mtuStr[10];
	ret = 1;
	unsigned int mark, mask;
	char ifName[IFNAMSIZ];
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	unsigned int *wanApplyIfindexs=NULL;
	int wanApplyCount=0;
#ifdef CONFIG_USER_LANNETINFO
	int lannetinfo_pid = 0;
#endif

#ifdef CONFIG_00R0
	int hasVoIPWan = 0;
#endif
#if defined(CONFIG_USER_RTK_MULTI_BRIDGE_WAN)
	cleanAllFirewallRule();
#endif

	ipEnabled = 0;
	AUG_PRT("%s %s\n", __func__, config_WAN_action_type_to_str(configAll, pEntry, 1, action_type_str));
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	syslog(LOG_CRIT, "%s", config_WAN_action_type_to_str(configAll, pEntry, 1, action_type_str));
#else
	syslog(LOG_INFO, "%s", config_WAN_action_type_to_str(configAll, pEntry, 1, action_type_str));
#endif

#if defined(CONFIG_CTC_SDN)
	if(configAll != CONFIGALL &&
		(pEntry && pEntry->enable && pEntry->sdn_enable) &&
		check_ovs_enable() && check_ovs_running() == 0)
	{
		if(bind_ovs_interface_by_mibentry(OFP_PTYPE_WAN, pEntry, -1, -1) == 0)
		{
			printf("=====> Do bind WAN(ifIndex=%x) to SDN <=====\n", pEntry->ifIndex);
			return 0;
		}
	}
#endif

#if defined(CONFIG_ARCH_RTL8198F) && !defined(CONFIG_RTL_MULTI_ETH_WAN)
	/* when enable multiple wan, per-wan can configure as bridge, donot set this proc file.
	 *	this proc file will set 8367r lan wan in the same vlan group.
	*/
	{
	char	buf[64]={0};

	if (pEntry == NULL) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, 0, (void *)&Entry))
			Entry.cmode = 1; // IPoE
	}
	else {
		Entry.cmode = pEntry->cmode;
	}
	sprintf(buf,"echo ChannelMode %d > /proc/driver/realtek/flash_mib", Entry.cmode);
	system(buf);
	}
#endif

	// Mason Yu.
	//If it is a router modem , we should config forwarding first.
	// Because the /proc/.../conf/all/forwarding value, will affect /proc/.../conf/vc0/forwarding value.
	//IulianWu. move the proc operation to /etc/init.d/rcX
	//setup_ipforwarding(1);

#if defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN)

//#ifdef NEW_PORTMAPPING
	// Get port-mapping bitmapped LAN ports (fgroup)
	//get_pmap_fgroup(pmap_list, MAX_VC_NUM);
//#endif

#if 0 //defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_ENABLE)
	if(Init_RG_ELan(UntagCPort, RoutingWan)!=SUCCESS){
		printf("Init_RG_ELan failed!!! \n");
		return -1;
	}
#endif

	mib_get_s(MIB_ELAN_MAC_ADDR, (void *)devAddr, sizeof(devAddr));
	snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
			devAddr[0], devAddr[1], devAddr[2],
			devAddr[3], devAddr[4], devAddr[5]);
#if defined(CONFIG_PTMWAN)
	rootdev=ALIASNAME_PTM0;
	va_cmd(IFCONFIG, 4, 1, rootdev, "hw", "ether", macaddr);
	va_cmd(IFCONFIG, 2, 1, rootdev, "up");
#endif

#if defined(CONFIG_RTL_MULTI_ETH_WAN) && !defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
	rootdev=ALIASNAME_NAS0;
	mtu = rtk_wan_get_max_wan_mtu();
	snprintf(mtuStr, sizeof(mtuStr), "%u", mtu);
#ifndef CONFIG_ARCH_RTL8198F
	va_cmd(IFCONFIG, 4, 1, rootdev, "hw", "ether", macaddr);
#endif
	va_cmd(IFCONFIG, 2, 1, rootdev, "up");
	va_cmd(IFCONFIG, 3, 1, rootdev, "mtu", mtuStr);
#endif

#if defined(CONFIG_RTL9607C_SERIES) && defined(CONFIG_ETHTOOL) //if mtu > 1700, use ethtool to disable wan gso
	const char DEV_ETHTOOL[] = "/bin/ethtool";
	if(mtu > 1700)
		va_cmd(DEV_ETHTOOL, 4, 1, "-k", rootdev, "gso", "off");
	else
		va_cmd(DEV_ETHTOOL, 4, 1, "-k", rootdev, "gso", "on");
#endif

#ifdef CONFIG_USER_PPPD
	write_to_ppp_script();
#endif //CONFIG_USER_PPPD

#endif	//defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN)

	// If we set forwarding=1 for all, then the every interafce's forwarding will be modified to 1.
	// So we must set forwarding=1 for all first, then set every interafce's forwarding.
	// Mason Yu.
	//IulianWu. move the proc operation to /etc/init.d/rcX
	//setup_ipforwarding(1);
#ifdef CONFIG_USER_PPPOMODEM
	// wan3g run as routed mode
	if((configAll == CONFIGALL) || (pEntry && (TO_IFINDEX(MEDIA_3G,  MODEM_PPPIDX_FROM, 0) == pEntry->ifIndex)))
	{
		ipEnabled = wan3g_start();
	}
	else
		ipEnabled = wan3g_enable();
#endif //CONFIG_USER_PPPOMODEM

	if (configAll == CONFIGCWMP)
	{
		printf("[%s@%d] CONFIGCWMP start\n", __FUNCTION__, __LINE__);
		FILE *f_start = NULL;
		f_start = fopen(CWMP_START_WAN_LIST, "r");
		int ifIdx = 0;
		if (f_start == NULL)
		{
			printf("[%s@%d] Open list file fail\n", __FUNCTION__, __LINE__);
			return -1;
		}
		wanApplyIfindexs=calloc(vcTotal, sizeof(unsigned int));
		j=0;
		while (wanApplyIfindexs && fscanf(f_start, "%d\n", &ifIdx) != EOF)
		{
			wanApplyIfindexs[j++] = ifIdx;
		}
		wanApplyCount = j;
		fclose(f_start);
	}

	if(!(configAll == CONFIGONE && pEntry == NULL))
	{
	for (i = 0; i < vcTotal; i++)
	{
		/* get the specified chain record */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;

		if(!isInterfaceMatch(Entry.ifIndex))  // Magician: Only raise interfaces that was set by wan mode.
			continue;

		if (Entry.enable == 0)
			continue;

		if (configAll == CONFIGONE) {
			if(!pEntry || Entry.ifIndex != pEntry->ifIndex)
				continue;
		} else if (configAll == CONFIGCWMP) {
			for(j=0; j<wanApplyCount; j++) {
				if (Entry.ifIndex == wanApplyIfindexs[j])
					break;
			}
			if(j >= wanApplyCount)
				continue;
		}

#if defined(CONFIG_CTC_SDN)
		if((Entry.enable && Entry.sdn_enable) &&
			check_ovs_enable() && check_ovs_running() == 0)
		{
			if(check_ovs_interface_by_mibentry(OFP_PTYPE_WAN, &Entry, i, -1) == 0)
			{
				continue;
			}
		}
#endif
		ifGetName(Entry.ifIndex, ifName, sizeof(ifName));
		rtk_net_add_wan_policy_route_mark(ifName, &mark, &mask);

#if defined(CONFIG_RTL_MULTI_ETH_WAN) && !defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
		addEthWANdev(&Entry);
#endif

#if defined(CONFIG_GPON_FEATURE) && defined (CONFIG_TR142_MODULE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		rtk_tr142_sync_wan_info(&Entry, 1);
#endif

#if defined(CONFIG_USER_MULTI_PHY_ETH_WAN_RATE)
		setup_wan_phy_rate(rtk_port_get_phyPort(Entry.logic_port), Entry.phy_rate);
#endif
#ifdef CONFIG_USER_LANNETINFO
		lannetinfo_pid = read_pid(LANNETINFO_RUNFILE);
		if (lannetinfo_pid > 0)
			kill(lannetinfo_pid, SIGUSR1);
#endif

#ifdef CONFIG_IPV6
#ifndef CONFIG_USER_LAN_IPV6_SERVER_USE_LATEST_PD
		setup_delegated_default_wanconn(&Entry);
#endif
#endif
		ret |= startConnection(&Entry, i);

#if defined(CONFIG_ATM_CLIP)&&defined(CONFIG_DEV_xDSL)
#ifdef CONFIG_DEV_xDSL
		if (adsl_drv_get(RLCM_GET_LINK_SPEED, (void *)&vLs, RLCM_GET_LINK_SPEED_SIZE) && vLs.upstreamRate != 0) {
#endif
			if (Entry.cmode == CHANNEL_MODE_RT1577) {
				struct data_to_pass_st msg;
				char wanif[16];
				unsigned long addr = *((unsigned long *)Entry.remoteIpAddr);
				ifGetName(Entry.ifIndex, wanif, sizeof(wanif));
				snprintf(msg.data, BUF_SIZE, "mpoactl set %s inarprep %lu", wanif, addr);
				write_to_mpoad(&msg);
			}
#ifdef CONFIG_DEV_xDSL
		}
#endif
#endif

		if ((CHANNEL_MODE_T)Entry.cmode != CHANNEL_MODE_BRIDGE)
			ipEnabled = 1;

#ifdef CONFIG_RTK_DEV_AP
#ifdef CONFIG_BRIDGE_IGMP_SNOOPING
		if ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_BRIDGE){
			ifGetName(Entry.ifIndex,ifname,sizeof(ifname));
			snprintf(tmpBuf,sizeof(tmpBuf),"echo 2 > /sys/devices/virtual/net/%s/brif/%s/multicast_router",BRIF,ifname);
			system(tmpBuf);

			snprintf(tmpBuf,sizeof(tmpBuf),"echo 1 > /sys/devices/virtual/net/%s/brif/%s/multicast_fast_leave",BRIF,ifname);
			system(tmpBuf);
		}
#endif
#endif
		if (configAll == CONFIGONE)
			break;
	}
	}
	// execute firewall rules
	if (setupFirewall(isBoot) == -1)
	{
		printf("execute firewall rules failed !\n");
		//ret = -1;
	}
#ifdef YUEME_4_0_SPEC
	if(isBoot)
	{
#if 0 //use AppCtrlNF instead of dnsmonitor //def CONFIG_USER_DNS_MONITOR
		va_niced_cmd( "/bin/dnsmonitor", 0, 0 );
#endif
	}
#endif

	ret = startWanServiceDependency(ipEnabled, isBoot);

#if defined(CONFIG_BLOCK_BRIDGE_WAN_IGMP_TO_IGMPPROXY)
	// drop igmp from bridge wan when br0->multicast_router is 2.
	setup_block_igmp_from_bridge_wan_to_igmpproxy_rules();
#endif

#ifdef CONFIG_USER_RTK_MULTICAST_VID
	rtk_iptv_multicast_vlan_config(configAll, pEntry);
#endif

#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rtk_igmp_mld_snooping_module_init(BRIF, configAll, pEntry);
#endif

#if ! defined(_LINUX_2_6_) && defined(CONFIG_RTL_MULTI_WAN)
	checkWanModeStatus();
#endif

	if(wanApplyIfindexs){ free(wanApplyIfindexs); }

	return ret;
}

void setup_NAT(MIB_CE_ATM_VC_Tp pEntry, int set)
{
	char wanif[IFNAMSIZ];
	const char *action = FW_ADD;

	if(pEntry == NULL) return;
	ifGetName(pEntry->ifIndex,wanif,sizeof(wanif));

	if(!set)
		action = FW_DEL;
	else {
		va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", NAT_ADDRESS_MAP_TBL);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 4, 1, "-t", "nat", "-N", NAT_ADDRESS_MAP_TBL);
#endif
	}

	if (((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_IPOE)  ||
	  ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1483) ||
	  ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1577) ||
	  ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_6RD) )
	{
#ifdef CONFIG_IPV6
		if (pEntry->IpProtocol & IPVER_IPV4) {
#endif
			//Don't ues SNAT, because if the routing change the nf conntrack session cannot update
			//the NAT MASQUERADE will check the source ip, and update nf conntrack
			/*if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED)
			{
				if (pEntry->napt == 1)
				{
					// Setup one NAT Rule for the specfic interface
					startAddressMap(pEntry);
				}
			}
			else*/
			{
				if (pEntry->napt == 1)
				{	// Enable NAPT
					#ifdef CONFIG_SNAT_SRC_FILTER
					va_cmd(IPTABLES, 10, 1, "-t", "nat", action, NAT_ADDRESS_MAP_TBL, "-s", get_lan_ipnet(),
						ARG_O, wanif, "-j", "MASQUERADE");
					#else
					va_cmd(IPTABLES, 8, 1, "-t", "nat", action, NAT_ADDRESS_MAP_TBL,
						ARG_O, wanif, "-j", "MASQUERADE");
					#endif
				}
			}
#ifdef CONFIG_IPV6
		}

		if (pEntry->IpProtocol & IPVER_IPV6)
		{
#if defined(CONFIG_IP6_NF_TARGET_MASQUERADE) && defined (CONFIG_IP6_NF_NAT)
			if (pEntry->napt_v6 == 1)
			{
				// Enable NAT66
				va_cmd(IP6TABLES, 8, 1, "-t", "nat", action, NAT_ADDRESS_MAP_TBL, ARG_O, wanif, "-j", "MASQUERADE");
			}
#endif
		}
#endif
	}
	else if (((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE) ||
	      ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOA))
	{
#ifdef CONFIG_IPV6
		if (pEntry->IpProtocol & IPVER_IPV4) {
#endif
			if (pEntry->napt == 1)
			{	// Enable NAPT
				#ifdef CONFIG_SNAT_SRC_FILTER
				va_cmd(IPTABLES, 10, 1, "-t", "nat", action, NAT_ADDRESS_MAP_TBL, "-s", get_lan_ipnet(),
					"-o", wanif, "-j", "MASQUERADE");
				#else
				va_cmd(IPTABLES, 8, 1, "-t", "nat", action, NAT_ADDRESS_MAP_TBL,
					"-o", wanif, "-j", "MASQUERADE");
				#endif
			}
#ifdef CONFIG_IPV6
		}

		if (pEntry->IpProtocol & IPVER_IPV6)
		{
#if defined(CONFIG_IP6_NF_TARGET_MASQUERADE) && defined (CONFIG_IP6_NF_NAT)
			if (pEntry->napt_v6 == 1)
			{
				// Enable NAT66
				va_cmd(IP6TABLES, 8, 1, "-t", "nat", action, NAT_ADDRESS_MAP_TBL, ARG_O, wanif, "-j", "MASQUERADE");
			}
#endif
		}
#endif
	}
}

// configAll = CONFIGALL,  pEntry = NULL  : restart all WAN connections(include VC, ETHWAN, PTMWAN, VPN, 3g).
//									  It means that we add or modify all VC, ETHWAN, PTMWAN channel.
// configAll = CONFIGONE, pEntry != NULL : restart specified VC, ETHWAN, PTMWAN connection and VPN, 3g connections.
// 									  It means that we add or modify a VC, ETHWAN, PTMWAN channel.
// configAll = CONFIGONE, pEntry = NULL  : restart VPN, 3g connections. It means that we delete a VC, ETHWAN, PTMWAN channel.
void restartWAN(int configAll, MIB_CE_ATM_VC_Tp pEntry)
{
	int dhcrelay_pid, i;
	char vChar;
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	rtk_port_update_wan_phyMask();
	rtk_update_br_interface_list();
#endif
#ifdef CONFIG_IPV6
	unsigned char tmpBuf[64];
#endif
#if defined(CONFIG_YUEME)
	unsigned long long rx_packets, tx_packets, rx_bytes, tx_bytes;
	char ifname[IFNAMSIZ+1] = {0};
#endif

#if defined(CONFIG_CTC_SDN)
	if(configAll == CONFIGONE &&
		(pEntry && pEntry->enable && pEntry->sdn_enable) &&
		check_ovs_enable() && check_ovs_running() == 0)
	{
		if(bind_ovs_interface_by_mibentry(OFP_PTYPE_WAN, pEntry, -1, -1) == 0)
		{
			printf("=====> Do bind WAN(ifIndex=%x) to SDN <=====\n", pEntry->ifIndex);
			return ;
		}
	}
#endif

#ifdef CONFIG_DEV_xDSL
	itfcfg("sar", 0);
#endif

#if !defined(CONFIG_LUNA)
#ifdef CONFIG_USER_CONNTRACK_TOOLS
	va_cmd(CONNTRACK_TOOL, 1, 1, "-F");
#else
	va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
#endif
#endif

#ifdef CONFIG_RTK_DEV_AP
			//printf("\n%s:%d clean nf_conntrack icmp entry!!!\n",__FUNCTION__,__LINE__);
			system("echo 1 > /proc/sys/net/netfilter/nf_conntrack_icmp_timeout");
#endif

#if defined(NEED_CHECK_DHCP_SERVER)||defined(NEED_CEHCK_WLAN_INTERFACE)
	int needcommit=0;

	//for telmex:if only one bridge wan,turn down wlan & dhcp server
	if(isOnlyOneBridgeWan()>0)
	{
		needcommit =checkSpecifiedFunc();
#ifdef COMMIT_IMMEDIATELY
		if(needcommit)
		{
			Commit();
		}
#endif
	}
#endif

#ifdef CONFIG_IPV6
	cleanLanV6Server();
#endif
	cleanAllFirewallRule();
	rtk_lan_change_mtu();

#if defined(CONFIG_00R0) && defined(CONFIG_USER_RTK_WAN_CTYPE)
{
	if(pEntry != NULL){
		if(pEntry->dgw == 1 && (pEntry->applicationtype & X_CT_SRV_INTERNET) && (pEntry->cmode > 0)){
			mib_set(MIB_DEFAULT_GW_IFIDX, (void *)&pEntry->ifIndex);
#ifdef COMMIT_IMMEDIATELY
			Commit();
#endif
		}
	}
#if 0
	int EntryID, ret;
	MIB_CE_ATM_VC_T tmp;
	ret = rtk_wan_check_Droute(configAll,pEntry,&EntryID);

	fprintf(stderr, "[%s@%d] rtk_wan_check_Droute ret = %d \n", __FUNCTION__, __LINE__, ret);
	if(ret == 3 && configAll != CONFIGCWMP){
	/*remove, modify D route case!!!!*/
//AUG_PRT("%s-%d EntryID=%d\n",__func__,__LINE__,EntryID);
		if (!mib_chain_get(MIB_ATM_VC_TBL, EntryID, (void*)&tmp))
			return;
//AUG_PRT("%s-%d tmp.enable=%d\n",__func__,__LINE__,tmp.enable);

		deleteConnection(CONFIGONE, &tmp);// Modify or remove D route
		startWan(CONFIGONE, &tmp, NOT_BOOT);
	}
#endif
}
#endif

#ifdef CONFIG_USER_BRIDGE_GROUPING
	setup_bridge_grouping(DEL_RULE);
#endif
#ifdef CONFIG_USER_VLAN_ON_LAN
	setup_VLANonLAN(DEL_RULE);
#endif
	if (configAll == CONFIGALL)
		startWan(CONFIGALL, NULL, NOT_BOOT);
	else if (configAll == CONFIGONE) {
		startWan(CONFIGONE, pEntry, NOT_BOOT);
#if defined(CONFIG_YUEME)
		if(pEntry) {
			ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));
			if(rtk_load_interface_packet_statistics_log_file(ifname, &rx_packets, &tx_packets, &rx_bytes, &tx_bytes) == 0)
			{
				rtk_set_interface_packet_statistics(ifname, rx_packets, tx_packets, rx_bytes, tx_bytes);
			}
		}
#endif
	}
	else if (configAll == CONFIGCWMP)
		startWan(CONFIGCWMP, NULL, NOT_BOOT);

	mib_get_s(MIB_MPMODE, (void *)&vChar, sizeof(vChar));

#ifdef NEW_PORTMAPPING
	setupnewEth2pvc();
#endif
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	rtk_del_manual_firewall();
	rtk_init_manual_firewall();
	set_manual_load_balance_rules();
#endif

// Mason Yu. SIGRTMIN for DHCP Relay.catch SIGRTMIN(090605)
#ifdef COMBINE_DHCPD_DHCRELAY
	dhcrelay_pid = read_pid("/var/run/udhcpd.pid");
#else
	dhcrelay_pid = read_pid("/var/run/dhcrelay.pid");
#endif
	if (dhcrelay_pid > 0) {
		printf("restartWAN1: kick dhcrelay(%d) to Re-discover all network interface\n", dhcrelay_pid);
		// Mason Yu. catch SIGRTMIN(It is a real time signal and number is 32) for re-sync all interface on DHCP Relay
		// Kaohj -- SIGRTMIN is not constant, so we use 32.
		//kill(dhcrelay_pid, SIGRTMIN);
		kill(dhcrelay_pid, 32);
	}
	// Mason Yu.
#ifdef CONFIG_IPV6
	if (mib_get(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)tmpBuf) != 0)
	{
		char cmdBuf[100]={0};

		if (strcmp(tmpBuf, "")) {  // It is static config LLA IP
		//fix two default IPv6 gateway in LAN, Alan
		delOrgLanLinklocalIPv6Address();
		sprintf(cmdBuf, "%s/%d", tmpBuf, 64);
		va_cmd(IFCONFIG, 3, 1, LANIF, ARG_ADD, cmdBuf);
		}
	}

	/* del/add/modify wan all need to do restartLanV6Server(default dhcpv6 server and all radvd). */
	restartLanV6Server();
	if (configAll == CONFIGALL || (pEntry && pEntry->IpProtocol & IPVER_IPV6))
	{
//#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
//#ifdef CONFIG_USER_WIDE_DHCPV6
		//modify/add WAN need to restart multi-dhcp6s service.
//		restart_multi_wide_Dhcp6sService(0);
//#endif
//#endif
	}


#endif
#if defined(CONFIG_CMCC)
	if(pEntry && pEntry->applicationtype & X_CT_SRV_INTERNET)
	{
		printf("=========internet wan change need restart rsniffer =======\n");
		system("killall rsniffer");
	}
#endif
#ifdef CONFIG_DEV_xDSL
	itfcfg("sar", 1);
#endif

#ifdef CONFIG_USER_XFRM
#ifdef CONFIG_USER_STRONGSWAN
	ipsec_strongswan_take_effect();
#else
	ipsec_take_effect();
#endif
#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
#ifdef CONFIG_USER_PPTPD_PPTPD
	pptpd_take_effect();
#endif
#ifdef CONFIG_USER_PPTP_SERVER
	pptp_server_take_effect();
#endif
#endif

#ifdef CONFIG_USER_L2TPD_LNS
	l2tp_lns_take_effect();
#endif
#ifdef YUEME_4_0_SPEC
	bridgeDataAcl_take_effect(1);
#endif
#ifdef CONFIG_USER_VLAN_ON_LAN
	setup_VLANonLAN(ADD_RULE);
#endif
#ifdef CONFIG_USER_BRIDGE_GROUPING
	setup_bridge_grouping(ADD_RULE);
#endif
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
	rtk_igmp_mld_snooping_module_init(BRIF, configAll, pEntry);
#endif

#ifdef CONFIG_RTK_DEV_AP
		//printf("\n%s:%d clean nf_conntrack icmp entry!!!\n",__FUNCTION__,__LINE__);
		system("echo 30 > /proc/sys/net/netfilter/nf_conntrack_icmp_timeout");
#endif
#ifdef CONFIG_HYBRID_MODE
	rtk_pon_change_onu_devmode();
#endif
}

/*
 *	Deal with configuration dependency when a WAN channel has been deleted.
 */
void resolveServiceDependency(unsigned int idx)
{
	MIB_CE_ATM_VC_T Entry;
#ifdef CONFIG_USER_IGMPPROXY
	unsigned int igmp_proxy_itf;
	unsigned char igmp_enable;
#endif
#if defined (CONFIG_IPV6) && defined(CONFIG_USER_MLDPROXY)
	unsigned int mld_proxy_itf;
	unsigned char mld_proxy_enable;
#endif
#ifdef IP_PASSTHROUGH
	unsigned int ippt_itf;
	unsigned int ippt_lease;
#endif

#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_AWIFI_SUPPORT)
	char rtk_aWiFiEnabled;
#endif

	struct data_to_pass_st msg;
	int k;
#if defined(CONFIG_E8B) && defined(TIME_ZONE)
	unsigned int if_wan = DUMMY_IFINDEX;
#endif

	/* get the specified chain record */
	if (!mib_chain_get(MIB_ATM_VC_TBL, idx, (void *)&Entry))
	{
		return;
	}

#ifdef CONFIG_USER_IGMPPROXY
	// resolve IGMP proxy dependency
	if(!mib_get_s( MIB_IGMP_PROXY_ITF,  (void *)&igmp_proxy_itf, sizeof(igmp_proxy_itf)))
		return;
	if (Entry.ifIndex == igmp_proxy_itf)
	{ // This interface is IGMP proxy interface
		igmp_proxy_itf = DUMMY_IFINDEX;	// set to default
		mib_set(MIB_IGMP_PROXY_ITF, (void *)&igmp_proxy_itf);
		igmp_enable = 0;	// disable IGMP proxy
		mib_set(MIB_IGMP_PROXY, (void *)&igmp_enable);
	}
#endif

#if defined (CONFIG_IPV6) && defined(CONFIG_USER_MLDPROXY)
	if(!mib_get_s(MIB_MLD_PROXY_EXT_ITF,	(void *)&mld_proxy_itf, sizeof(mld_proxy_itf)))
		return;
	if (Entry.ifIndex == mld_proxy_itf)
	{ // This interface is MLD proxy interface
		mld_proxy_itf = DUMMY_IFINDEX; // set to default
		mib_set(MIB_MLD_PROXY_EXT_ITF, (void *)&mld_proxy_itf);
		//mld_proxy_enable = 0; // disable MLD proxy
		//mib_set(MIB_MLD_PROXY_DAEMON, (void *)&mld_proxy_enable);
	}
#endif

#ifdef IP_PASSTHROUGH
	// resolve IP passthrough dependency
	if(!mib_get_s( MIB_IPPT_ITF,  (void *)&ippt_itf, sizeof(ippt_itf)))
		return;
	if (Entry.ifIndex == ippt_itf)
	{ // This interface is IP passthrough interface
		ippt_itf = DUMMY_IFINDEX;	// set to default
		mib_set(MIB_IPPT_ITF, (void *)&ippt_itf);
		ippt_lease = 600;	// default to 10 min.
		mib_set(MIB_IPPT_LEASE, (void *)&ippt_lease);
	}
#endif

#if defined(CONFIG_USER_MINIUPNPD)
	restart_upnp(CONFIGONE, 0, Entry.ifIndex, 1);
#endif


#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_AWIFI_SUPPORT)
	if(rtk_wan_aWiFi_stop(Entry.ifIndex) > 0){
		rtk_aWiFiEnabled= 0;
		mib_set(MIB_RTK_AWIFI_ENABLE, (void *)&rtk_aWiFiEnabled);
	}
#endif

#ifdef PORT_FORWARD_GENERAL
	delPortForwarding( Entry.ifIndex );
#endif
#ifdef ROUTING
	delRoutingTable( Entry.ifIndex );
#endif
#ifdef CONFIG_USER_ROUTED_ROUTED
	delRipTable(Entry.ifIndex);
#endif
#if defined(CONFIG_E8B) && defined(TIME_ZONE)
	mib_get_s(MIB_NTP_IF_WAN, &if_wan, sizeof(if_wan));
	if (if_wan == Entry.ifIndex) {
		stopNTP();
		if_wan = DUMMY_IFINDEX;
		mib_set(MIB_NTP_IF_WAN, &if_wan);
	}
#endif
	delPPPoESession(Entry.ifIndex);
#if defined(CONFIG_CU_BASEON_YUEME)
	delVirtualServerRule(Entry.ifIndex);
#endif
}

#ifdef CONFIG_PPPOE_MONITOR
#define MONITOR_RUNFILE	"/var/run/ppp-monitor.pid"

int sendMessageToPppoeMonitor(PPPMONITOR_MSG_T *msg)
{
	struct pppMonitorMsg qbuf;
	key_t key;
	int  ret, pid, qid, cpid, ctgid;

	key = ftok("/bin/spppd", 's');
	if ((qid = open_queue(key, MQ_GET)) == -1) {
		perror("sendMessageToPppoeMonitor open queue fail!\n");
		return -2;
	}

	pid = read_pid(MONITOR_RUNFILE);
	if( (pid == -1)||(0 != kill(pid, 0)) )
	{
		printf("[%s %d]: There is no PPPoE Monitor Process!\n", __func__, __LINE__);
		return -3;
	}

	cpid = (int)syscall(SYS_gettid);
	ctgid = (int)getpid();

	/* Send a message to the queue */
	qbuf.mtype = pid;
	qbuf.request = cpid;
	qbuf.tgid = ctgid;
	qbuf.msg.cmd = msg->cmd;
	qbuf.msg.arg1 = msg->arg1;
	qbuf.msg.arg2 = msg->arg2;
	memcpy(qbuf.msg.mtext, msg->mtext, MAX_MONITOR_MSG_SIZE);

	ret = msgsnd(qid, (void *)&qbuf, sizeof(struct pppMonitorMsg)-sizeof(long), 0);
	if( ret== -1)
	{
		printf("[%s %d]:send request to pppoe monitor process error.\n", __func__, __LINE__);
		return ret;
	}
	return 0;
}

/*
	Return Value:
	Return -1 on failure;
	otherwise return the number of bytes actually copied into the mtext array.
*/
int readMessageFromPppoeMonitor(struct pppMonitorMsg *qbuf)
{
	int ret, qid, cpid, retry_count=20;
	key_t key;

	key = ftok("/bin/spppd", 's');
	qid = open_queue(key, MQ_GET);
	if(qid <= 0){
		printf("[%s %d]: get pppoe monitor msgqueue error!\n", __func__, __LINE__);
		return -1;
	}

	cpid = (int)syscall(SYS_gettid);
	qbuf->mtype = cpid;

rcv_retry:
	ret=msgrcv(qid, (void *)qbuf, sizeof(struct pppMonitorMsg)-sizeof(long), cpid, 0);
	if (ret == -1) {
		switch (errno) {
				case EINTR	 :
					printf("====>>%s %d:EINTR	 \n", __func__, __LINE__);
					goto rcv_retry;
				case EACCES :
					printf("====>>%s %d:EACCES	\n", __func__, __LINE__);
					break;
				case EFAULT   :
					printf("====>>%s %d:EFAULT	 \n", __func__, __LINE__);
					break;
				case EIDRM	:
					printf("====>>%s %d:EIDRM	 \n", __func__, __LINE__);
					break;
				case EINVAL   :
					printf("====>>%s %d:EINVAL	 \n", __func__, __LINE__);
					break;
				case ENOMSG   :
					printf("====>>%s %d:ENOMSG	 \n", __func__, __LINE__);
					break;
				case E2BIG   :
					printf("====>>%s %d:E2BIG	 \n", __func__, __LINE__);
					break;
				default:
					printf("====>>%s %d:Unknown	 \n", __func__, __LINE__);
			}
	}

	return ret;
}

int add_pppoe_monitor_wan_interface(const char *ifname)
{
	struct pppMonitorMsg qbuf;
	PPPMONITOR_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_ADD_MONITOR_WAN;
	strcpy(mymsg->mtext, ifname);

	ret = sendMessageToPppoeMonitor(&qbuf.msg);
	if(ret != 0)
		return ret;

	memset(&qbuf, 0, sizeof(struct pppMonitorMsg));
	ret = readMessageFromPppoeMonitor(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == PPPMONITOR_MSG_SUCC) {
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}

int del_pppoe_monitor_wan_interface(const char *ifname)
{
	struct pppMonitorMsg qbuf;
	PPPMONITOR_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_DEL_MONITOR_WAN;
	strcpy(mymsg->mtext, ifname);

	ret = sendMessageToPppoeMonitor(&qbuf.msg);
	if(ret != 0)
		return ret;

	memset(&qbuf, 0, sizeof(struct pppMonitorMsg));
	ret = readMessageFromPppoeMonitor(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == PPPMONITOR_MSG_SUCC) {
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}

int kick_pppoe_monitor_client(const char *ifname, const char *username)
{
	struct pppMonitorMsg qbuf;
	PPPMONITOR_MSG_T *mymsg;
	PPPACCOUNT_INFO_St *account;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_KICK_MONITOR_WAN;
	mymsg->arg1 = 60;// block client for 60s
	memset(mymsg->mtext, 0, MAX_MONITOR_MSG_SIZE);
	account = (PPPACCOUNT_INFO_St *)mymsg->mtext;
	strcpy(account->wanIfName, ifname);
	strcpy(account->pppAccount, username);

	ret = sendMessageToPppoeMonitor(&qbuf.msg);
	if(ret != 0)
		return ret;

	memset(&qbuf, 0, sizeof(struct pppMonitorMsg));
	ret = readMessageFromPppoeMonitor(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == PPPMONITOR_MSG_SUCC) {
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}

int get_br_pppoe_session_info(PPPoESessionInfo_St *pppoeInfo, int num)
{
	struct pppMonitorMsg qbuf;
	PPPMONITOR_MSG_T *mymsg;
	int  ret, size;

	if(pppoeInfo == NULL)
		return -1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_PPPOE_SESSION_INFO_GET;

	ret = sendMessageToPppoeMonitor(&qbuf.msg);
	if(ret != 0)
		return ret;

	memset(&qbuf, 0, sizeof(struct pppMonitorMsg));
	ret = readMessageFromPppoeMonitor(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == PPPMONITOR_MSG_SUCC) {
			size = (sizeof(PPPoESessionInfo_St)*num >= MAX_MONITOR_MSG_SIZE)? MAX_MONITOR_MSG_SIZE:(sizeof(PPPoESessionInfo_St)*num);
			memcpy(pppoeInfo, mymsg->mtext, size);
			return 0;
		}
	}

	printf("[%s %d]: Get br pppoe session info failed:ret=%d!\n", __func__, __LINE__, ret);
	return -4;
}

int cfg_pppoe_monitor_packet(struct PPPoEMonitorPacket *packet_feature, int act)
{
	struct pppMonitorMsg qbuf;
	PPPMONITOR_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	if((act==CMD_ADD_PPPOE_MONITOR_PACKET) || (act==CMD_DEL_PPPOE_MONITOR_PACKET))
	{
		mymsg->cmd = act;
		memcpy(mymsg->mtext, packet_feature, sizeof(struct PPPoEMonitorPacket));
	}
	else if(act==CMD_FLUSH_PPPOE_MONITOR_PACKET)
		mymsg->cmd = CMD_FLUSH_PPPOE_MONITOR_PACKET;
	else
	{
		printf("[%s %d]: failed! unknow act:%d\n", __func__, __LINE__, act);
		return -1;
	}

	ret = sendMessageToPppoeMonitor(&qbuf.msg);
	if(ret != 0)
		return ret;

	memset(&qbuf, 0, sizeof(struct pppMonitorMsg));
	ret = readMessageFromPppoeMonitor(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == PPPMONITOR_MSG_SUCC) {
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}

	printf("[%s %d]: failed(act=%d)!\n", __func__, __LINE__, act);
	return -4;
}
#endif

#if defined(CONFIG_USER_RTK_VLAN_PASSTHROUGH)
void init_vlan_passthrough(void)
{
	unsigned char vlanpass_enable = 0;
	char vlanpass_cmd[80];
	if (!mib_set(MIB_VLANPASS_ENABLED, (void *)&vlanpass_enable)) {
		printf("set Vlan Pass enable MIB failed!");
	}

	sprintf(vlanpass_cmd, "echo vlan_passthrough %d > /proc/driver/realtek/rtk_vlan_passthrough", vlanpass_enable);
	va_cmd("/bin/sh" , 2, 1, "-c", vlanpass_cmd);

	return;
}

void set_vlan_passthrough(unsigned int enable)
{
	unsigned char vlanpass_enable = 0;
	char vlanpass_cmd[80];
	if(enable){
		vlanpass_enable = 1;
	}
	else{
		vlanpass_enable = 0;
	}
	if (!mib_set(MIB_VLANPASS_ENABLED, (void *)&vlanpass_enable)) {
		printf("set Vlan Pass enable MIB failed!");
	}

	sprintf(vlanpass_cmd, "echo vlan_passthrough %d > /proc/driver/realtek/rtk_vlan_passthrough", vlanpass_enable);
	va_cmd("/bin/sh" , 2, 1, "-c", vlanpass_cmd);

	return;
}

void setup_vlan_passthrough(void)
{
	unsigned char vlanpass_enable = 0;
	char vlanpass_cmd[80];
	if (!mib_get( MIB_VLANPASS_ENABLED,  (void *)&vlanpass_enable)){
		printf("get vlan pass enable failed!\n");
	}

	sprintf(vlanpass_cmd, "echo vlan_passthrough %d > /proc/driver/realtek/rtk_vlan_passthrough", vlanpass_enable);
	va_cmd("/bin/sh" , 2, 1, "-c", vlanpass_cmd);

	return;
}
#endif

#ifdef CONFIG_USER_RTK_FASTPASSNF_MODULE
int rtk_switch_fastpass_nf_modules(int switch_flag)
{
	if (switch_flag)
	{
#ifdef CONFIG_CMCC
		va_cmd("/bin/insmod", 2, 1, "/lib/modules/FastPassNF.ko", "force_skip=1");
#else
		va_cmd("/bin/insmod", 1, 1, "/lib/modules/FastPassNF.ko");
#endif
		return 1;
	}
	else
	{
		va_cmd("/bin/rmmod", 1, 1, "/lib/modules/FastPassNF.ko");
		return 0;
	}
	return 0;
}
#endif
#ifdef CONFIG_USER_RTK_FAST_BR_PASSNF_MODULE
int rtk_switch_fastpass_br_modules(int switch_flag)
{
	if (switch_flag)
	{
		va_cmd("/bin/insmod", 1, 1, "/lib/modules/FastBrPassNF.ko");
		return 1;
	}
	else
	{
		va_cmd("/bin/rmmod", 1, 1, "/lib/modules/FastBrPassNF.ko");
		return 0;
	}
	return 0;
}
#endif


#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
int rtk_set_fc_skbmark_fwdByPs(int on)
{
	char buf[128] = {0};

#ifdef CONFIG_RTK_SKB_MARK2
	if (on == 1)
		snprintf(buf, sizeof(buf), "echo mark2 %d > /proc/fc/ctrl/skbmark_fwdByPs", SOCK_MARK2_FORWARD_BY_PS_START);
	else if (!on)
		snprintf(buf, sizeof(buf), "echo mark2 -1 > /proc/fc/ctrl/skbmark_fwdByPs");

	if (buf[0] != '\0')
	{
		printf("%s\n", buf);
		system(buf);
		return 1;
	}
#endif

	return 0;
}
#endif

#ifdef CONFIG_USER_AVALANCH_DETECT
/* rtk_switch_avalanche_process -
 * When testing Avalanche, some processes should STOP.
 * After finishing test Avalanche, process should restore.
 */
int rtk_switch_avalanche_process(int stop)
{
	char tmp_cmd[256] = {0};
	if (stop == 1)
	{
		va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGSTOP monitord");
		va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGKILL lanNetInfo");
		va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGKILL p0f");
		va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGKILL java");
		va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGKILL dm");
		//va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGKILL omci_app");
		//snprintf(tmp_cmd, sizeof(tmp_cmd), "ifconfig wlan0 down");
		//system(tmp_cmd);
		//snprintf(tmp_cmd, sizeof(tmp_cmd), "ifconfig wlan1 down");
		//system(tmp_cmd);

#if 0

#ifdef CONFIG_USER_MLDPROXY
		va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGSTOP mldproxy");
#endif
		va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGSTOP omd_main");
		va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGSTOP proxyDaemon"); //STOP reg_server should STOP proxyDaemon first
		va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGSTOP reg_server");
#endif
	}
	else
	{
		va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGCONT monitord");
		va_cmd("/bin/sh", 2, 0, "-c", "/bin/dm 0&");
		//snprintf(tmp_cmd, sizeof(tmp_cmd), "ifconfig wlan0 up");
		//system(tmp_cmd);
		//snprintf(tmp_cmd, sizeof(tmp_cmd), "ifconfig wlan1 up");
		//system(tmp_cmd);
#if 0
		va_cmd("/bin/sh", 2, 1, "-c", "lanNetInfo &"); //monitord will restart lanNetInfo if lanNetInfo is not exist.
#ifdef CONFIG_USER_MLDPROXY
		va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGCONT mldproxy");
#endif
		va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGCONT omd_main");
		va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGCONT reg_server");
		va_cmd("/bin/sh", 2, 1, "-c", "killall -SIGCONT proxyDaemon");
#endif
	}

	return 1;
}
#endif

#if defined(CONFIG_SECONDARY_IP)
void rtk_fw_set_lan_ip2_rule(void)
{
	char enable;
	struct in_addr lan_ip2;
	char str_ip[16]={0};

	mib_get_s(MIB_ADSL_LAN_ENABLE_IP2, (void *)&enable, sizeof(enable));
	if(enable)
	{
		mib_get_s(MIB_ADSL_LAN_IP2, (void *)&lan_ip2, sizeof(lan_ip2));

		if(lan_ip2.s_addr>0)
		{
			snprintf(str_ip, sizeof(str_ip), "%s", inet_ntoa(lan_ip2));

			va_cmd(EBTABLES, 12, 1, "-t", "nat", (char *)FW_DEL, FW_PREROUTING, "-p", "IPv4", "--ip-dst", str_ip, "-j", "redirect", "--redirect-target", FW_ACCEPT);

			va_cmd(EBTABLES, 12, 1, "-t", "nat", (char *)FW_ADD, FW_PREROUTING, "-p", "IPv4", "--ip-dst", str_ip, "-j", "redirect", "--redirect-target", FW_ACCEPT);
		}
	}
}
#endif

#if defined (CONFIG_IPV6) && defined(CONFIG_SECONDARY_IPV6)
void rtk_fw_set_lan_ipv6_2_rule(void)
{
	char str_ipv6[MAX_V6_IP_LEN]={0};

	mib_get_s(MIB_IPV6_LAN_IP_ADDR2, (void *)&str_ipv6, sizeof(str_ipv6));
	if(strlen(str_ipv6)!=0)
	{
		va_cmd(EBTABLES, 12, 1, "-t", "nat", (char *)FW_DEL, FW_PREROUTING, "-p", "IPv6", "--ip6-dst", str_ipv6, "-j", "redirect", "--redirect-target", FW_ACCEPT);
		va_cmd(EBTABLES, 12, 1, "-t", "nat", (char *)FW_ADD, FW_PREROUTING, "-p", "IPv6", "--ip6-dst", str_ipv6, "-j", "redirect", "--redirect-target", FW_ACCEPT);
	}
}
#endif

/*
	This function used to create a unix domain socket.

	Arguments:
	server: 1 (socket used by server), 0 (socket used by client)
	path: the unix domain socket file path
	block: 1 (set socket BLOCK), 0 (set socket UNBLOCK)
*/
int rtk_create_unix_domain_socket(int server, const char *path, int block)
{
	int sock;
	struct sockaddr_un sun;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock < 0)
		return RTK_FAILED;

	if(fcntl(sock, F_SETFD, fcntl(sock, F_GETFD) | FD_CLOEXEC) == -1)
		printf("fcntl failed: %s %d\n", __func__, __LINE__);
	if(block==0){
		if(fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK) ==-1)
			printf("fcntl failed: %s %d\n", __func__, __LINE__);
	}
	if(server && access(path, F_OK)==0)
		unlink(path);
	else if(server==0 && access(path, F_OK)!=0)
	{
		close(sock);
		return RTK_FAILED;
	}

	memset(&sun, 0, sizeof(sun));
	sun.sun_family=AF_UNIX;
	snprintf(sun.sun_path, sizeof(sun.sun_path), "%s", path);

	if(server)
	{
		const int one = 1;
		if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) !=0)
			printf("setsockopt failed: %s %d\n", __func__, __LINE__);
		if (!bind(sock, (struct sockaddr*)&sun, sizeof(sun)) && !listen(sock, SOMAXCONN))
			return sock;
	}
	else
	{
		if (!connect(sock, (struct sockaddr*)&sun, sizeof(sun)) || errno == EINPROGRESS)
			return sock;
	}

	close(sock);
	return RTK_FAILED;
}

unsigned int rtk_wan_get_max_wan_mtu()
{
	MIB_CE_ATM_VC_T entry;
	int entryNum = 0, i = 0;
	unsigned int mtu = 0;
	unsigned int max_mtu = 2000;
	unsigned char tr247_unmatched_veip_cfg = 0;

	mib_get_s(MIB_TR247_UNMATCHED_VEIP_CFG, (void *)&tr247_unmatched_veip_cfg, sizeof(tr247_unmatched_veip_cfg));
	if(!tr247_unmatched_veip_cfg)
		mib_get(MIB_NETDEV_MTU, &max_mtu);

#if defined(CONFIG_TR142_MODULE) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	MIB_CE_SW_PORT_T portEntry;
	for(i=0;i<ELANVIF_NUM;i++)
	{
		if(!mib_chain_get(MIB_SW_PORT_TBL, i, &portEntry))
			continue;
		if(max_mtu < portEntry.omci_uni_mtu)
			max_mtu = portEntry.omci_uni_mtu;
	}
#endif
	if(!tr247_unmatched_veip_cfg)
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < entryNum; i++) {
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, &entry)) {
				printf("get MIB chain error\n");
				return max_mtu;
			}

			if(entry.enable==0)
				continue;

			if(entry.mtu<=max_mtu && entry.mtu>mtu){
				mtu = entry.mtu;
				if (entry.cmode == CHANNEL_MODE_PPPOE)
				{
					if(entry.mtu <= (max_mtu-8))
						mtu = entry.mtu+8;
					//Include IPV6 header to avoid IPv6 fragmentation
					if ((entry.dslite_enable) && (entry.mtu <= (max_mtu-48)))
						mtu = entry.mtu+48;
				}
			}
		}
	}
	if(mtu == 0){
		return max_mtu;
	}else{
		return mtu;
	}
}

unsigned int rtk_lan_get_max_lan_mtu()
{
	unsigned int max_mtu = 1500;

	mib_get_s(MIB_NETDEV_MTU, (void *)&max_mtu, sizeof(max_mtu));
	if (max_mtu < 1500)
		max_mtu = 1500;
	return max_mtu;
}

void rtk_lan_change_mtu()
{
	unsigned int orig_mtu, max_wan_mtu;
	int i=0;
#ifdef WLAN_SUPPORT
	char wlanif[20];
#endif

	getInMTU(BRIF,&orig_mtu);
	max_wan_mtu=rtk_lan_get_max_lan_mtu();
	if(orig_mtu==max_wan_mtu)
		return;

	for(i=0;i<ELANVIF_NUM;i++){
		setInMtu(ELANRIF[i], max_wan_mtu);
		setInMtu(ELANVIF[i], max_wan_mtu);
	}

#ifdef WLAN_SUPPORT
	rtk_wlan_get_ifname(0, 0, wlanif);
	setInMtu(wlanif, max_wan_mtu);
#ifdef WLAN_MBSSID
	// Set macaddr for VAP
	for (i=1; i<WLAN_SSID_NUM; i++) {
		rtk_wlan_get_ifname(0, i, wlanif);
		setInMtu(wlanif, max_wan_mtu);
	}
#endif
#if defined(WLAN_MESH) && defined(WLAN_USE_VAP_AS_SSID1)
	rtk_wlan_get_ifname(0, WLAN_ROOT_ITF_INDEX, wlanif);
	setInMtu(wlanif, max_wan_mtu);
#endif

#if defined(CONFIG_RTL_92D_DMDP) || (defined(WLAN_DUALBAND_CONCURRENT) && !defined(CONFIG_LUNA_DUAL_LINUX))
	rtk_wlan_get_ifname(1, 0, wlanif);
	setInMtu(wlanif, max_wan_mtu);

#ifdef WLAN_MBSSID
	// Set macaddr for VAP
	for (i=1; i<WLAN_SSID_NUM; i++) {
		rtk_wlan_get_ifname(1, i, wlanif);
		setInMtu(wlanif, max_wan_mtu);
	}
#endif
#if defined(WLAN_MESH) && defined(WLAN_USE_VAP_AS_SSID1)
	rtk_wlan_get_ifname(1, WLAN_ROOT_ITF_INDEX, wlanif);
	setInMtu(wlanif, max_wan_mtu);
#endif
#endif //CONFIG_RTL_92D_DMDP
#endif // WLAN_SUPPORT

	setInMtu(ELANIF, max_wan_mtu);

	setInMtu(LANIF, max_wan_mtu);
}

void down_up_lan_interface(void)
{
	int i, ifi_flags=0;
	int link_interface[ELANVIF_NUM+1]={0};
	unsigned char op_mode = getOpMode();

#ifdef WLAN_SUPPORT
#ifdef WLAN_11AX
	//FIXME!!!
#else
	deauth_wlan_clients();
#endif
#endif

	for (i=0; i<ELANVIF_NUM; i++){
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
		if (rtk_port_is_wan_logic_port(i))
			continue;
#endif
		getInFlags((const char *)ELANRIF[i], &ifi_flags);
		if((ifi_flags & IFF_UP) && (ifi_flags & IFF_RUNNING)){
			link_interface[i] = 1;
		}
	}

	if (op_mode == BRIDGE_MODE) {
		getInFlags(ALIASNAME_NAS0, &ifi_flags);
		if((ifi_flags & IFF_UP) && (ifi_flags & IFF_RUNNING)){
			link_interface[ELANVIF_NUM]= 1;
		}
	}

	for(i=0; i<ELANVIF_NUM; i++)
	{
		if(link_interface[i]){
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
			va_cmd(IFCONFIG, 2, 1, ELANVIF[i], "down");
			va_cmd(IFCONFIG, 2, 1, ELANRIF[i], "down");
#else
			va_cmd(IFCONFIG, 2, 1, ELANRIF[i], "down");
#endif
		}
	}

	if(link_interface[ELANVIF_NUM]){
		va_cmd(IFCONFIG, 2, 1, ALIASNAME_NAS0, "down");
	}
	//printf("\n[%s:%d] link_interface[ELANVIF_NUM]= %d,%d,%d,%d \n\n", __FUNCTION__,__LINE__,link_interface[0],link_interface[1],link_interface[2],link_interface[3]);
	sleep(3);

	for(i=0; i<ELANVIF_NUM; i++)
	{
		if(link_interface[i]){
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
			va_cmd(IFCONFIG, 2, 1, ELANRIF[i], "up");
			va_cmd(IFCONFIG, 2, 1, ELANVIF[i], "up");
#else
			va_cmd(IFCONFIG, 2, 1, ELANRIF[i], "up");
#endif
		}
	}

	if(link_interface[ELANVIF_NUM]){
		va_cmd(IFCONFIG, 2, 1, ALIASNAME_NAS0, "up");
	}

}

/*
 *	Add or Delete LAN virtual interface (such as eth0.2.0/eth0.3.0/eth0.4.0/eth0.5.0)
 *	logicPort: the logic port of the virtual interface
 *	action: 1 - add, 0 - delete
 */
int rtk_update_lan_virtual_interface(int logicPort, int action)
{
#if defined(CONFIG_RTL_SMUX_DEV)
	int flags = 0;

	if (action == 1) { //add
		if(!getInFlags(ELANVIF[logicPort], &flags))
			return Init_LanPort_Def_Intf(logicPort);
	} else { //delete
		if(getInFlags(ELANVIF[logicPort], &flags))
			return Deinit_LanPort_Def_Intf(logicPort);
	}
#endif
	return 0;
}

/*
 *	Check interface whether is in bridge interface list
 *	ifname: interface name
 *	brname: bridge name
 */
int rtk_is_interface_in_bridge(char *ifname, char *brname)
{
	int find_br = 0, find_if = 0;
	char tmp_buffer[128] = {0};
	FILE *fp=NULL;
	char *pchar = NULL;
	char line_buffer[256] = {0};

	if (ifname == NULL || brname == NULL)
		return 0;

	snprintf(tmp_buffer, sizeof(tmp_buffer), "%s show > /var/brctl_show", BRCTL);
	va_cmd("/bin/sh", 2, 1, "-c", tmp_buffer);

	if ((fp = fopen("/var/brctl_show", "r")) == NULL)
		return 0;

	fgets(line_buffer, sizeof(line_buffer), fp);

	while (fgets(line_buffer,sizeof(line_buffer), fp)) {
		if (find_br && !isspace((int)line_buffer[0]))
			break;

		pchar = line_buffer;
		while (*pchar != '\n' && *pchar != '\0')
			pchar++;

		*pchar = '\0';
		if (!find_br && !isspace((int)line_buffer[0])) {
			memset(tmp_buffer, 0, sizeof(tmp_buffer));
			sscanf(line_buffer, "%s	%*s", tmp_buffer);
			if (strcmp(tmp_buffer, brname) == 0)
				find_br=1;
		}
		if (find_br && (pchar = strstr(line_buffer, ifname)) != NULL) {
			snprintf(tmp_buffer, sizeof(tmp_buffer), "%s", pchar);
			if (strcmp(tmp_buffer, ifname) == 0) {
				find_if = 1;
				break;
			}
		}
	}
	fclose(fp);
	return find_if;
}

/*
 *	Check interface whether is lan ethernet interface
 */
int rtk_check_is_lan_ethernet_interface(char *ifname)
{
	int i;

	if (ifname == NULL)
		return 0;

	for (i = 0; i < ELANVIF_NUM; i++) {
		if (strcmp(ifname, ELANVIF[i]) == 0)
			return 1;

		if (strcmp(ifname, ELANRIF[i]) == 0
	#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
			&& rtk_port_is_wan_logic_port(i) == 0
	#endif
		)
			return 1;
	}

#ifdef CONFIG_USER_RTK_BRIDGE_MODE
	unsigned char op_mode = getOpMode();
	if (op_mode == BRIDGE_MODE && strcmp(ifname, ALIASNAME_NAS0) == 0)
		return 1;
#endif
	return 0;
}

int rtk_wan_check_Droute(int configAll, MIB_CE_ATM_VC_Tp pEntry, int *EntryID)
{
	int vcTotal=-1;
	int i,key,idx=-1;
	MIB_CE_ATM_VC_T Entry;

	if(pEntry == NULL || EntryID == NULL)
		return 0;

	if(!pEntry->enable || pEntry->dgw == 0)
		return 0;

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	if(vcTotal<0)
		return -1;

	key=0;
	for (i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;

		if(!Entry.enable || pEntry->ifIndex == Entry.ifIndex)
			continue;

		if(Entry.dgw)
		{
			fprintf(stderr, "%s-%d Disable default gateway on WAN idx=%d\n", __func__,__LINE__, idx);
			Entry.dgw = 0;
			mib_chain_update(MIB_ATM_VC_TBL, &Entry, i);
			*EntryID = i;
#ifdef COMMIT_IMMEDIATELY
			Commit();
#endif
			return 3;
		}
	}

	return 0;
}

#ifdef CONFIG_USER_PPPD
int rtk_get_pppd_uptime(char *pppname, unsigned int *uptime)
{
	FILE *fp = NULL;
	char tmpst[64]={0}, uptimeStr[16]={0}, *tok;
	int spid = 0;

	if ((pppname == NULL) || (uptime == NULL))
		return -1;
	if (fp = popen("pidof pppd","r"))
	{
		fgets(tmpst, sizeof (tmpst),fp);

		tok = strtok(tmpst, " ");
		while(tok != NULL)
		{
			spid = atoi(tok);
			tok = strtok(NULL, " ");

			if (spid)
				kill(spid, SIGTRAP);
		}
		usleep(500);
		fclose(fp);
	} else {
		fprintf(stderr, "pppd process not exists\n");
		return -1;
	}

	sprintf(tmpst, "%s/%s", PATH_IF_UPTIME, pppname);
	if ((fp = fopen(tmpst, "r")))
	{
		usleep(500);
		fgets(uptimeStr, sizeof(uptimeStr), fp);
		fclose(fp);
	} else {
		fprintf(stderr, "pppd uptime file not exists\n");
		return -1;
	}
	if(uptimeStr)
		*uptime = strtoul(uptimeStr, NULL, 10);
	else
		*uptime = 0;

	return 0;
}
#endif
#ifdef BLOCK_LAN_MAC_FROM_WAN
int set_lanhost_block_from_wan(int enable)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_BLOCK_FROM_WAN;

	mymsg->arg1 = enable;

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}
	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}
#endif

#ifdef CONFIG_USER_LANNETINFO
#ifdef _PRMT_X_TRUE_LASTSERVEICE
int get_flow_info(char *pFlowInfo)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret=0;
	int size =0;
	if(pFlowInfo == NULL)
		return -1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_FLOWINFO_GET;

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	memset(&qbuf, 0, sizeof(struct lanNetInfoMsg));
	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			size = (ret >= MAX_FLOW_INFO_SIZE)?MAX_FLOW_INFO_SIZE:ret;
			memcpy(pFlowInfo, mymsg->mtext, size);
			return 0;
		}
	}

	return -4;
}
#endif

#ifdef EASY_MESH_STA_INFO
int get_lan_device_mac_info(lanMacInfo_t **ppLANDeviceMac, unsigned int *pCount)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int ret=0, size=0;
	lanMacInfo_t *pLANDeviceMac = NULL;

	if( (ppLANDeviceMac==NULL)||(pCount==NULL) )
		return -1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_DEVICE_MAC_GET;

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	memset(&qbuf, 0, sizeof(struct lanNetInfoMsg));
	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			size = qbuf.msg.arg1*sizeof(lanMacInfo_t);
			pLANDeviceMac = (lanMacInfo_t *) malloc(size);

			if(pLANDeviceMac == NULL)
				return -3;

			*pCount = qbuf.msg.arg1;
			*ppLANDeviceMac = pLANDeviceMac;
			memcpy(pLANDeviceMac, mymsg->mtext, size);

			return 0;
		}
	}
}
#endif
#endif

#ifdef BLOCK_INVALID_SRCIP_FROM_WAN
int Block_Invalid_SrcIP_From_WAN(int enable)
{
	char *lan_ip_mask;
	char ipv6_prefix_str[64]={0};
	unsigned int ip6PrefixLen = 0;

	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING,(char *)ARG_I, "nas+", "-j", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
#ifndef USE_DEONET /* not used, 20221102 */
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING,(char *)ARG_I, "ppp+", "-j", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
#endif
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING,(char *)ARG_I, "nas+", "-j", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
#ifndef USE_DEONET /* not used, 20221102 */
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING,(char *)ARG_I, "ppp+", "-j", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
#endif
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
#endif
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
	va_cmd(EBTABLES, 6, 1, (char *)FW_DEL, (char *)FW_FORWARD,(char *)ARG_I, "nas+", "-j", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
	va_cmd(EBTABLES, 2, 1, "-X", FW_BLOCK_INVALID_SRCIP_FROM_WAN);
	if(!enable)
		return;
	lan_ip_mask=get_lan_ipnet();
#ifdef CONFIG_IPV6
	rtk_ipv6_get_prefix_len(&ip6PrefixLen);
	if(ip6PrefixLen>0)
	{
		char ipv6_prefix[64]={0};
		struct in6_addr ip6Prefix = {0};
		rtk_ipv6_get_prefix((void *)&ip6Prefix);
		if(inet_ntop(AF_INET6,&ip6Prefix,ipv6_prefix,sizeof(ipv6_prefix)))
			sprintf(ipv6_prefix_str,"%s/%d",ipv6_prefix,ip6PrefixLen);
	}
#endif
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_PREROUTING,(char *)ARG_I, "nas+", "-j", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
#ifndef USE_DEONET /* not used, 20221102 */
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_PREROUTING,(char *)ARG_I, "ppp+", "-j", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
#endif
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_PREROUTING,(char *)ARG_I, "nas+", "-j", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
#ifndef USE_DEONET /* not used, 20221102 */
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_INSERT, (char *)FW_PREROUTING,(char *)ARG_I, "ppp+", "-j", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
#endif
#endif
	va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);
	va_cmd(EBTABLES, 3, 1, "-P", FW_BLOCK_INVALID_SRCIP_FROM_WAN, "RETURN");
	va_cmd(EBTABLES, 6, 1, (char *)FW_INSERT, (char *)FW_FORWARD,(char *)ARG_I, "nas+", "-j", (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN);

	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN, "-s", "255.255.255.255", "-j", (char *)FW_DROP);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN, "-s", "224.0.0.0/4", "-j", (char *)FW_DROP);
	va_cmd(IPTABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN, "-s", lan_ip_mask, "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN,  "-s", "ff00::/8", "-j", (char *)FW_DROP);
	if(strlen(ipv6_prefix_str)>0)
		va_cmd(IP6TABLES, 8, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN,  "-s", ipv6_prefix_str, "-j", (char *)FW_DROP);
#endif
	va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN, "-p", "IPv4", "--ip-src", "255.255.255.255", "-j", (char *)FW_DROP);
	va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN, "-p", "IPv4", "--ip-src", "224.0.0.0/4", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
	va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char *)FW_BLOCK_INVALID_SRCIP_FROM_WAN, "-p", "IPv6", "--ip6-src", "ff00::/8", "-j", (char *)FW_DROP);
#endif
}
#endif

int rtk_wan_get_ifindex_by_ifname(char *ifname)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T vcEntry;
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	MIB_L2TP_T l2tpEntry;
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	MIB_PPTP_T pptpEntry;
#endif
#if defined(CONFIG_USER_RTK_MULTI_BRIDGE_WAN)
	MIB_CE_BR_WAN_T brEntry;
#endif
	char ifName[IFNAMSIZ] = {0};

	if(ifname == NULL) return -1;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
		{
			continue;
		}

		if (vcEntry.enable == 0)
			continue;

		ifGetName(vcEntry.ifIndex, ifName, sizeof(ifName));

		if(!strcmp(ifname, ifName)){
			return vcEntry.ifIndex;
		}
	}

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	entryNum = mib_chain_total(MIB_L2TP_TBL);
	for (i=0; i<entryNum; i++) {
		if(!mib_chain_get(MIB_L2TP_TBL, i, (void*)&l2tpEntry))
		{
			continue;
		}

		ifGetName(l2tpEntry.ifIndex, ifName, sizeof(ifName));

		if(!strcmp(ifname, ifName)){
			return l2tpEntry.ifIndex;
		}
	}
#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	entryNum = mib_chain_total(MIB_PPTP_TBL);
	for (i=0; i<entryNum; i++) {
		if(!mib_chain_get(MIB_PPTP_TBL, i, (void*)&pptpEntry))
		{
			continue;
		}

		ifGetName(pptpEntry.ifIndex, ifName, sizeof(ifName));

		if(!strcmp(ifname, ifName)){
			return pptpEntry.ifIndex;
		}
	}
#endif

#if defined(CONFIG_USER_RTK_MULTI_BRIDGE_WAN)
	entryNum = mib_chain_total(MIB_BR_WAN_TBL);
	for (i=0; i<entryNum; i++) {
		if(!mib_chain_get(MIB_BR_WAN_TBL, i, (void*)&brEntry))
		{
			continue;
		}

		ifGetName(brEntry.ifIndex, ifName, sizeof(ifName));

		if(!strcmp(ifname, ifName)){
			return brEntry.ifIndex;
		}
	}
#endif

	return -1;
}


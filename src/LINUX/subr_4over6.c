#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#ifdef EMBED
#include <linux/config.h>
#else
#include "../../../../include/linux/autoconf.h"
#endif

#include "mib.h"
#include "utility.h"
#include "sockmark_define.h"
#include "ipv6_info.h"
#include "subr_net.h"

#ifndef TUNNEL46_IFACE_LEN 
#ifndef IFNAMSIZ
#define	IFNAMSIZ	16
#endif
#define TUNNEL46_IFACE_LEN	IFNAMSIZ
#endif

#ifndef TUNNEL46_FILE_LEN
#define TUNNEL46_FILE_LEN 64
#endif

/**
 * must be under 29 chars, 
 * pls refer iptables-1.4.18/include/linux/netfilter/x_tables.h  
 * #define XT_EXTENSION_MAXNAMELEN 29
 **/
#ifndef FW_CHAIN_NAME_LEN
#define FW_CHAIN_NAME_LEN 29
#endif

#define TUNNEL46_ERR(fmt, arg...)  do{printf("[%s:%d][ERR]: "fmt"\n", __FUNCTION__, __LINE__, ##arg);}while(0)
//#define TUNNEL46_LOG_ENABLE 1
#ifdef TUNNEL46_LOG_ENABLE
#define TUNNEL46_DEBUG(fmt, arg...)		do{printf("[%s:%d][tid=%d]: "fmt"\n", __FUNCTION__, __LINE__, pthread_self(), ##arg);}while(0)
#else
#define TUNNEL46_DEBUG(fmt, arg...)		do{}while(0)
#endif


#ifdef DUAL_STACK_LITE
static int dslite_gen_static_mode_parameter(char *ifName);
static int dslite_gen_dhcp_mode_parameter(char *ifName, char *aftr);
static int do_dslite(char *ifName);
static int dslite_clean_tunnel(char *ifName);
static void dslite_add_firewall(char *ifName);
static void dslite_del_firewall(char *ifName);
#endif

#ifdef CONFIG_USER_MAP_E
static int mape_gen_static_mode_parameter(char *ifName);
static int mape_gen_dhcp_mode_parameter(char *ifName, char *extra_info);
static int do_mape(char *ifName);
static int mape_stop_tunnel(char *ifName);
static void mape_add_firewall(char *ifName);
static void mape_del_firewall(char *ifName);
#endif

#ifdef CONFIG_USER_MAP_T
static int mapt_gen_static_mode_parameter(char *ifName);
static int mapt_gen_dhcp_mode_parameter(char *ifName, char *extra_info);
static int do_mapt(char *ifName);
static int mapt_stop_tunnel(char *ifName);
static void mapt_add_firewall(char *ifName);
static void mapt_del_firewall(char *ifName);
#endif

#ifdef CONFIG_USER_XLAT464
static int xlat464_clat_static_mode_parameter(char *ifName);
static int xlat464_clat_dhcp_mode_parameter(char *ifName, char *extra_info);
static int do_xlat464(char *ifName);
static int xlat464_stop_interface(char *ifName);
static void xlat464_add_firewall(char *ifName);
static void xlat464_delete_firewall(char *ifName);
static int xlat464_delete_nat46(char *ifName);
static int xlat464_setup_nat46(char *ifName);
static int xlat464_set_policy_route(char *ifName, int AddOrDelete);
#endif

#ifdef CONFIG_USER_LW4O6
static int lw4o6_gen_static_mode_parameter(char *ifName);
static int lw4o6_gen_dhcp_mode_parameter(char *ifName, char *extra_info);
static int do_lw4o6(char *ifName);
static int lw4o6_stop_tunnel(char *ifName);
static int lw4o6_setup_tunnel(char *ifName);
static void lw4o6_add_firewall(char *ifName);
static void lw4o6_delete_firewall(char *ifName);
#endif

typedef struct tunnel46_info{
	int type;
	char tun_name_prefix[8];
	char tun_conf_file[TUNNEL46_FILE_LEN];
	int (*gen_static_mode_info)(char *ifName);
	int (*gen_dhcp_mode_info)(char *ifName, char *extra_info);
	int (*start_tun46)(char *ifName);
	int (*stop_tun46)(char *ifName);
	void (*add_firewall)(char *ifName);
	void (*del_firewall)(char *ifName);
}tunnel46_info_t, *tunnel46_info_Tp;

tunnel46_info_t global_tunnel46_t[] = {
#ifdef DUAL_STACK_LITE
	{
		.type = IPV6_DSLITE, 
		.tun_name_prefix = "dslite", 
		.tun_conf_file="/tmp/dslite.conf",		
		.gen_static_mode_info = dslite_gen_static_mode_parameter,
		.gen_dhcp_mode_info = dslite_gen_dhcp_mode_parameter,
		.start_tun46 = do_dslite,
		.stop_tun46 = dslite_clean_tunnel,
		.add_firewall = dslite_add_firewall,
		.del_firewall = dslite_del_firewall
	},
#endif
#ifdef CONFIG_USER_MAP_E
	{
		.type = IPV6_MAPE, 
		.tun_name_prefix = "mape", 
		.tun_conf_file="/tmp/mape.conf",
		.gen_static_mode_info = mape_gen_static_mode_parameter,
		.gen_dhcp_mode_info = mape_gen_dhcp_mode_parameter,
		.start_tun46 = do_mape,
		.stop_tun46 = mape_stop_tunnel,
		.add_firewall = mape_add_firewall,
		.del_firewall = mape_del_firewall
	},
#endif
#ifdef CONFIG_USER_MAP_T
	{
		.type = IPV6_MAPT, 
		.tun_name_prefix = "mapt", 
		.tun_conf_file="/tmp/mapt.conf",
		.gen_static_mode_info = mapt_gen_static_mode_parameter,
		.gen_dhcp_mode_info = mapt_gen_dhcp_mode_parameter,
		.start_tun46 = do_mapt,
		.stop_tun46 = mapt_stop_tunnel,
		.add_firewall = mapt_add_firewall,
		.del_firewall = mapt_del_firewall
	},
#endif
#ifdef CONFIG_USER_XLAT464
	{
		.type = IPV6_XLAT464,
		.tun_name_prefix = "xlat464",
		.tun_conf_file = "/tmp/xlat464.conf",
		.gen_static_mode_info = xlat464_clat_static_mode_parameter,
		.gen_dhcp_mode_info = xlat464_clat_dhcp_mode_parameter,
		.start_tun46 = do_xlat464,
		.stop_tun46 = xlat464_stop_interface,
		.add_firewall = xlat464_add_firewall,
		.del_firewall = xlat464_delete_firewall
	},
#endif
#ifdef CONFIG_USER_LW4O6
	{
		.type = IPV6_LW4O6,
		.tun_name_prefix = "lw4o6",
		.tun_conf_file = "/tmp/lw4o6.conf",
		.gen_static_mode_info = lw4o6_gen_static_mode_parameter,
		.gen_dhcp_mode_info = lw4o6_gen_dhcp_mode_parameter,
		.start_tun46 = do_lw4o6,
		.stop_tun46 = lw4o6_stop_tunnel,
		.add_firewall = lw4o6_add_firewall,
		.del_firewall = lw4o6_delete_firewall
	},
#endif
	//End
	{
		.type = -1, 
		.tun_name_prefix = "", 
		.tun_conf_file="",
		.gen_static_mode_info = NULL,
		.gen_dhcp_mode_info = NULL,
		.start_tun46 = NULL,
		.stop_tun46 = NULL,
		.add_firewall = NULL,
		.del_firewall = NULL
	}
};

static tunnel46_info_Tp tunnel46_get_tunnel_info(int tunType){
	int i = 0;
	for (i = 0; global_tunnel46_t[i].type != -1; i++){
		if (tunType == global_tunnel46_t[i].type){
			return &(global_tunnel46_t[i]);
		}
	}
	return NULL;
}

int rtk_tun46_generate_static_mode_parameter(int tunType, char *ifName){
	tunnel46_info_Tp tun46_info_tp = tunnel46_get_tunnel_info(tunType);
	if (tun46_info_tp == NULL)
		return -1;
	return tun46_info_tp->gen_static_mode_info(ifName);
}

int rtk_tun46_generate_dhcp_mode_parameter(int tunType, char *ifName, char *extra_info){
	tunnel46_info_Tp tun46_info_tp = tunnel46_get_tunnel_info(tunType);
	if (tun46_info_tp == NULL)
		return -1;
	return tun46_info_tp->gen_dhcp_mode_info(ifName, extra_info);
}

int rtk_tun46_setup_tunnel(int tunType, char *ifName){
	struct ipv6_ifaddr ip6_addr;
	tunnel46_info_Tp tun46_info_tp = tunnel46_get_tunnel_info(tunType);
	if (!tun46_info_tp || !ifName || (getifip6(ifName, IPV6_ADDR_UNICAST, &ip6_addr, 1) <= 0))
		return -1;
	
	return tun46_info_tp->start_tun46(ifName);
}

int rtk_tun46_delete_tunnel(int tunType, char *ifName){
	tunnel46_info_Tp tun46_info_tp = tunnel46_get_tunnel_info(tunType);
	if (tun46_info_tp == NULL)
		return -1;
	
	return tun46_info_tp->stop_tun46(ifName);
}

/*================================================================================*/
static int tunnel46_get_tunnel_name(int tunType, char *ifName, char *tunnelName){
	tunnel46_info_Tp tun46_info_tp = tunnel46_get_tunnel_info(tunType);
	if (tun46_info_tp == NULL)
		return -1;

	snprintf(tunnelName, TUNNEL46_IFACE_LEN, "%s_%s", tun46_info_tp->tun_name_prefix, ifName);
	return 1;
}

static int tunnel46_get_tunnel_conf_file(int tunType, char *ifName, char *confFile){
	tunnel46_info_Tp tun46_info_tp = tunnel46_get_tunnel_info(tunType);
	if (tun46_info_tp == NULL)
		return -1;

	snprintf(confFile, TUNNEL46_FILE_LEN, "%s.%s", tun46_info_tp->tun_conf_file, ifName);
	return 1;
}

static int is_tunnel46_conf_file_exsit(int tunType, char *ifName){
	char tunnel_conf_file[TUNNEL46_FILE_LEN] = {0};
	if (tunnel46_get_tunnel_conf_file(tunType, ifName, tunnel_conf_file) == 1){
		if (isFileExist(tunnel_conf_file))
			return 1;
	}
	return 0;
}

static void tunnel46_del_tunnel_iface(int tunType, char *ifName){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	if (tunnel46_get_tunnel_name(tunType, ifName, tunnel_name) != 1){
		TUNNEL46_ERR("(tunType=%d) Get ifName(%s) tunnel_name failed!", tunType, ifName);
		return;
	}
	
	va_cmd("/usr/bin/ip", 5, 1, "link", "set", "dev", tunnel_name, "down");	
	va_cmd("/usr/bin/ip", 4, 1, "-6", "tunnel", "del", tunnel_name );
}

	
typedef enum{POLICY_ROUTE_ADD=1, POLICY_ROUTE_DEL=2} POLICY_ROUTE_OPERATE;
static int tunnel46_set_multi_wan_policy_route(int tunType, char *ifName, int AddOrDelete){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	MIB_CE_ATM_VC_T vc_entry;
	int tbl_id;
	char tbl_id_str[32]={0};
	uint32_t wan_mark = 0, wan_mask = 0;
	char policy_mark_str[64] = {0}, iprule_pri_policyrt[16] = {0};
	char lan_addr4[20]={0};
	char operate[8]={0};
	char ifname[IFNAMSIZ];

	if (!ifName || (is_tunnel46_conf_file_exsit(tunType, ifName) != 1)){		
		TUNNEL46_ERR("(tunType=%d) conf file ifName(%s) not exsit!", tunType, ifName?ifName:"");
		return -1;
	}

	if (AddOrDelete == POLICY_ROUTE_ADD)
		snprintf(operate, sizeof(operate), "%s", "Add");
	else if (AddOrDelete == POLICY_ROUTE_DEL)
		snprintf(operate, sizeof(operate), "%s", "Del");
	else
		return 0;

	if (tunnel46_get_tunnel_name(tunType, ifName, tunnel_name) != 1){
		TUNNEL46_ERR("(tunType=%d) Get ifName(%s) tunnel_name failed!", tunType, ifName);
		return -1;
	}
		
	if (!(getATMVCEntryByIfName(ifName, &vc_entry))){	
		TUNNEL46_ERR("(tunType=%d) Could not get MIB entry info from wanconn %s!", tunType, ifName);
		return -1;
	}

	//IPv4 MAP-E policy route table for multi_wan
	tbl_id = caculate_tblid(vc_entry.ifIndex);
	snprintf(tbl_id_str, sizeof(tbl_id_str), "%d", tbl_id);

	ifGetName(vc_entry.ifIndex, ifname, sizeof(ifname));
	if (rtk_net_get_wan_policy_route_mark(ifname, &wan_mark, &wan_mask) == -1){	
		TUNNEL46_ERR("(tunType=%d) Error to get WAN policy mark for ifindex(%u)!", tunType, vc_entry.ifIndex);
		return -1;
	}
	snprintf(policy_mark_str, sizeof(policy_mark_str), "0x%x/0x%x", wan_mark, wan_mask);
	snprintf(iprule_pri_policyrt, sizeof(iprule_pri_policyrt), "%d", IP_RULE_PRI_POLICYRT);

	va_cmd(BIN_IP, 7, 1, "-4", "rule", "del", "fwmark", policy_mark_str, "table", tbl_id_str);
	va_cmd(BIN_IP, 6, 1, "-4", "rule", "del", "fwmark", policy_mark_str, "prohibit");
	va_cmd(BIN_IP, 5, 1, "-4", "route", "flush", "table", tbl_id_str);
	if (AddOrDelete == POLICY_ROUTE_ADD){
		va_cmd(BIN_IP, 9, 1, "-4", "rule", "add", "fwmark", policy_mark_str, "pref", iprule_pri_policyrt, "table", tbl_id_str);

		snprintf(iprule_pri_policyrt, sizeof(iprule_pri_policyrt), "%d", IP_RULE_PRI_DEFAULT);
		va_cmd(BIN_IP, 8, 1, "-4", "rule", "add", "fwmark", policy_mark_str, "pref", iprule_pri_policyrt, "prohibit");

		va_cmd(BIN_IP, 8, 1, "-4", "route", "add", "default", "dev", tunnel_name, "table", tbl_id_str);
		
		va_cmd(BIN_IP, 8, 1, "-6", "route", "add", "default", "dev", ifName, "table", tbl_id_str);

		//other route rules (br0)?
		getPrimaryLanNetAddrInfo(lan_addr4,sizeof(lan_addr4));
		va_cmd(BIN_IP, 8, 1, "-4", "route", "add", lan_addr4, "dev", LANIF, "table", tbl_id_str);		
	}
	
	return 1;
}

//Ds-Lite
#ifdef DUAL_STACK_LITE
#define DSLITE_LOCAL_ADDR6_INFO "local_addr6:"
#define DSLITE_REMOTE_AFTR_INFO "remote_aftr:"
#define DSLITE_REMOTE_ADDR6_INFO "remote_addr6:"

static pthread_t cur_dslite_ptid;
static int dslite_parse_conf_file(char *ifName, char *localAddr6, int localLen, 
	char *aftrInfo, int aftrInfoLen, char *remoteAddr6, int remoteLen){
	char conf_file[TUNNEL46_FILE_LEN] = {0};
	FILE *fp = NULL;
	char line[128]={0};
	char local_addr6[INET6_ADDRSTRLEN]={0}, remote_info[64]={0};//remote info may be hostname
	int cnt = 0;	

	if (!ifName || tunnel46_get_tunnel_conf_file(IPV6_DSLITE, ifName, conf_file) != 1)
		return -1;
	
	if ( (fp = fopen(conf_file, "r")) != NULL){
		while(fgets(line, sizeof(line), fp)){
			if (line[strlen(line)-1] == '\n')
				line[strlen(line)-1] = '\0';
				
			if (strstr(line, DSLITE_LOCAL_ADDR6_INFO)){			
				snprintf(local_addr6, sizeof(local_addr6), "%s", line+strlen(DSLITE_LOCAL_ADDR6_INFO));
				TUNNEL46_DEBUG("(dslite) line=%s, local_addr6=%s\n", line, local_addr6);
				cnt++;
				
				if (localAddr6)
					snprintf(localAddr6, localLen, "%s", local_addr6);
			}else if (strstr(line, DSLITE_REMOTE_AFTR_INFO)){
				snprintf(remote_info, sizeof(remote_info), "%s", line+strlen(DSLITE_REMOTE_AFTR_INFO));
				TUNNEL46_DEBUG("(dslite) line=%s, remote_info=%s\n", line, remote_info);
				cnt++;
				
				if (aftrInfo)
					snprintf(aftrInfo, aftrInfoLen, "%s", remote_info);			
			}else if (strstr(line, DSLITE_REMOTE_ADDR6_INFO)){
				snprintf(remote_info, sizeof(remote_info), "%s", line+strlen(DSLITE_REMOTE_ADDR6_INFO));
				TUNNEL46_DEBUG("(dslite) line=%s, remote_addr6=%s\n", line, remote_info);
				cnt++;
				
				if (remoteAddr6)
					snprintf(remoteAddr6, remoteLen, "%s", remote_info);			
			}
		}
		fclose(fp);
	}else
		return 0;

	if (cnt >= 2)//must have 2 arguments at least (local and remote)
		return 1;
	return 0;
}

static void dslite_del_firewall(char *ifName){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	if (!ifName || (tunnel46_get_tunnel_name(IPV6_DSLITE, ifName, tunnel_name) != 1))
		return;
	//Delete TCP MSS
	va_cmd("/bin/iptables", 13, 1, "-D", "FORWARD", "-o", tunnel_name, "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j", "TCPMSS", "--set-mss", "1412");
}

static void dslite_add_firewall(char *ifName){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	if (!ifName || (tunnel46_get_tunnel_name(IPV6_DSLITE, ifName, tunnel_name) != 1))
		return;
	//Del TCP MSS
	va_cmd("/bin/iptables", 13, 1, "-D", "FORWARD", "-o", tunnel_name, "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j", "TCPMSS", "--set-mss", "1412");
	
	//Add TCP MSS
	va_cmd("/bin/iptables", 13, 1, "-I", "FORWARD", "-o", tunnel_name, "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j", "TCPMSS", "--set-mss", "1412");
}

static int dslite_del_multi_wan_policy_route(char *ifName){
	return tunnel46_set_multi_wan_policy_route(IPV6_DSLITE, ifName, POLICY_ROUTE_DEL);
}

static int dslite_add_multi_wan_policy_route(char *ifName){
	return tunnel46_set_multi_wan_policy_route(IPV6_DSLITE, ifName, POLICY_ROUTE_ADD);
}

static void dslite_del_tunnel_iface(char *ifName){
	tunnel46_del_tunnel_iface(IPV6_DSLITE, ifName);
}

#ifdef CONFIG_SUPPORT_AUTO_DIAG
static int get_SIMU_DSLiteENTRY(MIB_CE_ATM_VC_Tp pEntry, int *entry_index)
{
	int entryNum, i;

	entryNum = mib_chain_total(MIB_SIMU_ATM_VC_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_SIMU_ATM_VC_TBL, i, (void *)pEntry))
		{
			TUNNEL46_ERR("(dslite) Get MIB_SIMU_ATM_VC_TBL chain record error!\n");
			return -1;
		}

		if (pEntry->dslite_enable == 1)
		{
			*entry_index=i;
			return 0;
		}
	}

	return 0;
}

static int dslite_inform_autoSimu(char *ifName){
	int ret, entry_index;
	MIB_CE_ATM_VC_T Entry;
	unsigned int simu_pid;
	char aftr[64] = {0};
	
	if (!ifName || (strcmp(ifName, "ppp7") != 0))
		return 0;

	if ((dslite_parse_conf_file(ifName, NULL, 0, aftr, sizeof(aftr), NULL, 0) == 1) && (aftr[0] != 0)){
		ret = get_SIMU_DSLiteENTRY(&Entry, &entry_index);
		if(ret){
			TUNNEL46_ERR("(dslite) [SIMU]Find SIMU_ATM_VC_TBL interface %s Fail!\n",ifName);
			return 0;
		}
				
		simu_pid = read_pid("/var/run/autoSimu.pid");
		if (simu_pid > 0){
			TUNNEL46_ERR("(dslite) [SIMU]Send signal USR2 to autoSimu pid %d\n",simu_pid);
			kill(simu_pid, SIGUSR2);
		}
	}

	return 1;
}

#endif

/* Return value: 1 means error INPUT, so don't need to retry.
 *               2 means getaddrinfo not success, maybe AFTR need to retry to ask server.
 *               0 means get address info for AFTR successfully.
 */
int query_aftr(char *aftr,  char *aftr_dst, char *aftr_addr_str)
{
	struct addrinfo hints, *res, *p;
	int status;
	char ipstr[INET6_ADDRSTRLEN];
	void *addr;
	int ret = 2;

	if(!aftr || !aftr_dst || !aftr_addr_str){
		TUNNEL46_ERR("(dslite) Error! Null Input!");
		return 1;
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;

	TUNNEL46_DEBUG("(dslite) aftr=[%s]",aftr);
	if ((status = getaddrinfo(aftr, NULL, &hints, &res)) != 0) {
		TUNNEL46_ERR("(dslite) getaddrinfo:%s %s", aftr, gai_strerror(status));
		return 2;
	}

	for (p = res; p != NULL; p = p->ai_next){
		if (p->ai_family == AF_INET6) { // IPv6
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			memcpy(aftr_dst,addr,sizeof(struct in6_addr));

			// convert the IP to a string and print it:
			inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
			TUNNEL46_DEBUG("(dslite) IP addresses for [%s]: [%s]", aftr, ipstr);

			strcpy(aftr_addr_str,ipstr);
			ret = 0;
			break;
		}
	}
	
	freeaddrinfo(res);

	return ret;
}

static int getATM_VC_ENTRY_byName(char *pIfname, MIB_CE_ATM_VC_T *pEntry, int *entry_index)
{
	unsigned int entryNum, i;
	char ifname[IFNAMSIZ];

	if(pIfname == NULL) {
		return -1;
	}

	if(strlen(pIfname) == 0) {
		return -1;
	} 
	
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)pEntry))
		{
			printf("Get chain record error!\n");
			return -1;
		}

		if (pEntry->enable == 0)
			continue;

		ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));

		if(!strcmp(ifname, pIfname)) {
			break;
		}
	}

	if(i >= entryNum) {
		//printf("not find this interface!\n");
		return -1;
	}

	*entry_index = i;
	return 0;
}

static int dslite_get_local_remote_addr6(char *ifName){
	char local_addr6[INET6_ADDRSTRLEN]={0}, aftr_addr6[INET6_ADDRSTRLEN]={0};
	char aftr_info[64]={0};
	struct ipv6_ifaddr ip6_addr;
	int cur_retry_cnt=0, max_retry_cnt=0;
	MIB_CE_ATM_VC_T vc_entry, *pEntry = &vc_entry;
	int entry_idx=0, ret=0;
	pthread_t myptid = pthread_self();
	char dslite_conf_file[TUNNEL46_FILE_LEN]={0};
	char buf[128]={0};
	
	if (!ifName)
		return -1;

	if (dslite_parse_conf_file(ifName, local_addr6, sizeof(local_addr6), aftr_info, sizeof(aftr_info), NULL, 0) != 1){
		TUNNEL46_ERR("(dslite) parse_conf_file ifName(%s) failed!", ifName);
		return 0;
	}

	if (!mib_get_s(MIB_DSLITE_RETRY_COUNT, (void *)&max_retry_cnt, sizeof(max_retry_cnt))) {
		TUNNEL46_ERR("(dslite) get MIB_DSLITE_RETRY_COUNT fail!");
	}
	
// resolving AFTR to aftr_addr	
	if(inet_pton(PF_INET6, aftr_info, &ip6_addr) <= 0){//assume aftr_info is hostname, so try to get the corresponding IP address
		ret = getATM_VC_ENTRY_byName(ifName, pEntry, &entry_idx);
		if (ret != 0 || !pEntry->dslite_enable){
			TUNNEL46_ERR("(dslite) Dslite is disable in ATM_VC_TBL now ifName(%s)!!!\n", ifName);
			return 0;
		}
retry_get_remote6: 	
		ret = query_aftr(aftr_info, (char *)&(ip6_addr.addr), aftr_addr6);
		//First check the ptid after getaddrinfo, Note getaddrinfo has a select timeout (RES_TIMEOUT 5s by default), So do not sleep here
		if (myptid != cur_dslite_ptid) {
			TUNNEL46_ERR("(dslite) Not current pthread my_pid=%lu curr_pid=%lu\n", myptid, cur_dslite_ptid);
			return 0;
		}

		if (ret == 0){ //Get response for AFTR IP.
			if(pEntry->dslite_aftr_hostname[0] == 0){//update IPv6 address of aftr to AFTR MIB
				strcpy(pEntry->dslite_aftr_hostname, aftr_addr6);
				mib_chain_update(MIB_ATM_VC_TBL, pEntry, entry_idx);
			}
		}else {
			cur_retry_cnt++;
			if ( (max_retry_cnt > 0) && (cur_retry_cnt > max_retry_cnt)){//MIB_DSLITE_RETRY_COUNT = 0, means retry count is infinity.
				return 0;
			} else{
				sleep(3);
				goto retry_get_remote6;
			}
		}
	}else{ //aftr_info is IPv6 address  
		snprintf(aftr_addr6, sizeof(aftr_addr6), "%s", aftr_info);
		ret = getATM_VC_ENTRY_byName(ifName, pEntry, &entry_idx);
		if (ret != 0 || !pEntry->dslite_enable){
			TUNNEL46_ERR("(dslite) Dslite is disable in ATM_VC_TBL now ifName(%s)!!!\n", ifName);
			return 0;
		}
		if(pEntry->dslite_aftr_hostname[0] == 0 || strcmp(pEntry->dslite_aftr_hostname,aftr_addr6) != 0 ){//update IPv6 address of aftr to AFTR MIB
			strcpy(pEntry->dslite_aftr_hostname, aftr_addr6);
			mib_chain_update(MIB_ATM_VC_TBL, pEntry, entry_idx);
		}
	}

	//update aftr IPv6 address to dslite file	
	if (tunnel46_get_tunnel_conf_file(IPV6_DSLITE, ifName, dslite_conf_file) == 1){
		snprintf(buf, sizeof(buf), "echo \"%s%s\" >> %s", DSLITE_REMOTE_ADDR6_INFO, aftr_addr6, dslite_conf_file);
		system(buf);
		return 1;
	} else
		return -1;
}

static int dslite_setup_tunnel_iface(char *ifName){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	char local_addr6[INET6_ADDRSTRLEN]={0}, remote_addr6[INET6_ADDRSTRLEN]={0};
	MIB_CE_ATM_VC_T vc_entry;
	
	if (!ifName || (tunnel46_get_tunnel_name(IPV6_DSLITE, ifName, tunnel_name) != 1))
		return -1;

	if (dslite_parse_conf_file(ifName, local_addr6, sizeof(local_addr6), NULL, 0, remote_addr6, sizeof(remote_addr6)) != 1){
		TUNNEL46_ERR("(dslite) parse_conf_file ifName(%s) failed!", ifName);
		return -1;
	}

	TUNNEL46_DEBUG("(dslite) ifName(%s): local_addr6=%s, remote_addr6=%s",ifName, local_addr6, remote_addr6);	
	
	// ip -6 tunnel add dslite_nas0_0 mode ipip6 local 2001:240:63f:ff00::3 remote 2001:240:63f:ff01::1 dev nas0_0
	va_cmd("/usr/bin/ip", 14, 1, "-6", "tunnel", "add", tunnel_name, "mode", "ipip6", "local", local_addr6, "remote", remote_addr6, "dev", ifName, "encaplimit", "none");

	// ip link set dev dslite_nas0_0 up
	va_cmd("/usr/bin/ip", 5, 1, "link", "set", "dev", tunnel_name, "up");

	// ip address add 192.0.0.2 peer 192.0.0.1 dev dslite_nas0_0
	va_cmd("/usr/bin/ip", 7, 1, "addr", "add", "192.0.0.2", "peer", "192.0.0.1", "dev", tunnel_name);
	
	//Set the default route in main table to the last set Ds-Lite interface
	if (!(getATMVCEntryByIfName(ifName, &vc_entry))){	
		TUNNEL46_ERR("(dslite) Could not get MIB entry info from wanconn (ifName=%s)", ifName);
		return -1;
	}

	if (isDefaultRouteWan(&vc_entry)){
		va_cmd("/usr/bin/ip", 3, 1, "route", "delete", "default"); 
		va_cmd("/usr/bin/ip", 5, 1, "route", "add", "default", "dev", tunnel_name); //needed by packets from DUT self 
	}
	
	return 1;
}

static int dslite_clean_tunnel(char *ifName)
{
	if (!ifName || (is_tunnel46_conf_file_exsit(IPV6_DSLITE, ifName) != 1))
		return -1;

	/*delete dslite firewalls*/
	dslite_del_firewall(ifName);

	/*delete mape policy route for multi_wan*/
	dslite_del_multi_wan_policy_route(ifName);

	/*delete mape interface and some files*/
	dslite_del_tunnel_iface(ifName);
	
	return 1;
}

static void dslite_setup_tunnel(char *ifName)
{
	if (!ifName)
		return;

#ifdef CONFIG_SUPPORT_AUTO_DIAG
	if (dslite_inform_autoSimu(ifName) != 1)
#endif
	{
		if (dslite_get_local_remote_addr6(ifName) == 1){
			/*setup ds-lite tunnel interface*/
			dslite_setup_tunnel_iface(ifName);

			/*set filrewall rules*/
			dslite_add_firewall(ifName);

			/*set policy route for multi_wan*/
			dslite_add_multi_wan_policy_route(ifName);
		}	
	}
}

/**
 * Save the dslite local and remote information to info file
 **/
static int dslite_gen_static_mode_parameter(char *ifName)
{
	char wan_iface[IFNAMSIZ]={0};
	struct ipv6_ifaddr ip6_addr;
	char addr6_str[INET6_ADDRSTRLEN]={0};
	MIB_CE_ATM_VC_T vc_entry = {0};
	int entry_idx=0;
	char tmp_buf[256] = {0};	
	FILE *fp = NULL;	
		
	TUNNEL46_DEBUG("(dslite) ifName(%s)", ifName);

	if (!ifName)
		return -1;
	
	memcpy(wan_iface, ifName, (sizeof(wan_iface)>strlen(ifName)?strlen(ifName):sizeof(wan_iface)));
	if (getWanEntrybyIfname(wan_iface, &vc_entry, &entry_idx) < 0)
		return -1;

	if (tunnel46_get_tunnel_conf_file(IPV6_DSLITE, wan_iface, tmp_buf) != 1)
		return -1;		
	if ( (fp = (FILE *)fopen(tmp_buf, "w")) == NULL){
		TUNNEL46_ERR("(dslite) open %s failed!", tmp_buf);
		return -1;
	}

	if(getifip6(ifName, IPV6_ADDR_UNICAST, &ip6_addr, 1) > 0){
		if (inet_ntop(AF_INET6, &ip6_addr.addr, addr6_str, sizeof(addr6_str))){
			snprintf(tmp_buf, sizeof(tmp_buf), "%s%s\n", DSLITE_LOCAL_ADDR6_INFO, addr6_str);
			fputs(tmp_buf, fp);
			TUNNEL46_DEBUG("(dslite) tmp_buf(%s)", tmp_buf);
		}
	}else {
		fclose(fp);
		return 0;
	}
	
	snprintf(tmp_buf, sizeof(tmp_buf), "%s%s\n", DSLITE_REMOTE_AFTR_INFO, vc_entry.dslite_aftr_hostname);
	fputs(tmp_buf, fp);
	TUNNEL46_DEBUG("(dslite) tmp_buf(%s)", tmp_buf);

	fclose(fp);
	return 1;
}

static int dslite_gen_dhcp_mode_parameter(char *ifName, char *aftr){
	char wan_iface[IFNAMSIZ]={0};
	struct ipv6_ifaddr ip6_addr;
	char addr6_str[INET6_ADDRSTRLEN]={0};
	char tmp_buf[256] = {0};	
	FILE *fp = NULL;
	
	if (!ifName || !aftr)
		return -1;

	memcpy(wan_iface, ifName, (sizeof(wan_iface)>strlen(ifName)?strlen(ifName):sizeof(wan_iface)));
	if (tunnel46_get_tunnel_conf_file(IPV6_DSLITE, wan_iface, tmp_buf) != 1)
		return -1;	
	if ( (fp = (FILE *)fopen(tmp_buf, "w")) == NULL){
		TUNNEL46_ERR("(dslite) open %s failed!", tmp_buf);
		return -1;
	}

	if(getifip6(ifName, IPV6_ADDR_UNICAST, &ip6_addr, 1) > 0){
		if (inet_ntop(AF_INET6, &ip6_addr.addr, addr6_str, sizeof(addr6_str))){
			snprintf(tmp_buf, sizeof(tmp_buf), "%s%s\n", DSLITE_LOCAL_ADDR6_INFO, addr6_str);
			fputs(tmp_buf, fp);
			TUNNEL46_DEBUG("(dslite) tmp_buf(%s)", tmp_buf);
		}
	}else {
		fclose(fp);
		return 0;
	}
	
	snprintf(tmp_buf, sizeof(tmp_buf), "%s%s\n", DSLITE_REMOTE_AFTR_INFO, aftr);
	fputs(tmp_buf, fp);
	TUNNEL46_DEBUG("(dslite) tmp_buf(%s)", tmp_buf);

	fclose(fp);
	return 1;
}


/*
 *	pthread to parse aftr_info, resolve aftr and setup dslite tunnel.
 */
static void *pt_dslite_setup(void *ifName)
{
	pthread_t my_ptid = pthread_self();
	char wan_iface[IFNAMSIZ]={0};

	memcpy(wan_iface, ifName, (sizeof(wan_iface)>strlen(ifName)?strlen(ifName):sizeof(wan_iface)));
	free(ifName);
	
	if (my_ptid != cur_dslite_ptid) 
		return NULL;

	dslite_clean_tunnel(wan_iface);
	dslite_setup_tunnel(wan_iface);
	return NULL;
}

/*
 *	aftr_str: "[ifname],[aftr_hostname]", ex. "nas0_0,dslite.aftr.org"
 */
static int do_dslite(char *ifName)
{
	pthread_t ptid;
	char *wan_name;

	if (!ifName)
		return -1;

	if ((wan_name = (char *)malloc(IFNAMSIZ)) == NULL)
		return -1;
	
	memset(wan_name, 0x00, IFNAMSIZ);
	snprintf(wan_name, IFNAMSIZ, "%s", ifName);
	if (pthread_create(&ptid, NULL, pt_dslite_setup, (void *)wan_name)==0)
		cur_dslite_ptid = ptid;
	else{
		free(wan_name);
		return -1;
	}
	
	pthread_detach(ptid);
	return 0;
}
#endif

//IPv6 MAP-E
#ifdef CONFIG_USER_MAP_E
#define MAPE_INFO_FILE "/tmp/mape_%s.info"

#define MAPE_BMR_DMR_INFO "BMR_DMR: ipv6prefix=%s,prefix6len=%d,ipv4prefix=%s,prefix4len=%d,psid=%d,psidlen=%d,offset=%d,ealen=%d,br=%s\n"
#define MAPE_FMR_ENABLED_INFO "FMR = 1\n"
#define MAPE_FMR_DISABLED_INFO "FMR = 0\n"
#define MAPE_FMR_INFO "FMR: ipv6prefix=%s,prefix6len=%d,ipv4prefix=%s,prefix4len=%d,ealen=%d,offset=%d\n"

static char mape_input[FW_CHAIN_NAME_LEN]="mape_input";
static char mape_forward[FW_CHAIN_NAME_LEN]="mape_forward";
static char mape_output[FW_CHAIN_NAME_LEN]="mape_output";
static char mape_prerouting[FW_CHAIN_NAME_LEN]="mape_prerouting";
static char mape_postrouting[FW_CHAIN_NAME_LEN]="mape_postrouting";

static int mape_add_multi_wan_policy_route(char *ifName);
static int mape_del_multi_wan_policy_route(char *ifName);

static int mape_add_local_set_mark_firewall(char *ifName);
static int mape_del_local_set_mark_firewall(char *ifName);
static int mape_add_local_policy_route(char *ifName);
static int mape_del_local_policy_route(char *ifName);

static int mape_get_tunnel_addr4(char *ifName, char *v4AddrStr, int v4AddrLen){
	char conf_file[TUNNEL46_FILE_LEN] = {0};
	char line[256] = {0};
	FILE *fp = NULL;
	char *ptr = NULL;

	if (!ifName || !v4AddrStr)
		return -1;

	if (tunnel46_get_tunnel_conf_file(IPV6_MAPE, ifName, conf_file) != 1)
		return -1;
	if ((fp = fopen(conf_file, "r")) == NULL)
		return 0;
	while(fgets(line, sizeof(line), fp)){
		if (strstr(line, "BMR_DMR")){
			ptr = strtok(line, ",");
			while(ptr){
				if (strstr(ptr, "ipv4prefix=")){
					memset(v4AddrStr, 0x00, v4AddrLen);
					sscanf(ptr, "ipv4prefix=%s", v4AddrStr);

					fclose(fp);
					return 1;
				}
				ptr = strtok(NULL, ",");
			}
		}
	}

	fclose(fp);
	return 0;	
}

static int mape_get_fmr_support_status(char *ifName, int *fmr_enabled){
	char conf_file[TUNNEL46_FILE_LEN] = {0};
	FILE *fp = NULL;
	char line[256] = {0};
	
	if (!ifName || !fmr_enabled)
		return -1;
	
	if (tunnel46_get_tunnel_conf_file(IPV6_MAPE, ifName, conf_file) != 1)
		return -1;
	if ((fp = fopen(conf_file, "r")) != NULL){
		while (fgets(line, sizeof(line), fp)){
			if (strstr(line, MAPE_FMR_ENABLED_INFO)){
				*fmr_enabled = 1;
				break;
			}else if (strstr(line, MAPE_FMR_DISABLED_INFO)){
				*fmr_enabled = 0;
				break;
			}
		} 
		fclose(fp);
		return 1;
	}
	return 0;
}

/**
 * PREROUTING -> mape_prerouting -> PREROUTING_mape_nas0_0
 */
static int mape_get_permape_chain(char *ifName, const char *hookChain, char *chainName){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	if (tunnel46_get_tunnel_name(IPV6_MAPE, ifName, tunnel_name) != 1)
		return 0;
		
    memset(chainName, 0x00, FW_CHAIN_NAME_LEN);
    snprintf(chainName, FW_CHAIN_NAME_LEN, "%s_%s", hookChain, tunnel_name);
	return 1;
}

static int mape_add_snat_firewall(char *ifName){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	char tunnel_addr4[16] = {0};
	char *potocol[3] = {"icmp", "tcp", "udp"};
	char per_mape_post_chain[FW_CHAIN_NAME_LEN];	
	char info_file[64]={0};
	FILE *fp=NULL;	
	char line[1024]={0}, snat_buf[64] = {0};
	MIB_CE_ATM_VC_T vc_entry;	
	char *ptr, *save_ptr;	
	int i;

	TUNNEL46_DEBUG("(mape) ifName(%s)", ifName);

	if (!ifName || (tunnel46_get_tunnel_name(IPV6_MAPE, ifName, tunnel_name) != 1)){
		TUNNEL46_ERR("(mape) Get mape ifName(%s) tunnel_name failed!", ifName?ifName:"");
		return -1;
	}
	
	if (mape_get_tunnel_addr4(ifName, tunnel_addr4, sizeof(ifName)) != 1){		
		TUNNEL46_ERR("(mape) Get mape ifName(%s) tunnel_addr4 failed!", ifName);
		return -1;
	}
		
	if (!(getATMVCEntryByIfName(ifName, &vc_entry))){		
		TUNNEL46_ERR("(mape) Error! Could not get MIB entry info from wanconn %s!", ifName);
		return -1;
	}

	snprintf(info_file, sizeof(info_file), MAPE_INFO_FILE, ifName);
	if ( (fp = (FILE *)fopen(info_file, "r")) == NULL){		
		TUNNEL46_ERR("(mape) Error! open %s failed!!", info_file);
		return -1;
	}
	
	mape_get_permape_chain(ifName, FW_POSTROUTING, per_mape_post_chain);
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-F", per_mape_post_chain);
	while(fgets(line, sizeof(line), fp) != NULL){
		if (strstr(line, "PORTSETS=")){		
			ptr = strtok_r(line+strlen("PORTSETS="), " ", &save_ptr);
			while(ptr){
				if (ptr[strlen(ptr)-1] == '\n')//not include the charater "\n"
					ptr[strlen(ptr)-1] = '\0';

				if (ptr[0] == '\0')
					break;
				
				snprintf(snat_buf, sizeof(snat_buf), "%s:%s", tunnel_addr4, ptr);		
				TUNNEL46_DEBUG("(mape) ifName(%s), snat_buf(%s)", ifName, snat_buf);
				
				for(i=0; i<3; i++){									
				#ifdef CONFIG_MAPE_ROUTING_MODE //Feature: routing mode support
					if (vc_entry.mape_forward_mode == IPV6_MAPE_NAT_MODE){//nat mode
						va_cmd("/bin/iptables", 12, 1, "-t", "nat", "-A", per_mape_post_chain, "-o", tunnel_name, "-p", potocol[i], "-j", "SNAT", "--to-source", snat_buf);
					}else{//routing mode: LAN packets do routing and DUT packets do SNAT				
						va_cmd("/bin/iptables", 14, 1, "-t", "nat", "-A", per_mape_post_chain, "-o", tunnel_name, "-p", potocol[i], "-s", tunnel_addr4, "-j", "SNAT", "--to-source", snat_buf);
					}
				#else
					va_cmd("/bin/iptables", 12, 1, "-t", "nat", "-A", per_mape_post_chain, "-o", tunnel_name, "-p", potocol[i], "-j", "SNAT", "--to-source", snat_buf);
				#endif
				}
				ptr = strtok_r(NULL, " ", &save_ptr);
			}

			fclose(fp);
			return 1;
		}else
			continue;
	}

	fclose(fp);
	return -1;
}

static void mape_add_firewall(char *ifName){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	char per_mape_pre_chain[FW_CHAIN_NAME_LEN], per_mape_post_chain[FW_CHAIN_NAME_LEN];
	char per_mape_input_chain[FW_CHAIN_NAME_LEN], per_mape_output_chain[FW_CHAIN_NAME_LEN];
	char per_mape_forward_chain[FW_CHAIN_NAME_LEN];
		
	TUNNEL46_DEBUG("(mape) ifName(%s)", ifName);

	if (!ifName || (tunnel46_get_tunnel_name(IPV6_MAPE, ifName, tunnel_name) != 1)){		
		TUNNEL46_ERR("(mape) Get mape ifName(%s) tunnel_name failed!", ifName?ifName:"");
		return;
	}

	mape_get_permape_chain(ifName, FW_PREROUTING, per_mape_pre_chain);
	mape_get_permape_chain(ifName, FW_POSTROUTING, per_mape_post_chain);
	mape_get_permape_chain(ifName, FW_FORWARD, per_mape_forward_chain);
	mape_get_permape_chain(ifName, FW_INPUT, per_mape_input_chain);
	mape_get_permape_chain(ifName, FW_OUTPUT, per_mape_output_chain);
	
//filter
	va_cmd("/bin/iptables", 2, 1, "-N", mape_input);
	va_cmd("/bin/iptables", 2, 1, "-N", mape_forward);
	va_cmd("/bin/iptables", 2, 1, "-N", mape_output);	
	va_cmd("/bin/iptables", 4, 1, "-D", "INPUT", "-j", mape_input);
	va_cmd("/bin/iptables", 4, 1, "-D", "FORWARD", "-j", mape_forward);
	va_cmd("/bin/iptables", 4, 1, "-D", "OUTPUT", "-j", mape_output);	
	va_cmd("/bin/iptables", 4, 1, "-A", "INPUT", "-j", mape_input);
	va_cmd("/bin/iptables", 4, 1, "-A", "FORWARD", "-j", mape_forward);
	va_cmd("/bin/iptables", 4, 1, "-A", "OUTPUT", "-j", mape_output);

	va_cmd("/bin/iptables", 2, 1, "-N", per_mape_input_chain);
	va_cmd("/bin/iptables", 2, 1, "-N", per_mape_forward_chain);
	va_cmd("/bin/iptables", 2, 1, "-N", per_mape_output_chain);
	va_cmd("/bin/iptables", 2, 1, "-F", per_mape_input_chain);
	va_cmd("/bin/iptables", 2, 1, "-F", per_mape_forward_chain);
	va_cmd("/bin/iptables", 2, 1, "-F", per_mape_output_chain);
	va_cmd("/bin/iptables", 4, 1, "-D", mape_input, "-j", per_mape_input_chain);
	va_cmd("/bin/iptables", 4, 1, "-D", mape_forward, "-j", per_mape_forward_chain);
	va_cmd("/bin/iptables", 4, 1, "-D", mape_output, "-j", per_mape_output_chain);
	va_cmd("/bin/iptables", 4, 1, "-A", mape_input, "-j", per_mape_input_chain);
	va_cmd("/bin/iptables", 4, 1, "-A", mape_forward, "-j", per_mape_forward_chain);
	va_cmd("/bin/iptables", 4, 1, "-A", mape_output, "-j", per_mape_output_chain);

//TCP MSS
	va_cmd("/bin/iptables", 13, 1, "-D", per_mape_forward_chain, "-o", tunnel_name, "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j", "TCPMSS", "--set-mss", "1412");
	va_cmd("/bin/iptables", 13, 1, "-I", per_mape_forward_chain, "-o", tunnel_name, "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j", "TCPMSS", "--set-mss", "1412");

	va_cmd("/bin/iptables", 6, 1, "-A", per_mape_input_chain, "-i", "lo", "-j", "ACCEPT");
	va_cmd("/bin/iptables", 6, 1, "-A", per_mape_output_chain, "-o", "lo", "-j", "ACCEPT");
	
#if defined(CONFIG_00R0)
	char ipfilter_spi_state=1;
	mib_get_s(MIB_IPFILTER_SPI_ENABLE, (void*)&ipfilter_spi_state, sizeof(ipfilter_spi_state));
	if(ipfilter_spi_state != 0)
	{
#endif
		va_cmd("/bin/iptables", 8, 1, "-A", per_mape_input_chain, "-m", "conntrack", "--ctstate", "RELATED,ESTABLISHED", "-j", "ACCEPT");
		va_cmd("/bin/iptables", 8, 1, "-A", per_mape_forward_chain, "-m", "conntrack", "--ctstate", "RELATED,ESTABLISHED", "-j", "ACCEPT");
		va_cmd("/bin/iptables", 8, 1, "-A", per_mape_output_chain, "-m", "conntrack", "--ctstate", "RELATED,ESTABLISHED", "-j", "ACCEPT");
#if defined(CONFIG_00R0)
	}
#endif	

//nat
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-N", mape_prerouting);
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-N", mape_postrouting); 	
	va_cmd("/bin/iptables", 6, 1, "-t", "nat", "-D", "PREROUTING", "-j", mape_prerouting);
	va_cmd("/bin/iptables", 6, 1, "-t", "nat", "-D", "POSTROUTING", "-j", mape_postrouting);	
	va_cmd("/bin/iptables", 6, 1, "-t", "nat", "-A", "PREROUTING", "-j", mape_prerouting);
	va_cmd("/bin/iptables", 6, 1, "-t", "nat", "-A", "POSTROUTING", "-j", mape_postrouting);

	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-N", per_mape_pre_chain);
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-N", per_mape_post_chain);
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-F", per_mape_pre_chain);
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-F", per_mape_post_chain);	
	va_cmd("/bin/iptables", 6, 1, "-t", "nat", "-D", mape_prerouting, "-j", per_mape_pre_chain);
	va_cmd("/bin/iptables", 6, 1, "-t", "nat", "-D", mape_postrouting, "-j", per_mape_post_chain);
	va_cmd("/bin/iptables", 6, 1, "-t", "nat", "-A", mape_prerouting, "-j", per_mape_pre_chain);
	va_cmd("/bin/iptables", 6, 1, "-t", "nat", "-A", mape_postrouting, "-j", per_mape_post_chain);

//parse portsets 
	mape_add_snat_firewall(ifName);

//fmr mark for dst CE with same IPv4 address
	mape_add_local_set_mark_firewall(ifName);
}

static void mape_del_firewall(char *ifName){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	char per_mape_pre_chain[FW_CHAIN_NAME_LEN], per_mape_post_chain[FW_CHAIN_NAME_LEN];
	char per_mape_input_chain[FW_CHAIN_NAME_LEN], per_mape_output_chain[FW_CHAIN_NAME_LEN];
	char per_mape_forward_chain[FW_CHAIN_NAME_LEN];
	
	if (!ifName || (tunnel46_get_tunnel_name(IPV6_MAPE, ifName, tunnel_name) != 1)){	
		TUNNEL46_ERR("(mape) Get mape ifName(%s) tunnel_name failed!", ifName?ifName:"");
		return;
	}

	mape_get_permape_chain(ifName, FW_PREROUTING, per_mape_pre_chain);
	mape_get_permape_chain(ifName, FW_POSTROUTING, per_mape_post_chain);
	mape_get_permape_chain(ifName, FW_FORWARD, per_mape_forward_chain);
	mape_get_permape_chain(ifName, FW_INPUT, per_mape_input_chain);
	mape_get_permape_chain(ifName, FW_OUTPUT, per_mape_output_chain);
		
	//filter
	va_cmd("/bin/iptables", 4, 1, "-D", mape_input, "-j", per_mape_input_chain);
	va_cmd("/bin/iptables", 4, 1, "-D", mape_forward, "-j", per_mape_forward_chain);
	va_cmd("/bin/iptables", 4, 1, "-D", mape_output, "-j", per_mape_output_chain);	
	va_cmd("/bin/iptables", 2, 1, "-F", per_mape_input_chain);
	va_cmd("/bin/iptables", 2, 1, "-F", per_mape_forward_chain);
	va_cmd("/bin/iptables", 2, 1, "-F", per_mape_output_chain);	
	va_cmd("/bin/iptables", 2, 1, "-X", per_mape_input_chain);
	va_cmd("/bin/iptables", 2, 1, "-X", per_mape_forward_chain);
	va_cmd("/bin/iptables", 2, 1, "-X", per_mape_output_chain);

	//nat
	va_cmd("/bin/iptables", 6, 1, "-t", "nat", "-D", mape_prerouting, "-j", per_mape_pre_chain);
	va_cmd("/bin/iptables", 6, 1, "-t", "nat", "-D", mape_postrouting, "-j", per_mape_post_chain);
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-F", per_mape_pre_chain);
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-F", per_mape_post_chain);
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-X", per_mape_pre_chain);
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-X", per_mape_post_chain);

	//LOCAL	
	mape_del_local_set_mark_firewall(ifName);
}

void mape_recover_firewalls(){
	MIB_CE_ATM_VC_T vc_entry;
	char ifname[IFNAMSIZ];
	int i = 0, entry_nums = 0;
	
	entry_nums = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entry_nums; i++)
	{
		if (mib_chain_get(MIB_ATM_VC_TBL, i, &vc_entry) == 0)
			continue;

		if (!vc_entry.enable)
			continue;

		if (vc_entry.IpProtocol != IPVER_IPV6)
			continue;
		
		if (vc_entry.mape_enable && vc_entry.mape_mode == IPV6_MAPE_MODE_STATIC){
			ifGetName(vc_entry.ifIndex, ifname, sizeof(ifname));
			TUNNEL46_DEBUG("(mape) ============recover MAP-E firewall(%s)===============", ifname);
			mape_add_firewall(ifname);
		}

	}
}

void mape_flush_firewalls(){
	FILE *fp = NULL;
	char *table[3] = {"mangle", "filter", "nat"};
	char buf[256] = {0}, chain_name[FW_CHAIN_NAME_LEN]={0};
	int i = 0;

	TUNNEL46_DEBUG("(mape) ============flush MAP-E firewall===============");
	
//filter
	va_cmd(IPTABLES, 4, 1, "-D", "INPUT", "-j", mape_input);
	va_cmd(IPTABLES, 4, 1, "-D", "FORWARD", "-j", mape_forward);
	va_cmd(IPTABLES, 4, 1, "-D", "OUTPUT", "-j", mape_output);	
	va_cmd(IPTABLES, 2, 1, "-F", mape_input);
	va_cmd(IPTABLES, 2, 1, "-F", mape_forward);
	va_cmd(IPTABLES, 2, 1, "-F", mape_output);	
	va_cmd(IPTABLES, 2, 1, "-X", mape_input);
	va_cmd(IPTABLES, 2, 1, "-X", mape_forward);
	va_cmd(IPTABLES, 2, 1, "-X", mape_output);

//nat
	va_cmd(IPTABLES, 6, 1, "-t", "nat", "-D", "PREROUTING", "-j", mape_prerouting);
	va_cmd(IPTABLES, 6, 1, "-t", "nat", "-D", "POSTROUTING", "-j", mape_postrouting);	
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", mape_prerouting);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", mape_postrouting);	
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", mape_prerouting);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", mape_postrouting);
	
//mangle		
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "PREROUTING", "-j", mape_prerouting);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", mape_prerouting);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", mape_prerouting);


	for (i=0; i < 3; i++){
		snprintf(buf, sizeof(buf), "iptables -t %s -S | grep _mape_ | grep \"\\- N\" ", table[i]);
		fp = popen(buf, "r");
		if (fp==NULL){
			TUNNEL46_ERR("(mape) popen iptables(%s) fail!", buf);
		}
		else{
			while(fgets(buf, sizeof(buf), fp)!=NULL){
				sscanf(buf, "-N %s", chain_name);
				va_cmd(IPTABLES, 4, 1, "-t", table[i], "-F", chain_name);
				va_cmd(IPTABLES, 4, 1, "-t", table[i], "-X", chain_name);
			}
			pclose(fp);
		}
	}	
}

static int mape_parse_local_br_addr(char *ifName, char *addr4, char *addr6, char *brAddr6){
	char info_file[64]={0};
	FILE *fp=NULL;	
	char buf[256]={0}, *tmp;
	char flag, offset, ret=1;	

	if (!ifName || !addr4 || !addr6 || !brAddr6)
		return -1;
	
	snprintf(info_file, sizeof(info_file), MAPE_INFO_FILE, ifName);
	if ((fp = fopen(info_file, "r")) == NULL){
		TUNNEL46_ERR("(mape) open %s failed!", info_file);
		return -1;
	}

	while(fgets(buf, sizeof(buf), fp) != NULL){
		flag = 0;
		if(strstr(buf, "IPV4ADDR="))
			flag = 1;
		else if(strstr(buf, "IPV6ADDR="))
			flag = 2;	
		else if(strstr(buf, "BR_ADDR="))
			flag = 3;

		if(flag != 0){
			offset = strstr(buf, "=")-buf+1;
			tmp = buf+offset;
			switch(flag){
			case 1:
				strncpy(addr4, tmp, strlen(tmp)-1);//not include "\n"
				break;
			case 2:
				strncpy(addr6, tmp, strlen(tmp)-1);//not include "\n"
				break;
			case 3:
				strncpy(brAddr6, tmp, strlen(tmp)-1);//not include "\n"
				break;
			default:
				break;
			}
		}
	}

	if(!addr4[0] || !addr6[0] || !brAddr6[0])
		ret = -1;

	fclose(fp);
	return ret;
}

static void mape_save_rules_in_info_file(char *ifName){
	char conf_file[TUNNEL46_FILE_LEN]={0};
	char line[256]={0}, cmd_line[512]={0};
	FILE *fp = NULL;

	if (!ifName)
		return;
	
	if (tunnel46_get_tunnel_conf_file(IPV6_MAPE, ifName, conf_file) != 1)
		return;
	if ((fp = fopen(conf_file, "r")) == NULL)
		return;
	while (fgets(line, sizeof(line), fp)){
		TUNNEL46_DEBUG("(mape) line=%s", line);
	
		if (strstr(line, "BMR_DMR:")){/*BMR + DMR*/
			snprintf(cmd_line, sizeof(cmd_line), "/bin/mapcalc %d %s type=map-e,%s", IPV6_MAPE, ifName, line+strlen("BMR_DMR: "));
			system(cmd_line);
			
			TUNNEL46_DEBUG("(mape) cmd_line=%s", cmd_line);
		}else if (strstr(line, "FMR:")){/*FMR*/
			snprintf(cmd_line, sizeof(cmd_line), "/bin/mapcalc %d %s fmr,%s", IPV6_MAPE, ifName, line+strlen("FMR: "));
			system(cmd_line);
		
			TUNNEL46_DEBUG("(mape) cmd_line=%s", cmd_line);
		}		
	}
	fclose(fp);
}

static int mape_add_multi_wan_policy_route(char *ifName){
	return tunnel46_set_multi_wan_policy_route(IPV6_MAPE, ifName, POLICY_ROUTE_ADD);
}

static int mape_del_multi_wan_policy_route(char *ifName){
	return tunnel46_set_multi_wan_policy_route(IPV6_MAPE, ifName, POLICY_ROUTE_DEL);
}

//Feature: use FMR to handle the case for dest CE of the same IPv4 address with this CE self
static int mape_get_local_policy_route_tbl_id(uint32_t ifindex){
	int tbl_id;

	if( MEDIA_INDEX(ifindex)==MEDIA_ETH ){
		if(PPP_INDEX(ifindex) == DUMMY_PPP_INDEX)
			tbl_id = ETH_INDEX(ifindex) + PMAP_MAPE_NAS_START;
		else
			tbl_id = PPP_INDEX(ifindex) + PMAP_MAPE_PPP_START;
	}
	else
		return -1;

	return tbl_id;
}

static int mape_get_local_policy_route_mark(MIB_CE_ATM_VC_Tp pEntry, unsigned int *mark, unsigned int *mask){
	unsigned int _mark=0, _mask=0;
	char ifName[IFNAMSIZ];
	
	//combine the WAN MARK with MAP-E local MARK as MAP-E policy route 
	ifGetName(pEntry->ifIndex, ifName, sizeof(ifName));
	rtk_net_get_wan_policy_route_mark(ifName, &_mark, &_mask);
	_mark = SOCK_MARK_MAPE_LOCAL_ROUTE_SET(_mark, 1);
	_mask = _mask | SOCK_MARK_MAPE_LOCAL_ROUTE_BIT_MASK;
	
	if(mark) *mark = _mark;
	if(mask) *mask = _mask;
	
	return 1;
}

//Mark to distinguish whether packets are sent to the local or forwarded
static int mape_add_local_set_mark_firewall(char *ifName){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	char tunnel_addr4[16]={0};
	char per_mape_pre_chain[FW_CHAIN_NAME_LEN];
	char conf_file[TUNNEL46_FILE_LEN]={0}, info_file[TUNNEL46_FILE_LEN]={0};
	FILE *fp=NULL;	
	char line[1024]={0};
	int fmr_enabled=0;
	char *ptr, *save_ptr, *tag;	
	MIB_CE_ATM_VC_T vc_entry;
	uint32_t mape_local_mark, mape_local_mask;
	char mape_local_mark_str[32]={0};	
	
	TUNNEL46_DEBUG("(mape) ifName(%s)", ifName);
	
	if (!ifName || (tunnel46_get_tunnel_name(IPV6_MAPE, ifName, tunnel_name) != 1)){	
		TUNNEL46_ERR("(mape) Get mape ifName(%s) tunnel name failed!", ifName?ifName:"");
		return -1;
	}
	
	mape_get_fmr_support_status(ifName, &fmr_enabled); 
	if (fmr_enabled == 0)
		return 1;

	if (mape_get_tunnel_addr4(ifName, tunnel_addr4, sizeof(tunnel_addr4)) != 1 ){		
		TUNNEL46_ERR("(mape) Get mape ifName(%s) tunnel_addr4 failed!", ifName);
		return -1;
	}
	
	if (!(getATMVCEntryByIfName(ifName, &vc_entry))){	
		TUNNEL46_ERR("(mape) Could not get MIB entry info from wanconn %s!", ifName);
		return -1;
	}
	
	if (mape_get_local_policy_route_mark(&vc_entry, &mape_local_mark, &mape_local_mask) != 1){	
		TUNNEL46_ERR("(mape) Error to get MAP-E local policy mark for ifindex(%u)", vc_entry.ifIndex);
		return -1;
	}
	
	snprintf(mape_local_mark_str, sizeof(mape_local_mark_str), "0x%x/0x%x", mape_local_mark, mape_local_mask);
	
	mape_get_permape_chain(ifName, FW_PREROUTING, per_mape_pre_chain);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", mape_prerouting);
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "PREROUTING", "-j", mape_prerouting);
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "PREROUTING", "-j", mape_prerouting);
	
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", per_mape_pre_chain);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", per_mape_pre_chain);

	va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-D", mape_prerouting, "-i", "br0", "-d", tunnel_addr4, "-p", "tcp", "-j", per_mape_pre_chain);
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-D", mape_prerouting, "-i", "br0", "-d", tunnel_addr4, "-p", "udp", "-j", per_mape_pre_chain);
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-D", mape_prerouting, "-i", "br0", "-d", tunnel_addr4, "-p", "icmp", "-j", per_mape_pre_chain);
	
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", mape_prerouting, "-i", "br0", "-d", tunnel_addr4, "-p", "tcp", "-j", per_mape_pre_chain);
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", mape_prerouting, "-i", "br0", "-d", tunnel_addr4, "-p", "udp", "-j", per_mape_pre_chain);
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", mape_prerouting, "-i", "br0", "-d", tunnel_addr4, "-p", "icmp", "-j", per_mape_pre_chain);

	snprintf(info_file, sizeof(info_file), MAPE_INFO_FILE, ifName);
	if ( (fp = fopen(info_file, "r")) == NULL){	
		TUNNEL46_ERR("(mape) open %s failed!", info_file);
		return -1;
	}
	while(fgets(line, sizeof(line), fp) != NULL){
		if(strstr(line, "PORTSETS=")){
			ptr = strtok_r(line+strlen("PORTSETS="), " ", &save_ptr);
			while(ptr){
				if (ptr[strlen(ptr)-1] == '\n'){//remove the charater "\n"
					ptr[strlen(ptr)-1] = '\0';
				}
				if (ptr[0] == '\0')
					break;
				
				tag = strchr(ptr, '-');
				if (tag != NULL)
					*tag = ':';
			
				va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", per_mape_pre_chain, "-p", "tcp", "--destination-port", ptr, "-j", "MARK", "--set-mark", mape_local_mark_str);					
				va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", per_mape_pre_chain, "-p", "udp", "--destination-port", ptr, "-j", "MARK", "--set-mark", mape_local_mark_str);
				ptr = strtok_r(NULL, " ", &save_ptr);
			}
			va_cmd(IPTABLES, 10, 1, "-t", "mangle", "-A", per_mape_pre_chain, "-p", "icmp", "-j", "MARK", "--set-mark", mape_local_mark_str);
			fclose(fp);
			return 1;
		}else
			continue;
	}

	fclose(fp);
	return -1;
}

static int mape_del_local_set_mark_firewall(char *ifName){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	char tunnel_addr4[16] = {0};
	char per_mape_pre_chain[FW_CHAIN_NAME_LEN];
	
	if (!ifName || (tunnel46_get_tunnel_name(IPV6_MAPE, ifName, tunnel_name) != 1)){	
		TUNNEL46_ERR("(mape) Get mape ifName(%s) tunnel_name failed!", ifName?ifName:"");
		return -1;
	}
	
	if (mape_get_tunnel_addr4(ifName, tunnel_addr4, sizeof(tunnel_addr4)) != 1)
		return -1;
	
	mape_get_permape_chain(ifName, FW_PREROUTING, per_mape_pre_chain);
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-D", mape_prerouting, "-i", "br0", "-d", tunnel_addr4, "-p", "tcp", "-j", per_mape_pre_chain);
	va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-D", mape_prerouting, "-i", "br0", "-d", tunnel_addr4, "-p", "udp", "-j", per_mape_pre_chain);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", per_mape_pre_chain);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", per_mape_pre_chain);
	
	return 1;
}

static int mape_set_local_policy_route(char *ifName, int AddOrDelete){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	char tunnel_addr4[16] = {0};
	MIB_CE_ATM_VC_T vc_entry;
	uint32_t local_tbl_id;
	char tbl_id_str[32]={0};
	uint32_t mark=0, mask=0;
	char policy_mark_str[64] = {0}, iprule_pri_policyrt[16] = {0};
	char operate[8]={0};
	int fmr_supported = 0;

	TUNNEL46_DEBUG("(mape) ifName(%s)", ifName);
	
	if (!ifName || (tunnel46_get_tunnel_name(IPV6_MAPE, ifName, tunnel_name) != 1))
		return -1;

	if (AddOrDelete == POLICY_ROUTE_ADD){
		mape_get_fmr_support_status(ifName, &fmr_supported); 
		if (fmr_supported == 0)
			return 1;
		snprintf(operate, sizeof(operate), "%s", "Add");
	}else if (AddOrDelete == POLICY_ROUTE_DEL)
		snprintf(operate, sizeof(operate), "%s", "Del");
	else
		return 0;

	if (!(getATMVCEntryByIfName(ifName, &vc_entry))){
		TUNNEL46_ERR("(mape) Could not get MIB entry info from wanconn %s!", ifName);
		return -1;
	}

	/*policy mark to local (TCP/UDP)*/
	local_tbl_id = mape_get_local_policy_route_tbl_id(vc_entry.ifIndex);
	snprintf(tbl_id_str, sizeof(tbl_id_str), "%d", local_tbl_id);	
	if (mape_get_local_policy_route_mark(&vc_entry, &mark, &mask) != 1){	
		TUNNEL46_ERR("(mape) Error to get MAP-E local policy mark for ifindex(%u)", vc_entry.ifIndex);
		return -1;
	}
	snprintf(policy_mark_str, sizeof(policy_mark_str), "0x%x/0x%x", mark, mask);
	snprintf(iprule_pri_policyrt, sizeof(iprule_pri_policyrt), "%d", IP_RULE_PRI_MAPE_LOCAL);

	TUNNEL46_DEBUG("(mape) (%s) ifName(%s), local_tbl_id=%d, policy_mark_str=%s, iprule_pri_policyrt=%s", 
		operate, ifName, local_tbl_id, policy_mark_str, iprule_pri_policyrt);
	
	va_cmd(BIN_IP, 9, 1, "-4", "rule", "del", "fwmark", policy_mark_str, "pref", iprule_pri_policyrt, "table", tbl_id_str);
	if (AddOrDelete == POLICY_ROUTE_ADD){
		va_cmd(BIN_IP, 9, 1, "-4", "rule", "add", "fwmark", policy_mark_str, "pref", iprule_pri_policyrt, "table", tbl_id_str);
		
		if (mape_get_tunnel_addr4(ifName, tunnel_addr4, sizeof(tunnel_addr4)) != 1){		
			TUNNEL46_ERR("(mape) Get mape ifName(%s) local IPv4 address failed!", ifName);
			return -1;
		}

		//move local route rule from local table to multi_wan policy route table, avoid to trap to INPUT
		va_cmd(BIN_IP, 12, 1, "route", "del", "local", tunnel_addr4, "dev", tunnel_name, "proto", "kernel", "scope", "host", "src", tunnel_addr4); 	
		va_cmd(BIN_IP, 14, 1, "route", "add", "local", tunnel_addr4, "dev", tunnel_name, "proto", "kernel", "scope", "host", "src", tunnel_addr4, "table", tbl_id_str);//wan table
	}
	
	return 1;
}

//Differentiate packets by marking to query different routing tables
static int mape_add_local_policy_route(char *ifName){
	return mape_set_local_policy_route(ifName, POLICY_ROUTE_ADD);
}

static int mape_del_local_policy_route(char *ifName){
	return mape_set_local_policy_route(ifName, POLICY_ROUTE_DEL);
}

#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK

#ifndef SIOCSETPSID
#ifndef SIOCDEVPRIVATE
#define SIOCDEVPRIVATE	0x89F0	/* to 89FF */
#endif
#define SIOCSETPSID		(SIOCDEVPRIVATE + 13) 
#endif

struct mape_psid_info{
	struct in6_addr laddr;
	struct in6_addr raddr;

	__u8 psid_offset;
	__u8 psid_len;
	__u16 psid;
};

static int mape_parse_psid_info(char *ifName, unsigned char *offset, unsigned char *len, unsigned short *psid){
	char conf_file[TUNNEL46_FILE_LEN] = {0};
	FILE *fp = NULL;
	char line[256]={0};
	char *ptr=NULL;
	char key_nums=0;

	if (!ifName || !offset || !psid || !len)
		return -1;

	if (tunnel46_get_tunnel_conf_file(IPV6_MAPE, ifName, conf_file) != 1)
		return -1;

	if ( (fp = fopen(conf_file, "r")) == NULL)
		return -1;

	while(fgets(line, sizeof(line), fp)){
		if (strstr(line, "BMR_DMR:")){
			ptr = strtok(line, ",");
			while(ptr != NULL){
			    if (strstr(ptr, "psid=")){
			        *psid = atoi(ptr+strlen("psid="));
			        key_nums++;
			    }else if (strstr(ptr, "psidlen=")){
			        *len = atoi(ptr+strlen("psidlen="));
			        key_nums++;
			    }else if (strstr(ptr, "offset=")){
			        *offset = atoi(ptr+strlen("offset="));
			        key_nums++;
			    }
			    if (key_nums == 3)
			        break;
			    ptr = strtok(NULL, ",");
			}

			TUNNEL46_DEBUG("(mape) psid=%d, psid_len=%d, psid_offset=%d", *psid, *len, *offset);
		}
	}
	fclose(fp);   
	
	if (key_nums != 3){
		TUNNEL46_ERR("(mape) No psid/psid_len/psid_offset in %s!", conf_file);
		return -1;
	}
	return 1;
}

static int mape_set_psid_to_kernel(char *ifName){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	char tunnel_addr6[48]={0}, br_addr6[48]={0};
	char tunnel_addr4[16]={0};	
	struct in6_addr laddr6, raddr6;
	struct mape_psid_info mape_psid_t;
	struct ifreq ifr;
	int fd, err;

	if (!ifName || (tunnel46_get_tunnel_name(IPV6_MAPE, ifName, tunnel_name) != 1)){
		TUNNEL46_ERR("(mape) Get mape ifName(%s) tunnel_name failed!", ifName?ifName:"");
		return -1;
	}

	memset(&mape_psid_t, 0x00, sizeof(mape_psid_t));
	if (mape_parse_psid_info(ifName, &mape_psid_t.psid_offset, &mape_psid_t.psid_len, &mape_psid_t.psid) != 1){
		return -1;
	}

	if (mape_parse_local_br_addr(ifName, tunnel_addr4, tunnel_addr6, br_addr6) != 1){
		return -1;
	}
	if ((inet_pton(PF_INET6, tunnel_addr6, &laddr6) <= 0) || (inet_pton(PF_INET6, br_addr6, &raddr6) <= 0))
		return -1;    
	memcpy(&mape_psid_t.laddr, &laddr6, sizeof(struct in6_addr));
	memcpy(&mape_psid_t.raddr, &raddr6, sizeof(struct in6_addr));

	fd = socket(AF_INET6, SOCK_DGRAM, 0);
	if (fd < 0) {
		TUNNEL46_ERR("(mape) create socket failed: %s\n", strerror(errno));
		return -1;
	}

	snprintf(ifr.ifr_name, IFNAMSIZ, "%s", tunnel_name);
	ifr.ifr_ifru.ifru_data = &mape_psid_t;

	err = ioctl(fd, SIOCSETPSID, &ifr);
	if (err){
		TUNNEL46_ERR("(mape) ioctl mape psid \"%s\" failed: %s\n", ifr.ifr_name, strerror(errno));
	}

	close(fd);
	return err;
}

#ifdef CONFIG_IPV6_MAPE_IPID_ADJUST
static int mape_del_set_ipid_firewall(char *ifName){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	char per_mape_post_chain[FW_CHAIN_NAME_LEN];
	
	if (!ifName || (tunnel46_get_tunnel_name(IPV6_MAPE, ifName, tunnel_name) != 1)){
		TUNNEL46_ERR("(mape) Get mape ifName(%s) tunnel_name failed!", ifName?ifName:"");
		return -1;
	}

	mape_get_permape_chain(ifName, FW_POSTROUTING, per_mape_post_chain);
	
	va_cmd("/bin/iptables", 8, 1, "-t", "mangle", "-D", mape_postrouting, "-o", tunnel_name, "-j", per_mape_post_chain);
	va_cmd("/bin/iptables", 4, 1, "-t", "mangle", "-F", per_mape_post_chain);
	va_cmd("/bin/iptables", 4, 1, "-t", "mangle", "-X", per_mape_post_chain);
	
	return 1;
}

static int mape_add_set_ipid_firewall(char *ifName){
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	char per_mape_post_chain[FW_CHAIN_NAME_LEN];
	
	if (!ifName || (tunnel46_get_tunnel_name(IPV6_MAPE, ifName, tunnel_name) != 1)){
		TUNNEL46_ERR("(mape) Get mape ifName(%s) tunnel_name failed!", ifName?ifName:"");
		return -1;
	}

	mape_get_permape_chain(ifName, FW_POSTROUTING, per_mape_post_chain);
	
	va_cmd("/bin/iptables", 4, 1, "-t", "mangle", "-N", per_mape_post_chain);
	va_cmd("/bin/iptables", 4, 1, "-t", "mangle", "-F", per_mape_post_chain);
	va_cmd("/bin/iptables", 7, 1, "-t", "mangle", "-A", per_mape_post_chain, "-j", "IPID", "--ipid-mape");

	va_cmd("/bin/iptables", 4, 1, "-t", "mangle", "-N", mape_postrouting);
	va_cmd("/bin/iptables", 8, 1, "-t", "mangle", "-D", mape_postrouting, "-o", tunnel_name, "-j", per_mape_post_chain);
	va_cmd("/bin/iptables", 8, 1, "-t", "mangle", "-A", mape_postrouting, "-o", tunnel_name, "-j", per_mape_post_chain);

	va_cmd("/bin/iptables", 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", mape_postrouting);
	va_cmd("/bin/iptables", 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", mape_postrouting);

	return 1;
}
#endif

#endif

static int mape_setup_tunnel_iface(char *ifName){//char *fmrStr, 
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	char tunnel_conf_file[TUNNEL46_FILE_LEN]={0};
	char tunnel_addr6[48]={0}, br_addr6[48]={0};
	char tunnel_addr4[16]={0};	
	char addr6_prefix[48]={0}, addr4_prefix[16]={0};
	int addr6_prefix_len, addr4_prefix_len, ea_len, psid_offset;
	FILE *fp = NULL;
	char rule[256]={0}, line[256]={0};
	char *cmd_buf=NULL, *ptr=NULL;
	int buf_len=0;	
	MIB_CE_ATM_VC_T vc_entry;
	
	if (!ifName || (tunnel46_get_tunnel_name(IPV6_MAPE, ifName, tunnel_name) != 1))
		return -1;

	/*parse local v4/v6 addr and br v6 addr from /tmp/mape-interface.info file*/
	if (mape_parse_local_br_addr(ifName, tunnel_addr4, tunnel_addr6, br_addr6) < 0){
		TUNNEL46_ERR("(mape) Parse local IPv4/IPv6 addr and BR IPv6 addr failed! (ifName=%s)", ifName);
		return -1;
	}

	/*create mape tunnel*/
	snprintf(rule, sizeof(rule), "ip -6 tunnel add %s mode ipip6 remote %s local %s dev %s encaplimit none",
			tunnel_name, br_addr6, tunnel_addr6, ifName);
	buf_len = strlen(rule)+1;
	cmd_buf = (char *)malloc(buf_len);
	if (cmd_buf == NULL)
		return -1;
	snprintf(cmd_buf, buf_len, "%s", rule);

	if (tunnel46_get_tunnel_conf_file(IPV6_MAPE, ifName, tunnel_conf_file) != 1)
		return -1;
	if ( (fp = fopen(tunnel_conf_file, "r")) == NULL){
		system(cmd_buf);
		free(cmd_buf);
		return -1;
	}

	/*FMR*/
	while(fgets(line, sizeof(line), fp)){
		if (strstr(line, MAPE_FMR_DISABLED_INFO)){
			break;
		}else if (strstr(line, "FMR: ")){	
			TUNNEL46_DEBUG("(mape) line=%s",line);
			ptr = strtok(line+strlen("FMR: "), ",");
			while(ptr != NULL){
				if (strstr(ptr, "ipv6prefix=")){
					snprintf(addr6_prefix, sizeof(addr6_prefix), "%s", ptr+strlen("ipv6prefix="));
				}else if (strstr(ptr, "prefix6len=")){
					addr6_prefix_len = atoi(ptr+strlen("prefix6len="));
				}else if (strstr(ptr, "ipv4prefix=")){
					snprintf(addr4_prefix, sizeof(addr4_prefix), "%s", ptr+strlen("ipv4prefix="));
				}else if (strstr(ptr, "prefix4len=")){
					addr4_prefix_len = atoi(ptr+strlen("prefix4len="));
				}else if (strstr(ptr, "ealen=")){
					ea_len = atoi(ptr+strlen("ealen="));
				}else if (strstr(ptr, "offset=")){
					psid_offset = atoi(ptr+strlen("offset="));
				}

				ptr = strtok(NULL, ",");
			}
			
			TUNNEL46_DEBUG(MAPE_FMR_INFO,addr6_prefix,addr6_prefix_len,addr4_prefix,addr4_prefix_len,ea_len,psid_offset);
			
			snprintf(rule, sizeof(rule), " fmrs %s/%d,%s/%d,%d,%d", 
				addr6_prefix, addr6_prefix_len, addr4_prefix, addr4_prefix_len, ea_len, psid_offset);
			
			buf_len += strlen(rule);
			cmd_buf = (char *)realloc(cmd_buf, buf_len);
			strcat(cmd_buf, rule);
			cmd_buf[buf_len-1] = '\0';
		}
	}
	TUNNEL46_DEBUG("(mape) cmd_buf=%s", cmd_buf);
	system(cmd_buf);
	free(cmd_buf);		
	fclose(fp);
	
	snprintf(rule, sizeof(rule), "echo 0 > /proc/sys/net/ipv6/conf/%s/disable_ipv6", tunnel_name);
	system(rule);

	va_cmd("/usr/bin/ip", 6, 1, "-4", "addr", "add", tunnel_addr4, "dev", tunnel_name);	
	sleep(2);
	va_cmd("/usr/bin/ip", 6, 1, "-6", "addr", "add", tunnel_addr6, "dev", tunnel_name);
	
	va_cmd("/usr/bin/ip", 5, 1, "link", "set", "dev", tunnel_name, "up") ;	

	//Set the default route in main table to the last set MAP-E interface
	if (!(getATMVCEntryByIfName(ifName, &vc_entry))){	
		TUNNEL46_ERR("(mape) Could not get MIB entry info from wanconn (ifName=%s)", ifName);
		return -1;
	}

	if (isDefaultRouteWan(&vc_entry)){
		va_cmd("/usr/bin/ip", 3, 1, "route", "delete", "default"); 
		va_cmd("/usr/bin/ip", 5, 1, "route", "add", "default", "dev", tunnel_name); //needed by packets from DUT self 
	}

	va_cmd("/usr/bin/ip", 6, 1, "-6", "route", "add", br_addr6, "dev", ifName);
	return 1;
}

static void mape_del_tunnel_iface(char *ifName){
	tunnel46_del_tunnel_iface(IPV6_MAPE, ifName);
}

void mape_setup_tunnel(char *ifName){	
	if (!ifName)
		return;

	/*Generate mape tunnel ipv6 address and portset in /tmp/mape-%s.info*/
	mape_save_rules_in_info_file(ifName);

	/*setup map-e tunnel interface*/
	mape_setup_tunnel_iface(ifName);

	/*set PSID info to kernel*/
#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
	mape_set_psid_to_kernel(ifName);
#endif

	/*set snat filrewall rules*/
	mape_add_firewall(ifName);

	/*set mod ipid firewall*/
#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
#ifdef CONFIG_IPV6_MAPE_IPID_ADJUST
	mape_add_set_ipid_firewall(ifName);
#endif
#endif

	/*set policy route for multi_wan*/
	mape_add_multi_wan_policy_route(ifName);
	
	/*set policy route for LOCAL of dest CE of same IPv4 addr with this CE*/
	mape_add_local_policy_route(ifName);		
}

static int mape_stop_tunnel(char *ifName){
	if (!ifName || (is_tunnel46_conf_file_exsit(IPV6_MAPE, ifName) != 1))
		return -1;

	/*delete mape firewalls*/
	mape_del_firewall(ifName);

	/*delete mod ipid firewall*/
#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
#ifdef CONFIG_IPV6_MAPE_IPID_ADJUST
	mape_del_set_ipid_firewall(ifName);
#endif
#endif

	/*delete mape policy route for multi_wan*/
	mape_del_multi_wan_policy_route(ifName);

	/*delete mape policy route for LOCAL*/
	mape_del_local_policy_route(ifName);

	/*delete mape interface and some files*/
	mape_del_tunnel_iface(ifName);

	return 1;
}

int mape_gen_static_mode_parameter(char *ifName){
	char addr6_prefix[48] = {0}, br_addr6[48] = {0};
	char *addr4_str = NULL;
	int ea_len;
	char wan_iface[IFNAMSIZ]={0};
	MIB_CE_ATM_VC_T vc_entry = {0};
	MIB_CE_ATM_VC_Tp pEntry = &vc_entry;
	MIB_CE_MAPE_FMRS_T fmr_entry;
	int fmr_nums, idx = 0;
	char tmp_buf[256] = {0};	
	FILE *fp = NULL;
		
	TUNNEL46_DEBUG("(mape) ifName(%s)", ifName);

	if (!ifName)
		return -1;
	
	memcpy(wan_iface, ifName, strlen(ifName));
	if (getWanEntrybyIfname(wan_iface, &vc_entry, &idx) < 0)
		return -1;

	if (tunnel46_get_tunnel_conf_file(IPV6_MAPE, ifName, tmp_buf) != 1)
		return -1;	
	if ( (fp = (FILE *)fopen(tmp_buf, "w")) == NULL){
		TUNNEL46_ERR("(mape) open %s failed!", tmp_buf);
		return -1;
	}
	
	inet_ntop(PF_INET6, pEntry->mape_local_v6Prefix, addr6_prefix, sizeof(addr6_prefix));
	inet_ntop(PF_INET6, pEntry->mape_remote_v6Addr, br_addr6, sizeof(br_addr6));
	addr4_str = inet_ntoa(*((struct in_addr *)&pEntry->mape_local_v4Addr));
	ea_len = (32-pEntry->mape_local_v4MaskLen) + pEntry->mape_psid_len;
	
	//BMR_DMR
	snprintf(tmp_buf, sizeof(tmp_buf), MAPE_BMR_DMR_INFO, 
		addr6_prefix, pEntry->mape_local_v6PrefixLen, addr4_str, pEntry->mape_local_v4MaskLen, 
		pEntry->mape_psid, pEntry->mape_psid_len, pEntry->mape_psid_offset, ea_len, br_addr6);
	fputs(tmp_buf, fp);

	TUNNEL46_DEBUG("(mape) tmp_buf=%s", tmp_buf);

	//FMR
	if (pEntry->mape_fmr_enabled){
		fputs(MAPE_FMR_ENABLED_INFO, fp);
		
		snprintf(tmp_buf, sizeof(tmp_buf), MAPE_FMR_INFO, 
			addr6_prefix, pEntry->mape_local_v6PrefixLen, addr4_str, 
			pEntry->mape_local_v4MaskLen, ea_len, pEntry->mape_psid_offset);
		fputs(tmp_buf, fp);
		TUNNEL46_DEBUG("(mape) tmp_buf=%s", tmp_buf);
		
		fmr_nums = mib_chain_total(MIB_MAPE_FMRS_TBL);
		for(idx = 0; idx < fmr_nums; idx++){
			if (!mib_chain_get(MIB_MAPE_FMRS_TBL, idx, (void *)&fmr_entry)){
				fclose(fp);
				return -1;
			}
					
			if (fmr_entry.mape_fmr_extIf != pEntry->ifIndex)
				continue;
			
			inet_ntop(PF_INET6, fmr_entry.mape_fmr_v6Prefix, addr6_prefix, sizeof(addr6_prefix));
			addr4_str = inet_ntoa(*((struct in_addr *)&fmr_entry.mape_fmr_v4Prefix));
			snprintf(tmp_buf, sizeof(tmp_buf), MAPE_FMR_INFO, 
				addr6_prefix, fmr_entry.mape_fmr_v6PrefixLen, addr4_str, 
				fmr_entry.mape_fmr_v4MaskLen, fmr_entry.mape_fmr_eaLen, fmr_entry.mape_fmr_psidOffset);
			fputs(tmp_buf, fp);
		
			TUNNEL46_DEBUG("(mape) tmp_buf=%s", tmp_buf);
		}
		
	}else{
		fputs(MAPE_FMR_DISABLED_INFO, fp);
	}

	fclose(fp);
	return 1;
}

int mape_gen_dhcp_mode_parameter(char *ifName, char *extra_info){
	DLG_INFO_T dlgInfo ;
	FILE *fp=NULL ;
	char ipv4addr[INET_ADDRSTRLEN]={0};
	char ipv6prefix[INET6_ADDRSTRLEN]={0};
	char br_addr[INET6_ADDRSTRLEN]={0};
	char leasefile[64]={0};
	char conf_file[TUNNEL46_FILE_LEN]={0};
	char line[256] = {0};	

	snprintf(leasefile, sizeof(leasefile), "%s.%s", PATH_DHCLIENT6_LEASE, ifName);

	/* Checking leasefile for MAP-E */
	if (getLeasesInfo(leasefile,&dlgInfo) & LEASE_GET_MAPE) {
		if (tunnel46_get_tunnel_conf_file(IPV6_MAPE, ifName, conf_file) != 1)
			return -1;	
		
		if ( (fp = (FILE *)fopen(conf_file, "w")) == NULL){
			TUNNEL46_ERR("(mape) open %s failed!", conf_file);
			return -1;
		}

		//BMR_DMR
		inet_ntop(PF_INET, &dlgInfo.map_info.s46_rule.map_addr, ipv4addr, sizeof(ipv4addr));
		inet_ntop(PF_INET6, &dlgInfo.map_info.s46_rule.map_addr6, ipv6prefix, sizeof(ipv6prefix));
		inet_ntop(PF_INET6, &dlgInfo.map_info.s46_br.mape_br_addr, br_addr, sizeof(br_addr));
		snprintf(line, sizeof(line), MAPE_BMR_DMR_INFO, 
			ipv6prefix, dlgInfo.map_info.s46_rule.v6_prefix_len, ipv4addr, dlgInfo.map_info.s46_rule.v4_prefix_len,
			dlgInfo.map_info.s46_portparams.psid_val, dlgInfo.map_info.s46_portparams.psid_len, dlgInfo.map_info.s46_portparams.psid_offset, dlgInfo.map_info.s46_rule.ea_len, br_addr);
		fputs(line, fp);
		TUNNEL46_DEBUG("(mape) line=%s", line);
		
		//FMR
		if (dlgInfo.map_info.s46_rule.ea_flag){
			fputs(MAPE_FMR_ENABLED_INFO, fp);
		
			snprintf(line, sizeof(line), MAPE_FMR_INFO, 
				ipv6prefix, dlgInfo.map_info.s46_rule.v6_prefix_len, ipv4addr, dlgInfo.map_info.s46_rule.v4_prefix_len,
				dlgInfo.map_info.s46_rule.ea_len, dlgInfo.map_info.s46_portparams.psid_offset);
			fputs(line, fp);
		}else{
			fputs(MAPE_FMR_DISABLED_INFO, fp);
		}
		
		fclose(fp);
		return 1;
	}else
		return 0;
}

pthread_mutex_t mape_mutex;
void mape_mutex_init(void)
{
	pthread_mutex_init(&mape_mutex, NULL);
}
int mape_mutex_lock(void)
{
	if (pthread_mutex_lock(&mape_mutex))
		return 0;	
	return -1;
}
int mape_mutex_unlock(void)
{
	pthread_mutex_unlock(&mape_mutex);
	return 0;	
}

static pthread_t cur_mape_ptid;

/*
 *	pthread to resetup MAP-E
 */
static void *pt_mape_setup(void *ifName)
{
	pthread_t my_ptid = pthread_self();
	char wan_iface[IFNAMSIZ]={0};

	memcpy(wan_iface, ifName, strlen(ifName));
	free(ifName);
	
	if (my_ptid != cur_mape_ptid) 
		return NULL;
	
	mape_mutex_lock();
	mape_stop_tunnel(wan_iface);
	mape_setup_tunnel(wan_iface);	
	mape_mutex_unlock();
	return NULL;
}

static int do_mape(char *ifName){
	pthread_t ptid;
	char *wan_name = (char *)malloc(IFNAMSIZ);
	if (wan_name == NULL)
		return -1;
	
	memset(wan_name, 0x00, IFNAMSIZ);
	snprintf(wan_name, IFNAMSIZ, "%s", ifName);
	//printf("[%s:%d] wan_name=%s(%p), strlen(wan_name)=%d\n",__FUNCTION__,__LINE__,wan_name,wan_name,strlen(wan_name));
	if (pthread_create(&ptid, NULL, pt_mape_setup, (void *)wan_name)==0)
		cur_mape_ptid = ptid;
	else{
		free(wan_name);
		return -1;
	}
	
	pthread_detach(ptid);
	return 0;
}
#endif

#ifdef CONFIG_USER_MAP_T
const char MAPT_CONFIG[] = "/etc/scripts/map-t.sh";
#define MAPT_DEV "mapt"

static char mapt_input[FW_CHAIN_NAME_LEN]="mapt_input";
static char mapt_forward[FW_CHAIN_NAME_LEN]="mapt_forward";
static char mapt_output[FW_CHAIN_NAME_LEN]="mapt_output";
static char mapt_prerouting[FW_CHAIN_NAME_LEN]="mapt_prerouting";
static char mapt_postrouting[FW_CHAIN_NAME_LEN]="mapt_postrouting";

static int get_mapt_subnet(unsigned char*ipaddr,unsigned char prefixlen, unsigned char*subnet)
{
	unsigned char idx,i;
	unsigned char offset;
	idx = prefixlen/8;
	offset = prefixlen%8;
	for(i=0;i<idx;i++)
	{
		*(subnet+i) = *(ipaddr+i);
	}
	if(offset>0)
	{
		*(subnet+idx) = (((*(ipaddr+idx))>>(8-offset))<<(8-offset));
	}
	return 0;
}

static int config_mapt_route(char act)
{
	char map_dev[16]={0};
	char ifname[16];
	char tableid[16];
	char gateway[64]={0};
	unsigned char zero_ip[IP6_ADDR_LEN]={0};
	unsigned int ip_rule_table_id=0;
	MIB_CE_ATM_VC_T vcEntry;
	int vcTotal=0, i=0;

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < vcTotal; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
			return -1;
		if(vcEntry.mapt_enable){
			ip_rule_table_id=2000+ETH_INDEX(vcEntry.ifIndex);
			ifGetName(vcEntry.ifIndex,ifname,sizeof(ifname));
			snprintf(map_dev,sizeof(map_dev),"%s_nas0_%d",MAPT_DEV,ETH_INDEX(vcEntry.ifIndex));
			snprintf(tableid,sizeof(tableid),"%d",ip_rule_table_id);
			va_cmd("/usr/bin/ip",5,1,"-6", "rule", "del", "pref", tableid);
			va_cmd("/usr/bin/ip",5,1,"-6", "route", "flush", "table", tableid);
			va_cmd("/usr/bin/ip",5,1, "route", "del", "default", "dev", map_dev);
			if(act==1)//add
			{
				va_cmd("/usr/bin/ip",9,1, "-6", "rule", "add", "iif",map_dev,"table",tableid,"pref",tableid);
				inet_ntop(AF_INET6,vcEntry.RemoteIpv6Addr,gateway,sizeof(gateway));
				if(memcmp(gateway,zero_ip,IP6_ADDR_LEN) == 0)
					return -1;
				va_cmd("/usr/bin/ip",10,1, "-6", "route", "add", "default", "via",gateway,"dev",ifname,"table",tableid);
				va_cmd("/usr/bin/ip",5,1, "route", "add", "default","dev", map_dev);
			}
		}
	}
	return 0;
}

#if 1
static void mapt_add_firewall(char *ifname){
	//TCP MSS
	va_cmd("/bin/iptables", 11, 1, "-A", "FORWARD", "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j", "TCPMSS", "--set-mss", "1412");

	//va_cmd("/bin/ip6tables", 3, 1, "-P", "INPUT", "ACCEPT");

	//filter
	va_cmd("/bin/iptables", 2, 1, "-N", mapt_input );
	va_cmd("/bin/iptables", 2, 1, "-N", mapt_forward );
	va_cmd("/bin/iptables", 2, 1, "-N", mapt_output );

	va_cmd("/bin/iptables", 4, 1, "-A", "INPUT", "-j", mapt_input );
	va_cmd("/bin/iptables", 4, 1, "-A", "FORWARD", "-j", mapt_forward );
	va_cmd("/bin/iptables", 4, 1, "-A", "OUTPUT", "-j", mapt_output );

	va_cmd("/bin/iptables", 6, 1, "-A", mapt_input, "-i", "lo", "-j", "ACCEPT");
	va_cmd("/bin/iptables", 6, 1, "-A", mapt_output, "-o", "lo", "-j", "ACCEPT");
#if defined(CONFIG_00R0)
	char ipfilter_spi_state=1;
	mib_get_s(MIB_IPFILTER_SPI_ENABLE, (void*)&ipfilter_spi_state, sizeof(ipfilter_spi_state));
	if(ipfilter_spi_state != 0)
	{
#endif
	va_cmd("/bin/iptables", 8, 1, "-A", mapt_input, "-m", "conntrack", "--ctstate", "RELATED,ESTABLISHED", "-j", "ACCEPT");
	va_cmd("/bin/iptables", 8, 1, "-A", mapt_forward, "-m", "conntrack", "--ctstate", "RELATED,ESTABLISHED", "-j", "ACCEPT");
	va_cmd("/bin/iptables", 8, 1, "-A", mapt_output, "-m", "conntrack", "--ctstate", "RELATED,ESTABLISHED", "-j", "ACCEPT");
#if defined(CONFIG_00R0)
	}
#endif

	/* NAT postrouing chain is added by map-t.sh */
#if 0
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-N", mapt_prerouting );
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-N", mapt_postrouting );

	va_cmd("/bin/iptables", 6, 1, "-t", "nat", "-A", "PREROUTING", "-j", mapt_prerouting );
	va_cmd("/bin/iptables", 6, 1, "-t", "nat", "-A", "POSTROUTING", "-j", mapt_postrouting );
#endif
}

static void mapt_del_firewall(char *ifname){
	//get map type is map-e or map-t
//	get_map_type_name(map_type);

	//TCP MSS
	va_cmd("/bin/iptables", 11, 1, "-D", "FORWARD", "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j", "TCPMSS", "--set-mss", "1412");

	//filter
	va_cmd("/bin/iptables", 2, 1, "-F", mapt_input);
	va_cmd("/bin/iptables", 2, 1, "-F", mapt_forward);
	va_cmd("/bin/iptables", 2, 1, "-F", mapt_output);

	va_cmd("/bin/iptables", 4, 1, "-D", "INPUT", "-j", mapt_input);
	va_cmd("/bin/iptables", 4, 1, "-D", "FORWARD", "-j", mapt_forward);
	va_cmd("/bin/iptables", 4, 1, "-D", "OUTPUT", "-j", mapt_output);

	va_cmd("/bin/iptables", 2, 1, "-X", mapt_input);
	va_cmd("/bin/iptables", 2, 1, "-X", mapt_forward);
	va_cmd("/bin/iptables", 2, 1, "-X", mapt_output);

	//nat
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-F", mapt_prerouting);
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-F", mapt_postrouting);

	va_cmd("/bin/iptables", 6, 1, "-t", "nat", "-D", "PREROUTING", "-j", mapt_prerouting);
	va_cmd("/bin/iptables", 6, 1, "-t", "nat", "-D", "POSTROUTING", "-j", mapt_postrouting);

	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-X", mapt_prerouting);
	va_cmd("/bin/iptables", 4, 1, "-t", "nat", "-X", mapt_postrouting);
}
#endif

/* 1. Use mapcalc to calculate essential mapping rule and save in /tmp/mape(t)-ifname.info
   2. In map-e: use mape-ifname.info to setup tunnel.
   3. In map-t: use nat46 script to setup interface. */
static void mapt_setup_tunnel_iface(char *ifName){//char *fmrStr, 
	char conf_file[TUNNEL46_FILE_LEN]={0};
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	char rule[256]={0}, cmd_buf[256]={0};
	FILE *fp=NULL;
	int wan_mtu=0;
	char *argv[8];
	char sValue[8];

	if (tunnel46_get_tunnel_conf_file(IPV6_MAPT, ifName, conf_file) != 1)
		return;
	if ((fp = fopen(conf_file, "r")) == NULL)
		return;
	if (fgets(rule, sizeof(rule), fp)){
		snprintf(cmd_buf, sizeof(cmd_buf), "/bin/mapcalc %d %s %s", IPV6_MAPT, ifName, rule);
		system(cmd_buf);
	}
	fclose(fp);

	snprintf(cmd_buf,sizeof(cmd_buf),"%s add mapt %s",MAPT_CONFIG, ifName);
	system(cmd_buf);

	/* Set MTU with WAN interface minus 20 (difference of header of ipv4 and ipv6) for map-t
	 * instead of 16384 set by nat46 module. */
	if(tunnel46_get_tunnel_name(IPV6_MAPT, ifName, tunnel_name)!=1)
		TUNNEL46_ERR("Get mapt ifName(%s) tunnel_name failed!", ifName?ifName:"");

	if (getInMTU((const char *)ifName, &wan_mtu)){
		snprintf(sValue, sizeof(sValue), "%d", wan_mtu-20);
		va_cmd(IFCONFIG, 3, 1, tunnel_name, "mtu", sValue);
	}
}

static void mapt_del_tunnel_iface(char *ifName){
	char cmd[128]={0};
	snprintf(cmd,sizeof(cmd),"%s del mapt %s", MAPT_CONFIG, ifName);
	system(cmd);
}

static void mapt_setup_tunnel(char *ifName){	
	if (!ifName)
		return;

	/*Disable hw fastpath on 98D platform*/
	#if defined(CONFIG_RTK_SOC_RTL8198D)
	system("echo 2 > /proc/fc/ctrl/hwnat");
	#endif

	/*setup map-e tunnel interface*/
	mapt_setup_tunnel_iface(ifName);

	config_mapt_route(1);

	/*set snat filrewall rules*/
	mapt_add_firewall(ifName);
}

static int mapt_stop_tunnel(char *ifName){
	if (!ifName || (is_tunnel46_conf_file_exsit(IPV6_MAPT, ifName) != 1))
		return -1;

	/*Enable hw fastpath on 98D platform*/
	#if defined(CONFIG_RTK_SOC_RTL8198D)
	system("echo 1 > /proc/fc/ctrl/hwnat");
	#endif

	/*delete mape firewalls*/
	//mapt_del_firewall(ifName);
	
	/*delete mape interface and some files*/
	mapt_del_tunnel_iface(ifName);

	config_mapt_route(0);
	return 1;
}

static int mapt_gen_static_mode_parameter(char *ifName){
	char wan_iface[IFNAMSIZ]={0};
	MIB_CE_ATM_VC_T vc_entry = {0};
	MIB_CE_ATM_VC_Tp pEntry = &vc_entry;
	char addr6PrefixStr[48] = {0}, brAddr6Str[48] = {0};
	char *add4Str = NULL;
	int eaLen, idx;	
	char local_v4prefix[64]={0};
	unsigned char prefixaddr[IP6_ADDR_LEN];
	char addrStr[64]={0}, tmp_buf[256] = {0};
	FILE *fp = NULL;

	if (!ifName)
		return -1;
	
	memcpy(wan_iface, ifName, strlen(ifName));
	if (getWanEntrybyIfname(wan_iface, &vc_entry, &idx) < 0)
		return -1;

	if (tunnel46_get_tunnel_conf_file(IPV6_MAPT, ifName, tmp_buf) != 1)
		return -1;	
	if ( (fp = (FILE *)fopen(tmp_buf, "w")) == NULL){
		TUNNEL46_ERR("(mapt) open %s failed!", tmp_buf);
		return -1;
	}
	inet_ntop(PF_INET6, pEntry->mapt_local_v6Prefix, addr6PrefixStr, sizeof(addr6PrefixStr));
	inet_ntop(PF_INET6, pEntry->mapt_remote_v6Prefix, brAddr6Str, sizeof(brAddr6Str));
	add4Str = inet_ntoa(*((struct in_addr *)&pEntry->mapt_local_v4Addr));
	eaLen = (32-pEntry->mapt_local_v4MaskLen) + pEntry->mapt_psid_len;

	/* Get ipv4 mask addr for nat46 use */
	memset(prefixaddr,0,sizeof(prefixaddr));
	get_mapt_subnet(pEntry->mapt_local_v4Addr,pEntry->mapt_local_v4MaskLen,prefixaddr);
	inet_ntop(AF_INET, prefixaddr, addrStr, sizeof(addrStr));
	snprintf(local_v4prefix, sizeof(local_v4prefix), "%s",addrStr);

	snprintf(tmp_buf, sizeof(tmp_buf),
		"type=map-t,ipv6prefix=%s,prefix6len=%d,ipv4prefix=%s,prefix4len=%d,ipv4prefixstr=%s,psid=%d,psidlen=%d,offset=%d,ealen=%d,dmr=%s,dmrprefixlen=%d",
		addr6PrefixStr, pEntry->mapt_local_v6PrefixLen, add4Str, pEntry->mapt_local_v4MaskLen,
		local_v4prefix, pEntry->mapt_psid, pEntry->mapt_psid_len, pEntry->mapt_psid_offset, eaLen,
		brAddr6Str, pEntry->mapt_remote_v6PrefixLen);

	fputs(tmp_buf, fp);
	fclose(fp);
	return 1;
}

static int mapt_gen_dhcp_mode_parameter(char *ifName, char *extra_info){
	DLG_INFO_T dlgInfo ;
	FILE *fp=NULL ;
	char ipv4addr[INET_ADDRSTRLEN]={0};
	char ipv6prefix[INET6_ADDRSTRLEN]={0};
	char dmr_prefix[INET6_ADDRSTRLEN]={0};
	char mapt_parameter[256]={0};
	char leasefile[64]={0};
	char conf_file[TUNNEL46_FILE_LEN]={0};
	char addr_prefix_str[INET_ADDRSTRLEN]={0};

	snprintf(leasefile, sizeof(leasefile), "%s.%s", PATH_DHCLIENT6_LEASE, ifName);

	/* Checking leasefile for MAP-T */
	if (getLeasesInfo(leasefile,&dlgInfo) & LEASE_GET_MAPT) {
		inet_ntop(PF_INET, &dlgInfo.map_info.s46_rule.map_addr, ipv4addr, sizeof(ipv4addr));
		inet_ntop(PF_INET, &dlgInfo.map_info.s46_rule.map_addr_subnet, addr_prefix_str, sizeof(addr_prefix_str));
		inet_ntop(PF_INET6, &dlgInfo.map_info.s46_rule.map_addr6, ipv6prefix, sizeof(ipv6prefix));
		inet_ntop(PF_INET6, &dlgInfo.map_info.s46_dmr.mapt_dmr_prefix, dmr_prefix, sizeof(dmr_prefix));

		/* Make map-t config string */
		snprintf(mapt_parameter,sizeof(mapt_parameter),"type=map-t,ipv6prefix=%s,prefix6len=%d,ipv4prefix=%s,prefix4len=%d,ipv4prefixstr=%s,psid=%d,psidlen=%d,offset=%d,ealen=%d,dmr=%s,dmrprefixlen=%d"
		,ipv6prefix,dlgInfo.map_info.s46_rule.v6_prefix_len,ipv4addr,dlgInfo.map_info.s46_rule.v4_prefix_len,addr_prefix_str,dlgInfo.map_info.s46_portparams.psid_val,dlgInfo.map_info.s46_portparams.psid_len,dlgInfo.map_info.s46_portparams.psid_offset,dlgInfo.map_info.s46_rule.ea_len,dmr_prefix,dlgInfo.map_info.s46_dmr.dmr_prefix_len);

		if (tunnel46_get_tunnel_conf_file(IPV6_MAPT, ifName, conf_file) != 1)
			return -1;	
		
		if ( (fp = (FILE *)fopen(conf_file, "w")) == NULL){
			TUNNEL46_ERR("(mapt) open %s failed!", conf_file);
			return -1;
		}
		fputs(mapt_parameter, fp);
		fclose(fp);
		return 1;
	}else
		return 0;
}

static pthread_t cur_mapt_ptid;
static void *pt_mapt_setup(void *ifName)
{
	pthread_t my_ptid = pthread_self();
	char wan_iface[IFNAMSIZ]={0};

	memcpy(wan_iface, ifName, strlen(ifName));
	free(ifName);
	
	if (my_ptid != cur_mapt_ptid) 
		return NULL;
	
	mapt_stop_tunnel(wan_iface);
	mapt_setup_tunnel(wan_iface);	
	return NULL;
}

static int do_mapt(char *ifName){
	pthread_t ptid;
	char *wan_name = (char *)malloc(IFNAMSIZ);
	if (wan_name == NULL)
		return -1;
	
	memset(wan_name, 0x00, IFNAMSIZ);
	snprintf(wan_name, IFNAMSIZ, "%s", ifName);
	if (pthread_create(&ptid, NULL, pt_mapt_setup, (void *)wan_name)==0)
		cur_mapt_ptid = ptid;
	else{
		free(wan_name);
		return -1;
	}
	
	pthread_detach(ptid);
	return 0;
}
#endif

#ifdef CONFIG_USER_XLAT464
#define XLAT464_CLAT_PLAT_INFO "CLAT_PREFIX6=%s\nPLAT_PREFIX6=%s\n"
#define XLAT464_CLAT_INFO "CLAT_PREFIX6="
#define XLAT464_PLAT_INFO "PLAT_PREFIX6="
#define XLAT464_NAT46_MODULE "/lib/modules/nat46.ko"
#define XLAT464_NAT46_FILE "/proc/net/nat46/control"

static char xlat464_input[FW_CHAIN_NAME_LEN]="xlat464_input";
static char xlat464_forward[FW_CHAIN_NAME_LEN]="xlat464_forward";
static char xlat464_output[FW_CHAIN_NAME_LEN]="xlat464_output";
static char xlat464_prerouting[FW_CHAIN_NAME_LEN]="xlat464_prerouting";
static char xlat464_postrouting[FW_CHAIN_NAME_LEN]="xlat464_postrouting";

static pthread_t cur_xlat464_ptid;

static int xlat464_clat_static_mode_parameter(char *ifName){
	MIB_CE_ATM_VC_T vc_entry={0};
	char conf_file[TUNNEL46_FILE_LEN]={0};
	FILE *file_xlat464 = NULL;
	unsigned char clatprefix6[INET6_ADDRSTRLEN]={0};
	unsigned char platprefix6[INET6_ADDRSTRLEN]={0};

	TUNNEL46_DEBUG("(xlat464_static) ifName(%s)", ifName);

	if(!ifName)
		return -1;

	if(getATMVCEntryByIfName(ifName, &vc_entry)== NULL)
		return -1;

	if(tunnel46_get_tunnel_conf_file(IPV6_XLAT464, ifName, conf_file)!= 1)
		return -1;

	if((file_xlat464 = fopen(conf_file, "w")) == NULL){
		TUNNEL46_ERR("(xlat464_static) open %s failed!", conf_file);
		return -1;
	}

	//CLAT: MIB
	inet_ntop(PF_INET6, vc_entry.xlat464_clat_prefix6, clatprefix6, sizeof(clatprefix6));

	//PLAT: MIB
	inet_ntop(PF_INET6, vc_entry.xlat464_plat_prefix6, platprefix6, sizeof(platprefix6));

	snprintf(conf_file, sizeof(conf_file), XLAT464_CLAT_PLAT_INFO, clatprefix6, platprefix6);
	fputs(conf_file, file_xlat464);
	TUNNEL46_DEBUG("(xlat464_static) conf_file=%s", conf_file);
	fclose(file_xlat464);

	return 1 ;
}

static int xlat464_clat_dhcp_mode_parameter(char *ifName, char *extra_info){
	DLG_INFO_T dlgInfo ={0};
	char leasefile[64]={0};
	char conf_file[TUNNEL46_FILE_LEN]={0};
	char clatprefix6[INET6_ADDRSTRLEN]={0};
	char platprefix6[INET6_ADDRSTRLEN]={0};
	FILE *file_xlat464=NULL;
	MIB_CE_ATM_VC_T Entry={0};

	if(!ifName)
		return -1;

	snprintf(leasefile, sizeof(leasefile), "%s.%s", PATH_DHCLIENT6_LEASE, ifName);

	//CLAT: dhcpv6pd
	if(getLeasesInfo(leasefile, &dlgInfo) & LEASE_GET_IAPD)
		inet_ntop(PF_INET6, &dlgInfo.prefixIP, clatprefix6, sizeof(clatprefix6));

	//PLAT: mib
	if(getATMVCEntryByIfName(ifName, &Entry))
		inet_ntop(PF_INET6, Entry.xlat464_plat_prefix6, platprefix6, sizeof(platprefix6));

	if(tunnel46_get_tunnel_conf_file(IPV6_XLAT464, ifName, conf_file)!= 1)
		return -1;

	if((file_xlat464 = fopen(conf_file ,"w"))== NULL){
		TUNNEL46_ERR("(xlat464_dhcpv6pd) open %s failed!", conf_file);
		return -1;
	}

	snprintf(conf_file, sizeof(conf_file), XLAT464_CLAT_PLAT_INFO, clatprefix6, platprefix6);
	fputs(conf_file, file_xlat464);
	TUNNEL46_DEBUG("(xlat464_dhcpv6pd) conf_file=%s", conf_file);
	fclose(file_xlat464);

	return 1;
}

static int xlat464_setup_nat46_interface(char *ifName){
	FILE *file_xlat464=NULL, *file_nat46=NULL;
	char conf_file[TUNNEL46_FILE_LEN]={0}, buf[256]={0}, xlat464_ifname[TUNNEL46_IFACE_LEN]={0};
	char clat_prefix6[INET6_ADDRSTRLEN+4]={0}, plat_prefix6[INET6_ADDRSTRLEN+4]={0};

	if(!ifName || (tunnel46_get_tunnel_name(IPV6_XLAT464, ifName, xlat464_ifname)!= 1))
		return -1;

	if(tunnel46_get_tunnel_conf_file(IPV6_XLAT464, ifName, conf_file)!= 1)
		return -1;

	/* access the /tmp/xlat464.conf.nas0_0 to get the clat and plat prefix. */
	if((file_xlat464 = fopen(conf_file, "r")) == NULL){
		TUNNEL46_ERR("(xlat464) open %s failed!", conf_file);
		return -1;
	}

	while(fgets(buf, sizeof(buf), file_xlat464)!= NULL){
		if(buf[strlen(buf)-1]== '\n')
			buf[strlen(buf)-1] = '\0';

		if(strstr(buf, XLAT464_CLAT_INFO))
			snprintf(clat_prefix6, sizeof(clat_prefix6), "%s/96", buf+strlen(XLAT464_CLAT_INFO));
		else if(strstr(buf, XLAT464_PLAT_INFO))
			snprintf(plat_prefix6, sizeof(plat_prefix6), "%s/96", buf+strlen(XLAT464_PLAT_INFO));
	}

	/* Insert nat46.ko and create nat46 interface for xlat464. */
	va_cmd("insmod", 1, 1, XLAT464_NAT46_MODULE);

	if((file_nat46= fopen(XLAT464_NAT46_FILE, "w"))== NULL){
		TUNNEL46_ERR("(xlat464) open %s failed!", XLAT464_NAT46_FILE);
		return -1;
	}

	fprintf(file_nat46, "add %s\nconfig %s local.style RFC6052 local.v4 192.0.0.1/32 local.v6 %s remote.style RFC6052 remote.v6 %s\n", xlat464_ifname, xlat464_ifname, clat_prefix6, plat_prefix6);

	fclose(file_xlat464);
	fclose(file_nat46);

	return 1 ;
}

static int xlat464_delete_nat46_interface(char *ifName){
	char xlat464_ifname[TUNNEL46_IFACE_LEN]={0};
	FILE *file_nat46=NULL;
	char cmd[128]={0};

	if(!ifName || (tunnel46_get_tunnel_name(IPV6_XLAT464, ifName, xlat464_ifname)!= 1))
		return -1;

	if((file_nat46= fopen(XLAT464_NAT46_FILE, "w"))== NULL){
		TUNNEL46_ERR("(xlat464) open %s failed!", XLAT464_NAT46_FILE);
		return -1;
	}

	fprintf(file_nat46, "del %s", xlat464_ifname);
	fclose(file_nat46);

	return 1;
}

int xlat464_setup_interface(char * ifName){
	char xlat464_ifname[TUNNEL46_IFACE_LEN]={0};
	int wan_mtu=0;
	char sValue[8]={0};

	if(!ifName || (tunnel46_get_tunnel_name(IPV6_XLAT464, ifName, xlat464_ifname)!= 1))
		return -1;

	/* Use nat46 kernel module to create the nat46 interface for xlat464. */
	xlat464_setup_nat46_interface(ifName);

	/* Enable ipv6 interface. */
	setup_disable_ipv6(xlat464_ifname, 0);

	/* ip link set dev xlat464_nas0_x up */
	va_cmd(BIN_IP, 5, 1, "link", "set", "dev", xlat464_ifname, "up");

	/* Set MTU with WAN interface minus 20 (difference of header of ipv4 and ipv6) for XLAT464
	 * instead of 16384 set by nat46 module. */
	if (getInMTU((const char *)ifName, &wan_mtu)){
		snprintf(sValue, sizeof(sValue), "%d", wan_mtu-20);
		va_cmd(IFCONFIG, 3, 1, xlat464_ifname, "mtu", sValue);
	}

	/* Set IPv4 default gw to xlat464 interface */
	va_cmd(BIN_IP, 6, 1, "-4", "route", "add", "default", "dev", xlat464_ifname);

	/* Set IPv6 policy route */
	xlat464_set_policy_route(ifName, ROUTE_ADD);

	return 1;
}

int xlat464_delete_interface(char * ifName){
	char xlat464_ifname[TUNNEL46_IFACE_LEN]={0};
	char conf_file[TUNNEL46_FILE_LEN]={0};

	if(!ifName || (tunnel46_get_tunnel_name(IPV6_XLAT464, ifName, xlat464_ifname)!= 1))
		return -1;

	if(tunnel46_get_tunnel_conf_file(IPV6_XLAT464, ifName, conf_file)!= 1)
		return -1;

	//ip ro del default
	va_cmd_no_error(BIN_IP, 3, 1, "route", "delete", "default");
	//ip link set dev xlat464_nas0_x down
	va_cmd(BIN_IP, 5, 1, "link", "set", "dev", xlat464_ifname, "down");
	/* Delete IPv6 policy route */
	xlat464_set_policy_route(ifName, ROUTE_DEL);

	xlat464_delete_nat46_interface(ifName);

	return 1;
}

static void xlat464_add_firewall(char *ifName){
	char xlat464_ifname[TUNNEL46_IFACE_LEN]={0};

	if(ifName && (tunnel46_get_tunnel_name(IPV6_XLAT464, ifName, xlat464_ifname)== 1)){
		//ip6tables -D FORWARD -i xlat464_nas0_x -j ACCEPT
		va_cmd(IP6TABLES, 6, 1, "-D", "FORWARD", "-i", xlat464_ifname, "-j", "ACCEPT");
		//ip6tables -I FORWARD -i xlat464_nas0_x -j ACCEPT
		va_cmd(IP6TABLES, 6, 1, "-I", "FORWARD", "-i", xlat464_ifname, "-j", "ACCEPT");

		//Del TCP MSS
		va_cmd("/bin/iptables", 13, 1, "-D", "FORWARD", "-o", xlat464_ifname, "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j", "TCPMSS", "--set-mss", "1412");
		//Add TCP MSS
		va_cmd("/bin/iptables", 13, 1, "-I", "FORWARD", "-o", xlat464_ifname, "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j", "TCPMSS", "--set-mss", "1412");

		//nat table
		/* do snat to 192.0.0.1 for incoming ipv4 */
		va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", xlat464_postrouting);
		va_cmd(IPTABLES, 4, 1, "-t", "nat", "-D", "POSTROUTING", "-j", xlat464_postrouting);
		va_cmd(IPTABLES, 8, 1, "-t", "nat", "-A", "POSTROUTING", "-o", xlat464_ifname, "-j", xlat464_postrouting);
		va_cmd(IPTABLES, 12, 1, "-t", "nat", "-A", xlat464_postrouting, "-o", xlat464_ifname, "-p", "icmp", "-j", "SNAT", "--to-source", "192.0.0.1");
		va_cmd(IPTABLES, 12, 1, "-t", "nat", "-A", xlat464_postrouting, "-o", xlat464_ifname, "-p", "tcp", "-j", "SNAT", "--to-source", "192.0.0.1");
		va_cmd(IPTABLES, 12, 1, "-t", "nat", "-A", xlat464_postrouting, "-o", xlat464_ifname, "-p", "udp", "-j", "SNAT", "--to-source", "192.0.0.1");
	}
}

static void xlat464_delete_firewall(char *ifName){
	char xlat464_ifname[TUNNEL46_IFACE_LEN]={0};

	if(ifName && (tunnel46_get_tunnel_name(IPV6_XLAT464, ifName, xlat464_ifname)== 1)){
		//ip6tables -D FORWARD -i xlat464_nas0_x -j ACCEPT
		va_cmd(IP6TABLES, 6, 1, "-D", "FORWARD", "-i", xlat464_ifname, "-j", "ACCEPT");

		//nat table
		va_cmd(IPTABLES, 8, 1, "-t", "nat", "-D", "POSTROUTING", "-o", xlat464_ifname, "-j", xlat464_postrouting);
		va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", xlat464_postrouting);
		va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", xlat464_postrouting);
	}
}

static int xlat464_set_policy_route(char *ifName, int AddOrDelete){
	FILE *file_xlat464=NULL;
	char xlat464_ifname[TUNNEL46_IFACE_LEN]={0};
	char conf_file[TUNNEL46_FILE_LEN]={0}, buf[256]={0};
	char clat_prefix6[INET6_ADDRSTRLEN]={0};
	char str_tblid[10]={0};

	if(!ifName || (tunnel46_get_tunnel_name(IPV6_XLAT464, ifName, xlat464_ifname)!= 1))
		return -1;

	if(tunnel46_get_tunnel_conf_file(IPV6_XLAT464, ifName, conf_file)!= 1)
		return -1;

	/* treats as a PD policy route */
	snprintf(str_tblid, sizeof(str_tblid), "%d", IP_ROUTE_LAN_TABLE);

	/* access the /tmp/xlat464.conf.nas0_0 to get the clat prefix. */
	if((file_xlat464 = fopen(conf_file, "r")) == NULL){
		TUNNEL46_ERR("(xlat464) open %s failed!", conf_file);
		return -1;
	}

	while(fgets(buf, sizeof(buf), file_xlat464)!= NULL){
		if(buf[strlen(buf)-1]== '\n')
			buf[strlen(buf)-1] = '\0';
		if(strstr(buf, XLAT464_CLAT_INFO))
			snprintf(clat_prefix6, sizeof(clat_prefix6), "%s/64", buf+strlen(XLAT464_CLAT_INFO));
	}

	if(AddOrDelete==ROUTE_ADD){
		//ip -6 ro add {clat_prefix6} dev {xlat464_interface} {table_id}
		va_cmd(BIN_IP, 8, 1, "-6", "route", "add", clat_prefix6, "dev", xlat464_ifname, "table", str_tblid);
	}
	else{
		//ip -6 ro del {clat_prefix6} dev {xlat464_interface} {table_id}
		va_cmd_no_error(BIN_IP, 8, 1, "-6", "route", "delete", clat_prefix6, "dev", xlat464_ifname, "table", str_tblid);
	}

	fclose(file_xlat464);
	return 1;
}

int xlat464_start_interface(char * ifName){

	if(!ifName)
		return -1;

	xlat464_setup_interface(ifName);
	xlat464_add_firewall(ifName);

	return 1;
}

int xlat464_stop_interface(char * ifName){

	if(!ifName)
		return -1;

	xlat464_delete_interface(ifName);
	xlat464_delete_firewall(ifName);

	return 1;
}

static void *pt_xlat464_setup(void *ifName){
	pthread_t my_ptid = pthread_self();
	char wan_iface[IFNAMSIZ]={0};

	memcpy(wan_iface, ifName, (sizeof(wan_iface)>strlen(ifName)?strlen(ifName):sizeof(wan_iface)));
	free(ifName);

	if(my_ptid != cur_xlat464_ptid)
		return NULL;

	xlat464_stop_interface(wan_iface);
	xlat464_start_interface(wan_iface);

	return NULL;
}

int do_xlat464(char *ifname){
	pthread_t pid;
	char *wan_name=NULL;

	if(!ifname)
		return -1;

	if((wan_name = (char *)malloc(IFNAMSIZ)) == NULL)
		return -1;

	memset(wan_name, 0, IFNAMSIZ);
	snprintf(wan_name, IFNAMSIZ, "%s", ifname);
	if(pthread_create(&pid, NULL, pt_xlat464_setup, (void *)wan_name)==0){
		cur_xlat464_ptid = pid;
	}
	else{
		return -1;
	}

	pthread_detach(pid);

	return 1;
}
#endif

#ifdef CONFIG_USER_LW4O6
#define LW4O6_INFO_FILE "/tmp/lw4o6-%s.info"
#define LW4O6_INFO "RULE:ipv6prefix=%s,prefix6len=%d,ipv4prefix=%s,prefix4len=%d,psid=%d,psidlen=%d,offset=%d,br=%s\n"
#define LW4O6_LOCALV6_INFO "IPV6ADDR="
#define LW4O6_AFTR_INFO "BR_ADDR="
#define LW4O6_IPV4_INFO "IPV4ADDR="


static char lw4o6_input[FW_CHAIN_NAME_LEN]="lw4o6_input";
static char lw4o6_forward[FW_CHAIN_NAME_LEN]="lw4o6_forward";
static char lw4o6_output[FW_CHAIN_NAME_LEN]="lw4o6_output";
static char lw4o6_prerouting[FW_CHAIN_NAME_LEN]="lw4o6_prerouting";
static char lw4o6_postrouting[FW_CHAIN_NAME_LEN]="lw4o6_postrouting";

static pthread_t cur_lw4o6_ptid;

/*
 * Save aftr address, local ipv6 prefix, local ipv4 address in /tmp/lw4o6_nas0_0.conf
 * # cat /tmp/lw4o6.conf.nas0_0
 * RULE:ipv6prefix=2001:db8:1::,prefix6len=64,ipv4prefix=192.168.10.77,prefix4len=32,psid=52,psidlen=8,offset=4,br=2001:db8:ffff::1
 */
static int lw4o6_gen_static_mode_parameter(char *ifName){
	MIB_CE_ATM_VC_T vc_entry={0};
	FILE *file_lw4o6=NULL;
	char conf_file[TUNNEL46_FILE_LEN]={0};
	char tmp_buf[256]={0};
	char addr6_prefix[INET6_ADDRSTRLEN]={0};
	unsigned char aftr_addr[INET6_ADDRSTRLEN]={0};
	unsigned char ipv4_addr[INET_ADDRSTRLEN]={0};

	TUNNEL46_DEBUG("(lw4o6_static) ifName(%s)", ifName);

	if(!ifName)
		return -1;

	if(getATMVCEntryByIfName(ifName, &vc_entry)== NULL)
		return -1;

	if(tunnel46_get_tunnel_conf_file(IPV6_LW4O6, ifName, conf_file)!= 1)
		return -1;

	if((file_lw4o6 = fopen(conf_file, "w"))== NULL){
		TUNNEL46_ERR("(lw4o6_static) open %s failed!", conf_file);
		return -1;
	}

	//AFTR address
	inet_ntop(PF_INET6, vc_entry.lw4o6_aftr_addr, aftr_addr, sizeof(aftr_addr));
	//IPv6 prefix
	inet_ntop(PF_INET6, vc_entry.lw4o6_local_v6Prefix, addr6_prefix, sizeof(addr6_prefix));
	//IPv4 address
	inet_ntop(PF_INET, vc_entry.lw4o6_v4addr, ipv4_addr, sizeof(ipv4_addr));

	snprintf(tmp_buf, sizeof(tmp_buf), LW4O6_INFO, addr6_prefix, vc_entry.lw4o6_local_v6PrefixLen, ipv4_addr, 32, vc_entry.lw4o6_psid, vc_entry.lw4o6_psid_len, 4, aftr_addr);
	fputs(tmp_buf, file_lw4o6);
	TUNNEL46_DEBUG("(lw4o6_static) tmp_buf=%s", tmp_buf);
	fclose(file_lw4o6);

	return 1;
}

/*
 * Save aftr address, local ipv6 prefix, local ipv4 address in /tmp/lw4o6_nas0_0.conf
 * # cat /tmp/lw4o6.conf.nas0_0
 * RULE:ipv6prefix=2001:db8:1::,prefix6len=64,ipv4prefix=192.168.10.77,prefix4len=32,psid=52,psidlen=8,offset=4,br=2001:db8:ffff::1
 */
static int lw4o6_gen_dhcp_mode_parameter(char *ifName, char *extra_info){
	DLG_INFO_T dlgInfo;
	char leasefile[64]={0};
	char lw4o6_conf_file[TUNNEL46_FILE_LEN]={0};
	FILE *file_lw4o6=NULL;
	unsigned char aftr_addr[INET6_ADDRSTRLEN]={0};
	unsigned char ipv4_addr[INET_ADDRSTRLEN]={0};
	unsigned char addr6_prefix[INET6_ADDRSTRLEN]={0};
	char tmp_buf[256]={0};

	/* Get lw4o6 dhcp parameters from leasefile and save in the config file. */
	snprintf(leasefile, sizeof(leasefile), "%s.%s", PATH_DHCLIENT6_LEASE, ifName);
	if(getLeasesInfo(leasefile, &dlgInfo) & LEASE_GET_LW4O6){
		if(tunnel46_get_tunnel_conf_file(IPV6_LW4O6, ifName, lw4o6_conf_file) != 1)
			return -1;

		if((file_lw4o6 = fopen(lw4o6_conf_file, "w"))== NULL){
			TUNNEL46_ERR("(lw4o6_dhcp) open %s failed!", lw4o6_conf_file);
			return -1;
		}

		//AFTR address
		inet_ntop(PF_INET6, &dlgInfo.lw4o6_info.s46_br.lw4o6_aftr_addr, aftr_addr, sizeof(aftr_addr));
		//ipv4 address
		inet_ntop(PF_INET, &dlgInfo.lw4o6_info.s46_v4v6bind.lw4o6_v4_addr, ipv4_addr, sizeof(ipv4_addr));
		//ipv6 prefix
		inet_ntop(PF_INET6, &dlgInfo.lw4o6_info.s46_v4v6bind.lw4o6_v6_prefix, addr6_prefix, sizeof(addr6_prefix));

		snprintf(tmp_buf, sizeof(tmp_buf), LW4O6_INFO,
			addr6_prefix, dlgInfo.lw4o6_info.s46_v4v6bind.prefix6_len, ipv4_addr, 32, dlgInfo.lw4o6_info.s46_portparams.psid_val,
			dlgInfo.lw4o6_info.s46_portparams.psid_len, dlgInfo.lw4o6_info.s46_portparams.psid_offset, aftr_addr);

		fputs(tmp_buf, file_lw4o6);
		TUNNEL46_DEBUG("(lw4o6_dhcp) tmp_buf=%s", tmp_buf);

		fclose(file_lw4o6);
	}

	return 1;
}

/*
 * Use mapcal to calculate mapping rules and save in /tmp/lw4o6-nas0_0.rule(info)
 * # cat /tmp/lw4o6-nas0_0.info
 * IPV6ADDR=2001:db8:1::c0a8:a4d:34
 * IPV4ADDR=192.168.10.77
 * PORTSETS=4928-4943 9024-9039 13120-13135 17216-17231 21312-21327 25408-25423 29504-29519 33600-33615
 *          37696-37711 41792-41807 45888-45903 49984-49999 54080-54095 58176-58191 62272-62287
 * BR_ADDR=2001:db8:ffff::1
 */
static void lw4o6_save_rules_in_info_file(char *ifName){
	char conf_file[TUNNEL46_FILE_LEN]={0};
	char line[256]={0}, cmd_line[512]={0};
	FILE *fp = NULL;

	if(!ifName)
		return;

	if(tunnel46_get_tunnel_conf_file(IPV6_LW4O6, ifName, conf_file)!= 1)
		return;

	if((fp = fopen(conf_file, "r")) == NULL)
		return;

	while(fgets(line, sizeof(line), fp)){
		TUNNEL46_DEBUG("(lw4o6) line=%s", line);

		if(strstr(line, "RULE:")){
			snprintf(cmd_line, sizeof(cmd_line), "/bin/mapcalc %d %s type=lw4o6,%s", IPV6_LW4O6, ifName, line+strlen("RULE:"));
			system(cmd_line);

			TUNNEL46_DEBUG("(lw4o6) cmd_line=%s", cmd_line);
		}
	}
	fclose(fp);
}

static int lw4o6_setup_tunnel_iface(char *ifName){
	MIB_CE_ATM_VC_T vc_entry={0};
	char tunnel_name[TUNNEL46_IFACE_LEN]={0};
	char info_file[TUNNEL46_FILE_LEN]={0};
	char local_v6addr[INET6_ADDRSTRLEN]={0};
	char aftr_addr[INET6_ADDRSTRLEN]={0};
	char tunnel_addr4[INET_ADDRSTRLEN]={0};
	FILE *fp=NULL;
	char line[128]={0};

	if(!ifName || (tunnel46_get_tunnel_name(IPV6_LW4O6, ifName, tunnel_name)!= 1))
		return -1;

	snprintf(info_file, sizeof(info_file), LW4O6_INFO_FILE, ifName);

	if((fp = fopen(info_file, "r"))!= NULL){
		while(fgets(line, sizeof(line), fp)){
			if(line[strlen(line)-1] == '\n')
				line[strlen(line)-1] = '\0';

			if(strstr(line, LW4O6_LOCALV6_INFO)){
				snprintf(local_v6addr, sizeof(local_v6addr), "%s", line+strlen(LW4O6_LOCALV6_INFO));
				TUNNEL46_DEBUG("(lw4o6) line=%s, local_v6addr=%s\n", line, local_v6addr);
			}
			else if(strstr(line, LW4O6_AFTR_INFO)){
				snprintf(aftr_addr, sizeof(aftr_addr), "%s", line+strlen(LW4O6_AFTR_INFO));
				TUNNEL46_DEBUG("(lw4o6) line=%s, aftr_addr=%s\n", line, aftr_addr);
			}
			else if(strstr(line, LW4O6_IPV4_INFO)){
				snprintf(tunnel_addr4, sizeof(tunnel_addr4), "%s", line+strlen(LW4O6_IPV4_INFO));
				TUNNEL46_DEBUG("(lw4o6) line=%s, ipv4_addr=%s\n", line, tunnel_addr4);
			}
		}
		fclose(fp);
	}

	//ip -6 tunnel add lw4o6_nas0_0 mode ipip6 local local_v6addr remote aftr_addr dev nas0_0 encaplimit none
	va_cmd("/usr/bin/ip", 14, 1, "-6", "tunnel", "add", tunnel_name, "mode", "ipip6", "local", local_v6addr, "remote", aftr_addr, "dev", ifName, "encaplimit", "none");

	//ip link set dev lw4o6_nas0_0 up
	va_cmd("/usr/bin/ip", 5, 1, "link", "set", "dev", tunnel_name, "up");

	//Enable lw4o6_nas0_0 ipv6 interface
	setup_disable_ipv6(tunnel_name, 0);

	//ip -6 addr add local_v6addr dev lw4o6_nas0_0
	va_cmd("/usr/bin/ip", 6, 1, "-6", "addr", "add", local_v6addr, "dev", tunnel_name);

	//ip addr add {tunnel_v4} peer {tunnel_v4} dev lw4o6_nas0_0
	va_cmd("/usr/bin/ip", 7, 1, "addr", "add", tunnel_addr4, "peer", tunnel_addr4, "dev", tunnel_name);

	if(!(getATMVCEntryByIfName(ifName, &vc_entry))){
		TUNNEL46_ERR("(lw4o6) Could not get MIB entry info from wanconn (ifName=%s)", ifName);
		return -1;
	}

	if(isDefaultRouteWan(&vc_entry)){
		va_cmd("/usr/bin/ip", 3, 1, "route", "delete", "default");
		va_cmd("/usr/bin/ip", 5, 1, "route", "add", "default", "dev", tunnel_name);
	}

	return 1;
}

static int lw4o6_add_snat_firewall(char *ifName){
	MIB_CE_ATM_VC_T vc_entry={0};
	char lw4o6_ifname[TUNNEL46_IFACE_LEN]={0};
	char info_file[TUNNEL46_FILE_LEN]={0}; // /tmp/lw4o6-nas0_0.info
	FILE *fp=NULL;
	char line[256]={0}, snat_buf[64]={0};
	char tunnel_addr4[INET_ADDRSTRLEN]={0};
	char *protocol[3]={"icmp", "tcp", "udp"};
	char *ptr=NULL, *save_ptr=NULL;
	char lw4o6_post_chain[FW_CHAIN_NAME_LEN]={0};
	int i=0;

	TUNNEL46_DEBUG("(lw4o6) ifName(%s)", ifName);

	if(!ifName || (tunnel46_get_tunnel_name(IPV6_LW4O6, ifName, lw4o6_ifname)!= 1)){
		TUNNEL46_ERR("(lw4o6) Get lw4o6 ifName(%s) tunnel_name failed!", ifName?ifName:"");
		return -1;
	}

	if (!(getATMVCEntryByIfName(ifName, &vc_entry))){
		TUNNEL46_ERR("(lw4o6) Error! Could not get MIB entry info from wanconn %s!", ifName);
		return -1;
	}

	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", lw4o6_postrouting);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-D", lw4o6_postrouting);
	va_cmd(IPTABLES, 6, 1, "-t", "nat", "-A", "POSTROUTING", "-j", lw4o6_postrouting);

	snprintf(lw4o6_post_chain, sizeof(lw4o6_post_chain), "POST_ROUTING_%s", lw4o6_ifname);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", lw4o6_post_chain);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", lw4o6_post_chain);
	va_cmd(IPTABLES, 6, 1, "-t", "nat", "-D", lw4o6_postrouting, "-j", lw4o6_post_chain);
	va_cmd(IPTABLES, 6, 1, "-t", "nat", "-A", lw4o6_postrouting, "-j", lw4o6_post_chain);

	//retrieved ipv4 and port set from /tmp/lw4o6-nas0_0.info
	snprintf(info_file, sizeof(info_file), LW4O6_INFO_FILE, ifName);
	if((fp = fopen(info_file, "r")) == NULL)
		return 0;

	while(fgets(line, sizeof(line), fp)!= NULL){
		TUNNEL46_DEBUG("(lw4o6) line=%s\n", line);
		if(line[strlen(line)-1] == '\n')
			line[strlen(line)-1] = '\0';
		if(strstr(line, LW4O6_IPV4_INFO)){
			snprintf(tunnel_addr4, sizeof(tunnel_addr4), "%s", line+strlen(LW4O6_IPV4_INFO));
			TUNNEL46_DEBUG("(lw4o6) line=%s, ipv4_addr=%s\n", line, tunnel_addr4);
		}
		if(strstr(line, "PORTSETS=")){
			ptr = strtok_r(line+strlen("PORTSETS="), " ", &save_ptr);
			while(ptr){
				if(ptr[strlen(ptr)-1 ]== '\n')
					ptr[strlen(ptr)-1] = '\0';
				snprintf(snat_buf, sizeof(snat_buf), "%s:%s", tunnel_addr4, ptr);
				TUNNEL46_DEBUG("(lw4o6) ifName(%s), snat_buf(%s)", ifName, snat_buf);
				for(i=0;i<3;i++){//icmp,tcp,udp
					va_cmd(IPTABLES, 12, 1, "-t", "nat", "-A", lw4o6_post_chain, "-o", lw4o6_ifname, "-p", protocol[i], "-j", "SNAT", "--to-source", snat_buf);
				}
				ptr = strtok_r(NULL, " ", &save_ptr);
			}
			fclose(fp);
			return 1;
		}
		else
			continue;
	}

	fclose(fp);
	return -1;
}

static void lw4o6_add_firewall(char *ifName){
	char lw4o6_ifname[TUNNEL46_FILE_LEN]={0};

	if(ifName && (tunnel46_get_tunnel_name(IPV6_LW4O6, ifName, lw4o6_ifname))== 1){
		//iptables TCP MSS
		va_cmd(IPTABLES, 13, 1, "-D", "FORWARD", "-o", lw4o6_ifname, "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j", "TCPMSS", "--set-mss", "1412");
		va_cmd(IPTABLES, 13, 1, "-I", "FORWARD", "-o", lw4o6_ifname, "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j", "TCPMSS", "--set-mss", "1412");
		//ip6tables FORWARD
		va_cmd(IP6TABLES, 6, 1, "-D", "FORWARD", "-i", lw4o6_ifname, "-j", "ACCEPT");
		va_cmd(IP6TABLES, 6, 1, "-I", "FORWARD", "-i", lw4o6_ifname, "-j", "ACCEPT");
	}

	/* Parse portsets and add them to snat rule */
	lw4o6_add_snat_firewall(ifName);

}
static void lw4o6_delete_firewall(char *ifName){
	char lw4o6_ifname[TUNNEL46_FILE_LEN]={0};
	char lw4o6_post_chain[FW_CHAIN_NAME_LEN]={0};

	if(ifName && (tunnel46_get_tunnel_name(IPV6_LW4O6, ifName, lw4o6_ifname))== 1){
		//TCP MSS
		va_cmd(IPTABLES, 13, 1, "-D", "FORWARD", "-o", lw4o6_ifname, "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j", "TCPMSS", "--set-mss", "1412");
		//filter
		va_cmd(IP6TABLES, 6, 1, "-D", "FORWARD", "-i", lw4o6_ifname, "-j", "ACCEPT");
		//nat
		snprintf(lw4o6_post_chain, sizeof(lw4o6_post_chain), "POST_ROUTING_%s", lw4o6_ifname);
		va_cmd(IPTABLES, 6, 1, "-t", "nat", "-D", lw4o6_postrouting, "-j", lw4o6_post_chain);
		va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", lw4o6_post_chain);
		va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", lw4o6_post_chain);
		va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", lw4o6_postrouting);
	}
}

static void lw4o6_del_tunnel_iface(char *ifName){
	tunnel46_del_tunnel_iface(IPV6_LW4O6, ifName);
}

static int lw4o6_stop_tunnel(char *ifName){
	if(!ifName || (is_tunnel46_conf_file_exsit(IPV6_LW4O6, ifName)!= 1))
		return -1;

	lw4o6_del_tunnel_iface(ifName);
	lw4o6_delete_firewall(ifName);

	return 1;
}

static int lw4o6_setup_tunnel(char *ifName){
	if(!ifName)
		return -1;

	lw4o6_save_rules_in_info_file(ifName);

	lw4o6_setup_tunnel_iface(ifName);

	lw4o6_add_firewall(ifName);

	return 1;
}


static void *pt_lw4o6_setup(void *ifName){
	pthread_t my_pid = pthread_self();
	char wan_iface[IFNAMSIZ]={0};

	memcpy(wan_iface, ifName, (sizeof(wan_iface)>strlen(ifName)?strlen(ifName):sizeof(wan_iface)));
	free(ifName);

	if(my_pid != cur_lw4o6_ptid)
		return NULL;

	lw4o6_stop_tunnel(wan_iface);
	lw4o6_setup_tunnel(wan_iface);

	return NULL;
}

static int do_lw4o6(char *ifName){
	pthread_t pid;
	char *wan_name=NULL;

	if(!ifName)
		return -1;

	if((wan_name = (char*)malloc(IFNAMSIZ))== NULL)
		return -1;

	memset(wan_name, 0, IFNAMSIZ);
	snprintf(wan_name, IFNAMSIZ, "%s", ifName);
	if(pthread_create(&pid, NULL, pt_lw4o6_setup, (void *)wan_name)==0){
		cur_lw4o6_ptid = pid;
	}
	else{
		return -1;
	}

	pthread_detach(pid);

	return 1;
}
#endif


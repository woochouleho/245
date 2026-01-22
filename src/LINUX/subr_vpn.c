/*
 *      VPN basic routines
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/file.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include "mib.h"
#include "utility.h"
#include "sockmark_define.h"
#include <sys/stat.h>
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#include "fc_api.h"
#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD) || defined(CONFIG_USER_PPTP_CLIENT)
#include <regex.h>
#endif

#if defined(CONFIG_CTC_SDN)
#include "subr_ctcsdn.h"
#endif
#include "subr_net.h"

/* Const string define */
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
const char NF_PPTP_ROUTE_CHAIN[] = "pptp_%s_route";
const char NF_PPTP_ROUTE_COMMENT[] = "pptp_route_idx_%d";
#ifdef VPN_MULTIPLE_RULES_BY_FILE
const char NF_PPTP_IPSET_IPRANGE_CHAIN[] = "pptp_ip_range_%s";
const char NF_PPTP_IPSET_IPSUBNET_CHAIN[] = "pptp_ip_subnet_%s";
const char NF_PPTP_IPSET_SMAC_CHAIN[] = "pptp_smac_%s";
const char NF_PPTP_IPSET_RULE_INDEX_COMMENT[] = "pptp_ruletype_%d_route_idx_%d";
const char NF_PPTP_IPSET_IPRANGE_RULE_COMMENT[] = ",%s"; // ex:,192.168.100.100-192.168.100.200
#endif
#endif

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
static const char *tunnel_auth[2] = {"none", "challenge"};
static const char *l2tp_auth[] = {"auto", "pap", "chap", "chapms-v2"};
static const char *l2tp_encryt[] = {"none", "+MPPE", "+MPPC", "+BOTH"};
const char NF_L2TP_ROUTE_CHAIN[] = "l2tp_%s_route";
const char NF_L2TP_ROUTE_COMMENT[] = "l2tp_route_idx_%d";
#ifdef VPN_MULTIPLE_RULES_BY_FILE
const char NF_L2TP_IPSET_IPRANGE_CHAIN[] = "l2tp_ip_range_%s";
const char NF_L2TP_IPSET_IPSUBNET_CHAIN[] = "l2tp_ip_subnet_%s";
const char NF_L2TP_IPSET_SMAC_CHAIN[] = "l2tp_smac_%s";
const char NF_L2TP_IPSET_RULE_INDEX_COMMENT[] = "l2tp_ruletype_%d_route_idx_%d";
const char NF_L2TP_IPSET_IPRANGE_RULE_COMMENT[] = ",%s"; // ex:,192.168.100.100-192.168.100.200
#endif
#endif
#ifdef CONFIG_USER_STRONGSWAN
const char CHARONPID[] = "/var/run/charon.pid";
#endif
/* Const string define end */

/* Function prototype define */
// int xxx(int yy);

/* Function prototype define end */

/* Function define */
#if defined (CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_L2TPD_LNS)
int set_LAN_VPN_accept(char *chain)
{
	FILE* fp;

	fp=fopen("/var/ppp/ppp.conf","r");
	if(fp)
	{
		char *tmp,linestr[128];
		int pppidx=0;
		while(fgets(linestr,sizeof(linestr),fp))
		{
			if(strstr(linestr,"ppp"))
			{
				tmp=linestr+3;
				while((*tmp>='0') && (*tmp<='9'))
				{
					pppidx=pppidx*10+(*tmp-'0');
					tmp++;
				}
				*tmp='\0';
				if(pppidx>12)
					va_cmd (IPTABLES, 6,1, FW_ADD,chain,(char *)ARG_I,linestr,"-j",(char *)FW_ACCEPT);
			}
		}
		fclose(fp);
	}

	return 0;
}
#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD) || defined(CONFIG_USER_PPTP_CLIENT)

const char NF_VPN_ROUTE_CHAIN[] = "VPN_POLICY_ROUTE";
const char NF_VPN_ROUTE_TABLE[] = "mangle";
const char FW_VPNGRE[] = "vpngre";

int rtk_wan_vpn_tunnel_name_to_table(unsigned char *tunnelName)
{
	unsigned int EntryNum, i;
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	MIB_PPTP_T pptp_entry;
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	MIB_L2TP_T l2tp_entry;
#endif
	int table_idx;


	if(!tunnelName) {
		AUG_PRT("Null pointer !\n");
		return -1;
	}

	if(tunnelName[0] == '\0') {
		AUG_PRT("Null string !\n");
		return -1;
	}

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	EntryNum = mib_chain_total(MIB_PPTP_TBL);
	for (i=0; i<EntryNum; i++)
	{
		if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&pptp_entry) )
				continue;

		if(!strcmp(pptp_entry.tunnelName, tunnelName))
			return 101 + pptp_entry.idx;
	}
#endif

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	EntryNum = mib_chain_total(MIB_L2TP_TBL);
	for (i=0; i<EntryNum; i++)
	{
		if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tp_entry) )
				continue;

		if(!strcmp(l2tp_entry.tunnelName, tunnelName))
			return 103 + l2tp_entry.idx;
	}
#endif

	AUG_PRT("Not VPN tunnel name(tunnelName=%s)\n", tunnelName);
	return -1;
}

int rtk_wan_vpn_policy_route_get_mark(VPN_TYPE_T vpn_type, unsigned char *tunnelName, unsigned int *mark, unsigned int *mask)
{
	int entryIdx;
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	MIB_L2TP_T l2tpEntry;
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	MIB_PPTP_T pptpEntry;
#endif
	char ifName[IFNAMSIZ];
	int ret=-1;

	if(!mark || !mask || !tunnelName) {
		AUG_PRT("Null pointer !\n");
		return -1;
	}

	switch(vpn_type)
	{
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
		case VPN_TYPE_L2TP:
		{
			if(getL2TPEntryByTunnelName(tunnelName, &l2tpEntry, &entryIdx) == NULL)
				return -1;

#if 1
			ifGetName(l2tpEntry.ifIndex, ifName, sizeof(ifName));
			ret=rtk_net_get_wan_policy_route_mark(ifName, mark, mask);
#else
			*mark = SOCK_MARK_WAN_VPN_L2TP_INDEX_TO_MARK(l2tpEntry.idx);
			*mask = SOCK_MARK_WAN_INDEX_BIT_MASK;
#endif
			//*mark |= SOCK_MARK_FORWARD_BY_PS_TO_MARK(1);
			//*mask |= SOCK_MARK_FORWARD_BY_PS_BIT_MASK;
			break;
		}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
		case VPN_TYPE_PPTP:
		{
			if(getPPTPEntryByTunnelName(tunnelName, &pptpEntry, &entryIdx) == NULL)
				return -1;

#if 1
			ifGetName(pptpEntry.ifIndex, ifName, sizeof(ifName));
			ret=rtk_net_get_wan_policy_route_mark(ifName, mark, mask);
#else
			*mark = SOCK_MARK_WAN_VPN_PPTP_INDEX_TO_MARK(pptpEntry.idx);
			*mask = SOCK_MARK_WAN_INDEX_BIT_MASK;
#endif
			
			//*mark |= SOCK_MARK_FORWARD_BY_PS_TO_MARK(1);
			//*mask |= SOCK_MARK_FORWARD_BY_PS_BIT_MASK;
			break;
		}
#endif
		default:
			return -1;
	}

	return ret;
}

int rtk_wan_vpn_policy_route_get_route_tableId(VPN_TYPE_T vpn_type, unsigned char *tunnelName, unsigned int *tableId)
{
	int entryIdx;
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	MIB_L2TP_T l2tpEntry;
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	MIB_PPTP_T pptpEntry;
#endif

	if(!tableId || !tunnelName) {
		AUG_PRT("Null pointer !\n");
		return -1;
	}

	switch(vpn_type)
	{
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
		case VPN_TYPE_L2TP:
		{
			if(getL2TPEntryByTunnelName(tunnelName, &l2tpEntry, &entryIdx) == NULL)
				return -1;
			*tableId = 103 + l2tpEntry.idx;
			break;
		}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
		case VPN_TYPE_PPTP:
		{
			if(getPPTPEntryByTunnelName(tunnelName, &pptpEntry, &entryIdx) == NULL)
				return -1;
			*tableId = 101 + pptpEntry.idx;
			break;
		}
#endif
		default:
			return -1;
	}

	return 0;
}

#ifndef CONFIG_E8B
int rtk_wan_vpn_attach_policy_route_init(void)
{
    char cmd[256];

    sprintf(cmd, "%s -t %s -N %s", IPTABLES, NF_VPN_ROUTE_TABLE, NF_VPN_ROUTE_CHAIN);
    va_cmd("/bin/sh", 2, 1, "-c", cmd);
    sprintf(cmd, "%s -t %s -D %s -i br+ -j %s", IPTABLES, NF_VPN_ROUTE_TABLE, FW_PREROUTING, NF_VPN_ROUTE_CHAIN);
    va_cmd("/bin/sh", 2, 1, "-c", cmd);
    sprintf(cmd, "%s -t %s -D %s -j %s", IPTABLES, NF_VPN_ROUTE_TABLE, FW_OUTPUT, NF_VPN_ROUTE_CHAIN);
    va_cmd("/bin/sh", 2, 1, "-c", cmd);
    sprintf(cmd, "%s -t %s -A %s -i br+ -j %s", IPTABLES, NF_VPN_ROUTE_TABLE, FW_PREROUTING, NF_VPN_ROUTE_CHAIN);
    va_cmd("/bin/sh", 2, 1, "-c", cmd);
    sprintf(cmd, "%s -t %s -A %s -j %s", IPTABLES, NF_VPN_ROUTE_TABLE, FW_OUTPUT, NF_VPN_ROUTE_CHAIN);
    va_cmd("/bin/sh", 2, 1, "-c", cmd);

    return 0;
}

#endif
/*
 *	remove the ifup_ppp(vpc)x script for WAN interface
 */
static int rtk_wan_vpn_remove_ifup_script(VPN_TYPE_T type, void *pentry)
{
	char ifname[IFNAMSIZ] = {0};
	char path[64];
	char *tunnelName = NULL;

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if(type == VPN_TYPE_L2TP)
	{
		ifGetName(((MIB_L2TP_T *)pentry)->ifIndex,ifname,sizeof(ifname));
		tunnelName = ((MIB_L2TP_T *)pentry)->tunnelName;
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if(type == VPN_TYPE_PPTP)
	{
		ifGetName(((MIB_PPTP_T *)pentry)->ifIndex,ifname,sizeof(ifname));
		tunnelName = ((MIB_PPTP_T *)pentry)->tunnelName;
	}
#endif
	if(tunnelName == NULL){
		printf("[%s] Error tunnel Name\n", __FUNCTION__);
		return -1;
	}

	snprintf(path, sizeof(path), "/var/ppp/ifup_%s", ifname);
	unlink(path);
#ifdef CONFIG_IPV6_VPN
	snprintf(path, sizeof(path), "/var/ppp/ifupv6_%s", ifname);
	unlink(path);
#endif

	return 0;
}

/*
 *	Do the ifdown_ppp(vpn)x script before remove ifdown_ppp(vpn)x script for WAN interface
 */
static int rtk_wan_vpn_do_ifdown_script(VPN_TYPE_T type, void *pentry)
{
	char ifname[IFNAMSIZ] = {0};
	char path[64];
	char *tunnelName = NULL;

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if(type == VPN_TYPE_L2TP)
	{
		ifGetName(((MIB_L2TP_T *)pentry)->ifIndex,ifname,sizeof(ifname));
		tunnelName = ((MIB_L2TP_T *)pentry)->tunnelName;
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if(type == VPN_TYPE_PPTP)
	{
		ifGetName(((MIB_PPTP_T *)pentry)->ifIndex,ifname,sizeof(ifname));
		tunnelName = ((MIB_PPTP_T *)pentry)->tunnelName;
	}
#endif
	if(tunnelName == NULL){
		printf("[%s] Error tunnel Name\n", __FUNCTION__);
		return -1;
	}

	snprintf(path, sizeof(path), "/var/ppp/ifdown_%s", ifname);
	if(!access(path, F_OK))
	{
		va_cmd(path, 0, 1);
		printf("%s: run %s\n", __FUNCTION__, path);
	}

#ifdef CONFIG_IPV6_VPN
	snprintf(path, sizeof(path), "/var/ppp/ifdownv6_%s", ifname);
	if(!access(path, F_OK))
	{
		va_cmd(path, 0, 1);
		printf("%s: run %s\n", __FUNCTION__, path);
	}
#endif

	return 0;
}

/*
 *	remove the ifdown_ppp(vpn)x script for WAN interface
 */
static int rtk_wan_vpn_remove_ifdown_script(VPN_TYPE_T type, void *pentry)
{
	char ifname[IFNAMSIZ] = {0};
	char path[64];
	char *tunnelName = NULL;

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if(type == VPN_TYPE_L2TP)
	{
		ifGetName(((MIB_L2TP_T *)pentry)->ifIndex,ifname,sizeof(ifname));
		tunnelName = ((MIB_L2TP_T *)pentry)->tunnelName;
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if(type == VPN_TYPE_PPTP)
	{
		ifGetName(((MIB_PPTP_T *)pentry)->ifIndex,ifname,sizeof(ifname));
		tunnelName = ((MIB_PPTP_T *)pentry)->tunnelName;
	}
#endif
	if(tunnelName == NULL){
		printf("[%s] Error tunnel Name\n", __FUNCTION__);
		return -1;
	}

	snprintf(path, sizeof(path), "/var/ppp/ifdown_%s", ifname);

	unlink(path);
#ifdef CONFIG_IPV6_VPN
	snprintf(path, sizeof(path), "/var/ppp/ifdownv6_%s", ifname);
	unlink(path);
#endif

	return 0;
}

/*
 *	generate the ifup_ppp(vpc)x script for WAN interface
 */
static int rtk_wan_vpn_generate_ifup_script(VPN_TYPE_T type, void *pentry)
{
	FILE *fp;
	char ifname[IFNAMSIZ] = {0};
	char path[64];
	char *tunnelName = NULL;
	unsigned int fw_mark = 0, fw_mask = 0, tableId = 0;

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if(type == VPN_TYPE_L2TP)
	{
		ifGetName(((MIB_L2TP_T *)pentry)->ifIndex,ifname,sizeof(ifname));
		tunnelName = ((MIB_L2TP_T *)pentry)->tunnelName;
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP)  || defined(CONFIG_USER_PPTP_CLIENT)
	if(type == VPN_TYPE_PPTP)
	{
		ifGetName(((MIB_PPTP_T *)pentry)->ifIndex,ifname,sizeof(ifname));
		tunnelName = ((MIB_PPTP_T *)pentry)->tunnelName;
	}
#endif
	if(tunnelName == NULL){
		printf("[%s] Error tunnel Name\n", __FUNCTION__);
		return -1;
	}
	if(rtk_wan_vpn_policy_route_get_mark(type, tunnelName, &fw_mark, &fw_mask) != 0){
		printf("[%s]Cannot get MARK for tunnel name(%s) !!\n", __FUNCTION__, tunnelName);
		return -1;
	}
	if(rtk_wan_vpn_policy_route_get_route_tableId(type, tunnelName, &tableId) != 0){
		printf("[%s]Cannot get route table Id for tunnel name(%s) !!\n", __FUNCTION__, tunnelName);
		return -1;
	}

	// for L2TP/PPTP need add server host route in policy route table
	// spppd need support env 'server', 'outif' for ifup script
	// ifup script use server and outif to add host route
	snprintf(path, sizeof(path), "/var/ppp/ifup_%s", ifname);
	if((fp=fopen(path, "w+")))
	{
		fprintf(fp, "#!/bin/sh\n\n");
		fprintf(fp, "mark=0x%x\n", fw_mark);
		fprintf(fp, "mask=0x%x\n", fw_mask);
		fprintf(fp, "rule_priority=%d\n", IP_RULE_PRI_POLICYRT);
		fprintf(fp, "tid=%d\n", tableId);
		fprintf(fp, "dev=%s\n", ifname);
		fprintf(fp, "gw=\n");
		fprintf(fp,
"if [ x${server} != x ]; then                                         \n"
"	c='ip route get '$server                                          \n"
"	if [ x${outif} != x ]; then                                       \n"
"		c=$c' oif '$outif                                             \n"
"	fi	                                                              \n"
"	r=$($c)                                                           \n"
"	if [ \"x${r}\" != \"x${r##*via}\" ]; then                         \n"
"		gw=${r##*via}                                                 \n"
"		set -- $gw                                                    \n"
"		gw=$1                                                         \n"
"	fi                                                                \n"
"	if [ x${outif} = x ]; then                                        \n"
"		if [ \"x${r}\" != \"x${r##*dev}\" ]; then                     \n"
"			outif=${r##*dev}                                          \n"
"			set -- $outif                                             \n"
"			outif=$1                                                  \n"
"		fi                                                            \n"
"	fi                                                                \n"
"fi                                                                   \n"
"ip rule del fwmark $mark/$mask table $tid                            \n"
"ip route flush table $tid                                            \n"
"if [ x${server} != x -a x${gw} != x -a x${outif} != x ]; then        \n"
"	ip route add ${server} via $gw dev $outif table $tid              \n"
"	ip route add $gw dev $outif table $tid                            \n"
"elif [ x${server} != x -a x${outif} != x ]; then                     \n"
"	ip route add ${server} dev $outif table $tid                      \n"
"fi                                                                   \n"
"ip route add default dev $dev table $tid                             \n"
"ip rule add fwmark $mark/$mask pref $rule_priority table $tid        \n"
				);
		fprintf(fp, "\n");
		fclose(fp);
		chmod(path, 484);
	}
#ifdef CONFIG_IPV6_VPN
	snprintf(path, sizeof(path), "/var/ppp/ifupv6_%s", ifname);
	if((fp=fopen(path, "w+")))
	{
		fprintf(fp, "#!/bin/sh\n\n");
		fclose(fp);
		chmod(path, 484);
	}
#endif

	return 0;
}
/*
 *	generate the ifdown_ppp(vpn)x script for WAN interface
 */
static int rtk_wan_vpn_generate_ifdown_script(VPN_TYPE_T type, void *pentry)
{
	FILE *fp;
	char ifname[IFNAMSIZ] = {0};
	char path[64];
	char *tunnelName = NULL;
	unsigned int fw_mark = 0, fw_mask = 0, tableId = 0;

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if(type == VPN_TYPE_L2TP)
	{
		ifGetName(((MIB_L2TP_T *)pentry)->ifIndex,ifname,sizeof(ifname));
		tunnelName = ((MIB_L2TP_T *)pentry)->tunnelName;
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if(type == VPN_TYPE_PPTP)
	{
		ifGetName(((MIB_PPTP_T *)pentry)->ifIndex,ifname,sizeof(ifname));
		tunnelName = ((MIB_PPTP_T *)pentry)->tunnelName;
	}
#endif
	if(tunnelName == NULL){
		printf("[%s] Error tunnel Name\n", __FUNCTION__);
		return -1;
	}
	if(rtk_wan_vpn_policy_route_get_mark(type, tunnelName, &fw_mark, &fw_mask) != 0){
		printf("[%s]Cannot get MARK for tunnel name(%s) !!\n", __FUNCTION__, tunnelName);
		return -1;
	}
	if(rtk_wan_vpn_policy_route_get_route_tableId(type, tunnelName, &tableId) != 0){
		printf("[%s]Cannot get route table Id for tunnel name(%s) !!\n", __FUNCTION__, tunnelName);
		return -1;
	}

	snprintf(path, sizeof(path), "/var/ppp/ifdown_%s", ifname);
	if((fp=fopen(path, "w+")))
	{
		fprintf(fp, "#!/bin/sh\n\n");
		fprintf(fp, "ip rule del fwmark 0x%x/0x%x table %d\n", fw_mark, fw_mask, tableId);
		fprintf(fp, "ip route flush table %d\n", tableId);
		fclose(fp);
		chmod(path, 484);
	}
#ifdef CONFIG_IPV6_VPN
	snprintf(path, sizeof(path), "/var/ppp/ifdownv6_%s", ifname);
	if((fp=fopen(path, "w+")))
	{
		fprintf(fp, "#!/bin/sh\n\n");
		fclose(fp);
		chmod(path, 484);
	}
#endif

	return 0;
}

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)

int NF_Set_PPTP_Dynamic_URL_Route(char *tunnelName, int routeIdx, int weight, char *url, struct in_addr addr)
{
/*
	MIB_PPTP_T pptpEntry;
	int enable,pptpEntryIdx;
*/
	char cmd[256], *pcmd;
	unsigned int fw_mark, fw_mask;
	unsigned char chain_name[64], comment_name[256] = {0}, tmp_comment_name[256] = {0};
	const char *table_name;
/*
	if ( !mib_get_s(MIB_PPTP_ENABLE, (void *)&enable, sizeof(enable)) ){
		printf("MIB_PPTP_ENABLE is not exist!");
		return -1;
	}

	if(!enable){
		printf("MIB_PPTP_ENABLE is not enable!");
		return 0;
	}

	if(getPPTPEntryByTunnelName(tunnelName, &pptpEntry, &pptpEntryIdx) == NULL){
		printf("Cannot found L2TP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}
*/
#ifndef VPN_MULTIPLE_RULES_BY_FILE
	if(rtk_wan_vpn_policy_route_get_mark(VPN_TYPE_PPTP, tunnelName, &fw_mark, &fw_mask) != 0){
		printf("Cannot get MARK of L2TP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}

	table_name = NF_VPN_ROUTE_TABLE;
	sprintf(chain_name, NF_PPTP_ROUTE_CHAIN, tunnelName);

	cmd[0] = '\0';
	pcmd = cmd;
	sprintf(comment_name, NF_PPTP_ROUTE_COMMENT, routeIdx);
	pcmd += sprintf(pcmd, "%s -t %s -A %s", IPTABLES, table_name, chain_name);
	pcmd += sprintf(pcmd, " -m comment --comment \"%s url:%s\"", comment_name, url);
	pcmd += sprintf(pcmd, " -m mark --mark 0x0/0x%x", fw_mask);
	pcmd += sprintf(pcmd, " --ipv4");
	{
		pcmd += sprintf(pcmd, " -d %s", inet_ntoa(addr));
	}
	pcmd += sprintf(pcmd, " -j MARK --set-mark 0x%x/0x%x", fw_mark, fw_mask);

	va_cmd("/bin/sh", 2, 1, "-c", cmd);
#else
	FILE *fp = NULL;
	char file_path[128] = {0};
	char line_buf[256] = {0}, tmp_tunnel_name[64] = {0}, match_rule[64] = {0};
	int pptp_rule_index = -1, rule_mode = -1;

	snprintf(file_path, sizeof(file_path), "%s%s", VPN_PPTP_RULE_FILE, tunnelName);
	fp = fopen(file_path, "r");
	if (fp == NULL)
	{
		printf("%s: Open file = %s failed !!!\n", __FUNCTION__, file_path);
		return -1;
	}
	while (fgets(line_buf, sizeof(line_buf), fp) != NULL)
	{
		//fprintf(fp, "%s %d %d %d %d %d %d %d %d %s\n", vpn_tunnel_info->tunnelName, static_set, i, priority, attach_mode, rule_mode, reserve_idx, pkt_cnt, pkt_pass_lasttime, match_rule);
		if (sscanf(line_buf, "%s %*d %d %*d %*d %d %*d %*d %*d %s", &tmp_tunnel_name, &pptp_rule_index, &rule_mode, &match_rule) != 4)
			continue;

		if (!strcmp(tunnelName, tmp_tunnel_name) && !strcmp(url, match_rule))
		{
			break;
		}
	}
	fclose(fp);
	snprintf(chain_name, sizeof(chain_name), NF_PPTP_IPSET_IPRANGE_CHAIN, tunnelName);
	snprintf(tmp_comment_name, sizeof(tmp_comment_name), NF_PPTP_IPSET_RULE_INDEX_COMMENT, rule_mode, pptp_rule_index);
	/* If you want to change below format, please also change format in rtk_wan_vpn_get_ipset_packet_count_by_route_index() */
	snprintf(comment_name, sizeof(comment_name), "%s,url:%s", tmp_comment_name, url);
	snprintf(cmd, sizeof(cmd), "%s -A %s %s %s \"%s\" -exist", IPSET, chain_name, inet_ntoa(addr), IPSET_OPTION_COMMENT, comment_name);

	//printf("%s: cmd url = %s\n", __FUNCTION__, cmd);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

#endif

	return 0;
}

int NF_Del_PPTP_Dynamic_URL_Route(char *tunnelName, int routeIdx, int weight, char *url, struct in_addr addr)
{
/*
	MIB_PPTP_T pptpEntry;
	int enable,pptpEntryIdx;
*/
	char cmd[256], *pcmd;
	unsigned int fw_mark, fw_mask;
	unsigned char chain_name[64], comment_name[64];
	const char *table_name;
/*
	if ( !mib_get_s(MIB_PPTP_ENABLE, (void *)&enable, sizeof(enable)) ){
		printf("MIB_PPTP_ENABLE is not exist!");
		return -1;
	}

	if(!enable){
		printf("MIB_PPTP_ENABLE is not enable!");
		return 0;
	}

	if(getPPTPEntryByTunnelName(tunnelName, &pptpEntry, &pptpEntryIdx) == NULL){
		printf("Cannot found PPTP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}
*/
#ifndef VPN_MULTIPLE_RULES_BY_FILE
	if(rtk_wan_vpn_policy_route_get_mark(VPN_TYPE_PPTP, tunnelName, &fw_mark, &fw_mask) != 0){
		printf("Cannot get MARK of L2TP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}

	table_name = NF_VPN_ROUTE_TABLE;
	sprintf(chain_name, NF_PPTP_ROUTE_CHAIN, tunnelName);

	cmd[0] = '\0';
	pcmd = cmd;
	sprintf(comment_name, NF_PPTP_ROUTE_COMMENT, routeIdx);
	pcmd += sprintf(pcmd, "%s -t %s -D %s", IPTABLES, table_name, chain_name);
	pcmd += sprintf(pcmd, " -m comment --comment \"%s url:%s\"", comment_name, url);
	pcmd += sprintf(pcmd, " -m mark --mark 0x0/0x%x", fw_mask);
	pcmd += sprintf(pcmd, " --ipv4");
	{
		pcmd += sprintf(pcmd, " -d %s", inet_ntoa(addr));
	}
	pcmd += sprintf(pcmd, " -j MARK --set-mark 0x%x/0x%x", fw_mark, fw_mask);

	va_cmd("/bin/sh", 2, 1, "-c", cmd);

#else
	snprintf(chain_name, sizeof(chain_name), NF_L2TP_IPSET_IPRANGE_CHAIN, tunnelName);
	//snprintf(comment_name, sizeof(comment_name), "url:%s", url);
	//ipset del can't use comment option
	snprintf(cmd, sizeof(cmd), "%s -D %s %s", IPSET, chain_name, inet_ntoa(addr));

	va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
	return 0;
}

#endif //CONFIG_USER_PPTP_CLIENT_PPTP

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
int NF_Del_L2TP_Dynamic_URL_Route(char *tunnelName, int routeIdx, int weight, char *url, struct in_addr addr)
{
/*
	MIB_L2TP_T l2tpEntry;
	int enable,l2tpEntryIdx;
*/
	char cmd[256], *pcmd;
	unsigned int fw_mark, fw_mask;
	unsigned char chain_name[64], comment_name[64];
	const char *table_name;
/*
	if ( !mib_get_s(MIB_L2TP_ENABLE, (void *)&enable, sizeof(enable)) ){
		printf("MIB_L2TP_ENABLE is not exist!");
		return -1;
	}

	if(!enable){
		printf("MIB_L2TP_ENABLE is not enable!");
		return 0;
	}

	if(getL2TPEntryByTunnelName(tunnelName, &l2tpEntry, &l2tpEntryIdx) == NULL){
		printf("Cannot found L2TP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}
*/
#ifndef VPN_MULTIPLE_RULES_BY_FILE
	if(rtk_wan_vpn_policy_route_get_mark(VPN_TYPE_L2TP, tunnelName, &fw_mark, &fw_mask) != 0){
		printf("Cannot get MARK of L2TP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}

	table_name = NF_VPN_ROUTE_TABLE;
	sprintf(chain_name, NF_L2TP_ROUTE_CHAIN, tunnelName);

	cmd[0] = '\0';
	pcmd = cmd;
	sprintf(comment_name, NF_L2TP_ROUTE_COMMENT, routeIdx);
	pcmd += sprintf(pcmd, "%s -t %s -D %s", IPTABLES, table_name, chain_name);
	pcmd += sprintf(pcmd, " -m comment --comment \"%s url:%s\"", comment_name, url);
	pcmd += sprintf(pcmd, " -m mark --mark 0x0/0x%x", fw_mask);
	pcmd += sprintf(pcmd, " --ipv4");
	{
		pcmd += sprintf(pcmd, " -d %s", inet_ntoa(addr));
	}
	pcmd += sprintf(pcmd, " -j MARK --set-mark 0x%x/0x%x", fw_mark, fw_mask);

	va_cmd("/bin/sh", 2, 1, "-c", cmd);
#else
	snprintf(chain_name, sizeof(chain_name), NF_L2TP_IPSET_IPRANGE_CHAIN, tunnelName);
	//snprintf(comment_name, sizeof(comment_name), "url:%s", url);
	//ipset del can't use comment option
	snprintf(cmd, sizeof(cmd), "%s -D %s %s", IPSET, chain_name, inet_ntoa(addr));

	va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
	return 0;
}

#ifdef VPN_MULTIPLE_RULES_BY_FILE
int rtk_wan_vpn_l2tp_get_route_idx_with_template(unsigned char *tunnelName, unsigned int rule_mode, char *match_rule)
{
	char vpn_l2tp_file_path[128] = {0};
	FILE *fp = NULL;
	unsigned char rule_mode_tmp = ATTACH_RULE_MODE_NONE;
	char match_rule_tmp[256] = {0}, line_buf[128] = {0};
	int l2tp_rule_index = -1;

	snprintf(vpn_l2tp_file_path, sizeof(vpn_l2tp_file_path), "%s%s", VPN_L2TP_RULE_FILE, tunnelName);
	fp = fopen(vpn_l2tp_file_path, "r");
	if (fp == NULL)
	{
		printf("%s: L2TP route file open failed!\n", __FUNCTION__);
		return -1;
	}

	while (fgets(line_buf, sizeof(line_buf), fp) != NULL)
	{
		if (sscanf(line_buf, "%*s %*d %d %*d %*d %d %*d %*d %*d %s", &l2tp_rule_index, &rule_mode_tmp,  &match_rule_tmp) != 3)
			continue;
		if ((rule_mode == rule_mode_tmp) && !strcmp(match_rule, match_rule_tmp))
		{
			//printf("%s: match_rule_tmp = %s, match_rule = %s\n ", __FUNCTION__, match_rule_tmp, match_rule);
			if (fp)
				fclose(fp);
			return l2tp_rule_index;
		}
	}
	if (fp)
		fclose(fp);

	return -1;
}

int rtk_wan_vpn_pptp_get_route_idx_with_template(unsigned char *tunnelName, unsigned int rule_mode, char *match_rule)
{
	char vpn_pptp_file_path[128] = {0};
	FILE *fp = NULL;
	unsigned char rule_mode_tmp = ATTACH_RULE_MODE_NONE;
	char match_rule_tmp[256] = {0}, line_buf[128] = {0};
	int pptp_rule_index = -1;

	snprintf(vpn_pptp_file_path, sizeof(vpn_pptp_file_path), "%s%s", VPN_PPTP_RULE_FILE, tunnelName);
	fp = fopen(vpn_pptp_file_path, "r");
	if (fp == NULL)
	{
		printf("%s: PPTP route file open failed!\n", __FUNCTION__);
		return -1;
	}

	while (fgets(line_buf, sizeof(line_buf), fp) != NULL)
	{
		if (sscanf(line_buf, "%*s %*d %d %*d %*d %d %*d %*d %*d %s", &pptp_rule_index, &rule_mode_tmp,  &match_rule_tmp) != 3)
			continue;
		if ((rule_mode == rule_mode_tmp) && !strcmp(match_rule, match_rule_tmp))
		{
			//printf("%s: match_rule_tmp = %s, match_rule = %s\n ", __FUNCTION__, match_rule_tmp, match_rule);
			if (fp)
				fclose(fp);
			return pptp_rule_index;
		}
	}
	if (fp)
		fclose(fp);

	return -1;
}

#endif

int rtk_wan_vpn_l2tp_attach_check_dip(unsigned char *tunnelName,unsigned int ipv4_addr1,unsigned int ipv4_addr2)
{
	unsigned int ip_start = 0, ip_end = 0;
	int totalnum,i,checkIt=0;
	unsigned int tmp_ip_start = 0, tmp_ip_end = 0;
	MIB_CE_L2TP_ROUTE_T entry;

	ip_start = ntohl(ipv4_addr1);
	ip_end = ntohl(ipv4_addr2);

	totalnum = mib_chain_total(MIB_L2TP_ROUTE_TBL); /* get chain record size */
	//AUG_PRT("%s-%d totalnum=%d ipv4_addr1=%x, ipv4_addr2=%x\n",__func__,__LINE__,totalnum,ipv4_addr1,ipv4_addr2);

	for(i=0;i<totalnum;i++)
	{
		if ( !mib_chain_get(MIB_L2TP_ROUTE_TBL, i, (void *)&entry) )
			continue;
		if(tunnelName && (!strcmp(entry.tunnelName,tunnelName) || strstr(tunnelName, MAGIC_TUNNEL_NAME)))
		{
			tmp_ip_start = ntohl(entry.ipv4_src_start);
			tmp_ip_end = ntohl(entry.ipv4_src_end);
			if((ip_start>=tmp_ip_start && ip_start<=tmp_ip_end) ||
					(ip_end>=tmp_ip_start && ip_end<=tmp_ip_end))
			{
				//AUG_PRT("%s-%d i=%d\n",__func__,__LINE__, i);
				return i;
			}
		}
	}
	return -1;
}

int rtk_wan_vpn_l2tp_attach_check_smac(unsigned char *tunnelName, unsigned char* sMAC_addr)
{
	int totalnum,i,checkIt=0;
	MIB_CE_L2TP_ROUTE_T entry;
	unsigned char atta_sMAC_addr[20];
	unsigned char rtbl_sMAC_addr[20];

	totalnum = mib_chain_total(MIB_L2TP_ROUTE_TBL); /* get chain record size */

	//AUG_PRT("%s-%d totalnum=%d sMAC_addr=%X:%X:%X:%X:%X:%X \n",__func__,__LINE__,totalnum,sMAC_addr[0],sMAC_addr[1],sMAC_addr[2],sMAC_addr[3],sMAC_addr[4],sMAC_addr[5]);

	for(i=0;i<totalnum;i++)
	{
		if ( !mib_chain_get(MIB_L2TP_ROUTE_TBL, i, (void *)&entry) )
			continue;

		if(tunnelName && (!strcmp(entry.tunnelName,tunnelName) || strstr(tunnelName, MAGIC_TUNNEL_NAME)))
		{
			sprintf(atta_sMAC_addr, "%X:%X:%X:%X:%X:%X", sMAC_addr[0], sMAC_addr[1], sMAC_addr[2], sMAC_addr[3], sMAC_addr[4], sMAC_addr[5]);
			sprintf(rtbl_sMAC_addr, "%X:%X:%X:%X:%X:%X", entry.sMAC[0], entry.sMAC[1], entry.sMAC[2], entry.sMAC[3], entry.sMAC[4], entry.sMAC[5]);
			if(!strcmp(atta_sMAC_addr,rtbl_sMAC_addr))
			{
				//AUG_PRT("%s-%d totalnum=%d sMAC_addr=%s\n",__func__,__LINE__,totalnum,atta_sMAC_addr);
				/*route list alredy exist!*/
					return i;
			}
		}
	}

	return -1;
}

int rtk_wan_vpn_l2tp_attach_check_url(unsigned char *tunnelName,char *url)
{
	int totalnum,i,checkIt=0;
	MIB_CE_L2TP_ROUTE_T entry;
	int cflags = REG_EXTENDED;
	regmatch_t pmatch[1];
	const size_t nmatch=1;
	regex_t reg;

	totalnum = mib_chain_total(MIB_L2TP_ROUTE_TBL); /* get chain record size */

	for(i=0;i<totalnum;i++)
	{
		if ( !mib_chain_get(MIB_L2TP_ROUTE_TBL, i, (void *)&entry) )
			continue;

		if(tunnelName && !strcmp(entry.tunnelName,tunnelName))
		{
			if(!strcmp(entry.url,url))
			{
				/*route list alredy exist!*/
				checkIt=1;
				return 1;
			}
		}
		else if(tunnelName && !strcmp(tunnelName,MAGIC_TUNNEL_NAME))
		{
			if(url[0] == '\0' || entry.url[0] == '\0')
				continue;

			if(regcomp(&reg, entry.url, cflags))
			{
				printf("failed to compile!\n");
				continue;
			}

			if(!regexec(&reg, url, nmatch, pmatch, 0) || !strcmp(entry.url,url)){
				regfree(&reg);
				return i;
			}

			regfree(&reg);
		}
	}
	return -1;
}

int rtk_wan_vpn_l2tp_attach_delete_url(unsigned char *tunnelName,char *url)
{
	int totalnum,i,checkIt=0;
	MIB_CE_L2TP_ROUTE_T entry;

	totalnum = mib_chain_total(MIB_L2TP_ROUTE_TBL); /* get chain record size */
	for(i=0;i<totalnum;i++)
	{
		if ( !mib_chain_get(MIB_L2TP_ROUTE_TBL, i, (void *)&entry) )
			continue;
		if(tunnelName && !strcmp(entry.tunnelName,tunnelName))
		{
			if(!strcmp(entry.url,url))
			{
				/*match route list!*/
				mib_chain_delete(MIB_L2TP_ROUTE_TBL,i);
				return 1;
			}
		}
	}
	return 0;
}

int rtk_wan_vpn_l2tp_attach_delete_dip(unsigned char *tunnelName,unsigned int ipv4_addr1,unsigned int ipv4_addr2)
{
	int totalnum,i,checkIt=0;
	MIB_CE_L2TP_ROUTE_T entry;

	totalnum = mib_chain_total(MIB_L2TP_ROUTE_TBL); /* get chain record size */
	for(i=0;i<totalnum;i++)
	{
		if ( !mib_chain_get(MIB_L2TP_ROUTE_TBL, i, (void *)&entry) )
			continue;
		if(tunnelName && !strcmp(entry.tunnelName,tunnelName))
		{
			if((ipv4_addr1 == entry.ipv4_src_start) && (ipv4_addr2 == entry.ipv4_src_end))
			{

				/*match route list!*/
				mib_chain_delete(MIB_L2TP_ROUTE_TBL,i);
				return 1;
			}
		}
	}
	return 0;
}

int rtk_wan_vpn_l2tp_attach_delete_smac(unsigned char *tunnelName, unsigned char* sMAC_addr)
{
	int totalnum,i,checkIt=0;
	MIB_CE_L2TP_ROUTE_T entry;
	unsigned char atta_sMAC_addr[20];
	unsigned char rtbl_sMAC_addr[20];

	totalnum = mib_chain_total(MIB_L2TP_ROUTE_TBL); /* get chain record size */
	for(i=0;i<totalnum;i++)
	{
		if ( !mib_chain_get(MIB_L2TP_ROUTE_TBL, i, (void *)&entry) )
			continue;

		if(tunnelName && !strcmp(entry.tunnelName,tunnelName))
		{
			sprintf(atta_sMAC_addr, "%X:%X:%X:%X:%X:%X", sMAC_addr[0], sMAC_addr[1], sMAC_addr[2], sMAC_addr[3], sMAC_addr[4], sMAC_addr[5]);
			sprintf(rtbl_sMAC_addr, "%X:%X:%X:%X:%X:%X", entry.sMAC[0], entry.sMAC[1], entry.sMAC[2], entry.sMAC[3], entry.sMAC[4], entry.sMAC[5]);
			if(!strcmp(atta_sMAC_addr,rtbl_sMAC_addr))
			{
				/*match route list!*/
				mib_chain_delete(MIB_L2TP_ROUTE_TBL,i);
				return 1;

			}
		}
	}
	return 0;
}

int NF_Flush_L2TP_Dynamic_URL_Route(unsigned char *tunnelName)
{
	char cmd[256], cmd2[256], *pcmd;
	unsigned int lineNumber, del_num = 0, delId;
	unsigned char chain_name[64]={0}, path[64]={0};
	const char *table_name;
	FILE *fp = NULL;

	table_name = NF_VPN_ROUTE_TABLE;
	sprintf(chain_name, NF_L2TP_ROUTE_CHAIN, tunnelName);

	sprintf(path, "/tmp/vpn_%d_%s-flush-url-rules", VPN_TYPE_L2TP, tunnelName);
	sprintf(cmd, "%s -t %s -nvL %s --line-number | grep \"/\\* .\\+ url:.\\+ \\*/\" > %s",
			IPTABLES, table_name, chain_name, path);

	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	if((fp=fopen(path, "r")))
	{
		while(fgets(cmd,sizeof(cmd),fp) != NULL)
		{
			if(sscanf(cmd,"%d %*d %*d %*s", &lineNumber) == 1)
			{
				delId = lineNumber - del_num;
				if(delId > 0){
					sprintf(cmd2, "%s -t %s -D %s %d", IPTABLES, table_name, chain_name, delId);
					va_cmd("/bin/sh", 2, 1, "-c", cmd2);
					del_num++;
				}
			}
		}
		fclose(fp);
	}

	unlink(path);
	return 0;
}

int NF_Flush_L2TP_Route(unsigned char *tunnelName)
{
	MIB_L2TP_T l2tpEntry;
	int l2tpEntryIdx;
	char cmd[256], *pcmd;
	unsigned int fw_mark, fw_mask;
	unsigned char chain_name[64];
	const char *table_name;

	if(getL2TPEntryByTunnelName(tunnelName, &l2tpEntry, &l2tpEntryIdx) == NULL){
		printf("Cannot found L2TP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}
#if 1
	if(rtk_wan_vpn_policy_route_get_mark(VPN_TYPE_L2TP, tunnelName, &fw_mark, &fw_mask) != 0){
		printf("Cannot get MARK of L2TP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}
#endif
	table_name = NF_VPN_ROUTE_TABLE;
	sprintf(chain_name, NF_L2TP_ROUTE_CHAIN, tunnelName);
	sprintf(cmd, "%s -t %s -D %s -m mark --mark 0x0/0x%x -j %s", IPTABLES, table_name, NF_VPN_ROUTE_CHAIN, fw_mask, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -F %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -X %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef VPN_MULTIPLE_RULES_BY_FILE
	/* Flush ipset iprange, subnet and smac chain and remove */
	snprintf(chain_name, sizeof(chain_name), NF_L2TP_IPSET_IPRANGE_CHAIN, tunnelName);
	snprintf(cmd, sizeof(cmd), "%s -X %s", IPSET, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	snprintf(chain_name, sizeof(chain_name), NF_L2TP_IPSET_IPSUBNET_CHAIN, tunnelName);
	snprintf(cmd, sizeof(cmd), "%s -X %s", IPSET, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	snprintf(chain_name, sizeof(chain_name), NF_L2TP_IPSET_SMAC_CHAIN, tunnelName);
	snprintf(cmd, sizeof(cmd), "%s -X %s", IPSET, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	snprintf(cmd, sizeof(cmd), "%s%s", VPN_L2TP_RULE_FILE, tunnelName);
	unlink(cmd);
	snprintf(cmd, sizeof(cmd), "%s%s", VPN_L2TP_IPSET_RESTORE_RULE_FILE, tunnelName);
	unlink(cmd);
#endif
	return 0;

}

int NF_Set_L2TP_Policy_Route(unsigned char *tunnelName)
{
	MIB_L2TP_T l2tpEntry;
	MIB_CE_L2TP_ROUTE_T entry;
	int total_entry,i,enable,l2tpEntryIdx;
	char cmd[256] = {0}, *pcmd = NULL, *pcmd2 = NULL;
	unsigned int fw_mark, fw_mask;
	unsigned char chain_name[64], comment_name[64];
	const char *table_name;

	if ( !mib_get_s(MIB_L2TP_ENABLE, (void *)&enable, sizeof(enable)) ){
		printf("MIB_L2TP_ENABLE is not exist!");
		return -1;
	}

	if(!enable){
		printf("MIB_L2TP_ENABLE is not enable!");
		return 0;
	}

	if(getL2TPEntryByTunnelName(tunnelName, &l2tpEntry, &l2tpEntryIdx) == NULL){
		printf("Cannot found L2TP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}

	if(rtk_wan_vpn_policy_route_get_mark(VPN_TYPE_L2TP, tunnelName, &fw_mark, &fw_mask) != 0){
		printf("Cannot get MARK of L2TP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}

	table_name = NF_VPN_ROUTE_TABLE;
	sprintf(chain_name, NF_L2TP_ROUTE_CHAIN, tunnelName);
	sprintf(cmd, "%s -t %s -F %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -X %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -N %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	sprintf(cmd, "%s -t %s -N %s", IPTABLES, table_name, NF_VPN_ROUTE_CHAIN);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -D %s -m mark --mark 0x0/0x%x -j %s", IPTABLES, table_name, NF_VPN_ROUTE_CHAIN, fw_mask, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -A %s -m mark --mark 0x0/0x%x -j %s", IPTABLES, table_name, NF_VPN_ROUTE_CHAIN, fw_mask, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	total_entry = mib_chain_total(MIB_L2TP_ROUTE_TBL);
	for(i=0;i<total_entry;i++)
	{
		if(mib_chain_get(MIB_L2TP_ROUTE_TBL, i, (void *)&entry) == 0)
			continue;

		if(!strcmp(entry.tunnelName,tunnelName))
		{
			if(((entry.ipv4_src_start || entry.ipv4_src_end) && (ATTACH_MODE_DIP==l2tpEntry.attach_mode)) ||
				((entry.sMAC[0]|entry.sMAC[1]|entry.sMAC[2]|entry.sMAC[3]|entry.sMAC[4]|entry.sMAC[5]) && (ATTACH_MODE_SMAC==l2tpEntry.attach_mode)))/*route by URL*/
			{
				cmd[0] = '\0';
				pcmd = cmd;
				sprintf(comment_name, NF_L2TP_ROUTE_COMMENT, i);
				pcmd += sprintf(pcmd, "%s -t %s -A %s", IPTABLES, table_name, chain_name);
				pcmd += sprintf(pcmd, " -m comment --comment \"%s\"", comment_name);
				pcmd += sprintf(pcmd, " -m mark --mark 0x0/0x%x", fw_mask);
				if(entry.ipv4_src_start || entry.ipv4_src_end)
				{
					pcmd += sprintf(pcmd, " --ipv4");
					if((entry.ipv4_src_start > 0 && entry.ipv4_src_end > 0) &&
						entry.ipv4_src_start != entry.ipv4_src_end)
					{
						pcmd += sprintf(pcmd, " -m iprange --dst-range %s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_start)));
						pcmd += sprintf(pcmd, "-%s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_end)));
					}
					else if(entry.ipv4_src_start > 0)
					{
						pcmd += sprintf(pcmd, " -d %s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_start)));
					}
					else
					{
						pcmd += sprintf(pcmd, " -d %s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_end)));
					}
				}
				else
				{
					pcmd += sprintf(pcmd, " -m mac --mac-source %02x:%02x:%02x:%02x:%02x:%02x",
										entry.sMAC[0], entry.sMAC[1], entry.sMAC[2], entry.sMAC[3], entry.sMAC[4], entry.sMAC[5]);
				}

				pcmd += sprintf(pcmd, " -j MARK --set-mark 0x%x/0x%x", fw_mark, fw_mask);
				//printf("%s\n", cmd);
				va_cmd("/bin/sh", 2, 1, "-c", cmd);
			}
			else if (ATTACH_MODE_ALL == l2tpEntry.attach_mode)
			{
				cmd[0] = '\0';
				pcmd = cmd;
				sprintf(comment_name, NF_L2TP_ROUTE_COMMENT, i);
				pcmd += sprintf(pcmd, "%s -t %s -A %s", IPTABLES, table_name, chain_name);
				pcmd += sprintf(pcmd, " -m comment --comment \"%s\"", comment_name);
				pcmd += sprintf(pcmd, " -m mark --mark 0x0/0x%x", fw_mask);
				pcmd2 = pcmd;
				if(entry.ipv4_src_start || entry.ipv4_src_end)
				{
					pcmd += sprintf(pcmd, " --ipv4");
					if((entry.ipv4_src_start > 0 && entry.ipv4_src_end > 0) &&
						entry.ipv4_src_start != entry.ipv4_src_end)
					{
						pcmd += sprintf(pcmd, " -m iprange --dst-range %s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_start)));
						pcmd += sprintf(pcmd, "-%s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_end)));
					}
					else if(entry.ipv4_src_start > 0)
					{
						pcmd += sprintf(pcmd, " -d %s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_start)));
					}
					else
					{
						pcmd += sprintf(pcmd, " -d %s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_end)));
					}
					pcmd += sprintf(pcmd, " -j MARK --set-mark 0x%x/0x%x", fw_mark, fw_mask);
					//printf("%s\n", cmd);
					va_cmd("/bin/sh", 2, 1, "-c", cmd);
				}
				if (isValidMacAddr(entry.sMAC))
				{
					pcmd += sprintf(pcmd2, " -m mac --mac-source %02x:%02x:%02x:%02x:%02x:%02x",
										entry.sMAC[0], entry.sMAC[1], entry.sMAC[2], entry.sMAC[3], entry.sMAC[4], entry.sMAC[5]);
					pcmd += sprintf(pcmd, " -j MARK --set-mark 0x%x/0x%x", fw_mark, fw_mask);
					pcmd += sprintf(pcmd, "\0");
					//printf("%s\n", cmd);
					va_cmd("/bin/sh", 2, 1, "-c", cmd);
				}

			}
		}
	}

	return 0;
}

int NF_Set_L2TP_Dynamic_URL_Route(char *tunnelName, int routeIdx, int weight, char *url, struct in_addr addr)
{
/*
	MIB_L2TP_T l2tpEntry;
	int enable,l2tpEntryIdx;
*/
	char cmd[256], *pcmd;
	unsigned int fw_mark, fw_mask;
	unsigned char chain_name[64], comment_name[256] = {0}, tmp_comment_name[256] = {0};
	const char *table_name;
/*
	if ( !mib_get_s(MIB_L2TP_ENABLE, (void *)&enable, sizeof(enable)) ){
		printf("MIB_L2TP_ENABLE is not exist!");
		return -1;
	}

	if(!enable){
		printf("MIB_L2TP_ENABLE is not enable!");
		return 0;
	}

	if(getL2TPEntryByTunnelName(tunnelName, &l2tpEntry, &l2tpEntryIdx) == NULL){
		printf("Cannot found L2TP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}
*/
#ifndef VPN_MULTIPLE_RULES_BY_FILE
	if(rtk_wan_vpn_policy_route_get_mark(VPN_TYPE_L2TP, tunnelName, &fw_mark, &fw_mask) != 0){
		printf("Cannot get MARK of L2TP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}

	table_name = NF_VPN_ROUTE_TABLE;
	sprintf(chain_name, NF_L2TP_ROUTE_CHAIN, tunnelName);

	cmd[0] = '\0';
	pcmd = cmd;
	sprintf(comment_name, NF_L2TP_ROUTE_COMMENT, routeIdx);
	pcmd += sprintf(pcmd, "%s -t %s -A %s", IPTABLES, table_name, chain_name);
	pcmd += sprintf(pcmd, " -m comment --comment \"%s url:%s\"", comment_name, url);
	pcmd += sprintf(pcmd, " -m mark --mark 0x0/0x%x", fw_mask);
	pcmd += sprintf(pcmd, " --ipv4");
	{
		pcmd += sprintf(pcmd, " -d %s", inet_ntoa(addr));
	}
	pcmd += sprintf(pcmd, " -j MARK --set-mark 0x%x/0x%x", fw_mark, fw_mask);

	va_cmd("/bin/sh", 2, 1, "-c", cmd);
#else

	FILE *fp = NULL;
	char file_path[128] = {0};
	char line_buf[256] = {0}, tmp_tunnel_name[64] = {0}, match_rule[64] = {0};
	int l2tp_rule_index = -1, rule_mode = -1;


	snprintf(file_path, sizeof(file_path), "%s%s", VPN_L2TP_RULE_FILE, tunnelName);
	//printf("%s: file = %s  !!!\n", __FUNCTION__, file_path);
	fp = fopen(file_path, "r");
	if (fp == NULL)
	{
		printf("%s: Open file = %s failed !!!\n", __FUNCTION__, file_path);
		return -1;
	}
	while (fgets(line_buf, sizeof(line_buf), fp) != NULL)
	{
		//fprintf(fp, "%s %d %d %d %d %d %d %d %d %s\n", vpn_tunnel_info->tunnelName, static_set, i, priority, attach_mode, rule_mode, reserve_idx, pkt_cnt, pkt_pass_lasttime, match_rule);
		if (sscanf(line_buf, "%s %*d %d %*d %*d %d %*d %*d %*d %s", &tmp_tunnel_name, &l2tp_rule_index, &rule_mode, &match_rule) != 4)
			continue;

		if (!strcmp(tunnelName, tmp_tunnel_name) && !strcmp(url, match_rule))
		{
			break;
		}
	}
	fclose(fp);
	snprintf(chain_name, sizeof(chain_name), NF_L2TP_IPSET_IPRANGE_CHAIN, tunnelName);
	snprintf(tmp_comment_name, sizeof(tmp_comment_name), NF_L2TP_IPSET_RULE_INDEX_COMMENT, rule_mode, l2tp_rule_index);
	/* If you want to change below format, please also change format in rtk_wan_vpn_get_ipset_packet_count_by_route_index() */
	snprintf(comment_name, sizeof(comment_name), "%s,url:%s", tmp_comment_name, url);
	snprintf(cmd, sizeof(cmd), "%s -A %s %s %s \"%s\" -exist", IPSET, chain_name, inet_ntoa(addr), IPSET_OPTION_COMMENT, comment_name);

	//printf("%s: cmd url = %s\n", __FUNCTION__, cmd);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif

	return 0;
}

#ifdef VPN_MULTIPLE_RULES_BY_FILE
int NF_Set_L2TP_Policy_Route_by_file(unsigned char *tunnelName)
{
	MIB_L2TP_T l2tpEntry;
	int enable = 0, l2tpEntryIdx = 0, priority = 0;
	char cmd[256] = {0};
	//char *pcmd = NULL, *pcmd2 = NULL;
	unsigned int fw_mark = 0, fw_mask = 0, l2tp_rule_index = 0;
	unsigned char chain_name[64] = {0}, comment_name[64] = {0};
	const char *table_name = NULL;
	char vpn_l2tp_file_path[128] = {0}, vpn_l2tp_ipset_restore_file_path[128] = {0}, tmp_tunnel_name[64] = {0}, line_buf[128] = {0}, match_rule[64] = {0};
	FILE *fp_file = NULL, *fp_ipset_restore_file = NULL;
	int tmp_attach_mode = 0, rule_mode = 0;
	struct in_addr ipv4_addr1 = {0}, ipv4_addr2 = {0},tmp_addr = {0};
	char *str_p = NULL;
	unsigned char ipset_iprange_chain_name[64] = {0}, ipset_subnet_chain_name[64] = {0}, ipset_smac_chain_name[64] = {0};
	unsigned char ipset_comment[128] = {0}, ipset_iprange_comment[128] = {0}, ipset_tmp_comment[256] = {0};
	int bits = 0;

	if ( !mib_get_s(MIB_L2TP_ENABLE, (void *)&enable, sizeof(enable)) ){
		printf("MIB_L2TP_ENABLE is not exist!");
		return -1;
	}

	if(!enable){
		printf("MIB_L2TP_ENABLE is not enable!");
		return 0;
	}

	if(getL2TPEntryByTunnelName(tunnelName, &l2tpEntry, &l2tpEntryIdx) == NULL){
		printf("Cannot found L2TP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}

	if(rtk_wan_vpn_policy_route_get_mark(VPN_TYPE_L2TP, tunnelName, &fw_mark, &fw_mask) != 0){
		printf("Cannot get MARK of L2TP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}

	table_name = NF_VPN_ROUTE_TABLE;
	sprintf(chain_name, NF_L2TP_ROUTE_CHAIN, tunnelName);
	sprintf(cmd, "%s -t %s -F %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -X %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -N %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	/* Set ipset 3 chain name. */
	snprintf(ipset_iprange_chain_name, sizeof(ipset_iprange_chain_name), NF_L2TP_IPSET_IPRANGE_CHAIN, tunnelName);
	snprintf(ipset_subnet_chain_name, sizeof(ipset_subnet_chain_name), NF_L2TP_IPSET_IPSUBNET_CHAIN, tunnelName);
	snprintf(ipset_smac_chain_name, sizeof(ipset_smac_chain_name), NF_L2TP_IPSET_SMAC_CHAIN, tunnelName);
	snprintf(ipset_tmp_comment, sizeof(ipset_tmp_comment), "%s%s", NF_L2TP_IPSET_RULE_INDEX_COMMENT, NF_L2TP_IPSET_IPRANGE_RULE_COMMENT);
	/* Firset delete ipset L2TP Tunnel chain */
	snprintf(cmd, sizeof(cmd), "%s -X %s > /dev/null 2>&1", IPSET, ipset_iprange_chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	snprintf(cmd, sizeof(cmd), "%s -X %s > /dev/null 2>&1", IPSET, ipset_subnet_chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	snprintf(cmd, sizeof(cmd), "%s -X %s > /dev/null 2>&1", IPSET, ipset_smac_chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	/* Create ipset L2TP Tunnel iprange chain(hash:ip). */
	snprintf(cmd, sizeof(cmd), "%s -N %s %s %s %s", IPSET, ipset_iprange_chain_name, IPSET_SETTYPE_HASHIP, IPSET_OPTION_COMMENT, IPSET_OPTION_COUNTERS);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	/* Create ipset L2TP Tunnel subnet chain(hash:net). */
	snprintf(cmd, sizeof(cmd), "%s -N %s %s %s %s", IPSET, ipset_subnet_chain_name, IPSET_SETTYPE_HASHNET, IPSET_OPTION_COMMENT, IPSET_OPTION_COUNTERS);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	/* Create ipset L2TP Tunnel mac chain(hash:net). */
	snprintf(cmd, sizeof(cmd), "%s -N %s %s %s %s", IPSET, ipset_smac_chain_name, IPSET_SETTYPE_HASHMAC, IPSET_OPTION_COMMENT, IPSET_OPTION_COUNTERS);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	snprintf(vpn_l2tp_ipset_restore_file_path, sizeof(vpn_l2tp_ipset_restore_file_path), "%s%s", VPN_L2TP_IPSET_RESTORE_RULE_FILE, tunnelName);
	fp_ipset_restore_file = fopen(vpn_l2tp_ipset_restore_file_path, "w");
	if (fp_ipset_restore_file == NULL)
	{
		printf("%s: Open file = %s failed !!!\n", __FUNCTION__, vpn_l2tp_file_path);
		return -1;
	}

	/* Create iptables chain. */
	sprintf(cmd, "%s -t %s -N %s", IPTABLES, table_name, NF_VPN_ROUTE_CHAIN);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -D %s -m mark --mark 0x0/0x%x -j %s", IPTABLES, table_name, NF_VPN_ROUTE_CHAIN, fw_mask, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -A %s -m mark --mark 0x0/0x%x -j %s", IPTABLES, table_name, NF_VPN_ROUTE_CHAIN, fw_mask, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	/* Create iptables rules to link to 3 ipset chain. */
	sprintf(cmd, "%s -t %s -A  %s -m mark --mark 0x0/0x%x -m set --match-set %s dst -j MARK --set-mark 0x%x/0x%x", IPTABLES, table_name, chain_name, fw_mask, ipset_iprange_chain_name, fw_mark, fw_mask);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -A  %s -m mark --mark 0x0/0x%x -m set --match-set %s dst -j MARK --set-mark 0x%x/0x%x", IPTABLES, table_name, chain_name, fw_mask, ipset_subnet_chain_name, fw_mark, fw_mask);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -A  %s -m mark --mark 0x0/0x%x -m set --match-set %s src -j MARK --set-mark 0x%x/0x%x", IPTABLES, table_name, chain_name, fw_mask, ipset_smac_chain_name, fw_mark, fw_mask);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	snprintf(vpn_l2tp_file_path, sizeof(vpn_l2tp_file_path), "%s%s", VPN_L2TP_RULE_FILE, tunnelName);

	fp_file = fopen(vpn_l2tp_file_path, "r");
	if (fp_file == NULL)
	{
		printf("%s: Open file = %s failed !!!\n", __FUNCTION__, vpn_l2tp_file_path);
	}
	else
	{
		while (fgets(line_buf, sizeof(line_buf), fp_file) != NULL)
		{
			//fprintf(fp, "%s %d %d %d %d %d %d %d %d %s\n", vpn_tunnel_info->tunnelName, static_set, i, priority, attach_mode, rule_mode, reserve_idx, pkt_cnt, pkt_pass_lasttime, match_rule);
			if (sscanf(line_buf, "%s %*d %d %d %d %d %*d %*d %*d %s", &tmp_tunnel_name, &l2tp_rule_index, &priority, &tmp_attach_mode, &rule_mode,  &match_rule) != 6)
			continue;

			/*
			 * We will use ipset restore file to add ipset rule instead of entering command.
			 * Format will not be include ipset in the head.
			 * Ex:
			 *    -A aaa 192.168.1.2 -exist comment "hello world"
			 *    -A aaa 223.205.23.116/24 -exist comment "hello world"
			 *    -A aaa AA:BB:CC:DD:EE:FF -exist comment
			 */
			if (!strcmp(tunnelName, tmp_tunnel_name))
			{
				snprintf(ipset_comment, sizeof(ipset_comment), NF_L2TP_IPSET_RULE_INDEX_COMMENT, rule_mode, l2tp_rule_index);
				//typedef enum { ATTACH_MODE_ALL=0, ATTACH_MODE_DIP=1, ATTACH_MODE_SMAC=2, ATTACH_MODE_NONE } ATTACH_MODE_T;
				if (((str_p = strstr(match_rule, "-")) || isIPAddr(match_rule) == 1) && tmp_attach_mode != ATTACH_MODE_SMAC) //IP case
				{
					if (str_p != NULL)
					{
						*str_p = '\0';
						if (isIPAddr(match_rule) != 1)
						{
							*str_p = '-';
							goto domain_case;
						}
						if (isIPAddr(str_p + 1) != 1)
						{
							*str_p = '-';
							goto domain_case;
						}
						*str_p = '-';
						/* ip range comment is special like: "l2tp_ruletype_3_route_idx_1,192.168.1.10-192.168.15" */
						snprintf(ipset_iprange_comment, sizeof(ipset_iprange_comment), ipset_tmp_comment, rule_mode, l2tp_rule_index, match_rule);
#if 0 // We don't need to make range ip to be single IP if start ip = end ip
						if (!strcmp(match_rule, str_p + 1)) //ip_start = ip_end
							pcmd += sprintf(pcmd, " -d %s", match_rule);
						else //ip_start != ip_end
#endif
						//snprintf(cmd, sizeof(cmd), "%s -A %s %s -exist comment \"%s\"", IPSET, ipset_iprange_chain_name, match_rule, ipset_iprange_comment);
						fprintf(fp_ipset_restore_file, "-A %s %s -exist comment \"%s\"\n", ipset_iprange_chain_name, match_rule, ipset_iprange_comment);
					}
					else //pure 1 IP
					{
						//snprintf(cmd, sizeof(cmd), "%s -A %s %s -exist comment \"%s\"", IPSET, ipset_iprange_chain_name, match_rule, ipset_comment);
						fprintf(fp_ipset_restore_file, "-A %s %s -exist comment \"%s\"\n", ipset_iprange_chain_name, match_rule, ipset_comment);
					}
				}
				else if ((str_p = strstr(match_rule, "/")) && tmp_attach_mode != ATTACH_MODE_SMAC) //IP subnet case
				{
					*str_p = '\0';
					bits = atoi(str_p + 1);
					if (isIPAddr(match_rule) != 1 || !(bits > 0 && bits <= 32))
					{
						*str_p = '/';
						goto domain_case;
					}
					*str_p = '/';
					//ipset -A ipset_subnet_chain_name 192.168.1.10/24 -exist comment "l2tp_ruletype_%d_route_idx_%d"
					//snprintf(cmd, sizeof(cmd), "%s -A %s %s -exist comment \"%s\"", IPSET, ipset_subnet_chain_name, match_rule, ipset_comment);
					fprintf(fp_ipset_restore_file, "-A %s %s -exist comment \"%s\"\n", ipset_subnet_chain_name, match_rule, ipset_comment);
				}
				else if (isValidMacString(match_rule) && (tmp_attach_mode != ATTACH_MODE_DIP)) //SMAC case
				{
					//snprintf(cmd, sizeof(cmd), "%s -A %s %s -exist comment \"%s\"", IPSET, ipset_smac_chain_name, match_rule, ipset_comment);
					fprintf(fp_ipset_restore_file, "-A %s %s -exist comment \"%s\"\n", ipset_smac_chain_name, match_rule, ipset_comment);
				}
				else // domain case
				{
					rtk_wan_vpn_preset_url_policy_route_by_file(VPN_TYPE_L2TP, tunnelName, l2tp_rule_index, priority, match_rule);
				}

				continue;
domain_case:
				rtk_wan_vpn_preset_url_policy_route_by_file(VPN_TYPE_L2TP, tunnelName, l2tp_rule_index, priority, match_rule);
			}
		}

		if (fp_ipset_restore_file)
		{
			fclose(fp_ipset_restore_file);
			fp_ipset_restore_file = NULL;
		}
		snprintf(cmd, sizeof(cmd), "%s %s -f %s", IPSET, IPSET_COMMAND_RESTORE, vpn_l2tp_ipset_restore_file_path);
		//printf("cmd file = %s\n", cmd);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
	}
	if (fp_ipset_restore_file)
		fclose(fp_ipset_restore_file);
	if (fp_file)
		fclose(fp_file);


	return 0;
}
#endif

#endif //#ifdef CONFIG_USER_L2TPD_L2TPD

#ifdef YUEME_4_0_SPEC
const char NF_VPN_SERVER_CHAIN[] = "VPN_SERVER_ROUTE";
const char L2TP_ROUTE_CHAIN[] = "L2TP_SERVER_ROUTE";
const char PPTP_ROUTE_CHAIN[] = "PPTP_SERVER_ROUTE";
#endif

#ifdef CONFIG_E8B
int rtk_wan_vpn_attach_policy_route_init(void)
{
	char cmd[256];

	sprintf(cmd, "%s -t %s -N %s", IPTABLES, NF_VPN_ROUTE_TABLE, NF_VPN_ROUTE_CHAIN);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -D %s -i br+ -j %s", IPTABLES, NF_VPN_ROUTE_TABLE, FW_PREROUTING, NF_VPN_ROUTE_CHAIN);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -A %s -i br+ -j %s", IPTABLES, NF_VPN_ROUTE_TABLE, FW_PREROUTING, NF_VPN_ROUTE_CHAIN);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	/* Now don't add OUTPUT chain to jump VPN_POLICY_ROUTE. Because when use SMAC, xt_tables show
	 * ip_tables: mac match: used from hooks PREROUTING/OUTPUT, but only valid from PREROUTING/INPUT/FORWARD
	 * means OUTPUT chain doesn't support mac match.
	 */
	//sprintf(cmd, "%s -t %s -D %s -j %s", IPTABLES, NF_VPN_ROUTE_TABLE, FW_OUTPUT, NF_VPN_ROUTE_CHAIN);
	//va_cmd("/bin/sh", 2, 1, "-c", cmd);
	//sprintf(cmd, "%s -t %s -A %s -j %s", IPTABLES, NF_VPN_ROUTE_TABLE, FW_OUTPUT, NF_VPN_ROUTE_CHAIN);
	//va_cmd("/bin/sh", 2, 1, "-c", cmd);
	
#ifdef YUEME_4_0_SPEC
    	//iptables -t mangle -N VPN_SERVER_ROUTE
    	sprintf(cmd, "%s -t %s -N %s", IPTABLES, NF_VPN_ROUTE_TABLE, NF_VPN_SERVER_CHAIN);
    	va_cmd("/bin/sh", 2, 1, "-c", cmd);

    	//iptables -t mangle -A OUTPUT -j VPN_SERVER_ROUTE
    	sprintf(cmd, "%s -t %s -A %s -j %s", IPTABLES, NF_VPN_ROUTE_TABLE, FW_OUTPUT, NF_VPN_SERVER_CHAIN);
    	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	
#endif
	return 0;
}
#endif

unsigned long long rtk_wan_vpn_get_netfilter_packet_count(VPN_TYPE_T vpn_type, unsigned char *tunnelName)
{
	FILE *fp = NULL;
	char cmd[256];
	unsigned char chain_name[64]={0}, path[64]={0};
	const char *table_name;
	unsigned long long packetCount=0, totalCount=0;

	switch(vpn_type)
	{
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
		case VPN_TYPE_L2TP:
			{
				table_name = NF_VPN_ROUTE_TABLE;
				sprintf(chain_name, NF_L2TP_ROUTE_CHAIN, tunnelName);
				break;
			}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP)  || defined(CONFIG_USER_PPTP_CLIENT)
		case VPN_TYPE_PPTP:
			{
				table_name = NF_VPN_ROUTE_TABLE;
				sprintf(chain_name, NF_PPTP_ROUTE_CHAIN, tunnelName);
				break;
			}
#endif
		default:
			return -1;
	}
	sprintf(path, "/tmp/vpn_%d_%s-attch-rules", vpn_type, tunnelName);
	sprintf(cmd, "%s -t %s -nvL %s > %s", IPTABLES, table_name, chain_name, path);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	if((fp=fopen(path, "r")))
	{
		while(fgets(cmd,sizeof(cmd),fp)!=NULL)
		{
			if(sscanf(cmd,"%lld %*d %*s", &packetCount) == 1)
			{
				totalCount+=packetCount;
			}
		}
		fclose(fp);
	}
	unlink(path);

	return totalCount;
}

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
void domain_to_regular_expression_cmcc(unsigned char *domain_name, unsigned char *regular_expression)
{
	int i=0;
	unsigned char *ptr;
	unsigned char tmp_domain_name[128]={0};

	if(domain_name == NULL){
		return;
	}

	if(domain_name[0] == '\0'){
		return;
	}

	if(domain_name[0] != '*'){
		sprintf(tmp_domain_name, "*.");
		strncat(tmp_domain_name, domain_name,sizeof(tmp_domain_name)-1-strlen(tmp_domain_name));
	}
	else
		strncpy(tmp_domain_name, domain_name, sizeof(tmp_domain_name));
	ptr = regular_expression;
	while(i<=strlen(tmp_domain_name))
	{
		if(tmp_domain_name[i] == '*' && tmp_domain_name[i+1] == '.')
		{
			sprintf(ptr, "(.+\\.)*");
			ptr+=7;
			i+=2;
		}
		else if(tmp_domain_name[i] == '*' && tmp_domain_name[i+1] == '\0')
		{
			sprintf(ptr, "(.+)*");
			ptr+=5;
			i+=2;
		}
		else
		{
			sprintf(ptr, "%c", tmp_domain_name[i]);
			ptr+=1;
			i+=1;
		}
	}

	//AUG_PRT("regular_expression=%s \n", regular_expression);
}
#endif

void domain_to_regular_expression(unsigned char *domain_name, unsigned char *regular_expression)
{
	int i=0;
	unsigned char *ptr;

	if(domain_name == NULL){
		return;
	}

	if(domain_name[0] == '\0'){
		return;
	}

	ptr = regular_expression;
	while(i<=strlen(domain_name))
	{
		if(domain_name[i] == '*' && domain_name[i+1] == '.')
		{
			sprintf(ptr, "(.+\\.)*");
			ptr+=7;
			i+=2;
		}
		else if(domain_name[i] == '*' && domain_name[i+1] == '\0')
		{
			sprintf(ptr, "(.+)*");
			ptr+=5;
			i+=2;
		}
		else
		{
			sprintf(ptr, "%c", domain_name[i]);
			ptr+=1;
			i+=1;
		}
	}

	//AUG_PRT("regular_expression=%s \n", regular_expression);
}

const char VPN_URL_ROUTE_TBL[] = "/var/rtk_wan_vpn_url_route_idx";
const char VPN_URL_ROUTE_TBL_TMP[] = "/var/rtk_wan_vpn_url_route_idx_tmp";
const char VPN_URL_ROUTE_TBL_LOCK[] = "/var/.rtk_wan_vpn_url_route_idx_lock";

int rtk_wan_vpn_config_url_policy_route(char *url, struct in_addr addr, int mode)
{
	FILE *fp = NULL, *fp_lock=NULL;
	int totalEntry, i, weight, type, route_idx, do_regcomp, ret;
	char line[512];
	char match_rule[256], name[32];
	regex_t reg;
	regmatch_t pmatch[1];
	const size_t nmatch=1;

	VPN_READ_LOCK(VPN_URL_ROUTE_TBL_LOCK, fp_lock);
	if(fp_lock == NULL)
	{
		fprintf(stderr, "[%s] Cannot get LOCK (%s)!\n", __FUNCTION__, VPN_URL_ROUTE_TBL_LOCK);
		return -1;
	}

	if(!(fp = fopen(VPN_URL_ROUTE_TBL, "r")))
	{
		fprintf(stderr, "[%s] ERROR to open file(%s)! %s\n", __FUNCTION__, VPN_URL_ROUTE_TBL, strerror(errno));
		VPN_RW_UNLOCK(fp_lock);
		return -2;
	}

	while(fgets(line, sizeof(line)-1, fp) != NULL)
	{
		//vpnType, tunnelName, mibIdx, weight, url
		if(sscanf(line, "%d %s %d %d %s\n", &type, name, &route_idx, &weight, match_rule) != 5)
		{
			printf("[%s] Error to scan file(%s), line= %s\n", __FUNCTION__, VPN_URL_ROUTE_TBL, line);
			continue;
		}

		do_regcomp = 0;
		if(strspn(match_rule, "*+()[]\\{},") > 0)
		{
			if((ret = regcomp(&reg, match_rule, REG_EXTENDED)) != 0)
			{
				fprintf(stderr, "[%s] error regcomp for rule(%s), error code=%d  \n", __FUNCTION__, match_rule, ret);
				continue;
			}
			do_regcomp = 1;
		}

		if((do_regcomp && regexec(&reg, url, nmatch, pmatch, 0) == 0) ||
			(!do_regcomp && strstr(url, match_rule)))
		{
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
			if(type==VPN_TYPE_L2TP)
			{
				if(mode)
				{
					NF_Set_L2TP_Dynamic_URL_Route(name, route_idx, weight, url, addr);
				}
				else
				{
					NF_Del_L2TP_Dynamic_URL_Route(name, route_idx, weight, url, addr);
				}
			}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
			if(type==VPN_TYPE_PPTP)
			{
				if(mode)
				{
					NF_Set_PPTP_Dynamic_URL_Route(name, route_idx, weight, url, addr);
				}
				else
				{
					NF_Del_PPTP_Dynamic_URL_Route(name, route_idx, weight, url, addr);
				}
			}
#endif
		}

		if(do_regcomp) regfree(&reg);
	}

	fclose(fp);
	VPN_RW_UNLOCK(fp_lock);

	return 0;
}

int rtk_wan_vpn_flush_url_policy_route(VPN_TYPE_T vpn_type, unsigned char *tunnelName)
{
	FILE *fp = NULL, *fp_tmp = NULL, *fp_lock = NULL;
	int totalEntry, i, weight, type, route_idx;
	char line[512];
	char match_rule[256], name[32];
	int ret=-1;

	VPN_WRITE_LOCK(VPN_URL_ROUTE_TBL_LOCK, fp_lock);
	if(fp_lock == NULL)
	{
		fprintf(stderr, "[%s] Cannot get LOCK (%s)!\n", __FUNCTION__, VPN_URL_ROUTE_TBL_LOCK);
		return -1;
	}

	if(!(fp = fopen(VPN_URL_ROUTE_TBL, "r")))
	{
		fprintf(stderr, "[%s] ERROR to open file(%s)! %s\n", __FUNCTION__, VPN_URL_ROUTE_TBL, strerror(errno));
		VPN_RW_UNLOCK(fp_lock);
		return -2;
	}

	if(!(fp_tmp = fopen(VPN_URL_ROUTE_TBL_TMP, "w")))
	{
		fprintf(stderr, "[%s] ERROR to open file(%s)! %s\n", __FUNCTION__, VPN_URL_ROUTE_TBL_TMP, strerror(errno));
		fclose(fp);
#ifdef RESTRICT_PROCESS_PERMISSION
		chmod(VPN_URL_ROUTE_TBL_TMP,0666);
#endif
		VPN_RW_UNLOCK(fp_lock);
		return -2;
	}

	while(fgets(line, sizeof(line)-1, fp) != NULL)
	{
		//vpnType, tunnelName, mibIdx, weight, url
		if(sscanf(line, "%d %s %d %d %s\n", &type, name, &route_idx, &weight, match_rule) != 5)
		{
			printf("[%s] Error to scan file(%s), line= %s\n", __FUNCTION__, VPN_URL_ROUTE_TBL_TMP, line);
			continue;
		}
		if(type == vpn_type && !strcmp(name, tunnelName))
			continue;

		fprintf(fp_tmp, "%d %s %d %d %s\n", type, name, route_idx, weight, match_rule);
	}

	fclose(fp_tmp);
	fclose(fp);
	unlink(VPN_URL_ROUTE_TBL);
	ret=rename(VPN_URL_ROUTE_TBL_TMP, VPN_URL_ROUTE_TBL);
	if(ret)
		printf("rename error!\n");
	VPN_RW_UNLOCK(fp_lock);

	return 0;
}

int rtk_wan_vpn_preset_url_policy_route(VPN_TYPE_T vpn_type, unsigned char *tunnelName)
{
	FILE *fp = NULL, *fp_lock = NULL;
	int totalEntry, i, weight;
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	MIB_CE_L2TP_ROUTE_T l2tp_route_entry;
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	MIB_CE_PPTP_ROUTE_T pptp_route_entry;
#endif
	unsigned char regular_expression[MAX_DOMAIN_LENGTH] = {0};

	VPN_WRITE_LOCK(VPN_URL_ROUTE_TBL_LOCK, fp_lock);
	if(fp_lock == NULL)
	{
		fprintf(stderr, "[%s] Cannot get LOCK (%s)!\n", __FUNCTION__, VPN_URL_ROUTE_TBL_LOCK);
		return -1;
	}
	if(!(fp = fopen(VPN_URL_ROUTE_TBL, "a")))
	{
		fprintf(stderr, "[%s] ERROR to open file(%s)! %s\n", __FUNCTION__, VPN_URL_ROUTE_TBL, strerror(errno));
		VPN_RW_UNLOCK(fp_lock);
		return -2;
	}

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if(vpn_type==VPN_TYPE_L2TP)
	{
		totalEntry = mib_chain_total(MIB_L2TP_ROUTE_TBL);
		for(i=0;i<totalEntry;i++)
		{
			if(mib_chain_get(MIB_L2TP_ROUTE_TBL, i, (void *)&l2tp_route_entry) == 0)
				continue;

			if(!strcmp(l2tp_route_entry.tunnelName, tunnelName) && l2tp_route_entry.url[0] != '\0')
			{
				weight = VPN_PRIO_7-l2tp_route_entry.priority;
				domain_to_regular_expression(l2tp_route_entry.url , regular_expression);
				//vpnType, tunnelName, mibIdx, weight, url
				fprintf(fp, "%d %s %d %d %s\n", vpn_type, tunnelName, i, weight, regular_expression);
			}
		}
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if(vpn_type==VPN_TYPE_PPTP)
	{
		totalEntry = mib_chain_total(MIB_PPTP_ROUTE_TBL);
		for(i=0;i<totalEntry;i++)
		{
			if(mib_chain_get(MIB_PPTP_ROUTE_TBL, i, (void *)&pptp_route_entry) == 0)
				continue;

			if(!strcmp(pptp_route_entry.tunnelName, tunnelName) && pptp_route_entry.url[0] != '\0')
			{
				weight = VPN_PRIO_7-pptp_route_entry.priority;

				domain_to_regular_expression(l2tp_route_entry.url , regular_expression);
				//vpnType, tunnelName, mibIdx, weight, url
				fprintf(fp, "%d %s %d %d %s\n", vpn_type, tunnelName, i, weight, regular_expression);
			}
		}
	}
#endif

	fclose(fp);
	VPN_RW_UNLOCK(fp_lock);

	return 0;
}

int rtk_wan_vpn_flush_attach_rules(VPN_TYPE_T vpn_type, unsigned char *tunnelName, VPN_AC_T mode) //mode: 0=all, 1=dynimic
{
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if(vpn_type==VPN_TYPE_L2TP)
	{
		if(mode == VPN_AC_ALL){
			NF_Flush_L2TP_Route(tunnelName);
			rtk_wan_vpn_flush_url_policy_route(VPN_TYPE_L2TP, tunnelName);
		}
		else if(mode == VPN_AC_URL){ //dynamic url only

		}
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if(vpn_type == VPN_TYPE_PPTP)
	{
		if(mode == VPN_AC_ALL){
			NF_Flush_PPTP_Route(tunnelName);
			rtk_wan_vpn_flush_url_policy_route(VPN_TYPE_PPTP, tunnelName);
		}
		else if(mode == VPN_AC_URL){ //dynamic url only

		}
	}
#endif
	return 0;
}

int rtk_wan_vpn_flush_attach_rules_all(VPN_TYPE_T vpn_type, VPN_AC_T mode) //mode: 0=all, 1=dynimic
{
	int entryNum, i;
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	MIB_L2TP_T l2tpEntry;
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	MIB_PPTP_T pptpEntry;
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if(vpn_type==VPN_TYPE_L2TP)
	{
		entryNum = mib_chain_total(MIB_L2TP_TBL);
		for (i=0; i<entryNum; i++)
		{
			if (!mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tpEntry))
			{
				continue;
			}
			rtk_wan_vpn_flush_attach_rules(VPN_TYPE_L2TP, l2tpEntry.tunnelName, mode);
		}
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if(vpn_type == VPN_TYPE_PPTP)
	{
		entryNum = mib_chain_total(MIB_PPTP_TBL);
		for (i=0; i<entryNum; i++)
		{
			if (!mib_chain_get(MIB_PPTP_TBL, i, (void *)&pptpEntry))
			{
				continue;
			}
			rtk_wan_vpn_flush_attach_rules(VPN_TYPE_PPTP, pptpEntry.tunnelName, mode);
		}
	}
#endif
	return 0;
}

int rtk_wan_vpn_set_attach_rules(VPN_TYPE_T vpn_type, unsigned char *tunnelName, VPN_AC_T mode) //mode: 0=all, 1=dynimic
{
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if(vpn_type==VPN_TYPE_L2TP)
	{
		if(mode == VPN_AC_ALL){
/* If you want to support VPNConnection command with MIB setting and dbus rule with file,
 * maybe you can delete VPN_MULTIPLE_RULES_BY_FILE MACRO. */
#ifndef VPN_MULTIPLE_RULES_BY_FILE
			NF_Set_L2TP_Policy_Route(tunnelName);
			rtk_wan_vpn_preset_url_policy_route(VPN_TYPE_L2TP, tunnelName);
#else
			NF_Set_L2TP_Policy_Route_by_file(tunnelName);
#endif
		}
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if(vpn_type == VPN_TYPE_PPTP)
	{
		if(mode == VPN_AC_ALL){
/* If you want to support VPNConnection command with MIB setting and dbus rule with file,
 * maybe you can delete VPN_MULTIPLE_RULES_BY_FILE MACRO. */
#ifndef VPN_MULTIPLE_RULES_BY_FILE
			NF_Set_PPTP_Policy_Route(tunnelName);
			rtk_wan_vpn_preset_url_policy_route(VPN_TYPE_PPTP, tunnelName);
#else
			NF_Set_PPTP_Policy_Route_by_file(tunnelName);
#endif
		}
	}
#endif
	return 0;
}

int rtk_wan_vpn_set_attach_rules_all(VPN_TYPE_T vpn_type, VPN_AC_T mode) //mode: 0=all, 1=dynimic
{
	int entryNum, i;
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	MIB_L2TP_T l2tpEntry;
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP)  || defined(CONFIG_USER_PPTP_CLIENT)
	MIB_PPTP_T pptpEntry;
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if(vpn_type==VPN_TYPE_L2TP)
	{
		entryNum = mib_chain_total(MIB_L2TP_TBL);
		for (i=0; i<entryNum; i++)
		{
			if (!mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tpEntry))
			{
				continue;
			}
			rtk_wan_vpn_set_attach_rules(VPN_TYPE_L2TP, l2tpEntry.tunnelName, mode);
		}
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if(vpn_type == VPN_TYPE_PPTP)
	{
		entryNum = mib_chain_total(MIB_PPTP_TBL);
		for (i=0; i<entryNum; i++)
		{
			if (!mib_chain_get(MIB_PPTP_TBL, i, (void *)&pptpEntry))
			{
				continue;
			}
			rtk_wan_vpn_set_attach_rules(VPN_TYPE_PPTP, pptpEntry.tunnelName, mode);
		}
	}
#endif
	return 0;
}

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
int rtk_wan_vpn_pptp_attach_check_url(unsigned char *tunnelName,char *url)
{
	int totalnum,i,checkIt=0;
	MIB_CE_PPTP_ROUTE_T entry;
	int cflags = REG_EXTENDED;
	regmatch_t pmatch[1];
	const size_t nmatch=1;
	regex_t reg;

	totalnum = mib_chain_total(MIB_PPTP_ROUTE_TBL); /* get chain record size */

	for(i=0;i<totalnum;i++)
	{
		if ( !mib_chain_get(MIB_PPTP_ROUTE_TBL, i, (void *)&entry) )
			continue;
		if(!strcmp(entry.tunnelName,tunnelName))
		{
			if(!strcmp(entry.url,url))
			{
				/*route list alredy exist!*/
				checkIt=1;
				return 1;
			}
		}
		else if(!strcmp(tunnelName, MAGIC_TUNNEL_NAME))
		{
			if(url[0] == '\0' || entry.url[0] == '\0')
				continue;

			if(regcomp(&reg, entry.url, cflags))
			{
				printf("failed to compile!\n");
				continue;
			}

			if(!regexec(&reg, url, nmatch, pmatch, 0) || !strcmp(entry.url,url)){
				regfree(&reg);
				return i;
			}

			regfree(&reg);
		}
	}

	if(strstr(tunnelName, MAGIC_TUNNEL_NAME))
		return -1;
	else
		return 0;

}

int rtk_wan_vpn_pptp_attach_check_dip(unsigned char *tunnelName,unsigned int ipv4_addr1,unsigned int ipv4_addr2)
{
	unsigned int ip_start = 0, ip_end = 0;
#ifndef VPN_MULTIPLE_RULES_BY_FILE
	int totalnum,i,checkIt=0;
	MIB_CE_PPTP_ROUTE_T entry;

	totalnum = mib_chain_total(MIB_PPTP_ROUTE_TBL); /* get chain record size */
	//AUG_PRT("%s-%d totalnum=%d ipv4_addr1=%x, ipv4_addr2=%x\n",__func__,__LINE__,totalnum,ipv4_addr1,ipv4_addr2);
	for(i=0;i<totalnum;i++)
	{
		if ( !mib_chain_get(MIB_PPTP_ROUTE_TBL, i, (void *)&entry) )
			continue;
		if(!strcmp(entry.tunnelName,tunnelName) || strstr(tunnelName, MAGIC_TUNNEL_NAME))
		{
			if(ipv4_addr1>=entry.ipv4_src_start && ipv4_addr1<=entry.ipv4_src_end){
				//AUG_PRT("%s-%d i=%d\n",__func__,__LINE__,i);
				return i;
			}
		}
	}
#else
	ip_start = ntohl(ipv4_addr1);
	ip_end = ntohl(ipv4_addr2);

	if (ip_end < ip_start)
		return 1;
#endif
	return -1;
}

int rtk_wan_vpn_pptp_attach_check_smac(unsigned char *tunnelName, unsigned char* sMAC_addr)
{
	int totalnum,i,checkIt=0;
	MIB_CE_PPTP_ROUTE_T entry;
	unsigned char atta_sMAC_addr[20];
	unsigned char rtbl_sMAC_addr[20];

	totalnum = mib_chain_total(MIB_PPTP_ROUTE_TBL); /* get chain record size */

	//AUG_PRT("%s-%d totalnum=%d sMAC_addr=%X:%X:%X:%X:%X:%X \n",__func__,__LINE__,totalnum,sMAC_addr[0],sMAC_addr[1],sMAC_addr[2],sMAC_addr[3],sMAC_addr[4],sMAC_addr[5]);

	for(i=0;i<totalnum;i++)
	{
		if ( !mib_chain_get(MIB_PPTP_ROUTE_TBL, i, (void *)&entry) )
			continue;

		if(!strcmp(entry.tunnelName,tunnelName) || strstr(tunnelName, MAGIC_TUNNEL_NAME))
		{
			sprintf(atta_sMAC_addr, "%X:%X:%X:%X:%X:%X", sMAC_addr[0], sMAC_addr[1], sMAC_addr[2], sMAC_addr[3], sMAC_addr[4], sMAC_addr[5]);
			sprintf(rtbl_sMAC_addr, "%X:%X:%X:%X:%X:%X", entry.sMAC[0], entry.sMAC[1], entry.sMAC[2], entry.sMAC[3], entry.sMAC[4], entry.sMAC[5]);
			if(!strcmp(atta_sMAC_addr,rtbl_sMAC_addr))
			{
				//AUG_PRT("%s-%d totalnum=%d sMAC_addr=%s \n",__func__,__LINE__,totalnum,atta_sMAC_addr);
				/*route list alredy exist!*/
				return i;
			}
		}
	}

	return -1;
}

int rtk_wan_vpn_pptp_attach_delete_url(unsigned char *tunnelName,char *url)
{
	int totalnum,i,checkIt=0;
	MIB_CE_PPTP_ROUTE_T entry;

	totalnum = mib_chain_total(MIB_PPTP_ROUTE_TBL); /* get chain record size */
	for(i=0;i<totalnum;i++)
	{
		if ( !mib_chain_get(MIB_PPTP_ROUTE_TBL, i, (void *)&entry) )
			continue;
		if(!strcmp(entry.tunnelName,tunnelName))
		{
			if(!strcmp(entry.url,url))
			{
				/*match route list!*/
				mib_chain_delete(MIB_PPTP_ROUTE_TBL,i);
				return 1;
			}
		}
	}
	return 0;
}

int rtk_wan_vpn_pptp_attach_delete_dip(unsigned char *tunnelName,unsigned int ipv4_addr1,unsigned int ipv4_addr2)
{
	int totalnum,i,checkIt=0;
	MIB_CE_PPTP_ROUTE_T entry;

	totalnum = mib_chain_total(MIB_PPTP_ROUTE_TBL); /* get chain record size */
	for(i=0;i<totalnum;i++)
	{
		if ( !mib_chain_get(MIB_PPTP_ROUTE_TBL, i, (void *)&entry) )
			continue;
		if(!strcmp(entry.tunnelName,tunnelName) || strstr(tunnelName, MAGIC_TUNNEL_NAME))
		{
			if((ipv4_addr1 == entry.ipv4_src_start) && (ipv4_addr2 == entry.ipv4_src_end))
			{

				/*match route list!*/
				mib_chain_delete(MIB_PPTP_ROUTE_TBL,i);
				return 1;
			}
		}
	}
	return 0;
}

int rtk_wan_vpn_pptp_attach_delete_smac(unsigned char *tunnelName, unsigned char* sMAC_addr)
{
	int totalnum,i,checkIt=0;
	MIB_CE_PPTP_ROUTE_T entry;
	unsigned char atta_sMAC_addr[20];
	unsigned char rtbl_sMAC_addr[20];

	totalnum = mib_chain_total(MIB_PPTP_ROUTE_TBL); /* get chain record size */
	for(i=0;i<totalnum;i++)
	{
		if ( !mib_chain_get(MIB_PPTP_ROUTE_TBL, i, (void *)&entry) )
			continue;

		if(!strcmp(entry.tunnelName,tunnelName) || strstr(tunnelName, MAGIC_TUNNEL_NAME))
		{
			sprintf(atta_sMAC_addr, "%X:%X:%X:%X:%X:%X", sMAC_addr[0], sMAC_addr[1], sMAC_addr[2], sMAC_addr[3], sMAC_addr[4], sMAC_addr[5]);
			sprintf(rtbl_sMAC_addr, "%X:%X:%X:%X:%X:%X", entry.sMAC[0], entry.sMAC[1], entry.sMAC[2], entry.sMAC[3], entry.sMAC[4], entry.sMAC[5]);
			if(!strcmp(atta_sMAC_addr,rtbl_sMAC_addr))
			{
				/*match route list!*/
				mib_chain_delete(MIB_PPTP_ROUTE_TBL,i);
				return 1;

			}
		}
	}
	return 0;
}

int NF_Flush_PPTP_Dynamic_URL_Route(unsigned char *tunnelName)
{
	char cmd[256], cmd2[256], *pcmd;
	unsigned int lineNumber, del_num = 0, delId;
	unsigned char chain_name[64]={0}, path[64]={0};
	const char *table_name;
	FILE *fp = NULL;

	table_name = NF_VPN_ROUTE_TABLE;
	sprintf(chain_name, NF_PPTP_ROUTE_CHAIN, tunnelName);

	sprintf(path, "/tmp/vpn_%d_%s-flush-url-rules", VPN_TYPE_PPTP, tunnelName);
	sprintf(cmd, "%s -t %s -nvL %s --line-number | grep \"/\\* .\\+ url:.\\+ \\*/\" > %s",
			IPTABLES, table_name, chain_name, path);

	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	if((fp=fopen(path, "r")))
	{
		while(fgets(cmd,sizeof(cmd),fp) != NULL)
		{
			if(sscanf(cmd,"%d %*d %*d %*s", &lineNumber) == 1)
			{
				delId = lineNumber - del_num;
				if(delId > 0){
					sprintf(cmd2, "%s -t %s -D %s %d", IPTABLES, table_name, chain_name, delId);
					va_cmd("/bin/sh", 2, 1, "-c", cmd2);
					del_num++;
				}
			}
		}
		fclose(fp);
	}

	unlink(path);
	return 0;
}

#ifdef YUEME_4_0_SPEC
int rtk_wan_vpn_add_bind_wan_policy_route_rule(int type, void *entry)
{
	MIB_L2TP_T *l2tpEntry;
	MIB_PPTP_T *pptpEntry;
	struct in_addr inAddr;
	unsigned int fw_mark, fw_mask;
	char cmd[256]={0}, ifName[16]={0};
	int flags = 0;

	switch(type)
	{
		case VPN_TYPE_L2TP:
			l2tpEntry = (MIB_L2TP_T *) entry;
			
			ifGetName(l2tpEntry->bind_wan_ifindex, ifName, sizeof(ifName));

			//bind_wan_ifname exist and up
			if (strlen(ifName)>0 && getInFlags(ifName, &flags) == 1 && (flags & IFF_UP) && (flags & IFF_RUNNING)) {
				
			        if(inet_pton(AF_INET, l2tpEntry->server, &inAddr)!=1 && l2tpEntry->server[0] != '\0')
					restart_dnsrelay();

				//dynamically create chain name
				// clear exist rules first
				rtk_wan_vpn_remove_bind_wan_policy_route_rule(VPN_TYPE_L2TP, entry);

				//iptables -t mangle -N L2TP_SERVER_ROUTE_%
    				sprintf(cmd, "%s -t %s -N %s_%d", IPTABLES, NF_VPN_ROUTE_TABLE, L2TP_ROUTE_CHAIN, l2tpEntry->idx+1);
    				va_cmd("/bin/sh", 2, 1, "-c", cmd);
				
				//add L2TP_SERVER_ROUTE_% rule to VPN_SERVER_ROUTE
				//iptables -t mangle -A VPN_SERVER_ROUTE -j L2TP_SERVER_ROUTE_%
    				sprintf(cmd, "%s -t %s -A %s -j %s_%d", IPTABLES, NF_VPN_ROUTE_TABLE, NF_VPN_SERVER_CHAIN, L2TP_ROUTE_CHAIN, l2tpEntry->idx+1);
    				va_cmd("/bin/sh", 2, 1, "-c", cmd);

				rtk_wan_vpn_policy_route_get_mark(type, l2tpEntry->tunnelName, &fw_mark, &fw_mask);

				snprintf(cmd, sizeof(cmd), "%s -t mangle -A %s_%d -d %s -j MARK --set-mark 0x%x/0x%x", IPTABLES, L2TP_ROUTE_CHAIN, l2tpEntry->idx+1, l2tpEntry->server, fw_mark, fw_mask);
				va_cmd("/bin/sh", 2, 1, "-c", cmd);

			}

			break;
		case VPN_TYPE_PPTP:
			pptpEntry = (MIB_PPTP_T *) entry;

			ifGetName(pptpEntry->bind_wan_ifindex, ifName, sizeof(ifName));

			//bind_wan_ifname exist and up
			if (strlen(ifName)>0 && getInFlags(ifName, &flags) == 1 && (flags & IFF_UP) && (flags && IFF_RUNNING)) {

			    	//if server name need to parse, dnsmasq.conf should be updated.
				if(inet_pton(AF_INET, pptpEntry->server, &inAddr)!=1 && pptpEntry->server[0] != '\0')
					restart_dnsrelay();

				//dynamically create chain name
				// clear exist rules first
				rtk_wan_vpn_remove_bind_wan_policy_route_rule(VPN_TYPE_PPTP, entry);

				//iptables -t mangle -N PPTP_SERVER_ROUTE_%
    				sprintf(cmd, "%s -t %s -N %s_%d", IPTABLES, NF_VPN_ROUTE_TABLE, PPTP_ROUTE_CHAIN, pptpEntry->idx+1);
    				va_cmd("/bin/sh", 2, 1, "-c", cmd);
				
				//add PPTP_SERVER_ROUTE_% rule to VPN_SERVER_ROUTE
				//iptables -t mangle -A VPN_SERVER_ROUTE -j PPTP_SERVER_ROUTE_%
    				sprintf(cmd, "%s -t %s -A %s -j %s_%d", IPTABLES, NF_VPN_ROUTE_TABLE, NF_VPN_SERVER_CHAIN, PPTP_ROUTE_CHAIN, pptpEntry->idx+1);
    				va_cmd("/bin/sh", 2, 1, "-c", cmd);

				rtk_wan_vpn_policy_route_get_mark(type, pptpEntry->tunnelName, &fw_mark, &fw_mask);

				snprintf(cmd, sizeof(cmd), "%s -t mangle -A %s_%d -d %s -j MARK --set-mark 0x%x/0x%x", IPTABLES, PPTP_ROUTE_CHAIN, pptpEntry->idx+1, pptpEntry->server, fw_mark, fw_mask);
				va_cmd("/bin/sh", 2, 1, "-c", cmd);

			}

			break;
		default:
			break;
	}
}

int rtk_wan_vpn_remove_bind_wan_policy_route_rule(int type, void *entry)
{
	MIB_L2TP_T *l2tpEntry;
	MIB_PPTP_T *pptpEntry;

	char cmd[256]={0};

	switch(type)
	{
		case VPN_TYPE_L2TP:
		    	l2tpEntry = (MIB_L2TP_T *) entry;
			
			//iptables -t mangle -F L2TP_SERVER_ROUTE_%
			snprintf(cmd, sizeof(cmd), "%s -t mangle -F %s_%d", IPTABLES, L2TP_ROUTE_CHAIN, l2tpEntry->idx+1);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);

			//remove L2TP_SERVER_ROUTE_% rule to VPN_SERVER_ROUTE
			//iptables -t mangle -D VPN_SERVER_ROUTE -j L2TP_SERVER_ROUTE_%
    			sprintf(cmd, "%s -t %s -D %s -j %s_%d", IPTABLES, NF_VPN_ROUTE_TABLE, NF_VPN_SERVER_CHAIN, L2TP_ROUTE_CHAIN, l2tpEntry->idx+1);
    			va_cmd("/bin/sh", 2, 1, "-c", cmd);
			
			//iptables -t mangle -X L2TP_SERVER_ROUTE_%
			snprintf(cmd, sizeof(cmd), "%s -t mangle -X %s_%d", IPTABLES, L2TP_ROUTE_CHAIN, l2tpEntry->idx+1);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);

			break;

		case VPN_TYPE_PPTP:
		    	pptpEntry = (MIB_PPTP_T *) entry;

			//iptables -t mangle -F PPTP_SERVER_ROUTE_%
			snprintf(cmd, sizeof(cmd), "%s -t mangle -F %s_%d", IPTABLES, PPTP_ROUTE_CHAIN, pptpEntry->idx+1);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);

			//remove PPTP_SERVER_ROUTE_% rule to VPN_SERVER_ROUTE
			//iptables -t mangle -D VPN_SERVER_ROUTE -j PPTP_SERVER_ROUTE_%
    			sprintf(cmd, "%s -t %s -D %s -j %s_%d", IPTABLES, NF_VPN_ROUTE_TABLE, NF_VPN_SERVER_CHAIN, PPTP_ROUTE_CHAIN, pptpEntry->idx+1);
    			va_cmd("/bin/sh", 2, 1, "-c", cmd);
			
			//iptables -t mangle -X PPTP_SERVER_ROUTE_%
			snprintf(cmd, sizeof(cmd), "%s -t mangle -X %s_%d", IPTABLES, PPTP_ROUTE_CHAIN, pptpEntry->idx+1);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);

			break;
		default:
			break;
	}
}
#endif

int NF_Flush_PPTP_Route(unsigned char *tunnelName)
{
	MIB_PPTP_T pptpEntry;
	int pptpEntryIdx;
	char cmd[256], *pcmd;
	unsigned int fw_mark, fw_mask;
	unsigned char chain_name[64];
	const char *table_name;

	if(getPPTPEntryByTunnelName(tunnelName, &pptpEntry, &pptpEntryIdx) == NULL){
		printf("Cannot found PPTP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}
#if 1
	//We need to get fw_mark and fw_mask to know which rule we need to delete.
	if(rtk_wan_vpn_policy_route_get_mark(VPN_TYPE_PPTP, tunnelName, &fw_mark, &fw_mask) != 0){
		printf("Cannot get MARK of PPTP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}
#endif

	table_name = NF_VPN_ROUTE_TABLE;
	sprintf(chain_name, NF_PPTP_ROUTE_CHAIN, tunnelName);
	sprintf(cmd, "%s -t %s -D %s -m mark --mark 0x0/0x%x -j %s", IPTABLES, table_name, NF_VPN_ROUTE_CHAIN, fw_mask, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -F %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -X %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef VPN_MULTIPLE_RULES_BY_FILE
	/* Flush ipset iprange, subnet and smac chain and remove */
	snprintf(chain_name, sizeof(chain_name), NF_PPTP_IPSET_IPRANGE_CHAIN, tunnelName);
	snprintf(cmd, sizeof(cmd), "%s -X %s", IPSET, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	snprintf(chain_name, sizeof(chain_name), NF_PPTP_IPSET_IPSUBNET_CHAIN, tunnelName);
	snprintf(cmd, sizeof(cmd), "%s -X %s", IPSET, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	snprintf(chain_name, sizeof(chain_name), NF_PPTP_IPSET_SMAC_CHAIN, tunnelName);
	snprintf(cmd, sizeof(cmd), "%s -X %s", IPSET, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	snprintf(cmd, sizeof(cmd), "%s%s", VPN_PPTP_RULE_FILE, tunnelName);
	unlink(cmd);
	snprintf(cmd, sizeof(cmd), "%s%s", VPN_PPTP_IPSET_RESTORE_RULE_FILE, tunnelName);
	unlink(cmd);
#endif

	return 0;
}

int NF_Set_PPTP_Policy_Route(unsigned char *tunnelName)
{
	MIB_PPTP_T pptpEntry;
	MIB_CE_PPTP_ROUTE_T entry;
	int total_entry,i,enable,pptpEntryIdx;
	char cmd[256] = {0}, *pcmd = NULL, *pcmd2 = NULL;
	unsigned int fw_mark, fw_mask;
	unsigned char chain_name[64], comment_name[64];
	const char *table_name;

	if ( !mib_get_s(MIB_PPTP_ENABLE, (void *)&enable, sizeof(enable)) ){
		printf("MIB_PPTP_ENABLE is not exist!");
		return -1;
	}

	if(!enable){
		printf("MIB_PPTP_ENABLE is not enable!");
		return 0;
	}

	if(getPPTPEntryByTunnelName(tunnelName, &pptpEntry, &pptpEntryIdx) == NULL){
		printf("Cannot found PPTP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}

	if(rtk_wan_vpn_policy_route_get_mark(VPN_TYPE_PPTP, tunnelName, &fw_mark, &fw_mask) != 0){
		printf("Cannot get MARK of PPTP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}

	table_name = NF_VPN_ROUTE_TABLE;
	sprintf(chain_name, NF_PPTP_ROUTE_CHAIN, tunnelName);
	sprintf(cmd, "%s -t %s -F %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -X %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -N %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	sprintf(cmd, "%s -t %s -N %s", IPTABLES, table_name, NF_VPN_ROUTE_CHAIN);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -D %s -m mark --mark 0x0/0x%x -j %s", IPTABLES, table_name, NF_VPN_ROUTE_CHAIN, fw_mask, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -A %s -m mark --mark 0x0/0x%x -j %s", IPTABLES, table_name, NF_VPN_ROUTE_CHAIN, fw_mask, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	total_entry = mib_chain_total(MIB_PPTP_ROUTE_TBL);
	for(i=0;i<total_entry;i++)
	{
		if(mib_chain_get(MIB_PPTP_ROUTE_TBL, i, (void *)&entry) == 0)
			continue;

		if(!strcmp(entry.tunnelName,tunnelName))
		{
			if(((entry.ipv4_src_start || entry.ipv4_src_end) && (ATTACH_MODE_DIP==pptpEntry.attach_mode)) ||
				((entry.sMAC[0]|entry.sMAC[1]|entry.sMAC[2]|entry.sMAC[3]|entry.sMAC[4]|entry.sMAC[5]) && (ATTACH_MODE_SMAC==pptpEntry.attach_mode)))/*route by URL*/
			{
				cmd[0] = '\0';
				pcmd = cmd;
				sprintf(comment_name, NF_PPTP_ROUTE_COMMENT, i);
				pcmd += sprintf(pcmd, "%s -t %s -A %s", IPTABLES, table_name, chain_name);
				pcmd += sprintf(pcmd, " -m comment --comment \"%s\"", comment_name);
				pcmd += sprintf(pcmd, " -m mark --mark 0x0/0x%x", fw_mask);
				if(entry.ipv4_src_start || entry.ipv4_src_end)
				{
					pcmd += sprintf(pcmd, " --ipv4");
					if((entry.ipv4_src_start > 0 && entry.ipv4_src_end > 0) &&
						entry.ipv4_src_start != entry.ipv4_src_end)
					{
						pcmd += sprintf(pcmd, " -m iprange --dst-range %s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_start)));
						pcmd += sprintf(pcmd, "-%s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_end)));
					}
					else if(entry.ipv4_src_start > 0)
					{
						pcmd += sprintf(pcmd, " -d %s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_start)));
					}
					else
					{
						pcmd += sprintf(pcmd, " -d %s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_end)));
					}
				}
				else
				{
					pcmd += sprintf(pcmd, " -m mac --mac-source %02x:%02x:%02x:%02x:%02x:%02x",
										entry.sMAC[0], entry.sMAC[1], entry.sMAC[2], entry.sMAC[3], entry.sMAC[4], entry.sMAC[5]);
				}

				pcmd += sprintf(pcmd, " -j MARK --set-mark 0x%x/0x%x", fw_mark, fw_mask);

				va_cmd("/bin/sh", 2, 1, "-c", cmd);
			}
			else if (ATTACH_MODE_ALL == pptpEntry.attach_mode)
			{
				cmd[0] = '\0';
				pcmd = cmd;
				sprintf(comment_name, NF_PPTP_ROUTE_COMMENT, i);
				pcmd += sprintf(pcmd, "%s -t %s -A %s", IPTABLES, table_name, chain_name);
				pcmd += sprintf(pcmd, " -m comment --comment \"%s\"", comment_name);
				pcmd += sprintf(pcmd, " -m mark --mark 0x0/0x%x", fw_mask);
				pcmd2 = pcmd;
				if(entry.ipv4_src_start || entry.ipv4_src_end)
				{
					pcmd += sprintf(pcmd, " --ipv4");
					if((entry.ipv4_src_start > 0 && entry.ipv4_src_end > 0) &&
						entry.ipv4_src_start != entry.ipv4_src_end)
					{
						pcmd += sprintf(pcmd, " -m iprange --dst-range %s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_start)));
						pcmd += sprintf(pcmd, "-%s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_end)));
					}
					else if(entry.ipv4_src_start > 0)
					{
						pcmd += sprintf(pcmd, " -d %s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_start)));
					}
					else
					{
						pcmd += sprintf(pcmd, " -d %s", inet_ntoa(*((struct in_addr*)&entry.ipv4_src_end)));
					}
					pcmd += sprintf(pcmd, " -j MARK --set-mark 0x%x/0x%x", fw_mark, fw_mask);
					//printf("%s\n", cmd);
					va_cmd("/bin/sh", 2, 1, "-c", cmd);
				}
				if (isValidMacAddr(entry.sMAC))
				{
					pcmd += sprintf(pcmd2, " -m mac --mac-source %02x:%02x:%02x:%02x:%02x:%02x",
							entry.sMAC[0], entry.sMAC[1], entry.sMAC[2], entry.sMAC[3], entry.sMAC[4], entry.sMAC[5]);
					pcmd += sprintf(pcmd, " -j MARK --set-mark 0x%x/0x%x", fw_mark, fw_mask);
					pcmd += sprintf(pcmd, "\0");
					//printf("%s\n", cmd);
					va_cmd("/bin/sh", 2, 1, "-c", cmd);
				}

			}
		}
	}

	return 0;
}
#endif

#ifdef VPN_MULTIPLE_RULES_BY_FILE
int NF_Set_PPTP_Policy_Route_by_file(unsigned char *tunnelName)
{
	MIB_PPTP_T pptpEntry;
	int enable = 0, pptpEntryIdx = 0, priority = 0;
	char cmd[256] = {0};
	//char *pcmd = NULL, *pcmd2 = NULL;
	unsigned int fw_mark = 0, fw_mask = 0, pptp_rule_index = 0;
	unsigned char chain_name[64] = {0}, comment_name[64] = {0};
	const char *table_name = NULL;
	char vpn_pptp_file_path[128] = {0}, vpn_pptp_ipset_restore_file_path[128] = {0},tmp_tunnel_name[64] = {0}, line_buf[128] = {0}, match_rule[64] = {0};
	FILE *fp_file = NULL, *fp_ipset_restore_file = NULL;;
	int tmp_attach_mode = 0, rule_mode = 0;
	struct in_addr ipv4_addr1 = {0}, ipv4_addr2 = {0},tmp_addr = {0};
	char *str_p = NULL;
	unsigned char ipset_iprange_chain_name[64] = {0}, ipset_subnet_chain_name[64] = {0}, ipset_smac_chain_name[64] = {0};
	unsigned char ipset_comment[128] = {0}, ipset_iprange_comment[128] = {0}, ipset_tmp_comment[256] = {0};
	int bits = 0;

	if ( !mib_get_s(MIB_PPTP_ENABLE, (void *)&enable, sizeof(enable)) ){
		printf("MIB_PPTP_ENABLE is not exist!");
		return -1;
	}

	if(!enable){
		printf("MIB_PPTP_ENABLE is not enable!");
		return 0;
	}

	if(getPPTPEntryByTunnelName(tunnelName, &pptpEntry, &pptpEntryIdx) == NULL){
		printf("Cannot found PPTP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}

	if(rtk_wan_vpn_policy_route_get_mark(VPN_TYPE_PPTP, tunnelName, &fw_mark, &fw_mask) != 0){
		printf("Cannot get MARK of PPTP entry for tunnel name(%s) !!\n", tunnelName);
		return -1;
	}

	table_name = NF_VPN_ROUTE_TABLE;
	sprintf(chain_name, NF_PPTP_ROUTE_CHAIN, tunnelName);
	sprintf(cmd, "%s -t %s -F %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -X %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -N %s", IPTABLES, table_name, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	/* Set ipset 3 chain name. */
	snprintf(ipset_iprange_chain_name, sizeof(ipset_iprange_chain_name), NF_PPTP_IPSET_IPRANGE_CHAIN, tunnelName);
	snprintf(ipset_subnet_chain_name, sizeof(ipset_subnet_chain_name), NF_PPTP_IPSET_IPSUBNET_CHAIN, tunnelName);
	snprintf(ipset_smac_chain_name, sizeof(ipset_smac_chain_name), NF_PPTP_IPSET_SMAC_CHAIN, tunnelName);
	//snprintf(ipset_comment, sizeof(ipset_comment), NF_PPTP_IPSET_RULE_INDEX_COMMENT, );
	snprintf(ipset_tmp_comment, sizeof(ipset_tmp_comment), "%s%s", NF_PPTP_IPSET_RULE_INDEX_COMMENT, NF_PPTP_IPSET_IPRANGE_RULE_COMMENT);
	/* Firset delete ipset PPTP Tunnel chain */
	snprintf(cmd, sizeof(cmd), "%s -X %s > /dev/null 2>&1", IPSET, ipset_iprange_chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	snprintf(cmd, sizeof(cmd), "%s -X %s > /dev/null 2>&1", IPSET, ipset_subnet_chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	snprintf(cmd, sizeof(cmd), "%s -X %s > /dev/null 2>&1", IPSET, ipset_smac_chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	/* Create ipset PPTP Tunnel iprange chain(hash:ip). */
	snprintf(cmd, sizeof(cmd), "%s -N %s %s %s %s", IPSET, ipset_iprange_chain_name, IPSET_SETTYPE_HASHIP, IPSET_OPTION_COMMENT, IPSET_OPTION_COUNTERS);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	/* Create ipset PPTP Tunnel subnet chain(hash:net). */
	snprintf(cmd, sizeof(cmd), "%s -N %s %s %s %s", IPSET, ipset_subnet_chain_name, IPSET_SETTYPE_HASHNET, IPSET_OPTION_COMMENT, IPSET_OPTION_COUNTERS);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	/* Create ipset PPTP Tunnel mac chain(hash:net). */
	snprintf(cmd, sizeof(cmd), "%s -N %s %s %s %s", IPSET, ipset_smac_chain_name, IPSET_SETTYPE_HASHMAC, IPSET_OPTION_COMMENT, IPSET_OPTION_COUNTERS);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	snprintf(vpn_pptp_ipset_restore_file_path, sizeof(vpn_pptp_ipset_restore_file_path), "%s%s", VPN_PPTP_IPSET_RESTORE_RULE_FILE, tunnelName);
	fp_ipset_restore_file = fopen(vpn_pptp_ipset_restore_file_path, "w");
	if (fp_ipset_restore_file == NULL)
	{
		printf("%s: Open file = %s failed !!!\n", __FUNCTION__, vpn_pptp_file_path);
		return -1;
	}

	/* Create iptables chain. */
	sprintf(cmd, "%s -t %s -N %s", IPTABLES, table_name, NF_VPN_ROUTE_CHAIN);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -D %s -m mark --mark 0x0/0x%x -j %s", IPTABLES, table_name, NF_VPN_ROUTE_CHAIN, fw_mask, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -A %s -m mark --mark 0x0/0x%x -j %s", IPTABLES, table_name, NF_VPN_ROUTE_CHAIN, fw_mask, chain_name);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	/* Create iptables rules to link to 3 ipset chain. */
	sprintf(cmd, "%s -t %s -A  %s -m mark --mark 0x0/0x%x -m set --match-set %s dst -j MARK --set-mark 0x%x/0x%x", IPTABLES, table_name, chain_name, fw_mask, ipset_iprange_chain_name, fw_mark, fw_mask);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -A  %s -m mark --mark 0x0/0x%x -m set --match-set %s dst -j MARK --set-mark 0x%x/0x%x", IPTABLES, table_name, chain_name, fw_mask, ipset_subnet_chain_name, fw_mark, fw_mask);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	sprintf(cmd, "%s -t %s -A  %s -m mark --mark 0x0/0x%x -m set --match-set %s src -j MARK --set-mark 0x%x/0x%x", IPTABLES, table_name, chain_name, fw_mask, ipset_smac_chain_name, fw_mark, fw_mask);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	snprintf(vpn_pptp_file_path, sizeof(vpn_pptp_file_path), "%s%s", VPN_PPTP_RULE_FILE, tunnelName);

	fp_file = fopen(vpn_pptp_file_path, "r");
	if (fp_file == NULL)
	{
		printf("%s: Open file = %s failed !!!\n", __FUNCTION__, vpn_pptp_file_path);
	}
	else
	{
		while (fgets(line_buf, sizeof(line_buf), fp_file) != NULL)
		{
			//fprintf(fp, "%s %d %d %d %d %d %d %d %d %s\n", vpn_tunnel_info->tunnelName, static_set, i, priority, attach_mode, rule_mode, reserve_idx, pkt_cnt, pkt_pass_lasttime, match_rule);
			if (sscanf(line_buf, "%s %*d %d %d %d %d %*d %*d %*d %s", &tmp_tunnel_name, &pptp_rule_index, &priority, &tmp_attach_mode, &rule_mode,  &match_rule) != 6)
			continue;

			/*
			 * We will use ipset restore file to add ipset rule instead of entering command.
			 * Format will not be include ipset in the head.
			 * Ex:
			 *    -A aaa 192.168.1.2 -exist comment "hello world"
			 *    -A aaa 223.205.23.116/24 -exist comment "hello world"
			 *    -A aaa AA:BB:CC:DD:EE:FF -exist comment
			 */
			if (!strcmp(tunnelName, tmp_tunnel_name))
			{
				snprintf(ipset_comment, sizeof(ipset_comment), NF_PPTP_IPSET_RULE_INDEX_COMMENT, rule_mode, pptp_rule_index);
				//typedef enum { ATTACH_MODE_ALL=0, ATTACH_MODE_DIP=1, ATTACH_MODE_SMAC=2, ATTACH_MODE_NONE } ATTACH_MODE_T;
				if (((str_p = strstr(match_rule, "-")) || isIPAddr(match_rule) == 1) && tmp_attach_mode != ATTACH_MODE_SMAC) //IP case
				{
					if (str_p != NULL)
					{
						*str_p = '\0';
						if (isIPAddr(match_rule) != 1)
						{
							*str_p = '-';
							goto domain_case;
						}
						if (isIPAddr(str_p + 1) != 1)
						{
							*str_p = '-';
							goto domain_case;
						}
						*str_p = '-';

						/* ip range comment is special like: "pptp_ruletype_3_route_idx_1,192.168.1.10-192.168.15" */
						snprintf(ipset_iprange_comment, sizeof(ipset_iprange_comment), ipset_tmp_comment, rule_mode, pptp_rule_index, match_rule);
#if 0 // We don't need to make range ip to be single IP if start ip = end ip
						if (!strcmp(match_rule, str_p + 1)) //ip_start = ip_end
							pcmd += sprintf(pcmd, " -d %s", match_rule);
						else //ip_start != ip_end
#endif
						//snprintf(cmd, sizeof(cmd), "%s -A %s %s -exist comment \"%s\"", IPSET, ipset_iprange_chain_name, match_rule, ipset_iprange_comment);
						fprintf(fp_ipset_restore_file, "-A %s %s -exist comment \"%s\"\n", ipset_iprange_chain_name, match_rule, ipset_iprange_comment);
					}
					else //pure 1 IP
					{
						//snprintf(cmd, sizeof(cmd), "%s -A %s %s -exist comment \"%s\"", IPSET, ipset_iprange_chain_name, match_rule, ipset_comment);
						fprintf(fp_ipset_restore_file, "-A %s %s -exist comment \"%s\"\n", ipset_iprange_chain_name, match_rule, ipset_comment);
					}
				}
				else if ((str_p = strstr(match_rule, "/")) && tmp_attach_mode != ATTACH_MODE_SMAC) //IP subnet case
				{
					*str_p = '\0';
					bits = atoi(str_p + 1);
					if (isIPAddr(match_rule) != 1 || !(bits > 0 && bits <= 32))
					{
						*str_p = '/';
						goto domain_case;
					}
					*str_p = '/';
					//ipset -A ipset_subnet_chain_name 192.168.1.10/24 -exist comment "pptp_ruletype_%d_route_idx_%d"
					//snprintf(cmd, sizeof(cmd), "%s -A %s %s -exist comment \"%s\"", IPSET, ipset_subnet_chain_name, match_rule, ipset_comment);
					fprintf(fp_ipset_restore_file, "-A %s %s -exist comment \"%s\"\n", ipset_subnet_chain_name, match_rule, ipset_comment);
				}
				else if (isValidMacString(match_rule) && (tmp_attach_mode != ATTACH_MODE_DIP)) //SMAC case
				{
					//snprintf(cmd, sizeof(cmd), "%s -A %s %s -exist comment \"%s\"", IPSET, ipset_smac_chain_name, match_rule, ipset_comment);
					fprintf(fp_ipset_restore_file, "-A %s %s -exist comment \"%s\"\n", ipset_smac_chain_name, match_rule, ipset_comment);
				}
				else // domain case
				{
					rtk_wan_vpn_preset_url_policy_route_by_file(VPN_TYPE_PPTP, tunnelName, pptp_rule_index, priority, match_rule);
				}

				continue;
domain_case:
					rtk_wan_vpn_preset_url_policy_route_by_file(VPN_TYPE_PPTP, tunnelName, pptp_rule_index, priority, match_rule);
			}
		}

		if (fp_ipset_restore_file)
		{
			fclose(fp_ipset_restore_file);
			fp_ipset_restore_file = NULL;
		}
		snprintf(cmd, sizeof(cmd), "%s %s -f %s", IPSET, IPSET_COMMAND_RESTORE, vpn_pptp_ipset_restore_file_path);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
	}
	if (fp_ipset_restore_file)
		fclose(fp_ipset_restore_file);
	if (fp_file)
		fclose(fp_file);


	return 0;
}
#endif

#endif

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
#ifdef CONFIG_USER_PPPD
#ifdef CONFIG_USER_XL2TPD
extern const char XL2TPD[];
#endif
#endif //CONFIG_USER_PPPD

#if defined(CONFIG_USER_L2TPD_L2TPD)
int applyL2TP(MIB_L2TP_T *pentry, int enable, int l2tp_index)
{
	unsigned int mark, mask;
	struct data_to_pass_st msg;
	char index[5];
	int len;
	char ifName[IFNAMSIZ];

	if( pentry->vpn_enable==VPN_DISABLE ) {
		AUG_PRT("%s-%d VPN tunnel is disabled, don't make it work!\n", __func__, __LINE__);
		return -1;
	}

#if defined(CONFIG_CTC_SDN)
	if(pentry->sdn_enable)
	{
		if (enable == 1 || enable == 4) 
		{
			bind_ovs_interface_by_mibentry(OFP_PTYPE_VPN_L2TP, (void *)pentry, l2tp_index, 0);
		}
		else if(enable == 0 || enable == 5)
		{
			unbind_ovs_interface_by_mibentry(OFP_PTYPE_VPN_L2TP, (void *)pentry, l2tp_index, 0, 1);
		}
		return 1;
	}
#endif

	ifGetName(pentry->ifIndex, ifName, sizeof(ifName));

	// Mason Yu. Add VPN ifIndex
	// unit declarations for ppp  on if_sppp.h
	// (1) 0 ~ 7: pppoe/pppoa, (2) 8: 3G, (3) 9 ~ 10: PPTP, (4) 11 ~12: L2TP
	if (enable == 1 || enable == 4) {	// add(persistent) or new(Dial-on-demand)
		rtk_net_add_wan_policy_route_mark(ifName, &mark, &mask);
#ifdef YUEME_4_0_SPEC
		rtk_wan_vpn_add_bind_wan_policy_route_rule(VPN_TYPE_L2TP, (void *)pentry);
#endif
		if (pentry->conntype != MANUAL) {
			snprintf(msg.data, BUF_SIZE,
				"spppctl add %d l2tp username %s password %s gw %d mru %d", pentry->idx+L2TP_VPN_STRAT,
				pentry->username, pentry->password, pentry->dgw, pentry->mtu);
		}
		else {
			snprintf(msg.data, BUF_SIZE,
				"spppctl new %d l2tp username %s password %s gw %d mru %d", pentry->idx+L2TP_VPN_STRAT,
				pentry->username, pentry->password, pentry->dgw, pentry->mtu);
		}
		len=BUF_SIZE-strlen(msg.data);
		snprintf(msg.data+strlen(msg.data), len, " auth %s", l2tp_auth[pentry->authtype]);

		len=BUF_SIZE-strlen(msg.data);
		snprintf(msg.data+strlen(msg.data), len, " enctype %s", l2tp_encryt[pentry->enctype]);

		len=BUF_SIZE-strlen(msg.data);
		snprintf(msg.data+strlen(msg.data), len, " server %s", pentry->server);

		len=BUF_SIZE-strlen(msg.data);
		snprintf(msg.data+strlen(msg.data), len, " tunnel_auth %s", tunnel_auth[pentry->tunnel_auth]);
		if (pentry->tunnel_auth) {
			len=BUF_SIZE-strlen(msg.data);
			snprintf(msg.data+strlen(msg.data), len, " secret %s", pentry->secret);
		}

		if (pentry->conntype == CONNECT_ON_DEMAND)
		{
			len=BUF_SIZE-strlen(msg.data);
			snprintf(msg.data+strlen(msg.data), len, " timeout %d", pentry->idletime);
		}
		else if(pentry->conntype == CONNECT_ON_DEMAND_PKT_COUNT)
		{
			len=BUF_SIZE-strlen(msg.data);
			snprintf(msg.data+strlen(msg.data), len, " timeout_en 1 timeout %d", pentry->idletime);
		}
#ifdef CONFIG_IPV6_VPN
		len=BUF_SIZE-strlen(msg.data);
		snprintf(msg.data+strlen(msg.data), len, " ipt %u", pentry->IpProtocol - 1);
#endif

		printf("%s: %s\n", __func__, msg.data);
		rtk_wan_vpn_generate_ifup_script(VPN_TYPE_L2TP, pentry);
		rtk_wan_vpn_generate_ifdown_script(VPN_TYPE_L2TP, pentry);

		write_to_pppd(&msg);
	}
	else if (enable == 2) { 	// connect(up) for dial on-demand
		snprintf(msg.data, BUF_SIZE, "spppctl up %d", pentry->idx+L2TP_VPN_STRAT);
		write_to_pppd(&msg);
	}
	else if (enable == 3) { // disconnect(down) for dial on-demand
		snprintf(msg.data, BUF_SIZE, "spppctl down %d", pentry->idx+L2TP_VPN_STRAT);
		write_to_pppd(&msg);
	}
	else if(enable == 0 || enable == 5){
		snprintf(index, 5, "%d", pentry->idx+L2TP_VPN_STRAT);
		va_cmd("/bin/spppctl", 4, 1, "del", index, "l2tp", "0");
		rtk_wan_vpn_remove_ifup_script(VPN_TYPE_L2TP, pentry);
		rtk_wan_vpn_do_ifdown_script(VPN_TYPE_L2TP, pentry);
		rtk_wan_vpn_remove_ifdown_script(VPN_TYPE_L2TP, pentry);
		rtk_net_remove_wan_policy_route_mark(ifName);
#ifdef YUEME_4_0_SPEC
		rtk_wan_vpn_remove_bind_wan_policy_route_rule(VPN_TYPE_L2TP, (void *)pentry);
#endif
	}

	return 1;
}
#elif defined(CONFIG_USER_XL2TPD)
int applyL2TP(MIB_L2TP_T *pentry, int enable, int l2tp_index)
{
	char buff[64];
	unsigned int entrynum, i;//, j;
	char xl2tpdpid_str[32];
	int xl2tpd_pid=0;
	char *argv[32];
	int idx;

	if (enable == 1 || enable == 4) {
		write_to_l2tp_config();
		snprintf(xl2tpdpid_str, sizeof(xl2tpdpid_str), "%s", XL2TPD_PID);
		xl2tpd_pid = read_pid((char *)xl2tpdpid_str);
		if (xl2tpd_pid >= 1) {
			kill_by_pidfile(xl2tpdpid_str, SIGTERM);
		}

		entrynum = mib_chain_total(MIB_L2TP_TBL);
		if (entrynum > 0) {
			idx = 0;
			argv[idx++] = (char *)XL2TPD;
			//argv[idx++] = "-D";
			argv[idx++] = NULL;
		
			printf("%s", XL2TPD);
			for (i=1; i<idx-1; i++)
				printf(" %s", argv[i]);
			printf("\n");
			//do_cmd(XL2TPD, argv, 0);
			do_cmd(XL2TPD, argv, 1);
		}

		connect_xl2tpd(PPP_INDEX(pentry->ifIndex));
	}
	else if (enable == 0 || enable == 5) {
#ifdef CONFIG_USER_PPPD
		disconnectPPP(PPP_INDEX(pentry->ifIndex));
#else
		//sprintf(buff,"echo \"h\" > /var/run/xl2tpd/l2tp-control");
		sprintf(buff,"echo \"d connect%u\" > /var/run/xl2tpd/l2tp-control", PPP_INDEX(pentry->ifIndex));
		printf("%s\n", buff);
		system(buff);
#endif
	}

	return 1;
}

#endif

void l2tp_take_effect(void)
{
	MIB_L2TP_T entry;
	unsigned int entrynum, i;//, j;
	int enable;
	int mode=L2TP_MODE_LAC;
#if defined(CONFIG_USER_PPPD) && defined(CONFIG_USER_XL2TPD)
	char xl2tpdpid_str[32];
	int xl2tpd_pid=0;
#endif

	if ( !mib_get(MIB_L2TP_ENABLE, (void *)&enable) )
		return;

#ifdef CONFIG_USER_L2TPD_LNS
	setup_l2tp_server_firewall();
#endif

#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
	if (enable) {
		l2tpIPSec_LAC_strongswan_take_effect();
		start_process_check_pidfile((const char*)CHARONPID);
	}
#endif

	entrynum = mib_chain_total(MIB_L2TP_TBL);

#if defined(CONFIG_USER_PPPD) && defined(CONFIG_USER_XL2TPD)
	snprintf(xl2tpdpid_str, sizeof(xl2tpdpid_str), "%s", XL2TPD_PID);
	xl2tpd_pid = read_pid((char *)xl2tpdpid_str);

	if (xl2tpd_pid >= 1) {
#endif
	//delete all firstly
	for (i=0; i<entrynum; i++)
	{
		if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&entry) )
			return;
#if defined(CONFIG_CTC_SDN)
		if(entry.sdn_enable)
			unbind_ovs_interface_by_mibentry(OFP_PTYPE_VPN_L2TP, (void *)&entry, i, 0, 0);
		else
#endif
		applyL2TP(&entry, 0, i);
	}
#if defined(CONFIG_USER_PPPD) && defined(CONFIG_USER_XL2TPD)
	}
#endif

	if (enable) {
		mib_get_s(MIB_L2TP_MODE, (void *)&mode, sizeof(mode));
		switch(mode){
		case L2TP_MODE_LAC:
			{
		for (i=0; i<entrynum; i++)
		{
			if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&entry) )
				return;
#if defined(CONFIG_CTC_SDN)
			if(entry.sdn_enable)
				bind_ovs_interface_by_mibentry(OFP_PTYPE_VPN_L2TP, (void *)&entry, i, 0);
			else
#endif
			applyL2TP(&entry, 1, i);
		}
			}
			break;
		case L2TP_MODE_LNS:
			{
#if defined(CONFIG_USER_L2TPD_LNS)
				l2tp_lns_take_effect();
#else
				fprintf(stderr, "[%s] NOT support L2TP_MODE_LNS\n", __FUNCTION__);
#endif
			}
			break;
		default:
			fprintf(stderr, "[%s] unknown L2TP Mode(%d)\n", __FUNCTION__, mode);
			break;
		}
	}
	else{
#if defined(CONFIG_USER_PPPD) && defined(CONFIG_USER_XL2TPD)
		if (xl2tpd_pid >= 1) {
			kill_by_pidfile(xl2tpdpid_str, SIGTERM);
		}
#endif
	}
}

int getIfIndexByL2TPName(char *pIfname)
{
	unsigned int entryNum, i;
	MIB_L2TP_T Entry;
	char ifname[IFNAMSIZ];
	entryNum = mib_chain_total(MIB_L2TP_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_L2TP_TBL, i, (void *)&Entry))
		{
			printf("Get chain record error!\n");
			return -1;
		}
		ifGetName(Entry.ifIndex,ifname,sizeof(ifname));
		if(!strcmp(ifname,pIfname)){
			break;
		}
	}
	if(i>= entryNum){
		printf("not find this interface!\n");
		return -1;
	}
	return(Entry.ifIndex);
}

MIB_L2TP_T *getL2TPEntryByIfname(char *ifname, MIB_L2TP_T *p, int *entryIndex)
{
	int i,num;
	unsigned char mib_ifname[32];
	if( (p==NULL) || (ifname==NULL) ) return NULL;
	num = mib_chain_total( MIB_L2TP_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_L2TP_TBL, i, (void*)p ))
			continue;
		ifGetName(p->ifIndex, mib_ifname, sizeof(mib_ifname));
		if(!strcmp(mib_ifname, ifname))
		{
			if(entryIndex) *entryIndex = i;
			return p;
		}
	}
	return NULL;
}

MIB_L2TP_T *getL2TPEntryByIfIndex(unsigned int ifIndex, MIB_L2TP_T *p, int *entryIndex)
{
	int i,num;
	if( (p==NULL) || (ifIndex==DUMMY_IFINDEX) ) return NULL;
	num = mib_chain_total( MIB_L2TP_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_L2TP_TBL, i, (void*)p ))
			continue;
		if( p->ifIndex==ifIndex )
		{
			if(entryIndex) *entryIndex = i;
			return p;
		}
	}
	return NULL;
}

MIB_L2TP_T *getL2TPEntryByTunnelName(char *tunnelName, MIB_L2TP_T *p, int *entryIndex)
{
	int i,num;
	if( (p==NULL) || (tunnelName==NULL) ) return NULL;
	num = mib_chain_total( MIB_L2TP_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_L2TP_TBL, i, (void*)p ))
			continue;
		if( !strcmp(p->tunnelName, tunnelName) )
		{
			if(entryIndex) *entryIndex = i;
			return p;
		}
	}
	return NULL;
}
#endif

#if (defined(CONFIG_USER_L2TPD_LNS) && defined(CONFIG_USER_XL2TPD)) || (defined(CONFIG_USER_PPTP_CLIENT) && defined(CONFIG_USER_PPTP_SERVER))
void getVpnServerLocalIP(unsigned char vpn_type, char *ipaddr)
{
	MIB_VPND_T server;
	unsigned int pptp_entry_num = 0;
	int i=0;
	struct in_addr endaddr;
	char str_ipaddr[16] ={0};

	memset(&endaddr, 0, sizeof(struct in_addr));
	pptp_entry_num = mib_chain_total(MIB_VPN_SERVER_TBL);
	for (i=0 ; i<pptp_entry_num ; i++) {
		if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, (void *)&server))
			continue;

		if(server.type == vpn_type) {
			if (server.peeraddr != 0)
				endaddr.s_addr = ntohl((htonl(server.peeraddr)|0xff)-1); //192.168.1."254"
			break;
		}
	}
	sprintf(str_ipaddr, "%s", inet_ntoa(endaddr));
	strcpy(ipaddr, str_ipaddr);
}

void getVpnServerSubnet(unsigned char vpn_type, char *str_subnet)
{
	MIB_VPND_T server;
	unsigned int pptp_entry_num = 0;
	int i=0, isConfig=0;
	struct in_addr subnet;
	char strbuf[16] ={0};

	memset(&subnet, 0, sizeof(struct in_addr));
	pptp_entry_num = mib_chain_total(MIB_VPN_SERVER_TBL);
	for (i=0 ; i<pptp_entry_num ; i++) {
		if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, (void *)&server))
			continue;

		if(server.type == vpn_type) {
			if (server.peeraddr != 0) {
				subnet.s_addr = ntohl((htonl(server.peeraddr)|0xff)-0xff); //192.168.1."0"
				isConfig = 1;
				break;
			}
		}
	}
	if (isConfig) {
		sprintf(strbuf, "%s/24", inet_ntoa(subnet));
		strcpy(str_subnet, strbuf);
	}
}
#endif

#ifdef CONFIG_USER_L2TPD_LNS
#if defined(CONFIG_USER_XL2TPD)
void applyL2tpAccount(MIB_VPN_ACCOUNT_T *pentry, int enable)
{
	unsigned int entrynum, i;
	char xl2tpdpid_str[32];
	int xl2tpd_pid=0;
	char *argv[32];
	int idx;
	char pidfile[64], back_pidfile[64];

	if (enable == 1) {
		/* check ppp interface before start lac connection */
		snprintf(pidfile, sizeof(pidfile), "%s/ppp11.pid", PPPD_CONFIG_FOLDER);
		snprintf(back_pidfile, sizeof(back_pidfile), "%s/ppp11.pid_back", PPPD_CONFIG_FOLDER);
		if (read_pid(pidfile) <= 0){
			kill_by_pidfile(pidfile, SIGTERM);
		}else if(read_pid(back_pidfile) > 0){
			/* PID file be remove when PPP link_terminated
			 * it will cause delete ppp daemon fail, So backup it on ipcp/ipv6cp up script. */
			kill_by_pidfile(back_pidfile, SIGTERM);
		}

		entrynum = mib_chain_total(MIB_VPN_ACCOUNT_TBL);
		write_to_l2tp_lns_config();
		if (entrynum > 0) {
			idx = 0;
			argv[idx++] = (char *)XL2TPD;
			//argv[idx++] = "-D";
			argv[idx++] = NULL;
			
			printf("%s", XL2TPD);
			for (i=1; i<idx-1; i++)
				printf(" %s", argv[i]);
			printf("\n");
			//do_cmd(XL2TPD, argv, 0);
			do_cmd(XL2TPD, argv, 1);
		}
	}
	else{
		snprintf(xl2tpdpid_str, sizeof(xl2tpdpid_str), "/var/run/xl2tpd.pid");
		kill_by_pidfile(xl2tpdpid_str, SIGTERM);

		/**********************************************************************/
		/* Write PAP/CHAP secrets file                                        */
		/**********************************************************************/
		rtk_create_ppp_secrets();
	}
}
#else
void applyL2tpAccount(MIB_VPN_ACCOUNT_T *pentry, int enable)
{
	MIB_VPND_T server;
	int total, i;
	struct data_to_pass_st msg;
	char index[5];
	char auth[20];
	char enctype[20];
	char localip[20], peerip[20];

	if (1 == enable) {//add a pptp account

		if (0 == pentry->enable)
			return;

		//get server auth info
		total = mib_chain_total(MIB_VPN_SERVER_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, &server))
				continue;

			if (VPN_L2TP == server.type)
				break;
		}

		if (i >= total) {
			printf("please configure l2tp server info first.\n");
			return;
		}

		switch (server.authtype)
		{
		case 1://pap
			strcpy(auth, "pap");
			break;
		case 2://chap
			strcpy(auth, "chap");
			break;
		case 3://chapmsv2
			strcpy(auth, "chapms-v2");
			break;
		default:
			strcpy(auth, "auto");
			break;
		}

		switch (server.enctype)
		{
		case 1://MPPE
			strcpy(enctype, "+MPPE");
			break;
        case 2://MPPC
			strcpy(enctype, "+MPPC");
			break;
		case 3://MPPE&MPPC
			strcpy(enctype, "+BOTH");
			break;
		default:
			strcpy(enctype, "none");
			break;
		}

		snprintf(localip, 20, "%s", inet_ntoa(*(struct in_addr *)&server.localaddr));
		snprintf(peerip, 20, "%s", inet_ntoa(*(struct in_addr *)&server.peeraddr));

		snprintf(msg.data, BUF_SIZE, "spppctl addvpn %s type L2TP auth %s enctype %s username %s password %s localip %s peerip %s",
					pentry->name, auth, enctype, pentry->username, pentry->password, localip, peerip);

		snprintf(msg.data, BUF_SIZE, "%s tunnel_auth %s", msg.data, tunnel_auth[server.tunnel_auth]);
		if (server.tunnel_auth) {
			snprintf(msg.data, BUF_SIZE, "%s secret %s", msg.data, server.tunnel_key);
		}

		printf("%s: %s\n", __func__, msg.data);

		write_to_pppd(&msg);
	}
	else {
		printf("/bin/spppctl delvpn %s\n", pentry->name);

		va_cmd("/bin/spppctl", 2, 1, "delvpn", pentry->name);
	}
}
#endif

void setup_l2tp_server_firewall(void)
{
	char local_ipaddr[16] ={0};
	char str_subnet[32] ={0};
	char lan_ipaddr[16] ={0};
	int enable;
	int type;
	struct in_addr lanIp;
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
	unsigned int entrynum;
	MIB_VPND_T entry;
	int i;
#endif

	/* clean vpn firewall rule */
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_L2TP_VPN_IN);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_L2TP_VPN_FW);
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
	system("iptables -t nat -D POSTROUTING -m policy --dir out --pol ipsec --mode tunnel -j ACCEPT");
#endif

	if ( !mib_get(MIB_L2TP_ENABLE, (void *)&enable) )
		return;
	if (!enable) return;

	mib_get_s(MIB_L2TP_MODE, (void *)&type, sizeof(type));
	if (type != L2TP_MODE_LNS) return;

	//iptables -A l2tp_vpn_input ! -i br+ -p udp --dport 1701 -j ACCEPT
	va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)IPTABLE_L2TP_VPN_IN, "!", (char *)ARG_I, (char *)LANIF_ALL, "-p", "udp", (char *)FW_DPORT, (char *)PORT_L2TP, "-j", (char *)FW_ACCEPT);

	getVpnServerLocalIP(VPN_L2TP, local_ipaddr);
	getVpnServerSubnet(VPN_L2TP, str_subnet);
	if (!str_subnet[0]) return;

	if (mib_get_s( MIB_ADSL_LAN_IP, (void *)&lanIp, sizeof(lanIp))) {
		if (lanIp.s_addr != INADDR_NONE ) {
			sprintf(lan_ipaddr, "%s", inet_ntoa(lanIp));
		}
	}

	//iptables -A l2tp_vpn_input ! -i br+ -d 11.0.0.254 -s 11.0.0.0/24 -j ACCEPT
	va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)IPTABLE_L2TP_VPN_IN, "!", (char *)ARG_I, (char *)LANIF_ALL, "-d", local_ipaddr, "-s", str_subnet, "-j", (char *)FW_ACCEPT);
	//iptables -A l2tp_vpn_input ! -i br+ -d 192.168.1.1 -s 11.0.0.0/24 -j ACCEPT
	va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)IPTABLE_L2TP_VPN_IN, "!", (char *)ARG_I, (char *)LANIF_ALL, "-d", lan_ipaddr, "-s", str_subnet, "-j", (char *)FW_ACCEPT);
	//iptables -A l2tp_vpn_fw ! -i br+ -s 11.0.0.0/24 -j ACCEPT
	va_cmd(IPTABLES, 9, 1, (char *)FW_ADD, (char *)IPTABLE_L2TP_VPN_FW, "!", (char *)ARG_I, (char *)LANIF_ALL, "-s", str_subnet, "-j", (char *)FW_ACCEPT);
	//iptables -A l2tp_vpn_fw ! -i br+ -d 11.0.0.0/24 -j ACCEPT
	va_cmd(IPTABLES, 9, 1, (char *)FW_ADD, (char *)IPTABLE_L2TP_VPN_FW, "!", (char *)ARG_I, (char *)LANIF_ALL, "-d", str_subnet, "-j", (char *)FW_ACCEPT);

#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
	entrynum = mib_chain_total(MIB_VPN_SERVER_TBL);
	for(i = 0; i<entrynum; i++){
		mib_chain_get(MIB_VPN_SERVER_TBL, i, (void *)&entry);
		if(entry.overIPSec == 1){
			//iptables -I INPUT -p udp -m udp --dport 500 -j ACCEPT
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)IPTABLE_L2TP_VPN_IN, "-p", "udp", "-m", "udp", (char *)FW_DPORT, (char *)PORT_IPSEC, "-j", "ACCEPT");
			//iptables -I INPUT -p udp -m udp --dport 4500 -j ACCEPT
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)IPTABLE_L2TP_VPN_IN, "-p", "udp", "-m", "udp", (char *)FW_DPORT, (char *)PORT_IPSEC_NAT, "-j", "ACCEPT");
			//iptables -I INPUT ! -i br0 -p 50 -j ACCEPT
			va_cmd(IPTABLES, 9, 1, (char *)FW_ADD, (char *)IPTABLE_L2TP_VPN_IN, "!", ARG_I, LANIF_ALL, "-p", "50", "-j", "ACCEPT");

			system("iptables -t nat -I POSTROUTING -m policy --dir out --pol ipsec --mode tunnel -j ACCEPT");
			break;
		}
	}
#endif
}

void l2tp_lns_take_effect(void)
{
	MIB_VPN_ACCOUNT_T entry;
	unsigned int entrynum, i;
	int enable;
	int l2tpType;

	setup_l2tp_server_firewall();
	if ( !mib_get(MIB_L2TP_ENABLE, (void *)&enable) )
		return;

	mib_get_s(MIB_L2TP_MODE, (void *)&l2tpType, sizeof(l2tpType));
	if (l2tpType == L2TP_MODE_LAC) return;

	entrynum = mib_chain_total(MIB_VPN_ACCOUNT_TBL);
	for (i=0; i<entrynum; i++)
	{
		if (!mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&entry))
			continue;
		if(VPN_L2TP == entry.type)
			applyL2tpAccount(&entry, 0);
	}

	if (enable) {
		for (i=0; i<entrynum; i++)
		{
			if (!mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&entry))
				continue;
			if(VPN_L2TP == entry.type)
				applyL2tpAccount(&entry, 1);
		}
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
		l2tpIPSec_LNS_strongswan_take_effect();
#endif
	}
}
#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)

#ifdef CONFIG_USER_PPTPD_PPTPD
void applyPptpAccount(MIB_VPN_ACCOUNT_T *pentry, int enable)
{
	MIB_VPND_T server;
	int total, i;
	struct data_to_pass_st msg;
	char index[5];
	char auth[20];
	char enctype[20];
	char localip[20], peerip[20];

	if (1 == enable) {//add a pptp account

		if (0 == pentry->enable)
			return;

		//get server auth info
		total = mib_chain_total(MIB_VPN_SERVER_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, &server))
				continue;

			if (VPN_PPTP == server.type)
				break;
		}

		if (i >= total) {
			printf("please configure pptp server info first.\n");
			return;
		}

		switch (server.authtype)
		{
		case 1://pap
			strcpy(auth, "pap");
			break;
		case 2://chap
			strcpy(auth, "chap");
			break;
		case 3://chapmsv2
			strcpy(auth, "chapms-v2");
			break;
		default:
			strcpy(auth, "auto");
			break;
		}

		switch (server.enctype)
		{
		case 1://MPPE
			strcpy(enctype, "+MPPE");
			break;
        case 2://MPPC
			strcpy(enctype, "+MPPC");
			break;
		case 3://MPPE&MPPC
			strcpy(enctype, "+BOTH");
			break;
		default:
			strcpy(enctype, "none");
			break;
		}

		snprintf(localip, 20, "%s", inet_ntoa(*(struct in_addr *)&server.localaddr));
		snprintf(peerip, 20, "%s", inet_ntoa(*(struct in_addr *)&server.peeraddr));

		snprintf(msg.data, BUF_SIZE, "spppctl addvpn %s type PPTP auth %s enctype %s username %s password %s localip %s peerip %s",
					pentry->name, auth, enctype, pentry->username, pentry->password, localip, peerip);

		printf("%s: %s\n", __func__, msg.data);

		write_to_pppd(&msg);
	}
	else {
		printf("/bin/spppctl delvpn %s\n", pentry->name);

		va_cmd("/bin/spppctl", 2, 1, "delvpn", pentry->name);
	}
}

void pptpd_take_effect(void)
{
	MIB_VPN_ACCOUNT_T entry;
	unsigned int entrynum, i;
	int enable;

	if ( !mib_get(MIB_PPTP_ENABLE, (void *)&enable) )
		return;

	entrynum = mib_chain_total(MIB_VPN_ACCOUNT_TBL);
	for (i=0; i<entrynum; i++)
	{
		if (!mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&entry))
			continue;
		if(VPN_PPTP == entry.type)
			applyPptpAccount(&entry, 0);
	}

	if (enable) {
		for (i=0; i<entrynum; i++)
		{
			if (!mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&entry))
				continue;
			if(VPN_PPTP == entry.type)
				applyPptpAccount(&entry, 1);
		}
	}
}
#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP)
void applyPPtP(MIB_PPTP_T *pentry, int enable, int pptp_index)
{
	unsigned int mark, mask;
	struct data_to_pass_st msg;
	char index[5];
	char auth[20];
	char enctype[20];
	int len;
	char ifName[IFNAMSIZ];

	if( pentry->vpn_enable==VPN_DISABLE ) {
		AUG_PRT("%s-%d VPN tunnel is disabled, don't make it work!\n", __func__, __LINE__);
		return;
	}
	
#if defined(CONFIG_CTC_SDN)
	if(pentry->sdn_enable)
	{
		if (enable == 1 || enable == 2) 
		{
			bind_ovs_interface_by_mibentry(OFP_PTYPE_VPN_PPTP, (void *)pentry, pptp_index, 0);
		}
		else if(enable == 0 || enable == 3)
		{
			unbind_ovs_interface_by_mibentry(OFP_PTYPE_VPN_PPTP, (void *)pentry, pptp_index, 0, 1);
		}
		return ;
	}
#endif

	ifGetName(pentry->ifIndex, ifName, sizeof(ifName));

	// Mason Yu. Add VPN ifIndex
	// unit declarations for ppp  on if_sppp.h
	// (1) 0 ~ 7: pppoe/pppoa, (2) 8: 3G, (3) 9 ~ 10: PPTP, (4) 11 ~12: L2TP
	if (enable == 1 || enable == 2) {	// add(persistent) or new(Dial-on-demand)
		rtk_net_add_wan_policy_route_mark(ifName, &mark, &mask);
#ifdef YUEME_4_0_SPEC
		rtk_wan_vpn_add_bind_wan_policy_route_rule(VPN_TYPE_PPTP, (void *) pentry);
#endif
		switch (pentry->authtype)
		{
			case 1://pap
				strcpy(auth, "pap");
				break;
			case 2://chap
				strcpy(auth, "chap");
				break;
			case 3://chapmsv2
				strcpy(auth, "chapms-v2");
				break;
			default:
				strcpy(auth, "auto");
				break;
		}

		switch (pentry->enctype)
		{
			case 1://MPPE
				strcpy(enctype, "+MPPE");
				break;
			case 2://MPPC
				strcpy(enctype, "+MPPC");
				break;
			case 3://MPPE&MPPC
				strcpy(enctype, "+BOTH");
				break;
			default:
				strcpy(enctype, "none");
				break;
		}

		if (pentry->dgw) {
			snprintf(msg.data, BUF_SIZE,
					"spppctl add %d pptp auth %s username %s password %s server %s gw %d enctype %s", pentry->idx+PPTP_VPN_STRAT,
					auth, pentry->username, pentry->password, pentry->server, pentry->dgw, enctype);
		}
		else {
			snprintf(msg.data, BUF_SIZE,
					"spppctl add %d pptp auth %s username %s password %s server %s enctype %s", pentry->idx+PPTP_VPN_STRAT,
					auth, pentry->username, pentry->password, pentry->server, enctype);
		}

		if (pentry->conntype == CONNECT_ON_DEMAND)
		{
			len=BUF_SIZE-strlen(msg.data);
			snprintf(msg.data+strlen(msg.data), len, " timeout %d", pentry->idletime);
		}

		else if(pentry->conntype == CONNECT_ON_DEMAND_PKT_COUNT)
		{
			len=BUF_SIZE-strlen(msg.data);
			snprintf(msg.data+strlen(msg.data), len, " timeout_en 1 timeout %d", pentry->idletime);
		}

#ifdef CONFIG_IPV6_VPN
		len=BUF_SIZE-strlen(msg.data);
		snprintf(msg.data+strlen(msg.data), len, " ipt %u", pentry->IpProtocol - 1);
#endif

		printf("%s: %s\n", __func__, msg.data);

		rtk_wan_vpn_generate_ifup_script(VPN_TYPE_PPTP, pentry);
		rtk_wan_vpn_generate_ifdown_script(VPN_TYPE_PPTP, pentry);

		write_to_pppd(&msg);
	}
	else if(enable == 0 || enable == 3){
		snprintf(index, 5, "%d", pentry->idx+PPTP_VPN_STRAT);
		va_cmd("/bin/spppctl", 4, 1, "del", index, "pptp", "0");
		rtk_wan_vpn_remove_ifup_script(VPN_TYPE_PPTP, pentry);
		rtk_wan_vpn_do_ifdown_script(VPN_TYPE_PPTP, pentry);
		rtk_wan_vpn_remove_ifdown_script(VPN_TYPE_PPTP, pentry);
		rtk_net_remove_wan_policy_route_mark(ifName);
#ifdef YUEME_4_0_SPEC
		rtk_wan_vpn_remove_bind_wan_policy_route_rule(VPN_TYPE_PPTP, (void *)pentry);
#endif
	}
}

void pptp_take_effect(void)
{
	MIB_PPTP_T entry;
	unsigned int entrynum, i;
	int enable;

	if ( !mib_get(MIB_PPTP_ENABLE, (void *)&enable) )
		return;

	entrynum = mib_chain_total(MIB_PPTP_TBL);

	for (i=0; i<entrynum; i++)
	{
		if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&entry) )
			return;

		applyPPtP(&entry, 3, i);
	}

	if (enable) {
		va_cmd("/bin/modprobe", 1, 1, "pptp");

		for (i=0; i<entrynum; i++)
		{
			if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&entry) )
				return;

			applyPPtP(&entry, 2, i);
		}
	}
}

#elif defined(CONFIG_USER_PPTP_CLIENT)
void applyPPtP(MIB_PPTP_T *pentry, int enable, int pptp_index)
{
	unsigned int mark, mask;
	char buff[64];
	char ifName[IFNAMSIZ];

	ifGetName(pentry->ifIndex, ifName, sizeof(ifName));

	if (enable == 1) {
		rtk_net_add_wan_policy_route_mark(ifName, &mark, &mask);
		write_to_pptp_config();
		connect_pptp(PPP_INDEX(pentry->ifIndex));
	}
	else if (enable == 0) {
#ifdef CONFIG_USER_PPPD
		disconnectPPP(PPP_INDEX(pentry->ifIndex));
#else
		char ppppid_str[32];
		int ppp_pid=0;
		int status=1;
		snprintf(ppppid_str, 32, "%s/ppp%u.pid", PPPD_CONFIG_FOLDER, PPP_INDEX(pentry->ifIndex));

		ppp_pid = read_pid((char *)ppppid_str);
		if (ppp_pid >= 1) {
			// kill it
			status = kill(ppp_pid, SIGKILL);
			if (status != 0)
				printf("Could not kill PPP9(PPTP) pid '%d'\n", PPP_INDEX(pentry->ifIndex), ppp_pid);
			unlink(ppppid_str);
		}
#endif
		rtk_net_remove_wan_policy_route_mark(ifName);
	}
}

void pptp_take_effect(void)
{
	MIB_PPTP_T entry;
	unsigned int entrynum, i;
	int enable;
	int pptpType;

	if ( !mib_get(MIB_PPTP_ENABLE, (void *)&enable) )
		return;

	mib_get_s(MIB_PPTP_MODE, (void *)&pptpType, sizeof(pptpType));
	if (pptpType == PPTP_MODE_SERVER) return;

	entrynum = mib_chain_total(MIB_PPTP_TBL);

	for (i=0; i<entrynum; i++)
	{
		if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&entry) )
			return;

		applyPPtP(&entry, 0, i);
	}

	if (enable) {
		va_cmd("/bin/modprobe", 1, 1, "pptp");

		for (i=0; i<entrynum; i++)
		{
			if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&entry) )
				return;

			applyPPtP(&entry, 1, i);
		}
	}
}

#ifdef CONFIG_USER_PPTP_SERVER
void setup_pptp_server_firewall(PPTPS_RULE_TYPE_T type)
{
	char local_ipaddr[16] ={0};
	char str_subnet[32] ={0};
	char lan_ipaddr[16] ={0};
	int enable;
	int pptpType;
	struct in_addr lanIp;

	if ( !mib_get(MIB_PPTP_ENABLE, (void *)&enable) )
		return;
	if (!enable) return;

	mib_get_s(MIB_PPTP_MODE, (void *)&pptpType, sizeof(pptpType));
	if (pptpType == PPTP_MODE_CLIENT) return;

	/* clean pptp firewall rule */
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_PPTP_VPN_IN);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_PPTP_VPN_FW);

	//iptables -A pptp_vpn_input ! -i br+ -p udp --dport 1723 -j ACCEPT 
	va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)IPTABLE_PPTP_VPN_IN, "!", (char *)ARG_I, (char *)LANIF_ALL, "-p", "tcp", (char *)FW_DPORT, (char *)PORT_PPTP, "-j", (char *)FW_ACCEPT);

	getVpnServerLocalIP(VPN_PPTP, local_ipaddr);
	getVpnServerSubnet(VPN_PPTP, str_subnet);
	if (!str_subnet[0]) return;

	if (mib_get_s( MIB_ADSL_LAN_IP, (void *)&lanIp, sizeof(lanIp))) {
			if (lanIp.s_addr != INADDR_NONE ) {
				sprintf(lan_ipaddr, "%s", inet_ntoa(lanIp));
			}
	}

	if (type == PPTPS_RULE_TYPE_IN) {
		va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)IPTABLE_PPTP_VPN_IN, "-d", local_ipaddr, "-s", str_subnet, "-j", (char *)FW_ACCEPT);
		va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)IPTABLE_PPTP_VPN_IN, "-d", lan_ipaddr, "-s", str_subnet, "-j", (char *)FW_ACCEPT);
	}
	else if (type == PPTPS_RULE_TYPE_FW) {
		va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)IPTABLE_PPTP_VPN_FW, "-s", str_subnet, "-j", (char *)FW_ACCEPT);
		// accept wan to wan forward packets
		// iptables -A ipfilter ! -i br0  -o ppp+ -j DROP
		va_cmd(IPTABLES, 9, 1, (char *)FW_ADD, (char *)IPTABLE_PPTP_VPN_FW, "!", ARG_I, "br+"
			, "-d", str_subnet, "-j", (char *)FW_ACCEPT);
	}
	else {
		va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)IPTABLE_PPTP_VPN_IN, "-d", local_ipaddr, "-s", str_subnet, "-j", (char *)FW_ACCEPT);
		va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)IPTABLE_PPTP_VPN_IN, "-d", lan_ipaddr, "-s", str_subnet, "-j", (char *)FW_ACCEPT);
		va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)IPTABLE_PPTP_VPN_FW, "-s", str_subnet, "-j", (char *)FW_ACCEPT);
		// accept wan to wan forward packets
		// iptables -A ipfilter ! -i br0 -o ppp+ -j DROP
		va_cmd(IPTABLES, 9, 1, (char *)FW_ADD, (char *)IPTABLE_PPTP_VPN_FW, "!", ARG_I, "br+"
			, "-d", str_subnet, "-j", (char *)FW_ACCEPT);
	}
}

void applyPptpAccount(MIB_VPN_ACCOUNT_T *pentry, int enable)
{
	unsigned int entrynum, i;
	char pptpdpid_str[32];
	int pptpd_pid=0;
	char *argv[32];
	int idx;

	if (enable == 1) {
		entrynum = mib_chain_total(MIB_VPN_ACCOUNT_TBL);
		write_to_pptp_server_config();
		setup_pptp_server_firewall(PPTPS_RULE_TYPE_ALL);
		if (entrynum > 0) {
			idx = 0;
			argv[idx++] = (char *)PPTPD;
			//argv[idx++] = "-D";
			argv[idx++] = NULL;

			printf("%s", PPTPD);
			for (i=1; i<idx-1; i++)
				printf(" %s", argv[i]);
			printf("\n");
			//do_cmd(PPTPD, argv, 0);
			do_cmd(PPTPD, argv, 1);
		}
	}
	else{
		snprintf(pptpdpid_str, sizeof(pptpdpid_str), "%s", (char *)PPTPD_PID);
		kill_by_pidfile(pptpdpid_str, SIGTERM);

		/**********************************************************************/
		/* Write PAP/CHAP secrets file										  */
		/**********************************************************************/
		rtk_create_ppp_secrets();
	}
}

void pptp_server_take_effect(void)
{
	MIB_VPN_ACCOUNT_T entry;
	unsigned int entrynum, i;
	int enable;
	int pptpType;

	if ( !mib_get(MIB_PPTP_ENABLE, (void *)&enable) )
		return;

	mib_get_s(MIB_PPTP_MODE, (void *)&pptpType, sizeof(pptpType));
	if (pptpType == PPTP_MODE_CLIENT) return;

	/* clean pptp firewall rule */
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_PPTP_VPN_IN);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_PPTP_VPN_FW);

	entrynum = mib_chain_total(MIB_VPN_ACCOUNT_TBL);
	for (i=0; i<entrynum; i++)
	{
		if (!mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&entry))
			continue;
		if(VPN_PPTP == entry.type) {
			applyPptpAccount(&entry, 0);
			break;
		}
	}

	if (enable) {
		for (i=0; i<entrynum; i++)
		{
			if (!mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&entry))
				continue;
			if(VPN_PPTP == entry.type) {
				applyPptpAccount(&entry, 1);
				break;
			}
		}
		setup_pptp_server_firewall(PPTPS_RULE_TYPE_ALL);
	}
}
#endif
#endif


MIB_PPTP_T *getPPTPEntryByIfname(char *ifname, MIB_PPTP_T *p, int *entryIndex)
{
	int i,num;
	unsigned char mib_ifname[32];
	if( (p==NULL) || (ifname==NULL) ) return NULL;
	num = mib_chain_total( MIB_PPTP_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_PPTP_TBL, i, (void*)p ))
			continue;
		ifGetName(p->ifIndex, mib_ifname, sizeof(mib_ifname));
		if(!strcmp(mib_ifname, ifname))
		{
			if(entryIndex) *entryIndex = i;
			return p;
		}
	}
	return NULL;
}

MIB_PPTP_T *getPPTPEntryByTunnelName(char *tunnelName, MIB_PPTP_T *p, int *entryIndex)
{
	int i,num;
	if( (p==NULL) || (tunnelName==NULL) ) return NULL;
	num = mib_chain_total( MIB_PPTP_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_PPTP_TBL, i, (void*)p ))
			continue;
		if( !strcmp(p->tunnelName, tunnelName) )
		{
			if(entryIndex) *entryIndex = i;
			return p;
		}
	}
	return NULL;
}

MIB_PPTP_T *getPPTPEntryByIfIndex(unsigned int ifIndex, MIB_PPTP_T *p, int *entryIndex)
{
	int i,num;

	if( (p==NULL) || (ifIndex==DUMMY_IFINDEX) ) return NULL;
	num = mib_chain_total( MIB_PPTP_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_PPTP_TBL, i, (void*)p ))
			continue;
		if( p->ifIndex==ifIndex )
		{
			printf("%s-%d ifIndex=%d\n",__func__,__LINE__,ifIndex);
			if(entryIndex) *entryIndex = i;
			return p;
		}
	}
	return NULL;
}

int getIfIndexByPPTPName(char *pIfname)
{
	unsigned int entryNum, i;
	MIB_PPTP_T Entry;
	char ifname[IFNAMSIZ];

	entryNum = mib_chain_total(MIB_PPTP_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_PPTP_TBL, i, (void *)&Entry))
		{
			printf("Get chain record error!\n");
			return -1;
		}
		ifGetName(Entry.ifIndex,ifname,sizeof(ifname));
		if(!strcmp(ifname,pIfname)){
			break;
		}
	}
	if(i>= entryNum){
		printf("not find this interface!\n");
		return -1;
	}
	return(Entry.ifIndex);
}

#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD) \
	 || defined(CONFIG_USER_PPTP_CLIENT) || defined(CONFIG_USER_XL2TPD)
unsigned long long rtk_wan_vpn_get_attached_pkt_count_by_ifname(VPN_TYPE_T vpn_type, unsigned char *if_name)
{
	int entryIdx;
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	MIB_L2TP_T l2tpEntry;
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	MIB_PPTP_T pptpEntry;
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if(vpn_type==VPN_TYPE_L2TP)
	{
		if(getL2TPEntryByIfname(if_name, &l2tpEntry, &entryIdx) == NULL)
			return 0;
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		return rtk_wan_vpn_get_netfilter_packet_count(VPN_TYPE_L2TP, l2tpEntry.tunnelName);
#endif
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if(vpn_type == VPN_TYPE_PPTP)
	{
		if(getPPTPEntryByIfname(if_name, &pptpEntry, &entryIdx) == NULL)
			return 0;
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		return rtk_wan_vpn_get_netfilter_packet_count(VPN_TYPE_PPTP, pptpEntry.tunnelName);
#endif
	}
#endif
	return 0;
}

#ifdef VPN_MULTIPLE_RULES_BY_FILE
int rtk_wan_vpn_preset_url_policy_route_by_file(VPN_TYPE_T vpn_type, unsigned char *tunnelName, unsigned int rule_index, unsigned int priority, char *url)
{
	FILE *fp = NULL, *fp_lock = NULL;
	int totalEntry, i, weight;
	unsigned char regular_expression[MAX_DOMAIN_LENGTH] = {0};

	if (tunnelName == NULL || url == NULL)
		return -1;

	VPN_WRITE_LOCK(VPN_URL_ROUTE_TBL_LOCK, fp_lock);
	if(fp_lock == NULL)
	{
		fprintf(stderr, "[%s] Cannot get LOCK (%s)!\n", __FUNCTION__, VPN_URL_ROUTE_TBL_LOCK);
		return -1;
	}
	if(!(fp = fopen(VPN_URL_ROUTE_TBL, "a")))
	{
		fprintf(stderr, "[%s] ERROR to open file(%s)! %s\n", __FUNCTION__, VPN_URL_ROUTE_TBL, strerror(errno));
		VPN_RW_UNLOCK(fp_lock);
		return -2;
	}

	if(vpn_type == VPN_TYPE_L2TP || vpn_type == VPN_TYPE_PPTP)
	{
		if (url[0] != '\0')
		{
			weight = VPN_PRIO_7 - priority;
			domain_to_regular_expression(url , regular_expression);
			//vpnType, tunnelName, mibIdx, weight, url
			fprintf(fp, "%d %s %d %d %s\n", vpn_type, tunnelName, rule_index, weight, regular_expression);
		}
	}

	fclose(fp);
	VPN_RW_UNLOCK(fp_lock);

	return 0;
}

int rtk_wan_vpn_get_ipset_packet_count_by_route_index(VPN_TYPE_T vpn_type, unsigned char *tunnelName, unsigned int routeIdx, unsigned int rule_mode)
{
	char cmd[256] = {0}, tmp_tunnelName[256] = {0};
	unsigned char path[64] = {0};
	int packetCount = 0, totalCount = 0, tmp_routeIdx = -1, tmp_vpn_type = -1;
	FILE *fp_file = NULL, *fp_ipset_restore_file = NULL;
	char url[256] = {0};
	//unsigned char ipset_iprange_chain_name[64] = {0}, ipset_subnet_chain_name[64] = {0}, ipset_smac_chain_name[64] = {0};
	unsigned char ipset_chain_name[64] = {0};
	//unsigned char ipset_comment[128] = {0}, ipset_iprange_comment[128] = {0}, ipset_tmp_comment[256] = {0};
	unsigned char match_comment[128] = {0};
	int bits = 0;
	switch(vpn_type)
	{
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
		case VPN_TYPE_L2TP:
		{
			if (rule_mode == ATTACH_RULE_MODE_SINGLE_DIP || rule_mode == ATTACH_RULE_MODE_DIP_RANGE)
			{
				snprintf(ipset_chain_name, sizeof(ipset_chain_name), NF_L2TP_IPSET_IPRANGE_CHAIN, tunnelName);
				snprintf(match_comment, sizeof(match_comment), NF_L2TP_IPSET_RULE_INDEX_COMMENT, rule_mode, routeIdx);

			}
			//else if (rule_mode == ATTACH_RULE_MODE_DIP_RANGE)
			else if (rule_mode == ATTACH_RULE_MODE_DIP_RANGE_SUBNET)
			{
				snprintf(ipset_chain_name, sizeof(ipset_chain_name), NF_L2TP_IPSET_IPSUBNET_CHAIN, tunnelName);
				snprintf(match_comment, sizeof(match_comment), NF_L2TP_IPSET_RULE_INDEX_COMMENT, rule_mode, routeIdx);
			}
			else if (rule_mode == ATTACH_RULE_MODE_SMAC)
			{
				snprintf(ipset_chain_name, sizeof(ipset_chain_name), NF_L2TP_IPSET_SMAC_CHAIN, tunnelName);
				snprintf(match_comment, sizeof(match_comment), NF_L2TP_IPSET_RULE_INDEX_COMMENT, rule_mode, routeIdx);
			}
			else if (rule_mode == ATTACH_RULE_MODE_DOMAIN)
			{
				snprintf(ipset_chain_name, sizeof(ipset_chain_name), NF_L2TP_IPSET_IPRANGE_CHAIN, tunnelName);
				//format ex: 124.108.103.103 packets 8 bytes 480 comment "l2tp_ruletype_5_route_idx_0,url:www.yahoo.com"
				snprintf(match_comment, sizeof(match_comment), NF_L2TP_IPSET_RULE_INDEX_COMMENT, rule_mode, routeIdx);
				//snprintf(match_comment, sizeof(match_comment), ",url:%s", url);
			}

			snprintf(path, sizeof(cmd), VPN_L2TP_IPSET_MATCH_PACKET_COUNT_FILE, routeIdx);
			snprintf(cmd, sizeof(cmd), "%s list %s | grep \"%s\" > %s", IPSET, ipset_chain_name, match_comment, path);
			break;
		}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) ||  defined(CONFIG_USER_PPTP_CLIENT)
		case VPN_TYPE_PPTP:
		{
			if (rule_mode == ATTACH_RULE_MODE_SINGLE_DIP || rule_mode == ATTACH_RULE_MODE_DIP_RANGE)
			{
				snprintf(ipset_chain_name, sizeof(ipset_chain_name), NF_PPTP_IPSET_IPRANGE_CHAIN, tunnelName);
				snprintf(match_comment, sizeof(match_comment), NF_PPTP_IPSET_RULE_INDEX_COMMENT, rule_mode, routeIdx);

			}
			//else if (rule_mode == ATTACH_RULE_MODE_DIP_RANGE)
			else if (rule_mode == ATTACH_RULE_MODE_DIP_RANGE_SUBNET)
			{
				snprintf(ipset_chain_name, sizeof(ipset_chain_name), NF_PPTP_IPSET_IPSUBNET_CHAIN, tunnelName);
				snprintf(match_comment, sizeof(match_comment), NF_PPTP_IPSET_RULE_INDEX_COMMENT, rule_mode, routeIdx);
			}
			else if (rule_mode == ATTACH_RULE_MODE_SMAC)
			{
				snprintf(ipset_chain_name, sizeof(ipset_chain_name), NF_PPTP_IPSET_SMAC_CHAIN, tunnelName);
				snprintf(match_comment, sizeof(match_comment), NF_PPTP_IPSET_RULE_INDEX_COMMENT, rule_mode, routeIdx);
			}
			else if (rule_mode == ATTACH_RULE_MODE_DOMAIN)
			{
				snprintf(ipset_chain_name, sizeof(ipset_chain_name), NF_PPTP_IPSET_IPRANGE_CHAIN, tunnelName);
				//format ex: 31.13.87.36 packets 3 bytes 180 comment "pptp_ruletype_5_route_idx_0,url:www.facebook.com"
				snprintf(match_comment, sizeof(match_comment), NF_PPTP_IPSET_RULE_INDEX_COMMENT, rule_mode, routeIdx);
				//snprintf(match_comment, sizeof(match_comment), ",url:%s", url);
			}

			snprintf(path, sizeof(cmd), VPN_PPTP_IPSET_MATCH_PACKET_COUNT_FILE, routeIdx);
			snprintf(cmd, sizeof(cmd), "%s list %s | grep \"%s\" > %s", IPSET, ipset_chain_name, match_comment, path);
			break;
		}
#endif
		default:
			return -1;
	}
	//printf("%s: cmd = %s\n", __FUNCTION__, cmd);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	if ((fp_file = fopen(path, "r")))
	{
		while (fgets(cmd, sizeof(cmd), fp_file) != NULL)
		{
			//ex: 8.8.8.8 packets 0 bytes 0 comment "l2tp_ruletype_2_route_idx_5"
			if (sscanf(cmd,"%*s %*s %d %*s %*d %*s %*s", &packetCount) == 1)
			{
				totalCount += packetCount;
			}
		}
		fclose(fp_file);
	}
	unlink(path);
	return totalCount;
}
#endif

unsigned long long rtk_wan_vpn_get_netfilter_packet_count_by_route_index(VPN_TYPE_T vpn_type, unsigned char *tunnelName, unsigned int routeIdx)
{
	char cmd[256];
	unsigned char chain_name[64]={0}, path[64]={0}, comment_name[64];
	const char *table_name;
	unsigned long long packetCount=0, totalCount=0;
	FILE *fp = NULL;

	switch(vpn_type)
	{
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
		case VPN_TYPE_L2TP:
		{
			table_name = NF_VPN_ROUTE_TABLE;
			sprintf(chain_name, NF_L2TP_ROUTE_CHAIN, tunnelName);
			sprintf(comment_name, NF_L2TP_ROUTE_COMMENT, routeIdx);
			break;
		}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
		case VPN_TYPE_PPTP:
		{
			table_name = NF_VPN_ROUTE_TABLE;
			sprintf(chain_name, NF_PPTP_ROUTE_CHAIN, tunnelName);
			sprintf(comment_name, NF_PPTP_ROUTE_COMMENT, routeIdx);
			break;
		}
#endif
		default:
			return -1;
	}
	sprintf(path, "/tmp/vpn_%d_%s-attch-rules_%d", vpn_type, tunnelName, routeIdx);
	sprintf(cmd, "%s -t %s -nvL %s | grep \"/\\* %s .*\\*/\" > %s", IPTABLES, table_name, chain_name, comment_name,  path);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	if((fp=fopen(path, "r")))
	{
		while(fgets(cmd,sizeof(cmd),fp)!=NULL)
		{
			if(sscanf(cmd,"%lld %*d %*s", &packetCount) == 1)
			{
				totalCount+=packetCount;
			}
		}
		fclose(fp);
	}
	unlink(path);

	return totalCount;
}

unsigned long long rtk_wan_vpn_get_attached_pkt_count_by_route_index(VPN_TYPE_T vpn_type, unsigned int index)
{
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	MIB_CE_L2TP_ROUTE_T l2tpRouteEntry;
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	MIB_CE_PPTP_ROUTE_T pptpRouteEntry;
#endif

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if(vpn_type==VPN_TYPE_L2TP)
	{
		if(mib_chain_get(MIB_L2TP_ROUTE_TBL, index, (void *)&l2tpRouteEntry) == 0)
			return 0;
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		return rtk_wan_vpn_get_netfilter_packet_count_by_route_index(VPN_TYPE_L2TP, l2tpRouteEntry.tunnelName, index);
#endif
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if(vpn_type == VPN_TYPE_PPTP)
	{
		if(mib_chain_get(MIB_PPTP_ROUTE_TBL, index, (void *)&pptpRouteEntry) == 0)
			return 0;
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		return rtk_wan_vpn_get_netfilter_packet_count_by_route_index(VPN_TYPE_PPTP, pptpRouteEntry.tunnelName, index);
#endif
	}
#endif

	return 0;
}

int getVPNOutIfname(VPN_TYPE_T type, void *pentry, char *oifname)
{
	char buff[256], found = 0;
	char ifname[IFNAMSIZ] = {0}, ppp_ifname[IFNAMSIZ] = {0};
	char out_ifname[IFNAMSIZ] = {0};
	FILE *fp = NULL;
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if(type == VPN_TYPE_L2TP)
	{
		ifGetName(((MIB_L2TP_T *)pentry)->ifIndex,ifname,sizeof(ifname));
	}
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if(type == VPN_TYPE_PPTP)
	{
		ifGetName(((MIB_PPTP_T *)pentry)->ifIndex,ifname,sizeof(ifname));
	}
#endif
	if(ifname[0] == '\0') return 0;

	if (!(fp=fopen(PPP_CONF, "r"))) {
		printf("%s not exists.\n", PPP_CONF);
		return 0;
	}
	else {
		fgets(buff, sizeof(buff), fp);
		while( fgets(buff, sizeof(buff), fp) != NULL ) {
			if(sscanf(buff, "%s %*s %s %*s", ppp_ifname, out_ifname) != 2) {
				found=0;
				printf("Unsuported ppp configuration format\n");
				break;
			}

			if ( !strcmp(ifname, ppp_ifname) ) {
				found = 1;
				break;
			}
		}
		fclose(fp);
	}

	if(found){
		if(oifname) strcpy(oifname, out_ifname);
		return 1;
	}

	return 0;
}
#endif
/* Function define end */

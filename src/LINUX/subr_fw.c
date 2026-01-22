#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "mib.h"
#include "utility.h"
#include "subr_net.h"
#include "defs.h"
#include "debug.h"
#include <signal.h>
#include "sockmark_define.h"
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#include <rt_flow_ext.h>
#endif

#if defined(CONFIG_COMMON_RT_API)
#include "common/error.h"
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#include "fc_api.h"
#endif
#endif

#ifdef CONFIG_CU
#include "subr_cu.h"
#endif

#define SUBRFW_MSG(fmt,args...)		printf("subr_fw: "fmt"\n", ##args)
#define SUBRFW_INFO(fmt,args...)	printf("\033[1;33;46m""subr_fw: "fmt"\033[m""\n", ##args)
#define SUBRFW_ERROR(fmt,args...)	printf("\033[1;33;41m""subr_fw: "fmt"\033[m""\n", ##args)

#define SUBRFW_DEBUGGING
#ifdef SUBRFW_DEBUGGING
#define SUBRFW_DEBUG			SUBRFW_INFO
#else
#define SUBRFW_DEBUG(fmt,args...)
#endif
#if defined(URL_BLOCKING_SUPPORT) || defined(URL_ALLOWING_SUPPORT)
typedef struct _URLBLK_PORT_LIST{
	int port;
	int ref;
	struct _URLBLK_PORT_LIST *next;
}URLBLK_PORT_LIST;
#ifdef SUPPORT_TIME_SCHEDULE
unsigned int schdURLFilterIdx[SCHD_URL_FILTER_SIZE];
unsigned int schdIpFilterIdx[SCHD_IP_FILTER_SIZE];
unsigned int schdIp6FilterIdx[SCHD_IP6_FILTER_SIZE];
#endif

int checkUrlFilterPortList(URLBLK_PORT_LIST **portList, int port, int config)
{
	unsigned char found = 0;
	URLBLK_PORT_LIST *pNext = *portList, *pLast = NULL;
	while(pNext) {
		if(pNext->port == port) {
			found = 1;
			if(config) pNext->ref++;
			else pNext->ref = (pNext->ref > 0) ? pNext->ref-- : 0;
			break;
		}
		pLast = pNext;
		pNext = pLast->next;
	}
	if(!found) {
		pNext = calloc(1, sizeof(URLBLK_PORT_LIST));
		if(pNext) {
			pNext->port = port;
			if(config) pNext->ref = 1;
			if(pLast)
				pLast->next = pNext;
			else
				*portList = pNext;
		}
	}
	return 1;
}

int freeUrlFilterPortList(URLBLK_PORT_LIST *portList)
{
	URLBLK_PORT_LIST *p = portList, *p1;
	while(p)
	{
		p1 = p->next;
		free(p);
		p = p1;
	}
	return 0;
}

typedef struct _URLBLK_MAC_LIST{
	char mac[18];
	int ref;
	struct _URLBLK_MAC_LIST *next;
}URLBLK_MAC_LIST;

int checkUrlFilterMacList(URLBLK_MAC_LIST **macList, char *mac, int config)
{
	unsigned char found = 0;
	URLBLK_MAC_LIST *pNext = *macList, *pLast = NULL;
	while(pNext) {
		if(!strcmp(pNext->mac, mac)) {
			found = 1;
			if(config) pNext->ref++;
			else pNext->ref = (pNext->ref > 0) ? pNext->ref-- : 0;
			break;
		}
		pLast = pNext;
		pNext = pLast->next;
	}
	if(!found) {
		pNext = calloc(1, sizeof(URLBLK_MAC_LIST));
		if(pNext) {
			strcpy(pNext->mac, mac);
			if(config) pNext->ref = 1;
			if(pLast)
				pLast->next = pNext;
			else
				*macList = pNext;
		}
	}
	return 1;
}

int freeUrlFilterMacList(URLBLK_MAC_LIST *macList)
{
	URLBLK_MAC_LIST *p = macList, *p1;
	while(p)
	{
		p1 = p->next;
		free(p);
		p = p1;
	}
	return 0;
}

#define URLBLK_PORT_FILTER "urlblk_port_filter"
#define URLBLK_MAC_FILTER "urlblk_mac_filter"
#define URLBLK_RULE_ENABLE_FILE "/var/URLBLK_RULE_ENABLE"
#define URLBLK_WHITE_DEF_FULE "urlblk_white_def_rules"
#define URLBLK_BLACK_DEF_FULE "urlblk_black_def_rules"
int config_url_filter_rule(int config, URLBLK_PORT_LIST *portList, URLBLK_MAC_LIST *macList)
{
	FILE *fp;
	unsigned char flush_main_rule=0, flush_entry_rule=0;
	unsigned char config_main_rule=0, config_entry_rule=0;
	unsigned char defWhiteRule = 0;
	char cmd[1025];
	URLBLK_PORT_LIST *pPortList;
	URLBLK_MAC_LIST *pMacList;
#ifdef CONFIG_RTK_SKB_MARK2
	unsigned long long mask, mark;
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	unsigned long long trap_mask, trap_mark;
#endif

	mask = SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_BIT_MASK;
	mark = SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_BIT_MASK;
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	trap_mask = SOCK_MARK2_FORWARD_BY_PS_BIT_MASK;
	trap_mark = SOCK_MARK2_FORWARD_BY_PS_BIT_MASK;
#endif
#endif

	if(config)
	{
		if(config == 2)
		{
			// check URL work rule file
			fp = fopen(URLBLK_RULE_ENABLE_FILE, "r");
			if(fp)
				fclose(fp);
			else
			{
				flush_main_rule = 1;
				config_main_rule = 1;
			}
			config_entry_rule = 1;
		}
		else
		{
			flush_main_rule = 1;
			config_main_rule = 1;
			config_entry_rule = 1;
		}
	}
	else {
		flush_main_rule = 1;
		flush_entry_rule = 1;
	}

	if(flush_main_rule)
	{
		va_cmd(IPTABLES, 2, 1, "-F", WEBURL_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", WEBURLPOST_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", WEBURLPRE_CHAIN);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 2, 1, "-F", WEBURL_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", WEBURLPOST_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", WEBURLPRE_CHAIN);
#endif
#ifdef CONFIG_IP_SET
		snprintf(cmd, sizeof(cmd), "%s flush %s", IPSET, URLBLK_PORT_FILTER);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s destroy %s", IPSET, URLBLK_PORT_FILTER);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s create %s bitmap:port range 1-65535", IPSET, URLBLK_PORT_FILTER);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);

		snprintf(cmd, sizeof(cmd), "%s flush %s", IPSET, URLBLK_MAC_FILTER);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s destroy %s", IPSET, URLBLK_MAC_FILTER);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s create %s hash:mac counters", IPSET, URLBLK_MAC_FILTER);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif

		unlink(URLBLK_RULE_ENABLE_FILE);
	}

	if(flush_entry_rule)
	{
		va_cmd(IPTABLES, 2, 1, "-F", WEBURL_CHAIN_BLACK);
		va_cmd(IPTABLES, 2, 1, "-X", WEBURL_CHAIN_BLACK);
		va_cmd(IPTABLES, 2, 1, "-N", WEBURL_CHAIN_BLACK);
		va_cmd(IPTABLES, 2, 1, "-F", WEBURL_CHAIN_WHITE);
		va_cmd(IPTABLES, 2, 1, "-X", WEBURL_CHAIN_WHITE);
		va_cmd(IPTABLES, 2, 1, "-N", WEBURL_CHAIN_WHITE);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 2, 1, "-F", WEBURL_CHAIN_BLACK);
		va_cmd(IP6TABLES, 2, 1, "-X", WEBURL_CHAIN_BLACK);
		va_cmd(IP6TABLES, 2, 1, "-N", WEBURL_CHAIN_BLACK);
		va_cmd(IP6TABLES, 2, 1, "-F", WEBURL_CHAIN_WHITE);
		va_cmd(IP6TABLES, 2, 1, "-X", WEBURL_CHAIN_WHITE);
		va_cmd(IP6TABLES, 2, 1, "-N", WEBURL_CHAIN_WHITE);
#endif
	}

	if(config_main_rule)
	{
#ifdef CONFIG_RTK_SKB_MARK2
		snprintf(cmd, sizeof(cmd), "%s -t mangle -A %s -p tcp -m set --match-set %s dst -m conntrack --ctstate ESTABLISHED -j CONNMARK2 --restore-mark2 --ctmask2 0x%llx --nfmask 0x%llx",
									IPTABLES, WEBURLPRE_CHAIN, URLBLK_PORT_FILTER, mask, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s -t mangle -A %s -p tcp -m set --match-set %s dst -m conntrack --ctstate ESTABLISHED -j CONNMARK2 --save-mark2 --ctmask2 0x%llx --nfmask 0x%llx",
									IPTABLES, WEBURLPOST_CHAIN, URLBLK_PORT_FILTER, mask, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
		snprintf(cmd, sizeof(cmd), "%s -t mangle -A %s -p tcp -m set --match-set %s dst -m mark2 --mark2 0x0/0x%llx -j MARK2 --set-mark2 0x%llx/0x%llx",
									IPTABLES, WEBURLPOST_CHAIN, URLBLK_PORT_FILTER, mask, trap_mark, trap_mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
		snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m set ! --match-set %s dst -j RETURN",
									IPTABLES, WEBURL_CHAIN, URLBLK_PORT_FILTER);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m mark2 --mark2 0x%llx/0x%llx -j RETURN",
									IPTABLES, WEBURL_CHAIN, mark, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);

		//Balck rule
		snprintf(cmd, sizeof(cmd), "%s -A %s -j %s",
									IPTABLES, WEBURL_CHAIN, WEBURL_CHAIN_BLACK);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s -D %s -m comment --comment %s -j MARK2 --set-mark2 0x%llx/0x%llx",
									IPTABLES, WEBURL_CHAIN_BLACK, URLBLK_BLACK_DEF_FULE, mark, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s -A %s -m comment --comment %s -j MARK2 --set-mark2 0x%llx/0x%llx",
									IPTABLES, WEBURL_CHAIN_BLACK, URLBLK_BLACK_DEF_FULE, mark, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);

		//White rule
		snprintf(cmd, sizeof(cmd), "%s -A %s -m mark2 --mark2 0x%llx/0x%llx -j %s",
									IPTABLES, WEBURL_CHAIN, mark, mask, WEBURL_CHAIN_WHITE);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s -D %s -m set --match-set %s src -j MARK2 --set-mark2 0x0/0x%llx",
									IPTABLES, WEBURL_CHAIN_WHITE, URLBLK_MAC_FILTER, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s -A %s -m set --match-set %s src -j MARK2 --set-mark2 0x0/0x%llx",
									IPTABLES, WEBURL_CHAIN_WHITE, URLBLK_MAC_FILTER, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);

		// default drop rule
		snprintf(cmd, sizeof(cmd), "%s -A %s -m mark2 --mark2 0x0/0x%llx -j DROP",
									IPTABLES, WEBURL_CHAIN, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);


#ifdef CONFIG_IPV6
		snprintf(cmd, sizeof(cmd), "%s -t mangle -A %s -p tcp -m set --match-set %s dst -m conntrack --ctstate ESTABLISHED -j CONNMARK2 --restore-mark2 --ctmask2 0x%llx --nfmask 0x%llx",
									IP6TABLES, WEBURLPRE_CHAIN, URLBLK_PORT_FILTER, mask, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s -t mangle -A %s -p tcp -m set --match-set %s dst -m conntrack --ctstate ESTABLISHED -j CONNMARK2 --save-mark2 --ctmask2 0x%llx --nfmask 0x%llx",
									IP6TABLES, WEBURLPOST_CHAIN, URLBLK_PORT_FILTER, mask, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
		snprintf(cmd, sizeof(cmd), "%s -t mangle -A %s -p tcp -m set --match-set %s dst -m mark2 --mark2 0x0/0x%llx -j MARK2 --set-mark2 0x%llx/0x%llx",
									IP6TABLES, WEBURLPOST_CHAIN, URLBLK_PORT_FILTER, mask, trap_mark, trap_mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
		snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m set ! --match-set %s dst -j RETURN",
									IP6TABLES, WEBURL_CHAIN, URLBLK_PORT_FILTER);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m mark2 --mark2 0x%llx/0x%llx -j RETURN",
									IP6TABLES, WEBURL_CHAIN, mark, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		//Balck rule
		snprintf(cmd, sizeof(cmd), "%s -A %s -j %s",
									IP6TABLES, WEBURL_CHAIN, WEBURL_CHAIN_BLACK);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s -D %s -m comment --comment %s -j MARK2 --set-mark2 0x%llx/0x%llx",
									IP6TABLES, WEBURL_CHAIN_BLACK, URLBLK_BLACK_DEF_FULE, mark, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s -A %s -m comment --comment %s -j MARK2 --set-mark2 0x%llx/0x%llx",
									IP6TABLES, WEBURL_CHAIN_BLACK, URLBLK_BLACK_DEF_FULE, mark, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);

		//White rule
		snprintf(cmd, sizeof(cmd), "%s -A %s -m mark2 --mark2 0x%llx/0x%llx -j %s",
									IP6TABLES, WEBURL_CHAIN, mark, mask, WEBURL_CHAIN_WHITE);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s -D %s -m set --match-set %s src -j MARK2 --set-mark2 0x0/0x%llx",
									IP6TABLES, WEBURL_CHAIN_WHITE, URLBLK_MAC_FILTER, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s -A %s -m set --match-set %s src -j MARK2 --set-mark2 0x0/0x%llx",
									IP6TABLES, WEBURL_CHAIN_WHITE, URLBLK_MAC_FILTER, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);

		// default drop rule
		snprintf(cmd, sizeof(cmd), "%s -A %s -m mark2 --mark2 0x0/0x%llx -j DROP",
									IP6TABLES, WEBURL_CHAIN, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
		// write URL work rule file
		fp = fopen(URLBLK_RULE_ENABLE_FILE, "w");
		if(fp)fclose(fp);
#endif
	}

	if(config_entry_rule)
	{
		pPortList = portList;
		while(pPortList)
		{
			if(pPortList->port > 0 && pPortList->port < 65535)
			{
#ifdef CONFIG_IP_SET
				if(pPortList->ref > 0)
					snprintf(cmd, sizeof(cmd), "%s add %s %d &> /dev/null", IPSET, URLBLK_PORT_FILTER, pPortList->port);
				else
					snprintf(cmd, sizeof(cmd), "%s del %s %d &> /dev/null", IPSET, URLBLK_PORT_FILTER, pPortList->port);
				va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
			}
			pPortList = pPortList->next;
		}

		pMacList = macList;
		while(pMacList)
		{
			if(pMacList->mac[0] != ' ')
			{
#ifdef CONFIG_IP_SET
				if(pMacList->ref > 0)
					snprintf(cmd, sizeof(cmd), "%s add %s %s &> /dev/null", IPSET, URLBLK_MAC_FILTER, pMacList->mac);
				else
					snprintf(cmd, sizeof(cmd), "%s del %s %s &> /dev/null", IPSET, URLBLK_MAC_FILTER, pMacList->mac);
				va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
			}
			else if(pMacList->ref > 0)
				defWhiteRule = 1;

			pMacList = pMacList->next;
		}
#ifdef CONFIG_RTK_SKB_MARK2
		snprintf(cmd, sizeof(cmd), "%s -D %s -m comment --comment %s -j MARK2 --set-mark2 0x0/0x%llx ",
									IPTABLES, WEBURL_CHAIN_WHITE, URLBLK_WHITE_DEF_FULE, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef CONFIG_IPV6
		snprintf(cmd, sizeof(cmd), "%s -D %s -m comment --comment %s -j MARK2 --set-mark2 0x0/0x%llx",
									IP6TABLES, WEBURL_CHAIN_WHITE, URLBLK_WHITE_DEF_FULE, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
		if(defWhiteRule)
		{
			snprintf(cmd, sizeof(cmd), "%s -A %s -m comment --comment %s -j MARK2 --set-mark2 0x0/0x%llx",
										IPTABLES, WEBURL_CHAIN_WHITE, URLBLK_WHITE_DEF_FULE, mask);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef CONFIG_IPV6
			snprintf(cmd, sizeof(cmd), "%s -A %s -m comment --comment %s -j MARK2 --set-mark2 0x0/0x%llx",
										IP6TABLES, WEBURL_CHAIN_WHITE, URLBLK_WHITE_DEF_FULE, mask);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
		}
#endif
	}

	return 1;
}

// Added by Mason Yu for URL Blocking
void filter_set_url(int enable)
{
	int i, j, total, totalKeywd, rule_black_count=0, rule_white_count=0;
	MIB_CE_URL_FQDN_T Entry;
	MIB_CE_KEYWD_FILTER_T filterEntry;
#ifdef URL_ALLOWING_SUPPORT
	MIB_CE_URL_FQDN_T allowEntry;
#endif
	URLBLK_PORT_LIST *portList = NULL;
	URLBLK_MAC_LIST *macList = NULL;
	char reject_log_action[64]={0}, accept_log_action[64]={0}, reject_action[64], accept_action[64]={0};
#ifdef SUPPORT_TIME_SCHEDULE
	time_t curTime;
	struct tm curTm;
	int shouldSchedule = 0;
	unsigned int isScheduled;
	MIB_CE_TIME_SCHEDULE_T schdEntry;
	int totalSched=0;

	totalSched = mib_chain_total(MIB_TIME_SCHEDULE_TBL);
	time(&curTime);
	memset(schdURLFilterIdx, 0, sizeof(schdURLFilterIdx));
	memcpy(&curTm, localtime(&curTime), sizeof(curTm));
#endif

	//Kevin, check does it need to enable/disable IP fastpath status
	UpdateIpFastpathStatus();

	// check if URL Capability is enabled ?

	config_url_filter_rule(0, NULL, NULL);

	// reset action chain of iptables
	snprintf(reject_log_action, sizeof(reject_log_action), "URLBLK_ACT_REJECT_LOG");
	va_cmd(IPTABLES, 2, 1, "-F", reject_log_action);
	va_cmd(IPTABLES, 2, 1, "-X", reject_log_action);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", reject_log_action);
	va_cmd(IP6TABLES, 2, 1, "-X", reject_log_action);
#endif

	snprintf(reject_action, sizeof(reject_action), "URLBLK_ACT_REJECT");
	va_cmd(IPTABLES, 2, 1, "-F", reject_action);
	va_cmd(IPTABLES, 2, 1, "-X", reject_action);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", reject_action);
	va_cmd(IP6TABLES, 2, 1, "-X", reject_action);
#endif

	snprintf(accept_log_action, sizeof(accept_log_action), "URLBLK_ACT_ACCEPT_LOG");
	va_cmd(IPTABLES, 2, 1, "-F", accept_log_action);
	va_cmd(IPTABLES, 2, 1, "-X", accept_log_action);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", accept_log_action);
	va_cmd(IP6TABLES, 2, 1, "-X", accept_log_action);
#endif

	snprintf(accept_action, sizeof(accept_action), "URLBLK_ACT_ACCEPT");
	va_cmd(IPTABLES, 2, 1, "-F", accept_action);
	va_cmd(IPTABLES, 2, 1, "-X", accept_action);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", accept_action);
	va_cmd(IP6TABLES, 2, 1, "-X", accept_action);
#endif

	if (enable == 0)
		return;

	checkUrlFilterPortList(&portList, 80, 1);
#ifdef CONFIG_NETFILTER_XT_MATCH_WEBURL
	checkUrlFilterPortList(&portList, 443, 1);
#endif

	// init action chain of iptables
	va_cmd(IPTABLES, 2, 1, "-N", reject_log_action);
	va_cmd(IPTABLES, 8, 1,  (char *)FW_ADD, reject_log_action, "-j", "LOG", "--log-prefix", "URLFilter black rejected: ", "--log-level", "2");
	va_cmd(IPTABLES, 8, 1,  (char *)FW_ADD, reject_log_action, "-p", "tcp", "-j", "REJECT", "--reject-with", "tcp-reset");
	va_cmd(IPTABLES, 4, 1,  (char *)FW_ADD, reject_log_action, "-j", "DROP");
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", reject_log_action);
	va_cmd(IP6TABLES, 8, 1,  (char *)FW_ADD, reject_log_action, "-j", "LOG", "--log-prefix", "URLFilter black rejected: ", "--log-level", "2");
	va_cmd(IP6TABLES, 8, 1,  (char *)FW_ADD, reject_log_action, "-p", "tcp", "-j", "REJECT", "--reject-with", "tcp-reset");
	va_cmd(IP6TABLES, 4, 1,  (char *)FW_ADD, reject_log_action, "-j", "DROP");
#endif

	va_cmd(IPTABLES, 2, 1, "-N", reject_action);
	va_cmd(IPTABLES, 8, 1,  (char *)FW_ADD, reject_action, "-p", "tcp", "-j", "REJECT", "--reject-with", "tcp-reset");
	va_cmd(IPTABLES, 4, 1,  (char *)FW_ADD, reject_action, "-j", "DROP");
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", reject_action);
	va_cmd(IP6TABLES, 8, 1,  (char *)FW_ADD, reject_action, "-p", "tcp", "-j", "REJECT", "--reject-with", "tcp-reset");
	va_cmd(IP6TABLES, 4, 1,  (char *)FW_ADD, reject_action, "-j", "DROP");
#endif

	va_cmd(IPTABLES, 2, 1, "-N", accept_log_action);
	va_cmd(IPTABLES, 8, 1,  (char *)FW_ADD, accept_log_action, "-j", "LOG", "--log-prefix", "URLFilter white accepted: ", "--log-level", "2");
	va_cmd(IPTABLES, 4, 1,  (char *)FW_ADD, accept_log_action, "-j", "RETURN");
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", accept_log_action);
	va_cmd(IP6TABLES, 8, 1,  (char *)FW_ADD, accept_log_action, "-j", "LOG", "--log-prefix", "URLFilter white accepted: ", "--log-level", "2");
	va_cmd(IP6TABLES, 4, 1,  (char *)FW_ADD, accept_log_action, "-j", "RETURN");
#endif

	// Add URL policy to urlblock chain
	total = mib_chain_total(MIB_URL_FQDN_TBL);

	for (i=0; i<total; i++) {
		if (!mib_chain_get(MIB_URL_FQDN_TBL, i, (void *)&Entry))
			continue;
#ifdef SUPPORT_TIME_SCHEDULE
		if(Entry.schedIndex >= 1 && Entry.schedIndex <= totalSched)
		{
			if (!mib_chain_get(MIB_TIME_SCHEDULE_TBL, Entry.schedIndex-1, (void *)&schdEntry))
			{
				continue;
			}
			shouldSchedule = check_should_schedule(&curTm, &schdEntry);
			isScheduled = isScheduledIndex(i, schdURLFilterIdx, SCHD_URL_FILTER_SIZE);
			if((shouldSchedule && (isScheduled == 0)) ||
				((shouldSchedule == 0) && isScheduled))
			{
				if(shouldSchedule && (isScheduled == 0))
				{
					//not scheduled yet, should add rule
					addScheduledIndex(i, schdURLFilterIdx, SCHD_URL_FILTER_SIZE);
				}
				else
				{
					//scheduled but is out of schedule time, should delete rule
					delScheduledIndex(i, schdURLFilterIdx, SCHD_URL_FILTER_SIZE);
					continue;
				}
			}
			else
			{
				continue;
			}
		}
#endif

		if(Entry.fqdn[0] == '\0')
			continue;

		if (enable == 1)//blacklist
		{
#ifdef CONFIG_NETFILTER_XT_MATCH_WEBURL
			va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, WEBURL_CHAIN_BLACK, "-p", "tcp", "-m", "weburl", "--contains", Entry.fqdn, "--domain_only", "-g", reject_log_action);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 11, 1, (char *)FW_ADD, WEBURL_CHAIN_BLACK, "-p", "tcp", "-m", "weburl", "--contains", Entry.fqdn, "--domain_only", "-g", reject_log_action);
#endif
#else
			va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, WEBURL_CHAIN_BLACK, "-p", "tcp", "-m", "string", "--url", Entry.fqdn, "--algo", "bm", "-g", reject_log_action);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 12, 1, (char *)FW_ADD, WEBURL_CHAIN_BLACK, "-p", "tcp", "-m", "string", "--url", Entry.fqdn, "--algo", "bm", "-g", reject_log_action);
#endif
#endif
			rule_black_count++;
		}
		else if (enable == 2)//whitelist
		{
#ifdef CONFIG_NETFILTER_XT_MATCH_WEBURL
			va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, WEBURL_CHAIN_WHITE, "-p", "tcp", "-m", "weburl", "--contains", Entry.fqdn, "--domain_only", "-g", accept_log_action);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 11, 1, (char *)FW_ADD, WEBURL_CHAIN_WHITE, "-p", "tcp", "-m", "weburl", "--contains", Entry.fqdn, "--domain_only", "-g", accept_log_action);
#endif
#else
			va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, WEBURL_CHAIN_WHITE, "-p", "tcp", "--dport", "80", "-m", "string", "--urlalw", Entry.fqdn, "--algo", "bm", "-g", accept_log_action);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, WEBURL_CHAIN_WHITE, "-p", "tcp", "--dport", "80", "-m", "string", "--urlalw", Entry.fqdn, "--algo", "bm", "-g", accept_log_action);
#endif
#endif
			rule_white_count++;
		}
	}

	// Add Keyword filtering policy to urlblock chain
	totalKeywd = mib_chain_total(MIB_KEYWD_FILTER_TBL);

	for (i=0; i<totalKeywd; i++) {
		if (!mib_chain_get(MIB_KEYWD_FILTER_TBL, i, (void *)&filterEntry))
			continue;

		if(filterEntry.keyword[0] == '\0')
			continue;

#ifdef CONFIG_NETFILTER_XT_MATCH_WEBURL
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, WEBURL_CHAIN_BLACK, "-p", "tcp", "-m", "weburl", "--contains", filterEntry.keyword, "-g", reject_action);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, WEBURL_CHAIN_BLACK, "-p", "tcp", "-m", "weburl", "--contains", filterEntry.keyword, "-g", reject_action);
#endif
#else
		va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, WEBURL_CHAIN_BLACK, "-p", "tcp", "--dport", "80", "-m", "string", "--url", filterEntry.keyword, "--algo", "bm", "-g", reject_action);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, WEBURL_CHAIN_BLACK, "-p", "tcp", "--dport", "80", "-m", "string", "--url", filterEntry.keyword, "--algo", "bm", "-g", reject_action);
#endif
#endif
		rule_black_count++;
	}

#ifdef URL_ALLOWING_SUPPORT
	total = mib_chain_total(MIB_URL_ALLOW_FQDN_TBL);
	for (i=0; i<total; i++) {
		if (!mib_chain_get(MIB_URL_ALLOW_FQDN_TBL, i, (void *)&allowEntry))
			continue;

		if(allowEntry.fqdn[0] == '\0')
			continue;

#ifdef CONFIG_NETFILTER_XT_MATCH_WEBURL
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, WEBURL_CHAIN_WHITE, "-p", "tcp", "-m", "weburl", "--contains", allowEntry.fqdn, "--domain_only", "-g", accept_action);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 11, 1, (char *)FW_ADD, WEBURL_CHAIN_WHITE, "-p", "tcp", "-m", "weburl", "--contains", allowEntry.fqdn, "--domain_only", "-g", accept_action);
#endif
#else
		va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, WEBURL_CHAIN_WHITE, "-p", "tcp", "--dport", "80", "-m", "string", "--urlalw", allowEntry.fqdn, "--algo", "bm", "-g", accept_log_action);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, WEBURL_CHAIN_WHITE, "-p", "tcp", "--dport", "80", "-m", "string", "--urlalw", allowEntry.fqdn, "--algo", "bm", "-g", accept_log_action);
#endif
#endif

		rule_white_count++;
	}
#endif

	if(rule_white_count) //for whitelist default drop rule
	{
		checkUrlFilterMacList(&macList, " ", 1);
	}

	if(rule_white_count || rule_black_count)
	{
		config_url_filter_rule(1, portList, macList);
	}
	else
	{
		// clear action chain of iptables if no rule exists
		va_cmd(IPTABLES, 2, 1, "-F", reject_log_action);
		va_cmd(IPTABLES, 2, 1, "-X", reject_log_action);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 2, 1, "-F", reject_log_action);
		va_cmd(IP6TABLES, 2, 1, "-X", reject_log_action);
#endif

		va_cmd(IPTABLES, 2, 1, "-F", reject_action);
		va_cmd(IPTABLES, 2, 1, "-X", reject_action);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 2, 1, "-F", reject_action);
		va_cmd(IP6TABLES, 2, 1, "-X", reject_action);
#endif

		va_cmd(IPTABLES, 2, 1, "-F", accept_log_action);
		va_cmd(IPTABLES, 2, 1, "-X", accept_log_action);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 2, 1, "-F", accept_log_action);
		va_cmd(IP6TABLES, 2, 1, "-X", accept_log_action);
#endif

		va_cmd(IPTABLES, 2, 1, "-F", accept_action);
		va_cmd(IPTABLES, 2, 1, "-X", accept_action);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 2, 1, "-F", accept_action);
		va_cmd(IP6TABLES, 2, 1, "-X", accept_action);
#endif
	}

	freeUrlFilterPortList(portList);
	freeUrlFilterMacList(macList);

	// Kill all conntrack
	if(rule_white_count || rule_black_count)
		va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
}

int restart_urlblocking(void)
{
	unsigned char urlEnable = 0;

	va_cmd(IPTABLES, 2, 1, "-F", WEBURL_CHAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", WEBURL_CHAIN);
#endif

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
	filter_set_url(urlEnable);

	return 0;
}

#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
void free_urlfilter_list(URLFILTER_URLLIST_T *pList)
{
	URLFILTER_URLLIST_T *pURLList=NULL, *pTmp;
	pURLList = pList;
	while(pURLList)
	{
		pTmp = pURLList->next;
		free(pURLList);
		pURLList = pTmp;
	}
}
#endif

/* Get key and port from url */
int parse_ur_filter(char *url, char *key, int *port)
{
	char urlTmp[256]={0};
	char *pTmp;

	if((url == NULL) || (key == NULL) || (port == NULL) || (strlen(url) == 0))
		return 0;

	if (!strncmp(url, "http://", 7))
		strcpy(urlTmp, url+7);
	else
		strcpy(urlTmp, url);

	pTmp = strtok(urlTmp, ":");

	if (!strncmp(pTmp, "www.", 4))
	{
		if (pTmp[4])
			strcpy(key, pTmp + 4);
	}
	else
	{
		if (pTmp[0])
			strcpy(key, pTmp);
	}

	pTmp = strtok(NULL, ":");
	if(pTmp == NULL)
		*port = 0;
	else
		*port = atoi(pTmp);

	//printf("url key:%s, port=%d\n", key, *port);
	return 1;
}

#ifdef SUPPORT_URL_FILTER
#ifdef CONFIG_YUEME
static unsigned short hash16(const unsigned char* data_p, int length){
    unsigned char x;
    unsigned short crc = 0xFFFF;

    while (length--){
        x = crc >> 8 ^ *data_p++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
    }
    return crc;
}
#endif
#define URLBLOCK_PROCESS_LOCK "/var/run/URLBLK_PROCESS_LOCK"
int config_url_filter_rule_entry(MIB_CE_URL_FILTER_T *pentry, URLBLK_PORT_LIST **portList, URLBLK_MAC_LIST **macList)
{
	char *argv[64], *pUrl, *pUrl_tmp=NULL, *p, *pPort, commentStr[128]={0}, macStr[18]={0}, portStr[10]={0};
	int argidx, port, rule_count, len, config = 1, check_port;
	unsigned char mac[6];
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
	int isValid;
#endif

	rule_count = 0;
	len = strlen(pentry->url)+1;
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
	len += strlen(pentry->urllist)+2; //include end and ';'
#endif
	if(len == 0) return 0;
	pUrl_tmp = (char*)malloc(len);
	if(pUrl_tmp == NULL) return 0;

	*pUrl_tmp = '\0';
	strcpy(pUrl_tmp, pentry->url);
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
	p = pUrl_tmp+strlen(pUrl_tmp);
	*(p++) = ';';
	strcpy(p, pentry->urllist);
#endif
	pUrl = trim_white_space(pUrl_tmp);

	memset(mac, 0, sizeof(mac));
	memset(macStr, 0, sizeof(macStr));
	if(pentry->mac[0] != '\0' && convertMacFormat(pentry->mac, mac))
	{
		sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}
	else
		*macStr = ' ';

#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
	isValid = checkURLFilter(pentry);

	if(!isValid && pentry->rule_add) config = 0;
	else if(isValid && pentry->rule_add) config = 2;
	else if(!isValid) config = 0;
	else config = 1;

	if(pentry->mode != 0) //for special whitelist, no matter isValid or config, config MAC to check default rule
		checkUrlFilterMacList(macList, macStr, 1);
#endif

	{
#ifdef CONFIG_YUEME
		unsigned short hashKey = 0;
		hashKey ^= hash16(pentry->name, sizeof(pentry->name));
		hashKey ^= hash16(pentry->url, sizeof(pentry->url));
		hashKey ^= hash16(pentry->mac, sizeof(pentry->mac));
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
		hashKey ^= hash16(pentry->urllist, sizeof(pentry->urllist));
#endif
		sprintf(commentStr, "URLBLK_RULE_%s_%04x_FILTER", pentry->name, hashKey);
#else
		if(pentry->name[0] != '\0')
			sprintf(commentStr, "URLBLK_RULE_%s_FILTER", pentry->name);
#endif
	}

#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
	if(pentry->mode != 0)
		check_port = 1;
	else
#endif
	check_port = (config) ? 1 : 0;

	while(pUrl && (pUrl = strtok_r(pUrl, ";", &p)))
	{
		port = 0;
		if(!strncmp(pUrl, "http://", 7))
			pUrl += 7;
		pPort = strchr(pUrl, ':');
		if(pPort){
			if(!((port = atoi(pPort+1)) > 0 && port < 65535))
				port = 0;
			*pPort = '\0';
		}

		if(port != 0)
		{
			checkUrlFilterPortList(portList, port, check_port);
		}
		else
		{
			checkUrlFilterPortList(portList, 80, check_port);
			checkUrlFilterPortList(portList, 443, check_port);
		}

		pUrl = trim_white_space(pUrl);

		if(*pUrl == '\0') {
			pUrl = p;
			continue;
		}

		if(config) rule_count++;

#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
		if(config == 2) { //only check port, mac and rule count
			pUrl = p;
			continue;
		}
#endif
		argidx = 0;
		argv[argidx++] = NULL;
		if(config)
			argv[argidx++] = (char*)FW_INSERT;
		else
			argv[argidx++] = (char*)FW_DEL;
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
		if(pentry->mode == 0)
			argv[argidx++] = (char*)WEBURL_CHAIN_BLACK;
		else
			argv[argidx++] = (char*)WEBURL_CHAIN_WHITE;
#else
		argv[argidx++] = (char*)WEBURL_CHAIN_BLACK;
#endif
		argv[argidx++] = "-p";
		argv[argidx++] = "tcp";
		if(port > 0)
		{
			sprintf(portStr, "%d", port);
			argv[argidx++] = "--dport";
			argv[argidx++] = portStr;
		}
		if(macStr[0] != '\0' && macStr[0] != ' ')
		{
			argv[argidx++] = "-m";
			argv[argidx++] = "mac";
			argv[argidx++] = "--mac-source";
			argv[argidx++] = macStr;
		}
		if(commentStr[0] != '\0')
		{
			argv[argidx++] = "-m";
			argv[argidx++] = "comment";
			argv[argidx++] = "--comment";
			argv[argidx++] = commentStr;
		}
#ifdef CONFIG_NETFILTER_XT_MATCH_WEBURL
		argv[argidx++] = "-m";
		argv[argidx++] = "weburl";
		argv[argidx++] = "--contains";
		argv[argidx++] = pUrl;
		argv[argidx++] = "--domain_only";
#else
		argv[argidx++] = "-m";
		argv[argidx++] = "string";
		argv[argidx++] = "--url";
		argv[argidx++] = pUrl;
		argv[argidx++] = "--algo";
		argv[argidx++] = "-bm";
#endif
		argv[argidx++] = "-j";
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
		if(pentry->mode == 0)
			argv[argidx++] = "DROP";
		else
#endif
		argv[argidx++] = "RETURN";
		argv[argidx++] = NULL;

		do_cmd(IPTABLES, argv, 1);
#ifdef CONFIG_IPV6
		do_cmd(IP6TABLES, argv, 1);
#endif
		pUrl = p;
	}
	if(pUrl_tmp) free(pUrl_tmp);

#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
	if(rule_count <= 0 && pentry->mode != 0 && !isValid)
		rule_count = -255; //special for whitelistRule, if rule not in schedule, set rule_count to -255
#endif

	return rule_count;
}

#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC

URLFILTER_URLLIST_T * parse_urlfilter_urlist(char *urllist)
{
	URLFILTER_URLLIST_T *URLFilterPtr=NULL, *pList=NULL;
	char *str1, *str2, *token, *pTmp;
	char *saveptr1, *saveptr2;
	char urlTmp[256]={0};
	char buff[256]={0};
	int j;
#if defined(CONFIG_RTK_CTCAPD_FILTER_SUPPORT)
	char urlAddr[MAX_URLLIST_LEN+1]={0};

	//use urlAddr to avoid modifing urlfilter mib entry's urllist
	strncpy(urlAddr,urllist,sizeof(urlAddr)-1);

	for (j = 1, str1 = urlAddr; ; j++, str1 = NULL)
#else
	for (j = 1, str1 = urllist; ; j++, str1 = NULL)
#endif
	{
		token = strtok_r(str1, ";", &saveptr1);
		if (token == NULL)
			break;

		if (!strncmp(token, "http://", 7))
			strncpy(urlTmp, token+7, sizeof(urlTmp)-1);
#if defined(CONFIG_RTK_CTCAPD_FILTER_SUPPORT)
		else if(!strncmp(token, "https://", 8))
			strncpy(urlTmp, token+8, sizeof(urlTmp)-1);
#endif
		else
			strncpy(urlTmp, token, sizeof(urlTmp)-1);

		for (str2 = urlTmp; ; str2 = NULL)
		{
			pTmp = strtok_r(str2, ":", &saveptr2);
			if(pTmp == NULL)
				break;

			if (!strncmp(pTmp, "www.", 4))
			{
				if (pTmp[4])
					strncpy(buff, pTmp + 4, sizeof(buff)-1);
			}
			else
			{
				if (pTmp[0])
					strncpy(buff, pTmp, sizeof(buff)-1);
			}

			URLFilterPtr = (URLFILTER_URLLIST_Tp)malloc(sizeof(URLFILTER_URLLIST_T));
			memset(URLFilterPtr, 0, sizeof(URLFILTER_URLLIST_T));
			if(URLFilterPtr)
			{
				strncpy(URLFilterPtr->url, buff, sizeof(URLFilterPtr->url)-1);
				if(saveptr2 == NULL)
					URLFilterPtr->port = 80;
				else
					URLFilterPtr->port= atoi(saveptr2);

				if(pList == NULL)
				{
					pList = URLFilterPtr;
				}
				else
				{
					URLFILTER_URLLIST_T *tmp=pList;
					while(tmp->next)
						tmp=tmp->next;
					tmp->next=URLFilterPtr;
				}
			}
		}
	}

	return pList;
}

int getMinutes(unsigned char hour, unsigned char minute)
{
	return (hour*60+minute);
}

int Filter_time_check(int StartTime1, int EndTime1, int StartTime2, int EndTime2, int StartTime3, int EndTime3, unsigned int RepeatDay)
{
	time_t tm;
	struct tm the_tm;
	time(&tm);
	memcpy(&the_tm, localtime(&tm), sizeof(the_tm));
	int t_now = 0;

#if defined(CONFIG_RTK_CTCAPD_FILTER_SUPPORT)
	if((isTimeSynchronized() == 0) || RepeatDay == 0)
		return 0;

	if ((RepeatDay & (1 << (the_tm.tm_wday-1))) != 0 || ((the_tm.tm_wday==0) && (RepeatDay & 0x40)))
	{
		t_now = (the_tm.tm_hour * 60) + the_tm.tm_min;
		if( (StartTime1 == 0) && (EndTime1 == 0) && (StartTime2 == 0)
			&& (EndTime2 == 0) && (StartTime3 == 0) && (EndTime3 == 0)){
				return 0;
			}

		if ((t_now >= StartTime1) && (t_now <= EndTime1) || (t_now >= StartTime2) && (t_now <= EndTime2)
			|| (t_now >= StartTime3) && (t_now <= EndTime3)){
				return 1;
			}
	}
#else
	if(isTimeSynchronized() == 0)
		return 1;

	if ((RepeatDay == 0) || (RepeatDay & (1 << (the_tm.tm_wday-1))) != 0)
	{
		t_now = (the_tm.tm_hour * 60) + the_tm.tm_min;
		if( (StartTime1 == 0) && (EndTime1 == 0) && (StartTime2 == 0)
			&& (EndTime2 == 0) && (StartTime3 == 0) && (EndTime3 == 0))
			return 1;

		if ((t_now >= StartTime1) && (t_now <= EndTime1) || (t_now >= StartTime2) && (t_now <= EndTime2)
			|| (t_now >= StartTime3) && (t_now <= EndTime3))
			return 1;
	}
#endif

	return 0;
}

int getUrlFilterTime(MIB_CE_URL_FILTER_Tp pEntry, char *buff)
{
	if((pEntry == NULL) || (buff == NULL))
		return 0;

	if((pEntry->end_hr2 == 0) && (pEntry->end_min2 == 0))
	{
		sprintf(buff, "%02hhu:%02hhu-%02hhu:%02hhu", pEntry->start_hr1, pEntry->start_min1, pEntry->end_hr1, pEntry->end_min1);
	}
	else if((pEntry->end_hr3 == 0) && (pEntry->end_min3 == 0))
	{
		sprintf(buff, "%02hhu:%02hhu-%02hhu:%02hhu, %02hhu:%02hhu-%02hhu:%02hhu", pEntry->start_hr1, pEntry->start_min1, pEntry->end_hr1,
				pEntry->end_min1, pEntry->start_hr2, pEntry->start_min2, pEntry->end_hr2, pEntry->end_min2);
	}
	else
	{
		sprintf(buff, "%02hhu:%02hhu-%02hhu:%02hhu, %02hhu:%02hhu-%02hhu:%02hhu, %02hhu:%02hhu-%02hhu:%02hhu", pEntry->start_hr1,
				pEntry->start_min1, pEntry->end_hr1, pEntry->end_min1, pEntry->start_hr2, pEntry->start_min2, pEntry->end_hr2,
					 pEntry->end_min2, pEntry->start_hr3, pEntry->start_min3, pEntry->end_hr3, pEntry->end_min3);
	}

	return 1;
}

int isValidTimeFormat(unsigned char start_hr, unsigned char start_min, unsigned char end_hr, unsigned char end_min)
{
	int start_time=0, end_time=0;

	if((start_hr>23) || (start_min>59))
		return 0;

	if((end_hr>23) || (end_min>59))
		return 0;

	start_time = getMinutes(start_hr, start_min);
	end_time = getMinutes(end_hr, end_min);
	if(start_time>end_time)
		return 0;

	return 1;
}

int checkURLFilter(MIB_CE_URL_FILTER_T *url_filter)
{
	int timecheck=0;
	int StartTime1=0, EndTime1=0;
	int StartTime2=0, EndTime2=0;
	int StartTime3=0, EndTime3=0;

	if(url_filter == NULL)
		return 0;

	StartTime1 = getMinutes(url_filter->start_hr1, url_filter->start_min1);
	EndTime1 = getMinutes(url_filter->end_hr1, url_filter->end_min1);
	StartTime2 = getMinutes(url_filter->start_hr2, url_filter->start_min2);
	EndTime2 = getMinutes(url_filter->end_hr2, url_filter->end_min2);
	StartTime3 = getMinutes(url_filter->start_hr3, url_filter->start_min3);
	EndTime3 = getMinutes(url_filter->end_hr3, url_filter->end_min3);
	timecheck = Filter_time_check(StartTime1, EndTime1, StartTime2, EndTime2, StartTime3, EndTime3, url_filter->WeekDays);

	if(timecheck)
		return 1;

	return 0;
}

int IsVaildFilterTime(unsigned char startH1, unsigned char startMin1, unsigned char endH1, unsigned char endMin1,
						unsigned char startH2, unsigned char startMin2, unsigned char endH2, unsigned char endMin2)
{
	int starttime1=0, endtime1=0;
	int starttime2=0, endtime2=0;
	int maxvalue, minvalue;

	if(!isValidTimeFormat(startH1, startMin1, endH1, endMin1))
		return 0;

	if(!isValidTimeFormat(startH2, startMin2, endH2, endMin2))
		return 0;

	starttime1 = getMinutes(startH1, startMin1);
	endtime1 = getMinutes(endH1, endMin1);
	starttime2 = getMinutes(startH2, startMin2);
	endtime2 = getMinutes(endH2, endMin2);

	maxvalue=(starttime1>starttime2)?starttime1:starttime2;
	minvalue=(endtime1<endtime2)?endtime1:endtime2;
	if(maxvalue<= minvalue && (maxvalue != 0))
		return 0;

	return 1;
}

int check_url_filter_time_schedule(void)
{
	int i, total, rule_count, hasChange, updateMIB, fd, count;
	MIB_CE_URL_FILTER_T entry;
	URLBLK_PORT_LIST *portList = NULL;
	URLBLK_MAC_LIST *macList = NULL;
	unsigned char mac[6];
	char macStr[18]={0};

	fd = lock_file_by_flock(URLBLOCK_PROCESS_LOCK, 0);
	if(fd < 0){
		fprintf(stderr, "[%s] Cannot get lock (%s)\n", __FUNCTION__, URLBLOCK_PROCESS_LOCK);
		return -1;
	}

	rule_count = hasChange = 0;
	total = mib_chain_total(MIB_URL_FILTER_TBL);
	for (i=0; i<total; i++)
	{
		if (!mib_chain_get(MIB_URL_FILTER_TBL, i, (void *)&entry))
			continue;
		if(entry.Enable == 0)
			continue;

		updateMIB = 0;
		count = config_url_filter_rule_entry(&entry, &portList, &macList);

		if(count > 0 && entry.rule_add == 0)
		{
			hasChange++;
			entry.rule_add = 1;
			updateMIB = 1;
		}
		else if(count <= 0 && entry.rule_add != 0)
		{
			hasChange++;
			entry.rule_add = 0;
			updateMIB = 1;
		}

		if(updateMIB)
		{
			mib_chain_update(MIB_URL_FILTER_TBL, &entry, i);
		}

		if(count > 0)
			rule_count += count;
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
		if(count == -255)
			rule_count++;
#endif

	}

	if(hasChange)
	{
		if(rule_count)
			config_url_filter_rule(2, portList, macList);
		else
			config_url_filter_rule(0, portList, macList);
	}

	freeUrlFilterPortList(portList);
	freeUrlFilterMacList(macList);

	unlock_file_by_flock(fd);

	return 0;
}
#endif

void set_url_filter_config()
{
	int i, total, rule_count = 0, fd, count;
	MIB_CE_URL_FILTER_T entry;
	URLBLK_PORT_LIST *portList = NULL;
	URLBLK_MAC_LIST *macList = NULL;
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
	unsigned char mac[6];
	char macStr[18]={0};
#endif

	fd = lock_file_by_flock(URLBLOCK_PROCESS_LOCK, 1);
	if(fd < 0){
		fprintf(stderr, "[%s] Cannot get lock (%s)\n", __FUNCTION__, URLBLOCK_PROCESS_LOCK);
		return;
	}

	//Kevin, check does it need to enable/disable IP fastpath status
	UpdateIpFastpathStatus();

	config_url_filter_rule(0, NULL, NULL);

	total = mib_chain_total(MIB_URL_FILTER_TBL);
	for (i=0; i<total; i++)
	{
		if (!mib_chain_get(MIB_URL_FILTER_TBL, i, (void *)&entry))
			continue;
		if(entry.Enable == 0)
			continue;
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
		entry.rule_add = 0;
#endif

		count = config_url_filter_rule_entry(&entry, &portList, &macList);

#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
		if(count > 0 && entry.rule_add != 1)
		{
			entry.rule_add = 1;
			mib_chain_update(MIB_URL_FILTER_TBL, &entry, i);
		}
		else if(count <= 0 && entry.rule_add != 0)
		{
			entry.rule_add = 0;
			mib_chain_update(MIB_URL_FILTER_TBL, &entry, i);
		}
#endif

		if(count > 0)
			rule_count += count;
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
		if(count == -255)
			rule_count++;
#endif
	}

	if(rule_count)
	{
		config_url_filter_rule(1, portList, macList);
	}

	freeUrlFilterPortList(portList);
	freeUrlFilterMacList(macList);

	// Kill all conntrack
	if(rule_count)
		va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");

	unlock_file_by_flock(fd);
}

int rtk_url_get_iptables_drop_pkt_count(MIB_CE_URL_FILTER_T *pentry)
{
	char commentStr[128]={0}, buf[256];
	const char *chain;
	FILE *fp;
	int count, total_count;

	if(pentry == NULL || !pentry->Enable ) return 0;

#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
	if(!pentry->rule_add && pentry->mode == 1)
	{
		unsigned char mac[6];
		char macStr[18]={0};

		if(pentry->mac[0] != '\0' && convertMacFormat(pentry->mac, mac))
		{
			sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			total_count = 0;
			snprintf(buf, sizeof(buf), "%s list %s | grep '%s' | awk '{print $3}'", IPSET, URLBLK_MAC_FILTER, macStr);
			if((fp = popen(buf, "r")))
			{
				while(!feof(fp)) {
					if(fgets(buf, sizeof(buf), fp) != NULL){
						if(sscanf(buf, "%u", &count))
							total_count += count;
					}
				}
				pclose(fp);
			}
			return total_count;
		}
		else
			sprintf(commentStr, URLBLK_WHITE_DEF_FULE);
	}
	else
#endif
	{
#ifdef CONFIG_YUEME
		unsigned short hashKey = 0;
		hashKey ^= hash16(pentry->name, sizeof(pentry->name));
		hashKey ^= hash16(pentry->url, sizeof(pentry->url));
		hashKey ^= hash16(pentry->mac, sizeof(pentry->mac));
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
		hashKey ^= hash16(pentry->urllist, sizeof(pentry->urllist));
#endif
		sprintf(commentStr, "URLBLK_RULE_%s_%04x_FILTER", pentry->name, hashKey);
#else
		if(pentry->name[0] != '\0')
			sprintf(commentStr, "URLBLK_RULE_%s_FILTER", pentry->name);
#endif
	}

	if(commentStr[0] == '\0')
		return 0;

#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
	if(pentry->mode == 0)
		chain = WEBURL_CHAIN_BLACK;
	else
		chain = WEBURL_CHAIN_WHITE;
#else
	chain = WEBURL_CHAIN_BLACK;
#endif

	total_count = 0;
	snprintf(buf, sizeof(buf), "%s -nvL %s | grep '%s' | awk '{print $1}'", IPTABLES, chain, commentStr);
	if((fp = popen(buf, "r")))
	{
		while(!feof(fp)) {
			if(fgets(buf, sizeof(buf), fp) != NULL){
				if(sscanf(buf, "%u", &count))
					total_count += count;
			}
		}
		pclose(fp);
	}
#ifdef CONFIG_IPV6
	snprintf(buf, sizeof(buf), "%s -nvL %s | grep '%s' | awk '{print $1}'", IP6TABLES, chain, commentStr);
	if((fp = popen(buf, "r")))
	{
		while(!feof(fp)) {
			if(fgets(buf, sizeof(buf), fp) != NULL){
				if(sscanf(buf, "%u", &count))
					total_count += count;
			}
		}
		pclose(fp);
	}
#endif

	return total_count;
}
#endif

void set_url_filter()
{
	restart_urlfilter();
}

int restart_urlfilter(void)
{
#ifdef SUPPORT_URL_FILTER
	set_url_filter_config();
	return 0;
#else
	return restart_urlblocking();
#endif
}

#endif

#ifdef SUPPORT_URL_FILTER
void DeleteSplitcharFromURLFilter(char *macstring, char *macstring2)
{
	char *pch = NULL;
	char tmp[20]={0};
	char tmpmacstring[512]={0};
	char backmacstring[512]={0};
	unsigned char macAddr[MAC_ADDR_LEN];

	if((macstring == NULL) || (macstring2 == NULL) || (strlen(macstring) == 0))
		return ;

	strncpy(backmacstring, macstring, sizeof(backmacstring)-1);
	if(strstr(backmacstring, ","))
	{
		pch = strtok(backmacstring, ",");
		while (pch != NULL)
		{
			changeMacFormat(pch,':','-');
			changeStringToMac(macAddr, pch);
			snprintf(tmp, sizeof(tmp), "%02X%02X%02X%02X%02X%02X,",
				macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]);
			strcat(tmpmacstring, tmp);
			pch = strtok(NULL, ",");
		}

		strncpy(macstring2, tmpmacstring, strlen(tmpmacstring)-1);
	}
	else
	{
		changeMacFormat(backmacstring,':','-');
		changeStringToMac(macAddr, backmacstring);
		snprintf(macstring2, 512, "%02X%02X%02X%02X%02X%02X",
				macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]);
	}
}

void changeMacstringForURUFilter(char *macstring, char *macstring2)
{
	char *pch = NULL;
	char tmp[20]={0};
	char tmpmacstring[512]={0};
	char backmacstring[512]={0};
	unsigned char macAddr[MAC_ADDR_LEN];

	if((macstring == NULL) || (macstring2 == NULL))
		return ;

	strncpy(backmacstring, macstring, sizeof(backmacstring)-1);
	if(strstr(backmacstring, ","))
	{
		pch = strtok(backmacstring, ",");
		while (pch != NULL)
		{
			rtk_string_to_hex(pch, macAddr, 12);
			snprintf(tmp, sizeof(tmp), "%02x:%02x:%02x:%02x:%02x:%02x,",
				macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]);
			strcat(tmpmacstring, tmp);
			pch = strtok(NULL, ",");
		}

		strncpy(macstring2, tmpmacstring, strlen(tmpmacstring)-1);
	}
	else
	{
		rtk_string_to_hex(backmacstring, macAddr, 12);
		snprintf(macstring2, 512, "%02x:%02x:%02x:%02x:%02x:%02x",
		macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]);
	}
}
#endif


#ifdef SUPPORT_DNS_FILTER
int getDNSFilterTime(MIB_CE_DNS_FILTER_Tp pEntry, char *buff)
{
	if((pEntry == NULL) || (buff == NULL))
		return 0;

	if((pEntry->end_hr2 == 0) && (pEntry->end_min2 == 0))
	{
		sprintf(buff, "%02hhu:%02hhu-%02hhu:%02hhu", pEntry->start_hr1, pEntry->start_min1, pEntry->end_hr1, pEntry->end_min1);
	}
	else if((pEntry->end_hr3 == 0) && (pEntry->end_min3 == 0))
	{
		sprintf(buff, "%02hhu:%02hhu-%02hhu:%02hhu, %02hhu:%02hhu-%02hhu:%02hhu", pEntry->start_hr1, pEntry->start_min1, pEntry->end_hr1,
				pEntry->end_min1, pEntry->start_hr2, pEntry->start_min2, pEntry->end_hr2, pEntry->end_min2);
	}
	else
	{
		sprintf(buff, "%02hhu:%02hhu-%02hhu:%02hhu, %02hhu:%02hhu-%02hhu:%02hhu, %02hhu:%02hhu-%02hhu:%02hhu", pEntry->start_hr1,
				pEntry->start_min1, pEntry->end_hr1, pEntry->end_min1, pEntry->start_hr2, pEntry->start_min2, pEntry->end_hr2,
					 pEntry->end_min2, pEntry->start_hr3, pEntry->start_min3, pEntry->end_hr3, pEntry->end_min3);
	}

	return 1;
}

int UpdateDNSFilterBlocktime(char *name, char *url, int blocktime)
{
	int len=0, ret = 0;
	FILE *fp=NULL;
	FILE *fp2=NULL;
	char line[512]={0};
	char buf[512]={0};
	struct stat st;

	if(url == NULL || name == NULL)
		return -1;

	if(stat(DNSFILTERFILENAME, &st) && st.st_size == 0)
	{
		fp = fopen(DNSFILTERFILENAME, "w");
		if(fp)
		{
			fprintf(fp, "%s|%s|%d\n", name, url, blocktime);
			fclose(fp);
			return 0;
		}
	}

	fp = fopen(DNSFILTERFILENAME, "r");
	fp2 = fopen(TMPDNSFILTERFILENAME, "w");

	if(!fp || !fp2){
		if(fp) fclose(fp);
		if(fp2) fclose(fp2);
		return -1;
	}
	sprintf(buf, "%s|%s|", name, url);
	len = strlen(buf);
	while (!feof(fp))
	{
		fgets(line, sizeof(line)-1, fp);

		if(ret == 0 && strncmp(line, buf, len) == 0)
		{
			fprintf(fp2, "%s|%s|%d\n", name, url, blocktime);
			ret = 1;
		}
		else
		{
			fputs(line, fp2);
		}
	}

	if(ret == 0)
	{
		fprintf(fp2, "%s|%s|%d\n", name, url, blocktime);
	}

	fclose(fp);
	fclose(fp2);

	unlink(DNSFILTERFILENAME);
	rename(TMPDNSFILTERFILENAME, DNSFILTERFILENAME);

	return 1;
}

int getDNSFilterBlockedTimes(char *name, char *url)
{
	int len, ret = 0;
	FILE *fp;
	char line[MAX_DOMAINLIST_LEN+512]={0};
	char buf[MAX_DOMAINLIST_LEN+512]={0}, *tmp;
	struct stat st;

	if(name == NULL || url == NULL)
		return 0;

	if(stat(DNSFILTERFILENAME, &st) && st.st_size == 0)
	{
		return 0;
	}

	fp = fopen(DNSFILTERFILENAME, "r");
	if (fp == NULL)
	{
		printf("ERROR: error to file open %s.\n", DNSFILTERFILENAME);
		return 0;
	}
	else
	{
		sprintf(buf, "%s|%s|", name, url);
		len = strlen(buf);
		while (!feof(fp))
		{
			fgets(line, sizeof(line)-1, fp);

			if(strncmp(line, buf, len) == 0)
			{
				tmp = strrchr(line, '|');
				if(tmp) ret = atoi(tmp+1);
				break;
			}
		}
	}
	fclose(fp);

	return (ret<0)?0:ret;
}
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
void patch_for_office365(void)
{
	/* wpad.domain.name should be forbidden by router */
	char blkDomain[MAX_DOMAIN_LENGTH]="wpad.domain.name";
#ifdef CONFIG_NETFILTER_XT_MATCH_DNS
	va_cmd(IPTABLES, 13, 1, (char *)FW_ADD, "domainblk", "-p", "udp", "--dport", "53", "-m", "dns", "--qname", blkDomain, "--rmatch", "-j", (char *)FW_DROP);
	#ifdef CONFIG_IPV6
	// ipv6 filter
	// ip6tables -A FORWARD -p udp --dport 53 -m string --domain yahoo.com --algo bm -j DROP
	va_cmd(IP6TABLES, 13, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-p", "udp", "--dport", "53", "-m", "dns", "--qname", blkDomain, "--rmatch", "-j", (char *)FW_DROP);
	va_cmd(IP6TABLES, 13, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-p", "udp", "--dport", "53", "-m", "dns", "--qname", blkDomain, "--rmatch", "-j", (char *)FW_DROP);
	#endif

#else
	unsigned char *needle_tmp, *str;
	char len[MAX_DOMAIN_GROUP];
	unsigned char seg[MAX_DOMAIN_GROUP][MAX_DOMAIN_SUB_STRING];
	unsigned char cmpStr[MAX_DOMAIN_LENGTH]="\0";
	int j, k;

	needle_tmp = blkDomain;

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
	}
    //printf("cmpStr=%s len=%d\n", cmpStr, strlen(cmpStr));
	//printf("\n");

	// iptables -A domainblk -p udp --dport 53 -m string --domain yahoo.com --algo bm -j DROP

	va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, "domainblk", "-p", "udp", "--dport", "53", "-m", "string", "--domain", cmpStr, "--algo", "bm", "-j", (char *)FW_DROP);
	#ifdef CONFIG_IPV6
	// ipv6 filter
	// ip6tables -A FORWARD -p udp --dport 53 -m string --domain yahoo.com --algo bm -j DROP
	va_cmd(IP6TABLES, 14, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-p", "udp", "--dport", "53", "-m", "string", "--domain", cmpStr, "--algo", "bm", "-j", (char *)FW_DROP);
	va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-p", "udp", "--dport", "53", "-m", "string", "--domain", cmpStr, "--algo", "bm", "-j", (char *)FW_DROP);
	#endif
#endif
}

void rtk_set_mark_for_domainBLK(int enable)
{
	char skbmark_buf[64]={0};

	va_cmd(IPTABLES, 4, 1, "-t", "mangle" ,"-F", (char *)MARK_FOR_DOMAIN);

	if(enable){
#ifdef CONFIG_RTK_SKB_MARK2
		snprintf(skbmark_buf, sizeof(skbmark_buf), "0x%x", (1<<SOCK_MARK2_FORWARD_BY_PS_START));
		//iptables -t mangle -A mark_for_dns_filter -p udp --sport 53 -j MARK2 --or-mark 0x1
		va_cmd(IPTABLES, 12, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, (char *)MARK_FOR_DOMAIN,
			"-p", (char *)ARG_UDP, (char *)FW_SPORT, (char *)PORT_DNS, "-j", "MARK2","--or-mark",skbmark_buf);
#else
		//SKB_MARK1
		snprintf(skbmark_buf, sizeof(skbmark_buf), "0x%x", (1<<SOCK_MARK_METER_INDEX_END));
		//iptables -t mangle -A mark_for_dns_filter -p udp --sport 53 -j MARK --or-mark 0x80000000
		va_cmd(IPTABLES, 12, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, (char *)MARK_FOR_DOMAIN,
			"-p", (char *)ARG_UDP, (char *)FW_SPORT, (char *)PORT_DNS, "-j", "MARK","--or-mark",skbmark_buf);
#endif
	}

}

// Added by Mason Yu for Domain Blocking
void filter_set_domain(int enable)
{
	int i, total;
	MIB_CE_DOMAIN_BLOCKING_T Entry;
	unsigned char sdest[MAX_DOMAIN_LENGTH];
	int j, k;
	unsigned char *needle_tmp, *str;
	char len[MAX_DOMAIN_GROUP];
	unsigned char seg[MAX_DOMAIN_GROUP][MAX_DOMAIN_SUB_STRING];
	unsigned char cmpStr[MAX_DOMAIN_LENGTH]="\0";

	//Kevin, check does it need to enable/disable IP fastpath status
	UpdateIpFastpathStatus();

	//use fc mark to trap udp 53 to ps
	rtk_set_mark_for_domainBLK(enable);

	// check if Domain Blocking Capability is enabled ?
	if (!enable) {
		va_cmd(IPTABLES, 2, 1, "-F", "domainblk");
		#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 2, 1, "-F", "domainblk");
		#endif
	}
	else {
		// Add policy to domainblk chain
		total = mib_chain_total(MIB_DOMAIN_BLOCKING_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_DOMAIN_BLOCKING_TBL, i, (void *)&Entry))
			{
				return;
			}

			// Mason Yu
			// calculate domain sub string lengh and form the compare sub string.
			// Foe example, If the domain is aa.bbb.cccc, the compare sub string 1 is 0x02 0x61 0x61.
			// The compare sub string 2 is 0x03 0x62 0x62 0x62. The compare sub string 3 is 0x04 0x63 0x63 0x63 0x63.
			#ifdef CONFIG_NETFILTER_XT_MATCH_DNS
			va_cmd(IPTABLES, 13, 1, (char *)FW_ADD, "domainblk", "-p", "udp", "--dport", "53", "-m", "dns", "--qname", Entry.domain, "--rmatch", "-j", (char *)FW_DROP);
			#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 13, 1, (char *)FW_ADD, "domainblk", "-p", "udp", "--dport", "53", "-m", "dns", "--qname", Entry.domain, "--rmatch", "-j", (char *)FW_DROP);
			#endif
			va_cmd(IPTABLES, 13, 1, (char *)FW_ADD, "domainblk", "-p", "tcp", "--dport", "53", "-m", "dns", "--qname", Entry.domain, "--rmatch", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 13, 1, (char *)FW_ADD, "domainblk", "-p", "tcp", "--dport", "53", "-m", "dns", "--qname", Entry.domain, "--rmatch", "-j", (char *)FW_DROP);
#endif
			#else
			needle_tmp = Entry.domain;

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

		 	// iptables -A domainblk -p udp --dport 53 -m string --domain yahoo.com -j DROP
			va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, "domainblk", "-p", "udp", "--dport", "53", "-m", "string", "--domain", cmpStr, "--algo", "bm", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, "domainblk", "-p", "udp", "--dport", "53", "-m", "string", "--domain", cmpStr, "--algo", "bm", "-j", (char *)FW_DROP);
#endif
			va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, "domainblk", "-p", "tcp", "--dport", "53", "-m", "string", "--domain", cmpStr, "--algo", "bm", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, "domainblk", "-p", "tcp", "--dport", "53", "-m", "string", "--domain", cmpStr, "--algo", "bm", "-j", (char *)FW_DROP);
#endif
			#endif
			cmpStr[0] = '\0';
		}
	}
}


void filter_set_domain_default(void)
{

	char blkDomain[MAX_DOMAIN_LENGTH]="example.com";
#ifdef CONFIG_NETFILTER_XT_MATCH_DNS
		va_cmd(IPTABLES, 13, 1, (char *)FW_ADD, "domainblk", "-p", "udp", "--dport", "53", "-m", "dns", "--qname", blkDomain, "--rmatch", "-j", (char *)FW_DROP);
	#ifdef CONFIG_IPV6
		// ipv6 filter
		// ip6tables -A FORWARD -p udp --dport 53 -m string --domain yahoo.com --algo bm -j DROP
		va_cmd(IP6TABLES, 13, 1, (char *)FW_DEL, "domainblk", "-p", "udp", "--dport", "53", "-m", "dns", "--qname", blkDomain, "--rmatch", "-j", (char *)FW_DROP);
		va_cmd(IP6TABLES, 13, 1, (char *)FW_ADD, "domainblk", "-p", "udp", "--dport", "53", "-m", "dns", "--qname", blkDomain, "--rmatch", "-j", (char *)FW_DROP);
	#endif
#endif
#ifdef CONFIG_CU
	// fix nessus 35450 DNS Server Spoofed Request Amplification DDoS
	// Add policy to domainblk chain for root NS query
	// iptables -A domainblk -p udp --dport 53 -m string --hex-string "|001000010000000000000000020001|" --algo bm -j DROP
	va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, "domainblk", "-p", "udp", "--dport", "53", "-m", "string", "--hex-string", "|001000010000000000000000020001|", "--algo", "bm", "-j", (char *)FW_DROP);
	// ip6tables -A domainblk -p udp --dport 53 -m string --hex-string "|001000010000000000000000020001|" --algo bm -j DROP
	va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, "domainblk", "-p", "udp", "--dport", "53", "-m", "string", "--hex-string", "|001000010000000000000000020001|", "--algo", "bm", "-j", (char *)FW_DROP);
#endif
}

int restart_domainBLK(void)
{
	unsigned char domainEnable;

	va_cmd(IPTABLES, 2, 1, "-F", "domainblk");
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", "domainblk");
#endif
	mib_get_s(MIB_DOMAINBLK_CAPABILITY, (void *)&domainEnable, sizeof(domainEnable));
	if (domainEnable == 1)  // domain blocking Capability enabled
		filter_set_domain(1);
	else
		filter_set_domain(0);
	return 0;
}
#endif

#ifdef CONFIG_ANDLINK_SUPPORT
void filter_set_andlink(int enable)
{
	// check if andlink is enabled
	if (!enable)
		va_cmd(IPTABLES, 2, 1, "-F", "andlink");
	else {
		// Add policy to andlink chain
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, "andlink", "-p", "udp", "!", "-i", BRIF, "--sport", "5683", "-j", (char *)FW_ACCEPT);
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, "andlink", "-p", "udp", "!", "-i", BRIF, "--dport", "5683", "-j", (char *)FW_ACCEPT);
	}
}

int filter_restart_andlink(void)
{
	unsigned char andlinkEnable;

	va_cmd(IPTABLES, 2, 1, "-F", "andlink");
	mib_get(MIB_RTL_LINK_ENABLE, (void *)&andlinkEnable);
	if (andlinkEnable == 1)
		filter_set_andlink(1);
	else
		filter_set_andlink(0);
	return 0;
}
#endif


#if defined(DOS_SUPPORT)||defined(_PRMT_X_TRUE_FIREWALL_) || defined(FIREWALL_SECURITY)
#if defined(CONFIG_E8B)||defined(_PRMT_X_TRUE_FIREWALL_)  || defined(FIREWALL_SECURITY)
int addLowGradeRule(VC_STATE_Pt pEntry, int enable)
{
	if (enable) {     //high to low
		//iptables -I INPUT 1 -i vc0 -p udp --dport 69 -j ACCEPT
		//va_cmd(IPTABLES, 11, 1, (char *)FW_INSERT, "fwinput", "1", (char *)ARG_I,
		//	pEntry->ifName, "-p", "udp", "--dport", "69", "-j", (char *)FW_ACCEPT);
		//iptables -I INPUT 2 -i vc0 -p tcp --dport 23 -j ACCEPT
		//va_cmd(IPTABLES, 11, 1, (char *)FW_INSERT, "fwinput", "2", (char *)ARG_I,
		//	pEntry->ifName, "-p", "tcp", "--dport", "23", "-j", (char *)FW_ACCEPT);
		//iptables -I INPUT 3 -i vc0 -p icmp --icmp-type 8 -j ACCEPT
		va_cmd(IPTABLES,8, 1, (char *)FW_DEL, FW_INPUT,"-p", "icmp", "--icmp-type", "8", "-j", (char *)FW_DROP);
		//iptables -I INPUT 4 -i vc0 -p tcp --dport 80 -j ACCEPT
		//va_cmd(IPTABLES, 11, 1, (char *)FW_INSERT, "fwinput", "4", (char *)ARG_I,
		//	pEntry->ifName, "-p", "tcp", "--dport", "80", "-j", (char *)FW_ACCEPT);
	} else {     //low to high
		//iptables -D INPUT -i vc0 -p udp --dport 69 -j ACCEPT
		//va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, "fwinput", (char *)ARG_I,
		//	pEntry->ifName, "-p", "udp", "--dport", "69", "-j", (char *)FW_ACCEPT);
		//iptables -D INPUT -i vc0 -p tcp --dport 23 -j ACCEPT
		//va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, "fwinput", (char *)ARG_I,
		//	pEntry->ifName, "-p", "tcp", "--dport", "23", "-j", (char *)FW_ACCEPT);
		//iptables -D INPUT -i vc0 -p icmp --icmp-type 8 -j ACCEPT
		va_cmd(IPTABLES,8, 1, (char *)FW_DEL, FW_INPUT,"-p", "icmp", "--icmp-type", "8", "-j", (char *)FW_DROP);
		va_cmd(IPTABLES, 8, 1, (char *)FW_INSERT, FW_INPUT,  "-p", "icmp", "--icmp-type", "8", "-j", (char *)FW_DROP);
		//iptables -D INPUT -i vc0 -p tcp --dport 80 -j ACCEPT
		//va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, "fwinput", (char *)ARG_I,
		//	pEntry->ifName, "-p", "tcp", "--dport", "80", "-j", (char *)FW_ACCEPT);
	}

	return 1;
}

//use fw_state_t to reserve the set state of firewall grade
FW_GRADE_INIT_St fw_state_t;

int changeFwGrade(unsigned char enable, int currGrade)
{
	int low_enable;
	int mibCnt;
	int totalNum;
	MIB_CE_ATM_VC_T entry;
	VC_STATE_T vcState;

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	if(currGrade == FW_MIDDLE)
		currGrade = FW_HIGH;
#endif

	if(enable == 0)
	{
		//Prevent any rules to be added
		currGrade = FW_LOW;
	}

	/* disable tcp_stateful_tracking is for testing, we should keep  tcp_stateful_tracking is in enable state
	  * to avoid URL black/white list function will not work
	*/
	if (fw_state_t.preFwGrade == (char)FW_HIGH) {
		if (currGrade != (char)FW_HIGH)//HIGH->LOW
			low_enable = 1;
		else
			return 0;
	}
	else if (currGrade == (char)FW_HIGH) {
		if (fw_state_t.preFwGrade != FW_HIGH)//LOW->High
			low_enable = 0;
		else
			return 0;
	} else//don't change
		return 0;

	//Prevent any rules to be added
	if(enable == 0)
		low_enable = 0;

	totalNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(mibCnt=0; mibCnt<totalNum; mibCnt++)
	{
		memset(&vcState, 0, sizeof(VC_STATE_T));

		if (!mib_chain_get(MIB_ATM_VC_TBL, mibCnt, (void *)&entry))
		{
			printf("get MIB chain error\n");
			return -1;
		}
		if ((entry.cmode == CHANNEL_MODE_BRIDGE) || !entry.enable)
			continue;


		ifGetName(entry.ifIndex, vcState.ifName, sizeof(vcState.ifName));

		addLowGradeRule(&vcState, low_enable);
	}

	fw_state_t.preFwGrade = currGrade;

	return 1;
}

int startFirewall(void)
{
	unsigned char filterLevel = FW_LOW;
	unsigned char enable = 0;

	mib_get(MIB_FW_ENABLE, &enable);

	if (!mib_get(MIB_FW_GRADE, (void *)&filterLevel))
	{
		printf("get fw grade fail\n");
		return 0;
	}
	changeFwGrade(enable, filterLevel);

	return 1;
}
#endif //defined(CONFIG_E8B)||defined(_PRMT_X_TRUE_FIREWALL_)

#ifdef DOS_SUPPORT
#ifdef _PRMT_X_CMCC_SECURITY_
struct ParentalCtrl_info
{
	unsigned char MACAddress[MAC_ADDR_LEN];
	unsigned char UrlFilterPolicy;
	unsigned char UrlFilterRight;
	unsigned char DurationPolicy;
	unsigned char DurationRight;
	unsigned char StartTime_hr;
	unsigned char StartTime_min;
	unsigned char EndTime_hr;
	unsigned char EndTime_min;
	unsigned int RepeatDay;
	char UrlAddress[64];
	unsigned char cur_state;
	struct ParentalCtrl_info *next;
};
struct ParentalCtrl_info *ParentalCtrl_list = NULL;
int is_rule_ready = 0;
enum {
	PARENTALCTRL_MODE_MAC_FILTER_NONE = 0,
	PARENTALCTRL_MODE_MAC_FILTER_ALL,
	PARENTALCTRL_MODE_URL_BLACK,
	PARENTALCTRL_MODE_URL_WHITE
};

int get_Templates_entry_by_inst_num(unsigned int num, MIB_PARENTALCTRL_TEMPLATES_Tp pEntry, int *idx)
{
	int i = 0, total = mib_chain_total(MIB_PARENTALCTRL_TEMPLATES_TBL);

	if (pEntry == NULL || idx == NULL || num == 0)
		return -1;

	*idx = -1;
	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_PARENTALCTRL_TEMPLATES_TBL, i, pEntry) == 0)
			continue;

		if (pEntry->inst_num == num)
		{
			*idx = i;
			return 0;
		}
	}
	return -1;
}

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
/* mode: 0 -> remove mac
         1 -> add mac
   return value: is_modify
*/
int ParentalCtrl_MAC_Filter_set(int mode, unsigned char *MACAddress)
{
	int i = 0, cnt = 0, is_modify = 0;
	char mac_str[18] = {0};
	unsigned char macCap = 0; // 0-off, 1-black list, 2-white list
	struct routemac_entry mac_filter_entry;

	fprintf(stderr, "mode = %d, MACAddress = %02x:%02x:%02x:%02x:%02x:%02x\n", mode, MACAddress[0], MACAddress[1], MACAddress[2], MACAddress[3], MACAddress[4], MACAddress[5]);

	if (mode == 0)
	{
		mib_get(MIB_MAC_FILTER_SRC_ENABLE, (void *)&macCap);
		if (macCap != 0)
		{
			fprintf(stderr, "MIB_MAC_FILTER_SRC_ENABLE set to 0\n");
			macCap = 0;
			mib_set(MIB_MAC_FILTER_SRC_ENABLE, (void *)&macCap);
			is_modify = 1;
		}

		snprintf(mac_str, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
										MACAddress[0], MACAddress[1], MACAddress[2],
										MACAddress[3], MACAddress[4], MACAddress[5]);

		cnt = mib_chain_total(MIB_MAC_FILTER_ROUTER_TBL);
		for (i = 0; i < cnt; i++)
		{
			memset(&mac_filter_entry, 0, sizeof(struct routemac_entry));
			if (!mib_chain_get(MIB_MAC_FILTER_ROUTER_TBL, i, (void *)&mac_filter_entry))
				continue;

			if (strcmp(mac_filter_entry.mac, mac_str) == 0)
			{
				fprintf(stderr, "MIB_MAC_FILTER_ROUTER_TBL delete %d\n", i);
				mib_chain_delete(MIB_MAC_FILTER_ROUTER_TBL, i);
				is_modify = 1;
				break;
			}
		}
	}
	else if (mode == 1)
	{
		mib_get(MIB_MAC_FILTER_SRC_ENABLE, (void *)&macCap);
		if (macCap != 1)
		{
			fprintf(stderr, "MIB_MAC_FILTER_SRC_ENABLE set to 1\n");
			macCap = 1;
			mib_set(MIB_MAC_FILTER_SRC_ENABLE, (void *)&macCap);
			is_modify = 1;
		}

		snprintf(mac_str, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
										MACAddress[0], MACAddress[1], MACAddress[2],
										MACAddress[3], MACAddress[4], MACAddress[5]);

		cnt = mib_chain_total(MIB_MAC_FILTER_ROUTER_TBL);
		for (i = 0; i < cnt; i++)
		{
			memset(&mac_filter_entry, 0, sizeof(struct routemac_entry));
			if (!mib_chain_get(MIB_MAC_FILTER_ROUTER_TBL, i, (void *)&mac_filter_entry))
				continue;

			if (strcmp(mac_filter_entry.mac, mac_str) == 0)
			{

				if (mac_filter_entry.mode != 0)
				{
					fprintf(stderr, "MIB_MAC_FILTER_ROUTER_TBL update %d\n", i);
					mac_filter_entry.mode = 0;
					mib_chain_update(MIB_MAC_FILTER_ROUTER_TBL, (void *)&mac_filter_entry, i);
					is_modify = 1;
				}
				break;
			}
		}

		if (i >= cnt)
		{
			fprintf(stderr, "MIB_MAC_FILTER_ROUTER_TBL add\n");
			memset(&mac_filter_entry, 0, sizeof(struct routemac_entry));
			strcpy(mac_filter_entry.mac, mac_str);
			strcpy(mac_filter_entry.name, mac_str);
			mac_filter_entry.mode = 0;
			mib_chain_add(MIB_MAC_FILTER_ROUTER_TBL, (void *)&mac_filter_entry);
			is_modify = 1;
		}
	}

	fprintf(stderr, "ParentalCtrl_MAC_Filter_set is_modify = %d\n", is_modify);
	return is_modify;
}
#elif defined(CONFIG_CU_BASEON_YUEME) && defined(COM_CUC_IGD1_ParentalControlConfig)
/* mode: 0 -> remove mac
         1 -> add mac
   return value: is_modify
*/
int ParentalCtrl_MAC_Filter_set(int mode, unsigned char *MACAddress)
{
	int i = 0, cnt = 0, is_modify = 1;
	char mac_str[18] = {0};
	unsigned char macCap = 0; // 0-off, 1-black list, 2-white list
	MIB_PARENTALCTRL_MAC_T mac_filter_entry;

	fprintf(stderr, "mode = %d, MACAddress = %02x:%02x:%02x:%02x:%02x:%02x\n", mode, MACAddress[0], MACAddress[1], MACAddress[2], MACAddress[3], MACAddress[4], MACAddress[5]);

	if (mode == 0)
	{
		snprintf(mac_str, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
										MACAddress[0], MACAddress[1], MACAddress[2],
										MACAddress[3], MACAddress[4], MACAddress[5]);

		cnt = mib_chain_total(MIB_PARENTALCTRL_MAC_TBL);
		for (i = 0; i < cnt; i++)
		{
			memset(&mac_filter_entry, 0, sizeof(MIB_PARENTALCTRL_MAC_T));
			if (!mib_chain_get(MIB_PARENTALCTRL_MAC_TBL, i, (void *)&mac_filter_entry))
				continue;

			if (memcmp(mac_filter_entry.MACAddress, MACAddress,MAC_ADDR_LEN) == 0)
			{
				is_modify = 0; // if PARENTALCTRL_MAC_TBL is empty,the fw rules will not be cleaned
				fprintf(stderr, "[PartCtr]Remove %s from MacBlkList\n", mac_str);
				if(mac_filter_entry.fwexist)
					is_modify = 1;
				mac_filter_entry.fwexist = 0;
				mib_chain_update(MIB_PARENTALCTRL_MAC_TBL, (void *)&mac_filter_entry, i);
				break;
			}
		}
	}
	else if (mode == 1)
	{
		snprintf(mac_str, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
										MACAddress[0], MACAddress[1], MACAddress[2],
										MACAddress[3], MACAddress[4], MACAddress[5]);

		cnt = mib_chain_total(MIB_PARENTALCTRL_MAC_TBL);
		for (i = 0; i < cnt; i++)
		{
			memset(&mac_filter_entry, 0, sizeof(MIB_PARENTALCTRL_MAC_T));
			if (!mib_chain_get(MIB_PARENTALCTRL_MAC_TBL, i, (void *)&mac_filter_entry))
				continue;

			if (memcmp(mac_filter_entry.MACAddress, MACAddress,MAC_ADDR_LEN) == 0)
			{
				is_modify = 0;
				fprintf(stderr, "[PartCtr]Add %s to MacBlkList\n", mac_str);
				if(mac_filter_entry.fwexist == 0)
					is_modify = 1;
				mac_filter_entry.fwexist = 1;
				mib_chain_update(MIB_PARENTALCTRL_MAC_TBL, (void *)&mac_filter_entry, i);
				break;
			}
		}
	}

	fprintf(stderr, "ParentalCtrl_MAC_Filter_set is_modify = %d\n", is_modify);
	return is_modify;
}
#endif

void ParentalCtrl_chain_setup(int action, unsigned char *MACAddress)
{
	char chain_name[32] = {0}, mac_str[18] = {0};

	snprintf(chain_name, sizeof(chain_name), "%s_%02x%02x%02x%02x%02x%02x",
				PARENTALCTRL_CHAIN,
				MACAddress[0], MACAddress[1], MACAddress[2],
				MACAddress[3], MACAddress[4], MACAddress[5]);

	snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
				MACAddress[0], MACAddress[1], MACAddress[2],
				MACAddress[3], MACAddress[4], MACAddress[5]);

	va_cmd(IPTABLES, 2, 1, "-F", chain_name);
	va_cmd(IPTABLES, 18, 1, (char *)FW_DEL, (char *)PARENTALCTRL_CHAIN, "-p", "tcp",
			"--tcp-flags", "SYN,FIN,RST", "NONE", "-m", "length", "!", "--length", "0:84",
			"-m", "mac", "--mac-source", mac_str, "-j", chain_name);
	va_cmd(IPTABLES, 2, 1, "-X", chain_name);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", chain_name);
	va_cmd(IP6TABLES, 18, 1, (char *)FW_DEL, (char *)PARENTALCTRL_CHAIN, "-p", "tcp",
			"--tcp-flags", "SYN,FIN,RST", "NONE", "-m", "length", "!", "--length", "0:104",
			"-m", "mac", "--mac-source", mac_str, "-j", chain_name);
	va_cmd(IP6TABLES, 2, 1, "-X", chain_name);
#endif


	if (action)
	{
		va_cmd(IPTABLES, 2, 1, "-N", chain_name);
		va_cmd(IPTABLES, 18, 1, (char *)FW_ADD, (char *)PARENTALCTRL_CHAIN, "-p", "tcp",
				"--tcp-flags", "SYN,FIN,RST", "NONE", "-m", "length", "!", "--length", "0:84",
				"-m", "mac", "--mac-source", mac_str, "-j", chain_name);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 2, 1, "-N", chain_name);
		va_cmd(IP6TABLES, 18, 1, (char *)FW_ADD, (char *)PARENTALCTRL_CHAIN, "-p", "tcp",
				"--tcp-flags", "SYN,FIN,RST", "NONE", "-m", "length", "!", "--length", "0:104",
				"-m", "mac", "--mac-source", mac_str, "-j", chain_name);
#endif
	}

	return;
}

void ParentalCtrl_MAC_Policy_Set(unsigned char *MACAddress, int mode)
{
	char chain_name[32] = {0};
	struct ParentalCtrl_info *node = NULL;
#ifdef CONFIG_RTK_SKB_MARK2
	char cmd[1024] = {0};
	unsigned long long mask, mark;
	mask = SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_BIT_MASK;
	mark = SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_BIT_MASK;
#endif

	snprintf(chain_name, sizeof(chain_name), "%s_%02x%02x%02x%02x%02x%02x",
			PARENTALCTRL_CHAIN,
			MACAddress[0], MACAddress[1], MACAddress[2],
			MACAddress[3], MACAddress[4], MACAddress[5]);

	va_cmd(IPTABLES, 2, 1, "-F", chain_name);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", chain_name);
#endif

	if (mode > PARENTALCTRL_MODE_MAC_FILTER_NONE)
	{
		if (mode == PARENTALCTRL_MODE_MAC_FILTER_ALL)
		{
#ifdef CONFIG_RTK_SKB_MARK2
			snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m multiport --dports 80,443 -j MARK2 --set-mark2 0x0/0x%llx",
										IPTABLES, chain_name, mask);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef CONFIG_IPV6
			snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m multiport --dports 80,443 -j MARK2 --set-mark2 0x0/0x%llx",
										IP6TABLES, chain_name, mask);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
#endif
			va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, chain_name, "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, chain_name, "-j", (char *)FW_DROP);
#endif
		}
		else if (mode == PARENTALCTRL_MODE_URL_BLACK)
		{
			if (ParentalCtrl_list)
			{
				node = ParentalCtrl_list;
				while (node != NULL)
				{
					if (memcmp(node->MACAddress, MACAddress, MAC_ADDR_LEN) == 0 && strlen(node->UrlAddress) > 0)
					{
#ifdef CONFIG_NETFILTER_XT_MATCH_WEBURL
						va_cmd(IPTABLES, 13, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "80", "-m", "weburl", "--contains", node->UrlAddress, "--domain_only", "-j", (char *)FW_DROP);
						va_cmd(IPTABLES, 13, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "443", "-m", "weburl", "--contains", node->UrlAddress, "--domain_only", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
						va_cmd(IP6TABLES, 13, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "80", "-m", "weburl", "--contains", node->UrlAddress, "--domain_only", "-j", (char *)FW_DROP);
						va_cmd(IP6TABLES, 13, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "443", "-m", "weburl", "--contains", node->UrlAddress, "--domain_only", "-j", (char *)FW_DROP);
#endif
#else
						va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "80", "-m", "string", "--url", node->UrlAddress, "--algo", "bm", "-j", (char *)FW_DROP);
						va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "443", "-m", "string", "--url", node->UrlAddress, "--algo", "bm", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
						va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "80", "-m", "string", "--url", node->UrlAddress, "--algo", "bm", "-j", (char *)FW_DROP);
						va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "443", "-m", "string", "--url", node->UrlAddress, "--algo", "bm", "-j", (char *)FW_DROP);
#endif
#endif
					}

					node = node->next;
				}
			}

#ifdef CONFIG_RTK_SKB_MARK2
			snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m multiport --dports 80,443 -j MARK2 --set-mark2 0x%llx/0x%llx",
										IPTABLES, chain_name, mark, mask);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef CONFIG_IPV6
			snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m multiport --dports 80,443 -j MARK2 --set-mark2 0x%llx/0x%llx",
										IP6TABLES, chain_name, mark, mask);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
#endif
		}
		else if (mode == PARENTALCTRL_MODE_URL_WHITE)
		{
			if (ParentalCtrl_list)
			{
#ifdef CONFIG_RTK_SKB_MARK2
				//tp add match 0x4000 return, for dpi will trap to ps 16 times, should add this for no drop.
				snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m mark2 --mark2 0x%llx/0x%llx -j RETURN",
											IPTABLES, chain_name, mark, mask);
				va_cmd("/bin/sh", 2, 1, "-c", cmd);

				snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m multiport --dports 80,443 -j MARK2 --set-mark2 0x%llx/0x%llx",
											IPTABLES, chain_name, mark, mask);
				va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef CONFIG_IPV6
				snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m mark2 --mark2 0x%llx/0x%llx -j RETURN",
											IP6TABLES, chain_name, mark, mask);
				va_cmd("/bin/sh", 2, 1, "-c", cmd);

				snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m multiport --dports 80,443 -j MARK2 --set-mark2 0x%llx/0x%llx",
											IP6TABLES, chain_name, mark, mask);
				va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
#endif

				node = ParentalCtrl_list;
				while (node != NULL)
				{
					if (memcmp(node->MACAddress, MACAddress, MAC_ADDR_LEN) == 0 && strlen(node->UrlAddress) > 0)
					{
#ifdef CONFIG_NETFILTER_XT_MATCH_WEBURL
						va_cmd(IPTABLES, 13, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "80", "-m", "weburl", "--contains", node->UrlAddress, "--domain_only", "-j", (char *)FW_ACCEPT);
						va_cmd(IPTABLES, 13, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "443", "-m", "weburl", "--contains", node->UrlAddress, "--domain_only", "-j", (char *)FW_ACCEPT);
#ifdef CONFIG_IPV6
						va_cmd(IP6TABLES, 13, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "80", "-m", "weburl", "--contains", node->UrlAddress, "--domain_only", "-j", (char *)FW_ACCEPT);
						va_cmd(IP6TABLES, 13, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "443", "-m", "weburl", "--contains", node->UrlAddress, "--domain_only", "-j", (char *)FW_ACCEPT);
#endif
#else
						va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "80", "-m", "string", "--urlalw", node->UrlAddress, "--algo", "bm", "-j", (char *)FW_ACCEPT);
						va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "443", "-m", "string", "--urlalw", node->UrlAddress, "--algo", "bm", "-j", (char *)FW_ACCEPT);
#ifdef CONFIG_IPV6
						va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "80", "-m", "string", "--urlalw", node->UrlAddress, "--algo", "bm", "-j", (char *)FW_ACCEPT);
						va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "443", "-m", "string", "--urlalw", node->UrlAddress, "--algo", "bm", "-j", (char *)FW_ACCEPT);
#endif
#endif
					}

					node = node->next;
				}
			}

#ifdef CONFIG_RTK_SKB_MARK2
			snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m multiport --dports 80,443 -j MARK2 --set-mark2 0x0/0x%llx",
										IPTABLES, chain_name, mask);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef CONFIG_IPV6
			snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m multiport --dports 80,443 -j MARK2 --set-mark2 0x0/0x%llx",
										IP6TABLES, chain_name, mask);
			va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
#endif

#ifdef CONFIG_NETFILTER_XT_MATCH_WEBURL
			va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "80", "-m", "state", "--state", "ESTABLISHED", "-j", (char *)FW_DROP);
			va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "443", "-m", "state", "--state", "ESTABLISHED", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 12, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "80", "-m", "state", "--state", "ESTABLISHED", "-j", (char *)FW_DROP);
			va_cmd(IP6TABLES, 12, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "443", "-m", "state", "--state", "ESTABLISHED", "-j", (char *)FW_DROP);
#endif
#else
			va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "80", "-m", "string", "--urlalw", "&endofurl&", "--algo", "bm", "-j", (char *)FW_DROP);
			va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "443", "-m", "string", "--urlalw", "&endofurl&", "--algo", "bm", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "80", "-m", "string", "--urlalw", "&endofurl&", "--algo", "bm", "-j", (char *)FW_DROP);
			va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, chain_name, "-p", "tcp", "--dport", "443", "-m", "string", "--urlalw", "&endofurl&", "--algo", "bm", "-j", (char *)FW_DROP);
#endif
#endif
		}
	}
	else //PARENTALCTRL_MODE_MAC_FILTER_NONE
	{
#ifdef CONFIG_RTK_SKB_MARK2
		snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m multiport --dports 80,443 -j MARK2 --set-mark2 0x%llx/0x%llx",
									IPTABLES, chain_name, mark, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef CONFIG_IPV6
		snprintf(cmd, sizeof(cmd), "%s -A %s -p tcp -m multiport --dports 80,443 -j MARK2 --set-mark2 0x%llx/0x%llx",
									IP6TABLES, chain_name, mark, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
#endif
	}

	return;
}

void ParentalCtrl_rule_free(void)
{
	if (ParentalCtrl_list)
	{
		int is_modify = 0;
		struct ParentalCtrl_info *node = ParentalCtrl_list, *tmp = NULL;
		while (node != NULL)
		{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
			is_modify |= ParentalCtrl_MAC_Filter_set(0, node->MACAddress);
#endif
			ParentalCtrl_chain_setup(0, node->MACAddress);

			tmp = node;
			node = node->next;
			free(tmp);
		}

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
		if (is_modify == 1)
		{
			mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
#if defined(CONFIG_CU_BASEON_YUEME) && defined(COM_CUC_IGD1_ParentalControlConfig)
			/*mergy for CU DBUS: for parental MAC filter not use MACfilter MIB*/
			setupMacFilter4ParentalCtrl();
#else

			setupMacFilterTables();
#endif
		}
#endif
	}

	ParentalCtrl_list = NULL;

	va_cmd(IPTABLES, 2, 1, "-F", PARENTALCTRL_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", PARENTALCTRLPOST_CHAIN);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", PARENTALCTRLPRE_CHAIN);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", PARENTALCTRL_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", PARENTALCTRLPOST_CHAIN);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", PARENTALCTRLPRE_CHAIN);
#endif

	return;
}

void ParentalCtrl_rule_dump(void)
{
	if (ParentalCtrl_list)
	{
		struct ParentalCtrl_info *node = ParentalCtrl_list;
		while (node != NULL)
		{
			fprintf(stderr, "--------------------\n");
			fprintf(stderr, "MACAddress = %02x:%02x:%02x:%02x:%02x:%02x\n", node->MACAddress[0], node->MACAddress[1], node->MACAddress[2],
																								node->MACAddress[3], node->MACAddress[4], node->MACAddress[5]);
			fprintf(stderr, "UrlFilterPolicy = %d\n", node->UrlFilterPolicy);
			fprintf(stderr, "UrlFilterRight = %d\n", node->UrlFilterRight);
			fprintf(stderr, "DurationPolicy = %d\n", node->DurationPolicy);
			fprintf(stderr, "DurationRight = %d\n", node->DurationRight);
			fprintf(stderr, "StartTime_hr = %d\n", node->StartTime_hr);
			fprintf(stderr, "StartTime_min = %d\n", node->StartTime_min);
			fprintf(stderr, "EndTime_hr = %d\n", node->EndTime_hr);
			fprintf(stderr, "EndTime_min = %d\n", node->EndTime_min);
			fprintf(stderr, "RepeatDay = %u\n", node->RepeatDay);
			fprintf(stderr, "UrlAddress = %s\n", (strlen(node->UrlAddress) == 0) ? "":node->UrlAddress);
			fprintf(stderr, "cur_state = %u\n\n", node->cur_state);
			node = node->next;
		}
	}
	return;
}

void ParentalCtrl_rule_init(void)
{
	int i = 0, j = 0, k = 0;
	struct ParentalCtrl_info *new_info;

	is_rule_ready = 0;

	ParentalCtrl_rule_free();

	MIB_PARENTALCTRL_MAC_T mac_entry;
	int mac_total = mib_chain_total(MIB_PARENTALCTRL_MAC_TBL);
	for (i = 0; i < mac_total; i++)
	{
		if (!mib_chain_get(MIB_PARENTALCTRL_MAC_TBL, i, (void*)&mac_entry))
			continue;

		MIB_PARENTALCTRL_TEMPLATES_T templates_entry;
		int templates_idx = -1;
		if (get_Templates_entry_by_inst_num(mac_entry.TemplateInst, &templates_entry, &templates_idx) == 0)
		{
			MIB_PARENTALCTRL_TEMPLATES_URLFILTER_T url_entry;
			MIB_PARENTALCTRL_TEMPLATES_DURATION_T duration_entry;
			int url_total = mib_chain_total(MIB_PARENTALCTRL_TEMPLATES_URLFILTER_TBL);
			int duration_total = mib_chain_total(MIB_PARENTALCTRL_TEMPLATES_DURATION_TBL);

			ParentalCtrl_chain_setup(1, mac_entry.MACAddress);

			if ((templates_entry.UrlFilterRight == 1 && templates_entry.DurationRight == 0) ||
				(templates_entry.UrlFilterRight == 1 && templates_entry.DurationRight == 1 && (url_total > 0 && duration_total == 0)))
			{
				// check URL only
				if (url_total == 0)
				{
					fprintf(stderr, "MAC %d, url_total = 0\n", i);
					new_info = malloc(sizeof(struct ParentalCtrl_info));
					memset(new_info, 0, sizeof(struct ParentalCtrl_info));
					memcpy(new_info->MACAddress, mac_entry.MACAddress, 6);
					new_info->UrlFilterPolicy = templates_entry.UrlFilterPolicy;
					new_info->UrlFilterRight = templates_entry.UrlFilterRight;
					new_info->DurationPolicy = 0;
					new_info->DurationRight = 0;
					new_info->cur_state = 2;

					new_info->next = ParentalCtrl_list;
					ParentalCtrl_list = new_info;
				}
				else
				{
					for (j = 0; j < url_total; j++)
					{
						if (!mib_chain_get(MIB_PARENTALCTRL_TEMPLATES_URLFILTER_TBL, j, (void*)&url_entry))
							continue;

						if (url_entry.TemplateInst == templates_entry.inst_num)
						{
							if (strlen(url_entry.UrlAddress) > 0)
							{
								fprintf(stderr, "MAC %d, URLFILTER %d\n", i, j);
								new_info = malloc(sizeof(struct ParentalCtrl_info));
								memset(new_info, 0, sizeof(struct ParentalCtrl_info));
								memcpy(new_info->MACAddress, mac_entry.MACAddress, 6);
								new_info->UrlFilterPolicy = templates_entry.UrlFilterPolicy;
								new_info->UrlFilterRight = templates_entry.UrlFilterRight;
								new_info->DurationPolicy = 0;
								new_info->DurationRight = 0;
								new_info->cur_state = 2;

								strncpy(new_info->UrlAddress, url_entry.UrlAddress, sizeof(url_entry.UrlAddress));

								new_info->next = ParentalCtrl_list;
								ParentalCtrl_list = new_info;
							}
						}
					}
				}
			}
			else if ((templates_entry.UrlFilterRight == 0 && templates_entry.DurationRight == 1) ||
						(templates_entry.UrlFilterRight == 1 && templates_entry.DurationRight == 1 && (url_total == 0 && duration_total > 0)))
			{
				// check duration only
				if (duration_total == 0)
				{
					fprintf(stderr, "MAC %d, duration_total = 0\n", i);
					new_info = malloc(sizeof(struct ParentalCtrl_info));
					memset(new_info, 0, sizeof(struct ParentalCtrl_info));
					memcpy(new_info->MACAddress, mac_entry.MACAddress, 6);
					new_info->UrlFilterPolicy = 0;
					new_info->UrlFilterRight = 0;
					new_info->DurationPolicy = templates_entry.DurationPolicy;
					new_info->DurationRight = templates_entry.DurationRight;

					new_info->StartTime_hr = 0;
					new_info->StartTime_min = 0;
					new_info->EndTime_hr = 23;
					new_info->EndTime_min = 59;
					new_info->RepeatDay = 0x7f; //FLAG_ALL_DAY
					new_info->cur_state = 2;

					new_info->next = ParentalCtrl_list;
					ParentalCtrl_list = new_info;
				}
				else
				{
					for (k = 0; k < duration_total; k++)
					{
						if (!mib_chain_get(MIB_PARENTALCTRL_TEMPLATES_DURATION_TBL, k, (void*)&duration_entry))
							continue;

						if (duration_entry.TemplateInst == templates_entry.inst_num)
						{
							int t1 = (duration_entry.StartTime_hr * 60) + duration_entry.StartTime_min;
							int t2 = (duration_entry.EndTime_hr * 60) + duration_entry.EndTime_min;
							if (t1 >= 0 && t2 > 0 && t2 >= t1 && duration_entry.RepeatDay > 0)
							{
								fprintf(stderr, "MAC %d, DURATION %d\n", i, k);
								new_info = malloc(sizeof(struct ParentalCtrl_info));
								memset(new_info, 0, sizeof(struct ParentalCtrl_info));
								memcpy(new_info->MACAddress, mac_entry.MACAddress, 6);
								new_info->UrlFilterPolicy = 0;
								new_info->UrlFilterRight = 0;
								new_info->DurationPolicy = templates_entry.DurationPolicy;
								new_info->DurationRight = templates_entry.DurationRight;

								new_info->StartTime_hr = duration_entry.StartTime_hr;
								new_info->StartTime_min = duration_entry.StartTime_min;
								new_info->EndTime_hr = duration_entry.EndTime_hr;
								new_info->EndTime_min = duration_entry.EndTime_min;
								new_info->RepeatDay = duration_entry.RepeatDay;
								new_info->cur_state = 2;

								new_info->next = ParentalCtrl_list;
								ParentalCtrl_list = new_info;
							}
						}
					}
				}
			}
			else if (templates_entry.UrlFilterRight == 1 && templates_entry.DurationRight == 1 && (url_total > 0 && duration_total > 0))
			{
				// check URL and duration
				for (j = 0; j < url_total; j++)
				{
					if (!mib_chain_get(MIB_PARENTALCTRL_TEMPLATES_URLFILTER_TBL, j, (void*)&url_entry))
						continue;

					if (url_entry.TemplateInst == templates_entry.inst_num)
					{
						for (k = 0; k < duration_total; k++)
						{
							if (!mib_chain_get(MIB_PARENTALCTRL_TEMPLATES_DURATION_TBL, k, (void*)&duration_entry))
								continue;

							if (duration_entry.TemplateInst == templates_entry.inst_num)
							{
								int t1 = (duration_entry.StartTime_hr * 60) + duration_entry.StartTime_min;
								int t2 = (duration_entry.EndTime_hr * 60) + duration_entry.EndTime_min;
								if ((strlen(url_entry.UrlAddress) > 0) && (t1 >= 0 && t2 > 0 && t2 >= t1 && duration_entry.RepeatDay > 0))
								{
									fprintf(stderr, "MAC %d, URLFILTER %d, DURATION %d\n", i, j, k);
									new_info = malloc(sizeof(struct ParentalCtrl_info));
									memset(new_info, 0, sizeof(struct ParentalCtrl_info));
									memcpy(new_info->MACAddress, mac_entry.MACAddress, 6);
									new_info->UrlFilterPolicy = templates_entry.UrlFilterPolicy;
									new_info->UrlFilterRight = templates_entry.UrlFilterRight;
									new_info->DurationPolicy = templates_entry.DurationPolicy;
									new_info->DurationRight = templates_entry.DurationRight;

									strncpy(new_info->UrlAddress, url_entry.UrlAddress, sizeof(url_entry.UrlAddress));

									new_info->StartTime_hr = duration_entry.StartTime_hr;
									new_info->StartTime_min = duration_entry.StartTime_min;
									new_info->EndTime_hr = duration_entry.EndTime_hr;
									new_info->EndTime_min = duration_entry.EndTime_min;
									new_info->RepeatDay = duration_entry.RepeatDay;
									new_info->cur_state = 2;

									new_info->next = ParentalCtrl_list;
									ParentalCtrl_list = new_info;
								}
							}
						}
					}
				}
			}
			else if (templates_entry.UrlFilterRight == 1 && templates_entry.DurationRight == 1 && (url_total == 0 && duration_total == 0))
			{
				fprintf(stderr, "MAC %d, empty rule\n", i);
			}
		}
	}

	if (ParentalCtrl_list)
	{
#ifdef CONFIG_RTK_SKB_MARK2
		char cmd[1024] = {0};
		unsigned long long mask, mark;
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
		unsigned long long trap_mask, trap_mark;
#endif

		mask = SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_BIT_MASK;
		mark = SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_BIT_MASK;
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
		trap_mask = SOCK_MARK2_FORWARD_BY_PS_BIT_MASK;
		trap_mark = SOCK_MARK2_FORWARD_BY_PS_BIT_MASK;
#endif

		snprintf(cmd, sizeof(cmd), "%s -t mangle -A %s -p tcp -m multiport --dports 80,443 -m conntrack --ctstate ESTABLISHED -j CONNMARK2 --restore-mark2 --ctmask2 0x%llx --nfmask 0x%llx",
									IPTABLES, PARENTALCTRLPRE_CHAIN, mask, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s -t mangle -A %s -p tcp -m multiport --dports 80,443 -m conntrack --ctstate ESTABLISHED -j CONNMARK2 --save-mark2 --ctmask2 0x%llx --nfmask 0x%llx",
									IPTABLES, PARENTALCTRLPOST_CHAIN, mask, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
		snprintf(cmd, sizeof(cmd), "%s -t mangle -A %s -p tcp -m multiport --dports 80,443 -m mark2 --mark2 0x0/0x%llx -j MARK2 --set-mark2 0x%llx/0x%llx",
									IPTABLES, PARENTALCTRLPOST_CHAIN, mask, trap_mark, trap_mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif

#ifdef CONFIG_IPV6
		snprintf(cmd, sizeof(cmd), "%s -t mangle -A %s -p tcp -m multiport --dports 80,443 -m conntrack --ctstate ESTABLISHED -j CONNMARK2 --restore-mark2 --ctmask2 0x%llx --nfmask 0x%llx",
									IP6TABLES, PARENTALCTRLPRE_CHAIN, mask, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "%s -t mangle -A %s -p tcp -m multiport --dports 80,443 -m conntrack --ctstate ESTABLISHED -j CONNMARK2 --save-mark2 --ctmask2 0x%llx --nfmask 0x%llx",
									IP6TABLES, PARENTALCTRLPOST_CHAIN, mask, mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
		snprintf(cmd, sizeof(cmd), "%s -t mangle -A %s -p tcp -m multiport --dports 80,443 -m mark2 --mark2 0x0/0x%llx -j MARK2 --set-mark2 0x%llx/0x%llx",
									IP6TABLES, PARENTALCTRLPOST_CHAIN, mask, trap_mark, trap_mask);
		va_cmd("/bin/sh", 2, 1, "-c", cmd);
#endif
#endif
#endif
	}

	ParentalCtrl_rule_dump();
	is_rule_ready = 1;
	return;
}

int ParentalCtrl_time_check(int StartTime, int EndTime, unsigned int RepeatDay)
{
	time_t tm;
	struct tm the_tm;
	time(&tm);
	memcpy(&the_tm, localtime(&tm), sizeof(the_tm));
	int t_now = 0;
	//printf("day of the week:%d, RepeatDay:%d\r\n", (1 << the_tm.tm_wday), RepeatDay);

	if ((RepeatDay & (1 << the_tm.tm_wday)) != 0)
	{
		t_now = (the_tm.tm_hour * 60) + the_tm.tm_min;
		//printf("t_now(%d), StartTime(%d), EndTime(%d)\r\n", t_now, StartTime, EndTime);
		if ((t_now >= StartTime) && (t_now <= EndTime))
			return 1;
	}
	return 0;
}

int ParentalCtrl_rule_set(void)
{
	if (is_rule_ready)
	{
		int i = 0, j = 0, k = 0, is_modify = 0;
		if (ParentalCtrl_list)
		{
			struct ParentalCtrl_info *node = ParentalCtrl_list;
			while (node != NULL)
			{
				if (0)
				{
					fprintf(stderr, "--------------------\n");
					fprintf(stderr, "MACAddress = %02x:%02x:%02x:%02x:%02x:%02x\n", node->MACAddress[0], node->MACAddress[1], node->MACAddress[2],
																										node->MACAddress[3], node->MACAddress[4], node->MACAddress[5]);
					fprintf(stderr, "UrlFilterPolicy = %d\n", node->UrlFilterPolicy);
					fprintf(stderr, "UrlFilterRight = %d\n", node->UrlFilterRight);
					fprintf(stderr, "DurationPolicy = %d\n", node->DurationPolicy);
					fprintf(stderr, "DurationRight = %d\n", node->DurationRight);
					fprintf(stderr, "StartTime_hr = %d\n", node->StartTime_hr);
					fprintf(stderr, "StartTime_min = %d\n", node->StartTime_min);
					fprintf(stderr, "EndTime_hr = %d\n", node->EndTime_hr);
					fprintf(stderr, "EndTime_min = %d\n", node->EndTime_min);
					fprintf(stderr, "RepeatDay = %u\n", node->RepeatDay);
					fprintf(stderr, "UrlAddress = %s\n", (strlen(node->UrlAddress) == 0) ? "":node->UrlAddress);
					fprintf(stderr, "cur_state = %u\n\n", node->cur_state);
				}

				if (node->UrlFilterRight == 1 && node->DurationRight == 0 && (node->cur_state == 0 || node->cur_state == 2))
				{
					if (strlen(node->UrlAddress) > 0)
					{
						fprintf(stderr, "(URL 1) Set %02x:%02x:%02x:%02x:%02x:%02x (Mode: %s) (URL: %s)\n\n",
												node->MACAddress[0], node->MACAddress[1], node->MACAddress[2],
												node->MACAddress[3], node->MACAddress[4], node->MACAddress[5],
												(node->UrlFilterPolicy == 0) ? "BLACK":"WHITE",
												node->UrlAddress);
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
						is_modify |= ParentalCtrl_MAC_Filter_set(0, node->MACAddress);
#endif
						ParentalCtrl_MAC_Policy_Set(node->MACAddress, (node->UrlFilterPolicy == 0) ? (PARENTALCTRL_MODE_URL_BLACK):(PARENTALCTRL_MODE_URL_WHITE));
					}
					else
					{
						if (node->UrlFilterPolicy == 1)
						{
							fprintf(stderr, "(URL 2) Set %02x:%02x:%02x:%02x:%02x:%02x (Mode: %s) (URL: NULL)\n\n",
												node->MACAddress[0], node->MACAddress[1], node->MACAddress[2],
												node->MACAddress[3], node->MACAddress[4], node->MACAddress[5],
												(node->UrlFilterPolicy == 0) ? "BLACK":"WHITE");

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
							is_modify |= ParentalCtrl_MAC_Filter_set(1, node->MACAddress);
#endif
							ParentalCtrl_MAC_Policy_Set(node->MACAddress, PARENTALCTRL_MODE_MAC_FILTER_ALL);
						}
					}
					node->cur_state = 1;
				}
				else if (node->UrlFilterRight == 0 && node->DurationRight == 1)
				{
					int timecheck = 0, t1 = 0, t2 = 0;
					t1 = (node->StartTime_hr * 60) + node->StartTime_min;
					t2 = (node->EndTime_hr * 60) + node->EndTime_min;
					timecheck = ParentalCtrl_time_check(t1, t2, node->RepeatDay);

					if (node->DurationPolicy == 0 && timecheck == 1 && (node->cur_state == 0 || node->cur_state == 2))
					{
						fprintf(stderr, "(Duration 1) Block all URLs of %02x:%02x:%02x:%02x:%02x:%02x\n\n",
												node->MACAddress[0], node->MACAddress[1], node->MACAddress[2],
												node->MACAddress[3], node->MACAddress[4], node->MACAddress[5]);

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
						is_modify |= ParentalCtrl_MAC_Filter_set(1, node->MACAddress);
#endif
						ParentalCtrl_MAC_Policy_Set(node->MACAddress, PARENTALCTRL_MODE_MAC_FILTER_ALL);
						node->cur_state = 1;
					}
					else if (node->DurationPolicy == 0 && timecheck == 0 && (node->cur_state == 1 || node->cur_state == 2))
					{
						fprintf(stderr, "(Duration 2) Remove blocked MAC %02x:%02x:%02x:%02x:%02x:%02x\n\n",
												node->MACAddress[0], node->MACAddress[1], node->MACAddress[2],
												node->MACAddress[3], node->MACAddress[4], node->MACAddress[5]);
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
						is_modify |= ParentalCtrl_MAC_Filter_set(0, node->MACAddress);
#endif
						ParentalCtrl_MAC_Policy_Set(node->MACAddress, PARENTALCTRL_MODE_MAC_FILTER_NONE);
						node->cur_state = 0;
					}
					else if (node->DurationPolicy == 1 && timecheck == 1 && (node->cur_state == 1 || node->cur_state == 2))
					{
						fprintf(stderr, "(Duration 3) Remove blocked MAC %02x:%02x:%02x:%02x:%02x:%02x\n\n",
												node->MACAddress[0], node->MACAddress[1], node->MACAddress[2],
												node->MACAddress[3], node->MACAddress[4], node->MACAddress[5]);
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
						is_modify |= ParentalCtrl_MAC_Filter_set(0, node->MACAddress);
#endif
						ParentalCtrl_MAC_Policy_Set(node->MACAddress, PARENTALCTRL_MODE_MAC_FILTER_NONE);
						node->cur_state = 0;
					}
					else if (node->DurationPolicy == 1 && timecheck == 0 && (node->cur_state == 0 || node->cur_state == 2))
					{
						fprintf(stderr, "(Duration 4) Block all URLs of %02x:%02x:%02x:%02x:%02x:%02x\n\n",
												node->MACAddress[0], node->MACAddress[1], node->MACAddress[2],
												node->MACAddress[3], node->MACAddress[4], node->MACAddress[5]);
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
						is_modify |= ParentalCtrl_MAC_Filter_set(1, node->MACAddress);
#endif
						ParentalCtrl_MAC_Policy_Set(node->MACAddress, PARENTALCTRL_MODE_MAC_FILTER_ALL);
						node->cur_state = 1;
					}
				}
				else if (node->UrlFilterRight == 1 && node->DurationRight == 1)
				{
					int timecheck = 0, t1 = 0, t2 = 0;
					t1 = (node->StartTime_hr * 60) + node->StartTime_min;
					t2 = (node->EndTime_hr * 60) + node->EndTime_min;
					timecheck = ParentalCtrl_time_check(t1, t2, node->RepeatDay);

					if (node->UrlFilterPolicy == 0)//blacklist
					{
						if (((node->DurationPolicy == 0 && timecheck == 1) || (node->DurationPolicy == 1 && timecheck == 0)) && (node->cur_state == 1 || node->cur_state == 2))
						{
							fprintf(stderr, "(Both 1) Block all URLs of %02x:%02x:%02x:%02x:%02x:%02x\n\n",
													node->MACAddress[0], node->MACAddress[1], node->MACAddress[2],
													node->MACAddress[3], node->MACAddress[4], node->MACAddress[5]);

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
							is_modify |= ParentalCtrl_MAC_Filter_set(1, node->MACAddress);
#endif
							ParentalCtrl_MAC_Policy_Set(node->MACAddress, PARENTALCTRL_MODE_MAC_FILTER_ALL);
							node->cur_state = 0;
						}
						else if (((node->DurationPolicy == 0 && timecheck == 0) || (node->DurationPolicy == 1 && timecheck == 1)) && (node->cur_state == 0 || node->cur_state == 2))
						{
							if (strlen(node->UrlAddress) > 0)
							{
								fprintf(stderr, "(Both 2) Set %02x:%02x:%02x:%02x:%02x:%02x (Mode: %s) (URL: %s)\n\n",
														node->MACAddress[0], node->MACAddress[1], node->MACAddress[2],
														node->MACAddress[3], node->MACAddress[4], node->MACAddress[5],
														(node->UrlFilterPolicy == 0) ? "BLACK":"WHITE",
														node->UrlAddress);
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
								is_modify |= ParentalCtrl_MAC_Filter_set(0, node->MACAddress);
#endif
								ParentalCtrl_MAC_Policy_Set(node->MACAddress, (node->UrlFilterPolicy == 0) ? (PARENTALCTRL_MODE_URL_BLACK):(PARENTALCTRL_MODE_URL_WHITE));
							}
							node->cur_state = 1;
						}
					}
					else if (node->UrlFilterPolicy == 1)
					{
						if (((node->DurationPolicy == 0 && timecheck == 0) || (node->DurationPolicy == 1 && timecheck == 1)) && (node->cur_state == 1 || node->cur_state == 2))
						{
							fprintf(stderr, "(Both 3) Remove blocked MAC %02x:%02x:%02x:%02x:%02x:%02x\n\n",
													node->MACAddress[0], node->MACAddress[1], node->MACAddress[2],
													node->MACAddress[3], node->MACAddress[4], node->MACAddress[5]);

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
							is_modify |= ParentalCtrl_MAC_Filter_set(0, node->MACAddress);
#endif
#ifdef CONFIG_CU
							ParentalCtrl_MAC_Policy_Set(node->MACAddress, (node->UrlFilterPolicy == 0) ? (PARENTALCTRL_MODE_URL_BLACK):(PARENTALCTRL_MODE_URL_WHITE));

#else
							ParentalCtrl_MAC_Policy_Set(node->MACAddress, PARENTALCTRL_MODE_MAC_FILTER_NONE);
#endif
							node->cur_state = 0;
						}
						else if (((node->DurationPolicy == 0 && timecheck == 1) || (node->DurationPolicy == 1 && timecheck == 0)) && (node->cur_state == 0 || node->cur_state == 2))
						{
							if (strlen(node->UrlAddress) > 0)
							{
								fprintf(stderr, "(Both 4) Set %02x:%02x:%02x:%02x:%02x:%02x (Mode: %s) (URL: %s)\n\n",
														node->MACAddress[0], node->MACAddress[1], node->MACAddress[2],
														node->MACAddress[3], node->MACAddress[4], node->MACAddress[5],
														(node->UrlFilterPolicy == 0) ? "BLACK":"WHITE",
														node->UrlAddress);
#if defined(CONFIG_CU)
	/* CU spec: out the range of Duration, should block all */
								is_modify |= ParentalCtrl_MAC_Filter_set(1, node->MACAddress);
								ParentalCtrl_MAC_Policy_Set(node->MACAddress, PARENTALCTRL_MODE_MAC_FILTER_ALL);

#else
#if defined(CONFIG_CMCC)
								is_modify |= ParentalCtrl_MAC_Filter_set(0, node->MACAddress);
#endif
								ParentalCtrl_MAC_Policy_Set(node->MACAddress, (node->UrlFilterPolicy == 0) ? (PARENTALCTRL_MODE_URL_BLACK):(PARENTALCTRL_MODE_URL_WHITE));
#endif
							}
							node->cur_state = 1;
						}
					}
				}
				node = node->next;
			}

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
			if (is_modify == 1)
			{
				mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
#if defined(CONFIG_CU_BASEON_YUEME) && defined(COM_CUC_IGD1_ParentalControlConfig)
				/*mergy for CU DBUS: for parental MAC filter not use MACfilter MIB*/
				setupMacFilter4ParentalCtrl();
#else
				setupMacFilterTables();
#endif
			}
#endif
		}
	}
	return 0;
}
#endif

#if defined(CONFIG_LUNA) && defined(CONFIG_COMMON_RT_API)
#ifdef SUPPORT_DOS_SYSLOG
#define DOS_LOG_RATE   1  // 1/m
const char FW_DOS_SYNFLOOD[] = "DOS_SYNFLOOD";
const char FW_DOS_FINFLOOD[] = "DOS_FINFLOOD";
const char FW_DOS_ICMPFLOOD[] = "DOS_ICMPFLOOD";

static void rtk_fw_dos_syslog_set(unsigned int dos_enable)
{
	char limit_rate[32]={0};
	char limit_burst[32]={0};
	char log_rate[32]={0};
	char log_burst[32]={0};
	unsigned int floodrate=100;

	va_cmd(IPTABLES, 11, 1, "-t", "mangle", "-D", FW_DOS, "-p", "tcp", "--tcp-flags", "SYN,ACK", "SYN", "-j", FW_DOS_SYNFLOOD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", FW_DOS_SYNFLOOD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", FW_DOS_SYNFLOOD);

	va_cmd(IPTABLES, 11, 1, "-t", "mangle", "-D", FW_DOS, "-p", "tcp", "--tcp-flags", "FIN", "FIN", "-j", FW_DOS_FINFLOOD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", FW_DOS_FINFLOOD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", FW_DOS_FINFLOOD);

	va_cmd(IPTABLES, 8, 1, "-t", "mangle", "-D", FW_DOS, "-p", "icmp", "-j", FW_DOS_ICMPFLOOD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", FW_DOS_ICMPFLOOD);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", FW_DOS_ICMPFLOOD);
	if(!(dos_enable & DOS_ENABLE))
		return;
	snprintf(log_rate, sizeof(log_rate), "%d/m", DOS_LOG_RATE);
	snprintf(log_burst, sizeof(log_burst), "%d", DOS_LOG_RATE);
	if(dos_enable & SYSFLOODSYN)
	{
		if (!mib_get_s( MIB_DOS_SYSSYN_FLOOD,  (void *)&floodrate, sizeof(floodrate)))
		{
			printf("DOS parameter:MIB_DOS_SYSSYN_FLOOD failed!\n");
		}
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", FW_DOS_SYNFLOOD);
		snprintf(limit_rate, sizeof(limit_rate), "%d/s", floodrate);
		snprintf(limit_burst, sizeof(limit_burst), "%d", 2*floodrate);
		va_cmd(IPTABLES, 11, 1, "-t", "mangle", "-A", FW_DOS, "-p", "tcp", "--tcp-flags", "SYN,ACK", "SYN", "-j", FW_DOS_SYNFLOOD);
		va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", FW_DOS_SYNFLOOD, "-m", "limit", "--limit", limit_rate,
			"--limit-burst", limit_burst, "-j", FW_RETURN);
		va_cmd(IPTABLES, 16, 1, "-t", "mangle", "-A", FW_DOS_SYNFLOOD, "-m", "limit", "--limit", log_rate,
			"--limit-burst", log_burst, "-j", "LOG", "--log-level", "emerg", "--log-prefix", "dos_syn_flood:");
		//va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", FW_DOS_SYNFLOOD, "-j", FW_DROP);
	}
	if(dos_enable & SYSFLOODFIN)
	{
		if (!mib_get_s( MIB_DOS_SYSFIN_FLOOD,  (void *)&floodrate, sizeof(floodrate)))
		{
			printf("DOS parameter:MIB_DOS_SYSFIN_FLOOD failed!\n");
		}
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", FW_DOS_FINFLOOD);
		snprintf(limit_rate, sizeof(limit_rate), "%d/s", floodrate);
		snprintf(limit_burst, sizeof(limit_burst), "%d", 2*floodrate);
		va_cmd(IPTABLES, 11, 1, "-t", "mangle", "-A", FW_DOS, "-p", "tcp", "--tcp-flags", "FIN", "FIN", "-j", FW_DOS_FINFLOOD);
		va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", FW_DOS_FINFLOOD, "-m", "limit", "--limit", limit_rate,
			"--limit-burst", limit_burst, "-j", FW_RETURN);
		va_cmd(IPTABLES, 16, 1, "-t", "mangle", "-A", FW_DOS_FINFLOOD, "-m", "limit", "--limit", log_rate,
			"--limit-burst", log_burst, "-j", "LOG", "--log-level", "emerg", "--log-prefix", "dos_fin_flood:");
		//va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", FW_DOS_FINFLOOD, "-j", FW_DROP);
	}
	if(dos_enable & SYSFLOODICMP)
	{
		if (!mib_get_s( MIB_DOS_SYSICMP_FLOOD,  (void *)&floodrate, sizeof(floodrate)))
		{
			printf("DOS parameter:MIB_DOS_SYSICMP_FLOOD failed!\n");
		}
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", FW_DOS_ICMPFLOOD);
		snprintf(limit_rate, sizeof(limit_rate), "%d/s", floodrate);
		snprintf(limit_burst, sizeof(limit_burst), "%d", 2*floodrate);
		va_cmd(IPTABLES, 8, 1, "-t", "mangle", "-A", FW_DOS, "-p", "icmp", "-j", FW_DOS_ICMPFLOOD);
		va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", FW_DOS_ICMPFLOOD, "-m", "limit", "--limit", limit_rate,
			"--limit-burst", limit_burst, "-j", FW_RETURN);
		va_cmd(IPTABLES, 16, 1, "-t", "mangle", "-A", FW_DOS_ICMPFLOOD, "-m", "limit", "--limit", log_rate,
			"--limit-burst", log_burst, "-j", "LOG", "--log-level", "emerg", "--log-prefix", "dos_icmp_flood:");
		//va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", FW_DOS_ICMPFLOOD, "-j", FW_DROP);
	}
}
#endif
#include <rtk/rt/rt_sec.h>
#include <rtk/rt/rt_switch.h>
static void rtk_fw_dos_set(unsigned int dos_enable)
{
	rtk_switch_devInfo_t devInfo;
	int port = 0;
	int ret = 0;
	unsigned int state = (dos_enable & DOS_ENABLE);
	if(dos_enable)
		state = 1;

	ret = rtk_switch_deviceInfo_get(&devInfo);
	if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);

#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_YUEME)
	{
		//due 827x hw ddos protection is higher priority than acl action
		//so trap 2 cpu would affect by ddos protection. (drop than trap)
		//so, by yueme ddos test case, we setup wan port only.
		int phyPortId = rtk_port_get_wan_phyID();
		if(phyPortId>0)
		{
			printf("enable ddos phy port: %d \n",phyPortId);
			ret = rt_sec_portAttackPreventState_set(phyPortId, state);
			if(ret != RT_ERR_OK) printf("[%s %d] port %d ret = %d\n", __func__, __LINE__, port, ret);
		}
	}
#else
	for (port = devInfo.ether.min; port <= devInfo.ether.max; port++)
	{
		if(port>=32) break;

		if (devInfo.ether.portmask.bits[0] & 1<<port)
		{
			//All Port
			ret = rt_sec_portAttackPreventState_set(port, state);
			if(ret != RT_ERR_OK) printf("[%s %d] port %d ret = %d\n", __func__, __LINE__, port, ret);
		}
	}
#endif
#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	/*
	  Do not set SYN flood of HW DDoS,
	  since it will make ITMS cannot access ONU when ONU is attacking by SYN flooding
	  even we have trap packet of ITMS server with priority 7 (rtk_fc_itms_qos_ingress_control_packet_protection_set())
	  rtk_fc_ds_tcp_syn_rate_limit_set() has been set in default
	 */
#else
	if(dos_enable & SYSFLOODSYN){
		#ifdef CONFIG_USER_RT_ACL_SYN_FLOOD_PROOF_SUPPORT
		ret = rtk_fc_ddos_tcp_syn_rate_limit_add();
		if(ret < 0) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
		#else
		ret = rt_sec_attackPrevent_set(RT_SYNFLOOD_DENY, ACTION_DROP);
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
		ret = rt_sec_attackFloodThresh_set(RT_SEC_SYNCFLOOD, 3); //threshold 3K/sec
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
		#endif
	}
	else{
		#ifdef CONFIG_USER_RT_ACL_SYN_FLOOD_PROOF_SUPPORT
		ret = rtk_fc_ddos_tcp_syn_rate_limit_delete();
		if(ret < 0) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
		#else
		ret = rt_sec_attackPrevent_set(RT_SYNFLOOD_DENY, ACTION_FORWARD);
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
		#endif
	}
#endif

	if(dos_enable & SYSFLOODFIN){
		ret = rt_sec_attackPrevent_set(RT_FINFLOOD_DENY, ACTION_DROP);
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
		ret = rt_sec_attackFloodThresh_set(RT_SEC_FINFLOOD, 3); //threshold 3K/sec
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
	}
	else{
		ret = rt_sec_attackPrevent_set(RT_FINFLOOD_DENY, ACTION_FORWARD);
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
	}

	if(dos_enable & SYSFLOODICMP){
		ret = rt_sec_attackPrevent_set(RT_ICMPFLOOD_DENY, ACTION_DROP);
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
		ret = rt_sec_attackFloodThresh_set(RT_SEC_ICMPFLOOD, 5); //threshold 5K/sec
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
	}
	else{
		ret = rt_sec_attackPrevent_set(RT_ICMPFLOOD_DENY, ACTION_FORWARD);
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
	}

	ret = rt_sec_attackPrevent_set(RT_LAND_DENY, (dos_enable & IPLANDENABLED)? ACTION_DROP: ACTION_FORWARD);
	if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);

	ret = rt_sec_attackPrevent_set(RT_POD_DENY, (dos_enable & PINGOFDEATHENABLED)? ACTION_DROP: ACTION_FORWARD);
	if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);

	ret = rt_sec_attackPrevent_set(RT_UDPDOMB_DENY, (dos_enable & UDPBombEnabled)? ACTION_DROP: ACTION_FORWARD);
	if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);

	ret = rt_sec_attackPrevent_set(RT_ICMP_FRAG_PKTS_DENY, (dos_enable & ICMPFRAGMENT)? ACTION_DROP: ACTION_FORWARD);
	if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);

	ret = rt_sec_attackPrevent_set(RT_TCP_FRAG_OFF_MIN_CHECK, (dos_enable & TCPFRAGOFFMIN)? ACTION_DROP: ACTION_FORWARD);
	if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);

	ret = rt_sec_attackPrevent_set(RT_TCPHDR_MIN_CHECK, (dos_enable & TCPHDRMIN)? ACTION_DROP: ACTION_FORWARD);
	if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);

	if(dos_enable & TCPSCANENABLED){
		ret = rt_sec_attackPrevent_set(RT_SYNFIN_DENY, ACTION_DROP);
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
		ret = rt_sec_attackPrevent_set(RT_XMA_DENY, ACTION_DROP);
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
		ret = rt_sec_attackPrevent_set(RT_NULLSCAN_DENY, ACTION_DROP);
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
	}
	else{
		ret = rt_sec_attackPrevent_set(RT_SYNFIN_DENY, ACTION_FORWARD);
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
		ret = rt_sec_attackPrevent_set(RT_XMA_DENY, ACTION_FORWARD);
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
		ret = rt_sec_attackPrevent_set(RT_NULLSCAN_DENY, ACTION_FORWARD);
		if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
	}

	ret = rt_sec_attackPrevent_set(RT_SYNWITHDATA_DENY, (dos_enable & TCPSynWithDataEnabled)? ACTION_DROP: ACTION_FORWARD);
	if(ret != RT_ERR_OK) printf("[%s %d] ret = %d\n", __func__, __LINE__, ret);
#ifdef SUPPORT_DOS_SYSLOG
	rtk_fw_dos_syslog_set(dos_enable);
#endif
}
#endif
#ifdef CONFIG_E8B
#define DOS_ENABLE_E8B	(DOS_ENABLE|SYSFLOODSYN|SYSFLOODFIN|SYSFLOODUDP|SYSFLOODICMP|IPFLOODSYN|\
						IPFLOODFIN|IPFLOODUDP|IPFLOODICMP|TCPUDPPORTSCAN|ICMPSMURFENABLED|\
						IPLANDENABLED|IPSPOOFENABLED|IPTEARDROPENABLED|PINGOFDEATHENABLED|\
						TCPSCANENABLED|TCPSynWithDataEnabled|UDPBombEnabled|ICMPFRAGMENT|\
						TCPFRAGOFFMIN|TCPHDRMIN|sourceIPblock)
#endif

#if	defined(CONFIG_USER_DOS_WINNUKE)
#define RTK_DOS_ENABLE	(DOS_ENABLE|WINNUKE|ICMPSMURFENABLED|IPTEARDROPENABLED|IPSPOOFENABLED)
#endif

#if defined(CONFIG_USER_DOS_ICMP_REDIRECTION)
int rtk_set_dos_icmpRedirect_rule(void){
	unsigned int dos_enable;

	if (!mib_get_s( MIB_DOS_ENABLED,  (void *)&dos_enable, sizeof(dos_enable))){
		printf("DOS parameter failed!\n");
	}

	va_cmd(IPTABLES, 11, 1, (char *)FW_DEL, (char *)FW_INPUT, (char *)ARG_NO, (char *)ARG_I, (char *)BRIF, "-p", (char *)ARG_ICMP, "--icmp-type", "5", "-j", (char *)FW_DROP);

	if(dos_enable & DOS_ENABLE){
		va_cmd(IPTABLES, 11, 1, (char *)FW_INSERT, (char *)FW_INPUT, (char *)ARG_NO, (char *)ARG_I, (char *)BRIF, "-p", (char *)ARG_ICMP, "--icmp-type", "5", "-j", (char *)FW_DROP);
	}

	return 0;
}
#endif

#ifdef CONFIG_DEONET_DOS // Deonet GNT2400R
static void setup_dos_droplist(unsigned int dos_enable)
{
	// init chain droplist
	// iptables -F droplist
	va_cmd(IPTABLES, 2, 1, (char *)FW_FLUSH, (char *)FW_DROPLIST);
	// iptables -D OUTPUT -p tcp --tcp-flags RST RST -j DROP
	va_cmd(IPTABLES, 9, 1, (char *)FW_DEL, (char *)FW_OUTPUT, "-p", (char *)ARG_TCP, "--tcp-flags", "RST", "RST", "-j", (char *)FW_DROP);

	if (dos_enable & DOS_ENABLE) {
		if (dos_enable & TCPUDPPORTSCAN) {
			// iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP
			va_cmd(IPTABLES, 9, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-p", (char *)ARG_TCP, "--tcp-flags", "RST", "RST", "-j", (char *)FW_DROP);
			// iptables -I droplist -i nas0_0 -p tcp --syn -dport :1024 -j DROP
			va_cmd(IPTABLES, 11, 1, (char *)FW_INSERT, (char *)FW_DROPLIST, (char *)ARG_I, "nas0_0", "-p", (char *)ARG_TCP, "--syn", "--dport", ":1024", "-j", (char *)FW_DROP);
		}
	}
}
#endif

void setup_dos_protection(void)
{
	unsigned int dos_enable;
	unsigned int rtk_dos_enable;
	//unsigned int dos_syssyn_flood;
	//unsigned int dos_sysfin_flood;
	//unsigned int dos_sysudp_flood;
	//unsigned int dos_sysicmp_flood;
	unsigned int dos_pipsyn_flood;
	unsigned int dos_pipfin_flood;
	unsigned int dos_pipudp_flood;
	unsigned int dos_pipicmp_flood;
	unsigned int dos_snmp_flood;
	unsigned int dos_block_time;
	unsigned int dos_mac_flood;
	unsigned int dos_icmp_ignore;
	unsigned int dos_ntp_block;
	unsigned int dos_arp_spoof;
	unsigned int dos_ratelimit_icmp;
	unsigned int dos_ratelimit_icmp_enable;
	unsigned char buffer[256],dosstr[256];
	int dostmpip[4],dostmpmask[4];
	int dosip,dosmask;
	int count = 0, phyport = 0, i;
	char str;
	char cmd[128];
	FILE *dosfp;

	if (!mib_get_s( MIB_DOS_ENABLED,  (void *)&dos_enable, sizeof(dos_enable))){
		printf("DOS parameter failed!\n");
	}
#if 0
	if (!mib_get_s( MIB_DOS_SYSSYN_FLOOD,  (void *)&dos_syssyn_flood, sizeof(dos_syssyn_flood))){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get_s( MIB_DOS_SYSFIN_FLOOD,  (void *)&dos_sysfin_flood, sizeof(dos_sysfin_flood))){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get_s( MIB_DOS_SYSUDP_FLOOD,  (void *)&dos_sysudp_flood, sizeof(dos_sysudp_flood))){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get_s( MIB_DOS_SYSICMP_FLOOD,  (void *)&dos_sysicmp_flood, sizeof(dos_sysicmp_flood))){
		printf("DOS parameter failed!\n");
	}
#endif
	if (!mib_get_s( MIB_DOS_RATELIMIT_ICMP_ENABLED,  (void *)&dos_ratelimit_icmp_enable, sizeof(dos_ratelimit_icmp_enable))){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get_s( MIB_DOS_RATELIMIT_ICMP_FLOOD,  (void *)&dos_ratelimit_icmp, sizeof(dos_ratelimit_icmp))){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get_s( MIB_DOS_PIPSYN_FLOOD,  (void *)&dos_pipsyn_flood, sizeof(dos_pipsyn_flood))){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get_s( MIB_DOS_PIPFIN_FLOOD,  (void *)&dos_pipfin_flood, sizeof(dos_pipfin_flood))){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get_s( MIB_DOS_PIPUDP_FLOOD,  (void *)&dos_pipudp_flood, sizeof(dos_pipudp_flood))){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get_s( MIB_DOS_PIPICMP_FLOOD,  (void *)&dos_pipicmp_flood, sizeof(dos_pipicmp_flood))){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get_s( MIB_DOS_SNMP_FLOOD,  (void *)&dos_snmp_flood, sizeof(dos_snmp_flood))){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get_s( MIB_DOS_BLOCK_TIME,  (void *)&dos_block_time, sizeof(dos_block_time))){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get_s( MIB_DOS_ICMP_IGNORE,  (void *)&dos_icmp_ignore, sizeof(dos_icmp_ignore))){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get_s( MIB_DOS_NTP_BLOCK,  (void *)&dos_ntp_block, sizeof(dos_ntp_block))){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get_s( MIB_DOS_ARP_SPOOF,  (void *)&dos_arp_spoof, sizeof(dos_arp_spoof))){
		printf("DOS parameter failed!\n");
	}

#if defined(CONFIG_USER_DOS_ICMP_REDIRECTION)
	rtk_set_dos_icmpRedirect_rule();
#endif

	//get ip
	if(!mib_get_s( MIB_ADSL_LAN_IP, (void *)buffer, sizeof(buffer)))
		return;
	sscanf(inet_ntoa(*((struct in_addr *)buffer)),"%d.%d.%d.%d",&dostmpip[0],&dostmpip[1],&dostmpip[2],&dostmpip[3]);
	dosip= dostmpip[0]<<24 | dostmpip[1]<<16 | dostmpip[2]<<8 | dostmpip[3];
	//get mask
	if(!mib_get_s( MIB_ADSL_LAN_SUBNET, (void *)buffer, sizeof(buffer)))
		return;
	sscanf(inet_ntoa(*((struct in_addr *)buffer)),"%d.%d.%d.%d",&dostmpmask[0],&dostmpmask[1],&dostmpmask[2],&dostmpmask[3]);
	dosmask= dostmpmask[0]<<24 | dostmpmask[1]<<16 | dostmpmask[2]<<8 | dostmpmask[3];
	dosip &= dosmask;

	rtk_dos_enable = dos_enable;
#ifdef CONFIG_E8B
	if (rtk_dos_enable & 0x1){
#ifdef CONFIG_USER_DOS_WINNUKE
		rtk_dos_enable = RTK_DOS_ENABLE;
#else
		rtk_dos_enable = DOS_ENABLE_E8B;
#endif
	}
#endif

#ifdef CONFIG_USER_ARP_ATTACK
	rtk_dos_enable |= ARPATTACK;
	dos_enable |= ARPATTACK;
#endif
#ifdef CONFIG_USER_FRAGMENT_HEADER
	rtk_dos_enable |= FRAGMENTHEADER;
	dos_enable |= TCPHDRMIN;
#endif


#ifndef CONFIG_DEONET_DOS
	sprintf(dosstr,"1 %x %x %d %d %d %d %d %d %d %d %d %d %d\n",
		dosip,dosmask,rtk_dos_enable,dos_syssyn_flood,dos_sysfin_flood,dos_sysudp_flood,
		dos_sysicmp_flood,dos_pipsyn_flood,dos_pipfin_flood,dos_pipudp_flood,
		dos_pipicmp_flood,dos_block_time,dos_snmp_flood);
#else
	sprintf(dosstr,"1 %x %x %d %d %d %d %d %d %d %d %d %d %d\n",
		dosip, dosmask,rtk_dos_enable, 0, 0, 0, 0,
		dos_pipsyn_flood, dos_pipfin_flood, dos_pipudp_flood,
		dos_pipicmp_flood, dos_block_time, dos_snmp_flood);

#endif
	//printf("dosstr:%s\n\n",dosstr);
	dosstr[strlen(dosstr)]=0;
#if defined(CONFIG_RTK_DOS) || defined(CONFIG_DOS)
	if (dosfp = fopen("/proc/enable_dos", "w"))
	{
		fprintf(dosfp, "%s",dosstr);
		fclose(dosfp);
	}
#endif

	if (dos_arp_spoof)
		system("echo start > /proc/arp_spoof");

	if (dos_ratelimit_icmp_enable) {
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd , "echo %d > /proc/sys/net/ipv4/icmp_ratelimit ", dos_ratelimit_icmp);
		system(cmd);
		system("echo 6169 > /proc/sys/net/ipv4/icmp_ratemask");
	} else {
		system("echo 10 > /proc/sys/net/ipv4/icmp_ratelimit");
		system("echo 6169 > /proc/sys/net/ipv4/icmp_ratemask");
	}


#ifndef CONFIG_DEONET_DOS
	// mac flood
	if (dos_mac_flood) {
		for (i = 0; i < CONFIG_LAN_PORT_NUM; i++) {
			phyport = rtk_port_get_lan_phyID(i);
			if (phyport == -1)
				continue;

			rt_l2_portLimitLearningCnt_set(phyport, dos_mac_flood);
		}
	} else {
		for (i = 0; i < CONFIG_LAN_PORT_NUM; i++) {
			phyport = rtk_port_get_lan_phyID(i);
			rt_l2_portLimitLearningCnt_set(phyport, -1); // max is -1 on swtich
		}
	}
#endif

	// ignore for traceroute
	if (dos_icmp_ignore)
		system("/bin/echo icmpignore 1 > /proc/driver/realtek/icmpignore");
	else
		system("/bin/echo icmpignore 0 > /proc/driver/realtek/icmpignore");

	// block ntp attack
	system("/bin/iptables --list ntpdrop --line-numbers | grep 'udp' > /tmp/ntpchain");
	dosfp = fopen("/tmp/ntpchain", "r");
	for (str = getc(dosfp); str != EOF; str = getc(dosfp)) {
		if (str == '\n')
			count = count + 1;
	}
	fclose(dosfp);

	if (dos_ntp_block) {
		if (count == 0) {
			system("/bin/iptables -N ntpdrop");
			system("/bin/iptables -A INPUT -j ntpdrop");
		} else {
			system("/bin/iptables -D ntpdrop -p udp -i nas0_0 --dport 123 -m limit --limit 200/sec --limit-burst 100 -j ACCEPT");
			system("/bin/iptables -D ntpdrop -p udp -i nas0_0 --dport 123 -j DROP");
		}

		system("/bin/iptables -I ntpdrop -p udp -i nas0_0 --dport 123 -j DROP");
		system("/bin/iptables -I ntpdrop -p udp -i nas0_0 --dport 123 -m limit --limit 200/sec --limit-burst 100 -j ACCEPT");
	} else {
		if (count > 0) {
			system("/bin/iptables -D ntpdrop -p udp -i nas0_0 --dport 123 -m limit --limit 200/sec --limit-burst 100 -j ACCEPT");
			system("/bin/iptables -D ntpdrop -p udp -i nas0_0 --dport 123 -j DROP");
		}
	}

#if defined(CONFIG_COMMON_RT_API)
	rtk_fw_dos_set(dos_enable);

	// move to boa main function on deonet
	//rtk_fw_setup_arp_storm_control(dos_enable);
#ifdef CONFIG_DEONET_DOS // Deonet GNT2400R
	setup_dos_droplist(dos_enable);
#endif
#endif

}
#endif
#endif//defined(DOS_SUPPORT)||defined(_PRMT_X_TRUE_FIREWALL_)

char* getInternetWanIfName(void)
{
	int total, i, ret;
	MIB_CE_ATM_VC_T entry;
	struct in_addr inAddr;
	static char ifname[64]="None";

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (mib_chain_get(MIB_ATM_VC_TBL, i, &entry) == 0)
			continue;

		if ((entry.cmode != CHANNEL_MODE_BRIDGE) && (entry.applicationtype & X_CT_SRV_INTERNET) &&
				ifGetName(entry.ifIndex, ifname, sizeof(ifname)) &&
				getInFlags(ifname, &ret) &&
				(ret & IFF_UP) &&
				getInAddr(ifname, IP_ADDR, &inAddr))
			break;
	}

	return ifname;
}

#ifdef SUPPORT_TIME_SCHEDULE
char * weekDay[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

int isScheduledIndex(int index,unsigned int * bitmap, int size)
{
	int idx  = index >> 5;
	int offset = index & 0xffff;

	if(idx >=  size)
		return 0;

	if(bitmap[idx] & (1 << offset))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void addScheduledIndex(int index, unsigned int * bitmap, int size)
{
	int idx  = index >> 5;
	int offset = index & 0xffff;

	if(idx >= size)
		return;

	bitmap[idx] |= (1 << offset);
}

void delScheduledIndex(int index, unsigned int * bitmap, int size)
{
	int idx  = index >> 5;
	int offset = index & 0xffff;

	if(idx >= size)
		return;

	bitmap[idx] &=  ~(1 << offset);
}

int check_should_schedule(struct tm * pTime, MIB_CE_TIME_SCHEDULE_T * pEntry)
{
	int i;
	unsigned int sch_time;
	int result = 0;
#ifdef CONFIG_CMCC_ENTERPRISE
	int tmp1,tmp2,tmp3;
#endif

	sch_time = ((unsigned int *)pEntry->sch_time)[pTime->tm_wday];
#ifdef CONFIG_CMCC_ENTERPRISE
	if(sch_time)
	{
		tmp1 = pEntry->sch_min[0] * 60 +  pEntry->sch_min[1];
		tmp2 = pEntry->sch_min[2] * 60 +  pEntry->sch_min[3];
		tmp3 = pTime->tm_hour *60 + pTime->tm_min;
		if ((tmp3 >= tmp1) && (tmp3 <= tmp2) )
			result = 1;
	}
#else
	if(sch_time & (1 << (pTime->tm_hour)))
		result = 1;
#endif
	printf("[%s %d]tm_wday=%d, tm_hour=%d, sch_time=%d, sch_name=%s\n", __func__, __LINE__, pTime->tm_wday, pTime->tm_hour, sch_time, pEntry->name);
	printf("[%s %d]result=%d\n", __func__, __LINE__, result);
	return result;
}

void firewall_schedule()
{
	unsigned char  urlEnable=0;
	unsigned char urlfilter_mode = 0;
	mib_get_s(MIB_URLFILTER_MODE, (void *)&urlfilter_mode, sizeof(urlfilter_mode));
	if (urlfilter_mode == 1) /* White List */
		urlEnable = 2;
	else if (urlfilter_mode == 2) /* Black List */
		urlEnable = 1;

	filter_set_url(urlEnable);
	setupMacFilterTables();
	setupIPFilter();
#ifdef CONFIG_IPV6
	restart_IPV6Filter();
#endif
}
#endif

#ifdef MAC_FILTER
int setupMacFilter(void)
{
	int i, total, total_in = 0, total_out = 0;
	char *policy;
	char srcmacaddr[18], dstmacaddr[18];
	char smacParm[64]={0};
	char dmacParm[64]={0};
	char ipsmacParm[64]={0};
	char ipdmacParm[64]={0};
	MIB_CE_MAC_FILTER_T MacEntry;
	char mac_in_dft, mac_out_dft;
	char eth_mac_ctrl=0, wlan_mac_ctrl=0;
	char wanname[MAX_WAN_NAME_LEN];
	int mac_filter_in_permit  = 1;
	int mac_filter_out_permit = 1;

	DOCMDINIT;

	// Delete all Macfilter rule
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_MACFILTER_FWD);
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_MACFILTER_LI);
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_MACFILTER_LO);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_MACFILTER_FWD);

	/* default policy -- set to RETURN so we can lookup following chains */
	va_cmd(EBTABLES, 3, 1, "-P", (char *)FW_MACFILTER_FWD, (char *)FW_RETURN);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)FW_MACFILTER_LI, (char *)FW_RETURN);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)FW_MACFILTER_LO, (char *)FW_RETURN);

#if defined(CONFIG_00R0)
	char mactfilter_state=0;
	mib_get_s(MIB_MAC_FILTER_EBTABLES_ENABLE, (void*)&mactfilter_state, sizeof(mactfilter_state));

	if(mactfilter_state == 0) return 0;
#endif

	mib_get_s(MIB_MACF_OUT_ACTION, (void *)&mac_out_dft, sizeof(mac_out_dft));
	mib_get_s(MIB_MACF_IN_ACTION, (void *)&mac_in_dft, sizeof(mac_in_dft));
	mib_get_s(MIB_ETH_MAC_CTRL, (void *)&eth_mac_ctrl, sizeof(eth_mac_ctrl));
	mib_get_s(MIB_WLAN_MAC_CTRL, (void *)&wlan_mac_ctrl, sizeof(wlan_mac_ctrl));

	total = mib_chain_total(MIB_MAC_FILTER_TBL);

	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_MAC_FILTER_TBL, i, (void *)&MacEntry))
			return -1;

		smacParm[0]=dmacParm[0]='\0';

		if (MacEntry.action == 0)
			policy = (char *)FW_DROP;
		else
			policy = (char *)FW_RETURN;

		if(memcmp(MacEntry.srcMac,"\x00\x00\x00\x00\x00\x00",MAC_ADDR_LEN)) {
			snprintf(srcmacaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
				MacEntry.srcMac[0], MacEntry.srcMac[1],
				MacEntry.srcMac[2], MacEntry.srcMac[3],
				MacEntry.srcMac[4], MacEntry.srcMac[5]);
			snprintf(smacParm, sizeof(smacParm),"-s %s", srcmacaddr);
			snprintf(ipsmacParm, sizeof(ipsmacParm),"-m mac --mac-source %s", srcmacaddr);
		}

		if(memcmp(MacEntry.dstMac,"\x00\x00\x00\x00\x00\x00",MAC_ADDR_LEN)) {
			snprintf(dstmacaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
				MacEntry.dstMac[0], MacEntry.dstMac[1],
				MacEntry.dstMac[2], MacEntry.dstMac[3],
				MacEntry.dstMac[4], MacEntry.dstMac[5]);
			snprintf(dmacParm, sizeof(dmacParm),"-d %s", dstmacaddr);
			//snprintf(ipdmacParm, sizeof(ipdmacParm),"-m mac --mac-destination %s", dstmacaddr);
		}

		if (MacEntry.dir == DIR_OUT) {
			total_out++;
			if (!strlen(dmacParm)) { // compatible with TR069 -- smac outgoing
				if(mac_out_dft) {
					char intf[8];

					if(MacEntry.ifIndex != 0)
						sprintf(intf, "eth0.%d+", MacEntry.ifIndex);
					else
						strcpy(intf, "eth0+");

					// ebtables -I macfilter -i eth0+  -s 00:xx:..:xx -j ACCEPT/DROP
					DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s %s %s -j %s", (char *)FW_MACFILTER_FWD,
						(char *)intf,smacParm,dmacParm,policy);
#ifdef WLAN_SUPPORT
					// ebtables -I macfilter -i wlan0+  -s 00:xx:..:xx -j ACCEPT/DROP
					DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_FWD,
						(char *)WLAN_DEV_PREFIX,smacParm,dmacParm,policy);
#endif
					// ebtables -I macfilter_local_in -i eth0+	-s 00:xx:..:xx -j ACCEPT/DROP
					DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s %s %s -j %s", (char *)FW_MACFILTER_LI,
						(char *)intf,smacParm,dmacParm,policy);
#ifdef WLAN_SUPPORT
					// ebtables -I macfilter_local_in -i wlan+  -s 00:xx:..:xx -j ACCEPT/DROP
					DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_LI,
						(char *)WLAN_DEV_PREFIX,smacParm,dmacParm,policy);
#endif
					DOCMDARGVS(IPTABLES,DOWAIT,"-I %s -i %s %s %s -j %s", (char *)FW_MACFILTER_FWD,
						(char *)intf,ipsmacParm,ipdmacParm,policy);
				}
				else {
					if (eth_mac_ctrl) {
						// ebtables -I macfilter -i eth0+  -s 00:xx:..:xx -j ACCEPT/DROP
						DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_FWD,
							(char *)ELANIF,smacParm,dmacParm,policy);

						// ebtables -I macfilter_local_in -i eth0+	-s 00:xx:..:xx -j ACCEPT/DROP
						DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_LI,
							(char *)ELANIF,smacParm,dmacParm,policy);
					}
					if (wlan_mac_ctrl) {
#ifdef WLAN_SUPPORT
						// ebtables -I macfilter -i wlan+  -s 00:xx:..:xx -j ACCEPT/DROP
						DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_FWD,
							(char *)WLAN_DEV_PREFIX,smacParm,dmacParm,policy);

						// ebtables -I macfilter_local_in -i wlan+  -s 00:xx:..:xx -j ACCEPT/DROP
						DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_LI,
							(char *)WLAN_DEV_PREFIX,smacParm,dmacParm,policy);
#endif
					}
					DOCMDARGVS(IPTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_FWD,
						(char *)BRIF,ipsmacParm,ipdmacParm,policy);
				}
			}
			else {
				// ebtables -I macfilter -i eth0+  -s 00:xx:..:xx -j ACCEPT/DROP
				DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_FWD,
					(char *)ELANIF, smacParm,dmacParm,policy);

				// ebtables -I macfilter_local_in -i eth0+	-s 00:xx:..:xx -j ACCEPT/DROP
				DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_LI,
					(char *)ELANIF, smacParm,dmacParm,policy);
#ifdef WLAN_SUPPORT
				// ebtables -I macfilter -i wlan+  -s 00:xx:..:xx -j ACCEPT/DROP
				DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_FWD,
					(char *)WLAN_DEV_PREFIX,smacParm,dmacParm,policy);

				// ebtables -I macfilter_local_in -i wlan+  -s 00:xx:..:xx -j ACCEPT/DROP
				DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_LI,
					(char *)WLAN_DEV_PREFIX,smacParm,dmacParm,policy);
#endif
				DOCMDARGVS(IPTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_FWD,
						(char *)BRIF,ipsmacParm,ipdmacParm,policy);
			}
		}
		else { // DIR_IN
			total_in++;
#ifdef CONFIG_DEV_xDSL
			// ebtables -I macfilter -i vc+  -s 00:xx:..:xx -j ACCEPT/DROP
			DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_FWD,
				ALIASNAME_VC, smacParm,dmacParm,policy);
#endif
#ifdef CONFIG_ETHWAN
			if(MacEntry.ifIndex == DUMMY_IFINDEX){
				// ebtables -I macfilter -i nas0+  -s 00:xx:..:xx -j ACCEPT/DROP
				DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_FWD, ALIASNAME_NAS0,
					smacParm,dmacParm,policy);
				DOCMDARGVS(IPTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_FWD,
							ALIASNAME_NAS0,ipsmacParm,ipdmacParm,policy);
				DOCMDARGVS(IPTABLES,DOWAIT,"-I %s -i ppp+ %s %s -j %s", (char *)FW_MACFILTER_FWD
							,ipsmacParm,ipdmacParm,policy);
			}
			else{
				/* fix wan interface */
				DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s %s %s -j %s", (char *)FW_MACFILTER_FWD, "nas0+",
					smacParm,dmacParm,policy);
				DOCMDARGVS(IPTABLES,DOWAIT,"-I %s -i %s %s %s -j %s", (char *)FW_MACFILTER_FWD, "nas0+",
					ipsmacParm,ipdmacParm,policy);
			}
#endif
#ifdef CONFIG_PTMWAN
			// ebtables -I macfilter -i ptm0+  -s 00:xx:..:xx -j ACCEPT/DROP
						DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER_FWD, ALIASNAME_PTM0,
							smacParm,dmacParm,policy);
#endif
						// ebtables -I macfilter_local_out -o eth0+  -s 00:xx:..:xx -j ACCEPT/DROP
						DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -o %s+ %s %s -j %s", (char *)FW_MACFILTER_FWD, (char *)ELANIF,
							smacParm,dmacParm,policy);
						// ebtables -I macfilter_local_out -o eth0+  -s 00:xx:..:xx -j ACCEPT/DROP
						DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -o %s+ %s %s -j %s", (char *)FW_MACFILTER_LO, (char *)ELANIF,
							smacParm,dmacParm,policy);
#ifdef WLAN_SUPPORT
						// ebtables -I macfilter_forward -o wlan+  -s 00:xx:..:xx -j ACCEPT/DROP
						DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -o %s+ %s %s -j %s", (char *)FW_MACFILTER_FWD, (char *)WLAN_DEV_PREFIX,
							smacParm,dmacParm,policy);

						// ebtables -I macfilter_local_out -o wlan+  -s 00:xx:..:xx -j ACCEPT/DROP
						DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -o %s+ %s %s -j %s", (char *)FW_MACFILTER_LO, (char *)WLAN_DEV_PREFIX,
				smacParm,dmacParm,policy);
#endif
		}
	}

	// default action
	DOCMDARGVS(EBTABLES,DOWAIT,"-P %s RETURN", (char *)FW_MACFILTER_FWD);
	if(mac_out_dft == 0) { // DROP
		mac_filter_out_permit = 0;
		if (eth_mac_ctrl) {
			// ebtables -A macfilter -i eth0+ -j DROP
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -j DROP", (char *)FW_MACFILTER_FWD, (char *)ELANIF);
		}
#ifdef WLAN_SUPPORT
		if (wlan_mac_ctrl) {
			// ebtables -A macfilter -i wlan0+ -j DROP
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -j DROP", (char *)FW_MACFILTER_FWD, (char *)WLAN_DEV_PREFIX);
		}
#endif
		if (eth_mac_ctrl) {
			// ebtables -A macfilter -i eth0+ -j DROP
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -j DROP", (char *)FW_MACFILTER_LI, (char *)ELANIF);
		}
#ifdef WLAN_SUPPORT
		if (wlan_mac_ctrl) {
			// ebtables -A macfilter -i wlan0+ -j DROP
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -j DROP", (char *)FW_MACFILTER_LI, (char *)WLAN_DEV_PREFIX);
		}
#endif
		/* fix wan interface */
		va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_MACFILTER_FWD, "-i", "eth0+", "-j", (char *)FW_DROP);
	}

	if(mac_in_dft == 0) { // DROP
		mac_filter_in_permit = 0;
		// ebtables -A macfilter -o eth+ -j DROP
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -o %s+ -j DROP", (char *)FW_MACFILTER_FWD, (char *)ELANIF);
#ifdef WLAN_SUPPORT
		// ebtables -A macfilter -o wlan+ -j DROP
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -o %s+ -j DROP", (char *)FW_MACFILTER_FWD, (char *)WLAN_DEV_PREFIX);
#endif
		// ebtables -A macfilter -o eth+ -j DROP
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -o %s+ -j DROP", (char *)FW_MACFILTER_LO, (char *)ELANIF);
#ifdef WLAN_SUPPORT
		// ebtables -A macfilter -o wlan+ -j DROP
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -o %s+ -j DROP", (char *)FW_MACFILTER_LO, (char *)WLAN_DEV_PREFIX);
#endif
	}

	//Local in/out default rules
	unsigned char tmpBuf[100]={0};
	unsigned char value[6];
#ifdef CONFIG_IPV6
	mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)tmpBuf, sizeof(tmpBuf));
	// ebtables -A macfilter -p IPv6 --ip6-dst fe80::1 -j ACCEPT
	if(total_out || mac_out_dft == 0)
	{
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst %s -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);

		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type echo-request -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type echo-reply -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type router-solicitation -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type router-advertisement -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-solicitation -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-advertisement -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst ff02::1:ff00:0000/ffff:ffff:ffff:ffff:ffff:ffff:ff00:0 --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-solicitation -j ACCEPT", (char *)FW_MACFILTER_LI);
	}

	if(total_in || mac_in_dft == 0)
	{
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-src %s -j ACCEPT", (char *)FW_MACFILTER_LO, tmpBuf);

		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-src %s --ip6-proto ipv6-icmp --ip6-icmp-type echo-request -j ACCEPT", (char *)FW_MACFILTER_LO, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-src %s --ip6-proto ipv6-icmp --ip6-icmp-type echo-reply -j ACCEPT", (char *)FW_MACFILTER_LO, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-src %s --ip6-proto ipv6-icmp --ip6-icmp-type router-solicitation -j ACCEPT", (char *)FW_MACFILTER_LO, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-src %s --ip6-proto ipv6-icmp --ip6-icmp-type router-advertisement -j ACCEPT", (char *)FW_MACFILTER_LO, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-src %s --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-solicitation -j ACCEPT", (char *)FW_MACFILTER_LO, tmpBuf);
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-src %s --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-advertisement -j ACCEPT", (char *)FW_MACFILTER_LO, tmpBuf);
	}
#ifdef CONFIG_SECONDARY_IPV6
	memset(tmpBuf,0,sizeof(tmpBuf));
	mib_get_s(MIB_IPV6_LAN_IP_ADDR2, (void *)tmpBuf, sizeof(tmpBuf));
	if (strlen(tmpBuf)!=0)
	{
		if(total_out || mac_out_dft == 0)
		{
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst %s -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type echo-request -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type echo-reply -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type router-solicitation -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type router-advertisement -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-solicitation -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-advertisement -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
		}

		if(total_in || mac_in_dft == 0)
		{
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-src %s -j ACCEPT", (char *)FW_MACFILTER_LO, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-src %s --ip6-proto ipv6-icmp --ip6-icmp-type echo-request -j ACCEPT", (char *)FW_MACFILTER_LO, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-src %s --ip6-proto ipv6-icmp --ip6-icmp-type echo-reply -j ACCEPT", (char *)FW_MACFILTER_LO, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-src %s --ip6-proto ipv6-icmp --ip6-icmp-type router-solicitation -j ACCEPT", (char *)FW_MACFILTER_LO, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-src %s --ip6-proto ipv6-icmp --ip6-icmp-type router-advertisement -j ACCEPT", (char *)FW_MACFILTER_LO, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-src %s --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-solicitation -j ACCEPT", (char *)FW_MACFILTER_LO, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv6 --ip6-src %s --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-advertisement -j ACCEPT", (char *)FW_MACFILTER_LO, tmpBuf);
		}
	}
#endif
#endif
	mib_get_s(MIB_ADSL_LAN_IP, (void *)tmpBuf, sizeof(tmpBuf));
#ifdef CONFIG_TELMEX
	if(total_out || mac_out_dft == 0)
	{
	// ebtables -A macfilter_local_in -i eth0+ -p ipv4 --ip-proto udp --ip-dport 67:68 -j ACCEPT/DROP
	DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -p ipv4 --ip-proto udp --ip-dport 67:68 -j RETURN", (char *)FW_MACFILTER_LI, (char *)ELANIF);
	// ebtables -A macfilter_local_in -i wlan+ -p ipv4 --ip-proto udp --ip-dport 67:68 -j ACCEPT/DROP
	DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -p ipv4 --ip-proto udp --ip-dport 67:68 -j RETURN", (char *)FW_MACFILTER_LI, (char *)WLAN_DEV_PREFIX);
	}
#endif
	if(total_out || mac_out_dft == 0) DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv4 --ip-dst %s -j ACCEPT", (char *)FW_MACFILTER_LI, inet_ntoa(*((struct in_addr *)tmpBuf)));
	if(total_in || mac_in_dft == 0) DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv4 --ip-src %s -j ACCEPT", (char *)FW_MACFILTER_LO, inet_ntoa(*((struct in_addr *)tmpBuf)));
#ifdef CONFIG_SECONDARY_IP
	mib_get_s(MIB_ADSL_LAN_ENABLE_IP2, (void *)value, sizeof(value));
	if (value[0] == 1) {
		mib_get_s(MIB_ADSL_LAN_IP2, (void *)tmpBuf, sizeof(tmpBuf));
		if(total_out || mac_out_dft == 0) DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv4 --ip-dst %s -j ACCEPT", (char *)FW_MACFILTER_LI, inet_ntoa(*((struct in_addr *)tmpBuf)));
		if(total_in || mac_in_dft == 0) DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv4 --ip-src %s -j ACCEPT", (char *)FW_MACFILTER_LO, inet_ntoa(*((struct in_addr *)tmpBuf)));
	}
#endif
	if(total_out || mac_out_dft == 0) DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p ARP -d ff:ff:ff:ff:ff:ff -j ACCEPT", (char *)FW_MACFILTER_LI);
	mib_get(MIB_ELAN_MAC_ADDR, (void *)value);
	if(total_out || mac_out_dft == 0) DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p ARP -d %02x:%02x:%02x:%02x:%02x:%02x -j ACCEPT", (char *)FW_MACFILTER_LI, value[0], value[1], value[2], value[3], value[4], value[5]);
	if(total_in || mac_in_dft == 0) DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p ARP -s %02x:%02x:%02x:%02x:%02x:%02x -j ACCEPT", (char *)FW_MACFILTER_LO, value[0], value[1], value[2], value[3], value[4], value[5]);

	//Kevin, clear bridge fastpath table
	TRACE(STA_SCRIPT,"/bin/echo 2 > /proc/fastbridge\n");
	system("/bin/echo 2 > /proc/fastbridge");
#if defined(CONFIG_HWNAT)
	TRACE(STA_SCRIPT,"/bin/echo clean_L2 > /proc/rtl865x/acl_mode\n");
	system("/bin/echo clean_L2 > /proc/rtl865x/acl_mode");
#if defined(CONFIG_RTL8676_Static_ACL)
	if( mac_filter_in_permit==1 && mac_filter_out_permit==1 )
	{
		TRACE(STA_SCRIPT,"/bin/echo all_permit> /proc/rtl865x/acl_mac_filter_mode\n");
		system("/bin/echo all_permit> /proc/rtl865x/acl_mac_filter_mode");
	}
	else if( mac_filter_in_permit==0 && mac_filter_out_permit==1 )
	{
		TRACE(STA_SCRIPT,"/bin/echo in_drop_out_permit> /proc/rtl865x/acl_mac_filter_mode\n");
		system("/bin/echo in_drop_out_permit> /proc/rtl865x/acl_mac_filter_mode");
	}
	else if( mac_filter_in_permit==1 && mac_filter_out_permit==0 )
	{
		TRACE(STA_SCRIPT,"/bin/echo in_permit_out_drop> /proc/rtl865x/acl_mac_filter_mode\n");
		system("/bin/echo in_permit_out_drop> /proc/rtl865x/acl_mac_filter_mode");
	}
	else if( mac_filter_in_permit==0 && mac_filter_out_permit==0 )
	{
		TRACE(STA_SCRIPT,"/bin/echo all_drop> /proc/rtl865x/acl_mac_filter_mode\n");
		system("/bin/echo all_drop> /proc/rtl865x/acl_mac_filter_mode");
	}
#endif
#endif
	rtk_hwnat_flow_flush();
	return 0;
}
#endif

#ifdef IP_PORT_FILTER
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
int setupIPFilter(void)
{

	char *argv[32]={0}, *ifname;
	unsigned char value[32], byte;
	int vInt, i, total;
	MIB_CE_IP_PORT_FILTER_T IpEntry;
	char *policy, *filterSIP = NULL, *filterDIP = NULL, srcPortRange[12], dstPortRange[12];
	char  srcip[20], dstip[20], old_srcip[20], old_dstip[20];
	// jim support ip range options " iptables -m iprange --src-range xx.xx.xx.xx-yy.yy.yy.yy --dst-range a.a.a.a-b.b.b.b ..."
	char srcip2[55]={0}, dstip2[55]={0};
	char SIPRange[110]={0};
	char DIPRange[110]={0};
	char *filterSIPRange=NULL;
	char *filterDIPRange=NULL;
	filterSIP=filterDIP=NULL;
	unsigned char in_enable = 0, out_enable = 0;
	unsigned char in_action = 1, out_action = 1;	//0: white list, 1: black list
	unsigned char ipf_drop_wan_packet_enable = 0;
#ifdef CONFIG_USER_CONNTRACK_TOOLS
	char *conn_argv[20];
	char  conn_srcip[20]={0}, conn_dstip[20]={0}, conn_srcmask[20]={0}, conn_dstmask[20]={0};
#endif
#ifdef SUPPORT_TIME_SCHEDULE
	time_t curTime;
	struct tm curTm;
	int shouldSchedule = 0;
	unsigned int isScheduled;
	MIB_CE_TIME_SCHEDULE_T schdEntry;
	int totalSched=0;

	totalSched = mib_chain_total(MIB_TIME_SCHEDULE_TBL);
	time(&curTime);
	memset(schdIpFilterIdx, 0, sizeof(schdIpFilterIdx));
	memcpy(&curTm, localtime(&curTime), sizeof(curTm));
#endif

	// Delete ipfilter rules
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_IPFILTER_IN);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_IPFILTER_OUT);

	mib_get_s(MIB_IPFILTER_IN_ENABLE, (void*)&in_enable, sizeof(in_enable));
	mib_get_s(MIB_IPFILTER_OUT_ENABLE, (void*)&out_enable, sizeof(out_enable));
	mib_get_s(MIB_IPF_IN_ACTION, (void*)&in_action, sizeof(in_action));
	mib_get_s(MIB_IPF_OUT_ACTION, (void*)&out_action, sizeof(out_action));
	mib_get_s(MIB_IPF_DROP_WAN_PACKET_ENABLE, (void*)&ipf_drop_wan_packet_enable, sizeof(ipf_drop_wan_packet_enable));

	va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_IPFILTER_IN,
		"!", (char *)ARG_I, LANIF_ALL, "-m", "mark", "--mark", RMACC_MARK, "-j", FW_ACCEPT);

	// drop wan to wan forward packets
	// ex: (before interface route is created) dhcp offers from wan will be frowarded to pppoe wan (default route)
	// iptables -A ipfilter ! -i br0 ! -o br0 -j DROP
	va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_IPFILTER_IN, "!", ARG_I, LANIF_ALL
		,"!", ARG_O, LANIF_ALL, "-j", (char *)FW_DROP);

	// iptables -A ipfilter_in -d 224.0.0.0/4 -j RETURN
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_IPFILTER_IN, "-d",
		"224.0.0.0/4", "-j", (char *)FW_RETURN);

	// iptables -A ipfilter_out -d 224.0.0.0/4 -j RETURN
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_IPFILTER_OUT, "-d",
		"224.0.0.0/4", "-j", (char *)FW_RETURN);

#ifdef CONFIG_USER_VXLAN
	//iptables -I ipfilter_in -i vxlan+ -j RETURN
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_IPFILTER_IN, "-i", "vxlan+", "-j", (char *)FW_RETURN);
#endif

	total = mib_chain_total(MIB_IP_PORT_FILTER_TBL);
	for (i = 0; i < total; i++)
	{
		int idx = 0;
		int udp_tcp_idx=-1;
#ifdef CONFIG_USER_CONNTRACK_TOOLS
		int conn_idx=1; // conn_argv[0]="/bin/conntrack"
		int conn_udp_tcp_idx=-1;
#endif

		if (!mib_chain_get(MIB_IP_PORT_FILTER_TBL, i, (void *)&IpEntry))
			continue;
#ifdef SUPPORT_TIME_SCHEDULE
		if(IpEntry.schedIndex >= 1 && IpEntry.schedIndex <= totalSched
#ifdef CONFIG_CMCC_ENTERPRISE
			&& IpEntry.changed == 0
#endif
		)
		{
			if (!mib_chain_get(MIB_TIME_SCHEDULE_TBL, IpEntry.schedIndex-1, (void *)&schdEntry))
			{
				continue;
			}
			shouldSchedule = check_should_schedule(&curTm, &schdEntry);
			isScheduled = isScheduledIndex(i, schdIpFilterIdx, SCHD_IP_FILTER_SIZE);
			if((shouldSchedule && (isScheduled == 0)) ||
				((shouldSchedule == 0) && isScheduled))
			{
				if(shouldSchedule && (isScheduled == 0))
				{
					//not scheduled yet, should add rule
					addScheduledIndex(i, schdIpFilterIdx, SCHD_IP_FILTER_SIZE);
				}
				else
				{
					//scheduled but is out of schedule time, should delete rule
					delScheduledIndex(i, schdIpFilterIdx, SCHD_IP_FILTER_SIZE);
					continue;
				}
			}
			else
			{
				continue;
			}
		}

#endif
		if(IpEntry.enable == 0)
			continue;

		if(IpEntry.dir == DIR_IN && !in_enable)
			continue;

		if(IpEntry.dir == DIR_OUT && !out_enable)
			continue;

#ifdef CONFIG_IPV6
		if(IpEntry.IpProtocol == IPVER_IPV6)
			continue;
#endif

		if ((IpEntry.dir == DIR_IN && in_action == 1) || (IpEntry.dir == DIR_OUT && out_action == 1))
			policy = (char *)FW_DROP;
		else
			policy = (char *)FW_RETURN;

		// source port
		if (IpEntry.srcPortFrom == 0)
			srcPortRange[0]='\0';
		else if (IpEntry.srcPortTo == 0)
			snprintf(srcPortRange, 12, "%u", IpEntry.srcPortFrom);
		else
			snprintf(srcPortRange, 12, "%u:%u",
				IpEntry.srcPortFrom, IpEntry.srcPortTo);

		// destination port
		if (IpEntry.dstPortFrom == 0)
			dstPortRange[0]='\0';
		else if (IpEntry.dstPortTo == 0)
			snprintf(dstPortRange, 12, "%u", IpEntry.dstPortFrom);
		else
			snprintf(dstPortRange, 12, "%u:%u",
				IpEntry.dstPortFrom, IpEntry.dstPortTo);

		// source ip, mask
		if(IpEntry.srcIp2[0] == 0)    // normal ip filter, no iprange supported
		{
			filterSIPRange = 0;
			strncpy(srcip, inet_ntoa(*((struct in_addr *)IpEntry.srcIp)), 16);
			if (strcmp(srcip, ARG_0x4) == 0)
				filterSIP = 0;
			else {
				if (IpEntry.smaskbit!=0) {
					strncpy( old_srcip, srcip, sizeof(srcip));
					snprintf(srcip, 20, "%s/%d", old_srcip, IpEntry.smaskbit);
				}
				filterSIP = srcip;
			}
		}
		else
		{
			strncpy(srcip, inet_ntoa(*((struct in_addr *)IpEntry.srcIp)), 16);
			strncpy(srcip2, inet_ntoa(*((struct in_addr *)IpEntry.srcIp2)),16);

			if(strcmp(srcip, ARG_0x4) ==0 || strcmp(srcip2, ARG_0x4) ==0)
				filterSIPRange=0;
			else
			{
				snprintf(SIPRange, 40, "%s-%s", srcip, srcip2);
				filterSIPRange=SIPRange;
			}
		}

		// destination ip, mask
		if(IpEntry.dstIp2[0] == 0)    // normal ip filter, no iprange supported
		{
			filterDIPRange = 0;
			strncpy(dstip, inet_ntoa(*((struct in_addr *)IpEntry.dstIp)), 16);
			if (strcmp(dstip, ARG_0x4) == 0)
				filterDIP = 0;
			else {
				if (IpEntry.dmaskbit!=0) {
					strncpy( old_dstip, dstip, sizeof(dstip));
					snprintf(dstip, 20, "%s/%d", old_dstip, IpEntry.dmaskbit);
				}
				filterDIP = dstip;
			}
		}
		else
		{
			strncpy(dstip, inet_ntoa(*((struct in_addr *)IpEntry.dstIp)), 16);
			strncpy(dstip2, inet_ntoa(*((struct in_addr *)IpEntry.dstIp2)),16);

			if(strcmp(dstip, ARG_0x4) ==0 || strcmp(dstip2, ARG_0x4) ==0)
				filterDIPRange=0;
			else
			{
				snprintf(DIPRange, 40, "%s-%s", dstip, dstip2);
				filterDIPRange=DIPRange;
			}
		}

		argv[idx++] = (char *)IPTABLES;
		argv[idx++] = (char *)FW_ADD;
		if(IpEntry.dir == DIR_IN)
			argv[idx++] = (char *)FW_IPFILTER_IN;
		else
			argv[idx++] = (char *)FW_IPFILTER_OUT;

		if(IpEntry.dir == DIR_IN){
			if(strlen(IpEntry.WanPath))
			{
				argv[idx++] = "-i";
				argv[idx++] = IpEntry.WanPath;
			}
			else
			{
				ifname = getInternetWanIfName();
				if(strcmp(ifname, "None")!=0)
				{
					argv[idx++] = "-i";
					argv[idx++] = ifname;
				}
			}
		}

#ifdef CONFIG_USER_CONNTRACK_TOOLS
		conn_argv[conn_idx++]="-D"; // delete action
#endif

		// protocol
		if (IpEntry.protoType != PROTO_NONE) {
			argv[idx++] = "-p";
#ifdef CONFIG_USER_CONNTRACK_TOOLS
			conn_argv[conn_idx++]="-p";	// specific proto
			if (IpEntry.protoType == PROTO_TCP)
				conn_argv[conn_idx++]=(char *)ARG_TCP;	// tcp
			else if (IpEntry.protoType == PROTO_UDP)
				conn_argv[conn_idx++]=(char *)ARG_UDP;	// tcp
			else if (IpEntry.protoType == PROTO_ICMP)
				conn_argv[conn_idx++]=(char *)ARG_ICMP;	// tcp
			else if(IpEntry.protoType == PROTO_UDPTCP){
				//add udp rule first for udp/tcp protocol
				conn_udp_tcp_idx = conn_idx;
				conn_argv[conn_idx++]=(char *)ARG_UDP;
			}
#endif

			if (IpEntry.protoType == PROTO_TCP)
				argv[idx++] = (char *)ARG_TCP;
			else if (IpEntry.protoType == PROTO_UDP)
				argv[idx++] = (char *)ARG_UDP;
			else if (IpEntry.protoType == PROTO_ICMP)
				argv[idx++] = (char *)ARG_ICMP;
			else if(IpEntry.protoType == PROTO_UDPTCP){
				//add udp rule first for udp/tcp protocol
				udp_tcp_idx = idx;
				argv[idx++] = (char *)ARG_UDP;
			}
		}

		// src ip
		if (filterSIP != 0)
		{
			argv[idx++] = "-s";
			argv[idx++] = filterSIP;
#ifdef CONFIG_USER_CONNTRACK_TOOLS
			conn_argv[conn_idx++]="-s"; // source ip
			strncpy(conn_srcip, inet_ntoa(*((struct in_addr *)IpEntry.srcIp)), sizeof(conn_srcip));
			conn_argv[conn_idx++]=conn_srcip;
			if (IpEntry.smaskbit!=0 && IpEntry.smaskbit <=32){
				conn_argv[conn_idx++]="--mask-src"; // src mask with ip format
				strncpy(conn_srcmask, mask2string(IpEntry.smaskbit), sizeof(conn_srcmask));
				conn_argv[conn_idx++]=conn_srcmask;
			}
#endif
		}

		// src port
		if ((IpEntry.protoType==PROTO_TCP ||
			IpEntry.protoType==PROTO_UDP ||
			IpEntry.protoType==PROTO_UDPTCP) &&
			srcPortRange[0] != 0) {
			argv[idx++] = (char *)FW_SPORT;
			argv[idx++] = srcPortRange;
		}

		// dst ip
		if (filterDIP != 0)
		{
			argv[idx++] = "-d";
			argv[idx++] = filterDIP;
#ifdef CONFIG_USER_CONNTRACK_TOOLS
			conn_argv[conn_idx++]="-d"; // dst ip
			strncpy(conn_dstip, inet_ntoa(*((struct in_addr *)IpEntry.dstIp)), sizeof(conn_dstip));
			conn_argv[conn_idx++]=conn_dstip;
			if (IpEntry.dmaskbit!=0 && IpEntry.dmaskbit <=32){
				conn_argv[conn_idx++]="--mask-dst"; // dst mask with ip format
				strncpy(conn_dstmask, mask2string(IpEntry.dmaskbit), sizeof(conn_dstmask));
				conn_argv[conn_idx++]=conn_dstmask;
			}
#endif
		}
		// dst port
		if ((IpEntry.protoType==PROTO_TCP ||
			IpEntry.protoType==PROTO_UDP ||
			IpEntry.protoType==PROTO_UDPTCP) &&
			dstPortRange[0] != 0) {
			argv[idx++] = (char *)FW_DPORT;
			argv[idx++] = dstPortRange;
		}

		if(filterSIPRange || filterDIPRange)
		{
			argv[idx++] = strdup("-m");
			argv[idx++] = strdup("iprange");
			if(filterSIPRange)
			{
				argv[idx++] = strdup("--src-range");
				argv[idx++] = strdup(filterSIPRange);
			}
			if(filterDIPRange)
			{
				argv[idx++] = strdup("--dst-range");
				argv[idx++] = strdup(filterDIPRange);
			}
		}

		// target/jump
		argv[idx++] = "-j";
		argv[idx++] = policy;
		argv[idx++] = NULL;

		do_cmd(IPTABLES, argv, 1);
		if(udp_tcp_idx!=-1){
			//add tcp rule for udp/tcp protocol
			argv[udp_tcp_idx] = (char *)ARG_TCP;
			do_cmd(IPTABLES, argv, 1);
		}

#ifdef CONFIG_USER_CONNTRACK_TOOLS
		//flush ip filter
		conn_argv[conn_idx++] = NULL;
		do_cmd(CONNTRACK_TOOL, conn_argv, 1);
		if(conn_udp_tcp_idx!=-1){
			//add tcp rule for udp/tcp protocol
			argv[conn_udp_tcp_idx] = (char *)ARG_TCP;
			do_cmd(CONNTRACK_TOOL, argv, 1);
		}
#endif
	}

	// accept related
//#if 0
	// iptables -A ipfilter_in -m state --state ESTABLISHED,RELATED -j RETURN
	if(ipf_drop_wan_packet_enable == 1)
		va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_IPFILTER_IN, "-m", "state",
		"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_RETURN);
//#endif

	// iptables -A ipfilter_out -m state --state ESTABLISHED,RELATED -j RETURN
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_IPFILTER_OUT, "-m", "state",
		"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_RETURN);

#ifdef _PRMT_C_CU_USERACCOUNT_
	int k = 0, user_account_ipfilter_cnt = 0;
	USER_ACCOUNT_IP_T ipEntry;
	//iptables -t filter -A ipfilter_in -s 172.29.38.5 -j DROP
	char *argv_in[8] = {0};
	char *argv_out[8]={0};
	argv_in[0] = argv_out[0] =  (char *)IPTABLES;
	argv_in[1] = argv_out[1] =  (char *)FW_ADD;
	argv_in[2] = strdup("ipfilter_in");
	argv_in[3] = strdup("-s");
	argv_out[2] = strdup("ipfilter_out");
	argv_out[3] = strdup("-d");
	argv_in[5] = argv_out[5] =  strdup("-j");
	argv_in[6] = argv_out[6] =  strdup("DROP");
	argv_in[7] = argv_out[7] = NULL;

	user_account_ipfilter_cnt = mib_chain_total(CWMP_USER_ACCOUNT_IP);
	for(k = 0; k < user_account_ipfilter_cnt; k++)
	{
		if(!mib_chain_get(CWMP_USER_ACCOUNT_IP,k,&ipEntry))
			continue;
		if(ipEntry.ipaddr == 0)
			continue;
		argv_in[4] = argv_out[4] = strdup(inet_ntoa(*(struct in_addr*)&ipEntry.ipaddr));

		do_cmd(IPTABLES, argv_in, 1);
		do_cmd(IPTABLES, argv_out, 1);
	}
#endif



	rtk_hwnat_flow_flush();

	// Kill all conntrack (to kill the established conntrack when change iptables rules)
#ifndef CONFIG_USER_CONNTRACK_TOOLS
	va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
#endif
	return 1;
}
#else
int setupIPFilter(void)
{
	char *argv[20];
	unsigned char value[32], byte;
	int vInt, i, total, len;
	MIB_CE_IP_PORT_FILTER_T IpEntry;
	MIB_CE_ATM_VC_T vcEntry;
	char *policy, *filterSIP, *filterDIP, srcPortRange[12], dstPortRange[12];
	char  srcip[20], dstip[20];
	char wanname[IFNAMSIZ];
	uint16 etherval;
#ifdef IP_RANGE_FILTER_SUPPORT
	// jim support ip range options " iptables -m iprange --src-range xx.xx.xx.xx-yy.yy.yy.yy --dst-range a.a.a.a-b.b.b.b ..."
	//char srcip2[20]={0}, dstip2[20]={0};
	char srcip2[55]={0}, dstip2[55]={0};
	//char SIPRange[40]={0};
	char SIPRange[110]={0};
	//char DIPRange[40]={0};
	char DIPRange[110]={0};
	char *filterSIPRange=NULL;
	char *filterDIPRange=NULL;
	filterSIP=filterDIP=NULL;
#endif
#if defined(CONFIG_E8B) ||defined(CONFIG_00R0)
	unsigned char ipportfilter_state=0;
	char ipfilter_spi_state=1;
#endif
#ifdef SUPPORT_TIME_SCHEDULE
	int totalSched;
#endif
	// Delete ipfilter rule
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_IPFILTER);

	// clear ether rtk acl
	ethertype_acl_clear();

#if defined(CONFIG_USER_PPTP_SERVER)
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_PPTP_VPN_FW);
	setup_pptp_server_firewall(PPTPS_RULE_TYPE_FW);
	va_cmd(IPTABLES, 4, 1, "-A", (char *)FW_IPFILTER, "-j", (char *)IPTABLE_PPTP_VPN_FW);
#endif
#if defined(CONFIG_USER_L2TPD_LNS)
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_L2TP_VPN_FW);
	va_cmd(IPTABLES, 4, 1, "-A", (char *)FW_IPFILTER, "-j", (char *)IPTABLE_L2TP_VPN_FW);
#endif

#ifdef SUPPORT_TIME_SCHEDULE
	memset(schdIpFilterIdx, 0, sizeof(schdIpFilterIdx));
	totalSched = mib_chain_total(MIB_TIME_SCHEDULE_TBL);
#endif
	// packet filtering
	// ip filtering

	// Add chain for ip filtering
	// iptables -N ipfilter
	//va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_IPFILTER);

	// drop wan to wan forward packets
	// ex: (before interface route is created) dhcp offers from wan will be frowarded to pppoe wan (default route)
	// iptables -A ipfilter ! -i br0 ! -o br0 -j DROP
	va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_IPFILTER, "!", ARG_I, LANIF_ALL
			,"!", ARG_O, LANIF_ALL, "-j", (char *)FW_DROP);

	// iptables -A ipfilter -d 224.0.0.0/4 -j RETURN
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_IPFILTER, "-d",
			"224.0.0.0/4", "-j", (char *)FW_RETURN);

#if defined(CONFIG_E8B) ||defined(CONFIG_00R0)
	mib_get_s(MIB_IPFILTER_ON_OFF, (void*)&ipportfilter_state, sizeof(ipportfilter_state));

	if(ipportfilter_state == 0) return 0;
#endif

	total = mib_chain_total(MIB_IP_PORT_FILTER_TBL);

#ifdef CONFIG_00R0 //change to black list if these have no any rule
	if(total < 1) {
		byte = 1;
		mib_set(MIB_IPF_OUT_ACTION, (void *)&byte);
		goto IPFILTER_END;
	}
#endif

	for (i = 0; i < total; i++)
	{
		int idx=0;
		unsigned int ether_ret = 0;
#ifdef CONFIG_E8B
		int udp_tcp_idx=-1;
#endif
		MIB_CE_IP_PORT_FILTER_T UpEntry;

		memset(&UpEntry, '\0', sizeof(UpEntry));
		/*
		 *	srcPortRange: src port
		 *	dstPortRange: dst port
		 */
		if (!mib_chain_get(MIB_IP_PORT_FILTER_TBL, i, (void *)&IpEntry))
			return -1;

#ifdef SUPPORT_TIME_SCHEDULE
		if(IpEntry.schedIndex >= 1 && IpEntry.schedIndex <= totalSched)
		{
			continue;
		}
#endif
#ifdef CONFIG_TELMEX_DEV
		if (!IpEntry.enable)
			continue;
#endif
#ifdef CONFIG_E8B
		if (IpEntry.IpProtocol != IPVER_IPV4) // 1: IPv4, 2:IPv6
			continue;
#endif

		if (IpEntry.etherType == ETHER_NONE) {
			if (IpEntry.action == 0)
				policy = (char *)FW_DROP;
			else
				policy = (char *)FW_RETURN;

			// source port
			if (IpEntry.srcPortFrom == 0)
				srcPortRange[0]='\0';
			else if (IpEntry.srcPortFrom == IpEntry.srcPortTo)
				snprintf(srcPortRange, 12, "%u", IpEntry.srcPortFrom);
			else
				snprintf(srcPortRange, 12, "%u:%u",
						IpEntry.srcPortFrom, IpEntry.srcPortTo);

			// destination port
			if (IpEntry.dstPortFrom == 0)
				dstPortRange[0]='\0';
			else if (IpEntry.dstPortFrom == IpEntry.dstPortTo)
				snprintf(dstPortRange, 12, "%u", IpEntry.dstPortFrom);
			else
				snprintf(dstPortRange, 12, "%u:%u",
						IpEntry.dstPortFrom, IpEntry.dstPortTo);

			// source ip, mask
#ifdef IP_RANGE_FILTER_SUPPORT
			if(IpEntry.srcIp2[0] == 0)	  // normal ip filter, no iprange supported
			{
				filterSIPRange=0;
#endif
				strncpy(srcip, inet_ntoa(*((struct in_addr *)IpEntry.srcIp)), 16);
				if (strcmp(srcip, ARG_0x4) == 0)
					filterSIP = 0;
				else {
					if (IpEntry.smaskbit!=0) {
						len = sizeof(srcip) - strlen(srcip);
						snprintf(srcip + strlen(srcip), len, "/%d", IpEntry.smaskbit);
					}
					filterSIP = srcip;
				}
#ifdef IP_RANGE_FILTER_SUPPORT
			}
			else
			{
				strncpy(srcip, inet_ntoa(*((struct in_addr *)IpEntry.srcIp)), 16);
				strncpy(srcip2, inet_ntoa(*((struct in_addr *)IpEntry.srcIp2)),16);

				if(strcmp(srcip, ARG_0x4) ==0 || strcmp(srcip2, ARG_0x4) ==0)
					filterSIPRange=0;
				else
				{
					snprintf(SIPRange, 40, "%s-%s", srcip, srcip2);
					filterSIPRange=SIPRange;
				}
			}
#endif
			// destination ip, mask
#ifdef IP_RANGE_FILTER_SUPPORT
			if(IpEntry.dstIp2[0] == 0)	  // normal ip filter, no iprange supported
			{
				filterDIPRange=0;
#endif
				strncpy(dstip, inet_ntoa(*((struct in_addr *)IpEntry.dstIp)), 16);
				if (strcmp(dstip, ARG_0x4) == 0)
					filterDIP = 0;
				else {
					if (IpEntry.dmaskbit!=0) {
						len = sizeof(dstip) - strlen(dstip);
						snprintf(dstip + strlen(dstip), len, "/%d", IpEntry.dmaskbit);
					}
					filterDIP = dstip;
				}
#ifdef IP_RANGE_FILTER_SUPPORT
			}
			else
			{
				strncpy(dstip, inet_ntoa(*((struct in_addr *)IpEntry.dstIp)), 16);
				strncpy(dstip2, inet_ntoa(*((struct in_addr *)IpEntry.dstIp2)),16);

				if(strcmp(dstip, ARG_0x4) ==0 || strcmp(dstip2, ARG_0x4) ==0)
					filterDIPRange=0;
				else
				{
					snprintf(DIPRange, 40, "%s-%s", dstip, dstip2);
					filterDIPRange=DIPRange;
				}
			}
#endif
			// interface
			argv[1] = (char *)FW_ADD;
			argv[2] = (char *)FW_IPFILTER;

			idx = 3;

			if (IpEntry.dir == DIR_IN){
				if(IpEntry.ifIndex == DUMMY_IFINDEX){
					argv[idx++] = "!";
					argv[idx++] = (char *)ARG_I;
					argv[idx++] = (char *)LANIF;
				}
				else{
					getWanEntrybyindex(&vcEntry,IpEntry.ifIndex);
					ifGetName(IpEntry.ifIndex, wanname, sizeof(wanname));
					if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
					{
						argv[idx++] = "-m";
						argv[idx++] = "physdev";
						argv[idx++] = "--physdev-in";
					}
					else
					{
						argv[idx++] = (char *)ARG_I;
					}
					argv[idx++] = wanname;
				}
			} else{
				argv[idx++] = (char *)ARG_I;
				argv[idx++] = (char *)LANIF;
			}

			// protocol
			if (IpEntry.protoType != PROTO_NONE) {
				argv[idx++] = "-p";
				if (IpEntry.protoType == PROTO_TCP)
					argv[idx++] = (char *)ARG_TCP;
				else if (IpEntry.protoType == PROTO_UDP)
					argv[idx++] = (char *)ARG_UDP;
#ifdef CONFIG_E8B
				else if (IpEntry.protoType == PROTO_ICMP)
					argv[idx++] = (char *)ARG_ICMP;
				else if(IpEntry.protoType == PROTO_UDPTCP){
					//add udp rule first for udp/tcp protocol
					udp_tcp_idx = idx;
					argv[idx++] = (char *)ARG_UDP;
				}
#else
				else //if (IpEntry.protoType == PROTO_ICMP)
					argv[idx++] = (char *)ARG_ICMP;
#endif
			}

			// src ip
			if (filterSIP != 0)
			{
				argv[idx++] = "-s";
				argv[idx++] = filterSIP;
			}

			// src port
			if ((IpEntry.protoType==PROTO_TCP ||
						IpEntry.protoType==PROTO_UDP
#ifdef CONFIG_E8B
						|| IpEntry.protoType==PROTO_UDPTCP
#endif
				) &&
					srcPortRange[0] != 0) {
				argv[idx++] = (char *)FW_SPORT;
				argv[idx++] = srcPortRange;
			}

			// dst ip
			if (filterDIP != 0)
			{
				argv[idx++] = "-d";
				argv[idx++] = filterDIP;
			}
			// dst port
			if ((IpEntry.protoType==PROTO_TCP ||
						IpEntry.protoType==PROTO_UDP
#ifdef CONFIG_E8B
						|| IpEntry.protoType==PROTO_UDPTCP
#endif
				) &&
					dstPortRange[0] != 0) {
				argv[idx++] = (char *)FW_DPORT;
				argv[idx++] = dstPortRange;
			}

#ifdef IP_RANGE_FILTER_SUPPORT
			if(filterSIPRange || filterDIPRange)
			{
				argv[idx++] = strdup("-m");
				argv[idx++] = strdup("iprange");
				if(filterSIPRange)
				{
					argv[idx++] = strdup("--src-range");
					argv[idx++] = strdup(filterSIPRange);
				}
				if(filterDIPRange)
				{
					argv[idx++] = strdup("--dst-range");
					argv[idx++] = strdup(filterDIPRange);
				}
			}
#endif
			// target/jump
			argv[idx++] = "-j";
			argv[idx++] = policy;
			argv[idx++] = NULL;

			TRACE(STA_SCRIPT, "%s %s %s %s %s %s %s ...\n", IPTABLES, argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
			do_cmd(IPTABLES, argv, 1);
		} else {
			if (IpEntry.etherType != ETHER_NONE) {
				if (IpEntry.etherType == ETHER_IPV4)
					etherval = 0x0800;
				else if (IpEntry.etherType == ETHER_IPV6)
					etherval = 0x86dd;
				else if (IpEntry.etherType == ETHER_ARP)
					etherval = 0x0806;
				else if (IpEntry.etherType == ETHER_PPPOE)
					etherval = 0x8863;
				else if (IpEntry.etherType == ETHER_WAKELAN)
					etherval = 0x0842;
				else if (IpEntry.etherType == ETHER_APPLETALK)
					etherval = 0x809B;
				else if (IpEntry.etherType == ETHER_PROFINET)
					etherval = 0x8892;
				else if (IpEntry.etherType == ETHER_FLOWCONTROL)
					etherval = 0x8808;
				else if (IpEntry.etherType == ETHER_IPX)
					etherval = 0x8137;
				else if (IpEntry.etherType == ETHER_MPLS_UNI)
					etherval = 0x8847;
				else if (IpEntry.etherType == ETHER_MPLS_MULTI)
					etherval = 0x8848;
				else if (IpEntry.etherType == ETHER_802_1X)
					etherval = 0x888E;
				else if (IpEntry.etherType == ETHER_LLDP)
					etherval = 0x88CC;
				else if (IpEntry.etherType == ETHER_RARP)
					etherval = 0x8035;
				else if (IpEntry.etherType == ETHER_NETBIOS)
					etherval = 0x8191;
				else if (IpEntry.etherType == ETHER_X25)
					etherval = 0x0805;

				if (IpEntry.dir == DIR_IN) // nas0+
					ether_ret = ethertype_acl_set(DIR_IN, etherval);
				else // br0(port 0, 1, 2, 4)
					ether_ret = ethertype_acl_set(DIR_OUT, etherval);
			}
		}

#ifdef CONFIG_E8B
		if(udp_tcp_idx!=-1){
			//add tcp rule for udp/tcp protocol
			argv[udp_tcp_idx] = (char *)ARG_TCP;
			TRACE(STA_SCRIPT, "%s %s %s %s %s %s %s ...\n", IPTABLES, argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
			do_cmd(IPTABLES, argv, 1);
		}
#endif

		if (IpEntry.etherType == ETHER_NONE) {
#ifdef CONFIG_USER_CONNTRACK_TOOLS
			in_addr_t mask=0;
			char smask[20]={};
			char dmask[20]={};
			char *ct_argv[20];
			int ct_idx=1;

			if(!strcmp(policy, FW_DROP))
			{
				//should delete conntrack here
				ct_argv[ct_idx++] = "-D";//dekete conntrack
				//			ct_argv[ct_idx++] = "-L";//list

				// protocol
				if (IpEntry.protoType != PROTO_NONE) {
					ct_argv[ct_idx++] = "-p";
					if (IpEntry.protoType == PROTO_TCP)
						ct_argv[ct_idx++] = (char *)ARG_TCP;
					else if (IpEntry.protoType == PROTO_UDP)
						ct_argv[ct_idx++] = (char *)ARG_UDP;
					else //if (IpEntry.protoType == PROTO_ICMP)
						ct_argv[ct_idx++] = (char *)ARG_ICMP;
				}

				// source ip, mask
				strncpy(srcip, inet_ntoa(*((struct in_addr *)IpEntry.srcIp)), 16);
				if (strcmp(srcip, ARG_0x4) != 0)
				{
					ct_argv[ct_idx++] = "-s";
					ct_argv[ct_idx++] = srcip;

					if (IpEntry.smaskbit!=0)
					{
						mask = ~0 << (sizeof(in_addr_t)*8 - IpEntry.smaskbit);
						mask = htonl(mask);
						sprintf(smask, "%s", inet_ntoa(*((struct in_addr *)&mask)));

						ct_argv[ct_idx++] = "--mask-src";
						ct_argv[ct_idx++] = smask;
					}
				}

				// destination ip, mask
				strncpy(dstip, inet_ntoa(*((struct in_addr *)IpEntry.dstIp)), 16);
				if (strcmp(dstip, ARG_0x4) != 0)
				{
					ct_argv[ct_idx++] = "-d";
					ct_argv[ct_idx++] = dstip;

					if (IpEntry.dmaskbit!=0)
					{
						mask = ~0 << (sizeof(in_addr_t)*8 - IpEntry.dmaskbit);
						mask = htonl(mask);
						sprintf(dmask, "%s", inet_ntoa(*((struct in_addr *)&mask)));

						ct_argv[ct_idx++] = "--mask-dst";
						ct_argv[ct_idx++] = dmask;
					}
				}

				ct_argv[ct_idx++] = NULL;
				do_cmd("/bin/conntrack", ct_argv, 1);
			}
#endif
		}

		if (IpEntry.etherType != ETHER_NONE) {
			UpEntry.dir = IpEntry.dir;				// direction
			UpEntry.etherType = IpEntry.etherType;	// ethertype value
			UpEntry.etherIndex = ether_ret;			// rtk acl index for delete
			UpEntry.ifIndex = IpEntry.ifIndex;		// interface index
			UpEntry.protoType = PROTO_NONE;			// nothing proto type
			UpEntry.action = 0;						// support only drop
			mib_chain_update(MIB_IP_PORT_FILTER_TBL, (void*)&UpEntry, i); //for update MIB
		}
	}

IPFILTER_END:
	// accept related
	// iptables -A ipfilter -m state --state ESTABLISHED,RELATED -j RETURN
#if defined(CONFIG_00R0)
	mib_get_s(MIB_IPFILTER_SPI_ENABLE, (void*)&ipfilter_spi_state, sizeof(ipfilter_spi_state));
	if(ipfilter_spi_state != 0)
#endif
		va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_IPFILTER, "-m", "state",
				"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_RETURN);

	// Kill all conntrack (to kill the established conntrack when change iptables rules)
#ifndef CONFIG_USER_CONNTRACK_TOOLS
	va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
#endif

	return 1;
}
#endif 	// defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#endif	// IP_PORT_FILTER

#ifdef IP_PORT_FILTER
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
int setup_default_IPFilter(void)
{
	// Set default action for ipfilter
	unsigned char action = 1;	//0: drop (white list), 1: return (black list)
	unsigned char in_enable = 0, out_enable = 0;
	unsigned char ipf_drop_wan_packet_enable = 0;
	mib_get_s(MIB_IPF_DROP_WAN_PACKET_ENABLE, (void*)&ipf_drop_wan_packet_enable, sizeof(ipf_drop_wan_packet_enable));

	mib_get_s(MIB_IPFILTER_IN_ENABLE, (void*)&in_enable, sizeof(in_enable));
	mib_get_s(MIB_IPFILTER_OUT_ENABLE, (void*)&out_enable, sizeof(out_enable));

	mib_get_s(MIB_IPF_OUT_ACTION, (void *)&action, sizeof(action));
	if(out_enable && action == 0)
	{
		// iptables -A ipfilter_out -j DROP
		va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_IPFILTER_OUT, "-j", (char *)FW_DROP);
	}

	mib_get_s(MIB_IPF_IN_ACTION, (void *)&action, sizeof(action));
	if(in_enable && action == 0)
	{
		// iptables -A ipfilter_in -j DROP
		va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_IPFILTER_IN, "-j", (char *)FW_DROP);
	}

	if(ipf_drop_wan_packet_enable == 1)
	{
		// iptables -A ipfilter_in ! -i br+ -j DROP
		va_cmd(IPTABLES, 7, 1, (char *)FW_ADD, (char *)FW_IPFILTER_IN, (char *)ARG_NO,(char *)ARG_I, "br+","-j", (char *)FW_DROP);
	}

	return 1;
}
#else
int setup_default_IPFilter(void)
{
	// Set default action for ipfilter
	unsigned char value[32];
	int vInt=-1;

#if defined(CONFIG_E8B) ||defined(CONFIG_00R0)
	unsigned char ipportfilter_state;

	mib_get_s(MIB_IPFILTER_ON_OFF, (void*)&ipportfilter_state, sizeof(ipportfilter_state));

	if(ipportfilter_state == 0) return 0;
#endif

	if (mib_get_s(MIB_IPF_OUT_ACTION, (void *)value, sizeof(value)) != 0)
	{
		vInt = (int)(*(unsigned char *)value);

		if (vInt == 0)	// DROP
		{
			// iptables -A ipfilter -i $LAN_IF -j DROP
			va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_IPFILTER, (char *)ARG_I, "br+", "-j", (char *)FW_DROP);
#ifdef CONFIG_USER_CONNTRACK_TOOLS
			va_cmd("/bin/conntrack", 1, 1, "-D");
#endif
		}
	}

	if (mib_get_s(MIB_IPF_IN_ACTION, (void *)value, sizeof(value)) != 0)
	{
		vInt = (int)(*(unsigned char *)value);

		if (vInt == 0)	// DROP
		{
#if defined (CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_L2TPD_LNS)
			set_LAN_VPN_accept((char *)FW_IPFILTER);
#endif
			// iptables -A ipfilter ! -i $LAN_IF -j DROP
			va_cmd(IPTABLES, 7, 1, (char *)FW_ADD, (char *)FW_IPFILTER, "!", (char *)ARG_I, "br+", "-j", (char *)FW_DROP);
#ifdef CONFIG_USER_CONNTRACK_TOOLS
			va_cmd("/bin/conntrack", 1, 1, "-D");
#endif
		}
	}

	return 1;
}
#endif	// defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#endif	// IP_PORT_FILTER

#if defined(MAC_FILTER) && defined(IGMPHOOK_MAC_FILTER)
#ifdef CONFIG_E8B
//reset  mac blacklist of igmpHook module
int reset_IgmpHook_MacFilter()
{
	int i, j,total,totalDev;
	char *policy;
	MIB_CE_ROUTEMAC_T MacEntry;
	unsigned char macFilterEnable = 0;
	unsigned char macFilterMode = 0; // 0-black list, 1-white list
	char cmd[256];

	//flush all mac from blacklist of igmpHook module
	memset(cmd, 0 ,sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "echo FLUSH 1 > /proc/igmp/ctrl/blackList");
	system(cmd);

	mib_get_s(MIB_MAC_FILTER_SRC_ENABLE, &macFilterEnable, sizeof(macFilterEnable));

	if(macFilterEnable&1)//BIT0 set - black list mode. if both BIT0&BIT1 are set, only black list mode
		macFilterMode = 0;
	else if(macFilterEnable&2)//BIT1 set, BIT0 not set - white list mode.
		macFilterMode = 1;

	//only for blacklist
	if((!macFilterEnable) || (macFilterMode != 0))
		return 0;

	total = mib_chain_total(MIB_MAC_FILTER_ROUTER_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_MAC_FILTER_ROUTER_TBL, i, (void *)&MacEntry))
			continue;
#ifdef MAC_FILTER_BLOCKTIMES_SUPPORT
		if((macFilterMode != MacEntry.mode)|| (!MacEntry.enable))
#else
		if(macFilterMode != MacEntry.mode)
#endif
			continue;

		for (j=PMAP_ETH0_SW0; j < PMAP_WLAN0 && j<ELANVIF_NUM; j++)
		{
			memset(cmd, 0 ,sizeof(cmd));
			snprintf(cmd, sizeof(cmd), "echo DEVIFNAME %s SMAC %s ADDMAC 1  > /proc/igmp/ctrl/blackList",(char *)ELANVIF[j],MacEntry.mac);
			system(cmd);
		}

#ifdef WLAN_SUPPORT
		for (j=PMAP_WLAN0; j<=PMAP_WLAN0_VAP3; j++) {
			if(wlan_en[j-PMAP_WLAN0])
			{
				memset(cmd, 0 ,sizeof(cmd));
				snprintf(cmd, sizeof(cmd), "echo DEVIFNAME %s SMAC %s ADDMAC 1  > /proc/igmp/ctrl/blackList",(char *)wlan[j-PMAP_WLAN0],MacEntry.mac);
				system(cmd);
			}
		}

		for (j=PMAP_WLAN1; j<=PMAP_WLAN1_VAP3; j++) {
			if(wlan_en[j-PMAP_WLAN0])
			{
				memset(cmd, 0 ,sizeof(cmd));
				snprintf(cmd, sizeof(cmd), "echo DEVIFNAME %s SMAC %s ADDMAC 1  > /proc/igmp/ctrl/blackList",(char *)wlan[j-PMAP_WLAN0],MacEntry.mac);
				system(cmd);
			}
		}
#endif // of WLAN_SUPPORT

	}

	return 0;
}

#else
int reset_IgmpHook_MacFilter()
{
	int i, j,total,totalDev;
	char *policy;
	MIB_CE_MAC_FILTER_T Entry;
	char cmd[256];
	char mac[18];

	//flush all mac from blacklist of igmpHook module
	memset(cmd, 0 ,sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "echo FLUSH 1 > /proc/igmp/ctrl/blackList");
	system(cmd);


	total = mib_chain_total(MIB_MAC_FILTER_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_MAC_FILTER_TBL, i, (void *)&Entry))
			continue;

		// action: 0 - Deny, 1 - Allow
		if (Entry.action)
			continue;

		if ( Entry.srcMac[0]==0 && Entry.srcMac[1]==0
			&& Entry.srcMac[2]==0 && Entry.srcMac[3]==0
			&& Entry.srcMac[4]==0 && Entry.srcMac[5]==0 )
			continue;

		memset(mac,'\0',sizeof(mac));
		snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",Entry.srcMac[0], Entry.srcMac[1],Entry.srcMac[2], Entry.srcMac[3],Entry.srcMac[4], Entry.srcMac[5]);

		for (j=PMAP_ETH0_SW0; j < PMAP_WLAN0 && j<ELANVIF_NUM; j++)
		{
			memset(cmd, 0 ,sizeof(cmd));
			snprintf(cmd, sizeof(cmd), "echo DEVIFNAME %s SMAC %s ADDMAC 1	> /proc/igmp/ctrl/blackList",(char *)ELANVIF[j],mac);
			system(cmd);
		}

#ifdef WLAN_SUPPORT
		for (j=PMAP_WLAN0; j<=PMAP_WLAN0_VAP3; j++) {
			if(wlan_en[j-PMAP_WLAN0])
			{
				memset(cmd, 0 ,sizeof(cmd));
				snprintf(cmd, sizeof(cmd), "echo DEVIFNAME %s SMAC %s ADDMAC 1	> /proc/igmp/ctrl/blackList",(char *)wlan[j-PMAP_WLAN0],mac);
				system(cmd);
			}
		}

		for (j=PMAP_WLAN1; j<=PMAP_WLAN1_VAP3; j++) {
			if(wlan_en[j-PMAP_WLAN0])
			{
				memset(cmd, 0 ,sizeof(cmd));
				snprintf(cmd, sizeof(cmd), "echo DEVIFNAME %s SMAC %s ADDMAC 1	> /proc/igmp/ctrl/blackList",(char *)wlan[j-PMAP_WLAN0],mac);
				system(cmd);
			}
		}
#endif // of WLAN_SUPPORT


	}

	return 0;
}

#endif
#endif


#if defined(IP_PORT_FILTER) || defined(MAC_FILTER) || defined(DMZ)
int restart_IPFilter_DMZ_MACFilter(void)
{
#ifdef IP_PORT_FILTER
	setupIPFilter();
#endif
#if defined(IP_PORT_FILTER) && defined(DMZ)
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	// iptables -A filter_in -j dmz
	va_cmd(IPTABLES, 4, 1, "-A", (char *)FW_IPFILTER_IN, "-j", (char *)IPTABLE_DMZ);
#else
	// iptables -A filter -j dmz
	va_cmd(IPTABLES, 4, 1, "-A", (char *)FW_IPFILTER, "-j", (char *)IPTABLE_DMZ);
#endif
#endif
#ifdef IP_PORT_FILTER
	setup_default_IPFilter();
#endif
#ifdef MAC_FILTER
#ifdef CONFIG_E8B
	setupMacFilterTables();
#else
	setupMacFilter();
#endif
#ifdef IGMPHOOK_MAC_FILTER
	reset_IgmpHook_MacFilter();
#endif
#endif
#ifdef CONFIG_IPV6
	restart_IPV6Filter();
#endif
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_flow_flush();
#endif
	return 1;
}
#endif

#if defined(CONFIG_USER_RTK_NAT_ALG_PASS_THROUGH) || (defined(CONFIG_TRUE) && defined(CONFIG_IP_NF_ALG_ONOFF))
//add by ramen return 1--sucessful
// Mason Yu. alg_onoff_20101023
int setupAlgOnOff(void)
{
	unsigned char value=0;
	char ports[64] = {0};
#if defined(CONFIG_CMCC)
	unsigned char alg_off_force_drop=1;
	mib_get_s(MIB_ALG_OFF_FORCE_DROP,(void*)&alg_off_force_drop, sizeof(alg_off_force_drop));
	printf("setupAlgOnOff alg_off_force_drop=%d\n",alg_off_force_drop);
#endif
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_VPNPASSTHRU);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)FW_VPNPASSTHRU);
#endif
#ifdef CONFIG_NF_CONNTRACK_IPSEC_MODULE
	if(mib_get_s(MIB_IP_ALG_IPSEC,&value, sizeof(value))&& value==0) {
		//va_cmd("/bin/modprobe", 2, 1, "-r", "nf_nat_ipsec");
#if defined(CONFIG_CMCC)
		if(alg_off_force_drop==1){
			//iptables -A vpnPassThrough -i br0 -p udp --dport 500 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "udp", "--dport", "500", "-j", (char *)FW_DROP);
			//iptables -A vpnPassThrough -i br0 -p udp --dport 4500 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "udp", "--dport", "4500", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			//ip6tables -A vpnPassThrough -i br0 -p udp --dport 500 -j DROP
			va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "udp", "--dport", "500", "-j", (char *)FW_DROP);
			//ip6tables -A vpnPassThrough -i br0 -p udp --dport 4500 -j DROP
			va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "udp", "--dport", "4500", "-j", (char *)FW_DROP);
#endif
		}
#endif
	}
	else{//enable
		snprintf(ports, sizeof(ports), "ports=500,4500");
		printf("%s:%d ipsec_port %s\n", __FUNCTION__, __LINE__, ports);
		va_cmd("/bin/modprobe", 2, 1, "nf_conntrack_ipsec", ports);
		//va_cmd("/bin/modprobe", 1, 1, "nf_nat_ipsec");
#ifdef CONFIG_USER_CONNTRACK_TOOLS
		va_cmd(CONNTRACK_TOOL, 3, 1, "-D", "--dport", "4500");
		va_cmd(CONNTRACK_TOOL, 3, 1, "-D", "--dport", "500");
#endif
	}
#endif

#ifdef CONFIG_NF_CONNTRACK_L2TP_MODULE
	if(mib_get_s(MIB_IP_ALG_L2TP,&value, sizeof(value))&& value==0) {
		va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-p", "udp", "--dport", "1701", "-j", (char *)FW_DROP);
		va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-p", "udp", "--dport", "4500", "-j", (char *)FW_DROP);
		va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-p", "udp", "--dport", "500", "-j", (char *)FW_DROP);
#ifdef CONFIG_USER_CONNTRACK_TOOLS
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "udp", "--dport", "1701");
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "udp", "--dport", "4500");
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "udp", "--dport", "500");
#endif
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-p", "udp", "--dport", "1701", "-j", (char *)FW_DROP);
		va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-p", "udp", "--dport", "4500", "-j", (char *)FW_DROP);
		va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-p", "udp", "--dport", "500", "-j", (char *)FW_DROP);
#endif
	}
#endif

#ifdef CONFIG_NF_CONNTRACK_PPTP_MODULE
	if(mib_get_s(MIB_IP_ALG_PPTP,&value, sizeof(value))&& value==0){
		va_cmd("/bin/modprobe", 2, 1, "-r", "nf_nat_pptp");
#if defined(CONFIG_CMCC)
		if(alg_off_force_drop==1){
			//iptables -A vpnPassThrough -i br0 -p tcp --dport 1723 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "tcp", "--dport", "1723", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			//ip6tables -A vpnPassThrough -i br0 -p tcp --dport 1723 -j DROP
			va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "tcp", "--dport", "1723", "-j", (char *)FW_DROP);
#endif
		}
#endif
	}
	else{//enable
		snprintf(ports, sizeof(ports), "ports=1723");
		printf("%s:%d pptp_port %s\n", __FUNCTION__, __LINE__, ports);
		va_cmd("/bin/modprobe", 2, 1, "nf_conntrack_pptp", ports);
		va_cmd("/bin/modprobe", 1, 1, "nf_nat_pptp");
#ifdef CONFIG_USER_CONNTRACK_TOOLS
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "tcp", "--dport", "1723");
#endif
	}
#endif

#ifdef CONFIG_NF_CONNTRACK_FTP_MODULE
	if(mib_get_s(MIB_IP_ALG_FTP,&value, sizeof(value))&& value==0){
		va_cmd("/bin/modprobe", 2, 1, "-r", "nf_nat_ftp");
#if defined(CONFIG_CMCC)
		if(alg_off_force_drop==1){
			//iptables -A vpnPassThrough -i br0 -p tcp --dport 21 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "tcp", "--dport", "21", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			//ip6tables -A vpnPassThrough -i br0 -p tcp --dport 21 -j DROP
			va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "tcp", "--dport", "21", "-j", (char *)FW_DROP);
#endif
		}
#endif
	}
	else{//enable
		snprintf(ports, sizeof(ports), "ports=21");
		printf("%s:%d ftp_port %s\n", __FUNCTION__, __LINE__, ports);
		va_cmd("/bin/modprobe", 2, 1, "nf_conntrack_ftp", ports);
		va_cmd("/bin/modprobe", 1, 1, "nf_nat_ftp");
#ifdef CONFIG_USER_CONNTRACK_TOOLS
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "tcp", "--dport", "21");
#endif
	}
#endif
#ifdef CONFIG_NF_CONNTRACK_TFTP_MODULE
	if(mib_get_s(MIB_IP_ALG_TFTP,&value, sizeof(value))&& value==0){
		va_cmd("/bin/modprobe", 2, 1, "-r", "nf_nat_tftp");
#if defined(CONFIG_CMCC)
		if(alg_off_force_drop==1){
			//iptables -A vpnPassThrough -i br0 -p udp --dport 69 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "udp", "--dport", "69", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			//ip6tables -A vpnPassThrough -i br0 -p udp --dport 69 -j DROP
			va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "udp", "--dport", "69", "-j", (char *)FW_DROP);
#endif
		}
#endif
	}
	else{//enable
		snprintf(ports, sizeof(ports), "ports=69");
		printf("%s:%d tftp_port %s\n", __FUNCTION__, __LINE__, ports);
		va_cmd("/bin/modprobe", 2, 1, "nf_conntrack_tftp", ports);
		va_cmd("/bin/modprobe", 1, 1, "nf_nat_tftp");
#ifdef CONFIG_USER_CONNTRACK_TOOLS
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "udp", "--dport", "69");
#endif
	}
#endif
#ifdef CONFIG_NF_CONNTRACK_H323_MODULE
	if(mib_get_s(MIB_IP_ALG_H323,&value, sizeof(value))&& value==0) {
		va_cmd("/bin/modprobe", 2, 1, "-r", "nf_nat_h323");
#if defined(CONFIG_CMCC)
		if(alg_off_force_drop==1){
			//iptables -A vpnPassThrough -i br0 -p tcp --dport 1729 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "tcp", "--dport", "1729", "-j", (char *)FW_DROP);
			//iptables -A vpnPassThrough -i br0 -p tcp --dport 1720 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "tcp", "--dport", "1720", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			//ip6tables -A vpnPassThrough -i br0 -p tcp --dport 1729 -j DROP
			va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "tcp", "--dport", "1729", "-j", (char *)FW_DROP);
			//ip6tables -A vpnPassThrough -i br0 -p tcp --dport 1720 -j DROP
			va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "tcp", "--dport", "1720", "-j", (char *)FW_DROP);
#endif
		}
#endif
	}
	else{//enable
		va_cmd("/bin/modprobe", 1, 1, "nf_nat_h323");
		snprintf(ports, sizeof(ports), "ports=1729,1720");
		printf("%s:%d h323_port %s\n", __FUNCTION__, __LINE__, ports);
		va_cmd("/bin/modprobe", 2, 1, "nf_conntrack_h323", ports);
#ifdef CONFIG_USER_CONNTRACK_TOOLS
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "tcp", "--dport", "1729");
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "tcp", "--dport", "1720");
#endif
	}
#endif
#ifdef CONFIG_NF_CONNTRACK_IRC_MODULE
	if(mib_get_s(MIB_IP_ALG_IRC,&value, sizeof(value))&& value==0){
		va_cmd("/bin/modprobe", 2, 1, "-r", "nf_nat_irc");
#if defined(CONFIG_CMCC)
		//iptables -A vpnPassThrough -i br0 -p tcp --dport 6667 -j DROP
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "tcp", "--dport", "6667", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
		//ip6tables -A vpnPassThrough -i br0 -p tcp --dport 6667 -j DROP
#endif
		va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "tcp", "--dport", "6667", "-j", (char *)FW_DROP);
#endif
	}
	else{//enable
		snprintf(ports, sizeof(ports), "ports=6667");
		printf("%s:%d irc_port %s\n", __FUNCTION__, __LINE__, ports);
		va_cmd("/bin/modprobe", 2, 1, "nf_conntrack_irc", ports);
		va_cmd("/bin/modprobe", 1, 1, "nf_nat_irc");
#ifdef CONFIG_USER_CONNTRACK_TOOLS
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "tcp", "--dport", "6667");
#endif
	}
#endif
#ifdef CONFIG_NF_CONNTRACK_RTSP_MODULE
	if(mib_get_s(MIB_IP_ALG_RTSP,&value, sizeof(value))&& value==0) {
		va_cmd("/bin/modprobe", 2, 1, "-r", "nf_nat_rtsp");
#if defined(CONFIG_CMCC)
		if(alg_off_force_drop==1){
			//iptables -A vpnPassThrough -i br0 --dport 554 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "tcp", "--dport", "554", "-j", (char *)FW_DROP);
			//iptables -A vpnPassThrough -i br0 --dport 8554 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "tcp", "--dport", "8554", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			//ip6tables -A vpnPassThrough -i br0 --dport 554 -j DROP
			va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "tcp", "--dport", "554", "-j", (char *)FW_DROP);
			//ip6tables -A vpnPassThrough -i br0 --dport 8554 -j DROP
			va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "tcp", "--dport", "8554", "-j", (char *)FW_DROP);
#endif
		}
#endif
	}
	else{//enable
		snprintf(ports, sizeof(ports), "ports=554,8554");
		printf("%s:%d rtsp_port %s\n", __FUNCTION__, __LINE__, ports);
		va_cmd("/bin/modprobe", 2, 1, "nf_conntrack_rtsp", ports);
		va_cmd("/bin/modprobe", 1, 1, "nf_nat_rtsp");
#ifdef CONFIG_USER_CONNTRACK_TOOLS
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "tcp", "--dport", "554");
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "udp", "--dport", "554");
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "tcp", "--dport", "8554");
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "udp", "--dport", "8554");
#endif
	}
#endif
#if defined(CONFIG_NF_CONNTRACK_SIP_MODULE)
	if(mib_get_s(MIB_IP_ALG_SIP,&value, sizeof(value))&& value==0) {
		va_cmd("/bin/modprobe", 2, 1, "-r", "nf_nat_sip");  // Session Initiation Protocol (SIP)
#if 0//defined(CONFIG_CMCC)
		if(alg_off_force_drop==1){
			//iptables -A vpnPassThrough -i br0 -p udp --dport 5060 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "udp", "--dport", "5060", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			//ip6tables -A vpnPassThrough -i br0 -p udp --dport 5060 -j DROP
			va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_VPNPASSTHRU, "-i","br0","-p", "udp", "--dport", "5060", "-j", (char *)FW_DROP);
#endif
		}
#endif
	}
	else{//enable
		snprintf(ports, sizeof(ports), "ports=5060");
		printf("%s:%d sip_port %s\n", __FUNCTION__, __LINE__, ports);
		va_cmd("/bin/modprobe", 2, 1, "nf_conntrack_sip", ports);  // Session Initiation Protocol (SIP)
		va_cmd("/bin/modprobe", 1, 1, "nf_nat_sip");
#ifdef CONFIG_USER_CONNTRACK_TOOLS
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "tcp", "--dport", "5060"); //only use udp
		va_cmd(CONNTRACK_TOOL, 5, 1, "-D", "-p", "udp", "--dport", "5060");
#endif
#ifdef QOS_SUPPORT_RTP
		{
			int boa_pid;
			char cmd_str[128]={0};
			boa_pid=read_pid(BOA_PID);
			if(boa_pid>0){
				sprintf(cmd_str,"/bin/echo %d >/proc/sys/net/boa_pid",boa_pid);
				system(cmd_str);
			}
		}
#endif
	}
#endif

#ifdef CONFIG_RTK_RG_INIT
	RTK_RG_ALG_Set();
#endif
	// Kill all conntrack
#ifndef CONFIG_USER_CONNTRACK_TOOLS
	// unused on deonet
	//va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
#endif
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	system("echo 1 > /proc/fc/ctrl/rstconntrack");
	system("echo 1 > /proc/fc/ctrl/flow_flush");
#endif
	return 0;
}
#endif

#ifdef PORT_FORWARD_GENERAL
int delPortForwarding( unsigned int ifindex )
{
	int total,i;

	total = mib_chain_total( MIB_PORT_FW_TBL );
	//for( i=0;i<total;i++ )
	for( i=total-1;i>=0;i-- )
	{
		MIB_CE_PORT_FW_T *c, port_entity;
		c = &port_entity;
		if( !mib_chain_get( MIB_PORT_FW_TBL, i, (void*)c ) )
			continue;

		if(c->ifIndex==ifindex)
			mib_chain_delete( MIB_PORT_FW_TBL, i );
	}
	return 0;
}

int updatePortForwarding( unsigned int old_id, unsigned int new_id )
{
	unsigned int total,i;

	total = mib_chain_total( MIB_PORT_FW_TBL );
	for( i=0;i<total;i++ )
	{
		MIB_CE_PORT_FW_T *c, port_entity;
		c = &port_entity;
		if( !mib_chain_get( MIB_PORT_FW_TBL, i, (void*)c ) )
			continue;

		if(c->ifIndex==old_id)
		{
			c->ifIndex = new_id;
			mib_chain_update( MIB_PORT_FW_TBL, (unsigned char*)c, i );
		}
	}
	return 0;
}
#endif

#ifdef PORT_FORWARD_GENERAL
void clear_dynamic_port_fw(int (*upnp_delete_redirection)(unsigned short eport, const char * protocol))
{
	int i, total;
	MIB_CE_PORT_FW_T port_entity;

	total = mib_chain_total(MIB_PORT_FW_TBL);

	for (i = total - 1; i >= 0; i--) {
		if (!mib_chain_get(MIB_PORT_FW_TBL, i, &port_entity))
			continue;

		if (port_entity.dynamic) {
			/* created by UPnP */
			mib_chain_delete(MIB_PORT_FW_TBL, i);

			if (upnp_delete_redirection)
				upnp_delete_redirection(port_entity.externalfromport,
					port_entity.protoType == PROTO_UDP ? "UDP" : "TCP");
		}
	}
}

// Mason Yu
int setupPortFW(void)
{
	int vInt, i, total;
	unsigned char value[32];
	MIB_CE_PORT_FW_T PfEntry;

	// Clean all rules
	// iptables -F portfw
	va_cmd(IPTABLES, 2, 1, "-F", (char *)PORT_FW);
	// iptables -t nat -F portfw
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)PORT_FW);

#ifdef NAT_LOOPBACK
	// iptables -t nat -F portfwPreNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)PORT_FW_PRE_NAT_LB);
	// iptables -t nat -F portfwPreNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)PORT_FW_POST_NAT_LB);
	#ifdef CONFIG_NETFILTER_XT_TARGET_NOTRACK
	/* iptables -t raw -F rawTrackNatLB_PF */
	va_cmd(IPTABLES, 4, 1, "-t", "raw", "-F", (char *)PF_RAW_TRACK_NAT_LB);
	#endif

	cleanALLEntry_NATLB_rule_dynamic_link(DEL_PORTFW_NATLB_DYNAMIC);
#endif
	// Remote access not go port-fw (IP_ACL/Remote_Access prevails port_forwarding)
	va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_ADD, (char *)PORT_FW, "!", (char *)ARG_I,
		(char *)LANIF, "-m", "mark", "--mark", RMACC_MARK, "-j", FW_RETURN);

	vInt = 0;
	if (mib_get_s(MIB_PORT_FW_ENABLE, (void *)value, sizeof(value)) != 0)
		vInt = (int)(*(unsigned char *)value);

	if (vInt == 1)
	{
		int negate=0, hasRemote=0;
		char * proto = 0;
		char intPort[32], extPort[32];

		total = mib_chain_total(MIB_PORT_FW_TBL);

#ifdef NAT_LOOPBACK
		if (total > 0)
		{
			add_portfw_dynamic_NATLB_chain();
		}
#endif

		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_PORT_FW_TBL, i, (void *)&PfEntry))
				return -1;

			if (PfEntry.dynamic == 1)
				continue;
			portfw_modify( &PfEntry, 0 );
		}
	}//if (vInt == 1)
#ifndef CONFIG_TELMEX_DEV
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	system("echo 1 > /proc/fc/ctrl/rstconntrack");
	system("echo 1 > /proc/fc/ctrl/flow_flush");
#endif
#endif

#ifdef CONFIG_TELMEX_DEV
	filter_patch_for_dmz();
#endif

	return 1;
}
#endif

#if defined(CONFIG_USER_BOA_PRO_PASSTHROUGH)
int Setup_VpnPassThrough()
{
   unsigned int entryNum, i;
   MIB_CE_ATM_VC_T Entry;
   char wanname[IFNAMSIZ];

   // iptables -F IPSEC_PASSTHROUGH_FORWARD
   va_cmd(IPTABLES, 2, 1, "-F", (char *)IPSEC_PASSTHROUGH_FORWARD);

   // iptables -F L2TP_PASSTHROUGH_FORWARD
   va_cmd(IPTABLES, 2, 1, "-F", (char *)L2TP_PASSTHROUGH_FORWARD);

   // iptables -F PPTP_PASSTHROUGH_FORWARD
   va_cmd(IPTABLES, 2, 1, "-F", (char *)PPTP_PASSTHROUGH_FORWARD);

   entryNum = mib_chain_total(MIB_ATM_VC_TBL);

   for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;

		if(!isInterfaceMatch(Entry.ifIndex))
			continue;

		if (Entry.enable == 0 || !isValidMedia(Entry.ifIndex))
			continue;

		ifGetName(Entry.ifIndex, wanname, sizeof(wanname));
		printf("%s.%d. i=%d  wanname=%s\n",__FUNCTION__,__LINE__, i, wanname);

		if(Entry.vpnpassthru_ipsec_enable==0){

			// iptables -A IPSEC_PASSTHROUGH_FORWARD  -p udp dport 500 in pIfaceWan -j Drop;
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)IPSEC_PASSTHROUGH_FORWARD,
				"-p", (char *)ARG_UDP, (char *)FW_DPORT, (char *)PORT_IPSEC, (char *)ARG_I, (char *)wanname, "-j", (char *)FW_DROP);

			// iptables -A IPSEC_PASSTHROUGH_FORWARD  -p udp dport 4500 in pIfaceWan -j Drop;
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)IPSEC_PASSTHROUGH_FORWARD,
				"-p", (char *)ARG_UDP, (char *)FW_DPORT, (char *)PORT_IPSEC_NAT, (char *)ARG_I, (char *)wanname, "-j", (char *)FW_DROP);
        }
		else{

			// iptables -A IPSEC_PASSTHROUGH_FORWARD  -p udp dport 500 in pIfaceWan -j Drop;
			va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char *)IPSEC_PASSTHROUGH_FORWARD,
				"-p", (char *)ARG_UDP, (char *)FW_DPORT, (char *)PORT_IPSEC, (char *)ARG_I, (char *)wanname,(char *)ARG_O, (char *)LANIF, "-j", (char *)FW_ACCEPT);
		}

		if(Entry.vpnpassthru_l2tp_enable==0){

			// iptables -A L2TP_PASSTHROUGH_FORWARD  -p udp dport 1701 in pIfaceWan -j Drop;
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)L2TP_PASSTHROUGH_FORWARD,
				"-p", (char *)ARG_UDP, (char *)FW_SPORT, (char *)PORT_L2TP, (char *)ARG_I, (char *)wanname, "-j", (char *)FW_DROP);
		}
		else{

			// iptables -A L2TP_PASSTHROUGH_FORWARD  -p udp sport 1701 in pIfaceWan -j Accept;
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)L2TP_PASSTHROUGH_FORWARD,
				"-p", (char *)ARG_UDP, (char *)FW_DPORT, (char *)PORT_L2TP, (char *)ARG_I, (char *)wanname, "-j", (char *)FW_ACCEPT);

			// iptables -A L2TP_PASSTHROUGH_FORWARD	-p udp dport 1701 in pIfaceWan	-j Accept;
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)L2TP_PASSTHROUGH_FORWARD,
				"-p", (char *)ARG_UDP, (char *)FW_SPORT, (char *)PORT_L2TP, (char *)ARG_I, (char *)wanname, "-j", (char *)FW_ACCEPT);
		}

		if(Entry.vpnpassthru_pptp_enable==0){

			// iptables -A PPTP_PASSTHROUGH_FORWARD_WAN -p tcp dport 1723 in pIfaceWan -j Drop;
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)PPTP_PASSTHROUGH_FORWARD,
				"-p", (char *)ARG_TCP, (char *)FW_SPORT, (char *)PORT_PPTP, (char *)ARG_I, (char *)wanname, "-j", (char *)FW_DROP);
		}
		else{

			// iptables -A PPTP_PASSTHROUGH_FORWARD_WAN  -p tcp dport 1723 in pIfaceWan -j Accept;
			//va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)PPTP_PASSTHROUGH_FORWARD,
			//   "-p", (char *)ARG_TCP, (char *)FW_DPORT, (char *)PORT_PPTP, (char *)ARG_I, (char *)wanname, "-j", (char *)FW_ACCEPT);

			// iptables -A PPTP_PASSTHROUGH_FORWARD_WAN  -p tcp sport 1723 in pIfaceWan -j Accept;
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)PPTP_PASSTHROUGH_FORWARD,
			   "-p", (char *)ARG_TCP, (char *)FW_SPORT, (char *)PORT_PPTP, (char *)ARG_I, (char *)wanname, "-j", (char *)FW_ACCEPT);

			// iptables -A PPTP_PASSTHROUGH_FORWARD_WAN  -p  47  in pIfaceWan  -j Accept;
			va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)PPTP_PASSTHROUGH_FORWARD,
			   "-p", "47", (char *)ARG_I, (char *)wanname, "-j", (char *)FW_ACCEPT);

		}
   }

}
#endif


#ifdef PORT_FORWARD_ADVANCE
static int setupPFWAdvance(void)
{
	// iptables -N pptp
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_PPTP);
	// iptables -A FORWARD -j pptp
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_PPTP);
	// iptables -t nat -N pptp
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)FW_PPTP);
	// iptables -t nat -A PREROUTING -j pptp
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)FW_PPTP);


	// iptables -N l2tp
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_L2TP);
	// iptables -A FORWARD -j l2tp
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_L2TP);
	// iptables -t nat -N l2tp
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)FW_L2TP);
	// iptables -t nat -A PREROUTING -j l2tp
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)FW_L2TP);

	config_PFWAdvance(ACT_START);

	return 0;
}

static int stopPFWAdvance(void)
{
	unsigned int entryNum, i;
	MIB_CE_PORT_FW_ADVANCE_T Entry;
	int pptp_enable=1;
	int l2tp_enable=1;

	entryNum = mib_chain_total(MIB_PFW_ADVANCE_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_PFW_ADVANCE_TBL, i, (void *)&Entry))
		{
  			printf("stopPFWAdvance: Get chain record error!\n");
			return 1;
		}

		if ( strcmp("PPTP", PFW_Rule[(PFW_RULE_T)Entry.rule]) == 0 && pptp_enable == 1) {
			// iptables -F pptp
			va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_PPTP);
			// iptables -t nat -F pptp
			va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)FW_PPTP);
			pptp_enable = 0;
		}

		if ( strcmp("L2TP", PFW_Rule[(PFW_RULE_T)Entry.rule]) == 0 && l2tp_enable == 1) {
			// iptables -F l2tp
			va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_L2TP);
			// iptables -t nat -F l2tp
			va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)FW_L2TP);
			l2tp_enable = 0;
		}
	}
	return 0;
}

static int startPFWAdvance(void)
{
	unsigned int entryNum, i;
	MIB_CE_PORT_FW_ADVANCE_T Entry;
	char interface_name[IFNAMSIZ], lanIP[16], ip_port[32];
	struct in_addr dest;
	int pf_enable;
	unsigned char value[32];
	int pptp_enable=0;
	int l2tp_enable=0;

	entryNum = mib_chain_total(MIB_PFW_ADVANCE_TBL);


	pf_enable = 0;
	if (mib_get_s(MIB_PORT_FW_ENABLE, (void *)value, sizeof(value)) != 0)
		pf_enable = (int)(*(unsigned char *)value);

	if ( pf_enable != 1 ) {
		printf("Port Forwarding is disable and stop to setup Port Forwarding Advance!\n");
		return 1;
	}

	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_PFW_ADVANCE_TBL, i, (void *)&Entry))
		{
  			printf("startPFWAdvance: Get chain record error!\n");
			return 1;
		}

		if ( strcmp("PPTP", PFW_Rule[(PFW_RULE_T)Entry.rule]) == 0 && pptp_enable == 0) {
			// LAN IP Address
			dest.s_addr = *(unsigned long *)Entry.ipAddr;
			// inet_ntoa is not reentrant, we have to
			// copy the static memory before reuse it
			strcpy(lanIP, inet_ntoa(dest));
			lanIP[15] = '\0';
			sprintf(ip_port,"%s:%d",lanIP, 1723);

			// interface

			// iptables -A pptp -p tcp --destination-port 1723 --dst $LANIP -j ACCEPT
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_PPTP, "-p", (char *)ARG_TCP, "--destination-port", "1723", "--dst", lanIP, "-j", (char *)FW_ACCEPT);

			// iptables -A pptp -p 47 --dst $LANIP -j ACCEPT
			va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_PPTP, "-p", "47", "--dst", lanIP, "-j", (char *)FW_ACCEPT);


			if (ifGetName(Entry.ifIndex, interface_name, sizeof(interface_name))) {
				// iptables -t nat -A pptp -i ppp0 -p tcp --dport 1723 -j DNAT --to-destination $LANIP:1723
				va_cmd(IPTABLES, 14, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PPTP, "-i", interface_name, "-p", (char *)ARG_TCP, "--dport", "1723", "-j", "DNAT", "--to-destination", ip_port);

				// iptables -t nat -A pptp -i ppp0 -p 47 -j DNAT --to-destination $LANIP
				va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PPTP, "-i", interface_name, "-p", "47", "-j", "DNAT", "--to-destination", lanIP);
			} else {
				// iptables -t nat -A pptp -p tcp --dport 1723 -j DNAT --to-destination $LANIP:1723
				va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PPTP, "-p", (char *)ARG_TCP, "--dport", "1723", "-j", "DNAT", "--to-destination", ip_port);

				// iptables -t nat -A pptp -p 47 -j DNAT --to-destination $LANIP
				va_cmd(IPTABLES, 10, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PPTP, "-p", "47", "-j", "DNAT", "--to-destination", lanIP);
			}
			pptp_enable = 1;
		}

		if ( strcmp("L2TP", PFW_Rule[(PFW_RULE_T)Entry.rule]) == 0 && l2tp_enable == 0) {
			// LAN IP Address
			dest.s_addr = *(unsigned long *)Entry.ipAddr;
			// inet_ntoa is not reentrant, we have to
			// copy the static memory before reuse it
			strcpy(lanIP, inet_ntoa(dest));
			lanIP[15] = '\0';
			sprintf(ip_port,"%s:%d",lanIP, 1701);

			// interface

			// iptables -A l2tp -p udp --destination-port 1701 --dst $LANIP -j ACCEPT
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_L2TP, "-p", (char *)ARG_UDP, "--destination-port", "1701", "--dst", lanIP, "-j", (char *)FW_ACCEPT);

			// iptables -t nat -A l2tp -i ppp0 -p udp --dport 1701 -j DNAT --to-destination $LANIP:1701
			if (!ifGetName(Entry.ifIndex, interface_name, sizeof(interface_name))) {
				va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_L2TP, "-p", (char *)ARG_UDP, "--dport", "1701", "-j", "DNAT", "--to-destination", ip_port);
			} else {
				va_cmd(IPTABLES, 14, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_L2TP, "-i", interface_name, "-p", (char *)ARG_UDP, "--dport", "1701", "-j", "DNAT", "--to-destination", ip_port);
			}
			l2tp_enable = 1;
		}
	}
	return 0;
}
#endif

#ifdef PORT_FORWARD_ADVANCE
int config_PFWAdvance( int action_type )
{
	switch( action_type )
	{
	case ACT_START:
		startPFWAdvance();
		break;

	case ACT_RESTART:
		stopPFWAdvance();
		startPFWAdvance();
		break;

	case ACT_STOP:
		stopPFWAdvance();
		break;

	default:
		return -1;
	}
	return 0;
}
#endif

#ifdef DMZ
static void clearDMZ(void)
{
#if defined(CONFIG_CMCC)
	unsigned char value=0;
#endif
	// iptables -F dmz
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_DMZ);
	// iptables -t nat -F dmz
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)IPTABLE_DMZ);
#ifdef NAT_LOOPBACK
	cleanALLEntry_NATLB_rule_dynamic_link(DEL_DMZ_NATLB_DYNAMIC);

	// iptables -t nat -N dmzPreNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)IPTABLE_DMZ_PRE_NAT_LB);
	// iptables -t nat -N dmzPostNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)IPTABLE_DMZ_POST_NAT_LB);
	#ifdef CONFIG_NETFILTER_XT_TARGET_NOTRACK
	/* iptables -t raw -F rawTrackNatLB_DMZ */
	va_cmd(IPTABLES, 4, 1, "-t", "raw", "-F", (char *)DMZ_RAW_TRACK_NAT_LB);
	#endif
#endif
	/* Remove nf_nat_ftp module which is required by FTP passive mode 20210419 */
#if defined(CONFIG_CMCC)
	if(mib_get_s(MIB_IP_ALG_FTP,&value, sizeof(value))&& value==0)
#endif
		va_cmd("/bin/modprobe", 2, 1, "-r", "nf_nat_ftp");
}

static void setDMZ(char *ip)
{
#ifdef CONFIG_USER_RTK_VOIP
	voip_setup_iptable(1);
#endif
#ifdef CONFIG_USER_CWMP_TR069
	int total=0, port=0, i=0;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ]={0}, portbuff[6]={0};

	mib_get_s(CWMP_CONREQ_PORT, &port, sizeof(port));
	snprintf(portbuff, sizeof(portbuff), "%u", port);
#ifdef CONFIG_00R0 //Internet can be used for TR069
	va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_ADD, (char *)IPTABLE_DMZ, "!", (char *)ARG_I,
			(char *)LANIF_ALL, "-p", (char *)ARG_TCP, (char *)FW_DPORT, portbuff, "-j", (char *)FW_ACCEPT);
#else
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return;
		if (Entry.cmode == CHANNEL_MODE_BRIDGE)
			continue;
		if (Entry.applicationtype & X_CT_SRV_TR069)
		{
			ifGetName(Entry.ifIndex, ifname, sizeof(ifname));
			//iptables -t nat -I dmz -i nas0_0 -p tcp --dport 7547 -j ACCEPT
			va_cmd(IPTABLES, 12, 1,  "-t", "nat", (char *)FW_INSERT, (char *)IPTABLE_DMZ,
					(char *)ARG_I, ifname, "-p", (char *)ARG_TCP,
					(char *)FW_DPORT, portbuff, "-j", (char *)"ACCEPT");
		}
	}
#endif
#endif
	//DNS 53
	va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_ADD, (char *)IPTABLE_DMZ, "!", (char *)ARG_I,
		(char *)LANIF_ALL, "-p", (char *)ARG_UDP, (char *)FW_DPORT, "53", "-j", (char *)FW_ACCEPT);
	//SNTP 123
	va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_ADD, (char *)IPTABLE_DMZ, "!", (char *)ARG_I,
		(char *)LANIF_ALL, "-p", (char *)ARG_UDP, (char *)FW_DPORT, "123", "-j", (char *)FW_ACCEPT);
	//DHCP 67
	va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_ADD, (char *)IPTABLE_DMZ, "!", (char *)ARG_I,
		(char *)LANIF_ALL, "-p", (char *)ARG_UDP, (char *)FW_DPORT, "67", "-j", (char *)FW_ACCEPT);
	// Kaohj -- Multicast not involved
	// iptables -t nat -A dmz -i ! $LAN_IF -d 224.0.0.0/4 -j ACCEPT
	va_cmd(IPTABLES, 11, 1, "-t", "nat", (char *)FW_ADD, (char *)IPTABLE_DMZ, "!", (char *)ARG_I,
		(char *)LANIF_ALL, "-d", "224.0.0.0/4", "-j", (char *)FW_ACCEPT);
	// Remote access not go DMZ
	va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_ADD, (char *)IPTABLE_DMZ, "!", (char *)ARG_I,
		(char *)LANIF_ALL, "-m", "mark", "--mark", RMACC_MARK, "-j", FW_RETURN);
	// iptables -t nat -A dmz -i ! $LAN_IF -j DNAT --to-destination $DMZ_IP
	va_cmd(IPTABLES, 11, 1, "-t", "nat", (char *)FW_ADD, (char *)IPTABLE_DMZ, "!", (char *)ARG_I,
		(char *)LANIF_ALL, "-j", "DNAT", "--to-destination", ip);
	// iptables -A dmz -i ! $LAN_IF -o $LAN_IF -d $DMZ_IP -j ACCEPT
	va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)IPTABLE_DMZ, "!", (char *)ARG_I,
		(char *)LANIF_ALL, (char *)ARG_O, (char *)LANIF, "-d", ip, "-j", (char *)FW_ACCEPT);
#ifdef NAT_LOOPBACK
	addALLEntry_NATLB_rule_dynamic_link(ip, DEL_DMZ_NATLB_DYNAMIC);
	iptable_dmz_natLB(0, ip);
#endif
}

// Mason Yu
int setupDMZ(void)
{
	int vInt;
	unsigned char value[32];
	char ipaddr[32];
#if defined(CONFIG_USER_CONNTRACK_TOOLS)
	FILE *fp;
	char cmdStr[128], buff[128];
	int ctNum = 0;
	time_t start_time, curr_time;
#endif
#if defined(CONFIG_CMCC)
	unsigned char alg_ftp=0;
#endif
	clearDMZ();

	vInt = 0;
	if (mib_get_s(MIB_DMZ_ENABLE, (void *)value, sizeof(value)) != 0)
		vInt = (int)(*(unsigned char *)value);

	if (mib_get_s(MIB_DMZ_IP, (void *)value, sizeof(value)) != 0)
	{
		if (vInt == 1)
		{
			strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
			ipaddr[15] = '\0';
			setDMZ(ipaddr);
			/* FTP-ALG (NAT) needs to be support with FTP passive mode since FTP
			server would give the client ip which is tranlated by DMZ. 20210419 */
#if defined(CONFIG_CMCC)
			if(mib_get_s(MIB_IP_ALG_FTP,&alg_ftp, sizeof(alg_ftp))&& alg_ftp==0)
#endif
				va_cmd("/bin/modprobe", 1, 1, "nf_nat_ftp");
		}
#ifdef CONFIG_TELMEX_DEV
		else
		{
#if defined(CONFIG_USER_CONNTRACK_TOOLS)
			strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
			ipaddr[15] = '\0';

			start_time = time(NULL);
			do {
				snprintf(cmdStr, sizeof(cmdStr), "/bin/conntrack -L | grep %s | wc -l", ipaddr);
				fp = popen(cmdStr, "r");
				if (fp)
				{
					fgets(buff, sizeof(buff), fp);
					ctNum = atoi(buff);
					if (ctNum)
					{
						va_cmd("/bin/conntrack", 3, 1, "-D", "-s", ipaddr);
						va_cmd("/bin/conntrack", 3, 1, "-D", "-g", ipaddr);
						system("/bin/echo 7 > /proc/fc/ctrl/flow_operation");
					}

					pclose(fp);
				}
				curr_time = time(NULL);

				if ((0 == ctNum) || ((curr_time-start_time) >= 5))
					break;
			} while (1);
#else
			//flush conntrack when disable DMZ
			system("/bin/echo flush > /proc/fc/ctrl/rstconntrack");
			system("/bin/echo 7 > /proc/fc/ctrl/flow_operation");
#endif
		}
#endif
	}
#ifdef CONFIG_TELMEX_DEV
	filter_patch_for_dmz();
#endif

	return 0;
}
#endif

#ifdef PORT_TRIGGERING_DYNAMIC
static porttrigger_modify( MIB_CE_PORT_TRG_DYNAMIC_T *p)
{
	char intPort[32], extPort[32], pass_intPort[32], ifname[IFNAMSIZ];

	if(p==NULL) return;

	char vCh=0;
	mib_get_s(MIB_PORT_TRIGGER_ENABLE, (void *)&vCh, sizeof(vCh));
	if(vCh==0) return;

        if (p->fromPort)
        {
            if (p->fromPort == p->toPort)
            {
                snprintf(intPort, sizeof(intPort), "%u", p->fromPort);
                            snprintf(pass_intPort, sizeof(intPort), "%u", p->fromPort);
            }
            else
            {
                /* "%u-%u" is used by port triggering */
                snprintf(intPort, sizeof(intPort), "%u-%u", p->fromPort, p->toPort);
                            snprintf(pass_intPort, sizeof(intPort), "%u:%u", p->fromPort, p->toPort);
            }
        }


	if (p->externalfromport && p->externaltoport && (p->externalfromport != p->externaltoport)) {
		snprintf(extPort, sizeof(extPort), "%u:%u", p->externalfromport, p->externaltoport);
	} else if (p->externalfromport) {
		snprintf(extPort, sizeof(extPort), "%u", p->externalfromport);
	} else if (p->externaltoport) {
		snprintf(extPort, sizeof(extPort), "%u", p->externaltoport);
	}
	//printf( "entry.externalfromport:%d entry.externaltoport=%d\n",  p->externalfromport, p->externaltoport);

	ifGetName(p->ifIndex, ifname, sizeof(ifname));

	if(p->protoType == PROTO_TCP || p->protoType == PROTO_UDPTCP)
	{
		// iptables -A portTrigger -o nas0_0 -p tcp --dport 8081 -j TRIGGER --trigger-type out --trigger-proto tcp --trigger-match InPortRange --trigger-relate InPortRange
		va_cmd(IPTABLES, 18, 1, (char *)FW_ADD, (char *)IPTABLES_PORTTRIGGER, (char *)ARG_O, (char *)ifname, "-p", (char *)ARG_TCP, FW_DPORT, extPort,
			"-j", "TRIGGER", "--trigger-type", "out", "--trigger-proto", (char *)ARG_TCP, "--trigger-match", (char *)intPort, "--trigger-relate", (char *)intPort);
		//pass down packets
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)IPTABLES_PORTTRIGGER, (char *)ARG_I, (char *)ifname, "-j", "ACCEPT",
		"-p", (char *)ARG_TCP, FW_DPORT, (char *)pass_intPort);
	}
	if(p->protoType == PROTO_UDP || p->protoType == PROTO_UDPTCP)
	{
		// iptables -A portTrigger -o nas0_0 -p udp --dport 8081 -j TRIGGER --trigger-type out --trigger-proto udp --trigger-match InPortRange --trigger-relate InPortRange
		va_cmd(IPTABLES, 18, 1, (char *)FW_ADD, (char *)IPTABLES_PORTTRIGGER, (char *)ARG_O, (char *)ifname, "-p", (char *)ARG_UDP, FW_DPORT, extPort,
			"-j", "TRIGGER", "--trigger-type", "out", "--trigger-proto", (char *)ARG_UDP, "--trigger-match", (char *)intPort, "--trigger-relate", (char *)intPort);
		//pass down packets
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)IPTABLES_PORTTRIGGER, (char *)ARG_I, (char *)ifname, "-j", "ACCEPT",
		"-p", (char *)ARG_UDP, FW_DPORT, (char *)pass_intPort);
	}

}

int setupPortTriggering(void)
{
	int i, total;
	unsigned char vChar;
	MIB_CE_PORT_TRG_DYNAMIC_T TriggerEntry;
	// Clean all rules
	// iptables -F portTrigger
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLES_PORTTRIGGER);
	// iptables -t nat -F portTrigger
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)IPTABLES_PORTTRIGGER);

	mib_get_s(MIB_PORT_TRIGGER_ENABLE, (void *)&vChar, sizeof(vChar));
	if (vChar == 1)
	{
		total = mib_chain_total(MIB_PORT_TRG_DYNAMIC_TBL);
		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_PORT_TRG_DYNAMIC_TBL, i, (void *)&TriggerEntry))
				return -1;
			porttrigger_modify(&TriggerEntry);
		}
		// iptables -t nat -A PREROUTING -j TRIGGER --trigger-type dnat
		va_cmd(IPTABLES, 8, 1, "-t", "nat", (char *)FW_ADD,	(char *)IPTABLES_PORTTRIGGER, "-j", "TRIGGER", "--trigger-type", "dnat");
	}
	return 1;
}
#endif

#ifdef VIRTUAL_SERVER_SUPPORT
#ifdef CONFIG_E8B
const char IPTABLE_VTLSUR[]="virtual_server";
int setupVtlsvr(int type)
{

	int i, total;
	struct vtlsvr_entryx Entry;
	char remoteHost[32];
#ifdef VIRTUAL_SERVER_INTERFACE
	char ifname[IFNAMSIZ]={0};
#endif
	char in_interface[32]={0};
	char out_interface[32]={0};
	char lan_dport[32]={0};
	char lan_dport2[32]={0};
#ifdef NAT_LOOPBACK
	char ipaddr[32]={0};
	char extPort[32]={0};
	int hasLocalPort=0;
	int hasExtPort=0;
#endif

	total = mib_chain_total(MIB_VIRTUAL_SVR_TBL);
	DOCMDINIT;
#ifdef CONFIG_NETFILTER_XT_MATCH_PSD
	unsigned char psd_enable = 0;
	char lan_dport_PSD[32]={0};
	mib_get(MIB_PSD_ENABLE, &psd_enable);
#endif
/*
#ifdef IP_FILTER_PRIORITY_HIGHEST
	int position=mib_chain_total(MIB_IP_PORT_FILTER_TBL)+3;
#else
	int position=3;
#endif
*/
	//clear the IPTABLE_VTLSUR rules in the filter tables
	DOCMDARGVS(IPTABLES, DOWAIT, "-F %s", IPTABLE_VTLSUR);

	//clear the IPTABLE_VTLSUR rules in the nat tables
	DOCMDARGVS(IPTABLES, DOWAIT, "-t nat -F %s", IPTABLE_VTLSUR);

	DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle -F %s", IPTABLE_VTLSUR);

#ifdef NAT_LOOPBACK
	// iptables -t nat -F virserverPreNatLB
	DOCMDARGVS(IPTABLES, DOWAIT, "-t nat -F %s", VIR_SER_PRE_NAT_LB);
	// iptables -t nat -F virserverPostNatLB
	DOCMDARGVS(IPTABLES, DOWAIT, "-t nat -F %s", VIR_SER_POST_NAT_LB);
#endif

	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_VIRTUAL_SVR_TBL, i, (void *)&Entry))
			return -1;

		if(Entry.enable == 0)
			continue;

		if(Entry.remotehost[0])
		{
			sprintf(remoteHost, "-s %s", inet_ntoa(*((struct in_addr *)&(Entry.remotehost))));
		}
		else
		{
			remoteHost[0] = '\0';
		}
#ifdef VIRTUAL_SERVER_INTERFACE
		if(Entry.ifIndex){
			ifGetName(Entry.ifIndex,ifname,sizeof(ifname));
			snprintf(in_interface, sizeof(in_interface), "-i %s",ifname);
		}else
#endif
		{
#ifdef CONFIG_PPPOE_PROXY_IF_NAME
		snprintf(in_interface, sizeof(in_interface), "! -i %s",LANIF_ALL);
#else
		snprintf(in_interface, sizeof(in_interface), "! -i %s", LANIF);
#endif
		}
#ifdef CONFIG_PPPOE_PROXY_IF_NAME
		snprintf(out_interface, sizeof(in_interface), "-o %s",LANIF_ALL);
#else
		snprintf(out_interface, sizeof(in_interface), "-o %s", LANIF);
#endif
#ifdef VIRTUAL_SERVER_MODE_NTO1
		if(Entry.mappingType == 1){
		//N-to-1
			snprintf(lan_dport, sizeof(lan_dport), "%u", Entry.lanPort);
			snprintf(lan_dport2, sizeof(lan_dport2), "%u", Entry.lanPort);
#ifdef CONFIG_NETFILTER_XT_MATCH_PSD
			sprintf(lan_dport_PSD, "--psd-specified-port %u", Entry.lanPort);
#endif
		}else
#endif
		{
			snprintf(lan_dport, sizeof(lan_dport), "%u-%u", Entry.lanPort, Entry.lanPort+(Entry.wanEndPort-Entry.wanStartPort));
			snprintf(lan_dport2, sizeof(lan_dport2), "%u:%u", Entry.lanPort, Entry.lanPort+(Entry.wanEndPort-Entry.wanStartPort));
			//lanPortStr_PSD[0] = '\0';
		}
#ifdef NAT_LOOPBACK
		if (Entry.lanPort)
		{
			snprintf(ipaddr, sizeof(ipaddr), "%s:%s", inet_ntoa(*((struct in_addr *)Entry.serverIp)), lan_dport);
			hasLocalPort = 1;
		}
		else
		{
			snprintf(ipaddr, sizeof(ipaddr), "%s", inet_ntoa(*((struct in_addr *)Entry.serverIp)));
			hasLocalPort = 0;
		}
		if (Entry.wanStartPort && Entry.wanEndPort && (Entry.wanStartPort != Entry.wanEndPort)){
			snprintf(extPort, sizeof(extPort), "%u:%u", Entry.wanStartPort, Entry.wanEndPort);
			hasExtPort = 1;
		}else if (Entry.wanStartPort) {
			snprintf(extPort, sizeof(extPort), "%u", Entry.wanStartPort);
			hasExtPort = 1;
		} else if (Entry.wanEndPort) {
			snprintf(extPort, sizeof(extPort), "%u", Entry.wanEndPort);
			hasExtPort = 1;
		} else {
			hasExtPort = 0;
		}
#endif
		if (Entry.protoType == PROTO_TCP || Entry.protoType == PROTO_UDPTCP)//0--TCP/UDP
		{

		/*ping_zhang:20080809 START:replace 'br0' with 'br+' in order to involve pppoe proxy connection*/
			// iptables -t nat -A virtual_server -i ! $LAN_IF -p TCP --dport srcPortRange -j DNAT --to-destination ipaddr --to-ports 8080
			DOCMDARGVS(IPTABLES, DOWAIT, "-t nat %s %s %s -p TCP %s --dport %u:%u -j DNAT --to-destination %s:%s ",VIRTUAL_SERVER_ACTION_APPEND(type),IPTABLE_VTLSUR,
				in_interface, remoteHost, Entry.wanStartPort, Entry.wanEndPort,	inet_ntoa(*((struct in_addr *)&(Entry.serverIp))),lan_dport);

			DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle %s %s %s -p TCP %s --dport %u:%u -j MARK --set-mark %s ",VIRTUAL_SERVER_ACTION_APPEND(type),IPTABLE_VTLSUR,
				in_interface, remoteHost, Entry.wanStartPort, Entry.wanEndPort,	RMACC_MARK);

			// iptables -A virtual_server 3 -i ! $LAN_IF -o $LAN_IF -p TCP --dport dstPortRange -j ACCEPT
			DOCMDARGVS(IPTABLES, DOWAIT, "%s %s %s %s -p TCP -d %s --dport %s -j ACCEPT",VIRTUAL_SERVER_ACTION_APPEND(type),IPTABLE_VTLSUR,
				in_interface, out_interface, inet_ntoa(*((struct in_addr *)&(Entry.serverIp))),lan_dport2);
	 	/*ping_zhang:20080809 END*/

		#ifdef NAT_LOOPBACK
			if(type == VIRTUAL_SERVER_ADD)
				iptable_virtual_server_natLB( 0, Entry.ifIndex, ARG_TCP, hasExtPort ? extPort : 0, hasLocalPort ? lan_dport2 : 0, ipaddr);
		#endif
#ifdef CONFIG_NETFILTER_XT_MATCH_PSD
			//iptables -A virtual_server -p TCP -m psd -j DROP --psd-weight-threshold 21 --psd-delay-threshold 3000 --psd-lo-ports-weight 3 --psd-hi-ports-weight 1 --psd-specified-port 21
			if(psd_enable){
#if defined(CONFIG_CMCC_BEIJING_GROUP_VERIFICATION)
				DOCMDARGVS(IPTABLES, DOWAIT, "%s %s %s %s -p TCP -m psd -j DROP %s",VIRTUAL_SERVER_ACTION_INSERT(type),IPTABLE_VTLSUR,
				in_interface, out_interface, lan_dport_PSD);
#else
				DOCMDARGVS(IPTABLES, DOWAIT, "%s %s %s %s -p TCP -m psd --psd-timeout-threshold 2000 -j DROP %s",VIRTUAL_SERVER_ACTION_INSERT(type),IPTABLE_VTLSUR,
				in_interface, out_interface, lan_dport_PSD);
#endif
			}
#endif
		}

		if (Entry.protoType == PROTO_UDP || Entry.protoType == PROTO_UDPTCP)
		{
	 	/*ping_zhang:20080809 START:replace 'br0' with 'br+' in order to involve pppoe proxy connection*/
			// iptables -t nat -A virtual_server -i ! $LAN_IF -p TCP --dport srcPortRange -j DNAT --to-destination ipaddr --to-ports 8080
			DOCMDARGVS(IPTABLES, DOWAIT, "-t nat %s %s %s -p UDP %s --dport %u:%u -j DNAT --to-destination %s:%s ",VIRTUAL_SERVER_ACTION_APPEND(type),IPTABLE_VTLSUR,
				in_interface, remoteHost, Entry.wanStartPort, Entry.wanEndPort,	inet_ntoa(*((struct in_addr *)&(Entry.serverIp))),lan_dport);

			DOCMDARGVS(IPTABLES, DOWAIT, "-t mangle %s %s %s -p UDP %s --dport %u:%u -j MARK --set-mark %s ",VIRTUAL_SERVER_ACTION_APPEND(type),IPTABLE_VTLSUR,
				in_interface, remoteHost, Entry.wanStartPort, Entry.wanEndPort,	RMACC_MARK);

			// iptables -A virtual_server -i ! $LAN_IF -o $LAN_IF -p TCP --dport dstPortRange -j ACCEPT
			DOCMDARGVS(IPTABLES, DOWAIT, "%s %s %s %s -p UDP -d %s --dport %s -j ACCEPT",VIRTUAL_SERVER_ACTION_INSERT(type),IPTABLE_VTLSUR,
				in_interface, out_interface, inet_ntoa(*((struct in_addr *)&(Entry.serverIp))),lan_dport2);
	 	/*ping_zhang:20080809 END*/

		#ifdef NAT_LOOPBACK
			if(type == VIRTUAL_SERVER_ADD)
				iptable_virtual_server_natLB( 0, Entry.ifIndex, ARG_UDP, hasExtPort ? extPort : 0, hasLocalPort ? lan_dport2 : 0, ipaddr);
		#endif
#ifdef CONFIG_NETFILTER_XT_MATCH_PSD
			//iptables -A virtual_server -p TCP -m psd -j DROP --psd-weight-threshold 21 --psd-delay-threshold 3000 --psd-lo-ports-weight 3 --psd-hi-ports-weight 1 --psd-specified-port 21
			if(psd_enable){
#if defined(CONFIG_CMCC_BEIJING_GROUP_VERIFICATION)
				DOCMDARGVS(IPTABLES, DOWAIT, "%s %s %s %s -p UDP -m psd -j DROP %s",VIRTUAL_SERVER_ACTION_INSERT(type),IPTABLE_VTLSUR,
				in_interface, out_interface, lan_dport_PSD);
#else
				DOCMDARGVS(IPTABLES, DOWAIT, "%s %s %s %s -p UDP -m psd --psd-timeout-threshold 2000 -j DROP %s",VIRTUAL_SERVER_ACTION_INSERT(type),IPTABLE_VTLSUR,
				in_interface, out_interface, lan_dport_PSD);
#endif
			}
#endif
		}
	}

	return 1;
}
#else
int setupVtlsvr(void)
{
	int i, total;
	MIB_CE_VTL_SVR_T Entry;
	char srcPortRange[12], dstPortRange[12], dstPort[12];
	char dstip[32];
	char ipAddr[20];
	//char *act;
	char ifname[16];

	total = mib_chain_total(MIB_VIRTUAL_SVR_TBL);
	// attach the vrtsvr rules to the chain for ip filtering

	for (i = 0; i < total; i++)
	{
		/*
		 *	srcPortRange: src port
		 *	dstPortRange: dst port
		 */
		if (!mib_chain_get(MIB_VIRTUAL_SVR_TBL, i, (void *)&Entry))
			return -1;

		// destination ip(server ip)
		strncpy(dstip, inet_ntoa(*((struct in_addr *)Entry.svrIpAddr)), 16);
		snprintf(ipAddr, 20, "%s/%d", dstip, 32);


		//wan port
		if (Entry.wanStartPort == 0)
			srcPortRange[0]='\0';
		else if (Entry.wanStartPort == Entry.wanEndPort)
			snprintf(srcPortRange, 12, "%u", Entry.wanStartPort);
		else
			snprintf(srcPortRange, 12, "%u:%u",
				Entry.wanStartPort, Entry.wanEndPort);

		// server port
		if (Entry.svrStartPort == 0)
			dstPortRange[0]='\0';
		else {
			if (Entry.svrStartPort == Entry.svrEndPort) {
				snprintf(dstPortRange, 12, "%u", Entry.svrStartPort);
				snprintf(dstPort, 12, "%u", Entry.svrStartPort);
			} else {
				snprintf(dstPortRange, 12, "%u-%u",
					Entry.svrStartPort, Entry.svrEndPort);
				snprintf(dstPort, 12, "%u:%u",
					Entry.svrStartPort, Entry.svrEndPort);
			}
			snprintf(dstip, 32, "%s:%s", dstip, dstPortRange);
		}

		//act = (char *)FW_ADD;
		// interface
		strcpy(ifname, LANIF);

		//printf("idx=%d\n", idx);
		if (Entry.protoType == PROTO_TCP || Entry.protoType == PROTO_UDPTCP)
		{
			// iptables -t nat -A PREROUTING ! -i $LAN_IF -p TCP --dport dstPortRange -j DNAT --to-destination ipaddr
			va_cmd(IPTABLES, 15, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "!", (char *)ARG_I,
				(char *)ifname,	"-p", (char *)ARG_TCP, (char *)FW_DPORT, srcPortRange, "-j", "DNAT",
				"--to-destination", dstip);

#ifdef IP_PORT_FILTER
			// iptables -I ipfilter 3 ! -i $LAN_IF -o $LAN_IF -p TCP --dport dstPortRange -j RETURN
			va_cmd(IPTABLES, 16, 1, (char *)FW_INSERT, (char *)FW_IPFILTER, "3", "!", (char *)ARG_I, (char *)ifname,
				(char *)ARG_O, (char *)ifname, "-p", (char *)ARG_TCP, (char *)FW_DPORT, dstPort, "-d",
				(char *)ipAddr, "-j",(char *)FW_RETURN);
#endif
		}
		if (Entry.protoType == PROTO_UDP || Entry.protoType == PROTO_UDPTCP)
		{
			va_cmd(IPTABLES, 15, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "!", (char *)ARG_I,
				(char *)ifname,	"-p", (char *)ARG_UDP, (char *)FW_DPORT, srcPortRange, "-j", "DNAT",
				"--to-destination", dstip);

#ifdef IP_PORT_FILTER
			// iptables -I ipfilter 3 -i ! $LAN_IF -o $LAN_IF -p UDP --dport dstPortRange -j RETURN
			va_cmd(IPTABLES, 16, 1, (char *)FW_INSERT, (char *)FW_IPFILTER, "3", "!", (char *)ARG_I, (char *)ifname,
				(char *)ARG_O, (char *)LANIF, "-p", (char *)ARG_UDP, (char *)FW_DPORT, dstPort, "-d",
				(char *)ipAddr, "-j",(char *)FW_RETURN);
#endif
		}
	}

	return 1;
}
#endif
#endif

#if defined(CONFIG_CU_BASEON_YUEME)
int delVirtualServerRule(int ifindex)
{
#ifdef VIRTUAL_SERVER_SUPPORT
	int total,i;
	struct vtlsvr_entryx Entry;
	total=mib_chain_total(MIB_VIRTUAL_SVR_TBL);

	for(i=total-1;i>=0;i--)
	{
		if (!mib_chain_get(MIB_VIRTUAL_SVR_TBL, i, (void *)&Entry))
			return -1;

		if (Entry.ifIndex!=ifindex)
			continue;

		mib_chain_delete(MIB_VIRTUAL_SVR_TBL,i);
	}
#endif

	return 1;
}
#endif

#if defined(IP_PORT_FILTER) || defined(MAC_FILTER)
void syslogMacFilterEntry(MIB_CE_MAC_FILTER_T Entry, int isAdd, char* user)
{
	const char *ract, *dir, *add;
	char tmpBuf[100], tmpBuf2[100];
	char message[1024];

	memset(message,0,sizeof(message));

	if(isAdd==1)
		add =  (char *)ARG_ADD;
	else
		add =  (char *)ARG_DEL;

#ifndef CONFIG_RTK_DEV_AP
	if (Entry.dir == DIR_OUT)
		dir = Toutgoing_ippfilter;
	else
		dir = Tincoming_ippfilter;
#endif

	if ( Entry.action == 0 )
		ract = Tdeny_ippfilter;
	else
		ract = Tallow_ippfilter;

	if ( Entry.srcMac[0]==0 && Entry.srcMac[1]==0
		&& Entry.srcMac[2]==0 && Entry.srcMac[3]==0
		&& Entry.srcMac[4]==0 && Entry.srcMac[5]==0 ) {
		strcpy(tmpBuf, "------");
	}else {
		snprintf(tmpBuf, 100, "%02x-%02x-%02x-%02x-%02x-%02x",
			Entry.srcMac[0], Entry.srcMac[1], Entry.srcMac[2],
			Entry.srcMac[3], Entry.srcMac[4], Entry.srcMac[5]);
	}

	if ( Entry.dstMac[0]==0 && Entry.dstMac[1]==0
		&& Entry.dstMac[2]==0 && Entry.dstMac[3]==0
		&& Entry.dstMac[4]==0 && Entry.dstMac[5]==0 ) {
		strcpy(tmpBuf2, "------");
	}else {
		snprintf(tmpBuf2, 100, "%02x-%02x-%02x-%02x-%02x-%02x",
			Entry.dstMac[0], Entry.dstMac[1], Entry.dstMac[2],
			Entry.dstMac[3], Entry.dstMac[4], Entry.dstMac[5]);
	}

#ifndef CONFIG_RTK_DEV_AP
	sprintf(message, "%s:%s, %s %s:%s, %s %s:%s, %s:%s",
		multilang(LANG_DIRECTION), dir,
		multilang(LANG_SOURCE), multilang(LANG_MAC_ADDRESS),tmpBuf,
		multilang(LANG_DESTINATION), multilang(LANG_MAC_ADDRESS), tmpBuf2,
		multilang(LANG_RULE_ACTION), ract);
#else
	sprintf(message, "%s %s:%s, %s %s:%s, %s:%s",
		multilang(LANG_SOURCE), multilang(LANG_MAC_ADDRESS),tmpBuf,
		multilang(LANG_DESTINATION), multilang(LANG_MAC_ADDRESS), tmpBuf2,
		multilang(LANG_RULE_ACTION), ract);
#endif
	syslog(LOG_INFO, "FW: %s %s MAC Filtering rule. Rule: %s\n", user, add, message);
}

void syslogIPPortFilterEntry(MIB_CE_IP_PORT_FILTER_T Entry, int isAdd, char* user)
{
	const char *dir, *ract, *add;
	char	*type, *sip, *dip;
	char	sipaddr[20], dipaddr[20], sportRange[20], dportRange[20];
	char message[1024];

	memset(message,0,sizeof(message));

	if(isAdd==1)
		add =  (char *)ARG_ADD;
	else
		add =  (char *)ARG_DEL;

	if (Entry.dir == DIR_OUT)
		dir = Toutgoing_ippfilter;
	else
		dir = Tincoming_ippfilter;

	// Modified by Mason Yu for Block ICMP packet
	if ( Entry.protoType == PROTO_ICMP )
	{
		type = (char *)ARG_ICMP;
	}
	else if ( Entry.protoType == PROTO_TCP )
		type = (char *)ARG_TCP;
	else
		type = (char *)ARG_UDP;

	sip = inet_ntoa(*((struct in_addr *)Entry.srcIp));
	if ( !strcmp(sip, "0.0.0.0"))
		sip = (char *)BLANK;
	else {
		if (Entry.smaskbit==0)
			snprintf(sipaddr, 20, "%s", sip);
		else
			snprintf(sipaddr, 20, "%s/%d", sip, Entry.smaskbit);
		sip = sipaddr;
	}

	if ( Entry.srcPortFrom == 0)
		strcpy(sportRange, BLANK);
	else if ( Entry.srcPortFrom == Entry.srcPortTo )
		snprintf(sportRange, 20, "%d", Entry.srcPortFrom);
	else
		snprintf(sportRange, 20, "%d-%d", Entry.srcPortFrom, Entry.srcPortTo);

	dip = inet_ntoa(*((struct in_addr *)Entry.dstIp));
	if ( !strcmp(dip, "0.0.0.0"))
		dip = (char *)BLANK;
	else {
		if (Entry.dmaskbit==0)
			snprintf(dipaddr, 20, "%s", dip);
		else
			snprintf(dipaddr, 20, "%s/%d", dip, Entry.dmaskbit);
		dip = dipaddr;
	}

	if ( Entry.dstPortFrom == 0)
		strcpy(dportRange, BLANK);
	else if ( Entry.dstPortFrom == Entry.dstPortTo )
		snprintf(dportRange, 20, "%d", Entry.dstPortFrom);
	else
		snprintf(dportRange, 20, "%d-%d", Entry.dstPortFrom, Entry.dstPortTo);

	if ( Entry.action == 0 )
		ract = Tdeny_ippfilter;
	else
		ract = Tallow_ippfilter;

	sprintf(message, "%s:%s, %s:%s, %s %s:%s, %s:%s, %s %s:%s, %s:%s, %s:%s",
		multilang(LANG_DIRECTION), dir,
		multilang(LANG_PROTOCOL), type,
		multilang(LANG_SOURCE), multilang(LANG_IP_ADDRESS),sip,
		multilang(LANG_SOURCE_PORT), sportRange,
		multilang(LANG_DESTINATION), multilang(LANG_IP_ADDRESS), dip,
		multilang(LANG_DESTINATION_PORT), dportRange,
		multilang(LANG_RULE_ACTION), ract);

	syslog(LOG_INFO, "FW: %s %s IP Port Filtering rule. Rule: %s\n", user, add, message);
}
#endif

#ifdef PORT_FORWARD_GENERAL
void syslogPortFwEntry(MIB_CE_PORT_FW_T Entry, int isAdd, char* user)
{
	char	*type, portRange[20]={0}, *ip, localIP[20]={0}, *add;
	char	remoteIP[20]={0}, remotePort[20];//, *fw_enable;
	int fw_enable;
	char extFromPort[8], extToPort[8];
	int interface_id=0;
	char interface_name[IFNAMSIZ];
	char message[1024];

	memset(message,0,sizeof(message));

	if(isAdd==1)
		add =  (char *)ARG_ADD;
	else
		add =  (char *)ARG_DEL;

	ip = inet_ntoa(*((struct in_addr *)Entry.ipAddr));
	strncpy( localIP, ip, sizeof(localIP)-1);
	if ( !strcmp(localIP, "0.0.0.0"))
		strncpy( localIP, "----", sizeof(localIP)-1);

	if ( Entry.protoType == PROTO_UDPTCP )
		type = "TCP+UDP";
	else if ( Entry.protoType == PROTO_TCP )
		type = "TCP";
	else
		type = "UDP";

	if ( Entry.fromPort == 0)
		strncpy(portRange, "----", sizeof(portRange)-1);
	else if ( Entry.fromPort == Entry.toPort )
		snprintf(portRange, 20, "%d", Entry.fromPort);
	else
		snprintf(portRange, 20, "%d-%d", Entry.fromPort, Entry.toPort);

	ip = inet_ntoa(*((struct in_addr *)Entry.remotehost));
	strncpy( remoteIP, ip ,sizeof(remoteIP)-1);
	if ( !strcmp(remoteIP, "0.0.0.0"))
		strcpy( remoteIP, "" );

	if ( Entry.externalfromport == 0)
		strcpy(remotePort, "----");
	else if ( Entry.externalfromport == Entry.externaltoport )
		snprintf(remotePort, 20, "%d", Entry.externalfromport);
	else
		snprintf(remotePort, 20, "%d-%d", Entry.externalfromport, Entry.externaltoport);

	if ( Entry.enable == 0 )
		fw_enable = LANG_DISABLE;
	else
		fw_enable = LANG_ENABLE;

	if ( Entry.ifIndex == DUMMY_IFINDEX )
	{
		strcpy( interface_name, "---" );
	}else {
		ifGetName(Entry.ifIndex, interface_name, sizeof(interface_name));
	}

//		snprintf(extPort, sizeof(extPort), "%u", Entry.externalport);
	snprintf(extFromPort, sizeof(extFromPort), "%u", Entry.externalfromport);
	snprintf(extToPort, sizeof(extToPort), "%u", Entry.externaltoport);

	sprintf(message, "%s:%s, %s %s:%s, %s:%s, %s:%s, %s:%s, %s:%s, %s:%s, %s:%s",
		multilang(LANG_COMMENT), Entry.comment,
		multilang(LANG_LOCAL), multilang(LANG_IP_ADDRESS),localIP,
		multilang(LANG_PROTOCOL), type,
		multilang(LANG_LOCAL_PORT), portRange,
		multilang(LANG_ENABLE), multilang(fw_enable),
		multilang(LANG_REMOTE_HOST), remoteIP,
		multilang(LANG_PUBLIC_PORT), remotePort,
		multilang(LANG_INTERFACE), interface_name);

	syslog(LOG_INFO, "FW: %s %s Port Porwarding rule. Rule: %s\n", user, add, message);
}
#endif

#ifdef DMZ
void syslogDMZ(char enable, char* ipaddr, char* user)
{
	int dmz_enable;
	char message[1024];

	memset(message,0,sizeof(message));
	if ( enable == 0 )
		dmz_enable = LANG_DISABLE;
	else
		dmz_enable = LANG_ENABLE;

	sprintf(message, "%s:%s, %s:%s",
		multilang(LANG_ENABLE), multilang(dmz_enable),
		multilang(LANG_IP_ADDRESS),ipaddr);

	syslog(LOG_INFO, "FW: %s change DMZ setting %s\n", user, message);
}
#endif

#ifdef URL_BLOCKING_SUPPORT
void syslogUrlFqdnEntry(MIB_CE_URL_FQDN_T Entry, int isAdd, char* user)
{
	char	*add;
	char message[1024];

	memset(message,0,sizeof(message));

	if(isAdd==1)
		add =  (char *)ARG_ADD;
	else
		add =  (char *)ARG_DEL;

	sprintf(message, "%s:%s",
		multilang(LANG_FQDN), Entry.fqdn);

	syslog(LOG_INFO, "FW: %s %s URL Blocking FQDN rule. Rule: %s\n", user, add, message);
}

void syslogUrlKeywdEntry(MIB_CE_KEYWD_FILTER_T Entry, int isAdd, char* user)
{
	char	*add;
	char message[1024];

	memset(message,0,sizeof(message));

	if(isAdd==1)
		add =  (char *)ARG_ADD;
	else
		add =  (char *)ARG_DEL;

	sprintf(message, "%s:%s",
		multilang(LANG_FILTERED_KEYWORD), Entry.keyword);

	syslog(LOG_INFO, "FW: %s %s URL Blocking Keyword rule. Rule: %s\n", user, add, message);
}
#endif	/* URL_BLOCKING_SUPPORT */

#ifdef DOMAIN_BLOCKING_SUPPORT
void syslogDomainBlockEntry(MIB_CE_DOMAIN_BLOCKING_T Entry, int isAdd, char* user)
{
	char	*add;
	char message[1024];
	unsigned char sdest[MAX_DOMAIN_LENGTH];

	memset(message,0,sizeof(message));

	if(isAdd==1)
		add =  (char *)ARG_ADD;
	else
		add =  (char *)ARG_DEL;

	strncpy(sdest, Entry.domain, sizeof(sdest));
		sdest[strlen(Entry.domain)] = '\0';

	sprintf(message, "%s:%s",
		multilang(LANG_DOMAIN), sdest);

	syslog(LOG_INFO, "FW: %s %s Domain Blocking rule. Rule: %s\n", user, add, message);
}
#endif


#if defined(CONFIG_LUNA) && defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_rate.h>
#include <rtk/switch.h>
#include <rtk/rate.h>

void rtk_fw_setup_storm_control(void)
{
	int portId=0;
	unsigned int uumId=0;
	unsigned int bcmId=0;
	unsigned int mcmId=0;
	unsigned int unmcmId=0;
	unsigned int dhmId=0;
	unsigned int igmpmldmId=0;
	unsigned int meter_rate=0;
	unsigned int fix_rate = 500;
	int ret=0;
	rtk_switch_devInfo_t devInfo;

	if((ret = rtk_switch_deviceInfo_get (&devInfo)) != RT_ERR_OK)
		printf("[%s-%d] ret =%x\n",__func__,__LINE__,ret);

	// set meter type & rate for UNKNOWN UNICAST
	mib_get_s(MIB_STORM_UNKNOWN_UNICAST_RATE, (void *)&meter_rate, sizeof(meter_rate));
	ret = rt_rate_shareMeterType_add(RT_METER_TYPE_STORM, &uumId);
	ret = rt_rate_shareMeterRate_set(uumId, meter_rate);

	// set meter type & rate for BROCAST
	mib_get_s(MIB_STORM_BROCAST_RATE, (void *)&meter_rate, sizeof(meter_rate));
	ret = rt_rate_shareMeterType_add(RT_METER_TYPE_STORM, &bcmId);
	ret = rt_rate_shareMeterRate_set(bcmId, meter_rate);

	// set meter type & rate for MULTICAST
	// mib_get_s(MIB_STORM_MULTICAST_RATE, (void *)&fix_rate, sizeof(fix_rate));
	ret = rt_rate_shareMeterType_add(RT_METER_TYPE_STORM, &mcmId);
	ret = rt_rate_shareMeterRate_set(mcmId, fix_rate);

	// set meter type & rate for UNKNOWN MULTICAST
	meter_rate = 0;
	mib_get_s(MIB_STORM_UNKNOWN_MULTICAST_RATE, (void *)&meter_rate, sizeof(meter_rate));
	ret = rt_rate_shareMeterType_add(RT_METER_TYPE_STORM, &unmcmId);
	ret = rt_rate_shareMeterRate_set(unmcmId, meter_rate);

	// set meter type & rate for DHCP
	meter_rate = 0;
	mib_get_s(MIB_STORM_DHCP_RATE, (void *)&meter_rate, sizeof(meter_rate));
	ret = rt_rate_shareMeterType_add(RT_METER_TYPE_STORM, &dhmId);
	ret = rt_rate_shareMeterRate_set(dhmId, meter_rate);

	// set meter type & rate for IGMP & MLD
	meter_rate = 0;
	mib_get_s(MIB_STORM_IGMP_MLD_RATE, (void *)&meter_rate, sizeof(meter_rate));
	ret = rt_rate_shareMeterType_add(RT_METER_TYPE_STORM, &igmpmldmId);
	ret = rt_rate_shareMeterRate_set(igmpmldmId, meter_rate);

	// set bucket
	ret = rt_rate_shareMeterBucket_set (bcmId, 1496);

	for (portId = devInfo.ether.min; portId <= devInfo.ether.max; portId++)
	{
		if (portId>=32)
			break;

		if (devInfo.ether.portmask.bits[0] & 1<<portId)
		{
			// set meter
			ret = rt_rate_stormControlMeterIdx_set(portId, RT_STORM_GROUP_UNKNOWN_UNICAST, uumId);
			ret = rt_rate_stormControlMeterIdx_set(portId, RT_STORM_GROUP_BROADCAST, bcmId);
			//ret = rt_rate_stormControlMeterIdx_set(portId, RT_STORM_GROUP_MULTICAST, mcmId);
			ret = rt_rate_stormControlMeterIdx_set(portId, RT_STORM_GROUP_UNKNOWN_MULTICAST, unmcmId);
			ret = rt_rate_stormControlMeterIdx_set(portId, RT_STORM_GROUP_DHCP, dhmId);
			ret = rt_rate_stormControlMeterIdx_set(portId, RT_STORM_GROUP_IGMP_MLD, igmpmldmId);

			// set status
			ret = rt_rate_stormControlPortEnable_set(portId, RT_STORM_GROUP_UNKNOWN_UNICAST, ENABLED);
			ret = rt_rate_stormControlPortEnable_set(portId, RT_STORM_GROUP_BROADCAST, ENABLED);
			//ret = rt_rate_stormControlPortEnable_set(portId, RT_STORM_GROUP_MULTICAST, ENABLED);
			ret = rt_rate_stormControlPortEnable_set(portId, RT_STORM_GROUP_UNKNOWN_MULTICAST, ENABLED);
			ret = rt_rate_stormControlPortEnable_set(portId, RT_STORM_GROUP_DHCP, ENABLED);
			ret = rt_rate_stormControlPortEnable_set(portId, RT_STORM_GROUP_IGMP_MLD, ENABLED);
		}
	}
	ret = rt_rate_stormControlMeterIdx_set(7, RT_STORM_GROUP_UNKNOWN_UNICAST, uumId);
	ret = rt_rate_stormControlPortEnable_set(7, RT_STORM_GROUP_UNKNOWN_UNICAST, ENABLED);

	ret = rt_rate_stormControlMeterIdx_set(5, RT_STORM_GROUP_MULTICAST, mcmId);
}

void rtk_fw_setup_arp_storm_control(void)
{
	int portId=0;
	unsigned int meterId=0;
	unsigned int bcmeterId=0;
	//unsigned char meter_mode=0;
	unsigned int meter_rate=0;
	int ret=0;
	rtk_switch_devInfo_t devInfo;

	if((ret = rtk_switch_deviceInfo_get (&devInfo)) != RT_ERR_OK)
		printf("[%s-%d] ret =%x\n",__func__,__LINE__,ret);

	mib_get_s(MIB_STORM_ARP_RATE, (void *)&meter_rate, sizeof(meter_rate));

	if((ret = rt_rate_shareMeterType_add(RT_METER_TYPE_STORM, &meterId)) != RT_ERR_OK)
		printf("[%s-%d] ret =%x\n",__func__,__LINE__,ret);
	//printf("==--=-=-=-rate:%u meterId:%u\n",meter_rate,meterId);
	if((ret = rt_rate_shareMeterRate_set (meterId, meter_rate))!= RT_ERR_OK)
		printf("[%s-%d] ret =%x\n",__func__,__LINE__,ret);

	for (portId = devInfo.ether.min; portId <= devInfo.ether.max; portId++)
	{
		if(portId>=32) break;

		if (devInfo.ether.portmask.bits[0] & 1<<portId)
		{
			if((ret = rt_rate_stormControlMeterIdx_set(portId, RT_STORM_GROUP_ARP, meterId)) != RT_ERR_OK)
				printf("[%s-%d] ret =%x\n",__func__,__LINE__,ret);
			if((ret = rt_rate_stormControlPortEnable_set (portId, RT_STORM_GROUP_ARP, ENABLED)) != RT_ERR_OK)
				printf("[%s-%d] ret =%x\n",__func__,__LINE__,ret);
		}
	}
}
#endif

#ifdef CONFIG_USER_WIRED_8021X
int rtk_wired_8021x_clean(int isPortNeedDownUp)
{
	unsigned char chainName[128];
#ifdef CONFIG_RTL_SMUX_DEV
	char rule_alias_name[256];
#endif
	int i;

	printf("%s %d: enter\n", __func__, __LINE__);

	for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
	{
		if(i>=ELANVIF_NUM)
			continue;

		sprintf(chainName, "%s_%s", FW_WIRED_8021X_PREROUTING, ELANVIF[i]);
		va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_WIRED_8021X_PREROUTING, "-j", (char *)chainName);
		va_cmd(EBTABLES, 4, 1, "-t", "nat", "-X", (char *)chainName);
		sprintf(chainName, "%s_%s", FW_WIRED_8021X_POSTROUTING, ELANVIF[i]);
		va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_WIRED_8021X_POSTROUTING, "-j", (char *)chainName);
		va_cmd(EBTABLES, 4, 1, "-t", "nat", "-X", (char *)chainName);
#ifdef CONFIG_RTL_SMUX_DEV
		//smuxctl --rx --tags 0 --if eth0.x --rule-remove-alias wired_8021x_default_eth0.x
		sprintf(rule_alias_name, "wired_8021x_default_%s", ELANRIF[i]);
		va_cmd(SMUXCTL, 7, 1, "--rx", "--tags", "0", "--if", ELANRIF[i], "--rule-remove-alias", (char *)rule_alias_name);
#endif
	}
	//ebtables -t nat -D PREROUTING -j wired_8021x_prerouting
	va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", (char *)FW_WIRED_8021X_PREROUTING);
	//ebtables -t nat -D POSTROUTING -j wired_8021x_postrouting
	va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_POSTROUTING, "-j", (char *)FW_WIRED_8021X_POSTROUTING);
	//ebtables -t nat -X wired_8021x_prerouting
	va_cmd(EBTABLES, 4, 1, "-t", "nat", "-X", (char *)FW_WIRED_8021X_PREROUTING);
	//ebtables -t nat -X wired_8021x_postrouting
	va_cmd(EBTABLES, 4, 1, "-t", "nat", "-X", (char *)FW_WIRED_8021X_POSTROUTING);
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_wired_8021x_clean(isPortNeedDownUp);
#endif
	return 0;
}

int rtk_wired_8021x_setup(void)
{
	unsigned char wired8021xMode;
	unsigned char wired8021xEnable[MAX_LAN_PORT_NUM] = {0};
	unsigned char chainName[128];
	int i, j, existWired8021xPort = 0;
	char rule_alias_name[256];

	printf("%s %d: enter\n", __func__, __LINE__);

	if(!mib_get(MIB_WIRED_8021X_ENABLE, (void *)wired8021xEnable))
	{
		printf("%s %d: get mib MIB_WIRED_8021X_ENABLE FAIL.\n", __func__, __LINE__);
		return -1;
	}

	for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
	{
		if(wired8021xEnable[i])
			existWired8021xPort = 1;
	}

	if(existWired8021xPort)
	{
		//ebrables -t nat -N wired_8021x_prerouting
		va_cmd(EBTABLES, 4, 1, "-t", "nat", "-N", (char *)FW_WIRED_8021X_PREROUTING);
		//ebrables -t nat -N wired_8021x_postrouting
		va_cmd(EBTABLES, 4, 1, "-t", "nat", "-N", (char *)FW_WIRED_8021X_POSTROUTING);
		//ebrables -t nat -I PREROUTING -j wired_8021x_prerouting
		va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-j", (char *)FW_WIRED_8021X_PREROUTING);
		//ebrables -t nat -I POTROUTING -j wired_8021x_postrouting
		va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_POSTROUTING, "-j", (char *)FW_WIRED_8021X_POSTROUTING);

		for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
		{
			if(i>=ELANVIF_NUM)
				continue;

			if(wired8021xEnable[i])
			{
				//ebtables -t nat -I wired_8021x_prerouting -i eth0.x -p 0x888e -j ACCEPT
				va_cmd(EBTABLES, 10, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_WIRED_8021X_PREROUTING, "-i", ELANVIF[i], "-p", "0x888e", "-j", (char *)FW_ACCEPT);
				//ebtables -t nat -I wired_8021x_postrouting -o eth0.x -p 0x888e -j ACCEPT
				va_cmd(EBTABLES, 10, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_WIRED_8021X_POSTROUTING, "-o", ELANVIF[i], "-p", "0x888e", "-j", (char *)FW_ACCEPT);
				if(mib_get(MIB_WIRED_8021X_MODE, (void *)&wired8021xMode) != 0)
				{
					if(wired8021xMode == WIRED_8021X_MODE_MAC_BASED)
					{
						va_cmd(EBTABLES, 10, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_WIRED_8021X_POSTROUTING, "-o", ELANVIF[i], "-d", "01:00:00:00:00:00/01:00:00:00:00:00", "-j", (char *)FW_ACCEPT);
					}
				}
				//ebtables -t nat -A wired_8021x_prerouting -i eth0.x -j DROP
				va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_WIRED_8021X_PREROUTING, "-i", ELANVIF[i], "-j", (char *)FW_DROP);
				//ebtables -t nat -A wired_8021x_postrouting -o eth0.x -j DROP
				va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_WIRED_8021X_POSTROUTING, "-o", ELANVIF[i], "-j", (char *)FW_DROP);
#ifdef CONFIG_RTL_SMUX_DEV
				//smuxctl --rx --tags 0 --if eth0.x --filter-ethertype 0x888E --filter-multicast --set-rxif eth0.2.0 --rule-priority 1000 --duplicate-forward --rule-alias wired_8021x_default_eth0.x --rule-append
				sprintf(rule_alias_name, "wired_8021x_default_%s", ELANRIF[i]);
				va_cmd(SMUXCTL, 16, 1, "--rx", "--tags", "0", "--if", ELANRIF[i], "--filter-ethertype", "0x888e", "--filter-multicast", "--set-rxif", ELANVIF[i], "--rule-priority", "1000", "--duplicate-forward", "--rule-alias", rule_alias_name, "--rule-append");
#endif
			}
		}

		for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
		{
			if(i>=ELANVIF_NUM)
				continue;

			if(wired8021xEnable[i])
			{
				sprintf(chainName, "%s_%s", FW_WIRED_8021X_PREROUTING, ELANVIF[i]);
				//ebtables -t nat -N wired_8021x_preroute_eth0.x
				va_cmd(EBTABLES, 4, 1, "-t", "nat", "-N", (char *)chainName);
				//ebtables -t nat -P wired_8021x_preroute_eth0.x RETURN
				va_cmd(EBTABLES, 5, 1, "-t", "nat", "-P", (char *)chainName, FW_RETURN);
				//ebtables -t nat -I wired_8021x_preroute -j wired_8021x_preroute_eth0.x
				va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_WIRED_8021X_PREROUTING, "-j", (char *)chainName);
				sprintf(chainName, "%s_%s", FW_WIRED_8021X_POSTROUTING, ELANVIF[i]);
				//ebtables -t nat -N wired_8021x_postroute_eth0.x
				va_cmd(EBTABLES, 4, 1, "-t", "nat", "-N", (char *)chainName);
				//ebtables -t nat -P wired_8021x_postroute_eth0.x RETURN
				va_cmd(EBTABLES, 5, 1, "-t", "nat", "-P", (char *)chainName, FW_RETURN);
				//ebtables -t nat -I wired_8021x_postroute -j wired_8021x_postroute_eth0.x
				va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_WIRED_8021X_POSTROUTING, "-j", (char *)chainName);
			}
		}
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		rtk_fc_wired_8021x_setup();
#endif
	}

	return 0;
}

int rtk_wired_8021x_set_mode(WIRED_8021X_MODE_T mode)
{
	unsigned char wired8021xMode;
	int i;

	printf("%s %d: enter\n", __func__, __LINE__);

	if(mib_get(MIB_WIRED_8021X_MODE, (void *)&wired8021xMode) != 0)
	{
		if(wired8021xMode != mode)
		{
			wired8021xMode = mode;
			mib_set(MIB_WIRED_8021X_MODE, (void *)&wired8021xMode);
#ifdef COMMIT_IMMEDIATELY
			Commit();
#endif
			rtk_wired_8021x_clean(1);
			rtk_wired_8021x_setup();
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

int rtk_wired_8021x_set_enable(unsigned int portmask)
{
	unsigned char wired8021xEnable[MAX_LAN_PORT_NUM] = {0};
	int i, changed=0;

	printf("%s %d: enter\n", __func__, __LINE__);

	if(!mib_get(MIB_WIRED_8021X_ENABLE, (void *)wired8021xEnable))
	{
		printf("%s %d: get mib MIB_WIRED_8021X_ENABLE FAIL.\n", __func__, __LINE__);
		return -1;
	}

	for(i=0 ; i<SW_LAN_PORT_NUM ; i++)
	{
		if((portmask&(1<<i)))
		{
			if(wired8021xEnable[i] != 1)
			{
				wired8021xEnable[i] = 1;
				changed = 1;
			}
		}
		else
		{
			if(wired8021xEnable[i] != 0)
			{
				wired8021xEnable[i] = 0;
				changed = 1;
			}
		}
	}

	if(changed)
	{
		mib_set(MIB_WIRED_8021X_ENABLE, (void *)wired8021xEnable);
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
		rtk_wired_8021x_clean(1);
		rtk_wired_8021x_setup();
	}
	return 0;
}

int rtk_wired_8021x_set_auth(int portid, unsigned char *mac)
{
	unsigned char wired8021xMode;
	unsigned char wired8021xEnable[MAX_LAN_PORT_NUM] = {0};
	unsigned char macStr[18], chainName[128];
	int i;

	printf("%s %d: enter\n", __func__, __LINE__);

	if((portid == -1 || portid >= SW_LAN_PORT_NUM) && (mac==NULL))
	{
		printf("%s %d: invalid portid && MAC !\n", __func__, __LINE__);
		return -1;
	}

	if(!mib_get(MIB_WIRED_8021X_ENABLE, (void *)wired8021xEnable))
	{
		printf("%s %d: get mib MIB_WIRED_8021X_ENABLE FAIL.\n", __func__, __LINE__);
		return -1;
	}

	if(mib_get(MIB_WIRED_8021X_MODE, (void *)&wired8021xMode) != 0)
	{
		if(mac==NULL)
		{
			if(wired8021xMode != WIRED_8021X_MODE_PORT_BASED)
			{
				printf("%s %d: wired 802.1x is in MAC based mode, MAC parameter should not be NULL !\n", __func__, __LINE__);
				return -1;
			}

			if(!wired8021xEnable[portid])
			{
				printf("%s %d: wired 802.1x of this port=%d was disabled.\n", __func__, __LINE__, portid);
				return -1;
			}

			printf("%s %d: portid=%d\n", __func__, __LINE__, portid);
			//ebtables -t nat -D wired_8021x_prerouting -i eth0.x -j ACCEPT
			va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_WIRED_8021X_PREROUTING, "-i", ELANVIF[portid], "-j", (char *)FW_ACCEPT);
			//ebtables -t nat -D wired_8021x_postrouting -o eth0.x -j ACCEPT
			va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_WIRED_8021X_POSTROUTING, "-o", ELANVIF[portid], "-j", (char *)FW_ACCEPT);
			//ebtables -t nat -I wired_8021x_prerouting -i eth0.x -j ACCEPT
			va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_WIRED_8021X_PREROUTING, "-i", ELANVIF[portid], "-j", (char *)FW_ACCEPT);
			//ebtables -t nat -I wired_8021x_postrouting -o eth0.x -j ACCEPT
			va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_WIRED_8021X_POSTROUTING, "-o", ELANVIF[portid], "-j", (char *)FW_ACCEPT);
		}
		else
		{
			if(wired8021xMode != WIRED_8021X_MODE_MAC_BASED)
			{
				printf("%s %d: wired 802.1x is in port based mode, MAC parameter should be NULL !\n", __func__, __LINE__);
				return -1;
			}

			if(portid != -1 && portid < SW_LAN_PORT_NUM)
			{

				printf("%s %d: portid=%d MAC=%02X:%02X:%02X:%02X:%02X:%02X\n", __func__, __LINE__, portid
					, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				snprintf(macStr, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
					mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				//ebtables -t nat -D wired_8021x_prerouting -s 00:11:22:33:44:55 -j ACCEPT
				sprintf(chainName, "%s_%s", FW_WIRED_8021X_PREROUTING, ELANVIF[portid]);
				va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_DEL, (char *)chainName, "-s", macStr, "-j", (char *)FW_ACCEPT);
				//ebtables -t nat -D wired_8021x_postrouting -d 00:11:22:33:44:55 -j ACCEPT
				sprintf(chainName, "%s_%s", FW_WIRED_8021X_POSTROUTING, ELANVIF[portid]);
				va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_DEL, (char *)chainName, "-d", macStr, "-j", (char *)FW_ACCEPT);
				//ebtables -t nat -I wired_8021x_prerouting -s 00:11:22:33:44:55 -j ACCEPT
				sprintf(chainName, "%s_%s", FW_WIRED_8021X_PREROUTING, ELANVIF[portid]);
				va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_INSERT, (char *)chainName, "-s", macStr, "-j", (char *)FW_ACCEPT);
				//ebtables -t nat -D wired_8021x_postrouting -d 00:11:22:33:44:55 -j ACCEPT
				sprintf(chainName, "%s_%s", FW_WIRED_8021X_POSTROUTING, ELANVIF[portid]);
				va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_INSERT, (char *)chainName, "-d", macStr, "-j", (char *)FW_ACCEPT);
			}
			else
			{
				printf("%s %d: MAC=%02X:%02X:%02X:%02X:%02X:%02X\n", __func__, __LINE__
					, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				snprintf(macStr, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
					mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				//ebtables -t nat -D wired_8021x_prerouting -s 00:11:22:33:44:55 -j ACCEPT
				va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_WIRED_8021X_PREROUTING, "-s", macStr, "-j", (char *)FW_ACCEPT);
				//ebtables -t nat -D wired_8021x_postrouting -d 00:11:22:33:44:55 -j ACCEPT
				va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_WIRED_8021X_POSTROUTING, "-d", macStr, "-j", (char *)FW_ACCEPT);
				//ebtables -t nat -I wired_8021x_prerouting -s 00:11:22:33:44:55 -j ACCEPT
				va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_WIRED_8021X_PREROUTING, "-s", macStr, "-j", (char *)FW_ACCEPT);
				//ebtables -t nat -D wired_8021x_postrouting -d 00:11:22:33:44:55 -j ACCEPT
				va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_WIRED_8021X_POSTROUTING, "-d", macStr, "-j", (char *)FW_ACCEPT);
			}
		}
	}
	else
	{
		printf("%s %d: get mib MIB_WIRED_8021X_MODE FAIL.\n", __func__, __LINE__);
		return -1;
	}

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_wired_8021x_set_auth(portid, mac);
#endif
	return 0;
}

int rtk_wired_8021x_set_unauth(int portid, unsigned char *mac)
{
	unsigned char wired8021xMode;
	unsigned char wired8021xEnable[MAX_LAN_PORT_NUM] = {0};
	unsigned char macStr[18], chainName[128];
	int i;

	printf("%s %d: enter\n", __func__, __LINE__);

	if((portid == -1 || portid >= SW_LAN_PORT_NUM) && (mac==NULL))
	{
		printf("%s %d: invalid portid && MAC !\n", __func__, __LINE__);
		return -1;
	}

	if(!mib_get(MIB_WIRED_8021X_ENABLE, (void *)wired8021xEnable))
	{
		printf("%s %d: get mib MIB_WIRED_8021X_ENABLE FAIL.\n", __func__, __LINE__);
		return -1;
	}

	if(mib_get(MIB_WIRED_8021X_MODE, (void *)&wired8021xMode) != 0)
	{
		if(mac==NULL)
		{
			if(wired8021xMode != WIRED_8021X_MODE_PORT_BASED)
			{
				printf("%s %d: wired 802.1x is in MAC based mode, MAC parameter should not be NULL !\n", __func__, __LINE__);
				return -1;
			}

			if(!wired8021xEnable[portid])
			{
				printf("%s %d: wired 802.1x of this port=%d was disabled.\n", __func__, __LINE__, portid);
				return -1;
			}

			printf("%s %d: portid=%d\n", __func__, __LINE__, portid);
			//ebtables -t nat -D wired_8021x_prerouting -i eth0.x -j ACCEPT
			va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_WIRED_8021X_PREROUTING, "-i", ELANVIF[portid], "-j", (char *)FW_ACCEPT);
			//ebtables -t nat -D wired_8021x_postrouting -o eth0.x -j ACCEPT
			va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_WIRED_8021X_POSTROUTING, "-o", ELANVIF[portid], "-j", (char *)FW_ACCEPT);
		}
		else
		{
			if(wired8021xMode != WIRED_8021X_MODE_MAC_BASED)
			{
				printf("%s %d: wired 802.1x is in port based mode, MAC parameter should be NULL !\n", __func__, __LINE__);
				return -1;
			}

			if(portid != -1 && portid < SW_LAN_PORT_NUM)
			{
				printf("%s %d: portid=%d MAC=%02X:%02X:%02X:%02X:%02X:%02X\n", __func__, __LINE__, portid
					, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				snprintf(macStr, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
					mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				//ebtables -t nat -D wired_8021x_prerouting_eth0.x -s 00:11:22:33:44:55 -j ACCEPT
				sprintf(chainName, "%s_%s", FW_WIRED_8021X_PREROUTING, ELANVIF[portid]);
				va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_DEL, (char *)chainName, "-s", macStr, "-j", (char *)FW_ACCEPT);
				//ebtables -t nat -D wired_8021x_postrouting_eth0.x -d 00:11:22:33:44:55 -j ACCEPT
				sprintf(chainName, "%s_%s", FW_WIRED_8021X_POSTROUTING, ELANVIF[portid]);
				va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_DEL, (char *)chainName, "-d", macStr, "-j", (char *)FW_ACCEPT);
			}
			else
			{
				printf("%s %d: MAC=%02X:%02X:%02X:%02X:%02X:%02X\n", __func__, __LINE__
					, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				snprintf(macStr, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
					mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				//ebtables -t nat -D wired_8021x_prerouting -s 00:11:22:33:44:55 -j ACCEPT
				va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_WIRED_8021X_PREROUTING, "-s", macStr, "-j", (char *)FW_ACCEPT);
				//ebtables -t nat -D wired_8021x_postrouting -d 00:11:22:33:44:55 -j ACCEPT
				va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_WIRED_8021X_POSTROUTING, "-d", macStr, "-j", (char *)FW_ACCEPT);
			}
		}
	}
	else
	{
		printf("%s %d: get mib MIB_WIRED_8021X_MODE FAIL.\n", __func__, __LINE__);
		return -1;
	}

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_wired_8021x_set_unauth(portid, mac);
#endif
	return 0;
}

int rtk_wired_8021x_flush_port(int portid)
{
	unsigned char wired8021xMode;
	unsigned char macStr[18], chainName[128];
	int i;

	printf("%s %d: enter\n", __func__, __LINE__);

	if((portid == -1 || portid >= SW_LAN_PORT_NUM))
	{
		printf("%s %d: invalid portid !\n", __func__, __LINE__);
		return -1;
	}

	if(mib_get(MIB_WIRED_8021X_MODE, (void *)&wired8021xMode) != 0)
	{
		printf("%s %d: portid=%d\n", __func__, __LINE__, portid);
		if(wired8021xMode == WIRED_8021X_MODE_PORT_BASED)
		{
			//ebtables -t nat -D wired_8021x_prerouting -i eth0.x -j ACCEPT
			va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_WIRED_8021X_PREROUTING, "-i", ELANVIF[portid], "-j", (char *)FW_ACCEPT);
			//ebtables -t nat -D wired_8021x_postrouting -o eth0.x -j ACCEPT
			va_cmd(EBTABLES, 8, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_WIRED_8021X_POSTROUTING, "-o", ELANVIF[portid], "-j", (char *)FW_ACCEPT);
		}
		else
		{
			//ebtables -t nat -F wired_8021x_preroute_eth0.x
			sprintf(chainName, "%s_%s", FW_WIRED_8021X_PREROUTING, ELANVIF[portid]);
			va_cmd(EBTABLES, 4, 1, "-t", "nat", (char *)"-F", (char *)chainName);
			//ebtables -t nat -F wired_8021x_postroute_eth0.x
			sprintf(chainName, "%s_%s", FW_WIRED_8021X_POSTROUTING, ELANVIF[portid]);
			va_cmd(EBTABLES, 4, 1, "-t", "nat", (char *)"-F", (char *)chainName);
		}
	}

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_wired_8021x_flush_port(portid);
#endif
	return 0;
}

int rtk_wired_8021x_phyport_linkdown(char *ifName)
{
	int i;

	for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
	{
		if(!strcmp(ELANVIF[i], ifName))
		{
			return rtk_wired_8021x_flush_port(i);
		}
	}
}

int rtk_wired_8021x_take_effect(void)
{
	unsigned char wired8021xEnable[MAX_LAN_PORT_NUM] = {0};
	int hostapd_pid, i, existWired8021xPort = 0;
	unsigned short wired8021xRadiasUdpPort;
	unsigned char rsPassword[MAX_PSK_LEN+1];
	unsigned char rsIpStr[32];
	char path[100] = {0}, allPath[300] = {0};
	char cmd[512] = {0};
	FILE *fp = NULL;

	printf("%s %d: enter\n", __func__, __LINE__);

	hostapd_pid = read_pid(HOSTAPDPID);
	if (hostapd_pid > 0) {
		printf("%s %d: hostapd_pid=%d\n", __func__, __LINE__, hostapd_pid);
		kill(hostapd_pid, SIGKILL);
	}
	unlink(HOSTAPDPID);

	if(!mib_get(MIB_WIRED_8021X_ENABLE, (void *)wired8021xEnable))
	{
		printf("%s %d: get mib MIB_WIRED_8021X_ENABLE FAIL.\n", __func__, __LINE__);
		return -1;
	}

	for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
	{
		sprintf(path, "%s.%s", HOSTAPD_SCRIPT_NAME, ELANVIF[i]);
		unlink(path);

		if(wired8021xEnable[i])
			existWired8021xPort = 1;
	}

	if(existWired8021xPort)
	{
		if(!mib_get(MIB_WIRED_8021X_RADIAS_IP, (void *)rsIpStr))
		{
			printf("%s %d: mib_get MIB_WIRED_8021X_RADIAS_IP FAIL.\n", __func__, __LINE__);
			return -1;
		}

		if(!mib_get(MIB_WIRED_8021X_RADIAS_UDP_PORT, (void *)&wired8021xRadiasUdpPort))
		{
			printf("%s %d: mib_get MIB_WIRED_8021X_RADIAS_UDP_PORT FAIL.\n", __func__, __LINE__);
			return -1;
		}

		if(!mib_get(MIB_WIRED_8021X_RADIAS_SECRET, (void *)rsPassword))
		{
			printf("%s %d: mib_get MIB_WIRED_8021X_RADIAS_SECRET FAIL.\n", __func__, __LINE__);
			return -1;
		}

		for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
		{
			if(wired8021xEnable[i])
			{
				sprintf(path, "%s.%s", HOSTAPD_SCRIPT_NAME, ELANVIF[i]);
				if(!(fp = fopen(path, "a+")))
				{
					printf("%s %d: open %s FAIL.\n", __func__, __LINE__, path);
					return -1;
				}
				fprintf(fp, "interface=%s\n", ELANVIF[i]);
				fprintf(fp, "driver=wired_rtk\n");
				fprintf(fp, "logger_syslog=-1\n");
				fprintf(fp, "logger_syslog_level=2\n");
				fprintf(fp, "logger_stdout=-1\n");
				fprintf(fp, "logger_stdout_level=2\n");
				fprintf(fp, "ctrl_interface=/var/run/hostapd\n");
				fprintf(fp, "ctrl_interface_group=0\n");
				fprintf(fp, "max_num_sta=255\n");
				fprintf(fp, "rts_threshold=-1\n");
				fprintf(fp, "fragm_threshold=-1\n");
				fprintf(fp, "macaddr_acl=0\n");
				fprintf(fp, "auth_algs=3\n");
				fprintf(fp, "ignore_broadcast_ssid=0\n");
				fprintf(fp, "wmm_enabled=0\n");
				fprintf(fp, "ieee8021x=1\n");
				fprintf(fp, "auth_server_addr=%s\n", inet_ntoa(*((struct in_addr *)rsIpStr)));
				fprintf(fp, "auth_server_port=%d\n", wired8021xRadiasUdpPort);
				fprintf(fp, "auth_server_shared_secret=%s\n", rsPassword);
				fprintf(fp, "acct_server_addr=%s\n", inet_ntoa(*((struct in_addr *)rsIpStr)));
				fprintf(fp, "acct_server_port=%d\n", wired8021xRadiasUdpPort);
				fprintf(fp, "acct_server_shared_secret=%s\n", rsPassword);
				fclose(fp);
				if(allPath[0]=='\0')
					snprintf(allPath, 300, "%s", path);
               	else
					snprintf(allPath, 300, "%s %s", allPath, path);
			}
		}

		sprintf(cmd, "%s %s", HOSTAPD, allPath);
		va_niced_cmd("/bin/sh", 2, 0, "-c", cmd);
	}

	return 0;
}
#endif

#if defined(CONFIG_RTK_CTCAPD_FILTER_SUPPORT)
int rtk_ctcapd_url_filter_set_mode(unsigned int url_filter_mode)
{
	unsigned char FilterMode = 0;

	FilterMode = url_filter_mode;
	//printf("%s:%d FilterMode:%d\n",__FUNCTION__,__LINE__,FilterMode);

	if(!mib_set(MIB_URLFILTER_MODE, (void *)&FilterMode)){
		printf("%s:%d set url filter table fail!!\n",__FUNCTION__,__LINE__);
		return -1;
	}

	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);

	if(FilterMode==URL_FILTER_MODE_WHITELIST){
		//iptables -A urlblock -p tcp -m weburl ! --contains domain_name --domain_only -j DROP
		va_cmd(IPTABLES, 12, 1, FW_ADD, WEBURL_CHAIN, "-p", "tcp", "-m" , "weburl","!", "--contains", "allname", "--domain_only" ,"-j", FW_DROP);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 12, 1, FW_ADD, WEBURL_CHAIN, "-p", "tcp", "-m" , "weburl","!", "--contains", "allname", "--domain_only" ,"-j", FW_DROP);
#endif
	}
	else{
		//iptables -D urlblock -p tcp -m weburl ! --contains domain_name --domain_only -j DROP
		va_cmd(IPTABLES, 12, 1, FW_DEL, WEBURL_CHAIN, "-p", "tcp", "-m" , "weburl","!", "--contains", "allname", "--domain_only" ,"-j", FW_DROP);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 12, 1, FW_DEL, WEBURL_CHAIN, "-p", "tcp", "-m" , "weburl","!", "--contains", "allname", "--domain_only" ,"-j", FW_DROP);
#endif
	}

	return 0;
}

int rtk_ctcapd_dns_filter_set_mode(unsigned int dns_fitler_mode)
{
	unsigned int FilterMode = 0;

	FilterMode = dns_fitler_mode;
	//printf("%s:%d FilterMode:%d\n",__FUNCTION__,__LINE__,FilterMode);

	if(!mib_set(MIB_CTCAPD_DNS_FILTER_MODE, (void *)&FilterMode)){
		printf("%s:%d set dns filter table fail!!\n",__FUNCTION__,__LINE__);
		return -1;
	}
	printf("%s:%d mode:%d\n",__FUNCTION__,__LINE__,FilterMode);

	if(FilterMode){
		system("echo white > /proc/driver/realtek/rtk_dns_filter_list");
	}
	else{
		system("echo black > /proc/driver/realtek/rtk_dns_filter_list");
	}

	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);

	return 0;

}

int rtk_modify_filter_rules_mode_and_action(int filter_type, int filter_mode)
{
	int total = 0,i = 0;
	MIB_CE_CTCAPD_DNS_FILTER_T dns_filter_entry;
	MIB_CE_URL_FILTER_T url_filter_entry;
	unsigned int match_flag = 0;

	if(filter_type)
	{
		total = mib_chain_total(MIB_CTCAPD_DNS_FILTER_TBL);
		if(total == 0){
			//if no rule need modify total mode to blacklist
			if(filter_mode!=0){
				if(rtk_ctcapd_dns_filter_set_mode(0)){
					printf("%s %d set dns filter mode error!",__FUNCTION__,__LINE__);
					return -1;
				}
			}

			return 0;
		}

		for(i=0;i<total;i++)
		{
			if (!mib_chain_get(MIB_CTCAPD_DNS_FILTER_TBL, i, (void *)&dns_filter_entry)) {
				return -1;
			}

			//when setting whitelist rule should change total mode to whitelist
			if(dns_filter_entry.mode){
				match_flag = 1;

				//modify total whitelist action
				rtk_set_dns_filter_whitelist_action(dns_filter_entry.action);
				if(filter_mode!= dns_filter_entry.mode){
					if(rtk_ctcapd_dns_filter_set_mode(dns_filter_entry.mode)){
						printf("%s %d set dns filter mode error!",__FUNCTION__,__LINE__);
						return -1;
					}
				}
			}
		}

		//if no whitelist but total mode is whitelist ,should change mode
		if(match_flag==0 && filter_mode){
			if(rtk_ctcapd_dns_filter_set_mode(match_flag)){
				printf("%s %d set dns filter mode error!",__FUNCTION__,__LINE__);
				return -1;
			}
		}
	}
	else
	{
		total = mib_chain_total(MIB_URL_FILTER_TBL);
		if(total == 0){
			if(filter_mode == URL_FILTER_MODE_WHITELIST){
				if(rtk_ctcapd_url_filter_set_mode(URL_FILTER_MODE_BLACKLIST)){
					printf("%s %d set url filter mode error!",__FUNCTION__,__LINE__);
					return -1;
				}
			}
			return 0;
		}

		for(i=0;i<total;i++)
		{
			if (!mib_chain_get(MIB_URL_FILTER_TBL, i, (void *)&url_filter_entry)) {
				printf("%s %d mib get url filter tbl error!",__FUNCTION__,__LINE__);
				return -1;
			}

			if(url_filter_entry.mode){
				match_flag = 1;

				//modify total whitelist action
				if(filter_mode != URL_FILTER_MODE_WHITELIST){
					if(rtk_ctcapd_url_filter_set_mode(URL_FILTER_MODE_WHITELIST)){
						printf("%s %d set url filter mode error!",__FUNCTION__,__LINE__);
						return -1;
					}
				}
				break;
			}
		}

		if(match_flag==0 && filter_mode==URL_FILTER_MODE_WHITELIST){
			if(rtk_ctcapd_url_filter_set_mode(URL_FILTER_MODE_BLACKLIST)){
				printf("%s %d set url filter mode error!",__FUNCTION__,__LINE__);
				return -1;
			}
		}
	}

	return 0;
}

int rtk_set_dns_filter_whitelist_action(int action)
{
	char cmdBuffer[70];

	memset(cmdBuffer,0,sizeof(cmdBuffer));
	snprintf(cmdBuffer,sizeof(cmdBuffer),"echo totalwhitemode:%d > /proc/driver/realtek/rtk_dns_filter_list",action);
	system(cmdBuffer);

	return 0;

}

int rtk_set_dns_filter_rules_for_ubus(void)
{
	unsigned int dns_filter_enable = 0;
	unsigned int FilterMode = 0;
	char skbmark_buf[64]={0};

	//iptables -t mangle -F mark_for_dns_filter
	va_cmd(IPTABLES, 4, 1,(char *)ARG_T, "mangle","-F", (char *)MARK_FOR_DNS_FILTER);

	if(!mib_get(MIB_CTCAPD_DNS_FILTER_ENABLE, (void *)&dns_filter_enable)){
		printf("[%s:%d]dns filter enable get fail!!\n",__FUNCTION__, __LINE__);
		return -1;
	}

	if(!mib_get(MIB_CTCAPD_DNS_FILTER_MODE, (void *)&FilterMode)){
		printf("%s:%d get dns filter tables fail!!\n",__FUNCTION__,__LINE__);
		return -1;
	}

	if(dns_filter_enable){
		system("echo enable > /proc/driver/realtek/rtk_dns_filter_enable");
		system("echo flush > /proc/driver/realtek/rtk_dns_filter_list");

		//set mark for trap dns query response pkts to ps
#ifdef CONFIG_RTK_SKB_MARK2
		snprintf(skbmark_buf, sizeof(skbmark_buf), "0x%x", (1<<SOCK_MARK2_FORWARD_BY_PS_START));
		//iptables -t mangle -A mark_for_dns_filter -p udp --sport 53 -j MARK2 --or-mark 0x1
		va_cmd(IPTABLES, 12, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, (char *)MARK_FOR_DNS_FILTER,
			"-p", (char *)ARG_UDP, (char *)FW_SPORT, (char *)PORT_DNS, "-j", "MARK2","--or-mark",skbmark_buf);
#else
		snprintf(skbmark_buf, sizeof(skbmark_buf), "0x%x", (1<<SOCK_MARK_METER_INDEX_END));
		//iptables -t mangle -A mark_for_dns_filter -p udp --sport 53 -j MARK --or-mark 0x80000000
		va_cmd(IPTABLES, 12, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, (char *)MARK_FOR_DNS_FILTER,
			"-p", (char *)ARG_UDP, (char *)FW_SPORT, (char *)PORT_DNS, "-j", "MARK","--or-mark",skbmark_buf);
#endif
	}
	else{
		system("echo disable > /proc/driver/realtek/rtk_dns_filter_enable");
		system("echo flush > /proc/driver/realtek/rtk_dns_filter_list");
	}

	if(FilterMode){
		system("echo white > /proc/driver/realtek/rtk_dns_filter_list");
	}
	else{
		system("echo black > /proc/driver/realtek/rtk_dns_filter_list");
	}

	return 0;
}

int rtk_set_url_filter_iptables_rule_for_ubus(void)
{
	unsigned int url_filter_enable = 0;
	char skbmark_buf[64]={0};

	if(!mib_get(MIB_CTCAPD_URL_FILTER_ENABLE, (void *)&url_filter_enable)){
		printf("[%s:%d]url filter enable get fail!!\n",__FUNCTION__, __LINE__);
		return -1;
	}

	//iptables -t mangle -F mark_for_dns_filter
	va_cmd(IPTABLES, 4, 1,(char *)ARG_T, "mangle","-F", (char *)MARK_FOR_URL_FILTER);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1,(char *)ARG_T, "mangle","-F", (char *)MARK_FOR_URL_FILTER);
#endif

	if(url_filter_enable){
		//set mark for trap dns query response pkts to ps
#ifdef CONFIG_RTK_SKB_MARK2
		snprintf(skbmark_buf, sizeof(skbmark_buf), "0x%x", (1<<SOCK_MARK2_FORWARD_BY_PS_START));
		//iptables -t mangle -A mark_for_url_filter -i br0 -p tcp --dport 80 -j MARK2 --or-mark 0x1
		va_cmd(IPTABLES, 14, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, (char *)MARK_FOR_URL_FILTER,(char *)ARG_I,(char *)BRIF,
			"-p", (char *)ARG_TCP, (char *)FW_DPORT, "80", "-j", "MARK2","--or-mark",skbmark_buf);
		//iptables -t mangle -A mark_for_url_filter -i br0 -p tcp --dport 443 -j MARK2 --or-mark 0x1
		va_cmd(IPTABLES, 14, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, (char *)MARK_FOR_URL_FILTER,(char *)ARG_I,(char *)BRIF,
			"-p", (char *)ARG_TCP, (char *)FW_DPORT, "443", "-j", "MARK2","--or-mark",skbmark_buf);
#ifdef CONFIG_IPV6
		//ip6tables -t mangle -A mark_for_url_filter -i br0 -p tcp --dport 80 -j MARK2 --or-mark 0x1
		va_cmd(IP6TABLES, 14, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, (char *)MARK_FOR_URL_FILTER,(char *)ARG_I,(char *)BRIF,
			"-p", (char *)ARG_TCP, (char *)FW_DPORT, "80", "-j", "MARK2","--or-mark",skbmark_buf);
		//ip6tables -t mangle -A mark_for_url_filter -i br0 -p tcp --dport 443 -j MARK2 --or-mark 0x1
		va_cmd(IP6TABLES, 14, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, (char *)MARK_FOR_URL_FILTER,(char *)ARG_I,(char *)BRIF,
			"-p", (char *)ARG_TCP, (char *)FW_DPORT, "443", "-j", "MARK2","--or-mark",skbmark_buf);
#endif
#else
		snprintf(skbmark_buf, sizeof(skbmark_buf), "0x%x", (1<<SOCK_MARK_METER_INDEX_END));
		//iptables -t mangle -A mark_for_url_filter -i br0 -p tcp --dport 80 -j MARK --or-mark 0x80000000
		va_cmd(IPTABLES, 14, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, (char *)MARK_FOR_URL_FILTER,(char *)ARG_I,(char *)BRIF,
			"-p", (char *)ARG_TCP, (char *)FW_DPORT, "80", "-j", "MARK","--or-mark",skbmark_buf);
		//iptables -t mangle -A mark_for_url_filter -i br0 -p tcp --dport 443 -j MARK --or-mark 0x80000000
		va_cmd(IPTABLES, 14, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, (char *)MARK_FOR_URL_FILTER,(char *)ARG_I,(char *)BRIF,
			"-p", (char *)ARG_TCP, (char *)FW_DPORT, "443", "-j", "MARK","--or-mark",skbmark_buf);
#ifdef CONFIG_IPV6
		//ip6tables -t mangle -A mark_for_url_filter -i br0 -p tcp --dport 80 -j MARK --or-mark 0x80000000
		va_cmd(IP6TABLES, 14, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, (char *)MARK_FOR_URL_FILTER,(char *)ARG_I,(char *)BRIF,
			"-p", (char *)ARG_TCP, (char *)FW_DPORT, "80", "-j", "MARK","--or-mark",skbmark_buf);
		//ip6tables -t mangle -A mark_for_url_filter -i br0 -p tcp --dport 443 -j MARK --or-mark 0x80000000
		va_cmd(IP6TABLES, 14, 1, (char *)ARG_T, "mangle", (char *)FW_ADD, (char *)MARK_FOR_URL_FILTER,(char *)ARG_I,(char *)BRIF,
			"-p", (char *)ARG_TCP, (char *)FW_DPORT, "443", "-j", "MARK","--or-mark",skbmark_buf);
#endif
#endif
	}

	return 0;
}

int rtk_restore_ctcapd_filter_state(void)
{
	MIB_CE_URL_FILTER_T url_entry;
	MIB_CE_CTCAPD_DNS_FILTER_T dns_entry;
	int i, total;

	//iptables -t mangle -N mark_for_dns_filter
	va_cmd(IPTABLES, 4, 1,(char *)ARG_T, "mangle","-N", (char *)MARK_FOR_DNS_FILTER);
	//iptables -t mangle -I PREROUTING -j mark_for_dns_filter
	va_cmd(IPTABLES, 6, 1, (char *)ARG_T, "mangle",(char *)FW_INSERT, (char *)FW_PREROUTING, "-j", (char *)MARK_FOR_DNS_FILTER);
	//iptables -t mangle -N mark_for_dns_filter

	//restore dns filter state
	total = mib_chain_total(MIB_CTCAPD_DNS_FILTER_TBL);

	if(!total)
		return 0;

	for (i=0; i<total; i++)
	{
		if (!mib_chain_get(MIB_CTCAPD_DNS_FILTER_TBL, i, (void *)&dns_entry)){
			printf("%s:%d get dns filter fail!!\n",__FUNCTION__,__LINE__);
			return -1;
		}

		dns_entry.rule_add = 0;

		if(!mib_chain_update(MIB_CTCAPD_DNS_FILTER_TBL, (void*)&dns_entry, i))
		{
			printf("%s:%d update dns filter fail!!\n",__FUNCTION__,__LINE__);
			return -1;
		}
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	}

	return 0;
}

int rtk_dns_filter_check_time(MIB_CE_CTCAPD_DNS_FILTER_T filter_entry)
{
	time_t tm;
	struct tm tm_time;
	unsigned char tmp_buff[36];
	char *tokptr1=NULL, *strptr1 = NULL;
	unsigned int hour_start = 0, hour_end = 0;
	unsigned int min_start = 0, min_end = 0;
	unsigned int start_time = 0, end_time = 0;
	unsigned int weekday = 0;
	unsigned int match_flag = 0;//0:block 1:allow
	unsigned int current_time = 0;

	if(isTimeSynchronized() == 0)
		return 0;

	time(&tm);
	memcpy(&tm_time, localtime(&tm), sizeof(tm_time));
	current_time = getMinutes(tm_time.tm_hour, tm_time.tm_min);

	memset(tmp_buff, '\0', sizeof(tmp_buff));
	strncpy(tmp_buff, filter_entry.weekdays ,sizeof(tmp_buff));
	strptr1 = tmp_buff;

	for (tokptr1 = strsep(&strptr1,","); tokptr1 != NULL;
		  tokptr1 = strsep(&strptr1,","))
	{
		sscanf(tokptr1, "%d", &weekday);

		if(weekday == 8)
		{
			//all weekday forbid;
			match_flag = 0;
			break;
		}
		else{
			if(weekday == 7){
				//Sunday will set zero;
				weekday= 0;
			}

			if(weekday == tm_time.tm_wday)
			{
				match_flag = 1;
				break;
			}
			else{
				match_flag = 0;
			}
		}
	}

	//match weekday
	if(match_flag){
		memset(tmp_buff, '\0', sizeof(tmp_buff));
		strncpy(tmp_buff, filter_entry.timerange ,sizeof(tmp_buff)-1);
		strptr1 = tmp_buff;
		for (tokptr1 = strsep(&strptr1,","); tokptr1 != NULL;
			  tokptr1 = strsep(&strptr1,",")){

			sscanf(tokptr1, "%d:%d-%d:%d", &hour_start , &min_start , &hour_end , &min_end);

			if(hour_start == 0 && min_start==0 && hour_end==0 && min_end==0){
				//block all day
				match_flag = 0;
				break;
			}
			start_time = getMinutes(hour_start, min_start);
			end_time = getMinutes(hour_end, min_end);

			if(start_time<=current_time && end_time>=current_time){
				match_flag = 1;
				break;
			}
			else
				match_flag = 0;
		}
	}

	return match_flag;
}

void rtk_set_dns_filter_rule(MIB_CE_CTCAPD_DNS_FILTER_T filter_entry, unsigned int rule_add_flag)
{
	unsigned char cmdBuffer[50+CTCAPD_FILTER_DOMAIN_LENTH*CTCAPD_FILTER_DOMAIN_NUM+CTCAPD_FILTER_MAC_NUM*CTCAPD_FILTER_MAC_LENTH];

	if(filter_entry.rule_add != rule_add_flag){
		if(rule_add_flag ==0){
			memset(cmdBuffer,0,sizeof(cmdBuffer));
			snprintf(cmdBuffer,sizeof(cmdBuffer),"echo del:%s %s> /proc/driver/realtek/rtk_dns_filter_list",filter_entry.dnsAddr,filter_entry.macAddr);
			system(cmdBuffer);
		}
		else{
			memset(cmdBuffer,0,sizeof(cmdBuffer));
			snprintf(cmdBuffer,sizeof(cmdBuffer),"echo add:%d %d %s %s > /proc/driver/realtek/rtk_dns_filter_list",filter_entry.mode,filter_entry.action,filter_entry.dnsAddr,filter_entry.macAddr);
			system(cmdBuffer);
		}
	}
}

int rtk_update_dns_filter_state(MIB_CE_CTCAPD_DNS_FILTER_T filter_entry,unsigned int rule_add_flag,int dns_filter_index)
{
	rtk_set_dns_filter_rule(filter_entry, rule_add_flag);

	if(rule_add_flag!=filter_entry.rule_add){

		filter_entry.rule_add = rule_add_flag;
		if(!mib_chain_update(MIB_CTCAPD_DNS_FILTER_TBL, (void*)&filter_entry, (unsigned int)dns_filter_index))
		{
			printf("[%s:%d]update dns filter fail!!\n",__FUNCTION__, __LINE__);
			return -1;
		}

		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	}

	return 0;
}

int rtk_check_dns_filter_time_schedule(void)
{
	unsigned int total = 0, i = 0;
	MIB_CE_CTCAPD_DNS_FILTER_T ctcapd_dns_filter_entry = {0};
	unsigned int time_match = 0;
	unsigned int dns_filter_enable = 0,dns_filter_mode = 0;

	total = mib_chain_total(MIB_CTCAPD_DNS_FILTER_TBL);

	if(!mib_get(MIB_CTCAPD_DNS_FILTER_ENABLE, (void *)&dns_filter_enable)){
		printf("[%s:%d]dns filter enable get fail!!\n",__FUNCTION__, __LINE__);
		return -1;
	}

	if(dns_filter_enable == 0 || total == 0){
		//printf("[%s:%d]dns filter is disable:%d\n",__FUNCTION__, __LINE__, dns_filter_enable);
		return -1;
	}

	if(!mib_get(MIB_CTCAPD_DNS_FILTER_MODE, (void *)&dns_filter_mode)){
		printf("[%s:%d]dns filter mode get fail!!\n",__FUNCTION__, __LINE__);
		return -1;
	}

	rtk_modify_filter_rules_mode_and_action(DNSFILTERMODE, dns_filter_mode);

	for(i = 0;i < total;i++)
	{
		if (!mib_chain_get(MIB_CTCAPD_DNS_FILTER_TBL, i, (void *)&ctcapd_dns_filter_entry)) {
			printf("get dns filter entry by index fail!\n");
			return -1;
		}

		//only enable ctcapd filter could add to proc
		if(strncmp(ctcapd_dns_filter_entry.enable, "yes", 3))
		{
			continue;
		}

		time_match = rtk_dns_filter_check_time(ctcapd_dns_filter_entry);
		rtk_update_dns_filter_state(ctcapd_dns_filter_entry, time_match, i);
	}

	return 0;
}

void rtk_set_url_filter_rule(MIB_CE_URL_FILTER_T entry, int rule_add_flag)
{
	int port,j;
	char portStr[6];
	char key[256]={0};
	char *argv[64];
	int argidx=1;
	int tcp_port_idx=-1, url_idx=-1;
	URLFILTER_URLLIST_T *pURLFilterList = NULL, *pTmp=NULL;
	unsigned int change_flag=0;
	char tmp_buf1[512], tmpbuf2[18], *strptr1 = NULL, *tokptr1 = NULL;

	argv[argidx++] = (char *)FW_INSERT;
	argv[argidx++] = (char *)URLBLOCK_CHAIN;
	argv[argidx++] = "-p";
	argv[argidx++] = "tcp";
	tcp_port_idx = argidx;

	if(rule_add_flag != entry.rule_add)
	{
		if(rule_add_flag == 0)
			argv[1] = (char *)FW_DEL;

		parse_ur_filter(entry.url, key, &port);

		pURLFilterList = parse_urlfilter_urlist(entry.urllist);
		pTmp = pURLFilterList;

		memset(tmp_buf1, '\0', sizeof(tmp_buf1));
		//set url filter's mac=="all" when users set rules without mac by webUI
		if(strlen(entry.mac) == 0){
			snprintf(tmp_buf1, sizeof(tmp_buf1), "%s", "all");
			//printf("%s:%d mac is %s\n",__FUNCTION__,__LINE__,tmp_buf1);
		}
		else{
			strncpy(tmp_buf1, entry.mac ,sizeof(tmp_buf1)-1);
		}

		strptr1 = tmp_buf1;
		for (tokptr1 = strsep(&strptr1,","); tokptr1 != NULL;
		tokptr1 = strsep(&strptr1,","))
		{
			pURLFilterList = pTmp;
			memset(tmpbuf2, '\0', sizeof(tmpbuf2));
			snprintf(tmpbuf2, sizeof(tmpbuf2), "%s", tokptr1);
			argidx = tcp_port_idx;
			if(port != 0)
			{
				snprintf(portStr, 6, "%d", port);
				argv[argidx++] = "--dport";
				argv[argidx++] = portStr;
			}
			if(strncmp(tmpbuf2, "all", 3))
			{
				argv[argidx++] = "-m";
				argv[argidx++] = "mac";
				argv[argidx++] = "--mac-source";
				argv[argidx++] = tmpbuf2;
			}
			url_idx = argidx;
			if(strlen(key))
			{
#ifdef CONFIG_NETFILTER_XT_MATCH_WEBURL
				argv[argidx++] = "-m";
				argv[argidx++] = "weburl";
				argv[argidx++] = "--contains";
				argv[argidx++] = key;
				argv[argidx++] = "--domain_only";
				argv[argidx++] = "-j";
				if(entry.mode == 0)//black list
					argv[argidx++] = (char *)FW_DROP;
				else//white list
					argv[argidx++] = (char *)FW_ACCEPT;
#else
				argv[argidx++] = "-m";
				argv[argidx++] = "string";
				argv[argidx++] = "--url";
				argv[argidx++] = key;
				argv[argidx++] = "--algo";
				argv[argidx++] = "-bm";
				argv[argidx++] = "-j";
				if(entry.mode == 0)//black list
					argv[argidx++] = (char *)FW_DROP;
				else//white list
					argv[argidx++] = (char *)FW_ACCEPT;
#endif
				argv[argidx++] = NULL;

				do_cmd(IPTABLES, argv, 1);
#ifdef CONFIG_IPV6
				do_cmd(IP6TABLES, argv, 1);
#endif
			}

			while(pURLFilterList)
			{
				if(strlen(pURLFilterList->url)>0)
				{
					argidx = url_idx;
#ifdef CONFIG_NETFILTER_XT_MATCH_WEBURL
					argv[argidx++] = "-m";
					argv[argidx++] = "weburl";
					argv[argidx++] = "--contains";
					//argv[argidx++] = key;
					argv[argidx++] = pURLFilterList->url;
					argv[argidx++] = "--domain_only";
					argv[argidx++] = "-j";
					if(entry.mode == 0)//black list
						argv[argidx++] = (char *)FW_DROP;
					else//white list
						argv[argidx++] = (char *)FW_ACCEPT;
#else
					argv[argidx++] = "-m";
					argv[argidx++] = "string";
					argv[argidx++] = "--url";
					//argv[argidx++] = key;
					argv[argidx++] = pURLFilterList->url;
					argv[argidx++] = "--algo";
					argv[argidx++] = "-bm";
					argv[argidx++] = "-j";
					if(entry.mode == 0)//black list
						argv[argidx++] = (char *)FW_DROP;
					else//white list
						argv[argidx++] = (char *)FW_ACCEPT;
#endif
					argv[argidx++] = NULL;
					/*
					{
						int para_idx = 1;
						printf("iptables para:");
						while(argv[para_idx])
						{
							printf("%s ", argv[para_idx]);
							para_idx++;
						}
						printf("\n");
					}
					*/

					do_cmd(IPTABLES, argv, 1);
#ifdef CONFIG_IPV6
					do_cmd(IP6TABLES, argv, 1);
#endif
				}
				pURLFilterList = pURLFilterList->next;
			}
		}
		free_urlfilter_list(pTmp);
	}

}

int rtk_update_url_filter_state(MIB_CE_URL_FILTER_T entry, int rule_add_flag,int url_filter_index)
{
	rtk_set_url_filter_rule(entry,rule_add_flag);

	if(rule_add_flag != entry.rule_add)
	{
		entry.rule_add = rule_add_flag;
		if(!mib_chain_update(MIB_URL_FILTER_TBL, (void*)&entry, (unsigned int)url_filter_index))
		{
			printf("[%s:%d]update url filter state fail!!\n",__FUNCTION__, __LINE__);
			return -1;
		}
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	}

	return 0;
}

int rtk_ctcapd_set_filter_rule_by_ubus(MIB_CE_URL_FILTER_Tp url_entry,int action)
{
	int i = 0, flag = 0, total = 0, weekdays[7] = {0};
	char tmpbuf[32] = {0}, tmpbuf2[32]  ={0};
	RTK_CTCAPD_FILTER_T ctcapd_filter;
	MIB_CE_URL_FILTER_T url_filter_entry;
	char *tokptr1 = NULL, *strptr1 = NULL;
	char url_name[(CTCAPD_FILTER_DOMAIN_LENTH+2)*CTCAPD_FILTER_DOMAIN_NUM] = {0}, mac[(CTCAPD_FILTER_MAC_NUM+2)*CTCAPD_FILTER_MAC_LENTH]= {0};
	char cmdbuff[UBUS_CMD_LENTH] = {0};
	//char *argv[];
	int macflag = 0, add_flag = 0;

	if(url_entry == NULL)
		return RTK_FAILED;

	memset(&ctcapd_filter,0,sizeof(ctcapd_filter));

	if(url_entry->Enable == 1)
		snprintf(ctcapd_filter.enable,CTCAPD_FILTER_ENABLE_LENTH,"%s","yes");
	else
		snprintf(ctcapd_filter.enable,CTCAPD_FILTER_ENABLE_LENTH,"%s","no");

	if(strlen(url_entry->urllist))
		snprintf(ctcapd_filter.dnsaddr,CTCAPD_FILTER_DOMAIN_LENTH*CTCAPD_FILTER_DOMAIN_NUM,"%s;%s",url_entry->url,url_entry->urllist);
	else
		snprintf(ctcapd_filter.dnsaddr,CTCAPD_FILTER_DOMAIN_LENTH*CTCAPD_FILTER_DOMAIN_NUM,"%s",url_entry->url);

	strptr1 = ctcapd_filter.dnsaddr;
	for (tokptr1 = strsep(&strptr1,";"); tokptr1 != NULL;
		tokptr1 = strsep(&strptr1,";"))
	{
		memset(tmpbuf,0,sizeof(tmpbuf));
		if(flag == 0){
			snprintf(tmpbuf,CTCAPD_FILTER_DOMAIN_LENTH,"\"%s\"",tokptr1);
			flag =1;
		}
		else
		{
			snprintf(tmpbuf, CTCAPD_FILTER_DOMAIN_LENTH, ",\"%s\"", tokptr1);
		}

		strncat(url_name,tmpbuf,sizeof(tmpbuf));
	}

	snprintf(ctcapd_filter.name,CTCAPD_FILTER_NAME_LENTH,"%s",url_entry->name);

	snprintf(ctcapd_filter.macaddr,CTCAPD_FILTER_MAC_NUM*CTCAPD_FILTER_MAC_LENTH,"%s",url_entry->mac);

	if(strlen(ctcapd_filter.macaddr)==0){
		memset(mac,0,sizeof(mac));
	}
	else{
		strptr1 = ctcapd_filter.macaddr;
		flag = 0;
		for (tokptr1 = strsep(&strptr1,","); tokptr1 != NULL;
			tokptr1 = strsep(&strptr1,","))
		{
			i= 0;
			macflag = 0;
			add_flag = 0;
			memset(tmpbuf,0,sizeof(tmpbuf));
			memset(tmpbuf2,0,sizeof(tmpbuf2));

			if(flag == 0){
				snprintf(tmpbuf,sizeof(tmpbuf),"%s",tokptr1);
				while(tmpbuf[i])
				{
					if(macflag==2){
						add_flag++;
						macflag = 0;
					}
					tmpbuf2[i] = tmpbuf[i+add_flag];
					macflag++;
					i++;
				}
				snprintf(tmpbuf,sizeof(tmpbuf),"\"%s\"",tmpbuf2);
				flag =1;
			}
			else
			{
				snprintf(tmpbuf,sizeof(tmpbuf),"%s",tokptr1);
				while(tmpbuf[i])
				{
					if(macflag==2){
						add_flag++;
						macflag = 0;
					}
					tmpbuf2[i] = tmpbuf[i+add_flag];
					macflag++;
					i++;
				}
				printf("%s %d mac:%s\n",__FUNCTION__,__LINE__,tmpbuf2);
				snprintf(tmpbuf,sizeof(tmpbuf),",\"%s\"",tmpbuf2);
			}
			strncat(mac,tmpbuf,sizeof(tmpbuf));
		}
	}

	ctcapd_filter.mode = url_entry->mode;
	ctcapd_filter.action = 0;

	if(url_entry->WeekDays == 0){
		memset(ctcapd_filter.weekdays,0,sizeof(ctcapd_filter.weekdays));
	}
	else{
		flag = 0;
		memset(tmpbuf,0,sizeof(tmpbuf));
		for(i=0;i<7;i++)
		{
			if(url_entry->WeekDays & (1<<i)){
				if(flag==0){
					snprintf(tmpbuf2, sizeof(tmpbuf2), "%d", i+1);
					flag =1;
				}
				else{
					snprintf(tmpbuf2, sizeof(tmpbuf2), ",%d", i+1);
				}

				strncat(tmpbuf,tmpbuf2,sizeof(tmpbuf)-strlen(tmpbuf)-1);
			}
		}
		printf("%s %d week:%s\n",__FUNCTION__,__LINE__,tmpbuf);
		snprintf(ctcapd_filter.weekdays, CTCAPD_FILTER_WEEKDAY_LENTH, "%s", tmpbuf);
	}


	memset(tmpbuf2,0,sizeof(tmpbuf2));
	memset(tmpbuf,0,sizeof(tmpbuf));
	snprintf(tmpbuf2, sizeof(tmpbuf2), "%02d:%02d-%02d:%02d", url_entry->start_hr1,url_entry->start_min1,url_entry->end_hr1,url_entry->end_min1);
	if(!strncmp(tmpbuf2,"00:00-00:00",sizeof(tmpbuf2)))
	{
		memset(ctcapd_filter.timerange,0,sizeof(ctcapd_filter.timerange));
	}
	else if(!strncmp(tmpbuf2,"00:00-23:59",sizeof(tmpbuf2)))
	{
		snprintf(ctcapd_filter.timerange, sizeof(ctcapd_filter.timerange), "00:00-00:00");
	}
	else
	{
		strncat(tmpbuf,tmpbuf2,sizeof(tmpbuf)-strlen(tmpbuf)-1);
		if(url_entry->start_hr2!=0 || url_entry->start_min2!=0 || url_entry->end_hr2!=0 || url_entry->end_min2!=0){
			memset(tmpbuf2,0,sizeof(tmpbuf2));
			snprintf(tmpbuf2, sizeof(tmpbuf2), ",%02d:%02d-%02d:%02d", url_entry->start_hr2,url_entry->start_min2,url_entry->end_hr2,url_entry->end_min2);
			strncat(tmpbuf,tmpbuf2,sizeof(tmpbuf)-strlen(tmpbuf)-1);
		}
		if(url_entry->start_hr3!=0 || url_entry->start_min3!=0 || url_entry->end_hr3!=0 || url_entry->end_min3!=0){
			memset(tmpbuf2,0,sizeof(tmpbuf2));
			snprintf(tmpbuf2, sizeof(tmpbuf2), ",%02d:%02d-%02d:%02d", url_entry->start_hr3,url_entry->start_min3,url_entry->end_hr3,url_entry->end_min3);
			strncat(tmpbuf,tmpbuf2,sizeof(tmpbuf)-strlen(tmpbuf)-1);
		}
		snprintf(ctcapd_filter.timerange,CTCAPD_FILTER_TIMERANGE_LENTH,"%s",tmpbuf);
	}

	if(action == ADD_UBUS_FILTER){
		snprintf(cmdbuff,sizeof(cmdbuff),"/bin/ubus call ctcapd.filter.url add '{\"enable\":\"%s\",\"urls\":[%s],\"name\":\"%s\",\"macs\":[%s],\"mode\":%d,\"weekdays\":[%s],\"timerange\":\"%s\"}'",
			ctcapd_filter.enable, url_name, ctcapd_filter.name, mac, ctcapd_filter.mode, ctcapd_filter.weekdays, ctcapd_filter.timerange);
	}
	else{
		if (!mib_chain_get(MIB_URL_FILTER_TBL, url_entry->index, (void *)&url_filter_entry)) {
			return RTK_FAILED;
		}

		snprintf(cmdbuff,sizeof(cmdbuff),"/bin/ubus call ctcapd.filter.url.%d set '{\"enable\":\"%s\",\"urls\":[%s],\"name\":\"%s\",\"macs\":[%s],\"mode\":%d,\"weekdays\":[%s],\"timerange\":\"%s\"}'",
			url_filter_entry.index, ctcapd_filter.enable, url_name, ctcapd_filter.name, mac, ctcapd_filter.mode, ctcapd_filter.weekdays, ctcapd_filter.timerange);
	}
	printf("%s %d cmdbuff:%s\n", __FUNCTION__,__LINE__,cmdbuff);

	va_cmd("/bin/sh", 2, 1, "-c", cmdbuff);

	return RTK_SUCCESS;
}

int rtk_ctcapd_del_filter_rule_by_ubus(int index)
{
	MIB_CE_URL_FILTER_T url_entry;
	char cmdbuff[64];

	if (!mib_chain_get(MIB_URL_FILTER_TBL, index, (void *)&url_entry)) {
		return RTK_FAILED;
	}

	snprintf(cmdbuff,sizeof(cmdbuff),"/bin/ubus call ctcapd.filter.url delete '{\"idx\":%d}'",url_entry.index);
	printf("%s %d cmdbuff:%s\n", __FUNCTION__,__LINE__,cmdbuff);

	va_cmd("/bin/sh", 2, 1,"-c", cmdbuff);

	return RTK_SUCCESS;
}
#endif

#if defined(CONFIG_USER_CTCAPD)&&defined(WLAN_SUPPORT)
extern int set_wlan_idx_by_wlanIf(unsigned char * ifname,int *vwlan_idx);
int rtk_set_wifi_permit_rule(void)
{
	int i=0,num=0,j=0,vindex=0;
	MIB_CE_WIFI_PERMIT_T rule_entry;
	MIB_CE_MBSSIB_T Entry;
	char tmp_ip[32]={0};
	#define MAX_WLAN_IF NUM_WLAN_INTERFACE*(MAX_WLAN_VAP+1)
	char wlan_name[MAX_WLAN_IF][IFNAMESIZE]={0};
	DOCMDINIT

	//delete old setting
	va_cmd(EBTABLES, 2, 1, "-F",WIFI_PERMIT_IN_TBL);
	va_cmd(EBTABLES, 4, 1, "-D", "INPUT", "-j", WIFI_PERMIT_IN_TBL);
	va_cmd(EBTABLES, 2, 1, "-X",WIFI_PERMIT_IN_TBL);
	va_cmd(EBTABLES, 2, 1, "-F",WIFI_PERMIT_FWD_TBL);
	va_cmd(EBTABLES, 4, 1, "-D", "FORWARD", "-j", WIFI_PERMIT_FWD_TBL);
	va_cmd(EBTABLES, 2, 1, "-X",WIFI_PERMIT_FWD_TBL);

	//add new chain
	va_cmd(EBTABLES, 2, 1, "-N", WIFI_PERMIT_IN_TBL);
	va_cmd(EBTABLES, 4, 1, "-I", "INPUT", "-j", WIFI_PERMIT_IN_TBL);
	va_cmd(EBTABLES, 2, 1, "-N", WIFI_PERMIT_FWD_TBL);
	va_cmd(EBTABLES, 4, 1, "-I", "FORWARD", "-j", WIFI_PERMIT_FWD_TBL);

	//set default action : return, make other packets can match next rule
	va_cmd(EBTABLES, 3, 1, "-P", WIFI_PERMIT_IN_TBL, "RETURN");
	va_cmd(EBTABLES, 3, 1, "-P", WIFI_PERMIT_FWD_TBL, "RETURN");

	num=mib_chain_total(MIB_WIFI_PERMIT_TBL);

	for(i=0;i<num;i++){
		memset(&rule_entry,0,sizeof(rule_entry));
		if(!mib_chain_get(MIB_WIFI_PERMIT_TBL, i, (void*)&rule_entry))
			continue;

		//check if wlan's userisolation==2, only add rule when it is 2
		mib_save_wlanIdx();
		set_wlan_idx_by_wlanIf(rule_entry.name,&vindex);
		memset(&Entry,0,sizeof(Entry));
		if (!mib_chain_get(MIB_MBSSIB_TBL, vindex, (void *)&Entry)){
			mib_recov_wlanIdx();
  			continue;
		}
		mib_recov_wlanIdx();
		if(Entry.userisolation!=2)
			continue;

		snprintf(tmp_ip, sizeof(tmp_ip), "%s", inet_ntoa(*(struct in_addr *)(rule_entry.dip)));

		//add permit rule for specified dip+dport
		DOCMDARGVS(EBTABLES, DOWAIT, "-A %s -i %s -p ipv4 --ip-proto 6 --ip-dst %s --ip-dport %d -j RETURN",
				WIFI_PERMIT_IN_TBL, rule_entry.name, tmp_ip, rule_entry.dport);
		DOCMDARGVS(EBTABLES, DOWAIT, "-A %s -i %s -p ipv4 --ip-proto 17 --ip-dst %s --ip-dport %d -j RETURN",
				WIFI_PERMIT_IN_TBL, rule_entry.name, tmp_ip, rule_entry.dport);
		DOCMDARGVS(EBTABLES, DOWAIT, "-A %s -i %s -p ipv4 --ip-proto 6 --ip-dst %s --ip-dport %d -j RETURN",
				WIFI_PERMIT_FWD_TBL, rule_entry.name, tmp_ip, rule_entry.dport);
		DOCMDARGVS(EBTABLES, DOWAIT, "-A %s -i %s -p ipv4 --ip-proto 17 --ip-dst %s --ip-dport %d -j RETURN",
				WIFI_PERMIT_FWD_TBL, rule_entry.name, tmp_ip, rule_entry.dport);

		//record wlan-device name,only once
		for(j=0;j<MAX_WLAN_IF;j++){
			if(strlen(wlan_name[j])){
				if(strncmp(wlan_name[j],rule_entry.name,IFNAMESIZE)==0)
					break;
				else
					continue;
			}else{
				strncpy(wlan_name[j],rule_entry.name,IFNAMESIZE);
				break;
			}
		}
	}

	for(j=0;j<MAX_WLAN_IF;j++){
		if(strlen(wlan_name[j])){
			//add permit rule for dhcp
			DOCMDARGVS(EBTABLES, DOWAIT, "-I %s -p ipv4 -i %s --ip-proto 17 --ip-sport 67 -j RETURN",
				WIFI_PERMIT_IN_TBL,wlan_name[j]);
			DOCMDARGVS(EBTABLES, DOWAIT, "-I %s -p ipv4 -i %s --ip-proto 17 --ip-sport 68 -j RETURN",
				WIFI_PERMIT_IN_TBL,wlan_name[j]);
			DOCMDARGVS(EBTABLES, DOWAIT, "-I %s -p ipv4 -i %s --ip-proto 17 --ip-sport 67 -j RETURN",
				WIFI_PERMIT_FWD_TBL,wlan_name[j]);
			DOCMDARGVS(EBTABLES, DOWAIT, "-I %s -p ipv4 -i %s --ip-proto 17 --ip-sport 68 -j RETURN",
				WIFI_PERMIT_FWD_TBL,wlan_name[j]);

			//add default drop rule
			DOCMDARGVS(EBTABLES, DOWAIT, "-A %s -p ipv4 -i %s -j DROP",WIFI_PERMIT_IN_TBL, wlan_name[j]);
			DOCMDARGVS(EBTABLES, DOWAIT, "-A %s -p ipv4 -i %s -j DROP",WIFI_PERMIT_FWD_TBL, wlan_name[j]);
		}
	}

	return 0;
}
#endif

#ifdef CONFIG_USER_AVALANCH_DETECT
void init_avalanch_detect_fw(void)
{
	char tmp_cmd[1024] = {0};
	char avalanch_detect_enable = 0;
	char avalanch_detect_string[256];
	char mstring[128];
	char *p,*q;

	mib_get_s(PROVINCE_AVALANCH_DETECT_ENABLE, (void *)&avalanch_detect_enable, sizeof(avalanch_detect_enable));
	if (!avalanch_detect_enable)
		return;
	mib_get_s(PROVINCE_AVALANCH_DETECT_STRING, (void *)&avalanch_detect_string, sizeof(avalanch_detect_string));
	if (!avalanch_detect_string)
		return;


	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)IPTABLE_AVALANCH_DETECT);
	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-X", (char *)IPTABLE_AVALANCH_DETECT);
	// iptables -N AVALANCH_DETECT
	va_cmd_no_error(IPTABLES, 4, 1, "-t", "mangle", "-N", (char *)IPTABLE_AVALANCH_DETECT);

	//iptables -t mangle -A AVALANCH_DETECT -p tcp ! --tcp-flags PSH PSH -j MARK2 --set-mark2 0x1/0x1
	//snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -I %s -p tcp ! --tcp-flags PSH PSH -j MARK2 --set-mark2 0x1/0x1", IPTABLE_AVALANCH_DETECT);
	//va_cmd("/bin/sh", 2, 1, "-c", tmp_cmd);
#if 0
	//iptables -t mangle -A AVALANCH_DETECT -p tcp --tcp-flags ACK ACK -m length ! --length 0:100 -m string --algo bm --from 40 --to 300 --string 'Server: Microsoft-IIS/6.0' -m string --algo bm --from 40 --to 300 --string 'Last-Modified: Mon Jan  1 00:00:17 2001' -m string --algo bm --from 40 --to 300 --string 'Content-Length: 56000'  -j NFLOG --nflog-group 6 --nflog-prefix AVALANCH_DETECT
	snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -A %s -p tcp --tcp-flags ACK ACK -m length ! --length 0:100 -m string --algo bm --from 40 --to 300 --string 'Server: Microsoft-IIS/6.0' -m string --algo bm --from 40 --to 300 --string 'Last-Modified: Thu Jan  1 00:00:17 1970' -m string --algo bm --from 40 --to 300 --string 'Content-Length: 64'  -j NFLOG --nflog-group 6 --nflog-prefix AVALANCH_DETECT", IPTABLE_AVALANCH_DETECT);
#else
	snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -I %s -p tcp --tcp-flags ACK ACK -m length ! --length 0:100 -m recent  --set --name avapool ", IPTABLE_AVALANCH_DETECT);
	va_cmd("/bin/sh", 2, 1, "-c", tmp_cmd);
#endif
	snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -A %s -p tcp --tcp-flags ACK ACK -m recent --name avapool --rcheck --seconds 60 --hitcount 3 -j RETURN", IPTABLE_AVALANCH_DETECT);
	va_cmd("/bin/sh", 2, 1, "-c", tmp_cmd);

#if 0
		//iptables -t mangle -A AVALANCH_DETECT -p tcp --tcp-flags ACK ACK -m length ! --length 0:100 -m string --algo bm --from 40 --to 300 --string 'Server: Microsoft-IIS/6.0' -m string --algo bm --from 40 --to 300 --string 'Last-Modified: Mon Jan  1 00:00:17 2001' -m string --algo bm --from 40 --to 300 --string 'Content-Length: 56000'  -j NFLOG --nflog-group 6 --nflog-prefix AVALANCH_DETECT
		snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -A %s -p tcp --tcp-flags ACK ACK -m length ! --length 0:100 -m string --algo bm --from 40 --to 300 --string 'Server: Microsoft-IIS/6.0' -m string --algo bm --from 40 --to 300 --string 'Last-Modified: Thu Jan	1 00:00:17 1970' -m string --algo bm --from 40 --to 300 --string 'Content-Length: 64'  -j NFLOG --nflog-group 6 --nflog-prefix AVALANCH_DETECT", IPTABLE_AVALANCH_DETECT);
#else
	snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -A %s -p tcp --tcp-flags ACK ACK -m length ! --length 0:100", IPTABLE_AVALANCH_DETECT);
	p = avalanch_detect_string;
	q= NULL;

	while(p)
	{
		q = strstr(p,"+");
		if(q != NULL){
			*q = 0;
			memset(mstring, 0, sizeof(mstring));
			snprintf(mstring, sizeof(mstring), " -m string --algo bm --from 40 --to 300 --string '%s'", p);
			strcat(tmp_cmd,mstring);
			p = q+1;
		}else{
			memset(mstring, 0, sizeof(mstring));
			snprintf(mstring, sizeof(mstring), " -m string --algo bm --from 40 --to 300 --string '%s'", p);
			strcat(tmp_cmd,mstring);
			break;
		}
	}
	strcat(tmp_cmd," -j NFLOG --nflog-group 6 --nflog-prefix AVALANCH_DETECT");
#endif
	va_cmd("/bin/sh", 2, 1, "-c", tmp_cmd);

}

void setup_avalanch_detect_fw(void)
{
	char tmp_cmd[256] = {0};

	snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -D PREROUTING ! -i br+ -p tcp --sport 80 -j AVALANCH_DETECT");
	va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);
	//iptables -t mangle -I PREROUTING ! -i br+ -p tcp --sport 80 -j AVALANCH_DETECT
	snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -I PREROUTING ! -i br+ -p tcp --sport 80 -j AVALANCH_DETECT");
	va_cmd("/bin/sh", 2, 1, "-c", tmp_cmd);
}

void clear_avalanch_detect_fw(void)
{
	//char tmp_cmd[256] = {0};

	//iptables -t mangle -D PREROUTING ! -i br+ -p tcp --sport 80 -j AVALANCH_DETECT
	//snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -D PREROUTING ! -i br+ -p tcp --sport 80 -j AVALANCH_DETECT");
	//va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);

}

void rtk_avalanche_iptables_accept_add(void)
{
	char tmp_cmd[256] = {0};

	//iptables -t mangle -I POSTROUTING -j ACCEPT
	snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -I POSTROUTING -j ACCEPT");
	va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);

	//iptables -t mangle -I FORWARD -j ACCEPT
	snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -I FORWARD -j ACCEPT");
	va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);
	setup_avalanch_detect_fw();
	//iptables -t mangle -I PREROUTING -j ACCEPT
	snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -I PREROUTING 2 -j ACCEPT");
	va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);

	//iptables -t nat -I PREROUTING -j ACCEPT
	snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t nat -I PREROUTING -j ACCEPT");
	va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);
}

void rtk_avalanche_iptables_accept_delete(void)
{
	char tmp_cmd[256] = {0};

	//iptables -t mangle -D POSTROUTING -j ACCEPT
	snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -D POSTROUTING -j ACCEPT");
	va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);

	//iptables -t mangle -D FORWARD -j ACCEPT
	snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -D FORWARD -j ACCEPT");
	va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);

	//iptables -t mangle -D PREROUTING -j ACCEPT
	snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t mangle -D PREROUTING -j ACCEPT");
	va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);

	//iptables -t nat -D PREROUTING -j ACCEPT
	snprintf(tmp_cmd, sizeof(tmp_cmd), "iptables -t nat -D PREROUTING -j ACCEPT");
	va_cmd_no_error("/bin/sh", 2, 1, "-c", tmp_cmd);
}


#endif

#ifdef CONFIG_USER_RT_ACL_SYN_FLOOD_PROOF_SUPPORT
int rtk_lan_syn_flood_proof_fw_rules_add(unsigned long lan_ip, unsigned long lan_mask)
{
	unsigned long mbit, subnet;
	unsigned char ipaddrStr[20],subnetStr[20];
	char lan_ipaddr[32];
	DOCMDINIT

	//iptables -F LAN_SYN_FLOOD_PROOF
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_LAN_SYN_FLOOD_PROOF);
	//iptables -D INPUT -j LAN_SYN_FLOOD_PROOF
	va_cmd(IPTABLES, 4, 1, "-D", (char *)FW_INPUT, "-j" ,(char *)FW_LAN_SYN_FLOOD_PROOF);
	//iptables -X LAN_SYN_FLOOD_PROOF
	va_cmd(IPTABLES, 2, 1, "-X",(char *)FW_LAN_SYN_FLOOD_PROOF);

	//iptables -N LAN_SYN_FLOOD_PROOF
	va_cmd(IPTABLES, 2, 1, "-N",(char *)FW_LAN_SYN_FLOOD_PROOF);
	//iptables -I INPUT -j LAN_SYN_FLOOD_PROOF
	va_cmd(IPTABLES, 4, 1, "-I",(char *)FW_INPUT, "-j", (char *)FW_LAN_SYN_FLOOD_PROOF);

	strncpy(ipaddrStr, inet_ntoa(*((struct in_addr *)&lan_ip)), 16);
	ipaddrStr[15] = '\0';

	subnet = (lan_ip&lan_mask);
	strncpy(subnetStr, inet_ntoa(*((struct in_addr *)&subnet)), 16);
	subnetStr[15] = '\0';
	mbit=0;
	lan_mask = htonl(lan_mask);
	while (1) {
		if (lan_mask&0x80000000) {
			mbit++;
			lan_mask <<= 1;
		}
		else
			break;
	}
	snprintf(lan_ipaddr, sizeof(lan_ipaddr), "%s/%lu", subnetStr, mbit);

	//iptables -A LAN_SYN_FLOOD_PROOF -i br0 -p TCP --tcp-flags SYN SYN -s 192.168.1.0/24 -d 192.168.1.1 --dport 80 -j ACCEPT
	DOCMDARGVS(IPTABLES,DOWAIT,"-A %s -i %s -p %s --tcp-flags SYN SYN -s %s -d %s %s %d -j %s",
		(char *)FW_LAN_SYN_FLOOD_PROOF, (char *)LANIF, (char *)ARG_TCP, lan_ipaddr, ipaddrStr, (char *)FW_DPORT, 80, (char *)FW_ACCEPT);

	//iptables -A LAN_SYN_FLOOD_PROOF -i br0 -p TCP --tcp-flags SYN SYN -d 192.168.1.1 --dport 80 -j DROP
	DOCMDARGVS(IPTABLES,DOWAIT,"-A %s -i %s -p %s --tcp-flags SYN SYN -d %s %s %d -j %s",
		(char *)FW_LAN_SYN_FLOOD_PROOF, (char *)LANIF, (char *)ARG_TCP, ipaddrStr, (char *)FW_DPORT, 80, (char *)FW_DROP);

	return 0;
}

int rtk_lan_syn_flood_proof_fw_rules_delete( void )
{
	//iptables -F LAN_SYN_FLOOD_PROOF
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_LAN_SYN_FLOOD_PROOF);
	//iptables -D INPUT -j LAN_SYN_FLOOD_PROOF
	va_cmd(IPTABLES, 4, 1, "-D", (char *)FW_INPUT, "-j" ,(char *)FW_LAN_SYN_FLOOD_PROOF);
	//iptables -X LAN_SYN_FLOOD_PROOF
	va_cmd(IPTABLES, 2, 1, "-X",(char *)FW_LAN_SYN_FLOOD_PROOF);

	return 0;
}
#endif

#ifdef CONFIG_USER_LOCK_NET_FEATURE_SUPPORT
int rtk_set_net_locked_rules(unsigned char enabled)
{
	int ret = 0;
	unsigned char op_mode=0;

	#if 0
	if (mib_get_s(MIB_RTK_NET_MANAGE_SERVER, (void *)servername, sizeof(servername)) == 0){
		return -1;
	}
	#endif

#ifdef CONFIG_USER_RTK_BRIDGE_MODE
	op_mode = getOpMode();
#endif
	if(op_mode == GATEWAY_MODE){
		/*Should delete useless ebtables rules when change bridge opmode to gw opmode by these ebtable rules*/
		va_cmd(EBTABLES, 2, 1, "-F", (char *)BR_NETLOCK_INPUT);
		va_cmd(EBTABLES, 2, 1, "-F", (char *)BR_NETLOCK_FWD);
		va_cmd(EBTABLES, 2, 1, "-F", (char *)BR_NETLOCK_OUTPUT);

		va_cmd(IPTABLES, 2, 1, "-F", (char *)NETLOCK_INPUT);
		va_cmd(IPTABLES, 2, 1, "-F", (char *)NETLOCK_FWD);
		va_cmd(IPTABLES, 2, 1, "-F", (char *)NETLOCK_OUTPUT);
		va_cmd(IP6TABLES, 2, 1, "-F", (char *)NETLOCK_INPUT);
		va_cmd(IP6TABLES, 2, 1, "-F", (char *)NETLOCK_FWD);
		va_cmd(IP6TABLES, 2, 1, "-F", (char *)NETLOCK_OUTPUT);
	}
	else if(op_mode == BRIDGE_MODE){
		va_cmd(EBTABLES, 2, 1, "-F", (char *)BR_NETLOCK_INPUT);
		va_cmd(EBTABLES, 2, 1, "-F", (char *)BR_NETLOCK_FWD);
		va_cmd(EBTABLES, 2, 1, "-F", (char *)BR_NETLOCK_OUTPUT);
	}
	else{
		//do nothing
		return 0;
	}

	//iptables -t nat -D PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 80
	va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-p", "tcp","--dport","80", "-j", "REDIRECT", "--to-ports", "80");
	//ip6tables -t nat -D PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 80
	va_cmd(IP6TABLES, 12, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-p", "tcp","--dport","80", "-j", "REDIRECT", "--to-ports", "80");
	//ebtables -t nat -D PREROUTING -p IPv4 --ip-proto tcp --ip-dport 80 -j redirect --redirect-target ACCEPT
	va_cmd(EBTABLES, 14, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-p", "IPv4","--ip-proto","tcp", "--ip-dport", "80", "-j", "redirect", "--redirect-target", "ACCEPT");
	//ebtables -t nat -D PREROUTING -p IPv4 --ip-proto tcp --ip-dport 80 -j redirect --redirect-target ACCEPT
	va_cmd(EBTABLES, 14, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-p", "IPv6","--ip6-proto","tcp", "--ip6-dport", "80", "-j", "redirect", "--redirect-target", "ACCEPT");

#ifdef CONFIG_USER_BOA_WITH_SSL
	//iptables -t nat -D PREROUTING -p tcp --dport 443 -j REDIRECT --to-ports 443
	va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-p", "tcp","--dport","443", "-j", "REDIRECT", "--to-ports", "443");
	//ip6tables -t nat -D PREROUTING -p tcp --dport 443 -j REDIRECT --to-ports 443
	va_cmd(IP6TABLES, 12, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-p", "tcp","--dport","443", "-j", "REDIRECT", "--to-ports", "443");
	//ebtables -t nat -D PREROUTING -p IPv4 --ip-proto tcp --ip-dport 443 -j redirect --redirect-target ACCEPT
	va_cmd(EBTABLES, 14, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-p", "IPv4","--ip-proto","tcp", "--ip-dport", "443", "-j", "redirect", "--redirect-target", "ACCEPT");
	//ebtables -t nat -D PREROUTING -p IPv6 --ip6-proto tcp --ip6-dport 443 -j redirect --redirect-target ACCEPT
	va_cmd(EBTABLES, 14, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-p", "IPv6","--ip6-proto","tcp", "--ip6-dport", "443", "-j", "redirect", "--redirect-target", "ACCEPT");
#endif

	if(enabled){
		#if 0
		ret = inet_aton (servername, &server_addr);
		if(ret==0){
			//servername maybe url address
			server_hostent = gethostbyname (servername);
			if (server_hostent == NULL)
			{
				printf ("%s:%d gethostname %s error.\n", __FUNCTION__, __LINE__, servername);
				return -1;
			}

			if(!inet_ntop(server_hostent->h_addrtype, server_hostent->h_addr, str, sizeof(str))){
				printf ("%s:%d get server ip fail\n", __FUNCTION__, __LINE__);
				return -1;
			}
		}
		else
		{
			//servername maybe ip address
			snprintf(str,sizeof(str),"%s",servername);
		}
		#endif

		if(rtk_set_net_locked_all_whitelist())
		{
			printf("%s:%d set whitelist fail!\n", __FUNCTION__, __LINE__);
			return -1;
		}

		if(op_mode == GATEWAY_MODE){
			ret = rtk_set_net_locked_in_gw();
		}
		else if(op_mode == BRIDGE_MODE){
			ret = rtk_set_net_locked_in_bridge();
		}
		else{
			//do nothing
		}

		if(ret){
			printf ("%s:%d server ip is NULL\n", __FUNCTION__, __LINE__);
			return -1;
		}
	}
	else{
		//delete tmp file
	}

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	rtk_fc_flow_flush();
#endif

	return 0;

}

int rtk_set_net_locked_whitelist_rule_by_server_name(char *servername)
{
	int ret = 0;
	struct hostent *server_hostent;
	struct in_addr server_addr;
	char str[32] = {0};
	unsigned char op_mode=0;

	if(servername==NULL)
	{
		printf("%s %d *servername is NULL\n",__FUNCTION__,__LINE__);
		return -1;
	}

	ret = inet_aton(servername, &server_addr);
	if(ret==0){
		//servername maybe url address
		server_hostent = gethostbyname(servername);
		if (server_hostent == NULL)
		{
			printf ("%s:%d gethostname %s error.\n", __FUNCTION__, __LINE__, servername);
			return -1;
		}

		if(!inet_ntop(server_hostent->h_addrtype, server_hostent->h_addr, str, sizeof(str))){
			printf ("%s:%d get server ip fail\n", __FUNCTION__, __LINE__);
			return -1;
		}
	}
	else
	{
		//servername maybe ip address
		snprintf(str,sizeof(str),"%s",servername);
	}
	//printf("%s %d servername:%s str:%s\n",__FUNCTION__,__LINE__, servername, str);

#ifdef CONFIG_USER_RTK_BRIDGE_MODE
	op_mode = getOpMode();
#endif

	if(op_mode == GATEWAY_MODE){
		//iptables -I NetLock_Input -s 58.218.215.130 -j ACCEPT
		va_cmd(IPTABLES, 6, 1, (char *)FW_INSERT, (char *)NETLOCK_INPUT, "-s", str, "-j", (char *)FW_ACCEPT);
		//iptables -I NetLock_Output -s 58.218.215.130 -j ACCEPT
		va_cmd(IPTABLES, 6, 1, (char *)FW_INSERT, (char *)NETLOCK_OUTPUT, "-d", str, "-j", (char *)FW_ACCEPT);
	}
	else if(op_mode == BRIDGE_MODE){
		//ebtables -I Br_NetLock_Input -p IPv4 --ip-src 192.168.2.200 -j ACCEPT
		va_cmd(EBTABLES, 8, 1, (char *)FW_INSERT, (char *)BR_NETLOCK_INPUT, "-p", "IPv4","--ip-src", str, "-j", (char *)FW_ACCEPT);
		//ebtables -I Br_NetLock_Output -p IPV4 --ip-dst 192.168.2.200 -j ACCEPT
		va_cmd(EBTABLES, 8, 1, (char *)FW_INSERT, (char *)BR_NETLOCK_OUTPUT, "-p", "IPv4","--ip-dst", str, "-j", (char *)FW_ACCEPT);
	}
	else{
		//do nothing
	}

	return 0;
}

int rtk_set_net_locked_all_whitelist(void)
{
	FILE *fp;
	int i = 0;
	char line_buffer[128]={0};
	char tmp_server_name[128] = {0};
	char *static_server_name[STATIC_SERVER_NUM] = {"open.home.komect.com","cgw.komect.com","iotsec.komect.com",
		"ifa.ihnm.chinamobile.com","fota.home.komect.com"};

	if((fp= fopen("/etc/hynetwork.conf", "r"))==NULL)
		return -1;

	while(fgets(line_buffer, sizeof(line_buffer), fp))
	{
		line_buffer[sizeof(line_buffer)-1]='\0';

		memset(tmp_server_name,0,sizeof(tmp_server_name));

		if(strstr(line_buffer,"URL") && !strstr(line_buffer,"IFA"))
		{
			sscanf(line_buffer,"%*[^=]=%[^:]",tmp_server_name);

			if(rtk_set_net_locked_whitelist_rule_by_server_name(tmp_server_name))
			{
				printf("%s %d set whitelist fail!\n",__FUNCTION__,__LINE__);
			}
		}
	}
	fclose(fp);

	for(i = 0;i<STATIC_SERVER_NUM;i++)
	{
		if(rtk_set_net_locked_whitelist_rule_by_server_name(static_server_name[i]))
		{
			printf("%s %d set whitelist fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
	}

	return 0;
}


int rtk_set_net_locked_in_gw(void)
{
	#if 0
	if(server_ip == NULL)
		return -1;
	#endif

	//iptables -A NetLock_Fwd -i br+ -j DROP
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)NETLOCK_FWD, "-i","br+","-j", (char *)FW_DROP);

	//iptables -A NetLock_Input -p udp --sport 53 -j ACCEPT
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)NETLOCK_INPUT, "-p", "udp","--sport","53","-j", (char *)FW_ACCEPT);
	//iptables -A NetLock_Output -p udp --dport 53 -j ACCEPT
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)NETLOCK_OUTPUT, "-p", "udp","--dport","53","-j", (char *)FW_ACCEPT);
	//iptables -t nat -I PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 80
	va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-p", "tcp","--dport","80", "-j", "REDIRECT", "--to-ports", "80");

	//iptables -A NetLock_Input -p udp --dport 68 -j ACCEPT
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)NETLOCK_INPUT, "-p", "udp", "--dport", "68", "-j", (char *)FW_ACCEPT);
	//iptables -A NetLock_Output -p udp --dport 67 -j ACCEPT
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)NETLOCK_OUTPUT, "-p", "udp", "--dport", "67", "-j", (char *)FW_ACCEPT);

	//iptables -A NetLock_Output -j DROP
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)NETLOCK_OUTPUT, "-j", (char *)FW_DROP);
	//iptables -A NetLock_Input -j DROP
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)NETLOCK_INPUT, "-j", (char *)FW_DROP);
	#if 0
	//iptables -I NetLock_Input -s 58.218.215.130 -j ACCEPT
	va_cmd(IPTABLES, 6, 1, (char *)FW_INSERT, (char *)NETLOCK_INPUT, "-s", server_ip, "-j", (char *)FW_ACCEPT);
	//iptables -I NetLock_Output -s 58.218.215.130 -j ACCEPT
	va_cmd(IPTABLES, 6, 1, (char *)FW_INSERT, (char *)NETLOCK_OUTPUT, "-d", server_ip, "-j", (char *)FW_ACCEPT);
	#endif

	//ip6tables -I NetLock_Fwd -i br+ -j DROP
	va_cmd(IP6TABLES, 6, 1, (char *)FW_ADD, (char *)NETLOCK_FWD, "-i","br+","-j", (char *)FW_DROP);

	//ip6tables -A NetLock_Input -p udp --dport 546 -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)NETLOCK_INPUT, "-p", "udp", "--dport", "546", "-j", (char *)FW_ACCEPT);
	//ip6tables -A NetLock_Output -p udp --dport 547 -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)NETLOCK_OUTPUT, "-p", "udp", "--dport", "547", "-j", (char *)FW_ACCEPT);
	//ip6tables -A NetLock_Input -p udp --sport 53 -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)NETLOCK_INPUT, "-p", "udp","--sport","53","-j", (char *)FW_ACCEPT);
	//ip6tables -A NetLock_Output -p udp --dport 53 -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)NETLOCK_OUTPUT, "-p", "udp","--dport","53","-j", (char *)FW_ACCEPT);

	//ip6tables -A NetLock_Output -j DROP
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)NETLOCK_OUTPUT, "-j", (char *)FW_DROP);
	//ip6tables -A NetLock_Input -j DROP
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)NETLOCK_INPUT, "-j", (char *)FW_DROP);
	//ip6tables -t nat -I PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 80
	va_cmd(IP6TABLES, 12, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-p", "tcp","--dport","80", "-j", "REDIRECT", "--to-ports", "80");

#ifdef CONFIG_USER_BOA_WITH_SSL
	//iptables -t nat -I PREROUTING -p tcp --dport 443 -j REDIRECT --to-ports 443
	va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-p", "tcp","--dport","443", "-j", "REDIRECT", "--to-ports", "443");
	//ip6tables -t nat -I PREROUTING -p tcp --dport 443 -j REDIRECT --to-ports 443
	va_cmd(IP6TABLES, 12, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-p", "tcp","--dport","443", "-j", "REDIRECT", "--to-ports", "443");
#endif

	return 0;
}

int rtk_set_net_locked_in_bridge(void)
{
	#if 0
	if(server_ip == NULL)
		return -1;
	#endif

	//ebtables -t nat -I PREROUTING -p IPv4 --ip-proto tcp --ip-dport 80 -j redirect --redirect-target ACCEPT
	va_cmd(EBTABLES, 14, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-p", "IPv4","--ip-proto","tcp", "--ip-dport", "80", "-j", "redirect", "--redirect-target", "ACCEPT");
	//ebtables -t nat -I PREROUTING -p IPv6 --ip6-proto tcp --ip-dport 80 -j redirect --redirect-target ACCEPT
	va_cmd(EBTABLES, 14, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-p", "IPv6","--ip6-proto","tcp", "--ip6-dport", "80", "-j", "redirect", "--redirect-target", "ACCEPT");
	//iptables -t nat -I PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 80
	va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-p", "tcp","--dport","80", "-j", "REDIRECT", "--to-ports", "80");
	//ip6tables -t nat -I PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 80
	va_cmd(IP6TABLES, 12, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-p", "tcp","--dport","80", "-j", "REDIRECT", "--to-ports", "80");

#ifdef CONFIG_USER_BOA_WITH_SSL
	//ebtables -t nat -I PREROUTING -p IPv4 --ip-proto tcp --ip-dport 443 -j redirect --redirect-target ACCEPT
	va_cmd(EBTABLES, 14, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-p", "IPv4","--ip-proto","tcp", "--ip-dport", "443", "-j", "redirect", "--redirect-target", "ACCEPT");
	//ebtables -t nat -I PREROUTING -p IPv6 --ip6-proto tcp --ip-dport 443 -j redirect --redirect-target ACCEPT
	va_cmd(EBTABLES, 14, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-p", "IPv6","--ip6-proto","tcp", "--ip6-dport", "443", "-j", "redirect", "--redirect-target", "ACCEPT");
	//iptables -t nat -I PREROUTING -p tcp --dport 443 -j REDIRECT --to-ports 443
	va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-p", "tcp","--dport","443", "-j", "REDIRECT", "--to-ports", "443");
	//ip6tables -t nat -I PREROUTING -p tcp --dport 443 -j REDIRECT --to-ports 443
	va_cmd(IP6TABLES, 12, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-p", "tcp","--dport","443", "-j", "REDIRECT", "--to-ports", "443");
#endif

	//ebtables -A Br_NetLock_Input -p IPv4 --ip-proto udp --ip-dport 67 -j ACCEPT
	va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char *)BR_NETLOCK_INPUT, "-p", "IPv4","--ip-proto","udp", "--ip-dport", "68", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Output -i nas+ -p IPv4 --ip-proto udp --ip-dport 68 -j ACCEPT
	va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char *)BR_NETLOCK_OUTPUT, "-p", "IPv4","--ip-proto","udp", "--ip-dport", "67", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Input -p IPv6 --ip6-proto udp --ip-dport 546 -j ACCEPT
	va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char *)BR_NETLOCK_INPUT, "-p", "IPv6","--ip6-proto","udp", "--ip6-dport", "546", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Output -i nas+ -p IPv6 --ip6-proto udp --ip-dport 547 -j ACCEPT
	va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char *)BR_NETLOCK_OUTPUT, "-p", "IPv6","--ip6-proto","udp", "--ip6-dport", "547", "-j", (char *)FW_ACCEPT);

	//ebtables -A Br_NetLock_Input -p IPv4 --ip-proto udp --ip-sport 53 -j ACCEPT
	va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char *)BR_NETLOCK_INPUT, "-p", "IPv4","--ip-proto","udp", "--ip-sport", "53", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Output -i nas+ -p IPv4 --ip-proto udp --ip-dport 53 -j ACCEPT
	va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char *)BR_NETLOCK_OUTPUT, "-p", "IPv4","--ip-proto","udp", "--ip-dport", "53", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Input -p IPv6 --ip6-proto udp --ip-sport 53 -j ACCEPT
	va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char *)BR_NETLOCK_INPUT, "-p", "IPv6","--ip6-proto","udp", "--ip6-sport", "53", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Output -i nas+ -p IPv6 --ip6-proto udp --ip-dport 53 -j ACCEPT
	va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char *)BR_NETLOCK_OUTPUT, "-p", "IPv6","--ip6-proto","udp", "--ip6-dport", "53", "-j", (char *)FW_ACCEPT);

	#if 0
	//ebtables -I Br_NetLock_Input -p IPv4 --ip-src 192.168.2.200 -j ACCEPT
	va_cmd(EBTABLES, 8, 1, (char *)FW_INSERT, (char *)BR_NETLOCK_INPUT, "-p", "IPv4","--ip-src", server_ip, "-j", (char *)FW_ACCEPT);
	//ebtables -I Br_NetLock_Output -p IPV4 --ip-dst 192.168.2.200 -j ACCEPT
	va_cmd(EBTABLES, 8, 1, (char *)FW_INSERT, (char *)BR_NETLOCK_OUTPUT, "-p", "IPv4","--ip-dst", server_ip, "-j", (char *)FW_ACCEPT);
	#endif

	//ebtables -A Br_NetLock_Fwd -i ! nas+ -p IPv4 --ip-proto udp --ip-dport 67 -j ACCEPT
	va_cmd(EBTABLES, 13, 1, (char *)FW_ADD, (char *)BR_NETLOCK_FWD, "-i", "!", "nas+", "-p", "IPv4","--ip-proto","udp", "--ip-dport", "67", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Fwd -i nas+ -p IPv4 --ip-proto udp --ip-dport 68 -j ACCEPT
	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)BR_NETLOCK_FWD, "-i", "nas+", "-p", "IPv4","--ip-proto","udp", "--ip-dport", "68", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Fwd -i ! nas+ -p IPv4 --ip-proto udp --ip-dport 53 -j ACCEPT
	va_cmd(EBTABLES, 13, 1, (char *)FW_ADD, (char *)BR_NETLOCK_FWD, "-i", "!", "nas+", "-p", "IPv4","--ip-proto","udp", "--ip-dport", "53", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Fwd -i nas+ -p IPv4 --ip-proto udp --ip-sport 53 -j ACCEPT
	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)BR_NETLOCK_FWD, "-i", "nas+", "-p", "IPv4","--ip-proto","udp", "--ip-sport", "53", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Fwd -i ! nas+ -p IPv6 --ip6-proto udp --ip-dport 547 -j ACCEPT
	va_cmd(EBTABLES, 13, 1, (char *)FW_ADD, (char *)BR_NETLOCK_FWD, "-i", "!", "nas+", "-p", "IPv6","--ip6-proto","udp", "--ip6-dport", "547", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Fwd -i nas+ -p IPv6 --ip6-proto udp --ip-dport 546 -j ACCEPT
	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)BR_NETLOCK_FWD, "-i", "nas+", "-p", "IPv6","--ip6-proto","udp", "--ip6-dport", "546", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Fwd -i ! nas+ -p IPv6 --ip6-proto udp --ip-dport 53 -j ACCEPT
	va_cmd(EBTABLES, 13, 1, (char *)FW_ADD, (char *)BR_NETLOCK_FWD, "-i", "!", "nas+", "-p", "IPv6","--ip6-proto","udp", "--ip6-dport", "53", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Fwd -i nas+ -p IPv6 --ip6-proto udp --ip-sport 53 -j ACCEPT
	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)BR_NETLOCK_FWD, "-i", "nas+", "-p", "IPv6","--ip6-proto","udp", "--ip6-sport", "53", "-j", (char *)FW_ACCEPT);

	//ebtables -A Br_NetLock_Input -p ARP -j ACCEPT
	va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, (char *)BR_NETLOCK_INPUT, "-p", "ARP", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Fwd -p ARP -j ACCEPT
	va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, (char *)BR_NETLOCK_FWD, "-p", "ARP", "-j", (char *)FW_ACCEPT);
	//ebtables -A Br_NetLock_Output -p ARP -j ACCEPT
	va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, (char *)BR_NETLOCK_OUTPUT, "-p", "ARP", "-j", (char *)FW_ACCEPT);

	//ebtables -A Br_NetLock_Fwd -j DROP
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)BR_NETLOCK_FWD, "-j", (char *)FW_DROP);
	//ebtables -A Br_NetLock_Input -j DROP
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)BR_NETLOCK_INPUT, "-j", (char *)FW_DROP);
	//ebtables -A Br_NetLock_Output -j DROP
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)BR_NETLOCK_OUTPUT, "-j", (char *)FW_DROP);

	return 0;
}

void rtk_clean_net_locked_rule()
{
	// iptables -D INPUT -j NetLock_Input
	va_cmd(IPTABLES, 7, 1, (char *)FW_DEL, (char *)FW_INPUT, "!", "-i", "br+", "-j", (char *)NETLOCK_INPUT);
	// iptables -D FORWARD -j NetBlock_Fwd
	va_cmd(IPTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)NETLOCK_FWD);
	// iptables -D OUTPUT -j NetBlock_Output
	va_cmd(IPTABLES, 7, 1, (char *)FW_DEL, (char *)FW_OUTPUT, "!", "-o", "br+", "-j", (char *)NETLOCK_OUTPUT);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)NETLOCK_INPUT);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)NETLOCK_FWD);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)NETLOCK_OUTPUT);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)NETLOCK_INPUT);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)NETLOCK_FWD);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)NETLOCK_OUTPUT);

	// ip6tables -D INPUT -j NetLock_Input
	va_cmd(IP6TABLES, 7, 1, (char *)FW_DEL, (char *)FW_INPUT, "!", "-i", "br+", "-j", (char *)NETLOCK_INPUT);
	// ip6tables -D FORWARD -j NetBlock_Fwd
	va_cmd(IP6TABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)NETLOCK_FWD);
	// ip6tables -D OUTPUT ! -o br+ -j NetBlock_Output
	va_cmd(IP6TABLES, 7, 1, (char *)FW_DEL, (char *)FW_OUTPUT, "!", "-o", "br+", "-j", (char *)NETLOCK_OUTPUT);
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)NETLOCK_INPUT);
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)NETLOCK_FWD);
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)NETLOCK_OUTPUT);
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)NETLOCK_INPUT);
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)NETLOCK_FWD);
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)NETLOCK_OUTPUT);

	// ebtables -D INPUT -j Br_NetBlock_Input
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_INPUT, "-j", (char *)BR_NETLOCK_INPUT);
	// ebtables -D FORWARD -j Br_NetBlock_Fwd
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)BR_NETLOCK_FWD);
	// ebtables -D OUTPUT -j Br_NetBlock_Output
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_OUTPUT, "-j", (char *)BR_NETLOCK_OUTPUT);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)BR_NETLOCK_INPUT);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)BR_NETLOCK_FWD);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)BR_NETLOCK_OUTPUT);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)BR_NETLOCK_INPUT);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)BR_NETLOCK_FWD);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)BR_NETLOCK_OUTPUT);
}
#endif

#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
int rtk_update_manual_load_balance_info_by_wan_ifIndex(unsigned int old_wan_index, unsigned int new_wan_index, int action)
{
	MIB_STATIC_LOAD_BALANCE_T sta_balance_entry;
	int total_manual_ld_num = 0;
	unsigned int i = 0;
	char old_wan_name[IFNAMSIZ] = {0}, new_wan_name[IFNAMSIZ] = {0};

	total_manual_ld_num = mib_chain_total(MIB_STATIC_LOAD_BALANCE_TBL);

	if((old_wan_index == new_wan_index)&&(action == MODIFY_WAN_ENTRY))
		return 0;

	ifGetName(old_wan_index, old_wan_name, sizeof(old_wan_name));
	ifGetName(new_wan_index, new_wan_name, sizeof(new_wan_name));

	for(i = 0; i < total_manual_ld_num; i++){
		if(!mib_chain_get(MIB_STATIC_LOAD_BALANCE_TBL, i, &sta_balance_entry))
				continue;

		if(!strncmp(old_wan_name,sta_balance_entry.wan_itfname,IFNAMSIZ))
		{
			if(action == MODIFY_WAN_ENTRY){

				memset(sta_balance_entry.wan_itfname, 0 ,sizeof(sta_balance_entry.wan_itfname));
				snprintf(sta_balance_entry.wan_itfname, sizeof(sta_balance_entry.wan_itfname),"%s",new_wan_name);

				if(!mib_chain_update(MIB_STATIC_LOAD_BALANCE_TBL, (void*)&sta_balance_entry, i))
				{
					printf("[%s:%d]update laod balance fail!!\n",__FUNCTION__, __LINE__);
					return -1;
				}
			}

			if(action == DEL_WAN_ENTRY){
				//printf("%s %d oldwan:%s sta_wan:%s action:%d\n",__FUNCTION__,__LINE__,old_wan_name,sta_balance_entry.wan_itfname,action);
				if(mib_chain_delete(MIB_STATIC_LOAD_BALANCE_TBL, i) != 1) {
					printf("%s %d delete manual load balance rule fail\n",__FUNCTION__,__LINE__);
					return -1;
				}
			}
		}
	}

	return 0;
}

int set_manual_load_balance_rules(void)
{
	MIB_STATIC_LOAD_BALANCE_T sta_balance_entry;
	MIB_CE_ATM_VC_T wan_entry;
	char manual_load_balance_enable = 0, global_balance_enable = 0;
	int total_wan_num =0, total_manual_ld_num = 0, i = 0, j = 0;
	char wanif[IFNAMSIZ] = {0},str_wan_mark[64] = {0};
	unsigned int wan_mark = 0, wan_mask = 0;

	rtk_flush_manual_firewall();

	if(!mib_get_s(MIB_STATIC_LOAD_BALANCE_ENABLE, (void *)&manual_load_balance_enable,sizeof(manual_load_balance_enable)))
	{
		printf("[%s %d]Get MIB_STATIC_LOAD_BALANCE_ENABLE fail\n",__FUNCTION__,__LINE__);
		return -1;
	}

	if(!mib_get_s(MIB_GLOBAL_LOAD_BALANCE, (void *)&global_balance_enable,sizeof(global_balance_enable)))
	{
		printf("[%s %d]Get MIB_GLOBAL_LOAD_BALANCE fail\n",__FUNCTION__,__LINE__);
		return -1;
	}

	if(manual_load_balance_enable == 0 || global_balance_enable == 0){
		printf("[%s %d]manual load balance is disabled\n",__FUNCTION__,__LINE__);
		return 0;
	}

	total_wan_num = mib_chain_total(MIB_ATM_VC_TBL);
	total_manual_ld_num = mib_chain_total(MIB_STATIC_LOAD_BALANCE_TBL);
	for(i = 0;i< total_wan_num;i++){
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &wan_entry))
			continue;

		ifGetName(wan_entry.ifIndex, wanif, sizeof(wanif));

		for(j=0; j<total_manual_ld_num; j++){
			if(!mib_chain_get(MIB_STATIC_LOAD_BALANCE_TBL, j, &sta_balance_entry))
				continue;

			if(strncmp(sta_balance_entry.wan_itfname, wanif, sizeof(wanif)))
				continue;

			if (!rtk_net_get_wan_policy_route_mark(wanif, &wan_mark, &wan_mask))
			{
				snprintf(str_wan_mark, sizeof(str_wan_mark), "0x%x", wan_mark);
				//va_cmd(IPTABLES, 12, 1,  "-t", "mangle", "-A", (char *)MANUAL_LOAD_BALANCE_PREROUTING, "-m", "mac","--mac-source", (char *)sta_balance_entry.mac, "-j", "MARK", "--set-mark", str_wan_mark);
				va_cmd(EBTABLES, 10, 1,  "-t", "broute", (char *)FW_ADD, (char *)MANUAL_LOAD_BALANCE_PREROUTING, "-s", (char *)sta_balance_entry.mac, "-j", "mark", "--set-mark", (char *)str_wan_mark);
				va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char *)MANUAL_LOAD_BALANCE_OUTPUT, "-d", (char *)sta_balance_entry.mac, "--mark", (char *)str_wan_mark, "-j", (char *)FW_ACCEPT);
				if(wan_entry.cmode == CHANNEL_MODE_BRIDGE){
					//bridge wan
					va_cmd(IPTABLES, 14, 1, "-t", "filter", (char *)FW_ADD, (char *)MANUAL_LOAD_BALANCE_INPUT, "-m", "mac","--mac-source", (char *)sta_balance_entry.mac, "-p", "udp", "--dport", "67", "-j", "DROP");
					va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char *)MANUAL_LOAD_BALANCE_FORWARD, "-i", (char *)wanif, "-d", (char *)sta_balance_entry.mac, "-j", (char *)FW_ACCEPT);
				}
			}
		}
	}

	return 0;
}

void rtk_init_manual_firewall(void)
{
	va_cmd(IPTABLES, 2, 1, "-N", (char *)MANUAL_LOAD_BALANCE_INPUT);
	va_cmd(IPTABLES, 6, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-i", "br+", "-j", (char *)MANUAL_LOAD_BALANCE_INPUT);

	va_cmd(EBTABLES, 2, 1, "-N", (char *)MANUAL_LOAD_BALANCE_FORWARD);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)MANUAL_LOAD_BALANCE_FORWARD, (char *)FW_RETURN);
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", (char *)MANUAL_LOAD_BALANCE_FORWARD);

	va_cmd(EBTABLES, 2, 1, "-N", (char *)MANUAL_LOAD_BALANCE_OUTPUT);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)MANUAL_LOAD_BALANCE_OUTPUT, (char *)FW_RETURN);
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_OUTPUT, "-j", (char *)MANUAL_LOAD_BALANCE_OUTPUT);

	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N", (char *)MANUAL_LOAD_BALANCE_PREROUTING);
	va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P", (char *)MANUAL_LOAD_BALANCE_PREROUTING, (char *)FW_RETURN);
	va_cmd(EBTABLES, 6, 1, "-t", "broute", (char *)FW_ADD, "BROUTING", "-j", (char *)MANUAL_LOAD_BALANCE_PREROUTING);
}

void rtk_del_manual_firewall(void)
{
	va_cmd(IPTABLES, 2, 1, "-F", (char *)MANUAL_LOAD_BALANCE_INPUT);
	va_cmd(IPTABLES, 6, 1, "-D", (char *)FW_INPUT, "-i", "br+", "-j", (char *)MANUAL_LOAD_BALANCE_INPUT);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)MANUAL_LOAD_BALANCE_INPUT);
	va_cmd(EBTABLES, 2, 1, "-F", (char *)MANUAL_LOAD_BALANCE_FORWARD);
	va_cmd(EBTABLES, 4, 1, "-D", (char *)FW_FORWARD, "-j", (char *)MANUAL_LOAD_BALANCE_FORWARD);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)MANUAL_LOAD_BALANCE_FORWARD);
	va_cmd(EBTABLES, 2, 1, "-F", (char *)MANUAL_LOAD_BALANCE_OUTPUT);
	va_cmd(EBTABLES, 4, 1, "-D", (char *)FW_OUTPUT, "-j", (char *)MANUAL_LOAD_BALANCE_OUTPUT);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)MANUAL_LOAD_BALANCE_OUTPUT);
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F",(char *)MANUAL_LOAD_BALANCE_PREROUTING);
	va_cmd(EBTABLES, 6, 1, "-t", "broute", "-D", "BROUTING", "-j", (char *)MANUAL_LOAD_BALANCE_PREROUTING);
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X",(char *)MANUAL_LOAD_BALANCE_PREROUTING);
}

void rtk_flush_manual_firewall(void)
{
	va_cmd(IPTABLES, 2, 1, "-F", (char *)MANUAL_LOAD_BALANCE_INPUT);
	va_cmd(EBTABLES, 2, 1, "-F", (char *)MANUAL_LOAD_BALANCE_FORWARD);
	va_cmd(EBTABLES, 2, 1, "-F", (char *)MANUAL_LOAD_BALANCE_OUTPUT);
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", (char *)MANUAL_LOAD_BALANCE_PREROUTING);
}
extern uint32_t wan_diff_itfgroup;
void rtk_init_dynamic_firewall(void)
{
	char polmark[64] = {0};
	uint32_t wan_mark = 0, wan_mask = 0, count=0;;
	int i, j, total = 0;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ] = {0};
   	con_wan_info_T *pConWandata=NULL;
    char cmd[128] = {0};
    char ifName[IFNAMSIZ];

	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N", (char *)DYNAMIC_IP_LOAD_BALANCE);
	va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P", (char *)DYNAMIC_IP_LOAD_BALANCE, (char *)FW_RETURN);
	va_cmd(EBTABLES, 6, 1, "-t", "broute", (char *)FW_INSERT, "BROUTING", "-j", (char *)DYNAMIC_IP_LOAD_BALANCE);

	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N", (char *)DYNAMIC_MAC_LOAD_BALANCE);
	va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P", (char *)DYNAMIC_MAC_LOAD_BALANCE, (char *)FW_RETURN);
	va_cmd(EBTABLES, 6, 1, "-t", "broute", (char *)FW_INSERT, "BROUTING", "-j", (char *)DYNAMIC_MAC_LOAD_BALANCE);
	//for bridge wan
	va_cmd(EBTABLES, 4, 1,"-t", "filter", "-N", (char *)DYNAMIC_INPUT_LOAD_BALANCE);
	va_cmd(EBTABLES, 5, 1,"-t", "filter", "-P", (char *)DYNAMIC_INPUT_LOAD_BALANCE, (char *)FW_RETURN);
	va_cmd(EBTABLES, 6, 1,"-t", "filter", (char *)FW_INSERT, (char *)FW_INPUT, "-j", (char *)DYNAMIC_INPUT_LOAD_BALANCE);

	va_cmd(EBTABLES, 4, 1,"-t", "filter", "-N", (char *)DYNAMIC_FORWARD_LOAD_BALANCE);
	va_cmd(EBTABLES, 5, 1,"-t", "filter", "-P", (char *)DYNAMIC_FORWARD_LOAD_BALANCE, (char *)FW_RETURN);
	va_cmd(EBTABLES, 6, 1,"-t", "filter", (char *)FW_INSERT, (char *)FW_FORWARD, "-j", (char *)DYNAMIC_FORWARD_LOAD_BALANCE);

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if(!Entry.enable)
			continue;

		ifGetName(Entry.ifIndex, ifName, sizeof(ifName));
		rtk_net_get_wan_policy_route_mark(ifName, &wan_mark, &wan_mask);
		sprintf(polmark, "0x%x/0x%x", wan_mark, SOCK_MARK_WAN_INDEX_BIT_MASK);

		ifGetName(PHY_INTF(Entry.ifIndex), ifname, sizeof(ifname));
		if(Entry.cmode == CHANNEL_MODE_BRIDGE)
			va_cmd(EBTABLES, 9, 1, (char *)FW_ADD, (char *)DYNAMIC_FORWARD_LOAD_BALANCE, "-o", ifname, "!", "--mark", polmark , "-j", "DROP");
	}

	if(get_con_wan_info(&pConWandata, &count) == 0)
	{
		for(i=0; i<count; i++)
		{
			if(pConWandata[i].wan_mark)
			{
				sprintf(polmark, "0x%x", pConWandata[i].wan_mark);

				for(j = 0; j < MAX_MAC_NUM ; j++)
				{	//only support V4
					if(isZeroMacstr(pConWandata[i].mac_info[j].mac_str)==0)
						va_cmd(EBTABLES, 12, 1, "-t", "broute", "-A", (char *)DYNAMIC_MAC_LOAD_BALANCE, "-s", pConWandata[i].mac_info[j].mac_str, "-p", "IPv4", "-j", "mark", "--set-mark", polmark, "--mark-target", "CONTINUE");
				}
			}
		}
	}

	//when port mapping change,just indicate systemd to delete rules,don't re-assign STAs(re-assign when wan connect),because wan MIB hasn't been updated
	if(wan_diff_itfgroup)
	{
		memset(cmd, 0, sizeof(cmd));
		snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_web_change_process port_map_change %d",wan_diff_itfgroup);
		system(cmd);
	}


	return;
}

void rtk_del_dynamic_firewall(void)
{
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", (char *)DYNAMIC_IP_LOAD_BALANCE);
	va_cmd(EBTABLES, 6, 1, "-t", "broute", "-D", "BROUTING", "-j", (char *)DYNAMIC_IP_LOAD_BALANCE);
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X", (char *)DYNAMIC_IP_LOAD_BALANCE);

	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", (char *)DYNAMIC_MAC_LOAD_BALANCE);
	va_cmd(EBTABLES, 6, 1, "-t", "broute", "-D", "BROUTING", "-j", (char *)DYNAMIC_MAC_LOAD_BALANCE);
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X", (char *)DYNAMIC_MAC_LOAD_BALANCE);

	//for bridge wan
	va_cmd(EBTABLES, 2, 1, "-F",(char *)DYNAMIC_INPUT_LOAD_BALANCE);
	va_cmd(EBTABLES, 6, 1, "-t", "filter", "-D", (char *)FW_INPUT, "-j", (char *)DYNAMIC_INPUT_LOAD_BALANCE);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)DYNAMIC_INPUT_LOAD_BALANCE);

	va_cmd(EBTABLES, 2, 1, "-F",(char *)DYNAMIC_FORWARD_LOAD_BALANCE);
	va_cmd(EBTABLES, 6, 1, "-t", "filter", "-D", (char *)FW_FORWARD, "-j", (char *)DYNAMIC_FORWARD_LOAD_BALANCE);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)DYNAMIC_FORWARD_LOAD_BALANCE);

	return;
}

void rtk_flush_dynamic_firewall(void)
{
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", (char *)DYNAMIC_IP_LOAD_BALANCE);

	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", (char *)DYNAMIC_MAC_LOAD_BALANCE);

	//for bridge wan
	va_cmd(EBTABLES, 2, 1, "-F",(char *)DYNAMIC_INPUT_LOAD_BALANCE);

	va_cmd(EBTABLES, 2, 1, "-F",(char *)DYNAMIC_FORWARD_LOAD_BALANCE);

	return;
}

#endif

/*
 *	For dns request packet, set skb mark to make it look up policy route.
 */
void rtk_fw_set_skb_mark_for_dns_pkt(void)
{
	int vcTotal, i;
	unsigned int wan_mark = 0, wan_mask = 0;
	char polmark[64] = {0};
	char wan_ifname[IFNAMSIZ] = {0};
	MIB_CE_ATM_VC_T entry;


	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", (char *)FW_OUTPUT, "-j", WAN_OUTPUT_RTTBL);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", WAN_OUTPUT_RTTBL);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", WAN_OUTPUT_RTTBL);

#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", (char *)FW_OUTPUT, "-j", WAN_OUTPUT_RTTBL);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", WAN_OUTPUT_RTTBL);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", WAN_OUTPUT_RTTBL);
#endif

	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", WAN_OUTPUT_RTTBL);
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", (char *)FW_OUTPUT, "-j", WAN_OUTPUT_RTTBL);

#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", WAN_OUTPUT_RTTBL);
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", (char *)FW_OUTPUT, "-j", WAN_OUTPUT_RTTBL);
#endif

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < vcTotal; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
			continue;

		if (entry.enable == 0 || entry.cmode == CHANNEL_MODE_BRIDGE)
			continue;

		ifGetName(entry.ifIndex, wan_ifname, sizeof(wan_ifname));
		if (rtk_net_get_wan_policy_route_mark(wan_ifname, &wan_mark, &wan_mask) != 0)
			continue;

		snprintf(polmark, sizeof(polmark), "0x%x/0x%x", wan_mark, wan_mask);

		va_cmd(IPTABLES, 16, 1, "-t", "mangle", "-A", (char *)WAN_OUTPUT_RTTBL, "-o", wan_ifname, "-p", "udp", "-m", "udp", "--dport", "53", "-j", "MARK", "--set-mark", polmark);

#if defined(CONFIG_IPV6)
		va_cmd(IP6TABLES, 16, 1, "-t", "mangle", "-A", (char *)WAN_OUTPUT_RTTBL, "-o", wan_ifname, "-p", "udp", "-m", "udp", "--dport", "53", "-j", "MARK", "--set-mark", polmark);
#endif
	}
}

#if defined(DHCP_ARP_IGMP_RATE_LIMIT)
const char FW_DHCPARPIGMP_RATE_LIMIT_INPUT[] = "dhcparpigmp_rate_b_INPUT";
const char FW_DHCPARPIGMP_RATE_LIMIT_FORWARD[] = "dhcparpigmp_rate_b_FORWARD";
void rtk_cmcc_flush_dhcparpigmp_rate_limit(void)
{
	/* dhcparpigmp_rate_b_INPUT */
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_DHCPARPIGMP_RATE_LIMIT_INPUT);
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_INPUT, "-j", (char *)FW_DHCPARPIGMP_RATE_LIMIT_INPUT);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_DHCPARPIGMP_RATE_LIMIT_INPUT);

	/* dhcparpigmp_rate_b_FORWARD */
	va_cmd(EBTABLES, 4, 1, "-t", "nat", "-F", (char *)FW_DHCPARPIGMP_RATE_LIMIT_FORWARD);
	va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", (char *)FW_DHCPARPIGMP_RATE_LIMIT_FORWARD);
	va_cmd(EBTABLES, 4, 1, "-t", "nat", "-X", (char *)FW_DHCPARPIGMP_RATE_LIMIT_FORWARD);
	return;
}

int rtk_cmcc_setup_dhcparpigmp_rate_limit()
{
	int total=0,i=0;
	MIB_L2_BC_LIMIT_T entry={0};
	char port_ifname[16]={0}, limit_str[8]={0};
	/* dhcparpigmp_rate_b_INPUT */
	va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_DHCPARPIGMP_RATE_LIMIT_INPUT);
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_DHCPARPIGMP_RATE_LIMIT_INPUT);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)FW_DHCPARPIGMP_RATE_LIMIT_INPUT, "RETURN");

	/* dhcparpigmp_rate_b_FORWARD */
	va_cmd(EBTABLES, 4, 1, "-t", "nat", "-N", (char *)FW_DHCPARPIGMP_RATE_LIMIT_FORWARD);
	va_cmd(EBTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)FW_DHCPARPIGMP_RATE_LIMIT_FORWARD);
	va_cmd(EBTABLES, 5, 1, "-t", "nat", "-P", (char *)FW_DHCPARPIGMP_RATE_LIMIT_FORWARD, "RETURN");

	total = mib_chain_total(MIB_L2_BROADCAST_LIMIT_TBL);
	for(i=0; i<total && i<ELANVIF_NUM; i++)
	{
		sprintf(port_ifname, "%s", ELANVIF[i]);
		if (!mib_chain_get(MIB_L2_BROADCAST_LIMIT_TBL, i, &entry))
			continue;
		if(!entry.dhcpLimit && !entry.igmpLimit && !entry.arpLimit)
			continue;
		if(entry.dhcpLimit)
		{
			snprintf(limit_str, sizeof(limit_str), "%d", entry.dhcpLimit);
			va_cmd(EBTABLES, 14, 1, (char *)FW_ADD, (char *)FW_DHCPARPIGMP_RATE_LIMIT_INPUT, "-i", (char *)port_ifname,
				"-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "67:68", "--limit", (char *)limit_str, "-j", "RETURN");
			va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)FW_DHCPARPIGMP_RATE_LIMIT_INPUT, "-i", (char *)port_ifname,
				"-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "67:68", "-j", "DROP");
			va_cmd(EBTABLES, 16, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_DHCPARPIGMP_RATE_LIMIT_FORWARD, "-i", (char *)port_ifname,
				"-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "67:68", "--limit", (char *)limit_str, "-j", "RETURN");
			va_cmd(EBTABLES, 14, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_DHCPARPIGMP_RATE_LIMIT_FORWARD, "-i", (char *)port_ifname,
				"-p", "IPv4", "--ip-proto", "udp", "--ip-dport", "67:68",  "-j", "DROP");
		}
		if(entry.igmpLimit)
		{
			snprintf(limit_str, sizeof(limit_str), "%d", entry.igmpLimit);
			va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)FW_DHCPARPIGMP_RATE_LIMIT_INPUT, "-i", (char *)port_ifname,
				"-p", "IPv4", "--ip-proto", "igmp",  "--limit", (char *)limit_str, "-j", "RETURN");
			va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char *)FW_DHCPARPIGMP_RATE_LIMIT_INPUT, "-i", (char *)port_ifname,
				"-p", "IPv4", "--ip-proto", "igmp",  "-j", "DROP");
			va_cmd(EBTABLES, 14, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_DHCPARPIGMP_RATE_LIMIT_FORWARD, "-i", (char *)port_ifname,
				"-p", "IPv4", "--ip-proto", "igmp",  "--limit", (char *)limit_str, "-j", "RETURN");
			va_cmd(EBTABLES, 12, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_DHCPARPIGMP_RATE_LIMIT_FORWARD, "-i", (char *)port_ifname,
				"-p", "IPv4", "--ip-proto", "igmp",  "-j", "DROP");
		}
		if(entry.arpLimit)
		{
			snprintf(limit_str, sizeof(limit_str), "%d", entry.arpLimit);
			va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, (char *)FW_DHCPARPIGMP_RATE_LIMIT_INPUT, "-i", (char *)port_ifname,
				"-p", "0x0806",  "--limit", (char *)limit_str, "-j", "RETURN");
			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, (char *)FW_DHCPARPIGMP_RATE_LIMIT_INPUT, "-i", (char *)port_ifname,
				"-p", "0x0806",  "-j", "DROP");
			va_cmd(EBTABLES, 12, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_DHCPARPIGMP_RATE_LIMIT_FORWARD, "-i", (char *)port_ifname,
				"-p", "0x0806",  "--limit", (char *)limit_str, "-j", "RETURN");
			va_cmd(EBTABLES, 10, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_DHCPARPIGMP_RATE_LIMIT_FORWARD, "-i", (char *)port_ifname,
				"-p", "0x0806",  "-j", "DROP");
		}
	}

	return 0;
}
int initDhcpIgmpArpLimit()
{
	rtk_cmcc_flush_dhcparpigmp_rate_limit();
	rtk_cmcc_setup_dhcparpigmp_rate_limit();

	return 0;
}
#endif

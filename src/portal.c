#ifdef EMBED
#include <rtk/utility.h>
#else
#include "LINUX/utility.h"
#endif
#include "boa.h"

#ifdef RTK_BOA_PORTAL_TO_NET_LOCKED_UI

//#define PORTAL_DEBUG 1

#ifdef PORTAL_DEBUG
#define PORTAL_DEBUG_INFO(fmt, arg...)	do{printf("[%s:%d]"fmt"\n", __FUNCTION__, __LINE__, ##arg);}while(0)
#else
#define PORTAL_DEBUG_INFO(fmt, arg...)	do{}while(0)
#endif

const unsigned char LAN_IFACE[] = "br0";
#define DNS_TRAP_ENABLE_PROC_FILE "/proc/driver/realtek/dns_trap_enable"
#define DNS_TRAP_DOMAIN_NAME_PROC_FILE "/proc/driver/realtek/domain_name"

int rtk_http_host_is_DUT_domain(request * req){
#ifdef CONFIG_RTK_DNS_TRAP
	char line[64]={0};//MAX_NAME_LEN
	int enable=0, str_len=0;
	char domain_name[64]={0};
	FILE *fp;
	fp = fopen(DNS_TRAP_ENABLE_PROC_FILE, "r");
	if(fp != NULL){
		if(fgets(line, sizeof(line), fp) != NULL){
			enable = line[0]-'0';
		}
		fclose(fp);
	}
	if(enable){
		fp = fopen(DNS_TRAP_DOMAIN_NAME_PROC_FILE, "r");
		if(fp != NULL){
			memset(line,0x00,sizeof(line));
			if(fgets(line, sizeof(line), fp) != NULL){
				snprintf(domain_name, sizeof(domain_name), "%s", line);	
				str_len = strlen(domain_name);
				if(str_len > 0 && domain_name[str_len-1]=='\n')	//domain_name ends with \n	
					domain_name[str_len-1]='\0';
			}
			fclose(fp);
			
			if(strlen(req->host) == strlen(domain_name) && !strcmp(req->host, domain_name))
				return 1;
		}	
	}
	PORTAL_DEBUG_INFO("req->host=%s(len=%d), dnstrap_enable=%d, domain=%s(len=%d)", req->host, strlen(req->host),enable, domain_name,strlen(domain_name));
#endif
	return 0;
}

int rtk_http_host_is_DUT_ip(request * req, unsigned char *iface){
	struct in_addr in_addr;
	unsigned char addr_str[64]={0};

	if (getInAddr(iface, IP_ADDR, (void *)&in_addr)) {
		if(inet_ntop(AF_INET, (void *)&in_addr, addr_str, INET_ADDRSTRLEN)){		
			PORTAL_DEBUG_INFO("req->host=%s, [br0 v4_addr=%s]", req->host, addr_str);
			if((strlen(req->host) == strlen(addr_str)) && !strcmp(req->host, addr_str))
				return 1;
		}
	}
	
#ifdef CONFIG_IPV6
	struct ipv6_ifaddr ip6_addr[6];
	unsigned char http_addr_str[64]={0};

	if(getifip6(iface, IPV6_ADDR_LINKLOCAL, ip6_addr, 1)){
		if(inet_ntop(PF_INET6, (void *)&(ip6_addr[0].addr), addr_str, INET6_ADDRSTRLEN)){	
			snprintf(http_addr_str, sizeof(http_addr_str), "[%s]", addr_str);
			PORTAL_DEBUG_INFO("req->host=%s, [br0 v6_addr=%s]", req->host, http_addr_str);
			if((strlen(req->host) == strlen(http_addr_str)) && !strcmp(req->host, http_addr_str))
				return 1;
		}		
	}
#endif

	return 0;
}

int rtk_access_internet_with_net_locked(request * req){
	unsigned char enabled=0;
	if(!req || !req->host)
		return 0;

	PORTAL_DEBUG_INFO("\n\nreq->host = %s", req->host);
	if(isFileExist(RTK_NET_LOCKED_STATUS_FILE) == 0){//net not locked
		PORTAL_DEBUG_INFO("net is not locked");
		return 0;
	}
	
	if(rtk_http_host_is_DUT_domain(req) == 1){//host is domain name when dnstrap
		PORTAL_DEBUG_INFO("host is DUT domain!");
		return 0;
	}
	if(rtk_http_host_is_DUT_ip(req, (unsigned char *)LAN_IFACE) == 1){//host is br0 IPv4/IPv6 address
		PORTAL_DEBUG_INFO("host is DUT IP address!");
		return 0;
	}
	return 1;
}

int rtk_portal_to_netlocked_ui(request * req){	
	PORTAL_DEBUG_INFO("portal to net locked UI!");
	send_r_portal_net_locked(req);
	return 0;
}

int rtk_get_wanip_of_andlink(char *wanIface, char *wanIpStr, int wanIpStrLen){
	char cmd[32] = {0}, addr_str[40]={0};
	FILE *pp = NULL;
	int ret = 0;

	if (!wanIpStr)
		return ret;
	
#ifdef CONFIG_USER_CURL	
	snprintf(cmd, sizeof(cmd), "curl ifconfig.me");
	if ( (pp = popen(cmd, "r")) != NULL){
		if (fgets(addr_str, sizeof(addr_str), pp) != NULL){			
			snprintf(wanIpStr, wanIpStrLen, "%s", addr_str);
			ret = 1;
		}
		pclose(pp);
	}
#else
	printf("[%s:%d] curl is not supported!!!\n", __FUNCTION__,__LINE__);
#endif
	return ret;	
}


#endif

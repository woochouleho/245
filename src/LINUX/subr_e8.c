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
#include <ctype.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/file.h>
#include <stdarg.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>


#include "rtk_timer.h"
#include "mib.h"
#include "utility.h"
#include "debug.h"
 
#include "subr_net.h"

#include "module/gpon/gpon.h"
#if defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_gpon.h>
#include <rtk/rt/rt_cls.h>
#include <rtk/rt/rt_switch.h>
#endif
#include "sockmark_define.h"

#include "subr_net.h"
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD) \
	|| defined(CONFIG_USER_PPTP_CLIENT) || defined(CONFIG_USER_XL2TPD)
#include <regex.h>
#endif
#include <pthread.h>

#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#include "fc_api.h"
#endif

#ifdef CONFIG_SUPPORT_AUTO_DIAG
#ifdef CONFIG_RTL_MULTI_ETH_WAN
void addSimuEthWANdev(MIB_CE_ATM_VC_Tp pEntry, int autosimu);
#endif
int setup_simu_ethernet_config(MIB_CE_ATM_VC_Tp pEntry, char *wanif);
#endif

#ifdef CONFIG_CU_BASEON_YUEME
/*sync with rtk_host.c enum*/
enum Host_InterfaceType
{
	Host_Ethernet = 0,
	Host_802_11,
};
#endif

#ifdef CONFIG_TR142_MODULE
#include <rtk/rtk_tr142.h>
#endif
#ifdef SUPPORT_TIME_SCHEDULE
unsigned int schdMacFilterIdx[SCHD_MAC_FILTER_SIZE];
#endif

#ifdef _PRMT_X_CT_COM_WANEXT_
const char IPFORWARD_TBL[] = "ipforwardlist";

void set_IPForward_by_WAN_entry(MIB_CE_ATM_VC_Tp pentry)
{
	int ipver = 0, flags = 0, flags_found = 0, isPPP = 0;
	char *pToken = NULL, *pSave = NULL, *pTmp = NULL;
	char ifname[IFNAMSIZ]= {0}, gw_v4[INET_ADDRSTRLEN] = {0}, *tmpList = NULL, tmpIP_s[64] = {0}, tmpIP_e[64] = {0};
	unsigned char ip_s[sizeof(struct in6_addr)] = {0}, ip_e[sizeof(struct in6_addr)] = {0};
#ifdef CONFIG_IPV6
	char gw_v6[INET6_ADDRSTRLEN] = {0};
#endif
	char chainname[32];
	unsigned int wan_mark,wan_mask;
	char polmark[64]={0};
	char iprange[128]={0};

	if(!pentry) return;
	ifGetName(pentry->ifIndex, ifname, sizeof(ifname));
	printf("%s:ifIndex=%d,ifname=%s\n",__func__,pentry->ifIndex,ifname);

	sprintf(chainname,"ipforward_%s",ifname);
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", (char *)IPFORWARD_TBL, "-j", chainname);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", chainname);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", chainname);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", (char *)IPFORWARD_TBL, "-j", chainname);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", chainname);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", chainname);
#endif

	if (
#ifdef _PRMT_X_CU_EXTEND_
		pentry->IPForwardModeEnabled &&
#endif
		(pentry->IPForwardList[0] != '\0')){
		flags_found = getInFlags(ifname, &flags);
		switch (pentry->cmode) {
			case CHANNEL_MODE_BRIDGE:
				isPPP = 0;
				break;
			case CHANNEL_MODE_IPOE:
				isPPP = 0;
				break;
			case CHANNEL_MODE_PPPOE:
				isPPP = 1;
				break;
			case CHANNEL_MODE_PPPOA:
				isPPP = 1;
				break;
			default:
				isPPP = 0;
				break;
		}
		if (flags_found) {
			if (isPPP) {
				struct in_addr inAddr;
				if (getInAddr(ifname, DST_IP_ADDR, &inAddr) == 1)
					strcpy(gw_v4, inet_ntoa(inAddr));
			}
			else {
				if(pentry->ipDhcp == (char)DHCP_CLIENT) {
					FILE *fp = NULL;
					char fname[128] = {0};
					sprintf(fname, "%s.%s", MER_GWINFO, ifname);
					if ((fp = fopen(fname, "r"))) {
						if(fscanf(fp, "%s", gw_v4)!=1)
							printf("[%s %d] fscanf fail\n", __FUNCTION__, __LINE__);
						fclose(fp);
					}
				}
				else {
					unsigned char zero[IP_ADDR_LEN] = {0};
					if (memcmp(pentry->remoteIpAddr, zero, IP_ADDR_LEN))
						strncpy(gw_v4, inet_ntoa(*((struct in_addr *)pentry->remoteIpAddr)),INET_ADDRSTRLEN-1);
				}
			}
#ifdef CONFIG_IPV6
			if ((pentry->cmode != CHANNEL_MODE_BRIDGE) && (pentry->IpProtocol & IPVER_IPV6)) {
				unsigned char zero[IP6_ADDR_LEN] = {0};
				if (memcmp(pentry->RemoteIpv6Addr, zero, IP6_ADDR_LEN))
					inet_ntop(AF_INET6, &pentry->RemoteIpv6Addr, gw_v6, INET6_ADDRSTRLEN);
			}
#endif
		}

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", chainname);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", (char *)IPFORWARD_TBL, "-j", chainname);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", chainname);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", (char *)IPFORWARD_TBL, "-j", chainname);
#endif
		tmpList = pentry->IPForwardList;
		pToken = strtok_r(tmpList, ",", &pSave);
		while (pToken) {
			tmpIP_s[0] = '\0';
			tmpIP_e[0] = '\0';
			pTmp = strchr(pToken, '-');
			if (pTmp != NULL) {
				*pTmp = 0;
				pTmp++;
				snprintf(tmpIP_e,sizeof(tmpIP_e), "%s", pTmp);
			}
			snprintf(tmpIP_s, sizeof(tmpIP_s), "%s", pToken);

			if ((pentry->IpProtocol & IPVER_IPV4) && (inet_pton(AF_INET, tmpIP_s, ip_s) == 1) && (tmpIP_e[0] == '\0' || inet_pton(AF_INET, tmpIP_e, ip_e) == 1)) {
				if(!rtk_net_get_wan_policy_route_mark(ifname, &wan_mark, &wan_mask)){
					printf("[%s@%d] IP [%s]-[%s], Gateway [%s]\n", __FUNCTION__, __LINE__,tmpIP_s, (tmpIP_e[0] == '\0')?tmpIP_s:tmpIP_e, gw_v4);
					sprintf(polmark, "0x%x/0x%x", wan_mark, wan_mask);
					if(tmpIP_e[0] == '\0')	
						va_cmd(IPTABLES, 10, 1, "-t", "mangle", "-A", (char *)chainname, "-d", tmpIP_s, "-j", "MARK", "--set-mark", polmark);
					else{
						sprintf(iprange,"%s-%s",tmpIP_s,tmpIP_e);
						va_cmd(IPTABLES, 12, 1, "-t", "mangle", "-A", (char *)chainname, "-m", "iprange", "--dst-range", iprange, "-j", "MARK", "--set-mark", polmark);
					}
				}
			}
#ifdef CONFIG_IPV6
			else if ((pentry->IpProtocol & IPVER_IPV6) && (inet_pton(AF_INET6, tmpIP_s, ip_s) == 1) && (tmpIP_e[0] == '\0' || inet_pton(AF_INET6, tmpIP_e, ip_e) == 1)) {
				if(!rtk_net_get_wan_policy_route_mark(ifname, &wan_mark, &wan_mask)){
					printf("[%s@%d] IP [%s]-[%s], Gateway [%s]\n", __FUNCTION__, __LINE__,tmpIP_s, (tmpIP_e[0] == '\0')?tmpIP_s:tmpIP_e, gw_v6);
					sprintf(polmark, "0x%x/0x%x", wan_mark, wan_mask);
					if(tmpIP_e[0] == '\0')	
						va_cmd(IP6TABLES, 10, 1, "-t", "mangle", "-A", (char *)chainname, "-d", tmpIP_s, "-j", "MARK", "--set-mark", polmark);
					else{
						sprintf(iprange,"%s-%s",tmpIP_s,tmpIP_e);
						va_cmd(IP6TABLES, 12, 1, "-t", "mangle", "-A", (char *)chainname, "-m", "iprange", "--dst-range", iprange, "-j", "MARK", "--set-mark", polmark);
					}
				}
			}
#endif
			
			pToken = strtok_r(pSave, ",", &pSave);
		}
	}
}

void set_IPForward_by_WAN(int wan_idx)
{
	MIB_CE_ATM_VC_T wan_entry;

	if(mib_chain_get(MIB_ATM_VC_TBL, wan_idx, &wan_entry))
	{
		return set_IPForward_by_WAN_entry(&wan_entry);
	}
}

void handle_IPForwardMode(int wan_idx)
{
	unsigned char enable = 0;
	int i = 0, total_entry = 0;
	MIB_CE_ATM_VC_T wan_entry;
	mib_get_s(CWMP_CT_IP_FORWARD_MODE_ENABLED, (void *)&enable, sizeof(enable));
	total_entry = mib_chain_total(MIB_ATM_VC_TBL);

	if (enable) {
		int isRemove = 0;
		for (i = 0 ; i < total_entry ; i++) {
			if(mib_chain_get(MIB_ATM_VC_TBL, i, &wan_entry) == 0)
				continue;

			int isModify = 0;
			if ((wan_entry.applicationtype & X_CT_SRV_VOICE) && wan_entry.napt == 0) {
				wan_entry.napt = 1;
				isModify = 1;
			}

			setup_NAT(&wan_entry, 1);
			
#ifdef NEW_PORTMAPPING
			if (wan_entry.itfGroup) { // Remove port-based mapping
				wan_entry.itfGroup = 0;
				isRemove = 1;
				isModify = 1;
			}
#endif
			if (isModify)
				mib_chain_update(MIB_ATM_VC_TBL, (void *)&wan_entry, i);
		}

#ifdef NEW_PORTMAPPING
		if (isRemove) {
			setupnewEth2pvc();
		}
#endif
	}

	if (wan_idx == (-1)) { // All WAN
		for (i = 0 ; i < total_entry ; i++) {
			if(mib_chain_get(MIB_ATM_VC_TBL, i, &wan_entry) == 0)
				continue;
			set_IPForward_by_WAN_entry(&wan_entry);
		}
	}
	else {
		if(mib_chain_get(MIB_ATM_VC_TBL, wan_idx, &wan_entry))
			set_IPForward_by_WAN_entry(&wan_entry);
	}
}

void clean_IPForward_by_WAN_entry(MIB_CE_ATM_VC_Tp pentry)
{
	char ifname[IFNAMSIZ]= {0};
	char chainname[32];
	if(!pentry) return;

	ifGetName(pentry->ifIndex, ifname, sizeof(ifname));
	
	printf("%s:ifindex=%u,ifname=%s\n",__func__,pentry->ifIndex,ifname);
	sprintf(chainname,"ipforward_%s",ifname);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", chainname);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", chainname);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", chainname);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", chainname);
#endif
}

void clean_IPForward_by_WAN(int wan_idx)
{
	MIB_CE_ATM_VC_T wan_entry;

	if(mib_chain_get(MIB_ATM_VC_TBL, wan_idx, &wan_entry))
	{
		return clean_IPForward_by_WAN_entry(&wan_entry);
	}
}

void rtk_fw_set_ipforward(int addflag)
{
	int i = 0, total_entry = 0;
	MIB_CE_ATM_VC_T wan_entry;
	total_entry = mib_chain_total(MIB_ATM_VC_TBL);
			
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", (char *)FW_PREROUTING, "-j", IPFORWARD_TBL);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", IPFORWARD_TBL);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", IPFORWARD_TBL);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", (char *)FW_PREROUTING, "-j", IPFORWARD_TBL);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", IPFORWARD_TBL);
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", IPFORWARD_TBL);
#endif
	
	if(addflag){
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", IPFORWARD_TBL);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", (char *)FW_PREROUTING, "-j", IPFORWARD_TBL);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", IPFORWARD_TBL);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", (char *)FW_PREROUTING, "-j", IPFORWARD_TBL);
#endif
		for (i = 0 ; i < total_entry ; i++) {
			if(mib_chain_get(MIB_ATM_VC_TBL, i, &wan_entry) == 0)
				continue;
#ifdef _PRMT_X_CU_EXTEND_
			handle_WANIPForwardMode(i);
#else
			handle_IPForwardMode(i);
#endif
		}
	}

}
#endif
#ifdef CONFIG_USER_OPENSSL
#include <openssl/des.h>
int do_3des(const unsigned char *key, const unsigned char *in, unsigned char *out, size_t len, int enc_act)
{
	DES_key_schedule ks1, ks2, ks3;
	DES_cblock block_key;
	DES_cblock tmp1, tmp2;
	int cnt, i;

	if(len&0x7)
	{
		fprintf(stderr, "[%s %d] in data len not 8*x byte size\n",__func__,__LINE__);
		return -1;
	}

	memcpy(&block_key, key, 8);
	DES_set_key_unchecked(&block_key, &ks1);

	memcpy(&block_key, key + 8, 8);
    DES_set_key_unchecked(&block_key, &ks2);

	memcpy(&block_key, key + 16, 8);
    DES_set_key_unchecked(&block_key, &ks3);

	cnt = len / 8;

	for(i = 0; i < cnt ; i++)
	{
		memcpy(tmp1, in + 8*i, 8);
		DES_ecb3_encrypt(&tmp1, &tmp2, &ks1, &ks2, &ks3, enc_act);
		memcpy(out + 8*i, &tmp2, 8);
	}
	return len;
}
#endif
#if defined(CONFIG_USER_RTK_VOIP) || defined(SUPPORT_CLOUD_VR_SERVICE) || defined(CONFIG_CMCC) || defined(CONFIG_CU)
#ifdef _PRMT_X_CT_COM_DHCP_
#define DEBUG_CHN_TEL(format, ...)
//#define DEBUG_CHN_TEL  printf
#ifdef CONFIG_USER_BOA_WITH_SSL
#include <openssl/md5.h>
#if defined(CONFIG_USER_RTK_VOIP)
#include "web_voip.h"
#include <voip_flash.h>
#endif

void output_hex(const char *prefix, const unsigned char *data, size_t size)
{
	int i;

	DEBUG_CHN_TEL("%s", prefix);
	for(i=0 ; i < size ; i++)
	{
		DEBUG_CHN_TEL("%02X ", data[i]);
	}
	DEBUG_CHN_TEL("\n");
}

/** Utilities to encrypt VoIP username/password **/
/** For DHCP option 60 & DHCPv6 option 16 **/
void get_key(const unsigned char *in, const unsigned char *R, const unsigned char *TS, unsigned char *out)
{
	unsigned char tmp[80];
	unsigned char *cur = tmp;

	memcpy(cur, R, 8);
	cur += 8;

	memcpy(cur, in, strlen(in));
	cur += strlen(in);

	memcpy(cur, TS, 8);
	cur += 8;

	MD5(tmp, strlen(in)+16, out);
}

static int get_encrypt_text(char *user, char *R, char *TS, char *buf)
{
	char des3_key[24] = {0};
	int user_len = strlen(user);
	int i;
	char *plain_text;
	int plain_len;
	unsigned char pkcs7;


	output_hex("R = ", R, 8);
	output_hex("TS = ", TS, 8);

	// key for 3DES
	memcpy(des3_key, R, 8);
	memcpy(des3_key + 8, TS, 8);
	//output_hex("3des_key = ", des3_key, 24);

	// Prepare plain text for 3DES
	pkcs7 = (user_len % 8 == 0) ? 8 : 8 - (user_len % 8);
	plain_len = user_len + pkcs7;
	plain_text = malloc(plain_len);
	strcpy(plain_text, user);
	for( i = 0 ; i < pkcs7 ; i++)
		*(plain_text + user_len + i) = pkcs7;

	//output_hex("plain_text = ", plain_text, plain_len);

	//clear output buffer
	memset(buf, 0, plain_len);

	do_3des(des3_key, plain_text, buf, plain_len, DES_ENCRYPT);

	output_hex("encrypted text = ", buf, plain_len);
	free(plain_text);

	return plain_len;
}

/*----------------------------------------------------------------------------
 * Name:
 *      get_ctc_voip_encrypted_data
 * Descriptions:
 *      Generate DHCP option 60 or DHCPv6 option 16 field type 34 values.
 * return:
 *      Length of encrypted data.
 *---------------------------------------------------------------------------*/
static int get_ctc_voip_encrypted_data(char *out, int len, char *username, char *password)
{
	char buf[100];
	char *cur = out;
	int buf_len;
	char serial[64] = {0};
	char oui[32] = {0};

	//disable this code block and enable above test data for testing
	char R[8] = {0};	//8 bytes random number
	char TS[8] = {0};	//8 bytes time stamp with leading zero
	long temp;

	temp = time(NULL);
	memcpy(TS, &temp, 4);

	srandom(temp);
	temp = random();
	memcpy(R, &temp, 4);
	temp = random();
	memcpy(R+4, &temp, 4);

	mib_get_s(MIB_HW_SERIAL_NUMBER, serial, sizeof(serial));
	mib_set(RTK_DEVID_OUI, "");
	getOUIfromMAC(oui);

	DEBUG_CHN_TEL("Username: %s\n", username);
	DEBUG_CHN_TEL("Password: %s\n", password);
	DEBUG_CHN_TEL("OUI: %s\n", oui);
	DEBUG_CHN_TEL("Serial Number: %s\n", serial);

#ifdef CONFIG_CMCC
	char province[32]={0};
	mib_get_s(MIB_HW_PROVINCE_NAME, province, sizeof(province));
	if(strncmp(province, "ZJ", 2) != 0) 	//no oui for province ZJ
#endif
	{
		buf_len = strlen(oui);
		memcpy(cur, oui, buf_len);
		cur += buf_len;

		memcpy(cur, "-", 1);
		cur += 1;

		buf_len = strlen(serial);
		memcpy(cur, serial, buf_len);
		cur += buf_len;
	}

	if(strlen(username) == 0 || strlen(password) == 0)
	{
		printf("Use default: admin/admin\n");
		strcpy(cur, "admin/admin");
		cur += strlen("admin/admin");
		return (cur-out);
	}

	// First byte is encryption type;
	*cur = 1;
	cur += 1;

	// concat random number
	memcpy(cur, R, 8);
	cur += 8;

	// concat time stamp
	memcpy(cur, TS, 8);
	cur += 8;

	// length of MD5 result is 16
	get_key(password, R, TS, buf);
	output_hex("Key = ", buf, 16);
	memcpy(cur, buf, 16);
	cur += 16;

	buf_len = get_encrypt_text(username, R, TS, buf);
	memcpy(cur, buf, buf_len);
	cur += buf_len;

	output_hex("Result: ", out, cur - out);

	return (cur - out);
}

static int get_cmcc_encrypted_data(char *out, int len, unsigned char field_type, char *username, char *password)
{
	char buf[100];
	char *cur = out;
	int buf_len;

	//disable this code block and enable above test data for testing
	char R[8] = {0};	//8 bytes random number
	char TS[8] = {0};	//8 bytes time stamp with leading zero
	long temp;

	temp = time(NULL);
	memcpy(TS, &temp, 4);

	srandom(temp);
	temp = random();
	memcpy(R, &temp, 4);
	temp = random();
	memcpy(R+4, &temp, 4);

	DEBUG_CHN_TEL("Username: %s\n", username);
	DEBUG_CHN_TEL("Password: %s\n", password);
	
	if(strlen(username) == 0 || strlen(password) == 0)
	{
		printf("Use default: admin/admin\n");
		strcpy(cur, "admin/admin");
		cur += strlen("admin/admin");
		return (cur-out);
	}

	// First byte is encryption type;
	*cur = 1;
	cur += 1;

	// concat random number
	memcpy(cur, R, 8);
	cur += 8;

	// concat time stamp
	memcpy(cur, TS, 8);
	cur += 8;

	// length of MD5 result is 16
	get_key(password, R, TS, buf);
	output_hex("Key = ", buf, 16);
	memcpy(cur, buf, 16);
	cur += 16;

	buf_len = get_encrypt_text(username, R, TS, buf);
	memcpy(cur, buf, buf_len);
	cur += buf_len;

	output_hex("Result: ", out, cur - out);

	return (cur - out);
}


int get_cu_encrypted_data(char *out, int len, char *username, char *password)
{
	char buf[100];
	char *cur = out;
	int buf_len;
	char serial[64] = {0};
	char oui[32] = {0};

	//disable this code block and enable above test data for testing
	char R[8] = {0};	//8 bytes random number
	char TS[8] = {0};	//8 bytes time stamp with leading zero
	long temp;

	temp = time(NULL);
	memcpy(TS, &temp, 4);

	srandom(temp);
	temp = random();
	memcpy(R, &temp, 4);
	temp = random();
	memcpy(R+4, &temp, 4);

#if 0
	mib_get_s(MIB_HW_SERIAL_NUMBER, serial, sizeof(serial));
	mib_set(RTK_DEVID_OUI, "");
	getOUIfromMAC(oui);

	DEBUG_CHN_TEL("Username: %s\n", username);
	DEBUG_CHN_TEL("Password: %s\n", password);
	DEBUG_CHN_TEL("OUI: %s\n", oui);
	DEBUG_CHN_TEL("Serial Number: %s\n", serial);

	buf_len = strlen(oui);
	memcpy(cur, oui, buf_len);
	cur += buf_len;

	memcpy(cur, "-", 1);
	cur += 1;

	buf_len = strlen(serial);
	memcpy(cur, serial, buf_len);
	cur += buf_len;
#endif

	if(strlen(username) == 0 || strlen(password) == 0)
	{
		printf("Use default: admin/admin\n");
		strcpy(cur, "admin/admin");
		cur += strlen("admin/admin");
		return (cur-out);
	}

	// First byte is encryption type;
	*cur = 1;
	cur += 1;

	// concat random number
	memcpy(cur, R, 8);
	cur += 8;

	// concat time stamp
	memcpy(cur, TS, 8);
	cur += 8;

	// length of MD5 result is 16
	get_key(password, R, TS, buf);
	output_hex("Key = ", buf, 16);
	memcpy(cur, buf, 16);
	cur += 16;

	buf_len = get_encrypt_text(username, R, TS, buf);
	memcpy(cur, buf, buf_len);
	cur += buf_len;

	output_hex("Result: ", out, cur - out);

	return (cur - out);
}
static void get_3des(const unsigned char *key, const unsigned char *in, unsigned char *out, size_t len)
{
	DES_key_schedule ks1, ks2, ks3;
	DES_cblock block_key;
	DES_cblock tmp1, tmp2;
	int cnt, i;

	memcpy(&block_key, key, 8);
	DES_set_key_unchecked(&block_key, &ks1);

	memcpy(&block_key, key + 8, 8);
    DES_set_key_unchecked(&block_key, &ks2);

	memcpy(&block_key, key + 16, 8);
    DES_set_key_unchecked(&block_key, &ks3);

	cnt = len / 8;

	for(i = 0; i < cnt ; i++)
	{
		memcpy(tmp1, in + 8*i, 8);
		DES_ecb3_encrypt(&tmp1, &tmp2, &ks1, &ks2, &ks3, DES_DECRYPT);
		memcpy(out + 8*i, &tmp2, 8);
	}
}

int get_decrypt_text(char *user, char *R, char *TS, char *buf,int type_des)
{
	char des3_key[24] = {0};
	int user_len = 16;
	int i;
	char *plain_text;
	int plain_len;
	unsigned char pkcs7;

	output_hex("R = ", R, 8);
	output_hex("TS = ", TS, 8);

	// key for 3DES
	memcpy(des3_key, R, 8);
	memcpy(des3_key + 8, TS, 8);

    plain_len = user_len;
	plain_text = malloc(plain_len);
	memcpy(plain_text, user,16);

	output_hex("plain_text = ", plain_text, plain_len);

	//clear output buffer
	memset(buf, 0, plain_len);

    get_3des(des3_key, plain_text, buf, plain_len);

	output_hex("encrypted text = ", buf, plain_len);
	free(plain_text);

	return plain_len;
}
#endif //CONFIG_USER_BOA_WITH_SSL
#endif //_PRMT_X_CT_COM_DHCP_
#endif //CONFIG_USER_RTK_VOIP


#ifdef _PRMT_X_CT_COM_DHCP_
int gen_ctcom_dhcp_opt(unsigned char type, char *output, int out_len, char *account, char *passwd)
{
	int len;
	char buf[256] = {0};

	if(output == NULL)
		return 0;

	switch(type)
	{
	case 1:
		mib_get_s(MIB_HW_CWMP_MANUFACTURER, (void *)output, out_len);
		return strlen(output);
	case 2:
		strncpy(output, "HGW", out_len);
		return strlen(output);
	case 3:
		mib_get_s(MIB_HW_CWMP_PRODUCTCLASS, (void *)output, out_len);
		return strlen(output);
	case 4:
		{
			char hw[64] = {0}, sw[64] = {0};
			mib_get_s(MIB_HW_HWVER, hw, sizeof(hw));
			getSYS2Str(SYS_FWVERSION, sw);
			snprintf(output, out_len, "%s/%s", hw, sw);
		return strlen(output);
		}
	case 31:
		return get_ctc_voip_encrypted_data(output, out_len, account, passwd);
	case 32:
		strncpy(output, "CTCDHCP0001", out_len);
		return strlen(output);
		break;
#if defined(CONFIG_USER_RTK_VOIP) && defined(CONFIG_USER_BOA_WITH_SSL)
	case 34:
	{
		char username[DNS_LEN] = "";
		char password[DNS_LEN] = "";
		voipCfgParam_t * pCfg;
		voipCfgPortParam_t *VoIPport;

		if(voip_flash_get( &pCfg) == 0)
		{
			unsigned char type = 0;
			char *login_id = pCfg->ports[0].proxies[0].login_id;

			mib_get(PROVINCE_DHCP_OPT60_TYPE, &type);

			if(type == DHCP_OPT60_TYPE_JSU && strchr(login_id, '@') == NULL)
			{
				DEBUG_CHN_TEL("Appdneding hostname %s\n", pCfg->ports[0].proxies[0].addr);
				snprintf(username, sizeof(username), "%s@%s", login_id, pCfg->ports[0].proxies[0].addr);
			}
			else
				strcpy(username, login_id);

			strcpy(password, pCfg->ports[0].proxies[0].password);
		}
#ifdef CONFIG_CMCC
		
		char province[32]={0};
		mib_get_s(MIB_HW_PROVINCE_NAME, province, sizeof(province));
		if(!strncmp(province, "JS", 2))		
			return get_ctc_voip_encrypted_data(output, out_len, account, passwd);
		else
#endif
		return get_ctc_voip_encrypted_data(output, out_len, username, password);
	}
#endif
#if defined(SUPPORT_CLOUD_VR_SERVICE) && defined(CONFIG_USER_BOA_WITH_SSL)
	case 35:
	{
		char userid[256] = {0};
		char password[256] = {0};

		mib_get(MIB_CLOUDVR_USERID, userid);
		mib_get(MIB_CLOUDVR_PASSWORD, password);

		return get_ctc_voip_encrypted_data(output, out_len, userid, password);
	}
#endif
#if 0 //def CONFIG_SUPPORT_HQOS_APPLICATIONTYPE
	case HQOS_OPTION60_FIELD_TYPE:
		return get_cu_encrypted_data(output, out_len, account, passwd);
#endif
	default:
		return -1;
	}
}

int gen_cmcc_dhcp_opt(unsigned char type, char *output, int out_len, char *account, char *passwd)
{
	int len;
	char buf[256] = {0};

	if(output == NULL)
		return 0;

	switch(type)
	{
	case 31:
		return get_cmcc_encrypted_data(output, out_len, type, account, passwd);
	default:
		return -1;
	}
}

int gen_cu_dhcp_opt(unsigned char type, char *output, int out_len, char *account, char *passwd)
{
	if(output == NULL)
		return 0;

	switch(type)
	{
#ifdef CONFIG_SUPPORT_HQOS_APPLICATIONTYPE
	case HQOS_OPTION60_FIELD_TYPE:
		return get_cu_encrypted_data(output, out_len, account, passwd);
#endif
	default:
		return -1;
	}
}
#endif //_PRMT_X_CT_COM_DHCP_


#ifdef _PRMT_X_CT_COM_PORTALMNT_
static const char *avoid_force_portal_list[]=
{
	"detectportal.firefox.com",
	"clients1.google.com",
	"login.live.com",
	"ocsp.digicert.com",
	"ocsp2.globalsign.com",
	NULL
};
void setPortalMNT(void)
{
	char tmpstr[MAX_URL_LEN];
	char urlstr[MAX_URL_LEN + 16];
	unsigned char vChar;
	FILE *fp;

	mib_get_s(CWMP_CT_PM_ENABLE, &vChar, sizeof(vChar));
	fp = fopen(TR069_FILE_PORTALMNT, "w");
	if(fp)
	{
		if (vChar == 1)
			fwrite("on", 2, 1, fp);
		else{
			fwrite("off", 3, 1, fp);
		}
		fclose(fp);
	}

	fp = fopen(TR069_FILE_PORTALMNT, "w");
	if(fp)
	{
		mib_get_s(CWMP_CT_PM_URL4PC, tmpstr, sizeof(tmpstr));
		if (tmpstr[0]) {
			sprintf(urlstr, "PC_addr:%s", tmpstr);
			fwrite(urlstr, strlen(urlstr), 1, fp);
		}

		mib_get_s(CWMP_CT_PM_URL4STB, tmpstr, sizeof(tmpstr));
		if (tmpstr[0]) {
			sprintf(urlstr, "STB_addr:%s", tmpstr);
			fwrite(urlstr, strlen(urlstr), 1, fp);
		}

		mib_get_s(CWMP_CT_PM_URL4MOBILE, tmpstr, sizeof(tmpstr));
		if (tmpstr[0]) {
			sprintf(urlstr, "MOB_addr:%s", tmpstr);
			fwrite(urlstr, strlen(urlstr), 1, fp);
		}

		fclose(fp);
	}

}
#endif
#if defined(_PRMT_X_CMCC_IPOEDIAGNOSTICS_) || defined(_PRMT_X_CT_COM_IPoEDiagnostics_)
static int ipoe_diag_is_ip_got()
{
	FILE *fp = fopen(IPOE_DIAG_RESULT_DHCPC_FILE, "r");
	char line[256] = {0};
	int ret = 0;

	if (fp == NULL)
		return 0;

	if (fgets(line, sizeof(line), fp) == NULL || strstr(line, "OK") == NULL)
		ret = 0;
	else
		ret = 1;

	fclose(fp);
	return ret;
}

static int ipoe_diag_do_ping(char *ifname, char *ping_host, unsigned int repitation, unsigned int timeout)
{
	char cmdstr[256] = {0};
	int length = 0;

	if (ifname == NULL || ping_host == NULL)
		return -1;

	sprintf(cmdstr, "/bin/ping -4 -I %s", ifname);

	if (repitation){
		length = sizeof(cmdstr)-strlen(cmdstr);
		snprintf(cmdstr+strlen(cmdstr),length, " -c %d", repitation);
	}

	if (timeout > 1000){
		length = sizeof(cmdstr)-strlen(cmdstr);
		snprintf(cmdstr+strlen(cmdstr),length, " -W %d", timeout/1000);
	}
	length = sizeof(cmdstr)-strlen(cmdstr);
	snprintf(cmdstr+strlen(cmdstr), length," %s", ping_host);

	strncat(cmdstr, " > "IPOE_DIAG_RESULT_PING_FILE" 2>&1", sizeof(cmdstr)-strlen(cmdstr)-1);

	fprintf(stderr, "<%s:%d> ping cmdstr=%s\n", __FUNCTION__, __LINE__, cmdstr);
	system(cmdstr);

	return 0;
}

int ipoeSimulationStart(int ifIndex, unsigned char *mac, unsigned char *VendorClassID, char *ping_host, unsigned int repitation, unsigned int timeout)
{
	int i = 0, totalNum = 0;
	MIB_CE_ATM_VC_T Entry;
	MIB_CE_ATM_VC_T simuEntry;
	unsigned char ipt = 0;
	char ifname[16] = {0}, ifname_simu[16] = {0};
	char buff[256] = {0};
	struct sysinfo info;
	int simStartTime = 0;
	unsigned char zero[MAC_ADDR_LEN] = {0};
	int pid = -1;
	FILE  *fp = NULL;
	unsigned int mark=0, mask=0;

	fprintf(stderr, "[IPDiag] %s\n", __func__);
	if (mac == NULL || ping_host == NULL)
		return -1;

	totalNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < totalNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -2;

		if (Entry.enable == 0)
			continue;

		if (Entry.ifIndex == ifIndex)
			break;
	}

	if (i == totalNum)
	{
		fprintf(stderr, "[IPDiag] <%s:%d> ifIndex(%x) not found in ATM_VC_TBL\n", __func__, __LINE__, ifIndex);
		return -3;
	}

	totalNum = mib_chain_total(MIB_SIMU_ATM_VC_TBL);
	for (i = 0; i < totalNum; i++)
	{
		if (!mib_chain_get(MIB_SIMU_ATM_VC_TBL, i, (void *)&simuEntry))
			return -2;

		if (simuEntry.ifIndex == ifIndex)
			break;
	}

	memcpy(&simuEntry, &Entry, sizeof(MIB_CE_ATM_VC_T));
	simuEntry.applicationtype = X_CT_SRV_INTERNET;
	simuEntry.cmode = CHANNEL_MODE_IPOE;
#ifdef CONFIG_IPV6
	simuEntry.IpProtocol = IPVER_IPV4; //force IPv4 only
#endif
	simuEntry.mbs = 1;
	simuEntry.ipDhcp = DHCP_CLIENT;
#if defined(ITF_GROUP) || defined(NEW_PORTMAPPING)
	simuEntry.itfGroup = 0;
#endif

	if (memcmp(mac, zero, MAC_ADDR_LEN) != 0)
	{
		memcpy(simuEntry.MacAddr, mac, MAC_ADDR_LEN);
	}
#if defined(CONFIG_LUNA) && defined(GEN_WAN_MAC)
	else
	{
		unsigned char macaddr[MAC_ADDR_LEN] = {0};
		mib_get(MIB_ELAN_MAC_ADDR, (void *)macaddr);
		setup_mac_addr(macaddr, WAN_HW_ETHER_START_BASE + ETH_INDEX(simuEntry.ifIndex + ITF_SOURCE_ROUTE_SIMU_START));
		memcpy(simuEntry.MacAddr, macaddr, MAC_ADDR_LEN);
	}
#endif

	if (VendorClassID && strlen(VendorClassID))
	{
		simuEntry.dhcp_opt60_enable[0] = 1;
		simuEntry.dhcp_opt60_value_mode[0] = 0;
		simuEntry.dhcp_opt60_type[0] = 60;
		memcpy(simuEntry.dhcp_opt60_value[0], VendorClassID, strlen(VendorClassID));
	}

	if (i != totalNum)
		mib_chain_update(MIB_SIMU_ATM_VC_TBL, &simuEntry, i);
	else
		mib_chain_add(MIB_SIMU_ATM_VC_TBL, &simuEntry);

	//generate Simulation WAN
	ifGetName(PHY_INTF(ifIndex), ifname, 16);
	snprintf(ifname_simu, 16,  "%s_0", ifname);
	fprintf(stderr, "[%s@%d] ifname_simu = %s \n", __FUNCTION__, __LINE__, ifname_simu);

	rtk_net_add_wan_policy_route_mark(ifname_simu, &mark, &mask);
#ifdef CONFIG_SUPPORT_AUTO_DIAG
#ifdef CONFIG_RTL_MULTI_ETH_WAN
	addSimuEthWANdev(&simuEntry, 0);
#endif
	setup_simu_ethernet_config(&simuEntry, ifname_simu);
#endif
	unlink(IPOE_DIAG_RESULT_DHCPC_FILE);

	//OK, simulation dial.
	startDhcpc(ifname_simu, &simuEntry, 1);

	while(access( IPOE_DIAG_RESULT_DHCPC_FILE, F_OK ) != 0)
		sleep(1);

	printf("IPoE diagnostics result got\n");

	if (ipoe_diag_is_ip_got())
		ipoe_diag_do_ping(ifname_simu, ping_host, repitation, timeout);

	//Clean
	snprintf(buff, sizeof(buff), "%s.%s", (char*)DHCPC_PID, ifname_simu);
	pid = read_pid(buff);
	if ( pid > 0)
	{
		printf("Send signal TERM to pid %d to kill thread.\n",pid);
		kill(pid, SIGTERM);
	}

	set_ipv4_policy_route(&simuEntry, 3);
	rtk_net_remove_wan_policy_route_mark(ifname_simu);

#if defined(CONFIG_RTL_SMUX_DEV)
	va_cmd("/bin/smuxctl", 2, 1, "--if-delete", ifname_simu);
#else
	va_cmd("/bin/ethctl", 4, 1, "remsmux", "ipoe", ALIASNAME_NAS0, ifname_simu);
#endif

	mib_chain_delete(MIB_SIMU_ATM_VC_TBL, i);

	return 0;
}
#endif

#ifdef CONFIG_CTC_E8_CLIENT_LIMIT
int rtk_mwband_set()
{
#ifdef CONFIG_COMMON_RT_API
	rtk_fc_accesswanlimit_set();
#else
	#error "rtk_mwband_set() Only support RT API"
#endif
	return 0;
}
#endif //CONFIG_CTC_E8_CLIENT_LIMIT


#ifdef _PRMT_X_CT_COM_ALARM_MONITOR_
int init_alarm_numbers()
{
	int total = mib_chain_total(CWMP_CT_ALARM_TBL);
	CWMP_CT_ALARM_T alarm;
	int i;

	for(i = total - 1 ; i >= 0 ; i--)
	{
		mib_chain_get(CWMP_CT_ALARM_TBL, i, &alarm);
		if(alarm.mode == ALARM_MODE_NONE && alarm.alarmNumber != CTCOM_ALARM_DEV_RESTART
			&& alarm.alarmNumber != CTCOM_ALARM_SW_UPGRADE_FAIL)
			mib_chain_delete(CWMP_CT_ALARM_TBL, i);
	}

	return i;
}

int find_alarm_num(unsigned int alarm_num)
{
	int total = mib_chain_total(CWMP_CT_ALARM_TBL);
	CWMP_CT_ALARM_T alarm;
	int i;

	for(i = 0 ; i < total ; i++)
	{
		mib_chain_get(CWMP_CT_ALARM_TBL, i, &alarm);
		if(alarm.alarmNumber == alarm_num)
			break;
	}

	if(i >= total)
		i = -1;

	return i;
}

int get_alarm_by_alarm_num(unsigned int alarm_num, CWMP_CT_ALARM_T *entry, int *idx)
{
	int total = mib_chain_total(CWMP_CT_ALARM_TBL);
	int i;

	if(entry == NULL || idx == NULL)
		return -1;
	
	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(CWMP_CT_ALARM_TBL, i, entry) == 0)
			continue;
	
		if(entry->alarmNumber == alarm_num)
		{
			*idx = i;
			return 0;
		}
	}

	*idx = -1;
	return -1;
}

int update_alarm_event_time(unsigned int alarm_num)
{
	int total = mib_chain_total(CWMP_CT_ALARM_TBL);
	CWMP_CT_ALARM_T alarm;
	int i = 0, ret = 0;

	for(i = 0 ; i < total ; i++)
	{
		mib_chain_get(CWMP_CT_ALARM_TBL, i, &alarm);
		if(alarm.alarmNumber == alarm_num)
		{
			alarm.EventTime = time(NULL);
			mib_chain_update(CWMP_CT_ALARM_TBL,&alarm,i);
			mib_update(CURRENT_SETTING, CONFIG_MIB_CHAIN);
			//printf("[%s:%d] update alarm_num = %d, event time = %d\n",__FUNCTION__,__LINE__,alarm_num,alarm.EventTime);
			ret = 1;
			break;
		}
	}

	return ret;
}

int set_ctcom_alarm(unsigned int alarm_num)
{
	int cwmp_events;
	unsigned char enable = 0;
#ifdef AUTO_SAVE_ALARM_CODE
    int def_code_no_info[] = 
    {
    104012,104030,104032,104050,104051,104052,104053,104054,104057,104142,104143,0
    };

    int total = sizeof(def_code_no_info)/sizeof(int);
    int i = 0;
    for(i = 0; i < total; i++)
    {
        if(alarm_num == def_code_no_info[i])
            auto_save_alarm_code_add(alarm_num, "");
    }
#endif
	mib_get_s(CWMP_CT_ALARM_ENABLE, &enable, sizeof(enable));

	if(find_alarm_num(alarm_num) == -1)
	{
		CWMP_CT_ALARM_T alarm = {0};

		alarm.alarmNumber = alarm_num;
		alarm.mode = ALARM_MODE_NONE;
		alarm.state = STATE_NOTIFY;
		alarm.EventTime = time(NULL);

		//printf("[%s:%d] add alarm_num = %d, event time = %d\n",__FUNCTION__,__LINE__,alarm_num,alarm.EventTime);
		mib_chain_add(CWMP_CT_ALARM_TBL, (void *)&alarm);
		mib_update(CURRENT_SETTING, CONFIG_MIB_CHAIN);

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
		if(enable && (alarm_num < 104050 || alarm_num >= 104070))
		{
			mib_get_s(CWMP_INFORM_USER_EVENTCODE, &cwmp_events, sizeof(cwmp_events));
			cwmp_events |=	EC_X_CT_COM_ALARM;
			mib_set(CWMP_INFORM_USER_EVENTCODE, &cwmp_events);
		}
#endif
	}
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	else if(enable && alarm_num == CTCOM_ALARM_INET_REDIAL)
	{
		mib_get_s(CWMP_INFORM_USER_EVENTCODE, &cwmp_events, sizeof(cwmp_events));
		cwmp_events |=	EC_X_CT_COM_ALARM;
		mib_set(CWMP_INFORM_USER_EVENTCODE, &cwmp_events);
	}
#endif
	else
		update_alarm_event_time(alarm_num);

	return 0;
}

int clear_ctcom_alarm(unsigned int alarm_num)
{
	CWMP_CT_ALARM_T alarm;
	int idx;

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	int cwmp_events;
	unsigned char enable = 0;
#endif

	if(get_alarm_by_alarm_num(alarm_num, &alarm, &idx) != 0)
		return -1;

	if (alarm.mode == ALARM_MODE_NONE && alarm.state == STATE_NOTIFY)
	{
		mib_chain_delete(CWMP_CT_ALARM_TBL, idx);
	}
	else
	{
		alarm.state = STATE_CLEAR;
		mib_chain_update(CWMP_CT_ALARM_TBL, &alarm, idx);

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
		mib_get_s(CWMP_CT_ALARM_ENABLE, &enable, sizeof(enable));
		if(enable && (alarm_num < 104050 || alarm_num >= 104070))
		{
			mib_get_s(CWMP_INFORM_USER_EVENTCODE, &cwmp_events, sizeof(cwmp_events));
			cwmp_events |=  EC_X_CT_COM_CLEARALARM;
			mib_set(CWMP_INFORM_USER_EVENTCODE, &cwmp_events);
		}
#endif
	}
	return 0;
}
#endif //_PRMT_X_CT_COM_ALARM_MONITOR_

#if defined(CONFIG_SUPPORT_AUTO_DIAG)||defined(_PRMT_X_CT_COM_IPoEDiagnostics_)
#define MAX_CH 8
int omcistate = 0;
int simu_debug = 1;
struct callout_s autoSimulation_ch[MAX_CH];
enum PPP_AUTODIAG_RESULT {
	DIAG_INPROCESS, 					// 0
	DIAG_TIMEOUT,						// 1
	DIAG_PARAMNEGOFAIL,					// 2
	DIAG_USERAUTHENTICATIONFAIL,		// 3
	DIAG_PARAMNEGOFAIL_IPV4,			// 4
	DIAG_PARAMNEGOFAIL_IPV6,			// 5
	DIAG_USERSTOP,						// 6
	DIAG_UNKNOWN,						// 7
	DIAG_SERVER_OUT_OF_RESOURCE_IPV6,	// 8
	DIAG_SUCCESS 						// 9
};
const char* resultArray[] =
{
	"inProcess",		//reserved.
	"Timeout",							// 1
	"ParamNegoFail",					// 2
	"UserAuthenticationFail",			// 3
	"ParamNegoFail_IPv4",				// 4
	"ParamNegoFail_IPv6",				// 5
	"UserStop",							// 6
	"unKnown",							// 7
	"ERROR_SERVER_OUT_OF_RESOURCES_IPv6",		// 8
	"Success"							// 9
};

#ifdef CONFIG_RTL_MULTI_ETH_WAN
void addSimuEthWANdev(MIB_CE_ATM_VC_Tp pEntry, int autosimu)
{
	MEDIA_TYPE_T mType;
	char ifname[IFNAMSIZ] = {0}, ifname_orig[IFNAMSIZ] = {0};
	int flag=0, wait_count = 0;

	ifGetName(PHY_INTF(pEntry->ifIndex), ifname_orig, sizeof(ifname_orig));
	snprintf(ifname, IFNAMSIZ,  "%s_%d", ifname_orig, autosimu);
	mType = MEDIA_INDEX(pEntry->ifIndex);

	if(((mType == MEDIA_ETH) && (WAN_MODE & MODE_Ethernet)))
	{
#if defined(CONFIG_RTL_SMUX_DEV)
		int i = 0;
		char cmd_str[256] = {0}, mac_str[16] = {0};

		/* create simulation interface */
		sprintf(cmd_str, "/bin/smuxctl --if-create-name %s %s", ifname_orig, ifname);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_str);

		/* set rx rules for simulation interface */
		sprintf(mac_str, "%02x%02x%02x%02x%02x%02x", pEntry->MacAddr[0]&0xff, pEntry->MacAddr[1]&0xff, pEntry->MacAddr[2]&0xff, pEntry->MacAddr[3]&0xff, pEntry->MacAddr[4]&0xff, pEntry->MacAddr[5]&0xff);

		for (i = 0; i <= 2; i++)
		{
			if (pEntry->vlan && pEntry->vid)
			{
				if (i == 0) continue;
			}
			else
			{
				if (i != 0) continue;
			}

			sprintf(cmd_str, "/bin/smuxctl --if %s --rx --tags %d --filter-dmac %s --set-rxif %s  --target ACCEPT --rule-append", ifname_orig, i, mac_str, ifname);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);

			sprintf(cmd_str, "/bin/smuxctl --if %s --rx --tags %d --filter-dmac ffffffffffff --duplicate-forward --set-rxif %s  --target ACCEPT --rule-append", ifname_orig, i, ifname);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
		}
#else
		int tmp_group;
		char cmd_str[100];
		//const char smux_brg_cmd[]="/bin/ethctl addsmux bridge %s %s";
		//const char smux_pppoe_cmd[]="/bin/ethctl addsmux pppoe %s %s";
		const char smux_ipoe_cmd[]="/bin/ethctl addsmux ipoe %s %s";

		/*
		if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
			snprintf(cmd_str, 100, smux_brg_cmd, rootdev, ifname);
		else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_IPOE || (CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_6RD)
			snprintf(cmd_str, 100, smux_ipoe_cmd, rootdev, ifname);
		else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
			snprintf(cmd_str, 100, smux_pppoe_cmd, rootdev, ifname);
		*/

		snprintf(cmd_str, 100, smux_ipoe_cmd, ALIASNAME_NAS0, ifname);
		if (pEntry->napt)
			strncat(cmd_str, " napt", 100);

		if (pEntry->vlan)
		{
			unsigned int vlantag;
			vlantag = (pEntry->vid|((pEntry->vprio) << 13));
			snprintf(cmd_str, 100, "%s vlan %d", cmd_str, vlantag );
		}
		if(simu_debug)
			printf("TRACE: %s\n", cmd_str);
		system(cmd_str);
#endif

		wait_count = 0;
		while(getInFlags(ifname, &flag)==0)
		{
			//wait the device created successly, Iulian Wu
			if(simu_debug)
				printf("Ethernet WAN not ready!\n");
			sleep(1);
			if (++wait_count > 5) break;
		}
	}
}
#endif

int setup_simu_ethernet_config(MIB_CE_ATM_VC_Tp pEntry, char *wanif)
{
	char *argv[8];
	int status=0;
	unsigned char devAddr[MAC_ADDR_LEN];
	char macaddr[MAC_ADDR_LEN*2+1];
	int wanPhyPort;

#if defined(CONFIG_LUNA) && defined(GEN_WAN_MAC)
	snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
		pEntry->MacAddr[0], pEntry->MacAddr[1], pEntry->MacAddr[2], pEntry->MacAddr[3], pEntry->MacAddr[4], pEntry->MacAddr[5]);

	argv[1]=wanif;
	argv[2]="hw";
	argv[3]="ether";
	argv[4]=macaddr;
	argv[5]=NULL;

	if(simu_debug)
		TRACE(STA_SCRIPT, "%s %s %s %s %s\n", IFCONFIG, argv[1], argv[2], argv[3], argv[4]);
	status|=do_cmd(IFCONFIG, argv, 1);
#endif

	char sysbuf[128] = {0};

	// ifconfig nas0_0_0 txqueuelen 10
	argv[1] = wanif;
	argv[2] = "txqueuelen";
	argv[3] = "10";
	argv[4] = NULL;
	if(simu_debug)
		TRACE(STA_SCRIPT, "%s %s %s %s\n", IFCONFIG, argv[1], argv[2], argv[3]);
	status|=do_cmd(IFCONFIG, argv, 1);
#ifdef CONFIG_IPV6
	// Disable ipv6 in bridge
	if (pEntry->cmode == CHANNEL_MODE_BRIDGE)
		setup_disable_ipv6(wanif, 1);
	else
	{
		char pppifname[16];
		char *pwanif=wanif;
		if(pEntry->cmode == CHANNEL_MODE_PPPOE)
		{
			snprintf(pppifname, sizeof(pppifname), "ppp%u", PPP_INDEX(pEntry->ifIndex));
			pwanif = pppifname;
		}
		// enable ipv6 if applicable
		if (pEntry->IpProtocol & IPVER_IPV6)
			setup_disable_ipv6(pwanif, 0);
		else
			setup_disable_ipv6(pwanif, 1);
	}
#endif
	// ifconfig vc0 up
	argv[2] = "up";
	argv[3] = NULL;
	if(simu_debug)
		TRACE(STA_SCRIPT, "%s %s %s\n", IFCONFIG, argv[1], argv[2]);
	status|=do_cmd(IFCONFIG, argv, 1);

#if defined(CONFIG_RTL_SMUX_DEV)
	sprintf(sysbuf, "echo \"carrier %s on\" > /proc/rtk_smuxdev/interface", wanif);
	system(sysbuf);
#else
	//ethctl setsmux nas0 nas0_0_7 carrier 1
	argv[1] = "setsmux";
	argv[2] = ALIASNAME_NAS0;
	argv[3] = wanif;
	argv[4] = "carrier";
	argv[5] = "1";
	argv[6] = NULL;
	status |= do_cmd("/bin/ethctl", argv, 1);
#endif
	sleep(1); // wait interface up

	return status;
}

#ifdef CONFIG_IPV6
/*
 *	generate the ifupv6_pppx script
 */
int generate_ifupv6_script_manual(char* wanif, MIB_CE_ATM_VC_Tp pEntry)
{
	FILE *fp;
	char ifup_path[32];
	int ret = 0;

	if(simu_debug)
		printf("[%s %d]wan=%s, cmode=%d, Ipv6Dhcp=%d, Ipv6DhcpRequest=%d\n", __func__, __LINE__, wanif, pEntry->cmode, pEntry->Ipv6Dhcp, pEntry->Ipv6DhcpRequest);

	snprintf(ifup_path, 32, "/var/ppp/ifupv6_ppp7");
	if (fp=fopen(ifup_path, "w+") )
	{
		unsigned char pidfile[64], leasefile[64];
		unsigned char value[256];

		fprintf(fp, "#!/bin/sh\n\n");
		if((CHANNEL_MODE_BRIDGE != pEntry->cmode)&&(pEntry->AddrMode == IPV6_WAN_DHCP))
		{
			//DHCPv6 only
			fprintf(fp, "/bin/echo 0 > /proc/sys/net/ipv6/conf/ppp7/autoconf\n");
		}
		else
		{
			// DHCPv6&SLAAC, or SLAAC only
			if(CHANNEL_MODE_BRIDGE != pEntry->cmode)
			{
			    fprintf(fp, "/bin/echo 1 > /proc/sys/net/ipv6/conf/ppp7/autoconf\n");
			}
		}
#ifdef DHCPV6_ISC_DHCP_4XX
		if ((1 == pEntry->Ipv6Dhcp) || (CHANNEL_MODE_BRIDGE == pEntry->cmode))
		{
			int len = 0;
			// Start DHCPv6 client
			// dhclient -6 -sf /var/dhclient-script -lf /var/dhclient6-leases -pf /var/run/dhclient6.pid ppp0 -d -q -N -P
			snprintf(leasefile, 64, "%s.%s", PATH_DHCLIENT6_LEASE, "ppp7");
			snprintf(pidfile, 64, "%s.%s", PATH_DHCLIENT6_PID, "ppp7");
			len = snprintf(value, sizeof(value), "/bin/dhclient -6 -sf /etc/simu_dhclient_script -lf %s -pf %s %s -d -q", leasefile, pidfile, "ppp7");	//
			// Request Address
			if ((pEntry->Ipv6DhcpRequest & M_DHCPv6_REQ_IANA) || (CHANNEL_MODE_BRIDGE == pEntry->cmode))
				len += snprintf(value+len, sizeof(value)-len, " -N");
			// Request Prefix
			if ((pEntry->Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD)
                ||(CHANNEL_MODE_BRIDGE == pEntry->cmode)
#ifdef DUAL_STACK_LITE
                ||((pEntry->dslite_enable == 1) && (pEntry->dslite_aftr_mode == IPV6_DSLITE_MODE_AUTO))
#endif
            )
            {
				len += snprintf(value+len, sizeof(value)-len, " -P");
            }

			// If dslite enabled, using dhcpv6 option 64 to request AFTR.
#if defined(DUAL_STACK_LITE)
			if((pEntry->dslite_enable == 1) && (pEntry->dslite_aftr_mode == IPV6_DSLITE_MODE_AUTO)){
				char fconf[64] = {0};
				FILE *fp_conf;

				sprintf(fconf, "/var/dhclient6_%s.conf", "ppp7");
				fp_conf = fopen(fconf, "w");
				if(fp_conf)
				{
					fprintf(fp_conf, "option dhcp6.dslite code 64 = domain-list; \n");
					fprintf(fp_conf, "also request dhcp6.dslite; \n");
					fclose(fp_conf);
				}
				len += snprintf(value+len, sizeof(value)-len, " -cf %s ", fconf);
			}
#endif
			len += snprintf(value+len, sizeof(value)-len, " &\n");
			fprintf(fp, value);
		}
#endif
		fclose(fp);
		if(chmod(ifup_path, 484) !=0)
			printf("chmod failed: %s %d\n", __func__, __LINE__);
	}

	return ret;
}

static int generate_ifupv6_script_get_PD(char* wanif, MIB_CE_ATM_VC_Tp pEntry)
{
	FILE *fp;
	char ifup_path[32];
	int ret = 0;

	if(simu_debug)
		printf("[%s %d]wan=%s, cmode=%d, Ipv6Dhcp=%d, Ipv6DhcpRequest=%d\n", __func__, __LINE__, wanif, pEntry->cmode, pEntry->Ipv6Dhcp, pEntry->Ipv6DhcpRequest);

	snprintf(ifup_path, 32, "/var/ppp/ifupv6_ppp7");
	if (fp=fopen(ifup_path, "w+") )
	{
		unsigned char pidfile[64], leasefile[64];
		unsigned char value[256];

		fprintf(fp, "#!/bin/sh\n\n");
#ifdef DHCPV6_ISC_DHCP_4XX
		if ((1 == pEntry->Ipv6Dhcp) || (CHANNEL_MODE_BRIDGE == pEntry->cmode))
		{
			// Start DHCPv6 client
			// dhclient -6 -sf /var/dhclient-script -lf /var/dhclient6-leases -pf /var/run/dhclient6.pid ppp0 -d -q -N -P
			snprintf(leasefile, 64, "%s.%s", PATH_DHCLIENT6_LEASE, "ppp7");
			snprintf(pidfile, 64, "%s.%s", PATH_DHCLIENT6_PID, "ppp7");
			snprintf(value, sizeof(value), "/bin/dhclient -6 -sf /etc/simu_dhclient_script -lf %s -pf %s %s -d -q", leasefile, pidfile, "ppp7");	//
			// Request Address
			if((1==pEntry->Ipv6Dhcp)||(CHANNEL_MODE_BRIDGE == pEntry->cmode))
			{
				FILE *f;
				char devname[IFNAMSIZ];
				unsigned int scope, flags;
				int mflag = 0;

				f = fopen("/proc/net/if_inet6", "r");
				if (f == NULL){
					fclose(fp);
					return 0;
				}

				while (fscanf(f, "%*32s %*08x %*02x %02x %02x %16s\n", &scope, &flags, devname) != EOF) {
					printf("[%s %d]scope=0x%02x, flags=0x%02x, devname=%s\n", __func__, __LINE__, scope, flags, devname);
					if ((!strcmp(devname, "ppp7"))&&(scope==0x0000U) &&((flags&0x40)!=0)) {
						mflag = 1;
						break;
					}
				}
				if(mflag==1)
				{
					strncat(value, " -N", sizeof(value)-strlen(value)-1);
				}
				fclose(f);
			}
			// Request Prefix
			if (pEntry->Ipv6DhcpRequest & M_DHCPv6_REQ_IAPD
                ||(CHANNEL_MODE_BRIDGE == pEntry->cmode)
#ifdef DUAL_STACK_LITE
                ||((pEntry->dslite_enable == 1) && (pEntry->dslite_aftr_mode == IPV6_DSLITE_MODE_AUTO))
#endif
                )
                {
				    strncat(value, " -P", sizeof(value) - strlen(value) -1);
                }

			// If dslite enabled, using dhcpv6 option 64 to request AFTR.
#if defined(DUAL_STACK_LITE)
			if((pEntry->dslite_enable == 1) && (pEntry->dslite_aftr_mode == IPV6_DSLITE_MODE_AUTO)){
				char fconf[64] = {0};
				FILE *fp;
				int len = 0;

				sprintf(fconf, "/var/dhclient6_%s.conf", "ppp7");
				fp = fopen(fconf, "w");
				if(fp)
				{
					fprintf(fp, "option dhcp6.dslite code 64 = domain-list; \n");
					fprintf(fp, "also request dhcp6.dslite; \n");
					fclose(fp);
				}
				len = strlen(value);
				snprintf(value+len, sizeof(value)-len, " -cf %s ", fconf);
			}
#endif
			strncat(value, " &\n", sizeof(value)-strlen(value) -1);

			fprintf(fp, value);
		}
#endif
		fclose(fp);
		if(chmod(ifup_path, 484) !=0)
			printf("chmod failed: %s %d\n", __func__, __LINE__);
	}

	return ret;
}
#endif

unsigned int getOptionFromleases(char* filename, char* iaprfix, char* dns, char* aftr)
{
	FILE* fp=NULL;
	char buff[256];
	char *p, *str, *dns1;
	char *saveptr;
	unsigned int tok = 0;

	if(!filename)
	{
		printf("[%s %d]filename==NULL\n", __func__, __LINE__);
		return 0;
	}
	fp = fopen(filename, "r");
	if(!fp)
	{
		printf("[%s %d]open(%s) failed\n", __func__, __LINE__, filename);
		return 0;
	}
	while(fgets(buff, 256, fp)!=NULL)
	{
		if((iaprfix!=NULL)&&((p=strstr(buff, "iaprefix"))!=NULL))
		{
			str = strtok_r(p, " ", &saveptr);
			if(str)
			{
				str = strtok_r(NULL, " ", &saveptr);
				if(str)
				{
					memcpy(iaprfix, str, 64);
					tok |= 1;
				}
			}
		}
		else if((dns!=NULL)&&((p=strstr(buff, "name-servers"))!=NULL))
		{
			p += 13;
			str = strtok_r(p, ";", &saveptr);
			dns1 = strtok_r(str, ",", &saveptr);	//in case two dns addr exist.
			memcpy(dns, dns1, 64);
			tok |= 2;
		}
		else if((aftr!=NULL)&&((p=strstr(buff, "dslite"))!=NULL))
		{
			str = strtok_r(p, "\"", &saveptr);
			if(str)
			{
				str = strtok_r(NULL, "\"", &saveptr);
				if(str)
				{
					str[strlen(str)-1] = '\0';
					memcpy(aftr, str, strlen(str));
					tok |= 4;
				}
			}
		}

		continue;
	}

	fclose(fp);
	return tok;
}

/*
Manual start the pppoe simulation.
username: dial ppp username, NULL if no username
	password: dial ppp password, NULL if no password
	ipv6AddrMode: 1 - SLAAC; 2 - DHCPv6

return :
	  0 : - start success
	-1 : - parameter invalid, null point.
	-2 : - mib operator failed
	-3 : - devname invalid, no such wan
*/
static char *ppp_auth_str[] = {"auto", "pap", "chap", "chapms-v1", "chapms-v2", "none"};
int pppoeSimulationStart(char* devname, char* username, char* password, PPP_AUTH_T auth, int ipv6AddrMode)
{
	int i;
	int totalNum;
	MIB_CE_ATM_VC_T Entry;
	MIB_CE_ATM_VC_T simuEntry;
	unsigned char ipt = 0;
	char ifname[IFNAMSIZ];
	char ifname_simu[IFNAMSIZ];
	char str_auth[16] = {0};
	char buff[256];
	struct data_to_pass_st msg;
	struct sysinfo info;
	int simStartTime = 0;
	int autosimu = 0;
	int len = 0;

	if(simu_debug)
		printf("[SIMU]pppoeSimulationStart\n");
	if(NULL == devname)
		return -1;

	//ipv6AddrMode = 0;
	totalNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				return -2;

		if(Entry.enable == 0)
			continue;

		memset(ifname, 0, IFNAMSIZ);
		ifGetName(PHY_INTF(Entry.ifIndex), ifname, sizeof(ifname));
		if(!strncmp(devname, ifname, IFNAMSIZ))
		{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
			ipt = 0; // IPv4 only
#else
			ipt = Entry.IpProtocol - 1;
#endif
			/*
			if((Entry.cmode != CHANNEL_MODE_BRIDGE)&&(ipt>0))
			{
				// 1 - SLAAC; 2 - DHCPv6
				ipv6AddrMode = (Entry.AddrMode&IPV6_WAN_AUTO)?1:2;
			}
			*/
			break;
		}
	}

	if(i == totalNum)
	{
		if(simu_debug)
			printf("[%s %d][SIMU]dev(%s) not found in ATM_VC_TBL\n", __func__, __LINE__, devname);
		return -3;
	}

	totalNum = mib_chain_total(MIB_SIMU_ATM_VC_TBL);
	for(i = 0; i < totalNum; i++)
	{
		if(!mib_chain_get(MIB_SIMU_ATM_VC_TBL, i, (void *)&simuEntry))
			return -2;

		memset(ifname, 0, IFNAMSIZ);
		ifGetName(PHY_INTF(simuEntry.ifIndex), ifname, sizeof(ifname));
		if((!strncmp(devname, ifname, IFNAMSIZ))&&(1 == simuEntry.mbs))
		{
			//duplicate...
			break;
		}
	}

	memcpy(&simuEntry, &Entry, sizeof(MIB_CE_ATM_VC_T));
	simuEntry.applicationtype = X_CT_SRV_INTERNET;
	simuEntry.cmode = CHANNEL_MODE_PPPOE;
	simuEntry.mbs = 1;
	simuEntry.MacAddr[MAC_ADDR_LEN-1] = (simuEntry.MacAddr[MAC_ADDR_LEN-1] + 8);
	simuEntry.MacAddr[MAC_ADDR_LEN-2]++;
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	simuEntry.IpProtocol = IPVER_IPV4; // IPv4 only
#endif
	//simuEntry.IpProtocol = Entry.IpProtocol;
#if defined(ITF_GROUP) || defined(NEW_PORTMAPPING)
    simuEntry.itfGroup = 0;     //simu connection, no need to bind with any lan dev
#endif
    simuEntry.brmode = BRIDGE_DISABLE;
    simuEntry.ifIndex &= ~(0xFF<<8);
    simuEntry.ifIndex |= (7<<8);
	simuEntry.pppAuth = auth;
	snprintf(simuEntry.pppUsername, sizeof(simuEntry.pppUsername), "%s", username);
	snprintf(simuEntry.pppPassword, sizeof(simuEntry.pppPassword), "%s", password);
	
	if(i != totalNum)
	{
		mib_chain_update(MIB_SIMU_ATM_VC_TBL, &simuEntry, i);
	}
	else
	{
		mib_chain_add(MIB_SIMU_ATM_VC_TBL, &simuEntry);
	}

	//generate Simulation WAN
	snprintf(ifname_simu, IFNAMSIZ,  "%s_%d", devname, autosimu);
#ifdef CONFIG_RTL_MULTI_ETH_WAN
	addSimuEthWANdev(&simuEntry, autosimu);
#endif
	setup_simu_ethernet_config(&simuEntry, ifname_simu);

	//OK, simulation dial.
	snprintf(buff, 256, "rm -rf /tmp/ppp_manual_diag.%s", ifname_simu);
	system(buff);

#ifdef CONFIG_USER_PPPD
#ifdef CONFIG_IPV6
	snprintf(buff, 256, "rm -rf /tmp/ppp_manual_diag_ipv6.%s", ifname_simu);
	system(buff);
#endif
	generate_pppd_init_script(&simuEntry);
	generate_pppd_ipup_script(&simuEntry);
	generate_pppd_ipdown_script(&simuEntry);
	write_to_simu_ppp_options(&simuEntry);
	connectPPPExt(ifname_simu, &simuEntry, CHANNEL_MODE_PPPOE, 1);
#endif
#ifdef CONFIG_USER_SPPPD
	snprintf(msg.data, BUF_SIZE, "spppctl add 7 simulation 1 ipt %d pppoe %s", ipt, ifname_simu);
	if(auth != PPP_AUTH_AUTO)
	{
		len = strlen(msg.data);
		snprintf(msg.data+len, BUF_SIZE-len, " auth %s", ppp_auth_str[auth]);
	}
	if(username!=NULL)
	{
		len = strlen(msg.data);
		snprintf(msg.data+len, BUF_SIZE-len, " username %s", username);
	}
	if(password!=NULL)
	{
		len = strlen(msg.data);
		snprintf(msg.data+len, BUF_SIZE-len, " password %s", password);
	}
#ifdef CONFIG_IPV6
	if(ipt>0)
	{
		generate_ifupv6_script_manual(ifname_simu, &Entry);
		setup_disable_ipv6("ppp7", 0);
	}
#endif
	//for debug
	//snprintf(msg.data, BUF_SIZE, "%s servicename linux247", msg.data);
	if(simu_debug)
		printf("[%s %d][SIMU]:%s\n", __func__, __LINE__, msg.data);
	//end

	write_to_pppd(&msg);
#endif //CONFIG_USER_SPPPD

	sysinfo(&info);
	simStartTime = (int)info.uptime;

	mib_set(MIB_AUTO_DIAG_STARTTIME, (void *)&simStartTime);
	return 0;
}

int stopPPPoESimulation(char *devname)
{
	int i, totalNum;
	int if_unit=-1, resultNum=-1;
	MIB_CE_ATM_VC_T simuEntry;
	unsigned char ipt = 0;
	char ifname[IFNAMSIZ];
	char ifname_simu[IFNAMSIZ];
	char filename[64];
	unsigned char buf[300];
	unsigned char strName[16];
	unsigned char strVal[256];
	FILE *fp=NULL;
	int autosimu = 0;
	unsigned int pid;

	if(simu_debug)
		printf("[SIMU]stopPPPoESimulation\n");
	if((NULL == devname))
		return -1;

	totalNum = mib_chain_total(MIB_SIMU_ATM_VC_TBL);
	for(i = 0; i < totalNum; i++)
	{
		if(!mib_chain_get(MIB_SIMU_ATM_VC_TBL, i, (void *)&simuEntry))
			return -2;

		memset(ifname, 0, IFNAMSIZ);
		ifGetName(PHY_INTF(simuEntry.ifIndex), ifname, sizeof(ifname));
		if((!strncmp(devname, ifname, IFNAMSIZ))&&(1 == simuEntry.mbs))
		{
			//exist
			ipt = simuEntry.IpProtocol - 1;
			break;
		}
	}

	if(i == totalNum)
	{
		if(simu_debug)
			printf("[SIMU]No simulation on dev %s\n", devname);
		return 0;
	}

	snprintf(ifname_simu, IFNAMSIZ,  "%s_%d", devname, autosimu);
	snprintf(filename, 64, "/tmp/ppp_manual_diag.%s", ifname_simu);
	if(simu_debug)
	{
		printf("[SIMU]TimeOut and File(%s) not exist, dev=%s\n", filename, devname);
	}
#ifdef CONFIG_USER_PPPD
	disconnectPPP(7);
#endif
#ifdef CONFIG_USER_SPPPD
	va_cmd("/bin/spppctl", 2, 1, "del", "7");
#endif
#if defined(CONFIG_RTL_SMUX_DEV)
	va_cmd("/bin/smuxctl", 2, 1, "--if-delete", ifname_simu);
#else
	va_cmd("/bin/ethctl", 4, 1, "remsmux", "ipoe", ALIASNAME_NAS0, ifname_simu);
#endif
	mib_chain_delete(MIB_SIMU_ATM_VC_TBL, i);
#ifdef CONFIG_IPV6
	if(ipt>0)
	{
		setup_disable_ipv6("ppp7", 1);
	}
#endif
	return 0;
}

/*
return :
	  0 : - get success
	-1 : - devname invalid(null or not exist)
	-2 : - mib operator failed
	-3 : - time-out!
	-4 : - result file not exist.
	-5 : - inprocess.
*/
int getSimulationResult(char* devname, struct DIAG_RESULT_T* state)
{
	long queryTime = 0;
	struct sysinfo info;
	int i, totalNum;
	int if_unit=-1, resultNum=-1;
	MIB_CE_ATM_VC_T simuEntry;
	unsigned char ipt = 0;
	char ifname[IFNAMSIZ];
	char ifname_simu[IFNAMSIZ];
	char filename[64];
	unsigned char buf[300];
	unsigned char strName[16];
	unsigned char strVal[256];
	FILE *fp=NULL;
	struct DIAG_RESULT_T result;
	int simStartTime = 0;
	int autosimu = 0;
	unsigned int pid = -1;
	int foundFile = 0;

	if(simu_debug)
		printf("[SIMU]getSimulationResult\n");
	if((NULL == devname)||(NULL == state))
		return -1;

	sysinfo(&info);
	queryTime = (int)info.uptime;

	mib_get_s(MIB_AUTO_DIAG_STARTTIME, (void *)&simStartTime, sizeof(simStartTime));

	totalNum = mib_chain_total(MIB_SIMU_ATM_VC_TBL);
	for(i = 0; i < totalNum; i++)
	{
		if(!mib_chain_get(MIB_SIMU_ATM_VC_TBL, i, (void *)&simuEntry))
			return -2;

		memset(ifname, 0, IFNAMSIZ);
		ifGetName(PHY_INTF(simuEntry.ifIndex), ifname, sizeof(ifname));
		if((!strncmp(devname, ifname, IFNAMSIZ))&&(1 == simuEntry.mbs))
		{
			//exist
			ipt = simuEntry.IpProtocol - 1;
			break;
		}
	}

	if(i == totalNum)
	{
		if(simu_debug)
			printf("[SIMU]No simulation on dev %s\n", devname);
		return -1;
	}

	snprintf(ifname_simu, IFNAMSIZ,  "%s_%d", devname, autosimu);

#ifdef CONFIG_USER_PPPD
	if(read_pid("/var/ppp/ppp7.pid") > 0)
		goto inprogress;
#endif

	snprintf(filename, 64, "/tmp/ppp_manual_diag.%s", ifname_simu);
	memset(&result, 0, sizeof(struct DIAG_RESULT_T));
	result.ipType = ipt;
	
	fp = fopen(filename, "r");
	if(fp!=NULL)
	{
		foundFile++;
		while((fgets(buf, 300, fp))!=NULL)
		{
			if(sscanf(buf, "%s %s\n", strName, strVal)!= -1)
			{
				if(simu_debug)
				{
					printf("[%s %d][SIMU]strName=%s, strVal=%s\n", __func__, __LINE__, strName, strVal);
				}
				if(!strcmp(strName, "unit"))
				{
					if_unit = atoi(strVal);
				}
				else if(!strcmp(strName, "state"))
				{
					resultNum = atoi(strVal);
					if(DIAG_INPROCESS!=resultNum)
					{
						memcpy(result.result, resultArray[resultNum], 128);
					}
				}
				else if(!strcmp(strName, "errCode"))
				{
					result.errCode= atoi(strVal);
				}
				else if(!strcmp(strName, "authMSG"))
				{
					memcpy(result.authMSG, buf+8, 256);
					char *p = NULL;
					p = strchr(result.authMSG, '\n');
					if (p) *p = '\0';
					if(simu_debug)
					{
						printf("[SIMU]---->authMSG=%s\n", result.authMSG);
					}
				}
				else if(!strcmp(strName, "session"))
				{
					result.sessionId = atoi(strVal);
				}
				else if(!strcmp(strName, "ipAddr"))
				{
					memcpy(result.ipAddr, strVal, 16);
				}
				else if(!strcmp(strName, "gw"))
				{
					memcpy(result.gateWay, strVal, 16);
				}
				else if(!strcmp(strName, "dns"))
				{
					memcpy(result.dns, strVal, 16);
				}
				else if(!strcmp(strName, "ipv6GW"))
				{
					memcpy(result.ipv6GW, strVal, 256);
				}
			}
		}
		fclose(fp);
	}
	
#ifdef CONFIG_USER_PPPD
#ifdef CONFIG_IPV6
	snprintf(filename, 64, "/tmp/ppp_manual_diag_ipv6.%s", ifname_simu);
	fp = fopen(filename, "r");
	if(fp!=NULL)
	{
		foundFile++;
		while((fgets(buf, 300, fp))!=NULL)
		{
			if(sscanf(buf, "%s %s\n", strName, strVal)!= -1)
			{
				if(simu_debug)
				{
					printf("[%s %d][SIMU]strName=%s, strVal=%s\n", __func__, __LINE__, strName, strVal);
				}
				if(!strcmp(strName, "unit"))
				{
					if_unit = atoi(strVal);
				}
				else if(!strcmp(strName, "state"))
				{
					resultNum = atoi(strVal);
					if(DIAG_INPROCESS!=resultNum)
					{
						memcpy(result.result, resultArray[resultNum], 128);
					}
				}
				else if(!strcmp(strName, "errCode"))
				{
					result.errCode= atoi(strVal);
				}
				else if(!strcmp(strName, "authMSG"))
				{
					memcpy(result.authMSG, buf+8, 256);
					char *p = NULL;
					p = strchr(result.authMSG, '\n');
					if (p) *p = '\0';
					if(simu_debug)
					{
						printf("[SIMU]---->authMSG=%s\n", result.authMSG);
					}
				}
				else if(!strcmp(strName, "session"))
				{
					result.sessionId = atoi(strVal);
				}
				else if(!strcmp(strName, "ipv6GW"))
				{
					memcpy(result.ipv6GW, strVal, 256);
				}
			}
		}
		fclose(fp);
	}
#endif
#endif

inprogress:
	if(foundFile == 0){
	//result file not exist.
		if((queryTime-simStartTime)>=30)	//time-out
		{
			if(simu_debug)
			{
				printf("[SIMU]TimeOut and File(%s) not exist, dev=%s\n", filename, devname);
			}
#ifdef CONFIG_USER_PPPD
			disconnectPPP(7);
#endif
#ifdef CONFIG_USER_SPPPD
			va_cmd("/bin/spppctl", 2, 1, "del", "7");
#endif
#if defined(CONFIG_RTL_SMUX_DEV)
			va_cmd("/bin/smuxctl", 2, 1, "--if-delete", ifname_simu);
#else
			va_cmd("/bin/ethctl", 4, 1, "remsmux", "ipoe", ALIASNAME_NAS0, ifname_simu);
#endif
            mib_chain_delete(MIB_SIMU_ATM_VC_TBL, i);

			memset(&result, 0, sizeof(struct DIAG_RESULT_T));
			memcpy(result.result, "unKnown", strlen("unKnown"));
			result.errCode = 6;
			memcpy(state, &result, sizeof(struct DIAG_RESULT_T));
			return 0;
		}
		else
		{
			if(simu_debug)
				printf("[SIMU]File(%s) not exist, dev=%s\n", filename, devname);
			return -5;
		}
	}
	
	if((DIAG_SUCCESS==resultNum)&&(result.ipType>0))
	{
		int num;
		struct ipv6_ifaddr ip6_addr;
		struct in6_addr ip6Prefix;
		unsigned char value[48], len;
		unsigned int ret = 0;
		char iaprfix[64], dns[64], aftr[64];

		snprintf(ifname, IFNAMSIZ, "ppp%d", if_unit);
		num=getifip6(ifname, IPV6_ADDR_UNICAST, &ip6_addr, 1);
		if (!num)
		{
			//IPv6 Address has be not get already.
			if(simu_debug)
				printf("[%s %d][SIMU]IP6Addr on %s is not ready.\n", __func__, __LINE__, ifname);
			sysinfo(&info);
			queryTime = (int)info.uptime;
			if((queryTime-simStartTime)>=30)
			{
				if(simu_debug)
					printf("[%s %d][SIMU]TimeOut, result=ERROR_SERVER_OUT_OF_RESOURCES_IPV6, errCode=9\n", __func__, __LINE__);
				pid = read_pid("/var/run/autoSimu.pid");
				if ( pid > 0){
					if(simu_debug)
						printf("Send signal USR1 to pid %d to kill thread.\n",pid);
					kill(pid, SIGUSR1);
				}
				if(if_unit==-1)
				{
#ifdef CONFIG_USER_PPPD
					disconnectPPP(7);
#endif
#ifdef CONFIG_USER_SPPPD
					va_cmd("/bin/spppctl", 2, 1, "del", "7");
#endif
				}
				else
				{
#ifdef CONFIG_USER_PPPD
					disconnectPPP(if_unit);
#endif
#ifdef CONFIG_USER_SPPPD
					snprintf(buf, 64, "%d", if_unit);
					va_cmd("/bin/spppctl", 2, 1, "del", buf);
#endif
				}
#if defined(CONFIG_RTL_SMUX_DEV)
				va_cmd("/bin/smuxctl", 2, 1, "--if-delete", ifname_simu);
#else
				va_cmd("/bin/ethctl", 4, 1, "remsmux", "ipoe", ALIASNAME_NAS0, ifname_simu);
#endif
				mib_chain_delete(MIB_SIMU_ATM_VC_TBL, i);
				resultNum = DIAG_SERVER_OUT_OF_RESOURCE_IPV6;
				memcpy(result.result, resultArray[resultNum], 128);
				result.errCode = 9;
				memcpy(state, &result, sizeof(struct DIAG_RESULT_T));
				snprintf(buf, 256, "rm -rf /tmp/ppp_manual_diag.%s", ifname_simu);
				system(buf);
#ifdef CONFIG_IPV6
				snprintf(buf, 256, "rm -rf /tmp/ppp_manual_diag_ipv6.%s", ifname_simu);
				system(buf);
#endif
				return 0;
			}
			else
			{
				MIB_CE_ATM_VC_T Entry;
				int j, totalNum;
				int found = 0;

				//printf("[%s %d]simuEntry.ifIndex=%d\n", __func__, __LINE__, simuEntry.ifIndex);
				totalNum = mib_chain_total(MIB_ATM_VC_TBL);
				for(j = 0; j < totalNum; j++)
				{
					if(!mib_chain_get(MIB_ATM_VC_TBL, j, (void *)&Entry))
						continue;
					if(Entry.enable == 0)
						continue;

					//printf("[%s %d]Entry.ifIndex=%d\n", __func__, __LINE__, Entry.ifIndex);
					if(PHY_INTF(Entry.ifIndex) == PHY_INTF(simuEntry.ifIndex))
					{
						found = 1;
						//printf("[%s %d]found\n", __func__, __LINE__);
						break;
					}
				}

				if((1==found)&&(Entry.cmode==CHANNEL_MODE_BRIDGE))
				{
					char ifup_path[32];
					//printf("[%s %d]rewrite script and run it\n", __func__, __LINE__);
					generate_ifupv6_script_manual(ifname_simu, &Entry);
					snprintf(ifup_path, 32, "/var/ppp/ifupv6_ppp7");
					va_niced_cmd(ifup_path, 0, 1);
					sleep(5);
				}
				resultNum = DIAG_INPROCESS;
			}
		}
		else
		{
			inet_ntop(PF_INET6, &ip6_addr.addr, buf, 256);
			sprintf(result.ipv6Addr, "%s/%d", buf, ip6_addr.prefix_len);

			memcpy(&ip6Prefix, &ip6_addr.addr, 16);
			{
				int j;
				for(j=0; j<(ip6_addr.prefix_len>>3); j++)
				{
					ip6Prefix.s6_addr[15-j] = 0;
				}

			}
			inet_ntop(PF_INET6, &ip6Prefix, buf, 256);
			sprintf(result.ipv6Prefix, "%s/%d", buf, ip6_addr.prefix_len);

#ifdef DHCPV6_ISC_DHCP_4XX
		if ((1==simuEntry.Ipv6Dhcp) || (CHANNEL_MODE_BRIDGE == simuEntry.cmode)
#ifdef DUAL_STACK_LITE
			||((simuEntry.dslite_enable == 1) && (simuEntry.dslite_aftr_mode == IPV6_DSLITE_MODE_AUTO))
#endif
			)
            {
                snprintf(filename, 64, "%s.ppp%d", PATH_DHCLIENT6_LEASE, if_unit);
			    sleep(1);
			    ret = getOptionFromleases(filename, iaprfix, dns, aftr);
                if(0 == ret)
    			{
    				MIB_CE_ATM_VC_T Entry;
    				int j, totalNum;
    				int found = 0;

    				//printf("[%s %d]simuEntry.ifIndex=%d\n", __func__, __LINE__, simuEntry.ifIndex);
    				totalNum = mib_chain_total(MIB_ATM_VC_TBL);
    				for(j = 0; j < totalNum; j++)
    				{
    					if(!mib_chain_get(MIB_ATM_VC_TBL, j, (void *)&Entry))
    						continue;
    					if(Entry.enable == 0)
    						continue;

    					//printf("[%s %d]Entry.ifIndex=%d\n", __func__, __LINE__, Entry.ifIndex);
    					if(PHY_INTF(Entry.ifIndex) == PHY_INTF(simuEntry.ifIndex))
    					{
    						found = 1;
    						//printf("[%s %d]found\n", __func__, __LINE__);
    						break;
    					}
    				}

    				if(1==found)
    				{
    					char ifup_path[32];
                        if(CHANNEL_MODE_BRIDGE == simuEntry.cmode)
                        {
        					//printf("[%s %d]rewrite script and run it\n", __func__, __LINE__);
        					generate_ifupv6_script_get_PD(ifname_simu, &Entry);
        					snprintf(ifup_path, 32, "/var/ppp/ifupv6_ppp7");
        					va_niced_cmd(ifup_path, 0, 1);
                        }
    					sleep(5);
    					ret = getOptionFromleases(filename, iaprfix, dns, aftr);
    					//printf("[%s %d]ret=%d\n", __func__, __LINE__, ret);
    				}
    			}
    			if(0!=(ret&1))
    			{
    				memcpy(result.ipv6LANPrefix, iaprfix, 256);
    			}
    			if(0!=(ret&2))
    			{
    				memcpy(result.ipv6DNS, dns, 256);
    			}
    			if(0!=(ret&4))
    			{
    				memcpy(result.aftr, aftr, 256);
    			}
            }
#endif

#if	defined(DUAL_STACK_LITE)
			if(simuEntry.dslite_enable == 1)
			{
				if(1==simuEntry.dslite_aftr_mode)
				{
                    //manual mode, get aftr from mib_entry.
					memcpy(result.aftr, simuEntry.dslite_aftr_hostname, 64);

                    pid = read_pid("/var/run/autoSimu.pid");
    				if ( pid > 0){
    					if(simu_debug)
    						printf("Send signal USR2 to autoSimu pid %d\n",pid);
    					kill(pid, SIGUSR2);
    				}
				}

				sysinfo(&info);
				queryTime = (int)info.uptime;
				if((!simuEntry.dslite_aftr_addr[0])&&((queryTime-simStartTime)<30))
				{
					return -5;
				}
				if((simuEntry.dslite_aftr_addr[0])&&((queryTime-simStartTime)<30))
				{
					char str_gateway[64];
					inet_ntop(AF_INET6, (const void *) simuEntry.dslite_aftr_addr, str_gateway, 64);
					memcpy(result.aftr, str_gateway, 256);
				}

				if((queryTime-simStartTime)>=30)
				{
					memcpy(result.result, resultArray[DIAG_SERVER_OUT_OF_RESOURCE_IPV6], 128);
					result.errCode = 9;
					goto setok;
				}
			}
#endif
		}
	}


	if((queryTime-simStartTime)>=30)	//time-out
	{
		if(simu_debug)
		{
			printf("[%s %d]Time-Out\n", __func__, __LINE__);
		}
		pid = read_pid("/var/run/autoSimu.pid");
		if ( pid > 0){
			if(simu_debug)
				printf("Send signal USR1 to pid %d to kill thread.\n",pid);
			kill(pid, SIGUSR1);
		}
		if(if_unit==-1)
		{
#ifdef CONFIG_USER_PPPD
			disconnectPPP(7);
#endif
#ifdef CONFIG_USER_SPPPD		
			va_cmd("/bin/spppctl", 2, 1, "del", "7");
#endif
		}
		else
		{
#ifdef CONFIG_USER_PPPD
			disconnectPPP(if_unit);
#endif
#ifdef CONFIG_USER_SPPPD
			snprintf(buf, 64, "%d", if_unit);
			va_cmd("/bin/spppctl", 2, 1, "del", buf);
#endif
		}
#if defined(CONFIG_RTL_SMUX_DEV)
		va_cmd("/bin/smuxctl", 2, 1, "--if-delete", ifname_simu);
#else
		va_cmd("/bin/ethctl", 4, 1, "remsmux", "ipoe", ALIASNAME_NAS0, ifname_simu);
#endif

		mib_chain_delete(MIB_SIMU_ATM_VC_TBL, i);
		snprintf(buf, 256, "rm -rf /tmp/ppp_manual_diag.%s", ifname_simu);
		system(buf);
#ifdef CONFIG_IPV6
		snprintf(buf, 256, "rm -rf /tmp/ppp_manual_diag_ipv6.%s", ifname_simu);
		system(buf);
#endif

		memset(&result, 0, sizeof(struct DIAG_RESULT_T));
		memcpy(result.result, resultArray[DIAG_UNKNOWN], 128);
		result.errCode = 6;
		memcpy(state, &result, sizeof(struct DIAG_RESULT_T));
		return 0;
	}

	if(DIAG_INPROCESS==resultNum)
	{
		return -5;
	}

setok:
	pid = read_pid("/var/run/autoSimu.pid");
	if ( pid > 0){
		if(simu_debug)
			printf("Send signal USR1 to pid %d to kill thread.\n",pid);
		kill(pid, SIGUSR1);
	}
	if(if_unit==-1)
	{
#ifdef CONFIG_USER_PPPD
		disconnectPPP(7);
#endif
#ifdef CONFIG_USER_SPPPD
		va_cmd("/bin/spppctl", 2, 1, "del", "7");
#endif
	}
	else
	{
#ifdef CONFIG_USER_PPPD
		disconnectPPP(if_unit);
#endif
#ifdef CONFIG_USER_SPPPD
		snprintf(buf, 64, "%d", if_unit);
		va_cmd("/bin/spppctl", 2, 1, "del", buf);
#endif
	}
#if defined(CONFIG_RTL_SMUX_DEV)
	va_cmd("/bin/smuxctl", 2, 1, "--if-delete", ifname_simu);
#else
	va_cmd("/bin/ethctl", 4, 1, "remsmux", "ipoe", ALIASNAME_NAS0, ifname_simu);
#endif
	mib_chain_delete(MIB_SIMU_ATM_VC_TBL, i);
#if	defined(DUAL_STACK_LITE)
	if(simuEntry.dslite_enable == 1)
	{
		restart_dnsrelay();
	}
#endif
	memcpy(state, &result, sizeof(struct DIAG_RESULT_T));
	if((state->errCode == 14)&&(1==simu_debug))
	{
		printf("[SIMU]2---->authMSG=%s\n", state->authMSG);
	}
	return 0;
}

void startAutoBridgePppoeDail(void * ifIndex);

int getValidPPPidx()
{
    int i, totalNum;
    MIB_CE_ATM_VC_T entry;
    unsigned char pppmask = 0;

    totalNum = mib_chain_total(MIB_ATM_VC_TBL);
    for(i = 0; i < totalNum; i++)
    {
        if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry))
		{
			continue;
		}
        if(entry.cmode == CHANNEL_MODE_PPPOE)
        {
            pppmask |= (0x1<<PPP_INDEX(entry.ifIndex));
        }
    }

    totalNum = mib_chain_total(MIB_SIMU_ATM_VC_TBL);
    for(i = 0; i < totalNum; i++)
    {
        if(!mib_chain_get(MIB_SIMU_ATM_VC_TBL, i, (void *)&entry))
		{
			continue;
		}
        if(entry.cmode == CHANNEL_MODE_PPPOE)
        {
            pppmask |= (0x1<<PPP_INDEX(entry.ifIndex));
        }
    }

    for(i = 6; i >= 0; i--)
    {
        if(0 == (pppmask&(0x1<<i)))
        {
            return i;
        }
    }

    return -1;
}
void checkAutoSimulationResult(void * arg)
{
	int suceess=0;
	unsigned int time;
	MIB_CE_AUTO_DIAG_PARAM_T paramEntry;
	char ifname[IFNAMSIZ];
	char fileName[64];
	char buff[64];
	FILE *fp=NULL;
	int autosimu = 1;
    int ppp_idx = 0;
	unsigned int ifIndex;

	if(arg == NULL){
		printf("%s %d: arg is NULL\n",__FUNCTION__, __LINE__);
		return;
	}
	ifIndex = *((unsigned int*)arg);
    ppp_idx = PPP_INDEX(ifIndex);

    ifGetName(PHY_INTF(ifIndex), ifname, sizeof(ifname));
    if(simu_debug)
		printf("[SIMU]checkAutoSimulationResult, wan=%s\n", ifname);
	snprintf(ifname+strlen(ifname), IFNAMSIZ-strlen(ifname), "_%d", autosimu);
	snprintf(fileName, 64, "/tmp/ppp_auto_diag.%s", ifname);
    fp = fopen(fileName, "r");
	if(fp!=NULL)
	{
		char unit[16], result[32];
		if(((fgets(buff, 64, fp))!=NULL)&&(sscanf(buff, "%s %s\n", unit, result)!= -1))
		{
			if((0==atoi(result))||(14==atoi(result)))
			{
				suceess = 1;
			}
			if(simu_debug)
				printf("[%s %d][SIMU]del ppp unit %s\n", __func__, __LINE__, unit);
#ifdef CONFIG_USER_PPPD
			disconnectPPP(atoi(unit));
#endif
#ifdef CONFIG_USER_SPPPD
			snprintf(buff, 64, "/bin/spppctl del %s", unit);
			system(buff);
#endif
		}
		fclose(fp);
	}

    if(mib_chain_get(MIB_AUTO_DIAG_PARAM_TBL, 0, (void *)&paramEntry))
    {
        if(0==suceess)
		{
			time = paramEntry.failRetryTimeList*60 - 30;
		}
		else
		{
			time = paramEntry.timeList*60 - 30;
		}

		if(simu_debug)
			printf("[%s %d][SIMU]startAutoBridgePppoeDail after %d seconds\n", __func__, __LINE__, time);
		unsigned int *malloc_int;
		malloc_int = malloc(sizeof(unsigned int));
		*malloc_int = ifIndex ;
		TIMEOUT_F(startAutoBridgePppoeDail, (void *)malloc_int, time, autoSimulation_ch[ppp_idx]);
    }
	free(arg);
	return;
}

void startAutoBridgePppoeDail(void * arg)
{
    MIB_CE_AUTO_DIAG_PARAM_T paramEntry;
    struct data_to_pass_st msg;
    char ifname[IFNAMSIZ];
	char ifname_simu[IFNAMSIZ];
    int autosimu = 1;
    int ppp_idx = 0;
	unsigned int ifIndex;
	int entryIndex=0;
	MIB_CE_ATM_VC_T simuEntry;
	if(arg == NULL){
		printf("%s %d: arg is NULL\n",__FUNCTION__, __LINE__);
		return;
	}

	ifIndex = *((unsigned int*)arg);
    ppp_idx = PPP_INDEX(ifIndex);
    if(mib_chain_get(MIB_AUTO_DIAG_PARAM_TBL, 0, (void *)&paramEntry))
    {
        ifGetName(PHY_INTF(ifIndex), ifname, sizeof(ifname));
        snprintf(ifname_simu, IFNAMSIZ, "%s_%d", ifname, autosimu);

#ifdef CONFIG_USER_PPPD
		ifGetName(ifIndex, ifname, sizeof(ifname));
		getSimuWanEntrybyIfname(ifname, &simuEntry, &entryIndex);
		snprintf(simuEntry.pppUsername, sizeof(simuEntry.pppUsername), "%s", paramEntry.userName);
		snprintf(simuEntry.pppPassword, sizeof(simuEntry.pppPassword), "%s", paramEntry.passWord);
		generate_pppd_init_script(&simuEntry);
		generate_pppd_ipup_script(&simuEntry);
		generate_pppd_ipdown_script(&simuEntry);
		write_to_simu_ppp_options(&simuEntry);
		connectPPPExt(ifname_simu, &simuEntry, CHANNEL_MODE_PPPOE, 1);
#endif
#ifdef CONFIG_USER_SPPPD
        //start auto simulation for bridge wan.
    	snprintf(msg.data, BUF_SIZE, "spppctl add %d simulation 16 auth %s pppoe %s", ppp_idx, ppp_auth[paramEntry.authType], ifname_simu);
    	if(paramEntry.userName[0]!='\0')
    	{
			snprintf(msg.data + strlen(msg.data), BUF_SIZE - strlen(msg.data), " username %s", paramEntry.userName);
    	}
    	if(paramEntry.passWord[0]!='\0')
    	{
			snprintf(msg.data + strlen(msg.data), BUF_SIZE - strlen(msg.data), " password %s", paramEntry.passWord);
    	}
    	//for debug
    	//snprintf(msg.data, BUF_SIZE, "%s servicename linux247", msg.data);
    	if(simu_debug)
    		printf("[%s %d][SIMU]:%s\n", __func__, __LINE__, msg.data);
    	//end

    	write_to_pppd(&msg);
#endif

	unsigned int *malloc_int;
	malloc_int = malloc(sizeof(unsigned int));
	*malloc_int = ifIndex ;
        TIMEOUT_F(checkAutoSimulationResult, (void *)malloc_int, 30, autoSimulation_ch[ppp_idx]);
    }

    free(arg);

    return;
}

void startAutoBridgePppoeSimulationTimeout(void* arg);

/*
  parameter:
  wanname: bridge wan's name(nas0_0/nas0_1..etc), can be null(means start all bridge&internet wan).
  return:
 */
int startAutoBridgePppoeSimulation(char* wanname)
{
	int i, totalNum, simu_idx, totalsimuNum;
	MIB_CE_ATM_VC_T Entry;
	MIB_CE_ATM_VC_T simuEntry;
	MIB_CE_AUTO_DIAG_PARAM_T paramEntry;
	char ifname[IFNAMSIZ];
	char ifname_simu[IFNAMSIZ];
	unsigned char autoDiagEnable;
	int autosimu = 1;
	int ppp_idx = 0;

	if(simu_debug)
	{
		printf("[SIMU]startAutoBridgePppoeSimulation, for %s\n", (wanname==NULL)?"all interface":wanname);
	}

	if((mib_get_s(MIB_AUTO_DIAG_ENABLE, (void *)(&autoDiagEnable), sizeof(autoDiagEnable))) && (0==autoDiagEnable))
	{
		if(simu_debug)
			printf("[%s %d][SIMU]auto simulation disabled\n", __func__, __LINE__);
		return -1;
	}

	if((0 == omcistate) || !mib_chain_get(MIB_AUTO_DIAG_PARAM_TBL, 0, (void *)&paramEntry))
	{
		if(simu_debug)
			printf("[%s %d][SIMU]omci does not successed(omcistate=%d) or DIAG_PARAM get fail.\n", __func__, __LINE__, omcistate);
        if(0 == omcistate)
        {
            //if omci is not ready, try 10 seconds later.
            TIMEOUT_F(startAutoBridgePppoeSimulationTimeout, 0, 10, autoSimulation_ch[MAX_CH-1]);
        }
		return -1;
	}

	totalNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			if(simu_debug)
				printf("[%s %d][SIMU]Get mib[%d] failed\n", __func__, __LINE__, i);
			continue;
		}

		if((Entry.enable == 0)||(Entry.cmode != CHANNEL_MODE_BRIDGE)||(Entry.applicationtype!=X_CT_SRV_INTERNET))
		{
			continue;
		}
		memset(ifname, 0, IFNAMSIZ);
		ifGetName(PHY_INTF(Entry.ifIndex), ifname, sizeof(ifname));

        if((wanname != NULL) && (strncmp(ifname, wanname, 6)))
        {
            //only start a bridge wan by wan name
            continue;
        }
		snprintf(ifname_simu, IFNAMSIZ, "%s_%d", ifname, autosimu);

		totalsimuNum = mib_chain_total(MIB_SIMU_ATM_VC_TBL);
		for(simu_idx = 0; simu_idx < totalsimuNum; simu_idx++)
		{
			if(!mib_chain_get(MIB_SIMU_ATM_VC_TBL, simu_idx, (void *)&simuEntry))
			{
				continue;
			}

			if((PHY_INTF(simuEntry.ifIndex)==PHY_INTF(Entry.ifIndex))&&(2 == simuEntry.mbs))
			{
				//duplicate...delete the simulation first.
#if defined(CONFIG_RTL_SMUX_DEV)
				va_cmd("/bin/smuxctl", 2, 1, "--if-delete", ifname_simu);
#else
				va_cmd("/bin/ethctl", 4, 1, "remsmux", "ipoe", ALIASNAME_NAS0, ifname_simu);
#endif
				break;
			}
		}

        ppp_idx = getValidPPPidx();

        if(ppp_idx >= 0)
        {

            memcpy(&simuEntry, &Entry, sizeof(MIB_CE_ATM_VC_T));
            simuEntry.applicationtype = X_CT_SRV_INTERNET;
            simuEntry.cmode = CHANNEL_MODE_PPPOE;
            simuEntry.mbs = 2;
#if defined(ITF_GROUP) || defined(NEW_PORTMAPPING)
            simuEntry.itfGroup = 0;     //simu connection, no need to bind with any lan dev
#endif
            simuEntry.MacAddr[MAC_ADDR_LEN-1] = (simuEntry.MacAddr[MAC_ADDR_LEN-1] + 8);
            simuEntry.MacAddr[MAC_ADDR_LEN-2]++;
            simuEntry.brmode = BRIDGE_DISABLE;
            simuEntry.ifIndex &= ~(0xFF<<8);
            simuEntry.ifIndex |= (ppp_idx<<8);
            if(simu_idx != totalsimuNum)
            {
                mib_chain_update(MIB_SIMU_ATM_VC_TBL, &simuEntry, simu_idx);
            }
            else
            {
                mib_chain_add(MIB_SIMU_ATM_VC_TBL, &simuEntry);
            }

            //generate Simulation WAN
#ifdef CONFIG_RTL_MULTI_ETH_WAN
            addSimuEthWANdev(&simuEntry, autosimu);
#endif
            setup_simu_ethernet_config(&simuEntry, ifname_simu);

		unsigned int *malloc_int;
		malloc_int = malloc(sizeof(unsigned int));
		*malloc_int = simuEntry.ifIndex ;
            startAutoBridgePppoeDail((void *)malloc_int);
        }
	}

    return 0;
}

void startAutoBridgePppoeSimulationTimeout(void* arg)
{
	startAutoBridgePppoeSimulation((char *)arg);
}

/*
  parameter:
  wanname: bridge wan's name(nas0_0/nas0_1..etc), can be null(means stop all bridge wan).
  return:
 */
int stopAutoBridgePppoeSimulation(char* wanname)
{
	int i, totalNum;
	MIB_CE_ATM_VC_T simuEntry;
	char ifname[IFNAMSIZ];
    char ifname_simu[IFNAMSIZ];
	char fileName[64];
	char buff[64];
	FILE *fp=NULL;
	int autosimu = 1;

	if(simu_debug)
		printf("[SIMU]stopAutoBridgePppoeSimulation\n");
	
	totalNum = mib_chain_total(MIB_SIMU_ATM_VC_TBL);
	for(i = totalNum-1; i >= 0; i--)
	{
		mib_chain_get(MIB_SIMU_ATM_VC_TBL, i, (void *)&simuEntry);
		if(2 != simuEntry.mbs)//not bridge pppoe simulation
		{
			continue;
		}

		ifGetName(PHY_INTF(simuEntry.ifIndex), ifname, sizeof(ifname));
        if((wanname != NULL) && (strncmp(ifname, wanname, 6)))
        {
            //only stop the bridge wan by wan name
            continue;
        }
        UNTIMEOUT_F(0, 0, autoSimulation_ch[PPP_INDEX(simuEntry.ifIndex)]);
		snprintf(ifname_simu, IFNAMSIZ, "%s_%d", ifname, autosimu);
		snprintf(fileName, 64, "/tmp/ppp_auto_diag.%s", ifname_simu);
		fp = fopen(fileName, "r");
		if(fp!=NULL)
		{
			char unit[16], result[32];
			if(((fgets(buff, 64, fp))!=NULL)&&(sscanf(buff, "%s %s\n", unit, result)!= -1))
			{
				if(simu_debug)
					printf("[%s %d][SIMU]del ppp unit %s\n", __func__, __LINE__, unit);
#ifdef CONFIG_USER_PPPD
				disconnectPPP(atoi(unit));
#endif
#ifdef CONFIG_USER_SPPPD
				snprintf(buff, 64, "/bin/spppctl del %s", unit);
				system(buff);
#endif
			}
			fclose(fp);
		}
		//delete.
#if defined(CONFIG_RTL_SMUX_DEV)
		va_cmd("/bin/smuxctl", 2, 1, "--if-delete", ifname_simu);
#else
		va_cmd("/bin/ethctl", 4, 1, "remsmux", "ipoe", ALIASNAME_NAS0, ifname_simu);
#endif
		mib_chain_delete(MIB_SIMU_ATM_VC_TBL, i);
	}

	return 0;
}

int initAutoBridgeFIFO()
{
	int ret = 0;
	int server_fifo_fd = -1;

	unlink("/tmp/autoBridgePPP_fifo");
	ret = mkfifo("/tmp/autoBridgePPP_fifo", 0777);

	if(ret != 0)
	{
		printf("[%s %d][SIMU]errno = %d\n", __func__, __LINE__, errno);
	}

	server_fifo_fd = open("/tmp/autoBridgePPP_fifo", O_RDONLY|O_NONBLOCK);
	if(fcntl(server_fifo_fd, F_SETFD, fcntl(server_fifo_fd, F_GETFD) | FD_CLOEXEC) == -1)
		printf("[%s %d]fcntl fail\n",__FUNCTION__, __LINE__);
	if (server_fifo_fd == -1)
	{
		printf("[SIMU]create ppp_autoBridge fifo failure\n");
	}

	return server_fifo_fd;
}

int setOmciState(int state)
{
	if((0==omcistate)&&(1==state))
	{
		omcistate = state;
		printf("[%s %d][SIMU]omcistate=%d\n", __func__, __LINE__, omcistate);
		startAutoBridgePppoeSimulation(NULL);
	}

	return 0;
}

int setSimuDebug(int debug)
{
	simu_debug = debug;
	printf("[%s %d][SIMU]Set simu_debug=%d\n", __FILE__, __LINE__, simu_debug);
	return 0;
}

int poll_msg(int fd)
{
	int bytesToRead = 0;

	if(fd < 0)
		return 0;
	(void)ioctl(fd, FIONREAD, (int *)&bytesToRead);
	return bytesToRead;
}
#endif //auto diag

int clsTypeProtoToIPQosProto(int clsTypeProto){
	switch(clsTypeProto){
		case IP_QOS_PROTOCOL_TCP:
			return PROTO_TCP;
		case IP_QOS_PROTOCOL_UDP:
			return PROTO_UDP;
		case IP_QOS_PROTOCOL_TCPUDP:
			return PROTO_UDPTCP;
		case IP_QOS_PROTOCOL_ICMP:
			return PROTO_ICMP;
		case IP_QOS_PROTOCOL_ICMP6:
			return PROTO_ICMP;
		case IP_QOS_PROTOCOL_RTP:
			return PROTO_RTP;
		case IP_QOS_PROTOCOL_ALL:
			return PROTO_NONE;
	}
	return 0;
}

int typeProtoToIPQosProto(int proto_type){
	switch(proto_type){
		case PROTO_TCP:
			return IP_QOS_PROTOCOL_TCP;
		case PROTO_UDP:
			return IP_QOS_PROTOCOL_UDP;
		case PROTO_UDPTCP:
			return IP_QOS_PROTOCOL_TCPUDP;
		case PROTO_ICMP:
			return IP_QOS_PROTOCOL_ICMP;
		case PROTO_RTP:
			return IP_QOS_PROTOCOL_RTP;
		case PROTO_NONE:
			return IP_QOS_PROTOCOL_ALL;
	}
	return 0;
}

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_USER_AP_E8B_QOS)
char* ip_qos_classficationtype_str[IP_QOS_CLASSFICATIONTYPE_MAX]={"SMAC","DMAC","8021P","SIP","DIP","SPORT","DPORT","TOS","DSCP",
#ifdef CONFIG_IPV6
				"SIP6","DIP6","SPORT6","DPORT6","TrafficClass",
#ifdef CONFIG_USER_AP_E8B_QOS
				"FlowLabel", 
#endif
#endif
				"WANInterface","LANInterface","EtherType"};

unsigned int getQosClassficationType(char *type, char *value){
	int  i =0;
	struct in6_addr ip6_addr;
	
#ifndef CONFIG_USER_AP_CT_QOS
	if(strcmp(type, "SIP") == 0)
	{
		if(inet_pton(AF_INET6, value, (void *)&ip6_addr))
			return IP_QOS_CLASSFICATIONTYPE_SIP6;
	}
	else if(strcmp(type, "DIP") == 0)
	{
		if(inet_pton(AF_INET6, value, (void *)&ip6_addr))
			return IP_QOS_CLASSFICATIONTYPE_DIP6;
	}
#endif
	
	for(i=0;i<IP_QOS_CLASSFICATIONTYPE_MAX;i++){
		if(!strcmp(ip_qos_classficationtype_str[i],type)){
			return (i);
		}
	}
	return IP_QOS_CLASSFICATIONTYPE_MAX;
}

unsigned int getQosClassficationProtocol(int ClsIndex)
{
	int totalQosRuleNums;
	int qosRuleIdx = 0;
	totalQosRuleNums=mib_chain_total(MIB_IP_QOS_CLASSFICATIONTYPE_TBL);
	for(qosRuleIdx=0; qosRuleIdx<totalQosRuleNums; qosRuleIdx++){
		MIB_CE_IP_QOS_CLASSFICATIONTYPE_T qosentry;
		mib_chain_get(MIB_IP_QOS_CLASSFICATIONTYPE_TBL,qosRuleIdx,&qosentry);						
		if(qosentry.cls_id==ClsIndex){
			if((qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_SIP))
				|| (qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_DIP)))
				return 1;
			if((qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_SIP6))
				|| (qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_DIP6)))
				return 2;		
		}
	}
	return 0;
}

void updateQosClsTypeEntry(int ClsIndex)
{
#ifndef CONFIG_USER_AP_CT_QOS
	int protocol = 0;
	int totalQosRuleNums;
	int qosRuleIdx = 0;

	protocol = getQosClassficationProtocol(ClsIndex);

	totalQosRuleNums=mib_chain_total(MIB_IP_QOS_CLASSFICATIONTYPE_TBL);
	
	for(qosRuleIdx=0; qosRuleIdx<totalQosRuleNums; qosRuleIdx++){
		MIB_CE_IP_QOS_CLASSFICATIONTYPE_T qosentry;
		mib_chain_get(MIB_IP_QOS_CLASSFICATIONTYPE_TBL,qosRuleIdx,&qosentry);						
		if(qosentry.cls_id==ClsIndex){
			if(protocol == 2)
			{
				if((qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_SPORT | 1<<IP_QOS_CLASSFICATIONTYPE_SPORT6))
					|| (qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_SPORT)))
				{
					qosentry.classficationType = 1<<IP_QOS_CLASSFICATIONTYPE_SPORT6;
					qosentry.sPort6=qosentry.sPort;
					qosentry.sPort6RangeMax=qosentry.sPortRangeMax;
					qosentry.sPort=0;
					qosentry.sPortRangeMax=0;
					mib_chain_update(MIB_IP_QOS_CLASSFICATIONTYPE_TBL, &qosentry, qosRuleIdx);
				}
				else if((qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_DPORT | 1<<IP_QOS_CLASSFICATIONTYPE_DPORT6))
					|| (qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_DPORT)))
				{
					qosentry.classficationType = 1<<IP_QOS_CLASSFICATIONTYPE_DPORT6;
					qosentry.dPort6=qosentry.dPort;
					qosentry.dPort6RangeMax=qosentry.dPortRangeMax;
					qosentry.dPort=0;
					qosentry.dPortRangeMax=0;
					mib_chain_update(MIB_IP_QOS_CLASSFICATIONTYPE_TBL, &qosentry, qosRuleIdx);
				}
				else if((qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_DSCP | 1<<IP_QOS_CLASSFICATIONTYPE_TrafficClass))
					|| (qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_DSCP)))
				{
					qosentry.classficationType = 1<<IP_QOS_CLASSFICATIONTYPE_TrafficClass;
					qosentry.tc = qosentry.qosDscp;
					qosentry.tc_end = qosentry.qosDscp_end;
					qosentry.qosDscp = 0;
					qosentry.qosDscp_end = 0;
					mib_chain_update(MIB_IP_QOS_CLASSFICATIONTYPE_TBL, &qosentry, qosRuleIdx);
				}
			}
			else if(protocol == 1)
			{
				if((qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_SPORT | 1<<IP_QOS_CLASSFICATIONTYPE_SPORT6))
					|| (qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_SPORT6)))
				{
					qosentry.classficationType = 1<<IP_QOS_CLASSFICATIONTYPE_SPORT;
					qosentry.sPort=qosentry.sPort6;
					qosentry.sPortRangeMax=qosentry.sPort6RangeMax;
					qosentry.sPort6=0;
					qosentry.sPort6RangeMax=0;
					mib_chain_update(MIB_IP_QOS_CLASSFICATIONTYPE_TBL, &qosentry, qosRuleIdx);
				}
				else if((qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_DPORT | 1<<IP_QOS_CLASSFICATIONTYPE_DPORT6))
					|| (qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_DPORT6)))
				{
					qosentry.classficationType = 1<<IP_QOS_CLASSFICATIONTYPE_DPORT;
					qosentry.dPort=qosentry.dPort6;
					qosentry.dPortRangeMax=qosentry.dPort6RangeMax;
					qosentry.dPort6=0;
					qosentry.dPort6RangeMax=0;
					mib_chain_update(MIB_IP_QOS_CLASSFICATIONTYPE_TBL, &qosentry, qosRuleIdx);
				}
				else if((qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_DSCP | 1<<IP_QOS_CLASSFICATIONTYPE_TrafficClass))
					|| (qosentry.classficationType == (1<<IP_QOS_CLASSFICATIONTYPE_TrafficClass)))
				{
					qosentry.classficationType = 1<<IP_QOS_CLASSFICATIONTYPE_DSCP;
					qosentry.qosDscp=qosentry.tc;
					qosentry.qosDscp_end=qosentry.tc_end;
					qosentry.tc=0;
					qosentry.tc_end=0;
					mib_chain_update(MIB_IP_QOS_CLASSFICATIONTYPE_TBL, &qosentry, qosRuleIdx);
				}
			}
			else if(protocol == 0)
			{
				if(qosentry.classficationType == 1<<IP_QOS_CLASSFICATIONTYPE_SPORT)
				{
					qosentry.classficationType = 1<<IP_QOS_CLASSFICATIONTYPE_SPORT | 1<<IP_QOS_CLASSFICATIONTYPE_SPORT6;
					qosentry.sPort6=qosentry.sPort;
					qosentry.sPort6RangeMax=qosentry.sPortRangeMax;
					mib_chain_update(MIB_IP_QOS_CLASSFICATIONTYPE_TBL, &qosentry, qosRuleIdx);
				}
				else if(qosentry.classficationType == 1<<IP_QOS_CLASSFICATIONTYPE_DPORT)
				{
					qosentry.classficationType = 1<<IP_QOS_CLASSFICATIONTYPE_DPORT | 1<<IP_QOS_CLASSFICATIONTYPE_DPORT6;
					qosentry.dPort6=qosentry.dPort;
					qosentry.dPort6RangeMax=qosentry.dPortRangeMax;
					mib_chain_update(MIB_IP_QOS_CLASSFICATIONTYPE_TBL, &qosentry, qosRuleIdx);
				}
				else if(qosentry.classficationType == 1<<IP_QOS_CLASSFICATIONTYPE_DSCP)
				{
					qosentry.classficationType = 1<<IP_QOS_CLASSFICATIONTYPE_DSCP | 1<<IP_QOS_CLASSFICATIONTYPE_TrafficClass;
					qosentry.tc=qosentry.qosDscp;
					qosentry.tc_end=qosentry.qosDscp_end;
					mib_chain_update(MIB_IP_QOS_CLASSFICATIONTYPE_TBL, &qosentry, qosRuleIdx);

				}
			}
		}
	}
#endif	
}

unsigned int GetQosClsType(unsigned int clsType)
{
	int idx = -1;
	while(clsType)
	{
		idx ++;
		if(clsType & 0x1)
			break;
		clsType = (clsType >> 1);
	}
	if(idx == -1 || idx >= IP_QOS_CLASSFICATIONTYPE_MAX)
		return IP_QOS_CLASSFICATIONTYPE_MAX;
	return idx;
}

char* QosClsTypeToStr(unsigned int clsType)
{
	int idx =-1;		
	while(clsType)
	{
		clsType = (clsType >> 1);
		idx ++;
	}

#if !defined(CONFIG_RTK_DEV_AP)
	if(idx >= IP_QOS_CLASSFICATIONTYPE_SIP6 && idx <= IP_QOS_CLASSFICATIONTYPE_DPORT6)
		idx -= 6;
	if(idx == IP_QOS_CLASSFICATIONTYPE_TrafficClass)
		idx = IP_QOS_CLASSFICATIONTYPE_DSCP;
#endif
	if(idx != -1)
		return ip_qos_classficationtype_str[idx];
	else
		return NULL;
}

char* ip_qos_protocol_str[IP_QOS_PROTOCOL_MAX]={"TCP","UDP","TCP,UDP","ICMP","ICMP6","RTP","ALL"};
int getProtoType(char *proto){
	int  i =0;
	for(i=0;i<IP_QOS_PROTOCOL_MAX;i++){
		if(!strcmp(ip_qos_protocol_str[i],proto)){
			return i;
		}
	}
	return IP_QOS_PROTOCOL_MAX;
}
int getBitPos(unsigned int bits){
	int i = 0;
	while(i<(sizeof(int)*8)){
		if((1<<i)&bits)
			return i;
		i++;
	}
	return 0;
}

char *get_clsType_protoStr(int protoType)
{
	if(protoType < 0 || protoType >= IP_QOS_PROTOCOL_MAX)
		return NULL;
	return ip_qos_protocol_str[protoType];
}

unsigned char getValidClsID(){
	MIB_CE_IP_QOS_CLASSFICATION_T entry;
	unsigned char id=IP_QOS_CLASSFICATION_ID_START;
	int totalClsNums = mib_chain_total(MIB_IP_QOS_CLASSFICATION_TBL);
	int i = 0;
refind:
	i=0;
	for(i=0;i<totalClsNums;i++){
		if(mib_chain_get(MIB_IP_QOS_CLASSFICATION_TBL,i, &entry)){
			if(id==entry.cls_id){
				id++;
				goto refind;
			}
		}
	}
	return id;
}
int getClsPostionInMib(int cls_id){
	MIB_CE_IP_QOS_CLASSFICATION_T entry;
	unsigned char id=0;
	int totalClsNums = mib_chain_total(MIB_IP_QOS_CLASSFICATION_TBL);
	int i = 0;
	for(i=0;i<totalClsNums;i++){
		if(mib_chain_get(MIB_IP_QOS_CLASSFICATION_TBL,i, &entry)){
			if(cls_id==entry.cls_id){
				return i;
			}
		}
	}
	return -1;
}
int getClsIDInMib(int pos){
	MIB_CE_IP_QOS_CLASSFICATION_T entry;
	unsigned char id=0;
	int totalClsNums = mib_chain_total(MIB_IP_QOS_CLASSFICATION_TBL);
	int i = 0;
	for(i=0;i<totalClsNums;i++){
		if(mib_chain_get(MIB_IP_QOS_CLASSFICATION_TBL,i, &entry)){
			if(i==pos){
				return entry.cls_id;
			}
		}
	}
	return -1;
}

int getQosQueueEnable(int prio){
	int totalQosQueueNum = mib_chain_total(MIB_IP_QOS_QUEUE_TBL);
	int i = 0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	for (i = 0; i < totalQosQueueNum; i++) {
			if (mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i, &qEntry)) {
				if(qEntry.prior==prio){										
					return qEntry.enable;
				}
			}
	}
	return 0;
}

int getClassificationByInstNum(unsigned int instnum, MIB_CE_IP_QOS_CLASSFICATION_T * p,
				     unsigned int *id)
{
	int ret = -1, i, num;

	if ((instnum == 0) || (p == NULL) || (id == NULL))
		return ret;

	num = mib_chain_total(MIB_IP_QOS_CLASSFICATION_TBL);
	for (i = 0; i < num; i++) {
		if (!mib_chain_get(MIB_IP_QOS_CLASSFICATION_TBL, i, p))
			continue;
		if (p->InstanceNum == instnum) {
			*id = i;
			ret = 0;
			break;
		}
	}

	return ret;
}

unsigned int find_max_classification_instanNum()
{
	int total = mib_chain_total(MIB_IP_QOS_CLASSFICATION_TBL);
	int i = 0, max = 0;
	MIB_CE_IP_QOS_CLASSFICATION_T qos_entity;

	for(i = 0 ; i < total ; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_CLASSFICATION_TBL, i, &qos_entity))
			continue;

		if(qos_entity.InstanceNum > max)
			max = qos_entity.InstanceNum;
	}

	return max;
}

unsigned int classification_type_total_by_clsid(unsigned char cls_id)
{
	int total = mib_chain_total(MIB_IP_QOS_CLASSFICATIONTYPE_TBL);
	int i = 0, type_total = 0;
	MIB_CE_IP_QOS_CLASSFICATIONTYPE_T qos_entity;

	for(i = 0 ; i < total ; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_CLASSFICATIONTYPE_TBL, i, &qos_entity))
			continue;

		if(qos_entity.cls_id == cls_id)
		{
			type_total ++;
		}
	}

	return type_total;
}

int getClassificationTypeByInstNumAndClsid(unsigned char cls_id,unsigned int instnum, MIB_CE_IP_QOS_CLASSFICATIONTYPE_T * p,unsigned int *id)
{
	int ret = -1, i, num;

	if ((instnum == 0) || (p == NULL) || (id == NULL))
		return ret;

	num = mib_chain_total(MIB_IP_QOS_CLASSFICATIONTYPE_TBL);
	for (i = 0; i < num; i++) {
		if (!mib_chain_get(MIB_IP_QOS_CLASSFICATIONTYPE_TBL, i, p))
			continue;
		if (p->cls_id == cls_id && p->InstanceNum == instnum) {
			*id = i;
			ret = 0;
			break;
		}
	}

	return ret;
}

unsigned int find_max_classification_type_instanNum_by_clsid(unsigned char cls_id)
{
	int total = mib_chain_total(MIB_IP_QOS_CLASSFICATIONTYPE_TBL);
	int i = 0, max = 0;
	MIB_CE_IP_QOS_CLASSFICATIONTYPE_T qos_entity;

	for(i = 0 ; i < total ; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_CLASSFICATIONTYPE_TBL, i, &qos_entity))
			continue;
		
		if(qos_entity.cls_id == cls_id)
		{
			if(qos_entity.InstanceNum > max)
				max = qos_entity.InstanceNum;
		}

	}

	return max;
}

void delQosRuleByclsid(int cls_id)
{
	int i, entryNum, found=0;
	MIB_CE_IP_QOS_T qosentry;

DeleteNext:	
	found=0;
	entryNum = mib_chain_total(MIB_IP_QOS_TBL);
	for (i = 0; i < entryNum; i++) {
		if (mib_chain_get(MIB_IP_QOS_TBL, i, &qosentry)) {
			if((qosentry.cls_id-IP_QOS_CLASSFICATION_QOS_ID_OFFSET)==cls_id){
				printf("%s %d delete MIB_IP_QOS_TBL %d\n",__FUNCTION__,__LINE__,i);
				found=1;
				mib_chain_delete(MIB_IP_QOS_TBL, i);		
				break;
			}
		}
	}

	if(found == 1)
		goto DeleteNext;
		
}

void QosClassficationToQosRule(int action,int cls_id){
	MIB_CE_IP_QOS_T qosentry;
	MIB_CE_IP_QOS_CLASSFICATION_T clsEntry;
	MIB_CE_IP_QOS_CLASSFICATIONTYPE_T clsTypeEntry;

	int entryNum = mib_chain_total(MIB_IP_QOS_TBL);
	int i=0,j=0;
	printf("%s %d cls_id=%d\n",__FUNCTION__,__LINE__,cls_id);
	switch (action){
			case QOSCLASSFICATION_TO_QOSRULE_ACTION_DEL:
				{			
					delQosRuleByclsid(cls_id);
				}
			break;
			case QOSCLASSFICATION_TO_QOSRULE_ACTION_ADD:
			break;
			case QOSCLASSFICATION_TO_QOSRULE_ACTION_MODIFY:
				{
					//delete the old qos rule
					delQosRuleByclsid(cls_id);
					//get clsEntry
					int totalClsNum = mib_chain_total(MIB_IP_QOS_CLASSFICATION_TBL);
					for (i = 0; i < totalClsNum; i++) {
						if (mib_chain_get(MIB_IP_QOS_CLASSFICATION_TBL, i, &clsEntry)) {
							if(clsEntry.cls_id==cls_id){										
								break;
							}
						}
					}
					//add new qos rule
					int totalClsTypeNum = mib_chain_total(MIB_IP_QOS_CLASSFICATIONTYPE_TBL);
					printf("%s %d totalClsTypeNum %d\n",__FUNCTION__,__LINE__,totalClsTypeNum);
					int addQosRuleFlag = 0;
					
					/* the IpVersion is IPV4(only_ipv4=1) for ICMP/SIP/DIP/SPORT/DPORT/DSCP/TOS/Ipv4ethType;
					   the IpVersion is IPV6(only_ipv6=1) for ICMP6/SIP6/DIP6/SPORT6/DPORT6/TrafficClass/FlowLabel/Ipv6ethType;
					   others are IPV4_IPV6*/
					int only_ipv4 = 0, only_ipv6 = 0;
#ifndef CONFIG_CMCC_ENTERPRISE
					updateQosClsTypeEntry(cls_id);
#endif
					//init qosentry, set cttypemap[j] to 0xff which means not used.
					memset(&qosentry,0,sizeof(qosentry));
#ifdef _PRMT_X_CT_COM_QOS_
					for (j = 0; j < CT_TYPE_NUM; j++) 
					{						
						qosentry.cttypemap[j] = 0xff;					
					}
#endif
					if(clsEntry.m_dscp)
						qosentry.m_dscp=clsEntry.m_dscp;
					qosentry.prior=clsEntry.queue;
					if(clsEntry.m_1p)
						qosentry.m_1p=clsEntry.m_1p+1;
					qosentry.cls_id=clsEntry.cls_id+IP_QOS_CLASSFICATION_QOS_ID_OFFSET;
					qosentry.outif = DUMMY_IFINDEX;
					qosentry.enable = getQosQueueEnable(clsEntry.queue);
					for(i=0;i<totalClsTypeNum;i++){
						if (!mib_chain_get(MIB_IP_QOS_CLASSFICATIONTYPE_TBL, i, &clsTypeEntry)||clsTypeEntry.cls_id!=cls_id)
							continue;
						qosentry.classficationType|=clsTypeEntry.classficationType;
#ifdef _PRMT_X_CT_COM_QOS_
						for (j = 0; j < CT_TYPE_NUM; j++) {
							if (qosentry.cttypemap[j] == 0xff)
								break;
						}
						if(j == CT_TYPE_NUM)
							break;	
#endif
						addQosRuleFlag = 1;
						qosentry.protoType=clsTypeProtoToIPQosProto(clsTypeEntry.protoType);
						printf("qosentry.protoType=%d clsTypeEntry.protoType=%d\n",qosentry.protoType,clsTypeEntry.protoType);
						//Assign the ICMPv6 protocol type to IPv6
						if(clsTypeEntry.protoType == IP_QOS_PROTOCOL_ICMP6)
							only_ipv6 = 1;
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
						else if (clsTypeEntry.protoType == IP_QOS_PROTOCOL_ICMP)
						{
							if (clsTypeEntry.ethType == 0x0800)
								only_ipv4 = 1;
							else if (clsTypeEntry.ethType == 0x86dd)
								only_ipv6 = 1;
						}
						else
							qosentry.IpProtocol = IPVER_IPV4_IPV6;
#endif
						switch(clsTypeEntry.classficationType){
							case (1<<IP_QOS_CLASSFICATIONTYPE_SMAC):								
								memcpy(qosentry.smac,clsTypeEntry.smac,sizeof(qosentry.smac));
								memcpy(qosentry.smac_end,clsTypeEntry.smac_end,sizeof(qosentry.smac_end));
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_SMAC;
#endif
								break;
							case (1<<IP_QOS_CLASSFICATIONTYPE_DMAC):
								memcpy(qosentry.dmac,clsTypeEntry.dmac,sizeof(qosentry.dmac));
								memcpy(qosentry.dmac_end,clsTypeEntry.dmac_end,sizeof(qosentry.dmac_end));
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_DMAC;
#endif
								break;
							case (1<<IP_QOS_CLASSFICATIONTYPE_8021P):
								qosentry.vlan1p = clsTypeEntry.vlan1p+1;
								qosentry.vlan1p_end = clsTypeEntry.vlan1p_end+1;
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_8021P;
#endif
								qosentry.protoType = PROTO_NONE;
								break;
							case (1<<IP_QOS_CLASSFICATIONTYPE_SIP):
								only_ipv4 = 1;
								memcpy(qosentry.sip,clsTypeEntry.sip,sizeof(qosentry.sip));
								memcpy(qosentry.sip_end,clsTypeEntry.sip_end,sizeof(qosentry.sip_end));
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_SIP;
#endif
								break;
							case (1<<IP_QOS_CLASSFICATIONTYPE_DIP):								
								only_ipv4 = 1;
								memcpy(qosentry.dip,clsTypeEntry.dip,sizeof(qosentry.dip));
								memcpy(qosentry.dip_end,clsTypeEntry.dip_end,sizeof(qosentry.dip_end));
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_DIP;
#endif
								break;
							case (1<<IP_QOS_CLASSFICATIONTYPE_SPORT):
								only_ipv4 = 1;
								qosentry.sPort = clsTypeEntry.sPort;
								qosentry.sPortRangeMax = clsTypeEntry.sPortRangeMax;
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_SPORT;
#endif
								if (qosentry.protoType == PROTO_NONE)
									qosentry.protoType = PROTO_UDPTCP;
								break;
							case (1<<IP_QOS_CLASSFICATIONTYPE_DPORT):
								only_ipv4 = 1;
								qosentry.dPort = clsTypeEntry.dPort;
								qosentry.dPortRangeMax = clsTypeEntry.dPortRangeMax;
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_DPORT;
#endif
								if (qosentry.protoType == PROTO_NONE)
									qosentry.protoType = PROTO_UDPTCP;
								break;
							case (1<<IP_QOS_CLASSFICATIONTYPE_TOS):
								only_ipv4 = 1;
								qosentry.tos = clsTypeEntry.tos;
#ifdef CONFIG_USER_AP_E8B_QOS
								qosentry.tos_end = clsTypeEntry.tos_end;
#endif
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_TOS;
#endif
								break;
							case (1<<IP_QOS_CLASSFICATIONTYPE_DSCP):
								only_ipv4 = 1;
								qosentry.qosDscp = clsTypeEntry.qosDscp;
								qosentry.qosDscp_end = clsTypeEntry.qosDscp_end;
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_DSCP;
#endif
								break;
#ifdef CONFIG_IPV6
							case (1<<IP_QOS_CLASSFICATIONTYPE_SIP6):
								only_ipv6 = 1;
								memcpy(qosentry.sip6,clsTypeEntry.sip6,sizeof(qosentry.sip6));
								qosentry.sip6PrefixLen = 128;
#if defined(CONFIG_USER_AP_E8B_QOS) || defined(CONFIG_CMCC_ENTERPRISE)
								memcpy(qosentry.sip6_end,clsTypeEntry.sip6_end,sizeof(qosentry.sip6_end));
								qosentry.sip6PrefixLen_end = 128;
#endif
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_SIP6;
#endif
								break;
							case (1<<IP_QOS_CLASSFICATIONTYPE_DIP6):								
								only_ipv6 = 1;
								memcpy(qosentry.dip6,clsTypeEntry.dip6,sizeof(qosentry.dip6));
								qosentry.dip6PrefixLen = 128;
#if defined(CONFIG_USER_AP_E8B_QOS) || defined(CONFIG_CMCC_ENTERPRISE)
								memcpy(qosentry.dip6_end,clsTypeEntry.dip6_end,sizeof(qosentry.dip6_end));
								qosentry.dip6PrefixLen_end = 128;
#endif
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_DIP6;
#endif
								break;
#ifndef CONFIG_USER_AP_CT_QOS
							case (1<<IP_QOS_CLASSFICATIONTYPE_SPORT6):
								only_ipv6 = 1;
								qosentry.sPort = clsTypeEntry.sPort6;
								qosentry.sPortRangeMax = clsTypeEntry.sPort6RangeMax;
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_SPORT6;
#endif
								if (qosentry.protoType == PROTO_NONE)
									qosentry.protoType = PROTO_UDPTCP;
								break;
							case (1<<IP_QOS_CLASSFICATIONTYPE_DPORT6):
								only_ipv6 = 1;
								qosentry.dPort = clsTypeEntry.dPort6;
								qosentry.dPortRangeMax = clsTypeEntry.dPort6RangeMax;
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_DPORT6;
#endif
								if (qosentry.protoType == PROTO_NONE)
									qosentry.protoType = PROTO_UDPTCP;
								break;
#endif
#ifdef CONFIG_USER_AP_E8B_QOS
							case (1<<IP_QOS_CLASSFICATIONTYPE_FlowLabel):
								only_ipv6 = 1;
								qosentry.flowlabel = clsTypeEntry.flowlabel;
								qosentry.flowlabel_end = clsTypeEntry.flowlabel_end;
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_FlowLabel;
#endif
								break;
#endif
							case (1<<IP_QOS_CLASSFICATIONTYPE_TrafficClass):
								only_ipv6 = 1;
#ifndef CONFIG_USER_AP_CT_QOS
								qosentry.qosDscp = clsTypeEntry.tc;
								qosentry.qosDscp_end = clsTypeEntry.tc_end;
#else
								qosentry.tos = clsTypeEntry.tc;
								qosentry.tos_end = clsTypeEntry.tc_end;
#endif
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_TrafficClass;
#endif
								break;
#endif
							case ((1<<IP_QOS_CLASSFICATIONTYPE_SPORT) |(1<<IP_QOS_CLASSFICATIONTYPE_SPORT6)):
								qosentry.sPort = clsTypeEntry.sPort;
								qosentry.sPortRangeMax = clsTypeEntry.sPortRangeMax;
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_SPORT;
#endif
								if (qosentry.protoType == PROTO_NONE)
									qosentry.protoType = PROTO_UDPTCP;
								break;
							case ((1<<IP_QOS_CLASSFICATIONTYPE_DPORT) |(1<<IP_QOS_CLASSFICATIONTYPE_DPORT6)):
								qosentry.dPort = clsTypeEntry.dPort;
								qosentry.dPortRangeMax = clsTypeEntry.dPortRangeMax;
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_DPORT;
#endif
								if (qosentry.protoType == PROTO_NONE)
									qosentry.protoType = PROTO_UDPTCP;
								break;
							case ((1<<IP_QOS_CLASSFICATIONTYPE_DSCP) |(1<<IP_QOS_CLASSFICATIONTYPE_TrafficClass)):
								qosentry.qosDscp = clsTypeEntry.qosDscp;
								qosentry.qosDscp_end = clsTypeEntry.qosDscp_end;
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_DSCP;
#endif
								break;
							case (1<<IP_QOS_CLASSFICATIONTYPE_WANINTERFACE):
								qosentry.outif = clsTypeEntry.wanitf;
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_WANINTERFACE;
#endif
								break;
							case (1<<IP_QOS_CLASSFICATIONTYPE_LANINTERFACE):
								qosentry.phyPort= clsTypeEntry.phyPort;
								qosentry.phyPort_end = clsTypeEntry.phyPort_end;
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_LANINTERFACE;
#endif
								break;
							case (1<<IP_QOS_CLASSFICATIONTYPE_ETHERTYPE):
								qosentry.ethType = clsTypeEntry.ethType;	
								if (qosentry.ethType == 0x0800)
								{
									only_ipv4 = 1;
									only_ipv6 = 0;
								}
								else if (qosentry.ethType == 0x86dd)
								{
									only_ipv4 = 0;
									only_ipv6 = 1;
								}
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_ETHERTYPE;
#endif
#ifndef CONFIG_CMCC_ENTERPRISE
								if (qosentry.protoType == PROTO_NONE)
									qosentry.ipqos_rule_type = IPQOS_ETHERTYPE_BASE;
#endif

								break;
#ifdef CONFIG_CMCC_ENTERPRISE
							case (1<<IP_QOS_CLASSFICATIONTYPE_VID):
								qosentry.vid= clsTypeEntry.vid;
								qosentry.vid_end = clsTypeEntry.vid_end;
#ifdef _PRMT_X_CT_COM_QOS_
								qosentry.cttypemap[j] = IP_QOS_CLASSFICATIONTYPE_VID;
#endif
								break;
#endif

							default:
								break;

							}
					#ifdef CONFIG_USER_AP_CT_QOS
						qosentry.IpProtocol = clsEntry.ipProtocol;
					#endif
					}
					
					// make sure that the protoType is not PROTO_NONE when there are sport/dport rules in qos entry
					if((qosentry.sPort || qosentry.dPort) && (qosentry.protoType==PROTO_NONE)){
						qosentry.protoType = PROTO_UDPTCP;
						printf("[%s:%d]Change protoType from PROTO_NONE to PROTO_UDPTCP because of sport/dport\n",__FUNCTION__,__LINE__);
					}
					// set IpVersion of qos entry for different cases  
					if(only_ipv4 && !only_ipv6)
						qosentry.IpProtocol = IPVER_IPV4;
					else if(only_ipv6 && !only_ipv4)
						qosentry.IpProtocol = IPVER_IPV6;
					else if(only_ipv4 && only_ipv6){ 
						printf("[%s:%d]Fail to set both ipv4/ipv6 elements in a single qos entry!\n",__FUNCTION__,__LINE__);
						return;
					}
					else{// default
						qosentry.IpProtocol = IPVER_IPV4_IPV6;
					}
					if(addQosRuleFlag){
						// tc mark for IPv6 packet
						if(qosentry.IpProtocol == 0 && qosentry.m_dscp > 0)
						{
							MIB_CE_IP_QOS_T entry_for_tc;
							memcpy(&entry_for_tc, &qosentry, sizeof(MIB_CE_IP_QOS_T));
							entry_for_tc.IpProtocol = IPVER_IPV6;
							mib_chain_add(MIB_IP_QOS_TBL, &entry_for_tc);
						}
						printf("%s %d add MIB_IP_QOS_TBL \n",__FUNCTION__,__LINE__);
#ifdef _PRMT_X_CT_COM_QOS_
						qosentry.modeTr69 = MODEOTHER;
#endif
						mib_chain_add(MIB_IP_QOS_TBL, &qosentry);
					}
				}
			break;
			default:
			break;
	}
	
}


int delQosClassficationTypeRule(int ifIndex){
	int totalClsTypeRuleNums=mib_chain_total(MIB_IP_QOS_CLASSFICATIONTYPE_TBL);
	int clsTypeRuleIdx = 0;
	for(clsTypeRuleIdx=(totalClsTypeRuleNums-1);clsTypeRuleIdx>=0;clsTypeRuleIdx--){
			MIB_CE_IP_QOS_CLASSFICATIONTYPE_T clsTypeEntry;
			mib_chain_get(MIB_IP_QOS_CLASSFICATIONTYPE_TBL,clsTypeRuleIdx,&clsTypeEntry);
			printf("clsTypeEntry.classficationType=%d clsTypeEntry.wanitf=%d ifIndex=%d\n",clsTypeEntry.classficationType,clsTypeEntry.wanitf,ifIndex);
			if((clsTypeEntry.classficationType==(1<<IP_QOS_CLASSFICATIONTYPE_WANINTERFACE))&&clsTypeEntry.wanitf==ifIndex){
				mib_chain_delete(MIB_IP_QOS_CLASSFICATIONTYPE_TBL,clsTypeRuleIdx);
				QosClassficationToQosRule(QOSCLASSFICATION_TO_QOSRULE_ACTION_MODIFY,clsTypeEntry.cls_id);
			}
		
	}			
	return 0;
}

int syncQosClassficationTypeRule()
{
	int totalClsTypeRuleNums=mib_chain_total(MIB_IP_QOS_CLASSFICATIONTYPE_TBL);
	int clsTypeRuleIdx = 0;
	
	for(clsTypeRuleIdx=0; clsTypeRuleIdx<totalClsTypeRuleNums; clsTypeRuleIdx++){
		MIB_CE_IP_QOS_CLASSFICATIONTYPE_T clsTypeEntry;
		mib_chain_get(MIB_IP_QOS_CLASSFICATIONTYPE_TBL,clsTypeRuleIdx,&clsTypeEntry);
		QosClassficationToQosRule(QOSCLASSFICATION_TO_QOSRULE_ACTION_MODIFY,clsTypeEntry.cls_id);
	}	
	return 0;
}
#endif

//20080825 add by ramen to fix the special char in the html
char * fixSpecialChar(char *str,char *srcstr,int length)
{
   int i=0,j=0;
   if(!str||!srcstr)  return NULL;
   int strlength=strlen(srcstr);
   if(length<strlength) return NULL;
   char *tempstr=(char*)malloc(strlength*2);
   memset(tempstr,0,2*strlength);
    for(i=0,j=0;i<=strlength;i++)
   	{
   	  if(srcstr[i]=='"'||srcstr[i]=='\\')
   	  	{
   	  	   	  	tempstr[j++]='\\';
		}
	  	tempstr[j++]=srcstr[i];
   	}
   tempstr[j++]='\0';
   if(j>length) goto ERR;
    memcpy(str,tempstr,j);
   ERR:
    free(tempstr);
    return str;
}

/* mac to string function */
int hex(unsigned char ch)
{
	if (ch >= 'a' && ch <= 'f')
		return ch-'a'+10;
	if (ch >= '0' && ch <= '9')
		return ch-'0';
	if (ch >= 'A' && ch <= 'F')
		return ch-'A'+10;
	return -1;
}

//mac's size must be >=17, content format:"00-16-76-D9-A6-AF"
#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
void convert_mac(char *mac)
{
	char temp[ETH_ALEN];
	int i;
	for(i = 0; i < ETH_ALEN; i++) {
		temp[i] = hex(mac[i * 3]) * 16 + hex(mac[i * 3 + 1]);
	}
	memcpy(mac, temp, ETH_ALEN);
}

void changeMacFormat(char *str, char s, char d)
{
    int i=0;
    int length=strlen(str);
    for (i=0;i<length;i++)
    {
        if (str[i]==s) str[i]=d;
    }
}

void changeMacStrToLower(char *str)
{
	char buff[MAC_ADDR_STRING_LEN]={0}, temp=0;
	int i=0;
	strncpy(buff,str,MAC_ADDR_STRING_LEN-1);

	while(buff[i])
	{
		if(isupper(buff[i])){
			temp=tolower(buff[i]);
			buff[i]=temp;
		}
		i++;
	}
	strncpy(str,buff,MAC_ADDR_STRING_LEN-1);
}

#if defined(WIFI_TIMER_SCHEDULE) || defined(LED_TIMER) || defined(SLEEP_TIMER) || defined(SUPPORT_TIME_SCHEDULE)

#define SSID_INTERFACE_SHIFT_BIT 16

int _crond_check_current_state(int startHour, int startMin, int endHour, int endMin)
{
	time_t curTime;
	struct tm curTm;
	int curState = 0;

	time(&curTime);
	memcpy(&curTm, localtime(&curTime), sizeof(curTm));

	//today time
	if(((endHour - startHour) > 0) ||
		(((endHour - startHour) == 0) && ((endMin - startMin) > 0))
	) {
	    	if(startHour < curTm.tm_hour && endHour > curTm.tm_hour)
		    	curState = 1;
		else if ((startHour == curTm.tm_hour) && (startMin <= curTm.tm_min) && (endHour > curTm.tm_hour))
		    	curState = 1;
		else if ((startHour < curTm.tm_hour) && (endHour == curTm.tm_hour) && (endMin >= curTm.tm_min))
		    	curState = 1;
		else if ((startHour == curTm.tm_hour) && (startMin <= curTm.tm_min) && (endHour == curTm.tm_hour) && (endMin >=curTm.tm_min))
		    	curState = 1;
		else
			curState = 0;
	}
	//future time
	else if(((endHour - startHour) < 0) ||
		(((endHour - startHour) == 0) && ((endMin-startMin) < 0))
	) {
		if(startHour > curTm.tm_hour && endHour < curTm.tm_hour)
		    	curState = 0;
		else if ((startHour == curTm.tm_hour) && (startMin >= curTm.tm_min) && (endHour < curTm.tm_hour))
		    	curState = 0;
		else if ((startHour > curTm.tm_hour) && (endHour == curTm.tm_hour) && (endMin <= curTm.tm_min))
		    	curState = 0;
		else if ((startHour == curTm.tm_hour) && (startMin >= curTm.tm_min) && (endHour == curTm.tm_hour) && (endMin <= curTm.tm_min))
		    	curState = 0;
		else 
		    	curState = 1;
	}

	return curState;
}

void updateScheduleCrondFile_time_change(int signum)
{
	updateScheduleCrondFile("/var/spool/cron/crontabs", 0);
}
#endif

int disablePortBandWidthControl(void)
{
	int i, phyPortId;
	return 0;
}

int applyPortBandWidthControl(void)
{
	unsigned char enable;
	int i, total;
	int phyPortId;
	unsigned int portmask = 0;
	MIB_CE_PORT_BANDWIDTH_ENTRY_T entry;

	if(0==mib_get_s(MIB_PORT_BANDWIDTH_CONTROL_ENABLE, &enable, sizeof(enable)))
	{
		printf("[%s %d]Get MIB_PORT_BANDWIDTH_CONTROL_ENABLE failed.\n", __func__, __LINE__);
		return -1;
	}

	if(0==enable)
	{
		printf("[%s %d]PORT_BANDWIDTH_CONTROL is disabled.\n", __func__, __LINE__);
		disablePortBandWidthControl();
		return 0;
	}

	total = mib_chain_total(MIB_PORT_BANDWIDTH_TBL);
	for(i=0; i<total; i++)
	{
		if(0==mib_chain_get(MIB_PORT_BANDWIDTH_TBL, i, (void*)&entry))
		{
			continue;
		}
		if(entry.port>=SW_LAN_PORT_NUM)
			continue;
		portmask |= (1<<entry.port);
	}

	for(i=0; i<SW_LAN_PORT_NUM; i++)
	{
		if(0==(portmask&(1<<i)))
		{
		}
	}

	return 0;
}

#if defined(CONFIG_GPON_FEATURE)
void RTK_Gpon_Get_Fec(uint32_t* ds_fec_status, uint32_t* us_fec_status)
{
#if defined(CONFIG_COMMON_RT_API)
	rt_gpon_fec_status_t fec_status;

	rt_gpon_fec_get(&fec_status);
#else
	rtk_gpon_fec_status_t fec_status;

	rtk_gpon_fec_get(&fec_status);
#endif
	*ds_fec_status = fec_status.ds_fec_status;
	*us_fec_status = fec_status.us_fec_status;
}
#endif

void SaveLOIDReg()
{
	unsigned char loid[MAX_NAME_LEN];
	unsigned char password[MAX_LOID_PW_LEN];
	unsigned char old_loid[MAX_NAME_LEN];
	unsigned char old_password[MAX_LOID_PW_LEN];
	int changed = 0;

	mib_get_s(MIB_LOID, loid, sizeof(loid));
	mib_get_s(MIB_LOID_OLD, old_loid, sizeof(old_loid));
	if(strcmp(loid, old_loid) != 0)
	{
		mib_set(MIB_LOID_OLD, loid);
		changed = 1;
	}
	mib_get_s(MIB_LOID_PASSWD, password, sizeof(password));
	mib_get_s(MIB_LOID_PASSWD_OLD, old_password, sizeof(old_password));
	if(strcmp(password, old_password) != 0 || changed)
	{
		mib_set(MIB_LOID_PASSWD_OLD, password);
		#if defined(CONFIG_GPON_FEATURE) && (defined(CONFIG_CMCC) || defined(CONFIG_CU))
		mib_set(MIB_GPON_PLOAM_PASSWD, password);
		#endif
		changed = 1;
	}

#ifdef COMMIT_IMMEDIATELY
#if !defined(CONFIG_CMCC) && !defined(CONFIG_CU_BASEON_CMCC)
	if(changed)
#endif
		Commit();
#endif
}

void get_dns_by_wan(MIB_CE_ATM_VC_T *pEntry, char *dns1, char *dns2)
{
	unsigned char zero[IP_ADDR_LEN] = {0};

	if ( ((pEntry->cmode == CHANNEL_MODE_RT1483) || (pEntry->cmode == CHANNEL_MODE_IPOE)) && (pEntry->dnsMode == REQUEST_DNS_NONE) )
	{
		//manual
		if(memcmp(zero, pEntry->v4dns1, IP_ADDR_LEN) != 0)
		{
			//v4dns1 is already in network byte order
			inet_ntop(AF_INET, pEntry->v4dns1, dns1, INET_ADDRSTRLEN);
		}
		if(memcmp(zero, pEntry->v4dns2, IP_ADDR_LEN) != 0)
		{
			//v4dns2 is already in network byte order
			inet_ntop(AF_INET, pEntry->v4dns2, dns2, INET_ADDRSTRLEN);
		}
	}
	else
	{
		FILE *infdns = NULL;
		char line[128] = {0};
		char ifname[IFNAMSIZ] = {0};

		ifGetName(pEntry->ifIndex,ifname,sizeof(ifname));
		if ((DHCP_T)pEntry->ipDhcp == DHCP_CLIENT)
			snprintf(line, 64, "%s.%s", (char *)DNS_RESOLV, ifname);
		if (pEntry->cmode == CHANNEL_MODE_PPPOE || pEntry->cmode == CHANNEL_MODE_PPPOA)
			snprintf(line, 64, "%s.%s", (char *)PPP_RESOLV, ifname);

		infdns=fopen(line,"r");
		if(infdns)
		{
			char *p = NULL;
			int cnt = 0;

			while(fgets(line,sizeof(line),infdns))
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
			fclose(infdns);
		}
	}
}

int getCPUClass(char *buf, int len)
{
	unsigned int chipId, rev, subType;
	
	if(buf == NULL)
		return -1;

	rtk_switch_version_get(&chipId, &rev, &subType);
	//printf("%s:%d subType %u\n", __FUNCTION__, __LINE__, subType);
	switch(chipId)
	{
		case RTL9601B_CHIP_ID:
			snprintf(buf, len, "Realtek RTL9601B");
			break;
		case RTL9602C_CHIP_ID:
			snprintf(buf, len, "Realtek RTL9602C");
			break;
		case RTL9607C_CHIP_ID:
			switch(subType)
			{
			case RTL9607C_CHIP_SUB_TYPE_RTL9607CP:
				snprintf(buf, len, "Realtek RTL9607CP");	
				break;
			case 0x10:
				snprintf(buf, len, "Realtek RTL9603CP");
				break;
			case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA4:
			case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA5:
			case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA6:
				snprintf(buf, len, "Realtek RTL9603C");
				break;
			case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA6:
			case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA7:
			default:
				snprintf(buf, len, "Realtek RTL9607C");
				break;
			}
			break;
		case APOLLOMP_CHIP_ID:
			snprintf(buf, len, "Realtek RTL%04X", chipId>>16);
			break;
		case LUNA_G3_CHIP_ID:
			snprintf(buf, len, "Realtek LUNA G3");
			break;
		case CA8279_CHIP_ID:
			snprintf(buf, len, "Realtek CA8279");
			break;
		case CA8277B_CHIP_ID:
			switch(subType)
			{
			case CA8277B_CHIP_SUB_TYPE_CA8277B:
				snprintf(buf, len, "Realtek CA8277B");	
				break;
			case CA8277B_CHIP_SUB_TYPE_RTL9617B:
				snprintf(buf, len, "Realtek RTL9617B");	
				break;
			case CA8277B_CHIP_SUB_TYPE_RTL9607DA:
				snprintf(buf, len, "Realtek RTL9607DA");	
				break;
			case CA8277B_CHIP_SUB_TYPE_RTL8198XA:
				snprintf(buf, len, "Realtek RTL8198XA");	
				break;
			case CA8277B_CHIP_SUB_TYPE_RTL9619:
				snprintf(buf, len, "Realtek RTL9619");	
				break;
			case CA8277B_CHIP_SUB_TYPE_RTL9619B:
				snprintf(buf, len, "Realtek RTL9619B");	
				break;
			default:
				snprintf(buf, len, "Realtek RTL9617B");
				break;
			}
			break;
		case RTL8277C_CHIP_ID:
			switch(subType)
			{
			case RTL8277C_CHIP_SUB_TYPE_RTL8277C:
				snprintf(buf, len, "Realtek RTL8277C");	
				break;
			case RTL8277C_CHIP_SUB_TYPE_RTL9617C:
				snprintf(buf, len, "Realtek RTL9617C");	
				break;
			case RTL8277C_CHIP_SUB_TYPE_RTL9607DQ:
				snprintf(buf, len, "Realtek RTL9607DQ");	
				break;
			case RTL8277C_CHIP_SUB_TYPE_RTL8198XB:
				snprintf(buf, len, "Realtek RTL8198XB");	
				break;
			case RTL8277C_CHIP_SUB_TYPE_RTL9619:
				snprintf(buf, len, "Realtek RTL9619");	
				break;
			case RTL8277C_CHIP_SUB_TYPE_RTL9619C:
				snprintf(buf, len, "Realtek RTL9619C");	
				break;
			case RTL8277C_CHIP_SUB_TYPE_RTL9607DM:
				snprintf(buf, len, "Realtek RTL9607DM");	
				break;
			case RTL8277C_CHIP_SUB_TYPE_RTL9617CM:
				snprintf(buf, len, "Realtek RTL9617CM");
				break;
			default:
				snprintf(buf, len, "Realtek RTL9617C");
				break;
			}
			break;
		default:
			return -1;
	}

	return 0;
}

int getCPUTemperature(int *cpuTemp)
{
	if(NULL == cpuTemp)
	{
		return -1;		
	}
	
	int TempIntger = 0;
	int TempDecimal = 0;
	rtk_switch_thermal_get(&TempIntger, &TempDecimal);

	*cpuTemp = TempIntger;
	
	return 0;
}

void changeStringToMac(unsigned char *mac, unsigned char *macString)
{
	unsigned int mac0, mac1, mac2, mac3, mac4, mac5;
	if( (mac==NULL)||(macString==NULL) )
		return;
	sscanf(macString, "%2x-%2x-%2x-%2x-%2x-%2x",&mac0,&mac1,&mac2,&mac3,&mac4,&mac5);

	mac[0] = (unsigned char)mac0;
	mac[1] = (unsigned char)mac1;
	mac[2] = (unsigned char)mac2;
	mac[3] = (unsigned char)mac3;
	mac[4] = (unsigned char)mac4;
	mac[5] = (unsigned char)mac5;
}

unsigned int isMacAddrInvalid(unsigned char *pMac)
{
	if (pMac == NULL)
		return 1;

	if(pMac[0]&0x01)
		return 1;

	if(0x00 == (pMac[0]|pMac[1]|pMac[2]|pMac[3]|pMac[4]|pMac[5]) )
		return 1;

	return 0;
}

#ifdef CONFIG_USER_LANNETINFO
char rtk_lanhost_get_host_index_by_mac(char *mac);

#include <sys/msg.h>
/*
 * FUNCTION:get new and leave device info
 *
 * INPUT:
 *	pNewDeviceInfo:
 *		target lanHostInfo_t array to store new device info
 *
 * RETURN:
 *	 0 -- successful
 *	-1 -- Input error
 *  -2 -- get msg queue error
 *  -3 -- lanNetInfo process not found
 *	-4 -- other
 */
int get_new_and_leave_device_info(lanHostInfo_t *pLanDeviceInfo, int num)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret, size;

	if(pLanDeviceInfo == NULL)
		return -1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_NEW_DEVICE_INFO_GET|CMD_LEAVE_DEVICE_INFO_GET;

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	memset(&qbuf, 0, sizeof(struct lanNetInfoMsg));
	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{	
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			//printf("[%s %d]:receive new lan net info successfully.\n", __func__, __LINE__);
			size = (sizeof(lanHostInfo_t)*num >= MAX_DEVICE_INFO_SIZE)? MAX_DEVICE_INFO_SIZE:(sizeof(lanHostInfo_t)*num);
			memcpy(pLanDeviceInfo, mymsg->mtext, size);
			return 0;
		}
	}

	printf("[%s %d]: Get new and leave device info failed:ret=%d!\n", __func__, __LINE__, ret);
	return -4;
}

/*
 * FUNCTION:SET_ATTCH_DEVICE_NAME
 *
 * INPUT:
 *	macAddr:
 *		target device's MAC address
 *	pDevName:
 *		devName to set
 *
 * RETURN:
 *	 0 -- successful
 *	-1 -- Input error
 *  -2 -- device not found
 *  -3 -- other
 */
int set_attach_device_name(unsigned char *pMacAddr, char *pDevName)
{
	int i, totalNum;
	MIB_LAN_DEV_NAME_T entry;

	if( (pMacAddr==NULL)||(pDevName==NULL) )
		return -1;

	if( ( totalNum = mib_chain_total(MIB_LAN_DEV_NAME_TBL) ) <= 0 )
	{
		printf("Lan host mib table is empty.\n");
		goto add;
	}

	memset(&entry, 0, sizeof(MIB_LAN_DEV_NAME_T));
	for(i=0; i<totalNum; i++)
	{
		if(!mib_chain_get(MIB_LAN_DEV_NAME_TBL,i, (void*)&entry))
			continue;

		if( !macAddrCmp(pMacAddr, entry.mac))
			break;
	}


	if( i!=totalNum )
	{
		strncpy(entry.devName, pDevName, MAX_LANNET_DEV_NAME_LENGTH-1);
		entry.devName[MAX_LANNET_DEV_NAME_LENGTH-1] = 0;
		if(!mib_chain_update(MIB_LAN_DEV_NAME_TBL,(void*)&entry, i))
			return -3;
		goto end;
	}

add:
	memcpy(entry.mac, pMacAddr, MAC_ADDR_LEN);
	strncpy(entry.devName, pDevName, MAX_LANNET_DEV_NAME_LENGTH-1);
	entry.devName[MAX_LANNET_DEV_NAME_LENGTH-1] = 0;
	if(!mib_chain_add(MIB_LAN_DEV_NAME_TBL,(void*)&entry))
		return -3;
end:
	return 0;
}

int get_attach_device_name(unsigned char *pMacAddr, char *pDevName)
{
	int i, totalNum;
	MIB_LAN_DEV_NAME_T entry;

	if( (pMacAddr==NULL)||(pDevName==NULL) )
		return -1;

	if( ( totalNum = mib_chain_total(MIB_LAN_DEV_NAME_TBL) ) <= 0 )
	{
		return -2;
	}

	memset(&entry, 0, sizeof(MIB_LAN_DEV_NAME_T));
	for(i=0; i<totalNum; i++)
	{
		if(!mib_chain_get(MIB_LAN_DEV_NAME_TBL,i, (void*)&entry))
			continue;

		if( !macAddrCmp(pMacAddr, entry.mac))
			break;
	}

	if(i!=totalNum)
	{// find
		if(strlen(entry.devName) == 0){
			return -2;
		}
		strncpy(pDevName, entry.devName, MAX_LANNET_DEV_NAME_LENGTH-1);
		pDevName[MAX_LANNET_DEV_NAME_LENGTH-1] = 0;
		return 0;
	}

	return -2;
}

#ifdef CONFIG_CU
int set_attach_device_type(unsigned char *pMacAddr, char *pDevType)
{
	int i, totalNum;
	MIB_LAN_DEV_NAME_T entry;

	if( (pMacAddr==NULL)||(pDevType==NULL) )
		return -1;

	if( ( totalNum = mib_chain_total(MIB_LAN_DEV_NAME_TBL) ) <= 0 )
	{
		goto add;
	}
	memset(&entry, 0, sizeof(MIB_LAN_DEV_NAME_T));
	for(i=0; i<totalNum; i++)
	{
		if(!mib_chain_get(MIB_LAN_DEV_NAME_TBL,i, (void*)&entry))
			continue;

		if( !macAddrCmp(pMacAddr, entry.mac))
			break;
	}

	if( i!=totalNum )
	{
		strncpy(entry.devType, pDevType, MAX_LANNET_DEV_NAME_LENGTH-1);
		entry.devType[MAX_LANNET_DEV_NAME_LENGTH-1] = 0;
		if(!mib_chain_update(MIB_LAN_DEV_NAME_TBL,(void*)&entry, i))
			return -3;
		goto end;
	}

add:
	memcpy(entry.mac, pMacAddr, MAC_ADDR_LEN);
	strncpy(entry.devType, pDevType, MAX_LANNET_DEV_NAME_LENGTH-1);
	entry.devType[MAX_LANNET_DEV_NAME_LENGTH-1] = 0;
	if(!mib_chain_add(MIB_LAN_DEV_NAME_TBL,(void*)&entry))
		return -3;
end:
	return 0;
}

int get_attach_device_type(unsigned char *pMacAddr, char *pDevType)
{
	int i, totalNum;
	MIB_LAN_DEV_NAME_T entry;

	if( (pMacAddr==NULL)||(pDevType==NULL) )
		return -1;

	if( ( totalNum = mib_chain_total(MIB_LAN_DEV_NAME_TBL) ) <= 0 )
	{
		return -2;
	}

	memset(&entry, 0, sizeof(MIB_LAN_DEV_NAME_T));
	for(i=0; i<totalNum; i++)
	{
		if(!mib_chain_get(MIB_LAN_DEV_NAME_TBL,i, (void*)&entry))
			continue;

		if( !macAddrCmp(pMacAddr, entry.mac))
			break;
	}

	if(i!=totalNum)
	{// find
		strncpy(pDevType, entry.devType, MAX_LANNET_DEV_NAME_LENGTH-1);
		pDevType[MAX_LANNET_DEV_NAME_LENGTH-1] = 0;
		return 0;
	}

	return -2;
}
#endif

int get_hg_dev_name(hgDevInfo_t **ppHGDevData, unsigned int *pCount)
{
	int32 ret, i, size=0;
	lanHostInfo_t *pLANHostData = NULL;
	hgDevInfo_t *pHGDevData =NULL;

	if((ppHGDevData==NULL)||(pCount == NULL))
		return -1;

	/* should get lan Client table */
	ret = get_lan_net_info(&pLANHostData, pCount);
	if(ret)
		return ret;
	++(*pCount);

	size = (*pCount) * sizeof(hgDevInfo_t);
	pHGDevData = (hgDevInfo_t *)malloc( size );
	if(pHGDevData==NULL)
	{
		if(pLANHostData) free(pLANHostData);
		return -3;
	}
	*ppHGDevData = pHGDevData;

	memset(pHGDevData, 0, size);
	/* get gateway MacAddr */
	if ( !mib_get_s(MIB_ELAN_MAC_ADDR, (void *)pHGDevData[0].mac, sizeof(pHGDevData[0].mac)))
	{
		free(pHGDevData);
		if(pLANHostData) free(pLANHostData);
		return -3;
	}

	/* get gateway DevName */
	strcpy(pHGDevData[0].devName, DEVICE_MODEL_STR);

	/* get all lan dev name */
	for(i=1; i<(*pCount);i++)
	{
			memcpy(pHGDevData[i].mac, pLANHostData[i-1].mac, MAC_ADDR_LEN);
			strncpy(pHGDevData[i].devName, pLANHostData[i-1].devName, MAX_LANNET_DEV_NAME_LENGTH-1);
			pHGDevData[i].devName[MAX_LANNET_DEV_NAME_LENGTH-1]=0;

			get_attach_device_name(pHGDevData[i].mac, pHGDevData[i].devName);
	}

	free(pLANHostData);
	return 0;
}

int set_lanhost_max_number(unsigned int number)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;
	unsigned char max_lanHost_number = number;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_MAX_NUMBER_SET;

	mymsg->arg1 = number;

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			mib_set( MIB_MAX_LAN_HOST_NUM, (void *)&max_lanHost_number);
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}

int get_lanhost_max_number(unsigned int * number)
{
	unsigned int max_lanHost_number;
	if(0 == mib_get_s( MIB_MAX_LAN_HOST_NUM, (void *)&max_lanHost_number, sizeof(max_lanHost_number)))
		return -1;

	*number = max_lanHost_number;
	return 0;
}

int get_lanhost_number(unsigned int * number)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	if(number == NULL)
		return -1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_NUMBER_GET;

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			//printf("[%s %d]: [%d] successfully.\n", __func__, __LINE__, mymsg->arg1);
			*number = mymsg->arg1;
			return 0;
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}

int set_controllist_max_number(unsigned int number)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;
	unsigned int max_controlList_number = number;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CONTROL_LIST_MAX_NUMBER_SET;

	mymsg->arg1 = number;

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			mib_set( MIB_MAX_CONTROL_LIST_NUM, (void *)&max_controlList_number);
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}

int get_controllist_max_number(unsigned int * number)
{
	unsigned int max_controlList_number;
	if(0 == mib_get_s( MIB_MAX_CONTROL_LIST_NUM, (void *)&max_controlList_number, sizeof(max_controlList_number)))
		return -1;

	*number = max_controlList_number;
	return 0;
}

int get_controllist_number(unsigned int * number)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	if(number == NULL)
		return -1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_CONTROL_LIST_NUMBER_GET;

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			*number = mymsg->arg1;
			return 0;
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}

int set_lanhost_control_status(unsigned char *pMacAddr, unsigned char controlStatus)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_CONTROL_STATUS_SET;

	mymsg->arg1 = controlStatus;
	memcpy(mymsg->mtext, pMacAddr, MAC_ADDR_LEN);

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

int get_lanhost_control_status(unsigned char *pMacAddr, unsigned char * controlStatus)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_CONTROL_STATUS_GET;

	memcpy(mymsg->mtext, pMacAddr, MAC_ADDR_LEN);

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			*controlStatus = mymsg->arg1 ;
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}

int set_lanhost_access_right(unsigned char *pMacAddr, unsigned char internetAccessRight, unsigned char storageAccessRight)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_ACCESS_RIGHT_SET;

	mymsg->arg1 = internetAccessRight;
	mymsg->arg2 = storageAccessRight;
	memcpy(mymsg->mtext, pMacAddr, MAC_ADDR_LEN);

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
#ifdef SUPPORT_ACCESS_RIGHT
			return set_attach_device_right(pMacAddr, internetAccessRight, storageAccessRight);
#else
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
#endif
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}

int get_lanhost_access_right(unsigned char *pMacAddr, unsigned char * internetAccessRight, unsigned char * storageAccessRight)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_ACCESS_RIGHT_GET;

	memcpy(mymsg->mtext, pMacAddr, MAC_ADDR_LEN);

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			*internetAccessRight = mymsg->arg1;
			*storageAccessRight = mymsg->arg2;
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}

int set_lanhost_device_type(unsigned char *pMacAddr, unsigned char devType)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_DEVICE_TYPE_SET;

	memcpy(mymsg->mtext, pMacAddr, MAC_ADDR_LEN);
	mymsg->arg1 = devType;

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

int get_lanhost_device_type(unsigned char *pMacAddr, unsigned char *devType)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_DEVICE_TYPE_GET;

	if(devType==NULL)
		return -1;

	memcpy(mymsg->mtext, pMacAddr, MAC_ADDR_LEN);

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			*devType = mymsg->arg1;
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}

int set_lanhost_brand(unsigned char *pMacAddr, unsigned char *brand)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_BRAND_SET;

	if(brand==NULL)
		return -1;

	memcpy(mymsg->mtext, pMacAddr, MAC_ADDR_LEN);
	strncpy(mymsg->mtext+MAC_ADDR_LEN, brand, MAX_DEVICE_INFO_SIZE-MAC_ADDR_LEN);
	mymsg->mtext[MAX_DEVICE_INFO_SIZE-1] = '\0';

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

int get_lanhost_brand(unsigned char *pMacAddr, unsigned char *brand)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_BRAND_GET;

	if(brand==NULL)
		return -1;

	memcpy(mymsg->mtext, pMacAddr, MAC_ADDR_LEN);

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			strcpy(brand, mymsg->mtext+MAC_ADDR_LEN);
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;

}

int set_lanhost_model(unsigned char *pMacAddr, unsigned char *model)
{
			struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_MODEL_SET;

	if(model==NULL)
		return -1;

	memcpy(mymsg->mtext, pMacAddr, MAC_ADDR_LEN);
	strncpy(mymsg->mtext+MAC_ADDR_LEN, model, MAX_DEVICE_INFO_SIZE-MAC_ADDR_LEN);
	mymsg->mtext[MAX_DEVICE_INFO_SIZE-1] = '\0';

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

int get_lanhost_model(unsigned char *pMacAddr, unsigned char *model)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_MODEL_GET;

	if(model==NULL)
		return -1;

	memcpy(mymsg->mtext, pMacAddr, MAC_ADDR_LEN);

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			strcpy(model, mymsg->mtext+MAC_ADDR_LEN);
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;
}

int set_lanhost_OS(unsigned char *pMacAddr, unsigned char *os)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_OS_SET;

	if(os==NULL)
		return -1;

	memcpy(mymsg->mtext, pMacAddr, MAC_ADDR_LEN);
	strncpy(mymsg->mtext+MAC_ADDR_LEN, os, MAX_DEVICE_INFO_SIZE-MAC_ADDR_LEN);
	mymsg->mtext[MAX_DEVICE_INFO_SIZE-1] = '\0';

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

int get_lanhost_OS(unsigned char *pMacAddr, unsigned char *os)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_OS_GET;

	if(os==NULL)
		return -1;

	memcpy(mymsg->mtext, pMacAddr, MAC_ADDR_LEN);

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			strcpy(os, mymsg->mtext+MAC_ADDR_LEN);
			//printf("[%s %d]: successfully.\n", __func__, __LINE__);
			return 0;
		}
	}

	printf("[%s %d]: failed!\n", __func__, __LINE__);
	return -4;

}

/****************************************************************************/

int get_lanhost_information_change(lanHostInfo_t *pLanDeviceInfo, int num)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret, size=0;

	if(pLanDeviceInfo == NULL)
		return -1;

	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_LAN_HOST_INFORMATION_CHANGE_GET;

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	memset(&qbuf, 0, sizeof(struct lanNetInfoMsg));
	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			size = ( (sizeof(lanHostInfo_t)*num) >= MAX_DEVICE_INFO_SIZE )? MAX_DEVICE_INFO_SIZE:(sizeof(lanHostInfo_t)*num);
			memcpy(pLanDeviceInfo, mymsg->mtext, size);
			return 0;
		}
	}

	printf("[%s %d]: Get lan host change information failed!\n", __func__, __LINE__);
	return -4;
}

int get_online_and_offline_device_info(lanHostInfo_t **ppLANNetInfoData, unsigned int *pCount)
{
	struct lanNetInfoMsg qbuf;
	LANNETINFO_MSG_T *mymsg;
	int  ret, size, i, count=0;
	lanHostInfo_t *pLANNetInfoData = NULL;

	if( (ppLANNetInfoData==NULL)||(pCount==NULL) )
		return -1;
	
	mymsg = &qbuf.msg;
	mymsg->cmd = CMD_NEW_DEVICE_INFO_GET|CMD_LEAVE_DEVICE_INFO_GET;

	ret = sendMessageToLanNetInfo(&qbuf.msg);
	if(ret != 0)
		return ret;

	memset(&qbuf, 0, sizeof(struct lanNetInfoMsg));
	ret = readMessageFromLanNetInfo(&qbuf);
	if(ret != -1)
	{	
		if (qbuf.request == LANNETINFO_MSG_SUCC) {
			size = sizeof(lanHostInfo_t)*5;
			pLANNetInfoData = (lanHostInfo_t *) malloc(size);

			if(pLANNetInfoData == NULL)
				return -3;

			*ppLANNetInfoData = pLANNetInfoData;
			memset(pLANNetInfoData, 0, size);
			memcpy(pLANNetInfoData, mymsg->mtext, size);

			for(i=0; i<5; i++)
			{
				if(isZeroMac(pLANNetInfoData[i].mac) == 0)
					count++;
			}

			*pCount=count;
			return 0;
			
		}
	}

	printf("[%s %d]: Get new and leave device info failed:ret=%d!\n", __func__, __LINE__, ret);
	return -4;
}
void rtk_e8_mib_set_signal_to_lannetinfo(int ret, int id)
{
	int lanNetInfoPid=-1;
	if(ret != 0 && (id == MIB_LANNETINFO_FLAG || id == MIB_MAX_LAN_HOST_NUM || id == MIB_MAX_CONTROL_LIST_NUM
#ifdef CONFIG_USER_LAN_BANDWIDTH_MONITOR
		|| id == MIB_LANHOST_BANDWIDTH_MONITOR_ENABLE
#endif
#ifdef CONFIG_CU
		|| id == MIB_POWER_TEST_START
#endif
		))
	{
		lanNetInfoPid = read_pid("/var/run/lannetinfo.pid");
		if(lanNetInfoPid > 0) kill(lanNetInfoPid, SIGUSR1);
	}
}

/****************************************************************************/
#endif //#ifdef CONFIG_USER_LANNETINFO

#ifdef CTC_DNS_TUNNEL
static int find_dns_tunnel(MIB_CE_DNS_TUNNEL_T *target)
{
	int idx = -1;
	int total, i;
	MIB_CE_DNS_TUNNEL_T entry;

	if(target == NULL)
		return -1;

	total = mib_chain_total(MIB_DNS_TUNNEL_TBL);

	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(MIB_DNS_TUNNEL_TBL, i, &entry) == 0)
			continue;

		if(memcmp(target->server_ip, entry.server_ip, sizeof(entry.server_ip)) == 0
			&& strcmp(target->domain, entry.domain) == 0)
		{
			idx = i;
			break;
		}
	}

	return idx;
}

/*
 * FUNCTION:ATTACH_WAN_DNSTUNNEL
 *
 * INPUT:
 *	tunnel:
 *		Pointer of DNS tunnel to attach. Only accept one node at a time.
 *
 * RETURN:
 *	0 -- successful
 *	-1 -- Input error
 *  -2 -- out of memory
 *  -3 -- other
 */
int attach_wan_dns_tunnel(dns_tunnel_node_t *node)
{
	MIB_CE_DNS_TUNNEL_T entry;
	struct sockaddr_in sa;	// used to check IP
	char *domain = NULL, *save_ptr = NULL, *buf;

	if(node == NULL || node->server_ip == NULL || node->domain == NULL)
		return -1;

	/* Check server_ip is a valid IP */
	if(inet_pton(AF_INET, node->server_ip, &(sa.sin_addr)) != 1)
		return -1;

	memcpy(entry.server_ip, &(sa.sin_addr), sizeof(entry.server_ip));

	buf = strdup(node->domain);
	if(buf == NULL)
		return -2;

	domain = strtok_r(buf, "; ", &save_ptr);
	while(domain != NULL)
	{
		memset(entry.domain, 0, sizeof(entry.domain));
		strcpy(entry.domain, domain);
		if(find_dns_tunnel(&entry) < 0)
		{
			mib_chain_add(MIB_DNS_TUNNEL_TBL, &entry);
			//fprintf(stderr, "<%s:%d> added domain %s\n", __func__, __LINE__, domain);
		}
		domain = strtok_r(NULL, "; ", &save_ptr);
	}


	Commit();
	// update dnsmasq config
	restart_dnsrelay();

	free(buf);
	return 0;
}

static void clear_dns_tunnel_by_domain(char *domain)
{
	int total, i;
	MIB_CE_DNS_TUNNEL_T entry;

	if(domain == NULL)
		return;

	total = mib_chain_total(MIB_DNS_TUNNEL_TBL);

	for(i = total-1 ; i >= 0; i--)
	{
		if(mib_chain_get(MIB_DNS_TUNNEL_TBL, i, &entry) == 0)
			continue;

		if(strcmp(domain, entry.domain) == 0)
			mib_chain_delete(MIB_DNS_TUNNEL_TBL, i);
	}
}

/*
 * FUNCTION: DETACH_WAN_DNSTUNNEL
 *
 * INPUT:
 *	tunnel:
 *		Pointer of DNS tunnel to dettach, only support one node at a time.
 *
 * RETURN:
 *	0 -- successful
 *	-1 -- Input error
 *  -2 -- out of memory
 *  -3 -- other
 */
int detach_wan_dns_tunnel(dns_tunnel_node_t *node)
{
	MIB_CE_DNS_TUNNEL_T entry;
	struct sockaddr_in sa;	// used to check IP
	char *domain = NULL, *save_ptr = NULL, *buf = NULL;

	if(node == NULL || node->server_ip == NULL)
		return -1;

	if(strcmp(node->server_ip, "all") == 0)
	{
		mib_chain_clear(MIB_DNS_TUNNEL_TBL);
	}
	else if(strcmp(node->server_ip, "other") == 0)
	{
		if(node->domain == NULL)
			return -1;

		buf = strdup(node->domain);
		if(buf == NULL)
			return -2;

		domain = strtok_r(buf, "; ", &save_ptr);
		while(domain != NULL)
		{
			clear_dns_tunnel_by_domain(domain);

			// get next domain
			domain = strtok_r(NULL, "; ", &save_ptr);
		}

		free(buf);
	}
	else
	{
		int idx = -1;

		/* Check server_ip is a valid IP */
		if(inet_pton(AF_INET, node->server_ip, &(sa.sin_addr)) != 1)
			return -1;

		if(node->domain == NULL)
			return -1;

		buf = strdup(node->domain);
		if(buf == NULL)
			return -2;

		memcpy(entry.server_ip, &(sa.sin_addr), sizeof(entry.server_ip));

		domain = strtok_r(buf, "; ", &save_ptr);
		while(domain != NULL)
		{
			memset(entry.domain, 0, sizeof(entry.domain));
			strcpy(entry.domain, domain);

			idx = find_dns_tunnel(&entry);
			if(idx >= 0)
				mib_chain_delete(MIB_DNS_TUNNEL_TBL, idx);

			// get next domain
			domain = strtok_r(NULL, "; ", &save_ptr);
		}

		free(buf);
	}

	Commit();

	if (access("/tmp/running_dbus_cts", F_OK) == 0)
	{
		printf("%s skip update dnsmasq\n", __FUNCTION__);
	}
	else
	{
		// update dnsmasq config
		restart_dnsrelay();
	}
	return 0;
}

void free_dns_tunnels(dns_tunnel_node_t *tunnels)
{
	if(tunnels)
	{
		dns_tunnel_node_t *node = tunnels, *tmp = NULL;
		while(node != NULL)
		{
			tmp = node;
			node = node->next;
			if(tmp->server_ip) free(tmp->server_ip);
			if(tmp->domain) free(tmp->domain);
			free(tmp);
		}
	}
}

extern int vasprintf(char **strp, const char *fmt, va_list ap);
static char* str_realloc_and_concat(char **str, const char *format, ...)
{
	va_list ap;
	int i, n;
	char *s;

	n = strlen(*str);
	va_start(ap, format);
	i = vasprintf(&s, format, ap);
	*str = (char *)(realloc(*str, n + i + 1));
	if (*str == NULL)
		return NULL;

	strncpy(&(*str)[n], s, i);
	va_end(ap);
}


/*
 * FUNCTION: GET_WAN_DNSTUNNEL
 *
 * OUTPUT:
 *	num:
 *		Number of DNS tunnel instance
 *	tunnels:
 *		Array of dns_tunnel_node_t
 *
 * RETURN:
 *	number of dns_tunnel_node_t nodes.
 *	-1 -- out of memory
 *	-2 -- other
 *
 * NOTES:
 * Don't forget to call free_dns_tunnels().
 */
int get_wan_dns_tunnel(dns_tunnel_node_t **tunnels)
{
	MIB_CE_DNS_TUNNEL_T entry;
	int total, i, j;
	char *domain = NULL, ip_addr[40] = "";
	dns_tunnel_node_t *node = NULL, *tail = NULL;
	int num = 0;

	*tunnels = NULL;

	total = mib_chain_total(MIB_DNS_TUNNEL_TBL);
	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(MIB_DNS_TUNNEL_TBL, i, &entry) == 0)
			continue;

		if(inet_ntop(AF_INET, (void *)entry.server_ip, ip_addr, sizeof(ip_addr)) == NULL)
		{
			fprintf(stderr, "WARNNING: server_ip of index %d is invalid\n", i);
			continue;
		}

		// find existed node
		node = *tunnels;
		while(node != NULL)
		{
			if(strcmp(node->server_ip, ip_addr) == 0)
				break;

			node = node->next;
		}

		if(node == NULL)
		{
			// create & add new node
			node = malloc(sizeof(dns_tunnel_node_t));
			if(node == NULL)
				goto out_of_mem;

			memset(node, 0, sizeof(dns_tunnel_node_t));

			if(*tunnels == NULL)
				*tunnels = node;
			else
				tail->next = node;
			tail = node;
			num++;

			node->server_ip = strdup(ip_addr);
			if(node->server_ip == NULL)
				goto out_of_mem;

			node->domain = strdup(entry.domain);
			if(node->domain == NULL)
				goto out_of_mem;
		}
		else
		{
			// dynamic string concatate
			str_realloc_and_concat(&node->domain, ";%s", entry.domain);
		}
	}

	//fprintf(stderr, "<%s:%d> total dns tunnel nodes: %d\n", __func__, __LINE__, num);

	return num;

out_of_mem:
	free_dns_tunnels(*tunnels);
	*tunnels = NULL;
	return -1;
}
#endif	//CTC_DNS_TUNNEL

#ifdef CTC_TELNET_SCHEDULED_CLOSE
struct callout_s sch_telnet_ch;

void schTelnetCheck()
{
#ifdef REMOTE_ACCESS_CTL
	MIB_CE_ACC_T Entry;
	static unsigned char telnet_enable = 0;
	static time_t startTime;
	time_t curTime;
	struct sysinfo info;

	if (!mib_chain_get(MIB_ACC_TBL, 0, (void *)&Entry))
	{
		printf("mib chain get MIB_ACC_TBL failed!");
	}else{
		if(Entry.telnet == 0)
		{
			if(telnet_enable)
				telnet_enable = 0;
		}else{
			if(telnet_enable ==0)
			{
				sysinfo(&info);
				startTime = info.uptime;
				telnet_enable = 1;
			}

			sysinfo(&info);
			curTime = info.uptime;
			if((curTime - startTime) > CTC_TELNET_SCHEDULED_CLOSE_TIME)
			{
				//close telnet
				printf("%s %d close telnet\n",__FUNCTION__,__LINE__);

				filter_set_remote_access(0);

				Entry.telnet = 0;
				mib_chain_update(MIB_ACC_TBL, (void *)&Entry, 0);

				filter_set_remote_access(1);
			}
		}
	}
#endif

	TIMEOUT_F((void*)schTelnetCheck, 0, 10, sch_telnet_ch);
}
#endif

#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL
struct callout_s macinfo_ch;

void poll_macinfo(void)
{
	unsigned char lanHostBandwidthControlEnable=0;

	mib_get_s(MIB_LANHOST_BANDWIDTH_CONTROL_ENABLE, (void *)&lanHostBandwidthControlEnable, sizeof(lanHostBandwidthControlEnable));
	if((1==lanHostBandwidthControlEnable)&&(1 == reCalculateLanPortRateLimitInfo()))
	{
		apply_lanPortRateLimit();
#ifdef WLAN_SUPPORT
		apply_wifiRateLimit();
#endif
	}

	TIMEOUT_F((void*)poll_macinfo, 0, 1, macinfo_ch);
}

int find_lanHostByMac(char *mac)
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
			return 0;
		}

		if(!memcmp(mac, entry.mac, ETHER_ADDR_LEN))
		{
			return 1;
		}
	}

	return 0;
}

// dir=0: UP, dir=1: DOWN
int is_lanHostMacNeedTrap(char *mac, int dir)
{
	int ratelimit_trap_to_ps=0;
	MIB_LAN_HOST_BANDWIDTH_T entry;
	int i, totalNum;
	
	if(!mib_get(MIB_LANPORT_RATELIMIT_TRAP_TO_PS, (void *)&ratelimit_trap_to_ps))
	{
		printf("[%s %d]Get mib value MIB_LANPORT_RATELIMIT_TRAP_TO_PS failed!\n", __func__, __LINE__);
		return 0;
	}
	
	memset(&entry, 0 , sizeof(MIB_LAN_HOST_BANDWIDTH_T));
	totalNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);
	for(i=0; i<totalNum; i++)
	{
		if (!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, i, (void *)&entry))
		{
			printf("[%s %d]get LAN_HOST_BANDWIDTHl chain entry error!\n", __func__, __LINE__);
			continue;
		}

		if(dir==0 && !memcmp(mac, entry.mac, ETHER_ADDR_LEN) && entry.maxUsBandwidth <= ratelimit_trap_to_ps)
		{
			return 1;
		}
		else if(dir==1 && !memcmp(mac, entry.mac, ETHER_ADDR_LEN) && entry.maxDsBandwidth <= ratelimit_trap_to_ps)
		{
			return 1;
		}
	}

	return 0;
}

#if defined(CONFIG_USER_LAN_BANDWIDTH_EX_CONTROL)
int set_attach_device_maxbandwidth(unsigned char *pMacAddr, unsigned int maxUsBandWidth, unsigned int maxDsBandWidth, unsigned int minUsBandWidth, unsigned int minDsBandWidth)
#else
int set_attach_device_maxbandwidth(unsigned char *pMacAddr, unsigned int maxUsBandWidth, unsigned int maxDsBandWidth)
#endif
{
	int ret, i, totalNum;
	unsigned char lanHostBandwidthControlEnable=0;
	MIB_LAN_HOST_BANDWIDTH_T entry;

	/* check if this function is on */
	mib_get_s(MIB_LANHOST_BANDWIDTH_CONTROL_ENABLE, (void*)&lanHostBandwidthControlEnable, sizeof(lanHostBandwidthControlEnable));
	if(lanHostBandwidthControlEnable == 0)
		return	-3;

	/* check input first */
	if(isMacAddrInvalid(pMacAddr))
		return -1;

	if( (maxUsBandWidth>=DEFAULT_MAX_US_BANDWIDTH)||(maxUsBandWidth>=DEFAULT_MAX_US_BANDWIDTH) )
		return -1;

	clear_maxBandwidth();//clear old

	totalNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);
	memset(&entry, 0 , sizeof(MIB_LAN_HOST_BANDWIDTH_T));
	for(i=0; i<totalNum; i++)
	{
		if(!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, i, (void*)&entry))
		{
			printf("Get lan host bandwidth table error.\n");
			return -3;
		}

		if( !macAddrCmp(pMacAddr, entry.mac))
		{
			if( (maxUsBandWidth==0)&&(maxDsBandWidth==0) )
			{//0 mean not rate limit
				mib_chain_delete(MIB_LAN_HOST_BANDWIDTH_TBL, i);
			}
			else
			{
				/* Set usBandwidth and dsBandwidth by macAddr */
				entry.maxUsBandwidth = maxUsBandWidth;
				entry.maxDsBandwidth = maxDsBandWidth;
#if defined(CONFIG_USER_LAN_BANDWIDTH_EX_CONTROL)
				entry.minUsBandwidth = minUsBandWidth;
				entry.minDsBandwidth = minDsBandWidth;
#endif
				mib_chain_update(MIB_LAN_HOST_BANDWIDTH_TBL, (void*)&entry, i);
			}

			goto apply;
		}
	}
	
	if( (maxUsBandWidth==0)&&(maxDsBandWidth==0) )
		return -2;

	if( (maxUsBandWidth != 0)||(maxDsBandWidth != 0) )
	{
		/* add an entry */
		memset(&entry, 0 , sizeof(MIB_LAN_HOST_BANDWIDTH_T));
		memcpy( entry.mac, pMacAddr, MAC_ADDR_LEN );
		entry.maxUsBandwidth = maxUsBandWidth;
		entry.maxDsBandwidth = maxDsBandWidth;

#if defined(CONFIG_USER_LAN_BANDWIDTH_EX_CONTROL)
		entry.minUsBandwidth = minUsBandWidth;
		entry.minDsBandwidth = minDsBandWidth;
#endif

		if(rtk_lan_host_get_host_by_mac(&entry.mac[0]) != -1)
			entry.hostIndex = rtk_lan_host_get_host_by_mac(&entry.mac[0]);
		else
			entry.hostIndex = rtk_lan_host_get_new_host_index();
		ret = mib_chain_add(MIB_LAN_HOST_BANDWIDTH_TBL, (void*)&entry);
		if (ret <= 0)
			return -3;
	}

apply:
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif // of #if COMMIT_IMMEDIATELY
	init_maxBandwidth();
	apply_maxBandwidth();

#ifdef CONFIG_USER_LANNETINFO
	int set_lanhost_control_status(unsigned char *pMacAddr, unsigned char controlStatus);
	if(maxUsBandWidth > 0 || maxDsBandWidth > 0)
	{
		set_lanhost_control_status(pMacAddr, 1);
	}
#endif
	return 0;
}


/*
 * FUNCTION:GET_ATTACH_DEVICE_BANDWIDTH
 *
 * INPUT:
 *	macAddr:
 *		target device's MAC address
 *	pUsBandWidth:
 *		save upstream bandwidth
 *	pDsBandWidth:
 *		save downstream bandwidth
 * RETURN:
 *	0  -- successful--RT_ERR_CTC_IGD1_OK
 *	-1 -- Input error
 *  -2 -- device not find
 *  -3 -- other
 */
int get_attach_device_maxbandwidth(unsigned char *pMacAddr, unsigned int *pUsBandWidth, unsigned int *pDsBandWidth, unsigned int *pUsBandWidthMin, unsigned int *pDsBandWidthMin)
{
	int i, totalNum;
	unsigned char lanHostBandwidthControlEnabel;
	MIB_LAN_HOST_BANDWIDTH_T entry;

	/* check if this function is on */
	mib_get_s(MIB_LANHOST_BANDWIDTH_CONTROL_ENABLE, &lanHostBandwidthControlEnabel, sizeof(lanHostBandwidthControlEnabel));
	if(lanHostBandwidthControlEnabel == 0){
		printf("Func is off.\n");
		return	-3;
	}
	/* check input first */
	if( (isMacAddrInvalid(pMacAddr)) ||(pUsBandWidth == NULL) || (pDsBandWidth == NULL) )
		return -1;

#if defined(CONFIG_USER_LAN_BANDWIDTH_EX_CONTROL)
	if( (pUsBandWidthMin == NULL) || (pDsBandWidthMin == NULL) )
		return -1;
#endif

	if( ( totalNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL) ) <= 0 )
	{
		printf("Lan host mib table is empty.\n");
		return -2;
	}

	memset(&entry, 0, sizeof(MIB_LAN_HOST_BANDWIDTH_T));
	for(i=0; i<totalNum; i++)
	{
		if(!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL,i, (void*)&entry))
			continue;

		if( !macAddrCmp(pMacAddr, entry.mac))
			break;
	}

	if( i==totalNum )
	{
		return -2;
	}
	/* Get usBandwidth and dsBandwidth by macAddr */
	*pUsBandWidth = entry.maxUsBandwidth;
	*pDsBandWidth = entry.maxDsBandwidth;

#if defined(CONFIG_USER_LAN_BANDWIDTH_EX_CONTROL)
	*pUsBandWidthMin = entry.minUsBandwidth;
	*pDsBandWidthMin = entry.minDsBandwidth;
#endif

	return 0;
}

#endif //#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL

static char* http_redirect2register_page_header =
	"HTTP/1.1 302 Object Moved\r\n"
	"Location: http://%s/usereg.asp\r\n"
	"Server: adsl-router-gateway\r\n"
	"Content-Type: text/html\r\n"
	"Content-Length: %d\r\n"
	"\r\n"
	"%s";
static char* http_redirect2register_page_content =
	"<html><head><title>Object Moved</title></head>"
	"<body><h1>Object Moved</h1>This Object may be found in "
#ifdef CONFIG_YUEME
	"<a HREF=\"http://%s:8080/usereg.asp\">here</a>.</body><html>";
#else
	"<a HREF=\"http://%s/usereg.asp\">here</a>.</body><html>";
#endif

static char* http_redirect2ethregister_page_content =
        "<html><head><title>Object Moved</title></head>"
	"<body><h1>Object Moved</h1>This Object may be found in "
	"<a HREF=\"http://%s/quickregister.html\">here</a>.</body><html>";

static void gen_http_redirect2register_reply(char * content, int size)
{
	char buff[512] = {0};
	int filesize;
	unsigned char lan_ip[IP_ADDR_LEN] = {0};
	char lan_ip_str[INET_ADDRSTRLEN] = {0};

	mib_get_s(MIB_ADSL_LAN_IP, lan_ip, sizeof(lan_ip));
	inet_ntop(AF_INET, lan_ip, lan_ip_str, INET_ADDRSTRLEN);

#ifdef CONFIG_CMCC_ENTERPRISE
	snprintf(buff, 512, http_redirect2ethregister_page_content, lan_ip_str);
#else
	snprintf(buff, 512, http_redirect2register_page_content, lan_ip_str);
#endif
	filesize = strlen(buff);

	snprintf(content, size, http_redirect2register_page_header, lan_ip_str, filesize, buff);
}

#ifdef SUPPORT_WEB_REDIRECT

#define WEB_REDIR_PORT 8080

int setup_web_redir(int enable)
{
	int status=0;
	char tmpbuf[MAX_URL_LEN]={0};
	char ipaddr[16]={0}, ip_port[32]={0};
	int  redir_port=WEB_REDIR_PORT;
	ipaddr[0]='\0'; ip_port[0]='\0';//redir_server[0]='\0';
	char contents[1024] = {0};

	if(enable)
	{
		//create rules to redirect web to user registered pages 192.168.1.1/usereg.asp
		if (mib_get_s(MIB_ADSL_LAN_IP, (void *)tmpbuf, sizeof(tmpbuf)) != 0)
		{
			strncpy(ipaddr, inet_ntoa(*((struct in_addr *)tmpbuf)), 16);
			ipaddr[15] = '\0';
			sprintf(ip_port,"%s:%d",ipaddr,redir_port);
		}
		//iptables -t nat -N WebRedirectReg
		status|=va_cmd(IPTABLES, 4, 1, "-t", "nat","-N","WebRedirectReg");

		//iptables -t nat -A WebRedirectReg -p tcp --dport 80 -j DNAT --to-destination 192.168.1.1:8080
		status|=va_cmd(IPTABLES, 12, 1, "-t", "nat",(char *)FW_ADD,"WebRedirectReg",
			"-p", "tcp", "--dport", "80", "-j", "DNAT",
			"--to-destination", ip_port);

		//iptables -t nat -A PREROUTING -i br0 -p tcp --dport 80 -j WebRedirectReg
		status|=va_cmd(IPTABLES, 12, 1, "-t", "nat",(char *)FW_ADD,"PREROUTING",
			"-i", LANIF,
			"-p", "tcp", "--dport", "80", "-j", "WebRedirectReg");
		gen_http_redirect2register_reply(contents, sizeof(contents));
	}
	else{
		//flush iptable rules
		status|=va_cmd(IPTABLES, 4, 1, "-t", "nat","-F","WebRedirectReg");

		//iptables -t nat -D PREROUTING -i br0 -p tcp --dport 80 -j WebRedirectReg
		status|=va_cmd(IPTABLES, 12, 1, "-t", "nat",(char *)FW_DEL,"PREROUTING",
			"-i", LANIF,
			"-p", "tcp", "--dport", "80", "-j", "WebRedirectReg");
	}

	return status;

}
#endif
void enable_http_redirect2register_page(int enable)
{
#ifdef SUPPORT_WEB_REDIRECT
	setup_web_redir(enable);
#endif //end SUPPORT_WEB_REDIRECT
}


#ifdef SUPPORT_ACCESS_RIGHT
const char FW_INTERNET_ACCESSRIGHT_BRIDGE_INPUT[] = "internet_accessright_b_INPUT";
const char FW_INTERNET_ACCESSRIGHT_BRIDGE_FORWARD[] = "internet_accessright_b_FORWARD";
const char FW_INTERNET_ACCESSRIGHT_ROUTER[] = "internet_accessright_r";
const char FW_STORAGE_ACCESSRIGHT_ROUTER[] = "storage_accessright_r";

void flush_internet_access_right()
{
	DOCMDINIT;

	DOCMDARGVS(EBTABLES,DOWAIT,"-F %s",(char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_INPUT);
	DOCMDARGVS(EBTABLES,DOWAIT,"-F %s",(char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_FORWARD);
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_INPUT, "-j", (char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_INPUT);
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_FORWARD);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_INPUT);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_FORWARD);

	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_INTERNET_ACCESSRIGHT_ROUTER);
	va_cmd(IPTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)FW_INTERNET_ACCESSRIGHT_ROUTER);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_INTERNET_ACCESSRIGHT_ROUTER);

#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)FW_INTERNET_ACCESSRIGHT_ROUTER);
	va_cmd(IP6TABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)FW_INTERNET_ACCESSRIGHT_ROUTER);
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_INTERNET_ACCESSRIGHT_ROUTER);
#endif
}

/* internetAccessRight value:
   0: can't access modem and internet
   1: can access modem and can't access inernet
   2: can access inernet */
void setup_internet_access_right(void)
{

	int totalNum,index;
	unsigned char macString[32] = {0};
	MIB_LAN_HOST_ACCESS_RIGHT_T entry;
	DOCMDINIT;

	totalNum = mib_chain_total(MIB_LAN_HOST_ACCESS_RIGHT_TBL);

	va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_INPUT);
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_INPUT);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_INPUT, "RETURN");
	va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_FORWARD);
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_FORWARD);
	va_cmd(EBTABLES, 3, 1, "-P", (char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_FORWARD, "RETURN");

	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_INTERNET_ACCESSRIGHT_ROUTER);
	va_cmd(IPTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", (char *)FW_INTERNET_ACCESSRIGHT_ROUTER);

#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_INTERNET_ACCESSRIGHT_ROUTER);
	va_cmd(IP6TABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", (char *)FW_INTERNET_ACCESSRIGHT_ROUTER);
#endif
	for(index=0; index<totalNum; index++)
	{
		mib_chain_get(MIB_LAN_HOST_ACCESS_RIGHT_TBL,index,&entry);
		if( entry.internetAccessRight == INTERNET_ACCESS_DENY)
		{
			snprintf(macString, sizeof(macString), "%02x:%02x:%02x:%02x:%02x:%02x", entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);

			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -s %s -j DROP", (char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_INPUT, macString);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -s %s -j DROP", (char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_FORWARD, macString);
			va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_INTERNET_ACCESSRIGHT_ROUTER, "-m", "mac", "--mac-source", macString, "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INTERNET_ACCESSRIGHT_ROUTER, "-m", "mac", "--mac-source", macString, "-j", (char *)FW_DROP);
#endif
			//Add IGMP snooping black list
			rtk_set_all_lan_igmp_blacklist_by_mac(1, macString);

		}
		else if(entry.internetAccessRight == INTERNET_ACCESS_NO_INTERNET)
		{
			snprintf(macString, sizeof(macString), "%02x:%02x:%02x:%02x:%02x:%02x", entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);

			// For blocking bridged lanhost to access internet
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -s %s --logical-in br+ -o %s+ -j DROP", (char *)FW_INTERNET_ACCESSRIGHT_BRIDGE_FORWARD, macString, ALIASNAME_NAS0);
			va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_INTERNET_ACCESSRIGHT_ROUTER, "-m", "mac", "--mac-source", macString, "!", "-o", "br+", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 11, 1, (char *)FW_ADD, (char *)FW_INTERNET_ACCESSRIGHT_ROUTER, "-m", "mac", "--mac-source", macString, "!", "-o", "br+", "-j", (char *)FW_DROP);
			//Add IGMP snooping black list
			rtk_set_all_lan_igmp_blacklist_by_mac(1, macString);
#endif
		}
	}
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#ifdef WLAN_SUPPORT
#ifdef WLAN_11AX
	setup_wlan_MAC_ACL(-1,-1);
#else
	setup_wlan_MAC_ACL();
#endif
#endif
#endif
}

void flush_storage_access_right()
{
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_STORAGE_ACCESSRIGHT_ROUTER);
	va_cmd(IPTABLES, 4, 1, (char *)FW_DEL, (char *)FW_INPUT, "-j", (char *)FW_STORAGE_ACCESSRIGHT_ROUTER);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_STORAGE_ACCESSRIGHT_ROUTER);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)FW_STORAGE_ACCESSRIGHT_ROUTER);
	va_cmd(IP6TABLES, 4, 1, (char *)FW_DEL, (char *)FW_INPUT, "-j", (char *)FW_STORAGE_ACCESSRIGHT_ROUTER);
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_STORAGE_ACCESSRIGHT_ROUTER);
#endif
}
void setup_storage_access_right(void)
{

	int totalNum,index;
	unsigned char macString[32] = {0};
	MIB_LAN_HOST_ACCESS_RIGHT_T entry;
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	unsigned char cmd_buf[64];
	int limitcount = 0;
	
	memset(cmd_buf, 0x0, sizeof(cmd_buf));
#endif

	
	totalNum = mib_chain_total(MIB_LAN_HOST_ACCESS_RIGHT_TBL);

	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_STORAGE_ACCESSRIGHT_ROUTER);
	va_cmd(IPTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-j", (char *)FW_STORAGE_ACCESSRIGHT_ROUTER);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_STORAGE_ACCESSRIGHT_ROUTER);
	va_cmd(IP6TABLES, 4, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-j", (char *)FW_STORAGE_ACCESSRIGHT_ROUTER);
#endif
	for(index=0; index<totalNum; index++)
	{
		mib_chain_get(MIB_LAN_HOST_ACCESS_RIGHT_TBL,index,&entry);
		if( entry.storageAccessRight == 0 )
		{
			snprintf(macString, sizeof(macString),"%02x:%02x:%02x:%02x:%02x:%02x", entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);

			/* iptables -A FW_STORAGE_ACCESSRIGHT_ROUTER -m mac --mac-source xx:xx:xx:xx:xx:xx -p tcp --dport 21 -j DROP */
			va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char*)FW_STORAGE_ACCESSRIGHT_ROUTER, "-m", "mac", "--mac-source", macString, "-p", "tcp", "--dport", "21", "-j", (char *)FW_DROP);

			/* iptables -A FW_STORAGE_ACCESSRIGHT_ROUTER -m mac --mac-source xx:xx:xx:xx:xx:xx -p tcp --dport 139/445 -j DROP */
			va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char*)FW_STORAGE_ACCESSRIGHT_ROUTER, "-m", "mac", "--mac-source", macString, "-p", "tcp", "--dport", "139", "-j", (char *)FW_DROP);
			va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char*)FW_STORAGE_ACCESSRIGHT_ROUTER, "-m", "mac", "--mac-source", macString, "-p", "tcp", "--dport", "445", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
		/* ip6tables -A FW_STORAGE_ACCESSRIGHT_ROUTER -m mac --mac-source xx:xx:xx:xx:xx:xx -p tcp --dport 21 -j DROP */
		va_cmd(IP6TABLES, 12, 1, (char *)FW_ADD, (char*)FW_STORAGE_ACCESSRIGHT_ROUTER, "-m", "mac", "--mac-source", macString, "-p", "tcp", "--dport", "21", "-j", (char *)FW_DROP);

		/* ip6tables -A FW_STORAGE_ACCESSRIGHT_ROUTER -m mac --mac-source xx:xx:xx:xx:xx:xx -p tcp --dport 139/445 -j DROP */
		va_cmd(IP6TABLES, 12, 1, (char *)FW_ADD, (char*)FW_STORAGE_ACCESSRIGHT_ROUTER, "-m", "mac", "--mac-source", macString, "-p", "tcp", "--dport", "139", "-j", (char *)FW_DROP);
		va_cmd(IP6TABLES, 12, 1, (char *)FW_ADD, (char*)FW_STORAGE_ACCESSRIGHT_ROUTER, "-m", "mac", "--mac-source", macString, "-p", "tcp", "--dport", "445", "-j", (char *)FW_DROP);
#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#ifdef CONFIG_RTK_SMB_SPEEDUP
			snprintf(cmd_buf, 46, "echo set 0 > /proc/smbshortcut\n");
			system(cmd_buf);
#endif
#ifdef CONFIG_USER_RTK_FASTPASSNF_MODULE
			rtk_fastpassnf_smbspeedup(0);
#endif
			limitcount ++;
#endif			
		}
	}
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	if ( limitcount == 0 ){ /* That means there is no hosts to be limited. */
#ifdef CONFIG_RTK_SMB_SPEEDUP
		snprintf(cmd_buf, 46, "echo set 1 > /proc/smbshortcut\n");
		system(cmd_buf);
#endif
#ifdef CONFIG_USER_RTK_FASTPASSNF_MODULE
		rtk_fastpassnf_smbspeedup(1);
#endif
	}
#endif	
}

void apply_accessRight(unsigned char enable)
{
	flush_internet_access_right();
	flush_storage_access_right();
	if(enable){
		setup_internet_access_right();
		setup_storage_access_right();
	}
	rtk_hwnat_flow_flush();
}

/*
 * FUNCTION:SET_ATTACH_DEVICE_RIGHT
 *
 * INPUT:
 *	macAddr:
 *		target device's MAC address
 *	internetAccessRight:
 *		internet access right, 1-ON, 0-OFF
 *  storageAccessRight:
 *		storage access right, 1-ON, 0-OFF
 *
 * RETURN:
 *	0  successful
 *	-1 Input error
 *  -2 device not find
 *  -3 Other error
 */
int set_attach_device_right(unsigned char *pMacAddr, unsigned char internetAccessRight, unsigned char storageAccessRight)
{
	int ret, i, totalNum;
	MIB_LAN_HOST_ACCESS_RIGHT_T entry;
	unsigned char macString[32] = {0};

	/* check input first */
	if( isMacAddrInvalid(pMacAddr) )
		return -1;

	/* get Host by macAddr */
	 totalNum = mib_chain_total(MIB_LAN_HOST_ACCESS_RIGHT_TBL);

	memset(&entry, 0, sizeof(MIB_LAN_HOST_ACCESS_RIGHT_T));
	for(i=0; i<totalNum; i++)
	{
		if(!mib_chain_get(MIB_LAN_HOST_ACCESS_RIGHT_TBL, i, (void*)&entry))
		{
			printf("Get lan host access right mib entry error.\n");
			return -3;
		}

		if( !macAddrCmp(pMacAddr, entry.mac))
		{
			if( (internetAccessRight==INTERNET_ACCESS_ALLOW) && (storageAccessRight==STORAGE_ACCESS_ALLOW) )//delete mib entry
			{
				snprintf(macString, sizeof(macString),"%02x:%02x:%02x:%02x:%02x:%02x", pMacAddr[0], pMacAddr[1], pMacAddr[2], pMacAddr[3], pMacAddr[4], pMacAddr[5]);

				//Clear IGMP snooping black list
				rtk_set_all_lan_igmp_blacklist_by_mac(0, macString);
				printf("Delete the LAN_HOST_ACCESS_RIGHT_TBL MIB entry %d.\n", i);
				mib_chain_delete(MIB_LAN_HOST_ACCESS_RIGHT_TBL, i);
				goto apply;
			}
			/* Set internetAccessRight and storageAccessRight by macAddr */
			entry.internetAccessRight = internetAccessRight;
			entry.storageAccessRight = storageAccessRight;
			mib_chain_update(MIB_LAN_HOST_ACCESS_RIGHT_TBL, (void*)&entry, i);
			goto apply;
		}
	}
	
	if( (internetAccessRight==INTERNET_ACCESS_ALLOW) && (storageAccessRight==STORAGE_ACCESS_ALLOW) )//delete mib entry
		return -2;

	/* add an entry */
	memset(&entry, 0, sizeof(MIB_LAN_HOST_ACCESS_RIGHT_T));
	memcpy(entry.mac, pMacAddr, MAC_ADDR_LEN);

	entry.internetAccessRight = internetAccessRight;
	entry.storageAccessRight = storageAccessRight;

	ret = mib_chain_add(MIB_LAN_HOST_ACCESS_RIGHT_TBL, (void*)&entry);
	if(ret <= 0)
		return -3;

apply:
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif // of #if COMMIT_IMMEDIATELY
	apply_accessRight(1);

end:
	return 0;
}
#endif //#ifdef SUPPORT_ACCESS_RIGHT
#ifdef CONFIG_CU_BASEON_YUEME
static int get_lanInfo_mibentry_byMAC(unsigned char * MacAddr, MIB_CE_SMART_LAN_HOST_T *lanInfoEntry)
{
	MIB_CE_SMART_LAN_HOST_T entry;
	int totalEntry = 0,num = 0;
	unsigned char smac[MAC_ADDR_LEN]={0};

	convertMacFormat(MacAddr, smac);

	memset(&entry, 0 ,sizeof(entry));
	totalEntry = mib_chain_total(MIB_DBUS_LANHOST_TBL);
	if(totalEntry)
	{
		for(num = 0; num < totalEntry; num++)
		{
			if(!mib_chain_get(MIB_DBUS_LANHOST_TBL,num,&entry))
				continue;
			if(memcmp(entry.mac, smac, 6)==0)
			{
				if(lanInfoEntry)
				{
					memcpy(lanInfoEntry, &entry, sizeof(MIB_CE_SMART_LAN_HOST_T));
					return 1;
				}
				break;
			}				
		}
	}
	return 0;
}
#endif

const char FW_MACFILTER_ROUTER[] = "macfilter_r";

int setupMacFilterTables()
{
	int i, total, total_out = 0;
	char *policy;
	MIB_CE_ROUTEMAC_T MacEntry;
	unsigned char macFilterEnable = 0;
	unsigned char macFilterMode = 0; // 0-black list, 1-white list
#ifdef CONFIG_CU_BASEON_YUEME
	MIB_CE_SMART_LAN_HOST_T lanInfo_entry;
	uint32 ret1;
	uint8 errCode[64]={0};
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	char cmd[256];
	snprintf(cmd, sizeof(cmd), "echo 5 > %s", KMODULE_CMCC_MAC_FILTER_FILE);
	system(cmd);
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
	memset(schdMacFilterIdx, 0, sizeof(schdMacFilterIdx));
	memcpy(&curTm, localtime(&curTime), sizeof(curTm));
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#ifdef WLAN_SUPPORT
#ifdef WLAN_11AX
	setup_wlan_MAC_ACL(-1,-1);
#else
	setup_wlan_MAC_ACL();
#endif
#endif
#endif

	DOCMDINIT;

	// Delete all Macfilter rule
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_MACFILTER_FWD);
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_MACFILTER_LI);
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_MACFILTER_LO);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_MACFILTER_FWD_L3);
	va_cmd(EBTABLES, 4, 1, "-t", "nat", "-F", (char *)FW_MACFILTER_MC);

	mib_get_s(MIB_MAC_FILTER_SRC_ENABLE, &macFilterEnable, sizeof(macFilterEnable));

	if(macFilterEnable&1)//BIT0 set - black list mode. if both BIT0&BIT1 are set, only black list mode
		macFilterMode = 0;
	else if(macFilterEnable&2)//BIT1 set, BIT0 not set - white list mode.
		macFilterMode = 1;

	if(macFilterEnable)
	{
		if (macFilterMode == 0)
			policy = (char *)FW_DROP;
		else
			policy = (char *)FW_RETURN;

		total = mib_chain_total(MIB_MAC_FILTER_ROUTER_TBL);

		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_MAC_FILTER_ROUTER_TBL, i, (void *)&MacEntry))
				return -1;

			if (strlen(MacEntry.mac) == 0)
				continue;

#ifdef SUPPORT_TIME_SCHEDULE
			if(MacEntry.schedIndex >= 1 && MacEntry.schedIndex <= totalSched)
			{

				if (!mib_chain_get(MIB_TIME_SCHEDULE_TBL, MacEntry.schedIndex-1, (void *)&schdEntry))
				{
					continue;
				}
				shouldSchedule = check_should_schedule(&curTm, &schdEntry);
				isScheduled = isScheduledIndex(i, schdMacFilterIdx, SCHD_MAC_FILTER_SIZE);
				if((shouldSchedule && (isScheduled == 0)) || 
					((shouldSchedule == 0) && isScheduled))
				{
					if(shouldSchedule && (isScheduled == 0))
					{
						//not scheduled yet, should add rule
						addScheduledIndex(i, schdMacFilterIdx, SCHD_MAC_FILTER_SIZE);
					}
					else
					{
						//scheduled but is out of schedule time, should delete rule
						delScheduledIndex(i, schdMacFilterIdx, SCHD_MAC_FILTER_SIZE);
						continue;
					}
				}
				else
				{
					continue;
				}
			}

#endif
			if(macFilterMode == MacEntry.mode){
#ifdef MAC_FILTER_BLOCKTIMES_SUPPORT
				if (!MacEntry.enable)
					continue;
#endif
#ifdef CONFIG_CMCC_ENTERPRISE
				if (MacEntry.direction == DIR_IN)
				{
					DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv4 --ip-proto udp --ip-dport 67:68 -d %s -j RETURN", (char *)FW_MACFILTER_LO, MacEntry.mac);

					DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -o %s+ -d %s -j %s", (char *)FW_MACFILTER_FWD,
						(char *)ELANIF, MacEntry.mac, policy);
					DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -o %s+ -d %s -j %s", (char *)FW_MACFILTER_LO,
						(char *)ELANIF, MacEntry.mac, policy);

#ifdef WLAN_SUPPORT
					DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -o %s+ -d %s -j %s", (char *)FW_MACFILTER_FWD,
						(char *)"wlan", MacEntry.mac, policy);
					DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -o %s+ -d %s -j %s", (char *)FW_MACFILTER_LO,
						(char *)"wlan", MacEntry.mac, policy);
#endif
				}
				else
#endif
				{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
				DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -p IPv4 --ip-proto udp --ip-dport 67:68 -s %s -j RETURN", (char *)FW_MACFILTER_LI,
					MacEntry.mac); 
#endif
				if(MacEntry.access_level==0){
					//default policy,block accessing br0 and wan
					DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -s %s -j %s", (char *)FW_MACFILTER_FWD,
						(char *)ELANIF, MacEntry.mac, policy);
					DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -s %s -j %s", (char *)FW_MACFILTER_LI,
						(char *)ELANIF, MacEntry.mac, policy);
					DOCMDARGVS(EBTABLES,DOWAIT,"-t nat -A %s -i %s+ -s %s -p IPv4 --ip-proto igmp -j %s",
						(char *)FW_MACFILTER_MC, (char *)ELANIF, MacEntry.mac, policy);
#ifdef CONFIG_IPV6
					DOCMDARGVS(EBTABLES,DOWAIT,"-t nat -A %s -i %s+ -s %s -p IPv6 --ip6-proto ipv6-icmp --ip6-icmp-type 130:132/0:255 -j %s",
						(char *)FW_MACFILTER_MC, (char *)ELANIF, MacEntry.mac, policy);
					DOCMDARGVS(EBTABLES,DOWAIT,"-t nat -A %s -i %s+ -s %s -p IPv6 --ip6-proto ipv6-icmp --ip6-icmp-type 143/0:255 -j %s",
						(char *)FW_MACFILTER_MC, (char *)ELANIF, MacEntry.mac, policy);

#endif
#ifdef WLAN_SUPPORT
					DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -s %s -j %s", (char *)FW_MACFILTER_FWD,
						(char *)"wlan", MacEntry.mac, policy);
					DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -s %s -j %s", (char *)FW_MACFILTER_LI,
						(char *)"wlan", MacEntry.mac, policy);
					DOCMDARGVS(EBTABLES,DOWAIT,"-t nat -A %s -i %s+ -s %s -p IPv4 --ip-proto igmp -j %s",
						(char *)FW_MACFILTER_MC, (char *)"wlan", MacEntry.mac, policy);
#ifdef CONFIG_IPV6
					DOCMDARGVS(EBTABLES,DOWAIT,"-t nat -A %s -i %s+ -s %s -p IPv6 --ip6-proto ipv6-icmp --ip6-icmp-type 130:132/0:255 -j %s",
						(char *)FW_MACFILTER_MC, (char *)"wlan", MacEntry.mac, policy);
					DOCMDARGVS(EBTABLES,DOWAIT,"-t nat -A %s -i %s+ -s %s -p IPv6 --ip6-proto ipv6-icmp --ip6-icmp-type 143/0:255 -j %s",
						(char *)FW_MACFILTER_MC, (char *)"wlan", MacEntry.mac, policy);
#endif
#endif
				}
				else if(MacEntry.access_level == 1)
				{
					//only allow accessing br0
					DOCMDARGVS(IPTABLES,DOWAIT,"-A %s -m mac --mac-source %s -j %s", (char *)FW_MACFILTER_FWD_L3, MacEntry.mac, policy);
				}
				}
#ifdef CONFIG_CU_BASEON_YUEME
#ifdef WLAN_SUPPORT
		        /*if wifi device is in macblacklist and online, also kick out of wifi*/
				if(macFilterMode == 0 && get_lanInfo_mibentry_byMAC(MacEntry.mac, &lanInfo_entry))
				{
					if(lanInfo_entry.connectionType == Host_802_11 && lanInfo_entry.Active == 1)
					{
						printf("[%s %d]:kick out device(%02x-%02x-%02x-%02x-%02x-%02x) from SSID%d.\n",__func__,__LINE__,lanInfo_entry.mac[0],\
							lanInfo_entry.mac[1],lanInfo_entry.mac[2],lanInfo_entry.mac[3],lanInfo_entry.mac[4],lanInfo_entry.mac[5],lanInfo_entry.port);
						kickOutDevice(lanInfo_entry.mac,  lanInfo_entry.port, &ret1, errCode);
						if(ret1==-1)
							printf("[%s %d]:errCode=%s.\n", __func__, __LINE__, errCode);
					}
				}
#endif
#endif

#ifndef MAC_FILTER_SRC_ONLY
#error "You need to add more ebtable rule for filter DST mac\n"
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
				if(macFilterMode == 0
#ifdef CONFIG_CMCC_ENTERPRISE
					&& MacEntry.direction == DIR_OUT
#endif
					)
				{
					snprintf(cmd, sizeof(cmd), "echo 0 0 %s > %s", MacEntry.mac, KMODULE_CMCC_MAC_FILTER_FILE);
					system(cmd);
				}
#endif
				total_out++;
			}
		}
	}

	// default action
	DOCMDARGVS(EBTABLES,DOWAIT,"-P %s RETURN", (char *)FW_MACFILTER_FWD);
	DOCMDARGVS(EBTABLES,DOWAIT,"-P %s RETURN", (char *)FW_MACFILTER_LI);
	DOCMDARGVS(EBTABLES,DOWAIT,"-P %s RETURN", (char *)FW_MACFILTER_LO);

	if(macFilterEnable){
		if(macFilterMode == 1) { // DROP
			// ebtables -A macfilter -i eth0+ -j DROP
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -j DROP", (char *)FW_MACFILTER_FWD, (char *)ELANIF);
			// ebtables -A macfilter -i eth0+ -j DROP
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -j DROP", (char *)FW_MACFILTER_LI, (char *)ELANIF);
#ifdef WLAN_SUPPORT
			// ebtables -A macfilter -i wlan0+ -j DROP
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -j DROP", (char *)FW_MACFILTER_FWD, (char *)"wlan");
			// ebtables -A macfilter -i wlan0+ -j DROP
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -j DROP", (char *)FW_MACFILTER_LI, (char *)"wlan");
#endif
		}
		unsigned char macFilterwhitelistLocalAllow = 1;
		mib_get_s(PROVINCE_MACFILTER_WHITELIST_LOCAL_ALLOW, &macFilterwhitelistLocalAllow, sizeof(macFilterwhitelistLocalAllow));

		if(macFilterwhitelistLocalAllow == 0)
			goto end;

		//Local in default rules
		unsigned char tmpBuf[100]={0};
		unsigned char value[6];
#ifdef CONFIG_IPV6
		mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)tmpBuf, sizeof(tmpBuf));
		// ebtables -I macfilter -p IPv6 --ip6-dst fe80::1 -j ACCEPT
		if(total_out || macFilterMode == 1)
		{
			DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);

			DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type echo-request -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type echo-reply -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type router-solicitation -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type router-advertisement -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-solicitation -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-advertisement -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
			DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst ff02::1:ff00:0000/ffff:ffff:ffff:ffff:ffff:ffff:ff00:0 --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-solicitation -j ACCEPT", (char *)FW_MACFILTER_LI);
		}
#ifdef CONFIG_SECONDARY_IPV6
		memset(tmpBuf,0,sizeof(tmpBuf));
		mib_get_s(MIB_IPV6_LAN_IP_ADDR2, (void *)tmpBuf, sizeof(tmpBuf));
		if(strlen(tmpBuf)!=0)
		{
			if(total_out || macFilterMode == 1)
			{
				DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
		
				DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type echo-request -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
				DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type echo-reply -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
				DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type router-solicitation -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
				DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type router-advertisement -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
				DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-solicitation -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
				DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv6 --ip6-dst %s --ip6-proto ipv6-icmp --ip6-icmp-type neighbour-advertisement -j ACCEPT", (char *)FW_MACFILTER_LI, tmpBuf);
			}
		}
#endif
#endif
		mib_get_s(MIB_ADSL_LAN_IP, (void *)tmpBuf, sizeof(tmpBuf));
		if(total_out || macFilterMode == 1) DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv4 --ip-dst %s -j ACCEPT", (char *)FW_MACFILTER_LI, inet_ntoa(*((struct in_addr *)tmpBuf)));
#ifdef CONFIG_SECONDARY_IP
		mib_get_s(MIB_ADSL_LAN_ENABLE_IP2, (void *)value, sizeof(value));
		if (value[0] == 1) {
			mib_get_s(MIB_ADSL_LAN_IP2, (void *)tmpBuf, sizeof(tmpBuf));
			if(total_out || macFilterMode == 1) DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p IPv4 --ip-dst %s -j ACCEPT", (char *)FW_MACFILTER_LI, inet_ntoa(*((struct in_addr *)tmpBuf)));
		}
#endif
		if(total_out || macFilterMode == 1) DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p ARP -d ff:ff:ff:ff:ff:ff -j ACCEPT", (char *)FW_MACFILTER_LI);
		mib_get(MIB_ELAN_MAC_ADDR, (void *)value);
		if(total_out || macFilterMode == 1) DOCMDARGVS(EBTABLES,DOWAIT,"-I %s -p ARP -d %02x:%02x:%02x:%02x:%02x:%02x -j ACCEPT", (char *)FW_MACFILTER_LI, value[0], value[1], value[2], value[3], value[4], value[5]);
	}
end:
	rtk_hwnat_flow_flush();
	return 0;
}
#if defined(CONFIG_CU_BASEON_YUEME) && defined(COM_CUC_IGD1_ParentalControlConfig)
int setupMacFilter4ParentalCtrl()
{
	MIB_PARENTALCTRL_MAC_T entry;
	int cnt,index;

	char mac_str[18] = {0};

	DOCMDINIT;
	// Delete all Macfilter rule
	DOCMDARGVS(EBTABLES,DOWAIT,"-F %s",(char *)FW_PARENTAL_MAC_CTRL_LOCALIN);
	DOCMDARGVS(EBTABLES,DOWAIT,"-F %s",(char *)FW_PARENTAL_MAC_CTRL_FWD);

	cnt=mib_chain_total(MIB_PARENTALCTRL_MAC_TBL);
	for(index=0;index<cnt;index++)
	{
		mib_chain_get(MIB_PARENTALCTRL_MAC_TBL,index,&entry);
		if(entry.fwexist > 0)
		{	
			snprintf(mac_str, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
											entry.MACAddress[0], entry.MACAddress[1], entry.MACAddress[2],
											entry.MACAddress[3], entry.MACAddress[4], entry.MACAddress[5]);

			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -s %s -j %s", (char *)FW_PARENTAL_MAC_CTRL_LOCALIN,
				(char *)ELANIF, mac_str, FW_DROP);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -s %s -j %s", (char *)FW_PARENTAL_MAC_CTRL_FWD,
				(char *)ELANIF, mac_str, FW_DROP);
#ifdef WLAN_SUPPORT
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -s %s -j %s", (char *)FW_PARENTAL_MAC_CTRL_FWD,
				(char *)"wlan", mac_str, FW_DROP);
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -s %s -j %s", (char *)FW_PARENTAL_MAC_CTRL_LOCALIN,
				(char *)"wlan", mac_str, FW_DROP);
#endif


		}
	}

	rtk_hwnat_flow_flush();
	return 0;
}
#endif


#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#include <netinet/ether.h>
unsigned int rtk_mac_filter_get_drop_pkt_count(const char *macStr)
{
//#define IPTABLES_SRC_MAC_PATTERN "SRC MAC %s"

	FILE *fp=NULL;
	char cmd[1024]={0}, result[256]={0};
	unsigned int count=0, total_count=0;
	struct ether_addr mac;
	char ebtables_mac_string[32]={0};

	if(ether_aton_r(macStr, &mac) != NULL){
		snprintf(ebtables_mac_string, sizeof(ebtables_mac_string), "%x:%x:%x:%x:%x:%x",
			mac.ether_addr_octet[0], mac.ether_addr_octet[1], mac.ether_addr_octet[2],
			mac.ether_addr_octet[3], mac.ether_addr_octet[4], mac.ether_addr_octet[5]);

		snprintf(cmd, sizeof(cmd), "ebtables -L %s --Lc | grep -i '%s'", FW_MACFILTER_FWD, ebtables_mac_string);
		//printf(cmd, "cmd %s \n", cmd);
		fp = popen(cmd, "r");
		if(fp){
			while(fgets(result, sizeof(result), fp) != NULL){
				sscanf(result, "%*s %*s %*s %*s %*s %*s %*s pcnt = %u", &count);
				total_count += count;
				//printf("count %u \n", count);
			}
			pclose(fp);
			fp = NULL;
		}
		snprintf(cmd, sizeof(cmd), "ebtables -L %s --Lc | grep -i '%s'", FW_MACFILTER_LI, ebtables_mac_string);
		//printf(cmd, "cmd %s \n", cmd);
		fp = popen(cmd, "r");
		if(fp){
			while(fgets(result, sizeof(result), fp) != NULL){
				sscanf(result, "%*s %*s %*s %*s %*s %*s %*s pcnt = %u", &count);
				total_count += count;
				//printf("count %u \n", count);
			}
			pclose(fp);
			fp = NULL;
		}
	}

#if defined(WLAN_SUPPORT)
	total_count += get_wlan_MAC_ACL_BlockTimes(mac.ether_addr_octet);
#endif

	//printf("total_count %u \n", total_count);
	return total_count;
}
#endif

char* replaceChar(char*source, char found, char c)
{
	int i = 0;
	for(i=0; i < strlen(source); i++){
		if(source[i] == found){
			source[i] = c;
		}
	}
	return source;
}

// parse format  x-y/!x/x
// type 0: x-y
// type 1: !x
// type 2: x
// type 3: 0
int parseRemotePort(char* source, int*start, int*end)
{
	char *pch;
	int i = 0;
	char tmp[128] = {0};

	strncpy(tmp, source, sizeof(tmp)-1);

	if(tmp[0] == '!'){
		*start = atoi(&tmp[1]);
		*end = atoi(&tmp[1]);
		return 1;
	}
	else if(!strstr(tmp, "-")){//format x
		*start = atoi(tmp);
		*end = atoi(tmp);
		if(*start == 0 && *end == 0)
			return 3;
		else
			return 2;
	}
	else{//format x-y
		for(i=0; i < strlen(tmp); i++){
			if(tmp[i] == '-'){
				break;
			}
		}
		tmp[i] = 0;
		*start = atoi(tmp);
		*end = atoi(&tmp[i+1]);

		return 0;
	}
	return -1;
}


/*
 *	LAN port ifidx has been changed to VLAN-based port mapping.
 *	This routine resolve the possible port-mapping mode conflict.
 */
int sync_itfGroup(int ifidx)
{
	MIB_CE_ATM_VC_T vcEntry;
	int total, i;

	total = mib_chain_total(MIB_ATM_VC_TBL);

	for (i=0; i<total; i++) {
		mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry);
		if (vcEntry.itfGroup & (1<<ifidx)) {
			// Remove this port from port-based mapping as it is vlan-based mode now.
			vcEntry.itfGroup &= ~(1<<ifidx);
			mib_chain_update(MIB_ATM_VC_TBL, (void*)&vcEntry, i);
		}
	}
	return 0;
}

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD) \
	|| defined(CONFIG_USER_PPTP_CLIENT) || defined(CONFIG_USER_XL2TPD)
int renew_vpn_accpw_from_accpxy(VPN_TYPE_T vpn_type, unsigned char *sppp_if_name, unsigned char *new_username, unsigned char *new_password)
{
	gdbus_vpn_tunnel_info_t vpn_tunnel_info;
	unsigned char reason[128];
	int EntryNum, i;
	unsigned char if_name[IFNAMSIZ];
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	MIB_PPTP_T pptp_entry;
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	MIB_L2TP_T l2tp_entry;
#endif
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	if(vpn_type==VPN_TYPE_PPTP) {
		EntryNum = mib_chain_total(MIB_PPTP_TBL);
		for (i=0; i<EntryNum; i++)
		{
			if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&pptp_entry) )
				continue;

			snprintf(if_name, sizeof(if_name), "ppp%d", 9+pptp_entry.idx);
			if( !strcmp(sppp_if_name, if_name) ) {
				if(pptp_entry.vpn_mode == VPN_MODE_RANDOM) {
					printf("%s: before: username=%s password=%s\n", if_name, pptp_entry.username, pptp_entry.password);
					snprintf(vpn_tunnel_info.account_proxy, sizeof(vpn_tunnel_info.account_proxy), "%s", pptp_entry.account_proxy);
					snprintf(vpn_tunnel_info.userID, sizeof(vpn_tunnel_info.userID), "%s", pptp_entry.userID);
					snprintf(vpn_tunnel_info.tunnelName, sizeof(vpn_tunnel_info.tunnelName), "%s", pptp_entry.tunnelName);
					if( request_vpn_accpxy_server(&vpn_tunnel_info, reason)!=-1 ) {
						if ( mib_chain_get(MIB_PPTP_TBL, i, (void *)&pptp_entry) ) {
							sprintf(pptp_entry.username, "%s", vpn_tunnel_info.userName);
							sprintf(pptp_entry.password, "%s", vpn_tunnel_info.passwd);
							mib_chain_update(MIB_PPTP_TBL, &pptp_entry, i);
							printf("%s: after: username=%s password=%s\n", if_name, pptp_entry.username, pptp_entry.password);
							sprintf(new_username, "%s", vpn_tunnel_info.userName);
							sprintf(new_password, "%s", vpn_tunnel_info.passwd);
						} else {
							printf("%s: after: get MIB fail !\n", if_name);
						}
					}
				}
			}
		}
	}
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if(vpn_type==VPN_TYPE_L2TP) {
		EntryNum = mib_chain_total(MIB_L2TP_TBL);
		for (i=0; i<EntryNum; i++)
		{
			if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tp_entry) )
				continue;

			snprintf(if_name, sizeof(if_name), "ppp%d", 11+l2tp_entry.idx);
			if( !strcmp(sppp_if_name, if_name) ) {
				if(l2tp_entry.vpn_mode == VPN_MODE_RANDOM) {
					printf("%s: before: username=%s password=%s\n", if_name, l2tp_entry.username, l2tp_entry.password);
					snprintf(vpn_tunnel_info.account_proxy, sizeof(vpn_tunnel_info.account_proxy), "%s", l2tp_entry.account_proxy);
					snprintf(vpn_tunnel_info.userID, sizeof(vpn_tunnel_info.userID), "%s", l2tp_entry.userID);
					snprintf(vpn_tunnel_info.tunnelName, sizeof(vpn_tunnel_info.tunnelName), "%s", l2tp_entry.tunnelName);
					if( request_vpn_accpxy_server(&vpn_tunnel_info, reason)!=-1 ) {
						if ( mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tp_entry) ) {
							sprintf(l2tp_entry.username, "%s", vpn_tunnel_info.userName);
							sprintf(l2tp_entry.password, "%s", vpn_tunnel_info.passwd);
							mib_chain_update(MIB_L2TP_TBL, &l2tp_entry, i);
							printf("%s: after: username=%s password=%s\n", if_name, l2tp_entry.username, l2tp_entry.password);
							sprintf(new_username, "%s", vpn_tunnel_info.userName);
							sprintf(new_password, "%s", vpn_tunnel_info.passwd);
						} else {
							printf("%s: after: get MIB fail !\n", if_name);
						}
					}
				}
			}
		}
	}
#endif
	return 0;
}

extern void *VPNConnectionThread(void *arg);
VPN_STATUS_T WanVPNConnectStart
(
	gdbus_vpn_connection_info_t *vpn_connection_info,
	ATTACH_MODE_T attach_mode,
	unsigned char *reason
)
{
	pthread_t ntid = 0;
	int err = 0;

	err = pthread_create(&ntid, NULL, VPNConnectionThread, vpn_connection_info);
	if (err != 0) {
		printf("can't create thread: %s\n", strerror(err));
		sleep(1);
		return 0;
	}
	pthread_detach(ntid);
	return 0;
}

VPN_STATUS_T WanVPNConnection( gdbus_vpn_connection_info_t *vpn_connection_info, unsigned char *reason )
{
	unsigned char vpn_type = VPN_TYPE_NONE;
	unsigned char attach_mode = ATTACH_MODE_NONE;
	unsigned int vpn_port=0;
	int i;
	int status=0;
	int ret=0;

	if(vpn_connection_info==NULL || reason==NULL) {
		if (reason != NULL)
			strcpy(reason, "some parameter(s) exist NULL pointer!");
		ret=-1;
	}

	WanVPNConnectStart(vpn_connection_info,
					attach_mode,
					reason);

	return ret;
}

VPN_STATUS_T WanVPNConnectionStat( gdbus_vpn_connection_info_t *vpn_connection_info, unsigned char *status[] )
{
	int i, status_idx=0, attachSize=0;
	unsigned char **ipDomainNameAddr = NULL;
	unsigned char reason[100] = {0};
	VPN_TYPE_T vpn_type;

	if( vpn_connection_info==NULL ) {
		strcpy(reason, "some parameter(s) exist NULL pointer!");
		return VPN_STATUS_NG;
	}

	//vpn_connection_info->attach_mode = ATTACH_MODE_DIP;
	if(get_attach_pattern_by_mode(vpn_connection_info, &ipDomainNameAddr, &attachSize, reason) != 0)
		return VPN_STATUS_NG;

	if(ipDomainNameAddr == NULL || attachSize == 0)
	{
		if(ipDomainNameAddr)
			free(ipDomainNameAddr);
		return VPN_STATUS_NG;
	}
	vpn_type = vpn_connection_info->vpn_tunnel_info.vpn_type;

#ifndef VPN_MULTIPLE_RULES_BY_FILE
	for(i=0; i<attachSize; i++)
	{
		unsigned char temp_status[128];
		unsigned int packet_count;
		int route_idx = -1;
		unsigned char sMac[MAC_ADDR_LEN];
		char *ip=NULL, *p=NULL;
		unsigned int mask,tmp_mask;
		unsigned int ipv4_addr1=0,ipv4_addr2=0,tmp_addr;
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
		char str_ipaddr1[64]={0}, str_ipaddr2[64] ={0};
#endif

		ip = ipDomainNameAddr[i];
		if(ip == NULL || ip[0] == '\0')
		continue;
		//strcpy(vpn_connection_info->vpn_tunnel_info.tunnelName, MAGIC_TUNNEL_NAME);

		if(isIPAddr(ip)==1 || (p=strstr(ip,"/")) || (p=strstr(ip,"-")))
		{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
			if(p == NULL && getIpRange(ip, str_ipaddr1, str_ipaddr2) > 0)
			{
				inet_pton(AF_INET, str_ipaddr1, &ipv4_addr1);
				inet_pton(AF_INET, str_ipaddr2, &ipv4_addr2);
			}
			else
#endif
				if(p == NULL)
				{
					ipv4_addr1 = inet_addr(ip);
					ipv4_addr2 = ipv4_addr1;
				}
				else if(*p == '/')
				{
					*p = '\0';
					tmp_addr = ntohl(inet_addr(ip));
					tmp_mask = atoi(p+1);
					if(tmp_mask!=0){
						mask = ~((1<<(32-tmp_mask))-1);
						tmp_addr &= mask;
						ipv4_addr1 = htonl(tmp_addr);
						ipv4_addr2 = htonl(tmp_addr | ~mask);
						*p = '/';
					}
				}
				else if(*p == '-')
				{
					*p = '\0';
					ipv4_addr1 = inet_addr(ip);
					ipv4_addr2 = inet_addr(p+1);
					*p = '-';
				}

			switch(vpn_type)
			{
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
				case VPN_TYPE_PPTP:
					{
						route_idx=rtk_wan_vpn_pptp_attach_check_dip(vpn_connection_info->vpn_tunnel_info.tunnelName,ipv4_addr1,ipv4_addr2);
						break;
					}
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
				case VPN_TYPE_L2TP:
					{
						route_idx=rtk_wan_vpn_l2tp_attach_check_dip(vpn_connection_info->vpn_tunnel_info.tunnelName,ipv4_addr1,ipv4_addr2);
						break;
					}
#endif
			}

			if(route_idx>=0)
			{
				packet_count = rtk_wan_vpn_get_attached_pkt_count_by_route_index(vpn_type, route_idx);
				memset(temp_status, 0, sizeof(temp_status));
				snprintf(temp_status, sizeof(temp_status), "%s@%d", ip, packet_count);
				status[status_idx] = malloc(strlen(temp_status)+1);
				snprintf(status[status_idx], strlen(temp_status)+1, "%s@%d", ip, packet_count);
				status_idx++;
			}
			else
			{
				AUG_PRT("ipv4_addr1=%x, ipv4_addr2=%x not founded! \n",ipv4_addr1,ipv4_addr2);
			}
		}
		else if (strstr(ip,":"))
		{
			sscanf(ip, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &sMac[0], &sMac[1], &sMac[2], &sMac[3], &sMac[4], &sMac[5]);
			switch(vpn_type)
			{
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
				case VPN_TYPE_PPTP:
					{
						route_idx=rtk_wan_vpn_pptp_attach_check_smac(vpn_connection_info->vpn_tunnel_info.tunnelName,sMac);
						break;
					}
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
				case VPN_TYPE_L2TP:
					{
						route_idx=rtk_wan_vpn_l2tp_attach_check_smac(vpn_connection_info->vpn_tunnel_info.tunnelName,sMac);
						break;
					}
#endif
			}
			if(route_idx>=0)
			{
				packet_count = rtk_wan_vpn_get_attached_pkt_count_by_route_index(vpn_type, route_idx);
				memset(temp_status, 0, sizeof(temp_status));
				snprintf(temp_status, sizeof(temp_status), "%X%X%X%X%X%X@%d", sMac[0], sMac[1], sMac[2], sMac[3], sMac[4], sMac[5], packet_count);
				status[status_idx] = malloc(strlen(temp_status)+1);
				snprintf(status[status_idx], strlen(temp_status)+1, "%X%X%X%X%X%X@%d", sMac[0], sMac[1], sMac[2], sMac[3], sMac[4], sMac[5], packet_count);
				status_idx++;
			}
			else
			{
				AUG_PRT("sMAC=%s not founded ! \n",ip);
			}
		}
		else
		{
			switch(vpn_type)
			{
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
				case VPN_TYPE_PPTP:
					{
						route_idx=rtk_wan_vpn_pptp_attach_check_url(vpn_connection_info->vpn_tunnel_info.tunnelName,ip);
						break;
					}
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
				case VPN_TYPE_L2TP:
					{
						route_idx=rtk_wan_vpn_l2tp_attach_check_url(vpn_connection_info->vpn_tunnel_info.tunnelName,ip);
						break;
					}
#endif
			}

			if(route_idx>=0)
			{
				packet_count = rtk_wan_vpn_get_attached_pkt_count_by_route_index(vpn_type, route_idx);
				memset(temp_status, 0, sizeof(temp_status));
				snprintf(temp_status, sizeof(temp_status), "%s@%d", ip, packet_count);
				status[status_idx] = malloc(strlen(temp_status)+1);
				snprintf(status[status_idx], strlen(temp_status)+1, "%s@%d", ip, packet_count);
				status_idx++;
			}
			else
			{
				AUG_PRT("domain=%s not founded ! \n", ip);
			}
		}
	}
#else
	for(i=0; i<attachSize; i++)
	{
		unsigned char temp_status[128] = {0};
		unsigned int packet_count = 0;
		int route_idx = -1;
		unsigned char sMac[MAC_ADDR_LEN] = {0};
		char *ip = NULL, *p = NULL;
		unsigned int bits = 0, rule_mode = ATTACH_RULE_MODE_NONE;
		unsigned int ipv4_addr1 = 0, ipv4_addr2 = 0, tmp_addr = 0;
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
		char str_ipaddr1[64]={0}, str_ipaddr2[64] ={0};
#endif

		ip = ipDomainNameAddr[i];
		if (ip == NULL || ip[0] == '\0')
			continue;
		//strcpy(vpn_connection_info->vpn_tunnel_info.tunnelName, MAGIC_TUNNEL_NAME);

		if(isIPAddr(ip) == 1 || (p = strstr(ip, "/")) || (p = strstr(ip, "-")))
		{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
			if(p == NULL && getIpRange(ip, str_ipaddr1, str_ipaddr2) > 0)
			{
				inet_pton(AF_INET, str_ipaddr1, &ipv4_addr1);
				inet_pton(AF_INET, str_ipaddr2, &ipv4_addr2);
			}
			else
#endif
			if (p == NULL)
			{
				rule_mode = ATTACH_RULE_MODE_SINGLE_DIP;
			}
			else if (*p == '/')
			{
				*p = '\0';
				bits = atoi(p + 1);
				if (isIPAddr(ip) != 1 || !(bits > 0 && bits <= 32))
				{
					*p = '/';
					goto domain_case;
				}
				*p = '/';
				rule_mode = ATTACH_RULE_MODE_DIP_RANGE_SUBNET;
			}
			else if (*p == '-')
			{
				*p = '\0';
				if (isIPAddr(ip) != 1)
				{
					*p = '-';
					goto domain_case;
				}
				if (isIPAddr(p + 1) != 1)
				{
					*p = '-';
					goto domain_case;
				}
				*p = '-';
				rule_mode = ATTACH_RULE_MODE_DIP_RANGE;
			}

			switch(vpn_type)
			{
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
				case VPN_TYPE_PPTP:
					{
						//route_idx=rtk_wan_vpn_pptp_attach_check_dip(vpn_connection_info->vpn_tunnel_info.tunnelName,ipv4_addr1,ipv4_addr2);
						route_idx = rtk_wan_vpn_pptp_get_route_idx_with_template(vpn_connection_info->vpn_tunnel_info.tunnelName, rule_mode, ip);
						break;
					}
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
				case VPN_TYPE_L2TP:
					{
						//route_idx=rtk_wan_vpn_l2tp_attach_check_dip(vpn_connection_info->vpn_tunnel_info.tunnelName,ipv4_addr1,ipv4_addr2);
						route_idx = rtk_wan_vpn_l2tp_get_route_idx_with_template(vpn_connection_info->vpn_tunnel_info.tunnelName, rule_mode, ip);
						break;
					}
#endif
			}

			if(route_idx>=0)
			{
			    //packet_count = rtk_wan_vpn_get_attached_pkt_count_by_route_index(vpn_type, route_idx);
				packet_count = rtk_wan_vpn_get_ipset_packet_count_by_route_index(vpn_type, vpn_connection_info->vpn_tunnel_info.tunnelName, route_idx, rule_mode);
				memset(temp_status, 0, sizeof(temp_status));
				snprintf(temp_status, sizeof(temp_status), "%s@%d", ip, packet_count);
				status[status_idx] = malloc(strlen(temp_status)+1);
				snprintf(status[status_idx], strlen(temp_status)+1, "%s@%d", ip, packet_count);
				status_idx++;
			}
			else
			{
				AUG_PRT("ipv4_addr1=%x, ipv4_addr2=%x not founded! \n",ipv4_addr1,ipv4_addr2);
			}
		}
		else if (strstr(ip,":"))
		{
			rule_mode = ATTACH_RULE_MODE_SMAC;
			sscanf(ip, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &sMac[0], &sMac[1], &sMac[2], &sMac[3], &sMac[4], &sMac[5]);
			switch(vpn_type)
			{
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
				case VPN_TYPE_PPTP:
					{
						//route_idx=rtk_wan_vpn_pptp_attach_check_smac(vpn_connection_info->vpn_tunnel_info.tunnelName,sMac);
						route_idx = rtk_wan_vpn_pptp_get_route_idx_with_template(vpn_connection_info->vpn_tunnel_info.tunnelName, rule_mode, ip);
						break;
					}
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
				case VPN_TYPE_L2TP:
					{
						//route_idx=rtk_wan_vpn_l2tp_attach_check_smac(vpn_connection_info->vpn_tunnel_info.tunnelName,sMac);
						route_idx = rtk_wan_vpn_l2tp_get_route_idx_with_template(vpn_connection_info->vpn_tunnel_info.tunnelName, rule_mode, ip);
						break;
					}
#endif
			}
			if(route_idx>=0)
			{
				//packet_count = rtk_wan_vpn_get_attached_pkt_count_by_route_index(vpn_type, route_idx);
				packet_count = rtk_wan_vpn_get_ipset_packet_count_by_route_index(vpn_type, vpn_connection_info->vpn_tunnel_info.tunnelName, route_idx, rule_mode);
				memset(temp_status, 0, sizeof(temp_status));
				snprintf(temp_status, sizeof(temp_status), "%X%X%X%X%X%X@%d", sMac[0], sMac[1], sMac[2], sMac[3], sMac[4], sMac[5], packet_count);
				status[status_idx] = malloc(strlen(temp_status)+1);
				snprintf(status[status_idx], strlen(temp_status)+1, "%X%X%X%X%X%X@%d", sMac[0], sMac[1], sMac[2], sMac[3], sMac[4], sMac[5], packet_count);
				status_idx++;
			}
			else
			{
				AUG_PRT("sMAC=%s not founded ! \n",ip);
			}
		}
		else
		{
domain_case:
			rule_mode = ATTACH_RULE_MODE_DOMAIN;
			switch(vpn_type)
			{
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
				case VPN_TYPE_PPTP:
					{
						//route_idx=rtk_wan_vpn_pptp_attach_check_url(vpn_connection_info->vpn_tunnel_info.tunnelName,ip);
						route_idx = rtk_wan_vpn_pptp_get_route_idx_with_template(vpn_connection_info->vpn_tunnel_info.tunnelName, rule_mode, ip);
						break;
					}
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
				case VPN_TYPE_L2TP:
					{
						//route_idx=rtk_wan_vpn_l2tp_attach_check_url(vpn_connection_info->vpn_tunnel_info.tunnelName,ip);
						route_idx = rtk_wan_vpn_l2tp_get_route_idx_with_template(vpn_connection_info->vpn_tunnel_info.tunnelName, rule_mode, ip);
						break;
					}
#endif
			}

			if(route_idx>=0)
			{
				//packet_count = rtk_wan_vpn_get_attached_pkt_count_by_route_index(vpn_type, route_idx);
				packet_count = rtk_wan_vpn_get_ipset_packet_count_by_route_index(vpn_type, vpn_connection_info->vpn_tunnel_info.tunnelName, route_idx, rule_mode);
				memset(temp_status, 0, sizeof(temp_status));
				snprintf(temp_status, sizeof(temp_status), "%s@%d", ip, packet_count);
				status[status_idx] = malloc(strlen(temp_status)+1);
				snprintf(status[status_idx], strlen(temp_status)+1, "%s@%d", ip, packet_count);
				status_idx++;
			}
			else
			{
				AUG_PRT("domain=%s not founded ! \n", ip);
			}
		}
	}
#endif
	if(ipDomainNameAddr)
	{
		for(i=0; i<attachSize; i++)
		{
			if(ipDomainNameAddr[i] != NULL)
				free(ipDomainNameAddr[i]);
			ipDomainNameAddr[i] = NULL;
		}
		free(ipDomainNameAddr);
		ipDomainNameAddr = NULL;
	}

	return VPN_STATUS_OK;
}

int get_attach_pattern_by_mode
(
	gdbus_vpn_connection_info_t *vpn_connection_info,
	unsigned char ***attach_pattern,
	int *attach_pattern_size,
	unsigned char *reason
)
{
	int i, j=0, allocSize=0;
	int ret=0;
	char **allocPattern = NULL;

	if(attach_pattern == NULL || attach_pattern_size == NULL)
	{
		if(reason) strcpy(reason, "Invalid argument !");
		return -1;
	}

	switch( vpn_connection_info->attach_mode )
	{
		case ATTACH_MODE_DIP:
			i = 0;
			while( vpn_connection_info->domains[i++] != NULL) {
				allocSize++;
			}
			i = 0;
			while( vpn_connection_info->ips[i++] != NULL) {
				allocSize++;
			}
			break;
		case ATTACH_MODE_SMAC:
			i = 0;
			while( vpn_connection_info->terminal_mac[i++] != NULL) {
				allocSize++;
			}
			break;
		case ATTACH_MODE_ALL:
			i = 0;
			while( vpn_connection_info->domains[i++] != NULL) {
				allocSize++;
			}
			i = 0;
			while( vpn_connection_info->ips[i++] != NULL) {
				allocSize++;
			}
			i = 0;
			while( vpn_connection_info->terminal_mac[i++] != NULL) {
				allocSize++;
			}
			break;
		default:
			if(reason) strcpy(reason, "Invalid attach mode !");
			return -1;
	}

	allocPattern = (char **)calloc(sizeof(char*), allocSize);
	if(allocPattern == NULL)
	{
		if(reason) strcpy(reason, "No memory !");
		return -1;
	}

	j = 0;
	switch( vpn_connection_info->attach_mode )
	{
		case ATTACH_MODE_DIP:
			i = 0;
			while( vpn_connection_info->domains[i] != NULL)
			{
				allocPattern[j] = calloc(1, strlen(vpn_connection_info->domains[i])+1);
				strcpy(allocPattern[j], vpn_connection_info->domains[i]);
				i++; j++;
			}

			i = 0;
			while( vpn_connection_info->ips[i] != NULL)
			{
				allocPattern[j] = calloc(1, strlen(vpn_connection_info->ips[i])+1);
				strcpy(allocPattern[j], vpn_connection_info->ips[i]);
				i++; j++;
			}
			break;

		case ATTACH_MODE_SMAC:
			i = 0;
			while( vpn_connection_info->terminal_mac[i] != NULL)
			{
				allocPattern[j] = calloc(1, 18);
				if(strlen(vpn_connection_info->terminal_mac[i]) == 12)
					snprintf(allocPattern[j], 18, "%.02s:%.02s:%.02s:%.02s:%.02s:%.02s"
						, &vpn_connection_info->terminal_mac[i][0], &vpn_connection_info->terminal_mac[i][2], &vpn_connection_info->terminal_mac[i][4]
						, &vpn_connection_info->terminal_mac[i][6], &vpn_connection_info->terminal_mac[i][8], &vpn_connection_info->terminal_mac[i][10]);
				else if(strlen(vpn_connection_info->terminal_mac[i]) == 17)
					snprintf(allocPattern[j], 18, "%.02s:%.02s:%.02s:%.02s:%.02s:%.02s"
						, &vpn_connection_info->terminal_mac[i][0], &vpn_connection_info->terminal_mac[i][3], &vpn_connection_info->terminal_mac[i][6]
						, &vpn_connection_info->terminal_mac[i][9], &vpn_connection_info->terminal_mac[i][12], &vpn_connection_info->terminal_mac[i][15]);
				i++; j++;
			}
			break;
		case ATTACH_MODE_ALL:
			i = 0;
			while( vpn_connection_info->domains[i] != NULL)
			{
				allocPattern[j] = calloc(1, strlen(vpn_connection_info->domains[i])+1);
				strcpy(allocPattern[j], vpn_connection_info->domains[i]);
				i++; j++;
			}

			i = 0;
			while( vpn_connection_info->ips[i] != NULL)
			{
				allocPattern[j] = calloc(1, strlen(vpn_connection_info->ips[i])+1);
				strcpy(allocPattern[j], vpn_connection_info->ips[i]);
				i++; j++;
			}

			i = 0;
			while( vpn_connection_info->terminal_mac[i] != NULL)
			{
				allocPattern[j] = calloc(1, 18);
				if(strlen(vpn_connection_info->terminal_mac[i]) == 12)
					snprintf(allocPattern[j], 18, "%.02s:%.02s:%.02s:%.02s:%.02s:%.02s"
						, &vpn_connection_info->terminal_mac[i][0], &vpn_connection_info->terminal_mac[i][2], &vpn_connection_info->terminal_mac[i][4]
						, &vpn_connection_info->terminal_mac[i][6], &vpn_connection_info->terminal_mac[i][8], &vpn_connection_info->terminal_mac[i][10]);
				else if(strlen(vpn_connection_info->terminal_mac[i]) == 17)
					snprintf(allocPattern[j], 18, "%.02s:%.02s:%.02s:%.02s:%.02s:%.02s"
						, &vpn_connection_info->terminal_mac[i][0], &vpn_connection_info->terminal_mac[i][3], &vpn_connection_info->terminal_mac[i][6]
						, &vpn_connection_info->terminal_mac[i][9], &vpn_connection_info->terminal_mac[i][12], &vpn_connection_info->terminal_mac[i][15]);
				i++; j++;
			}
			break;
	}

	*attach_pattern = (unsigned char **)allocPattern;
	*attach_pattern_size = allocSize;
	if (reason != NULL && reason[0] != '\0')
		printf("%s: %d, reason=%s\n", __FUNCTION__, __LINE__, reason);
	return 0;
}
#endif //#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD)

static char base64_chars[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "abcdefghijklmnopqrstuvwxyz0123456789+/=";

/*
 * Name: base64_encode()
 *
 * Description: Encodes a buffer using BASE64.
 */
void base64_encode(unsigned char *from, char *to, int len)
{
	while (len) {
		unsigned long k;
		int c;

		c = (len < 3) ? len : 3;
		k = 0;
		if(len>=3) {
			len -= c;
			while (c--)
				k = (k << 8) | *from++;
			*to++ = base64_chars[ (k >> 18) & 0x3f ];
			*to++ = base64_chars[ (k >> 12) & 0x3f ];
			*to++ = base64_chars[ (k >> 6) & 0x3f ];
			*to++ = base64_chars[ k & 0x3f ];

		} else {
			switch (len) {
				case 1:
					k = (k << 8) | *from;
					*to++ = base64_chars[ (k >> 2) & 0x3f ];
					*to++ = base64_chars[ (k << 4) & 0x3f ];
					*to++ = base64_chars[ 64 ];
					*to++ = base64_chars[ 64 ];
					break;
				case 2:
					k = (k << 8) | *from++;
					k = (k << 8) | *from;
					*to++ = base64_chars[ (k >> 10) & 0x3f ];
					*to++ = base64_chars[ (k >> 4) & 0x3f ];
					*to++ = base64_chars[ (k << 2) & 0x3f ];
					*to++ = base64_chars[ 64 ];
					break;
			}
			len = 0;
		}

	}
	*to++ = 0;
}

/*
 * Name: base64encode()
 *
 * Description: Encodes a buffer using BASE64.
 */
void base64encode(unsigned char *from, char *to, int len)
{
	char base64chars[64] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";

  while (len) {
    unsigned long k;
    int c;

    c = (len < 3) ? len : 3;
    k = 0;
    len -= c;
    while (c--)
      k = (k << 8) | *from++;
    *to++ = base64chars[ (k >> 18) & 0x3f ];
    *to++ = base64chars[ (k >> 12) & 0x3f ];
    *to++ = base64chars[ (k >> 6) & 0x3f ];
    *to++ = base64chars[ k & 0x3f ];
  }
  *to++ = 0;
}

int base64_decode(void *dst,char *src,int maxlen)
{
 int bitval,bits;
 int val;
 int len,x,y;

 len = strlen(src);
 bitval=0;
 bits=0;
 y=0;

 for(x=0;x<len;x++)
  {
   if ((src[x]>='A')&&(src[x]<='Z')) val=src[x]-'A'; else
   if ((src[x]>='a')&&(src[x]<='z')) val=src[x]-'a'+26; else
   if ((src[x]>='0')&&(src[x]<='9')) val=src[x]-'0'+52; else
   if (src[x]=='+') val=62; else
   if (src[x]=='-') val=63; else
    val=-1;
   if (val>=0)
    {
     bitval=bitval<<6;
     bitval+=val;
     bits+=6;
     while (bits>=8)
      {
       if (y<maxlen)
        ((char *)dst)[y++]=(bitval>>(bits-8))&0xFF;
       bits-=8;
       bitval &= (1<<bits)-1;
      }
    }
  }
 if (y<maxlen)
   ((char *)dst)[y++]=0;
 return y;
}

int update_hosts_by_ip(char *hostname, char* ipaddr)
{
	if(!hostname || !ipaddr) return 0;

	struct in_addr ina;
	struct in6_addr ina6;

	FILE *fp;
	int isV4Existed = 0, isV6Existed = 0;
	char line[80], ip[64];
	int dnsrelaypid = -1;

	if(inet_pton(AF_INET6, hostname, &ina6) > 0 || inet_pton(AF_INET, hostname, &ina) > 0)
		return 0;

	fp = fopen(HOSTS, "a+");
	if(fp){
		while(fgets(line, 80, fp) != NULL)
		{
			char *ip,*name,*saveptr1;
			ip = strtok_r(line," \n", &saveptr1);
			name = strtok_r(NULL," \n", &saveptr1);

		if(name && !strcmp(name, hostname))
		{
			if(strchr(ip, '.') != NULL)
				isV4Existed = 1;
			else
				isV6Existed = 1;
		}
	}

		strncpy(ip, ipaddr, sizeof(ip));
		ip[sizeof(ip)-1] = '\0';

	if(inet_pton(AF_INET6, ipaddr, &ina6) > 0){
		if(!isV6Existed)
	 		fprintf(fp, "%-15s %s\n", ip, hostname);
	}

	if(inet_pton(AF_INET, ipaddr, &ina) > 0){
		if(!isV4Existed)
			fprintf(fp, "%-15s %s\n", ip, hostname);
	}

		fclose(fp);
	}

	dnsrelaypid = read_pid((char*)DNSRELAYPID);

	if(dnsrelaypid > 0)
		kill(dnsrelaypid, SIGHUP);

	return 1;
}

#ifdef HANDLE_DEVICE_EVENT
#if !defined(CONFIG_CMCC)
static pthread_t reset_led_thread = 0;
static void *  __reset_led_thread_func(void *arg)
{
	static int led_on = 0;
    pthread_detach(pthread_self());

	sigset_t mask;
    int rc;
	sigfillset(&mask);
    rc = pthread_sigmask(SIG_BLOCK, &mask, NULL);
    if (rc != 0) {
        fprintf(stderr, "[%s(%d)]: %d, %s\n", __FILE__, __LINE__, rc, strerror(rc));
		pthread_exit(NULL);
    }

	while(1)
	{
		if(led_on)
			system("mpctl led off");
		else
			system("mpctl led on");
		led_on = 1 - led_on;
		sleep(1);
	}
    pthread_exit(NULL);
}
#endif

int rtk_e8_pushbtn_reset_action(int action, int btn_test, int diff_time)
{
	static int reset = 0;
	static int last_diff_time = 0;

	if(btn_test){
		if((action==0) && (diff_time>=1))
			printf("Detect pressed reset button\n");
		return 0;
	}

	if(reset==1)
		return 0;

	if(action == 2){ //still press
		//handle led action every 1 sec
		if(diff_time <(last_diff_time+1))
			return 0;
		else
			last_diff_time = diff_time;
	}

	if(action == 0){ //release key
#ifdef CONFIG_CMCC
		if(diff_time >= 10)
		{
			reset = 1;
			printf("Going to Reload Default, longreset = 1\n");
#if defined(CONFIG_CMCC_ENTERPRISE) && defined(_PRMT_X_CMCC_DEVICEINFO_)
			unsigned int factoryreset_mode = 2; 
			mib_set(MIB_FACTORYRESET_MODE , &factoryreset_mode);
			Commit();
#endif
			reset_cs_to_default(3);
			cmd_reboot();
		}
#elif defined(CONFIG_CU)
		/*
			ChinaUnicom Spec:
			short reset: 3~5 s
			long reset: >= 10s
			
		*/
		if (diff_time < 3) {
			printf("ChinaUnicom:   Push Button do nothing.\n"); 
			last_diff_time = 0;
			system("mpctl led restore");
		} else if(diff_time >= 10) {
			reset = 1;
			printf("ChinaUnicom:   Going to Reload Default, longreset = 1\n");
			reset_cs_to_default(4);
			unsigned int ch = 0;
			mib_get_s(CWMP_FLAG2, (void *)&ch, sizeof(ch));
			ch &= ~(CWMP_FLAG2_HAD_SENT_LONGRESET);
			mib_set(CWMP_FLAG2, (void *)&ch);
			cmd_reboot();
		}
		else if(diff_time <= 5) {
			reset = 1;
			printf("ChinaUnicom:   Going to Reload Default, longreset = 0\n");
			reset_cs_to_default(3);
			cmd_reboot();

		}
		else{
			printf("ChinaUnicom:   Push Button do nothing.\n");
			system("mpctl led restore");
		}
#else
		if(diff_time < 2){
			printf("Push Button do nothing\n");
			last_diff_time = 0;
		}
		else if((diff_time >= 2) && (diff_time < 10)){
			reset = 1;
			printf("Going to Reload Default, longreset = 0\n");
			reset_cs_to_default(3);
			cmd_reboot();
		}
		else if(diff_time >= 10){
			reset = 1;
			printf("Going to Reload Default, longreset = 1\n");
			reset_cs_to_default(4);
			cmd_reboot();
		}
#endif
	}
	else if(action == 2){ //still press

#ifdef CONFIG_CMCC
		if(diff_time >= 10)
		{
			reset = 1;
			printf("Going to Reload Default, longreset = 1\n");
#if defined(CONFIG_CMCC_ENTERPRISE) && defined(_PRMT_X_CMCC_DEVICEINFO_)
			unsigned int factoryreset_mode = 2; 
			mib_set(MIB_FACTORYRESET_MODE , &factoryreset_mode);
			Commit();
#endif
			reset_cs_to_default(3);
			cmd_reboot();
		}
#else
		if(diff_time >= 10){
			reset = 1;
			printf("Going to Reload Default, longreset = 1\n");
			reset_cs_to_default(4);
#ifdef CONFIG_CU
			unsigned int ch = 0;
			mib_get_s(CWMP_FLAG2, (void *)&ch, sizeof(ch));
			ch &= ~(CWMP_FLAG2_HAD_SENT_LONGRESET);
			mib_set(CWMP_FLAG2, (void *)&ch);
#endif
			cmd_reboot();
		}
		else if(diff_time >= 2){
			int ret;
			if (reset_led_thread == 0) {
				ret = pthread_create(&reset_led_thread, NULL, &__reset_led_thread_func, NULL);
			}
		}
#endif
	}
	return 0;
}
#endif


#ifdef REMOTE_ACCESS_CTL
int set_icmp_Firewall(int action)
{
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_ICMP_FILTER);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)FW_ICMP_FILTER);
#endif

	if (action)
	{
		int i = 0, total = 0;
		char if_name[16] = {0}, policy[16] = {0};
		MIB_CE_ATM_VC_T entry;
		MIB_CE_ACC_T accEntry;

#ifdef _PRMT_SC_CT_COM_Ping_Enable_
		unsigned int function_mask = 0;
		mib_get_s(PROVINCE_SICHUAN_FUNCTION_MASK, &function_mask, sizeof(function_mask));
#endif

		total = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < total; i++)
		{
			memset(if_name, 0, sizeof(if_name));
			memset(policy, 0, sizeof(policy));
			if (mib_chain_get(MIB_ATM_VC_TBL, i, &entry) == 0)
				continue;

			ifGetName(entry.ifIndex, if_name, 16);

#ifdef _PRMT_SC_CT_COM_Ping_Enable_
			if ((function_mask & PROVINCE_SICHUAN_PING_ENABLE) != 0)
			{
				if (entry.applicationtype & X_CT_SRV_TR069)
				{
					strcpy(policy, FW_ACCEPT);
				}
			}
#endif

#ifdef CONFIG_YUEME
			if (strlen(policy) == 0)
			{
				if (entry.PingResponseEnable == 0)
					strcpy(policy, FW_DROP);
				else
				{
					if (strlen(entry.PingResponseWhiteList) == 0) /* accept all */
					{
						strcpy(policy, FW_ACCEPT);
					}
					else /* accept WhiteList, drop others */
					{
						char tmpList[512] = {0};
						char *pToken = NULL, *pSave = NULL;
						struct in_addr ina;
#ifdef CONFIG_IPV6
						struct in6_addr ina6;
#endif

						strcpy(policy, FW_DROP);

						sprintf(tmpList, "%s", entry.PingResponseWhiteList);
						pToken = strtok_r(tmpList, ",", &pSave);
						while (pToken)
						{
							printf("[%s@%d] pToken = %s\n", __FUNCTION__, __LINE__, pToken);
							if (inet_pton(AF_INET, pToken, &ina) > 0)
							{
								va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char *)FW_ICMP_FILTER,
									(char *)ARG_I, if_name,
									"-p", "icmp", "--icmp-type", "echo-request",
									"-s", pToken,
									"-j", (char *)FW_ACCEPT);
							}
#ifdef CONFIG_IPV6
							else if (inet_pton(AF_INET6, pToken, &ina6) > 0)
							{
								va_cmd(IP6TABLES, 12, 1, (char *)FW_ADD, (char *)FW_ICMP_FILTER,
									(char *)ARG_I, if_name,
									"-p", "icmpv6", "--icmpv6-type", "echo-request",
									"-s", pToken,
									"-j", (char *)FW_ACCEPT);
							}
#endif
							pToken = strtok_r(NULL, ",", &pSave);
						}
					}
				}
			}
#endif

			if (strlen(if_name) > 0 && strlen(policy) > 0)
			{
				va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, (char *)FW_ICMP_FILTER,
					(char *)ARG_I, if_name,
					"-p", "icmp", "--icmp-type", "echo-request",
					"-m", "limit", "--limit", "20/s",
					"-j", policy);
#ifdef CONFIG_IPV6
				if (entry.IpProtocol & IPVER_IPV6)
				{
					va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, (char *)FW_ICMP_FILTER,
						(char *)ARG_I, if_name,
						"-p", "icmpv6", "--icmpv6-type", "echo-request",
						"-m", "limit","--limit", "20/s",
						"-j", policy);
				}
#endif
			}
		}

		//Accept loopback device
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_ICMP_FILTER,
			(char *)ARG_I, LOCALHOST,
			"-p", "icmp", "--icmp-type", "echo-request",
			"-j", FW_ACCEPT);

#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_ICMP_FILTER,
			(char *)ARG_I, LOCALHOST,
			"-p", "icmpv6", "--icmpv6-type", "echo-request",
			"-j", FW_ACCEPT);
#endif

		if (!mib_chain_get(MIB_ACC_TBL, 0, (void *)&accEntry))
			return 0;

		if (accEntry.icmp & 0x01) // can WAN access
		{
			va_cmd(IPTABLES, 15, 1, (char *)FW_ADD, (char *)FW_ICMP_FILTER,
				"!", (char *)ARG_I, (char *)LANIF,
				"-p", "icmp", "--icmp-type", "echo-request",
				"-m", "limit", "--limit", "20/s",
				"-j", (char *)FW_ACCEPT);

#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 15, 1, (char *)FW_ADD, (char *)FW_ICMP_FILTER,
				"!", (char *)ARG_I, (char *)LANIF_ALL,
				"-p", "icmpv6", "--icmpv6-type", "echo-request",
				"-m", "limit","--limit", "20/s",
				"-j", (char *)FW_ACCEPT);
#endif
		}

		if (!(accEntry.icmp & 0x01))
		{
			va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_ICMP_FILTER,
				"!", (char *)ARG_I, (char *)LANIF_ALL,
				"-p", "icmp", "--icmp-type", "echo-request",
				"-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
			va_cmd(IP6TABLES, 11, 1, (char *)FW_ADD, (char *)FW_ICMP_FILTER,
				"!", (char *)ARG_I, (char *)LANIF_ALL,
				"-p", "icmpv6", "--icmpv6-type", "echo-request",
				"-j", (char *)FW_DROP);
#endif
		}
	}

	return 0;
}

#ifdef _PRMT_SC_CT_COM_Ping_Enable_
void rtk_cwmp_set_sc_ct_ping_enable(int enable)
{
	MIB_CE_ACC_T accEntry;
	if (!mib_chain_get(MIB_ACC_TBL, 0, (void *)&accEntry))
		return;
	accEntry.icmp = (enable == 0) ? (0x2):(0x3);
	mib_chain_update(MIB_ACC_TBL, (void *)&accEntry, 0);

	set_icmp_Firewall(0);
	set_icmp_Firewall(1);

	return;
}
#endif
#endif

#ifdef SUPPORT_WEB_PUSHUP
static struct callout_s upgradeWebPushUpTimer;
char firmware_upgrade_pushup_base_url[1024];

static char HTTP_FIRMWARE_UPGRADE_PUSHUP[] =
{
	"HTTP/1.0 200 OK\r\n"
	"Connection: keep-alive\r\n"
	"Content-Type: text/html\r\n"
	"\r\n"
	"<!doctype html>\r\n"
    "<html>\r\n"
    "<head>\r\n"
	"<meta http-equiv=\"Content-Type\" content=\"text/html\" charset=\"utf-8\">\r\n"
    "<title></title>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "<script>\r\n"
	"function openwin(u, w, h) {\r\n"
	"	var l = (screen.width - w)/2;\r\n"
	"	var t = (screen.height - h) / 2;\r\n"
	"	var s = 'width=' + w + ', height=' + h + ', top=' + t + ', left=' + l +"
	"      ', toolbar=no, scrollbars=no, menubar=no, location=no, resizable=no';\r\n"
	"	window.open(u, \"_blank\", s);\r\n"
	"}\r\n"
	"openwin(\"http://192.168.1.1/upgrade_pop.asp\", 400, 100);\r\n"
	"window.location.href=\"http://%s\";\r\n"
    "</script>\r\n"
    "</body>\r\n"
    "</html>"
};


/* This function still need TODO in FC constructure. */
int upgradeWebSet(int enable)
{
	char content[1024] = {0};

	if(enable)
	{
		memset(content, 0, sizeof(content));
		strcpy(content, HTTP_FIRMWARE_UPGRADE_PUSHUP);
		printf("content:\n%s\n", content);
	}
	else
	{
	}

	return 1;
}
static int isValidImageFile(const char *fname)
{
	int ret;
	char buf[256];

	// todo: validate the image file
	snprintf(buf, sizeof(buf), "/bin/tar tf %s md5.txt > /dev/null", fname);
        /* To prevent system() to return ECHILD */
        signal(SIGCHLD, SIG_DFL);
        fprintf(stderr, "%s\n", buf);
	ret = system(buf);

	return !ret;
}

int startUpgradeFirmware(int needreboot, char *downUrl)
{
	char filename[256];
	char userStr[100], passwdStr[100];
	char pathStr[100];
	char portStr[100];
	//char downUrl[128];
	struct stat statbuf;
	int ret;
	int urlLen, i;
#ifdef SUPPORT_WEB_PUSHUP
	firmwareUpgradeConfigStatusSet(FW_UPGRADE_STATUS_PROGGRESSING);
#endif
	snprintf(userStr, sizeof(userStr), "user=%s", "anonymous");
	snprintf(passwdStr, sizeof(passwdStr), "passwd=%s", "anonymous");
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	snprintf(pathStr, sizeof(pathStr), "path=%s", "/var/tmp2");
#else
	snprintf(pathStr, sizeof(pathStr), "path=%s", "/tmp");
#endif
	snprintf(portStr, sizeof(portStr), "port=%d", 21);

	//mib_get_s(MIB_FIRMWARE_DOWNURL, (void *)downUrl, sizeof(downUrl));

	va_cmd("/bin/wget_manage", 5, 1, userStr, passwdStr, pathStr, portStr, downUrl);

	/* get filename */
	urlLen = strlen(downUrl);
	if (downUrl[urlLen-1] == '/') downUrl[urlLen-1] = 0;

	for (i=urlLen-1; i>=0; i--)
	{
		if (downUrl[i] == '/')
			break;
	}
	if (i>=0)//finename found
	{
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
		snprintf(filename, sizeof(filename), "/var/tmp2/%s", &downUrl[i+1]);
#else
		snprintf(filename, sizeof(filename), "/tmp/%s", &downUrl[i+1]);
#endif
	}

	if((ret = stat(filename, &statbuf)) != 0 || statbuf.st_size == 0){
		printf("download image file fail\n");
		if(ret == 0) unlink(filename);
		firmwareUpgradeConfigStatusSet(FW_UPGRADE_STATUS_DOWNLOAD_FAIL);
		return 0;
	}

	if (!isValidImageFile(filename)) {
		printf("Incorrect image file\n");
		unlink(filename);
		firmwareUpgradeConfigStatusSet(FW_UPGRADE_STATUS_VER_INCORRECT);
		return 0;
	}

	// Save file for upgrade Firmware
	return cmd_upload(filename, 0, statbuf.st_size, needreboot);
}
static void upgradeWebPushupCheck(void)
{
	upgradeWebSet(1);
}

void startPushwebTimer(unsigned int time)//time unit: seconds
{
	TIMEOUT_F((void*)upgradeWebPushupCheck, 0, time, upgradeWebPushUpTimer);
}

#define	FIRMWARE_UPGRADE_STATUS		"/var/config/firmware_upgrade_status"

FW_UPGRADE_STATUS_T firmwareUpgradeConfigStatus( void )
{
	FILE *fp;
	char line[64];
	FW_UPGRADE_STATUS_T status;


	if(!(fp = fopen(FIRMWARE_UPGRADE_STATUS, "r")))
		return FW_UPGRADE_STATUS_NONE;

	fgets(line, 12, fp);
	sscanf(line, "%d", &status);

	fclose(fp);

	return status;
}
#if defined(CONFIG_YUEME)
// dbusproxy/reg/include/smart_proxy)common.h
#ifndef COM_CTC_SMART_APP_SYSTEMCMD_TAB
#define COM_CTC_SMART_APP_SYSTEMCMD_TAB 0x124
#define e_dbus_proxy_signal_firmware_upgrade_status_change_id 0x8100
#define e_dbus_proxy_signal_set_uapasswd_change_id 0x8102
#endif
void firmwareUpgradeStatusChgEmitSignal()
{
#ifdef CONFIG_USER_DBUS_PROXY
	mib2dbus_notify_app_t notifyMsg;
	memset((char*)&notifyMsg, 0, sizeof(notifyMsg));
	notifyMsg.table_id = COM_CTC_SMART_APP_SYSTEMCMD_TAB;
	notifyMsg.Signal_id = e_dbus_proxy_signal_firmware_upgrade_status_change_id;
	mib_2_dbus_notify_dbus_api(&notifyMsg);
#endif
}
//sync useradmin account passwd both with samba and ftp by yueme .
//10.1.8.1 FTP¿SAMBA ¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿
void SetUAPasswdChgEmitSignal(void)
{
#ifdef CONFIG_USER_DBUS_PROXY
	mib2dbus_notify_app_t notifyMsg;
	memset((char*)&notifyMsg, 0, sizeof(notifyMsg));
	notifyMsg.table_id = COM_CTC_SMART_APP_SYSTEMCMD_TAB;
	notifyMsg.Signal_id = e_dbus_proxy_signal_set_uapasswd_change_id;
	mib_2_dbus_notify_dbus_api(&notifyMsg);
#endif
}
#endif

void firmwareUpgradeConfigStatusClean(void)
{
	char buf[100];

	sprintf(buf, "/bin/echo %d > %s", FW_UPGRADE_STATUS_NONE, FIRMWARE_UPGRADE_STATUS);
	system(buf);
	sync();

#if defined(CONFIG_YUEME)
	firmwareUpgradeStatusChgEmitSignal();
#endif
}

void firmwareUpgradeConfigStatusSet( FW_UPGRADE_STATUS_T status )
{
	char buf[100];

	if(status > FW_UPGRADE_STATUS_NOTIFY)
	{
		sprintf(buf, "/bin/echo %d > %s", status, FIRMWARE_UPGRADE_STATUS);
		system(buf);
		sync();
	}

#if defined(CONFIG_YUEME)
	firmwareUpgradeStatusChgEmitSignal();
#endif
}

int firmwareUpgradeConfigSet(char *downUrl, int mode, int method, int needreboot, char *upgradeID)
{
	int ret = 0;
	/*sanity check for parameter*/
	if (NULL == downUrl)
	{
		printf("downURL should not be NULL.\n");
		firmwareUpgradeConfigStatusSet(FW_UPGRADE_STATUS_DOWNLOAD_FAIL);
		return 0;
	}

	if (strncmp(downUrl, "ftp://", 6) &&
		strncmp(downUrl, "http://", 7))
	{
		printf("wrong downURL!\n");
		firmwareUpgradeConfigStatusSet(FW_UPGRADE_STATUS_DOWNLOAD_FAIL);
		return 0;
	}

	if ((mode < 0) || (mode > 5))
	{
		printf("unknown mode, should be 0~5.\n");
		firmwareUpgradeConfigStatusSet(FW_UPGRADE_STATUS_OTHERS);
		return 0;
	}

	if ((method != 0) && (method != 1))
	{
		printf("unknow method, should be 0 or 1!\n");
		firmwareUpgradeConfigStatusSet(FW_UPGRADE_STATUS_OTHERS);
		return 0;
	}

	/* save the config */
	//mib_set(MIB_FIRMWARE_DOWNURL, (void *)downUrl);
	mib_set(MIB_UPGRADE_MODE, (void *)&mode);
	mib_set(MIB_UPGRADE_METHOD, (void *)&method);
	mib_set(MIB_UPGRADE_ID, (void *)upgradeID);

	/* take effect now */
	UNTIMEOUT_F(upgradeWebPushupCheck, 0, upgradeWebPushUpTimer);
	if (0 == mode)//upgrade firmware
	{
#if !defined(YUEME_3_0_SPEC)
		if (0 == method)
		{//proposal for upgrade
			ret = upgradeWebSet(1);
		}
		else if (1 == method)
#endif
		{//force to upgrade
			ret = startUpgradeFirmware(needreboot, downUrl);
		}
	}
	else
	{
		//TODO: upgrade OSGI framework, java machine, NOS, etc...
	}

	return ret;
}
#endif
#if defined(NESSUS_FILTER)
int setup_filter_nessus()
{
	system("iptables -N in_nes_filter");
	system("iptables -I INPUT -j in_nes_filter");

	system("iptables -I in_nes_filter -p tcp -m multiport --dports 445,8200,5555,49152:49155 -m state --state ESTABLISHED -m string --string \"essus\" --algo bm -j DROP");
#ifndef CONFIG_CU
	system("iptables -I in_nes_filter -p udp --dport 4011 -j DROP");
#endif
	system("iptables -I in_nes_filter -p udp --sport 53 -m state --state NEW -j DROP");
	system("iptables -I in_nes_filter -p udp --sport 53 -i br0 -j DROP");
#ifdef CONFIG_CU
	system("iptables -I in_nes_filter -p tcp --dport 53 -j DROP");
#else
	system("iptables -I in_nes_filter ! -i br+ -p tcp --dport 53 -j DROP");
#endif
	system("iptables -I in_nes_filter -p tcp -m multiport --dports 8080,8200,5555,49150:49155 -m state --state ESTABLISHED -m string --string \"User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0)\" --algo bm -m string --string \"Pragma: no-cache\" --algo bm -j DROP");
	system("iptables -I in_nes_filter -p tcp -m multiport --dports 8080,8200,5555,49150:49155 -m state --state ESTABLISHED -m string --string \"User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0)\" --algo bm -m string --string \"GET / HTTP/1.1\" --algo bm  -m string ! --string \"%\" --algo bm -m string ! --string \"Referer:\" --algo bm -j ACCEPT");
	system("iptables -I in_nes_filter -p tcp -m multiport --dports 8200,5555 -m state --state ESTABLISHED -m string --string \"User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0)\" --algo bm -m string --string \"POST /UD/?\" --algo bm -m string ! --string \"Referer:\" --algo bm -m string --string \"Pragma: no-cache\" --algo bm -j ACCEPT");
	system("iptables -I in_nes_filter -p tcp -m multiport --dports 8200,5555 -m state --state ESTABLISHED -m string --string \"User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0)\" --algo bm -m string --string \"GET /DeviceDescription.xml\" --algo bm -m string ! --string \"Referer:\" --algo bm -m string --string \"Pragma: no-cache\" --algo bm -j ACCEPT");
	system("iptables -I in_nes_filter -p icmp --icmp-type router-advertisement -m string --string \"XXXXXXXXXXXXXXXX\" --algo bm -j DROP");
#ifndef CONFIG_CU	// these packets should not be dropped to fix nessus 10496 and 10169
	system("iptables -I in_nes_filter -p tcp -m string --string \"XXXXXXXXXXXXXXXX\" --algo bm -j DROP");
	system("iptables -I in_nes_filter -p tcp -m string --string \"////////////////\" --algo bm -j DROP");
#endif
#ifdef CONFIG_CU
	// fix nessus 10496 IMail Host: Header Field Handling Remote Overflow
	//system("iptables -I in_nes_filter -p tcp -m string --string \"Host: XXXXXXXXXXXXXXXX\" --algo bm -j RETURN");
	// fix nessus 10169 OpenLink Web Configurator GET Request Remote Overflow
	//system("iptables -I in_nes_filter -p tcp -m string --string \"GET ////////////////\" --algo bm -j RETURN");
#endif

	system("ip6tables -N in_nes_filter");
	system("ip6tables -I INPUT -j in_nes_filter");

	system("ip6tables -I in_nes_filter -p tcp -m multiport --dports 445,8200,5555,49152:49155 -m state --state ESTABLISHED -m string --string \"essus\" --algo bm -j DROP");
	system("ip6tables -I in_nes_filter -p tcp -m multiport --dports 8080,8200,5555,49150:49155 -m state --state ESTABLISHED -m string --string \"User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0)\" --algo bm -m string --string \"Pragma: no-cache\" --algo bm -j DROP");
	system("ip6tables -I in_nes_filter -p tcp -m multiport --dports 8080,8200,5555,49150:49155 -m state --state ESTABLISHED -m string --string \"User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0)\" --algo bm -m string --string \"GET / HTTP/1.1\" --algo bm  -m string ! --string \"%\" --algo bm -m string ! --string \"Referer:\" --algo bm -j ACCEPT");
	system("ip6tables -I in_nes_filter -p udp --sport 53 -m state --state NEW -j DROP");
	system("ip6tables -I in_nes_filter -p udp --sport 53 -i br0 -j DROP");
#ifdef CONFIG_CU
	system("ip6tables -I in_nes_filter -p tcp --dport 53 -j DROP");
#else
	system("ip6tables -I in_nes_filter ! -i br+ -p tcp --dport 53 -j DROP");
#endif

	return 1;
}
#endif

void clearAlarm(unsigned int code)
{
#ifdef _PRMT_C_CU_LOGALARM_
	int i, ret, count=0;
	ALARMRECORD_TP alarmRecord=NULL;
	ret = getAlarmInfo(&alarmRecord, &count);

	if((count > 0) && (ret >= 0) && alarmRecord)
	{
		for(i=0; i<count; i++)
		{
			if(alarmRecord[i].alarmcode == code)
			{
				syslogAlarm(alarmRecord[i].alarmcode, alarmRecord[i].alarmstatus, alarmRecord[i].perceivedseverity, alarmRecord[i].logStr, 1);
				break;
			}
		}
	}

	if(alarmRecord)
	{
		free(alarmRecord);
		alarmRecord=NULL;
	}
#else

#endif
}

#ifdef _PRMT_C_CU_FTPSERVICE_
#define FTPSERVER_USERLIST_FILE "/var/ftp_userList"
void cu_ftp_server_accounts_init()
{
	FILE *fp;
	FTP_SERVER_T entry;
	int mibtotal=0, i;
	char buf[256];
	
	fp = fopen(FTPSERVER_USERLIST_FILE, "w");
	if (!fp)
		return;
	
	mibtotal = mib_chain_total(CWMP_FTP_SERVER);
	for (i=0; i<mibtotal; i++)
	{
		if (!mib_chain_get(CWMP_FTP_SERVER, i, (void *)&entry))
			continue;

		if (!entry.username[0])//account is invalid
			continue;

		memset(buf,0,sizeof(buf));
		sprintf(buf, "%s %s\n", entry.username,entry.password);
		fputs(buf, fp);

	}

	fclose(fp);
	chmod(PW_HOME_DIR, 0x1fd);	// let owner and group have write access
}
#endif

#ifdef CONFIG_USER_CTC_EOS_MIDDLEWARE
int start_eos_middleware()
{	
	if(findPidByName("init.ctc")>1)
		return 0;

	if(findPidByName("appd")>1)	
		va_cmd("/bin/killall", 1, 1, "appd");

	if(findPidByName("elinkclt")>1)		
		va_cmd("/bin/killall", 1, 1, "elinkclt");

	if(findPidByName("qlink")>1)
		va_cmd("/bin/killall", 1, 1, "qlink");	

	va_cmd("/bin/init.ctc", 0, 0);
	return 0;
}	

void rtk_sys_restart_elinkclt(int dowait, int debug)
{	
	char cmdBuf[128]={0};
	
	printf("\n%s:%d restart elinkclt!!!\n",__FUNCTION__,__LINE__);

	snprintf(cmdBuf, sizeof(cmdBuf), "/bin/ubus call ctcapd.appd stop '{\"appname\":\"elinkclt\"}'");
	//printf("\n%s:%d cmdBuf:%s\n", __FUNCTION__,__LINE__,cmdBuf);
	va_cmd("/bin/sh", 2, 1, "-c", cmdBuf);

	if(debug)
		snprintf(cmdBuf, sizeof(cmdBuf), "/bin/ubus call ctcapd.appd run '{\"appname\":\"elinkclt\",\"debug\":1}'");
	else
		snprintf(cmdBuf, sizeof(cmdBuf), "/bin/ubus call ctcapd.appd run '{\"appname\":\"elinkclt\"}'");
	
	//printf("\n%s:%d cmdBuf:%s\n", __FUNCTION__,__LINE__,cmdBuf);
	va_cmd("/bin/sh", 2, dowait, "-c", cmdBuf);
}

void rtk_sys_restart_qlink(int dowait, int debug)
{	
	char cmdBuf[128]={0};
	
	printf("\n%s:%d restart qlink!!!\n",__FUNCTION__,__LINE__);

	snprintf(cmdBuf, sizeof(cmdBuf), "/bin/ubus call ctcapd.appd stop '{\"appname\":\"qlink\"}'");
	//printf("\n%s:%d cmdBuf:%s\n", __FUNCTION__,__LINE__,cmdBuf);
	va_cmd("/bin/sh", 2, 1, "-c", cmdBuf);

	if(debug)
		snprintf(cmdBuf, sizeof(cmdBuf), "/bin/ubus call ctcapd.appd run '{\"appname\":\"qlink\",\"debug\":1}'");
	else
		snprintf(cmdBuf, sizeof(cmdBuf), "/bin/ubus call ctcapd.appd run '{\"appname\":\"qlink\"}'");
	
	//printf("\n%s:%d cmdBuf:%s\n", __FUNCTION__,__LINE__,cmdBuf);
	va_cmd("/bin/sh", 2, dowait, "-c", cmdBuf);
}
#endif

#ifdef CONFIG_USER_AVALANCH_DETECT
#ifdef CONFIG_RTL9607C_SERIES
#define RTK_CLS_AVALANCHE_INDEX_PATH "/var/rtk_cls_avalanche_index_"
#define AVA_L2_TABLE_LEARNING_PORT_NUM 16
static char avalache_l2_lookup_miss_bc_flood_port[16] = {0};
static char avalache_l2_lookup_miss_uc_flood_port[16] = {0};
static char avalache_l2_table_lookup_hit_uc_forward[16] = {0}; /* fb, fb-lut, lut */
struct avalache_l2_table_limit_learning_port_count_t
{
	unsigned short enable;
	int ori_value;
};
static struct avalache_l2_table_limit_learning_port_count_t ava_l2_table_limit_learning_port_count[AVA_L2_TABLE_LEARNING_PORT_NUM] = {0}; /* limit count value */


int rt_cls_avalanche_set(void)
{
	rt_cls_rule_t ClsRule;
	MIB_CE_ATM_VC_T vcEntry;
	int vcTotal = -1, i = 0, flowID = -1;
	unsigned int wanIfidx = 0, pon_mode = 0, cls_index = 0, qosEnable = 0;
	char ifname[IFNAMSIZ] = {0}, cls_avalanche_path_wan[128] = {0};
	FILE *fp = NULL;
#ifdef CONFIG_TR142_MODULE
	struct wanif_flowq wanq;
#endif

	if (!mib_get_s(MIB_PON_MODE, (void *)&pon_mode, sizeof(pon_mode)))
	{
		printf("%s: get MIB_PON_MODE fail!\n", __FUNCTION__);
		return -1;
	}
	if (!mib_get_s(MIB_QOS_ENABLE_QOS, (void *)&qosEnable, sizeof(qosEnable)))
		printf("%s: get MIB_QOS_ENABLE_QOS fail!\n", __FUNCTION__);
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0 ; i<vcTotal ; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
			continue;
		//find bridge wan
		if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
		{
			memset(&wanq, 0, sizeof(wanq));
			wanIfidx = vcEntry.ifIndex;
			ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));

			//UPstream
			memset(&ClsRule, 0, sizeof(ClsRule));
			//ClsRule.valid = ENABLED;
			ClsRule.index = cls_index;
			if (pon_mode == GPON_MODE) //GPON
			{
#ifdef CONFIG_TR142_MODULE
				wanq.wanIfidx = wanIfidx;
				if (get_wan_flowid(&wanq) == -1)
				{
					printf("<%s %d> not find valid wan(%u) streamid\n",__func__,__LINE__, wanIfidx);
					continue;
				}
				else
				{
					if (qosEnable)
						ClsRule.flowId = wanq.usflow[7]; //Should be set high queue.
					else
						ClsRule.flowId = wanq.usflow[0];
				}
#endif
			}
			else if (pon_mode == EPON_MODE) //EPON
				ClsRule.flowId = 0;
			printf("ClsRule.flowId = %d!\n", ClsRule.flowId);

			ClsRule.direction = RT_CLS_DIR_US;
			if (vcEntry.itfGroup != 0) //PHYPORT(LAN port);
				ClsRule.ingressPortMask = rtk_port_get_lan_phyMask(vcEntry.itfGroup);
				//ClsRule.ingressPortMask = rtk_port_get_lan_phyMask(vcEntry.itfGroup) | rtk_port_get_cpu_phyMask();
			else if (vcEntry.applicationtype & X_CT_SRV_INTERNET) //PHYPORT(ALL_LANPORT)
				ClsRule.ingressPortMask = rtk_port_get_all_lan_phyMask();
				//ClsRule.ingressPortMask = rtk_port_get_all_lan_phyMask() | rtk_port_get_cpu_phyMask();

			printf("ClsRule.ingressPortMask = %d!\n", ClsRule.ingressPortMask);
			if(vcEntry.vlan > 0)
			{
				ClsRule.innerTagAct.tagAction     = RT_CLS_TAG_TAGGING;
				ClsRule.innerTagAct.tagTpidAction = RT_CLS_TPID_ASSIGN;
				ClsRule.innerTagAct.tagVidAction  = RT_CLS_VID_ASSIGN;
				//if(vcEntry.vprio>0)
				//	ClsRule.innerTagAct.tagPriAction  = RT_CLS_PRI_ASSIGN;
				ClsRule.innerTagAct.assignedTpid  = 0x8100;
				ClsRule.innerTagAct.assignedVid   = vcEntry.vid;
			}
			snprintf(cls_avalanche_path_wan, sizeof(cls_avalanche_path_wan), "%s%s", RTK_CLS_AVALANCHE_INDEX_PATH, ifname);
			if ((fp = fopen(cls_avalanche_path_wan, "w")) == NULL)
			{
				printf("%s: open file %s fail!\n", __FUNCTION__, cls_avalanche_path_wan);
				continue;
			}
			if (RT_ERR_OK == rt_cls_rule_add(&ClsRule))
				fprintf(fp, "%d\n", cls_index);
			else
				printf("%s: cls rule add fail!\n", __FUNCTION__);

			cls_index++;
			fclose(fp);
		}
	}
	return 0;
}

int rt_cls_avalanche_del(void)
{
	MIB_CE_ATM_VC_T vcEntry;
	int vcTotal = -1, i = 0;
	unsigned int wanIfidx = 0, cls_index = 0;
	char ifname[IFNAMSIZ] = {0}, cls_avalanche_path_wan[128] = {0};
	FILE *fp = NULL;
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);

	for(i=0 ; i<vcTotal ; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
			continue;
		//find bridge wan
		//if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
		{
			fp = NULL;
			wanIfidx = vcEntry.ifIndex;
			ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));

			snprintf(cls_avalanche_path_wan, sizeof(cls_avalanche_path_wan), "%s%s", RTK_CLS_AVALANCHE_INDEX_PATH, ifname);
			if ((fp = fopen(cls_avalanche_path_wan, "r")) == NULL)
			{
				//printf("%s: open file %s fail!\n", __FUNCTION__, cls_avalanche_path_wan);
				continue;
			}

			if (fscanf(fp, "%d", &cls_index) == 1)
			{
				rt_cls_rule_delete(cls_index);
				//printf("%s: del %s rule!\n", __FUNCTION__, cls_avalanche_path_wan);
				unlink(cls_avalanche_path_wan);
			}
			if (fp)
				fclose(fp);
		}
	}

	return 0;
}

int diag_avalanche_vlan_set(void)
{
	MIB_CE_ATM_VC_T vcEntry;
	int vcTotal = -1, i = 0, lan_logic_port_num = 0, lan_logic_port = 0,lan_phy_port = 0;
	unsigned int wanIfidx = 0, cls_index = 0;
	char ifname[IFNAMSIZ] = {0}, cls_avalanche_path_wan[128] = {0};
	FILE *fp = NULL;
	char cmd[256] = {0};
	char lan_phy_port_list[16] = {0}, *lan_phy_port_list_p = NULL;

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
			continue;
		//find bridge wan
		if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
		{
			fp = NULL;
			wanIfidx = vcEntry.ifIndex;
			ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
			memset(lan_phy_port_list, 0, sizeof(lan_phy_port_list));
			lan_phy_port_list_p = &lan_phy_port_list[0];
			if (vcEntry.itfGroup)
			{
				for (lan_logic_port_num = 0; lan_logic_port_num < ELANVIF_NUM; lan_logic_port_num++)
				{
					//rtk_port_get_lan_phyID = rtk_port_get_lan_phyMask(vcEntry.itfGroup);
					if (BIT_IS_SET(vcEntry.itfGroup, lan_logic_port_num))
					{
						lan_phy_port = rtk_port_get_lan_phyID(lan_logic_port_num);
						lan_phy_port_list_p += snprintf(lan_phy_port_list_p, (&lan_phy_port_list[16] - lan_phy_port_list_p), "%d,", lan_phy_port);
					}
				}
				if (lan_phy_port_list[0])
					lan_phy_port_list[strlen(lan_phy_port_list) - 1] = '\0';

			}
			else //set all LAN port
			{
				for (lan_logic_port_num = 0; lan_logic_port_num < ELANVIF_NUM; lan_logic_port_num++)
				{
					//if (rtk_port_is_wan_logic_port(lan_logic_port_num))
					//	continue;
					lan_phy_port = rtk_port_get_lan_phyID(lan_logic_port_num);
					if (lan_phy_port != -1)
					{
						lan_phy_port_list_p += snprintf(lan_phy_port_list_p, (&lan_phy_port_list[16] - lan_phy_port_list_p), "%d,", lan_phy_port);
					}

				if (lan_phy_port_list[0])
					lan_phy_port_list[strlen(lan_phy_port_list) - 1] = '\0';
				}
			}
			if (lan_phy_port_list[0])
				printf("lan_phy_port_list = %s\n", lan_phy_port_list);

			snprintf(cmd, sizeof(cmd), "diag vlan create vlan-table vid %d", vcEntry.vid);
			system(cmd);
			//printf("cmd = %s\n", cmd);
			snprintf(cmd, sizeof(cmd), "diag vlan set vlan-table vid %d member %s", vcEntry.vid, lan_phy_port_list);
			system(cmd);
			//printf("cmd = %s\n", cmd);
			snprintf(cmd, sizeof(cmd), "diag vlan set vlan-table vid %d tag-member %d", vcEntry.vid, rtk_port_get_wan_phyID());
			system(cmd);
			//printf("cmd = %s\n", cmd);
			snprintf(cmd, sizeof(cmd), "diag vlan set pvid port %s %d", lan_phy_port_list, vcEntry.vid);
			system(cmd);
			//printf("cmd = %s\n", cmd);
			}
		}
	return 0;
}

int diag_avalanche_vlan_del(void)
{
	MIB_CE_ATM_VC_T vcEntry;
	int vcTotal = -1, i = 0;
	char cmd[256] = {0};

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
			continue;
		//find bridge wan
		if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
		{
			snprintf(cmd, sizeof(cmd), "diag vlan destroy vlan-table vid %d", vcEntry.vid);
			system(cmd);
			//printf("cmd = %s\n", cmd);
		}
	}
}


int diag_avalanche_l2_set(void)
{
	MIB_CE_ATM_VC_T vcEntry;
	int vcTotal = -1, i = 0, lan_logic_port_num = 0, lan_logic_port = 0,lan_phy_port = 0;
	unsigned int wanIfidx = 0, cls_index = 0;
	char ifname[IFNAMSIZ] = {0}, cls_avalanche_path_wan[128] = {0};
	FILE *fp_cmd = NULL;
	char cmd[256] = {0};
	char lan_phy_port_list[16] = {0}, *lan_phy_port_list_p = NULL;
	char tmp_cmd[256] = {0};
	char buf[256] = {0}, cmd_buf[256] = {0}, port_learning_limit_key[64] = {0};
	int port_index = 0, learning_limit = 0;

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
			continue;
		//find bridge wan
		if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
		{
			wanIfidx = vcEntry.ifIndex;
			ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
			memset(lan_phy_port_list, 0, sizeof(lan_phy_port_list));
			lan_phy_port_list_p = &lan_phy_port_list[0];
			if (vcEntry.itfGroup)
			{
				for (lan_logic_port_num = 0; lan_logic_port_num < ELANVIF_NUM; lan_logic_port_num++)
				{
					//rtk_port_get_lan_phyID = rtk_port_get_lan_phyMask(vcEntry.itfGroup);
					if (BIT_IS_SET(vcEntry.itfGroup, lan_logic_port_num))
					{
						lan_phy_port = rtk_port_get_lan_phyID(lan_logic_port_num);
						lan_phy_port_list_p += snprintf(lan_phy_port_list_p, (&lan_phy_port_list[16] - lan_phy_port_list_p), "%d,", lan_phy_port);
						/* Record origin phyport limit-learning count value. */
						snprintf(tmp_cmd, sizeof(tmp_cmd), "diag l2-table get limit-learning port all count");
						snprintf(port_learning_limit_key, sizeof(port_learning_limit_key), "Port %d", lan_phy_port);
						if ((fp_cmd = popen(tmp_cmd, "r")) != NULL)
						{
							while (fgets(buf, sizeof(buf), fp_cmd))
							{
								if (!strstr(buf, port_learning_limit_key)) //Like "Port 1"
									continue;
								if (sscanf(buf, "%*s%d%*s%*s%d ", &port_index, &learning_limit) != 2)
								{
									//printf("%s-%d sscanf fail!\n", __FUNCTION__, __LINE__);
									continue;
								}
								ava_l2_table_limit_learning_port_count[port_index].enable = 1;
								ava_l2_table_limit_learning_port_count[port_index].ori_value = learning_limit;
								//printf("%s [%d] value = %d!\n", __FUNCTION__, port_index, ava_l2_table_limit_learning_port_count[port_index].ori_value);
								break;
							}

							pclose(fp_cmd);
						}
						else
							printf("%s: popen %s failed!\n", __FUNCTION__, tmp_cmd);

					}
				}
				if (lan_phy_port_list[0])
					lan_phy_port_list[strlen(lan_phy_port_list) - 1] = '\0';
			}
			else //set all LAN port
			{
				for (lan_logic_port_num = 0; lan_logic_port_num < ELANVIF_NUM; lan_logic_port_num++)
				{
					lan_phy_port = rtk_port_get_lan_phyID(lan_logic_port_num);
					if (lan_phy_port != -1)
					{
						lan_phy_port_list_p += snprintf(lan_phy_port_list_p, (&lan_phy_port_list[16] - lan_phy_port_list_p), "%d,", lan_phy_port);
						/* Record origin phyport limit-learning count value. */
						snprintf(tmp_cmd, sizeof(tmp_cmd), "diag l2-table get limit-learning port all count");
						snprintf(port_learning_limit_key, sizeof(port_learning_limit_key), "Port %d", lan_phy_port);
						if ((fp_cmd = popen(tmp_cmd, "r")) != NULL)
						{
							while (fgets(buf, sizeof(buf), fp_cmd))
							{
								if (!strstr(buf, port_learning_limit_key)) //Like "Port 1"
									continue;
								//if (fscanf(fp, "%d", &cls_index) == 1)
								if (sscanf(buf, "%*s%d%*s%*s%d ", &port_index, &learning_limit) != 2)
								{
									//printf("%s-%d sscanf fail!\n", __FUNCTION__, __LINE__);
									continue;
								}
								ava_l2_table_limit_learning_port_count[port_index].enable = 1;
								ava_l2_table_limit_learning_port_count[port_index].ori_value = learning_limit;
								//printf("%s [%d] value = %d!\n", __FUNCTION__, port_index, ava_l2_table_limit_learning_port_count[port_index].ori_value);
								break;
							}
							pclose(fp_cmd);
						}
						else
							printf("%s: popen %s failed!\n", __FUNCTION__, tmp_cmd);
					}

					if (lan_phy_port_list[0])
						lan_phy_port_list[strlen(lan_phy_port_list) - 1] = '\0';
				}
			}
			if (lan_phy_port_list[0])
				printf("lan_phy_port_list = %s\n", lan_phy_port_list);

			/* Record origin l2 and l2-table setting */
			snprintf(tmp_cmd, sizeof(tmp_cmd), "diag l2 get lookup-miss broadcast flood-ports");
			if ((fp_cmd = popen(tmp_cmd, "r")) != NULL)
			{
				while (fgets(buf, sizeof(buf), fp_cmd))
				{
					//string: Lookup-miss Broadcast Lookup miss flood portmask: 9
					if (sscanf(buf, "%*s%*s%*s%*s%*s%*s%s", avalache_l2_lookup_miss_bc_flood_port) != 1)
					{
						//printf("%s sscanf avalache_l2_lookup_miss_bc_flood_port fail!\n", __FUNCTION__);
						continue;
					}
					else
						break;
				}
				pclose(fp_cmd);
			}
			else
				printf("%s: popen %s failed!\n", __FUNCTION__, tmp_cmd);

			snprintf(tmp_cmd, sizeof(tmp_cmd), "diag l2 get lookup-miss unicast flood-ports");
			if ((fp_cmd = popen(tmp_cmd, "r")) != NULL)
			{
				while (fgets(buf, sizeof(buf), fp_cmd))
				{
					//string: Lookup-miss Unicast Lookup miss flood portmask: 9
					if (sscanf(buf, "%*s%*s%*s%*s%*s%*s%s", avalache_l2_lookup_miss_uc_flood_port) != 1)
					{
						//printf("%s sscanf avalache_l2_lookup_miss_uc_flood_port fail!\n", __FUNCTION__);
						continue;
					}
					else
						break;
				}
				pclose(fp_cmd);
			}

			snprintf(tmp_cmd, sizeof(tmp_cmd), "diag l2-table get lookup-hit unicast forwarding");
			if ((fp_cmd = popen(tmp_cmd, "r")) != NULL)
			{
				while (fgets(buf, sizeof(buf), fp_cmd))
				{
					if (sscanf(buf, "%*s%s", cmd) != 1)
					{
						//printf("%s sscanf avalache_l2_lookup_miss_uc_forward fail!\n", __FUNCTION__);
						continue;
					}
					else
					{
						if (strstr(cmd, ":")) // string : decision:fb
						{
							snprintf(avalache_l2_table_lookup_hit_uc_forward, sizeof(avalache_l2_table_lookup_hit_uc_forward), "%s", strstr(cmd, ":") + 1);
							break;
						}
					}
				}
				pclose(fp_cmd);
			}

			/*Set avalanche rules */
			snprintf(cmd, sizeof(cmd), "diag l2 set lookup-miss broadcast flood-ports %s,%d", lan_phy_port_list, rtk_port_get_wan_phyID());
			system(cmd);
			//printf("cmd = %s\n", cmd);
			snprintf(cmd, sizeof(cmd), "diag l2 set lookup-miss unicast flood-ports %s,%d", lan_phy_port_list, rtk_port_get_wan_phyID());
			system(cmd);
			//printf("cmd = %s\n", cmd);
			snprintf(cmd, sizeof(cmd), "diag l2-table set lookup-hit unicast forwarding lut");
			system(cmd);
			//printf("cmd = %s\n", cmd);
			snprintf(cmd, sizeof(cmd), "diag l2-table set limit-learning port %s count unlimited", lan_phy_port_list);
			system(cmd);
			//printf("cmd = %s\n", cmd);
		}
	}
	return 0;
}


int diag_avalanche_l2_del(void)
{
	char cmd[256] = {0};
	char lan_phy_port_list[16] = {0}, *lan_phy_port_list_p = NULL;
	char tmp_cmd[128] = {0};
	char buf[256] = {0}, cmd_buf[256] = {0}, port_learning_limit_key[64] = {0};
	int port_index = 0, learning_limit = 0, i = 0;

	if (avalache_l2_lookup_miss_bc_flood_port[0] != '\0')
	{
		snprintf(cmd, sizeof(cmd), "diag l2 set lookup-miss broadcast flood-ports %s", avalache_l2_lookup_miss_bc_flood_port);
		//printf("cmd = %s\n", cmd);
		system(cmd);
		memset(avalache_l2_lookup_miss_bc_flood_port, 0, sizeof(avalache_l2_lookup_miss_bc_flood_port));
	}

	if (avalache_l2_lookup_miss_uc_flood_port[0] != '\0')
	{
		snprintf(cmd, sizeof(cmd), "diag l2 set lookup-miss unicast flood-ports %s", avalache_l2_lookup_miss_uc_flood_port);
		//printf("cmd = %s\n", cmd);
		system(cmd);
		memset(avalache_l2_lookup_miss_uc_flood_port, 0, sizeof(avalache_l2_lookup_miss_uc_flood_port));
	}

	if (avalache_l2_table_lookup_hit_uc_forward[0] != '\0')
	{
		snprintf(cmd, sizeof(cmd), "diag l2-table set lookup-hit unicast forwarding %s", avalache_l2_table_lookup_hit_uc_forward);
		//printf("cmd = %s\n", cmd);
		system(cmd);
		memset(avalache_l2_table_lookup_hit_uc_forward, 0, sizeof(avalache_l2_table_lookup_hit_uc_forward));
	}

	for (i = 0; i < AVA_L2_TABLE_LEARNING_PORT_NUM; i++)
	{
		if (ava_l2_table_limit_learning_port_count[i].enable == 1)
		{
			snprintf(cmd, sizeof(cmd), "diag l2-table set limit-learning port %d count %d", i, ava_l2_table_limit_learning_port_count[i].ori_value);
			system(cmd);
			//printf("cmd = %s\n", cmd);
			ava_l2_table_limit_learning_port_count[i].enable = 0;
			ava_l2_table_limit_learning_port_count[i].ori_value = 0;

		}
	}

	return 0;
}

#endif

#if defined(CONFIG_LUNA_G3_SERIES)
const char AVALANCHE_FW_CONTROL_PACKET_PROTECTION[] = "ava_control_packet_protection";

int rtk_avalanche_ebtables(int set)
{
	char cmd_str[256] = {0}, ifname[IFNAMSIZ] = {0};
	unsigned int mark = 0, qid = 0;
	MIB_CE_ATM_VC_T vcEntry;
	int vcTotal = -1, i = 0;

	va_cmd("/bin/sh", 2, 1, "-c", cmd_str);

#ifdef CONFIG_USER_IP_QOS
	qid = getIPQosQueueNumber() - 1;
#endif
	mark |= (qid)<<SOCK_MARK_QOS_SWQID_START;

	va_cmd_no_error(EBTABLES, 6, 1, "-t", "filter", "-D", "FORWARD", "-j", AVALANCHE_FW_CONTROL_PACKET_PROTECTION);
	va_cmd_no_error(EBTABLES, 4, 1, "-t", "filter", "-F", AVALANCHE_FW_CONTROL_PACKET_PROTECTION);
	va_cmd_no_error(EBTABLES, 4, 1, "-t", "filter", "-X", AVALANCHE_FW_CONTROL_PACKET_PROTECTION);
	if (set == 1)
	{
		va_cmd(EBTABLES, 4, 1, "-t", "filter", "-N", AVALANCHE_FW_CONTROL_PACKET_PROTECTION);
		va_cmd(EBTABLES, 5, 1, "-t", "filter", "-P", AVALANCHE_FW_CONTROL_PACKET_PROTECTION, "RETURN");
		va_cmd(EBTABLES, 6, 1, "-t", "filter", "-I", "FORWARD", "-j", AVALANCHE_FW_CONTROL_PACKET_PROTECTION);

		vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < vcTotal; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
				continue;
			//find bridge wan
			if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
			{
				ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));

				//ebtables -t filtwe -I FORWARD -o nas0_1 -j mark --mark-or 0x70
				snprintf(cmd_str, sizeof(cmd_str), "%s -t filter -A %s -o %s -j mark --mark-or 0x%x", EBTABLES, AVALANCHE_FW_CONTROL_PACKET_PROTECTION, ifname, mark);
				va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
			}
		}
	}

	return 0;
}
#endif
#endif

#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
int check_fwupgrade_phase(void) 
{
	FILE *fp = NULL;
	int phase = 0;
	
	if((fp=fopen(FWUPGRADE_UPDATE, "r"))) {
		fscanf(fp, "%d", &phase);
		fclose(fp);
	}
	return phase;
}

void update_fwupgrade_phase(int phase)
{
	char redir_port[16];
	FILE *fp = NULL;
	
	va_cmd(IPTABLES, 4, 1, "-t", "nat", FW_FLUSH, FW_PUSHWEB_FOR_UPGRADE);
	va_cmd(IPSET, 2, 1, "destroy", FWUPGRADE_SET);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "nat", FW_FLUSH, FW_PUSHWEB_FOR_UPGRADE);
	va_cmd(IPSET, 2, 1, "destroy", FWUPGRADE_SETV6);
#endif
	if((phase == 1) || (phase == 2))
	{
		va_cmd(IPSET, 5, 1, "create", FWUPGRADE_SET, "hash:ip", "timeout", "60");
		va_cmd(IPTABLES, 11, 1, "-t", "nat", FW_ADD, FW_PUSHWEB_FOR_UPGRADE, "-m", "set", "--match-set", FWUPGRADE_SET, "src", "-j", "RETURN");
#ifdef CONFIG_IPV6
		va_cmd(IPSET, 7, 1, "create", FWUPGRADE_SETV6, "hash:ip", "timeout", "60", "family", "inet6");
		va_cmd(IP6TABLES, 11, 1, "-t", "nat", FW_ADD, FW_PUSHWEB_FOR_UPGRADE, "-m", "set", "--match-set", FWUPGRADE_SETV6, "src", "-j", "RETURN");
#endif
		snprintf(redir_port, sizeof(redir_port), "%d", WEB_REDIR_PORT_BY_FWUPGRADE);
		va_cmd(IPTABLES, 12, 1, "-t", "nat", FW_ADD, FW_PUSHWEB_FOR_UPGRADE, "-p", "tcp", "--dport", "80", "-j", "REDIRECT", "--to-port", redir_port);
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 12, 1, "-t", "nat", FW_ADD, FW_PUSHWEB_FOR_UPGRADE, "-p", "tcp", "--dport", "80", "-j", "REDIRECT", "--to-port", redir_port);
#endif

		if((fp=fopen(FWUPGRADE_UPDATE, "w"))) {
			fprintf(fp, "%d\n", phase);
			fclose(fp);
		}
	}
	else {
		unlink(FWUPGRADE_UPDATE);
	}
	
    mib_set(CWMP_DL_PHASE, (void *)&phase);
}
#endif//endof SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE

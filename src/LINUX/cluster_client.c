#include <sys/types.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <linux/if_ether.h>
#include <linux/if.h>
#include <netinet/ip.h>  
#include <linux/sockios.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <signal.h> 
#include <sys/param.h> 
#include <sys/stat.h> 
#include <sys/un.h> 

#include "cluster_common.h"
#include "mib.h"
#include "mibtbl.h"

#define CLU_CLIENT_INTERVAL 15
//total port = lan ports + wlan ports(exclude vxd)
#define CLU_LAN_PORT_NUM	4
#define CLU_MAX_LAN_WLAN_PORT_NUM	((1+WLAN_MBSSID_NUM)*NUM_WLAN_INTERFACE+CLU_LAN_PORT_NUM)

static int cluster_net_sock_client;
static struct sockaddr_ll clu_ll_addr;
static unsigned short cur_seq;
static unsigned short next_seq;
static unsigned char broadcast_mac[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static int chain_idx;
static int lan_ifindex[CLU_MAX_LAN_WLAN_PORT_NUM];

static void log_pid()
{
	FILE *f;
	pid_t pid;
	pid = getpid();

	if((f = fopen(CLU_CLIENT_RUNFILE, "w")) == NULL)
		return;
	fprintf(f, "%d\n", pid);
	fclose(f);
}

static int cluster_client_init(void)
{
    char elan_mac[ETH_ALEN] = {0};
    struct ifreq ifstruct;
	int i=0, j=0, idx=0, maxlen = CLUSTER_PKT_MAX_LEN;
	
	cluster_net_sock_client = socket(AF_PACKET, SOCK_RAW, htons(CLUSTER_MANAGE_PROTOCOL));

	//init lan ports
	for(i = 0 ; i < CLU_LAN_PORT_NUM ; i++)
	{
		sprintf(ifstruct.ifr_ifrn.ifrn_name, "eth0.%d", i+2);
		if(ioctl(cluster_net_sock_client, SIOCGIFINDEX, &ifstruct)==-1)
		{
			printf("%s Get %s ifindex failed\n", __func__, ifstruct.ifr_ifrn.ifrn_name);
			return -1;
		}
		lan_ifindex[idx] = ifstruct.ifr_ifindex;
		debug_print("lan_ifindex[%d]:%x(%s)\n", idx, lan_ifindex[idx], ifstruct.ifr_ifrn.ifrn_name);
		idx++;
	}

	//init wlan ports
	for(i = 0 ; i < NUM_WLAN_INTERFACE ; i++)
	{
		for(j = 0 ; j<(1+WLAN_MBSSID_NUM); j++)
		{
			if(j==0)
				sprintf(ifstruct.ifr_ifrn.ifrn_name, WLAN_IF, i);
			else
				sprintf(ifstruct.ifr_ifrn.ifrn_name, VAP_IF, i, j-1);
			if(ioctl(cluster_net_sock_client, SIOCGIFINDEX, &ifstruct)==-1)
			{
				printf("%s Get %s ifindex failed\n", __func__, ifstruct.ifr_ifrn.ifrn_name);
				return -1;
			}
			lan_ifindex[idx] = ifstruct.ifr_ifindex;
			debug_print("lan_ifindex[%d]:%x(%s)\n", idx, lan_ifindex[idx], ifstruct.ifr_ifrn.ifrn_name);
			idx++;
		}
	}
    
    if (!mib_get(MIB_ELAN_MAC_ADDR, (void *)elan_mac))
    {
        printf("Get ELAN MAC Failed\n");
        return -1;
    }

    memset(&clu_ll_addr, 0, sizeof(struct sockaddr_ll));
    clu_ll_addr.sll_family = AF_PACKET; 
    clu_ll_addr.sll_halen = ETH_ALEN;
	clu_ll_addr.sll_protocol = htons(CLUSTER_MANAGE_PROTOCOL);
    memcpy(clu_ll_addr.sll_addr, elan_mac, ETH_ALEN);
	for(i=0; i<ETH_ALEN; i++)
		debug_print("[%d]:0x%02x\n", i, clu_ll_addr.sll_addr[i]);

    chain_idx = 0;
	return 0;
}

/* Signal handler */
static void signal_handler(int sig)
{
	//when receive SIGUSR1, means configration changed. We need to update sequence for next configration pacekts flood.
	if(SIGUSR1 == sig)
	{
        if(next_seq == cur_seq)
        {
            next_seq++;
            debug_print("cur_seq=%d, next_seq=%d\n", cur_seq, next_seq);
        }
	}
}

/*
 return:
      0 if add success
    -1 if length exceed the CLUSTER_PKT_MAX_LEN
    -2 if other error exist such as mib get failed.
 */
static int addOption(unsigned short type_idx, unsigned char *ether_frame, unsigned short len, unsigned short *optlen, MIB_CE_MBSSIB_Tp entry)
{
    OPT_TLV_Tp opt;
    unsigned short opt_len;
    unsigned char vChar;
    unsigned int vInt;
    int orig_wlanid;
    int total;
    int i;

    orig_wlanid = wlan_idx;
    opt = (OPT_TLV_Tp)(ether_frame + len);
    if(entry)
    {
        switch (type_idx)
        {
            case WLAN0_DISABLED:
            case WLAN0_VAP1_DISABLED:
            case WLAN0_VAP2_DISABLED:
            case WLAN0_VAP3_DISABLED:
            case WLAN0_VAP4_DISABLED:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_DISABLED:
            case WLAN1_VAP1_DISABLED:
            case WLAN1_VAP2_DISABLED:
            case WLAN1_VAP3_DISABLED:
            case WLAN1_VAP4_DISABLED:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->wlanDisabled;
                //memcpy(opt->data, &(entry->wlanDisabled), 1);
                break;
            case WLAN0_SSID:
            case WLAN0_VAP1_SSID:
            case WLAN0_VAP2_SSID:
            case WLAN0_VAP3_SSID:
            case WLAN0_VAP4_SSID:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_SSID:
            case WLAN1_VAP1_SSID:
            case WLAN1_VAP2_SSID:
            case WLAN1_VAP3_SSID:
            case WLAN1_VAP4_SSID:
#endif
                opt_len = 4 + strlen(entry->ssid);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                memcpy(opt->data, entry->ssid, strlen(entry->ssid));
                break;
			case WLAN0_WLANMODE:
            //case WLAN0_VAP1_WLANMODE:
            //case WLAN0_VAP2_WLANMODE:
            //case WLAN0_VAP3_WLANMODE:
            //case WLAN0_VAP4_WLANMODE:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_WLANMODE:
            //case WLAN1_VAP1_WLANMODE:
            //case WLAN1_VAP2_WLANMODE:
            //case WLAN1_VAP3_WLANMODE:
            //case WLAN1_VAP4_WLANMODE:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->wlanMode;
				break;
			case WLAN0_HIDESSID:
            //case WLAN0_VAP1_HIDESSID:
            //case WLAN0_VAP2_HIDESSID:
            //case WLAN0_VAP3_HIDESSID:
            //case WLAN0_VAP4_HIDESSID:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_HIDESSID:
            //case WLAN1_VAP1_HIDESSID:
            //case WLAN1_VAP2_HIDESSID:
            //case WLAN1_VAP3_HIDESSID:
            //case WLAN1_VAP4_HIDESSID:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->hidessid;
				break;
			case WLAN0_USERISOLATION:
            //case WLAN0_VAP1_USERISOLATION:
            //case WLAN0_VAP2_USERISOLATION:
            //case WLAN0_VAP3_USERISOLATION:
            //case WLAN0_VAP4_USERISOLATION:
#ifdef WLAN_DUALBAND_CONCURRENT
            //case WLAN1_USERISOLATION:
            //case WLAN1_VAP1_USERISOLATION:
            //case WLAN1_VAP2_USERISOLATION:
            //case WLAN1_VAP3_USERISOLATION:
            //case WLAN1_VAP4_USERISOLATION:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->userisolation;
				break;
			case WLAN0_WLANBAND:
            //case WLAN0_VAP1_WLANBAND:
            //case WLAN0_VAP2_WLANBAND:
            //case WLAN0_VAP3_WLANBAND:
            //case WLAN0_VAP4_WLANBAND:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_WLANBAND:
            //case WLAN1_VAP1_WLANBAND:
            //case WLAN1_VAP2_WLANBAND:
            //case WLAN1_VAP3_WLANBAND:
            //case WLAN1_VAP4_WLANBAND:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->wlanBand;
				break;
			//WiFi security start
            case WLAN0_ENCRYPT:
            case WLAN0_VAP1_ENCRYPT:
            case WLAN0_VAP2_ENCRYPT:
            case WLAN0_VAP3_ENCRYPT:
            case WLAN0_VAP4_ENCRYPT:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_ENCRYPT:
            case WLAN1_VAP1_ENCRYPT:
            case WLAN1_VAP2_ENCRYPT:
            case WLAN1_VAP3_ENCRYPT:
            case WLAN1_VAP4_ENCRYPT:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->encrypt;
                //memcpy(opt->data, &(entry->encrypt), 1);
                break;
			case WLAN0_WEP:
            case WLAN0_VAP1_WEP:
            case WLAN0_VAP2_WEP:
            case WLAN0_VAP3_WEP:
            case WLAN0_VAP4_WEP:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_WEP:
            case WLAN1_VAP1_WEP:
            case WLAN1_VAP2_WEP:
            case WLAN1_VAP3_WEP:
            case WLAN1_VAP4_WEP:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->wep;
				break;
			case WLAN0_WPAAUTH:
            case WLAN0_VAP1_WPAAUTH:
            case WLAN0_VAP2_WPAAUTH:
            case WLAN0_VAP3_WPAAUTH:
            case WLAN0_VAP4_WPAAUTH:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_WPAAUTH:
            case WLAN1_VAP1_WPAAUTH:
            case WLAN1_VAP2_WPAAUTH:
            case WLAN1_VAP3_WPAAUTH:
            case WLAN1_VAP4_WPAAUTH:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->wep;
				break;
			case WLAN0_WPAPSKFORMAT:
            case WLAN0_VAP1_WPAPSKFORMAT:
            case WLAN0_VAP2_WPAPSKFORMAT:
            case WLAN0_VAP3_WPAPSKFORMAT:
            case WLAN0_VAP4_WPAPSKFORMAT:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_WPAPSKFORMAT:
            case WLAN1_VAP1_WPAPSKFORMAT:
            case WLAN1_VAP2_WPAPSKFORMAT:
            case WLAN1_VAP3_WPAPSKFORMAT:
            case WLAN1_VAP4_WPAPSKFORMAT:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->wpaPSKFormat;
				break;
			case WLAN0_WPAPSK:
            case WLAN0_VAP1_WPAPSK:
            case WLAN0_VAP2_WPAPSK:
            case WLAN0_VAP3_WPAPSK:
            case WLAN0_VAP4_WPAPSK:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_WPAPSK:
            case WLAN1_VAP1_WPAPSK:
            case WLAN1_VAP2_WPAPSK:
            case WLAN1_VAP3_WPAPSK:
            case WLAN1_VAP4_WPAPSK:
#endif
                opt_len = 4 + strlen(entry->wpaPSK);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                memcpy(opt->data, entry->wpaPSK, strlen(entry->wpaPSK));
				break;
			case WLAN0_WPAGROUPREKEYTIME:
            case WLAN0_VAP1_WPAGROUPREKEYTIME:
            case WLAN0_VAP2_WPAGROUPREKEYTIME:
            case WLAN0_VAP3_WPAGROUPREKEYTIME:
            case WLAN0_VAP4_WPAGROUPREKEYTIME:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_WPAGROUPREKEYTIME:
            case WLAN1_VAP1_WPAGROUPREKEYTIME:
            case WLAN1_VAP2_WPAGROUPREKEYTIME:
            case WLAN1_VAP3_WPAGROUPREKEYTIME:
            case WLAN1_VAP4_WPAGROUPREKEYTIME:
#endif
                opt_len = 4 + sizeof(entry->wpaGroupRekeyTime);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                *(unsigned long *)(opt->data) = htonl(entry->wpaGroupRekeyTime);
				break;
			case WLAN0_AUTHTYPE:
            case WLAN0_VAP1_AUTHTYPE:
            case WLAN0_VAP2_AUTHTYPE:
            case WLAN0_VAP3_AUTHTYPE:
            case WLAN0_VAP4_AUTHTYPE:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_AUTHTYPE:
            case WLAN1_VAP1_AUTHTYPE:
            case WLAN1_VAP2_AUTHTYPE:
            case WLAN1_VAP3_AUTHTYPE:
            case WLAN1_VAP4_AUTHTYPE:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->authType;
				break;
			case WLAN0_UNICASTCIPHER:
            case WLAN0_VAP1_UNICASTCIPHER:
            case WLAN0_VAP2_UNICASTCIPHER:
            case WLAN0_VAP3_UNICASTCIPHER:
            case WLAN0_VAP4_UNICASTCIPHER:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_UNICASTCIPHER:
            case WLAN1_VAP1_UNICASTCIPHER:
            case WLAN1_VAP2_UNICASTCIPHER:
            case WLAN1_VAP3_UNICASTCIPHER:
            case WLAN1_VAP4_UNICASTCIPHER:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->unicastCipher;
				break;
			case WLAN0_WPA2UNICASTCIPHER:
            case WLAN0_VAP1_WPA2UNICASTCIPHER:
            case WLAN0_VAP2_WPA2UNICASTCIPHER:
            case WLAN0_VAP3_WPA2UNICASTCIPHER:
            case WLAN0_VAP4_WPA2UNICASTCIPHER:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_WPA2UNICASTCIPHER:
            case WLAN1_VAP1_WPA2UNICASTCIPHER:
            case WLAN1_VAP2_WPA2UNICASTCIPHER:
            case WLAN1_VAP3_WPA2UNICASTCIPHER:
            case WLAN1_VAP4_WPA2UNICASTCIPHER:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->wpa2UnicastCipher;
				break;
#ifdef CONFIG_RTL_WAPI_SUPPORT
			case WLAN0_WAPIPSK:
            case WLAN0_VAP1_WAPIPSK:
            case WLAN0_VAP2_WAPIPSK:
            case WLAN0_VAP3_WAPIPSK:
            case WLAN0_VAP4_WAPIPSK:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_WAPIPSK:
            case WLAN1_VAP1_WAPIPSK:
            case WLAN1_VAP2_WAPIPSK:
            case WLAN1_VAP3_WAPIPSK:
            case WLAN1_VAP4_WAPIPSK:
#endif
                opt_len = 4 + strlen(entry->wapiPsk);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                memcpy(opt->data, entry->wapiPsk, strlen(entry->wapiPsk));
				break;
			case WLAN0_WAPIPSKLEN:
            case WLAN0_VAP1_WAPIPSKLEN:
            case WLAN0_VAP2_WAPIPSKLEN:
            case WLAN0_VAP3_WAPIPSKLEN:
            case WLAN0_VAP4_WAPIPSKLEN:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_WAPIPSKLEN:
            case WLAN1_VAP1_WAPIPSKLEN:
            case WLAN1_VAP2_WAPIPSKLEN:
            case WLAN1_VAP3_WAPIPSKLEN:
            case WLAN1_VAP4_WAPIPSKLEN:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->wapiPskLen;
				break;
			case WLAN0_WAPIAUTH:
            case WLAN0_VAP1_WAPIAUTH:
            case WLAN0_VAP2_WAPIAUTH:
            case WLAN0_VAP3_WAPIAUTH:
            case WLAN0_VAP4_WAPIAUTH:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_WAPIAUTH:
            case WLAN1_VAP1_WAPIAUTH:
            case WLAN1_VAP2_WAPIAUTH:
            case WLAN1_VAP3_WAPIAUTH:
            case WLAN1_VAP4_WAPIAUTH:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->wapiAuth;
				break;
			case WLAN0_WAPIPSKFORMAT:
            case WLAN0_VAP1_WAPIPSKFORMAT:
            case WLAN0_VAP2_WAPIPSKFORMAT:
            case WLAN0_VAP3_WAPIPSKFORMAT:
            case WLAN0_VAP4_WAPIPSKFORMAT:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_WAPIPSKFORMAT:
            case WLAN1_VAP1_WAPIPSKFORMAT:
            case WLAN1_VAP2_WAPIPSKFORMAT:
            case WLAN1_VAP3_WAPIPSKFORMAT:
            case WLAN1_VAP4_WAPIPSKFORMAT:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->wapiPskFormat;
				break;
			case WLAN0_WAPIASIPADDR:
			case WLAN0_VAP1_WAPIASIPADDR:
			case WLAN0_VAP2_WAPIASIPADDR:
			case WLAN0_VAP3_WAPIASIPADDR:
			case WLAN0_VAP4_WAPIASIPADDR:
#ifdef WLAN_DUALBAND_CONCURRENT
			case WLAN1_WAPIASIPADDR:
			case WLAN1_VAP1_WAPIASIPADDR:
			case WLAN1_VAP2_WAPIASIPADDR:
			case WLAN1_VAP3_WAPIASIPADDR:
			case WLAN1_VAP4_WAPIASIPADDR:
#endif
                opt_len = 4 + sizeof(entry->wapiAsIpAddr);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                *(unsigned long *)(opt->data) = htonl(*((unsigned long *)entry->wapiAsIpAddr));
				break;
#endif
			case WLAN0_WEPKEYTYPE:
            case WLAN0_VAP1_WEPKEYTYPE:
            case WLAN0_VAP2_WEPKEYTYPE:
            case WLAN0_VAP3_WEPKEYTYPE:
            case WLAN0_VAP4_WEPKEYTYPE:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_WEPKEYTYPE:
            case WLAN1_VAP1_WEPKEYTYPE:
            case WLAN1_VAP2_WEPKEYTYPE:
            case WLAN1_VAP3_WEPKEYTYPE:
            case WLAN1_VAP4_WEPKEYTYPE:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->wepKeyType;
				break;
			case WLAN0_WEPDEFAULTKEY:
			case WLAN0_VAP1_WEPDEFAULTKEY:
			case WLAN0_VAP2_WEPDEFAULTKEY:
			case WLAN0_VAP3_WEPDEFAULTKEY:
			case WLAN0_VAP4_WEPDEFAULTKEY:
#ifdef WLAN_DUALBAND_CONCURRENT
			case WLAN1_WEPDEFAULTKEY:
			case WLAN1_VAP1_WEPDEFAULTKEY:
			case WLAN1_VAP2_WEPDEFAULTKEY:
			case WLAN1_VAP3_WEPDEFAULTKEY:
			case WLAN1_VAP4_WEPDEFAULTKEY:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->wepDefaultKey;
				break;
			case WLAN0_WEP64KEY1:
			case WLAN0_VAP1_WEP64KEY1:
			case WLAN0_VAP2_WEP64KEY1:
			case WLAN0_VAP3_WEP64KEY1:
			case WLAN0_VAP4_WEP64KEY1:
#ifdef WLAN_DUALBAND_CONCURRENT
			case WLAN1_WEP64KEY1:
			case WLAN1_VAP1_WEP64KEY1:
			case WLAN1_VAP2_WEP64KEY1:
			case WLAN1_VAP3_WEP64KEY1:
			case WLAN1_VAP4_WEP64KEY1:
#endif
                opt_len = 4 + strlen(entry->wep64Key1);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                memcpy(opt->data, entry->wep64Key1, strlen(entry->wep64Key1));
				break;
#if 0
			case WLAN0_WEP64KEY2:
			case WLAN0_VAP1_WEP64KEY2:
			case WLAN0_VAP2_WEP64KEY2:
			case WLAN0_VAP3_WEP64KEY2:
			case WLAN0_VAP4_WEP64KEY2:
#ifdef WLAN_DUALBAND_CONCURRENT
			case WLAN1_WEP64KEY2:
			case WLAN1_VAP1_WEP64KEY2:
			case WLAN1_VAP2_WEP64KEY2:
			case WLAN1_VAP3_WEP64KEY2:
			case WLAN1_VAP4_WEP64KEY2:
#endif
                opt_len = 4 + strlen(entry->wep64Key2);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                memcpy(opt->data, entry->wep64Key2, strlen(entry->wep64Key2));
				break;
			case WLAN0_WEP64KEY3:
			case WLAN0_VAP1_WEP64KEY3:
			case WLAN0_VAP2_WEP64KEY3:
			case WLAN0_VAP3_WEP64KEY3:
			case WLAN0_VAP4_WEP64KEY3:
#ifdef WLAN_DUALBAND_CONCURRENT
			case WLAN1_WEP64KEY3:
			case WLAN1_VAP1_WEP64KEY3:
			case WLAN1_VAP2_WEP64KEY3:
			case WLAN1_VAP3_WEP64KEY3:
			case WLAN1_VAP4_WEP64KEY3:
#endif
                opt_len = 4 + strlen(entry->wep64Key3);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                memcpy(opt->data, entry->wep64Key3, strlen(entry->wep64Key3));
				break;
			case WLAN0_WEP64KEY4:
			case WLAN0_VAP1_WEP64KEY4:
			case WLAN0_VAP2_WEP64KEY4:
			case WLAN0_VAP3_WEP64KEY4:
			case WLAN0_VAP4_WEP64KEY4:
#ifdef WLAN_DUALBAND_CONCURRENT
			case WLAN1_WEP64KEY4:
			case WLAN1_VAP1_WEP64KEY4:
			case WLAN1_VAP2_WEP64KEY4:
			case WLAN1_VAP3_WEP64KEY4:
			case WLAN1_VAP4_WEP64KEY4:
#endif
                opt_len = 4 + strlen(entry->wep64Key4);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                memcpy(opt->data, entry->wep64Key4, strlen(entry->wep64Key4));
				break;
#endif
			case WLAN0_WEP128KEY1:
			case WLAN0_VAP1_WEP128KEY1:
			case WLAN0_VAP2_WEP128KEY1:
			case WLAN0_VAP3_WEP128KEY1:
			case WLAN0_VAP4_WEP128KEY1:
#ifdef WLAN_DUALBAND_CONCURRENT
			case WLAN1_WEP128KEY1:
			case WLAN1_VAP1_WEP128KEY1:
			case WLAN1_VAP2_WEP128KEY1:
			case WLAN1_VAP3_WEP128KEY1:
			case WLAN1_VAP4_WEP128KEY1:
#endif
                opt_len = 4 + strlen(entry->wep128Key1);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                memcpy(opt->data, entry->wep128Key1, strlen(entry->wep128Key1));
				break;
#if 0
			case WLAN0_WEP128KEY2:
			case WLAN0_VAP1_WEP128KEY2:
			case WLAN0_VAP2_WEP128KEY2:
			case WLAN0_VAP3_WEP128KEY2:
			case WLAN0_VAP4_WEP128KEY2:
#ifdef WLAN_DUALBAND_CONCURRENT
			case WLAN1_WEP128KEY2:
			case WLAN1_VAP1_WEP128KEY2:
			case WLAN1_VAP2_WEP128KEY2:
			case WLAN1_VAP3_WEP128KEY2:
			case WLAN1_VAP4_WEP128KEY2:
#endif
                opt_len = 4 + strlen(entry->wep128Key2);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                memcpy(opt->data, entry->wep128Key2, strlen(entry->wep128Key2));
				break;
			case WLAN0_WEP128KEY3:
			case WLAN0_VAP1_WEP128KEY3:
			case WLAN0_VAP2_WEP128KEY3:
			case WLAN0_VAP3_WEP128KEY3:
			case WLAN0_VAP4_WEP128KEY3:
#ifdef WLAN_DUALBAND_CONCURRENT
			case WLAN1_WEP128KEY3:
			case WLAN1_VAP1_WEP128KEY3:
			case WLAN1_VAP2_WEP128KEY3:
			case WLAN1_VAP3_WEP128KEY3:
			case WLAN1_VAP4_WEP128KEY3:
#endif
                opt_len = 4 + strlen(entry->wep128Key3);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                memcpy(opt->data, entry->wep128Key3, strlen(entry->wep128Key3));
				break;
			case WLAN0_WEP128KEY4:
			case WLAN0_VAP1_WEP128KEY4:
			case WLAN0_VAP2_WEP128KEY4:
			case WLAN0_VAP3_WEP128KEY4:
			case WLAN0_VAP4_WEP128KEY4:
#ifdef WLAN_DUALBAND_CONCURRENT
			case WLAN1_WEP128KEY4:
			case WLAN1_VAP1_WEP128KEY4:
			case WLAN1_VAP2_WEP128KEY4:
			case WLAN1_VAP3_WEP128KEY4:
			case WLAN1_VAP4_WEP128KEY4:
#endif
                opt_len = 4 + strlen(entry->wep128Key4);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                memcpy(opt->data, entry->wep128Key4, strlen(entry->wep128Key4));
				break;
#endif
			//WiFi security end
            //11R start
#ifdef WLAN_11R
            case WLAN0_FT_MDID:
            case WLAN0_VAP1_FT_MDID:
            case WLAN0_VAP2_FT_MDID:
            case WLAN0_VAP3_FT_MDID:
            case WLAN0_VAP4_FT_MDID:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_FT_MDID:
            case WLAN1_VAP1_FT_MDID:
            case WLAN1_VAP2_FT_MDID:
            case WLAN1_VAP3_FT_MDID:
            case WLAN1_VAP4_FT_MDID:
#endif
                opt_len = 4 + strlen(entry->ft_mdid);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                memcpy(opt->data, entry->ft_mdid, strlen(entry->ft_mdid));
                break;
            case WLAN0_FT_ENABLE:
            case WLAN0_VAP1_FT_ENABLE:
            case WLAN0_VAP2_FT_ENABLE:
            case WLAN0_VAP3_FT_ENABLE:
            case WLAN0_VAP4_FT_ENABLE:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_FT_ENABLE:
            case WLAN1_VAP1_FT_ENABLE:
            case WLAN1_VAP2_FT_ENABLE:
            case WLAN1_VAP3_FT_ENABLE:
            case WLAN1_VAP4_FT_ENABLE:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->ft_enable;
                break;
#if 0
            case WLAN0_FT_OVER_DS:
            case WLAN0_VAP1_FT_OVER_DS:
            case WLAN0_VAP2_FT_OVER_DS:
            case WLAN0_VAP3_FT_OVER_DS:
            case WLAN0_VAP4_FT_OVER_DS:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_FT_OVER_DS:
            case WLAN1_VAP1_FT_OVER_DS:
            case WLAN1_VAP2_FT_OVER_DS:
            case WLAN1_VAP3_FT_OVER_DS:
            case WLAN1_VAP4_FT_OVER_DS:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->ft_over_ds;
                break;
            case WLAN0_FT_RES_REQ:           //ft_res_request
            case WLAN0_VAP1_FT_RES_REQ:
            case WLAN0_VAP2_FT_RES_REQ:
            case WLAN0_VAP3_FT_RES_REQ:
            case WLAN0_VAP4_FT_RES_REQ:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_FT_RES_REQ:
            case WLAN1_VAP1_FT_RES_REQ:
            case WLAN1_VAP2_FT_RES_REQ:
            case WLAN1_VAP3_FT_RES_REQ:
            case WLAN1_VAP4_FT_RES_REQ:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->ft_res_request;
                break;
            case WLAN0_FT_R0KEY_TO:          //ft_r0key_timeout
            case WLAN0_VAP1_FT_R0KEY_TO:
            case WLAN0_VAP2_FT_R0KEY_TO:
            case WLAN0_VAP3_FT_R0KEY_TO:
            case WLAN0_VAP4_FT_R0KEY_TO:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_FT_R0KEY_TO:
            case WLAN1_VAP1_FT_R0KEY_TO:
            case WLAN1_VAP2_FT_R0KEY_TO:
            case WLAN1_VAP3_FT_R0KEY_TO:
            case WLAN1_VAP4_FT_R0KEY_TO:
#endif
                opt_len = 4 + sizeof(entry->ft_r0key_timeout);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                *(int *)(opt->data) = htonl(*((int *)entry->ft_r0key_timeout));
                break;
            case WLAN0_FT_REASOC_TO:         //ft_reasoc_timeout
            case WLAN0_VAP1_FT_REASOC_TO:
            case WLAN0_VAP2_FT_REASOC_TO:
            case WLAN0_VAP3_FT_REASOC_TO:
            case WLAN0_VAP4_FT_REASOC_TO:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_FT_REASOC_TO:
            case WLAN1_VAP1_FT_REASOC_TO:
            case WLAN1_VAP2_FT_REASOC_TO:
            case WLAN1_VAP3_FT_REASOC_TO:
            case WLAN1_VAP4_FT_REASOC_TO:
#endif
                opt_len = 4 + sizeof(entry->ft_reasoc_timeout);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                *(int *)(opt->data) = htonl(*((int *)entry->ft_reasoc_timeout));
                break;
            case WLAN0_FT_R0KH_ID:           //ft_r0kh_id
            case WLAN0_VAP1_FT_R0KH_ID:
            case WLAN0_VAP2_FT_R0KH_ID:
            case WLAN0_VAP3_FT_R0KH_ID:
            case WLAN0_VAP4_FT_R0KH_ID:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_FT_R0KH_ID:
            case WLAN1_VAP1_FT_R0KH_ID:
            case WLAN1_VAP2_FT_R0KH_ID:
            case WLAN1_VAP3_FT_R0KH_ID:
            case WLAN1_VAP4_FT_R0KH_ID:
#endif
                opt_len = 4 + strlen(entry->ft_r0kh_id);
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                memcpy(opt->data, entry->ft_mdid, strlen(entry->ft_r0kh_id));
                break;
            case WLAN0_FT_PUSH:              //ft_push
            case WLAN0_VAP1_FT_PUSH:
            case WLAN0_VAP2_FT_PUSH:
            case WLAN0_VAP3_FT_PUSH:
            case WLAN0_VAP4_FT_PUSH:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_FT_PUSH:
            case WLAN1_VAP1_FT_PUSH:
            case WLAN1_VAP2_FT_PUSH:
            case WLAN1_VAP3_FT_PUSH:
            case WLAN1_VAP4_FT_PUSH:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->ft_push;
                break;
            case WLAN0_FT_KH_NUM:            //ft_kh_num
            case WLAN0_VAP1_FT_KH_NUM:
            case WLAN0_VAP2_FT_KH_NUM:
            case WLAN0_VAP3_FT_KH_NUM:
            case WLAN0_VAP4_FT_KH_NUM:
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_FT_KH_NUM:
            case WLAN1_VAP1_FT_KH_NUM:
            case WLAN1_VAP2_FT_KH_NUM:
            case WLAN1_VAP3_FT_KH_NUM:
            case WLAN1_VAP4_FT_KH_NUM:
#endif
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = entry->ft_kh_num;
                break;
#endif
#endif
            //11R end
        }
    }
    else
    {
        //other conf not in MBSSIB_TBL
        wlan_idx = 0;
        switch (type_idx)
        {
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_AUTO_CHAN_ENABLED:
                wlan_idx = 1;
#endif
            case WLAN0_AUTO_CHAN_ENABLED:
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                mib_get(MIB_WLAN_AUTO_CHAN_ENABLED, &vChar);
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = vChar;
                break;
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_CHAN_NUM:
                wlan_idx = 1;
#endif
            case WLAN0_CHAN_NUM:
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                mib_get(MIB_WLAN_CHAN_NUM, &vChar);
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = vChar;
                break;
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_CHANNEL_WIDTH:
                wlan_idx = 1;
#endif
            case WLAN0_CHANNEL_WIDTH:
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                mib_get(MIB_WLAN_CHANNEL_WIDTH, &vChar);
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = vChar;
                break;
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_11N_COEXIST:
                wlan_idx = 1;
#endif
            case WLAN0_11N_COEXIST:
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                mib_get(MIB_WLAN_11N_COEXIST, &vChar);
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = vChar;
                break;
#if 0
            case MAC_FILTER_SRC_ENABLE:
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                mib_get(MIB_MAC_FILTER_SRC_ENABLE, &vChar);
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = vChar;
                break;
            case IPFILTER_ON_OFF:
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                mib_get(MIB_IPFILTER_ON_OFF, &vChar);
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = vChar;
                break;
#endif
#ifdef URL_BLOCKING_SUPPORT
            case URL_CAPABILITY:
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                mib_get(MIB_URL_CAPABILITY, &vChar);
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = vChar;
                break;
#endif
            case DOS_ENABLED:
                opt_len = 8;    //unsigned int
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                mib_get(MIB_DOS_ENABLED, &vInt);
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                *(unsigned int *)(opt->data) = htons(vInt);
                break;
#if 0
            case FW_GRADE:
                opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                mib_get(MIB_FW_GRADE, &vChar);
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = vChar;
                break;
#endif
			case DHCP_MODE:
				opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                vChar = DHCPV4_LAN_NONE; /* Fixed Value: diable dhcpv6 server */
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = vChar;
				break;
		#ifdef CONFIG_IPV6
		#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
			case DHCPV6_MODE:
				opt_len = 5;    //char
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                vChar = DHCP_LAN_NONE; /* Fixed Value: diable dhcpv6 server */
                opt->type = htons(type_idx);
                opt->len = htons(opt_len);
                *optlen = opt_len;
                opt->data[0] = vChar;
				break;
		#endif
		#endif
            //chain
#ifdef WLAN_11R
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_FTKH_TBL:
                wlan_idx = 1;
#endif
            case WLAN0_FTKH_TBL:
                {
                    int i = 0;
                    MIB_CE_WLAN_FTKH_T khEntry;
                    total = mib_chain_total(MIB_WLAN_FTKH_TBL);
                    debug_print("[%s %d]type=%d, curr chain_idx=%d, total=%d\n", __func__, __LINE__, type_idx, chain_idx, total);
                    if(chain_idx < total)
                    {
                        opt_len = 4 + sizeof(khEntry);
                        if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                        {
                            //exceed
                            return -1;
                        }
                        
                        if (mib_chain_get(MIB_WLAN_FTKH_TBL, chain_idx, (void *)&khEntry))
        				{
        					opt->type = htons(type_idx);
                            opt->len = htons(opt_len);
                            *optlen = opt_len;
                            memcpy(opt->data, &khEntry, sizeof(MIB_CE_WLAN_FTKH_T));
        				}
                        
                        //each entry has a CLU_HDR.
                        chain_idx++;
                        wlan_idx = orig_wlanid;
                        return 0;
                    }
                    else
                    {
                        chain_idx = 0;
                        wlan_idx = orig_wlanid;
                        return -2;  //mean all chain entry has added.
                    }
                }
                break;
#ifdef SUPPORT_TIME_SCHEDULE
            case TIME_SCHEDULE_TBL:
#if 0
                //FIXME
                {
                    int i = 0;
                    MIB_CE_TIME_SCHEDULE_T schdEntry;
                    total = mib_chain_total(MIB_TIME_SCHEDULE_TBL);
                    debug_print("[%s %d], type=%d, curr chain_idx=%d, total=%d\n", __func__, __LINE__, type_idx, chain_idx, total);
                    if(chain_idx < total)
                    {
                        opt_len = 4 + sizeof(MIB_CE_TIME_SCHEDULE_T);
                        if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                        {
                            //exceed
                            return -1;
                        }
                        if (mib_chain_get(MIB_TIME_SCHEDULE_TBL, chain_idx, (void *)&schdEntry))
        				{
        					opt->type = htons(type_idx);
                            opt->len = htons(opt_len);
                            *optlen = opt_len;
                            memcpy(opt->data, &schdEntry, _OFFSET_CE(MIB_CE_TIME_SCHEDULE_T, sch_time));
                            for(i=0; i<7; i++)
                            {
                                *(unsigned int *)(opt->data+_OFFSET_CE(MIB_CE_TIME_SCHEDULE_T, sch_time)+4*i) = htonl(((unsigned int *)(schdEntry.sch_time))[i]);
                            }
        				}
                        //each entry has a CLU_HDR.
                        chain_idx++;
                        return 0;
                    }
                    else
                    {
                        chain_idx = 0;
                        return -2;  //mean all chain entry has added.
                    }
                }
#endif
                break;
#endif
#endif
#if 0
            case MAC_FILTER_ROUTER_TBL:
                {
                    MIB_CE_ROUTEMAC_T Entry;
                    total = mib_chain_total(MIB_MAC_FILTER_ROUTER_TBL);
                    debug_print("[%s %d], type=%d, curr chain_idx=%d, total=%d\n", __func__, __LINE__, type_idx, chain_idx, total);
                    if(chain_idx < total)
                    {
                        opt_len = 4 + sizeof(MIB_CE_ROUTEMAC_T);
                        if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                        {
                            //exceed
                            return -1;
                        }
                        if (mib_chain_get(MIB_MAC_FILTER_ROUTER_TBL, chain_idx, (void *)&Entry))
        				{
        					opt->type = htons(type_idx);
                            opt->len = htons(opt_len);
                            *optlen = opt_len;
#ifdef SUPPORT_TIME_SCHEDULE
#if 0
                            //FIXME
                            memcpy(opt->data, &Entry, _OFFSET_CE(MIB_CE_ROUTEMAC_T, schedIndex));
                            *(unsigned int *)(opt->data+_OFFSET_CE(MIB_CE_ROUTEMAC_T, schedIndex)) = htonl(Entry.schedIndex);
#endif
#else
#if 0
                            //FIXME
                            memcpy(opt->data, &Entry, sizeof(MIB_CE_TIME_SCHEDULE_T));                 
#endif
#endif
        				}
                        //each entry has a CLU_HDR.
                        chain_idx++;
                        return 0;
                    }
                    else
                    {
                        chain_idx = 0;
                        return -2;  //mean all chain entry has added.
                    }
                }
                break;
#endif
            case IP_PORT_FILTER_TBL:
                {
                    MIB_CE_IP_PORT_FILTER_T Entry;
                    MIB_CE_IP_PORT_FILTER_Tp pEntry;
                    total = mib_chain_total(MIB_IP_PORT_FILTER_TBL);
                    debug_print("[%s %d], type=%d, curr chain_idx=%d, total=%d\n", __func__, __LINE__, type_idx, chain_idx, total);
                    if(chain_idx < total)
                    {
                        opt_len = 4 + sizeof(MIB_CE_IP_PORT_FILTER_T);
                        if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                        {
                            //exceed
                            return -1;
                        }
                        if (mib_chain_get(MIB_IP_PORT_FILTER_TBL, chain_idx, (void *)&Entry))
        				{
        					opt->type = htons(type_idx);
                            opt->len = htons(opt_len);
                            *optlen = opt_len;

                            pEntry = (MIB_CE_IP_PORT_FILTER_Tp)opt->data;
                            pEntry->action = Entry.action;
                            *(unsigned int *)pEntry->srcIp = htonl(*(unsigned int *)Entry.srcIp);
                            *(unsigned int *)pEntry->dstIp = htonl(*(unsigned int *)Entry.dstIp);
                            pEntry->smaskbit = Entry.smaskbit;
                            pEntry->dmaskbit = Entry.dmaskbit;
                            pEntry->srcPortFrom = htons(Entry.srcPortFrom);
                            pEntry->dstPortFrom = htons(Entry.dstPortFrom);
                            pEntry->srcPortTo = htons(Entry.srcPortTo);
                            pEntry->dstPortTo = htons(Entry.dstPortTo);
                            pEntry->dir = Entry.dir;
                            pEntry->protoType = Entry.protoType;
#if 0
                            memcpy(pEntry->name, Entry.name, sizeof(Entry.name));
                            *(unsigned int *)pEntry->srcIp2 = htonl(*(unsigned int *)Entry.srcIp2);
                            *(unsigned int *)pEntry->dstIp2 = htonl(*(unsigned int *)Entry.dstIp2);
#ifdef CONFIG_IPV6
                            pEntry->IpProtocol = Entry.IpProtocol;
                            memcpy(pEntry->sip6Start, Entry.sip6Start, sizeof(Entry.sip6Start));
                            memcpy(pEntry->sip6End, Entry.sip6End, sizeof(Entry.sip6End));
                            memcpy(pEntry->dip6Start, Entry.dip6Start, sizeof(Entry.dip6Start));
                            memcpy(pEntry->dip6End, Entry.dip6End, sizeof(Entry.dip6End));
                            pEntry->sip6PrefixLen = Entry.sip6PrefixLen;
                            pEntry->dip6PrefixLen = Entry.dip6PrefixLen;
#endif
                            
#ifdef SUPPORT_TIME_SCHEDULE
                            pEntry->schedIndex = htonl(Entry.schedIndex);
                            pEntry->aclIndex = htonl(Entry.aclIndex);
#endif
#endif
        				}
                        //each entry has a CLU_HDR.
                        chain_idx++;
                        return 0;
                    }
                    else
                    {
                        chain_idx = 0;
                        return -2;  //mean all chain entry has added.
                    }
                }
                break;
#ifdef URL_BLOCKING_SUPPORT
            case URL_FQDN_TBL:
                {
                    MIB_CE_URL_FQDN_T Entry;
                    MIB_CE_URL_FQDN_Tp pEntry;
                    total = mib_chain_total(MIB_URL_FQDN_TBL);
                    debug_print("[%s %d], type=%d, curr chain_idx=%d, total=%d\n", __func__, __LINE__, type_idx, chain_idx, total);
                    if(chain_idx < total)
                    {
                        opt_len = 4 + sizeof(MIB_CE_URL_FQDN_T);
                        if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                        {
                            //exceed
                            return -1;
                        }
                        if (mib_chain_get(MIB_URL_FQDN_TBL, chain_idx, (void *)&Entry))
        				{
        					opt->type = htons(type_idx);
                            opt->len = htons(opt_len);
                            *optlen = opt_len;

                            pEntry = (MIB_CE_URL_FQDN_Tp)opt->data;
                            memcpy(pEntry->fqdn, Entry.fqdn, sizeof(Entry.fqdn));
                            //memcpy(pEntry->url, Entry.url, sizeof(Entry.url));
                            //memcpy(pEntry->key, Entry.key, sizeof(Entry.key));
                            //pEntry->port = htons(Entry.port);
#ifdef SUPPORT_TIME_SCHEDULE
                            pEntry->schedIndex = htonl(Entry.schedIndex);
#endif
        				}
                        //each entry has a CLU_HDR.
                        chain_idx++;
                        return 0;
                    }
                    else
                    {
                        chain_idx = 0;
                        return -2;  //mean all chain entry has added.
                    }
                }
                break;
#endif
#ifdef REMOTE_ACCESS_CTL
            case ACC_TBL:
                {
                    MIB_CE_ACC_T Entry;
                    MIB_CE_ACC_Tp pEntry;
                    total = mib_chain_total(MIB_ACC_TBL);
                    debug_print("[%s %d], type=%d, curr chain_idx=%d, total=%d\n", __func__, __LINE__, type_idx, chain_idx, total);
                    if(chain_idx < total)
                    {
                        opt_len = 4 + sizeof(MIB_CE_ACC_T);
                        if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                        {
                            //exceed
                            return -1;
                        }
                        if (mib_chain_get(MIB_ACC_TBL, chain_idx, (void *)&Entry))
                        {
                            opt->type = htons(type_idx);
                            opt->len = htons(opt_len);
                            *optlen = opt_len;

                            pEntry = (MIB_CE_ACC_Tp)opt->data;
                            pEntry->telnet= Entry.telnet;
                            pEntry->ftp= Entry.ftp;
                            pEntry->tftp= Entry.tftp;
                            pEntry->web= Entry.web;
                            pEntry->snmp= Entry.snmp;
                            pEntry->ssh= Entry.ssh;
                            pEntry->icmp= Entry.icmp;
                            pEntry->netlog= Entry.netlog;
                            pEntry->smb= Entry.smb;
                            pEntry->https= Entry.https;
                            pEntry->telnet_port = htons(Entry.telnet_port);
                            pEntry->web_port = htons(Entry.web_port);
                            pEntry->ftp_port = htons(Entry.ftp_port);
                            pEntry->https_port = htons(Entry.https_port);
                        }
                        //each entry has a CLU_HDR.
                        chain_idx++;
                        return 0;
                    }
                    else
                    {
                        chain_idx = 0;
                        return -2;  //mean all chain entry has added.
                    }
                }
                break;
#endif
			case ATM_VC_TBL:
               	{
					//FIXME
                    MIB_CE_ATM_VC_T Entry;
                    MIB_CE_ATM_VC_WANTp pEntry;
                    total = 1; /* Fixed Value */
                    //printf("[%s %d], type=%d, curr chain_idx=%d, total=%d size=%d\n", __func__, __LINE__, type_idx, chain_idx, total,sizeof(MIB_CE_ATM_VC_WAN_T));
                    if(chain_idx < total)
                    {
                        opt_len = 4 + sizeof(MIB_CE_ATM_VC_WAN_T);
                        if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                        {
                            //exceed
                            debug_print("[%s %d] type[%d] require len[%d] is larger than remain len[%d]\n", __func__, __LINE__, type_idx, opt_len, (CLUSTER_PKT_MAX_LEN - len));
                            return -1;
                        }
                        if (1)
                        {
							/* Fixed MIB_CE_ATM_VC_T entry value: bridge wan, no vlan,  */
							unsigned int ifMap = 0;
							memset(&Entry, 0, sizeof(Entry));

							Entry.enable = 1;
							Entry.cmode = CHANNEL_MODE_BRIDGE;							
							Entry.ifIndex = if_find_index(Entry.cmode, ifMap);
							Entry.ifIndex = TO_IFINDEX(MEDIA_ETH, PPP_INDEX(Entry.ifIndex), ETH_INDEX(Entry.ifIndex));

							Entry.brmode = BRIDGE_ETHERNET;
							Entry.applicationtype = X_CT_SRV_INTERNET;
							Entry.encap = 1;
							Entry.mtu = 1500;
							Entry.itfGroup = 543; /* port binding all lan ports and wifi ports */
							Entry.IpProtocol = IPVER_IPV4;
							//Entry.disableLanDhcp = 1;
						#ifdef _CWMP_MIB_ /*jiunming, for cwmp-tr069*/
							Entry.ConDevInstNum = 1 + findMaxConDevInstNum(MEDIA_INDEX(Entry.ifIndex));
							Entry.ConIPInstNum = 1;
						#endif						
						#if defined(CONFIG_LUNA) && defined(GEN_WAN_MAC)
							{	
								char macaddr[MAC_ADDR_LEN];
								mib_get(MIB_ELAN_MAC_ADDR, (void *)macaddr);
								macaddr[MAC_ADDR_LEN-1] += WAN_HW_ETHER_START_BASE + ETH_INDEX(Entry.ifIndex);
								memcpy(Entry.MacAddr, macaddr, MAC_ADDR_LEN);
							}
						#endif						
							/* Fixed MIB_CE_ATM_VC_T entry value end */
							
                            opt->type = htons(type_idx);
                            opt->len = htons(opt_len);
                            *optlen = opt_len;

                            pEntry = (MIB_CE_ATM_VC_WANTp)opt->data;
							
                            pEntry->ifIndex= htonl(Entry.ifIndex);
                            pEntry->vpi= Entry.vpi;
                            pEntry->qos= Entry.qos;
                            pEntry->vci= htons(Entry.vci);
                            pEntry->pcr= htons(Entry.pcr);
                            pEntry->scr= htons(Entry.scr);
                            pEntry->mbs= htons(Entry.mbs);
                            pEntry->cdvt= htonl(Entry.cdvt);
                            pEntry->encap= Entry.encap;
                            pEntry->napt= Entry.napt;
							pEntry->cmode= Entry.cmode;
							pEntry->brmode= Entry.brmode;
							memcpy(pEntry->pppUsername, Entry.pppUsername, sizeof(Entry.pppUsername));
							memcpy(pEntry->pppPassword, Entry.pppPassword, sizeof(Entry.pppPassword));
							pEntry->pppAuth= Entry.pppAuth;
							memcpy(pEntry->pppACName, Entry.pppACName, sizeof(Entry.pppACName));
							memcpy(pEntry->pppServiceName, Entry.pppServiceName, sizeof(Entry.pppServiceName));
							pEntry->pppCtype= Entry.pppCtype;
							pEntry->pppIdleTime= htons(Entry.pppIdleTime);
						#ifdef CONFIG_USER_IP_QOS
							pEntry->enableIpQos= Entry.enableIpQos;
						#endif
						#ifdef CONFIG_IGMPPROXY_MULTIWAN
							pEntry->enableIGMP= Entry.enableIGMP;
						#endif
							pEntry->ipDhcp= Entry.ipDhcp;
							pEntry->rip= Entry.rip;
							*(int *)pEntry->ipAddr= htonl(*(int *)Entry.ipAddr);
							*(int *)pEntry->remoteIpAddr= htonl(*(int *)Entry.remoteIpAddr);
							pEntry->dgw= Entry.dgw;
							pEntry->mtu= htonl(Entry.mtu);
							pEntry->enable= Entry.enable;
							*(int *)pEntry->netMask= htonl(*(int *)Entry.netMask);
							pEntry->ipunnumbered= Entry.ipunnumbered;
							pEntry->dnsMode= Entry.dnsMode;
							*(int *)pEntry->v4dns1= htonl(*(int *)Entry.v4dns1);
							*(int *)pEntry->v4dns2= htonl(*(int *)Entry.v4dns2);
						#if defined(ITF_GROUP) || defined(NEW_PORTMAPPING)
							pEntry->vlan= Entry.vlan;
							pEntry->vid= htons(Entry.vid);
							pEntry->vprio= htons(Entry.vprio);
							pEntry->vpass= Entry.vpass;
							pEntry->itfGroup= htons(Entry.itfGroup);
						#endif
						#ifdef CONFIG_MCAST_VLAN
							pEntry->mVid= htons(Entry.mVid);
						#endif
							pEntry->cpePppIfIndex= htonl(Entry.cpePppIfIndex);
							pEntry->cpeIpIndex= htonl(Entry.cpeIpIndex);
						#ifdef _CWMP_MIB_
							pEntry->connDisable= Entry.connDisable;
							pEntry->ConDevInstNum= htonl(Entry.ConDevInstNum);
							pEntry->ConIPInstNum= htonl(Entry.ConIPInstNum);
							pEntry->ConPPPInstNum= htonl(Entry.ConPPPInstNum);
							pEntry->autoDisTime= htons(Entry.autoDisTime);
							pEntry->warnDisDelay= htons(Entry.warnDisDelay);
						#ifdef _PRMT_TR143_
							pEntry->TR143UDPEchoItf= Entry.TR143UDPEchoItf;
						#endif
						#ifdef _PRMT_X_CT_COM_WANEXT_
							memcpy(pEntry->IPForwardList, Entry.IPForwardList, sizeof(Entry.IPForwardList));
						#endif
						#endif
							memcpy(pEntry->WanName, Entry.WanName, sizeof(Entry.WanName));
						#ifdef CONFIG_USER_PPPOE_PROXY
							pEntry->PPPoEProxyEnable= Entry.PPPoEProxyEnable;
							pEntry->PPPoEProxyMaxUser= htonl(Entry.PPPoEProxyMaxUser);
						#endif
							pEntry->applicationtype= htonl(Entry.applicationtype);
							//pEntry->disableLanDhcp= Entry.disableLanDhcp;
							//pEntry->svtype= Entry.svtype;
						#ifdef CONFIG_SPPPD_STATICIP
							pEntry->pppIp= Entry.pppIp;
						#endif
						#ifdef CONFIG_USER_WT_146
							pEntry->bfd_enable= Entry.bfd_enable;
							pEntry->bfd_opmode= Entry.bfd_opmode;
							pEntry->bfd_role= Entry.bfd_role;
							pEntry->bfd_mintxint= htonl(Entry.bfd_mintxint);
							pEntry->bfd_minrxint= htonl(Entry.bfd_minrxint);
							pEntry->bfd_minechorxint= htonl(Entry.bfd_minechorxint);
							pEntry->bfd_detectmult= Entry.bfd_detectmult;
							pEntry->bfd_authtype= Entry.bfd_authtype;
							pEntry->bfd_authkeyid= Entry.bfd_authkeyid;
							pEntry->bfd_authkeylen= Entry.bfd_authkeylen;
							memcpy(pEntry->bfd_authkey, Entry.bfd_authkey, sizeof(Entry.bfd_authkey));
							pEntry->bfd_dscp= Entry.bfd_dscp;
							pEntry->bfd_ethprio= Entry.bfd_ethprio;
						#endif
						#ifdef CONFIG_IPV6
							pEntry->IpProtocol= Entry.IpProtocol;
							pEntry->AddrMode= Entry.AddrMode;
							memcpy(pEntry->Ipv6Addr, Entry.Ipv6Addr, sizeof(Entry.Ipv6Addr));
							memcpy(pEntry->RemoteIpv6Addr, Entry.RemoteIpv6Addr, sizeof(Entry.RemoteIpv6Addr));
							//pEntry->IPv6PrefixOrigin= Entry.IPv6PrefixOrigin;
							//memcpy(pEntry->Ipv6Prefix, Entry.Ipv6Prefix, sizeof(Entry.Ipv6Prefix));
							pEntry->Ipv6AddrPrefixLen= Entry.Ipv6AddrPrefixLen;
							//pEntry->IPv6PrefixPltime= htonl(Entry.IPv6PrefixPltime);
							//pEntry->IPv6PrefixVltime= htonl(Entry.IPv6PrefixVltime);
							//memcpy(pEntry->IPv6DomainName, Entry.IPv6DomainName, sizeof(Entry.IPv6DomainName));
							pEntry->Ipv6Dhcp= Entry.Ipv6Dhcp;
							pEntry->Ipv6DhcpRequest= Entry.Ipv6DhcpRequest;
							memcpy(pEntry->RemoteIpv6EndPointAddr, Entry.RemoteIpv6EndPointAddr, sizeof(Entry.RemoteIpv6EndPointAddr));
							//memcpy(pEntry->Ipv6AddrAlias, Entry.Ipv6AddrAlias, sizeof(Entry.Ipv6AddrAlias));
							//memcpy(pEntry->Ipv6PrefixAlias, Entry.Ipv6PrefixAlias, sizeof(Entry.Ipv6PrefixAlias));
							memcpy(pEntry->Ipv6Dns1, Entry.Ipv6Dns1, sizeof(Entry.Ipv6Dns1));
							memcpy(pEntry->Ipv6Dns2, Entry.Ipv6Dns2, sizeof(Entry.Ipv6Dns2));
						#endif
						#ifdef CONFIG_IPV6_SIT_6RD
							*(int *)pEntry->SixrdBRv4IP= htonl(*(int *)Entry.SixrdBRv4IP);
							pEntry->SixrdIPv4MaskLen= Entry.SixrdIPv4MaskLen;
							memcpy(pEntry->SixrdPrefix, Entry.SixrdPrefix, sizeof(Entry.SixrdPrefix));
							pEntry->SixrdPrefixLen= Entry.SixrdPrefixLen;
						#endif
							memcpy(pEntry->MacAddr, Entry.MacAddr, sizeof(Entry.MacAddr));
						#ifdef CONFIG_RTK_RG_INIT
							pEntry->rg_wan_idx= htonl(Entry.rg_wan_idx);
							pEntry->check_br_pm= htonl(Entry.check_br_pm);
						#endif
						#if defined(CONFIG_GPON_FEATURE)
							pEntry->sid= Entry.sid;
						#endif
                        #if defined(CONFIG_IPV6) && defined(DUAL_STACK_LITE)
							pEntry->dslite_enable= Entry.dslite_enable;
							pEntry->dslite_aftr_mode= Entry.dslite_aftr_mode;
							memcpy(pEntry->dslite_aftr_addr, Entry.dslite_aftr_addr, sizeof(Entry.dslite_aftr_addr));
							memcpy(pEntry->dslite_aftr_hostname, Entry.dslite_aftr_hostname, sizeof(Entry.dslite_aftr_hostname));
						#endif
						#if defined(CONFIG_IPV6)
							pEntry->dnsv6Mode= Entry.dnsv6Mode;
						#endif
                        }
                        //each entry has a CLU_HDR.
                        chain_idx++;
                        return 0;
                    }
                    else
                    {
                        chain_idx = 0;
                        return -2;  //mean all chain entry has added.
                    }
                }		
				break;
            case TYPE_IDX_END:
                opt_len = 4;
                if((len + opt_len)>CLUSTER_PKT_MAX_LEN)
                {
                    //exceed
                    return -1;
                }
                opt = (OPT_TLV_Tp)(ether_frame + len);
                opt->type = htons(TYPE_IDX_END);
                opt->len = htons(opt_len);
                *optlen = opt_len;
            default:
                break;
        }
	}
    
    wlan_idx = orig_wlanid;
    return 0;
}

static void configrations_send(void)
{
    unsigned short index = 0;
    unsigned short len = 0;
    unsigned short checksum = 0;
    unsigned short optionlen = 0;
    unsigned short type_idx = WLAN0_CONF_START+1;      //WLAN0_CONF_START is the first type index.
    unsigned char ether_frame[CLUSTER_PKT_MAX_LEN];
    struct ethhdr *machdr;
    CLU_HDR_Tp cluhdr;
    MIB_CE_MBSSIB_T Entry[12];
    int orig_wlanid;
    int ret = 0;
    int i = 0;

    machdr = (struct ethhdr *)ether_frame;
    memcpy(machdr->h_dest, broadcast_mac, ETH_ALEN);
    memcpy(machdr->h_source, clu_ll_addr.sll_addr, ETH_ALEN);
    machdr->h_proto = htons(CLUSTER_MANAGE_PROTOCOL);

    cluhdr = (CLU_HDR_Tp)(ether_frame + ETH_HLEN);
    cluhdr->sequence = htons(cur_seq);

    //GetEntry first...
    orig_wlanid = wlan_idx;
    wlan_idx = 0;
    for(i=0; i<6; i++)
    {
        wlan_getEntry(&(Entry[i]), i);
    }
#ifdef WLAN_DUALBAND_CONCURRENT
    wlan_idx = 1;
    for(i=6; i<12; i++)
    {
        wlan_getEntry(&Entry[i], i-6);
    }
#endif
    wlan_idx = orig_wlanid;
    
Next_packets:
    cluhdr->id = htons(index);
    cluhdr->checksum = 0;
    cluhdr->totlen = 0;
    len = ETH_HLEN + sizeof(CLU_HDR_T);
    memset(ether_frame+len, 0, CLUSTER_PKT_MAX_LEN-len);
    
    //Now add options to packets.
    for(; type_idx<=TYPE_IDX_END; type_idx++)
    {
        optionlen = 0;

        //first, wlan0 amd mbssid conf
        if((type_idx>WLAN0_CONF_START) && (type_idx<WLAN0_CONF_END))
        {
            //wlan0
            ret = addOption(type_idx, ether_frame, len, &optionlen, &(Entry[0]));
        }
        else if((type_idx>WLAN0_VAP1_CONF_START) && (type_idx<WLAN0_VAP1_CONF_END))
        {
            //wlan0-vap1
            ret = addOption(type_idx, ether_frame, len, &optionlen, &(Entry[1]));
        }
        else if((type_idx>WLAN0_VAP2_CONF_START) && (type_idx<WLAN0_VAP2_CONF_END))
        {
            //wlan0-vap2
            ret = addOption(type_idx, ether_frame, len, &optionlen, &(Entry[2]));
        }
        else if((type_idx>WLAN0_VAP3_CONF_START) && (type_idx<WLAN0_VAP3_CONF_END))
        {
            //wlan0-vap3
            ret = addOption(type_idx, ether_frame, len, &optionlen, &(Entry[3]));
        }
        else if((type_idx>WLAN0_VAP4_CONF_START) && (type_idx<WLAN0_VAP4_CONF_END))
        {
            //wlan0-vap4
            ret = addOption(type_idx, ether_frame, len, &optionlen, &(Entry[4]));
        }
#ifdef WLAN_DUALBAND_CONCURRENT
        else if((type_idx>WLAN1_CONF_START) && (type_idx<WLAN1_CONF_END))
        {
            //wlan1
            ret = addOption(type_idx, ether_frame, len, &optionlen, &Entry[6]);
        }
        else if((type_idx>WLAN1_VAP1_CONF_START) && (type_idx<WLAN1_VAP1_CONF_END))
        {
            //wlan1-vap1
            ret = addOption(type_idx, ether_frame, len, &optionlen, &Entry[7]);
        }

        else if((type_idx>WLAN1_VAP2_CONF_START) && (type_idx<WLAN1_VAP2_CONF_END))
        {
            //wlan0-vap2
            ret = addOption(type_idx, ether_frame, len, &optionlen, &Entry[8]);
        }
        else if((type_idx>WLAN1_VAP3_CONF_START) && (type_idx<WLAN1_VAP3_CONF_END))
        {
            //wlan0-vap3
            ret = addOption(type_idx, ether_frame, len, &optionlen, &Entry[9]);
        }

        else if((type_idx>WLAN1_VAP4_CONF_START) && (type_idx<WLAN1_VAP4_CONF_END))
        {
            //wlan0-vap4
            ret = addOption(type_idx, ether_frame, len, &optionlen, &Entry[10]);
        }
#endif
        else
        {
            //other conf not in MBSSIB_TBL
            ret = addOption(type_idx, ether_frame, len, &optionlen, NULL);
        }

        if(0 == ret)
        {
            //add success, update current length and go on.
            len += optionlen;
            if(optionlen != 0)
            {
                //debug_print("Add option %d, Now len=%d\n", type_idx, len);
                if((type_idx>MIB_CHAIN_START) && (type_idx<MIB_CHAIN_END))
                {
                    //chain have entrys to add to the packet.
                    type_idx--;
                }
            }
        }
        else if(-1 == ret)
        {
            //exceed the packet max length, send current options first.
            debug_print("exceed the packet max length, send current options first\n");
            break;
        }
    }

    cluhdr->totlen = htons(len-ETH_HLEN);
    checksum = get_checksum(ether_frame+ETH_HLEN, len-ETH_HLEN);
    cluhdr->checksum = htons(checksum);

    //debug_dump_packet(ether_frame, len);
    for(i = 0 ; i < CLU_MAX_LAN_WLAN_PORT_NUM ; i++)
	{
		clu_ll_addr.sll_ifindex = lan_ifindex[i];
    	sendto(cluster_net_sock_client, ether_frame, len, 
               0, (struct sockaddr *)&clu_ll_addr, sizeof(struct sockaddr_ll));
		debug_print("sendto sll_ifindex %x, len:%x\n", clu_ll_addr.sll_ifindex, len);
    }

    if(-1 == ret)
    {
        debug_print("index=%d, need send other configrations in next packet\n", index);
        index++;
		sleep(1); /* sleep one second before send second snyc packet to fix server can't receive second sync packet */
        goto Next_packets;
    }

    debug_print("total packets = %d, send finish\n", index+1);
}

int main(int argc, char**argv)
{
    cur_seq = next_seq = 0;

    log_pid();
    signal(SIGUSR1, signal_handler);    //need known when configration changed

    if(0!=cluster_client_init())
	{
		printf("cluster_client init failed, exit!\n");
		return 0;
	}

    while(1)
    {
        //sync sequence num first
        cur_seq = next_seq;
        configrations_send();

        sleep(CLU_CLIENT_INTERVAL);
    }

    close(cluster_net_sock_client);
    return 0;
}

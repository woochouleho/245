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
#include <sys/wait.h>


#include "cluster_common.h"
#include "mib.h"
#include "mibtbl.h"
#include "utility.h"

#define CLU_SERVER_RUNFILE	"/var/run/clu_server.pid"

#define IS_MASKED(mask, idx) ((mask[idx/8]&(0x1 << (idx&0x7)))!=0x0)
#define MASK_IDX(mask, idx) (mask[idx/8] |= (0x1 << (idx&0x7)))
#ifdef WLAN_11R
const char FT_DAEMON_PROG[]	= "/bin/ftd";
#endif

extern int wlan_idx;	// interface index

static int cluster_net_sock_server;
static unsigned short cur_seq;
static unsigned char pktidmask[8];
static unsigned char chainidmask[8];
static int totalcnt;     //implys we received the last packet for conf

static void log_pid()
{
	FILE *f;
	pid_t pid;
	pid = getpid();

	if((f = fopen(CLU_SERVER_RUNFILE, "w")) == NULL)
		return;
	fprintf(f, "%d\n", pid);
	fclose(f);
}

static int cluster_server_init(void)
{
CreateSocket:
	cluster_net_sock_server = socket(AF_PACKET, SOCK_RAW, htons(CLUSTER_MANAGE_PROTOCOL));
    if(cluster_net_sock_server==-1){
		debug_print("Creat server socket error.\n");
		sleep(2);
		goto CreateSocket;
	}

    cur_seq = 0;
    totalcnt = 0;
    memset(pktidmask, 0, sizeof(pktidmask));
    memset(chainidmask, 0, sizeof(chainidmask));
	return 0;
}

static int cluster_reinit(void)
{
    cur_seq = 0;
    totalcnt = 0;
    memset(pktidmask, 0, sizeof(pktidmask));
    memset(chainidmask, 0, sizeof(chainidmask));
    return;
}

static int all_received()
{
    int i;
    
	if(totalcnt!=0)
	{
        for(i=0; i< totalcnt; i++)
        {
            if(!IS_MASKED(pktidmask, i))
            {
                debug_print("packet %d has not received.\n", i);
                return 0;
            }
        }
        debug_print("all packets have been received.\n", i);
        return 1;
	}
    else
    {
        return 0;
    }
}


/* Signal handler */
static void signal_handler(int sig)
{
	//ToDo if needed.
	debug_print("do nothing now.\n");
}

static void sigchld_handler(int sig)
{
	int status = 0;
    pid_t pid = 0;

    debug_print("cluster_server got SIGCHLD.\n");
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        debug_print("waitpid() %d\n", pid);
    }

    return;
}


/*
 return:
      0 if add success
    -1 if length exceed the CLUSTER_PKT_MAX_LEN
    -2 if other error exist such as mib get failed.
 */
static int parseOptions(unsigned char *clumsg, unsigned short clu_len)
{
    unsigned char *ptr;
    unsigned short len;
    MIB_CE_MBSSIB_T Entry;
    int orig_wlanid = wlan_idx;
    OPT_TLV_Tp opt;
    unsigned short opt_len;
    unsigned short type;
    int wlan_index;
    unsigned char vChar;
    unsigned int vInt;
    CLU_HDR_Tp cluhdr;

    //skip CLU_HDR
    cluhdr = (CLU_HDR_Tp)clumsg;
    ptr = clumsg+sizeof(CLU_HDR_T);
    len = clu_len - sizeof(CLU_HDR_T);
    while(len > 0)
    {
        if(len < sizeof(OPT_TLV_T))
        {
            printf("invalid packet, len = %d\n", len);
            return -1;
        }

        opt = (OPT_TLV_Tp)ptr;
        opt_len = ntohs(opt->len);
        if(len < opt_len)
        {
            printf("optlen exceed, optlen=%d, len=%d\n", opt_len, len);
        }

        type = ntohs(opt->type);
        option_debug(type, opt_len, opt->data);
        ptr += opt_len;
        len -= opt_len;
        wlan_idx = 0;
        if((type>WLAN0_CONF_START) && (type<WLAN0_CONF_END))
        {
            //wlan0
            wlan_index=0;
            wlan_idx = 0;
        }
        else if((type>WLAN0_VAP1_CONF_START) && (type<WLAN0_VAP1_CONF_END))
        {
            //wlan0-vap1
            wlan_index=1;
            wlan_idx = 0;
        }
        else if((type>WLAN0_VAP2_CONF_START) && (type<WLAN0_VAP2_CONF_END))
        {
            //wlan0-vap2
            wlan_index=2;
            wlan_idx = 0;
        }
        else if((type>WLAN0_VAP3_CONF_START) && (type<WLAN0_VAP3_CONF_END))
        {
            //wlan0-vap3
            wlan_index=3;
            wlan_idx = 0;
        }
        else if((type>WLAN0_VAP4_CONF_START) && (type<WLAN0_VAP4_CONF_END))
        {
            //wlan0-vap4
            wlan_index=4;
            wlan_idx = 0;
        }
        else if((type>WLAN1_CONF_START) && (type<WLAN1_CONF_END))
        {
            //wlan1
            wlan_index=0;
            wlan_idx = 1;
        }
        else if((type>WLAN1_VAP1_CONF_START) && (type<WLAN1_VAP1_CONF_END))
        {
            //wlan1-vap1
            wlan_index=1;
            wlan_idx = 1;
        }
        else if((type>WLAN1_VAP2_CONF_START) && (type<WLAN1_VAP2_CONF_END))
        {
            //wlan1-vap2
            wlan_index=2;
            wlan_idx = 1;
        }
        else if((type>WLAN1_VAP3_CONF_START) && (type<WLAN1_VAP3_CONF_END))
        {
            //wlan1-vap3
            wlan_index=3;
            wlan_idx = 1;
        }
        else if((type>WLAN1_VAP4_CONF_START) && (type<WLAN1_VAP4_CONF_END))
        {
            //wlan1-vap4
            wlan_index=4;
            wlan_idx = 1;
        }
        if((type>WLAN0_CONF_START) && (type<WLAN1_VAP4_CONF_END))
        {
            //MBSSID_TBL configration
            wlan_getEntry(&Entry, wlan_index);
        }
        
        switch (type)
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
                Entry.wlanDisabled = opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wlanDisabled=%d\n", wlan_index, wlan_idx, Entry.wlanDisabled);
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
                memset(&Entry.ssid, 0, sizeof(Entry.ssid));
                memcpy(&Entry.ssid, opt->data, opt_len-4);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.ssid=%s\n", wlan_index, wlan_idx, Entry.ssid);
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
                Entry.wlanMode = opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wlanMode=%d\n", wlan_index, wlan_idx, Entry.wlanMode);
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
                Entry.hidessid= opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.hidessid=%d\n", wlan_index, wlan_idx, Entry.hidessid);
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
                Entry.userisolation= opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.userisolation=%d\n", wlan_index, wlan_idx, Entry.userisolation);
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
                Entry.wlanBand= opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wlanBand=%d\n", wlan_index, wlan_idx, Entry.wlanBand);
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
                Entry.encrypt = opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.encrypt=%d\n", wlan_index, wlan_idx, Entry.encrypt);
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
                Entry.wep = opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wep=%d\n", wlan_index, wlan_idx, Entry.wep);
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
                Entry.wpaAuth= opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wpaAuth=%d\n", wlan_index, wlan_idx, Entry.wpaAuth);
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
                Entry.wpaPSKFormat= opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wpaPSKFormat=%d\n", wlan_index, wlan_idx, Entry.wpaPSKFormat);
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
                memset(&Entry.wpaPSK, 0, sizeof(Entry.wpaPSK));
                memcpy(&Entry.wpaPSK, opt->data, opt_len-4);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wpaPSK=%s\n", wlan_index, wlan_idx, Entry.wpaPSK);
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
                Entry.wpaGroupRekeyTime = ntohl(*(int *)opt->data);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wpaGroupRekeyTime=%d\n", wlan_index, wlan_idx, Entry.wpaGroupRekeyTime);
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
                Entry.authType= opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.authType=%d\n", wlan_index, wlan_idx, Entry.authType);
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
                Entry.unicastCipher= opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.unicastCipher=%d\n", wlan_index, wlan_idx, Entry.unicastCipher);
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
                Entry.wpa2UnicastCipher= opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wpa2UnicastCipher=%d\n", wlan_index, wlan_idx, Entry.wpa2UnicastCipher);
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
                memset(&Entry.wapiPsk, 0, sizeof(Entry.wapiPsk));
                memcpy(&Entry.wapiPsk, opt->data, opt_len-4);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wapiPsk=%s\n", wlan_index, wlan_idx, Entry.wapiPsk);
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
                Entry.wapiPskLen= opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wapiPskLen=%d\n", wlan_index, wlan_idx, Entry.wapiPskLen);
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
                Entry.wapiAuth= opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wapiAuth=%d\n", wlan_index, wlan_idx, Entry.wapiAuth);
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
                Entry.wapiPskFormat= opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wapiPskFormat=%d\n", wlan_index, wlan_idx, Entry.wapiPskFormat);
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
                *((unsigned long *)Entry.wapiAsIpAddr) = ntohl(*(unsigned long *)opt->data);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wapiAsIpAddr=%d\n", wlan_index, wlan_idx, Entry.wapiAsIpAddr);
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
                Entry.wepKeyType= opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wepKeyType=%d\n", wlan_index, wlan_idx, Entry.wepKeyType);
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
                Entry.wepDefaultKey= opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wepDefaultKey=%d\n", wlan_index, wlan_idx, Entry.wepDefaultKey);
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
                memset(&Entry.wep64Key1, 0, sizeof(Entry.wep64Key1));
                memcpy(&Entry.wep64Key1, opt->data, opt_len-4);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wep64Key1=%s\n", wlan_index, wlan_idx, Entry.wep64Key1);
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
                memset(&Entry.wep64Key2, 0, sizeof(Entry.wep64Key2));
                memcpy(&Entry.wep64Key2, opt->data, opt_len-4);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wep64Key2=%s\n", wlan_index, wlan_idx, Entry.wep64Key2);
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
                memset(&Entry.wep64Key3, 0, sizeof(Entry.wep64Key3));
                memcpy(&Entry.wep64Key3, opt->data, opt_len-4);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wep64Key3=%s\n", wlan_index, wlan_idx, Entry.wep64Key3);
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
                memset(&Entry.wep64Key4, 0, sizeof(Entry.wep64Key4));
                memcpy(&Entry.wep64Key4, opt->data, opt_len-4);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wep64Key4=%s\n", wlan_index, wlan_idx, Entry.wep64Key4);
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
                memset(&Entry.wep128Key1, 0, sizeof(Entry.wep128Key1));
                memcpy(&Entry.wep128Key1, opt->data, opt_len-4);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wep128Key1=%s\n", wlan_index, wlan_idx, Entry.wep128Key1);
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
                memset(&Entry.wep128Key2, 0, sizeof(Entry.wep128Key2));
                memcpy(&Entry.wep128Key2, opt->data, opt_len-4);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wep128Key2=%s\n", wlan_index, wlan_idx, Entry.wep128Key2);
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
                memset(&Entry.wep128Key3, 0, sizeof(Entry.wep128Key3));
                memcpy(&Entry.wep128Key3, opt->data, opt_len-4);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wep128Key3=%s\n", wlan_index, wlan_idx, Entry.wep128Key3);
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
                memset(&Entry.wep128Key4, 0, sizeof(Entry.wep128Key4));
                memcpy(&Entry.wep128Key4, opt->data, opt_len-4);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.wep128Key4=%s\n", wlan_index, wlan_idx, Entry.wep128Key4);
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
                memset(&Entry.ft_mdid, 0, sizeof(Entry.ft_mdid));
                memcpy(&Entry.ft_mdid, opt->data, opt_len-4);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.ft_mdid=%s\n", wlan_index, wlan_idx, Entry.ft_mdid);
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
                Entry.ft_enable = opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.ft_enable=%d\n", wlan_index, wlan_idx, Entry.ft_enable);
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
                Entry.ft_over_ds = opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.ft_over_ds=%d\n", wlan_index, wlan_idx, Entry.ft_over_ds);
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
                Entry.ft_res_request = opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.ft_res_request=%d\n", wlan_index, wlan_idx, Entry.ft_res_request);
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
                Entry.ft_r0key_timeout = ntohl(*(int *)opt->data);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.ft_r0key_timeout=%d\n", wlan_index, wlan_idx, Entry.ft_r0key_timeout);
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
                Entry.ft_reasoc_timeout = ntohl(*(int *)opt->data);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.ft_reasoc_timeout=%d\n", wlan_index, wlan_idx, Entry.ft_reasoc_timeout);
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
                memset(&Entry.ft_r0kh_id, 0, sizeof(Entry.ft_r0kh_id));
                memcpy(&Entry.ft_r0kh_id, opt->data, opt_len-4);
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.ft_r0kh_id=%s\n", wlan_index, wlan_idx, Entry.ft_r0kh_id);
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
                Entry.ft_push = opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.ft_push=%d\n", wlan_index, wlan_idx, Entry.ft_push);
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
                Entry.ft_kh_num = opt->data[0];
                wlan_setEntry(&Entry, wlan_index);
                debug_print("wlan_index=%d, wlan_idx=%d, Entry.ft_kh_num=%d\n", wlan_index, wlan_idx, Entry.ft_kh_num);
                break;
#endif
#endif
            //11R end
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_AUTO_CHAN_ENABLED:
                wlan_idx = 1;
#endif
            case WLAN0_AUTO_CHAN_ENABLED:
                mib_get(MIB_WLAN_AUTO_CHAN_ENABLED, &vChar);
                if(vChar != opt->data[0])
                {
                    mib_set(MIB_WLAN_AUTO_CHAN_ENABLED, opt->data);
                }
                break;
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_CHAN_NUM:
                wlan_idx = 1;
#endif
            case WLAN0_CHAN_NUM:
                mib_get(MIB_WLAN_CHAN_NUM, &vChar);
                if(vChar != opt->data[0])
                {
                    mib_set(MIB_WLAN_CHAN_NUM, opt->data);
                }
                break;
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_CHANNEL_WIDTH:
                wlan_idx = 1;
#endif
            case WLAN0_CHANNEL_WIDTH:
                mib_get(MIB_WLAN_CHANNEL_WIDTH, &vChar);
                if(vChar != opt->data[0])
                {
                    mib_set(MIB_WLAN_CHANNEL_WIDTH, opt->data);
                }
                break;
#ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_11N_COEXIST:
                wlan_idx = 1;
#endif
            case WLAN0_11N_COEXIST:
                mib_get(MIB_WLAN_11N_COEXIST, &vChar);
                if(vChar != opt->data[0])
                {
                    mib_set(MIB_WLAN_11N_COEXIST, opt->data);
                }
                break;
#if 0
            case MAC_FILTER_SRC_ENABLE:
                mib_get(MIB_MAC_FILTER_SRC_ENABLE, &vChar);
                if(vChar != opt->data[0])
                {
                    mib_set(MIB_MAC_FILTER_SRC_ENABLE, opt->data);
                }
                break;
            case IPFILTER_ON_OFF:
                mib_get(MIB_IPFILTER_ON_OFF, &vChar);
                if(vChar != opt->data[0])
                {
                    mib_set(MIB_IPFILTER_ON_OFF, opt->data);
                }
                break;
#endif
#ifdef URL_BLOCKING_SUPPORT
            case URL_CAPABILITY:
                mib_get(MIB_URL_CAPABILITY, &vChar);
                if(vChar != opt->data[0])
                {
                    mib_set(MIB_URL_CAPABILITY, opt->data);
                }
                break;
#endif
            case DOS_ENABLED:
                mib_get(MIB_DOS_ENABLED, &vInt);
                if(vInt != ntohl(*(unsigned int *)opt->data))
                {
                    vInt = ntohl(*(unsigned int *)opt->data);
                    mib_set(MIB_DOS_ENABLED, &vInt);
                }
                break;
#if 0
            case FW_GRADE:
                mib_get(MIB_FW_GRADE, &vChar);
                if(vChar != opt->data[0])
                {
                    mib_set(MIB_FW_GRADE, opt->data);
                }
                break;
#endif
			case DHCP_MODE:
                mib_get(MIB_DHCP_MODE, &vChar);
                if(vChar != opt->data[0])
                {
                    mib_set(MIB_DHCP_MODE, opt->data);
                }
                break;
		#ifdef CONFIG_IPV6
		#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
			case DHCPV6_MODE:
				mib_get(MIB_DHCPV6_MODE, &vChar);
                if(vChar != opt->data[0])
                {
                    mib_set(MIB_DHCPV6_MODE, opt->data);
                }
				break;
		#endif
		#endif
#ifdef WLAN_11R
            //mib table
 #ifdef WLAN_DUALBAND_CONCURRENT
            case WLAN1_FTKH_TBL:
                wlan_idx = 1;
#endif
            case WLAN0_FTKH_TBL:
                {
                    int i,j;
                    int orig_id;
                    unsigned int idx = type - MIB_CHAIN_START - 1;
                    unsigned short length = ntohs(opt->len);
                    MIB_CE_WLAN_FTKH_Tp khEntry;

                    if(length != (sizeof(MIB_CE_WLAN_FTKH_T)+4))
                    {
                        printf("[%s %d]length of type[%d] is invalid, len=%d, validlen=%d\n", __func__, __LINE__, type, length, sizeof(MIB_CE_WLAN_FTKH_T)+4);
                        goto PARSE_FAIL;
                    }
                    if(!IS_MASKED(chainidmask, idx))
                    {
                        //receive the first entry for mib chain, clear orig value;
                        mib_chain_clear(MIB_WLAN_FTKH_TBL);
                        //set ft_kh_num to 0 for every MBSSID_TBL
                        orig_id = wlan_idx;
                        for(i = 0; i<NUM_WLAN_INTERFACE; i++)
                        {
                            wlan_idx = i;

                            for (j=1; j<=NUM_VWLAN_INTERFACE; j++)
                            {
                                if (!mib_chain_get(MIB_MBSSIB_TBL, j, (void *)&Entry)) 
                                {
                      				printf("Error! Get MIB_MBSSIB_TBL for %d error, wlan_idx=%d.\n", j, wlan_idx);
                                    continue;
			                    }
                                if(0 != Entry.ft_kh_num)
                                {
                                    Entry.ft_kh_num = 0;
                                    mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, j);
                                }
                            }
                        }
                        wlan_idx = orig_id;
                        MASK_IDX(chainidmask, idx);
                    }
                    
                    khEntry = (MIB_CE_WLAN_FTKH_Tp)opt->data;
                    wlan_idx = khEntry->wlanIdx;
                    if (!mib_chain_get(MIB_MBSSIB_TBL, khEntry->intfIdx, (void *)&Entry)) 
                    {
          				printf("Error! Get MIB_MBSSIB_TBL for %d error, wlan_idx=%d.\n", khEntry->intfIdx, wlan_idx);
                        goto PARSE_FAIL;
                    }
                    if ( Entry.ft_kh_num >= MAX_VWLAN_FTKH_NUM ) {
            			printf("Error! Entry.ft_kh_num reach max[%d], interface[%d], wlan_idx[%d], can not add khEntry.\n", MAX_VWLAN_FTKH_NUM, khEntry->intfIdx, wlan_idx);
            			goto PARSE_FAIL;;
            		}
                    else
                    {
                        Entry.ft_kh_num++;
                        mib_chain_add(MIB_WLAN_FTKH_TBL, (void *)opt->data);
                        mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, khEntry->intfIdx);
                    }
                }
                break;
#endif
#ifdef SUPPORT_TIME_SCHEDULE
            case TIME_SCHEDULE_TBL:
                {
                    unsigned int idx = type - MIB_CHAIN_START - 1;
                    unsigned short length = ntohs(opt->len);
                    MIB_CE_TIME_SCHEDULE_T schdEntry;
                    int i;

                    if(length != (sizeof(MIB_CE_TIME_SCHEDULE_T)+4))
                    {
                        printf("[%s %d]length of type[%d] is invalid, len=%d, validlen=%d\n", __func__, __LINE__, type, length, sizeof(MIB_CE_TIME_SCHEDULE_T)+4);
                        goto PARSE_FAIL;
                    }
                    if(!IS_MASKED(chainidmask, idx))
                    {
                        //receive the first entry for mib chain, clear orig value;
                        mib_chain_clear(MIB_TIME_SCHEDULE_TBL);
                        MASK_IDX(chainidmask, idx);
                    }
                    memcpy(&schdEntry, opt->data, _OFFSET_CE(MIB_CE_TIME_SCHEDULE_T, sch_time));
                    for(i=0; i<7; i++)
                    {
                        ((unsigned int *)(schdEntry.sch_time))[i] = ntohl(*(unsigned int *)(opt->data+_OFFSET_CE(MIB_CE_TIME_SCHEDULE_T, sch_time)+4*i));
                    }
                    mib_chain_add(MIB_TIME_SCHEDULE_TBL, (void *)&schdEntry);
                }
                break;
#endif
#if 0
            case MAC_FILTER_ROUTER_TBL:
                {
                    unsigned int idx = type - MIB_CHAIN_START - 1;
                    unsigned short length = ntohs(opt->len);
                    MIB_CE_ROUTEMAC_T Entry;

                    if(length != (sizeof(MIB_CE_ROUTEMAC_T)+4))
                    {
                        printf("[%s %d]length of type[%d] is invalid, len=%d, validlen=%d\n", __func__, __LINE__, type, length, sizeof(MIB_CE_ROUTEMAC_T)+4);
                        goto PARSE_FAIL;
                    }
                    if(!IS_MASKED(chainidmask, idx))
                    {
                        //receive the first entry for mib chain, clear orig value;
                        mib_chain_clear(MIB_MAC_FILTER_ROUTER_TBL);
                        MASK_IDX(chainidmask, idx);
                    }
#ifdef SUPPORT_TIME_SCHEDULE
                    memcpy(&Entry, opt->data, _OFFSET_CE(MIB_CE_ROUTEMAC_T, schedIndex));
                    Entry.schedIndex = ntohl(*(unsigned int *)(opt->data+_OFFSET_CE(MIB_CE_ROUTEMAC_T, schedIndex)));
                    mib_chain_add(MIB_MAC_FILTER_ROUTER_TBL, (void *)&Entry);
#else
                    mib_chain_add(MIB_MAC_FILTER_ROUTER_TBL, (void *)opt->data);
#endif
                }
                break;
#endif
            case IP_PORT_FILTER_TBL:
                {
                    unsigned int idx = type - MIB_CHAIN_START - 1;
                    unsigned short length = ntohs(opt->len);
                    MIB_CE_IP_PORT_FILTER_T Entry;
                    MIB_CE_IP_PORT_FILTER_Tp pEntry;

                    if(length != (sizeof(MIB_CE_IP_PORT_FILTER_T)+4))
                    {
                        printf("[%s %d]length of type[%d] is invalid, len=%d, validlen=%d\n", __func__, __LINE__, type, length, sizeof(MIB_CE_IP_PORT_FILTER_T)+4);
                        goto PARSE_FAIL;
                    }
                    if(!IS_MASKED(chainidmask, idx))
                    {
                        //receive the first entry for mib chain, clear orig value;
                        mib_chain_clear(MIB_IP_PORT_FILTER_TBL);
                        MASK_IDX(chainidmask, idx);
                    }
                    pEntry = (MIB_CE_IP_PORT_FILTER_Tp)opt->data;
                    Entry.action = pEntry->action;
                    *(unsigned int *)Entry.srcIp = ntohl(*(unsigned int *)pEntry->srcIp);
                    *(unsigned int *)Entry.dstIp = ntohl(*(unsigned int *)pEntry->dstIp);
                    Entry.smaskbit = pEntry->smaskbit;
                    Entry.dmaskbit = pEntry->dmaskbit;
                    Entry.srcPortFrom = ntohs(pEntry->srcPortFrom);
                    Entry.dstPortFrom = ntohs(pEntry->dstPortFrom);
                    Entry.srcPortTo = ntohs(pEntry->srcPortTo);
                    Entry.dstPortTo = ntohs(pEntry->dstPortTo);
                    Entry.dir = pEntry->dir;
                    Entry.protoType = pEntry->protoType;
#if 0
                    memcpy(Entry.name, pEntry->name, sizeof(Entry.name));
                    *(unsigned int *)Entry.srcIp2 = ntohl(*(unsigned int *)pEntry->srcIp2);
                    *(unsigned int *)Entry.dstIp2 = ntohl(*(unsigned int *)pEntry->dstIp2);
#ifdef CONFIG_IPV6
                    Entry.IpProtocol= pEntry->IpProtocol;
                    memcpy(Entry.sip6Start, pEntry->sip6Start, sizeof(Entry.sip6Start));
                    memcpy(Entry.sip6End, pEntry->sip6End, sizeof(Entry.sip6End));
                    memcpy(Entry.dip6Start, pEntry->dip6Start, sizeof(Entry.dip6Start));
                    memcpy(Entry.dip6End, pEntry->dip6End, sizeof(Entry.dip6End));
                    Entry.sip6PrefixLen= pEntry->sip6PrefixLen;
                    Entry.dip6PrefixLen= pEntry->dip6PrefixLen;
#endif
                    
#ifdef SUPPORT_TIME_SCHEDULE
                    Entry.schedIndex = ntohl(pEntry->schedIndex);
                    Entry.aclIndex = ntohl(pEntry->aclIndex);
#endif
#endif
                    mib_chain_add(MIB_IP_PORT_FILTER_TBL, (void *)&Entry);
                }
                break;
#ifdef URL_BLOCKING_SUPPORT
            case URL_FQDN_TBL:
                {
                    unsigned int idx = type - MIB_CHAIN_START - 1;
                    unsigned short length = ntohs(opt->len);
                    MIB_CE_URL_FQDN_T Entry;
                    MIB_CE_URL_FQDN_Tp pEntry;

                    if(length != (sizeof(MIB_CE_URL_FQDN_T)+4))
                    {
                        printf("[%s %d]length of type[%d] is invalid, len=%d, validlen=%d\n", __func__, __LINE__, type, length, sizeof(MIB_CE_URL_FQDN_T)+4);
                        goto PARSE_FAIL;
                    }
                    if(!IS_MASKED(chainidmask, idx))
                    {
                        //receive the first entry for mib chain, clear orig value;
                        mib_chain_clear(MIB_URL_FQDN_TBL);
                        MASK_IDX(chainidmask, idx);
                    }
                    pEntry = (MIB_CE_URL_FQDN_Tp)opt->data;
                    memcpy(Entry.fqdn, pEntry->fqdn, sizeof(Entry.fqdn));
                    //memcpy(Entry.url, pEntry->url, sizeof(Entry.url));
                    //memcpy(Entry.key, pEntry->key, sizeof(Entry.key));
                    //Entry.port = ntohs(pEntry->port);
#ifdef SUPPORT_TIME_SCHEDULE
                    Entry.schedIndex = ntohl(pEntry->schedIndex);
#endif
                    mib_chain_add(MIB_URL_FQDN_TBL, (void *)&Entry);
                }
                break;
#endif
#ifdef REMOTE_ACCESS_CTL
            case ACC_TBL:
                {
                    unsigned int idx = type - MIB_CHAIN_START - 1;
                    unsigned short length = ntohs(opt->len);
                    MIB_CE_ACC_T Entry;
                    MIB_CE_ACC_Tp pEntry;

                    if(length != (sizeof(MIB_CE_ACC_T)+4))
                    {
                        printf("[%s %d]length of type[%d] is invalid, len=%d, validlen=%d\n", __func__, __LINE__, type, length, sizeof(MIB_CE_ACC_T)+4);
                        goto PARSE_FAIL;
                    }
                    if(!IS_MASKED(chainidmask, idx))
                    {
                        //receive the first entry for mib chain, clear orig value;
                        mib_chain_clear(MIB_ACC_TBL);
                        MASK_IDX(chainidmask, idx);
                    }
                    pEntry = (MIB_CE_ACC_Tp)opt->data;
                    Entry.telnet= pEntry->telnet;
                    Entry.ftp= pEntry->ftp;
                    Entry.tftp= pEntry->tftp;
                    Entry.web= pEntry->web;
                    Entry.snmp= pEntry->snmp;
                    Entry.ssh= pEntry->ssh;
                    Entry.icmp= pEntry->icmp;
                    Entry.netlog= pEntry->netlog;
                    Entry.smb= pEntry->smb;
                    Entry.https= pEntry->https;
                    Entry.telnet_port = ntohs(pEntry->telnet_port);
                    Entry.web_port = ntohs(pEntry->web_port);
                    Entry.ftp_port = ntohs(pEntry->ftp_port);
                    Entry.https_port = ntohs(pEntry->https_port);
                    mib_chain_add(MIB_ACC_TBL, (void *)&Entry);
                }
                break;
#endif
			case ATM_VC_TBL:
				{
                    unsigned int idx = type - MIB_CHAIN_START - 1;
                    unsigned short length = ntohs(opt->len);
                    MIB_CE_ATM_VC_T Entry;
                    MIB_CE_ATM_VC_WANTp pEntry;

					printf("[%s %d] type[%d] len=%d, validlen=%d\n", __func__, __LINE__, type, length, sizeof(MIB_CE_ATM_VC_WAN_T)+4);
                    if(length != (sizeof(MIB_CE_ATM_VC_WAN_T)+4))
                    {
                        printf("[%s %d]length of type[%d] is invalid, len=%d, validlen=%d\n", __func__, __LINE__, type, length, sizeof(MIB_CE_ATM_VC_WAN_T)+4);
                        //goto PARSE_FAIL;
                        //skip this section
                        break;
                    }
                    if(!IS_MASKED(chainidmask, idx))
                    {
                        //receive the first entry for mib chain, clear orig value;
                        mib_chain_clear(MIB_ATM_VC_TBL);
                        MASK_IDX(chainidmask, idx);
                    }
                    pEntry = (MIB_CE_ATM_VC_WANTp)opt->data;

					Entry.ifIndex= htonl(pEntry->ifIndex);
                    Entry.vpi= pEntry->vpi;
                    Entry.qos= pEntry->qos;
                    Entry.vci= htons(pEntry->vci);
                    Entry.pcr= htons(pEntry->pcr);
                    Entry.scr= htons(pEntry->scr);
                    Entry.mbs= htons(pEntry->mbs);
                    Entry.cdvt= htonl(pEntry->cdvt);
                    Entry.encap= pEntry->encap;
                    Entry.napt= pEntry->napt;
					Entry.cmode= pEntry->cmode;
					Entry.brmode= pEntry->brmode;
					memcpy(Entry.pppUsername, pEntry->pppUsername, sizeof(Entry.pppUsername));
					memcpy(Entry.pppPassword, pEntry->pppPassword, sizeof(Entry.pppPassword));
					Entry.pppAuth= pEntry->pppAuth;
					memcpy(Entry.pppACName, pEntry->pppACName, sizeof(Entry.pppACName));
					memcpy(Entry.pppServiceName, pEntry->pppServiceName, sizeof(Entry.pppServiceName));
					Entry.pppCtype= pEntry->pppCtype;
					Entry.pppIdleTime= htons(pEntry->pppIdleTime);
				#ifdef CONFIG_USER_IP_QOS
					Entry.enableIpQos= pEntry->enableIpQos;
				#endif
				#ifdef CONFIG_IGMPPROXY_MULTIWAN
					Entry.enableIGMP= pEntry->enableIGMP;
				#endif
					Entry.ipDhcp= pEntry->ipDhcp;
					Entry.rip= pEntry->rip;
					*(int *)Entry.ipAddr= htonl(*(int *)pEntry->ipAddr);
					*(int *)Entry.remoteIpAddr= htonl(*(int *)pEntry->remoteIpAddr);
					Entry.dgw= pEntry->dgw;
					Entry.mtu= htonl(pEntry->mtu);
					Entry.enable= pEntry->enable;
					*(int *)Entry.netMask= htonl(*(int *)pEntry->netMask);
					Entry.ipunnumbered= pEntry->ipunnumbered;
					Entry.dnsMode= pEntry->dnsMode;
					*(int *)Entry.v4dns1= htonl(*(int *)pEntry->v4dns1);
					*(int *)Entry.v4dns2= htonl(*(int *)pEntry->v4dns2);
				#if defined(ITF_GROUP) || defined(NEW_PORTMAPPING)
					Entry.vlan= pEntry->vlan;
					Entry.vid= htons(pEntry->vid);
					Entry.vprio= htons(pEntry->vprio);
					Entry.vpass= pEntry->vpass;
					Entry.itfGroup= htons(pEntry->itfGroup);
				#endif
				#ifdef CONFIG_MCAST_VLAN
					Entry.mVid= htons(pEntry->mVid);
				#endif
					Entry.cpePppIfIndex= htonl(pEntry->cpePppIfIndex);
					Entry.cpeIpIndex= htonl(pEntry->cpeIpIndex);
				#ifdef _CWMP_MIB_
					Entry.connDisable= pEntry->connDisable;
					Entry.ConDevInstNum= htonl(pEntry->ConDevInstNum);
					Entry.ConIPInstNum= htonl(pEntry->ConIPInstNum);
					Entry.ConPPPInstNum= htonl(pEntry->ConPPPInstNum);
					Entry.autoDisTime= htons(pEntry->autoDisTime);
					Entry.warnDisDelay= htons(pEntry->warnDisDelay);
				#ifdef _PRMT_TR143_
					Entry.TR143UDPEchoItf= pEntry->TR143UDPEchoItf;
				#endif
				#ifdef _PRMT_X_CT_COM_WANEXT_
					Entry.ServiceList= pEntry->ServiceList;
					memcpy(Entry.IPForwardList, pEntry->IPForwardList, sizeof(Entry.IPForwardList));
				#endif
				#endif
					memcpy(Entry.WanName, pEntry->WanName, sizeof(Entry.WanName));
				#ifdef CONFIG_USER_PPPOE_PROXY
					Entry.PPPoEProxyEnable= pEntry->PPPoEProxyEnable;
					Entry.PPPoEProxyMaxUser= htonl(pEntry->PPPoEProxyMaxUser);
				#endif
					Entry.applicationtype= htonl(pEntry->applicationtype);
					//Entry.disableLanDhcp= pEntry->disableLanDhcp;
					//Entry.svtype= pEntry->svtype;
				#ifdef CONFIG_SPPPD_STATICIP
					Entry.pppIp= pEntry->pppIp;
				#endif
				#ifdef CONFIG_USER_WT_146
					Entry.bfd_enable= pEntry->bfd_enable;
					Entry.bfd_opmode= pEntry->bfd_opmode;
					Entry.bfd_role= pEntry->bfd_role;
					Entry.bfd_mintxint= htonl(pEntry->bfd_mintxint);
					Entry.bfd_minrxint= htonl(pEntry->bfd_minrxint);
					Entry.bfd_minechorxint= htonl(pEntry->bfd_minechorxint);
					Entry.bfd_detectmult= pEntry->bfd_detectmult;
					Entry.bfd_authtype= pEntry->bfd_authtype;
					Entry.bfd_authkeyid= pEntry->bfd_authkeyid;
					Entry.bfd_authkeylen= pEntry->bfd_authkeylen;
					memcpy(Entry.bfd_authkey, pEntry->bfd_authkey, sizeof(Entry.bfd_authkey));
					Entry.bfd_dscp= pEntry->bfd_dscp;
					Entry.bfd_ethprio= pEntry->bfd_ethprio;
				#endif
				#ifdef CONFIG_IPV6
					Entry.IpProtocol= pEntry->IpProtocol;
					Entry.AddrMode= pEntry->AddrMode;
					memcpy(Entry.Ipv6Addr, pEntry->Ipv6Addr, sizeof(Entry.Ipv6Addr));
					memcpy(Entry.RemoteIpv6Addr, pEntry->RemoteIpv6Addr, sizeof(Entry.RemoteIpv6Addr));
					//Entry.IPv6PrefixOrigin= pEntry->IPv6PrefixOrigin;
					//memcpy(Entry.Ipv6Prefix, pEntry->Ipv6Prefix, sizeof(Entry.Ipv6Prefix));
					Entry.Ipv6AddrPrefixLen= pEntry->Ipv6AddrPrefixLen;
					//Entry.IPv6PrefixPltime= htonl(pEntry->IPv6PrefixPltime);
					//Entry.IPv6PrefixVltime= htonl(pEntry->IPv6PrefixVltime);
					//memcpy(Entry.IPv6DomainName, pEntry->IPv6DomainName, sizeof(Entry.IPv6DomainName));
					Entry.Ipv6Dhcp= pEntry->Ipv6Dhcp;
					Entry.Ipv6DhcpRequest= pEntry->Ipv6DhcpRequest;
					memcpy(Entry.RemoteIpv6EndPointAddr, pEntry->RemoteIpv6EndPointAddr, sizeof(Entry.RemoteIpv6EndPointAddr));
					//memcpy(Entry.Ipv6AddrAlias, pEntry->Ipv6AddrAlias, sizeof(Entry.Ipv6AddrAlias));
					//memcpy(Entry.Ipv6PrefixAlias, pEntry->Ipv6PrefixAlias, sizeof(Entry.Ipv6PrefixAlias));
					memcpy(Entry.Ipv6Dns1, pEntry->Ipv6Dns1, sizeof(Entry.Ipv6Dns1));
					memcpy(Entry.Ipv6Dns2, pEntry->Ipv6Dns2, sizeof(Entry.Ipv6Dns2));
				#endif
				#ifdef CONFIG_IPV6_SIT_6RD
					*(int *)Entry.SixrdBRv4IP= htonl(*(int *)pEntry->SixrdBRv4IP);
					Entry.SixrdIPv4MaskLen= pEntry->SixrdIPv4MaskLen;
					memcpy(Entry.SixrdPrefix, pEntry->SixrdPrefix, sizeof(Entry.SixrdPrefix));
					Entry.SixrdPrefixLen= pEntry->SixrdPrefixLen;
				#endif
					memcpy(Entry.MacAddr, pEntry->MacAddr, sizeof(Entry.MacAddr));
				#ifdef CONFIG_RTK_RG_INIT
					Entry.rg_wan_idx= htonl(pEntry->rg_wan_idx);
					Entry.check_br_pm= htonl(pEntry->check_br_pm);
				#endif
				#if defined(CONFIG_GPON_FEATURE)
					Entry.sid= pEntry->sid;
				#endif
                #if defined(CONFIG_IPV6) && defined(DUAL_STACK_LITE)
					Entry.dslite_enable= pEntry->dslite_enable;
					Entry.dslite_aftr_mode= pEntry->dslite_aftr_mode;
					memcpy(Entry.dslite_aftr_addr, pEntry->dslite_aftr_addr, sizeof(Entry.dslite_aftr_addr));
					memcpy(Entry.dslite_aftr_hostname, pEntry->dslite_aftr_hostname, sizeof(Entry.dslite_aftr_hostname));
				#endif
				#if defined(CONFIG_IPV6)
					Entry.dnsv6Mode= pEntry->dnsv6Mode;
				#endif

                    mib_chain_add(MIB_ATM_VC_TBL, (void *)&Entry);
                }
                break;	

            case TYPE_IDX_END:
                {
                    unsigned short index;

                    index = ntohs(cluhdr->id);
                    totalcnt = index + 1;
                    wlan_idx = orig_wlanid;
                    
                    debug_print("meet last Option, stop parse.\n");
                    return 0;
                }
            default:
                break;
        }
    }

PARSE_FAIL:
    wlan_idx = orig_wlanid;
    return 0;
}

static void handle_conf_packet(unsigned char *recvbuffer, unsigned short rcvlen)
{
    unsigned short len = 0;
    unsigned short checksum = 0;
    unsigned short seq = 0;
    unsigned short totlen = 0;
    unsigned short index = 0;
    struct ethhdr *machdr;
    CLU_HDR_Tp cluhdr;
    int ret;

    machdr = (struct ethhdr *)recvbuffer;
    if(machdr->h_proto != htons(CLUSTER_MANAGE_PROTOCOL))
    {
        //eth proto not match
        debug_print("invalid eth proto(%04x)\n", ntohs(machdr->h_proto));
        return;
    }

    cluhdr = (CLU_HDR_Tp)(recvbuffer + ETH_HLEN);
    totlen = ntohs(cluhdr->totlen);
    if(totlen > (rcvlen-ETH_HLEN))
    {
        //invalid total length
        debug_print("invalid packet length\n");
        return;
    }
    
    checksum = ntohs(cluhdr->checksum);
    cluhdr->checksum = 0;
    if(checksum != get_checksum(recvbuffer+ETH_HLEN, rcvlen-ETH_HLEN))
    {
        //invalid checksum
        debug_print("invalid checksum\n");
        return;
    }
    seq = ntohs(cluhdr->sequence);

    if(seq!=cur_seq)
    {
        //new configration packet received
        cluster_reinit();
        cur_seq = seq;
    }

    index = ntohs(cluhdr->id);
    if(IS_MASKED(pktidmask, index))
    {
        //same conf packets received, ignore it.
        debug_print("sequense[%d] index[%d] has beed handle yet...\n", seq, index);
        return;
    }
    else
    {
        debug_print("Received packet sequense[%d] index[%d].\n", seq, index);
        MASK_IDX(pktidmask, index);
        parseOptions(recvbuffer+ETH_HLEN, rcvlen-ETH_HLEN);
    }

    //take effect new conf.
    debug_print("totalcnt:%d\n", totalcnt);
    if((totalcnt!=0)&&(all_received()))
    {
        unsigned char urlcap; // 0-off, 1-black list, 2-white list
        unsigned char filterEnable;
        unsigned int dosEnble;	//1- 使能;  0- 禁用
        unsigned char filterLevel;	//1- 低;  2- 中;  3- 高
        
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
		
        //WiFi
        debug_print("Apply Wlan conf.\n");
        config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);

        //11r
#ifdef WLAN_11R
        debug_print("Apply WLAN_11R conf.\n");
        genFtKhConfig();
		va_cmd(FT_DAEMON_PROG, 1 , 1 , "-clear");
		va_cmd(FT_DAEMON_PROG, 1 , 1 , "-update");
#endif
        //SECURITY FILTER
        debug_print("Apply SECURITY conf.\n");
        restart_IPFilter_DMZ_MACFilter();

        //url filter
        set_url_filter();
		
		restart_dhcp();
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
		//start_dhcpv6(0);	/* stop dhcpv6 server */
#endif
#endif

		restartWAN(CONFIGALL, NULL);

#if 0
        //FIXME
		{	/* set lan port link down and up to force lan pc release/renew ip */
			unsigned char down_time;
			int i;
			system("/bin/diag port set phy-force-power-down port 0-3 state enable");
			mib_get(MIB_LINK_DOWN_TIME,(void*)&down_time);
			printf("link down_time=%d\n",down_time);
			for(i=0;i<down_time;i++)
				sleep(1);
			system("/bin/diag port set phy-force-power-down port 0-3 state disable");
		}
#endif
    }
    
    return;
}

int main(int argc, char**argv)
{
    int rcvlen = 0;
    int clilen=0;
    struct sockaddr_ll cliaddr;
    unsigned char recvbuffer[CLUSTER_PKT_MAX_LEN];
    
    log_pid();
    signal(SIGCHLD, sigchld_handler);    //need known when configration changed
    signal(SIGUSR1, signal_handler);    //need known when configration changed

    cluster_server_init();

    while(1)
    {
        rcvlen = recvfrom(cluster_net_sock_server,recvbuffer,sizeof(recvbuffer),0,(struct sockaddr*)&cliaddr,&clilen);

        if(rcvlen>0)
        {
            debug_print("Receive conf sync packet.\n");
            //debug_dump_packet(recvbuffer, rcvlen);

            handle_conf_packet(recvbuffer, rcvlen);
        }
        
    }

    close(cluster_net_sock_server);
	return 0;
}


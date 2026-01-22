
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h> 

#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h> 

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/socket.h> 
#include <sys/param.h>
#include <sys/syscall.h>

#include <linux/netlink.h> 
#include <netpacket/packet.h>
#include <net/if_arp.h> 
#include <linux/if_ether.h>

#include <arpa/inet.h>

#include <rtk/mib.h>
#include <rtk/utility.h>
#include <sched.h>

#define ARP_PKT_LEN		42
#define ETH_ALEN	6	

unsigned char arpRequestPacket[ARP_PKT_LEN];
struct in_addr dstAddr;
int debug = 0;

static void dump_buffer(unsigned char *buf,int size)
{
    {
    	int i;
    	printf("=================Data Size:%d =================\n",size);
    	for(i=0;i<size;i++)
    	{      
    		if((0==i%16)&&(i>0)) 
    			printf("\n");
    		printf("%02x ",buf[i]); 
    	}
    	printf("\n");
    }
}

static int ARPRequestCreate(char *ifname)
{
	unsigned int i, off;
	unsigned char *pARPData = (unsigned char *)arpRequestPacket;
	unsigned char arp_hdr_len = sizeof(struct arphdr);
	struct ethhdr *pL2Hdr = (struct ethhdr *)pARPData;
	struct arphdr *arpHdr = (struct arphdr *)&pARPData[ETH_HLEN];
	unsigned char sMac[ETH_ALEN];
	struct in_addr inAddr;
	struct sockaddr sa;

	getInAddr(ifname, HW_ADDR, (void *)&sa);
	memcpy(sMac, sa.sa_data, ETH_ALEN);
	
	getInAddr(ifname, IP_ADDR, &inAddr);

	printf("%s,%d, get hw %02x:%02x:%02x:%02x:%02x:%02x, ip is %s\n", __FUNCTION__, __LINE__,
		sMac[0],sMac[1],sMac[2],sMac[3],sMac[4],sMac[5], inet_ntoa(inAddr));

	memset(pARPData, 0, ARP_PKT_LEN);

	/* set Ethernet Header  */
	memset(pL2Hdr->h_dest, 0xff, ETH_ALEN);
	memcpy(pL2Hdr->h_source, sMac, ETH_ALEN);
	pL2Hdr->h_proto = htons(ETH_P_ARP);

	/* set ARP Header  */
	arpHdr->ar_hrd = htons(ARPHRD_ETHER);
	arpHdr->ar_pro = htons(ETH_P_IP);

	arpHdr->ar_hln = ETH_ALEN;
	arpHdr->ar_pln = 4;//IP length
	arpHdr->ar_op = htons(ARPOP_REQUEST);
	off = ETH_HLEN+arp_hdr_len;
	
	memcpy(&pARPData[off], sMac, ETH_ALEN);
	off += ETH_ALEN;
	
	memcpy(&pARPData[off], &(inAddr.s_addr), 4);
	off += 4;
	
	memset(&pARPData[off], 0, ETH_ALEN);
	off += ETH_ALEN;

	memcpy(&pARPData[off], &(dstAddr.s_addr), 4);

	return 0;
}

int get_ifindex(char *dev)   
{   
    int s;     
    struct ifreq req;    
    int err;    
    strcpy(req.ifr_name, dev);    
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)   
    {   
        printf("[%s %d]:socket() error.\n", __func__, __LINE__);   
        exit(1);   
    }   
    err = ioctl(s, SIOCGIFINDEX, &req);    
    close(s);  
    if (err == -1)    
    {
    	printf("[%s %d]:ioctl error.\n", __func__, __LINE__);   
        exit(1);    
    }
	
    return req.ifr_ifindex;   
} 

int main(int argc,char *argv[]) 
{
	int arp_socket = -1;
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	struct sockaddr_ll	etheraddr;
	int interval = 0;
    pid_t arpsendPid = getpid();
    cpu_set_t cpuMask;
    unsigned long len = sizeof (cpuMask);
	int cpuid = 0;
	char ifname[16] = {0};

	if(argc < 3){
		printf("usage: arpsend [destination IP] [interval] {cpuid} {deubg(1/0)}\n");
		return 0;
	}

	interval = atoi(argv[2]);

	dstAddr.s_addr = inet_addr(argv[1]);

	if(argc >=4){
		cpuid = atoi(argv[3]);
	}
	if(argc == 5){
		debug = atoi(argv[4]);
	}

	printf("sendarp to %s and interval %s seconds on cpu %d with debug %d\n", argv[1], argv[2], cpuid, debug);
    CPU_ZERO(&cpuMask);
    CPU_SET(cpuid, &cpuMask);
    if(sched_setaffinity(arpsendPid, len, &cpuMask) == -1) 
    {
        printf ("Failed to set arpsend to cpu %d\n", cpuid);
    }
	
	/* Create sockaddr_ll( link layer ) struct */  
	memset(&etheraddr, 0, sizeof(etheraddr));
	etheraddr.sll_family = AF_PACKET;
	etheraddr.sll_protocol = htons(ETH_P_ARP);
	etheraddr.sll_hatype	= ARPHRD_ETHER;
	etheraddr.sll_pkttype = PACKET_BROADCAST;
	etheraddr.sll_hatype	= ETH_ALEN;

	/* getInternetWAN name */

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
  			printf("Get chain record error!\n");
			return -1;
		}

		if (Entry.enable == 0)
			continue;

		if(Entry.applicationtype&X_CT_SRV_INTERNET){
			ifGetName(Entry.ifIndex,ifname,sizeof(ifname));
			printf("Internet WAN name is %s\n", ifname);
			break;
		}
	}

	if(ifname[0] == 0){
		printf("do not exist Internet WAN, exit!\n");
		return 0;
	}
	
	etheraddr.sll_ifindex = get_ifindex(ifname);
	printf("sll_ifindex is %d\n", etheraddr.sll_ifindex);

	/* Create pf_packet socket  */
	arp_socket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if (arp_socket < 0)
	{
    	printf("[%s %d]:open socket error\n", __FUNCTION__, __LINE__);
   	 	return -1;
	}
	 /* bind to socket */    
	if (bind(arp_socket, (struct sockaddr*)&etheraddr, sizeof(etheraddr)))   
	{
    	printf("[%s %d]:bind socket error\n", __FUNCTION__, __LINE__);
    	return -1;
		}

	ARPRequestCreate(ifname);

	while(1){
		if(debug){
			printf("send arp pkt:\n");
			dump_buffer(arpRequestPacket, ARP_PKT_LEN);
		}
		sendto(arp_socket, (void *)arpRequestPacket, ARP_PKT_LEN, MSG_DONTWAIT, (struct sockaddr *)&etheraddr, sizeof(etheraddr));
		sleep(interval);
	}

	close(arp_socket);
	arp_socket = -1;
	return 0;
}

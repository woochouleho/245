#include "informgetdata.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <memory.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/file.h>
#include <arpa/inet.h>

#include "utility.h"
#include "mib.h"

#ifdef CONFIG_CU_BASEON_CMCC
#include <rtk/accessservices.h>
#endif
#include <rtk/subr_cu.h>
#include <rtk/subr_net.h>

#ifdef CONFIG_RTK_L34_ENABLE
#include <rtk_rg_liteRomeDriver.h>
#elif defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_switch.h>
#include <rtk/rt/rt_port.h>
#endif
#include <sys/sysinfo.h>

#if defined(CONFIG_RTL9607C_SERIES) && defined(CONFIG_RTK_L34_ENABLE)
#include <rtk_rg_struct.h>
//default value for 9603CVD before INIT_RG_PORT_DEFINE(chipId)
unsigned int rtk_rg_port_pon_value              = 4;
unsigned int rtk_rg_port_cpu_value              = 5;
unsigned int rtk_rg_ext_port0_value             = 6;
unsigned int rtk_rg_ext_port1_value             = 7;
unsigned int rtk_rg_ext_port2_value             = 8;
unsigned int rtk_rg_ext_port3_value             = 9;
unsigned int rtk_rg_ext_port4_value             = 10;
unsigned int rtk_rg_ext_port5_value             = 11;
unsigned int rtk_rg_port_max_value             = 12;
unsigned int rtk_rg_mac_port_cpu_value     = 5;
unsigned int rtk_rg_mac_port_max_value    = 6;
unsigned int rtk_rg_all_cpu_portmask_value                                = 0x0;
unsigned int rtk_rg_all_mac_cpu_portmask_value                       = 0x0;
unsigned int rtk_rg_all_mac_portmask_without_cpu_value        = 0x0;
unsigned int rtk_rg_all_master_cpu_portmask_value          = 0x0;
unsigned int rtk_rg_all_mac_master_cpu_portmask_value = 0x0;
unsigned int rtk_rg_all_mac_slave_cpu_portmask_value    = 0x0;
unsigned int rtk_rg_all_ext_portmask_value                                = 0x0;
unsigned int rtk_rg_all_master_ext_portmask_value           = 0x0;
unsigned int rtk_rg_all_slave_ext_portmask_value              = 0x0;
unsigned int rtk_rg_all_lan_portmask_value                                = 0x0;
unsigned int rtk_rg_port_maincpu_value              = 5;
unsigned int rtk_rg_port_lastcpu_value                = 5;
unsigned int rtk_rg_mac_port_maincpu_value     = 5;
unsigned int rtk_rg_mac_port_lastcpu_value       = 5;
#endif

#define CONFIG_PIDFILE "/var/run/informgetd.pid"
static int attachDeviceInfoStart = 0;
static DeviceInfo *device_info = NULL;
static StaInfo *sta_info = NULL;
static unsigned int *flag = NULL;
static struct timeval querytv;
static int device_shm_id = -1;
static int station_shm_id = -1;
static StaInfo *sta_info_tmp = NULL;
static void *shm = NULL;

#define INFORMGETD_DEBUG
#ifdef INFORMGETD_DEBUG
int debug_flag;
#define DBG(x) if(debug_flag) x
#else
#define DBG(x)
#endif

typedef struct ponSpeed_info{
	struct  timeval    pretv;
	unsigned long long sendCnt;   /* save upload counter last time */
	unsigned long long recvCnt; /* save download counter last time */
	unsigned long long sendSpeed;   /* in unit of bps */
	unsigned long long recvSpeed; /* in unit of bps */
}PON_SPEED_INFO;
PON_SPEED_INFO ponSpeedInfo;

static unsigned char *devTypeString[10] = {
	"OTHER",
	"phone",
	"PC",
	"Pad",
	"STB",
	"PLC",
	"AP",
	"ROUTER",
	"SMTDEV",
	"CloudVR"	
};

#ifdef WLAN_SUPPORT
typedef enum { WLAN_BAND_11B=1, WLAN_BAND_11G=2, WLAN_BAND_11A=4, WLAN_BAND_11N=8, WLAN_BAND_5G_11N=16,
	WLAN_BAND_5G_11AC=64, WLAN_BAND_11AX=128} WLAN_BAND_TYPE_T;

#if 0
int getBandTypeStr(unsigned char bandType, char *bandTypeStr)
{
	int ret=0;
	
	if (bandType==BAND_11B)
		strcpy(bandTypeStr, "802.11b");
	else if (bandType==BAND_11G)
		strcpy(bandTypeStr, "802.11g");
	else if (bandType==BAND_11BG)
		strcpy(bandTypeStr, "802.11b/g");
	else if (bandType==BAND_11A)
		strcpy(bandTypeStr, "802.11a");
	else if (bandType==BAND_11N)
		strcpy(bandTypeStr, "802.11n");
	else if (bandType==BAND_5G_11AN)
		strcpy(bandTypeStr, "802.11a/n");
	else if(bandType==BAND_5G_11AC)
		strcpy(bandTypeStr, "802.11ac");
	else if(bandType==BAND_5G_11AAC)
		strcpy(bandTypeStr, "802.11a/ac");
	else if(bandType==BAND_5G_11NAC)
		strcpy(bandTypeStr, "802.11n/ac");
	else if(bandType==BAND_5G_11ANAC)
		strcpy(bandTypeStr, "802.11a/n/ac");
	else if(bandType==(BAND_11BG|BAND_11N))
		strcpy(bandTypeStr, "802.11b/g/n");
	else if(bandType==BAND_11AX)
		strcpy(bandTypeStr, "802.11ax");
	else if(bandType==(BAND_11AX|BAND_11BG|BAND_11N))
		strcpy(bandTypeStr, "802.11ax/b/g/n");
	else
	{
		strcpy(bandTypeStr, "");
		ret=-1;
	}
	return ret;
}
#else
void getBandTypeStr(WLAN_STA_INFO_Tp pInfo, char *bandTypeStr)
{
#ifdef WLAN_11AX
#ifdef WIFI5_WIFI6_COMP
	if(pInfo->network& BAND_5G_11AX)
		sprintf(bandTypeStr, "%s", ("802.11ax"));
	else if(pInfo->network& BAND_5G_11AC)
		sprintf(bandTypeStr, "%s", ("802.11ac"));
	else if (pInfo->network & BAND_11N)
		sprintf(bandTypeStr, "%s", ("802.11n"));
	else if (pInfo->network & BAND_11G)
		sprintf(bandTypeStr, "%s", ("802.11g"));
	else if (pInfo->network & BAND_11B)
		sprintf(bandTypeStr, "%s", ("802.11b"));
	else if (pInfo->network& BAND_11A)
		sprintf(bandTypeStr, "%s", ("802.11a"));
#else
	if(pInfo->wireless_mode& WLAN_MD_11AX)
		sprintf(bandTypeStr, "%s", ("802.11ax"));
	else if(pInfo->wireless_mode& WLAN_MD_11AC)
		sprintf(bandTypeStr, "%s", ("802.11ac"));
	else if(pInfo->wireless_mode& WLAN_MD_11N)
		sprintf(bandTypeStr, "%s", ("802.11n"));
	else if(pInfo->wireless_mode& WLAN_MD_11G)
		sprintf(bandTypeStr, "%s", ("802.11g"));
	else if(pInfo->wireless_mode& WLAN_MD_11A)
		sprintf(bandTypeStr, "%s", ("802.11a"));
	else if(pInfo->wireless_mode& WLAN_MD_11B)
		sprintf(bandTypeStr, "%s", ("802.11b"));
#endif
#else
	if(pInfo->network& BAND_5G_11AC)
		sprintf(bandTypeStr, "%s", ("802.11ac"));
	else if (pInfo->network & BAND_11N)
		sprintf(bandTypeStr, "%s", ("802.11n"));
	else if (pInfo->network & BAND_11G)
		sprintf(bandTypeStr,"%s",	("802.11g"));
	else if (pInfo->network & BAND_11B)
		sprintf(bandTypeStr, "%s", ("802.11b"));
	else if (pInfo->network& BAND_11A)
		sprintf(bandTypeStr, "%s", ("802.11a"));
#endif
	else
		sprintf(bandTypeStr, "%s", (""));
}
#endif

static int tranRate(WLAN_STA_INFO_T *pInfo)
{
	char txrate[16];
	rtk_wlan_get_sta_rate(pInfo, WLAN_STA_TX_RATE, txrate, 16);
	return atof(txrate) * (double)1000;	//convert to kbps
}
#endif
int findPPPUsername(char *pppoe)
{
	char buff[256];
	FILE *fp;
	char strif[6];
	char username[64];
	int found=0;

	memset(pppoe, 0, MAX_PPP_NAME_LEN);
	if (!(fp=fopen(PPP_CONF, "r"))) {
		//printf("%s not exists.\n", PPP_CONF);
	}
	else {
		fgets(buff, sizeof(buff), fp);
		while( fgets(buff, sizeof(buff), fp) != NULL ) {
			if(sscanf(buff, "%s %*s %*s %*d %*s %s", strif, username)!=2) {
				found=0;
				printf("Unsuported ppp configuration format\n");
				break;
			}
			//printf("%s %d username=%s\n", __FUNCTION__, __LINE__, username);
			strcpy(pppoe, username);
			found = 1;
			break;

		}
		fclose(fp);
	}
	return found;
}

static int getStaInfo()
{
	lanHostInfo_t *pLanNetInfo=NULL;
	unsigned int count = 0;
	int ret = -1, idx, i;
	struct in_addr lanIP;
	//struct timeval tv;
	//unsigned long time; 
	memset(sta_info, 0, sizeof(StaInfo)*64);
	ret = get_lan_net_info(&pLanNetInfo, &count);
	if(ret<0)
		goto end;
	
	if(count > 64)
		count = 64;
	DBG(printf("%s %d count=%d stainfo size=%d\n", __FUNCTION__, __LINE__, count, sizeof(StaInfo)));
	for(idx = 0; idx < count; idx++)
	{
		
		//gettimeofday(&tv, NULL);
		//time = tv.tv_sec*1000 + tv.tv_usec/1000;
		//printf("%s %d time=%lu\n",__FUNCTION__, __LINE__, time);
		//DBG(printf("%s %d &sta_info[%d]=0x%x\n", __FUNCTION__, __LINE__, idx, &sta_info[idx]));
		
		lanIP.s_addr = pLanNetInfo[idx].ip;
		sta_info[idx].StaOnlineTime = pLanNetInfo[idx].onLineTime;
		strncpy(sta_info[idx].szStaName, pLanNetInfo[idx].devName, MAX_LANNET_DEV_NAME_LENGTH);
		if(pLanNetInfo[idx].devType < 10 && pLanNetInfo[idx].devType >= 0)
			strcpy(sta_info[idx].pszStaType, devTypeString[pLanNetInfo[idx].devType]);
		else
			strcpy(sta_info[idx].pszStaType, "Unknown");
		memcpy(sta_info[idx].szStaMac, pLanNetInfo[idx].mac, MAC_ADDR_LEN);
		strcpy(sta_info[idx].szStaIP, inet_ntoa(lanIP));
		strncpy(sta_info[idx].szStaBrand, pLanNetInfo[idx].brand, MAX_LANNET_BRAND_NAME_LENGTH);
		strncpy(sta_info[idx].szStaOS, pLanNetInfo[idx].os, MAX_LANNET_MODEL_NAME_LENGTH);
		sta_info[idx].connectionType = pLanNetInfo[idx].connectionType;

		DBG(printf("%s %d pLanNetInfo[%d].rxBytes=%llu pLanNetInfo[%d].txBytes=%llu\n", 
			__FUNCTION__, __LINE__, idx, pLanNetInfo[idx].rxBytes, idx, pLanNetInfo[idx].txBytes));
		sta_info[idx].StaRecvTraffic = pLanNetInfo[idx].rxBytes;
		sta_info[idx].StaSendTraffic = pLanNetInfo[idx].txBytes;

		DBG(printf("%s %d sta_info[%d].szStaMac=%02x:%02x:%02x:%02x:%02x:%02x szStaIP=%s connectionType=%d szStaBrand=%s szStaOS=%s szStaName=%s devType=%s StaOnlineTime=%d\n", 
			__FUNCTION__, __LINE__, idx, sta_info[idx].szStaMac[0], sta_info[idx].szStaMac[1], sta_info[idx].szStaMac[2]
			, sta_info[idx].szStaMac[3], sta_info[idx].szStaMac[4], sta_info[idx].szStaMac[5], sta_info[idx].szStaIP, sta_info[idx].connectionType
			, sta_info[idx].szStaBrand, sta_info[idx].szStaOS, sta_info[idx].szStaName, sta_info[idx].pszStaType, sta_info[idx].StaOnlineTime));

#ifdef WLAN_SUPPORT
		if(pLanNetInfo[idx].connectionType == 1)//wireless
		{
			int x,y,i,j;
			char *buf=NULL;
			char ifname[16];
			int wlanIndex=0;
			WLAN_STA_INFO_Tp stainfo=NULL;
			//MIB_CE_MBSSIB_T entry;
			buf = malloc(sizeof(WLAN_STA_INFO_T)*(MAX_STA_NUM + 1));

			for(x=0; x<NUM_WLAN_INTERFACE; x++)
			{
				for(y=0; y<=WLAN_MBSSID_NUM; y++)
				{
					memset(buf, 0, sizeof(WLAN_STA_INFO_T)*(MAX_STA_NUM + 1));
					rtk_wlan_get_ifname(x, y, ifname);
					if(getWlStaInfo(ifname, (WLAN_STA_INFO_Tp)buf)<0)
						continue;
					for(i=1; i<=MAX_STA_NUM; i++)
					{
						stainfo = (WLAN_STA_INFO_Tp) &buf[i * sizeof(WLAN_STA_INFO_T)];
						if(memcmp(pLanNetInfo[idx].mac, stainfo->addr, 6) == 0)
						{
							sta_info[idx].StaWIFIIntensity = (unsigned int)stainfo->rssi - 100;
							if(x == 0)
							{
								wlanIndex = 0;
								sprintf(sta_info[idx].szRadioType, "2.4G");
								//mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIndex, y, (void *)&entry);
							}
							else
							{
								wlanIndex = 1;
								sprintf(sta_info[idx].szRadioType, "5G");
								//mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIndex, y, (void *)&entry);
							}
#if 0
							getBandTypeStr(entry.wlanBand, sta_info[idx].szWifiProtocol);
#else
							getBandTypeStr(stainfo, sta_info[idx].szWifiProtocol);
#endif
							sta_info[idx].StaSpeed = tranRate(stainfo);
							DBG(printf("%s %d szRadioType=%s szWifiProtocol=%s StaSpeed=%d\n", __FUNCTION__, __LINE__, 
								sta_info[idx].szRadioType, sta_info[idx].szWifiProtocol, sta_info[idx].StaSpeed));
							break;
						}

					}
				}
			}
			if(buf)
				free(buf);
			
		}
		else
#endif
		{
#ifdef CONFIG_RTK_L34_ENABLE
			rtk_rg_portStatusInfo_t portInfo[ELANVIF_NUM];
			memset(portInfo, 0x0, sizeof(rtk_rg_portStatusInfo_t) *ELANVIF_NUM);
			for (i = 0; i < ELANVIF_NUM; i++) 
			{
				if((pLanNetInfo[idx].port-1) == i)
				{
					int ret = RG_get_phyPort_status(i, &(portInfo[i]));
					if(ret != 1) // get fail
					{
						printf("%s get port %d status failed!\n", __FUNCTION__, i);
						break;
					}

					int speed = 0;
					switch(portInfo[i].linkSpeed)
					{
						case RTK_RG_PORT_SPEED_1000M:
							speed = 1000*1000;
							break;
						case RTK_RG_PORT_SPEED_100M:
							speed = 100*1000;
							break;
						case RTK_RG_PORT_SPEED_10M:
							speed = 10*1000;
							break;
						default:
							speed = 100*1000;
							break;
					}
					sta_info[idx].StaSpeed = speed;
					DBG(printf("%s %d StaSpeed=%d\n", __FUNCTION__, __LINE__, sta_info[idx].StaSpeed));
				}
				
			}
#elif defined(CONFIG_COMMON_RT_API)
			//printf("%s %d\n", __FUNCTION__, __LINE__);
			rt_port_linkStatus_t pLinkStatus;
			int speed = 100*1000;
			memset(&pLinkStatus, 0, sizeof(rt_port_linkStatus_t));
			for (i = 0; i < ELANVIF_NUM; i++) 
			{
				if((pLanNetInfo[idx].port-1) == i)
				{
					if(0 == rt_port_link_get(rtk_port_get_lan_phyID(i), &pLinkStatus))
					{
						if(pLinkStatus == RT_PORT_LINKUP)
						{	
							rt_port_speed_t  pSpeed;
							rt_port_duplex_t pDuplex;
							rt_port_speedDuplex_get(rtk_port_get_lan_phyID(i),&pSpeed,&pDuplex);
		
							switch(pSpeed)
							{
								case RT_PORT_SPEED_1000M:
									speed = 1000*1000;
									break;
								case RT_PORT_SPEED_100M:
									speed = 100*1000;
									break;
								case RT_PORT_SPEED_10M:
									speed = 10*1000;
									break;
								default:
									speed = 100*1000;
									break;
							}
							
						}
					}
					sta_info[idx].StaSpeed = speed;
					DBG(printf("%s %d StaSpeed=%d\n", __FUNCTION__, __LINE__, sta_info[idx].StaSpeed));
					break;
				}
			}
#endif			
		}

		sta_info[idx].status = 1;
		
#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL
		int totalNum = 0;
		int i;
		MIB_LAN_HOST_BANDWIDTH_T entry;
		totalNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);
		for(i = 0; i < totalNum; i++)
		{
			mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, i, (void *)&entry);
			if(0 == memcmp(entry.mac, pLanNetInfo[idx].mac, 6))
			{
				sta_info[idx].usBandwidth = entry.maxUsBandwidth;
				sta_info[idx].dsBandwidth = entry.maxDsBandwidth;
#ifdef CONFIG_USER_LAN_BANDWIDTH_EX_CONTROL
				sta_info[idx].usGuaranteeBandwidth = entry.minUsBandwidth;
				sta_info[idx].dsGuaranteeBandwidth = entry.minDsBandwidth;
#endif
			}
		}
	
#endif
		
		sta_info[idx].upSpeed = pLanNetInfo[idx].upRate;
		sta_info[idx].downSpeed = pLanNetInfo[idx].downRate;
		
		//gettimeofday(&tv, NULL);
		//time = tv.tv_sec*1000 + tv.tv_usec/1000;
		//printf("%s %d time=%lu\n",__FUNCTION__, __LINE__, time);
	}

end:
	if(pLanNetInfo)
		free(pLanNetInfo);

	return count;	
}
	
int InformGetData()
{
	//struct timeval tv;
	//unsigned long time; 
	//gettimeofday(&tv, NULL);
	//time = tv.tv_sec*1000 + tv.tv_usec/1000;
	//printf("%s %d time=%lu\n",__FUNCTION__, __LINE__, time);

	findPPPUsername(device_info->pppoe);
	
	double txpower=0, rxpower=0, temp=0;
#ifdef CONFIG_E8B
	get_poninfo(4, &txpower);
	get_poninfo(5, &rxpower);
	get_poninfo(1, &temp);
#endif	

	device_info->ponTXPower = txpower;
	device_info->ponRXPower = rxpower;

	mib_get(MIB_ELAN_MAC_ADDR, (void *)device_info->szDeviceMac);

	struct sysinfo info;
	if (sysinfo(&info) == 0)
		device_info->onlineTime = info.uptime;

	device_info->sendSpeed = ponSpeedInfo.sendSpeed;
	device_info->recvSpeed = ponSpeedInfo.recvSpeed;
	device_info->sendTraffic = ponSpeedInfo.sendCnt;
	device_info->recvTraffic = ponSpeedInfo.recvCnt;
	
	mib_get(MIB_ELAN_MAC_ADDR, (void *)device_info->szDeviceLanMac);
	mib_get(MIB_ELAN_MAC_ADDR, (void *)device_info->szDeviceLanMac[1]);
	mib_get(MIB_ELAN_MAC_ADDR, (void *)device_info->szDeviceLanMac[2]);
	mib_get(MIB_ELAN_MAC_ADDR, (void *)device_info->szDeviceLanMac[3]);
	
	device_info->cpuRate = get_cpu_usage();
	device_info->memRate = get_mem_usage();

	device_info->ponTemperature = temp;
	device_info->StaInfoNum = getStaInfo();
	device_info->pStaInfos = sta_info;
	
	*flag = 1;
	
	//printf("%s %d SET *flag to 1\n", __FUNCTION__, __LINE__);
	//gettimeofday(&tv, NULL);
	//time = tv.tv_sec*1000 + tv.tv_usec/1000;
	//printf("%s %d time=%lu\n",__FUNCTION__, __LINE__, time);
	return 0;
}

#ifdef CONFIG_CU
void record_pon_speed_info(unsigned long long upSpeed,unsigned long long downSpeed)
{
	FILE *fp=NULL;
	char buf[64]={0};

	fp = fopen(PON_SPPED_RECORD, "w+");
	if(fp){
		sprintf(buf,"%llu %llu\n", upSpeed, downSpeed);
		fprintf(fp, buf);
		fclose(fp);
	}
}
#endif

void getAndCalcPonCurrBandwidth()
{
	int ret;
	struct	timeval  tv;
	uint64_t recv, send;
	unsigned int msecond;
	ret = gettimeofday(&tv,NULL);
	if(ret == -1)
	{
		printf("[%s %d]: exec gettimeofday failed.\n\n", __func__, __LINE__);
		return;
	}
		
	get_pon_traffic(&send, &recv);

	if((send <= ponSpeedInfo.sendCnt) || (recv <= ponSpeedInfo.recvCnt))
	{
		ponSpeedInfo.sendSpeed = 0;
		ponSpeedInfo.recvSpeed = 0;
	}
	else
	{	
		msecond = (tv.tv_sec - ponSpeedInfo.pretv.tv_sec) * 1000 + 
				((int)(tv.tv_usec) - (int)(ponSpeedInfo.pretv.tv_usec)) / 1000;
		ponSpeedInfo.sendSpeed = ((send - ponSpeedInfo.sendCnt) << 3) * 1000 / msecond;
		ponSpeedInfo.recvSpeed = ((recv - ponSpeedInfo.recvCnt) << 3) * 1000 / msecond;
	}
			
	ponSpeedInfo.sendCnt = send;
	ponSpeedInfo.recvCnt = recv;
	ponSpeedInfo.pretv.tv_sec = tv.tv_sec;
	ponSpeedInfo.pretv.tv_usec = tv.tv_usec;
	//DBG(printf("%s %d ponSpeedInfo.sendSpeed=%lu ponSpeedInfo.recvSpeed=%lu\n", __FUNCTION__, __LINE__, ponSpeedInfo.sendSpeed, ponSpeedInfo.recvSpeed));

#ifdef CONFIG_CU
	record_pon_speed_info(ponSpeedInfo.sendSpeed, ponSpeedInfo.recvSpeed);
#endif
}

#define INFORMGET_INFO "/var/run/informget.lock"
static int lock_shm_by_flock()
{
	int lockfd;
	
	if ((lockfd = open(INFORMGET_INFO, O_RDWR | O_CREAT)) == -1) {
		perror("open shm lockfile");
		return lockfd;
	}
	while (flock(lockfd, LOCK_EX | LOCK_NB) == -1 && errno==EINTR) {
		printf("configd write lock failed by flock. errno=%d\n", errno);
	}
	return lockfd;
}

static int unlock_shm_by_flock(int lockfd)
{
	while (flock(lockfd, LOCK_UN) == -1 && errno==EINTR) {
		printf("configd write unlock failed by flock. errno=%d\n", errno);
	}
	close(lockfd);
	return 0;
}

void signal_handler(int sig)
{
	//printf("get signal\n");
	if(attachDeviceInfoStart == 1)
		return;
	attachDeviceInfoStart = 1;
	return;
}

void doInformGetData()
{
	InformGetData();
	return;
}

static void log_pid()
{
	FILE *f;
	pid_t pid;
	char *pidfile = CONFIG_PIDFILE;

	pid = getpid();

	if((f = fopen(pidfile, "w")) == NULL)
		return;
	fprintf(f, "%d\n", pid);
	fclose(f);
	printf("log_pid pid=%d\n", pid);
}

int initDeviceInfoShm()
{
	printf("%s %d device_shm_id=%d\n", __FUNCTION__, __LINE__, device_shm_id);
	device_shm_id = shmget((key_t)0x9981, (sizeof(unsigned int)+sizeof(DeviceInfo)), 0644 | IPC_CREAT);
	if(device_shm_id == -1)
		return -1;
	
	shm = shmat(device_shm_id, NULL, 0);
	if(shm == (void*)(-1))
		return -1;
	flag = ((unsigned int *)shm);
	printf("flag address = %x\n", flag);
	device_info = (DeviceInfo *)((void*)shm + 4);
	printf("device_info address = %x\n", device_info);
	return 0;	
}

int initStaInfoShm()
{
	printf("%s %d station_shm_id=%d\n", __FUNCTION__, __LINE__, station_shm_id);
	station_shm_id = shmget((key_t)0x1899, sizeof(StaInfo)*64, 0644 | IPC_CREAT);
	if(station_shm_id == -1)
		return -1;
	
	sta_info = (StaInfo *)shmat(station_shm_id, NULL, 0);
	if(sta_info == (void*)(-1))
		return -1;
	printf("sta_info address = %x\n", sta_info);
	return 0;
}

void freeDeviceInfoShm()
{
	if(shmdt(shm) == -1)
	{
		printf("shmdt failed\n");
		return;
	}

	if(shmctl(device_shm_id,IPC_RMID,0) == -1)
	{
		printf("shmctl failed\n");
		return;
	}
	return;
}

void freeStaInfoShm()
{
	if(shmdt(sta_info) == -1)
	{
		printf("shmdt failed\n");
		return;
	}

	if(shmctl(station_shm_id,IPC_RMID,0) == -1)
	{
		printf("shmctl failed\n");
		return;
	}
	return;
}

void signal_terminal(int sig)
{
	int fd;
	printf("get terminal signal\n");
	fd = lock_shm_by_flock();
	freeDeviceInfoShm();
	freeStaInfoShm();
	unlock_shm_by_flock(fd);
	return;
}

void signal_alrm(int dummy)
{
#ifdef INFORMGETD_DEBUG
	if(access("/tmp/informgetd_debug", F_OK) == 0){
		debug_flag = 1;
	}else{
		debug_flag = 0;
	}
#endif
	alarm(1);
}

int main(void)
{
	struct	timeval  tv;
	int ret;
	int fd;
	initDeviceInfoShm();
	initStaInfoShm();
	log_pid();

#if defined(CONFIG_RTL9607C_SERIES) && defined(CONFIG_RTK_L34_ENABLE)
	unsigned int chipId;
	unsigned int rev;
	unsigned int subType;
	rtk_rg_switch_version_get(&chipId, &rev, &subType);
	INIT_RG_PORT_DEFINE(chipId);
#endif

	signal(SIGUSR1, signal_handler);
	signal(SIGTERM, signal_terminal);
	signal(SIGALRM, signal_alrm);
	alarm(1);

	gettimeofday(&querytv,NULL);

	get_cpu_usage();

	while(1)
	{
		ret = gettimeofday(&tv,NULL);
		if(ret)
		{
			printf("[%s %d]:gettimeofday exec failed, continue.\n", __func__, __LINE__);
			continue;
		}

		if(attachDeviceInfoStart == 1)
		{
			fd = lock_shm_by_flock();
			doInformGetData();
			unlock_shm_by_flock(fd);
			attachDeviceInfoStart = 0;
		}
		
		if(((tv.tv_sec - querytv.tv_sec) * 1000 + 
				((int)(tv.tv_usec) - (int)(querytv.tv_usec)) / 1000) > 500)
		{
			querytv.tv_sec = tv.tv_sec;
			querytv.tv_usec = tv.tv_usec;
			//DBG(printf("%s %d tv_sec=%d tv_usec=%d\n", __FUNCTION__, __LINE__, tv.tv_sec, tv.tv_usec));
			getAndCalcPonCurrBandwidth();
		}
		else if(((tv.tv_sec - querytv.tv_sec) * 1000 + 
			((int)(tv.tv_usec) - (int)(querytv.tv_usec)) / 1000) < 0)
		{
			querytv.tv_sec = tv.tv_sec;
			querytv.tv_usec = tv.tv_usec;
		}
		usleep(5000);//each 5ms exec a time
	}
	return 0;
}

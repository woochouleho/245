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
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/file.h>

#include "rtk_timer.h"
#include "mib.h"
#include "utility.h"
#include "debug.h"

#include "rtk/ponmac.h"
#include "rtk/gponv2.h"
#include "rtk/epon.h"
#include "hal/chipdef/chip.h"
#include "rtk/pon_led.h"
#include "rtk/stat.h"
#include "rtk/switch.h"

#include <common/util/rt_util.h>

#ifdef CONFIG_USER_RTK_VOIP
#include "web_voip.h"
#endif

#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_) || defined(_PRMT_X_CT_COM_WirelessTestDiagnostics_)
//#ifdef _PRMT_X_CT_COM_WirelessTestDiagnostics_
enum
{
	eWirelessTest_None=0,
	eWirelessTest_Requested,
	eWirelessTest_Completed,
	
	eWirelessTest_Error_InitConnectionFailed,
	eWirelessTest_Error_NoResponse,
	eWirelessTest_Error_TransferFailed,
	eWirelessTest_Error_Timeout,
	
	eWirelessTest_End /*last one*/
};
//#endif

int getAllWirelessChannelOnce(void)
{
	unsigned char res;
	int wait_time;
	int status;
	char tmpBuf[100];
	int bssdb_idx;
	static SS_STATUS_T Status={0};
	FILE *fp_tmp = NULL;
		
	// issue scan request
	wait_time = 0;
	while (1) 
	{
		if ( getWlSiteSurveyRequest(getWlanIfName(),  &status) < 0 ) 
		{
			printf("Site-survey request failed!");
			return -eWirelessTest_Error_NoResponse;
		}
		if (status != 0)
		{	
			if (wait_time++ > 5) 
			{
				printf("scan request timeout!");
				return -eWirelessTest_Error_Timeout;
			}
			sleep(1);
		}
		else
			break;
	}
	
		// wait until scan completely
	wait_time = 0;
	while (1) 
	{
		Status.number=0;
		if ( getWlSiteSurveyResult(getWlanIfName(), (SS_STATUS_Tp)&Status) < 0 ) 
		{
			strcpy(tmpBuf, "Read site-survey status failed!");
			return -eWirelessTest_Error_TransferFailed;
		}
		if (Status.number == 0xff) 
		{   
			if (wait_time++ > 10) 
			{
				printf("scan timeout!");
				return -eWirelessTest_Error_Timeout;
			}
			sleep(1);
		}
		else
			break;
	}
		
	if(!(fp_tmp = fopen("/var/wireless_neighbor_info", "w+")))
	{
		return -eWirelessTest_Error_TransferFailed;
	}
	
#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_AllWirelessChannelEnable_) || defined(_PRMT_X_CT_COM_WirelessTestDiagnostics_)
	int curChanIndex, chanIndex;
	unsigned int neighborNum[14] = {0};
	for(bssdb_idx=0 ; bssdb_idx<Status.number ; bssdb_idx++)
	{
		curChanIndex = Status.bssdb[bssdb_idx].channel;
		if(curChanIndex > 13)
		{
			continue;
		}
				
			neighborNum[curChanIndex]++;
	}
			
	for(chanIndex=1 ; chanIndex<14 ; chanIndex++)
	{
		if(neighborNum[chanIndex])
		{
			fprintf(fp_tmp, "%d:%d+", chanIndex, neighborNum[chanIndex]);
		}
	}
#endif
#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_BestWirelessChannelEnable_) || defined(_PRMT_X_CT_COM_WirelessTestDiagnostics_)
	fprintf(fp_tmp, "##%d+%d", Status.bssdb[0].channel, Status.bssdb[0].rssi-100);
#endif
	fclose(fp_tmp);			
	return 0;		
}

int get_WirelessChannelResult(unsigned char phyband, char *AllWirelessChannel, char *BestWirelessChannel)
{
	char tmpBuf[64] = {0}, band[8] = {0};
	FILE *fp_tmp = NULL;
	char *pBuf = NULL;
	int tmpLen = 0;

	if (phyband != PHYBAND_2G && phyband != PHYBAND_5G)
	{
		return -1;
	}

	if (!(fp_tmp = fopen("/var/wireless_neighbor_info", "r")))
	{
		printf("(%s) error: can't open wireless_neighbor_info!\n", __FUNCTION__);
		return -1;
	}

	sprintf(band, "%s:", (phyband == PHYBAND_2G) ? "2.4G":"5.0G");

	memset(tmpBuf, 0, sizeof(tmpBuf));
	while (fgets(tmpBuf, sizeof(tmpBuf), fp_tmp))
	{
		pBuf = strrchr(tmpBuf, '\n');
		if (pBuf)
		{
			*pBuf = '\0';
		}

		if (strstr(tmpBuf, band))
		{
			pBuf = strstr(tmpBuf, "##");

			if (pBuf)
			{
				if (*(pBuf - 1) == '+')
					tmpLen = pBuf - tmpBuf - 1;
				else
					tmpLen = pBuf - tmpBuf;

				if (AllWirelessChannel != NULL)
				{
					strncpy(AllWirelessChannel, tmpBuf, tmpLen);
					AllWirelessChannel[tmpLen] = '\0';
				}

				if (BestWirelessChannel != NULL)
				{
					sprintf(BestWirelessChannel, "%s%s", band, (pBuf + 2));
				}
			}
			else
			{
				if (AllWirelessChannel != NULL)
				{
					strncpy(AllWirelessChannel, tmpBuf, sizeof(tmpBuf));
					AllWirelessChannel[strlen(tmpBuf)] = '\0';
				}
			}

			break;
		}
	}
	fclose(fp_tmp);

	//fprintf(stderr, "(%s) AllWirelessChannel = [%s], BestWirelessChannel = [%s]\n", __FUNCTION__, AllWirelessChannel?AllWirelessChannel:"", BestWirelessChannel?BestWirelessChannel:"");
	return 0;
}

//
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterNumberEnable_
#define DEFAULT_DHCPC_REG_FILE "/var/run/udhcpc.reg"
#define DEFAULT_DHCPC_REG_LOCKFILE "/var/run/udhcpc.reg.lock"
int getDHCPRegisterNumber(){
	int DHCPRegisterNumber = 0;
	FILE *fp = NULL;
	char ch;
	int lockfd = 0;
	if ((lockfd = lock_file_by_flock(DEFAULT_DHCPC_REG_LOCKFILE, 0)) == -1)
	{
		printf("%s, the file %s have been locked\n", __FUNCTION__, DEFAULT_DHCPC_REG_FILE);
		return 0;
	}
	if(fp = fopen(DEFAULT_DHCPC_REG_FILE, "r")){
		if(fscanf(fp, "%d", &DHCPRegisterNumber)!=1){
			printf("can not read DHCPRegisterNumber from %s!!\n",DEFAULT_DHCPC_REG_FILE);
			DHCPRegisterNumber = 0;
		}
		fclose(fp);
	}
	else{ 
		printf("fail to open file %s\n",DEFAULT_DHCPC_REG_FILE);
		DHCPRegisterNumber = 0;
	}
	//printf("read DHCPRegisterNumber=%d\n",DHCPRegisterNumber);
end:
	unlock_file_by_flock(lockfd);
	return DHCPRegisterNumber;
}
void addDHCPRegisterNumber(){
	int DHCPRegisterNumber = 0;
	FILE *fp = NULL;
	char ch;
	int lockfd = 0;
	DHCPRegisterNumber = getDHCPRegisterNumber();
	if ((lockfd = lock_file_by_flock(DEFAULT_DHCPC_REG_LOCKFILE, 1)) == -1)
	{
		printf("%s, the file %s have been locked\n", __FUNCTION__, DEFAULT_DHCPC_REG_FILE);
		return;
	}
	if(fp = fopen(DEFAULT_DHCPC_REG_FILE, "w+")){
		DHCPRegisterNumber++;
		//printf("add DHCPRegisterNumber=%d\n",DHCPRegisterNumber);
		fprintf(fp, "%d", DHCPRegisterNumber);
		fclose(fp);
	}
	else{ 
		printf("fail to open file %s\n",DEFAULT_DHCPC_REG_FILE);
	}
end:
	unlock_file_by_flock(lockfd);
	return;
}
void clearDHCPRegisterNumber(){
	int DHCPRegisterNumber = 0;
	FILE *fp = NULL;
	char ch;
	int lockfd = 0;
	if ((lockfd = lock_file_by_flock(DEFAULT_DHCPC_REG_LOCKFILE, 1)) == -1)
	{
		printf("%s, the file %s have been locked\n", __FUNCTION__, DEFAULT_DHCPC_REG_FILE);
		return;
	}
	if(fp = fopen(DEFAULT_DHCPC_REG_FILE, "w+")){
		//printf("clearDHCPRegisterNumber!!\n");
		fprintf(fp, "0");
		fclose(fp);
	}
	else{ 
		printf("fail to open file %s\n",DEFAULT_DHCPC_REG_FILE);
	}
end:
	unlock_file_by_flock(lockfd);
	return;
}
#define DEFAULT_ITMS_REG_FILE "/var/run/itms.reg"
#define DEFAULT_ITMS_LOCK     "/var/run/itms.lock"
int getRegisterNumberITMS(){
	int ITMSRegisterNumber = 0,lockfd=0;
	FILE *fp=NULL;
	char ch;
    if ((lockfd = lock_file_by_flock(DEFAULT_ITMS_LOCK, 0)) == -1)
	{
		printf("%s, the file have been locked\n", __FUNCTION__);
		return -1;
	}
	if(fp = fopen(DEFAULT_ITMS_REG_FILE, "r")){
		if(fscanf(fp, "%d", &ITMSRegisterNumber)!=1){
			printf("can not readITMSRegisterNumber from %s!!\n",DEFAULT_ITMS_REG_FILE);
			ITMSRegisterNumber=0;
		}
		fclose(fp);
	}
	else{ 
		printf("fail to open file %s\n",DEFAULT_ITMS_REG_FILE);
		ITMSRegisterNumber = 0;
	}
    unlock_file_by_flock(lockfd);
	printf("read ITMSRegisterNumber=%d\n",ITMSRegisterNumber);
	return ITMSRegisterNumber;
}
int addRegisterNumberITMS(){
	int ITMSRegisterNumber = 0,lockfd=0;
	FILE *fp=NULL;
	char ch;
    
	ITMSRegisterNumber = getRegisterNumberITMS();
    if ((lockfd = lock_file_by_flock(DEFAULT_ITMS_LOCK, 0)) == -1)
	{
		printf("%s, the file have been locked\n", __FUNCTION__);
		return -1;
	}
	if(fp = fopen(DEFAULT_ITMS_REG_FILE, "w+")){
		ITMSRegisterNumber++;
		printf("add ITMSRegisterNumber=%d\n",ITMSRegisterNumber);
		fprintf(fp, "%d", ITMSRegisterNumber);
		fclose(fp);
        unlock_file_by_flock(lockfd);
		return 0;
	}
	else{ 
		printf("fail to open file %s\n",DEFAULT_ITMS_REG_FILE);
        unlock_file_by_flock(lockfd);
		return -1;
	}
}
int defRegisterNumberITMS(){
	int ITMSRegisterNumber = 0,lockfd=0;
	FILE *fp=NULL;
	char ch;
    if ((lockfd = lock_file_by_flock(DEFAULT_ITMS_LOCK, 0)) == -1)
	{
		printf("%s, the file have been locked\n", __FUNCTION__);
		return -1;
	}
	if(fp = fopen(DEFAULT_ITMS_REG_FILE, "w+")){
		printf("defITMSRegisterNumber!!\n");
		fprintf(fp, "0");
		fclose(fp);
        unlock_file_by_flock(lockfd);
		return 0;
	}
	else{ 
		printf("fail to open file %s\n",DEFAULT_ITMS_REG_FILE);
        unlock_file_by_flock(lockfd);
		return -1;
	}
}

#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterSuccessNumberEnable_
#define DEFAULT_DHCPC_SUCCESS_FILE "/var/run/udhcpc.success"
#define DEFAULT_DHCPC_SUCCESS_LOCKFILE "/var/run/udhcpc.success.lock"
int getDHCPSuccessNumber(){
	int DHCPSuccessNumber = 0;
	FILE *fp;
	char ch;
	int lockfd = 0;
	if ((lockfd = lock_file_by_flock(DEFAULT_DHCPC_SUCCESS_LOCKFILE, 0)) == -1)
	{
		printf("%s, the file %s have been locked\n", __FUNCTION__, DEFAULT_DHCPC_SUCCESS_FILE);
		return 0;
	}
	if(fp = fopen(DEFAULT_DHCPC_SUCCESS_FILE, "r")){
		if(fscanf(fp, "%d", &DHCPSuccessNumber)!=1){
			printf("can not read DHCPSuccessNumber from %s!!\n",DEFAULT_DHCPC_SUCCESS_FILE);
			DHCPSuccessNumber = 0;
		}
		//printf("DHCPSuccessNumber=%d\n",DHCPSuccessNumber);
		fclose(fp);
	}
	else{ 
		printf("fail to open file %s\n",DEFAULT_DHCPC_SUCCESS_FILE);
		DHCPSuccessNumber = 0;
	}
end:
	unlock_file_by_flock(lockfd);
	return DHCPSuccessNumber;
}
int addDHCPSuccessNumber(){
	int DHCPSuccessNumber = 0;
	FILE *fp;
	DHCPSuccessNumber = getDHCPSuccessNumber();
	int lockfd = 0;
	if ((lockfd = lock_file_by_flock(DEFAULT_DHCPC_SUCCESS_LOCKFILE, 1)) == -1)
	{
		printf("%s, the file %s have been locked\n", __FUNCTION__, DEFAULT_DHCPC_SUCCESS_FILE);
		return -1;
	}
	if(fp = fopen(DEFAULT_DHCPC_SUCCESS_FILE, "w+")){
		DHCPSuccessNumber++;
		fprintf(fp, "%d", DHCPSuccessNumber);
		fclose(fp);
	}
	else{ 
		printf("fail to open file %s\n",DEFAULT_DHCPC_SUCCESS_FILE);
	}
end:
	unlock_file_by_flock(lockfd);
	return -1;
}
int clearDHCPSuccessNumber(){
	FILE *fp;
	int lockfd = 0;
	if ((lockfd = lock_file_by_flock(DEFAULT_DHCPC_SUCCESS_LOCKFILE, 1)) == -1)
	{
		printf("%s, the file %s have been locked\n", __FUNCTION__, DEFAULT_DHCPC_SUCCESS_FILE);
		return -1;
	}
	if(fp = fopen(DEFAULT_DHCPC_SUCCESS_FILE, "w+")){
		printf("clearDHCPSuccessNumber\n");
		fprintf(fp, "0");
		fclose(fp);
	}
	else{ 
		printf("fail to open file %s\n",DEFAULT_DHCPC_SUCCESS_FILE);
	}
end:
	unlock_file_by_flock(lockfd);
	return -1;
}

#define DEFAULT_ITMS_SUCCESS_FILE "/var/run/itms.success"
#define DEFAULT_ITMS_SUCCESS_LOCK "/var/run/itmssucc.lock"
int getRegisterSuccNumITMS(){
	int ITMSSuccessNumber = 0,lockfd=0;
	FILE *fp=NULL;
	char ch;
    if ((lockfd = lock_file_by_flock(DEFAULT_ITMS_SUCCESS_LOCK, 0)) == -1)
	{
		printf("%s, the file have been locked\n", __FUNCTION__);
		return -1;
	}
	if(fp = fopen(DEFAULT_ITMS_SUCCESS_FILE, "r")){
		if(fscanf(fp, "%d", &ITMSSuccessNumber)!=1){
			printf("can not read ITMSSuccessNumber from %s!!\n",DEFAULT_ITMS_SUCCESS_FILE);
			ITMSSuccessNumber = 0;
		}
		printf("ITMSSuccessNumber=%d\n",ITMSSuccessNumber);
		fclose(fp);
	}
	else{ 
		printf("fail to open file %s\n",DEFAULT_ITMS_SUCCESS_FILE);
		ITMSSuccessNumber = 0;
	}
    unlock_file_by_flock(lockfd);
	return ITMSSuccessNumber;
}
int addRegisterSuccNumITMS(){
	int ITMSSuccessNumber = 0,lockfd=0;
	FILE *fp=NULL;
	ITMSSuccessNumber = getRegisterSuccNumITMS();
    if ((lockfd = lock_file_by_flock(DEFAULT_ITMS_SUCCESS_LOCK, 0)) == -1)
	{
		printf("%s, the file have been locked\n", __FUNCTION__);
		return -1;
	}
	if(fp = fopen(DEFAULT_ITMS_SUCCESS_FILE, "w+")){
		ITMSSuccessNumber++;
		fprintf(fp, "%d", ITMSSuccessNumber);
		fclose(fp);
        unlock_file_by_flock(lockfd);
		return 0;
	}
	else{ 
		printf("fail to open file %s\n",DEFAULT_ITMS_REG_FILE);
        unlock_file_by_flock(lockfd);
		return -1;
	}
}
int defRegisterSuccNumITMS(){
	FILE *fp=NULL;
    int lockfd=0;
    if ((lockfd = lock_file_by_flock(DEFAULT_ITMS_SUCCESS_LOCK, 0)) == -1)
	{
		printf("%s, the file have been locked\n", __FUNCTION__);
		return -1;
	}
	if(fp = fopen(DEFAULT_ITMS_SUCCESS_FILE, "w+")){
		printf("defITMSSuccessNumber\n");
		fprintf(fp, "0");
		fclose(fp);
        unlock_file_by_flock(lockfd);
		return 0;
	}
	else{ 
		printf("fail to open file %s\n",DEFAULT_ITMS_REG_FILE);
        unlock_file_by_flock(lockfd);
		return -1;
	}
}
#endif


#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_SEREnable_
static unsigned long long pon_counter_ECPL_old[4];
static unsigned long long pon_counter_total_old[4];

enum {PON_COUNTER_SER=0, PON_COUNTER_PLR=1, PON_COUNTER_PacketLost=2, PON_COUNTER_ErrorCode=3,PON_COUNTER_MAX};
void reset_pon_stat_counters(){
	int i=0;
	for(i=0;i<PON_COUNTER_MAX;i++){
		pon_counter_total_old[i]=0;
		pon_counter_ECPL_old[i]=0;
	}
}
unsigned int get_pon_stat_counters(rtk_stat_port_cntr_t *counters)
{
	unsigned int port_pon_idx = 1;
	rtk_switch_phyPortId_get(RTK_PORT_PON, &port_pon_idx);

	rtk_stat_port_getAll(port_pon_idx, counters);

	return counters->etherStatsFragments+counters->etherStatsJabbers+counters->etherStatsRxOversizePkts+
		counters->etherStatsRxPkts64Octets+counters->etherStatsRxPkts512to1023Octets+counters->etherStatsRxPkts65to127Octets+
		counters->etherStatsRxPkts256to511Octets+counters->etherStatsRxPkts1519toMaxOctets+counters->etherStatsRxPkts128to255Octets+
		counters->etherStatsRxPkts1024to1518Octets;
}

void start_record_ser_errorCode(int flag)
{
	rtk_stat_port_cntr_t counters;
	memset(&counters, 0, sizeof(counters));
	switch(flag)
	{
		case PON_COUNTER_SER:
			pon_counter_total_old[PON_COUNTER_SER] = get_pon_stat_counters(&counters);
			pon_counter_ECPL_old[PON_COUNTER_SER] = counters.dot3StatsSymbolErrors;
			break;
		case PON_COUNTER_ErrorCode:
			pon_counter_total_old[PON_COUNTER_ErrorCode] = get_pon_stat_counters(&counters);
			pon_counter_ECPL_old[PON_COUNTER_ErrorCode] = counters.dot3StatsSymbolErrors;
			break;
		case PON_COUNTER_PLR:
			pon_counter_total_old[PON_COUNTER_PLR] = get_pon_stat_counters(&counters);
			pon_counter_ECPL_old[PON_COUNTER_PLR] = counters.dot1dTpPortInDiscards;
			break;
		case PON_COUNTER_PacketLost:
			pon_counter_total_old[PON_COUNTER_PacketLost] = get_pon_stat_counters(&counters);
			pon_counter_ECPL_old[PON_COUNTER_PacketLost] = counters.dot1dTpPortInDiscards;
			break;
		default:
			break;
	}
	
}

unsigned int getSER(int clear)
{
	rtk_stat_port_cntr_t counters;
	memset(&counters, 0, sizeof(counters));
	unsigned int total = get_pon_stat_counters(&counters);
	unsigned int ser = 0;
	if(total){
		if(total<=pon_counter_total_old[PON_COUNTER_SER]){
			reset_pon_stat_counters();
		}
		ser = (counters.dot3StatsSymbolErrors-pon_counter_ECPL_old[PON_COUNTER_SER])* 100 / (total-pon_counter_total_old[PON_COUNTER_SER]);
	}else
		ser = 0;	
	//printf("SER  %d  %d\n",ECPL_old[SER], total_old[SER]);
	if(clear)
		start_record_ser_errorCode(PON_COUNTER_SER);
	//printf("SER  %d  %d\n",ECPL_old[SER], total_old[SER]);
	return ser;
}
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_ErrorCodeEnable_
unsigned int getErrorCode(int clear)
{
	rtk_stat_port_cntr_t counters;
	memset(&counters, 0, sizeof(counters));
	unsigned int total = get_pon_stat_counters(&counters);
	if(total<=pon_counter_total_old[PON_COUNTER_ErrorCode]){
			reset_pon_stat_counters();
	}
	unsigned int error_code = counters.dot3StatsSymbolErrors - pon_counter_ECPL_old[PON_COUNTER_ErrorCode];
	//printf("ERRORCODE  %d  %d\n",ECPL_old[ErrorCode], total_old[ErrorCode]);
	if(clear)
		start_record_ser_errorCode(PON_COUNTER_ErrorCode);
	//printf("ERRORCODE  %d  %d\n",ECPL_old[ErrorCode], total_old[ErrorCode]);
	return error_code;
}
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_PLREnable_
unsigned int getPLR(int clear)
{
	rtk_stat_port_cntr_t counters;
	memset(&counters, 0, sizeof(counters));
	unsigned int total = get_pon_stat_counters(&counters);
	unsigned int plr = 0;
	if(total){
		if(total<=pon_counter_total_old[PON_COUNTER_PLR]){
			reset_pon_stat_counters();
		}
		plr=(counters.dot1dTpPortInDiscards-pon_counter_ECPL_old[PON_COUNTER_PLR]) * 100 / (total-pon_counter_total_old[PON_COUNTER_PLR]);
	}else
		plr = 0;
	//printf("PLR  %d  %d\n",ECPL_old[PLR], total_old[PLR]);
	if(clear)
		start_record_ser_errorCode(PON_COUNTER_PLR);
	//printf("PLR  %d  %d\n",ECPL_old[PLR], total_old[PLR]);
	return plr;
}
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_PacketLostEnable_
unsigned int getPacketLost(int clear)
{
	rtk_stat_port_cntr_t counters;
	memset(&counters, 0, sizeof(counters));
	unsigned int total = get_pon_stat_counters(&counters);
	if(total<=pon_counter_total_old[PON_COUNTER_PacketLost]){
			reset_pon_stat_counters();
	}
	unsigned int packet_lost = counters.dot1dTpPortInDiscards - pon_counter_ECPL_old[PON_COUNTER_PacketLost];
	//printf("PacketLost  %d  %d\n",ECPL_old[PacketLost], total_old[PacketLost]);
	if(clear)
		start_record_ser_errorCode(PON_COUNTER_PacketLost);
	//printf("PacketLost  %d  %d\n",ECPL_old[PacketLost], total_old[PacketLost]);
	return packet_lost;
}
#endif

#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_AllWirelessChannelEnable_) || defined(_PRMT_X_CT_COM_WirelessTestDiagnostics_)
int getWlan0AllChannel(char *allChannelBuf, int size)
{
	if(NULL == allChannelBuf)
	{
		printf("%s,error: parameter is NULL!\n", __FUNCTION__);
		return -1;
	}

	if(size < 64)
	{
		printf("%s,error: buffer should larger than 64!\n", __FUNCTION__);
		return -1;
	}

	char tmpBuf[64];
	char *pBuf = NULL;
	FILE *fp_tmp = NULL;
	int tmpLen = 0;
	
	if(!(fp_tmp = fopen("/var/wireless_neighbor_info", "r+")))
	{		
		printf("%s,error: can't open wireless_neighbor_info!\n", __FUNCTION__);
		return -1;
	}

	memset(tmpBuf, 0, sizeof(tmpBuf));
	fgets(tmpBuf, sizeof(tmpBuf), fp_tmp);
	pBuf = tmpBuf;
	pBuf = strstr(tmpBuf, "##");
	
	if(pBuf)
	{
		tmpLen = pBuf - tmpBuf - 1;
		strncpy(allChannelBuf, tmpBuf, tmpLen);
		allChannelBuf[tmpLen] = '\0';
		
	}
	else
	{
		strncpy(allChannelBuf, tmpBuf, sizeof(tmpBuf));
	}

	fprintf(stderr, "%s %d allChannelBuf=%s!\n",__FUNCTION__,__LINE__,allChannelBuf);
	fclose(fp_tmp);
	return 0;
}
#endif

#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_BestWirelessChannelEnable_) || defined(_PRMT_X_CT_COM_WirelessTestDiagnostics_)
int getWlan0BestChannel(char* bestChannelBuf, int size)
{
	if(NULL == bestChannelBuf)
	{
		printf("%s,error: parameter is NULL!\n", __FUNCTION__);
		return -1;
	}

	if(size < 8)
	{
		printf("%s,error: buffer should larger than 8!\n", __FUNCTION__);
		return -1;
	}

	char tmpBuf[64];
	char *pBuf = NULL;
	FILE *fp_tmp = NULL;
	
	if(!(fp_tmp = fopen("/var/wireless_neighbor_info", "r+")))
	{		
		printf("%s,error: can't open wireless_neighbor_info!\n", __FUNCTION__);
		return -1;
	}

	fgets(tmpBuf, sizeof(tmpBuf), fp_tmp);
	pBuf = strstr(tmpBuf, "##");

	if(pBuf)
	{
		strncpy(bestChannelBuf, pBuf + 2, 8);
	}
	else
	{
		fclose(fp_tmp);
		printf("bestChannel not Found in wireless_neighbor_info!\n", __FUNCTION__);
		return -1;
	}

	fprintf(stderr, "%s %d bestChannelBuf=%s!\n",__FUNCTION__,__LINE__,bestChannelBuf);
	fclose(fp_tmp);
	return 0;
}
#endif 

#ifdef  _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_WirelessBandwidthEnable_
int getWlan0BandWidth(void)
{
	char vBandwidth, vTmpChar;
	int result;

	if(!check_wlan_module())
	{
		return -1;
	}
	mib_get_s( MIB_WLAN_CHANNEL_WIDTH, (void *)&vBandwidth, sizeof(vBandwidth));
	if(vBandwidth==1)
	{
		mib_get_s( MIB_WLAN_11N_COEXIST, (void *)&vTmpChar, sizeof(vTmpChar));
		if(vTmpChar)
			vBandwidth=3;
	}

	if(vBandwidth == 0) 
	{
		result = 20;
	}
	else
	{
		result = 40;
	}
fprintf(stderr, "%s %d result=%d!\n",__FUNCTION__,__LINE__,result);
	return result;
}
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_WirelessChannelNumberEnable_
int getWlan0CurChannel(char *curChannel)
{	
	if(NULL == curChannel)
	{
		printf("%s,error: parameter is NULL!\n", __FUNCTION__);
		return -1;
	}
		
	if(!check_wlan_module())
	{
		printf("%s,error: wlan0 module disabled!\n", __FUNCTION__);
		return -1;
	}
	
	bss_info	bss;
	getWlBssInfo(WLANIF[0], &bss);
	sprintf(curChannel, "%d", bss.channel);
		fprintf(stderr, "%s %d curChannel=%s!\n",__FUNCTION__,__LINE__,curChannel);
	return 0;
}
#endif

#ifdef  _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_WirelessPowerEnable_
int getWlan0Power(void)
{
	unsigned char txpowerscale, phyband;
	int txpowervalue;

	if(!check_wlan_module())
	{
		return -1;
	}
	
	mib_local_mapping_get(MIB_TX_POWER, 0, (void *)&txpowerscale);
	mib_local_mapping_get(MIB_WLAN_PHY_BAND_SELECT, 0, (void *)&phyband);
	txpowervalue = get_TxPowerValue(phyband,txpowerscale);
fprintf(stderr, "%s %d txpowervalue=%d!\n",__FUNCTION__,__LINE__, txpowervalue);
	return txpowervalue;
}
#endif

#ifdef  _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_QosTypeEnable_
int getWlan0QosType(void)
{
	MIB_CE_MBSSIB_T Entry;
	mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry);
fprintf(stderr, "%s %d wmmEnabled=%d!\n",__FUNCTION__,__LINE__, Entry.wmmEnabled);
	return Entry.wmmEnabled;
}
#endif

#ifdef  _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_WirelessTypeEnable_
int getWlan0SpecialBand(unsigned int wlanBand)
{
	switch (wlanBand) 
	{
		case 1: 
			return 0;		
		case 2: 
			return 1;
		case 3: 
			return 2;
		case 8: 
			return 3;
		case 10: 
			return 4;
		case 11: 
			return 5;
		default:
			return 6;
	}
}
const char* wlan0ModeStr[7] = {"b", "g", "bg", "n", "gn", "bgn","x" };
int getWlan0WirelessType(char* curWirlessType, int size)
{
	if(NULL == curWirlessType)	
	{
		printf("%s,error: parameter is NULL!\n", __FUNCTION__);
		return -1;
	}

	if(size < 4)
	{
		printf("%s,error: buffer should larger than 3!\n", __FUNCTION__);
		return -1;
	}

	if(!check_wlan_module())
	{
		return -1;
	}
	
	int index;
#ifdef WLAN_BAND_CONFIG_MBSSID
	MIB_CE_MBSSIB_T Entry;
	mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry);
	index = getWlan0SpecialBand(Entry.wlanBand);
#else
	unsigned char wlband=0;
	mib_get_s(MIB_WLAN_BAND, (void *)&wlband, sizeof(wlband));
	index = getWlan0SpecialBand(wlband);
#endif
	if(index > 6)
	{
		printf("%s,error: get invalid wlan0 band!\n", __FUNCTION__);
		return -1;
	}

	strcpy(curWirlessType, wlan0ModeStr[index]);
fprintf(stderr, "%s %d curWirlessType=%s!\n",__FUNCTION__,__LINE__,curWirlessType);
	return 0;	
}
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_WorkingTimeEnable_
int getSysWorkingTime(void)
{
	struct sysinfo info;
	int upminutes;
	sysinfo(&info);		
	upminutes = (int) info.uptime / 60;
fprintf(stderr, "%s %d upminutes=%d!\n",__FUNCTION__,__LINE__,upminutes);
	return upminutes;
}
#endif

#ifdef _PRMT_SC_CT_COM_GroupCompanyService_Plugin_
int getPluginNameAndState(int instnum, char *pluginName, unsigned int *pluginState)
{
	FILE *fp;
	unsigned int stats, count = 0;
	char tmpbuf[1024], appname[1024];
	int nFound = 0;

	va_cmd("/bin/get_plugin_module", 0, 1);

	if(NULL == pluginName)
	{
		printf("%s,error:parameter pluginName is NULL!\n", __FUNCTION__);
		return -1;
	}

	if(NULL == pluginState)
	{
		printf("%s,error:parameter pluginState is NULL!\n", __FUNCTION__);
		return -1;
	}
	
	if ((fp = fopen("/tmp/applist_query", "r")) == NULL)
	{
		printf("error:%s can't open File applist_query!\n", __FUNCTION__);
		return -1;
	}
	
	while (fgets(tmpbuf, sizeof(tmpbuf), fp))
	{
		count ++;
		tmpbuf[strlen(tmpbuf) - 1] = '\0';
		memset(appname, '\0', 1024);
		sscanf(tmpbuf, "%s %u", appname, &stats);
		if(count == instnum)
		{
			strcpy(pluginName, appname);
			*pluginState = stats;
			nFound = 1;
			break;
		}
	}
	
	fclose(fp);

	if(nFound  == 1)
	{
		return 0;
	}
	return -2;
}
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_PluginUpNumberEnable_
unsigned int getPluginUpNumber(void)
{
	FILE *fp;
	unsigned int stats, count = 0;
	char tmpbuf[1024], appname[1024];

	va_cmd("/bin/get_plugin_module", 0, 1);

	if ((fp = fopen("/tmp/applist_query", "r")) == NULL)
	{
		printf("error:%s can't open File applist_query!\n", __FUNCTION__);
		return 0;
	}

	while (fgets(tmpbuf, sizeof(tmpbuf), fp))
	{
		tmpbuf[strlen(tmpbuf) - 1] = '\0';
		memset(appname, '\0', 1024);
		sscanf(tmpbuf, "%s %u", appname, &stats);
		if(stats)
		{
			count++;
		}
	}

	fclose(fp);
fprintf(stderr, "%s %d count=%u!\n",__FUNCTION__,__LINE__,count);
	return count;
}
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_PluginAllNumberEnable_
unsigned int getPluginAllNumber(void)
{
	FILE *fp;
	unsigned int count = 0;
	char tmpbuf[1024];
	
	va_cmd("/bin/get_plugin_module", 0, 1);
	
	if ((fp = fopen("/tmp/applist_query", "r")) == NULL)
	{
		printf("error:%s can't open File applist_query!\n", __FUNCTION__);
		return 0;
	}

	while (fgets(tmpbuf, sizeof(tmpbuf), fp))
	{
		count++;
	}

	fclose(fp);
fprintf(stderr, "%s %d PluginAllNum=%u!\n",__FUNCTION__,__LINE__,count);
	return count;
}
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_
int IS_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_ON(int item_bit) {
	unsigned char val[8];
	if(mib_get_s(PROVINCE_CWMP_PERFORMANCE_REPORT_SUBITEM,&val, sizeof(val))){
		if(val[item_bit/8]&(1<<(item_bit%8)))
			return 1;
	}
	return 0;
}

int SET_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM(int item_bit, int enable) {
	unsigned char val[8];
	if(mib_get_s(PROVINCE_CWMP_PERFORMANCE_REPORT_SUBITEM,&val, sizeof(val))){
		if(enable)
		{
			val[item_bit/8] = val[item_bit/8] | (1 << (item_bit%8));
		}
		else
		{
			val[item_bit/8] = val[item_bit/8] & ~(1 << (item_bit%8));
		}
		mib_set(PROVINCE_CWMP_PERFORMANCE_REPORT_SUBITEM,(void *) &val);
	}
	return 0;
}
#endif



#if defined _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_UpDataEnable_ || defined _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DownDataEnable_
static int getLanPortApplication(int pmap, char* application, int size)
{
	int i = 0;
	int apptype = 0;
	int total_entries = 0;
	int found = 0;
	MIB_CE_ATM_VC_T vc_entry;

	total_entries = mib_chain_total(MIB_ATM_VC_TBL);

	for(i = 0; i < total_entries; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vc_entry))
		continue;

		if(vc_entry.itfGroup & (1 << pmap))
		{
			found = 1;
			break;	
		}
	}

	if(found)
	{
		switch(vc_entry.applicationtype)
		{
			case	X_CT_SRV_OTHER:
				snprintf(application, size, "iTV");
				break;
			default:
				snprintf(application, size, "Internet");
		}
	}
	else
		snprintf(application, size, "Internet");

	return 0;
}
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_UpDataEnable_
unsigned long lan_rx_old[ELANVIF_NUM] = {0};
unsigned long wlan_rx_old = 0;
int getUpData(char *data, int size, int clear)
{
	unsigned long rx_bytes_cycle = 0;
	int logicPortId = 0;
	int offset = 0;
	char apptype[32] = {0};
	float bytes = 0, base = 1048576;
	unsigned long tx_pkts,tx_drops,tx_errs,rx_pkts,rx_drops,rx_errs;
	unsigned long long int tx_bytes,rx_bytes;
	struct net_device_stats nds;

	if(NULL == data)
	{
		printf("%s,error: parameter is NULL!", __FUNCTION__);
		return -1;
	}

	if (size < 64)
	{
		printf("%s,error: buffer should larger than 64!", __FUNCTION__);
		return -1;	
	}

	get_wlan_net_device_stats(getWlanIfName(), &nds);
	rx_bytes_cycle =  nds.rx_bytes - wlan_rx_old;
	bytes = rx_bytes_cycle/base;
	if(clear)
		wlan_rx_old = nds.rx_bytes;
	getLanPortApplication(PMAP_WLAN0, apptype, sizeof(apptype));
	snprintf(data+offset, size-offset, "WLAN:%s:%.3f-", apptype, bytes);
	offset = strlen(data);	

	for(logicPortId = 0 ; logicPortId < ELANVIF_NUM; logicPortId++)
	{

		if(RG_get_portCounter(logicPortId,&tx_bytes,&tx_pkts,&tx_drops,&tx_errs,&rx_bytes,&rx_pkts,&rx_drops,&rx_errs) == 0 ){
			// get fail , assign all counter to 0
			tx_pkts = tx_drops = tx_errs = rx_pkts = rx_drops = rx_errs = 0;
			tx_bytes = rx_bytes = 0;
		}

		//printf("%s, rx_bytes is %d, lan_tx_old[logicPortId] is %d\n", __FUNCTION__, rx_bytes, lan_rx_old[logicPortId]);
		rx_bytes_cycle = rx_bytes - lan_rx_old[logicPortId];
		//printf("%s, rx_bytes_cycle is %d\n", __FUNCTION__, rx_bytes_cycle);
		bytes = rx_bytes_cycle/base;
		if(clear)
			lan_rx_old[logicPortId] = rx_bytes;
		getLanPortApplication(logicPortId, apptype, sizeof(apptype));
		snprintf(data+offset, size-offset, "LAN%d:%s:%.3f-", logicPortId+1, apptype, bytes);
		offset = strlen(data);
	}

	data[offset-1] = 0;
fprintf(stderr, "%s %d updata=%s!\n",__FUNCTION__,__LINE__,data);
	return 0;
}
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DownDataEnable_
unsigned long lan_tx_old[ELANVIF_NUM] = {0};
unsigned long wlan_tx_old = 0;
int getDownData(char *data, int size, int clear)
{
	unsigned long tx_bytes_cycle = 0;
	int logicPortId = 0;
	int offset = 0;
	char apptype[32] = {0};
	float bytes = 0, base = 1048576;
	unsigned long tx_pkts,tx_drops,tx_errs,rx_pkts,rx_drops,rx_errs;
	unsigned long long int tx_bytes,rx_bytes;
	struct net_device_stats nds;

	if(NULL == data)
	{
		printf("%s,error: parameter is NULL!", __FUNCTION__);
		return -1;
	}

	if (size < 64)
	{
		printf("%s,error: buffer should larger than 64!", __FUNCTION__);
		return -1;
	}

	get_wlan_net_device_stats(getWlanIfName(), &nds);
	tx_bytes_cycle =  nds.tx_bytes - wlan_tx_old;
	bytes = tx_bytes_cycle/base;
	if(clear)
		wlan_tx_old = nds.tx_bytes;
	getLanPortApplication(PMAP_WLAN0, apptype, sizeof(apptype));
	snprintf(data+offset, size-offset, "WLAN:%s:%.3f-", apptype, bytes);
	offset = strlen(data);


	for(logicPortId = 0 ; logicPortId < ELANVIF_NUM; logicPortId++)
	{

		if(RG_get_portCounter(logicPortId,&tx_bytes,&tx_pkts,&tx_drops,&tx_errs,&rx_bytes,&rx_pkts,&rx_drops,&rx_errs) == 0 ){
			// get fail , assign all counter to 0
			tx_pkts = tx_drops = tx_errs = rx_pkts = rx_drops = rx_errs = 0;
			tx_bytes = rx_bytes = 0;
		}

		//printf("%s, tx_bytes is %d, lan_tx_old[logicPortId] is %d\n", __FUNCTION__, tx_bytes, lan_tx_old[logicPortId]);
		tx_bytes_cycle = tx_bytes - lan_tx_old[logicPortId];
		//printf("%s, tx_bytes_cycle is %d\n", __FUNCTION__, tx_bytes_cycle);
		bytes = tx_bytes_cycle/base;
		if(clear)
			lan_tx_old[logicPortId] = tx_bytes;
		getLanPortApplication(logicPortId, apptype, sizeof(apptype));
		snprintf(data+offset, size-offset, "LAN%d:%s:%.3f-", logicPortId+1, apptype, bytes);
		offset = strlen(data);
	}

	data[offset-1] = 0;
fprintf(stderr, "%s %d downdata=%s!\n",__FUNCTION__,__LINE__,data);
	return 0;
}
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_LANxStateEnable_
int getLANxState(char *state, int size)
{
	rtk_port_linkStatus_t eStatus;
	int offset = 0;
	int LogicPort = 0;
	int phyPort = 0;
	int wlan_enable = 0;
	unsigned char vchar;

	if(NULL == state)
	{
		printf("%s,error: parameter is NULL!", __FUNCTION__);
		return -1;
	}

	if (size < 64)
	{
		printf("%s,error: buffer should larger than 64!", __FUNCTION__);
		return -1;	
	}

	//OUTPUT sample:"LANw:1-LAN1:1-LAN2:0-LAN3:0-LAN4:0"

	mib_get_s(MIB_WIFI_MODULE_DISABLED, (void *)&vchar, sizeof(vchar));
	snprintf(state+offset, size-offset, "LANw:%d-", vchar?0:1);
	offset = strlen(state);
	
	for(LogicPort = 0; LogicPort < ELANVIF_NUM; LogicPort++)
	{
		phyPort = rtk_port_get_lan_phyID(LogicPort);
		rtk_port_link_get(phyPort, &eStatus);
		snprintf(state+offset, size-offset, "LAN%d:%d-", LogicPort+1, eStatus);

		offset = strlen(state);
	}

	state[offset-1] = 0;
fprintf(stderr, "%s %d state=%s!\n",__FUNCTION__,__LINE__,state);
	return 0;

}
#endif

#ifdef TERMINAL_INSPECTION_SC
int getLANxStateTerminal(char *state, int size)
{
	rtk_port_linkStatus_t eStatus;
	int offset = 0;
	int LogicPort = 0;
	int phyPort = 0;
	int wlan_enable = 0;
	unsigned char vchar;

	if(NULL == state)
	{
		printf("%s,error: parameter is NULL!", __FUNCTION__);
		return -1;
	}

	if (size < 64)
	{
		printf("%s,error: buffer should larger than 64!", __FUNCTION__);
		return -1;	
	}
	
	for(LogicPort = 0; LogicPort < ELANVIF_NUM; LogicPort++)
	{
		phyPort = rtk_port_get_lan_phyID(LogicPort);
		rtk_port_link_get(phyPort, &eStatus);
		snprintf(state+offset, size-offset, "LAN%d:%s  ", LogicPort+1, eStatus?"连接":"未连接");

		offset = strlen(state);
	}

	state[offset-1] = 0;

	return 0;

}

#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_LANxWorkBandwidthEnable_
int getLANxWorkBandwidth(char *bandwith, int size)
{
	int offset = 0;
	int logicPort = 0;
	int uni_capability = 0;
	int speed = 0;

	if(NULL == bandwith)
	{
		printf("%s,error: parameter is NULL!", __FUNCTION__);
		return -1;
	}

	if (size < 64)
	{
		printf("%s,error: buffer should larger than 64!", __FUNCTION__);
		return -1;
	}

	for(logicPort = 0; logicPort < ELANVIF_NUM; logicPort++)
	{
		uni_capability =  getUniPortCapability(logicPort);

		switch(uni_capability)
		{
			case	UNI_PORT_FE:
				speed = 100;
				break;
			case 	UNI_PORT_GE:
				speed = 1000;
				break;
		}

		snprintf(bandwith+offset, size-offset, "LAN%d:%d-",logicPort+1, speed);

		offset = strlen(bandwith);

		//printf("%s, offset is %d, bandwith is %s\n", __FUNCTION__, offset, bandwith);

		
		if(offset > size)	
		{
			printf("%s,error: buffer should larger than 64!", __FUNCTION__);
			return -1;
		}
	}

	bandwith[offset-1] = 0;
fprintf(stderr, "%s %d bandwidth=%s!\n",__FUNCTION__,__LINE__,bandwith);
	return 0;
}
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_AllDeviceNumberEnable_
int getAllDeviceNumber(void)
{
	int logicPortId = 0;
	int macidx = 0;
	int i = 0;
	unsigned int count=0, lanNetCnt = 0;
	rtk_rg_macEntry_t macEntry;
	lanHostInfo_t *pLanNetInfo=NULL;

	//add Lan port client
	for(logicPortId = 0; logicPortId < ELANVIF_NUM; logicPortId++)
	{
		macidx = 0;
		while (RT_ERR_RG_OK == rtk_rg_macEntry_find(&macEntry, &macidx))
		{
			if (macEntry.port_idx == rtk_port_get_lan_phyID(logicPortId)) {
				count++;
			}
			macidx++;
			memset(&macEntry, 0, sizeof(rtk_rg_macEntry_t));
		}
	}

	//add wlan client
	get_lan_net_info(&pLanNetInfo, &lanNetCnt);

	for(i = 0; i < lanNetCnt; i++)
	{
		if(pLanNetInfo[i].connectionType != 1)
			continue;

		count++;
	}

	if(pLanNetInfo)
		free(pLanNetInfo);

	fprintf(stderr, "%s %d alldeviceNum=%u!\n",__FUNCTION__,__LINE__,count);
	
	return count;
}
#endif

#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_WLANDeviceMACEnable_) || defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_LANDeviceMACEnable_) 
unsigned int sessCnt[256] = {0};
static int getEveryClientSessionCnt(void)
{
	int i = 0, retval = 0;
	rtk_l34_naptInbound_entry_t asic_intcpudp;

	memset(sessCnt, 0, sizeof(int)*256);

	for(i = 0; i < MAX_NAPT_IN_HW_TABLE_SIZE; i++)
	{
       retval = rtk_l34_naptInboundTable_get(i, &asic_intcpudp);
        if (retval == FAIL)
            continue;

        if (asic_intcpudp.valid != 0)
        {
			sessCnt[asic_intcpudp.intIp&0x000000ff]++;
        }		
	}

	return 0;
}


//caller should free client_options if it is not null.
static int getEveryClientOptions(struct dhcpOfferedAddrLease **client_options)
{
	char *buf = NULL;
	int lockfd = -1;
	int lease_cnt = 0;
	struct stat status;
	unsigned long leaseFileSize;

	lease_cnt = getDhcpClientLeasesDB(&buf, &leaseFileSize);
	if(lease_cnt <= 0)
		goto err;

	*client_options = (struct dhcpOfferedAddrLease *)buf;
	//printf("lease_cnt is %d\n", lease_cnt);
	return lease_cnt;

	err:
		if (buf)
			free(buf);

	return 0;
}

enum DeviceType
{
	CTC_Computer=0,
	CTC_Camera,
	CTC_HGW,
	CTC_STB,
	CTC_PHONE,
	CTC_UNKNOWN=100
};

static int getCategoryString(int type, char *name, int size)
{
	if(NULL == name)
		return -1;
	
	switch(type)
	{
		case CTC_Computer:
			snprintf(name, size, "Computer");
			break;
		case CTC_Camera:
			snprintf(name, size, "Camera");
			break;
		case CTC_HGW:
			snprintf(name, size, "HGW");
			break;
		case CTC_STB:
			snprintf(name, size, "STB");
			break;
		case CTC_PHONE:
			snprintf(name, size, "PHONE");
			break;
		case CTC_UNKNOWN:
		default:
			snprintf(name, size, "NONE");
	}

	return 0;
}

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_WLANDeviceMACEnable_
int  signalStrength[256] = {0};
static int getEveryClientSignalStrength(void)
{
	int i = 0, j = 0;
	int x = 0, y = 0;
	char *buff;
	unsigned int count;
	lanHostInfo_t *pLanNetInfo=NULL;
	WLAN_STA_INFO_Tp pInfo;
	char ifname[16];

	get_lan_net_info(&pLanNetInfo, &count);

	buff = calloc(1, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM + 1));

	for (x=0; x<NUM_WLAN_INTERFACE; x++)
	{
		for(y=0; y<=WLAN_MBSSID_NUM; y++)
		{
			memset(buff, 0, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM + 1));
			if(y==0)
				snprintf(ifname, 16, "%s", WLANIF[x]);
			else
				snprintf(ifname, 16, "%s-vap%d", WLANIF[x], y-1);

			if (getInFlags(ifname, 0)==0){
				continue;
			}
			if (getWlStaInfo(ifname, (WLAN_STA_INFO_Tp) buff) < 0) {
				printf("Read wlan sta info failed!\n");
				continue;
			}

			for (i = 1; i <= MAX_STA_NUM; i++) {
				pInfo = (WLAN_STA_INFO_Tp) & buff[i * sizeof(WLAN_STA_INFO_T)];
				if (pInfo->aid && (pInfo->flags & STA_INFO_FLAG_ASOC))
				{
					/*printf("[%s]wireless client MAC:%02X:%02X:%02X:%02X:%02X:%02X", __FUNCTION__,
							pInfo->addr[0], pInfo->addr[1],pInfo->addr[2],pInfo->addr[3],
							pInfo->addr[4], pInfo->addr[5]);*/

					for(j = 0; j < count; j++)
					{
						if(0 == memcmp(pLanNetInfo[j].mac, pInfo->addr, MAC_ADDR_LEN))
						{
							signalStrength[ntohl(pLanNetInfo[j].ip)&0x000000ff] = pInfo->rssi - 100;
							break;
						}
					}
				}
			}
		}
	}

	if(buff)
		free(buff);

	return 0;
}
#endif

static int moveNotLetterOrNumber(unsigned char *string, int len)
{
	int i = 0, j = 0, move_cnt = 0;
	int cur_len = len;
	
	while(0 != string[i])
	{
		if(((string[i] >= '0') && (string[i] <= '9')) ||
			((string[i] >= 'A') && (string[i] <= 'Z')) ||
			((string[i] >= 'a') && (string[i] <= 'z')))
		{
			i++;
			continue;
		}

		j = i;
			
		cur_len--;
		printf("%s, move %c\n", __FUNCTION__, string[i]);

		while(0 != string[j])
		{
			string[j] = string[j+1];
			j++;
		}
	}

	return cur_len;
}


#define DHCP_CTC_FIELD_LEN 36
#define DHCP_CTC_FQDN_LEN 64

static int addDeviceOptions
(
	char *postion, 
	int size, 
	int dhcp_lease_cnt, 
	struct dhcpOfferedAddrLease *pclientOptions, 
	u_int32_t ip
)
{
	int j = 0;
	int found_options = 0;
	int offset = 0;
	char op60_vendor[DHCP_CTC_FIELD_LEN] = {0};
	char op60_model[DHCP_CTC_FIELD_LEN] = {0};
	char op81_fqdn[DHCP_CTC_FQDN_LEN] = {0};
	char categoryName[32] = {0};

	if ((dhcp_lease_cnt > 0) && (NULL != pclientOptions))
	{
		for(j = 0; j < dhcp_lease_cnt; j++)
		{
			//printf("%s, dhcp ip is 0x%x, client ip is 0x%x\n", __FUNCTION__, pclientOptions[j].yiaddr, ip);
			if(pclientOptions[j].yiaddr == ip)
			{
				found_options = 1;
	
				if(0 == pclientOptions[j].isCtcVendor) //this option 60 is not china telecom enterprise type
				{
					//add Vendor
					if(pclientOptions[j].szVendor[0] != 0)
					{
						strncpy(op60_vendor, pclientOptions[j].szVendor, DHCP_CTC_FIELD_LEN);
						moveNotLetterOrNumber(op60_vendor, strlen(op60_vendor));
						snprintf(postion+offset, size - offset, "%s_", op60_vendor);
						offset += strlen(op60_vendor) + 1;
					}
					else
					{
						snprintf(postion+offset, size - offset, "NONE_");
						offset += 5;
					}							
				}
				else  // this option 60 is china telecom enterprise type
				{
					//add Vendor
					if(pclientOptions[j].szVendor[0] != 0)
					{
						strncpy(op60_vendor, pclientOptions[j].szVendor, DHCP_CTC_FIELD_LEN);
						moveNotLetterOrNumber(op60_vendor, strlen(op60_vendor));
						snprintf(postion+offset, size - offset, "%s&", op60_vendor);
						offset += strlen(op60_vendor) + 1;
					}
					else
					{
						snprintf(postion+offset, size - offset, "NONE&");
						offset += 5;
					}
					
					//add Category
					getCategoryString(pclientOptions[j].category, categoryName, sizeof(categoryName));
					snprintf(postion+offset, size - offset, "%s&", categoryName);
					offset += strlen(categoryName)+1;
	
					//add Model
					if(pclientOptions[j].szModel[0] != 0)
					{
						strncpy(op60_model, pclientOptions[j].szModel, DHCP_CTC_FIELD_LEN);
						moveNotLetterOrNumber(op60_model, strlen(op60_model));
						snprintf(postion+offset, size - offset, "%s_", op60_model);
						offset += strlen(op60_model) + 1;
					}
					else
					{
						snprintf(postion+offset, size - offset, "NONE_");
						offset += 5;
					}
				}
	
				//add FQDN(option 81)
				if(pclientOptions[j].szFQDN[0] != 0)
				{
					strncpy(op81_fqdn, pclientOptions[j].szFQDN, DHCP_CTC_FQDN_LEN);
					moveNotLetterOrNumber(op81_fqdn, strlen(op81_fqdn));
					snprintf(postion+offset, size - offset, "%s+", op81_fqdn);
					offset += strlen(op81_fqdn) + 1;
				}
				else
				{
					snprintf(postion+offset, size - offset, "NONE+");
					offset += 5;
				}
				
				break;
			}
		}
	}
	if(0 == found_options)
	{
		snprintf(postion+offset, size - offset, "NONE_NONE+");
		offset += 10;
	}

	return offset;
}
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_WLANDeviceMACEnable_
int getWLANDeviceMAC(char *deviceInfo, int size)
{
	lanHostInfo_t *pLanNetInfo=NULL;
	unsigned int count=0;
	int i = 0, report_cnt = 0;
	int offset = 0, len = 0;
	int dhcp_lease_cnt = 0;
	struct dhcpOfferedAddrLease *pclientOptions = NULL;
	char op12_devname[MAX_LANNET_DEV_NAME_LENGTH] = {0};

	if(NULL == deviceInfo)
	{
		printf("%s,error: parameter is NULL!", __FUNCTION__);
		return -1;
	}

	if (size < 1024)
	{
		printf("%s,error: buffer should larger than 64!", __FUNCTION__);
		return -1;
	}

	get_lan_net_info(&pLanNetInfo, &count);
	getEveryClientSessionCnt();
	getEveryClientSignalStrength();
	
	dhcp_lease_cnt = getEveryClientOptions(&pclientOptions);

	for(i = 0; i < count; i++)
	{
		if(pLanNetInfo[i].connectionType != 1)
			continue;
		
		snprintf(deviceInfo+offset, size - offset, "%02X:%02X:%02X:%02X:%02X:%02X+",
				pLanNetInfo[i].mac[0], pLanNetInfo[i].mac[1],pLanNetInfo[i].mac[2],pLanNetInfo[i].mac[3],
				pLanNetInfo[i].mac[4], pLanNetInfo[i].mac[5]);
		offset += 18;

		//add option 12, this is store in RG, so do not use dhcp lease content.
		if(pLanNetInfo[i].devName[0] != 0)
		{
			strncpy(op12_devname, pLanNetInfo[i].devName, MAX_LANNET_DEV_NAME_LENGTH);
			moveNotLetterOrNumber(op12_devname, strlen(op12_devname));
			snprintf(deviceInfo+offset, size - offset, "%s_", op12_devname);
			offset += strlen(op12_devname) + 1;
		}
		else
		{
			snprintf(deviceInfo+offset, size - offset, "NONE_");
			offset += 5;
		}
		
		//add dhcp options
		len = addDeviceOptions(deviceInfo+offset, size - offset, dhcp_lease_cnt, pclientOptions, pLanNetInfo[i].ip);
		offset += len;
		
		snprintf(deviceInfo+offset, size - offset, "%d", sessCnt[ntohl(pLanNetInfo[i].ip)&0x000000ff]);
		offset = strlen(deviceInfo);

		snprintf(deviceInfo+offset, size - offset, "+%d", signalStrength[ntohl(pLanNetInfo[i].ip)&0x000000ff]);
		offset = strlen(deviceInfo);

		snprintf(deviceInfo+offset, size - offset, "-");
		offset++;

		report_cnt++;
		if(report_cnt > 10)  //should not report larger than 10 client info.
			break;
	}

	if(report_cnt > 0)
		deviceInfo[offset-1] = 0;
	else
		snprintf(deviceInfo, size, "NONE");

	if(pLanNetInfo)
		free(pLanNetInfo);

	if(pclientOptions)
		free(pclientOptions);

	fprintf(stderr, "%s %d deviceInfo=%s!\n",__FUNCTION__,__LINE__,deviceInfo);

	return 0;
}
#endif


#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_LANDeviceMACEnable_
int getLANDeviceMAC(char *deviceInfo, int size)
{
	int logicPortId = 0;
	int macidx = 0;
	int report_cnt = 0;
	int offset = 0, len = 0;
	rtk_rg_macEntry_t macEntry;
	lanHostInfo_t *pLanNetInfo=NULL;
	unsigned int count=0;
	int i = 0, found_lannetinfo = 0;
	int dhcp_lease_cnt = 0;
	struct dhcpOfferedAddrLease *pclientOptions = NULL;
	char op12_devname[MAX_LANNET_DEV_NAME_LENGTH] = {0};

	if(NULL == deviceInfo)
	{
		printf("%s,error: parameter is NULL!", __FUNCTION__);
		return -1;
	}

	if (size < 1024)
	{
		printf("%s,error: buffer should larger than 64!", __FUNCTION__);
		return -1;
	}

	get_lan_net_info(&pLanNetInfo, &count);
	getEveryClientSessionCnt();
	dhcp_lease_cnt = getEveryClientOptions(&pclientOptions);

	memset(&macEntry, 0, sizeof(rtk_rg_macEntry_t));

	for(logicPortId = 0; logicPortId < ELANVIF_NUM; logicPortId++)
	{
		macidx = 0;
		while (RT_ERR_RG_OK == rtk_rg_macEntry_find(&macEntry, &macidx))
		{
			if (macEntry.port_idx == rtk_port_get_lan_phyID(logicPortId)) {				
				snprintf(deviceInfo+offset, size - offset, "LAN%d+", logicPortId+1);
				offset += 5;
				snprintf(deviceInfo+offset, size - offset, "%02X:%02X:%02X:%02X:%02X:%02X_",
						(unsigned char)macEntry.mac.octet[0], (unsigned char)macEntry.mac.octet[1], (unsigned char)macEntry.mac.octet[2],
						(unsigned char)macEntry.mac.octet[3], (unsigned char)macEntry.mac.octet[4], (unsigned char)macEntry.mac.octet[5]);
				offset += 18;

				for(i = 0; i < count; i++)
				{
					found_lannetinfo = 0;
					if(0 == memcmp(macEntry.mac.octet, pLanNetInfo[i].mac, ETHER_ADDR_LEN))		
					{
						//add option 12, this is store in RG, so do not use dhcp lease content.
						if(pLanNetInfo[i].devName[0] != 0)
						{
							strncpy(op12_devname, pLanNetInfo[i].devName, MAX_LANNET_DEV_NAME_LENGTH);
							moveNotLetterOrNumber(op12_devname, strlen(op12_devname));
							snprintf(deviceInfo+offset, size - offset, "%s_", op12_devname);
							offset += strlen(op12_devname) + 1;
						}
						else
						{
							snprintf(deviceInfo+offset, size - offset, "NONE_");
							offset += 5;
						}

						//add dhcp options
						len = addDeviceOptions(deviceInfo+offset, size - offset, dhcp_lease_cnt, pclientOptions, pLanNetInfo[i].ip);
						offset += len;

						//add session count
						snprintf(deviceInfo+offset, size - offset, "%d", sessCnt[ntohl(pLanNetInfo[i].ip)&0x000000ff]);
						offset = strlen(deviceInfo);

						snprintf(deviceInfo+offset, size - offset, "-");
						offset++;
						
						found_lannetinfo = 1;
						break;
					}
				}

				if(0 == found_lannetinfo)
				{
					snprintf(deviceInfo+offset, size - offset, "NONE_NONE_NONE+0-");
					offset += 17;
				}

				report_cnt++;
				if(report_cnt > 10)  //should not report larger than 10 client info.
					break;
			}

			macidx++;
			memset(&macEntry, 0, sizeof(rtk_rg_macEntry_t));
		}

		if(report_cnt > 10)  //should not report larger than 10 client info.
			break;
	}

	if(report_cnt > 0)
		deviceInfo[offset-1] = 0;
	else
		snprintf(deviceInfo, size, "NONE");

	if(pLanNetInfo)
		free(pLanNetInfo);

	if(pclientOptions)
		free(pclientOptions);

	fprintf(stderr, "%s %d deviceInfo=%s!\n",__FUNCTION__,__LINE__,deviceInfo);

	return 0;	
}
#endif

#define LAN_DEVICE_PKT_LOSS_LOCKFILE	"/var/run/lan_device_pkt_losss.lock"
#define PPPOE_DIALING_INFO_LOCKFILE		"/var/run/pppoe_dialing_info.lock"

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DevicePacketLossEnable_
int getDevicePacketLoss(char *pktLoss, int size)
{
	FILE *fp;
	int i = 0;
	int lockfd = 0;
	unsigned long deviceInfoFileSize = 0;
	char *buf = NULL;
	struct stat status;
	unsigned int deviceCnt = 0, offset = 0;
	struct stLanDevicePktLoss *pDevPktLoss = NULL;
	struct in_addr addr;

	if(NULL == pktLoss)
	{
		printf("%s,error: parameter is NULL!", __FUNCTION__);
		return -1;
	}

	if (size < 1024)
	{
		printf("%s,error: buffer should larger than 64!", __FUNCTION__);
		return -1;
	}

	//printf("getDevicePacketLoss==============lock file=======================\n");

	if ((lockfd = lock_file_by_flock(LAN_DEVICE_PKT_LOSS_LOCKFILE, 0)) == -1)
	{
		printf("%s, the file have been locked\n", __FUNCTION__);
		return -1;
	}

	if (stat(LAN_DEVICE_PKT_LOSS_FILENAME, &status) < 0)
		goto err;

	deviceInfoFileSize = (unsigned long)(status.st_size);
	
	buf = malloc(deviceInfoFileSize);
	if (buf == NULL)
		goto err;

	fp = fopen(LAN_DEVICE_PKT_LOSS_FILENAME, "r");
	if (fp == NULL)
		goto err;

	fread(buf, deviceInfoFileSize, 1, fp);
	fclose(fp);

	//printf("getDevicePacketLoss45213==============unlock file=======================\n");
	unlock_file_by_flock(lockfd);

	deviceCnt = deviceInfoFileSize/sizeof(struct stLanDevicePktLoss);
	pDevPktLoss = (struct stLanDevicePktLoss *)buf;

	for(i = 0; i < deviceCnt; i++)
	{
		snprintf(pktLoss+offset, size-offset, "%02X:%02X:%02X:%02X:%02X:%02X+", 
				pDevPktLoss[i].clinetMac[0], pDevPktLoss[i].clinetMac[1], pDevPktLoss[i].clinetMac[2],
				pDevPktLoss[i].clinetMac[3], pDevPktLoss[i].clinetMac[4], pDevPktLoss[i].clinetMac[5]);
		offset += 18;

		addr.s_addr = pDevPktLoss[i].ip;
		snprintf(pktLoss+offset, size-offset, "%s+", inet_ntoa(addr));
		offset = strlen(pktLoss);

		snprintf(pktLoss+offset, size-offset, "%d+", pDevPktLoss[i].pktLoss);
		offset = strlen(pktLoss);

		snprintf(pktLoss+offset, size-offset, "%d-", pDevPktLoss[i].averagDelay);
		offset = strlen(pktLoss);
	}

	if(deviceCnt > 0)
		pktLoss[offset-1] = 0;
	else
		snprintf(pktLoss, size, "NONE");
	fprintf(stderr, "%s %d pktLoss=%s!\n",__FUNCTION__,__LINE__,pktLoss);
	if (buf)
		free(buf);
	return 0;
	
err:
	//printf("getDevicePacketLoss45243==============unlock file=======================\n");
	unlock_file_by_flock(lockfd);
	if (buf)
		free(buf);

	return -1;
}
#endif

#ifdef  _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_CPURateEnable_
int getCPURate(unsigned int *CPURate)
{
	char cmd[100], buff[256], *ptr;
	FILE *fp;
	char filename[128];
	sprintf(filename, "/tmp/top.cpu.%08x", filename);
	sprintf(cmd, "top -n 1 | grep CPU > %s", filename);
	
//	sprintf(cmd, "top -n 1 | grep CPU > /tmp/top.cpu");
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	if ((fp = fopen(filename, "r")) == 0) {
		printf("%s can not open %s\n", __func__,filename);
		return -1;
	}
	fgets(buff, 256, fp);
	if ((ptr = strstr(buff, "CPU")) == NULL) 
		goto err;	
					
	if (sscanf(ptr, "CPU: %u .%*s", CPURate) < 1 ){
		printf("can not parse cpu used val.\n");
		goto err;
	}
	
	fclose(fp);
	unlink(filename);			
//	printf("CPURate= %u,filename=%s\n", *CPURate,filename);
//	printf("CPURate%s %s\n", __func__,CPURate);
	return 0;

	err:
		fclose(fp);
		unlink(filename);	
		return -1;
}
#endif

#ifdef  _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_MemRateEnable_
int getMemRate(unsigned int *MemRate)
{
	char cmd[100], buff[256], *ptr;
	unsigned int memUsed,memFree;
	unsigned long memTotal;
	
	FILE *fp;
	char filename[128];
	sprintf(filename, "/tmp/top.mem.%08x", filename);
	sprintf(cmd, "top -n 1 | grep Mem > %s", filename);
	
//	sprintf(cmd, "top -n 1 | grep Mem > /tmp/top.mem");
	va_cmd("/bin/sh", 2, 1, "-c", cmd);
	if ((fp = fopen(filename, "r")) == 0) {
		printf("%s can not open filename %s\n", __func__,filename);
		return -1;
	}
	fgets(buff, 256, fp);
	if ((ptr = strstr(buff, "Mem")) == NULL) 
		goto err;
	
	if (sscanf(ptr, "Mem: %uK%*s", &memUsed) < 1 ){
		printf("can not parse mem used val.\n");
		goto err;
	}
	
	if ((ptr = strstr(buff, "used")) == NULL) 
		goto err;	
	
	if (sscanf(ptr, "used, %uK%*s", &memFree) < 1 ){
		printf("can not parse mem free val.\n");
		goto err;
	}
	fclose(fp);
	unlink(filename);
	memTotal=memUsed+memFree;
	*MemRate = memUsed*100/memTotal;
	//printf("MemRate= %u filename= %s\n",*MemRate,filename);
	
	return 0;

err:
	fclose(fp);
	unlink(filename);	
	return -1;
}
#endif

#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DialingNumberEnable_) \
|| defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DialingErrorEnable_)
const char PPPOE_DIALING_INFO[] = "/tmp/ppp_dialing_info";
int getPppoeDialingNumber(void)
{
	char tmp[64]="";
	FILE *fp;
	int DialingNumber = 0,Error678,Error676,Error691,Error999,padiSendFlag;
	int lockfd = 0;

	if ((lockfd = lock_file_by_flock(PPPOE_DIALING_INFO_LOCKFILE, 0)) == -1)
	{
		printf("%s, the file %s have been locked\n", __FUNCTION__, PPPOE_DIALING_INFO_LOCKFILE);
		return -1;
	}
	if(fp = fopen(PPPOE_DIALING_INFO, "r")){
		if(fgets(tmp, sizeof(tmp), fp) != NULL){
			sscanf(tmp, "%d:%d:%d:%d:%d:%d",&DialingNumber,&Error678,&Error676,&Error691,&Error999,&padiSendFlag);
		}
		fclose(fp);
	}
	else{ 
		printf("fail to open file %s\n",PPPOE_DIALING_INFO);
	}	
	unlock_file_by_flock(lockfd);
	
	fprintf(stderr, "%s %d DialingNumber=%d!\n",__FUNCTION__,__LINE__,DialingNumber);
	return DialingNumber;
}

int getPppoeDialingError(char *DialingError)
{
	char tmp[64]="";
	FILE *fp;
	int DialingNumber,padiSendFlag;
	int	Error678 =0,Error676=0 ,Error691 = 0,Error999 = 0;
	int lockfd = 0;

	if ((lockfd = lock_file_by_flock(PPPOE_DIALING_INFO_LOCKFILE, 0)) == -1)
	{
		printf("%s, the file %s have been locked\n", __FUNCTION__, PPPOE_DIALING_INFO_LOCKFILE);
		return -1;
	}

	if(fp = fopen(PPPOE_DIALING_INFO, "r")){
		if(fgets(tmp, sizeof(tmp), fp) != NULL){
			sscanf(tmp, "%d:%d:%d:%d:%d:%d",&DialingNumber,&Error678,&Error676,&Error691,&Error999,&padiSendFlag);
		}
		fclose(fp);
	}else{ 
		printf("fail to open file %s\n",PPPOE_DIALING_INFO);
	}	
	unlock_file_by_flock(lockfd);
	
	sprintf(DialingError,"678:%d+676:%d+691:%d+999:%d",Error678,Error676,Error691,Error999);
	fprintf(stderr, "%s %d DialingError=%s!\n",__FUNCTION__,__LINE__,DialingError);
	return 0;
}
#endif

#if defined(CONFIG_USER_RTK_VOIP)
int get_register(int port)
{
	FILE *fh;
	char buf[MAX_VOIP_PORTS * MAX_PROXY];
	char p0_reg_st, p1_reg_st;
	int ret=0;	
	fh = fopen(_PATH_TMP_STATUS, "r");
	if (!fh) {
		printf("Warning: cannot open %s. Limited output.\n", _PATH_TMP_STATUS);
		printf("\nerrno=%d\n", errno);
		return 0;
	}

	memset(buf, 0, sizeof(buf));
	if (fread(buf, sizeof(buf), 1, fh) == 0) {
		printf("Web: The content of /tmp/status is NULL!!\n");
		printf("\nerrno=%d\n", errno);
		ret=0;
	}
	else {

		p0_reg_st = buf[port * MAX_PROXY];
		p1_reg_st = buf[port * MAX_PROXY + 1];
		if((p0_reg_st == '1')||(p1_reg_st == '1'))
		{
			ret=1;
		}
		else{
			ret=0;
		}
	}
	fclose(fh);
	return ret;
}

#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_VoiceInfoEnable_)
int voip_e8c_getInfo(char* voiceInfo, int size)
{
	voipCfgParam_t *pVoIPCfg;
	voipCfgPortParam_t *pCfg1;
	voipCfgPortParam_t *pCfg;
	int i,state,offset = 0;
	int voip_port,voip_port_temp;
	if(!voiceInfo||!size)
		return -1;
	if (voip_flash_get(&pVoIPCfg) != 0){
		printf("%s,error: parameter is NULL!\n", __FUNCTION__);
		return -1;
	}
	memset(voiceInfo,0,size);

	pCfg1 = &pVoIPCfg->ports[0];
	
	FILE *fh;
		char buf[MAX_VOIP_PORTS];
		
		fh = fopen(_PATH_TMP_E8C_STATUS, "r");
		if (!fh) {						
			printf("%s,error: File can't be read!\n", __FUNCTION__);
			return -1;
		}
		memset(buf, 0, sizeof(buf));
		if ((fread(buf, sizeof(buf), 1, fh) == 0) || (2 == pVoIPCfg->X_CT_servertype) || (pVoIPCfg->ports[voip_port].proxies[0].number == NULL)) {		
			printf("%s,error: File or number is NULL!\n", __FUNCTION__);
			fclose(fh);
			return -1;
		}
		else {
			for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
			{
				char p_reg_st;
				if((pVoIPCfg->ports[voip_port].proxies[0].enable & PROXY_ENABLED)==0)
				{
					state = 7;	
				}
				else
				{
					if(get_register(voip_port)==1)
					{
						state = 7;
					}else{
					p_reg_st=buf[voip_port];
					//fprintf(stderr,"p_reg_st=%c voip_port=%d\n", p_reg_st, voip_port);
					//fprintf(stderr, "buf is %s\n", buf);
					switch (p_reg_st) {
				case '0'://VOIP_STATUS_INIT
					state = 7;
					break;
				case '1'://VOIP_STATUS_SUCCESS
					state = 0;
					break;
				case '2'://VOIP_NOVALID_CONNECTION
					state = 1;
					break;
				case '3'://VOIP_STATUS_IADERROR
					state = 7;
					break;
				case '4'://VOIP_STATUS_NOROUTE
					state = 1;
					break;
				case '5'://VOIP_STATUS_NORESPONSE
					state = 1;
					break;
				case '6'://VOIP_STATUS_ACCOUNTERR
					state = 3;
					break;
				case '7'://VOIP_STATUS_UNKNOWN
					state = 7;
					break;
				case '8'://VOIP_STATUS_LINE_DISABLED
					state = 1;
					break;
				case '9'://VOIP_STATUS_ADDR_UNKNOW_HOST
					state = 2;
					break;						
				default:
					state = 7;
					break;
					}
				}
			}
			if(pVoIPCfg->ports[voip_port].proxies[0].number[voip_port] != '\0')
			{
				printf("[%s %d]number=%s\n", __func__, __LINE__, pVoIPCfg->ports[voip_port].proxies[0].number);
				voip_port_temp = voip_port + 1;
				snprintf(voiceInfo+offset, size-offset, "%d+%s+%d-",voip_port_temp,pVoIPCfg->ports[voip_port].proxies[0].number, state);
				offset = strlen(voiceInfo);
			}
		}
		voiceInfo[offset-1] = 0;
fprintf(stderr, "%s %d voiceInfo=%s!\n",__FUNCTION__,__LINE__,voiceInfo);
		}
		fclose(fh);	
	return 0;

}
#endif

int VoiceStateValue(unsigned int port)
{
	int PortStatus=0;
	voip_state_share_t *g_shmVoIPstate;
	voip_line_state_init_variables();
    voip_line_state_share_get(&g_shmVoIPstate);
	
	if (g_shmVoIPstate)
	{
	
		if(g_shmVoIPstate->VOIPLineStatus[port].voipVoiceServerStatus==REGISTER_ONGOING)
			return 0;

		if(g_shmVoIPstate->HookState[port]==0)
			return 0;	//idle

		if((g_shmVoIPstate->HookState[port]==1) && (g_shmVoIPstate->VOIPLineStatus[port].voip_Port_status==PORT_IDEL))
			return 3;

		switch(g_shmVoIPstate->VOIPLineStatus[port].voip_Port_status){
			case PORT_IDEL:
				PortStatus=0;
				break;
			case PORT_DIALING:
				PortStatus=2;
				break;
			case PORT_RINGING:
				PortStatus=3;
				break;
			case PORT_RINGBACKTONE:
				PortStatus=4;
				break;
			case PORT_CONNECTING:
				PortStatus=5;
				break;
			case PORT_CONNECTED:
				PortStatus=6;
				break;
			case PORT_RELEASE_CONNECTED:
			case PORT_DISABLE:
				PortStatus=9;
				break;		
			default:
				PortStatus=10;
		}

	}
	return PortStatus;
}

#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_VoiceStateEnable_)
int voip_e8c_getState(char*voiceState,int size )
{	
	voipCfgParam_t *pVoIPCfg;
	int i,offset=0;
	unsigned int status = 0;
	int voip_port, voip_port_temp;
	if(!voiceState||!size)
		return -1;
	if (voip_flash_get(&pVoIPCfg) != 0){
		printf("%s,error: parameter is NULL!\n", __FUNCTION__);
		return -1;
	}
	memset(voiceState,0,size);
	if (2 != pVoIPCfg->X_CT_servertype)
	{
		for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
		{
			status = VoiceStateValue(voip_port);
			if(pVoIPCfg->ports[voip_port].proxies[0].number != NULL)
			{
				voip_port_temp = voip_port + 1;
				//boaWrite(wp, "<td>%d:%d-</td>",voip_port_temp,portstatus); 
				snprintf(voiceState+offset, size-offset, "%d:%d-",voip_port_temp,status);
				offset = strlen(voiceState);
			}
		}
		voiceState[offset-1] = 0;
	}
	return 0;
}

#endif
#endif //end defined(CONFIG_USER_RTK_VOIP)

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_TEMPEnable_
int getTEMP(double *buf)
{
	rtk_transceiver_data_t transceiver, readableCfg;
	memset(&transceiver, 0, sizeof(transceiver));
	memset(&readableCfg, 0, sizeof(readableCfg));

	rtk_ponmac_transceiver_get
		(RTK_TRANSCEIVER_PARA_TYPE_TEMPERATURE, &transceiver);
		_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_TEMPERATURE, &transceiver, &readableCfg);

	*buf = atof(readableCfg.buf);
	fprintf(stderr, "%s %d temp=%.2lf!\n",__FUNCTION__,__LINE__, *buf);
	return 0;
}
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_OpticalInPowerEnable_
int getOpticalInPower(char *buf)
{
	rtk_transceiver_data_t transceiver, readableCfg;
	memset(&transceiver, 0, sizeof(transceiver));
	memset(&readableCfg, 0, sizeof(readableCfg));

	rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_RX_POWER, &transceiver);
	_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_RX_POWER, &transceiver, &readableCfg);

	strcpy(buf, readableCfg.buf);
	fprintf(stderr, "%s %d buf=%s!\n",__FUNCTION__,__LINE__,buf);
	return 0;
}
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_OpticalOutPowerEnable_
int getOpticalOutPower(char *buf)
{
	rtk_transceiver_data_t transceiver, readableCfg;
	memset(&transceiver, 0, sizeof(transceiver));
	memset(&readableCfg, 0, sizeof(readableCfg));

	rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_TX_POWER,
		&transceiver);
	_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_TX_POWER, &transceiver, &readableCfg);

	strcpy(buf, readableCfg.buf);
	fprintf(stderr, "%s %d buf=%s!\n",__FUNCTION__,__LINE__,buf);
	return 0;
}
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RoutingModeEnable_
int getRoutingMode()
{
    unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) 
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
  			printf("Get chain record error!\n");
			return -1;
		}
		if (Entry.enable == 0)
			continue;
		if (Entry.applicationtype&X_CT_SRV_INTERNET)
		{
			if(Entry.cmode == CHANNEL_MODE_BRIDGE)
				return 1; //bridge
			else
				return 0; //routing
		}
	}
	if(i==entryNum)
		return -1;
}
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterNumberEnable_
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterSuccessNumberEnable_
static const char REGISTER_NUM_FILE[] = "/tmp/oam_register_num";
static const char REGISTER_NUM_FILE_LAST[] = "/tmp/oam_register_num_last";

int getOLTNumberInFile(const char *fname, int *regNum, int *regSuccNum)
{
	int fd;
	char buf[256]={};
	char *tmp;
	
	fd = lock_file_by_flock(fname, 1);
	if(fd<0)
	{
		printf("%s %d: File lock error\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if(read(fd, buf, sizeof(buf))<0)
	{
		printf("%s %d: read error!\n", __FUNCTION__, __LINE__);
		unlock_file_by_flock(fd);
		return -1;
	}
	buf[255]='\0';
	if(tmp = strstr(buf, "registerNum="))
	{
		sscanf(tmp, "registerNum=%d\n%*s", regNum);
	}
	if(tmp = strstr(buf, "registerSuccNum="))
	{
		sscanf(tmp, "registerSuccNum=%d\n%*s", regSuccNum);
	}
	unlock_file_by_flock(fd);
	return 0;
}

int getRegisterOLTNumber()
{
	int regNum = 0, regSuccNum = 0;
	int lstRegNum = 0, lstRegSuccNum = 0;

	va_cmd("/bin/oamcli", 4, 1, "get", "ctc", "registernum", "1"); /* save current number to /tmp/oam_register_num */
	if(getOLTNumberInFile(REGISTER_NUM_FILE, &regNum, &regSuccNum)<0)
	{
		printf("%s %d: getCurOLTRegistertime failed!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if(getOLTNumberInFile(REGISTER_NUM_FILE_LAST, &lstRegNum, &lstRegSuccNum)<0)
	{
		printf("%s %d: getCurOLTRegistertime failed!\n", __FUNCTION__, __LINE__);
		lstRegNum=0; // When boa first start up, REGISTER_NUM_FILE_LAST may not exist!!
	}
	return regNum - lstRegNum; //Must clear to 0 for a new day!
}
int getRegisterOLTSuccNumber()
{
	int regNum = 0, regSuccNum = 0;
	int lstRegNum = 0, lstRegSuccNum = 0;

	va_cmd("/bin/oamcli", 4, 1, "get", "ctc", "registernum", "1"); /* save current number to /tmp/oam_register_num */
	if(getOLTNumberInFile(REGISTER_NUM_FILE, &regNum, &regSuccNum)<0)
	{
		printf("%s %d: getCurOLTRegistertime failed!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if(getOLTNumberInFile(REGISTER_NUM_FILE_LAST, &lstRegNum, &lstRegSuccNum)<0)
	{
		printf("%s %d: getCurOLTRegistertime failed!\n", __FUNCTION__, __LINE__);
		lstRegSuccNum=0; // When boa first start up, REGISTER_NUM_FILE_LAST may not exist!!
	}
	return regSuccNum - lstRegSuccNum; //Must clear to 0 for a new day!
}
#endif
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_MulticastNumberEnable_
int getMulticastNumber()
{
	FILE *fd;
	char line[256] = {};
	char *tmp;
	int v1Grp=0, v2Grp=0, v3Grp=0;
	char cmd[100];
	const char partern[]="igmp list: ";
	char fileName[128] = {};
	sprintf(fileName, "/tmp/mcgrp.%08x", fileName);
	const char fname[] = "/proc/rg/igmpSnooping";
	sprintf(cmd, "cat %s | grep \"%s\" > %s", fname, partern, fileName);
	va_cmd("/bin/sh", 2, 1, "-c", cmd);

	fd = fopen(fileName, "r");
	if(!fd)
	{
		printf("%s %d: open file failed!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	while(fgets(line, 256, fd));
	{
		if(tmp = strstr(line, partern))
		{
			sscanf(tmp+strlen(partern), "V1=%d, V2=%d, V3=%d", &v1Grp, &v2Grp, &v3Grp);
		}
	}
	fclose(fd);
	unlink(fileName);
	fprintf(stderr, "%s %d multNum=%d!\n",__FUNCTION__,__LINE__, v1Grp+v2Grp+v3Grp);
	return v1Grp+v2Grp+v3Grp;
}
#endif

#ifdef _PRMT_X_CT_COM_MULTICAST_DIAGNOSIS_

int setMcastVidToSmux(char *ifname, int vid)
{
	int ret;
	char buf[50] = {0};
	
	sprintf(buf, "%d", vid);
	ret = va_cmd("/bin/ethctl", 5, 1, "setsmux", ALIASNAME_NAS0, ifname, "mvid", buf);
	
	return (ret == 0 )?1:0;
}
#endif





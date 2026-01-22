#ifndef INCLUDE_SUBR_CU_H
#define INCLUDE_SUBR_CU_H
#ifdef CONFIG_USER_CUSPEEDTEST
#include <sys/time.h>

enum
{
	eSpeedTest_None=0,
	eSpeedTest_Requested,
	eSpeedTest_Completed,

	eSpeedTest_STAT_PPPOE_DIALING,
	eSpeedTest_STAT_SERVER_CONNECTING,
	eSpeedTest_STAT_SERVER_CONNECTED,
	eSpeedTest_STAT_SPEED_TESTING,
	eSpeedTest_STAT_UPLOAD_TEST_RESULT,
	eSpeedTest_STAT_TEST_FINISHED,

	eSpeedTest_End /*last one*/
};

enum
{
	eDownlink=0,
	eUplink,

	eDirectionEnd
};

enum
{
	edownloadMode=0,
	eserverMode,
	euploadMode,

	eModeEnd
};

enum
{
	esource_RMS=4,
	esource_APP,
	esource_PLATFORM,
	esource_WEB,

	esourceEnd
};

enum
{
	eSpeedTest_FailReason_None=0,
	eSpeedTest_FailReason_PPPoE_Unconfiged,
	eSpeedTest_FailReason_PPPoE_Failed,
	eSpeedTest_FailReason_Wait_For_Completed,
	eSpeedTest_FailReason_Req_Not_Supported,
	eSpeedTest_FailReason_Req_Format_Error,
	eSpeedTest_FailReason_Frame_Format_Error,
	eSpeedTest_FailReason_Req_Timeout,
	eSpeedTest_FailReason_Upload_data_error,
	eSpeedTest_FailReason_User_Password_Error,
	eSpeedTest_FailReason_File_Cant_Download,

	eSpeedTest_FailReason_End /*last one*/
};

#define MAX_SAMPLED 15

struct CU_SpeedTest_Diagnostics
{
	int		DiagnosticsState;
#ifdef CONFIG_USER_CUSPEEDTEST
	int 		testDirection;
#endif
	int 		testMode;
	char 	*pdownloadURL;
//#ifdef SUPPORT_UPLOAD_SPEEDTEST	
	char 	*puploadURL;
//#endif	
	char 	*ptestURL;
	char		*preportURL;
	unsigned int	status;
	unsigned 	int 	Cspeed;
	unsigned int	Aspeed;
	unsigned int	Bspeed;
	unsigned int	maxspeed;
	struct timeval		starttime;
	struct timeval		endtime;
	unsigned long	Totalsize;
	unsigned long 	backgroundsize;
	unsigned int 	Failcode;
	unsigned int	FailReason;
	char 	*Eupppoename;
	char		*Eupassword;
	char		*pInterface;
	char		IfName[32];
	char  	*wanip;
	char		*pppoename;
	char		*pppoepasswd;

	int		http_pid;
	char 	*prealURL;
	int 		source;
	char *identify;

#ifdef CONFIG_E8B
	unsigned char max_sampled;	/* 0: do not sample */
	unsigned long wan_rx_bytes_start;
	unsigned long wan_tx_bytes_start;
	unsigned long *SampledValues;
	unsigned long *SampledTotalValues;
	unsigned int TestBytesSent;
#endif
	char *test_if;
	unsigned int	newwanflag;
	char *wanPppoeName;
};

void clearSpeedTestDiagnostics(struct CU_SpeedTest_Diagnostics *);
void FreeSpeedTestDiagnosticsSouce(struct CU_SpeedTest_Diagnostics *);
int setSpeedtestDoneFlag(int);
int getCWMPSpeedtestDoneFlag();
void StartCU_SpeedTestDiag_General(struct CU_SpeedTest_Diagnostics *);
#ifdef CONFIG_USER_DBUS_PROXY
extern int get_state_change_flag();
extern void set_state_change_flag(int flag);
extern int get_status_change_flag();
extern int set_status_change_flag(int flag);
#endif
#endif

void get_pon_traffic(uint64_t  *tx_value, uint64_t  *rx_value);
void get_pon_temperature(char * xString);
void get_pon_voltage(char * xString);
void get_pon_current(char * xString);
void get_pon_txpower(char *xString);
void get_pon_rxpower(char *xString);

#ifdef CONFIG_USER_CUMANAGEDEAMON
struct wan_status_info {
	char protocol[10];
	char ipAddr[INET_ADDRSTRLEN];
	char *strStatus;
	char servName[MAX_WAN_NAME_LEN];
	unsigned short vlanId;
	unsigned char igmpEnbl;
	char qosEnbl;
	char servType[20];
	char encaps[8];
	char netmask[INET_ADDRSTRLEN];
	char gateway[INET_ADDRSTRLEN];
	char dns1[INET_ADDRSTRLEN];
	char dns2[INET_ADDRSTRLEN];
	char ipv6Addr[64];	/* With Prefix Length */
	char ipv6Prefix[64];	/* With Prefix Length */
	char ipv6Gateway[INET6_ADDRSTRLEN];
	char ipv6Dns1[INET6_ADDRSTRLEN];
	char ipv6Dns2[INET6_ADDRSTRLEN];
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	unsigned char ipv6PrefixOrigin;
	unsigned char addrMode;
#endif
};

int getSYSInfoTimer();
void startRegStatus();
int getUserRegResult(char *para1, char *para2);
int getWanName2(MIB_CE_ATM_VC_T * pEntry, char *name);
int get_wan_status(int ifindex, struct wan_status_info *reEntry);
void get_query_wan_info(char *wanname, char *strStatus, char *ipv6Addr, char *ipv6prefixLen, unsigned char *ipv6Gateway, char *ipv6Dns1, char *ipv6Dns2, char *ipv6Prefix);
int getTimeFormat(struct timeval *pTime, char *timestring);
int get_attach_device_type(unsigned char *, char *);
int getDHCPClientType(char *mac, int *hosttype);
int getLanNetInfoEx(char *pMacAddr, int *num, char *info);
int getDevWifiInfo(char *pMacAddr, int *num, char *info);
int add_acl_rule_for_dscp_mark(unsigned char *);
int del_acl_rule_for_dscp_mark(unsigned char *);
int get_acl_rule_for_dscp_mark(unsigned char *);
int getAttachDevInfo(char *info);
void getWanSrcIP(struct in_addr server_addr, char *ipaddr);
void getWanPPPoEUsername(char *pppoename, int namelen);

int get_attach_device_maxbandwidth(unsigned char *pMacAddr, unsigned int *pUsBandWidth, unsigned int *pDsBandWidth, unsigned int *pUsBandWidthMin, unsigned int *pDsBandWidthMin);
int get_rule_for_dscp_mark(unsigned char *smac);
#endif

#ifdef CONFIG_CU
void cuPPPoEManual_Action_DisconOrCon(MIB_CE_ATM_VC_T vcEntry);
#endif

#if defined(PON_HISTORY_TRAFFIC_MONITOR)
void get_pon_traffic_base(char *xString, int start);
void get_pon_traffic_history(char *xString, int start, int num, int len);
#endif

extern int getWANIfindexFromIfName(char *wanName);

#ifdef _PRMT_C_CU_FTPSERVICE_	
void cu_ftp_server_accounts_init();	
#endif

#if defined(CONFIG_CU_BASEON_YUEME)
#ifdef COM_CUC_IGD1_ParentalControlConfig
int setupMacFilter4ParentalCtrl();
int getPlatformStatus(char *Srvip, int *port, int *status);
#endif
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
int rtk_dbus_init_traffic_monitor_rule(void);
#endif
#endif

#ifdef COM_CUC_IGD1_WMMConfiguration
int getWMMEnable(int instnum);
int rtk_setup_wmm_service_rule(MIB_CE_WMM_SERVICE_T *entry, int type);
int rtk_fc_add_wmm_service_rule(MIB_CE_WMM_SERVICE_T *entry);
int rtk_fc_del_wmm_service_rule(MIB_CE_WMM_SERVICE_T *entry);
int set_wmm_service_tbl();
int flush_wmm_service_tbl();
#endif
#endif

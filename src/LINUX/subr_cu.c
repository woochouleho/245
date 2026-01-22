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
#include <stdarg.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

#include "rtk_timer.h"
#include "mib.h"
#include "utility.h"
#include "debug.h"

#include "rtk/ponmac.h"
#include <module/gpon/gpon.h>
#include "rtk/epon.h"
#include "hal/chipdef/chip.h"
#include "rtk/pon_led.h"
#include "rtk/stat.h"
#include "rtk/switch.h"
#if defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_stat.h>
#include <rtk/rt/rt_switch.h>
#include <rtk/rt/rt_epon.h>
#include <rtk/rt/rt_gpon.h>
#endif

#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#include <rt_flow_ext.h>
#include <rtk/rt/rt_l2.h>
#endif

#ifdef CONFIG_USER_CUMANAGEDEAMON
#ifdef CONFIG_RTK_OMCI_V1
#include <omci_api.h>
#include <gos_type.h>
#endif
#endif

#ifdef CONFIG_USER_CUSPEEDTEST
#include "cJSON.h"
typedef void (*sighandler_t)(int);
#endif

#include <common/util/rt_util.h>
#include "subr_cu.h"
#include "sockmark_define.h"

extern int getSYSInfoTimer();
extern int get_online_and_offline_device_info(lanHostInfo_t **, unsigned int *);
#ifdef _PRMT_X_CT_COM_WANEXT_
extern void set_IPForward_by_WAN(int );
#endif


#define CURL_GET_SPEED_RESULT_FILE "/tmp/curl_get_speed_result_file"
#define CURL_REPORT_SPEED_RESULT_FILE "/tmp/curl_report_speed_result_file" 

#ifdef _PRMT_X_CU_EXTEND_
void handle_WANIPForwardMode(int wan_idx)
{
	MIB_CE_ATM_VC_T wan_entry;

	if(mib_chain_get(MIB_ATM_VC_TBL, wan_idx, &wan_entry) == 0)
		return;
	if(wan_entry.IPForwardModeEnabled)
	{
#ifdef NEW_PORTMAPPING	
		int isRemove = 0;
		if (wan_entry.itfGroup) 
		{ // Remove port-based mapping
			wan_entry.itfGroup = 0;
			isRemove = 1;
			mib_chain_update(MIB_ATM_VC_TBL, (void *)&wan_entry, wan_idx);
		}

		if (isRemove) {
			setupnewEth2pvc();
		}
#endif
	}

	set_IPForward_by_WAN(wan_idx);
	return;
}

void handle_WANIPForwardMode_all()
{
	int i, total_entry;
	MIB_CE_ATM_VC_T wan_entry;
	
	total_entry = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0 ; i < total_entry ; i++) {
		if(mib_chain_get(MIB_ATM_VC_TBL, i, &wan_entry) == 0)
			continue;
		handle_WANIPForwardMode(i);
	}
}
#endif

#ifdef CONFIG_USER_CUMANAGEDEAMON
int generateWanName2(MIB_CE_ATM_VC_T *entry, char* wanname)
{
	char vpistr[6];
	char vcistr[6];
	char vid[16];
	int i, mibtotal;
	MIB_CE_ATM_VC_T tmpEntry;
	MEDIA_TYPE_T mType;

	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0; i< mibtotal; i++)
	{
		mib_chain_get(MIB_ATM_VC_TBL, i, &tmpEntry);
		if(tmpEntry.ifIndex == entry->ifIndex)
			break;
	}
	if(i==mibtotal)
		return -1;
	i++;

	mType = MEDIA_INDEX(entry->ifIndex);
	sprintf(wanname, "%d_", i);
	if(entry==NULL || wanname ==NULL)
		return -1;
	memset(vpistr, 0, sizeof(vpistr));
	memset(vcistr, 0, sizeof(vcistr));
	if (entry->applicationtype&X_CT_SRV_TR069)
		strcat(wanname, "TR069_");
#ifdef CONFIG_USER_RTK_VOIP
	if (entry->applicationtype == X_CT_SRV_VOICE)
		strcat(wanname, "VOICE_");
	else if (entry->applicationtype&X_CT_SRV_VOICE)
		strcat(wanname, WAN_VOIP_VOICE_NAME_CONN);
#endif
	if (entry->applicationtype&X_CT_SRV_INTERNET)
	{
#if defined(CONFIG_SUPPORT_HQOS_APPLICATIONTYPE)
		if(entry->othertype == OTHER_HQOS_TYPE)
			strcat(wanname, "HQoS_");
		else
#endif
		strcat(wanname, "INTERNET_");
	}
	if (entry->applicationtype&X_CT_SRV_OTHER)
	{
#ifdef CONFIG_SUPPORT_IPTV_APPLICATIONTYPE
		if(entry->othertype == OTHER_IPTV_TYPE)
			strcat(wanname, "IPTV_");
		else		
#endif
		strcat(wanname, "OTHER_");
	}
#ifdef CONFIG_YUEME
	if (entry->applicationtype&X_CT_SRV_SPECIAL_SERVICE_1)
		strcat(wanname, "SPECIAL_SERVICE_1_");
	if (entry->applicationtype&X_CT_SRV_SPECIAL_SERVICE_2)
		strcat(wanname, "SPECIAL_SERVICE_2_");
	if (entry->applicationtype&X_CT_SRV_SPECIAL_SERVICE_3)
		strcat(wanname, "SPECIAL_SERVICE_3_");
	if (entry->applicationtype&X_CT_SRV_SPECIAL_SERVICE_4)
		strcat(wanname, "SPECIAL_SERVICE_4_");
#endif
	if(entry->cmode == CHANNEL_MODE_BRIDGE)
		strcat(wanname, "B_");
	else
		strcat(wanname, "R_");
	if ((mType == MEDIA_ETH) || (mType == MEDIA_PTM)) {
		strcat(wanname, "VID_");
		if(entry->vlan==1){
			sprintf(vid, "%d", entry->vid);
			strcat(wanname, vid);
		}
	}
	else if (mType == MEDIA_ATM) {
		sprintf(vpistr, "%d", entry->vpi);
		sprintf(vcistr, "%d", entry->vci);
		strcat(wanname, vpistr);
		strcat(wanname, "_");
		strcat(wanname, vcistr);
	}
	return 0;
}

int getWanName2(MIB_CE_ATM_VC_T * pEntry, char *name)
{
	if (pEntry == NULL || name == NULL)
		return 0;
#ifdef _CWMP_MIB_
	if (*(pEntry->WanName))
		strcpy(name, pEntry->WanName);
	else
#endif
	{			//if not set by ACS. then generate automaticly.
		generateWanName2(pEntry, name);
	}
	return 1;
}
#endif

#ifdef CONFIG_SUPPORT_CAPTIVE_PORTAL
static char* http_redirect2CaptivePortal_page_header =
	"HTTP/1.1 302 Object Moved\r\n"
	"Location: %s\r\n"
	"Server: adsl-router-gateway\r\n"
	"Content-Type: text/html\r\n"
	"Content-Length: %d\r\n"
	"\r\n"
	"%s";
static char* http_redirect2CaptivePortal_page_content =
	"<html><head><title>Object Moved</title></head>"
	"<body><h1>Object Moved</h1>This Object may be found in "
	"<a HREF=\"%s\">here</a>.</body><html>";

static void gen_http_redirect2CaptivePortalURL_reply(char * content, int size)
{
	char buff[512] = {0};
	int filesize;
	char cp_url[MAX_URL_LEN]={0};
	mib_get_s(MIB_CAPTIVEPORTAL_URL, (void *)cp_url, sizeof(cp_url));

	snprintf(buff, 512, http_redirect2CaptivePortal_page_content, cp_url);
	filesize = strlen(buff);

	snprintf(content, size, http_redirect2CaptivePortal_page_header, cp_url, filesize, buff);
}

void enable_http_redirect2CaptivePortalURL(int enable)
{
#ifdef SUPPORT_WEB_REDIRECT
	FILE *fp;
	char cmdStr[100];
	char contents[1024] = {0};
	char cp_url[MAX_URL_LEN]={0};

	if(enable)
	{
		/* when enable push web function, hwnat rule should be created at tcp handshake phase.*/
		fp = fopen("/proc/rg/tcp_hw_learning_at_syn", "w");
		if(fp)
		{
			fprintf(fp, "0\n");
			fclose(fp);
		}
		else
			fprintf(stderr, "open /proc/rg/tcp_hw_learning_at_syn fail!\n");

		RG_set_http_trap_for_bridge(1);
		gen_http_redirect2CaptivePortalURL_reply(contents, sizeof(contents));
		
		printf("<%s, %d, contents=%s>\n", __FUNCTION__, __LINE__, contents);
		RG_set_redirect_http_Count(1, contents, strlen(contents), 1);
	}
	else
	{
/*
		fp = fopen("/proc/rg/tcp_hw_learning_at_syn", "w");
		if(fp)
		{
			fprintf(fp, "1\n");
			fclose(fp);
		}
		else
    			fprintf(stderr, "open /proc/rg/tcp_hw_learning_at_syn fail!\n");
*/
		RG_set_http_trap_for_bridge(0);
		//RG_set_redirect_http_all(0, content, 0, count);
		RG_set_redirect_http_Count(0, cp_url, strlen(cp_url), -1);
	}
#endif
}
#endif

#ifdef CONFIG_E8B
unsigned int WANDEVNUM_ATM = 2;
unsigned int WANDEVNUM_PTM = 3;
unsigned int WANDEVNUM_ETH = 1;
#else
unsigned int WANDEVNUM_ATM = 1;
unsigned int WANDEVNUM_PTM = 2;
unsigned int WANDEVNUM_ETH = 3;
#endif

static unsigned int getWanInstFromMediaType(MEDIA_TYPE_T type)
{
	unsigned int wanInst;

	if (type == MEDIA_ETH)
		wanInst = WANDEVNUM_ETH;
	else if (type == MEDIA_PTM)
		wanInst = WANDEVNUM_PTM;
	else			//default
		wanInst = WANDEVNUM_ATM;

	return wanInst;
}

int transfer2WANterface( unsigned int ifindex, char *name )
{
	int total,i;
	MIB_CE_ATM_VC_T *pEntry, vc_entity;

	if( ifindex==DUMMY_IFINDEX ) return -1;
	if( name==NULL ) return -1;
	name[0]=0;

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for( i=0; i<total; i++ )
	{
		pEntry = &vc_entity;
		if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)pEntry ) )
			continue;
		if(pEntry->ifIndex==ifindex)
		{
			char strfmt[]="InternetGatewayDevice.WANDevice.%u.WANConnectionDevice.%u.%s.%u"; //wt-121v8-3.33, no trailing dot
			char ipstr[]="WANIPConnection";
			char pppstr[]="WANPPPConnection";
			char *pconn=NULL;
			unsigned int instnum=0;

			if( (pEntry->cmode==CHANNEL_MODE_PPPOE) ||
#ifdef PPPOE_PASSTHROUGH
			    ((pEntry->cmode==CHANNEL_MODE_BRIDGE)&&(pEntry->brmode==BRIDGE_PPPOE)) ||
#endif
			    (pEntry->cmode==CHANNEL_MODE_PPPOA) )
			{
				pconn = pppstr;
				instnum = pEntry->ConPPPInstNum;
			}else{
				pconn = ipstr;
				instnum = pEntry->ConIPInstNum;
			}

			if( pEntry->connDisable==0 )
			{
				unsigned int waninst;
				waninst=getWanInstFromMediaType(MEDIA_INDEX(pEntry->ifIndex));
				sprintf( name, strfmt, waninst, pEntry->ConDevInstNum , pconn, instnum );

				break;
			}else
				return -1;
		}
	}
	return 0;
}

int getWANIfindexFromIfName(char *wanName)
{
	int i, mibtotal, ifindex=-1;
	MIB_CE_ATM_VC_T tmpEntry;
	char Ifname[64]={0};

	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0; i< mibtotal; i++)
	{
		mib_chain_get(MIB_ATM_VC_TBL, i, &tmpEntry);
		generateWanName(&tmpEntry, Ifname);
		if(strcmp(wanName, Ifname) == 0)
		{
			ifindex = tmpEntry.ifIndex;
			break;
		}
	}

	return ifindex;
}

#ifdef CONFIG_USER_CUSPEEDTEST
#ifdef CONFIG_USER_DBUS_PROXY
#define SIGNAL_FINISH	1
#define SIGNAL_STATUS	2
int state_change_flag=0;
int status_chagne_flag=0;

int get_state_change_flag()
{
	return state_change_flag;
}

void set_state_change_flag(int flag)
{
	state_change_flag = flag;
}

int get_status_change_flag()
{
	return status_chagne_flag;
}

int set_status_change_flag(int flag)
{
	status_chagne_flag = flag;
}


void setstatus_for_dbus(int signal)
{
	char state;
	if(signal == SIGNAL_FINISH){
		state_change_flag = 1;
	}else if(signal == SIGNAL_STATUS){
		status_chagne_flag = 1;
	}
}
#endif

#define SPEED_TEST_RESULT_FILE "/tmp/speed_test_result"

int speedtestdoneflag=0;
int cwmp_speedtestdoneflag=0;

int getSpeedtestDoneFlag()
{
	return speedtestdoneflag;
}

int setSpeedtestDoneFlag(int flag)
{
	speedtestdoneflag = flag;
}

int getCWMPSpeedtestDoneFlag()
{
	return cwmp_speedtestdoneflag;
}

int setCWMPSpeedtestDoneFlag(int flag)
{
	cwmp_speedtestdoneflag = flag;
}

int is_spec_tianjin_enable()
{
#ifdef CONFIG_CU_BASEON_CMCC
	unsigned char enable;
	mib_get( CU_SRVMGT_TIANJIN_SPEC_ENABLE, (void*)&enable);
	return enable;
#else
	return 0;
#endif
}

void FreeSpeedTestDiagnosticsSouce(struct CU_SpeedTest_Diagnostics *p)
{
	if( p->ptestURL )
	{
		free( p->ptestURL );
		p->ptestURL = NULL;
	}
	
	if( p->preportURL )
	{
		free( p->preportURL );
		p->preportURL = NULL;
	}

	if(p->pdownloadURL )
	{
		free( p->pdownloadURL );
		p->pdownloadURL = NULL;
	}

#ifdef SUPPORT_UPLOAD_SPEEDTEST
	if(p->puploadURL )
	{
		free( p->puploadURL );
		p->puploadURL = NULL;
	}
#endif

	if( p->Eupppoename)
	{
		free( p->Eupppoename );
		p->Eupppoename = NULL;
	}

	if( p->Eupassword)
	{
		free( p->Eupassword );
		p->Eupassword = NULL;
	}

	if( p->pInterface)
	{
		free( p->pInterface );
		p->pInterface = NULL;
	}

	if( p->wanPppoeName)
	{
		free( p->wanPppoeName );
		p->wanPppoeName = NULL;
	}
}

static void ResetCU_SpeedTestResultValues(struct CU_SpeedTest_Diagnostics *p)
{
	if(p)
	{
		p->status = 0;
		p->Cspeed = 0;
		p->Aspeed = 0;
		p->Bspeed = 0;
		p->maxspeed = 0;
		p->Totalsize = 0;
		p->backgroundsize = 0;
		p->Failcode = 0;
		p->FailReason = 0;

		p->http_pid=0;
	
		memset( &p->starttime, 0, sizeof(p->starttime) );	
		memset( &p->endtime, 0, sizeof(p->endtime) );
/*
		if( p->pInterface && p->pInterface[0] && (transfer2IfName( p->pInterface, p->IfName )==0) )
		{
#ifdef DUAL_STACK_LITE
			MIB_CE_ATM_VC_T wan;
			int idx;

			if(getATMVCEntry(p->pInterface, &wan, &idx) == 0 && wan.dslite_enable)
				strcpy(p->IfName, "tun1");
#endif

			LANDEVNAME2BR0(p->IfName);
		}else
			p->IfName[0]=0;
*/	

		if(p->prealURL){
			free(p->prealURL);
			p->prealURL=NULL;
		}

		if(p->wanip){
			free(p->wanip);
			p->wanip=NULL;
		}
		if(p->pppoename){
			free(p->pppoename);
			p->pppoename=NULL;
		}
		if(is_spec_tianjin_enable())
		{
			if(p->pppoepasswd){
				free(p->pppoepasswd);
				p->pppoepasswd=NULL;
			}
		}

		if(p->identify)
		{
			free( p->identify );
			p->identify = NULL;
		}

#ifdef CONFIG_E8B
		unsigned char max_sampled = MAX_SAMPLED;
		p->max_sampled = max_sampled;

		if(p->SampledValues)
			free(p->SampledValues);
		p->SampledValues = NULL;

		if(p->SampledTotalValues)
			free(p->SampledTotalValues);
		p->SampledTotalValues = NULL;

		if(max_sampled > 0)
		{
			p->SampledValues = malloc(sizeof(double) * max_sampled);
			memset(p->SampledValues, 0, (sizeof(double) * max_sampled));
			p->SampledTotalValues = malloc(sizeof(double) * max_sampled);
			memset(p->SampledTotalValues, 0, (sizeof(double) * max_sampled));
	}
#endif

		p->newwanflag = 0;
		if(p->test_if){
			free(p->test_if);
			p->test_if = NULL;
		}
	}
	unlink(SPEED_TEST_RESULT_FILE);
}

void clearSpeedTestDiagnostics(struct CU_SpeedTest_Diagnostics *p)
{
	memset(p,0,sizeof(struct CU_SpeedTest_Diagnostics));
}

void StopCU_SpeedTestDiag(struct CU_SpeedTest_Diagnostics *p)
{
#if 1
#ifdef SUPPORT_UPLOAD_SPEEDTEST
	if(p && (p->testDirection == eUplink))
		system("/bin/killall uploadTest");
	else
#endif	
	{
#if defined(CONFIG_E8B) && defined(CONFIG_USER_AXEL)
		system("/bin/killall axel");
#endif
	}
#else
#if defined(CONFIG_E8B) && defined(CONFIG_USER_AXEL)
	unsigned char use_axel = 0;
	mib_get_s(PROVINCE_TR143_SPEED_UP, &use_axel, sizeof(use_axel));
	if(use_axel)
	{
		system("/bin/killall axel");
	}
	else
#endif
	{
		TR143StopHttpDiag( &gDownloadDiagnostics );
#ifdef CONFIG_USER_FTP_FTP_FTP
		TR143StopFtpDiag( &gDownloadDiagnostics );
		unlink(FTPDIAG_DOWNLOADFILE);
		unlink(FTPDIAG_DOWNLOADFILE_RESULT);
#endif //CONFIG_USER_FTP_FTP_FTP
	}
#endif
}

int getTimeFormat(struct timeval *pTime, char *timestring)
{
	struct tm m;
	if((pTime == NULL) || (timestring == NULL))
		return 0;
	
	localtime_r(&pTime->tv_sec, &m);
	sprintf(timestring, "%04d%02d%02d%02d%02d%02d", m.tm_year+1900, m.tm_mon+1, m.tm_mday, m.tm_hour, m.tm_min, m.tm_sec);
	return 1;
}	

int URLEncode(const char* str, const int strSize, char* result, const int resultSize)  
{  
    int i;  
    int j = 0;//for result index  
    char ch;  
  
    if ((str==NULL) || (result==NULL) || (strSize<=0) || (resultSize<=0)) {  
        return 0;  
    }  
  
    for ( i=0; (i<strSize)&&(j<resultSize); ++i) {  
        ch = str[i];  
        if (((ch>='A') && (ch<='Z')) ||  
            ((ch>='a') && (ch<='z')) ||  
            ((ch>='0') && (ch<='9'))) {  
            result[j++] = ch;  
        } else if (ch == ' ') {  
		continue;
        } else if (ch == '.' || ch == '-' || ch == '_' || ch == '*' || ch == ':' || ch == ',') {  
            result[j++] = ch;  
        } else {  
            if (j+3 < resultSize) {  
                sprintf(result+j, "%%%02X", (unsigned char)ch);  
                j += 3;  
            } else {  
                return 0;  
            }  
        }  
    }  
  
    result[j] = '\0';  
    return j;  
}

char *strWeb_SpeedTestState[] =
{
	"None",
	"Requested",
	"Completed",

	"STAT_PPPOE_DIALING",
	"STAT_SERVER_CONNECTING",
	"STAT_SERVER_CONNECTED",
	"STAT_SPEED_TESTING",
	"STAT_UPLOAD_TEST_RESULT",
	"STAT_TEST_FINISHED",	
	""
};

char *getSpeedTestFile()
{
	return SPEED_TEST_RESULT_FILE;	
}

static void writeSpeedTestFile(struct CU_SpeedTest_Diagnostics *p)
{	
	FILE *fp;
	char tmpstr[128]={0};

	fp = fopen(SPEED_TEST_RESULT_FILE,"w");
	if(fp){
		switch(p->source){
			case esource_RMS:
				strcpy(tmpstr,"RMS");
				break;
			case esource_APP:
				strcpy(tmpstr,"APP");
				break;
			case esource_PLATFORM:
				strcpy(tmpstr,"PLATFORM");
				break;
			case esource_WEB:
				strcpy(tmpstr,"WEB");
				break;
			default:
				break;
		}
		fprintf(fp,"Source\t%s\n",tmpstr);

#ifdef SUPPORT_UPLOAD_SPEEDTEST
		if(p->testMode==edownloadMode)
			fprintf(fp,"TestMode\t%s\n", "downloadMode");
		else if(p->testMode==euploadMode)
			fprintf(fp,"TestMode\t%s\n", "uploadMode");
		else if(p->testMode==eserverMode)
			fprintf(fp,"TestMode\t%s\n", "serverMode");
		fprintf(fp,"UploadURL\t%s\n",p->puploadURL?p->puploadURL:"");
		fprintf(fp,"Type\t%s\n",p->testDirection==eUplink?"UpLink":"DownLink");
#else
		fprintf(fp,"TestMode\t%s\n",p->testMode==edownloadMode?"downloadMode":"serverMode");
#endif
		fprintf(fp,"DownloadURL\t%s\n",p->pdownloadURL?p->pdownloadURL:"");
		fprintf(fp,"TestURL\t%s\n",p->ptestURL?p->ptestURL:"");
		fprintf(fp,"ReportURL\t%s\n",p->preportURL?p->preportURL:"");
		fprintf(fp,"Eupppoename\t%s\n",p->Eupppoename?p->Eupppoename:"");
		fprintf(fp,"Eupassword\t%s\n",p->Eupassword?p->Eupassword:"");
		fprintf(fp,"WanInterface\t%s\n",p->IfName);

		if((p->DiagnosticsState>=eSpeedTest_None) && (p->DiagnosticsState<=eSpeedTest_Completed))
			strcpy(tmpstr,strWeb_SpeedTestState[p->DiagnosticsState]);
		else
			tmpstr[0]=0;
		fprintf(fp,"DiagnosticsState\t%s\n",tmpstr);
		
		if(p->status>= eSpeedTest_STAT_PPPOE_DIALING)
			strcpy(tmpstr,strWeb_SpeedTestState[p->status]);
		else
			tmpstr[0]=0;
		fprintf(fp,"Status\t%s\n",tmpstr);

		fprintf(fp,"WanIP\t%s\n",p->wanip?p->wanip:"");
		fprintf(fp,"PPPoeUsername\t%s\n",p->pppoename?p->pppoename:"");
		fprintf(fp,"WanPPPoename\t%s\n",p->wanPppoeName?p->wanPppoeName:"");
		fprintf(fp,"RealURL\t%s\n",p->prealURL?p->prealURL:"");

		sprintf(tmpstr,"%.2f",((double)p->Cspeed*8)/(1024.0*1024.0));
		fprintf(fp,"Cspeed\t%s\n",tmpstr);

		sprintf(tmpstr,"%.2f",((double)p->Aspeed*8)/(1024.0*1024.0));
		fprintf(fp,"Aspeed\t%s\n",tmpstr);

		sprintf(tmpstr,"%.2f",((double)p->Bspeed)/(1024.0*1024.0));
		fprintf(fp,"Bspeed\t%s\n",tmpstr);

		sprintf(tmpstr,"%.2f",((double)p->maxspeed*8)/(1024.0*1024.0));
		fprintf(fp,"Maxspeed\t%s\n",tmpstr);

		getTimeFormat(&(p->starttime), tmpstr);
		fprintf(fp,"Starttime\t%s\n",tmpstr);

		getTimeFormat(&(p->endtime), tmpstr);
		fprintf(fp,"Endtime\t%s\n",tmpstr);

		sprintf(tmpstr,"%.2f",((double)p->Totalsize)/1024.0);
		fprintf(fp,"Totalsize\t%s\n",tmpstr);

		sprintf(tmpstr,"%.2f",((double)p->backgroundsize)/1024.0);
		fprintf(fp,"Backgroundsize\t%s\n",tmpstr);
		
		fprintf(fp,"Failcode\t%u\n",p->Failcode);

		fclose(fp);
	}
}

int getValueFromSpeedTestFile(char *para, char *value)
{
	int nBytesSent = 0;
	char *speedfile=getSpeedTestFile();
	FILE *fp;

	if(access(speedfile, F_OK) != 0){
		return 0;
	}else{
		fp = fopen(speedfile,"r");
		if(fp){
			char buff[256];
			char part1[32];
			char part2[256];
			
			while (fgets(buff, sizeof(buff), fp) != NULL) {
				part1[0]=0;
				part2[0]=0;
				sscanf(buff,"%s\t%s",part1,part2);
				if(!strcmp(part1,"Source")){
					if(strcmp(part2,"APP"))
						return 0;
				}else if(!strcmp(part1,para)){
					if(strlen(part2))
						strcpy(value,part2);
					return 1;
				}
			}
	
			fclose(fp);			
		}
		return 0;
	}

}

#if defined(CONFIG_E8B) && defined(CONFIG_USER_AXEL)

#ifdef CONFIG_SUPPORT_AUTO_DIAG

void cleanup_speedtest_interface()
{
	char ifname[IFNAMSIZ] = {0};
	char ifname_simu[IFNAMSIZ] = {0};
	int i, totalNum;
	MIB_CE_ATM_VC_T simuEntry;

	totalNum = mib_chain_total(MIB_SIMU_ATM_VC_TBL);
	for(i = 0; i < totalNum; i++)
	{
		if(!mib_chain_get(MIB_SIMU_ATM_VC_TBL, i, (void *)&simuEntry))
			continue;

		memset(ifname, 0, IFNAMSIZ);
		if(111 == simuEntry.mbs)
			break;
	}

	if(i == totalNum)
		return;

	ifGetName(PHY_INTF(simuEntry.ifIndex), ifname, sizeof(ifname));
	snprintf(ifname_simu, IFNAMSIZ,  "%s_0", ifname);

	va_cmd("/bin/spppctl", 2, 1, "del", "7");

#if defined(CONFIG_RTL_SMUX_DEV)
	//printf("smuxctl delete interface, ifname=%s\n", ifname_simu);
	va_cmd("/bin/smuxctl", 2, 1, "--if-delete", ifname_simu);
#else
	va_cmd("/bin/ethctl", 4, 1, "remsmux", "ipoe", ALIASNAME_NAS0, ifname_simu);
#endif

#ifdef CONFIG_RTK_L34_ENALE
	printf("DEL interface, ifname=%s, wan_idx=%d, acl_idx=%d\n", ifname_simu, simuEntry.rg_wan_idx, simuEntry.cdvt);
	RG_Del_Simu_Trap_ACL(simuEntry.cdvt);
	RG_WAN_Interface_Del(simuEntry.rg_wan_idx);
#endif
	mib_chain_delete(MIB_SIMU_ATM_VC_TBL, i);
}
#endif

int skipThisOptmize(unsigned char step)
{
	unsigned char optimize_test=0;

	mib_get(MIB_SPEEDTEST_REMOVE_OPTIMIZE_TEST_RESULT,(void *)&optimize_test);
	if(optimize_test & (1<<(step-1)))
		return 1;
}

unsigned int get_customer_optimize_value()
{
	unsigned int optimize_value=0;

	mib_get(MIB_SPEEDTEST_OPTIMIZE_VALUE,(void *)&optimize_value);
	return optimize_value;
}

#ifdef CONFIG_RTK_HOST_SPEEDUP
#define SPEEDTEST_RATE_330M		42240	//(330*1024/8)
#define SPEEDTEST_RATE_450M		57600	//(450*1024/8)
#define SPEEDTEST_RATE_530M 	67840	//(530*1024/8)
#define SPEEDTEST_RATE_550M 	70400	//(550*1024/8)
#define SPEEDTEST_RATE_650M		83200	//(650*1024/8)
#define SPEEDTEST_RATE_765M		97920	//(765*1024/8)
#define SPEEDTEST_RATE_180M		23040	//(180*1024/8)
#define SPEEDTEST_RATE_205M 	26240	//(205*1024/8)
#define SPEEDTEST_RATE_230M 	29440	//(230*1024/8)
#define SPEEDTEST_RATE_290M		37120	//(290*1024/8)
#define SPEEDTEST_RATE_10M		1280		//(10*1024/8)

//don't let sample value exceed 10% of average sample value.
//if exceeding half of samples need modification, use real total average value to modify all sample value
static int optmize_test_result(unsigned int *sample_values, int sample_num)
{
	unsigned int avg_value=0, max_value_diff=0, value_diff=0;
	int i=0, invalid_num=0, max_value_diff_index=-1;
	unsigned int realavg=0;

	if(sample_values == NULL)
		return 0;

	if(skipThisOptmize(1))
		return 0;

	/* avg_value is the average rate of all valid sample */
	for(i=0; i<sample_num; i++)
	{
		if(sample_values[i] > 0)
			avg_value += sample_values[i];
		else
			invalid_num++;
	}
	if((sample_num - invalid_num) == 0)
		return 0;
	avg_value /= (sample_num - invalid_num);
	//fprintf(stderr, "%s avg_value=%lld\n", __func__, avg_value);
	if(invalid_num == 0)
	{
		int ex_num=0;
		
		realavg = avg_value*sample_num;
		for(i=0; i<sample_num; i++ )
		{
			if(sample_values[i] < (avg_value/15))
			{
				realavg -= sample_values[i];
				ex_num++;
			}
		}
		/*drop all tiny samples and save the average rate of the rest samples to realavg */
		realavg /= (sample_num-ex_num);

		/* when average rate lower than 765Mbps, don't tune average rage, I think we just need to care 1Gbps bandwidth */
		if ((realavg < SPEEDTEST_RATE_765M) && (avg_value < SPEEDTEST_RATE_650M))
		{
			for(i=0; i<sample_num; i++)
			{
				int deviation;
			
				if (avg_value > SPEEDTEST_RATE_450M)
					deviation = (int)avg_value*(2*i-sample_num)/sample_num;
				else
					deviation = (int)realavg*(2*i-sample_num)/sample_num;
				srandom(time(NULL)+i);
				deviation = deviation*(random()%20)/1000;
				sample_values[i] = realavg + deviation;
			}
			return 1;
		}
	}
	else if ((sample_num<5)|| (sample_num-invalid_num<5) || ((sample_num-invalid_num) < ((sample_num+1)/2)))
	{
		//printf("\n===>>use avg create sample");
		//fprintf(stderr, "%s realavg=%lld\n", __func__, realavg);
		for(i=0; i<sample_num; i++)
		{
			int deviation = (int)avg_value*(2*i-sample_num)/sample_num;
			srandom(time(NULL)+i);
			deviation = deviation*(random()%20)/1000;
			sample_values[i] = avg_value + deviation;
		}
		return 1;
	}
	for(i=0; i<sample_num; i++)
	{
		if(sample_values[i] > 0)
		{
			if(sample_values[i]>avg_value)
				value_diff = sample_values[i] - avg_value;
			else
				value_diff = avg_value - sample_values[i];
			if(value_diff > max_value_diff)
			{
				max_value_diff_index = i;
				max_value_diff = value_diff;
			}
		}
	}
	if((max_value_diff_index >= 0) && (max_value_diff > (avg_value/10)))
	{
		sample_values[max_value_diff_index] = 0;
		optmize_test_result(sample_values, sample_num);
		return 1;
	}
	else
	{
		for(i=0; i<sample_num; i++)
		{
			if(sample_values[i] == 0)
			{
				int deviation = (int)avg_value*(2*i-sample_num)/sample_num;
				srandom(time(NULL)+i);
				deviation = deviation*(random()%20)/1000;
				sample_values[i] = avg_value+deviation;
			}
		}
	}
	return 0;
}
#endif

static int getrealrate(unsigned int *rate)
{
#ifdef CONFIG_RTK_HOST_SPEEDUP
	FILE* fp;
	char buf[1024];
	unsigned int total_sample_values[120];
	unsigned int sample_values[120];
	int valid_count=0, validSampleIdx=0;
	unsigned int maxRate=0;
	int i, j;
	unsigned int sum = 0;
	unsigned int first_sample_value=0;
#else
	int fd;
	char buf[64];
#endif

#ifdef CONFIG_RTK_HOST_SPEEDUP
	if((fp = fopen("/proc/HostSpeedUP-Info", "r"))!=NULL)
	{
		int sample_num=0;

		while(fgets(buf,sizeof(buf),fp))
		{
			if(strstr(buf,"valid sample num") && sscanf(buf,"valid sample num:%d",&sample_num)==1)
			{
				if(sample_num>0 && fgets(buf,sizeof(buf),fp))
				{
					char *tmp=buf,*token;
    					fgets(buf,sizeof(buf),fp);
					fgets(buf,sizeof(buf),fp);//get real sample value
					valid_count=0;
					while((token=strsep(&tmp," "))!=NULL)
					{
						if (valid_count < sample_num)
						{
							if(0 == valid_count)//drop first sample value
							{
								first_sample_value = strtoul(token, NULL, 10);
								valid_count++;
								continue;
							}
							else
								total_sample_values[valid_count-1] = strtoul(token, NULL, 10) - first_sample_value;	//KBps
							if (1 == valid_count)
							{
								sample_values[0] = total_sample_values[0];					//KBps
							}
							else
							{
								sample_values[valid_count-1] = total_sample_values[valid_count-1] - total_sample_values[valid_count-2];
							}

							valid_count++;
						}
						else
							break;
					}
					valid_count--;

					/* found the best rate value */
					if (valid_count > 0)
					{
						if(valid_count > 2)
						{
							optmize_test_result(sample_values, valid_count) ;
						}
						sum = 0;
						for (i=0; i<valid_count; i++)
						{
							//DEBUG_TRACE(DEBUG_RTK_API_FLAG,("[%s %d]: sample[%d]=&u\n",__func__,__LINE__,i,sample_values[i]));
							sum += sample_values[i];
						}

						*rate = sum/valid_count*8;
						if(*rate>=1000000)//anhui app stuck when rate exceed 1000000
						{
							*rate = 900000+(*rate)%100000;
						}
	                    		fclose(fp);
	                    		return 0;
					}

					break;
				}
			}
		}
		fclose(fp);
	}
	return -1;
#else
	fd = open("/var/fh_speedtest_ds", O_RDONLY);
	if(fd < 0)
	{
		printf("[%s %d]:open file error\n", __func__, __LINE__);
		return -1;
	}
	
	if(!flock_set(fd, F_RDLCK))
	{
	    memset(buf, 0, sizeof(buf));
	    read(fd, buf, sizeof(buf));
        *rate = *(unsigned int *)buf;
		flock_set(fd, F_UNLCK);
	}
	close(fd);

	return 0;
#endif
}

static int get_or_create_speedtest_interface(struct CU_SpeedTest_Diagnostics *p, unsigned char auto_create_wan, char *test_if)
{
	int i;
	int totalNum;
	MIB_CE_ATM_VC_T Entry;
	MIB_CE_ATM_VC_T simuEntry;
	char ifname[IFNAMSIZ] = {0};
	struct in_addr addr  = {0};
	char ifname_simu[IFNAMSIZ];
	struct data_to_pass_st msg;
	int idx = 0;
	int def_wan_idx=-1;
	int ret;
	char *specified_if = p->IfName;
	char *pppusername = p->Eupppoename;
	char *ppppassword = p->Eupassword;
	int routeflag=0;

	if(specified_if == NULL || specified_if[0] == '\0' || test_if == NULL)
	{
		printf( "Use default route\n");
#ifdef CONFIG_GPON_FEATURE
#ifdef CONFIG_RTK_HOST_SPEEDUP
        def_wan_idx = routeInternetWanCheck();
#endif
#endif

#ifdef CONFIG_CU
		if((-1 != def_wan_idx) && (!p->wanip)){
			mib_chain_get(MIB_ATM_VC_TBL, def_wan_idx, (void *)&Entry);
			ifGetName(Entry.ifIndex, ifname, sizeof(ifname));
			if(getInAddr(ifname, IP_ADDR, (void *)&addr) == 1)
			{
				p->wanip = strdup(inet_ntoa(addr));
				if(p->pppoename)
					free(p->pppoename);
				p->pppoename = strdup(Entry.pppUsername);
			}
		}
#endif
		return -1;
	}

	// find specified WAN
	totalNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if(Entry.enable == 0)
			continue;

			ifGetName(Entry.ifIndex, ifname, sizeof(ifname));
		if(strcmp(ifname, specified_if) == 0){
			if(Entry.cmode != CHANNEL_MODE_BRIDGE)
				routeflag = 1;
			break;
		}
        
#ifdef CONFIG_GPON_FEATURE
        if (-1 == def_wan_idx)
        {
            if ((Entry.cmode != CHANNEL_MODE_BRIDGE) && (Entry.applicationtype & X_CT_SRV_INTERNET) &&
                    getInFlags(ifname, &ret) && (ret & IFF_UP) &&
                    getInAddr(ifname, IP_ADDR, &addr))
                def_wan_idx = i;
        }
#endif
		}

	if(i == totalNum)
	{
		printf( "%s not found, use default route\n", specified_if);
#ifdef CONFIG_GPON_FEATURE
#ifdef CONFIG_RTK_HOST_SPEEDUP

		if (-1 != def_wan_idx)
		{
			mib_chain_get(MIB_ATM_VC_TBL, def_wan_idx, (void *)&Entry);
			set_speedup_usflow(&Entry);
		}
#endif
#endif
		return -1;
	}

#ifdef CONFIG_GPON_FEATURE
#ifdef CONFIG_RTK_HOST_SPEEDUP
	/* host speedup must assign stream id for quickly ack*/
	set_speedup_usflow(&Entry);
#endif
#endif

#ifdef CONFIG_SUPPORT_AUTO_DIAG
	//Specified WAN has IP, use it to do diagnostics
		if(getInAddr(ifname, IP_ADDR, (void *)&addr) == 1)
		{
			strcpy(test_if, ifname);
			if(p->wanip)
				free(p->wanip);
			p->wanip = strdup(inet_ntoa(addr));
			if(p->pppoename)
				free(p->pppoename);
			p->pppoename = strdup(Entry.pppUsername);
			if(is_spec_tianjin_enable())
			{
				if(p->pppoepasswd)
					free(p->pppoepasswd);
				p->pppoepasswd = strdup(Entry.pppPassword);
			}
			printf( "%s has an IP addres, use it\n", test_if);
			return 0;
		}else if(routeflag)
			return -2;

	if(!auto_create_wan)
	{
		strcpy(test_if, ifname);
		return 0;
	}

	//Create new WAN
	totalNum = mib_chain_total(MIB_SIMU_ATM_VC_TBL);
	for(i = 0; i < totalNum; i++)
	{
		if(!mib_chain_get(MIB_SIMU_ATM_VC_TBL, i, (void *)&simuEntry))
			continue;

		if(111 == simuEntry.mbs)
			break;
	}

	memcpy(&simuEntry, &Entry, sizeof(MIB_CE_ATM_VC_T));
	simuEntry.applicationtype = X_CT_SRV_INTERNET;
	simuEntry.cmode = CHANNEL_MODE_PPPOE;
	simuEntry.brmode = BRIDGE_DISABLE;
	simuEntry.mbs = 111;	//create for TR-143
	simuEntry.MacAddr[MAC_ADDR_LEN-1] = (simuEntry.MacAddr[MAC_ADDR_LEN-1] + 8);
	simuEntry.MacAddr[MAC_ADDR_LEN-2]++;
	simuEntry.mtu = Entry.mtu - 8;
	simuEntry.pppCtype = CONTINUOUS;
	simuEntry.AddrMode = IPV6_WAN_AUTO;
	simuEntry.Ipv6DhcpRequest = M_DHCPv6_REQ_IAPD;
	simuEntry.itfGroup = 0;
	if(i != totalNum)
	{
		mib_chain_update(MIB_SIMU_ATM_VC_TBL, &simuEntry, i);
	}
	else
	{
		mib_chain_add(MIB_SIMU_ATM_VC_TBL, &simuEntry);
	}

	// convert pppx to nas0_x
	ifGetName(PHY_INTF(Entry.ifIndex), ifname, sizeof(ifname));
	snprintf(ifname_simu, IFNAMSIZ,  "%s_0", ifname);

	addSimuEthWANdev(&simuEntry, 0);
	setup_simu_ethernet_config(&simuEntry, ifname_simu);

	//Start to dial
	p->status = eSpeedTest_STAT_PPPOE_DIALING;
#ifdef CONFIG_USER_DBUS_PROXY
	if((p->source == esource_APP) || (p->source == esource_PLATFORM))
		setstatus_for_dbus(SIGNAL_STATUS);
#endif	
	idx += snprintf(&msg.data[idx], BUF_SIZE - idx, "spppctl add 7 ipt 0 pppoe %s", ifname_simu);
	if(pppusername && pppusername[0] != '\0')
	{
		if(p->pppoename)
			free(p->pppoename);
		p->pppoename = strdup(pppusername);
		idx += snprintf(&msg.data[idx], BUF_SIZE - idx, " username %s", pppusername);
	}
	if(ppppassword && ppppassword[0] != '\0')
	{
		if(is_spec_tianjin_enable())
		{
			if(p->pppoepasswd)
				free(p->pppoepasswd);
			p->pppoepasswd = strdup(ppppassword);
		}
		idx += snprintf(&msg.data[idx], BUF_SIZE - idx, " password %s", ppppassword);
	}
	idx += snprintf(&msg.data[idx], BUF_SIZE - idx, " gw 1");

	printf( "%s\n", msg.data);
	write_to_pppd(&msg);

	// Check ppp7 is up
	i = 0;
	while(getInAddr("ppp7", IP_ADDR, (void *)&addr) != 1)
	{
		sleep(1);
		i++;

		if(i >= 30)
		{
			cleanup_speedtest_interface();
#ifdef CONFIG_GPON_FEATURE
#ifdef CONFIG_RTK_HOST_SPEEDUP
			if (-1 != def_wan_idx)
			{
				mib_chain_get(MIB_ATM_VC_TBL, def_wan_idx, (void *)&Entry);
				set_speedup_usflow(&Entry);
			}
#endif
#endif
			return -1;
		}
	}
	if(p->wanip)
		free(p->wanip);
	p->wanip = strdup(inet_ntoa(addr));
	printf("%s:%d p->wanip is %s\n",__func__,__LINE__, p->wanip);
	strcpy(test_if, "ppp7");
#ifdef CONFIG_GPON_FEATURE
#ifdef CONFIG_RTK_HOST_SPEEDUP
    /* host speedup must assign stream id for quickly ack*/
    set_speedup_usflow(&simuEntry);
#endif
#endif
#endif

	printf( "Return 0\n");
	return 0;
}

#ifdef SUPPORT_UPLOAD_SPEEDTEST
#define SPEEDTEST_UPLOAD_RESULT "/tmp/speedtest_uploadresult"
static void parse_uploadtest_speedtest_results(struct CU_SpeedTest_Diagnostics*p)
{
	FILE *f;
	int cnt = 0;
	char line[1024] = {0};

	if(p == NULL)
		return;
again:
	f = fopen(SPEEDTEST_UPLOAD_RESULT, "r");
	if(f == NULL)
	{
		cnt++;
		usleep(500000);
		if(cnt > 3)
		{
			printf( "Open "SPEEDTEST_UPLOAD_RESULT" failed, give up\n");
			return;
		}
		else
		{
			printf( "Open "SPEEDTEST_UPLOAD_RESULT" failed, try again. cnt=%d\n", cnt);
			goto again;
		}
	}

	while(fgets(line, sizeof(line), f) != NULL)
	{
		char name[16] = {0};
		char value[128] = {0};
		sscanf(line, "%[^=]=%[^\n]", name, value);

		printf( "name=%s, value=%s\n", name ,value);

		if(strcmp(name, "bom_sec") == 0)
		{
			sscanf(value, "%ld", &p->starttime.tv_sec);
		}
		else if(strcmp(name, "bom_usec") == 0)
		{
			sscanf(value, "%ld", &p->starttime.tv_usec);
		}
		else if(strcmp(name, "TestBytesSent") == 0)
		{
			sscanf(value, "%lu", &p->Totalsize);
		}
		else if(strcmp(name, "eom_sec") == 0)
		{
			sscanf(value, "%ld", &p->endtime.tv_sec);
		}
		else if(strcmp(name, "eom_usec") == 0)
		{
			sscanf(value, "%ld", &p->endtime.tv_usec);
		}
		else if(strcmp(name, "TotalBytesSend") == 0)
		{
			sscanf(value, "%lu", &p->backgroundsize);
		}
		else if(strcmp(name, "Cspeed") == 0)
		{
			sscanf(value, "%u", &p->Cspeed);
		}
		else if(strcmp(name, "Aspeed") == 0)
		{
			sscanf(value, "%u", &p->Aspeed);
		}
		else if(strcmp(name, "Maxspeed") == 0)
		{
			sscanf(value, "%u", &p->maxspeed);
		}
		else
		{
			printf( "FIXME: unknown name: %s\n", name);
		}
	}

	if(p->endtime.tv_sec > 15){
		if((p->endtime.tv_sec - p->starttime.tv_sec) > 5)
			p->endtime.tv_sec = p->starttime.tv_sec + 15;
	}

	printf("%s:starttime=%ld,endtime=%ld,size=%lu,backgroup=%lu,Aspeed=%lu,maxspeed=%lu\n",
		__func__,p->starttime.tv_sec,p->endtime.tv_sec,p->Totalsize,p->backgroundsize,p->Aspeed,p->maxspeed);

	fclose(f);
}
#endif

#define CU_SPEED_RESULT_FILE "/tmp/CU_axel_result"
static void parse_axel_speedtest_results(struct CU_SpeedTest_Diagnostics*p)
{
	FILE *f;
	int cnt = 0;
	char line[1024] = {0};

	if(p == NULL)
		return;
again:
	f = fopen(CU_SPEED_RESULT_FILE, "r");
	if(f == NULL)
	{
		cnt++;
		usleep(500000);
		if(cnt > 3)
		{
			printf( "Open "CU_SPEED_RESULT_FILE" failed, give up\n");
			return;
		}
		else
		{
			printf( "Open "CU_SPEED_RESULT_FILE" failed, try again. cnt=%d\n", cnt);
			goto again;
	}
	}

	while(fgets(line, sizeof(line), f) != NULL)
	{
		char name[16] = {0};
		char value[128] = {0};
		sscanf(line, "%[^=]=%[^\n]", name, value);

		printf( "name=%s, value=%s\n", name ,value);

		if(strcmp(name, "result") == 0)
		{
			p->DiagnosticsState = atoi(value);
			if((p->DiagnosticsState < eSpeedTest_None) || (p->DiagnosticsState > eSpeedTest_Completed))
				p->DiagnosticsState = eSpeedTest_Completed;
		}
		else if(strcmp(name, "bom_sec") == 0)
		{
			sscanf(value, "%ld", &p->starttime.tv_sec);
		}
		else if(strcmp(name, "bom_usec") == 0)
		{
			sscanf(value, "%ld", &p->starttime.tv_usec);
		}
		else if(strcmp(name, "bytes_recv") == 0)
		{
			sscanf(value, "%lu", &p->Totalsize);
		}
		else if(strcmp(name, "eom_sec") == 0)
		{
			sscanf(value, "%ld", &p->endtime.tv_sec);
		}
		else if(strcmp(name, "eom_usec") == 0)
		{
			sscanf(value, "%ld", &p->endtime.tv_usec);
		}
		else if(strcmp(name, "total_bytes") == 0)
		{
			sscanf(value, "%lu", &p->backgroundsize);
		}
		else if(strcmp(name, "sampled_cnt") == 0)
		{
			int cnt = atoi(value);
			int i;
			unsigned long bytes_per_sec;
			unsigned long max_speed=0,bytes_five_sec=0,totalbytes_five_sec=0;
			unsigned long bytes_all=0;

			if(cnt > p->max_sampled)
				cnt = p->max_sampled;

			for(i = 0 ; i < cnt ; i++)
			{
				fgets(line, sizeof(line), f);
				sscanf(line, "%lu", &bytes_per_sec);
				if(p->SampledValues){
					p->SampledValues[i] = bytes_per_sec;
					if(i < 5)
						bytes_five_sec += bytes_per_sec;

					if(max_speed < bytes_per_sec)
						max_speed = bytes_per_sec;
					bytes_all += bytes_per_sec;
				}

				fgets(line, sizeof(line), f);
				sscanf(line, "%lu", &bytes_per_sec);
				if(p->SampledTotalValues){
					p->SampledTotalValues[i] = bytes_per_sec;
					if(i < 5)
						totalbytes_five_sec += bytes_per_sec;
				}
			}

			printf("%s:Totalsize=%ld,bytes_five_sec=%ld,backgroundsize=%ld,totalbytes_five_sec=%ld\n",
				__func__,p->Totalsize,bytes_five_sec,p->backgroundsize,totalbytes_five_sec);

			p->maxspeed = max_speed;
			p->Cspeed = bytes_per_sec;
			if(p->backgroundsize > p->Totalsize)
				p->backgroundsize -= p->Totalsize;
			else
				p->backgroundsize = 0;

			if(cnt >= 5)
				p->Totalsize = bytes_all;

			printf("%s:Totalsize=%ld\n",
				__func__,p->Totalsize);
			
			if(cnt >= 5){
				if(p->Totalsize > bytes_five_sec)
					p->Totalsize -= bytes_five_sec;
				else
					p->Totalsize = 0;
			}
//			else
//				p->Totalsize = 0;

			double num;
			if(cnt <  p->max_sampled)
			{
				cnt = p->endtime.tv_sec - p->starttime.tv_sec;
				num = (double)cnt + (double)(p->endtime.tv_usec- p->starttime.tv_usec) / 1000000;
				p->Aspeed = (p->Totalsize/(num>5?(double)(num-5):num));
			}
			else
			{
				p->Aspeed = (p->Totalsize/(cnt>5?(cnt-5):cnt));
			}
			
			if(p->Aspeed >= p->maxspeed)
				p->Aspeed = p->maxspeed - 1000000;

//			p->Cspeed = 0;
			break;
		}
		else
		{
			printf( "FIXME: unknown name: %s\n", name);
		}
	}

	if(p->endtime.tv_sec > 15){
		if((p->endtime.tv_sec - p->starttime.tv_sec) > 5)
			p->endtime.tv_sec = p->starttime.tv_sec + 15;
	}

	printf("%s:starttime=%ld,endtime=%ld,size=%lu,backgroup=%lu,Aspeed=%u,maxspeed=%u\n",
		__func__,p->starttime.tv_sec,p->endtime.tv_sec,p->Totalsize,p->backgroundsize,p->Aspeed,p->maxspeed);

	fclose(f);
}

int ReportTestSpeedResult(struct CU_SpeedTest_Diagnostics *pCU_SpeedTestDiagnostics)
{
	int ifindex=-1;
	char wanitf[256]={0};
	char source[32]={0};
	char percent[32]={0}, aspeed[32]={0}, bspeed[32]={0};
	int speed=0, curl_status;
	char result[1024]={0};
	char obj[1024] = {0};  
	char reportResult[1024]={0};
	char starttime[32]={0}, endtime[32]={0}, totalsize[32]={0}, maxspeed[32]={0};
	unsigned int srclength;
	sighandler_t old_handler;
	
	if(!pCU_SpeedTestDiagnostics->preportURL)
		return -1;

	cJSON *otherPara = cJSON_CreateObject();

	if(pCU_SpeedTestDiagnostics->pppoename)
	{
		cJSON_AddStringToObject(otherPara,"name", pCU_SpeedTestDiagnostics->pppoename);
	}
	else
	{
		cJSON_AddStringToObject(otherPara,"name", "");
	}

	ifindex = getifIndexByWanName(pCU_SpeedTestDiagnostics->IfName);
	transfer2WANterface( ifindex, wanitf );
	cJSON_AddStringToObject(otherPara,"WANInterface", wanitf);

	sprintf(aspeed,"%.2f",((double)pCU_SpeedTestDiagnostics->Aspeed*8)/(1024.0*1024.0));
	cJSON_AddStringToObject(otherPara,"average", aspeed);

	cJSON_AddStringToObject(otherPara,"ip", pCU_SpeedTestDiagnostics->wanip);

	sprintf(bspeed,"%dM",(pCU_SpeedTestDiagnostics->Bspeed)/(1024*1024));
	cJSON_AddStringToObject(otherPara,"speed", bspeed);

	getTimeFormat(&(pCU_SpeedTestDiagnostics->starttime), starttime);
	cJSON_AddStringToObject(otherPara,"startTime", starttime);

	getTimeFormat(&(pCU_SpeedTestDiagnostics->endtime), endtime);
	cJSON_AddStringToObject(otherPara,"stopTime", endtime);

	sprintf(maxspeed,"%.2f",((double)pCU_SpeedTestDiagnostics->maxspeed*8)/(1024.0*1024.0));
	cJSON_AddStringToObject(otherPara,"maxSpeed", maxspeed);

	sprintf(totalsize,"%.2f",((double)pCU_SpeedTestDiagnostics->Totalsize)/1024.0);
	cJSON_AddStringToObject(otherPara,"downfilelarge", totalsize);

	if(pCU_SpeedTestDiagnostics->Bspeed)
		sprintf(percent,"%.2f",((double)pCU_SpeedTestDiagnostics->Aspeed*8)*100/(pCU_SpeedTestDiagnostics->Bspeed));
	cJSON_AddStringToObject(otherPara,"percent", percent);

	if(pCU_SpeedTestDiagnostics->source == esource_APP)
		strcpy(source, "APP");
	else if(pCU_SpeedTestDiagnostics->source == esource_PLATFORM)
		strcpy(source, "Platform");
	cJSON_AddStringToObject(otherPara,"Source", source);

	if(pCU_SpeedTestDiagnostics->identify)
		cJSON_AddStringToObject(otherPara,"identify", pCU_SpeedTestDiagnostics->identify);
	else
		cJSON_AddStringToObject(otherPara,"identify", "");

	strcpy(reportResult, cJSON_PrintUnformatted(otherPara));
	srclength = strlen(reportResult);  
    	URLEncode(reportResult, srclength, obj, 1024); 

	sprintf(result, "%s?info=%s", pCU_SpeedTestDiagnostics->preportURL, obj);
	cJSON_Delete(otherPara);
	if(pCU_SpeedTestDiagnostics->newwanflag)
	{
		printf("%s:%d /bin/curl -X GET %s --connect-timeout 15 --retry 3 --interface %s -o %s\n",__func__,__LINE__,result ,pCU_SpeedTestDiagnostics->test_if, CURL_REPORT_SPEED_RESULT_FILE);
		old_handler = signal(SIGCHLD, SIG_DFL);
		curl_status=va_cmd("/bin/curl", 11, 1, "-X", "GET", result, "--connect-timeout", "15", "--retry", "3", "--interface", pCU_SpeedTestDiagnostics->test_if , "-o", CURL_REPORT_SPEED_RESULT_FILE);	
		signal(SIGCHLD, old_handler);
	}
	else
	{
		printf("%s:%d /bin/curl -X GET %s --connect-timeout 15 --retry 3 -o %s\n",__func__,__LINE__,result, CURL_REPORT_SPEED_RESULT_FILE);
		old_handler = signal(SIGCHLD, SIG_DFL);
		curl_status=va_cmd("/bin/curl", 9, 1, "-X", "GET", result, "--connect-timeout", "15", "--retry", "3", "-o", CURL_REPORT_SPEED_RESULT_FILE);	
		signal(SIGCHLD, old_handler);
	}
	printf("%s:%d curl_status is %d\n",__func__,__LINE__,curl_status );
	return curl_status;
}

void *CU_SpeedTestAxelThread(void *data)
{
	struct CU_SpeedTest_Diagnostics *p=data;
	if(!data) return NULL;
	int ret = 1;
	int timer = 0;
	unsigned long cur_tx_bytes = 0, cur_rx_bytes = 0;
	unsigned char auto_create_wan = 1;
	char str_max_sampled[8] = "0";
	char *testURL;
	int shm_id;
	unsigned 	int *pCspeed=NULL;
	unsigned int Cspeed=0;
	MIB_CE_ATM_VC_T wanentry;
	unsigned char cu_speedtest_thread_number=0;
	unsigned int gpon_speed=0;	
	char buf[256]={0};
	char str_thread_num[8] = "0";
	
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE) || defined(CONFIG_FIBER_FEATURE)	
	mib_get_s(MIB_PON_SPEED, (void *)&gpon_speed, sizeof(gpon_speed));
#endif	

	sprintf(str_max_sampled, "%d", p->max_sampled);
	unlink(CU_SPEED_RESULT_FILE);
	mib_get(PROVINCE_CU_SPEEDTEST_THREAD_NUMBER , (void *)&cu_speedtest_thread_number);		
	sprintf(str_thread_num, "%d", cu_speedtest_thread_number);

	// /bin/axel -n 10 -Z -q -o /tmp http://192.168.1.100:80/iqiyi.pgf &
	printf("%s:%d p->newwanflag is %d\n",__func__,__LINE__,p->newwanflag );
	if(p->newwanflag)
	{
		//printf( "/bin/axel -n 4 -y -q -o /tmp -t %s -i %s %s\n", str_max_sampled, test_if, p->prealURL);
		//va_cmd("/bin/axel", 11, 0, "-n", "4", "-y", "-q", "-o", "/tmp", "-t", str_max_sampled, "-i", test_if, p->prealURL);
		//newwanflag = 1;
#ifdef SUPPORT_UPLOAD_SPEEDTEST		
		if(p->testDirection == eUplink)
		{
			printf( "/bin/uploadTest -n %s -o /tmp/speedtest_uploadresult -t %s -i %s %s\n", str_thread_num, str_max_sampled, p->test_if, p->prealURL);
			va_cmd("/bin/uploadTest", 10, 0, "-n", str_thread_num, "-o", SPEEDTEST_UPLOAD_RESULT, "-t", str_max_sampled, "-d", "-i", p->test_if, p->prealURL);		
		}
		else
#endif		
		{
			if(gpon_speed == 1)
			{
				sprintf(buf,"nice -n -20 axel -n %s -y -q -o /tmp -t %s -i %s %s", str_thread_num, str_max_sampled, p->test_if, p->prealURL);
				printf("%s\n", buf);
				system(buf);
			}
			else
			{
				printf( "/bin/axel -n %s -y -q -o /tmp -t %s -i %s %s\n", str_thread_num, str_max_sampled, p->test_if, p->prealURL);
				va_cmd("/bin/axel", 11, 0, "-n", str_thread_num, "-y", "-q", "-o", "/tmp", "-t", str_max_sampled, "-i", p->test_if, p->prealURL);
			}
		}
	}
	else
	{
#ifdef SUPPORT_UPLOAD_SPEEDTEST	
		if(p->testDirection == eUplink)
		{
			printf( "/bin/uploadTest -n %s -o /tmp/speedtest_uploadresult -t %s %s\n", str_thread_num, str_max_sampled, p->prealURL);
			va_cmd("/bin/uploadTest", 8, 0, "-n", str_thread_num, "-o", SPEEDTEST_UPLOAD_RESULT, "-t", str_max_sampled, "-d", p->prealURL);
		}
		else
#endif	
		{
			if(gpon_speed == 1)
			{
				sprintf(buf, "nice -n -20 axel -n %s -y -q -o /tmp -t %s %s", str_thread_num, str_max_sampled, p->prealURL);
				printf("%s\n", buf);
				system(buf);
			}
			else
			{
				printf( "/bin/axel -n %s -y -q -o /tmp -t %s %s\n", str_thread_num, str_max_sampled, p->prealURL);
				va_cmd("/bin/axel", 9, 0, "-n", str_thread_num, "-y", "-q", "-o", "/tmp", "-t", str_max_sampled, p->prealURL);
			}
		}
	}

	p->status = eSpeedTest_STAT_SPEED_TESTING;
#ifdef CONFIG_USER_DBUS_PROXY
	if((p->source == esource_APP) || (p->source == esource_PLATFORM))
		setstatus_for_dbus(SIGNAL_STATUS);
#endif
	shm_id = shmget((key_t)0x5252, sizeof(unsigned int), 0644 | IPC_CREAT);
	if(shm_id != -1){
		pCspeed = (unsigned int *)shmat ( shm_id , NULL , 0 );
		if(pCspeed == (unsigned int *)(-1))
			pCspeed = NULL;
	}

	printf( "p->max_sampled=%d\n", p->max_sampled);
	while(access(CU_SPEED_RESULT_FILE, F_OK) != 0)
	{
		sleep(1);
		timer++;
#if 1
		if(getrealrate(&Cspeed)==0){
			Cspeed = Cspeed*1024/8;
			printf("\nCspeed=%d\n",Cspeed);
			if(Cspeed > 0)
				p->Cspeed = Cspeed;
		}
#else
		if(pCspeed){
			if(*pCspeed > 0)
				p->Cspeed = *pCspeed;
		}else
			p->Cspeed = 0;
#endif

		// 30 is addtional timeout
		if(p->max_sampled > 0 && timer >= p->max_sampled + 30)
		{
			printf( "Max sampled count reached\n");
			break;
		}
	}

	shmdt(pCspeed);
	shmctl(shm_id, IPC_RMID, NULL);
	
#ifdef SUPPORT_UPLOAD_SPEEDTEST
	if(p->testDirection == eUplink)
	{
		system("/bin/killall uploadTest");
		parse_uploadtest_speedtest_results(p);
	}
	else
#endif		
	{
		system("/bin/killall axel");
		parse_axel_speedtest_results(p);
	}

	if(p->source == esource_RMS)
		setCWMPSpeedtestDoneFlag(1);
	else
		setSpeedtestDoneFlag(1);
	p->http_pid=0;
	
	printf("%s:%d p->newwanflag is %d\n",__func__,__LINE__,p->newwanflag );

	if(p->newwanflag){
		//p->wanip has got in get_or_create_speedtest_interface()
		if(!p->pppoename){
			if(getATMVCEntryByIfName(p->IfName, &wanentry)){
				if(wanentry.cmode == CHANNEL_MODE_PPPOE)
				{
					p->pppoename = strdup(wanentry.pppUsername);
					if(is_spec_tianjin_enable())
						p->pppoepasswd= strdup(wanentry.pppPassword);
				}
			}
		}
	}else{
		if(!p->wanip || !p->pppoename){
			if(getDefaultRouteATMVCEntry(&wanentry)){
				if(!p->wanip){
					char ifname[32];
					struct in_addr addr;
					ifGetName(wanentry.ifIndex, ifname, sizeof(ifname));
					if(getInAddr(ifname, IP_ADDR, (void *)&addr) == 1)
					{
						p->wanip = strdup(inet_ntoa(addr));
					}
				}
				if(!p->pppoename){
					if(wanentry.cmode == CHANNEL_MODE_PPPOE)
					{
						p->pppoename = strdup(wanentry.pppUsername);
						if(is_spec_tianjin_enable())
							p->pppoepasswd = strdup(wanentry.pppPassword);
					}
				}
			}
		}
	}

	if(p->testMode == eserverMode)
	{
		p->status = eSpeedTest_STAT_UPLOAD_TEST_RESULT;
#ifdef CONFIG_USER_DBUS_PROXY
		if((p->source == esource_APP) || (p->source == esource_PLATFORM))
			setstatus_for_dbus(SIGNAL_STATUS);
#endif
		if(ReportTestSpeedResult(p))
			p->Failcode = 3;
		p->status = eSpeedTest_STAT_TEST_FINISHED;
#ifdef CONFIG_USER_DBUS_PROXY
		if((p->source == esource_APP) || (p->source == esource_PLATFORM))
			setstatus_for_dbus(SIGNAL_STATUS);
#endif
	}
	else if(p->testMode == edownloadMode)
	{
		p->status = eSpeedTest_STAT_TEST_FINISHED;
#ifdef CONFIG_USER_DBUS_PROXY
		if((p->source == esource_APP) || (p->source == esource_PLATFORM))
			setstatus_for_dbus(SIGNAL_STATUS);
#endif
	}
#ifdef SUPPORT_UPLOAD_SPEEDTEST	
	else if(p->testMode == euploadMode)
	{
		p->status = eSpeedTest_STAT_TEST_FINISHED;
#ifdef CONFIG_USER_DBUS_PROXY
		if((p->source == esource_APP) || (p->source == esource_PLATFORM))
			setstatus_for_dbus(SIGNAL_STATUS);
#endif
	}
#endif	

	if(p->DiagnosticsState == eSpeedTest_Requested)
		p->DiagnosticsState = eSpeedTest_Completed;

#ifdef CONFIG_SUPPORT_AUTO_DIAG
#ifdef CONFIG_RTK_HOST_SPEEDUP
	if(auto_create_wan)
		cleanup_speedtest_interface();
#endif
#endif
#ifdef CONFIG_GPON_FEATURE
#ifdef CONFIG_RTK_HOST_SPEEDUP
	clear_speedup_usflow();
#endif
#endif	

	writeSpeedTestFile(p);
#ifdef CONFIG_USER_DBUS_PROXY
	if((p->source == esource_APP) || (p->source == esource_PLATFORM))
		setstatus_for_dbus(SIGNAL_FINISH);
#endif	
	return NULL;
}

void StartCU_SpeedTestDiagAxel(struct CU_SpeedTest_Diagnostics *pCU_SpeedTestDiagnostics)
{
	pthread_t pid;

	if( pthread_create( &pid, NULL, CU_SpeedTestAxelThread, pCU_SpeedTestDiagnostics) != 0 )
	{
		pCU_SpeedTestDiagnostics->DiagnosticsState=eSpeedTest_Completed;
		if(pCU_SpeedTestDiagnostics->source == esource_RMS)
			setCWMPSpeedtestDoneFlag(1);
		else
			setSpeedtestDoneFlag(1);
		return;
	}
	pCU_SpeedTestDiagnostics->http_pid=pid;
	pthread_detach(pid);
	return;
}

int RequestSpeedDownloadURL(struct CU_SpeedTest_Diagnostics *pCU_SpeedTestDiagnostics)
{
	int diatype=1, mode=0;
	char diatypestr[32]={0}, wanip[32] = {0}, mac[32]={0};
	char requeststring[512]={0}, obj[512]={0};
	char softver[64] = {0};
	char hwver[64] = {0};
	char productclass[64] =  {0};
	char request[512]={0};
	unsigned char macAddr[MAC_ADDR_LEN];
	unsigned char pppUsername[MAX_PPP_NAME_LEN+1]={0};
	MIB_CE_ATM_VC_T wanentry;
	unsigned int srclength;
	int curl_status;
	sighandler_t old_handler;
	unsigned char cu_speedtestdiag_new_spec_enable=1;
#if defined(CONFIG_CU)
	mib_get_s(MIB_CU_SPEEDTESTDIAG_NEW_SPEC_ENABLE, &cu_speedtestdiag_new_spec_enable, sizeof(cu_speedtestdiag_new_spec_enable));
#endif

	if(!pCU_SpeedTestDiagnostics->ptestURL)
		return -1;

	if((pCU_SpeedTestDiagnostics->Eupppoename) && strlen(pCU_SpeedTestDiagnostics->Eupppoename))
		diatype = 0;

	sprintf(diatypestr, "%d", diatype);
	
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)|| defined(CONFIG_FIBER_FEATURE)

	mib_get_s(MIB_PON_MODE, &mode, sizeof(mode));
	
	if(mode == GPON_MODE)
	{
		getMIB2Str(MIB_GPON_SN, mac);
	}
	else
#endif
	{
		mib_get_s( MIB_ELAN_MAC_ADDR, (void *)macAddr, sizeof(macAddr));
		snprintf (mac, sizeof(mac), "%02X-%02X-%02X-%02X-%02X-%02X", macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]);	
	}
	getSYS2Str(SYS_FWVERSION, softver);
	mib_get_s(MIB_HW_HWVER, hwver, sizeof(hwver));
	mib_get_s( MIB_HW_CWMP_PRODUCTCLASS, productclass, sizeof(productclass));
	
	cJSON *otherPara = cJSON_CreateObject();

	if(is_spec_tianjin_enable())
	{
		if(pCU_SpeedTestDiagnostics->pppoename)
			cJSON_AddStringToObject(otherPara,"name", pCU_SpeedTestDiagnostics->pppoename);
		else
			cJSON_AddStringToObject(otherPara,"name", "");

		if(pCU_SpeedTestDiagnostics->pppoepasswd)
			cJSON_AddStringToObject(otherPara,"password", pCU_SpeedTestDiagnostics->pppoepasswd);
		else
			cJSON_AddStringToObject(otherPara,"password", "");

		cJSON_AddStringToObject(otherPara, "diatype", diatypestr); 
	}
	else
	{
		if(pCU_SpeedTestDiagnostics->pppoename)
			cJSON_AddStringToObject(otherPara,"name", pCU_SpeedTestDiagnostics->pppoename);
		else
			cJSON_AddStringToObject(otherPara,"name", "");
		if(pCU_SpeedTestDiagnostics->wanip)
			cJSON_AddStringToObject(otherPara, "ip", pCU_SpeedTestDiagnostics->wanip); 
		else
			cJSON_AddStringToObject(otherPara, "ip", ""); 
		cJSON_AddStringToObject(otherPara, "diatype", diatypestr); 
		cJSON_AddStringToObject(otherPara, "mac", mac); 
		cJSON_AddStringToObject(otherPara, "hardwareversion", hwver); 
		cJSON_AddStringToObject(otherPara, "softwareversion", softver); 
		cJSON_AddStringToObject(otherPara, "productclass", productclass);
		if(cu_speedtestdiag_new_spec_enable)
		{
#ifdef SUPPORT_UPLOAD_SPEEDTEST		
			if(pCU_SpeedTestDiagnostics->testDirection==eUplink)
				cJSON_AddStringToObject(otherPara, "type", "UpLink");
			else
#endif				
				cJSON_AddStringToObject(otherPara, "type", "DownLink");
		}
	}

	strcpy(requeststring, cJSON_PrintUnformatted(otherPara));
	srclength = strlen(requeststring);  
    	URLEncode(requeststring, srclength, obj, 512); 

	sprintf(request, "%s?info=%s", pCU_SpeedTestDiagnostics->ptestURL, obj);

	cJSON_Delete(otherPara);

	if(pCU_SpeedTestDiagnostics->newwanflag)
	{
		printf("%s:%d /bin/curl -X GET %s --connect-timeout 15 --retry 3 --interface %s --max-filesize 2048 -o %s\n",__func__,__LINE__,request , pCU_SpeedTestDiagnostics->test_if, CURL_GET_SPEED_RESULT_FILE);
		old_handler = signal(SIGCHLD, SIG_DFL);
		curl_status=va_cmd("/bin/curl", 13, 1, "-X", "GET", request, "--connect-timeout", "15", "--retry", "3", "--interface", pCU_SpeedTestDiagnostics->test_if ,"--max-filesize", "2048", "-o", CURL_GET_SPEED_RESULT_FILE);
		signal(SIGCHLD, old_handler);
	}
	else
	{
		printf("%s:%d /bin/curl -X GET %s --connect-timeout 15 --retry 3 --max-filesize 2048 -o %s\n",__func__,__LINE__,request , CURL_GET_SPEED_RESULT_FILE);
		old_handler = signal(SIGCHLD, SIG_DFL);
		curl_status=va_cmd("/bin/curl", 11, 1, "-X", "GET", request, "--connect-timeout", "15", "--retry", "3", "--max-filesize", "2048", "-o", CURL_GET_SPEED_RESULT_FILE);
		signal(SIGCHLD, old_handler);
	}
	printf("%s:%d curl_status is %d\n",__func__,__LINE__,curl_status );
	return curl_status;

}

int parse_curl_get_results(char* download, char *ip, char *port, struct CU_SpeedTest_Diagnostics *pCU_SpeedTestDiagnostics)
{
	int speed, ret;
	FILE *f;
	char *code=NULL;
	char *speedstring=NULL;
	char *identify=NULL;
	char *threadcount=NULL;
	char result[256]={0};
	cJSON* result_json=NULL;
	cJSON* temp=NULL;
       char buf[4096]={0};
	char starttime[32]={0};   

	f = fopen(CURL_GET_SPEED_RESULT_FILE, "r");
	if (f == NULL)
	{
		fprintf(stderr,
			"FAIL: unable to open %s: %s\n",
			CURL_GET_SPEED_RESULT_FILE, strerror(errno));
	}else{
#if 0
		strcpy(result, "{\"speed\":\"200M\",\"download\":\"Dat/1G.rar\",\"identify\":\"17b82539-052a-43dc-9550-246f591f2121\",\"ip\":\"112.225.241.62\",\"port\":\"8080\"}");
		result_json = cJSON_Parse(result);
#else
		ret=fread(buf, 1, 4096, f);
		result_json = cJSON_Parse(buf);
#endif
		if(!result_json){
			fclose(f);
			return -1;
		}
		
		//record error code
		temp = cJSON_GetObjectItem(result_json, "code");
		if(temp)
		{
			code = temp->valuestring;
		}
		
		if(code)
		{
			int codeval;
			sscanf(code,"%d",&codeval);
			printf("parse_curl_get_results:return code %d\n",codeval);
			if(codeval >= 400){
				switch(codeval){
					case 400:
						pCU_SpeedTestDiagnostics->FailReason = eSpeedTest_FailReason_Req_Format_Error;
						break;
					case 421:
						pCU_SpeedTestDiagnostics->FailReason = eSpeedTest_FailReason_Frame_Format_Error;
						break;
					case 422:
						pCU_SpeedTestDiagnostics->FailReason = eSpeedTest_FailReason_User_Password_Error;
						break;
					case 423:
						pCU_SpeedTestDiagnostics->FailReason = eSpeedTest_FailReason_Req_Timeout;
						break;
					case 424:
						pCU_SpeedTestDiagnostics->FailReason = eSpeedTest_FailReason_File_Cant_Download;
						break;
					case 425:
						pCU_SpeedTestDiagnostics->FailReason = eSpeedTest_FailReason_Upload_data_error;
						break;
					default:
						break;
				}
			}
			pCU_SpeedTestDiagnostics->Failcode = codeval;
			return -1;
		}
		else
		{
			temp = cJSON_GetObjectItem(result_json, "speed");
			if(temp && temp->valuestring)
				speedstring = temp->valuestring;
	
			if(speedstring)
			{
				sscanf(speedstring , "%dM", &speed);
				pCU_SpeedTestDiagnostics->Bspeed= speed*1024*1024;
			}

			temp = cJSON_GetObjectItem(result_json, "identify");
			if(temp)
				identify = temp->valuestring;
			if(identify)
				pCU_SpeedTestDiagnostics->identify = strdup(identify);

			temp = cJSON_GetObjectItem(result_json, "download");
			if(temp && temp->valuestring)
				strcpy(download, temp->valuestring);

			if(is_spec_tianjin_enable())
			{
				temp = cJSON_GetObjectItem(result_json, "starttime");
				if(temp && temp->valuestring)
					strcpy(starttime, temp->valuestring);
			}
			else
			{
				temp = cJSON_GetObjectItem(result_json, "threadcount");
				if(temp)
					threadcount = temp->valuestring;

				temp = cJSON_GetObjectItem(result_json, "ip");
				if(temp && temp->valuestring)
					strcpy(ip, temp->valuestring);

				temp = cJSON_GetObjectItem(result_json, "port");
				if(temp && temp->valuestring)
					strcpy(port, temp->valuestring);
			}

		}

		fclose(f);
	}

	cJSON_Delete(result_json);
	return 0;
}

int parse_testurl(char *url, char *ip, char *port)
{
	char urlTmp[256]={0};
	char *pTmp=NULL;

	if((url == NULL) || (ip == NULL) || (port == NULL) || (strlen(url) == 0))
		return 0;

	if (!strncmp(url, "http://", 7))
		strcpy(urlTmp, url+7);
	else
		strcpy(urlTmp, url);

	pTmp = strtok(urlTmp, "/"); //urlTmp  ip:port or ip
	if(strchr(urlTmp, ':')){  
		pTmp = strtok(urlTmp, ":");
		strcpy(ip, pTmp);
		pTmp = strtok(NULL, ":");
		if(pTmp)
			strcpy(port, pTmp);
	}
	else{  // if port not exist in url, delim char is /
		strcpy(ip, pTmp);
		strcpy(port, "80");
	}

	return 1;
}

void StartCU_SpeedTestDiag_General(struct CU_SpeedTest_Diagnostics *pCU_SpeedTestDiagnostics)
{
	char download[64]={0}, ip[32]={0}, port[32]={0};
	char testurl_ip[32]={0}, testurl_port[32]={0}; 
	char realDownloadUrl[256]={0};
	int curl_status;
	unsigned char auto_create_wan = 1;
	char test_if[IFNAMSIZ] = {0};
	int ret = 0;
	
	StopCU_SpeedTestDiag(pCU_SpeedTestDiagnostics);
	ResetCU_SpeedTestResultValues(pCU_SpeedTestDiagnostics);

	ret = get_or_create_speedtest_interface(pCU_SpeedTestDiagnostics, auto_create_wan, test_if);
	if( ret == 0)
	{
		pCU_SpeedTestDiagnostics->newwanflag = 1;
		pCU_SpeedTestDiagnostics->test_if = strdup(test_if);
	}else if(ret == -2){
		pCU_SpeedTestDiagnostics->DiagnosticsState = eSpeedTest_Completed;
		pCU_SpeedTestDiagnostics->status = eSpeedTest_STAT_TEST_FINISHED;
#ifdef CONFIG_USER_DBUS_PROXY
		if((pCU_SpeedTestDiagnostics->source == esource_APP) || (pCU_SpeedTestDiagnostics->source == esource_PLATFORM))
			setstatus_for_dbus(SIGNAL_STATUS);
#endif
		pCU_SpeedTestDiagnostics->Failcode = 4;
		goto FIND_ERR;
	}
	
	if(pCU_SpeedTestDiagnostics->testMode == eserverMode){
		curl_status= RequestSpeedDownloadURL(pCU_SpeedTestDiagnostics);
		pCU_SpeedTestDiagnostics->status = eSpeedTest_STAT_SERVER_CONNECTING;
#ifdef CONFIG_USER_DBUS_PROXY
		if((pCU_SpeedTestDiagnostics->source == esource_APP) || (pCU_SpeedTestDiagnostics->source == esource_PLATFORM))
			setstatus_for_dbus(SIGNAL_STATUS);
#endif
		if(curl_status)
		{
			pCU_SpeedTestDiagnostics->DiagnosticsState = eSpeedTest_Completed;
			pCU_SpeedTestDiagnostics->status = eSpeedTest_STAT_TEST_FINISHED;
#ifdef CONFIG_USER_DBUS_PROXY
			if((pCU_SpeedTestDiagnostics->source == esource_APP) || (pCU_SpeedTestDiagnostics->source == esource_PLATFORM))
				setstatus_for_dbus(SIGNAL_STATUS);
#endif
			pCU_SpeedTestDiagnostics->Failcode = 1;
			goto FIND_ERR;
		}
		curl_status=parse_curl_get_results(download, ip, port, pCU_SpeedTestDiagnostics);
		if(download[0] == '/')
		{
			// strip lead '/'
			char *tmp = strdup(&download[1]);
			if(tmp)
			{
				strcpy(download, tmp);
				free(tmp);
			}
		}
		if(curl_status)
		{
			pCU_SpeedTestDiagnostics->DiagnosticsState = eSpeedTest_Completed;
			pCU_SpeedTestDiagnostics->status = eSpeedTest_STAT_TEST_FINISHED;
#ifdef CONFIG_USER_DBUS_PROXY
			if((pCU_SpeedTestDiagnostics->source == esource_APP) || (pCU_SpeedTestDiagnostics->source == esource_PLATFORM))
				setstatus_for_dbus(SIGNAL_STATUS);
#endif
			if(!pCU_SpeedTestDiagnostics->Failcode)
				pCU_SpeedTestDiagnostics->Failcode = 1;
			goto FIND_ERR;
		}

		if(pCU_SpeedTestDiagnostics->ptestURL)
		{
			parse_testurl(pCU_SpeedTestDiagnostics->ptestURL, testurl_ip, testurl_port);
		}
	
		if(strlen(ip) == 0)
			strcpy(ip, testurl_ip);
		if(strlen(port) == 0)
			strcpy(port, testurl_port);
		
		if(strlen(ip) && strlen(port)){
			pCU_SpeedTestDiagnostics->status = eSpeedTest_STAT_SERVER_CONNECTED;
#ifdef CONFIG_USER_DBUS_PROXY
			if((pCU_SpeedTestDiagnostics->source == esource_APP) || (pCU_SpeedTestDiagnostics->source == esource_PLATFORM))
				setstatus_for_dbus(SIGNAL_STATUS);
#endif
		}
		if(pCU_SpeedTestDiagnostics->identify)
			sprintf(realDownloadUrl, "http://%s:%s/%s?identify=%s", ip, port, download, pCU_SpeedTestDiagnostics->identify);
		else
			sprintf(realDownloadUrl, "http://%s:%s/%s", ip, port, download);
		pCU_SpeedTestDiagnostics->prealURL = strdup(realDownloadUrl);
#ifdef SUPPORT_UPLOAD_SPEEDTEST			
	}else if(pCU_SpeedTestDiagnostics->testMode == euploadMode){
		if(pCU_SpeedTestDiagnostics->puploadURL)
			pCU_SpeedTestDiagnostics->prealURL = strdup(pCU_SpeedTestDiagnostics->puploadURL);
#endif	
	}else{
		if(pCU_SpeedTestDiagnostics->pdownloadURL)
			pCU_SpeedTestDiagnostics->prealURL = strdup(pCU_SpeedTestDiagnostics->pdownloadURL);
	}
	
	if(!pCU_SpeedTestDiagnostics->prealURL)
		goto FIND_ERR;
	
	printf("%s:%d pCU_SpeedTestDiagnostics->prealURL is %s\n",__func__,__LINE__,pCU_SpeedTestDiagnostics->prealURL );
#if defined(CONFIG_E8B) && defined(CONFIG_USER_AXEL)
	StartCU_SpeedTestDiagAxel(pCU_SpeedTestDiagnostics);

	if(pCU_SpeedTestDiagnostics->source == esource_RMS)
	{
		while(pCU_SpeedTestDiagnostics->DiagnosticsState != eSpeedTest_Completed)
		{
			sleep(1);
		}
	}
	return;
#endif

FIND_ERR:
	if(pCU_SpeedTestDiagnostics->source == esource_RMS)
		setCWMPSpeedtestDoneFlag(1);
	else
		setSpeedtestDoneFlag(1);
#ifdef CONFIG_SUPPORT_AUTO_DIAG
#ifdef CONFIG_RTK_HOST_SPEEDUP
	cleanup_speedtest_interface();
#endif
#endif
#ifdef CONFIG_GPON_FEATURE
#ifdef CONFIG_RTK_HOST_SPEEDUP
	clear_speedup_usflow();
#endif
#endif
	writeSpeedTestFile(pCU_SpeedTestDiagnostics);
#ifdef CONFIG_USER_DBUS_PROXY
	if((pCU_SpeedTestDiagnostics->source == esource_APP) || (pCU_SpeedTestDiagnostics->source == esource_PLATFORM))
		setstatus_for_dbus(SIGNAL_FINISH);
#endif
	return;
}

#endif

#endif

#ifdef CONFIG_CU
void cuPPPoEManual_Action_DisconOrCon(MIB_CE_ATM_VC_T vcEntry)
{
    char infname[IFNAMSIZ]={0};
    char pppoetmpfile[128] = {0};
    char buff[256]={0};
    struct data_to_pass_st msg;
    FILE *fp1;
    FILE *fp;
    char buf[64];
    int pppoeupdownflag = -1;
    unsigned int i,flag, inf;

#ifdef CONFIG_USER_PPPOMODEM
    unsigned int cflag[MAX_PPP_NUM+MAX_MODEM_PPPNUM]={0};
#else
    unsigned int cflag[MAX_PPP_NUM]={0};
#endif //CONFIG_USER_PPPOMODEM

    if(vcEntry.pppCtype == MANUAL){
        ifGetName( vcEntry.ifIndex, infname, sizeof(infname));
        sprintf(pppoetmpfile, "/tmp/%s_ManualStatus", infname);

        fp1=fopen(pppoetmpfile,"r");
        if(fp1)
        {
            if(fgets(buf,64,fp1))
            {
                sscanf(buf,"%d\n",&pppoeupdownflag);
            }

            printf("%s:pppoe[%s] up or down:%d\n",__func__,infname,pppoeupdownflag);
            fclose(fp1);


            // Added by Jenny, for PPP connecting/disconnecting
#ifdef CONFIG_USER_PPPOMODEM
            for (i=0; i<(MAX_PPP_NUM+MAX_MODEM_PPPNUM); i++)
#else
                for (i=0; i<MAX_PPP_NUM; i++)
#endif //CONFIG_USER_PPPOMODEM
                {
                    char tmp[15], tp[10];

                    sprintf(tp, "ppp%d", i);
					if(strcmp(tp,infname))
						continue;
                    if (find_ppp_from_conf(tp)) {
                        if (fp=fopen("/tmp/ppp_up_log", "r")) {
                            while ( fgets(buff, sizeof(buff), fp) != NULL ) {
                                if(sscanf(buff, "%d %d", &inf, &flag) != 2)
                                    break;
                                else {
                                    if (inf == i)
                                        cflag[i] = flag;
                                }
                            }
                            fclose(fp);
                        }

                        if (pppoeupdownflag == 1)
                        {
                            if (!cflag[i]) {
                                snprintf(msg.data, BUF_SIZE, "spppctl up %u", i);
                                usleep(3000000);
                                TRACE(STA_SCRIPT, "%s\n", msg.data);
                                write_to_pppd(&msg);
                                //add by ramen to resolve for clicking "connect" button twice.
                            }
                        }
                        else if (pppoeupdownflag == 0)
                        {
							
                            snprintf(msg.data, BUF_SIZE, "spppctl down %u", i);
                            TRACE(STA_SCRIPT, "%s\n", msg.data);
                            write_to_pppd(&msg);
                            //add by ramen to resolve for clicking "disconnect" button twice.
                        }
                    }
                }

        }

    }

    return;
}

#endif

#ifdef CONFIG_USER_CUMANAGEDEAMON
#ifdef CONFIG_USER_LANNETINFO
#include "subr_net.h"
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
static const char IF_CONNET[] = "connecting";
#endif

static const char IF_UP[] = "up";
static const char IF_DOWN[] = "down";
static const char IF_NA[] = "n/a";
int get_wan_status(int ifindex, struct wan_status_info *reEntry)
{
	char ifname[IFNAMSIZ], *str_ipv4;
	char vprio_str[20];
	char MacAddr[20];
	char cmode_str[20];
	int flags, i, k, entryNum, flags_found, isPPP;
#ifdef EMBED
	int spid;
#endif
	MIB_CE_ATM_VC_T entry;
	struct in_addr inAddr;
#ifdef CONFIG_DEV_xDSL
	Modem_LinkSpeed vLs;
	int adslflag;
	MEDIA_TYPE_T mType;
#endif
	int pon_mode = 0;
	FILE * pFile;
	char tmpFile[32], dhcpState[1024];

	struct wan_status_info sEntry[MAX_VC_NUM + MAX_PPP_NUM] = { 0 };
	int count = 0;

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
#endif

#ifdef CONFIG_GPON_FEATURE
	int onu;
	onu = getGponONUState();
#endif

#ifdef CONFIG_EPON_FEATURE
		int ret;
		if(pon_mode == EPON_MODE){
			int eonu = 0;
							
			eonu = getEponONUState(0);
			if (eonu == 5)	
				ret = epon_getAuthState(0);
		}
#endif

#ifdef EMBED
	if ((spid = read_pid(PPP_PID)) > 0)
		kill(spid, SIGUSR2);
	else
		fprintf(stderr, "spppd pidfile not exists\n");
#endif

#ifdef CONFIG_DEV_xDSL
	// check for xDSL link
	if (!adsl_drv_get
	    (RLCM_GET_LINK_SPEED, (void *)&vLs, RLCM_GET_LINK_SPEED_SIZE)
	    || vLs.upstreamRate == 0)
		adslflag = 0;
	else
		adslflag = 1;
#endif
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, &entry)) {
			printf("get MIB chain error\n");
			return -1;
		}
		if(entry.ifIndex != ifindex) continue;
		//record the index.
		count = i;
#ifdef CONFIG_IPV6
		if ((entry.IpProtocol & IPVER_IPV4) == 0)
			continue;	// not IPv4 capable
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#if (defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)) 
		if (entry.omci_configured==1 && entry.applicationtype&X_CT_SRV_VOICE)
			continue;   
#endif

#ifdef CONFIG_CMCC
		/*
			don't show the WAN interface on GUI when Add a WANConnectionDevice.{i} object by TR069
		*/
		if(entry.ConIPInstNum == 0 && entry.ConPPPInstNum == 0)
			continue;
#endif

#endif

		ifGetName(entry.ifIndex, ifname, sizeof(ifname));
		flags_found = getInFlags(ifname, &flags);

		switch (entry.cmode) {
		case CHANNEL_MODE_BRIDGE:
			strcpy(sEntry[i].protocol, "br1483");
			isPPP = 0;
			break;
		case CHANNEL_MODE_IPOE:
			#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
				if(entry.ipDhcp == 0){
					strcpy(sEntry[i].protocol, "Static");
					}
				else{
					strcpy(sEntry[i].protocol, "DHCP");
					}
			#else
				strcpy(sEntry[i].protocol, "IPoE");
				
			#endif			
			isPPP = 0;			
			break;
		case CHANNEL_MODE_PPPOE:	//patch for pppoe proxy
			strcpy(sEntry[i].protocol, "PPPoE");
			isPPP = 1;
			break;
		case CHANNEL_MODE_PPPOA:
			strcpy(sEntry[i].protocol, "PPPoA");
			isPPP = 1;
			break;
		default:
			isPPP = 0;
			break;
		}

		strcpy(sEntry[i].ipAddr, "");
#ifdef EMBED
		if (flags_found && getInAddr(ifname, IP_ADDR, &inAddr) == 1) {
			str_ipv4 = inet_ntoa(inAddr);
			// IP Passthrough or IP unnumbered
			if (flags & IFF_POINTOPOINT && (strcmp(str_ipv4, "10.0.0.1") == 0))
				strcpy(sEntry[i].ipAddr, STR_UNNUMBERED);
			else
				strcpy(sEntry[i].ipAddr, str_ipv4);
		}
#endif

		strcpy(sEntry[i].netmask, "");
		if (flags_found && getInAddr(ifname, SUBNET_MASK, &inAddr) == 1) {
			strcpy(sEntry[i].netmask, inet_ntoa(inAddr));
		}

		strcpy(sEntry[i].gateway, "");
		if (flags_found)
		{
			if(isPPP)
			{
				if(getInAddr(ifname, DST_IP_ADDR, &inAddr) == 1)
					strcpy(sEntry[i].gateway, inet_ntoa(inAddr));
			}
			else
			{
				if(entry.ipDhcp == (char)DHCP_CLIENT)
				{
					FILE *fp = NULL;
					char fname[128] = {0};

					sprintf(fname, "%s.%s", MER_GWINFO, ifname);

					if(fp = fopen(fname, "r"))
					{
					fscanf(fp, "%s", sEntry[i].gateway);
					fclose(fp);
				}
				}
				else
				{
					unsigned char zero[IP_ADDR_LEN] = {0};
					if(memcmp(entry.remoteIpAddr, zero, IP_ADDR_LEN))
						strcpy(sEntry[i].gateway, inet_ntoa(*((struct in_addr *)entry.remoteIpAddr)));
				}
			}
		}

		strcpy(sEntry[i].dns1, "");
		strcpy(sEntry[i].dns2, "");
		get_dns_by_wan(&entry, sEntry[i].dns1, sEntry[i].dns2);

		k = getInAddr(ifname, IP_ADDR, &inAddr);
		// set status flag
		if (flags_found) {
			if ((flags & IFF_UP)&& (flags&IFF_RUNNING)) {
#ifdef CONFIG_DEV_xDSL
				mType = MEDIA_INDEX(entry.ifIndex);

				if (!adslflag &&
#ifdef CONFIG_PTMWAN
				(
#endif
				mType == MEDIA_ATM
#ifdef CONFIG_PTMWAN
				|| mType == MEDIA_PTM)
#endif
						)
					sEntry[i].strStatus = (char *)IF_DOWN;
				else {
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
					if ( getInAddr(ifname, IP_ADDR, &inAddr) == 1
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
					&&((pon_mode == GPON_MODE && onu == 5)|| (pon_mode == EPON_MODE && ret==1))
#endif
#ifdef CONFIG_GPON_FEATURE
						|| (pon_mode == GPON_MODE && onu == 5 && strcmp(sEntry[i].protocol, "br1483") == 0)
						#endif
					    || (pon_mode == EPON_MODE && ret==1 && strcmp(sEntry[i].protocol, "br1483") == 0))
#else
					if (strcmp(sEntry[i].protocol, "br1483") == 0
						|| getInAddr(ifname, IP_ADDR, &inAddr) == 1)
#endif
						sEntry[i].strStatus =
						    (char *)IF_UP;
					
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
					else if ((pon_mode == GPON_MODE && onu == 5)|| (pon_mode == EPON_MODE && ret==1))
						sEntry[i].strStatus =
							(char *)IF_CONNET;
#endif
#endif
					else
						sEntry[i].strStatus =
						    (char *)IF_DOWN;

#ifdef CONFIG_DEV_xDSL
				}
#endif
			}
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
			else if ((pon_mode == GPON_MODE && onu == 5)|| (pon_mode == EPON_MODE && ret==1))
				sEntry[i].strStatus =
					(char *)IF_CONNET;
#endif
#endif
			else
				sEntry[i].strStatus = (char *)IF_DOWN;
		} else
			sEntry[i].strStatus = (char *)IF_NA;

		if (isPPP && strcmp(sEntry[i].strStatus, (char *)IF_UP)) {
			sEntry[i].ipAddr[0] = '\0';
		}

		getWanName(&entry, sEntry[i].servName);

#if defined(CONFIG_EXT_SWITCH) || defined(CONFIG_RTL_MULTI_ETH_WAN) || (defined(ITF_GROUP_1P) && defined(ITF_GROUP))
		sEntry[i].vlanId = entry.vid;
#endif

#ifdef CONFIG_IGMPPROXY_MULTIWAN
		sEntry[i].igmpEnbl = entry.enableIGMP;
#endif

		if (entry.qos == 0) {
			if (entry.svtype == 0) {
				strcpy(sEntry[i].servType, "UBR Without PCR");
			} else {
				strcpy(sEntry[i].servType, "UBR With PCR");
			}
		} else if (entry.qos == 1) {
			strcpy(sEntry[i].servType, "CBR");
		} else if (entry.qos == 2) {
			strcpy(sEntry[i].servType, "Non Realtime VBR");
		} else if (entry.qos == 3) {
			strcpy(sEntry[i].servType, "Realtime VBR");
		}

		if (entry.encap == 1) {
			strcpy(sEntry[i].encaps, "LLC");
		} else {
			strcpy(sEntry[i].encaps, "VCMUX");
		}
		
		if (entry.cmode == CHANNEL_MODE_IPOE && entry.ipDhcp == DHCP_CLIENT) {
			if(strlen(sEntry[i].ipAddr) < 2) {
				ifGetName(entry.ifIndex, ifname, sizeof(ifname));
				sprintf(tmpFile, "%s.%s", DEFAULT_STATE_FILE, ifname);
				pFile = fopen (tmpFile,"r");
				if (pFile!=NULL)
				{
					memset(dhcpState,0,sizeof(dhcpState));
					fgets(dhcpState, sizeof(dhcpState), pFile);
					sprintf(sEntry[i].ipAddr, "%s", dhcpState);
					fclose (pFile);
				}
			}
		}

		memcpy(reEntry,&sEntry[count],sizeof(struct wan_status_info));
		return 1;
	}

	return 0;
}

void get_pon_temperature(char * xString)
{
	rtk_transceiver_data_t transceiver, readableCfg;
	rtk_ponmac_transceiver_get
	    (RTK_TRANSCEIVER_PARA_TYPE_TEMPERATURE, &transceiver);
	_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_TEMPERATURE, &transceiver, &readableCfg);

	char* c_str=NULL;
	if(c_str=strstr(readableCfg.buf,"C")){
		printf("c_str=%s\n",c_str);
		printf("readableCfg.buf=%s len=%d\n",readableCfg.buf,strlen(readableCfg.buf));
		(*c_str)='\0';
		printf("readableCfg.buf=%s len=%d\n",readableCfg.buf,strlen(readableCfg.buf));
	    sprintf(xString, "%s", readableCfg.buf);
	}
}

void get_pon_voltage(char * xString)
{
	rtk_transceiver_data_t transceiver, readableCfg;
	rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_VOLTAGE,
				   &transceiver);
	_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_VOLTAGE, &transceiver, &readableCfg);

	sprintf(xString, "%s", readableCfg.buf);
}

void get_pon_current(char * xString)
{
	rtk_transceiver_data_t transceiver, readableCfg;
	rtk_ponmac_transceiver_get
	    (RTK_TRANSCEIVER_PARA_TYPE_BIAS_CURRENT, &transceiver);
	_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_BIAS_CURRENT, &transceiver, &readableCfg);

	sprintf(xString, "%s", readableCfg.buf);
}

void get_pon_txpower(char *xString)
{
	rtk_transceiver_data_t transceiver, readableCfg;

	rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_TX_POWER,
				   &transceiver);
	_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_TX_POWER, &transceiver, &readableCfg);
	sprintf(xString, "%s", readableCfg.buf);
}

void get_pon_rxpower(char *xString)
{
	rtk_transceiver_data_t transceiver, readableCfg;
	rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_RX_POWER,
				   &transceiver);
	_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_RX_POWER, &transceiver, &readableCfg);

	sprintf(xString, "%s", readableCfg.buf);
}

static char record_ip[32]={0};
void getWanSrcIP(struct in_addr server_addr, char *ipaddr)
{
	char buf[256];
	char *tmpfile="/tmp/wansrcip_tmp";
	FILE *fp;

	sprintf(buf,"ip route get %s > %s",inet_ntoa(server_addr),tmpfile);
	system(buf);

	fp=fopen(tmpfile,"r");
	if(fp){
		if(fgets(buf,sizeof(buf),fp)){
			unsigned char *ptr=strstr(buf,"src ");
			if(ptr){
				sscanf(ptr,"src %s \n",ipaddr);
			}
		}
		fclose(fp);
	}
	unlink(tmpfile);
	if(strlen(ipaddr)){
		strcpy(record_ip,ipaddr);
	}else{
		printf("getWanSrcIP:ipaddr is Null, copy from record %s!\n",record_ip);
		strcpy(ipaddr,record_ip);
	}
}

void getWanPPPoEUsername(char *pppoename, int namelen)
{
	MIB_CE_ATM_VC_T Entry; 

	if(getDefaultRouteATMVCEntry(&Entry)){
		strncpy(pppoename,Entry.pppUsername,namelen);
	}

}

/******CU Manage Tool LOID Resigter API START******/
unsigned long regstarttime=0;
unsigned int regstart=0;
unsigned int regresult=eUserReg_REGISTER_DEFAULT;

void resetRegStatus(int flag)
{
	regstarttime = 0;
	regstart = 0;
	regresult = flag;
}

void startRegStatus()
{
	regstarttime = getSYSInfoTimer(); 
	regstart = 1;
	regresult = eUserReg_REGISTER_DEFAULT;
}

int getUserRegResult(char *para1, char *para2)
{
	static int rebootTime = 0;
	int i, total, ret;
	MIB_CE_ATM_VC_T entry;
	struct in_addr inAddr;
	FILE *fp;
	char buf[256], serviceName[32];
	unsigned int regStatus;
	unsigned int regLimit;
	unsigned int regTimes;
	unsigned char regInformStatus;
	unsigned int regResult;
	unsigned char needReboot;
	unsigned char enable4StageDiag;
	int serviceNum;
	unsigned char cwmp_status = CWMP_STATUS_NOT_CONNECTED;
	/* Set defautl to JSU becuase mib_get_s may fail before reboot. */
	/* To prevent show progress bar with FW for JSU. */
	unsigned char reg_type = DEV_REG_TYPE_JSU;
#ifdef CONFIG_RTK_OMCI_V1
	PON_OMCI_CMD_T msg;
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	unsigned int pon_mode=0, pon_state=0;
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
#endif

	mib_get_s(PROVINCE_DEV_REG_TYPE, &reg_type, sizeof(reg_type));

	mib_get_s(CWMP_USERINFO_STATUS, &regStatus, sizeof(regStatus));
	mib_get_s(CWMP_USERINFO_LIMIT, &regLimit, sizeof(regLimit));
	mib_get_s(CWMP_USERINFO_TIMES, &regTimes, sizeof(regTimes));
	mib_get_s(CWMP_REG_INFORM_STATUS, &regInformStatus, sizeof(regInformStatus));
	mib_get_s(CWMP_USERINFO_RESULT, &regResult, sizeof(regResult));
	mib_get_s(CWMP_USERINFO_NEED_REBOOT, &needReboot, sizeof(needReboot));
	mib_get_s(CWMP_USERINFO_SERV_NUM, &serviceNum, sizeof(serviceNum));
	mib_get_s(RS_CWMP_STATUS, &cwmp_status, sizeof(cwmp_status));

#if 0
	if(regstart == 0){
		return regresult;
	}else
#endif
	{
		if(regTimes >= regLimit){
			resetRegStatus(eeUserReg_REGISTER_FAIL);
			return eeUserReg_REGISTER_FAIL;
		}
		if(regStatus ==5){
			resetRegStatus(eUserReg_REGISTER_REGISTED);
			return eUserReg_REGISTER_REGISTED;
		}
		if(regstart && (getSYSInfoTimer()-regstarttime)<3){ //wait boa task to start reg
			strcpy(para1,"20");
			return eUserReg_REGISTER_OLT;
		}
	}

	if (regInformStatus != CWMP_REG_RESPONSED) {	//ACS not returned result
		total = mib_chain_total(MIB_ATM_VC_TBL);

		for (i = 0; i < total; i++) {
			if (mib_chain_get(MIB_ATM_VC_TBL, i, &entry) == 0)
				continue;

			if ((entry.applicationtype & X_CT_SRV_TR069) &&
					ifGetName(entry.ifIndex, buf, sizeof(buf)) &&
					getInFlags(buf, &ret) &&
					(ret & IFF_UP) &&
					getInAddr(buf, IP_ADDR, &inAddr))
				break;
		}

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
		if(pon_mode == 1)
		{
#if !defined(CONFIG_CMCC) && !defined(CONFIG_CU_BASEON_CMCC)
#ifdef CONFIG_RTK_OMCI_V1
			/* During deactivate, the IP may not be cleared in a small period of time.*/
			/* So check gpon state first. */
			memset(&msg, 0, sizeof(msg));
			msg.cmd = PON_OMCI_CMD_LOIDAUTH_GET_RSP;
			ret = omci_SendCmdAndGet(&msg);

			if (ret != GOS_OK || (msg.state != 0 && msg.state != 1)) {
				resetRegStatus(eUserReg_REGISTER_OLT_FAIL);
				return eUserReg_REGISTER_OLT_FAIL;
			}
			pon_state = msg.state;
#endif
#endif
#if defined(CONFIG_E8B) && defined(CONFIG_PON_CONFIGURATION_COMPLETE_EVENT)
			pon_state = get_omci_complete_event_shm();
#endif
#if (defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)) && defined(CONFIG_E8B) && defined(CONFIG_GPON_FEATURE)
			if(getGponONUState()==GPON_STATE_O5){
				pon_state = 1;
				printf("%s:%d pon_state is O5\n", __FUNCTION__, __LINE__);
			}
			else{
				pon_state = 0;
				printf("%s:%d pon_state is %d\n", __FUNCTION__, __LINE__, getGponONUState());
			}
#endif
		}
		else if(pon_mode == 2)
		{
			int eonu = 0;
							
			eonu = getEponONUState(0);
			pon_state = (eonu==5?1:0);//OLT Register successful
		}
#endif

		if (regstart && (getSYSInfoTimer()-regstarttime)>= 120)
		{
			/* 120 seconds, timeout */
			if (i == total) {
				/* The interface for TR069 is not ready */
				resetRegStatus(eUserReg_REGISTER_TIMEOUT);
				return eUserReg_REGISTER_TIMEOUT;
			} else {
				resetRegStatus(eeUserReg_REGISTER_FAIL);
				return eeUserReg_REGISTER_FAIL;
			}
		} else {
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
			if (pon_state == 0) {
				strcpy(para1,"20");
				return eUserReg_REGISTER_OLT;
			}

#endif

			if (i == total) {
				/* The interface for TR069 is not ready */
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
				if(pon_state == 1)
				{
					strcpy(para1,"30");
					return eUserReg_REGISTER_OLT;
				}
#else
				strcpy(para1,"20");
				return eUserReg_REGISTER_OLT;
#endif
			} else {
				strcpy(para1,"40");
				return eUserReg_REGISTER_OLT;
			}
		}
/*star:20080827 END*/
	} else {
		if (regStatus == 0) {
REG_RESULT_RECHECK:
			switch (regResult) {
			case NO_SET:
				strcpy(para1,"50");
				return eUserReg_REGISTER_OK_DOWN_BUSINESS;
			case NOW_SETTING:
				mib_get_s(CWMP_USERINFO_SERV_NAME, serviceName, sizeof(serviceName));
				if (!strstr(serviceName, "IPTV") &&
					!strstr(serviceName, "INTERNET") &&
					!strstr(serviceName, "VOIP")
				   ) {
				} else {
					strcpy(para2,serviceName);
				}
				mib_get_s(CWMP_USERINFO_SERV_NUM_DONE, &i, sizeof(i));

				if(serviceNum > 0)
				{
					if(i>0)
					{
						sprintf(para1,"%d",60 + 40 * (i-1) / serviceNum);
					}
					else
					{
						strcpy(para1,"60");
					}
				}
				else
				{
					strcpy(para1,"60");
				}
				return eUserReg_REGISTER_OK_DOWN_BUSINESS;
			case SET_SUCCESS:
				if(cwmp_status == CWMP_STATUS_CONNETED)
				{
					regResult = NOW_SETTING;
					goto REG_RESULT_RECHECK;
				}
#ifndef CONFIG_CU_BASEON_CMCC				
				if (needReboot) {
					sprintf(para2,"%d",serviceNum);
					resetRegStatus(eUserReg_REGISTER_OK_NOW_REBOOT);
					return eUserReg_REGISTER_OK_NOW_REBOOT;
				} else 
#endif				
				{
					sprintf(para2,"%d",serviceNum);
					resetRegStatus(eUserReg_REGISTER_OK);
					return eUserReg_REGISTER_OK;
				}
				break;
			case SET_FAULT:
				resetRegStatus(eUserReg_REGISTER_POK);
				return eUserReg_REGISTER_POK;
			}
		}else if (regStatus == 1) {
			if (regTimes < regLimit) {
				resetRegStatus(eUserReg_REGISTER_NOACCOUNT_NOLIMITED);
				sprintf(para1,"%d", regLimit-regTimes);
				return eUserReg_REGISTER_NOACCOUNT_NOLIMITED;
			} else {
				resetRegStatus(eUserReg_REGISTER_NOACCOUNT_LIMITED);
				return eUserReg_REGISTER_NOACCOUNT_LIMITED;
			}
		}else if (regStatus == 2) {
			if (regTimes < regLimit) {
				resetRegStatus(eUserReg_REGISTER_NOUSER_NOLIMITED);
				sprintf(para1,"%d", regLimit-regTimes);
				return eUserReg_REGISTER_NOUSER_NOLIMITED;
			} else {
				resetRegStatus(eUserReg_REGISTER_NOUSER_LIMITED);
				return eUserReg_REGISTER_NOUSER_LIMITED;
			}
		}else if (regStatus == 3) {
			if (regTimes < regLimit) {
				resetRegStatus(eUserReg_REGISTER_NOMATCH_NOLIMITED);
				sprintf(para1,"%d", regLimit-regTimes);
				return eUserReg_REGISTER_NOMATCH_NOLIMITED;
			} else {
				resetRegStatus(eUserReg_REGISTER_NOACCOUNT_LIMITED);
				return eUserReg_REGISTER_NOACCOUNT_LIMITED;
			}
		}else if (regStatus == 4) {
			resetRegStatus(eUserReg_REGISTER_TIMEOUT);
			return eUserReg_REGISTER_TIMEOUT;
		} else if (regStatus == 5) {
			resetRegStatus(eUserReg_REGISTER_REGISTED);
			return eUserReg_REGISTER_REGISTED;
		} else {
			resetRegStatus(eeUserReg_REGISTER_FAIL);
			return eeUserReg_REGISTER_FAIL;
		}
	}

	return eUserReg_REGISTER_DEFAULT;
}


void cuRestartWAN(int chainId)
{
    MIB_CE_ATM_VC_T vcEntry;

    memset(&vcEntry, 0, sizeof(vcEntry));

    if (!mib_chain_get(MIB_ATM_VC_TBL, chainId, (void *)&vcEntry)) {
        printf("cuRestartWAN Fail!!!get mib_atm_vc_tbl error!\n");
        return;
    }
    restartWAN(CONFIGONE, &vcEntry);

	cuPPPoEManual_Action_DisconOrCon(vcEntry);

    return;
}

void checkGUESTSSID()
{
    FILE *fp;
    char buf[64];
    int restartwlan = -1;
    static int timecount = 0;
    unsigned long guest_ssid_endtime;
    int duration = 0;
    unsigned char guest_ssid_2g,guest_ssid_5g;
    char ifname[16];
    static int wlanstopflag = 0;

    fp=fopen(CUMANAGE_GUESTSSID_FILE,"r");
    if(fp)
    {
        if(fgets(buf,64,fp))
        {
            sscanf(buf,"restart:%d\n",&restartwlan);
        }

        printf("%s:restart wlan for guest ssid(%d) 0-stop  1-2Grestart  2-allstop 3-allRestart\n",__func__,restartwlan);

        mib_get_s(MIB_WLAN_GUEST_SSID, (void *)&guest_ssid_2g, sizeof(guest_ssid_2g));
        mib_get_s(MIB_WLAN1_GUEST_SSID, (void *)&guest_ssid_5g, sizeof(guest_ssid_5g));

        switch(restartwlan){
            case 3:
                config_WLAN(ACT_RESTART_5G, guest_ssid_5g-1-WLAN_SSID_NUM);
                get_ifname_by_ssid_index(guest_ssid_5g, ifname);	
                va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=0");
            case 1:
                config_WLAN(ACT_RESTART_2G, guest_ssid_2g-1);
                // Turn off the guest SSID directly by func_off=1
                get_ifname_by_ssid_index(guest_ssid_2g, ifname);	
                va_cmd(IWPRIV, 3, 1, ifname, "set_mib", "func_off=0");
                break;
            case 2:
                config_WLAN(ACT_STOP_5G, guest_ssid_5g-1-WLAN_SSID_NUM);
            case 0:
                config_WLAN(ACT_STOP_2G, guest_ssid_2g-1);
                char guest_idx=0;
                mib_local_mapping_set(MIB_WLAN_GUEST_SSID, 0, (void *)&guest_idx);
#ifdef WLAN_DUALBAND_CONCURRENT
                mib_local_mapping_set(MIB_WLAN_GUEST_SSID, 1, (void *)&guest_idx);
#endif
                break;
            default:
                printf("wlan restart unknown flag:%d 0-stop  1-2Grestart  2-allstop 3-allRestart\n",restartwlan);
                break;
        }

        unlink(CUMANAGE_GUESTSSID_FILE);
    }

    return;
}

void cuRestartLan(int flag)
{
	if(flag & 0x1)
	{
		restart_lanip();
	}
	if(flag & 0x2)
		restart_dhcp();
}

struct callout_s cuManage_ch;
void cuManageSechedule(void *dummy)
{
	FILE *fp;
	char buf[64];
	unsigned char userid[64]={0},password[64]={0};
	int wanIndex = -1;
	int restartlan = -1;
	static int timecounter=0;
	static struct in_addr wanAddr;
	

	fp=fopen(CUMANAGE_LOID_FILE,"r");
	if(fp)
	{
		if(fgets(buf,64,fp))
		{
			sscanf(buf,"userid=%s\n",userid);
		}
		if(fgets(buf,64,fp))
		{
			sscanf(buf,"password=%s\n",password);
		}

		printf("%s:get userid=%s,password=%s\n",__func__,userid,password);

		startUserReg(userid,password);

		unlink(CUMANAGE_LOID_FILE);
	}
	
	fp=fopen(CUMANAGE_PPPOEACCOUNT_FILE,"r");
	if(fp)
	{
		if(fgets(buf,64,fp))
		{
			sscanf(buf,"restart:%d\n",&wanIndex);
		}

		printf("%s:restartflag:%d\n",__func__,wanIndex);

		cuRestartWAN(wanIndex);

		unlink(CUMANAGE_PPPOEACCOUNT_FILE);
	}
	fp=fopen(CUMANAGE_LANPARM_FILE,"r");
	if(fp)
	{
		if(fgets(buf,64,fp))
		{
			sscanf(buf,"restart:%d\n",&restartlan);
		}

		printf("%s:restart lan param : %d  bit0-restartlanip bit1-restartdhcp \n",__func__,restartlan);

		cuRestartLan(restartlan);

		unlink(CUMANAGE_LANPARM_FILE);
	}

	checkGUESTSSID();

	timecounter++;
	if(timecounter >= 3){
		MIB_CE_ATM_VC_T Entry; 
		char ifname[IFNAMSIZ];
		int flags,flags_found;
		static struct in_addr inAddr;
		static char ipchangeflag = 0;

		if(ipchangeflag){
			printf("Wan addr change, restart cumangle \n");
			restart_cumanage();
			ipchangeflag = 0;
		}

		if(getDefaultRouteATMVCEntry(&Entry)){
			ifGetName(Entry.ifIndex, ifname, sizeof(ifname));
			flags_found = getInFlags(ifname, &flags);

			if (flags_found && getInAddr(ifname, IP_ADDR, &inAddr) == 1) {
				//printf("Old Wan addr is %s\n", inet_ntoa(wanAddr));
				//printf("Wan addr is %s\n", inet_ntoa(inAddr));
				if((wanAddr.s_addr != 0) && (inAddr.s_addr!=0) && (wanAddr.s_addr!=inAddr.s_addr)){
					printf("Wan addr change, Nxt cycle will restart cumanage!\n");
					wanAddr.s_addr = inAddr.s_addr;
					ipchangeflag = 1;
					//restart_cumanage();
				}else if(wanAddr.s_addr == 0){
					wanAddr.s_addr = inAddr.s_addr;
				}
			}
		}

		timecounter = 0;
	}


	TIMEOUT_F(cuManageSechedule, 0, 1, cuManage_ch);

}

/******CU Manage Tool LOID Resigter API END******/
#define WIRELESS_NEIGHBOR_INFO_TBL	"/var/wireless_neighbor_info"
int getSitePowerLevel(char *pMacAddr, char *interface_name, int *powerLevel)
{
	WLAN_STA_INFO_T buff[MAX_STA_NUM + 1]={0};
	WLAN_STA_INFO_Tp pInfo;
	int i = 0, found = 0;

	*powerLevel = 0;
	if(getWlStaInfo(interface_name, (WLAN_STA_INFO_Tp) buff) < 0)
		return -1;
	for (i = 1; i <= MAX_STA_NUM; i++) {
		pInfo = (WLAN_STA_INFO_Tp) &buff[i];
		if (pInfo->aid && (pInfo->flags & STA_INFO_FLAG_ASOC)) {
			if(!memcmp(pInfo->addr, pMacAddr, 6)){
				found = 1;
				*powerLevel = pInfo->rssi - 100;
				break;
			}
		}
	}
	if(found)
		return 0;
}

int getLanNetInfoEx(char *pMacAddr, int *num, char *info)
{
	int i, count, ret, specifyMac=0;
	lanHostInfo_t *pLanNetInfo=NULL;
	unsigned char macString[32]={0};
	char ConnectInterface[20]={0};
	char DeviceInfo[128]={0};
	struct in_addr lanIP;
	int powerlevel=0, devNum=0;
	char pDevName[MAX_LANNET_DEV_NAME_LENGTH]={0};
	int speedup=0;
	int wlan_if_index = 0;
	char wlanInterface[32] = {0};

	if((num == NULL) || (info == NULL))
		return -1;

	if(pMacAddr && strlen(pMacAddr) != 0)
		specifyMac = 1;

	ret = get_lan_net_info(&pLanNetInfo, &count);
	if(ret<0)
	{
		goto end;
	}
	
	for(i=0; i<count; i++)
	{
		sprintf(macString, "%2X%2X%2X%2X%2X%2X", (pLanNetInfo[i].mac)[0], (pLanNetInfo[i].mac)[1],(pLanNetInfo[i].mac)[2], 
			(pLanNetInfo[i].mac)[3],(pLanNetInfo[i].mac)[4], (pLanNetInfo[i].mac)[5]);
		fillcharZeroToMacString(macString);
		if(specifyMac && (strcasecmp(pMacAddr, macString) != 0))
			continue;

		if(get_attach_device_name(pLanNetInfo[i].mac, pDevName) != 0)
			strcpy(pDevName, pLanNetInfo[i].devName);

		devNum++;

		lanIP.s_addr = pLanNetInfo[i].ip;	
		if(pLanNetInfo[i].connectionType==1){
			sprintf(ConnectInterface, "SSID%d", pLanNetInfo[i].port);
			if(pLanNetInfo[i].port > WLAN_SSID_NUM)
				wlan_if_index = 1;
			else
				wlan_if_index = 0;
			if(pLanNetInfo[i].port == 1 || pLanNetInfo[i].port-WLAN_SSID_NUM == 1)
				snprintf(wlanInterface, sizeof(wlanInterface), "%s", WLANIF[wlan_if_index]);
			else
				snprintf(wlanInterface, sizeof(wlanInterface), "%s-vap%d", WLANIF[wlan_if_index], wlan_if_index==1?pLanNetInfo[i].port-WLAN_SSID_NUM-1:pLanNetInfo[i].port-2);
			//printf("[%s] wlan_if = %s\n",__FUNCTION__,wlanInterface);
			getSitePowerLevel(pLanNetInfo[i].mac,wlanInterface,&powerlevel);
		}
		else if(pLanNetInfo[i].connectionType==0){
			sprintf(ConnectInterface, "LAN%d", pLanNetInfo[i].port);
			powerlevel = 0;
		}

		if(get_rule_for_dscp_mark(pLanNetInfo[i].mac))
			speedup=1;
		else
			speedup=0;

		if((*num) == 0)
			sprintf(DeviceInfo, "%s:%s:%s:%s:%d:%d:%d", pDevName, macString, inet_ntoa(lanIP), ConnectInterface, speedup, powerlevel, pLanNetInfo[i].onLineTime);
		else
			sprintf(DeviceInfo, "/%s:%s:%s:%s:%d:%d:%d", pDevName, macString, inet_ntoa(lanIP), ConnectInterface, speedup, powerlevel, pLanNetInfo[i].onLineTime);
		strcat(info, DeviceInfo);
		(*num)++;
		if(specifyMac == 1)
			break;
	}

end:
	if(pLanNetInfo)
		free(pLanNetInfo);

	return 0;
	
}

void getAttachWifiDevInfo(int port, char *Protocol, char *RadioType, char *Width, char *Antenna,char *Speed)
{
	MIB_CE_MBSSIB_T Entry;
	unsigned char vChar;
	unsigned char wlband = 0;

#ifdef WLAN_BAND_CONFIG_MBSSID
	if(mib_chain_get(MIB_MBSSIB_TBL, port-1, &Entry) == 0)
		return;
	wlband = Entry.wlanBand;
#else
	if(mib_get_s(MIB_WLAN_BAND, (void *)&wlband, sizeof(wlband)) == 0)
		return;
#endif

	if( wlband==BAND_11B)
		strcpy(Protocol, "802.11b");
	else if( wlband==BAND_11A)
		strcpy(Protocol, "802.11a");
	else if( wlband==BAND_11N)
		strcpy(Protocol, "802.11n");
	else if(wlband>=BAND_5G_11AC)
		strcpy(Protocol, "802.11ac");
	else if(wlband>=BAND_11BG)
		strcpy(Protocol, "802.11g");

	if(port > (1+WLAN_MBSSID_NUM))
		strcpy(RadioType, "5G");
	else
		strcpy(RadioType, "2.4G");

#if defined(CONFIG_USB_RTL8192SU_SOFTAP) || defined(CONFIG_RTL8192CD)	//11n only
	if(wl_isNband(wlband))
	{
		mib_get_s(MIB_WLAN_CHANNEL_WIDTH, &vChar, sizeof(vChar));
		if(vChar)
			strcpy(Width, "40MHz");
		else
			strcpy(Width, "20MHz");
	}
	else
#endif
		strcpy(Width, "20MHz");

	//todo
	strcpy(Antenna, "2*2");	
	strcpy(Speed, "140M"); 
		
}

int getDevWifiInfo(char *pMacAddr, int *num, char *info)
{
	char ConnectInterface[20]={0}, DeviceInfo[128]={0}, Width[32]={0};
	char Protocol[32]={0}, RadioType[32]={0};
	char Antenna[20]={0}, Speed[32]={0};
	unsigned char macString[32]={0};
	int i, count, ret, specifyMac=0, devNum=0;
	lanHostInfo_t *pLanNetInfo=NULL;
	struct in_addr lanIP;
	char pDevName[MAX_LANNET_DEV_NAME_LENGTH]={0};

	if((num == NULL) || (info == NULL))
		return -1;

	if(pMacAddr && (strlen(pMacAddr) != 0))
		specifyMac = 1;

	ret = get_lan_net_info(&pLanNetInfo, &count);
	if(ret<0)
	{
		goto end;
	}

	for(i=0; i<count; i++)
	{
		sprintf(macString, "%2X%2X%2X%2X%2X%2X", (pLanNetInfo[i].mac)[0], (pLanNetInfo[i].mac)[1],(pLanNetInfo[i].mac)[2],
			(pLanNetInfo[i].mac)[3],(pLanNetInfo[i].mac)[4], (pLanNetInfo[i].mac)[5]);
		fillcharZeroToMacString(macString);
		if(specifyMac && (strcasecmp(pMacAddr, macString) != 0))
			continue;

		if(pLanNetInfo[i].connectionType==0)
			continue;

		if(get_attach_device_name(pLanNetInfo[i].mac, pDevName) != 0)
			strcpy(pDevName, pLanNetInfo[i].devName);

		devNum++;

		lanIP.s_addr = pLanNetInfo[i].ip;	
		sprintf(ConnectInterface, "SSID%d", pLanNetInfo[i].port);
		getAttachWifiDevInfo(pLanNetInfo[i].port, Protocol, RadioType, Width, Antenna, Speed);
		if((*num) == 0)
			sprintf(DeviceInfo, "%s:%s:%s:%s:%s:%s:%s:%s:%s", pDevName, macString, inet_ntoa(lanIP), ConnectInterface, Protocol, RadioType, Width, Antenna, Speed);
		else
			sprintf(DeviceInfo, "/%s:%s:%s:%s:%s:%s:%s:%s:%s", pDevName, macString, inet_ntoa(lanIP), ConnectInterface, Protocol, RadioType, Width, Antenna, Speed);
		
		strcat(info, DeviceInfo);
		(*num)++;
		if(specifyMac == 1)
			break;
	}


end:
	if(pLanNetInfo)
		free(pLanNetInfo);

	return 0;
	
}

unsigned char *DevTypeString[3] = {
	"Other",
	"Phone",
	"PC",
};

enum ForcePortalDeviceType
{
	FP_InvalidDevice = 0,
	FP_Computer,
	FP_STB,
	FP_MOBILE,
};

int getDHCPClientType(char *mac, int *hosttype)
{
	FILE 	*fp;
	int 	pid, clientNum=0, i;
	unsigned long read_size=0;
	char *DHCPClientInfo = NULL;
	char macString[32]={0};
	struct dhcpOfferedAddrLease *p = NULL;

	clientNum = getDhcpClientLeasesDB(&DHCPClientInfo, &read_size);
	if(clientNum <= 0)
		goto err;
	
	for(i=0; i<clientNum; i++)
	{
		p = (struct dhcpOfferedAddrLease *) (DHCPClientInfo + i * sizeof( struct dhcpOfferedAddrLease ));
		sprintf(macString, "%2x%2x%2x%2x%2x%2x",
			p->chaddr[0],p->chaddr[1],p->chaddr[2],
			p->chaddr[3],p->chaddr[4],p->chaddr[5]);
		fillcharZeroToMacString(macString);

		if(strcmp(mac, macString) == 0)
		{
			*hosttype = p->hostType;
			break;
		}
			
	}

err:	
	if(DHCPClientInfo)
	{
		free(DHCPClientInfo);
		DHCPClientInfo=NULL;
	}
	
	return 0;
}

int getAttachDevInfo(char *info)
{
	int i, ret, count=0, port=0, devType=0;
	lanHostInfo_t *pLanNetInfo=NULL;
	unsigned char macString[32]={0};
	char DeviceInfo[128]={0};
	struct in_addr lanIP;
	char DevName[MAX_LANNET_DEV_NAME_LENGTH]={0};
	char DevType[32]={0};
	char Action[32]={0};

	if(info == NULL)
		return 0;

	ret = get_online_and_offline_device_info(&pLanNetInfo, &count);
	if(ret<0)
	{
		goto end;
	}

	for(i=0; i<count; i++)
	{
		sprintf(macString, "%2X%2X%2X%2X%2X%2X", (pLanNetInfo[i].mac)[0], (pLanNetInfo[i].mac)[1],
			(pLanNetInfo[i].mac)[2], (pLanNetInfo[i].mac)[3],(pLanNetInfo[i].mac)[4], (pLanNetInfo[i].mac)[5]);
		fillcharZeroToMacString(macString);
		
		if(get_attach_device_name(pLanNetInfo[i].mac, DevName) != 0)
			strcpy(DevName, pLanNetInfo[i].devName);

		lanIP.s_addr = pLanNetInfo[i].ip;		
		if(pLanNetInfo[i].connectionType==1){
			port = 0;
		}	

		if(get_attach_device_type(pLanNetInfo[i].mac, DevType) != 0)
		{

			if((pLanNetInfo[i].devType == 1) || (pLanNetInfo[i].devType == 2))
			{
				strcpy(DevType, DevTypeString[pLanNetInfo[i].devType]);
			}
			else				
			{
				getDHCPClientType(macString, &devType);
				if(devType == FP_STB)
					strcpy(DevType, "STB");
				else if(devType == FP_Computer)
					strcpy(DevType, "PC");
				else if(devType == FP_MOBILE)
					strcpy(DevType, "Phone");
				else
					strcpy(DevType, "Other");
			}
		}

		if(pLanNetInfo[i].firstConnect == 1)
			strcpy(Action, "Online");
		else if(pLanNetInfo[i].disConnect == 1) 
			strcpy(Action, "Offline");
		
		if(i != (count-1))
			sprintf(DeviceInfo, "%s:%s:%s:%s:%d:%d:%s:%s:%s/", DevName, DevType, macString, inet_ntoa(lanIP), pLanNetInfo[i].connectionType, port, pLanNetInfo[i].brand, pLanNetInfo[i].os, Action);
		else
			sprintf(DeviceInfo, "%s:%s:%s:%s:%d:%d:%s:%s:%s", DevName, DevType, macString, inet_ntoa(lanIP), pLanNetInfo[i].connectionType, port, pLanNetInfo[i].brand, pLanNetInfo[i].os, Action);
		strcat(info, DeviceInfo);
	}

end:
	if(pLanNetInfo)
		free(pLanNetInfo);

	return count;
	
}

void get_query_wan_info(char *wanname, char *strStatus, char *ipv6Addr, char *ipv6prefixLen, unsigned char *ipv6Gateway, char *ipv6Dns1, char *ipv6Dns2, char *ipv6Prefix)
{
	int mibTotal, i, j, k;
	int flags;
#ifdef CONFIG_DEV_xDSL
	Modem_LinkSpeed vLs;
	int adslflag;
	MEDIA_TYPE_T mType;
#endif
	int pon_mode = 0;
	char interface_name[MAX_WAN_NAME_LEN]={0};
	MIB_CE_ATM_VC_T vcEntry;
	struct ipv6_ifaddr ipv6_addr[6];
	char ifname[IFNAMSIZ], str_ipv6[INET6_ADDRSTRLEN];
	unsigned char zero[IP6_ADDR_LEN] = {0};

#ifdef CONFIG_DEV_xDSL
	// check for xDSL link
	if (!adsl_drv_get
	    (RLCM_GET_LINK_SPEED, (void *)&vLs, RLCM_GET_LINK_SPEED_SIZE)
	    || vLs.upstreamRate == 0)
		adslflag = 0;
	else
		adslflag = 1;
#endif

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
#endif

#ifdef CONFIG_GPON_FEATURE
	int onu;
	onu = getGponONUState();
#endif

#ifdef CONFIG_EPON_FEATURE
	int ret;
	if(pon_mode == EPON_MODE){
		int eonu = 0;
						
		eonu = getEponONUState(0);
		if (eonu == 5)	
			ret = epon_getAuthState(0);
	}
#endif

	memset(&vcEntry, 0, sizeof(vcEntry));
	mibTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<mibTotal; i++) 
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry)) 
			continue;
		
		getWanName(&vcEntry, interface_name);
		if(!strcmp(interface_name, wanname))
		{
			ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));

			k = getifip6(ifname, IPV6_ADDR_UNICAST, ipv6_addr, 6);
			if (k) {
				for (j = 0; j < k; j++) {
					inet_ntop(AF_INET6, &ipv6_addr[j].addr, str_ipv6,
						  INET6_ADDRSTRLEN);
					if (j == 0)
					{
						sprintf(ipv6Addr, "%s", str_ipv6);
						sprintf(ipv6prefixLen, "%d", ipv6_addr[j].prefix_len);
					}
					else
					{
						sprintf(ipv6Addr, "%s, %s", ipv6Addr, str_ipv6);
						sprintf(ipv6prefixLen, "%s, %d", ipv6prefixLen, ipv6_addr[j].prefix_len);
					}
				}
			}

			if((vcEntry.IpProtocol & IPVER_IPV6) == 0)
			{
				strcpy(strStatus, "Unconfigured");
			}
			else
			{
				if (getInFlags(ifname, &flags) == 1) {
				if ((flags & IFF_UP) && (flags&IFF_RUNNING)) {
#ifdef CONFIG_DEV_xDSL
					mType = MEDIA_INDEX(vcEntry.ifIndex);

					if (!adslflag &&
#ifdef CONFIG_PTMWAN
					(
#endif
					mType == MEDIA_ATM
#ifdef CONFIG_PTMWAN
					|| mType == MEDIA_PTM)
#endif
							)
						strcpy(strStatus, "Disconnected");
					else {
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
						if ( k!=0
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
						&&((pon_mode == GPON_MODE && onu == 5)|| (pon_mode == EPON_MODE && ret==1)) 			
#endif
#ifdef CONFIG_GPON_FEATURE
							|| (pon_mode == GPON_MODE && onu == 5 && (vcEntry.cmode == CHANNEL_MODE_BRIDGE))
#endif
						    || (pon_mode == EPON_MODE && ret==1 && (vcEntry.cmode == CHANNEL_MODE_BRIDGE)))
#else
						if ((vcEntry.cmode == CHANNEL_MODE_BRIDGE)
							|| k!=0)
#endif
							strcpy(strStatus, "Connected");
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
						else if ((pon_mode == GPON_MODE && onu == 5)|| (pon_mode == EPON_MODE && ret==1))	
							strcpy(strStatus, "Connecting");
#endif
#endif
						else
							strcpy(strStatus, "Disconnected");
#ifdef CONFIG_DEV_xDSL
					}
#endif
				}
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
				else if ((pon_mode == GPON_MODE && onu == 5)|| (pon_mode == EPON_MODE && ret==1))	
					strcpy(strStatus, "Connecting");
#endif
#endif
				else
					strcpy(strStatus, "Disconnected");
			} else
			{
				strcpy(strStatus, "Unconfigured");
			}
		}

		if(memcmp(vcEntry.RemoteIpv6Addr, zero, IP6_ADDR_LEN))
			inet_ntop(AF_INET6, &vcEntry.RemoteIpv6Addr, ipv6Gateway, INET6_ADDRSTRLEN);

		get_dns6_by_wan(&vcEntry, ipv6Dns1, ipv6Dns2);

		if(vcEntry.cmode != CHANNEL_MODE_BRIDGE && vcEntry.IpProtocol & IPVER_IPV6)
		{
			if(vcEntry.AddrMode & IPV6_WAN_STATIC)
			{
				struct in6_addr prefix = {0};
				const char *dst;

				ip6toPrefix(vcEntry.Ipv6Prefix, vcEntry.Ipv6AddrPrefixLen, &prefix);
					dst = inet_ntop(AF_INET6, &prefix, ipv6Prefix, sizeof(ipv6Prefix));
#ifdef CONFIG_USER_RTK_IPV6_MULTI_LAN_SERVICE_WITH_SINGLE_BR
					if(dst)
						sprintf(ipv6Prefix, "%s/%d", dst, vcEntry.Ipv6PrefixLen);
#endif			
				}
				else
				{
					DLG_INFO_T dlg_info;
					char fname[256] = {0};
					int ret;
					const char *dst;

					memset(&dlg_info, 0, sizeof(dlg_info));
					snprintf(fname, 256, "%s.%s", PATH_DHCLIENT6_LEASE, ifname);
					ret = getLeasesInfo(fname, &dlg_info);
					dst = inet_ntop(AF_INET6, dlg_info.prefixIP, ipv6Prefix, 64);

					if ((ret & LEASE_GET_IAPD) && dst)
						sprintf(ipv6Prefix, "%s/%d", dst, dlg_info.prefixLen);
				}
				
				if(strcmp(ipv6Prefix, "::") == 0)
					strcpy(ipv6Prefix, "-");
			}

		}
	}

}

void restart_cumanage()
{
#if 1
	int pid;
	pid = read_pid("/var/run/cumanagedaemon.pid");
	if (pid > 0) {
		kill(pid, SIGUSR1);
		usleep(1000);
	}
#else
	system("killall cumanagedaemon");	
	va_cmd("/bin/cumanagedaemon",0,0);
#endif
}
#endif

#ifdef CU_APP_SCHEDULE_LOG
void GenerateScheduledLog(time_t ts, time_t te)
{
	struct tm tm_log;
	time_t tl = 0;
	FILE *fp = NULL, *fp_w = NULL; 
	int jump_file_header_lines = 0;
	char buf[256] ={0};
	
#ifdef SUPPORT_LOG_TO_TEMP_FILE
	unsigned char syslogEnable;
	mib_get(MIB_SYSLOG,(void*)&syslogEnable);
	if(syslogEnable == 0)
		fp=fopen("/var/syslogd.txt","r");
	else
#endif
		fp=fopen("/var/config/syslogd.txt","r");
	if(!fp)
		goto END;

	fp_w = fopen("/var/config/sched_syslog","w");
	if(!fp_w)
		goto END;
	
	while(!feof(fp) && fgets(buf,256,fp))
	{
		//printf("%s\n",buf);
		jump_file_header_lines ++;
		if(jump_file_header_lines <= 7)
		{
			fputs(buf,fp_w);
			continue;
		}

		memset(&tm_log,0,sizeof(struct tm));
		sscanf(buf,"%04d-%02d-%02d %02d:%02d:%02d %*s",&tm_log.tm_year,&tm_log.tm_mon,&tm_log.tm_mday,
		&tm_log.tm_hour,&tm_log.tm_min,&tm_log.tm_sec);
		tm_log.tm_year -= 1900;
		tm_log.tm_mon -= 1;	
		tl = mktime(&tm_log);	

		//printf("[%lu - %lu]  time_log = %lu\n",ts,te,tl);	
		if(tl >= ts && tl <= te)
			fputs(buf,fp_w);
	}

END:
	if(fp)
		fclose(fp);
	if(fp_w)
		fclose(fp_w);
	
}

void UploadScheduledLog()
{	
	FILE * fp_sched = NULL;

	fp_sched = fopen("/var/config/syslog_upload_path","r");
	if(fp_sched)
	{
		char upload_path[256]={0}, cmd[256] ={0};
		fgets(upload_path,256,fp_sched);
		printf("[%s]  %s\n",__FUNCTION__,upload_path);
		if(!strncmp(upload_path,"http",4))
		{
			//path like http://software:software@172.29.38.4/
			snprintf(cmd,sizeof(cmd),"curl -T /var/config/sched_syslog --connect-timeout 5 %s",upload_path);
			system(cmd);			
		}
		else if(!strncmp(upload_path,"ftp",3))
		{
			//path like ftp://software:software@172.29.38.4/
			snprintf(cmd,sizeof(cmd),"curl -T /var/config/sched_syslog --connect-timeout 5 %s",upload_path);
			system(cmd);
		}
		
		fclose(fp_sched);
	}	
}
#endif

void get_pon_traffic(uint64_t  *tx_value, uint64_t  *rx_value)
{
#if !defined(CONFIG_RTK_L34_ENABLE)
	unsigned int pon_port_idx = 1;
#endif

#if !defined(CONFIG_RTK_L34_ENABLE)
#if defined(CONFIG_COMMON_RT_API)
	rt_switch_phyPortId_get(LOG_PORT_PON, &pon_port_idx);
#else
	rtk_switch_phyPortId_get(RTK_PORT_PON, &pon_port_idx);
#endif
#endif

#if defined(CONFIG_RTK_L34_ENABLE)
	rtk_rg_stat_port_get(RTK_RG_PORT_PON, IF_OUT_OCTETS_INDEX, tx_value);
#elif defined(CONFIG_COMMON_RT_API)
	rt_stat_port_get(pon_port_idx, RT_IF_OUT_OCTETS_INDEX, tx_value);
#else
	rtk_stat_port_get(pon_port_idx, IF_OUT_OCTETS_INDEX, tx_value);
#endif

#if defined(CONFIG_RTK_L34_ENABLE)
	rtk_rg_stat_port_get(RTK_RG_PORT_PON, IF_IN_OCTETS_INDEX, rx_value);
#elif defined(CONFIG_COMMON_RT_API)
	rt_stat_port_get(pon_port_idx, RT_IF_IN_OCTETS_INDEX, rx_value);
#else
	rtk_stat_port_get(pon_port_idx, IF_IN_OCTETS_INDEX, rx_value);
#endif
}

#if defined(PON_HISTORY_TRAFFIC_MONITOR)
#define PON_TRAFFIC_RECORD_REVERSE 1

#ifdef PON_TRAFFIC_RECORD_REVERSE
void set_pon_traffic_his(unsigned long long  tx, unsigned long long  rx)
{
	FILE *fp;
	char buf[64]={0};
	int rec_num=0;

	if (access(PON_TRAFFIC_RECORD, F_OK)== -1) {
		fp = fopen(PON_TRAFFIC_RECORD, "w+");
		if(fp){
			sprintf(buf,"%llu %llu\n", tx, rx);
			fprintf(fp, buf);
			fclose(fp);
		}
	}
	else {
		fp=fopen(PON_TRAFFIC_RECORD,"r");
		if(fp) {
			char rc;
			unsigned long long  tx_bytes=0;
			unsigned long long  rx_bytes=0;
			FILE *fp2;
			
			fp2=fopen(PON_TRAFFIC_RECORD_TMP,"w");
			while (fp2 && !(feof(fp))) {
				if(rec_num == 0){
					sprintf(buf,"%llu %llu\n", tx, rx);
					fprintf(fp2, buf);
					rec_num++;
				}

				rc = fscanf(fp, "%llu %llu", &tx_bytes, &rx_bytes);
				if (2 == rc && EOF != rc) {
					rec_num++;

					if(rec_num > PON_HISTORY_TRAFFIC_MONITOR_ITEM_MAX) {
						break;
					} else {
						sprintf(buf,"%llu %llu\n", tx_bytes, rx_bytes);
						fprintf(fp2, buf);
					}
				}
			}

			fclose(fp);

			if(fp2) {
				fclose(fp2);

				char cmd[256];
				snprintf(cmd, 256, "cp -f %s %s", PON_TRAFFIC_RECORD_TMP, PON_TRAFFIC_RECORD);
				system(cmd);
				
				unlink(PON_TRAFFIC_RECORD_TMP);
			}
		}
	}
}
#else
void set_pon_traffic_his(unsigned long long  tx, unsigned long long  rx)
{
	FILE *fp;
	char buf[64]={0};
	int rec_num=0;

	fp=fopen(PON_TRAFFIC_RECORD,"a+");
	if(fp) {
		sprintf(buf,"%llu %llu\n", tx, rx);
		fprintf(fp, buf);
		fclose(fp);
	}
	
	fp=fopen(PON_TRAFFIC_RECORD,"r");
	if(fp) {
		char rc;
		unsigned long long  tx_bytes=0;
		unsigned long long  rx_bytes=0;
		FILE *fp2;
		
		fp2=fopen(PON_TRAFFIC_RECORD_TMP,"w");
		while (fp2 && !(feof(fp))) {
			rc = fscanf(fp, "%llu %llu", &tx_bytes, &rx_bytes);
			if (2 == rc && EOF != rc) {
				rec_num++;

				//remove first line
				if(rec_num > 1) {
					sprintf(buf,"%llu %llu\n", tx_bytes, rx_bytes);
					fprintf(fp2, buf);
				}
			}
		}

		fclose(fp);

		if(fp2) {
			fclose(fp2);
		
			if(rec_num > PON_HISTORY_TRAFFIC_MONITOR_ITEM_MAX) {
				char cmd[256];
				snprintf(cmd, 256, "cp -f %s %s", PON_TRAFFIC_RECORD_TMP, PON_TRAFFIC_RECORD);
				system(cmd);
			}
			unlink(PON_TRAFFIC_RECORD_TMP);
		}
	}
}
#endif

void get_pon_traffic_by_index(unsigned long long  *tx, unsigned long long  *rx, int index)
{
	unsigned long long  tx_bytes=0;
	unsigned long long  rx_bytes=0;
	FILE *fp;
	char buf[64]={0};
	char rc;
	int rec_num=0;

	fp=fopen(PON_TRAFFIC_RECORD,"r");
	if(fp) {
		while ((!(feof(fp))) && (rec_num !=index)) {
			rc = fscanf(fp, "%llu %llu", &tx_bytes, &rx_bytes);
			if (2 == rc && EOF != rc) {
				rec_num++;
			}
		}
		fclose(fp);
	}

	if(rec_num ==index)
	{
		*tx = tx_bytes;
		*rx = rx_bytes;
	}
}

void get_pon_traffic_base(char *xString, int start)
{
	unsigned long long  tx_bytes=0;
	unsigned long long  rx_bytes=0;
	
	if(start < PON_HISTORY_TRAFFIC_MONITOR_ITEM_MAX && start >=0) {
		get_pon_traffic_by_index(&tx_bytes, &rx_bytes, start);
		sprintf(xString, "%llu/%llu", tx_bytes/1024, rx_bytes/1024);
	}
}

void get_pon_traffic_history(char *xString, int start, int num, int len)
{
	int i, k=0;
	unsigned long long  tx_value = 0, rx_value = 0;
	unsigned long long  tx_value_last = 0, rx_value_last = 0;
	unsigned long long  delta_tx_value = 0, delta_rx_value = 0;

	snprintf (xString, len, "%s", "[");

	if(start < PON_HISTORY_TRAFFIC_MONITOR_ITEM_MAX && start >=0 && num >1) {
		for(i=start+1; (i< PON_HISTORY_TRAFFIC_MONITOR_ITEM_MAX && k<=num); i++) {
			tx_value = 0;
			rx_value = 0;
			tx_value_last = 0;
			rx_value_last = 0;

			get_pon_traffic_by_index(&tx_value, &rx_value, i);

#if 0
			if(tx_value ==0 && rx_value==0)
				break;
#endif			
			get_pon_traffic_by_index(&tx_value_last, &rx_value_last, i-1);

#ifdef PON_TRAFFIC_RECORD_REVERSE
			delta_tx_value = tx_value_last - tx_value;
			delta_rx_value = rx_value_last - rx_value;
#else
			delta_tx_value = tx_value - tx_value_last;
			delta_rx_value = rx_value - rx_value_last;
#endif
		
			snprintf(xString, len, "%s%s\"%llu/%llu\"", xString, (k==0)?"":",",delta_tx_value/1024, delta_rx_value/1024);
			k++;
		}
	}

	snprintf(xString, len, "%s]", xString);
}

struct callout_s pon_traffic_monitor_ch;
void pon_traffic_monitor(void *dummy)
{
	uint64_t  tx_bytes;
	uint64_t  rx_bytes;

	get_pon_traffic(&tx_bytes, &rx_bytes);
	
	set_pon_traffic_his(tx_bytes, rx_bytes);
	
	TIMEOUT_F(pon_traffic_monitor, 0, PON_HISTORY_TRAFFIC_MONITOR_INTERVAL, pon_traffic_monitor_ch);
}
#endif

int get_alldns_by_wan(MIB_CE_ATM_VC_T *pEntry, char *dns, int dnslen)
{
	unsigned char zero[IP_ADDR_LEN] = {0};
	char *pdns = NULL;
	int cnt = 0;
			
	if ( ((pEntry->cmode == CHANNEL_MODE_RT1483) || (pEntry->cmode == CHANNEL_MODE_IPOE)) && (pEntry->dnsMode == REQUEST_DNS_NONE) )
	{
		//manual
		pdns = dns;
		memset(pdns,0,2*dnslen);
		if(memcmp(zero, pEntry->v4dns1, IP_ADDR_LEN) != 0)
		{
			strcpy(pdns, inet_ntoa(*((struct in_addr *)pEntry->v4dns1)));
			cnt++;
		}
		if(memcmp(zero, pEntry->v4dns2, IP_ADDR_LEN) != 0)
		{
			strcpy(pdns+dnslen, inet_ntoa(*((struct in_addr *)pEntry->v4dns2)));
			cnt++;
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

			while(fgets(line,sizeof(line),infdns))
			{
				if((strlen(line)==0))
					continue;

				p = strchr(line, '@');
				if (p)
				{
					pdns = dns+cnt*dnslen;
					memcpy(pdns, line, p-line);
					pdns[p-line] = '\0';
					cnt++;
					//printf("----------get dns: %s--------\n",pdns);
				}
			}
			fclose(infdns);
		}
	}
	return cnt;
}

#ifdef CONFIG_CU_BASEON_YUEME
struct callout_s framework_ch;

void schFramework_chCheck(void *dummy)
{
	//#define command_saf "pidof ufwmg"
#define command_saf "cat /var/run/ufwmg.pid"
#ifdef CONFIG_CU
	char cmdBuf[512];
#endif
	#define command_reg "ps |grep reg_server|grep -v grep"
	FILE *fp;
	char buf[1024], *pbuf;
	int saf_cnt;
	int check_saf = 1, pidExist=1;

	if((fp = popen(command_reg,"r")) != NULL)
	{
		if( (fgets(buf,1024,fp))== NULL )
		{
			printf("pid reg_server is not found!\n");
			pidExist = 0;
		}
		pclose(fp);
	}
	
#ifdef CONFIG_CU
	if(access("/usr/sbin/ufwmg", F_OK)!=0 || (pidExist == 0))
		check_saf = 0;
#else
	if(access("usr/sbin/saf", F_OK)!=0)
		check_saf = 0;
#endif

	if(check_saf){
		/*if pid /usr/sbin/dbus-daemon --system is exit*/
		saf_cnt = 0;
		if((fp = popen(command_saf,"r")) != NULL)
		{
#ifdef CONFIG_CU
			memset(buf, 0, sizeof(buf));
			fgets(buf,1024,fp);
			pclose(fp);
			int pid = atoi(buf);
			if(pid > 0)
			{
				memset(cmdBuf, 0, sizeof(cmdBuf));
				snprintf(cmdBuf, sizeof(cmdBuf), "/proc/%d", pid);
				if (!access(cmdBuf, F_OK))
					saf_cnt = 2;
			}
#else
			buf[0] = '\0';
			fgets(buf,1024,fp);
			if(buf[0] != '\0') saf_cnt = 1;
			pclose(fp);
			pbuf = buf; 
			while((pbuf = strchr(pbuf, ' '))) {
				pbuf++;
				saf_cnt++;
			}
#endif
		}
		if(saf_cnt < 2){
#ifdef CONFIG_CU
			if(saf_cnt <= 0)
#else
			if(saf_cnt <= 0 || access("/opt/upt/apps/saf-starttime", F_OK) != 0)
#endif
			{
			system("killall ufwmg");
#ifdef CONFIG_CU
			printf("Recover ufwmg ....\n");
#else
			printf("Recover saf ....\n");
#endif
			#ifdef CONFIG_STATIC_CONFIG
#ifdef CONFIG_LUNA_G3_SERIES
			char *f1 = "11", *f2 = "12", *c1 = "13";
#else
			char *f1 = "8", *f2 = "9", *c1 = "10";
#endif
			parseFrameworkPartition(&f1,&f2,&c1);
#ifdef CONFIG_CU
			va_niced_cmd("/usr/sbin/ufwmg", 4, 0, "service", f1, f2, c1);
#else
			va_niced_cmd("/usr/sbin/saf", 4, 0, "service", f1, f2, c1);
#endif
			#else
#ifdef CONFIG_LUNA_G3_SERIES
#ifdef CONFIG_CU
			system("ufwmg service 11 12 13");
#else
			system("saf service 11 12 13");
#endif
#else
#ifdef CONFIG_CU
			system("ufwmg service 8 9 10");
#else
			system("saf service 8 9 10");
#endif
#endif
			#endif
			}
		}
	}

	TIMEOUT_F((void*)schFramework_chCheck, 0, 5, framework_ch); 
}

#define PLATFORM_STATUS_FILE "/tmp/platform_status_file"
#define MAX_LINE_SIZE 512
int getPlatformStatus(char *Srvip, int *port, int *status)
{
	FILE *f=NULL;
	char line[MAX_LINE_SIZE]={0};

	va_cmd("/bin/get_platform_status", 0, 1);
	f=fopen(PLATFORM_STATUS_FILE,"rb");
	if(!f){
		printf("open %s file error!\n", PLATFORM_STATUS_FILE);
		return -1;
	}
	
	while (!feof(f))
	{
		fgets(line, MAX_LINE_SIZE - 1, f);
		if (strstr(line, "Srvip")) {
			sscanf(line, "Srvip: %s\n", Srvip);
		} 
		else if (strstr(line, "port")) {
			sscanf(line, "port: %d\n", port);
		}
		else if (strstr(line, "status")) {
			sscanf(line, "status: %d\n", status);
		}
	}
	
	fclose(f);
	return 0;
}
#endif

#ifdef _PRMT_C_CU_LOGALARM_
int isAlarmEnable()
{
	int vInt=0;
	mib_get_s(MIB_ALARM_ENABLE,&vInt, sizeof(vInt));
	if (vInt==1)
		return 1;
	return 0;
}

int getAlarmLevel()
{
	int vInt=0;
	mib_get_s(MIB_ALARM_LEVEL,&vInt, sizeof(vInt));
	return vInt;
}

int getAlarmInfo(ALARMRECORD_TP *info, int *count)
{
    ALARMRECORD_TP finfo = NULL;
    ALARMRECORD_T entry;
    FILE *fp=NULL;
    int alarmid, alarmcode, raisetime, cleartime;
    int perceivedseverity, alarmstatus, idx=0, len;
    char logStr[64]={0};
    char *tmpBuf=NULL;
    char line[512]={0};

    memset(&entry, 0, sizeof(ALARMRECORD_T));
    finfo = (ALARMRECORD_TP) malloc(sizeof(ALARMRECORD_T) * 100);
    if (finfo == NULL) {
        return -1;
    }

    *info = (ALARMRECORD_TP) finfo;
    memset(finfo, 0, sizeof(ALARMRECORD_T) * 100);

    fp = fopen(LOG_ALARM_PATH, "r");
    if(fp == NULL)
    {
        printf( "Open %s failed\n", LOG_ALARM_PATH);
        return -1;
    }
    else
    {
        while(fgets(line, sizeof(line), fp) != NULL)
        {
        	if(strstr(line, "["))
        	{
	            sscanf(line, "%d %d %d %d %d %d [%s]", &alarmid, &alarmcode, &alarmstatus, &perceivedseverity, &raisetime, &cleartime, logStr);
	            tmpBuf = strstr(line, "[")+1;
	            len = strlen(tmpBuf);
	            tmpBuf[len-2]='\0';
	            entry.alarmid = alarmid;
	            entry.alarmcode = alarmcode;
	            entry.alarmstatus = alarmstatus;
	            entry.perceivedseverity = perceivedseverity;
	            entry.alarmRaisedTime = raisetime;
	            entry.cleartime = cleartime;
	            strcpy(entry.logStr, tmpBuf);

	            memcpy(&finfo[idx], &entry, sizeof(ALARMRECORD_T));
	            idx++;
        	}
        }

        *count=idx;

        fclose(fp);
    }

    return 0;
}

unsigned int findMaxAlarmid()
{
	unsigned int ret=0, i,count;
	ALARMRECORD_TP alarmRecord=NULL;
	ret = getAlarmInfo(&alarmRecord, &count);

	if((count > 0) && (ret >= 0) && alarmRecord)
	{
		for(i=0; i<count; i++)
		{
			if(alarmRecord[i].alarmid >= ret)
			{
				ret = alarmRecord[i].alarmid;
			}
		}
	}

	if(alarmRecord)
	{
		free(alarmRecord);
		alarmRecord=NULL;
	}

	return ret;
}

int syslogAlarm(int code, int status, int severity, char *AlarmDetail, int flag)
{
	char buf[512]={0};
	char line[512]={0};
	int alarmlevel, alarmcode, alarmstatus;
	int id=0, raisetime=0, cleartime=0;
	int perceivedseverity;
	char logStr[64]={0};
	int find=0;
	FILE *fp = NULL,  *fp_tmp = NULL;

	if(!isAlarmEnable())
		return -1;

	alarmlevel=getAlarmLevel();
	if(severity > alarmlevel)
		return -1;

	fp = fopen(LOG_ALARM_PATH,"r");
	if(fp == NULL)
	{
		fp = fopen(LOG_ALARM_PATH,"w");
		if(fp == NULL)
		{
			printf("open %s failed!", LOG_ALARM_PATH);
			return -1;
		}

		id = findMaxAlarmid() + 1;
		raisetime = time(NULL);
		sprintf(buf,"%d %d %d %d %d %d [%s]\n", id, code, status, severity, raisetime, cleartime, AlarmDetail);
		fputs(buf, fp);
		fclose(fp);
	}
	else
	{
		fp_tmp = fopen(TEMP_LOG_ALARM_PATH,"w");
		if(fp_tmp == NULL)
		{
			printf("open %s failed!", TEMP_LOG_ALARM_PATH);
			fclose(fp);
			return -1;
		}

		while(fgets(line, sizeof(line), fp) != NULL)
		{
			sscanf(line, "%d %d %d %d %d %d %[%s]\n", &id, &alarmcode, &alarmstatus, &perceivedseverity, &raisetime, &cleartime, logStr);
			if(alarmcode != code) {
				fprintf(fp_tmp, line);
			} else {
				find = 1;
				if(flag == 0)
				{
					raisetime = time(NULL);
					sprintf(buf,"%d %d %d %d %d %d [%s]\n", id, code, status, severity, raisetime, 0, AlarmDetail);
				}
				else
				{
					status = ALARM_RECOVERED;
					cleartime = time(NULL);
					sprintf(buf,"%d %d %d %d %d %d [%s]\n", id, code, status, severity, raisetime, cleartime, AlarmDetail);
				}

				fputs(buf, fp_tmp);
			}

		}

		if(find == 0)
		{
			id = findMaxAlarmid() + 1;
			raisetime = time(NULL);
			sprintf(buf,"%d %d %d %d %d %d [%s]\n", id, code, status, severity, raisetime, 0, AlarmDetail);
			fputs(buf, fp_tmp);
		}

		fclose(fp);
		fclose(fp_tmp);
		unlink(LOG_ALARM_PATH);
		rename(TEMP_LOG_ALARM_PATH, LOG_ALARM_PATH);
	}
	return 0;
}
#endif

const char DSCP_REMAKR_FILE[] = "/var/dscp_remark_rules";
const char DSCP_REMAKR_TEMP_FILE[] = "/var/dscp_remark_rules_temp";

extern const char QOS_EB_US_CHAIN[];
int set_dscp_for_device(char *mac,unsigned char dscp)
{
	char cmd_buf[200]={0};
	char cmd[256] = {0};
	char macString[32]={0};
	unsigned int qosRuleNum = getQosRuleNum();

	if(qosRuleNum == 0){//QOS_EB_US_CHAIN isn't created
		va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N", QOS_EB_US_CHAIN);
		va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P", QOS_EB_US_CHAIN, "RETURN");
		va_cmd(EBTABLES, 6, 1, "-t", "broute", "-A", "BROUTING", "-j", QOS_EB_US_CHAIN);
	}

	/* Set DSCP value */
	sprintf(cmd_buf,"%s -t %s -A %s -s %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx -j ftos --set-ftos 0x%x",
		EBTABLES, "broute", QOS_EB_US_CHAIN, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],dscp);
	system(cmd_buf);	
	sprintf(macString, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	sprintf(cmd, "echo %s >> %s", macString, DSCP_REMAKR_FILE);
	system(cmd);
	return 0;
}

int unset_dscp_for_device(char *mac,unsigned char dscp)
{
	char cmd_buf[200]={0};
	FILE *fp, *fp_tmp;
	char line[64];
	char macString[32]={0};
	char tmpString[32]={0};

	if(!(fp = fopen(DSCP_REMAKR_FILE, "r")))
		return -2;

	if(!(fp_tmp = fopen(DSCP_REMAKR_TEMP_FILE, "w")))
		return -2;

	sprintf(tmpString, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	while(fgets(line, 64, fp) != NULL)
	{
		sscanf(line, "%s\n", macString);
		if(strcmp(tmpString, macString)==0) {
			/* Unset DSCP value */
			sprintf(cmd_buf,"%s -t %s -D %s -s %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx -j ftos --set-ftos 0x%x",
				EBTABLES, "broute", QOS_EB_US_CHAIN, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],dscp);
			system(cmd_buf);
		} else {
			fprintf(fp_tmp, "%s\n", macString);
		}
	}

	fclose(fp);
	fclose(fp_tmp);
	unlink(DSCP_REMAKR_FILE);
	rename(DSCP_REMAKR_TEMP_FILE, DSCP_REMAKR_FILE);
	return 0;
}

int get_rule_for_dscp_mark(unsigned char *smac)
{
	FILE *fp=NULL;
	char line[64];
	char macString[32]={0};
	char tmpString[32]={0};
	int  ret=0;

	if(!(fp = fopen(DSCP_REMAKR_FILE, "r")))
		return 0;

	sprintf(tmpString, "%02x:%02x:%02x:%02x:%02x:%02x", smac[0], smac[1], smac[2], smac[3], smac[4], smac[5]);
	while(fgets(line, 64, fp) != NULL)
	{
		sscanf(line, "%s\n", macString);
		if(strcmp(tmpString, macString)==0) {
			ret = 1;
			break;
		} 
	}

	fclose(fp);
	return ret;
}

#if defined(CONFIG_CU)
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
int rtk_fc_setup_fastpath_speedup_rule(MIB_CU_FASTPATH_SPEEDUP_RULE_T *entry, int index, int type)
{
	rt_flow_ops_t flow_op;
	rt_flow_ops_data_t flow_ops_data;
	uint32 ipaddr=0;

	memset(&flow_ops_data, 0 , sizeof(flow_ops_data));
	flow_op = RT_FLOW_OPS_HIPRIFLOW_PTN_ACT;
	if(type == 1)
		flow_ops_data.data_hiPriflowPtnAct.hipriflow_ptn_act = RT_FLOW_OP_HIPRIFLOW_PTN_ACT_ADD_BY_IDX;
	else
		flow_ops_data.data_hiPriflowPtnAct.hipriflow_ptn_act = RT_FLOW_OP_HIPRIFLOW_PTN_ACT_DEL_BY_IDX;

	flow_ops_data.data_hiPriflowPtnAct.index = index;
	if(strlen(entry->SourceIP))
	{
		inet_aton(entry->SourceIP, (struct in_addr *)&ipaddr);
		flow_ops_data.data_hiPriflowPtnAct.flowPattern.sip4_valid = 1;
		flow_ops_data.data_hiPriflowPtnAct.flowPattern.sip4 = ntohl(ipaddr);
	}
	if(entry->SourcePort)
	{
		flow_ops_data.data_hiPriflowPtnAct.flowPattern.sport_valid = 1;
		flow_ops_data.data_hiPriflowPtnAct.flowPattern.sport = entry->SourcePort;
	}

	if(strlen(entry->DestIP))
	{
		inet_aton(entry->DestIP, (struct in_addr *)&ipaddr);
		flow_ops_data.data_hiPriflowPtnAct.flowPattern.dip4_valid = 1;
		flow_ops_data.data_hiPriflowPtnAct.flowPattern.dip4 = ntohl(ipaddr);
	}
	if(entry->DestPort)
	{
		flow_ops_data.data_hiPriflowPtnAct.flowPattern.dport_valid = 1;
		flow_ops_data.data_hiPriflowPtnAct.flowPattern.dport = entry->DestPort;
	}

	if(strlen(entry->Protocol))
	{
		if(strcmp(entry->Protocol, "ALL") == 0)
			flow_ops_data.data_hiPriflowPtnAct.flowPattern.l4proto_valid = 0;
		else 
		{
			flow_ops_data.data_hiPriflowPtnAct.flowPattern.l4proto_valid = 1;
			if(strcmp(entry->Protocol, "TCP") == 0)
				flow_ops_data.data_hiPriflowPtnAct.flowPattern.l4proto = RT_FLOW_OP_FLOW_PATTERN_L4PROTO_TCP;
			else if(strcmp(entry->Protocol, "UDP") == 0)
				flow_ops_data.data_hiPriflowPtnAct.flowPattern.l4proto = RT_FLOW_OP_FLOW_PATTERN_L4PROTO_UDP;
		}
	}

	if(rt_flow_operation_add(flow_op, &flow_ops_data)) 
		return -1;
	
	return 0;
}

int rtk_fc_add_fastpath_speedup_rule(MIB_CU_FASTPATH_SPEEDUP_RULE_T *entry, int index)
{
	return(rtk_fc_setup_fastpath_speedup_rule(entry, index, 1));
}

int rtk_fc_del_fastpath_speedup_rule(MIB_CU_FASTPATH_SPEEDUP_RULE_T *entry, int index)
{
	return(rtk_fc_setup_fastpath_speedup_rule(entry, index, 0));
}

#endif
#endif

#ifdef COM_CUC_IGD1_WMMConfiguration
int getWMMSupportSSID(int instnum)
{
	int i, num, wlaninst=0;
	MIB_CE_WMM_CONFIGURATION_T entry, *pEntry;

	num = mib_chain_total(MIB_WMM_CONFIGURATION_TBL);	
	for(i = 0; i < num; i++)
	{
		pEntry = &entry;
		if(!mib_chain_get(MIB_WMM_CONFIGURATION_TBL,i, pEntry))
			continue;   	

		if(pEntry->InstanceNum == instnum)
		{
			sscanf( pEntry->wmmssid, "SSID%d", &wlaninst );
			break;
		}
	}

	return wlaninst;
}

int getWMMEnable(int instnum)
{
	int i, num, wmmenable=0;
	MIB_CE_WMM_CONFIGURATION_T entry, *pEntry;

	num = mib_chain_total(MIB_WMM_CONFIGURATION_TBL);	
	for(i = 0; i < num; i++)
	{
		pEntry = &entry;
		if(!mib_chain_get(MIB_WMM_CONFIGURATION_TBL,i, pEntry))
			continue;   	

		if(pEntry->InstanceNum == instnum)
		{
			wmmenable = pEntry->wmmenable;
			break;
		}
	}

	return wmmenable;
}

const char WMM_SERVICE_CHAIN[] =  "wmm_service_rules";
#define WMM_SERVICERULE_ADD  1
#define WMM_SERVICERULE_MOD  2
#define WMM_SERVICERULE_ACTION_APPEND(type) (type==WMM_SERVICERULE_ADD)?"-A":"-D"

int rtk_setup_wmm_service_rule(MIB_CE_WMM_SERVICE_T *entry, int type)
{
	char cmd[512] = {0}, cmd2[512] = {0};
	char tmp[128] = {0}, tmp2[128] = {0};
	char tmpstr[48]={0};
	char wlan_ifname[16]={0};
	unsigned int wlaninst=0, iptableMark=0;
	unsigned short qid=0;
	
	DOCMDINIT;

	wlaninst = getWMMSupportSSID(entry->wmmconfinst);
	if(wlaninst)
	{
		rtk_wlan_get_ifname_by_ssid_idx(wlaninst, wlan_ifname);
	}

	printf("%s:%d RuleName %s, sip %s, sport %d, dip %s, dport %d, protoType %d, ACLevel %s\n", __FUNCTION__, __LINE__, 
		entry->RuleName, 
		entry->sip, 
		entry->sPort,
		entry->dip, 
		entry->dPort,
		entry->protoType,
		entry->ACLevel);	

	if(entry->protoType == PROTO_TCP)	
	{
		sprintf(tmp, " -t mangle %s %s -p TCP", WMM_SERVICERULE_ACTION_APPEND(type),WMM_SERVICE_CHAIN);
	}
	else if(entry->protoType == PROTO_UDP)	
	{
		sprintf(tmp, " -t mangle %s %s -p UDP", CMCC_FORWARDRULE_ACTION_APPEND(type),WMM_SERVICE_CHAIN);
	}
	else 
	{
		if((entry->sPort == 0) && (entry->dPort == 0))
		{
			sprintf(tmp, " -t mangle %s %s", CMCC_FORWARDRULE_ACTION_APPEND(type),WMM_SERVICE_CHAIN);
		}
		else
		{
			sprintf(tmp, " -t mangle %s %s -p TCP", WMM_SERVICERULE_ACTION_APPEND(type),WMM_SERVICE_CHAIN);
			sprintf(tmp2, " -t mangle %s %s -p UDP", WMM_SERVICERULE_ACTION_APPEND(type),WMM_SERVICE_CHAIN);
		}
	}

	strcat(cmd, tmp);
	if(strlen(tmp2))
		strcat(cmd2, tmp2);

	if(wlaninst)
	{
		memset(tmp, 0, 128);
		sprintf(tmp, " -m physdev --physdev-out %s", wlan_ifname);
		strcat(cmd, tmp);
		if(strlen(tmp2))
			strcat(cmd2, tmp);
	}

	memset(tmp, 0, 128);
	if(entry->sPort != 0)
		sprintf(tmp, " --sport %d", entry->sPort);
	strcat(cmd, tmp);
	if(strlen(tmp2))
		strcat(cmd2, tmp);

	memset(tmp, 0, 128);
	if(entry->dPort != 0)
		sprintf(tmp, " --dport %d", entry->dPort);
	strcat(cmd, tmp);
	if(strlen(tmp2))
		strcat(cmd2, tmp);

	memset(tmp, 0, 128);
	if(memcmp(entry->sip, "\x00\x00\x00\x00", IP_ADDR_LEN) != 0)
	{
		snprintf(tmpstr, 16, "%s", inet_ntoa(*((struct in_addr *)entry->sip)));
		if(entry->smaskbit)
			sprintf(tmp, " -s %s/%d", tmpstr, entry->smaskbit);
		else
			sprintf(tmp, " -s %s", tmpstr);
	}
	strcat(cmd, tmp);
	if(strlen(tmp2))
		strcat(cmd2, tmp);

	memset(tmp, 0, 128);
	if(memcmp(entry->dip, "\x00\x00\x00\x00", IP_ADDR_LEN) != 0)
	{
		snprintf(tmpstr, 16, "%s", inet_ntoa(*((struct in_addr *)entry->dip)));
		if(entry->dmaskbit)
			sprintf(tmp, " -d %s/%d", tmpstr, entry->dmaskbit);
		else
			sprintf(tmp, " -d %s", tmpstr);
	}
	strcat(cmd, tmp);
	if(strlen(tmp2))
		strcat(cmd2, tmp);

	memset(tmp, 0, 128);
	if(entry->prior>0)
	{
		qid = (getIPQosQueueNumber()- (entry->prior));
		iptableMark |= (qid)<<SOCK_MARK_QOS_SWQID_START;
		sprintf(tmp, " -j MARK --set-mark 0x%x/0x%x", iptableMark, SOCK_MARK_QOS_MASK_ALL);
	}
	strcat(cmd, tmp);
	if(strlen(tmp2))
		strcat(cmd2, tmp);

	printf("%s:%d cmd %s\n", __FUNCTION__, __LINE__, cmd);
	DOCMDARGVS(IPTABLES, DOWAIT, cmd);

	if(strlen(cmd2))
	{
		printf("%s:%d cmd2 %s\n", __FUNCTION__, __LINE__, cmd2);
		DOCMDARGVS(IPTABLES, DOWAIT, cmd2);
	}
	
	return 0;
}

int rtk_fc_add_wmm_service_rule(MIB_CE_WMM_SERVICE_T *entry)
{
	return(rtk_setup_wmm_service_rule(entry, 1));
}

int rtk_fc_del_wmm_service_rule(MIB_CE_WMM_SERVICE_T *entry)
{
	return(rtk_setup_wmm_service_rule(entry, 0));
}

int set_wmm_service_tbl()
{
	// iptables -t mangle -N wmm_service_rules
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", WMM_SERVICE_CHAIN);
	// iptables -t mangle -A FORWARD -j wmm_service_rules
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", WMM_SERVICE_CHAIN);
	
	return 0;
}

int flush_wmm_service_tbl()
{
	// iptables -t mangle -D FORWARD -j wmm_service_rules
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", WMM_SERVICE_CHAIN);
	// iptables -t mangle -F wmm_service_rules
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", WMM_SERVICE_CHAIN);
	// iptables -t mangle -X wmm_service_rules
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", WMM_SERVICE_CHAIN);	
	return 0;
}

#endif


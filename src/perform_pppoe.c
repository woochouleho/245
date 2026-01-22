#include <stdio.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <math.h>
#include <signal.h>


#include "boa.h"
#include "cJSON.h"
//#include "/LINUX/mib.h"

#ifdef BOA_WITH_OPENSSL
#include <openssl/ssl.h>
#endif

//#ifdef SUPPORT_ASP
#include "asp_page.h"
//#endif
#include "./LINUX/utility.h"

//#ifdef SUPER_NAME_SUPPORT
//	#include "auth.h"
//#endif 
#define RTL_LINK_CHK_PING_FILE		"/var/tmp/tmp_ping"
#define PPPD_ERROR_FILE             "/tmp/ppp_error_log"

const char DHCPC_PID[] = "/var/run/udhcpc.pid";
#define RTL_LINK_LIMIT_FLIE "/tmp/overlimit_info"

typedef struct URLInfo_s {
	unsigned char  username[MAX_NAME_LEN];
	unsigned char password[MAX_NAME_LEN];
	unsigned char pppoename[MAX_PPP_NAME_LEN+1];
	unsigned char pppoepwd[MAX_NAME_LEN];
	unsigned char action[MAX_PPP_NAME_LEN];
	unsigned char area[MAX_PPP_NAME_LEN];
	int isconnectPPP;
	unsigned char sn[ANDLINK_SN_NUM];
}URLInfo;
static char globel_area[RTL_LINK_MAX_INFO_LEN];
int register_num =0;
#ifdef CONFIG_USER_ANDLINK_PLUGIN
static char restart_plugin;
#endif
int getFileValue(char *filename, int *value)
{
	FILE *fp = NULL;
	char buff[1024] = {0};
	char *pchar = NULL;

	if(isFileExist(filename)){
		fp = fopen(filename, "r");
		if(fp==NULL){
			printf("open %s fail \n", filename);
			return -1;
		}
		if(strncmp(filename, PPPD_ERROR_FILE, strlen(PPPD_ERROR_FILE)) == 0){
			while (fgets(buff, sizeof(buff), fp)){
				buff[strlen(buff)-1]='\0';
				if((pchar = strstr(buff, ":")) != NULL){
					pchar += 1;
					*value = atoi(pchar);
				}
			}
		}else{
			while (fgets(buff, sizeof(buff), fp)) {
				buff[strlen(buff)-1]='\0';
				*value = atoi(buff);
				break;
			}
		}
		fclose(fp);
	}else{
		printf("%s doesn't exist \n", filename);
		return -1;
	}
	
	return 0;
}

static int rtl_link_parse_ping_file(char *filename)
{
	int ret=0;
	FILE *fp=NULL;
	char buffer[200]={0};

	if((fp = fopen(filename,"r")) == NULL)
		return ret;	

	/*
		PING www.baidu.com (180.97.33.107): 56 data bytes
		64 bytes from 180.97.33.107: seq=0 ttl=55 time=6.469 ms

		--- www.baidu.com ping statistics ---
		1 packets transmitted, 1 packets received, 0% packet loss
	*/

	while(fgets(buffer, sizeof(buffer), fp)) {
		if(strstr(buffer, "1 packets transmitted, 1 packets received")){
			ret = 1;
			break;
		}		
	}
	fclose(fp);
	unlink(filename);

	return ret;
}

static int rtl_link_checkWanping(char *server1, char *server2, char *server3)
{
	int ret=0;
	char buffer[128]={0};
	
	//system() returns -1 for error, and other values as command return value, see system(3).
	//ping returns EXIT_SUCCESS for succeed and EXIT_FAILURE or 1 for no response, see busybox ping.c
	
	snprintf(buffer,sizeof(buffer),"ping %s -c 1 2> %s > %s",server1, RTL_LINK_CHK_PING_FILE, RTL_LINK_CHK_PING_FILE);	
	ret=system(buffer);	
	if(ret==0){
		if(!rtl_link_parse_ping_file(RTL_LINK_CHK_PING_FILE))
			return 0;
	}else{
		return 0;
	}

	snprintf(buffer,sizeof(buffer),"ping %s -c 1 2> %s > %s",server2, RTL_LINK_CHK_PING_FILE, RTL_LINK_CHK_PING_FILE);	
	ret=system(buffer);
	if(ret==0){
		if(!rtl_link_parse_ping_file(RTL_LINK_CHK_PING_FILE))
			return 0;
	}else{
		return 0;
	}

	snprintf(buffer,sizeof(buffer),"ping %s -c 1 2> %s > %s",server3, RTL_LINK_CHK_PING_FILE, RTL_LINK_CHK_PING_FILE);
	ret=system(buffer);
	if(ret==0){
		if(!rtl_link_parse_ping_file(RTL_LINK_CHK_PING_FILE))
			return 0;
	}else{
		return 0;
	}
	
	return 1;
}

static int rtl_link_wan_access()
{
	int ret = 0, max_times=1, i;

#ifdef CONFIG_USER_ANDLINK_PLUGIN
	// give more chances to try, in case of packet loss even when WAN is connected.
	max_times = 3;
#endif

	for(i=0; i<max_times; ++i){
		if(rtl_link_checkWanping("www.baidu.com","www.sohu.com","www.163.com")==1){
			ret = 1;
			break;
		}else{
			//sleep(1);
		}
	}

	return ret;
}

int createRsp(cJSON *jsonRsp, char *rspCont)
{

	cJSON_AddStringToObject(jsonRsp, "Result", rspCont);
	return 0;
	
}
void Get_pppoeStatus(URLInfo *urlInfo)
{
	int ret = 0, tmpValue = 0;
	int i, ifcount=0;
	struct wstatus_info sEntry[MAX_VC_NUM+MAX_PPP_NUM];
	unsigned char area[RTL_LINK_MAX_INFO_LEN]={0};

	mib_get(MIB_RTL_LINK_AREA, (void *)area);
	getFileValue(PPPD_ERROR_FILE, &tmpValue);
	/* check if pppoe connection is up */
	ifcount = getWanStatus(sEntry, MAX_VC_NUM+MAX_PPP_NUM);
	for (i=0; i<ifcount; i++)
	{
		if (sEntry[i].cmode == CHANNEL_MODE_PPPOE)
		{
			if (sEntry[i].link_state && !sEntry[i].pppDoD &&
				(sEntry[i].itf_state==1))
			{
				ret = 1;
				break;
			}
		}
	}

	if(ret){
		printf("(%s) %d PPPOE connect!\n",__FUNCTION__,__LINE__);
		urlInfo->isconnectPPP = 1;
	}else if(tmpValue == 14){  //14 means pppoe error code ERROR_AUTHENTICATION_FAILURE
		printf("(%s) %d Device PPPOE Password Wrong!\n",__FUNCTION__,__LINE__);
		urlInfo->isconnectPPP = 2;
	}else if(strncmp(area, globel_area, strlen(area))){
		printf("(%s) %d Device Area Dismatch!\n",__FUNCTION__,__LINE__);
		urlInfo->isconnectPPP = 3;
	}else{
		printf("(%s) %d PPPOE disconnect!\n",__FUNCTION__,__LINE__);
		urlInfo->isconnectPPP = 0;
	}
}
int Set_pppoe(URLInfo *urlInfo){
	int ifIndex_offset=0;
	MIB_CE_ATM_VC_T wanEntry;

	memset(&wanEntry, 0, sizeof(wanEntry));

 	if(!mib_chain_get(MIB_ATM_VC_TBL,0,(void *)&wanEntry))
	{	
		printf("\nget wanIface mib error!\n");
		return -1;
	}

	ifIndex_offset=ETH_INDEX(wanEntry.ifIndex);

	deleteConnection(CONFIGONE, &wanEntry);

	wanEntry.applicationtype=X_CT_SRV_INTERNET;
	wanEntry.napt=1;
#ifdef CONFIG_E8B
	wanEntry.dgw=0;
#else
	wanEntry.dgw=1;
#endif

	wanEntry.cmode=CHANNEL_MODE_PPPOE;
	wanEntry.ipDhcp=DHCP_CLIENT;
	wanEntry.ifIndex=0x10000+ifIndex_offset;			
	wanEntry.dnsMode=1;
	wanEntry.mtu=1492;		
	wanEntry.pppCtype=0;
	snprintf(wanEntry.pppUsername, sizeof(wanEntry.pppUsername), "%s", urlInfo->pppoename);			
	snprintf(wanEntry.pppPassword, sizeof(wanEntry.pppPassword), "%s", urlInfo->pppoepwd);
	
	mib_chain_update(MIB_ATM_VC_TBL, (void *)&wanEntry, 0);

	restartWAN(CONFIGONE, &wanEntry);
	
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
#ifdef CONFIG_USER_ANDLINK_PLUGIN
	restart_plugin = 1;
#endif

	return 0;	
}

int Get_pppoeInfo(URLInfo *urlInfo, char *buf)
{
	char buffer[2048], tmpbuff[2048];
	char *pchar=NULL, *pchar_tmp=NULL;
	char *savestr1=NULL;

	snprintf(buffer, sizeof(buffer), "%s", buf);

	//username=CMCCAdmin&pwd=aDmA&pppoename=15979102612&pppoepass=1234567890
	pchar = strtok_r(buffer, "&", &savestr1);
	while(pchar){
		tmpbuff[sizeof(tmpbuff)-1]='\0';
		strncpy(tmpbuff, pchar, sizeof(tmpbuff)-1);
		
		pchar_tmp = strstr(tmpbuff, "=");
		if(pchar_tmp){
			*pchar_tmp = '\0';
		}
		if(strstr(tmpbuff, "username")){
			urlInfo->username[sizeof(urlInfo->username)-1]='\0';
			strncpy((char *)urlInfo->username, pchar_tmp+1, sizeof(urlInfo->username)-1);
		}else if(strstr(tmpbuff, "userpwd")){
			urlInfo->password[sizeof(urlInfo->password)-1]='\0';
			strncpy((char *)urlInfo->password, pchar_tmp+1, sizeof(urlInfo->password)-1);
		}else if(strstr(tmpbuff, "PPPOEuser")){
			urlInfo->pppoename[sizeof(urlInfo->pppoename)-1]='\0';
			strncpy((char *)urlInfo->pppoename, pchar_tmp+1, sizeof(urlInfo->pppoename)-1);
		}else if(strstr(tmpbuff, "PPPOEpassword")){
			urlInfo->pppoepwd[sizeof(urlInfo->pppoepwd)-1]='\0';
			strncpy((char *)urlInfo->pppoepwd, pchar_tmp+1, sizeof(urlInfo->pppoepwd)-1);
		}else if(strstr(tmpbuff, "action")){
			urlInfo->action[sizeof(urlInfo->action)-1]='\0';
			strncpy((char *)urlInfo->action, pchar_tmp+1, sizeof(urlInfo->action)-1);
		}else if(strstr(tmpbuff, "area")){
			urlInfo->area[sizeof(urlInfo->area)-1]='\0';
			strncpy((char *)urlInfo->area, pchar_tmp+1, sizeof(urlInfo->area)-1);
			if(urlInfo->area[0]){
				globel_area[sizeof(globel_area)-1]='\0';
				strncpy(globel_area, urlInfo->area, sizeof(globel_area)-1);
			}
		}else if(strstr(tmpbuff, "sn")){
			urlInfo->sn[sizeof(urlInfo->sn)-1]='\0';
			strncpy((char *)urlInfo->sn, pchar_tmp+1, sizeof(urlInfo->sn));
		}
		
		pchar = strtok_r(NULL, "&", &savestr1);
	}
	
	return 0;
}

void Pppoe_handleJson(request *req , char *buf)
{
	URLInfo urlInfo;
	char user_name[MAX_NAME_LEN], user_password[MAX_PASSWD_LEN], sn_num[ANDLINK_SN_NUM];
	cJSON *jsonRsp = NULL;
	char *jsonData = NULL;
	int opMode=0, tmpValue=0;
	
	//send_r_request_ok2(req);		/* All's well */
	send_r_request_ok(req);		/* All's well */

	memset(&urlInfo, 0, sizeof(URLInfo));
	jsonRsp = cJSON_CreateObject();
	
	Get_pppoeInfo(&urlInfo, buf);
	printf("[%s %d] urlInfo: \n",__FUNCTION__,__LINE__);
	printf("    urlInfo.username:%s\n",urlInfo.username);
	printf("    urlInfo.password:%s\n",urlInfo.password);
	printf("    urlInfo.pppoename:%s\n",urlInfo.pppoename);
	printf("    urlInfo.pppoepwd:%s\n",urlInfo.pppoepwd);
	printf("    urlInfo.action:%s\n",urlInfo.action);
	printf("    urlInfo.area:%s\n",urlInfo.area);
	
	printf("    globel_area:%s\n",globel_area);
	//compare username and password
	mib_get(MIB_USER_NAME, (void *)user_name);
	mib_get(MIB_USER_PASSWORD, (void *)user_password);
	printf("[%s %d] MIB_USER_NAME:%s, MIB_USER_PASSWORD:%s\n",__FUNCTION__,__LINE__,user_name,user_password);
	if(strncmp(user_name, urlInfo.username, sizeof(user_name)) || strncmp(user_password, urlInfo.password, sizeof(user_password))){
	//if(strncmp(user_name, urlInfo.username, sizeof(user_name))){
		//response Adminauth Failed
		printf("(%s) %d Device Adminauth Failed!\n",__FUNCTION__,__LINE__);
		createRsp(jsonRsp, "Auth Failed");
		jsonData = cJSON_PrintUnformatted(jsonRsp);
		//req_format_write(req,jsonData);
		boaWrite(req, "%s", jsonData);
		goto EXIT;
	}
	
	//set pppoe,call reinit
	if(!urlInfo.action[0]){
		if(isFileExist(RTL_LINK_LIMIT_FLIE)){
			register_num=0;
			unlink(RTL_LINK_LIMIT_FLIE);
		}
		else
			register_num++;
		mib_get(MIB_OP_MODE,(void *)&opMode);
		if(opMode == BRIDGE_MODE){
			printf("(%s) %d Device is in bridge mode, can't initialize PPPOE!\n",__FUNCTION__,__LINE__);
			createRsp(jsonRsp, "Bridge Mode Failed");
			jsonData = cJSON_PrintUnformatted(jsonRsp);
			//req_format_write(req,jsonData);
			boaWrite(req, "%s", jsonData);
			goto EXIT;
		}

		if(!isFileExist("/var/run/spppd.pid")){
			if(!Set_pppoe(&urlInfo)){
			//response Success
				printf("(%s) %d Device Successfully initializing PPPOE!\n",__FUNCTION__,__LINE__);
				createRsp(jsonRsp, "Default Configuration Succ");
				jsonData = cJSON_PrintUnformatted(jsonRsp);
				//req_format_write(req,jsonData);
				boaWrite(req, "%s", jsonData);
				goto EXIT;
			}
		}else{
			if(!Set_pppoe(&urlInfo)){
				printf("(%s) %d Device is running PPPOE!\n",__FUNCTION__,__LINE__);
				createRsp(jsonRsp, "Register is running");
				jsonData = cJSON_PrintUnformatted(jsonRsp);
				//req_format_write(req,jsonData);
				boaWrite(req, "%s", jsonData);
				goto EXIT;
			}
		}
		mib_get(MIB_RTL_LINK_SN, (void *)sn_num);
		if(strncmp(sn_num,urlInfo.sn,sizeof(sn_num))){
			printf("(%s) %d Device sn is incorrect!\n",__FUNCTION__,__LINE__);
			createRsp(jsonRsp, "SN ERROR");
			jsonData = cJSON_PrintUnformatted(jsonRsp);
			//req_format_write(req,jsonData);
			boaWrite(req, "%s", jsonData);
			goto EXIT;
		}
		if(register_num>ANDLINK_RETRY_LIMIT_NUM){
			printf("(%s) %d Device register num is over limit!\n",__FUNCTION__,__LINE__);
			createRsp(jsonRsp, "Over Limit");
			jsonData = cJSON_PrintUnformatted(jsonRsp);
			//req_format_write(req,jsonData);
			boaWrite(req, "%s", jsonData);
			goto EXIT;
		}
		//what about "SN error" and "Over Limit"
			
	}else{
		Get_pppoeStatus(&urlInfo);
		if(isFileExist("/var/run/spppd.pid")){
			if(urlInfo.isconnectPPP == 1){
				printf("(%s) %d Device PPPOE Connect Success!\n", __FUNCTION__,__LINE__);
				//TODO: internet connected or not
				if(rtl_link_wan_access()==1){
					printf("(%s) %d Device Wan Access Success!\n",__FUNCTION__,__LINE__);
#ifdef CONFIG_USER_ANDLINK_PLUGIN
					//restart andlink plugin to get correct status code
					if(restart_plugin)
					{
						start_andlink_plugin();
						restart_plugin = 0;
					}
#endif
					//TODO: judge if6 connection status
					getFileValue(ANDLINK_IF6_STATUS, &tmpValue);
					if(tmpValue == REGISTERING){
						//reponse Registering Intelligent Platform
						printf("(%s) %d Device reponse Registering Intelligent Platform!\n",__FUNCTION__,__LINE__);
						createRsp(jsonRsp, "Registering Intelligent Platform");
						jsonData = cJSON_PrintUnformatted(jsonRsp);
						//req_format_write(req,jsonData);
						boaWrite(req, "%s", jsonData);
						goto EXIT;
					}else if(tmpValue == REGISTER_SUCCESS){
						//response Reg And Service Succ
						printf("(%s) %d Device reponse Reg And Service Succ!\n",__FUNCTION__,__LINE__);
						createRsp(jsonRsp, "Reg And Service Succ");
						jsonData = cJSON_PrintUnformatted(jsonRsp);
						//req_format_write(req,jsonData);
						boaWrite(req, "%s", jsonData);
						goto EXIT;
					}else if(tmpValue == REGISTER_ERROR){
						//response Intelligent Platform Normal Error
						printf("(%s) %d Device reponse Intelligent Platform Normal Error!\n",__FUNCTION__,__LINE__);
						createRsp(jsonRsp, "Intelligent Platform Normal Error");
						jsonData = cJSON_PrintUnformatted(jsonRsp);
						//req_format_write(req,jsonData);
						boaWrite(req, "%s", jsonData);
						goto EXIT;
					}else if(tmpValue == AUTH_SUCCESS){
						//response Intelligent Platform Auth Succ
						printf("(%s) %d Device reponse Intelligent Platform Auth Succ!\n",__FUNCTION__,__LINE__);
						createRsp(jsonRsp, "Intelligent Platform Auth Succ");
						jsonData = cJSON_PrintUnformatted(jsonRsp);
						//req_format_write(req,jsonData);
						boaWrite(req, "%s", jsonData);
						goto EXIT;
					}else if(tmpValue == AUTH_FAIL){
						//response Intelligent Platform Auth Failed
						printf("(%s) %d Device reponse Intelligent Platform Auth Failed!\n",__FUNCTION__,__LINE__);
						createRsp(jsonRsp, "Intelligent Platform Auth Failed");
						jsonData = cJSON_PrintUnformatted(jsonRsp);
						//req_format_write(req,jsonData);
						boaWrite(req, "%s", jsonData);
						goto EXIT;
					}else if(tmpValue == RE_REGISTER){
						//response Re-Registering Intelligent Platform
						printf("(%s) %d Device reponse Re-Registering Intelligent Platform!\n",__FUNCTION__,__LINE__);
						createRsp(jsonRsp, "Re-Registering Intelligent Platform");
						jsonData = cJSON_PrintUnformatted(jsonRsp);
						//req_format_write(req,jsonData);
						boaWrite(req, "%s", jsonData);
						goto EXIT;
					}else if(tmpValue == REGISTER_ANOTHER_SRV){
						//response Intelligent Platform Register to Another Server
						printf("(%s) %d Device reponse Intelligent Platform Register to Another Server!\n",__FUNCTION__,__LINE__);
						createRsp(jsonRsp, "Intelligent Platform Register to Another Server");
						jsonData = cJSON_PrintUnformatted(jsonRsp);
						//req_format_write(req,jsonData);
						boaWrite(req, "%s", jsonData);
						goto EXIT;
					}
					
				}else{
					//reponse Getting Address Failed
					printf("(%s) %d Device Wan Access Failed!\n",__FUNCTION__,__LINE__);
					createRsp(jsonRsp, "Getting Address Failed");
					jsonData = cJSON_PrintUnformatted(jsonRsp);
					//req_format_write(req,jsonData);
					boaWrite(req, "%s", jsonData);
					goto EXIT;
				}
			}else if(urlInfo.isconnectPPP == 2){  //PPPOE password wrong
					printf("(%s) %d Device PPPOE Fail!\n",__FUNCTION__,__LINE__);
					createRsp(jsonRsp, "PPPOE Failed");
					jsonData = cJSON_PrintUnformatted(jsonRsp);
					//req_format_write(req,jsonData);
					boaWrite(req, "%s", jsonData);
					goto EXIT;
			}else if(urlInfo.isconnectPPP == 3){ //Area wrong
				//reponse Getting Address Failed
				printf("(%s) %d Device reponse Getting Address Failed!\n",__FUNCTION__,__LINE__);
				createRsp(jsonRsp, "Getting Address Failed");
				jsonData = cJSON_PrintUnformatted(jsonRsp);
				//req_format_write(req,jsonData);
				boaWrite(req, "%s", jsonData);
				goto EXIT;
			}else{
				//response Getting WAN Address
				printf("(%s) %d Device is getting WAN Address!\n",__FUNCTION__,__LINE__);
				createRsp(jsonRsp, "Getting WAN Address");
				jsonData = cJSON_PrintUnformatted(jsonRsp);
				//req_format_write(req,jsonData);
				boaWrite(req, "%s", jsonData);
				goto EXIT;
			}
		}else{
			printf("(%s) %d Device PPPOE Fail!\n",__FUNCTION__,__LINE__);
			createRsp(jsonRsp, "PPPOE Fail");
			jsonData = cJSON_PrintUnformatted(jsonRsp);
			//req_format_write(req,jsonData);
			boaWrite(req, "%s", jsonData);
			goto EXIT;
		}
	}

EXIT:
	printf("Sending %s\n",jsonData);
	if(jsonRsp != NULL)
		cJSON_Delete(jsonRsp);
	if(jsonData != NULL)
		free(jsonData);
		
	update_content_length(req);
	
	req->buffer[req->buffer_end] = '\0';
	
	//printf("[%s %d] response buffer:%s\n",__FUNCTION__,__LINE__,req->buffer);
	req->status = CLOSE;
	freeAllTempStr();
	freeAllASPTempStr();
	printf("\n ------>End Process Zero Config Requset\n");
	return;
}

int Pppoe_init_json(request * req)
{
	struct stat statbuf;
	SQUASH_KA(req);
	complete_env(req);
	char *buf;
	
	buf=(char *)malloc(req->max_buffer_size*2+1);
	if(buf==NULL) {
		printf("(%s) %d malloc buf fail!\n",__FUNCTION__,__LINE__);
		return 0;
	}
	printf("\n Start Process Zero Config Requset------>\n");
	memcpy(buf, req->header_line, req->max_buffer_size*2+1);
	Pppoe_handleJson(req, buf);

	if (buf != NULL){
		free(buf);
		buf = NULL;
	}
	return 0;
	//return 1;

}



#include "asp_page.h"
#define __USE_GNU
#include "port.h" // ANDREW
#include <ctype.h>
#include "./LINUX/rtl_flashdrv_api.h"
#include "./LINUX/utility.h"
#ifdef SUPPORT_ASP
#ifdef EMBED
#include <linux/config.h>
#else
#include "../../../include/linux/autoconf.h"
#endif
#include <stdio.h>
#include <stdlib.h>
//#define __USE_GNU
#include <string.h>
#include "boa.h"
//ANDREW #include "rtl865x/rome_asp.c"
#ifdef CONFIG_USER_BOA_CSRF
#ifdef USE_LIBMD5
#include <libmd5wrapper.h>
#else
#include "md5.h"
#endif //USE_LIBMD5
#endif

asp_name_t *root_asp=NULL;
form_name_t *root_form=NULL;
temp_mem_t root_temp;
temp_mem_t root_asp_temp;
char *query_temp_var=NULL;

//ANDREW int boaASPDefine(char* name, int (*fn)(request * req,  int argc, char **argv))
int boaASPDefine(const char* const name, int (*fn)(int eid, request * req,  int argc, char **argv))
{
	asp_name_t *asp;
	asp=(asp_name_t*)malloc(sizeof(asp_name_t));
	if(asp==NULL) return ERROR;
	/*
	asp->pagename=(char *)malloc(strlen(name)+1);
	strcpy(asp->pagename,name);
	*/
	asp->pagename = name; // andrew, use the constant directly

	asp->next=NULL;
	asp->function=fn;


	if(root_asp==NULL)
		root_asp=asp;
	else
	{
		// prepend the newly-created asp to the list pointed by root_asp
		asp->next = root_asp;
		root_asp = asp;
	}
	return SUCCESS;
}

#ifdef MULTI_USER_PRIV
typedef enum {PRIV_USER=0, PRIV_ENG=1, PRIV_ROOT=2};	// copy from mib.h
struct formPrivilege formPrivilegeList[] = {
	{"formTcpipLanSetup", PRIV_ROOT},
#ifdef CONFIG_USER_LAN_VLAN_TRANSLATE
	{"formLanVlanSetup", PRIV_ROOT},
#endif
#ifdef _CWMP_MIB_
	{"formTR069Config", PRIV_ROOT},
	{"formTR069CPECert", PRIV_ROOT},
	{"formTR069CACert", PRIV_ROOT},
#endif
#ifdef CONFIG_CMCC_ENTERPRISE
	{"formTR069ConfigStun", PRIV_ROOT},
#endif
#ifdef PORT_FORWARD_GENERAL
	{"formPortFw", PRIV_ROOT},
#endif
#ifdef NATIP_FORWARDING
	{"formIPFw", PRIV_ROOT},
#endif
#ifdef PORT_TRIGGERING_STATIC
	{"formGaming", PRIV_ROOT},
#endif
#ifdef PORT_TRIGGERING_DYNAMIC
	{"formPortTrigger", PRIV_ROOT},
#endif
	{"formFilter", PRIV_ROOT},
#ifdef LAYER7_FILTER_SUPPORT
	{"formlayer7", PRIV_ROOT},
#endif
#ifdef PARENTAL_CTRL
	{"formParentCtrl", PRIV_ROOT},
#endif
	{"formDMZ", PRIV_ROOT},
	{"formFirewall", PRIV_ROOT},
#ifdef CONFIG_TRUE
	{"formRmoteAccess", PRIV_ROOT},
#endif
	{"formPasswordSetup", PRIV_ROOT},
	{"formUserPasswordSetup", PRIV_USER},
#ifdef ACCOUNT_CONFIG
	{"formAccountConfig", PRIV_ROOT},
#endif
#ifdef WEB_UPGRADE
	{"formUpload", PRIV_ENG},
#endif
	{"formSaveConfig", PRIV_ENG},
	{"formSnmpConfig", PRIV_ROOT},
	{"formRadvdSetup", PRIV_ROOT},
	{"formSetAdsl", PRIV_ROOT},
	{"formAdslSafeMode", PRIV_ROOT},
	{"formStatAdsl", PRIV_USER},
	{"formDiagAdsl", PRIV_ROOT},
	{"formStats", PRIV_USER},
	{"formPonStats", PRIV_USER},
	{"formRconfig", PRIV_ROOT},
//	{"formSysCmd", PRIV_ROOT},
#ifdef CONFIG_USER_RTK_SYSLOG
	{"formSysLog", PRIV_USER},
#endif
	{"formHolepunching", PRIV_USER},
	{"formLdap", PRIV_USER},
	{"formBankStatus", PRIV_USER},
	{"formSetAdslTone", PRIV_ROOT},
	{"formStatus", PRIV_USER},
	{"formStatusModify", PRIV_ROOT},
	{"formWanAtm", PRIV_ROOT},
	{"formWanAdsl", PRIV_ENG},
	{"formPPPEdit", PRIV_ENG},
	{"formIPEdit", PRIV_ENG},
	{"formBrEdit", PRIV_ENG},
	{"formBridging", PRIV_ROOT},
	{"formRefleshFdbTbl", PRIV_USER},
	{"formRouting", PRIV_ENG},
#ifdef CONFIG_IPV6
	{"formIPv6Routing", PRIV_ENG},
#endif
	{"formRefleshRouteTbl", PRIV_USER},
#ifdef CONFIG_IPV6
	{"formIPv6RefleshRouteTbl", PRIV_USER},
#endif
	{"formDhcpServer", PRIV_ROOT},
#if defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_CU)
	{"formIpRange", PRIV_ROOT},
#endif
	{"formReflashClientTbl", PRIV_USER},
	{"formDNS", PRIV_ROOT},
#ifdef CONFIG_USER_DDNS
	{"formDDNS", PRIV_USER},
#endif
#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_AWIFI_SUPPORT)
	{"rtk_formAWiFi", PRIV_ROOT},
	{"rtk_formAWiFiMAC", PRIV_ROOT},
	{"rtk_formAWiFiURL", PRIV_ROOT},
#ifdef CONFIG_USER_AWIFI_AUDIT_SUPPORT
	{"rtk_formAWiFiAudit", PRIV_ROOT},
#endif
#endif
	{"formDhcrelay", PRIV_ROOT},
	{"formPing", PRIV_USER},
	{"formTracert", PRIV_USER},
#ifdef CONFIG_IPV6
	{"formPing6", PRIV_USER},
	{"formTracert6", PRIV_USER},
#endif
#ifdef CONFIG_USER_TCPDUMP_WEB
	{"formCapture", PRIV_ROOT},
#endif
	{"formOAMLB", PRIV_USER},
#ifdef ADDRESS_MAPPING
	{"formAddressMap", PRIV_ROOT},
#endif
#ifdef FINISH_MAINTENANCE_SUPPORT
	{"formFinishMaintenance", PRIV_ROOT},
#endif
#ifdef ACCOUNT_LOGIN_CONTROL
	{"formAdminLogout", PRIV_ROOT},
	{"formUserLogout", PRIV_USER},
#endif
#ifdef USE_LOGINWEB_OF_SERVER
	{"userLogin", PRIV_USER},
	{"formLogout", PRIV_USER},
#endif
	{"formReboot", PRIV_USER},
	{"formDhcpMode", PRIV_ROOT},
	{"formSsh", PRIV_ROOT},
#ifdef CONFIG_USER_IGMPPROXY
	{"formIgmproxy", PRIV_ROOT},
#endif
	{"formMLDSnooping", PRIV_ROOT},    // Mason Yu. MLD snooping
#if defined(CONFIG_USER_UPNPD)||defined(CONFIG_USER_MINIUPNPD)
	{"formUpnp", PRIV_ROOT},
#endif
#if defined(CONFIG_USER_ROUTED_ROUTED) || defined(CONFIG_USER_ZEBRA_OSPFD_OSPFD)
	{"formRip", PRIV_ROOT},
#endif
#ifdef IP_ACL
	{"formACL", PRIV_ROOT},
#endif
#ifdef NAT_CONN_LIMIT
	{"formConnlimit", PRIV_ROOT},
#endif
	{"formmacBase", PRIV_ROOT},
#ifdef URL_BLOCKING_SUPPORT
	{"formURL", PRIV_ROOT},
#endif
#ifdef DOMAIN_BLOCKING_SUPPORT
	{"formDOMAINBLK", PRIV_ROOT},
#endif
#ifdef TIME_ZONE
	{"formNtp", PRIV_ROOT},
#endif
	{"formOthers", PRIV_ROOT},
#ifdef AUTO_PROVISIONING
	{"formAutoProvision", PRIV_ROOT},
#endif
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
#ifdef ELAN_LINK_MODE
	{"formLinkMode", PRIV_ROOT},
#endif
#else // of CONFIG_RTL_MULTI_LAN_DEV
#ifdef ELAN_LINK_MODE_INTRENAL_PHY
	{"formLinkMode", PRIV_ROOT},
#endif
#endif	// of CONFIG_RTL_MULTI_LAN_DEV
	{"formSAC", PRIV_ROOT},
#ifdef WLAN_SUPPORT
	{"formWlanSetup", PRIV_ROOT},
	{"formWirelessTbl", PRIV_USER},
#ifdef WLAN_ACL
	{"formWlAc", PRIV_ROOT},
#endif
	{"formAdvanceSetup", PRIV_ROOT},
#ifdef WLAN_WPA
	{"formWlEncrypt", PRIV_ROOT},
#endif
#ifdef WLAN_WDS
	{"formWlWds", PRIV_ROOT},
	{"formWdsEncrypt", PRIV_ROOT},
#endif
#if defined(WLAN_CLIENT) || defined(WLAN_SITESURVEY)
	{"formWlSiteSurvey", PRIV_ROOT},
#endif
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
	{"formWsc", PRIV_ROOT},
#endif
#ifdef WLAN_11R
	{"formFt", PRIV_ROOT},
#endif
#ifdef CONFIG_USER_CLUSTER_MANAGE
	{"formCLU", PRIV_ROOT},
#endif
#endif	// of WLAN_SUPPORT
#ifdef DIAGNOSTIC_TEST
	{"formDiagTest", PRIV_USER},
#endif
#ifdef DOS_SUPPORT
	{"formDosCfg", PRIV_ROOT},
#endif
#ifdef CONFIG_USER_RTK_NAT_ALG_PASS_THROUGH
	{"formALGOnOff", PRIV_ROOT},
#endif
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
	{"formInternetMode", PRIV_ROOT},
#endif
#ifdef WLAN_SCHEDULE_SUPPORT
		{"formNewSchedule", PRIV_ROOT},
#endif
#if defined(CONFIG_CMCC)
	{"formPermission", PRIV_ENG},
	{"formBundleUpload", PRIV_ENG},
	{"formBundleInstall", PRIV_ENG},
#endif
#ifdef USE_DEONET /* sghong, 200230824 */
	{"formNatSessionSetup", PRIV_ROOT},
#endif

	{"\0", 0}
};

unsigned char getPagePriv(const char* const name)
{
	struct formPrivilege * pformPrivilege = formPrivilegeList;
	while(strlen(pformPrivilege->pagename)) {
		if (strcmp(pformPrivilege->pagename, name) == 0)
			return pformPrivilege->privilege;
		pformPrivilege ++;
	}
	return PRIV_ROOT;
}
#endif

int boaFormDefine(const char* const name, void (*fn)(request * req,  char* path, char* query))
{

	form_name_t *form;
	form=(form_name_t *)malloc(sizeof(form_name_t));
	if(form==NULL) return ERROR;
	/*
	form->pagename=(char *)malloc(strlen(name)+1);
	strcpy(form->pagename,name);
	*/
	form->pagename=name;// andrew, use the constant directly
#ifdef MULTI_USER_PRIV
//	form->privilege = priv;
	form->privilege = getPagePriv(form->pagename);
#endif
	form->next=NULL;
	form->function=fn;

	if(root_form==NULL)
		root_form=form;
	else
	{
		//modify the previous's next pointer
		form_name_t *now_form;
		now_form=root_form;
		while(now_form->next!=NULL) now_form=now_form->next;
		now_form->next=form;
	}
//	printf("boaFormDefine: form->pagename=%s form->privilege=%d\n", form->pagename, form->privilege);
	return SUCCESS;

}

static int addMallocTempStr(char *str, temp_mem_t *root_tmp_str)
{
	temp_mem_t *temp,*newtemp;
	temp=root_tmp_str;
	while(temp->next)
	{
		temp=temp->next;
	}
	newtemp=(temp_mem_t *)malloc(sizeof(temp_mem_t));
	if(newtemp==NULL) return FAILED;
	newtemp->str=str;
	newtemp->next=NULL;
	temp->next=newtemp;
	return SUCCESS;
}

int addTempStr(char *str)
{
	return addMallocTempStr(str, &root_temp);
}

int addASPTempStr(char *str)
{
	return addMallocTempStr(str, &root_asp_temp);
}

int update_content_length(request *req)
{
	char *content_len_orig1;
	char *content_len_orig2;
	int orig_char_length=0;
	int exact_content_len=0;
	int exact_content_str_len=0, byte_shift=0;
	char *content_len_start;
	char buff[32];

	content_len_orig1 = strstr(req->buffer, "Content-Length:");
	if (content_len_orig1==NULL) {//do nothing
		return 0;
	}
	content_len_orig2 = strstr(content_len_orig1, "\r\n");
	content_len_start = strstr((content_len_orig2+2), "\r\n")+strlen("\r\n");
	content_len_orig1 = content_len_orig1 + strlen("Content-Length: ");
	orig_char_length = content_len_orig2 - content_len_orig1;

	exact_content_len = ((req->buffer+req->buffer_end) - content_len_start);
//	printf("[%s:%d] req->filesize=%d, exact_content_len=%d\n",__FUNCTION__,__LINE__,req->filesize, exact_content_len);

	exact_content_str_len = strlen(simple_itoa(exact_content_len));
	if(orig_char_length == exact_content_str_len){
		memcpy(content_len_orig1, simple_itoa(exact_content_len),exact_content_str_len);
	}else if(orig_char_length < exact_content_str_len){
		byte_shift = exact_content_str_len - orig_char_length;
		memmove(content_len_orig2+byte_shift, content_len_orig2, exact_content_len+4);
		memcpy(content_len_orig1, simple_itoa(exact_content_len), exact_content_str_len);
		req->buffer_end += byte_shift;
	}else{
		byte_shift = orig_char_length - exact_content_str_len;
		memmove(content_len_orig2-byte_shift, content_len_orig2, exact_content_len+4);
		memcpy(content_len_orig1, simple_itoa(exact_content_len), exact_content_str_len);
		req->buffer_end -= byte_shift;
	}



	return exact_content_len;
}

static void freeAllMallocTempStr(temp_mem_t *root_tmp_str)
{
	temp_mem_t *temp,*ntemp;
	temp=root_tmp_str->next;
	root_tmp_str->next=NULL;
	while(1)
	{
		if(temp==NULL) break;
		ntemp=temp->next;
		free(temp->str);
		free(temp);
		temp=ntemp;
	}
}

void freeAllTempStr(void)
{
	freeAllMallocTempStr(&root_temp);
}

void freeAllASPTempStr(void)
{
	freeAllMallocTempStr(&root_asp_temp);
}

int getcgiparam_adv(char *dst, int maxlen, char *query_string, int qlen, char *param, int index)
{
	int len, plen, nlen, y;
    int count = 0;
	char *start, *pval, *p;

	plen=strlen(param);
	if(qlen < plen) return -1;

	len = qlen;
	start = query_string;
	while(len>0 && start && (pval = memstr(start, param, len)))
	{
		if((pval == start || *(pval-1) == '&') && *(pval+plen) == '=')
		{
			p = (pval+plen+1);
			len -= (p - start);
			start = p;
			start = memstr(start, "&", len);
			if(start){
				nlen = start - p;
				start += 1;
				len -= (nlen+1);
			}
			else {
				nlen = len;
			}

			if(index < 0 || index == count)
			{
				y = 0;
				while (nlen > 0 && y<maxlen && p && (*p) && (*p != '&'))
				{
					if ((*p == '%') && nlen>2) {
						if ((isxdigit(*(p+1)))&&(isxdigit(*(p+2))))
						{
							dst[y++] = ((toupper(*(p+1))>='A'?toupper(*(p+1))-'A'+0xa:toupper(*(p+1))-'0') << 4)
										+ (toupper(*(p+2))>='A'?toupper(*(p+2))-'A'+0xa:toupper(*(p+2))-'0');
							p += 3;
							nlen -= 3;
							continue;
						}
					}

					if (*p == '+')
						dst[y++] = ' ';
					else
						dst[y++] = *p;
					p++;
					nlen--;
				}
				return y;
			}
			count++;
		}
		else {
			len -= ((pval+plen) - start);
			start = pval+plen;
		}
	}

	return -1;
}

int getcgiparam(char *dst, int maxlen, char *query_string, int qlen, char *param)
{
	return getcgiparam_adv(dst, maxlen, query_string, qlen, param, -1);
}

/* Get form data from multipart/form-data content */
int getcgiparam_multipart(request *req, char *dst, int maxlen, char *query_string, int qlen, char *param)
{
	int y, len, nlen, plen, boundry_len;
	char *start, *p, *pval, boundary[128]={'-', '-'}, found=0;

	plen=strlen(param);
	if(qlen < plen) return -1;

	if(!(start = strstr(req->content_type, "boundary=")))
		return -1;
	start += strlen("boundary=");
	p = strchr(start, ';');
	if(p) boundry_len = p - start;
	else boundry_len = strlen(start);
	if(boundry_len > (sizeof(boundary)-3)) return -1;
	strncpy(boundary+2, start, boundry_len);
	boundry_len += 2;
	boundary[boundry_len] = '\0';

	len = qlen;
	start = query_string;
	while(len>0 && start && (pval = memstr(start, boundary, len))) {
		pval += boundry_len;
		len -= (pval - start);
		start = pval;
		p = memstr(start, "\r\n\r\n", len);
		if(p) {
			p += 4;
			nlen = (p - start);
			pval = memstr(start, "name=", nlen);
			if(pval && (pval=memstr(pval+5, param, (nlen - (pval+5-start)))) &&
				(*(pval+plen)=='"' || *(pval+plen)==';' || *(pval+plen)=='\''))
				found = 1;
			else found = 0;
			len -= nlen;
			start = p;
			if(found) {
				pval = start;
				p = memstr(pval, boundary, len);
				if(p && (nlen = (p-2-pval)) < maxlen) {
					memcpy(dst, pval, nlen);
					*(dst+nlen) = '\0';
					return nlen;
				}
				else return -1;
			}
		}
	}
	return -1;
}

int addEscapeStr(char* data, int datalength)
{
	char specialchar[] = "'\"&><;(){}$";

	char *buf=NULL;
	int i = 0, j = 0;

	for (i = 0; i<datalength ; i++)
	{
		if(!buf)
			buf=(char *)malloc(datalength);
		for (j = 0; j<strlen(specialchar); j++) {
			if (data[i] == specialchar[j])
			{
				memset(buf,0,datalength);
				memcpy(buf,(data+i),datalength-i);
				data[i] = '\\';
				memcpy((data+i+1),buf,datalength-i);
				datalength++;
				i++;
				free(buf);
				buf=NULL;
				break;
			}
		}
	}

	if(buf)
		free(buf);

	return datalength;
}

char *boaGetVar_adv(request *req, char *var, char *defaultGetValue, int index)
{
	char *buf = NULL, *ptr = NULL;
	unsigned int bufSize = 0, searchSize = 0;
	int ret = -1;
	char buf_tmp[MAX_QUERY_TEMP_VAL_SIZE] = {0};

	if (req->method == M_POST)
	{
		if(req->post_data_fd > 0) {
			lseek(req->post_data_fd, 0, SEEK_END);
			bufSize = lseek(req->post_data_fd, 0, SEEK_CUR);
			lseek(req->post_data_fd, 0, SEEK_SET);
		}

		if(bufSize == 0 && req->query_string != NULL) {
			buf = req->query_string;
			ptr = buf;
			searchSize = strlen(req->query_string);
		}
		else if (bufSize > 0 && (buf = mmap(NULL, bufSize, PROT_READ, MAP_SHARED, req->post_data_fd, 0)) != MAP_FAILED)
		{
			ptr = buf;
			searchSize = bufSize;
		}
		else return defaultGetValue;
	}
	else if(req->query_string)
	{
		buf = req->query_string;
		ptr = buf;
		searchSize = strlen(req->query_string);
	}
	else
	{
		buf = ptr = NULL;
		searchSize = 0;
	}

	if(ptr != NULL)
	{
		if(req->content_type != NULL && strstr(req->content_type, "multipart/form-data")) {
			ret = getcgiparam_multipart(req, query_temp_var, MAX_QUERY_TEMP_VAL_SIZE, ptr, searchSize, var);
		}
		else ret = getcgiparam_adv(query_temp_var, MAX_QUERY_TEMP_VAL_SIZE, ptr, searchSize, var, index);
	}

	if(buf != req->query_string && buf) munmap(buf, bufSize);

	//printf("query str=%s ret=%d var=%s query_temp_var=%s\n",buf,ret,var,query_temp_var);
	sprintf(buf_tmp, "%s", query_temp_var);
	ret = addEscapeStr(buf_tmp,ret);

	if(ret<0) return defaultGetValue;

	buf = (char *)malloc(ret+1);
	memset(buf, 0 ,ret+1);
	memcpy(buf, buf_tmp, ret);
	buf[ret]=0;
	addTempStr(buf);

	if (strContainXSSChar(buf)){
		printf("[WARN][%s:%d] The value(%s) set for name(%s) in url(%s) may have the risk of XSS attack!!!\n",__func__,__LINE__,buf,var,req->request_uri);
	}

	return (char *)buf;	//this buffer will be free by freeAllTempStr().
}

char *boaGetVar(request *req, char *var, char *defaultGetValue)
{
	return boaGetVar_adv(req, var, defaultGetValue, -1);
}

unsigned int __flash_base;
unsigned int __crmr_addr;

//ANDREW int32 board_cfgmgr_init(uint8 syncAll);

void boaASPInit(int argc,char **argv)
{
	flashdrv_init();
	rtl8670_AspInit();
	query_temp_var=(char *)malloc(MAX_QUERY_TEMP_VAL_SIZE);
	if(query_temp_var==NULL) exit(0);

#ifdef ANDREW
	int shmid=0;
#ifndef __uClinux__
	__flash_base=rtl865x_mmap(0x1e000000,0x1000000); /* 0xbe000000~0xbeffffff */
	__crmr_addr=rtl865x_mmap(0x1c805000,0x1000)+0x0104; /* 0xbc805104 */
	printf("__flash_base=0x%08x\n",__flash_base);
	printf("__crmr_addr=0x%08x\n",__crmr_addr);
#endif
	shmid=shmget(SHM_PROMECFGPARAM,sizeof(romeCfgParam_t),0666|IPC_CREAT);
	pRomeCfgParam=shmat(shmid,(void*)0,0);

/*
 *	Initialize the memory allocator. Allow use of malloc and start
 *	with a 60K heap.  For each page request approx 8KB is allocated.
 *	60KB allows for several concurrent page requests.  If more space
 *	is required, malloc will be used for the overflow.
 */

	if (strstr(argv[0],"ip-up") || strstr(argv[0],"ip-down"))
	{
		ipupdown(argc,argv);
		goto main_end;
	}

	/* =============================================
		We always init cfgmgr module before all procedures
	============================================= */
	#if defined(CONFIG_RTL865X_DIAG_LED)
	if (board_cfgmgr_init(TRUE) != SUCCESS)
	{
		//re865xIoctl("eth0",SET_VLAN_IP,*(uint32*)(&(pRomeCfgParam->pptpCfgParam.ipAddr)),netmask,0);
		//re865xIoctl(char * name, uint32 arg0, uint32 arg1, uint32 arg2, uint32 arg3);
		re865xIoctl("eth0",RTL8651_IOCTL_DIAG_LED,2,0,0);
	}
	#else
	board_cfgmgr_init(TRUE); //before any flash read/write
	#endif /* CONFIG_RTL865X_DIAG_LED */

	if(argc>1)
	{

		/* =============================================
			Process argument
		============================================= */

		if (!strcmp(argv[1],"default"))
		{
			/*
				user call "boa default"
				--> We must do factory default after init cfgmgr system.
			*/
			cfgmgr_factoryDefault(0);
			printf("cfgmgr_factoryDefault() done!\n");
		}

		else if (!strcmp(argv[1],"save"))
		{
			cfgmgr_task();
			printf("call cfgmgr_task() to save romeCfgParam to flash done!\n");
		}

		else if (!strcmp(argv[1],"-c")) //start
		{
			/* demo system configuration inialization entry point
			 * defined in board.c
			 */
			sysInit();
			goto conti;
		}

		else if (!strcmp(argv[1],"bound"))
		{
			dhcpcIPBound();
		}

		else if (!strcmp(argv[1], "renew"))
		{
			dhcpcIPRenew();
		}

		else if (!strcmp(argv[1],"deconfig"))
		{
			dhcpcDeconfig();
		}

		else if (!strcmp(argv[1],"addacl"))
		{
			acl_tableDriverSingleHandle(ACL_SINGLE_HANDLE_ADD,atoi(argv[2]),atoi(argv[3]));
		}

		else if (!strcmp(argv[1],"delacl"))
		{
			acl_tableDriverSingleHandle(ACL_SINGLE_HANDLE_DEL,atoi(argv[2]),atoi(argv[3]));
		}

		goto main_end;
	}

conti:
	root_temp.next=NULL;
	root_temp.str=NULL;

	boaFormDefine("asp_setLan", asp_setLan);
	boaFormDefine("asp_setDmz", asp_setDmz);
	boaFormDefine("asp_dhcpServerAdvance", asp_dhcpServerAdvance);
	boaFormDefine("asp_setDhcpClient", asp_setDhcpClient);
	boaFormDefine("asp_setPppoeWizard", asp_setPppoeWizard);
	boaFormDefine("asp_setPppoe", asp_setPppoe);
	boaFormDefine("asp_setUnnumberedWizard", asp_setUnnumberedWizard);
	boaFormDefine("asp_setNat", asp_setNat);
	boaFormDefine("asp_setPing", asp_setPing);
	boaFormDefine("asp_setRouting", asp_setRouting);
	boaFormDefine("asp_setPort", asp_setPort);
	boaFormDefine("asp_setStaticWizard", asp_setStaticWizard);
#if defined(CONFIG_RTL865X_PPTPL2TP)||defined(CONFIG_RTL865XB_PPTPL2TP)
	boaFormDefine("asp_setPptpWizard", asp_setPptpWizard);
	boaFormDefine("asp_setL2tpWizard", asp_setL2tpWizard);
	boaFormDefine("asp_setDhcpL2tpWizard", asp_setDhcpL2tpWizard);
#endif
	boaFormDefine("asp_upload", asp_upload);
	boaFormDefine("asp_setUrlFilter", asp_setUrlFilter);
	boaFormDefine("asp_restart", asp_restart);
	boaFormDefine("asp_systemDefault", asp_systemDefault);
	boaFormDefine("asp_wanIp", asp_wanIp);
	boaFormDefine("asp_setAcl", asp_setAcl);
	boaFormDefine("asp_setTZ", asp_setTZ);
	boaFormDefine("asp_setDdns", asp_setDdns);
	boaFormDefine("asp_setSpecialApplication", asp_setSpecialApplication);
	boaFormDefine("asp_setGaming", asp_setGaming);
	boaFormDefine("asp_setServerPort", asp_setServerPort);
	boaFormDefine("asp_setEventLog", asp_setEventLog);
	boaFormDefine("asp_setUpnp", asp_setUpnp);
	boaFormDefine("asp_setDos", asp_setDos);
	boaFormDefine("asp_setDosProc", asp_setDosProc);
	boaFormDefine("asp_setAlg", asp_setAlg);
	boaFormDefine("asp_setBt", asp_setBt);
	boaFormDefine("asp_setUser", asp_setUser);
	boaFormDefine("asp_setMailAlert", asp_setMailAlert);
	boaFormDefine("asp_setRemoteLog", asp_setRemoteLog);
	boaFormDefine("asp_setRemoteMgmt", asp_setRemoteMgmt);
	boaFormDefine("asp_setWlanBasic", asp_setWlanBasic);
	boaFormDefine("asp_setWlanAdvance", asp_setWlanAdvance);
	boaFormDefine("asp_setWlanSecurity", asp_setWlanSecurity);
	boaFormDefine("asp_setWlanAccessCtrl", asp_setWlanAccessCtrl);
	boaFormDefine("asp_selectWlanNeighborAp", asp_selectWlanNeighborAp);
	boaFormDefine("asp_setUdpBlocking", asp_setUdpBlocking);
	boaFormDefine("asp_setQos", asp_setQos);
	boaFormDefine("asp_setQos1", asp_setQos1);
	boaFormDefine("asp_setpseudovlan", asp_setpseudovlan);
	boaFormDefine("asp_setPbnat", asp_setPbnat);
	boaFormDefine("asp_setFlash", asp_setFlash);
	boaFormDefine("asp_setRateLimit", asp_setRateLimit);
	boaFormDefine("asp_setRatio_qos", asp_setRatio_qos);
	boaFormDefine("asp_setModeConfig", asp_setModeConfig);
	boaFormDefine("asp_setRipConfig", asp_setRipConfig);
	boaFormDefine("asp_setPassthru", asp_setPassthru);
	boaFormDefine("asp_setMcast", asp_setMcast);
	boaFormDefine("asp_setNaptOpt", asp_setNaptOpt);
	boaASPDefine("asp_rateLimit", asp_rateLimit);
	boaASPDefine("asp_acl", asp_acl);
	boaASPDefine("asp_dmz", asp_dmz);
	boaASPDefine("asp_ddns", asp_ddns);
	boaASPDefine("asp_upnp", asp_upnp);
	boaASPDefine("asp_dos", asp_dos);
	boaASPDefine("asp_dosProc", asp_dosProc);
	boaASPDefine("asp_alg", asp_alg);
	boaASPDefine("asp_bt", asp_bt);
	boaASPDefine("asp_user", asp_user);
	boaASPDefine("asp_mailAlert", asp_mailAlert);
	boaASPDefine("asp_remoteLog", asp_remoteLog);
	boaASPDefine("asp_remoteMgmt", asp_remoteMgmt);
	boaASPDefine("asp_urlFilter", asp_urlFilter);
	boaASPDefine("asp_pppoe", asp_pppoe);
	boaASPDefine("asp_ping", asp_ping);
	boaASPDefine("asp_routing", asp_routing);
	boaASPDefine("asp_port", asp_port);
	boaASPDefine("asp_TZ", asp_TZ);
	boaASPDefine("asp_TZ2", asp_TZ2);
	boaASPDefine("asp_eventLog", asp_eventLog);
	boaASPDefine("asp_serverPort", asp_serverPort);
	boaASPDefine("asp_specialApplication", asp_specialApplication);
	boaASPDefine("asp_gaming", asp_gaming);
	boaASPDefine("asp_statusExtended", asp_statusExtended);
	boaASPDefine("asp_configChanged", asp_configChanged);
	boaASPDefine("asp_flashGetCfgParam", asp_flashGetCfgParam);
	boaASPDefine("asp_dhcpServerLeaseList", asp_dhcpServerLeaseList);
	boaASPDefine("asp_flashGetPppoeParam", asp_flashGetPppoeParam);
	boaASPDefine("asp_getWanAddress", asp_getWanAddress);
	boaASPDefine("asp_flashGetCloneMac", asp_flashGetCloneMac);
	boaASPDefine("asp_flashGetString", asp_flashGetString);
	boaASPDefine("asp_flashGetIpElement", asp_flashGetIpElement);
	boaASPDefine("asp_wlanBasic", asp_wlanBasic);
	boaASPDefine("asp_wlanAdvance", asp_wlanAdvance);
	boaASPDefine("asp_wlanSecurity", asp_wlanSecurity);
	boaASPDefine("asp_wlanAccessCtrl", asp_wlanAccessCtrl);
	boaASPDefine("asp_wlanClientList", asp_wlanClientList);
	boaASPDefine("asp_wlanNeighborApList", asp_wlanNeighborApList);
	boaASPDefine("asp_updateFW", asp_updateFW);
	boaASPDefine("asp_reboot", asp_reboot);
	boaASPDefine("asp_udpBlocking", asp_udpBlocking);
#if defined(CONFIG_RTL865X_PPTPL2TP)||defined(CONFIG_RTL865XB_PPTPL2TP)
	boaASPDefine("asp_pptpWizard", asp_pptpWizard);
	boaASPDefine("asp_l2tpWizard", asp_l2tpWizard);
	boaASPDefine("asp_dhcpL2tpWizard", asp_dhcpL2tpWizard);
#endif
	boaASPDefine("asp_qos", asp_qos);
	boaASPDefine("asp_pseudovlan", asp_pseudovlan);
	boaASPDefine("asp_getLanPortStatus", asp_getLanPortStatus);
	boaASPDefine("asp_getWanPortStatus", asp_getWanPortStatus);
	boaASPDefine("asp_pbnat", asp_pbnat);
	boaASPDefine("asp_flash", asp_flash);
	boaASPDefine("asp_ratio_qos", asp_ratio_qos);
	boaASPDefine("asp_webcam", asp_webcam);
	boaASPDefine("asp_naptOpt", asp_naptOpt);
	boaASPDefine("asp_modeConfig", asp_modeConfig);
	boaASPDefine("asp_RipConfig", asp_RipConfig);
//	boaASPDefine("asp_aclPage", asp_aclPage);
	boaASPDefine("asp_serverpPage", asp_serverpPage);
	boaASPDefine("asp_urlfilterPage", asp_urlfilterPage);
	boaASPDefine("asp_qosPage", asp_qosPage);
	boaASPDefine("asp_ratelimitPage", asp_ratelimitPage);
	boaASPDefine("asp_ratio_qosPage", asp_ratio_qosPage);
	boaASPDefine("asp_routingPage", asp_routingPage);
	boaASPDefine("asp_ripPage", asp_ripPage);
	boaASPDefine("asp_ddnsPage", asp_ddnsPage);
	boaASPDefine("asp_specialapPage", asp_specialapPage);
	boaASPDefine("asp_gamingPage", asp_gamingPage);
	boaASPDefine("asp_algPage", asp_algPage);
	boaASPDefine("asp_dmzPage", asp_dmzPage);
	boaASPDefine("asp_dosPage", asp_dosPage);
	boaASPDefine("asp_udpblockingPage", asp_udpblockingPage);
	boaASPDefine("asp_pbnatPage", asp_pbnatPage);
	boaASPDefine("asp_pingPage", asp_pingPage);
	boaASPDefine("asp_naptoptPage", asp_naptoptPage);
	boaASPDefine("asp_pseudovlanPage", asp_pseudovlanPage);
	boaASPDefine("asp_passthruPage", asp_passthruPage);
	boaASPDefine("asp_mcastPage", asp_mcastPage);
	boaASPDefine("asp_bittorrentPage", asp_bittorrentPage);
	boaASPDefine("asp_passthru", asp_passthru);
	boaASPDefine("asp_mcast", asp_mcast);
	boaASPDefine("asp_IpsecExist", asp_IpsecExist);
#ifdef CONFIG_KLIPS
	boaASPDefine("asp_flashGetIpsec", asp_flashGetIpsec);
	boaASPDefine("asp_GetIpsecStatus", asp_GetIpsecStatus);
	boaFormDefine("asp_setIpsecBasic", asp_setIpsecBasic);
	boaFormDefine("asp_IpsecConnect", asp_IpsecConnect);
#endif

	query_temp_var=(char *)malloc(MAX_QUERY_TEMP_VAL_SIZE);
	if(query_temp_var==NULL) exit(0);

	return;

main_end:
	shmdt(pRomeCfgParam);
	exit(0);
#endif // UNUSED
}

int boaRefreshParent(request *req,char *url)
{
	req->buffer_end=0;
	send_r_request_ok(req);
	boaWrite(req, "<script>parent.location.href='%s';</script>", url);
	return SUCCESS;
}

int boaRedirect(request *req,char *url)
{
	req->buffer_end=0;
	send_redirect_perm(req,url);
	return SUCCESS;
}

int boaRedirectTemp(request *req,char *url)
{
	req->buffer_end=0;
	send_redirect_temp(req,url);
	return SUCCESS;
}


int allocNewBuffer(request *req)
{
	char *newBuffer;
	newBuffer=(char *)malloc(req->max_buffer_size*2+1);
	if(newBuffer==NULL) return FAILED;
	memcpy(newBuffer,req->buffer,req->max_buffer_size);
	req->max_buffer_size<<=1;
	free(req->buffer);
	req->buffer=newBuffer;
	return SUCCESS;
}

int boaWriteBlock(request *req, char *buf, int nChars)
{
	int bob=nChars;
	int pos = 0;

#ifndef SUPPORT_ASP
	if((bob+req->buffer_end)>BUFFER_SIZE) bob=BUFFER_SIZE- req->buffer_end;
#else
	while((bob+req->buffer_end)>req->max_buffer_size)
	{
		int ret;
		ret=allocNewBuffer(req);
		if(ret==FAILED) {bob=BUFFER_SIZE- req->buffer_end; break;}
	}
#endif

	if(bob>0)
	{
		memcpy(req->buffer + req->buffer_end,	buf+pos, bob);
		req->buffer_end+=bob;
		return 0;
	}
	return -1;
}

#ifdef CONFIG_USER_BOA_CSRF
unsigned char g_csrf_token[33] = {0};
#endif

/*
 * Enter Key is parsed as 0x0D0x0A in some brower, while in any other browser it maybe parsed as 0x0A
 * getPostTableForValidityCheck is aimed to translate %0D%0A to %0A
 */
static int getPostTableForValidityCheck(char *buf, char *dst)
{
	char *chkptr;
	int i, postTblLen=0;

	/* chkptr point to the new post table list */
	chkptr = buf;
	while (*chkptr)
	{
		if ((*chkptr == '%') && (strlen(chkptr)>=6) && (chkptr[1] == '0') && (chkptr[2] == 'D') &&
			(chkptr[3] == '%') && (chkptr[4] == '0') && (chkptr[5] == 'A'))
		{
			dst[postTblLen++] = '%';
			dst[postTblLen++] = '0';
			dst[postTblLen++] = 'A';
			chkptr += 6;
		}
		else if(*chkptr == '@')
		{
			/* for IE, the character '@' is not translated to %40 in post parameters, but in postTableEncrypt()
			 * the character '@' will be translated to %40 which makes two checksum values different.
			 * we must translate character '@' to %40 */
			dst[postTblLen++] = '%';
			dst[postTblLen++] = '4';
			dst[postTblLen++] = '0';
			chkptr ++;
		}
		else if((chkptr[0] == '%') && (chkptr[1] == '3') && (chkptr[2] == 'F'))
		{
			dst[postTblLen++] = '?';
			chkptr += 3;
		}
		else
		{
			dst[postTblLen++] = *chkptr;
			chkptr++;
		}
	}

	return postTblLen;
}

#define SCRIPT_ALIAS "/boaform/"
#define SCRIPT_USERALIAS "/boaform/admin/"
void handleForm(request *req)
{
	char *ptr;
	char *myalias;

	if(ptr=strstr(req->request_uri,SCRIPT_USERALIAS))
		myalias = SCRIPT_USERALIAS;
	else if(ptr=strstr(req->request_uri,SCRIPT_ALIAS))
		myalias = SCRIPT_ALIAS;
#ifdef SUPPORT_ZERO_CONFIG
	else if(ptr=strstr(req->request_uri,"/itms"))
			myalias = "/";
#endif

//	printf("into handleForm %s........\n",req->request_uri);

	if(ptr==NULL)
	{
		send_r_not_found(req);
		return;
	}
	else
	{
		form_name_t *now_form;

		ptr+=strlen(myalias);
		//ptr+=strlen(SCRIPT_ALIAS);
		now_form=root_form;

		if(attack_req_check(req)) {
#ifdef CU_APPSCAN_RSP_DBG
			send_appscan_response(req);
#else
			send_r_bad_request(req);
#endif
			return;
		}

		if (req->method == M_POST)
		{
			char *chkptr;

			chkptr = boaGetVar(req, "postSecurityFlag", "");
			if(chkptr)
			{
				char *buf, *ptr, *dst;
				unsigned int bufSize, searchSize;
				unsigned int postSecurityFlagValue = 0;
				unsigned int csum = 0;
				int i, postTblLen=0;

				postSecurityFlagValue = strtoul(chkptr, NULL, 10);

				lseek(req->post_data_fd, 0, SEEK_END);
				bufSize = lseek(req->post_data_fd, 0, SEEK_CUR);
				lseek(req->post_data_fd, 0, SEEK_SET);

				if (bufSize > 0 && (buf = mmap(NULL, bufSize, PROT_READ, MAP_SHARED, req->post_data_fd, 0)) != MAP_FAILED)
				{
					chkptr = memstr(buf, "postSecurityFlag", bufSize);
					if(chkptr)
					{
						//FIXME: Dont do checksum check when the post variable after the variable postSecurityFlag. Why???
						dst = buf;
						postTblLen = chkptr-buf;

						i = 0;
						while (i<postTblLen)
						{
							if ((i+4) > postTblLen)
							{
								if (i < postTblLen)
									csum += (dst[i]<<24);
								if ((i+1) < postTblLen)
									csum += (dst[i+1]<<16);
								if ((i+2) < postTblLen)
									csum += (dst[i+2]<<8);

								break;
							}
							else
							{
								csum += (dst[i]<<24) + (dst[i+1]<<16) + (dst[i+2]<<8) + dst[i+3];
								i += 4;
							}
						}
						csum = (csum & 0xffff) + (csum >> 16);
						csum = csum&0xffff;
						csum = (~csum)&0xffff;

						if (postSecurityFlagValue != csum)
						{
							printf("[boa] Error checksum check !! postSecurityFlag %d csum %d\n", postSecurityFlagValue, csum);
							send_r_bad_request(req);
							munmap(buf, bufSize);
							return;
						}
					}
					munmap(buf, bufSize);
				}
			}
		}

//csrf token
#ifdef CONFIG_USER_BOA_CSRF
		char *skipPage[]={
			"formPortFilter",
			"formSysLog",
			"formModify",
			"formFactory",
			"formVendorVersion",
			"formMBSSID",
			"formPortMirror",
			"formUpload",
			"formSaveConfig",
			"userLogin",
			"formUserReg",
			"formBundleInstall",
#ifdef CONFIG_CMCC_ENTERPRISE
			"formTR069ConfigStun",
#endif
#ifdef SUPPORT_ZERO_CONFIG
			"itms",
#endif
			NULL
		};
		//printf("%s:%d req->request_uri %s\n", __FUNCTION__, __LINE__, req->request_uri);

		int skipFlag=0, i;
		for(i=0;skipPage[i];i++)
		{
			if(strstr(req->request_uri, skipPage[i])){
				skipFlag=1;
				break;
			}
		}

		//skip check csrf and reset csrf when logout
		if(strstr(req->request_uri, "formLogout")){
			memset(g_csrf_token, 0, sizeof(g_csrf_token));
			skipFlag=1;
		}

		if(req->method==M_POST && skipFlag==0){
			char	*str;
			str = boaGetVar(req, CSRF_TOKEN_STRING, "");

#ifdef CONFIG_USER_BOA_CSRF_TOKEN_INDEPDENT
			getCSRFTokenfromTokenInfo(g_csrf_token,req);
#endif

			//printf("%s:%d %s %s\n", __FUNCTION__, __LINE__, CSRF_TOKEN_STRING, str);
			if (!str[0] && g_csrf_token[0] !=0 ) {//page does not contain csrf token, but the web page refeshed before
				//when csrf check fail, we do not redirect to forbidden, just retuen refresh page
				str = boaGetVar(req, "submit-url", "");
				boaRedirect(req, str);
				return;
			}
			else if(g_csrf_token[0]==0){//bootup
				//printf("%s:%d csrftokne bootup\n", __FUNCTION__, __LINE__);
				//pass csrf check when bootup and no web page refresh
			}
			else{
				if(strcmp(str, g_csrf_token)!=0){//the csrf token is not matched
					//printf("[%s:%d]\n",__func__,__LINE__);
					//printf("%s=%s, g_csrf_token=%s\n",CSRF_TOKEN_STRING, str,g_csrf_token);
					//when csrf check fail, we do not redirect to forbidden, just retuen refresh page
					str = boaGetVar(req, "submit-url", "");
					boaRedirect(req, str);
					return;
				}
			}
		}
#endif

		while(1)
		{
			if (	(strlen(ptr) == strlen(now_form->pagename)) &&
				(memcmp(ptr,now_form->pagename,strlen(now_form->pagename))==0))
			{
 #ifdef VOIP_SUPPORT
  /* for VoIP config load page */
				if(0 == strncmp(now_form->pagename,"voip_config_load", strlen("voip_config_load"))){
					now_form->function(req,NULL,NULL);
					send_r_request_ok(req);
				}else{
#ifdef SUPPORT_ZERO_CONFIG
					if(strcmp(ptr,"itms"))
#endif
					send_r_request_ok(req);
					now_form->function(req,NULL,NULL);
				}
#else
#ifdef MULTI_USER_PRIV
				//printf("handleForm: req->user=%s form->pagename=%s form->privilege=%d\n", req->user, now_form->pagename, now_form->privilege);
				//boaRedirect(req, now_form->pagename);
				if ((int)now_form->privilege > getAccPriv(req->user)) {
					boaHeader(req);
					boaWrite(req, "<body><blockquote><h2><font color=\"#FF0000\">Access Denied !!</font></h2><p>\n");
					boaWrite(req, "<h4>Sorry, you don't have enough privilege to take this action.</h4>\n");
					boaWrite(req, "<form><input type=\"button\" onclick=\"history.go (-1)\" value=\"&nbsp;&nbsp;OK&nbsp;&nbsp\" name=\"OK\"></form></blockquote></body>");
					boaFooter(req);
					boaDone(req, 200);
				}
				else {
#ifdef SUPPORT_ZERO_CONFIG
					if(strcmp(ptr,"itms"))
#endif
					send_r_request_ok(req);		/* All's well */
					now_form->function(req,NULL,NULL);
				}
#else
#ifdef SUPPORT_ZERO_CONFIG
				if(strcmp(ptr,"itms"))
#endif
				send_r_request_ok(req);		/* All's well */
				now_form->function(req,NULL,NULL);
#endif
#endif
				freeAllTempStr();
				freeAllASPTempStr();

				//boaWrite(req,"okokok\n");
				return;
			}

			if(now_form->next==NULL) break;
			now_form=now_form->next;
		}

		send_r_not_found(req);
		return;
	}
}

void handleScript(request * req, char *left, char *right)
{
	asp_name_t *now_asp;
	uint32_t funcNameLength;
	int i;

	/*
	 * left = strstr(last_right, "<%");
	 * right = strstr(left, "%>");
	 */
	left += 2;
	right -= 1;
	while (left < right) {
		left += strspn(left, " ;(),");
		if (left >= right)
			break;

		/* count the length of the function name */
		funcNameLength = strcspn(left, "( ");

		for (now_asp = root_asp; now_asp; now_asp = now_asp->next) {
			if (strlen(now_asp->pagename) == funcNameLength &&
			    strncmp(left, now_asp->pagename, funcNameLength) == 0) {
				char *start, *end, *semicolon;
				int argc = 0;
				char *argv[8] =
				    { NULL, NULL, NULL, NULL, NULL, NULL, NULL,
					NULL
				};
				int len = 0;

				/*
				 *       start   end right
				 *         |      |    |
				 *         v      v    v
				 * <% func(" args ");   %>
				 *    ^             ^
				 *    |             |
				 *   left        semicolon
				 */
				left += funcNameLength;
				semicolon = memchr(left, ';', right-left+1);
				if (semicolon == NULL) {
					printf("%s(%d): Script function(\033[0;31m%s\033[m) not ended with semicolon in \033[0;31m%s\033[m.\n", __func__, __LINE__, now_asp->pagename, req->pathname);
					printf("Syntax: <%% funcName(\"...\"); %%>\n\n");
				}
				for (; left < right && left < semicolon; argc++) {
					len = right - left;
					start = memchr(left, '"', len);
					if (start == NULL || start >= semicolon || start >= right)
						break;

					len = right - (start + 1);
					end = memchr(start + 1, '"', len);
					if (end == NULL || end >= semicolon || end >= right)
						break;

					left = end + 1;

					argv[argc] = strndup((const char *)(start + 1), end - start - 1);
					if (argv[argc] == NULL)
						break;
				}

				now_asp->function(0, req, argc, argv);	// ANDREW

				for (i = 0; i < argc; i++)
					free(argv[i]);
				break;
			}
		}
		left += strcspn(left, " ") + 1;
	}
}

#ifdef CONFIG_USER_BOA_CSRF
void get_csrf_hash(char *referer, char* csrf)
{
	unsigned char digest[16] = {0};
	char tmp[10]={0};
	int i = 0;
	struct MD5Context context;
	MD5Init(&context);
	MD5Update(&context, referer, strlen(referer));
	MD5Final(digest, &context);
	memset(csrf, 0, 32);
	for(i=0; i<16; i++){
		sprintf(tmp, "%02x", digest[i]);
		strcat(csrf, tmp);
	}
	//printf("csrf md5 string %s\n", csrf);
}
/*
 *	Input:
 *		token: csrf token buffer, and size should greater than 32 byte
 *	Return:
 *		0 for special asp page, keep orignal CSRF token
 *		1 for Create new CSRF token
 *		-1 for error
 */
int genCSRFToken(request * req, char *token){
	int ret, skipFlag=0, i;
	char csrf_buf[128]={0};

	/* those page should NOT generate new csrf token */
	char *skipPage[]={
		"share.js", "wlstatbl.asp",	"wlstatbl_vap.asp",
		"dhcptbl.asp", "portBaseFilterDhcp.asp",
		"macIptbl.asp", "fdbtbl.asp", "dhcptblv6.asp",
		"routetbl_ipv6.asp",	//IPv6 Static RoutingConfiguration
		"voip_sip_status.asp",
		NULL
	};

	for(i=0;skipPage[i];i++)
	{
		if(strstr(req->request_uri, skipPage[i])){
			skipFlag=1;
			break;
		}
	}

	// generate csrf token, exclude share.js
	// voip port1/port2 has multiple asp request, it ignore sub-request.
	if(skipFlag == 0){
		//generate csrf token string
		time_counter = getSYSInfoTimer();
		snprintf(csrf_buf, sizeof(csrf_buf), "%d+%s", time_counter, req->referer);
		get_csrf_hash(csrf_buf, (char *)token);
		return 1;
	}else{
		return 0;
	}
}

/*
 * Name: handleCSRFToken
 *
 * Description: go through buff and find keywork "</form>", then insert CSRF token.
 * Returns 0 for error, or 1 for success
 */
int handleCSRFToken(request * req)
{
	char real_path_name[256]="/tmp/csrf_tmp_file", cmd[512]={0};
	FILE *fp;
	int csrf_data_fd;
	char *csrf_data_mem=NULL;
	unsigned long csrf_filesize=0;		/* filesize */
	struct stat csrf_statbuf;
	int hasFrom=0;
	unsigned char tmp_csrf_token[33] = {0};

	if(req->buffer_start == 0){
		/*	case 1: csrf_end == 0, it is first time flush, then go through buffer to append csrf token
		 *	case 2: csrf_end != 0
		 *		if csrf_end == buffer_end, we assume socket write return -1 in last req_flush()
		 *		others, old buffer should be flush to client in req_flush_retry() and new buffer income,
		 *				so reset csrf_end to buffer_start to go through buff again.
		 */
		if(req->csrf_end !=0){
			if(req->csrf_end == req->buffer_end){
				return 1;
			}else{
				req->csrf_end = req->buffer_start;
			}
		}
	}else{
		if(req->csrf_end == req->buffer_end){
			/* socket write uncomplete, ignore it */
			return 1;
		}
	}

	//create another file for adding csrf token
	if(!(fp = fopen(real_path_name, "wb")))
	{
		boa_perror(req, "file open fail");
		return 0;
	}

	// force to fwrite '\n' at the end of string, otherwise missing '\0' it maybe stuck at strstr function
	fwrite(&req->buffer[req->csrf_end], 1, req->buffer_end - req->csrf_end, fp);
	fseek( fp, 0, SEEK_END);
	fprintf(fp,"\n");
	fclose(fp);

	//mmap to virtual memory
	csrf_data_fd = open(real_path_name, O_RDONLY);
	if (csrf_data_fd == -1) {
		boa_perror(req, "file open fail");
		return 0;
	}

	fstat(csrf_data_fd, &csrf_statbuf);
	csrf_filesize = csrf_statbuf.st_size;

	/* MAP_OPTIONS: see compat.h */
	csrf_data_mem = mmap(0, csrf_filesize,
#ifdef USE_NLS
			PROT_READ|PROT_WRITE
#else
			PROT_READ
#endif
			, MAP_OPTIONS, csrf_data_fd, 0);
	close(csrf_data_fd);				/* close data file */

	if ((long) csrf_data_mem == -1) {
		boa_perror(req, "mmap");
		return 0;
	}

	if(req->data_mem){
		munmap(req->data_mem, req->filesize);
	}
	req->data_mem = csrf_data_mem;
	req->filesize = csrf_filesize;
	req->buffer_end = req->csrf_end;	//roll back to csrf end

	req->filepos = 0;	//reset file position

	{
		//parse and send asp page
		char *left,*last_right;
		int bob, csrf_buf_len;
		char csrf_buf[128]={0};

#ifdef CONFIG_USER_BOA_CSRF_TOKEN_INDEPDENT
		getCSRFTokenfromTokenInfo(g_csrf_token,req);
#endif

		if(genCSRFToken(req, tmp_csrf_token) == 0 ){
			// special page, it use original csrf token
			snprintf(tmp_csrf_token, sizeof(tmp_csrf_token), "%s", g_csrf_token);
			//printf("[%s] special page, use original csrf token, uri=%s\n", __FUNCTION__, req->request_uri);
		}

		//printf("[%s] uri=%s, tmp_csrf_token=%s, g_csrf_token=%s\n", __FUNCTION__, req->request_uri, tmp_csrf_token, g_csrf_token);
#ifdef CONFIG_TELMEX_DEV
		csrf_buf_len = snprintf(csrf_buf, sizeof(csrf_buf), "<input type=\"hidden\" name=\"%s\" value=\"%s\" />", CSRF_TOKEN_STRING, tmp_csrf_token);
#else
		csrf_buf_len = snprintf(csrf_buf, sizeof(csrf_buf), "<input type='hidden' name='%s' value='%s' />\r\n", CSRF_TOKEN_STRING, tmp_csrf_token);
#endif
		csrf_buf[csrf_buf_len] = '\0';

#define TOCKEN_FORM "</FORM>"
#define TOCKEN_SIZE 7
		last_right=req->data_mem;
		while(last_right && (last_right<(req->data_mem+csrf_filesize)) && *last_right != '\0')
		{
			left = last_right;
			if(((last_right+TOCKEN_SIZE) < (req->data_mem+csrf_filesize)) &&
				strncasecmp(left, TOCKEN_FORM, TOCKEN_SIZE)==0)
			{
				/* original content + CSRF content + </FORM> */
				if(req->buffer_end+csrf_buf_len+TOCKEN_SIZE > req->max_buffer_size)
				{
					if(allocNewBuffer(req) == FAILED){
						boa_perror(req, "Error alloc buffer");
						break;
					}
				}
				// copy CSRF content
				memcpy(req->buffer + req->buffer_end, csrf_buf, csrf_buf_len);
				req->buffer_end += csrf_buf_len;

				// copy </FORM>
				memcpy(req->buffer + req->buffer_end, last_right, TOCKEN_SIZE);
				req->buffer_end += TOCKEN_SIZE;
				req->filepos += TOCKEN_SIZE;
				last_right += TOCKEN_SIZE;
				hasFrom++;
			}
			else
			{
				if(req->buffer_end+1 > req->max_buffer_size)
				{
					if(allocNewBuffer(req) == FAILED){
						boa_perror(req, "Error alloc buffer");
						break;
					}
				}
				*(req->buffer + req->buffer_end) = *last_right;
				req->buffer_end++;
				req->filepos ++;
				last_right++;
			}
		}
	}

	//printf("[%s] hasform=%d, uri=%s, tmp_csrf_token=%s, g_csrf_token=%s\n", __FUNCTION__, hasFrom, req->request_uri, tmp_csrf_token, g_csrf_token);
	if(hasFrom){
		snprintf(g_csrf_token, sizeof(g_csrf_token), "%s", tmp_csrf_token);
#ifdef CONFIG_USER_BOA_CSRF_TOKEN_INDEPDENT
		setCSRFTokentoTokenInfo(g_csrf_token,req);
#endif
		//printf("[%s] update g_csrf_token to %s\n", __FUNCTION__, g_csrf_token);
	}

	unlink(real_path_name);
	req->csrf_end = req->buffer_end;
	return 1;
}
#endif
#endif


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "sockmark_define.h"
#include "utility.h"
#include <arpa/inet.h>

#if defined(CONFIG_USER_P0F)
const char TABLE_LANNETINFO_P0F_FWD[] = "lannetinfo_p0f_fwd";
#endif

#if defined(CONFIG_USER_P0F)
void rtk_lannetinfo_initial_iptables(void)
{
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", TABLE_LANNETINFO_P0F_FWD);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", TABLE_LANNETINFO_P0F_FWD);
#endif

	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-i", "br0", "-p", "tcp", "--dport", "80", "-j", TABLE_LANNETINFO_P0F_FWD);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_FORWARD, "-i", "br0", "-p", "tcp", "--dport", "80", "-j", TABLE_LANNETINFO_P0F_FWD);
#endif

	va_cmd(IPTABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-i", "br0", "-p", "tcp", "--dport", "80", "-j", TABLE_LANNETINFO_P0F_FWD);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 12, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-i", "br0", "-p", "tcp", "--dport", "80", "-j", TABLE_LANNETINFO_P0F_FWD);
#endif
}

//0: del
//1: add
void rtk_lannetinfo_set_p0f_iptables(unsigned char* mac, int type)
{
#ifdef CONFIG_RTK_SKB_MARK2
		unsigned char mark0[64], mark[64];
		char mac_str[20];
		unsigned char p0f_enable=0;

		mib_get_s(MIB_LANNETINFO_P0F_ENABLE, (void*)&p0f_enable, sizeof(p0f_enable));
		if(p0f_enable == 0) return;
		
		sprintf(mark0, "0x0/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);
		sprintf(mark, "0x%llx/0x%llx", SOCK_MARK2_FORWARD_BY_PS_BIT_MASK, SOCK_MARK2_FORWARD_BY_PS_BIT_MASK);

		sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", 
			mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

		va_cmd_no_error(IPTABLES, 22, 1, "-t", "mangle", (type==1)?"-A":"-D", 
			(char *)TABLE_LANNETINFO_P0F_FWD, "-p", "tcp", "-m", "mac", 
			"--mac-source", mac_str, "-m", "connbytes", "--connbytes", "0:4", 
			"--connbytes-dir", "both", "--connbytes-mode", "packets", "-j", "MARK2", "--set-mark2", mark);
#ifdef CONFIG_IPV6
		va_cmd_no_error(IP6TABLES, 22, 1, "-t", "mangle", (type==1)?"-A":"-D", 
			(char *)TABLE_LANNETINFO_P0F_FWD, "-p", "tcp", "-m", "mac", 
			"--mac-source", mac_str, "-m", "connbytes", "--connbytes", "0:4", 
			"--connbytes-dir", "both", "--connbytes-mode", "packets", "-j", "MARK2", "--set-mark2", mark);
#endif
#endif
}

void rtk_lannetinfo_get_brand(unsigned char *mac, char* brand, int size)
{
	char cmd[256];
	char macStr[20];

	char* ptr=NULL;

	if(mac == NULL || brand ==NULL) return;
	
	snprintf(macStr, sizeof(macStr), "%02X%02X%02X", mac[0], mac[1], mac[2]);
	snprintf(cmd, sizeof(cmd),"cat /etc/oui.txt | grep %s | awk -F\")\" '{print $2}' | awk '$1=$1' > /tmp/lannetinfo_brand", macStr);
	system(cmd);

	usleep(10000);
	
	FILE *pp = fopen("/tmp/lannetinfo_brand", "r");
	if(pp){
		ptr = fgets(brand, size, pp);
		if(ptr != NULL) {
			brand[strlen(brand)-1] = '\0';
		}
		if(ptr == NULL){
			snprintf(brand, size, "OTHER");
		}
		fclose(pp);
	}
}

void rtk_lannetinfo_get_os(unsigned int ip, char* os, int size)
{
	char cmd[256];
	char ipStr[INET6_ADDRSTRLEN];
	struct in_addr in_ip;
	char* ptr=NULL;
	char tmp[1024];

	if(ip == 0 || os ==NULL) return;

	in_ip.s_addr = ip;
	inet_ntop(AF_INET, &in_ip, ipStr, INET_ADDRSTRLEN);
	snprintf(cmd, sizeof(cmd),"/bin/p0f-client /var/run/p0f.sock %s | grep 'Detected OS' | awk -F\"=\" '{print $2}' | awk '$1=$1' > /tmp/lannetinfo_os", ipStr);
	system(cmd);

	usleep(10000);
	
	FILE *pp = fopen("/tmp/lannetinfo_os", "r");
	if(pp){
		ptr = fgets(os, size, pp);
		if(ptr != NULL) {
			os[strlen(os)-1] = '\0';
		}
		if(ptr == NULL || strcmp(os, "???") == 0 || strlen(os) == 0){
			snprintf(os, size, "OTHER");
		}
		fclose(pp);
	}

	snprintf(cmd, sizeof(cmd),"/bin/p0f-client /var/run/p0f.sock %s | grep 'HTTP software' | awk -F\"=\" '{print $2}' | awk '$1=$1' > /tmp/lannetinfo_os", ipStr);
	system(cmd);

	usleep(10000);

	pp=NULL;
	pp = fopen("/tmp/lannetinfo_os", "r");
	if(pp){
		ptr = fgets(tmp, 1024, pp);
		if(ptr != NULL) {
			tmp[strlen(tmp)-1] = '\0';
		}
		if(ptr != NULL && strcmp(tmp, "???") != 0){
			snprintf(os, size, "%s", tmp);
		}
		fclose(pp);
	}
}

void rtk_lannetinfo_get_model(unsigned int ip, char* model, int size)
{
	char cmd[256];
	char ipStr[INET6_ADDRSTRLEN];
	struct in_addr in_ip;
	char* ptr=NULL;

	if(ip == 0 || model ==NULL) return;

	in_ip.s_addr = ip;
	inet_ntop(AF_INET, &in_ip, ipStr, INET_ADDRSTRLEN);
	snprintf(cmd, sizeof(cmd),"/bin/p0f-client /var/run/p0f.sock %s | grep 'HTTP User-Agent' | awk -F\"(\" '{print $2}' | awk -F\")\" '{print $1}' > /tmp/lannetinfo_model", ipStr);
	system(cmd);

	usleep(10000);
	
	FILE *pp = fopen("/tmp/lannetinfo_model", "r");
	if(pp){
		ptr = fgets(model, size, pp);
		if(ptr != NULL) {
			model[strlen(model)-1] = '\0';
		}
		if(ptr == NULL || strcmp(model, "???") == 0 || strlen(model) == 0){
			snprintf(model, size, "OTHER");
		}
		fclose(pp);
	}
}


/*Guess devType from brand, os, model*/
void rtk_lannetinfo_get_devType(char* brand, char* os, char* model, unsigned char* devType)
{
	char cmd[256];
	char ipStr[INET6_ADDRSTRLEN];
	struct in_addr in_ip;
	char* ptr=NULL;

	if(brand == NULL || os ==NULL || model ==NULL || devType ==NULL) return;

	*devType = LANHOSTINFO_TYPE_OTHER;
	
	if(strstr(os, "Windows")){
		*devType = LANHOSTINFO_TYPE_PC;
		return;
	}

	if(strstr(model, "Pad")){
		*devType = LANHOSTINFO_TYPE_PAD;
		return;
	}

	if(strstr(model, "Ap")){
		*devType = LANHOSTINFO_TYPE_AP;
		return;
	}

	if(strstr(model, "Box")){
		*devType = LANHOSTINFO_TYPE_STB;
		return;
	}

	if(strstr(os, "Phone") || strstr(model, "Phone") || 
		strstr(model, "HTC") || strstr(model, "Vivo International") ||
		strstr(model, "Huawei") || strstr(model, "Xiaomi")){
		*devType = LANHOSTINFO_TYPE_PHONE;
		return;
	}
}

/*
	Ancelotti_Chen: 2022-02-14

	MAC OS:
	----------------------------------------------------------------------------------------
		MAC book:
		Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10_6_0; en-US) 
			AppleWebKit/528.10 (KHTML, like Gecko) Chrome/2.0.157.2 Safari/528.10

		iPhone:
		Mozilla/5.0 (iPhone; CPU iPhone OS 14_3 like Mac OS X) 
			AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148

		iPod Touch:
		Mozilla/5.0 (iPod; U; CPU iPhone OS 4_3_3 like Mac OS X; en-us) 
			AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8J2 Safari/6533.18.5

		iPad:
		Mozilla/5.0 (iPad; U; CPU OS 4_3_3 like Mac OS X; en-us) 
			AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8J2 Safari/6533.18.5

	Windows:
	----------------------------------------------------------------------------------------
		Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:97.0) Gecko/20100101 Firefox/97.0
		Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0)
		Mozilla/5.0 (Windows; U; Windows NT 6.1; fr; rv:1.9.1.9) Gecko/20100315 Firefox/3.5.9
		Mozilla/5.0 (Windows NT 6.1; WOW64; rv:70.0) Gecko/20190101 Firefox/70.0
		Mozilla/5.0 (Windows NT 10.0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/40.0.2214.93 Safari/537.36

	Linux:
	----------------------------------------------------------------------------------------
		Mozilla/5.0 (Linux; Android 12; SM-G9810 Build/SP1A.210812.016; wv)
		Mozilla/5.0 (Linux; U; Android 6.0.1; zh-CN; Redmi 3X Build/MMB29M)
		Dalvik/2.1.0 (Linux; U; Android 12; SM-G9810 Build/SP1A.210812.016)
		Mozilla/5.0 (Linux; U; Android 2.3.3; zh-tw; HTC Pyramid Build/GRI40) 
		Mozilla/5.0 (X11; Ubuntu; Linux x86_64)
		Mozilla/5.0 (X11; Linux x86_64)
		Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.2.15)
		Opera/9.01 (X11; OpenBSD i386; U; en)
		Opera/9.01 (X11; Linux i686; U; en)
		Opera/9.01 (X11; FreeBSD 6 i386; U;pl)

	Guess OS Version:
		1. if OS = Windows, match (Windows NT 10.0)

			ver =
			/bin/p0f-client /var/run/p0f.sock 192.168.1.2 | awk -F'[()]' '/User-Agent/{print $2}' 
		 	| awk -F';' '{i=1;while(i<=NF){if($i~/Windows NT/){print 
		 	$i;break;}i++}}' | sed 's/^ //g'


		2. if OS = Mac OS X, match (OS 14_3)

			ver = 
			/bin/p0f-client /var/run/p0f.sock 192.168.1.2 | awk -F'[()]' '/User-Agent/{print $2}' 
		 	| awk -F';' '{i=1;while(i<=NF){if($i~/Mac OS/){print $i;break;}i++}}' | sed 's/^ //g'	

		 	devType =
		 	/bin/p0f-client /var/run/p0f.sock 192.168.1.2 | awk -F'[()]' '/User-Agent/{print $2}' 
		 	| awk -F';' '{print $1}'

		3. if OS = Linux or X11, match (Android 12 or Ubuntu 7.04 or FreeBSD or OpenBSD)

			ver = 
			/bin/p0f-client /var/run/p0f.sock 192.168.1.2 | awk -F'[()]' '/User-Agent/{print $2}' 
		 	| awk -F';' '{i=1;while(i<=NF){if($i~/(Android|Ubuntu|FreeBSD|OpenBSD)/){print 
		 	$i;break;}i++}}' | sed 's/^ //g'

	Guess model & sw version:
		1. if OS = Linux, match (Build)
			model =
			/bin/p0f-client /var/run/p0f.sock 192.168.1.2 | awk -F'[()]' '/User-Agent/{print $2}' 
		 	| awk -F';' '{i=1;while(i<=NF){if($i~/Build/){print $i;break;}i++}}' | sed 's/^ //g'
		 	| awk -F' Build/' '{print $1}'

		 	sw version =
			/bin/p0f-client /var/run/p0f.sock 192.168.1.2 | awk -F'[()]' '/User-Agent/{print $2}' 
		 	| awk -F';' '{i=1;while(i<=NF){if($i~/Build/){print $i;break;}i++}}' | sed 's/^ //g'
		 	| awk -F' Build/' '{print $2}'
	 
*/
void rtk_lannetinfo_device_info_init(unsigned int ip)
{
	char cmd[1024];
	char ipStr[INET6_ADDRSTRLEN];
	struct in_addr in_ip;

	if(ip == 0) return;

	in_ip.s_addr = ip;
	inet_ntop(AF_INET, &in_ip, ipStr, INET_ADDRSTRLEN);
	snprintf(cmd, sizeof(cmd),"/bin/p0f-client /var/run/p0f.sock %s > /tmp/lannetinfo_deviceinfo.%x",ipStr,ip);
	system(cmd);
	usleep(10000);
}

void strip_line_enter(char *str, int len)
{
	int i = 0;
	
	if(str == NULL || len <= 0)
		return;
	
	while(str[i] != '\0' && i < len){
		if(str[i] == '\n'){
			str[i] = '\0';
			break;
		}
		i++;
	}
}

void rtk_lannetinfo_get_device_info(unsigned int ip, lannet_devinfo_t *deviceInfo)
{
	char cmd[1024];
	FILE *fp = NULL;
	char* pStr=NULL;

	if(ip == 0 || deviceInfo == NULL) return;

	memset(cmd,0,1024);
	snprintf(cmd, sizeof(cmd),"cat /tmp/lannetinfo_deviceinfo.%x | awk -F'=' '/Detected OS/{print $2}' | sed 's/^ //g' > /tmp/lannetinfo_os", ip);
	system(cmd);
	usleep(10000);	
	
	snprintf(cmd, sizeof(cmd),"cat /tmp/lannetinfo_deviceinfo.%x | awk -F'[()]' '/User-Agent/{print $2}' > /tmp/lannetinfo_temp",ip);
	system(cmd);
	usleep(10000);

	memset(cmd,0,1024);
	snprintf(cmd, sizeof(cmd),"cat /tmp/lannetinfo_temp | awk -F';' '{i=1;while(i<=NF){if($i~/(Windows NT|Mac OS|Android|Ubuntu|FreeBSD|OpenBSD)/){print $i;break;}i++}}' | sed 's/^ //g' | sed 's/ $//g' > /tmp/lannetinfo_os_ver");
	system(cmd);
	usleep(10000);	
	
	memset(cmd,0,1024);
	snprintf(cmd, sizeof(cmd),"cat /tmp/lannetinfo_temp | awk -F';' '{i=1;while(i<=NF){if($i~/Build/){print $i;break;}i++}}' | sed 's/^ //g' | sed 's/ $//g' | awk -F' Build/' '{print $1}' > /tmp/lannetinfo_model");
	system(cmd);
	usleep(10000);

	memset(cmd,0,1024);
	snprintf(cmd, sizeof(cmd),"cat /tmp/lannetinfo_temp | awk -F';' '{i=1;while(i<=NF){if($i~/Build/){print $i;break;}i++}}' | sed 's/^ //g' | sed 's/ $//g' | awk -F' Build/' '{print $2}' > /tmp/lannetinfo_sw_ver");
	system(cmd);
	usleep(10000);	

	fp = fopen("/tmp/lannetinfo_os", "r");
	if(fp){
		memset(deviceInfo->os,0,sizeof(deviceInfo->os));
		if(!fgets(deviceInfo->os, sizeof(deviceInfo->os), fp))
			strncpy(deviceInfo->os,"OTHER",sizeof(deviceInfo->os));
		else
			strip_line_enter(deviceInfo->os,sizeof(deviceInfo->os));
		fclose(fp);
	}

	fp = fopen("/tmp/lannetinfo_model", "r");
	if(fp){
		memset(deviceInfo->model,0,sizeof(deviceInfo->model));
		if(!fgets(deviceInfo->model, sizeof(deviceInfo->model), fp))
			strncpy(deviceInfo->model,"OTHER",sizeof(deviceInfo->model));	
		else
			strip_line_enter(deviceInfo->model,sizeof(deviceInfo->model));
		fclose(fp);
	}

	if(strstr(deviceInfo->os,"Mac")){
		
		memset(cmd,0,1024);
		snprintf(cmd, sizeof(cmd),"cat /tmp/lannetinfo_temp | awk -F';' '{print $1}' > /tmp/lannetinfo_mac_type");
		system(cmd);
		usleep(10000);	
		
		fp = fopen("/tmp/lannetinfo_mac_type", "r");
		if(fp){
			char type[256] = {0};
			if(fgets(type, sizeof(type), fp))
				strip_line_enter(type,sizeof(type));
			if(!strncmp(deviceInfo->model,"OTHER",sizeof(deviceInfo->model)) && strcmp(type,""))
				strncpy(deviceInfo->model,type,sizeof(deviceInfo->model));
			fclose(fp);
		}
	}

	fp = fopen("/tmp/lannetinfo_os_ver", "r");
	if(fp){
		memset(deviceInfo->osVer,0,sizeof(deviceInfo->osVer));
		if(!fgets(deviceInfo->osVer, sizeof(deviceInfo->osVer), fp))
			strncpy(deviceInfo->osVer,"",sizeof(deviceInfo->osVer));
		else
			strip_line_enter(deviceInfo->osVer,sizeof(deviceInfo->osVer));
		fclose(fp);
	}

	fp = fopen("/tmp/lannetinfo_sw_ver", "r");
	if(fp){
		memset(deviceInfo->swVer,0,sizeof(deviceInfo->swVer));
		if(!fgets(deviceInfo->swVer, sizeof(deviceInfo->swVer), fp))
			strncpy(deviceInfo->swVer,"",sizeof(deviceInfo->swVer));
		else
			strip_line_enter(deviceInfo->swVer,sizeof(deviceInfo->swVer));
		fclose(fp);
	}

	/* Guess device type*/
	if(strstr(deviceInfo->model, "Box"))
		deviceInfo->devType = LANHOSTINFO_TYPE_STB;
	else if(strstr(deviceInfo->os,"Mac")){
		if(strstr(deviceInfo->model,"iPhone"))
			deviceInfo->devType = LANHOSTINFO_TYPE_PHONE;
		else if(strstr(deviceInfo->model,"iPad"))
			deviceInfo->devType = LANHOSTINFO_TYPE_PAD;
		else if(strstr(deviceInfo->model,"Macintosh"))
			deviceInfo->devType = LANHOSTINFO_TYPE_PC;
		else
			deviceInfo->devType = LANHOSTINFO_TYPE_OTHER;
	}
	else if(!strncasecmp(deviceInfo->os,"Windows",7))
		deviceInfo->devType = LANHOSTINFO_TYPE_PC;
	else if(!strncasecmp(deviceInfo->osVer,"Android",7))
		deviceInfo->devType = LANHOSTINFO_TYPE_PHONE;
	else if(!strncasecmp(deviceInfo->osVer,"Ubuntu",6) || strstr(deviceInfo->osVer,"BSD"))
		deviceInfo->devType = LANHOSTINFO_TYPE_PC;
	else
		deviceInfo->devType = LANHOSTINFO_TYPE_OTHER;
	
	//printf("\ntype=%d model=%s,os=%s,os ver=%s,sw ver=%s\n",
		//deviceInfo->devType,deviceInfo->model,deviceInfo->os,deviceInfo->osVer,deviceInfo->swVer);
}

#endif


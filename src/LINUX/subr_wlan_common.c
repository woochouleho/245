#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/file.h>
#include <arpa/inet.h>
#include "debug.h"
#include "utility.h"
#include "subr_net.h"
#include <linux/wireless.h>
#include <dirent.h>
//#include "subr_wlan.h"
#ifdef WLAN_FAST_INIT
#include "../../../linux-2.6.x/drivers/net/wireless/rtl8192cd/ieee802_mib.h"
#endif

#if IS_AX_SUPPORT || defined(WIFI5_WIFI6_COMP)
#include "hapd_conf.h" 
#endif
#if defined(CONFIG_CTC_SDN)
#include "subr_ctcsdn.h"		
#endif


/**
* @brief
*	   Get the encryption info of wlan interface
*
* @param[in]
*	   ifname (unsigned char *) WLAN interface name to operate
*
* @return
*	   pEncInfo (RTK_WLAN_ENCRYPT_INFO *) Pointer to struct of encryption info obtained
*
* @retval:
* - 	0 - RTK_SUCCESS
* -    -1 - RTK_FAILED
*/

int rtk_wlan_get_encryption(unsigned char *ifname, RTK_WLAN_ENCRYPT_INFO *pEncInfo)
{

	int ret = RTK_FAILED;
	int vwlan_idx;
	char def_key_idx = 0;
	MIB_CE_MBSSIB_T Wlan_Entry;
	
	ISP_API_EXIT_WITH_VAL_ON(ifname==NULL||pEncInfo==NULL, RTK_FAILED);
	
	memset(pEncInfo, 0x0, sizeof(*pEncInfo));	
	memset(&Wlan_Entry,0x0,sizeof(MIB_CE_MBSSIB_T));
	
	if((mib_chain_get_wlan(ifname,&Wlan_Entry,&vwlan_idx))!=RTK_SUCCESS){
		ISP_API_DEBUG("mib chain get error\n");
		return RTK_FAILED;
	}
	//encrypt type
	pEncInfo->encrypt_type=Wlan_Entry.encrypt;
	//if encryption is none, exit now!	
	ISP_API_EXIT_WITH_VAL_ON(pEncInfo->encrypt_type==ENCRYPT_DISABLED, RTK_SUCCESS);
	//authentication(1X)
	pEncInfo->rtk_api_enable_1x = Wlan_Entry.enable1X;
	if(pEncInfo->rtk_api_enable_1x==1){
		memcpy(pEncInfo->rtk_api_rs_addr,Wlan_Entry.rsIpAddr,IP_ADDR_LEN);//WU&
		pEncInfo->rtk_api_rs_port=Wlan_Entry.rsPort;
		memcpy(pEncInfo->rtk_api_rs_pass,Wlan_Entry.rsPassword,MAX_RS_PASS_LEN);
	}
	switch(pEncInfo->encrypt_type)
	{
	case ENCRYPT_DISABLED:
		//do nothing else
		break;
	case ENCRYPT_WEP:
		//authentication(non-1X)
		if(pEncInfo->rtk_api_enable_1x==0)
		{
			pEncInfo->rtk_api_auth_type=Wlan_Entry.authType;
		}
		//encryption
		pEncInfo->rtk_api_wep_key_len=Wlan_Entry.wep;
		pEncInfo->rtk_api_wep_key_type=Wlan_Entry.wepKeyType;
		def_key_idx=Wlan_Entry.wepDefaultKey;
		if(pEncInfo->rtk_api_wep_key_len==WEP64){
			if(def_key_idx==0){
				memcpy(pEncInfo->rtk_api_wep_key64,Wlan_Entry.wep64Key1,WEP64_KEY_LEN);
			}
			else if(def_key_idx==1)
				memcpy(pEncInfo->rtk_api_wep_key64,Wlan_Entry.wep64Key2,WEP64_KEY_LEN);
			else if(def_key_idx==2)
				memcpy(pEncInfo->rtk_api_wep_key64,Wlan_Entry.wep64Key3,WEP64_KEY_LEN);
			else
				memcpy(pEncInfo->rtk_api_wep_key64,Wlan_Entry.wep64Key4,WEP64_KEY_LEN);
		}else if(pEncInfo->rtk_api_wep_key_len==WEP128){
			if(def_key_idx==0)
				memcpy(pEncInfo->rtk_api_wep_key128,Wlan_Entry.wep128Key1,WEP128_KEY_LEN);
			else if(def_key_idx==1)
				memcpy(pEncInfo->rtk_api_wep_key128,Wlan_Entry.wep128Key2,WEP128_KEY_LEN);
			else if(def_key_idx==2)
				memcpy(pEncInfo->rtk_api_wep_key128,Wlan_Entry.wep128Key3,WEP128_KEY_LEN);
			else
				memcpy(pEncInfo->rtk_api_wep_key128,Wlan_Entry.wep128Key4,WEP128_KEY_LEN);
		}else
			ISP_API_EXIT_WITH_VAL_ON(1, RTK_FAILED);
			
		break;

#ifdef CONFIG_RTL_WAPI_SUPPORT
	case ENCRYPT_WAPI:
		pEncInfo->rtk_api_wapi_auth = Wlan_Entry.wapiAuth;
		if(Wlan_Entry.wapiAuth == 1) //psk
		{
			pEncInfo->rtk_api_wapi_key_len = Wlan_Entry.wapiPskLen;
			pEncInfo->rtk_api_wapi_key_format = Wlan_Entry.wapiPskFormat;
			memcpy(pEncInfo->rtk_api_wapi_key,Wlan_Entry.wapiPsk,MAX_PSK_LEN+1);
		}
		else if((Wlan_Entry.wapiAuth == 0)
			memcpy(pEncInfo->rtk_api_wapi_as_ipaddr,Wlan_Entry.wapiAsIpAddr,IP_ADDR_LEN);
		else
			SP_API_EXIT_WITH_VAL_ON(1, RTK_FAILED);

		break;
#endif
		
	case ENCRYPT_WPA:
	case ENCRYPT_WPA2:
	case ENCRYPT_WPA2_MIXED:
#ifdef WLAN_WPA3
	case ENCRYPT_WPA3:
	case ENCRYPT_WPA3_MIXED:
#endif
		//authentication (non-1X)
		if(pEncInfo->rtk_api_enable_1x==0)
			pEncInfo->rtk_api_auth_type=Wlan_Entry.wpaAuth; 
		//encryption
		if(pEncInfo->encrypt_type==ENCRYPT_WPA||pEncInfo->encrypt_type==ENCRYPT_WPA2_MIXED)
			pEncInfo->rtk_api_wpa_cipher_suite=Wlan_Entry.unicastCipher;
		
		if(pEncInfo->encrypt_type==ENCRYPT_WPA2||pEncInfo->encrypt_type==ENCRYPT_WPA2_MIXED
#ifdef WLAN_WPA3
		||pEncInfo->encrypt_type==ENCRYPT_WPA3||pEncInfo->encrypt_type==ENCRYPT_WPA3_MIXED
#endif
		)
			pEncInfo->rtk_api_wpa2_cipher_suite=Wlan_Entry.wpa2UnicastCipher;
		
		pEncInfo->rtk_api_wpa_psk_format=Wlan_Entry.wpaPSKFormat;
		memcpy(pEncInfo->rtk_api_wpa_key,Wlan_Entry.wpaPSK,MAX_PSK_LEN+1);

		break;
		
	default:
		ISP_API_EXIT_WITH_VAL_ON(1, RTK_FAILED);
		break;
	}

	ret = RTK_SUCCESS;
	return ret; 
}

/**
* @brief
*	   Set the encryption info of wlan interface,
*	   Note that different encryption type requires different RTK_WLAN_ENCRYPT_INFO member value
*
* @param[in]
*	   ifname (unsigned char *) WLAN interface name to operate
*
* @return
*	   pEncInfo (RTK_WLAN_ENCRYPT_INFO) The struct of encryption info to be set
*
* @retval:
* - 	0 - RTK_SUCCESS
* -    -1 - RTK_FAILED
*/

int rtk_wlan_set_encryption(unsigned char *ifname, RTK_WLAN_ENCRYPT_INFO encInfo)
{
	int ret = RTK_FAILED;
	int vwlan_idx;
	char def_key_idx = 0;
	int wapi_len = 0;
	char tmp_buf[MAX_PSK_LEN+1] = {0};
	MIB_CE_MBSSIB_T Wlan_Entry;
	memset(&Wlan_Entry,0x0,sizeof(Wlan_Entry));
	ISP_API_EXIT_WITH_VAL_ON(ifname==NULL, RTK_FAILED);

	mib_save_wlanIdx();
	ret = set_wlan_idx_by_wlanIf(ifname,&vwlan_idx);
	ISP_API_GOTO_LABLE_ON(ret==RTK_FAILED, exit);
			
	if (!mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&Wlan_Entry)) {
		ISP_API_DEBUG("mib_chain get error\n");
		ISP_API_GOTO_LABLE_ON(1, exit);
	}
	//encrypt type
	Wlan_Entry.encrypt=encInfo.encrypt_type;
	
	if(!mib_set(MIB_WLAN_ENCRYPT, (void *)&encInfo.encrypt_type))
		ISP_API_GOTO_LABLE_ON(1, exit);
	//if encryption is none, exit now!
	if(encInfo.encrypt_type==ENCRYPT_DISABLED){
		printf("encrypt type is ENCRYPT_DISABLED\n");
		mib_chain_update(MIB_MBSSIB_TBL, (void *)&Wlan_Entry, vwlan_idx);	
		mib_recov_wlanIdx();	
		return RTK_SUCCESS;
	}
	//authentication(1X)
	Wlan_Entry.enable1X=encInfo.rtk_api_enable_1x;
	if(!mib_set(MIB_WLAN_ENABLE_1X, (void *)&encInfo.rtk_api_enable_1x))
		ISP_API_GOTO_LABLE_ON(1, exit);
	
	if(encInfo.rtk_api_enable_1x==1)
	{
		memcpy(Wlan_Entry.rsIpAddr,encInfo.rtk_api_rs_addr,IP_ADDR_LEN);
		Wlan_Entry.rsPort=encInfo.rtk_api_rs_port;
		memcpy(Wlan_Entry.rsPassword,encInfo.rtk_api_rs_pass,MAX_RS_PASS_LEN);
		
		if(!mib_set(MIB_WLAN_RS_IP, (void *)encInfo.rtk_api_rs_addr))
			ISP_API_GOTO_LABLE_ON(1, exit);

		if(!mib_set(MIB_WLAN_RS_PORT, (void *)&encInfo.rtk_api_rs_port))
			ISP_API_GOTO_LABLE_ON(1, exit);

		if(!mib_set(MIB_WLAN_RS_PASSWORD, (void *)&encInfo.rtk_api_rs_pass))
			ISP_API_GOTO_LABLE_ON(1, exit);
	}

	switch(encInfo.encrypt_type)
	{		
	case ENCRYPT_WEP:
		//authentication(non-1X)
		if(encInfo.rtk_api_enable_1x==0)
		{
			Wlan_Entry.authType=encInfo.rtk_api_auth_type;
			if(!mib_set(MIB_WLAN_AUTH_TYPE, (void *)&encInfo.rtk_api_auth_type))
				ISP_API_GOTO_LABLE_ON(1, exit);
		}
		
		//encryption
		Wlan_Entry.wep=encInfo.rtk_api_wep_key_len;
		Wlan_Entry.wepKeyType=encInfo.rtk_api_wep_key_type;
		Wlan_Entry.wepDefaultKey=def_key_idx;
		
		if(!mib_set(MIB_WLAN_WEP, (void *)&encInfo.rtk_api_wep_key_len))
			ISP_API_GOTO_LABLE_ON(1, exit);

		if(!mib_set(MIB_WLAN_WEP_KEY_TYPE, (void *)&encInfo.rtk_api_wep_key_type))
			ISP_API_GOTO_LABLE_ON(1, exit);

		if(!mib_set(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&def_key_idx))
			ISP_API_GOTO_LABLE_ON(1, exit);

		if(encInfo.rtk_api_wep_key_len==WEP64){
			if(def_key_idx==0){
				memset(Wlan_Entry.wep64Key1,0x0,WEP64_KEY_LEN); 	
				memcpy(Wlan_Entry.wep64Key1,encInfo.rtk_api_wep_key64,WEP64_KEY_LEN);
				if(! mib_set(MIB_WLAN_WEP64_KEY1, (void *)encInfo.rtk_api_wep_key64))
					ISP_API_GOTO_LABLE_ON(1, exit);
			}else if(def_key_idx==1){
				memset(Wlan_Entry.wep64Key2,0x0,WEP64_KEY_LEN); 	
				memcpy(Wlan_Entry.wep64Key2,encInfo.rtk_api_wep_key64,WEP64_KEY_LEN);
				if(!mib_set(MIB_WLAN_WEP64_KEY2, (void *)encInfo.rtk_api_wep_key64))	
					ISP_API_GOTO_LABLE_ON(1, exit);
			}else if(def_key_idx==2){
				memset(Wlan_Entry.wep64Key3,0x0,WEP64_KEY_LEN); 
				memcpy(Wlan_Entry.wep64Key3,encInfo.rtk_api_wep_key64,WEP64_KEY_LEN);
				if(!mib_set(MIB_WLAN_WEP64_KEY3, (void *)encInfo.rtk_api_wep_key64))
					ISP_API_GOTO_LABLE_ON(1, exit);
			}else{ 
				memset(Wlan_Entry.wep64Key4,0x0,WEP64_KEY_LEN);
				memcpy(Wlan_Entry.wep64Key4,encInfo.rtk_api_wep_key64,WEP64_KEY_LEN);
				if(!mib_set(MIB_WLAN_WEP64_KEY4, (void *)encInfo.rtk_api_wep_key64))
					ISP_API_GOTO_LABLE_ON(1, exit);
			}
		}else if(encInfo.rtk_api_wep_key_len==WEP128){
			if(def_key_idx==0){
				memset(Wlan_Entry.wep128Key1,0x0,WEP128_KEY_LEN);
				memcpy(Wlan_Entry.wep128Key1,encInfo.rtk_api_wep_key128,WEP128_KEY_LEN);
				if(!mib_set(MIB_WLAN_WEP128_KEY1, (void *)encInfo.rtk_api_wep_key128))
					ISP_API_GOTO_LABLE_ON(1, exit);
			}else if(def_key_idx==1){
				memset(Wlan_Entry.wep128Key2,0x0,WEP128_KEY_LEN);
				memcpy(Wlan_Entry.wep128Key2,encInfo.rtk_api_wep_key128,WEP128_KEY_LEN);
				if(!mib_set(MIB_WLAN_WEP128_KEY2, (void *)encInfo.rtk_api_wep_key128))
					ISP_API_GOTO_LABLE_ON(1, exit);
			}else if(def_key_idx==2){
				memset(Wlan_Entry.wep128Key3,0x0,WEP128_KEY_LEN);
				memcpy(Wlan_Entry.wep128Key3,encInfo.rtk_api_wep_key128,WEP128_KEY_LEN);
				if(!mib_set(MIB_WLAN_WEP128_KEY3, (void *)encInfo.rtk_api_wep_key128))			
					ISP_API_GOTO_LABLE_ON(1, exit);
			}else{ 
				memset(Wlan_Entry.wep128Key4,0x0,WEP128_KEY_LEN);
				memcpy(Wlan_Entry.wep128Key4,encInfo.rtk_api_wep_key128,WEP128_KEY_LEN);
				if(!mib_set(MIB_WLAN_WEP128_KEY4, (void *)encInfo.rtk_api_wep_key128))
					ISP_API_GOTO_LABLE_ON(1, exit);
				}
		}else
			ISP_API_GOTO_LABLE_ON(1, exit);

		break;

#ifdef CONFIG_RTL_WAPI_SUPPORT
	case ENCRYPT_WAPI:
	//authentication
	Wlan_Entry.wapiAuth = encInfo.rtk_api_wapi_auth;
	if(!mib_set(MIB_WLAN_WAPI_AUTH, (void *)encInfo.rtk_api_wapi_auth))
		ISP_API_GOTO_LABLE_ON(1, exit);

	if(Wlan_Entry.wapiAuth == 1) //psk
	{
		wapi_len = strlen(encInfo.rtk_api_wapi_key);
		Wlan_Entry.wapiPskFormat = encInfo.rtk_api_wapi_key_format;
		if(!mib_set(MIB_WLAN_WAPI_PSK_FORMAT, (void *)&encInfo.rtk_api_wapi_key_format))
			ISP_API_GOTO_LABLE_ON(1, exit);
		if(Wlan_Entry.wapiPskFormat == 1) //hex
		{
			if(rtk_string_to_hex(encInfo.rtk_api_wapi_key, tmp_buf, wapi_len) == 0)
				ISP_API_GOTO_LABLE_ON(1, exit);
			if((wapi_len & 1) || (wapi_len/2 >= MAX_PSK_LEN))
				ISP_API_GOTO_LABLE_ON(1, exit);
			wapi_len = wapi_len/2;
			Wlan_Entry.wapiPskLen = wapi_len;
			if(!mib_set(MIB_WLAN_WAPI_PSKLEN, &wapi_len))
				ISP_API_GOTO_LABLE_ON(1, exit);
			memcpy(Wlan_Entry.wapiPsk, tmp_buf, sizeof(Wlan_Entry.wapiPsk));
			if(!mib_set(MIB_WLAN_WAPI_PSK, (void *)tmp_buf))
				ISP_API_GOTO_LABLE_ON(1, exit);
		}
		else //passphrase
		{
			if(wapi_len==0 || wapi_len>(MAX_PSK_LEN-1))
				ISP_API_GOTO_LABLE_ON(1, exit);
			Wlan_Entry.wapiPskLen = wapi_len;
			if(!mib_set(MIB_WLAN_WAPI_PSKLEN, &wapi_len))
				ISP_API_GOTO_LABLE_ON(1, exit);
			memcpy(Wlan_Entry.wapiPsk, tmp_buf, sizeof(Wlan_Entry.wapiPsk));
			if(!mib_set(MIB_WLAN_WAPI_PSK, (void *)tmp_buf))
				ISP_API_GOTO_LABLE_ON(1, exit);
		}
	}
	else if(Wlan_Entry.wapiAuth == 0) //AS
	{
		Wlan_Entry.wapiAsIpAddr = encInfo.rtk_api_wapi_as_ipaddr;
		if(!mib_set(MIB_WLAN_WAPI_ASIPADDR, (void *)&encInfo.rtk_api_wapi_as_ipaddr))
			ISP_API_GOTO_LABLE_ON(1, exit);
	}
	else
		ISP_API_GOTO_LABLE_ON(1, exit);

	break;
#endif
		
	case ENCRYPT_WPA:
	case ENCRYPT_WPA2:
	case ENCRYPT_WPA2_MIXED:
#ifdef WLAN_WPA3
	case ENCRYPT_WPA3:
	case ENCRYPT_WPA3_MIXED:
#endif
		//authentication (non-1X)
		if(encInfo.rtk_api_enable_1x==0)
		{
			Wlan_Entry.wpaAuth=encInfo.rtk_api_auth_type;
			if(!mib_set(MIB_WLAN_WPA_AUTH, (void *)&encInfo.rtk_api_auth_type))
				ISP_API_GOTO_LABLE_ON(1, exit);
		}
		
		//encryption
		if(encInfo.encrypt_type==ENCRYPT_WPA||encInfo.encrypt_type==ENCRYPT_WPA2_MIXED)
		{
			Wlan_Entry.unicastCipher=encInfo.rtk_api_wpa_cipher_suite;
			if(!mib_set(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&encInfo.rtk_api_wpa_cipher_suite))
				ISP_API_GOTO_LABLE_ON(1, exit);
		}
		if(encInfo.encrypt_type==ENCRYPT_WPA2||encInfo.encrypt_type==ENCRYPT_WPA2_MIXED
#ifdef WLAN_WPA3
		||encInfo.encrypt_type==ENCRYPT_WPA3||encInfo.encrypt_type==ENCRYPT_WPA3_MIXED
#endif
		)
		{
			Wlan_Entry.wpa2UnicastCipher=encInfo.rtk_api_wpa2_cipher_suite;
			if(!mib_set(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&encInfo.rtk_api_wpa2_cipher_suite))
				ISP_API_GOTO_LABLE_ON(1, exit);
		}
		Wlan_Entry.wpaPSKFormat=encInfo.rtk_api_wpa_psk_format;
		memset(Wlan_Entry.wpaPSK,0x0,MAX_PSK_LEN+1);
		memcpy(Wlan_Entry.wpaPSK,encInfo.rtk_api_wpa_key,MAX_PSK_LEN+1);
		
		if(!mib_set(MIB_WLAN_WPA_PSK_FORMAT, (void *)&encInfo.rtk_api_wpa_psk_format))
			ISP_API_GOTO_LABLE_ON(1, exit);
		
		if(!mib_set(MIB_WLAN_WPA_PSK, (void *)encInfo.rtk_api_wpa_key))
			ISP_API_GOTO_LABLE_ON(1, exit);
		break;
	
	default:
		ISP_API_GOTO_LABLE_ON(1, exit);
		break;
	}
	mib_chain_update(MIB_MBSSIB_TBL, (void *)&Wlan_Entry, vwlan_idx);

	ret = RTK_SUCCESS;

exit:
	mib_recov_wlanIdx();

	return (ret==RTK_SUCCESS?RTK_SUCCESS:RTK_FAILED);
}

#if defined(WLAN_HAPD) || defined(WLAN_WPAS)
#define WRITE_HOSTAPD_GENERAL_SCRIPT_PARAM(dst, dst_size, tmp, tmp_size, str, val, ret_err) {	\
	snprintf(tmp, tmp_size, str, val); \
	if ((dst_size-1) < strlen(tmp)) \
		ret_err = 1; \
	else { \
	memcpy(dst, tmp, strlen(tmp)); \
	dst += strlen(tmp); \
		dst_size -= strlen(tmp); \
		ret_err = 0; \
	} \
}

int writeHostapdGeneralScript(const char *in, const char *out)
{
	int fh=-1;
	char tmpbuf[256]={0};
	char sysbuf[2048]={0};
	char *buf=NULL, *ptr=NULL, *sysptr=NULL;
	struct stat status;
	int ret_err_write=0, ssize, fsize;

	if (stat(in, &status) < 0) {
		printf("stat() error [%s]!\n", in);
		goto ERROR;
	}

	buf = malloc(status.st_size);
	if (buf == NULL) {
		printf("malloc() error [%d]!\n", (int)status.st_size);
		goto ERROR;
	}

	ptr = buf;
	memset(ptr, '\0', status.st_size);

	fh = open(in, O_RDONLY);
	if (fh == -1) {
		printf("open() error [%s]!\n", in);
		goto ERROR;
	}

	lseek(fh, 0L, SEEK_SET);
	if (read(fh, ptr, status.st_size) != status.st_size) {
		printf("read() error [%s]!\n", in);
		goto ERROR;
	}

	ptr = strstr(ptr, "System Settings - END");
	ptr += strlen("System Settings - END");
	fsize = ptr - buf;
	close(fh);

	sysptr = sysbuf;
	ssize = sizeof(sysbuf);

	WRITE_HOSTAPD_GENERAL_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"SYSTEMD_USOCK_SERVER=\"%s\"\n", SYSTEMD_USOCK_SERVER, ret_err_write);

	if (ret_err_write) {
		printf("WRITE_HOSTAPD_WPS_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

#ifdef RESTRICT_PROCESS_PERMISSION
	fh = open(out, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR|S_IXGRP|S_IXOTH);
#else
	fh = open(out, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
#endif
	if (fh == -1) {
		printf("Create hostapd wps file %s error!\n", out);
		goto ERROR;
	}
	snprintf(tmpbuf, sizeof(tmpbuf), "#!/bin/sh\n");
	if (write(fh, tmpbuf, strlen(tmpbuf)) != strlen(tmpbuf)) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}

	if (write(fh, sysbuf, sysptr-sysbuf) != (sysptr-sysbuf) ) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}

	if (write(fh, ptr, (status.st_size-fsize)) != (status.st_size-fsize) ) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}
	close(fh);
	free(buf);

	return 0;
ERROR:
	if (buf) free(buf);
	if (-1 != fh) close(fh);

	return -1;
}
#endif

#if defined(WLAN_WPS_HAPD) || defined(WLAN_WPS_WPAS)
int rtk_wlan_get_wps_led_interface(char *ifname)
{
	FILE *fp=NULL;
	char buf[32]={0};
	int ret = -1;
	fp = fopen(WPS_LED_INTERFACE, "r");
	if(fp){
		if(fscanf(fp, "%s", buf) == 1){
			strcpy(ifname, buf);
			ret = 0;
		}
		fclose(fp);
	}
	return ret;
}

static int getIP(char *ip, char *mac)
{
	FILE *fp;
	char strbuf[256];
	char *ptr;
	int ret = -1;

	fp = fopen("/proc/net/arp", "r");
	if (fp == NULL){
		printf("read arp file fail!\n");
		return ret;
	}
	fgets(strbuf, sizeof(strbuf), fp);
	while (fgets(strbuf, sizeof(strbuf), fp)) {
		ptr=strstr(strbuf, mac);
		if(ptr!=NULL){
			sscanf(strbuf, "%s %*s %*s %*s %*s %*s", ip);
			//printf("ip %s\n", ip);
			ret = 0;
			goto end;
		}
	}
end:
	fclose(fp);
	return ret;
}
static int check_wps_dev_info(char *mac, char *devinfo)
{
	char ipaddr[20] = {0};
	int ret = -1;
	ret = getIP(ipaddr, mac);
	sprintf(devinfo, "IP:%s;MAC:%s", ipaddr, mac);
	return ret;
}

void run_wps_query(int ssid_idx, char *wps_status, char *devinfo)
{
	FILE *fp=NULL;
	char buf[4][1024]={0}, newline[1024]={0};
	char ifname[IFNAMSIZ]={0};
	int i=0, flag=0;
	rtk_wlan_get_ifname_by_ssid_idx(ssid_idx, ifname);

	if (getInFlags(ifname, &flag) == 0 || (flag & IFF_UP) == 0 || (flag & IFF_RUNNING) == 0)
	{
		*wps_status=4;
		return;
	}

	snprintf(buf[0], sizeof(buf[0]), "%s wps_get_status -i %s", HOSTAPD_CLI, ifname);

	fp = popen(buf[0], "r");
	if(fp)
	{
		for(i=0; i<4;i++){
			if(fgets(newline, 1024, fp) != NULL){
				sscanf(newline, "%*[^:]:%s", buf[i]);
				//printf("sw_commit %d\n", version);
				//printf("%s\n", buf[i]);
			}
		}
		pclose(fp);
	}
	else{
		*wps_status=4;
		return;
	}
	if(!strcmp(buf[0], "Active")){
		*wps_status=0;
	}
	else if(!strcmp(buf[0], "Disabled") && !strcmp(buf[1], "None")){
		*wps_status=4;
	}
	else if(!strcmp(buf[0], "Timed-out")){
		*wps_status=3;
	}
	else if(!strcmp(buf[1], "Failed")){
		*wps_status=1;
	}
	else if(!strcmp(buf[1], "Success")){
		*wps_status=2;
		if(check_wps_dev_info(buf[2], devinfo) != 0)
			*wps_status=0;
	}
}

static char *_get_token(char *data, char *token)
{
	char *ptr=data;
	int len=0, idx=0;

	while (*ptr && *ptr != '\n' ) {
		if (*ptr == '=') {
			if (len <= 1)
				return NULL;
			memcpy(token, data, len);

			/* delete ending space */
			for (idx=len-1; idx>=0; idx--) {
				if (token[idx] !=  ' ')
					break;
			}
			token[idx+1] = '\0';

			return ptr+1;
		}
		len++;
		ptr++;
	}
	return NULL;
}

static int _get_value(char *data, char *value)
{
	char *ptr=data;
	int len=0, idx, i;

	while (*ptr && *ptr != '\n' && *ptr != '\r') {
		len++;
		ptr++;
	}

	/* delete leading space */
	idx = 0;
	while (len-idx > 0) {
		if (data[idx] != ' ')
			break;
		idx++;
	}
	len -= idx;

	/* delete bracing '"' */
	if (data[idx] == '"') {
		for (i=idx+len-1; i>idx; i--) {
			if (data[i] == '"') {
				idx++;
				len = i - idx;
			}
			break;
		}
	}

	if (len > 0) {
		memcpy(value, &data[idx], len);
		value[len] = '\0';
	}
	return len;
}

#define WRITE_SCRIPT_PARAM(dst, dst_size, tmp, tmp_size, str, val, ret_err) {	\
	snprintf(tmp, tmp_size, str, val); \
	if ((dst_size-1) < strlen(tmp)) \
		ret_err = 1; \
	else { \
	memcpy(dst, tmp, strlen(tmp)); \
	dst += strlen(tmp); \
		dst_size -= strlen(tmp); \
		ret_err = 0; \
	} \
}

const char *LED_WPS_G_LABEL_NAME[]={"LED_WPS_G", "LED_WPS", ""};
#define WPS_LED_CONTROL_STATUS_FILE "/tmp/wps_led_control_status"
#define WPS_LED_CONTROL_RESTORE_FILE "/tmp/wps_led_control_restore"
#define WPS_LED_TIMER_PID "/var/run/wps_timer.sh.pid"

int writeWpsScript(const char *in, const char *out)
{
	int fh=-1;
	char tmpbuf[256]={0};
	char sysbuf[2048]={0};
	char *buf=NULL, *ptr=NULL, *sysptr=NULL;
	struct stat status;
	int ret_err_write=0, ssize, fsize;
	int index = 0;

	if (stat(in, &status) < 0) {
		printf("stat() error [%s]!\n", in);
		goto ERROR;
	}

	buf = malloc(status.st_size);
	if (buf == NULL) {
		printf("malloc() error [%d]!\n", (int)status.st_size);
		goto ERROR;
	}

	ptr = buf;
	memset(ptr, '\0', status.st_size);

	fh = open(in, O_RDONLY);
	if (fh == -1) {
		printf("open() error [%s]!\n", in);
		goto ERROR;
	}

	lseek(fh, 0L, SEEK_SET);
	if (read(fh, ptr, status.st_size) != status.st_size) {
		printf("read() error [%s]!\n", in);
		goto ERROR;
	}

	ptr = strstr(ptr, "System Settings - END");
	ptr += strlen("System Settings - END");
	fsize = ptr - buf;
	close(fh);

	sysptr = sysbuf;
	ssize = sizeof(sysbuf);

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"SYSTEMD_USOCK_SERVER=\"%s\"\n", SYSTEMD_USOCK_SERVER, ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"WPS_LED_INTERFACE=\"%s\"\n", WPS_LED_INTERFACE, ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	while(strlen(LED_WPS_G_LABEL_NAME[index]) != 0){
		snprintf(tmpbuf, sizeof(tmpbuf), "/sys/class/leds/%s/brightness", LED_WPS_G_LABEL_NAME[index]);
		if(!access(tmpbuf, F_OK))
			break;
		index++;
	}
	if(strlen(LED_WPS_G_LABEL_NAME[index]) == 0){
		printf("WPS LED proc is not found!\nPlease check if you need WPS LED!\n");
		index--;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"LED_WPS_G_LABEL=\"%s\"\n", LED_WPS_G_LABEL_NAME[index], ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"WPS_LED_CONTROL_STATUS_FILE=\"%s\"\n", WPS_LED_CONTROL_STATUS_FILE, ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"WPS_LED_CONTROL_RESTORE_FILE=\"%s\"\n", WPS_LED_CONTROL_RESTORE_FILE, ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"WPS_LED_TIMER_PID=\"%s\"\n", WPS_LED_TIMER_PID, ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

#if 0
	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"LED_WPS_G_TRIGGER=\"/sys/class/leds/%s/trigger\"\n", LED_WPS_G_LABEL_NAME[index], ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"LED_WPS_G_DELAY_ON=\"/sys/class/leds/%s/delay_on\"\n", LED_WPS_G_LABEL_NAME[index], ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"LED_WPS_G_DELAY_OFF=\"/sys/class/leds/%s/delay_off\"\n", LED_WPS_G_LABEL_NAME[index], ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"LED_WPS_G_BRIGHTNESS=\"/sys/class/leds/%s/brightness\"\n", LED_WPS_G_LABEL_NAME[index], ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}

	WRITE_SCRIPT_PARAM(sysptr, ssize, tmpbuf, sizeof(tmpbuf),
		"LED_WPS_G_SHOT=\"/sys/class/leds/%s/shot\"\n", LED_WPS_G_LABEL_NAME[index], ret_err_write);

	if (ret_err_write) {
		printf("WRITE_SCRIPT_PARAM memcpy error\n");
		goto ERROR;
	}
#endif


#ifdef RESTRICT_PROCESS_PERMISSION
	fh = open(out, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR|S_IXGRP|S_IXOTH);
#else
	fh = open(out, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
#endif
	if (fh == -1) {
		printf("Create hostapd wps file %s error!\n", out);
		goto ERROR;
	}
	snprintf(tmpbuf, sizeof(tmpbuf), "#!/bin/sh\n");
	if (write(fh, tmpbuf, strlen(tmpbuf)) != strlen(tmpbuf)) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}

	if (write(fh, sysbuf, sysptr-sysbuf) != (sysptr-sysbuf) ) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}

	if (write(fh, ptr, (status.st_size-fsize)) != (status.st_size-fsize) ) {
		printf("Write() file error [%s]!\n", out);
		goto ERROR;
	}
	close(fh);
	free(buf);

	return 0;
ERROR:
	if (buf) free(buf);
	if (-1 != fh) close(fh);

	return -1;
}

// led_mode: 0:led off   1:led on 2:led state restore
int _set_wlan_wps_led_status(int led_mode)
{
	int led_index = 0;
	unsigned int max_brightness = 0;
	char buf[128]={0}, cmd[256]={0};
	FILE *fp=NULL;
	while(strlen(LED_WPS_G_LABEL_NAME[led_index]) != 0){
		snprintf(buf, sizeof(buf), "/sys/class/leds/%s/brightness", LED_WPS_G_LABEL_NAME[led_index]);
		if(!access(buf, F_OK))
			break;
		led_index++;
	}
	if(strlen(LED_WPS_G_LABEL_NAME[led_index]) == 0){
		//printf("WPS LED proc is not found!\nPlease check if you need WPS LED!\n");
		return -1;
	}
	if(led_mode != 2){
		snprintf(cmd, sizeof(cmd), "echo 1 > %s", WPS_LED_CONTROL_STATUS_FILE);
		va_niced_cmd("/bin/sh", 2, 1, "-c", cmd);
		if(led_mode == 1){
			snprintf(buf, sizeof(buf), "/sys/class/leds/%s/max_brightness", LED_WPS_G_LABEL_NAME[led_index]);
			fp=fopen(buf, "r");
			if(fp){
				fscanf(fp, "%u", &max_brightness);
				fclose(fp);
			}
		}
		snprintf(cmd, sizeof(cmd), "echo none > /sys/class/leds/%s/trigger", LED_WPS_G_LABEL_NAME[led_index]);
		va_niced_cmd("/bin/sh", 2, 1, "-c", cmd);
		snprintf(cmd, sizeof(cmd), "echo %u > /sys/class/leds/%s/brightness", max_brightness, LED_WPS_G_LABEL_NAME[led_index]);
		va_niced_cmd("/bin/sh", 2, 1, "-c", cmd);
	}
	else{ //restore
		unlink(WPS_LED_CONTROL_STATUS_FILE);
		if(!access(WPS_LED_CONTROL_RESTORE_FILE, F_OK)){
			fp = fopen(WPS_LED_CONTROL_RESTORE_FILE, "r");
			if(fp){
				while(fgets(cmd,sizeof(cmd),fp)){
					va_niced_cmd("/bin/sh", 2, 1, "-c", cmd);
					//printf("cmd: %s\n", cmd);
				}
				fclose(fp);
			}
			unlink(WPS_LED_CONTROL_RESTORE_FILE);
		}
		else {
			if(!access(WPS_LED_TIMER_PID, F_OK)){
				snprintf(buf, sizeof(buf), "/sys/class/leds/%s/max_brightness", LED_WPS_G_LABEL_NAME[led_index]);
				fp=fopen(buf, "r");
				if(fp){
					fscanf(fp, "%u", &max_brightness);
					fclose(fp);
				}
			}
			snprintf(cmd, sizeof(cmd), "echo %u > /sys/class/leds/%s/brightness", max_brightness, LED_WPS_G_LABEL_NAME[led_index]);
			va_niced_cmd("/bin/sh", 2, 1, "-c", cmd);
			//printf("cmd: %s\n", cmd);
		}
	}
	return 0;
}
#endif

#if defined(WLAN_WPS_HAPD)
void rtk_wlan_update_wlan_wps_from_file(char *ifname)
{
	FILE *fp=NULL;
	char line[200], value[100], token[40], tmpBuf[40]={0}, *ptr;
	int set_band=0, i, tmp_wpa=0, tmp_wpa_key_mgmt=0, tmp_wpa_pairwise=0, tmp_wsc_config=0, is_restart_wlan=0;
	int /*ori_wlan_idx = wlan_idx,*/ virtual_idx=0;
	MIB_CE_MBSSIB_T Entry, Entry1;
	int vwlan_idx[NUM_WLAN_INTERFACE]={0}, root_idx[NUM_WLAN_INTERFACE]={0};
	char filename[256];

	for(i=0; i<NUM_WLAN_INTERFACE ; i++)
	{
		if(!strncmp(ifname, WLANIF[i], strlen(WLANIF[i])))
			break;
	}
	if(i>=NUM_WLAN_INTERFACE)
		return;

	
	snprintf(filename, sizeof(filename), HAPD_CONF_PATH_NAME, ifname);
	fp = fopen(filename, "r");

	if(!fp){
		printf("[%s]open %s fail\n", __FUNCTION__,filename);
		return;
	}
	printf("%s %s\n", __func__, ifname);

	memset(&Entry, 0, sizeof(MIB_CE_MBSSIB_T));
	if(fgets(line, sizeof(line), fp))
	{
		if(!strstr(line,"WPS configuration - START"))
		{
			fclose(fp);
			return;
		}
		is_restart_wlan = 1;
		while ( fgets(line, sizeof(line), fp) ) {
			if(strstr(line,"WPS configuration - END"))
				break;
			ptr = (char *)_get_token(line, token);
			if (ptr == NULL){
				continue;
			}
			if (_get_value(ptr, value)==0)
				continue;
			printf("%s=%s\n", token, value);
			if(!strcmp(token, "wps_state")){
				if(atoi(value) == 2)
				{
					Entry.wsc_configured = 1;
					if(virtual_idx == 0)
						tmp_wsc_config = 1;
				}
				else
					Entry.wsc_configured = 0;
			}else if(!strcmp(token, "ssid")){
				memset(Entry.ssid, 0, sizeof(Entry.ssid));
				strncpy(Entry.ssid, value, sizeof(Entry.ssid)-1);
			}else if(!strcmp(token, "wpa")){
				tmp_wpa = atoi(value);
			}else if(!strcmp(token, "wpa_key_mgmt")){
				if(!strcmp(value, "WPA-PSK"))
				{
					tmp_wpa_key_mgmt = 1;
					Entry.wpaAuth = WPA_AUTH_PSK;
				}
				else if(!strcmp(value, "WPA-PSK-SHA256"))
				{
					Entry.encrypt = WIFI_SEC_WPA2;
#ifdef WLAN_11W
					Entry.sha256 = 1;
#endif
					Entry.wpaAuth = WPA_AUTH_PSK;
				}
			}else if(!strcmp(token, "wpa_pairwise")){
				Entry.unicastCipher = 0;
				Entry.wpa2UnicastCipher = 0;
				if(strstr(value,"TKIP") && strstr(value,"CCMP"))
					tmp_wpa_pairwise = WPA_CIPHER_MIXED;
				else if(strstr(value,"TKIP"))
					tmp_wpa_pairwise = WPA_CIPHER_TKIP;
				else if(strstr(value,"CCMP"))
					tmp_wpa_pairwise = WPA_CIPHER_AES;
			}else if(!strcmp(token, "wpa_passphrase")){
				strncpy(Entry.wpaPSK, value, sizeof(Entry.wpaPSK));
				strncpy(Entry.wscPsk, value, sizeof(Entry.wscPsk));
				Entry.wpaPSKFormat = 0;
			}else if(!strcmp(token, "wpa_psk")){
				strncpy(Entry.wpaPSK, value, sizeof(Entry.wpaPSK));
				strncpy(Entry.wscPsk, value, sizeof(Entry.wscPsk));
				Entry.wpaPSKFormat = 1;
			}else if(!strcmp(token, "auth_algs")){
				if(atoi(value) == 2)
					Entry.authType = 1;
				else if(atoi(value) == 1)
					Entry.authType = 2;
			}
			else
				printf("[%s %d]unkown mib:%s\n", __FUNCTION__,__LINE__, token);
		}
		fclose(fp);
		if(tmp_wpa_pairwise == WPA_CIPHER_MIXED)
		{
			Entry.unicastCipher = 3;
			Entry.wpa2UnicastCipher = 3;
		}
		else if(tmp_wpa_pairwise == WPA_CIPHER_AES)
		{
			Entry.wpa2UnicastCipher = 2;
		}
		else if(tmp_wpa_pairwise == WPA_CIPHER_TKIP)
		{
			Entry.unicastCipher = 1;
		}
		if(tmp_wpa_key_mgmt == 1)
		{
			if(tmp_wpa == 3)
			{
				Entry.encrypt = WIFI_SEC_WPA2_MIXED;
				Entry.wsc_auth = WSC_AUTH_WPA2PSKMIXED;
				Entry.wsc_enc = WSC_ENCRYPT_TKIPAES;
			}
			else if(tmp_wpa == 2)
			{
				Entry.encrypt = WIFI_SEC_WPA2;
				Entry.wsc_auth = WSC_AUTH_WPA2PSK;
				Entry.wsc_enc = WSC_ENCRYPT_AES;
			}
			else if(tmp_wpa == 1)
			{
				Entry.encrypt = WIFI_SEC_WPA;
				Entry.wsc_auth = WSC_AUTH_WPAPSK;
				Entry.wsc_enc = WSC_ENCRYPT_TKIP;
			}
		}
		//printf("=============================\n\n");
#if !defined(CONFIG_RTK_DEV_AP)
		for(i=0; i<NUM_WLAN_INTERFACE; i++)
#endif
		{
			memset(&Entry1, 0, sizeof(MIB_CE_MBSSIB_T));
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, virtual_idx, (void *)&Entry1);
			strncpy(Entry1.ssid, Entry.ssid, sizeof(Entry.ssid));
			Entry1.authType = Entry.authType;
			Entry1.encrypt = Entry.encrypt;
			if(Entry.encrypt==ENCRYPT_WEP)
				printf("[%s %d]remote ap encrypt is wep\n", __FUNCTION__,__LINE__);
			Entry1.wsc_auth = Entry.wsc_auth;
			Entry1.wpaAuth = Entry.wpaAuth;
			strncpy(Entry1.wpaPSK, Entry.wpaPSK, sizeof(Entry.wpaPSK));
			strncpy(Entry1.wscPsk, Entry.wscPsk, sizeof(Entry.wscPsk));
			Entry1.wpaPSKFormat = Entry.wpaPSKFormat;
			Entry1.unicastCipher = Entry.unicastCipher;
			Entry1.wpa2UnicastCipher = Entry.wpa2UnicastCipher;
			Entry1.wep = Entry.wep;
			Entry1.wsc_enc = Entry.wsc_enc;
			Entry1.wsc_configured = Entry.wsc_configured;
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry1, virtual_idx);
			if(virtual_idx==0){
				mib_local_mapping_set(MIB_WLAN_SSID, i, Entry.ssid);
				mib_local_mapping_set(MIB_WLAN_ENCRYPT, i, &Entry.encrypt);
				mib_local_mapping_set(MIB_WLAN_AUTH_TYPE, i, &Entry.authType);
				mib_local_mapping_set(MIB_WSC_AUTH, i, &Entry.wsc_auth);
				mib_local_mapping_set(MIB_WLAN_WPA_AUTH, i, &Entry.wpaAuth);
				mib_local_mapping_set(MIB_WLAN_WPA_PSK_FORMAT, i, &Entry.wpaPSKFormat);
				mib_local_mapping_set(MIB_WLAN_WPA_PSK, i, Entry.wpaPSK);
				mib_local_mapping_set(MIB_WSC_PSK, i, Entry.wscPsk);
				mib_local_mapping_set(MIB_WLAN_WPA_CIPHER_SUITE, i, &Entry.unicastCipher);
				mib_local_mapping_set(MIB_WLAN_WPA2_CIPHER_SUITE, i, &Entry.wpa2UnicastCipher);
				mib_local_mapping_set(MIB_WSC_ENC, i, &Entry.wsc_enc);
				mib_local_mapping_set(MIB_WSC_CONFIGURED, i, &Entry.wsc_configured);
			}
		}
	}
	else
		fclose(fp);

	if(tmp_wsc_config == 1)
	{
		for(i=0; i<NUM_WLAN_INTERFACE ; i++)
		{
			memset(&Entry1, 0, sizeof(MIB_CE_MBSSIB_T));
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, 0, (void *)&Entry1);
			Entry1.wsc_configured = 1;
			mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry1, 0);
		}
	}

	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	if(is_restart_wlan == 1)
	{
		printf("wps wlan restart\n");
		//pthreadWlanConfig(ACT_RESTART, CONFIG_SSID_ALL);
		config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
	}
	return;
}

void rtk_wlan_update_wlan_wps_from_string(char *ifname, char *hexdump)
{
	//do nothing
	return;
}
#endif

#ifdef WLAN_WPS_WPAS
void rtk_wlan_update_wlan_client_wps_from_file(char *ifname)
{
#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
	FILE *fp=NULL;
	int wlanIdx=0, vwlan_idx=0, network_num=0, tmp_wpa_pairwise=0, tmp_wpa_key_mgmt=0, tmp_wpa=0, restart_flag=0;
	MIB_CE_MBSSIB_T Entry;
	char line[200]={0}, value[100], token[40], tmpBuf[40]={0}, *ptr;
#ifdef WLAN_UNIVERSAL_REPEATER
	unsigned char rpt_enabled=0;
#endif
	char filename[256]={0};

	if(!ifname)
		return;

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));

	if(rtk_wlan_get_mib_index_mapping_by_ifname(ifname, &wlanIdx, &vwlan_idx) != 0){
		printf("can not find correct wlan interface!");
		return;
	}

	if (!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIdx, vwlan_idx, (void *)&Entry)){
		printf("mib_chain get error\n");
		return;
	}

#ifdef WLAN_UNIVERSAL_REPEATER
	mib_local_mapping_get(MIB_REPEATER_ENABLED1, wlanIdx, (void *)&rpt_enabled);
#endif

	if(Entry.wlanMode == CLIENT_MODE
#ifdef WLAN_UNIVERSAL_REPEATER
		|| rpt_enabled
#endif
	)
	{
		memset(filename, 0, sizeof(filename));
		if(vwlan_idx == 0)
			snprintf(filename, sizeof(filename), WPAS_CONF_PATH_NAME, WLANIF[wlanIdx]);
#ifdef WLAN_UNIVERSAL_REPEATER
		else if(rpt_enabled && vwlan_idx == WLAN_REPEATER_ITF_INDEX)
			snprintf(filename, sizeof(filename), WPAS_CONF_PATH_VXD_NAME, WLANIF[wlanIdx]);
#endif

		fp = fopen(filename, "r");

		if(!fp){
			printf("[%s]open %s fail\n", __FUNCTION__, filename);
			return;
		}

		while (fgets(line, sizeof(line), fp))
		{
			if(strstr(line,"network={"))
				network_num++;

			if(network_num == 2)
			{
				restart_flag = 1;
				while ( fgets(line, sizeof(line), fp) )
				{
					ptr = (char *)_get_token(line, token);

					if (ptr == NULL)
						continue;
					if (_get_value(ptr, value)==0)
						continue;
					printf("%s=%s\n", token, value);
					if(!strcmp(token, "	ssid"))
					{
						memset(Entry.ssid, 0, sizeof(Entry.ssid));
						strncpy(Entry.ssid, value, sizeof(Entry.ssid)-1);
					}
					else if(!strcmp(token, "	psk"))
					{
						strncpy(Entry.wpaPSK, value, sizeof(Entry.wpaPSK));
						strncpy(Entry.wscPsk, value, sizeof(Entry.wscPsk));
						Entry.wpaPSKFormat = 0;
						if(strlen(Entry.wpaPSK) > 63)
							Entry.wpaPSKFormat = 1;
					}
					else if(!strcmp(token, "	pairwise"))
					{
						Entry.unicastCipher = 0;
						Entry.wpa2UnicastCipher = 0;

						if(strstr(value,"TKIP") && strstr(value,"CCMP"))
							tmp_wpa_pairwise = WPA_CIPHER_MIXED;
						else if(strstr(value,"TKIP"))
							tmp_wpa_pairwise = WPA_CIPHER_TKIP;
						else if(strstr(value,"CCMP"))
							tmp_wpa_pairwise = WPA_CIPHER_AES;
					}
					else if(!strcmp(token, "	key_mgmt"))
					{
						if(!strcmp(value, "WPA-PSK"))
						{
							tmp_wpa_key_mgmt = 1;
							Entry.wpaAuth = WPA_AUTH_PSK;
#ifdef WLAN_11W
							Entry.sha256 = 1;
							Entry.dotIEEE80211W = 1;
#endif
						}
						else if(!strcmp(value, "WPA-PSK-SHA256"))
						{
							Entry.encrypt = WIFI_SEC_WPA2;
#ifdef WLAN_11W
							Entry.sha256 = 1;
#endif
							Entry.wpaAuth = WPA_AUTH_PSK;
						}
						else if(!strcmp(value, "NONE"))
						{
							Entry.encrypt = WIFI_SEC_NONE;
							Entry.wsc_auth = WSC_AUTH_OPEN;
							Entry.wsc_enc = WSC_ENCRYPT_NONE;
						}
					}
					else if(!strcmp(token, "	proto"))
					{
						if(strstr(value,"WPA") && strstr(value,"RSN"))
							tmp_wpa = 3;
						else if(strstr(value,"RSN"))
							tmp_wpa = 2;
						else if(strstr(value,"WPA"))
							tmp_wpa = 1;
					}
					else if(!strcmp(token, "	auth_alg"))
					{
						if(strstr(value,"SHARED"))
							Entry.authType = 1;
					}
				}

				break;
			}

		}

		fclose(fp);

		if(restart_flag)
		{
			if(tmp_wpa_pairwise == WPA_CIPHER_MIXED)
			{
				Entry.unicastCipher = 3;
				Entry.wpa2UnicastCipher = 3;
			}
			else if(tmp_wpa_pairwise == WPA_CIPHER_AES)
			{
				Entry.wpa2UnicastCipher = 2;
			}
			else if(tmp_wpa_pairwise == WPA_CIPHER_TKIP)
			{
				Entry.unicastCipher = 1;
			}

			if(tmp_wpa_key_mgmt == 1)
			{
				if(tmp_wpa == 3)
				{
					Entry.encrypt = WIFI_SEC_WPA2_MIXED;
					Entry.wsc_auth = WSC_AUTH_WPA2PSKMIXED;
					Entry.wsc_enc = WSC_ENCRYPT_TKIPAES;
				}
				else if(tmp_wpa == 2)
				{
#ifdef WLAN_WPA3
					Entry.encrypt = WIFI_SEC_WPA2_WPA3_MIXED;
#ifdef WLAN_11W
					Entry.dotIEEE80211W = 1;
					Entry.sha256 = 0;
#endif
#else
					Entry.encrypt = WIFI_SEC_WPA2;
#endif
					Entry.wsc_auth = WSC_AUTH_WPA2PSK;
					Entry.wsc_enc = WSC_ENCRYPT_AES;
				}
				else if(tmp_wpa == 1)
				{
					Entry.encrypt = WIFI_SEC_WPA;
					Entry.wsc_auth = WSC_AUTH_WPAPSK;
					Entry.wsc_enc = WSC_ENCRYPT_TKIP;
				}
			}

			Entry.wsc_configured = 1;

			if (!mib_chain_local_mapping_update(MIB_MBSSIB_TBL, wlanIdx, (void *)&Entry, vwlan_idx)){
				printf("mib_chain update error\n");
				return;
			}

			if(vwlan_idx == 0){

				mib_local_mapping_set(MIB_WLAN_SSID, wlanIdx, Entry.ssid);
				mib_local_mapping_set(MIB_WLAN_ENCRYPT, wlanIdx, &Entry.encrypt);
				mib_local_mapping_set(MIB_WLAN_AUTH_TYPE, wlanIdx, &Entry.authType);
				mib_local_mapping_set(MIB_WSC_AUTH, wlanIdx, &Entry.wsc_auth);
				mib_local_mapping_set(MIB_WLAN_WPA_AUTH, wlanIdx, &Entry.wpaAuth);
				mib_local_mapping_set(MIB_WLAN_WPA_PSK_FORMAT, wlanIdx, &Entry.wpaPSKFormat);
				mib_local_mapping_set(MIB_WLAN_WPA_PSK, wlanIdx, Entry.wpaPSK);
				mib_local_mapping_set(MIB_WSC_PSK, wlanIdx, Entry.wscPsk);
				mib_local_mapping_set(MIB_WLAN_WPA_CIPHER_SUITE, wlanIdx, &Entry.unicastCipher);
				mib_local_mapping_set(MIB_WLAN_WPA2_CIPHER_SUITE, wlanIdx, &Entry.wpa2UnicastCipher);
				mib_local_mapping_set(MIB_WSC_ENC, wlanIdx, &Entry.wsc_enc);
				mib_local_mapping_set(MIB_WSC_CONFIGURED, wlanIdx, &Entry.wsc_configured);

			}

			mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
			printf("wps wlan restart\n");
			config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
		}
	}
#endif //WLAN_CLIENT || WLAN_UNIVERSAL_REPEATER
}

/** WPS overlap detected in PBC mode */
#define WPS_EVENT_OVERLAP "WPS-OVERLAP-DETECTED"
/** WPS registration failed after M2/M2D */
#define WPS_EVENT_FAIL "WPS-FAIL"
/** WPS registration completed successfully */
#define WPS_EVENT_SUCCESS "WPS-SUCCESS"
/** WPS enrollment attempt timed out and was terminated */
#define WPS_EVENT_TIMEOUT "WPS-TIMEOUT"
/* PBC mode was activated */
#define WPS_EVENT_ACTIVE "WPS-PBC-ACTIVE"
/* PBC mode was disabled */
#define WPS_EVENT_DISABLE "WPS-PBC-DISABLE"

void rtk_wlan_store_wpas_wps_status(char *event_str, char *ifname)
{
#if defined(WLAN_CLIENT) || defined(WLAN_UNIVERSAL_REPEATER)
	MIB_CE_MBSSIB_T Entry;
	int wlanIdx=0, vwlan_idx=0;
	FILE *wpas_wps_stas;
#ifdef WLAN_UNIVERSAL_REPEATER
	unsigned char rpt_enabled=0;
#endif

	if(!event_str || !ifname)
		return;

	memset(&Entry,0,sizeof(MIB_CE_MBSSIB_T));

	if(rtk_wlan_get_mib_index_mapping_by_ifname(ifname, &wlanIdx, &vwlan_idx) != 0){
		printf("can not find correct wlan interface!");
		return;
	}

	if (!mib_chain_local_mapping_get(MIB_MBSSIB_TBL, wlanIdx, vwlan_idx, (void *)&Entry)){
		printf("mib_chain get error\n");
		return;
	}

#ifdef WLAN_UNIVERSAL_REPEATER
	mib_local_mapping_get(MIB_REPEATER_ENABLED1, wlanIdx, (void *)&rpt_enabled);
#endif

	if(Entry.wlanMode == CLIENT_MODE
#ifdef WLAN_UNIVERSAL_REPEATER
		|| rpt_enabled
#endif
	)
	{
		wpas_wps_stas = fopen("/var/wpas_wps_stas", "w+");

		if(wpas_wps_stas == NULL){
			printf("!! Error,  [%s][%d] wpas_wps_stas = NULL \n", __FUNCTION__, __LINE__);
			return;
		}

		if(!strcmp(event_str, WPS_EVENT_ACTIVE))
		{
			fprintf(wpas_wps_stas, "%s", "PBC Status: Active\n");
			fprintf(wpas_wps_stas, "%s", "Last WPS result: None\n");
		}
		else if(!strcmp(event_str, WPS_EVENT_TIMEOUT))
		{
			fprintf(wpas_wps_stas, "%s", "PBC Status: Timed-out\n");
			fprintf(wpas_wps_stas, "%s", "Last WPS result: None\n");
		}
		else if(!strcmp(event_str, WPS_EVENT_OVERLAP))
		{
			fprintf(wpas_wps_stas, "%s", "PBC Status: Overlap\n");
			fprintf(wpas_wps_stas, "%s", "Last WPS result: None\n");
		}
		else if(!strcmp(event_str, WPS_EVENT_FAIL))
		{
			fprintf(wpas_wps_stas, "%s", "PBC Status: Disabled\n");
			fprintf(wpas_wps_stas, "%s", "Last WPS result: Failed\n");
		}
		else if(!strcmp(event_str, WPS_EVENT_SUCCESS))
		{
			fprintf(wpas_wps_stas, "%s", "PBC Status: Disabled\n");
			fprintf(wpas_wps_stas, "%s", "Last WPS result: Success\n");
		}
		else
		{
			fprintf(wpas_wps_stas, "%s", "PBC Status: Disabled\n");
			fprintf(wpas_wps_stas, "%s", "Last WPS result: None\n");
		}

		fprintf(wpas_wps_stas, "%s %s\n", "Interface:", ifname);
		fclose(wpas_wps_stas);
	}
#endif //WLAN_CLIENT || WLAN_UNIVERSAL_REPEATER
}
#endif

int rtk_wlan_get_mib_index_mapping_by_ifname(const char *ifname, int *wlan_index, int *vwlan_idx)
{
	int i=0;
	for(i=0; i<NUM_WLAN_INTERFACE ; i++)
	{
		if(!strncmp(ifname, WLANIF[i], strlen(WLANIF[i])))
			break;
	}
	if(i>=NUM_WLAN_INTERFACE)
		return -1;

	*wlan_index = i;

	if(NULL != strstr(ifname, "vap"))
		*vwlan_idx = (ifname[9] - '0') + WLAN_VAP_ITF_INDEX;
	else if(NULL != strstr(ifname, "vxd"))
		*vwlan_idx = WLAN_REPEATER_ITF_INDEX;
	else
		*vwlan_idx = WLAN_ROOT_ITF_INDEX;

	return 0;
}

static char *get_wlan_proc_token_with_sep(char *data, char *token, char *separator)
{
	char *ptr=NULL, *data_ptr=data;
	int len=0;

	/* skip leading space */
	while(*data_ptr && *data_ptr == ' '){
		data_ptr++;
	}

	ptr = data_ptr;
	while (*ptr && *ptr != '\n' ) {
		if (*ptr == *separator) {
			if (len <= 1)
				return NULL;
			memcpy(token, data_ptr, len);

			token[len] = '\0';

			return ptr+1;
		}
		len++;
		ptr++;
	}
	return NULL;
}

static int get_wlan_proc_value(char *data, char *value)
{
	char *ptr=data;
	int len=0, idx=0;

	while (*ptr && *ptr != '\n' && *ptr != '\r'&& *ptr != '(') {
		len++;
		ptr++;
	}

	/* delete leading space */
	idx = 0;
	while (len-idx > 0) {
		if (data[idx] != ' ')
			break;
		idx++;
	}
	len -= idx;

	if (len > 0) {
		memcpy(value, &data[idx], len);
		value[len] = '\0';
	}
	return len;
}

int getWlIdlePercentInfo(char *interface, unsigned int *pInfo)
{
	int ret = -1;
	FILE *fp = NULL;
	char wlan_path[128] = {0}, line[1024] = {0};
	char token[512]={0}, value[512]={0};
	char *ptr = NULL;
	snprintf(wlan_path, sizeof(wlan_path), PROC_WLAN_DIR_NAME"/%s/ap_info" , interface);

	fp = fopen(wlan_path, "r");
	if(fp){
		while (fgets(line, sizeof(line),fp)) {
			ptr = get_wlan_proc_token_with_sep(line, token, "=");
			if (ptr == NULL)
				continue;
			if (get_wlan_proc_value(ptr, value)==0){
				continue;
			}
			//fprintf(stderr, "%s::%s:%s\n", __func__,token, value);
			if (!strcmp(token, "idle_free_time")) {
				if(sscanf(value,"%u", pInfo)){
					ret = 0;
					//printf("%s idle_free_time %u\n", interface, *pInfo);
					break;
				}
			}
		}
		fclose(fp);
	}
	return ret;
}

#ifdef BAND_STEERING_SUPPORT
int rtk_wlan_get_band_steering_status(void)
{
	int i = 0, check_wpa_cipher = 0;
	int orig_wlan_idx = wlan_idx;
	MIB_CE_MBSSIB_T wlan_entry[NUM_WLAN_INTERFACE];

	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		wlan_getEntry(&wlan_entry[i], 0);
	}
	wlan_idx = orig_wlan_idx;

	if(!strcmp(wlan_entry[0].ssid, wlan_entry[1].ssid))
	{
		if(wlan_entry[0].encrypt == wlan_entry[1].encrypt)
		{
			if(wlan_entry[0].encrypt == WIFI_SEC_NONE)
			{
				if(wlan_entry[0].enable1X == wlan_entry[1].enable1X)
					return 1;
			}
			else if(wlan_entry[0].encrypt == WIFI_SEC_WEP)
			{
				if(wlan_entry[0].enable1X == wlan_entry[1].enable1X)
				{
					if(wlan_entry[0].enable1X == 0)
					{
						if((wlan_entry[0].wep == wlan_entry[1].wep) && (wlan_entry[0].authType == wlan_entry[1].authType))
						{
							if(wlan_entry[0].wep == WEP64)
							{
								if(!strncmp(wlan_entry[0].wep64Key1, wlan_entry[1].wep64Key1, WEP64_KEY_LEN))
									return 1;
							}
							else if(wlan_entry[0].wep == WEP128)
							{
								if(!strncmp(wlan_entry[0].wep128Key1, wlan_entry[1].wep128Key1, WEP128_KEY_LEN))
									return 1;
							}
						}
					}
					else
					{
						if(wlan_entry[0].wep == wlan_entry[1].wep)
							return 1;
					}
				}
			}
			else if(wlan_entry[0].encrypt >= WIFI_SEC_WPA)
			{
				switch(wlan_entry[0].encrypt)
				{
					case WIFI_SEC_WPA:
						if(wlan_entry[0].unicastCipher == wlan_entry[1].unicastCipher)
							check_wpa_cipher = 1;
						break;
					case WIFI_SEC_WPA2:
						if((wlan_entry[0].wpa2UnicastCipher == wlan_entry[1].wpa2UnicastCipher)
#ifdef WLAN_11W
						&& (wlan_entry[0].dotIEEE80211W == wlan_entry[1].dotIEEE80211W)
#endif
						)
						{
#ifdef WLAN_11W

							if((wlan_entry[0].dotIEEE80211W == 1) && (wlan_entry[0].sha256 == wlan_entry[1].sha256)) //1: MGMT_FRAME_PROTECTION_OPTIONAL
								check_wpa_cipher = 1;
							else if(wlan_entry[0].dotIEEE80211W != 1)
#endif
								check_wpa_cipher = 1;
						}
						break;
					case WIFI_SEC_WPA2_MIXED:
						if((wlan_entry[0].unicastCipher == wlan_entry[1].unicastCipher) &&
							(wlan_entry[0].wpa2UnicastCipher == wlan_entry[1].wpa2UnicastCipher))
							check_wpa_cipher = 1;
						break;
#ifdef WLAN_WPA3
					case WIFI_SEC_WPA3:
					case WIFI_SEC_WPA2_WPA3_MIXED:
						check_wpa_cipher = 1;
						break;
#endif
					default:
						break;
				}
				if(wlan_entry[0].wpaAuth == WPA_AUTH_PSK)
				{
					if(check_wpa_cipher && (!strncmp(wlan_entry[0].wpaPSK, wlan_entry[1].wpaPSK, MAX_PSK_LEN+1)))
						return 1;
				}
				else
					if(check_wpa_cipher)
						return 1;

			}
		}
	}

	return 0;
}

/*
 * call when two SSIDs or Encryption change, also before mib update & config_WLAN
 */
int rtk_wlan_update_band_steering_status(void)
{
	unsigned char sta_control = 0;
	if(rtk_wlan_get_band_steering_status() == 0)
	{
		mib_get_s(MIB_WIFI_STA_CONTROL_ENABLE, &sta_control, sizeof(sta_control));
		if(sta_control == 1){
			sta_control = 0;
			mib_set(MIB_WIFI_STA_CONTROL_ENABLE, &sta_control);
			syslog(LOG_ALERT, "WLAN: Disable Band Steering");
			return 1;
		}
	}
	return 0;
}
#endif

int check_wlan_interface(void)
{
#if !defined(CONFIG_RTK_DEV_AP)
	int i=0, j=0, vInt=0;
	char proc_name[256]={0}, var_name[256]={0};
	DIR* dir;
	unsigned char mapping=0, band=0;
	MIB_CE_MBSSIB_T Entry;
	for(i=0; i<NUM_WLAN_INTERFACE;i++){
		snprintf(proc_name, sizeof(proc_name), PROC_WLAN_DIR_NAME"/%s", WLANIF[i]);
		dir = opendir(proc_name);
		if(dir){
			closedir(dir);
			snprintf(var_name, sizeof(var_name), "/var/%s", WLANIF[i]);
			va_cmd("/bin/ln", 3, 1, "-snf", proc_name, var_name);
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_ROOT_ITF_INDEX, (void *)&Entry);
			if(Entry.is_ax_support != 1){
				for(j=0; j<=NUM_VWLAN_INTERFACE;j++){
					mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&Entry);
					Entry.is_ax_support = 1;
					mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, j);
				}
#ifdef WIFI5_WIFI6_RUNTIME
#ifdef CONFIG_RTK_L34_FC_IPI_WIFI_RX
				vInt = -2;
				mib_local_mapping_set(MIB_WLAN_SMP_DISPATCH_RX, i, (void *)&vInt);
#endif
#endif
#ifdef COMMIT_IMMEDIATELY
				Commit();
#endif
			}
			continue;
		}
		snprintf(proc_name, sizeof(proc_name), "/proc/%s", WLANIF[i]);
		dir = opendir(proc_name);
		if(dir){
			closedir(dir);
			snprintf(var_name, sizeof(var_name), "/var/%s", WLANIF[i]);
			va_cmd("/bin/ln", 3, 1, "-snf", proc_name, var_name);
#ifdef WLAN_BAND_CONFIG_MBSSID
			mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_ROOT_ITF_INDEX, (void *)&Entry);
			band = Entry.wlanBand;
#else
			mib_local_mapping_get(MIB_WLAN_BAND, i, (void *)&band);
#endif
			if(band & BAND_11AX){
#ifdef WIFI5_WIFI6_RUNTIME
#ifdef WLAN_USE_DEFAULT_SSID_PSK
				mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, WLAN_ROOT_ITF_INDEX, (void *)&Entry);
				Entry.encrypt = WIFI_SEC_WPA2_MIXED;
				mib_local_mapping_set(MIB_WLAN_ENCRYPT, i, (void *)&Entry.encrypt);
#ifdef CONFIG_CMCC
				Entry.unicastCipher = 3;
#else
				Entry.unicastCipher = 1;
#endif
				mib_local_mapping_set(MIB_WLAN_WPA_CIPHER_SUITE, i, (void *)&Entry.unicastCipher);
#ifdef CONFIG_CMCC
				Entry.wpa2UnicastCipher = 3;
#else
				Entry.wpa2UnicastCipher = 2;
#endif
				mib_local_mapping_set(MIB_WLAN_WPA2_CIPHER_SUITE, i, (void *)&Entry.wpa2UnicastCipher);
#ifdef WLAN_11W
				Entry.dotIEEE80211W = 0;
				mib_local_mapping_set(MIB_WLAN_DOTIEEE80211W, i, (void *)&Entry.dotIEEE80211W);
				Entry.sha256 = 0;
				mib_local_mapping_set(MIB_WLAN_SHA256, i, (void *)&Entry.sha256);
#endif
				mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, WLAN_ROOT_ITF_INDEX);
#endif
#endif
				band &= ~(BAND_11AX);
				mib_local_mapping_set(MIB_WLAN_BAND, i, (void *)&band);
				for(j=0; j<=NUM_VWLAN_INTERFACE;j++){
					mib_chain_local_mapping_get(MIB_MBSSIB_TBL, i, j, (void *)&Entry);
					if(Entry.is_ax_support != 0)
						Entry.is_ax_support = 0;
#ifdef WLAN_BAND_CONFIG_MBSSID
					if(Entry.wlanBand & BAND_11AX)
						Entry.wlanBand &= ~(BAND_11AX);
#endif
					mib_chain_local_mapping_update(MIB_MBSSIB_TBL, i, (void *)&Entry, j);
				}
#ifdef COMMIT_IMMEDIATELY
				Commit();
#endif
			}
		}
	}
#endif
	return 0;
}


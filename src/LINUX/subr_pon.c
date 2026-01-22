#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mib.h"
#include "utility.h"
#include <sys/types.h>
#include <signal.h>

#ifdef EMBED
#include <linux/config.h>
#include <config/autoconf.h>
#else
#include "../../../../include/linux/autoconf.h"
#include "../../../../config/autoconf.h"
#endif

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
#include "rtk/ponmac.h"
#include <module/gpon/gpon.h>
#include "rtk/epon.h"
#include "rtk/pon_led.h"
#include "rtk/stat.h"
#include "rtk/switch.h"
#if defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_gpon.h>
#include <rtk/rt/rt_epon.h>
#endif

#ifdef CONFIG_TR142_MODULE
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <rtk/rtk_tr142.h>
#endif
#ifdef CONFIG_RTK_OMCI_V1
#include <omci_dm_sd.h>
#endif
#endif

#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#include "fc_api.h"
#endif


#ifdef CONFIG_GPON_FEATURE
const char *OMCI_SW_VER_DEF[] = {"V1.7.1", "V1R007C00S001", "V1.7.1", "V1.7.1"};
const char OMCC_VER_DEF[] = {0x80,0x86,0xA0,0x80};
const char OMCI_TM_OPT_DEF[] = {2,2,2,2};
const char *OMCI_EQID_DEF[] = {"RTL9601B", "RTL9601B", "RTL9601B", "RTL9601B"};
const char *OMCI_ONT_VER_DEF[] = {"R1", "R1", "R1", "R1"};
const char *OMCI_VENDOR_ID_DEF[] = {"RTKG", "HWTC", "ZTEG", "RTKG"};

const char OMCICLI[] = "/bin/omcicli";

#if 0
void getOMCI_EQID(char *buf)
{
	unsigned int chipId, rev, subType;
	rtk_switch_version_get(&chipId, &rev, &subType);
	switch(chipId)
	{			
		case RTL9601B_CHIP_ID:
			snprintf(buf, 10, "RTL9601B");
			break;
		case RTL9602C_CHIP_ID:
			snprintf(buf, 10, "RTL9602C");
			break;
		case RTL9607C_CHIP_ID:
			snprintf(buf, 10, "RTL9607C");
			break;
		case APOLLOMP_CHIP_ID:
		default:
			snprintf(buf, 10, "RTL%04X", chipId>>16);
			break;
	}
}
#endif

int checkOMCImib(char mode)
{
	char buf[100], newline[256], buf2[20];
	FILE *fp;
	char *cmd;
	
	int index, i;
	char *delim = " ";
	char *pch;
	char vChar;
	int startup=0;

	if(!mib_get_s(MIB_OMCI_OLT_MODE, &vChar, sizeof(vChar)))
		return 0;

	if(mode==-1){ //startup checking
		startup = 1;
		mode = vChar;
	}
	else if(mode == vChar) //if mode is not changed, do not update omci mib
		return 0;

	if(mode==0){
		fp = popen("nv getenv sw_version0", "r");
		if(fp){
			if(fgets(newline, 256, fp) != NULL){
				sscanf(newline, "%*[^=]=%s", buf);
				//printf("sw version 1 %s\n", buf);
				if(!mib_set(MIB_OMCI_SW_VER1, buf))
					printf("set omci sw ver 1 failed\n");
			}
			pclose(fp);
		}

		newline[0]='\0';
		buf[0]='\0';
		
		fp = popen("nv getenv sw_version1", "r");
		if(fp){
			if(fgets(newline, 256, fp) != NULL){
				sscanf(newline, "%*[^=]=%s", buf);
				//printf("sw version 2 %s\n", buf);
				if(!mib_set(MIB_OMCI_SW_VER2, buf))
					printf("set omci sw ver 2 failed\n");
			}
			pclose(fp);
		}
/*
		buf[0]='\0';
		getOMCI_EQID(buf);
		if(!mib_set(MIB_HW_CWMP_PRODUCTCLASS, buf))
			printf("set hw cwmp product class failed\n");

		newline[0]='\0';
		buf[0]='\0';
		fp = popen("dbg_tool get info", "r");
		if(fgets(newline, 256, fp) != NULL){
			sscanf(newline, "%s %*s", buf);
			index = atoi(buf);
			//printf("index %d\n", index);
			for(i=0;i<3; i++)
				fgets(newline, 256, fp);

			//printf ("Splitting string \"%s\" into tokens:\n",str);
			pch = strtok(newline,delim);
			i = 0;
			while (pch != NULL)
			{
				if(index==i) break;
				//printf ("%s\n",pch);
				pch = strtok (NULL, delim);
				i++;
			} 
			//printf ("%s\n",pch);
			snprintf(buf, 100, "R%d", atoi(pch));
			if(!mib_set(MIB_HW_HWVER, buf))
				printf("set hw version failed\n");
		}
		pclose(fp);
*/
		if(!startup){
			vChar = 0x80;
			if(!mib_set(MIB_OMCC_VER, &vChar))
				printf("set omcc version failed\n");
			vChar = 2;
			if(!mib_set(MIB_OMCI_TM_OPT, &vChar))
				printf("set omci tm opt failed\n");
		}
		

	}
	else{
		if(!mib_set(MIB_OMCI_SW_VER1, (void *)OMCI_SW_VER_DEF[mode]))
			printf("set omci sw ver 1 failed\n");
		if(!mib_set(MIB_OMCI_SW_VER2, (void *)OMCI_SW_VER_DEF[mode]))
			printf("set omci sw ver 2 failed\n");
		//if(!mib_set(MIB_OMCI_EQID, OMCI_EQID_DEF[mode]))
		//	printf("set omci eqid failed\n");
/*
		getOMCI_EQID(buf);
		if(!mib_set(MIB_HW_CWMP_PRODUCTCLASS, buf))
			printf("set cwmp product class failed\n");
		
		if(!mib_set(MIB_HW_HWVER, OMCI_ONT_VER_DEF[mode]))
			printf("set hw version failed\n");
*/		
		if(!startup){
			vChar = OMCC_VER_DEF[mode];
			if(!mib_set(MIB_OMCC_VER, &vChar))
				printf("set omcc version failed\n");
			vChar = OMCI_TM_OPT_DEF[mode];
			if(!mib_set(MIB_OMCI_TM_OPT, &vChar))
				printf("set omci tm opt failed\n");
		}
		
	}
#if 0
	if(mode==0 || mode==3){
		buf[0]='\0';
		mib_get_s(MIB_GPON_SN, buf, sizeof(buf));
		strncpy(buf2, buf, 4);
		buf2[5]="\0";
		if(!mib_set(MIB_PON_VENDOR_ID, buf2))
			printf("set omci vendor id failed\n");
		
	}
	else{
		if(!mib_set(MIB_PON_VENDOR_ID, OMCI_VENDOR_ID_DEF[mode]))
			printf("set omci vendor id failed\n");
	}
#endif
	return 1;
}
void checkOMCI_startup(void)
{
	char mode;
	int ret;
	
	if(!mib_get_s(MIB_OMCI_OLT_MODE, &mode, sizeof(mode)))
		return;

	if(mode==0){
		ret = checkOMCImib(-1);

#ifdef COMMIT_IMMEDIATELY
		if(ret){
			Commit();
			mib_update(HW_SETTING, CONFIG_MIB_ALL);
		}
#endif
	}

}

void restartOMCI(void)
{
	unsigned int pon_mode=0;
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
	if(pon_mode == GPON_MODE)
	{
		va_cmd(OMCICLI, 2, 1, "debug", "reactive");
	}
}

#ifdef CONFIG_COMMON_RT_API
int rtgponstatus2int(int rt_onu_state)
{
	switch (rt_onu_state)
	{
		case RT_GPON_ONU_STATES_O1:
		return 1;
		case RT_GPON_ONU_STATES_O2:
		return 2;
		case RT_GPON_ONU_STATES_O3:
		return 3;
		case RT_GPON_ONU_STATES_O4:
		return 4;
		case RT_GPON_ONU_STATES_O5:
		return 5;
		case RT_GPON_ONU_STATES_O6:
		return 6;
		case RT_GPON_ONU_STATES_O7:
		return 7;
		default: //something wrong
		return 8;
	}
}
#endif

int rtk_wrapper_gpon_deactivate(void)
{
#ifdef CONFIG_COMMON_RT_API
	return rt_gpon_deactivate();
#else
	return rtk_gpon_deActivate();
#endif
}

int rtk_wrapper_gpon_password_set(char *password)
{
#ifdef CONFIG_COMMON_RT_API
		rt_gpon_registrationId_t gpon_ploam_pwd;

		memset(&gpon_ploam_pwd,0, sizeof(gpon_ploam_pwd));
		if(strlen(password) > 0)
			memcpy(gpon_ploam_pwd.regId, password, strlen(password));
		
		return rt_gpon_registrationId_set(&gpon_ploam_pwd);
#else
		rtk_gpon_password_t gpon_ploam_pwd;

		memset(&gpon_ploam_pwd,0, sizeof(gpon_ploam_pwd));
		if(strlen(password) > 0)
			memcpy(gpon_ploam_pwd.password, password, strlen(password));
		
		return rtk_gpon_password_set(&gpon_ploam_pwd);
#endif
}

int rtk_wrapper_gpon_activate(void)
{
#ifdef CONFIG_COMMON_RT_API
		return rt_gpon_activate(RT_GPON_ONU_INIT_STATE_O1);
#else
		return rtk_gpon_activate(RTK_GPONMAC_INIT_STATE_O1);
#endif
}

int getGponONUState(void)
{
#ifdef CONFIG_COMMON_RT_API
	rt_gpon_onuState_t onu;

	rt_gpon_onuState_get(&onu);
	onu = rtgponstatus2int(onu);

	return onu;
#else
	rtk_gpon_fsm_status_t onu;

	rtk_gpon_ponStatus_get(&onu);
	if(onu <= GPON_STATE_O5)
		return onu;//O1, O2, O3, O4, O5
	else
		return 0;
#endif 	
}

#ifdef CONFIG_USER_CUMANAGEDEAMON
void show_gpon_status(char * xString)
{
	unsigned int pon_mode;
	int onu;
	int nBytesSent = 0;

	if (mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode)) != 0) {
		if (pon_mode != GPON_MODE) {
			fprintf(stderr, "not GPON_MODE Error!");
			return;
		}
	} else {
		fprintf(stderr, "mib_get_s Error!");
		return;
	}

	onu = getGponONUState();
	if(onu == 5)
		//nBytesSent += boaWrite(wp, "å¨çº¿");
		sprintf(xString,"PON_STATUS_REG_AUTH");
	else
		if(onu == 2 || onu ==3 || onu == 4)
			//nBytesSent += boaWrite(wp, "ç¦»çº¿ä¸?);
			sprintf(xString,"PON_STATUS_REG_NO_AUTH");
		else
			//nBytesSent += boaWrite(wp, "è¿çº¿");
			sprintf(xString,"PON_STATUS_NO_REG_NO_AUTH");
}
#endif
#endif

#ifdef CONFIG_EPON_FEATURE
int epon_getAuthState(int llid)
{
    FILE *fp;
    char cmdStr[256], line[256];
    char tmpStr[20];
    int ret=2;

    snprintf(cmdStr, 256, "oamcli get ctc auth %d > /tmp/auth.result 2>&1", llid);
    va_cmd("/bin/sh", 2, 1, "-c", cmdStr);

    fp = fopen("/tmp/auth.result", "r");
    if (fp)
    {
        fgets(line, sizeof(line), fp);
        sscanf(line, "%*[^:]: %s", tmpStr);
        if (!strcmp(tmpStr, "success"))
            ret = 1;
        else if (!strcmp(tmpStr, "fail"))
            ret = 0;

        fclose(fp);
    }
    return (ret);
}

int rtk_pon_getEponllidEntryNum(void)
{
	int entryNum=0;

#ifdef CONFIG_COMMON_RT_API
	entryNum = 1;
#else
	rtk_epon_llidEntryNum_get(&entryNum);
#endif

	return entryNum;
}

int getEponONUState(unsigned int llidx)
{
	int ret;
#ifdef CONFIG_COMMON_RT_API
	rt_epon_llid_entry_t llidEntry;
	rt_epon_counter_t counter;
#else
	rtk_epon_llid_entry_t  llidEntry;
	rtk_epon_counter_t counter;
#endif

	memset(&llidEntry, 0, sizeof(rtk_epon_llid_entry_t));
	llidEntry.llidIdx = llidx;
#ifdef CONFIG_COMMON_RT_API
	if(rt_epon_llid_entry_get(&llidEntry) != RT_ERR_OK)
		printf("rt_epon_llid_entry_get failed: %s %d\n", __func__, __LINE__);
#else
	rtk_epon_llid_entry_get(&llidEntry);
#endif

	if(llidEntry.valid)//OLT Register successful
		return 5;

	memset(&counter, 0, sizeof(rtk_epon_counter_t));
	counter.llidIdx = llidx;
#ifdef CONFIG_COMMON_RT_API
	ret = rt_epon_mibCounter_get(&counter);
#else
	ret = rtk_epon_mibCounter_get(&counter);
#endif
	if(ret == RT_ERR_OK)
	{
		if(counter.llidIdxCnt.mpcpRxGate)
			return 4;
		else if(counter.mpcpTxRegRequest)
			return 3;
		else if(counter.mpcpRxDiscGate)
			return 2;
		else
			return 1;
	}
	return 0;
}

long epon_getctcAuthSuccTime(int llid)
{
    FILE *fp;
    char cmdStr[256], line[256];
   	unsigned long authSuccTime=0;
    int ret=2;

    snprintf(cmdStr, 256, "oamcli get ctc succAuthTime %d", llid);

    fp = popen(cmdStr, "r");
    if (fp)
    {
        fgets(line, sizeof(line), fp);
        sscanf(line, "%*[^:]: %ld", &authSuccTime);

        pclose(fp);
    }
    return (authSuccTime);
}
	
int config_oam_vlancfg(void)
{
	char cfg_type, manu_mode, manu_pri;
	unsigned short manu_vid;
	char cmd[128];
	int status = -1;
	int i;
	
	if(!mib_get_s(MIB_VLAN_CFG_TYPE, (void *)&cfg_type, sizeof(cfg_type)))
	{
		printf("MIB_VLAN_CFG_TYPE get failed!\n");
		return status;
	}

	if(!mib_get_s(MIB_VLAN_MANU_MODE, (void *)&manu_mode, sizeof(manu_mode)))
	{
		printf("MIB_VLAN_MANU_MODE get failed!\n");
		return status;
	}
	
	if(cfg_type == VLAN_AUTO)
	{
		snprintf(cmd, sizeof(cmd), "oamcli set ctc statusVlan 0");
		system(cmd);
		printf("%s\n",cmd);
	}
	else
	{
		snprintf(cmd, sizeof(cmd), "oamcli set ctc statusVlan 1");
		system(cmd);
		printf("%s\n",cmd);
	}
	
	if((cfg_type == VLAN_MANUAL) && (manu_mode == VLAN_MANU_TAG))
	{
		if(!mib_get_s(MIB_VLAN_MANU_TAG_VID, (void *)&manu_vid, sizeof(manu_vid)))
		{
			printf("MIB_VLAN_MANU_TAG_VID get failed!\n");
			return status;
		}
		if(!mib_get_s(MIB_VLAN_MANU_TAG_PRI, (void *)&manu_pri, sizeof(manu_pri)))
		{
			printf("MIB_VLAN_MANU_TAG_PRI get failed!\n");
			return status;
		}

		for(i=0; i<ELANVIF_NUM; i++)
		{
			snprintf(cmd, sizeof(cmd), "oamcli set ctc vlan %d 1 %d/%d", i, manu_vid, manu_pri);
			status = system(cmd);
			printf("%s\n",cmd);
		}
	}
	else
	{
		for(i=0; i<ELANVIF_NUM; i++)
		{
			snprintf(cmd, sizeof(cmd), "oamcli set ctc vlan %d 0",i);
			status = system(cmd);
			printf("%s\n",cmd);
		}
	}

	return status;
}
void rtk_pon_sync_epon_llid_entry(void)
{
	unsigned int totalEntry;
	int entryNum=4;
	int index;
	int retVal;
	int i;
	MIB_CE_MIB_EPON_LLID_T mib_llidEntry;

#ifdef CONFIG_COMMON_RT_API
	if(rtk_epon_llidEntryNum_get(&entryNum) != RT_ERR_OK)
		printf("rtk_epon_llidEntryNum_get failed: %s %d\n", __func__, __LINE__);
#endif

	totalEntry = mib_chain_total(MIB_EPON_LLID_TBL); /* get chain record size */
	if(totalEntry == 0)
	{
		//First time to boot, the mib chain of LLID MAC table is empty.
		//need to create entry according to the LLID numbers chip supported.
		unsigned char mac[MAC_ADDR_LEN]={0x00,0x11,0x22,0x33,0x44,0x55};
        mib_get_s(MIB_ELAN_MAC_ADDR, (void *)mac, sizeof(mac));

		printf("First time to create EPON LLID MIB Table, now create %d entries.\n",entryNum);

		for(index=0;index<entryNum;index++)
		{
			memset(&mib_llidEntry,0,sizeof(mib_llidEntry));

			//Add new EPON_LLID_ENTRY into mib chain
			memcpy(mib_llidEntry.macAddr,mac, MAC_ADDR_LEN);
			retVal = mib_chain_add(MIB_EPON_LLID_TBL, (unsigned char*)&mib_llidEntry);
			if (retVal == 0) {
				printf("Error!!!!!! %s:%s\n",__func__,"Error! Add chain record.");
				return;
			}
			else if (retVal == -1) {
				printf("Error!!!!!! %s:%s\n",__func__,"Error! Table Full.");
				return;
			}
			printf("Add EPON LLID default entry into MIB Table with mac %2x:%2x:%2x:%2x:%2x:%2x Success!\n",
					mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

			setup_mac_addr(mac, 1);
		}

#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif

	}
}
#ifdef CONFIG_USER_CUMANAGEDEAMON
void show_epon_status(char * xString)
{
	unsigned int pon_mode;
	int eonu = 0;

	if (mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode)) != 0) {
		if (pon_mode != EPON_MODE) {
			fprintf(stderr, "not EPON_MODE Error!");
			return;
		}
	} else {
		fprintf(stderr, "mib_get_s Error!");
		return;
	}

	eonu = getEponONUState(0);
    if (eonu == 5)
    {
        int ret;

        ret = epon_getAuthState(0);
        if (1 == ret)
            //nBytesSent += boaWrite(wp, "å·²æ³¨åå·²è®¤è¯Ö¤");
            sprintf(xString,"PON_STATUS_REG_AUTH");
        else
            //nBytesSent += boaWrite(wp, "å·²æ³¨åæªè®¤è¯");
            sprintf(xString,"PON_STATUS_REG_NO_AUTH");
    }
    else
    {
        //nBytesSent += boaWrite(wp, "æªæ³¨åæªè®¤è¯");
        sprintf(xString,"PON_STATUS_NO_REG_NO_AUTH");
    }

}
#endif
#endif
#if defined(CONFIG_CMCC) && defined(CONFIG_SUPPORT_ADAPT_DPI_V3_3_0)
void get_poninfo_eqd(uint32 *eqd){
	if(eqd == NULL) return;
	if(rtk_gpon_eqd_get(eqd) != RT_ERR_OK)
		printf("get_poninfo_eqd fail!!\n");
}
void get_wan_error(unsigned long *rx_error, char *rx_error_rate){
	unsigned long rx_errs;
	FILE *fp;
	int lock_fd;
	char  buf[256];
	
	lock_fd = lock_file_by_flock(WANSE_FILE_LOCK, 1);
	fp = fopen(WANSE_FILE,"r");
	if(fp){
		fgets(buf,256, fp);
		sscanf(buf,"%d	%s ", rx_error, rx_error_rate);
		fclose(fp);
	}else{
		*rx_error = 0;
		snprintf(rx_error_rate,16,"0.0000%%");
	}
	unlock_file_by_flock(lock_fd);

	return;
}

#endif

#if defined(CONFIG_EPON_FEATURE)||defined(CONFIG_GPON_FEATURE)
//cxy 2016-8-22: led_mode: 0:led off   1:led on 2:led state restore
int set_pon_led_state(int led_mode)
{
	unsigned int pon_mode=0;
	rtk_pon_led_pon_mode_t led_pon_mode=PON_LED_PON_MODE_END;

	mib_get_s(MIB_PON_MODE,&pon_mode, sizeof(pon_mode));
	if(pon_mode==EPON_MODE)
		led_pon_mode=PON_LED_PON_MODE_EPON;
	else if(pon_mode==GPON_MODE)
		led_pon_mode=PON_LED_PON_MODE_GPON;
	else
		return -1;

	switch(led_mode)
	{
		case 0://pon led state led off
			if(rtk_pon_led_status_set(led_pon_mode, PON_LED_FORCE_OFF) != RT_ERR_OK)
				printf("rtk_pon_led_status_set failed: %s %d\n", __func__, __LINE__);
			break;
		case 1://pon led state led on
			if(rtk_pon_led_status_set(led_pon_mode, PON_LED_FORCE_ON) != RT_ERR_OK)
				printf("rtk_pon_led_status_set failed: %s %d\n", __func__, __LINE__);
			break;
		case 2://lpon led state restore
			if(rtk_pon_led_status_set(led_pon_mode, PON_LED_STATE_RESTORE) != RT_ERR_OK)
				printf("rtk_pon_led_status_set failed: %s %d\n", __func__, __LINE__);
			break;
		default:
			break;
	}

	return 0;
}

//cxy 2016-8-29:control los led. led_mode: 0:led off   1:led on 2:led state restore
int set_los_led_state(int led_mode)
{
	switch(led_mode)
	{
		case 0:
			if(rtk_pon_led_status_set(PON_LED_PON_MODE_OTHERS, PON_LED_FORCE_OFF) != RT_ERR_OK)//LOS led off
				printf("rtk_pon_led_status_set failed: %s %d\n", __func__, __LINE__);
			break;
		case 1:
			if(rtk_pon_led_status_set(PON_LED_PON_MODE_OTHERS, PON_LED_FORCE_ON) != RT_ERR_OK)
				printf("rtk_pon_led_status_set failed: %s %d\n", __func__, __LINE__);
			break;
		case 2:
			if(rtk_pon_led_status_set(PON_LED_PON_MODE_OTHERS, PON_LED_STATE_RESTORE) != RT_ERR_OK)
				printf("rtk_pon_led_status_set failed: %s %d\n", __func__, __LINE__);
			break;
		default:
			break;
	}
	return 0;
}

#ifdef CONFIG_E8B
void get_poninfo(int s,double *buffer)
{
    rtk_transceiver_data_t transceiver, readableCfg;
    if(s == 1)
    {
        memset(&transceiver, 0, sizeof(transceiver));
        memset(&readableCfg, 0, sizeof(readableCfg));
        rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_TEMPERATURE, &transceiver);

        _get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_TEMPERATURE, &transceiver, &readableCfg);
        strtok(readableCfg.buf, " ");
        *buffer = atof(readableCfg.buf);
    }
    else if(s == 2)
    {
        memset(&transceiver, 0, sizeof(transceiver));
        memset(&readableCfg, 0, sizeof(readableCfg));
		rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_VOLTAGE,&transceiver);

		_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_VOLTAGE, &transceiver, &readableCfg);
				strtok(readableCfg.buf, " ");
        *buffer = atof(readableCfg.buf);
    }
    else if(s == 3)
    {
        memset(&transceiver, 0, sizeof(transceiver));
        memset(&readableCfg, 0, sizeof(readableCfg));
		rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_BIAS_CURRENT, &transceiver);

		_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_BIAS_CURRENT, &transceiver, &readableCfg);
				strtok(readableCfg.buf, " ");
        *buffer = atof(readableCfg.buf);
    }
    else if(s == 4)
    {
        memset(&transceiver, 0, sizeof(transceiver));
        memset(&readableCfg, 0, sizeof(readableCfg));
		rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_TX_POWER,&transceiver);

		_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_TX_POWER, &transceiver, &readableCfg);
				strtok(readableCfg.buf, " ");
        *buffer = atof(readableCfg.buf);
    }
    else if(s == 5)
    {
        memset(&transceiver, 0, sizeof(transceiver));
        memset(&readableCfg, 0, sizeof(readableCfg));
		rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_RX_POWER, &transceiver);

		_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_RX_POWER, &transceiver, &readableCfg);
				strtok(readableCfg.buf, " ");
        *buffer = atof(readableCfg.buf);
    }
}
#endif

int isDevInPonMode(void)
{
	unsigned int pon_mode;

	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));

	if(pon_mode == GPON_MODE || pon_mode == EPON_MODE)
		return 1;
	else
		return 0;
}

#endif


#ifdef CONFIG_TR142_MODULE
#define TR142_DEV_FILE "/dev/rtk_tr142"
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
/* get_wan_flowid()
 * Description:
 * 		If you want to use this function to get flowid, be careful to know MIB_QOS_ENABLE_QOS
 * 		MIB_QOS_ENABLE_QOS = 0, only get queue 0 flowid.
 */
int get_wan_flowid(void *parg)
{
	int fd;
	struct wanif_flowq wan_flow=*((struct wanif_flowq*)parg);

	fd = open(TR142_DEV_FILE, O_WRONLY);
	if(fd < 0)
	{
		DBPRINT(1, "ERROR: failed to open %s\n", TR142_DEV_FILE);
		return -1;
	}
	if(ioctl(fd, RTK_TR142_IOCTL_GET_WAN_STREAMID, &wan_flow) != 0)
	{
		DBPRINT(1, "ERROR:get wan(%u) streamid failed\n", wan_flow.wanIfidx);
		close(fd);
		return -1;
	}
	close(fd);
	memcpy(parg, (void *)&wan_flow, sizeof(wan_flow));
	return 1;
}
#endif
void set_wan_ponmac_qos_queue_num(void)
{
	int fd;
	unsigned char queue_num=4;
	
	fd = open(TR142_DEV_FILE, O_WRONLY);
	if(fd < 0)
	{
		DBPRINT(1, "ERROR: failed to open %s\n", TR142_DEV_FILE);
		return;
	}
#ifdef CONFIG_RTK_OMCI_V1
	mib_get_s(MIB_OMCI_WAN_QOS_QUEUE_NUM,&queue_num, sizeof(queue_num));
#endif
	if(ioctl(fd, RTK_TR142_IOCTL_SET_WAN_QUEUE_NUM, &queue_num) != 0)
	{
		DBPRINT(1, "ERROR: set PON QoS queues failed\n");
	}
	close(fd);
}

int set_dot1p_value_byCF(void)
{
	int fd;
	unsigned char dot1p_byCF;

	fd = open(TR142_DEV_FILE, O_WRONLY);
	if(fd < 0)
	{
		DBPRINT(1, "ERROR: failed to open %s\n", TR142_DEV_FILE);
		return 0;
	}
	
	if (mib_get_s(MIB_DOT1P_VALUE_BYCF, (void *)&dot1p_byCF, sizeof(dot1p_byCF)) != 0)
	{	
		if(ioctl(fd, RTK_TR142_IOCTL_SET_DOT1P_VALUE_BYCF, &dot1p_byCF) != 0)
		{
			DBPRINT(1, "ERROR: set Dot1p value by CF failed\n");
		}
	}
	close(fd);
	return 0;
}

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
/* 
action: 0 -> delete
        1 -> add
*/
int rtk_tr142_sync_wan_info(MIB_CE_ATM_VC_Tp pEntry, int action)
{
	char cmdStr[256] = {0};
	int omci_mode = (-1), vid = (-1), pri = (-1), mvid = (-1);
	if (pEntry == NULL || (action != 0 && action != 1))
		return -1;

	if (pEntry->vlan)
	{
		vid = (pEntry->vid > 0) ? (pEntry->vid) : (-1);
		pri = (pEntry->vprio > 0) ? (pEntry->vprio - 1) : (-1);
	}
	else
	{
		unsigned char tr247_unmatched_veip_cfg = 0;
		mib_get_s(MIB_TR247_UNMATCHED_VEIP_CFG, (void *)&tr247_unmatched_veip_cfg, sizeof(tr247_unmatched_veip_cfg));
		if((tr247_unmatched_veip_cfg || pEntry->vpass) && pEntry->cmode == CHANNEL_MODE_BRIDGE) 
			vid = -2; //all transparent
		else 
			vid = -1; //untag
	}
	
	if (pEntry->mVid)
	{
		mvid = pEntry->mVid;
	}

	switch (pEntry->cmode)
	{
		case CHANNEL_MODE_IPOE:
#ifdef CONFIG_IPV6
			if ((pEntry->IpProtocol == IPVER_IPV4_IPV6) && (pEntry->napt == 1))
				omci_mode = OMCI_MODE_IPOE_V4NAPT_V6;
			else
#endif
				omci_mode = OMCI_MODE_IPOE;
			//ipoe passthrough mode need to set OMCI_MODE_BRIDGE to avoid filter-unicast-mac
			if(ROUTE_CHECK_BRIDGE_MIX_ENABLE(pEntry))
				omci_mode = OMCI_MODE_BRIDGE;
			break;

		case CHANNEL_MODE_PPPOE:
#ifdef CONFIG_IPV6
			if ((pEntry->IpProtocol == IPVER_IPV4_IPV6) && (pEntry->napt == 1))
				omci_mode = OMCI_MODE_PPPOE_V4NAPT_V6;
			else
#endif
				omci_mode = OMCI_MODE_PPPOE;
			//pppoe passthrough mode need to set OMCI_MODE_BRIDGE to avoid filter-unicast-mac
			if(ROUTE_CHECK_BRIDGE_MIX_ENABLE(pEntry))
				omci_mode = OMCI_MODE_BRIDGE;
			break;

		case CHANNEL_MODE_BRIDGE:
			omci_mode = OMCI_MODE_BRIDGE;
			break;

		default:
			printf("Unknow cmode %d\n", pEntry->cmode);
			break;
	}

	//don't add PPTP bridge WAN to TR142 Wan Info
	if(!(pEntry->applicationtype & X_CT_SRV_PON_PPTP))
	{
		snprintf(cmdStr, sizeof(cmdStr),"echo %u %d %d %d %d %d %d %d > %s", pEntry->ifIndex, action, vid, pri, omci_mode, mvid, pEntry->omci_configured, pEntry->omci_if_id, TR142_WAN_INFO);
		printf("cmdStr: %s\n", cmdStr);
		system(cmdStr);
	}

	return 0;
}
#endif
#endif

#ifdef CONFIG_USER_IP_QOS
void setup_pon_queues(void)
{
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, i, fd;
	unsigned char policy = 0, qosEnable = 0, user_queue_num = 0, expand_queueEnable = 0;
	mib_get_s(MIB_QOS_ENABLE_QOS, (void *)&qosEnable, sizeof(qosEnable));
	mib_get_s(MIB_QOS_POLICY, (void *)&policy, sizeof(policy));
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	unsigned int ip_qos_queue_num = 0;
	mib_get_s(MIB_IP_QOS_QUEUE_NUM, (void *)&ip_qos_queue_num, sizeof(ip_qos_queue_num));
#else
	unsigned char queue_num = MAX_QOS_QUEUE_NUM;
#ifdef CONFIG_RTK_OMCI_V1
	mib_get_s(MIB_OMCI_WAN_QOS_QUEUE_NUM, &queue_num, sizeof(queue_num));
#endif
#endif
#if defined(CONFIG_COMMON_RT_API)
	rt_epon_queueCfg_t	rtEponQueueCfg = {0};
#endif
	unsigned int pon_mode = 0;

	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));

	if(pon_mode == GPON_MODE)
	{
#ifdef CONFIG_TR142_MODULE
		rtk_tr142_qos_queues_t queues = {0};
		
		fd = open(TR142_DEV_FILE, O_WRONLY);
		if(fd < 0)
		{
			DBPRINT(1, "ERROR: failed to open %s\n", TR142_DEV_FILE);
			return;
		}

		if((qEntryNum = mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		{
			DBPRINT(1, "ERROR: set PON QoS queues failed\n");
			close(fd);
			return;
		}

		for(i = 0; i < qEntryNum; i++)
		{
			if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i, (void*)&qEntry))
				continue;

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
			if (i >= ip_qos_queue_num)
				break;

			user_queue_num++;

			if (policy==PLY_PRIO || policy==PLY_CAR)
			{
				queues.queue[i].priority = (i + 1);
			}
			else
				queues.queue[i].priority = 0;
#else
			if(i >= queue_num)
				break;
#endif

			queues.queue[i].enable = qEntry.enable;
			queues.queue[i].type = (policy==PLY_PRIO || policy==PLY_CAR) ? STRICT_PRIORITY : WFQ_WRR_PRIORITY;
			queues.queue[i].weight = (policy==PLY_WRR) ? qEntry.weight : 0;
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
			queues.queue[i].pir = -1;
			if (qosEnable==0)continue;
			if(qEntry.shaping_rate!=0)
#if defined(CONFIG_00R0)
				queues.queue[i].pir = (qEntry.shaping_rate)/(1024*8);
#else
				queues.queue[i].pir = qEntry.shaping_rate;
#endif

			if(policy== 2)
			{
				if(qEntry.car == 0)
				{
					queues.queue[i].cir = 0;
					queues.queue[i].pir = 0;
				}
				else if(qEntry.car == -1)
				{
					queues.queue[i].cir = 0;
				}
				else
				{
					queues.queue[i].cir = qEntry.car/8;
				}
			}
#endif
		}

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
		queues.qos_Enable = qosEnable;
		mib_get_s(MIB_TR142_EXPAND_QUEUE_ENABLE, (void *)&expand_queueEnable, sizeof(expand_queueEnable));
		if(expand_queueEnable == 1)
			queues.qos_user_QueueNum = user_queue_num;
		else
			queues.qos_user_QueueNum = 1;
#endif

		if(ioctl(fd, RTK_TR142_IOCTL_SET_QOS_QUEUES, &queues) != 0)
		{
			DBPRINT(1, "ERROR: set PON QoS queues failed\n");
		}
		close(fd);
#endif
	}
	else if(pon_mode == EPON_MODE)
	{
		if((qEntryNum = mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		{
			DBPRINT(1, "ERROR: set PON QoS queues failed\n");
			return;
		}

		for(i=0 ; i<qEntryNum ; i++)
		{
			if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i, (void*)&qEntry))
				continue;

			if(!qEntry.enable)
				continue;

			if(i >= MAX_QOS_QUEUE_NUM)
				break;

#if defined(CONFIG_COMMON_RT_API)
			if (policy == PLY_PRIO)
			{
				rtEponQueueCfg.cir = 0;
				rtEponQueueCfg.pir = MAX_GPON_RATE;
				rtEponQueueCfg.scheduleType = RT_EPON_LLID_QUEUE_SCHEDULER_SP;
				rtEponQueueCfg.weight = 0;
			}
			else if (policy == PLY_WRR)
			{
				rtEponQueueCfg.cir = 0;
				rtEponQueueCfg.pir = MAX_GPON_RATE;
				rtEponQueueCfg.scheduleType = RT_EPON_LLID_QUEUE_SCHEDULER_WRR;
#if defined(CONFIG_LUNA_G3_SERIES)
				rtEponQueueCfg.weight = qEntry.weight ? qEntry.weight : 1;
#else
				rtEponQueueCfg.weight = qEntry.weight;
#endif
			}
			else if (policy == PLY_CAR)
			{
				if(qEntry.car == 0)
				{
					rtEponQueueCfg.cir = 0;
					rtEponQueueCfg.pir = 0;
				}
				else if(qEntry.car == -1)
				{
					rtEponQueueCfg.cir = 0;
					rtEponQueueCfg.pir = MAX_GPON_RATE;
				}
				else
				{
					rtEponQueueCfg.cir = qEntry.car;
					rtEponQueueCfg.pir = MAX_GPON_RATE;
				}
				rtEponQueueCfg.scheduleType = RT_EPON_LLID_QUEUE_SCHEDULER_SP;
				rtEponQueueCfg.weight = 0;
			}
			AUG_PRT("cir=%d pir=%d scheduleType=%d weight=%d\n", rtEponQueueCfg.cir, rtEponQueueCfg.pir, rtEponQueueCfg.scheduleType, rtEponQueueCfg.weight);
			rt_epon_ponQueue_set(0, (MAX_QOS_QUEUE_NUM-1-i), &rtEponQueueCfg);
#endif
		}
	}
	else if(pon_mode == ETH_MODE)
	{
		rtk_fc_qos_setup_upstream_wrr(policy);
	}
}

void clear_pon_queues(void)
{
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, i, fd;
	unsigned int pon_mode = 0;
#if defined(CONFIG_COMMON_RT_API)
	rt_epon_queueCfg_t	rtEponQueueCfg = {0};
#endif

	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
	if(pon_mode == EPON_MODE)
	{
		if((qEntryNum = mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		{
			DBPRINT(1, "ERROR: set PON QoS queues failed\n");
			return;
		}

		for(i=0 ; i<qEntryNum ; i++)
		{
			if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i, (void*)&qEntry))
				continue;

			if(i >= MAX_QOS_QUEUE_NUM)
				break;
                  
#if defined(CONFIG_COMMON_RT_API)
			rtEponQueueCfg.pir = MAX_GPON_RATE;
			AUG_PRT("cir=%d pir=%d scheduleType=%d weight=%d\n", rtEponQueueCfg.cir, rtEponQueueCfg.pir, rtEponQueueCfg.scheduleType, rtEponQueueCfg.weight);
			rt_epon_ponQueue_set(0, (MAX_QOS_QUEUE_NUM-1-i), &rtEponQueueCfg);
#endif
		}
	}
}
#endif

#if defined(CONFIG_PON_CONFIGURATION_COMPLETE_EVENT)
#include <sys/ipc.h> 
#include <sys/shm.h>
unsigned int get_omci_complete_event_shm( void )
{
	key_t shm_key;
	int shm_id; // shared memory ID
	char *shm_start; // attaching address
#ifdef CONFIG_EPON_FEATURE
	unsigned int pon_mode=0;

	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
	if(pon_mode == EPON_MODE)
	{
		return 1;
	}
#endif

	shm_key = ftok("/proc/omci", 'o');

	if ((shm_id = shmget((key_t)shm_key, 32, 0644 | IPC_CREAT)) == -1) {
		perror("shmget");
		return 0;
	}
	if ((shm_start = (char *)shmat ( shm_id , NULL , 0 ) )==(char *)(-1)) {
		perror("shmat");
		return 0;
	}

	if(shm_start[0] == '\0' || strcmp(shm_start, OMCI_CONFIG_COMPELETE_STR)){
		shmdt(shm_start);
		return 0;
	}
	else{
		shmdt(shm_start);
		return 1;
	}

}
#endif

#ifdef CONFIG_SUPPORT_PON_LINK_DOWN_PROMPT
int getPONLinkState(void)
{
	unsigned int pon_mode;
	int onu;
	rtk_epon_llid_entry_t llid_entry;
	memset(&llid_entry, 0, sizeof(rtk_epon_llid_entry_t));
	llid_entry.llidIdx = 0;
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));

	if(pon_mode == GPON_MODE)
	{
		onu = getGponONUState();
		if(onu >= 2 && onu  <= 5)
		{
			return 1;
		}
	}
	else if(pon_mode == EPON_MODE)
	{
		onu = getEponONUState(0);
		if (onu == 5)
		{
			return 1;
		}
	}

	return 0;
}
#endif

#ifdef CONFIG_USER_DBUS_PROXY
static int DBUS_epon_getAuthState(int llid)
{
	int ret=2;
#if defined(CONFIG_EPON_FEATURE)
    FILE *fp;
    char cmdStr[256], line[256];
    char tmpStr[20]; 

    snprintf(cmdStr, sizeof(cmdStr), "oamcli get ctc auth %d > /tmp/auth.result 2>&1", llid);
    va_cmd("/bin/sh", 2, 1, "-c", cmdStr);

    fp = fopen("/tmp/auth.result", "r");
    if (fp)
    {
        fgets(line, sizeof(line), fp);
        sscanf(line, "%*[^:]: %s", tmpStr);
        if (!strcmp(tmpStr, "success"))
            ret = 1;
        else if (!strcmp(tmpStr, "fail"))
            ret = 0;

        fclose(fp);
    }
#endif
    return (ret);
}

int DBUS_ifponstate(void)
{
    int ret = 0;
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	unsigned int pon_mode;
	int onu;

	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
	if(pon_mode == 1)
	{
	    onu = getGponONUState();

	    if(onu == 5)
	        ret = 1;
	}
	else if(pon_mode == 2)
	{
		onu = getEponONUState(0);
		if (onu == 5)
	    {
			int rett;
			rett = DBUS_epon_getAuthState(0);
			if (1 == rett)
			{
				ret = 1;
			}
			else
				ret = 2;
	    }
	    else
	    {
			ret = 0;
	    }
	}
#endif
	return ret;
}

void DBUS_poninfo(int s,double *buffer)
{
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	get_poninfo(s, buffer);
#endif
}
#endif

void startUserReg(unsigned char *loid, unsigned char *password) //same as formUserReg()
{	
	char *s;
	unsigned char vChar;
	unsigned char enable4StageDiag;
	unsigned int regLimit;
	unsigned int regTimes;
	unsigned int regStatus;
	unsigned int regResult;
	unsigned int lineno;
	pid_t cwmp_pid;
	int num_done;
#if defined(CONFIG_GPON_FEATURE)
	int i=0;
#endif
#if defined(CONFIG_EPON_FEATURE)
	int index = 0;
#endif
	int sleep_time = 3;
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	unsigned int pon_mode;
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
#endif
	unsigned char reg_type;
	unsigned char password_hex[MAX_LOID_PW_LEN]={0};
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	unsigned char gui_passauth_enable = 1;
#endif
#if defined(CONFIG_GPON_FEATURE)
	unsigned int gpon_speed=0;
	int ploam_pw_length=GPON_PLOAM_PASSWORD_LENGTH;
							
	mib_get_s(MIB_PON_SPEED, (void *)&gpon_speed, sizeof(gpon_speed));
	if(gpon_speed==0){
		ploam_pw_length=GPON_PLOAM_PASSWORD_LENGTH;
	}
	else{
		ploam_pw_length=NGPON_PLOAM_PASSWORD_LENGTH;
	}
#endif

	mib_get_s(PROVINCE_DEV_REG_TYPE, &reg_type, sizeof(reg_type));
#ifdef _PRMT_X_CT_COM_USERINFO_
	mib_get_s(CWMP_USERINFO_LIMIT, &regLimit, sizeof(regLimit));
	mib_get_s(CWMP_USERINFO_TIMES, &regTimes, sizeof(regTimes));
	mib_get_s(CWMP_USERINFO_STATUS, &regStatus, sizeof(regStatus));
	mib_get_s(CWMP_USERINFO_LIMIT, &regLimit, sizeof(regLimit));
	mib_get_s(CWMP_USERINFO_TIMES, &regTimes, sizeof(regTimes));
	mib_get_s(CWMP_USERINFO_RESULT, &regResult, sizeof(regResult));
	
	if (regTimes >= regLimit) {
		vChar = CWMP_REG_IDLE;
		mib_set(CWMP_REG_INFORM_STATUS, &vChar);
		goto FINISH;
	}
	if((regStatus == 0 && regResult == 1) || regStatus ==5){
		goto check_err;
	}
#endif

	if (loid[0]) {
		mib_set(MIB_LOID, loid);
		if(reg_type != DEV_REG_TYPE_DEFAULT)
			mib_set(MIB_LOID_OLD,loid);
		#ifdef CONFIG_USER_RTK_ONUCOMM
		//init_onucomm_sock();
		onucomm_pon_loid(loid);
		//close_onucomm_sock();
		#endif
	}
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	else{
		mib_set(MIB_LOID, loid);
		if(reg_type != DEV_REG_TYPE_DEFAULT)
			mib_set(MIB_LOID_OLD,loid);
	}
#else
	else {
		fprintf(stderr, "get LOID error!\n");
		goto check_err;
	}
#endif

	mib_set(MIB_LOID_PASSWD, password);
	if(reg_type != DEV_REG_TYPE_DEFAULT)
		mib_set(MIB_LOID_PASSWD_OLD,password);

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)		
	mib_set(CWMP_GUI_PASSWORD_ENABLE, &gui_passauth_enable);
#endif

#ifdef _PRMT_X_CT_COM_USERINFO_
/*xl_yue:20081225 record the inform status to avoid acs responses twice for only once informing*/
	vChar = CWMP_REG_REQUESTED;
	mib_set(CWMP_REG_INFORM_STATUS, &vChar);
	/* reset to zero */
	num_done = 0;
	mib_set(CWMP_USERINFO_SERV_NUM_DONE, &num_done);
	mib_set(CWMP_USERINFO_SERV_NAME_DONE, "");
/*xl_yue:20081225 END*/
#endif

#if defined(CONFIG_GPON_FEATURE)
	if(pon_mode == 1)
	{
	// Deactive GPON
	// do not use rtk_rg_gpon_deActivate() becuase it does not send link down event.

#ifndef CONFIG_10G_GPON_FEATURE
		system("diag gpon reg-set page 1 offset 0x10 value 0x1");
#endif

		system("omcicli mib reset");

		if (password[0]) {
			va_cmd("/bin/omcicli", 4, 1, "set", "loid", loid, password);
		} else if (loid[0]){
			va_cmd("/bin/omcicli", 3, 1, "set", "loid", loid);
		}

		//because we deactivate the gpon
		//we need CONFIG_CMCC_OSGIMANAGE=y
		system("/bin/diag gpon deactivate");
		if (password[0]) {
			//diag gpon set password xxxx
			formatPloamPasswordToHex(password, password_hex, ploam_pw_length);
			va_cmd("/sbin/diag", 4, 1, "gpon", "set", "password-hex", password_hex);
		}
		else {
			formatPloamPasswordToHex("", password_hex, ploam_pw_length);
			va_cmd("/sbin/diag", 4, 1, "gpon", "set", "password-hex", password_hex);
		}
		system("/bin/diag gpon activate init-state o1");

#ifndef CONFIG_10G_GPON_FEATURE
		while(i++ < 10)
			system("diag gpon reg-set page 1 offset 0x10 value 0x3");
#endif
	}
#endif
#if defined(CONFIG_EPON_FEATURE)
	if(pon_mode == 2)
	{
		/* Martin ZHU add: release all wan connection IP */
		va_cmd("/bin/ethctl", 2, 1, "enable_nas0_wan", "0");

		va_cmd("/bin/ethctl", 2, 1, "enable_nas0_wan", "1");

		/* Martin ZHU: 2016-3-24  */
		mib_get_s(PROVINCE_PONREG_4STAGEDIAG, (void *) &enable4StageDiag, sizeof(enable4StageDiag));
		index = 0;
		{
			if(enable4StageDiag)
			{
				system("diag epon reset mib-counter");
			}

			if (password[0]) {
				va_cmd("/bin/oamcli", 6, 1, "set", "ctc", "loid", "0", loid, password); /* "0" means index = 0 */
			} else {
				va_cmd("/bin/oamcli", 5, 1, "set", "ctc", "loid", "0", loid);
			}

			/* 2016-04-29 siyuan: oam needs to register again using new loid and password */
			va_cmd("/bin/oamcli", 3, 1, "trigger", "register", "0");
		}
	}
#endif
#ifdef _PRMT_X_CT_COM_USERINFO_
	if(reg_type == DEV_REG_TYPE_JSU)
	{
		int result = NOW_SETTING;
		int status = 99;
		mib_set(CWMP_USERINFO_RESULT, &result);
		mib_set(CWMP_USERINFO_STATUS, &status);
		unlink("/var/inform_status");
	}
#endif
	{
		pid_t tr069_pid;

		// send signal to tr069
		tr069_pid = read_pid("/var/run/cwmp.pid");
		if ( tr069_pid > 0)
			kill(tr069_pid, SIGUSR2);
	}

	// Purposes:
	// 1. Wait for PON driver ready.
	// 2. Wait for old IP release.
	//while(sleep_time)
	//	sleep_time = sleep(sleep_time);

FINISH:
#ifdef COMMIT_IMMEDIATELY
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	SaveLOIDReg(); //save LOID and PASSWORD directly, SaveLOIDReg() already has Commit() function
#else
	Commit();
#endif
#endif

check_err:
	return;
}



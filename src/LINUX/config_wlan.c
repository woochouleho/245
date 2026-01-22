#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <signal.h>
#include "utility.h"


static void set_func(int wlanIdx, int disable)
{
	if(disable)
		va_cmd(IWPRIV, 3, 1, WLANIF[wlanIdx], "set_mib", "func_off=1");
	else
		va_cmd(IWPRIV, 3, 1, WLANIF[wlanIdx], "set_mib", "func_off=0");
}

void clear_wlan_guest_duration(int idx)
{
	int duration = 0;
	unsigned long guest_ssid_endtime = 0;
	mib_local_mapping_set(MIB_WLAN_GUEST_SSID_DURATION, idx, (void *)&duration);
	mib_local_mapping_set(MIB_WLAN_GUEST_SSID_ENDTIME, idx, (void *)&guest_ssid_endtime);
}

static void start_wifi(unsigned char start, unsigned int mask)
{
	unsigned char vChar;
	unsigned char wlanDisabled = start? 0:1;
	MIB_CE_MBSSIB_T Entry;
	int vcTotal, i, j;
	//unsigned int wlan_dev_map=0;
	//unsigned int orig_ssid_enable_status, ssid_enable_status=0;
	//MIB_CE_ATM_VC_T vcEntry;
	unsigned char phyband=0;
#if 0
#if defined(WLAN_DUALBAND_CONCURRENT)
	int orig_idx = wlan_idx;
#endif
	int i, j;
	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry);
		if(wlanDisabled != Entry.wlanDisabled){
			Entry.wlanDisabled = wlanDisabled;
			mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, 0);
		}
		if(wlanDisabled==1){
			for(j=0; j<WLAN_MBSSID_NUM; j++){
				mib_chain_get(MIB_MBSSIB_TBL, j+1, (void *)&Entry);
				if(wlanDisabled != Entry.wlanDisabled){
					Entry.wlanDisabled = wlanDisabled;
					mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, j+1);
				}
			}
		}

#ifdef CONFIG_WIFI_SIMPLE_CONFIG // WPS
		update_wps_configured(0);
#endif
	}
#if defined(WLAN_DUALBAND_CONCURRENT)
	wlan_idx = orig_idx;
#endif
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	mib_set(MIB_WIFI_MODULE_DISABLED, (void *)&wlanDisabled);
#else
//	mib_set(MIB_WIFI_MODULE_DISABLED, (void *)&wlanDisabled);
#if 0
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);

	for (i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&vcEntry))
				return -1;

		if(!(vcEntry.applicationtype & X_CT_SRV_INTERNET)){
			wlan_dev_map |= vcEntry.itfGroup;
		}
	}
	//printf("wlan_dev_map = %u\n", wlan_dev_map);
#endif
	//mib_get_s(MIB_WIFI_SSID_ENABLE_STATUS, (void *)&orig_ssid_enable_status, sizeof(orig_ssid_enable_status)); ////0x10001, bit 0 ssid 2G-1, bit 16 ssid 5G-1

#if defined(WLAN_DUALBAND_CONCURRENT)
	int orig_idx = wlan_idx;
#endif

	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		mib_get_s(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));
		mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry);

		//if(wlanDisabled && !Entry.wlanDisabled)
		//	ssid_enable_status |= (1 << i*(SSID_INTERFACE_SHIFT_BIT));

		//if(!(wlan_dev_map & (1<<(PMAP_WLAN0 + i*5)))){
		if((mask & (1<<(i*SSID_INTERFACE_SHIFT_BIT)))){
			if(wlanDisabled != Entry.wlanDisabled){
				/*if((!wlanDisabled && (orig_ssid_enable_status &(1 << i*SSID_INTERFACE_SHIFT_BIT))) || wlanDisabled)*/
				{
					Entry.wlanDisabled = wlanDisabled;
					mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, 0);
#ifdef WLAN_USE_VAP_AS_SSID1
					if(phyband==PHYBAND_5G)
						config_WLAN(wlanDisabled? ACT_STOP_5G : ACT_START_5G, 0);
					else
						config_WLAN(wlanDisabled? ACT_STOP_2G : ACT_START_2G, 0);
#else
#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_CTCAPD)
		if(wlanDisabled == 0)
		{
			if(Entry.func_off == 1)
				wlanDisabled = 1;
			else
				wlanDisabled = 0;
		}
#endif
		set_func(i, wlanDisabled);
#endif
				}
			}
		}

		for(j=0; j<WLAN_MBSSID_NUM; j++){

			mib_chain_get(MIB_MBSSIB_TBL, j+1, (void *)&Entry);

			//if(wlanDisabled && !Entry.wlanDisabled)
			//	ssid_enable_status |= (1 << ((j+1) + i*SSID_INTERFACE_SHIFT_BIT));

			//if(!(wlan_dev_map & (1<<(PMAP_WLAN0 + (j+1) + i*5)))){
			if((mask & (1<<(i*SSID_INTERFACE_SHIFT_BIT+(j+1))))){
				if(wlanDisabled != Entry.wlanDisabled){
					/*if((!wlanDisabled && (orig_ssid_enable_status &(1 << ((j+1)+ i*SSID_INTERFACE_SHIFT_BIT)))) || wlanDisabled)*/
					{
						Entry.wlanDisabled = wlanDisabled;
						mib_chain_update(MIB_MBSSIB_TBL, (void *)&Entry, j+1);
						if(phyband==PHYBAND_5G)
							config_WLAN(wlanDisabled? ACT_STOP_5G : ACT_START_5G, j+1);
						else
							config_WLAN(wlanDisabled? ACT_STOP_2G : ACT_START_2G, j+1);
					}
#ifdef CONFIG_CU_BASEON_YUEME
					mib_get(MIB_WLAN_GUEST_SSID, (void *)&vChar);
					printf("%s	wlan%d-vap%d closed, it %s a wlan guest.\n",__FUNCTION__,i,j,(vChar == j+1)?"is":"isn't");
					if(vChar == j+1)
						clear_wlan_guest_duration(i);
#endif

				}
			}
		}
	}
#if defined(WLAN_DUALBAND_CONCURRENT)
	wlan_idx = orig_idx;
#endif
#ifdef CONFIG_WIFI_SIMPLE_CONFIG // WPS
	//update_wps_configured(0);
#endif

	//if(wlanDisabled)
	//	mib_set(MIB_WIFI_SSID_ENABLE_STATUS, (void *)&ssid_enable_status);

	//recheck wifi led
	set_wlan_led_status(2);

#endif
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif // of #if COMMIT_IMMEDIATELY

//	if(start==0)
//		config_WLAN(ACT_STOP, CONFIG_SSID_ALL);
//	else
//		config_WLAN(ACT_START, CONFIG_SSID_ALL);
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC) || defined(CONFIG_CU_BASEON_YUEME)
	config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
#endif

}

int main (int argc, char **argv)
{
	if(argc==3){
		if(!strcmp(argv[1],"0"))
			start_wifi(0, atoi(argv[2]));
		else
			start_wifi(1, atoi(argv[2]));
	}
	else{
		if(!strcmp(argv[1],"0"))
			//start_wifi(0, 0xffffffff);
			config_WLAN(ACT_STOP, CONFIG_SSID_ALL);
		else
			//start_wifi(1, 0xffffffff);
			config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
	}
	return 0;
}

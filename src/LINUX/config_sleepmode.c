#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <signal.h>
#include "utility.h"
#include "rtk/ponmac.h"
#include "rtk/gponv2.h"
#include "rtk/epon.h"
#include "hal/chipdef/chip.h"


//extern int wlan_idx;
#define WRITE_MEM32(addr, val)   (*(volatile unsigned int *)   (addr)) = (val)
#define READ_MEM32(addr)         (*(volatile unsigned int *)   (addr))

static void set_sleepmode(unsigned char enable)
{
	unsigned char vChar;
	unsigned char wlanDisabled = enable;
	//MIB_CE_MBSSIB_T Entry;
	int i;
	//unsigned int cid, crev, csub;
#if defined(WLAN_DUALBAND_CONCURRENT)
	//int orig_idx = wlan_idx;
#endif

#if 0
	for(i=0; i<NUM_WLAN_INTERFACE; i++)
	{
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
#endif
#if defined(YUEME_3_0_SPEC) || defined(CONFIG_CU_BASEON_YUEME)
	mib_local_mapping_set(MIB_WLAN_MODULE_DISABLED, 0, (void *)&wlanDisabled);
#ifdef WLAN_DUALBAND_CONCURRENT
	mib_local_mapping_set(MIB_WLAN_MODULE_DISABLED, 1, (void *)&wlanDisabled);
#endif
#else
	mib_set(MIB_WIFI_MODULE_DISABLED, (void *)&wlanDisabled);
#endif
	mib_set(RS_SLEEP_STATUS, (void *)&enable);

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	if(enable==1)
	{
		//LED
		system("/bin/mpctl led off");
		//LAN
#if defined(CONFIG_LUNA) && defined(CONFIG_RTL8686_SWITCH)
		set_all_lan_port_powerdown_state(1);
#endif
		//For Storage, Suspend the USB
		//system("echo 0 > /sys/bus/usb/devices/usb1/authorized");
		system("find /sys/bus/usb/devices/usb*/authorized -type f -exec sh -c 'echo 0 > {}' \\;");

#ifdef WLAN_SUPPORT
		//WiFi
		//for(i=0; i<NUM_WLAN_INTERFACE; i++)
		{
			//wlan_idx = i;
			config_WLAN(ACT_STOP, CONFIG_SSID_ALL);
		}
#endif
	}
	else
	{
		unsigned char led_status;
		mib_get(MIB_LED_STATUS, (void *)&led_status);
		if(led_status)
		//LED
			system("/bin/mpctl led restore");
		//LAN
#if defined(CONFIG_LUNA) && defined(CONFIG_RTL8686_SWITCH)
		set_all_lan_port_powerdown_state(0);
#endif
		//For Storage, Resume the USB
		//system("echo 1 > /sys/bus/usb/devices/usb1/authorized");
		system("find /sys/bus/usb/devices/usb*/authorized -type f -exec sh -c 'echo 1 > {}' \\;");

#ifdef WLAN_SUPPORT
		//WiFi
		//for(i=0; i<NUM_WLAN_INTERFACE; i++)
		{
			//wlan_idx = i;
			config_WLAN(ACT_START, CONFIG_SSID_ALL);
		}
#endif
	}

#if defined(WLAN_DUALBAND_CONCURRENT)
	//wlan_idx = orig_idx;
#endif

}


int main (int argc, char **argv)
{
	if(!strcmp(argv[1],"0")){
		set_sleepmode(0);
	}
	else{
		set_sleepmode(1);
	}
	return 0;
}


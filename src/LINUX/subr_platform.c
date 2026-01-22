/*-- System inlcude files --*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include <err.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <libgen.h>
#include <string.h>

#include "subr_net.h"
#include "utility.h"
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_rate.h>
#include <rtk/rt/rt_stat.h>
#include "rtk/gpon.h"
#endif

int adjustSwitchQueue(void){
	int queueIdx = 0, ret = 0;
	char cmd[256] = {0};
#if defined(CONFIG_RTL9607C) && defined(CONFIG_COMMON_RT_API)
	for ( queueIdx = 1 ; queueIdx <= 7; queueIdx ++ ){
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "diag flowctrl set ingress egress-drop queue-id %d threshold 80", queueIdx);
		printf("cmd [%s]\n", cmd);
		system(cmd);
		//if((rtl9607c_raw_flowctrl_queueEegressDropThreshold_set(queueIdx, 80)) != RT_ERR_OK)
		//	printf("[%s-%d] set flowctrl_queueEegressDropThreshold_set fail ret =%x\n", __func__,__LINE__,ret);
	}
#endif
	return 0;
}

int disableStormControl(void){
	int i = 0, portIdx = 0, ret = 0;
#ifdef CONFIG_COMMON_RT_API
	for(i=0;i<ELANVIF_NUM;i++)
	{
		portIdx = rtk_port_get_lan_phyID(i);
		if 
((ret = rt_rate_stormControlPortEnable_set(portIdx, RT_STORM_GROUP_UNKNOWN_UNICAST, DISABLED)) != RT_ERR_OK)
			printf("[%s-%d] set storm control fail ret =%x\n", __func__,__LINE__,ret);
	}
#ifndef CONFIG_RTL_MULTI_PHY_ETH_WAN
	portIdx = rtk_port_get_wan_phyID();
	if 
((ret = rt_rate_stormControlPortEnable_set(portIdx, RT_STORM_GROUP_UNKNOWN_UNICAST, DISABLED)) != RT_ERR_OK)
		printf("[%s-%d] set storm control fail ret =%x\n", __func__,__LINE__,ret);
#endif
#endif
	
	return 0;
}


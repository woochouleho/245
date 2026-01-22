#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mib.h"
#include "sysconfig.h"
#include "utility.h"
#include "subr_switch.h"

#if defined(CONFIG_RTL8367_SUPPORT) && defined(CONFIG_LIB_8367)
#include "rtk_error.h"
#include "rtk_types.h"
#include "rtk_switch.h"
#include "svlan.h"
#include "port_extsw.h"
#include "l2_extsw.h"
#endif

#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
#if defined(CONFIG_RTL8367_SUPPORT) && defined(CONFIG_LIB_8367)
int rtk_switch_rtl8367_init(void)
{
	char sysbuf[128];
	rtksw_api_ret_t retVal;
	rtksw_svlan_memberCfg_t svlanCfg;
	int ret, i, phyPortId, pvid;
	char pvidStartStr[32];
	unsigned char igmpSingleNetdev = 0;

#if 0
	ret = rtksw_switch_init();
	if (ret == RT_ERR_OK) {
		printf("rtksw_switch_init - Done");
	}
	else {
		if (ret == RT_ERR_FAILED)
			printf("rtksw_switch_init - Failed (%d)", ret);
		else if (ret == RT_ERR_SMI)
			printf("rtksw_switch_init - SMI access error (%d)", ret);
		else
			printf("rtksw_switch_init - Unknown error (%d)", ret);
	}
#endif

	/* SVLAN initialization */
	if ((retVal = rtksw_svlan_init()) != RT_ERR_OK)
	    return retVal;

	/* Setup uplink port = EXT_PORT0 */
	if ((retVal = rtksw_svlan_servicePort_add(EXT_PORT0)) != RT_ERR_OK)
	    return retVal;

	mib_get_s(MIB_EXTERNAL_SWITCH_PVID_START, (void *)pvidStartStr, sizeof(pvidStartStr));
	for(i=0;i<ELANVIF_NUM;i++)
	{
		phyPortId = rtk_external_port_get_lan_phyID(i);
		if(phyPortId == -1){
			continue;
		}
		pvid = atoi(pvidStartStr)+phyPortId;
		printf("%s-%d: port=%d pvid=%d\n", __func__, __LINE__, phyPortId, pvid);

		memset(&svlanCfg, 0x00, sizeof(rtksw_svlan_memberCfg_t));
		RTKSW_PORTMASK_PORT_SET(svlanCfg.memberport, EXT_PORT0);
		RTKSW_PORTMASK_PORT_SET(svlanCfg.memberport, phyPortId);
		RTKSW_PORTMASK_PORT_SET(svlanCfg.untagport, phyPortId);
		if ((retVal = rtksw_svlan_memberPortEntry_set(pvid, &svlanCfg)) != RT_ERR_OK)
		{
			printf("%s-%d: rtksw_svlan_memberPortEntry_set FAIL (%d)\n", __func__, __LINE__, retVal);
		}

		if ((retVal = rtksw_svlan_defaultSvlan_set(phyPortId, pvid)) != RT_ERR_OK)
		{
			printf("%s-%d: rtksw_svlan_defaultSvlan_set FAIL (%d)\n", __func__, __LINE__, retVal);
		}
	}

	mib_get_s(MIB_EXTERNAL_SWITCH_HANDLE_IGMP_SINGLE_NETDEV, (void *)&igmpSingleNetdev, sizeof(igmpSingleNetdev));
	if (igmpSingleNetdev) {
		pvid = atoi(pvidStartStr)+SVLAN_VID_MULTICAST_OFFSET;
		printf("%s-%d: multicast svid=%d\n", __func__, __LINE__, pvid);

		memset(&svlanCfg, 0x00, sizeof(rtksw_svlan_memberCfg_t));
		RTKSW_PORTMASK_PORT_SET(svlanCfg.memberport, EXT_PORT0);
		for(i=0;i<ELANVIF_NUM;i++)
		{
			phyPortId = rtk_external_port_get_lan_phyID(i);
			if(phyPortId == -1){
				continue;
			}
			RTKSW_PORTMASK_PORT_SET(svlanCfg.memberport, phyPortId);
			RTKSW_PORTMASK_PORT_SET(svlanCfg.untagport, phyPortId);
		}
		if ((retVal = rtksw_svlan_memberPortEntry_set(pvid, &svlanCfg)) != RT_ERR_OK)
		{
			printf("%s-%d: rtksw_svlan_memberPortEntry_set FAIL (%d)\n", __func__, __LINE__, retVal);
		}
		sprintf(sysbuf, "/bin/echo MCAST %d > /proc/rtl8367/svlan", pvid);
		va_cmd("/bin/sh", 2, 1, "-c", sysbuf);
	}

	retVal = rtksw_l2_limitLearningCntAction_set(RTKSW_WHOLE_SYSTEM, LIMIT_LEARN_CNT_ACTION_DROP);

	return RT_ERR_OK;
}

#define MAX_NUM_OF_LEARN_LIMIT (2112)
int rtk_switch_rtl8367_l2_limitLearningCnt_set(unsigned int port, int mac_cnt)
{
	int ret = 0;

	if(mac_cnt == -1)
		mac_cnt = MAX_NUM_OF_LEARN_LIMIT;

	printf("[rtl8367] Setting action drop on port %d, mac_cnt=%d\n", port, mac_cnt);

	if(rtksw_l2_limitLearningCnt_set(port, mac_cnt) != RT_ERR_OK)
		fprintf(stderr, "[rtl8367] rtksw_l2_limitLearningCnt_set failed, ret=%d\n", ret);

	return ret;
}
#endif
#endif

int rtk_switch_l2_limitLearningCnt_set(unsigned int port, int cnt)
{
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
#if defined(CONFIG_RTL8367_SUPPORT) && defined(CONFIG_LIB_8367)
		return rtk_switch_rtl8367_l2_limitLearningCnt_set(port, cnt);
#else
		return 0;
#endif
#else
		return 0;
#endif
}

int rtk_switch_init(void)
{
#ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
#if defined(CONFIG_RTL8367_SUPPORT) && defined(CONFIG_LIB_8367)
	return rtk_switch_rtl8367_init();
#else
	return 0;
#endif
#else
	return 0;
#endif
}


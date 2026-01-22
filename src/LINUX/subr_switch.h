#ifndef SUBR_SWITCH_H
#define SUBR_SWITCH_H

#ifdef CONFIG_EXTERNAL_SWITCH_LAN_PORT_NUM
#define SVLAN_VID_MULTICAST_OFFSET	(CONFIG_EXTERNAL_SWITCH_LAN_PORT_NUM+1)
#else
#define SVLAN_VID_MAX	4095
#define SVLAN_VID_MULTICAST_OFFSET	(SVLAN_VID_MAX-1)
#endif

int rtk_switch_l2_limitLearningCnt_set(unsigned int port, int cnt);

#endif

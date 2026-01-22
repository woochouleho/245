#include <sys/ioctl.h>
#include <netinet/if_ether.h>
#include <linux/if.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <memory.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/config.h>

#include <common/rt_type.h>
#include <common/rt_error.h>
#include <common/util/rt_util.h>

#include <rtk/rt/rt_switch.h>
#include <rtk/rt/rt_port.h>
#include <rtk/rt/rt_mirror.h>

#include "utility.h"

#define MAX_PORT_NUM	11
void usage(void);

#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_RTL9603CVD_SERIES)
#include <rtk/pbo.h>
int pboDisable(unsigned int toPort)
{
	rtk_enable_t pboEnable;
	rtk_enable_t pboAutoConfEnable;
	int ret;

	ret=rtk_swPbo_portState_get(toPort, &pboEnable);
	if(ret==RT_ERR_OK && pboEnable==ENABLED)
	{
		char cmd[128] = {0};
		sprintf(cmd, "echo 1 > /tmp/PortMirrorPboEnabledPort%d", toPort);
		system(cmd);
		ret=rtk_swPbo_portAutoConf_get(toPort, &pboAutoConfEnable);
		if(ret==RT_ERR_OK)
		{
			if(pboAutoConfEnable==ENABLED)
				sprintf(cmd, "echo 1 > /tmp/PortMirrorPboAutoConfEnabledPort%d", toPort);
			else
				sprintf(cmd, "echo 1 > /tmp/PortMirrorPboAutoConfDisabledPort%d", toPort);
			system(cmd);
		}
		pboEnable = DISABLED;
		ret=rtk_swPbo_portState_set(toPort, pboEnable);
		pboAutoConfEnable = DISABLED;
		ret=rtk_swPbo_portAutoConf_set(toPort, pboAutoConfEnable);
	}

	return 0;
}

int pboRestore(void)
{
	rtk_enable_t pboEnable;
	rtk_enable_t pboAutoConfEnable;
	char dirPbo[128] = {0};
	char dirPboAutoConf[128] = {0};
	int i, ret;

	for(i=0 ; i<CONFIG_LAN_PORT_NUM ; i++)
	{
		sprintf(dirPbo, "/tmp/PortMirrorPboEnabledPort%d", rtk_port_get_lan_phyID(i));
		if(!access(dirPbo, F_OK))
		{
			pboEnable = ENABLED;
			ret=rtk_swPbo_portState_set(rtk_port_get_lan_phyID(i), pboEnable);
			unlink(dirPbo);
		}
		sprintf(dirPboAutoConf, "/tmp/PortMirrorPboAutoConfEnabledPort%d", rtk_port_get_lan_phyID(i));
		if(!access(dirPboAutoConf, F_OK))
		{
			pboAutoConfEnable = ENABLED;
			ret=rtk_swPbo_portAutoConf_set(rtk_port_get_lan_phyID(i), pboAutoConfEnable);
			unlink(dirPboAutoConf);
		}
		sprintf(dirPboAutoConf, "/tmp/PortMirrorPboAutoConfDisabledPort%d", rtk_port_get_lan_phyID(i));
		if(!access(dirPboAutoConf, F_OK))
		{
			pboAutoConfEnable = DISABLED;
			ret=rtk_swPbo_portAutoConf_set(rtk_port_get_lan_phyID(i), pboAutoConfEnable);
			unlink(dirPboAutoConf);
		}
	}

	return 0;
}
#endif

static int hw_port_to_str(rt_port_t port, char *buf)
{
	unsigned int phyPortId;
	int remapped = 0;
	int i;

	if(buf == NULL)
	{
		fprintf(stderr, "%s: buf is NULL\n", __func__);
		return -1;
	}


	for(i=0 ; i<SW_LAN_PORT_NUM ; i++)
	{
		phyPortId = rtk_port_get_lan_phyID(i);
		if(phyPortId == port)
		{
			remapped = 1;
			sprintf(buf, "port%d", i+1);
		}
	}
	if(!remapped) {
		if(port == rtk_port_get_wan_phyID()) {
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
			sprintf(buf, "pon");
#else
			sprintf(buf, "wan");
#endif
		} else
			sprintf(buf, "port%u", port);
	}

	return 0;
}

static int hw_portmask_to_str(uint32_t portmask, char *buf)
{
	unsigned int phyPortId, cpu_portmask = rtk_port_get_cpu_phyMask();
	int remapped = 0;
	int i, j;
	char str_port[8] = "";

	if(buf == NULL)
	{
		fprintf(stderr, "%s: buf is NULL\n", __func__);
		return -1;
	}

	buf[0] = '\0';

	if((portmask & cpu_portmask) == cpu_portmask)
	{
		sprintf(buf, "%s ", "cpu");
		portmask &= ~cpu_portmask;
	}

	for(i=0 ; i<MAX_PORT_NUM ; i++)
	{
		if(portmask & (1<<i)) {
			remapped = 0;
			for(j=0 ; j<SW_LAN_PORT_NUM ; j++)
			{
				phyPortId = rtk_port_get_lan_phyID(i);
				if(phyPortId == i) {
					remapped = 1;
					hw_port_to_str(phyPortId, str_port);
					sprintf(buf + strlen(buf), "%s ", str_port);
				}
			}
			if(!remapped)
			{
				if(i==rtk_port_get_wan_phyID()) {
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
					sprintf(buf + strlen(buf), "%s ", "pon ");
#else
					sprintf(buf + strlen(buf), "%s ", "wan ");
#endif
				}
				else
					sprintf(buf + strlen(buf), "port%d ", i);
			}
		}
	}

	return 0;
}

int dumpPortMirror(void)
{
	rt_port_t port;
    rt_portmask_t tx_portmask;
    rt_portmask_t rx_portmask;
	char port_str[256] = "";

	unsigned char re_map_tbl[MAX_LAN_PORT_NUM];
	int remapped;
	int ret, i, j;

	ret = rt_mirror_portBased_get(&port, &rx_portmask, &tx_portmask);
	if(ret != RT_ERR_OK)
		return -1;

	hw_port_to_str(port, port_str);
	printf("Monitor port: %s\n", port_str);

	hw_portmask_to_str(rx_portmask.bits[0], port_str);
	printf("Mirroring RX portmask: %s\n", port_str);

	hw_portmask_to_str(tx_portmask.bits[0], port_str);
	printf("Mirroring TX portmask: %s\n", port_str);

	return ret;
}

int setPortMirror(unsigned char *fromPort, unsigned char *toPort, unsigned char *direction)
{
	rt_port_t monitor_port = 0;
	rt_portmask_t mirrored_port_mask;
	rt_portmask_t rx_port_mask, tx_port_mask;

	unsigned char re_map_tbl[MAX_LAN_PORT_NUM];
	unsigned int phyPortId;
	char tempFromPort[256] = {0};

	char *dot = ",";
	char * pch;

	//printf ("Splitting fromPort \"%s\" into tokens:\n",fromPort);

	if(!fromPort)
		return -1;

	mirrored_port_mask.bits[0] = 0;

	snprintf(tempFromPort,sizeof(tempFromPort), "%s", fromPort);
	pch = strtok(fromPort, dot);
	while (pch != NULL)
	{
		//printf ("%s\n",pch);
		if(atoi(pch)-1 >= 0 && atoi(pch)-1 < MAX_PORT_NUM) {
			if(atoi(pch) <= SW_LAN_PORT_NUM) {
				phyPortId = rtk_port_get_lan_phyID(atoi(pch)-1);
				mirrored_port_mask.bits[0] |= (1<<phyPortId);
			} else {
				mirrored_port_mask.bits[0] |= (1<<atoi(pch));
			}
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
		} else if(!strcmp(pch, "pon")) {
#else
		} else if(!strcmp(pch, "wan")) {
#endif
			mirrored_port_mask.bits[0] |= rtk_port_get_wan_phyMask();
		} else if(!strcmp(pch, "cpu")) {
			mirrored_port_mask.bits[0] |= rtk_port_get_cpu_phyMask();
		} else {
			printf("Invalid fromPort(%d).\n", atoi(pch));
			return -1;
		}

		pch = strtok (NULL, dot);
	}

	if(atoi(toPort)-1 >= 0 && atoi(toPort)-1 < MAX_PORT_NUM) {
		if(atoi(toPort) <= SW_LAN_PORT_NUM) {
			phyPortId = rtk_port_get_lan_phyID(atoi(toPort)-1);
			monitor_port = phyPortId;
		} else {
			monitor_port = atoi(toPort);
		}
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	} else if(!strcmp(toPort, "pon")) {
#else
	} else if(!strcmp(toPort, "wan")) {
#endif
		monitor_port = rtk_port_get_wan_phyID();
	} else if(!strcmp(toPort, "cpu")) {
		printf("Invalid toPort(cpu).\n");
		return -1;
	} else {
		printf("Invalid toPort(%d).\n", atoi(toPort));
		return -1;
	}

	if(!strcmp(direction, "rx")) {
		rx_port_mask.bits[0] = mirrored_port_mask.bits[0];
		tx_port_mask.bits[0] = 0;
	} else if(!strcmp(direction, "tx")) {
		rx_port_mask.bits[0] = 0;
		tx_port_mask.bits[0] = mirrored_port_mask.bits[0];
	} else if(!strcmp(direction, "rtx")) {
		rx_port_mask.bits[0] = mirrored_port_mask.bits[0];
		tx_port_mask.bits[0] = mirrored_port_mask.bits[0];
	} else {
		printf("Invalid direction(%s).\n", direction);
		return -1;
	}
#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_RTL9603CVD_SERIES)
	pboDisable(monitor_port);
#endif

	//printf("Set port mirror direction:%s from port %s to port %s.\n", direction, tempFromPort, toPort);

	if(rt_mirror_portBased_set(monitor_port, &rx_port_mask, &tx_port_mask) == RT_ERR_OK)
		return 0;
	else
		return -1;
}

int clearPortMirror(void)
{
	if(rt_mirror_init() == RT_ERR_OK) {
		//printf("Port mirror cleared.\n");
		return 0;
	} else
		return -1;
}

int main(int argc, char *argv[])
{
	int argIdx=1;


	if (argc<=1)
		goto arg_err_rtn;

	if(!strcmp(argv[argIdx], "dump"))
	{
		dumpPortMirror();
	}
	else if(!strcmp(argv[argIdx], "set"))
	{
		if (argc<=4)
			goto arg_err_rtn;

#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_RTL9603CVD_SERIES)
		pboRestore();
#endif
		if(setPortMirror(argv[argIdx+1], argv[argIdx+2], argv[argIdx+3]) == -1)
			printf("Set FAIL!\n");
		else {
			system("echo 1 > /proc/fc/ctrl/dsliteHwAcceleration_disable");
			printf("DONE.\n");
		}
	}
	else if(!strcmp(argv[argIdx], "clear"))
	{
		system("echo 0 > /proc/fc/ctrl/dsliteHwAcceleration_disable");
#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_RTL9603CVD_SERIES)
		pboRestore();
#endif
		clearPortMirror();
	}
	else
	{
		if(strcmp(argv[argIdx], "--help") && strcmp(argv[argIdx], "?"))
			printf("Invalid parameter!\n");
		goto arg_err_rtn;
	}

	return 0;

arg_err_rtn:
	usage();
	exit(1);

}

void usage(void)
{
	printf("Usage:\n");
	printf("	PortMirror dump\n");
	printf("	PortMirror set [fromPorts] [toPort] [rtx/rx/tx]\n");
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	printf("	- ex. PortMirror set pon,cpu 1 rtx\n");
	printf("	- fromPorts: 1 means LAN1, 2 means LAN2, ... , pon means PON port, cpu means CPU port\n");
#else
	printf("	- ex. PortMirror set wan,cpu 1 rtx\n");
	printf("	- fromPorts: 1 means LAN1, 2 means LAN2, ... , wan means WAN port, cpu means CPU port\n");
#endif
	printf("	PortMirror clear\n");
}


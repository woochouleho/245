#include <netinet/in.h>
#include <linux/config.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "utility.h"
#include "mib.h"
#include "subr_net.h"
#include "fc_api.h"
#include "options.h"
#include "subr_net.h"
#include "sockmark_define.h"

#ifdef CONFIG_COMMON_RT_API
#include <rtk/rt/rt_rate.h>
#include <rtk/rt/rt_l2.h>
#include <rtk/rt/rt_qos.h>
#include <rtk/rt/rt_epon.h>
#include <rtk/rt/rt_gpon.h>
#include <rtk/rt/rt_switch.h>
#include <rtk/rt/rt_i2c.h>
#include "rt_rate_ext.h"
#include "rt_igmp_ext.h"

void usage()
{
	printf("usage:\n");
	printf("acl_limit_bandwidth add [length_start] [length_end] [rate]\n");
	printf("acl_limit_bandwidth del [rate]\n");
}

int rtk_fc_acl_bandwidth_limit_add(int length_start, int length_end, unsigned int rate)
{
	rt_acl_filterAndQos_t aclRule;
	unsigned int aclIdx=0;
	int meter_idx[ELANVIF_NUM], ret, i;
	FILE *fp;
	
	if(!(fp = fopen("/var/fc_bandwidth_limit_acl", "a")))
	{
		fprintf(stderr, "ERROR! %s\n", strerror(errno));
		return -1;
	}
	
	for(i=0; i < ELANVIF_NUM; i++){
		meter_idx[i] = rtk_qos_share_meter_set(RTK_APP_TYPE_ACL_LIMIT_BANDWIDTH, 0, rate);
#ifdef CONFIG_COMMON_RT_API
		//wan ingress SYN pkt limit 1M
		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = WEIGHT_HIGH;
		aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		aclRule.ingress_port_mask = (1 << rtk_port_get_lan_phyID(i));
		aclRule.filter_fields |= RT_ACL_INGRESS_PKT_LEN_RANGE_BIT;
		aclRule.ingress_packet_length_start = length_start;
		aclRule.ingress_packet_length_end = length_end;
		aclRule.action_fields = RT_ACL_ACTION_METER_GROUP_SHARE_METER_BIT;
		aclRule.action_meter_group_share_meter_index = meter_idx[i];
		if((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
			fprintf(fp, "%d\n", aclIdx);
		else
			printf("[%s@%d] rt_acl_filterAndQos_add QoS rule failed! (ret = %d)\n",__func__,__LINE__, ret);
#endif
	}

	fclose(fp);
}

int rtk_fc_acl_bandwidth_limit_del(unsigned int rate)
{
	unsigned int aclIdx=0;
	int meter_idx, ret;
	char line[1024];
	FILE *fp;
	
	if(!(fp = fopen("/var/fc_bandwidth_limit_acl", "r")))
	{
		fprintf(stderr, "ERROR! %s\n", strerror(errno));
		return -2;
	}

	while(fgets(line, 1023, fp) != NULL)
	{
		sscanf(line, "%d\n", &aclIdx);
		if((ret = rt_acl_filterAndQos_del(aclIdx)) == RT_ERR_OK)
			printf("%s: delete syn flood acl rule %d success ! (ret = %d)\n", __FUNCTION__,aclIdx,ret);
		else
			printf("%s: delete flood acl rule %d fail ! (ret = %d)\n", __FUNCTION__,aclIdx,ret);
	}

	fclose(fp);
	unlink("/var/fc_bandwidth_limit_acl");
	
	meter_idx = rtk_qos_share_meter_get(RTK_APP_TYPE_ACL_LIMIT_BANDWIDTH, 0, rate);
	if (meter_idx >= 0){
		rtk_qos_share_meter_delete(RTK_APP_TYPE_ACL_LIMIT_BANDWIDTH, 0);
	}
}

int main(int argc, char *argv[])
{
	if(argc >= 2)
	{
		int argi = 0;
		for (argi = 1 ; argi < argc; argi++)
		{
			if (strcmp(argv[argi], "add") == 0 && argc == 5)
			{
				rtk_fc_acl_bandwidth_limit_add(atoi(argv[argi+1]), atoi(argv[argi+2]), atoi(argv[argi+3]));
			}
			else if (strcmp(argv[argi], "del") == 0 && argc == 3)
			{
				rtk_fc_acl_bandwidth_limit_del(atoi(argv[argi+1]));
			}
		}
	}
	else{
		usage();
	}
}
#endif



/*-- System inlcude files --*/
#include <string.h>
#include <signal.h>
#include <sys/socket.h>

/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "multilang.h"

///////////////////////////////////////////////////////////////////
void formConfig_802_1x(request * wp, char *path, char *query)
{
	char *str, *submitUrl, *strSubmit;
	char tmpBuf[100];
	unsigned char rsIpAddr[IP_ADDR_LEN];
	unsigned short rsPort;
	unsigned char rsPassword[MAX_PSK_LEN+1];
	int ad_state_map = 0;
	struct in_addr rsIp;

	submitUrl = boaGetVar(wp, "submit-url", "");

	strSubmit = boaGetVar(wp, "refresh", "");
	if (strSubmit[0]) {
		boaRedirect(wp, submitUrl);
		return;
	}

	str = boaGetVar(wp, "portBasedAuthEnabled", "");
	//printf("portBasedAuthEnabled: %d\n", atoi(str));
	rtk_wired_8021x_set_mode(atoi(str) ? WIRED_8021X_MODE_PORT_BASED : WIRED_8021X_MODE_MAC_BASED);

	str = boaGetVar(wp, "radiusIP", "");
	*(unsigned int*)&(rsIpAddr) = inet_addr(str);
	//printf("rsIpAddr: %s\n", str);
	memset(&rsIp, 0, sizeof(struct in_addr));
	inet_aton(str, &rsIp);
	mib_set(MIB_WIRED_8021X_RADIAS_IP, (void *)&rsIp);

	str = boaGetVar(wp, "radiusPort", "");
	rsPort = atoi(str);
	//printf("rsPort: %d\n", rsPort);
	mib_set(MIB_WIRED_8021X_RADIAS_UDP_PORT, (void *)&rsPort);

	str = boaGetVar(wp, "radiusPass", "");
	strncpy(rsPassword,str, MAX_PSK_LEN+1);
	rsPassword[MAX_PSK_LEN] = 0;
	//printf("rsPassword: %s\n", rsPassword);
	mib_set(MIB_WIRED_8021X_RADIAS_SECRET, (void *)rsPassword);

	str = boaGetVar(wp, "adminStatePortmap", "");
	ad_state_map = atoi(str);
	//printf("ad_state_map: 0x%x\n", ad_state_map);
	rtk_wired_8021x_set_enable(ad_state_map);
	
	Commit();
	OK_MSG(submitUrl);

	rtk_wired_8021x_take_effect();
	return;

setErr_others:
	ERR_MSG(tmpBuf);
}

int getPortStatus(int eid, request * wp, int argc, char **argv)
{
        char *name, boaVariable[64];
        int i, result[SW_LAN_PORT_NUM];

        for(i=0 ; i<SW_LAN_PORT_NUM ; i++)
        {
                boaArgs(argc, argv, "%s", &name);
                sprintf(boaVariable, "port%d", i+1);
	        if( !strcmp(name, boaVariable) )
                {
                        if(!rtk_port_get_lan_phyStatus(i, &result[i]))
                        {
                                if(result[i] == 1)
                                        boaWrite(wp, "Link Up");
                                else if(result[i] == 0)
                                        boaWrite(wp, "Link Down");
                        }
                        else
                                boaWrite(wp, "Link Down");
	        }
        }

        return 0;
}

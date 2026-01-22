#include <sys/ioctl.h>
#include <netinet/if_ether.h>
#include "adsl_drv.h"	
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include "utility.h"

int main(int argc, char *argv[])
{
	int ret, ret1, mibIndex, i;
	unsigned int macAddr[6];
	unsigned char macAddrChar[6];
	unsigned char wired8021xMode;

	mib_get(MIB_WIRED_8021X_MODE, (void *)&wired8021xMode);
	
	if(!strcmp(argv[1], "0")||!strcmp(argv[1], "mode"))
	{
		if(!strcmp(argv[2], "mac"))
			ret = rtk_wired_8021x_set_mode(WIRED_8021X_MODE_MAC_BASED);
		else if(!strcmp(argv[2], "port"))
			ret = rtk_wired_8021x_set_mode(WIRED_8021X_MODE_PORT_BASED);
		else
		{
			printf("no such mode ... \n");
			ret = -1;
		}
	}
	else if(!strcmp(argv[1], "1")||!strcmp(argv[1], "enable"))
	{
		ret = rtk_wired_8021x_set_enable(atoi(argv[2]));
	}
	else if(!strcmp(argv[1], "2")||!strcmp(argv[1], "authmac"))
	{
		ret1 = sscanf(argv[2], "%02x:%02x:%02x:%02x:%02x:%02x", &macAddr[0]
			, &macAddr[1], &macAddr[2], &macAddr[3], &macAddr[4], &macAddr[5]);
		if(ret1!= 1 ) {
			printf("Did not convert the value\n");
		}
		printf("%s %d: MAC=%02X:%02X:%02X:%02X:%02X:%02X\n", __func__, __LINE__, macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
		macAddrChar[0] = macAddr[0];
		macAddrChar[1] = macAddr[1];
		macAddrChar[2] = macAddr[2];
		macAddrChar[3] = macAddr[3];
		macAddrChar[4] = macAddr[4];
		macAddrChar[5] = macAddr[5];
		ret = rtk_wired_8021x_set_auth(-1, &macAddrChar[0]);
	}
	else if(!strcmp(argv[1], "3")||!strcmp(argv[1], "authport"))
	{
		ret = rtk_wired_8021x_set_auth(atoi(argv[2]), NULL);
	}
	else if(!strcmp(argv[1], "4")||!strcmp(argv[1], "unauthmac"))
	{
		ret1 = sscanf(argv[2], "%02x:%02x:%02x:%02x:%02x:%02x", &macAddr[0]
			, &macAddr[1], &macAddr[2], &macAddr[3], &macAddr[4], &macAddr[5]);
		if(ret1!= 1 ) {
			printf("Did not convert the value\n");
		}
		printf("%s %d: MAC=%02X:%02X:%02X:%02X:%02X:%02X\n", __func__, __LINE__, macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
		macAddrChar[0] = macAddr[0];
		macAddrChar[1] = macAddr[1];
		macAddrChar[2] = macAddr[2];
		macAddrChar[3] = macAddr[3];
		macAddrChar[4] = macAddr[4];
		macAddrChar[5] = macAddr[5];
		ret = rtk_wired_8021x_set_unauth(-1, &macAddrChar[0]);
	}
	else if(!strcmp(argv[1], "5")||!strcmp(argv[1], "unauthport"))
	{
		ret = rtk_wired_8021x_set_unauth(atoi(argv[2]), NULL);
	}
	else if(!strcmp(argv[1], "6")||!strcmp(argv[1], "auth"))
	{
		if(wired8021xMode == WIRED_8021X_MODE_PORT_BASED)
		{
			for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
			{
				if(!strcmp(ELANVIF[i], argv[2]))
				{
					ret = rtk_wired_8021x_set_auth(i, NULL);
					break;
				}
			}
		}
		else if(argc == 4)
		{
			ret1 = sscanf(argv[3], "%02x:%02x:%02x:%02x:%02x:%02x", &macAddr[0]
				, &macAddr[1], &macAddr[2], &macAddr[3], &macAddr[4], &macAddr[5]);
			if(ret1!= 1 ) {
				printf("Did not convert the value\n");
			}
			printf("%s %d: MAC=%02X:%02X:%02X:%02X:%02X:%02X\n", __func__, __LINE__, macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
			macAddrChar[0] = macAddr[0];
			macAddrChar[1] = macAddr[1];
			macAddrChar[2] = macAddr[2];
			macAddrChar[3] = macAddr[3];
			macAddrChar[4] = macAddr[4];
			macAddrChar[5] = macAddr[5];
			for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
			{
				if(!strcmp(ELANVIF[i], argv[2]))
				{
					ret = rtk_wired_8021x_set_auth(i, &macAddrChar[0]);
					break;
				}
			}
		}
	}
	else if(!strcmp(argv[1], "7")||!strcmp(argv[1], "unauth"))
	{
		if(wired8021xMode == WIRED_8021X_MODE_PORT_BASED)
		{
			for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
			{
				if(!strcmp(ELANVIF[i], argv[2]))
				{
					ret = rtk_wired_8021x_set_unauth(i, NULL);
					break;
				}
			}
		}
		else if(argc == 4)
		{
			ret1 = sscanf(argv[3], "%02x:%02x:%02x:%02x:%02x:%02x", &macAddr[0]
				, &macAddr[1], &macAddr[2], &macAddr[3], &macAddr[4], &macAddr[5]);
			if(ret1!= 1 ) {
				printf("Did not convert the value\n");
			}
			printf("%s %d: MAC=%02X:%02X:%02X:%02X:%02X:%02X\n", __func__, __LINE__, macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
			macAddrChar[0] = macAddr[0];
			macAddrChar[1] = macAddr[1];
			macAddrChar[2] = macAddr[2];
			macAddrChar[3] = macAddr[3];
			macAddrChar[4] = macAddr[4];
			macAddrChar[5] = macAddr[5];
			for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
			{
				if(!strcmp(ELANVIF[i], argv[2]))
				{
					ret = rtk_wired_8021x_set_unauth(i, &macAddrChar[0]);
					break;
				}
			}
		}
	}
	else if(!strcmp(argv[1], "8")||!strcmp(argv[1], "flushport"))
	{
		for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
		{
			if(!strcmp(ELANVIF[i], argv[2]))
			{
				ret = rtk_wired_8021x_flush_port(i);
				break;
			}
		}		
	}
	else if(!strcmp(argv[1], "9")||!strcmp(argv[1], "clean"))
	{
		if((argc == 3) && (atoi(argv[2]) == 0 || atoi(argv[2]) == 1))
			rtk_wired_8021x_clean(atoi(argv[2]));
		else
			rtk_wired_8021x_clean(1);
	}
	else if(!strcmp(argv[1], "10")||!strcmp(argv[1], "setup"))
	{
		rtk_wired_8021x_setup();
	}

	if(!ret)
		printf("SUCESS !!!\n");
	else
		printf("FAIL !!!\n");
}


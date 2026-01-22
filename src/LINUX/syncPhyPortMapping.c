#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include "utility.h"
#include <sys/stat.h>

#define _DEBUG 1

static int parsePort(char *src, char *output, int num)
{
	int count=0;
	char *strptr = NULL, *tokptr = NULL;
#if _DEBUG
	printf("src %s\n", src);
#endif
	const char *delim = ";";
	char *saveptr = NULL;
	
	strptr = src;
	tokptr = strtok_r(strptr, delim, &saveptr);
	while(tokptr!=NULL){
		if(count < num){
			output[count] = atoi(tokptr);
#if _DEBUG
			printf("output[%d]=%d\n", count, output[count]);
#endif
		}
		else{
			printf("parse port error!!\n");
			return -1;
		}
		count++;
		tokptr = strtok_r(NULL, delim, &saveptr);
	}
	if(count < num && tokptr != NULL){
		output[count] = atoi(tokptr);
#if _DEBUG
		printf("output[%d]=%d\n", count, output[count]);
#endif
		count++;
	}
	return count;
}

int main(int argc, char **argv)
{
	char phy_map_tbl[MAX_LAN_PORT_NUM]={0};
	char zero_map_tbl[MAX_LAN_PORT_NUM]={0};
	char cmd[128]={0};
	char tmpstr[10]={0}, tmpbuf[128]={0};
	int i = 0, line=0, logicPortNum=0;
	FILE *fp = NULL;
	
	mib_get_s(MIB_HW_PHY_PORTMAPPING, (void *)phy_map_tbl, sizeof(phy_map_tbl));
	
	if(memcmp(phy_map_tbl, zero_map_tbl, sizeof(zero_map_tbl)) ==0){
		//sync from /proc/realtek/uni_capability to mib
		fp = fopen("/proc/device-tree/rtk_uni_capability/physical_mapping", "r");
		if (fp == NULL){
			printf("%s:%d /proc/device-tree/rtk_uni_capability/physical_mapping cannot open\n", __FUNCTION__, __LINE__);
			return 0;
		}
		
		while (fgets(tmpbuf, sizeof(tmpbuf), fp)){
			if (tmpbuf[0]=='#'){
				continue;
			}
#if 0
			if(line == 0){ //skip first line, physical port mapping is located at the second line
				line++;
				continue;
			}
#endif
#if _DEBUG
			printf("tmpbuf is %s\n", tmpbuf);
#endif
			logicPortNum = parsePort(tmpbuf, phy_map_tbl, MAX_LAN_PORT_NUM);
#if _DEBUG
			printf("%s:%d logicPortNum %d\n", __FUNCTION__, __LINE__, logicPortNum);
#endif
			if(logicPortNum > 0) {
				mib_set(MIB_HW_PHY_PORTMAPPING, (void *)phy_map_tbl);
				mib_update(HW_SETTING, CONFIG_MIB_ALL);
			}
		}
		if(fp != NULL) fclose(fp);
	}
	else{
		for(i=0; i < ELANVIF_NUM; i++){
			snprintf(tmpstr, sizeof(tmpstr), "%d", phy_map_tbl[i]);
			tmpstr[strlen(tmpstr)+1]='\0';
			strcat(tmpbuf, tmpstr);
			if(i<ELANVIF_NUM-1){
				snprintf(tmpstr, sizeof(tmpstr), ";");
			}
			else{
				snprintf(tmpstr, sizeof(tmpstr), "");
			}
			tmpstr[strlen(tmpstr)+1]='\0';
			strcat(tmpbuf, tmpstr);
		}
		
		snprintf(cmd, sizeof(cmd),"echo 'REMAP %s' > %s", tmpbuf, PROC_UNI_CAPABILITY);
#if _DEBUG
		printf("cmd is %s\n", cmd);
#endif
		system(cmd);
	}

	return 0;
}
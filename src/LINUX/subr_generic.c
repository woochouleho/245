#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "utility.h"
#include "subr_generic.h"

#ifdef HANDLE_DEVICE_EVENT
int rtk_generic_pushbtn_reset_action(int action, int btn_test, int diff_time)
{
	if(action==0){	
		if ((diff_time >2) && (diff_time <=5)) {
			printf("Reset button reboot the system\n");
			if(!btn_test)
				cmd_reboot();
		}

		if (diff_time >5) {
			printf("Reset button reset to default\n");
			if(!btn_test)
			{
				//LED flash while factory reset
				//system("echo 2 > /proc/load_default");
				reset_cs_to_default(0);
				cmd_reboot();
			}
		}
	}
	return 0;
}
#endif

#ifdef CONFIG_CWMP_TR181_SUPPORT
char rtk_generic_update_chanchg_reason_count(char *ifname, char reason, char write)
{
	FILE *pFile;
	unsigned int count=0;
	char fileNameR[64], buff[8], fileNameC[64];

	memset( buff, 0, sizeof(buff) );
	memset( fileNameR, 0, sizeof(fileNameR) );
	memset( fileNameC, 0, sizeof(fileNameC) );

	if( access(WLAN_STATS_FOLDER, 0))
		mkdir(WLAN_STATS_FOLDER, 0755);

	snprintf(fileNameR, sizeof(fileNameR), "%s/%s_chan_chg_reason", WLAN_STATS_FOLDER, ifname);
	switch(reason)
	{    
		case WALN_CHANGE_MANUAL:
			snprintf(fileNameC, sizeof(fileNameC), "%s/%s_%s", WLAN_STATS_FOLDER, ifname, CHANNEL_CHANGE_COUNT_MANUAL);
			break;
		case WALN_CHANGE_AUTO_USER:
			snprintf(fileNameC, sizeof(fileNameC), "%s/%s_%s", WLAN_STATS_FOLDER, ifname, CHANNEL_CHANGE_COUNT_AUTO_USER);
			break;
		case WALN_CHANGE_AUTO_DFS:
			snprintf(fileNameC, sizeof(fileNameC), "%s/%s_%s", WLAN_STATS_FOLDER, ifname, CHANNEL_CHANGE_COUNT_AUTO_DFS);
			break;
		case WALN_CHANGE_AUTO_STARTUP:
		case WALN_CHANGE_AUTO_Refresh:
		case WALN_CHANGE_AUTO_DYNAMIC:
		case WALN_CHANGE_UNKNOWN:
			snprintf(fileNameC, sizeof(fileNameC), "%s/%s_%s", WLAN_STATS_FOLDER, ifname, CHANNEL_CHANGE_COUNT_KNOWN);
			break;
	}

	if(write)
	{
		pFile = fopen( fileNameR , "w" );
		if( NULL!=pFile )
		{    
			fprintf( pFile, "%u", reason );
			fclose( pFile );
		}

		pFile = fopen( fileNameC, "r" );
		if( NULL!=pFile )
		{    
			if(fgets( buff, sizeof(buff), pFile )!=NULL)
				count = atoi(buff);
			fclose( pFile );
		}
		pFile = fopen( fileNameC , "w" );
		if( NULL!=pFile )
		{
			count++;
			fprintf( pFile, "%u", count );
			fclose( pFile );
		}
		return 0;
	}
	else
	{
		pFile = fopen( fileNameR, "r" );
		if( NULL!=pFile )
		{    
			if(fgets( buff, sizeof(buff), pFile )!=NULL)
				reason = atoi(buff);
			fclose( pFile );
			return reason;
		}
		else
			return WALN_CHANGE_UNKNOWN;
	}   
}

unsigned int rtk_generic_update_chanchg_time(char *ifname, char write)
{
	FILE *pFile;
	char fileName[64], buff_time[16];
	time_t seconds=0;

	memset( buff_time, 0, sizeof(buff_time) );
	memset( fileName, 0, sizeof(fileName) );

	if( access(WLAN_STATS_FOLDER, 0))
		mkdir(WLAN_STATS_FOLDER, 0755);
	snprintf(fileName, sizeof(fileName), "%s/%s_channel_change", WLAN_STATS_FOLDER, ifname);
	if(write)
	{    
		pFile = fopen( fileName , "w" );
		if( NULL!=pFile )
		{    
			time(&seconds);
			fprintf( pFile, "%u", seconds );
			fclose( pFile );
		}    
		return 0;
	}    
	else 
	{    
		pFile = fopen( fileName, "r" );
		if( NULL!=pFile )
		{    
			if(fgets( buff_time, sizeof(buff_time), pFile )!=NULL)
				seconds = atoi(buff_time);
			fclose( pFile );
		}    
		return seconds;
	}    
}
#endif

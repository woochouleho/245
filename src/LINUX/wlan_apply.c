#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <signal.h>
#include "utility.h"


static void showHelp(char *cli)
{
	printf("usage: %s [commands]\n", cli);
	printf("commands:\n");
	printf("  restart = restart wlan.\n");
	printf("  help = show help.\n");
}


int main (int argc, char **argv)
{
	if(argc == 2){
		if(!strcmp(argv[1],"restart")){
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)
			update_wps_configured(0);
#endif
			config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
			goto end;
		}
		else
			goto show_help;
	}
show_help:
	showHelp(argv[0]);
	return -1;
end:
	return 0;
}

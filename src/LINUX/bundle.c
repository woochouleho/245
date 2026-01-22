#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include "utility.h"
#include <sys/stat.h>

void usage()
{
	printf("usage:\n");
	printf("bundle show\n");
	printf("bundle start [id]\n");
	printf("bundle stop [id]\n");
	printf("bundle restart [id]\n");
	printf("bundle uninstall [id]\n");
	printf("bundle install [path]\n");
	printf("bundle update [id] [path]\n");
}

int main(int argc, char **argv)
{
	MIB_CMCC_OSGI_PLUGIN_T plEntry;
	int index=0;
	int bundleId=0;
	struct stat st;
	OSGI_BUNDLE_INFO_Tp list=NULL;
	int entryNum=0, i;
	char cmd[256];
	char id_str[10];
	char level_str[10];
	int ret;
	
	if(argc >= 2)
	{
		int argi = 0;
		for (argi = 1 ; argi < argc; argi++)
		{
			if (strcmp(argv[argi], "show") == 0 && argc == 2)
			{
				listAllOsgiPlugin(&list, &entryNum);
				printf("----------------------------------------------------------------------------------------------------------------------------------------------\n");
				printf("%-5s%-42s%-45s%-20s%-12s%-7s\n", "ID", "Symbolic Name", "Plugin Name", "Version", "Status", "Level");
				printf("----------------------------------------------------------------------------------------------------------------------------------------------\n");
				for(i=0; i < entryNum; i++){
					sprintf(id_str, "%d", list[i].bundle_id);
					sprintf(level_str, "%d", list[i].level);
					printf("%-5s%-42s%-45s%-20s%-12s%-7s\n", id_str, list[i].symbolic_name, list[i].plugin_name, list[i].version, list[i].status, level_str);
				}

				if(list){
					free(list);
					list = NULL;
				}
				return 0;
			}
			else if(strcmp(argv[argi], "start") == 0 && argc == 3){
				va_cmd("/bin/sh", 4, 1, OSGI_PLUGIN_RUN, argv[argi+1], hideErrMsg1, hideErrMsg2);
				index = rtk_osgi_get_plugin_by_bundleid(atoi(argv[argi+1]), &plEntry);
				if(index >=0){
					plEntry.state=BUNDLE_STATE_ACTIVE;
					mib_chain_update(MIB_CMCC_OSGI_PLUGIN_TBL, &plEntry,index);
				}
				//printf("start bundle id %s\n", argv[argi+1]);
				return 0;
			}
			else if(strcmp(argv[argi], "stop") == 0 && argc == 3){
				va_cmd("/bin/sh", 4, 1, OSGI_PLUGIN_STOP, argv[argi+1], hideErrMsg1, hideErrMsg2);
				index = rtk_osgi_get_plugin_by_bundleid(atoi(argv[argi+1]), &plEntry);
				if(index >=0){
					plEntry.state=BUNDLE_STATE_RESOLVED;
					mib_chain_update(MIB_CMCC_OSGI_PLUGIN_TBL, &plEntry,index);
				}
				//printf("stop bundle id %s\n", argv[argi+1]);
				return 0;
			}
			else if(strcmp(argv[argi], "restart") == 0 && argc == 3){
				va_cmd("/bin/sh", 4, 1, OSGI_PLUGIN_RESTART, argv[argi+1], hideErrMsg1, hideErrMsg2);
				//printf("restart bundle id %s\n", argv[argi+1]);
				index = rtk_osgi_get_plugin_by_bundleid(atoi(argv[argi+1]), &plEntry);
				if(index >=0){
					plEntry.state=BUNDLE_STATE_ACTIVE;
					mib_chain_update(MIB_CMCC_OSGI_PLUGIN_TBL, &plEntry,index);
				}
				return 0;
			}
			else if(strcmp(argv[argi], "uninstall") == 0 && argc == 3){
				va_cmd("/bin/sh", 4, 1, OSGI_PLUGIN_UNINSTALL, argv[argi+1], hideErrMsg1, hideErrMsg2);
				index = rtk_osgi_get_plugin_by_bundleid(atoi(argv[argi+1]), &plEntry);
				if(index >=0){
					mib_chain_delete(MIB_CMCC_OSGI_PLUGIN_TBL, index);
				}
				//printf("uninstall bundle id %s\n", argv[argi+1]);
				return 0;
			}
			else if(strcmp(argv[argi], "install") == 0 && argc == 3){
				//check file
				ret = stat(argv[argi+1], &st);
				if(ret == -1 && errno == ENOENT){
					printf("file %s not exist!!\n",argv[argi+1]);
					return 1;
				}
				
				va_cmd("/bin/sh", 4, 1, OSGI_PLUGIN_INSTALL, argv[argi+1], hideErrMsg1, hideErrMsg2);
				if(rtk_osgi_is_bundle_action_timeout(&bundleId, argv[argi+1]) == 1){
					printf("%s:%d install timeout\n",__FUNCTION__, __LINE__);
					return 1;
				}
				printf("%s:%d bundle id is %d\n",__FUNCTION__, __LINE__, bundleId);
				sleep(1);
				listAllOsgiPlugin(&list, &entryNum);
				memset(&plEntry, 0, sizeof(MIB_CMCC_OSGI_PLUGIN_T));

				for(i=0; i < entryNum; i++){
					if(list[i].bundle_id == bundleId){
						printf("%s:%d bundle id %d, symbolic_name %s\n",__FUNCTION__, __LINE__, list[i].bundle_id, list[i].symbolic_name);
						plEntry.auto_start=1;
						plEntry.bundle_id = bundleId;
						strcpy(plEntry.os, "0");
						strcpy(plEntry.path, argv[argi+1]);
						//stat(plEntry.path, &st);
						plEntry.size=st.st_size;
						plEntry.state = BUNDLE_STATE_RESOLVED;
						strcpy(plEntry.symbolic_name, list[i].symbolic_name);
						strcpy(plEntry.version, list[i].version);
						mib_chain_add(MIB_CMCC_OSGI_PLUGIN_TBL, &plEntry);
						break;
					}
				}
					
				if(list){
					free(list);
					list = NULL;
				}
				//printf("install bundle path %s\n", argv[argi+1]);
				return 0;
			}
			else if(strcmp(argv[argi], "update") == 0 && argc == 4){
				//check file
				ret = stat(argv[argi+2], &st);
				if(ret == -1 && errno == ENOENT){
					printf("file %s not exist!!\n",argv[argi+2]);
					return 1;
				}
				va_cmd("/bin/sh", 5, 1, OSGI_PLUGIN_UPDATE, argv[argi+1], argv[argi+2], hideErrMsg1, hideErrMsg2);
				if(rtk_osgi_is_bundle_action_timeout(&bundleId, argv[argi+2]) == 1){
					printf("%s:%d install timeout\n",__FUNCTION__, __LINE__);
					return 1;
				}
				index = rtk_osgi_get_plugin_by_bundleid(atoi(argv[argi+1]), &plEntry);
				if(index < 0) memset(&plEntry, 0, sizeof(MIB_CMCC_OSGI_PLUGIN_T));
				sleep(1);
				listAllOsgiPlugin(&list, &entryNum);
				for(i=0; i < entryNum; i++){
					if(list[i].bundle_id == bundleId){
						printf("found list[i].bundle_id=%d\n",list[i].bundle_id);
						strcpy(plEntry.path, argv[argi+2]);
						//stat(plEntry.path, &st);
						plEntry.size=st.st_size;
						strcpy(plEntry.symbolic_name, list[i].symbolic_name);
						strcpy(plEntry.version, list[i].version);
						if(index < 0){
							plEntry.auto_start=1;
							plEntry.bundle_id = bundleId;
							strcpy(plEntry.os, "0");
							if(strcmp(list[i].status,"Active")==0 || strcmp(list[i].status,"Starting")==0)
								plEntry.state = BUNDLE_STATE_ACTIVE;
							else
								plEntry.state = BUNDLE_STATE_RESOLVED;
							mib_chain_add(MIB_CMCC_OSGI_PLUGIN_TBL, &plEntry);
						}
						else
							mib_chain_update(MIB_CMCC_OSGI_PLUGIN_TBL, &plEntry,index);
						break;
					}
				}
				if(list){
					free(list);
					list = NULL;
				}
				return 0;
			}
			else{
				usage();
			}
		}
	}
	else{
		usage();
	}
	return 0;
}

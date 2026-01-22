#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#include "mib.h"
#include "sysconfig.h"

static char *tmp_file = "/tmp/tmp.txt";
static int wlan_schl=0;
static int wlan_schkmulti[NUM_WLAN_INTERFACE]={0};
unsigned int SCHEDULE_MODE[NUM_WLAN_INTERFACE]={0};

#define WLAN_SCHEDULE_FILE "/var/wlsch.conf"
extern int wlan_idx;

#define SLEEP_TIME	3
#define MAX_WLAN_INT_NUM 2 /*we assume Max 2 WLAN interface */
static int pre_enable_state = -1;
static int pre_enable_state_new[MAX_WLAN_INT_NUM] = {-1,-1};

#if 0
#define DEBUG_PRINT printf

#else
#define DEBUG_PRINT
#endif

typedef struct ps_info {
	unsigned int fTime;
	unsigned int tTime;
	unsigned int action;
	unsigned int date;
}ps_info_T,*ps_info_Tp;

#if defined(TIMER_TASK_SUPPORT)
#define MAX_TASK_ACTION_LEN  		20
#define TIMER_TASK_STR_REBOOT 		"ToReboot"
#define TIMER_TASK_STR_HEALTHMODE   "ToSetHealthMode"
#define WRITE_FLASH_PROG			("flash")
#define PARAM_TEMP_FILE				("/tmp/task_timer_file")
#define MAX_TASK_SCHEDULE_NUM 		(10*7) 
#define SECONDS_OF_A_DAY    86400

typedef struct task_info {
	unsigned int fTime;	//unit:second
	unsigned int taskid;
	unsigned int date;  //0:Sun, 1:Mon, 2:Tue, ..., 6:Sat, 7:everyday
	unsigned char action[MAX_TASK_ACTION_LEN];
	unsigned int tTime;
	unsigned int index;
	unsigned char loop;	
}task_info_T, *task_info_Tp;
#endif

static int write_line_to_file(char *filename, int mode, char *line_data)
{
	unsigned char tmpbuf[512];
	int fh=0;

	if(mode == 1) {/* write line datato file */
		
		fh = open(filename, O_RDWR|O_CREAT|O_TRUNC);
		
	}else if(mode == 2){/*append line data to file*/
		
		fh = open(filename, O_RDWR|O_APPEND);	
	}
	
	if (fh < 0) {
		fprintf(stderr, "Create %s error!\n", filename);
		return 0;
	}
	sprintf((char *)tmpbuf, "%s", line_data);
	write(fh, tmpbuf, strlen((char *)tmpbuf));

	close(fh);
	return 1;
}
extern int set_wlan_idx_by_wlanIf(unsigned char * ifname,int *vwlan_idx);
static int rtk_wlan_set_wlan_enable(unsigned char *ifname, unsigned int enable)
{
	int ret = -1;
	int vwlan_idx;
	unsigned int intValue;
	char mibChar;
	MIB_CE_MBSSIB_T WlanEnable_Entry;
	if(ifname==NULL)
		return (-1);
	memset(&WlanEnable_Entry,0,sizeof(MIB_CE_MBSSIB_T));
	
	mib_save_wlanIdx();
	if((set_wlan_idx_by_wlanIf(ifname,&vwlan_idx))==(-1)){
		goto exit;
	}
	
	if(!mib_chain_get(MIB_MBSSIB_TBL, vwlan_idx, (void *)&WlanEnable_Entry))
		goto exit;
	
	if(enable==0){
		WlanEnable_Entry.wlanDisabled = 1;
		intValue = 1;
	}else if(enable==1){
		WlanEnable_Entry.wlanDisabled = 0;
		intValue = 0;
	}
	
	mib_chain_update(MIB_MBSSIB_TBL, (void *)&WlanEnable_Entry, vwlan_idx);		

	ret=0;

	// just for root
	if(vwlan_idx==0)
	{
		mibChar=(char)((unsigned int)intValue);
		if(!mib_set(MIB_WLAN_DISABLED,(void *)&mibChar))
			ret = (-1);
	}

	// for vxd
	intValue = 1;
	mibChar=(char)((unsigned int)intValue);	
	if(vwlan_idx==WLAN_REPEATER_ITF_INDEX)
	{
#ifdef WLAN_UNIVERSAL_REPEATER
		mib_set(MIB_REPEATER_ENABLED1, (void *)&mibChar);
#endif
	}
exit:
	mib_recov_wlanIdx();

	return ret; 
}

static int is_wlan_exist(int index)
{
	char tmpbuf[100], *ptr;
	char wlanIntname[10];
	FILE *fp;
	
	sprintf(tmpbuf, "ifconfig > %s", tmp_file);
	system(tmpbuf);
	
	fp = fopen(tmp_file, "r");
	if (fp == NULL)
		return 0;
	sprintf(wlanIntname,WLAN_IF,index);
	while (fgets(tmpbuf, 100, fp)) {
		ptr = strstr(tmpbuf, wlanIntname);	
		if (ptr) {
			if (strlen(ptr) <= 5)
				break;
			if (*(ptr+5) != '-')
				break;
		}
	}
	fclose(fp);

#if IS_AX_SUPPORT
	if(ptr)
	{
		if (!is_interface_run(wlanIntname))
		{
			ptr = NULL;
		}
	}
#endif

	return (ptr ? 1 : 0);
}

static void enable_wlan(int index) 
{
	int wlan_enabled;
	int i;
	char cmdBuffer[128];
	char wlan_ifname[32];
#ifdef CONFIG_RTL8186_KB
	int guest_enabled;
#else
	int repeader_enabled;
	int guest0_enabled;
	int guest1_enabled;
	int guest2_enabled;
	int guest3_enabled;
#endif
	unsigned char vChar;
	int vap_num;
//	DEBUG_PRINT("Enable wlan!\n");
	mib_get_s(MIB_WIFI_MODULE_DISABLED, (void *)&vChar, sizeof(vChar));
	if(vChar)
		return;


	sprintf(cmdBuffer,WLAN_IF,index);
	rtk_wlan_get_wlan_enable(cmdBuffer, &wlan_enabled);
	
	
#ifdef CONFIG_RTL8186_KB
	sprintf(cmdBuffer,VAP_IF,index,vap_num=0);
	rtk_wlan_get_wlan_enable(cmdBuffer, &guest_enabled);
#else
	sprintf(cmdBuffer,VAP_IF,index,vap_num=0);
	rtk_wlan_get_wlan_enable(cmdBuffer, &guest0_enabled);

	sprintf(cmdBuffer,VAP_IF,index,vap_num=1);
	rtk_wlan_get_wlan_enable(cmdBuffer, &guest1_enabled);
	
	sprintf(cmdBuffer,VAP_IF,index,vap_num=2);
	rtk_wlan_get_wlan_enable(cmdBuffer, &guest2_enabled);
	
	sprintf(cmdBuffer,VAP_IF,index,vap_num=3);
	rtk_wlan_get_wlan_enable(cmdBuffer, &guest3_enabled);
	
	sprintf(cmdBuffer,VXD_IF,index);
	rtk_wlan_get_wlan_enable(cmdBuffer, &repeader_enabled);
#endif

	if (wlan_enabled){
		
		sprintf(wlan_ifname,WLAN_IF,index);
		sprintf(cmdBuffer,"ifconfig %s up",wlan_ifname);
		system(cmdBuffer);
	}
#ifdef CONFIG_RTL8186_KB
	if (guest_enabled) 
	{
		sprintf(wlan_ifname,VAP_IF,index,vap_num=0);
		sprintf(cmdBuffer,"ifconfig %s up",wlan_ifname);
		system(cmdBuffer);
	}
#else	

	if (guest0_enabled) 
	{
		sprintf(wlan_ifname,VAP_IF,index,vap_num=0);
		sprintf(cmdBuffer,"ifconfig %s up",wlan_ifname);
		system(cmdBuffer); 
	}
	if (guest1_enabled) 
	{
		sprintf(wlan_ifname,VAP_IF,index,vap_num=1);
		sprintf(cmdBuffer,"ifconfig %s up",wlan_ifname);
		system(cmdBuffer); 
	}
	if (guest2_enabled) 
	{
		sprintf(wlan_ifname,VAP_IF,index,vap_num=2);
		sprintf(cmdBuffer,"ifconfig %s up",wlan_ifname);
		system(cmdBuffer); 
	}
	if (guest3_enabled) 
	{
		sprintf(wlan_ifname,VAP_IF,index,vap_num=3);
		sprintf(cmdBuffer,"ifconfig %s up",wlan_ifname);
		system(cmdBuffer); 
	}
	if (repeader_enabled) 
	{
		sprintf(wlan_ifname,VXD_IF,index);
		sprintf(cmdBuffer,"ifconfig %s up",wlan_ifname);
		system(cmdBuffer); 
	}
#endif	
}

static void disable_wlan(int index)
{
	int wlan_enabled;
	int i;
	char cmdBuffer[128];
	char wlan_ifname[32];
#ifdef CONFIG_RTL8186_KB
	int guest_enabled;
#else
	int guest0_enabled;
	int guest1_enabled;
	int guest2_enabled;
	int guest3_enabled;
	int repeader_enabled;
#endif
	int vap_num;

	sprintf(cmdBuffer,WLAN_IF,index);
	rtk_wlan_get_wlan_enable(cmdBuffer, &wlan_enabled);

#ifdef CONFIG_RTL8186_KB
	sprintf(cmdBuffer,VAP_IF,index,vap_num=0);
	rtk_wlan_get_wlan_enable(cmdBuffer, &guest_enabled);
#else
	sprintf(cmdBuffer,VAP_IF,index,vap_num=0);
	rtk_wlan_get_wlan_enable(cmdBuffer, &guest0_enabled);

	sprintf(cmdBuffer,VAP_IF,index,vap_num=1);
	rtk_wlan_get_wlan_enable(cmdBuffer, &guest1_enabled);
	
	sprintf(cmdBuffer,VAP_IF,index,vap_num=2);
	rtk_wlan_get_wlan_enable(cmdBuffer, &guest2_enabled);
	
	sprintf(cmdBuffer,VAP_IF,index,vap_num=3);
	rtk_wlan_get_wlan_enable(cmdBuffer, &guest3_enabled);
	
	sprintf(cmdBuffer,VXD_IF,index);
	rtk_wlan_get_wlan_enable(cmdBuffer, &repeader_enabled);
#endif


	if (wlan_enabled) {
		sprintf(wlan_ifname,WLAN_IF,index);
		sprintf(cmdBuffer,"iwpriv %s set_mib keep_rsnie=1",wlan_ifname);
		system(cmdBuffer);

		sprintf(cmdBuffer,"ifconfig %s down",wlan_ifname);
		system(cmdBuffer);
	}

#ifdef CONFIG_RTL8186_KB
	if (guest_enabled) {
		sprintf(wlan_ifname,VAP_IF,index,vap_num=0);
		sprintf(cmdBuffer,"iwpriv %s set_mib keep_rsnie=1",wlan_ifname);
		system(cmdBuffer);

		sprintf(cmdBuffer,"ifconfig %s down",wlan_ifname);
		system(cmdBuffer);
	}
#else
	if (guest0_enabled) {
		sprintf(wlan_ifname,VAP_IF,index,vap_num=0);
		sprintf(cmdBuffer,"iwpriv %s set_mib keep_rsnie=1",wlan_ifname);		
		system(cmdBuffer);
		
		sprintf(cmdBuffer,"ifconfig %s down",wlan_ifname);
		system(cmdBuffer);
	}

	if (guest1_enabled) {
		sprintf(wlan_ifname,VAP_IF,index,vap_num=1);
		sprintf(cmdBuffer,"iwpriv %s set_mib keep_rsnie=1",wlan_ifname);
		system(cmdBuffer);

		sprintf(cmdBuffer,"ifconfig %s down",wlan_ifname);
		system(cmdBuffer);
	}

	if (guest2_enabled) {
		sprintf(wlan_ifname,VAP_IF,index,vap_num=2);
		sprintf(cmdBuffer,"iwpriv %s set_mib keep_rsnie=1",wlan_ifname);		
		system(cmdBuffer);

		sprintf(cmdBuffer,"ifconfig %s down",wlan_ifname);
		system(cmdBuffer);
	}

	if (guest3_enabled) {
		sprintf(wlan_ifname,VAP_IF,index,vap_num=3);
		sprintf(cmdBuffer,"iwpriv %s set_mib keep_rsnie=1",wlan_ifname);
		system(cmdBuffer);

		sprintf(cmdBuffer,"ifconfig %s down",wlan_ifname);
		system(cmdBuffer);
	}

	if (repeader_enabled) {
		sprintf(wlan_ifname,VXD_IF,index);
		sprintf(cmdBuffer,"iwpriv %s set_mib keep_rsnie=1",wlan_ifname);
		system(cmdBuffer);

		sprintf(cmdBuffer,"ifconfig %s down",wlan_ifname);
		system(cmdBuffer);
	}
#endif	
}
//andlink start
#if ((defined(CONFIG_ANDLINK_IF5_SUPPORT)||defined(CONFIG_USER_ANDLINK_PLUGIN))&&(defined(TIMER_TASK_SUPPORT)))
#define BIT(x)			(1<<x)
#define MAX_TASK_BITS 	7

int andlink_change_tbl_staus(int index)
{
	int count=0, i=0, ret = 0, index_tmp = 0;
	MIB_CE_WLAN_TIMER_TASK_ENTRY_T entry, entry_new;

	count = mib_chain_total(MIB_TIMER_TASK_TBL);
	for(i=0; i<count; i++){
		if(!mib_chain_get(MIB_TIMER_TASK_TBL, i, &entry)){
			printf("[%s %d] get timer task tbl error!\n",__FUNCTION__,__LINE__);
			ret = -1;
			goto EXIT;
		}
		
		index_tmp = atoi(entry.taskid);
		if(index_tmp == index){
			printf("[%s %d] update %d's enable status\n",__FUNCTION__,__LINE__, index);
			memset(&entry_new, 0, sizeof(MIB_CE_WLAN_TIMER_TASK_ENTRY_T));
			memcpy(&entry_new, &entry, sizeof(MIB_CE_WLAN_TIMER_TASK_ENTRY_T));
			entry_new.enable = 0;
			mib_chain_update(MIB_TIMER_TASK_TBL, (void *)&entry_new, i);
			mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
			break;
		}
	}

EXIT:
	return ret;
}

void andlink_setup_tasktimer()
{
	char filename[50]={0}, line_buffer[100]={0};
	int entry_num=0, idx, newfile=1, date, start, end, nextline=0, i;
	//WLAN_TIMER_TASK_ENTRY_T timer_task_entry;
	MIB_CE_WLAN_TIMER_TASK_ENTRY_T timer_task_entry;
	int cnt = 0, cmd_loop=0;	

	snprintf(filename, sizeof(filename), "%s", "/var/wlsch.conf.task");

	//mib_get(MIB_TIMER_TASK_NUM, (void *)&entry_num);	
	entry_num = mib_chain_total(MIB_TIMER_TASK_TBL);
	for(idx = 0; idx < entry_num; idx++){
		memset(&timer_task_entry, 0, sizeof(MIB_CE_WLAN_TIMER_TASK_ENTRY_T));
		memset(line_buffer, 0, sizeof(line_buffer));
		
		//*((char *)&timer_task_entry) = (char)idx;
		mib_chain_get(MIB_TIMER_TASK_TBL, idx,(void *)&timer_task_entry);
				
		if(timer_task_entry.enable){
			if(timer_task_entry.enable==1)
				cmd_loop = 1;
			else if(timer_task_entry.enable==2)
				cmd_loop = 0;		
			
			//taskid, week, timeoffset, action, timeroffset2, index, command loop or not
			if(timer_task_entry.week){
				for(i=0; i<MAX_TASK_BITS; ++i){
					if(timer_task_entry.week & BIT(i)){
						nextline = 0;
						date = i; //0:Sun, 1:Mon, 2:Tue, 3:Wen, 4:Thu, 5:Fri, 6:Sat
						cnt++;

						//if timeoffset2>=86400, divided it into two entries
						start = timer_task_entry.timeoffset;
						if(timer_task_entry.timeoffset2 >= SECONDS_OF_A_DAY){
							nextline = 1;
							end = SECONDS_OF_A_DAY-1;
						}else{
							end = timer_task_entry.timeoffset2;
						}
						//entry idx, taskid, week, start, end, action, index, loop  
						sprintf(line_buffer,"%d,%d,%d,%d,%d,%s,%d,%d\n", 
								cnt, atoi(timer_task_entry.taskid), date, start, end, timer_task_entry.action, 
								timer_task_entry.idxnum, cmd_loop);
						write_line_to_file(filename, (newfile==1?1:2), line_buffer);	
						newfile = 2;
						if(nextline){
							if(date <= 5)
								date++;
							else
								date = 0;
							cnt++;
							start = 0;
							end = timer_task_entry.timeoffset2 - SECONDS_OF_A_DAY;
							sprintf(line_buffer,"%d,%d,%d,%d,%d,%s,%d,%d\n", 
									cnt, atoi(timer_task_entry.taskid), date, start, end, timer_task_entry.action, 
									timer_task_entry.idxnum, cmd_loop);
							write_line_to_file(filename, (newfile==1?1:2), line_buffer);	
						}
					}					
				}
			}			
		}		
	}

	return;
}
int andlink_get_tasktimer_from_file(task_info_Tp tasks)
{
	FILE *fp;
	unsigned char *tmpfilename="/var/wlsch.conf.task";
	int idx1=0,taskSchk = 0;
	char *str1, *lineptr1;
	char line[200];

	fp = fopen(tmpfilename, "r");
	if (fp == NULL)
	{
		printf("file %s not exist!\n", tmpfilename);										
	}
	else
	{		
		while ( fgets(line, sizeof(line), fp) )
		{
			//entry idx, taskid, week, start, end, action, index, loop
			if(strlen(line) != 0)
			{
				lineptr1 = line;
				
				str1 = strsep(&lineptr1,",");
				idx1 = atoi(str1);
				idx1--;

				tasks[idx1].taskid = atoi(strsep(&lineptr1,","));
				tasks[idx1].date = atoi(strsep(&lineptr1,","));
				tasks[idx1].fTime = atoi(strsep(&lineptr1,","));
				tasks[idx1].tTime = atoi(strsep(&lineptr1,","));
				strncpy(tasks[idx1].action, strsep(&lineptr1,","), MAX_TASK_ACTION_LEN-1);
				tasks[idx1].index = atoi(strsep(&lineptr1,","));
				tasks[idx1].loop = atoi(strsep(&lineptr1,","));
			
				DEBUG_PRINT("	idx=[%u],taskid=[%d],date=[%u],fTime=[%u],tTime=[%u],action=[%s],index=[%u],loop=[%d]__[%s-%u]\n",
					idx1,tasks[idx1].taskid, tasks[idx1].date,tasks[idx1].fTime,tasks[idx1].tTime,tasks[idx1].action,tasks[idx1].index,tasks[idx1].loop,
					__FILE__,__LINE__);
				
				taskSchk = 1;
			}
		}
		fclose(fp);
	}

	return taskSchk;
}
void chk_taskSch_ksRule(task_info_Tp ks)
{
	int i=0;
	time_t tm;
	struct tm tm_time;
	task_info_Tp ps;
	int hit_date = 0,hit_time = 0, start, end, current, hit_idx=0;
	unsigned char cmd[MAX_TASK_ACTION_LEN+10]={0}, tmpbuf[100]={0};	
	int max_sec_offset = SLEEP_TIME + 1;
	FILE *fp;

	time(&tm);
	memcpy(&tm_time, localtime(&tm), sizeof(tm_time));
	
	DEBUG_PRINT(" tm_wday=%d, tm_hour=%d, tm_min=%d, tm_sec=%d\n", tm_time.tm_wday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);	

	memset(ps,0,sizeof(task_info_T));
	for(i=0 ; i<MAX_TASK_SCHEDULE_NUM ; i++)
	{	
		if(ks[i].date > 7) // this item is null
		{
			continue;
		}
		else
		{	
			hit_time = 0;
			hit_date = 0;
			ps = &ks[i];

			if(ps->date == tm_time.tm_wday)
			{
				hit_date = 1;
			}
			else
				continue;
			
			
			DEBUG_PRINT("\n ps.date = [%u], ps.fTime = [%u], ps.tTime = [%u], ps.action = [%s], ps.index = [%u], ps.loop = [%d]__[%s-%u]\n",
			ps->date, ps->fTime, ps->tTime, ps->action, ps->index, ps->loop, __FILE__,__LINE__);
 
			start = ps->fTime;
			end = ps->tTime;
			current = (tm_time.tm_hour * 60 + tm_time.tm_min) * 60 + tm_time.tm_sec;
			
			DEBUG_PRINT("start=%d, end=%d, current=%d\n", start, end, current);

			//if(current==target_time)
			if(!strncmp(ps->action, TIMER_TASK_STR_REBOOT, sizeof(TIMER_TASK_STR_REBOOT)-1)){
				if(((start>=current) && (start-current<=max_sec_offset)) ||
					((start<current) && (current-start<=max_sec_offset)))
					hit_time = 1;	
			}else if(!strncmp(ps->action, TIMER_TASK_STR_HEALTHMODE, sizeof(TIMER_TASK_STR_HEALTHMODE)-1)){
				if(end >= start){
					if ((current >= start) && (current < end))
						hit_time = 1;
				}else {
				if ((current >= start) || (current < end))
					hit_time = 1;
				}	
				
				if(!hit_date || !hit_time){
					if(ps->index == 0){ //index:0 indicate all band
						if (!is_wlan_exist(0)){
							printf("[%s %d] set ifconfig wlan%d up\n",__FUNCTION__,__LINE__,0);
							enable_wlan(0);
						}
						if (!is_wlan_exist(1)){
							printf("[%s %d] set ifconfig wlan%d up\n",__FUNCTION__,__LINE__,1);
							enable_wlan(1);
						}
					}else if(ps->index >= 1 && ps->index<= 4){ //index:1-4 indicate 2.4G
						if (!is_wlan_exist(1)){
							printf("[%s %d] set ifconfig wlan%d up\n",__FUNCTION__,__LINE__,1);
							enable_wlan(1);
						}
					}else if(ps->index >= 5 && ps->index <= 8){ //index:5-8 indicate 5G
						if (!is_wlan_exist(0)){
							printf("[%s %d] set ifconfig wlan%d up\n",__FUNCTION__,__LINE__,0);
							enable_wlan(0);
						}
					}

					//action:ToSetHealthMode, no hit_date and hit_time, current > end, need hit_idx
					if((ps->loop == 0) && (current > end)){
						hit_idx = ps->taskid;
					}
					
					fp = fopen(PARAM_TEMP_FILE, "w");
					if(fp){
						//action, hit_idx, index, disable
						snprintf(cmd, sizeof(cmd), "%s,%d,%d,%d", TIMER_TASK_STR_HEALTHMODE, hit_idx, ps->index,0);
						fputs(cmd,fp);
						fclose(fp);
					}else{
						printf("create file %s fail\n", PARAM_TEMP_FILE);
						return;
					}

					snprintf(tmpbuf, sizeof(tmpbuf), "flash -file_task_timer %s", PARAM_TEMP_FILE);				
					//system(tmpbuf);	
					
				}
			}

			if(hit_date && hit_time){
				//for action To Reboot
				hit_idx = ps->taskid;
				break;		
			}	
		}
	}
	
	DEBUG_PRINT("hit_date=%d, hit_time=%d, action:%s, loop:%d\n", hit_date, hit_time, ps->action, ps->loop);	
	if(hit_date && hit_time){		
		if(!strncmp(ps->action, TIMER_TASK_STR_REBOOT, sizeof(TIMER_TASK_STR_REBOOT)-1)){
			// timer done just one time
			if(ps->loop==0){
				#if 0
				fp = fopen(PARAM_TEMP_FILE, "w");
				if(fp){
					snprintf(cmd, sizeof(cmd), "%s,%d,%d,%d", TIMER_TASK_STR_REBOOT, hit_idx, ps->index, 0);
					fputs(cmd,fp);
					fclose(fp);
				}else{
					printf("create file %s fail\n", PARAM_TEMP_FILE);
					return;
				}

				snprintf(tmpbuf, sizeof(tmpbuf), "flash -file_task_timer %s", PARAM_TEMP_FILE);				
				system(tmpbuf);
				#endif
				/***to set TIMER TASK TBL(hit_idx) enable as 0***/ 
				andlink_change_tbl_staus(hit_idx);
				sleep(1);
			}
			
			system("reboot");
		}else if(!strncmp(ps->action, TIMER_TASK_STR_HEALTHMODE, sizeof(TIMER_TASK_STR_HEALTHMODE)-1)){
			if(ps->index == 0){    //index:0 indicate all band
				if (is_wlan_exist(0)){
					printf("[%s %d] set ifconfig wlan%d and vap down\n",__FUNCTION__,__LINE__,0);
					disable_wlan(0);
				}
				if (is_wlan_exist(1)){
					printf("[%s %d] set ifconfig wlan%d and vap down\n",__FUNCTION__,__LINE__,1);
					disable_wlan(1);
				}
			}else if(ps->index >= 1 && ps->index <= 4){  //index:1-4 indicate 2.4G
				if (is_wlan_exist(1)){
					printf("[%s %d] set ifconfig wlan%d and vap down\n",__FUNCTION__,__LINE__,1);
					disable_wlan(1);
				}
			}else if(ps->index >= 5 && ps->index <= 8){ //index:5-8 indicate 5G
				if (is_wlan_exist(0)){
					printf("[%s %d] set ifconfig wlan%d and vap down\n",__FUNCTION__,__LINE__,0);
					disable_wlan(0);
				}
			}
			
			hit_idx = 0;
		
			fp = fopen(PARAM_TEMP_FILE, "w");
			if(fp){
				//action, hit_idx, index, disable
				snprintf(cmd, sizeof(cmd), "%s,%d,%d,%d", TIMER_TASK_STR_HEALTHMODE, hit_idx, ps->index,1);
				fputs(cmd,fp);
				fclose(fp);
			}else{
				printf("create file %s fail\n", PARAM_TEMP_FILE);
				return;
			}

			snprintf(tmpbuf, sizeof(tmpbuf), "flash -file_task_timer %s", PARAM_TEMP_FILE);				
			//system(tmpbuf);		

		}
	}
	
	return;
}
#endif
//andlink end

void dunpWlanScheduleMode()
{
	int index;
	unsigned int mode;
	
	mib_save_wlanIdx();
	for(index=0;index<NUM_WLAN_INTERFACE;index++)
	{
		wlan_idx=index;
		mib_get_s(MIB_WLAN_SCHEDULE_MODE,&mode,sizeof(mode));
		SCHEDULE_MODE[index]=mode;
	}
	mib_recov_wlanIdx();
}

void chk_wlanSch_ksRule(struct ps_info *ks, int index)
{
	int i=0;
	int start=0,end=0,current=0;
	time_t tm;
	struct tm tm_time;
	struct ps_info *ps;
	int hit_date = 0;
	int hit_time = 0;
	int action = 0;
	int pre_hit_time=0;
	unsigned int wlanScheduleMode=0;
	time(&tm);
	memcpy(&tm_time, localtime(&tm), sizeof(tm_time));
	DEBUG_PRINT(" tm_wday=%d, tm_hour=%d, tm_min=%d\n", tm_time.tm_wday, tm_time.tm_hour, tm_time.tm_min);	

	wlanScheduleMode=SCHEDULE_MODE[index];

	for(i=0 ; i<MAX_WLAN_SCHEDULE_NUM ; i++)
	{
		if(ks[i].date > 7) // this item is null
		{
			continue;
		}
		else
		{	
			//hit_time = 0;
			//hit_date = 0;
			ps = &ks[i];
			if(ps->date == 7) // 7:everyday
			{
				hit_date = 1;
			}
			else if(ps->date == tm_time.tm_wday)
			{
				hit_date = 1;
			}
			else
				continue;
			
			//DEBUG_PRINT("\r\n ps.date = [%u], ps.fTime = [%u], ps.tTime = [%u]__[%s-%u]\r\n",ps->date,ps->fTime,ps->tTime,__FILE__,__LINE__);

			start = ps->fTime;
			current = tm_time.tm_hour * 60 + tm_time.tm_min;
			if(wlanScheduleMode==ELINK_TIMER_MODE){
			DEBUG_PRINT("start=%d, current=%d, action=%d\n", start, current, action);
	
			if (current >= start){
				if(pre_hit_time){
					if(start > pre_hit_time){
						pre_hit_time = start;
						action = ps->action;
					}
				}else{						
					pre_hit_time = start;
					action = ps->action;
				}
				hit_time = 1;
			}else{
				if(pre_hit_time == 0){
					hit_time = 0;
				}
			}
			DEBUG_PRINT("(%s)%d pre_hit_time=%d,hit_time=%d,action=%d\n",__FUNCTION__,__LINE__,pre_hit_time,hit_time,action);		
		}								
			else if(wlanScheduleMode==DEFAULT_TIMER_MODE){
				hit_time=0;
				end = ps->tTime;
				if (end >= start) {
						if ((current >= start) && (current < end))
							hit_time = 1;
				}
				else {
					if ((current >= start) || (current < end))
						hit_time = 1;
				}	
				DEBUG_PRINT("start=%d, end=%d, current=%d hit_time=%d\n", start, end, current,hit_time);
				if(hit_date && hit_time)
					break;
			}
		}								
	}
	if(wlanScheduleMode==ELINK_TIMER_MODE)	{
		DEBUG_PRINT("(%s)%d MODE1 hit_date=%d, hit_time=%d,mode1_action=%d\n", __FUNCTION__,__LINE__,hit_date, hit_time,action);											
		if (hit_date && hit_time) {
			if(action){
				if (!is_wlan_exist(index))
					enable_wlan(index);
			}else{
				if (is_wlan_exist(index))
					disable_wlan(index);
			}
		}
	}
	else if(wlanScheduleMode==DEFAULT_TIMER_MODE){	
		DEBUG_PRINT("pre_enable_state=%d, hit_date=%d, hit_time=%d\n", pre_enable_state_new[index], hit_date, hit_time);											
		if (pre_enable_state_new[index] < 0) { // first time
			if (hit_date && hit_time) {
				if (is_wlan_exist(index))
					disable_wlan(index);
				pre_enable_state_new[index] = 1;
			}
			else {
				if (!is_wlan_exist(index))
					enable_wlan(index);
				pre_enable_state_new[index] = 0;
			}
		}
		else {
			if (!pre_enable_state_new[index] && hit_date && hit_time) {
				if (is_wlan_exist(index))
					disable_wlan(index);
				pre_enable_state_new[index] = 1;
			}
			else if (pre_enable_state_new[index] && (!hit_date || !hit_time)) {
				if (!is_wlan_exist(index))
					enable_wlan(index);
				pre_enable_state_new[index] = 0;
			}
		}
		DEBUG_PRINT("%s %d\n",__FUNCTION__,__LINE__);
		}
}
/*************************reinit func wlan*********************************/
void start_wlan_by_schedule(int index)
{
	int intValue=0,  i=0, bak_idx=0, bak_vidx=0;
	char tmp1[64]={0};
	char line_buffer[100]={0};
	char ifname[16]={0};
	MIB_CE_SCHEDULE_T wlan_sched;
	int newfile=1;
	int schedule_action = 0;
	unsigned int enable=0,entryNum=0;
	
	mib_save_wlanIdx();
	
	wlan_idx=index;
	static int vwlan_idx=0;
	
	memset(&wlan_sched,0x0,sizeof(MIB_CE_SCHEDULE_T));
	sprintf(ifname,WLAN_IF,index);
	rtk_wlan_get_wlan_enable(ifname, &intValue);
	
	sprintf(tmp1,WLAN_SCHEDULE_FILE"%d",index);
	unlink(tmp1);

	if(intValue==1){
		mib_get_s(MIB_WLAN_SCHEDULE_ENABLED, (void *)&enable,sizeof(enable));
		mib_get_s(MIB_WLAN_SCHEDULE_TBL_NUM, (void *)&entryNum,sizeof(entryNum));
		
		if(enable==1 && entryNum > 0){
			
			wlan_schl=1;
			for (i=0; i<entryNum; i++) {
				
				if(!mib_chain_get(MIB_WLAN_SCHEDULE_TBL, i,(void *)&wlan_sched))
					DEBUG_PRINT("[%s %d]get mib tbl error!\n",__FUNCTION__,__LINE__);
				if(((wlan_sched.eco & ECO_MODE_MASK)==0x10) && (wlan_sched.eco&0x1 == 1)) //&& (wlan_sched.tTime == 0)
				{	
					//wlanScheduleMode=1;
					if((wlan_sched.eco & ECO_ACTION_MASK)==0x20)
						schedule_action = 1;
					else
						schedule_action = 0;
					sprintf(line_buffer,"%d,%d,%d,%d\n",i,wlan_sched.day,wlan_sched.fTime, schedule_action);
					sprintf(tmp1,WLAN_SCHEDULE_FILE"%d",index);
					write_line_to_file(tmp1, (newfile==1?1:2), line_buffer);
					newfile = 2;	
				}
				else if(((wlan_sched.eco & ECO_MODE_MASK)==0) && (wlan_sched.eco & 0x1 == 1) && !(wlan_sched.tTime == 0))
				{
					//wlanScheduleMode=0;
					sprintf(line_buffer,"%d,%d,%d,%d\n",i,wlan_sched.day,wlan_sched.fTime, wlan_sched.tTime);
					sprintf(tmp1,WLAN_SCHEDULE_FILE"%d",index);
					write_line_to_file(tmp1, (newfile==1?1:2), line_buffer);
					newfile = 2;	
				}		
			}
		}
	}
	mib_recov_wlanIdx();
}
void setup_wlan_schedule()
{
	int index;
	for(index=0; index<NUM_WLAN_INTERFACE; index++)
		start_wlan_by_schedule(index);
}

/**************************************************************************/
int main(int argc, char *argv[])
{
	FILE *fp;
	int i=0,j=0;
	char line[200];
	unsigned char *filename="/var/wlsch.conf";
	unsigned char tmpfilename[20];
	unsigned int wlanScheduleMode=0;
	
	struct ps_info ks[NUM_WLAN_INTERFACE][MAX_WLAN_SCHEDULE_NUM];
	static int  wlan_schk=0;
	
#if defined(TIMER_TASK_SUPPORT) 
	task_info_T tasks[MAX_TASK_SCHEDULE_NUM];
	static int task_schk = 0;

	memset(tasks, 0, sizeof(task_info_T)*MAX_TASK_SCHEDULE_NUM);
#endif

	dunpWlanScheduleMode();//get Mode
	
#if ((defined(CONFIG_ANDLINK_IF5_SUPPORT)||defined(CONFIG_USER_ANDLINK_PLUGIN))&&(defined(TIMER_TASK_SUPPORT)))
	andlink_setup_tasktimer();//read tbl and write file

	task_schk=andlink_get_tasktimer_from_file(tasks);
	
	while(1)
	{
		if(task_schk==1)
			chk_taskSch_ksRule(tasks);
		else
			break;
		
		sleep(SLEEP_TIME);
	}
#else
	setup_wlan_schedule();//check wlan schedule tbl ,write to file

	memset(ks, 0xff, sizeof(ps_info_T)*MAX_WLAN_SCHEDULE_NUM*NUM_WLAN_INTERFACE);
			
	for(j=0;j<NUM_WLAN_INTERFACE;j++)
	{	
		wlanScheduleMode=SCHEDULE_MODE[j];
		
		sprintf(tmpfilename,"%s%d",filename,j);
		DEBUG_PRINT("filename (%s)\n",tmpfilename);
		fp = fopen(tmpfilename, "r");
		if (fp == NULL)
		{
			DEBUG_PRINT("open wlan schedule file failed!\n");
		}
		else
		{
			while(fgets(line, 200, fp))
			{
				int idx;
				char *str;
				if(strlen(line) != 0)
				{
					char *lineptr = line;
					//strcat(line,",");
					
					str = strsep(&lineptr,",");
													
					idx = atoi(str);

					//idx--;
					
					ks[j][idx].date = atoi(strsep(&lineptr,","));
					ks[j][idx].fTime = atoi(strsep(&lineptr,","));
					if(wlanScheduleMode==ELINK_TIMER_MODE)
					ks[j][idx].action  = atoi(strsep(&lineptr,","));
					else if(wlanScheduleMode==DEFAULT_TIMER_MODE)
						ks[j][idx].tTime  = atoi(strsep(&lineptr,","));
					
					DEBUG_PRINT("[%s %d] ks[%d][%d].date:%d  ks[%d][%d].fTime:%d  ks[%d][%d].action:%d \n",__FUNCTION__,__LINE__,j,idx,ks[j][idx].date,j,idx,ks[j][idx].fTime,j,idx,ks[j][idx].action);

					wlan_schk = 1;
					wlan_schkmulti[j]=1;
					idx++;
				}
			}
			if(fp)
				fclose(fp);
		}
	}

	while(1)
	{		
		if((wlan_schk == 1)&&(wlan_schl==1))
		{
			for(i=0;i<NUM_WLAN_INTERFACE;i++)
			{
				if(wlan_schkmulti[i])
					chk_wlanSch_ksRule(ks[i],i);
			}	
		}else
			break;
						
		sleep(SLEEP_TIME);
	}
#endif

	return 0;
}
	

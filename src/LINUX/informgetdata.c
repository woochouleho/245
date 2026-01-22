#include "informgetdata.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sysinfo.h>
#include <sys/file.h>
#include <sys/time.h>
#include <errno.h>

static DeviceInfo device_info;
static StaInfo sta_info[64];
static int device_shm_id = -1;
static int station_shm_id = -1;
static StaInfo *sta_info_tmp = NULL;
static void *shm = NULL;
static int myoldpid = 0;

#define INFORMGET_INFO "/var/run/informget.lock"
static int lock_shm_by_flock()
{
	int lockfd;
	
	if ((lockfd = open(INFORMGET_INFO, O_RDWR | O_CREAT)) == -1) {
		perror("open shm lockfile");
		return lockfd;
	}
	while (flock(lockfd, LOCK_EX | LOCK_NB) == -1 && errno==EINTR) {
		printf("configd write lock failed by flock. errno=%d\n", errno);
	}
	return lockfd;
}

static int unlock_shm_by_flock(int lockfd)
{
	while (flock(lockfd, LOCK_UN) == -1 && errno==EINTR) {
		printf("configd write unlock failed by flock. errno=%d\n", errno);
	}
	if(lockfd != -1)
		close(lockfd);
	return 0;
}

int getStaInform(int StaInfoNum)
{
	//int shm_id;
	//StaInfo *sta_info_tmp;
	int i;

	//printf("%s %d station_shm_id=%d\n", __FUNCTION__, __LINE__, station_shm_id);
	if(station_shm_id == -1)
	{
		station_shm_id = shmget((key_t)0x1899, sizeof(StaInfo)*64, 0);
		if(station_shm_id == -1)
			return 0;
	
		sta_info_tmp = (StaInfo *)shmat(station_shm_id, NULL, 0);
	}

	if(sta_info_tmp == NULL)
		return -1;
	
	//printf("%s %d StaInfoNum=%d sta_info_tmp=%x\n", __FUNCTION__, __LINE__, StaInfoNum, sta_info_tmp);
	memset(sta_info, 0, sizeof(StaInfo)*64);
	for(i = 0; i < StaInfoNum; i++)
	{
		//printf("%s %d sta_info_tmp[%d]=0x%x &sta_info[i]=0x%x\n", __FUNCTION__, __LINE__, i, &sta_info_tmp[i], &sta_info[i]);
		memcpy(&sta_info[i], &sta_info_tmp[i], sizeof(StaInfo));
	}
	return 1;
}

int getDeviceInform(unsigned int *flag)
{
	//int shm_id;
	DeviceInfo *device_info_tmp;
	//printf("%s %d device_shm_id=%d\n", __FUNCTION__, __LINE__, device_shm_id);
	if(device_shm_id == -1)
	{
		device_shm_id = shmget((key_t)0x9981, (sizeof(unsigned int)+sizeof(DeviceInfo)), 0);
		if(device_shm_id == -1)
			return -1;

		shm = shmat(device_shm_id, NULL, 0);
		if(shm == (void *)(-1))
			return -1;
	}

	if(shm == NULL)
		return -1;
	
	flag = (unsigned int *)shm;
	//printf("getFlag 0X%x %d\n", shm, *flag);
	if(*flag != 1)
		return -1;
	
	device_info_tmp = (DeviceInfo *)((void*)shm + 4);
	//printf("getDeviceInfo 0x%x\n", device_info_tmp);
	
	if(device_info_tmp->StaInfoNum != 0)
	{
	//	printf("%s %d device_info_tmp->StaInfoNum %d\n", __FUNCTION__, __LINE__, device_info_tmp->StaInfoNum);
		if(getStaInform(device_info_tmp->StaInfoNum) == 0)
		{
			device_info_tmp->StaInfoNum = 0;
			device_info_tmp->pStaInfos = NULL;
		}
		else
			device_info_tmp->pStaInfos = sta_info;
	}
	else
		device_info_tmp->pStaInfos = NULL;
	
	memcpy(&device_info, device_info_tmp, sizeof(DeviceInfo));

	//clear flag
	*flag = 0;
	memcpy(shm, flag, sizeof(unsigned int));
	
	return 0;
}

int read_pid(const char *filename)
{
	FILE *fp;
	int pid = 0;

	if ((fp = fopen(filename, "r")) == NULL)
		return -1;
	if(fscanf(fp, "%d", &pid) != 1 || kill(pid, 0) != 0){
		pid = 0;
	}
	fclose(fp);

	return pid;
}

int InformGetData_func(DeviceInfo **data)
{
	int mypid;
	//struct sysinfo systeminfo;
	//long time_start;
	int fd;
	struct timeval tv;
	unsigned long time_start, time_now; 
	
	mypid = read_pid("/var/run/informgetd.pid");
	if(mypid >= 1)
	{
		if(mypid != myoldpid)
		{
			printf("mypid=%d\n", mypid);
			device_shm_id = -1;
			station_shm_id = -1;
			myoldpid = mypid;
		}
		kill(mypid, SIGUSR1);
	}
	else
	{
		printf("read mypid fail\n");
		*data = NULL;
		return -1;
	}
	
	//sysinfo(&systeminfo);
	//time_start=systeminfo.uptime;
	//printf("%s %d time_start=%lu\n", __FUNCTION__, __LINE__, time_start);
	gettimeofday(&tv, NULL);
	time_start = tv.tv_sec*1000 + tv.tv_usec/1000;
	
	while(1)
	{
		//sysinfo(&systeminfo);

		unsigned int *flag = NULL;
		fd = lock_shm_by_flock();
		if(getDeviceInform(flag) == 0)
		{
			*data = &device_info;
			unlock_shm_by_flock(fd);
			return 0;
		}
		unlock_shm_by_flock(fd);
		
		//if(systeminfo.uptime-time_start>5)
		gettimeofday(&tv, NULL);
		time_now = tv.tv_sec*1000 + tv.tv_usec/1000;
		if(time_now - time_start > 500)
		{	
			*data = &device_info;
			return 0;
		}
	}
	
	return 0;
}

int InformReleaseData_func(DeviceInfo *data)
{
	//memset(&device_info, 0, sizeof(DeviceInfo));
	//memset(sta_info, 0, sizeof(StaInfo)*64);
	return 0 ;
}

#include <netinet/in.h>
#include <linux/config.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/file.h>
#include "mib.h"
#include "utility.h"
#include "subr_net.h"
#include "sockmark_define.h"

#ifndef IFNAMSIZ
#define	IFNAMSIZ	32
#endif

#if 1
#define SockmarkLock "/var/run/sockmarkLock"
#define LOCK_SOCKMARK()	\
int lockfd; \
do {	\
	if ((lockfd = open(SockmarkLock, O_RDWR)) == -1) {	\
		perror("open SOCKMARK lockfile");	\
		return 0;	\
	}	\
	while (flock(lockfd, LOCK_EX)) { \
		if (errno != EINTR) \
			break; \
	}	\
} while (0)

#define UNLOCK_SOCKMARK()	\
do {	\
	flock(lockfd, LOCK_UN);	\
	close(lockfd);	\
} while (0)
#else
#define LOCK_SOCKMARK()
#define UNLOCK_SOCKMARK()
#endif

const char WAN_SOCKMARK_FILE[] = "/var/wan_sockmark_file";
const char WAN_SOCKMARK_TMP_FILE[] = "/var/wan_sockmark_file_temp";

static int rtk_wan_sockmark_check(unsigned int mark)
{
	unsigned int sockmark_f, type_f;
	char line[1024], ifname_f[64];
	int refcnt_f, tblid_f, refcnt_f2, ret2;
	FILE *fp;

	if (!(mark&SOCK_MARK_WAN_INDEX_BIT_MASK)) {
		printf("%s-%d: not in WAN sockmark range.\n", __func__, __LINE__);
		return 0;
	}

	if(!(fp = fopen(WAN_SOCKMARK_FILE, "r"))) {
		return 0;
	}

	while(fgets(line, sizeof(line)-1, fp) != NULL)
	{
		ret2 = sscanf(line, "ifname=%s sockmark=0x%x tblid=%d refcnt=%d refcnt2=%d type=0x%x\n", &ifname_f[0], &sockmark_f, &tblid_f, &refcnt_f, &refcnt_f2, &type_f);
		if(ret2==6 && mark == sockmark_f)
		{
			//AUG_PRT("ifname=%s has been registered as sockmark=%0x%x.\n", ifname_f, sockmark);
			fclose(fp);
			return 1;
		}
	}

	fclose(fp);
	return 0;
}

static unsigned int rtk_wan_sockmark_get_available(void)
{
	unsigned int sockmarkValue;

	for (sockmarkValue=SOCK_MARK_WAN_INDEX_TO_MARK(1) ; sockmarkValue<=SOCK_MARK_WAN_INDEX_BIT_MASK ; sockmarkValue += (1<<SOCK_MARK_WAN_INDEX_START))
	{
		if (rtk_wan_sockmark_check(sockmarkValue) == 1)
			continue;

		return sockmarkValue;
	}

	return 0;
}

static int rtk_wan_sockmark_inc_refcnt(char *ifname, unsigned int *mark, unsigned int type)
{
	unsigned int sockmark_f, type_f;
	char line[1024], ifname_f[64];
	int refcnt_f, tblid_f, refcnt_f2, ret2;
	FILE *fp, *fp_tmp;
	int ret = 1;

	if (ifname == NULL) {
		printf("%s-%d: ifname is NULL.\n", __func__, __LINE__);
		return -1;
	}

	if(!(fp = fopen(WAN_SOCKMARK_FILE, "r"))) {
		return -1;
	}

	if(!(fp_tmp = fopen(WAN_SOCKMARK_TMP_FILE, "w"))) {
		fclose(fp);
		return -1;
	}

	while(fgets(line, sizeof(line)-1, fp) != NULL)
	{
		ret2 = sscanf(line, "ifname=%s sockmark=0x%x tblid=%d refcnt=%d refcnt2=%d type=0x%x\n", &ifname_f[0], &sockmark_f, &tblid_f, &refcnt_f, &refcnt_f2, &type_f);
		if(ret2 == 6 && !strcmp(ifname, ifname_f))
		{
			if(type & WANMARK_TYPE_APP)
				refcnt_f2++;
			else 
				refcnt_f++;
			
			type_f |= type;
			if(mark) *mark = sockmark_f;
			ret = 0;
		}
		fprintf(fp_tmp, "ifname=%s sockmark=0x%x tblid=%d refcnt=%d refcnt2=%d type=0x%x\n", ifname_f, sockmark_f, tblid_f, refcnt_f, refcnt_f2, type_f);	
	}

	fclose(fp);
	fclose(fp_tmp);
	unlink(WAN_SOCKMARK_FILE);
	if(rename(WAN_SOCKMARK_TMP_FILE, WAN_SOCKMARK_FILE) != 0)
	{
		printf("%s-%d: unable to rename the file\n", __func__, __LINE__);
	}
	return ret;
}

static int rtk_wan_sockmark_dec_refcnt(char *ifname, unsigned int type)
{
	unsigned int sockmark_f, type_f;
	char line[1024], ifname_f[64];
	int refcnt_f, tblid_f, refcnt_f2, ret2;
	FILE *fp, *fp_tmp;
	int ret = 1;

	if (ifname == NULL) {
		printf("%s-%d: ifname is NULL.\n", __func__, __LINE__);
		return -1;
	}

	if(!(fp = fopen(WAN_SOCKMARK_FILE, "r"))) {
		return -1;
	}

	if(!(fp_tmp = fopen(WAN_SOCKMARK_TMP_FILE, "w"))) {
		fclose(fp);
		return -1;
	}
	
	while(fgets(line, sizeof(line)-1, fp) != NULL)
	{
		ret2 = sscanf(line, "ifname=%s sockmark=0x%x tblid=%d refcnt=%d refcnt2=%d type=0x%x\n", &ifname_f[0], &sockmark_f, &tblid_f, &refcnt_f, &refcnt_f2, &type_f);
		if(ret2 == 6 && !strcmp(ifname, ifname_f))
		{
			if(type & WANMARK_TYPE_APP) {
				refcnt_f2 = ((refcnt_f2-1)<0)?0:(refcnt_f2-1);
				if(refcnt_f2 <= 0) type_f &= ~type;
			}
			else { 
				refcnt_f = ((refcnt_f-1)<0)?0:(refcnt_f-1);
				if(refcnt_f <= 0) type_f &= ~type;
			}

			if((refcnt_f + refcnt_f2) <= 0)
				ret = 0;
			else {
				fprintf(fp_tmp, "ifname=%s sockmark=0x%x tblid=%d refcnt=%d refcnt2=%d type=0x%x\n", ifname_f, sockmark_f, tblid_f, refcnt_f, refcnt_f2, type_f);	
				ret = 1;
			}
		}
		else {
			fprintf(fp_tmp, "ifname=%s sockmark=0x%x tblid=%d refcnt=%d refcnt2=%d type=0x%x\n", ifname_f, sockmark_f, tblid_f, refcnt_f, refcnt_f2, type_f);
		}
	}

	fclose(fp);
	fclose(fp_tmp);
	unlink(WAN_SOCKMARK_FILE);
	if(rename(WAN_SOCKMARK_TMP_FILE, WAN_SOCKMARK_FILE) != 0)
	{
		printf("%s-%d: unable to rename the file\n", __func__, __LINE__);
	}
	return ret;
}

static int rtk_wan_sockmark_create(char *ifname, unsigned int *mark, unsigned int type)
{
	unsigned int sockmark, type_f=0;
	int refcnt_f=0, tblid_f=0, refcnt_f2=0;
	FILE *fp;

	if (ifname == NULL) {
		printf("%s-%d: ifname is NULL.\n", __func__, __LINE__);
		return -1;
	}

	sockmark=rtk_wan_sockmark_get_available();
	if (sockmark <= 0) {
		printf("%s-%d: no available sockmark.\n", __func__, __LINE__);
 		return -1;
	}

	if(!(fp = fopen(WAN_SOCKMARK_FILE, "a"))) {
		return -1;
	}
	
	if(type & WANMARK_TYPE_APP)
		refcnt_f2++;
	else 
		refcnt_f++;
	type_f |= type;
	tblid_f = WAN_POLICY_ROUTE_TBLID_START+(sockmark>>SOCK_MARK_WAN_INDEX_START)-1;

	fprintf(fp, "ifname=%s sockmark=0x%x tblid=%d refcnt=%d refcnt2=%d type=0x%x\n", ifname, sockmark, tblid_f, refcnt_f, refcnt_f2, type_f);
	fclose(fp);
	
	if(mark) *mark = sockmark;
	return 0;
}

int rtk_wan_sockmark_remove(char *ifname, unsigned int type)
{
	int ret;
	if (ifname == NULL || ifname[0] == '\0') {
		printf("%s-%d: ifname is NULL.\n", __func__, __LINE__);
		return -1;
	}
	LOCK_SOCKMARK();
	ret = rtk_wan_sockmark_dec_refcnt(ifname, type);
	UNLOCK_SOCKMARK();
	return ret;
}

int rtk_wan_sockmark_add(char *ifname, unsigned int *mark, unsigned int *mask, unsigned int type)
{
	int ret = -1;
	if (ifname == NULL || ifname[0] == '\0') {
		printf("%s-%d: ifname is NULL.\n", __func__, __LINE__);
		return -1;
	}

	LOCK_SOCKMARK();
	//AUG_PRT("ifname=%s\n", ifname);
	if(rtk_wan_sockmark_inc_refcnt(ifname, mark, type) == 0 || 
		rtk_wan_sockmark_create(ifname, mark, type) == 0) 
	{
		if(mask) *mask = SOCK_MARK_WAN_INDEX_BIT_MASK;
		ret = 0;
	}
	
	UNLOCK_SOCKMARK();
	
	return ret; 
}

int rtk_wan_sockmark_tblid_get(char *ifname, unsigned int *mark, unsigned int *mask, int *table, unsigned int *type)
{
	unsigned int sockmark_f, type_f;
	char line[1024], ifname_f[64];
	int refcnt_f, tblid_f, refcnt_f2, ret2, ret = -1;
	FILE *fp;

	if (ifname == NULL || ifname[0] == '\0') {
		printf("%s-%d: ifname is NULL.\n", __func__, __LINE__);
		return -1;
	}

	LOCK_SOCKMARK();
	//AUG_PRT("ifname=%s\n", ifname);
	
	if(!(fp = fopen(WAN_SOCKMARK_FILE, "r"))) {
		UNLOCK_SOCKMARK();
		return -1;
	}

	while(fgets(line, sizeof(line)-1, fp) != NULL)
	{
		ret2 = sscanf(line, "ifname=%s sockmark=0x%x tblid=%d refcnt=%d refcnt2=%d type=0x%x\n", &ifname_f[0], &sockmark_f, &tblid_f, &refcnt_f, &refcnt_f2, &type_f);
		if(ret2 == 6 && !strcmp(ifname, ifname_f))
		{
			//AUG_PRT("ifname=%s has been registered as sockmark=%0x%x.\n", &ifname_f[0], &sockmark_f);
			if(mark) *mark = sockmark_f;
			if(mask) *mask = SOCK_MARK_WAN_INDEX_BIT_MASK;
			if(table) *table = tblid_f;
			if(type) *type = type_f;
			ret = 0;
			break;
		}
	}

	fclose(fp);

	UNLOCK_SOCKMARK();
 	return ret;
}

int rtk_wan_sockmark_get(char *ifname, unsigned int *mark, unsigned int *mask)
{
	unsigned int sockmark_f, sockmask_f, type;
	int table_id;
	
	if(rtk_wan_sockmark_tblid_get(ifname, &sockmark_f, &sockmask_f, &table_id, &type) == 0) {
		if(mark) *mark = sockmark_f;
		if(mask) *mask = sockmask_f;
		return 0;
	}
 	return -1;
}

int rtk_wan_tblid_get(char *ifname)
{
	unsigned int sockmark_f, sockmask_f, type;
	int table_id;
	
	if(rtk_wan_sockmark_tblid_get(ifname, &sockmark_f, &sockmask_f, &table_id, &type) == 0) {
		return table_id;
	}
 	return -1;
}

int rtk_sockmark_init(void)
{
	FILE *f;
	
	va_cmd("/bin/touch", 1, 1, WAN_SOCKMARK_FILE);

	if((f = fopen(SockmarkLock, "w")) == NULL)
		return -1;
	
	fclose(f);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod (SockmarkLock, 0666);
#endif

	return 0;	
}


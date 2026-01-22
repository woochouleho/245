#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sysinfo.h>
#include <errno.h>
#include "../msgq.h"
#include <unistd.h>
#include <config/autoconf.h>
#include <rtk/options.h>
#include <sys/syscall.h>



#define MSG_CONFIGD_PID		8

static const char* get_process_name_by_pid(const int pid, char *name, int name_length);

static const char* get_process_name_by_pid(const int pid, char *name, int name_length)
{
	if(name){
		sprintf(name, "/proc/%d/cmdline",pid);
		FILE* f = fopen(name,"r");
		if(f){
			size_t size;
			size = fread(name, sizeof(char), name_length, f);
			if(size>0){
				if('\n'==name[size-1])
					name[size-1]='\0';
			}
			fclose(f);
		} else {
			name[0] = '\0';
		}
		name[name_length-1] = '\0';
	}
	return name;
}

int main()
{
	key_t key;
	int qid_snd, qid_rcv, cpid, ctgid, spid;
	struct mymsgbuf qbuf;
	char process_name[513];
	key = ftok("/bin/init", 'r');
	if ((qid_snd = open_queue(key, MQ_GET)) == -1) {
		perror("open_queue");
		return -1;
	}

	cpid = (int)syscall(SYS_gettid);
	ctgid = (int)getpid();

	spid = MSG_CONFIGD_PID;
	printf("%s:%d qid_snd %d %d %d\n", __FUNCTION__, __LINE__, qid_snd, sizeof(struct mymsgbuf)-sizeof(long), sizeof(struct mymsgbuf));
#if 0
	while(1){
		

		if ((msgrcv(qid_snd, (struct msgbuf *)&qbuf,
			sizeof(struct mymsgbuf)-sizeof(long), 0, 0)) < 0) {
			printf("%s:%d qid_snd %d\n", __FUNCTION__, __LINE__, qid_snd);
			perror("msq clean up");
			printf("[CONFIGD] Try clean one message! ERROR\n");
			break;
		} else {
			printf("%s:%d qid_snd %d\n", __FUNCTION__, __LINE__, qid_snd);
			printf("[CONFIGD] Clean one message! request %s pid %ld, \n\ttgid %s pid %ld\n\tmtype %ld\n",
					get_process_name_by_pid(qbuf.request, process_name, sizeof(process_name)), qbuf.request,
					get_process_name_by_pid(qbuf.tgid, process_name, sizeof(process_name)), qbuf.tgid,
					qbuf.mtype);
			printf("[CONFIGD] cmd=%d, arg1=%d, arg2=%d\n", qbuf.msg.cmd, qbuf.msg.arg1, qbuf.msg.arg1);
		}

	}
#endif
	send_message(qid_snd, cpid, cpid, ctgid, &qbuf);

	return 0;
}

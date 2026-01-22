
/*
 *	msgutil.c -- System V IPC message queue utility
 */

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
#include "msgq.h"
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>

int isConfigd = 0;
int msg_can_send = 1;
static unsigned long gettickcount() 
{
    struct timespec ts;
    if (0==clock_gettime(CLOCK_MONOTONIC, &ts)){
        return (ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000);
    }
    return 0;
}

int open_queue( key_t keyval, int flag )
{
	int     qid;

	if (flag == MQ_CREATE) {
		/* Create the queue - fail if exists */
		/* if set 0660, other user who is not in groups won't have privilege to get msg.
		 * first number "0"660 means Octal base.
		 */
		if((qid = msgget( keyval, IPC_CREAT |IPC_EXCL | 0666 )) == -1)
		{
			return(-1);
		}
	}
	else {
		/* Get the queue id - fail if not exists */
		if((qid = msgget( keyval, 0666 )) == -1)
		{
			return(-1);
		}
	}

	return(qid);
}

const char* get_process_name_by_pid(const int pid, char *name, int name_length)
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

static inline int checkPidExist(int tid, int tgid)
{
	int ret;
	ret = syscall(SYS_tgkill, tgid, tid, 0);

//	printf("check pid %d, ret=%d\n",tid, ret);
	if((ret == -1) && (errno == ESRCH))
	{
		printf("tid %d not existed!\n",tid);
		return 0;
	}
	else
		return 1;
}

//type is process pid, qid should receive queue id
static void drop_unexist_pid_message()
{
	msg_can_send = 0;
	char process_name[513];
	struct mymsgbuf qbuf, qbuf_tmp[MAX_MSGQ_MSGNUM];
	unsigned int num_of_msg = 0;
	struct msqid_ds msstat;
	unsigned int i = 0;
	int isDropQueue = 0, dropLong = 0;
	unsigned long stamptime = 0;
	long pid=0;
	long tgid = 0;
	int qid_rcv = 0;
	unsigned long currenttime = 0;
	key_t key;

	/* Create unique key via call to ftok() */
	key = ftok("/bin/init", 'r');
	if ((qid_rcv = open_queue(key, MQ_GET)) == -1) {
		perror("open_queue");
		msg_can_send = 1;
		return;
	}
	memset(qbuf_tmp, 0, sizeof(struct mymsgbuf)*MAX_MSGQ_MSGNUM);
	currenttime = gettickcount();
	
	if(isConfigd == 1){
	printf("[CONFIGD] Try clean all message!\n");
		int rc = msgctl(qid_rcv, IPC_STAT, &msstat);
		num_of_msg = (uint)(msstat.msg_qnum);

	for(i=0; i < num_of_msg; i++){
			if ((msgrcv(qid_rcv, (struct msgbuf *)&qbuf,
			sizeof(struct mymsgbuf)-sizeof(long), 0, 0)) < 0) {
			perror("msq clean up");
			printf("[CONFIGD] Try clean all message! ERROR\n");
		} else {
				if(i==0) {//initial the first message as the longest message
					stamptime = qbuf.stamptime;
					pid = qbuf.mtype;
					tgid = qbuf.tgid;
				}
				//record the longest message, if the following does not drop any message, we will drop the longest message
				if(qbuf.stamptime < stamptime){
					stamptime = qbuf.stamptime;
					pid = qbuf.mtype;
					tgid = qbuf.tgid;
				}
				
				if(checkPidExist(qbuf.mtype, qbuf.tgid)==1){//process exist
					memcpy(&qbuf_tmp[dropLong], &qbuf, sizeof(qbuf));
					dropLong++;
				}
				else{
					printf("=========================================================\n");
					printf("[CONFIGD] Clean all message! request %s pid %ld, \n\ttgid %s pid %ld\n\tmtype %ld  stamptime %ld currenttime %ld\n",
						get_process_name_by_pid(qbuf.mtype, process_name, sizeof(process_name)), qbuf.request,
						get_process_name_by_pid(qbuf.tgid, process_name, sizeof(process_name)), qbuf.tgid,
						qbuf.mtype, qbuf.stamptime, currenttime);
					printf("[CONFIGD] cmd=%d, arg1=%d, arg2=%d\n", qbuf.msg.cmd, qbuf.msg.arg1, qbuf.msg.arg1);
					isDropQueue = 1;
				}
			}
		}

		if(isDropQueue == 0 && pid != 0){ //all message's pid is exist, we drop the longest message
			printf("[CONFIGD] Try clean the longest message! pid %ld tgid %ld\n", pid, tgid);
			for(i=0; i < dropLong; i++){
				if(qbuf_tmp[i].mtype == pid && qbuf_tmp[i].tgid == tgid){
					printf("=========================================================\n");
					printf("[CONFIGD] Drop longest message request %s pid %ld, \n\ttgid %s pid %ld\n\tmtype %ld  stamptime %ld  currenttime %ld\n",
							get_process_name_by_pid(qbuf.mtype, process_name, sizeof(process_name)), qbuf.request,
					get_process_name_by_pid(qbuf.tgid, process_name, sizeof(process_name)), qbuf.tgid,
							qbuf.mtype, qbuf.stamptime, currenttime);
					printf("[CONFIGD] cmd=%d, arg1=%d, arg2=%d\n", qbuf.msg.cmd, qbuf.msg.arg1, qbuf.msg.arg1);
					qbuf_tmp[i].mtype = 0;
				}
			}
		}
		
		//send the message back
		for(i=0; i < MAX_MSGQ_MSGNUM; i++){
			if(qbuf_tmp[i].mtype != 0){
				msgsnd(qid_rcv, (struct msgbuf *)&qbuf_tmp[i], sizeof(struct mymsgbuf)-sizeof(long), IPC_NOWAIT);
				printf("=========================================================\n");
				printf("[CONFIGD] Send back(longest message) request %s pid %ld, \n\ttgid %s pid %ld\n\tmtype %ld  stamptime %ld  currenttime %ld\n",
						get_process_name_by_pid(qbuf.mtype, process_name, sizeof(process_name)), qbuf.request,
						get_process_name_by_pid(qbuf.tgid, process_name, sizeof(process_name)), qbuf.tgid,
						qbuf.mtype, qbuf.stamptime, currenttime);
			printf("[CONFIGD] cmd=%d, arg1=%d, arg2=%d\n", qbuf.msg.cmd, qbuf.msg.arg1, qbuf.msg.arg1);
		}
	}
	}
	msg_can_send = 1;
	return;
}

int send_message_nonblock(int qid, long type, long req, long tgid, void *buf)
{
	struct mymsgbuf *qbuf;
	long time_start;
	struct sysinfo systeminfo;
	int showmsg=0;
	int ret = 0;
	unsigned int num_of_msg = 0;
	int rc;

	if(buf == NULL)
	{
		qbuf = (struct mymsgbuf*)calloc(1, sizeof(struct mymsgbuf));
	}
	else{
		qbuf = (struct mymsgbuf *)buf;
	}
	if(qbuf == NULL){
		perror("qbuf = NULL"); 
		return -1;
	}
	
	/* Send a message to the queue */
	//printf("Sending a message ... qid=%d, mtype=%d\n", qid, type);
	qbuf->mtype = type;
	qbuf->request = req;
	qbuf->tgid = tgid;
	/*qbuf.msg.cmd = msg->cmd;
	qbuf.msg.arg1 = msg->arg1;
	qbuf.msg.arg2 = msg->arg2;
	memcpy(qbuf.msg.mtext, msg->mtext, MAX_SEND_SIZE);*/

	sysinfo(&systeminfo);
	time_start=systeminfo.uptime;
	qbuf->stamptime = gettickcount();

	//wait drop_unexist_pid_message() finish
	while(msg_can_send == 0){
		usleep(1000);
	}
callmsgsnd:
	if((msgsnd(qid, qbuf, sizeof(struct mymsgbuf)-sizeof(long), IPC_NOWAIT)) == -1)
	{
		if(isConfigd == 1){//configd
		if(errno==EAGAIN)
		{
			sysinfo(&systeminfo);
			//if( (showmsg==0)&&(systeminfo.uptime-time_start>120) )
			if( systeminfo.uptime-time_start>10 )
			{
				fprintf( stderr, "\nsend_message_nonblock: call msgsnd timeout and errno==EAGAIN reqid==%ld\n", req );
				//showmsg=1;
				time_start = systeminfo.uptime;
					drop_unexist_pid_message();
			}
			goto callmsgsnd;
		}else{
			perror("msgsnd");
				ret = -1;
			}
		}
		else{ //other process
			goto callmsgsnd;
		}
	}
	
	if(buf == NULL) free(qbuf);

	return ret;
}

int send_message(int qid, long type, long req, long tgid, void *buf)
{
	struct mymsgbuf *qbuf;
	long time_start;
	struct sysinfo systeminfo;
	char msg[513];
	int retry = 0;
	int ret = 0;
	
	if(buf == NULL)
	{
		qbuf = (struct mymsgbuf*)calloc(1, sizeof(struct mymsgbuf));
	}
	else{
		qbuf = (struct mymsgbuf *)buf;
	}
	if(qbuf == NULL){
		perror("send_message qbuf = NULL"); 
		return -1; 
	}
	
	/* Send a message to the queue */
	//printf("Sending a message ... qid=%d, mtype=%d\n", qid, type);
	qbuf->mtype = type;
	qbuf->request = req;
	qbuf->tgid = tgid;

	sysinfo(&systeminfo);
	time_start=systeminfo.uptime;
	qbuf->stamptime = gettickcount();

retrymsgsnd:
	if((msgsnd(qid, qbuf, sizeof(struct mymsgbuf)-sizeof(long), 0)) == -1)
	{
		if (errno == EAGAIN)
		{
			sysinfo(&systeminfo);
			if( systeminfo.uptime-time_start<10 )
			{
			snprintf(msg, sizeof(msg), "send_message msgsnd EAGAIN type %ld, reqid==%ld", type, req);
			perror(msg);
			goto retrymsgsnd;
		}
			ret = -1;
		}
		else if (errno == EINTR)
		{
			sysinfo(&systeminfo);
			if( systeminfo.uptime-time_start<10 )
			{
			snprintf(msg, sizeof(msg), "send_message msgsnd EINTR type %ld, reqid==%ld", type, req);
			perror(msg);
			goto retrymsgsnd;
		}
			ret = -1;
		}
		else
		{
			perror("msgsnd");
			ret = -1;
		}
	}
	
	if(buf == NULL) free(qbuf);
	
	return ret;
}

/*
	Return Value:
	Return -1 on failure;
	otherwise return the number of bytes actually copied into the mtext array.
*/
int read_message(int qid, void *buf, long type)
{
	int ret=0;
	struct mymsgbuf *qbuf = buf;
	
	if(qbuf == NULL){
		perror("send_message qbuf = NULL");
		return -1;
	}

	/* Read a message from the queue */
	//printf("Reading a message ...\n");
	qbuf->mtype = type;
read_retry:
	ret=msgrcv(qid, (struct msgbuf *)qbuf,
		sizeof(struct mymsgbuf)-sizeof(long), type, 0);
	if (ret == -1 && errno == EINTR) {
		//printf("EINTR\n");
		goto read_retry;
	}
/*
	if (ret == -1) {
		switch (errno) {
			case E2BIG   :
				printf("E2BIG    \n");
				break;
			case EACCES :
				printf("EACCES  \n");
				break;
			case EFAULT   :
				printf("EFAULT   \n");
				break;
			case EIDRM  :
				printf("EIDRM  \n");
				break;
			case EINTR    :
				printf("EINTR    \n");
				break;
			case EINVAL   :
				printf("EINVAL   \n");
				break;
			case ENOMSG   :
				printf("ENOMSG   \n");
				break;
			default:
				printf("unknown\n");
		}
	}
	printf("Type: %ld Text: %s\n", qbuf->mtype, qbuf->mtext);
*/
	return ret;
}

void remove_queue(int qid)
{
	/* Remove the queue */
	msgctl(qid, IPC_RMID, 0);
}

/*
 *	Set max number of bytes allowed in queue (msg_qbytes).
 *	/proc/sys/kernel/msgmnb		default max size of a message queue.
 *	/proc/sys/kernel/msgmax		max size of a message (bytes).
 */
int set_msgqueue_max_size(int qid, unsigned int msgmaxsize)
{
	int ret=-1;
	struct msqid_ds msgqidds;

	//fprintf(stderr, "%s:%d>qid=%d, msgmaxsize=%d\n", __FUNCTION__, __LINE__, qid, msgmaxsize);
	if( msgctl(qid, IPC_STAT, &msgqidds)==0 )
	{
		msgqidds.msg_qbytes = msgmaxsize;
		if( msgctl(qid, IPC_SET, &msgqidds)==0 )
			ret=0;
		else
			perror( "msgctl,IPC_SET" );

		if( (msgctl(qid, IPC_STAT, &msgqidds)==0) &&(msgqidds.msg_qbytes == msgmaxsize) )
				fprintf(stderr, "%s:%d> set msgqidds.msg_qbytes=%d OK\n", __FUNCTION__, __LINE__, msgmaxsize);
	}else
		perror( "msgctl,IPC_STAT" );

	return ret;
}



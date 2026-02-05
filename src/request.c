/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Some changes Copyright (C) 1996,97 Larry Doolittle <ldoolitt@jlab.org>
 *  Some changes Copyright (C) 1996,97 Jon Nelson <nels0988@tc.umn.edu>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* boa: request.c */

#define _GNU_SOURCE 
#include <string.h>
#include "asp_page.h"
#include "boa.h"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include "LINUX/sysconfig.h"

#ifdef SERVER_SSL
#ifdef USES_MATRIX_SSL
#include <sslSocket.h>
#else /*!USES_MATRIX_SSL*/
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif /*USES_MATRIX_SSL*/
#endif

#include "syslog.h"

#ifdef EMBED
#include "./LINUX/options.h"
#if defined(WLAN_WEB_REDIRECT) || defined(WEB_REDIRECT_BY_MAC) || defined(_SUPPORT_CAPTIVEPORTAL_PROFILE_)
#include "./LINUX/mib.h"
#endif
#else
#include "./LINUX/options.h"
#if defined(WLAN_WEB_REDIRECT) || defined(WEB_REDIRECT_BY_MAC) || defined(_SUPPORT_CAPTIVEPORTAL_PROFILE_)
#include "./LINUX/mib.h"
#endif
#endif
//ql
#ifdef ENABLE_SIGNATURE_ADV
#include "defs.h"
#endif

#ifdef CONFIG_00R0
#include "rtk/gpon.h"
#endif

#ifdef EMBED
#include <rtk/utility.h>
#else
#include "LINUX/utility.h"
#endif

extern unsigned char g_login_username[];

request *get_sock_request(int sock_fd);


int sockbufsize = SOCKETBUF_SIZE;
int g_upgrade_firmware=FALSE;

#ifdef CONFIG_YUEME
/*
 * fast_maintain is set when fast maintain is clicked
 * it should be cleared after user try to login(formLogin is posted)
 */
int fast_maintain = 0;
#endif

//extern int server_s;			/* boa socket */
extern int do_sock;				/*Do normal sockets??*/
extern time_t time_counter;

#ifdef SERVER_SSL
//extern int server_ssl;			/*the socket to listen for ssl requests*/
#ifdef USES_MATRIX_SSL
#else /* !USES_MATRIX_SSL */
extern SSL_CTX *ctx;			/*The global connection context*/
extern int do_ssl;				/*do ssl sockets??*/
#endif /*USES_MATRIX_SSL */
#endif /*SERVER_SSL*/

extern int rtk_util_get_ipv4addr_from_ipv6addr(char* ipaddr);

/*
 * the types of requests we can handle
 */

static struct {
	char	*str;
	int		 type;
} request_types[] = {
	{ "GET ",	M_GET },
	{ "POST ",	M_POST },
	{ "HEAD ",	M_HEAD },
	{ NULL,		0 }
};

/*
 * return the request type for a request,  short or invalid
 */

int request_type(request *req)
{
	int i, n, max_out = 0;

	for (i = 0; request_types[i].str; i++) {
		n = strlen(request_types[i].str);
		if (req->client_stream_pos < n) {
			max_out = 1;
			continue;
		}
		if (!memcmp(req->client_stream, request_types[i].str, n))
			return(request_types[i].type);
	}
	return(max_out ? M_SHORT : M_INVALID);
}


/*
 * Name: new_request
 * Description: Obtains a request struct off the free list, or if the
 * free list is empty, allocates memory
 *
 * Return value: pointer to initialized request
 */

request *new_request(void)
{
	request *req;

	if (request_free) {
		req = request_free;		/* first on free list */
		dequeue(&request_free, req);	/* dequeue the head */
	} else {
#if 1
		return NULL;
#else
		req = (request *) malloc(sizeof(request));
		if (!req)
			die(OUT_OF_MEMORY);
#endif
	}

#ifdef SERVER_SSL
	req->ssl = NULL;
#endif /*SERVER_SSL*/

	memset(req, 0, sizeof(request) - NO_ZERO_FILL_LENGTH);

#ifdef SUPPORT_ASP
		req->max_buffer_size=CLIENT_STREAM_SIZE;
		req->buffer=(char *)malloc(req->max_buffer_size+1);
		if(!req->buffer)
			die(OUT_OF_MEMORY);
#endif

	return req;
}


/*
 * Name: get_request
 *
 * Description: Polls the server socket for a request.  If one exists,
 * does some basic initialization and adds it to the ready queue;.
 */

void get_request(int sock_fd)
{
	get_sock_request(sock_fd);
}


#if defined(WLAN_WEB_REDIRECT) || defined(SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE) //jiunming,web_redirect
#ifdef WLAN_WEB_REDIRECT
extern int redir_server_s;
#endif
#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
extern int fwupgrade_redir_server_s;
#endif
void get_redir_request(int redir_server_socket)
{
	request *new_req;
	new_req = get_sock_request(redir_server_socket);
	if(new_req)
	{
#ifdef WLAN_WEB_REDIRECT
		if(redir_server_socket == redir_server_s)
			new_req->request_from=1;
		else
#endif
#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
		if(redir_server_socket == fwupgrade_redir_server_s)
			new_req->request_from=2;
#endif
	}		
}
#endif

#ifdef WEB_REDIRECT_BY_MAC
extern int mac_redir_server_s;
void get_mac_redir_request(void)
{
	request *new_req;
	new_req = get_sock_request(mac_redir_server_s);
	if(new_req)
		new_req->request_mac=1;
}
int handle_mac_for_webredirect( char *mac )
{
	unsigned char tmp[6];
	unsigned int tmp1[6];

	if(mac==NULL) return -1;

	if( sscanf( mac, "%x:%x:%x:%x:%x:%x",
			&tmp1[0], &tmp1[1], &tmp1[2], &tmp1[3], &tmp1[4], &tmp1[5] )!=6 )
	{
		printf( "\n paser the mac format error: %s\n", mac );
		return -1;
	}


	if( tmp1[0]==0 && tmp1[1]==0 && tmp1[2]==0 &&
		tmp1[3]==0 && tmp1[4]==0 && tmp1[5]==0 )
	{
		printf( "\n the mac address is zero: %s\n", mac );
		return -1;
	}

	tmp[0]=tmp1[0];tmp[1]=tmp1[1];tmp[2]=tmp1[2];
	tmp[3]=tmp1[3];tmp[4]=tmp1[4];tmp[5]=tmp1[5];
	//printf( "\n the parsed mac address is %02X:%02X:%02X:%02X:%02X:%02X\n", tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5] );

	{
		int num;
		unsigned char tmp2[18];
		MIB_WEB_REDIR_BY_MAC_T	wrm_entry;

		num = mib_chain_total( MIB_WEB_REDIR_BY_MAC_TBL );
		//printf( "\nori num=%d\n", num );
		if( num>=MAX_WEB_REDIR_BY_MAC )
		{
			//delete the first one
			if( mib_chain_get( MIB_WEB_REDIR_BY_MAC_TBL, 0, (void*)&wrm_entry )!=0 )
			{
				sprintf( tmp2, "%02X:%02X:%02X:%02X:%02X:%02X",
					wrm_entry.mac[0], wrm_entry.mac[1], wrm_entry.mac[2], wrm_entry.mac[3], wrm_entry.mac[4], wrm_entry.mac[5] );
				//printf( "delete old one mac: %s \n", tmp2 );
				// iptables -A macfilter -i eth0  -m mac --mac-source $MAC -j ACCEPT/DROP
				va_cmd("/bin/iptables", 10, 1, "-t", "nat", "-D", "WebRedirectByMAC", "-m", "mac", "--mac-source", tmp2, "-j", "RETURN");
			}

			mib_chain_delete( MIB_WEB_REDIR_BY_MAC_TBL, 0 );
			//num = mib_chain_total( MIB_WEB_REDIR_BY_MAC_TBL );
			//printf( "\after del nnum=%d\n", num );
		}

		//add the new one
		memcpy( wrm_entry.mac, tmp, 6 );
		mib_chain_add( MIB_WEB_REDIR_BY_MAC_TBL, (unsigned char*)&wrm_entry );
		// iptables -A macfilter -i eth0  -m mac --mac-source $MAC -j ACCEPT/DROP
		va_cmd("/bin/iptables", 10, 1, "-t", "nat", "-I", "WebRedirectByMAC", "-m", "mac", "--mac-source", mac, "-j", "RETURN");
		//num = mib_chain_total( MIB_WEB_REDIR_BY_MAC_TBL );
		//printf( "\after add nnum=%d\n", num );

		//update to the flash
		#if 0
		itfcfg("sar", 0);
		itfcfg("eth0", 0);
		#endif
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
		#if 0
		itfcfg("sar", 1);
		itfcfg("eth0", 1);
		#endif
	}

	return 0;
}
#endif

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
extern int captiveportal_server_s;
void get_captiveportal_request(void)
{
	request *new_req;
	new_req = get_sock_request(captiveportal_server_s);

	if(new_req)
		new_req->request_captiveportal = 1;
	/*else
		new_req->request_captiveportal = 0;*/
}
#endif

#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
extern int fwupgrade_redir_server_s;
void get_fwupgrade_redir_request(void)
{
	request *new_req;
	new_req = get_sock_request(fwupgrade_redir_server_s);
	if(new_req)
		new_req->request_from=2;
}
#endif

#ifdef USE_DEONET
extern int redir_server_s;
void get_access_redir_request(int redir_server_socket)
{
	request *new_req;
	new_req = get_sock_request(redir_server_socket);
	if(new_req)
	{
		if(redir_server_socket == redir_server_s)
			new_req->request_from=3;
	}		
}
#endif


#ifdef SERVER_SSL
#ifdef USES_MATRIX_SSL
	extern sslConn_t	*mtrx_cp;
	extern sslKeys_t	*mtrx_keys;
void get_ssl_request(int sock_fd)
{
	request *conn;
	int	rc,flags=0;


	conn = get_sock_request(sock_fd);
	if (!conn)
		return;

	if ((rc = sslAccept(&mtrx_cp, conn->fd, mtrx_keys, NULL, flags)) != 0) {
		socketShutdown(conn->fd);
		return;
	}
	conn->ssl = mtrx_cp;
}
#ifdef WEB_REDIRECT_BY_MAC
extern int mac_redir_server_ssl;
void get_mac_redir_ssl_request(void)
{
	request *conn;
	int	rc,flags=0;


	conn = get_sock_request(mac_redir_server_ssl);
	if (!conn)
		return;
	if(conn)
		conn->request_mac=1;

	if ((rc = sslAccept(&mtrx_cp, conn->fd, mtrx_keys, NULL, flags)) != 0) {
		socketShutdown(conn->fd);
		return;
	}
	conn->ssl = mtrx_cp;
}
#endif
#else /*!USES_MATRIX_SSL */
void get_ssl_request(int sock_fd)
{
	request *conn;

	conn = get_sock_request(sock_fd);
	if (!conn)
		return;
	conn->ssl = SSL_new (ctx);
	if(conn->ssl == NULL){
		printf("Couldn't create ssl connection stuff\n");
		return;
	}
	SSL_set_fd (conn->ssl, conn->fd);
	if(SSL_accept(conn->ssl) <= 0){
		ERR_print_errors_fp(stderr);
		return;
	}
	else{/*printf("SSL_accepted\n");*/}
}

#ifdef WEB_REDIRECT_BY_MAC
extern int mac_redir_server_ssl;
void get_mac_redir_ssl_request(void)
{
	request *conn;

	conn = get_sock_request(mac_redir_server_ssl);
	if (!conn)
		return;
	if(conn)
		conn->request_mac=1;
	//printf("SSL_accepted: 10443 packet\n");
	conn->ssl = SSL_new (ctx);
	if(conn->ssl == NULL){
		printf("Couldn't create ssl connection stuff\n");
		return;
	}
	SSL_set_fd (conn->ssl, conn->fd);
	if(SSL_accept(conn->ssl) <= 0){
		ERR_print_errors_fp(stderr);
		return;
	}
	else{/*printf("SSL_accepted\n");*/}
}
#endif
#endif /*USES_MATRIX_SSL*/
#endif /*SERVER_SSL*/

/* This routine works around an interesting problem encountered with IE
 * and the 2.4 kernel (i.e. no problems with netscape or with the 2.0 kernel).
 * The hassle is that the connection socket has a couple of crap bytes sent to
 * it by IE after the HTTP request.  When we close a socket with some crap waiting
 * to be read, the 2.4 kernel shuts down with a reset whereas earlier kernels
 * did the standard fin stuff.  IE complains about the reset and aborts page
 * rendering immediately.
 *
 * We must not loop here otherwise a DoS will have us for breakfast.
 */
static void safe_close(int fd) {
	fd_set rfd;
	struct timeval to;
	char buf[32];

	to.tv_sec = 0;
	to.tv_usec = 100;
	FD_ZERO(&rfd);
	FD_SET(fd, &rfd);
	if ((select(fd+1, &rfd, NULL, NULL, &to)) > 0 && FD_ISSET(fd, &rfd))
		read(fd, buf, sizeof buf);
	close(fd);
}

/*
 * Name: free_request
 *
 * Description: Deallocates memory for a finished request and closes
 * down socket.
 */

void free_request(request ** list_head_addr, request * req)
{
	DBG(printf("%s:%d: req->remote_port=%d,req->buffer_end=%d\n",__func__,__LINE__,req->remote_port,req->buffer_end);)
	if (req->buffer_end)
		return;

	if(list_head_addr) dequeue(list_head_addr, req);	/* dequeue from ready or block list */

	if (req->buffer_end)
		FD_CLR(req->fd, &block_write_fdset);
	else {
		switch (req->status) {
		case PIPE_WRITE:
		case WRITE:
			FD_CLR(req->fd, &block_write_fdset);
			break;
		case PIPE_READ:
			FD_CLR(req->data_fd, &block_read_fdset);
			break;
		case BODY_WRITE:
			FD_CLR(req->post_data_fd, &block_write_fdset);
			break;
		default:
			FD_CLR(req->fd, &block_read_fdset);
		}
	}

	if (req->logline)			/* access log */
		log_access(req);

	if (req->data_mem)
		munmap(req->data_mem, req->filesize);

	if (req->data_fd)
		close(req->data_fd);

	if (req->response_status >= 400)
		status.errors++;

	if ((req->keepalive == KA_ACTIVE) &&
		(req->response_status < 400) &&
		(++req->kacount < ka_max)) {

		request *conn;

		conn = new_request();
		conn->fd = req->fd;
		conn->status = READ_HEADER;
		conn->header_line = conn->client_stream;
		conn->time_last = time_counter;
		conn->kacount = req->kacount;
#ifdef SERVER_SSL
		conn->ssl = req->ssl; /*MN*/
#endif /*SERVER_SSL*/

		/* we don't need to reset the fd parms for conn->fd because
		   we already did that for req */
		/* for log file and possible use by CGI programs */

		strcpy(conn->remote_ip_addr, req->remote_ip_addr);

		/* for possible use by CGI programs */
		conn->remote_port = req->remote_port;

		if (req->local_ip_addr)
			conn->local_ip_addr = strdup(req->local_ip_addr);

		status.requests++;

		if (conn->kacount + 1 == ka_max)
			SQUASH_KA(conn);

		conn->pipeline_start = req->client_stream_pos -
								req->pipeline_start;

		if (conn->pipeline_start) {
			memcpy(conn->client_stream,
				req->client_stream + req->pipeline_start,
				conn->pipeline_start);
			enqueue(&request_ready, conn);
		} else
			block_request(conn);
	} else{
		if (req->fd != -1) {
			status.connections--;
			safe_close(req->fd);
		}
		req->fd = -1;
#ifdef SERVER_SSL
#ifdef USES_MATRIX_SSL
		if (req->ssl)
		sslFreeConnection((sslConn_t**)&req->ssl);
#else /*!USES_MATRIX_SSL*/
		SSL_free(req->ssl);
#endif /*USES_MATRIX_SSL*/
#endif /*SERVER_SSL*/
	}

	if (req->cgi_env) {
		int i = COMMON_CGI_VARS;
		req->cgi_env[req->cgi_env_index]=0;
		while (req->cgi_env[i])
		{
			free(req->cgi_env[i++]);
		}
		free(req->cgi_env);
	}
	if (req->pathname)
		free(req->pathname);
	if (req->path_info)
		free(req->path_info);
	if (req->path_translated)
		free(req->path_translated);
	if (req->script_name)
		free(req->script_name);
	if (req->query_string)
		free(req->query_string);
	if (req->local_ip_addr)
		free(req->local_ip_addr);
/*
 *	need to clean up if anything went wrong
 */
 // Mason Yu
 	if (g_upgrade_firmware==FALSE) {
		if (req->post_file_name) {
			unlink(req->post_file_name);
			free(req->post_file_name);
			close(req->post_data_fd);
			req->post_data_fd = -1;
			req->post_file_name = NULL;
		}
	} else {
		if (access("/tmp/g_upgrade_firmware", F_OK) != 0){
			g_upgrade_firmware=FALSE;
		}
	}

	if(req->buffer)
	{
		free(req->buffer);
		req->buffer = NULL;
	}


	enqueue(&request_free, req);	/* put request on the free list */

	return;
}


/*
 * Name: process_requests
 *
 * Description: Iterates through the ready queue, passing each request
 * to the appropriate handler for processing.  It monitors the
 * return value from handler functions, all of which return -1
 * to indicate a block, 0 on completion and 1 to remain on the
 * ready list for more procesing.
 */

void process_requests(void)
{
	int retval = 0;
	request *current, *trailer;
	current = request_ready;
#ifdef USER_WEB_WIZARD
	int isConnect = get_ponStatus();
	unsigned char redirectTroubleWizard, wizardFlag;
	mib_get_s(RS_WEB_WIZARD_REDIRECTTROUBLEWIZARD, &redirectTroubleWizard, sizeof(redirectTroubleWizard));
	mib_get_s(MIB_USER_WEB_WIZARD_FLAG, (void *)&wizardFlag, sizeof(wizardFlag));
#endif
	while (current) {
#ifdef CRASHDEBUG
		crashdebug_current = current;
#endif
#ifdef CONFIG_VIR_BOA_JEMBENCHTEST
		if(!strcmp(current->request_uri, "/jembenchtest")){
			srand(time(NULL));
			printf("##xxxx##Access %s\n",current->request_uri);
			sleep(180+rand()%10);
			printf("##xxxx##Access %s\n",current->request_uri);

		}
#endif
		DBG(printf("%s:%d: current->remote_port=%d,current->request_uri=%s,current->status=%d!\n",__func__,__LINE__,current->remote_port,current->request_uri,current->status);)
		if (current->buffer_end) {
			req_flush(current);
			if (current->status == CLOSE)
				retval = 0;
			else
				retval = 1;
		} else {
			switch (current->status) {
			case READ_HEADER:
			case ONE_CR:
			case ONE_LF:
			case TWO_CR:
				retval = read_header(current);
				break;
			case BODY_READ:
				retval = read_body(current);
				break;
			case BODY_WRITE:
				retval = write_body(current);
				break;
			case WRITE:
				retval = process_get(current);
				break;
			case PIPE_READ:
				retval = read_from_pipe(current);
				break;
			case PIPE_WRITE:
				retval = write_from_pipe(current);
				break;
			default:
				retval = 0;
#if 0
				fprintf(stderr, "Unknown status (%d), closing!\n",
						current->status);
#endif
				break;
			}
		}

		if (lame_duck_mode)
			SQUASH_KA(current);
		// Mason Yu
		//printf("retval=%d\n", retval);
		DBG(printf("%s:%d: retval=%d!\n",__func__,__LINE__,retval);)
		switch (retval) {
		case -1:				/* request blocked */
			trailer = current;
			current = current->next;
#ifdef USER_WEB_WIZARD
			if((isConnect == 0 && redirectTroubleWizard == 1) || (wizardFlag==1))
				free_request(&request_ready, trailer);
			else
#endif
			block_request(trailer);
			break;
		default:			/* everything else means an error, jump ship */
			send_r_error(current);
			/* fall-through */
		case 0:				/* request complete */
			trailer = current;
			current = current->next;
			free_request(&request_ready, trailer);
#ifdef SUPPORT_ASP
			if(trailer->status==REBOOT)
			{
				cmd_reboot();
			}
#endif
			if(trailer->status==LANIP_CHANGE)
			{
				restart_lanip();
			}
			break;
		case 1:				/* more to do */
			current->time_last = time_counter;
			current = current->next;
			break;
		}
	}
#ifdef CRASHDEBUG
		crashdebug_current = current;
#endif
}

/*
 * Name: process_logline
 *
 * Description: This is called with the first req->header_line received
 * by a request, called "logline" because it is logged to a file.
 * It is parsed to determine request type and method, then passed to
 * translate_uri for further parsing.  Also sets up CGI environment if
 * needed.
 */

int process_logline(request * req)
{
	char *stop, *stop2;
	static char *SIMPLE_HTTP_VERSION = "HTTP/0.9";
	struct user_info * pUser_info = NULL;

	req->logline = req->header_line;
	req->method = request_type(req);
	if (req->method == M_INVALID || req->method == M_SHORT) {
#ifdef BOA_TIME_LOG
		log_error_time();
		fprintf(stderr, "malformed request: \"%s\"\n", req->logline);
#endif
		syslog(LOG_ERR, "malformed request: \"%s\" from %s\n", req->logline, req->remote_ip_addr);
		send_r_bad_request(req);
		return 0;
	}

	/* Guaranteed to find ' ' since we matched a method above */
	stop = req->logline + 3;
	if (*stop != ' ')
		++stop;

	/* scan to start of non-whitespace */
	while (*(++stop) == ' ');

	stop2 = stop;

	/* scan to end of non-whitespace */
	while (*stop2 != '\0' && *stop2 != ' ')
		++stop2;

	if (stop2 - stop > MAX_HEADER_LENGTH) {
#ifdef BOA_TIME_LOG
		log_error_time();
		fprintf(stderr, "URI too long %d: \"%s\"\n", MAX_HEADER_LENGTH,
				req->logline);
#endif
		syslog(LOG_ERR, "URI too long %d: \"%s\" from %s\n", MAX_HEADER_LENGTH,
				req->logline, req->remote_ip_addr);
		send_r_bad_request(req);
		return 0;
	}
	memcpy(req->request_uri, stop, stop2 - stop);
	req->request_uri[stop2 - stop] = '\0';
	if (strstr(req->request_uri, "Menu.js")) {
		for (pUser_info = user_login_list; pUser_info; pUser_info = pUser_info->next) {
			if (!strcmp(req->remote_ip_addr, pUser_info->remote_ip_addr)) {
				break;
			}
		}
		if (!pUser_info) {
#if 0 /* adminMenu.js로의 직접 접근 시 login.asp로 이동 */
			return 0; 
#endif
		}
	}

	if (*stop2 == ' ') {
		/* if found, we should get an HTTP/x.x */
		int p1, p2;

		if (sscanf(++stop2, "HTTP/%d.%d", &p1, &p2) == 2 && p1 >= 1) {
			req->http_version = stop2;
			req->simple = 0;
		} else {
#ifdef BOA_TIME_LOG
			log_error_time();
			fprintf(stderr, "bogus HTTP version: \"%s\"\n", stop2);
#endif
			syslog(LOG_ERR, "bogus HTTP version: \"%s\" from %s\n", stop2, req->remote_ip_addr);
			send_r_bad_request(req);
			return 0;
		}

	} else {
		req->http_version = SIMPLE_HTTP_VERSION;
		req->simple = 1;
	}


	if (req->method == M_HEAD && req->simple) {
		syslog(LOG_ERR, "Simple HEAD request not allowed from %s\n", req->remote_ip_addr);
		send_r_bad_request(req);
		return 0;
	}
	create_env(req);    /* create cgi env[], we don't know if url is cgi */
	return 1;
}

/*
 * return value: 1 - attack  0 - normal
 */

int attack_check(char *buf, int len)
{
	char *hitKeyword[] = {
		"<script",
		"/script",
		"alert(",
		"<img src",
		"<iframe",
		"/iframe",
		"onerror",
		"onclick",
		"window.location",
		"><a",
		"javascript",
		"jQuery",
		"sleep",
		"%3Cscript",// <script
		"%2Fscript",// /script
		"alert%28",  // alert(
		"><p",
		NULL
	};
#ifdef CONFIG_CU
	char *hitKeyword_appscan[] = {
		"securityip.appsechcl.com",
		"appsechcl",
		"and 'f'='",
		"and 'b'='",
		"or 'b'='",
		"or 'f'='",
		"+and+",
		"+or+",
		"objectclass",
		"%28name%3D*",
		"%27+%2B",
		"SQL",
		"f%",
		"b%",
		"||",
		"waitfor%20",
		NULL
	};
#endif

	int i;
	
	for (i=0; hitKeyword[i]; i++)
	{
		if (memcasestr(buf, hitKeyword[i], len))
		{
			return 1;
		}
	}

#ifdef CONFIG_CU
	if(appscan_flag)
	{
		for (i=0; hitKeyword_appscan[i]; i++)
		{
			if (memcasestr(buf, hitKeyword_appscan[i], len))
			{
				if((!strcmp("f%", hitKeyword_appscan[i]) && (memcasestr(buf, "2f%", len)))
					|| (!strcmp("b%", hitKeyword_appscan[i]) && (memcasestr(buf, "2b%", len)))
					)
					continue;

				return 1;
			}
		}
	}
#endif

	return 0;
}

int attack_req_check(request * req)
{
	int ret = 0;
	
	if(req->method == M_POST) 
	{
		if(req->post_data_fd > 0) 
		{
			char *buf = NULL, *ptr = NULL;
			unsigned int bufSize = 0, searchSize = 0;
			
			lseek(req->post_data_fd, 0, SEEK_END);
			bufSize = lseek(req->post_data_fd, 0, SEEK_CUR);
			lseek(req->post_data_fd, 0, SEEK_SET);
			
			if (bufSize > 0 && (buf = mmap(NULL, bufSize, PROT_READ, MAP_SHARED, req->post_data_fd, 0)) != MAP_FAILED)
			{
				ptr = buf;
				searchSize = bufSize;		
				if(req->content_type != NULL && strstr(req->content_type, "multipart/form-data")) 
				{
					char *start, *p, *pval, boundary[128]={'-', '-'}, check;
					int len, nlen, boundry_len;
					if((start = strstr(req->content_type, "boundary="))) 
					{
						start += strlen("boundary=");
						p = strchr(start, ';');
						if(p) boundry_len = p - start;
						else boundry_len = strlen(start);
						boundry_len = (boundry_len < (sizeof(boundary)-3)) ? boundry_len : (sizeof(boundary)-3);
						strncpy(boundary+2, start, boundry_len);
						boundry_len += 2;
						boundary[boundry_len] = '\0';
						
						len = searchSize;
						start = ptr;
						while(!ret && len>0 && start && (pval = memstr(start, boundary, len))) 
						{
							pval += boundry_len;
							len -= (pval - start);
							start = pval;
							p = memstr(start, "\r\n\r\n", len);
							if(p) 
							{
								p += 4;
								nlen = (p - start);		
								pval = memstr(start, "Content-Type:", nlen);
								if(pval) check = 0;
								else check = 1;
								len -= nlen;
								start = p;	
								if(!pval) {
									pval = start;
									p = memstr(pval, boundary, len);
									if(p) {
										nlen = (p-2-pval);
										len -= (nlen+2);
										start = p;
									}
									else {
										nlen = len;
										len = 0;
										start = NULL;
									}
									ret = attack_check(pval, nlen);
								}
							}
						}
					}
				}
				else {
					ret = attack_check(ptr, searchSize);
				}
				
				munmap(buf, bufSize);
			}
		}
	}
	if(!ret && req->query_string != NULL)
		ret = attack_check(req->query_string, strlen(req->query_string));
	return (ret > 0) ? 1:0;
}

#ifdef CONFIG_CU
unsigned char g_appscan=0;
struct callout_s t_appscan;

void clear_appscan(void *dummy)
{
	g_appscan = 0;
	//printf("%s:%d\n", __FUNCTION__, __LINE__);
	UNTIMEOUT_F(0, 0, t_appscan);
}
#endif

#define DEO_MAX_ADMIN_WEB_PAGE_LIST		173 /* 171 */
#define DEO_MAX_USER_WEB_PAGE_LIST		153 /* 151 */

static char *deo_admin_web_page_list[DEO_MAX_ADMIN_WEB_PAGE_LIST] = 
{
	"status.asp",
	"status_ipv6.asp",
	"status_pon.asp",
	"lan_port_status.asp",
	"tcpiplan.asp",
	"deo_multi_wan_generic.asp",
	"dhcpd.asp",
	"ddns.asp",
	"igmproxy.asp",
	"access_limit.asp",
	"client_info.asp",
	"algonoff.asp",
	"fw-ipportfilter.asp",
	"fw-macfilter.asp",
	"fw-portfw.asp",
	"static-mapping.asp",
	"fw-dmz.asp",
	"arptable.asp",
	"lbd.asp",
	"snmpv3.asp",
	"net_qos_cls.asp",
	"net_qos_traffictl.asp",
	"ipv6_enabledisable.asp",
	"app_mldProxy.asp",
	"app_mld_snooping.asp",
	"fw-ipportfilter-v6.asp",
	"aclv6.asp",
	"ping.asp",
	"ping_result.asp",
	"ping6.asp",
	"ping6_result.asp",
	"tracert.asp",
	"tracert_result.asp",
	"tracert6.asp",
	"tracert6_result.asp",
	"grouptable.asp",
	"grouptable_result.asp",
	"ssh.asp",
	"ssh_result.asp",
	"netstat.asp",
	"netstat_result.asp",
	"portmirror.asp",
	"portmirror_result.asp",
	"reboot.asp",
	"saveconf.asp",
	"syslog.asp",
	"user_syslog.asp",
	"syslog_server.asp",
	"ldap.asp",
	"holepunching.asp",
	"dos.asp",
	"password.asp",
	"upgrade.asp",
	"autoreboot.asp",
	"natsession.asp",
	"useage.asp",
	"selfcheck.asp",
	"acl.html",
	"tz.asp",
	"logout.asp",
	"stats.asp",
	"pon-stats.asp",
	"login.asp",
	"login_base.asp",
	"login.cgi",
	"usereg.asp",
	"useregresult.asp",
	"terminal_inspec.asp",
	"dl_notify.asp",
	"net_qos_cls_edit.asp",
	"net_qos_traffictl_edit.asp",
	"dhcptbl.asp",
	"portBaseFilterDhcp.asp",
	"macIptbl.asp",
	"lanApConnection.asp",
	"wanApConnection.asp",
	"invalid_page.asp",
	"formWanApConnection",
	"formUpload",
	"formWanRedirect",
	"formPing",
	"formTracert",
	"formSshResult",
	"formNetstatResult",
	"formPortMirrorResult",
	"formGroupTable",
	"formSsh",
	"formNetstat",
	"formPortMirror",
	"formBankStatus",
	"formUseageSetup",
	"formNatSessionSetup",
	"formDosCfg",
	"formAutoReboot",
	"formNtp",
	"formPasswordSetup",
	"formTcpipLanSetup",
	"formWanEth",
	"formDhcpServer",
	"formDDNS",
	"formAccessLimit",
	"formReflashClientTbl",
	"formNickname",
	"formALGOnOff",
	"formFilter",
	"formPortFw",
	"formStaticMapping",
	"formDMZ",
	"formRefleshFdbTbl",
	"formLBD",
	"formWanEth",
	"formFilterV6",
	"formReboot",
	"formSaveConfig",
	"formStats",
	"formStatus",
	"formLANPortStatus",
	"formIgmproxy",
	"formSnmpConfig",
	"formMLDProxy",
	"formMLDSnooping",
	"formV6ACL",
	"formLogout",
	"formmacBase",
	"formSysLog",
	"formHolepunching",
	"formLdap",
	"formACL",
	"formQosRuleEdit",
	"formQosRule",
	"formQosTraffictlEdit",
	"formQosTraffictl",
	"userLogin",
	"bootstrap",
	"captcha.png",
	"common.js",
	"common.css",
	"base64_code.js",
	"sha256_code.js",
	"default.css",
	"btn_login_bg.gif",
	"login_bg_new.jpg",
	"icon_login.png",
	"logo.gif",
	"bg_ah_login_01.gif",
	"bg_ah_login.gif",
	"loid_register.gif",
	"loidreg_ah_01.gif",
	"loidreg_ah.gif",
	"favicon.ico",
	"sk_loing_logo.png",
	"reset.css",
	"style.css", 
	"jquip.js",
	"base.css",
	"jquip_sizzle.js",
	"juicer.js",
	"ui.js",
	"init.js",
	"adminMenu.js",
	"userMenu.js",
	"top_bg.jpg",
	"nav_line.jpg",
	"icon_menu_li.jpg",
	"share.js",
	"btn_refresh.gif",
	"btn_orange2.gif",
	"php-crypt-md5.js",
	"md5.js",
	"messages",
	"ram_log",
};

static char *deo_user_web_page_list[DEO_MAX_USER_WEB_PAGE_LIST] = 
{
	"status.asp",
	"status_ipv6.asp",
	"status_pon.asp",
	"lan_port_status.asp",
	"tcpiplan.asp",
	"deo_multi_wan_generic.asp",
	"dhcpd.asp",
	"ddns.asp",
	"access_limit.asp",
	"client_info.asp",
	"algonoff.asp",
	"fw-ipportfilter.asp",
	"fw-macfilter.asp",
	"fw-portfw.asp",
	"static-mapping.asp",
	"fw-dmz.asp",
	"arptable.asp",
	"lbd.asp",
	"ipv6_enabledisable.asp",
	"fw-ipportfilter-v6.asp",
	"ping.asp",
	"ping_result.asp",
	"ping6.asp",
	"ping6_result.asp",
	"tracert.asp",
	"tracert_result.asp",
	"tracert6.asp",
	"tracert6_result.asp",
	"grouptable.asp",
	"grouptable_result.asp",
	"ssh.asp",
	"ssh_result.asp",
	"portmirror.asp",
	"portmirror_result.asp",
	"reboot.asp",
	"saveconf.asp",
	"user_syslog.asp",
	"syslog_server.asp",
	"dos.asp",
	"password.asp",
	"upgrade.asp",
	"autoreboot.asp",
	"natsession.asp",
	"useage.asp",
	"selfcheck.asp",
	"tz.asp",
	"logout.asp",
	"stats.asp",
	"pon-stats.asp",
	"login.asp",
	"login_base.asp",
	"login.cgi",
	"usereg.asp",
	"useregresult.asp",
	"terminal_inspec.asp",
	"dl_notify.asp",
	"net_qos_cls_edit.asp",
	"net_qos_traffictl_edit.asp",
	"dhcptbl.asp",
	"portBaseFilterDhcp.asp",
	"macIptbl.asp",
	"lanApConnection.asp",
	"wanApConnection.asp",
	"invalid_page.asp",
	"formWanApConnection",
	"formUpload",
	"formWanRedirect",
	"formPing",
	"formTracert",
	"formSshResult",
	"formNetstatResult",
	"formPortMirrorResult",
	"formGroupTable",
	"formSsh",
	"formNetstat",
	"formPortMirror",
	"formBankStatus",
	"formUseageSetup",
	"formNatSessionSetup",
	"formDosCfg",
	"formAutoReboot",
	"formNtp",
	"formPasswordSetup",
	"formTcpipLanSetup",
	"formWanEth",
	"formDhcpServer",
	"formDDNS",
	"formAccessLimit",
	"formReflashClientTbl",
	"formNickname",
	"formALGOnOff",
	"formFilter",
	"formPortFw",
	"formStaticMapping",
	"formDMZ",
	"formRefleshFdbTbl",
	"formLBD",
	"formWanEth",
	"formFilterV6",
	"formReboot",
	"formSaveConfig",
	"formStats",
	"formStatus",
	"formLANPortStatus",
	"formIgmproxy",
	"formSnmpConfig",
	"formMLDProxy",
	"formMLDSnooping",
	"formV6ACL",
	"formLogout",
	"formmacBase",
	"formSysLog",
	"userLogin",
	"bootstrap",
	"captcha.png",
	"common.js",
	"common.css",
	"base64_code.js",
	"sha256_code.js",
	"default.css",
	"btn_login_bg.gif",
	"login_bg_new.jpg",
	"icon_login.png",
	"logo.gif",
	"bg_ah_login_01.gif",
	"bg_ah_login.gif",
	"loid_register.gif",
	"loidreg_ah_01.gif",
	"loidreg_ah.gif",
	"favicon.ico",
	"sk_loing_logo.png",
	"reset.css",
	"style.css", 
	"jquip.js",
	"base.css",
	"jquip_sizzle.js",
	"juicer.js",
	"ui.js",
	"init.js",
	"adminMenu.js",
	"userMenu.js",
	"top_bg.jpg",
	"nav_line.jpg",
	"icon_menu_li.jpg",
	"share.js",
	"btn_refresh.gif",
	"btn_orange2.gif",
	"php-crypt-md5.js",
	"md5.js",
	"messages",
	"ram_log",
};


int checkNL_CR(char *url)
{
	if ((strstr(url, "%0d") != NULL) || (strstr(url, "%0D") != NULL) ||
		(strstr(url, "%0a") != NULL) || (strstr(url, "%0A") != NULL))
	{
		DBG_ARG("\n%s %d %s NL & CR check ",__func__, __LINE__, url);
		return 1;
	}

	if ((strstr(url, "%0d%0a") != NULL) || (strstr(url, "%0D%0a") != NULL) ||
		(strstr(url, "%0d%0A") != NULL) || (strstr(url, "%0D%0A") != NULL))
	{
		DBG_ARG("\n%s %d %s NL & CR check ",__func__, __LINE__, url);
		return 1;
	}

	if ((strstr(url, "%0a%0d") != NULL) || (strstr(url, "%0A%0d") != NULL) ||
		(strstr(url, "%0a%0D") != NULL) || (strstr(url, "%0A%0D") != NULL))
	{
		DBG_ARG("\n%s %d %s NL & CR check ",__func__, __LINE__, url);
		return 1;
	}

	return 0;
}
	
int checkAdminWhiteListPage(char *url)
{
	int ix;
	int ret = 1;

	for (ix = 0; ix < DEO_MAX_ADMIN_WEB_PAGE_LIST; ix++)
	{
		if (deo_admin_web_page_list[ix] == NULL)
			break;

		if (strstr(url, deo_admin_web_page_list[ix]) != NULL)
		{
			ret = 0;
			break;
		}
	}

	if (strcmp(url, "/") == 0)
		ret = 0;

	return ret;
}

int checkUserWhiteListPage(char *url, char *host)
{
	int ix;
	int ret = 1;
	char wan_ip[16] = {0,};

	for (ix = 0; ix < DEO_MAX_USER_WEB_PAGE_LIST; ix++)
	{
		if (deo_user_web_page_list[ix] == NULL)
			break;

		/* exception case, admin selfcheck. */
		if (strstr(url, "netstat_result.asp") != NULL)
		{
			ret = 0;
			break;
		}
	
		if (strstr(url, "netstat.asp") != NULL)
		{
			if (strstr(host, "8080") != NULL)
			{
				ret = 0;
				break;
			}
		}

		if (strstr(url, deo_user_web_page_list[ix]) != NULL)
		{
			/* exception case */
			if (strstr(url, "app_mld_snooping.asp") != NULL)
			{
				if (strcmp(deo_user_web_page_list[ix], "ping.asp") == 0)
					continue;
			}

			ret = 0;
			break;
		}
	}

	if (strcmp(url, "/") == 0)
		ret = 0;

	return ret;
}

static int deo_redirect_login_web(request *req)
{
	boaWrite(req, "HTTP/1.0 302 Moved Temporarily\r\n");
	boaWrite(req, "Content-Type: text/html; charset=UTF-8\r\n");
	boaWrite(req, "Set-Cookie: sessionid=0;  Max-Age=0; Path=/; \r\n");
	boaWrite(req, "\r\n");
	boaWrite(req, "<html><head>\r\n"
	"<style>#cntdwn{ border-color: white;border-width: 0px;font-size: 12pt;color: red;text-align:center; font-weight:bold; font-family: Courier;}\r\n"
	"body { background-repeat: no-repeat; background-attachment: fixed;     background-position: center;   text-align:left;}\r\n"
	"div.msg { font-size: 20px; top: 100px; width: 470px; text-align:center;}\r\n"
	"#progress-boader{ border: 2px #ccc solid;border-radius: 10px;height: 25; top: 10px; width :420px; text-align:center ;left: 30px}\r\n"
	"</style>\r\n"
	"<script language=javascript>\r\n"
	"window.open(\"/admin/login.asp\",target=\"_top\"); \r\n"
	"</script></head>\r\n"
	"<body>");
	req_write(req, "</body></html>\n");

	return 0;
}

/*
 * Name: process_header_end
 *
 * Description: takes a request and performs some final checking before
 * init_cgi or init_get
 * Returns 0 for error or NPH, or 1 for success
 */
#ifndef CONFIG_USER_BOA_CSRF
#define NO_REFERER_LOG
#endif
int process_header_end(request * req)
{
#ifdef CHECK_IP_MAC
	char mac[18];  /*Naughty, assuming we are on ethernet!!!*/
#endif /*CHECK_IP_MAC*/

	if (!req->logline) {
		send_r_error(req);
		return 0;
	}
	/*MATT2 - I figured this was a good place to check for the MAC address*/
#ifdef CHECK_IP_MAC
	if(get_mac_from_IP(mac, req->remote_ip_addr))
		do_mac_crap(req->remote_ip_addr, mac);
	else;
		/*they could be on a remote lan, or just not in the arp cache*/
#endif



#ifdef USE_BROWSERMATCH
       browser_match_request(req);
#endif

#ifndef NO_REFERER_LOG
	if (req->referer)
		log_referer(req);
#endif

#ifndef NO_AGENT_LOG
	if (req->user_agent)
		log_user_agent(req);
#endif

	if((req->method == M_GET) && (req->request_uri !=NULL) && attack_req_check(req)) {
#ifdef CU_APPSCAN_RSP_DBG
		send_appscan_response(req);
#else
		send_r_bad_request(req);
#endif
		return 0;
	}

        if (translate_uri(req) == 0) {  /* unescape, parse uri */
                SQUASH_KA(req);
                return 0;               /* failure, close down */
        }

#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
	if((req->method == M_GET))
	{
		if(req->request_from == 2)
		{
			char cmd_str[256]={0};
			char addrstr[64]={0};
#ifdef CONFIG_IPV6
			int is_v6 = 0;
#endif
			if(strncmp(req->remote_ip_addr,"::ffff:",7) == 0)
			{
				va_cmd(IPSET, 4, 1, "-!", "add", FWUPGRADE_SET, req->remote_ip_addr+7);		
			}
#ifdef CONFIG_IPV6
			else
			{
				struct ipv6_ifaddr ip6_addr;
				int numOfIpv6;
				va_cmd(IPSET, 4, 1, "-!", "add", FWUPGRADE_SETV6, req->remote_ip_addr);
				numOfIpv6 = getifip6("br0", IPV6_ADDR_UNICAST, &ip6_addr, 1);
				if(numOfIpv6 > 0)
				{
					inet_ntop(AF_INET6, (void*)&(ip6_addr.addr), addrstr, sizeof(addrstr));
					is_v6 = 1;
				}
				else
				{
					numOfIpv6 = getifip6("br0", IPV6_ADDR_LINKLOCAL, &ip6_addr, 1);
					if(numOfIpv6 > 0)
					{
						inet_ntop(AF_INET6, (void*)&(ip6_addr.addr), addrstr, sizeof(addrstr));
						is_v6 = 1;
					}
				}
			}
#endif
			
			if(!addrstr[0]) {
				struct in_addr lan_ip={0};
				mib_get_s(MIB_ADSL_LAN_IP, (void *)&lan_ip, sizeof(lan_ip));
				inet_ntop(AF_INET, (void*)&lan_ip, addrstr, sizeof(addrstr));
#ifdef CONFIG_IPV6
				is_v6 = 0;
#endif
			}

			cmd_str[0] = '\0';
			if(addrstr[0]) 
			{
#ifdef CONFIG_IPV6
				if(is_v6) {
#ifdef CONFIG_YUEME
					snprintf(cmd_str, sizeof(cmd_str), "http://[%s]:8080/dl_notify.asp", addrstr);
#endif
				}
				else
#endif
				{
#ifdef CONFIG_YUEME
					snprintf(cmd_str, sizeof(cmd_str), "http://%s:8080/dl_notify.asp", addrstr);
#endif
				}
			} 
			send_popwin_and_reload(req, cmd_str);
			//send_redirect_temp(req, popurl);
			return 0;
		}
	}
#endif
#if defined(CONFIG_YUEME) && defined(CONFIG_USER_DBUS_PROXY)
#if 1 //FIXME!!!
	//cxy 2018-4-4 redirect boa web login to appframework login
	if(search_login_list(req) == NULL)
	{
		//printf("===>>method %d requesturi: %s, query_string: %s\n\n",req->method,req->request_uri, req->query_string==NULL?"NULL":req->query_string);
		//wpeng 2018-4-5, fast maintain do not need redirect to port 80..
		if ((req->method == M_GET) && strcmp(req->request_uri, "/maintain")==0 && req->query_string==NULL)
		{
			/* I think it is better to redirect in auth_authorize, do nothing here */
			printf("set fast_maintain!\n");
			fast_maintain = 1;
		}
		else if (((req->method == M_GET) &&  (0 != fast_maintain) &&
					(strcmp(req->request_uri, "/admin/login.asp")==0 || 
					strcmp(req->request_uri, "/usereg.asp")==0 ||
					strcmp(req->request_uri, "/useregresult.asp")==0 ||
					strcmp(req->request_uri, "/common.js")==0 ||
					strcmp(req->request_uri, "/base64_code.js")==0 ||
					strcmp(req->request_uri, "/style/default.css")==0 ||
					strcmp(req->request_uri, "/base64_code.js")==0 ||
					strcmp(req->request_uri, "/sha256_code.js")==0 ||
					strcmp(req->request_uri, "/image/logo.gif")==0 ||
					strcmp(req->request_uri, "/image/bg_ah_login_01.gif")==0 ||
					strcmp(req->request_uri, "/image/bg_ah_login.gif")==0) ||
					strcmp(req->request_uri, "/image/loid_register.gif") == 0 ||
					strcmp(req->request_uri, "/image/loidreg_ah_01.gif") == 0 ||
#ifdef TERMINAL_INSPECTION_SC
					strcmp(req->request_uri, "/terminal_inspec.asp") == 0 ||
#endif
#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
					strcmp(req->request_uri, "/dl_notify.asp") == 0 ||
#endif
					strcmp(req->request_uri, "/image/loidreg_ah.gif") == 0) 
				|| ((req->method == M_POST) && (0 != fast_maintain) && strcmp(req->request_uri, "/boaform/formUserReg")==0)
				)
		{
			if (req->query_string != NULL)
			{
				free(req->query_string);
				req->query_string = NULL;
			}
		}
		else if (!(((req->method == M_GET) && strcmp(req->request_uri, "/boaform/admin/userLogin")==0 && req->query_string!=NULL) 
			|| ((req->method == M_POST) && strcmp(req->request_uri, "/boaform/admin/userLogin")==0)
			|| strcmp(req->request_uri,"/login.cgi")==0))
		{
			char tmpbuf[MAX_URL_LEN];
			if(req->host)
			{
				if(strlen(req->host)>=128)
					dump_request(req);
				int len;
				char *h , *ptmp;
				h = strrchr(req->host, ':');
				if(h && !strrchr(h, ']')) *h = '\0';
				else h = NULL;
				snprintf(tmpbuf, sizeof(tmpbuf)-1 ,"http://%s/", req->host);
				if(h) *h = ':';
			}
			else
			{
#ifdef IPV6
				if(req->addr_family == AF_INET6)
				{
					unsigned char ipv6_lan_ip[64];
					mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)&ipv6_lan_ip, sizeof(ipv6_lan_ip));
					sprintf(tmpbuf, "http://[%s]/", ipv6_lan_ip);
				}
				else
#endif
				{
					struct in_addr lan_ip;
					mib_get_s(MIB_ADSL_LAN_IP, (void *)&lan_ip, sizeof(lan_ip));
					sprintf(tmpbuf, "http://%s/", inet_ntoa(*(struct in_addr *)&lan_ip));
				}
#if defined(CONFIG_USER_RTK_NESSUS_SCAN_FIXED)
				//fix nessus Web Server HTTP Header Internal IP Disclosure 10759
				unsigned char dhcpDomain[MAX_NAME_LEN];
				mib_get_s(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)dhcpDomain, sizeof(dhcpDomain));
				sprintf(tmpbuf, "http://%s/", dhcpDomain);	
#endif
			}
			//printf("===>>method %d requesturi: %s\n",req->method,req->request_uri);
			send_redirect_temp(req,tmpbuf);
			return 0;
		}
	}
	else
	{
		if ((req->method == M_GET) && 
			(strcmp(req->request_uri, "/maintain")==0) &&
			req->query_string==NULL)
		{
			boaRedirectTemp(req, "/usereg.asp");
			return 0;
		}
	}
#endif
#endif
#ifdef CONFIG_CU
	//detect appscan start
	if ((req->method == M_GET)){
		if(strcasestr(req->request_uri, "AppScan_fingerprint")){
			g_appscan=1;
			TIMEOUT_F(clear_appscan, 0, 1800, t_appscan);
		}
	}

	if ((req->method == M_GET)){
		if(strcasestr(req->request_uri, "/bottom.html")){
			send_r_not_found(req);
			return 0;
		}
		if(strcasestr(req->request_uri, "/cgi-bin/logs")){
			send_r_not_found(req);
			return 0;
		}
	}
#endif

#ifdef WLAN_WEB_REDIRECT //jiunming,web_redirect
	if(req->request_from==1)
	{
		char tmpbuf[MAX_URL_LEN];

		if( !mib_get_s(MIB_WLAN_WEB_REDIR_URL, (void*)tmpbuf, sizeof(tmpbuf)) )
		{
			send_r_error(req);
		}else{
			char tmpbuf2[MAX_URL_LEN+10], *phttp;

			phttp = strstr(tmpbuf, "http://");
			if( phttp==NULL )
				sprintf(tmpbuf2,"http://%s", tmpbuf);
			else
				sprintf(tmpbuf2,"%s", phttp);
			send_redirect_temp(req,tmpbuf2);
		}
		return 0;
	}
#endif

#ifdef USE_DEONET /* sghong, 20230913, web 접속 제한 */
	if (req->request_from == 3)
	{
		send_web_redirect_msg(req);
		return 0;
	}
#endif

#ifdef CONFIG_CMCC_FORWARD_RULE_SUPPORT
int total = 0,i = 0;
MIB_CMCC_FORWARD_RULE_T entry;
int iptype = 0;;
int ret = 0;
char tmp[MAX_URL_LEN];

total = mib_chain_total(MIB_CMCC_FORDWARD_RULE_TBL);
for(i=0; i < total; i++){
	if(!mib_chain_get(MIB_CMCC_FORDWARD_RULE_TBL, i, &entry))
		continue;	
	iptype = checkIPv4OrIPv6(entry.remoteAddress, entry.forwardToIP);
	if((iptype == 0 ||iptype == 3 || iptype == 6)){
		printf("[%s:%d] %s %s %s \n",__func__,__LINE__, req->request_uri, req->host,entry.remoteAddress);
		if(strstr(req->host, entry.remoteAddress)){
			sprintf(tmp,"http://%s:%d", entry.forwardToIP,entry.forwardToPort);
			send_redirect_temp(req,tmp);
		}
	}
}
#endif

#ifdef WEB_REDIRECT_BY_MAC
	if(req->request_mac==1)
	{
		char tmpbuf[MAX_URL_LEN];
		char mac[18];

		memset( mac, 0, sizeof(mac) );
		if( get_mac_from_IP(mac, req->remote_ip_addr) )
		{
			//printf( "\na new connection from %s:%s \n", req->remote_ip_addr, mac );
			if( handle_mac_for_webredirect( mac )<0 )
			{
				send_r_error(req);
			}else{
				if( !mib_get_s(MIB_WEB_REDIR_BY_MAC_URL, (void*)tmpbuf, sizeof(tmpbuf)) )
				{
					send_r_error(req);
				}else{
					char tmpbuf2[MAX_URL_LEN+10], *phttp;

					phttp = strstr(tmpbuf, "http://");
					if( phttp==NULL )
						sprintf(tmpbuf2,"http://%s", tmpbuf);
					else
						sprintf(tmpbuf2,"%s", phttp);
					send_redirect_temp(req,tmpbuf2);
					//send_popwin_and_reload( req,tmpbuf2 );
				}
			}
		}else
			send_r_error(req);
		return 0;
	}
#endif

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	if(req->request_captiveportal == 1)
	{
		char cp_url[MAX_URL_LEN+10];
		char *cp_url_ptr=NULL;
		char url_tmp[MAX_URL_LEN+10]={0};
		struct hostent *hp;
		
		/* Internet Succeed */
		if(mib_get_s(MIB_CAPTIVEPORTAL_URL, (void *)cp_url, sizeof(cp_url)))
		{
			cp_url_ptr = cp_url;
			while(*cp_url_ptr){
				if(*cp_url_ptr != ' ')
					break;
				cp_url_ptr++;
			}
			if(strncmp(cp_url_ptr, "http://", 7) && strncmp(cp_url_ptr, "https://", 8)){
				snprintf(url_tmp, sizeof(url_tmp)-1, "http://%s",cp_url_ptr);
				url_tmp[sizeof(url_tmp)-1]='\0';
			}else{
				strncpy(url_tmp, cp_url_ptr, sizeof(url_tmp)-1);
				url_tmp[sizeof(url_tmp)-1]='\0';			
			}
			send_redirect_temp(req, url_tmp);
			stop_captiveportal();
		}
		else
			send_r_error(req);

		return 0;
	}
#endif

#ifdef USER_WEB_WIZARD
	{
		unsigned char value[6], webwizard_flag;

		if (mib_get_s(MIB_ADSL_LAN_IP, (void *)value, sizeof(value)) != 0) {
			char ipaddr[16], tmpbuf[50];
			strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
			ipaddr[15] = '\0';
			if (mib_get_s(MIB_USER_WEB_WIZARD_FLAG, (void *)&webwizard_flag, sizeof(webwizard_flag))) {
			#ifdef CONFIG_00R0
				unsigned char redirectTroubleWizard;
				mib_get_s(RS_WEB_WIZARD_REDIRECTTROUBLEWIZARD, &redirectTroubleWizard, sizeof(redirectTroubleWizard));
				//printf("[%s:%d] webwizard_flag=%d, redirectTroubleWizard=%d\n",__func__,__LINE__,webwizard_flag,redirectTroubleWizard);
				if (webwizard_flag == 1) {
					if ((strstr(req->request_uri, "wizard_screen") == NULL) && (strstr(req->request_uri, ".jpg") == NULL) && (strstr(req->request_uri, ".gif") == NULL)
						&& (strstr(req->request_uri, ".js") == NULL) && (strstr(req->request_uri, ".css") == NULL) && (strstr(req->request_uri, "boaform") == NULL)) {
						sprintf(tmpbuf, "http://%s/admin/wizard_screen_index.asp", ipaddr);
						send_redirect_temp(req, tmpbuf);
						return 0;
					}
				}			
				{
					extern int connectOnceUp;
					int isConnect = get_ponStatus();

					//printf("[%s:%d] %d %d \n",__func__,__LINE__, get_internet_status() , redirectTroubleWizard);
					if(isConnect == 0)  //Detected not plug pon cable!
					{
						if(get_internet_status() && redirectTroubleWizard )  //Internet Conncted and redirectTroubleWizard
						{
							printf("[%s:%d]  get_internet_status(%d) redirectTroubleWizard(%d) strlen(req->request_uri)=%d\n",
									__func__,__LINE__, get_internet_status() , redirectTroubleWizard, strlen(req->request_uri));
							if(redirectTroubleWizard==1)
							{
								if ((strstr(req->request_uri, "wizard_screen") == NULL) && (strstr(req->request_uri, "trouble_wizard") == NULL) && (strstr(req->request_uri, ".jpg") == NULL)
										&& (strstr(req->request_uri, ".js") == NULL) && (strstr(req->request_uri, ".css") == NULL) && (strstr(req->request_uri, "boaform") == NULL)
										&& (strstr(req->request_uri, ".gif") == NULL))
								{
									sprintf(tmpbuf, "http://%s/admin/trouble_wizard_index.asp", ipaddr);
									send_redirect_temp(req, tmpbuf);
									return 0;
								}
							}
							else if(redirectTroubleWizard==2)
							{
								if (strlen(req->request_uri) < 2)
								{
									sprintf(tmpbuf, "http://%s/admin/trouble_wizard_index.asp", ipaddr);
									send_redirect_temp(req, tmpbuf);
									return 0;
								}
							}
						}
					}
				}
			#else
				if (webwizard_flag == 1) {
					if ((strstr(req->request_uri, "web_wizard") == NULL)
						&& (strstr(req->request_uri, ".js") == NULL) && (strstr(req->request_uri, ".css") == NULL) && (strstr(req->request_uri, "boaform") == NULL)) {
						sprintf(tmpbuf, "http://%s/web_wizard_index.asp", ipaddr);
						//printf("==========%s %d web redirect !!! webwizard_flag=%d\n", __FUNCTION__, __LINE__, webwizard_flag);
						send_redirect_temp(req, tmpbuf);
						return 0;
					}
				}
				else {
					if (strstr(req->request_uri, "web_wizard")!=NULL) {
						//printf("==========%s %d web redirect !!! webwizard_flag=%d\n", __FUNCTION__, __LINE__, webwizard_flag);
						if ((strstr(req->request_uri, "web_wizard") != NULL)) {
							sprintf(tmpbuf, "http://%s/index.html", ipaddr);
							//printf("==========%s %d web redirect !!! webwizard_flag=%d\n", __FUNCTION__, __LINE__, webwizard_flag);
							send_redirect_temp(req, tmpbuf);
							return 0;
						}
					}
				}
			#endif
			}
		}
	}
#endif

#ifdef USE_NLS
#ifdef USE_NLS_REFERER_REDIR
				if (!req->cp_name)
				{
					if (!nls_try_redirect(req))
						return 0;
				}
#endif
				nls_set_codepage(req);
#endif


		int use_auth=1;
#ifdef USER_WEB_WIZARD //When in Wizard Page, don't need to do auth
		unsigned char  webwizard_flag;
		if (mib_get_s(MIB_USER_WEB_WIZARD_FLAG, (void *)&webwizard_flag, sizeof(webwizard_flag))) {
			if (webwizard_flag == 1) {
				use_auth=0;
			}
		}
#endif
#ifdef USE_AUTH
#ifdef USE_DEONET
		if(strcmp(req->request_uri, "/admin/lanApConnection.asp")==0
		   ||strcmp(req->request_uri, "/admin/wanApConnection.asp")==0
		   ||strcmp(req->request_uri, "/boaform/admin/formWanApConnection")==0)
			use_auth = 0;
#endif
		if(use_auth){
			closelog();
			openlog("boa", LOG_PID, LOG_AUTHPRIV);
			if (!auth_authorize(req)) {
				openlog("boa", LOG_PID, 0);
				return 0;
			}
		}
#endif
	{
		if (strstr(req->client_stream, ".asp") != NULL)
		{
			if (strstr(req->client_stream, "login.asp") == NULL)
			{
				/*
				 * 직접 입력 
				 * client_stream : [GET /status.asp HTTP/1.1]
				 * reperer : NULL
				 * 웹 선택 
				 * client_stream : [GET /status.asp?v=1722552926000 HTTP/1.1]
				 * reperer : http://192.168.75.1/
				 */
				if (strstr(req->client_stream, "?v=") == NULL)
				{
					if (req->referer == NULL)
					{
						deo_redirect_login_web(req);
						req_flush(req);
						return 0;
					}
				}
			}
		}

		if (strcmp(g_login_username, "root") == 0)
		{
			if(checkAdminWhiteListPage(req->request_uri))
			{
				deo_redirect_login_web(req);
				req_flush(req);
				return 0;
			} 
		}
		else if (strcmp(g_login_username, "admin") == 0)
		{
			if(checkUserWhiteListPage(req->request_uri, req->host))
			{
				deo_redirect_login_web(req);
				req_flush(req);
				return 0;
			} 
		}
	}

	if (req->query_string != NULL)
	{
		if (strstr(req->query_string, "redirect-url") != NULL)
		{
			deo_redirect_login_web(req);
			req_flush(req);
			return 0;
		}
	}

	if (req->method == M_POST) {
		#ifdef CONFIG_HTTP_FILE
		char *tmpfilep = "/proc/rtk/http_buf";
		#else
		char tmpfilep[] = "/tmp/rtktmpXXXXXX";
		#endif

		/* open temp file for post data */
		if ((req->post_data_fd = mkstemp(tmpfilep)) == -1) {
#if 0
			boa_perror(req, "tmpfile open");
#endif
			return 0;
		}
		req->post_file_name = strdup(tmpfilep);
		return 1;
	}
#ifdef CONFIG_VIR_BOA_JEMBENCHTEST
	if(strstr(req->request_uri,"jembenchtest")){
		req->is_cgi=1;
	}
#endif

#ifdef CONFIG_CMCC_ENTERPRISE
	if(strcmp(req->request_uri,"/")==0 ||
		strstr(req->request_uri,"menu_dir.js") ||
		strstr(req->request_uri,"index_user.html")
		){
		req->is_cgi=1;
	}
#endif
#ifdef SUPPORT_ZERO_CONFIG
	int isItms = 0;

	if(strstr(req->request_uri, "itms"))
	{	
		isItms = 1;
		strcpy(req->request_uri, "/boaform/itms");
	}
	if (req->is_cgi || isItms)
#else
	if (req->is_cgi)
#endif
#ifdef SUPPORT_ASP
#ifdef CONFIG_E8B
		if(strstr(req->request_uri, ".cgi"))
			return init_cgi(req);
		else
#endif
			return init_get2(req);
#else
		return init_cgi(req);
#endif
	req->status = WRITE;
	return init_get(req);		/* get and head */
}

/*
 * Name: process_option_line
 *
 * Description: Parses the contents of req->header_line and takes
 * appropriate action.
 */

int process_option_line(request * req)
{
	char c, *value, *line = req->header_line;
	int eat_line = 0;

/* Start by aggressively hacking the in-place copy of the header line */

#ifdef FASCIST_LOGGING
	fprintf(stderr, "\"%s\"\n", line);
#endif

	value = strchr(line, ':');
	if (value == NULL)
		return 0;
	*value++ = '\0';			/* overwrite the : */
	to_upper(line);				/* header types are case-insensitive */
	while ((c = *value) && (c == ' ' || c == '\t'))
		value++;

	if (!memcmp(line, "IF_MODIFIED_SINCE", 18) && !req->if_modified_since)
		req->if_modified_since = value;

	else if (!memcmp(line, "CONTENT_TYPE", 13) && !req->content_type)
		req->content_type = value;

	else if (!memcmp(line, "CONTENT_LENGTH", 15) && !req->content_length && atoi(value) >= 0)
		req->content_length = value;

	else if (!memcmp(line, "HOST",5) && !req->host)
		req->host = value;

#ifndef NO_REFERER_LOG
	else if (!memcmp(line, "REFERER", 8) && !req->referer)
		req->referer = value;
#else
	else if (!memcmp(line, "REFERER", 8) && !req->referer)
		req->referer = value;
#endif

#ifdef USE_AUTH
	else if (!memcmp(line,"AUTHORIZATION",14) && !req->authorization)
		req->authorization = value;
#endif

#ifndef NO_AGENT_LOG
        else if (!memcmp(line, "USER_AGENT", 11) && !req->user_agent)
                req->user_agent = value;
#endif

#ifndef NO_COOKIES
	else if (!memcmp(line, "COOKIE", 6) && !req->cookie) {
		req->cookie = value;
	}
#endif

	else if (!memcmp(line, "CONNECTION", 11) &&
			 ka_max &&
			 req->keepalive != KA_STOPPED)
#ifdef SUPPORT_ASP
		req->keepalive = KA_STOPPED;
#else
		req->keepalive = (!strncasecmp(value, "Keep-Alive", 10) ?
						  KA_ACTIVE : KA_STOPPED);
#endif

#ifdef ACCEPT_ON
	else if (!memcmp(line, "ACCEPT", 7)) {
		add_accept_header(req, value);
		eat_line = 1;
	}
#endif

	/* Silently ignore unknown header lines unless is_cgi */

	else  {
		add_cgi_env(req, line, value);
		eat_line = 1;
	}

	if (eat_line) {
		int throw = (req->header_end - req->header_line) + 1;
		memmove(req->header_line, req->header_end + 1,
				CLIENT_STREAM_SIZE -
						((req->header_end + 1) - req->client_stream));
		req->client_stream_pos -= throw;
		return(throw);
	}

	return 0;
}

/*
 * Name: add_accept_header
 * Description: Adds a mime_type to a requests accept char buffer
 *   silently ignore any that don't fit -
 *   shouldn't happen because of relative buffer sizes
 */

void add_accept_header(request * req, char *mime_type)
{
#ifdef ACCEPT_ON
	int l = strlen(req->accept);

	if ((strlen(mime_type) + l + 2) >= MAX_HEADER_LENGTH)
		return;

	if (req->accept[0] == '\0')
		strcpy(req->accept, mime_type);
	else {
		sprintf(req->accept + l, ", %s", mime_type);
	}
#endif
}

void free_requests(void)
{
	request *ptr, *next;

	ptr = request_free;
	while(ptr != NULL) {
		next = ptr->next;
		/*Free the socket stuff if it exists*/
#ifdef SUPPORT_ASP
		if(ptr->buffer!=NULL) free(ptr->buffer);
#endif
		if(ptr)	free(ptr);
		ptr = next;
	}
	request_free = NULL;
}

void init_free_requests(int num)
{
	request *req = NULL;
	int i;
	
	for(i=0;i<num;i++)
	{	
		req = (request *) malloc(sizeof(request));
		if (!req)
			die(OUT_OF_MEMORY);
		enqueue(&request_free, req);
	}
	
}

/*
 * Name: dump_request
 *
 * Description: Prints request to stderr for debugging purposes.
 */
void dump_request(request*req)
{
#if 0
	fputs("-----[ REQUEST DUMP ]-----\n",stderr);
	if (!req)
	{
		fputs("no request!\n",stderr);
		return;
	}
	fprintf(stderr,"Logline: %s\n",req->logline);
	fprintf(stderr,"request_uri: %s\n",req->request_uri);
	fprintf(stderr,"Pathname: %s\n",req->pathname);
	fprintf(stderr,"Status: %u\n",req->status);
	fprintf(stderr,"Host: %s\n",req->host);
	fprintf(stderr,"local_ip_addr: %s\n",req->local_ip_addr);
	fprintf(stderr,"remote_ip_addr: %s\n",req->remote_ip_addr);
#ifdef USE_NLS
	fprintf(stderr,"cp_name: %s\n",req->cp_name);
#endif
	fputs("---------------------------\n\n",stderr);
#endif
}


request *get_sock_request(int sock_fd)
{
	int fd = -1;						/* socket */
#ifdef IPV6
	struct sockaddr_in6 remote_addr;
	memset(&remote_addr, 0, sizeof(struct sockaddr_in6));
#else
	struct sockaddr_in remote_addr;		/* address */
	memset(&remote_addr, 0, sizeof(struct sockaddr_in));
#endif
	int remote_addrlen = sizeof(remote_addr);
	request *conn = NULL;				/* connection */

	if (max_connections != -1 && status.connections >= max_connections){
		DBG(printf("%s:%d: %ld reach max connections %d!\n",__func__,__LINE__,status.connections,max_connections);)
		return NULL;
	}
	
	conn = new_request();
	if(conn == NULL){ 
		DBG(printf("%s:%d: no free conn!\n",__func__,__LINE__);)
		return NULL;
	}

#ifdef IPV6
	remote_addr.sin6_family = 0xdead;
#else
  remote_addr.sin_family = 0xdead;
#endif
	fd = accept(sock_fd, (struct sockaddr *) &remote_addr, &remote_addrlen);

	if (fd == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)	/* no requests */
			goto error;
		else {					/* accept error */
			log_error_time();
#if 0
			perror("accept");
#endif
			goto error;
		}
	}
#ifdef DEBUGNONINET
	/*  This shows up due to race conditions in some Linux kernels
	 *  when the client closes the socket sometime between
	 *  the select() and accept() syscalls.
	 *  Code and description by Larry Doolittle <ldoolitt@jlab.org>
	 */
#define HEX(x) (((x)>9)?(('a'-10)+(x)):('0'+(x)))
	if (remote_addr.sin_family != AF_INET) {
		struct sockaddr *bogus = (struct sockaddr *) &remote_addr;
		char *ap, ablock[44];
		int i;
		close(fd);
#ifdef BOA_TIME_LOG
		log_error_time();
#endif
		for (ap = ablock, i = 0; i < remote_addrlen && i < 14; i++) {
			*ap++ = ' ';
			*ap++ = HEX((bogus->sa_data[i] >> 4) & 0x0f);
			*ap++ = HEX(bogus->sa_data[i] & 0x0f);
		}
		*ap = '\0';
#ifdef BOA_TIME_LOG
		fprintf(stderr, "non-INET connection attempt: socket %d, "
				"sa_family = %hu, sa_data[%d] = %s\n",
				fd, bogus->sa_family, remote_addrlen, ablock);
#endif
		goto error;
	}
#endif

	if ((setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
		sizeof(sock_opt))) == -1)
	{
#ifdef _DEBUG_MESSAGE
		fprintf(stderr, "[BOA] Error SO_REUSEADDR TCP_NODELAY\n");
#endif
	}

	conn->fd = fd;
	conn->status = READ_HEADER;
	conn->header_line = conn->client_stream;
	conn->time_last = time_counter;
#ifdef USE_CHARSET_HEADER
	conn->send_charset = 1;
#endif

	/* nonblocking socket */
	if (fcntl(conn->fd, F_SETFL, NOBLOCK) == -1) {
#ifdef BOA_TIME_LOG
		log_error_time();
		perror("request.c, fcntl");
#endif
	}
	/* set close on exec to true */
	if (fcntl(conn->fd, F_SETFD, 1) == -1) {
#ifdef BOA_TIME_LOG
		log_error_time();
		perror("request.c, fcntl-close-on-exec");
#endif
	}

	/* large buffers */
	if (setsockopt(conn->fd, SOL_SOCKET, SO_SNDBUF, (void *) &sockbufsize,
				   sizeof(sockbufsize)) == -1)
	{
#ifdef _DEBUG_MESSAGE
		fprintf(stderr, "[BOA] Error SO_SNDBUF TCP_NODELAY\n");
#endif
	}

	/* for log file and possible use by CGI programs */
#ifdef IPV6
#if 0
	if (getnameinfo((struct sockaddr *)&remote_addr,
									sizeof(remote_addr),
									conn->remote_ip_addr, 20,
								 	NULL, 0, NI_NUMERICHOST)) {
#ifdef _DEBUG_MESSAGE
			fprintf(stderr, "[IPv6] getnameinfo failed\n");
#endif
			conn->remote_ip_addr[0]=0;
		}
#else
	inet_ntop(remote_addr.sin6_family, &remote_addr.sin6_addr, conn->remote_ip_addr, sizeof(conn->remote_ip_addr));
#endif
#else
	strncpy(conn->remote_ip_addr, (char *) inet_ntoa(remote_addr.sin_addr), sizeof(conn->remote_ip_addr));
#endif

#ifdef IPV6
	if(strstr(&conn->remote_ip_addr[0], "%"))
		conn->addr_family = AF_INET6;
	else
		conn->addr_family = AF_INET;
#endif

	/* for possible use by CGI programs */
#ifdef IPV6
	conn->remote_port = ntohs(remote_addr.sin6_port);
#else
	conn->remote_port = ntohs(remote_addr.sin_port);
#endif

	DBG(printf("%s:%d: conn->remote_port=%d!\n",__func__,__LINE__,conn->remote_port);)

	if (virtualhost) {
#ifdef IPV6
		char host[20];
		struct sockaddr_in6 salocal;
		int dummy;

		dummy = sizeof(salocal);
		if (getsockname(conn->fd, (struct sockaddr *) &salocal, &dummy) == -1)
									      die(SERVER_ERROR);
			if (getnameinfo((struct sockaddr *)&salocal, sizeof(salocal),
			          host, 20, NULL, 0, NI_NUMERICHOST)) {
#ifdef _DEBUG_MESSAGE
				fprintf(stderr, "[IPv6] getnameinfo failed\n");
#endif
			}else
				conn->local_ip_addr = strdup(host);
#else
		struct sockaddr_in salocal;
		int dummy;

		dummy = sizeof(salocal);
		if (getsockname(conn->fd, (struct sockaddr *) &salocal, &dummy) == -1){
			goto error;
		}
		conn->local_ip_addr = strdup(inet_ntoa(salocal.sin_addr));
#endif
	}
	status.requests++;
	status.connections++;

	/* Thanks to Jef Poskanzer <jef@acme.com> for this tweak */
	{
		int one = 1;
		if (setsockopt(conn->fd, IPPROTO_TCP, TCP_NODELAY, (void *) &one,
			sizeof(one)) == -1)
		{
#ifdef _DEBUG_MESSAGE
			fprintf(stderr, "[BOA] Error setsockopt TCP_NODELAY\n");
#endif
		}
	}
	enqueue(&request_block, conn);
	return conn;
	
error:
	if(conn) free_request(NULL, conn);
	if(fd >= 0) close(fd);
	return NULL;
}

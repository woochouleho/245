/* vi:set tabstop=2 cindent shiftwidth=2: */
/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Some changes Copyright (C) 1996 Charles F. Randall <crandall@goldsys.com>
 *  Some changes Copyright (C) 1996 Larry Doolittle <ldoolitt@jlab.org>
 *  Some changes Copyright (C) 1996,97,98 Jon Nelson <nels0988@tc.umn.edu>
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

/* boa: boa.c */
// run BOA as console (menu-driven CLI)
//#define CONSOLE_BOA

// Added by davian_kuo
#include "./LINUX/form_src/multilang.h"
#include "./LINUX/form_src/multilang_set.h"

#include "asp_page.h"
#include "boa.h"
#include <rtk/rtk_timer.h>
#include <grp.h>
#include "syslog.h"
#include <sys/param.h>
#include <signal.h>
#ifdef SERVER_SSL
#ifdef USES_MATRIX_SSL
#include <sslSocket.h>
#include <pthread.h>
#else
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <pthread.h>
#endif /*USES_MATRIX_SSL*/
#endif /*SERVER_SSL*/

// Added by Mason Yu(2 level)
#include <stdio.h>
#ifdef USE_LIBMD5
#include <libmd5wrapper.h>
#else
#include "md5.h"
#endif //USE_LIBMD5
#include "./LINUX/mib.h"
#include "./LINUX/utility.h"
// Added by ql
#include "./LINUX/mib_reserve.h"

#include "msgq.h"
#include "./LINUX/debug.h"

#ifdef CONFIG_BOA_APPLY_FAST
#include "./LINUX/form_src/fmapply.h"
#endif

#ifdef PARENTAL_CTRL
#include <time.h>
#endif
#include <crypt.h>
#include "syslog2d.h"

struct web_socket_info *p_web_socket = NULL;

#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
struct http_session session_auth_array[HTTP_SESSION_MAX];
#endif

//add by ramen to use struct PPPOE_DRV_CTRL for save_pppoe_sessionid()
//#include "../../spppd/pppoe.h"
//#define MAX_NAME_LEN		30
//added by xl_yue
#ifdef ACCOUNT_LOGIN_CONTROL
#define	LOGIN_WAIT_TIME	60   //about 1 minute
#define	ACCOUNT_TIMEOUT		300	//about 5 minutes
#endif

char lock_ipv6=1;

#ifdef CONFIG_CU_BASEON_CMCC
	unsigned char web_style_ln=0;
#endif
#define        LOG_PID         0x01
//int server_s;					/* boa socket */

static int this_pid;

int server_sub_port = 8080;

#ifdef SERVER_SSL
#define SSL_KEYF "/etc/ssl_key.pem"
#define SSL_CERTF "/etc/ssl_cert.pem"
	//int server_ssl;				/*ssl socket */
	int mac_redir_server_ssl;
	int do_ssl = 1;					/*We want to actually perform all of the ssl stuff.*/
	int do_sock = 1;				/*We may not want to actually connect to normal sockets*/
#ifdef USES_MATRIX_SSL
	sslConn_t		*mtrx_cp = NULL;
	sslKeys_t			*mtrx_keys;
#else
	SSL_CTX *ctx;				/*SSL context information*/
	const SSL_METHOD *meth;			/*SSL method information*/
#endif
	int ssl_server_port = 443;		/*The port that the server should listen on*/
	int ssl_server_port_web_redirect = 10443;
	/*Note that per socket ssl information is stored in */
#ifdef IPV6
	struct sockaddr_in6 server_sockaddr;		/* boa ssl socket address */
#else
	struct sockaddr_in ssl_server_sockaddr;		/* boa ssl socket address */
#endif/*IPV6*/
extern int InitSSLStuff(void);
extern void get_ssl_request(int sock_fd);
#endif /*SERVER_SSL*/

unsigned int signal_action = 0;

#ifdef BOA_DEBUG
int debug_flag = 0;
#endif

int backlog = SO_MAXCONN;
#ifdef IPV6
struct sockaddr_in6 server_sockaddr;		/* boa socket address */
#else
struct sockaddr_in server_sockaddr;		/* boa socket address */
#endif

struct timeval req_timeout;		/* timeval for select */

extern char *optarg;			/* getopt */

fd_set block_read_fdset;
fd_set block_write_fdset;

int lame_duck_mode = 0;

time_t time_counter = 0;
#if defined(SUPPORT_URL_FILTER) && defined(SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC)
int ntp_synced=0;
#endif
int sock_opt = 1;
int do_fork = 1;

static int max_fd = 0;

// Added by Mason Yu
char suName[MAX_NAME_LEN];
char usName[MAX_NAME_LEN];

//add by xl_yue
#ifdef USE_LOGINWEB_OF_SERVER
#ifdef ONE_USER_LIMITED
struct account_status suStatus;
struct account_status usStatus;
#endif

struct user_info * user_login_list = NULL;

#ifdef LOGIN_ERR_TIMES_LIMITED
struct errlogin_entry * errlogin_list = NULL;
#endif

#endif

#ifdef CONFIG_USER_BOA_CSRF_TOKEN_INDEPDENT
struct token_info * user_token_list = NULL;
#endif

//add by xl_yue
#ifdef ACCOUNT_LOGIN_CONTROL
struct account_info su_account;
struct account_info us_account;
struct errlogin_entry * errlogin_list = NULL;
#endif

#ifdef CONFIG_CU
unsigned char appscan_flag = 0;
#ifdef CU_APPSCAN_RSP_DBG
unsigned int appscan_rsp_flag = 0;
#endif
#endif

#ifdef CONFIG_VIR_BOA_JEMBENCHTEST
extern void rtk_cmcc_do_jspeedup();
extern void rtk_cmcc_clear_jspeedup();
#endif

#ifdef CU_APP_SCHEDULE_LOG
extern void GenerateScheduledLog(time_t ts, time_t te);
extern void UploadScheduledLog();
#endif

void free_web_socket_list(void)
{
	struct web_socket_info *p_entry = NULL;

	while (p_web_socket)
	{
		p_entry = p_web_socket;
		p_web_socket = p_web_socket->next;
		free(p_entry);
	}
	return;
}

void init_web_socket_list(void)
{
	if (p_web_socket)
	{
		free_web_socket_list();
	}
	p_web_socket = NULL;
	return;
}

void dump_web_socket_list(void)
{
	struct web_socket_info *p_entry = NULL;

	p_entry = p_web_socket;
	while (p_entry)
	{
		fprintf(stderr, "<%s:%d> socket:%d, ip_ver:%d, port:%d\n", __func__, __LINE__, p_entry->socket, p_entry->ip_ver, p_entry->port);
		p_entry = p_entry->next;
	}
	return;
}

struct web_socket_info *search_web_socket_list(int ip_ver, int port)
{
	struct web_socket_info *p_entry = NULL;
	for (p_entry = p_web_socket; p_entry != NULL; p_entry = p_entry->next)
	{
		if (p_entry->ip_ver == ip_ver && p_entry->port == port)
		{
			break;
		}
	}
	return p_entry;
}

int add_web_socket_list(int socket, int ip_ver, int port)
{
	struct web_socket_info *p_entry = NULL;

	p_entry = search_web_socket_list(ip_ver, port);
	if (p_entry == NULL)
	{
		p_entry = malloc(sizeof(struct web_socket_info));
		memset(p_entry, 0, sizeof(struct web_socket_info));
		p_entry->socket = socket;
		p_entry->ip_ver = ip_ver;
		p_entry->port = port;
		p_entry->next = p_web_socket;
		p_web_socket = p_entry;
	}
	return 0;
}

int delete_web_socket_list(struct web_socket_info *p_entry)
{
	if (p_entry)
	{
		struct web_socket_info **p_indirect = &p_web_socket;

		while (*p_indirect != p_entry)
		{
			p_indirect = &(*p_indirect)->next;
		}

		*p_indirect = p_entry->next;
		free(p_entry);
	}

	return 0;
}

int bind_web_socket(int port)
{
	int web_socket = -1;
	int socket_v4 = 0, socket_v6 = 0;
	struct web_socket_info *p_entry = NULL;

#if defined(WEB_HTTP_SERVER_BIND_IP)
	socket_v4 = 1;
#ifdef IPV6
	socket_v6 = 1;
#endif
#else
#ifdef IPV6
	socket_v6 = 1;
#else
	socket_v4 = 1;
#endif
#endif

	if (socket_v4)
	{
		p_entry = search_web_socket_list(4, port);
		if (p_entry == NULL)
		{
#ifdef IPV6
			web_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
#else
			web_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
			if (web_socket != -1)
			{
				//if do not set FD_CLOEXEC, igmpproxy will carry on this fd, case boa rebind port fail!
				if(-1 == fcntl(web_socket, F_SETFD, fcntl(web_socket, F_GETFD) | FD_CLOEXEC)) // FD_CLOEXEC: Let child process won't inherit this fd
				{
					perror("[boa]set ipv4 socket FD_CLOEXEC fail.");
				}
				add_web_socket_list(web_socket, 4, port);
			}
			else
			{
				syslog(LOG_ALERT, "%s, %i:Couldn't create socket for port %d(%s)",  __FILE__, __LINE__,port,strerror(errno));
				die(NO_CREATE_SOCKET);
				return 0;
			}
		}
	}

#ifdef IPV6
	if (socket_v6)
	{
		p_entry = search_web_socket_list(6, port);
		if (p_entry == NULL)
		{
			web_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
			if (web_socket != -1)
			{
				//if do not set FD_CLOEXEC, igmpproxy will carry on this fd, case boa rebind port fail!
				if(-1 == fcntl(web_socket, F_SETFD, fcntl(web_socket, F_GETFD) | FD_CLOEXEC)) // FD_CLOEXEC: Let child process won't inherit this fd
				{
					perror("[boa]set ipv6 socket FD_CLOEXEC fail.");
				}
				add_web_socket_list(web_socket, 6, port);
			}
			else
			{
				syslog(LOG_ALERT, "%s, %i:Couldn't create socket for port %d(%s)", __FILE__, __LINE__,port,strerror(errno));
				die(NO_CREATE_SOCKET);
				return 0;
			}
		}
	}
#endif

	p_entry = p_web_socket;
	while (p_entry)
	{
		if (p_entry->port == port)
		{
			fprintf(stderr, "<%s:%d> socket:%d, ip_ver:%d, port:%d\n", __func__, __LINE__, p_entry->socket, p_entry->ip_ver, p_entry->port);

			if (fcntl(p_entry->socket, F_SETFL, NOBLOCK) == -1)
			{
				syslog(LOG_ALERT, "%s, %i:Couldn't fcntl(%s)", __FILE__, __LINE__,strerror(errno));
				die(NO_FCNTL);
				return 0;
			}
			if (setsockopt(p_entry->socket, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt, sizeof(sock_opt)) == -1)
			{
				syslog(LOG_ALERT, "%s, %i:Couldn't sockopt(%s)", __FILE__,__LINE__,strerror(errno));
				die(NO_SETSOCKOPT);
				return 0;
			}

#if defined(RTK_WEB_HTTP_SERVER_BIND_DEV)
			struct ifreq ifr = {0};
			strcpy(ifr.ifr_name, LANIF);
			if (setsockopt(p_entry->socket, SOL_SOCKET, SO_BINDTODEVICE, (char *)(&ifr), sizeof(ifr)) == -1)
			{
				syslog(LOG_ALERT, "%s, %i:Couldn't sockopt(%s)", __FILE__,__LINE__,strerror(errno));
				die(NO_SETSOCKOPT);
				return 0;
			}
#endif

			if (p_entry->ip_ver == 4)
			{
#ifdef IPV6
				struct sockaddr_in6 sa;
#else
				struct sockaddr_in sa;
#endif
				memset(&sa, 0, sizeof(sa));
#if defined(WEB_HTTP_SERVER_BIND_IP)
				char ip_str[64] = {0};
				getMIB2Str_s(MIB_ADSL_LAN_IP, ip_str, sizeof(ip_str));

				if (strlen(ip_str) > 0)
				{
#ifdef IPV6
					char ip6_str[INET6_ADDRSTRLEN] = {0};
					snprintf(ip6_str, sizeof(ip6_str), "::ffff:%s", ip_str);
					if (inet_pton(AF_INET6, ip6_str, &(sa.sin6_addr)) != 1)
					{
						syslog(LOG_ALERT, "%s, %i:Couldn't bind to port %d(%s)", __FILE__, __LINE__,p_entry->port,strerror(errno));
						die(NO_BIND);
						return 0;
					}
					sa.sin6_family = AF_INET6;
					sa.sin6_port = htons(p_entry->port);
#else
					if (inet_pton(AF_INET, ip_str, &(sa.sin_addr)) != 1)
					{
						syslog(LOG_ALERT, "%s, %i:Couldn't bind to port %d(%s)", __FILE__, __LINE__,p_entry->port,,strerror(errno));
						die(NO_BIND);
						return 0;
					}
					sa.sin_family = AF_INET;
					sa.sin_port = htons(p_entry->port);
#endif
				}
				else
				{
					syslog(LOG_ALERT, "%s, %i:Couldn't bind to port %d(%s)", __FILE__, __LINE__,p_entry->port,strerror(errno));
					die(NO_BIND);
					return 0;
				}
#else
#ifdef IPV6
				memcpy(&sa.sin6_addr, &in6addr_any, sizeof(in6addr_any));
				sa.sin6_family = AF_INET6;
				sa.sin6_port = htons(p_entry->port);
#else
				sa.sin_addr.s_addr = htonl(INADDR_ANY);
				sa.sin_family = AF_INET;
				sa.sin_port = htons(p_entry->port);
#endif
#endif

				if (bind(p_entry->socket, (struct sockaddr *)&sa, sizeof(sa)) == -1)
				{
					syslog(LOG_ALERT, "%s, %i:Couldn't bind to port %d(%s)", __FILE__, __LINE__,p_entry->port,strerror(errno));
					die(NO_BIND);
					return 0;
				}
			}
#ifdef IPV6
			else if (p_entry->ip_ver == 6)
			{
				struct sockaddr_in6 sa6;
				int on = 1, len = 0;
				memset(&sa6, 0, sizeof(sa6));
#if defined(WEB_HTTP_SERVER_BIND_IP)
				char ip_str[64] = {0}, port_str[8] = {0};
				struct addrinfo hints, *res;

#if defined(WEB_HTTP_SERVER_BIND_IP) && !defined(RTK_WEB_HTTP_SERVER_BIND_DEV)
				struct ifreq ifr = {0};
				strcpy(ifr.ifr_name, LANIF);
				if (setsockopt(p_entry->socket, SOL_SOCKET, SO_BINDTODEVICE, (char *)(&ifr), sizeof(ifr)) == -1)
				{
					syslog(LOG_ALERT, "%s, %i:Couldn't sockopt(%s)", __FILE__,__LINE__,strerror(errno));
					die(NO_SETSOCKOPT);
					return 0;
				}
#endif

				mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)ip_str, sizeof(ip_str));
				if (strlen(ip_str) > 0)
				{
					memset(&hints, 0, sizeof hints);
					hints.ai_family = AF_INET6;
					hints.ai_socktype = SOCK_STREAM;
					sprintf(port_str, "%d", p_entry->port);
					if (getaddrinfo(ip_str, port_str, &hints, &res) == 0)
					{
						memcpy(&sa6, res->ai_addr, res->ai_addrlen);
						len = res->ai_addrlen;
						freeaddrinfo(res);
					}
					else
					{
						syslog(LOG_ALERT, "%s, %i:Couldn't bind to port %d(%s)", __FILE__, __LINE__,p_entry->port,strerror(errno));
						die(NO_BIND);
						return 0;
					}
				}
				else
				{
					syslog(LOG_ALERT, "%s, %i:Couldn't bind to port %d(%s)", __FILE__, __LINE__,p_entry->port,strerror(errno));
					die(NO_BIND);
					return 0;
				}

				if (setsockopt(p_entry->socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(on)) == -1)
				{
					syslog(LOG_ALERT, "%s, %i:Couldn't sockopt(%s)", __FILE__,__LINE__,strerror(errno));
					die(NO_SETSOCKOPT);
					return 0;
				}
#else
				memcpy(&sa6.sin6_addr, &in6addr_any, sizeof(in6addr_any));
				sa6.sin6_family = AF_INET6;
				sa6.sin6_port = htons(p_entry->port);
				len = sizeof(sa6);
#endif

				if (bind(p_entry->socket, (struct sockaddr *)&sa6, len) == -1)
				{
					syslog(LOG_ALERT, "%s, %i:Couldn't bind to port %d(%s)", __FILE__, __LINE__,p_entry->port,strerror(errno));
					die(NO_BIND);
					return 0;
				}
			}
#endif
			if (max_connections != -1)
				backlog = MIN(backlog, max_connections);

			if (listen(p_entry->socket, backlog) == -1)
			{
				die(NO_LISTEN);
				return 0;
			}

			if (p_entry->socket > max_fd)
			{
				max_fd = p_entry->socket;
			}
		}
		p_entry = p_entry->next;
	}

	return 1;
}

int rebind_web_socket(void)
{
	close_web_socket(server_port);
	bind_web_socket(server_port);

	close_web_socket(server_sub_port);
	bind_web_socket(server_sub_port);

#if 0 // because ssl not open socket
#ifdef SERVER_SSL
	close_web_socket(ssl_server_port);
	bind_web_socket(ssl_server_port);
#endif
#endif

	dump_web_socket_list();

	return 1;
}

int close_web_socket(int port)
{
	struct web_socket_info *p_entry = NULL, *p_next = NULL;

	p_entry = p_web_socket;
	while (p_entry)
	{
		p_next = p_entry->next;
		if (p_entry->port == port)
		{
			FD_CLR(p_entry->socket, &block_read_fdset);
			close(p_entry->socket);
			delete_web_socket_list(p_entry);
		}
		p_entry = p_next;
	}

	return 1;
}

//add by xl_yue
#ifdef USE_LOGINWEB_OF_SERVER
#if defined(CONFIG_DEONET_SESSION)
struct user_info * search_login_list(request * req)
{
	struct user_info * pUser_info = NULL;
	char session_id[32] = {0,};

	if (req == NULL)
		return NULL;

	if (req->cookie == NULL) {
		for (pUser_info = user_login_list; pUser_info; pUser_info = pUser_info->next) {
			if (!strcmp(req->remote_ip_addr, pUser_info->remote_ip_addr)) {
				break;
			}
		}

		if (pUser_info) {
			// TODO: what for?
		}
	} else {
		if (get_sessionid_from_cookie(req, session_id) == 0)
			return NULL;

		for (pUser_info = user_login_list; pUser_info; pUser_info = pUser_info->next) {
			if (!strcmp(req->remote_ip_addr, pUser_info->remote_ip_addr)) {
				if (strlen(pUser_info->session_id) > 0) {
					if (!strcmp(session_id, pUser_info->session_id))
						break;
				} else {
					break;
				}
			}
		}
	}

	return pUser_info;
}

#elif defined(ONE_USER_BY_SESSIONID)
struct user_info * search_login_list(request * req)
{
	struct user_info * pUser_info = NULL;
	char sessid[32] = {0};

	if(req == NULL || req->cookie == NULL)
	{
		if (req != NULL)
		{
			//printf("\n=====>> %s request_uri:%s\n", __func__, req->request_uri);
			if (!strstr(req->request_uri, "login.asp"))
			{
				for(pUser_info = user_login_list;pUser_info;pUser_info = pUser_info->next){
					if(!strcmp(req->remote_ip_addr,pUser_info->remote_ip_addr))
						return pUser_info;
				}
			}
		}
		return NULL;
	}

	if(get_sessionid_from_cookie(req, sessid) == 0)
		return NULL;

	for(pUser_info = user_login_list;pUser_info;pUser_info = pUser_info->next)
	{
		if(!strcmp(sessid, pUser_info->paccount->sessionid)) {
			break;
		}
	}
	return pUser_info;
}
#else
struct user_info * search_login_list(request * req)
{
	struct user_info * pUser_info;
	for(pUser_info = user_login_list;pUser_info;pUser_info = pUser_info->next){
		if(!strcmp(req->remote_ip_addr,pUser_info->remote_ip_addr))
			break;
	}
	return pUser_info;
}
#endif

#ifdef CONFIG_DEONET_SESSION
int getNumLoginUserInfo()
{
	struct user_info * pUser_info = NULL;
	int num = 0;

	for (pUser_info = user_login_list; pUser_info; pUser_info = pUser_info->next) {
		num++;
	}

	return num;
}
#endif

void ulist_free_login_entry(struct user_info * bUser_info,struct user_info * pUser_info)
{
	if(pUser_info == user_login_list)
		user_login_list = pUser_info->next;
	else
		bUser_info->next = pUser_info->next;

	if(pUser_info->directory)
		free(pUser_info->directory);

#ifdef ONE_USER_LIMITED
	if(pUser_info->paccount)
		pUser_info->paccount->busy = 0;
#endif

	UNTIMEOUT_F(NULL,NULL,pUser_info->autologout);  // boa crash issue

	free(pUser_info);
}

#if defined(ONE_USER_BY_SESSIONID) || defined(CONFIG_DEONET_SESSION)
int free_from_login_list(request * req)
{
	struct user_info * pUser_info;
	struct user_info * bUser_info = NULL;
	char sessid[32] = {0};

	if (req == NULL) {
		printf("%s cookie: %p\n", __func__, req->cookie);
		return 0;
	}

	if (req->cookie != NULL && get_sessionid_from_cookie(req, sessid) != 0) {
		for (pUser_info = user_login_list; pUser_info; bUser_info = pUser_info,pUser_info = pUser_info->next) {
			if (!strcmp(pUser_info->session_id, sessid))
				break;
		}

		if (!pUser_info) {
			printf("%s no user_info found\n", __func__);
			return 0;
		}

		ulist_free_login_entry(bUser_info,pUser_info);
	}

	return 1;
}
int free_from_login_list_by_ip(request * req)
{
	struct user_info * pUser_info;
	struct user_info * bUser_info = NULL;
	char sessid[32] = {0};

	if (req == NULL) {
		return 0;
	}

	for (pUser_info = user_login_list; pUser_info; bUser_info = pUser_info,pUser_info = pUser_info->next) {
			if (!strcmp(req->remote_ip_addr, pUser_info->remote_ip_addr))
			{
				ulist_free_login_entry(bUser_info,pUser_info);
			}
	}
	return 1;
}
#else
int free_from_login_list(request * req)
{
	struct user_info * pUser_info;
	struct user_info * bUser_info = NULL;
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
	struct http_session *s;
	int i;
#endif

	for(pUser_info = user_login_list;pUser_info;bUser_info = pUser_info,pUser_info = pUser_info->next){
		if(!strcmp(pUser_info->remote_ip_addr, req->remote_ip_addr))
		{
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
			for (i = 0; i < HTTP_SESSION_MAX; i++)
			{
				s = &session_auth_array[i];
    			if (!s->in_use)
      				continue;

   				if (!strcmp(s->remote_ip_addr, pUser_info->remote_ip_addr)){
   					s->in_use = 0;
					break;
   				}
			}
#endif
			break;
		}
	}

	if(!pUser_info)
		return 0;

	ulist_free_login_entry(bUser_info,pUser_info);

	return 1;
}
#endif

void free_login_list(void)
{
	struct user_info * pUser_info;
	struct user_info * bUser_info = NULL;

	for(pUser_info = user_login_list;pUser_info;bUser_info = pUser_info,pUser_info = pUser_info->next)
		if(pUser_info)	ulist_free_login_entry(bUser_info, pUser_info);
}

#ifdef CONFIG_DEONET_SESSION
int get_sessionid_from_cookie(request *req, char *id)
{
	char *session_id;
	int ret = 0;

	if (req == NULL || req->cookie == NULL || id == NULL)
		return 0;

	session_id = strstr(req->cookie, "sessionid=");
	if (session_id && sscanf(session_id, "sessionid=%[^;]", id) > 0 && id[0])
		ret = 1;

	return ret;
}

#else
#ifdef ONE_USER_LIMITED
#ifdef ONE_USER_BY_SESSIONID
int get_sessionid_from_cookie(request *req, char *id)
{
        char *sess;

        if(req == NULL || req->cookie == NULL || id == NULL)
            return 0;

        //fprintf(stderr, "<%s:%d> req->cookie=%s\n", __func__, __LINE__, req->cookie);

        sess = strstr(req->cookie, "sessionid=");
        if(sess && sscanf(sess, "sessionid=%[^;]", id) > 0 && id[0])
        {

            //fprintf(stderr, "<%s:%d> got sessionid: %s\n", __func__, __LINE__, id);
            return 1;
        }
        else
        {
            return 0;
        }
}

int free_from_login_list_by_sessionid(char *sess)
{
	struct user_info * pUser_info;
	struct user_info * bUser_info = NULL;

	for(pUser_info = user_login_list;pUser_info;bUser_info = pUser_info,pUser_info = pUser_info->next){
		if(!strcmp(pUser_info->paccount->sessionid, sess))
			break;
	}

	if(!pUser_info)
		return 0;

	ulist_free_login_entry(bUser_info,pUser_info);

	return 1;
}
#else
int free_from_login_list_by_ip_addr(char * remote_ip_addr)
{
	struct user_info * pUser_info;
	struct user_info * bUser_info = NULL;

	for(pUser_info = user_login_list;pUser_info;bUser_info = pUser_info,pUser_info = pUser_info->next){
		if(!strcmp(pUser_info->remote_ip_addr, remote_ip_addr))
			break;
	}

	if(!pUser_info)
		return 0;

	ulist_free_login_entry(bUser_info,pUser_info);

	return 1;
}
#endif
#endif
#endif

#ifdef LOGIN_ERR_TIMES_LIMITED
//added by xl_yue
struct errlogin_entry * search_errlog_list(request * req)
{
	struct errlogin_entry * perrlog;

	for(perrlog = errlogin_list;perrlog;perrlog = perrlog->next){
		if(!strcmp(perrlog->remote_ip_addr, req->remote_ip_addr))
			break;
	}

	return perrlog;
}
void ulist_free_errlog_entry(struct errlogin_entry * berrlog,struct errlogin_entry * perrlog)
{
	//unlist and free
	if(perrlog == errlogin_list)
		errlogin_list = perrlog->next;
	else
		berrlog->next = perrlog->next;

	free(perrlog);
}

//added by xl_yue
int free_from_errlog_list(request * req)
{
	struct errlogin_entry * perrlog;
	struct errlogin_entry * berrlog = NULL;

	for(perrlog = errlogin_list;perrlog;berrlog = perrlog,perrlog = perrlog->next){
		if(!strcmp(perrlog->remote_ip_addr, req->remote_ip_addr))
			break;
	}

	if(!perrlog)
		return 0;

	ulist_free_errlog_entry(berrlog,perrlog);
	return 1;
}
#endif

#endif


#ifdef EMBED
static void log_pid()
{
	FILE *f;
	pid_t pid;
	char *pidfile = BOA_RUNFILE;

	pid = getpid();
	this_pid = pid;
	if((f = fopen(pidfile, "w")) == NULL)
		return;
	fprintf(f, "%d\n", pid);
	fclose(f);
}
#endif



//jim luo
//#if 0
#ifdef CONFIG_CTC_E8_CLIENT_LIMIT
// int maxPCClients=1;//MAXPCCLIENTS;
// int maxCameraClients=1;//MAXCAMERACLIENTS;
#define CLIENTSMONITOR  "/proc/ClientsMonitor"
 struct itimerval read_proc_interval;
//struct CTC_Clients accepted_PC_Clients[MAXPCCLIENTS];
//struct CTC_Clients accepted_Camera_Clients[MAXCAMERACLIENTS];
static void read_proc_handler(int dummy)
{
	FILE *fp;
	int totallen;
	int parsedlen=0;
	int i;
	char buffer[1024+1];
	char *cur_ptr=buffer;
	char *str_end=buffer;
	int str_found;
	char cmd_str[256];
	//printf("read proc \n");
	fp=fopen(CLIENTSMONITOR, "r");
	if(fp==NULL)
	{
		printf("file %s open failed\n", CLIENTSMONITOR);
		return;
	}
	memset(buffer, 0, sizeof(buffer));
	totallen=fread(buffer, 1, 1024, fp);
	buffer[1024]='\0';
	if(totallen!=0)
		printf("read size=%d, buffer=%s\n", totallen, buffer);
	if(totallen < 0)
		goto err;

	while(1)
	{
		str_found=0;
		memset(cmd_str, 0, sizeof(cmd_str));
		for(i=0; i<totallen-parsedlen; i++)
		{
			if(*str_end=='\n')
			{
				str_found=1;
				break;
			}
			str_end++;
		}
		if(str_found)
		{
			memcpy(cmd_str, cur_ptr, (unsigned)(str_end-cur_ptr));
			printf("cmd parsed out: %s", cmd_str);
			system(cmd_str);//execute it....
			parsedlen+=i;
			str_end+=1;
			cur_ptr=str_end;

		}else
		{
			break;
		}
	}
err:
	fclose(fp);
	return;
}
#endif


// Kaohj
// enable menu-driven CLI pthread
//#define MENU_CLI

#ifdef MENU_CLI
/******************************
 *	Start the CLI pthread
 */

void climenu (void);

void startCli()
{
	pthread_t ptCliId;
	//pthread_attr_t attCliAttributes;
	//struct sched_param priorityParams;

	//if (pthread_attr_init(&attCliAttributes) != 0)
	//	return;
	//if (pthread_create( &ptCliId, &attCliAttributes, &climenu, 0 )!=0)
	//	return;
	if (pthread_create( &ptCliId, 0, &climenu, 0 )!=0)
		return;
}
#endif

void calPasswdMD5(char *pass, char *passMD5)
{
	char *xpass;

	xpass = crypt((const char *)pass, (const char *)"$1$");
	if(xpass)
		strcpy(passMD5, xpass);
	else
		*passMD5 = '\0';
}

#ifdef WLAN_WEB_REDIRECT //jiunming,web_redirect
int redir_server_s;
void get_redir_request(int redir_server_socket);
int init_redirect_socket(void)
{
	int	ret = -1;
#ifdef IPV6
	struct sockaddr_in6 server_sockaddr;		/* socket address */
#else
	struct sockaddr_in server_sockaddr;		/* socket address */
#endif


#ifdef IPV6
	if ((redir_server_s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1)
#else
	if ((redir_server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
#endif
		die(NO_CREATE_SOCKET);

	/* server socket is nonblocking */
	if (fcntl(redir_server_s, F_SETFL, NOBLOCK) == -1)
		die(NO_FCNTL);

	if ((setsockopt(redir_server_s, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
					sizeof(sock_opt))) == -1)
		die(NO_SETSOCKOPT);

	/* internet socket */
	//maybe, binding to 127.0.0.1 is better.
#ifdef IPV6
	server_sockaddr.sin6_family = AF_INET6;
	memcpy(&server_sockaddr.sin6_addr,&in6addr_any,sizeof(in6addr_any));
	server_sockaddr.sin6_port = htons(8080);
#else
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sockaddr.sin_port = htons(8080);
#endif

	if (bind(redir_server_s, (struct sockaddr *) &server_sockaddr,
			 sizeof(server_sockaddr)) == -1)
		die(NO_BIND);

	//printf("\nCreate a web redirect socket at port 8080.");

	if (listen(redir_server_s, 5) == -1)
		die(NO_LISTEN);

	return redir_server_s;
}
#endif


#ifdef WEB_REDIRECT_BY_MAC
int mac_redir_server_s;
int mac_redir_server_port=WEB_REDIR_BY_MAC_PORT;
void get_mac_redir_request(void);
int init_mac_redirect_socket(void)
{
	int	ret = -1;
#ifdef IPV6
	struct sockaddr_in6 server_sockaddr;		/* socket address */
#else
	struct sockaddr_in server_sockaddr;		/* socket address */
#endif


#ifdef IPV6
	if ((mac_redir_server_s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1)
#else
	if ((mac_redir_server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
#endif
		die(NO_CREATE_SOCKET);

	/* server socket is nonblocking */
	if (fcntl(mac_redir_server_s, F_SETFL, NOBLOCK) == -1)
		die(NO_FCNTL);

	if ((setsockopt(mac_redir_server_s, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
					sizeof(sock_opt))) == -1)
		die(NO_SETSOCKOPT);

	/* internet socket */
	//maybe, binding to 127.0.0.1 is better.
#ifdef IPV6
	server_sockaddr.sin6_family = AF_INET6;
	memcpy(&server_sockaddr.sin6_addr,&in6addr_any,sizeof(in6addr_any));
	server_sockaddr.sin6_port = htons(mac_redir_server_port);
#else
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sockaddr.sin_port = htons(mac_redir_server_port);
#endif

	if (bind(mac_redir_server_s, (struct sockaddr *) &server_sockaddr,
			 sizeof(server_sockaddr)) == -1)
		die(NO_BIND);

	//printf("\nCreate a web redirect socket at port mac_redir_server_port.");

	if (listen(mac_redir_server_s, 5) == -1)
		die(NO_LISTEN);

	return mac_redir_server_s;
}
#endif

#ifdef USE_DEONET
int redir_server_s;
int init_access_redirect_socket(void)
{
	int ret = -1;
	struct sockaddr_in server_sockaddr;   /* socket address */

	if ((redir_server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		die(NO_CREATE_SOCKET);

	/* server socket is nonblocking */
	if (fcntl(redir_server_s, F_SETFL, NOBLOCK) == -1)
		die(NO_FCNTL);

	if ((setsockopt(redir_server_s, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
					sizeof(sock_opt))) == -1)
		die(NO_SETSOCKOPT);

	/* internet socket */
	//maybe, binding to 127.0.0.1 is better.
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sockaddr.sin_port = htons(34159);

	if (bind(redir_server_s, (struct sockaddr *) &server_sockaddr,
				sizeof(server_sockaddr)) == -1)
		die(NO_BIND);

	//printf("\nCreate a web redirect socket at port 8080.");

	if (listen(redir_server_s, 5) == -1)
		die(NO_LISTEN);

	return redir_server_s;
}
#endif

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
int captiveportal_server_s;
void get_captiveportal_request(void);
int init_captiveportal_socket(void)
{
	int	ret = -1;
#ifdef IPV6
	struct sockaddr_in6 server_sockaddr;		/* socket address */
#else
	struct sockaddr_in server_sockaddr;		/* socket address */
#endif

#ifdef IPV6
	if ((captiveportal_server_s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1)
#else
	if ((captiveportal_server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
#endif
		die(NO_CREATE_SOCKET);

	/* server socket is nonblocking */
	if (fcntl(captiveportal_server_s, F_SETFL, NOBLOCK) == -1)
		die(NO_FCNTL);

	if ((setsockopt(captiveportal_server_s, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt, sizeof(sock_opt))) == -1)
		die(NO_SETSOCKOPT);

	/* internet socket */
	//maybe, binding to 127.0.0.1 is better.
#ifdef IPV6
	memset(&server_sockaddr,0,sizeof(struct sockaddr_in6));
	server_sockaddr.sin6_family = AF_INET6;
	memcpy(&server_sockaddr.sin6_addr, &in6addr_any, sizeof(in6addr_any));
	server_sockaddr.sin6_port = htons(CAPTIVEPORTAL_PORT);
#else
	memset(&server_sockaddr,0,sizeof(struct sockaddr_in));
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sockaddr.sin_port = htons(CAPTIVEPORTAL_PORT);
#endif

	if (bind(captiveportal_server_s, (struct sockaddr *) &server_sockaddr, sizeof(server_sockaddr)) == -1)
		die(NO_BIND);

	//printf("\nCreate a web redirect socket at port %d\n", CAPTIVEPORTAL_PORT);

	if (listen(captiveportal_server_s, 5) == -1)
		die(NO_LISTEN);

	return captiveportal_server_s;
}
#endif

#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
int fwupgrade_redir_server_s=-1;
int fwupgrade_redir_server_port=WEB_REDIR_PORT_BY_FWUPGRADE;
void get_redir_request(int redir_server_socket);
int init_fwupgrade_redirect_socket(void)
{
	int	ret = -1;
#ifdef IPV6
	struct sockaddr_in6 server_sockaddr;		/* socket address */
#else
	struct sockaddr_in server_sockaddr;		/* socket address */
#endif

	if(fwupgrade_redir_server_s >= 0) close(fwupgrade_redir_server_s);
	fwupgrade_redir_server_s = -1;

#ifdef IPV6
	if ((fwupgrade_redir_server_s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1)
#else
	if ((fwupgrade_redir_server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
#endif
		die(NO_CREATE_SOCKET);

	/* server socket is nonblocking */
	if (fcntl(fwupgrade_redir_server_s, F_SETFL, NOBLOCK) == -1)
		die(NO_FCNTL);

	if ((setsockopt(fwupgrade_redir_server_s, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
					sizeof(sock_opt))) == -1)
		die(NO_SETSOCKOPT);

	/* internet socket */
	//maybe, binding to 127.0.0.1 is better.
#ifdef IPV6
	server_sockaddr.sin6_family = AF_INET6;
	memcpy(&server_sockaddr.sin6_addr,&in6addr_any,sizeof(in6addr_any));
	server_sockaddr.sin6_port = htons(fwupgrade_redir_server_port);
#else
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sockaddr.sin_port = htons(fwupgrade_redir_server_port);
#endif

	if (bind(fwupgrade_redir_server_s, (struct sockaddr *) &server_sockaddr,
			 sizeof(server_sockaddr)) == -1)
		die(NO_BIND);

	if (listen(fwupgrade_redir_server_s, 5) == -1)
		die(NO_LISTEN);

	return fwupgrade_redir_server_s;
}
#endif

#if 0
int mib_chain_backup(int id, unsigned char ** ptr)
{
	unsigned int chainSize;
	unsigned int size;
	int index, idx=0;
	unsigned int currentRecord=0;
	MIB_CHAIN_ENTRY_Tp pChain = NULL;

	chainSize = __mib_chain_total(id);
	index = __mib_chain_mib2tbl_id(id);
	if (chainSize > 0)
	{
		size += chainSize * mib_chain_record_table[index].per_record_size;
	}

	*ptr = (unsigned char *)malloc(chainSize);
	pChain = mib_chain_record_table[index].pChainEntry;
	while(pChain != NULL)
	{
		memcpy(*ptr+idx, pChain->pValue, mib_chain_record_table[index].per_record_size);
		pChain = pChain->pNext;
		idx += mib_chain_record_table[index].per_record_size;
		currentRecord++;
	}
	if (currentRecord != chainSize)
	{
		TRACE("(currentRecord!=chainSize) \n");
		return 0;
	}
	return 1;
}
#endif

static void init_global_parm()
{
	FILE	*fp=NULL;
	char buff[256], tmp1[20], tmp2[20];
#if 1//def CONFIG_RTL8686
	int size=0;
#endif
	// MAX_UPLOAD_FILESIZE
	// Check MTD block size
	g_max_upload_size = MAX_UPLOAD_FILESIZE;
	if (!(fp=fopen("/proc/mtd", "r")))
		printf("/proc/mtd not exists.\n");
	else {
		fgets(buff, sizeof(buff), fp);
		while (fgets(buff, sizeof(buff), fp) != NULL) {
			if (sscanf(buff, "%*s%s%*s%s", tmp1, tmp2) != 2) {
				printf("Unsuported MTD partition format\n");
				break;
			}
#if 1//def CONFIG_RTL8686
			if (strcmp(tmp2, "\"linux\"") == 0) {
				size += strtol(tmp1, 0, 16);
				printf("size of linux : %d\n",size);
				g_max_upload_size = size;
			}
			if (strcmp(tmp2, "\"rootfs\"") == 0) {
				size += strtol(tmp1, 0, 16);
				printf("size of linux : %d\n",size);
				g_max_upload_size = size;
				printf("%s%d::size of g_max_upload_size : %d\n",__func__,__LINE__,g_max_upload_size);
				break;
			}
#else
			if (strcmp(tmp2, "\"rootfs\"") == 0) {
				g_max_upload_size = strtol(tmp1, 0, 16);
				break;
			}
#endif
		}
		fclose(fp);
	}
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	g_max_upload_size = 64 * 1024 * 1024;	 //for osgi patition
#endif
}

#if defined(CONFIG_IPOE_ARPING_SUPPORT)
static void *start_arping(void *data)
{
	TIMEOUT_F(arpingLcpHandler, 0, ARPING_HANDLER_POLLING_INTERVAL, arping_lcp_ch);
#ifdef EMBED
	calltimeout();
#endif
}
#endif


void boa_do_signal_handler_for_mib(int *sig);
#if defined(CONFIG_E8B) && (defined(WIFI_TIMER_SCHEDULE) || defined(LED_TIMER) || defined(SLEEP_TIMER))
extern int _crond_check_current_state(int startHour, int startMin, int endHour, int endMin);
extern void updateScheduleCrondFile_time_change(int signum);
#endif
extern void sigsegv(int);
extern void sigbus(int);
extern void sigterm(int);
extern void sighup(int);
extern void sighup_run(void);
extern void sigint(int);
extern void sigchld(int);
extern void sigchld_run(void);
extern void sigusr1(int);
extern void sigalrm(int);
extern void send_igmp_query(int);

void boa_do_signal_handler_for_mib(int *sig)
{

    if (*sig & (1 << SIGSEGV))
	  {
				sigsegv(SIGSEGV);
				*sig &= ~(1 << SIGSEGV);
		}
		if (*sig & (1 << SIGBUS))
	  {
				sigbus(SIGBUS);
				*sig &= ~(1 << SIGBUS);
		}
    if (*sig & (1 << SIGTERM))
	  {
				sigterm(SIGTERM);
				*sig &= ~(1 << SIGTERM);
		}
    if (*sig & (1 << SIGHUP))
	  {
				//sighup(SIGHUP);
				sighup_run();
				*sig &= ~(1 << SIGHUP);
		}
    if (*sig & (1 << SIGINT))
	  {
				sigint(SIGINT);
				*sig &= ~(1 << SIGINT);
		}
    if (*sig & (1 << SIGCHLD))
	  {
				//sigchld(SIGCHLD);
				sigchld_run();
				*sig &= ~(1 << SIGCHLD);
		}
    if (*sig & (1 << SIGUSR1))
	  {
				sigusr1(SIGUSR1);
				*sig &= ~(1 << SIGUSR1);
		}
    if (*sig & (1 << SIGUSR2))
	  {
#if defined(CONFIG_E8B) && (defined(WIFI_TIMER_SCHEDULE) || defined(LED_TIMER) || defined(SLEEP_TIMER))
				updateScheduleCrondFile_time_change(SIGUSR2);
#endif
				if (deo_wan_nat_mode_get() == DEO_NAT_MODE) {
					if (igmp_restart_get_mode() == 1) {
						system("echo deonet_igmp_restart 0 > /proc/driver/realtek/deonet_igmp_restart &");
						unlink(IGMPPROXY_RUNFILE);
						system("/bin/igmpproxy -c 2 -d br0 -u nas0_0 -D &");
					}
				}
				else
					send_igmp_query(SIGUSR2);

				*sig &= ~(1 << SIGUSR2);
		}
}

void set_boa_signal_handler(int sig);
void set_boa_signal_handler(int sig)
{
	   signal_action |= (1 << sig);
}

int main(int argc, char **argv)
{
	int c;						/* command line arg */
#if defined(CONFIG_YUEME) && defined(CONFIG_USER_DBUS_PROXY)
	int s_port = 8080;
#else
	int s_port = 80;
#endif
	FILE *fp, *fp2;
	int i, i2, ret;
	char reset_button_flag=1;
#if defined(WLAN_SUPPORT) && defined(CONFIG_WIFI_SIMPLE_CONFIG)
	int wlan_found = 0;
#endif
//xl_yue
#ifdef ACCOUNT_LOGIN_CONTROL
	struct errlogin_entry * perrlog;
	struct errlogin_entry * berrlog = NULL;
#endif

//xl_yue add
#ifdef USE_LOGINWEB_OF_SERVER
	struct user_info * pUser_info;
	struct user_info * bUser_info = NULL;

#ifdef LOGIN_ERR_TIMES_LIMITED
	struct errlogin_entry * perrlog;
	struct errlogin_entry * berrlog = NULL;
#endif

#endif


	Modem_LinkSpeed vLs;
	vLs.upstreamRate=0;
	pid_t boa_pid;
	unsigned char value[16];
	unsigned char buf[256];

	struct web_socket_info *p_entry = NULL;
	init_web_socket_list();

	g_del_escapechar = 1;
#ifdef CONFIG_CU_BASEON_CMCC
	mib_get_s(PROVINCE_CU_LN_WEB_STYLE, &web_style_ln, sizeof(web_style_ln));
#endif

#ifdef CONFIG_CU
	mib_get_s(MIB_APPSCAN_TEST_START, &appscan_flag, sizeof(appscan_flag));
#ifdef CU_APPSCAN_RSP_DBG
	mib_get_s(MIB_APPSCAN_RSP_FLAG, &appscan_rsp_flag, sizeof(appscan_rsp_flag));
#endif
#endif

#ifdef ONE_USER_LIMITED
	memset(&suStatus, 0, sizeof(suStatus));
	memset(&usStatus, 0, sizeof(usStatus));
#endif

	// print debug message
	//DEBUGMODE(STA_INFO|STA_SCRIPT|STA_WARNING|STA_ERR);
#ifdef SUPPORT_ASP
	boaASPInit(argc,argv);
#endif
#ifdef EMBED
	log_pid();
#endif
#ifndef USE_DEONET
	openlog("boa", LOG_PID, 0);
#else
	openlog2(LOG_DM_HTTP);
#endif

	// Added by davian_kuo
	/* Init the language set with previous state. */
	char mStr[MAX_LANGSET_LEN] = {0};
	if (!mib_get_s (MIB_MULTI_LINGUAL, (void *)mStr, sizeof(mStr))) {
		fprintf (stderr, "mib get multi-lingual setting (boa) failed!\n");
		return -1;
	}

	// Prevent segmentation fault or index wrong lang after updating rootfs.
	unsigned char langIdx = 0;
	for (langIdx = 0; langIdx < LANG_MAX; langIdx++) {
		if (!strcmp(mStr, lang_set[langIdx].langType)) break;
	}
	if (langIdx < LANG_MAX) {
		g_language_state = g_language_state_prev = langIdx;
	} else {
		fprintf (stderr,
			"Original [%s] is missing!! Switch to new default [%s].\n",
			mStr, lang_set[0].langType);

			g_language_state = g_language_state_prev = 0;
	}

	//printf ("Language state => %d\n", g_language_state);
#if MULTI_LANG_DL == 1
	if (dl_handle != NULL) dlclose (dl_handle);

	char *lib_name = (char *) malloc (sizeof(char) * 25);
	if (lib_name == NULL) {
		fprintf (stderr, "lib_name malloc failed!\n"); return;
	}
	char *strtbl_name = (char *) malloc (sizeof(char) * 15);
	if (strtbl_name == NULL) {
		fprintf (stderr, "strtbl_name malloc failed!\n"); return;
	}
	snprintf (lib_name, 25, "libmultilang_%s.so", lang_set[g_language_state].langType);
	snprintf (strtbl_name, 15, "strtbl_%s", lang_set[g_language_state].langType);
	dl_handle = dlopen (lib_name, RTLD_LAZY);
	strtbl = (const char **) dlsym (dl_handle, strtbl_name);

	free (lib_name);
	free (strtbl_name);
#else
	strtbl = strtbl_name[g_language_state];
#endif


#ifdef SERVER_SSL
	while ((c = getopt(argc, argv, "p:vc:dns")) != -1)
#else
	while ((c = getopt(argc, argv, "p:vc:d")) != -1)
#endif /*SERVER_SSL*/
	{
		switch (c) {
		case 'c':
			server_root = strdup(optarg);
			break;
		case 'v':
			verbose_logs = 1;
			break;
		case 'd':
			do_fork = 0;
			break;
#ifdef EMBED
		case 'p':
			s_port= atoi(optarg);
			break;
#endif
#ifdef SERVER_SSL
		case 'n':
			do_sock = 0;		/*We don't want to do normal sockets*/
			break;
		case 's':
			do_ssl = 0;		/*We don't want to do ssl sockets*/
			break;

#endif /*SERVER_SSL*/
		default:
#if 0
			fprintf(stderr, "Usage: %s [-v] [-s] [-n] [-c serverroot] [-d]\n", argv[0]);
#endif
			exit(1);
		}
	}

	fixup_server_root();

#ifdef EMBED
	// Added by Mason Yu(2 level)
	fp = NULL;
	fp2 = NULL;
	fp = fopen("/var/boaSuper.passwd", "w+");
	fp2 = fopen("/var/boaUser.passwd", "w+");
	if(fp)
		fclose(fp);
	if(fp2)
		fclose(fp2);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod("/var/boaSuper.passwd", 0666);
	chmod("/var/boaUser.passwd", 0666);
#endif
	#if SUPPORT_AUTH_DIGEST
	fp = NULL;
	fp = fopen("/var/DigestUser.passwd", "w+");
	if(fp)
		fclose(fp);
	fp=NULL;
	fp = fopen("/var/DigestSuper.passwd", "w+");
	if(fp)
		fclose(fp);
	#endif
#endif


	read_config_files();
#ifdef CONFIG_BOA_APPLY_FAST
	init_fm_apply_thread();
#endif
#ifdef EMBED
	if(s_port != 80)
		set_server_port(s_port);
#endif
#ifdef BOA_TIME_LOG
	open_logs();
#endif
	create_common_env();

#if 0 // unused ssl socket for deonet
#ifdef SERVER_SSL
	if (do_ssl) {
		if (InitSSLStuff() != 1) {
			/*TO DO - emit warning the SSL stuff will not work*/
			syslog(LOG_ALERT, "Failure initialising SSL support - ");
			if (do_sock == 0) {
				syslog(LOG_ALERT, "    normal sockets disabled, so exiting");fflush(NULL);
				return 0;
			} else {
				syslog(LOG_ALERT, "    supporting normal (unencrypted) sockets only");fflush(NULL);
				do_sock = 2;
			}
	  }
	} else
		do_sock = 2;
#endif /*SERVER_SSL*/
#endif

#if 0 // unused ssl socket for deonet
#ifdef SERVER_SSL
	if(do_sock){
#endif /*SERVER_SSL*/
#endif

		bind_web_socket(server_port);
		bind_web_socket(server_sub_port);

#if 0 // unused ssl socket for deonet
#ifdef SERVER_SSL
	}
#endif /*SERVER_SSL*/
#endif

#ifdef USE_DEONET /* sghong, 20230914 */
	init_access_redirect_socket();
#endif

#ifdef WLAN_WEB_REDIRECT //jiunming,web_redirect
	if( init_redirect_socket() > 0 )
		if (redir_server_s > max_fd)
			max_fd = redir_server_s;
#endif

#ifdef WEB_REDIRECT_BY_MAC
	if( init_mac_redirect_socket() > 0 )
		if (mac_redir_server_s > max_fd)
			max_fd = mac_redir_server_s;
#if defined(SERVER_SSL)
	if( init_mac_redirect_socket_ssl() > 0 )
		if (mac_redir_server_ssl > max_fd)
			max_fd = mac_redir_server_ssl;
#endif
#endif
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	if( init_captiveportal_socket() > 0 )
		if (captiveportal_server_s > max_fd)
			max_fd = captiveportal_server_s;
#endif

	init_global_parm();
	init_signals();
#ifdef CONSOLE_BOA
	signal(SIGINT, SIG_IGN);
#endif
	/* background ourself */

#ifndef EMBED
	if (do_fork)
		if (fork())
			exit(0);
#endif

	/* close server socket on exec
	 * so cgi's can't write to it */

	p_entry = p_web_socket;
	while (p_entry)
	{
		if (p_entry->port == server_port && p_entry->socket > 0)
		{
			if (fcntl(p_entry->socket, F_SETFD, 1) == -1)
			{
#if 0
				perror("can't set close-on-exec on server socket!");
#endif
				exit(0);
			}
		}
		else if (p_entry->port == server_sub_port && p_entry->socket > 0) // for 8080 port
		{
			if (fcntl(p_entry->socket, F_SETFD, 1) == -1)
			{
#if 0
				perror("can't set close-on-exec on server socket!");
#endif
				exit(0);
			}
		}

		p_entry = p_entry->next;
	}

#if 0
	/* close STDIN on exec so cgi's can't read from it */
	if (fcntl(STDIN_FILENO, F_SETFD, 1) == -1) {
#if 0
		perror("can't set close-on-exec on STDIN!");
#endif
		exit(0);
	}
#endif

	/* translate paths to server_chroot */
	chroot_aliases();
	chroot_virtual_hosts();

#ifdef USE_CHROOT
	if (server_chroot)
	{
		if (!strncmp(server_root,server_chroot,strlen(server_chroot)))
				strcpy(server_root,server_root + strlen(server_chroot) -
						(server_chroot[strlen(server_chroot)-1]=='/'?1:0) );
		else
		{
#ifdef BOA_TIME_LOG
			log_error_time();
			fprintf(stderr,"Warning: server_root not accessible from %s\n",
					server_chroot);
#endif
			syslog(LOG_ERR, "server root not accessible");
		}
	}
  if (server_chroot)
  {
    if (!strncmp(dirmaker,server_chroot,strlen(server_chroot)))
        strcpy(dirmaker,dirmaker + strlen(server_chroot) -
            (server_chroot[strlen(server_chroot)-1]=='/'?1:0) );
    else
    {
#ifdef BOA_TIME_LOG
      log_error_time();
      fprintf(stderr,"Warning: directory maker not accessible from %s\n",
          server_chroot);
#endif
	  syslog(LOG_ERR, "directory maker not accessible");
    }
  }
#endif

#ifdef USE_AUTH
	auth_check();	/* Check Auth'ed directories */
#endif

	DBG(printf("main:give out privs\n");)
	/* give away our privs if we can */
#ifdef EMBED
	server_gid = getgid();
	server_uid = getuid();
#else
	if (getuid() == 0) {
		struct passwd *passwdbuf;
		passwdbuf = getpwuid(server_uid);
		if (passwdbuf == NULL)
			die(GETPWUID);
		if (initgroups(passwdbuf->pw_name, passwdbuf->pw_gid) == -1)
			die(INITGROUPS);
                if (server_chroot)
                  if (chroot(server_chroot))
                    die(CANNOT_CHROOT);
		if (setgid(server_gid) == -1)
			die(NO_SETGID);
		if (setuid(server_uid) == -1)
			die(NO_SETUID);
	} else {
		if (server_gid || server_uid) {
#ifdef BOA_TIME_LOG
			log_error_time();
			fprintf(stderr, "Warning: "
					"Not running as root: no attempt to change to uid %d gid %d\n",
					server_uid, server_gid);
#endif
		}
		server_gid = getgid();
		server_uid = getuid();
	}
#endif /* EMBED */

	init_free_requests(30);
	/* main loop */

#ifdef EMBED

	// Added by Mason Yu(2 level)
	mib_get_s(MIB_SUSER_NAME, (void *)suName, sizeof(suName));
	mib_get_s(MIB_USER_NAME, (void *)usName, sizeof(usName));
	rtk_util_update_boa_user_account();

#ifdef MENU_CLI
	// start climenu thread
	startCli();
#endif
#endif
	timestamp();
	FD_ZERO(&block_read_fdset);
	FD_ZERO(&block_write_fdset);

	status.connections = 0;
	status.requests = 0;
	status.errors = 0;
//#if 0
//jim


#ifdef EMBED
// Kaohj --- hook auto-hunt register here
#ifdef CONFIG_ATM_REALTEK
#ifdef AUTO_PVC_SEARCH_AUTOHUNT
	void register_pvc(int signum);
	// Auto-pvc search ---- auto-hunt
	boa_pid = (int)getpid();
	//printf("Inform boa PID %d to kernel by sarctl\n", boa_pid);
	sprintf(value,"%d",boa_pid);

        /* inform kernel the boa process ID for the further kernel signal trigger */
	if (va_cmd("/bin/sarctl",2,1,"boa_pid",value))
		printf("sarctl boa_pid %s failed\n", value);

	signal(SIGUSR2, register_pvc);
#endif
#endif
#endif // of EMBED

#ifdef QOS_SUPPORT_RTP
	{
		void qos_handler(int signum);
		signal(SIGUSR2, qos_handler);
	}
#endif

#ifdef PARENTAL_CTRL
	parent_ctrl_table_init();
#endif
#ifdef _PRMT_X_CMCC_SECURITY_
	ParentalCtrl_rule_init();
#endif

#ifdef REMOTE_HTTP_ACCESS_AUTO_TIMEOUT
	rtk_fw_update_rmtacc_auto_logout_time();
#endif
#ifdef _PRMT_X_CMCC_AUTORESETCONFIG_
	auto_reset_rule_init();
#endif

#ifdef EMBED
#ifdef WEB_REDIRECT_BY_MAC
	TIMEOUT_F(clearLandingPageRule, 0, 10, landingPage_ch);
#endif

#ifdef AUTO_DETECT_DMZ
	TIMEOUT_F(poll_autoDMZ, 0, 10, autoDMZ_ch);
#endif

#ifdef CONFIG_NET_IPGRE
	TIMEOUT_F(poll_GRE, 0, GRE_BACKUP_INTERVAL, gre_ch);
#endif
#ifdef CONFIG_USER_CUMANAGEDEAMON
  TIMEOUT_F(cuManageSechedule, 0, 1, cuManage_ch);
#endif

#if defined(PON_HISTORY_TRAFFIC_MONITOR)
	TIMEOUT_F(pon_traffic_monitor, 0, PON_HISTORY_TRAFFIC_MONITOR_INTERVAL, pon_traffic_monitor_ch);
#endif

#endif // of EMBED
#if defined(CONFIG_CMCC) && defined(CONFIG_SUPPORT_ADAPT_DPI_V3_3_0)
	TIMEOUT_F(calWanSymbolErrors, 0, 60, wanSE_ch);
#endif

#ifdef CTC_TELNET_SCHEDULED_CLOSE
        TIMEOUT_F((void*)schTelnetCheck, 0, 10, sch_telnet_ch);
#endif

#ifdef CONFIG_CU_BASEON_YUEME
	TIMEOUT_F(schFramework_chCheck, 0, 5, framework_ch);
#endif
#ifdef CONFIG_WAN_INTERVAL_TRAFFIC
	pthread_t start_wanIntervalTraffic_tid;
	pthread_create(&start_wanIntervalTraffic_tid, NULL, start_wanIntervalTraffic, NULL);
	pthread_detach(start_wanIntervalTraffic_tid);
#endif

#if defined(CONFIG_IPOE_ARPING_SUPPORT)
	pthread_t tid;
	pthread_create(&tid, NULL, start_arping, NULL);
	pthread_detach(tid);
#endif
  init_ap_connection_thread();

#ifdef DOS_SUPPORT
	{
		char cmdStr[50];

		snprintf(cmdStr, sizeof(cmdStr), "echo %d > /proc/dos_syslog", this_pid);
		system(cmdStr);
		signal(SIGUSR1, DoS_syslog);
	}
#endif
#if defined(CONFIG_CT_AWIFI_JITUAN_SMARTWIFI)
	  unsigned char functype=0;
	  mib_get(AWIFI_PROVINCE_CODE, &functype);
	  if(functype == AWIFI_ZJ){
	    TIMEOUT_F(wifiAuthCheck, 0, 5, wifiAuth_ch);
	  }
#endif

#if defined(CONFIG_CMCC_WIFI_PORTAL)
	TIMEOUT_F(wifiAuthCheck, 0, 5, wifiAuth_ch);
#endif

#ifdef RESTRICT_PROCESS_PERMISSION
	/* restrict system resource access permissions */
	set_otherUser_Capability(EUID_OTHER);
#endif
#ifdef SUPPORT_NON_SESSION_IPOE
	TIMEOUT_F(neighDetectCheck, 0, 1, neighdetect_ch);
#endif
#if defined(WLAN_SUPPORT) && defined(CONFIG_WIFI_SIMPLE_CONFIG)
#if defined(TRIBAND_SUPPORT)
	wlan_found = getInFlags((char *)WLANIF[0], 0);
	wlan_found &= getInFlags((char *)WLANIF[1], 0);
	wlan_found &= getInFlags((char *)WLANIF[2], 0);
#elif defined(WLAN_DUALBAND_CONCURRENT)
	wlan_found = getInFlags((char *)WLANIF[0], 0);
	wlan_found &= getInFlags((char *)WLANIF[1], 0);
#else
	wlan_found = getInFlags((char *)WLANIF[0], 0);
#endif
#endif

#ifdef USE_DEONET /* sghong, 20230525 */
	{
		char cmdStr[50];
		snprintf(cmdStr, sizeof(cmdStr), "iptables -t nat -N access_limit");
		system(cmdStr);

		snprintf(cmdStr, sizeof(cmdStr), "iptables -t nat -I PREROUTING -j access_limit");
		system(cmdStr);
	}
#endif

	while (1) {
		static int dsl_link = 0;
		unsigned char vChar;
		static unsigned long link_status_cnt = 0;
		struct timespec currentTime;

		clock_gettime(CLOCK_MONOTONIC, &currentTime);

		DBG(printf("\n%s:%d: enter while loop!\n",__func__,__LINE__);)

#ifdef EMBED
		calltimeout();
		// Kaohj --- Internet LED
		//time_t newtime,oldtime=0;
#ifdef CONFIG_LEDPWR_PBC
                if((fp = fopen("/proc/led_power","r")) != NULL)
				{
					static unsigned int last_led_power_flag = 1;
					unsigned int led_power_flag = 1;

					buf[0] = '\0';
					fread(buf, sizeof(buf), 1, fp);
					fclose(fp);

					sscanf(buf,"%u",&led_power_flag);
					if(led_power_flag != last_led_power_flag)
					{
							DBG(printf("%s:%d: handle /proc/led_power!\n",__func__,__LINE__);)
							last_led_power_flag = led_power_flag;
							if(led_power_flag == 0)// all led off
							{
									system("/bin/mpctl led off");
							}
							else if(led_power_flag == 1)//all led restore original state
							{
									system("/bin/mpctl led restore");
							}
					}
                }
#endif//CONFIG_LEDPWR_PBC

#ifdef HANDLE_DEVICE_EVENT
//move to systemd by rtk_wlan_wifi_button_on_off_action()
#else
//star: for wlan on/off button
#if defined( WLAN_SUPPORT) && defined(CONFIG_WLAN_ON_OFF_BUTTON)
		if((fp = fopen("/proc/wlan_onoff","r")) != NULL)
		{
			int button_flag=0;
			unsigned char wlan_flag=0;
			int orig_wlanid = wlan_idx;
			MIB_CE_MBSSIB_T mEntry;

			buf[0] = '\0';
			fread(buf, sizeof(buf), 1, fp);
			fclose(fp);

			sscanf(buf,"%d",&button_flag);

			if(button_flag==1)
			{
				DBG(printf("%s:%d: handle /proc/wlan_onoff!\n",__func__,__LINE__);)
            #if defined(TRIBAND_SUPPORT)
                {
                    int k;
                    for (k=0; k<NUM_WLAN_INTERFACE; k++) {
                        wlan_idx = k;
                        wlan_getEntry(&mEntry, 0);
                        if (k==0) {
                            vChar = mEntry.wlanDisabled;
                            if(vChar == 0)
                                vChar = 1;
                            else
                                vChar = 0;
                        }
                        mEntry.wlanDisabled = vChar;
                        wlan_setEntry(&mEntry, 0);
                    }
                }
            #else /* !defined(TRIBAND_SUPPORT) */
				wlan_idx=0;
				wlan_getEntry(&mEntry, 0);
				vChar = mEntry.wlanDisabled;
				if(vChar == 0)
					vChar = 1;
				else
					vChar = 0;
				mEntry.wlanDisabled = vChar;
				wlan_setEntry(&mEntry, 0);
				#ifdef WLAN_DUALBAND_CONCURRENT
				wlan_idx=1;
				wlan_getEntry(&mEntry, 0);
				mEntry.wlanDisabled = vChar;
				wlan_setEntry(&mEntry, 0);
				#endif
            #endif /* defined(TRIBAND_SUPPORT) */
				wlan_idx = orig_wlanid;
#ifdef CONFIG_WIFI_SIMPLE_CONFIG // WPS
				update_wps_configured(0);
#endif
				if(mib_update(CURRENT_SETTING, CONFIG_MIB_ALL) == 0)
					printf("CS Flash error! \n");
				config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);

				wlan_flag = '0';
			}
			else if(button_flag==2){
				DBG(printf("%s:%d: handle /proc/wlan_onoff!\n",__func__,__LINE__);)
				config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);

				wlan_flag = '0';
			}

			if(wlan_flag != 0 && (fp = fopen("/proc/wlan_onoff","w")) != NULL)
			{
				fprintf(fp, "%c", wlan_flag);
				fclose(fp);
			}
		}
#endif
#endif
//cathy, wsc write "R" to gpio for restarting wlan, if "2" is read from gpio->restart wlan
#if defined(WLAN_SUPPORT) && defined(CONFIG_WIFI_SIMPLE_CONFIG)
		if(wlan_found)
		{
#if defined(TRIBAND_SUPPORT)
            if (getInFlags((char *)WLANIF[0], 0) || getInFlags((char *)WLANIF[1], 0) || getInFlags((char *)WLANIF[2], 0))
#elif defined(WLAN_DUALBAND_CONCURRENT)
			if (getInFlags((char *)WLANIF[0], 0) || getInFlags((char *)WLANIF[1], 0))
#else
			if (getInFlags((char *)WLANIF[0], 0))
#endif
			{
				int wps_wlan_restart=0;
				//unsigned char wps_flag;

				if((fp = fopen("/proc/gpio","r")) != NULL)
				{
					buf[0] = '\0';
					fread(buf, sizeof(buf), 1, fp);
					fclose(fp);

					sscanf(buf,"%d",&wps_wlan_restart);
					if(wps_wlan_restart==2) {
#if 1// defined(CONFIG_RTK_DEV_AP)
						update_wps_from_file();
						//update_wps_from_mibtable();
#else
						sync_wps_config_mib();
#endif
						printf("wps wlan restart\n");
						config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
					}
				}
			}
		}
#endif
#ifdef WLAN_QTN
	{
		rt_report_qtn_button_state();
		rt_check_qtn_wps_config();
	}
#endif
#endif // of EMBED


#ifdef EMBED
#ifndef HANDLE_DEVICE_EVENT
		// Added by Mason Yu for check Reset button
		// (1) Check Reset to default
		if (reset_button_flag == 1)
		{
			if((fp = fopen("/proc/load_default","r")) != NULL)
			{
				int flag = 0;

				buf[0] = '\0';
				fread(buf, sizeof(buf), 1, fp);
				fclose(fp);

				sscanf(buf,"%d",&flag);
				if(flag==1)
				{
				//Mason Yu,  LED flash while factory reset
					system("echo 2 > /proc/load_default");
					printf("Going to Reload Default\n");
					reset_cs_to_default(1);
#ifdef CONFIG_CT_AWIFI_JITUAN_SMARTWIFI
        unsigned char functype=0;
        mib_get(AWIFI_PROVINCE_CODE, &functype);
        if(functype == AWIFI_ZJ){
          system("cp -f /var/config/awifi/awifi_bak.conf /var/config/awifi/awifi.conf");
          if(access("/var/config/awifi/binversion", F_OK) == 0)
            system("rm -f /var/config/awifi/binversion");
        }
#endif
					cmd_reboot();
					reset_button_flag = 0;
				}
			}
		}
#endif
		// (2) Check Reset to current
		if (reset_button_flag == 1)
		{
			if((fp = fopen("/proc/load_reboot","r")) != NULL)
			{
				int flag = 0;

				buf[0] = '\0';
				fread(buf, sizeof(buf), 1, fp);
				fclose(fp);

				sscanf(buf,"%d",&flag);
				if(flag==1)
				{
					printf("Going to Reboot\n");
					cmd_reboot();
					reset_button_flag = 0;
				}
			}
		}

#ifdef CONFIG_RTL8676_CHECK_WIFISTATUS
			if((fp = fopen("/proc/wifi_checkstatus","r")) != NULL)
			{
				int wifi_status_num=0;

				buf[0] = '\0';
				fread(buf, sizeof(buf), 1, fp);
				fclose(fp);

				sscanf(buf,"%d",&wifi_status_num);
				if(wifi_status_num==1)
				{
					fprintf(stderr, "Wi-Fi Restart\n");
					system("echo 1 > /proc/wifi_checkstatus");
					cmd_wlan_delay_restart();
				}
			}
#endif

#endif // of EMBED
#ifdef EMBED
#ifdef CONFIG_DEV_xDSL
       		if ( adsl_drv_get(RLCM_GET_LINK_SPEED, (void *)&vLs,
			RLCM_GET_LINK_SPEED_SIZE) && vLs.upstreamRate != 0)
#else
		if (1)
#endif
		{
			if (dsl_link == 0) { // down --> up
				unsigned int igmpItf;
				int flags;
				syslog(LOG_INFO, "DSL Link up");

#ifdef CONFIG_ATM_CLIP
				sendAtmInARPRep(1);
#endif
			}
			dsl_link = 1;
		}
		else {
			if (dsl_link == 1) { // up --> down
				syslog(LOG_INFO, "DSL Link down");
			}
			dsl_link = 0;
		}
#endif // of EMBED

#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
	if(check_fwupgrade_phase()) {
		if(fwupgrade_redir_server_s < 0) {
			init_fwupgrade_redirect_socket();
		}
	} else if(fwupgrade_redir_server_s >= 0) {
		close(fwupgrade_redir_server_s);
		fwupgrade_redir_server_s = -1;
	}
#endif

//xl_yue add,if no action lasting for 5 minutes,auto logout
#ifdef USE_LOGINWEB_OF_SERVER
		for(pUser_info = user_login_list;pUser_info;bUser_info = pUser_info,pUser_info = pUser_info->next){
#ifdef AUTO_LOGOUT_SUPPORT
			//if no action lasting for 5 minutes,auto logout
			if ((time_counter - pUser_info->last_time > LOGIN_AUTOEXIT_TIME)
#ifdef REMOTE_HTTP_ACCESS_AUTO_TIMEOUT
				|| (pUser_info->remote_access && (time_counter - pUser_info->first_time > remote_user_auto_logout_time))
#endif
				)
#else
			//default:1 day
			if ((time_counter - pUser_info->last_time > LOGIN_MAX_TIME)
#ifdef REMOTE_HTTP_ACCESS_AUTO_TIMEOUT
				|| (pUser_info->remote_access && (time_counter - pUser_info->first_time > remote_user_auto_logout_time))
#endif
				)
#endif
			{
				ulist_free_login_entry(bUser_info,pUser_info);
			}
		}

#if !defined(_PRMT_X_CT_COM_ALARM_MONITOR_)
#ifdef LOGIN_ERR_TIMES_LIMITED
		for(perrlog = errlogin_list;perrlog;berrlog = perrlog,perrlog = perrlog->next){
			if(time_counter - perrlog->last_time > LOGIN_ERR_WAIT_TIME){//after 1 mintue, unlist and free
				ulist_free_errlog_entry(berrlog,perrlog);
			}
		}
#endif
#endif
#endif

#if defined(WEB_HTTP_SERVER_BIND_IP)
		if ((fp = fopen(WEB_HTTP_SERVER_REBIND_FILE, "r")) != NULL)
		{
			char rebind_flag[4] = {0};
			DBG(printf("%s:%d: handle server rebind!\n",__func__,__LINE__);)
			if (fgets(rebind_flag, sizeof(rebind_flag), fp) != 0)
			{
				if (strstr(rebind_flag, "1"))
				{
					rebind_web_socket();
				}
			}
			fclose(fp);
			unlink(WEB_HTTP_SERVER_REBIND_FILE);
		}
#endif

		DBG(printf("%s:%d: time_counter=%ld!\n",__func__,__LINE__,time_counter);)

#ifdef PARENTAL_CTRL
		if ((time_counter & 0x07) == 0 )
		{
			time_t tm;
			struct tm the_tm;

	 		time(&tm);
			memcpy(&the_tm, localtime(&tm), sizeof(the_tm));
			parent_ctrl_table_rule_update();
			//printf("!!!UPdate!! \r\n");
			//parent_ctrl_table_rule_update();

			//printf("day of the week:%d\r\n",(int)the_tm.tm_wday);
			//printf("hour of the day: %d\r\n", the_tm.tm_hour *60 + the_tm.tm_min);
			//printf("month:%d\r\n",the_tm.tm_mon);
			//printf("time_counter:%d\r\n",time_counter);

		}
#endif

#ifdef _PRMT_X_CMCC_SECURITY_
		if ((time_counter & 0x07) == 0)
		{
			unsigned char rule_reset = 0;
			mib_get(MIB_RS_PARENTALCTRL_RULE_RESET, (void *)&rule_reset);
			if (rule_reset)
			{
				rule_reset = 0;
				mib_set(MIB_RS_PARENTALCTRL_RULE_RESET, (void *)&rule_reset);
				ParentalCtrl_rule_init();
			}
			ParentalCtrl_rule_set();
		}
#endif
#ifdef _PRMT_X_CMCC_AUTORESETCONFIG_
		if ((time_counter & 0x1f) == 0)//32s
		{
			unsigned char rule_reset = 0;
			mib_get(MIB_RS_CMCC_AUTO_RESET_RULE_RESET, (void *)&rule_reset);
			if (rule_reset)
			{
				rule_reset = 0;
				mib_set(MIB_RS_CMCC_AUTO_RESET_RULE_RESET, (void *)&rule_reset);
				auto_reset_rule_init();
			}
			auto_reset_rule_check();
		}
#endif
#ifdef CONFIG_VIR_BOA_JEMBENCHTEST
		do{
			unsigned int config_vir_jspeed = 0;
			mib_get_s(MIB_OSGI_VIR_BOA_JSPEED,(void*)&config_vir_jspeed, sizeof(config_vir_jspeed));
			if(config_vir_jspeed == 1){
				static unsigned int vir_boa_jspeed_counter=0;
				unsigned int now= getSYSInfoTimer();
				if ((now-vir_boa_jspeed_counter)>=2 )
				{
					//printf("%s %d####vir_boa_jspeed_counter=%d now=%d\n",__FUNCTION__,__LINE__,vir_boa_jspeed_counter,now);
					vir_boa_jspeed_counter=now;
					rtk_cmcc_do_jspeedup();
					//rtk_cmcc_do_jembench_test_iptables();
				}
			}else{
					rtk_cmcc_clear_jspeedup();
					//cleanJembenchTestIptables();
			}
		}while(0);
#endif

#ifdef CU_APP_SCHEDULE_LOG
		{
			time_t t_cur = time(NULL);
			time_t t_schd[2];
			FILE * fp_sched = NULL;

			fp_sched = fopen("/var/config/syslog_schedule_upload","r");
			if(fp_sched)
			{
				fread(&t_schd,sizeof(t_schd),1,fp_sched);
				fclose(fp_sched);

				if(t_cur >= t_schd[1])
				{
					unsigned char upload_log_flag = 0;
					if(mib_get(SCHEDULE_LOG_FLAG,(void*)&upload_log_flag) && upload_log_flag == 0){
						printf("Create log - Start from : %s\n",ctime(&t_schd[0]));
						printf("Create log - End at : %s\n",ctime(&t_schd[1]));
						GenerateScheduledLog(t_schd[0],t_schd[1]);
						UploadScheduledLog();
						upload_log_flag = 1;
						mib_set(SCHEDULE_LOG_FLAG,(void*)&upload_log_flag);
						Commit();
					}
				}
			}
		}
#endif

#if defined(SUPPORT_URL_FILTER) && defined(SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC)
#if defined(CONFIG_RTK_CTCAPD_FILTER_SUPPORT)
		if (!(time_counter%60))
#else
		if ((time_counter & 0x10) == 0 )
#endif
		{
			static time_t urlSchLastTime = 0;

			if((ntp_synced == 0) && isTimeSynchronized())
				ntp_synced = 1;
			if(ntp_synced == 1){
				if(urlSchLastTime == 0)
					urlSchLastTime = currentTime.tv_sec;
				else if((currentTime.tv_sec - urlSchLastTime) >= 60){
					check_url_filter_time_schedule();
#if defined(CONFIG_RTK_CTCAPD_FILTER_SUPPORT)
					rtk_check_dns_filter_time_schedule();
#endif
					urlSchLastTime = currentTime.tv_sec;
				}
			}
		}
#endif

#ifdef ACCOUNT_LOGIN_CONTROL
		//added by xl_yue,check the errlogin_list and free the entrys that have pass 1 minutes
		for(perrlog = errlogin_list;perrlog;berrlog = perrlog,perrlog = perrlog->next){

			if(time_counter - perrlog->last_time > LOGIN_WAIT_TIME){//after 1 mintue, unlist and free
				if(perrlog == errlogin_list)
					errlogin_list = perrlog->next;
				else
					berrlog->next = perrlog->next;

				free(perrlog);
			}
		}

		//Added by xl_yue;
		if((time_counter - su_account.last_time > ACCOUNT_TIMEOUT) && su_account.account_busy){		//account timeout,then logout
			su_account.account_timeout = 1;
			su_account.account_busy = 0;
//			printf("su_account timeout\n");
//		}else{
//			su_account.account_timeout = 0;
//			printf("su_account return not timeout\n");
		}
		//added by xl_yue
		if((time_counter - us_account.last_time > ACCOUNT_TIMEOUT) && us_account.account_busy){
			us_account.account_timeout = 1;
			us_account.account_busy = 0;
//			printf("us_account timeout\n");
//		}else{
//			us_account.account_timeout = 0;
//			printf("us_account return not timeout\n");
		}
#endif
		/* Let signal to do at end of while, it can void boa get mib info with signal interrupt.
		 * We want first to remember signal ID when recv SIGNAL, then do signal event to void signal interrupt
		 * boa get mib process. 2019/11/26. */
		boa_do_signal_handler_for_mib((int *)&signal_action);

		switch(lame_duck_mode) {
			case 1:
				p_entry = p_web_socket;
				while (p_entry)
				{
					if (p_entry->port == server_port && p_entry->socket > 0)
					{
						lame_duck_mode_run(p_entry->socket);
					}
					else if (p_entry->port == server_sub_port && p_entry->socket > 0) // for 8080 port
					{
						lame_duck_mode_run(p_entry->socket);
					}
					p_entry = p_entry->next;
				}

				/* fall-through */
                        case 2:
				if (!request_ready && !request_block)
					die(SHUTDOWN);
				break;
			default:
				break;
		}

#ifdef RTK_WEB_HTTP_SERVER_BIND_DEV
		if(isFileExist(RTK_TRIGGLE_BOA_REBIND_SOCK_FILE)){
			syslog(LOG_DEBUG, "[%s:%d] boa rebind_web_socket.",__FILE__,__LINE__);
			rebind_web_socket();
			unlink(RTK_TRIGGLE_BOA_REBIND_SOCK_FILE);
		}
#endif

		DBG(printf("%s:%d: start handle web request!\n",__func__,__LINE__);)

		/* move selected req's from request_block to request_ready */
		fdset_update();
		if (request_ready)
		{
			DBG(printf("%s:%d: have request_ready\n",__func__,__LINE__);)
			process_requests();		/* any blocked req's move from request_ready to request_block */
			FD_ZERO(&block_read_fdset);
			FD_ZERO(&block_write_fdset);
		}
		else
		{
			request *current;
			DBG(printf("%s:%d: no request_ready\n",__func__,__LINE__);)
			max_fd = 0;

			p_entry = p_web_socket;
			while (p_entry)
			{
				if (p_entry->socket > 0)
				{
					if (p_entry->port == server_port)
					{
						max_fd = MAX(p_entry->socket, max_fd);
					}
					else if (p_entry->port == server_sub_port) // for 8080 port
					{
						max_fd = MAX(p_entry->socket, max_fd);
					}

#ifdef SERVER_SSL
					if (p_entry->port == ssl_server_port)
					{
						if (do_sock < 2)
						{
							max_fd = MAX(p_entry->socket, max_fd);
						}
					}
#endif
				}
				p_entry = p_entry->next;
			}

#ifdef WLAN_WEB_REDIRECT //jiunming,web_redirect
			max_fd = MAX(redir_server_s, max_fd);
			FD_SET(redir_server_s, &block_read_fdset);
#endif
#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
			if(fwupgrade_redir_server_s >= 0) {
				max_fd = MAX(fwupgrade_redir_server_s, max_fd);
				FD_SET(fwupgrade_redir_server_s, &block_read_fdset);
			}
#endif
#ifdef WEB_REDIRECT_BY_MAC
			max_fd = MAX(mac_redir_server_s, max_fd);
			FD_SET(mac_redir_server_s, &block_read_fdset);
#if defined(SERVER_SSL)
			max_fd = MAX(mac_redir_server_ssl, max_fd);
			FD_SET(mac_redir_server_ssl, &block_read_fdset);
#endif
#endif
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
			max_fd = MAX(captiveportal_server_s, max_fd);
			FD_SET(captiveportal_server_s, &block_read_fdset);
#endif

#ifdef USE_DEONET
			max_fd = MAX(redir_server_s, max_fd);
			FD_SET(redir_server_s, &block_read_fdset);
#endif
			for (current = request_block; current; current = current->next) {
				max_fd = MAX(current->fd, max_fd);
				max_fd = MAX(current->data_fd, max_fd);
				max_fd = MAX(current->post_data_fd, max_fd);
			}

			req_timeout.tv_sec = 1;
			req_timeout.tv_usec = 0;
			ret = select(max_fd + 1, &block_read_fdset, &block_write_fdset, NULL, &req_timeout);
			DBG(printf("%s:%d: socket select ret=%d!\n",__func__,__LINE__,ret);)
			if (ret < 0) {
				if (errno == EINTR || errno == EBADF)
					continue;	/* while(1) */
				else if(errno != EBADF)
					die(SELECT);
			}
			if(ret == 0) continue;
			//printf("==> ret = %d\n", ret);

#ifdef WEB_REDIRECT_BY_MAC
			if (FD_ISSET(mac_redir_server_s, &block_read_fdset))
				get_mac_redir_request();
#endif
#ifdef SUPPORT_PUSHWEB_FOR_FIRMWARE_UPGRADE
			if (fwupgrade_redir_server_s >= 0 && FD_ISSET(fwupgrade_redir_server_s, &block_read_fdset))
				get_redir_request(fwupgrade_redir_server_s);
#endif
			p_entry = p_web_socket;
			while (p_entry)
			{
				if (p_entry->socket > 0)
				{
					if (p_entry->port == server_port)
					{
#ifdef SERVER_SSL
						if (do_sock)
#endif
						{
							if (FD_ISSET(p_entry->socket, &block_read_fdset))
								get_request(p_entry->socket);
						}
					}
					else if (p_entry->port == server_sub_port) // for 8080 port
					{
#ifdef SERVER_SSL
						if (do_sock)
#endif
						{
							if (FD_ISSET(p_entry->socket, &block_read_fdset))
								get_request(p_entry->socket);
						}
					}

#ifdef SERVER_SSL
					if (p_entry->port == ssl_server_port)
					{
						if (do_sock < 2)
						{
							if (FD_ISSET(p_entry->socket, &block_read_fdset))
							{
								get_ssl_request(p_entry->socket);
							}
						}
					}
#endif
				}
				p_entry = p_entry->next;
			}

#ifdef SERVER_SSL
#if defined(WEB_REDIRECT_BY_MAC)
			if (FD_ISSET(mac_redir_server_ssl, &block_read_fdset))
				get_mac_redir_ssl_request();
#endif
#ifdef CONFIG_CU
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
			if (FD_ISSET(captiveportal_server_s, &block_read_fdset))
			{
				get_captiveportal_request();
			}
#endif
#endif
#else
#ifdef WLAN_WEB_REDIRECT //jiunming,web_redirect
			if (FD_ISSET(redir_server_s, &block_read_fdset))
				get_redir_request(redir_server_s);
#endif
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
			if (FD_ISSET(captiveportal_server_s, &block_read_fdset))
				get_captiveportal_request();
#endif
#endif /*SERVER_SSL*/

#ifdef USE_DEONET
			if (FD_ISSET(redir_server_s, &block_read_fdset))
				get_access_redir_request(redir_server_s);
#endif
		}
	}
}

/*
 * Name: fdset_update
 *
 * Description: iterate through the blocked requests, checking whether
 * that file descriptor has been set by select.  Update the fd_set to
 * reflect current status.
 */

void fdset_update(void)
{
	request *current, *next;
	time_t current_time;
	struct web_socket_info *p_entry = NULL;

	current = request_block;

	current_time = time_counter;

	while (current) {
		time_t time_since;
		next = current->next;

		DBG(printf("%s:%d: current->remote_port=%d,current->buffer_end=%d,current->status=%d\n",__func__,__LINE__,current->remote_port,current->buffer_end,current->status);)

		time_since = current_time - current->time_last;

		/* hmm, what if we are in "the middle" of a request and not
		 * just waiting for a new one... perhaps check to see if anything
		 * has been read via header position, etc... */
		if (current->kacount && (time_since >= ka_timeout) && !current->logline) {
			SQUASH_KA(current);
			free_request(&request_block, current);
		} else if (time_since > REQUEST_TIMEOUT) {
#ifdef BOA_TIME_LOG
			log_error_doc(current);
			fputs("connection timed out\n", stderr);
#endif
			SQUASH_KA(current);
			free_request(&request_block, current);
		} else if (current->buffer_end) {
			if (FD_ISSET(current->fd, &block_write_fdset))
				ready_request(current);
		} else {
			switch (current->status) {
			case PIPE_WRITE:
			case WRITE:
				if (FD_ISSET(current->fd, &block_write_fdset))
					ready_request(current);
				else
					FD_SET(current->fd, &block_write_fdset);
				break;
			case PIPE_READ:
				if (FD_ISSET(current->data_fd, &block_read_fdset))
					ready_request(current);
				else
					FD_SET(current->data_fd, &block_read_fdset);
				break;
			case BODY_WRITE:
				if (FD_ISSET(current->post_data_fd, &block_write_fdset))
					ready_request(current);
				else
					FD_SET(current->post_data_fd, &block_write_fdset);
				break;
			default:
				if (FD_ISSET(current->fd, &block_read_fdset))
					ready_request(current);
				else
					FD_SET(current->fd, &block_read_fdset);
				break;
			}
		}
		current = next;
	}

	p_entry = p_web_socket;
	while (p_entry)
	{
		if (p_entry->socket > 0)
		{
			if (p_entry->port == server_port)
			{
				if (!lame_duck_mode && (max_connections == -1 || status.connections < max_connections))
				{
#ifdef SERVER_SSL
					if (do_sock)
#endif
					{
						FD_SET(p_entry->socket, &block_read_fdset);
					}
				}
				else
				{
#ifdef SERVER_SSL
					if (do_sock)
#endif
					{
						FD_CLR(p_entry->socket, &block_read_fdset);
					}
				}
			}
			else if (p_entry->port == server_sub_port) // for 8080 port
			{
				if (!lame_duck_mode && (max_connections == -1 || status.connections < max_connections))
				{
#ifdef SERVER_SSL
					if (do_sock)
#endif
					{
						FD_SET(p_entry->socket, &block_read_fdset);
					}
				}
				else
				{
#ifdef SERVER_SSL
					if (do_sock)
#endif
					{
						FD_CLR(p_entry->socket, &block_read_fdset);
					}
				}
			}

#ifdef SERVER_SSL
			if (p_entry->port == ssl_server_port)
			{
				if (do_sock < 2)
				{
					if (max_connections == -1 || status.connections < max_connections)
					{
					  FD_SET(p_entry->socket, &block_read_fdset);
					}
					else
					{
					  FD_CLR(p_entry->socket, &block_read_fdset);
					}
				}
			}
#endif
		}
		p_entry = p_entry->next;
	}

	req_timeout.tv_sec = (ka_timeout ? ka_timeout : REQUEST_TIMEOUT);
	req_timeout.tv_usec = 0l;	/* reset timeout */
}

/*
 * Name: die
 * Description: die with fatal error
 */

void die(int exit_code)
{
#ifdef BOA_TIME_LOG
	log_error_time();

	switch (exit_code) {
	case SERVER_ERROR:
		fputs("fatal error: exiting\n", stdout);
		break;
	case OUT_OF_MEMORY:
		perror("malloc");
		break;
	case NO_CREATE_SOCKET:
		perror("socket create");
		break;
	case NO_FCNTL:
		perror("fcntl");
		break;
	case NO_SETSOCKOPT:
		perror("setsockopt");
		break;
	case NO_BIND:
		perror("bind");
		break;
	case NO_LISTEN:
		perror("listen");
		break;
	case NO_SETGID:
		perror("setgid/initgroups");
		break;
	case NO_SETUID:
		perror("setuid");
		break;
	case NO_OPEN_LOG:
		perror("logfile fopen");	/* ??? */
		break;
	case SELECT:
		perror("select");
		break;
	case GETPWUID:
		perror("getpwuid");
		break;
	case INITGROUPS:
		perror("initgroups");
		break;
	case CANNOT_CHROOT:
                perror("chroot");
                break;
	case SHUTDOWN:
		fputs("completing shutdown\n", stderr);
		break;
	default:
		break;
	}
#endif
	syslog(LOG_WARNING, "Shutting down - %d", exit_code);

	fclose(stderr);
	exit(exit_code);
}

#ifdef SERVER_SSL
#ifdef USES_MATRIX_SSL
int
InitSSLStuff(void)
{
	int rc;

 	syslog(LOG_NOTICE, "Enabling SSL security system");

	bind_web_socket(ssl_server_port);

#ifdef CONFIG_USER_MATRIXSSL
	/*Matrix SSL initialization */
	if (matrixSslOpen() < 0) {
		die(NO_SSL);
		fprintf(stderr, "matrixSslOpen failed, exiting...");
		return 0;
	}

	/* read ssl key and cert from file*/
	if (matrixSslReadKeys(&mtrx_keys,SSL_CERTF ,SSL_KEYF , NULL, NULL) < 0)  {
		fprintf(stderr, "Error reading or parsing %s or %s.\n",
			SSL_CERTF, SSL_KEYF);
		return 0;
	}
#else
	/*Matrix SSL initialization */
	if (matrixSslOpen() < 0)
	{
		fprintf(stderr, "matrixSslOpen failed, exiting...");
		die(NO_SSL);
		return 0;
	}

	/* read ssl key and cert from file*/
	if (matrixSslNewKeys(&mtrx_keys) < 0) {
		fprintf(stderr, "matrixSslNewKeys failed, exiting...");
		return 0;
	}

	if ((rc = matrixSslLoadRsaKeys(mtrx_keys, SSL_CERTF, SSL_KEYF, NULL, NULL)) < 0)  {
		fprintf(stderr, "Error reading or parsing %s or %s, errno=%d.\n",
			SSL_CERTF, SSL_KEYF, rc);
		return 0;
	}
#endif

	return 1;
}

#ifdef WEB_REDIRECT_BY_MAC
init_mac_redirect_socket_ssl(void)
{
	int rc;

 	syslog(LOG_NOTICE, "Enabling SSL security system");
#ifdef IPV6
	if ((mac_redir_server_ssl = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1){
#else
	if ((mac_redir_server_ssl = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
#endif
		syslog(LOG_ALERT,"Couldn't create socket for ssl");
		die(NO_CREATE_SOCKET);
		return 0;
	}

	/* server socket is nonblocking */
	if (fcntl(mac_redir_server_ssl, F_SETFL, NOBLOCK) == -1){
		syslog(LOG_ALERT, "%s, %i:Couldn't fcntl", __FILE__, __LINE__);
		die(NO_FCNTL);
		return 0;
	}
	if ((setsockopt(mac_redir_server_ssl, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
		sizeof(sock_opt))) == -1){
		syslog(LOG_ALERT,"%s, %i:Couldn't sockopt", __FILE__,__LINE__);
		die(NO_SETSOCKOPT);
		return 0;
	}
	/* internet socket */
#ifdef IPV6
	server_sockaddr.sin6_family = AF_INET6;
	memcpy(&server_sockaddr.sin6_addr,&in6addr_any,sizeof(in6addr_any));
	server_sockaddr.sin6_port = htons(ssl_server_port_web_redirect);
#else
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sockaddr.sin_port = htons(ssl_server_port_web_redirect);
#endif

	if (bind(mac_redir_server_ssl, (struct sockaddr *) &server_sockaddr,
		sizeof(server_sockaddr)) == -1){
#ifdef IPV6
		syslog(LOG_ALERT, "Couldn't bind ssl to port %d", ntohs(server_sockaddr.sin6_port));
#else
		syslog(LOG_ALERT, "Couldn't bind ssl to port %d", ntohs(server_sockaddr.sin_port));
#endif
		die(NO_BIND);
		return 0;
	}

	/* listen: large number just in case your kernel is nicely tweaked */
	if (listen(mac_redir_server_ssl, backlog) == -1){
		die(NO_LISTEN);
		return 0;
	}

	if (mac_redir_server_ssl > max_fd)
		max_fd = mac_redir_server_ssl;

#ifdef CONFIG_USER_MATRIXSSL
	/*Matrix SSL initialization */
	if (matrixSslOpen() < 0) {
		die(NO_SSL);
		fprintf(stderr, "matrixSslOpen failed, exiting...");
		return 0;
	}

	/* read ssl key and cert from file*/
	if (matrixSslReadKeys(&mtrx_keys,SSL_CERTF ,SSL_KEYF , NULL, NULL) < 0)  {
		fprintf(stderr, "Error reading or parsing %s or %s.\n",
			SSL_CERTF, SSL_KEYF);
		return 0;
	}
#else
	/*Matrix SSL initialization */
	if (matrixSslOpen() < 0)
	{
		fprintf(stderr, "matrixSslOpen failed, exiting...");
		die(NO_SSL);
		return 0;
	}

	/* read ssl key and cert from file*/
	if (matrixSslNewKeys(&mtrx_keys) < 0) {
		fprintf(stderr, "matrixSslNewKeys failed, exiting...");
		return 0;
	}

	if ((rc = matrixSslLoadRsaKeys(mtrx_keys, SSL_CERTF, SSL_KEYF, NULL, NULL)) < 0)  {
		fprintf(stderr, "Error reading or parsing %s or %s, errno=%d.\n",
			SSL_CERTF, SSL_KEYF, rc);
		return 0;
	}
#endif

	return 1;
}
#endif
#else /*!USES_MATRIX_SSL*/
int
InitSSLStuff(void)
{
	syslog(LOG_NOTICE, "Enabling SSL security system");

	bind_web_socket(ssl_server_port);

	/*Init all of the ssl stuff*/
//	i don't know why this line is commented out... i found it like that - damion may-02
/*	SSL_load_error_strings();*/
	SSLeay_add_ssl_algorithms();
#ifdef CONFIG_TELMEX_DEV
	meth = TLSv1_2_server_method();
#else
	meth = SSLv23_server_method();
#endif
	if(meth == NULL){
		ERR_print_errors_fp(stderr);
		syslog(LOG_ALERT, "Couldn't create the SSL method");
		die(NO_SSL);
		return 0;
	}

	ctx = SSL_CTX_new(meth);
	if(!ctx){
		syslog(LOG_ALERT, "Couldn't create a connection context\n");
		ERR_print_errors_fp(stderr);
		die(NO_SSL);
		return 0;
	}
#if defined(CONFIG_USER_RTK_NESSUS_SCAN_FIXED)
	//fix nessus
	/* disable SSL2.0, SSL3.0, TLS1.0, TLS1.1
	   Apple, Google, Microsoft, and Mozilla jointly announced they would deprecate TLS 1.0 and 1.1 in March 2020
	 */
	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);

	if (!SSL_CTX_set_cipher_list(ctx, "ALL:!EXPORT:!LOW:!aNULL:!eNULL:!SSLv2:!3DES:!RC4")) {
		syslog(LOG_ALERT, "Failure on setting cipher list");fflush(NULL);
		close_web_socket(ssl_server_port);
		return 0;
	}
#endif
#if OPENSSL_VERSION_NUMBER >= 0x10100000L /* openssl 1.1.0 */
	/* This setter function were added in OpenSSL 1.1.0.
	 * At the time of publishing 1.1.1d, there are compability issue with
	 * different browsers for TLS 1.3, so we limit proto to TLS 1.2 */
	if (SSL_CTX_set_max_proto_version(ctx, TLS1_2_VERSION) <= 0) {
		syslog(LOG_ALERT, "Failure on setting max proto version: %d", TLS1_2_VERSION);fflush(NULL);
		close_web_socket(ssl_server_port);
		return 0;
	}
#endif
	if (SSL_CTX_use_certificate_file(ctx, SSL_CERTF, SSL_FILETYPE_PEM) <= 0) {
		syslog(LOG_ALERT, "Failure reading SSL certificate file: %s",SSL_CERTF);fflush(NULL);
		close_web_socket(ssl_server_port);
		return 0;
	}
	syslog(LOG_DEBUG, "Loaded SSL certificate file: %s",SSL_CERTF);fflush(NULL);

	if (SSL_CTX_use_PrivateKey_file(ctx, SSL_KEYF, SSL_FILETYPE_PEM) <= 0) {
		syslog(LOG_ALERT, "Failure reading private key file: %s",SSL_KEYF);fflush(NULL);
		close_web_socket(ssl_server_port);
		return 0;
	}
	syslog(LOG_DEBUG, "Opened private key file: %s",SSL_KEYF);fflush(NULL);

	if (!SSL_CTX_check_private_key(ctx)) {
		syslog(LOG_ALERT, "Private key does not match the certificate public key");fflush(NULL);
		close_web_socket(ssl_server_port);
		return 0;
	}

	/*load and check that the key files are appropriate.*/
	syslog(LOG_NOTICE,"SSL security system enabled");
	return 1;
}

#ifdef WEB_REDIRECT_BY_MAC
init_mac_redirect_socket_ssl(void)
{
	syslog(LOG_NOTICE, "Enabling SSL security system");
#ifdef IPV6
	if ((mac_redir_server_ssl = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		die(NO_CREATE_SOCKET);
		return 0;
	}
#else
	if ((mac_redir_server_ssl = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
		syslog(LOG_ALERT,"Couldn't create socket for ssl");
		die(NO_CREATE_SOCKET);
		return 0;
	}
#endif /*IPV6*/

	/* server socket is nonblocking */
	if (fcntl(mac_redir_server_ssl, F_SETFL, NOBLOCK) == -1){
		syslog(LOG_ALERT, "%s, %i:Couldn't fcntl", __FILE__, __LINE__);
		die(NO_FCNTL);
		return 0;
	}

	if ((setsockopt(mac_redir_server_ssl, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
		sizeof(sock_opt))) == -1){
		syslog(LOG_ALERT,"%s, %i:Couldn't sockopt", __FILE__,__LINE__);
		die(NO_SETSOCKOPT);
		return 0;
	}

	/* internet socket */
#ifdef IPV6
	server_sockaddr.sin6_family = AF_INET6;
	memcpy(&server_sockaddr.sin6_addr,&in6addr_any,sizeof(in6addr_any));
	server_sockaddr.sin6_port = htons(ssl_server_port_web_redirect);
#else
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sockaddr.sin_port = htons(ssl_server_port_web_redirect);
#endif

	if (bind(mac_redir_server_ssl, (struct sockaddr *) &server_sockaddr,
		sizeof(server_sockaddr)) == -1){
#ifdef IPV6
		syslog(LOG_ALERT, "Couldn't bind ssl to port %d", ntohs(server_sockaddr.sin6_port));
#else
		syslog(LOG_ALERT, "Couldn't bind ssl to port %d", ntohs(server_sockaddr.sin_port));
#endif
		die(NO_BIND);
		return 0;
	}

	/* listen: large number just in case your kernel is nicely tweaked */
	if (listen(mac_redir_server_ssl, backlog) == -1){
		die(NO_LISTEN);
		return 0;
	}

	if (mac_redir_server_ssl > max_fd)
		max_fd = mac_redir_server_ssl;

	/*Init all of the ssl stuff*/
//	i don't know why this line is commented out... i found it like that - damion may-02
/*	SSL_load_error_strings();*/
	SSLeay_add_ssl_algorithms();
	meth = SSLv23_server_method();
	if(meth == NULL){
		ERR_print_errors_fp(stderr);
		syslog(LOG_ALERT, "Couldn't create the SSL method");
		die(NO_SSL);
		return 0;
	}

	ctx = SSL_CTX_new(meth);
	if(!ctx){
		syslog(LOG_ALERT, "Couldn't create a connection context\n");
		ERR_print_errors_fp(stderr);
		die(NO_SSL);
		return 0;
	}
#if defined(CONFIG_USER_RTK_NESSUS_SCAN_FIXED)
	//fix nessus
	/* disable SSL2.0, SSL3.0, TLS1.0, TLS1.1
	   Apple, Google, Microsoft, and Mozilla jointly announced they would deprecate TLS 1.0 and 1.1 in March 2020
	 */
	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);

	if (!SSL_CTX_set_cipher_list(ctx, "ALL:!EXPORT:!LOW:!aNULL:!eNULL:!SSLv2:!3DES:!RC4")) {
		syslog(LOG_ALERT, "Failure on setting cipher list");fflush(NULL);
		close(server_ssl);
		return 0;
	}
#endif
	if (SSL_CTX_use_certificate_file(ctx, SSL_CERTF, SSL_FILETYPE_PEM) <= 0) {
		syslog(LOG_ALERT, "Failure reading SSL certificate file: %s",SSL_CERTF);fflush(NULL);
		close(mac_redir_server_ssl);
		return 0;
	}
	syslog(LOG_DEBUG, "Loaded SSL certificate file: %s",SSL_CERTF);fflush(NULL);

	if (SSL_CTX_use_PrivateKey_file(ctx, SSL_KEYF, SSL_FILETYPE_PEM) <= 0) {
		syslog(LOG_ALERT, "Failure reading private key file: %s",SSL_KEYF);fflush(NULL);
		close(mac_redir_server_ssl);
		return 0;
	}
	syslog(LOG_DEBUG, "Opened private key file: %s",SSL_KEYF);fflush(NULL);

	if (!SSL_CTX_check_private_key(ctx)) {
		syslog(LOG_ALERT, "Private key does not match the certificate public key");fflush(NULL);
		close(mac_redir_server_ssl);
		return 0;
	}

	/*load and check that the key files are appropriate.*/
	syslog(LOG_NOTICE,"SSL security system enabled");
	return 1;
}
#endif
#endif /* USES_MATRIX_SSL */
#endif /*SERVER_SSL*/

#ifdef CONFIG_USER_BOA_CSRF_TOKEN_INDEPDENT
struct token_info * search_token_list(request * req)
{
	struct token_info * pToken_info;
	for(pToken_info = user_token_list;pToken_info;pToken_info = pToken_info->next){
		if(!strcmp(req->remote_ip_addr,pToken_info->remote_ip_addr))
			break;
	}
	return pToken_info;
}

int getCSRFTokenfromTokenInfo(unsigned char *g_csrf_token,request * req) {
	struct token_info * pToken_info;
	pToken_info = search_token_list(req);
	if (pToken_info !=NULL){
		memcpy(g_csrf_token, pToken_info->g_csrf_token_indepent, sizeof(pToken_info->g_csrf_token_indepent));
		return 0;
	}
	return -1;
}

int setCSRFTokentoTokenInfo(unsigned char *g_csrf_token,request * req){
	struct token_info * pToken_info;
	pToken_info = search_token_list(req);
	if (pToken_info !=NULL){
		memcpy(pToken_info->g_csrf_token_indepent, g_csrf_token, sizeof(pToken_info->g_csrf_token_indepent));
		return 0;
	}
	else{
		pToken_info = malloc(sizeof(struct token_info));
		memcpy(pToken_info->g_csrf_token_indepent, g_csrf_token, sizeof(pToken_info->g_csrf_token_indepent));
		memcpy(pToken_info->remote_ip_addr, req->remote_ip_addr, sizeof(req->remote_ip_addr));
		pToken_info->next = user_token_list;
		user_token_list = pToken_info;
	}
	return -1;
}
#endif /* CONFIG_USER_BOA_CSRF_TOKEN_INDEPDENT */

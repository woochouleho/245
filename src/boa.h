/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Some changes Copyright (C) 1996,97 Larry Doolittle <ldoolitt@jlab.org>
 *  Some changes Copyright (C) 1997 Jon Nelson <nels0988@tc.umn.edu>
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

#ifndef _BOA_H
#define _BOA_H

#define BOA_DEBUG
#ifdef BOA_DEBUG
extern int debug_flag;
#define DBG(x) if(debug_flag) x
#else
#define DBG(x)
#endif
#ifndef __BYTE_ORDER
#ifdef __MIPSEB__
#define __BYTE_ORDER __BIG_ENDIAN
#endif
#endif



#include "defines.h"
#include "globals.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>				/* OPEN_MAX */
#include <netinet/in.h>			/* sockaddr_in, sockaddr */
#include <stdlib.h>				/* malloc, free, etc. */
#include <stdio.h>				/* stdin, stdout, stderr */
#include <string.h>				/* strdup */
#include <time.h>				/* localtime, time */
#include <pwd.h>

#include <arpa/inet.h>			/* inet_ntoa */

#include <unistd.h>
#include <sys/mman.h>			/* mmap */
#include <sys/time.h>			/* select */
#include <sys/resource.h>		/* setrlimit */
#include <sys/types.h>			/* socket, bind, accept */
#include <sys/socket.h>			/* socket, bind, accept, setsockopt, */
#include <sys/stat.h>			/* open */
#include "syslog.h"

#include "compat.h"				/* oh what fun is porting */

#include <rtk/mib.h>

#ifdef CONFIG_USER_BOA_CSRF
#define CSRF_TOKEN_STRING "csrftoken"
#endif

/* new parser */
#define ALIAS                   0
#define SCRIPTALIAS             1
#define REDIRECT                2

struct ccommand {
        char *name;
        int type;
        void (*action) (char *,char *,void *);
        void *object;
};
struct ccommand *lookup_keyword(char *c);

//andrew, added digest support
//#define SUPPORT_AUTH_DIGEST 1

#define HTTP_SESSION_MAX 32
#define HTTP_AUTH_BASIC  0
#define HTTP_AUTH_DIGEST  1

#define HTTP_DA_REALMLEN 31
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
#define HTTP_DA_NONCELEN 32
#else
#define HTTP_DA_NONCELEN 21
#endif

#define HTTP_DA_OPAQUELEN 9

#define HTTP_DA_EXPIRY_TIME 300
struct http_session {
	char realm[HTTP_DA_REALMLEN + 1];
	char nonce[HTTP_DA_NONCELEN + 1];
	char opaque[HTTP_DA_OPAQUELEN + 1];
	unsigned long modified;
	unsigned long ncount;
	int in_use;
#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
#ifdef IPV6
	char remote_ip_addr[INET6_ADDRSTRLEN];
#else
	char remote_ip_addr[20];
#endif
#endif
};

#ifdef BOA_AUTH_WEB_PASSWORD_ENCRYPT
typedef enum { NONCE_CHECK_FAIL=0, NONCE_CHECK_OK=1, NONCE_CHECK_TIMEOUT=2} NONCE_CHECK_RESULT;
#endif

struct convertHtmlStrMap_s {
	char tagChar;
	char mapStr[8];
};
static struct convertHtmlStrMap_s htmlTagCharMapping[] = {
	{'&',   "&amp;" },
	{'<',   "&lt;"  },
	{'>',   "&gt;"  },
	{'\"',  "&quot;"},
	{'\'',  "&#39;" },
	{0, 	0}
};

/* alias */

void add_alias(char *fakename, char *realname, int script);
void chroot_aliases();
int translate_uri(request * req);
int init_script_alias(request * req, alias * current);
void dump_alias(void);


/* boa */

void die(int exit_code);
void fdset_update(void);

/* config */

void read_config_files(void);
#ifdef EMBED
	void set_server_port(int port);
	void chcek_server_port(void);
#endif

/* get */

int init_get(request * req);
int process_get(request * req);
int get_dir(request * req, struct stat *statbuf);

/* hash */

void add_mime_type(char *extension, char *type);
void add_virtual_host(char *name,char *docroot);
int get_mime_hash_value(char *extension);
char *get_mime_type(char *filename);
char *get_home_dir(char *name);
char *get_virtual_host(char *name);
void chroot_virtual_hosts();
void dump_mime(void);
void dump_passwd(void);
#ifdef USE_BROWSERMATCH
void add_browsermatch(char *browser,char *action);
int get_browser_hash_value(char *browser);
void browser_match_request(request *req);
#endif

/* log */

void open_logs(void);
void close_access_log(void);
void log_access(request * req);
void log_referer(request * req);
void log_user_agent(request * req);
void log_error_time(void);
void log_error_doc(request * req);
void boa_perror(request * req, char *message);

/* queue */

void block_request(request * req);
void ready_request(request * req);
void dequeue(request ** head, request * req);
void enqueue(request ** head, request * req);

/* read */

int read_header(request * req);
int read_body(request * req);
int write_body(request * req);

/* request */

request *new_request(void);
void get_request(int sock_fd);
void free_request(request ** list_head_addr, request * req);
void process_requests(void);
int process_header_end(request * req);
int process_header_line(request * req);
int process_logline(request * req);
int process_option_line(request * req);
void add_accept_header(request * req, char *mime_type);
void free_requests(void);
void init_free_requests(int num);
void dump_request(request *req);

/* response */

void print_content_type(request * req);
void print_content_length(request * req);
void print_last_modified(request * req);
void print_http_headers(request * req);

void send_r_request_ok(request * req);	/* 200 */
void send_redirect_perm(request * req, char *url);	/* 301 */
void send_redirect_temp(request * req, char *url);	/* 302 */
void send_r_not_modified(request * req);	/* 304 */
void send_r_bad_request(request * req);		/* 400 */
void send_r_bad_request_2(request * req);		/* 400 */
void send_r_unauthorized(request * req, char *name);	/* 401 */
void send_r_unauthorized_digest(request * req, struct http_session *);
void send_r_forbidden(request * req);	/* 403 */
void send_r_not_found(request * req);	/* 404 */
void send_r_entity_too_large(request * req); /* 413 */
void send_r_error(request * req);	/* 500 */
void send_r_not_implemented(request * req);		/* 501 */
void send_r_bad_version(request * req);		/* 505 */
//#ifdef WEB_REDIRECT_BY_MAC
void send_popwin_and_reload(request * req, char *url);
//#endif
void send_r_forbidden3(request * req, long time_last);
void send_r_forbidden_timeout(request * req, long time_last);
void send_web_redirect_msg(request * req);

char *escape_url(char *url);
char *escape_msg(char *msg);

#ifdef CU_APPSCAN_RSP_DBG
void send_appscan_response(request * req);
#endif

#ifdef RTK_BOA_PORTAL_TO_NET_LOCKED_UI
extern void send_r_portal_net_locked(request * req);
extern int rtk_access_internet_with_net_locked(request * req);
extern int rtk_portal_to_netlocked_ui(request * req);
extern int rtk_get_wanip_of_andlink(char *wanIface, char *wanIpStr, int wanIpStrLen);
#endif

/* cgi */

void create_common_env(void);
void create_env(request * req);
#define env_gen(x,y) env_gen_extra(x,y,0)
char *env_gen_extra(const char *key, const char *value, int extra);
void add_cgi_env(request * req, char *key, char *value);
void complete_env(request * req);
char *create_argv(request * req, char **aargv);
int init_cgi(request * req);

/* signals */

void init_signals(void);
void sighup_run(void);
void sigchld_run(void);
void lame_duck_mode_run(int server_s);

/* util */

void clean_pathname(char *pathname);
int strmatch(char *str,char *s2);
int modified_since(time_t * mtime, char *if_modified_since);
int month2int(char *month);
char *to_upper(char *str);
int unescape_uri(char *uri);
char *escape_uri(char *uri);
void close_unused_fds(request * head);
void fixup_server_root(void);
char *get_commonlog_time(void);
int req_write_rfc822_time(request *req, time_t s);
char *simple_itoa(int i);
char *escape_string(char *inp, char *buf);
int req_write(request *req, char *msg);
int req_flush(request *req);
int base64decode(void *dst,char *src,int maxlen);
void base64encode(unsigned char *from, char *to, int len);

/* cgi_header */
int process_cgi_header(request * req);

/* pipe */
int write_from_pipe(request * req);
int read_from_pipe(request * req);

/* timestamp */
void timestamp(void);

extern int request_type(request *req);

/* nls */
#ifdef USE_NLS
void nls_load_codepage(char *name,char *filename);
int get_cp_hash_value(char *name);
unsigned char *nls_get_table(char *name);
void add_cp_brows(char *browser,char *codepage);
int nls_try_redirect(request * req);
void nls_set_codepage(request *req);
#endif

/* auth */
#ifdef USE_AUTH
void* auth_add(char *directory,char *file);
void auth_add_digest(char *directory,char *file,char* realm);
int auth_authorize(request * req);
void auth_check();
void nls_convert(unsigned char * buffer, unsigned char * table, long count);
void dump_auth(void);
#endif

extern char usName[MAX_NAME_LEN];
extern char suName[MAX_NAME_LEN];
extern time_t time_counter;

#ifdef ACCOUNT_LOGIN_CONTROL
extern struct account_info su_account;
extern struct account_info us_account;
extern struct errlogin_entry * errlogin_list;
#endif

//added by xl_yue
#ifdef USE_LOGINWEB_OF_SERVER
#ifdef AUTO_LOGOUT_SUPPORT
#ifdef CONFIG_TRUE
#define	LOGIN_AUTOEXIT_TIME	1800 //about 30 mintues
#else
#define	LOGIN_AUTOEXIT_TIME	600 //about 5 mintues
#endif
#endif
#define	LOGIN_MAX_TIME		86400 //about 1 day
#ifdef LOGIN_ERR_TIMES_LIMITED
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#define	CWMP_MAX_LOGIN_NUM	10
#endif
#define	MAX_LOGIN_NUM		3
#ifdef CONFIG_TRUE
#define	LOGIN_ERR_WAIT_TIME	1800   //about 30 minute
#else
#define	LOGIN_ERR_WAIT_TIME	60   //about 1 minute
#endif
struct errlogin_entry * search_errlog_list(request * req);
#endif
#endif

//added by xl_yue
#ifdef USE_LOGINWEB_OF_SERVER
int free_from_login_list(request * req);
#ifdef ONE_USER_LIMITED
#ifdef ONE_USER_BY_SESSIONID
int get_sessionid_from_cookie(request *req, char *id);
int free_from_login_list_by_sessionid(char *sess);
#else
int free_from_login_list_by_ip_addr(char * remote_ip_addr);
#endif
extern struct account_status suStatus;
extern struct account_status usStatus;
#endif
extern struct user_info * user_login_list;

#ifdef LOGIN_ERR_TIMES_LIMITED
extern struct errlogin_entry * errlogin_list;
int free_from_errlog_list(request * req);
#endif

extern struct user_info * search_login_list(request * req);
extern int free_from_log_list(request * req);
extern void free_login_list(void);
#endif

#ifdef CONFIG_DEONET_SESSION
extern int getNumLoginUserInfo();
#endif


int boaASPDefine(const char* const name, int (*fn)(int eid, request * req,  int argc, char **argv));
int boaFormDefine(const char* const name, void (*fn)(request * req,  char* path, char* query));

#ifdef CONFIG_CU_BASEON_CMCC
extern unsigned char web_style_ln;
#endif

void free_web_socket_list(void);
void init_web_socket_list(void);
void dump_web_socket_list(void);
struct web_socket_info *search_web_socket_list(int ip_ver, int port);
int add_web_socket_list(int socket, int ip_ver, int port);
int delete_web_socket_list(struct web_socket_info *p_entry);
int bind_web_socket(int port);
extern int rebind_web_socket(void);
int close_web_socket(int port);
extern struct web_socket_info *p_web_socket;
extern int strContainXSSChar(char *str);
extern char *strValToASP(char *str);

char *memstr(char *haystack, char *needle, int size);
char *memcasestr(char *haystack, char *needle, int size);
int attack_check(char *buf, int len);
int attack_req_check(request * req);

#define IGMPPROXY_RUNFILE "/var/run/igmp_pid"

#ifdef CONFIG_CU
extern unsigned char appscan_flag;
#ifdef CU_APPSCAN_RSP_DBG
extern unsigned int appscan_rsp_flag;
#endif
#endif
#endif

/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Some changes Copyright (C) 1996 Larry Doolittle <ldoolitt@jlab.org>
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
 *  $Id: response.c,v 1.4 2008/01/23 12:45:16 jiunming Exp $
 */

/* boa: response.c */

#include "asp_page.h"
#include "syslog.h"
#include "boa.h"
#include "port.h"
#include <rtk/sysconfig.h>
#include "LINUX/form_src/multilang.h"
#include "./LINUX/utility.h"


static char e_s[MAX_HEADER_LENGTH * 3];
#ifdef CONFIG_CU
extern unsigned char g_appscan;
#ifdef CU_APPSCAN_RSP_DBG
extern unsigned int appscan_rsp_flag;
#endif
#endif
extern char *escapeHtmlstr(char *str);

int generateRandomNumber(int seed) 
{
	int randomNumber = 0;
	int i = 0;

	srand(seed);

	for (i = 0; i < 10; i++) 
	{
		randomNumber = randomNumber * 10 + rand() % 10;
	}

	return randomNumber;
}

void print_set_cookie(request * req)
{
#if defined(ONE_USER_BY_SESSIONID) || defined(CONFIG_DEONET_SESSION)
	char buf[64] = {0,};
	char sessionid[32] = {0,};
	char *sess = NULL;
	struct user_info * pUser_info = NULL;
	char *p_char = NULL;

	req_write(req, "Set-Cookie: ");
	if (get_sessionid_from_cookie(req, sessionid)) {
		snprintf(buf, sizeof(buf), "sessionid=%s; Path=/; ", sessionid);
	} else {
		int tmp;
		time_counter = getSYSInfoTimer();

		tmp = generateRandomNumber(time_counter);

		snprintf(buf, sizeof(buf), "sessionid=%ld; Path=/; ", tmp);
	}

	for (pUser_info = user_login_list; pUser_info; pUser_info = pUser_info->next) {
		if (!strcmp(req->remote_ip_addr, pUser_info->remote_ip_addr)) 
			break;
	}

	if (pUser_info) {
		memset(sessionid, 0, sizeof(sessionid));
		p_char = strstr(buf, "sessionid=");
		if (p_char && sscanf(p_char, "sessionid=%[^;]", sessionid) > 0) 
			snprintf(pUser_info->session_id, sizeof(pUser_info->session_id), "%s", sessionid);
	}

//	fprintf(stderr, "<%s:%d> buf=%s\n", __func__, __LINE__, buf);

	req_write(req, buf);
	
	req_write(req, "\r\n");
#endif
}

void print_content_type(request * req)
{
	req_write(req, "Content-Type: ");
#ifdef WLAN_RTIC_SUPPORT
	if(strstr(req->request_uri, "wlrtic_url.asp"))
		req->content_type="text/xml";
#endif
	if (req->content_type)
		req_write(req,req->content_type);
	else	req_write(req, get_mime_type(req->request_uri));
#ifdef USE_CHARSET_HEADER
#ifdef USE_BROWSERMATCH
       if (req->send_charset)
#endif
         {
	  req_write(req,"; charset=");
#ifdef USE_NLS
		if (req->cp_name)
		{
			req_write(req,req->cp_name);
		}else
#endif
	  if (local_codepage)
	    req_write(req,local_codepage);
	  else req_write(req,DEFAULT_CHARSET);
         }
#endif
	req_write(req, "\r\n");
}

void print_content_length(request * req)
{
	req_write(req, "Content-Length: ");
	req_write(req, simple_itoa(req->filesize));
	req_write(req, "\r\n");
}

void print_last_modified(request * req)
{
	req_write(req, "Last-Modified: ");
	req_write_rfc822_time(req, req->last_modified);
	req_write(req, "\r\n");
}

void print_http_headers(request * req)
{

	req_write(req, "Date: ");
	req_write_rfc822_time(req, 0);
	req_write(req, "\r\nServer: " SERVER_VERSION "\r\n");
	req_write(req, "X-Frame-Options: SAMEORIGIN\r\n");    // Add this line. To add X-Frame-Options into HTTP header

//fix nessus plugin 84502 - HSTS Missing From HTTPS Server
#ifdef CONFIG_CU
#ifdef SERVER_SSL
	if (req->ssl){
		//Strict-Transport-Security: max-age=expireTime [; includeSubDomains] [; preload]
		req_write(req, "Strict-Transport-Security: max-age=63072000; includeSubDomains; preload\r\n");	  // Add this line. To add HSTS into HTTP header
		//printf("%s %d Strict-Transport-Security: max-age=63072000; includeSubDomains; preload\n",__FUNCTION__,__LINE__);
	}
#endif
#endif

	if (req->keepalive == KA_ACTIVE) {
		req_write(req, "Connection: Keep-Alive\r\n" \
			"Keep-Alive: timeout=");
		req_write(req, simple_itoa(ka_timeout));
		req_write(req, ", max=");
		req_write(req, simple_itoa(ka_max));
		req_write(req, "\r\n");
	} else
		req_write(req, "Connection: close\r\n");

	if(req->is_cgi)
		req_write(req, "Cache-Control: no-cache\r\n");
	
#ifdef CONFIG_CU
	if(req->user_agent){
		if(g_appscan ==1 && (appscan_flag != 2)){
			req_write(req, "Content-Security-Policy: default-src 'self';style-src 'self' 'unsafe-inline' ; script-src 'self' 'nonce-2726c7f26c'; object-src 'self'; frame-ancestors 'self';\r\n");
		}
	}
#endif
}

/* The routines above are only called by the routines below.
 * The rest of Boa only enters through the routines below.
 */

/* R_REQUEST_OK: 200 */
void send_r_request_ok(request * req)
{
	struct user_info * pUser_info = NULL;
	char session_id[32] = {0,};
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	req->response_status = R_REQUEST_OK;
	if (req->simple)
		return;

	req_write(req, "HTTP/1.0 200 OK\r\n");

	req_write(req, "X-XSS-Protection: 1;mode=block\r\n");
	//req_write(req, "X-Frame-Options: SAMEORIGIN\r\n");    // Add this line. To add X-Frame-Options into HTTP header
	print_http_headers(req);
	if (!strncmp(req->request_uri, "//", 2)) 
		print_set_cookie(req);
	if (req->is_cgi)
	{
#ifdef SUPPORT_ASP
		if (strstr(req->request_uri,".js")) /* special handle, let share.js could apply multi-language */
			req->content_type="text/javascript";
		else if(strstr(req->request_uri, ".css")){
			req->content_type="text/css";
		}
		else if(!req->content_type || !strstr(req->content_type, "text/"))
			req->content_type="text/html";
#endif
		if (req->content_type)
			print_content_type(req);
#ifndef SUPPORT_ASP
		else
			if (req->is_cgi)
			{
				req_write(req, req->header_line);
				req_write(req, "\r\n");
			}
#endif		
#ifndef NO_COOKIES
		if (req->cookie)
		{
			req_write(req, req->cookie);
			req_write(req, "\r\n");	
		}
#endif
	}else
	{
		print_content_type(req);
		print_content_length(req);
		print_last_modified(req);
	}

//#ifdef CONFIG_USER_BOA_CSRF
//	req_write(req, "X-Frame-Options: SAMEORIGIN\r\n");
//#endif
		req_write(req, "\r\n");	/* terminate header */
}

/* R_MOVED_PERM: 301 */
void send_redirect_perm(request * req, char *url)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
#ifdef CONFIG_CU
	if(req->user_agent && req->method != M_HEAD && strcmp(url, "")==0){
		send_r_bad_request(req);
		return;
	}
#endif

	req->response_status = R_MOVED_PERM;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 301 Moved Permanently\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n");

		req_write(req, "Location: ");
		req_write(req, escape_string(url, e_s));
		req_write(req, "\r\n\r\n");
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>301 Moved Permanently</TITLE></HEAD>\n"
					 "<BODY>\n<H1>301 Moved</H1>The document has moved\n"
					 "<A HREF=\"");
		req_write(req, escape_string(url, e_s));
		req_write(req, "\">here</A>.\n</BODY></HTML>\n");
	}
	req_flush(req);
}

/* R_MOVED_TEMP: 302 */
void send_redirect_temp(request * req, char *url)
{

	char ip6Addr[MAX_V6_IP_LEN]={0};

	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)

	req->response_status = R_MOVED_TEMP;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 302 Moved Temporarily\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n");

		req_write(req, "Location: ");

		mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR, (void *)&ip6Addr, sizeof(ip6Addr));

		if(strstr( url, ip6Addr )) //fix ipv6 logout redirect
			req_write(req, url);
		else
			req_write(req, escape_string(url, e_s));
		req_write(req, "\r\n\r\n");
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>302 Moved Temporarily</TITLE></HEAD>\n"
					 "<BODY>\n<H1>302 Moved</H1>The document has moved\n"
					 );
		#if 0
					 //"<A HREF=\"");
		//req_write(req, escape_string(url, e_s));
		//req_write(req, "\">here</A>.\n
		#endif
		req_write(req,"</BODY></HTML>\n");
	}
	req_flush(req);
}

/* R_NOT_MODIFIED: 304 */
void send_r_not_modified(request * req)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	req_write(req, "HTTP/1.0 304 Not Modified\r\n");
	req->response_status = R_NOT_MODIFIED;
	print_http_headers(req);
	print_content_type(req);
	req_write(req, "\r\n");		/* terminate header */
	req_flush(req);
}

/* R_BAD_REQUEST: 400 */
void send_r_bad_request(request * req)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	req->response_status = R_BAD_REQUEST;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 400 Bad Request\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n"
				"<BODY><H1>400 Bad Request</H1>\nYour client has issued "
					 "a malformed or illegal request.\n</BODY></HTML>\n");
	}
	req_flush(req);
}

/* R_BAD_REQUEST: 400 */
void send_r_bad_request_2(request * req)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	req->response_status = R_BAD_REQUEST;
	req_write(req, "HTTP/1.0 400 Bad Request\r\n");
	print_http_headers(req);
	req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
		req_write(req, "<HTML><HEAD><TITLE>400 Bad Request</TITLE><META http-equiv=content-type content=\"text/html; charset=utf-8\"></HEAD>\n"
					"<BODY>\n<script type=\"text/javascript\">setTimeout('countdown()', 1000);\n"
					"function countdown() {\n"
					"var s = document.getElementById('timer');\n"
					"s.innerHTML = s.innerHTML - 1;\n"
					"if (s.innerHTML == 0)\n"
					"window.location = '/admin/login.asp';\n"
					"else\n"
					"setTimeout('countdown()', 1000);\n"
					"}\n"
					"</script>\n"
					"<H1>400 Bad Request</H1>\n" //Your client has issued a malformed or illegal request.
					"客户端异常, 请重新操作\n<br><br>系统将于<span id='timer'>5</span>秒后, 为您自动返回首页\n</BODY></HTML>\n");
	req_flush(req);
}

/* R_UNAUTHORIZED: 401 */
void send_r_unauthorized(request * req, char *realm_name)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	req->response_status = R_UNAUTHORIZED;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 401 Unauthorized\r\n");
		print_http_headers(req);
		req_write(req, "WWW-Authenticate: Basic realm=\"");
		req_write(req, realm_name);
		req_write(req, "\"\r\n");
		req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>401 Unauthorized</TITLE></HEAD>\n"
				 "<BODY><H1>401 Unauthorized</H1>\nYour client does not "
					 "have permission to get URL ");
		req_write(req, escape_string(req->request_uri, e_s));
		req_write(req, " from this server.\n</BODY></HTML>\n");
	}
	req_flush(req);
}


#if SUPPORT_AUTH_DIGEST
/* R_UNAUTHORIZED: 401 */
void send_r_unauthorized_digest(request * req, struct http_session *da)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	req->response_status = R_UNAUTHORIZED;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 401 Unauthorized\r\n");
		print_http_headers(req);
		req_write(req, "WWW-Authenticate: Digest realm=\"");
		req_write(req, da->realm);
		req_write(req, "\", qop=\"auth\", nonce=\"");
		req_write(req, da->nonce);
		req_write(req, "\", opaque=\"");
		req_write(req, da->opaque);	
		req_write(req, "\"\r\n");
		req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>401 Unauthorized</TITLE></HEAD>\n"
				 "<BODY><H1>401 Unauthorized</H1>\nYour client does not "
					 "have permission to get URL ");
		req_write(req, escape_string(req->request_uri, e_s));
		req_write(req, " from this server.\n</BODY></HTML>\n");
	}
	req_flush(req);
}
#endif

/* R_FORBIDDEN: 403 */
void send_r_forbidden(request * req)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	req->response_status = R_FORBIDDEN;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 403 Forbidden\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
	}
	#ifdef CONFIG_E8B
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>400 Bad Request</TITLE><META http-equiv=content-type content=\"text/html; charset=utf-8\"></HEAD>\n"
					"<BODY>\n<script type=\"text/javascript\">setTimeout('countdown()', 1000);\n"
					"function countdown() {\n"
					"var s = document.getElementById('timer');\n"
					"s.innerHTML = s.innerHTML - 1;\n"
					"if (s.innerHTML == 0)\n"
					"window.location = '/admin/login.asp';\n"
					"else\n"
					"setTimeout('countdown()', 1000);\n"
					"}\n"
					"</script>\n"
					"<H1>400 Bad Request</H1>\n" //Your client has issued a malformed or illegal request.
					"客户端异常, 请重新操作\n<br><br>系统将于<span id='timer'>5</span>秒后, 为您自动返回首页\n</BODY></HTML>\n");
	}
	#else
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n"
					 "<BODY><H1>403 Forbidden</H1>\nYour client does not "
					 "have permission to get URL ");
		req_write(req, escape_string(req->request_uri, e_s));
		req_write(req, " from this server.\n</BODY></HTML>\n");
	}
	#endif
	req_flush(req);
}

void send_r_forbidden3(request * req, long time_last)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	char time_last_string[50]={0};
	req->response_status = R_FORBIDDEN;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 403 Forbidden\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>403 Forbidden</TITLE><META http-equiv=content-type content=\"text/html; charset=utf-8\"></HEAD>\n");
		req_write(req, "<BODY>\n<script type=\"text/javascript\">setTimeout('countdown()', 1000);\n"
					"function countdown() {\n"
					"var s = document.getElementById(\"timer\");\n"
					"if (s.innerHTML <= 0)\n"
					"window.location.replace(\"/admin/login.asp\");\n"
					"else\n"
					"setTimeout('countdown()', 1000);\n"
					"if (s.innerHTML > 0)\n"
					"s.innerHTML = s.innerHTML - 1;\n"
					"}\n"
					"</script>\n"
#ifdef CONFIG_TELMEX_DEV
					"<H1>403 Forbidden</H1>\nYou have tried too many times, please login later!\n"
#else
					"<H1>403 Forbidden</H1>\n您已尝试多次, 请稍后再登入\n"
#endif
					); //You had too many tries, please wait for a while to login again.
#ifdef CONFIG_TELMEX_DEV
		req_write(req, "<br><br>System will be back to home page after <span id='timer'>");
#else
		req_write(req, "<br><br>系统将于<span id='timer'>");
#endif
		snprintf(time_last_string, sizeof(time_last_string), "%ld", time_last);
		req_write(req, time_last_string);
#ifdef CONFIG_TELMEX_DEV
		req_write(req, "</span> seconds!\n</BODY></HTML>\n");
#else
		req_write(req, "</span>秒后, 为您自动返回首页\n</BODY></HTML>\n");
#endif
	}
	req_flush(req);
}

#ifdef CONFIG_USER_BOA_WITH_MULTILANG
void send_r_forbidden_timeout(request * req, long time_last)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	char time_last_string[50]={0};
	req->response_status = R_FORBIDDEN;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 403 Forbidden\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>403 Forbidden</TITLE><META http-equiv=content-type content=\"text/html; charset=utf-8\"></HEAD>\n");
		boaWrite(req, "<BODY>\n<script type=\"text/javascript\">setTimeout('countdown()', 1000);\n"
					"function countdown() {\n"
					"var s = document.getElementById(\"timer\");\n"
					"if (s.innerHTML <= 0)\n"
					"window.location.replace(\"/admin/login.asp\");\n"
					"else\n"
					"setTimeout('countdown()', 1000);\n"
					"if (s.innerHTML > 0)\n"
					"s.innerHTML = s.innerHTML - 1;\n"
					"}\n"
					"</script>\n"
					"<H1>403 Forbidden</H1>\n %s \n", multilang(LANG_LOGIN_ERROR_LOGIN_LATER)); //You had too many tries, please wait for a while to login again.
		boaWrite(req, "<br><br> %s <span id='timer'>", multilang(LANG_SYSTEM_WILL_COMEBACK));
		snprintf(time_last_string, sizeof(time_last_string), "%ld", time_last);
		req_write(req, time_last_string);
		boaWrite(req, "</span> %s\n</BODY></HTML>\n", multilang(LANG_REDIRECT_TO_MAINPAGE));
	}
	req_flush(req);
}
#endif

/* R_NOT_FOUND: 404 */
void send_r_not_found(request * req)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
#ifdef CONFIG_USER_WEB_404_REDIR	
	char redirectUrl[256]={0};
#ifdef SERVER_SSL
	if(req->ssl == NULL)//https request needn't do redirect
#endif
	{
		if(mib_get_s( MIB_WEB_404_REDIR_URL, (void *)redirectUrl, sizeof(redirectUrl) )){
			if(redirectUrl[0]){
				send_redirect_temp(req, redirectUrl);
				return;
			}
		}
	}
#endif

	req->response_status = R_NOT_FOUND;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 404 Not Found\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n"
					 "<BODY><H1>404 Not Found</H1>\nThe requested URL ");
		req_write(req, escape_string(req->request_uri, e_s));
		req_write(req, " was not found on this server.\n</BODY></HTML>\n");
	}
	req_flush(req);
}

/* R_ENTITY_TOO_LARGE: 413 */
void send_r_entity_too_large(request * req)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	req->response_status = R_ENTITY_TOO_LARGE;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 413 Request Entity Too Large\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>413 Request Entity Too Large</TITLE><META http-equiv=content-type content=\"text/html; charset=utf-8\"></HEAD>\n"
					 "<BODY>\n<script type=\"text/javascript\">setTimeout('countdown()', 1000);\n"
					"function countdown() {\n"
					"var s = document.getElementById('timer');\n"
					"s.innerHTML = s.innerHTML - 1;\n"
					"if (s.innerHTML == 0)\n"
					"window.location = '/admin/login.asp';\n"
					"else\n"
					"setTimeout('countdown()', 1000);\n"
					"}\n"
					"</script>\n"
					"<H1>413 Request Entity Too Large</H1>\n此页面 "); //The requested URL was not found on this server. 
		req_write(req, escape_string(req->request_uri, e_s));
		req_write(req, " 不存在, 请重新操作\n<br><br>系统将于<span id='timer'>5</span>秒后, 为您自动返回首页\n</BODY></HTML>\n");
	}
	req_flush(req);
}

/* R_ERROR: 500 */
void send_r_error(request * req)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	req->response_status = R_ERROR;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 500 Server Error\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>500 Server Error</TITLE></HEAD>\n"
			   "<BODY><H1>500 Server Error</H1>\nThe server encountered "
			   "an internal error and could not complete your request.\n"
					 "</BODY></HTML>\n");
	}
	req_flush(req);
}

/* R_NOT_IMP: 501 */
void send_r_not_implemented(request * req)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	req->response_status = R_NOT_IMP;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 501 Not Implemented\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>501 Not Implemented</TITLE></HEAD>\n"
				"<BODY><H1>501 Not Implemented</H1>\nPOST to non-script "
					 "is not supported in Boa.\n</BODY></HTML>\n");
	}
	req_flush(req);
}

/* R_NOT_IMP: 505 */
void send_r_bad_version(request * req)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	req->response_status = R_BAD_VERSION;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 505 HTTP Version Not Supported\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>505 HTTP Version Not Supported</TITLE></HEAD>\n"
				"<BODY><H1>505 HTTP Version Not Supported</H1>\nHTTP versions "
					 "other than 0.9 and 1.0 "
					 "are not supported in Boa.\n<p><p>Version encountered: ");
		req_write(req, req->http_version);
		req_write(req, "<p><p></BODY></HTML>\n");
	}
	req_flush(req);
}

#ifdef USE_DEONET /* sghong, 20230914 */
void send_web_redirect_msg(request * req)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	req->response_status = R_REQUEST_OK;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 200 OK\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n");
		req_write(req, "\r\n");
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><meta http-equiv='Content-Type' content='text/html' charset='utf-8' /><TITLE></TITLE></HEAD>\r\n"
					"<BODY><TABLE>\r\n");
		req_write(req, 
				"<tr><td>현재 접속한 단말은 고객님이 인터넷접속제한서비스 기능으로 인해 인터넷 사용이 불가능한 시간입니다. </td></tr>\r\n"
				 "</TABLE></BODY></HTML>\r\n");
	}
	req_flush(req);
}
#endif

//#ifdef WEB_REDIRECT_BY_MAC
/* R_REQUEST_OK: 200 */
void send_popwin_and_reload(request * req, char *url)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	req->response_status = R_REQUEST_OK;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 200 OK\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n");
		req_write(req, "\r\n");
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><TITLE>test</TITLE></HEAD>\n"
					"<BODY>\n"
					"<script type=\"text/javascript\">\n"
					"<!--\n");
		if(strlen(url)>1)
		{
			req_write(req, "window.open('");
			req_write(req, escape_string(url, e_s));
			req_write(req, "','__DUT_NOTIFY__','');\n"); //set __DUT_NOTIFY__ let open in same page
		}
		req_write(req, 
				"location.reload();\n"
				"//-->\n"
				"</script>\n"
				 "</BODY></HTML>\n");
	}
	req_flush(req);
}
//#endif //#ifdef WEB_REDIRECT_BY_MAC
#ifdef CONFIG_E8B
void freeRequestSocket(request * req)
{
	if (req->data_fd)
		close(req->data_fd);	
	if (req->fd) {
		status.connections--;
		close(req->fd);
		//shutdown(req->fd, SHUT_RDWR);
		req->fd = -1;
	}
}
#endif
#ifdef SUPPORT_ZERO_CONFIG
void send_r_zero_config(request * req, char mode, char* result, char* status, int flag)
{
	char reportStr[256]={0};

	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	//200 ok
	req->response_status = R_REQUEST_OK;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 200 OK\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
	}
	
	if(mode==0 || flag)
		snprintf(reportStr, sizeof(reportStr), "{\"Result\" : \"%s\"}", result);
	else if (mode==1)
	{
		if(!strcmp(result, "Fail"))
			snprintf(reportStr, sizeof(reportStr), "{\"Result\" : \"%s\", \"Reason\" : \"%s\"}", result, status);
		else 
			snprintf(reportStr, sizeof(reportStr), "{\"Result\" : \"%s\", \"RegStat\" : \"%s\"}", result, status);
	}
	//printf("reportStr=%s\n",reportStr);
	req_write(req, reportStr);
	req_flush(req);
	freeRequestSocket(req);
}
#endif

#ifdef RTK_BOA_PORTAL_TO_NET_LOCKED_UI
#define PORTAL_WEB_TIPS_FOR_DIFFERENT_NETWORK "本路由器设备限制在中国移动宽带网络下使用，请切换网络重试"
#define PORTAL_WEB_CONTENT "设备当前网络IP地址为"
void send_r_portal_net_locked(request * req)
{
	char wan_addr_str[40]={0};
	rtk_get_wanip_of_andlink(NULL, wan_addr_str, sizeof(wan_addr_str));

	req->response_status = R_REQUEST_OK;
	if (!req->simple) {
		req_write(req, "HTTP/1.0 200 OK\r\n");
		print_http_headers(req);
		req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
	}
	if (req->method != M_HEAD) {
		req_write(req, "<HTML><HEAD><meta charset=\"UTF-8\"><script>alert(\"");
		req_write(req, PORTAL_WEB_TIPS_FOR_DIFFERENT_NETWORK);
		req_write(req, "\")</script></HEAD>");

		req_write(req, "<BODY><H1>");
		req_write(req, PORTAL_WEB_CONTENT);
		req_write(req, wan_addr_str);
		req_write(req, "</H1></BODY></HTML>");
	}
	req_flush(req);
}
#endif

char *escape_url(char *url){
	return escape_string(url, e_s);
}

char *escape_msg(char *msg){
	return (char *)escapeHtmlstr(msg);
}

#ifdef CU_APPSCAN_RSP_DBG
void send_appscan_response(request * req)
{
	DBG(printf("%s:%d: req->remote_port=%d!\n",__func__,__LINE__,req->remote_port);)
	char reportStr[256]={0};
	int response_status = appscan_rsp_flag;
	req->response_status = response_status;

	switch(appscan_rsp_flag){
		case R_REQUEST_OK:
			send_r_request_ok(req);
			break;
		case R_MOVED_PERM:
			send_redirect_perm(req,"/admin/login.asp");
			break;
		case R_MOVED_TEMP:
			send_redirect_temp(req,"/admin/login.asp");
			break;
		case R_NOT_MODIFIED:
			send_r_not_modified(req);
			break;
		case R_BAD_REQUEST:
			send_r_bad_request(req);
			break;
		case R_UNAUTHORIZED:
			send_r_unauthorized(req,server_name);
			break;
		case R_FORBIDDEN:
			send_r_forbidden(req);
			break;
		case R_NOT_FOUND:
			send_r_not_found(req);
			break;
		case R_ENTITY_TOO_LARGE:
			send_r_entity_too_large(req);
			break;
		case R_ERROR:
			send_r_error(req);
			break;
		case R_NOT_IMP:
			send_r_not_implemented(req);
			break;
		case R_BAD_VERSION:
			send_r_bad_version(req);
			break;
		case R_CREATED:
		case R_ACCEPTED:
		case R_PROVISIONAL:
		case R_NO_CONTENT:
		case R_MULTIPLE:
		case R_PAYMENT:
		case R_METHOD_NA:
		case R_NONE_ACC:
		case R_PROXY:
		case R_REQUEST_TO:
		case R_CONFLICT:
		case R_GONE:
		case R_BAD_GATEWAY:
		case R_SERVICE_UNAV:
		case R_GATEWAY_TO:
		default:
			if (!req->simple) {
				snprintf(reportStr, sizeof(reportStr), "HTTP/1.0 %d Bad Request\r\n", response_status);
				req_write(req, reportStr);
				print_http_headers(req);
				req_write(req, "Content-Type: text/html\r\n\r\n");	/* terminate header */
			}
			if (req->method != M_HEAD) {
				snprintf(reportStr, sizeof(reportStr), "<HTML><HEAD><TITLE>%d Bad Request</TITLE></HEAD>\n"
						"<BODY><H1>%d Bad Request</H1>\nYour client has issued "
							 "a malformed or illegal request.\n</BODY></HTML>\n", response_status, response_status);
				req_write(req, reportStr);
			}
			req_flush(req);
			break;
	}
}
#endif

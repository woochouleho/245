/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Some changes Copyright (C) 1996,97 Larry Doolittle <ldoolitt@jlab.org>
 *  Some changes Copyright (C) 1996 Charles F. Randall <crandall@goldsys.com>
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

/* boa: util.c */

#include "asp_page.h"
#include "boa.h"
#include <ctype.h>
#include "syslog.h"
#ifdef SERVER_SSL
#ifdef USES_MATRIX_SSL
#include <sslSocket.h>
#else /*!USES_MATRIX_SSL*/
#include <openssl/ssl.h>
#endif /*USES_MATRIX_SSL*/
#endif

#define HEX_TO_DECIMAL(char1, char2)	\
  (((char1 >= 'A') ? (((char1 & 0xdf) - 'A') + 10) : (char1 - '0')) * 16) + \
  (((char2 >= 'A') ? (((char2 & 0xdf) - 'A') + 10) : (char2 - '0')))

#define INT_TO_HEX(x) \
  ((((x)-10)>=0)?('A'+((x)-10)):('0'+(x)))

static time_t cur_time;
const char month_tab[48] = "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec ";
const char day_tab[] = "Sun,Mon,Tue,Wed,Thu,Fri,Sat,";

#include "escape.h"

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#include <sys/wait.h>
#include "LINUX/mib.h"
#include "LINUX/utility.h"
#ifdef CONFIG_RTK_OMCI_V1
#include <module/gpon/gpon.h>
#include <omci_api.h>
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
#include "rtk/epon.h"
#if defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_epon.h>
#endif
#endif
#endif

/*
 * Name: clean_pathname
 * 
 * Description: Replaces unsafe/incorrect instances of:
 *  //[...] with /
 *  /./ with /
 *  /../ with / (technically not what we want, but browsers should deal 
 *   with this, not servers)
 *
 * Stops parsing at '?'
 */

void clean_pathname(char *pathname)
{
	char *cleanpath, c;
	int cgiarg = 0;

	cleanpath = pathname;
	while ((c = *pathname++)) {
		if (c == '/' && !cgiarg) {
			while (1) {
				if (*pathname == '/')
					pathname++;
				else if (*pathname == '.' && *(pathname + 1) == '/')
					pathname += 2;
				else if (*pathname == '.' && *(pathname + 1) == '.' &&
						 *(pathname + 2) == '/') {
					pathname += 3;
				} else
					break;
			}
			*cleanpath++ = '/';
		} else {
			*cleanpath++ = c;
			cgiarg |= (c == '?');
		}
	}

	*cleanpath = '\0';
}

/*
 * Name: strmatch
 *
 * Descripton: This function implements simple regexp. '?' is ANY character,
 *	'*' is zero or more characters. Returns 1 when strings matches.
 */
int strmatch(char *str,char *s2)
{
 while (*s2)
 {
  if (*s2=='*' )
  {
   if (s2[1]==0)
    return 1;
   /* Removes two '**' ... */
   while (s2[1]=='*')  s2++;
   while (*str)
    {
     if (strmatch(str,s2+1))
       return 1;
     str++;
    }
   /* Test if matches "" string */
   if (strmatch(str,s2+1))
     return 1;
   return 0;
  }

//  if (*str) return 1;

  if (*s2!='?')
  {
   if (*str!=*s2)
    return 0;
  } else
    if(!*str)
      return 0;

  str++;
  s2++;
 }
 if (*str)
  return 0;
 return 1;
}


/*
 * Name: modified_since
 * Description: Decides whether a file's mtime is newer than the 
 * If-Modified-Since header of a request.
 * 
 * RETURN VALUES:
 *  0: File has not been modified since specified time.
 *  1: File has been.
 */

int modified_since(time_t * mtime, char *if_modified_since)
{
	struct tm *file_gmt;
	char *ims_info;
	char monthname[MAX_HEADER_LENGTH + 1];
	int day, month, year, hour, minute, second;
	int comp;

	ims_info = if_modified_since;
	
	/* the pre-space in the third scanf skips whitespace for the string */
	//modified by ramen 
	if (sscanf(ims_info, "%*s %d %3s %d %d:%d:%d GMT",	/* RFC 1123 */
		&day, monthname, &year, &hour, &minute, &second) == 6);
	else if (sscanf(ims_info, "%d-%3s-%d %d:%d:%d GMT",  /* RFC 1036 */
		 &day, monthname, &year, &hour, &minute, &second) == 6)
		year += 1900;
	else if (sscanf(ims_info, " %3s %d %d:%d:%d %d",  /* asctime() format */
		monthname, &day, &hour, &minute, &second, &year) == 6);
	else 
		return -1;				/* error */
	
	file_gmt = gmtime(mtime);
	month = month2int(monthname);	
	/* Go through from years to seconds -- if they are ever unequal,
	   we know which one is newer and can return */

#ifdef EMBED
	if (1900 + file_gmt->tm_year != year)
		return 1;
	if (file_gmt->tm_mon != month)
		return 1;
	if (file_gmt->tm_mday != day)
		return 1;
	if (file_gmt->tm_hour != hour)
		return 1;
	if (file_gmt->tm_min != minute)
		return 1;
	if (file_gmt->tm_sec != second)
		return 1;
#else
	if ((comp = 1900 + file_gmt->tm_year - year))
		return (comp > 0);
	if ((comp = file_gmt->tm_mon - month))
		return (comp > 0);
	if ((comp = file_gmt->tm_mday - day))
		return (comp > 0);
	if ((comp = file_gmt->tm_hour - hour))
		return (comp > 0);
	if ((comp = file_gmt->tm_min - minute))
		return (comp > 0);
	if ((comp = file_gmt->tm_sec - second))
		return (comp > 0);
#endif

	return 0;					/* this person must really be into the latest/greatest */
}

/*
 * Name: month2int
 * 
 * Description: Turns a three letter month into a 0-11 int
 * 
 * Note: This function is from wn-v1.07 -- it's clever and fast
 */

int month2int(char *monthname)
{
	switch (*monthname) {
	case 'A':
		return (*++monthname == 'p' ? 3 : 7);
	case 'D':
		return (11);
	case 'F':
		return (1);
	case 'J':
		if (*++monthname == 'a')
			return (0);
		return (*++monthname == 'n' ? 5 : 6);
	case 'M':
		return (*(monthname + 2) == 'r' ? 2 : 4);
	case 'N':
		return (10);
	case 'O':
		return (9);
	case 'S':
		return (8);
	default:
		return (-1);
	}
}


/*
 * Name: to_upper
 * 
 * Description: Turns a string into all upper case (for HTTP_ header forming)
 * AND changes - into _ 
 */

char *to_upper(char *str)
{
	char *start = str;

	while (*str) {
		if (*str == '-')
			*str = '_';
		else
			*str = toupper(*str);

		str++;
	}

	return start;
}

/*
 * Name: unescape_uri
 *
 * Description: Decodes a uri, changing %xx encodings with the actual 
 * character.  The query_string should already be gone.
 * 
 * Return values:
 *  1: success
 *  0: illegal string
 */

int unescape_uri(char *uri)
{
	char c, d;
	char *uri_old;

	/* escape LF+CR in uri header, it should not be translated */
	while ((c=*uri) != '\0')
	{
		if ((c == 0xd) || (c == 0xa))
			break;
		uri++;
	}
	uri_old = uri;
	while ((c = *uri_old)) {
		if (c == '%') {
			uri_old++;
			if ((c = *uri_old++) && (d = *uri_old++))
				*uri++ = HEX_TO_DECIMAL(c, d);
			else
				return 0;		/* NULL in chars to be decoded */
		} else {
			*uri++ = c;
			uri_old++;
		}
	}

	*uri = '\0';
	return 1;
}

/*
 * Name: close_unused_fds
 *
 * Description: Closes child's unused file descriptors, in particular
 * all the active transaction channels.  Leaves std* untouched.
 * Call once for request_block, and once for request_ready.
 *
 */

void close_unused_fds(request * head)
{
	request *current;
	for (current = head; current; current = current->next) {
		close(current->fd);
	}
}

/*
 * Name: fixup_server_root
 *
 * Description: Makes sure the server root is valid.
 *
 */

void fixup_server_root()
{
	char *dirbuf;
	int dirbuf_size;

	if (!server_root) {
#ifdef SERVER_ROOT
		server_root = strdup(SERVER_ROOT);
#else
#if 0
		fputs(
				 "boa: don't know where server root is.  Please #define "
				 "SERVER_ROOT in boa.h\n"
				 "and recompile, or use the -c command line option to "
				 "specify it.\n", stderr);
#endif
		exit(1);
#endif
	}
	if (chdir(server_root) == -1) {
#if 0
		fprintf(stderr, "Could not chdir to %s: aborting\n", server_root);
#endif
		syslog(LOG_ERR, "Could not chdir to server root");
		exit(1);
	}
	if (server_root[0] == '/')
		return;

	/* if here, server_root (as specified on the command line) is
	 * a relative path name. CGI programs require SERVER_ROOT
	 * to be absolute.
	 */

	dirbuf_size = MAX_PATH_LENGTH * 2 + 1;
	if ((dirbuf = (char *) malloc(dirbuf_size)) == NULL) {
#if 0
		fprintf(stderr,
				"boa: Cannot malloc %d bytes of memory. Aborting.\n",
				dirbuf_size);
#endif
		syslog(LOG_ERR, "Cannot allocate memory");
		exit(1);
	}
	if (getcwd(dirbuf, dirbuf_size) == NULL) {
#if 0
		if (errno == ERANGE)
			
			perror("boa: getcwd() failed - unable to get working directory. "
				   "Aborting.");
		else if (errno == EACCES)
			perror("boa: getcwd() failed - No read access in current "
				   "directory. Aborting.");
		else
			perror("boa: getcwd() failed - unknown error. Aborting.");
#endif
		syslog(LOG_ERR, "getcwd() failed");
		exit(1);
	}
#if 0
	fprintf(stderr,
			"boa: Warning, the server_root directory specified"
			" on the command line,\n"
			"%s, is relative. CGI programs expect the environment\n"
			"variable SERVER_ROOT to be an absolute path.\n"
		 "Setting SERVER_ROOT to %s to conform.\n", server_root, dirbuf);
#endif
	free(server_root);
	server_root = dirbuf;
}

/* rfc822 (1123) time is exactly 29 characters long
 * "Sun, 06 Nov 1994 08:49:37 GMT"
 */

int req_write_rfc822_time(request *req, time_t s)
{	
	struct tm *t;
	char *p;
	unsigned int a;

#ifdef SUPPORT_ASP
	if (req->buffer_end + 29 > req->max_buffer_size)
#else
	if (req->buffer_end + 29 > BUFFER_SIZE)
#endif
		return 0;	
	
	if (!s) {
		time(&cur_time);
		t = gmtime(&cur_time);
	} else
		t = gmtime(&s);
	
	p = req->buffer + req->buffer_end + 28;
	/* p points to the last char in the buf */
	
	p -= 3; /* p points to where the ' ' will go */
	memcpy(p--, " GMT", 4);
	
	a = t->tm_sec;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = t->tm_min;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = t->tm_hour;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ' ';
	a = 1900 + t->tm_year;
	while (a) {
		*p-- = '0' + a % 10;
		a /= 10;
	}
	/* p points to an unused spot to where the space will go */
	p -= 3;
	/* p points to where the first char of the month will go */
	memcpy(p--, month_tab + 4 * (t->tm_mon), 4);
	*p-- = ' ';
	a = t->tm_mday;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ' ';
	p -= 3;
	memcpy(p, day_tab + t->tm_wday * 4, 4);
	
	req->buffer_end += 29;	
	return 29;
}



/*
 * Name: get_commonlog_time
 *
 * Description: Returns the current time in common log format in a static
 * char buffer.
 *
 * commonlog time is exactly 24 characters long
 * because this is only used in logging, we add "[" before and "] " after
 * making 27 characters
 * "27/Feb/1998:20:20:04 GMT"
 */

char *get_commonlog_time(void)
{
#ifdef BOA_TIME_LOG
	struct tm *t;
	char *p;
	unsigned int a;
	static char buf[27];
	
	time(&cur_time);
	t = gmtime(&cur_time);
	
	p = buf + 27 - 1 - 5;
	
	memcpy(p--, " GMT] ", 6);
	
	a = t->tm_sec;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = t->tm_min;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = t->tm_hour;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = 1900 + t->tm_year;
	while (a) {
		*p-- = '0' + a % 10;
		a /= 10;
	}
	/* p points to an unused spot */
	*p-- = '/';	
	p -= 2;
	memcpy(p--, month_tab + 4 * (t->tm_mon), 3);
	*p-- = '/';
	a = t->tm_mday;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p = '[';
	return p; /* should be same as returning buf */
#else
	return NULL;
#endif
}

char *simple_itoa(int i)
{
	/* 21 digits plus null terminator, good for 64-bit or smaller ints */
	/* for bigger ints, use a bigger buffer! */
	static char local[22];
	char *p = &local[21];
	*p-- = '\0';
	do {
		*p-- = '0' + i % 10;
		i /= 10;
	} while (i > 0);
	return p + 1;
}

/*
 * Name: escape_string
 * 
 * Description: escapes the string inp.  Uses variable buf.  If buf is
 *  NULL when the program starts, it will attempt to dynamically allocate
 *  the space that it needs, otherwise it will assume that the user 
 *  has already allocated enough space for the variable buf, which
 *  could be up to 3 times the size of inp.  If the routine dynamically
 *  allocates the space, the user is responsible for freeing it afterwords
 * Returns: NULL on error, pointer to string otherwise.
 */

char *escape_string(char *inp, char *buf)
{
	int max;
	char *index;
	unsigned char c;

	max = strlen(inp) * 3;

	if (buf == NULL && max)
		buf = malloc(sizeof(char) * max + 1);

	if (buf == NULL)
		return NULL;

	index = buf;
	while ((c = *inp++) && max > 0) {
		if (needs_escape((unsigned int) c)) {
			*index++ = '%';
			*index++ = INT_TO_HEX(c >> 4);
			*index++ = INT_TO_HEX(c & 15);
		} else
			*index++ = c;
	}
	*index = '\0';
	return buf;
}

/*
 * Name: req_write
 * 
 * Description: Buffers data before sending to client.
 */

int req_write(request *req, char *msg)
		
{
	int msg_len;
	
	msg_len = strlen(msg);
	if (!msg_len)
		return 1;

#ifdef SUPPORT_ASP
	if (req->buffer_end + msg_len > req->max_buffer_size) {
#else
	if (req->buffer_end + msg_len > BUFFER_SIZE) {
#endif
		log_error_time();
#if 0
		fprintf(stderr, "Ran out of Buffer space!\n");
#endif
		syslog(LOG_ERR, "Out of buffer space");
		return 0;
	}
	
	memcpy(req->buffer + req->buffer_end, msg, msg_len);
	req->buffer_end += msg_len;
	return 1;
}

/*
 * Name: flush_req
 * 
 * Description: Sends any backlogged buffer to client.
 */

// Kaohj
int waitsec=0;
time_t lasttime;
extern time_t time_counter;
int req_flush(request * req)
{
	int bytes_to_write;

#ifdef CONFIG_USER_BOA_CSRF
	if(req->is_cgi)
		handleCSRFToken(req);
#endif
	bytes_to_write = req->buffer_end - req->buffer_start;
	if (bytes_to_write) {
		int bytes_written;

#ifdef USE_NLS
	if (req->cp_table)
	{
		nls_convert(req->buffer + req->buffer_conv,
				req->cp_table,
				req->buffer_end-req->buffer_conv);
		req->buffer_conv = req->buffer_end;
	}
#endif
#ifdef SERVER_SSL
	if(req->ssl == NULL){
#endif /*SERVER_SSL*/
		// Kaohj -- applying timeout for socket write
		if (waitsec == 0) {
			waitsec = 10; // allow write() trying for waitsec
			lasttime = time_counter;
		}
		bytes_written = write(req->fd, 
				req->buffer + req->buffer_start,
				bytes_to_write);
		// Kaohj -- still trying before timeout
		if (bytes_written == -1) {
			waitsec -= (time_counter - lasttime);
			lasttime = time_counter;
			if (waitsec == 0) {
				//printf("time is out\n");
				req->buffer_start = req->buffer_end = 0;
				return 0;
			}
		}
		else
			waitsec = 0;
#ifdef SERVER_SSL
	}else{
#ifdef USES_MATRIX_SSL
		int mtrx_status;
		if (bytes_to_write  >SSL_MAX_PLAINTEXT_LEN)
			bytes_to_write = SSL_MAX_PLAINTEXT_LEN;
		bytes_written = sslWrite(req->ssl, req->buffer + req->buffer_start, bytes_to_write,&mtrx_status);
#else /* ! USES_MATRIX_SSL*/
		bytes_written = SSL_write(req->ssl, req->buffer + req->buffer_start, bytes_to_write);
#endif /* USES_MATRIX_SSL*/
	}
#endif /*SERVER_SSL*/
		
		if (bytes_written == -1) {
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				return -1;			/* request blocked at the pipe level, but keep going */
			else {
				req->buffer_start = req->buffer_end = 0;
#ifdef USE_NLS
				req->buffer_conv = 0;
#endif
#if 0
				if (errno != EPIPE)
					perror("buffer flush");	/* OK to disable if your logs get too big */
#endif
				return 0;
			}
		}
		req->buffer_start += bytes_written;
    }
	if (req->buffer_start == req->buffer_end)
	{
		req->buffer_start = req->buffer_end = 0;
#ifdef USE_NLS
		req->buffer_conv = 0;
#endif
	}
	return 1; /* successful */
}

/*
 * Name: base64decode
 *
 * Description: Decodes BASE-64 encoded string
 */
int base64decode(void *dst,char *src,int maxlen)
{
 int bitval,bits;
 int val;
 int len,x,y;

 len = strlen(src);
 bitval=0;
 bits=0;
 y=0;

 for(x=0;x<len;x++)
  {
   if ((src[x]>='A')&&(src[x]<='Z')) val=src[x]-'A'; else
   if ((src[x]>='a')&&(src[x]<='z')) val=src[x]-'a'+26; else
   if ((src[x]>='0')&&(src[x]<='9')) val=src[x]-'0'+52; else
   if (src[x]=='+') val=62; else
   if (src[x]=='-') val=63; else
    val=-1;
   if (val>=0)
    {
     bitval=bitval<<6;
     bitval+=val;
     bits+=6;
     while (bits>=8)
      {
       if (y<maxlen)
        ((char *)dst)[y++]=(bitval>>(bits-8))&0xFF;
       bits-=8;
       bitval &= (1<<bits)-1;
      }
    }
  }
 if (y<maxlen)
   ((char *)dst)[y++]=0;
 return y;
}

static char base64chars[64] = "abcdefghijklmnopqrstuvwxyz"
                              "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";

/*
 * Name: base64encode()
 *
 * Description: Encodes a buffer using BASE64.
 */
void base64encode(unsigned char *from, char *to, int len)
{
  while (len) {
    unsigned long k;
    int c;

    c = (len < 3) ? len : 3;
    k = 0;
    len -= c;
    while (c--)
      k = (k << 8) | *from++;
    *to++ = base64chars[ (k >> 18) & 0x3f ];
    *to++ = base64chars[ (k >> 12) & 0x3f ];
    *to++ = base64chars[ (k >> 6) & 0x3f ];
    *to++ = base64chars[ k & 0x3f ];
  }
  *to++ = 0;
}


//#ifdef CHECK_IP_MAC /*This stuff is hacked into place for Dan!!*/
#if defined(CHECK_IP_MAC) || defined(WEB_REDIRECT_BY_MAC)

#define ARP "/proc/net/arp"

int get_mac_from_IP(char *MAC, char *remote_IP){
	FILE *fd;
	char tmp[81];
	int len = 81;
	int i;
	char *str;
	int amtRead;
	char * trim_remote_IP;

#ifdef IPV6
	// The address notation for IPv4-mapped-on-IPv6 is ::FFFF:<IPv4 address>.
	//  So we shift 7 bytes (::FFFF:).
	trim_remote_IP = remote_IP+7;
#else
	trim_remote_IP = remote_IP;
#endif	
	if((fd = fopen(ARP, "r")) == 0){/*should be able to open the ARP cache*/
		return 0;
	}
	while(fgets(tmp, len, fd)){
		/*MN - should do some more checks!!*/
		str = strtok(tmp, " \t");
		if(strcmp(str, trim_remote_IP) == 0){ /*found IP address*/
			for(i=0;i<3;i++){  /*Move along until the HW address*/
				str = strtok(NULL, " \t");
			}
			strcpy(MAC, str);
			/*MATT2 you could obtain the ethernet device in here if you wanted*/
			fclose(fd);
			return 1;
		}
	}
	fclose(fd);
	return 0;
}

int do_mac_crap(char *IP, char *MAC)
{

}

#endif /*CHECK_IP_MAC*/

#ifdef SUPPORT_ZERO_CONFIG
static time_t first_reg_olt_time=0, now, after;
static time_t waitRMStimes_old=0, waitRMStimes_new=0;
static time_t first_get_ip_time=0, get_ip_now;
static int set_olt_done=0;
static int reg_times=0;
void start_app_passwd_register(char * app_password)
{
	char loid[MAX_NAME_LEN]={0}, *password, *s;
	unsigned char vChar;
	unsigned char enable4StageDiag;
	unsigned int regLimit;
	unsigned int regTimes;
	unsigned int lineno;
	pid_t cwmp_pid;
	int num_done;
#if defined(CONFIG_GPON_FEATURE)
	int i=0;
#endif
#if defined(CONFIG_EPON_FEATURE)
	int index = 0;
	char cmdBuf[64] = {0};
#endif
	int sleep_time = 3;
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	unsigned int pon_mode;
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
#endif
	unsigned char reg_type;
	unsigned char gui_passauth_enable = 1;
#if defined(CONFIG_GPON_FEATURE)
	unsigned int gpon_speed=0;
	int ploam_pw_length=GPON_PLOAM_PASSWORD_LENGTH;
								
	mib_get_s(MIB_PON_SPEED, (void *)&gpon_speed, sizeof(gpon_speed));
	if(gpon_speed==0){
		ploam_pw_length=GPON_PLOAM_PASSWORD_LENGTH;
	}
	else{
		ploam_pw_length=NGPON_PLOAM_PASSWORD_LENGTH;
	}
#endif

	now = time(NULL);
	first_reg_olt_time=0;
	waitRMStimes_old=0;
	first_get_ip_time = 0;
	reg_times++;
	if(first_reg_olt_time==0)
		first_reg_olt_time = now;

	mib_get_s(PROVINCE_DEV_REG_TYPE, &reg_type, sizeof(reg_type));
#ifdef _PRMT_X_CT_COM_USERINFO_
	mib_get_s(CWMP_USERINFO_LIMIT, &regLimit, sizeof(regLimit));
	mib_get_s(CWMP_USERINFO_TIMES, &regTimes, sizeof(regTimes));
	if (regTimes >= regLimit) {
		vChar = CWMP_REG_IDLE;
		mib_set(CWMP_REG_INFORM_STATUS, &vChar);
		goto FINISH;
	}
#endif
	mib_get_s(MIB_LOID, (void *)loid, sizeof(loid));
	printf("loid is %s\n", loid);

#if (defined(CONFIG_CMCC) || defined(CONFIG_CU)) && defined(CONFIG_E8B) && defined(CONFIG_GPON_FEATURE)
	if(getWebLoidPageEnable()==1)
	{
		memset(loid, 0, sizeof(loid));
		strncpy(loid, app_password, MAX_NAME_LEN - 1);
		printf("ZeroConfig new loid is %s\n", loid);
	}
#endif

	if (loid[0]) {
		mib_set(MIB_LOID, loid);
		if(reg_type != DEV_REG_TYPE_DEFAULT)
			mib_set(MIB_LOID_OLD,loid);
#ifdef CONFIG_USER_RTK_ONUCOMM
		//init_onucomm_sock();
		onucomm_pon_loid(loid);
		//close_onucomm_sock();
#endif
	}
	else{
		mib_set(MIB_LOID, loid);
		if(reg_type != DEV_REG_TYPE_DEFAULT)
			mib_set(MIB_LOID_OLD,loid);
	}

	password = app_password;
	printf("password is %s\n", password);
	mib_set(MIB_LOID_PASSWD, password);
	if(reg_type != DEV_REG_TYPE_DEFAULT)
		mib_set(MIB_LOID_PASSWD_OLD,password);
#ifdef _CWMP_MIB_
	mib_set(CWMP_GUI_PASSWORD_ENABLE, &gui_passauth_enable);
#endif
	/*xl_yue:20081225 record the inform status to avoid acs responses twice for only once informing*/
#ifdef _PRMT_X_CT_COM_USERINFO_
	vChar = CWMP_REG_REQUESTED;
	mib_set(CWMP_REG_INFORM_STATUS, &vChar);
#endif
	/* reset to zero */
	num_done = 0;
#ifdef E8B_NEW_DIAGNOSE
	mib_set(CWMP_USERINFO_SERV_NUM_DONE, &num_done);
	mib_set(CWMP_USERINFO_SERV_NAME_DONE, "");
#endif
	/*xl_yue:20081225 END*/

#if defined(CONFIG_GPON_FEATURE)
	if(pon_mode == 1)
	{
		// Deactive GPON
		// do not use rtk_rg_gpon_deActivate() becuase it does not send link down event.

#ifndef CONFIG_10G_GPON_FEATURE
		system("diag gpon reg-set page 1 offset 0x10 value 0x1");
#endif
		system("omcicli mib reset &");

		if(loid[0])
		{
			if (password[0]) {
				va_cmd("/bin/omcicli", 4, 1, "set", "loid", loid, password);
			} else {
				va_cmd("/bin/omcicli", 3, 1, "set", "loid", loid);
			}
		}

		//because we deactivate the gpon
		//we need CONFIG_CMCC_OSGIMANAGE=y
		//system("/bin/diag gpon deactivate");
		rtk_wrapper_gpon_deactivate();
		rtk_wrapper_gpon_password_set(password);
		rtk_wrapper_gpon_activate();

#ifndef CONFIG_10G_GPON_FEATURE
		while(i++ < 10)
		system("diag gpon reg-set page 1 offset 0x10 value 0x3");
#endif
	}
#endif
#if defined(CONFIG_EPON_FEATURE)
	if(pon_mode == 2)
	{
		/* Martin ZHU add: release all wan connection IP */
		va_cmd("/bin/ethctl", 2, 1, "enable_nas0_wan", "0");

		va_cmd("/bin/ethctl", 2, 1, "enable_nas0_wan", "1");

		/* Martin ZHU: 2016-3-24  */
		mib_get_s(PROVINCE_PONREG_4STAGEDIAG, (void *) &enable4StageDiag, sizeof(enable4StageDiag));
		index = 0;
		{
			if(enable4StageDiag)
			{
				system("diag epon reset mib-counter");
			}

			memset(cmdBuf, 0, sizeof(cmdBuf));

			if (password[0]) {
				sprintf(cmdBuf, "/bin/oamcli set ctc loid %d %s %s\n",index, loid, password);
			} else {
				sprintf(cmdBuf, "/bin/oamcli set ctc loid %d %s\n",index, loid);
			}
			system(cmdBuf);

			/* 2016-04-29 siyuan: oam needs to register again using new loid and password */
			sprintf(cmdBuf,"/bin/oamcli trigger register %d", index);
			system(cmdBuf);
		}
	}
#endif

#ifdef _CWMP_MIB_ 

	if(reg_type == DEV_REG_TYPE_JSU)
	{
		int result = NOW_SETTING;
		int status = 99;
		mib_set(CWMP_USERINFO_RESULT, &result);
		mib_set(CWMP_USERINFO_STATUS, &status);
		unlink("/var/inform_status");
	}

	{
		pid_t tr069_pid;

		// send signal to tr069
		tr069_pid = read_pid("/var/run/cwmp.pid");
		if ( tr069_pid > 0)
			kill(tr069_pid, SIGUSR2);
	}
#endif
	// Purposes:
	// 1. Wait for PON driver ready.
	// 2. Wait for old IP release.
	while(sleep_time)
		sleep_time = sleep(sleep_time);

	/*Martin ZHU 2016-03-22  END*/
#ifdef CONFIG_USER_RTK_ONUCOMM
	g_check_eth_link = 1;
#endif // CONFIG_USER_RTK_ONUCOMM

FINISH:
#ifdef COMMIT_IMMEDIATELY
	SaveLOIDReg(); //save LOID and PASSWORD directly, SaveLOIDReg() already has Commit() function
#endif
}

int get_register_result_status(char * status)
{
	static int passRegFailTimes = 0;  //for pass reg
	static long last_time = 0;
	const int waitRMStime = 600;
	int i, total, ret, valid_ip=0;
	MIB_CE_ATM_VC_T entry;
	struct in_addr inAddr;
	char buf[256];
	char getip[32];
	unsigned int regStatus;
	unsigned int regLimit;
	unsigned char regInformStatus;
	unsigned int regResult;
#ifdef _CWMP_MIB_
	unsigned char cwmp_status = CWMP_STATUS_NOT_CONNECTED;
	unsigned char reg_type = DEV_REG_TYPE_JSU;
#endif
#ifdef CONFIG_RTK_OMCI_V1
	PON_OMCI_CMD_T msg;
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	unsigned int pon_mode=0, pon_state=0;
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
#endif

#ifdef _PRMT_X_CT_COM_USERINFO_
	mib_get_s(PROVINCE_DEV_REG_TYPE, &reg_type, sizeof(reg_type));
	mib_get_s(CWMP_USERINFO_STATUS, &regStatus, sizeof(regStatus));
	mib_get_s(CWMP_USERINFO_LIMIT, &regLimit, sizeof(regLimit));
	mib_get_s(CWMP_REG_INFORM_STATUS, &regInformStatus, sizeof(regInformStatus));
	mib_get_s(CWMP_USERINFO_RESULT, &regResult, sizeof(regResult));
	mib_get_s(RS_CWMP_STATUS, &cwmp_status, sizeof(cwmp_status));
#endif

	//plug-in/out
	if(passRegFailTimes>2 && getSYSInfoTimer()-last_time<=180 && set_olt_done==1)
	{
		first_reg_olt_time = time(NULL);
#if defined(CONFIG_E8B) && defined(CONFIG_PON_CONFIGURATION_COMPLETE_EVENT)
		pon_state = get_omci_complete_event_shm();
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
		if (pon_state!=5) {
			set_olt_done=0;
			strcpy(status, "Registing Olt");
		}
#endif
	}

	if(getSYSInfoTimer()-last_time>180)
		passRegFailTimes = 0;
#ifdef _PRMT_X_CT_COM_USERINFO_
	if (regInformStatus != CWMP_REG_RESPONSED) {	//ACS not returned result
		total = mib_chain_total(MIB_ATM_VC_TBL);

		for (i = 0; i < total; i++) {
			if (mib_chain_get(MIB_ATM_VC_TBL, i, &entry) == 0)
				continue;

			if ((entry.applicationtype & X_CT_SRV_TR069) &&
					ifGetName(entry.ifIndex, buf, sizeof(buf)) &&
					getInFlags(buf, &ret) &&
					(ret & IFF_UP) &&
					getInAddr(buf, IP_ADDR, &inAddr))
				break;
		}
		getip[sizeof(getip)-1]='\0';
		strncpy(getip, inet_ntoa(inAddr),sizeof(getip)-1);
		valid_ip=isValidIpAddr(getip);

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
		if(pon_mode == 1)
		{
#if defined(CONFIG_E8B) && defined(CONFIG_PON_CONFIGURATION_COMPLETE_EVENT)
			pon_state = get_omci_complete_event_shm();
#endif
#if (defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)) && defined(CONFIG_E8B) && defined(CONFIG_GPON_FEATURE)
			if(getWebLoidPageEnable()==1){
#ifdef CONFIG_RTK_OMCI_V1
				/* During deactivate, the IP may not be cleared in a small period of time.*/
				/* So check gpon state first. */
				memset(&msg, 0, sizeof(msg));
				msg.cmd = PON_OMCI_CMD_LOIDAUTH_GET_RSP;
				ret = omci_SendCmdAndGet(&msg);

				if (ret != GOS_OK || (msg.state != 0 && msg.state != 1)) {
					strcpy(status, "Olt Failed");
					return 0;
				}
				pon_state = msg.state;
				printf("%s:%d pon_state is %d\n", __FUNCTION__, __LINE__, pon_state);
#endif
			}
			else{
				if(getGponONUState()==GPON_STATE_O5){
					pon_state = 1;
					printf("%s:%d pon_state is O5\n", __FUNCTION__, __LINE__);
				}
				else{
					pon_state = 0;
					printf("%s:%d pon_state is %d\n", __FUNCTION__, __LINE__, getGponONUState());
				}
			}
#endif
		}
		else if(pon_mode == 2)
		{
			int eonu = 0;
			
			eonu = getEponONUState(0);
			pon_state = (eonu==5? 1:0);//OLT Register successful
		}
#endif
		after = time(NULL);

		if ((after-first_reg_olt_time) >= 180)
		{
			/* 180 seconds, timeout */
			if (i == total
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
            && pon_state!=1
#endif
            ) {
				/* The interface for TR069 is not ready */
				passRegFailTimes++;
				last_time = getSYSInfoTimer(); 
				strcpy(status, "Olt Timeout");
				return 0;	
			} else {
				if(valid_ip==0)
				{
					get_ip_now = time(NULL);
					if(first_get_ip_time==0)
						first_get_ip_time = get_ip_now;
					strcpy(status, "Getting WAN Address");

					if((time(NULL)-first_get_ip_time)>120)
						strcpy(status, "Getting Address Failed");
				}
				else
				{
					first_get_ip_time = 0;
					strcpy(status, "Registing RMS");
				}
				first_reg_olt_time=0;
				set_olt_done=1;
			}
		} else {
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
			if (pon_state == 0) {
				set_olt_done=0;
				strcpy(status, "Registing Olt");
				return 0;
			}
#endif
			if (i == total) {
				/* The interface for TR069 is not ready */
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
				if(pon_state == 1)
				{
					if(valid_ip==0)
					{
						get_ip_now = time(NULL);
						if(first_get_ip_time==0)
							first_get_ip_time = get_ip_now;
						strcpy(status, "Getting WAN Address");

						if((time(NULL)-first_get_ip_time)>120)
							strcpy(status, "Getting Address Failed");
					}
					else
					{
						first_get_ip_time = 0;
						strcpy(status, "Registing RMS");
					}
					first_reg_olt_time=0;
					set_olt_done=1;
					SaveLOIDReg();
				}
#else
				strcpy(status, "Registing RMS");
#endif
			} else {
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
				if(pon_state == 1)
				{
					if(valid_ip==0)
					{
						get_ip_now = time(NULL);
						if(first_get_ip_time==0)
							first_get_ip_time = get_ip_now;
						strcpy(status, "Getting WAN Address");

						if((time(NULL)-first_get_ip_time)>120)
							strcpy(status, "Getting Address Failed");
					}
					else
					{
						first_get_ip_time = 0;
						strcpy(status, "Registing RMS");
					}
					first_reg_olt_time=0;
					set_olt_done=1;
					SaveLOIDReg();
				}
#endif
				SaveLOIDReg();
			}
		}
	} else {   
		first_reg_olt_time=0;
		if (regStatus == 0) {
REG_RESULT_RECHECK:
			switch (regResult) {
				case NO_SET:
					strcpy(status, "Waitting Service");
					break;
				case NOW_SETTING:
					if(waitRMStimes_old==0)
					{
						waitRMStimes_old=time(NULL);
						waitRMStimes_new=0;
					}
					else 
						waitRMStimes_new=time(NULL);
					if(waitRMStimes_new-waitRMStimes_old > waitRMStime)
					{
						strcpy(status, "Service Timeout");
					}else
						strcpy(status, "Service Being Delivering");
					break;
				case SET_SUCCESS:
					passRegFailTimes=0;
					waitRMStimes_old=0;
					if(cwmp_status == CWMP_STATUS_CONNETED)
					{
						regResult = NOW_SETTING;
						goto REG_RESULT_RECHECK;
					}
					strcpy(status, "Reg And Service Succ");
					break;
				case SET_FAULT:
					strcpy(status, "Service Failed");
					break;
			}
		} else if (regStatus >= 1 && regStatus <= 3 ) {
			strcpy(status, "RMS Failed");
		} else if (regStatus == 4) {
			strcpy(status, "RMS Timeout");
		} else if (regStatus == 5) {
			strcpy(status, "No Need Update Service");
		} else{
			strcpy(status, "RMS Failed");
		}
	}
#endif

	return 0;
}

int app_password_reg(char *result, char *status, char* username, char* passwd, char* password, char* sn, char* action, int flag)
{
	int bytes_sent = 0;
	char mib_user[MAX_NAME_LEN];
	char mib_pass[MAX_PASSWD_LEN];
#ifdef CONFIG_GPON_FEATURE
	char mib_sn[13];
#endif
	unsigned int regStatus;
	unsigned int regLimit;
	unsigned int regTimes;
	unsigned int regResult;
	unsigned char regInformStatus;

	mib_get(MIB_SUSER_NAME, mib_user);
	mib_get(MIB_SUSER_PASSWORD, mib_pass);
#ifdef CONFIG_GPON_FEATURE
	mib_get(MIB_GPON_SN, mib_sn);
#endif	

	if(flag)
	{
		if(!strcmp(action, "GetRegStat"))
		{
			printf("user name and password\n");
			mib_get(MIB_USER_NAME, mib_user);
			mib_get(MIB_USER_PASSWORD, mib_pass);
		}
	}
	//printf("[%s %d]username=%s,pass=%s,password=%s,sn=%s,action=%s,flag=%d\n",__func__, __LINE__,username,passwd,password,sn,action,flag);
	if(strcmp(username, mib_user) || strcmp(passwd, mib_pass))
	{
		if(!strcmp(action, "GetRegStat"))
		{
			strcpy(status,"Auth Failed");		
			strcpy(result,"Fail");
		}
		else
		{
			if(!flag)
				strcpy(result,"Auth Failed");
			else
				strcpy(result,"Adminauth Failed");
		}	
	}
#ifdef _PRMT_X_CT_COM_USERINFO_
	else if(!strcmp(action, "GetRegStat"))
	{
		mib_get(CWMP_USERINFO_LIMIT, &regLimit);
		mib_get(CWMP_USERINFO_TIMES, &regTimes);
		strcpy(result,"Succ");
		if (regTimes >= regLimit)
		{
			strcpy(status, "Over Limit");
			goto REGDONE;
		}
		if(reg_times==0)
			strcpy(status, "Ready");
		get_register_result_status(status);
	}
#endif
#ifdef CONFIG_GPON_FEATURE
	else if(flag && strcmp(sn, mib_sn))
	{
		strcpy(result,"SN Error");
	}
#endif
	else
	{
#ifdef _PRMT_X_CT_COM_USERINFO_
		start_app_passwd_register(password);
		fprintf(stderr, "App start register device... \n");
		strcpy(result,"Succ");
		mib_get(CWMP_USERINFO_STATUS, &regStatus);
		mib_get(CWMP_USERINFO_LIMIT, &regLimit);
		mib_get(CWMP_USERINFO_TIMES, &regTimes);
		mib_get(CWMP_USERINFO_RESULT, &regResult);
		mib_get(CWMP_REG_INFORM_STATUS, &regInformStatus);
		if (regTimes >= regLimit)
		{
			strcpy(result,"Over Limit");
			goto REGDONE;
		}

		if (regInformStatus != CWMP_REG_RESPONSED) {
			strcpy(status,"Register is running");
			//start_app_passwd_register(password);
		}
		else
		{
			if (regStatus == 0)
			{
				strcpy(status,"No Need Reg Again");
				goto REGDONE;
			}
			else if (regStatus == 2 && regResult == 99 || regStatus == 1)
			{
				strcpy(status,"Loid Param Wrong");
				goto REGDONE;				
			}
			else if (regStatus == 5)
			{
				strcpy(status,"No Need Update Service");
				goto REGDONE;
			}
			else
				strcpy(status,"Register is running");

			//start_app_passwd_register(password);		
		}
#endif
	}
REGDONE:
	if(flag && status != NULL && strlen(status)>1)
	{
		strcpy(result,status);
		status[0] = '\0';
	}
	return 0;
}
#endif //SUPPORT_ZERO_CONFIG

static int calcHtmlStrSize(char *str)
{
	char *ptr = str;
	int i = 0, len = 0;
	
	if (!str){
		printf("[%s:%d] ERROR: str=NULL!!!\n",__func__,__LINE__);
		return 0;
	}

	while(*ptr != '\0'){
		for (i = 0; htmlTagCharMapping[i].tagChar != 0; i++){
			if (*ptr == htmlTagCharMapping[i].tagChar){
				len += strlen(htmlTagCharMapping[i].mapStr);
				break;
			}
		}
		
		if (htmlTagCharMapping[i].tagChar != 0){
			ptr++;
			continue;
		}else{
			len++;
			ptr++;
		}
		
	}

	return len;
}

char *escapeHtmlstr(char *str)
{
	char *ptr = NULL, *data = NULL;
	int data_len = 0, i = 0;
	if (!str){
		printf("[%s:%d] ERROR: str=NULL!!!\n",__func__,__LINE__);
		return NULL;
	}

	data_len = calcHtmlStrSize(str)+1;
	if (data_len > 0){
		data = (char *)malloc(data_len);
		if (data == NULL){
			printf("[%s:%d] ERROR: malloc(%d) fail!!!\n",__func__,__LINE__,data_len);
			return NULL;
		}
		memset(data, 0x00, data_len);

		ptr = str;
		while(*ptr != '\0'){
			for (i = 0; htmlTagCharMapping[i].tagChar != 0; i++){
				if (*ptr == htmlTagCharMapping[i].tagChar){
					if (data_len-strlen(data) > strlen(htmlTagCharMapping[i].mapStr))
						strcat(data+strlen(data), htmlTagCharMapping[i].mapStr);
					break;
				}
			}

			if (htmlTagCharMapping[i].tagChar != 0){
				ptr++;
				continue;
			}else {
				data[strlen(data)] = *ptr;
				ptr++;
			}
			
		}

		data[data_len-1] = '\0';

		return data;
	}

	return NULL;
}


int strContainXSSChar(char *str)
{
	char *ptr = str;
	int i = 0;
	if (str == NULL)
		return 0;

	while (*ptr != '\0'){
		for (i = 0; htmlTagCharMapping[i].tagChar != 0; i++){
			if (*ptr == htmlTagCharMapping[i].tagChar)
				return 1;
		}

		ptr++;
	}

	return 0;
}

char *strValToASP(char *str)
{
	char *dst = NULL;
	if (strContainXSSChar(str) == 1){
		dst = escapeHtmlstr(str);
		if ( dst != NULL){
			if (addASPTempStr(dst) == FAILED){
				printf("[%s:%d] ERROR: addASPTempStr fail, may get the risk of XSS attack!!!\n",__func__,__LINE__);
				free(dst);
			}else{
				return dst;
			}		
		}
	}
	return str;
}

char *memstr(char *haystack, char *needle, int size)
{
	char *p;
	char needlesize = strlen(needle);

	for (p = haystack; p <= (haystack-needlesize+size); p++)
	{
		if (memcmp(p, needle, needlesize) == 0)
			return p; /* found */
	}
	return NULL;
}

char *memcasestr(char *haystack, char *needle, int size)
{
	int i;
	char *p;
	char needlesize = strlen(needle);

	for (p = haystack; p <= (haystack-needlesize+size); p++)
	{
		for(i=0; i<needlesize; i++)
			if(tolower(*(p+i)) != tolower(*(needle+i)))
				break;
		if(i == needlesize)
			return p;
	}
	return NULL;
}
/* vi: set sw=4 ts=4: */
/*
 * wget - retrieve a file using HTTP or FTP
 *
 * Chip Rosenthal Covad Communications <chip@laserlink.net>
 *
 */

/* We want libc to give us xxx64 functions also */
/* http://www.unix.org/version2/whatsnew/lfs20mar.html */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <alloca.h>




#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>


#include <arpa/inet.h>
#include <netdb.h>

#include <sys/socket.h>

#define  RET_ERROR 0
#define  RET_SUCCESS 1
#if 0
#define __DBG_IMGUP__
#define DBG_ARG(arg...)\
    {\
			FILE *f; \
			if( ( f = fopen( "/dev/console", "w" ) ) != NULL ) { \
				fprintf( f, arg);\
				fclose( f );\
			} \
	}
#else
#define DBG_ARG(arg...) printf(arg)
#endif
//#define BUFSIZE     14*1024*1024
#define BUFSIZE     1024*1024

#define	USER_AGENT_WGET		"Wget"

/* Some useful definitions */
#define FALSE   ((int) 0)
#define TRUE    ((int) 1)
#define SKIP	((int) 2)

typedef struct len_and_sockaddr {
	int len;
	union {
		struct sockaddr sa;
		struct sockaddr_in sin;
	};  
} len_and_sockaddr;


struct host_info {
	// May be used if we ever will want to free() all xstrdup()s...
	/* char *allocated; */
	char *host;
	int port;
	char *path;
	int is_ftp;
	char *user;
};

static int parse_url(const char *url, struct host_info *h);
static FILE *open_socket(len_and_sockaddr *lsa);
static char *gethdr(char *buf, size_t bufsiz, FILE *fp, int *istrunc);

/* Globals (can be accessed from signal handlers */
static off_t content_len;        /* Content-length of the file */
static int chunked;                     /* chunked transfer encoding */



#if 0	//psy-2011.0712
static ssize_t safe_write(int fd, const void *buf, size_t count)
{
	ssize_t		n;

	do {
		n = write(fd, buf, count);
	} while (n < 0 && errno == EINTR);

	return n;
}
#endif





const char *memory_exhausted = "Memory exhausted";
const char *applet_name = "debug stuff usage";

#if 0
extern void verror_msg(const char *s, va_list p)
{
	fflush(stdout);
	fprintf(stderr, "%s: ", applet_name);
	vfprintf(stderr, s, p);
}

void error_msg_and_die(const char *s, ...)
{
	va_list p;

	va_start(p, s);
	verror_msg(s, p);
	va_end(p);
	putc('\n', stderr);
	exit(EXIT_FAILURE);
}

static void close_and_delete_outfile(FILE* output, char *fname_out, int do_continue)
{
	if (output != stdout && do_continue==0) {
		fclose(output);
		unlink(fname_out);
	}
}

#define close_delete_and_die(s...) { \
	close_and_delete_outfile(output, fname_out, do_continue); \
	error_msg_and_die(s); }


char * xstrdup (const char *s) {
	char *t;

	if (s == NULL)
		return NULL;

	t = strdup (s);

	if (t == NULL)
		error_msg_and_die(memory_exhausted);

	return t;
}
#endif

/* Globals (can be accessed from signal handlers */
static off_t filesize = 0;		/* content-length of the file */

static size_t safe_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;

	do {
		clearerr(stream);
		ret += fread((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}

static char *safe_fgets(char *s, int size, FILE *stream)
{
	char *ret;

	do {
		clearerr(stream);
		ret = fgets(s, size, stream);
	} while (ret == NULL && ferror(stream) && errno == EINTR);

	return ret;
}

#if 0
static size_t safe_fwrite(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;

	do {
		clearerr(stream);
		ret += fwrite((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}
#endif

static char *skip_whitespace(const char *s)
{
	while (isspace(*s)) ++s;
	return (char *)s;
}

#if 0	//psy-2011.0712
static int concat_path_file(char *fullpath, const char *path, const char *filename)
{
	int			lc = 0;

	if (path == NULL)
		path = "";
	lc = path[strlen(path) - 1];
	sprintf(fullpath, "%s%s%s", path, (lc == '/' ? "" : "/"), filename);
	return 0;
}
#endif

void set_nport(len_and_sockaddr *lsa, unsigned port)
{       
	if (lsa->sa.sa_family == AF_INET) {
		lsa->sin.sin_port = port;
	}   
}   

static char* sockaddr2str(const struct sockaddr *sa, socklen_t salen, int flags)
{
	char	host[128];
	char	serv[16];
	char	*ptr;
	int		rc;

	rc = getnameinfo(sa, salen, host, sizeof(host), serv,
					sizeof(serv), flags | NI_NUMERICSERV);
	if (rc) return NULL;
	asprintf(&ptr, "%s:%s", host, serv);
	return ptr;
}       
		
static len_and_sockaddr * str2sockaddr(const char *host, int port)
{
	int					rc;
	len_and_sockaddr	*r = NULL;
	struct addrinfo		*result = NULL;
	const char			*org_host = host; /* only for error msg */
	const char			*cp;
	struct addrinfo		hint;

#ifdef __DBG_IMGUP__	// psy-2011.07xx
    printf("\n++%s(%d): ", __func__, __LINE__);
#endif

	/* Ugly parsing of host:addr */
	cp = strrchr(host, ':');
	if (cp) 
	{
		int sz = cp - host + 1;
		host = strncpy(alloca(sz), host, sz);
		cp++; /* skip ':' */
		port = strtoul(cp, NULL, 10);
	}

	memset(&hint, 0 , sizeof(hint));
	hint.ai_family   = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	rc = getaddrinfo(host, NULL, &hint, &result);
	if (rc || !result) {
		DBG_ARG("bad address '%s'", org_host);
		return NULL;
	}
	if ((r = malloc(offsetof(len_and_sockaddr, sa) + result->ai_addrlen)) == NULL) {
		DBG_ARG("malloc(result->ai_addrlen): %s\n", strerror(errno));
		return NULL;
	}
	r->len = result->ai_addrlen;
	memcpy(&r->sa, result->ai_addr, result->ai_addrlen);
	set_nport(r, htons(port));
	freeaddrinfo(result);

#ifdef __DBG_IMGUP__	// psy-2011.07xx
    printf("\n--%s(%d): \n\n", __func__, __LINE__);
#endif
	
	return r;
}

static int ftpcmd(const char *s1, const char *s2, FILE *fp, char *buf)
{
	int result;

//    printf("\n++%s(%d): s1=[%s], s2=[%s]\n", __func__, __LINE__, s1, s2);
        
	if (s1) 
	{
		if (!s2) 
			s2 = "";

//        printf("\n +%s(%d): s1=[%s], s2=[%s]\n", __func__, __LINE__, s1, s2);
			
		fprintf(fp, "%s%s\r\n", s1, s2);
		fflush(fp);
	}

	do {
		char *buf_ptr;

		if (fgets(buf, 510, fp) == NULL) {
			printf("error getting response");
		}
		buf_ptr = strstr(buf, "\r\n");
		if (buf_ptr) {
			*buf_ptr = '\0';
		}
	} while (!isdigit(buf[0]) || buf[3] != ' ');

	buf[3] = '\0';
	result = atoi(buf);
	buf[3] = ' ';
	return result;
}

/*
 * "/"        -> "/"
 * "abc"      -> "abc"
 * "abc/def"  -> "def"
 * "abc/def/" -> ""
 */
char* bb_get_last_path_component_nostrip(const char *path)
{
	char *slash = strrchr(path, '/');

	if (!slash || (slash == path && !slash[1]))
		return (char*)path;

	return slash + 1;
}

char* _get_last_path_component(char *path, char *searchStr)
{
	char	*token;
	char	*path_in;

	DBG_ARG("path=[%s], searchStr=[%s]\n", path, searchStr);

	path_in = strstr(path, searchStr);
	if(path_in == NULL)
	{
        DBG_ARG("return path_in=[%s]\n", path_in);
		return path_in;
	}



	token = strtok(path_in, "=");

//    printf("\n +%s(%d): path_in=[%s], searchStr=[%s], token=[%s]\n", __func__, __LINE__, path_in, searchStr, token);

	token = strtok(NULL, " ");

//    printf("\n +%s(%d): path_in=[%s], searchStr=[%s], token=[%s]\n", __func__, __LINE__, path_in, searchStr, token);

//	printf("\n +%s(%d): token=[%s]\n", __func__, __LINE__, token);

	return token;

}

/*
 *	We assume that 'buffer' is large enough.
 */
int wget(const char *url, int c_port, char *d_filename)
{
	char				buf[512];
	struct host_info	server, target;
	len_and_sockaddr	*lsa;
	int					n, status;
	int					try = 5;				/* max redirection hop	*/
	char				*s;
	FILE				*sfp = NULL;			/* socket to web server         */
	int					got_clen = 0;			/* got content-length: from server  */

	int	port;
	FILE *dfp = NULL;			/* socket to ftp server (data)		*/
	int do_continue = 0;		/* continue a prev transfer (-c)	*/
	FILE *output = NULL;		/* socket to web server				*/

	char *fname_out = NULL;		/* where to direct output (-O)		*/

	char local_fname_out[128]={0, };
	
	long beg_range = 0L;		/*   range at which continue begins	*/

	int output_fd = -1;


	parse_url(url, &target);
	server.host = target.host;

#if 1	// psy : 2011.0816
	if(c_port == 0) /*&& ( 0 != atoi(G_ImgUp.port))*/
	{
        server.port = target.port;
	}
	else
	{
        server.port = c_port;
	}
#endif

#ifdef __DBG_IMGUP__	// psy-2011.08xx
    printf("\n +%s(%d) : server.host=[%s], server.port=[%d], target.path=[%s]", __func__, __LINE__, server.host, server.port, target.path);
#endif

	/* We want to do exactly _one_ DNS lookup, since some
	 * sites (i.e. ftp.us.debian.org) use round-robin DNS
	 * and we want to connect to only one IP... */
	lsa = str2sockaddr(server.host, server.port);
	if(lsa == NULL)
	{
		return RET_ERROR;
	}
	else
	{
		fprintf(stderr, "Connecting to %s [%s]\n", server.host, sockaddr2str(&lsa->sa, lsa->len, NI_NUMERICHOST)); 
	}

	if (!target.is_ftp) 
	{
        DBG_ARG("HTTP session\n\n");
		DBG_ARG("Connecting to %s [%s]\n\n", server.host, sockaddr2str(&lsa->sa, lsa->len, NI_NUMERICHOST));
	
		/*
		 *  HTTP session
		 */
		do {
			got_clen = chunked = 0;

			if (!--try) {
				DBG_ARG("too many redirections");
				return -1;
			}

			/*
			 * Open socket to http server
			 */
			if (sfp) fclose(sfp);
			sfp = open_socket(lsa);
			if (sfp == NULL) {
				return -1;
			}

			/*
			 * Send HTTP request.
			 */
			fprintf(sfp, "GET /%s HTTP/1.1\r\n", target.path);

			fprintf(sfp, "Host: %s\r\nUser-Agent: %s\r\n",
				target.host, USER_AGENT_WGET);

			fprintf(sfp, "Connection: close\r\n\r\n");

            DBG_ARG("Send HTTP requested...\n\n");

			/*
			* Retrieve HTTP response line and check for "200" status code.
			*/
	 read_response:
			if (fgets(buf, sizeof(buf), sfp) == NULL) {
				DBG_ARG("no response from server.\n");
				return -1;
			}

			s = buf;
			while (*s != '\0' && !isspace(*s)) ++s;
			s = skip_whitespace(s);
			// FIXME: no error check
			// xatou wouldn't work: "200 OK"
			status = atoi(s);
			switch (status) {
				case 0:
				case 100:
					while (gethdr(buf, sizeof(buf), sfp, &n) != NULL)
						/* eat all remaining headers */;
					goto read_response;
				case 200:
					break;
				case 300:	/* redirection */
				case 301:
				case 302:
				case 303:
					break;
				case 206:
				default:
					/* Show first line only and kill any ESC tricks */
					buf[strcspn(buf, "\n\r\x1b")] = '\0';
					DBG_ARG("server returned error: %s.\n", buf);
					return -1;
			}

            DBG_ARG("Retrieve HTTP headers.\n\n");

			/*
			 * Retrieve HTTP headers.
			 */
			while ((s = gethdr(buf, sizeof(buf), sfp, &n)) != NULL) 
			{
				if (strcasecmp(buf, "content-length") == 0) 
				{
					content_len = strtoul(s, NULL, 10);
					if (errno || content_len < 0) {
						DBG_ARG("content-length %s is garbage.\n", s);
						return -1;
					}
					got_clen = 1;
					continue;
				}
				if (strcasecmp(buf, "transfer-encoding") == 0) 
				{
					if (strcasecmp(s, "chunked") != 0) {
						DBG_ARG("server wants to do %s transfer encoding.\n", s);
						return -1;
					}
					chunked = got_clen = 1;
				}
				if (strcasecmp(buf, "location") == 0) 
				{
					if (s[0] == '/')
					{
						/* free(target.allocated); */
						target.path = strdup(s + 1);
					}
					else 
					{
						parse_url(s, &target);
						server.host = target.host;
						server.port = target.port;
						free(lsa);
						lsa = str2sockaddr(server.host, server.port);
						break;
					}
				}
			}
            DBG_ARG("size(buf)=[%d]\n\n", sizeof(buf));
			
		} while (status >= 300);

	    dfp = sfp;
	}
	else
	{
        DBG_ARG("url=[%s] : FTP session\n", url);
        DBG_ARG("target.user=[%s]\n", target.user);
	
		/*
		 *  FTP session
		 */
		if (! target.user)
			target.user = "anonymous:busybox@";

		sfp = open_socket(lsa);
		if (sfp == NULL)
			return -1;

		if (ftpcmd(NULL, NULL, sfp, buf) != 220)
		{
			DBG_ARG("\n\nError : %s", buf+4);
			return 0;
		}

		/* 
		 * Splitting username:password pair,
		 * trying to log in
		 */
		s = strchr(target.user, ':');
		if (s)
			*(s++) = '\0';
		switch(ftpcmd("USER ", target.user, sfp, buf)) 
		{
			case 230:
				break;
			case 331:
				if (ftpcmd("PASS ", s, sfp, buf) == 230)
					break;
				/* FALLTHRU (failed login) */
			default:
				printf("ftp login: %s", buf+4);
				return 0;
		}
		
		ftpcmd("type", NULL, sfp, buf);
		
		/*
		 * Querying file size
		 */
        DBG_ARG("\ntarget.path=[%s]\n", target.path);
		 
		if (ftpcmd("size ", target.path, sfp, buf) == 213) 		
		{
			filesize = atol(buf+4);
			got_clen = 1;
			content_len = filesize;
		}

		/*
		 * Entering passive mode
		 */
		if (ftpcmd("PASV", NULL, sfp, buf) !=  227)
			DBG_ARG("\n\nError : PASV: %s", buf+4);

		s = strrchr(buf, ',');
		*s = 0;
		port = atoi(s+1);

		s = strrchr(buf, ',');
		port += atoi(s+1) * 256;

        DBG_ARG("port=[%d], dfp=open_socket(lsa)\n",port);

		set_nport(lsa, htons(port));
		dfp = open_socket(lsa);

		if (do_continue) 
		{
			sprintf(buf, "REST %ld", beg_range);
			if (ftpcmd(buf, NULL, sfp, buf) != 350) {
				if (output != stdout)
					output = freopen(fname_out, "w", output);
				do_continue = 0;
			} else
				filesize -= beg_range;
		}

		if (ftpcmd("RETR ", target.path, sfp, buf) > 150)
		{
			DBG_ARG("\n\nError : bad response to RETR: %s", buf);
			return 0;
		}
	}

	
	/*
	 * Retrieve file
	 */

    DBG_ARG("target.path=[%s], strlen(target.path)==[%d], strlen(local_fname_out)==[%d]\n", target.path, strlen(target.path), strlen(local_fname_out));

	// get target file name
	if (!target.is_ftp)	
	{
//        fname_out = "server.ini";
//        fname_out = bb_get_last_path_component_nostrip(target.path);
	    fname_out = _get_last_path_component(target.path, "name=");
	    if(fname_out == NULL)
	    {
            fname_out = bb_get_last_path_component_nostrip(target.path);
	    }
	}
	else
	{
		fname_out = target.path;
	}

    sprintf(local_fname_out, "%s%s", "/tmp/", fname_out);

	strcpy(d_filename, local_fname_out);

    DBG_ARG("Retrieve file : chunked=[%d], content_len=[%d], fname_out=[%s], local_fname_out=[%s]\n", 
    			(int)chunked, (int)content_len, fname_out, local_fname_out);

	/* Do it before progress_meter (want to have nice error message) */
	if (output_fd < 0) 
	{
//		int o_flags = O_WRONLY | O_CREAT | O_TRUNC | O_EXCL;
		int o_flags = O_WRONLY | O_CREAT | O_TRUNC;
		
		/* compat with wget: -O FILE can overwrite */
		output_fd = open(local_fname_out, o_flags, 0666);
		if(output_fd < 0)
		{
            DBG_ARG("Error: File open.\n File exist.");
            return RET_ERROR;
        }
	}


	if (chunked)
		goto get_clen;

    DBG_ARG("Loops only if chunked : output_fd=[%d]\n", output_fd);

#define	DBG_NTMP	1
	/* Loops only if chunked */
	while (1) 
	{
		while (content_len > 0 || !got_clen) 
		{
			int n;
#ifdef	DBG_NTMP
			int nTmp;
#endif			
			unsigned rdsz = sizeof(buf);

			if (content_len < sizeof(buf) && (chunked || got_clen))
				rdsz = (unsigned)content_len;

			n = safe_fread(buf, 1, rdsz, dfp);
			if (n <= 0) 
			{
                DBG_ARG("Error\n\n");   
			
				if (ferror(dfp)) {
					/* perror will not work: ferror doesn't set errno */
                    DBG_ARG("Error: perror will not work: ferror doesn't set errno");
				}
				break;
			}

#ifdef	DBG_NTMP	            
			do {
			    nTmp = write(output_fd, buf, n);
			} while (nTmp < 0 /*&& errno == EINTR*/);
#else
			if (write(output_fd, buf, n)<0)
			{
                DBG_ARG("Error: write");
			}
#endif
			if (got_clen)
				content_len -= n;

		}

		if (!chunked)
			break;

		safe_fgets(buf, sizeof(buf), dfp); /* This is a newline */
 get_clen:
		safe_fgets(buf, sizeof(buf), dfp);

		/* FIXME: error check? */
		if (content_len == 0)
			break; /* all done! */
	}

    DBG_ARG("all done!\n\n");

	if (target.is_ftp) 
	{
		fclose(dfp);
		//if (ftpcmd(NULL, NULL, sfp, buf) != 226)
		//	DBG_ARG("ftp error: %s", buf+4);
		ftpcmd("QUIT", NULL, sfp, buf);
	}

	return RET_SUCCESS;
}

static unsigned short lookup_port(const char *port, const char *protocol, unsigned int default_port)
{
	unsigned int	port_nr = default_port;
	char			*endptr;

	if (port) {
		int			old_errno;

		old_errno = errno;
		port_nr = strtoul(port, &endptr, 10);
		if (errno || endptr == port) {
			struct servent *tserv = getservbyname(port, protocol);
			if (tserv)
				port_nr = ntohs(tserv->s_port);
		}
		errno = old_errno;
	}

	return (unsigned short)port_nr;
}

static int parse_url(const char *src_url, struct host_info *h)
{
	char *url, *p, *sp;

#ifdef __DBG_IMGUP__	// psy-2011.07xx
    printf("\n++%s(%d): src_url=[%s]\n", __func__, __LINE__, src_url);
#endif

	if ((url = strdup(src_url)) == NULL)
	{
		printf("strdup(%s): %s.\n", src_url, strerror(errno));
		return -1;
	}

	if (strncmp(url, "http://", 7) == 0) {
		h->port = lookup_port("http", "tcp", 80);
		h->host = url + 7;
		h->is_ftp = 0;
	} 
    else if (strncmp(url, "ftp://", 6) == 0) 
    {
        h->port = lookup_port("ftp", "tcp", 21);
        h->host = url + 6;
        h->is_ftp = 1;
	}
	else 
	{
		printf("not an http url: %s", url);
		return -1;
	}

	sp = strchr(h->host, '/');
	p = strchr(h->host, '?'); if (!sp || (p && sp > p)) sp = p;
	p = strchr(h->host, '#'); if (!sp || (p && sp > p)) sp = p;
	if (!sp) {
		/* must be writable because of bb_get_last_path_component() */
		static char nullstr[] = "";
		h->path = nullstr;
	} else if (*sp == '/') {
		*sp = '\0';
		h->path = sp + 1;
	} else { // '#' or '?'
		// http://busybox.net?login=john@doe is a valid URL
		// memmove converts to:
		// http:/busybox.nett?login=john@doe...
		memmove(h->host-1, h->host, sp - h->host);
		h->host--;
		sp[-1] = '\0';
		h->path = sp;
	}

	sp = strrchr(h->host, '@');
	h->user = NULL;
	if (sp != NULL) {
		h->user = h->host;
		*sp = '\0';
		h->host = sp + 1;
	}

	sp = h->host;

#ifdef __DBG_IMGUP__	// psy-2011.07xx
    printf("\n--%s(%d): h->port=[%d], h->is_ftp=[%d]\n\n", __func__, __LINE__, h->port, h->is_ftp);
#endif

	return 0;
}


static FILE *open_socket(len_and_sockaddr *lsa)
{
	int		fd;
	FILE	*fp;

#ifdef __DBG_IMGUP__	// psy-2011.07xx
        printf("\n++%s(%d): \n", __func__, __LINE__);
#endif

	fd = socket(lsa->sa.sa_family, SOCK_STREAM, 0);
	if (fd < 0) {
		printf("socket: %s.\n", strerror(errno));
		return NULL;
	}
	if (connect(fd, &lsa->sa, lsa->len) < 0)
	{
		printf("connect: %s.\n", strerror(errno));
		return NULL;
	}

	fp = fdopen(fd, "r+");
	if (fp == NULL) {
		printf("fdopen: %s.\n", strerror(errno));
		return NULL;
	}
	errno = 0;

#ifdef __DBG_IMGUP__	// psy-2011.07xx
    printf("--%s(%d): \n\n", __func__, __LINE__);
#endif

	return fp;
}


static char *gethdr(char *buf, size_t bufsiz, FILE *fp, int *istrunc)
{
	char *s, *hdrval;
	int c;

	*istrunc = 0;

	/* retrieve header line */
	if (fgets(buf, bufsiz, fp) == NULL)
	{
		DBG_ARG("retrieve header line\n");	
		return NULL;
	}
	
	/* see if we are at the end of the headers */
	for (s = buf; *s == '\r'; ++s)
		;
	if (s[0] == '\n')
		return NULL;

	/* convert the header name to lower case */
	for (s = buf; isalnum(*s) || *s == '-'; ++s)
		*s = tolower(*s);

	/* verify we are at the end of the header name */
	if (*s != ':')
	{
		DBG_ARG("bad header line: %s", buf);
		return NULL;
	}

	/* locate the start of the header value */
	for (*s++ = '\0'; *s == ' ' || *s == '\t'; ++s)
		;
	hdrval = s;

	/* locate the end of header */
	while (*s != '\0' && *s != '\r' && *s != '\n')
		++s;

	/* end of header found */
	if (*s != '\0') {
		*s = '\0';
//		DBG_ARG("end of header found\n");
		return hdrval;
	}

	/* Rats!  The buffer isn't big enough to hold the entire header value. */
	while (c = getc(fp), c != EOF && c != '\n')
		;
	*istrunc = 1;
	return hdrval;
}


#if 0
int main(int argc, char **argv)
{
	char            buf[BUFSIZE];
	int ret = 0;
	char			url[256];

    DBG_ARG("\n++%s(%d): url=[%s]\n", __func__, __LINE__, url);

	if (url == NULL)
		return -1;

	ret = wget(argv[1], buf);
	if ( ret != RET_SUCCESS)
	{   
		DBG_ARG("\n Error : wget file get fails. url=[%s]\n", url);
		return ret; 
	}   

	return RET_SUCCESS;
}
#endif

#include "globals.h"
#ifdef SUPPORT_ASP
#define ERROR -1
#define FAILED -1
#define SUCCESS 0
#define TRUE 1
#define FALSE 0

#define MAX_QUERY_TEMP_VAL_SIZE 8192


typedef int (*asp_funcptr)(int eid, request * req,  int argc, char **argv);
typedef void (*form_funcptr)(request * req, char* path, char* query);

typedef struct asp_name_s
{
	const char *pagename;
	asp_funcptr function;
	struct asp_name_s *next;
} asp_name_t;

typedef struct form_name_s
{
	const char *pagename;
#ifdef MULTI_USER_PRIV
	unsigned char privilege;	/* page privilege level */
#endif
	form_funcptr function;
	struct form_name_s *next;
} form_name_t;

typedef struct temp_mem_s
{
	char *str;
	struct temp_mem_s *next;
} temp_mem_t;

#ifdef MULTI_USER_PRIV
struct formPrivilege{
	const char* pagename;
	unsigned char privilege;	/* page privilege level */
};
extern struct formPrivilege formPrivilegeList[];
#endif

int boaWriteBlock(request *req, char *buf, int nChars);
//ANDREW #define boaWrite(req,format, arg...)  {	char *temp=(char *)malloc(1024); sprintf(temp,format, ##arg); boaWriteBlock(req,temp,strlen(temp)); free(temp); }

char *boaGetVar_adv(request *req, char *var, char *defaultGetValue, int index);
char *boaGetVar(request *req, char *var, char *defaultGetValue);
#ifdef CONFIG_USER_BOA_CSRF
extern unsigned char g_csrf_token[];
void get_csrf_hash(char *referer, char* csrf);
int genCSRFToken(request * req, char *token);
int handleCSRFToken(request * req);
#endif
void boaASPInit(int argc,char **argv);
#ifdef SUPPORT_ASP
void handleForm(request *req);
int init_get2(request * req);
int allocNewBuffer(request *req);
void handleScript(request * req, char *left, char *right);
void freeAllTempStr(void);
int init_form(request * req);

int addASPTempStr(char *str);
void freeAllASPTempStr(void);
#endif
int boaRefreshParent(request *req,char *url);
int boaRedirectTemp(request *req,char *url);
int boaRedirect(request *req,char *url);
#endif
#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_AP_CMCC)
int Pppoe_init_json(request * req);
#endif


#ifndef PORT_H_
#define PORT_H_

//typedef request* request *;
#define B_ARGS_DEC		char *file, int line

extern int getInfo(int eid, request* wp, int argc, char  **argv);
extern int ipPortFilterList(int eid, request* wp, int argc, char **argv);
extern int macFilterList(int eid, request* wp, int argc, char **argv);
extern int atmVcList(int eid, request* wp, int argc, char **argv);
extern int atmVcList2(int eid, request* wp, int argc, char **argv);
extern int wanConfList(int eid, request* wp, int argc, char **argv);
extern int DHCPSrvStatus(int eid, request *wp, int argc, char **argv);
/* Routines exported in fmtcpip.c */
extern void formTcpipLanSetup(request * wp, char *path, char *query);

/* Routines exported in fmfwall.c */
extern void formPortFw(request * wp, char *path, char *query);
extern void formStaticMapping(request * wp, char *path, char *query);
extern void formFilter(request * wp, char *path, char *query);
extern void formDMZ(request * wp, char *path, char *query);
#ifdef PORT_TRIGGERING_DYNAMIC
extern void formPortTrigger(request * wp, char *path, char *query);
extern int portTriggerList(int eid, request * wp, int argc, char **argv);
#endif
extern int portFwList(int eid, request * wp, int argc, char **argv);
extern int portStaticMappingList(int eid, request * wp, int argc, char **argv);
extern int ipPortFilterList(int eid, request * wp, int argc, char **argv);
extern int macFilterList(int eid, request * wp, int argc, char **argv);

/* Routines exported in fmmgmt.c */
extern void formPasswordSetup(request * wp, char *path, char *query);
extern void formUpload(request * wp, char * path, char * query);
extern void formSaveConfig(request * wp, char *path, char *query);
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
extern void formSnmpConfig(request * wp, char *path, char *query);
#endif
extern void formAdslDrv(request * wp, char *path, char *query);
extern void formStats(request * wp, char *path, char *query);
extern void formRconfig(request * wp, char *path, char *query);
extern int adslDrvSnrTblGraph(int eid, request * wp, int argc, char **argv);
extern int adslDrvSnrTblList(int eid, request * wp, int argc, char **argv);
extern int adslDrvBitloadTblGraph(int eid, request * wp, int argc, char **argv);
extern int adslDrvBitloadTblList(int eid, request * wp, int argc, char **argv);
extern int pktStatsList(int eid, request * wp, int argc, char **argv);

#ifndef _BOOL_T_
#define _BOOL_T_
typedef short bool_t;
#endif

char *mgmtUserName(void);
char *mgmtPassword(void);
int boaArgs(int argc, char **argv, const char *fmt, ...);
void boaError(request *wp, int code, const char *fmt, ...);
void boaDone(request *wp, int code);
void boaHeader(request* wp);
void boaFooter(request* wp);
int boaWrite(request * req, const char * fmt, ...);
int boaWriteDataNonBlock(request *req, char *msg, int msg_len);
int boaDeleteUser(char *user);
int boaDeleteAccessLimit(char *url);
int boaDeleteGroup(char *group);
bool_t boaGroupExists(char *group);
int boaAddGroup(char *group, short priv, void * am, bool_t prot, bool_t disabled);
bool_t boaAccessLimitExists(char *url);
int boaAddAccessLimit(char *url, void * am, short secure, char *group);
int boaAddUser(char *user, char *pass, char *group, bool_t prot, bool_t disabled);
void error(char *file, int line, int etype, char *fmt, ...);
void boaFormDefineUserMgmt(void);
#endif

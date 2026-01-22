/*
 *      Web server handler routines for System status
 *
 */

#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "debug.h"
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_bridge.h>
#include "adsl_drv.h"
#include <stdio.h>
#include <fcntl.h>
#include "signal.h"
#include "../defs.h"
#include "multilang.h"
#include <unistd.h>
#if defined(CONFIG_LUNA)
#include <rtk/port.h>
#if defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_stat.h>
#include <rtk/rt/rt_port.h>
#endif
#endif
#include "../subr_net.h"
#include <netinet/in.h>

static const char IF_UP[] = "up";
static const char IF_DOWN[] = "down";
static const char IF_NA[] = "n/a";
#ifdef EMBED
#ifdef CONFIG_USER_PPPOMODEM
const char PPPOM_CONF[] = "/var/ppp/pppom.conf";
#endif //CONFIG_USER_PPPOMODEM
#endif

//#if defined(CONFIG_RTL_8676HWNAT)
void formLANPortStatus(request * wp, char *path, char *query)
{
	char *submitUrl, *strSubmitR;
	strSubmitR = boaGetVar(wp, "refresh", "");
	// Refresh
	if (strSubmitR[0]) {
		goto setOk_filter;
	}

setOk_filter:
	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
  	return;
}
int showLANPortStatus(int eid, request * wp, int argc, char **argv)
{
	int i;
	unsigned char strbuf[256];
	for(i=0; i<SW_LAN_PORT_NUM; i++){
		getLANPortStatus(i, strbuf);
#ifndef CONFIG_GENERAL_WEB
		boaWrite(wp,"<tr bgcolor=\"#EEEEEE\">\n"
	    "<td width=40%%><font size=2><b>LAN%d</b></td>\n"
	    "<td width=60%%><font size=2>%s</td>\n</tr>", virt2user[i], strbuf);
#else
		boaWrite(wp,"<tr>\n"
	    "<td width=40%%>LAN%d</td>\n"
	    "<td width=60%%>%s</td>\n</tr>", virt2user[i], strbuf);
#endif
	}
	return 0;
}
//#endif

void formStatus(request * wp, char *path, char *query)
{
	char *submitUrl, *strSubmitR, tmpBuf[100];
#ifdef CONFIG_PPP
	char *strSubmitP;
	struct data_to_pass_st msg;
	char buff[256];
	unsigned int i, flag, inf;
	FILE *fp;
#endif
#ifdef CONFIG_USER_PPPD
	int vcTotal;
	MIB_CE_ATM_VC_T Entry;
	char tmp[8], tmp2[15];
	char ppppid_str[32];
	int ppp_pid=0;
	int status=1;
#endif
#ifdef CONFIG_USER_PPPOMODEM
	unsigned int cflag[MAX_PPP_NUM+MAX_MODEM_PPPNUM] = {0};
#else
	unsigned int cflag[MAX_PPP_NUM] = {0};
#endif //CONFIG_USER_PPPOMODEM

#ifdef CONFIG_PPP
#ifdef CONFIG_USER_SPPPD
	// for PPP connecting/disconnecting
#ifdef CONFIG_USER_PPPOMODEM
	for (i=0; i<(MAX_PPP_NUM+MAX_MODEM_PPPNUM); i++)
#else
	for (i=0; i<MAX_PPP_NUM; i++)
#endif //CONFIG_USER_PPPOMODEM
	{
		char tmp[15], tp[10];

		sprintf(tp, "ppp%d", i);
		cflag[i] = -1; //initial cflag -> -1, because when ppp index = 0, cflag[i] will = 0.
		if (find_ppp_from_conf(tp)) {
			if (fp=fopen("/tmp/ppp_up_log", "r")) {
				while ( fgets(buff, sizeof(buff), fp) != NULL ) {
					if(sscanf(buff, "%d %d", &inf, &flag) != 2)
						break;
					else {
						if (inf == i)
							cflag[i] = flag;
					}
				}
				fclose(fp);
			}
			sprintf(tmp, "submitppp%d", i);
			strSubmitP = boaGetVar(wp, tmp, "");
			if ( strSubmitP[0] ) {
				if ((strcmp(strSubmitP, multilang(LANG_CONNECT)) == 0)) {
					if (cflag[i] != -1) { /* not be -1 means have value from /tmp/ppp_up_log */
						snprintf(msg.data, BUF_SIZE, "spppctl up %u", i);
						usleep(3000000);
						TRACE(STA_SCRIPT, "%s\n", msg.data);
						write_to_pppd(&msg);
							//add by ramen to resolve for clicking "connect" button twice.
					}
				} else if (strcmp(strSubmitP, multilang(LANG_DISCONNECT)) == 0) {

					snprintf(msg.data, BUF_SIZE, "spppctl down %u", i);
					TRACE(STA_SCRIPT, "%s\n", msg.data);
					write_to_pppd(&msg);
						//add by ramen to resolve for clicking "disconnect" button twice.
				} else {
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, multilang(LANG_INVALID_PPP_ACTION),sizeof(tmpBuf)-1);
					goto setErr_filter;
				}
			}
		}
	}
#endif // CONFIG_USER_SPPPD
#ifdef CONFIG_USER_PPPD
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < vcTotal; i++) {
	/* get the specified chain record */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return;

		if (Entry.cmode != CHANNEL_MODE_PPPOE && Entry.cmode != CHANNEL_MODE_PPPOA)
			continue;

		ifGetName(Entry.ifIndex, tmp, sizeof(tmp));
		sprintf(tmp2, "submit%s", tmp);
		strSubmitP = boaGetVar(wp, tmp2, "");
		if ( strSubmitP[0] ) {
			snprintf(ppppid_str, 32, "%s/ppp%u.pid", PPPD_CONFIG_FOLDER, PPP_INDEX(Entry.ifIndex));
			ppp_pid = read_pid((char *)ppppid_str);

			if ((strcmp(strSubmitP, multilang(LANG_CONNECT)) == 0)) {
				if ((0 != access((const char *)ppppid_str, F_OK)) && (Entry.pppCtype == MANUAL)) {
					ifGetName(PHY_INTF(Entry.ifIndex), tmp, sizeof(tmp));
					connectPPP(tmp, &Entry, Entry.cmode);
				}
			} else if (strcmp(strSubmitP, multilang(LANG_DISCONNECT)) == 0) {
				if (ppp_pid > 0)
					kill(ppp_pid, SIGHUP);
			} else {
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, multilang(LANG_INVALID_PPP_ACTION),sizeof(tmpBuf)-1);
				goto setErr_filter;
			}
		}
	}
#endif	// CONFIG_USER_PPPD
#endif

	strSubmitR = boaGetVar(wp, "refresh", "");
	// Refresh
	if (strSubmitR[0]) {
		goto setOk_filter;
	}

setOk_filter:
	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
  	return;

setErr_filter:
	ERR_MSG(tmpBuf);
}

void formDate(request * wp, char *path, char *query)
{
	char *strVal, *submitUrl;
	time_t tm;
	struct tm tm_time;
	struct timespec ts;

	time(&tm);
	memcpy(&tm_time, localtime(&tm), sizeof(tm_time));

	strVal = boaGetVar(wp, "sys_month", "");
	if (strVal[0])
		tm_time.tm_mon = atoi(strVal);

	strVal = boaGetVar(wp, "sys_day", "");
	if (strVal[0])
		tm_time.tm_mday = atoi(strVal);

	strVal = boaGetVar(wp, "sys_year", "");
	if (strVal[0])
		tm_time.tm_year = atoi(strVal) - 1900;

	strVal = boaGetVar(wp, "sys_hour", "");
	if (strVal[0])
		tm_time.tm_hour = atoi(strVal);

	strVal = boaGetVar(wp, "sys_minute", "");
	if (strVal[0])
		tm_time.tm_min = atoi(strVal);

	strVal = boaGetVar(wp, "sys_second", "");
	if (strVal[0])
		tm_time.tm_sec = atoi(strVal);

	tm = mktime(&tm_time);

	ts.tv_sec = tm;
	ts.tv_nsec = 0;
	if (tm < 0 || clock_settime(CLOCK_REALTIME, &ts) < 0) {
		perror("cannot set date");
	}

	OK_MSG1(multilang(LANG_SYSTEM_DATE_HAS_BEEN_MODIFIED_SUCCESSFULLY_PLEASE_REFLESH_YOUR_STATUS_PAGE), (char *)NULL);
	return;
}

int cpuUtility(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
#ifdef CPU_UTILITY	
	unsigned char buffer[256+1]="";

	if (getSYS2Str(SYS_CPU_UTIL, buffer))
		sprintf(buffer, "");
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr bgcolor=\"#EEEEEE\">\n");
	nBytesSent += boaWrite(wp, "	<td width=40%%><font size=2><b>%s</b></td>\n", multilang(LANG_CPU_UTILITY));
	nBytesSent += boaWrite(wp, "	<td width=60%%><font size=2>%s</td>\n", buffer);
#else
	nBytesSent += boaWrite(wp, "<tr>\n");
	nBytesSent += boaWrite(wp, "    <th width=40%%>%s</th>\n", multilang(LANG_CPU_UTILITY));
	nBytesSent += boaWrite(wp, "	<td width=60%%><font size=2>\n");
	//progress bar
	nBytesSent += boaWrite(wp, "	<div class=\"progress\" style=\"width: 80%%; background-color: rgba(180,180,180,0.9);margin-bottom:0px ;height:15px;\">\n");
	nBytesSent += boaWrite(wp, "	<div class=\"progress-bar progress-bar-success progress-bar-striped progress-bar-animated active\" role=\"progressbar\" aria-valuenow=\"%d\" aria-valuemin=\"0\" aria-valuemax=\"100\" style=\"width:%s\"><font color=\"#FFFFFF\" size=2>%s</font></div>\n", get_cpu_usage(), buffer, buffer);
	nBytesSent += boaWrite(wp, "	</div>\n");
	//progress bar end
	nBytesSent += boaWrite(wp, "	</td>\n");
#endif
	nBytesSent += boaWrite(wp, "</tr>\n");
#endif	
	return nBytesSent;
}

int memUtility(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
#ifdef MEM_UTILITY	
	unsigned char buffer[256+1]="";
	
	if (getSYS2Str(SYS_MEM_UTIL, buffer))
		sprintf(buffer, "");
	
#ifndef CONFIG_GENERAL_WEB	
	nBytesSent += boaWrite(wp, "<tr bgcolor=\"#DDDDDD\">\n");
	nBytesSent += boaWrite(wp, "	<td width=40%%><font size=2><b>%s</b></td>\n", multilang(LANG_MEM_UTILITY));
	nBytesSent += boaWrite(wp, "	<td width=60%%><font size=2>%s</td>\n", buffer);
#else
	nBytesSent += boaWrite(wp, "<tr>\n");
	nBytesSent += boaWrite(wp, "    <th width=40%%>%s</th>\n", multilang(LANG_MEM_UTILITY));
	nBytesSent += boaWrite(wp, "    <td width=60%%>\n");
	//progress bar
	nBytesSent += boaWrite(wp, "	<div class=\"progress\" style=\"width: 80%%; background-color: rgba(180,180,180,0.9); margin-bottom:0px ;height:15px;\">\n");
	nBytesSent += boaWrite(wp, "	<div class=\"progress-bar progress-bar-success progress-bar-striped progress-bar-animated active\" role=\"progressbar\" aria-valuenow=\"%d\" aria-valuemin=\"0\" aria-valuemax=\"100\" style=\"width:%s\"><font color=\"#FFFFFF\" size=2>%s</font></div>\n", get_mem_usage(), buffer, buffer);
	nBytesSent += boaWrite(wp, "	</div>\n");
	//progress bar end
	nBytesSent += boaWrite(wp, "	</td>\n");
#endif
	nBytesSent += boaWrite(wp, "</tr>\n");
#endif
	return nBytesSent;
}

#ifdef CONFIG_USER_PPPOMODEM
#undef FILE_LOCK
int wan3GTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
#ifndef CONFIG_GENERAL_WEB
	
	nBytesSent += boaWrite(wp, "<br>\n");
	nBytesSent += boaWrite(wp, "<table width=600 border=0>\n");
	nBytesSent += boaWrite(wp, "  <tr>\n");
	nBytesSent += boaWrite(wp, "	<td width=100%% colspan=5 bgcolor=\"#008000\">\n"
								  " 	 <font color=\"#FFFFFF\" size=2><b>3G %s</b></font>\n"
								  "    </td>\n", multilang(LANG_CONFIGURATION));
	nBytesSent += boaWrite(wp, "  </tr>\n");
	nBytesSent += boaWrite(wp, "  <tr bgcolor=\"#808080\">\n"
								  "    <td width=\"15%%\" align=center><font size=2><b>%s</b></font></td>\n"
 	                              "    <td width=\"15%%\" align=center><font size=2><b>%s</b></font></td>\n"
 	                              "    <td width=\"25%%\" align=center><font size=2><b>%s</b></font></td>\n"
 	                              "    <td width=\"25%%\" align=center><font size=2><b>%s</b></font></td>\n"
 	                              "    <td width=\"20%%\" align=center><font size=2><b>%s</b></font></td>\n"
	                              "  </tr>\n",
#else
	nBytesSent += boaWrite(wp, "<div class=\"column\">\n");
	nBytesSent += boaWrite(wp, "<div class=\"column_title\">\n");
	nBytesSent += boaWrite(wp, "   <div class=\"column_title_left\"></div>\n"
	                              "      <p>3G %s</p>\n"
	                              "    <div class=\"column_title_right\"></div>\n", multilang(LANG_CONFIGURATION));
	nBytesSent += boaWrite(wp, "  </div>\n");

	nBytesSent += boaWrite(wp, "  <div class=\"data_common data_vertical\">\n"
								"<table>"
                              	      "    <th width=\"15%%\" align=center>%s</th>\n"
	                              "    <th width=\"15%%\" align=center>%s</th>\n"
	                              "    <th width=\"25%%\" align=center>%s</th>\n"
	                              "    <th width=\"25%%\" align=center>%s</th>\n"
	                              "    <th width=\"20%%\" align=center>%s</th>\n"
	                              "  </tr>\n",
#endif	              
				multilang(LANG_INTERFACE), multilang(LANG_PROTOCOL), multilang(LANG_IP_ADDRESS),
				multilang(LANG_GATEWAY), multilang(LANG_STATUS));

	{
		MIB_WAN_3G_T entry,*p;
		p=&entry;
		if( mib_chain_get( MIB_WAN_3G_TBL, 0, (void*)p) && p->enable )
		{
			int mppp_idx;
			char mppp_ifname[IFNAMSIZ];
			char mppp_protocol[10];
			char mppp_ipaddr[20];
			char mppp_remoteip[20];
			char *mppp_status;
			char mppp_uptime[20]="";
			char mppp_totaluptime[20]="";
			struct in_addr inAddr;
			int flags;
			char *temp;
			int pppConnectStatus, pppDod;
			FILE *fp;
			#ifdef FILE_LOCK
			struct flock flpom;
			int fdpom;
			#endif

			mppp_idx=MODEM_PPPIDX_FROM;
			sprintf( mppp_ifname, "ppp%d", mppp_idx );
			mppp_protocol[sizeof(mppp_protocol)-1]='\0';
			strncpy( mppp_protocol, "PPP" ,sizeof(mppp_protocol)-1);

			if (getInAddr( mppp_ifname, IP_ADDR, (void *)&inAddr) == 1)
			{
				sprintf( mppp_ipaddr, "%s",   inet_ntoa(inAddr) );
				if (strcmp(mppp_ipaddr, "64.64.64.64") == 0)
				{
					mppp_ipaddr[sizeof(mppp_ipaddr)-1]='\0';
					strncpy(mppp_ipaddr, "",sizeof(mppp_ipaddr)-1);
				}
			}else
			{
				mppp_ipaddr[sizeof(mppp_ipaddr)-1]='\0';
				strncpy( mppp_ipaddr, "" ,sizeof(mppp_ipaddr)-1);
			}

			if (getInAddr( mppp_ifname, DST_IP_ADDR, (void *)&inAddr) == 1)
			{
				struct in_addr gw_in;
				char gw_tmp[20];
				gw_in.s_addr=htonl(0x0a404040+mppp_idx);
				sprintf( gw_tmp, "%s",    inet_ntoa(gw_in) );

				sprintf( mppp_remoteip, "%s",   inet_ntoa(inAddr) );
				if( strcmp(mppp_remoteip, gw_tmp)==0 )
				{
					mppp_remoteip[sizeof(mppp_remoteip)-1]='\0';
					strncpy(mppp_remoteip, "",sizeof(mppp_remoteip)-1);
				}
				else if (strcmp(mppp_remoteip, "64.64.64.64") == 0)
				{
					mppp_remoteip[sizeof(mppp_remoteip)-1]='\0';
					strncpy(mppp_remoteip, "",sizeof(mppp_remoteip)-1);
				}
			}else
			{
				mppp_remoteip[sizeof(mppp_remoteip)-1]='\0';
				strncpy( mppp_remoteip, "" ,sizeof(mppp_remoteip)-1);
			}


			if (getInFlags( mppp_ifname, &flags) == 1)
			{
				if (flags & IFF_UP) {
					if (getInAddr(mppp_ifname, IP_ADDR, (void *)&inAddr) == 1) {
						temp = inet_ntoa(inAddr);
						if (strcmp(temp, "64.64.64.64"))
							mppp_status = (char *)IF_UP;
						else
							mppp_status = (char *)IF_DOWN;
					}else
						mppp_status = (char *)IF_DOWN;
				}else
					mppp_status = (char *)IF_DOWN;
			}else
				mppp_status = (char *)IF_NA;

			if (strcmp(mppp_status, (char *)IF_UP) == 0)
				pppConnectStatus = 1;
			else{
				pppConnectStatus = 0;
				mppp_ipaddr[0] = '\0';
				mppp_remoteip[0] = '\0';
			}

			if(p->backup || p->ctype==CONNECT_ON_DEMAND && p->idletime!=0) //added by paula, 3g backup PPP
				pppDod=1;
			else
				pppDod=0;

			#ifdef FILE_LOCK
			//file locking
			fdpom = open(PPPOM_CONF, O_RDWR);
			if (fdpom != -1) {
				flpom.l_type = F_WRLCK;
				flpom.l_whence = SEEK_SET;
				flpom.l_start = 0;
				flpom.l_len = 0;
				flpom.l_pid = getpid();
				if (fcntl(fdpom, F_SETLKW, &flpom) == -1)
					printf("pppom write lock failed\n");
				//printf( "wan3GTable: pppom write lock successfully\n" );
			}
			#endif
			if (!(fp=fopen(PPPOM_CONF, "r")))
				printf("%s not exists.\n", PPPOM_CONF);
			else {
				char	buff[256], tmp1[20], tmp2[20], tmp3[20], tmp4[20];

				fgets(buff, sizeof(buff), fp);
				if( fgets(buff, sizeof(buff), fp) != NULL )
				{
					if (sscanf(buff, "%s%*s%s%s", tmp1, tmp2, tmp3) != 3)
					{
						printf("Unsuported pppoa configuration format\n");
					}else {
						if( !strcmp(mppp_ifname,tmp1) )
						{
							mppp_uptime[sizeof(mppp_uptime)-1]='\0';
							strncpy(mppp_uptime, tmp2,sizeof(mppp_uptime)-1);
							mppp_totaluptime[sizeof(mppp_totaluptime)-1]='\0';
							strncpy(mppp_totaluptime, tmp3,sizeof(mppp_totaluptime)-1);
						}
					}
				}
				fclose(fp);
			}
			#ifdef FILE_LOCK
			//file unlocking
			if (fdpom != -1) {
				flpom.l_type = F_UNLCK;
				if (fcntl(fdpom, F_SETLK, &flpom) == -1)
					printf("pppom write unlock failed\n");
				close(fdpom);
				//printf( "wan3GTable: pppom write unlock successfully\n" );
			}
			#endif
#ifndef CONFIG_GENERAL_WEB
			nBytesSent += boaWrite(wp, "  <tr bgcolor=\"#EEEEEE\">\n"
			                              "    <td align=center width=\"15%%\"><font size=2>%s</td>\n"
			                              "    <td align=center width=\"15%%\"><font size=2>%s</td>\n"
			                              "    <td align=center width=\"25%%\"><font size=2>%s</td>\n"
			                              "    <td align=center width=\"25%%\"><font size=2>%s</td>\n",
			                              mppp_ifname, mppp_protocol, mppp_ipaddr, mppp_remoteip);

			nBytesSent += boaWrite(wp, "    <td align=center width=\"20%%\"><font size=2>%s %s / %s ",
								mppp_status, mppp_uptime, mppp_totaluptime );
			if(!pppDod)
			{
				nBytesSent += boaWrite(wp, "<input type=\"submit\" value=\"%s\" name=\"submit%s\" onClick=\"disButton('%s'); return on_submit(this)\">\n",
							(pppConnectStatus==1) ?
							multilang(LANG_DISCONNECT) : multilang(LANG_CONNECT), mppp_ifname);
			}
#else
			nBytesSent += boaWrite(wp, "  <tr>\n"
										  "    <td align=center width=\"15%%\">%s</td>\n"
										  "    <td align=center width=\"15%%\">%s</td>\n"
										  "    <td align=center width=\"25%%\">%s</td>\n"
										  "    <td align=center width=\"25%%\">%s</td>\n",					   
										  mppp_ifname, mppp_protocol, mppp_ipaddr, mppp_remoteip);

			nBytesSent += boaWrite(wp, "	<td align=center width=\"20%%\">%s %s / %s ",
								mppp_status, mppp_uptime, mppp_totaluptime );
			if(!pppDod)
			{
				nBytesSent += boaWrite(wp, "<input class=\"inner_btn\" type=\"submit\" value=\"%s\" name=\"submit%s\" onClick=\"disButton('%s'); return on_submit(this)\">\n",
							(pppConnectStatus==1) ?
							multilang(LANG_DISCONNECT) : multilang(LANG_CONNECT), mppp_ifname);
			}
#endif
			nBytesSent += boaWrite(wp, "	</td>\n");
			nBytesSent += boaWrite(wp, "  </tr>\n");
		}
	}

	nBytesSent += boaWrite(wp, "</table>");
#ifdef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "</div></div>");
#endif

	return nBytesSent;
}
#else
int wan3GTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	return nBytesSent;
}
#endif //CONFIG_USER_PPPOMODEM

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
int wanPPTPTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	MIB_PPTP_T Entry;
	unsigned int entryNum, i;

#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<br>\n");
	nBytesSent += boaWrite(wp, "<table width=600 border=0>\n");
	nBytesSent += boaWrite(wp, "  <tr>\n");
	nBytesSent += boaWrite(wp, "    <td width=100%% colspan=5 bgcolor=\"#008000\">\n"
	                              "      <font color=\"#FFFFFF\" size=2><b>PPTP %s</b></font>\n"
	                              "    </td>\n", multilang(LANG_CONFIGURATION));
	nBytesSent += boaWrite(wp, "  </tr>\n");

	nBytesSent += boaWrite(wp, "  <tr bgcolor=\"#808080\">\n"
                              	      "    <td width=\"15%%\" align=center><font size=2><b>%s</b></font></td>\n"
	                              "    <td width=\"15%%\" align=center><font size=2><b>%s</b></font></td>\n"
	                              "    <td width=\"25%%\" align=center><font size=2><b>%s</b></font></td>\n"
	                              "    <td width=\"25%%\" align=center><font size=2><b>%s</b></font></td>\n"
	                              "    <td width=\"20%%\" align=center><font size=2><b>%s</b></font></td>\n"
	                              "  </tr>\n",
#else
	nBytesSent += boaWrite(wp, "<div class=\"column\">\n");
	nBytesSent += boaWrite(wp, "<div class=\"column_title\">\n");
	nBytesSent += boaWrite(wp, "   <div class=\"column_title_left\"></div>\n"
	                              "      <p>PPTP %s</p>\n"
	                              "    <div class=\"column_title_right\"></div>\n", multilang(LANG_CONFIGURATION));
	nBytesSent += boaWrite(wp, "  </div>\n");
	nBytesSent += boaWrite(wp, "  <div class=\"data_common data_vertical\">\n"
								"<table>");
	nBytesSent += boaWrite(wp, "  <tr>\n"
                              	      "    <th width=\"15%%\" align=center>%s</th>\n"
	                              "    <th width=\"15%%\" align=center>%s</th>\n"
	                              "    <th width=\"25%%\" align=center>%s</th>\n"
	                              "    <th width=\"25%%\" align=center>%s</th>\n"
	                              "    <th width=\"20%%\" align=center>%s</th>\n"
	                              "  </tr>\n",
#endif
				multilang(LANG_INTERFACE), multilang(LANG_PROTOCOL), multilang(LANG_IP_ADDRESS),
				multilang(LANG_GATEWAY), multilang(LANG_STATUS));

	entryNum = mib_chain_total(MIB_PPTP_TBL);
	for (i=0; i<entryNum; i++)
	{
		char mppp_ifname[IFNAMSIZ];
		char mppp_protocol[10];
		char mppp_ipaddr[20];
		char mppp_remoteip[20];
		char *mppp_status;
		struct in_addr inAddr;
		int flags;

		if (!mib_chain_get(MIB_PPTP_TBL, i, (void *)&Entry))
		{
			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

		ifGetName(Entry.ifIndex, mppp_ifname, sizeof(mppp_ifname));
		mppp_protocol[sizeof(mppp_protocol)-1]='\0';
		strncpy( mppp_protocol, "PPP" ,sizeof(mppp_protocol)-1);

		if (getInAddr( mppp_ifname, IP_ADDR, (void *)&inAddr) == 1)
		{
			sprintf( mppp_ipaddr, "%s",   inet_ntoa(inAddr) );
		}else
		{
			mppp_ipaddr[sizeof(mppp_ipaddr)-1]='\0';
			strncpy( mppp_ipaddr, "" ,sizeof(mppp_ipaddr)-1);
		}

		if (getInAddr( mppp_ifname, DST_IP_ADDR, (void *)&inAddr) == 1)
		{
			sprintf( mppp_remoteip, "%s",   inet_ntoa(inAddr) );
		}else
		{
			mppp_remoteip[sizeof(mppp_remoteip)-1]='\0';
			strncpy( mppp_remoteip, "" ,sizeof(mppp_remoteip)-1);
		}

		if (getInFlags( mppp_ifname, &flags) == 1)
		{
			if (flags & IFF_UP) {
				mppp_status = (char *)IF_UP;
			}else
				mppp_status = (char *)IF_DOWN;
		}else
			mppp_status = (char *)IF_NA;
#ifndef CONFIG_GENERAL_WEB
		nBytesSent += boaWrite(wp, "  <tr bgcolor=\"#EEEEEE\">\n"
		                              "    <td align=center width=\"15%%\"><font size=2>%s</td>\n"
		                              "    <td align=center width=\"15%%\"><font size=2>%s</td>\n"
		                              "    <td align=center width=\"25%%\"><font size=2>%s</td>\n"
		                              "    <td align=center width=\"25%%\"><font size=2>%s</td>\n",
		                              mppp_ifname, mppp_protocol, mppp_ipaddr, mppp_remoteip);

		nBytesSent += boaWrite(wp, "    <td align=center width=\"20%%\"><font size=2>%s ",
							mppp_status );
#else
		nBytesSent += boaWrite(wp, "  <tr>\n"
		                              "    <td align=center width=\"15%%\">%s</td>\n"
		                              "    <td align=center width=\"15%%\">%s</td>\n"
		                              "    <td align=center width=\"25%%\">%s</td>\n"
		                              "    <td align=center width=\"25%%\">%s</td>\n",
		                              mppp_ifname, mppp_protocol, mppp_ipaddr, mppp_remoteip);

		nBytesSent += boaWrite(wp, "    <td align=center width=\"20%%\">%s ",
							mppp_status );
#endif
		nBytesSent += boaWrite(wp, "	</td>\n");
		nBytesSent += boaWrite(wp, "  </tr>\n");

	}
	nBytesSent += boaWrite(wp, "</table>");
#ifdef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "</div></div>");
#endif
	return nBytesSent;
}
#else
int wanPPTPTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	return nBytesSent;
}
#endif //CONFIG_USER_PPTP_CLIENT_PPTP

#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
int wanL2TPTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	MIB_L2TP_T Entry;
	unsigned int entryNum, i;

#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<br>\n");
	nBytesSent += boaWrite(wp, "<table width=600 border=0>\n");
	nBytesSent += boaWrite(wp, "  <tr>\n");
	nBytesSent += boaWrite(wp, "    <td width=100%% colspan=5 bgcolor=\"#008000\">\n"
	                              "      <font color=\"#FFFFFF\" size=2><b>L2TP %s</b></font>\n"
	                              "    </td>\n", multilang(LANG_CONFIGURATION));
	nBytesSent += boaWrite(wp, "  </tr>\n");

	nBytesSent += boaWrite(wp, "  <tr bgcolor=\"#808080\">\n"
                              	      "    <td width=\"15%%\" align=center><font size=2><b>%s</b></font></td>\n"
	                              "    <td width=\"15%%\" align=center><font size=2><b>%s</b></font></td>\n"
	                              "    <td width=\"25%%\" align=center><font size=2><b>%s %s</b></font></td>\n"
	                              "    <td width=\"25%%\" align=center><font size=2><b>%s %s</b></font></td>\n"
	                              "    <td width=\"20%%\" align=center><font size=2><b>%s</b></font></td>\n"
	                              "  </tr>\n",
#else
	nBytesSent += boaWrite(wp, "<div class=\"column\">\n");
	nBytesSent += boaWrite(wp, "<div class=\"column_title\">\n");
	nBytesSent += boaWrite(wp, "   <div class=\"column_title_left\"></div>\n"
	                              "      <p>L2TP %s</p>\n"
	                              "    <div class=\"column_title_right\"></div>\n", multilang(LANG_CONFIGURATION));
	nBytesSent += boaWrite(wp, "  </div>\n");
	nBytesSent += boaWrite(wp, "  <div class=\"data_common data_vertical\">\n"
								"<table>");
								
	nBytesSent += boaWrite(wp, "  <tr>\n"
                              	      "    <th width=\"15%%\" align=center>%s</th>\n"
	                              "    <th width=\"15%%\" align=center>%s</th>\n"
	                              "    <th width=\"25%%\" align=center>%s %s</th>\n"
	                              "    <th width=\"25%%\" align=center>%s %s</th>\n"
	                              "    <th width=\"20%%\" align=center>%s</th>\n"
	                              "  </tr>\n",
#endif
		multilang(LANG_INTERFACE), multilang(LANG_PROTOCOL), multilang(LANG_LOCAL),
		multilang(LANG_IP_ADDRESS), multilang(LANG_REMOTE), multilang(LANG_IP_ADDRESS),
		multilang(LANG_STATUS));

	entryNum = mib_chain_total(MIB_L2TP_TBL);
	for (i=0; i<entryNum; i++)
	{
		char mppp_ifname[IFNAMSIZ];
		char mppp_protocol[10];
		char mppp_ipaddr[20];
		char mppp_remoteip[20];
		char *mppp_status;
		struct in_addr inAddr;
		int flags;

		if (!mib_chain_get(MIB_L2TP_TBL, i, (void *)&Entry))
		{
			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

		ifGetName(Entry.ifIndex, mppp_ifname, sizeof(mppp_ifname));
		mppp_protocol[sizeof(mppp_protocol)-1]='\0';
		strncpy( mppp_protocol, "PPP" ,sizeof(mppp_protocol)-1);

		if (getInAddr( mppp_ifname, IP_ADDR, (void *)&inAddr) == 1)
		{
			sprintf( mppp_ipaddr, "%s",   inet_ntoa(inAddr) );
		}else
		{
			mppp_ipaddr[sizeof(mppp_ipaddr)-1]='\0';
			strncpy( mppp_ipaddr, "" ,sizeof(mppp_ipaddr)-1);
		}

		if (getInAddr( mppp_ifname, DST_IP_ADDR, (void *)&inAddr) == 1)
		{
			sprintf( mppp_remoteip, "%s",   inet_ntoa(inAddr) );
		}else
		{
			mppp_remoteip[sizeof(mppp_remoteip)-1]='\0';
			strncpy( mppp_remoteip, "" ,sizeof(mppp_remoteip)-1);
		}

		if (getInFlags( mppp_ifname, &flags) == 1)
		{
			if (flags & IFF_UP) {
				mppp_status = (char *)IF_UP;
			}else
				mppp_status = (char *)IF_DOWN;
		}else
			mppp_status = (char *)IF_NA;
#ifndef CONFIG_GENERAL_WEB
		nBytesSent += boaWrite(wp, "  <tr bgcolor=\"#EEEEEE\">\n"
		                              "    <td align=center width=\"15%%\"><font size=2>%s</td>\n"
		                              "    <td align=center width=\"15%%\"><font size=2>%s</td>\n"
		                              "    <td align=center width=\"25%%\"><font size=2>%s</td>\n"
		                              "    <td align=center width=\"25%%\"><font size=2>%s</td>\n",
		                              mppp_ifname, mppp_protocol, mppp_ipaddr, mppp_remoteip);

		nBytesSent += boaWrite(wp, "    <td align=center width=\"20%%\"><font size=2>%s ",
							mppp_status );
#else
		nBytesSent += boaWrite(wp, "  <tr>\n"
		                              "    <td align=center width=\"15%%\">%s</td>\n"
		                              "    <td align=center width=\"15%%\">%s</td>\n"
		                              "    <td align=center width=\"25%%\">%s</td>\n"
		                              "    <td align=center width=\"25%%\">%s</td>\n",
		                              mppp_ifname, mppp_protocol, mppp_ipaddr, mppp_remoteip);

		nBytesSent += boaWrite(wp, "    <td align=center width=\"20%%\">%s ",
							mppp_status );
#endif

		nBytesSent += boaWrite(wp, "	</td>\n");
		nBytesSent += boaWrite(wp, "  </tr>\n");

	}
	nBytesSent += boaWrite(wp, "</table>");
#ifdef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "</div></div>");
#endif
	return nBytesSent;
}
#else
int wanL2TPTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	return nBytesSent;
}
#endif //CONFIG_USER_L2TPD_L2TPD

#ifdef CONFIG_NET_IPIP
int wanIPIPTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	MIB_IPIP_T Entry;
	unsigned int entryNum, i;

#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<br>\n");
	nBytesSent += boaWrite(wp, "<table width=600 border=0>\n");
	nBytesSent += boaWrite(wp, "  <tr>\n");
	nBytesSent += boaWrite(wp, "    <td width=100%% colspan=5 bgcolor=\"#008000\">\n"
	                              "      <font color=\"#FFFFFF\" size=2><b>IPIP %s</b></font>\n"
	                              "    </td>\n", multilang(LANG_CONFIGURATION));
	nBytesSent += boaWrite(wp, "  </tr>\n");

	nBytesSent += boaWrite(wp, "  <tr bgcolor=\"#808080\">\n"
                              	      "    <td width=\"15%%\" align=center><font size=2><b>%s</b></font></td>\n"
	                              "    <td width=\"15%%\" align=center><font size=2><b>%s</b></font></td>\n"
	                              "    <td width=\"25%%\" align=center><font size=2><b>%s</b></font></td>\n"
	                              "    <td width=\"25%%\" align=center><font size=2><b>%s</b></font></td>\n"
	                              "    <td width=\"20%%\" align=center><font size=2><b>%s</b></font></td>\n"
	                              "  </tr>\n",
#else
	nBytesSent += boaWrite(wp, "<div class=\"column\">\n");
	nBytesSent += boaWrite(wp, "<div class=\"column_title\">\n");
	nBytesSent += boaWrite(wp, "   <div class=\"column_title_left\"></div>\n"
	                              "      <p>IPIP %s</p>\n"
	                              "    <div class=\"column_title_right\"></div>\n", multilang(LANG_CONFIGURATION));
	nBytesSent += boaWrite(wp, "  </div>\n");
	nBytesSent += boaWrite(wp, "  <div class=\"data_common data_vertical\">\n"
								"<table>"
	nBytesSent += boaWrite(wp, "  <tr>\n"
                              	      "    <th width=\"15%%\" align=center>%s</th>\n"
	                              "    <th width=\"15%%\" align=center>%s</th>\n"
	                              "    <th width=\"25%%\" align=center>%s</th>\n"
	                              "    <th width=\"25%%\" align=center>%s</th>\n"
	                              "    <th width=\"20%%\" align=center>%s</th>\n"
	                              "  </tr>\n",
#endif	                          
				multilang(LANG_INTERFACE), multilang(LANG_PROTOCOL), multilang(LANG_IP_ADDRESS),
				multilang(LANG_GATEWAY), multilang(LANG_STATUS));

	entryNum = mib_chain_total(MIB_IPIP_TBL);
	for (i=0; i<entryNum; i++)
	{
		char mppp_ifname[IFNAMSIZ];
		char mppp_protocol[10];
		char mppp_ipaddr[20];
		char mppp_remoteip[20];
		char *mppp_status;
		struct in_addr inAddr;
		int flags;

		if (!mib_chain_get(MIB_IPIP_TBL, i, (void *)&Entry))
		{
			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

		snprintf(mppp_ipaddr, 20, "%s", inet_ntoa(*((struct in_addr *)&Entry.saddr)));
		snprintf(mppp_remoteip, 20, "%s", inet_ntoa(*((struct in_addr *)&Entry.daddr)));
		ifGetName(Entry.ifIndex, mppp_ifname, sizeof(mppp_ifname));
		mppp_protocol[sizeof(mppp_protocol)-1]='\0';
		strncpy( mppp_protocol, "IPinIP" ,sizeof(mppp_protocol)-1);

		if (getInFlags( mppp_ifname, &flags) == 1)
		{
			if (flags & IFF_UP) {
				mppp_status = (char *)IF_UP;
			}else
				mppp_status = (char *)IF_DOWN;
		}else
			mppp_status = (char *)IF_DOWN;

#ifndef CONFIG_GENERAL_WEB
		nBytesSent += boaWrite(wp, "  <tr bgcolor=\"#EEEEEE\">\n"
		                              "    <td align=center width=\"15%%\"><font size=2>%s</td>\n"
		                              "    <td align=center width=\"15%%\"><font size=2>%s</td>\n"
		                              "    <td align=center width=\"25%%\"><font size=2>%s</td>\n"
		                              "    <td align=center width=\"25%%\"><font size=2>%s</td>\n",
		                              mppp_ifname, mppp_protocol, mppp_ipaddr, mppp_remoteip);

		nBytesSent += boaWrite(wp, "    <td align=center width=\"20%%\"><font size=2>%s ",
							mppp_status );
#else
		nBytesSent += boaWrite(wp, "  <tr>\n"
		                              "    <td align=center width=\"15%%\">%s</td>\n"
		                              "    <td align=center width=\"15%%\">%s</td>\n"
		                              "    <td align=center width=\"25%%\">%s</td>\n"
		                              "    <td align=center width=\"25%%\">%s</td>\n",
		                              mppp_ifname, mppp_protocol, mppp_ipaddr, mppp_remoteip);

		nBytesSent += boaWrite(wp, "    <td align=center width=\"20%%\">%s ",
							mppp_status );
#endif
		nBytesSent += boaWrite(wp, "	</td>\n");
		nBytesSent += boaWrite(wp, "  </tr>\n");

	}
	nBytesSent += boaWrite(wp, "</table>");
#ifdef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "</div></div>");
#endif
	return nBytesSent;
}
#else
int wanIPIPTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	return nBytesSent;
}
#endif //CONFIG_NET_IPIP

#ifdef CONFIG_00R0
int lanConfList(int eid, request * wp, int argc, char **argv)
{
	int i, nBytesSent=0, flags=0, portIdx;
	char ifname[IFNAMSIZ], status[16]={0}, speed[8]={0}, mode[8]={0};
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
	MIB_CE_SW_PORT_T Entry;
	int cwmp_Speed = SPEED_AUTO;
	int cwmp_Duplex = DUPLEX_TYPE_AUTO;
#if defined(CONFIG_COMMON_RT_API)
	rt_port_speed_t Speed;
	rt_port_duplex_t Duplex;
#else
	rtk_port_speed_t Speed;
	rtk_port_duplex_t Duplex;
#endif
#endif

	nBytesSent += boaWrite(wp, "<tr>"
			"<th width=\"12%%\" align=center>%s</th>\n"
			"<th width=\"12%%\" align=center>%s</th>\n"
			"<th width=\"12%%\" align=center>%s</th>\n"
			"<th width=\"12%%\" align=center>%s</th>\n",
			multilang(LANG_NAME), 
			multilang(LANG_STATUS), 
			multilang(LANG_SPEED),
			multilang(LANG_MODE));
	nBytesSent += boaWrite(wp, "<tr>\n");

	for (i=0; i<SW_LAN_PORT_NUM; i++) 
	{
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
		snprintf(ifname, sizeof(ifname), "%s", SW_LAN_PORT_IF[i]);
#else
		snprintf(ifname, sizeof(ifname), "eth%u", i);
#endif
		if (getInFlags(ifname, &flags) == 1) {
			if (flags & IFF_RUNNING)
				strcpy(status, "Up");
			else if(flags & IFF_UP)
				strcpy(status, "NoLink");
			else
				strcpy(status, "Down");
		} else
			strcpy(status, "Error");

#if defined(CONFIG_RTL_MULTI_LAN_DEV)
		mib_chain_get(MIB_SW_PORT_TBL, i, &Entry);
#ifdef CONFIG_LUNA
		if (getInFlags(ifname, &flags) == 1 && (flags & IFF_RUNNING) && (Entry.speed == SPEED_AUTO))
		{
			portIdx = rtk_port_get_lan_phyID(i);
#if defined(CONFIG_COMMON_RT_API)
			rt_port_speedDuplex_get(portIdx, &Speed, &Duplex);
			if (Speed == RT_PORT_SPEED_10M)
				cwmp_Speed = SPEED_10M;
			else if (Speed == RT_PORT_SPEED_100M)
				cwmp_Speed = SPEED_100M;
			else if (Speed == RT_PORT_SPEED_1000M)
				cwmp_Speed = SPEED_1000M;
			else
				cwmp_Speed = SPEED_AUTO;

			if (Duplex == RT_PORT_HALF_DUPLEX)
				cwmp_Duplex = DUPLEX_TYPE_HALF;
			else if (Duplex == RT_PORT_FULL_DUPLEX)
				cwmp_Duplex = DUPLEX_TYPE_FULL;
			else
				cwmp_Duplex = DUPLEX_TYPE_AUTO;
#else
			rtk_port_speedDuplex_get(portIdx, &Speed, &Duplex);
			if (Speed == PORT_SPEED_10M)
				cwmp_Speed = SPEED_10M;
			else if (Speed == PORT_SPEED_100M)
				cwmp_Speed = SPEED_100M;
			else if (Speed == PORT_SPEED_1000M)
				cwmp_Speed = SPEED_1000M;
			else
				cwmp_Speed = SPEED_AUTO;

			if (Duplex == PORT_HALF_DUPLEX)
				cwmp_Duplex = DUPLEX_TYPE_HALF;
			else if (Duplex == PORT_FULL_DUPLEX)
				cwmp_Duplex = DUPLEX_TYPE_FULL;
			else
				cwmp_Duplex = DUPLEX_TYPE_AUTO;
#endif
		}
		else
#endif
		{
			cwmp_Speed = Entry.speed;
			cwmp_Duplex = Entry.duplex;
		}

		switch (cwmp_Speed)
		{
			case SPEED_10M:
				strcpy(speed,"10");
				break;
			case SPEED_100M:
				strcpy(speed,"100");
				break;
			case SPEED_1000M:
				strcpy(speed,"1000");
				break;
			default:
				strcpy(speed,"Auto");
				break;
		}

		switch (cwmp_Duplex)
		{
			case DUPLEX_TYPE_HALF:
				strcpy(mode, "Half");
				break;
			case DUPLEX_TYPE_FULL:
				strcpy(mode, "Full");
				break;
			default:
				strcpy(mode, "Auto");
				break;
		}
#else
		strcpy(speed,"Auto");
		strcpy(mode,"Auto");
#endif

		nBytesSent += boaWrite(wp,
				"<td align=center width=\"12%%\">LAN%d</td>\n"
				"<td align=center width=\"12%%\">%s</td>\n"
				"<td align=center width=\"12%%\">%s</td>\n"
				"<td align=center width=\"12%%\">%s\n",
				i+1, 
				status,
				speed,
				mode);
		nBytesSent += boaWrite(wp, "</td></tr>\n");
	}

	return nBytesSent;
}
#endif

// Jenny, current status
#ifdef USE_DEONET
#define MAX_RETRY_COUNT 2
static int get_subnet_by_ifname(const char *ifname, char *subnet, int size)
{
	unsigned char	cmd[128];
	int				i = 0;

	if(ifname == NULL || subnet == NULL)
		return -1;

	memset(cmd, 0, 128);
	memset(subnet, 0, size);
	sprintf(cmd, 
			"ifconfig %s| grep 'inet addr:' |awk '{print $4}' | tr -d Mask:",
			ifname);
	for(i = 0; i < MAX_RETRY_COUNT; i++)
	{
		get_str_by_popen(cmd, subnet, size);
		if(subnet[0] != 0 &&  subnet[0] != '0')
			break;
	}

	return 0;
}

static int get_defaultGW(char *gateway, int size)
{
	int i = 0;

	memset(gateway, 0, size);

	for(i = 0; i < MAX_RETRY_COUNT; i++)
	{
		deo_defaultGW_get(gateway);
		if(gateway[0] != 0 && gateway[0] != '0')
			break;
	}

}

int wanConfList(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	int in_turn=0, ifcount=0;
	int i;
	int wan_mode;
	struct wstatus_info sEntry[MAX_VC_NUM+MAX_PPP_NUM];
	unsigned char WanName[MAX_NAME_LEN];
	unsigned char subnet[INET_ADDRSTRLEN] = {0, };
	unsigned char gateway[INET_ADDRSTRLEN] = {0, };

	ifcount = getWanStatus(sEntry, MAX_VC_NUM+MAX_PPP_NUM);
	in_turn = 0;
	for (i=0; i<ifcount; i++) {
		if ( sEntry[i].ipver == IPVER_IPV6 ) 
			continue;
		wan_mode = deo_wan_nat_mode_get();
		if (wan_mode == DEO_BRIDGE_MODE && !strcmp(sEntry[i].ifname, "nas0_0"))
			continue;
		if (wan_mode == DEO_NAT_MODE && !strcmp(sEntry[i].ifname, "br0"))
			continue;

		get_subnet_by_ifname(sEntry[i].ifname, subnet, INET_ADDRSTRLEN);
		get_defaultGW(gateway, INET_ADDRSTRLEN);

		if (in_turn == 0)
			nBytesSent += boaWrite(wp, "\n");
		if(strcmp(sEntry[i].WanName, ""))
			strcpy(WanName, sEntry[i].WanName);
		else
			strcpy(WanName, sEntry[i].ifDisplayName);
		in_turn ^= 0x01;
		nBytesSent += boaWrite(wp,
		"<tr>"
		"<th width=\"40%%\">%s</th>\n"
		"<td width=\"60%%\">%s</td>\n"
		"</tr>"
		"<tr>"
		"<th width=\"40%%\">%s</th>\n"
		"<td width=\"60%%\">%s</td>\n"
		"</tr>"
		"<tr>"
		"<th width=\"40%%\">%s</th>\n"
		"<td width=\"60%%\">%s</td>\n"
		"</tr>",
		multilang(LANG_IP_ADDRESS), 
		sEntry[i].ipAddr, 
		multilang(LANG_SUBNET_MASK),
		subnet,
		multilang(LANG_GATEWAY), 
		gateway);
	}	
	return nBytesSent;
}
#else
int wanConfList(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	int in_turn=0, ifcount=0;
	int i;
#ifdef CONFIG_00R0
	struct wstatus_info sEntry[MAX_VC_NUM+MAX_PPP_NUM]={0};
	char wanMac[30]={0};
#else
	struct wstatus_info sEntry[MAX_VC_NUM+MAX_PPP_NUM];
#endif
	unsigned char WanName[MAX_NAME_LEN];

	ifcount = getWanStatus(sEntry, MAX_VC_NUM+MAX_PPP_NUM);
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr bgcolor=\"#808080\">"
	"<td width=\"8%%\" align=center><font size=2><b>%s</b></font></td>\n"
#if defined(CONFIG_LUNA)
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td>\n"
#ifdef CONFIG_00R0
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td>\n"
#endif
#if (defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_RTK_WAN_CTYPE)) || !defined(CONFIG_RTK_DEV_AP)
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td>\n"
#endif
#else
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td>\n"
#endif
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"22%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"22%%\" align=center><font size=2><b>%s</b></font></td>\n"
	"<td width=\"12%%\" align=center><font size=2><b>%s</b></font></td></tr>\n",
#else
	nBytesSent += boaWrite(wp, "<tr>"
	"<th width=\"5%%\" align=center>%s</th>\n"
#if defined(CONFIG_LUNA)
	"<th width=\"10%%\" align=center>%s</th>\n"
#ifdef CONFIG_00R0
	"<th width=\"12%%\" align=center>%s</th>\n"
#endif
#if (defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_RTK_WAN_CTYPE)) || !defined(CONFIG_RTK_DEV_AP)
	"<th width=\"12%%\" align=center>%s</th>\n"
#endif
#else
	"<th width=\"12%%\" align=center>%s</th>\n"
	"<th width=\"12%%\" align=center>%s</th>\n"
#endif
	"<th width=\"12%%\" align=center>%s</th>\n"
#ifdef CONFIG_00R0
	"<th width=\"22%%\" align=center>%s / %s</th>\n"
#else
	"<th width=\"19%%\" align=center>%s</th>\n"
#endif
	"<th width=\"19%%\" align=center>%s</th>\n"
	"<th width=\"20%%\" align=center>%s</th></tr>\n",
#endif
	multilang(LANG_INTERFACE), 
#if defined(CONFIG_LUNA)	
	multilang(LANG_VLAN_ID), 
#ifdef CONFIG_00R0
	multilang(LANG_MAC),
#endif
#if (defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_RTK_WAN_CTYPE)) || !defined(CONFIG_RTK_DEV_AP)
	multilang(LANG_CONNECTION_TYPE),
#endif
#else	
	multilang(LANG_VPI_VCI), multilang(LANG_ENCAPSULATION),
#endif	
	multilang(LANG_PROTOCOL), multilang(LANG_IP_ADDRESS), 
#ifdef CONFIG_00R0
	multilang(LANG_SUBNET_MASK), 
#endif
	multilang(LANG_GATEWAY),
	multilang(LANG_STATUS));
	in_turn = 0;
	for (i=0; i<ifcount; i++) {
		if ( sEntry[i].ipver == IPVER_IPV6 ) 
			continue;
#ifndef CONFIG_GENERAL_WEB
		if (in_turn == 0)
			nBytesSent += boaWrite(wp, "<tr bgcolor=\"#EEEEEE\">\n");
		else
			nBytesSent += boaWrite(wp, "<tr bgcolor=\"#DDDDDD\">\n");
#else
		if (in_turn == 0)
			nBytesSent += boaWrite(wp, "<tr>\n");
#endif

#ifdef CONFIG_00R0
		//setup Mac
		char ifname[IFNAMSIZ] = {0};
		unsigned char hwaddr[MAC_ADDR_LEN];
#ifdef CONFIG_RTL_MULTI_ETH_WAN
		snprintf( ifname, IFNAMSIZ, "%s%d", ALIASNAME_MWNAS, ETH_INDEX(sEntry[i].ifIndex));
#else
		snprintf( ifname, IFNAMSIZ,  "%s%u", ALIASNAME_NAS, ETH_INDEX(sEntry[i].ifIndex) );
#endif
		getMacAddr(ifname, hwaddr);
		sprintf(wanMac,"%.02x:%.02x:%.02x:%.02x:%.02x:%.02x",hwaddr[0],hwaddr[1],hwaddr[2],hwaddr[3],hwaddr[4],hwaddr[5]);
#endif
		if(strcmp(sEntry[i].WanName, ""))
			strcpy(WanName, sEntry[i].WanName);
		else
			strcpy(WanName, sEntry[i].ifDisplayName);
		in_turn ^= 0x01;
		nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
		"<td align=center width=\"5%%\"><font size=2>%s</td>\n"
#if defined(CONFIG_LUNA)
		"<td align=center width=\"1%%\"><font size=2>%d</td>\n"
#ifdef CONFIG_00R0
		"<td align=center width=\"9%%\"><font size=2>%s</td>\n"
#endif
#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_RTK_WAN_CTYPE) || !defined(CONFIG_RTK_DEV_AP)
		"<td align=center width=\"9%%\"><font size=2>%s</td>\n"
#endif
#else
		"<td align=center width=\"5%%\"><font size=2>%s</td>\n"
		"<td align=center width=\"5%%\"><font size=2>%s</td>\n"
#endif		
		"<td align=center width=\"5%%\"><font size=2>%s</td>\n"
		"<td align=center width=\"10%%\"><font size=2>%s</td>\n"
		"<td align=center width=\"10%%\"><font size=2>%s</td>\n"
		"<td align=center width=\"23%%\"><font size=2>%s\n",
#else
		"<td align=center width=\"5%%\">%s</td>\n"
#if defined(CONFIG_LUNA)
		"<td align=center width=\"1%%\">%d</td>\n"
#ifdef CONFIG_00R0
		"<td align=center width=\"9%%\">%s</td>\n"
#endif
#if (defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_RTK_WAN_CTYPE)) || !defined(CONFIG_RTK_DEV_AP)
		"<td align=center width=\"9%%\">%s</td>\n"
#endif
#else
		"<td align=center width=\"5%%\">%s</td>\n"
		"<td align=center width=\"5%%\">%s</td>\n"
#endif		
		"<td align=center width=\"5%%\">%s</td>\n"
#ifdef CONFIG_00R0
		"<td align=center width=\"10%%\">%s / %s</td>\n"
#else
		"<td align=center width=\"10%%\">%s</td>\n"
#endif
		"<td align=center width=\"10%%\">%s</td>\n"
		"<td align=center width=\"23%%\">%s\n",
#endif
#ifdef CONFIG_00R0
		WanName,
#else
		//sEntry[i].ifDisplayName,
		sEntry[i].ifname,
#endif
#if defined(CONFIG_LUNA)
		sEntry[i].vid, 
#ifdef CONFIG_00R0
		wanMac, 
#endif

#if (defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_RTK_WAN_CTYPE)) || !defined(CONFIG_RTK_DEV_AP)
		sEntry[i].serviceType,
#endif
#else
		sEntry[i].vpivci, sEntry[i].encaps,
#endif		
		sEntry[i].protocol, sEntry[i].ipAddr, 
#ifdef CONFIG_00R0
		sEntry[i].mask, 
#endif
		sEntry[i].remoteIp, sEntry[i].strStatus);
		if (sEntry[i].cmode == CHANNEL_MODE_PPPOE || sEntry[i].cmode == CHANNEL_MODE_PPPOA) { // PPP mode
			if (sEntry[i].itf_state) {
#ifdef CONFIG_USER_PPPD
				unsigned int uptime, h=0, m=0, d=0;
				rtk_get_pppd_uptime(sEntry[i].ifname, &uptime);
				sprintf(sEntry[i].uptime, "%u", uptime);
				d = uptime/86400;
				h = (uptime/3600)%24;
				m = (uptime/60)%60;
				uptime = uptime%60;
				if (d)
					nBytesSent += boaWrite(wp, " %d day(s)", d);
				nBytesSent += boaWrite(wp, " %02d:%02d:%02d ", h, m, uptime);
#else
				nBytesSent += boaWrite(wp, " %s / %s ", sEntry[i].uptime, sEntry[i].totaluptime);
#endif
			}
			if (sEntry[i].link_state && !sEntry[i].pppDoD)
				if (sEntry[i].cmode == CHANNEL_MODE_PPPOE)
					nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
					//"<input type=\"submit\" id=\"%s\" value=\"%s\" name=\"submit%s\" onClick=\"disButton('%s')\">",
					"<input type=\"submit\" id=\"%s\" value=\"%s\" name=\"submit%s\" onClick=\"disButton('%s'); return on_submit(this)\">",
#else
					//"<input class=\"inner_btn\" type=\"submit\" id=\"%s\" value=\"%s\" name=\"submit%s\" onClick=\"disButton('%s')\">",			
					"<input class=\"inner_btn\" type=\"submit\" id=\"%s\" value=\"%s\" name=\"submit%s\" onClick=\"disButton('%s'); return on_submit(this)\">", 		
#endif
					sEntry[i].ifname, (sEntry[i].itf_state==1) ? multilang(LANG_DISCONNECT) : multilang(LANG_CONNECT),
					sEntry[i].ifname,sEntry[i].ifname);
				else
					nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
					//"<input type=\"submit\" value=\"%s\" name=\"submit%s\">"
					"<input type=\"submit\" value=\"%s\" name=\"submit%s\" onClick=\"return on_submit(this)\">"
#else
					//"<input class=\"inner_btn\" type=\"submit\" value=\"%s\" name=\"submit%s\">"
					"<input class=\"inner_btn\" type=\"submit\" value=\"%s\" name=\"submit%s\" onClick=\"return on_submit(this)\">"
#endif
					, (sEntry[i].itf_state==1) ?  multilang(LANG_DISCONNECT) : multilang(LANG_CONNECT),
					sEntry[i].ifname);
		}
		nBytesSent += boaWrite(wp, "</td></tr>\n");
	}	
	return nBytesSent;
}
#endif

int DHCPSrvStatus(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
#ifndef CONFIG_SFU
	char vChar = 0;
#ifdef CONFIG_USER_DHCP_SERVER
	if ( !mib_get_s( MIB_DHCP_MODE, (void *)&vChar, sizeof(vChar)) )
		return -1;
#endif
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp,"<tr bgcolor=\"#EEEEEE\">\n"
				"<td width=\"40%%\">\n"
				"<font size=2><b>DHCP %s</b></td>\n<td width=\"60%%\">\n"
    				"<font size=2>%s</td></tr>\n",
      				multilang(LANG_SERVER),DHCPV4_LAN_SERVER == vChar?multilang(LANG_ENABLED): multilang(LANG_DISABLED));
#else
	nBytesSent += boaWrite(wp,"<tr>\n"
				"<th width=\"40%%\">DHCP %s</th>\n"
				"<td width=\"60%%\">%s</td>"
				"</tr>\n",multilang(LANG_SERVER),DHCPV4_LAN_SERVER == vChar?multilang(LANG_ENABLED): multilang(LANG_DISABLED));
#endif
#endif

	return nBytesSent;
}

#ifdef CONFIG_DEV_xDSL
#define FM_DSL_VER \
"<tr bgcolor=\"#DDDDDD\">" \
"<td width=40%%><font size=2><b>%s</b></td>" \
"<td width=60%%><font size=2>%s</td>" \
"</tr>"

int DSLVer(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
	char s_ver[64];

	if(!(WAN_MODE & MODE_ATM) && !(WAN_MODE & MODE_PTM))
		return 0;

	getAdslInfo(ADSL_GET_VERSION, s_ver, 64);
	nBytesSent += boaWrite(wp, (char *)FM_DSL_VER, multilang(LANG_DSP_VERSION) , s_ver);

#ifdef CONFIG_USER_XDSL_SLAVE
	getAdslSlvInfo(ADSL_GET_VERSION, s_ver, 64);
	nBytesSent += boaWrite(wp, (char *)FM_DSL_VER, multilang(LANG_DSP_SLAVE_VERSION), s_ver);
#endif /*CONFIG_USER_XDSL_SLAVE*/

  return nBytesSent;
}

int DSLStatus(int eid, request * wp, int argc, char **argv)
{
#ifndef CONFIG_GENERAL_WEB
	const char FM_DSL_STATUS[] =
		"<tr>\n"
		"<td width=100%% colspan=\"2\" bgcolor=\"#008000\"><font color=\"#FFFFFF\" size=2><b>%s</b></font></td>\n"
		"</tr>\n"
		"<tr bgcolor=\"#EEEEEE\">\n"
		"<td width=40%%><font size=2><b>Operational Status</b></td>\n"
		"<td width=60%%><font size=2>%s</td>\n"
		"</tr>\n"
		"<tr bgcolor=\"#DDDDDD\">\n"
		"<td width=40%%><font size=2><b>Upstream Speed</b></td>\n"
		"<td width=60%%><font size=2>%s&nbsp;kbps&nbsp;</td>\n"
		"</tr>\n"
		"<tr bgcolor=\"#EEEEEE\">\n"
		"<td width=40%%><font size=2><b>Downstream Speed</b></td>\n"
		"<td width=60%%><font size=2>%s&nbsp;kbps&nbsp;</td>\n"
		"</tr>\n";
#else
	const char FM_DSL_STATUS[] =
		"<div class=\"column\">\n"
		"	<div class=\"column_title\">\n"
		"		<div class=\"column_title_left\"></div>\n"
		"		<p>%s</p>\n"
		"		<div class=\"column_title_right\"></div>\n"
		"	</div>\n"
		"	<div class=\"data_common\">"
		"	<table>"
		"		<tr>\n"
		"			<th width=40%%>Operational Status</th>\n"
		"			<td width=60%%>%s</td>\n"
		"		</tr>\n"
		"		<tr>\n"
		"			<th width=40%%>Upstream Speed</td>\n"
		"			<td width=60%%>%s&nbsp;kbps&nbsp;</td>\n"
		"		</tr>\n"
		"		<tr>\n"
		"			<th width=40%%>Downstream Speed</td>\n"
		"			<td width=60%%>%s&nbsp;kbps&nbsp;</td>\n"
		"		</tr>\n"
		"	</table>\n"
		"	</div>\n"
		"</div>\n";
#endif

	int nBytesSent = 0;
	char o_status[64], u_speed[16], d_speed[16];

	if(!(WAN_MODE & MODE_ATM) && !(WAN_MODE & MODE_PTM))
		return 0;

	getSYS2Str(SYS_DSL_OPSTATE, o_status);
	getAdslInfo(ADSL_GET_RATE_US, u_speed, 16);
	getAdslInfo(ADSL_GET_RATE_DS, d_speed, 16);

	nBytesSent += boaWrite(wp, (char *)FM_DSL_STATUS, "DSL", o_status, u_speed, d_speed);

#ifdef LOOP_LENGTH_METER
		char distance[16];
		const char FM_DSL_STATUS_LLM[] = "<tr bgcolor=\"#DDDDDD\">\n"
		"<td width=40%%><font size=2><b>Distance Measurement</b></td>\n"
		"<td width=60%%><font size=2>%s&nbsp;(m)&nbsp;</td>\n"
		"</tr>\n";

	getAdslInfo(ADSL_GET_LOOP_LENGTH_METER, distance, 16);
	nBytesSent += boaWrite(wp, (char *)FM_DSL_STATUS_LLM, distance);
#endif

#ifdef CONFIG_USER_XDSL_SLAVE
	getSYS2Str(SYS_DSL_SLV_OPSTATE, o_status);
	getAdslSlvInfo(ADSL_GET_RATE_US, u_speed, 16);
	getAdslSlvInfo(ADSL_GET_RATE_DS, d_speed, 16);

	nBytesSent += boaWrite(wp, (char *)FM_DSL_STATUS, "DSL Slave", o_status, u_speed, d_speed);
#endif /*CONFIG_USER_XDSL_SLAVE*/

  return nBytesSent;
}
#endif

//#ifdef CONFIG_MCAST_VLAN
struct iptv_mcast_info {
	unsigned int ifIndex;	
	char servName[MAX_NAME_LEN];
	unsigned short vlanId;
	unsigned char enable;
};

#define _PTS			", new it(\"%s\", \"%s\")"
#define _PTI			", new it(\"%s\", %d)"
#define __PME(entry,name)               #name, entry.name


int listWanName(int eid, request * wp, int argc, char **argv)
{
	char ifname[IFNAMSIZ];
	int i, entryNum;
	MIB_CE_ATM_VC_T entry;
	struct iptv_mcast_info mEntry[MAX_VC_NUM + MAX_PPP_NUM] = { 0 };
	
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, &entry)) {
			printf("get MIB chain error\n");
			return -1;
		}
		ifGetName(entry.ifIndex, ifname, sizeof(ifname));
		mEntry[i].servName[sizeof(mEntry[i].servName)-1]='\0';
		strncpy(mEntry[i].servName,ifname,sizeof(mEntry[i].servName)-1);
		mEntry[i].vlanId = entry.mVid;
		boaWrite(wp,
			 "links.push(new it_nr(\"%d\"" _PTS _PTI "));\n", i,
			 __PME(mEntry[i], servName), __PME(mEntry[i], vlanId));		
	}
	return 0;

}
//#endif

#ifdef CONFIG_00R0
int BootLoaderVersion(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
	char boot_sw[128], sw_crc1[64], sw_crc2[64], boot_crc[64]={0};
	getSYS2Str(SYS_BOOTLOADER_FWVERSION, boot_sw);
	getSYS2Str(SYS_BOOTLOADER_CRC, boot_crc);
	
	if(boot_crc[0]==0){
		boot_crc[sizeof(boot_crc)-1]='\0';
		strncpy(boot_crc ,"89c17bda",sizeof(boot_crc)-1);//Backward compatiable for old bootloader, 89c17bda is calcuated by loader_R3576_9602_nand_demo_dual_boot.img
	}

	getSYS2Str(SYS_FWVERSION_SUM_1, sw_crc1);
	getSYS2Str(SYS_FWVERSION_SUM_2, sw_crc2);
	
	nBytesSent += boaWrite(wp,"<tr bgcolor=\"#DDDDDD\">"
			"<td width=\"40%%\"><b>%s</b></td>\n"
			"<td width=\"60%%\">%s</td>\n</tr>\n"	
			"<tr bgcolor=\"#EEEEEE\">\n"
			"<td width=\"40%%\"><b>%s</b></td>\n"
			"<td width=\"60%%\">%s</td>\n</tr>\n"	
			"<tr bgcolor=\"#DDDDDD\">\n"			
			"<td width=\"40%%\"><b>%s</b></td>\n"
			"<td width=\"60%%\">%s</td>\n</tr>\n"
			"<tr bgcolor=\"#EEEEEE\">\n"
			"<td width=\"40%%\"><b>%s</b></td>\n"
			"<td width=\"60%%\">%s</td>\n</tr> \n",
			multilang(LANG_BOOTLOADER_VERSION), boot_sw,
			multilang(LANG_BOOTLOADER_VERSION_SUM), boot_crc,
			multilang(LANG_FIRMWARE_VERSION_SUM_1), sw_crc1,
			multilang(LANG_FIRMWARE_VERSION_SUM_2), sw_crc2);

	return nBytesSent;
}

int GPONSerialNumber(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
	// Use hex number to display first 4 bytes of serial number.
	char sn[64] = {0}, tmpBuf[128] = {0};
	mib_get_s(MIB_GPON_SN, (void *)sn, sizeof(sn));
	sprintf(tmpBuf, "%02X%02X%02X%02X%s", sn[0], sn[1], sn[2], sn[3], &sn[4]);
	
	nBytesSent += boaWrite(wp,"<tr bgcolor=\"#EEEEEE\">"
			"<td width=\"40%%\"><font size=2><b>%s</b></td>\n"
			"<td width=\"60%%\"><font size=2>%s</td>\n</tr>\n",
			multilang(LANG_SERIAL_NUMBER), tmpBuf);

	return nBytesSent;
}

#endif


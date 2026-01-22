/*
 * Web server handler routines for Access Limit
 *
 */

/*-- System inlcude files --*/
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <signal.h>
#ifdef EMBED
#include <linux/config.h>
#else
#include "../../../../include/linux/autoconf.h"
#endif
#include "options.h"

/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "multilang.h"

#ifdef __i386__
#define _LITTLE_ENDIAN_
#endif

#define MAX_CMD_NUM		20
#define TRUE			1
#define FALSE			0

struct dhcpOfferedAddrLease {
	u_int8_t chaddr[16];
	u_int32_t yiaddr;       /* network order */
	u_int32_t expires;      /* host order */
	u_int32_t interfaceType;
	u_int8_t hostName[64];
	u_int32_t hostType;
#ifdef GET_VENDOR_CLASS_ID
	u_int8_t vendor_class_id[64];
#endif
	u_int32_t active_time;
#ifdef _PRMT_X_CT_SUPPER_DHCP_LEASE_SC
	int category;
	int isCtcVendor;
	char szVendor[36];
	char szModel[36];
	char szUserClass[36];   //user class id in opt 77
	char szClientID[36];    //client id in opt 61
	char szFQDN[64];
#if defined(CONFIG_CU)
	char szVendorClassID[DHCP_VENDORCLASSID_LEN];
#endif
#endif
};

typedef struct connectHost
{
	char ssid[32];
	char ipAddress[46];
	char macAddress[32];
	u_int32_t timeExpired;
	u_int32_t portNum;
	char hostName[64];
} stConnectHost;

void accesslimit_del_iptables_set(MIB_ACCESSLIMIT_T *Entry, char *br_ip)
{
	int start1, start2, end1, end2;
	int start1_min, start2_min, end1_min, end2_min;
	int idx;
	char cmd[MAX_CMD_NUM][256];

	memset(cmd, 0, sizeof(cmd));

	if (Entry->accessBlock == TRUE)
	{
		idx = 0;
		sprintf(cmd[idx++], "iptables -t nat -D access_limit -p tcp --dport 80  -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34159 2>/dev/null",
				Entry->fromTimeHour, Entry->fromTimeMin, Entry->toTimeHour, Entry->toTimeMin, Entry->weekdays, Entry->accessMac, br_ip);
		sprintf(cmd[idx++], "iptables -t nat -D access_limit -p tcp --dport 80  -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s 2>/dev/null",
				Entry->fromTimeHour, Entry->fromTimeMin, Entry->toTimeHour, Entry->toTimeMin, Entry->weekdays, br_ip, Entry->accessMac, br_ip);

		sprintf(cmd[idx++], "iptables -t nat -D access_limit -p tcp --dport 443  -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34160 2>/dev/null",
				Entry->fromTimeHour, Entry->fromTimeMin, Entry->toTimeHour, Entry->toTimeMin, Entry->weekdays, Entry->accessMac, br_ip);
		sprintf(cmd[idx++], "iptables -t nat -D access_limit -p tcp --dport 443  -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s 2>/dev/null",
				Entry->fromTimeHour, Entry->fromTimeMin, Entry->toTimeHour, Entry->toTimeMin, Entry->weekdays, br_ip, Entry->accessMac, br_ip);

		sprintf(cmd[idx++], "iptables -t nat -D access_limit -p udp --dport 443  -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34160 2>/dev/null",
				Entry->fromTimeHour, Entry->fromTimeMin, Entry->toTimeHour, Entry->toTimeMin, Entry->weekdays, Entry->accessMac, br_ip);
		sprintf(cmd[idx++], "iptables -t nat -D access_limit -p udp --dport 443  -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s 2>/dev/null",
				Entry->fromTimeHour, Entry->fromTimeMin, Entry->toTimeHour, Entry->toTimeMin, Entry->weekdays, br_ip, Entry->accessMac, br_ip);
	}
	else
	{
		idx = 0;
		convert_hours(Entry->fromTimeHour, Entry->toTimeHour, &start1, &end1, &start2, &end2);
		convert_mins(Entry->fromTimeMin, Entry->toTimeMin, &end1, &start2, &start1_min, &end1_min, &start2_min, &end2_min);

		if (!((start1 == 0) && (end1 == 0) && (start1_min == 0) && (end1_min == 0)))
		{
			sprintf(cmd[idx++], "iptables -t nat -D access_limit -p tcp --dport 80 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34159 2>/dev/null",
					start1, start1_min, end1, end1_min, Entry->weekdays, Entry->accessMac, br_ip);
			sprintf(cmd[idx++], "iptables -t nat -D access_limit -p tcp --dport 80 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s 2>/dev/null",
					start1, start1_min, end1, end1_min, Entry->weekdays, br_ip, Entry->accessMac, br_ip);

			sprintf(cmd[idx++], "iptables -t nat -D access_limit -p tcp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34160 2>/dev/null",
					start1, start1_min, end1, end1_min, Entry->weekdays, Entry->accessMac, br_ip);
			sprintf(cmd[idx++], "iptables -t nat -D access_limit -p tcp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s 2>/dev/null",
					start1, start1_min, end1, end1_min, Entry->weekdays, br_ip, Entry->accessMac, br_ip);

			sprintf(cmd[idx++], "iptables -t nat -D access_limit -p udp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34160 2>/dev/null",
					start1, start1_min, end1, end1_min, Entry->weekdays, Entry->accessMac, br_ip);
			sprintf(cmd[idx++], "iptables -t nat -D access_limit -p udp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s 2>/dev/null",
					start1, start1_min, end1, end1_min, Entry->weekdays, br_ip, Entry->accessMac, br_ip);
		}

		if (!((start2 == 0) && (end2 == 0) && (start2_min == 0) && (end2_min == 0)))
		{
			sprintf(cmd[idx++], "iptables -t nat -D access_limit -p tcp --dport 80 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34159 2>/dev/null",
					start2, start2_min, end2, end2_min, Entry->weekdays, Entry->accessMac, br_ip);
			sprintf(cmd[idx++], "iptables -t nat -D access_limit -p tcp --dport 80 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s 2>/dev/null",
					start2, start2_min, end2, end2_min, Entry->weekdays, br_ip, Entry->accessMac, br_ip);

			sprintf(cmd[idx++], "iptables -t nat -D access_limit -p tcp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34160 2>/dev/null",
					start2, start2_min, end2, end2_min, Entry->weekdays, Entry->accessMac, br_ip);
			sprintf(cmd[idx++], "iptables -t nat -D access_limit -p tcp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s 2>/dev/null",
					start2, start2_min, end2, end2_min, Entry->weekdays, br_ip, Entry->accessMac, br_ip);

			sprintf(cmd[idx++], "iptables -t nat -D access_limit -p udp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -m mac --mac-source %s -j DNAT --to %s:34160 2>/dev/null",
					start2, start2_min, end2, end2_min, Entry->weekdays, Entry->accessMac, br_ip);
			sprintf(cmd[idx++], "iptables -t nat -D access_limit -p udp --dport 443 -m time --timestart %d:%d:00 --timestop %d:%d:59 --weekdays %s -d %s -m mac --mac-source %s -j DNAT --to %s 2>/dev/null",
					start2, start2_min, end2, end2_min, Entry->weekdays, br_ip, Entry->accessMac, br_ip);
		}
	}

	for (idx = 0; idx < MAX_CMD_NUM; idx++)
	{
		if (cmd[idx] != 0) 
			system(cmd[idx]);
	}

	return;
}

/*-- Macro declarations --*/
int getConnectHost(int nIdx, stConnectHost *pConnectHost)
{
	FILE *fp;
	char file[255] = "/var/udhcpd/udhcpd.leases";
	int i=0, nSize;
	unsigned long expires;
	unsigned long now;
	struct dhcpOfferedAddrLease lease;
	struct in_addr addr;

	if (!(fp  = fopen(file, "r")))
		return -1;

	now = time(0);

	while( (nSize =  fread(&lease, sizeof(lease), 1, fp)) != 0)
	{
		if(i == nIdx)
			break;
		if(i > nIdx)
		{
			fclose(fp);
			return -1;
		}
		i++;
	}
	fclose(fp);

	if (nSize == 0)
		return -1;

	addr.s_addr = lease.yiaddr;
	expires = ntohl(lease.expires);
	expires = (now < expires) ? expires : 0;

	sprintf(pConnectHost->ipAddress, "%s", inet_ntoa(addr));
	sprintf(pConnectHost->macAddress, "%02x:%02x:%02x:%02x:%02x:%02x",
			lease.chaddr[0], lease.chaddr[1], lease.chaddr[2],
			lease.chaddr[3], lease.chaddr[4], lease.chaddr[5]);
	sprintf(pConnectHost->hostName, "%s", lease.hostName);
	pConnectHost->timeExpired = expires;

	return 0;
}

int getConnectHostList(int eid, request * wp, int argc, char **argv)
{
	int nIdx = 0;
	stConnectHost connectHost;

	while((getConnectHost(nIdx++, &connectHost)) == 0)
	{
#ifdef __TODO__ /* sghong, 20230512 */
		if(connectHost.timeExpired == 0)
			continue;
#endif
		boaWrite(wp, "<option value='%s'>%s (%s)\n",
				connectHost.macAddress, connectHost.macAddress, connectHost.hostName);
	}

	return 0;
}

int hourTimeSet(int eid, request * wp, int argc, char **argv)
{
	int i;

	for(i=0; i<24; i++)
		boaWrite(wp, "<option value='%02d'>%02d\n", i, i);

	return 0;
}

int minTimeSet(int eid, request * wp, int argc, char **argv)
{
	int i;

	for(i=0; i<60; i++)
		boaWrite(wp, "<option value='%02d'>%02d\n", i, i);

	return 0;
}

int showCurrentTime(int eid, request * wp, int argc, char **argv)
{   
	int nYear;
	int nMon;
	int nDay;
	int nHour;
	int nMin;
	int nWeeks; 
	char strWeeks[30];
	FILE *fp;
	char buf[256] = {0,};
	char strCurrentTime[50];

	fp = popen("date  \"+%Y %m %d %H %M %w\"", "r");
	if (fp == NULL)
		return; 

	if (fgets(buf, 256, fp) != NULL)
	{
		sscanf(buf, "%d %d %d %d %d %d", &nYear, &nMon, &nDay,
				&nHour, &nMin, &nWeeks );

		switch(nWeeks)
		{
			case 0:
				sprintf(strWeeks, "일요일");
				break;
			case 1:
				sprintf(strWeeks, "월요일");
				break;
			case 2:
				sprintf(strWeeks, "화요일");
				break;
			case 3:
				sprintf(strWeeks, "수요일");
				break;
			case 4:
				sprintf(strWeeks, "목요일");
				break;
			case 5:
				sprintf(strWeeks, "금요일");
				break;
			case 6:
				sprintf(strWeeks, "토요일");
				break;
		}
	}

	sprintf(strCurrentTime, "%d년 %d월 %d일 %d:%d %s", nYear, nMon, nDay, nHour, nMin, strWeeks);

	pclose(fp);

	boaWrite(wp, "<P align='right'>현재시간:%s<input type='button' onclick='javascript: window.location.reload()' value='새로고침'></P>\n", strCurrentTime);

	return 0;
}

int delCurrentAccessList(int eid, request * wp, int argc, char **argv)
{   
	int  idx;
	int  entryNum;
	MIB_ACCESSLIMIT_T Entry;

	entryNum = mib_chain_total(MIB_ACCESSLIMIT_TBL);

	for (idx = 0; idx < entryNum; idx++)
	{
		if (!mib_chain_get(MIB_ACCESSLIMIT_TBL, idx, (void *)&Entry)) 
			break;

		boaWrite(wp, "<tr>\n");
		boaWrite(wp, "<td class='mt'>%d</td>\n", idx+1);
		boaWrite(wp, "<td class='mt'>%s</td>\n", Entry.accessDescription);
		boaWrite(wp, "<td class='mt'>%s</td>\n", Entry.accessMac);
		boaWrite(wp, "<td class='mt'>%s / %02d:%02d~%02d:%02d</td>\n",
				Entry.weekdays, Entry.fromTimeHour, Entry.fromTimeMin,
				Entry.toTimeHour, Entry.toTimeMin);
		boaWrite(wp, "<td class='mt'>%s</td>\n", (Entry.accessBlock == 0) ? "차단":"허용");
		boaWrite(wp, "<td align='center'><input type='checkbox' name='rml' value='%d'></td>\n", idx);
		boaWrite(wp, "</tr>\n");
	}

	return 0;
}

int showCurrentAccessList(int eid, request * wp, int argc, char **argv)
{   
	int  idx;
	int  entryNum;
	MIB_ACCESSLIMIT_T Entry;

	entryNum = mib_chain_total(MIB_ACCESSLIMIT_TBL);

	for (idx = 0; idx < entryNum; idx++)
	{
		if (!mib_chain_get(MIB_ACCESSLIMIT_TBL, idx, (void *)&Entry)) 
			break;

		boaWrite(wp, "<tr>\n");
		boaWrite(wp, "<td>%d</td>\n", idx+1);
		boaWrite(wp, "<td>%s</td>\n", Entry.accessDescription);
		if (Entry.used)
			boaWrite(wp, "<td align='center'><input type='checkbox' name='useselect%d' checked value='USE'></td>\n", idx);
		else
			boaWrite(wp, "<td align='center'><input type='checkbox' name='useselect%d' value='USE'></td>\n", idx);
		boaWrite(wp, "<td>%s</td>\n", Entry.accessMac);
		boaWrite(wp, "<td>%s / %02d:%02d~%02d:%02d</td>\n",
				Entry.weekdays, Entry.fromTimeHour, Entry.fromTimeMin,
				Entry.toTimeHour, Entry.toTimeMin);
		boaWrite(wp, "<td align='center'>%s</td>\n", (Entry.accessBlock == 1) ? "차단":"허용");
		boaWrite(wp, "<td align='center'><input type='checkbox' name='delselect%d' value='DEL'></td>\n", idx);
		boaWrite(wp, "</tr>\n");
	}

	return 0;
}

char *DAYS[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void formAccessLimit(request * wp, char *path, char *query)
{
	char *str, *submitUrl;
	char tmpBuf[100], strDays[30];
	char br_ip[16];
	char *hostname;
	char *startHour, *startMin, *toHour, *toMin;
	char *mac, *en_s, *everyday;
	char *days[7];
	int  i, idx;
	int  intVal;
	int  totalEntry;
	int  iptables_delset = 0;
	int  iptables_addset = 0;
	int  add_flag = 0;
	int  fromTime, toTime;
	char mode_str[10];
	char syslog_msg[120];
	int currentSntpState = 0;
	int syslogid = 0;
	MIB_ACCESSLIMIT_T Entry;
	static int sync_flag = 0;

	memset(strDays, 0, sizeof(strDays));
	memset(br_ip, 0, sizeof(br_ip));

	getIfAddr("br0", br_ip);

	str = boaGetVar(wp, "addAccessList", "");
	if (str[0]) {
		hostname = boaGetVar(wp, "hostName", "");
		startHour = boaGetVar(wp, "setStartTimeHour", "");
		startMin = boaGetVar(wp, "setStartTimeMin", "");
		toHour = boaGetVar(wp, "setEndTimeHour", "");
		toMin = boaGetVar(wp, "setEndTimeMin", "");
		mac = boaGetVar(wp, "hostAddress", "");
		en_s = boaGetVar(wp, "selectAllow", "");

		everyday = boaGetVar(wp, "cbSelDay7", "");
		if (everyday[0])
		{
			strcpy(strDays, "Sun,Mon,Tue,Wed,Thu,Fri,Sat");
		}
		else
		{
			for (i = 0; i < 7; i++)
			{
				sprintf(tmpBuf, "cbSelDay%d", i);
				days[i] = boaGetVar(wp, tmpBuf, "");

				if (days[i][0])
				{
					if (strDays[0] == 0)
						strcpy(strDays, DAYS[i]);
					else
						sprintf(strDays, "%s,%s", strDays, DAYS[i]);
				}
			}
		}

		totalEntry = mib_chain_total(MIB_ACCESSLIMIT_TBL); /* get chain record size */
		if (totalEntry == 0)
		{
			if(!mib_get(MIB_NTP_RUNNING_STATUS, &currentSntpState))
			{
				tmpBuf[sizeof(tmpBuf)-1]='\0';
				strncpy(tmpBuf, errGetEntry,sizeof(tmpBuf)-1);
				goto setErr_al;
			}

			if (currentSntpState == 2)
			{
				if (sync_flag == 0)
				{
					syslog2(LOG_DM_SNTP, LOG_DM_INTERNET_LIMIT_NTP_SYNC_OK, 
							"[INTERNET LIMIT] NTP Sync Ok, Start Internet Limitation");
					sync_flag = 1;
				}
			}
			else 
			{
				sync_flag = 0;
			}
		}

		memset(&Entry, 0, sizeof(MIB_ACCESSLIMIT_T));

		strcpy(Entry.accessDescription, hostname);
		strcpy(Entry.accessMac, mac);
		strcpy(Entry.weekdays, strDays);

		Entry.fromTimeHour = atoi(startHour);
		Entry.fromTimeMin = atoi(startMin);
		Entry.toTimeHour = atoi(toHour);
		Entry.toTimeMin = atoi(toMin);
		Entry.accessBlock = atoi(en_s);

		intVal = mib_chain_add(MIB_ACCESSLIMIT_TBL, (unsigned char*)&Entry);
		if (intVal == 0) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strAddListErr,sizeof(tmpBuf)-1);
			goto setErr_al;
		}
		else if (intVal == -1) {
			tmpBuf[sizeof(tmpBuf)-1]='\0';
			strncpy(tmpBuf, strTableFull,sizeof(tmpBuf)-1);
			goto setErr_al;
		}

#ifdef USE_DEONET /* sghong, 20240618 */
		if (Entry.accessBlock == 0) 
		{
			strcpy(mode_str, "Allow");
			syslogid = LOG_DM_INTERNET_LIMIT_ALLOW;
		}
		else
		{
			strcpy(mode_str, "Restrict");
			syslogid = LOG_DM_INTERNET_LIMIT_RESTRICT;
		}

		sprintf(syslog_msg, "[INTERNET LIMIT] Connect %s CPE Start %s (%02d:%02d ~ %02d:%02d, %s)", 
				mode_str, 
				Entry.accessMac,
				Entry.fromTimeHour, Entry.fromTimeMin,
				Entry.toTimeHour, Entry.toTimeMin,
				Entry.weekdays, strDays);

		syslog2(LOG_DM_SNTP, syslogid, syslog_msg);
#endif

		add_flag = 1;
	}

	str = boaGetVar(wp, "usedelAccessList", "");
	if (str[0]) 
	{
		int deleted = 0;
		int used = 0;
		char *strVal;

		totalEntry = mib_chain_total(MIB_ACCESSLIMIT_TBL); /* get chain record size */

		for (i = 0; i < totalEntry; i++)
		{
			idx = totalEntry-i-1;
			snprintf(tmpBuf, 20, "delselect%d", idx);
			strVal = boaGetVar(wp, tmpBuf, "");
			if ( !gstrcmp(strVal, "DEL") ) 
			{
				/* get the specified chain record */
				if (!mib_chain_get(MIB_ACCESSLIMIT_TBL, idx, (void *)&Entry)) 
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, errGetEntry,sizeof(tmpBuf)-1);
					goto setErr_al;
				}

				// delete from chain record
				if(mib_chain_delete(MIB_ACCESSLIMIT_TBL, idx) != 1) 
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, Tdelete_chain_error,sizeof(tmpBuf)-1);
					goto setErr_al;
				}
				iptables_delset = 1;
#ifdef USE_DEONET /* sghong, 20240618 */
				{
					if (Entry.accessBlock == 0) 
					{
						strcpy(mode_str, "Allow");
						syslogid = LOG_DM_INTERNET_LIMIT_ALLOW;
					}
					else
					{
						strcpy(mode_str, "Restrict");
						syslogid = LOG_DM_INTERNET_LIMIT_RESTRICT;
					}

					sprintf(syslog_msg, "[INTERNET LIMIT] Connect %s CPE Stop %s (%02d:%02d ~ %02d:%02d, %s)", 
							mode_str, 
							Entry.accessMac,
							Entry.fromTimeHour, Entry.fromTimeMin,
							Entry.toTimeHour, Entry.toTimeMin,
							Entry.weekdays, strDays);

					syslog2(LOG_DM_SNTP, syslogid, syslog_msg);
				}
#endif
			}

			if (iptables_delset)
				continue;

			snprintf(tmpBuf, 20, "useselect%d", idx);
			strVal = boaGetVar(wp, tmpBuf, "");
			if ( !gstrcmp(strVal, "USE") ) 
			{
				/* get the specified chain record */
				if (!mib_chain_get(MIB_ACCESSLIMIT_TBL, idx, (void *)&Entry)) 
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, errGetEntry,sizeof(tmpBuf)-1);
					goto setErr_al;
				}

				if (Entry.used == 0)
				{
					Entry.used = 1;

					/* update from chain record */
					if(mib_chain_update(MIB_ACCESSLIMIT_TBL, &Entry, idx) != 1) 
					{
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, Tupdate_chain_error,sizeof(tmpBuf)-1);
						goto setErr_al;
					}

					iptables_addset = 1;
				}
			}
			else
			{
				/* get the specified chain record */
				if (!mib_chain_get(MIB_ACCESSLIMIT_TBL, idx, (void *)&Entry)) 
				{
					tmpBuf[sizeof(tmpBuf)-1]='\0';
					strncpy(tmpBuf, errGetEntry,sizeof(tmpBuf)-1);
					goto setErr_al;
				}

				if (Entry.used == 1)
				{
					Entry.used = 0;

					/* update from chain record */
					if(mib_chain_update(MIB_ACCESSLIMIT_TBL, &Entry, idx) != 1) 
					{
						tmpBuf[sizeof(tmpBuf)-1]='\0';
						strncpy(tmpBuf, Tupdate_chain_error,sizeof(tmpBuf)-1);
						goto setErr_al;
					}
					iptables_addset = 1;
				}
			}
		}
	}

	Commit();

#ifdef USE_DEONET /* sghong, 20240119 */
	{
		int ntp_status;

		if ((iptables_delset) || (iptables_addset))
		{
			deo_accesslimit_iptables_flush();
			deo_accesslimit_set();
		}

		mib_get_s(MIB_NTP_RUNNING_STATUS, &ntp_status, 4);
		if (ntp_status != 2)
			deo_accesslimit_iptables_flush();
	}
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
	{
		if (add_flag == 1)
			boaRedirect(wp, submitUrl);
		else
			OK_MSG(submitUrl);
	}

	return;

setErr_al:
	ERR_MSG(tmpBuf);

	return;
}

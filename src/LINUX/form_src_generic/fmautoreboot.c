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

#define MAX_CMD_NUM		12
#define TRUE			1
#define FALSE			0

extern char *DAYS[7];


int startHourTimeSet(int eid, request * wp, int argc, char **argv)
{
	int i;

	for(i=0; i<24; i++)
		boaWrite(wp, "<option value='%02d'>%02d\n", i, i);

	return 0;
}

int startMinTimeSet(int eid, request * wp, int argc, char **argv)
{
	int i;

	for(i=0; i<60; i++)
		boaWrite(wp, "<option value='%02d'>%02d\n", i, i);

	return 0;
}

int endHourTimeSet(int eid, request * wp, int argc, char **argv)
{
	int i;

	for(i=0; i<24; i++)
		boaWrite(wp, "<option value='%02d'>%02d\n", i, i);

	return 0;
}

int endMinTimeSet(int eid, request * wp, int argc, char **argv)
{
	int i;

	for(i=0; i<60; i++)
		boaWrite(wp, "<option value='%02d'>%02d\n", i, i);

	return 0;
}

int ShowAutoRebootTargetTime(int eid, request * wp, int argc, char **argv)
{
	char targetTime[128];

	mib_get_s(MIB_AUTOREBOOT_TARGET_TIME, targetTime, sizeof(targetTime));

	boaWrite(wp, "%s", targetTime);

	return 0;
}

int ShowAutoSetting(int eid, request * wp, int argc, char **argv)
{
	unsigned char autoConfig;

	mib_get_s(MIB_ENABLE_AUTOREBOOT, &autoConfig, 1);

	if (autoConfig == 1)
		boaWrite(wp, "<td width=\"30%%\">%s</td><td><input type=\"checkbox\" name=\"cbEnblAutoReboot\"\n"
			" maxlength=\"2\" checked onClick=\"setEnblAutoReboot()\"</td>",
			"자동 Reboot 사용");
	else
		boaWrite(wp, "<td width=\"30%%\">%s</th><td><input type=\"checkbox\" name=\"cbEnblAutoReboot\"\n"
			" onClick=\"setEnblAutoReboot()\"</td>",
			"자동 Reboot 사용");

	return 0;
}

int ShowRebootTargetTime(int eid, request * wp, int argc, char **argv)
{
	unsigned char autoConfig, ntp_en;
	int ntp_issync;
	char target_time[128];

	mib_get_s(MIB_ENABLE_AUTOREBOOT, &autoConfig, 1);
	mib_get_s(MIB_NTP_ENABLED, &ntp_en, 1);
	ntp_issync = deo_ntp_is_sync();

	memset(target_time, 0, sizeof(target_time));
	mib_get_s(MIB_AUTOREBOOT_TARGET_TIME, target_time, 128);

	if ((autoConfig == 1) && (ntp_en == 1) && (ntp_issync == 1))
		boaWrite(wp, "<th width=\"30%%\">%s</th><td>%s</td>\n", "Reboot 예정시간", target_time);
	else
		boaWrite(wp, "<th width=\"30%%\">%s</th><td>%s</td>\n", "Reboot 예정시간", "");

	return 0;
}

int ShowManualSetting(int eid, request * wp, int argc, char **argv)
{
	unsigned char manualConfig;

	mib_get_s(MIB_AUTOREBOOT_ENABLE_STATIC_CONFIG, &manualConfig, 1);

	if (manualConfig == 1)
		boaWrite(wp, "<td>%s</td><td><input type=\"checkbox\" name=\"cbEnblAutoRebootStaticConfig\"\n"
			" maxlength=\"2\" checked </td>",
			"수동 설정");
	else
		boaWrite(wp, "<td>%s</th><td><input type=\"checkbox\" name=\"cbEnblAutoRebootStaticConfig\"\n"
			" </td>",
			"수동 설정");

	return 0;
}

int ShowAutoWanPortIdleValue(int eid, request * wp, int argc, char **argv)
{
	unsigned char value;

	mib_get_s(MIB_AUTOREBOOT_WAN_PORT_IDLE, &value, 1);

	if (value == 1)
		boaWrite(wp, "<td>%s</td><td><input type=\"checkbox\" name=\"cbAutoWanPortIdle\"\n"
			" maxlength=\"2\" checked </td>",
			"평균 데이터량 확인");
	else
		boaWrite(wp, "<td>%s</th><td><input type=\"checkbox\" name=\"cbAutoWanPortIdle\"\n"
			" </td>",
			"평균 데이터량 확인");

	return 0;
}

char *HDAYS[7] = { "일", "월", "화", "수", "목", "금", "토"};

int ShowAutoRebootDay(int eid, request * wp, int argc, char **argv)
{
	int i;
	unsigned int rebootDay;
	unsigned char days[7];
	char cbName[10];

	mib_get_s(MIB_AUTOREBOOT_DAY_0, &days[0], 1);
	mib_get_s(MIB_AUTOREBOOT_DAY_1, &days[1], 1);
	mib_get_s(MIB_AUTOREBOOT_DAY_2, &days[2], 1);
	mib_get_s(MIB_AUTOREBOOT_DAY_3, &days[3], 1);
	mib_get_s(MIB_AUTOREBOOT_DAY_4, &days[4], 1);
	mib_get_s(MIB_AUTOREBOOT_DAY_5, &days[5], 1);
	mib_get_s(MIB_AUTOREBOOT_DAY_6, &days[6], 1);
	mib_get_s(MIB_AUTOREBOOT_DAY, &rebootDay, 4);

	boaWrite(wp, "<td>요일</td>\n");
	boaWrite(wp, "<td>\n");

	for (i = 0; i < 7; i++)
	{
		sprintf(cbName, "cbSelDay%d", i);

		if (days[i] == 1)
			boaWrite(wp, "<input type=\"checkbox\" name=\"%s\" checked onChange=\"checkAutoRebootDay()\">%s &nbsp;&nbsp;\n", cbName, HDAYS[i]);
		else
			boaWrite(wp, "<input type=\"checkbox\" name=\"%s\" onChange=\"checkAutoRebootDay()\">%s &nbsp;&nbsp;\n", cbName, HDAYS[i]);
	}

	boaWrite(wp, "</td>\n");

	return 0;
}

void deo_autoRebootDayServerGet(char *strDay, unsigned int rebootDay)
{
	int i;
	char tmpbuf[128];

	memset(tmpbuf, 0, sizeof(tmpbuf));

	for (i = 6; i >= 0; i--)
	{
		if (rebootDay & (0x01 << (i * 4)))
		{
			switch(i)
			{
				case 6:
					strcpy(tmpbuf, "일 ");
					sprintf(strDay, "%s%s", strDay, tmpbuf);
					break;
				case 5:
					strcpy(tmpbuf, "월 ");
					sprintf(strDay, "%s%s", strDay, tmpbuf);
					break;
				case 4:
					strcpy(tmpbuf, "화 ");
					sprintf(strDay, "%s%s", strDay, tmpbuf);
					break;
				case 3:
					strcpy(tmpbuf, "수 ");
					sprintf(strDay, "%s%s", strDay, tmpbuf);
					break;
				case 2:
					strcpy(tmpbuf, "목 ");
					sprintf(strDay, "%s%s", strDay, tmpbuf);
					break;
				case 1:
					strcpy(tmpbuf, "금 ");
					sprintf(strDay, "%s%s", strDay, tmpbuf);
					break;
				case 0:
					strcpy(tmpbuf, "토 ");
					sprintf(strDay, "%s%s", strDay, tmpbuf);
					break;
			}
		}
	}

	return;
}

int showCurrentAutoRebootConfigInfo(int eid, request * wp, int argc, char **argv)
{   
	char strDay[128];
	MIB_AUTOREBOOT_T Entry;

	memset(strDay, 0, sizeof(strDay));
	memset(&Entry, 0, sizeof(MIB_AUTOREBOOT_T));

	mib_get_s(MIB_AUTOREBOOT_LDAP_CONFIG_SERVER, &Entry.ldapConfigServer, 1);
	mib_get_s(MIB_AUTOREBOOT_LDAP_FILE_VERSION_SERVER, Entry.ldapFileVersionServer, 128);
	mib_get_s(MIB_AUTOREBOOT_ON_IDEL_SERVER, &Entry.autoRebootOnIdelServer, 1);
	mib_get_s(MIB_AUTOREBOOT_UPTIME_SERVER, &Entry.autoRebootUpTimeServer, sizeof(int));
	mib_get_s(MIB_AUTOREBOOT_HR_STARTHOUR_SERVER, &Entry.autoRebootHRangeStartHourServer, sizeof(int));
	mib_get_s(MIB_AUTOREBOOT_HR_STARTMIN_SERVER, &Entry.autoRebootHRangeStartMinServer, sizeof(int));
	mib_get_s(MIB_AUTOREBOOT_HR_ENDHOUR_SERVER, &Entry.autoRebootHRangeEndHourServer, sizeof(int));
	mib_get_s(MIB_AUTOREBOOT_HR_ENDMIN_SERVER, &Entry.autoRebootHRangeEndMinServer, sizeof(int));
	mib_get_s(MIB_AUTOREBOOT_DAY_SERVER, &Entry.autoRebootDayServer, sizeof(int));
	mib_get_s(MIB_AUTOREBOOT_WAN_PORT_IDLE_SERVER, &Entry.autoRebootWanPortIdleServer, 1);
	mib_get_s(MIB_AUTOREBOOT_AVR_DATA_SERVER, &Entry.autoRebootAvrDataServer, sizeof(int));
	mib_get_s(MIB_AUTOREBOOT_AVR_DATA_EPG_SERVER, &Entry.autoRebootAvrDataEPGServer, sizeof(int));

#if 0
	Entry.enblAutoReboot;
	Entry.autoRebootTargetTime[128];
	Entry.ldapConfigServer;
	Entry.ldapFileVersionServer[128];
	Entry.autoRebootOnIdelServer;
	Entry.autoRebootUpTimeServer;
	Entry.autoRebootWanPortIdleServer;
	Entry.autoRebootHRangeStartHourServer;
	Entry.autoRebootHRangeStartMinServer;
	Entry.autoRebootHRangeEndHourServer;
	Entry.autoRebootHRangeEndMinServer;
	Entry.autoRebootDayServer;
	Entry.autoRebootAvrDataServer;
	Entry.autoRebootAvrDataEPGServer;
	Entry.enblAutoRebootStaticConfig;
	Entry.autoRebootUpTime;
	Entry.autoWanPortIdle;
	Entry.autoRebootHRangeStartHour;
	Entry.autoRebootHRangeStartMin;
	Entry.autoRebootHRangeEndHour;
	Entry.autoRebootHRangeEndMin;
	Entry.autoRebootDay;
	Entry.autoRebootAvrData;
	Entry.autoRebootAvrDataEPG;
#endif

	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<td width=\"30%%\">Ldap 설정 유효 확인</td>\n");

	if (Entry.ldapConfigServer == 1)
		boaWrite(wp, "<td>success</td>\n");
	else
		boaWrite(wp, "<td>fail</td>\n");
	boaWrite(wp, "</tr>\n");

	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<td width=\"30%%\">Ldap File Version</td>\n");
	boaWrite(wp, "<td>%s</td>\n", Entry.ldapFileVersionServer);
	boaWrite(wp, "</tr>\n");

	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<td width=\"30%%\">자동 Reboot On Idle</td>\n");
	if (Entry.autoRebootOnIdelServer == 1)
		boaWrite(wp, "<td>ON</td>\n");
	else
		boaWrite(wp, "<td>OFF</td>\n");
	boaWrite(wp, "</tr>\n");

	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<td width=\"30%%\">자동 Reboot Up Time</td>\n");
	boaWrite(wp, "<td>%d</td>\n", Entry.autoRebootUpTimeServer);
	boaWrite(wp, "</tr>\n");

	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<td width=\"30%%\">자동 Reboot 시간(00~23)</td>\n");
	boaWrite(wp, "<td>%02d:%02d ~ %02d:%02d</td>\n", 
			Entry.autoRebootHRangeStartHourServer, Entry.autoRebootHRangeStartMinServer,
			Entry.autoRebootHRangeEndHourServer, Entry.autoRebootHRangeEndMinServer);

	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<td width=\"30%%\">요일</td>\n");
	deo_autoRebootDayServerGet(strDay, Entry.autoRebootDayServer);
	boaWrite(wp, "<td>%s</td>\n", strDay);
	boaWrite(wp, "</tr>\n");

	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<td width=\"30%%\">평균 데이터량 확인</td>\n");
	if (Entry.autoRebootWanPortIdleServer == 1)
		boaWrite(wp, "<td>Yes</td>\n");
	else
		boaWrite(wp, "<td>No</td>\n");
	boaWrite(wp, "</tr>\n");

	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<td width=\"30%%\">평균 데이터량</td>\n");
	boaWrite(wp, "<td>%d</td>\n", Entry.autoRebootAvrDataServer);
	boaWrite(wp, "</tr>\n");

	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "<td width=\"30%%\">평균 데이터량(EPG)</td>\n");
	boaWrite(wp, "<td>%d</td>\n", Entry.autoRebootAvrDataEPGServer);
	boaWrite(wp, "</tr>\n");

	return 0;
}


void formAutoReboot(request * wp, char *path, char *query)
{
	char *str, *submitUrl;
	char tmpBuf[100], strDays[30];
	char *startHour, *startMin, *toHour, *toMin;
	char *days[7];
	char *idle, *avrData, *avrDataEPG, *rebootDay;
	char *ebManual, *rebootUpTime, *autoEn;
	int  i, idx;
	int  intVal;
	unsigned char value;
	MIB_AUTOREBOOT_T Entry;

	memset(strDays, 0, sizeof(strDays));
	memset(&Entry, 0, sizeof(MIB_AUTOREBOOT_T));

	str = boaGetVar(wp, "addAutoRebootStaticConfig", "");
	if (str[0]) {

		autoEn = boaGetVar(wp, "cbEnblAutoReboot", "");

		ebManual = boaGetVar(wp, "cbEnblAutoRebootStaticConfig", "");
		rebootUpTime = boaGetVar(wp, "txAutoRebootUpTime", "");

		startHour = boaGetVar(wp, "arSetStartTimeHour", "");
		startMin = boaGetVar(wp, "arSetStartTimeMin", "");
		toHour = boaGetVar(wp, "arSetEndTimeHour", "");
		toMin = boaGetVar(wp, "arSetEndTimeMin", "");

		idle = boaGetVar(wp, "cbAutoWanPortIdle", "");
		rebootDay = boaGetVar(wp, "cbAutoRebootDay", "");
		avrData = boaGetVar(wp, "txAutoRebootAvrData", "");
		avrDataEPG = boaGetVar(wp, "txAutoRebootAvrDataEPG", "");

		for (i = 0; i < 7; i++)
		{
			sprintf(tmpBuf, "cbSelDay%d", i);
			days[i] = boaGetVar(wp, tmpBuf, "");

			if (!strcmp(days[i], "on"))
			{
				value = 1;
				Entry.autoRebootDay |= 0x01 << ((6-i)*4);

				if (i == 0) mib_set(MIB_AUTOREBOOT_DAY_0, &value);
				else if (i == 1) mib_set(MIB_AUTOREBOOT_DAY_1, &value);
				else if (i == 2) mib_set(MIB_AUTOREBOOT_DAY_2, &value);
				else if (i == 3) mib_set(MIB_AUTOREBOOT_DAY_3, &value);
				else if (i == 4) mib_set(MIB_AUTOREBOOT_DAY_4, &value);
				else if (i == 5) mib_set(MIB_AUTOREBOOT_DAY_5, &value);
				else if (i == 6) mib_set(MIB_AUTOREBOOT_DAY_6, &value);
			}
			else
			{
				value = 0;

				if (i == 0) mib_set(MIB_AUTOREBOOT_DAY_0, &value);
				else if (i == 1) mib_set(MIB_AUTOREBOOT_DAY_1, &value);
				else if (i == 2) mib_set(MIB_AUTOREBOOT_DAY_2, &value);
				else if (i == 3) mib_set(MIB_AUTOREBOOT_DAY_3, &value);
				else if (i == 4) mib_set(MIB_AUTOREBOOT_DAY_4, &value);
				else if (i == 5) mib_set(MIB_AUTOREBOOT_DAY_5, &value);
				else if (i == 6) mib_set(MIB_AUTOREBOOT_DAY_6, &value);
			}

		}

		if ((!strcmp(autoEn, "ON")) || (!strcmp(autoEn, "on")))
			Entry.enblAutoReboot = 1;
		else
			Entry.enblAutoReboot = 0;

		if ((!strcmp(ebManual, "ON")) || (!strcmp(ebManual, "on")))
			Entry.enblAutoRebootStaticConfig = 1;
		else
			Entry.enblAutoRebootStaticConfig = 0;

		Entry.autoRebootUpTime = atoi(rebootUpTime); 
		if ((!strcmp(idle, "ON")) || (!strcmp(idle, "on")))
			Entry.autoWanPortIdle = 1;
		else
			Entry.autoWanPortIdle = 0;

		Entry.autoRebootHRangeStartHour = atoi(startHour); 
		Entry.autoRebootHRangeStartMin = atoi(startMin);
		Entry.autoRebootHRangeEndHour = atoi(toHour); 
		Entry.autoRebootHRangeEndMin = atoi(toMin);

		Entry.autoRebootAvrData = atoi(avrData); 
		Entry.autoRebootAvrDataEPG = atoi(avrDataEPG); 

		mib_set(MIB_ENABLE_AUTOREBOOT, &Entry.enblAutoReboot);

		mib_set(MIB_AUTOREBOOT_ENABLE_STATIC_CONFIG, &Entry.enblAutoRebootStaticConfig);
		mib_set(MIB_AUTOREBOOT_UPTIME, &Entry.autoRebootUpTime);
		mib_set(MIB_AUTOREBOOT_WAN_PORT_IDLE, &Entry.autoWanPortIdle);

		mib_set(MIB_AUTOREBOOT_HR_STARTHOUR, &Entry.autoRebootHRangeStartHour);
		mib_set(MIB_AUTOREBOOT_HR_STARTMIN, &Entry.autoRebootHRangeStartMin);
		mib_set(MIB_AUTOREBOOT_HR_ENDHOUR, &Entry.autoRebootHRangeEndHour);
		mib_set(MIB_AUTOREBOOT_HR_ENDMIN, &Entry.autoRebootHRangeEndMin);

		mib_set(MIB_AUTOREBOOT_DAY, &Entry.autoRebootDay);
		mib_set(MIB_AUTOREBOOT_AVR_DATA, &Entry.autoRebootAvrData);
		mib_set(MIB_AUTOREBOOT_AVR_DATA_EPG, &Entry.autoRebootAvrDataEPG);
	}

	Commit();

	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
	{
		boaRedirect(wp, submitUrl);

		/* restart autoreboot */
		stopAutoReboot();
		startAutoReboot();
	}

#if 0 /* sghong, 20240628 */
	{
		char buf[120] = {0, };
		char days[30] = {0, };
		char day[7][4] = {{0,0}, };

		if ((Entry.autoRebootDay >> 24) & 0x1)
			strcpy(day[0], "Sun");
		if ((Entry.autoRebootDay >> 20) & 0x1)
			strcpy(day[1], "Mon");
		if ((Entry.autoRebootDay >> 16) & 0x1)
			strcpy(day[2], "Tue");
		if ((Entry.autoRebootDay >> 12) & 0x1)
			strcpy(day[3], "Wed");
		if ((Entry.autoRebootDay >> 8) & 0x1)
			strcpy(day[4], "Thu");
		if ((Entry.autoRebootDay >> 4) & 0x1)
			strcpy(day[5], "Fri");
		if (Entry.autoRebootDay & 0x1)
			strcpy(day[6], "Sat");

		for (i = 0; i < 7; i++)
		{
			if (day[i][0] != 0)
			{
				if (days[0] == 0)
					sprintf(days, "%s", day[i]);
				else
					sprintf(days, "%s,%s", days, day[i]);
			}
		}

		sprintf(buf, "AutoReset Configured(User Start time : %02d:%02d ~ %02d:%02d, %s", 
				Entry.autoRebootHRangeStartHour, Entry.autoRebootHRangeStartMin, 
				Entry.autoRebootHRangeEndHour, Entry.autoRebootHRangeEndMin, days);

		syslog2(LOG_DM_AUTORESET, LOG_DM_AUTORESET_USER_CONF, buf);
	}
#endif

	return;

setErr_al:
	ERR_MSG(tmpBuf);

	return;
}

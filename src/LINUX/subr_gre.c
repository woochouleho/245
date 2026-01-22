
#include <string.h>
#include "debug.h"
#include "utility.h"
#include <linux/wireless.h>
#include <linux/route.h>
#include "form_src/multilang.h"
//#ifdef CONFIG_NET_IPGRE
//#include <net/route.h>
//#endif
#include <arpa/inet.h>

#ifdef CONFIG_NET_IPGRE
const char GRE_QOS_EB_CHAIN[] =  "gre_qos_eb_rules";
const char GRE_DSCP_EB_CHAIN[] =  "gre_dscp_eb_rules";
#define GRE_QOS_SETUP_PRINT_FUNCTION                    \
    do{fprintf(stderr,"%s: %s  %d\n", __FILE__, __FUNCTION__,__LINE__);}while(0);

static unsigned int g_cnt = 1;
static unsigned char g_class_flag[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int cmd_set_gre(unsigned char enable, unsigned char idx, int endpoint_idx)
{
	int ret = 1;
	char cmd[512];

	snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_set_gre %d %d %d", enable, idx, endpoint_idx);
	system(cmd);

	return ret;
}

#ifdef QOS_SETUP_IFB
static int addGreIFBQdisc(unsigned char totalBandWidthEn, unsigned int bandwidth, char *ifname)
{
	char s_rate[16], s_ceil[16];
	unsigned int rate, ceil;
	char *tc_act = NULL;

	DOCMDINIT;
	tc_act = (char*)ARG_ADD;
	//patch: actual bandwidth maybe a little greater than configured limit value, so I minish 7% of the configured limit value ahead.
	if (totalBandWidthEn)
		//ceil = bandwidth/100 * 93;
		ceil = bandwidth*93/100;
	else
		ceil = bandwidth;

	//tc qdisc add dev $DEV root handle 1: htb default 2 r2q 1
	va_cmd(TC, 12, 1, "qdisc", (char *)ARG_ADD, "dev", ifname,
		"root", "handle", "1:", "htb", "default", "2", "r2q", "1");

	// root class
	snprintf(s_rate, 16, "%dKbit", ceil);
	//tc class add dev $DEV parent 1: classid 1:1 htb rate $RATE ceil $CEIL
	va_cmd(TC, 17, 1, "class", (char *)ARG_ADD, "dev", ifname,
		"parent", "1:", "classid", "1:1", "htb", "rate", s_rate, "ceil", s_rate,
		"mpu", "64", "overhead", "4");

	//patch with above
	rate = (ceil>10)?10:ceil;

	// default class
	snprintf(s_rate, 16, "%dKbit", rate);
	snprintf(s_ceil, 16, "%dKbit", ceil);
	//tc class add dev $DEV parent 1:1 classid 1:2 htb rate $RATE ceil $CEIL
	va_cmd(TC, 17, 1, "class", (char *)ARG_ADD, "dev", ifname,
		"parent", "1:1", "classid", "1:2", "htb", "rate",
		s_rate, "ceil", s_ceil, "mpu", "64", "overhead", "4");

	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", ifname,
		"parent", "1:2", "handle", "2:", "pfifo", "limit", "100");

	return 1;
}
#endif

static int addGreQdisc(unsigned char totalBandWidthEn, unsigned int bandwidth, char *ifname, int upBandWidth, unsigned char itfGroup)
{
	char s_rate[16], s_ceil[16];
	unsigned int rate, ceil;
	int upLinkRate=0, childRate=0;
	int mark;
	char *tc_act = NULL;


	if (!upBandWidth)
		return 1;

	DOCMDINIT;
	tc_act = (char*)ARG_ADD;
#ifndef QOS_SETUP_IFB
	//patch: actual bandwidth maybe a little greater than configured limit value, so I minish 7% of the configured limit value ahead.
	if (totalBandWidthEn)
		//ceil = bandwidth/100 * 93;
		ceil = bandwidth*93/100;
	else
		ceil = bandwidth;

	//tc qdisc add dev $DEV root handle 1: htb default 2 r2q 1
	va_cmd(TC, 12, 1, "qdisc", (char *)ARG_ADD, "dev", ifname,
		"root", "handle", "1:", "htb", "default", "2", "r2q", "1");

	// root class
	snprintf(s_rate, 16, "%dKbit", ceil);
	//tc class add dev $DEV parent 1: classid 1:1 htb rate $RATE ceil $CEIL
	va_cmd(TC, 17, 1, "class", (char *)ARG_ADD, "dev", ifname,
		"parent", "1:", "classid", "1:1", "htb", "rate", s_rate, "ceil", s_rate,
		"mpu", "64", "overhead", "4");

	//patch with above
	rate = (ceil>10)?10:ceil;

	// default class
	snprintf(s_rate, 16, "%dKbit", rate);
	snprintf(s_ceil, 16, "%dKbit", ceil);
	//tc class add dev $DEV parent 1:1 classid 1:2 htb rate $RATE ceil $CEIL
	va_cmd(TC, 17, 1, "class", (char *)ARG_ADD, "dev", ifname,
		"parent", "1:1", "classid", "1:2", "htb", "rate",
		s_rate, "ceil", s_ceil, "mpu", "64", "overhead", "4");

	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", ifname,
		"parent", "1:2", "handle", "2:", "pfifo", "limit", "100");
#endif

	upLinkRate = upBandWidth;
	if(0 != upLinkRate)
	{
#ifdef QOS_SETUP_IFB
		char *devicename = "ifb0";

		va_cmd(TC, 12, 1, "qdisc", (char *)ARG_ADD, "dev", ifname,
			"root", "handle", "1:", "htb", "default", "2", "r2q", "1");
		DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: protocol ip u32 match u32 0 0 action mirred egress redirect dev ifb0", ifname);
#else
		char *devicename = ifname;
#endif

		//get mark
		//mark = (entry->entryid<<12);
		if (strncmp("gret", ifname, 4))
			g_cnt = itfGroup + itfGroup + 2;
		else
			g_cnt = itfGroup + itfGroup + 1;

		mark = (g_cnt<<12);
		//printf("ifname=%s, itfGroup=%d, g_cnt=%d\n", ifname, itfGroup, g_cnt);

		DOCMDARGVS(EBTABLES, DOWAIT, "-A %s -o %s -j mark --mark-or 0x%x --mark-target ACCEPT",
						GRE_QOS_EB_CHAIN, ifname, mark);

		if (g_class_flag[g_cnt-1] == 1) {
			//printf("The class %d have be set\n", g_cnt);
			return 1;
		}
		else
			g_class_flag[g_cnt-1] = 1;

		//patch: true bandwidth will be a little greater than limit value, so I minish 7% of set limit value ahead.
		int ceil;
		//ceil = upLinkRate/100 * 93;
		ceil = upLinkRate*93/100;

		childRate = (10>ceil)?ceil:10;
		//childRate = ceil;

		DOCMDARGVS(TC, DOWAIT, "class %s dev %s parent 1:1 classid 1:%d0 htb rate %dkbit ceil %dkbit mpu 64 overhead 4",
			tc_act, devicename, g_cnt, childRate, ceil);

		DOCMDARGVS(TC, DOWAIT, "qdisc %s dev %s parent 1:%d0 handle %d1: pfifo limit 100",
			tc_act, devicename, g_cnt, g_cnt);

		DOCMDARGVS(TC, DOWAIT, "filter %s dev %s parent 1: %s prio 0 handle 0x%x fw flowid 1:%d0",
			tc_act, devicename, "protocol ip", mark, g_cnt);

		//g_cnt++;
	}
	return 1;
}

#ifdef WLAN_SUPPORT
static int setupWLANGreQdisc(unsigned char totalBandWidthEn, unsigned int bandwidth, unsigned char itfGroup, int upBandWidth, unsigned int enable)
{

	int ori_wlan_idx, i;
	MIB_CE_MBSSIB_T mbssidEntry;
	char ifname[IFNAMSIZ];
	unsigned char mygroup;
	unsigned char vUChar;

	// set Qdisc rules(downstream) for WLAN on the same group
	ori_wlan_idx = wlan_idx;
	wlan_idx = 0;		// Magician: mib_get_s will add wlan_idx to mib id. Therefore wlan_idx must be set to 0.

#if defined(TRIBAND_SUPPORT)
{
    int k;
    for (k=0; k<NUM_WLAN_INTERFACE; k++) {
        wlan_idx = k;

        mib_get_s(MIB_WLAN_DISABLED, &vUChar, sizeof(vUChar));
        if (vUChar == 0) {
            mib_get_s(MIB_WLAN_ITF_GROUP, &mygroup, sizeof(mygroup));
            if (mygroup == itfGroup) {
                if (enable)
                    addGreQdisc(totalBandWidthEn, bandwidth, (char *)WLANIF[k], upBandWidth, itfGroup);
                else
                    va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", (char *)WLANIF[k], "root");
            }
        }
#ifdef WLAN_MBSSID
        for (i  = 0; i < IFGROUP_NUM - 1; i ++) {
            if (!mib_chain_get(MIB_MBSSIB_TBL, i  + 1, &mbssidEntry) || mbssidEntry.wlanDisabled)
                continue;

            mib_get_s(MIB_WLAN_VAP0_ITF_GROUP + (i << 1), &mygroup, sizeof(mygroup));
            sprintf(ifname, VAP_IF, k, i );
            if (mygroup == itfGroup) {
                if (enable)
                    addGreQdisc(totalBandWidthEn, bandwidth, ifname, upBandWidth, itfGroup);
                else
                    va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", ifname, "root");
            }
        }
#endif
    }
}
#else /* !defined(TRIBAND_SUPPORT) */

	// wlan0
	mib_get_s(MIB_WLAN_DISABLED, &vUChar, sizeof(vUChar));
	if (vUChar == 0) {
		mib_get_s(MIB_WLAN_ITF_GROUP, &mygroup, sizeof(mygroup));
		if (mygroup == itfGroup) {
			if (enable)
				addGreQdisc(totalBandWidthEn, bandwidth, (char *)WLANIF[0], upBandWidth, itfGroup);
			else
				va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", (char *)WLANIF[0], "root");
		}
	}

	// wlan1
#if defined(WLAN_DUALBAND_CONCURRENT)
	mib_get_s(MIB_WLAN1_DISABLED, &vUChar, sizeof(vUChar));
	if (vUChar == 0) {
		mib_get_s(MIB_WLAN1_ITF_GROUP, &mygroup, sizeof(mygroup));
		if (mygroup == itfGroup) {
			if (enable)
				addGreQdisc(totalBandWidthEn, bandwidth, (char *)WLANIF[1], upBandWidth, itfGroup);
			else
				va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", (char *)WLANIF[1], "root");
		}
	}
#endif

#ifdef WLAN_MBSSID
	for (i  = 0; i < IFGROUP_NUM - 1; i ++) {
		if (!mib_chain_get(MIB_MBSSIB_TBL, i  + 1, &mbssidEntry) || mbssidEntry.wlanDisabled)
			continue;

		mib_get_s(MIB_WLAN_VAP0_ITF_GROUP + (i << 1), &mygroup, sizeof(mygroup));
		sprintf(ifname, "wlan0-vap%d", i );
		if (mygroup == itfGroup) {
			if (enable)
				addGreQdisc(totalBandWidthEn, bandwidth, ifname, upBandWidth, itfGroup);
			else
				va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", ifname, "root");
		}
	}

#if defined(WLAN_DUALBAND_CONCURRENT)
	for (i = 0; i < IFGROUP_NUM - 1; i++) {
		if (!mib_chain_get(MIB_WLAN1_MBSSIB_TBL, i + 1, &mbssidEntry) || mbssidEntry.wlanDisabled)
			continue;

		mib_get_s(MIB_WLAN1_VAP0_ITF_GROUP + (i << 1), &mygroup, sizeof(mygroup));
		sprintf(ifname, "wlan1-vap%d", i);
		if (mygroup == itfGroup) {
			if (enable)
				addGreQdisc(totalBandWidthEn, bandwidth, ifname, upBandWidth, itfGroup);
			else
				va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", ifname, "root");
		}
	}
#endif
#endif
#endif /* defined(TRIBAND_SUPPORT) */

	wlan_idx = ori_wlan_idx;
	return 1;
}
#endif

#if defined(ITF_GROUP_4P) || defined(CONFIG_RTL_8676HWNAT) || defined(CONFIG_APOLLO_ROMEDRIVER)
static int setupELANGreQdisc(unsigned char totalBandWidthEn, unsigned int bandwidth, unsigned char itfGroup, int upBandWidth, unsigned int enable)
{

	unsigned int swNum;
	MIB_CE_SW_PORT_T Entry;
	char ifname[IFNAMSIZ];
	int i;

	// LAN ports
	swNum = mib_chain_total(MIB_SW_PORT_TBL);
	for (i = 0; i < ELANVIF_NUM; i++) {
		if (!mib_chain_get(MIB_SW_PORT_TBL, i, &Entry))
			return -1;
		if (Entry.itfGroup == itfGroup) {
			snprintf(ifname, sizeof(ifname), "%s.%hu", ELANIF, i+2);
			if (enable)
				addGreQdisc(totalBandWidthEn, bandwidth, ifname, upBandWidth, itfGroup);
			else
				va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", ifname, "root");
		}

#ifdef CONFIG_USER_VLAN_ON_LAN
		if (Entry.vlan_on_lan_enabled && Entry.vlan_on_lan_itfGroup == itfGroup) {
			snprintf(ifname, sizeof(ifname), "%s.%hu.%hu", ELANIF, i+2, Entry.vid);
			if (enable)
				addGreQdisc(totalBandWidthEn, bandwidth, ifname, upBandWidth, itfGroup);
			else
				va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", ifname, "root");
		}
#endif
	}
	return 1;
}
#endif

static int setupGreCarChain(unsigned int enable)
{
	GRE_QOS_SETUP_PRINT_FUNCTION;
	if (enable)
	{
		va_cmd(EBTABLES, 2, 1, "-N", GRE_QOS_EB_CHAIN);
		va_cmd(EBTABLES, 3, 1, "-P", GRE_QOS_EB_CHAIN, "RETURN");
		va_cmd(EBTABLES, 4, 1, "-A", "FORWARD", "-j", GRE_QOS_EB_CHAIN);

	}
	else {
		va_cmd(EBTABLES, 2, 1, "-F", GRE_QOS_EB_CHAIN);
		va_cmd(EBTABLES, 4, 1, "-D", "FORWARD", "-j", GRE_QOS_EB_CHAIN);
		va_cmd(EBTABLES, 2, 1, "-X", GRE_QOS_EB_CHAIN);
	}
	return 1;
}

/*
 *                              HTB(root qdisc, handle 10:)
 *                               |
 *                              HTB(root class, classid 10:1)
 *            ___________________|_____________________
 *            |         |        |          |          |
 *           HTB       HTB      HTB        HTB        HTB
 *(subclass id 10:10 rate Xkbit)........       (sub class id 10:N0 rate Ykbit)
 */
static int setupGreCarQdisc(unsigned int enable)
{
	MIB_GRE_T Entry;
	int i, EntryNum;
	char ifname[IFNAMSIZ];
	unsigned char totalBandWidthEn;
	unsigned int bandwidth;
	char greVlan[24];

	DOCMDINIT;
	GRE_QOS_SETUP_PRINT_FUNCTION;

	mib_get_s(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&totalBandWidthEn, sizeof(totalBandWidthEn));
	if (totalBandWidthEn)
		mib_get_s(MIB_TOTAL_BANDWIDTH, &bandwidth, sizeof(bandwidth));
	else
		bandwidth = getUpLinkRate();

#ifdef QOS_SETUP_IFB
	if (enable)
		addGreIFBQdisc(totalBandWidthEn, bandwidth, "ifb0");
	else
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", "ifb0", "root");
#endif

	EntryNum = mib_chain_total(MIB_GRE_TBL);
	for(i=0;i<EntryNum; i++)
	{
		unsigned char mygroup;
		#ifdef WLAN_MBSSID
		int ori_wlan_idx, j;
		MIB_CE_MBSSIB_T mbssidEntry;
		#endif

		if(!mib_chain_get(MIB_GRE_TBL, i, (void*)&Entry)||!Entry.enable)
			continue;

		strncpy(ifname, (char *)GREIF[i], sizeof(ifname));
		snprintf(greVlan, sizeof(greVlan), "%s.%u", ifname, Entry.vlanid);

		if (!enable) {
			va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", ifname, "root");
			if (Entry.vlanid != 0)
				va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", greVlan, "root");

			#ifdef WLAN_SUPPORT
			setupWLANGreQdisc(totalBandWidthEn, bandwidth, Entry.itfGroup, Entry.downBandWidth, enable);
			if (Entry.vlanid != 0 && Entry.itfGroup != Entry.itfGroupVlan)
				setupWLANGreQdisc(totalBandWidthEn, bandwidth, Entry.itfGroupVlan, Entry.downBandWidth, enable);
			#endif

			#if defined(ITF_GROUP_4P) || defined(CONFIG_RTL_8676HWNAT) || defined(CONFIG_APOLLO_ROMEDRIVER)
			setupELANGreQdisc(totalBandWidthEn, bandwidth, Entry.itfGroup, Entry.downBandWidth, enable);
			if (Entry.vlanid != 0 && Entry.itfGroup != Entry.itfGroupVlan)
				setupELANGreQdisc(totalBandWidthEn, bandwidth, Entry.itfGroupVlan, Entry.downBandWidth, enable);
			#endif
		}
		else {
			addGreQdisc(totalBandWidthEn, bandwidth, ifname, Entry.upBandWidth, Entry.itfGroup);
			if (Entry.vlanid != 0)
				addGreQdisc(totalBandWidthEn, bandwidth, greVlan, Entry.upBandWidth, Entry.itfGroupVlan);

			#ifdef WLAN_SUPPORT
			setupWLANGreQdisc(totalBandWidthEn, bandwidth, Entry.itfGroup, Entry.downBandWidth, enable);
			if (Entry.vlanid != 0 && Entry.itfGroup != Entry.itfGroupVlan)
				setupWLANGreQdisc(totalBandWidthEn, bandwidth, Entry.itfGroupVlan, Entry.downBandWidth, enable);
			#endif

			#if defined(ITF_GROUP_4P) || defined(CONFIG_RTL_8676HWNAT) || defined(CONFIG_APOLLO_ROMEDRIVER)
			setupELANGreQdisc(totalBandWidthEn, bandwidth, Entry.itfGroup, Entry.downBandWidth, enable);
			if (Entry.vlanid != 0 && Entry.itfGroup != Entry.itfGroupVlan)
				setupELANGreQdisc(totalBandWidthEn, bandwidth, Entry.itfGroupVlan, Entry.downBandWidth, enable);
			#endif
		}
	}
	return 0;
}

int needSetupGreQoS(void)
{
	MIB_GRE_T Entry;
	int i, EntryNum;

	EntryNum = mib_chain_total(MIB_GRE_TBL);
	for(i=0;i<EntryNum; i++) {
		if(!mib_chain_get(MIB_GRE_TBL, i, (void*)&Entry)||!Entry.enable)
			continue;
		if (Entry.upBandWidth!=0 || Entry.downBandWidth!=0)
			return 1;
	}
	return 0;
}

int setupGreQoS(void)
{
	if (needSetupGreQoS()) {
		system("/bin/echo 0 > /proc/realtek/fastbridge");
		#ifdef QOS_SETUP_IFB
		// ifconfig ifb0 up
		va_cmd(IFCONFIG, 2, 1, "ifb0", "up");
		#endif
		setupGreCarChain(1);
		setupGreCarQdisc(1);
	}
	return 1;
}

int stopGreQoS(void)
{
	int i;

	system("/bin/echo 1 > /proc/realtek/fastbridge");
	setupGreCarChain(0);
	setupGreCarQdisc(0);
	g_cnt = 1;
	for (i=0; i<10; i++)
		g_class_flag[i] = 0;

	return 1;
}

static int setupDscpTagRule(void)
{
	MIB_GRE_T Entry;
	int i, EntryNum;
	char ifname[IFNAMSIZ];
	char greVlan[24];

	DOCMDINIT;
	EntryNum = mib_chain_total(MIB_GRE_TBL);
	for(i=0;i<EntryNum; i++)
	{
		if(!mib_chain_get(MIB_GRE_TBL, i, (void*)&Entry)||!Entry.enable)
			continue;

		strncpy(ifname, (char *)GREIF[i], sizeof(ifname));
		snprintf(greVlan, sizeof(greVlan), "%s.%u", ifname, Entry.vlanid);

		// ebtables -I FORWARD -o gret0 -j ftos --set-ftos 0x38
		DOCMDARGVS(EBTABLES, DOWAIT, "-A %s -o %s -j ftos --set-ftos 0x%x",
				GRE_DSCP_EB_CHAIN, ifname, (Entry.m_dscp-1)&0xFF);
		if (Entry.vlanid != 0) {
			// ebtables -I FORWARD -o gret0.81 -j ftos --set-ftos 0x38
			DOCMDARGVS(EBTABLES, DOWAIT, "-A %s -o %s -j ftos --set-ftos 0x%x",
					GRE_DSCP_EB_CHAIN, greVlan, (Entry.m_dscp-1)&0xFF);
		}
	}
	return 1;
}

// enable: 1 - enable; 0 - disable.
static int setupDscpTagChain(unsigned int enable)
{
	if (enable)
	{
		va_cmd(EBTABLES, 2, 1, "-N", GRE_DSCP_EB_CHAIN);
		va_cmd(EBTABLES, 3, 1, "-P", GRE_DSCP_EB_CHAIN, "RETURN");
		va_cmd(EBTABLES, 4, 1, "-A", "FORWARD", "-j", GRE_DSCP_EB_CHAIN);

	}
	else {
		va_cmd(EBTABLES, 2, 1, "-F", GRE_DSCP_EB_CHAIN);
		va_cmd(EBTABLES, 4, 1, "-D", "FORWARD", "-j", GRE_DSCP_EB_CHAIN);
		va_cmd(EBTABLES, 2, 1, "-X", GRE_DSCP_EB_CHAIN);
	}
	return 1;
}

int setupDscpTag(void)
{
	system("/bin/echo 0 > /proc/realtek/fastbridge");
	setupDscpTagChain(1);
	setupDscpTagRule();
	return 1;
}

int stopDscpTag(void)
{
	system("/bin/echo 1 > /proc/realtek/fastbridge");
	setupDscpTagChain(0);
	return 1;
}

void update_gre_ssid()
{
	MIB_GRE_T greEntry;
	MIB_CE_MBSSIB_T mbssidEntry;
	int i, j, num, grpnum, strNum;
	char tmpBuf[100];
	unsigned char mygroup;
	char groupitf[128];
	char ssid[MAX_SSID_LEN];
#if defined(WLAN_SUPPORT)
	int ori_wlan_idx;
#endif
	unsigned char vUChar;

	num = mib_chain_total(MIB_GRE_TBL);
	for (i = 0; i < num; i++) {
		char idx_ssid=0;
		unsigned char itfGroup;

		if (!mib_chain_get(MIB_GRE_TBL, i, (void *)&greEntry))
			return;

#ifdef WLAN_SUPPORT
		ori_wlan_idx = wlan_idx;
		wlan_idx = 0;		// Magician: mib_get_s will add wlan_idx to mib id. Therefore wlan_idx must be set to 0.
ANOTHER_SSID:
		groupitf[0] = '\0';
		strNum = 0;

		if (idx_ssid == 1)
			itfGroup = greEntry.itfGroupVlan;
		else
			itfGroup = greEntry.itfGroup;

		// wlan0
		mib_get_s(MIB_WLAN_DISABLED, &vUChar, sizeof(vUChar));
		if (vUChar == 0) {
		mib_get_s(MIB_WLAN_ITF_GROUP, &mygroup, sizeof(mygroup));
		mib_get_s(MIB_WLAN_SSID, (void *)ssid, sizeof(ssid));
			if (mygroup == itfGroup) {
			if (!strNum) {
				strncat(groupitf, ssid, 64);
				strNum++;
			}
			else {
				strncat(groupitf, ", ", 64);
				strncat(groupitf, ssid, 64);
				strNum++;
			}
		}
		}

		for (j = 0; j< IFGROUP_NUM - 1; j++) {
			if (!mib_chain_get(MIB_MBSSIB_TBL, j + 1, &mbssidEntry) || mbssidEntry.wlanDisabled)
				continue;

			mib_get_s(MIB_WLAN_VAP0_ITF_GROUP + (j << 1), &mygroup, sizeof(mygroup));
			if (mygroup == itfGroup) {
				if (!strNum) {
					strncat(groupitf, mbssidEntry.ssid, 64);
					strNum++;
				}
				else {
					strncat(groupitf, ", ", 64);
					strncat(groupitf, mbssidEntry.ssid, 64);
					strNum++;
				}
			}
		}

		// wlan1
		#if defined(WLAN_DUALBAND_CONCURRENT)
		mib_get_s(MIB_WLAN1_DISABLED, &vUChar, sizeof(vUChar));
		if (vUChar == 0) {
		mib_get_s(MIB_WLAN1_ITF_GROUP, &mygroup, sizeof(mygroup));
		mib_get_s(MIB_WLAN1_SSID, (void *)ssid, sizeof(ssid));
			if (mygroup == itfGroup) {
			if (!strNum) {
				strncat(groupitf, ssid, 64);
				strNum++;
			}
			else {
				strncat(groupitf, ", ", 64);
				strncat(groupitf, ssid, 64);
				strNum++;
			}
		}
		}
		for (j = 0; j< IFGROUP_NUM - 1; j++) {
			if (!mib_chain_get(MIB_WLAN1_MBSSIB_TBL, j + 1, &mbssidEntry) || mbssidEntry.wlanDisabled)
				continue;

			mib_get_s(MIB_WLAN1_VAP0_ITF_GROUP + (j << 1), &mygroup, sizeof(mygroup));
			if (mygroup == itfGroup) {
				if (!strNum) {
					strncat(groupitf, mbssidEntry.ssid, 64);
					strNum++;
				}
				else {
					strncat(groupitf, ", ", 64);
					strncat(groupitf, mbssidEntry.ssid, 64);
					strNum++;
				}
			}
		}
		#endif

		if (!idx_ssid) {
			strncpy(greEntry.ssid, groupitf, 128);
			idx_ssid = 1;
			goto ANOTHER_SSID;
		}
		else if (idx_ssid) {
			strncpy(greEntry.ssidVlan, groupitf, 128);
			idx_ssid = 0;
		}

		mib_chain_update(MIB_GRE_TBL, (void *)&greEntry, i);
		wlan_idx = ori_wlan_idx;
#endif
	}
	return;
}

char ptr_ifname[IFNAMSIZ];

char *find_Internet_WAN_IP(char *ip)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ];
	struct in_addr inAddr;
	char *itfIP;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
  			printf("getNameByIP: Get chain record error!\n");
			return NULL;
		}

		if (Entry.enable == 0 || !isDefaultRouteWan(&Entry))
			continue;

		ifGetName(Entry.ifIndex,ifname,sizeof(ifname));
		strncpy(ptr_ifname, ifname, IFNAMSIZ);
		if (getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 1) {
			itfIP = (char*)inet_ntoa(inAddr);
			strncpy(ip, itfIP, 16);
			printf("DGW:Fine WAN %s interface, IP is %s\n", ifname, ip);
			return ptr_ifname;
		}
	}
	return NULL;
}

char *find_Route_WAN_IP(struct in_addr *haddr, char *ip)
{
	char buff[256];
	int flgs;
	struct in_addr dest, mask;
	struct in_addr inAddr;
	char *itfIP;
	FILE *fp;
	char ifname[IFNAMSIZ];

	if (!(fp = fopen("/proc/net/route", "r"))) {
		printf("Error: cannot open /proc/net/route - continuing...\n");
		return NULL;
	}
	fgets(buff, sizeof(buff), fp);
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		if (sscanf(buff, "%s%x%*x%x%*d%*d%*d%x", ifname, &dest, &flgs, &mask) != 4) {
			printf("Unsuported kernel route format\n");
			fclose(fp);
			return NULL;
		}
		strncpy(ptr_ifname, ifname, IFNAMSIZ);
		printf("ifname=%s, haddr=0x%x, dest=0x%x, mask=0x%x\n", ifname, haddr->s_addr, dest.s_addr, mask.s_addr);
		if ((flgs & RTF_UP) && mask.s_addr != 0) {
			if ((dest.s_addr & mask.s_addr) == (haddr->s_addr & mask.s_addr)) {
				//printf("dest=0x%x, mask=0x%x\n", dest.s_addr, mask.s_addr);
				if (getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 1) {
					itfIP = (char*)inet_ntoa(inAddr);
					strncpy(ip, itfIP, 16);
					printf("Route TBL: Fine %s WAN interface, IP is %s\n", ifname, ip);
				}
				fclose(fp);
				return ptr_ifname;
			}
		}
	}
	fclose(fp);
	return NULL;
}

// enable: 0 is disable, 1 is enable.
// endpoint_idx:  0 is Endpoint, 1 is Backup Endpoint
int applyGRE(int enable, int idx, int endpoint_idx)
{
	cmd_set_gre(enable, idx, endpoint_idx);
	return 1;
}

void gre_take_effect(int endpoint_idx)
{
	MIB_GRE_T entry;
	unsigned int entrynum, i;
	unsigned char enable;

	if ( !mib_get_s(MIB_GRE_ENABLE, (void *)&enable, sizeof(enable)) )
		return;

	entrynum = mib_chain_total(MIB_GRE_TBL);
	//delete all firstly
	for (i=0; i<entrynum; i++)
	{
		if ( !mib_chain_get(MIB_GRE_TBL, i, (void *)&entry) )
			return;
		applyGRE(0, i, endpoint_idx);
	}

	if (enable) {
		for (i=0; i<entrynum; i++)
		{
			if ( !mib_chain_get(MIB_GRE_TBL, i, (void *)&entry) )
				return;
			applyGRE(1, i, endpoint_idx);
		}
	}
}

#endif


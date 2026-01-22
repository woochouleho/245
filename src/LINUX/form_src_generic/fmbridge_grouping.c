/*
 *      Web server handler routines for Ethernet-to-PVC mapping stuffs
 *
 */


/*-- System inlcude files --*/
//#include <net/if.h>
#include <signal.h>
#ifdef EMBED
#include <linux/config.h>
#else
#include "../../../../include/linux/autoconf.h"
#endif

/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "options.h"

#ifdef __i386__
#define _LITTLE_ENDIAN_
#endif

void formBridgeGrouping(request * wp, char *path, char *query)
{
	char *str, *submitUrl;
	int grpnum;

	str = boaGetVar(wp, "select", "");
	if (str[0]) {
		setup_bridge_grouping(DEL_RULE);

		/* s1 ~ s4 */
		grpnum = str[1] - '0';
		str = boaGetVar(wp, "itfsGroup", "");
		if (str[0]) {
			setgroup(str, grpnum);
		}

		str = boaGetVar(wp, "itfsAvail", "");
		if (str[0]) {
			setgroup(str, 0);
		}

		setup_bridge_grouping(ADD_RULE);
#ifdef CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE
		rtk_igmp_mld_snooping_module_init(BRIF, CONFIGALL, NULL);
#endif

#ifdef CONFIG_NET_IPGRE
		update_gre_ssid();
		stopGreQoS();
		setupGreQoS();
#endif
// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
	}

	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG(submitUrl);
}

int ifGroupList(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
	int i, ifnum, num;
	struct itfInfo itfs[MAX_NUM_OF_ITFS];
	char groupitf[512], groupval[512], availitf[512], availval[512];
	char *ptr;

#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr><font size=2>"
			       "<td align=center bgcolor=\"#808080\">%s</td>\n"
			       "<td align=center bgcolor=\"#808080\">%s</td></font></tr>\n",multilang(LANG_SELECT),multilang(LANG_INTERFACES));
#else
	nBytesSent += boaWrite(wp, "<div class=\"data_common data_vertical\">\n<table>\n");
	nBytesSent += boaWrite(wp, "<tr>"
			       "<th align=center>%s</th>\n"
			       "<th align=center>%s</th></tr>\n",multilang(LANG_SELECT),multilang(LANG_INTERFACES));
#endif
	// Show default group
	ifnum = get_group_ifinfo(itfs, MAX_NUM_OF_ITFS, 0);
	availitf[0] = availval[0] = '\0';
	if (ifnum > 0) {
		strncat(availitf, itfs[0].name, 64);
		ptr = availval + snprintf(availval, 64, "%u",
			 IF_ID(itfs[0].ifdomain, itfs[0].ifid));
		ifnum--;
		for (i = 1; i <= ifnum; i++) {
			strncat(availitf, ", ", 64);
			strncat(availitf, itfs[i].name, 64);
			ptr += snprintf(ptr, 64, ", %u",
				 IF_ID(itfs[i].ifdomain, itfs[i].ifid));
		}
	}
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr><font size=2>"
			       "<td align=center bgcolor=\"#C0C0C0\">Default</td>\n"
			       "<td align=center bgcolor=\"#C0C0C0\">%s</td></font></tr>\n",
			       availitf);
#else
	nBytesSent += boaWrite(wp, "<tr>"
			       "<td align=center>Default</td>\n"
			       "<td align=center>%s</td></tr>\n",
			       availitf);
#endif
	// Show the specified groups
#ifdef CONFIG_00R0
	for (num = 1; num <IFGROUP_NUM; num++) {
#else
	for (num = 1; num <= 4; num++) {
#endif
		ifnum = get_group_ifinfo(itfs, MAX_NUM_OF_ITFS, num);
		groupitf[0] = groupval[0] = '\0';
		if (ifnum > 0) {
			strncat(groupitf, itfs[0].name, 64);
			ptr = groupval + snprintf(groupval, 64, "%u",
				 IF_ID(itfs[0].ifdomain, itfs[0].ifid));
			ifnum--;
			for (i = 1; i <= ifnum; i++) {
				strncat(groupitf, ", ", 64);
				strncat(groupitf, itfs[i].name, 64);
				ptr += snprintf(ptr, 64, ", %u",
					 IF_ID(itfs[i].ifdomain, itfs[i].ifid));
			}
		}
#ifndef CONFIG_GENERAL_WEB
		nBytesSent += boaWrite(wp, "<tr><font size=\"2\">"
				       "<td align=center bgcolor=\"#C0C0C0\"><input type=\"radio\" name=\"select\""
				       " value=\"s%d\" onClick=\"postit('%s','%s','%s','%s')\"</td>\n",
				       num, groupitf, groupval, availitf, availval);
		nBytesSent +=
		    boaWrite(wp,
			     "<td align=center bgcolor=\"#C0C0C0\">%s</td></font></tr>\n",
			     groupitf);
#else
		nBytesSent += boaWrite(wp, "<tr>"
				       "<td align=center><input type=\"radio\" name=\"select\""
				       " value=\"s%d\" onClick=\"postit('%s','%s','%s','%s')\"</td>\n",
				       num, groupitf, groupval, availitf, availval);
		nBytesSent +=
		    boaWrite(wp,
			     "<td align=center>%s</td></tr>\n",
			     groupitf);
#endif
	}
#ifdef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "</table>\n</div>");
#endif
	return nBytesSent;
}


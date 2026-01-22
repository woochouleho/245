/*
 *      Web server handler routines for Group Table diagnostic stuffs
 *
 */


/*-- System inlcude files --*/
#include <string.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <rtk/utility.h>
#include <pthread.h>
#include <sys/ioctl.h>

/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "../defs.h"
#include "boa.h"
#include "multilang.h"

#define IPV6_DEFAULT "0000:0000:0000:0000:0000:0000:0000:0000"

int clearGroupTableResult(int eid, request * wp, int argc, char **argv)
{
	unlink("/tmp/grouptable.tmp");
	return 0;
}

void formGroupTableResult(request * wp, char *path, char *query) {
	char line[512] = {0};
	FILE *pf = NULL;
	int nBytesSent=0;

	pf = fopen("/tmp/grouptable.tmp", "r");
	if(!pf) {
		//printf("open /tmp/grouptable.tmp fail.\n");
		return;
	}
	while (fgets(line, sizeof(line), pf)) {
		nBytesSent += boaWrite(wp, "<tr><td class=\"intro_content\">%s</td></tr>", line);
	}
	fclose(pf);
	return;
}

void get_port(char *ori, char *out)
{
	if (strstr(ori, "eth0.2"))
		sprintf(out, "%s", "LAN1");
	else if (strstr(ori, "eth0.3"))
		sprintf(out, "%s", "LAN2");
	else if (strstr(ori, "eth0.4"))
		sprintf(out, "%s", "LAN3");
	else if (strstr(ori, "eth0.5"))
		sprintf(out, "%s", "LAN4");

	return;
}

int user_confirm(int eid, request * wp, int argc, char **argv)
{
	char remote_ip_addr[INET6_ADDRSTRLEN] = {0};
	int nBytesSent = 0;
	int val;
	int ret = 0;

	strcpy(remote_ip_addr, wp->remote_ip_addr);
	rtk_util_get_ipv4addr_from_ipv6addr(remote_ip_addr);
	ret = rtk_intf_get_intf_by_ip(remote_ip_addr);

	if (ret == 0) { // for lan
		if (strstr(global_access_name, "root"))
			val = 1;
		else
			val = 0;
	}
	else // wan is not check for account level
		val = 1;

	nBytesSent += boaWrite(wp,"user_level=%d\n", val);
	return nBytesSent;
}

void formGroupTable(request * wp, char *path, char *query)
{
	char fgroup[128] = {0};
	char tgroup[128] = {0};
	char client[128] = {0};
	char ifname[128] = {0};
	char port[5] = {0};
	char vid[8] = {0};
	char time[8] = {0};
	FILE *flowfp = NULL;
	FILE *tablefp = NULL;
	FILE *outfp = NULL;
	char flowTemp[512] = {0};
	char tableTemp[512] = {0};
	char inbuf[512] = {0};
	char *strAct, *submitUrl, *posi;
	char *com = ".";
	char *colon = ":";
	int find = 0;
	int type = 0; // type 0 is v4, type 1 is v6

	unlink("/tmp/grouptable.tmp");
	strAct = boaGetVar(wp, "grouptableAct", "");

	if (!strncmp(strAct, "igmpSearch", 10) || !strncmp(strAct, "igmpReset", 9))
		type = 0; // igmp

	if (!strncmp(strAct, "mldSearch", 9) || !strncmp(strAct, "mldReset", 8))
		type = 1; // mld

	if (!strncmp(strAct, "allSearch", 9) || !strncmp(strAct, "allReset", 8))
		type = 2; // all

	outfp = fopen("/tmp/grouptable.tmp", "w");
	if (outfp == NULL) {
		printf("Error: outfp can't fopen()! \n");
		return;
	}

	tablefp = fopen("/proc/igmp/dump/igmpInfotable", "r");
	if (tablefp == NULL) {
		printf("Error: tablefp can't fopen()! \n");
		fclose(outfp);
		return;
	} else {
		while (!feof(tablefp)) {
			memset(tableTemp, 0, sizeof(tableTemp));
			memset(client, 0, sizeof(client));
			memset(ifname, 0, sizeof(ifname));
			memset(port, 0, sizeof(port));
			memset(time, 0, sizeof(time));

			fgets(tableTemp, sizeof(tableTemp), tablefp);
			if (strstr(tableTemp, "Group")) { // group and client
				memset(tgroup, 0, sizeof(tgroup));
				sscanf(tableTemp, "Group %s Client %s %s %s", tgroup, client, ifname, time);
			}
			else // only client
				sscanf(tableTemp, "Client %s %s %s", client, ifname, time);

			// interface to port name
			get_port(ifname, port);

			flowfp = fopen("/proc/igmp/dump/igmpInfoflow", "r");
			if (flowfp == NULL) {
				printf("Error: flowfp can't fopen()! \n");
				fclose(tablefp);
				fclose(outfp);
				return;
			} else {
				while (!feof(flowfp)) {
					memset(flowTemp, 0, sizeof(flowTemp));
					memset(fgroup, 0, sizeof(fgroup));
					memset(vid, 0, sizeof(vid));

					fgets(flowTemp, sizeof(flowTemp), flowfp);
					sscanf(flowTemp, "Group %s Vid %s", fgroup, vid);

					if (strlen(tgroup) > 0) {
						memset(inbuf, 0, sizeof(inbuf));
						if (!strcmp(fgroup, tgroup) && strcmp(client, IPV6_DEFAULT) && strlen(client) > 0 && strlen(time)){ // exist flow & join table
							snprintf(inbuf, sizeof(inbuf), "VID %s\t Group %s\t Report %s\t Timeout %s sec\t   ifname %s \n",
									vid, tgroup, client, time, port);

							if ((type == 0 && strstr(tgroup, com)) || (type == 1 && strstr(tgroup, colon)) || type == 2) {// check individual igmp or mld
								fputs(inbuf, outfp);
								find = 1;
							}
						}
					}
				}

				if ((type == 0 && strstr(tgroup, com)) || (type == 1 && strstr(tgroup, colon)) || type == 2) {// check individual igmp or mld
					if (!find && strlen(tgroup) > 0 && strcmp(client, IPV6_DEFAULT) && strlen(client) > 0 && strlen(time) > 0 && strlen(port) > 0) { // not exist flow or add only client
						snprintf(inbuf, sizeof(inbuf), "VID 0\t Group %s\t Report %s\t Timeout %s sec\t   ifname %s \n",
								tgroup, client, time, port);
						fputs(inbuf, outfp);
					}
				}
				else
					find = 0; // reset

				fclose(flowfp);
			}
		}
		fclose(tablefp);
	}
	fclose(outfp);

	if (!strncmp(strAct, "igmpSearch", 10) || !strncmp(strAct, "mldSearch", 9) || !strncmp(strAct, "allSearch", 9)) {
		wp->buffer_end=0; // clear header
		boaWrite(wp, "HTTP/1.0 204 No Content\n");
		boaWrite(wp, "Pragma: no-cache\n");
		boaWrite(wp, "Cache-Control: no-cache\n");
		boaWrite(wp, "\n");
	} else if (!strncmp(strAct, "igmpReset", 9) || !strncmp(strAct, "mldReset", 8) || !strncmp(strAct, "allReset", 8)) {
		unlink("/tmp/grouptable.tmp");
		submitUrl = boaGetVar(wp, "submit-url", "");
		if (submitUrl[0])
			boaRedirect(wp, submitUrl);
		else
			boaDone(wp, 200);
	}

	return;
}


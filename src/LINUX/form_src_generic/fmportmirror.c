/*
 *      Web server handler routines for Port Mirror diagnostic stuffs
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

int clearPortMirrorResult(int eid, request *wp, int argc, char **argv)
{
	system("PortMirror clear");
	unlink("/tmp/portmirror.tmp");

	return 0;
}

void formPortMirrorResult(request *wp, char *path, char *query) {
	char line[512] = {0};
	FILE *pf = NULL;
	int nBytesSent = 0;

	pf = fopen("/tmp/portmirror.tmp", "r");
	if (!pf) {
		//printf("open /tmp/portmirror.tmp fail.\n");
		return;
	}

	while (fgets(line, sizeof(line), pf)) {
		nBytesSent += boaWrite(wp, "<tr><td class=\"intro_content\">%s</td></tr>", line);
	}

	fclose(pf);
	return;
}

// exam1: PortMirror set pon 2 rtx
// exam2: PortMirror set pon 2 rx
// exam3: PortMirror set pon 2 tx
int portmirrorinit(int eid, request *wp, int argc, char **argv)
{
	char line[32] = {0};
	FILE *pf = NULL;
	int nBytesSent = 0;
	char target[4] = {0};
	char read[4] = {0};
	char type[4] = {0};

	pf = fopen("/tmp/portmirrorset.tmp", "r");
	if (!pf) {
		//printf("open /tmp/portmirror.tmp fail.\n");
		return 0;
	}

	while (fgets(line, sizeof(line), pf)) {
		sscanf(line, "PortMirror set %s %s %s", &target, &read, &type);
	}

	if (strstr(target, "pon"))
		nBytesSent += boaWrite(wp, "mingport=1\n");
	else if (strstr(target, "1"))
		nBytesSent += boaWrite(wp, "mingport=2\n");
	else if (strstr(target, "2"))
		nBytesSent += boaWrite(wp, "mingport=3\n");
	else if (strstr(target, "3"))
		nBytesSent += boaWrite(wp, "mingport=4\n");
	else if (strstr(target, "4"))
		nBytesSent += boaWrite(wp, "mingport=5\n");

	if (strstr(read, "1"))
		nBytesSent += boaWrite(wp, "medport=1\n");
	else if (strstr(read, "2"))
		nBytesSent += boaWrite(wp, "medport=2\n");
	else if (strstr(read, "3"))
		nBytesSent += boaWrite(wp, "medport=3\n");
	else if (strstr(read, "4"))
		nBytesSent += boaWrite(wp, "medport=4\n");

	if (strcmp(type, "rx") == 0)
		nBytesSent += boaWrite(wp, "rxtx=1\n");
	else if (strcmp(type, "tx") == 0)
		nBytesSent += boaWrite(wp, "rxtx=2\n");
	else if (strcmp(type, "rtx") == 0)
		nBytesSent += boaWrite(wp, "rxtx=3\n");

	return nBytesSent;
}

void formPortMirror(request *wp, char *path, char *query)
{
	char cmd[512] = {0};
	char log[512] = {0};
	char cmd_set[128] = {0};
	char *str, *strAct, *submitUrl;
	char *mingport0 = NULL, *mingport1 = NULL, *mingport2 = NULL, *mingport3 = NULL, *mingport4 = NULL;
	char *medport1 = NULL, *medport2 = NULL, *medport3 = NULL, *medport4 = NULL;
	char *rxtx0 = NULL, *rxtx1 = NULL, *rxtx2 = NULL;
	int num = 0;

	system("PortMirror clear");

	strAct = boaGetVar(wp, "portmirrorAct", "");
	mingport0 = boaGetVar(wp, "mingport0", "");
	mingport1 = boaGetVar(wp, "mingport1", "");
	mingport2 = boaGetVar(wp, "mingport2", "");
	mingport3 = boaGetVar(wp, "mingport3", "");
	mingport4 = boaGetVar(wp, "mingport4", "");

	medport1 = boaGetVar(wp, "medport1", "");
	medport2 = boaGetVar(wp, "medport2", "");
	medport3 = boaGetVar(wp, "medport3", "");
	medport4 = boaGetVar(wp, "medport4", "");

	rxtx0 = boaGetVar(wp, "rxtx0", "");
	rxtx1 = boaGetVar(wp, "rxtx1", "");
	rxtx2 = boaGetVar(wp, "rxtx2", "");

	memset(cmd, 0, sizeof(cmd));
	memset(log, 0, sizeof(log));

	if (strlen(rxtx0) == 0 && strlen(rxtx1) == 0 &&  strlen(rxtx2) == 0) {
		system("echo DONE. > /tmp/portmirror.tmp &");
		unlink("/tmp/portmirrorset.tmp");
	} else {
		unlink("/tmp/portmirror.tmp");
		strcat(cmd, "PortMirror set ");
		strcat(log, "Set Port Mirroring ");

		// rxtx
		if ((rxtx0[0] - '0') == 1)
			strcat(log, "rx");
		else if ((rxtx1[0] - '0') == 2)
			strcat(log, "tx");
		else if ((rxtx2[0] - '0') == 3)
			strcat(log, "both");

		strcat(log, " ");

		if ((mingport0[0] - '0') == 1) {
			strcat(cmd, "pon"); // wan
			strcat(log, "WAN");
			num++;
		}

		if ((mingport1[0] - '0') == 2) {
			if (num > 0)
				strcat(cmd, ",");
			strcat(cmd, "1"); // lan1
			strcat(log, "LAN1");
			num++;
		}

		if ((mingport2[0] - '0') == 3) {
			if (num > 0)
				strcat(cmd, ",");
			strcat(cmd, "2"); // lan2
			strcat(log, "LAN2");
			num++;
		}

		if ((mingport3[0] - '0') == 4) {
			if (num > 0)
				strcat(cmd, ",");
			strcat(cmd, "3"); // lan3
			strcat(log, "LAN3");
			num++;
		}

		if ((mingport4[0] - '0') == 5) {
			if (num > 0)
				strcat(cmd, ",");
			strcat(cmd, "4"); // lan4
			strcat(log, "LAN4");
		}

		strcat(cmd, " ");
		strcat(log, " => ");

		if ((medport1[0] - '0') == 1)
		{
			strcat(cmd, "1"); // lan1
			strcat(log, "LAN1");
		}
		else if ((medport2[0] - '0') == 2)
		{
			strcat(cmd, "2"); // lan2
			strcat(log, "LAN2");
		}
		else if ((medport3[0] - '0') == 3)
		{
			strcat(cmd, "3"); // lan3
			strcat(log, "LAN3");
		}
		else if ((medport4[0] - '0') == 4)
		{
			strcat(cmd, "4"); // lan4
			strcat(log, "LAN4");
		}

		strcat(cmd, " ");

		// rxtx
		if ((rxtx0[0] - '0') == 1)
			strcat(cmd, "rx");
		else if ((rxtx1[0] - '0') == 2)
			strcat(cmd, "tx");
		else if ((rxtx2[0] - '0') == 3)
			strcat(cmd, "rtx");

		snprintf(cmd_set, sizeof(cmd_set), "echo %s > /tmp/portmirrorset.tmp &", cmd);
		snprintf(cmd, sizeof(cmd), "%s > /tmp/portmirror.tmp &", cmd);
		system(cmd);

		// write current config
		system(cmd_set);
		syslog2(LOG_DM_HTTP, LOG_DM_HTTP_MIRROT_SET, "%s", log);
	}

	wp->buffer_end = 0; // clear header
	boaWrite(wp, "HTTP/1.0 204 No Content\n");
	boaWrite(wp, "Pragma: no-cache\n");
	boaWrite(wp, "Cache-Control: no-cache\n");
	boaWrite(wp, "\n");

	return;
}


/*
 *      Web server handler routines for Netstat diagnostic stuffs
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

int clearNetstatResult(int eid, request * wp, int argc, char **argv)
{
	unlink("/tmp/netstat.tmp");
	return 0;
}

void formNetstatResult(request * wp, char *path, char *query) {
	char line[512] = {0};
	FILE *pf = NULL;
	int nBytesSent=0;

	pf = fopen("/tmp/netstat.tmp", "r");
	if(!pf) {
		//printf("open /tmp/netstat.tmp fail.\n");
		return;
	}
	while (fgets(line, sizeof(line), pf)) {
		nBytesSent += boaWrite(wp, "<tr><td class=\"intro_content\">%s</td></tr>", line);
	}
	fclose(pf);
	return;
}

///////////////////////////////////////////////////////////////////
void formNetstat(request * wp, char *path, char *query)
{
	char cmd[512] = {0};
	char *str, *strAct, *submitUrl;

	unlink("/tmp/netstat.tmp");

	strAct = boaGetVar(wp, "netstatAct", "");

	system("netstat -apn > /tmp/netstat.tmp &");
	if (!strncmp(strAct, "Search", 6)) {
		wp->buffer_end=0; // clear header
		boaWrite(wp, "HTTP/1.0 204 No Content\n");
		boaWrite(wp, "Pragma: no-cache\n");
		boaWrite(wp, "Cache-Control: no-cache\n");
		boaWrite(wp, "\n");
	} else if (!strncmp(strAct, "Reset", 5)) {
		unlink("/tmp/netstat.tmp");
		submitUrl = boaGetVar(wp, "submit-url", "");
		if (submitUrl[0])
			boaRedirect(wp, submitUrl);
		else
			boaDone(wp, 200);
	}

	return;
}


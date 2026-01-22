/*
 *      Web server handler routines for Ping diagnostic stuffs
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

int clearPingResult(int eid, request * wp, int argc, char **argv)
{
	va_cmd("/bin/killall", 1, 1, "ping");
	va_cmd("/bin/killall", 1, 1, "ping6");
	unlink("/tmp/ping.tmp");
	return 0;
}

void formPingResult(request * wp, char *path, char *query) {
	char line[512] = {0};
	FILE *pf = NULL;
	int nBytesSent=0;

	pf = fopen("/tmp/ping.tmp", "r");
	if(!pf) {
		//printf("open /tmp/ping.tmp fail.\n");
		return;
	}
	while (fgets(line, sizeof(line), pf)) {
		nBytesSent += boaWrite(wp, "<tr><td class=\"intro_content\">%s</td></tr>", line);
	}
	fclose(pf);
	return;
}

///////////////////////////////////////////////////////////////////
void formPing(request * wp, char *path, char *query)
{
	char *ipaddr = NULL;
	char cmd[512] = {0};
	char *str, *strPingAct, *submitUrl, wanifname[IFNAMSIZ]={0};

	va_cmd("/bin/killall", 1, 1, "ping");
	unlink("/tmp/ping.tmp");

	strPingAct = boaGetVar(wp, "pingAct", "");
	ipaddr = boaGetVar(wp, "pingAddr", "");
	if (!ipaddr[0]) {
		ERR_MSG("Wrong IP address!");
		return;
	}

	if (isValidPingAddressValue(ipaddr) != 0)
	{
		ERR_MSG("Wrong IP address!");
		return;
	}

	//printf("%s IP address %s\n", __func__, ipaddr);
	str = boaGetVar(wp, "wanif", "");
	if (str[0])
	{
		unsigned int wan_ifindex = atoi(str);
		ifGetName(wan_ifindex, wanifname, sizeof(wanifname));
	}
	if(wanifname[0])
	{
		snprintf(cmd, sizeof(cmd), "ping -4 -c 4 -I %s %s > /tmp/ping.tmp 2>&1",wanifname, ipaddr);
	}
	else
	{
		snprintf(cmd, sizeof(cmd), "ping -4 -c 4 %s > /tmp/ping.tmp 2>&1", ipaddr);
	}

	va_cmd("/bin/sh", 2, 0, "-c", cmd);
	if (!strncmp(strPingAct, "Start", 5)) {
		wp->buffer_end=0; // clear header
		boaWrite(wp, "HTTP/1.0 204 No Content\n");
		boaWrite(wp, "Pragma: no-cache\n");
		boaWrite(wp, "Cache-Control: no-cache\n");
		boaWrite(wp, "\n");
	}
	else if (!strncmp(strPingAct, "Stop", 4)) {
		va_cmd("/bin/killall", 1, 1, "ping");
		unlink("/tmp/ping.tmp");
		submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
		if (submitUrl[0])
			boaRedirect(wp, submitUrl);
		else
			boaDone(wp, 200);
	}
	return;
}

#ifdef CONFIG_IPV6
void formPing6(request * wp, char *path, char *query)
{
	char *ipaddr = NULL;
	char cmd[512] = {0};
	char *str, *strPingAct, *submitUrl, wanifname[IFNAMSIZ]={0};

	va_cmd("/bin/killall", 1, 1, "ping");
	unlink("/tmp/ping.tmp");

	strPingAct = boaGetVar(wp, "pingAct", "");
	ipaddr = boaGetVar(wp, "pingAddr", "");
	if (!ipaddr[0]) {
		ERR_MSG("Wrong IP address!");
		return;
	}

	if (isValidPing6AddressValue(ipaddr) != 0)
	{
		ERR_MSG("Wrong IP address!");
		return;
	}

	//printf("%s IP address %s\n", __func__, ipaddr);
	str = boaGetVar(wp, "wanif", "");
	if (str[0]) 
	{
		unsigned int wan_ifindex = atoi(str);
		ifGetName(wan_ifindex, wanifname, sizeof(wanifname));		
	}
	if(wanifname[0])
	{
		snprintf(cmd, sizeof(cmd), "ping6 -c 4 -I %s %s > /tmp/ping.tmp 2>&1",wanifname, ipaddr);
	}
	else
	{
		snprintf(cmd, sizeof(cmd), "ping6 -c 4 %s > /tmp/ping.tmp 2>&1", ipaddr);
	}

	va_cmd("/bin/sh", 2, 0, "-c", cmd);
	if (!strncmp(strPingAct, "Start", 5)) {
		wp->buffer_end=0; // clear header
		boaWrite(wp, "HTTP/1.0 204 No Content\n");
		boaWrite(wp, "Pragma: no-cache\n");
		boaWrite(wp, "Cache-Control: no-cache\n");
		boaWrite(wp, "\n");
	}
	else if (!strncmp(strPingAct, "Stop", 4)) {
		va_cmd("/bin/killall", 1, 1, "ping6");
		unlink("/tmp/ping.tmp");
		submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
		if (submitUrl[0])
			boaRedirect(wp, submitUrl);
		else
			boaDone(wp, 200);
	}
	return;
}
#endif

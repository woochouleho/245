/*
 *      Web server handler routines for Tracert diagnostic stuffs
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
#include <unistd.h>

/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "../defs.h"

#ifdef CONFIG_ADV_SETTING
#include <sys/mman.h>  
#include <sys/types.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <stdio.h>  
#include <stdlib.h> 
#endif

int clearTracertResult(int eid, request * wp, int argc, char **argv)
{
	va_cmd("/bin/killall", 1, 1, "traceroute");
	va_cmd("/bin/killall", 1, 1, "traceroute6");
	unlink("/tmp/tracert.tmp");
	return 0;
}

void formTracertResult(request * wp, char *path, char *query) {
	char line[512] = {0};
	FILE *pf = NULL;
	int nBytesSent=0;

	pf = fopen("/tmp/tracert.tmp", "r");
	if(!pf) {
		//printf("open /tmp/tracert.tmp fail.\n");
		return;
	}
	while (fgets(line, sizeof(line), pf)) {
		nBytesSent += boaWrite(wp, "<tr><td class=\"intro_content\">%s</td></tr>", line);
	}
	fclose(pf);
	return;
}

void formTracert(request * wp, char *path, char *query)
{
	char *domainaddr, *strTracertAct, *submitUrl;
	char line[512] = {0}, cmd[512] = {0};
	FILE *pf = NULL;
	char *str;
	int len;

	va_cmd("/bin/killall", 1, 1, "traceroute");
	unlink("/tmp/tracert.tmp");

	strTracertAct = boaGetVar(wp, "tracertAct", "");
	domainaddr = boaGetVar(wp, "traceAddr", "");
	if (!domainaddr[0]) {
		ERR_MSG("wrong domain name!");
		return;
	}

	if (isValidPingAddressValue(domainaddr) != 0)
	{
		ERR_MSG("Wrong domain name!");
		return;
	}

	str = boaGetVar(wp, "proto", "");
	if(str[0] && str[0]=='1')
		snprintf(cmd, sizeof(cmd), "traceroute -4");
	else
		snprintf(cmd, sizeof(cmd), "traceroute -4 -I");
	str = boaGetVar(wp, "trys", "");
	if(str[0])
	{	
		len = sizeof(cmd)-strlen(cmd);
		if(snprintf(cmd+strlen(cmd), len, " -q %s", str) >= len){
			printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
		}
	}
	str = boaGetVar(wp, "timeout", "");
	if(str[0])
	{	
		len = sizeof(cmd)-strlen(cmd);
		if(snprintf(cmd+strlen(cmd), len, " -w %s", str) >= len){
			printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
		}
	}
	str = boaGetVar(wp, "dscp", "");
	if(str[0])
	{
		unsigned char dscp_val = atoi(str);
		len = sizeof(cmd)-strlen(cmd);
		if(snprintf(cmd+strlen(cmd), len, " -t %d", (dscp_val&0x3f)<<2) >= len){
			printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
		}
	}
	str = boaGetVar(wp, "maxhop", "");
	if(str[0])
	{
		len = sizeof(cmd)-strlen(cmd);
		if(snprintf(cmd+strlen(cmd), len, " -m %s", str) >= len){
			printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
		}
	}
	str = boaGetVar(wp, "wanif", "");
	if(str[0])
	{
		unsigned int wan_ifindex = atoi(str);
		char wanifname[IFNAMSIZ] = {0};

		ifGetName(wan_ifindex, wanifname, sizeof(wanifname));
		if(wanifname[0]){
			len = sizeof(cmd)-strlen(cmd);
			if(snprintf(cmd+strlen(cmd), len, " -i %s", wanifname) >= len){
				printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
			}
		}
	}
	len = sizeof(cmd)-strlen(cmd);
	if(snprintf(cmd+strlen(cmd), len, " %s", domainaddr) >= len){
		printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
	}
	str = boaGetVar(wp, "datasize", "");
	if(str[0])
	{
		len = sizeof(cmd)-strlen(cmd);
		if(snprintf(cmd+strlen(cmd), len, " %s", str) >= len){
			printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
		}
	}
	len = sizeof(cmd)-strlen(cmd);
	if(snprintf(cmd+strlen(cmd), len, " > /tmp/tracert.tmp 2>&1") >= len){
		printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
	}

	va_cmd("/bin/sh", 2, 0, "-c", cmd);
	if (!strncmp(strTracertAct, "Start", 5)) {
		wp->buffer_end=0; // clear header
		boaWrite(wp, "HTTP/1.0 204 No Content\n");
		boaWrite(wp, "Pragma: no-cache\n");
		boaWrite(wp, "Cache-Control: no-cache\n");
		boaWrite(wp, "\n");
	}
	else if (!strncmp(strTracertAct, "Stop", 4)) {
		va_cmd("/bin/killall", 1, 1, "traceroute");
		unlink("/tmp/tracert.tmp");
		submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
		if (submitUrl[0])
			boaRedirect(wp, submitUrl);
		else
			boaDone(wp, 200);
	}
	return;
}

void formTracert6(request * wp, char *path, char *query)
{
	char *domainaddr, *strTracertAct, *submitUrl;
	char line[512] = {0}, cmd[512] = {0};
	FILE *pf = NULL;
	char *str;
	int len;

	va_cmd("/bin/killall", 1, 1, "traceroute6");
	unlink("/tmp/tracert.tmp");

	strTracertAct = boaGetVar(wp, "tracertAct", "");
	domainaddr = boaGetVar(wp, "traceAddr", "");
	if ((!domainaddr[0]) ||(strContainXSSChar(domainaddr))){
		ERR_MSG("wrong domain name!");
		return;
	}
	//printf("%s domain %s\n", __func__, domainaddr);
	
	if (isValidPing6AddressValue(domainaddr) != 0)
	{
		ERR_MSG("Wrong domain name!");
		return;
	}

	snprintf(cmd, sizeof(cmd), "traceroute6");
	str = boaGetVar(wp, "trys", "");
	if(str[0])
	{
		len = sizeof(cmd)-strlen(cmd);
		if(snprintf(cmd+strlen(cmd), len, " -q %s", str) >= len){
			printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
		}
	}
	str = boaGetVar(wp, "timeout", "");
	if(str[0])
	{
		len = sizeof(cmd)-strlen(cmd);
		if(snprintf(cmd+strlen(cmd), len, " -w %s", str) >= len){
			printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
		}
	}
	str = boaGetVar(wp, "maxhop", "");
	if(str[0])
	{
		len = sizeof(cmd)-strlen(cmd);
		if(snprintf(cmd+strlen(cmd), len, " -m %s", str) >= len){
			printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
		}
	}
	str = boaGetVar(wp, "wanif", "");
	if(str[0])
	{
		unsigned int wan_ifindex = atoi(str);
		char wanifname[IFNAMSIZ] = {0};

		ifGetName(wan_ifindex, wanifname, sizeof(wanifname));
		if(wanifname[0]){
			len = sizeof(cmd)-strlen(cmd);
			if(snprintf(cmd+strlen(cmd), len, " -i %s", wanifname) >= len){
				printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
			}
		}
	}
	len = sizeof(cmd)-strlen(cmd);
	if(snprintf(cmd+strlen(cmd), len, " %s", domainaddr) >= len){
		printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
	}
	str = boaGetVar(wp, "datasize", "");
	if(str[0])
	{
		len = sizeof(cmd)-strlen(cmd);
		if(snprintf(cmd+strlen(cmd), len, " %s", str) >= len){
			printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
		}
	}
	len = sizeof(cmd)-strlen(cmd);
	if(snprintf(cmd+strlen(cmd), len, " > /tmp/tracert.tmp 2>&1") >= len){
		printf("[%s %d]warning, string truncated\n",__FUNCTION__,__LINE__);
	}

	va_cmd("/bin/sh", 2, 0, "-c", cmd);
	if (!strncmp(strTracertAct, "Start", 5)) {
		wp->buffer_end=0; // clear header
		boaWrite(wp, "HTTP/1.0 204 No Content\n");
		boaWrite(wp, "Pragma: no-cache\n");
		boaWrite(wp, "Cache-Control: no-cache\n");
		boaWrite(wp, "\n");
	}
	else if (!strncmp(strTracertAct, "Stop", 4)) {
		va_cmd("/bin/killall", 1, 1, "traceroute");
		unlink("/tmp/tracert.tmp");
		submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
		if (submitUrl[0])
			boaRedirect(wp, submitUrl);
		else
			boaDone(wp, 200);
	}
	return;
}


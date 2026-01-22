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

#include "../webs.h"
#include "webform.h"
#include "../defs.h"
#include "boa.h"
#include "multilang.h"

int sshPortGet(int eid, request * wp, int argc, char **argv)
{   
	int port;

	if (!mib_get_s(MIB_SSH_PORT_NUMBER, (void *)&port, 4))
		return -1;

	boaWrite(wp, "<tr><th>%s :</th>\n", multilang(LANG_SSH_PORT));
	boaWrite(wp, "<td><input type='text' name='sshPort' value=\"%d\" size='20' maxlength='30'></td></tr>\n", port);

	return 0;
}


int clearSshResult(int eid, request * wp, int argc, char **argv)
{
	return 0;
}

void formSshResult(request * wp, char *path, char *query) {
	int nBytesSent = 0;

	nBytesSent += boaWrite(wp, "<tr><td class=\"intro_content\">ssh start success</td></tr>");
	return;
}

int is_numeric_string(const char *str) 
{
	while (*str) 
	{
		if (!isdigit(*str)) 
			return 0; /* 문자열에 숫자가 아닌 값이 포함된 경우 */
		str++;
	}
	return 1; /* 문자열이 숫자만 포함하는 경우 */
}

void formSsh(request * wp, char *path, char *query)
{
	char cmd[64] = {0};
	char lan_ip[16] = {0};
	char wan_ip[16] = {0};
	char *strAct, *submitUrl, *strPort;
	int act, sshPort;

	strAct = boaGetVar(wp, "sshAct", "");

	if (!strncmp(strAct, "Enable", 6)) {
		memset(wan_ip, 0, sizeof(wan_ip));
		memset(lan_ip, 0, sizeof(lan_ip));

		if (deo_wan_nat_mode_get() != DEO_BRIDGE_MODE) {
			get_ipv4_from_interface(lan_ip, 1);
			get_ipv4_from_interface(wan_ip, 2);
		} else {
			get_ipv4_from_interface(lan_ip, 3);
			get_ipv4_from_interface(wan_ip, 1);
		}

		strPort = boaGetVar(wp, "sshPort", "");
		if (strPort[0] != 0) 
		{
			if (!is_numeric_string(strPort)) {
				boaWrite(wp, "ERROR: Please check the value again\n");
				return;
			}

			sshPort = atoi(strPort);
			if ((sshPort < 1) || (sshPort > 65535)) {
				boaWrite(wp, "ERROR: Invalid port number\n");
				return;
			}
		}

		if (findPidByName("dropbear") > 0)
			va_cmd("/bin/killall", 1, 1, "dropbear");

		snprintf(cmd, sizeof(cmd), "/bin/dropbear -F -p %s:%d &", wan_ip, sshPort);
		system(cmd);

		memset(cmd, 0, sizeof(cmd));
		snprintf(cmd, sizeof(cmd), "/bin/dropbear -F -p %s:%d &", lan_ip, sshPort);
		system(cmd);

		// allow tcp 22 port from FC
		system("echo ssh_mode 0 > /proc/driver/realtek/ssh_mode &");
		act = 1;
		mib_set(MIB_SSH_ENABLE, (void *)&act);
		mib_set(MIB_SSH_PORT_NUMBER, (void *)&sshPort);

		wp->buffer_end = 0; // clear header
		boaWrite(wp, "HTTP/1.0 204 No Content\n");
		boaWrite(wp, "Pragma: no-cache\n");
		boaWrite(wp, "Cache-Control: no-cache\n");
		boaWrite(wp, "\n");

		syslog2(LOG_DM_HTTP, LOG_DM_HTTP_SSH_SET, "Set SSH Enable");

	} else if (!strncmp(strAct, "Disable", 7)) {
		if (findPidByName("dropbear") > 0)
			va_cmd("/bin/killall", 1, 1, "dropbear");

		// block tcp 22 port from FC
		system("echo ssh_mode 1 > /proc/driver/realtek/ssh_mode &");
		act = 0;
		mib_set(MIB_SSH_ENABLE, (void *)&act);

		submitUrl = boaGetVar(wp, "submit-url", "");

		if (submitUrl[0])
			boaRedirect(wp, submitUrl);
		else
			boaDone(wp, 200);

		syslog2(LOG_DM_HTTP, LOG_DM_HTTP_SSH_SET, "Set SSH Disable");
	}

	return;
}


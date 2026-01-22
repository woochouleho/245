/*
 *      Web server handler routines for Commit/reboot stuffs
 *
 */


/*-- System inlcude files --*/
/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "syslog2d.h"
//alex_huang
#ifdef EMBED
#include <linux/config.h>
#include <config/autoconf.h>
#else
#include "../../../../include/linux/autoconf.h"
#include "../../../../config/autoconf.h"
#endif

void formReboot(request * wp, char *path, char *query)
{
	char tmpBuf[100];
	unsigned char vChar;
	int k;
#ifndef NO_ACTION
	int pid;
#endif
	boaHeader(wp);
	//--- Add timer countdown
	MSG_COUNTDOWN(multilang(LANG_REBOOT_WORD0), REBOOT_TIME, NULL);
	//--- End of timer countdown
	boaWrite(wp, "<body><blockquote>\n");
	boaWrite(wp, "%s<br><br>\n", multilang(LANG_REBOOT_WORD1));
	boaWrite(wp, "%s\n", multilang(LANG_REBOOT_WORD2));
	boaWrite(wp, "</blockquote></body>");
	boaFooter(wp);
	boaDone(wp, 200);

#ifdef EMBED
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
#endif

#if defined(CONFIG_00R0) && defined(CONFIG_USER_RTK_VOIP)
//send unregister to sip server 
	char strbuf[128]={0};
	sprintf(strbuf, "echo j > /var/run/solar_control.fifo");
	va_cmd("/bin/sh", 2, 0, "-c", strbuf);
	sleep(3);
#endif
#if defined(CONFIG_AIRTEL_EMBEDUR)
	write_omd_reboot_log(ADMIN_FLAG);
#endif
	syslog2(LOG_DM_HTTP, LOG_DM_HTTP_RESET, "Reboot by Web");
	cmd_reboot();

#ifndef NO_ACTION
	pid = fork();
        if (pid)
                waitpid(pid, NULL, 0);
        else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _CONFIG_SCRIPT_PROG);
#ifdef HOME_GATEWAY
		execl( tmpBuf, _CONFIG_SCRIPT_PROG, "gw", "bridge", NULL);
#else
		execl( tmpBuf, _CONFIG_SCRIPT_PROG, "ap", "bridge", NULL);
#endif
                exit(1);
        }
#endif

  	return;
}

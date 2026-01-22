#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
 
#ifdef EMBED
#include <linux/config.h>
#include <config/autoconf.h>
#else
#include "../../../../include/linux/autoconf.h"
#include "../../../../config/autoconf.h"
#endif
#include "utility.h"

int main (int argc, char **argv)
{
#if defined(CONFIG_RTL9607C_SERIES)
	va_cmd("/bin/sh", 2, 1, "-c", "ifconfig eth0 up");
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	unsigned int pon_mode = 0;
	mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));
#if defined(CONFIG_GPON_FEATURE)
	if(pon_mode==GPON_MODE)
		checkOMCI_startup();
#endif
#if defined(CONFIG_EPON_FEATURE)
	if(pon_mode==EPON_MODE)
		rtk_pon_sync_epon_llid_entry();
#endif
	//printf("==== check omci mib\n");
#endif
	return 0;
}



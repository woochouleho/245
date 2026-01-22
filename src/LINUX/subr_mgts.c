#include <stdio.h>
#include <stdlib.h>
#include "utility.h"

#ifdef HANDLE_DEVICE_EVENT
int rtk_mgts_pushbtn_reset_action(int action, int btn_test, int diff_time)
{
	static int reset = 0;
	if (diff_time >20) {
		printf("Reset button reset to default\n");
		if(!btn_test && !reset)
		{
			printf("Reset button reset to default excute\n");
			//LED flash while factory reset
			//system("echo 2 > /proc/load_default");
			reset = 1;
			reset_cs_to_default(0);
			cmd_reboot();
		}
	}
}
#endif

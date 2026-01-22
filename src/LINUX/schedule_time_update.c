#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "utility.h"
#include "mib.h"
extern void firewall_schedule();
int main (int argc, char **argv)
{
	syslog(LOG_INFO, "firewall_schedule: update time schedule rules from crontab!");
	firewall_schedule();
	return 0;
}


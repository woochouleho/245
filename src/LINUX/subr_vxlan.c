#include <string.h>
#include <unistd.h>
#include "mib.h"
#include "utility.h"
#include <netinet/in.h>
#include <linux/if.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/file.h>
#include "subr_net.h"


#ifdef EMBED
#include <linux/config.h>
#include <config/autoconf.h>
#else
#include "../../../../include/linux/autoconf.h"
#include "../../../../config/autoconf.h"
#endif

extern const char BIN_IP[];
extern const char PORTMAP_EBTBL[];
extern const char PORTMAP_SEVTBL[];
extern const char FW_MACFILTER_FWD[];
extern const char FW_FORWARD[];

int vxlan_fid,lockfd;
const char VXLAN_DECONFIG_SCRIPT[] = "/var/tmp/vxlan_deconfig";
const char VXLAN_DECONFIG_POLICYROUTE_SCRIPT[] = "/var/tmp/vxlan_deconfig_policyroute";
const char VXLAN_INTERFACE[] = "vxlan";
const char VXLAN_CMD[] = "/bin/ip";

#define ConfigVxlanLock "/var/run/configVxlanLock"
#define LOCK_VXLAN()	\
do {	\
	if ((lockfd = open(ConfigVxlanLock, O_RDWR)) == -1) {	\
		perror("open vxlan lockfile");	\
		return 0;	\
	}	\
	while (flock(lockfd, LOCK_EX)) { \
		if (errno != EINTR) \
			break; \
	}	\
} while (0)

#define UNLOCK_VXLAN()	\
do {	\
	flock(lockfd, LOCK_UN);	\
	close(lockfd);	\
} while (0)

static void CLOSE_VXLAN_DECONFIG_SCRIPT()
{
	close (vxlan_fid);
}

int reset_vxlan_rule(char* script_name)
{
	va_cmd(script_name, 0, 1);
	return 0;
}

int write_to_vxlan_dconfig(int num, ...)
{
	va_list ap;
	int k, len = 0;
	char *s;
	char buf[256];
	char ifname[MAXFNAME]={0};

	va_start(ap, num);
 	buf[0] = '\0';
	for (k=0; k<num; k++) {
		s = va_arg(ap, char *);
		len = strlen(buf);
		if (len)
			sprintf(buf+len, " %s", s);
		else
			sprintf(buf, "%s", s);
	}
	va_end(ap);
	len = strlen(buf);
	if (len)
		sprintf(buf+len, "\n");
 	write(vxlan_fid, buf, strlen(buf));
	return 0;
}

static void RESET_VXLAN_DECONFIG_SCRIPT(char* ifname)
{
	do {
		vxlan_fid=open(ifname, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);
		write_to_vxlan_dconfig(1, "#!/bin/sh");
	} while (0);
}

int  check_vxlan_portmp(MIB_CE_VXLAN_Tp vxlan_entry)
{
	int totalEntry=0, i=0, j=0;
	MIB_CE_ATM_VC_T wan_Entry;
	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i < totalEntry; ++i)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&wan_Entry))
		{
			if (!wan_Entry.enable)
				continue;
			for(j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j)
			{
				if( BIT_IS_SET(vxlan_entry->lan_interface, j) && BIT_IS_SET(wan_Entry.itfGroup, j)) {
					printf("check_vxlan_portmp error! vxlan(%d) using the same portmapping with WAN(%d)\n", 
						vxlan_entry->mib_idx, wan_Entry.ifIndex);
					return -1;
				}
			}
#ifdef WLAN_SUPPORT
			for(j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP3; ++j)
			{
				if(BIT_IS_SET(vxlan_entry->lan_interface, j) && BIT_IS_SET(wan_Entry.itfGroup, j)) {
					printf("check_vxlan_portmp error! vxlan(%d) using the same portmapping with WAN(%d)\n", 
						vxlan_entry->mib_idx, wan_Entry.ifIndex);
					return -1;
				}
			}
#endif
		}
	}
	return 0;
}

int exec_vxlan_portmp()
{
	int total=0, i=0, j=0, hit_entry = 0;
	unsigned int wan_mark = 0, wan_mask = 0;
	char vxlan_if[IFNAMSIZ+1] = {0}, vxlan_script[MAXFNAME], ppp_ifname[IFNAMSIZ+1];
	char chain_in[64], chain_out[64], polmark[64];
	MIB_CE_VXLAN_T vxlan_entry;

	LOCK_VXLAN();
	 total = mib_chain_total(MIB_VXLAN_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_VXLAN_TBL, i, (void *)&vxlan_entry)) {
			UNLOCK_VXLAN();
			return -1;
		}
		snprintf(vxlan_script, sizeof(vxlan_script), "%s_%d", VXLAN_DECONFIG_SCRIPT, vxlan_entry.ifIndex);

		reset_vxlan_rule(vxlan_script);
		RESET_VXLAN_DECONFIG_SCRIPT(vxlan_script);

	 	if (!vxlan_entry.enable) continue;

		if(check_vxlan_portmp(&vxlan_entry)<0)
		{
			printf("[%s:%d] ERROR! VXLAN lanInetrace(0x%x) conflict with WAN!\n",  __FUNCTION__, __LINE__, vxlan_entry.lan_interface);
			continue;
		}

		if (vxlan_entry.vlan_enable) {
			sprintf(vxlan_if, "%s%d.%d", VXLAN_INTERFACE, vxlan_entry.mib_idx, vxlan_entry.vlan);
		}
		else
			sprintf(vxlan_if, "%s%d", VXLAN_INTERFACE, vxlan_entry.mib_idx);

		sprintf(chain_in, "%s_%s_in", PORTMAP_EBTBL, vxlan_if);
		sprintf(chain_out, "%s_%s_out", PORTMAP_EBTBL, vxlan_if);

#if 1
		if (rtk_net_get_wan_policy_route_mark(vxlan_if, &wan_mark, &wan_mask) == -1)
			rtk_net_add_wan_policy_route_mark(vxlan_if, &wan_mark, &wan_mask);
#else
		wan_mark = SOCK_MARK_WAN_VXLAN_INDEX_TO_MARK(vxlan_entry.mib_idx);
		wan_mask = SOCK_MARK_WAN_INDEX_BIT_MASK;
#endif

		va_cmd(EBTABLES, 2, 1, "-N", chain_in);
		va_cmd(EBTABLES, 3, 1, "-P", chain_in, "DROP");
		va_cmd(EBTABLES, 2, 1, "-N", chain_out);
		va_cmd(EBTABLES, 3, 1, "-P", chain_out, "DROP");

		for(j = PMAP_ETH0_SW0; j < PMAP_WLAN0 && j < ELANVIF_NUM; ++j)
		{
			if(BIT_IS_SET(vxlan_entry.lan_interface, j))
			{
				hit_entry = 1;
				va_cmd(EBTABLES, 6, 1, "-I", chain_in, "-o", ELANVIF[j], "-j", "RETURN");
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", chain_in, "-o", ELANVIF[j], "-j", "RETURN");

				va_cmd(EBTABLES, 6, 1, "-I", chain_out, "-i", ELANVIF[j], "-j", "RETURN");
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", chain_out, "-i", ELANVIF[j], "-j", "RETURN");
				if(vxlan_entry.work_mode == VXLAN_WORKMODE_BRIDGE)
				{
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_INTBL, "-i", ELANVIF[j], "-j", PORTMAP_SEVTBL);
					write_to_vxlan_dconfig(7,  EBTABLES, "-D", PORTMAP_INTBL, "-i", ELANVIF[j], "-j", PORTMAP_SEVTBL);

					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", ELANVIF[j], "-j", PORTMAP_SEVTBL);
					write_to_vxlan_dconfig(7,  EBTABLES, "-D",  PORTMAP_OUTTBL, "-o", ELANVIF[j], "-j", PORTMAP_SEVTBL);
				}
				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", ELANVIF[j], "-j", chain_out);
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", PORTMAP_OUTTBL, "-o", ELANVIF[j], "-j", chain_out);

				sprintf(polmark, "0x%x", wan_mark);
				va_cmd(EBTABLES, 12, 1, "-t", "broute", "-A", PORTMAP_BRTBL, "-i", ELANVIF[j], "-j", "mark", "--mark-or", polmark, "--mark-target", "CONTINUE");
				write_to_vxlan_dconfig(13,  EBTABLES, "-t", "broute", "-D", PORTMAP_BRTBL, "-i", ELANVIF[j], "-j", "mark", "--mark-or", polmark, "--mark-target", "CONTINUE");

				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-i", ELANVIF[j], "-j", chain_in);
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", PORTMAP_EBTBL, "-i", ELANVIF[j], "-j", chain_in);

				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-o", ELANVIF[j], "-j", chain_out);
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", PORTMAP_EBTBL, "-o", ELANVIF[j], "-j", chain_out);
			}
		}
#ifdef WLAN_SUPPORT
		for(j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP_END; ++j)
		{
			if(BIT_IS_SET(vxlan_entry.lan_interface, j))
			{
				hit_entry = 1;
				va_cmd(EBTABLES, 6, 1, "-I", chain_in, "-o", wlan[j-PMAP_WLAN0], "-j", "RETURN");
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", chain_in, "-o", wlan[j-PMAP_WLAN0], "-j", "RETURN");

				va_cmd(EBTABLES, 6, 1, "-I", chain_out, "-i", wlan[j-PMAP_WLAN0], "-j", "RETURN");
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", chain_out, "-i", wlan[j-PMAP_WLAN0], "-j", "RETURN");
				if(vxlan_entry.work_mode == VXLAN_WORKMODE_BRIDGE)
				{
					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_INTBL, "-i", wlan[j-PMAP_WLAN0], "-j", PORTMAP_SEVTBL);
					write_to_vxlan_dconfig(7,  EBTABLES, "-D", PORTMAP_INTBL, "-i", wlan[j-PMAP_WLAN0], "-j", PORTMAP_SEVTBL);

					va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", wlan[j-PMAP_WLAN0], "-j", PORTMAP_SEVTBL);
					write_to_vxlan_dconfig(7,  EBTABLES, "-D",  PORTMAP_OUTTBL, "-o", wlan[j-PMAP_WLAN0], "-j", PORTMAP_SEVTBL);
				}
				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", wlan[j-PMAP_WLAN0], "-j", chain_out);
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", PORTMAP_OUTTBL, "-o", wlan[j-PMAP_WLAN0], "-j", chain_out);

				sprintf(polmark, "0x%x", wan_mark);
				va_cmd(EBTABLES, 12, 1, "-t", "broute", "-A", PORTMAP_BRTBL, "-i", wlan[j-PMAP_WLAN0], "-j", "mark", "--mark-or", polmark, "--mark-target", "CONTINUE");
				write_to_vxlan_dconfig(13,  EBTABLES, "-t", "broute", "-D", PORTMAP_BRTBL, "-i", wlan[j-PMAP_WLAN0], "-j", "mark", "--mark-or", polmark, "--mark-target", "CONTINUE");

				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-i", wlan[j-PMAP_WLAN0], "-j", chain_in);
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", PORTMAP_EBTBL, "-i", wlan[j-PMAP_WLAN0], "-j", chain_in);

				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-o", wlan[j-PMAP_WLAN0], "-j", chain_out);
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", PORTMAP_EBTBL, "-o", wlan[j-PMAP_WLAN0], "-j", chain_out);
			}
		}
#endif

		if(hit_entry)
		{
			if(vxlan_entry.work_mode == VXLAN_WORKMODE_BRIDGE)
			{
				va_cmd(EBTABLES, 3, 1, "-p", FW_MACFILTER_FWD, FW_RETURN);

				va_cmd(EBTABLES, 6, 1, "-I", chain_out, "-i", vxlan_if, "-j", "RETURN");
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", chain_out, "-i", vxlan_if, "-j", "RETURN");

				va_cmd(EBTABLES, 6, 1, "-I", chain_in, "-o", vxlan_if, "-j", "RETURN");
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", chain_in, "-o", vxlan_if, "-j", "RETURN");

				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-i", vxlan_if, "-j", chain_in);
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", PORTMAP_EBTBL, "-i", vxlan_if, "-j", chain_in);

				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_EBTBL, "-o", vxlan_if, "-j", chain_out);
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", PORTMAP_EBTBL, "-o", vxlan_if, "-j", chain_out);

				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_INTBL, "-i", vxlan_if, "-j", PORTMAP_SEVTBL);
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", PORTMAP_INTBL, "-i", vxlan_if, "-j", PORTMAP_SEVTBL);

				va_cmd(EBTABLES, 6, 1, "-A", PORTMAP_OUTTBL, "-o", vxlan_if, "-j", PORTMAP_SEVTBL);
				write_to_vxlan_dconfig(7,  EBTABLES, "-D", PORTMAP_OUTTBL, "-o", vxlan_if, "-j", PORTMAP_SEVTBL);

				sprintf(polmark, "0x%x", wan_mark);
				va_cmd(EBTABLES, 12, 1, "-t", "broute", "-A", PORTMAP_BRTBL, "-i", vxlan_if, "-j", "mark", "--mark-or", polmark, "--mark-target", "CONTINUE");
				write_to_vxlan_dconfig(13,  EBTABLES, "-t", "broute", "-D", PORTMAP_BRTBL, "-i", vxlan_if, "-j", "mark", "--mark-or", polmark, "--mark-target", "CONTINUE");
			}

			//for none binding to binding LAN port
			sprintf(polmark, "0x%x/0x%x", 0, wan_mask);
			va_cmd(EBTABLES, 12, 1, "-A", (char *)chain_out, "-i", "!", "vxlan+", "--mark", polmark, "-o", "!", "vxlan+", "-j", "RETURN");
			write_to_vxlan_dconfig(13, EBTABLES, "-D", (char *)chain_out, "-i", "!", "vxlan+", "--mark", polmark, "-o", "!", "vxlan+", "-j", "RETURN");

			va_cmd(EBTABLES, 10, 1, "-A", chain_in, "-i", "!", "vxlan+", "-o", "!", "vxlan+", "-j", "RETURN");
			write_to_vxlan_dconfig(11, EBTABLES, "-D", chain_in, "-i", "!", "vxlan+", "-o", "!", "vxlan+", "-j", "RETURN");
			//for binding MARK
			sprintf(polmark, "0x%x/0x%x", wan_mark, wan_mask);
			va_cmd(EBTABLES, 6, 1, "-A", (char *)chain_out, "--mark", polmark, "-j", "RETURN");
			write_to_vxlan_dconfig(7, EBTABLES, "-D", (char *)chain_out, "--mark", polmark, "-j", "RETURN");
		}

		//for routing WAN downstream mark
		sprintf(polmark, "0x%x/0x%x", wan_mark, wan_mask);
		va_cmd(IPTABLES, 10, 1, "-t", "mangle", "-A", (char *)PORTMAP_RTTBL, "-i", vxlan_if, "-j", "MARK", "--set-mark", polmark);
		write_to_vxlan_dconfig(11, IPTABLES, "-t", "mangle", "-D", (char *)PORTMAP_RTTBL, "-i", vxlan_if, "-j", "MARK", "--set-mark", polmark);

		//iptables -t mangle -A FORWARD -o vxlan0 -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu
		va_cmd(IPTABLES, 14, 1, "-t", "mangle", "-A", (char *)FW_FORWARD, "-o", vxlan_if, "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", "-j", "TCPMSS", "--clamp-mss-to-pmtu");
		write_to_vxlan_dconfig(15, IPTABLES, "-t", "mangle",  "-D", (char *)FW_FORWARD, "-o", vxlan_if, "-p", "tcp", "--tcp-flags",
				"SYN,RST", "SYN", "-j", "TCPMSS", "--clamp-mss-to-pmtu");

#if defined(CONFIG_IPV6)
		va_cmd(IP6TABLES, 10, 1, "-t", "mangle", "-A", (char *)PORTMAP_RTTBL, "-i", vxlan_if, "-j", "MARK", "--set-mark", polmark);
		write_to_vxlan_dconfig(11, IP6TABLES, "-t", "mangle", "-D", (char *)PORTMAP_RTTBL, "-i", vxlan_if, "-j", "MARK", "--set-mark", polmark);

		//ip6tables -t mangle -A FORWARD -o vxlan0 -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu
		va_cmd(IP6TABLES, 14, 1, "-t", "mangle", "-A", (char *)FW_FORWARD, "-o", vxlan_if, "-p", "tcp", "--tcp-flags",	"SYN,RST", "SYN", "-j", "TCPMSS", "--clamp-mss-to-pmtu");
		write_to_vxlan_dconfig(15, IP6TABLES, "-t", "mangle",  "-D", (char *)FW_FORWARD, "-o", vxlan_if, "-p", "tcp", "--tcp-flags",
				"SYN,RST",	"SYN", "-j", "TCPMSS", "--clamp-mss-to-pmtu");

		va_cmd(IP6TABLES, 6, 1, "-I", (char *)FW_IPV6FILTER, "-i", vxlan_if, "-j", "RETURN");
		write_to_vxlan_dconfig(7, IP6TABLES, "-D", (char *)FW_IPV6FILTER, "-i", vxlan_if, "-j", "RETURN");
#endif
		//va_cmd(EBTABLES, 2, 1, "-F", chain_in);
		write_to_vxlan_dconfig(3, EBTABLES, "-F", chain_in);
		//va_cmd(EBTABLES, 2, 1, "-X", chain_in);
		write_to_vxlan_dconfig(3, EBTABLES, "-X", chain_in);
		//va_cmd(EBTABLES, 2, 1, "-F", chain_out);
		write_to_vxlan_dconfig(3, EBTABLES, "-F", chain_out);
		//va_cmd(EBTABLES, 2, 1, "-X", chain_out);
		write_to_vxlan_dconfig(3, EBTABLES, "-X", chain_out);
		CLOSE_VXLAN_DECONFIG_SCRIPT();
	}
	UNLOCK_VXLAN();
	return 0;
}


int delVxLAN_portmp_rule(char* vxlan_script)
{
	// Clear Portbinding rules
	LOCK_VXLAN();
	reset_vxlan_rule(vxlan_script);
	RESET_VXLAN_DECONFIG_SCRIPT(vxlan_script);
	CLOSE_VXLAN_DECONFIG_SCRIPT();
	UNLOCK_VXLAN();

	return 0;
}


int setupVxLAN_FW_RULES()
{
	int total=0, i=0;
	char vxlan_if[32] = {0}, vxlan_dst_port[8] = {0};
	char ifname[IFNAMSIZ]={0};
	MIB_CE_VXLAN_T Vxlan_entry;
	struct in_addr inAddr;

	 total = mib_chain_total(MIB_VXLAN_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_VXLAN_TBL, i, (void *)&Vxlan_entry))
			return -1;

	 	if (!Vxlan_entry.enable)
			continue;

		ifGetName(Vxlan_entry.wan_interface, ifname, sizeof(ifname));

		if( getInAddr(ifname, IP_ADDR, (void *)&inAddr) != 1)
			continue;

		//vlxan0
		sprintf(vxlan_if, "%s%d", VXLAN_INTERFACE, i);
		if (Vxlan_entry.vlan_enable)
			sprintf(vxlan_if+strlen(vxlan_if), ".%d",  Vxlan_entry.vlan);

		//iptables -A INPUT -i nas0_0 -p UDP --dport 4789 -j ACCEPT
		sprintf(vxlan_dst_port, "%d", VXLAN_DSTPORT);
		va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_INPUT,  "-i", (char *)ifname, "-p", "UDP", "--dport", (char*)vxlan_dst_port, "-j", (char *)FW_ACCEPT);
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_INPUT,  "-i", (char *)ifname, "-p", "UDP", "--dport", (char*)vxlan_dst_port, "-j", (char *)FW_ACCEPT);
		switch(Vxlan_entry.work_mode)
		{
			case VXLAN_WORKMODE_BRIDGE:
				//ebtables -I disBCMC -d Broadcast -o vxlan0 -j ACCEPT
				va_cmd(EBTABLES, 8, 1,  (char*)FW_INSERT, "disBCMC", "-d", "Broadcast", "-o", (char*)vxlan_if, "-j", (char *)FW_ACCEPT );
				break;

			case VXLAN_WORKMODE_ROUTING:
				if (Vxlan_entry.addressing_type == VXLAN_ADDRESSTYPE_DHCP)
				{
				}
				else if (Vxlan_entry.addressing_type == VXLAN_ADDRESSTYPE_STATIC)
				{
				}
				//iptables -A POSTROUTING -t nat -o vxlan0 -j MASQUERADE
				if(Vxlan_entry.nat_enable == VXLAN_NAPT_ENABLED)
					va_cmd(IPTABLES, 8, 1, "-A", "POSTROUTING",  "-t", "nat", "-o", (char*)vxlan_if,  "-j", "MASQUERADE");
				break;

			default:
				break;
		}
	}
	return 0;
}

static void writeDhcpClientScript_vxlan(char *fname)
{
	int fh;
	int mark;
	char buff[64];
	unsigned char lanIP[16]={0};
	fh = open(fname, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);

	if (fh == -1) {
		printf("Create udhcpc script file %s error!\n", fname);
		return;
	}

	WRITE_DHCPC_FILE(fh, "#!/bin/sh\n");
	snprintf(buff, 64, "RESOLV_CONF=\"/var/udhcpc/resolv.conf.$interface\"\n");
	WRITE_DHCPC_FILE(fh, buff);

	WRITE_DHCPC_FILE(fh, "[ \"$broadcast\" ] && BROADCAST=\"broadcast $broadcast\"\n");
	WRITE_DHCPC_FILE(fh, "[ \"$subnet\" ] && NETMASK=\"netmask $subnet\"\n");
	WRITE_DHCPC_FILE(fh, "ifconfig $interface mtu 1500\n");
	WRITE_DHCPC_FILE(fh, "echo interface=$interface\n");
	WRITE_DHCPC_FILE(fh, "echo ciaddr=$ciaddr\n");
	WRITE_DHCPC_FILE(fh, "echo ip=$ip\n");
	WRITE_DHCPC_FILE(fh, "echo subnet=$subnet\n");
	WRITE_DHCPC_FILE(fh, "echo router=$router\n");
	WRITE_DHCPC_FILE(fh, "index=${interface##*vxlan}\n"); //get index from vxlan[0]
	WRITE_DHCPC_FILE(fh, "index2=${index%.*}\n");  //remove vid ifexist
	WRITE_DHCPC_FILE(fh, "echo index=$index\n");
	WRITE_DHCPC_FILE(fh, "echo index2=$index2\n");
	WRITE_DHCPC_FILE(fh, "mib set VXLAN_TBL.$index2.ip_addr $ip\n");
	WRITE_DHCPC_FILE(fh, "mib set VXLAN_TBL.$index2.subnet_mask $subnet\n");
	WRITE_DHCPC_FILE(fh, "mib set VXLAN_TBL.$index2.default_gateway $router\n");
	WRITE_DHCPC_FILE(fh, "mib commit\n");
	WRITE_DHCPC_FILE(fh, "if [ \"$ip\" != \"$ciaddr\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "  echo clear_ip\n");
	WRITE_DHCPC_FILE(fh, "  ifconfig $interface 0.0.0.0\n");
	WRITE_DHCPC_FILE(fh, "fi\n");
	WRITE_DHCPC_FILE(fh, "ifconfig $interface $ip $BROADCAST $NETMASK -pointopoint\n");

	/**************************** Important *****************************/
	/* Should set policy route before write resolv.conf, or there are   */
	/* still some packets may go to wrong pvc.                          */
	/*************************************** ****************************/
	WRITE_DHCPC_FILE(fh, "ip ro add default via $router dev $interface\n\n");
	WRITE_DHCPC_FILE(fh, "if [ \"$dns\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "  rm $RESOLV_CONF\n");
	WRITE_DHCPC_FILE(fh, "  for i in $dns\n");
	WRITE_DHCPC_FILE(fh, "    do\n");
	WRITE_DHCPC_FILE(fh, "      if [ $i = '0.0.0.0' ]|| [ $i = '255.255.255.255' ]; then\n");
	WRITE_DHCPC_FILE(fh, "        continue\n");
	WRITE_DHCPC_FILE(fh, "      fi\n");
	WRITE_DHCPC_FILE(fh, "      echo 'DNS=' $i\n");
	//WRITE_DHCPC_FILE(fh, "\techo nameserver $i >> $RESOLV_CONF\n");
	// echo 192.168.88.21@192.168.99.100
	snprintf(buff, 64, "      echo $i@$ip >> $RESOLV_CONF\n");
	WRITE_DHCPC_FILE(fh, buff);
	WRITE_DHCPC_FILE(fh, "    done\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

	//Microsoft AUTO IP in udhcp-0.9.9-pre2/dhcpc.c, compare first 8 char with "169.254." prefix
	WRITE_DHCPC_FILE(fh, "if [ \"${ip:0:8}\" = \"169.254.\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "  echo \"DHCP sever no response ==> recover to original LAN IP($ORIG_LAN_IP)\"\n");
	WRITE_DHCPC_FILE(fh, "  ifconfig $interface 0.0.0.0\n");
	WRITE_DHCPC_FILE(fh, "  ifconfig $interface $ORIG_LAN_IP\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

	close(fh);
}


int addVxLAN_policyroute_rule(MIB_CE_VXLAN_Tp vxlan_entry)
{
	int i=0,j=0;
	unsigned int wan_mark = 0, wan_mask = 0;
	char vxlan_if[32] = {0},vxlan_local_ipaddr[16]={0}, vxlan_gw_ipaddr[16]={0}, vxlan_policy_id[6]={0};
	char vxlan_script[MAXFNAME], polmark[64],  iprule_pri_policyrt[16] = {0};
	char vxlan_interface_ip6_addr[INET6_ADDRSTRLEN]={0}, vxlan_interface_ip6_gw_addr[INET6_ADDRSTRLEN]={0};
	LOCK_VXLAN();
	snprintf(vxlan_script, sizeof(vxlan_script), "%s_%d", VXLAN_DECONFIG_POLICYROUTE_SCRIPT, vxlan_entry->ifIndex);
	reset_vxlan_rule(vxlan_script);
	RESET_VXLAN_DECONFIG_SCRIPT(vxlan_script);

 	if (vxlan_entry->vlan_enable)
		sprintf(vxlan_if, "%s%d.%d", VXLAN_INTERFACE,  vxlan_entry->mib_idx, vxlan_entry->vlan);
	else
		sprintf(vxlan_if, "%s%d", VXLAN_INTERFACE, vxlan_entry->mib_idx);

#if 1
	if (rtk_net_get_wan_policy_route_mark(vxlan_if, &wan_mark, &wan_mask) == -1)
		rtk_net_add_wan_policy_route_mark(vxlan_if, &wan_mark, &wan_mask);
#else
	wan_mark = SOCK_MARK_WAN_VXLAN_INDEX_TO_MARK(vxlan_entry->mib_idx);
	wan_mask = SOCK_MARK_WAN_INDEX_BIT_MASK;
#endif
	sprintf(polmark, "0x%x/0x%x", wan_mark, wan_mask);
	sprintf(vxlan_policy_id,  "%d", vxlan_entry->ifIndex);

	if(vxlan_entry->IpProtocol == IPVER_IPV6)
	{
		setup_disable_ipv6(vxlan_if, 0);
		if((inet_ntop(PF_INET6, &(vxlan_entry->Ipv6Addr), vxlan_interface_ip6_addr, sizeof(vxlan_interface_ip6_addr))!=NULL) &&
			(inet_ntop(PF_INET6, &(vxlan_entry->RemoteIpv6Addr), vxlan_interface_ip6_gw_addr, sizeof(vxlan_interface_ip6_gw_addr))!=NULL))
		{
			//ip -6 route add default via 3000::2 dev vxlan0 table 589568
			va_cmd(VXLAN_CMD, 10, 1, "-6", "route", "add",  "default", "via", (char*)vxlan_interface_ip6_gw_addr,"dev" , (char*)vxlan_if, "table", (char*)vxlan_policy_id);
			write_to_vxlan_dconfig(11, VXLAN_CMD, "-6", "route", "del",	"default", "via", (char*)vxlan_interface_ip6_gw_addr,"dev" , (char*)vxlan_if, "table", (char*)vxlan_policy_id);

			//ip -6 rule add from 3000::1 lookup 589568
			va_cmd(VXLAN_CMD, 7, 1, "-6", "rule", "add",	"from", (char*)vxlan_interface_ip6_addr,"lookup" , (char*)vxlan_policy_id);
			write_to_vxlan_dconfig(8, VXLAN_CMD, "-6", "rule", "del",	"from", (char*)vxlan_interface_ip6_addr,"lookup" , (char*)vxlan_policy_id);

			sprintf(iprule_pri_policyrt, "%d", IP_RULE_PRI_POLICYRT);
			//ip -6 rule add fwmark 0x3a000/0x3e000 pref 20100 table 589568
			va_cmd(BIN_IP, 9, 1, "-6", "rule", "add", "fwmark", polmark, "pref", (char*)iprule_pri_policyrt, "table", (char*)vxlan_policy_id);
			write_to_vxlan_dconfig(10, BIN_IP, "-6", "rule", "del", "fwmark", polmark, "pref", (char*)iprule_pri_policyrt, "table", (char*)vxlan_policy_id);
		}
	}
	else
	{
		strncpy(vxlan_local_ipaddr, inet_ntoa(*((struct in_addr *)vxlan_entry->ip_addr)), 16);
		strncpy(vxlan_gw_ipaddr, inet_ntoa(*((struct in_addr *)vxlan_entry->default_gateway)), 16);

		//ip route add default via 10.20.1.254 dev vxlan0 table 589568
		va_cmd(VXLAN_CMD, 9, 1, "route", "add",  "default", "via", (char*)vxlan_gw_ipaddr,"dev" , (char*)vxlan_if, "table", (char*)vxlan_policy_id);
		write_to_vxlan_dconfig(10,  VXLAN_CMD, "route", "del",  "default", "via", (char*)vxlan_gw_ipaddr,"dev" , (char*)vxlan_if, "table", (char*)vxlan_policy_id);

		//ip rule add from 10.20.1.1 lookup 589568
		va_cmd(VXLAN_CMD, 6, 1, "rule", "add",	"from", (char*)vxlan_local_ipaddr,"lookup" , (char*)vxlan_policy_id);
		write_to_vxlan_dconfig(7,  VXLAN_CMD, "rule", "del",	"from", (char*)vxlan_local_ipaddr,"lookup" , (char*)vxlan_policy_id);

		sprintf(iprule_pri_policyrt, "%d", IP_RULE_PRI_POLICYRT);
		//ip rule add fwmark 0x3a000/0x3e000 pref 20100 table 589568
		va_cmd(BIN_IP, 8, 1, "rule", "add", "fwmark", polmark, "pref", (char*)iprule_pri_policyrt, "table", (char*)vxlan_policy_id);
		write_to_vxlan_dconfig(9, BIN_IP, "rule", "del", "fwmark", polmark, "pref", (char*)iprule_pri_policyrt, "table", (char*)vxlan_policy_id);
	}
	CLOSE_VXLAN_DECONFIG_SCRIPT();
	UNLOCK_VXLAN();

	return 0;
}

int delVxLAN_policyroute_rule(char* vxlan_script)
{
 	LOCK_VXLAN();
	reset_vxlan_rule(vxlan_script);
	RESET_VXLAN_DECONFIG_SCRIPT(vxlan_script);
	CLOSE_VXLAN_DECONFIG_SCRIPT();
	UNLOCK_VXLAN();
 	return 0;
}

int delVxLAN_for_spec_rule(MIB_CE_VXLAN_Tp vxlan_entry)
{
	char ifname[IFNAMSIZ]={0};
	char vxlan_if[32] = {0}, vxlan_dst_port[8]={0}, vxlan_local_ipaddr[16]={0}, dhcpc_pid_file[32]={0}, vxlan_policy_id[6]={0};
	char vxlan_script[MAXFNAME]={0};
	struct in_addr inAddr;

	// Clear Portbinding rules
	snprintf(vxlan_script, sizeof(vxlan_script), "%s_%d", VXLAN_DECONFIG_SCRIPT, vxlan_entry->ifIndex);
	delVxLAN_portmp_rule(vxlan_script);

 	// Clear IP policy route
	snprintf(vxlan_script, sizeof(vxlan_script), "%s_%d", VXLAN_DECONFIG_POLICYROUTE_SCRIPT, vxlan_entry->ifIndex);
	delVxLAN_policyroute_rule(vxlan_script);

	ifGetName(vxlan_entry->wan_interface, ifname, sizeof(ifname));
	//vlxan0
	strncpy(vxlan_if, vxlan_entry->vxlan_ifname, sizeof(vxlan_if));

	//ip link del vxlan0
	va_cmd(VXLAN_CMD, 3, 1, "link",  (char *)ARG_DEL, (char *)vxlan_if);

	if (vxlan_entry->vlan_enable) {
		sprintf(vxlan_if+strlen(vxlan_if), "%d",  vxlan_entry->vlan);
	}

	sprintf(vxlan_dst_port, "%d", VXLAN_DSTPORT);

	//iptables -D INPUT -i nas0_0 -p UDP --dport 4789 -j ACCEPT
	va_cmd(IPTABLES, 10, 1, "-D", (char *)FW_INPUT,  "-i", (char *)ifname, "-p", "UDP", "--dport", (char*)vxlan_dst_port, "-j", (char *)FW_ACCEPT);
	va_cmd(IP6TABLES, 10, 1, "-D", (char *)FW_INPUT,  "-i", (char *)ifname, "-p", "UDP", "--dport", (char*)vxlan_dst_port, "-j", (char *)FW_ACCEPT);

	switch(vxlan_entry->work_mode)
	{
		case VXLAN_WORKMODE_BRIDGE:
			//ebtables -I disBCMC -d Broadcast -o vxlan0 -j ACCEPT
			va_cmd(EBTABLES, 8, 1, (char *)FW_DEL,"disBCMC", "-d", "Broadcast", "-o", (char*)vxlan_if, "-j", (char *)FW_ACCEPT );
			break;
		case VXLAN_WORKMODE_ROUTING:
			strncpy(vxlan_local_ipaddr, inet_ntoa(*((struct in_addr *)vxlan_entry->ip_addr)), 16);			
			if (vxlan_entry->addressing_type == VXLAN_ADDRESSTYPE_DHCP)
			{
				memset(vxlan_entry->ip_addr, 0, sizeof(vxlan_entry->ip_addr));
				memset(vxlan_entry->subnet_mask, 0, sizeof(vxlan_entry->subnet_mask));
				memset(vxlan_entry->default_gateway, 0, sizeof(vxlan_entry->default_gateway));
				mib_chain_update(MIB_VXLAN_TBL, (void *)vxlan_entry, vxlan_entry->mib_idx);

				snprintf(dhcpc_pid_file, 32, "%s.%s", (char*)DHCPC_PID, vxlan_if);
				kill_by_pidfile(dhcpc_pid_file, SIGTERM);
			}
			else if (vxlan_entry->addressing_type == VXLAN_ADDRESSTYPE_STATIC)
			{
			}
			else
				printf("[%s:%d] Unsupport addressing_type =%d!", __FUNCTION__, __LINE__, vxlan_entry->addressing_type);


			//iptables -D POSTROUTING -t nat -o vxlan0 -j SNAT --to 10.20.1.2
			if(vxlan_entry->nat_enable == VXLAN_NAPT_ENABLED)
				va_cmd(IPTABLES, 8, 1, "-D", "POSTROUTING",  "-t", "nat", "-o", (char*)vxlan_if,  "-j", "MASQUERADE"); 

			break;
		default:
			printf("[%s:%d] Unsupport mode =%d!", __FUNCTION__, __LINE__, vxlan_entry->work_mode);
			break;
	}

	return 0;
}

int addVxLAN_for_spec_rule(MIB_CE_VXLAN_Tp vxlan_entry)
{
	int ifIndex=0, total=0, i=0, j=0;
	char vxlan_if[32] = {0}, vxlan_id[8]={0}, vxlan_mac[20];
	char vxlan_local_ipaddr[16]={0}, vxlan_remote_ipaddr[16]={0}, vxlan_gw_ipaddr[16]={0}, vxlan_broadcast[16]={0};
	char vxlan_netmask_ipaddr[16]={0}, vxlan_dst_port[8], vxlan_mtu[6]={0}, vxlan_vlan[6]={0}, vxlan_policy_id[10]={0};
	unsigned char value[32], value2[32];
	unsigned long broadcastIpAddr, netmask;
	char ifname[IFNAMSIZ]={0}, tmp[32]={0};
	unsigned char mac[MAC_ADDR_LEN];
	char vxlan_remote_ipaddr6[INET6_ADDRSTRLEN]={0}, vxlan_local_ipaddr6[INET6_ADDRSTRLEN]={0};
	char vxlan_interface_ip6_addr[INET6_ADDRSTRLEN]={0}, vxlan_interface_ip6_gw_addr[INET6_ADDRSTRLEN]={0};
	struct in_addr inAddr;
	struct in6_addr in6Addr = {0};
	struct ipv6_ifaddr ip6_addr_local={0};

	vxlan_entry->ifIndex = TO_IFINDEX(MEDIA_VXLAN, 0xff, VXLAN_INDEX(vxlan_entry->mib_idx));

 	if (!vxlan_entry->enable)
		return 0;

	if(check_vxlan_portmp(vxlan_entry)<0)
	{
		printf("[%s:%d] ERROR! VXLAN lanInetrace(0x%x) conflict with WAN!\n",  __FUNCTION__, __LINE__, vxlan_entry->lan_interface);
		return -1;
	}

	ifGetName(vxlan_entry->wan_interface, ifname, sizeof(ifname));
	//vlxan0
	sprintf(vxlan_if, "%s%d", VXLAN_INTERFACE, vxlan_entry->mib_idx);
	strncpy(vxlan_entry->vxlan_ifname, vxlan_if, sizeof(vxlan_entry->vxlan_ifname));
	mib_chain_update(MIB_VXLAN_TBL, (void *)vxlan_entry, vxlan_entry->mib_idx);
	//id
	sprintf(vxlan_id, "%d", vxlan_entry->tunnel_key);

	//dst port
	sprintf(vxlan_dst_port, "%d", VXLAN_DSTPORT);

	//ip link add vxlan1 type vxlan id 55 dstport 4789 remote 192.168.2.233 local 192.168.2.210 dev nas0_0
	//remote ip
	if (inet_ntop(PF_INET, &(vxlan_entry->tunnel_dst_ip), vxlan_remote_ipaddr,  sizeof(vxlan_remote_ipaddr)) != NULL)
	{
		if(strcmp(vxlan_remote_ipaddr, "0.0.0.0"))
		{
			//local ip
			if( getInAddr(ifname, IP_ADDR, (void *)&inAddr) != 1)
				return 0;
			strncpy(vxlan_local_ipaddr, inet_ntoa(*((struct in_addr *)&inAddr)), 16);
			va_cmd(VXLAN_CMD, 15, 1, "link", (char *)ARG_ADD, (char *)vxlan_if, "type", "vxlan", "id",  (char*)vxlan_id, "dstport", 
				(char*)vxlan_dst_port, "remote", (char*)vxlan_remote_ipaddr, "local", (char*)vxlan_local_ipaddr, "dev", (char*)ifname);
		}
	}
	//remote ip
	if (inet_ntop(PF_INET6, &(vxlan_entry->tunnel_dst_ip6), vxlan_remote_ipaddr6, sizeof(vxlan_remote_ipaddr6)) != NULL)
	{
		if(strcmp(vxlan_remote_ipaddr6, "::"))
		{
			//local ip
			getifip6((char *)ifname, IPV6_ADDR_UNICAST, &ip6_addr_local, 1);
			if (inet_ntop(PF_INET6, &ip6_addr_local.addr, vxlan_local_ipaddr6, sizeof(vxlan_local_ipaddr6)) != NULL)
			{
				va_cmd(VXLAN_CMD, 15, 1, "link", (char *)ARG_ADD, (char *)vxlan_if, "type", "vxlan", "id",  (char*)vxlan_id, "dstport", 
					(char*)vxlan_dst_port, "remote", (char*)vxlan_remote_ipaddr6, "local", (char*)vxlan_local_ipaddr6, "dev", (char*)ifname);
			}
		}
	}

	//ip link set vxlan0 address 00:01:02:03:04:05 , generate static mac
	mib_get(MIB_ELAN_MAC_ADDR, mac);
	snprintf(vxlan_mac, sizeof(vxlan_mac), "%x:%x:%x:%x:%x:%x",
		mac[0], mac[1], mac[2], (mac[3] + vxlan_entry->inst_num)&0xff,
		(mac[4] + vxlan_entry->inst_num)&0xff, (mac[5] + vxlan_entry->inst_num)&0xff);

	va_cmd(VXLAN_CMD, 5, 1, "link",  "set", (char *)vxlan_if,  "address", (char*)vxlan_mac);

	//ifconfig vxlan0 up
	sprintf(vxlan_mtu, "%d",vxlan_entry->max_mtu_size);
	va_cmd(IFCONFIG, 3, 1, (char *)vxlan_if, "mtu",  (char*)vxlan_mtu);
	va_cmd(IFCONFIG, 2, 1, (char *)vxlan_if, "up");

	//vlan enable
	if (vxlan_entry->vlan_enable) {
		sprintf(vxlan_vlan, "%d",  vxlan_entry->vlan);
		va_cmd(VCONFIG, 3, 1, "add", (char *)vxlan_if, (char *)vxlan_vlan);
		sprintf(vxlan_if+strlen(vxlan_if), ".%d",  vxlan_entry->vlan);
		va_cmd(IFCONFIG, 2, 1, (char *)vxlan_if, "up");
	}

	//iptables -A INPUT -i nas0_0 -p UDP --dport 4789 -j ACCEPT
	va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_INPUT,  "-i", (char *)ifname, "-p", "UDP", "--dport", (char*)vxlan_dst_port, "-j", (char *)FW_ACCEPT);
	va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_INPUT,  "-i", (char *)ifname, "-p", "UDP", "--dport", (char*)vxlan_dst_port, "-j", (char *)FW_ACCEPT);

	switch(vxlan_entry->work_mode)
	{
		case VXLAN_WORKMODE_BRIDGE:
			//brctl addif br0 vxlan0
			va_cmd(BRCTL, 3, 1,  "addif", (char*)BRIF, (char*)vxlan_if);

			//ebtables -I disBCMC -d Broadcast -o vxlan0 -j ACCEPT
			va_cmd(EBTABLES, 8, 1,	(char*)FW_INSERT, "disBCMC", "-d", "Broadcast", "-o", (char*)vxlan_if, "-j", (char *)FW_ACCEPT );
			break;

		case VXLAN_WORKMODE_ROUTING:
			if(vxlan_entry->IpProtocol == IPVER_IPV6)
			{
				setup_disable_ipv6(vxlan_if, 0);
				if((inet_ntop(PF_INET6, &(vxlan_entry->Ipv6Addr), vxlan_interface_ip6_addr, sizeof(vxlan_interface_ip6_addr))!=NULL) &&
					(inet_ntop(PF_INET6, &(vxlan_entry->RemoteIpv6Addr), vxlan_interface_ip6_gw_addr, sizeof(vxlan_interface_ip6_gw_addr))!=NULL))
				{
					sprintf(tmp, "%s/%d", vxlan_interface_ip6_addr, vxlan_entry->Ipv6AddrPrefixLen);
					va_cmd(VXLAN_CMD, 6, 1, "-6", "addr", "add", (char*)tmp, "dev", (char*)vxlan_if);
					//add policy route
					addVxLAN_policyroute_rule(vxlan_entry);

					//setup DNSmasq
					restart_dnsrelay();
				}
			}
			else
			{
				//ip rule del from 10.20.1.9 lookup vxlan0
				strncpy(vxlan_local_ipaddr, inet_ntoa(*((struct in_addr *)vxlan_entry->ip_addr)), 16);

				if (vxlan_entry->addressing_type == VXLAN_ADDRESSTYPE_DHCP)
				{
					//clean old ip address info
					memset(vxlan_entry->ip_addr,0, sizeof(vxlan_entry->ip_addr));
					memset(vxlan_entry->subnet_mask,0, sizeof(vxlan_entry->subnet_mask));
					memset(vxlan_entry->default_gateway,0, sizeof(vxlan_entry->default_gateway));
					mib_chain_update(MIB_VXLAN_TBL, (void *)vxlan_entry, vxlan_entry->mib_idx);

					snprintf(value, 32, "%s.%s", (char *)DHCPC_SCRIPT_NAME, vxlan_if);
					writeDhcpClientScript_vxlan(value);
					snprintf(value2, 32, "%s.%s", (char*)DHCPC_PID, vxlan_if);
					va_cmd(DHCPC, 4, 0, "-i", (char*)vxlan_if, "-p", (char*) value2);
				}
				else if (vxlan_entry->addressing_type == VXLAN_ADDRESSTYPE_STATIC)
				{
					//ipaddr
					strncpy(vxlan_local_ipaddr, inet_ntoa(*((struct in_addr *)vxlan_entry->ip_addr)), 16);

					//netmask
					strncpy(vxlan_netmask_ipaddr, inet_ntoa(*((struct in_addr *)vxlan_entry->subnet_mask)), 16);
					netmask=netmask_to_cidr(vxlan_netmask_ipaddr);
					sprintf(tmp, "%s/%lu", vxlan_local_ipaddr, netmask);

					//gateway 
					strncpy(vxlan_gw_ipaddr, inet_ntoa(*((struct in_addr *)vxlan_entry->default_gateway)), 16);

					//broadcast ip
					broadcastIpAddr = ((struct in_addr *)vxlan_entry->ip_addr)->s_addr | ~(((struct in_addr *)vxlan_entry->subnet_mask)->s_addr);
					strncpy(vxlan_broadcast, inet_ntoa(*((struct in_addr *)&broadcastIpAddr)), 16);

					//ip addr add 10.20.1.2/24 broadcast 10.20.1.255 dev vxlan0
					va_cmd(VXLAN_CMD, 7, 1, "addr", "add", (char*)tmp, "broadcast", (char*)vxlan_broadcast, "dev", (char*)vxlan_if);

					//add policy route
					addVxLAN_policyroute_rule(vxlan_entry);

					//setup DNSmasq
					restart_dnsrelay();
				}
				else
					printf("[%s:%d] Unsupport addressing_type =%d!", __FUNCTION__, __LINE__, vxlan_entry->addressing_type);

				//iptables -A POSTROUTING -t nat -o vxlan0 -j SNAT --to 10.20.1.2
				if(vxlan_entry->nat_enable == VXLAN_NAPT_ENABLED)
					va_cmd(IPTABLES, 8, 1, "-A", "POSTROUTING", "-t", "nat", "-o", (char*)vxlan_if,  "-j", "MASQUERADE");
			}
			break;
		default:
			printf("[%s:%d] Unsupport mode =%d!", __FUNCTION__, __LINE__, vxlan_entry->work_mode);
			break;
	}

	exec_vxlan_portmp();

	return 0;
}

int addVxLAN_for_interface(char* ifname)
{
	int ifIndex=0, total=0, i=0;
	MIB_CE_VXLAN_T Vxlan_entry;

	ifIndex = getIfIndexByName(ifname);
	 total = mib_chain_total(MIB_VXLAN_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_VXLAN_TBL, i, (void *)&Vxlan_entry))
			return -1;
	 	if (!Vxlan_entry.enable) continue;
		if (Vxlan_entry.wan_interface == ifIndex)
			addVxLAN_for_spec_rule(&Vxlan_entry);
	}

	return 0;
}

int delVxLAN_for_interface(char* ifname)
{
	int ifIndex=0, total=0, i=0;
	MIB_CE_VXLAN_T Vxlan_entry;

	ifIndex = getIfIndexByName(ifname);
	total = mib_chain_total(MIB_VXLAN_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_VXLAN_TBL, i, (void *)&Vxlan_entry))
			return -1;
	 	//if (!Vxlan_entry.enable) continue;
		if (Vxlan_entry.wan_interface == ifIndex)
			delVxLAN_for_spec_rule(&Vxlan_entry);
	}
	return 0;
}

int addVxLAN_for_dhcp(char* ifname)
{
	int ifIndex=0, total=0, i=0, j=0;
	char vxlan_if[32] = {0}, vxlan_policy_id[10]={0};
	MIB_CE_VXLAN_T vxlan_entry;
	char vxlan_local_ipaddr[16]={0}, vxlan_gw_ipaddr[16]={0};

	 total = mib_chain_total(MIB_VXLAN_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_VXLAN_TBL, i, (void *)&vxlan_entry))
			return -1;
	 	if (!vxlan_entry.enable) continue;
		if (vxlan_entry.vlan_enable)
			sprintf(vxlan_if, "%s%d.%d", VXLAN_INTERFACE, vxlan_entry.mib_idx, vxlan_entry.vlan);
		else
			sprintf(vxlan_if, "%s%d", VXLAN_INTERFACE, vxlan_entry.mib_idx);
		if (strstr(ifname, vxlan_if))
		{
			//ipaddr
			strncpy(vxlan_local_ipaddr, inet_ntoa(*((struct in_addr *)&vxlan_entry.ip_addr)), 16);

			//gateway
			strncpy(vxlan_gw_ipaddr, inet_ntoa(*((struct in_addr *)&vxlan_entry.default_gateway)), 16);

			//add policy route
			addVxLAN_policyroute_rule(&vxlan_entry);

 			//restart DNSmasq
			restart_dnsrelay();
			return 1;
		}
	}
	return 1;
}

/*-- System inlcude files --*/
#include <string.h>
#include <signal.h>
/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#ifdef CONFIG_USER_MINIUPNP_V2_1
/* Due to libiptc.h include <net/if.h>, it will redefinition with <linux/if.h>
 * So, We should include net/if.h first. To avoid compile error. */
#include "libiptc/libiptc.h"
#endif
#include "utility.h"

#ifdef CONFIG_USER_MINIUPNPD
#ifdef CONFIG_USER_MINIUPNP_V2_1
#include <iptables.h>
#include <linux/netfilter/nf_nat.h>

#define MINIUPNPD_CHAIN		"MINIUPNPD"

/* Dest NAT data consists of a multi-range, indicating where to map
   to. */
struct ipt_natinfo
{
	struct xt_entry_target t;
	struct nf_nat_ipv4_multi_range_compat mr;
};
#endif

void formUpnp(request * wp, char *path, char *query)
{
	char *str_enb, *str_extif, *submitUrl;
#ifdef CONFIG_TR_064
	char *tr064_en;
	unsigned char is_tr064_en, is_tr064_changed = 0;
#endif
	char tmpBuf[100]= {0};
	char ifname[IFNAMSIZ];
#ifndef NO_ACTION
	int pid;
#endif
	unsigned char is_enabled, pre_enabled;
	unsigned int ext_if, pre_ext_if;
#ifdef EMBED
	unsigned char if_num;
	int igmp_pid;
#endif

#ifdef CONFIG_TR_064
	tr064_en = boaGetVar(wp, "tr_064_sw", "");

	if(tr064_en[0])
	{
		is_tr064_en = tr064_en[0] - '0';

		if(is_tr064_en != TR064_STATUS)
		{
			if(!mib_set(MIB_TR064_ENABLED, (void *)&is_tr064_en))
			{
				sprintf(tmpBuf, "%s:%s", Tset_mib_error, MIB_TR064_ENABLED);
				goto setErr_igmp;
			}

			is_tr064_changed = 1;
		}
	}
#endif

	str_enb = boaGetVar(wp, "daemon", "");
	str_extif = boaGetVar(wp, "ext_if", "");

	if(str_enb[0])
	{
		is_enabled = str_enb[0] - '0';

		if(str_extif[0])
			ext_if = (unsigned int)atoi(str_extif);
		else
			ext_if = DUMMY_IFINDEX;  // No interface selected.

		mib_get_s(MIB_UPNP_DAEMON, (void *)&pre_enabled, sizeof(pre_enabled));
		mib_get_s(MIB_UPNP_EXT_ITF, (void *)&pre_ext_if, sizeof(pre_ext_if));

		if (is_enabled != pre_enabled || (ext_if != pre_ext_if)
#ifdef CONFIG_TR_064
			|| is_tr064_changed
#endif
				) {
			if (is_enabled != pre_enabled)
				mib_set(MIB_UPNP_DAEMON, (void *)&is_enabled);
			if (ext_if != pre_ext_if)
				mib_set(MIB_UPNP_EXT_ITF, (void *)&ext_if);
			restart_upnp(CONFIGALL, 0, 0, 0);
		}
// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif
	}

#ifndef NO_ACTION
	pid = fork();
	if (pid)
		waitpid(pid, NULL, 0);
	else if (pid == 0)
	{
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _CONFIG_SCRIPT_PROG);
#ifdef HOME_GATEWAY
		execl( tmpBuf, _CONFIG_SCRIPT_PROG, "gw", "bridge", NULL);
#else
		execl( tmpBuf, _CONFIG_SCRIPT_PROG, "ap", "bridge", NULL);
#endif
		exit(1);
	}
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");
	str_enb = boaGetVar(wp, "refresh", "");
	if (str_enb[0]) /* refresh */
		boaRedirect(wp, submitUrl);
	else
		OK_MSG(submitUrl);
	return;

#ifdef CONFIG_TR_064
setErr_igmp:
	ERR_MSG(tmpBuf);
#endif
}

int upnpPortFwList(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
#ifdef CONFIG_USER_MINIUPNP_V2_1
	struct xtc_handle *handle=NULL;
	const char *chain=NULL;
	const struct ipt_entry *entry=NULL;
	const struct xt_entry_target *t=NULL;
#else
	unsigned int entryNum, i;
	MIB_CE_PORT_FW_T Entry;
	entryNum = mib_chain_total(MIB_PORT_FW_TBL);
#endif
	char	*type, portRange[20], *ip, localIP[20];
	char	remotePort[20];
	
	nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
  	"<td align=center width=\"25%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
  	"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
  	"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
	"<td align=center bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td></tr>\n",
#else
	"<th align=center width=\"25%%\">%s</th>\n"
	"<th align=center width=\"20%%\">%s</th>\n"
	"<th align=center width=\"20%%\">%s</th>\n"
	"<th align=center width=\"20%%\">%s</th>\n"
	"<th align=center>%s</th></tr>\n",
#endif
	multilang(LANG_COMMENT), multilang(LANG_LOCAL_IP), multilang(LANG_PROTOCOL),
	multilang(LANG_LOCAL_PORT), multilang(LANG_REMOTE_PORT));
#ifdef CONFIG_USER_MINIUPNP_V2_1
	/* default miniupnpd will apply rule to specific chain, (please refer user/upnpctrl)
	 * So, we go through MINIUPNPD chain of nat table, */

	// get iptables snapshot of nat table
	handle = iptc_init ("nat");
	if (!handle){
		fprintf (stderr, "Could not init IPTC library: %s\n", iptc_strerror (errno));
		goto out;
	}

	// check MINIUPNP chain is exist or not
	if(!iptc_is_chain(MINIUPNPD_CHAIN, handle)){
		fprintf (stderr, "%s chain is not exist in nat table\n", MINIUPNPD_CHAIN);
		goto out;
	}

	// go throught each chain of nat table
	for (chain = iptc_first_chain(handle); chain; chain = iptc_next_chain(handle)) {
		if (strcmp(MINIUPNPD_CHAIN, chain) != 0)
			continue;

		/* go throught each rule of MINIUPNPD chain
		 * Each firewall rule has three part:
		 *		 <- Part 1 -> <---------- Part 2 ----------> <--------- Part 3 --------->
		 *		+------------+---------------------+--------+----------------------------+
		 *		| ipt_entry  | ipt_entry_match (1) | (2)... |  ipt_entry_target (target) |
		 *		+------------+---------------------+--------+----------------------------+
		 */
		for (entry = iptc_first_rule(chain, handle); entry; entry = iptc_next_rule(entry, handle))	{
			unsigned int __i;
			struct xt_entry_match *__m=NULL;
			type=NULL;

			/* parser match feild, we just support UDP/TCP match now. */
			for (__i = sizeof(struct ipt_entry); __i < (entry)->target_offset; __i += __m->u.match_size) {
				__m = (void *)entry + __i;

				switch(entry->ip.proto){
				case IPPROTO_TCP:
				{
					const struct xt_tcp *tcpinfo = (struct xt_tcp *)__m->data;
					type = strdup(__m->u.user.name);
					if (tcpinfo->dpts[0] != 0 || tcpinfo->dpts[1] != 0xFFFF) {
						if (tcpinfo->dpts[0] != tcpinfo->dpts[1]){
							snprintf(remotePort, sizeof(remotePort), "%s%u:%u", (tcpinfo->invflags & XT_TCP_INV_DSTPT)?"! ":"",
															tcpinfo->dpts[0], tcpinfo->dpts[1]);
						}else{
							snprintf(remotePort, sizeof(remotePort),"%s%u", (tcpinfo->invflags & XT_TCP_INV_DSTPT)?"! ":"",
														tcpinfo->dpts[0]);
						}
					}
					break;
				}
				case IPPROTO_UDP:
				{
					const struct xt_udp *udpinfo = (struct xt_udp *)__m->data;
					type = strdup(__m->u.user.name);
					if (udpinfo->dpts[0] != 0 || udpinfo->dpts[1] != 0xFFFF) {
						if (udpinfo->dpts[0] != udpinfo->dpts[1]){
							snprintf(remotePort, sizeof(remotePort), "%s%u:%u", (udpinfo->invflags & XT_TCP_INV_DSTPT)?"! ":"",
															udpinfo->dpts[0], udpinfo->dpts[1]);
						}else{
							snprintf(remotePort, sizeof(remotePort),"%s%u", (udpinfo->invflags & XT_TCP_INV_DSTPT)?"! ":"",
														udpinfo->dpts[0]);
						}
					}
					break;
				}
				default:
					fprintf(stderr, "unsupport proto(%d) now\n", entry->ip.proto);
					break;
				}
			}

			/* parser target feild, we just support DNAT match now. */
			strcpy(localIP, "----" );
			strcpy(portRange, "----");

			t = ipt_get_target((struct ipt_entry *)entry);
			if(!strncmp(t->u.user.name, "DNAT", strlen(t->u.user.name))){
				const struct ipt_natinfo *info = (const void *)t;

				for (__i = 0; __i < info->mr.rangesize; __i++) {
					const struct nf_nat_ipv4_range *r = &info->mr.range[__i];
					if (r->flags & NF_NAT_RANGE_MAP_IPS) {
						struct in_addr a, b;

						a.s_addr = r->min_ip;
						if (r->max_ip != r->min_ip) {
							b.s_addr = r->max_ip;
							snprintf(localIP, sizeof(localIP), "%s-%s", xtables_ipaddr_to_numeric(&a),
																		xtables_ipaddr_to_numeric(&b));
						}else
							snprintf(localIP, sizeof(localIP), "%s", xtables_ipaddr_to_numeric(&a));
					}
					if (r->flags & NF_NAT_RANGE_PROTO_SPECIFIED) {
						if (r->max.tcp.port != r->min.tcp.port){
							snprintf(portRange, sizeof(portRange), "%hu-%hu", ntohs(r->min.tcp.port), ntohs(r->max.tcp.port));
						}else{
							snprintf(portRange, sizeof(portRange), "%hu", ntohs(r->min.tcp.port));
						}
					}
				}
			}

			nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
				"<td align=center width=\"25%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
				"<td align=center bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td></tr>\n",
#else
				"<td align=center width=\"25%%\">%s</td>\n"
				"<td align=center width=\"20%%\">%s</td>\n"
				"<td align=center width=\"20%%\">%s</td>\n"
				"<td align=center width=\"20%%\">%s</td>\n"
				"<td align=center>%s</td></tr>\n",
#endif
					MINIUPNPD_CHAIN, localIP, type, portRange,
					remotePort);

			if(type) free(type);
		}
	}


out:
	if (handle) iptc_free (handle);
#else /* CONFIG_USER_MINIUPNP_V2_1 */
	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_PORT_FW_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, (char *)errGetEntry);
			return -1;
		}
		
		if (Entry.dynamic==0)
			continue;
		
		ip = inet_ntoa(*((struct in_addr *)Entry.ipAddr));
		strcpy( localIP, ip );
		if ( !strcmp(localIP, "0.0.0.0"))
			strcpy( localIP, "----" );

		if ( Entry.protoType == PROTO_UDPTCP )
			type = "TCP+UDP";
		else if ( Entry.protoType == PROTO_TCP )
			type = "TCP";
		else
			type = "UDP";

		if ( Entry.fromPort == 0)
			strcpy(portRange, "----");
		else if ( Entry.fromPort == Entry.toPort )
			snprintf(portRange, 20, "%d", Entry.fromPort);
		else
			snprintf(portRange, 20, "%d-%d", Entry.fromPort, Entry.toPort);

		if ( Entry.externalfromport == 0)
			strcpy(remotePort, "----");
		else if ( Entry.externalfromport == Entry.externaltoport )
			snprintf(remotePort, 20, "%d", Entry.externalfromport);
		else
			snprintf(remotePort, 20, "%d-%d", Entry.externalfromport, Entry.externaltoport);
		
		nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
			"<td align=center width=\"25%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
     		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
			"<td align=center bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td></tr>\n",
#else
			"<td align=center width=\"25%%\">%s</td>\n"
			"<td align=center width=\"20%%\">%s</td>\n"
			"<td align=center width=\"20%%\">%s</td>\n"
			"<td align=center width=\"20%%\">%s</td>\n"
			"<td align=center>%s</td></tr>\n",
#endif
				strValToASP(Entry.comment), localIP, type, portRange,
				remotePort);
	}
#endif /* CONFIG_USER_MINIUPNP_V2_1 */
	
	return nBytesSent;
}

#endif

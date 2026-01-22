#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../webs.h"
#include "mib.h"
#include "webform.h"
#include "utility.h"
#include "../subr_multiap.h"

void formMultiAP(request *wp, char *path, char *query)
{
	char *submitUrl, *strVal;
	MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_DUALBAND_CONCURRENT
	int orig_wlan_idx = wlan_idx;
#endif
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	unsigned char value[64] = {0};
	int dhcpc_pid = -1;
	unsigned char orig_dhcp_mode = 0;
#endif
	unsigned char dhcp_mode = 0;
#ifdef CONFIG_AUTO_DHCP_CHECK
	const char AUTO_DHCPPID[] = "/var/run/auto_dhcp.pid";
#endif
	char opmode=0;

	//Check if it is push button press, trigger push button then return.
	strVal    = boaGetVar(wp, "start_wsc", "");
	if (strVal[0]) {
#ifdef WLAN_WPS_HAPD
		system("echo 2 > /tmp/virtual_push_button");
#else
		system("echo 1 > /tmp/virtual_push_button");
#endif
		submitUrl = boaGetVar(wp, "submit-url", "");
		boaRedirect(wp, submitUrl);
		return;
	}
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	mib_get_s( MIB_DHCP_MODE, (void *)&orig_dhcp_mode, sizeof(orig_dhcp_mode));
	dhcp_mode = orig_dhcp_mode;
#endif

	mib_get_s(MIB_OP_MODE, (void *)&opmode, sizeof(opmode));

	int i, j;
	// Enable dot11kv if not already enabled
	unsigned char mibVal = 1;
	strVal     = boaGetVar(wp, "needEnable11kv", "");
	if (!strcmp(strVal, "1")) {
		for (i = 0; i < NUM_WLAN_INTERFACE; i++) {
			wlan_idx = i;
			for (j = 0; j < NUM_VWLAN_INTERFACE; j++) {
				wlan_getEntry(&Entry, j);
#if defined(WLAN_11K) && defined(WLAN_11V)
				Entry.rm_activated = mibVal;
				Entry.BssTransEnable = mibVal;
#endif
				wlan_setEntry(&Entry, j);
			}
		}
	}

	char *device_name = boaGetVar(wp, "device_name_text", "");
	mib_set(MIB_MAP_DEVICE_NAME, (void *)device_name);

	// mibVal = 1;
	// apmib_set(MIB_STP_ENABLED, (void *)&mibVal);

	char *role_prev = boaGetVar(wp, "role_prev", "");

	char *controller_backhaul_link = boaGetVar(wp, "controller_backhaul_radio", "");
	char *agent_backhaul_link = boaGetVar(wp, "agent_backhaul_radio", "");

	// Read role info from form and set to mib accordingly
	strVal = boaGetVar(wp, "role", "");
	mibVal = 0;
	if (!strcmp(strVal, "controller")) {
		setupMultiAPController(strVal, role_prev, controller_backhaul_link, &dhcp_mode);
	} else if (!strcmp(strVal, "agent")) {
		setupMultiAPAgent(strVal, role_prev, agent_backhaul_link, &dhcp_mode);
			}
#if defined(WLAN_MULTI_AP_ROLE_AUTO_SELECT)
	else if(!strcmp(strVal, "auto")){
		setupMultiAPAuto(strVal, role_prev, agent_backhaul_link, &dhcp_mode);
		}
#endif
	else if (!strcmp(strVal, "disabled")) {
		setupMultiAPDisabled();
		}

#ifdef CONFIG_AUTO_DHCP_CHECK
	kill_by_pidfile_new((const char*)AUTO_DHCPPID, SIGTERM);
#endif

#ifdef CONFIG_USER_DHCPCLIENT_MODE
	if(orig_dhcp_mode != dhcp_mode){
		if(orig_dhcp_mode == DHCPV4_LAN_CLIENT){
			snprintf(value, 64, "%s.%s", (char*)DHCPC_PID, ALIASNAME_BR0);
			dhcpc_pid = read_pid((char*)value);
			if (dhcpc_pid > 0)
				kill(dhcpc_pid, SIGTERM);
		}

		if( dhcp_mode == DHCPV4_LAN_CLIENT ){
			setupDHCPClient();
		}else{
			/* recover LAN IP address */
			restart_lanip();
		}
		restart_dhcp();
	}
#endif

#ifdef CONFIG_AUTO_DHCP_CHECK
	if(opmode == BRIDGE_MODE && dhcp_mode == AUTO_DHCP_BRIDGE)
	{	
		va_cmd("/bin/Auto_DHCP_Check", 8, 0, "-a", "15", "-n", "5", "-d", "30", "-m", "10");
		start_process_check_pidfile((const char*)AUTO_DHCPPID);
	}
#endif

	config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
	// update flash
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif // of #if COMMIT_IMMEDIATELY

	strVal    = boaGetVar(wp, "save_apply", "");
	submitUrl = boaGetVar(wp, "submit-url", "");
	// sysconf init   if save_apply
	if (strVal[0]) {
		OK_MSG(submitUrl);
	} else {
		boaRedirect(wp, submitUrl);
	}
#ifdef WLAN_DUALBAND_CONCURRENT
	wlan_idx = orig_wlan_idx;
#endif

	return;
}

int showBackhaulSelection(int eid, request * wp, int argc, char **argv)
{
#if defined(TRIBAND_SUPPORT)
	boaWrite(wp, "<tr id=\"controller_backhaul\"> <th width=\"30%%\">%s:</th> <td width=\"70%%\"> \
	<input type=\"radio\" id=\"controller_backhaul_wlan0\" name=\"controller_backhaul_radio\" value=\"0\" onclick=\"isBackhaulOnChange()\">wlan0&nbsp;&nbsp; \
	<input type=\"radio\" id=\"controller_backhaul_wlan1\" name=\"controller_backhaul_radio\" value=\"1\" onclick=\"isBackhaulOnChange()\">wlan1&nbsp;&nbsp; \
	<input type=\"radio\" id=\"controller_backhaul_wlan2\" name=\"controller_backhaul_radio\" value=\"2\" onclick=\"isBackhaulOnChange()\">wlan2&nbsp;&nbsp;</tr>", multilang(LANG_WLAN_EASYMESH_BACKHAUL_BSS));
	boaWrite(wp, "<tr id=\"agent_backhaul\"> <th width=\"30%%\">%s:</th> <td width=\"70%%\"> \
	<input type=\"radio\" id=\"agent_backhaul_wlan0\" name=\"agent_backhaul_radio\" value=\"0\" onclick=\"isBackhaulOnChange()\">wlan0&nbsp;&nbsp; \
	<input type=\"radio\" id=\"agent_backhaul_wlan1\" name=\"agent_backhaul_radio\" value=\"1\" onclick=\"isBackhaulOnChange()\">wlan1&nbsp;&nbsp; \
	<input type=\"radio\" id=\"agent_backhaul_wlan2\" name=\"agent_backhaul_radio\" value=\"2\" onclick=\"isBackhaulOnChange()\">wlan2&nbsp;&nbsp;</tr>", multilang(LANG_WLAN_EASYMESH_BACKHAUL_STA));
#else
	boaWrite(wp, "<tr id=\"controller_backhaul\"> <th width=\"30%%\">%s:</th> <td width=\"70%%\"> \
	<input type=\"radio\" id=\"controller_backhaul_wlan0\" name=\"controller_backhaul_radio\" value=\"0\" onclick=\"isBackhaulOnChange()\">wlan0&nbsp;&nbsp; \
	<input type=\"radio\" id=\"controller_backhaul_wlan1\" name=\"controller_backhaul_radio\" value=\"1\" onclick=\"isBackhaulOnChange()\">wlan1&nbsp;&nbsp;</tr>", multilang(LANG_WLAN_EASYMESH_BACKHAUL_BSS));
	boaWrite(wp, "<tr id=\"agent_backhaul\"> <th width=\"30%%\">%s:</th> <td width=\"70%%\"> \
	<input type=\"radio\" id=\"agent_backhaul_wlan0\" name=\"agent_backhaul_radio\" value=\"0\" onclick=\"isBackhaulOnChange()\">wlan0&nbsp;&nbsp; \
	<input type=\"radio\" id=\"agent_backhaul_wlan1\" name=\"agent_backhaul_radio\" value=\"1\" onclick=\"isBackhaulOnChange()\">wlan1&nbsp;&nbsp;</tr>", multilang(LANG_WLAN_EASYMESH_BACKHAUL_STA));
#endif
	return 0;
}

#ifdef RTK_MULTI_AP_R2
void formMultiAPVLAN(request *wp, char *path, char *query)
{
	char *submitUrl, *strVal;
	int i, j;
	int status = 0;
	MIB_CE_MBSSIB_T Entry;
#ifdef WLAN_DUALBAND_CONCURRENT
	int orig_wlan_idx = wlan_idx;
#endif
	// Enable VLAN
	strVal = boaGetVar(wp, "enable_vlan", "");
	unsigned char mibVal = 0;
	uint8_t value;
	if (!strcmp(strVal, "ON")) {
		mibVal = 1;
		status = mib_set(MIB_MAP_ENABLE_VLAN, (void *)&mibVal);
	} else if (!strcmp(strVal, "OFF")) {
		status = mib_set(MIB_MAP_ENABLE_VLAN, (void *)&mibVal);

	}

	//Set Primary VID
	char *primary_vid = boaGetVar(wp, "primary_vid_text", "");
	if(primary_vid[0])
	{
		value =atoi(primary_vid);
		status = mib_set(MIB_MAP_PRIMARY_VID, (void *)&value);
	}

	//Set Default Secondary VID
	char *default_secondary_vid = boaGetVar(wp, "default_secondary_vid_text", "");
	if(default_secondary_vid[0])
	{
		value =atoi(default_secondary_vid);
		status = mib_set(MIB_MAP_DEFAULT_SECONDARY_VID, (void *)&value);
	}

	//Set WLAN_MAP_VID
	strVal = boaGetVar(wp, "vid_info", "");
	char* token;
	for (i = 0; i < 2; i++) {
		wlan_idx = i;
		for (j = 0; j < 5; j++) {
			if (0 == i && 0 == j) {
				token = strtok(strVal, "_");
			} else {
				token = strtok(NULL, "_");
			}
			mibVal = atoi(token);
			wlan_getEntry(&Entry, j);
			Entry.multiap_vid = mibVal;
			wlan_setEntry(&Entry, j);
		}
	}

	config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
	// update flash
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif // of #if COMMIT_IMMEDIATELY

	strVal    = boaGetVar(wp, "save_apply", "");
	submitUrl = boaGetVar(wp, "submit-url", "");
	// sysconf init   if save_apply
	if (strVal[0]) {
		OK_MSG(submitUrl);
	} else {
		boaRedirect(wp, submitUrl);
	}
#ifdef WLAN_DUALBAND_CONCURRENT
	wlan_idx = orig_wlan_idx;
#endif

	return;
}

enum {
	CHANNEL_SCAN_BAND_2G = 1,
	CHANNEL_SCAN_BAND_5G = 2
};


static int _rtk_get_multi_ap_index_by_chan_scan_band(int band)
{
	int i = 0;
	unsigned char phyband=0, vChar=0;
	int orig_wlan_idx=0;

	if((band < CHANNEL_SCAN_BAND_2G) || (band > CHANNEL_SCAN_BAND_5G)){
		printf("Can find corresponding interface\n");
		return -1;
	}

#ifdef WLAN_DUALBAND_CONCURRENT
	orig_wlan_idx = wlan_idx;
#endif

	if(band == CHANNEL_SCAN_BAND_2G)
		phyband = PHYBAND_2G;
	else
		phyband = PHYBAND_5G;

	for(i=0; i<NUM_WLAN_INTERFACE; i++){
		wlan_idx = i;
		mib_get(MIB_WLAN_PHY_BAND_SELECT, &vChar);
		if(phyband == vChar)
			break;
	}
#ifdef WLAN_DUALBAND_CONCURRENT
	wlan_idx = orig_wlan_idx;
#endif
	return i;
}

static int _channel_scan_band = 0;

//return value: -1: channel is not available or other errors, 0: sideband is not acceptable, 1: nothing needs to be changed
static int _check_control_side_band(int wlan_index, unsigned char chan)
{
#if defined(WLAN_HAPD) || defined(WLAN_WPAS)
	unsigned char channelwidth = 0, sideband = 0;
	FILE *fp = NULL;
	char buf[1024] = {0};
	int phy_index = 0, channel = 0;
	int ret = 1;

	if(chan <= 14){
		mib_get_s( MIB_WLAN_CHANNEL_WIDTH, (void *)&channelwidth, sizeof(channelwidth));
		if(channelwidth == 1){
			mib_get_s( MIB_WLAN_CONTROL_BAND, (void *)&sideband, sizeof(sideband));
			snprintf(buf, sizeof(buf), "iw dev %s info | grep wiphy", WLANIF[wlan_index]);
			fp = popen(buf, "r");
			if(fp){
				if(fgets(buf, sizeof(buf), fp) != NULL){
					sscanf(buf, "%*[^1-9]%d", &phy_index);
				}
				pclose(fp);
			}
			snprintf(buf, sizeof(buf), "iw phy#%d channels", phy_index);
			fp = popen(buf, "r");
			if(fp){
				while(fgets(buf, sizeof(buf), fp) != NULL){
					if(sscanf(buf, "%*[^[][%d]", &channel) == 1 && (chan == channel)){
						if(strstr(buf, "disabled") != NULL){
							ret = -1;
							break;
						}
						if(fgets(buf, sizeof(buf), fp) == NULL){
							//Maximum TX power
							ret = -1;
							break;
						}
						if(fgets(buf, sizeof(buf), fp) == NULL){
							//Channel widths
							ret = -1;
							break;
						}
						if((sideband == 0) && (strstr(buf, "HT40-") == NULL)){
							ret = 0;
						}
						else if((sideband == 1) && (strstr(buf, "HT40+") == NULL)){
							ret = 0;
						}
						break;
					}
					else
						continue;
				}
				pclose(fp);
			}
		}
	}
	return ret;
#else
	unsigned char channelwidth = 0, sideband = 0;
	if(chan <= 14){
		mib_get_s( MIB_WLAN_CHANNEL_WIDTH, (void *)&channelwidth, sizeof(channelwidth));
		if(channelwidth == 1){
			mib_get_s( MIB_WLAN_CONTROL_BAND, (void *)&sideband, sizeof(sideband));
			if ((chan<=4 && sideband==0) || (chan>=10 && sideband==1)){
				return 0;
			}
		}
	}
	return 1;
#endif
}

void formChannelScan(request *wp, char *path, char *query)
{
	char *strVal, *submitUrl;
	int mibVal, vwlan_idx;
	uint8_t vChar = 0, sideband = 0;
    char cmd[256];
	uint8_t value;

	strVal = boaGetVar(wp, ("scan_band"), "");
	if (strVal[0]) {
		char cmd[64], scan_band;
		if(!strcmp(strVal, "2G")) {
			scan_band = 1;
			_channel_scan_band = CHANNEL_SCAN_BAND_2G;
		} else if(!strcmp(strVal, "5G")) {
			scan_band = 2;
			_channel_scan_band = CHANNEL_SCAN_BAND_5G;
		} else if(!strcmp(strVal, "all")) {
			scan_band = 0;
			_channel_scan_band = CHANNEL_SCAN_BAND_2G | CHANNEL_SCAN_BAND_5G;
		}

		sprintf(cmd, "echo %d > /tmp/channel_scan_band", scan_band);
		printf("%s\n", cmd);
		system(cmd);
		boaRedirect(wp, "/multi_ap_channel_scan_result.asp");
		return;
	}

#ifdef WLAN_DUALBAND_CONCURRENT
	int orig_wlan_idx = wlan_idx;
#endif
	strVal = boaGetVar(wp, ("band_select_1"), "");
	if (strVal[0]) {
		if (!strcmp(strVal, "selected")) {
			strVal = boaGetVar(wp, ("channel_switch_1"), "");
			if (strVal[0]) {
				value = atoi(strVal);
				wlan_idx = _rtk_get_multi_ap_index_by_chan_scan_band(CHANNEL_SCAN_BAND_2G);
				printf("_CHAN_NUM: %d\n", mibVal);
				if ( mib_set( MIB_WLAN_CHAN_NUM , (void *)&value) == 0)
					printf("_CHAN_NUM error!!!: %d\n", mibVal);
				printf("2g_CHAN_NUM: %d MIB_WLAN_CHAN_NUM: %d\n", mibVal,MIB_WLAN_CHAN_NUM);
				mib_set( MIB_WLAN_AUTO_CHAN_ENABLED, (void *)&vChar);
				if (_check_control_side_band(wlan_idx, value) == 0){
					mib_get_s( MIB_WLAN_CONTROL_BAND, (void *)&sideband, sizeof(sideband));
					sideband = ((sideband + 1) & 0x1);
					mib_set( MIB_WLAN_CONTROL_BAND, (void *)&sideband);
				}
			}
		}
	}

	strVal = boaGetVar(wp, ("band_select_2"), "");
	if (strVal[0]) {
		if (!strcmp(strVal, "selected")) {
			strVal = boaGetVar(wp, ("channel_switch_2"), "");
			if (strVal[0]) {
				value = atoi(strVal);
				wlan_idx = _rtk_get_multi_ap_index_by_chan_scan_band(CHANNEL_SCAN_BAND_5G);
				printf("_CHAN_NUM: %d\n", mibVal);
				if ( mib_set( MIB_WLAN_CHAN_NUM , (void *)&value) == 0)
					printf("_CHAN_NUM error!!!: %d\n", mibVal);
				printf("5g_CHAN_NUM: %d MIB_WLAN_CHAN_NUM: %d\n", mibVal,MIB_WLAN_CHAN_NUM);
				mib_set( MIB_WLAN_AUTO_CHAN_ENABLED, (void *)&vChar);
				if (_check_control_side_band(wlan_idx, value) == 0){
					mib_get_s( MIB_WLAN_CONTROL_BAND, (void *)&sideband, sizeof(sideband));
					sideband = ((sideband + 1) & 0x1);
					mib_set( MIB_WLAN_CONTROL_BAND, (void *)&sideband);
				}
			}
		}
	}
	config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
	// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif // of #if COMMIT_IMMEDIATELY

	printf("3_CHAN_NUM: %d\n", mibVal);
	strVal    = boaGetVar(wp, "save_apply", "");
	submitUrl = boaGetVar(wp, "submit-url", "");
	// sysconf init   if save_apply
	if (strVal[0]) {
		OK_MSG(submitUrl);
	} else {
		boaRedirect(wp, submitUrl);
	}
#ifdef WLAN_DUALBAND_CONCURRENT
	wlan_idx = orig_wlan_idx;
#endif
	return;
}

void get_channel_scan_results(struct channel_scan_results *results, int band)
{
	FILE *fp = NULL, *fp_nr = NULL;
	char *  line = NULL;
	size_t  len  = 0;
	ssize_t read;
	char    element[32];
	int 	val = 0, cu_weight = 0, noise_weight = 0, neighbor_weight = 100, best_score = 0, i = 0;

	if (band == CHANNEL_SCAN_BAND_2G) {
		fp_nr = fopen("/tmp/map_channel_scan_nr_2G", "r");
		fp = fopen("/tmp/map_channel_scan_report_2G", "r");
	} else if (band == CHANNEL_SCAN_BAND_5G) {
		fp_nr = fopen("/tmp/map_channel_scan_nr_5G", "r");
		fp = fopen("/tmp/map_channel_scan_report_5G", "r");
	}

	if (fp_nr && fp) {
		if (getline(&line, &len, fp_nr) != -1) {
			sscanf(line, "%s %d", element, &val);
			printf("%s = %d\n", element, val);
			if(0 == strcmp(element, "channel_nr")) {
				results->channel_nr = val;
				results->channels = (struct channel_list*)malloc(sizeof(struct channel_list) * results->channel_nr);
			}
			free(line);
			line = NULL;

			while ((read = getline(&line, &len, fp)) != -1) {
				sscanf(line, "%s %d", element, &val);
				 printf("%s = %d\n", element, val);

				if (0 == strcmp(element, "op_class")) {
					results->channels[i].op_class = val;
				} else if (0 == strcmp(element, "channel")) {
					results->channels[i].channel = val;
				} else if (0 == strcmp(element, "scan_status"))	{
					results->channels[i].scan_status = val;
				} else if (0 == strcmp(element, "utilization")) {
					results->channels[i].utilization = val;
				} else if (0 == strcmp(element, "noise")) {
					results->channels[i].noise = val;
				} else if (0 == strcmp(element, "neighbor_nr")) {
					results->channels[i].neighbor_nr = val;
					results->channels[i].score = ((255 - results->channels[i].utilization) * 100) / 255 * cu_weight + (100 - results->channels[i].noise) * noise_weight + (100 - results->channels[i].neighbor_nr) * neighbor_weight;
					results->channels[i].score /= (cu_weight + noise_weight + neighbor_weight);
					i++;
				}
			}
			free(line);

			results->best_channel = results->channels[0].channel;
			best_score = results->channels[0].score;
			for (i = 0; i < results->channel_nr; i++) {
				if (results->channels[i].scan_status == 0 && results->channels[i].score > best_score) {
					results->best_channel = results->channels[i].channel;
					best_score = results->channels[i].score;
				}
			}
		}
	}
	if(fp_nr)
        fclose(fp_nr);
    if(fp)
		fclose(fp);
}

void show_channel_scan_results(request *wp, struct channel_scan_results *results, int *nBytesSent)
{
	int i;

	nBytesSent += boaWrite(wp, ("<br>""<table border=\"1\" width=\"680\">\n"));

	nBytesSent += boaWrite(wp, ("<tr class=\"tbl_head\">"
	"<td align=center ><font size=\"2\"><b>OP Class</b></font></td>\n"
	"<td align=center ><font size=\"2\"><b>Channel</b></font></td>\n"
	"<td align=center ><font size=\"2\"><b>Score</b></font></td>\n"));
	nBytesSent += boaWrite(wp, ("</tr>\n"));

	for (i = 0; i < results->channel_nr; i++) {
		if (results->channels[i].scan_status == 0) {
			nBytesSent += boaWrite(wp, ("<tr class=\"tbl_body\">"
			"<td align=center ><font size=\"2\">%d</td>\n"
			"<td align=center ><font size=\"2\">%d</td>\n"
			"<td align=center ><font size=\"2\">%d</td>\n"),
			results->channels[i].op_class, results->channels[i].channel, results->channels[i].score);
			nBytesSent += boaWrite(wp, ("</tr>\n"));
		}
	}
	nBytesSent += boaWrite(wp, ("</table>\n"));
}

void show_best_channel(request *wp, struct channel_scan_results *results, int *nBytesSent, int *match, int band)
{
	char channel_num;
	mib_get_s(MIB_WLAN_CHAN_NUM, (void *)&channel_num,sizeof(channel_num));

	if(channel_num == results->best_channel) {
		(*match)++;
		nBytesSent += boaWrite(wp, ("<p>"
		"<font size=\"2\"><b>Current channel %d is already the best channel</b></font>\n""</p>\n"), channel_num);

		printf("already best channel");
	} else {
		nBytesSent += boaWrite(wp, ("<tr >"
		"<td ><input type=\"hidden\" id=\"channel_switch_%d\" value=\"%d\" name=\"channel_switch_%d\"><input type=\"checkbox\" id=\"band_select_%d\" value=\"\" name=\"band_select_%d\"></td>\n"
		"<td ><font size=\"2\"><b>Switch to best channel %d</b></font></td>\n"
		"</tr>\n"), band, results->best_channel, band, band, band, results->best_channel);

	}
}

int showChannelScanResults(int eid, request *wp, int argc, char **argv)
{
    int nBytesSent = 0;
	struct channel_scan_results results_5G = {0}, results_2G = {0};

	if (-1 != access("/tmp/map_channel_scan_report_5G", F_OK) && (_channel_scan_band & CHANNEL_SCAN_BAND_5G)) {
		printf("access map_channel_scan_report_5G!\n");
		get_channel_scan_results(&results_5G, CHANNEL_SCAN_BAND_5G);
		remove("/tmp/map_channel_scan_report_5G");
		remove("/tmp/map_channel_scan_nr_5G");
	}

	if (-1 != access("/tmp/map_channel_scan_report_2G", F_OK) && (_channel_scan_band & CHANNEL_SCAN_BAND_2G)) {
		printf("access map_channel_scan_report_2G!\n");
		get_channel_scan_results(&results_2G, CHANNEL_SCAN_BAND_2G);
		remove("/tmp/map_channel_scan_report_2G");
		remove("/tmp/map_channel_scan_nr_2G");
	}

	if(results_5G.channel_nr == 0 && results_2G.channel_nr == 0) {
		nBytesSent += boaWrite(wp, ("<div id=\"reload\"><font size=2> Please wait for the scan result......</div>\n"));
		return nBytesSent;
	}

	int channel_match = 0;
	//vwlan_idx = 0;
	if(results_5G.channel_nr > 0) {
		wlan_idx = _rtk_get_multi_ap_index_by_chan_scan_band(CHANNEL_SCAN_BAND_5G);
		show_best_channel(wp, &results_5G, &nBytesSent, &channel_match, CHANNEL_SCAN_BAND_5G);
	}

	if(results_2G.channel_nr > 0) {
		wlan_idx = _rtk_get_multi_ap_index_by_chan_scan_band(CHANNEL_SCAN_BAND_2G);
		show_best_channel(wp, &results_2G, &nBytesSent, &channel_match, CHANNEL_SCAN_BAND_2G);
	}

	if((channel_match < 2 && (_channel_scan_band == (CHANNEL_SCAN_BAND_2G | CHANNEL_SCAN_BAND_5G))) ||
	  (channel_match < 1 && (_channel_scan_band == CHANNEL_SCAN_BAND_2G || _channel_scan_band == CHANNEL_SCAN_BAND_5G))) {
		nBytesSent += boaWrite(wp, ("<p>""<input type=\"submit\" value=\"Apply Changes\"class=\"link_bg\" name=\"save_apply\" onClick=\"return saveChanges(this)\">\n""</p>"));
	}

	if(results_5G.channel_nr > 0) {
		//printf("show_channel_scan_results 5G!\n");
		show_channel_scan_results(wp, &results_5G, &nBytesSent);
	}

	if(results_2G.channel_nr > 0) {
	    //printf("show_channel_scan_results 2G!\n");
		show_channel_scan_results(wp, &results_2G, &nBytesSent);
	}

	if(results_5G.channels) {
		free(results_5G.channels);
	}
	if(results_2G.channels) {
		free(results_2G.channels);
	}

    return nBytesSent;
}
#endif
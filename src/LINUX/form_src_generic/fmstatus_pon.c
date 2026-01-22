/*
 *      Web server handler routines for System Pon status
 *
 */

#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "debug.h"
#include "multilang.h"
#include "rtk/ponmac.h"
#include "rtk/gpon.h"
#include "rtk/epon.h"
#include "rtk/stat.h"
#include "rtk/switch.h"
#if defined(CONFIG_COMMON_RT_API)
#include <rtk/rt/rt_stat.h>
#include <rtk/rt/rt_epon.h>
#include <rtk/rt/rt_gpon.h>
#include <rtk/rt/rt_switch.h>
#endif

#include <common/util/rt_util.h>

#ifdef CONFIG_RTK_OMCI_V1
#include <omci_api.h>
#include <gos_type.h>
#endif

unsigned short get_usflow_index(void)
{
	rtk_switch_devInfo_t devInfo;

	if(rtk_switch_deviceInfo_get(&devInfo) != RT_ERR_OK)
		printf("rtk_switch_deviceInfo_get failed: %s %d\n", __func__, __LINE__);

	switch(devInfo.chipId)
	{
		case RTL9601B_CHIP_ID:
		return 32;
		case APOLLOMP_CHIP_ID:
		default:
		return 127;
	}
}

const char *loid_get_authstatus(int index)
{
	switch (index)
	{
		case 0:
		return multilang(LANG_LOID_STATUS_INIT);
		case 1:
		return multilang(LANG_LOID_STATUS_SUCCESS);
		case 2:
		return multilang(LANG_LOID_STATUS_ERROR);
		case 3:
		return multilang(LANG_LOID_STATUS_PWDERR);
		case 4:
		return multilang(LANG_LOID_STATUS_DUPLICATE);
		default:
		return "WRONG";
	}
}

int showgpon_status(int eid, request * wp, int argc, char **argv)
{
	unsigned int pon_mode;
	int nBytesSent=0;
	int loid_st = 0;
	int onu;
#ifndef CONFIG_00R0
#ifdef CONFIG_COMMON_RT_API
	rt_gpon_omcc_t rtOmcc;
#else
	rtk_gpon_usFlow_attr_t aAttr;
	unsigned int usflow = 0;
#endif
#endif
#ifdef CONFIG_RTK_OMCI_V1
	PON_OMCI_CMD_T msg;
#endif
#ifdef CONFIG_00R0
	unsigned char onuid=5;
#endif

	if (mib_get_s(MIB_PON_MODE, (void *)&pon_mode, sizeof(pon_mode)) != 0)
	{
		if (pon_mode != GPON_MODE)
			return 0;
	}
	else
		return 0;

	onu = getGponONUState();

#ifndef CONFIG_00R0
#ifdef CONFIG_COMMON_RT_API
	if(rt_gpon_omcc_get(&rtOmcc) != RT_ERR_OK)
		printf("rt_gpon_omcc_get failed: %s %d\n", __func__, __LINE__);
#else
	usflow = get_usflow_index();
	rtk_gpon_usFlow_get(usflow, &aAttr);
#endif
#endif
#ifdef CONFIG_RTK_OMCI_V1
	memset(&msg, 0, sizeof(msg));
	msg.cmd = PON_OMCI_CMD_LOIDAUTH_GET_RSP;

	if(omci_SendCmdAndGet(&msg) == GOS_OK)
		loid_st = msg.state;
	else
		loid_st = -1; // wrong status
#endif

#ifdef CONFIG_00R0
#if defined(CONFIG_RTK_L34_ENABLE)
	rtk_rg_gpon_parameter_get(RTK_GPON_PARA_TYPE_ONUID, &onuid);
#else
	rtk_gpon_parameter_get(RTK_GPON_PARA_TYPE_ONUID, &onuid);
#endif
#endif
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr> <td width=100%% colspan=\"2\" bgcolor=\"#008000\"><font color=\"#FFFFFF\" size=2><b>%s</b></font></td> </tr>"
        "<tr bgcolor=\"#DDDDDD\">"
		"<td width=\"30%%\"><font size=2><b>%s</b></td>"
        "<td width=\"70%%\"><font size=2>O%d</td> </tr>\n"
		"<tr bgcolor=\"#DDDDDD\">"
		"<td width=\"30%%\"><font size=2><b>%s</b></td>"
		"<td width=\"70%%\"><font size=2>%d</td> </tr>\n"
		"<tr bgcolor=\"#DDDDDD\">"
		"<td width=\"30%%\"><font size=2><b>%s</b></td>"
		"<td width=\"70%%\"><font size=2>%s</td> </tr>\n",
#else
		nBytesSent += boaWrite(wp, "<div class=\"column_title\"><div class=\"column_title_left\"></div>\n"
							   "<p>%s</p><div class=\"column_title_right\"></div></div>\n"
							   "<div class=\"data_common\"><table>\n"
		"<tr><th width=\"40%%\">%s</th>\n"
        "<td width=\"60%%\">O%d</td> </tr>\n"
		"<tr>\n"
		"<th width=\"40%%\">%s</th>\n"
		"<td width=\"60%%\">%d</td> </tr>\n"
		"<tr>\n"
		"<th width=\"40%%\">%s</th>"
		"<td width=60%%>%s</td></tr></table></div>\n",
#endif
		multilang(LANG_GPON_STATUS), multilang(LANG_ONU_STATE), onu,
#ifdef CONFIG_00R0
		multilang(LANG_ONU_ID),onuid,
#else
#if defined(CONFIG_RTK_L34_ENABLE)
		multilang(LANG_ONU_ID),aAttr.gem_port_id,
#elif defined(CONFIG_COMMON_RT_API)
		multilang(LANG_ONU_ID),rtOmcc.allocId,
#else
		multilang(LANG_ONU_ID),aAttr.gem_port_id,
#endif
#endif
		multilang(LANG_LOID_STATUS),loid_get_authstatus(loid_st));	return 0;
}

int showepon_LLID_status(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;
    int entryNum=4;
	char tmpBuf[100];
	int i;
	unsigned int pon_mode;

	if (mib_get_s(MIB_PON_MODE, (void *)&pon_mode, sizeof(pon_mode)) != 0)
	{
		if (pon_mode != EPON_MODE)
			return 0;
	}
	else
			return 0;

	entryNum = rtk_pon_getEponllidEntryNum();
	if(entryNum<=0)
	{
 		strcpy(tmpBuf, multilang(LANG_GET_LLIDENTRYNUM_ERROR));
		ERR_MSG(tmpBuf);
		return -1;
	}

#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<td width=100%% colspan=\"2\" bgcolor=\"#008000\"><font color=\"#FFFFFF\" size=2><b> EPON LLID Status </b></font></td>");

	nBytesSent += boaWrite(wp, "<tr><font size=1>"
	"<td align=center width=\"33%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"33%%\" bgcolor=\"#808080\">%s</td>\n"
	"</font></tr>\n", "index", multilang(LANG_STATUS));
#else
    nBytesSent += boaWrite(wp, "<div class=\"column_title\"><div class=\"column_title_left\"></div>\n"
							   "<p>EPON LLID Status</p><div class=\"column_title_right\"></div></div>\n");

	nBytesSent += boaWrite(wp, "<div class=\"data_common data_vertical\"><table>\n"
	"<tr><th width=\"40%%\">%s</th>\n"
	"<th width=\"60%%\">%s</th>\n"
	"</tr>\n", "index", multilang(LANG_STATUS));
#endif

	for (i=0; i<entryNum; i++) {
		int onu = 0;

		onu = getEponONUState(i);
		nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
				"<td align=center width=\"33%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%d</b></font></td>\n"
				"<td align=center width=\"33%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%d</b></font></td><br>\n",
#else
				"<td align=center>%d</td>\n"
				"<td align=center>%d</td></tr>\n",
#endif
				i, (onu==5? 1:0));
	}
#ifdef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "</table>\n");
#endif
	return 0;

}

void formStatus_pon(request * wp, char *path, char *query)
{
	char *submitUrl;

	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
  	return;
}

void formPonStats(request * wp, char *path, char *query)
{
	char *submitUrl;
	char *str_enb;
#if defined(CONFIG_COMMON_RT_API)
	unsigned int pon_port_idx = 1;
	rt_switch_phyPortId_get(LOG_PORT_PON, &pon_port_idx);
#endif

	str_enb = boaGetVar(wp, "reset", "");
	if(str_enb[0])
	{
		if(str_enb[0] - '0') {	//reset
#if defined(CONFIG_COMMON_RT_API)
			rt_stat_port_reset(pon_port_idx);
#endif
		}
	}
	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
  	return;
}

int ponGetStatus(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
	char *name;
	uint64 value = 0;
	rtk_transceiver_data_t transceiver, readableCfg;
#ifdef CONFIG_COMMON_RT_API
	rt_stat_port_cntr_t counters;
#else
	rtk_stat_port_cntr_t counters;
#endif
	unsigned int pon_mode;
	unsigned int pon_port_idx = 1;

	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}

#ifdef CONFIG_COMMON_RT_API
	if(rt_switch_phyPortId_get(LOG_PORT_PON, &pon_port_idx) != 0)
		printf("rt_switch_phyPortId_get failed: %s %d\n", __func__, __LINE__);
#else
	rtk_switch_phyPortId_get(RTK_PORT_PON, &pon_port_idx);
#endif

	memset(&transceiver, 0, sizeof(transceiver));
	memset(&readableCfg, 0, sizeof(readableCfg));
	memset(&counters, 0, sizeof(counters));

	if (!strcmp(name, "vendor-name")) {

		if(rtk_ponmac_transceiver_get
		    (RTK_TRANSCEIVER_PARA_TYPE_VENDOR_NAME, &transceiver) != RT_ERR_OK)
			printf("rtk_ponmac_transceiver_get failed: %s %d\n", __func__, __LINE__);

		_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_VENDOR_NAME, &transceiver, &readableCfg);
		if ( readableCfg.buf[0] > 0x7F )
			memset(readableCfg.buf, 0x0, TRANSCEIVER_LEN);
		return boaWrite(wp, "%s", readableCfg.buf);
	} else if (!strcmp(name, "part-number")) {
		if(rtk_ponmac_transceiver_get
		    (RTK_TRANSCEIVER_PARA_TYPE_VENDOR_PART_NUM, &transceiver) != RT_ERR_OK)
			printf("rtk_ponmac_transceiver_get failed: %s %d\n", __func__, __LINE__);
		_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_VENDOR_PART_NUM, &transceiver, &readableCfg);
		if ( readableCfg.buf[0] > 0x7F )
			memset(readableCfg.buf, 0x0, TRANSCEIVER_LEN);
		return boaWrite(wp, "%s", readableCfg.buf);
	}
	else if (!strcmp(name, "bytes-sent")) {
#ifdef CONFIG_COMMON_RT_API
		if(rt_stat_port_get(pon_port_idx, RT_IF_OUT_OCTETS_INDEX, &value) != RT_ERR_OK)
			printf("rt_stat_port_get failed: %s %d\n", __func__, __LINE__);
#else
		rtk_stat_port_get(pon_port_idx, IF_OUT_OCTETS_INDEX, &value);
#endif
		return boaWrite(wp, "%llu", value);
	} else if (!strcmp(name, "bytes-received")) {
#ifdef CONFIG_COMMON_RT_API
		if(rt_stat_port_get(pon_port_idx, RT_IF_IN_OCTETS_INDEX, &value) != RT_ERR_OK)
			printf("rt_stat_port_get failed: %s %d\n", __func__, __LINE__);
#else
		rtk_stat_port_get(pon_port_idx, IF_IN_OCTETS_INDEX, &value);
#endif
		return boaWrite(wp, "%llu", value);
	} else if (!strcmp(name, "packets-sent")) {
#ifdef CONFIG_COMMON_RT_API
		if(rt_stat_port_getAll(pon_port_idx, &counters) != RT_ERR_OK)
			printf("rt_stat_port_getAll failed: %s %d\n", __func__, __LINE__);
#else
		rtk_stat_port_getAll(pon_port_idx, &counters);
#endif
		return boaWrite(wp, "%u", counters.ifOutUcastPkts + counters.ifOutMulticastPkts
						+ counters.ifOutBrocastPkts);
	} else if (!strcmp(name, "packets-received")) {
#ifdef CONFIG_COMMON_RT_API
		if(rt_stat_port_getAll(pon_port_idx, &counters) != RT_ERR_OK)
			printf("rt_stat_port_getAll failed: %s %d\n", __func__, __LINE__);
#else
		rtk_stat_port_getAll(pon_port_idx, &counters);
#endif
		return boaWrite(wp, "%u", counters.ifInUcastPkts + counters.ifInMulticastPkts
						+ counters.ifInBroadcastPkts);
	} else if (!strcmp(name, "unicast-packets-sent")) {
#ifdef CONFIG_COMMON_RT_API
		if(rt_stat_port_getAll(pon_port_idx, &counters) != RT_ERR_OK)
			printf("rt_stat_port_getAll failed: %s %d\n", __func__, __LINE__);
#else
		rtk_stat_port_getAll(pon_port_idx, &counters);
#endif
		return boaWrite(wp, "%u", counters.ifOutUcastPkts);
	} else if (!strcmp(name, "unicast-packets-received")) {
#ifdef CONFIG_COMMON_RT_API
		if(rt_stat_port_getAll(pon_port_idx, &counters) != RT_ERR_OK)
			printf("rt_stat_port_getAll failed: %s %d\n", __func__, __LINE__);
#else
		rtk_stat_port_getAll(pon_port_idx, &counters);
#endif
		return boaWrite(wp, "%u", counters.ifInUcastPkts);
	} else if (!strcmp(name, "multicast-packets-sent")) {
#ifdef CONFIG_COMMON_RT_API
		if(rt_stat_port_getAll(pon_port_idx, &counters) != RT_ERR_OK)
			printf("rt_stat_port_getAll failed: %s %d\n", __func__, __LINE__);
#else
		rtk_stat_port_getAll(pon_port_idx, &counters);
#endif
		return boaWrite(wp, "%u", counters.ifOutMulticastPkts);
	} else if (!strcmp(name, "multicast-packets-received")) {
#ifdef CONFIG_COMMON_RT_API
		if(rt_stat_port_getAll(pon_port_idx, &counters) != RT_ERR_OK)
			printf("rt_stat_port_getAll failed: %s %d\n", __func__, __LINE__);
#else
		rtk_stat_port_getAll(pon_port_idx, &counters);
#endif
		return boaWrite(wp, "%u", counters.ifInMulticastPkts);
	} else if (!strcmp(name, "broadcast-packets-sent")) {
#ifdef CONFIG_COMMON_RT_API
		if(rt_stat_port_getAll(pon_port_idx, &counters) != RT_ERR_OK)
			printf("rt_stat_port_getAll failed: %s %d\n", __func__, __LINE__);
#else
		rtk_stat_port_getAll(pon_port_idx, &counters);
#endif
		return boaWrite(wp, "%u", counters.ifOutBrocastPkts);
	} else if (!strcmp(name, "broadcast-packets-received")) {
#ifdef CONFIG_COMMON_RT_API
		if(rt_stat_port_getAll(pon_port_idx, &counters) != RT_ERR_OK)
			printf("rt_stat_port_getAll failed: %s %d\n", __func__, __LINE__);
#else
		rtk_stat_port_getAll(pon_port_idx, &counters);
#endif
		return boaWrite(wp, "%u", counters.ifInBroadcastPkts);
	} else if (!strcmp(name, "fec-errors")) {
		mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));

		if (pon_mode == GPON_MODE) {
			rtk_gpon_global_counter_t counter;

			memset(&counter, 0, sizeof(counter));
#ifdef CONFIG_COMMON_RT_API
			if(rtk_gpon_globalCounter_get(RTK_GPON_PMTYPE_DS_PHY, &counter) != RT_ERR_OK)
				printf("rtk_gpon_globalCounter_get failed: %s %d\n", __func__, __LINE__);
#endif
			nBytesSent += boaWrite(wp, "%u", counter.dsphy.rx_fec_uncor_cw);
		} else {
#ifdef CONFIG_COMMON_RT_API
			rt_epon_counter_t counter;
#else
			rtk_epon_counter_t counter;
#endif

			memset(&counter, 0, sizeof(counter));
#ifdef CONFIG_COMMON_RT_API
			if(rt_epon_mibCounter_get(&counter) != RT_ERR_OK)
				printf("rt_epon_mibCounter_get failed: %s %d\n", __func__, __LINE__);
#else
			rtk_epon_mibCounter_get(&counter);
#endif

			nBytesSent += boaWrite(wp, "%u", counter.fecUncorrectedBlocks);
		}

		return nBytesSent;
	} else if (!strcmp(name, "hec-errors")) {
		mib_get_s(MIB_PON_MODE, &pon_mode, sizeof(pon_mode));

		if (pon_mode == GPON_MODE) {
			rtk_gpon_global_counter_t counter;

			memset(&counter, 0, sizeof(counter));

			if(rtk_gpon_globalCounter_get(RTK_GPON_PMTYPE_DS_GEM,
						   &counter) != RT_ERR_OK)
				printf("rtk_gpon_globalCounter_get failed: %s %d\n", __func__, __LINE__);

			nBytesSent += boaWrite(wp, "%u", counter.dsgem.rx_hec_correct);
		} else {
			// EPON has no HEC error
			nBytesSent += boaWrite(wp, "0");
		}

		return nBytesSent;
	} else if (!strcmp(name, "packets-dropped")) {
#ifdef CONFIG_COMMON_RT_API
		if(rt_stat_port_get(pon_port_idx, RT_IF_OUT_DISCARDS_INDEX, &value) != RT_ERR_OK)
			printf("rt_stat_port_get failed: %s %d\n", __func__, __LINE__);
#else
		rtk_stat_port_get(pon_port_idx, IF_OUT_DISCARDS_INDEX, &value);
#endif
		return boaWrite(wp, "%llu", value);
	} else if (!strcmp(name, "pause-packets-sent")) {
#ifdef CONFIG_COMMON_RT_API
		if(rt_stat_port_get(pon_port_idx, RT_DOT3_OUT_PAUSE_FRAMES_INDEX, &value) != RT_ERR_OK)
			printf("rt_stat_port_get failed: %s %d\n", __func__, __LINE__);
#else
		rtk_stat_port_get(pon_port_idx, DOT3_OUT_PAUSE_FRAMES_INDEX, &value);
#endif
		return boaWrite(wp, "%llu", value);
	} else if (!strcmp(name, "pause-packets-received")) {
#ifdef CONFIG_COMMON_RT_API
		if(rt_stat_port_get(pon_port_idx, RT_DOT3_IN_PAUSE_FRAMES_INDEX, &value) != RT_ERR_OK)
			printf("rt_stat_port_get failed: %s %d\n", __func__, __LINE__);
#else
		rtk_stat_port_get(pon_port_idx, DOT3_IN_PAUSE_FRAMES_INDEX, &value);
#endif
		return boaWrite(wp, "%llu", value);
	} else if (!strcmp(name, "temperature")) {
		if(rtk_ponmac_transceiver_get
		    (RTK_TRANSCEIVER_PARA_TYPE_TEMPERATURE, &transceiver) != RT_ERR_OK)
			printf("rtk_ponmac_transceiver_get failed: %s %d\n", __func__, __LINE__);

		_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_TEMPERATURE, &transceiver, &readableCfg);
		return boaWrite(wp, "%s", readableCfg.buf);
	} else if (!strcmp(name, "voltage")) {
		if(rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_VOLTAGE,
					   &transceiver) != RT_ERR_OK)
			printf("rtk_ponmac_transceiver_get failed: %s %d\n", __func__, __LINE__);

		_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_VOLTAGE, &transceiver, &readableCfg);
		return boaWrite(wp, "%s", readableCfg.buf);
	} else if (!strcmp(name, "tx-power")) {
		if(rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_TX_POWER,
					   &transceiver) != RT_ERR_OK)
			printf("rtk_ponmac_transceiver_get failed: %s %d\n", __func__, __LINE__);

		_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_TX_POWER, &transceiver, &readableCfg);
		if(strstr(readableCfg.buf, "-inf"))
			return boaWrite(wp, "%s", multilang(LANG_NO_SIGNAL));

		return boaWrite(wp, "%s", readableCfg.buf);
	} else if (!strcmp(name, "rx-power")) {
		if(rtk_ponmac_transceiver_get(RTK_TRANSCEIVER_PARA_TYPE_RX_POWER,
					   &transceiver) != RT_ERR_OK)
			printf("rtk_ponmac_transceiver_get failed: %s %d\n", __func__, __LINE__);

		_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_RX_POWER, &transceiver, &readableCfg);
		if(strstr(readableCfg.buf, "-inf"))
			return boaWrite(wp, "%s", multilang(LANG_NO_SIGNAL));

		return boaWrite(wp, "%s", readableCfg.buf);
	} else if (!strcmp(name, "bias-current")) {
		if(rtk_ponmac_transceiver_get
		    (RTK_TRANSCEIVER_PARA_TYPE_BIAS_CURRENT, &transceiver) != RT_ERR_OK)
			printf("rtk_ponmac_transceiver_get failed: %s %d\n", __func__, __LINE__);

		_get_data_by_type(RTK_TRANSCEIVER_PARA_TYPE_BIAS_CURRENT, &transceiver, &readableCfg);
		return boaWrite(wp, "%s", readableCfg.buf);
	}

	return -1;
}

int ponflowGetStatus(int eid, request * wp, int argc, char **argv)
{
	FILE *cmd = NULL;
	char *name;
	char buff[8];
	size_t n;

	memset(buff, 0, sizeof(buff));

	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if (!strcmp(name, "hw")) {
		cmd = popen("cat /proc/fc/hw_dump/flow_statistic | grep 'Hw flow' | awk '{print $NF}'", "r");
		while ((n = fread(buff, 1, sizeof(buff) - 1, cmd)) > 0)
			buff[n] = '\0';
		pclose(cmd);
		return boaWrite(wp, "%s", buff);
	} else if (!strcmp(name, "sw")) {
		cmd = popen("cat /proc/fc/sw_dump/flow_statistic | grep 'Sw flow' | awk '{print $NF}'", "r");
		while ((n = fread(buff, 1, sizeof(buff) - 1, cmd)) > 0)
			buff[n] = '\0';
		pclose(cmd);
		return boaWrite(wp, "%s", buff);
	}

	return -1;
}

#ifdef USE_DEONET /* sghong, 20230918*/
int portCrcCounterGet(int eid, request * wp, int argc, char **argv)
{       
	int ret;
	int portNum;
	char *name;
	char buff[12];
	rt_stat_port_cntr_t portcnt;
	unsigned int counter;

	ret = rt_stat_port_getAll(0, &portcnt);

	memset(buff, 0, sizeof(buff));

	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if (!strcmp(name, "lan1"))
		portNum = 3;
	else if (!strcmp(name, "lan2"))  
		portNum = 2;
	else if (!strcmp(name, "lan3"))  
		portNum = 1;
	else if (!strcmp(name, "lan4"))  
		portNum = 0;
	else if (!strcmp(name, "wan"))  
		portNum = 5;

	if ((portNum >= 0) && (portNum < 5))
	{
		rt_stat_port_getAll(portNum, &portcnt);
		sprintf(buff, "%lu", portcnt.etherStatsCRCAlignErrors);
	}
	else
	{
		/* WAN BIP error */
		counter = get_wan_crc_counter();
		if(counter < 0)
			sprintf(buff, "%lu", 0);
		else
			sprintf(buff, "%lu", counter);
	}

	return boaWrite(wp, "%s", buff);
}
#endif

#ifdef CONFIG_GPON_FEATURE
void restartOMCIsettings()
{
#ifdef CONFIG_COMMON_RT_API
	if(rt_gpon_deactivate() != RT_ERR_OK)
		printf("rt_gpon_deactivate failed: %s %d\n", __func__, __LINE__);
#else
	rtk_gpon_deActivate();
#endif
	system("omcicli mib reset");
#ifdef CONFIG_COMMON_RT_API
	if(rt_gpon_activate(RT_GPON_ONU_INIT_STATE_O1) != RT_ERR_OK)
		printf("rt_gpon_activate failed: %s %d\n", __func__, __LINE__);
#else
	rtk_gpon_activate(RTK_GPONMAC_INIT_STATE_O1);
#endif
}

int showOMCI_OLT_mode(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0, i;
	char omci_olt_mode;
	const char *OMCI_OLT_MODE_STR_DISP[4];

	OMCI_OLT_MODE_STR_DISP[0] = multilang(LANG_OMCI_OLT_MODE_1);
	OMCI_OLT_MODE_STR_DISP[1] = multilang(LANG_OMCI_OLT_MODE_2);
	OMCI_OLT_MODE_STR_DISP[2] = multilang(LANG_OMCI_OLT_MODE_3);
	OMCI_OLT_MODE_STR_DISP[3] = multilang(LANG_OMCI_OLT_MODE_4);

	if(!mib_get_s(MIB_OMCI_OLT_MODE,  (void *)&omci_olt_mode, sizeof(omci_olt_mode)))
		printf("get MIB_OMCI_OLT_MODE failed\n");

	nBytesSent += boaWrite(wp," <tr>"
#ifndef CONFIG_GENERAL_WEB
	"<td width=\"30%%\"><font size=2><b>%s:</b></td>"
	"<td width=\"70%%\"><font size=2>"
#else
	"<th width=\"30%%\">%s:</th>"
	"<td width=\"70%%\">"
#endif
	"<select name=\"omci_olt_mode\">", multilang(LANG_OMCI_OLT_MODE));

	for(i=0;i<4;i++){
		nBytesSent +=  boaWrite(wp, "<option value=\"%d\" %s>%s</option>",
		i, omci_olt_mode == i ? "selected":"", OMCI_OLT_MODE_STR_DISP[i]);
	}

	nBytesSent +=  boaWrite(wp,"</select></td></tr>");

	return 0;
}

int fmOmciInfo_checkWrite(int eid, request * wp, int argc, char **argv)
{
	char *name, *strData;
	char tmpBuf[100];
	char vChar;
	int i;

	if (boaArgs(argc, argv, "%s", &name) < 1)
	{
   		boaError(wp, 400, "Insufficient args\n");
   		return -1;
   	}
	if(!strcmp(name, "omci_olt_info_readonly"))
	{
		if(!mib_get_s(MIB_OMCI_OLT_MODE,  (void *)&vChar, sizeof(vChar)))
		{
			strcpy(tmpBuf, "get omci olt mode error!\n");
			goto getErr;
		}
		if(vChar == 0)
			boaWrite(wp, "readonly");
		return 0;
	}
	if(!strcmp(name, "omci_olt_mode"))
	{
		if(!mib_get_s(MIB_OMCI_OLT_MODE,  (void *)&vChar, sizeof(vChar)))
		{
			strcpy(tmpBuf, "get omci olt mode error!\n");
			goto getErr;
		}
		boaWrite(wp, "\"%d\"", vChar);
		return 0;
	}
getErr:
	ERR_MSG(tmpBuf);
	return -1;
}

void formOmciInfo(request * wp, char *path, char *query)
{
	char *strData, tmpBuf[200];
	char vChar;
	int intVal;

	strData = boaGetVar(wp, "omci_vendor_id", "");
	if ( strData[0] )
	{
		if(!mib_set(MIB_PON_VENDOR_ID, (void *)strData))
		{
			strcpy(tmpBuf, "Save omci vendor id Error!\n");
			goto setErr;
		}
	}

	strData = boaGetVar(wp, "omci_sw_ver1", "");
	if ( strData[0] )
	{
		if(!mib_set(MIB_OMCI_SW_VER1, (void *)strData))
		{
			strcpy(tmpBuf, "Save omci software version 1 Error!\n");
			goto setErr;
		}
	}

	strData = boaGetVar(wp, "omci_sw_ver2", "");
	if ( strData[0] )
	{
		if(!mib_set(MIB_OMCI_SW_VER2, (void *)strData))
		{
			strcpy(tmpBuf, "Save omci software version 2 Error!\n");
			goto setErr;
		}
	}

	strData = boaGetVar(wp, "omcc_ver", "");
	if ( strData[0] )
	{

		if(!string_to_dec(strData, &intVal))
			printf("string_to_dec failed: %s %d\n", __func__, __LINE__);
		vChar = intVal;

		if(!mib_set(MIB_OMCC_VER, (void *)&vChar))
		{
			strcpy(tmpBuf, "Save omcc version Error!\n");
			goto setErr;
		}
	}

	strData = boaGetVar(wp, "omci_tm_opt", "");
	if ( strData[0] )
	{
		vChar = strData[0] - '0';
		if(!mib_set(MIB_OMCI_TM_OPT, (void *)&vChar))
		{
			strcpy(tmpBuf, "Save omci traffic managament option Error!\n");
			goto setErr;
		}
	}

	strData = boaGetVar(wp, "cwmp_productclass", "");
	if ( strData[0] )
	{
		if(!mib_set(MIB_HW_CWMP_PRODUCTCLASS, (void *)strData))
		{
			strcpy(tmpBuf, "Save cwmp product class Error!\n");
			goto setErr;
		}
	}

	strData = boaGetVar(wp, "cwmp_hw_ver", "");
	if ( strData[0] )
	{
		if(!mib_set(MIB_HW_HWVER, (void *)strData))
		{
			strcpy(tmpBuf, "Save hw version Error!\n");
			goto setErr;
		}
	}

	restartOMCIsettings();

	strData = boaGetVar(wp, "submit-url", "");

	OK_MSG(strData);
#ifdef COMMIT_IMMEDIATELY
	Commit();
	mib_update(HW_SETTING, CONFIG_MIB_ALL);
#endif
	return;

setErr:
	ERR_MSG(tmpBuf);

}
#endif


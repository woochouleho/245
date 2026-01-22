#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef EMBED
#include <linux/config.h>
#include <sys/ioctl.h>
#endif

#include <linux/atmdev.h>
#ifdef CONFIG_ATM_REALTEK
#include <rtk/linux/rtl_sar.h>
#include <linux/atm.h>
#endif
#ifdef CONFIG_ATM_CLIP
#include <linux/atmclip.h>
#include <linux/atmarp.h>
#endif

#include "mib.h"
#include "utility.h"
#include "adsl_drv.h"
#include "debug.h"
#ifdef CONFIG_DEV_xDSL
#include "subr_dsl.h"
#endif

#if defined(CONFIG_DSL_ON_SLAVE)
#define ADSL_DFAULT_DEV 	"/dev/xdsl_ipc"
#elif defined(CONFIG_XDSL_CTRL_PHY_IS_SOC)
#define ADSL_DFAULT_DEV 	"/dev/xdsl0"
#else
#define ADSL_DFAULT_DEV 	"/dev/adsl0"
#endif

#define XDSL_SLAVE_DEV		"/dev/xdsl1"


#ifdef CONFIG_USER_CMD_SERVER_SIDE
const char CMDSERV_CTRLERD[] = "/bin/controllerd";
#endif

const char VC_BR[] = "0";
const char LLC_BR[] = "1";
const char VC_RT[] = "3";
const char LLC_RT[] = "4";
#ifdef CONFIG_ATM_CLIP
const char LLC_1577[] = "6";
#endif
const char PROC_NET_ATM_PVC[] = "/proc/net/atm/pvc";

#ifdef CONFIG_USER_CMD_SERVER_SIDE
#define DSL_CHIP_ID	0
#define DSL_CHIP_PORT 0

#define GFAST_DEBUG(args...) 
//#define GFAST_DEBUG(format, args...) printf("[%s:%d] "format, __FILE__, __LINE__, ##args)

//int commserv_link_status=0;	// 0: link-down, 1: link-up
int commserv_getLinkStatus(void){
	int ret = -1;
	int commserv_link_status=0; // 0: link-down, 1: link-up=
	struct controller_states ctrld_states;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	ret = rtk_xdsl_get_controllerd_status(h, DSL_CHIP_ID, &ctrld_states, sizeof(struct controller_states));
	if (ret || !(ctrld_states.linkstate)) {
		commserv_link_status = 0;
	}else
		commserv_link_status = 1;

	rtk_xdsl_api_fini(h);
	return commserv_link_status; // 0: link-down, 1: link-up
}

/*
 *	Get firmware version of remote.
 */
int commserv_getfwVersion(unsigned char *fw_version)
{
	int ret;
	char states[64];
	struct api_handle *h;
	
	h = (struct api_handle *)rtk_xdsl_api_init();
	ret = rtk_xdsl_get_sys_fw_version(h, DSL_CHIP_ID, fw_version);
	rtk_xdsl_api_fini(h);
	return ret;
}

/*
 *	Get DSP version of remote.
 */
int commserv_getdspVersion(unsigned char *dsp_version)
{
	char version[64]={0}, buildDay[64]={0};

#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
	commserv_gfast_msg_get(UserGetDSPversion, version);
	commserv_gfast_msg_get(UserGetDSPBuild, buildDay);
#else
	adsl_drv_get(RLCM_GET_DRIVER_VERSION, (void *)version, sizeof(version));
	adsl_drv_get(RLCM_GET_DRIVER_BUILD, (void *)buildDay, sizeof(buildDay));
#endif
	sprintf(dsp_version, "%s (%s)", version, buildDay);
	return 1;
}

#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
// RLCM_USER_SET_DSL_DATA_PORT case on remote handler
int commserv_gfast_msg_set(unsigned int id, void *data){
	int ret = -1;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	if (!commserv_getLinkStatus()) {
		printf("[%s]commserv is not link up.\n", __FUNCTION__);
		goto err;
	}

	msgToDslPorts setMsg, getMsg;
	memset(&setMsg, 0, sizeof(msgToDslPorts));
	memset(&getMsg, 0, sizeof(msgToDslPorts));
	
	setMsg.ICNr = DSL_CHIP_ID;
	setMsg.PortStart= DSL_CHIP_PORT;
	setMsg.PortEnd = DSL_CHIP_PORT;

	switch(id){
		/******** G. Fast Set case ********/
		case SetTest:
			/* temporary for control g.fast phy */
			if(*(unsigned int *)data == 0)
				ret = Set_Gfast_Disable_Ports(h, &setMsg, &getMsg, 0);
			else
				ret = Set_Gfast_Enable_Ports(h, &setMsg, &getMsg, 0);

			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
#if 1	// TODO: using common api
	//for 1 argument
	case SetGfastSupportProfile:
		setMsg.message = id;
		((unsigned int*)(setMsg.intVal))[0] = *(unsigned int *)data;
		ret = Set_Gfast_UserData(h, &setMsg, &getMsg, 0);		//flag unuse
		if (ret)
			printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
		else {
			GFAST_DEBUG("[%s] set support Profile success(0x%x)\n", __FUNCTION__, ((unsigned int*)(setMsg.intVal))[0]);
		}
		break;
#endif
		default:
			printf("[%s:%d] unknown id(%d -> 0x%x)\n", __FUNCTION__, __LINE__, id, id);
			break;
	}

err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1); //follow _adsl_drv_get return
}

//RLCM_USER_GET_DSL_DATA_PORT case on remote handler
int commserv_gfast_msg_get(unsigned int id, void *data){
	int ret = -1;
	static uint8_t testbuf[12000];
	msgToDslPorts setMsg, getMsg;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	if (!commserv_getLinkStatus()) {
		printf("[%s]dsl phy is not ready.\n", __FUNCTION__);
		goto err;
	}

	memset(testbuf, 0, sizeof(testbuf));

	switch(id){
		/******** G. Fast Get case ********/
		case UserGetLineState:
			ret = rtk_gfast_dsp_GetLineState(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0); 
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((unsigned int *)data, &((unsigned int *)((msgToDslPorts *)testbuf)->intVal)[1], sizeof(unsigned int));
				GFAST_DEBUG("[%s] case %d, State=%02x\n", __FUNCTION__, id, *(unsigned int *)data);
			}
			break;
		case UserGetGfastSupportProfile:
			ret = rtk_gfast_dsp_GetSupProfile(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((unsigned int *)data, (unsigned int *)((msgToDslPorts *)testbuf)->intVal, sizeof(unsigned int));
				GFAST_DEBUG("[%s] case %d, Supported Profile=%x\n", __FUNCTION__, id, *(unsigned int *)data);
				if(GFAST_PROFILE_106A & *(unsigned int *)data)
					GFAST_DEBUG("GFAST_PROFILE_106A\n");
				if(GFAST_PROFILE_212A & *(unsigned int *)data)
					GFAST_DEBUG("GFAST_PROFILE_212A\n");
				if(GFAST_PROFILE_106B & *(unsigned int *)data)
					GFAST_DEBUG("GFAST_PROFILE_106B\n");
				if(GFAST_PROFILE_106C & *(unsigned int *)data)
					GFAST_DEBUG("GFAST_PROFILE_106C\n");
				if(GFAST_PROFILE_212C & *(unsigned int *)data)
					GFAST_DEBUG("GFAST_PROFILE_212C\n");
				if(GFAST_PROFILE_212B & *(unsigned int *)data)
					GFAST_DEBUG("GFAST_PROFILE_212B\n");
			}
			break;
		case UserGetGfastSelectProfile:
			ret = rtk_gfast_dsp_GetSelProfile(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((unsigned int *)data, (unsigned int *)((msgToDslPorts *)testbuf)->intVal, sizeof(unsigned int));
				GFAST_DEBUG("[%s] case %d, Selected Profile=%x\n", __FUNCTION__, id, *(unsigned int *)data);
				switch(*(unsigned int *)data){
					case GFAST_PROFILE_106A:
						GFAST_DEBUG("GFAST_PROFILE_106A\n");
						break;
					case GFAST_PROFILE_106B:
						GFAST_DEBUG("GFAST_PROFILE_106B\n");
						break;
					case GFAST_PROFILE_106C:
						GFAST_DEBUG("GFAST_PROFILE_106C\n");
						break;
					case GFAST_PROFILE_212A:
						GFAST_DEBUG("GFAST_PROFILE_212A\n");
						break;
					case GFAST_PROFILE_212B:
						GFAST_DEBUG("GFAST_PROFILE_212B\n");
						break;
					case GFAST_PROFILE_212C:
						GFAST_DEBUG("GFAST_PROFILE_212C\n");
						break;
					default:
						GFAST_DEBUG("No supported profile\n");
						break;
				}
			}
			break;
		case UserGetPMDParam:
			ret = rtk_gfast_dsp_GetPmdPara(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((unsigned int *)data, (unsigned int *)((msgToDslPorts *)testbuf)->intVal, sizeof(unsigned int)*6);
				GFAST_DEBUG("[%s] case %d\n", __FUNCTION__, id);
				GFAST_DEBUG("PMD MODE:\n");
				GFAST_DEBUG("Tf: %9d\n", ((unsigned int *)data)[0]);
				GFAST_DEBUG("Msf: %8d\n", ((unsigned int *)data)[1]);
				GFAST_DEBUG("Mds_Start: %2d\n", ((unsigned int *)data)[2]);
				GFAST_DEBUG("Mds_End: %4d\n", ((unsigned int *)data)[3]);
				GFAST_DEBUG("Mus_Start: %2d\n", ((unsigned int *)data)[4]);
				GFAST_DEBUG("Mus_End: %4d\n", ((unsigned int *)data)[5]);
			}
			break;
		case UserGetOlrType:
			ret = rtk_gfast_dsp_GetOlrType(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((unsigned int *)data, (unsigned int *)((msgToDslPorts *)testbuf)->intVal, sizeof(unsigned int));
				GFAST_DEBUG("[%s] case %d, OLR Type:0x%02x%02x%02x%02x\n\n", __FUNCTION__, id, ((uint8_t *)data)[0], ((uint8_t *)data)[1], ((uint8_t *)data)[2], ((uint8_t *)data)[3]);
			}
			break;
		case UserGetErrorCount:
			ret = rtk_gfast_dsp_GetErrCount(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((unsigned int *)data, (unsigned int *)((msgToDslPorts *)testbuf)->intVal, sizeof(unsigned int)*4);
				GFAST_DEBUG("[%s] case %d\n", __FUNCTION__, id);
				GFAST_DEBUG("Upstream CRC of this Port in Lp0: %04x\n", ((unsigned int *)data)[0]);
				GFAST_DEBUG("Upstream CRC of this Port in Lp1: %04x\n", ((unsigned int *)data)[1]);
				GFAST_DEBUG("Downstream CRC of this Port in Lp0: %04x\n", ((unsigned int *)data)[2]);
				GFAST_DEBUG("Downstream CRC of this Port in Lp1: %04x\n", ((unsigned int *)data)[3]);
			}
			break;
		case UserGetSNR:
			ret = rtk_gfast_dsp_GetNeSnr(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				int i=0;
				memcpy(&i, ((msgToDslPorts *)testbuf)->intVal, sizeof(int));	/* first 4 byte ==> tone number */
				memcpy((int *)data, (int *)((msgToDslPorts *)testbuf)->intVal, sizeof(int)*i);
				GFAST_DEBUG("[%s] case %d, NeSnr Tone size=%d\n", __FUNCTION__, id, ((int *)data)[0]);
			}
			break;
		case UserGetTrellisMode:
			ret = rtk_gfast_dsp_GetTrellisMode(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((struct Modem_Trellis*)data, (struct Modem_Trellis*)((msgToDslPorts *)testbuf)->intVal, sizeof(struct Modem_Trellis));
				GFAST_DEBUG("[%s] case %d, Trellis(Tx, Rx) = (%x, %x)\n", __FUNCTION__, id, ((struct Modem_Trellis*)data)->upstreamTrellis, ((struct Modem_Trellis*)data)->downstreamTrellis);
			}
			break;
		case UserGetNEBitAlloc:
			ret = rtk_gfast_dsp_GetNeBi(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				int i=0;
				memcpy(&i, ((msgToDslPorts *)testbuf)->intVal, sizeof(int));	/* first 4 byte ==> tone number */
				memcpy((int *)data, (int *)((msgToDslPorts *)testbuf)->intVal, sizeof(int)*i);
				GFAST_DEBUG("[%s] case %d, NeBi Tone size=%d\n", __FUNCTION__, id, ((int *)data)[0]);
			}
			break;
		case UserGetFEBitAlloc:
			ret = rtk_gfast_dsp_GetFeBi(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				int i=0;
				memcpy(&i, ((msgToDslPorts *)testbuf)->intVal, sizeof(int));	/* first 4 byte ==> tone number */
				memcpy((int *)data, (int *)((msgToDslPorts *)testbuf)->intVal, sizeof(int)*i);
				GFAST_DEBUG("[%s] case %d, FeBi Tone size=%d\n", __FUNCTION__, id, ((int *)data)[0]);
			}
			break;
		case UserGetLinkSpeed:
			ret = rtk_gfast_dsp_GetLineRate(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((struct Modem_LinkSpeed*)data, (struct Modem_LinkSpeed*)((msgToDslPorts *)testbuf)->intVal, sizeof(struct Modem_LinkSpeed));
				GFAST_DEBUG("[%s] case %d, US: %d kbps, DS: %d kbps\n", __FUNCTION__, id, ((struct Modem_LinkSpeed*)data)->upstreamRate, ((struct Modem_LinkSpeed*)data)->downstreamRate);
			}
			break;
		case UserGetDSPversion:
			ret = rtk_gfast_dsp_GetDspVersion(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				strcpy((unsigned char *)data , (unsigned char *)((msgToDslPorts *)testbuf)->intVal);
				GFAST_DEBUG("[%s] case %d, DSP Version: %s\n", __FUNCTION__, id, (unsigned char *)data);
			}
			break;
		case UserGetDSPBuild:
			ret = rtk_gfast_dsp_GetDspBuild(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret); 
			else{
				strcpy((unsigned char *)data , (unsigned char *)((msgToDslPorts *)testbuf)->intVal);
				GFAST_DEBUG("[%s] case %d, DSP Build: %s\n", __FUNCTION__, id, (unsigned char *)data);
			}
			break;
		default:
			printf("[%s:%d] unknown id(%d -> 0x%x)\n", __FUNCTION__, __LINE__, id, id);
			break;
	}

err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1); //follow _adsl_drv_get return
}
#endif /* CONFIG_USER_CMD_SUPPORT_GFAST */

/* 
 *	input:
 *		rValue: serial number in string
 *		len: rValue length
 */
int commserv_set_SerialNum(char *rValue, unsigned int len){
	int ret = 1;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	if (!commserv_getLinkStatus()) {
		printf("[%s]commserv is not link up.\n", __FUNCTION__);
		goto err;
	}

	ret = rtk_xdsl_dsp_Set_Serial_Num(h, DSL_CHIP_ID, DSL_CHIP_PORT, rValue, len);
	if (ret)
		printf("[%s:%d]Error = %d\n", __FUNCTION__, __LINE__, ret);
	else
		GFAST_DEBUG("[%s] rtk_xdsl_dsp_Set_Serial_Num(val=%s, ret=%d)\n", __FUNCTION__, rValue, ret);

err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1);	//follow _adsl_drv_get return
}

// RLCM_UserGetDslData case on remote handler
int commserv_xdsl_msg_get(unsigned int id, void *data){
	int ret = -1;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	if (!commserv_getLinkStatus()) {
		printf("[%s]commserv is not link up.\n", __FUNCTION__);
		goto err;
	}

	switch(id){
		case GetPmdMode:
			ret = rtk_xdsl_dsp_GetPmdMode(h, DSL_CHIP_ID, DSL_CHIP_PORT, (int *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] rtk_xdsl_dsp_GetPmdMode(val=0x%x, ret=%d)\n", __FUNCTION__, *((uint32_t *)data), ret);
			break;
		case GetTrellis:
			ret = rtk_xdsl_dsp_GetTrellis(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case GetPhyR:
			ret = rtk_xdsl_dsp_GetPhyR(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case GetPmsPara_Nfec:
			ret = rtk_xdsl_dsp_GetPmsPara_Nfec(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case GetPmsPara_L:
			ret = rtk_xdsl_dsp_GetPmsPara_L(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case GetPmsPara_INP:
			ret = rtk_xdsl_dsp_GetPmsPara_INP(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case GetTpsMode:
			ret = rtk_xdsl_dsp_GetTpsMode(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case GetAttNDR:
			ret = rtk_xdsl_dsp_GetAttainableRate(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
#if 1	// TODO: using common api, temp solution
		// data len 4 byte
		case GetVectorMode:
			ret = rtk_xdsl_dsp_GetUserData(h, DSL_CHIP_ID, DSL_CHIP_PORT, id, (uint32_t *)data, 1);
			if (ret)
				printf("[%s:%d]id=%d, Error = %d\n", __FILE__, __LINE__, id, ret);
			break;
#endif
		default:
			printf("[%s:%d] unknown id(%d -> 0x%x)\n", __FUNCTION__, __LINE__, id, id);
			break;
	}

err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1); //follow _adsl_drv_get return
}

// // RLCM_UserSetDslData case on remote handler
int commserv_xdsl_msg_set(unsigned int id, void *data){
	int ret = 1;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	if (!commserv_getLinkStatus()) {
		printf("[%s]commserv is not link up.\n", __FUNCTION__);
		goto err;
	}

	switch(id){
		case SetPmdMode:
			ret = rtk_xdsl_dsp_SetPmdMode(h, DSL_CHIP_ID, DSL_CHIP_PORT, *((uint32_t *)data));
			if (ret)
				printf("[%s:%d]Error = %d\n", __FUNCTION__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] rtk_xdsl_dsp_SetPmdMode(val=0x%x, ret=%d)\n", __FUNCTION__, *((uint32_t *)data), ret);
			break;
		case SetVdslProfile:
			ret = rtk_xdsl_dsp_SetVdslProfile(h, DSL_CHIP_ID, DSL_CHIP_PORT, *((uint32_t *)data));
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] SetVdslProfile(val=%u, ret=%d)\n", __FUNCTION__, *((uint32_t *)data), ret);
			break;
		case SetGInp:
			ret = rtk_xdsl_dsp_SetGInp(h, DSL_CHIP_ID, DSL_CHIP_PORT, *((uint32_t *)data));
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] SetGInp(val=%u, ret=%d)\n", __FUNCTION__, *((uint32_t *)data), ret);
			break;
		case SetOlr:
			ret = rtk_xdsl_dsp_SetOlr(h, DSL_CHIP_ID, DSL_CHIP_PORT, *((uint32_t *)data));
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] SetOlr(val=%u, ret=%d)\n", __FUNCTION__, *((uint32_t *)data), ret);
			break;
		case SetGVector:
			ret = rtk_xdsl_dsp_SetGVector(h, DSL_CHIP_ID, DSL_CHIP_PORT, *((uint32_t *)data));
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] SetGVector(val=%u, ret=%d)\n", __FUNCTION__, *((uint32_t *)data), ret);
			break;
		case SetDslBond:
			ret = rtk_xdsl_dsp_SetBondingMode(h, DSL_CHIP_ID, DSL_CHIP_PORT, *((uint8_t *)data));
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret); 
			else
				GFAST_DEBUG("[%s] SetDslBond(val=%u, ret=%d)\n", __FUNCTION__, *((uint8_t *)data), ret);
			break;
		default:
			printf("[%s:%d] unknown id=%d\n", __FUNCTION__, __LINE__, id);
			break;
	}

err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1); //follow _adsl_drv_get return
}

// 
int commserv_xdsl_drv_get(unsigned int id, void *data, unsigned int len){
	int ret = -1;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	if (!commserv_getLinkStatus()) {
		printf("[%s]commserv is not link up.\n", __FUNCTION__);
		goto err;
	}

	switch(id){
		case RLCM_PHY_ENABLE_MODEM:
			ret = rtk_xdsl_dsp_EnableModemLine(h, DSL_CHIP_ID, DSL_CHIP_PORT);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_PHY_ENABLE_MODEM(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_PHY_DISABLE_MODEM:
			ret = rtk_xdsl_dsp_DisableModemLine(h, DSL_CHIP_ID, DSL_CHIP_PORT);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_PHY_DISABLE_MODEM(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_SNR_MARGIN:
			ret = rtk_xdsl_dsp_GetSnrMargin(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_Data *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else{
				GFAST_DEBUG("[%s] RLCM_GET_SNR_MARGIN(ret=%d)\n", __FUNCTION__, ret);
				GFAST_DEBUG("dsdata: %d, usdata: %d\n", ((struct Modem_Data *)data)->dsdata, ((struct Modem_Data *)data)->usdata);
			}
			break;
		case RLCM_MODEM_FAR_END_LINE_DATA_REQ:
			ret = rtk_xdsl_dsp_GetPowerDS(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_FarEndLineOperData *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_MODEM_FAR_END_LINE_DATA_REQ(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_MODEM_NEAR_END_LINE_DATA_REQ:
			ret = rtk_xdsl_dsp_GetPowerUS(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_NearEndLineOperData *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_MODEM_NEAR_END_LINE_DATA_REQ(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_INIT_COUNT_LAST_LINK_RATE:
			ret = rtk_xdsl_dsp_GetInitCountAndLastLinkRate(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_InitLastRate *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_INIT_COUNT_LAST_LINK_RATE(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_DSL_ORHERS:
			ret = rtk_xdsl_dsp_get_sync_time(h, DSL_CHIP_ID, DSL_CHIP_PORT, (unsigned long *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_DSL_ORHERS(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_REHS_COUNT:
			ret = rtk_xdsl_dsp_get_sync_number(h, DSL_CHIP_ID, DSL_CHIP_PORT, (unsigned long *) data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_REHS_COUNT(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_DS_PMS_PARAM1:
			ret = rtk_xdsl_dsp_GetUsPMSParam1(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_PMSParm *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_DS_PMS_PARAM1(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_US_PMS_PARAM1:
			ret = rtk_xdsl_dsp_GetDsPMSParam1(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_PMSParm *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_US_PMS_PARAM1(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_US_ERROR_COUNT:
			ret = rtk_xdsl_dsp_GetUsErrorCount(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_MgmtCounter *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_US_ERROR_COUNT(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_DS_ERROR_COUNT:
			ret = rtk_xdsl_dsp_GetDsErrorCount(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_MgmtCounter *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_DS_ERROR_COUNT(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_FRAME_COUNT:
			ret = rtk_xdsl_dsp_GetFrameCount(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_Data *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_FRAME_COUNT(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_MODEM_RETRAIN:
			ret = rtk_xdsl_dsp_Retrain(h, DSL_CHIP_ID, DSL_CHIP_PORT);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_MODEM_RETRAIN(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_VDSL2_DIAG_SNR:
			ret = rtk_xdsl_dsp_GetVdsl2DiagSnr(h, DSL_CHIP_ID, DSL_CHIP_PORT, data, len);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_VDSL2_DIAG_SNR(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_REPORT_MODEM_STATE:
			ret = rtk_xdsl_dsp_ReportModemStates(h, DSL_CHIP_ID, DSL_CHIP_PORT, (char *)data, (int)len);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_REPORT_MODEM_STATE(ret=%d). data=%s\n", __FUNCTION__, ret, (char *)data);
			break;
		case RLCM_GET_LINK_SPEED:
			ret = rtk_xdsl_dsp_GetLinkSpeed(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_LinkSpeed *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else{
				GFAST_DEBUG("[%s] RLCM_GET_LINK_SPEED(ret=%d)\n", __FUNCTION__, ret);
				GFAST_DEBUG("US: %d kbps, DS: %d kbps\n", ((struct Modem_LinkSpeed*)data)->upstreamRate, ((struct Modem_LinkSpeed*)data)->downstreamRate);
			}
			break;
		case RLCM_SET_ATUR_SERIALNUM:
			ret = rtk_xdsl_dsp_Set_Serial_Num(h, DSL_CHIP_ID, DSL_CHIP_PORT, (char *)data, len);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FUNCTION__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] rtk_xdsl_dsp_Set_Serial_Num(val=%s, ret=%d)\n", __FUNCTION__, (char *)data, ret);
			break;
#if 1	// TODO: using common api, temp solution
		case RLCM_GET_CHANNEL_MODE:
		case RLCM_GET_LINK_POWER_STATE:
		case RLCM_GET_LOOP_ATT:
		case RLCM_GET_DRIVER_VERSION:
		case RLCM_GET_DRIVER_BUILD:
			ret = rtk_xdsl_dsp_drv_get(h, DSL_CHIP_ID, DSL_CHIP_PORT, id, data, len);
			if (ret)
				printf("[%s:%d] default case(id=%d) Error = %d\n", __FILE__, __LINE__, id, ret);
			break;
#endif
		default:
			printf("[%s:%d] unknown id(%d -> 0x%x, %d)\n", __FUNCTION__, __LINE__, id, id, (id-RLCM_IOC_MAGIC));
			break;
	}

err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1); //follow _adsl_drv_get return
}

int commserv_OAMLoobback(unsigned int action, void *data){
	int ret = -1;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	if (!commserv_getLinkStatus()) {
		printf("[%s]commserv is not link up.\n", __FUNCTION__);
		goto err;
	}

	switch(action){		
		case 0:/*OAM_START*/
			ret = rtk_xdsl_oam_loopback_start(h, DSL_CHIP_ID, (struct rtk_xdsl_oam_loopback_req *)data, &(((struct rtk_xdsl_oam_loopback_req *)data)->Tag));
			if (ret){
				printf("oam loopback start failed\n");
			} else {
				GFAST_DEBUG("oam loopback start successfuly: (%d, %d)tag is %d\n", ((struct rtk_xdsl_oam_loopback_req *)data)->vpi
																	, ((struct rtk_xdsl_oam_loopback_req *)data)->vci
																	, ((struct rtk_xdsl_oam_loopback_req *)data)->Tag);
			}
			break;
		case 1:/*OAM_STOP*/
			ret = rtk_xdsl_oam_loopback_stop(h, DSL_CHIP_ID, (struct rtk_xdsl_oam_loopback_req *)data, &(((struct rtk_xdsl_oam_loopback_req *)data)->Tag));
			if (ret){
				printf("oam loopback stop failed\n");
			} else {
				GFAST_DEBUG("oam loopback stop successfuly: (%d, %d)tag is %d\n", ((struct rtk_xdsl_oam_loopback_req *)data)->vpi
																	, ((struct rtk_xdsl_oam_loopback_req *)data)->vci
																	, ((struct rtk_xdsl_oam_loopback_req *)data)->Tag);
			}
			break;
		case 2:/*OAM_RESULT*/
			ret = rtk_xdsl_oam_loopback_result(h, DSL_CHIP_ID, (struct rtk_xdsl_oam_loopback_state *)data);
			if (ret){
				printf("oam loopback result: fail\n");
			} else {
				GFAST_DEBUG("oam loopback success:\n");
				GFAST_DEBUG("(vpi, vci):  (%d, %d)\n", ((struct rtk_xdsl_oam_loopback_state *)data)->vpi, ((struct rtk_xdsl_oam_loopback_state *)data)->vci);
				GFAST_DEBUG("count:  %d\n", ((struct rtk_xdsl_oam_loopback_state *)data)->count[0]);
				GFAST_DEBUG("time:   %d\n", ((struct rtk_xdsl_oam_loopback_state *)data)->rtt[0]);
				GFAST_DEBUG("status: %d\n", ((struct rtk_xdsl_oam_loopback_state *)data)->status[0]);
			}
			break;
		default:
			break;
	}

	usleep(1000);	//avoid continuous send c-api
err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1);	//follow _adsl_drv_get return
}

int commserv_atmSetup(unsigned int action, struct rtk_xdsl_atm_channel_info *data){
	int ret = -1;

	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	if (!commserv_getLinkStatus()) {
		printf("[%s]commserv is not link up.\n", __FUNCTION__);
		goto err;
	}

	switch(action){		
		case CMD_ADD:/*ADD*/
			ret = rtk_xdsl_add_vc(h, DSL_CHIP_ID, data);
			if (ret){
				printf("rtk_xdsl_add_vc fail\n");
			}else{
				GFAST_DEBUG("Create remote VC success,\nch_no=%d, (vpi,vci)=(%d, %d),\t\nrfc=%d, framing=%d,\n\tQoS_T=%d, pcr=%d, scr=%d, mbs=%d\n", 
					data->ch_no, data->vpi, data->vci, data->rfc, data->framing, data->qos_type, data->pcr, data->scr, data->mbs);
			}
			break;
		case CMD_DEL:/*DELETE*/
			ret = rtk_xdsl_del_vc(h, DSL_CHIP_ID, data);
			if (ret){
				printf("rtk_xdsl_del_vc fail, (vpi,vci)=(%d, %d)\n", data->vpi, data->vci);
			}else{
				GFAST_DEBUG("rtk_xdsl_delete_vc fail, (vpi,vci)=(%d, %d)\n", data->vpi, data->vci);
			}			
			break;
		default:
			break;
	}

err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1);	//follow _adsl_drv_get return
}

int commserv_GetHandler(unsigned int id, void *data){
#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
	if(id < GFAST_GET_BEGIN){
		return commserv_xdsl_msg_get(id, data);
	}else
		return commserv_gfast_msg_get(id, data);
#else
	return commserv_xdsl_msg_get(id, data);
#endif
}

int commserv_SetHandler(unsigned int id, void *data){
#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
	if(id < GFAST_SET_BEGIN){
		return commserv_xdsl_msg_set(id, data);
	}else
		return commserv_gfast_msg_set(id, data);
#else
	return commserv_xdsl_msg_set(id, data);
#endif
}
#endif /* CONFIG_USER_CMD_SERVER_SIDE */

#ifdef CONFIG_DEV_xDSL
// find if vc exists in /proc/net/atm/pvc
int find_pvc_from_running(int vpi, int vci)
{
	char buff[256];
	FILE *fp;
	int tvpi, tvci;
	int found=0;

	if (!(fp=fopen(PROC_NET_ATM_PVC, "r"))) {
		fclose(fp);
		printf("%s not exists.\n", PROC_NET_ATM_PVC);
	}
	else {
		fgets(buff, sizeof(buff), fp);
		while( fgets(buff, sizeof(buff), fp) != NULL ) {
			if(sscanf(buff, "%*d%d%d", &tvpi, &tvci)!=2) {
				found=0;
				printf("Unsuported pvc configuration format\n");
				break;
			}
			if ( tvpi==vpi && tvci==vci ) {
				found = 1;
				break;
			}
		}
		fclose(fp);
	}
	return found;
}
#endif//CONFIG_DEV_xDSL

// calculate the 15-0(bits) Cell Rate register value (PCR or SCR)
// return its corresponding register value
static int cr2reg(int pcr)
{
#ifdef CONFIG_RTL8672
	return pcr;
#else
	int k, e, m, pow2, reg;

	k = pcr;
	e=0;

	while (k>1) {
		k = k/2;
		e++;
	}

	//printf("pcr=%d, e=%d\n", pcr,e);
	pow2 = 1;
	for (k = 1; k <= e; k++)
		pow2*=2;

	//printf("pow2=%d\n", pow2);
	//m = ((pcr/pow2)-1)*512;
	k = 0;
	while (pcr >= pow2) {
		pcr -= pow2;
		k++;
	}
	m = (k-1)*512 + pcr*512/pow2;
	//printf("m=%d\n", m);
	reg = (e<<9 | m );
	//printf("reg=%d\n", reg);
	return reg;
#endif
}

#ifdef CONFIG_DEV_xDSL
#ifdef CONFIG_RTL_MULTI_PVC_WAN
static void addVCVirtualDev(MIB_CE_ATM_VC_Tp pEntry)
{
	MEDIA_TYPE_T mType;
	char ifname[IFNAMSIZ];  // virtual vc device name
	char wanif[IFNAMSIZ];   // major vc device name

	// Get virtual vc device name
	ifGetName(PHY_INTF(pEntry->ifIndex), ifname, sizeof(ifname));
	// Get major vc device name
	//ifGetName(MAJOR_VC_INTF(pEntry->ifIndex), wanif, sizeof(wanif));
	snprintf(wanif, IFNAMSIZ,  "vc%u", VC_MAJOR_INDEX(pEntry->ifIndex));
	mType = MEDIA_INDEX(pEntry->ifIndex);

	//add new vcwan dev here.
	if (mType == MEDIA_ATM && (WAN_MODE & MODE_ATM)) {
#if defined(CONFIG_RTL_SMUX_DEV)
		char cmd_str[256]={0}, *pcmd_str = NULL;
		unsigned char *rootdev=NULL;
		unsigned int vlantag;
		
		#ifdef CONFIG_PTMWAN
		if(mType == MEDIA_PTM)
			rootdev=ALIASNAME_PTM0;
		else
		#endif /*CONFIG_PTMWAN*/
			rootdev=ALIASNAME_NAS0;
		sprintf(cmd_str, "/bin/smuxctl --if-create-name %s %s", rootdev, ifname);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
		
		pcmd_str = cmd_str;
		pcmd_str += sprintf(pcmd_str, "/bin/smuxctl --if %s --rx", rootdev);
		if (pEntry->vlan) 
		{// filter vlan ID on RX 
			//vlantag = (pEntry->vid|((pEntry->vprio) << 13));
			vlantag = pEntry->vid;
			pcmd_str += sprintf(pcmd_str, " --tags 1 --filter-vid 0x%x 1 --pop-tag", vlantag);
		}
		else {
			pcmd_str += sprintf(pcmd_str, " --tags 0");
		}
		if(!(((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
				|| ROUTE_CHECK_BRIDGE_MIX_ENABLE(pEntry)
		))
		{// filter dest addr on unicast
			pcmd_str += sprintf(pcmd_str, " --filter-unicast-mac 1");
		}
		
		pcmd_str += sprintf(pcmd_str, " --set-rxif %s --rule-alias %s-rx-default --rule-append", ifname, ifname);
		printf("==> [%s] %s\n", __FUNCTION__, cmd_str);
		va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
		
		if (pEntry->vlan)
		{
			//vlantag = (pEntry->vid|((pEntry->vprio) << 13));
			vlantag = pEntry->vid;
			cmd_str[0] = '\0';
			pcmd_str = cmd_str;
			pcmd_str += sprintf(pcmd_str, "/bin/smuxctl --if %s --tx", rootdev);
			pcmd_str += sprintf(pcmd_str, " --tags 0");
			pcmd_str += sprintf(pcmd_str, " --filter-txif %s", ifname);
			pcmd_str += sprintf(pcmd_str, " --push-tag --set-vid 0x%x 1", vlantag);
			pcmd_str += sprintf(pcmd_str, " --rule-alias %s-tx-default --rule-append", ifname);
			printf("==> [%s] %s\n", __FUNCTION__, cmd_str);
			va_cmd("/bin/sh", 2, 1, "-c", cmd_str);
		}
#else
		int tmp_group;
		const char smux_brg_cmd[]="/bin/ethctl addsmux bridge %s %s";
		const char smux_ipoe_cmd[]="/bin/ethctl addsmux ipoe %s %s";
		const char smux_pppoe_cmd[]="/bin/ethctl addsmux pppoe %s %s";
		char cmd_str[100];

		//va_cmd(IFCONFIG, 2, 1, "vc0", "up");
		if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
			snprintf(cmd_str, 100, smux_brg_cmd, wanif, ifname);
		else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_IPOE)
			snprintf(cmd_str, 100, smux_ipoe_cmd, wanif, ifname);
		else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
			snprintf(cmd_str, 100, smux_pppoe_cmd, wanif, ifname);

		if (pEntry->napt)
			strncat(cmd_str, " napt", 100);

		#ifdef PPPOE_PASSTHROUGH
		if(pEntry->brmode != BRIDGE_DISABLE)
			strncat(cmd_str, " brpppoe", 100);
		#endif

		if (pEntry->vlan) {
			unsigned int vlantag;
			vlantag = (pEntry->vid|((pEntry->vprio) << 13));
			snprintf(cmd_str, 100, "%s vlan %d", cmd_str, vlantag );
		}
		printf("TRACE: %s\n", cmd_str);
		system(cmd_str);
#endif
	}
}
#endif

#ifdef CONFIG_USER_CMD_SERVER_SIDE
/*
 *	setup remote phy ATM channel: RFC2684, RFC1577, PPPoA
 */
int setup_remote_ATM_channel(MIB_CE_ATM_VC_Tp pEntry)
{
	struct rtk_xdsl_atm_channel_info data;
//	char *aal5Encap, qosParms[64];
	int pcreg, screg;

	if (MEDIA_INDEX(pEntry->ifIndex) != MEDIA_ATM)
		return 0;

	data.vpi = pEntry->vpi;
	data.vci = pEntry->vci;

	//setting QoS and it is a pair with commserv vcctl.c
	data.qos_type = ((ATM_QOS_T)pEntry->qos)+1;

	// calculate the 15-0(bits) PCR CTLSTS value
	pcreg = cr2reg(pEntry->pcr);

	/* due to qos_type=0 is for none case in commserv2, so qos_type need plus 1 */
	if ((ATM_QOS_T)pEntry->qos == ATMQOS_CBR)
	{
//		snprintf(qosParms, 64, "cbr:pcr=%u", pcreg);
		data.qos_type = ATMQOS_CBR+1;
		data.pcr = pcreg;
		data.mbs = pEntry->mbs;
	}
	else if ((ATM_QOS_T)pEntry->qos == ATMQOS_VBR_NRT)
	{
		screg = cr2reg(pEntry->scr);
//		snprintf(qosParms, 64, "nrt-vbr:pcr=%u,scr=%u,mbs=%u", pcreg, screg, pEntry->mbs);
		data.qos_type = ATMQOS_VBR_NRT+1;
		data.pcr = pcreg;
		data.scr = screg;
		data.mbs = pEntry->mbs;

	}
	else if ((ATM_QOS_T)pEntry->qos == ATMQOS_VBR_RT)
	{
		screg = cr2reg(pEntry->scr);
//		snprintf(qosParms, 64, "rt-vbr:pcr=%u,scr=%u,mbs=%u", pcreg, screg, pEntry->mbs);
		data.qos_type = ATMQOS_VBR_RT+1;
		data.pcr = pcreg;
		data.scr = screg;
		data.mbs = pEntry->mbs;
	}
	else //if ((ATM_QOS_T)pEntry->qos == ATMQOS_UBR)
	{
//		snprintf(qosParms, 64, "ubr:pcr=%u", pcreg);
		data.qos_type = ATMQOS_UBR+1;
		data.pcr = pcreg;
		data.mbs = pEntry->mbs;
	}

	// assign framing
	if (pEntry->encap == ENCAP_VCMUX)
		data.framing=RTK_XDSL_VC_MUX;
	else	// LLC
		data.framing=RTK_XDSL_LLC_SNAP;


	switch (pEntry->cmode) {
#ifdef CONFIG_PPP
	case CHANNEL_MODE_PPPOA:
		// PPPoA
		printf("[%s]PPPoA case\n", __FUNCTION__);
		data.rfc=RTK_XDSL_RFC2364;	/* PPP over ATM */
		commserv_atmSetup(CMD_ADD, &data);
		break;
#endif
#ifdef CONFIG_ATM_CLIP
	case CHANNEL_MODE_RT1577:
		// rfc1577-routed
		printf("1577 routed, Not implement\n");
		break;
#endif
	case CHANNEL_MODE_BRIDGE:
	case CHANNEL_MODE_IPOE:
	case CHANNEL_MODE_PPPOE:
	case CHANNEL_MODE_RT1483:
		printf("[%s]BR2684 case\n", __FUNCTION__);

		if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1483)
			data.rfc=RTK_XDSL_RFC1483_ROUTED;	/* Ethernet over ATM (routed IP) */
		else
			data.rfc=RTK_XDSL_RFC1483_BRIDGED;	/* Ethernet over ATM (bridged) */

		commserv_atmSetup(CMD_ADD, &data);
		break;
	default:
		printf("%s: Unknown channel mode(%d)\n", pEntry->cmode);
		break;
	}

	return 1;
}

#endif

/*
 *	ATM channel: RFC2684, RFC1577, PPPoA
 */
int setup_ATM_channel(MIB_CE_ATM_VC_Tp pEntry)
{
	struct data_to_pass_st msg;
	char *aal5Encap, qosParms[64];
	char wanif[IFNAMSIZ];
	int pcreg, screg;
	int repeat_i;
#ifdef CONFIG_ATM_CLIP
	unsigned long addr;
#endif

	if (MEDIA_INDEX(pEntry->ifIndex) != MEDIA_ATM)
		return 0;

	// get the aal5 encapsulation
	if (pEntry->encap == ENCAP_VCMUX)
	{
#ifdef CONFIG_ATM_CLIP
		if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1483 || (CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1577)
#else
		if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1483)
#endif
			aal5Encap = (char *)VC_RT;
		else
			aal5Encap = (char *)VC_BR;
	}
	else	// LLC
	{
		if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1483)
			aal5Encap = (char *)LLC_RT;
#ifdef CONFIG_ATM_CLIP
		else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1577)
			aal5Encap = (char *)LLC_1577;
#endif
		else
			aal5Encap = (char *)LLC_BR;
	}

	// calculate the 15-0(bits) PCR CTLSTS value
	pcreg = cr2reg(pEntry->pcr);

	if ((ATM_QOS_T)pEntry->qos == ATMQOS_CBR)
	{
		snprintf(qosParms, 64, "cbr:pcr=%u", pcreg);
	}
	else if ((ATM_QOS_T)pEntry->qos == ATMQOS_VBR_NRT)
	{
		screg = cr2reg(pEntry->scr);
		snprintf(qosParms, 64, "nrt-vbr:pcr=%u,scr=%u,mbs=%u",
			pcreg, screg, pEntry->mbs);
	}
	else if ((ATM_QOS_T)pEntry->qos == ATMQOS_VBR_RT)
	{
		screg = cr2reg(pEntry->scr);
		snprintf(qosParms, 64, "rt-vbr:pcr=%u,scr=%u,mbs=%u",
			pcreg, screg, pEntry->mbs);
	}
	else //if ((ATM_QOS_T)pEntry->qos == ATMQOS_UBR)
	{
		snprintf(qosParms, 64, "ubr:pcr=%u", pcreg);
	}
	// Mason Yu Test
	//printf("qosParms=%s\n", qosParms);
#ifdef CONFIG_RTL_MULTI_PVC_WAN
	ifGetName(MAJOR_VC_INTF(pEntry->ifIndex), wanif, sizeof(wanif));
#else
	ifGetName(PHY_INTF(pEntry->ifIndex), wanif, sizeof(wanif));
#endif
	// start this vc
	if (pEntry->cmode != CHANNEL_MODE_PPPOA && !find_pvc_from_running(pEntry->vpi, pEntry->vci))
	{
		unsigned char devAddr[MAC_ADDR_LEN];
		char macaddr[MAC_ADDR_LEN*2+1];

#ifdef CONFIG_USER_CMD_SERVER_SIDE
		setup_remote_ATM_channel(pEntry);
#endif

		// mpoactl add vc0 pvc 0.33 encaps 4 qos xxx brpppoe x

#ifdef CONFIG_ATM_CLIP
		if (pEntry->cmode != CHANNEL_MODE_RT1577)
#endif

		// 20130426 W.H. Hung: briged PPPoE setup has been replaced
		// by ebtables rules.
		snprintf(msg.data, BUF_SIZE,
			"mpoactl add %s pvc %u.%u encaps %s qos %s", wanif,
			pEntry->vpi, pEntry->vci, aal5Encap, qosParms);
#ifdef CONFIG_ATM_CLIP
		else
			snprintf(msg.data, BUF_SIZE,
				"mpoactl addclip %s pvc %u.%u encaps %s qos %s", wanif,
				pEntry->vpi, pEntry->vci, aal5Encap, qosParms);
#endif

		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_mpoad(&msg);

		{
			//make sure the channel is created (ifconfig hw ether may fail because the vc# is not ready)
			repeat_i = 0;
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			while( (repeat_i<10) )
#else
			while( (repeat_i<10) && (!find_pvc_from_running(pEntry->vpi, pEntry->vci)) )
#endif
			{
				usleep(50000);
				repeat_i++;
			}
		}
#ifdef CONFIG_RTL_MULTI_PVC_WAN
		// start root vc for multi-wan PVC
		setup_ethernet_config(pEntry, wanif);
#endif
	}

	switch (pEntry->cmode) {
#ifdef CONFIG_PPP
	case CHANNEL_MODE_PPPOA:
		// PPPoA
		printf("PPPoA\n");
		if (startPPP(0, pEntry, qosParms, CHANNEL_MODE_PPPOA) == -1)
		{
			printf("start PPPoA failed !\n");
			return 0;
		}
#ifdef CONFIG_IPV6
		if (startPPP_for_V6(pEntry) == -1)
		{
			printf("start PPPoA IPv6 failed !\n");
			return 0;
		}
#endif

#ifdef CONFIG_USER_CMD_SERVER_SIDE
		setup_remote_ATM_channel(pEntry);
#endif

		//wait finishing sar_open; sar_open will reset SAR header, so sarctl set_header must be called after it
		repeat_i = 0;
		while( (repeat_i < 20) && (!find_pvc_from_running(pEntry->vpi, pEntry->vci)) )
		{
			usleep(50000);
			repeat_i++;
		}
		if (repeat_i >= 20) printf("\nPPPoA channel may not be configured properly, please reboot to reload the configuration!\n");
		break;
#endif
#ifdef CONFIG_ATM_CLIP
	case CHANNEL_MODE_RT1577:
		// rfc1577-routed
		printf("1577 routed\n");
		addr = *((unsigned long *)pEntry->ipAddr);
		snprintf(msg.data, BUF_SIZE, "mpoactl set %s cip %lu", wanif, addr);
		write_to_mpoad(&msg);
		break;
#endif
	}

#ifdef CONFIG_RTL_MULTI_PVC_WAN
	// create virtual interface, ex. vc0_0
	addVCVirtualDev(pEntry);
#endif
	return 1;
}
#endif // CONFIG_DEV_xDSL

#ifdef CONFIG_RTL_MULTI_PVC_WAN
int find_latest_used_vc(MIB_CE_ATM_VC_Tp pEntry)
{
	int entryNum=0, i, found_another=0;
	MIB_CE_ATM_VC_T Entry;

	entryNum =  mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;
		if (pEntry->ifIndex == Entry.ifIndex)
			continue;

		if (MEDIA_INDEX(Entry.ifIndex) == MEDIA_ATM) {
		if (VC_MAJOR_INDEX(Entry.ifIndex) == VC_MAJOR_INDEX(pEntry->ifIndex)) {
				found_another = 1;
		}
	}
	}
	return found_another;
}
#endif

#ifdef CONFIG_USER_CMD_SERVER_SIDE
/*
** startCommserv should run after wan setting
** return value:
**	success:0
**	fail:1
*/
int startCommserv(void){
#ifdef CONFIG_DEV_xDSL
	MIB_CE_ATM_VC_T Entry;
	int vcTotal=0, i=0;
#endif

	/* it should enable switch before launch commserver */
#if defined(CONFIG_LUNA) && defined(CONFIG_RTL8686_SWITCH)
	set_all_port_powerdown_state(0);
#endif
	if(va_cmd("/bin/ethctl", 3, 1, "ignsmux", "add", "0x8899"))
		return 1;
	if(va_cmd("/bin/ethctl", 3, 1, "ignsmux", "add", "0x884c"))
		return 1;
	if(va_niced_cmd(CMDSERV_CTRLERD, 2, 0, "-i", CMD_CLIENT_MONITOR_INTF))
		return 1;

	while(!commserv_getLinkStatus()){
		sleep(1); // waiting DUT create commserv connection
	}

#ifdef CONFIG_DEV_xDSL
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if (Entry.enable == 0)
			continue;

		setup_remote_ATM_channel(&Entry);
	}
#endif

	return 0;
}
#endif

#ifdef CONFIG_DEV_xDSL
static char open_adsl_drv(char *dev_name)
{
	int ret = 1;
	struct id_info *id_info_r;

	id_info_r = id_info_search(dev_name);

	if(id_info_r->file_ptr != NULL){
		printf("Error : file fd already in use.\n");
		ret = 0;
		return ret;
	}

	if ((id_info_r->file_ptr = fopen(/*adslDevice*/dev_name, "r")) == NULL) {
//		printf("ERROR: failed to open %s, error(%s)\n", /*adslDevice*/dev_name,
//				strerror(errno));
			ret = 0;
			return ret;
	}
	return ret;
}

static void close_adsl_drv(char *dev_name)
{
	struct id_info *id_info_r = NULL;

	id_info_r = id_info_search(dev_name);

	if (id_info_r->file_ptr) {
		fclose(id_info_r->file_ptr);
		id_info_r->file_ptr = NULL;
	}
}


char _adsl_drv_get(char *dev_name ,unsigned int id, void *rValue, unsigned int len)
{
	int ret = 0;
	struct id_info *id_info_r = NULL;
	id_info_r = id_info_search(dev_name);

#ifdef EMBED
	if (open_adsl_drv(dev_name)) {
		obcif_arg myarg;
		myarg.argsize = (int)len;
		myarg.arg = (int)rValue;

		if (ioctl(fileno(id_info_r->file_ptr), id, &myarg) >= 0)
			ret = 1;

		close_adsl_drv(dev_name);

		return ret;
	}
#endif
	return ret;
}


char adsl_drv_get(unsigned int id, void *rValue, unsigned int len)
{
#ifdef CONFIG_USER_CMD_SERVER_SIDE
	return commserv_xdsl_drv_get(id, rValue, len);
#else
	return _adsl_drv_get(ADSL_DFAULT_DEV, id, rValue, len);
#endif
}

#ifdef CONFIG_VDSL
/*pval: must be an int[4]-arrary pointer*/
static char dsl_msg(unsigned int id, int msg, int *pval)
{
	MSGTODSL msg2dsl;
	char ret=0;

	msg2dsl.message=msg;
	msg2dsl.intVal=pval;
	ret=adsl_drv_get(id, &msg2dsl, sizeof(MSGTODSL));

	return ret;
}

char dsl_msg_set_array(int msg, int *pval)
{
	char ret=0;

	if(pval)
	{
#ifdef CONFIG_USER_CMD_SERVER_SIDE
		ret = commserv_SetHandler(msg, pval);
#else
		ret=dsl_msg(RLCM_UserSetDslData, msg, pval);
#endif
	}
	return ret;
}

char dsl_msg_set(int msg, int val)
{
	int tmpint[4];
	char ret=0;

	tmpint[0]=val;
	ret=dsl_msg_set_array(msg, tmpint);
	return ret;
}

char dsl_msg_get_array(int msg, int *pval)
{
	char ret=0;

	if(pval)
	{
#ifdef CONFIG_USER_CMD_SERVER_SIDE
		ret = commserv_GetHandler(msg, pval);
#else
		ret=dsl_msg(RLCM_UserGetDslData, msg, pval);
#endif
	}
	return ret;
}

char dsl_msg_get(int msg, int *pval)
{
	int tmpint[4];
	char ret=0;

	if(pval)
	{
		memset(tmpint, 0, sizeof(tmpint));
		ret=dsl_msg_get_array(msg, tmpint);
		if(ret) *pval=tmpint[0];
	}
	return ret;
}
#endif /*#CONFIG_VDSL*/

struct id_info id_name_mapping[] = {
	{0,ADSL_DFAULT_DEV,NULL},
	{1,XDSL_SLAVE_DEV,NULL}
};

struct id_info* id_info_search(char *dev_name){
	int i;
	struct id_info *ret = NULL;

	for(i=0;i<(sizeof(id_name_mapping) / sizeof(struct id_info));i++){
		if(strcmp(id_name_mapping[i].dev_name, dev_name) == 0){
			ret = &id_name_mapping[i];
			break;
		}
	}

	return ret;
}


static XDSL_OP xdsl0_op =
{
	0,
	adsl_drv_get,
#ifdef CONFIG_VDSL
	dsl_msg_set_array,
	dsl_msg_get_array,
	dsl_msg_set,
	dsl_msg_get,
#endif /*CONFIG_VDSL*/
	(int  (*)(int, char *, int))getAdslInfo,
};

#ifdef CONFIG_USER_XDSL_SLAVE
static XDSL_OP xdsl1_op =
{
	1,
	adsl_slv_drv_get,
#ifdef CONFIG_VDSL
	dsl_slv_msg_set_array,
	dsl_slv_msg_get_array,
	dsl_slv_msg_set,
	dsl_slv_msg_get,
#endif /*CONFIG_VDSL*/
	getAdslSlvInfo,
};
#endif /*CONFIG_USER_XDSL_SLAVE*/

static XDSL_OP *xdsl_op[]=
{
	&xdsl0_op,
#ifdef CONFIG_USER_XDSL_SLAVE
	&xdsl1_op
#endif /*CONFIG_USER_XDSL_SLAVE*/
};

XDSL_OP *xdsl_get_op(int id)
{
#ifdef CONFIG_USER_XDSL_SLAVE
	if(id)
	{
		if(id!=1) printf( "%s: error id=%d\n", __FUNCTION__, id );
		return xdsl_op[1];
	}
#endif /*CONFIG_USER_XDSL_SLAVE*/

	if(id!=0) printf( "%s: error id=%d\n", __FUNCTION__, id );
	return xdsl_op[0];
}

#ifdef CONFIG_DSL_VTUO
static void VTUOArrayDump(char *s, int *a, int size)
{
	int i;
	printf("\nDump %s array, size=%d\n", s?s:"", size );
	for(i=0; i<size; i++)
		printf( "  %02d: %d\n", i, a[i] );
	return;
}

int VTUOSetupChan(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	int tmp2dsl[6];
	unsigned int tmpUInt;
	unsigned char tmpUChar;

	mib_get_s(VTUO_CHAN_DS_NDR_MAX, (void *)&tmpUInt, sizeof(tmpUInt));
	tmp2dsl[0]=tmpUInt;
	mib_get_s(VTUO_CHAN_DS_NDR_MIN, (void *)&tmpUInt, sizeof(tmpUInt));
	tmp2dsl[1]=tmpUInt;
	mib_get_s(VTUO_CHAN_DS_MAX_DELAY, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[2]=tmpUChar;
	mib_get_s(VTUO_CHAN_DS_MIN_INP, (void *)&tmpUChar, sizeof(tmpUChar));
	if(tmpUChar==17) tmpUInt=5;
	else tmpUInt=tmpUChar*10;
	tmp2dsl[3]= tmpUInt;
	mib_get_s(VTUO_CHAN_DS_MIN_INP8, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[4]=tmpUChar*10;
	mib_get_s(VTUO_CHAN_DS_SOS_MDR, (void *)&tmpUInt, sizeof(tmpUInt));
	tmp2dsl[5]=tmpUInt;
	VTUOArrayDump( "SetChnProfDs", tmp2dsl, 6 );
	d->xdsl_msg_set_array( SetChnProfDs, tmp2dsl );

	mib_get_s(VTUO_CHAN_US_NDR_MAX, (void *)&tmpUInt, sizeof(tmpUInt));
	tmp2dsl[0]=tmpUInt;
	mib_get_s(VTUO_CHAN_US_NDR_MIN, (void *)&tmpUInt, sizeof(tmpUInt));
	tmp2dsl[1]=tmpUInt;
	mib_get_s(VTUO_CHAN_US_MAX_DELAY, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[2]=tmpUChar;
	mib_get_s(VTUO_CHAN_US_MIN_INP, (void *)&tmpUChar, sizeof(tmpUChar));
	if(tmpUChar==17) tmpUInt=5;
	else tmpUInt=tmpUChar*10;
	tmp2dsl[3]= tmpUInt;
	mib_get_s(VTUO_CHAN_US_MIN_INP8, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[4]=tmpUChar*10;
	mib_get_s(VTUO_CHAN_US_SOS_MDR, (void *)&tmpUInt, sizeof(tmpUInt));
	tmp2dsl[5]=tmpUInt;
	VTUOArrayDump( "SetChnProfUs", tmp2dsl, 6 );
	d->xdsl_msg_set_array( SetChnProfUs, tmp2dsl );
	return 0;
}

int VTUOSetupGinp(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	int tmp2dsl[11];
	unsigned int tmpUInt;
	unsigned short tmpUShort;
	unsigned char tmpUChar;

	mib_get_s(VTUO_GINP_DS_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[0]=tmpUChar;
	mib_get_s(VTUO_GINP_DS_ET_MAX, (void *)&tmpUInt, sizeof(tmpUInt));
	tmp2dsl[1]=tmpUInt;
	mib_get_s(VTUO_GINP_DS_ET_MIN, (void *)&tmpUInt, sizeof(tmpUInt));
	tmp2dsl[2]=tmpUInt;
	mib_get_s(VTUO_GINP_DS_MAX_DELAY, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[3]=tmpUChar;
	mib_get_s(VTUO_GINP_DS_MIN_DELAY, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[4]=tmpUChar;
	mib_get_s(VTUO_GINP_DS_MIN_INP, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[5]=tmpUChar*10;
	mib_get_s(VTUO_GINP_DS_REIN_SYM, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[6]=tmpUChar*10;
	mib_get_s(VTUO_GINP_DS_REIN_FREQ, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[7]=tmpUChar;
	mib_get_s(VTUO_GINP_DS_SHINE_RATIO, (void *)&tmpUShort, sizeof(tmpUShort));
	tmp2dsl[8]=tmpUShort;
	mib_get_s(VTUO_GINP_DS_LEFTR_THRD, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[9]=tmpUChar;
	mib_get_s(VTUO_GINP_DS_NDR_MAX, (void *)&tmpUInt, sizeof(tmpUInt));
	tmp2dsl[10]=tmpUInt;
	VTUOArrayDump( "SetChnProfGinpDs", tmp2dsl, 11 );
	d->xdsl_msg_set_array( SetChnProfGinpDs, tmp2dsl );

	mib_get_s(VTUO_GINP_US_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[0]=tmpUChar;
	mib_get_s(VTUO_GINP_US_ET_MAX, (void *)&tmpUInt, sizeof(tmpUInt));
	tmp2dsl[1]=tmpUInt;
	mib_get_s(VTUO_GINP_US_ET_MIN, (void *)&tmpUInt, sizeof(tmpUInt));
	tmp2dsl[2]=tmpUInt;
	mib_get_s(VTUO_GINP_US_MAX_DELAY, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[3]=tmpUChar;
	mib_get_s(VTUO_GINP_US_MIN_DELAY, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[4]=tmpUChar;
	mib_get_s(VTUO_GINP_US_MIN_INP, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[5]=tmpUChar*10;
	mib_get_s(VTUO_GINP_US_REIN_SYM, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[6]=tmpUChar*10;
	mib_get_s(VTUO_GINP_US_REIN_FREQ, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[7]=tmpUChar;
	mib_get_s(VTUO_GINP_US_SHINE_RATIO, (void *)&tmpUShort, sizeof(tmpUShort));
	tmp2dsl[8]=tmpUShort;
	mib_get_s(VTUO_GINP_US_LEFTR_THRD, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[9]=tmpUChar;
	mib_get_s(VTUO_GINP_US_NDR_MAX, (void *)&tmpUInt, sizeof(tmpUInt));
	tmp2dsl[10]=tmpUInt;
	VTUOArrayDump( "SetChnProfGinpUs", tmp2dsl, 11 );
	d->xdsl_msg_set_array( SetChnProfGinpUs, tmp2dsl );
	return 0;
}

int VTUOSetupLine(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	int tmp2dsl[10];
	unsigned int tmpUInt;
	unsigned short tmpUShort;
	unsigned char tmpUChar;
	short tmpShort;


	mib_get_s(VTUO_LINE_DS_MAX_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[0]=tmpShort;
	mib_get_s(VTUO_LINE_DS_TARGET_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[1]=tmpShort;
	mib_get_s(VTUO_LINE_DS_MIN_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[2]=tmpShort;
	/*
	mib_get_s(VTUO_LINE_DS_MAX_SNR_NOLMT, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[3]=tmpUChar;
	*/
	VTUOArrayDump( "SetLineMarginDs", tmp2dsl, 3 );
	d->xdsl_msg_set_array( SetLineMarginDs, tmp2dsl );

	mib_get_s(VTUO_LINE_DS_BITSWAP, (void *)&tmpUChar, sizeof(tmpUChar));
	printf("SetLineBSDs=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineBSDs, (unsigned int)tmpUChar );

	/*
	mib_get_s(VTUO_LINE_DS_MAX_TXPWR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[0]=0;
	tmp2dsl[1]=tmpShort;
	tmp2dsl[2]=0;
	VTUOArrayDump( "SetLinePowerDs", tmp2dsl, 3 );
	d->xdsl_msg_set_array( SetLinePowerDs, tmp2dsl );

	mib_get_s(VTUO_LINE_DS_MIN_OH_RATE, (void *)&tmpUShort, sizeof(tmpUShort));
	printf("SetOHrateDs=%u\n", (unsigned int)tmpUShort);
	d->xdsl_msg_set( SetOHrateDs, (unsigned int)tmpUShort );
	*/


	mib_get_s(VTUO_LINE_US_MAX_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[0]=tmpShort;
	mib_get_s(VTUO_LINE_US_TARGET_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[1]=tmpShort;
	mib_get_s(VTUO_LINE_US_MIN_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[2]=tmpShort;
	/*
	mib_get_s(VTUO_LINE_US_MAX_SNR_NOLMT, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[3]=tmpUChar;
	*/
	VTUOArrayDump( "SetLineMarginUs", tmp2dsl, 3 );
	d->xdsl_msg_set_array( SetLineMarginUs, tmp2dsl );

	mib_get_s(VTUO_LINE_US_BITSWAP, (void *)&tmpUChar, sizeof(tmpUChar));
	printf("SetLineBSUs=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineBSUs, (unsigned int)tmpUChar );
	/*
	mib_get_s(VTUO_LINE_US_MAX_RXPWR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[0]=tmpShort;
	mib_get_s(VTUO_LINE_US_MAX_TXPWR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[1]=tmpShort;
	mib_get_s(VTUO_LINE_US_MAX_RXPWR_NOLMT, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[2]=tmpUChar;
	*/
	VTUOArrayDump( "SetLinePowerUs", tmp2dsl, 3 );
	d->xdsl_msg_set_array( SetLinePowerUs, tmp2dsl );
	/*
	mib_get_s(VTUO_LINE_US_MIN_OH_RATE, (void *)&tmpUShort, sizeof(tmpUShort));
	printf("SetOHrateUs=%u\n", (unsigned int)tmpUShort);
	d->xdsl_msg_set( SetOHrateUs, (unsigned int)tmpUShort );


	mib_get_s(VTUO_LINE_TRANS_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	printf("SetLineTxMode=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineTxMode, (unsigned int)tmpUChar );
	*/

	mib_get_s(VTUO_LINE_ADSL_PROTOCOL, (void *)&tmpUInt, sizeof(tmpUInt));
	mib_get_s(VTUO_LINE_VDSL2_PROFILE, (void *)&tmpUShort, sizeof(tmpUShort));
	if (tmpUShort != 0)
		tmpUInt |= MODE_VDSL2;
	tmpUInt |= MODE_ANX_A;
	printf("SetPmdMode=0x%08x\n", (unsigned int)tmpUInt);
	d->xdsl_msg_set( SetPmdMode, (unsigned int)tmpUInt );

	mib_get_s(VTUO_LINE_VDSL2_PROFILE, (void *)&tmpUShort, sizeof(tmpUShort));
	printf("SetVdslProfile=0x%08x\n", (unsigned int)tmpUShort);
	d->xdsl_msg_set(SetVdslProfile, (unsigned int)tmpUShort);

	/*
	mib_get_s(VTUO_LINE_CLASS_MASK, (void *)&tmpUChar, sizeof(tmpUChar));
	printf("SetLineClassMask=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineClassMask, (unsigned int)tmpUChar );
	*/

	mib_get_s(VTUO_LINE_LIMIT_MASK, (void *)&tmpUChar, sizeof(tmpUChar));
	printf("SetLineLimitMask=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineLimitMask, (unsigned int)tmpUChar );

	mib_get_s(VTUO_LINE_US0_MASK, (void *)&tmpUChar, sizeof(tmpUChar));
	printf("SetLineUs0tMask=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineUs0tMask, (unsigned int)tmpUChar );

	mib_get_s(VTUO_LINE_UPBO_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[0]=tmpUChar;
	mib_get_s(VTUO_LINE_UPBOKL, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[1]=tmpShort;
	mib_get_s(VTUO_LINE_UPBO_1A, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[2]=tmpShort;
	mib_get_s(VTUO_LINE_UPBO_2A, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[3]=tmpShort;
	mib_get_s(VTUO_LINE_UPBO_3A, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[4]=tmpShort;
	mib_get_s(VTUO_LINE_UPBO_4A, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[5]=tmpShort;
	mib_get_s(VTUO_LINE_UPBO_1B, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[6]=tmpShort;
	mib_get_s(VTUO_LINE_UPBO_2B, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[7]=tmpShort;
	mib_get_s(VTUO_LINE_UPBO_3B, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[8]=tmpShort;
	mib_get_s(VTUO_LINE_UPBO_4B, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[9]=tmpShort;
	VTUOArrayDump( "SetLineUPBO", tmp2dsl, 10 );
	d->xdsl_msg_set_array( SetLineUPBO, tmp2dsl );
	/*
	mib_get_s(VTUO_LINE_RT_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	printf("SetLineRTMode=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineRTMode, (unsigned int)tmpUChar );
	*/
	mib_get_s(VTUO_LINE_US0_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	printf("SetLineUS0=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineUS0, (unsigned int)tmpUChar );
	return 0;
}



int VTUOSetupInm(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	int tmp2dsl[5];
	short tmpShort;
	unsigned char tmpUChar;

	mib_get_s(VTUO_INM_NE_INP_EQ_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[0]=tmpUChar;
	mib_get_s(VTUO_INM_NE_INMCC, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[1]=tmpUChar;
	mib_get_s(VTUO_INM_NE_IAT_OFFSET, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[2]=tmpUChar;
	mib_get_s(VTUO_INM_NE_IAT_SETUP, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[3]=tmpUChar;
	/*
	mib_get_s(VTUO_INM_NE_ISDD_SEN, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[4]=tmpShort;
	*/
	VTUOArrayDump( "SetInmNE", tmp2dsl, 4 );
	d->xdsl_msg_set_array( SetInmNE, tmp2dsl );

	mib_get_s(VTUO_INM_FE_INP_EQ_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[0]=tmpUChar;
	mib_get_s(VTUO_INM_FE_INMCC, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[1]=tmpUChar;
	mib_get_s(VTUO_INM_FE_IAT_OFFSET, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[2]=tmpUChar;
	mib_get_s(VTUO_INM_FE_IAT_SETUP, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[3]=tmpUChar;
	/*
	mib_get_s(VTUO_INM_FE_ISDD_SEN, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[4]=tmpShort;
	*/
	VTUOArrayDump( "SetInmFE", tmp2dsl, 4 );
	d->xdsl_msg_set_array( SetInmFE, tmp2dsl );
	return 0;
}


int VTUOSetupSra(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	int tmp2dsl[6];
	unsigned char tmpUChar;
	unsigned short tmpUShort;
	short tmpShort;


	mib_get_s(VTUO_SRA_DS_RA_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[0]=tmpUChar;
	mib_get_s(VTUO_SRA_DS_DYNAMIC_DEPTH, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[1]=tmpUChar;
	mib_get_s(VTUO_SRA_DS_USHIFT_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[2]=tmpShort;
	mib_get_s(VTUO_SRA_DS_DSHIFT_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[3]=tmpShort;
	mib_get_s(VTUO_SRA_DS_USHIFT_TIME, (void *)&tmpUShort, sizeof(tmpUShort));
	tmp2dsl[4]=tmpUShort;
	mib_get_s(VTUO_SRA_DS_DSHIFT_TIME, (void *)&tmpUShort, sizeof(tmpUShort));
	tmp2dsl[5]=tmpUShort;
	VTUOArrayDump( "SetLineRADs", tmp2dsl, 6 );
	d->xdsl_msg_set_array( SetLineRADs, tmp2dsl );

	mib_get_s(VTUO_SRA_DS_RA_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[0]=(tmpUChar==3)?1:0;
	mib_get_s(VTUO_SRA_DS_SOS_TIME, (void *)&tmpUShort, sizeof(tmpUShort));
	tmp2dsl[1]=tmpUShort;
	mib_get_s(VTUO_SRA_DS_SOS_CRC, (void *)&tmpUShort, sizeof(tmpUShort));
	tmp2dsl[2]=tmpUShort;
	mib_get_s(VTUO_SRA_DS_SOS_NTONE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[3]=tmpUChar;
	mib_get_s(VTUO_SRA_DS_SOS_MAX, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[4]=tmpUChar;
	/*
	mib_get_s(VTUO_SRA_DS_SOS_MSTEP_TONE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[5]=tmpUChar;
	*/
	VTUOArrayDump( "SetLineSOSDs", tmp2dsl, 5 );
	d->xdsl_msg_set_array( SetLineSOSDs, tmp2dsl );

	mib_get_s(VTUO_SRA_DS_ROC_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[0]=tmpUChar;
	mib_get_s(VTUO_SRA_DS_ROC_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[1]=tmpShort;
	mib_get_s(VTUO_SRA_DS_ROC_MIN_INP, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[2]=tmpUChar;
	VTUOArrayDump( "SetLineROCDs", tmp2dsl, 3 );
	d->xdsl_msg_set_array( SetLineROCDs, tmp2dsl );



	mib_get_s(VTUO_SRA_US_RA_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[0]=tmpUChar;
	mib_get_s(VTUO_SRA_US_DYNAMIC_DEPTH, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[1]=tmpUChar;
	mib_get_s(VTUO_SRA_US_USHIFT_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[2]=tmpShort;
	mib_get_s(VTUO_SRA_US_DSHIFT_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[3]=tmpShort;
	mib_get_s(VTUO_SRA_US_USHIFT_TIME, (void *)&tmpUShort, sizeof(tmpUShort));
	tmp2dsl[4]=tmpUShort;
	mib_get_s(VTUO_SRA_US_DSHIFT_TIME, (void *)&tmpUShort, sizeof(tmpUShort));
	tmp2dsl[5]=tmpUShort;
	VTUOArrayDump( "SetLineRAUs", tmp2dsl, 6 );
	d->xdsl_msg_set_array( SetLineRAUs, tmp2dsl );

	mib_get_s(VTUO_SRA_US_RA_MODE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[0]=(tmpUChar==3)?1:0;
	mib_get_s(VTUO_SRA_US_SOS_TIME, (void *)&tmpUShort, sizeof(tmpUShort));
	tmp2dsl[1]=tmpUShort;
	mib_get_s(VTUO_SRA_US_SOS_CRC, (void *)&tmpUShort, sizeof(tmpUShort));
	tmp2dsl[2]=tmpUShort;
	mib_get_s(VTUO_SRA_US_SOS_NTONE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[3]=tmpUChar;
	mib_get_s(VTUO_SRA_US_SOS_MAX, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[4]=tmpUChar;
	/*
	mib_get_s(VTUO_SRA_US_SOS_MSTEP_TONE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[5]=tmpUChar;
	*/
	VTUOArrayDump( "SetLineSOSUs", tmp2dsl, 5 );
	d->xdsl_msg_set_array( SetLineSOSUs, tmp2dsl );

	mib_get_s(VTUO_SRA_US_ROC_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[0]=tmpUChar;
	mib_get_s(VTUO_SRA_US_ROC_SNR, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[1]=tmpShort;
	mib_get_s(VTUO_SRA_US_ROC_MIN_INP, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[2]=tmpUChar;
	VTUOArrayDump( "SetLineROCUs", tmp2dsl, 3 );
	d->xdsl_msg_set_array( SetLineROCUs, tmp2dsl );

	return 0;
}

int VTUOSetupDpbo(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	int tmp2dsl[8];
	unsigned char tmpUChar;
	unsigned short tmpUShort;
	short tmpShort;
	int tmpInt;

	mib_get_s(VTUO_DPBO_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	tmp2dsl[0]=tmpUChar;
	mib_get_s(VTUO_DPBO_ESEL, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[1]=tmpShort;
	mib_get_s(VTUO_DPBO_ESCMA, (void *)&tmpInt, sizeof(tmpInt));
	tmp2dsl[2]=tmpInt;
	mib_get_s(VTUO_DPBO_ESCMB, (void *)&tmpInt, sizeof(tmpInt));
	tmp2dsl[3]=tmpInt;
	mib_get_s(VTUO_DPBO_ESCMC, (void *)&tmpInt, sizeof(tmpInt));
	tmp2dsl[4]=tmpInt;
	mib_get_s(VTUO_DPBO_MUS, (void *)&tmpShort, sizeof(tmpShort));
	tmp2dsl[5]=tmpShort;
	mib_get_s(VTUO_DPBO_FMIN, (void *)&tmpUShort, sizeof(tmpUShort));
	tmp2dsl[6]=tmpUShort;
	mib_get_s(VTUO_DPBO_FMAX, (void *)&tmpUShort, sizeof(tmpUShort));
	tmp2dsl[7]=tmpUShort;
	VTUOArrayDump( "SetLineDPBO", tmp2dsl, 8 );
	d->xdsl_msg_set_array( SetLineDPBO, tmp2dsl );

{
	MIB_CE_VTUO_DPBO_T entry, *p=&entry;
	int num, i;
	int *ptmp2dsl, int_size;

	num=mib_chain_total( MIB_VTUO_DPBO_TBL );
	int_size= 2+num*2;
	ptmp2dsl=malloc( int_size*sizeof(int) );
	if(!ptmp2dsl) return -1;
	memset( ptmp2dsl, 0, int_size*sizeof(int) );

	ptmp2dsl[0]= num?1:0;
	ptmp2dsl[1]= num;
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_DPBO_TBL, i, p))
			continue;
		ptmp2dsl[i*2+2]= p->ToneId;
		ptmp2dsl[i*2+3]= p->PsdLevel;
	}
	VTUOArrayDump( "SetLineDPBOPSD", ptmp2dsl, int_size );
	d->xdsl_msg_set_array( SetLineDPBOPSD, ptmp2dsl );
	free(ptmp2dsl);
}

	return 0;
}

int VTUOSetupPsd(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	MIB_CE_VTUO_PSD_T entry, *p=&entry;
	int num, i;
	int *ptmp2dsl, int_size;

	/*DS*/
	num=mib_chain_total( MIB_VTUO_PSD_DS_TBL );
	int_size= 2+num*2;
	ptmp2dsl=malloc( int_size*sizeof(int) );
	if(!ptmp2dsl) return -1;
	memset( ptmp2dsl, 0, int_size*sizeof(int) );

	ptmp2dsl[0]= num?1:0;
	ptmp2dsl[1]= num;
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_PSD_DS_TBL, i, p))
			continue;
		ptmp2dsl[i*2+2]= p->ToneId;
		ptmp2dsl[i*2+3]= p->PsdLevel;
	}
	VTUOArrayDump( "SetLinePSDMIBDs", ptmp2dsl, int_size );
	d->xdsl_msg_set_array( SetLinePSDMIBDs, ptmp2dsl );
	free(ptmp2dsl);

	/*US*/
	num=mib_chain_total( MIB_VTUO_PSD_US_TBL );
	int_size= 2+num*2;
	ptmp2dsl=malloc( int_size*sizeof(int) );
	if(!ptmp2dsl) return -1;
	memset( ptmp2dsl, 0, int_size*sizeof(int) );

	ptmp2dsl[0]= num?1:0;
	ptmp2dsl[1]= num;
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_PSD_US_TBL, i, p))
			continue;
		ptmp2dsl[i*2+2]= p->ToneId;
		ptmp2dsl[i*2+3]= p->PsdLevel;
	}
	VTUOArrayDump( "SetLinePSDMIBUs", ptmp2dsl, int_size );
	d->xdsl_msg_set_array( SetLinePSDMIBUs, ptmp2dsl );
	free(ptmp2dsl);

	return 0;
}
/*
int VTUOSetupVn(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	MIB_CE_VTUO_VN_T entry, *p=&entry;
	int num, i;
	int *ptmp2dsl, int_size;
	unsigned char tmpUChar;

	// DS
	num=mib_chain_total( MIB_VTUO_VN_DS_TBL );
	int_size= 2+num*2;
	ptmp2dsl=malloc( int_size*sizeof(int) );
	if(!ptmp2dsl) return -1;
	memset( ptmp2dsl, 0, int_size*sizeof(int) );

	mib_get_s(VTUO_VN_DS_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	ptmp2dsl[0]= tmpUChar?1:0;
	ptmp2dsl[1]= num;
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_VN_DS_TBL, i, p))
			continue;
		ptmp2dsl[i*2+2]= p->ToneId;
		ptmp2dsl[i*2+3]= p->NoiseLevel;
	}
	VTUOArrayDump( "SetLineVNDs", ptmp2dsl, int_size );
	d->xdsl_msg_set_array( SetLineVNDs, ptmp2dsl );
	free(ptmp2dsl);

	// US
	num=mib_chain_total( MIB_VTUO_VN_US_TBL );
	int_size= 2+num*2;
	ptmp2dsl=malloc( int_size*sizeof(int) );
	if(!ptmp2dsl) return -1;
	memset( ptmp2dsl, 0, int_size*sizeof(int) );

	mib_get_s(VTUO_VN_US_ENABLE, (void *)&tmpUChar, sizeof(tmpUChar));
	ptmp2dsl[0]= tmpUChar?1:0;
	ptmp2dsl[1]= num;
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_VN_US_TBL, i, p))
			continue;
		ptmp2dsl[i*2+2]= p->ToneId;
		ptmp2dsl[i*2+3]= p->NoiseLevel;
	}
	VTUOArrayDump( "SetLineVNUs", ptmp2dsl, int_size );
	d->xdsl_msg_set_array( SetLineVNUs, ptmp2dsl );
	free(ptmp2dsl);

	return 0;
}

int VTUOSetupRfi(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	MIB_CE_VTUO_RFI_T entry, *p=&entry;
	int num, i;
	int *ptmp2dsl, int_size;

	num=mib_chain_total( MIB_VTUO_RFI_TBL );
	int_size= 2+num*2;
	ptmp2dsl=malloc( int_size*sizeof(int) );
	if(!ptmp2dsl) return -1;
	memset( ptmp2dsl, 0, int_size*sizeof(int) );

	ptmp2dsl[0]= num?1:0;
	ptmp2dsl[1]= num;
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_RFI_TBL, i, p))
			continue;
		ptmp2dsl[i*2+2]= p->ToneId;
		ptmp2dsl[i*2+3]= p->ToneIdEnd;
	}
	VTUOArrayDump( "SetLineRFI", ptmp2dsl, int_size );
	d->xdsl_msg_set_array( SetLineRFI, ptmp2dsl );
	free(ptmp2dsl);

	return 0;
}
*/
int VTUOCheck(void)
{
	int val=1;
	XDSL_OP *d=xdsl_get_op(0);

	d->xdsl_msg_get(GetDeviceType,&val);
	val=(val==0)?1:0; //0:VTUO, 1:VTUR
	//printf( "%s: return %d\n", __FUNCTION__, val );
	return val;
}

int VTUOSetup(void)
{
	if( VTUOCheck() )
	{
		VTUOSetupSra();
		VTUOSetupPsd();
		/*
		VTUOSetupVn();
		*/
		VTUOSetupDpbo();
		/*
		VTUOSetupRfi();
		*/
		VTUOSetupLine();

		VTUOSetupGinp();
		VTUOSetupChan();

		VTUOSetupInm();
	}
	return 0;
}
#endif /*CONFIG_DSL_VTUO*/

#if 0
#define EOC_VENDORID_SIZE	8
#define EOC_VERSIONNUM_SIZE	16
static unsigned char vendorId[EOC_VENDORID_SIZE]={0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x0};		// adslctrl debug 1417
static unsigned char versionNum[EOC_VERSIONNUM_SIZE]={0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x0};	// adslctrl debug 1418
#endif
int setupDsl(void)
{
	char tone[64];
	unsigned char init_line, olr;
	unsigned char dsl_anxb;
	unsigned short dsl_mode;
#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
	unsigned short gfast_profile=0;
#endif

	int val;
	int ret=1;
	int tmp2dsl[10];

	// check INIT_LINE
	if (mib_get_s(MIB_INIT_LINE, (void *)&init_line, sizeof(init_line)) != 0)
	{
		if (init_line == 1)
		{
#ifdef CONFIG_VDSL
			unsigned int mode;
			unsigned short profile;
			unsigned char gvector;
			char serial_no[64]={0};
			int len_sn;

			//disable modem,20130203
			printf("xDSL disable modem\n");
			adsl_drv_get(RLCM_PHY_DISABLE_MODEM, NULL, 0);
			// set DSL EoC serial number
			mib_get_s( MIB_HW_SERIAL_NUMBER,  (void *)serial_no, sizeof(serial_no));
			len_sn = strlen(serial_no);
			// clean up the rest of content behind
			memset(serial_no+len_sn, 0, 32-len_sn);
			adsl_drv_get(RLCM_SET_ATUR_SERIALNUM, (void *)&serial_no[0], 32);
			
			// Set DSL EoC vendorID			
			//adsl_drv_get(RLCM_SET_ATUR_VENDORID, (void *)&vendorId, EOC_VENDORID_SIZE);
			
			// Set DSL EoC version number			
			//adsl_drv_get(RLCM_SET_ATUR_VERSIONNUM, (void *)&versionNum, EOC_VERSIONNUM_SIZE);

#ifdef CONFIG_DSL_VTUO
			VTUOSetup();
#else
			//Pmd mode
			mode=0;
			mib_get_s( MIB_DSL_ANNEX_B, (void *)&dsl_anxb, sizeof(dsl_anxb));
			mib_get_s(MIB_ADSL_MODE, (void *)&dsl_mode, sizeof(dsl_mode));
			if(dsl_mode&ADSL_MODE_GDMT)		mode |= MODE_GDMT;
			if(dsl_mode&ADSL_MODE_GLITE)	mode |= MODE_GLITE;
			if(dsl_mode&ADSL_MODE_ADSL2)	mode |= MODE_ADSL2;
			if(dsl_mode&ADSL_MODE_ADSL2P)	mode |= MODE_ADSL2PLUS;
			if(dsl_mode&ADSL_MODE_VDSL2)	mode |= MODE_VDSL2;
#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
			if(dsl_mode&ADSL_MODE_GFAST)	mode |= MODE_GFAST;
#endif

			//if(dsl_mode&ADSL_MODE_ANXB)
			if (dsl_anxb)
			{
				// Annex B
				mode |= MODE_ANX_B;
				if(dsl_mode&ADSL_MODE_T1413)	mode |= MODE_ETSI;
				if(dsl_mode&ADSL_MODE_ANXJ)	mode |= MODE_ANX_J;
			}else{
				// Annex A
				mode |= MODE_ANX_A;
				if(dsl_mode&ADSL_MODE_T1413)	mode |= MODE_ANSI;
				if(dsl_mode&ADSL_MODE_ANXL)		mode |= MODE_ANX_L;
				if(dsl_mode&ADSL_MODE_ANXM)		mode |= MODE_ANX_M;
				if(dsl_mode&ADSL_MODE_ANXI)	mode |= MODE_ANX_I;
			}

			//printf("SetPmdMode=0x%08x (dsl_mode=0x%04x)\n",  mode, dsl_mode);
			dsl_msg_set(SetPmdMode, mode);

			//vdsl2 profile
			mib_get_s(MIB_VDSL2_PROFILE, (void *)&profile, sizeof(profile));
			//printf("SetVdslProfile=0x%08x\n", (unsigned int)profile);
			dsl_msg_set(SetVdslProfile, (unsigned int)profile);

#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
			mib_get_s( MIB_GFAST_PROFILE, (void *)&gfast_profile, sizeof(gfast_profile));
			//printf("SetGfastSupportProfile=0x%08x\n", (unsigned int)gfast_profile);
			dsl_msg_set(SetGfastSupportProfile, (unsigned int)gfast_profile);
#endif

			// G.INP,	 default by dsplib
#ifdef ENABLE_ADSL_MODE_GINP
			val=0;
			if (dsl_mode & ADSL_MODE_GINP)	// G.INP
				val=3;
			//printf("SetGInp=0x%08x\n", val);
			dsl_msg_set(SetGInp, val);
#endif /*ENABLE_ADSL_MODE_GINP*/

			// G.Vector
			mib_get_s(MIB_DSL_G_VECTOR, (void *)&gvector, sizeof(gvector));
			val = (int)gvector;
			//printf("SetGVector=0x%08x\n", val);
			dsl_msg_set(SetGVector, val);
			// set OLR Type
			mib_get_s(MIB_ADSL_OLR, (void *)&olr, sizeof(olr));
			val = (int)olr;
			// SRA (should include bitswap)
			if(val == 2) val = 3;
			//printf("SetOlr=0x%08x (olr=0x%02x)\n", val, olr);
			dsl_msg_set(SetOlr, val);

#endif /*CONFIG_DSL_VTUO*/

			// Start handshaking; should be the last command
			mode=0;
			adsl_drv_get(RLCM_SET_ADSL_MODE, (void *)&mode, 4);

			if (WAN_MODE & MODE_BOND){
				//set bonding master
				tmp2dsl[0] = 1; //enable
				tmp2dsl[1] = 0; //0:master, 1:slave, 2:slave2, 3:slave3
				tmp2dsl[2] = 4; //TOTAL_SLAVE_NUM
				tmp2dsl[3] = 1; //bit0: PTM, bit1: TDIM, bit2: ATM
				dsl_msg_set_array(SetDslBond, tmp2dsl);
			}

			//enable modem,20130203
			printf("xDSL enable modem\n");
			adsl_drv_get(RLCM_PHY_ENABLE_MODEM, NULL, 0);

			ret = 1;
#else /*CONFIG_VDSL*/
			char mode[3], inp;
			int xmode,adslmode, axB, axM, axL;

			// start adsl
			mib_get_s(MIB_ADSL_MODE, (void *)&dsl_mode, sizeof(dsl_mode));
			mib_get_s( MIB_DSL_ANNEX_B, (void *)&dsl_anxb, sizeof(dsl_anxb));

			adslmode=(int)(dsl_mode & (ADSL_MODE_GLITE|ADSL_MODE_T1413|ADSL_MODE_GDMT));	// T1.413 & G.dmt
			#if 0
			adsl_drv_get(RLCM_PHY_START_MODEM, (void *)&adslmode, 4);
			#endif

			//if (dsl_mode & ADSL_MODE_ANXB) {	// Annex B
			if (dsl_anxb) {
				axB = 1;
				axL = 0;
				axM = 0;
			}
			else {	// Annex A
				axB = 0;
				if (dsl_mode & ADSL_MODE_ANXL)	// Annex L
					axL = 3; // Wide-Band & Narrow-Band Mode
				else
					axL = 0;
				if (dsl_mode & ADSL_MODE_ANXM)	// Annex M
					axM = 1;
				else
					axM = 0;
			}

			adsl_drv_get(RLCM_SET_ANNEX_B, (void *)&axB, 4);
			adsl_drv_get(RLCM_SET_ANNEX_L, (void *)&axL, 4);
			adsl_drv_get(RLCM_SET_ANNEX_M, (void *)&axM, 4);

#ifdef ENABLE_ADSL_MODE_GINP
			if (dsl_mode & ADSL_MODE_GINP)	// G.INP
				xmode = DSL_FUNC_GINP;
			else
				xmode = 0;
			adsl_drv_get(RLCM_SET_DSL_FUNC, (void *)&xmode, 4);
#endif

			xmode=0;
			if (dsl_mode & (ADSL_MODE_GLITE|ADSL_MODE_T1413|ADSL_MODE_GDMT))
				xmode |= 1;	// ADSL1
			if (dsl_mode & ADSL_MODE_ADSL2)
				xmode |= 2;	// ADSL2
			if (dsl_mode & ADSL_MODE_ADSL2P)
				xmode |= 4;	// ADSL2+
			adsl_drv_get(RLCM_SET_XDSL_MODE, (void *)&xmode, 4);

			// set OLR Type
			mib_get_s(MIB_ADSL_OLR, (void *)&olr, sizeof(olr));

			val = (int)olr;
			if (val == 2) // SRA (should include bitswap)
				val = 3;

			adsl_drv_get(RLCM_SET_OLR_TYPE, (void *)&val, 4);

			// set Tone mask
			mib_get_s(MIB_ADSL_TONE, (void *)tone, sizeof(tone));
			adsl_drv_get(RLCM_LOADCARRIERMASK, (void *)tone, GET_LOADCARRIERMASK_SIZE);

			mib_get_s(MIB_ADSL_HIGH_INP, (void *)&inp, sizeof(inp));
			xmode = inp;
			adsl_drv_get(RLCM_SET_HIGH_INP, (void *)&xmode, 4);

			// new_hibrid
			mib_get_s(MIB_DEVICE_TYPE, (void *)&inp, sizeof(inp));
			xmode = inp;
			switch(xmode) {
				case 1:
					xmode = 1052;
					break;
				case 2:
					xmode = 2099;
					break;
				case 3:
					xmode = 3099;
					break;
				case 4:
					xmode = 4052;
					break;
				case 5:
					xmode = 5099;
					break;
				case 9:
					xmode = 9099;
					break;
				default:
					xmode = 9099;
			}
			adsl_drv_get(RLCM_SET_HYBRID, (void *)&xmode, 4);
			// Start handshaking; should be the last command.
			adsl_drv_get(RLCM_SET_ADSL_MODE, (void *)&adslmode, 4);
			ret = 1;
#endif /*CONFIG_VDSL*/
		}
		else
			ret = 0;
	}
	else
		ret = -1;

#ifdef FIELD_TRY_SAFE_MODE
	unsigned char mode;
	SafeModeData vSmd;

	memset((void *)&vSmd, 0, sizeof(vSmd));
	mib_get_s(MIB_ADSL_FIELDTRYSAFEMODE, (void *)&mode, sizeof(mode));
	vSmd.FieldTrySafeMode = (int)mode;
	mib_get_s(MIB_ADSL_FIELDTRYTESTPSDTIMES, (void *)&vSmd.FieldTryTestPSDTimes, sizeof(vSmd.FieldTryTestPSDTimes));
	mib_get_s(MIB_ADSL_FIELDTRYCTRLIN, (void *)&vSmd.FieldTryCtrlIn, sizeof(vSmd.FieldTryCtrlIn));
	adsl_drv_get(RLCM_SET_SAFEMODE_CTRL, (void *)&vSmd, SAFEMODE_DATA_SIZE);
#endif

	return ret;
}
#endif // of CONFIG_DEV_xDSL

#if defined(CONFIG_USER_XDSL_SLAVE)
int setupSlvDsl(void)
{
	char tone[64];
	unsigned char init_line, olr;
	unsigned char dsl_anxb;
	unsigned short dsl_mode;
	int val;
	int ret=1;
	int tmp2dsl[10];

	// check INIT_LINE
	if (mib_get_s(MIB_INIT_LINE, (void *)&init_line, sizeof(init_line)) != 0)
	{
		if (init_line == 1)
		{
			unsigned int mode;
			unsigned short profile;
			unsigned char gvector;

			//disable modem,20130203
			printf("xDSL disable modem\n");
			adsl_slv_drv_get(RLCM_PHY_DISABLE_MODEM, NULL, 0);

			//Pmd mode
			mode=0;
			mib_get_s( MIB_DSL_ANNEX_B, (void *)&dsl_anxb, sizeof(dsl_anxb));
			mib_get_s(MIB_ADSL_MODE, (void *)&dsl_mode, sizeof(dsl_mode));
			if(dsl_mode&ADSL_MODE_GDMT)		mode |= MODE_GDMT;
			if(dsl_mode&ADSL_MODE_GLITE)	mode |= MODE_GLITE;
			if(dsl_mode&ADSL_MODE_ADSL2)	mode |= MODE_ADSL2;
			if(dsl_mode&ADSL_MODE_ADSL2P)	mode |= MODE_ADSL2PLUS;
			if(dsl_mode&ADSL_MODE_VDSL2)	mode |= MODE_VDSL2;

			//if(dsl_mode&ADSL_MODE_ANXB)
			if (dsl_anxb)
			{
				// Annex B
				mode |= MODE_ANX_B;
				if(dsl_mode&ADSL_MODE_T1413)	mode |= MODE_ETSI;
				if(dsl_mode&ADSL_MODE_ANXJ)	mode |= MODE_ANX_J;
			}else{
				// Annex A
				mode |= MODE_ANX_A;
				if(dsl_mode&ADSL_MODE_T1413)	mode |= MODE_ANSI;
				if(dsl_mode&ADSL_MODE_ANXL)		mode |= MODE_ANX_L;
				if(dsl_mode&ADSL_MODE_ANXM)		mode |= MODE_ANX_M;
				if(dsl_mode&ADSL_MODE_ANXI)	mode |= MODE_ANX_I;
			}
			printf("SetPmdMode=0x%08x (dsl_mode=0x%04x)\n",  mode, dsl_mode);
			dsl_slv_msg_set(SetPmdMode, mode);

			//vdsl2 profile
			mib_get_s(MIB_VDSL2_PROFILE, (void *)&profile, sizeof(profile));
			printf("SetVdslProfile=0x%08x\n", (unsigned int)profile);
			dsl_slv_msg_set(SetVdslProfile, (unsigned int)profile);

			// G.INP,	 default by dsplib
#ifdef ENABLE_ADSL_MODE_GINP
			val=0;
			if (dsl_mode & ADSL_MODE_GINP)	// G.INP
				val=3;
			printf("SetGInp=0x%08x\n", val);
			dsl_slv_msg_set(SetGInp, val);
#endif /*ENABLE_ADSL_MODE_GINP*/
			// G.Vector
			mib_get_s(MIB_DSL_G_VECTOR, (void *)&gvector, sizeof(gvector));
			val = (int)gvector;
			dsl_slv_msg_set(SetGVector, val);
			printf("SetGVector=0x%08x\n", val);
			// set OLR Type
			mib_get_s(MIB_ADSL_OLR, (void *)&olr, sizeof(olr));
			val = (int)olr;
			// SRA (should include bitswap)
			if(val == 2) val = 3;
			printf("SetOlr=0x%08x (olr=0x%02x)\n", val, olr);
			dsl_slv_msg_set(SetOlr, val);

			// Start handshaking; should be the last command
			mode=0;
			adsl_slv_drv_get(RLCM_SET_ADSL_MODE, (void *)&mode, 4);

			if (WAN_MODE & MODE_BOND){
				//set bonding slave
				tmp2dsl[0] = 1; //enable
				tmp2dsl[1] = 1; //0:master, 1:slave, 2:slave2, 3:slave3
				tmp2dsl[2] = 4; //TOTAL_SLAVE_NUM
				tmp2dsl[3] = 1; //bit0: PTM, bit1: TDIM, bit2: ATM
				dsl_slv_msg_set_array(SetDslBond, tmp2dsl);
			}

			//enable modem,20130203
			printf("xDSL enable modem\n");
			adsl_slv_drv_get(RLCM_PHY_ENABLE_MODEM, NULL, 0);

			ret = 1;

		}else 	
			ret = 0;
	} else
		ret = -1;
}
#endif

#ifdef CONFIG_DEV_xDSL
#define MAXWAIT		5
static int finished = 0;
int testOAMLoopback(MIB_CE_ATM_VC_Tp pEntry, unsigned char scope, unsigned char type)
{
#ifdef EMBED
#ifdef CONFIG_ATM_REALTEK
	int skfd;
	struct atmif_sioc mysio;
	ATMOAMLBReq lbReq;
	ATMOAMLBState lbState;
	time_t  curTime, preTime = 0;

	if((skfd = socket(PF_ATMPVC, SOCK_DGRAM, 0)) < 0){
		perror("socket open error");
		return -1;
	}

	memset(&lbReq, 0, sizeof(ATMOAMLBReq));
	memset(&lbState, 0, sizeof(ATMOAMLBState));
	lbReq.Scope = scope;
	lbReq.vpi = pEntry->vpi;
	if (type == 5)
		lbReq.vci = pEntry->vci;
	else if (type == 4) {
		if (scope == 0)	// Segment
			lbReq.vci = 3;
		else if (scope == 1)	// End-to-end
			lbReq.vci = 4;
	}
	memset(lbReq.LocID, 0xff, 16);	// Loopback Location ID
	mysio.number = 0;	// ATM interface number
	mysio.arg = (void *)&lbReq;
	// Start the loopback test
	if (ioctl(skfd, ATM_OAM_LB_START, &mysio)<0) {
		perror("ioctl: ATM_OAM_LB_START failed !");
		close(skfd);
		return -1;
	}

	finished = 0;
	time(&preTime);
	// Query the loopback status
	mysio.arg = (void *)&lbState;
	lbState.vpi = pEntry->vpi;
	if (type == 5)
		lbState.vci = pEntry->vci;
	else if (type == 4) {
		if (scope == 0)	// Segment
			lbState.vci = 3;
		else if (scope == 1)	// End-to-end
			lbState.vci = 4;
	}
	lbState.Tag = lbReq.Tag;

	while (1)
	{
		time(&curTime);
		if (curTime - preTime >= MAXWAIT)
		{
			//printf("OAMLB timeout!\n");
			finished = 1;
			break;	// break for timeout
		}

		if (ioctl(skfd, ATM_OAM_LB_STATUS, &mysio)<0) {
			perror("ioctl: ATM_OAM_LB_STATUS failed !");
			mysio.arg = (void *)&lbReq;
			ioctl(skfd, ATM_OAM_LB_STOP, &mysio);
			close(skfd);
			return -1;
		}

		if (lbState.count[0] > 0)
		{
			break;	// break for loopback success
		}
	}

	mysio.arg = (void *)&lbReq;
	// Stop the loopback test
	if (ioctl(skfd, ATM_OAM_LB_STOP, &mysio)<0) {
		perror("ioctl: ATM_OAM_LB_STOP failed !");
		close(skfd);
		return -1;
	}
	close(skfd);

	if (!finished) {
		printf("\n--- Loopback cell received successfully ---\n");
		return 1;	// successful
	}
	else {
		printf("\n--- Loopback failed ---\n");
		return 0;	// failed
	}
#endif // of CONFIG_ATM_REALTEK
#else
	return 1;
#endif
}
#endif	//CONFIG_DEV_xDSL


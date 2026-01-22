/*
 *      Commserv C API header, copy from commserv2/include/xdsl_internal.h
 *      Authors: WenJuin Lin	<wenjuin.lin@realtek.com>
 *
 */

#ifndef _C_API_H
#define _C_API_H

#ifdef CONFIG_USER_CMD_SERVER_SIDE
#include "rtk_xdsl_api.h"

enum {
	CMD_ADD = 0,
	CMD_DEL = 1
};

#define MAX_ENDPOINTS 4
struct endpoint_info {
	unsigned char partner_addr[6];
	unsigned int key;
	unsigned int hello_tx_count;
	unsigned int hello_rx_count;
	unsigned int api_tx_count;
	unsigned int api_rx_count;
	unsigned int unknown_drop;
};

struct controller_states {
	int linkstate;
	unsigned int total_tx;
	unsigned int total_rx;
	struct endpoint_info endpts_info[MAX_ENDPOINTS];
};
#endif
#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
enum {
	IDLE_L3=0,
	ACTIVATING=2,
	ITU_HANDSHAKING=3,//3 
	INITIALIZING=5,
	SHOWTIME_L0=6
};

/* reference G.fast data pump api doc  */
enum{
	//State
	//UserGetCurrentState=500, //(unsigned int) *1
	UserGetLineState = 502, //int, unsigned int
	UserGetGfastSupportProfile = 503,//(unsigned int)*1
	UserGetGfastSelectProfile = 504,//(unsigned int)*1
	//PMD
	UserGetPMDParam = 520,//(unsigned int)*6
	//PMSTC
	//UserGetPMSTCParam = 540,//TBD
	//Showtime
	UserGetOlrType = 560,//(unsigned int)*1
	UserGetErrorCount = 561,//{us Lp0, us Lp1,ds Lp0,ds Lp1}(unsigned int)*4
	UserGetSNRMargin=562, //{us,ds} int*2, us,ds
	UserGetSNR=563,//{size x, snr of tone 0,..tone8, tone 16,...,tone (x-1)*8}(int)*(4096+1), max is MAX_NUM_TONES=4096
	UserGetTrellisMode = 564,//{us,ds} (unsigned int)*2
	UserGetNEBitAlloc = 565,//(int)*(4096+1), max is MAX_NUM_TONES=4096
	UserGetFEBitAlloc = 566,        //(int)*(4096+1), max is MAX_NUM_TONES=4096
	UserGetLinkSpeed=567, //unsigned int *2 , us,ds
	//Other
	UserGetLinkPowerState=600,
	UserGetDSPversion = 601,//char, size8
	UserGetDSPBuild = 602,//char, size80
	GetTest=999
};

enum {
	SetPortPtr=500,
	SetGFastProfile=501,
	//intVal[0:1]={1 0}=106a
	//intVal[0:1]={2 0}=212a
	//intVal[0:1]={1 1}=141a
	SetVdslBandOff=502,
	//intVal[0]=0, G.FAST uses all VDSL band
	//intVal[0]=1, 17a
	//intVal[0]=2, 30a
	SetVdslFextBandTx=503, //{Tx0_ToneStart, Tx0_ToneEnd, ...... Tx4_ToneStart, Tx4_ToneEnd}; G.FAST TX does not use these 5 bands
	SetVdslFextBandRx=504, //{Rx0_ToneStart, Rx0_ToneEnd, ...... Rx4_ToneStart, Rx4_ToneEnd}; G.FAST RX does not use these 5 bands
	SetTddTf=505,
	SetTddMds=506, //Tdd=36, Mds=28; Mus=36-28-1=7
	SetLowestCarrier=507,
	SetHighestCarrier=508,
	SetTddRMCOffset=509, //Tdd=36, Mds=28; Mus=36-28-1=7
	
	//new
	//State
	SetGfastSupportProfile = 603,//size 4 bytes
	//PMD
	SetPMDParam = 620,
	//PMSTC
	SetPMSTCParam = 640,
	//Showtime
	SetSNRMargin=662, //int*2, us,ds
	SetTrellis = 664,
	//Other
	SetDSPversion = 601,
	SetPortAmin=602,
	
	SetTest=1000
};

#define GFAST_GET_BEGIN	UserGetLineState	// first Get G.fast Enum
#define GFAST_SET_BEGIN	SetPortPtr			// first Set G.fast Enum
#endif /* CONFIG_USER_CMD_SUPPORT_GFAST */

#endif /* _C_API_H */

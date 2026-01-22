#ifndef _CLUSTER_COMMON_H
#define _CLUSTER_COMMON_H
#include <stdio.h>
#include "options.h"
#include "mib.h"

#define CLUSTER_MANAGE_PROTOCOL 0xDCBA
#define CLUSTER_PKT_MAX_LEN 	1500	//FIXME: should set same as LAN MTU, to prevent sendto() failed with EMSGSIZE?

//#define CLU_MANEGE_DEBUG
#ifdef CLU_MANEGE_DEBUG
#define debug_print(f,args...) \
do{\
    printf("[%s %d] ",__func__,__LINE__);\
    printf(f,##args);\
}while(0)
#else
#define debug_print(f,args...)
#endif


extern int wlan_idx;	// interface index

typedef struct cluster_header_s
{
    unsigned short checksum;
    unsigned short totlen;      //total length include header and opt payload.
    unsigned short id;          //index of the configration packets.
    unsigned short sequence;    //if configration changed, sequence changed.
}CLU_HDR_T, *CLU_HDR_Tp;

enum CLUSTER_TLV_TYPE
{
    WLAN0_CONF_START = 0,       //start of WLAN0 conf, also start of ALL conf... do not edit.
    WLAN0_ENCRYPT,              //encrypt
    WLAN0_ENABLE1X,             //enable1X
    WLAN0_WEP,                  //wep
    WLAN0_WPAAUTH,              //wpaAuth
    WLAN0_WPAPSKFORMAT,         //wpaPSKFormat
    WLAN0_WPAPSK,               //wpaPSK[MAX_PSK_LEN+1]   
    WLAN0_WPAGROUPREKEYTIME,    //wpaGroupRekeyTime
    WLAN0_DISABLED,             //wlanDisabled
    WLAN0_SSID,                 //ssid[MAX_SSID_LEN]
    WLAN0_WLANMODE,             //wlanMode   
    WLAN0_AUTHTYPE,             //authType
    WLAN0_UNICASTCIPHER,        //unicastCipher
    WLAN0_WPA2UNICASTCIPHER,    //wpa2UnicastCipher
    WLAN0_BCNADVTISE,           //bcnAdvtisement
    WLAN0_HIDESSID,             //hidessid
    WLAN0_USERISOLATION,        //userisolation
    WLAN0_WAPIPSK,              //wapiPsk
    WLAN0_WAPIPSKLEN,           //wapiPskLen
    WLAN0_WAPIAUTH,             //wapiAuth
    WLAN0_WAPIPSKFORMAT,        //wapiPskFormat
    WLAN0_WAPIASIPADDR,         //wapiAsIpAddr
    WLAN0_WEPKEYTYPE,           //wepKeyType
    WLAN0_WEPDEFAULTKEY,        //wepDefaultKey
    WLAN0_WEP64KEY1,            //wep64Key1[WEP64_KEY_LEN]
    WLAN0_WEP64KEY2,            //wep64Key2[WEP64_KEY_LEN]
    WLAN0_WEP64KEY3,            //wep64Key3[WEP64_KEY_LEN]
    WLAN0_WEP64KEY4,            //wep64Key4[WEP64_KEY_LEN]
    WLAN0_WEP128KEY1,           //wep128Key1[WEP128_KEY_LEN]
    WLAN0_WEP128KEY2,           //wep128Key2[WEP128_KEY_LEN]
    WLAN0_WEP128KEY3,           //wep128Key3[WEP128_KEY_LEN]
    WLAN0_WEP128KEY4,           //wep128Key4[WEP128_KEY_LEN]
    WLAN0_WMMENABLE,            //wmmEnabled
    WLAN0_RATEADAPTENABLE,      //rateAdaptiveEnabled
    WLAN0_WLANBAND,             //wlanBand
    WLAN0_FIXEDTARATE,          //fixedTxRate
    WLAN0_FT_ENABLE,            //ft_enable
    WLAN0_FT_MDID,              //ft_mdid
    WLAN0_FT_OVER_DS,           //ft_over_ds
    WLAN0_FT_RES_REQ,           //ft_res_request
    WLAN0_FT_R0KEY_TO,          //ft_r0key_timeout
    WLAN0_FT_REASOC_TO,         //ft_reasoc_timeout
    WLAN0_FT_R0KH_ID,           //ft_r0kh_id
    WLAN0_FT_PUSH,              //ft_push
    WLAN0_FT_KH_NUM,            //ft_kh_num
    WLAN0_CONF_END,
    /* WLAN0-root configration end, you can add other conf for wlan0 in the position of WLAN0_CONF_START -> WLAN0_CONF_END */
    WLAN0_VAP1_CONF_START,
    WLAN0_VAP1_ENCRYPT,              //encrypt
    WLAN0_VAP1_ENABLE1X,             //enable1X
    WLAN0_VAP1_WEP,                  //wep
    WLAN0_VAP1_WPAAUTH,              //wpaAuth
    WLAN0_VAP1_WPAPSKFORMAT,         //wpaPSKFormat
    WLAN0_VAP1_WPAPSK,               //wpaPSK[MAX_PSK_LEN+1]   
    WLAN0_VAP1_WPAGROUPREKEYTIME,    //wpaGroupRekeyTime
    WLAN0_VAP1_DISABLED,             //wlanDisabled
    WLAN0_VAP1_SSID,                 //ssid[MAX_SSID_LEN]
    WLAN0_VAP1_WLANMODE,             //wlanMode   
    WLAN0_VAP1_AUTHTYPE,             //authType
    WLAN0_VAP1_UNICASTCIPHER,        //unicastCipher
    WLAN0_VAP1_WPA2UNICASTCIPHER,    //wpa2UnicastCipher
    WLAN0_VAP1_BCNADVTISE,           //bcnAdvtisement
    WLAN0_VAP1_HIDESSID,             //hidessid
    WLAN0_VAP1_USERISOLATION,        //userisolation
    WLAN0_VAP1_WAPIPSK,              //wapiPsk
    WLAN0_VAP1_WAPIPSKLEN,           //wapiPskLen
    WLAN0_VAP1_WAPIAUTH,             //wapiAuth
    WLAN0_VAP1_WAPIPSKFORMAT,        //wapiPskFormat
    WLAN0_VAP1_WAPIASIPADDR,         //wapiAsIpAddr
    WLAN0_VAP1_WEPKEYTYPE,           //wepKeyType
    WLAN0_VAP1_WEPDEFAULTKEY,        //wepDefaultKey
    WLAN0_VAP1_WEP64KEY1,            //wep64Key1[WEP64_KEY_LEN]
    WLAN0_VAP1_WEP64KEY2,            //wep64Key2[WEP64_KEY_LEN]
    WLAN0_VAP1_WEP64KEY3,            //wep64Key3[WEP64_KEY_LEN]
    WLAN0_VAP1_WEP64KEY4,            //wep64Key4[WEP64_KEY_LEN]
    WLAN0_VAP1_WEP128KEY1,           //wep128Key1[WEP128_KEY_LEN]
    WLAN0_VAP1_WEP128KEY2,           //wep128Key2[WEP128_KEY_LEN]
    WLAN0_VAP1_WEP128KEY3,           //wep128Key3[WEP128_KEY_LEN]
    WLAN0_VAP1_WEP128KEY4,           //wep128Key4[WEP128_KEY_LEN]
    WLAN0_VAP1_WMMENABLE,            //wmmEnabled
    WLAN0_VAP1_RATEADAPTENABLE,      //rateAdaptiveEnabled
    WLAN0_VAP1_WLANBAND,             //wlanBand
    WLAN0_VAP1_FIXEDTARATE,          //fixedTxRate
    WLAN0_VAP1_FT_ENABLE,            //ft_enable
    WLAN0_VAP1_FT_MDID,              //ft_mdid
    WLAN0_VAP1_FT_OVER_DS,           //ft_over_ds
    WLAN0_VAP1_FT_RES_REQ,           //ft_res_request
    WLAN0_VAP1_FT_R0KEY_TO,          //ft_r0key_timeout
    WLAN0_VAP1_FT_REASOC_TO,         //ft_reasoc_timeout
    WLAN0_VAP1_FT_R0KH_ID,           //ft_r0kh_id
    WLAN0_VAP1_FT_PUSH,              //ft_push
    WLAN0_VAP1_FT_KH_NUM,            //ft_kh_num
    WLAN0_VAP1_CONF_END,
    WLAN0_VAP2_CONF_START,
	WLAN0_VAP2_ENCRYPT,              //encrypt
    WLAN0_VAP2_ENABLE1X,             //enable1X
    WLAN0_VAP2_WEP,                  //wep
    WLAN0_VAP2_WPAAUTH,              //wpaAuth
    WLAN0_VAP2_WPAPSKFORMAT,         //wpaPSKFormat
    WLAN0_VAP2_WPAPSK,               //wpaPSK[MAX_PSK_LEN+1]   
    WLAN0_VAP2_WPAGROUPREKEYTIME,    //wpaGroupRekeyTime
    WLAN0_VAP2_DISABLED,             //wlanDisabled
    WLAN0_VAP2_SSID,                 //ssid[MAX_SSID_LEN]
    WLAN0_VAP2_WLANMODE,             //wlanMode   
    WLAN0_VAP2_AUTHTYPE,             //authType
    WLAN0_VAP2_UNICASTCIPHER,        //unicastCipher
    WLAN0_VAP2_WPA2UNICASTCIPHER,    //wpa2UnicastCipher
    WLAN0_VAP2_BCNADVTISE,           //bcnAdvtisement
    WLAN0_VAP2_HIDESSID,             //hidessid
    WLAN0_VAP2_USERISOLATION,        //userisolation
    WLAN0_VAP2_WAPIPSK,              //wapiPsk
    WLAN0_VAP2_WAPIPSKLEN,           //wapiPskLen
    WLAN0_VAP2_WAPIAUTH,             //wapiAuth
    WLAN0_VAP2_WAPIPSKFORMAT,        //wapiPskFormat
    WLAN0_VAP2_WAPIASIPADDR,         //wapiAsIpAddr
    WLAN0_VAP2_WEPKEYTYPE,           //wepKeyType
    WLAN0_VAP2_WEPDEFAULTKEY,        //wepDefaultKey
    WLAN0_VAP2_WEP64KEY1,            //wep64Key1[WEP64_KEY_LEN]
    WLAN0_VAP2_WEP64KEY2,            //wep64Key2[WEP64_KEY_LEN]
    WLAN0_VAP2_WEP64KEY3,            //wep64Key3[WEP64_KEY_LEN]
    WLAN0_VAP2_WEP64KEY4,            //wep64Key4[WEP64_KEY_LEN]
    WLAN0_VAP2_WEP128KEY1,           //wep128Key1[WEP128_KEY_LEN]
    WLAN0_VAP2_WEP128KEY2,           //wep128Key2[WEP128_KEY_LEN]
    WLAN0_VAP2_WEP128KEY3,           //wep128Key3[WEP128_KEY_LEN]
    WLAN0_VAP2_WEP128KEY4,           //wep128Key4[WEP128_KEY_LEN]
    WLAN0_VAP2_WMMENABLE,            //wmmEnabled
    WLAN0_VAP2_RATEADAPTENABLE,      //rateAdaptiveEnabled
    WLAN0_VAP2_WLANBAND,             //wlanBand
    WLAN0_VAP2_FIXEDTARATE,          //fixedTxRate
    WLAN0_VAP2_FT_ENABLE,            //ft_enable
    WLAN0_VAP2_FT_MDID,              //ft_mdid
    WLAN0_VAP2_FT_OVER_DS,           //ft_over_ds
    WLAN0_VAP2_FT_RES_REQ,           //ft_res_request
    WLAN0_VAP2_FT_R0KEY_TO,          //ft_r0key_timeout
    WLAN0_VAP2_FT_REASOC_TO,         //ft_reasoc_timeout
    WLAN0_VAP2_FT_R0KH_ID,           //ft_r0kh_id
    WLAN0_VAP2_FT_PUSH,              //ft_push
    WLAN0_VAP2_FT_KH_NUM,            //ft_kh_num
    WLAN0_VAP2_CONF_END,
    WLAN0_VAP3_CONF_START,
	WLAN0_VAP3_ENCRYPT,              //encrypt
    WLAN0_VAP3_ENABLE1X,             //enable1X
    WLAN0_VAP3_WEP,                  //wep
    WLAN0_VAP3_WPAAUTH,              //wpaAuth
    WLAN0_VAP3_WPAPSKFORMAT,         //wpaPSKFormat
    WLAN0_VAP3_WPAPSK,               //wpaPSK[MAX_PSK_LEN+1]   
    WLAN0_VAP3_WPAGROUPREKEYTIME,    //wpaGroupRekeyTime
    WLAN0_VAP3_DISABLED,             //wlanDisabled
    WLAN0_VAP3_SSID,                 //ssid[MAX_SSID_LEN]
    WLAN0_VAP3_WLANMODE,             //wlanMode   
    WLAN0_VAP3_AUTHTYPE,             //authType
    WLAN0_VAP3_UNICASTCIPHER,        //unicastCipher
    WLAN0_VAP3_WPA2UNICASTCIPHER,    //wpa2UnicastCipher
    WLAN0_VAP3_BCNADVTISE,           //bcnAdvtisement
    WLAN0_VAP3_HIDESSID,             //hidessid
    WLAN0_VAP3_USERISOLATION,        //userisolation
    WLAN0_VAP3_WAPIPSK,              //wapiPsk
    WLAN0_VAP3_WAPIPSKLEN,           //wapiPskLen
    WLAN0_VAP3_WAPIAUTH,             //wapiAuth
    WLAN0_VAP3_WAPIPSKFORMAT,        //wapiPskFormat
    WLAN0_VAP3_WAPIASIPADDR,         //wapiAsIpAddr
    WLAN0_VAP3_WEPKEYTYPE,           //wepKeyType
    WLAN0_VAP3_WEPDEFAULTKEY,        //wepDefaultKey
    WLAN0_VAP3_WEP64KEY1,            //wep64Key1[WEP64_KEY_LEN]
    WLAN0_VAP3_WEP64KEY2,            //wep64Key2[WEP64_KEY_LEN]
    WLAN0_VAP3_WEP64KEY3,            //wep64Key3[WEP64_KEY_LEN]
    WLAN0_VAP3_WEP64KEY4,            //wep64Key4[WEP64_KEY_LEN]
    WLAN0_VAP3_WEP128KEY1,           //wep128Key1[WEP128_KEY_LEN]
    WLAN0_VAP3_WEP128KEY2,           //wep128Key2[WEP128_KEY_LEN]
    WLAN0_VAP3_WEP128KEY3,           //wep128Key3[WEP128_KEY_LEN]
    WLAN0_VAP3_WEP128KEY4,           //wep128Key4[WEP128_KEY_LEN]
    WLAN0_VAP3_WMMENABLE,            //wmmEnabled
    WLAN0_VAP3_RATEADAPTENABLE,      //rateAdaptiveEnabled
    WLAN0_VAP3_WLANBAND,             //wlanBand
    WLAN0_VAP3_FIXEDTARATE,          //fixedTxRate
    WLAN0_VAP3_FT_ENABLE,            //ft_enable
    WLAN0_VAP3_FT_MDID,              //ft_mdid
    WLAN0_VAP3_FT_OVER_DS,           //ft_over_ds
    WLAN0_VAP3_FT_RES_REQ,           //ft_res_request
    WLAN0_VAP3_FT_R0KEY_TO,          //ft_r0key_timeout
    WLAN0_VAP3_FT_REASOC_TO,         //ft_reasoc_timeout
    WLAN0_VAP3_FT_R0KH_ID,           //ft_r0kh_id
    WLAN0_VAP3_FT_PUSH,              //ft_push
    WLAN0_VAP3_FT_KH_NUM,            //ft_kh_num
    WLAN0_VAP3_CONF_END,
    WLAN0_VAP4_CONF_START,
	WLAN0_VAP4_ENCRYPT,              //encrypt
    WLAN0_VAP4_ENABLE1X,             //enable1X
    WLAN0_VAP4_WEP,                  //wep
    WLAN0_VAP4_WPAAUTH,              //wpaAuth
    WLAN0_VAP4_WPAPSKFORMAT,         //wpaPSKFormat
    WLAN0_VAP4_WPAPSK,               //wpaPSK[MAX_PSK_LEN+1]   
    WLAN0_VAP4_WPAGROUPREKEYTIME,    //wpaGroupRekeyTime
    WLAN0_VAP4_DISABLED,             //wlanDisabled
    WLAN0_VAP4_SSID,                 //ssid[MAX_SSID_LEN]
    WLAN0_VAP4_WLANMODE,             //wlanMode   
    WLAN0_VAP4_AUTHTYPE,             //authType
    WLAN0_VAP4_UNICASTCIPHER,        //unicastCipher
    WLAN0_VAP4_WPA2UNICASTCIPHER,    //wpa2UnicastCipher
    WLAN0_VAP4_BCNADVTISE,           //bcnAdvtisement
    WLAN0_VAP4_HIDESSID,             //hidessid
    WLAN0_VAP4_USERISOLATION,        //userisolation
    WLAN0_VAP4_WAPIPSK,              //wapiPsk
    WLAN0_VAP4_WAPIPSKLEN,           //wapiPskLen
    WLAN0_VAP4_WAPIAUTH,             //wapiAuth
    WLAN0_VAP4_WAPIPSKFORMAT,        //wapiPskFormat
    WLAN0_VAP4_WAPIASIPADDR,         //wapiAsIpAddr
    WLAN0_VAP4_WEPKEYTYPE,           //wepKeyType
    WLAN0_VAP4_WEPDEFAULTKEY,        //wepDefaultKey
    WLAN0_VAP4_WEP64KEY1,            //wep64Key1[WEP64_KEY_LEN]
    WLAN0_VAP4_WEP64KEY2,            //wep64Key2[WEP64_KEY_LEN]
    WLAN0_VAP4_WEP64KEY3,            //wep64Key3[WEP64_KEY_LEN]
    WLAN0_VAP4_WEP64KEY4,            //wep64Key4[WEP64_KEY_LEN]
    WLAN0_VAP4_WEP128KEY1,           //wep128Key1[WEP128_KEY_LEN]
    WLAN0_VAP4_WEP128KEY2,           //wep128Key2[WEP128_KEY_LEN]
    WLAN0_VAP4_WEP128KEY3,           //wep128Key3[WEP128_KEY_LEN]
    WLAN0_VAP4_WEP128KEY4,           //wep128Key4[WEP128_KEY_LEN]
    WLAN0_VAP4_WMMENABLE,            //wmmEnabled
    WLAN0_VAP4_RATEADAPTENABLE,      //rateAdaptiveEnabled
    WLAN0_VAP4_WLANBAND,             //wlanBand
    WLAN0_VAP4_FIXEDTARATE,          //fixedTxRate
    WLAN0_VAP4_FT_ENABLE,            //ft_enable
    WLAN0_VAP4_FT_MDID,              //ft_mdid
    WLAN0_VAP4_FT_OVER_DS,           //ft_over_ds
    WLAN0_VAP4_FT_RES_REQ,           //ft_res_request
    WLAN0_VAP4_FT_R0KEY_TO,          //ft_r0key_timeout
    WLAN0_VAP4_FT_REASOC_TO,         //ft_reasoc_timeout
    WLAN0_VAP4_FT_R0KH_ID,           //ft_r0kh_id
    WLAN0_VAP4_FT_PUSH,              //ft_push
    WLAN0_VAP4_FT_KH_NUM,            //ft_kh_num
    WLAN0_VAP4_CONF_END,
    WLAN1_CONF_START,
    WLAN1_ENCRYPT,              //encrypt
    WLAN1_ENABLE1X,             //enable1X
    WLAN1_WEP,                  //wep
    WLAN1_WPAAUTH,              //wpaAuth
    WLAN1_WPAPSKFORMAT,         //wpaPSKFormat
    WLAN1_WPAPSK,               //wpaPSK[MAX_PSK_LEN+1]   
    WLAN1_WPAGROUPREKEYTIME,    //wpaGroupRekeyTime
    WLAN1_DISABLED,             //wlanDisabled
    WLAN1_SSID,                 //ssid[MAX_SSID_LEN]
    WLAN1_WLANMODE,             //wlanMode   
    WLAN1_AUTHTYPE,             //authType
    WLAN1_UNICASTCIPHER,        //unicastCipher
    WLAN1_WPA2UNICASTCIPHER,    //wpa2UnicastCipher
    WLAN1_BCNADVTISE,           //bcnAdvtisement
    WLAN1_HIDESSID,             //hidessid
    WLAN1_USERISOLATION,        //userisolation
    WLAN1_WAPIPSK,              //wapiPsk
    WLAN1_WAPIPSKLEN,           //wapiPskLen
    WLAN1_WAPIAUTH,             //wapiAuth
    WLAN1_WAPIPSKFORMAT,        //wapiPskFormat
    WLAN1_WAPIASIPADDR,         //wapiAsIpAddr
    WLAN1_WEPKEYTYPE,           //wepKeyType
    WLAN1_WEPDEFAULTKEY,        //wepDefaultKey
    WLAN1_WEP64KEY1,            //wep64Key1[WEP64_KEY_LEN]
    WLAN1_WEP64KEY2,            //wep64Key2[WEP64_KEY_LEN]
    WLAN1_WEP64KEY3,            //wep64Key3[WEP64_KEY_LEN]
    WLAN1_WEP64KEY4,            //wep64Key4[WEP64_KEY_LEN]
    WLAN1_WEP128KEY1,           //wep128Key1[WEP128_KEY_LEN]
    WLAN1_WEP128KEY2,           //wep128Key2[WEP128_KEY_LEN]
    WLAN1_WEP128KEY3,           //wep128Key3[WEP128_KEY_LEN]
    WLAN1_WEP128KEY4,           //wep128Key4[WEP128_KEY_LEN]
    WLAN1_WMMENABLE,            //wmmEnabled
    WLAN1_RATEADAPTENABLE,      //rateAdaptiveEnabled
    WLAN1_WLANBAND,             //wlanBand
    WLAN1_FIXEDTARATE,          //fixedTxRate
    WLAN1_FT_ENABLE,            //ft_enable
    WLAN1_FT_MDID,              //ft_mdid
    WLAN1_FT_OVER_DS,           //ft_over_ds
    WLAN1_FT_RES_REQ,           //ft_res_request
    WLAN1_FT_R0KEY_TO,          //ft_r0key_timeout
    WLAN1_FT_REASOC_TO,         //ft_reasoc_timeout
    WLAN1_FT_R0KH_ID,           //ft_r0kh_id
    WLAN1_FT_PUSH,              //ft_push
    WLAN1_FT_KH_NUM,            //ft_kh_num
    WLAN1_CONF_END,
    WLAN1_VAP1_CONF_START,
	WLAN1_VAP1_ENCRYPT,              //encrypt
    WLAN1_VAP1_ENABLE1X,             //enable1X
    WLAN1_VAP1_WEP,                  //wep
    WLAN1_VAP1_WPAAUTH,              //wpaAuth
    WLAN1_VAP1_WPAPSKFORMAT,         //wpaPSKFormat
    WLAN1_VAP1_WPAPSK,               //wpaPSK[MAX_PSK_LEN+1]   
    WLAN1_VAP1_WPAGROUPREKEYTIME,    //wpaGroupRekeyTime
    WLAN1_VAP1_DISABLED,             //wlanDisabled
    WLAN1_VAP1_SSID,                 //ssid[MAX_SSID_LEN]
    WLAN1_VAP1_WLANMODE,             //wlanMode   
    WLAN1_VAP1_AUTHTYPE,             //authType
    WLAN1_VAP1_UNICASTCIPHER,        //unicastCipher
    WLAN1_VAP1_WPA2UNICASTCIPHER,    //wpa2UnicastCipher
    WLAN1_VAP1_BCNADVTISE,           //bcnAdvtisement
    WLAN1_VAP1_HIDESSID,             //hidessid
    WLAN1_VAP1_USERISOLATION,        //userisolation
    WLAN1_VAP1_WAPIPSK,              //wapiPsk
    WLAN1_VAP1_WAPIPSKLEN,           //wapiPskLen
    WLAN1_VAP1_WAPIAUTH,             //wapiAuth
    WLAN1_VAP1_WAPIPSKFORMAT,        //wapiPskFormat
    WLAN1_VAP1_WAPIASIPADDR,         //wapiAsIpAddr
    WLAN1_VAP1_WEPKEYTYPE,           //wepKeyType
    WLAN1_VAP1_WEPDEFAULTKEY,        //wepDefaultKey
    WLAN1_VAP1_WEP64KEY1,            //wep64Key1[WEP64_KEY_LEN]
    WLAN1_VAP1_WEP64KEY2,            //wep64Key2[WEP64_KEY_LEN]
    WLAN1_VAP1_WEP64KEY3,            //wep64Key3[WEP64_KEY_LEN]
    WLAN1_VAP1_WEP64KEY4,            //wep64Key4[WEP64_KEY_LEN]
    WLAN1_VAP1_WEP128KEY1,           //wep128Key1[WEP128_KEY_LEN]
    WLAN1_VAP1_WEP128KEY2,           //wep128Key2[WEP128_KEY_LEN]
    WLAN1_VAP1_WEP128KEY3,           //wep128Key3[WEP128_KEY_LEN]
    WLAN1_VAP1_WEP128KEY4,           //wep128Key4[WEP128_KEY_LEN]
    WLAN1_VAP1_WMMENABLE,            //wmmEnabled
    WLAN1_VAP1_RATEADAPTENABLE,      //rateAdaptiveEnabled
    WLAN1_VAP1_WLANBAND,             //wlanBand
    WLAN1_VAP1_FIXEDTARATE,          //fixedTxRate
    WLAN1_VAP1_FT_ENABLE,            //ft_enable
    WLAN1_VAP1_FT_MDID,              //ft_mdid
    WLAN1_VAP1_FT_OVER_DS,           //ft_over_ds
    WLAN1_VAP1_FT_RES_REQ,           //ft_res_request
    WLAN1_VAP1_FT_R0KEY_TO,          //ft_r0key_timeout
    WLAN1_VAP1_FT_REASOC_TO,         //ft_reasoc_timeout
    WLAN1_VAP1_FT_R0KH_ID,           //ft_r0kh_id
    WLAN1_VAP1_FT_PUSH,              //ft_push
    WLAN1_VAP1_FT_KH_NUM,            //ft_kh_num
    WLAN1_VAP1_CONF_END,
    WLAN1_VAP2_CONF_START,
	WLAN1_VAP2_ENCRYPT,              //encrypt
    WLAN1_VAP2_ENABLE1X,             //enable1X
    WLAN1_VAP2_WEP,                  //wep
    WLAN1_VAP2_WPAAUTH,              //wpaAuth
    WLAN1_VAP2_WPAPSKFORMAT,         //wpaPSKFormat
    WLAN1_VAP2_WPAPSK,               //wpaPSK[MAX_PSK_LEN+1]   
    WLAN1_VAP2_WPAGROUPREKEYTIME,    //wpaGroupRekeyTime
    WLAN1_VAP2_DISABLED,             //wlanDisabled
    WLAN1_VAP2_SSID,                 //ssid[MAX_SSID_LEN]
    WLAN1_VAP2_WLANMODE,             //wlanMode   
    WLAN1_VAP2_AUTHTYPE,             //authType
    WLAN1_VAP2_UNICASTCIPHER,        //unicastCipher
    WLAN1_VAP2_WPA2UNICASTCIPHER,    //wpa2UnicastCipher
    WLAN1_VAP2_BCNADVTISE,           //bcnAdvtisement
    WLAN1_VAP2_HIDESSID,             //hidessid
    WLAN1_VAP2_USERISOLATION,        //userisolation
    WLAN1_VAP2_WAPIPSK,              //wapiPsk
    WLAN1_VAP2_WAPIPSKLEN,           //wapiPskLen
    WLAN1_VAP2_WAPIAUTH,             //wapiAuth
    WLAN1_VAP2_WAPIPSKFORMAT,        //wapiPskFormat
    WLAN1_VAP2_WAPIASIPADDR,         //wapiAsIpAddr
    WLAN1_VAP2_WEPKEYTYPE,           //wepKeyType
    WLAN1_VAP2_WEPDEFAULTKEY,        //wepDefaultKey
    WLAN1_VAP2_WEP64KEY1,            //wep64Key1[WEP64_KEY_LEN]
    WLAN1_VAP2_WEP64KEY2,            //wep64Key2[WEP64_KEY_LEN]
    WLAN1_VAP2_WEP64KEY3,            //wep64Key3[WEP64_KEY_LEN]
    WLAN1_VAP2_WEP64KEY4,            //wep64Key4[WEP64_KEY_LEN]
    WLAN1_VAP2_WEP128KEY1,           //wep128Key1[WEP128_KEY_LEN]
    WLAN1_VAP2_WEP128KEY2,           //wep128Key2[WEP128_KEY_LEN]
    WLAN1_VAP2_WEP128KEY3,           //wep128Key3[WEP128_KEY_LEN]
    WLAN1_VAP2_WEP128KEY4,           //wep128Key4[WEP128_KEY_LEN]
    WLAN1_VAP2_WMMENABLE,            //wmmEnabled
    WLAN1_VAP2_RATEADAPTENABLE,      //rateAdaptiveEnabled
    WLAN1_VAP2_WLANBAND,             //wlanBand
    WLAN1_VAP2_FIXEDTARATE,          //fixedTxRate
    WLAN1_VAP2_FT_ENABLE,            //ft_enable
    WLAN1_VAP2_FT_MDID,              //ft_mdid
    WLAN1_VAP2_FT_OVER_DS,           //ft_over_ds
    WLAN1_VAP2_FT_RES_REQ,           //ft_res_request
    WLAN1_VAP2_FT_R0KEY_TO,          //ft_r0key_timeout
    WLAN1_VAP2_FT_REASOC_TO,         //ft_reasoc_timeout
    WLAN1_VAP2_FT_R0KH_ID,           //ft_r0kh_id
    WLAN1_VAP2_FT_PUSH,              //ft_push
    WLAN1_VAP2_FT_KH_NUM,            //ft_kh_num
    WLAN1_VAP2_CONF_END,
    WLAN1_VAP3_CONF_START,
	WLAN1_VAP3_ENCRYPT,              //encrypt
    WLAN1_VAP3_ENABLE1X,             //enable1X
    WLAN1_VAP3_WEP,                  //wep
    WLAN1_VAP3_WPAAUTH,              //wpaAuth
    WLAN1_VAP3_WPAPSKFORMAT,         //wpaPSKFormat
    WLAN1_VAP3_WPAPSK,               //wpaPSK[MAX_PSK_LEN+1]   
    WLAN1_VAP3_WPAGROUPREKEYTIME,    //wpaGroupRekeyTime
    WLAN1_VAP3_DISABLED,             //wlanDisabled
    WLAN1_VAP3_SSID,                 //ssid[MAX_SSID_LEN]
    WLAN1_VAP3_WLANMODE,             //wlanMode   
    WLAN1_VAP3_AUTHTYPE,             //authType
    WLAN1_VAP3_UNICASTCIPHER,        //unicastCipher
    WLAN1_VAP3_WPA2UNICASTCIPHER,    //wpa2UnicastCipher
    WLAN1_VAP3_BCNADVTISE,           //bcnAdvtisement
    WLAN1_VAP3_HIDESSID,             //hidessid
    WLAN1_VAP3_USERISOLATION,        //userisolation
    WLAN1_VAP3_WAPIPSK,              //wapiPsk
    WLAN1_VAP3_WAPIPSKLEN,           //wapiPskLen
    WLAN1_VAP3_WAPIAUTH,             //wapiAuth
    WLAN1_VAP3_WAPIPSKFORMAT,        //wapiPskFormat
    WLAN1_VAP3_WAPIASIPADDR,         //wapiAsIpAddr
    WLAN1_VAP3_WEPKEYTYPE,           //wepKeyType
    WLAN1_VAP3_WEPDEFAULTKEY,        //wepDefaultKey
    WLAN1_VAP3_WEP64KEY1,            //wep64Key1[WEP64_KEY_LEN]
    WLAN1_VAP3_WEP64KEY2,            //wep64Key2[WEP64_KEY_LEN]
    WLAN1_VAP3_WEP64KEY3,            //wep64Key3[WEP64_KEY_LEN]
    WLAN1_VAP3_WEP64KEY4,            //wep64Key4[WEP64_KEY_LEN]
    WLAN1_VAP3_WEP128KEY1,           //wep128Key1[WEP128_KEY_LEN]
    WLAN1_VAP3_WEP128KEY2,           //wep128Key2[WEP128_KEY_LEN]
    WLAN1_VAP3_WEP128KEY3,           //wep128Key3[WEP128_KEY_LEN]
    WLAN1_VAP3_WEP128KEY4,           //wep128Key4[WEP128_KEY_LEN]
    WLAN1_VAP3_WMMENABLE,            //wmmEnabled
    WLAN1_VAP3_RATEADAPTENABLE,      //rateAdaptiveEnabled
    WLAN1_VAP3_WLANBAND,             //wlanBand
    WLAN1_VAP3_FIXEDTARATE,          //fixedTxRate
    WLAN1_VAP3_FT_ENABLE,            //ft_enable
    WLAN1_VAP3_FT_MDID,              //ft_mdid
    WLAN1_VAP3_FT_OVER_DS,           //ft_over_ds
    WLAN1_VAP3_FT_RES_REQ,           //ft_res_request
    WLAN1_VAP3_FT_R0KEY_TO,          //ft_r0key_timeout
    WLAN1_VAP3_FT_REASOC_TO,         //ft_reasoc_timeout
    WLAN1_VAP3_FT_R0KH_ID,           //ft_r0kh_id
    WLAN1_VAP3_FT_PUSH,              //ft_push
    WLAN1_VAP3_FT_KH_NUM,            //ft_kh_num
    WLAN1_VAP3_CONF_END,
    WLAN1_VAP4_CONF_START,
	WLAN1_VAP4_ENCRYPT,              //encrypt
    WLAN1_VAP4_ENABLE1X,             //enable1X
    WLAN1_VAP4_WEP,                  //wep
    WLAN1_VAP4_WPAAUTH,              //wpaAuth
    WLAN1_VAP4_WPAPSKFORMAT,         //wpaPSKFormat
    WLAN1_VAP4_WPAPSK,               //wpaPSK[MAX_PSK_LEN+1]   
    WLAN1_VAP4_WPAGROUPREKEYTIME,    //wpaGroupRekeyTime
    WLAN1_VAP4_DISABLED,             //wlanDisabled
    WLAN1_VAP4_SSID,                 //ssid[MAX_SSID_LEN]
    WLAN1_VAP4_WLANMODE,             //wlanMode   
    WLAN1_VAP4_AUTHTYPE,             //authType
    WLAN1_VAP4_UNICASTCIPHER,        //unicastCipher
    WLAN1_VAP4_WPA2UNICASTCIPHER,    //wpa2UnicastCipher
    WLAN1_VAP4_BCNADVTISE,           //bcnAdvtisement
    WLAN1_VAP4_HIDESSID,             //hidessid
    WLAN1_VAP4_USERISOLATION,        //userisolation
    WLAN1_VAP4_WAPIPSK,              //wapiPsk
    WLAN1_VAP4_WAPIPSKLEN,           //wapiPskLen
    WLAN1_VAP4_WAPIAUTH,             //wapiAuth
    WLAN1_VAP4_WAPIPSKFORMAT,        //wapiPskFormat
    WLAN1_VAP4_WAPIASIPADDR,         //wapiAsIpAddr
    WLAN1_VAP4_WEPKEYTYPE,           //wepKeyType
    WLAN1_VAP4_WEPDEFAULTKEY,        //wepDefaultKey
    WLAN1_VAP4_WEP64KEY1,            //wep64Key1[WEP64_KEY_LEN]
    WLAN1_VAP4_WEP64KEY2,            //wep64Key2[WEP64_KEY_LEN]
    WLAN1_VAP4_WEP64KEY3,            //wep64Key3[WEP64_KEY_LEN]
    WLAN1_VAP4_WEP64KEY4,            //wep64Key4[WEP64_KEY_LEN]
    WLAN1_VAP4_WEP128KEY1,           //wep128Key1[WEP128_KEY_LEN]
    WLAN1_VAP4_WEP128KEY2,           //wep128Key2[WEP128_KEY_LEN]
    WLAN1_VAP4_WEP128KEY3,           //wep128Key3[WEP128_KEY_LEN]
    WLAN1_VAP4_WEP128KEY4,           //wep128Key4[WEP128_KEY_LEN]
    WLAN1_VAP4_WMMENABLE,            //wmmEnabled
    WLAN1_VAP4_RATEADAPTENABLE,      //rateAdaptiveEnabled
    WLAN1_VAP4_WLANBAND,             //wlanBand
    WLAN1_VAP4_FIXEDTARATE,          //fixedTxRate
    WLAN1_VAP4_FT_ENABLE,            //ft_enable
    WLAN1_VAP4_FT_MDID,              //ft_mdid
    WLAN1_VAP4_FT_OVER_DS,           //ft_over_ds
    WLAN1_VAP4_FT_RES_REQ,           //ft_res_request
    WLAN1_VAP4_FT_R0KEY_TO,          //ft_r0key_timeout
    WLAN1_VAP4_FT_REASOC_TO,         //ft_reasoc_timeout
    WLAN1_VAP4_FT_R0KH_ID,           //ft_r0kh_id
    WLAN1_VAP4_FT_PUSH,              //ft_push
    WLAN1_VAP4_FT_KH_NUM,            //ft_kh_num
    WLAN1_VAP4_CONF_END,

    //conf not in MBSSIB_TBL
    WLAN0_AUTO_CHAN_ENABLED,
    WLAN1_AUTO_CHAN_ENABLED,
    WLAN0_CHAN_NUM,
    WLAN1_CHAN_NUM,
    WLAN0_CHANNEL_WIDTH,
    WLAN1_CHANNEL_WIDTH,
    WLAN0_11N_COEXIST,
    WLAN1_11N_COEXIST,

    //access security start
    MAC_FILTER_SRC_ENABLE,
    IPFILTER_ON_OFF,
#ifdef URL_BLOCKING_SUPPORT
    URL_CAPABILITY,
#endif
    DOS_ENABLED,
    FW_GRADE,
    DHCP_MODE,
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
    DHCPV6_MODE,
#endif
#endif
    
    //mib chains
    MIB_CHAIN_START,
    WLAN0_FTKH_TBL,
    WLAN1_FTKH_TBL,
    TIME_SCHEDULE_TBL,
    MAC_FILTER_ROUTER_TBL,
    IP_PORT_FILTER_TBL,
#ifdef URL_BLOCKING_SUPPORT
    URL_FQDN_TBL,
#endif
#ifdef REMOTE_ACCESS_CTL
    ACC_TBL,
#endif
	ATM_VC_TBL,
    MIB_CHAIN_END,
    //access security end
    
    TYPE_IDX_END,           //all tyoe index should add before it, last option, implys end of option
};

typedef struct opt_tlv_s
{
    unsigned short type;
    unsigned short len;
    unsigned char data[0];
}OPT_TLV_T, *OPT_TLV_Tp;

typedef struct atmvc_wan_entry {
	unsigned int ifIndex;	// resv | media | ppp | vc
	unsigned char vpi;
	unsigned char qos;
	unsigned short vci;
	unsigned short pcr;
	unsigned short scr;
	unsigned short mbs;
	unsigned int cdvt;
	unsigned char encap;
	unsigned char napt;
	unsigned char cmode;
	unsigned char brmode;	// 0: transparent bridging, 1: PPPoE bridging
	unsigned char pppUsername[MAX_PPP_NAME_LEN+1];
	unsigned char pppPassword[MAX_PPP_PWD_LEN+1];
	unsigned char pppAuth;	// 0:AUTO, 1:PAP, 2:CHAP
	unsigned char pppACName[MAX_NAME_LEN];
	unsigned char pppServiceName[MAX_NAME_LEN];	// Jenny, TR-069 PPPoEServiceName
	unsigned char pppCtype;
	unsigned short pppIdleTime;
#ifdef CONFIG_USER_IP_QOS
	unsigned char enableIpQos;
#endif
#ifdef CONFIG_IGMPPROXY_MULTIWAN
	unsigned char enableIGMP;
#endif
	unsigned char ipDhcp;
	unsigned char rip;
	unsigned char ipAddr[IP_ADDR_LEN];
	unsigned char remoteIpAddr[IP_ADDR_LEN];
	unsigned char dgw;
	unsigned int mtu;
	unsigned char enable;
	unsigned char netMask[IP_ADDR_LEN];	// Jenny; Subnet mask
	unsigned char ipunnumbered;	// Jenny, unnumbered(1)
	unsigned char dnsMode;  // 1: enable, 0: disable
	unsigned char v4dns1[IP_ADDR_LEN];
	unsigned char v4dns2[IP_ADDR_LEN];
// Mason Yu. combine_1p_4p_PortMapping, enable_802_1p_090722
#if defined(ITF_GROUP) || defined(NEW_PORTMAPPING)
	// used for VLAN mapping
	unsigned char vlan;
	unsigned short vid;
	unsigned short vprio;	// 802.1p priority bits
	unsigned char vpass;	// vlan passthrough
	// used for interface group
	unsigned int itfGroup;
#endif
#ifdef CONFIG_MCAST_VLAN
	unsigned short mVid; /*use for multicast vlan, IPTV*/
#endif
	unsigned long cpePppIfIndex;   // Mason Yu. Remote Management
	unsigned long cpeIpIndex;      // Mason Yu. Remote Management

#ifdef _CWMP_MIB_ /*jiunming, for cwmp-tr069*/
	unsigned char connDisable; //0:enable, 1:disable
	unsigned int ConDevInstNum;
	unsigned int ConIPInstNum;
	unsigned int ConPPPInstNum;
	unsigned short autoDisTime;	// Jenny, TR-069 PPP AutoDisconnectTime
	unsigned short warnDisDelay;	// Jenny, TR-069 PPP WarnDisconnectDelay
//	unsigned char pppServiceName[MAX_NAME_LEN];	// Jenny, TR-069 PPPoEServiceName

#ifdef _PRMT_TR143_
	unsigned char TR143UDPEchoItf;
#endif //_PRMT_TR143_
#ifdef _PRMT_X_CT_COM_WANEXT_
	unsigned char IPForwardList[IPFORWARD_SIZE+1];
#endif //_PRMT_X_CT_COM_WANEXT_
#endif //_CWMP_MIB_
unsigned char WanName[MAX_NAME_LEN];	//Name of this wan connection
#ifdef CONFIG_USER_PPPOE_PROXY
	unsigned char PPPoEProxyEnable;
	unsigned int  PPPoEProxyMaxUser;
#endif //CONFIG_USER_PPPOE_PROXY
	unsigned int applicationtype;  //TR069(1), INTERNET(2), IPTV(4), VOICE(8), SPECIAL_SERVICE_1(16), SPECIAL_SERVICE_2(32), SPECIAL_SERVICE_3(64), SPECIAL_SERVICE_4(128)
	//char applicationname[MAX_NAME_LEN];
	unsigned char disableLanDhcp;  /* disable dhcp on lan interface binding with this wan interface */
	unsigned char svtype;		// Mason Yu. t123
#ifdef CONFIG_SPPPD_STATICIP
	unsigned char pppIp;	// Jenny, static PPPoE
#endif
#ifdef CONFIG_USER_WT_146
	unsigned char	bfd_enable;
	unsigned char	bfd_opmode;
	unsigned char	bfd_role;
	unsigned int	bfd_mintxint;
	unsigned int	bfd_minrxint;
	unsigned int	bfd_minechorxint;
	unsigned char	bfd_detectmult;
	unsigned char	bfd_authtype;
	unsigned char	bfd_authkeyid;
	unsigned char	bfd_authkeylen;
	unsigned char	bfd_authkey[BFD_MAX_KEY_LEN];
	unsigned char	bfd_dscp;
	unsigned char	bfd_ethprio;
#endif //CONFIG_USER_WT_146

#ifdef CONFIG_IPV6
	unsigned char	IpProtocol; 		 // 1: IPv4, 2:IPv6, 3: IPv4 and IPv6
	unsigned char	AddrMode;            // Bitmap, bit0: Slaac, bit1: Static, bit2: DS-Lite , bit3: 6rd, bit4: DHCP Client
	unsigned char Ipv6Addr[IP6_ADDR_LEN];
	unsigned char RemoteIpv6Addr[IP6_ADDR_LEN];
	unsigned char IPv6PrefixOrigin;	// 0:PrefixDelegation, 1:Static, 2: PPPoE, 3:None
	unsigned char Ipv6Prefix[IP6_ADDR_LEN];	//only for static
	unsigned char Ipv6AddrPrefixLen;
	unsigned int IPv6PrefixPltime;			//only for static
	unsigned int IPv6PrefixVltime;			//only for static
	char IPv6DomainName[65];
	unsigned char Ipv6Dhcp;            // 0: disable, 1: enable
	unsigned char	Ipv6DhcpRequest;     // Bitmap, bit0: Request Address, bit1: Request Prefix
	unsigned char RemoteIpv6EndPointAddr[IP6_ADDR_LEN];
	char Ipv6AddrAlias[41];
	char Ipv6PrefixAlias[41];
	unsigned char Ipv6Dns1[IP6_ADDR_LEN];
	unsigned char Ipv6Dns2[IP6_ADDR_LEN];
#endif
    //6rd
#ifdef CONFIG_IPV6_SIT_6RD
	unsigned char SixrdBRv4IP[IP_ADDR_LEN];
	unsigned char SixrdIPv4MaskLen;
	unsigned char SixrdPrefix[IP6_ADDR_LEN];
	unsigned char SixrdPrefixLen;
#endif
	unsigned char MacAddr[MAC_ADDR_LEN];
#ifdef CONFIG_RTK_RG_INIT
	int rg_wan_idx;
	int check_br_pm; //check bridge portmapping
#endif
#if defined(CONFIG_GPON_FEATURE)
	unsigned char sid;
#endif
#if 0 /* ignore dhcp option to make the sizeof atmvc_wan_entry is less than CLUSTER_PKT_MAX_LEN */
	unsigned char dhcpv6_opt16_enable[4];
	unsigned char dhcpv6_opt16_type[4];
	unsigned char dhcpv6_opt16_value_mode[4];	//1: ITMS+ assign, 2: Auto-gen, 3: auto-gen encrypted voip info
	unsigned char dhcpv6_opt16_value[4][100];
	unsigned char dhcpv6_opt17_enable[4];
	unsigned char dhcpv6_opt17_type[4];		//1: match sub_code & sub_data, 2:match value
	unsigned char dhcpv6_opt17_sub_code[4];
	unsigned char dhcpv6_opt17_sub_data[4][36];	//max length is 32
	unsigned char dhcpv6_opt17_value[4][36];
	unsigned char dhcp_opt60_enable[4];
	unsigned char dhcp_opt60_type[4];
	unsigned char dhcp_opt60_value_mode[4];	//0: ITMS+ assign, 1: Auto-gen, 2: auto-gen encrypted voip info
	unsigned char dhcp_opt60_value[4][100];
	unsigned char dhcp_opt125_enable[4];
	unsigned char dhcp_opt125_type[4];		//1: match sub_code & sub_data, 2:match value
	unsigned char dhcp_opt125_sub_code[4];
	unsigned char dhcp_opt125_sub_data[4][36];	//max length is 32
	unsigned char dhcp_opt125_value[4][36];
#endif
#if defined(CONFIG_IPV6) && defined(DUAL_STACK_LITE)
	unsigned char dslite_enable;
	unsigned char dslite_aftr_mode;	//0: auto, 1:manual
	unsigned char dslite_aftr_addr[IP6_ADDR_LEN];
	unsigned char dslite_aftr_hostname[64];
#endif
#if defined(CONFIG_IPV6)
	unsigned char dnsv6Mode;  // 1: enable, 0: disable
#endif
} __PACK__ MIB_CE_ATM_VC_WAN_T, *MIB_CE_ATM_VC_WANTp;

void debug_dump_packet(unsigned char *ptr, unsigned short len);
void option_debug(unsigned short type, unsigned short len, unsigned char *ptr);
unsigned short get_checksum(unsigned char *ptr, unsigned short len);

#endif

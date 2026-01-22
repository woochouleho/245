/*-----------------------------------------------------------------
 * File: boamain.c
 *-----------------------------------------------------------------*/

#include <string.h>
#include "webform.h"
#include "mib.h"
#include "../defs.h"
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
//xl_yue
#include "utility.h"
#include "boa.h"


/*+++++add by Jack for VoIP project 20/03/07+++++*/
#ifdef VOIP_SUPPORT
#include "web_voip.h"
#endif /*VOIP_SUPPORT*/
/*-----end-----*/
#include "multilang.h"
#ifdef CONFIG_DEV_xDSL
#include "subr_dsl.h"
#endif
#include <arpa/inet.h>

#ifdef USE_LOGINWEB_OF_SERVER
// Mason Yu on True
extern unsigned char g_login_username[MAX_NAME_LEN];
#endif

//#ifdef _USE_RSDK_WRAPPER_
void initDosPage(request * wp);
void initSyslogPage(request * wp);
void initHolepunchingPage(request * wp);
void initDgwPage(request * wp);
void initEthoamY1731Page(request * wp);
void initEthoam8023AHPage(request * wp);
#ifdef CONFIG_USER_WIRED_8021X
void initWired8021x(request * wp);
#endif
//#endif //_USE_RSDK_WRAPPER_

static void hex_to_string(unsigned char* hex_str, int length, char * acsii_str)
{
        int i = 0, j = 0;

        for (i = 0; i < length/2; i++, j+=2)
                sprintf(acsii_str+j, "%02x", hex_str[i]);
}
void rtl8670_AspInit() {
   /*
 *	ASP script procedures and form functions.
 */
	boaASPDefine("getInfo", getInfo);
	boaASPDefine("check_display", check_display);
	// Kaohj
	boaASPDefine("checkWrite", checkWrite);
	boaASPDefine("initPage", initPage);
#ifdef CONFIG_GENERAL_WEB
	boaASPDefine("SendWebHeadStr", SendWebHeadStr);
	boaASPDefine("CheckMenuDisplay", CheckMenuDisplay);
#endif
	boaASPDefine("getReplacePwPage", getReplacePwPage);
#ifdef WLAN_QoS
	boaASPDefine("ShowWmm", ShowWmm);
#endif
#ifdef WLAN_11K
	boaASPDefine("ShowDot11k_v", ShowDot11k_v);
#endif
	   // add by yq_zhou 1.20
	boaASPDefine("write_wladvanced", write_wladvanced);

#if 1//defined(WLAN_SUPPORT) && defined(CONFIG_ARCH_RTL8198F)
    boaASPDefine("extra_wlbasic", extra_wlbasic);
    boaASPDefine("extra_wladvanced", extra_wladvanced);
#endif //defined(WLAN_SUPPORT) && defined(CONFIG_ARCH_RTL8198F)

/* add by yq_zhou 09.2.02 add sagem logo for 11n*/
/*#ifdef CONFIG_11N_SAGEM_WEB
	boaASPDefine("writeTitle", write_title);
	boaASPDefine("writeLogoBelow", write_logo_below);
#endif*/
#ifdef PORT_FORWARD_GENERAL
	boaASPDefine("portFwList", portFwList);
#endif
	boaASPDefine("portStaticMappingList", portStaticMappingList);
#ifdef NATIP_FORWARDING
	boaASPDefine("ipFwList", ipFwList);
#endif
#ifdef IP_PORT_FILTER
	boaASPDefine("ipPortFilterList", ipPortFilterList);
#endif
#ifdef MAC_FILTER
	boaASPDefine("macFilterList", macFilterList);
#endif

	boaASPDefine("atmVcList", atmVcList);
	boaASPDefine("atmVcList2", atmVcList2);
	boaASPDefine("wanConfList", wanConfList);
	boaASPDefine("DHCPSrvStatus", DHCPSrvStatus);
#ifdef CONFIG_00R0
	boaASPDefine("lanConfList", lanConfList);
#endif
#ifdef CONFIG_DEV_xDSL
	boaASPDefine("DSLVer", DSLVer);
	boaASPDefine("DSLStatus", DSLStatus);
#endif
#ifdef CONFIG_00R0
	boaASPDefine("BootLoaderVersion", BootLoaderVersion);
	boaASPDefine("GPONSerialNumber", GPONSerialNumber);
#ifdef CONFIG_WLAN
	boaASPDefine("wlanActiveConfList", wlanActiveConfList);
#endif
#endif
#ifdef CONFIG_IPV6
	boaASPDefine("wanip6ConfList", wanip6ConfList);
	boaASPDefine("routeip6ConfList", routeip6ConfList);
	boaASPDefine("dsliteConfList", dsliteConfList);
	boaASPDefine("neighborList", neighborList);
#endif
	boaASPDefine("getNameServer", getNameServer);
	boaASPDefine("getDefaultGW", getDefaultGW);
#ifdef CONFIG_IPV6
	boaASPDefine("getDefaultGW_ipv6", getDefaultGW_ipv6);
#endif
	boaFormDefine("formTcpipLanSetup", formTcpipLanSetup);	// TCP/IP Lan Setting Form
#ifdef CONFIG_USER_WIRED_8021X
	boaFormDefine("formConfig_802_1x", formConfig_802_1x);
	boaASPDefine("getPortStatus", getPortStatus);
#endif
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	boaASPDefine("DHCPClientScript", dhcpc_script);
	boaASPDefine("DHCPClientClickSetup", dhcpc_clicksetup);
#endif
	boaASPDefine("TcpipLanOninit", tcpip_lan_oninit);
#ifdef CONFIG_USER_VLAN_ON_LAN
	boaFormDefine("formVLANonLAN", formVLANonLAN);	// formVLANonLAN Setting Form
#endif
	boaASPDefine("lan_setting", lan_setting);
	boaASPDefine("checkIP2", checkIP2);
	boaASPDefine("lanScript", lan_script);
	boaASPDefine("lan_port_mask", lan_port_mask);

#ifdef WEB_REDIRECT_BY_MAC
	boaFormDefine("formLanding", formLanding);
#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT
	boaFormDefine("formUploadWapiCert1", formUploadWapiCert1);
	boaFormDefine("formUploadWapiCert2", formUploadWapiCert2);
	boaFormDefine("formWapiReKey", formWapiReKey);
#endif


	boaASPDefine("getWanIfDisplay", getWanIfDisplay );
	boaFormDefine("formWanRedirect", formWanRedirect);

	boaASPDefine("cpuUtility", cpuUtility );
	boaASPDefine("memUtility", memUtility );

//#ifdef CONFIG_USER_PPPOMODEM
	boaASPDefine("wan3GTable", wan3GTable );
//#endif //CONFIG_USER_PPPOMODEM
	boaASPDefine("wanPPTPTable", wanPPTPTable );
	boaASPDefine("wanL2TPTable", wanL2TPTable );
	boaASPDefine("wanIPIPTable", wanIPIPTable );
#ifdef CONFIG_USER_CUPS
	boaASPDefine("printerList", printerList);
#endif
#ifdef CONFIG_USER_PPPOMODEM
	boaASPDefine("fm3g_checkWrite", fm3g_checkWrite);
	boaFormDefine("form3GConf", form3GConf);
#endif //CONFIG_USER_PPPOMODEM

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	boaFormDefine("formStatus_pon", formStatus_pon);
	boaASPDefine("ponGetStatus", ponGetStatus);
	boaFormDefine("formPonStats", formPonStats);

	// add hw/sw flow count
	boaASPDefine("ponflowGetStatus", ponflowGetStatus);
#ifdef CONFIG_GPON_FEATURE
	boaASPDefine("showgpon_status", showgpon_status);
	boaASPDefine("fmgpon_checkWrite", fmgpon_checkWrite);
	boaFormDefine("formgponConf", formgponConf);
#ifdef CONFIG_00R0
	boaFormDefine("formOmciMcMode", formOmciMcMode);
#ifdef CONFIG_TR142_MODULE
	boaASPDefine("showPonLanPorts", showPonLanPorts);
	boaFormDefine("fmgponType", fmgponType);
#endif
#endif
#endif

#ifdef CONFIG_EPON_FEATURE
	boaASPDefine("showepon_LLID_status", showepon_LLID_status);
	boaASPDefine("showepon_LLID_MAC", showepon_LLID_MAC);
	boaASPDefine("fmepon_checkWrite", fmepon_checkWrite);
	boaFormDefine("formepon_llid_mac_mapping", formepon_llid_mac_mapping);
	boaFormDefine("formeponConf", formeponConf);
#endif
#endif
#ifdef CONFIG_GPON_FEATURE
	boaASPDefine("fmvlan_checkWrite", fmvlan_checkWrite);
	boaFormDefine("formVlan", formVlan);
	boaASPDefine("omciVlanInfo", omciVlanInfo);
	boaASPDefine("showOMCI_OLT_mode", showOMCI_OLT_mode);
	boaASPDefine("fmOmciInfo_checkWrite", fmOmciInfo_checkWrite);
	boaFormDefine("formOmciInfo", formOmciInfo);
#endif

#ifdef _CWMP_MIB_
	boaFormDefine("formTR069Config", formTR069Config);
	boaFormDefine("formTR069CPECert", formTR069CPECert);
	boaFormDefine("formTR069CACert", formTR069CACert);

	boaASPDefine("TR069ConPageShow", TR069ConPageShow);
	boaASPDefine("SupportTR111", SupportTR111);
#endif
	boaASPDefine("portFwTR069", portFwTR069);
#ifdef PORT_FORWARD_GENERAL
	boaFormDefine("formPortFw", formPortFw);					// Firewall Port forwarding Setting Form
#endif
	boaFormDefine("formStaticMapping", formStaticMapping);					// Firewall Port forwarding Setting Form
#ifdef NATIP_FORWARDING
	boaFormDefine("formIPFw", formIPFw);					// Firewall NAT IP forwarding Setting Form
#endif
#ifdef PORT_TRIGGERING_STATIC
	boaFormDefine("formGaming", formGaming);					// Firewall Port Triggering Setting Form
#endif
#ifdef PORT_TRIGGERING_DYNAMIC
	boaFormDefine("formPortTrigger", formPortTrigger);					// Firewall Port Triggering Setting Form
	boaASPDefine("portTriggerList", portTriggerList);
#endif
#if defined(IP_PORT_FILTER) || defined(MAC_FILTER)
	boaFormDefine("formFilter", formFilter);					// Firewall IP, Port, Mac Filter Setting Form
#endif
#ifdef LAYER7_FILTER_SUPPORT
	boaFormDefine("formlayer7", formLayer7);                            //star: for layer7 filter
	boaASPDefine("AppFilterList", AppFilterList);
#endif
#ifdef PARENTAL_CTRL
	boaFormDefine("formParentCtrl", formParentCtrl);
	boaASPDefine("parentControlList", parentalCtrlList);
#endif
#ifdef DMZ
	boaFormDefine("formDMZ", formDMZ);						// Firewall DMZ Setting Form
#endif

	boaFormDefine("formPasswordSetup", formPasswordSetup);	// Management Password Setting Form
	boaFormDefine("formUserPasswordSetup", formUserPasswordSetup);	// Management User Password Setting Form

#ifdef ACCOUNT_CONFIG
	boaFormDefine("formAccountConfig", formAccountConfig);	// Management Account Configuration Setting Form
	boaASPDefine("accountList", accountList);
#endif
#ifdef WEB_UPGRADE
	boaFormDefine("formUpload", formUpload);					// Management Upload Firmware Setting Form
#ifdef CONFIG_DOUBLE_IMAGE
	boaFormDefine("formStopUpload", formStopUpload);				// Management Stop Upload Firmware Setting Form
#endif
#endif
#if 1
	boaFormDefine("formSaveConfig", formSaveConfig);			// Management Upload/Download Configuration Setting Form
#endif
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	boaFormDefine("formSnmpConfig", formSnmpConfig);			// SNMP Configuration Setting Form
#endif
#ifdef CONFIG_USER_SNMPD_SNMPD_V3
	boaASPDefine("initSnmpConfig", initSnmpConfig);
#endif
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_RADVD
	boaFormDefine("formRadvdSetup", formRadvdSetup);			// RADVD Configuration Setting Form
#endif
#ifdef DHCPV6_ISC_DHCP_4XX
	boaFormDefine("formDhcpv6Server", formDhcpv6);			        // DHCPv6 Server Setting Form
	boaASPDefine("showDhcpv6SNameServerTable", showDhcpv6SNameServerTable);     // Name Server List for DHCPv6 Server
	boaASPDefine("showDhcpv6SDOMAINTable", showDhcpv6SDOMAINTable);             // Domian search List for DHCPv6 Server
	boaASPDefine("dhcpClientListv6", dhcpClientListv6);
	boaASPDefine("showDhcpv6SNTPServerTable", showDhcpv6SNTPServerTable);     // NTP Server List for DHCPv6 Server
	boaASPDefine("showDhcpv6SMacBindingTable", showDhcpv6SMacBindingTable);     // NTP Server List for DHCPv6 Server
#endif
#endif
#ifdef CONFIG_DEV_xDSL
#ifdef CONFIG_DSL_VTUO
	boaFormDefine("formSetVTUO", formSetVTUO);				// VTU-O Setting Form
	boaASPDefine("vtuo_checkWrite", vtuo_checkWrite);
#endif /*CONFIG_DSL_VTUO*/
	boaFormDefine("formSetAdsl", formSetAdsl);				// ADSL Driver Setting Form
	boaFormDefine("formStatAdsl", formStatAdsl);			// ADSL Statistics Form
#ifdef FIELD_TRY_SAFE_MODE
	boaFormDefine("formAdslSafeMode", formAdslSafeMode);		// ADSL Safe mode Setting Form
#endif
	boaFormDefine("formDiagAdsl", formDiagAdsl);			// ADSL Driver Diag Form
#endif
	boaFormDefine("formStats", formStats);				// Packet Statistics Form
	boaFormDefine("formRconfig", formRconfig);				// Remote Configuration Form
//	boaFormDefine("formSysCmd",  formSysCmd);				// hidden page for system command
	boaASPDefine("sysCmdLog", sysCmdLog);
#ifdef CONFIG_USER_RTK_SYSLOG
	boaFormDefine("formSysLog",  formSysLog);				// hidden page for system command
	boaASPDefine("sysLogList", sysLogList);			// Web Log
	boaASPDefine("RemoteSyslog", RemoteSyslog); // Jenny, for remote system log
	boaASPDefine("FileList", FileList); // Jenny, for remote system log
#endif
	boaFormDefine("formWanApConnection",  formWanApConnection);				// hidden page for system command
	boaASPDefine("CheckApConnection", CheckApConnection); // Jenny, for remote system log
	boaFormDefine("formHolepunching",  formHolepunching);
	boaASPDefine("holePunghing", holePunghing); // Jenny, for remote system log
	boaFormDefine("formLdap",  formLdap);
	boaFormDefine("formBankStatus",  formBankStatus);
	boaFormDefine("formUpload1",  formUpload1);
	boaFormDefine("formUpload2",  formUpload2);
	boaFormDefine("formUpload3",  formUpload3);
	boaFormDefine("formUpload4",  formUpload4);
	boaFormDefine("formUpload5",  formUpload5);
	boaFormDefine("formUpload6",  formUpload6);
#ifdef CONFIG_DEV_xDSL
	boaFormDefine("formSetAdslTone", formSetAdslTone);		// ADSL Driver Setting Tone Form
	boaFormDefine("formSetAdslPSD", formSetAdslPSD);		// ADSL Driver Setting Tone Form
	boaASPDefine("adslToneDiagTbl", adslToneDiagTbl);
	boaASPDefine("adslToneDiagList", adslToneDiagList);
	boaASPDefine("adslToneConfDiagList", adslToneConfDiagList);
	boaASPDefine("adslPSDMaskTbl", adslPSDMaskTbl);
	boaASPDefine("adslPSDMeasure", adslPSDMeasure);
//#ifdef CONFIG_VDSL
	boaASPDefine("vdslBandStatusTbl", vdslBandStatusTbl);
//#endif /*CONFIG_VDSL*/
#endif
	boaASPDefine("pktStatsList", pktStatsList);

	boaFormDefine("formStatus", formStatus);				// Status Form
//#if defined(CONFIG_RTL_8676HWNAT)
	boaFormDefine("formLANPortStatus", formLANPortStatus);	// formLANPortStatus Form
	boaASPDefine("showLANPortStatus", showLANPortStatus);
//#endif
#ifdef CONFIG_IPV6
	boaFormDefine("formStatus_ipv6", formStatus_ipv6);
#endif
	boaFormDefine("formStatusModify", formDate);

	boaFormDefine("formWanMode", formWanMode);
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN) || defined(WLAN_WISP)
	boaFormDefine("formWanEth", formEth);					// ADSL Configuration Setting Form
#ifdef CONFIG_USER_WAN_PORT_AUTO_SELECTION
	boaFormDefine("formWanPortSet", formWanPortSet);
#endif

#ifdef CONFIG_00R0
	boaFormDefine("formWanEth_admin", formEthAdmin);
#endif
#endif
#ifdef CONFIG_DEV_xDSL
	boaFormDefine("formWanAtm", formAtmVc);					// Atm VC Configuration Setting Form
	boaFormDefine("formWanAdsl", formAdsl);					// ADSL Configuration Setting Form
#endif
	boaFormDefine("formPPPEdit", formPPPEdit);				// PPP interface Configuration Form
	boaFormDefine("formIPEdit", formIPEdit);				// IP interface Configuration Form
	boaFormDefine("formBrEdit", formBrEdit);				// Bridged interface Configuration Form

	boaFormDefine("formBridging", formBridge);				// Bridge Configuration Form
	boaASPDefine("bridgeFdbList", bridgeFdbList);
	boaASPDefine("ARPTableList", ARPTableList);
	boaFormDefine("formRefleshFdbTbl", formRefleshFdbTbl);
#ifdef CONFIG_USER_RTK_LAN_USERLIST
	boaFormDefine("formRefleshLanUserTbl", formRefleshLanUserTbl);
	boaASPDefine("LanUserTableList", LanUserTableList);
#endif

#ifdef ROUTING
	boaFormDefine("formRouting", formRoute);				// Routing Configuration Form
	boaASPDefine("showStaticRoute", showStaticRoute);
#endif
#if defined(CONFIG_IPV6) && defined(ROUTING)
	boaFormDefine("formIPv6EnableDisable", formIPv6EnableDisable);		// IPv6 Enable/Disable Configuration Form
	boaFormDefine("formIPv6Routing", formIPv6Route);		// Routing Configuration Form
	boaASPDefine("showIPv6StaticRoute", showIPv6StaticRoute);
	boaFormDefine("formIPv6RefleshRouteTbl", formRefleshRouteTbl);
	boaASPDefine("routeIPv6List", routeIPv6List);
#ifdef IP_PORT_FILTER
	boaFormDefine("formFilterV6", formFilterV6);					// Firewall IPv6, Port, Setting Form
	boaASPDefine("ipPortFilterListV6", ipPortFilterListV6);
#endif
#endif

	boaASPDefine("ShowDefaultGateway", ShowDefaultGateway);	// Jenny, for DEFAULT_GATEWAY_V2
	boaASPDefine("GetDefaultGateway", GetDefaultGateway);
	boaASPDefine("DisplayDGW", DisplayDGW);
	boaASPDefine("DisplayTR069WAN", DisplayTR069WAN);
	boaFormDefine("formRefleshRouteTbl", formRefleshRouteTbl);
	boaASPDefine("routeList", routeList);

#ifdef CONFIG_USER_DHCP_SERVER
	boaFormDefine("formDhcpServer", formDhcpd);				// DHCP Server Setting Form
	boaASPDefine("dhcpClientList", dhcpClientList);
	boaASPDefine("dhcpClientListWithNickname", dhcpClientListWithNickname);
	boaFormDefine("formReflashClientTbl", formReflashClientTbl);
#endif

#ifdef CONFIG_USER_DDNS
	boaFormDefine("formDDNS", formDDNS);
	boaASPDefine("showDNSTable", showDNSTable);
	boaASPDefine("ddnsServicePort", ddnsServicePort);
#endif
#ifdef CONFIG_USER_DHCP_SERVER
	boaFormDefine("formDhcrelay", formDhcrelay);			// DHCPRelay Configuration Form
#endif
	boaFormDefine("formPing", formPing);					// Ping diagnostic Form
	boaFormDefine("formTracert", formTracert);				// Tracert diagnostic Form
	boaFormDefine("formTracertResult", formTracertResult);
	boaFormDefine("formPingResult", formPingResult);
	boaASPDefine("clearPingResult", clearPingResult);
	boaASPDefine("clearTracertResult", clearTracertResult);
	boaFormDefine("formGroupTable", formGroupTable);				// Group Table diagnostic Form
	boaFormDefine("formGroupTableResult", formGroupTableResult);	// Group Table Result diagnostic Form
	boaFormDefine("formSsh", formSsh);				// Group Table diagnostic Form
	boaFormDefine("formSshResult", formSshResult);	// Group Table Result diagnostic Form
	boaFormDefine("formNetstat", formNetstat);	// Netstat diagnostic Form
	boaFormDefine("formNetstatResult", formNetstatResult);	// Netstat diagnostic Form
	boaFormDefine("formPortMirror", formPortMirror);	// PortMirror diagnostic Form
	boaFormDefine("formPortMirrorResult", formPortMirrorResult);	// PortMirrorResult diagnostic Form
	boaASPDefine("clearPortMirrorResult", clearPortMirrorResult);
#ifdef CONFIG_IPV6
	boaFormDefine("formPing6", formPing6);					// Ping6 diagnostic Form
	boaFormDefine("formTracert6", formTracert6);			// Tracert6 diagnostic Form
#endif
#ifdef CONFIG_USER_TCPDUMP_WEB
	boaFormDefine("formCapture", formCapture);				// Packet Capture
#endif
#ifdef CONFIG_DEV_xDSL
	boaFormDefine("formOAMLB", formOamLb);					// OAM Loopback diagnostic Form
#endif
#ifdef ADDRESS_MAPPING
	boaFormDefine("formAddressMap", formAddressMap);		// AddressMap Configuration Form
#ifdef MULTI_ADDRESS_MAPPING
	boaASPDefine("AddressMapList", showMultAddrMappingTable);
#endif //MULTI_ADDRESS_MAPPING
#endif

#ifdef FINISH_MAINTENANCE_SUPPORT
	boaFormDefine("formFinishMaintenance", formFinishMaintenance);		// xl_yue added,inform itms
#endif
#ifdef ACCOUNT_LOGIN_CONTROL
	boaFormDefine("formAdminLogout", formAdminLogout);		// xl_yue added,
	boaFormDefine("formUserLogout", formUserLogout);		// xl_yue added,
#endif
//#ifdef CONFIG_MCAST_VLAN
	boaASPDefine("listWanName", listWanName);
	boaFormDefine("formMcastVlanMapping", formMcastVlanMapping);
//#endif
//added by xl_yue
#ifdef USE_LOGINWEB_OF_SERVER
	boaFormDefine("userLogin", userLogin);					// xl_yue added,
	boaFormDefine("formLogout", formLogout);				// xl_yue added,
	// Kaohj
	boaASPDefine("passwd2xmit", passwd2xmit);
	// davian_kuo
	boaFormDefine("userLoginMultilang", userLoginMultilang);
#ifdef USE_CAPCTHA_OF_LOGINWEB
	boaASPDefine("getCaptchastring", getCaptchastring);
#endif
#endif

	boaFormDefine("formReboot", formReboot);				// Commit/reboot Form

#ifdef CONFIG_USER_LAN_VLAN_TRANSLATE
	boaASPDefine("initPageLanVlan", initPageLanVlan);
	boaFormDefine("formLanVlanSetup", formLanVlanSetup);
#endif

#ifdef CONFIG_USER_DHCP_SERVER
	boaFormDefine("formDhcpMode", formDhcpMode);			// DHCP Mode Configuration Form
#endif

#ifdef CONFIG_USER_IGMPPROXY
	boaFormDefine("formIgmproxy", formIgmproxy);			// IGMP Configuration Form
	boaASPDefine("igmpproxyinit", igmpproxyinit);
#endif
	boaASPDefine("portmirrorinit", portmirrorinit);

	boaASPDefine("user_confirm", user_confirm);
	boaASPDefine("if_wan_list", ifwanList);
#ifdef QOS_TRAFFIC_SHAPING_BY_SSID
	boaASPDefine("ssid_list", ssid_list);
#endif
//#ifdef CONFIG_USER_UPNPD
#if defined(CONFIG_USER_UPNPD)||defined(CONFIG_USER_MINIUPNPD)
	boaFormDefine("formUpnp", formUpnp);					// UPNP Configuration Form
	boaASPDefine("upnpPortFwList", upnpPortFwList);
#endif
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_MLDPROXY
	boaFormDefine("formMLDProxy", formMLDProxy);			// MLD Proxy Configuration Form
	boaASPDefine("mldproxyinit", mldproxyinit);
#endif
	boaFormDefine("formMLDSnooping", formMLDSnooping);		// formIgmpSnooping Configuration Form  // Mason Yu. MLD snooping for e8b
#endif
#ifdef CONFIG_USER_MINIDLNA
	boaASPDefine("fmDMS_checkWrite", fmDMS_checkWrite);
	boaFormDefine("formDMSConf", formDMSConf);
#endif
#ifdef CONFIG_USB_SUPPORT
	boaASPDefine("listUsbDevices", listUsbDevices);
#endif
#if defined(CONFIG_USER_ROUTED_ROUTED) || defined(CONFIG_USER_ZEBRA_OSPFD_OSPFD)
	boaFormDefine("formRip", formRip);						// RIP Configuration Form
#endif
#ifdef CONFIG_USER_ROUTED_ROUTED
	boaASPDefine("showRipIf", showRipIf);
#endif
#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
	boaASPDefine("showOspfIf", showOspfIf);
#endif

#ifdef IP_ACL
	boaFormDefine("formACL", formACL);                  	// ACL Configuration Form
	boaASPDefine("showACLTable", showACLTable);
	boaASPDefine("showLANACLItem", LANACLItem);
	boaASPDefine("showWANACLItem", WANACLItem);
#ifdef CONFIG_GENERAL_WEB
	boaASPDefine("checkAclItems", checkAclItems);
#endif
#ifdef CONFIG_IPV6
	boaFormDefine("formV6ACL", formV6ACL);                  	// IPv6 ACL Configuration Form
	boaASPDefine("showV6ACLTable", showV6ACLTable);
	boaASPDefine("showLANV6ACLItem", LANV6ACLItem);
	boaASPDefine("showWANV6ACLItem", WANV6ACLItem);
#endif
#endif
#ifdef _CWMP_MIB_
	boaASPDefine("showCWMPACLTable", showCWMPACLTable);
#endif
#ifdef NAT_CONN_LIMIT
	boaFormDefine("formConnlimit", formConnlimit);
	boaASPDefine("showConnLimitTable", showConnLimitTable);
#endif
#ifdef TCP_UDP_CONN_LIMIT
	boaFormDefine("formConnlimit", formConnlimit);
	boaASPDefine("connlmitList", showConnLimitTable);

#endif //TCP_UDP_CONN_LIMIT
#ifdef CONFIG_USER_DHCP_SERVER
	boaFormDefine("formmacBase", formmacBase); 		// MAC-Based Assignment for DHCP Server
#ifdef IMAGENIO_IPTV_SUPPORT
	boaFormDefine("formIpRange", formIpRange);
#endif
	boaASPDefine("showMACBaseTable", showMACBaseTable);
#ifdef IMAGENIO_IPTV_SUPPORT
	boaASPDefine("showDeviceIpTable", showDeviceIpTable);
#endif //#ifdef IMAGENIO_IPTV_SUPPORT
#endif

#ifdef URL_BLOCKING_SUPPORT
	boaFormDefine("formURL", formURL);                  // URL Configuration Form
	boaASPDefine("showURLTable", showURLTable);
	boaASPDefine("showKeywdTable", showKeywdTable);
#endif
#ifdef URL_ALLOWING_SUPPORT
       boaASPDefine("showURLALLOWTable", showURLALLOWTable);
#endif



#ifdef DOMAIN_BLOCKING_SUPPORT
	boaFormDefine("formDOMAINBLK", formDOMAINBLK);                  // Domain Blocking Configuration Form
	boaASPDefine("showDOMAINBLKTable", showDOMAINBLKTable);
#endif

#ifdef TIME_ZONE
	boaFormDefine("formNtp", formNtp);
	boaASPDefine("timeZoneList", timeZoneList);
#endif

#ifdef IP_PASSTHROUGH
	boaFormDefine("formOthers", formOthers);	// Others advance Configuration Form
#endif

#ifdef CONFIG_USER_BRIDGE_GROUPING
	boaFormDefine("formBridgeGrouping", formBridgeGrouping);	// Interface grouping Form
	boaASPDefine("itfGrpList", ifGroupList);
#endif
#if defined(CONFIG_USER_INTERFACE_GROUPING)
	boaFormDefine("formInterfaceGrouping", formInterfaceGrouping);	// Interface grouping Form
#endif

#if defined(CONFIG_USER_USP_TR369)
	boaFormDefine("formTR369", formTR369);
	boaASPDefine("showTR369List", showTR369List);
#endif

	//boaFormDefine("formVlanCfg", formVlan);	// Vlan Configuration Form
	//boaASPDefine("vlanPost", vlanPost);
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
#ifdef ELAN_LINK_MODE
	boaFormDefine("formLinkMode", formLink);	// Ethernet Link Form
	boaASPDefine("show_lan", show_lanport);
#endif
#else // of CONFIG_RTL_MULTI_LAN_DEV
#ifdef ELAN_LINK_MODE_INTRENAL_PHY
	boaFormDefine("formLinkMode", formLink);
#endif
#endif	// of CONFIG_RTL_MULTI_LAN_DEV
#ifdef CONFIG_8021P_PRIO
       boaASPDefine("setting1p", setting_1ppriority);
#endif

#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
	boaFormDefine("formPPtP", formPPtP);
	boaASPDefine("pptpList", pptpList);
#endif //end of CONFIG_USER_PPTP_CLIENT_PPTP
#if defined(CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_PPTP_SERVER)
	boaASPDefine("pptpServerList", pptpServerList);
#endif // end of CONFIG_USER_PPTPD_PPTPD
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	boaFormDefine("formL2TP", formL2TP);
	boaASPDefine("lns_name_list", lns_name_list);
	boaASPDefine("l2tpList", l2tpList);
	boaASPDefine("l2tpWithIPSecLAC", l2tpWithIPSecLAC);
	boaASPDefine("l2tpWithIPSecLNS", l2tpWithIPSecLNS);
	boaASPDefine("l2tpWithIPSecLACTable", l2tpWithIPSecLACTable);
#endif //end of CONFIG_USER_L2TPD_L2TPD
#ifdef CONFIG_USER_L2TPD_LNS
	boaASPDefine("l2tpServerList", l2tpServerList);
#endif // end of CONFIG_USER_L2TPD_LNS
//#if defined(CONFIG_USER_BOA_PRO_PASSTHROUGH) && defined(CONFIG_RTK_DEV_AP)
#ifdef CONFIG_USER_BOA_PRO_PASSTHROUGH
    boaASPDefine("VpnPassThrulist", VpnPassThrulist);
    boaFormDefine("formVpnPassThru", formVpnPassThru);
#endif
#ifdef CONFIG_XFRM
#ifdef CONFIG_USER_STRONGSWAN
	boaFormDefine("formIPsecStrongswan", formIPsecStrongswan);
	boaFormDefine("formIPSecSwanCert", formIPSecSwanCert);
	boaFormDefine("formIPSecSwanKey", formIPSecSwanKey);
	boaFormDefine("formIPSecSwanGenCert", formIPSecSwanGenCert);
	boaASPDefine("ipsecSwanConfList", ipsecSwanConfList);
	boaASPDefine("ipsecSwanWanList", ipsecSwanWanList);
	boaASPDefine("ipsecSwanConfDetail", ipsecSwanConfDetail);
#else
	boaFormDefine("formIPsec", formIPsec);
	boaFormDefine("formIPSecCert", formIPSecCert);
	boaFormDefine("formIPSecKey", formIPSecKey);
	boaASPDefine("ipsec_wanList", ipsec_wanList);
	boaASPDefine("ipsec_ikePropList", ipsec_ikePropList);
	boaASPDefine("ipsec_encrypt_p2List", ipsec_encrypt_p2List);
	boaASPDefine("ipsec_auth_p2List", ipsec_auth_p2List);
	boaASPDefine("ipsecAlgoScripts", ipsecAlgoScripts);
	boaASPDefine("ipsec_infoList", ipsec_infoList);
#endif
#endif
#ifdef CONFIG_NET_IPIP
	boaFormDefine("formIPIP", formIPIP);
	boaASPDefine("ipipList", ipipList);
#endif//end of CONFIG_NET_IPIP
#if CONFIG_NET_IPGRE
	boaFormDefine("formGRE", formGRE);
	boaASPDefine("showGRETable", showGRETable);
#endif

#ifdef REMOTE_ACCESS_CTL
	boaFormDefine("formSAC", formAcc);	// Services Access Control
	boaASPDefine("accPost", accPost);
	boaASPDefine("rmtaccItem", accItem);
#endif
	//boaASPDefine("autopvcStatus", autopvcStatus);	// auto-pvc search
	//boaASPDefine("showPVCList", showPVCList);	// auto-pvc search
	boaASPDefine("ShowAutoPVC", ShowAutoPVC);	// auto-pvc search
	boaASPDefine("ShowChannelMode", ShowChannelMode);	// China Telecom jim...
	boaASPDefine("ShowBridgeMode", ShowBridgeMode);	// For PPPoE pass through
#if defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN) || defined(WLAN_WISP)
	boaASPDefine("initPageWaneth", initPageWaneth);
#endif
	boaASPDefine("ShowPPPIPSettings", ShowPPPIPSettings);	// China Telecom jim...
#ifdef CONFIG_00R0
	boaASPDefine("ShowPPPIPSettings_admin", ShowPPPIPSettings_admin);
#endif
	boaASPDefine("ShowWanPortSetting", ShowWanPortSetting);
	boaASPDefine("ShowNAPTSetting", ShowNAPTSetting);	// China Telecom jim...
#ifdef CONFIG_USER_RTK_WAN_CTYPE
	boaASPDefine("ShowConnectionType", ShowConnectionType);
	boaASPDefine("ShowConnectionTypeForBridge", ShowConnectionTypeForBridge);
	boaASPDefine("ShowConnectionTypeForUser", ShowConnectionTypeForUser);
#endif
	boaASPDefine("ShowMacClone", ShowMacClone);
	boaASPDefine("ShowIpProtocolType", ShowIpProtocolType);
	boaASPDefine("ShowIPV6Settings", ShowIPV6Settings);
	boaASPDefine("ShowPortMapping", ShowPortMapping);
	boaASPDefine("ShowPortBaseFiltering", ShowPortBaseFiltering);
    boaASPDefine("Show6rdSetting", Show6rdSetting);
	boaASPDefine("Show6in4Setting", Show6in4Setting);
	boaASPDefine("Show6to4Setting", Show6to4Setting);
	boaASPDefine("Showv6inv4TunnelSetting", Showv6inv4TunnelSetting);
#ifdef WLAN_WISP
	boaASPDefine("ShowWispWanItf", ShowWispWanItf);
	boaASPDefine("initWispWanItfStatus", initWispWanItfStatus);
#endif

	boaASPDefine("ShowIGMPSetting", ShowIGMPSetting);
	boaASPDefine("ShowMLDSetting", ShowMLDSetting);
#ifdef CONFIG_00R0
	boaASPDefine("ShowRIPv2Setting", ShowRIPv2Setting);
#endif

#ifdef CONFIG_USER_IP_QOS
	boaFormDefine("formQosPolicy", formQosPolicy);
	boaASPDefine("initQueuePolicy", initQueuePolicy);
	boaFormDefine("formQosRule",formQosRule);
	boaFormDefine("formQosRuleEdit",formQosRuleEdit);
	boaASPDefine("initQosRulePage",initQosRulePage);
	boaASPDefine("initRulePriority",initRulePriority);
	boaASPDefine("initQosLanif",initQosLanif);
	boaASPDefine("initOutif",initOutif);
	boaASPDefine("initTraffictlPage",initTraffictlPage);
	boaFormDefine("formQosTraffictl",formQosTraffictl);
	boaFormDefine("formQosTraffictlEdit",formQosTraffictlEdit);
	boaASPDefine("ifWanList_tc", ifWanList_tc);
	boaASPDefine("ipqos_dhcpopt", ipqos_dhcpopt);
	boaASPDefine("ipqos_dhcpopt_getoption60", ipqos_dhcpopt_getoption60);
	boaASPDefine("initEthTypes",initEthTypes);
	boaASPDefine("initProtocol",initProtocol);
#ifdef CONFIG_00R0
	boaFormDefine("formTrafficShaping", formTrafficShaping);
#endif
#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
	boaASPDefine("initQosSpeedLimitLanif",initQosSpeedLimitLanif);
	boaASPDefine("initQosSpeedLimitRule",initQosSpeedLimitRule);
	boaFormDefine("formQosSpeedLimit", formQosSpeedLimit);
#endif
#endif
#ifdef WLAN_SUPPORT
	//for WLAN enable/disable web control
	boaASPDefine("wlanStatus", wlanStatus);
	boaASPDefine("SSID_select", wlan_ssid_select);
	boaASPDefine("getwlanencrypt", getwlanencrypt);
#if defined(WLAN_DUALBAND_CONCURRENT)
	boaFormDefine("formWlanRedirect", formWlanRedirect);
#endif
	boaFormDefine("formWlanSetup", formWlanSetup);
#ifdef WLAN_ACL
	boaASPDefine("wlAcList", wlAcList);
#endif
	boaASPDefine("wirelessClientList", wirelessClientList);
	boaFormDefine("formWirelessTbl", formWirelessTbl);

#ifdef WLAN_ACL
	boaFormDefine("formWlAc", formWlAc);
#endif
	boaFormDefine("formAdvanceSetup", formAdvanceSetup);
#ifdef WLAN_RTIC_SUPPORT
	boaFormDefine("formWlRtic", formWlRtic);
#endif
#ifdef WLAN_WPA
	boaFormDefine("formWlEncrypt", formWlEncrypt);
#endif
#ifdef WLAN_WDS
	boaFormDefine("formWlWds", formWlWds);
	boaFormDefine("formWdsEncrypt", formWdsEncrypt);
	boaASPDefine("wlWdsList", wlWdsList);
	boaASPDefine("wdsList", wdsList);
#endif
#if defined(WLAN_CLIENT) || defined(WLAN_SITESURVEY)
	boaFormDefine("formWlSiteSurvey", formWlSiteSurvey);
	boaASPDefine("wlSiteSurveyTbl",wlSiteSurveyTbl);
#endif

#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS) //WPS
	boaFormDefine("formWsc", formWsc);
#endif
#ifdef WLAN_11R
	boaFormDefine("formFt", formFt);
	boaASPDefine("wlFtKhList", wlFtKhList);
#endif
	boaASPDefine("wlStatus_parm", wlStatus_parm);
#ifdef WLAN_MESH
	boaFormDefine("formMeshSetup", formMeshSetup);
#ifdef WLAN_MESH_ACL_ENABLE
	boaFormDefine("formMeshACLSetup", formMeshACLSetup);
	boaASPDefine("wlMeshAcList", wlMeshAcList);
#endif
	boaASPDefine("wlMeshNeighborTable", wlMeshNeighborTable);
	boaASPDefine("wlMeshRoutingTable", wlMeshRoutingTable);
	boaASPDefine("wlMeshPortalTable", wlMeshPortalTable);
	boaASPDefine("wlMeshProxyTable", wlMeshProxyTable);
	boaASPDefine("wlMeshRootInfo", wlMeshRootInfo);
#endif

#ifdef SUPPORT_FON_GRE
	boaFormDefine("formFonGre", formFonGre);
#endif
#ifdef RTK_MULTI_AP
	boaFormDefine("formMultiAP", formMultiAP);
#ifdef BACKHAUL_LINK_SELECTION
	boaASPDefine("showBackhaulSelection", showBackhaulSelection);
#endif
#ifdef RTK_MULTI_AP_R2
	boaFormDefine("formMultiAPVLAN", formMultiAPVLAN);
	boaFormDefine("formChannelScan", formChannelScan);
	boaASPDefine("showChannelScanResults",showChannelScanResults);
#endif
#endif
#ifdef WLAN_SCHEDULE_SUPPORT
	boaFormDefine("formNewSchedule", formNewSchedule);
	boaASPDefine("wlSchList", wlSchList);
#endif
#endif	// of WLAN_SUPPORT

#ifdef CONFIG_DEV_xDSL
	boaASPDefine("oamSelectList", oamSelectList);
#endif
#ifdef DIAGNOSTIC_TEST
	boaFormDefine("formDiagTest", formDiagTest);	// Diagnostic test
	boaASPDefine("lanTest", lanTest);	// Ethernet LAN connection test
	boaASPDefine("adslTest", adslTest);	// ADSL service provider connection test
	boaASPDefine("internetTest", internetTest);	// Internet service provider connection test
#endif
#ifdef DOS_SUPPORT
	boaFormDefine("formDosCfg", formDosCfg);
#endif
#ifdef WLAN_MBSSID
	boaASPDefine("SSIDStr", SSIDStr);
	boaASPDefine("getVirtualIndex", getVirtualIndex);
	boaFormDefine("formWlanMultipleAP", formWlanMultipleAP);
	boaASPDefine("wirelessVAPClientList", wirelessVAPClientList);
	boaFormDefine("formWirelessVAPTbl", formWirelessVAPTbl);
#endif

#ifdef PORT_FORWARD_ADVANCE
	boaFormDefine("formPFWAdvance", formPFWAdvance);
	boaASPDefine("showPFWAdvTable", showPFWAdvTable);
#endif
#ifdef PORT_FORWARD_GENERAL
	boaASPDefine("showPFWAdvForm", showPFWAdvForm);
#endif

/*
 *	Create the Form handlers for the User Management pages
 */
#ifdef USER_MANAGEMENT_SUPPORT
	boaFormDefineUserMgmt();
#endif
//add by ramen for ALG on-off
#ifdef CONFIG_USER_RTK_NAT_ALG_PASS_THROUGH
	boaFormDefine("formALGOnOff", formALGOnOff);
#endif
/*+++++add by Jack for VoIP project 20/03/07+++++*/
#ifdef VOIP_SUPPORT
	web_voip_init();
#endif /*VOIP_SUPPORT*/
/*-----end-----*/
#ifdef WEB_ENABLE_PPP_DEBUG
boaASPDefine("ShowPPPSyslog", ShowPPPSyslog); // Jenny, show PPP debug to syslog
#endif
#ifdef CONFIG_DEV_xDSL
	boaASPDefine("DSLuptime", DSLuptime);
#ifdef CONFIG_USER_XDSL_SLAVE
	boaASPDefine("DSLSlvuptime", DSLSlvuptime);
#endif /*CONFIG_USER_XDSL_SLAVE*/
#endif
	boaASPDefine("multilang", multilang_asp);
	boaASPDefine("WANConditions", WANConditions);
	boaASPDefine("ShowWanMode", ShowWanMode);

#ifdef CONFIG_USER_SAMBA
	boaFormDefine("formSamba", formSamba);
	boaFormDefine("formSambaAccount", formSambaAccount);
	boaASPDefine("showSambaAccount", showSambaAccount);
#endif

#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_AWIFI_SUPPORT)
	boaFormDefine("formAWiFi", formAWiFi);
	boaFormDefine("formAWiFiMAC", formAWiFiMAC);
	boaFormDefine("formAWiFiURL", formAWiFiURL);
#ifdef CONFIG_USER_AWIFI_AUDIT_SUPPORT
	boaFormDefine("formAWiFiAudit", formAWiFiAudit);
#endif

	boaASPDefine("showAWiFiMACTable", showAWiFiMACTable);
	boaASPDefine("showAWiFiURLTable", showAWiFiURLTable);
#endif

#ifdef CONFIG_INIT_SCRIPTS
	boaFormDefine("formInitStartScript", formInitStartScript);
	boaFormDefine("formInitStartScriptDel", formInitStartScriptDel);
	boaFormDefine("formInitEndScript", formInitEndScript);
	boaFormDefine("formInitEndScriptDel", formInitEndScriptDel);
#endif
#ifdef CONFIG_USER_DOT1AG_UTILS
	boaASPDefine("dot1ag_init", dot1ag_init);
	boaFormDefine("formDot1agConf", formDot1agConf);
	boaFormDefine("formDot1agAction", formDot1agAction);
	boaFormDefine("dot1agActionRefresh", dot1agActionRefresh);
	boaASPDefine("dot1ag_status_init", dot1ag_status_init);
#endif

#ifdef OSGI_SUPPORT
	boaASPDefine("getOSGIInfo", getOSGIInfo);
	boaASPDefine("getOSGIBundleList", getOSGIBundleList);
	boaFormDefine("formOsgiUpload", formOsgiUpload);
	boaFormDefine("formOsgiMgt", formOsgiMgt);
#endif

#if defined(CONFIG_USER_Y1731) || defined(CONFIG_USER_8023AH)
	boaFormDefine("formY1731", formY1731);
	boaASPDefine("getLinktraceMac", getLinktraceMac);
#endif

	boaFormDefine("formVersionMod",formVersionMod);
	boaFormDefine("formExportOMCIlog", formExportOMCIlog);
	boaFormDefine("formImportOMCIShell", formImportOMCIShell);

#ifdef USER_WEB_WIZARD
	boaFormDefine("form2WebWizardMenu", formWebWizardMenu);
	boaFormDefine("form2WebWizard1", formWebWizard1);
	boaFormDefine("form2WebWizard4", formWebWizard4);
	boaFormDefine("form2WebWizard5", formWebWizard5);
	boaFormDefine("form2WebWizard6", formWebWizard6);
	boaASPDefine("ShowWebWizardPage", ShowWebWizardPage);
#endif

#ifdef CONFIG_00R0
/* siyuan 2016-01-25 add wizard screen functions */
	boaASPDefine("getInitUrl", getInitUrl);
	boaFormDefine("formWizardScreen0_1", formWizardScreen0_1);
	boaASPDefine("initWizardScreen1_1", initWizardScreen1_1);
	boaFormDefine("formWizardScreen1_1", formWizardScreen1_1);
	boaFormDefine("formWizardScreen1_2", formWizardScreen1_2);
	boaASPDefine("initWizardScreen4_1", initWizardScreen4_1);
	boaASPDefine("initWizardScreen4_1_1", initWizardScreen4_1_1);
	boaFormDefine("formWizardScreen4_1_1", formWizardScreen4_1_1);
	boaASPDefine("initWizardScreen4_2", initWizardScreen4_2);
	boaASPDefine("postWizardScreen4_2", postWizardScreen4_2);
	boaFormDefine("formWizardScreen4_2", formWizardScreen4_2);
	boaFormDefine("formWizardScreen5_1", formWizardScreen5_1);
	boaASPDefine("initWizardScreen5_2", initWizardScreen5_2);
	boaFormDefine("formWizardScreen5_2", formWizardScreen5_2);
	boaASPDefine("getTroubleInitUrl", getTroubleInitUrl);
/* 2019 version wizard */
	boaASPDefine("getInitUrl_2019", getInitUrl_2019);
	boaASPDefine("initWizardScreen1_2019", initWizardScreen1_2019);
	boaASPDefine("initWizardScreen1_1_2019", initWizardScreen1_1_2019);
	boaASPDefine("initWizardScreen2_2_2019", initWizardScreen2_2_2019);
	boaFormDefine("formWizardScreen0_1_2019", formWizardScreen0_1_2019);
	boaFormDefine("formWizardScreen1_2019", formWizardScreen1_2019);
	boaFormDefine("formWizardScreen1_1_2019", formWizardScreen1_1_2019);
	boaFormDefine("formWizardScreen2_1_2019", formWizardScreen2_1_2019);
	boaFormDefine("formWizardScreen2_2_2019", formWizardScreen2_2_2019);
#endif
#ifdef _PRMT_X_CT_COM_MWBAND_
	boaASPDefine("initClientLimit", initClientLimit);
	boaFormDefine("formClientLimit", formClientLimit);
#endif
	// Added by davian kuo
	boaFormDefine("langSel", langSel);

#ifdef CONFIG_USER_RTK_BRIDGE_MODE
	boaFormDefine("formInternetMode", formInternetMode);
#endif
#ifdef CONFIG_USER_RTK_LBD
	boaASPDefine("initLBDPage", initLBDPage);
	boaFormDefine("formLBD",formLBD);
#endif
#ifdef CONFIG_ELINK_SUPPORT
	boaFormDefine("formElinkSetup", formElinkSetup);
#ifdef CONFIG_ELINKSDK_SUPPORT
	boaFormDefine("formElinkSDKUpload", formElinkSDKUpload);
#endif
#endif
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
	boaFormDefine("formStaticBalance", formStaticBalance);
	boaFormDefine("formDynmaicBalance", formDynmaicBalance);
	boaASPDefine("sta_balance_list", sta_balance_list);
	boaASPDefine("rtk_set_connect_load_balance", rtk_set_connect_load_balance);
	boaASPDefine("rtk_set_bandwidth_load_balance", rtk_set_bandwidth_load_balance);
	boaFormDefine("formLdStats", formLdStats);
#if defined(CONFIG_USER_LANNETINFO) && defined(CONFIG_USER_REFINE_CHECKALIVE_BY_ARP)
	boaASPDefine("rtk_set_lan_sta_load_balance", rtk_set_lan_sta_load_balance);
	boaASPDefine("LoadBalanceStatsList", LoadBalanceStatsList);
	boaASPDefine("wan_stats_show", wan_stats_show);
#endif

	boaASPDefine("rtk_load_balance_wan_nums", rtk_load_balance_wan_nums);
	boaASPDefine("rtk_load_balance_actual_wans", rtk_load_balance_actual_wans);
	boaASPDefine("rtk_load_balance_wan_info", rtk_load_balance_wan_info);
	boaASPDefine("rtk_load_balance_chart_header_info", rtk_load_balance_chart_header_info);
	boaFormDefine("formLdStatsChart_demo",formLdStatsChart_demo);
#endif
	boaASPDefine("ShowWanPortRateSetting", ShowWanPortRateSetting);

#ifdef CONFIG_USER_MAP_E
	boaFormDefine("formMAPE_FMR_Edit", formMAPE_FMR_Edit);
#endif

#ifdef USE_DEONET /* sghong, 20230511 */
	boaFormDefine("formAccessLimit", formAccessLimit);
	boaASPDefine("showCurrentTime", showCurrentTime);
	boaASPDefine("hourTimeSet", hourTimeSet);
	boaASPDefine("minTimeSet", minTimeSet);
	boaASPDefine("getConnectHostList", getConnectHostList);
	boaASPDefine("showCurrentAccessList", showCurrentAccessList);

	boaFormDefine("formAutoReboot", formAutoReboot);
	boaASPDefine("showCurrentAutoRebootConfigInfo", showCurrentAutoRebootConfigInfo);
	boaASPDefine("ShowManualSetting", ShowManualSetting);
	boaASPDefine("ShowAutoSetting", ShowAutoSetting);
	boaASPDefine("ShowAutoRebootDay", ShowAutoRebootDay);
	boaASPDefine("ShowAutoWanPortIdleValue", ShowAutoWanPortIdleValue);
	boaASPDefine("ShowRebootTargetTime", ShowRebootTargetTime);

	boaFormDefine("formNatSessionSetup", formNatSessionSetup);	// NAT session Setting Form
	boaASPDefine("ShowNatSessionCount", ShowNatSessionCount);
	boaASPDefine("ShowNatSessionThreshold", ShowNatSessionThreshold);

	boaFormDefine("formUseageSetup", formUseageSetup);	// USEAGE cpu & memory Form
	boaASPDefine("ShowUseageThreshold", ShowUseageThreshold);

	boaASPDefine("nvramGet", nvramGet);
	boaASPDefine("portCrcCounterGet", portCrcCounterGet);

	boaFormDefine("formSelfCheck", formSelfCheck);	// self check form
	boaASPDefine("ShowPortStatus", ShowPortStatus);
	boaASPDefine("ShowDaemonCheckStatus", ShowDaemonCheckStatus);
	boaASPDefine("ShowGatewayDnsCheckStatus", ShowGatewayDnsCheckStatus);

	boaASPDefine("pwmacinit", pwmacinit); // get to default pw
	boaFormDefine("formNickname", formNickname);	// nickname
	boaASPDefine("ShowNicknameTable", ShowNicknameTable);
	boaASPDefine("ShowAutoRebootTargetTime", ShowAutoRebootTargetTime);

	boaASPDefine("sshPortGet", sshPortGet);
#endif
}

#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_RADVD
void initRadvdConfPage(request * wp)
{
	unsigned char vChar;

	mib_get_s( MIB_V6_PREFIX_ENABLE, (void *)&vChar, sizeof(vChar));
	if( vChar == 1)
	{
		mib_get_s(MIB_V6_RADVD_PREFIX_MODE, (void *)&vChar, sizeof(vChar));
		boaWrite(wp, "%s.radvd.PrefixMode.value = %d;\n", DOCUMENT, vChar);

		mib_get_s(MIB_V6_ULAPREFIX_ENABLE, (void *)&vChar, sizeof(vChar));
		if (vChar == 0){
			boaWrite(wp, "%s.radvd.EnableULA[0].checked = true;\n", DOCUMENT);
		}else{
			boaWrite(wp, "%s.radvd.EnableULA[1].checked = true;\n", DOCUMENT);
		}

		mib_get_s(MIB_V6_ULAPREFIX_RANDOM, (void *)&vChar, sizeof(vChar));
		if (vChar)	{
			boaWrite(wp, "%s.radvd.ULAPrefixRandom.checked=true;\n", DOCUMENT);
		}else{
			boaWrite(wp, "%s.radvd.ULAPrefixRandom.checked=false;\n", DOCUMENT);
		}
		/* For RDNSS mode init */
		mib_get_s( MIB_V6_RADVD_RDNSS_MODE, (void *)&vChar, sizeof(vChar));
		boaWrite(wp, "%s.radvd.RDNSSMode.value = %d;\n", DOCUMENT, vChar);
	}
}
#endif
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
void initIPv6AdvPage(request * wp)
{
	unsigned char vChar;
	if(mib_get_s( MIB_V6_LOGO_TEST_MODE, (void *)&vChar, sizeof(vChar))){
		boaWrite(wp, "%s.ipv6enabledisable.logoTestMode.value = %d;\n", DOCUMENT, vChar);
	}
}
#endif
#endif

void initLanPage(request * wp, char priv)
{
	unsigned char vChar;
#ifdef CONFIG_SECONDARY_IP
	char dhcp_pool;
#endif

#ifdef CONFIG_SECONDARY_IP
	mib_get_s( MIB_ADSL_LAN_ENABLE_IP2, (void *)&vChar, sizeof(vChar));
	if (vChar!=0) {
		//boaWrite(wp, "%s.tcpip.enable_ip2.value = 1;\n", DOCUMENT);
		boaWrite(wp, "%s.tcpip.enable_ip2.checked = true;\n", DOCUMENT);
	}
	#ifndef DHCPS_POOL_COMPLETE_IP
	mib_get_s(MIB_ADSL_LAN_DHCP_POOLUSE, (void *)&dhcp_pool, sizeof(dhcp_pool));
	boaWrite(wp, "%s.tcpip.dhcpuse[%d].checked = true;\n", DOCUMENT, dhcp_pool);
	#endif
	boaWrite(wp, "updateInput();\n");
#endif

#if defined(CONFIG_RTL_IGMP_SNOOPING) || defined(CONFIG_BRIDGE_IGMP_SNOOPING)
	mib_get_s( MIB_MPMODE, (void *)&vChar, sizeof(vChar));
	// bitmap for virtual lan port function
	// Port Mapping: bit-0
	// QoS : bit-1
	// IGMP snooping: bit-2
	if (priv)
		boaWrite(wp, "%s.tcpip.snoop[%d].checked = true;\n", DOCUMENT, (vChar>>MP_IGMP_SHIFT)&0x01);
#ifdef CONFIG_IGMP_FORBID
	mib_get_s( MIB_IGMP_FORBID_ENABLE, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.tcpip.igmpforbid[%d].checked = true;\n", DOCUMENT, vChar);
#endif
#endif

#ifdef WLAN_SUPPORT
	mib_get_s( MIB_WLAN_BLOCK_ETH2WIR, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.tcpip.BlockEth2Wir[%d].checked = true;\n", DOCUMENT, vChar==0?0:1);
#endif

#ifdef CONFIG_USER_LLMNR
	mib_get_s( MIB_LLMNR_ENABLE, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.tcpip.llmnr[%d].checked = true;\n", DOCUMENT, vChar==0?0:1);
#endif

#ifdef CONFIG_USER_MDNS
	mib_get_s( MIB_MDNS_ENABLE, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.tcpip.mdns[%d].checked = true;\n", DOCUMENT, vChar==0?0:1);
#endif

#if defined(CONFIG_LUNA) && defined(CONFIG_RTL_MULTI_LAN_DEV)
	int i = 0;
	MIB_CE_SW_PORT_T lan_entry;
	for(i = 0 ; i < SW_LAN_PORT_NUM ; i++)
	{
		if(mib_chain_get(MIB_SW_PORT_TBL, i, (void *)&lan_entry) == 0)
		{
			fprintf(stderr, "<%s:%d> Get MIB_SW_PORT_TBL.%d failed\n", __func__, __LINE__, i);
			continue;
		}
		boaWrite(wp, "%s.tcpip.lan%d[%d].checked = true;\n", DOCUMENT, i, lan_entry.enable);
#ifdef USE_DEONET /* sghong, 20221123 */
		boaWrite(wp, "%s.tcpip.lan%d_fc[%d].checked = true;\n", DOCUMENT, i, lan_entry.flowcontrol);
#endif
		boaWrite(wp, "%s.tcpip.lan%d_GL[%d].checked = true;\n", DOCUMENT, i, lan_entry.gigalite);
	}
#endif
}

#ifdef _CWMP_MIB_
void initTr069ConfigPage(request * wp){
#ifdef _TR111_STUN_
	boaWrite(wp, "\t\t\tdocument.getElementById('TR111_STUN').style.display = 'block';\n");
#else
	boaWrite(wp, "\t\t\tdocument.getElementById('TR111_STUN').style.display = 'none';\n");
#endif	/* _TR111_STUN_ */
}
#endif

// Mason Yu. MLD snooping
#ifdef CONFIG_IPV6
void initMLDsnoopPage(request * wp)
{
	unsigned char vChar;
#if defined(CONFIG_RTL_MLD_SNOOPING) || defined(CONFIG_BRIDGE_IGMP_SNOOPING)
	mib_get_s( MIB_MPMODE, (void *)&vChar, sizeof(vChar));
	// bitmap for virtual lan port function
	// Port Mapping: bit-0
	// QoS : bit-1
	// IGMP snooping: bit-2
	// MLD snooping: bit-3
	boaWrite(wp, "%s.mldsnoop.snoop[%d].checked = true;\n", DOCUMENT, (vChar>>MP_MLD_SHIFT)&0x01);
#endif
}
#endif

#ifdef CONFIG_DEV_xDSL
#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
struct initGfast_t
{
	unsigned short	profile;
	unsigned char	*name;
	unsigned char	*tagname;
};

static struct initGfast_t initGfast[]=
{
{GFAST_PROFILE_106A,	"106a",	"GFast_106a"},
{GFAST_PROFILE_212A,	"212a",	"GFast_212a"},
{GFAST_PROFILE_106B,	"106b",	"GFast_106b"},
{GFAST_PROFILE_106C,	"106c",	"GFast_106c"},
{GFAST_PROFILE_212C,	"212c",	"GFast_212c"},
{GFAST_PROFILE_212B,	"212b",	"GFast_212b"},
};

#define initGfast_listnum (sizeof(initGfast)/sizeof(struct initGfast_t))

void initGFast_check(request * wp)
{
	boaWrite(wp, "\t && document.set_adsl.gfast.checked == false\n");
}

void initGFast_check_profile(request * wp)
{
	int i;
	unsigned long pfCap;

//#ifdef CHECK_XDSL_ONE_MODE
//	initVD2_check_xdsl_one_mode(wp);
//#endif /*CHECK_XDSL_ONE_MODE*/

#if 0	//wait get profile capacity function
	if(!d->xdsl_msg_get(UserGetGfastSupportProfile, &pfCap)){
		printf("[%s] check G. Fast support profile fail\n", __FUNCTION__);
		return;
	}
#else
	pfCap = 0xff;
#endif

	boaWrite(wp, "if(document.set_adsl.gfast.checked == true)\n");
	boaWrite(wp, "{\n");
	boaWrite(wp, "	if(\n");
	for(i=0; i<initGfast_listnum; i++)
	{
		if (!(initGfast[i].profile&pfCap))
			continue;
		if(i) boaWrite(wp, "&&");
		boaWrite(wp, "(document.set_adsl.%s.checked==false)\n", initGfast[i].tagname );
	}
	boaWrite(wp, "	){\n");
	boaWrite(wp, "		alert(\"G. Fast Profile cannot be empty.\");\n");
	boaWrite(wp, "		return false;\n");
	boaWrite(wp, "	}\n");
	boaWrite(wp, "}\n");
}

void initGFast_updatefn(request * wp)
{
	int i;
	unsigned long pfCap;

//#ifdef CHECK_XDSL_ONE_MODE
//	initVD2_check_xdsl_one_mode(wp);
//#endif /*CHECK_XDSL_ONE_MODE*/

#if 0	//wait get profile capacity function
	if(!d->xdsl_msg_get(UserGetGfastSupportProfile, &pfCap)){
		printf("[%s] check G. Fast support profile fail\n", __FUNCTION__);
		return;
	}
#else
	pfCap = 0xff;
#endif

	boaWrite(wp, "function updateGfast()\n{\n");
	boaWrite(wp, "	var Gfastdis;\n\n");
	boaWrite(wp, "	if(document.set_adsl.gfast.checked==true)\n");
	boaWrite(wp, "		Gfastdis=false;\n");
	boaWrite(wp, "	else\n");
	boaWrite(wp, "		Gfastdis=true;\n\n");
	for(i=0; i<initGfast_listnum; i++)
	{
		if (!(initGfast[i].profile&pfCap))
			continue;
		boaWrite(wp, "	document.set_adsl.%s.disabled=Gfastdis;\n", initGfast[i].tagname );
	}
	boaWrite(wp, "}\n");
}

void initGFast_opt(request * wp)
{
	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "	<th></th>\n");
	boaWrite(wp, "	<td><font size=2><input type=checkbox name=gfast value=1 ONCLICK=updateGfast()>G. Fast</td>\n");
	boaWrite(wp, "</tr>\n");
}

void initGFast_profile(request * wp)
{
	int i;
	unsigned long pfCap;

//#ifdef CHECK_XDSL_ONE_MODE
//	initVD2_check_xdsl_one_mode(wp);
//#endif /*CHECK_XDSL_ONE_MODE*/

#if 0	//wait get profile capacity function
	if(!d->xdsl_msg_get(UserGetGfastSupportProfile, &pfCap)){
		printf("[%s] check G. Fast support profile fail\n", __FUNCTION__);
		return;
	}
#else
	pfCap = 0xff;
#endif

	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "	<th align=left width=30%%><font size=2>G.fast Profile:</th>\n");
	boaWrite(wp, "	<td width=70%%></td>\n");
	boaWrite(wp, "</tr>\n");
	for(i=0; i<initGfast_listnum; i++)
	{
//		if (!(initGfast[i].profile&pfCap))
//			continue;
		boaWrite(wp, "<tr>\n");
		boaWrite(wp, "	<th></th>\n");
		boaWrite(wp, "	<td><font size=2><input type=checkbox name=%s value=1>%s</td>\n",
				initGfast[i].tagname, initGfast[i].name);
		boaWrite(wp, "</tr>\n");
	}
}
#endif /* CONFIG_USER_CMD_SUPPORT_GFAST */

#ifdef CONFIG_VDSL
struct initvd2p_t
{
	unsigned short	profile;
	unsigned char	*name;
	unsigned char	*tagname;
};

static struct initvd2p_t initvd2p[]=
{
{VDSL2_PROFILE_8A,	"8a",	"vdsl2p8a"},
{VDSL2_PROFILE_8B,	"8b",	"vdsl2p8b"},
{VDSL2_PROFILE_8C,	"8c",	"vdsl2p8c"},
{VDSL2_PROFILE_8D,	"8d",	"vdsl2p8d"},
{VDSL2_PROFILE_12A,	"12a",	"vdsl2p12a"},
{VDSL2_PROFILE_12B,	"12b",	"vdsl2p12b"},
{VDSL2_PROFILE_17A,	"17a",	"vdsl2p17a"},
{VDSL2_PROFILE_30A,	"30a",	"vdsl2p30a"},
{VDSL2_PROFILE_35B,	"35b",	"vdsl2p35b"},
};

#define initvd2p_listnum (sizeof(initvd2p)/sizeof(struct initvd2p_t))


void initVD2_check(request * wp)
{
	boaWrite(wp, "\t && document.set_adsl.vdsl2.checked == false\n");
}


//#define CHECK_XDSL_ONE_MODE

#ifdef CHECK_XDSL_ONE_MODE
void initVD2_check_xdsl_one_mode(request * wp)
{
	boaWrite(wp, "if((document.set_adsl.glite.checked == true\n");
	boaWrite(wp, "     || document.set_adsl.gdmt.checked == true\n");
	boaWrite(wp, "     || document.set_adsl.t1413.checked == true\n");
	boaWrite(wp, "     || document.set_adsl.adsl2.checked == true\n");
	boaWrite(wp, "     || document.set_adsl.adsl2p.checked == true)\n");
	boaWrite(wp, "  &&(document.set_adsl.vdsl2.checked == true)){\n");
	boaWrite(wp, "	alert(\"you can't select BOTH ADSL and VDSL!\");\n");
	boaWrite(wp, "	return false;\n");
	boaWrite(wp, "}\n");
}
#endif /*CHECK_XDSL_ONE_MODE*/



void initVD2_check_profile(request * wp)
{
	int i;
	XDSL_OP *d;
	unsigned int pfCap;

#ifdef CHECK_XDSL_ONE_MODE
	initVD2_check_xdsl_one_mode(wp);
#endif /*CHECK_XDSL_ONE_MODE*/

	d=xdsl_get_op(0);
	pfCap = 0;
	d->xdsl_msg_get(GetVdslProfileCap, &pfCap);
	if (pfCap == 0) pfCap = 0xff; // compatible with old driver
	boaWrite(wp, "if(document.set_adsl.vdsl2.checked == true)\n");
	boaWrite(wp, "{\n");
	boaWrite(wp, "	if(\n");
	for(i=0; i<initvd2p_listnum; i++)
	{
		if (!(initvd2p[i].profile&pfCap))
			continue;
		if(i) boaWrite(wp, "&&");
		boaWrite(wp, "(document.set_adsl.%s.checked==false)\n", initvd2p[i].tagname );
	}
	boaWrite(wp, "	){\n");
	boaWrite(wp, "		alert(\"VDSL2 Profile cannot be empty.\");\n");
	boaWrite(wp, "		return false;\n");
	boaWrite(wp, "	}\n");
	boaWrite(wp, "}\n");
}

void initVD2_updatefn(request * wp)
{
	int i;
	XDSL_OP *d;
	unsigned int pfCap;

	d=xdsl_get_op(0);
	pfCap = 0;
	d->xdsl_msg_get(GetVdslProfileCap, &pfCap);
	if (pfCap == 0) pfCap = 0xff; // compatible with old driver
	boaWrite(wp, "function updateVDSL2()\n{\n");
	boaWrite(wp, "	var vd2dis;\n\n");
	boaWrite(wp, "	if(document.set_adsl.vdsl2.checked==true)\n");
	boaWrite(wp, "		vd2dis=false;\n");
	boaWrite(wp, "	else\n");
	boaWrite(wp, "		vd2dis=true;\n\n");
	boaWrite(wp, "	document.set_adsl.gvec.disabled=vd2dis;\n");
	for(i=0; i<initvd2p_listnum; i++)
	{
		if (!(initvd2p[i].profile&pfCap))
			continue;
		boaWrite(wp, "	document.set_adsl.%s.disabled=vd2dis;\n", initvd2p[i].tagname );
	}
	boaWrite(wp, "}\n");
}

void initVD2_opt(request * wp)
{
	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "	<th></th>\n");
	boaWrite(wp, "	<td><font size=2><input type=checkbox name=vdsl2 value=1 ONCLICK=updateVDSL2()>VDSL2</td>\n");
	boaWrite(wp, "</tr>\n");
}


void initVD2_profile(request * wp)
{
	int i;
	XDSL_OP *d;
	unsigned int pfCap;

	d=xdsl_get_op(0);
	pfCap = 0;
	d->xdsl_msg_get(GetVdslProfileCap, &pfCap);
	if (pfCap == 0) pfCap = 0xff; // compatible with old driver
	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "	<th align=left width=30%%><font size=2>VDSL2 Profile:</th>\n");
	boaWrite(wp, "	<td width=70%%></td>\n");
	boaWrite(wp, "</tr>\n");
	for(i=0; i<initvd2p_listnum; i++)
	{
		if (!(initvd2p[i].profile&pfCap))
			continue;
		boaWrite(wp, "<tr>\n");
		boaWrite(wp, "	<th></th>\n");
		boaWrite(wp, "	<td><font size=2><input type=checkbox name=%s value=1>%s</td>\n",
				initvd2p[i].tagname, initvd2p[i].name);
		boaWrite(wp, "</tr>\n");
	}
}
#endif /*CONFIG_VDSL*/


void init_adsl_tone_mask(request * wp)
{
#ifndef CONFIG_VDSL
	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "	<th align=left><font size=2>ADSL %s:</th>\n", multilang(LANG_TONE_MASK) );
	boaWrite(wp, "	<td></td>\n");
	boaWrite(wp, "</tr>\n");
	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "	<th></th>\n");
	boaWrite(wp, "	<td><font size=2><input type=\"button\" value=\"%s\" name=\"adsltoneTbl\" onClick=\"adsltoneClick('/adsltone.asp')\"></td>\n", multilang(LANG_TONE_MASK) );
	boaWrite(wp, "</tr>\n");
#endif /*CONFIG_VDSL*/
}


void init_adsl_psd_mask(request * wp)
{
#ifndef CONFIG_VDSL
	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "	<th align=left><font size=2>ADSL %s:</th>\n", multilang(LANG_PSD_MASK) );
	boaWrite(wp, "	<td></td>\n");
	boaWrite(wp, "</tr>\n");
	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "	<th></th>\n");
	boaWrite(wp, "	<td><font size=2><input type=\"button\" value=\"%s\" name=\"adslPSDMaskTbl\" onClick=\"adsltoneClick('/adslpsd.asp')\"></td>\n", multilang(LANG_PSD_MASK) );
	boaWrite(wp, "</tr>\n");
#endif /*CONFIG_VDSL*/
}


void init_psd_msm_mode(request * wp)
{
#ifndef CONFIG_VDSL
#if SUPPORT_TR105
	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "	<th align=left><font size=2>%s:</th>\n", multilang(LANG_PSD_MEASUREMENT_MODE) );
	boaWrite(wp, "	<td></td>\n");
	boaWrite(wp, "</tr>\n");
	boaWrite(wp, "<tr>\n");
	boaWrite(wp, "	<th></th>\n");
	boaWrite(wp, "	<td><font size=2>\n");
	adslPSDMeasure(0, wp, 0, NULL);
	boaWrite(wp, "	</td>\n");
	boaWrite(wp, "</tr>\n");
#endif /*SUPPORT_TR105*/
#endif /*CONFIG_VDSL*/
}

void initSetDslPage(request * wp)
{
	unsigned char vChar;
	unsigned char dsl_anxb;
	unsigned short mode;
#ifdef CONFIG_VDSL
	unsigned short vd2p;
	XDSL_OP *d;
	unsigned int pfCap;
#endif /*CONFIG_VDSL*/
#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
	unsigned short gfast_profile;
//	XDSL_OP *d;
	unsigned int gfastCap;
#endif /*CONFIG_USER_CMD_SUPPORT_GFAST*/


	mib_get_s( MIB_ADSL_MODE, (void *)&mode, sizeof(mode));
	mib_get_s( MIB_DSL_ANNEX_B, (void *)&dsl_anxb, sizeof(dsl_anxb));

	// TODO: adsl mode
	if (dsl_anxb) {
		// Rename T1.413: Annex.B uses ETSI; Annex.A uses ANSI(T1.413)
		boaWrite(wp, "%s.getElementById(\"t1413_id\").innerHTML = \"<font size=2><input type=checkbox name=t1413 value=1>ETSI\";\n", DOCUMENT);
	}
	if (mode & ADSL_MODE_GLITE)
		boaWrite(wp, "%s.set_adsl.glite.checked = true;\n", DOCUMENT);
	if (mode & ADSL_MODE_T1413)
		boaWrite(wp, "%s.set_adsl.t1413.checked = true;\n", DOCUMENT);
	if (mode & ADSL_MODE_GDMT)
		boaWrite(wp, "%s.set_adsl.gdmt.checked = true;\n", DOCUMENT);
	if (mode & ADSL_MODE_ADSL2)
		boaWrite(wp, "%s.set_adsl.adsl2.checked = true;\n", DOCUMENT);
	if (mode & ADSL_MODE_ADSL2P)
		boaWrite(wp, "%s.set_adsl.adsl2p.checked = true;\n", DOCUMENT);
	//if (mode & ADSL_MODE_ANXB) {
	if (dsl_anxb) {
		if (mode & ADSL_MODE_ANXJ)
			boaWrite(wp, "%s.set_adsl.anxj.checked = true;\n", DOCUMENT);
		boaWrite(wp, "%s.set_adsl.anxl.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.set_adsl.anxm.disabled = true;\n", DOCUMENT);
	}
	else {
		boaWrite(wp, "%s.set_adsl.anxj.disabled = true;\n", DOCUMENT);
		if (mode & ADSL_MODE_ANXL)
			boaWrite(wp, "%s.set_adsl.anxl.checked = true;\n", DOCUMENT);
		if (mode & ADSL_MODE_ANXM)
			boaWrite(wp, "%s.set_adsl.anxm.checked = true;\n", DOCUMENT);
	}
#ifdef ENABLE_ADSL_MODE_GINP
	if (mode & ADSL_MODE_GINP)
		boaWrite(wp, "%s.set_adsl.ginp.checked = true;\n", DOCUMENT);
#endif

#ifdef CONFIG_VDSL
	mib_get_s( MIB_DSL_G_VECTOR, (void *)&vChar, sizeof(vChar));
	if (vChar != 0)
		boaWrite(wp, "%s.set_adsl.gvec.checked = true;\n", DOCUMENT);

	if (mode & ADSL_MODE_VDSL2)
		boaWrite(wp, "%s.set_adsl.vdsl2.checked = true;\n", DOCUMENT);

	mib_get_s( MIB_VDSL2_PROFILE, (void *)&vd2p, sizeof(vd2p));
	d=xdsl_get_op(0);
	pfCap = 0;
	d->xdsl_msg_get(GetVdslProfileCap, &pfCap);
	if (pfCap == 0) pfCap = 0xff; // compatible with old driver
	vd2p &= pfCap;
	{
		int i;
		for(i=0; i<initvd2p_listnum; i++)
		{
			if (vd2p & initvd2p[i].profile)
				boaWrite(wp, "%s.set_adsl.%s.checked = true;\n", DOCUMENT, initvd2p[i].tagname);
		}
	}
	boaWrite(wp, "updateVDSL2();\n");
#endif /*CONFIG_VDSL*/
#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
	if (mode & ADSL_MODE_GFAST)
		boaWrite(wp, "%s.set_adsl.gfast.checked = true;\n", DOCUMENT);

	mib_get_s( MIB_GFAST_PROFILE, (void *)&gfast_profile, sizeof(gfast_profile));
	if(!d->xdsl_msg_get(UserGetGfastSupportProfile,&pfCap))
	{
		printf("[%s] check G. Fast support profile fail\n", __FUNCTION__);
		return;
	}

	gfast_profile &= pfCap;
	{
		int i;
		for(i=0; i<initGfast_listnum; i++)
		{
			if (gfast_profile & initGfast[i].profile)
				boaWrite(wp, "%s.set_adsl.%s.checked = true;\n", DOCUMENT, initGfast[i].tagname);
		}
	}
	boaWrite(wp, "updateGfast();\n");
#endif /* CONFIG_USER_CMD_SUPPORT_GFAST */
	mib_get_s( MIB_ADSL_OLR, (void *)&vChar, sizeof(vChar));

	if (vChar & 1)
		boaWrite(wp, "%s.set_adsl.bswap.checked = true;\n", DOCUMENT);
	if (vChar & 2)
		boaWrite(wp, "%s.set_adsl.sra.checked = true;\n", DOCUMENT);
}

void initDiagDslPage(request * wp, XDSL_OP *d)
{
#ifdef CONFIG_VDSL
	int mval=0;
	if(d->xdsl_msg_get(GetPmdMode,&mval))
	{
		if(mval&(MODE_ADSL2|MODE_ADSL2PLUS|MODE_VDSL2))
			boaWrite(wp, "%s.diag_adsl.start.disabled = false;\n", DOCUMENT);
	}
#else
	char chVal[2];
	if(d->xdsl_drv_get(RLCM_GET_SHOWTIME_XDSL_MODE, (void *)&chVal[0], 1)) {
		chVal[0]&=0x1F;
		if (chVal[0] == 3 || chVal[0] == 5)  // ADSL2/2+
			boaWrite(wp, "%s.diag_adsl.start.disabled = false;\n", DOCUMENT);
	}
#endif /*CONFIG_VDSL*/
}
#endif

#ifdef PORT_TRIGGERING_STATIC
int portTrgList(request * wp)
{
	unsigned int entryNum, i;
	MIB_CE_PORT_TRG_T Entry;
	char	*type, portRange[20], *ip;

	entryNum = mib_chain_total(MIB_PORT_TRG_TBL);

#ifndef CONFIG_GENERAL_WEB
	boaWrite(wp,"<tr><td bgColor=#808080>%s</td><td bgColor=#808080>%s</td>"
		"<td bgColor=#808080>TCP %s</td><td bgColor=#808080>UDP %s</td><td bgColor=#808080>%s</td><td bgColor=#808080>%s</td></tr>\n",
#else
	boaWrite(wp,"<tr><th>%s</th><th>%s</th>"
			"<th>TCP %s</th><th>UDP %s</th><th>%s</th><th>%s</th></tr>\n",
#endif
	multilang(LANG_NAME), multilang(LANG_IP_ADDRESS), multilang(LANG_PORT_TO_OPEN),
	multilang(LANG_PORT_TO_OPEN), multilang(LANG_ENABLE), multilang(LANG_ACTION));

	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_PORT_TRG_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

#ifndef CONFIG_GENERAL_WEB
		//Name
		boaWrite(wp,"<tr><td bgColor=#C0C0C0>%s</td>\n",Entry.name);

		//IP
		boaWrite(wp,"<td bgColor=#C0C0C0>%s</td>\n",inet_ntoa(*((struct in_addr *)Entry.ip)));

		//TCP port to open
		boaWrite(wp,"<td bgColor=#C0C0C0>%s</td>\n",Entry.tcpRange);

		//UDP port to open
		boaWrite(wp,"<td bgColor=#C0C0C0>%s</td>\n",Entry.udpRange);

		//Enable
		boaWrite(wp,"<td bgColor=#C0C0C0>%s</td>\n",(Entry.enable==1)?"Enable":"Disable");

		//Action
		boaWrite(wp,"<td bgColor=#C0C0C0>");
		boaWrite(wp,
		"<a href=\"#?edit\" onClick=\"editClick(%d)\">"
		"<image border=0 src=\"graphics/edit.gif\" alt=\"Post for editing\" /></a>", i);

		boaWrite(wp,
		"<a href=\"#?delete\" onClick=\"delClick(%d)\">"
		"<image border=0 src=\"graphics/del.gif\" alt=Delete /></td></tr>\n", i);
#else
		//Name
		boaWrite(wp,"<tr><td>%s</td>\n",Entry.name);

		//IP
		boaWrite(wp,"<td>%s</td>\n",inet_ntoa(*((struct in_addr *)Entry.ip)));

		//TCP port to open
		boaWrite(wp,"<td>%s</td>\n",Entry.tcpRange);

		//UDP port to open
		boaWrite(wp,"<td>%s</td>\n",Entry.udpRange);

		//Enable
		boaWrite(wp,"<td>%s</td>\n",(Entry.enable==1)?"Enable":"Disable");

		//Action
		boaWrite(wp,"<td>");
		boaWrite(wp,
		"<a href=\"#?edit\" onClick=\"editClick(%d)\">"
		"<image border=0 src=\"graphics/edit.gif\" alt=\"Post for editing\" /></a>", i);

		boaWrite(wp,
		"<a href=\"#?delete\" onClick=\"delClick(%d)\">"
		"<image border=0 src=\"graphics/del.gif\" alt=Delete /></td></tr>\n", i);
#endif
	}

	return 0;
}

int gm_postIndex=-1;

void initGamingPage(request * wp)
{
	char *ipaddr;
	char *idx;
	int del;
	char ipaddr2[16]={0};
	MIB_CE_PORT_TRG_T Entry;
	int found=0;

	ipaddr=boaGetVar(wp,"ip","");
	idx=boaGetVar(wp,"idx","");
	del=atoi(boaGetVar(wp,"del",""));

	if (gm_postIndex >= 0) { // post this entry
		if (!mib_chain_get(MIB_PORT_TRG_TBL, gm_postIndex, (void *)&Entry))
			found = 0;
		else
			found = 1;
		gm_postIndex = -1;
	}

	if(del!=0)
	{
		boaWrite(wp,
		"<body onLoad=\"document.formname.submit()\">");
	}
	else
	{
#ifndef CONFIG_GENERAL_WEB
		boaWrite(wp,
		"<body bgcolor=\"#ffffff\" text=\"#000000\" onLoad=\"javascript:formLoad();\">");
		boaWrite(wp, "<blockquote><h2><font color=\"#0000FF\">%s%s</font></h2>\n",
		multilang(LANG_PORT_TRIGGERING), multilang(LANG_CONFIGURATION));
		boaWrite(wp, "<table border=0 width=850 cellspacing=4 cellpadding=0><tr><td><hr size=1 noshade align=top></td></tr>\n");
#else
		boaWrite(wp,"<div class=\"intro_main \">"
			"<p class=\"intro_title\">%s %s</p>\n"
			//"<p class=\"intro_content\"> <% multilang(LANG_PAGE_DESC_LAN_TO_INTERNET_DATA_PACKET_FILTER_TABLE); %></p>\n"
		"</div>\n",multilang(LANG_PORT_TRIGGERING), multilang(LANG_CONFIGURATION));
#endif
		//<b>%s Game Rule</b>%s",(strlen(idx)==0)?"Add":"Edit",(strlen(idx)==0)?"":" [<a href=\"gaming.asp\">Add New</a>]");
	}


	boaWrite(wp,
	"<form action=/boaform/formGaming method=POST name=formname>\n");

	if(del!=0)
	{
		int i=atoi(idx);
		boaWrite(wp,"<input type=hidden name=idx value=\"%d\">",i);
		boaWrite(wp,"<input type=hidden name=del value=1></form>");
		return;
	}

#ifndef CONFIG_GENERAL_WEB
	boaWrite(wp,
	"<table width=850 cellSpacing=1 cellPadding=2 border=0>\n" \
	"<tr><font size=2><td bgColor=#808080>%s</td><td bgColor=#808080>%s</td><td bgColor=#808080>TCP %s</td><td bgColor=#808080>UDP %s</td><td bgColor=#808080>%s</td></tr>\n",
	multilang(LANG_NAME), multilang(LANG_IP_ADDRESS), multilang(LANG_PORT_TO_OPEN),
	multilang(LANG_PORT_TO_OPEN), multilang(LANG_ENABLE));

	boaWrite(wp,
	"<tr><td bgColor=#C0C0C0><font size=2><input type=text name=\"game\" size=16  maxlength=20 value=\"%s\">&lt;&lt; <select name=\"gamelist\" onChange=\"javascript:changeItem();\"></select></td>" \
	"<td bgColor=#C0C0C0><input type=text name=\"ip\" size=12  maxlength=15 value=\"%s\"></td>" \
	"<td bgColor=#C0C0C0><input type=text name=\"tcpopen\" size=20  maxlength=31 value=\"%s\"></td>" \
	"<td bgColor=#C0C0C0><input type=text name=\"udpopen\" size=20  maxlength=31 value=\"%s\"></td>" \
	"<td bgColor=#C0C0C0><input type=checkbox name=\"open\" value=1 %s></td>" \
	"<input type=hidden name=idx value=%s>" \
	"</tr></table>\n",
	found ? (char *)Entry.name : "",
	found ? (char *)inet_ntoa(*((struct in_addr *)Entry.ip)) : "0.0.0.0",
	found ? (char *)Entry.tcpRange : "",
	found ? (char *)Entry.udpRange : "",
	found ? (Entry.enable == 1 ? multilang(LANG_CHECKED) :"") : "",
	(strlen(idx)==0)?"-1":idx
	);

	boaWrite(wp,
	"<input type=submit value=%s name=add onClick=\"return addClick()\">&nbsp;&nbsp;&nbsp;&nbsp;" \
	"<input type=submit value=%s name=modify onClick=\"return addClick()\">&nbsp;&nbsp;&nbsp;&nbsp;" \
	"<input type=reset value=%s><BR><BR>\n",
	multilang(LANG_ADD), multilang(LANG_MODIFY), multilang(LANG_RESET));
	boaWrite(wp,
	"<input type=hidden value=/gaming.asp name=submit-url>");

	boaWrite(wp,
	"<b>%s</b>\n" \
//	"<input type=hidden name=ms value=%d>\n" \
/*	"onSubmit=\"return checkRange();\"" \ */
	"<table cellSpacing=1 cellPadding=2 border=0>\n", multilang(LANG_GAME_RULES_LIST));
#else
	boaWrite(wp,"<div class=\"data_vertical data_common_notitle\">\n"
	"<div class=\"data_common \">\n"
	"<table>\n"
	"<tr><th>%s</th><th>%s</th><th>TCP %s</th><th>UDP %s</th><th>%s</th></tr>\n",
	multilang(LANG_NAME), multilang(LANG_IP_ADDRESS), multilang(LANG_PORT_TO_OPEN),
	multilang(LANG_PORT_TO_OPEN), multilang(LANG_ENABLE));

	boaWrite(wp,
	"<tr><td><input type=text name=\"game\" size=16  maxlength=20 value=\"%s\">&lt;&lt; <select name=\"gamelist\" onChange=\"javascript:changeItem();\"></select></td>" \
	"<td><input type=text name=\"ip\" size=12  maxlength=15 value=\"%s\"></td>" \
	"<td><input type=text name=\"tcpopen\" size=20  maxlength=31 value=\"%s\"></td>" \
	"<td><input type=text name=\"udpopen\" size=20  maxlength=31 value=\"%s\"></td>" \
	"<td><input type=checkbox name=\"open\" value=1 %s></td>" \
	"<input type=hidden name=idx value=%s>" \
	"</tr></table>\n",
	found ? (char *)Entry.name : "",
	found ? (char *)inet_ntoa(*((struct in_addr *)Entry.ip)) : "0.0.0.0",
	found ? (char *)Entry.tcpRange : "",
	found ? (char *)Entry.udpRange : "",
	found ? (Entry.enable == 1 ? multilang(LANG_CHECKED) :"") : "",
	(strlen(idx)==0)?"-1":idx
	);
	boaWrite(wp,"</table>\n</div>\n</div>\n");
	boaWrite(wp,"<div class=\"btn_ctl\">"
	"<input class=\"link_bg\" type=submit value=%s name=add onClick=\"return addClick()\">&nbsp;&nbsp;&nbsp;&nbsp;" \
	"<input class=\"link_bg \" type=submit value=%s name=modify onClick=\"return addClick()\">&nbsp;&nbsp;&nbsp;&nbsp;" \
	"<input class=\"link_bg \" type=reset value=%s>\n",
	multilang(LANG_ADD), multilang(LANG_MODIFY), multilang(LANG_RESET));
	boaWrite(wp,
	"<input type=hidden value=/gaming.asp name=submit-url>");
	boaWrite(wp,"</div>\n");

	boaWrite(wp,"<div class=\"column\">\n"
	"<div class=\"column_title\">\n"
	"<div class=\"column_title_left\"></div>\n"
	"<p>%s</p>\n"
	"<div class=\"column_title_right\"></div>\n"
	"</div>\n"
	"<div class=\"data_common\">\n"
	"<table>\n", multilang(LANG_GAME_RULES_LIST));
#endif
	portTrgList(wp);

#ifdef CONFIG_GENERAL_WEB
	boaWrite(wp, "</table>\n</div>\n</div>\n");
#endif
	boaWrite(wp, "<script>\nformLoad();</script>\n");
	boaWrite(wp, "</form>\n");
}
#endif

#ifdef CONFIG_USER_ROUTED_ROUTED
void initRipPage(request * wp)
{
	if (ifWanNum("rt") ==0) {
		boaWrite(wp, "%s.rip.rip_on[0].disabled = true;\n", DOCUMENT);
		boaWrite(wp, "\t%s.rip.rip_on[1].disabled = true;\n", DOCUMENT);
		boaWrite(wp, "\t%s.rip.rip_ver.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "\t%s.rip.rip_if.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "\t%s.rip.ripAdd.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "\t%s.rip.ripSet.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "\t%s.rip.ripReset.disabled = true;", DOCUMENT);
	}
	boaWrite(wp, "\t%s.rip.ripDel.disabled = true;\n", DOCUMENT);
}
#endif

#if defined(CONFIG_RTL_MULTI_LAN_DEV)
#ifdef ELAN_LINK_MODE
void initLinkPage(request * wp)
{
	unsigned int entryNum, i;
	MIB_CE_SW_PORT_T Entry;
	char ports[]="p0";

	entryNum = mib_chain_total(MIB_SW_PORT_TBL);

	if (entryNum >= SW_LAN_PORT_NUM)
		entryNum = SW_LAN_PORT_NUM;

	for (i=0; i<entryNum; i++) {
		if (mib_chain_get(MIB_SW_PORT_TBL, i, (void *)&Entry)) {
			ports[1]=i + '0';
			boaWrite(wp, "%s.link.%s.value = %d;\n", DOCUMENT, ports, Entry.linkMode);
		}
	}
}
#endif

#else
#ifdef ELAN_LINK_MODE_INTRENAL_PHY
void initLinkPage(request * wp)
{

	unsigned int entryNum, i;
	//MIB_CE_SW_PORT_T Entry;
	char ports[]="p0";
	unsigned char mode;

	//entryNum = mib_chain_total(MIB_SW_PORT_TBL);
	if (mib_get_s(MIB_ETH_MODE, &mode, sizeof(mode))) {
		boaWrite(wp, "%s.link.%s.value = %d;\n", DOCUMENT, ports, mode);
	}
}

#endif
#endif	// CONFIG_RTL_MULTI_LAN_DEV

#ifdef IP_PASSTHROUGH
void initOthersPage(request * wp)
{
	unsigned int vInt;
	unsigned char vChar;

	mib_get_s( MIB_IPPT_ITF, (void *)&vInt, sizeof(vInt));
	//if (vInt == 0xff) {
	if (vInt == DUMMY_IFINDEX) {
		boaWrite(wp, "%s.others.ltime.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.others.lan_acc.disabled = true;\n", DOCUMENT);
	}

	mib_get_s( MIB_IPPT_LANACC, (void *)&vChar, sizeof(vChar));
	if (vChar == 1)
		boaWrite(wp, "%s.others.lan_acc.checked = true\n", DOCUMENT);
}
#endif

#ifdef TIME_ZONE
void initNtpPage(request * wp)
{
	unsigned char vChar = 1;

	//boaWrite(wp, "%s.time.adjust_time.checked = false;\n", DOCUMENT);

	mib_get_s(MIB_DST_ENABLED, (void *)&vChar, sizeof(vChar));
	if (vChar == 1)
		boaWrite(wp, "%s.time.dst_enabled.checked = true;\n", DOCUMENT);

	mib_get_s(MIB_NTP_ENABLED, (void *)&vChar, sizeof(vChar));
	if (vChar == 1)
		boaWrite(wp, "%s.time.enabled.checked = true;\n", DOCUMENT);

	boaWrite(wp, "const td = document.querySelector(\"td[name='ntpSync']\");");
	if (isTimeSynchronized())
		boaWrite(wp, "td.textContent=\"성공\";\n");
	else
		boaWrite(wp, "td.textContent=\"실패\";\n");

#ifdef CONFIG_00R0
	int pri = NTP_EXT_ITF_PRI_LOW;
	mib_get_s(MIB_NTP_EXT_ITF_PRI, (void *)&pri, sizeof(pri));
	if(pri > NTP_EXT_ITF_PRI_LOW)
		boaWrite(wp, "configurable=0;\n");
#endif
}
#endif

#ifdef WLAN_SUPPORT

#ifdef WLAN_WPA
void initWlWpaPage(request * wp)
{
	unsigned char buffer[255];
	MIB_CE_MBSSIB_T Entry;
	if(mib_chain_get(MIB_MBSSIB_TBL, 0, &Entry) == 0)
		printf("\n %s %d\n", __func__, __LINE__);

	boaWrite(wp, "%s.formEncrypt.pskFormat.value = %d;\n", DOCUMENT, Entry.wpaPSKFormat);

#ifdef WLAN_1x
	if(Entry.wep!=0)
		boaWrite(wp, "%s.formEncrypt.wepKeyLen[%d].checked = true;\n", DOCUMENT, Entry.wep-1);

	if(Entry.enable1X==1)
		boaWrite(wp, "%s.formEncrypt.use1x.checked = true;\n", DOCUMENT);
	boaWrite(wp, "%s.formEncrypt.wpaAuth[%d].checked = true;\n", DOCUMENT, Entry.wpaAuth-1);
#else
	boaWrite(wp, "%s.formEncrypt.wpaAuth.disabled = true;\n", DOCUMENT);
	boaWrite(wp, "%s.formEncrypt.wepKeyLen.disabled = true;\n", DOCUMENT);
	boaWrite(wp, "%s.formEncrypt.use1x.disabled = true;\n", DOCUMENT);

#endif
}
#endif

void initWlBasicPage(request * wp)
{
	unsigned char vChar;
	MIB_CE_MBSSIB_T Entry;
	wlan_getEntry(&Entry, 0);

#ifdef WLAN_UNIVERSAL_REPEATER
	boaWrite(wp, "%s.getElementById(\"repeater_check\").style.display = \"\";\n", DOCUMENT);
	boaWrite(wp, "%s.getElementById(\"repeater_SSID\").style.display = \"\";\n", DOCUMENT);
#endif
	if (Entry.wlanDisabled!=0)
		// hidden type
		boaWrite(wp, "%s.wlanSetup.wlanDisabled.value = \"ON\";\n", DOCUMENT);
		// checkbox type
		//boaWrite(wp, "%s.wlanSetup.wlanDisabled.checked = true;\n", DOCUMENT);

#ifdef WLAN_6G_SUPPORT
		mib_get_s( MIB_WLAN_PHY_BAND_SELECT, (void *)&vChar, sizeof(vChar));
		if(vChar == PHYBAND_5G){
			boaWrite(wp, "%s.getElementById(\"optionfor6g\").style.display = \"\";\n", DOCUMENT);
			mib_get_s(MIB_WLAN_6G_SUPPORT, (void *)&vChar, sizeof(vChar));
			if(vChar){
				boaWrite(wp, "%s.wlanSetup.wlan6gSupport.value = \"ON\";\n", DOCUMENT);
				boaWrite(wp, "%s.wlanSetup.wlan6gSupport.checked = true;\n", DOCUMENT);
			}
			else{
				boaWrite(wp, "%s.wlanSetup.wlan6gSupport.value = \"OFF\";\n", DOCUMENT);
				boaWrite(wp, "%s.wlanSetup.wlan6gSupport.checked = false;\n", DOCUMENT);
			}
		}
#endif

#ifdef WLAN_BAND_CONFIG_MBSSID
		vChar = Entry.wlanBand;
#else
		mib_get_s( MIB_WLAN_BAND, (void *)&vChar, sizeof(vChar));
#endif
		boaWrite(wp, "%s.wlanSetup.band.value = %d;\n", DOCUMENT, vChar-1);
#if defined (WLAN_11N_COEXIST)
	unsigned char phyband;
	mib_get_s( MIB_WLAN_PHY_BAND_SELECT, (void *)&phyband, sizeof(phyband));
	if(phyband == PHYBAND_5G) {
#ifdef CONFIG_00R0
		mib_get_s( MIB_WLAN_11N_COEXIST, (void *)&vChar, sizeof(vChar));
		if(vChar!=0)
			vChar = 4;
		else
			mib_get_s( MIB_WLAN_CHANNEL_WIDTH,(void *)&vChar, sizeof(vChar));
#else
		mib_get_s( MIB_WLAN_CHANNEL_WIDTH,(void *)&vChar, sizeof(vChar));
#endif
		boaWrite(wp, "%s.wlanSetup.chanwid.value = %d;\n", DOCUMENT, vChar);
	}
	else{
		mib_get_s( MIB_WLAN_11N_COEXIST,(void *)&vChar, sizeof(vChar));
		if(vChar!=0)
			vChar = 3;
		else
			mib_get_s( MIB_WLAN_CHANNEL_WIDTH,(void *)&vChar, sizeof(vChar));
		boaWrite(wp, "%s.wlanSetup.chanwid.value = %d;\n", DOCUMENT, vChar);
	}
#else
	mib_get_s( MIB_WLAN_CHANNEL_WIDTH,(void *)&vChar, sizeof(vChar));
		boaWrite(wp, "%s.wlanSetup.chanwid.value = %d;\n", DOCUMENT, vChar);
#endif
	mib_get_s( MIB_WLAN_CONTROL_BAND,(void *)&vChar, sizeof(vChar));
		boaWrite(wp, "%s.wlanSetup.ctlband.value = %d;\n", DOCUMENT, vChar);
	mib_get_s( MIB_TX_POWER, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.wlanSetup.txpower.selectedIndex = %d;\n", DOCUMENT, vChar);
	mib_get_s( MIB_WIFI_REGDOMAIN_DEMO, (void *)&vChar, sizeof(vChar));
	if(vChar != 0)
	{
		boaWrite(wp, "%s.getElementById(\"wifi_regdomain_demo\").style.display = \"\";\n", DOCUMENT);
		mib_get_s( MIB_HW_REG_DOMAIN, (void *)&vChar, sizeof(vChar));
		boaWrite(wp, "%s.wlanSetup.regdomain_demo.selectedIndex = %d;\n", DOCUMENT, (vChar-1));
	}

#ifdef WLAN_LIMITED_STA_NUM
		boaWrite(wp, "%s.getElementById(\"wl_limit_stanum\").style.display = \"\";\n", DOCUMENT);
		boaWrite(wp, "%s.wlanSetup.wl_limitstanum.selectedIndex = %d;\n", DOCUMENT, Entry.stanum>0? 1:0);
		if(Entry.stanum)
			boaWrite(wp, "%s.wlanSetup.wl_stanum.value = %d;\n", DOCUMENT, Entry.stanum);
		else
			boaWrite(wp, "%s.wlanSetup.wl_stanum.disabled = true;\n", DOCUMENT);
#endif
	//boaWrite(wp, "updateState(%s.wlanSetup);\n", DOCUMENT);
#ifdef CONFIG_00R0
	{
		unsigned char username[MAX_NAME_LEN];
		mib_get_s(MIB_SUSER_NAME, (void *)username, sizeof(username));
		if(strcmp(g_login_username, username) == 0){
			boaWrite(wp, "enableTextField(%s.wlanSetup.regdomain_demo);\n", DOCUMENT);
		}
		else{
			boaWrite(wp, "disableTextField(%s.wlanSetup.regdomain_demo);\n", DOCUMENT);
		}
	}
#endif

}

//#ifdef WLAN_WDS
void initWlWDSPage(request * wp){
	unsigned char disWlan,mode;
	MIB_CE_MBSSIB_T Entry;
	if(mib_chain_get(MIB_MBSSIB_TBL, 0, &Entry) == 0)
		printf("\n %s %d\n", __func__, __LINE__);
	disWlan = Entry.wlanDisabled;
	mode = Entry.wlanMode;
	if(disWlan || mode != AP_WDS_MODE){
		boaWrite(wp,"%s.formWlWdsAdd.wlanWdsEnabled.disabled = true;\n",DOCUMENT);
	}
}

void initWlSurveyPage(request * wp){
#ifdef WLAN_6G_SUPPORT
	unsigned char vChar=0;
	mib_get_s(MIB_WLAN_6G_SUPPORT, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "\twlan6gSupport=%d;\n", vChar);
#endif
#if defined(WLAN_CLIENT) || defined(WLAN_SITESURVEY)
	boaWrite(wp,"%s.formWlSiteSurvey.refresh.disabled = false;\n",DOCUMENT);
#else
	boaWrite(wp,"%s.formWlSiteSurvey.refresh.disabled = true;\n",DOCUMENT);
#endif
}
//#endif

void initWlAdvPage(request * wp)
{
	unsigned char vChar;
#ifdef WIFI_TEST
	unsigned short vShort;
#endif
	MIB_CE_MBSSIB_T Entry;
	wlan_getEntry(&Entry, 0);
	mib_get_s( MIB_WLAN_PREAMBLE_TYPE, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.advanceSetup.preamble[%d].checked = true;\n", DOCUMENT, vChar);
	boaWrite(wp, "%s.advanceSetup.hiddenSSID[%d].checked = true;\n", DOCUMENT, Entry.hidessid);
	boaWrite(wp, "%s.advanceSetup.block[%d].checked = true;\n", DOCUMENT, Entry.userisolation==0?1:0);
#ifndef WLAN_11AX
	mib_get_s( MIB_WLAN_PROTECTION_DISABLED, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.advanceSetup.protection[%d].checked = true;\n", DOCUMENT, vChar);
#endif
	mib_get_s( MIB_WLAN_AGGREGATION, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.advanceSetup.aggregation[%d].checked = true;\n", DOCUMENT, (vChar & (1<<WLAN_AGGREGATION_AMPDU))==0?1:0);
	mib_get_s( MIB_WLAN_SHORTGI_ENABLED, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.advanceSetup.shortGI0[%d].checked = true;\n", DOCUMENT, vChar==0?1:0);
#ifdef WLAN_TX_BEAMFORMING
#ifdef WIFI5_WIFI6_COMP
	int show_txbf=0;
#if IS_AX_SUPPORT
	if(Entry.is_ax_support)
		show_txbf=1;
#endif
#ifdef WLAN_TX_BEAMFORMING_ENABLE
	show_txbf=1;
#endif
	if(show_txbf)
#endif
	{
		boaWrite(wp, "%s.advanceSetup.txbf[%d].checked = true;\n", DOCUMENT, Entry.txbf==0?1:0);
		boaWrite(wp, "%s.advanceSetup.txbf_mu[%d].checked = true;\n", DOCUMENT, Entry.txbf_mu==0?1:0);
#ifdef WLAN_INTF_TXBF_DISABLE
		if(wlan_idx == WLAN_INTF_TXBF_DISABLE){
			boaWrite(wp, "%s.getElementById(\"txbf_div\").style.display = \"none\";\n", DOCUMENT);
			boaWrite(wp, "%s.advanceSetup.txbf[%d].checked = true;\n", DOCUMENT, 1);
		}
#endif
	}
#endif
	boaWrite(wp, "%s.advanceSetup.mc2u_disable[%d].checked = true;\n", DOCUMENT, Entry.mc2u_disable);

#ifdef RTK_SMART_ROAMING
	mib_get_s( MIB_CAPWAP_MODE, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.advanceSetup.smart_roaming_enable[%d].checked = true;\n", DOCUMENT, (vChar & CAPWAP_ROAMING_ENABLE)?0:1);
	boaWrite(wp, "%s.advanceSetup.SR_AutoConfig_enable[%d].checked = true;\n", DOCUMENT, (vChar & CAPWAP_AUTO_CONFIG_ENABLE)?0:1);
#endif
#ifdef WLAN_QoS
#ifndef IS_AX_SUPPORT
	boaWrite(wp, "%s.advanceSetup.WmmEnabled[%d].checked = true;\n", DOCUMENT, Entry.wmmEnabled==0?1:0);
#endif
#endif
#ifdef WLAN_11K
	boaWrite(wp, "%s.advanceSetup.dot11kEnabled[%d].checked = true;\n", DOCUMENT, Entry.rm_activated==0?1:0);
#endif
#ifdef WLAN_11V
	boaWrite(wp, "%s.advanceSetup.dot11vEnabled[%d].checked = true;\n", DOCUMENT, Entry.BssTransEnable==0?1:0);
#endif
#ifdef WLAN_BAND_STEERING
	mib_get_s(MIB_WIFI_STA_CONTROL, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.advanceSetup.sta_control[%d].checked = true;\n", DOCUMENT, (vChar & STA_CONTROL_ENABLE)?0:1);
	boaWrite(wp, "%s.advanceSetup.stactrl_prefer_band.selectedIndex = %d;\n", DOCUMENT, (vChar & STA_CONTROL_PREFER_BAND)?1:0);
	if(rtk_wlan_get_band_steering_status() == 0){
		boaWrite(wp, "%s.advanceSetup.sta_control[0].disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.advanceSetup.sta_control[1].disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.advanceSetup.stactrl_prefer_band.disabled = true;\n", DOCUMENT);
	}
	else
		if(!(vChar & STA_CONTROL_ENABLE))
			boaWrite(wp, "%s.advanceSetup.stactrl_prefer_band.disabled = true;\n", DOCUMENT);
#endif
#ifdef BAND_STEERING_SUPPORT
	mib_get_s(MIB_WIFI_STA_CONTROL_ENABLE, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.advanceSetup.sta_control[%d].checked = true;\n", DOCUMENT, (vChar!=0)? 0:1);
	if(rtk_wlan_get_band_steering_status() == 0){
		boaWrite(wp, "%s.advanceSetup.sta_control[0].disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.advanceSetup.sta_control[1].disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.advanceSetup.stactrl_prefer_band.disabled = true;\n", DOCUMENT);
	}
	else
		if(vChar == 0)
			boaWrite(wp, "%s.advanceSetup.stactrl_prefer_band.disabled = true;\n", DOCUMENT);
	mib_get_s(MIB_PREFER_BAND, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.advanceSetup.stactrl_prefer_band.selectedIndex = %d;\n", DOCUMENT, vChar);
#endif
#ifdef WLAN_11R
	boaWrite(wp, "%s.advanceSetup.dot11r_enable[%d].checked = true;\n", DOCUMENT, (Entry.ft_enable==0)?1:0);
	if(!(Entry.encrypt == WIFI_SEC_WPA2 || Entry.encrypt == WIFI_SEC_WPA2_MIXED)){
		boaWrite(wp, "%s.advanceSetup.dot11r_enable[0].disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.advanceSetup.dot11r_enable[1].disabled = true;\n", DOCUMENT);
	}
#endif
#ifdef RTK_CROSSBAND_REPEATER
	if(wlan_idx == 0){
		mib_get_s(MIB_CROSSBAND_ENABLE, (void *)&vChar, sizeof(vChar));
		boaWrite(wp, "%s.advanceSetup.cross_band_enable[%d].checked = true;\n", DOCUMENT, vChar==0?1:0);
	}
#endif
#ifdef WLAN_11AX
	mib_get_s( MIB_WLAN_BAND, (void *)&vChar, sizeof(vChar));
	if(vChar & BAND_5G_11AX){
		mib_get_s( MIB_WLAN_OFDMA_ENABLED, (void *)&vChar, sizeof(vChar));
		boaWrite(wp, "%s.advanceSetup.ofdmaEnable[%d].checked = true;\n", DOCUMENT, vChar==0?1:0);
	}
#endif
}

#ifdef WLAN_MBSSID
void initWLMBSSIDPage(request * wp)
{
	MIB_CE_MBSSIB_T Entry;
	int i=0;
	unsigned char vChar;

	if (mib_get_s(MIB_WLAN_BLOCK_MBSSID, (void *)&vChar, sizeof(vChar)) == 0) {
		printf("get MBSSID error!");
	}
	for (i=1; i<=4; i++) {
		if (!mib_chain_get(MIB_MBSSIB_TBL, i, (void *)&Entry)) {
  			printf("Error! Get MIB_MBSSIB_TBL(initWLMBSSIDPage) error.\n");
  			return;
		}
		boaWrite(wp, "%s.WlanMBSSID.wlAPIsolation_wl%d[%d].checked = true;\n", DOCUMENT, i-1, Entry.userisolation?0:1);
	}
}

void initWLMultiApPage(request * wp)
{
	MIB_CE_MBSSIB_T Entry;
	int i=0;
	unsigned char vChar;

	if (mib_get_s(MIB_WLAN_BLOCK_MBSSID, (void *)&vChar, sizeof(vChar)) == 0) {
		printf("get MBSSID error!");
	}

	boaWrite(wp, "%s.MultipleAP.mbssid_block[%d].checked = true;\n", DOCUMENT, vChar);

	for (i=1; i<=4; i++) {
		if (!mib_chain_get(MIB_MBSSIB_TBL, i, (void *)&Entry)) {
  			printf("Error! Get MIB_MBSSIB_TBL(initWLMultiApPage) error.\n");
  			return;
		}
		boaWrite(wp, "%s.MultipleAP.elements[\"wl_hide_ssid%d\"].selectedIndex = %d;\n", DOCUMENT, i, Entry.hidessid?0:1);
#ifndef CONFIG_00R0
		boaWrite(wp, "%s.MultipleAP.elements[\"wl_access%d\"].selectedIndex = %d;\n", DOCUMENT, i, Entry.userisolation);
#endif
#ifdef WLAN_LIMITED_STA_NUM
		boaWrite(wp, "%s.getElementById(\"wl_limit_stanum_div%d\").style.display = \"\";\n", DOCUMENT, i);
		boaWrite(wp, "%s.MultipleAP.elements[\"wl_limitstanum%d\"].selectedIndex = %d;\n", DOCUMENT, i, Entry.stanum>0?1:0);
		if(Entry.stanum)
			boaWrite(wp, "%s.MultipleAP.elements[\"wl_stanum%d\"].value = %d;\n", DOCUMENT, i, Entry.stanum);
#endif

		boaWrite(wp, "if(%s.getElementById(\"wlan_band%d\") != null)\n", DOCUMENT, i);
#if defined(WLAN_BAND_CONFIG_MBSSID)
		boaWrite(wp, "\t%s.getElementById(\"wlan_band%d\").style.display = \"\";\n", DOCUMENT, i);
#else
		boaWrite(wp, "\t%s.getElementById(\"wlan_band%d\").style.display = \"none\";\n", DOCUMENT, i);
#endif
#if 1 //vap will follow root interface
	boaWrite(wp, "%s.getElementById(\"wl_txbf_div%d\").style.display = \"none\";\n", DOCUMENT, i);
	boaWrite(wp, "%s.MultipleAP.elements[\"wl_txbf_ssid%d\"].selectedIndex = %d;\n", DOCUMENT, i, 0);
	boaWrite(wp, "%s.getElementById(\"wl_txbfmu_div%d\").style.display = \"none\";\n", DOCUMENT, i);
	boaWrite(wp, "%s.MultipleAP.elements[\"wl_txbfmu_ssid%d\"].selectedIndex = %d;\n", DOCUMENT, i, 0);
#else
#ifdef WLAN_TX_BEAMFORMING
		boaWrite(wp, "%s.getElementById(\"wl_txbf_div%d\").style.display = \"\";\n", DOCUMENT, i);
		boaWrite(wp, "%s.MultipleAP.elements[\"wl_txbf_ssid%d\"].selectedIndex = %d;\n", DOCUMENT, i, Entry.txbf);
		boaWrite(wp, "%s.getElementById(\"wl_txbfmu_div%d\").style.display = \"\";\n", DOCUMENT, i);
		boaWrite(wp, "%s.MultipleAP.elements[\"wl_txbfmu_ssid%d\"].selectedIndex = %d;\n", DOCUMENT, i, Entry.txbf_mu);
#ifdef WLAN_INTF_TXBF_DISABLE
		if(wlan_idx == WLAN_INTF_TXBF_DISABLE){
			boaWrite(wp, "%s.getElementById(\"wl_txbf_div%d\").style.display = \"none\";\n", DOCUMENT, i);
			boaWrite(wp, "%s.MultipleAP.elements[\"wl_txbf_ssid%d\"].selectedIndex = %d;\n", DOCUMENT, i, 0);
			boaWrite(wp, "%s.getElementById(\"wl_txbfmu_div%d\").style.display = \"none\";\n", DOCUMENT, i);
			boaWrite(wp, "%s.MultipleAP.elements[\"wl_txbfmu_ssid%d\"].selectedIndex = %d;\n", DOCUMENT, i, 0);
		}
#endif
#endif
#endif
#ifndef CONFIG_00R0
		boaWrite(wp, "%s.getElementById(\"wl_mc2u_div%d\").style.display = \"\";\n", DOCUMENT, i);
		boaWrite(wp, "%s.MultipleAP.elements[\"wl_mc2u_ssid%d\"].selectedIndex = %d;\n", DOCUMENT, i, Entry.mc2u_disable?0:1);
#endif
#ifdef CONFIG_00R0
		boaWrite(wp, "%s.getElementById(\"wl_rate_limit_div%d\").style.display = \"\";\n", DOCUMENT, i);
		boaWrite(wp, "%s.MultipleAP.elements[\"wl_rate_limit_sel_sta%d\"].selectedIndex = %d;\n", DOCUMENT, i, Entry.tx_restrict?1:0);
		if(Entry.tx_restrict)
			boaWrite(wp, "%s.MultipleAP.elements[\"wl_rate_limit_sta%d\"].value = %d;\n", DOCUMENT, i, Entry.tx_restrict);
#endif
	}
#ifdef WLAN_LIMITED_STA_NUM
	boaWrite(wp, "%s.getElementById(\"wl_limit_stanum_div\").style.display = \"\";\n", DOCUMENT);
#endif
	boaWrite(wp, "if(%s.getElementById(\"wlan_band\") != null)\n", DOCUMENT, i);
#if defined(WLAN_BAND_CONFIG_MBSSID)
	boaWrite(wp, "\t%s.getElementById(\"wlan_band\").style.display = \"\";\n", DOCUMENT, i);
#else
	boaWrite(wp, "\t%s.getElementById(\"wlan_band\").style.display = \"none\";\n", DOCUMENT, i);
#endif
#if 1 //vap will follow root interface
	boaWrite(wp, "%s.getElementById(\"wl_txbf_div\").style.display = \"none\";\n", DOCUMENT);
	boaWrite(wp, "%s.getElementById(\"wl_txbfmu_div\").style.display = \"none\";\n", DOCUMENT);
#else
#ifdef WLAN_TX_BEAMFORMING
	boaWrite(wp, "%s.getElementById(\"wl_txbf_div\").style.display = \"\";\n", DOCUMENT);
	boaWrite(wp, "%s.getElementById(\"wl_txbfmu_div\").style.display = \"\";\n", DOCUMENT);
	#ifdef WLAN_INTF_TXBF_DISABLE
		if(wlan_idx == WLAN_INTF_TXBF_DISABLE){
			boaWrite(wp, "%s.getElementById(\"wl_txbf_div\").style.display = \"none\";\n", DOCUMENT);
			boaWrite(wp, "%s.getElementById(\"wl_txbfmu_div\").style.display = \"none\";\n", DOCUMENT);
		}
	#endif
#endif
#endif
#ifdef CONFIG_00R0
	boaWrite(wp, "%s.getElementById(\"wl_rate_limit_div\").style.display = \"\";\n", DOCUMENT);
#else
	boaWrite(wp, "%s.getElementById(\"wl_mc2u_div\").style.display = \"\";\n", DOCUMENT);
#endif
}

#ifdef CONFIG_USER_FON
void initWLFonPage(request * wp)
{
	MIB_CE_MBSSIB_T Entry;
	int i=0;
	unsigned char vChar;

	if (mib_get_s(MIB_WLAN_BLOCK_MBSSID, (void *)&vChar, sizeof(vChar)) == 0) {
		printf("get MBSSID error!");
	}
	boaWrite(wp, "%s.MultipleAP.mbssid_block[%d].checked = true;\n", DOCUMENT, vChar);

//	for (i=1; i<=4; i++) {
		if (!mib_chain_get(MIB_MBSSIB_TBL, 4, (void *)&Entry)) {
  			printf("Error! Get MIB_MBSSIB_TBL(initWLMultiApPage) error.\n");
  			return;
		}
		boaWrite(wp, "%s.MultipleAP.elements[\"wl_hide_ssid%d\"].selectedIndex = %d;\n", DOCUMENT, 4, Entry.hidessid?0:1);
#ifndef CONFIG_00R0
		boaWrite(wp, "%s.MultipleAP.elements[\"wl_access%d\"].selectedIndex = %d;\n", DOCUMENT, 4, Entry.userisolation);
#endif
//	}
}
#endif
#endif

extern void wapi_mod_entry(MIB_CE_MBSSIB_T *, char *, char *);
static void wlan_ssid_helper(MIB_CE_MBSSIB_Tp pEntry, char *psk, char *RsIp, char* Rs2Ip)
{
	int len;

	// wpaPSK
	for (len=0; len<strlen(pEntry->wpaPSK); len++)
		psk[len]='*';
	psk[len]='\0';

	// RsIp
	if ( ((struct in_addr *)pEntry->rsIpAddr)->s_addr == INADDR_NONE ) {
		sprintf(RsIp, "%s", "");
	} else {
		sprintf(RsIp, "%s", inet_ntoa(*((struct in_addr *)pEntry->rsIpAddr)));
	}
#ifdef WLAN_RADIUS_2SET
	if ( ((struct in_addr *)pEntry->rs2IpAddr)->s_addr == INADDR_NONE ) {
		sprintf(Rs2Ip, "%s", "");
	} else {
		sprintf(Rs2Ip, "%s", inet_ntoa(*((struct in_addr *)pEntry->rs2IpAddr)));
	}
#endif
	#ifdef CONFIG_RTL_WAPI_SUPPORT
	if (pEntry->encrypt == WIFI_SEC_WAPI) {
		wapi_mod_entry(pEntry, psk, RsIp);
	}
	#endif
}

#ifdef WLAN_11R
void initWlFtPage(request * wp)
{
	MIB_CE_MBSSIB_T Entry;
	char strbuf[MAX_PSK_LEN+1], strbuf2[20], strbuf3[20];
	int isNmode;
	int i, k;
	char wlanDisabled;

	k=0;
	for (i=0; i<=NUM_VWLAN_INTERFACE; i++) {
		wlan_getEntry(&Entry, i);
		if (i==0) // root
			wlanDisabled = Entry.wlanDisabled;
		if (Entry.wlanDisabled)
			continue;
		if (Entry.wlanMode == CLIENT_MODE)
			continue;
		wlan_ssid_helper(&Entry, strbuf, strbuf2, strbuf3);
		boaWrite(wp, "\t_encrypt[%d]=%d;\n", k, Entry.encrypt);

		boaWrite(wp, "\t_ft_enable[%d]=%d;\n", k, Entry.ft_enable);
		boaWrite(wp, "\t_ft_mdid[%d]=\"%s\";\n", k, Entry.ft_mdid);
		boaWrite(wp, "\t_ft_over_ds[%d]=%d;\n", k, Entry.ft_over_ds);
		boaWrite(wp, "\t_ft_res_request[%d]=%d;\n", k, Entry.ft_res_request);
		boaWrite(wp, "\t_ft_r0key_timeout[%d]=%d;\n", k, Entry.ft_r0key_timeout);
		boaWrite(wp, "\t_ft_reasoc_timeout[%d]=%d;\n", k, Entry.ft_reasoc_timeout);
		boaWrite(wp, "\t_ft_r0kh_id[%d]=\"%s\";\n", k, Entry.ft_r0kh_id);
		boaWrite(wp, "\t_ft_push[%d]=%d;\n", k, Entry.ft_push);
		boaWrite(wp, "\t_ft_kh_num[%d]=%d;\n", k, Entry.ft_kh_num);
		k++;
	}
	boaWrite(wp, "\tssid_num=%d;\n", k);

	if(wlanDisabled) {
		boaWrite(wp, "\t%s.getElementById(\"wlan_dot11r_table\").style.display = 'none';\n", DOCUMENT);
		boaWrite(wp, "\t%s.write(\"<font size=2> WLAN Disabled !</font>\")", DOCUMENT);
	}
	else {
		boaWrite(wp, "\t%s.getElementById(\"wlan_dot11r_table\").style.display = \"\";\n", DOCUMENT);
	}
}
#endif

void initWlWpaMbssidPage(request * wp)
{
	MIB_CE_MBSSIB_T Entry;
	char strbuf[MAX_PSK_LEN+1], strbuf2[20], strbuf3[20];
	int isNmode;
	int i, k;
	char wlanDisabled;
	char translate_web_str[200]={0};
	char password[ENC_MAX_PSK_LEN+1];
	unsigned char wlband = 0, vChar = 0;
	char password_64[ENC_MAX_PSK_LEN+1];
	char password_128[ENC_MAX_PSK_LEN+1];
	char tmp_password[MAX_PSK_LEN+1];

#ifdef WLAN_6G_SUPPORT
	mib_get_s(MIB_WLAN_6G_SUPPORT, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "\twlan6gSupport=%d;\n", vChar);
#endif

	i=0;
	for (k=0; k<=NUM_VWLAN_INTERFACE; k++) {
		wlan_getEntry(&Entry, k);
		if (k==0) // root
			wlanDisabled = Entry.wlanDisabled;
		if (Entry.wlanDisabled)
			continue;
		if (Entry.wlanMode == CLIENT_MODE)
			continue;
		wlan_ssid_helper(&Entry, strbuf, strbuf2, strbuf3);
		boaWrite(wp, "_wlan_mode[%d]=%d;\n", k, Entry.wlanMode);
#ifdef WLAN_BAND_CONFIG_MBSSID
		wlband = Entry.wlanBand;
#else
		mib_get_s(MIB_WLAN_BAND, (void *)&wlband, sizeof(wlband));
#endif
		boaWrite(wp, "\t_ssid_band[%d]=%d;\n", k, wlband-1);
		boaWrite(wp, "\t_encrypt[%d]=%d;\n", k, Entry.encrypt);
		boaWrite(wp, "\t_enable1X[%d]=%d;\n", k, Entry.enable1X);
		boaWrite(wp, "\t_wpaAuth[%d]=%d;\n", k, Entry.wpaAuth);
		boaWrite(wp, "\t_wpaPSKFormat[%d]=%d;\n", k, Entry.wpaPSKFormat);
		//boaWrite(wp, "\t_wpaPSK[%d]='%s';\n", k, strbuf);
		memset(password, 0, sizeof(password));
		rtk_util_data_base64encode(Entry.wpaPSK, password, sizeof(password));
		//rtk_util_convert_to_star_string(password,strlen(Entry.wpaPSK));
		password[ENC_MAX_PSK_LEN]='\0';
		translate_web_code(password);
		boaWrite(wp, "\t_wpaPSK[%d]='%s';\n", k, password);	//fix web check psk-key invalid problem
		boaWrite(wp, "\t_wpaGroupRekeyTime[%d]='%lu';\n", k, Entry.wpaGroupRekeyTime);
#ifdef WLAN_WPA3_H2E
		boaWrite(wp, "\t_wpa3_sae_pwe[%d]=%d;\n", k, Entry.sae_pwe);
#endif
		boaWrite(wp, "\t_rsPort[%d]=%d;\n", k, Entry.rsPort);
		boaWrite(wp, "\t_rsIpAddr[%d]='%s';\n", k, strbuf2);
		memset(password, 0, sizeof(password));
		//rtk_util_convert_to_star_string(password,strlen(Entry.rsPassword));
		rtk_util_data_base64encode(Entry.rsPassword, password, sizeof(password));
		password[ENC_MAX_PSK_LEN]='\0';
		boaWrite(wp, "\t_rsPassword[%d]='%s';\n", k, password);
#ifdef WLAN_RADIUS_2SET
		boaWrite(wp, "\t_rs2Port[%d]=%d;\n", k, Entry.rs2Port);
		boaWrite(wp, "\t_rs2IpAddr[%d]='%s';\n", k, strbuf3);
		memset(password, 0, sizeof(password));
		//rtk_util_convert_to_star_string(password,strlen(Entry.rs2Password));
		rtk_util_data_base64encode(Entry.rs2Password, password, sizeof(password));
		password[ENC_MAX_PSK_LEN]='\0';
		boaWrite(wp, "\t_rs2Password[%d]='%s';\n", k, password);
#endif
		boaWrite(wp, "\t_uCipher[%d]=%d;\n", k, Entry.unicastCipher);
		boaWrite(wp, "\t_wpa2uCipher[%d]=%d;\n", k, Entry.wpa2UnicastCipher);
		boaWrite(wp, "\t_wepAuth[%d]=%d;\n", k, Entry.authType);
		boaWrite(wp, "\t_wepLen[%d]=%d;\n", k, Entry.wep);
		boaWrite(wp, "\t_wepKeyFormat[%d]=%d;\n", k, Entry.wepKeyType);
		if(Entry.wepKeyType == 1) {
			memset(tmp_password, 0, sizeof(tmp_password));
			if(Entry.wep == 1) { //64bit
				memset(password_64, 0, sizeof(password_64));
				hex_to_string(Entry.wep64Key1, 2*sizeof(Entry.wep64Key1), tmp_password);
				rtk_util_data_base64encode(tmp_password, password_64, sizeof(password_64));
			}
			else { //128bit
				memset(password_128, 0, sizeof(password_128));
				hex_to_string(Entry.wep128Key1, 2*sizeof(Entry.wep128Key1), tmp_password);
				rtk_util_data_base64encode(tmp_password, password_128, sizeof(password_128));
			}
		}
		else {
			if(Entry.wep == 1) { //64bit
				memset(password_64, 0, sizeof(password_64));
				rtk_util_data_base64encode(Entry.wep64Key1, password_64, sizeof(password_64));
			}
			else { //128bit
				memset(password_128, 0, sizeof(password_128));
				rtk_util_data_base64encode(Entry.wep128Key1, password_128, sizeof(password_128));
			}
		}
		password_64[ENC_MAX_PSK_LEN]='\0';
		boaWrite(wp, "\t_wepKeyValue[%d]='%s';\n", k, strValToASP(password_64));
		password_128[ENC_MAX_PSK_LEN]='\0';
		boaWrite(wp, "\t_wepKey128Value[%d]='%s';\n", k, strValToASP(password_128));

#ifdef WLAN_BAND_CONFIG_MBSSID
		wlband = Entry.wlanBand;
#else
		mib_get_s(MIB_WLAN_BAND, (void *)&wlband, sizeof(wlband));
#endif
		isNmode=wl_isNband(wlband);
		boaWrite(wp, "\t_wlan_isNmode[%d]=%d;\n", k, isNmode);
#ifdef WLAN_11W
		boaWrite(wp, "\t_dotIEEE80211W[%d]=%d;\n", k, Entry.dotIEEE80211W);
		boaWrite(wp, "\t_sha256[%d]=%d;\n\t", k, Entry.sha256);
#else
		boaWrite(wp, "\t_dotIEEE80211W[%d]=%d;\n", k, 0);
		boaWrite(wp, "\t_sha256[%d]=%d;\n\t", k, 0);
#endif
		i++;
	}
	if(wlanDisabled)
		boaWrite(wp, "\tssid_num=0;\n");
	else
		boaWrite(wp, "\tssid_num=%d;\n", i);
	if(i==0 || wlanDisabled){
		boaWrite(wp, "\t%s.getElementById(\"wlan_security_table\").style.display = 'none';\n", DOCUMENT);
		if (wlanDisabled)
			boaWrite(wp, "%s.write(\"<font size=2> WLAN Disabled !</font>\")", DOCUMENT);
		else
#ifdef CONFIG_00R0
			boaWrite(wp, "%s.write(\"<font size=2> Please use Wi-Fi Radar Page to configure Client Security.</font>\")", DOCUMENT);
#else
			boaWrite(wp, "%s.write(\"<font size=2> Please use Site Survey Page to configure Client Security.</font>\")", DOCUMENT);
#endif
	}
	else{
		boaWrite(wp, "\t%s.getElementById(\"wlan_security_table\").style.display = \"\";\n", DOCUMENT);
	}
}

#ifdef WLAN_ACL
void initWlAclPage(request * wp)
{
	unsigned char vChar;
	MIB_CE_MBSSIB_T Entry;
	if(mib_chain_get(MIB_MBSSIB_TBL, 0, &Entry) == 0)
		printf("\n %s %d\n", __func__, __LINE__);

	if (Entry.wlanDisabled==0) // enabled
		boaWrite(wp,"wlanDisabled=0;\n");
	else // disabled
		boaWrite(wp,"wlanDisabled=1;\n");

	boaWrite(wp,"wlanMode=%d;\n", Entry.wlanMode);

#ifdef WLAN_VAP_ACL
	vChar = Entry.acl_enable;
#else
	mib_get_s( MIB_WLAN_AC_ENABLED, (void *)&vChar, sizeof(vChar));
#endif

	boaWrite(wp,"%s.formWlAcAdd.wlanAcEnabled.selectedIndex=%d\n", DOCUMENT, vChar);

}
#endif
#ifdef WLAN_MESH
void initWlMeshPage(request * wp)
{
	unsigned char vChar;
	MIB_CE_MBSSIB_T Entry;
	unsigned char mesh_id[32]={0};
	unsigned char mesh_wpaPSK[MAX_PSK_LEN+1]={0};
	mib_chain_get(MIB_MBSSIB_TBL, 0, &Entry);

	if (Entry.wlanDisabled==0) // enabled
		boaWrite(wp,"wlanDisabled=0;\n");
	else // disabled
		boaWrite(wp,"wlanDisabled=1;\n");

	boaWrite(wp,"wlanMode=%d;\n", Entry.wlanMode);

	mib_get_s( MIB_WLAN_MESH_ENABLE, (void *)&vChar, sizeof(vChar));
	boaWrite(wp,"mesh_enable=%d;\n", vChar);

	mib_get_s( MIB_WLAN_MESH_ID, (void *)mesh_id, sizeof(mesh_id));
	boaWrite(wp,"mesh_id=\"%s\";\n", mesh_id);

	mib_get_s( MIB_WLAN_MESH_WPA_PSK, (void *)mesh_wpaPSK, sizeof(mesh_wpaPSK));
	boaWrite(wp,"mesh_wpaPSK=\"%s\";\n", mesh_wpaPSK);

	mib_get_s( MIB_WLAN_MESH_ENCRYPT, (void *)&vChar, sizeof(vChar));
	boaWrite(wp,"encrypt=%d;\n", vChar);

	mib_get_s( MIB_WLAN_MESH_PSK_FORMAT, (void *)&vChar, sizeof(vChar));
	boaWrite(wp,"psk_format=%d;\n", vChar);

	mib_get_s( MIB_WLAN_MESH_CROSSBAND_ENABLED, (void *)&vChar, sizeof(vChar));
	boaWrite(wp,"crossband=%d;\n", vChar);

}
#ifdef WLAN_MESH_ACL_ENABLE
void initWlMeshAclPage(request * wp)
{
	unsigned char vChar;
	MIB_CE_MBSSIB_T Entry;
	mib_chain_get(MIB_MBSSIB_TBL, 0, &Entry);

	if (Entry.wlanDisabled==0) // enabled
		boaWrite(wp,"wlanDisabled=0;\n");
	else // disabled
		boaWrite(wp,"wlanDisabled=1;\n");

	boaWrite(wp,"wlanMode=%d;\n", Entry.wlanMode);

	mib_get_s( MIB_WLAN_MESH_ACL_ENABLED, (void *)&vChar, sizeof(vChar));

	boaWrite(wp,"%s.formMeshACLAdd.wlanAcEnabled.selectedIndex=%d\n", DOCUMENT, vChar);

}
#endif
#endif

#ifdef SUPPORT_FON_GRE
void initWlGrePage(request * wp)
{
	MIB_CE_MBSSIB_T wlEntry;

	unsigned char greEnable, freeGreEnable, closedGreEnable;
	unsigned char freeGreName[32], closedGreName[32];
	unsigned int  freeGreServer, closedGreServer;
	unsigned int  freeGreVlan, closedGreVlan;

	mib_get_s(MIB_FON_GRE_ENABLE, (void *)&greEnable, sizeof(greEnable));

	mib_get_s(MIB_WLAN_FREE_SSID_GRE_ENABLE, (void *)&freeGreEnable, sizeof(freeGreEnable));
	mib_get_s(MIB_WLAN_FREE_SSID_GRE_NAME, (void *)freeGreName, sizeof(freeGreName));
	mib_get_s(MIB_WLAN_FREE_SSID_GRE_SERVER, (void *)&freeGreServer, sizeof(freeGreServer));
	mib_get_s(MIB_WLAN_FREE_SSID_GRE_VLAN, (void *)&freeGreVlan, sizeof(freeGreVlan));

	mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_ENABLE, (void *)&closedGreEnable, sizeof(closedGreEnable));
	mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_NAME, (void *)closedGreName, sizeof(closedGreName));
	mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_SERVER, (void *)&closedGreServer, sizeof(closedGreServer));
	mib_get_s(MIB_WLAN_CLOSED_SSID_GRE_VLAN, (void *)&closedGreVlan, sizeof(closedGreVlan));

	boaWrite(wp,"%s.fon.fon_enable.checked = %s;\n", DOCUMENT,greEnable?"true":"false");

	boaWrite(wp,"%s.fon.grename1.value='%s';\n", DOCUMENT,freeGreName);
	boaWrite(wp,"%s.fon.fon_enable1.checked = %s;\n", DOCUMENT,freeGreEnable?"true":"false");
	if(wlan_getEntry(&wlEntry, 1)){
		boaWrite(wp,"%s.fon.ssid1.value='%s';\n", DOCUMENT,wlEntry.ssid);
	}
	boaWrite(wp,"%s.fon.wlanGw1.value='%s';\n", DOCUMENT,inet_ntoa(*((struct in_addr *)&freeGreServer)));
	boaWrite(wp,"%s.fon.vlantag1.value='%d';\n", DOCUMENT,freeGreVlan);

	boaWrite(wp,"%s.fon.grename2.value='%s';\n", DOCUMENT,closedGreName);
	boaWrite(wp,"%s.fon.fon_enable2.checked = %s;\n", DOCUMENT,closedGreEnable?"true":"false");
	if(wlan_getEntry(&wlEntry, 2)){
		printf("ssdi2=%s\n",wlEntry.ssid);
		boaWrite(wp,"%s.fon.ssid2.value='%s';\n", DOCUMENT,wlEntry.ssid);
		boaWrite(wp,"%s.fon.radiusPort.value='%d';\n", DOCUMENT,wlEntry.rsPort);
		boaWrite(wp,"%s.fon.radiusIP.value='%s';\n", DOCUMENT,inet_ntoa(*((struct in_addr *)&wlEntry.rsIpAddr)));
		boaWrite(wp,"%s.fon.radiusPass.value='%s';\n", DOCUMENT,wlEntry.rsPassword);
	}
	boaWrite(wp,"%s.fon.wlanGw2.value='%s';\n", DOCUMENT,inet_ntoa(*((struct in_addr *)&closedGreServer)));
	boaWrite(wp,"%s.fon.vlantag2.value='%d';\n", DOCUMENT,closedGreVlan);
}

void initWlGrePageStatus(request * wp)
{
	int status=0;
	boaWrite(wp,"%s.getElementById('fonstatus').innerHTML='%s';\n", DOCUMENT, status?"Connected":"Unconnected");
}
#endif
#endif // of WLAN_SUPPORT

#ifdef DIAGNOSTIC_TEST
void initDiagTestPage(request * wp)
{
	unsigned int inf;
	FILE *fp;

	if (fp = fopen("/tmp/diaginf", "r")) {
		if(fscanf(fp, "%d", &inf) != 1)
			printf("fscanf failed: %s %d\n", __func__, __LINE__);
		if (inf != DUMMY_IFINDEX)
			boaWrite(wp, "%s.diagtest.wan_if.value = %d;\n", DOCUMENT, inf);
		fclose(fp);
		fp = fopen("/tmp/diaginf", "w");
		if(fp==NULL)
			printf("Error opening  file /tmp/diaginf \n");
		fprintf(fp, "%d", DUMMY_IFINDEX); // reset to dummy
		fclose(fp);
	}
}
#endif

int ShowWmm(int eid, request * wp, int argc, char **argv)
{
#ifdef WLAN_QoS
#ifndef IS_AX_SUPPORT
	boaWrite(wp,
		"<tr><th>%s:</th>" \
		"<td>" \
		"<input type=\"radio\" name=WmmEnabled value=1>%s&nbsp;&nbsp;" \
		"<input type=\"radio\" name=WmmEnabled value=0>%s</td></tr>",
		multilang(LANG_WMM_SUPPORT), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
#endif
#endif
	return 0;
}

int ShowDot11k_v(int eid, request * wp, int argc, char **argv)
{
#ifdef WLAN_11K
	boaWrite(wp,
		"<tr><th>%s:</th>" \
		"<td>" \
		"<input type=\"radio\" name=dot11kEnabled value=1 onClick='wlDot11kChange()'>%s&nbsp;&nbsp;" \
		"<input type=\"radio\" name=dot11kEnabled value=0 onClick='wlDot11kChange()'>%s</td></tr>",
		multilang(LANG_DOT11K_SUPPORT), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
#endif
#ifdef WLAN_11V
	boaWrite(wp,
		"<tr id=\"dot11v\"  style=\"display:none\">" \
		"<th>%s:</th>" \
		"<td>" \
		"<input type=\"radio\" name=dot11vEnabled value=1>%s&nbsp;&nbsp;" \
		"<input type=\"radio\" name=dot11vEnabled value=0>%s</td></tr>",
		multilang(LANG_DOT11V_SUPPORT), multilang(LANG_ENABLED), multilang(LANG_DISABLED));
#endif
	return 0;
}

int extra_wlbasic(int eid, request * wp, int argc, char **argv)
{
#if defined(WLAN_SUPPORT)&&(!defined(WLAN_11AX) || defined(CONFIG_RTW_GBWC))
    unsigned int val;
    MIB_CE_MBSSIB_T Entry;
    if(mib_chain_get(MIB_MBSSIB_TBL, 0, &Entry) == 0)
	printf("\n %s %d\n", __func__, __LINE__);
    val = Entry.tx_restrict;
	boaWrite(wp,
        "<tr><th>%s:</th>" \
        "<td><input type=\"text\" name=tx_restrict size=5 maxlength=4 value=%u>&nbsp;Mbps&nbsp;(0:%s)</td></tr>",
        multilang(LANG_TX_RESTRICT), val, multilang(LANG_NO_RESTRICT));

    val = Entry.rx_restrict;
    boaWrite(wp,
        "<tr><th>%s:</th>" \
        "<td><input type=\"text\" name=rx_restrict size=5 maxlength=4 value=%u>&nbsp;Mbps&nbsp;(0:%s)</td></tr>",
        multilang(LANG_RX_RESTRICT), val, multilang(LANG_NO_RESTRICT));
#endif //defined(WLAN_SUPPORT)
	return 0;
}

int extra_wladvanced(int eid, request * wp, int argc, char **argv)
{
#if defined(WLAN_SUPPORT) && defined(CONFIG_ARCH_RTL8198F)
    unsigned int val;

    mib_get_s( MIB_WLAN_IAPP_ENABLED, (void *)&val, sizeof(val));
    boaWrite(wp,"<th>%s:</th>\n"
         "<td>\n"
         "<input type=\"radio\" name=\"iapp\" value=1%s>%s&nbsp;&nbsp;\n"
        "<input type=\"radio\" name=\"iapp\" value=0%s>%s</td></tr>\n",
        multilang(LANG_IAPP), (val==1?" checked":" "), multilang(LANG_ENABLED), (val==0?" checked":" "), multilang(LANG_DISABLED));

    mib_get_s( MIB_WLAN_STBC_ENABLED, (void *)&val, sizeof(val));
    boaWrite(wp,"<th>%s:</th>\n"
         "<td>\n"
         "<input type=\"radio\" name=\"tx_stbc\" value=1%s>%s&nbsp;&nbsp;\n"
        "<input type=\"radio\" name=\"tx_stbc\" value=0%s>%s</td></tr>\n",
        multilang(LANG_STBC), (val==1?" checked":" "), multilang(LANG_ENABLED), (val==0?" checked":" "), multilang(LANG_DISABLED));

    mib_get_s( MIB_WLAN_LDPC_ENABLED, (void *)&val, sizeof(val));
    boaWrite(wp,"<th>%s:</th>\n"
         "<td>\n"
         "<input type=\"radio\" name=\"tx_ldpc\" value=1%s>%s&nbsp;&nbsp;\n"
        "<input type=\"radio\" name=\"tx_ldpc\" value=0%s>%s</td></tr>\n",
        multilang(LANG_LDPC), (val==1?" checked":" "), multilang(LANG_ENABLED), (val==0?" checked":" "), multilang(LANG_DISABLED));
#if defined(TDLS_SUPPORT)
    mib_get_s( MIB_WLAN_TDLS_PROHIBITED, (void *)&val, sizeof(val));
    boaWrite(wp,"<th>%s:</th>\n"
         "<td>\n"
         "<input type=\"radio\" name=\"tdls_prohibited_\" value=1%s>%s&nbsp;&nbsp;\n"
        "<input type=\"radio\" name=\"tdls_prohibited_\" value=0%s>%s</td></tr>\n",
        multilang(LANG_TDLS_PROHIBITED), (val==1?" checked":" "), multilang(LANG_ENABLED), (val==0?" checked":" "), multilang(LANG_DISABLED));

    mib_get_s( MIB_WLAN_TDLS_CS_PROHIBITED, (void *)&val, sizeof(val));
    boaWrite(wp,"<th>%s:</th>\n"
         "<td>\n"
         "<input type=\"radio\" name=\"tdls_cs_prohibited_\" value=1%s>%s&nbsp;&nbsp;\n"
        "<input type=\"radio\" name=\"tdls_cs_prohibited_\" value=0%s>%s</td></tr>\n",
        multilang(LANG_TDLS_CHAN_SW_PROHIBITED), (val==1?" checked":" "), multilang(LANG_ENABLED), (val==0?" checked":" "), multilang(LANG_DISABLED));
#endif //TDLS_SUPPORT
#endif //defined(WLAN_SUPPORT) && defined(CONFIG_ARCH_RTL8198F)
	return 0;
}

#ifdef CONFIG_USER_DHCP_SERVER
void initDhcpMode(request * wp)
{
	unsigned char vChar;
	char buf[16];

// Kaohj --- assign DHCP pool ip prefix; no pool prefix for complete IP pool
#ifdef DHCPS_POOL_COMPLETE_IP
	boaWrite(wp, "pool_ipprefix='';\n");
#else
	getSYS2Str(SYS_DHCPS_IPPOOL_PREFIX, buf);
	boaWrite(wp, "pool_ipprefix='%s';\n", buf);
#endif
// Kaohj
#ifdef DHCPS_DNS_OPTIONS
	boaWrite(wp, "en_dnsopt=1;\n");
	mib_get_s(MIB_DHCP_DNS_OPTION, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "dnsopt=%d;\n", vChar);
#else
	boaWrite(wp, "en_dnsopt=0;\n");
#endif
	mib_get_s( MIB_DHCP_MODE, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.dhcpd.dhcpdenable[%d].checked = true;\n", DOCUMENT, vChar);
}
#endif

#ifdef CONFIG_IPV6
#ifdef DHCPV6_ISC_DHCP_4XX
void initDhcpv6Mode(request * wp)
{
	unsigned char vChar;
	char buf[16];

	mib_get_s( MIB_DHCPV6_MODE, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.dhcpd.dhcpdenable[%d].checked = true;\n", DOCUMENT, vChar);

	if(vChar == DHCP_LAN_SERVER)
		boaWrite(wp, "%s.getElementById(\"dhcpV6setting\").hidden = false;\n", DOCUMENT, vChar);
	mib_get_s( MIB_DHCPV6S_TYPE, (void *)&vChar, sizeof(vChar));
	boaWrite(wp, "%s.dhcpd.dhcpdv6Type[%d].checked = true;\n", DOCUMENT, vChar);
}
#endif
#endif

void initDhcpMacbase(request * wp)
{
	char buf[16];
// Kaohj --- assign DHCP pool ip prefix; no pool prefix for complete IP pool
#ifdef DHCPS_POOL_COMPLETE_IP
	boaWrite(wp, "pool_ipprefix='';\n");
#else
	getSYS2Str(SYS_DHCPS_IPPOOL_PREFIX, buf);
	boaWrite(wp, "pool_ipprefix='%s';\n", buf);
#endif
}

/*ping_zhang:20090319 START:replace ip range with serving pool of tr069*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
#ifdef IMAGENIO_IPTV_SUPPORT
void initDhcpIPRange(request * wp)
{
	char buf[16];
	unsigned int i, entryNum;
	DHCPS_SERVING_POOL_T Entry;
	MIB_CE_DHCP_OPTION_T rsvOptEntry;
	char startIp[16], endIp[16];
/*ping_zhang:20090526 START:Add gateway for each ip range*/
	char gwIp[16];
/*ping_zhang:20090526 END*/
	int id=-1;

	entryNum = mib_chain_total(MIB_DHCPS_SERVING_POOL_TBL);
	boaWrite(wp, "var devname=new Array(%d), devtype=new Array(%d), startip=new Array(%d), endip=new Array(%d), gwip=new Array(%d), option=new Array(%d), opCode=new Array(%d), opStr=new Array(%d);\n",
			entryNum, entryNum, entryNum, entryNum, entryNum, entryNum, entryNum, entryNum);

// Kaohj --- assign DHCP pool ip prefix; no pool prefix for complete IP pool
#ifdef DHCPS_POOL_COMPLETE_IP
	boaWrite(wp, "pool_ipprefix='';\n");
#else
	getSYS2Str(SYS_DHCPS_IPPOOL_PREFIX, buf);
	boaWrite(wp, "pool_ipprefix='%s';\n", buf);
#endif

	for (i=0; i<entryNum; i++) {

		mib_chain_get(MIB_DHCPS_SERVING_POOL_TBL, i, (void *)&Entry);
		strcpy(startIp, inet_ntoa(*((struct in_addr *)Entry.startaddr)));
		strcpy(endIp, inet_ntoa(*((struct in_addr *)Entry.endaddr)));
/*ping_zhang:20090526 START:Add gateway for each ip range*/
		strcpy(gwIp, inet_ntoa(*((struct in_addr *)Entry.iprouter)));
/*ping_zhang:20090526 END*/

		boaWrite(wp, "devname[%d]=\'%s\';\n", i, Entry.poolname);
		boaWrite(wp, "devtype[%d]=\'%d\';\n", i, Entry.deviceType);
		boaWrite(wp, "startip[%d]=\'%s\';\n", i, startIp);
		boaWrite(wp, "endip[%d]=\'%s\';\n", i, endIp);
/*ping_zhang:20090526 START:Add gateway for each ip range*/
		boaWrite(wp, "gwip[%d]=\'%s\';\n", i, gwIp);
/*ping_zhang:20090526 END*/
		boaWrite(wp, "option[%d]=\'%s\';\n", i, Entry.vendorclass);
		boaWrite(wp, "opCode[%d]=\'%d\';\n", i, Entry.rsvOptCode);

		getSPDHCPRsvOptEntryByCode(Entry.InstanceNum, Entry.rsvOptCode, &rsvOptEntry, &id);
		if(id != -1)
			boaWrite(wp, "opStr[%d]=\'%s\';\n", i, rsvOptEntry.value);
		else
			boaWrite(wp, "opStr[%d]=\'\';\n");
	}
}
#endif
#endif
/*ping_zhang:20090319 END*/

#ifdef ADDRESS_MAPPING
#ifdef MULTI_ADDRESS_MAPPING
initAddressMap(request * wp)
{
	boaWrite(wp, "%s.addressMap.lsip.disabled = false;\n", DOCUMENT);
	boaWrite(wp, "%s.addressMap.leip.disabled = true;\n", DOCUMENT);
	boaWrite(wp, "%s.addressMap.gsip.disabled = false;\n", DOCUMENT);
	boaWrite(wp, "%s.addressMap.geip.disabled = true;\n", DOCUMENT);
	boaWrite(wp, "%s.addressMap.addressMapType.selectedIndex= 0;\n", DOCUMENT);
}
#else
initAddressMap(request * wp)
{
	unsigned char vChar;

	mib_get_s( MIB_ADDRESS_MAP_TYPE, (void *)&vChar, sizeof(vChar));

	if(vChar == ADSMAP_NONE) {         // None
		boaWrite(wp, "%s.addressMap.lsip.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.leip.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.gsip.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.geip.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.addressMapType.selectedIndex= 0;\n", DOCUMENT);

	} else if (vChar == ADSMAP_ONE_TO_ONE) {  // One-to-One
		boaWrite(wp, "%s.addressMap.lsip.disabled = false;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.leip.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.gsip.disabled = false;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.geip.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.addressMapType.selectedIndex= 1;\n", DOCUMENT);

	} else if (vChar == ADSMAP_MANY_TO_ONE) {  // Many-to-One
		boaWrite(wp, "%s.addressMap.lsip.disabled = false;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.leip.disabled = false;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.gsip.disabled = false;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.geip.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.addressMapType.selectedIndex= 2;\n", DOCUMENT);

	} else if (vChar == ADSMAP_MANY_TO_MANY) {   // Many-to-Many
		boaWrite(wp, "%s.addressMap.lsip.disabled = false;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.leip.disabled = false;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.gsip.disabled = false;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.geip.disabled = false;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.addressMapType.selectedIndex= 3;\n", DOCUMENT);

	}
	// Masu Yu on True
#if 1
	else if (vChar == ADSMAP_ONE_TO_MANY) {   // One-to-Many
		boaWrite(wp, "%s.addressMap.lsip.disabled = false;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.leip.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.gsip.disabled = false;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.geip.disabled = false;\n", DOCUMENT);
		boaWrite(wp, "%s.addressMap.addressMapType.selectedIndex= 4;\n", DOCUMENT);

	}
#endif
}
#endif // MULTI_ADDRESS_MAPPING
#endif

#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
//ql
void initOspf(request * wp)
{
	unsigned char vChar;

#ifdef CONFIG_USER_ROUTED_ROUTED
	mib_get_s( MIB_RIP_ENABLE, (void *)&vChar, sizeof(vChar) );
	if (1 == vChar) {
		boaWrite(wp, "%s.rip.igp.selectedIndex = 0;\n", DOCUMENT);
		boaWrite(wp, "%s.rip.rip_on[1].checked = true;\n", DOCUMENT);
		return;
	}
	mib_get_s( MIB_OSPF_ENABLE, (void *)&vChar, sizeof(vChar));
	if (1 == vChar) {
		boaWrite(wp, "%s.rip.igp.selectedIndex = 1;\n", DOCUMENT);
		boaWrite(wp, "%s.rip.rip_on[1].checked = true;\n", DOCUMENT);
		return;
	}
#else
	mib_get_s( MIB_OSPF_ENABLE, (void *)&vChar, sizeof(vChar));
	if (1 == vChar) {
		boaWrite(wp, "%s.rip.igp.selectedIndex = 0;\n", DOCUMENT);
		boaWrite(wp, "%s.rip.rip_on[1].checked = true;\n", DOCUMENT);
		return;
	}
#endif
	//default
	boaWrite(wp, "%s.rip.igp.selectedIndex = 0;\n", DOCUMENT);
	boaWrite(wp, "%s.rip.rip_on[0].checked = true;\n", DOCUMENT);
}
#endif

#ifdef FIELD_TRY_SAFE_MODE
#ifdef CONFIG_DEV_xDSL
void initAdslSafeMode(request * wp)
{
	SafeModeData vSmd;

	memset((void *)&vSmd, 0, sizeof(vSmd));
	adsl_drv_get(RLCM_GET_SAFEMODE_CTRL, (void *)&vSmd, SAFEMODE_DATA_SIZE);
	if (vSmd.FieldTrySafeMode > 0)
		boaWrite(wp, "%s.adsl_safe.safemode.checked = true;\n", DOCUMENT);
	boaWrite(wp, "%s.adsl_safe.psdtime.value = \"%d\";\n", DOCUMENT, vSmd.FieldTryTestPSDTimes);
	boaWrite(wp, "%s.adsl_safe.ctrlin.value = \"%x\";\n", DOCUMENT, vSmd.FieldTryCtrlIn);
}
#endif
#endif

#ifdef CONFIG_ETHWAN
void initEthWan(request * wp)
{
	MIB_CE_ATM_VC_T Entry;
	int index;
#ifdef CONFIG_IPV6
	unsigned char 	Ipv6AddrStr[INET6_ADDRSTRLEN], RemoteIpv6AddrStr[INET6_ADDRSTRLEN];
#ifdef CONFIG_RTK_DEV_AP
#ifdef CONFIG_IPV6_SIT_6RD
	unsigned char	SixrdBRv4IP[INET_ADDRSTRLEN]={0};
#endif
#endif
#endif

	index = getWanEntrybyMedia(&Entry, MEDIA_ETH);
	if (index == -1)
		printf("EthWan interface not found !\n");

#ifdef PPPOE_PASSTHROUGH
	if((Entry.brmode==BRIDGE_ETHERNET) || (Entry.brmode==BRIDGE_PPPOE))
	{
		boaWrite(wp, "%s.ethwan.br.checked = %s;\n", DOCUMENT, "true");
		boaWrite(wp, "%s.getElementById('brmode').value=\"%d\";\n", DOCUMENT,Entry.brmode);
	}
	else
	{
		boaWrite(wp, "%s.ethwan.br.checked = %s;\n", DOCUMENT, "false");
	}
#endif
#ifdef CONFIG_USER_MAC_CLONE
	unsigned char macclone_addr[13];
	memset(macclone_addr, 0, sizeof(macclone_addr));
	hex_to_string(Entry.macclone, 12, macclone_addr);
	macclone_addr[12] = '\0';
	boaWrite(wp, "%s.ethwan.macclone.value=%d;\n",DOCUMENT, Entry.macclone_enable);
	if(Entry.macclone_enable==0){
		boaWrite(wp, "%s.getElementById(\"tb_macclone\").style.display = \"none\";\n",DOCUMENT);
	}
	else{
		boaWrite(wp, "%s.getElementById(\"tb_macclone\").style.display = \"\";\n",DOCUMENT);
		boaWrite(wp, "%s.ethwan.maccloneaddr.value = \"%s\";\n",DOCUMENT,macclone_addr);
	}
#endif

	boaWrite(wp, "%s.ethwan.naptEnabled.checked = %s;\n", DOCUMENT, Entry.napt?"true":"false");
#ifdef CONFIG_IGMPPROXY_MULTIWAN
	boaWrite(wp, "%s.ethwan.igmpEnabled.checked = %s;\n", DOCUMENT, Entry.enableIGMP?"true":"false");
#endif
#ifdef CONFIG_MLDPROXY_MULTIWAN
	boaWrite(wp, "%s.ethwan.mldEnabled.checked = %s;\n", DOCUMENT, Entry.enableMLD?"true":"false");
#endif
#ifdef CONFIG_USER_IP_QOS
	boaWrite(wp, "%s.ethwan.qosEnabled.checked = %s;\n", DOCUMENT, Entry.enableIpQos?"true":"false");
#endif

	boaWrite(wp, "%s.ethwan.ipMode[%d].checked = true;\n", DOCUMENT, Entry.ipDhcp == 0? 0:1);
	if((Entry.cmode == CHANNEL_MODE_IPOE) ||(Entry.cmode == CHANNEL_MODE_6RD)
		|| (Entry.cmode == CHANNEL_MODE_6IN4)|| (Entry.cmode == CHANNEL_MODE_6TO4)){//mer
#ifdef CONFIG_RTK_DEV_AP
	boaWrite(wp, "%s.getElementById('tbl_ppp').style.display='none';\n", DOCUMENT);
#endif
#ifdef CONFIG_IPV6
		if (Entry.IpProtocol & IPVER_IPV4) {
#endif
		if(Entry.ipDhcp == 0){//ip
#ifdef CONFIG_RTK_DEV_AP
			boaWrite(wp, "%s.ethwan.ip.disabled = false;\n", DOCUMENT);
			boaWrite(wp, "%s.ethwan.remoteIp.disabled = false;\n", DOCUMENT);
			boaWrite(wp, "%s.ethwan.netmask.disabled = false;\n", DOCUMENT);
#endif
			boaWrite(wp, "%s.ethwan.ip.value = \"%s\";\n", DOCUMENT, inet_ntoa(*((struct in_addr *)&Entry.ipAddr)));
			//printf("ip %s\n", Entry.ipAddr);
			boaWrite(wp, "%s.ethwan.remoteIp.value = \"%s\";\n", DOCUMENT, inet_ntoa(*((struct in_addr *)&Entry.remoteIpAddr)));
			boaWrite(wp, "%s.ethwan.netmask.value = \"%s\";\n", DOCUMENT, inet_ntoa(*((struct in_addr *)&Entry.netMask)));
		}
#ifdef CONFIG_RTK_DEV_AP
		if(Entry.ipDhcp == 1)
		{
			boaWrite(wp, "%s.ethwan.ip.disabled = true;\n", DOCUMENT);
			boaWrite(wp, "%s.ethwan.remoteIp.disabled = true;\n", DOCUMENT);
			boaWrite(wp, "%s.ethwan.netmask.disabled = true;\n", DOCUMENT);
		}
		if(Entry.dnsMode)
		{
			boaWrite(wp, "%s.ethwan.dnsMode[0].checked = true;\n", DOCUMENT);
			//boaWrite(wp, "%s.ethwan.dns1.disabled = true;\n", DOCUMENT);
			//boaWrite(wp, "%s.ethwan.dns2.disabled = true;\n", DOCUMENT);
		}
		else
		{
			boaWrite(wp, "%s.ethwan.dnsMode[1].checked = true;\n", DOCUMENT);

			//boaWrite(wp, "%s.ethwan.dns1.disabled = false;\n", DOCUMENT);
			//boaWrite(wp, "%s.ethwan.dns2.disabled = false;\n", DOCUMENT);
			boaWrite(wp, "%s.ethwan.dns1.value = \"%s\";\n", DOCUMENT, inet_ntoa(*((struct in_addr *)&Entry.v4dns1)));
			boaWrite(wp, "%s.ethwan.dns2.value = \"%s\";\n", DOCUMENT, inet_ntoa(*((struct in_addr *)&Entry.v4dns2)));
		}
#ifdef CONFIG_IPV6_SIT_6RD
		if (Entry.cmode == CHANNEL_MODE_6RD)
		{
			inet_ntop(PF_INET, (struct in_addr *)Entry.SixrdBRv4IP, SixrdBRv4IP, sizeof(SixrdBRv4IP));
			inet_ntop(PF_INET6, Entry.SixrdPrefix,Ipv6AddrStr , sizeof(Ipv6AddrStr));
			boaWrite(wp, "%s.ethwan.SixrdBRv4IP.value=\"%s\";\n", DOCUMENT, SixrdBRv4IP);
			boaWrite(wp, "%s.ethwan.SixrdIPv4MaskLen.value=\"%d\";\n", DOCUMENT, Entry.SixrdIPv4MaskLen);
			boaWrite(wp, "%s.ethwan.SixrdPrefix.value=\"%s\";\n", DOCUMENT, Ipv6AddrStr);
			boaWrite(wp, "%s.ethwan.SixrdPrefixLen.value=\"%d\";\n", DOCUMENT, Entry.SixrdPrefixLen);
		}
#endif
#endif
#ifdef CONFIG_IPV6
		}
#endif

	}
	else if(Entry.cmode == CHANNEL_MODE_PPPOE){//pppoe
	#ifdef CONFIG_RTK_DEV_AP
		boaWrite(wp, "%s.getElementById('tbl_ip').style.display='none';\n", DOCUMENT);
	#endif
		boaWrite(wp, "%s.ethwan.pppUserName.value = \"%s\";\n", DOCUMENT, Entry.pppUsername);
		boaWrite(wp, "%s.ethwan.pppPassword.value = \"%s\";\n", DOCUMENT, Entry.pppPassword);

	#ifdef CONFIG_RTK_DEV_AP
		boaWrite(wp, "%s.ethwan.pppConnectType.selectedIndex = \"%d\";\n", DOCUMENT, Entry.pppCtype);
		if(Entry.pppCtype == CONNECT_ON_DEMAND)//connect on demand
			boaWrite(wp, "%s.ethwan.pppIdleTime.value = \"%d\";\n", DOCUMENT, Entry.pppIdleTime);

		boaWrite(wp, "%s.ethwan.auth.selectedIndex = \"%d\";\n", DOCUMENT, Entry.pppAuth);
		boaWrite(wp, "%s.ethwan.acName.value = \"%s\";\n", DOCUMENT, Entry.pppACName);
		boaWrite(wp, "%s.ethwan.serviceName.value = \"%s\";\n", DOCUMENT, Entry.pppServiceName);
	#else
		boaWrite(wp, "%s.ethwan.pppConnectType[%d].checked = true;\n", DOCUMENT, Entry.pppCtype);
		if(Entry.pppCtype == '1'){//connect on demand
			boaWrite(wp, "%s.ethwan.pppConnectType.value = \"%d\";\n", DOCUMENT, Entry.pppIdleTime);
		}
	#endif
	}
	#ifdef CONFIG_RTK_DEV_AP
	else if(Entry.cmode == CHANNEL_MODE_BRIDGE){
		boaWrite(wp, "%s.getElementById('tbl_ppp').style.display='none';\n", DOCUMENT);
		boaWrite(wp, "%s.getElementById('tbl_ip').style.display='none';\n", DOCUMENT);

		boaWrite(wp, "%s.ethwan.naptEnabled.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.ethwan.igmpEnabled.disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.ethwan.droute[0].disabled = true;\n", DOCUMENT);
		boaWrite(wp, "%s.ethwan.droute[1].disabled = true;\n", DOCUMENT);
	}
	#endif

	if (Entry.cmode != CHANNEL_MODE_BRIDGE)
		boaWrite(wp, "%s.ethwan.droute[%d].checked = true;\n", DOCUMENT, Entry.dgw);
#ifdef CONFIG_IPV6
	boaWrite(wp, "%s.ethwan.IpProtocolType.value=%d;\n", DOCUMENT, Entry.IpProtocol);
	boaWrite(wp, "%s.ethwan.AddrMode.value=%d;\n", DOCUMENT,Entry.AddrMode);
	inet_ntop(PF_INET6, Entry.Ipv6Addr, Ipv6AddrStr, sizeof(Ipv6AddrStr));
	inet_ntop(PF_INET6, Entry.RemoteIpv6Addr, RemoteIpv6AddrStr, sizeof(RemoteIpv6AddrStr));
	boaWrite(wp, "%s.ethwan.Ipv6Addr.value=\"%s\";\n", DOCUMENT, Ipv6AddrStr);
	boaWrite(wp, "%s.ethwan.Ipv6PrefixLen.value=%d;\n", DOCUMENT, Entry.Ipv6AddrPrefixLen);
	boaWrite(wp, "%s.ethwan.Ipv6Gateway.value=\"%s\";\n", DOCUMENT, RemoteIpv6AddrStr);
	if (Entry.Ipv6DhcpRequest&M_DHCPv6_REQ_IANA)
		boaWrite(wp, "%s.ethwan.iana.checked=true;\n", DOCUMENT);
	else
		boaWrite(wp, "%s.ethwan.iana.checked=false;\n", DOCUMENT);
	if (Entry.Ipv6DhcpRequest&M_DHCPv6_REQ_IAPD)
		boaWrite(wp, "%s.ethwan.iapd.checked=true;\n", DOCUMENT);
	else
		boaWrite(wp, "%s.ethwan.iapd.checked=false;\n", DOCUMENT);
	boaWrite(wp, "ipver=%s.ethwan.IpProtocolType.value;\n", DOCUMENT);
#ifdef CONFIG_USER_NDPPD
	boaWrite(wp, "%s.ethwan.ndp_proxy.checked = %s;\n", DOCUMENT, Entry.ndp_proxy?"true":"false");
#endif
	boaWrite(wp, "%s.ethwan.napt_v6.selectedIndex = %d;\n", DOCUMENT, Entry.napt_v6);
#ifdef CONFIG_RTK_DEV_AP
	if (Entry.dnsv6Mode)
	{
		boaWrite(wp, "%s.ethwan.dnsV6Mode[0].checked=true;\n", DOCUMENT);
	}
	else
	{
		boaWrite(wp, "%s.ethwan.dnsV6Mode[1].checked=true;\n", DOCUMENT);
		inet_ntop(PF_INET6, Entry.Ipv6Dns1, Ipv6AddrStr, sizeof(Ipv6AddrStr));
		boaWrite(wp, "%s.ethwan.Ipv6Dns1.value=\"%s\";\n", DOCUMENT, Ipv6AddrStr);
		inet_ntop(PF_INET6, Entry.Ipv6Dns2, Ipv6AddrStr, sizeof(Ipv6AddrStr));
		boaWrite(wp, "%s.ethwan.Ipv6Dns2.value=\"%s\";\n", DOCUMENT, Ipv6AddrStr);
	}
	if (Entry.dslite_enable)
	{
		boaWrite(wp, "%s.ethwan.dslite_enable.checked=true;\n", DOCUMENT);
		boaWrite(wp, "%s.ethwan.dslite_aftr_mode.selectedIndex = \"%d\";\n", DOCUMENT, Entry.dslite_aftr_mode);
		if(Entry.dslite_aftr_mode)
		{
			boaWrite(wp, "%s.ethwan.dslite_aftr_hostname.value=\"%s\";\n", DOCUMENT,  Entry.dslite_aftr_hostname);
		}
	}
	else
	{
		boaWrite(wp, "%s.ethwan.dslite_enable.checked=false;\n", DOCUMENT);
	}
#endif

//IPv6 MAP-E
#ifdef CONFIG_USER_MAP_E
	extern void initMAPE(request * wp, MIB_CE_ATM_VC_Tp entryP);
	initMAPE(wp, &Entry);
#endif
//IPv6 MAP-T
#ifdef CONFIG_USER_MAP_T
	extern void initMAPT(request * wp, MIB_CE_ATM_VC_Tp entryP);
	initMAPT(wp, &Entry);
#endif
//IPv6 XLAT464
#ifdef CONFIG_USER_XLAT464
	extern void initXLAT464(request * wp, MIB_CE_ATM_VC_Tp entryP);
	initXLAT464(wp, &Entry);
#endif
//IPv6 LW4O6
#ifdef CONFIG_USER_LW4O6
	extern void initLW4O6(request * wp, MIB_CE_ATM_VC_Tp entryP);
	initLW4O6(wp, &Entry);
#endif

#endif
}
#endif

#ifdef CONFIG_USER_PPTPD_PPTPD
void initPptp(request * wp)
{
	MIB_VPND_T entry;
	int total, i;
	char peeraddr[16];
	char localaddr[16];

	total = mib_chain_total(MIB_VPN_SERVER_TBL);
	for (i=0; i<total; i++) {
		if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, &entry))
			continue;

		if (VPN_PPTP == entry.type)
			break;
	}

	if (i < total) {
		boaWrite(wp, "document.pptp.s_auth.selectedIndex=%d;\n", entry.authtype);
		boaWrite(wp, "\tdocument.pptp.s_enctype.selectedIndex=%d;\n", entry.enctype);
		sprintf(peeraddr, "%s", inet_ntoa(*(struct in_addr *)&entry.peeraddr));
		sprintf(localaddr, "%s", inet_ntoa(*(struct in_addr *)&entry.localaddr));
		boaWrite(wp, "\tdocument.pptp.peeraddr.value=\"%s\";\n", peeraddr);
		boaWrite(wp, "\tdocument.pptp.localaddr.value=\"%s\";\n", localaddr);
	}
}
#endif

//#if defined(CONFIG_USER_BOA_PRO_PASSTHROUGH) && defined(CONFIG_RTK_DEV_AP)
#ifdef CONFIG_USER_BOA_PRO_PASSTHROUGH
void initVPNPassThru(request * wp)
{
	unsigned char vChar;
	unsigned int entryNum,i;
	MIB_CE_ATM_VC_T Entry;
	unsigned int tmpNumInt;

    entryNum = mib_chain_total(MIB_ATM_VC_TBL);

	mib_get(TOTAL_WAN_CHAIN_NUM, (void *)&tmpNumInt);
	boaWrite(wp,"document.formVpnPassThru.totalNUM.value=%d;\n", tmpNumInt);

	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, "Get chain record error!\n");
			continue;
		}

		if(Entry.vpnpassthru_ipsec_enable==0){
	      boaWrite(wp,"document.formVpnPassThru.mutli_PassThru1WAN_%d.checked=false;\n", i);
		//  boaWrite(wp,"document.formVpnPassThru.mutli_PassThru1WAN_%d.value=\"OFF\";\n", i);
		}
		else{
	      boaWrite(wp,"document.formVpnPassThru.mutli_PassThru1WAN_%d.checked=true;\n", i);
		}

		if(Entry.vpnpassthru_pptp_enable==0){
	      boaWrite(wp,"document.formVpnPassThru.mutli_PassThru2WAN_%d.checked=false;\n", i);
		}
		else{
	      boaWrite(wp,"document.formVpnPassThru.mutli_PassThru2WAN_%d.checked=true;\n", i);
		}

		if(Entry.vpnpassthru_l2tp_enable==0){
	      boaWrite(wp,"document.formVpnPassThru.mutli_PassThru3WAN_%d.checked=false;\n", i);
		}
		else{
	      boaWrite(wp,"document.formVpnPassThru.mutli_PassThru3WAN_%d.checked=true;\n", i);
		}
	}
}
#endif


#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
void initL2tp(request * wp)
{
#ifdef CONFIG_USER_L2TPD_LNS
	MIB_VPND_T entry;
	int total, i;
	char peeraddr[16];
	char localaddr[16];
	int l2tpType;
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
	unsigned char IKEPreshareKey[IPSEC_NAME_LEN+1];
#endif

	boaWrite(wp, "document.l2tp.lns_en.value=1;\n"); /* enable l2tp lns */
	mib_get_s(MIB_L2TP_MODE, (void *)&l2tpType, sizeof(l2tpType));
	boaWrite(wp, "document.l2tp.ltype.value=%d;\n", l2tpType);
	total = mib_chain_total(MIB_VPN_SERVER_TBL);
	for (i=0; i<total; i++) {
		if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, &entry))
			continue;

		if (VPN_L2TP == entry.type)
			break;
	}

	if (i < total) {
		boaWrite(wp, "document.l2tp.s_name.value=\"%s\";\n", entry.name);
		boaWrite(wp, "document.l2tp.s_auth.selectedIndex=%d;\n", entry.authtype);
		boaWrite(wp, "\tdocument.l2tp.s_enctype.selectedIndex=%d;\n", entry.enctype);
		boaWrite(wp, "\tdocument.l2tp.s_tunnelAuth.checked=%s;\n", entry.tunnel_auth==1?"true":"false");
		boaWrite(wp, "\tdocument.l2tp.s_authKey.value=\"%s\";\n", entry.tunnel_key);
#if defined(CONFIG_USER_XL2TPD) && defined(CONFIG_USER_STRONGSWAN)
		memset(IKEPreshareKey, 0, sizeof(IKEPreshareKey));
		rtk_util_convert_to_star_string(IKEPreshareKey,strlen(entry.IKEPreshareKey));
		IKEPreshareKey[IPSEC_NAME_LEN]='\0';
		boaWrite(wp, "\tdocument.l2tp.overipsecLNS.checked=%s;\n", entry.overIPSec==1?"true":"false");
		boaWrite(wp, "\tdocument.l2tp.IKEPreshareKeyLNS.value=\"%s\";\n", IKEPreshareKey);
#endif
		sprintf(peeraddr, "%s", inet_ntoa(*(struct in_addr *)&entry.peeraddr));
		sprintf(localaddr, "%s", inet_ntoa(*(struct in_addr *)&entry.localaddr));
		boaWrite(wp, "\tdocument.l2tp.peeraddr.value=\"%s\";\n", peeraddr);
		boaWrite(wp, "\tdocument.l2tp.localaddr.value=\"%s\";\n", localaddr);
	}
#else
	boaWrite(wp, "document.l2tp.ltype.value=%d;\n", L2TP_MODE_LAC);
#endif
#ifdef CONFIG_IPV6
	boaWrite(wp, "document.l2tp.ipv6_en.value=1");
#endif
}
#endif

#if defined(CONFIG_USER_PPTP_CLIENT)
void initPptpVpn(request * wp)
{
#ifdef CONFIG_USER_PPTP_SERVER
	MIB_VPND_T entry;
	int total, i;
	char peeraddr[16];
	char localaddr[16];
	int pptpType;

	boaWrite(wp, "document.pptp.server_en.value=1;\n"); /* enable pptp server */
	mib_get_s(MIB_PPTP_MODE, (void *)&pptpType, sizeof(pptpType));
	boaWrite(wp, "document.pptp.ltype.value=%d;\n", pptpType);
	total = mib_chain_total(MIB_VPN_SERVER_TBL);
	for (i=0; i<total; i++) {
		if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, &entry))
			continue;

		if (VPN_PPTP == entry.type)
			break;
	}

	if (i < total) {
		boaWrite(wp, "document.pptp.s_name.value=\"%s\";\n", entry.name);
		boaWrite(wp, "document.pptp.s_auth.selectedIndex=%d;\n", entry.authtype);
		boaWrite(wp, "\tdocument.pptp.s_enctype.selectedIndex=%d;\n", entry.enctype);
		sprintf(peeraddr, "%s", inet_ntoa(*(struct in_addr *)&entry.peeraddr));
		//sprintf(localaddr, "%s", inet_ntoa(*(struct in_addr *)&entry.localaddr));
		getVpnServerLocalIP(VPN_PPTP, localaddr);
		boaWrite(wp, "\tdocument.pptp.peeraddr.value=\"%s\";\n", peeraddr);
		boaWrite(wp, "\tdocument.pptp.localaddr.value=\"%s\";\n", localaddr);
	}
#else
	boaWrite(wp, "document.pptp.ltype.value=%d;\n", PPTP_MODE_CLIENT);
#endif
}
#endif

int getwlanencrypt(int eid, request * wp, int argc, char **argv){
#ifdef WLAN_SUPPORT
	char *name;
	MIB_CE_MBSSIB_T Entry;
	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}
	if ( !strcmp(name, "root") ){
		wlan_getEntry(&Entry, 0);
		boaWrite(wp, "%d;\n",Entry.encrypt);
	}
	return -1;
#else
	return -1;
#endif
}

/////////////////////////////////////////////////////////////
// Kaohj
int initPage(int eid, request * wp, int argc, char **argv)
{
	char *name;
	struct user_info * pUser_info = NULL;
	pUser_info = search_login_list(wp);

	if (boaArgs(argc, argv, "%s", &name) < 1) {
		boaError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if ( !strcmp(name, "lan") )
		initLanPage(wp, pUser_info->priv);
#ifdef _CWMP_MIB_
	if ( !strcmp(name, "tr069Config") )
		initTr069ConfigPage(wp);
#endif
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_RADVD
	if ( !strcmp(name, "radvd_conf") )
		initRadvdConfPage(wp);
#endif
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
	if ( !strcmp(name, "ipv6_advance") )
		initIPv6AdvPage(wp);
#endif
	if ( !strcmp(name, "mldsnooping") )    // Mason Yu. MLD snooping
		initMLDsnoopPage(wp);
#endif
#ifdef CONFIG_DEV_xDSL
	if ( !strcmp(name, "setdsl") )
		initSetDslPage(wp);
	if ( !strcmp(name, "diagdsl") )
		initDiagDslPage(wp, xdsl_get_op(0) );
#ifdef CONFIG_USER_XDSL_SLAVE
	if ( !strcmp(name, "diagdslslv") )
		initDiagDslPage(wp, xdsl_get_op(1) );
#endif /*CONFIG_USER_XDSL_SLAVE*/
	if ( !strcmp(name, "adsl_tone_mask") )
		init_adsl_tone_mask(wp);
	if ( !strcmp(name, "adsl_psd_mask") )
		init_adsl_psd_mask(wp);
	if ( !strcmp(name, "psd_msm_mode") )
		init_psd_msm_mode(wp);
#ifdef CONFIG_VDSL
	if ( !strcmp(name, "vdsl2_check") )
		initVD2_check(wp);
	if ( !strcmp(name, "vdsl2_check_profile") )
		initVD2_check_profile(wp);
	if ( !strcmp(name, "vdsl2_updatefn") )
		initVD2_updatefn(wp);
	if ( !strcmp(name, "vdsl2_opt") )
		initVD2_opt(wp);
	if ( !strcmp(name, "vdsl2_profile") )
		initVD2_profile(wp);
#endif /*CONFIG_VDSL*/
#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
	if ( !strcmp(name, "gfast_check") )
		initGFast_check(wp);
	if ( !strcmp(name, "gfast_check_profile") )
		initGFast_check_profile(wp);
	if ( !strcmp(name, "gfast_opt") )
		initGFast_opt(wp);
	if ( !strcmp(name, "gfast_profile") )
		initGFast_profile(wp);
	if ( !strcmp(name, "gfast_updatefn") )
		initGFast_updatefn(wp);
#endif /* CONFIG_USER_CMD_SUPPORT_GFAST */
#endif
#ifdef PORT_TRIGGERING_STATIC
	if ( !strcmp(name, "gaming") )
		initGamingPage(wp);
#endif
#ifdef CONFIG_USER_ROUTED_ROUTED
	if ( !strcmp(name, "rip") )
		initRipPage(wp);
#endif
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
#ifdef ELAN_LINK_MODE
	if ( !strcmp(name, "linkMode") )
		initLinkPage(wp);
#endif
#else
#ifdef ELAN_LINK_MODE_INTRENAL_PHY
	if ( !strcmp(name, "linkMode") )
		initLinkPage(wp);
#endif
#endif	// of CONFIG_RTL_MULTI_LAN_DEV

#ifdef IP_PASSTHROUGH
	if ( !strcmp(name, "others") )
		initOthersPage(wp);
#endif
#ifdef TIME_ZONE
	if ( !strcmp(name, "ntp") )
		initNtpPage(wp);
#endif
#ifdef WLAN_SUPPORT
#ifdef WLAN_WPA
	if ( !strcmp(name, "wlwpa") )
		initWlWpaPage(wp);
#endif
#ifdef WLAN_11R
	if ( !strcmp(name, "wlft") )
		initWlFtPage(wp);
#endif
	// Mason Yu. 201009_new_security
	if ( !strcmp(name, "wlwpa_mbssid") )
		initWlWpaMbssidPage(wp);
	if ( !strcmp(name, "wlbasic") )
		initWlBasicPage(wp);
	if ( !strcmp(name, "wladv") )
		initWlAdvPage(wp);
#ifdef WLAN_MBSSID
	// Mason Yu
	if ( !strcmp(name, "wlmbssid") ) {
		initWLMBSSIDPage(wp);
	}
	if ( !strcmp(name, "wlmultipleap") ) {
		initWLMultiApPage(wp);
	}
#ifdef CONFIG_USER_FON
	if ( !strcmp(name, "wlfon") ) {
		initWLFonPage(wp);
	}
#endif
#endif
#ifdef WLAN_WDS
	if ( !strcmp(name, "wlwds") )
		initWlWDSPage(wp);
#endif
#if defined(WLAN_CLIENT) || defined(WLAN_SITESURVEY)
	if ( !strcmp(name, "wlsurvey") )
		initWlSurveyPage(wp);

#endif
#ifdef WLAN_ACL
	if ( !strcmp(name, "wlactrl") )
		initWlAclPage(wp);
#endif
#ifdef WLAN_MESH
	if(!strcmp(name, "wlmesh"))
		initWlMeshPage(wp);
#ifdef WLAN_MESH_ACL_ENABLE
	if(!strcmp(name, "wlmeshactrl"))
		initWlMeshAclPage(wp);
#endif
#endif
#ifdef SUPPORT_FON_GRE
	if ( !strcmp(name, "wlgre") )
		initWlGrePage(wp);
	if ( !strcmp(name, "wlgrestatus") )
		initWlGrePageStatus(wp);
#endif
#endif

#ifdef DIAGNOSTIC_TEST
	if ( !strcmp(name, "diagTest") )
		initDiagTestPage(wp);
#endif

#ifdef DOS_SUPPORT
	if ( !strcmp(name, "dos") )
		initDosPage(wp);
#endif


#ifdef ADDRESS_MAPPING
	if( !strcmp(name, "addressMap"))
		initAddressMap(wp);
#endif

#ifdef CONFIG_USER_DHCP_SERVER
	if ( !strcmp(name, "dhcp-mode") )
		initDhcpMode(wp);
#endif
#ifdef CONFIG_IPV6
#ifdef DHCPV6_ISC_DHCP_4XX
	if ( !strcmp(name, "dhcpv6-mode") )
		initDhcpv6Mode(wp);
#endif
#endif
	if ( !strcmp(name, "dhcp-macbase") )
		initDhcpMacbase(wp);
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
#ifdef IMAGENIO_IPTV_SUPPORT
	if ( !strcmp(name, "dhcp-iprange") )
		initDhcpIPRange(wp);
#endif
#endif
//add by ramen
#ifdef CONFIG_USER_RTK_NAT_ALG_PASS_THROUGH
if (!strcmp(name, "algonoff"))
	initAlgOnOff(wp);
#endif
#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
	if (!strcmp(name, "ospf"))
		initOspf(wp);
#endif
	if (!strcmp(name, "syslog"))
		initSyslogPage(wp);
#ifdef WEB_ENABLE_PPP_DEBUG
	if ( !strcmp(name, "pppSyslog") )
		initPPPSyslog(wp);
#endif
	if (!strcmp(name, "holepunching"))
		initHolepunchingPage(wp);
	if (!strcmp(name, "dgw"))
		initDgwPage(wp);

	if (!strcmp(name, "ethoamY1731"))
		initEthoamY1731Page(wp);

	if (!strcmp(name, "ethoam8023ah"))
		initEthoam8023AHPage(wp);

#ifdef FIELD_TRY_SAFE_MODE
	if ( !strcmp(name, "adslsafemode") )
		initAdslSafeMode(wp);
#endif
#ifdef CONFIG_ETHWAN
	if ( !strcmp(name, "ethwan") )
		initEthWan(wp);
#endif
#ifdef CONFIG_USER_PPTPD_PPTPD
	if (!strcmp(name, "pptp"))
		initPptp(wp);
#endif
#if defined(CONFIG_USER_PPTP_CLIENT)
	if (!strcmp(name, "pptp-vpn"))
		initPptpVpn(wp);
#endif
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
	if (!strcmp(name, "l2tp"))
		initL2tp(wp);
#endif
//#if defined(CONFIG_USER_BOA_PRO_PASSTHROUGH) && defined(CONFIG_RTK_DEV_AP)
#ifdef CONFIG_USER_BOA_PRO_PASSTHROUGH
	if (!strcmp(name, "VPN PassThr"))
		initVPNPassThru(wp);
#endif
#ifdef CONFIG_USER_WIRED_8021X
	if (!strcmp(name, "config_802_1x"))
		initWired8021x(wp);
#endif

	return 0;
}

#ifdef DOS_SUPPORT
#define DOSENABLE	0x1
#define DOSSYSFLOODSYN	0x2
#define DOSSYSFLOODFIN	0x4
#define	DOSSYSFLOODUDP	0x8
#define DOSSYSFLOODICMP	0x10
#define DOSIPFLOODSYN	0x20
#define DOSIPFLOODFIN	0x40
#define DOSIPFLOODUDP	0x80
#define DOSIPFLOODICMP	0x100
#define DOSTCPUDPPORTSCAN 0x200
#define DOSPORTSCANSENSI  0x800000
#define DOSICMPSMURFENABLED	0x400
#define DOSIPLANDENABLED	0x800
#define DOSIPSPOOFENABLED	0x1000
#define DOSIPTEARDROPENABLED	0x2000
#define DOSPINTOFDEATHENABLED	0x4000
#define DOSTCPSCANENABLED	0x8000
#define DOSTCPSYNWITHDATAENABLED	0x10000
#define DOSUDPBOMBENABLED		0x20000
#define DOSUDPECHOCHARGENENABLED	0x40000
#define DOSSOURCEIPBLOCK		0x400000
#define DOSFLOODSNMP		0x2000000
#define DOSICMPEGNORE		0x4000000
#define DOSNTPATTACK		0x8000000
#define DOSARPSPOOFENABLED	0x10000000
void initDosPage(request * wp){
	unsigned int mode;
	unsigned int ratelimit_icmp = 0;

	mib_get_s( MIB_DOS_ENABLED, (void *)&mode, sizeof(mode));

	mib_get_s( MIB_DOS_RATELIMIT_ICMP_ENABLED,  (void *)&ratelimit_icmp, sizeof(ratelimit_icmp));

	if (mode & DOSENABLE){
		boaWrite(wp, "%s.DosCfg.dosEnabled.checked = true;\n", DOCUMENT);
#if 0 // unused for deonet
		if (mode & DOSSYSFLOODSYN)
			boaWrite(wp, "%s.DosCfg.sysfloodSYN.checked = true;\n", DOCUMENT);
		if (mode & DOSSYSFLOODFIN)
			boaWrite(wp, "%s.DosCfg.sysfloodFIN.checked = true;\n", DOCUMENT);
		if (mode & DOSSYSFLOODUDP)
			boaWrite(wp, "%s.DosCfg.sysfloodUDP.checked = true;\n", DOCUMENT);
		if (mode & DOSSYSFLOODICMP)
			boaWrite(wp, "%s.DosCfg.sysfloodICMP.checked = true;\n", DOCUMENT);
#endif
		if (mode & DOSIPFLOODSYN)
			boaWrite(wp, "%s.DosCfg.ipfloodSYN.checked = true;\n", DOCUMENT);
		if (mode & DOSIPFLOODFIN)
			boaWrite(wp, "%s.DosCfg.ipfloodFIN.checked = true;\n", DOCUMENT);
		if (mode & DOSIPFLOODUDP)
			boaWrite(wp, "%s.DosCfg.ipfloodUDP.checked = true;\n", DOCUMENT);
		if (ratelimit_icmp)
			boaWrite(wp, "%s.DosCfg.ratelimitICMP.checked = true;\n", DOCUMENT);
		if (mode & DOSIPFLOODICMP)
			boaWrite(wp, "%s.DosCfg.ipfloodICMP.checked = true;\n", DOCUMENT);
		if (mode & DOSTCPUDPPORTSCAN)
			boaWrite(wp, "%s.DosCfg.TCPUDPPortScan.checked = true;\n", DOCUMENT);
		if (mode & DOSPORTSCANSENSI)
			boaWrite(wp, "%s.DosCfg.portscanSensi.value = 1;\n", DOCUMENT);
		if (mode & DOSICMPSMURFENABLED)
			boaWrite(wp, "%s.DosCfg.ICMPSmurfEnabled.checked = true;\n", DOCUMENT);
		if (mode & DOSIPLANDENABLED)
			boaWrite(wp, "%s.DosCfg.IPLandEnabled.checked = true;\n", DOCUMENT);
		if (mode & DOSIPSPOOFENABLED)
			boaWrite(wp, "%s.DosCfg.IPSpoofEnabled.checked = true;\n", DOCUMENT);
		if (mode & DOSARPSPOOFENABLED)
			boaWrite(wp, "%s.DosCfg.ARPSpoofEnabled.checked = true;\n", DOCUMENT);
		if (mode & DOSIPTEARDROPENABLED)
			boaWrite(wp, "%s.DosCfg.IPTearDropEnabled.checked = true;\n", DOCUMENT);
		if (mode & DOSPINTOFDEATHENABLED)
			boaWrite(wp, "%s.DosCfg.PingOfDeathEnabled.checked = true;\n", DOCUMENT);
		if (mode & DOSTCPSCANENABLED)
			boaWrite(wp, "%s.DosCfg.TCPScanEnabled.checked = true;\n", DOCUMENT);
		if (mode & DOSTCPSYNWITHDATAENABLED)
			boaWrite(wp, "%s.DosCfg.TCPSynWithDataEnabled.checked = true;\n", DOCUMENT);
		if (mode & DOSUDPBOMBENABLED)
			boaWrite(wp, "%s.DosCfg.UDPBombEnabled.checked = true;\n", DOCUMENT);
		if (mode & DOSUDPECHOCHARGENENABLED)
			boaWrite(wp, "%s.DosCfg.UDPEchoChargenEnabled.checked = true;\n", DOCUMENT);
		if (mode & DOSSOURCEIPBLOCK)
			boaWrite(wp, "%s.DosCfg.sourceIPblock.checked = true;\n", DOCUMENT);
		if (mode & DOSFLOODSNMP)
			boaWrite(wp, "%s.DosCfg.portfloodUDP.checked = true;\n", DOCUMENT);
		if (mode & DOSICMPEGNORE)
			boaWrite(wp, "%s.DosCfg.IcmpEchoIgnoreEnabled.checked = true;\n", DOCUMENT);
		if (mode & DOSNTPATTACK)
			boaWrite(wp, "%s.DosCfg.NtpAttackBlockEnabled.checked = true;\n", DOCUMENT);
	}
}
#endif

void initSyslogPage(request * wp)
{
	boaWrite(wp, "changelogstatus();");
}

void initHolepunchingPage(request * wp)
{
	boaWrite(wp, "changeholestatus();");
}

void initDgwPage(request * wp)
{
#ifdef DEFAULT_GATEWAY_V2
	unsigned char dgwip[16];
	unsigned int dgw;
	mib_get_s(MIB_ADSL_WAN_DGW_ITF, (void *)&dgw, sizeof(dgw));
	getMIB2Str(MIB_ADSL_WAN_DGW_IP, dgwip);
	boaWrite(wp, "\tdgwstatus = %d;\n", dgw);
	boaWrite(wp, "\tgtwy = '%s';\n", dgwip);
#endif
#ifdef CONFIG_RTL_MULTI_PVC_WAN
	boaWrite(wp, "%s.getElementById('vlan_show').style.display = 'block';\n", DOCUMENT);
#else
	boaWrite(wp, "%s.getElementById('vlan_show').style.display = 'none';\n", DOCUMENT);
#endif

	// Kaohj, differentiate user-level from admin-level
	if (strstr(wp->pathname, "web/admin/"))
		boaWrite(wp, "%s.adsl.add.disabled = true;\n", DOCUMENT);
	else
		boaWrite(wp, "%s.adsl.add.disabled = false;\n", DOCUMENT);
}

void initEthoamY1731Page(request * wp)
{
#ifdef CONFIG_USER_Y1731
	char mode = 0, ccmEnable = 0, level;
	unsigned short myid, vid;
	char megid[Y1731_MEGID_LEN];
	char incl_iface[Y1731_INCL_IFACE_LEN];
	char loglevel[8];
	unsigned char ccm_interval;
	char ifname[IFNAMSIZ] = {0};

	mib_get_s(Y1731_MODE, (void *)&mode, sizeof(mode));
	mib_get_s(Y1731_MYID, (void *)&myid, sizeof(myid));
	mib_get_s(Y1731_VLANID, (void *)&vid, sizeof(vid));
	mib_get_s(Y1731_MEGLEVEL, (void *)&level, sizeof(level));
	mib_get_s(Y1731_MEGID, (void *)megid, sizeof(megid));
	mib_get_s(Y1731_CCM_INTERVAL, (void *)&ccm_interval, sizeof(ccm_interval));
	mib_get_s(Y1731_LOGLEVEL, (void *)loglevel, sizeof(loglevel));
	mib_get_s(Y1731_ENABLE_CCM, (void *)&ccmEnable, sizeof(ccmEnable));
	mib_get_s(Y1731_WAN_INTERFACE, (void *)ifname, sizeof(ifname));

	//boaWrite(wp, "document.getElementById('oamMode').checked =%u;\n", mode);
	if (mode==0)
		boaWrite(wp, "document.Y1731.oamMode[0].checked = %u;\n", 1);
	if (mode==1)
		boaWrite(wp, "document.Y1731.oamMode[1].checked = %u;\n", 1);
	if (mode==2)
		boaWrite(wp, "document.Y1731.oamMode[2].checked = %u;\n", 1);

	boaWrite(wp, "document.getElementById('myid').value = \"%u\";\n", myid);
	if (vid == 0)
		boaWrite(wp, "document.getElementById('vid').value = \"\";\n");
	else
		boaWrite(wp, "document.getElementById('vid').value = \"%u\";\n", vid);
	boaWrite(wp, "document.getElementById('meglevel').value = \"%u\";\n", level);
	boaWrite(wp, "document.getElementById('megid').value = \"%s\";\n", megid);
	boaWrite(wp, "document.getElementById('ccminterval').value = \"%u\";\n", ccm_interval);
	boaWrite(wp, "document.getElementById('loglevel').value = \"%s\";\n", loglevel);
	boaWrite(wp, "document.getElementById('ccmenable').checked = %u;\n", ccmEnable);

	boaWrite(wp, "ifIdx = \"%s\";\n", ifname);
	boaWrite(wp, "document.Y1731.ext_if.selectedIndex = 0;\n");
	boaWrite(wp, "for( i = 1; i < document.Y1731.ext_if.options.length; i++ )\n");
	boaWrite(wp, "{\n");
		boaWrite(wp, "if( ifIdx == document.Y1731.ext_if.options[i].value )\n");
			boaWrite(wp, "document.Y1731.ext_if.selectedIndex = i;\n");
	boaWrite(wp, "}\n");

	boaWrite(wp, "%s.getElementById(\"ShowY1731p1\").style.display = \"block\";\n", DOCUMENT);
	boaWrite(wp, "%s.getElementById(\"ShowY1731p2\").style.display = \"block\";\n", DOCUMENT);
#endif
}

void initEthoam8023AHPage(request * wp)
{
#ifdef CONFIG_USER_8023AH
	char Enable = 0, Active = 0;
	char ifname[IFNAMSIZ] = {0};

	mib_get_s(EFM_8023AH_MODE, (void *)&Enable, sizeof(Enable));
	mib_get_s(EFM_8023AH_ACTIVE, (void *)&Active, sizeof(Active));
	mib_get_s(EFM_8023AH_WAN_INTERFACE, (void *)ifname, sizeof(ifname));

	boaWrite(wp, "document.getElementById('oam8023ahMode').checked = %u;\n", Enable);
	boaWrite(wp, "document.getElementById('8023ahActive').checked = %u;\n", Active);
#ifdef CONFIG_USER_8023AH_WAN_INTF_SELECTION
	boaWrite(wp, "ifIdx = \"%s\";\n", ifname);
	boaWrite(wp, "document.Y1731.efm_8023ah_ext_if.selectedIndex = 0;\n");
	boaWrite(wp, "for( i = 1; i < document.Y1731.efm_8023ah_ext_if.options.length; i++ )\n");
	boaWrite(wp, "{\n");
		boaWrite(wp, "if( ifIdx == document.Y1731.efm_8023ah_ext_if.options[i].value )\n");
			boaWrite(wp, "document.Y1731.efm_8023ah_ext_if.selectedIndex = i;\n");
	boaWrite(wp, "}\n");
#endif

	boaWrite(wp, "%s.getElementById(\"Show8023ah\").style.display = \"block\";\n", DOCUMENT);
#endif
}

#ifdef WEB_ENABLE_PPP_DEBUG
void initPPPSyslog(request * wp)
{
	int enable = 0;
	FILE *fp;

	if (fp = fopen(PPP_SYSLOG, "r")) {
		fscanf(fp, "%d", &enable);
		fclose(fp);
	}
	boaWrite(wp, "%s.formSysLog.pppcap[%d].checked = true;\n", DOCUMENT, enable);
}
#endif

#ifdef CONFIG_USER_WIRED_8021X
void initWired8021x(request * wp)
{
	unsigned char wired8021xEnable[MAX_LAN_PORT_NUM] = {0};
	unsigned char writeStr[1024] = {0};
	unsigned char wired8021xMode;
	unsigned short wired8021xRadiasUdpPort;
	unsigned char rsPassword[MAX_PSK_LEN+1];
	unsigned char rsIpStr[32];
	unsigned int i;

	if(mib_get(MIB_WIRED_8021X_MODE, (void *)&wired8021xMode))
	{
		sprintf(writeStr, "document.config_802_1x.port_based_authentication.checked=%s;\n", (wired8021xMode==WIRED_8021X_MODE_PORT_BASED)?"true":"false");
		//printf(">>>>>> writeStr=%s\n", writeStr);
		boaWrite(wp, writeStr);
	}

	if(mib_get(MIB_WIRED_8021X_RADIAS_IP, (void *)rsIpStr))
	{
		sprintf(writeStr, "document.config_802_1x.radiusIP.value=\"%s\";\n", inet_ntoa(*((struct in_addr *)rsIpStr)));
		//printf(">>>>>> writeStr=%s\n", writeStr);
		boaWrite(wp, writeStr);
	}

	if(mib_get(MIB_WIRED_8021X_RADIAS_UDP_PORT, (void *)&wired8021xRadiasUdpPort))
	{
		sprintf(writeStr, "document.config_802_1x.radiusPort.value=%d;\n", wired8021xRadiasUdpPort);
		//printf(">>>>>> writeStr=%s\n", writeStr);
		boaWrite(wp, writeStr);
	}

	if(mib_get(MIB_WIRED_8021X_RADIAS_SECRET, (void *)rsPassword))
	{
		sprintf(writeStr, "document.config_802_1x.radiusPass.value=\"%s\";\n", rsPassword);
		//printf(">>>>>> writeStr=%s\n", writeStr);
		boaWrite(wp, writeStr);
	}

	if(mib_get(MIB_WIRED_8021X_ENABLE, (void *)wired8021xEnable))
	{
		for(i=0 ; i<MAX_LAN_PORT_NUM ; i++)
		{
			sprintf(writeStr, "document.config_802_1x.admin_state[%d].value=%d;\n", i, wired8021xEnable[i]);
			//printf(">>>>>> writeStr=%s\n", writeStr);
			boaWrite(wp, writeStr);
		}
	}
}
#endif


var __GLOBAL__ = {
	pageRoot: ''
};
function generateNav() {
	var navs = {
		active: 0,
		items: [
			{
				name: '<% multilang(LANG_STATUS); %>',
				sub: 0
			},
#ifdef CONFIG_00R0
			{
				name: '<% multilang(LANG_WIZARD); %>',
				sub: 1
			},
#endif
			{
				name: '<% multilang(LANG_LAN); %>',
				sub: 2
			},
#ifdef WLAN_SUPPORT
			{
				name: '<% multilang(LANG_WLAN); %>',
				sub: 3
			},
#endif
#ifndef CONFIG_SFU
			{
				name: '<% multilang(LANG_WAN); %>',
				sub: 4
			},
			{
				name: '<% multilang(LANG_SERVICES); %>',
				sub: 5
			},
#endif
#ifdef VOIP_SUPPORT
			{
				name: '<% multilang(LANG_VOIP); %>',
				sub: 6
			},
#endif
			{
				name: '<% multilang(LANG_ADVANCE); %>',
				sub: 7
			},
#ifndef CONFIG_SFU
			{
				name: '<% multilang(LANG_DIAGNOSTICS); %>',
				sub: 8
			},
#endif
			{
				name:  '<% multilang(LANG_ADMIN); %>',
				sub: 9
			},
			{
				name: '<% multilang(LANG_STATISTICS); %>',
				sub: 10
			}
#ifdef OSGI_SUPPORT
			,
			{
				name: '<% multilang(LANG_OSGI); %>',
				sub: 11
			}
#endif

/* not use for deonet
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
			,
			{
				name: '<% multilang(LANG_OPERATION_MODE); %>',
				sub: 12
			}
#endif
*/
		]
	};

	return navs;
}
/**
 * ��nav��������ģ��ƴ��������Ȼ����Ⱦ��ҳ��
 */
function renderNav() {
	var nav = generateNav(); //��õ�������
	var tpl = $('#nav-tmpl').html(); //���navģ������
	var html = juicer(tpl, nav);
	$('#nav').html(html); //��Ⱦ��ҳ��
}
/**
 * ���ɵڶ����͵������˵������ݽṹ
 */
function generateSide() {
	var side = [];
	var sub0, sub1, sub2, sub3, sub4, sub5, sub6, sub7, sub8, sub9, sub10, sub11,sub12;
	var pageRoot = __GLOBAL__.pageRoot;
	//��һ��side
	sub0 = {
		key: 0,
		active: '0-0',
		items: [
			{
				collapsed: false,
				name: '<% multilang(LANG_STATUS); %>',
				items: [
					{
						name: '<% multilang(LANG_DEVICE); %>',
						href: pageRoot + 'status.asp'
					}
#if defined(CONFIG_IPV6) && !defined(CONFIG_SFU)
					,
					{
						name: '<% multilang(LANG_IPV6); %>',
						href: pageRoot + 'status_ipv6.asp'
					}
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
					,
					{
						name: '<% multilang(LANG_PON); %>',
						href: pageRoot + 'status_pon.asp'
					}
#endif
#if CONFIG_LAN_PORT_NUM > 0
					,
					{
						name: '<% multilang(LANG_LAN_PORT); %>',
						href: pageRoot + 'lan_port_status.asp'
					}
#endif
#ifdef VOIP_SUPPORT
					,
					{
						name: '<% multilang(LANG_VOIP); %>',
						href: pageRoot + 'voip_sip_status_new_web.asp'
					}
#endif
				]
			}
		]
	};
#ifdef CONFIG_00R0
	sub1={
		key:1,
		active:'0-0',
		items:[
			{
				collapsed: false,
				name: '<% multilang(LANG_WIZARD); %>',
				items: [
					{
						name: '<% multilang(LANG_SETUP_WIZARD); %>',
						href: pageRoot + 'wizard_screen_menu.asp'
					}
				]
			}
		]
	};
#endif
	sub2={
		key:2,
		active:'0-0',
		items:[
			{
				collapsed: false,
				name: '<% multilang(LANG_LAN); %>',
				items: [
					{
						name: '<% multilang(LANG_LAN_INTERFACE_SETTINGS); %>',
						href: pageRoot + 'tcpiplan.asp'
					}
#ifdef CONFIG_USER_LAN_VLAN_TRANSLATE
					,
					{
						name: 'LAN VLAN Setup',
						href: pageRoot + 'vlan_translate.asp'
					}
#endif
#ifdef CONFIG_USER_WIRED_8021X
					,
					{
						name: '<% multilang(LANG_SECURITY); %>',
						href: pageRoot + 'config_802_1x.asp'

 					}
#endif
				]
			}
		]
	};
#ifdef WLAN_SUPPORT
	sub3 = {
		key: 3,
		active: '0-0',
		items: [
#if defined(TRIBAND_SUPPORT)
			{
				collapsed: false,
				name: '<% multilang(LANG_WLAN0_5GHZ); %>',
				items: [
					{
						name: '<% multilang(LANG_BASIC_SETTINGS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlbasic.asp&wlan_idx=0'
					},
					{
						name: '<% multilang(LANG_ADVANCED_SETTINGS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wladvanced.asp&wlan_idx=0'
					},
					{
						name: '<% multilang(LANG_SECURITY); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwpa.asp&wlan_idx=0'
					}
#ifdef WLAN_11R
					,
					{
						name: '<% multilang(LANG_FAST_ROAMING); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlft.asp&wlan_idx=0'
					}
#endif
#ifdef WLAN_ACL
					,
					{
						name: '<% multilang(LANG_ACCESS_CONTROL); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlactrl.asp&wlan_idx=0'
					}
#endif
#ifdef WLAN_WDS
					,
					{
						name: '<% multilang(LANG_WDS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwds.asp&wlan_idx=0'
					}
#endif
#ifdef WLAN_MESH
					,
					{
						name: '<% multilang(LANG_WLAN_MESH); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlmesh.asp&wlan_idx=0'
					}
#endif
#if defined(WLAN_CLIENT) || defined(WLAN_SITESURVEY)
#ifdef CONFIG_00R0
					,
					{
						name: '<% multilang(LANG_WLAN_RADAR); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsurvey.asp&wlan_idx=0'
					}
#else
					,
					{
						name: '<% multilang(LANG_SITE_SURVEY); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsurvey.asp&wlan_idx=0'
					}
#endif
#endif
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)	// WPS
					,
					{
						name: '<% multilang(LANG_WPS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwps.asp&wlan_idx=0'
					}
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT
					,
					{
						name: '<% multilang(LANG_CERTIFICATION_INSTALLATION); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwapiinstallcert.asp&wlan_idx=0'
					}
#endif
#ifdef SUPPORT_FON_GRE
                    ,
                    {
                        name: '<% multilang(LANG_GRE_SETTINGS); %>',
                        href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlgre.asp&wlan_idx=0'
                    }
#endif
					,
					{
						name: '<% multilang(LANG_STATUS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlstatus.asp&wlan_idx=0'
					}
#ifdef WLAN_RTIC_SUPPORT
					,
					{
						name: '<% multilang(LANG_RTIC); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlrtic.asp&wlan_idx=0'
					}
#endif
#ifdef WLAN_SCHEDULE_SUPPORT
						,
						{
							name: '<% multilang(LANG_SCHEDULE); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsch.asp&wlan_idx=0'
						}
#endif
				]
			},
#if !defined(CONFIG_USB_AS_WLAN1)
			{
				collapsed: true,
				name: '<% multilang(LANG_WLAN1_5GHZ); %>',
				items: [
					{
						name: '<% multilang(LANG_BASIC_SETTINGS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlbasic.asp&wlan_idx=1'
					},
					{
						name: '<% multilang(LANG_ADVANCED_SETTINGS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wladvanced.asp&wlan_idx=1'
					},
					{
						name: '<% multilang(LANG_SECURITY); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwpa.asp&wlan_idx=1'
					}
#ifdef WLAN_11R
					,
					{
						name: '<% multilang(LANG_FAST_ROAMING); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlft.asp&wlan_idx=1'
					}
#endif
#ifdef WLAN_ACL
					,
					{
						name: '<% multilang(LANG_ACCESS_CONTROL); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlactrl.asp&wlan_idx=1'
					}
#endif
#ifdef WLAN_WDS
					,
					{
						name: '<% multilang(LANG_WDS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwds.asp&wlan_idx=1'
					}
#endif
#ifdef WLAN_MESH
					,
					{
						name: '<% multilang(LANG_WLAN_MESH); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlmesh.asp&wlan_idx=1'
					}
#endif
#if defined(WLAN_CLIENT) || defined(WLAN_SITESURVEY)
#ifdef CONFIG_00R0
					,
					{
						name: '<% multilang(LANG_WLAN_RADAR); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsurvey.asp&wlan_idx=1'
					}
#else
					,
					{
						name: '<% multilang(LANG_SITE_SURVEY); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsurvey.asp&wlan_idx=1'
					}
#endif
#endif
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)	// WPS
					,
					{
						name: '<% multilang(LANG_WPS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwps.asp&wlan_idx=1'
					}
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT
					,
					{
						name: '<% multilang(LANG_CERTIFICATION_INSTALLATION); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwapiinstallcert.asp&wlan_idx=1'
					}
#endif
#ifdef SUPPORT_FON_GRE
                    ,
                    {
                        name: '<% multilang(LANG_GRE_SETTINGS); %>',
                        href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlgre.asp&wlan_idx=1'
                    }
#endif
					,
					{
						name: '<% multilang(LANG_STATUS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlstatus.asp&wlan_idx=1'
					}
#ifdef WLAN_RTIC_SUPPORT
					,
					{
						name: '<% multilang(LANG_RTIC); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlrtic.asp&wlan_idx=1'
					}
#endif
#ifdef WLAN_SCHEDULE_SUPPORT
						,
						{
							name: '<% multilang(LANG_SCHEDULE); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsch.asp&wlan_idx=1'
						}
#endif
				]
			},
#endif /* !defined(CONFIG_USB_AS_WLAN1) */
			{
					collapsed: true,
					name: '<% multilang(LANG_WLAN2_2_4GHZ); %>',
					items: [
						{
							name: '<% multilang(LANG_BASIC_SETTINGS); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlbasic.asp&wlan_idx=2'
						},
						{
							name: '<% multilang(LANG_ADVANCED_SETTINGS); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wladvanced.asp&wlan_idx=2'
						},
						{
							name: '<% multilang(LANG_SECURITY); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwpa.asp&wlan_idx=2'
						}
#ifdef WLAN_11R
						,
						{
							name: '<% multilang(LANG_FAST_ROAMING); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlft.asp&wlan_idx=2'
						}
#endif
#ifdef WLAN_ACL
						,
						{
							name: '<% multilang(LANG_ACCESS_CONTROL); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlactrl.asp&wlan_idx=2'
						}
#endif
#ifdef WLAN_WDS
						,
						{
							name: '<% multilang(LANG_WDS); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwds.asp&wlan_idx=2'
						}
#endif
#ifdef WLAN_MESH
						,
						{
							name: '<% multilang(LANG_WLAN_MESH); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlmesh.asp&wlan_idx=2'
						}
#endif
#if defined(WLAN_CLIENT) || defined(WLAN_SITESURVEY)
#ifdef CONFIG_00R0
						,
						{
							name: '<% multilang(LANG_WLAN_RADAR); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsurvey.asp&wlan_idx=2'
						}
#else
						,
						{
							name: '<% multilang(LANG_SITE_SURVEY); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsurvey.asp&wlan_idx=2'
						}
#endif
#endif
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)	// WPS
						,
						{
							name: '<% multilang(LANG_WPS); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwps.asp&wlan_idx=2'
						}
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT
						,
						{
							name: '<% multilang(LANG_CERTIFICATION_INSTALLATION); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwapiinstallcert.asp&wlan_idx=2'
						}
#endif
#ifdef SUPPORT_FON_GRE
						,
						{
							name: '<% multilang(LANG_GRE_SETTINGS); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlgre.asp&wlan_idx=2'
						}
#endif
						,
						{
							name: '<% multilang(LANG_STATUS); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlstatus.asp&wlan_idx=2'
						}
#ifdef WLAN_RTIC_SUPPORT
						,
						{
							name: '<% multilang(LANG_RTIC); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlrtic.asp&wlan_idx=2'
						}
#endif
#ifdef WLAN_SCHEDULE_SUPPORT
							,
							{
								name: '<% multilang(LANG_SCHEDULE); %>',
								href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsch.asp&wlan_idx=2'
							}
#endif
					]
				}

#elif defined (WLAN_DUALBAND_CONCURRENT)
#if defined(WLAN1_5G_SUPPORT)
			{
				collapsed: false,
				name: '<% multilang(LANG_WLAN0_2_4GHZ); %>',
				items: [
					{
						name: '<% multilang(LANG_BASIC_SETTINGS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlbasic.asp&wlan_idx=0'
					},
					{
						name: '<% multilang(LANG_ADVANCED_SETTINGS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wladvanced.asp&wlan_idx=0'
					},
					{
						name: '<% multilang(LANG_SECURITY); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwpa.asp&wlan_idx=0'
					}
#ifdef WLAN_11R
					,
					{
						name: '<% multilang(LANG_FAST_ROAMING); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlft.asp&wlan_idx=0'
					}
#endif
#ifdef WLAN_ACL
					,
					{
						name: '<% multilang(LANG_ACCESS_CONTROL); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlactrl.asp&wlan_idx=0'
					}
#endif
#ifdef WLAN_WDS
					,
					{
						name: '<% multilang(LANG_WDS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwds.asp&wlan_idx=0'
					}
#endif
#ifdef WLAN_MESH
					,
					{
						name: '<% multilang(LANG_WLAN_MESH); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlmesh.asp&wlan_idx=0'
					}
#endif
#if defined(WLAN_CLIENT) || defined(WLAN_SITESURVEY)
#ifdef CONFIG_00R0
					,
					{
						name: '<% multilang(LANG_WLAN_RADAR); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsurvey.asp&wlan_idx=0'
					}
#else
					,
					{
						name: '<% multilang(LANG_SITE_SURVEY); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsurvey.asp&wlan_idx=0'
					}
#endif
#endif
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)	// WPS
					,
					{
						name: '<% multilang(LANG_WPS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwps.asp&wlan_idx=0'
					}
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT
					,
					{
						name: '<% multilang(LANG_CERTIFICATION_INSTALLATION); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwapiinstallcert.asp&wlan_idx=0'
					}
#endif
#ifdef SUPPORT_FON_GRE
                    ,
                    {
                        name: '<% multilang(LANG_GRE_SETTINGS); %>',
                        href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlgre.asp&wlan_idx=0'
                    }
#endif
					,
					{
						name: '<% multilang(LANG_STATUS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlstatus.asp&wlan_idx=0'
					}
#ifdef WLAN_RTIC_SUPPORT
					,
					{
						name: '<% multilang(LANG_RTIC); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlrtic.asp&wlan_idx=0'
					}
#endif
#ifdef WLAN_SCHEDULE_SUPPORT
						,
						{
							name: '<% multilang(LANG_SCHEDULE); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsch.asp&wlan_idx=0'
						}
#endif
				]
			},
			{
				collapsed: true,
#ifdef WLAN_6G_SUPPORT
				name: '<% multilang(LANG_WLAN1_5GHZ); %><% multilang(LANG_6GHZ); %>',
#else
				name: '<% multilang(LANG_WLAN1_5GHZ); %>',
#endif
				items: [
					{
						name: '<% multilang(LANG_BASIC_SETTINGS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlbasic.asp&wlan_idx=1'
					},
					{
						name: '<% multilang(LANG_ADVANCED_SETTINGS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wladvanced.asp&wlan_idx=1'
					},
					{
						name: '<% multilang(LANG_SECURITY); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwpa.asp&wlan_idx=1'
					}
#ifdef WLAN_11R
					,
					{
						name: '<% multilang(LANG_FAST_ROAMING); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlft.asp&wlan_idx=1'
					}
#endif
#ifdef WLAN_ACL
					,
					{
						name: '<% multilang(LANG_ACCESS_CONTROL); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlactrl.asp&wlan_idx=1'
					}
#endif
#ifdef WLAN_WDS
					,
					{
						name: '<% multilang(LANG_WDS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwds.asp&wlan_idx=1'
					}
#endif
#ifdef WLAN_MESH
					,
					{
						name: '<% multilang(LANG_WLAN_MESH); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlmesh.asp&wlan_idx=1'
					}
#endif
#if defined(WLAN_CLIENT) || defined(WLAN_SITESURVEY)
#ifdef CONFIG_00R0
					,
					{
						name: '<% multilang(LANG_WLAN_RADAR); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsurvey.asp&wlan_idx=1'
					}
#else
					,
					{
						name: '<% multilang(LANG_SITE_SURVEY); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsurvey.asp&wlan_idx=1'
					}
#endif
#endif
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)	// WPS
					,
					{
						name: '<% multilang(LANG_WPS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwps.asp&wlan_idx=1'
					}
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT
					,
					{
						name: '<% multilang(LANG_CERTIFICATION_INSTALLATION); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwapiinstallcert.asp&wlan_idx=1'
					}
#endif
#ifdef SUPPORT_FON_GRE
                    ,
                    {
                        name: '<% multilang(LANG_GRE_SETTINGS); %>',
                        href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlgre.asp&wlan_idx=1'
                    }
#endif
					,
					{
						name: '<% multilang(LANG_STATUS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlstatus.asp&wlan_idx=1'
					}
#ifdef WLAN_RTIC_SUPPORT
					,
					{
						name: '<% multilang(LANG_RTIC); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlrtic.asp&wlan_idx=1'
					}
#endif
#ifdef WLAN_SCHEDULE_SUPPORT
						,
						{
							name: '<% multilang(LANG_SCHEDULE); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsch.asp&wlan_idx=1'
						}
#endif

				]
			}
#else
			{
				collapsed: false,
#ifdef WLAN_6G_SUPPORT
				name: '<% multilang(LANG_WLAN0_5GHZ); %><% multilang(LANG_6GHZ); %>',
#else
				name: '<% multilang(LANG_WLAN0_5GHZ); %>',
#endif
				items: [
					{
						name: '<% multilang(LANG_BASIC_SETTINGS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlbasic.asp&wlan_idx=0'
					},
					{
						name: '<% multilang(LANG_ADVANCED_SETTINGS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wladvanced.asp&wlan_idx=0'
					},
					{
						name: '<% multilang(LANG_SECURITY); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwpa.asp&wlan_idx=0'
					}
#ifdef WLAN_11R
					,
					{
						name: '<% multilang(LANG_FAST_ROAMING); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlft.asp&wlan_idx=0'
					}
#endif
#ifdef WLAN_ACL
					,
					{
						name: '<% multilang(LANG_ACCESS_CONTROL); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlactrl.asp&wlan_idx=0'
					}
#endif
#ifdef WLAN_WDS
					,
					{
						name: '<% multilang(LANG_WDS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwds.asp&wlan_idx=0'
					}
#endif
#ifdef WLAN_MESH
					,
					{
						name: '<% multilang(LANG_WLAN_MESH); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlmesh.asp&wlan_idx=0'
					}
#endif
#if defined(WLAN_CLIENT) || defined(WLAN_SITESURVEY)
#ifdef CONFIG_00R0
					,
					{
						name: '<% multilang(LANG_WLAN_RADAR); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsurvey.asp&wlan_idx=0'
					}
#else
					,
					{
						name: '<% multilang(LANG_SITE_SURVEY); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsurvey.asp&wlan_idx=0'
					}
#endif
#endif
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)	// WPS
					,
					{
						name: '<% multilang(LANG_WPS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwps.asp&wlan_idx=0'
					}
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT
					,
					{
						name: '<% multilang(LANG_CERTIFICATION_INSTALLATION); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwapiinstallcert.asp&wlan_idx=0'
					}
#endif
#ifdef SUPPORT_FON_GRE
                    ,
                    {
                        name: '<% multilang(LANG_GRE_SETTINGS); %>',
                        href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlgre.asp&wlan_idx=0'
                    }
#endif
					,
					{
						name: '<% multilang(LANG_STATUS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlstatus.asp&wlan_idx=0'
					}
#ifdef WLAN_RTIC_SUPPORT
					,
					{
						name: '<% multilang(LANG_RTIC); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlrtic.asp&wlan_idx=0'
					}
#endif
#ifdef WLAN_SCHEDULE_SUPPORT
						,
						{
							name: '<% multilang(LANG_SCHEDULE); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsch.asp&wlan_idx=0'
						}
#endif
				]
			},
			{
				collapsed: true,
				name: '<% multilang(LANG_WLAN1_2_4GHZ); %>',
				items: [
					{
						name: '<% multilang(LANG_BASIC_SETTINGS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlbasic.asp&wlan_idx=1'
					},
					{
						name: '<% multilang(LANG_ADVANCED_SETTINGS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wladvanced.asp&wlan_idx=1'
					},
					{
						name: '<% multilang(LANG_SECURITY); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwpa.asp&wlan_idx=1'
					}
#ifdef WLAN_11R
					,
					{
						name: '<% multilang(LANG_FAST_ROAMING); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlft.asp&wlan_idx=1'
					}
#endif
#ifdef WLAN_ACL
					,
					{
						name: '<% multilang(LANG_ACCESS_CONTROL); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlactrl.asp&wlan_idx=1'
					}
#endif
#ifdef WLAN_WDS
					,
					{
						name: '<% multilang(LANG_WDS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwds.asp&wlan_idx=1'
					}
#endif
#ifdef WLAN_MESH
					,
					{
						name: '<% multilang(LANG_WLAN_MESH); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlmesh.asp&wlan_idx=1'
					}
#endif
#if defined(WLAN_CLIENT) || defined(WLAN_SITESURVEY)
#ifdef CONFIG_00R0
					,
					{
						name: '<% multilang(LANG_WLAN_RADAR); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsurvey.asp&wlan_idx=1'
					}
#else
					,
					{
						name: '<% multilang(LANG_SITE_SURVEY); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsurvey.asp&wlan_idx=1'
					}
#endif
#endif
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)	// WPS
					,
					{
						name: '<% multilang(LANG_WPS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwps.asp&wlan_idx=1'
					}
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT
					,
					{
						name: '<% multilang(LANG_CERTIFICATION_INSTALLATION); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlwapiinstallcert.asp&wlan_idx=1'
					}
#endif
#ifdef SUPPORT_FON_GRE
                    ,
                    {
                        name: '<% multilang(LANG_GRE_SETTINGS); %>',
                        href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlgre.asp&wlan_idx=1'
                    }
#endif
					,
					{
						name: '<% multilang(LANG_STATUS); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlstatus.asp&wlan_idx=1'
					}
#ifdef WLAN_RTIC_SUPPORT
					,
					{
						name: '<% multilang(LANG_RTIC); %>',
						href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlrtic.asp&wlan_idx=1'
					}
#endif
#ifdef WLAN_SCHEDULE_SUPPORT
						,
						{
							name: '<% multilang(LANG_SCHEDULE); %>',
							href: pageRoot + 'boaform/formWlanRedirect?redirect-url=/wlsch.asp&wlan_idx=1'
						}
#endif
				]
			}
#endif
#else
			{
				collapsed: false,
				name: '<% multilang(WLAN); %>',
				items: [
					{
						name: '<% multilang(LANG_BASIC_SETTINGS); %>',
						href: pageRoot + 'wlbasic.asp'
					}

#ifdef CONFIG_USER_FON
					,
					{
						name: '<% multilang(LANG_FON_SPOT_SETTINGS); %>',
						href: pageRoot + 'wlfon.asp'
					}
#endif
					,
					{
						name: '<% multilang(LANG_ADVANCED_SETTINGS); %>',
						href: pageRoot + 'wladvanced.asp'
					},
					{
						name: '<% multilang(LANG_SECURITY); %>',
						href: pageRoot + 'wlwpa.asp'
					}
#ifdef WLAN_11R
					,
					{
						name: '<% multilang(LANG_FAST_ROAMING); %>',
						href: pageRoot + 'wlft.asp'
					}
#endif
#ifdef WLAN_ACL
					,
					{
						name: '<% multilang(LANG_ACCESS_CONTROL); %>',
						href: pageRoot + 'wlactrl.asp'
					}
#endif
#ifdef WLAN_WDS
					,
					{
						name: '<% multilang(LANG_WDS); %>',
						href: pageRoot + 'wlwds.asp'
					}
#endif
#ifdef WLAN_MESH
					,
					{
						name: '<% multilang(LANG_WLAN_MESH); %>',
						href: pageRoot + 'wlmesh.asp'
					}
#endif
#if defined(WLAN_CLIENT) || defined(WLAN_SITESURVEY)
#ifdef CONFIG_00R0
					,
					{
						name: '<% multilang(LANG_WLAN_RADAR); %>',
						href: pageRoot + 'wlsurvey.asp'
					}
#else
					,
					{
						name: '<% multilang(LANG_SITE_SURVEY); %>',
						href: pageRoot + 'wlsurvey.asp'
					}
#endif
#endif
#if defined(CONFIG_WIFI_SIMPLE_CONFIG) || defined(WLAN_WPS)	// WPS
					,
					{
						name: '<% multilang(LANG_WPS); %>',
						href: pageRoot + 'wlwps.asp'
					}
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT
					,
					{
						name: '<% multilang(LANG_CERTIFICATION_INSTALLATION); %>',
						href: pageRoot + 'wlwapiinstallcert.asp'
					}
#endif
#ifdef SUPPORT_FON_GRE
                    ,
                    {
                        name: '<% multilang(LANG_GRE_SETTINGS); %>',
                        href: pageRoot + 'wlgre.asp'
                    }
#endif
					,
					{
						name: '<% multilang(LANG_STATUS); %>',
						href: pageRoot + 'wlstatus.asp'
					}
#ifdef WLAN_RTIC_SUPPORT
					,
					{
						name: '<% multilang(LANG_RTIC); %>',
						href: pageRoot + 'wlrtic.asp'
					}
#endif
#ifdef WLAN_SCHEDULE_SUPPORT
						,
						{
							name: '<% multilang(LANG_SCHEDULE); %>',
							href: pageRoot + 'wlsch.asp'
						}
#endif
				]
			}
#endif
#ifdef RTK_MULTI_AP
			,
			{
				collapsed: true,
				name: '<% multilang(LANG_WLAN_EASY_MESH); %>',
				items: [
					{
						name: '<% multilang(LANG_WLAN_EASY_MESH_INTERFACE_SETUP); %>',
						href: pageRoot + 'multi_ap_setting_general.asp'
					}
					<% CheckMenuDisplay("map_topology"); %>
#ifdef RTK_MULTI_AP_R2
					<% CheckMenuDisplay("map_channel_scan"); %>
					<% CheckMenuDisplay("map_vlan"); %>
#endif
				]
			}
#endif
		]
	};
#endif

#ifndef CONFIG_SFU
	sub4 = {
		key: 4,
		active: '0-0',
		items: [
			{
				collapsed: false,
				name: '<% multilang(LANG_WAN); %>',
				items: [
					<% CheckMenuDisplay("wan_mode"); %>
#ifdef CONFIG_ETHWAN
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
#ifndef USE_DEONET
					{
						name: '<% multilang(LANG_PON_WAN); %>',
						href: pageRoot + 'boaform/formWanRedirect?redirect-url=/multi_wan_generic.asp&if=pon'
					}
					,
#endif
					{
						name: '<% multilang(LANG_PON_WAN); %>',
						href: pageRoot + 'deo_multi_wan_generic.asp'
					}
#else
#ifdef CONFIG_USER_WAN_PORT_AUTO_SELECTION
					{
						name: '<% multilang(LANG_WAN_PORT_SET); %>',
						href: pageRoot + 'boaform/formWanRedirect?redirect-url=/wan_port_set.asp'
					}
					,
#endif
#ifdef CONFIG_RTL_MULTI_ETH_WAN
					{
						name: '<% multilang(LANG_ETHERNET_WAN); %>',
						href: pageRoot + 'boaform/formWanRedirect?redirect-url=/multi_wan_generic.asp&if=eth'
					}
					,

					{
						name: '<% multilang(LANG_ETHERNET_WAN); %>',
						href: pageRoot + 'deo_multi_wan_generic.asp'
					}
#else
					{
						name: '<% multilang(LANG_ETHERNET_WAN); %>',
						href: pageRoot + 'waneth.asp'
					}
#endif
#endif
#endif
#ifdef CONFIG_PTMWAN
#ifdef CONFIG_ETHWAN
					,
#endif
					{
						name: '<% multilang(LANG_PTM_WAN); %>',
						href: pageRoot + 'boaform/formWanRedirect?redirect-url=/multi_wan_generic.asp&if=ptm'
					}
					,
					{
						name: '<% multilang(LANG_PTM_WAN); %>',
						href: pageRoot + 'deo_multi_wan_generic.asp'
					}
#endif
#ifdef WLAN_WISP
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN)
					,
#endif
					{
						name: '<% multilang(LANG_WIRELESS_WAN); %>',
						href: pageRoot + 'wanwl.asp'
					}
#endif
#ifdef CONFIG_DEV_xDSL
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN) || defined(WLAN_WISP)
					,
#endif
					{
						name: '<% multilang(LANG_ATM_WAN); %>',
						href: pageRoot + 'wanadsl.asp'
					}
					,
					{
						name: '<% multilang(LANG_ATM_SETTINGS); %>',
						href: pageRoot + 'wanatm.asp'
					}
					,
					{
						name: '<% multilang(LANG_ADSL_SETTINGS); %>',
						href: pageRoot + '/admin/adsl-set.asp'
					}
#ifdef CONFIG_DSL_VTUO
					,
					{
						name: '<% multilang(LANG_VTUO_SETTINGS); %>',
						href: pageRoot + '/admin/vtuo-set.asp'
					}
#endif /*CONFIG_DSL_VTUO*/
#endif
#ifdef CONFIG_USER_PPPOMODEM
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN) || defined(WLAN_WISP) || defined(CONFIG_DEV_xDSL)
					,
#endif
					{
						name: '<% multilang(LANG_3G_SETTINGS); %>',
						href: pageRoot + 'wan3gconf.asp'
					}
#endif //CONFIG_USER_PPPOMODEM
#ifdef CONFIG_NET_IPGRE
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN) || defined(WLAN_WISP) || defined(CONFIG_DEV_xDSL) || defined(CONFIG_NET_IPGRE)
					,
#endif
					{
						name:'<% multilang(LANG_GRE_SETTINGS); %>',
						href: pageRoot + 'gre.asp'
					}
#endif
				]
			}
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_NET_IPIP)  || defined(CONFIG_XFRM) || defined(CONFIG_USER_XL2TPD) || defined(CONFIG_USER_PPTP_CLIENT)
			,
			{
				collapsed: true,
				name: '<% multilang(LANG_VPN); %>',
				items: [
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
#ifndef CONFIG_IPV6_VPN
#ifdef CONFIG_USER_PPTPD_PPTPD
					{
						name: '<% multilang(LANG_PPTP); %>',
						href: pageRoot + 'pptpd.asp'
					}
#else
					{
						name: '<% multilang(LANG_PPTP); %>',
						href: pageRoot + 'pptp.asp'
					}
#endif
#else
					{
						name: '<% multilang(LANG_PPTP); %>',
						href: pageRoot + 'pptpv6.asp'
					}
#endif
#endif //end of CONFIG_USER_PPTP_CLIENT_PPTP
#if defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD)
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_PPTP_CLIENT)
					,
#endif
					{
						name: '<% multilang(LANG_L2TP); %>',
						href: pageRoot + 'l2tpv6.asp'
					}
#endif //end of CONFIG_USER_L2TPD_L2TPD
#ifdef CONFIG_NET_IPIP
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_USER_XL2TPD) || defined(CONFIG_USER_PPTP_CLIENT)
					,
#endif
					{
						name: '<% multilang(LANG_IPIP); %>',
						href: pageRoot + 'ipip.asp'
					}
#endif //end of CONFIG_NET_IPIP
#ifdef CONFIG_XFRM
#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD) || defined(CONFIG_NET_IPIP) || defined(CONFIG_USER_XL2TPD) || defined(CONFIG_USER_PPTP_CLIENT)
					,
#endif
#ifdef CONFIG_USER_STRONGSWAN
					{
						name: '<% multilang(LANG_IPSEC); %>',
						href: pageRoot + 'ipsec_swan.asp'
					}
#else
					{
						name: '<% multilang(LANG_IPSEC); %>',
#ifndef CONFIG_IPV6_VPN
						href: pageRoot + 'ipsec.asp'
#else
						href: pageRoot + 'ipsecv6.asp'
#endif
					}
#endif
#endif
				]
			}
#endif
		]
	};
	sub5 = {
		key: 5,
		active: '0-0',
		items: [
			{
				collapsed: false,
				name: '<% multilang(LANG_SERVICE); %>',
				items: [
#ifdef CONFIG_USER_DHCP_SERVER
#ifdef IMAGENIO_IPTV_SUPPORT
					{
						name: '<% multilang(LANG_DHCP); %>',
						href: pageRoot + 'dhcpd_sc.asp'
					}
#else
					{
						name: '<% multilang(LANG_DHCP); %>',
						href: pageRoot + 'dhcpd.asp'
					}
#endif
					,
#endif
#ifdef CONFIG_USER_VLAN_ON_LAN
					{
						name: '<% multilang(LANG_VLAN_ON_LAN); %>',
						href: pageRoot + 'vlan_on_lan.asp'
					}
					,
#endif
#ifdef CONFIG_USER_DDNS
					{
						name: '<% multilang(LANG_DYNAMIC_DNS); %>',
						href: pageRoot + 'ddns.asp'
					}
					,
#endif
/* not use for deonet
#if defined(CONFIG_USER_IGMPPROXY)
					{
						name: '<% multilang(LANG_IGMP_PROXY); %>',
						href: pageRoot + 'igmproxy.asp'
					}
					,
#endif
#if defined(CONFIG_USER_UPNPD)||defined(CONFIG_USER_MINIUPNPD)
					{
						name: '<% multilang(LANG_UPNP); %>',
						href: pageRoot + 'upnp.asp'
					}
					,
#endif
#ifdef CONFIG_USER_ROUTED_ROUTED
					{
						name: '<% multilang(LANG_RIP); %>',
						href: pageRoot + 'rip.asp'
					}
					,
#endif
*/
					{
						name: '<% multilang(LANG_INTERNET_ACCESS_LIMIT); %>',
						href: pageRoot + 'access_limit.asp'
					}
					,
					{
						name: '<% multilang(LANG_CLIENT_INFOMATION); %>',
						href: pageRoot + 'client_info.asp'
					}
#ifdef WEB_REDIRECT_BY_MAC
					,
					{
						name: '<% multilang(LANG_LANDING_PAGE); %>',
						href: pageRoot + 'landing.asp'
					}
					,
#endif
#if defined(CONFIG_USER_MINIDLNA)
					{
						name: '<% multilang(LANG_DMS); %>',
						href: pageRoot + 'dms.asp'
					}
					,
#endif
#ifdef CONFIG_USER_SAMBA
					{
						name: '<% multilang(LANG_SAMBA); %>',
						href: pageRoot + 'samba.asp'
					}
#endif
#ifdef CONFIG_ELINK_SUPPORT
					,
					{
							name: '<% multilang(LANG_ELINK); %>',
							href: pageRoot + 'elink.asp'
					}
#endif

				]
			},
			{
				collapsed: true,
				name: '<% multilang(LANG_FIREWALL); %>',
				items: [
#ifdef CONFIG_USER_RTK_NAT_ALG_PASS_THROUGH
					{
						name: '<% multilang(LANG_ALG); %>',
						href: pageRoot + 'algonoff.asp'
					}
					,
#endif
//#ifdef IP_PORT_FILTER
#ifdef CONFIG_RTK_L34_ENABLE
					{
						name: '<% multilang(LANG_IP_PORT_FILTERING); %>',
						href: pageRoot + 'fw-ipportfilter_rg.asp'
					}
#else
					{
						name: '<% multilang(LANG_IP_PORT_FILTERING); %>',
						href: pageRoot + 'fw-ipportfilter.asp'
					}
#endif
//#endif
#ifdef MAC_FILTER
#ifdef CONFIG_RTK_DEV_AP
					,
                                        {
                                                name: '<% multilang(LANG_MAC_FILTERING); %>',
                                                href: pageRoot + 'fw-macfilter_gw.asp'
                                        }
#else
#ifdef CONFIG_RTK_L34_ENABLE
					,
					{
						name: '<% multilang(LANG_MAC_FILTERING); %>',
						href: pageRoot + 'fw-macfilter_rg.asp'
					}
#else
					,
					{
						name: '<% multilang(LANG_MAC_FILTERING); %>',
						href: pageRoot + 'fw-macfilter.asp'
					}
#endif
#endif
#endif
#ifdef PORT_FORWARD_GENERAL
					,
					{
						name: '<% multilang(LANG_PORT_FORWARDING); %>',
						href: pageRoot + 'fw-portfw.asp'
					}
#endif
					,
					{
						name: '<% multilang(LANG_STATIC_MAPPING); %>',
						href: pageRoot + 'static-mapping.asp'
					}
#ifdef URL_BLOCKING_SUPPORT
					,
					{
						name: '<% multilang(LANG_URL_BLOCKING); %>',
						href: pageRoot + 'url_blocking.asp'
					}
#endif
#ifdef DOMAIN_BLOCKING_SUPPORT
					,
					{
						name: '<% multilang(LANG_DOMAIN_BLOCKING); %>',
						href: pageRoot + 'domainblk.asp'
					}
#endif
#ifdef PARENTAL_CTRL
					,
					{
						name: '<% multilang(LANG_PARENTAL_CONTROL); %>',
						href: pageRoot + 'parental-ctrl.asp'
					}
#endif
#ifdef TCP_UDP_CONN_LIMIT
					,
					{
						name: '<% multilang(LANG_CONNECTION_LIMIT); %>',
						href: pageRoot + 'connlimit.asp'
					}
#endif // TCP_UDP_CONN_LIMIT
#ifdef NATIP_FORWARDING
					,
					{
						name: '<% multilang(LANG_NAT_IP_FORWARDING); %>',
						href: pageRoot + 'fw-ipfw.asp'
					}
#endif
#ifdef PORT_TRIGGERING_STATIC
					,
					{
						name: '<% multilang(LANG_PORT_TRIGGERING); %>',
						href: pageRoot + 'gaming.asp'
					}
#endif
#ifdef PORT_TRIGGERING_DYNAMIC
					,
					{
						name: '<% multilang(LANG_PORT_TRIGGERING); %>',
						href: pageRoot + 'fw-porttrigger.asp'
					}
#endif
#ifdef DMZ
					,
					{
						name: '<% multilang(LANG_DMZ); %>',
						href: pageRoot + 'fw-dmz.asp'
					}
#endif
//#if defined(CONFIG_USER_BOA_PRO_PASSTHROUGH) && defined(CONFIG_RTK_DEV_AP)
#ifdef CONFIG_USER_BOA_PRO_PASSTHROUGH
					,
					{
						name: 'VPN PassThr',
						href: pageRoot + 'pass_through.asp'
					}
#endif
#ifdef ADDRESS_MAPPING
#ifdef MULTI_ADDRESS_MAPPING
					,
					{
						name: '<% multilang(LANG_NAT_RULE_CONFIGURATION); %>',
						href: pageRoot + 'multi_addr_mapping.asp'
					}
#else //!MULTI_ADDRESS_MAPPING
					,
					{
						name: '<% multilang(LANG_NAT_RULE_CONFIGURATION); %>',
						href: pageRoot + 'addr_mapping.asp'
					}
#endif// end of !MULTI_ADDRESS_MAPPING
#endif
				]
			}
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
			,
			{
				collapsed: true,
				name: 'Load Balance',
				items:[
				{
					name: '<% multilang(LANG_LOAD_BALANCE); %>',
					href: pageRoot + 'load_balance.asp'
				}
				,
				{
					name: '<% multilang(LANG_LOAD_BALANCE_STATS); %>',
					href: pageRoot + 'load_balance_stats_all.asp'
				}
				]
			}
#endif
		]
	};
#else
#ifdef CONFIG_USER_DHCP_SERVER
	sub5 = {
		key: 5,
		active: '0-0',
		items: [
			{
				collapsed: false,
				name: '<% multilang(LANG_SERVICE); %>',
				items: [
#ifdef CONFIG_USER_DHCP_SERVER
#ifdef IMAGENIO_IPTV_SUPPORT
					{
						name: '<% multilang(LANG_DHCP); %>',
						href: pageRoot + 'dhcpd_sc.asp'
					}
#else
					{
						name: '<% multilang(LANG_DHCP); %>',
						href: pageRoot + 'dhcpd.asp'
					}
#endif
#endif
#ifdef CONFIG_USER_VLAN_ON_LAN
					,
					{
						name: '<% multilang(LANG_VLAN_ON_LAN); %>',
						href: pageRoot + 'vlan_on_lan.asp'
					}
#endif
#if defined(CONFIG_USER_IGMPPROXY)&&!defined(CONFIG_IGMPPROXY_MULTIWAN)
					,
					{
						name: '<% multilang(LANG_IGMP_PROXY); %>',
						href: pageRoot + 'igmproxy.asp'
					}
#endif
#if defined(CONFIG_USER_UPNPD)||defined(CONFIG_USER_MINIUPNPD)
					,
					{
						name: '<% multilang(LANG_UPNP); %>',
						href: pageRoot + 'upnp.asp'
					}
#endif
/* not use for deonet
#ifdef CONFIG_USER_ROUTED_ROUTED
					,
					{
						name: '<% multilang(LANG_RIP); %>',
						href: pageRoot + 'rip.asp'
					}
#endif
*/
#ifdef WEB_REDIRECT_BY_MAC
					,
					{
						name: '<% multilang(LANG_LANDING_PAGE); %>',
						href: pageRoot + 'landing.asp'
					}
#endif
#if defined(CONFIG_USER_MINIDLNA)
					,
					{
						name: '<% multilang(LANG_DMS); %>',
						href: pageRoot + 'dms.asp'
					}
#endif
#ifdef CONFIG_USER_SAMBA
					,
					{
						name: '<% multilang(LANG_SAMBA); %>',
						href: pageRoot + 'samba.asp'
					}
#endif
				]
			}
#ifdef CONFIG_RTL_MULTI_PHY_ETH_WAN
			,
			{
				collapsed: true,
				name: 'Load Balance',
				items:[
				{
					name: '<% multilang(LANG_LOAD_BALANCE); %>',
					href: pageRoot + 'load_balance.asp'
				}
				,
				{
					name: '<% multilang(LANG_LOAD_BALANCE_STATS); %>',
					href: pageRoot + 'load_balance_stats_all.asp'
				}
				]
			}
#endif

		]
	};
#endif
#endif

#ifdef VOIP_SUPPORT
	sub6 = {
		key: 6,
		active: '0-0',
		items: [
			{
				collapsed: false,
				name: '<% multilang(LANG_VOIP); %>',
				items: [
					{
						name: '<% multilang(LANG_PORT1); %>',
						href: pageRoot + 'voip_general_new_web.asp?port=0'
					}
#if CONFIG_RTK_VOIP_CON_CH_NUM > 1
					,
					{
						name: '<% multilang(LANG_PORT2); %>',
						href: pageRoot + 'voip_general_new_web.asp?port=1'
					}
#endif
					,
					{
						name: '<% multilang(LANG_ADVANCE); %>',
						href: pageRoot + 'voip_advanced_new_web.asp'
					}
#ifdef CONFIG_RTK_VOIP_DRIVERS_FXO
					,
					{
						name: '<% multilang(LANG_FXO); %>',
						href: pageRoot + 'voip_general_new_web.asp?port=2'
					}
#endif
					,
					{
						name: '<% multilang(LANG_TONE); %>',
						href: pageRoot + 'voip_tone_new_web.asp'
					}
					,
					{
						name: '<% multilang(LANG_OTHER); %>',
						href: pageRoot + 'voip_other_new_web.asp'
					}
					,
					{
						name: '<% multilang(LANG_NETWORK); %>',
						href: pageRoot + 'voip_network_new_web.asp'
					}
					,
					{
						name: '<% multilang(LANG_VOIP_CALLHISTORY); %>',
						href: pageRoot + 'voip_callhistory_new_web.asp'
					}
					,
					{
						name: '<% multilang(LANG_REGISTER_STATUS); %>',
						href: pageRoot + 'voip_sip_status_new_web.asp'
					}
				]
			}
		]
	};
#endif

	sub7 = {
		key: 7,
		active: '0-0',
		items: [
			{
				collapsed: false,
				name: '<% multilang(LANG_ADVANCE); %>',
				items: [
#ifdef CONFIG_RTL9601B_SERIES
					{
						name: '<% multilang(LANG_VLAN_SETTINGS); %>',
						href: pageRoot + 'vlan.asp'
					}
					,
#endif
					{
						name: '<% multilang(LANG_ARP_TABLE); %>',
						href: pageRoot + 'arptable.asp'
					}
#ifdef CONFIG_USER_RTK_LAN_USERLIST
					,
					{
						name: '<% multilang(LANG_LAN_DEVICE_TABLE); %>',
						href: pageRoot + 'landevice.asp'
					}
#endif
/*
#ifndef CONFIG_SFU
					,
					{
						name: '<% multilang(LANG_BRIDGING); %>',
						href: pageRoot + 'bridging.asp'
					}
#endif
*/
#ifdef CONFIG_USER_RTK_LBD
					,
					{
						name: '<% multilang(LANG_LOOP_DETECTION); %>',
						href: pageRoot + 'lbd.asp'
					}
#endif
#ifdef ROUTING
					//,
					//{
					//	name: '<% multilang(LANG_ROUTING); %>',
					//	href: pageRoot + 'routing.asp'
					//}
#endif
/*
#ifdef CONFIG_USER_SNMPD_SNMPD_V2CTRAP
					,
					{
						name: '<% multilang(LANG_SNMP); %>',
						href: pageRoot + 'snmp.asp'
					}
#endif
#ifdef CONFIG_USER_SNMPD_SNMPD_V3
					,
					{
						name: '<% multilang(LANG_SNMP); %>',
						href: pageRoot + 'snmpv3.asp'
					}
#endif
*/
#ifdef CONFIG_USER_BRIDGE_GROUPING
					,
					{
						name: '<% multilang(LANG_BRIDGE_GROUPING); %>',
						href: pageRoot + 'bridge_grouping.asp'
					}
#endif
#if CONFIG_USER_INTERFACE_GROUPING
					,
					{
						name: '<% multilang(LANG_INTERFACE_GROUPING); %>',
						href: pageRoot + 'interface_grouping.asp'
					}
#endif
#ifdef VLAN_GROUP
					,
					{
						name: '<% multilang(LANG_PORT_MAPPING); %>',
						href: pageRoot + 'eth2pvc_vlan.asp'
					}
#endif
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
#ifdef ELAN_LINK_MODE
					,
					{
						name: '<% multilang(LANG_LINK_MODE); %>',
						href: pageRoot + 'linkmode.asp'
					}
#endif
#else
#ifdef ELAN_LINK_MODE_INTRENAL_PHY
					,
					{
						name: '<% multilang(LANG_LINK_MODE); %>',
						href: pageRoot + 'linkmode_eth.asp'
					}
#endif
#endif
#ifdef REMOTE_ACCESS_CTL
					,
					{
						name: '<% multilang(LANG_REMOTE_ACCESS); %>',
						href: pageRoot + 'rmtacc.asp'
					}
#endif
#ifndef CONFIG_00R0
#ifdef CONFIG_USER_CUPS
					//,
					//{
					//	name: '<% multilang(LANG_PRINT_SERVER); %>',
					//	href: pageRoot + 'printServer.asp'
					//}
#endif //CONFIG_USER_CUPS
#endif
#ifdef IP_PASSTHROUGH
					,
					{
						name: '<% multilang(LANG_OTHERS); %>',
						href: pageRoot + 'others.asp'
					}
#endif
#ifdef _PRMT_X_CT_COM_MWBAND_
					,
					{
						name: '<% multilang(LANG_CLIENT_LIMIT); %>',
						href: pageRoot + 'clientlimit.asp'
					}
#endif
				]
			}
/*
#if defined(CONFIG_USER_IP_QOS)
			,
			{
				collapsed: true,
				name: '<% multilang(LANG_IP_QOS); %>',
				items: [
					{
						name: '<% multilang(LANG_QOS_POLICY); %>',
				#ifdef CONFIG_USER_AP_QOS_PHY_MULTI_WAN
						href: pageRoot + 'net_qos_imq_policy_mt_phy_wan.asp'
				#else
						href: pageRoot + 'net_qos_imq_policy.asp'
				#endif
					},
					{
						name: '<% multilang(LANG_QOS_CLASSIFICATION); %>',
						href: pageRoot + 'net_qos_cls.asp'
					},
				#ifndef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
					{
						name: '<% multilang(LANG_TRAFFIC_SHAPING); %>',
						href: pageRoot + 'net_qos_traffictl.asp'
					}
				#else
					{
						name: '<% multilang(LANG_SPEED_LIMIT); %>',
						href: pageRoot + 'net_qos_data_speed_limit_ct.asp'
					}
				#endif
				]
			}
#endif
*/
#if defined(CONFIG_IPV6) && !defined(CONFIG_SFU)
			,
			{
				collapsed: true,
				name: '<% multilang(LANG_IPV6); %>',
				items: [
					{
						name: '<% multilang(LANG_IPV6); %>',
						href: pageRoot + 'ipv6_enabledisable.asp'
					}
/*
#if defined(CONFIG_IPV6) && defined(CONFIG_USER_RADVD)
					,
					{
						name: '<% multilang(LANG_RADVD); %>',
						href: pageRoot + 'radvdconf.asp'
					}
#endif
#ifdef DHCPV6_ISC_DHCP_4XX
					,
					{
						name: '<% multilang(LANG_DHCPV6); %>',
						href: pageRoot + 'dhcpdv6.asp'
					}
#endif
#ifdef CONFIG_USER_MLDPROXY
					,
					{
						name: '<% multilang(LANG_MLD_PROXY); %>',
						href: pageRoot + 'app_mldProxy.asp'
					}
					,
					{
						name: '<% multilang(LANG_MLD_SNOOPING); %>',
						href: pageRoot + 'app_mld_snooping.asp'
					}
#endif
					,
					{
						name: '<% multilang(LANG_IPV6_ROUTING); %>',
						href: pageRoot + 'routing_ipv6.asp'
					}
*/
#ifdef IP_PORT_FILTER
#ifndef CONFIG_RTK_USER_IPV6_INTERFACEID_FILTER
#ifdef CONFIG_RTK_L34_ENABLE
					,
					{
						name: '<% multilang(LANG_IP_PORT_FILTERING); %>',
						href: pageRoot + 'fw-ipportfilter-v6_rg.asp'
					}
#else
					,
					{
						name: '<% multilang(LANG_IP_PORT_FILTERING); %>',
						href: pageRoot + 'fw-ipportfilter-v6.asp'
					}
#endif
#else
#ifdef CONFIG_RTK_L34_ENABLE
					,
					{
						name: '<% multilang(LANG_IP_PORT_FILTERING); %>',
						href: pageRoot + 'fw-ipportfilter-v6_IfId_rg.asp'
					}
#else
					,
					{
						name: '<% multilang(LANG_IP_PORT_FILTERING); %>',
						href: pageRoot + 'fw-ipportfilter-v6_IfId.asp'
					}
#endif
#endif
#endif
/*
#ifdef IP_ACL
					,
					{
						name: '<% multilang(LANG_IPV6_ACL); %>',
						href: pageRoot + 'aclv6.asp'
					}
#endif
*/
				]
			}
#endif
		]
	};

	sub8 = {
		key: 8,
		active: '0-0',
		items: [
			{
				collapsed: false,
				name: '<% multilang(LANG_DIAGNOSTICS); %>',
				items: [
					{
						name: '<% multilang(LANG_PING); %>',
						href: pageRoot + 'ping.asp'
					}
#ifdef CONFIG_IPV6
					,
					{
                        name: '<% multilang(LANG_PING); %>6',
                        href: pageRoot + 'ping6.asp'
                    }
#endif
                    ,
                    {
                        name: '<% multilang(LANG_TRACERT); %>',
                        href: pageRoot + 'tracert.asp'
                    }
#ifdef CONFIG_IPV6
					,
                    {
                        name: '<% multilang(LANG_TRACERT); %>6',
                        href: pageRoot + 'tracert6.asp'
                    }
#endif
					,
					{
                        name: '<% multilang(LANG_IGMP_GROUP); %>',
                        href: pageRoot + 'grouptable.asp'
                    }
					,
					{
                        name: '<% multilang(LANG_SSH_SERVICE); %>',
                        href: pageRoot + 'ssh.asp'
                    }
					,
					{
                        name: '<% multilang(LANG_NETSTAT); %>',
                        href: pageRoot + 'netstat.asp'
                    }
					,
					{
                        name: '<% multilang(LANG_PORT_MIRROR); %>',
                        href: pageRoot + 'portmirror.asp'
                    }
#ifdef CONFIG_USER_TCPDUMP_WEB
					,
					{
						name: '<% multilang(LANG_PACKET_DUMP); %>',
						href: pageRoot + 'pdump.asp'
					}
#endif
#ifdef CONFIG_DEV_xDSL
					,
					{
						name: '<% multilang(LANG_ATM_LOOPBACK); %>',
						href: pageRoot + 'oamloopback.asp'
					}
					,
					{
						name: '<% multilang(LANG_DSL_TONE); %>',
						href: pageRoot + '/admin/adsl-diag.asp'
					}
#endif
#ifdef CONFIG_USER_XDSL_SLAVE
					,
					{
						name: '<% multilang(LANG_DSL_SLAVE_TONE); %>',
						href: pageRoot + '/admin/adsl-slv-diag.asp'
					}
#endif /*CONFIG_USER_XDSL_SLAVE*/
#ifdef CONFIG_DEV_xDSL
#ifdef DIAGNOSTIC_TEST
					,
					{
						name: '<% multilang(LANG_ADSL_CONNECTION); %>',
						href: pageRoot + 'diag-test.asp'
					}
#endif
#endif
#if defined(CONFIG_USER_Y1731) || defined(CONFIG_USER_8023AH)
					,
					{
						name: '<% multilang(LANG_ETH_OAM); %>',
						href: pageRoot + 'ethoam.asp'
					}
#endif
				]
			}
#ifdef CONFIG_USER_DOT1AG_UTILS
			,{
				collapsed: false,
				name: '<% multilang(LANG_802_1AG); %>',
				items: [
					{
						name: '<% multilang(LANG_CONFIGURATION); %>',
						href: pageRoot + 'dot1ag_conf.asp'
					}
					,
					{
						name: '<% multilang(LANG_ACTION); %>',
						href: pageRoot + 'dot1ag_action.asp'
					}
					,
					{
						name: '<% multilang(LANG_STATUS); %>',
						href: pageRoot + 'dot1ag_status.asp'
					}
				]
			}
#endif
		]
	};

	sub9 = {
		key: 9,
		active: '0-0',
		items: [
			{
				collapsed: false,
				name: '<% multilang(LANG_ADMIN); %>',
				items: [
//					<% CheckMenuDisplay("pon_settings"); %>
#if defined(CONFIG_RTK_L34_ENABLE)
					{
						name: '<% multilang(LANG_MCAST_VLAN); %>',
						href: pageRoot + 'app_iptv.asp'
					}
					,
#endif
//					<% CheckMenuDisplay("omci_info"); %>
#ifdef CONFIG_INIT_SCRIPTS
					{
						name: '<% multilang(LANG_INIT_SCRIPTS); %>',
						href: pageRoot + 'init_script.asp'
					}
					,
#endif
#ifdef FINISH_MAINTENANCE_SUPPORT
					{
						name: '<% multilang(LANG_FINISH_MAINTENANCE); %>',
						href: pageRoot + 'finishmaintenance.asp'
					}
					,
#endif
					{
						name: '<% multilang(LANG_COMMIT_REBOOT); %>',
						href: pageRoot + 'reboot.asp'
					}
					// Added by davian kuo
#ifdef CONFIG_USER_BOA_WITH_MULTILANG
					,
					{
						name: '<% multilang(LANG_MULTI_LINGUAL_SETTINGS); %>',
						href: pageRoot + 'multi_lang.asp'
					}
#endif
#ifdef CONFIG_SAVE_RESTORE
					,
					{
						name: '<% multilang(LANG_BACKUP_RESTORE); %>',
						href: pageRoot + 'saveconf.asp'
					}
#endif
#ifdef ACCOUNT_LOGIN_CONTROL
					,
					{
						name: '<% multilang(LANG_LOGOUT); %>',
						href: pageRoot + '/admin/adminlogout.asp'
					}
#endif
#ifdef CONFIG_USER_RTK_SYSLOG
#ifndef SEND_LOG
					,
					{
						name: '<% multilang(LANG_SYSTEM_LOG); %>',
						href: pageRoot + '/admin/user_syslog.asp'
					}
/*
					,
					{
						name: '<% multilang(LANG_LDAP); %>',
						href: pageRoot + 'ldap.asp'
					}
*/
#else
					,
					{
						name: '<% multilang(LANG_SYSTEM_LOG); %>',
						href: pageRoot + 'syslog_server.asp'
					}
#endif
#endif
/*
					,
					{
						name: '<% multilang(LANG_HOLEPUNCHING); %>',
						href: pageRoot + 'holepunching.asp'
					}
*/
#ifdef DOS_SUPPORT
					,
					{
						name: '<% multilang(LANG_DOS); %>',
						href: pageRoot + 'dos.asp'
					}
#endif
#ifdef ACCOUNT_CONFIG
					,
					{
						name: '<% multilang(LANG_USER_ACCOUNT); %>',
						href: pageRoot + 'userconfig.asp'
					}
#else
					,
					{
						name: '<% multilang(LANG_PASSWORD); %>',
						href: pageRoot + 'password.asp'
					}
#endif
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
					,
					{
						name: '<% multilang(LANG_SAMBA); %> <% multilang(LANG_USER_ACCOUNT); %>',
						href: pageRoot + 'app_samba_account.asp'
					}
#endif
#ifdef WEB_UPGRADE
#ifdef UPGRADE_V1
					,
					{
						name: '<% multilang(LANG_FIRMWARE_UPGRADE); %>',
						href: pageRoot + 'upgrade.asp'
					}
#endif // of UPGRADE_V1
#endif // of WEB_UPGRADE
					,
					{
						name: '<% multilang(LANG_AUTO_REBOOT); %>',
						href: pageRoot + 'autoreboot.asp'
					}
					,
					{
						name: '<% multilang(LANG_NATSESSION_CONFIGURATION); %>',
						href: pageRoot + 'natsession.asp'
					}
					,
					{
						name: '<% multilang(LANG_USEAGE_CONFIGURATION); %>',
						href: pageRoot + 'useage.asp'
					}
					,
					{
						name: '<% multilang(LANG_SELF_CHECK); %>',
						href: pageRoot + 'selfcheck.asp'
					}
/*
#ifdef IP_ACL
					,
					{
						name: '<% multilang(LANG_ACL); %>',
						href: pageRoot + 'acl.asp'
					}
#endif
*/
#ifdef TIME_ZONE
					,
					{
						name: '<% multilang(LANG_TIME_ZONE); %>',
						href: pageRoot + 'tz.asp'
					}
#endif
/*
#ifdef _CWMP_MIB_
					,
					{
						name: '<% multilang(LANG_TR_069); %>',
						href: pageRoot + 'tr069config.asp'
					}
#endif
*/
#ifdef CONFIG_USER_USP_TR369
					,
					{
						name: 'TR-369',
						href: pageRoot + 'tr369.asp'
					}
#endif
					//added by xl_yue
#ifndef CONFIG_00R0
#ifdef USE_LOGINWEB_OF_SERVER
					,
					{
						name: '<% multilang(LANG_LOGOUT); %>',
						href: pageRoot + '/admin/logout.asp'
					}
#endif
#endif
				]
			}
		]
	};

	sub10 = {
		key: 10,
		active: '0-0',
		items: [
			{
				collapsed: false,
				name: '<% multilang(LANG_STATISTICS); %>',
				items: [
#ifdef CONFIG_SFU
					{
						name: '<% multilang(LANG_STATISTICS); %>',
						href: pageRoot + 'stats.asp'
					}
#else
					{
						name: '<% multilang(LANG_INTERFACE); %>',
						href: pageRoot + 'stats.asp'
					}
#endif
#ifdef CONFIG_DEV_xDSL
					,
					{
						name: '<% multilang(LANG_ADSL); %>',
						href: pageRoot + '/admin/adsl-stats.asp'
					}
#endif
#ifdef CONFIG_DSL_VTUO
					,
					{
						name: '<% multilang(LANG_VTUO_DSL); %>',
						href: pageRoot + '/admin/vtuo-stats.asp'
					}
#endif /*CONFIG_DSL_VTUO*/

#ifdef CONFIG_USER_XDSL_SLAVE
					,
					{
						name: '<% multilang(LANG_ADSL_SLAVE_STATISTICS); %>',
						href: pageRoot + '/admin/adsl-slv-stats.asp'
					}
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
					,
					{
						name: '<% multilang(LANG_PON_STATISTICS); %>',
						href: pageRoot + '/admin/pon-stats.asp'
					}
#endif
				]
			}
		]
	};

#ifdef OSGI_SUPPORT
	sub11 = {
		key: 11,
		active: '0-0',
		items: [
			{
				collapsed: false,
				name: '<% multilang(LANG_OSGI); %>',
				items: [
					{
						name: '<% multilang(LANG_FRAMEWORK_INFO); %>',
						href: pageRoot + 'osgi_fwork.asp'
					}
					,
					{
						name: '<% multilang(LANG_BUNDLE_INSTALLATION); %>',
						href: pageRoot + 'osgi_bndins.asp'
					}
					,
					{
						name: '<% multilang(LANG_BUNDLE_MANAGEMENT); %>',
						href: pageRoot + 'osgi_bndmgt.asp'
					}
				]
			}
		]
	};
#endif

/* not use for deonet
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
		sub12={
				key:12,
				active:'0-0',
				items:[
					{
						collapsed: false,
						name: '<% multilang(LANG_OPERATION_MODE); %>',
						items: [
							{
								name: '<% multilang(LANG_INTERNET_OPERATION_MODE); %>',
								href: pageRoot + 'opmode.asp'
							}
						]
					}
				]
			};
#endif
*/
	side.push(sub0);
#ifdef CONFIG_00R0
	side.push(sub1);
#endif
	side.push(sub2);
#ifdef WLAN_SUPPORT
	side.push(sub3);
#endif
#ifndef CONFIG_SFU
	side.push(sub4);
	side.push(sub5);
#else
	side.push(sub5);
#endif
#ifdef VOIP_SUPPORT
	side.push(sub6);
#endif
	side.push(sub7);
	side.push(sub8);
	side.push(sub9);
	side.push(sub10);
#ifdef OSGI_SUPPORT
	side.push(sub11);
#endif

#ifdef CONFIG_USER_RTK_BRIDGE_MODE
	    side.push(sub12);
#endif
	return side;
}

function adaptNav(side, key) {
    key = (key - 0)
        || 0; //��ֹ�����ַ�������
    var sideObj = {};
    for (var i = 0; i < side.length; i++) {
        if (side[i] && side[i].key === key) {
            sideObj.active = side[i].active;
            sideObj.items = side[i].items;
            for (var j = 0; j < sideObj.items.length; j++) {
                sideObj.items[j].index = j; //���õڶ���������;
            }
            return sideObj;
        }
    }
}
/**
 * ��side��������ģ��ƴ��������Ȼ����Ⱦ��ҳ��
 * @param key
 */
function renderSide(key) {
    var side = adaptNav(generateSide(), key);
    var tpl = $('#side-tmpl').html();
    var html = juicer(tpl, side);
//    var html = $('#side-tmpl').render(side);
    $('#side').html(html);
}
/**
 * ����ѡ�е�ǰ��
 */
function setActive(items, current) {
    $(items).removeClass('active');
    $(current).addClass('active');
}
/**
 * �۵���չ��side
 * @param item
 */
function setAccordion(item) {
    var $item = $(item);
    var className = 'collapsed';
    var $currentLi = $item.parents('li');
    var $allLi = $item.parents('#side').children('li');
    var $currentContent = $currentLi.children('ul');
    $allLi.addClass(className);
    $currentLi.removeClass(className);
}

var flag_web = 0;
function replacePwPage()
{
        var flag_c = "<% getReplacePwPage(); %>";
		if (!flag_web) {
			if (flag_c == "Yes") {
				$("#9").trigger("click"); // admin
				$("#password\\.asp").trigger("click"); // password
				alert('<% multilang(LANG_DEFAULT_PASSWORD_CHECK); %>');
				flag_web = 1;
			}
		}
}


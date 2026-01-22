#
# Makefile for the GoAhead web server reference source base
#  for the Linux OS
#
# Copyright (c) GoAhead Software, Inc. 1995-2000
#
# $Id: Makefile,v 1.106 2013/01/14 09:00:06 rubychou Exp $
#

ifndef UCLINUX_BUILD_USER
LIBCRYPT = -lcrypt
ifndef CONFIG_USE_RSDK_WRAPPER
LDFLAGS = -L. -L/lib
endif #CONFIG_USE_RSDK_WRAPPER
endif

ifdef CONFIG_YUEME
YUEME_CU_FEATURE=y
endif
ifdef CONFIG_CU_BASEON_YUEME
YUEME_CU_FEATURE=y
endif

ifeq ($(CROSS_TARGET),aarch64-linux)
OS_ARCH=aarch64
else
ifeq ($(CROSS_TARGET),arm-linux)
OS_ARCH=aarch32
else
OS_ARCH=mips
endif
endif

all:	compile

WEB_SRC_DIR = form_src
WEB_LIB	= libwebs.a
MIB_LIB = libmib.a
MIB_LIB_SO = libmib.so
ifeq ($(CONFIG_USER_CLI),y)
CLI = cli
endif

INFORMGETDATA_LIB_SO = libinformgetdata.so

ifndef CONFIG_USE_RSDK_WRAPPER
# Dick
ifdef NOT_IN_UCLINUX_DIST
CC = mips-uclibc-gcc
STRIP = mips-uclibc-strip
endif
endif #CONFIG_USE_RSDK_WRAPPER



# User Management switch
UMSW	= -DUSER_MANAGEMENT_SUPPORT

ifeq ($(ACTION),1)
RTFLAGS += -DAPPLY_CHANGE
endif

CONFIG_MMU = 1

ifdef CONFIG_MMU
RTFLAGS += -DCONFIG_MMU
FLASHDRV = rtl_flashdrv_mmu
else
FLASHDRV = rtl_flashdrv
endif

#Add for elink
LIBELINKSDK_DIR = $(ROOTDIR)/user/lib_elinksdk
ifeq ($(CONFIG_ELINKSDK_SUPPORT),y)
LDFLAGS += -I$(LIBELINKSDK_DIR) -I$(ROOTDIR)/lib/$(openssl_dir)/include
endif

ifeq ($(CONFIG_WIFI_TIMER_SCHEDULE), y)
WIFI_TIMER_SCHEDULE = y
endif
ifeq ($(CONFIG_USER_SLEEP_TIMER), y)
SLEEP_TIMER = y
endif
ifeq ($(CONFIG_LED_INDICATOR_TIMER), y)
LED_TIMER = y
endif

# Digest Access switch
#DASW	= -DDIGEST_ACCESS_SUPPORT

# SSL switches (enable all to activate)
# SSLINC = /RSASSL/library/include
# SSLLIB = /RSASSL/library/lib/libsslc.a
# SSLSW	= -DWEBS_SSL_SUPPORT -I$(SSLINC)
# SSLPATCHFILE = ../websSSL.o

# If-Modified-Support switches (requires math library, libm.a)
# IFMODSW = -DWEBS_IF_MODIFIED_SUPPORT
# IFMODLIB = /usr/lib/libm.a

# Dependencies
exec_y =

exec_$(CONFIG_USER_CLI)			+= cli
exec_$(CONFIG_USER_RTK_SYSLOG)		+= slogd
exec_$(CONFIG_USER_FON)			+= fonservice
exec_$(WIFI_TIMER_SCHEDULE)		+= config_wlan
exec_$(CONFIG_USER_WIRELESS_TOOLS)	+= wlan_apply
exec_$(LED_TIMER)			+= config_led
exec_$(SLEEP_TIMER)			+= config_sleepmode
exec_$(CONFIG_USER_CLUSTER_MANAGE) += cluster_client
exec_$(CONFIG_USER_CLUSTER_MANAGE) += cluster_server

ifeq ($(CONFIG_WLAN_SCHEDULE_SUPPORT),y)
	exec_$(CONFIG_WLAN_SCHEDULE_SUPPORT)      += wlan_schedule
endif

ifeq ($(CONFIG_E8B),y)
exec_y += acl_limit_bandwidth
ifeq ($(CONFIG_SUPPORT_AUTO_DIAG),y)
exec_y += config_test
exec_y += autoSimu
ifeq ($(CONFIG_USER_LANNETINFO),y)
exec_y += autoPing
endif
endif
ifeq ($(CONFIG_USER_PPTP_CLIENT_PPTP),y)
ifeq ($(CONFIG_USER_L2TPD_L2TPD),y)
exec_y					+= VPNConnection
endif
endif
ifeq ($(CONFIG_USER_PPTP_CLIENT),y)
ifeq ($(CONFIG_USER_XL2TPD),y)
exec_y					+= VPNConnection
endif
endif
endif

#exec_$(CONFIG_CMCC)			+= bundle msg_debug
ifeq ($(CONFIG_APACHE_FELIX_FRAMEWORK),y)
exec_$(CONFIG_CMCC)			+= bundle
endif
#exec_$(CONFIG_CU_BASEON_CMCC)			+= bundle
ifeq ($(CONFIG_APACHE_FELIX_FRAMEWORK),y)
exec_$(CONFIG_CU_BASEON_CMCC)			+= bundle
endif

ifneq ($(CONFIG_USER_XMLCONFIG),y)
exec_y					+= flash saveconfig loadconfig hsinfo
endif
exec_y					+= startup qc ShowStatus configd mib
ifdef CONFIG_00R0
exec_y					+= crc_count
endif
ifdef CONFIG_IPOE_ARPING_SUPPORT
exec_y					+= arping_ipoe
endif
exec_$(CONFIG_APOLLO_MP_TEST)			+= show ledtest led

exec_$(CONFIG_USER_SUPPORT_EXTERNAL_SWITCH)	+= syncPhyPortMapping

exec_y += consoleset
exec_y += arpsend
exec_$(CONFIG_USER_RTK_CONVERT_XML) += convertxml

ifeq ($(CONFIG_GENERAL_WEB),y)
exec_y                  += menu-js
endif
ifndef CONFIG_RTL_MULTI_PHY_ETH_WAN
ifdef CONFIG_COMMON_RT_API
exec_y					+= PortMirror
endif
endif
ifeq (y,$(filter $(CONFIG_EPON_FEATURE) $(CONFIG_GPON_FEATURE) $(CONFIG_FIBER_FEATURE),y))
exec_y					+= checkpon
endif
ifeq ($(CONFIG_USER_TIMELYCHECK),y)
exec_y					+= timelycheck
endif
ifdef CONFIG_USER_WIRED_8021X
exec_y					+= Wired8021x
endif

ifdef CONFIG_CU_DPI
exec_y					+= informgetd
endif
ifeq ($(CONFIG_CMCC_ENTERPRISE),y)
exec_y += schedule_time_update
endif
ifeq ($(CONFIG_RTL8277C_SERIES),y)
ifneq ("$(wildcard rtl8277c_config.c)","")
exec_y += rtl8277c_config
endif
endif
ifeq ($(CONFIG_USER_SUPPORT_XML_FORMAT_TURN),y)
exec_y += xml_encrypt_tool_mips
endif

# MIB_DEPEND_FILES = mib.o mibtbl.o $(FLASHDRV).o utility.o adsl_drv.o debug.o arping.o ../msgutil.o sysconfig.o
MIB_DEPEND_FILES = $(FLASHDRV).o utility.o debug.o arping.o ../msgutil.o \
				   sysconfig.o lzss.o subr_dhcpv6.o subr_natlb.o subr_net.o subr_fw.o subr_timer.o subr_mcast.o subr_platform.o wget.o

ifeq ($(CONFIG_USER_RTK_MULTI_AP),y)
MIB_DEPEND_FILES += cJSON.o
else
ifeq ($(CONFIG_E8B),y)
ifeq ($(CONFIG_USER_PPTP_CLIENT_PPTP),y)
ifeq ($(CONFIG_USER_L2TPD_L2TPD),y)
MIB_DEPEND_FILES += cJSON.o
endif
endif
ifeq ($(CONFIG_USER_PPTP_CLIENT),y)
ifeq ($(CONFIG_USER_XL2TPD),y)
MIB_DEPEND_FILES += cJSON.o
endif
endif
endif
endif

ifeq ($(CONFIG_E8B),y)
ifeq ($(CONFIG_USER_PPTP_CLIENT_PPTP),y)
ifeq ($(CONFIG_USER_L2TPD_L2TPD),y)
MIB_DEPEND_FILES += VPNConnection.o
endif
endif
endif

ifeq ($(CONFIG_E8B),y)
ifeq ($(CONFIG_USER_PPTP_CLIENT),y)
ifeq ($(CONFIG_USER_XL2TPD),y)
MIB_DEPEND_FILES += VPNConnection.o
endif
endif
endif

ifeq ($(CONFIG_USER_PPTP_CLIENT_PPTP),y)
MIB_DEPEND_FILES += subr_vpn.o
else ifeq ($(CONFIG_USER_L2TPD_L2TPD),y)
MIB_DEPEND_FILES += subr_vpn.o
else ifeq ($(CONFIG_USER_PPTP_CLIENT),y)
MIB_DEPEND_FILES += subr_vpn.o
else ifeq ($(CONFIG_USER_XL2TPD),y)
MIB_DEPEND_FILES += subr_vpn.o
endif

ifdef CONFIG_LUNA
ifeq ($(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE),y)
MIB_DEPEND_FILES += fc_api.o
endif
endif
ifdef CONFIG_LUNA_FIRMWARE_UPGRADE_SUPPORT
$(ROOTDIR)/user2/rtk_nv/libnv.o:
	make -C $(ROOTDIR)/user2/rtk_nv/
MIB_DEPEND_FILES += $(ROOTDIR)/user2/rtk_nv/libnv.o
ifeq ($(CONFIG_ARM64),y)
MIB_DEPEND_FILES += $(ROOTDIR)/user2/rtk_nv/uboot_2015_crc32.o
else
MIB_DEPEND_FILES += $(ROOTDIR)/user2/rtk_nv/rtk_crc32.o
endif
endif

ifdef CONFIG_USER_LOGIN_WEB_PASSWORD_ENCRYPT
MIB_DEPEND_FILES += ../md5.o
MIB_DEPEND_FILES += ../web_psd_encrypt.o
else
ifdef UCLINUX_BUILD_USER
ifndef CONFIG_LIB_LIBMD5_FORCE
MIB_DEPEND_FILES += ../md5.o
endif
else
MIB_DEPEND_FILES += ../md5.o
endif
endif

ifeq ($(YUEME_CU_FEATURE),y)
ENABLE_SLRE = y
endif

ifeq ($(CONFIG_RTL8852AE_BACKPORTS),y)
BUILD_WLAN_WITH_HAPD ?= y
endif
ifneq ($(CONFIG_RTL8852AE),)
BUILD_WLAN_WITH_HAPD ?= y
endif
ifneq ($(CONFIG_RTLWIFI6),)
BUILD_WLAN_WITH_HAPD ?= y
endif
ifneq ($(CONFIG_RTL8852CE),)
BUILD_WLAN_WITH_HAPD ?= y
endif
obj2_y =

obj2_$(CONFIG_HWNAT)				+= hwnat_ioctl.o
#obj2_$(CONFIG_USER_XDSL_SLAVE)			+= subr_nfbi_api.o
obj2_$(CONFIG_DEV_xDSL)				+= adsl_drv.o subr_dsl.o
obj2_$(CONFIG_USER_IP_QOS)			+= subr_qos_3.o
obj2_$(CONFIG_IPV6)				+= subr_ipv6.o subr_4over6.o
obj2_$(CONFIG_E8B)                              += subr_e8.o
ifeq ($(BUILD_WLAN_WITH_HAPD),y)
ifeq ($(CONFIG_RTK_DEV_AP), y)
obj2_$(CONFIG_USER_WIRELESS_TOOLS)              += subr_wlan_wifi5_wifi6.o hapd_conf.o wifi6_priv_conf.o subr_wlan_common.o wlan_manager_conf.o
else
ifneq ($(CPTCFG_RTL8192CD)$(CONFIG_RTL8192CD),)
obj2_$(CONFIG_USER_WIRELESS_TOOLS)		+= subr_wlan_wifi5_wifi6.o hapd_conf.o subr_wlan_common.o
else
obj2_$(CONFIG_USER_WIRELESS_TOOLS)		+= subr_wlan_wifi6.o hapd_conf.o
endif
endif
CFLAGS += -DWIFI6_SUPPORT
else
obj2_$(CONFIG_USER_WIRELESS_TOOLS)		+= subr_wlan.o
endif
ifneq ($(CONFIG_USER_WIRELESS_TOOLS),y)
ifeq ($(CONFIG_USER_RTK_MULTI_AP),y)
obj2_y						+= subr_wlan_wifi6.o hapd_conf.o
CFLAGS += -DRTK_MULTI_AP_ETH_ONLY=1 -DWLAN_SUPPORT=1
export RTK_MULTI_AP_ETH_ONLY:=y
endif
endif
obj2_$(CONFIG_USER_PORTMAPPING)			+= subr_pmap.o
obj2_y                  	+= subr_port.o
obj2_y                          += subr_switch.o
obj2_$(CONFIG_USER_BRIDGE_GROUPING)		+= subr_bridgegroup.o
obj2_$(CONFIG_NET_IPGRE)			+= subr_gre.o
ifeq ($(CONFIG_GPON_FEATURE),y)
obj2_y						+= subr_pon.o
else
ifeq ($(CONFIG_EPON_FEATURE),y)
obj2_y						+= subr_pon.o
endif
endif

obj2_$(CONFIG_USER_VXLAN)			+= subr_vxlan.o

ifeq ($(CONFIG_MGTS),y)
obj2_y						+= subr_mgts.o
else
obj2_y						+= subr_generic.o
endif

obj2_$(CONFIG_USER_RTK_MULTI_AP) += subr_multiap.o

obj2_$(CONFIG_CMCC)				+= subr_cmcc.o

obj2_$(CONFIG_CU)				+= subr_cmcc.o
obj2_$(CONFIG_CU)				+= subr_cu.o
obj2_$(CONFIG_USER_LANNETINFO)			+= subr_lannetinfo.o
obj2_$(CONFIG_USER_SAMBA)			+= subr_samba.o

obj2_$(YUEME_CU_FEATURE)				+= subr_yueme.o subr_yueme_pr.o
obj2_$(CONFIG_AIRTEL_EMBEDUR)			+= subr_airtel.o
obj2_$(ENABLE_SLRE)				+= slre.o
obj2_$(CONFIG_CTC_SDN)			+= subr_ctcsdn.o

obj2_$(CONFIG_USER_WLAN_QCSAPI) += rt_qcsapi.o
obj2_$(CONFIG_ATM_REALTEK)			+= subr_pvcsearch.o

ifdef_any_of = $(filter-out undefined,$(foreach v,$(1),$(origin $(v))))
ifneq ($(call ifdef_any_of,CONFIG_MULTI_FTPD_ACCOUNT CONFIG_MULTI_SMBD_ACCOUNT),)
obj2_y						+= subr_service.o
endif
obj2_$(LED_TIMER)				+= subr_led.o

ifeq ($(YUEME_CU_FEATURE),y)
obj2_$(CONFIG_USER_SAMBA)			+= subr_samba_init.o
endif

obj2_y			+= subr_sockmark.o

MIB_DEPEND_FILES += $(obj2_y)

startobj_y = startup.o
ifeq ($(CONFIG_CMCC_BEIJING_GROUP_VERIFICATION),y)
exec_y					+= counterRec
COUNTER_REC_FILE = counterRec.o
endif
startobj_$(CONFIG_USER_FON)	+= start_fon.o
STARTUP_FILES = $(startobj_y)

mib_obj_y = mib.o load_save_xml_file.o

ifdef CONFIG_USER_VSNTP
MIB_DEPEND_FILES += tz.o
endif
MIB_DEPEND_FILES += ./form_src/multilang.o

FLASH   = -DINCLUDE_DEFAULT_VALUE
RTFLAGS += -DNO_ACTION
WEBFLAG	= -DWEBS -DUEMF -DOS="LINUX" -DLINUX $(WLANSUPP) $(UMSW) $(DASW) \
	$(SSLSW) $(IFMODSW) $(RTFLAGS)
OTHERS	= -DB_STATS -DB_FILL -DDEBUG
#DEBUG	= -g -Wall
#DEBUG	=
DEBUG	= -D_DEBUG_MESSAGE
IFLAGS 	= -I..

ifeq ($(CONFIG_USER_RTK_VOIP),y)
VOIP_APP = $(ROOTDIR)/user/rtk_voip
FLASH_FILE = voip_flash.o voip_flash_mib.o voip_flash_tool.o voip_flash_server.o voip_line_state_share.o
MIB_DEPEND_FILES += $(FLASH_FILE)
CFLAGS += -DVOIP_SUPPORT -I$(VOIP_APP)/includes -I$(VOIP_APP)/web -I$(ROOTDIR)/rtk_voip/voip_manager_1.6/includes -I$(ROOTDIR)/$(LINUXDIR)/rtk_voip/include
LDFLAGS += -L$(ROOTDIR)/rtk_voip/voip_manager_1.6/ -lvoip_manager
RTK_VOIP_WEB = rtk_voip_target
rtk_voip_target:
	make -C $(VOIP_APP)/web -f rtl867x.mak WEB_FLAGS='$(DEBUG) $(CFLAGS) $(IFLAGS) -D_GNU_SOURCE' WEB_DIR=$(ROOTDIR)/romfs/home/httpd/web
endif

CFLAGS += -I$(PWD) -I$(PWD)/..

ifeq ($(CONFIG_USER_WLAN_QCSAPI),y)
QCSAPI_APP = $(ROOTDIR)/user/qcsapi
QCS_CFLAGS = -I$(QCSAPI_APP) -I. -fPIC -O -g -Wall -Werror -Wno-unused-variable -Wno-unused-parameter -DPCIE_RPC_TYPE=RPC_TYPE_QCSAPI_PCIE
LDFLAGS += -L$(QCSAPI_APP) -lqcsapi_client
endif

ifeq ($(YUEME_CU_FEATURE),y)
MIB_DEPEND_FILES += ftp_server_api.o
CFLAGS += -I$(ROOTDIR)/user/ctcyueme/ftpapi
endif
ifeq ($(CONFIG_VIR_BOA_JEMBENCHTEST),y)
JSPEED_FILES= jspeed.o ../md5.o ../sha256.o
JSPEED_LIB = ./release/$(OS_ARCH)/jspeed.a
JSPEED_LIB_SO = jspeed.so
MIB_DEPEND_FILES += $(JSPEED_LIB)
endif
MIB_DEPEND_FILES += ../sha256.o
ifeq ($(CONFIG_LIB_8367),y)
CFLAGS += -I$(ROOTDIR)/user/lib8367
endif

ifeq ($(CONFIG_USER_RTK_EXTGPHY),y)
CFLAGS += -I$(ROOTDIR)/user/libextgphy
MIB_DEPEND_FILES += $(ROOTDIR)/user/libextgphy/lib_extgphy.o
endif

ifeq ($(CONFIG_RTK_OMCI_V1), y)
OMCILIB = -lomci_api
endif	#CONFIG_RTK_OMCI_V1

.PHONY: $(WEB_LIB)
#compile: cleanmib $(WEB_LIB) flash webpage startup
ifdef UCLINUX_BUILD_USER
compile: cleanmib $(WEB_LIB) $(MIB_LIB) $(exec_y)
else
compile: cleanmib $(WEB_LIB) $(MIB_LIB)
endif
#compile: cleanmib $(NAME) cleanmib flash
#compile: cleanmib $(NAME) webpage

#
#	Build archive of objects
#
ifeq ($(CONFIG_VIR_BOA_JEMBENCHTEST),y)
ifneq ($(wildcard ./jspeed.c),)
$(JSPEED_LIB):$(JSPEED_FILES)
	$(AR) $(ARFLAGS) $(JSPEED_LIB) $(JSPEED_FILES)
endif
endif

$(WEB_LIB): $(RTK_VOIP_WEB)
	cd $(WEB_SRC_DIR) && $(MAKE)

$(MIB_LIB): $(MIB_DEPEND_FILES)
	$(AR) $(ARFLAGS) $(MIB_LIB) $(MIB_DEPEND_FILES)
ifdef UCLINUX_BUILD_USER
	#$(LD) -shared --warn-common --warn-once -z combreloc -soname=$(MIB_LIB_SO).0 \
	#	-o $(MIB_LIB_SO) --whole-archive $(MIB_LIB)
	$(CC) -shared -Wl,-soname,$(MIB_LIB_SO).0 -o $(MIB_LIB_SO) $(MIB_LIB) $(MIB_DEPEND_FILES) -lcrypt $(filter-out -pie, $(LDFLAGS))
  ifdef CONFIG_USE_RSDK_WRAPPER
	install -m 644 $(MIB_LIB_SO) $(ROOTDIR)/lib/librtk/$(MIB_LIB_SO)
	install -m 644 $(MIB_LIB) $(ROOTDIR)/lib/librtk/$(MIB_LIB)
  else #CONFIG_USE_RSDK_WRAPPER
	install -m 644 $(MIB_LIB_SO) ../../../../uClibc/lib/$(MIB_LIB_SO)
  endif #CONFIG_USE_RSDK_WRAPPER
endif

ifdef CONFIG_CU_DPI
	$(CC) -shared -fPIC $(CFLAGS) informgetdata.c -o $(INFORMGETDATA_LIB_SO)
	install -m 644 $(INFORMGETDATA_LIB_SO) $(ROOTDIR)/lib/librtk/$(INFORMGETDATA_LIB_SO)
endif

#
#	Primary link
#
ifeq ($(CONFIG_USER_CLI),y)
ifeq ($(CONFIG_USER_MENU_CLI),y)
cli: climenu.o cli_utility.o $(FLASHDRV).c mib.h mibtbl.h rtl_flashdrv.h $(MIB_LIB)
	$(CC) -o cli \
		climenu.o cli_utility.o $(LIBCRYPT) $(LDFLAGS) $(LIBMIB) $(LIBMD5)
endif
#added by xl_yue
ifeq ($(CONFIG_USER_CMD_CLI),y)
cli: clicmd.o cfgutility.o cmdedit.o $(FLASHDRV).c mib.h mibtbl.h rtl_flashdrv.h $(MIB_LIB)
	$(CC) -o cli \
		clicmd.o cfgutility.o cmdedit.o $(LIBCRYPT) $(LDFLAGS) $(LIBMIB) $(LIBMD5)
endif
endif

mib: mibutil.c $(FLASHDRV).c mib.h mibtbl.h rtl_flashdrv.h $(mib_obj_y) mibtbl.o lzss.o $(MIB_LIB)
	$(CC) -c -o mibutil.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) mibutil.c
	$(CC) -o mib \
		mibutil.o $(mib_obj_y) mibtbl.o lzss.o $(LDFLAGS) $(LIBMIB)

ifeq ($(CONFIG_GENERAL_WEB),y)
menu-js: mib.h options.h
	$(CROSS_COMPILE)cpp -P -o ../web/admin/userMenu.js ../web/admin/userMenu_base.js $(CFLAGS) $(WEBFLAG) $(IFLAGS) -include options.h
	$(CROSS_COMPILE)cpp -P -o ../web/adminMenu.js ../web/adminMenu_base.js $(CFLAGS) $(WEBFLAG) $(IFLAGS) -include options.h
ifeq ($(CONFIG_00R0),y)
	$(CROSS_COMPILE)cpp -P -o ../web/index.asp ../web/index_base.asp $(CFLAGS) $(WEBFLAG) $(IFLAGS) -DFIRMVER=$(VERSIONPKG) -include options.h
	$(CROSS_COMPILE)cpp -P -o ../web/index_user.asp ../web/index_user_base.asp $(CFLAGS) $(WEBFLAG) $(IFLAGS) -DFIRMVER=$(VERSIONPKG) -include options.h
endif
	$(CROSS_COMPILE)cpp -P -o ../web/index.html ../web/index_base.html $(CFLAGS) $(WEBFLAG) $(IFLAGS) -DFIRMVER=$(VERSIONPKG) -include options.h
	$(CROSS_COMPILE)cpp -P -o ../web/index_user.html ../web/index_user_base.html $(CFLAGS) $(WEBFLAG) $(IFLAGS) -DFIRMVER=$(VERSIONPKG) -include options.h
	$(CROSS_COMPILE)cpp -P -o ../web/admin/login.asp ../web/admin/login_base.asp $(CFLAGS) $(WEBFLAG) $(IFLAGS) -include options.h -DFIRMVER=$(VERSIONPKG)
endif

hsinfo: hsinfo.c $(FLASHDRV).c mib.h mibtbl.h rtl_flashdrv.h $(mib_obj_y) mibtbl.o lzss.o $(MIB_LIB)
	$(CC) -c -o hsinfo.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) hsinfo.c
	$(CC) -o hsinfo \
		hsinfo.o $(mib_obj_y) mibtbl.o lzss.o $(LDFLAGS) $(LIBMIB)

ifneq ($(CONFIG_USER_XMLCONFIG),y)
flash: flash.c $(FLASHDRV).c mib.h mibtbl.h rtl_flashdrv.h $(mib_obj_y) mibtbl.o lzss.o $(MIB_LIB)
	$(CC) -c -o flash.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) flash.c
	$(CC) -o flash \
		flash.o $(mib_obj_y) mibtbl.o lzss.o $(LDFLAGS) $(LIBMIB)

saveconfig: saveconfig.c $(FLASHDRV).c mib.h mibtbl.h $(mib_obj_y) mibtbl.o rtl_flashdrv.h $(MIB_LIB)
	$(CC) -c -o saveconfig.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) saveconfig.c
	$(CC) -o saveconfig \
		saveconfig.o $(mib_obj_y) mibtbl.o $(LDFLAGS) $(LIBMIB)

loadconfig: loadconfig.c $(FLASHDRV).c mib.h mibtbl.h $(mib_obj_y) mibtbl.o rtl_flashdrv.h $(MIB_LIB)
	$(CC) -c -o loadconfig.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) loadconfig.c
	$(CC) -o loadconfig \
		loadconfig.o $(mib_obj_y) mibtbl.o $(LDFLAGS) $(LIBMIB)
endif

ifeq ($(CONFIG_APOLLO_MP_TEST),y)
show: show.c mib.h mibtbl.h $(MIB_LIB)
	$(CC) -c -o show.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) show.c -DFIRMVER=$(VERSIONPKG)
	$(CC) -o show \
		 show.o $(LDFLAGS) $(LIBMIB)

led: led.c mib.h mibtbl.h $(mib_obj_y) mibtbl.o $(MIB_LIB)
	$(CC) -c -o led.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) led.c
	$(CC) -o led \
		 led.o $(mib_obj_y) mibtbl.o $(LDFLAGS) $(LIBMIB)

ledtest: led_test.c
	$(CC) -c -o led_test.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) led_test.c
	$(CC) -o ledtest \
		led_test.o $(LDFLAGS) $(LIBMIB)
endif

ifeq ($(CONFIG_USER_SUPPORT_EXTERNAL_SWITCH),y)
syncPhyPortMapping: syncPhyPortMapping.c
	$(CC) -c -o syncPhyPortMapping.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) syncPhyPortMapping.c
	$(CC) -o syncPhyPortMapping \
		syncPhyPortMapping.o $(LDFLAGS) $(LIBMIB)
endif

consoleset: console_set.c
	$(CC) -c -o console_set.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) console_set.c
	$(CC) -o consoleset \
		console_set.o $(LDFLAGS) $(LIBMIB)

arpsend: arpsend.c mib.h mibtbl.h $(MIB_LIB)
	$(CC) -c -o arpsend.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) arpsend.c
	$(CC) -o arpsend \
		arpsend.o $(LDFLAGS) $(LIBMIB)

convertxml: convertXML.c
	$(CC) -c -o convertXML.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) convertXML.c
	$(CC) -o convertxml \
		convertXML.o $(LDFLAGS) $(LIBMIB)

qc: qc.c mib.c mibtbl.c $(FLASHDRV).c mib.h mibtbl.h rtl_flashdrv.h $(MIB_LIB)
	$(CC) -c -o qc.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) qc.c
	$(CC) -o qc \
		qc.o $(LDFLAGS) $(LIBMIB)

ShowStatus: ShowStatus.c mib.c mibtbl.c $(FLASHDRV).c mib.h mibtbl.h rtl_flashdrv.h $(MIB_LIB)
	$(CC) -c -o ShowStatus.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) ShowStatus.c
	$(CC) -o ShowStatus \
		ShowStatus.o $(LDFLAGS) $(LIBMIB)

ifdef CONFIG_USER_WIRED_8021X
Wired8021x: Wired8021x.c mib.c mibtbl.c $(FLASHDRV).c mib.h mibtbl.h rtl_flashdrv.h $(MIB_LIB)
	$(CC) -c -o Wired8021x.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) Wired8021x.c
	$(CC) -o Wired8021x \
		Wired8021x.o $(LDFLAGS) $(LIBMIB)
endif

ifndef CONFIG_RTL_MULTI_PHY_ETH_WAN
ifdef CONFIG_COMMON_RT_API
PortMirror: PortMirror.c mib.c mibtbl.c $(FLASHDRV).c mib.h mibtbl.h rtl_flashdrv.h $(MIB_LIB)
	$(CC) -c -o PortMirror.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) PortMirror.c
	$(CC) -o PortMirror \
		PortMirror.o $(LDFLAGS) -lpthread $(LIBMIB)
endif
endif

ifeq ($(CONFIG_E8B),y)
acl_limit_bandwidth: acl_limit_bandwidth.o mib.h mibtbl.h $(MIB_LIB)
	$(CC) -c -o acl_limit_bandwidth.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) acl_limit_bandwidth.c
	$(CC) -o acl_limit_bandwidth \
		acl_limit_bandwidth.o $(LDFLAGS) $(LIBMIB)

ifeq ($(CONFIG_SUPPORT_AUTO_DIAG),y)
config_test: config_test.c mib.h mibtbl.h $(MIB_LIB)
	$(CC) -c -o config_test.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) config_test.c
	$(CC) -o config_test \
		config_test.o $(LDFLAGS) $(LIBMIB)

autoSimu: autoSimu.c mib.h mibtbl.h $(MIB_LIB)
	$(CC) -c -o autoSimu.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) autoSimu.c
	$(CC) -o autoSimu \
		autoSimu.o $(LDFLAGS) $(LIBMIB)
ifeq ($(CONFIG_USER_LANNETINFO),y)
autoPing: autoPing.c mib.h mibtbl.h $(MIB_LIB)
	$(CC) -c -o autoPing.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) autoPing.c
	$(CC) -o autoPing \
		autoPing.o $(LDFLAGS) $(LIBMIB)
endif
endif
ifeq ($(CONFIG_USER_PPTP_CLIENT_PPTP),y)
ifeq ($(CONFIG_USER_L2TPD_L2TPD),y)
VPNConnection: VPNConnection.c mib.c mibtbl.c $(FLASHDRV).c mib.h mibtbl.h rtl_flashdrv.h $(MIB_LIB)
	$(CC) -c -o VPNConnection.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) $(INCSSL) VPNConnection.c
	$(CC) -o VPNConnection \
		VPNConnection.o $(LDFLAGS) $(LIBMIB)
endif
endif
ifeq ($(CONFIG_USER_PPTP_CLIENT),y)
ifeq ($(CONFIG_USER_XL2TPD),y)
VPNConnection: VPNConnection.c mib.c mibtbl.c $(FLASHDRV).c mib.h mibtbl.h rtl_flashdrv.h $(MIB_LIB)
	$(CC) -c -o VPNConnection.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) $(INCSSL) VPNConnection.c
	$(CC) -o VPNConnection \
		VPNConnection.o $(LDFLAGS) $(LIBMIB)
endif
endif
endif

ifeq ($(CONFIG_CMCC),y)
bundle: bundle.c
	$(CC) $(CFLAGS) -c -o bundle.o $(DEBUG) $(IFLAGS) bundle.c
	$(CC) -o bundle bundle.o $(LDFLAGS) $(LIBMIB)

msg_debug: msg_debug.c
	$(CC) -c -o msg_debug.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) $(INCSSL)  msg_debug.c
	$(CC) -o  msg_debug \
		msg_debug.o $(LDFLAGS) $(LIBMIB)
endif

ifeq ($(CONFIG_CU_BASEON_CMCC),y)
bundle: bundle.c
	$(CC) $(CFLAGS) -c -o bundle.o $(DEBUG) $(IFLAGS) bundle.c
	$(CC) -o bundle bundle.o $(LDFLAGS) $(LIBMIB)

msg_debug: msg_debug.c
	$(CC) -c -o msg_debug.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) $(INCSSL)  msg_debug.c
	$(CC) -o  msg_debug \
		msg_debug.o $(LDFLAGS) $(LIBMIB)
endif

startup: Makefile $(STARTUP_FILES) mib.h mibtbl.h rtl_flashdrv.h $(MIB_LIB)
ifneq ($(filter y,$(CONFIG_00R0) $(CONFIG_E8B)),)
	$(CC) -o startup $(STARTUP_FILES) $(LDFLAGS) $(LIBMIB) $(OMCILIB) $(LIBMD5)
else
	$(CC) -o startup $(STARTUP_FILES) $(LDFLAGS) $(LIBMIB)
endif
#	$(CC) -o startup startup.o $(LDFLAGS) libmib.a

ifeq ($(CONFIG_CMCC_BEIJING_GROUP_VERIFICATION),y)
counterRec: Makefile $(COUNTER_REC_FILE) mib.h mibtbl.h rtl_flashdrv.h $(MIB_LIB)
	$(CC) -o counterRec $(COUNTER_REC_FILE) $(LDFLAGS) -lcrypt $(LIBMIB)
endif
configd: configd.o msgparser.o $(mib_obj_y) mibtbl.o lzss.o mib_reserve.o
	$(CC) $(CFLAGS) -o $@ $+ $(LDFLAGS) $(LIBMIB) -lrt

ifdef CONFIG_00R0
crc_count: crc_count.c
	$(CC) $(CFLAGS) -c -o crc_count.o $(DEBUG) crc_count.c
	$(CC) -o crc_count \
		crc_count.o $(LDFLAGS) $(LIBMIB)
endif

ifdef CONFIG_IPOE_ARPING_SUPPORT
arping_ipoe: arping_ipoe.c
	$(CC) $(CFLAGS) -c -o arping_ipoe.o $(DEBUG) arping_ipoe.c
	$(CC) -o arping_ipoe \
		arping_ipoe.o $(LDFLAGS) $(LIBMIB)
endif

slogd: slogd.c
	$(CC) $(CFLAGS) -M $(DEBUG) $(IFLAGS) slogd.c -MF slogd.d
	$(CC) $(CFLAGS) -DSYSLOG_NAMES -c -o slogd.o $(DEBUG) $(IFLAGS) slogd.c
	$(CC) $(LDFLAGS) -lc -o slogd $(LIBMIB) slogd.o

webpage:
	rm webpages.bin -f
	find ../web -name "*.*" >web_files

fonservice: Makefile fonservice.c mib.h mibtbl.h $(MIB_LIB)
	$(CC) -c -o fonservice.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) fonservice.c
	$(CC) -o fonservice \
                fonservice.o $(LDFLAGS) $(LIBMIB)

ifeq ($(CONFIG_USER_CLUSTER_MANAGE),y)
cluster_client: cluster_client.c cluster_common.o mib.h mibtbl.h $(MIB_LIB)
	$(CC) -c -o cluster_client.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) cluster_client.c
	$(CC) -o cluster_client \
		cluster_client.o cluster_common.o $(LDFLAGS) $(LIBMIB)

cluster_server: cluster_server.c cluster_common.o mib.h mibtbl.h $(MIB_LIB)
	$(CC) -c -o cluster_server.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) cluster_server.c
	$(CC) -o cluster_server \
		cluster_server.o cluster_common.o $(LDFLAGS) $(LIBMIB)
endif

ifeq ($(WIFI_TIMER_SCHEDULE), y)
config_wlan: config_wlan.c mib.h mibtbl.h $(MIB_LIB)
	$(CC) -c -o config_wlan.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) config_wlan.c
	$(CC) -o config_wlan \
		config_wlan.o $(LDFLAGS) $(LIBMIB)
endif

wlan_apply: wlan_apply.c mib.h mibtbl.h $(MIB_LIB)
	$(CC) -c -o wlan_apply.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) wlan_apply.c
	$(CC) -o wlan_apply wlan_apply.o $(LDFLAGS) $(LIBMIB)

ifeq ($(LED_TIMER),y)
config_led: config_led.c mib.h mibtbl.h $(MIB_LIB)
	$(CC) -c -o config_led.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) config_led.c
	$(CC) -o config_led \
		config_led.o $(LDFLAGS) $(LIBMIB)
endif

ifeq ($(SLEEP_TIMER), y)
config_sleepmode: config_sleepmode.c mib.h mibtbl.h $(MIB_LIB)
	$(CC) -c -o config_sleepmode.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) config_sleepmode.c
	$(CC) -o config_sleepmode \
		config_sleepmode.o $(LDFLAGS) $(LIBMIB)
endif
ifeq (y,$(filter $(CONFIG_EPON_FEATURE) $(CONFIG_GPON_FEATURE) $(CONFIG_FIBER_FEATURE),y))
checkpon: checkpon.c mib.h mibtbl.h rtl_flashdrv.h $(MIB_LIB)
	$(CC) -c -o checkpon.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) checkpon.c
	$(CC) -o checkpon checkpon.o $(LDFLAGS) $(LIBMIB)
endif

ifeq ($(CONFIG_WLAN_SCHEDULE_SUPPORT),y)
wlan_schedule: wlan_schedule.c mib.h sysconfig.h $(MIB_LIB)
	$(CC) -c -o wlan_schedule.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) wlan_schedule.c
	$(CC) -o wlan_schedule \
		wlan_schedule.o $(LDFLAGS) $(LIBMIB)
endif

ifeq ($(CONFIG_USER_TIMELYCHECK),y)
timelycheck: timelycheck.c mib.h $(MIB_LIB)
	$(CC) -c -o timelycheck.o $(FLASH) $(DEBUG) $(CFLAGS) $(WEBFLAG) $(IFLAGS) timelycheck.c
	$(CC) -o timelycheck \
		timelycheck.o $(LDFLAGS) $(LIBMIB)
endif

ifdef CONFIG_CU_DPI
informgetd: informgetd.c
	$(CC) -c -o informgetd.o informgetd.c $(CFLAGS)
	$(CC) -o informgetd informgetd.o $(LDFLAGS) $(LIBMIB)
endif

ifeq ($(CONFIG_CMCC_ENTERPRISE),y)
schedule_time_update: schedule_time_update.c
	$(CC) -c -o schedule_time_update.o schedule_time_update.c $(CFLAGS)
	$(CC) -o schedule_time_update schedule_time_update.o $(LDFLAGS) $(LIBMIB)
endif

ifeq ($(CONFIG_USER_SUPPORT_XML_FORMAT_TURN),y)
xml_encrypt_tool_mips: xml_encrypt_tool.c xml_encrypt.c
	$(CC) -c -o xml_encrypt_tool_mips.o xml_encrypt_tool.c $(CFLAGS)
	$(CC) -c -o xml_encrypt.o xml_encrypt.c $(CFLAGS)
	$(CC) -o xml_encrypt_tool_mips xml_encrypt.o xml_encrypt_tool_mips.o $(LDFLAGS) $(LIBMIB)
endif

ifeq ($(CONFIG_RTL8277C_SERIES),y)
rtl8277c_config: rtl8277c_config.c
	$(CC) -c -o rtl8277c_config.o rtl8277c_config.c $(CFLAGS)
	$(CC) -o rtl8277c_config rtl8277c_config.o $(LDFLAGS) $(LIBMIB)
endif

.PHONY: rcX
rcX:
ifeq ($(findstring y, $(LED_TIMER)$(SLEEP_TIMER)$(WIFI_TIMER_SCHEDULE)$(CONFIG_USER_RTK_SYSLOG_SAVE_TO_FLASH)),y)
	$(ROMFSINST) -a "mkdir -p /var/spool/cron/crontabs" $(RC_LEVEL_STARTUP)
endif
	$(ROMFSINST) -a "sysconf reboot 0" /etc/init.d/K10

romfs:
ifeq ($(CONFIG_USER_BOA_BOA),y)
	mkdir -p $(ROMFSDIR)/home/httpd
ifeq ("$(CONFIG_PRD_SIGNATURE)", "9601DVD")
	echo -n "Install web for 9601DVD"
	mkdir -p $(ROMFSDIR)/home/httpd/web/admin/graphics
	# Remove hidden files, ex. .svn, .git ...
	find $(ROMFSDIR)/home/httpd/web -name ".*" | xargs rm -rf
	rm -f $(ROMFSDIR)/home/httpd/web/*.bak
	rm -f $(ROMFSDIR)/home/httpd/web/admin/*.bak
	$(ROMFSINST) -s /home/httpd/web/share.js  /home/httpd/web/admin/share.js
	$(ROMFSINST) -s /home/httpd/web/base64_code.js /home/httpd/web/admin/base64_code.js
	$(ROMFSINST) -s /home/httpd/web/sha256_code.js /home/httpd/web/admin/sha256_code.js
	$(ROMFSINST) -s /home/httpd/web/acl.asp  /home/httpd/web/admin/acl.asp
	$(ROMFSINST) -s /home/httpd/web/url_blocking.asp  /home/httpd/web/admin/url_blocking.asp
	$(ROMFSINST) -s /home/httpd/web/fw-macfilter_rg.asp  /home/httpd/web/admin/fw-macfilter_rg.asp
	$(ROMFSINST) -s /home/httpd/web/reboot.asp  /home/httpd/web/admin/reboot.asp
	$(ROMFSINST) -s /home/httpd/web/multi_lang.asp  /home/httpd/web/admin/multi_lang.asp
	$(ROMFSINST) -s /home/httpd/web/saveconf.asp  /home/httpd/web/admin/saveconf.asp
	$(ROMFSINST) -s /home/httpd/web/status.asp  /home/httpd/web/admin/status.asp
	$(ROMFSINST) -e CONFIG_IPV6 -s /home/httpd/web/status_ipv6.asp  /home/httpd/web/admin/status_ipv6.asp
	$(ROMFSINST) -s /home/httpd/web/upgrade.asp  /home/httpd/web/admin/upgrade.asp
	$(ROMFSINST) -s /home/httpd/web/graphics/button-on.png  /home/httpd/web/admin/graphics/button-on.png
	$(ROMFSINST) -s /home/httpd/web/graphics/button-off.png  /home/httpd/web/admin/graphics/button-off.png
	$(ROMFSINST) -s /home/httpd/web/graphics/del.gif  /home/httpd/web/admin/graphics/del.gif
	$(ROMFSINST) -s /home/httpd/web/graphics/edit.gif  /home/httpd/web/admin/graphics/edit.gif
	$(ROMFSINST) -s /home/httpd/web/graphics/topbar.gif  /home/httpd/web/admin/graphics/topbar.gif
	$(ROMFSINST) -s /home/httpd/web/graphics/icon_menu_li.jpg  /home/httpd/web/admin/graphics/icon_menu_li.jpg
	$(ROMFSINST) -s /home/httpd/web/graphics/loading.gif  /home/httpd/web/admin/graphics/loading.gif
	$(ROMFSINST) -s /home/httpd/web/graphics/login_bg.png  /home/httpd/web/admin/graphics/login_bg.png
	$(ROMFSINST) -s /home/httpd/web/graphics/login_left.png  /home/httpd/web/admin/graphics/login_left.png
	$(ROMFSINST) -s /home/httpd/web/graphics/login_right.png  /home/httpd/web/admin/graphics/login_right.png
	$(ROMFSINST) -s /home/httpd/web/graphics/menu_bottom.jpg  /home/httpd/web/admin/graphics/menu_bottom.jpg
	$(ROMFSINST) -s /home/httpd/web/graphics/menu_title.jpg  /home/httpd/web/admin/graphics/menu_title.jpg
	$(ROMFSINST) -s /home/httpd/web/graphics/menu_top.jpg  /home/httpd/web/admin/graphics/menu_top.jpg
	$(ROMFSINST) -s /home/httpd/web/graphics/button_bg.jpg  /home/httpd/web/admin/graphics/button_bg.jpg
	$(ROMFSINST) -s /home/httpd/web/graphics/nav_bg.jpg  /home/httpd/web/admin/graphics/nav_bg.jpg
	$(ROMFSINST) -s /home/httpd/web/graphics/nav_left.jpg  /home/httpd/web/admin/graphics/nav_left.jpg
	$(ROMFSINST) -s /home/httpd/web/graphics/nav_line.jpg  /home/httpd/web/admin/graphics/nav_line.jpg
	$(ROMFSINST) -s /home/httpd/web/graphics/nav_right.jpg  /home/httpd/web/admin/graphics/nav_right.jpg
	$(ROMFSINST) -s /home/httpd/web/graphics/nice.png  /home/httpd/web/admin/graphics/nice.png
	$(ROMFSINST) -s /home/httpd/web/graphics/top_bg.jpg  /home/httpd/web/admin/graphics/top_bg.jpg
	$(ROMFSINST) -s /home/httpd/web/graphics/arrow-down.gif  /home/httpd/web/admin/graphics/arrow-down.gif
	$(ROMFSINST) -s /home/httpd/web/graphics/arrow-top.gif  /home/httpd/web/admin/graphics/arrow-top.gif
	$(ROMFSINST) -s /home/httpd/web/graphics/bad.png  /home/httpd/web/admin/graphics/bad.png
	$(ROMFSINST) -s /home/httpd/web/graphics/collapsed.png  /home/httpd/web/admin/graphics/collapsed.png
	$(ROMFSINST) -s /home/httpd/web/graphics/expanded.png  /home/httpd/web/admin/graphics/expanded.png
	$(ROMFSINST) -s /home/httpd/web/graphics/nice.png  /home/httpd/web/admin/graphics/nice.png
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_bar.gif  /home/httpd/web/admin/menu-images/menu_bar.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_corner.gif  /home/httpd/web/admin/menu-images/menu_corner.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_corner_minus.gif  /home/httpd/web/admin/menu-images/menu_corner_minus.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_corner_plus.gif  /home/httpd/web/admin/menu-images/menu_corner_plus.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_folder_closed.gif  /home/httpd/web/admin/menu-images/menu_folder_closed.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_folder_open.gif  /home/httpd/web/admin/menu-images/menu_folder_open.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_link_default.gif  /home/httpd/web/admin/menu-images/menu_link_default.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_pixel.gif  /home/httpd/web/admin/menu-images/menu_pixel.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_root.gif  /home/httpd/web/admin/menu-images/menu_root.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_tee.gif  /home/httpd/web/admin/menu-images/menu_tee.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_tee_minus.gif  /home/httpd/web/admin/menu-images/menu_tee_minus.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_tee_plus.gif  /home/httpd/web/admin/menu-images/menu_tee_plus.gif
	$(ROMFSINST) -s /home/httpd/web/wan3gconf.asp  /home/httpd/web/admin/wan3gconf.asp
	$(ROMFSINST) -s /home/httpd/web/multi_wan_generic.asp /home/httpd/web/admin/multi_wan_generic.asp
	$(ROMFSINST) -s /home/httpd/web/deo_multi_wan_generic.asp /home/httpd/web/admin/deo_multi_wan_generic.asp

	$(ROMFSINST) -s /home/httpd/web/access_limit.asp  /home/httpd/web/admin/access_limit.asp
	$(ROMFSINST) -s /home/httpd/web/natsession.asp  /home/httpd/web/admin/natsession.asp
	$(ROMFSINST) -s /home/httpd/web/useage.asp  /home/httpd/web/admin/useage.asp
	$(ROMFSINST) -s /home/httpd/web/autoreboot.asp  /home/httpd/web/admin/autoreboot.asp
	$(ROMFSINST) -s /home/httpd/web/selfcheck.asp  /home/httpd/web/admin/selfcheck.asp
	$(ROMFSINST) -s /home/httpd/web/ping_result.asp  /home/httpd/web/admin/ping_result.asp
	$(ROMFSINST) -s /home/httpd/web/netstat.asp  /home/httpd/web/admin/netstat.asp
	$(ROMFSINST) -s /home/httpd/web/netstat_result.asp  /home/httpd/web/admin/netstat_result.asp
	$(ROMFSINST) -s /home/httpd/web/grouptable_result.asp  /home/httpd/web/admin/grouptable_result.asp
	$(ROMFSINST) -s /home/httpd/web/client_info.asp  /home/httpd/web/admin/client_info.asp
	$(ROMFSINST) -s /home/httpd/web/invalid_page.asp  /home/httpd/web/admin/invalid_page.asp
else
	$(ROMFSINST) ../web  /home/httpd/web
	$(ROMFSINST) ../web/graphics /home/httpd/web/admin/graphics
	$(ROMFSINST) ../web/menu-images /home/httpd/web/admin/menu-images
	# Remove hidden files, ex. .svn, .git ...
	find $(ROMFSDIR)/home/httpd/web -name ".*" | xargs rm -rf
	rm -f $(ROMFSDIR)/home/httpd/web/*.bak
	rm -f $(ROMFSDIR)/home/httpd/web/admin/*.bak
	$(ROMFSINST) -s /home/httpd/web/mtmcode.js  /home/httpd/web/admin/mtmcode.js
	$(ROMFSINST) -s /home/httpd/web/share.js  /home/httpd/web/admin/share.js
	$(ROMFSINST) -s /home/httpd/web/common.js  /home/httpd/web/admin/common.js
	$(ROMFSINST) -s /home/httpd/web/base64_code.js /home/httpd/web/admin/base64_code.js
	$(ROMFSINST) -s /home/httpd/web/sha256_code.js /home/httpd/web/admin/sha256_code.js
ifdef CONFIG_00R0
	$(ROMFSINST) -s /home/httpd/web/saveconf.asp  /home/httpd/web/admin/saveconf.asp
	$(ROMFSINST) -s /home/httpd/web/code.asp  /home/httpd/web/admin/code.asp
endif
ifdef CONFIG_CMCC_ENTERPRISE
	$(ROMFSINST) -s /home/httpd/web/tr069-stun.asp  /home/httpd/web/admin/tr069-stun.asp
endif
	$(ROMFSINST) -s /home/httpd/web/common_style.css /home/httpd/web/admin/common_style.css
	$(ROMFSINST) -s /home/httpd/web/common.css /home/httpd/web/admin/common.css
	$(ROMFSINST) -s /home/httpd/web/acl.asp  /home/httpd/web/admin/acl.asp
ifndef CONFIG_00R0
	$(ROMFSINST) -s /home/httpd/web/tz.asp  /home/httpd/web/admin/tz.asp
endif
	$(ROMFSINST) -s /home/httpd/web/url_blocking.asp  /home/httpd/web/admin/url_blocking.asp
	$(ROMFSINST) -s /home/httpd/web/date.asp  /home/httpd/web/admin/date.asp
	$(ROMFSINST) -s /home/httpd/web/fw-macfilter.asp  /home/httpd/web/admin/fw-macfilter.asp
	$(ROMFSINST) -s /home/httpd/web/fw-macfilter_gw.asp  /home/httpd/web/admin/fw-macfilter_gw.asp
	$(ROMFSINST) -s /home/httpd/web/fw-macfilter_rg.asp  /home/httpd/web/admin/fw-macfilter_rg.asp
	$(ROMFSINST) -s /home/httpd/web/reboot.asp  /home/httpd/web/admin/reboot.asp
	$(ROMFSINST) -s /home/httpd/web/multi_lang.asp  /home/httpd/web/admin/multi_lang.asp
	$(ROMFSINST) -s /home/httpd/web/saveconf.asp  /home/httpd/web/admin/saveconf.asp
	$(ROMFSINST) -s /home/httpd/web/favicon.ico  /home/httpd/web/admin/favicon.ico
#	$(ROMFSINST) -s /home/httpd/web/syslog.asp  /home/httpd/web/admin/syslog.asp
	$(ROMFSINST) -s /home/httpd/web/holepunching.asp  /home/httpd/web/admin/holepunching.asp
	$(ROMFSINST) -s /home/httpd/web/ldap.asp  /home/httpd/web/admin/ldap.asp
	$(ROMFSINST) -s /home/httpd/web/lanApConnection.asp  /home/httpd/web/admin/lanApConnection.asp
	$(ROMFSINST) -s /home/httpd/web/wanApConnection.asp  /home/httpd/web/admin/wanApConnection.asp

ifdef CONFIG_00R0
	$(ROMFSINST) -s /home/httpd/web/upgrade.asp  /home/httpd/web/admin/upgrade.asp
	$(ROMFSINST) -s /home/httpd/web/dhcpd.asp /home/httpd/web/admin/dhcpd.asp
	$(ROMFSINST) -s /home/httpd/web/tcpiplan.asp /home/httpd/web/admin/tcpiplan.asp
	$(ROMFSINST) -s /home/httpd/web/dhcptbl.asp /home/httpd/web/admin/dhcptbl.asp
	$(ROMFSINST) -s /home/httpd/web/macIptbl.asp /home/httpd/web/admin/macIptbl.asp
	$(ROMFSINST) -s /home/httpd/web/portBaseFilterDhcp.asp /home/httpd/web/admin/portBaseFilterDhcp.asp
	$(ROMFSINST) -s /home/httpd/web/fdbtbl.asp /home/httpd/web/admin/fdbtbl.asp
	$(ROMFSINST) -s /home/httpd/web/dms.asp /home/httpd/web/admin/dms.asp
	$(ROMFSINST) -s /home/httpd/web/samba.asp /home/httpd/web/admin/samba.asp
endif
	$(ROMFSINST) -s /home/httpd/web/status.asp  /home/httpd/web/admin/status.asp
ifdef CONFIG_00R0
	$(ROMFSINST) -s /home/httpd/web/ddns.asp  /home/httpd/web/admin/ddns.asp
	$(ROMFSINST) -s /home/httpd/web/algonoff.asp  /home/httpd/web/admin/algonoff.asp
	$(ROMFSINST) -s /home/httpd/web/fw-ipportfilter_rg.asp  /home/httpd/web/admin/fw-ipportfilter_rg.asp
	$(ROMFSINST) -s /home/httpd/web/fw-portfw.asp  /home/httpd/web/admin/fw-portfw.asp
	$(ROMFSINST) -s /home/httpd/web/static-mapping.asp /home/httpd/web/admin/static-mapping.asp
	$(ROMFSINST) -s /home/httpd/web/domainblk.asp  /home/httpd/web/admin/domainblk.asp
	$(ROMFSINST) -s /home/httpd/web/fw-dmz.asp  /home/httpd/web/admin/fw-dmz.asp
	$(ROMFSINST) -s /home/httpd/web/pass_through.asp /home/httpd/web/admin/pass_through.asp
	$(ROMFSINST) -s /home/httpd/web/upnp.asp  /home/httpd/web/admin/upnp.asp
	$(ROMFSINST) -s /home/httpd/web/rip.asp  /home/httpd/web/admin/rip.asp
	$(ROMFSINST) -s /home/httpd/web/arptable.asp  /home/httpd/web/admin/arptable.asp
	$(ROMFSINST) -s /home/httpd/web/bridging.asp  /home/httpd/web/admin/bridging.asp
	$(ROMFSINST) -s /home/httpd/web/routing.asp  /home/httpd/web/admin/routing.asp
	$(ROMFSINST) -s /home/httpd/web/fdbtbl.asp  /home/httpd/web/admin/fdbtbl.asp
	$(ROMFSINST) -s /home/httpd/web/bridge_grouping.asp  /home/httpd/web/admin/bridge_grouping.asp
	$(ROMFSINST) -s /home/httpd/web/interface_grouping.asp  /home/httpd/web/admin/interface_grouping.asp
	$(ROMFSINST) -s /home/httpd/web/vlan_mapping.asp  /home/httpd/web/admin/vlan_mapping.asp
	$(ROMFSINST) -s /home/httpd/web/rmtacc.asp  /home/httpd/web/admin/rmtacc.asp
	$(ROMFSINST) -s /home/httpd/web/ipv6_enabledisable.asp  /home/httpd/web/admin/ipv6_enabledisable.asp
	$(ROMFSINST) -s /home/httpd/web/routetbl_ipv6.asp  /home/httpd/web/admin/routetbl_ipv6.asp
	$(ROMFSINST) -s /home/httpd/web/routetbl.asp  /home/httpd/web/admin/routetbl.asp
	$(ROMFSINST) -s /home/httpd/web/radvdconf.asp  /home/httpd/web/admin/radvdconf.asp
	$(ROMFSINST) -s /home/httpd/web/dhcpdv6.asp  /home/httpd/web/admin/dhcpdv6.asp
ifdef CONFIG_RTK_DEV_AP
	$(ROMFSINST) -s /home/httpd/web/app_mld_proxy.asp  /home/httpd/web/admin/app_mld_proxy.asp
endif
	$(ROMFSINST) -s /home/httpd/web/app_mldProxy.asp  /home/httpd/web/admin/app_mldProxy.asp
	$(ROMFSINST) -s /home/httpd/web/app_mld_snooping.asp  /home/httpd/web/admin/app_mld_snooping.asp
	$(ROMFSINST) -s /home/httpd/web/routing_ipv6.asp  /home/httpd/web/admin/routing_ipv6.asp
	$(ROMFSINST) -s /home/httpd/web/fw-ipportfilter-v6_IfId_rg.asp  /home/httpd/web/admin/fw-ipportfilter-v6_IfId_rg.asp
	$(ROMFSINST) -s /home/httpd/web/fw-ipportfilter-v6_IfId.asp  /home/httpd/web/admin/fw-ipportfilter-v6_IfId.asp
	$(ROMFSINST) -s /home/httpd/web/ping.asp  /home/httpd/web/admin/ping.asp
	$(ROMFSINST) -s /home/httpd/web/grouptable.asp  /home/httpd/web/admin/grouptable.asp
	$(ROMFSINST) -s /home/httpd/web/ssh.asp  /home/httpd/web/admin/ssh.asp
	$(ROMFSINST) -s /home/httpd/web/netstat.asp  /home/httpd/web/admin/netstat.asp
	$(ROMFSINST) -s /home/httpd/web/portmirror.asp  /home/httpd/web/admin/portmirror.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_0_1.asp  /home/httpd/web/admin/wizard_screen_0_1.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_1_1.asp  /home/httpd/web/admin/wizard_screen_1_1.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_1_2.asp    /home/httpd/web/admin/wizard_screen_1_2.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_4_1_1.asp  /home/httpd/web/admin/wizard_screen_4_1_1.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_4_1_2.asp  /home/httpd/web/admin/wizard_screen_4_1_2.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_4_1.asp    /home/httpd/web/admin/wizard_screen_4_1.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_4_2.asp    /home/httpd/web/admin/wizard_screen_4_2.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_5_1.asp    /home/httpd/web/admin/wizard_screen_5_1.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_5_2.asp    /home/httpd/web/admin/wizard_screen_5_2.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_index.asp  /home/httpd/web/admin/wizard_screen_index.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_menu.asp   /home/httpd/web/admin/wizard_screen_menu.asp
	$(ROMFSINST) -s /home/httpd/web/trouble_wizard_index.asp  /home/httpd/web/admin/trouble_wizard_index.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_0_1_2019.asp    /home/httpd/web/admin/wizard_screen_0_1_2019.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_1_1_2019.asp    /home/httpd/web/admin/wizard_screen_1_1_2019.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_1_2019.asp    /home/httpd/web/admin/wizard_screen_1_2019.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_2_1_2019.asp    /home/httpd/web/admin/wizard_screen_2_1_2019.asp
	$(ROMFSINST) -s /home/httpd/web/wizard_screen_2_2_2019.asp    /home/httpd/web/admin/wizard_screen_2_2_2019.asp
	$(ROMFSINST) -s /home/httpd/web/status_pon.asp  /home/httpd/web/admin/status_pon.asp
	$(ROMFSINST) -s /home/httpd/web/aWiFi_setting.asp  /home/httpd/web/admin/aWiFi_setting.asp
	$(ROMFSINST) -s /home/httpd/web/aWiFi_macList.asp  /home/httpd/web/admin/aWiFi_macList.asp
	$(ROMFSINST) -s /home/httpd/web/aWiFi_urlList.asp  /home/httpd/web/admin/aWiFi_urlList.asp
	$(ROMFSINST) -s /home/httpd/web/aWiFi_audit.asp  /home/httpd/web/admin/aWiFi_audit.asp
endif
	$(ROMFSINST) -s /home/httpd/web/wanadsl.asp  /home/httpd/web/admin/wanadsl.asp
	$(ROMFSINST) -s /home/httpd/web/wanatm.asp  /home/httpd/web/admin/wanatm.asp
	$(ROMFSINST) -s /home/httpd/web/waneth.asp  /home/httpd/web/admin/waneth.asp
	$(ROMFSINST) -e CONFIG_IPV6 -s /home/httpd/web/status_ipv6.asp  /home/httpd/web/admin/status_ipv6.asp
	$(ROMFSINST) -s /home/httpd/web/title.html  /home/httpd/web/admin/title.html
	$(ROMFSINST) -s /home/httpd/web/upgrade.asp  /home/httpd/web/admin/upgrade.asp
	$(ROMFSINST) -s /home/httpd/web/upgrade2.asp  /home/httpd/web/admin/upgrade2.asp
	$(ROMFSINST) -s /home/httpd/web/upgrade3.asp  /home/httpd/web/admin/upgrade3.asp
ifdef CONFIG_00R0
	$(ROMFSINST) -s /home/httpd/web/nprogress.js  /home/httpd/web/admin/nprogress.js
	$(ROMFSINST) -s /home/httpd/web/style/nprogress.css  /home/httpd/web/admin/nprogress.css
	$(ROMFSINST) -s /home/httpd/web/reset.css  /home/httpd/web/admin/reset.css
	$(ROMFSINST) -s /home/httpd/web/sk_reset.css  /home/httpd/web/admin/sk_reset.css
	$(ROMFSINST) -s /home/httpd/web/base.css  /home/httpd/web/admin/base.css
	$(ROMFSINST) -s /home/httpd/web/style.css  /home/httpd/web/admin/style.css
endif
	$(ROMFSINST) -s /home/httpd/web/wlbasic.asp  /home/httpd/web/admin/wlbasic.asp
	$(ROMFSINST) -s /home/httpd/web/wladvanced.asp  /home/httpd/web/admin/wladvanced.asp
	$(ROMFSINST) -s /home/httpd/web/wlwpa.asp  /home/httpd/web/admin/wlwpa.asp
	$(ROMFSINST) -s /home/httpd/web/wlactrl.asp  /home/httpd/web/admin/wlactrl.asp
	$(ROMFSINST) -e CONFIG_WIFI_SIMPLE_CONFIG -s /home/httpd/web/wlwps.asp  /home/httpd/web/admin/wlwps.asp
	$(ROMFSINST) -s /home/httpd/web/wlstatbl.asp  /home/httpd/web/admin/wlstatbl.asp
	$(ROMFSINST) -s /home/httpd/web/wlft.asp  /home/httpd/web/admin/wlft.asp
	$(ROMFSINST) -e CONFIG_USER_WIRELESS_WDS -s /home/httpd/web/wlwds.asp  /home/httpd/web/admin/wlwds.asp
	$(ROMFSINST) -s /home/httpd/web/wlsurvey.asp  /home/httpd/web/admin/wlsurvey.asp
	$(ROMFSINST) -s /home/httpd/web/wlwpa_mbssid.asp  /home/httpd/web/admin/wlwpa_mbssid.asp
	$(ROMFSINST) -s /home/httpd/web/wlstatus.asp  /home/httpd/web/admin/wlstatus.asp
ifdef CONFIG_RTIC
	$(ROMFSINST) -s /home/httpd/web/wlrtic.asp  /home/httpd/web/admin/wlrtic.asp
	$(ROMFSINST) -s /home/httpd/web/wlrtic_suggest_chan_url.asp  /home/httpd/web/admin/wlrtic_suggest_chan_url.asp
endif
	$(ROMFSINST) -s /home/httpd/web/wlbandmode.asp  /home/httpd/web/admin/wlbandmode.asp
	$(ROMFSINST) -s /home/httpd/web/wlmultipleap.asp  /home/httpd/web/admin/wlmultipleap.asp
	$(ROMFSINST) -s /home/httpd/web/wlfon.asp  /home/httpd/web/admin/wlfon.asp
	$(ROMFSINST) -s /home/httpd/web/wlstatbl_vap.asp  /home/httpd/web/admin/wlstatbl_vap.asp
	$(ROMFSINST) -s /home/httpd/web/graphics/del.gif  /home/httpd/web/admin/graphics/del.gif
	$(ROMFSINST) -s /home/httpd/web/graphics/edit.gif  /home/httpd/web/admin/graphics/edit.gif
	$(ROMFSINST) -s /home/httpd/web/graphics/topbar.gif  /home/httpd/web/admin/graphics/topbar.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_bar.gif  /home/httpd/web/admin/menu-images/menu_bar.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_corner.gif  /home/httpd/web/admin/menu-images/menu_corner.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_corner_minus.gif  /home/httpd/web/admin/menu-images/menu_corner_minus.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_corner_plus.gif  /home/httpd/web/admin/menu-images/menu_corner_plus.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_folder_closed.gif  /home/httpd/web/admin/menu-images/menu_folder_closed.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_folder_open.gif  /home/httpd/web/admin/menu-images/menu_folder_open.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_link_default.gif  /home/httpd/web/admin/menu-images/menu_link_default.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_pixel.gif  /home/httpd/web/admin/menu-images/menu_pixel.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_root.gif  /home/httpd/web/admin/menu-images/menu_root.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_tee.gif  /home/httpd/web/admin/menu-images/menu_tee.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_tee_minus.gif  /home/httpd/web/admin/menu-images/menu_tee_minus.gif
	$(ROMFSINST) -s /home/httpd/web/menu-images/menu_tee_plus.gif  /home/httpd/web/admin/menu-images/menu_tee_plus.gif
	$(ROMFSINST) -s /home/httpd/web/wan3gconf.asp  /home/httpd/web/admin/wan3gconf.asp
	$(ROMFSINST) -s /home/httpd/web/multi_wanptm.asp /home/httpd/web/admin/multi_wanptm.asp
	$(ROMFSINST) -s /home/httpd/web/multi_wan_generic.asp /home/httpd/web/admin/multi_wan_generic.asp
	$(ROMFSINST) -s /home/httpd/web/deo_multi_wan_generic.asp /home/httpd/web/admin/deo_multi_wan_generic.asp

	$(ROMFSINST) -s /home/httpd/web/access_limit.asp  /home/httpd/web/admin/access_limit.asp
	$(ROMFSINST) -s /home/httpd/web/natsession.asp  /home/httpd/web/admin/natsession.asp
	$(ROMFSINST) -s /home/httpd/web/useage.asp  /home/httpd/web/admin/useage.asp
	$(ROMFSINST) -s /home/httpd/web/autoreboot.asp  /home/httpd/web/admin/autoreboot.asp
	$(ROMFSINST) -s /home/httpd/web/selfcheck.asp  /home/httpd/web/admin/selfcheck.asp
	$(ROMFSINST) -s /home/httpd/web/ping_result.asp  /home/httpd/web/admin/ping_result.asp
	$(ROMFSINST) -s /home/httpd/web/netstat.asp  /home/httpd/web/admin/netstat.asp
	$(ROMFSINST) -s /home/httpd/web/netstat_result.asp  /home/httpd/web/admin/netstat_result.asp
	$(ROMFSINST) -s /home/httpd/web/grouptable_result.asp  /home/httpd/web/admin/grouptable_result.asp
	$(ROMFSINST) -s /home/httpd/web/ssh_result.asp  /home/httpd/web/admin/ssh_result.asp
	$(ROMFSINST) -s /home/httpd/web/client_info.asp  /home/httpd/web/admin/client_info.asp
	$(ROMFSINST) -s /home/httpd/web/invalid_page.asp  /home/httpd/web/admin/invalid_page.asp
ifdef CONFIG_TELMEX_DEV
	$(ROMFSINST) -s /home/httpd/web/multi_wan_simple.asp /home/httpd/web/admin/multi_wan_simple.asp
endif
ifdef CONFIG_TLKM
	$(ROMFSINST) -s /home/httpd/web/macLimit.asp /home/httpd/web/admin/macLimit.asp
endif
ifneq (, $(filter y, $(CONFIG_TRUE) $(CONFIG_TLKM)))
	$(ROMFSINST) -s /home/httpd/web/nprogress.js  /home/httpd/web/admin/nprogress.js
	$(ROMFSINST) -s /home/httpd/web/style/nprogress.css  /home/httpd/web/admin/nprogress.css
	$(ROMFSINST) -s /home/httpd/web/gpon.asp  /home/httpd/web/admin/gpon.asp
#	$(ROMFSINST) -s /home/httpd/web/syslog.asp  /home/httpd/web/admin/syslog.asp
	$(ROMFSINST) -s /home/httpd/web/holepunching.asp  /home/httpd/web/admin/holepunching.asp
	$(ROMFSINST) -s /home/httpd/web/ldap.asp  /home/httpd/web/admin/ldap.asp
	$(ROMFSINST) -s /home/httpd/web/lanApConnection.asp  /home/httpd/web/admin/lanApConnection.asp
	$(ROMFSINST) -s /home/httpd/web/wanApConnection.asp  /home/httpd/web/admin/wanApConnection.asp
	$(ROMFSINST) -s /home/httpd/web/app_iptv.asp  /home/httpd/web/admin/app_iptv.asp
	$(ROMFSINST) -s /home/httpd/web/omci_info.asp  /home/httpd/web/admin/omci_info.asp
	$(ROMFSINST) -s /home/httpd/web/dos.asp  /home/httpd/web/admin/dos.asp
	$(ROMFSINST) -s /home/httpd/web/upgrade.asp  /home/httpd/web/admin/upgrade.asp
	$(ROMFSINST) -s /home/httpd/web/algonoff.asp  /home/httpd/web/admin/algonoff.asp
	$(ROMFSINST) -s /home/httpd/web/fw-ipportfilter.asp  /home/httpd/web/admin/fw-ipportfilter.asp
	$(ROMFSINST) -s /home/httpd/web/fw-portfw.asp  /home/httpd/web/admin/fw-portfw.asp
	$(ROMFSINST) -s /home/httpd/web/static-mapping.asp /home/httpd/web/admin/static-mapping.asp
	$(ROMFSINST) -s /home/httpd/web/url_blocking.asp  /home/httpd/web/admin/url_blocking.asp
	$(ROMFSINST) -s /home/httpd/web/domainblk.asp  /home/httpd/web/admin/domainblk.asp
	$(ROMFSINST) -s /home/httpd/web/fw-dmz.asp  /home/httpd/web/admin/fw-dmz.asp
	$(ROMFSINST) -s /home/httpd/web/fw-enable.asp  /home/httpd/web/admin/fw-enable.asp
	$(ROMFSINST) -s /home/httpd/web/remote_management.asp  /home/httpd/web/admin/remote_management.asp
	$(ROMFSINST) -s /home/httpd/web/rmtacc.asp  /home/httpd/web/admin/rmtacc.asp
	$(ROMFSINST) -s /home/httpd/web/tcpiplan.asp  /home/httpd/web/admin/tcpiplan.asp
	$(ROMFSINST) -s /home/httpd/web/net_vlan_mapping.asp  /home/httpd/web/admin/net_vlan_mapping.asp
	$(ROMFSINST) -s /home/httpd/web/stats.asp /home/httpd/web/admin/stats.asp
	$(ROMFSINST) -s /home/httpd/web/dhcpd.asp /home/httpd/web/admin/dhcpd.asp
	$(ROMFSINST) -s /home/httpd/web/ddns.asp  /home/httpd/web/admin/ddns.asp
	$(ROMFSINST) -s /home/httpd/web/igmproxy.asp  /home/httpd/web/admin/igmproxy.asp
	$(ROMFSINST) -s /home/httpd/web/upnp.asp  /home/httpd/web/admin/upnp.asp
	$(ROMFSINST) -s /home/httpd/web/rip.asp  /home/httpd/web/admin/rip.asp
	$(ROMFSINST) -s /home/httpd/web/samba.asp  /home/httpd/web/admin/samba.asp
	$(ROMFSINST) -s /home/httpd/web/fw-ipportfilter_rg.asp  /home/httpd/web/admin/fw-ipportfilter_rg.asp
	$(ROMFSINST) -s /home/httpd/web/fw-macfilter_rg.asp  /home/httpd/web/admin/fw-macfilter_rg.asp
	$(ROMFSINST) -s /home/httpd/web/fw-portfw.asp  /home/httpd/web/admin/fw-portfw.asp
	$(ROMFSINST) -s /home/httpd/web/static-mapping.asp /home/httpd/web/admin/static-mapping.asp
	$(ROMFSINST) -s /home/httpd/web/domainblk.asp  /home/httpd/web/admin/domainblk.asp
	$(ROMFSINST) -s /home/httpd/web/arptable.asp  /home/httpd/web/admin/arptable.asp
	$(ROMFSINST) -s /home/httpd/web/bridging.asp  /home/httpd/web/admin/bridging.asp
	$(ROMFSINST) -s /home/httpd/web/routing.asp  /home/httpd/web/admin/routing.asp
	$(ROMFSINST) -s /home/httpd/web/net_qos_imq_policy.asp  /home/httpd/web/admin/net_qos_imq_policy.asp
	$(ROMFSINST) -s /home/httpd/web/net_qos_cls.asp  /home/httpd/web/admin/net_qos_cls.asp
	$(ROMFSINST) -s /home/httpd/web/net_qos_traffictl.asp  /home/httpd/web/admin/net_qos_traffictl.asp
	$(ROMFSINST) -s /home/httpd/web/ipv6_enabledisable.asp  /home/httpd/web/admin/ipv6_enabledisable.asp
	$(ROMFSINST) -s /home/httpd/web/radvdconf.asp  /home/httpd/web/admin/radvdconf.asp
	$(ROMFSINST) -s /home/httpd/web/dhcpdv6.asp  /home/httpd/web/admin/dhcpdv6.asp
	$(ROMFSINST) -s /home/httpd/web/app_mldProxy.asp  /home/httpd/web/admin/app_mldProxy.asp
	$(ROMFSINST) -s /home/httpd/web/app_mld_snooping.asp  /home/httpd/web/admin/app_mld_snooping.asp
	$(ROMFSINST) -s /home/httpd/web/routing_ipv6.asp  /home/httpd/web/admin/routing_ipv6.asp
	$(ROMFSINST) -s /home/httpd/web/fw-ipportfilter-v6_IfId_rg.asp  /home/httpd/web/admin/fw-ipportfilter-v6_IfId_rg.asp
	$(ROMFSINST) -s /home/httpd/web/aclv6.asp  /home/httpd/web/admin/aclv6.asp
	$(ROMFSINST) -s /home/httpd/web/dhcptblv6.asp  /home/httpd/web/admin/dhcptblv6.asp
	$(ROMFSINST) -s /home/httpd/web/net_qos_cls_edit.asp  /home/httpd/web/admin/net_qos_cls_edit.asp
	$(ROMFSINST) -s /home/httpd/web/net_qos_traffictl_edit.asp  /home/httpd/web/admin/net_qos_traffictl_edit.asp
	$(ROMFSINST) -s /home/httpd/web/routetbl_ipv6.asp  /home/httpd/web/admin/routetbl_ipv6.asp
	$(ROMFSINST) -s /home/httpd/web/fdbtbl.asp  /home/httpd/web/admin/fdbtbl.asp
	$(ROMFSINST) -s /home/httpd/web/routetbl.asp  /home/httpd/web/admin/routetbl.asp
	$(ROMFSINST) -s /home/httpd/web/macIptbl.asp /home/httpd/web/admin/macIptbl.asp
	$(ROMFSINST) -s /home/httpd/web/dhcptbl.asp /home/httpd/web/admin/dhcptbl.asp
	$(ROMFSINST) -s /home/httpd/web/portBaseFilterDhcp.asp /home/httpd/web/admin/portBaseFilterDhcp.asp
	$(ROMFSINST) -s /home/httpd/web/macIptbl.asp /home/httpd/web/admin/macIptbl.asp
	$(ROMFSINST) -s /home/httpd/web/catv.asp /home/httpd/web/admin/catv.asp
	$(ROMFSINST) -s /home/httpd/web/status_pon.asp /home/httpd/web/admin/status_pon.asp
	$(ROMFSINST) -s /home/httpd/web/status_usb.asp /home/httpd/web/admin/status_usb.asp
endif
ifeq ($(CONFIG_GENERAL_WEB),y)
ifeq ($(CONFIG_USER_MAP_E),y)
	$(ROMFSINST) -s /home/httpd/web/mape_fmr_lists.asp /home/httpd/web/admin/mape_fmr_lists.asp
endif
endif
ifneq (, $(filter y, $(CONFIG_PHIL) $(CONFIG_BRAZ) $(CONFIG_TRUE)))
	$(ROMFSINST) -s /home/httpd/web/multi_wan_generic_true.asp /home/httpd/web/admin/multi_wan_generic_true.asp
	$(ROMFSINST) -s /home/httpd/web/deo_multi_wan_generic_true.asp /home/httpd/web/admin/deo_multi_wan_generic_true.asp
endif
ifdef CONFIG_USER_RTK_VOIP
	$(ROMFSINST) -s /home/httpd/web/voip_sip_status_new_web.asp  /home/httpd/web/admin/voip_sip_status_new_web.asp
endif
ifdef CONFIG_CMCC_ENTERPRISE
	$(ROMFSINST) -s /home/httpd/web/devinfo.asp  /home/httpd/web/admin/devinfo.asp
	$(ROMFSINST) -s /home/httpd/web/wanstatus.asp  /home/httpd/web/admin/wanstatus.asp
	$(ROMFSINST) -s /home/httpd/web/laninfo.asp  /home/httpd/web/admin/laninfo.asp
	$(ROMFSINST) -s /home/httpd/web/voipinfo.asp  /home/httpd/web/admin/voipinfo.asp
	$(ROMFSINST) -s /home/httpd/web/quickwifi.asp  /home/httpd/web/admin/quickwifi.asp
	$(ROMFSINST) -s /home/httpd/web/dhcp.asp  /home/httpd/web/admin/dhcp.asp
	$(ROMFSINST) -s /home/httpd/web/wifi.asp  /home/httpd/web/admin/wifi.asp
	$(ROMFSINST) -s /home/httpd/web/wifi5g.asp  /home/httpd/web/admin/wifi5g.asp
	$(ROMFSINST) -s /home/httpd/web/ip_qos.asp  /home/httpd/web/admin/ip_qos.asp
	$(ROMFSINST) -s /home/httpd/web/vpn.asp  /home/httpd/web/admin/vpn.asp
	$(ROMFSINST) -s /home/httpd/web/url_filter.asp  /home/httpd/web/admin/url_filter.asp
	$(ROMFSINST) -s /home/httpd/web/firewall.asp  /home/httpd/web/admin/firewall.asp
	$(ROMFSINST) -s /home/httpd/web/mac_filter.asp  /home/httpd/web/admin/mac_filter.asp
	$(ROMFSINST) -s /home/httpd/web/ip_filter.asp  /home/httpd/web/admin/ip_filter.asp
	$(ROMFSINST) -s /home/httpd/web/nat.asp  /home/httpd/web/admin/nat.asp
	$(ROMFSINST) -s /home/httpd/web/ddns.asp  /home/httpd/web/admin/ddns.asp
	$(ROMFSINST) -s /home/httpd/web/upnp.asp  /home/httpd/web/admin/upnp.asp
	$(ROMFSINST) -s /home/httpd/web/igmp.asp  /home/httpd/web/admin/igmp.asp
	$(ROMFSINST) -s /home/httpd/web/mld.asp  /home/httpd/web/admin/mld.asp
	$(ROMFSINST) -s /home/httpd/web/user.asp  /home/httpd/web/admin/user.asp
	$(ROMFSINST) -s /home/httpd/web/device.asp  /home/httpd/web/admin/device.asp
	$(ROMFSINST) -s /home/httpd/web/ntp.asp  /home/httpd/web/admin/ntp.asp
	$(ROMFSINST) -s /home/httpd/web/language.asp  /home/httpd/web/admin/language.asp
	$(ROMFSINST) -s /home/httpd/web/diag.asp  /home/httpd/web/admin/diag.asp
	$(ROMFSINST) -s /home/httpd/web/help_status.asp  /home/httpd/web/admin/help_status.asp
	$(ROMFSINST) -s /home/httpd/web/help_network.asp  /home/httpd/web/admin/help_network.asp
	$(ROMFSINST) -s /home/httpd/web/help_security.asp  /home/httpd/web/admin/help_security.asp
	$(ROMFSINST) -s /home/httpd/web/help_admin.asp  /home/httpd/web/admin/help_admin.asp
	$(ROMFSINST) -s /home/httpd/web/pluginmanager.asp  /home/httpd/web/admin/pluginmanager.asp
	$(ROMFSINST) -s /home/httpd/web/logout.asp  /home/httpd/web/admin/logout.asp
	$(ROMFSINST) -s /home/httpd/web/logout.html  /home/httpd/web/admin/logout.html
#	$(ROMFSINST) -s /home/httpd/web/admin/login.asp  /home/httpd/web/login.asp
endif
endif	# of CONFIG_PRD_SIGNATURE=9601DVD
endif	# of CONFIG_USER_BOA_BOA

ifdef CONFIG_IPOE_ARPING_SUPPORT
	$(ROMFSINST) /bin/arping_ipoe
endif
	$(ROMFSINST) /bin/startup
ifdef CONFIG_CMCC_BEIJING_GROUP_VERIFICATION
	$(ROMFSINST) /bin/counterRec
endif
	$(ROMFSINST) /bin/configd
ifdef CONFIG_E8B
	$(ROMFSINST) -p 744 /etc/scripts/check_province.sh
endif
#	$(ROMFSINST) -e CONFIG_USER_MENU_CLI /bin/cli
##added by xl_yue for supporting menu_cli and command_cli
	$(ROMFSINST) -e CONFIG_USER_CLI /bin/cli
ifneq ($(CONFIG_USER_XMLCONFIG),y)
	$(ROMFSINST) /bin/flash
	$(ROMFSINST) /bin/saveconfig
	$(ROMFSINST) /bin/loadconfig
	$(ROMFSINST) -e CONFIG_RTL8192CD /bin/hsinfo
	$(ROMFSINST) -s /bin/flash /etc/scripts/flash
endif
	$(ROMFSINST) /bin/mib
	$(ROMFSINST) /lib/libmib.so

ifdef CONFIG_USE_RSDK_WRAPPER
	$(ROMFSINST) -s libmib.so /lib/libmib.so.0
else #CONFIG_USE_RSDK_WRAPPER
	$(ROMFSINST) -s /lib/libmib.so /lib/libmib.so.0
endif #CONFIG_USE_RSDK_WRAPPER

ifeq ($(CONFIG_APOLLO_MP_TEST),y)
	$(ROMFSINST) /bin/show
	$(ROMFSINST) /bin/led
	$(ROMFSINST) /bin/ledtest
endif
ifdef CONFIG_USER_SUPPORT_EXTERNAL_SWITCH
	$(ROMFSINST) /bin/syncPhyPortMapping
endif
	$(ROMFSINST) /bin/consoleset
	$(ROMFSINST) /bin/arpsend
ifdef CONFIG_USER_RTK_CONVERT_XML
	$(ROMFSINST) /bin/convertxml
endif
	$(ROMFSINST) /bin/qc
ifeq ($(CONFIG_E8B),y)
	$(ROMFSINST) /bin/acl_limit_bandwidth
ifeq ($(CONFIG_SUPPORT_AUTO_DIAG),y)
	$(ROMFSINST) /bin/config_test
	$(ROMFSINST) /bin/autoSimu
ifeq ($(CONFIG_USER_LANNETINFO),y)
	$(ROMFSINST) /bin/autoPing
endif
endif
ifeq ($(CONFIG_USER_PPTP_CLIENT_PPTP),y)
ifeq ($(CONFIG_USER_L2TPD_L2TPD),y)
	$(ROMFSINST) /bin/VPNConnection
endif
endif
ifeq ($(CONFIG_USER_PPTP_CLIENT),y)
ifeq ($(CONFIG_USER_XL2TPD),y)
	$(ROMFSINST) /bin/VPNConnection
endif
endif
endif
ifdef CONFIG_CMCC
	$(ROMFSINST) /bin/bundle
#	$(ROMFSINST) /bin/msg_debug
endif
ifdef CONFIG_CU_BASEON_CMCC
	$(ROMFSINST) /bin/bundle
#	$(ROMFSINST) /bin/msg_debug
endif
	$(ROMFSINST) /bin/ShowStatus
ifdef CONFIG_USER_WIRED_8021X
	$(ROMFSINST) /bin/Wired8021x
endif
ifndef CONFIG_RTL_MULTI_PHY_ETH_WAN
ifdef CONFIG_COMMON_RT_API
	$(ROMFSINST) /bin/PortMirror
endif
endif
	$(ROMFSINST) /bin/qc
ifdef CONFIG_00R0
	$(ROMFSINST) /bin/crc_count
endif
	$(ROMFSINST) -e CONFIG_USER_RTK_SYSLOG /bin/slogd
ifeq ($(CONFIG_USER_CLUSTER_MANAGE),y)
	$(ROMFSINST) /bin/cluster_client
	$(ROMFSINST) /bin/cluster_server
endif
ifeq ($(WIFI_TIMER_SCHEDULE), y)
	$(ROMFSINST) /bin/config_wlan
endif
	$(ROMFSINST) -e CONFIG_USER_WIRELESS_TOOLS /bin/wlan_apply
ifeq ($(LED_TIMER), y)
	$(ROMFSINST) /bin/config_led
endif
ifeq ($(SLEEP_TIMER), y)
	$(ROMFSINST) /bin/config_sleepmode
endif
ifeq (y,$(filter $(CONFIG_EPON_FEATURE) $(CONFIG_GPON_FEATURE) $(CONFIG_FIBER_FEATURE),y))
	$(ROMFSINST) /bin/checkpon
endif
	$(ROMFSINST) -e CONFIG_USER_FON /bin/fonservice
ifeq ($(CONFIG_WLAN_SCHEDULE_SUPPORT),y)
	$(ROMFSINST) /bin/wlan_schedule
endif
ifeq ($(CONFIG_USER_TIMELYCHECK),y)
	$(ROMFSINST) /bin/timelycheck
endif

ifeq ($(CONFIG_DEFAULT_CONF_ON_RT_XMLFILE),y)
	$(ROMFSINST) flash_set_rt.sh  /etc/scripts/flash_set_rt
	$(ROMFSINST) $(ROOTDIR)/config/config_default_rt.xml /etc/config_default_rt.xml
else
	if [ -f $(ROMFSDIR)/etc/config_default_rt.xml ];then \
		rm -f $(ROMFSDIR)/etc/config_default_rt.xml;\
	fi;
endif

ifeq ($(CONFIG_USER_RECOVER_XML_CONF),y)
	$(ROMFSINST) recover_xmlfile.sh  /etc/scripts/recover_xmlfile.sh
endif
ifeq ($(CONFIG_LUNA_FWU_SIGNATURE),y)
	$(ROMFSINST) $(ROOTDIR)/tools/security/fwu_cert.pem /etc/fwu_cert.pem
endif

ifdef CONFIG_CU_DPI
	$(ROMFSINST) /bin/informgetd
endif
ifeq ($(CONFIG_CMCC_ENTERPRISE),y)
	$(ROMFSINST) /bin/schedule_time_update
endif
ifeq ($(CONFIG_USER_SUPPORT_XML_FORMAT_TURN),y)
	$(ROMFSINST) xml_encrypt_tool_mips /bin/.get_new_sw_ver
endif

ifeq ($(CONFIG_RTL8277C_SERIES),y)
ifneq ("$(wildcard rtl8277c_config)","")
	$(ROMFSINST) /bin/rtl8277c_config
else
ifeq ($(CROSS_TARGET),aarch64-linux)
	$(ROMFSINST) release/aarch64/rtl8277c_config /bin/rtl8277c_config
endif
endif
endif
	$(ROMFSINST) -s /tmp/captcha.png /home/httpd/web/graphics/captcha.png
	$(ROMFSINST) -s /home/httpd/web/graphics/captcha.png  /home/httpd/web/admin/graphics/captcha.png

clean:
ifdef CONFIG_USE_RSDK_WRAPPER
	rm -rf $(ROOTDIR)/lib/librtk/$(MIB_LIB_SO)
	rm -rf $(ROOTDIR)/lib/librtk/$(MIB_LIB)
endif #CONFIG_USE_RSDK_WRAPPER
ifdef CONFIG_CU_DPI
	rm -f $(INFORMGETDATA_LIB_SO)
	rm -rf $(ROOTDIR)/lib/librtk/$(INFORMGETDATA_LIB_SO)
endif
ifeq ($(CONFIG_USER_SUPPORT_XML_FORMAT_TURN),y)
	rm -f xml_encrypt_tool_mips
endif
ifeq ($(CONFIG_GENERAL_WEB),y)
	rm -rf ../web/adminMenu.js ../web/index.html ../web/index_user.html ../web/admin/userMenu.js ../web/index.asp ../web/index_user.html
endif
	rm -f $(WEB_LIB) $(MIB_LIB) $(MIB_LIB_SO) flash  ledtest\
		webpages.bin startup cli qc \
		saveconfig loadconfig \
		ShowStatus Wired8021x configd slogd *.gdb libflashlib.a consoleset getoui
ifeq ($(CONFIG_CMCC_BEIJING_GROUP_VERIFICATION),y)
	rm -f counterRec
endif
ifdef CONFIG_USER_RTK_CONVERT_XML
	rm -f convertxml
endif
ifeq ($(CONFIG_USER_RTK_VOIP),y)
	rm -f $(FLASH_FILE)
	make -C $(VOIP_APP)/web -f rtl867x.mak clean WEB_DIR=$(ROOTDIR)/romfs/home/httpd/web
endif
ifndef CONFIG_RTL_MULTI_PHY_ETH_WAN
ifeq ($(CONFIG_COMMON_RT_API),y)
	rm -f PortMirror
endif
endif
ifeq ($(CONFIG_USER_CLUSTER_MANAGE),y)
	rm -f cluster_client
	rm -f cluster_server
endif
ifdef CONFIG_USER_FON
	rm -f fonservice
endif
ifeq ($(CONFIG_E8B),y)
ifeq ($(CONFIG_SUPPORT_AUTO_DIAG),y)
	rm -f config_test
	rm -f autoSimu
ifeq ($(CONFIG_USER_LANNETINFO),y)
	rm -f autoPing
endif
endif
endif
ifeq ($(WIFI_TIMER_SCHEDULE), y)
	rm -f config_wlan
endif
ifeq ($(CONFIG_USER_WIRELESS_TOOLS),y)
	rm -f wlan_apply
endif
ifeq ($(LED_TIMER), y)
	rm -f config_led
endif
ifeq ($(SLEEP_TIMER), y)
	rm -f config_sleepmode
endif
ifeq (y,$(filter $(CONFIG_EPON_FEATURE) $(CONFIG_GPON_FEATURE) $(CONFIG_FIBER_FEATURE),y))
	rm -f checkpon
endif
ifeq ($(CONFIG_WLAN_SCHEDULE_SUPPORT),y)
	rm -f wlan_schedule
endif
ifeq ($(CONFIG_USER_TIMELYCHECK),y)
	rm -f timelycheck
endif
ifeq ($(CONFIG_CMCC),y)
	rm -f bundle
#	rm -f msg_debug
endif
ifeq ($(CONFIG_CU_BASEON_CMCC),y)
	rm -f bundle
#	rm -f msg_debug
endif
	make -C $(WEB_SRC_DIR) clean
#ifeq ($(CONFIG_VIR_BOA_JEMBENCHTEST),y)
#	rm -f jspeed.c
#endif
cleanall:
	rm -f *.o

cleanmib:
	rm -f flash.o $(mib_obj_y) mibtbl.o lzss.o $(FLASHDRV).o flash

#
#	Dependencies
#
../asp.o:  ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../balloc.o: ../balloc.c ../uemf.h

../base64.o:  ../base64.c ../webs.h ../wsIntrn.h  ../ej.h ../ejIntrn.h ../uemf.h

../cgi.o:  ../webs.h ../wsIntrn.h ../uemf.h

../default.o:  ../default.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../ejlex.o:  ../ejlex.c ../ej.h ../ejIntrn.h ../uemf.h

../ejparse.o:  ../ejparse.c ../ej.h ../ejIntrn.h ../uemf.h

../emfdb.o:  ../emfdb.h ../wsIntrn.h ../uemf.h

../form.o:  ../form.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../h.o:  ../h.c ../uemf.h

../handler.o:  ../handler.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../md5c.o:  ../md5.h ../wsIntrn.h ../uemf.h

../mime.o:  ../mime.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../misc.o:  ../misc.c ../uemf.h

../page.o:  ../page.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../ringq.o:  ../ringq.c ../uemf.h

../rom.o:  ../rom.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../msgutil.o: ../msgutil.c ../uemf.h

../security.o:  ../security.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../sock.o:  ../sock.c ../uemf.h

../sockGen.o:  ../sockGen.c ../uemf.h

../sym.o:  ../sym.c ../uemf.h

../uemf.o:  ../uemf.c ../uemf.h

../um.o:  ../webs.h ../wsIntrn.h ../um.h ../uemf.h

../umui.o:  ../webs.h ../wsIntrn.h ../um.h ../uemf.h

../url.o:  ../url.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../value.o:  ../value.c ../uemf.h

../webrom.o:  ../webrom.c ../webs.h ../wsIntrn.h ../uemf.h

../webs.o:  ../webs.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../websda.o:  ../webs.h ../wsIntrn.h ../websda.h ../uemf.h

../websuemf.o:  ../websuemf.c ../webs.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

../websSSL.o:  ../websSSL.c ../websSSL.h ../wsIntrn.h ../ej.h ../ejIntrn.h ../uemf.h

$(MIB_DEPEND_FILES): options.h

mibtbl.o: mibtbl.c mib.h

mib.o: mib.c lzss.c mib.h

ifdef CONFIG_USER_XDSL_SLAVE
SUBR_NFBI_API_DEP = $(ROOTDIR)/user/rtl867x_nfbi/ucd/uc_mib.c \
                    $(ROOTDIR)/user/rtl867x_nfbi/ucd/uc_mib.h \
                    $(ROOTDIR)/user/rtl867x_nfbi/ucd/uc_udp.c \
                    $(ROOTDIR)/user/rtl867x_nfbi/ucd/uc_udp.h
subr_nfbi_api.o: subr_nfbi_api.c subr_nfbi_api.h $(SUBR_NFBI_API_DEP)
endif #CONFIG_USER_XDSL_SLAVE

utility.o:utility.c utility.h

cli_utility.o:cli_utility.c cli_utility.h

ifeq ($(CONFIG_USER_CMD_CLI),y)
cfgutility.o:cfgutility.c cfgutility.h
endif

debug.o:debug.c debug.h

ifeq ($(CONFIG_USER_CLUSTER_MANAGE),y)
cluster_common.o:cluster_common.c cluster_common.h
endif

$(FLASHDRV).o:$(FLASHDRV).c rtl_types.h rtl_board.h rtl_flashdrv.h

startup.o:startup.c mibtbl.h
ifeq ($(CONFIG_CMCC_BEIJING_GROUP_VERIFICATION),y)
counterRec.o:counterRec.c utility.h
endif

arping.o:arping.c utility.h

sysconfig.o:sysconfig.c sysconfig.h ../msgq.h mibtbl.h

mib_reserve.o:mib_reserve.c mib_reserve.h mib.h mibtbl.h

ifeq ($(CONFIG_USER_RTK_VOIP),y)
#+++++add by Jack for VoIP project 20/03/07+++++
voip_flash.o: $(VOIP_APP)/flash/voip_flash.c $(VOIP_APP)/includes/voip_flash.h
	$(CC) -c -o $@ $(DEBUG) $(FLASH) $(CFLAGS) $(WEBFLAG) $(IFLAGS) $<

voip_flash_mib.o: $(VOIP_APP)/flash/voip_flash_mib.c $(VOIP_APP)/includes/voip_flash_mib.h
	$(CC) -c -o $@ $(DEBUG) $(FLASH) $(CFLAGS) $(WEBFLAG) $(IFLAGS) $<

voip_flash_tool.o: $(VOIP_APP)/flash/voip_flash_tool.c $(VOIP_APP)/includes/voip_flash_tool.h
	$(CC) -c -o $@ $(DEBUG) $(FLASH) $(CFLAGS) $(WEBFLAG) $(IFLAGS) $<

voip_flash_server.o: $(VOIP_APP)/flash/voip_flash_server.c $(VOIP_APP)/includes/voip_flash.h
	$(CC) -c -o $@ $(DEBUG) $(FLASH) $(CFLAGS) $(WEBFLAG) $(IFLAGS) $<

voip_line_state_share.o: $(VOIP_APP)/flash/voip_line_state_share.c $(VOIP_APP)/includes/voip_flash.h
	$(CC) -c -o $@ $(DEBUG) $(FLASH) $(CFLAGS) $(WEBFLAG) $(IFLAGS) $<
#-----end-----
endif

rt_qcsapi.o: rt_qcsapi.c $(ROOTDIR)/include/linux/autoconf.h $(ROOTDIR)/config/autoconf.h
	$(CC) -c -o $@ $(DEBUG) $(FLASH) $(CFLAGS) $(QCS_CFLAGS) $(WEBFLAG) $(IFLAGS) $<

ifeq ($(YUEME_CU_FEATURE),y)
ftp_server_api.o: $(ROOTDIR)/user/ctcyueme/ftpapi/ftp_server_api.c $(ROOTDIR)/user/ctcyueme/ftpapi/ftp_server_api.h
	$(CC) -c -o $@ $(DEBUG) $(FLASH) $(CFLAGS) $(WEBFLAG) $(IFLAGS) $<
endif

%.o: %.c $(ROOTDIR)/include/linux/autoconf.h $(ROOTDIR)/config/autoconf.h
	$(CC) -c -o $@ $(DEBUG) $(FLASH) $(CFLAGS) $(WEBFLAG) $(IFLAGS) $<

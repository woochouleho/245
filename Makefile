.EXPORT_ALL_VARIABLES:
SRCNAME = src-project
DIRS = $(SRCNAME)
RC_LEVEL_E8B=/etc/init.d/rc8
RC_LEVEL_FLASH_CHK=/etc/init.d/rc18
ifeq ($(CONFIG_GPON_FEATURE),y)
RC_LEVEL_CONFIGD=/etc/init.d/rc3
else
RC_LEVEL_CONFIGD=/etc/init.d/rc18
endif
ifeq ($(CONFIG_RTK_OAM_V1),y)
RC_LEVEL_STARTUP=/etc/init.d/rc32
else
RC_LEVEL_STARTUP=/etc/init.d/rc22
endif
RC_LEVEL_BOA=/etc/init.d/rc34
# boa http server
CONFIG_USER_BOA_BOA=y

#
# to add a new branch, please define new ID, SRCORIGNAME, and WEBNAME, all enclosed by ifeq
#
ID = default_web_id
SRCORIGNAME = src
WEBNAME = web_general
ifdef CONFIG_00R0
WEBNAME = web_rostelecom
endif
ifdef CONFIG_E8B
WEBNAME=./web_ctc
ifeq ($(CONFIG_CU),y)
ifeq ($(CONFIG_CU_BASEON_CMCC),y)
WEBNAME = web_cu
endif
ifeq ($(CONFIG_CU_BASEON_YUEME),y)
WEBNAME = web_cu_dbus
endif
else
ifdef CONFIG_CMCC
WEBNAME=./web_cmcc
endif
# for AP team CMCC UI
ifdef CONFIG_USER_AP_CMCC
WEBNAME =./web_cmcc
endif
ifeq ($(CONFIG_CMCC_ENTERPRISE),y)
WEBNAME =./web_cmcc_enterprise
endif
endif
endif
ifdef CONFIG_TRUE
WEBNAME = web_true
endif
ifdef CONFIG_TELMEX
WEBNAME = web_telmex
endif
ifndef UCLINUX_BUILD_USER
include ../../config/.config
ifdef ROOTDIR
include $(ROOTDIR)/$(LINUXDIR)/.config
include $(ROOTDIR)/backports-5.2.8-1/.config
endif
endif

all romfs:
ifneq ($(ID),$(wildcard $(ID)))
	# remove symbolic links
	rm -rf $(SRCNAME)/web
	rm -rf $(SRCNAME)
	rm -rf src/LINUX/form_src

	# add new symbolic links
	ln -s $(SRCORIGNAME) $(SRCNAME)
	cd $(SRCNAME); ln -s $(WEBNAME) web; cd ..
ifdef CONFIG_E8B
	(cd src/LINUX/; ln -s form_src_e8 form_src)
else ifdef CONFIG_TELMEX
	(cd src/LINUX/; ln -s form_src_telmex form_src)
else ifdef CONFIG_TRUE
	(cd src/LINUX/; ln -s form_src_true form_src)
else
	(cd src/LINUX/; ln -s form_src_generic form_src)
endif

	make -C $(SRCNAME) clean

	# remove old IDs and touch the new ID
	rm -f *_id
	touch $(ID)
endif
	for i in $(DIRS) ; do make -C $$i $@ || exit $?; done
ifdef CONFIG_USER_CONF_ON_XMLFILE
	$(ROMFSINST) $(ROOTDIR)/config/config_default.xml /etc/config_default.xml
	$(ROMFSINST) $(ROOTDIR)/config/config_default_hs.xml /etc/config_default_hs.xml
	if [ -d $(ROOTDIR)/config/province ]; then\
		echo "Copy province data";\
		$(ROMFSINST) $(ROOTDIR)/config/province /etc/province;\
	fi;
endif
ifdef CONFIG_E8B
ifndef CONFIG_USER_XMLCONFIG
	if [ -d $(ROOTDIR)/config/province ]; then\
		echo "Copy province data";\
		$(ROMFSINST) $(ROOTDIR)/config/province /etc/province;\
	fi;
endif
endif

.PHONY: rcX
rcX:
	for i in $(DIRS) ; do make -C $$i $@ ; done
ifdef CONFIG_RTK_SMART_ROAMING
	$(ROMFSINST) -a "mkdir /var/capwap" $(RC_LEVEL_STARTUP)
endif
ifndef CONFIG_USER_XMLCONFIG
ifndef CONFIG_USER_CONF_ON_XMLFILE
	$(ROMFSINST) -a "/bin/flash check" $(RC_LEVEL_FLASH_CHK)
endif
endif

ifdef CONFIG_USER_XMLCONFIG
	$(ROMFSINST) -a "/etc/scripts/reset_default.sh" $(RC_LEVEL_CONFIGD)
endif
ifdef CONFIG_00R0
	$(ROMFSINST) -a "echo 3 > /proc/internet_flag" $(RC_LEVEL_STARTUP)
endif
ifdef CONFIG_USER_RTK_CONVERT_XML
	$(ROMFSINST) -a "convertxml" $(RC_LEVEL_CONFIGD)
endif
ifdef CONFIG_E8B
	$(ROMFSINST) -a "/etc/scripts/check_province.sh" $(RC_LEVEL_CONFIGD)
endif
	$(ROMFSINST) -a "configd -D" $(RC_LEVEL_CONFIGD)
ifdef CONFIG_LIB_UBUS
	$(ROMFSINST) -a "echo root:x:0: > /var/group" $(RC_LEVEL_CONFIGD)
	$(ROMFSINST) -a "echo root:x:0:0:root:/tmp:/bin/sh > /var/passwd" $(RC_LEVEL_CONFIGD)
	$(ROMFSINST) -a "ubusd&" $(RC_LEVEL_CONFIGD)
endif
ifdef CONFIG_USER_CTCAPD
	$(ROMFSINST) -a "ctcapd&" $(RC_LEVEL_STARTUP)
endif
ifdef CONFIG_RTK_DEV_AP
	$(ROMFSINST) -a "startup&" $(RC_LEVEL_STARTUP)
	$(ROMFSINST) -a "/bin/sh /etc/scripts/ax_wifi_ext.sh" $(RC_LEVEL_STARTUP)
else
	$(ROMFSINST) -a "startup -d &" $(RC_LEVEL_STARTUP)
endif
ifdef CONFIG_DSL_VTUO
	$(ROMFSINST) -a "/etc/scripts/swcore_type.sh" $(RC_LEVEL_STARTUP)
endif
ifdef CONFIG_ANDLINK_SUPPORT
	$(ROMFSINST) -a "echo 1 > /var/tmp/rtl_link_power_on" $(RC_LEVEL_STARTUP)
endif



clean:
	# remove old IDs
	rm -f *_id;
	for i in $(DIRS) ; do make -C $$i clean ; done


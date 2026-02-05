# Generated automatically from Makefile.in by configure.  

#KERNEL_HEADERS=-I$(ROOTDIR)/linux-2.4.x

## Need use GCC Warning compile error, don't remove it ##
CFLAGS += -Werror

ifdef CONFIG_IPV6
CFLAGS += -DIPV6
endif 

#CFLAGS += -D_DEBUG_MESSAGE

ifndef UCLINUX_BUILD_USER
LDFLAGS = -LLINUX
LIBCRYPT = -lcrypt
endif

ifdef CONFIG_LIB_LIBRTK_LED
CFLAGS += -I$(TMPFSDIR)/include/librtk_led
LDFLAGS += -L$(TMPFSDIR)/lib -lrtk_led
endif
ifdef CONFIG_USER_MINIUPNP_V2_1
ifeq ($(CONFIG_USER_IPTABLES_IPTABLES_1_4_18), y)
IPTABLES_DIR=$(ROOTDIR)/user/iptables-1.4.18
else ifeq ($(CONFIG_USER_IPTABLES_IPTABLES_1_4_21), y)
IPTABLES_DIR=$(ROOTDIR)/user/iptables-1.4.21
else
IPTABLES_DIR=$(ROOTDIR)/user/iptables
endif
CFLAGS+=-I$(IPTABLES_DIR)/include
LDFLAGS+=-L$(IPTABLES_DIR)/libiptc/.libs/ -lip4tc
ifeq ($(CONFIG_IPV6), y)
LDFLAGS+=-lip6tc
endif
LDFLAGS+=-L$(IPTABLES_DIR)/libxtables/.libs/ -lxtables
endif
LDFLAGS+=-L$(ROOTDIR)/lib/libupg/ -lupg -lsyslog -lrt
CFLAGS+=-I$(ROOTDIR)/lib/libupg/

GCC_FLAGS = -Wstrict-prototypes -Wpointer-arith -Wcast-align -Wcast-qual\
  -Wtraditional\
  -Wshadow\
  -Wconversion\
  -Waggregate-return\
  -Wmissing-prototypes\
  -Wnested-externs\
  -Wall

# Add -DNO_DAEMON if you do not want the server to fork upon launch
#LDFLAGS = -lnsl 
#CFLAGS = -O -DLINUX -I$(ROOTDIR)/lib/include -I$(ROOTDIR)/lib/include/sys -I$(ROOTDIR)/$(LINUXDIR)/include -I$(ROOTDIR)/$(LINUXDIR)/include/asm-mips -I$(ROOTDIR)/include -I. -I./rtl865x 

#CFLAGS += -DSERVER_SSL -I$(ROOTDIR)/lib/libssl/include
#SSLLIB =$(ROOTDIR)/lib/libssl/libssl.a $(ROOTDIR)/lib/libssl/libcrypto.a
#LDFLAGS	+= $(SSLLIB) $(IFMODLIB)
MANDIR = /usr/local/man
BINDIR = /usr/local/sbin

ifeq ($(CONFIG_RTK_DOS), y)
CFLAGS += -DCONFIG_DEONET_DOS
endif

ifeq ($(CONFIG_USER_IGMPPROXY), y)
CFLAGS += -DCONFIG_DEONET_IGMPPROXY
endif

CFLAGS += -DCONFIG_DEONET_DATAPATH
CFLAGS += -DCONFIG_DEONET_RULE_REFRESH
CFLAGS += -DCONFIG_DEONET_SESSION

ifeq ($(CONFIG_WFO_VIRT),m)
CFLAGS += -DCONFIG_WFO_VIRT
ifeq ($(CONFIG_WFO_VIRT_SAME_CPU),y)
CFLAGS += -DCONFIG_WFO_VIRT_SAME_CPU
endif #CPTCFG_WFO_VIRT_SAME_CPU
#CFLAGS += -DCONFIG_WFO_VIRT_DEBUG
endif #CPTCFG_WFO_VIRT

ifeq ($(CONFIG_USER_RTK_VOIP),y)
-include $(ROOTDIR)/$(LINUXDIR)/.config
VOIP_APP = $(ROOTDIR)/user/rtk_voip
CFLAGS += -DVOIP_SUPPORT -I$(ROOTDIR)/rtk_voip/voip_manager_1.6/includes -I$(VOIP_APP)/includes -I$(VOIP_APP)/web \
	  -I$(PWD) -I$(PWD)/.. -I$(VOIP_APP)/src/csv_parser
VOIPLIB =  $(VOIP_APP)/web/libweb_voip.a
VOIPLIB += -L$(ROOTDIR)/rtk_voip/voip_manager_1.6 -lvoip_manager
VOIPLIB += $(VOIP_APP)/src/csv_parser/libcsv_parser.a
endif

ifeq ($(CONFIG_USER_TR111_STUN),y)
TOPDIR := $(shell pwd)/../../..
STUN_DIR = $(TOPDIR)/user/cwmp-tr069/stun
CFLAGS += -D_TR111_STUN_ -I$(STUN_DIR)
endif

## --- HTTPS server support --- ##
ifeq ($(CONFIG_USER_BOA_WITH_SSL),y)
# ---- HTTPS with Matrix SSL library ---
ifeq ($(CONFIG_USER_MATRIXSSL),y)
LIBMATRIXSSL_INC_DIR=$(ROOTDIR)/lib/libmatrixssl/
CFLAGS +=  -DLINUX -I$(LIBMATRIXSSL_INC_DIR)
LDFLAGS += -L$(ROOTDIR)/lib/libmatrixssl/
SSLLIB = -lmatrixssl
else
ifeq ($(CONFIG_USER_MATRIXSSL_NEW),y)
LIBMATRIXSSL_INC_DIR=$(ROOTDIR)/user/cwmp-tr069/matrixssl-3-6-2-open
CFLAGS +=  -DLINUX -I$(LIBMATRIXSSL_INC_DIR) -I$(LIBMATRIXSSL_INC_DIR)/matrixssl
LDFLAGS += -L$(ROOTDIR)/lib/libmatrixssl_new/
SSLLIB = -lcore -lpscrypto -lmatrixssl_new
else
LIBSSL_INC_DIR=$(ROOTDIR)/lib/libssl/include/
CFLAGS +=  -DLINUX -I$(LIBSSL_INC_DIR)
SSLLIB = $(LIBSSL)
endif
endif
endif
ifdef CONFIG_00R0
LIBSSL_INC_DIR=$(ROOTDIR)/lib/libssl/include/
CFLAGS +=  -DLINUX -I$(LIBSSL_INC_DIR)
SSLLIB = -lssl -lcrypto
endif
# --- End of HTTPS ---

CFLAGS += -fPIC
ifeq ($(CONFIG_RTL_WPS2_SUPPORT),y)
CFLAGS += -DWPS20
endif

ifeq ($(CONFIG_RTK_OMCI_V1), y)
CFLAGS += -I$(APOLLODIR)/sdk/src/app/omci_v1/API -I$(APOLLODIR)/sdk/src/app/omci_v1/GOS/inc
LDFLAGS += -L$(APOLLODIR)/sdk/src/app/omci_v1/LIB
OMCILIB = -lomci_api
endif	#CONFIG_RTK_OMCI_V1

LDFLAGS += -lm

CFLAGS += -I$(APOLLODIR)/FleetConntrackDriver/rt/include

ifeq ($(CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE), m)
CFLAGS += -I$(APOLLODIR)/igmpHookModule/rt_igmpHook/include
endif

ifeq ($(CONFIG_RTL_WPS_VERSION_USER_CONFIG),y)
CFLAGS += -DWPS_VERSION_CONFIGURABLE
endif

#CFLAGS += -DMULTI_LANG_DL=$(MULTI_LANG_DL)
CFLAGS += -DMULTI_LANG_DL=0

ifeq ($(CONFIG_PRODUCT),luna_ap_arm)  ##98f 
ifneq ($(CONFIG_ARM64),y)
include $(BOOTLOADER_PATH)/u-boot-2016.07/include/config/auto.conf
ifdef CONFIG_LUNA_MULTI_BOOT
CFLAGS += -DLUNA_MULTI_BOOT
endif
endif
endif

ifeq ( $(CONFIG_USER_BOA_MULTI_USER_PRIV), y) ##MULTI_USER_PRIV
CFLAGS += -DMULTI_USER_PRIV
endif
# Change these if necessary

ifeq ($(CONFIG_USER_RTK_EXTGPHY),y)
LDFLAGS += -L$(ROOTDIR)/user/libextgphy
endif

SUBDIRS=LINUX
WEBLIB=LINUX/libwebs.a
# LIBMIB defined in user/Makefile
MIBLIB=$(LIBMIB)
# static-linked libmib
#MIBLIB=LINUX/libmib.a -lpthread $(LIBRTK)

# Web file 
ORI_WEB=./web_general
ifdef CONFIG_00R0
ORI_WEB=./web_rostelecom
endif
ifdef CONFIG_E8B
ORI_WEB=./web_ctc
ifeq ($(CONFIG_CU),y)
ifeq ($(CONFIG_CU_BASEON_CMCC),y)
ORI_WEB=./web_cu
endif
ifeq ($(CONFIG_CU_BASEON_YUEME),y)
ORI_WEB=./web_cu_dbus
endif
else
ifdef CONFIG_CMCC
ORI_WEB=./web_cmcc
endif
# for AP team CMCC UI
ifdef CONFIG_USER_AP_CMCC
ORI_WEB=./web_cmcc
endif
ifeq ($(CONFIG_CMCC_ENTERPRISE),y)
ORI_WEB=./web_cmcc_enterprise
endif
endif
endif
ifdef CONFIG_TELMEX
ORI_WEB=./web_telmex
endif
ifdef CONFIG_TRUE
ORI_WEB=./web_true
endif
ORI_SUB=$(ORI_WEB)/admin
PARSED_WEB=./web_parse_ml
PARSED_SUB=$(PARSED_WEB)/admin
ifeq ($(CONFIG_USER_RTK_VOIP),y)
VOIP_PARSED_WEB=$(VOIP_APP)/voip_web_parse_ml
endif
ifdef CONFIG_E8B
ASP_HDR_PATH=$(ROOTDIR)/user/boa/src/LINUX/form_src_e8/asp.h
else
ASP_HDR_PATH=$(ROOTDIR)/user/boa/src/LINUX/form_src/asp.h
endif
ASPFLAG=-E -x c -P -Xpreprocessor "-traditional-cpp"
ROMFS_WEB=./web_romfs
ROMFS_SUB=$(ROMFS_WEB)/admin
ROMFS_GIF=$(ROMFS_WEB)/graphics
# Web file end

ifeq ($(CONFIG_USER_AP_CMCC),y)
REALTEK_FILES += cJSON.c perform_pppoe.c
endif
ifeq ($(CONFIG_USER_AP_CMCC),y)
CFLAGS += -lm
LDFLAGS += -lm
endif
ifeq ($(CONFIG_LIB_LIBNETFILTER_CONNTRACK),y)
LDFLAGS +=  -I$(ROOTDIR)/lib/libnetfilter_conntrack/libnetfilter_conntrack-1.0.8/include -lnetfilter_conntrack -lmnl -lnfnetlink
endif
ifeq ($(CONFIG_USER_LOCK_NET_FEATURE_SUPPORT), y)
REALTEK_FILES += portal.c
endif
SOURCES = alias.c boa.c cgi.c config.c get.c hash.c log.c \
    queue.c read.c request.c response.c signals.c util.c \
    cgi_header.c pipe.c nls.c auth.c globals.c \
    asp_page.c port.c msgutil.c \
    $(REALTEK_FILES)
ifdef UCLINUX_BUILD_USER
ifndef CONFIG_LIB_LIBMD5_FORCE
SOURCES += md5.c
endif
else
SOURCES += md5.c
endif

ifdef CONFIG_CMCC
SOURCES += md5.c sha256.c
endif

ifdef CONFIG_CU_BASEON_CMCC
SOURCES += md5.c sha256.c
endif

SOURCES += sha256.c

ifdef CONFIG_USER_LOGIN_WEB_PASSWORD_ENCRYPT
SOURCES += web_psd_encrypt.c
endif

SOURCES += ap_connection.c 
    
PAGE_ADMIN_DEP1 = mtmcode.js share.js acl.html date.asp fw-macfilter.asp parental-ctrl.asp reboot.asp \
	saveconf.asp status.asp upgrade.asp upgrade2.asp upgrade3.asp toupgrade.asp index2.asp wladvanced.asp \
	wlbasic.asp wlstatbl.asp wlwpa.asp wlactrl.asp wlwds.asp wlsurvey.asp wlwps.asp connlimit.asp wlmultipleap.asp wlstatbl_vap.asp \
	ripospf.asp natsession.asa useage.asp autoreboot.asp selfcheck.asp client_info.asp invalid_page.asp
# add for 11n wagem logo

PAGE_ADMIN_DEP2 = graphics/del.gif graphics/edit.gif graphics/sagemlogo2.gif graphics/sagemlogo1.gif \
	graphics/topbar.gif menu-images/menu_bar.gif \
	menu-images/menu_corner.gif menu-images/menu_corner_minus.gif \
	menu-images/menu_corner_plus.gif menu-images/menu_folder_closed.gif \
	menu-images/menu_folder_open.gif menu-images/menu_link_default.gif \
	menu-images/menu_pixel.gif menu-images/menu_root.gif \
	menu-images/menu_tee.gif menu-images/menu_tee_minus.gif \
	menu-images/menu_tee_plus.gif
# add for 11n wagem logo

OBJS = y.tab.o lex.yy.o ${SOURCES:.c=.o} timestamp.o

.PHONY: clean mrclean depend all
 
ifdef UCLINUX_BUILD_USER
all:	${OBJS} subdir boa
else
all:	${OBJS} subdir boa webs_link
endif

subdir:
	for x in $(SUBDIRS); do (cd $$x && $(MAKE) ); done

msgutil.o: CFLAGS += -D_GNU_SOURCE

boa.o: $(ROOTDIR)/include/linux/autoconf.h

boa:	${OBJS} $(WEBLIB)
ifeq ($(MULTI_LANG_DL),1)
	${CC} $(LDFLAGS) -o $@ ${OBJS} ${WEBLIB} $(VOIPLIB) $(MIBLIB) $(LDLIBS$(LDLIBS-$(@))) $(OMCILIB) $(SSLLIB) $(LIBCRYPT) $(LIBMD5) -ldl
else
	${CC} $(LDFLAGS) -o $@ ${OBJS} ${WEBLIB} $(VOIPLIB) $(MIBLIB) $(LDLIBS$(LDLIBS-$(@))) $(OMCILIB) $(SSLLIB) $(LIBCRYPT) $(LIBMD5)
endif
webs_link:
	for i in $(PAGE_ADMIN_DEP1) ; do \
		[ -e web/admin/$$i ] || ln -s ../$$i web/admin/$$i; \
	done
	for i in $(PAGE_ADMIN_DEP2) ; do \
		[ -e web/admin/$$i ] || ln -s ../../$$i web/admin/$$i; \
	done

clean:
	find . -name '*.o' | xargs rm -f
	find . -name '*.so' | xargs rm -f
	rm -rf $(PARSED_WEB) $(VOIP_PARSED_WEB)
	rm -f boa
	for i in $(PAGE_ADMIN_DEP1) $(PAGE_ADMIN_DEP2); do \
		rm -f web/admin/$$i; \
	done
	rm -f 
	for x in $(SUBDIRS); do (cd $$x && $(MAKE) clean ); done
#	rm -f ${OBJS} boa core lex.yy.c y.tab.c y.tab.h rtl865x/*.o LINUX/*.o LINUX/*.a
	
mrclean:	clean
	rm -f config.status config.cache config.h Makefile config.log

#escape.h:	escape.pl
#	perl escape.pl >escape.h

%.d: %.c
	$(CC) -E $(CFLAGS) $(EXTRACFLAGS) -Wp,-MD,$*.d $< >/dev/null
	
depend:
	rm *.o rtl865x/*.o -f
	${CC} $(CFLAGS) -E -MM ${SOURCES} > .depend
	
-include .depend

timestamp.c:	${SOURCES} boa_grammar.y boa_lexer.l
	touch timestamp.c

install:	boa
	install -c -m 755 boa ${BINDIR} 
	install -c -m 755 ../utils/boa_indexer ${BINDIR}
	install -c -m 644 ../docs/boa.1 ${MANDIR}/man1

SOCKET = socket localhost 80
ECHO = /bin/echo

.PHONY: test dne cgi_bin dir normal

test:	dne cgi_bin dir normal

dne:
	${ECHO} 'GET /file_that_does_not_exit HTTP/1.0\n\n' | ${SOCKET}
	${ECHO} 'GET /file_that_does_not_exit\n' | ${SOCKET}
	${ECHO} 'HEAD /file_that_does_not_exit HTTP/1.0\n\n' | ${SOCKET}
	${ECHO} 'HEAD /file_that_does_not_exit\n' | ${SOCKET}

cgi_bin:
	${ECHO} 'GET /cgi-bin/cgi-test.cgi HTTP/1.0\n\n' | ${SOCKET}
	${ECHO} 'GET /cgi-bin/cgi-test.cgi\n' | ${SOCKET}
	${ECHO} 'HEAD /cgi-bin/cgi-test.cgi HTTP/1.0\n\n' | ${SOCKET}
	${ECHO} 'HEAD /cgi-bin/cgi-test.cgi\n' | ${SOCKET}

DIR = /doc/

dir:
	${ECHO} 'GET ${DIR} HTTP/1.0\n\n' | ${SOCKET}
	${ECHO} 'GET ${DIR}\n' | ${SOCKET}
	${ECHO} 'HEAD ${DIR} HTTP/1.0\n\n' | ${SOCKET}
	${ECHO} 'HEAD ${DIR}\n' | ${SOCKET}

NORMAL = /test_file.gz

normal:
	${ECHO} 'GET ${NORMAL} HTTP/1.0\n\n' | ${SOCKET} | wc -c
	${ECHO} 'GET ${NORMAL}\n' | ${SOCKET} | wc -c
	${ECHO} 'HEAD ${NORMAL} HTTP/1.0\n\n' | ${SOCKET}
	${ECHO} 'HEAD ${NORMAL}' | ${SOCKET}

GZ = /test_file

# Note: to test automatic gunzipping a file named "test_file.gz" must be in
# the document root.

gz:	
	${ECHO} 'GET ${GZ} HTTP/1.0\n\n' | ${SOCKET} | wc -c
	${ECHO} 'GET ${GZ}\n' | ${SOCKET} | wc -c
	${ECHO} 'HEAD ${GZ} HTTP/1.0\n\n' | ${SOCKET}
	${ECHO} 'HEAD ${GZ}\n' | ${SOCKET}

.PHONY: rcX
rcX:
	for x in $(SUBDIRS); do make -C $$x $@; done

include web_list
romfs:
	for x in $(SUBDIRS); do make -C $$x $@; done

ifeq ($(CONFIG_USER_BOA_BOA),y)
# general web
	if [ ! -d $(PARSED_WEB) ]; then \
		mkdir $(PARSED_WEB); \
	fi
	test -d $(PARSED_WEB) && rm -rf $(PARSED_WEB)

	find $(ORI_WEB) -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/\"/DOUBLE_QUOTE_TMP/g";
	find $(ORI_WEB) -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/'/SINGLE_QUOTE_TMP/g";
	
	cp -R $(ORI_WEB) $(PARSED_WEB)
	# Remove hidden files, ex. .svn, .git ...
	find $(PARSED_WEB) -name ".*" | xargs rm -rf

ifdef CONFIG_E8B
ifdef CONFIG_CMCC_ENTERPRISE
	-for f in $(ORI_WEB)/*.asp; do \
		name=$$(basename $$f); \
		$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) -include $(ROOTDIR)/config/autoconf.h $$f > $(PARSED_WEB)/$$name; \
	done
	-for f in $(ORI_SUB)/*.asp; do \
		name=$$(basename $$f); \
		$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) -include $(ROOTDIR)/config/autoconf.h $$f > $(PARSED_SUB)/$$name; \
	done
endif
else
	-for f in $(ORI_WEB)/*.asp; do \
		name=$$(basename $$f); \
		$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) -include $(ROOTDIR)/config/autoconf.h $$f > $(PARSED_WEB)/$$name; \
	done
	-for f in $(ORI_SUB)/*.asp; do \
		name=$$(basename $$f); \
		$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) -include $(ROOTDIR)/config/autoconf.h $$f > $(PARSED_SUB)/$$name; \
	done
endif

ifneq ($(CONFIG_GENERAL_WEB),y)
ifdef CONFIG_E8B
ifdef CONFIG_CMCC_ENTERPRISE
	-for f in $(ORI_WEB)/*.js; do \
		name=$$(basename $$f); \
		$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) $$f > $(PARSED_WEB)/$$name; \
	done
	-for f in $(ORI_SUB)/*.js; do \
		name=$$(basename $$f); \
		$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) $$f > $(PARSED_SUB)/$$name; \
	done
	$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) $(ORI_WEB)/js/menu_dir.js > $(PARSED_WEB)/js/menu_dir.js
	$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) $(ORI_WEB)/index.html > $(PARSED_WEB)/index.html
	$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) $(ORI_WEB)/admin/index_user.html > $(PARSED_WEB)/admin/index_user.html
endif
else
	-for f in $(ORI_WEB)/*.js; do \
		name=$$(basename $$f); \
		$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) $$f > $(PARSED_WEB)/$$name; \
	done
	-for f in $(ORI_SUB)/*.js; do \
		name=$$(basename $$f); \
		$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) $$f > $(PARSED_SUB)/$$name; \
	done
endif
else
	$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) $(ORI_WEB)/share.js > $(PARSED_WEB)/share.js
	$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) $(ORI_WEB)/adminMenu.js > $(PARSED_WEB)/adminMenu.js
	$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) $(ORI_WEB)/admin/userMenu.js > $(PARSED_WEB)/admin/userMenu.js
endif
	
	find $(ORI_WEB) -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/DOUBLE_QUOTE_TMP/\"/g";
	find $(ORI_WEB) -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/SINGLE_QUOTE_TMP/'/g";
	find $(PARSED_WEB) -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/DOUBLE_QUOTE_TMP/\"/g";
	find $(PARSED_WEB) -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/SINGLE_QUOTE_TMP/'/g";

# voip web
ifeq ($(CONFIG_USER_RTK_VOIP),y)
	if [ ! -d $(VOIP_PARSED_WEB) ]; then \
		mkdir $(VOIP_PARSED_WEB); \
	fi
	test -d $(VOIP_PARSED_WEB) && rm -rf $(VOIP_PARSED_WEB)

	find $(VOIP_APP)/web -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/\"/DOUBLE_QUOTE_TMP/g";
	find $(VOIP_APP)/web -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/'/SINGLE_QUOTE_TMP/g";
ifdef CONFIG_CMCC_ENTERPRISE
	find $(VOIP_APP)/web/e8c -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/\"/DOUBLE_QUOTE_TMP/g";
	find $(VOIP_APP)/web/e8c -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/'/SINGLE_QUOTE_TMP/g";
endif
	cp -Rf $(VOIP_APP)/web $(VOIP_PARSED_WEB)
	-for f in $(VOIP_APP)/web/*.asp; do \
		name=$$(basename $$f); \
		$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) $$f > $(VOIP_PARSED_WEB)/$$name; \
	done
ifdef CONFIG_CMCC_ENTERPRISE
	-for f in $(VOIP_APP)/web/e8c/*.asp; do \
		name=$$(basename $$f); \
		$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) $$f > $(VOIP_PARSED_WEB)/e8c/$$name; \
	done
endif
ifneq ($(CONFIG_GENERAL_WEB),y)
	-for f in $(VOIP_APP)/web/*.js; do \
		name=$$(basename $$f); \
		$(CC) $(ASPFLAG) -include $(ASP_HDR_PATH) $$f > $(VOIP_PARSED_WEB)/$$name; \
	done
endif
ifdef CONFIG_GENERAL_WEB
	rm -f $(ROMFSDIR)/home/httpd/web/adminMenu_base.js $(ROMFSDIR)/home/httpd/web/admin/userMenu_base.js
	rm -f $(ROMFSDIR)/home/httpd/web/index_base.html
	rm -f $(ROMFSDIR)/home/httpd/web/index_base.asp
	rm -f $(ROMFSDIR)/home/httpd/web/index_user_base.html
	rm -f $(ROMFSDIR)/home/httpd/web/index_user_base.asp
endif
	find $(VOIP_APP)/web -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/DOUBLE_QUOTE_TMP/\"/g";
	find $(VOIP_APP)/web -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/SINGLE_QUOTE_TMP/'/g";
	find $(VOIP_PARSED_WEB) -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/DOUBLE_QUOTE_TMP/\"/g";
	find $(VOIP_PARSED_WEB) -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/SINGLE_QUOTE_TMP/'/g";
ifdef CONFIG_CMCC_ENTERPRISE
	find $(VOIP_APP)/web/e8c -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/DOUBLE_QUOTE_TMP/\"/g";
	find $(VOIP_APP)/web/e8c -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/SINGLE_QUOTE_TMP/'/g";
	find $(VOIP_PARSED_WEB)/e8c -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/DOUBLE_QUOTE_TMP/\"/g";
	find $(VOIP_PARSED_WEB)/e8c -regextype posix-extended -regex '.*[js or asp]' -type f | xargs sed -i "s/SINGLE_QUOTE_TMP/'/g";
endif
endif
ifeq ("$(CONFIG_PRD_SIGNATURE)$(CONFIG_E8B)", "9601DVD")
	echo -n "Install web for 9601DVD"
	if [ ! -d $(ROMFS_WEB) ]; then \
		mkdir $(ROMFS_WEB); \
	fi
	test -d $(ROMFS_WEB) && rm -rf $(ROMFS_WEB)
	mkdir $(ROMFS_WEB)
	mkdir $(ROMFS_SUB)
	mkdir $(ROMFS_GIF)
	cp $(web-y) $(ROMFS_WEB)
	cp $(admin-y) $(ROMFS_SUB)
	cp $(graphic-y) $(ROMFS_GIF)
	cp $(js-y) $(ROMFS_WEB)
	cp $(html-y) $(ROMFS_WEB)
	cp $(PARSED_SUB)/LoginFiles -a $(ROMFS_SUB)
	cp $(PARSED_WEB)/menu-images -a $(ROMFS_WEB)
	cp $(PARSED_WEB)/style -a $(ROMFS_WEB)
	$(ROMFSINST) $(ROMFS_WEB)  /home/httpd/web
	rm -f $(ROMFSDIR)/home/httpd/web/wl*
	rm -f $(ROMFSDIR)/home/httpd/web/*adsl*
	rm -f $(ROMFSDIR)/home/httpd/web/admin/adsl*
else
	$(ROMFSINST) $(PARSED_WEB)  /home/httpd/web
endif
	$(ROMFSINST) /bin/boa
ifdef	CONFIG_USER_BOA_AUTH_DIGEST_SUPPORT
	$(ROMFSINST) boa_digest.conf /home/httpd/boa.conf
else
	$(ROMFSINST) /home/httpd/boa.conf
endif
	$(ROMFSINST) -e CONFIG_USER_Y1731 $(PARSED_WEB)/ethoam.asp /home/httpd/web
ifdef CONFIG_USER_RTK_VOIP
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_script.js /home/httpd/web
ifneq ($(CONFIG_GENERAL_WEB),y)
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_general.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_advanced.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_tone.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_network.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_callhistory.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_other.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_sip_status.asp /home/httpd/web
else
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_general_new_web.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_advanced_new_web.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_tone_new_web.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_network_new_web.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_callhistory_new_web.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_other_new_web.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_sip_status_new_web.asp /home/httpd/web
endif
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_tone2.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_ring.asp /home/httpd/web
#	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_config.asp /home/httpd/web
#	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_fxo.asp /home/httpd/web
ifeq ($(CONFIG_RTK_VOIP_IVR),y)
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_ivr_req.asp /home/httpd/web
endif
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/app_voip.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/app_voip2.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/cmcc_app_voip.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/cmcc_app_voip2.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/diag_voip.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/auto_call_voip.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/status_voip_info.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/cmcc_status_voip_info.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/voip_index.asp /home/httpd/web
ifdef CONFIG_E8B
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/voip_flags.asp /home/httpd/web
else
	$(ROMFSINST) $(VOIP_PARSED_WEB)/voip_flags.asp /home/httpd/web
endif
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/help_status_voip.html /home/httpd/web/help
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/help_apply_voip.html /home/httpd/web/help
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/help_diag_voip.html /home/httpd/web/help
ifeq ($(CONFIG_CMCC_ENTERPRISE),y)
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/cmcc_voip_quick_setup.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/quick_voip.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/voipinfo.asp /home/httpd/web
	$(ROMFSINST) $(VOIP_PARSED_WEB)/e8c/voip.asp /home/httpd/web
endif
	$(ROMFSINST) -s /var/config_voip.dat  /home/httpd/web/config_voip.dat
	$(ROMFSINST) -s /var/log_rx_pcap.pcap /home/httpd/web/log_rx_pcap.pcap
	$(ROMFSINST) -s /var/log_to_PS_pcap.pcap /home/httpd/web/log_to_PS_pcap.pcap
endif
ifeq ($(CONFIG_USER_BOA_WITH_SSL),y)
	$(ROMFSINST) /etc/ssl_key.pem
	$(ROMFSINST) /etc/ssl_cert.pem
endif
ifeq ($(CONFIG_00R0),y)
ifeq ($(CONFIG_RTL9603CVD_SERIES),y)
	$(ROMFSINST) -s connect_9603c.gif  /home/httpd/web/admin/graphics/connect.gif
	$(ROMFSINST) -s connect_9603c.gif  /home/httpd/web/graphics/connect.gif
else
	$(ROMFSINST) -s connect_9607c.gif  /home/httpd/web/admin/graphics/connect.gif
	$(ROMFSINST) -s connect_9607c.gif  /home/httpd/web/graphics/connect.gif
endif
endif

endif	# CONFIG_USER_BOA_BOA

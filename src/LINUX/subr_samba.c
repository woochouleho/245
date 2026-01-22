#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include "mib.h"
#include "utility.h"

const char SMBDPID[] = "/var/run/smbd.pid";
#ifdef CONFIG_USER_NMBD
const char NMBDPID[] = "/var/run/nmbd.pid";
#endif

const char FW_SAMBA_ACCOUNT[] = "samba_account";

int startSamba(void)
{
	unsigned char samba_enable;
	char *argv[10],
#ifdef CONFIG_USER_NMBD
	     nbn[MAX_SAMBA_NETBIOS_NAME_LEN], nbn_opt[MAX_SAMBA_NETBIOS_NAME_LEN + 32],
#endif
	     ss[MAX_SAMBA_SERVER_STRING_LEN], ss_opt[MAX_SAMBA_SERVER_STRING_LEN + 64];
	int i = -1, ret = 0;
	int j = 0;

	mib_get_s(MIB_SAMBA_ENABLE, &samba_enable, sizeof(samba_enable));
	if (!samba_enable) {
		return -1;
	}

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	//do nothing
#else
	initSamba();
#endif //defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)

#ifdef CONFIG_USER_NMBD
	mib_get_s(MIB_SAMBA_NETBIOS_NAME, nbn, sizeof(nbn));
#endif
	mib_get_s(MIB_SAMBA_SERVER_STRING, ss, sizeof(ss));

	/* set NetBIOS Name and Server String via command line arguments */
#ifdef CONFIG_USER_NMBD
	sprintf(nbn_opt, "--option=netbios name=%s", nbn);
#endif
	sprintf(ss_opt, "--option=server string=%s", ss);

	argv[++i] = "/bin/nmbd";
#ifdef CONFIG_USER_NMBD
	argv[++i] = nbn_opt;
#endif
	argv[++i] = ss_opt;
	argv[++i] = NULL;

#ifdef CONFIG_USER_NMBD
	ret = do_nice_cmd(argv[0], argv, 0);
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	argv[0] = "/bin/smbd";
	ret |= do_nice_cmd(argv[0], argv, 0);	
#else
#ifdef CONFIG_USER_NMBD
	ret |= va_niced_cmd("/bin/smbd", 5, 0, "-s", SMB_CONF_FILE, nbn_opt, ss_opt, "-D");
#else
	ret |= va_niced_cmd("/bin/smbd", 4, 0, "-s", SMB_CONF_FILE, ss_opt, "-D");
#endif	
#endif

	return ret;
}

int stopSamba(void)
{
	pid_t pid;

#ifdef CONFIG_USER_NMBD
	pid = read_pid(NMBDPID);
	if (pid > 0) {
		/* nmbd is running */
		kill(pid, 9);
		unlink(NMBDPID);
	}
#endif

	pid = read_pid(SMBDPID);
	if (pid > 0) {
		/* smbd is running */
		/* using -pid would kill all processes whose process group ID is pid */
		kill(-pid, 9);
		unlink(SMBDPID);
	}
	
	return 0;
}

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
//do nothing
#else
int addSmbPwd(char *username, char *passwd){
	FILE *fp_pass=NULL;
	char cmd[512]={0};

	if(!username || !passwd){
		fprintf(stderr, "[%s:%d] null data\n");
		return 1;
	}

	//fprintf(stderr, "[%s:%d] user:%s, pwd:%s\n", __FUNCTION__, __LINE__, username, passwd);
	fp_pass = fopen("/tmp/.pass", "w"); 
	if(fp_pass == NULL )	 
		return 1;

	fprintf(fp_pass, "%s\n",passwd);
	fprintf(fp_pass, "%s\n",passwd);
	fclose(fp_pass);
#ifdef RESTRICT_PROCESS_PERMISSION
	chmod("/tmp/.pass", 0666);
#endif

#ifdef CONFIG_USER_SMBPASSWD
#ifdef CONFIG_USER_SAMBA_3_0_24
	sprintf(cmd, "smbpasswd %s %s", username, passwd);
#else
	sprintf(cmd, "cat /tmp/.pass | smbpasswd -a %s -s", username);
#endif
#else
	sprintf(cmd, "cat /tmp/.pass | pdbedit -a %s -t", username);
#endif
	system(cmd);
	return 0;
}

void initSmbPwd(void){
	char userName[MAX_NAME_LEN], userPass[MAX_PASSWD_LEN];
#ifdef SAMBA_ACCOUNT_INDEPENDENT	
	char samba_user[MAX_RTK_NAME_LEN]={0}, samba_passwd[MAX_RTK_NAME_LEN]={0};
#endif
	
#ifdef ACCOUNT_CONFIG
	MIB_CE_ACCOUNT_CONFIG_T entry;
	unsigned int totalEntry;
#endif
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
	MIB_CE_SMB_ACCOUNT_T smbentry;
	int mibtotal=0;
#endif
	int i=0;
	char cmd[512];

	//flush password account
#ifdef CONFIG_USER_SMBPASSWD
	sprintf(cmd,"echo > %s", SMB_PWD_FILE);
	system(cmd);
#else
	FILE *pp = popen("pdbedit -L", "r");
	char *line=NULL;
	size_t len = 0;
	ssize_t nread;
	char tmpNAME[MAX_NAME_LEN]={0}, tmpUID[6]={0};

	if(!pp){
		printf("[%s:%d] ERROR: Initial samba password fail\n", __FUNCTION__, __LINE__);
		return;
	}

	/**
	 ** pdbedit -L       ==>	admin:0:
	 ** pdbedit -L -w  ==>	admin:0:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:F441F41AA59214CCCC3D4BA5ED1550CC:[U          ]:LCT-00000012:
	**/
       while ((nread = getline(&line, &len, pp)) != -1) {
		if(sscanf(line, "%[^:]:%[^:]:", tmpNAME, tmpUID) == 2){
			sprintf(cmd,"pdbedit -x %s", tmpNAME);
			system(cmd);
		}
       }

	free(line);
	pclose(pp);
#endif

#ifdef ACCOUNT_CONFIG
	totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL); /* get chain record size */
	for (i=0; i<totalEntry; i++) {
		if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&entry)) {
			printf("ERROR: Get account configuration information from MIB database failed.\n");
			return;
		}

		addSmbPwd((char *)entry.userName, (char *)entry.userPassword); 
	}
#endif
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
	mibtotal = mib_chain_total(MIB_SMBD_ACCOUNT_TBL);
	for (i=0; i<mibtotal; i++)
	{
		if (!mib_chain_get(MIB_SMBD_ACCOUNT_TBL, i, (void *)&smbentry)){
			printf("ERROR: Get account configuration information from MIB database failed.\n");
			return;
		}

		addSmbPwd((char *)smbentry.username, (char *)smbentry.password); 
	}
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	//do nothing
#else
#ifdef SAMBA_SYSTEM_ACCOUNT
	mib_get_s( MIB_SUSER_NAME, (void *)userName, sizeof(userName) );
	mib_get_s( MIB_SUSER_PASSWORD, (void *)userPass, sizeof(userPass) ); 
	addSmbPwd(userName, userPass);

	mib_get_s( MIB_USER_NAME, (void *)userName, sizeof(userName) );
	if (userName[0]) {
		mib_get_s( MIB_USER_PASSWORD, (void *)userPass, sizeof(userPass) );
		addSmbPwd(userName, userPass);
	}

#endif //defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)

#ifdef SAMBA_ACCOUNT_INDEPENDENT	
	mib_get_s( MIB_SAMBA_USERNAME, (void *)samba_user, sizeof(samba_user) );
	if (samba_user[0]) {
		mib_get_s( MIB_SAMBA_PASSWORD, (void *)samba_passwd, sizeof(samba_passwd) );
		addSmbPwd(samba_user, samba_passwd);
	}
#endif
#endif
	return;
}

int updateSambaCfg(unsigned char annonymous_enable)
{
	char userName[MAX_NAME_LEN], userPass[MAX_NAME_LEN];
	char samba_sharePath[MAX_RTK_PATH_LEN]={0};
#ifdef SAMBA_ACCOUNT_INDEPENDENT	
	char samba_user[MAX_RTK_NAME_LEN]={0};
#endif

#ifdef ACCOUNT_CONFIG
	MIB_CE_ACCOUNT_CONFIG_T entry;
	unsigned int totalEntry;
#endif
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
	MIB_CE_SMB_ACCOUNT_T smbentry;
	int mibtotal=0;
#endif
	int i;
	struct dirent **namelist;
	int n;
	unsigned char samba_use_smb2;
#ifdef CONFIG_USER_NMBD
	char fname[MAX_NAME_LEN];
#endif
#ifdef CONFIG_CMCC
	char ip_buff[16]={0};
#endif

	FILE *fp;
	fp = fopen(SMB_CONF_FILE,"wb+");
	n = scandir("/mnt", &namelist, usb_filter, alphasort);
	mib_get_s( MIB_SAMBA_USE_SMB2, &samba_use_smb2, sizeof(samba_use_smb2) );

	// part 1: write global config
	fprintf(fp,"%s\n","#");
	fprintf(fp,"%s\n","# Realtek Semiconductor Corp.");
	fprintf(fp,"%s\n","#");
	fprintf(fp,"%s\n","# Tony Wu (tonywu@realtek.com)");
	fprintf(fp,"%s\n","# Jan. 10, 2011\n");
	fprintf(fp,"\n");
	fprintf(fp,"%s\n","[global]");
	fprintf(fp,"%s\n","        # netbios name = Realtek");
	fprintf(fp,"%s\n","        # server string = Realtek Samba Server");
	fprintf(fp,"%s\n","        socket options = TCP_NODELAY SO_RCVBUF=131072 SO_SNDBUF=131072 IPTOS_LOWDELAY");
	fprintf(fp,"%s\n","        # unix charset = ISO-8859-1");
	fprintf(fp,"%s\n","        # preferred master = no");
	fprintf(fp,"%s\n","        # domain master = no");
	fprintf(fp,"%s\n","        # local master = yes");
	fprintf(fp,"%s\n","        # os level = 20");
	fprintf(fp,"%s\n","        smb ports = 445");
	fprintf(fp,"%s\n","        disable netbios = yes");
	fprintf(fp,"%s\n","        # guest account = admin");
	fprintf(fp,"%s\n","        deadtime = 15");
	fprintf(fp,"%s\n","        strict sync = no");
	fprintf(fp,"%s\n","        sync always = no");
	fprintf(fp,"%s\n","        dns proxy = no");
#ifdef CONFIG_CMCC
	getSYS2Str(SYS_DHCP_LAN_IP, ip_buff);
	fprintf(fp,"        socket address = %s\n", ip_buff);
	fprintf(fp,"%s\n","        bind interfaces only = yes");
	fprintf(fp,"%s\n","        interfaces = lo, br0");
#endif
	fprintf(fp,"%s\n","        use sendfile = true");
	fprintf(fp,"%s\n","        write cache size = 8192000");
	fprintf(fp,"%s\n","        min receivefile size = 16384");
	fprintf(fp,"%s\n","        aio read size = 131072");
	fprintf(fp,"%s\n","        aio write size = 131072");
	fprintf(fp,"%s\n","        aio write behind = true");
	fprintf(fp,"%s\n","        read raw = yes");
	fprintf(fp,"%s\n","        write raw = yes");
	fprintf(fp,"%s\n","        getwd cache = yes");
	fprintf(fp,"%s\n","        oplocks = yes");
	fprintf(fp,"%s\n","        max xmit = 32768");
	fprintf(fp,"%s\n","        large readwrite = yes");
	if(samba_use_smb2)
	{
		fprintf(fp, "\tprotocol = SMB2\n");
		fprintf(fp, "\tmin protocol = SMB2\n");
		fprintf(fp, "\tmax protocol = SMB2\n");
	}

	if(annonymous_enable && samba_use_smb2==0){	//share
		fprintf(fp,"%s\n","        syslog = 10");
		fprintf(fp,"%s\n","        security = share");
#if defined(CONFIG_CMCC)||defined(CONFIG_CU)
		fprintf(fp,"%s\n","        usershare allow guests = yes");
#else
		fprintf(fp,"%s\n","        mnt allow guests = no");
#endif
	}else{	//security
		fprintf(fp,"%s\n","        syslog = 1");
		fprintf(fp,"%s\n","        security = user");
		fprintf(fp,"%s\n","        encrypt passwords = true");
#if defined(CONFIG_CMCC)||defined(CONFIG_CU)
		fprintf(fp,"%s\n","        usershare allow guests = no");
#else
		fprintf(fp,"%s\n","        mnt allow guests = no");
#endif
	}
#ifdef CONFIG_USER_SMBPASSWD
	fprintf(fp,"%s\n","        passdb backend = smbpasswd");
	fprintf(fp,"%s\n","        smb passwd file = /var/samba/smbpasswd");
#endif

	fprintf(fp,"\n");	//finish part 1: write global config

#ifdef CONFIG_SMBD_SHARE_USB_ONLY
	if(annonymous_enable && samba_use_smb2==0){	//share
		for (i = 0; i < n; i++) {
			fprintf(fp, "[%s]\n", namelist[i]->d_name);
			fprintf(fp, "\tcomment = %s\n", namelist[i]->d_name);
			fprintf(fp, "\tpath = /mnt/%s\n", namelist[i]->d_name);
			fprintf(fp, "\tpublic = yes\n");
			fprintf(fp, "\twritable = yes\n");
			fprintf(fp, "\tprintable = no\n");
			fprintf(fp, "\tguest ok = yes\n");
		}
		fprintf(fp, "\n");
	}
	else {
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
		char add_samba_write_user[1024] = "        write list =";
		char tmpname[MAX_NAME_LEN];

		mibtotal = mib_chain_total(MIB_SMBD_ACCOUNT_TBL);
		for (i=0; i<mibtotal; i++)
		{
			if (!mib_chain_get(MIB_SMBD_ACCOUNT_TBL, i, (void *)&smbentry)){
				printf("ERROR: Get account configuration information from MIB database failed.\n");
				goto exit_func;
			}

			if (smbentry.writable==1) {
				sprintf(tmpname, " %s", (char*)smbentry.username);
				strncat(add_samba_write_user, tmpname, strlen(tmpname));
			}
		}
#endif
		for (i = 0; i < n; i++) {
			fprintf(fp, "[%s]\n", namelist[i]->d_name);
			fprintf(fp, "\tcomment = %s\n", namelist[i]->d_name);
			fprintf(fp, "\tpath = /mnt/%s\n", namelist[i]->d_name);
			fprintf(fp, "\tpublic = no\n");
			fprintf(fp, "\tprintable = no\n");
			fprintf(fp, "\tguest ok = no\n");
			fprintf(fp, "\tbrowseable = yes\n");
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
			fprintf(fp,"%s\n",add_samba_write_user);
#endif
		}
		fprintf(fp, "\n");
	}
#else // #ifdef CONFIG_SMBD_SHARE_USB_ONLY

	/** part2: write share folder config
	 ** job1: write smb.conf
	 ** job2: fillin smbpasswd file
	**/
#if defined(CONFIG_CMCC)||defined(CONFIG_CU)
	fprintf(fp, "[usbshare]\n");
#else
#ifdef CONFIG_USER_NMBD
	mib_get_s(MIB_SAMBA_NETBIOS_NAME, fname, sizeof(fname));
	fprintf(fp,"[%s]\n", fname);
#else
	fprintf(fp,"%s\n","[mnt]");
#endif
#endif

	fprintf(fp,"%s\n","        comment = realtek");
	mib_get_s( MIB_SAMBA_SHARE_PATH, (void *)samba_sharePath, sizeof(samba_sharePath) );
	fprintf(fp,"        path = %s\n", samba_sharePath[0]!=0?samba_sharePath:"/mnt");
	fprintf(fp,"%s\n","        writable = yes");
	fprintf(fp,"%s\n","        browseable = yes");
	fprintf(fp,"%s\n","        printable = no");

	if(annonymous_enable && samba_use_smb2==0){	//share
		fprintf(fp,"%s\n","        public = yes");
		fprintf(fp,"%s\n","        guest ok = yes");
	}else{	//security
		char add_samba_user_cmd[1024] = "        valid users =";
		char tmpname[MAX_NAME_LEN];

#ifdef ACCOUNT_CONFIG
		totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL); /* get chain record size */
		for (i=0; i<totalEntry; i++) {
			if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&entry)) {
				printf("ERROR: Get account configuration information from MIB database failed.\n");
				goto exit_func;
			}

			sprintf(tmpname, " %s", (char*)entry.userName);
			strncat(add_samba_user_cmd, tmpname, strlen(tmpname));
		}
#endif
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
		mibtotal = mib_chain_total(MIB_SMBD_ACCOUNT_TBL);
		for (i=0; i<mibtotal; i++)
		{
			if (!mib_chain_get(MIB_SMBD_ACCOUNT_TBL, i, (void *)&smbentry)){
				printf("ERROR: Get account configuration information from MIB database failed.\n");
				goto exit_func;
			}
		
			sprintf(tmpname, " %s", (char*)smbentry.username);
			strncat(add_samba_user_cmd, tmpname, strlen(tmpname));
		}
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
		//do nothing
#else
#ifdef SAMBA_SYSTEM_ACCOUNT
		mib_get_s( MIB_SUSER_NAME, (void *)userName, sizeof(userName) );
		sprintf(tmpname, " %s", userName);
		strncat(add_samba_user_cmd, tmpname, sizeof(add_samba_user_cmd)-strlen(add_samba_user_cmd)-1);

		mib_get_s( MIB_USER_NAME, (void *)userName, sizeof(userName) );
		if (userName[0]) {
			sprintf(tmpname, " %s", userName);
			strncat(add_samba_user_cmd, tmpname, sizeof(add_samba_user_cmd)-strlen(add_samba_user_cmd)-1);
		}
#endif //defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)

#ifdef SAMBA_ACCOUNT_INDEPENDENT		   
		mib_get_s( MIB_SAMBA_USERNAME, (void *)samba_user, sizeof(samba_user) );
		if (samba_user[0]) {
			sprintf(tmpname, " %s", samba_user);
			strncat(add_samba_user_cmd, tmpname, sizeof(add_samba_user_cmd)-strlen(add_samba_user_cmd)-1);
		}
#endif
#endif
		fprintf(fp,"%s\n",add_samba_user_cmd);
	}	

	fprintf(fp, "\n");
#ifdef CONFIG_MULTI_SMBD_ACCOUNT		
	mibtotal = mib_chain_total(MIB_SMBD_ACCOUNT_TBL);
	for (i=0; i<mibtotal; i++)
	{
		if (!mib_chain_get(MIB_SMBD_ACCOUNT_TBL, i, (void *)&smbentry)){
			printf("ERROR: Get account configuration information from MIB database failed.\n");
			goto exit_func;
		}
			
		if (!smbentry.username[0])//account is invalid
			continue;
				
		fprintf(fp, "[%s]\n", smbentry.username);
		fprintf(fp, "\tcomment = %s\n", smbentry.username);
		fprintf(fp, "\tpath = %s\n", smbentry.path[0]!=0 ? smbentry.path:"/mnt");
		fprintf(fp, "\tvalid users = %s\n", smbentry.username);
		fprintf(fp, "\tpublic = no\n");
		fprintf(fp, "\twritable = %s\n", smbentry.writable==1 ? "yes":"no");
		fprintf(fp, "\tprintable = no\n");
		fprintf(fp, "\tguest ok = no\n");
		fprintf(fp, "\n");
	}
#endif
#endif // #ifdef CONFIG_SMBD_SHARE_USB_ONLY

	// finish part2: write share folder config
exit_func:
	fclose(fp);
	return 0;
}

int initSamba(void)
{
	//char cmd1[512];
	unsigned char annonymous;

#if 0
	sprintf(cmd1,"echo \"telecomadmin:$1$$AwK5ysgOLgsgOriKA.7a./:0:0::/tmp:/bin/sh\" >> /var/passwd.smb");
	system(cmd1);
	sprintf(cmd1,"echo \"adsl:$1$$m9g7v7tSyWPyjvelclu6D1:0:0::/tmp:/bin/sh\" >> /var/passwd.smb");
	system(cmd1);
#endif

	/* it don't need group now */
	//sprintf(cmd1,"touch /var/group");
	//system(cmd1);

	//create samba config file
	mib_get_s(MIB_SMB_ANNONYMOUS, &annonymous, sizeof(annonymous));

	updateSambaCfg(annonymous);

	/**	init samba password, it should call after create samba config file,
	 **	because pdbedit will check smb.conf file
	 **/
	initSmbPwd();

	return 0;
}
#endif // CONFIG_YUEME

void delSambaAccessFw(){
	char FW_CHAIN[32]={0};
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	snprintf(FW_CHAIN, sizeof(FW_CHAIN), "%s", FW_INACC);
#else
	snprintf(FW_CHAIN, sizeof(FW_CHAIN), "%s", FW_INPUT);
#endif

	syslog(LOG_INFO, "iptables:in %s(%d)\r\n", __FUNCTION__, __LINE__);

	va_cmd(IPTABLES, 4, 1, "-D", (char *)FW_CHAIN, "-j", (char *)FW_SAMBA_ACCOUNT);
	//iptables -D INPUT -j samba_account
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_SAMBA_ACCOUNT);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_SAMBA_ACCOUNT);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-D", (char *)FW_INPUT, "-j", (char *)FW_SAMBA_ACCOUNT);
	va_cmd(IP6TABLES, 2, 1, "-F", (char *)FW_SAMBA_ACCOUNT);
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_SAMBA_ACCOUNT);
#endif
}

/*
*0:sambaserver disable;
*1:local enbale,remote disable;
*2:local disable,remote enable;
*3:local enbale,remote enbale
*/
void setSambaAccessFw(int enable){
	char FW_CHAIN[32]={0};
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	snprintf(FW_CHAIN, sizeof(FW_CHAIN), "%s", FW_INACC);
#else
	snprintf(FW_CHAIN, sizeof(FW_CHAIN), "%s", FW_INPUT);
#endif

	syslog(LOG_INFO, "iptables:in %s(%d):smba_enable[%d]\r\n", __FUNCTION__, __LINE__, enable);

	//iptables -N samba_account
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_SAMBA_ACCOUNT);
	//iptables -A INPUT -j samba_account
	va_cmd(IPTABLES, 4, 1, "-I", (char *)FW_CHAIN, "-j", (char *)FW_SAMBA_ACCOUNT);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_SAMBA_ACCOUNT);
	va_cmd(IP6TABLES, 4, 1, "-I", (char *)FW_INPUT, "-j", (char *)FW_SAMBA_ACCOUNT);
#endif
	if(enable == 0){//1:local enbale,remote disable;
		//iptables -A samba_account -i nas+  -p TCP --dport 445 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_DROP);
		
		//iptables -A samba_account -i nas+  -p UDP  --dport 137 -j DROP
	   va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		(char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
	   (char *)FW_DPORT, "137", "-j", (char *)FW_DROP);
		
	   //iptables -A samba_account -i nas+ -p UDP	--dport 138 -j DROP
	   va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		(char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
	   (char *)FW_DPORT, "138", "-j", (char *)FW_DROP);

	   //iptables -A samba_account -i ppp+  -p TCP --dport 445 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_DROP);

	    //iptables -A samba_account -i ppp+  -p UDP  --dport 137 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_DROP);
		
        //iptables -A samba_account -i ppp+  -p UDP  --dport 138 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_DROP);

	    //iptables -A samba_account -i br0  -p TCP --dport 445 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_DROP);

		 //iptables -A samba_account -i br0  -p UDP  --dport 137 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_DROP);
		 
        //iptables -A samba_account -i br0  -p UDP  --dport 138 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_DROP);
		
#ifdef CONFIG_IPV6
		//iptables -A samba_account -i nas+  -p TCP --dport 445 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "445", "-j", (char *)FW_DROP);

		//iptables -A samba_account -i nas+  -p UDP  --dport 137 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "137", "-j", (char *)FW_DROP);
		
		//iptables -A samba_account -i nas+  -p UDP  --dport 138 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "138", "-j", (char *)FW_DROP);

		//iptables -A samba_account -i ppp+  -p TCP --dport 445 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "445", "-j", (char *)FW_DROP);

		//iptables -A samba_account -i ppp+  -p UDP  --dport 137 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "137", "-j", (char *)FW_DROP);
		
		//iptables -A samba_account -i ppp+  -p UDP  --dport 138 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "138", "-j", (char *)FW_DROP);

		//iptables -A samba_account -i br0	-p TCP --dport 445 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "445", "-j", (char *)FW_DROP);

		//iptables -A samba_account -i br0	-p UDP	--dport 137 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "137", "-j", (char *)FW_DROP);
		
		//iptables -A samba_account -i br0	-p UDP	--dport 138 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "138", "-j", (char *)FW_DROP);
#endif
#ifdef CONFIG_USER_RTK_FASTPASSNF_MODULE
		rtk_fastpassnf_smbspeedup(0);
#endif
	}else if(enable == 1){//1:local enable,remote disable
		//iptables -A samba_account -i nas+  -p TCP --dport 445 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_DROP);
		
		//iptables -A samba_account -i nas+  -p UDP  --dport 137 -j DROP
	   va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		(char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
	   (char *)FW_DPORT, "137", "-j", (char *)FW_DROP);
		
	   //iptables -A samba_account -i nas+ -p UDP	--dport 138 -j DROP
	   va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		(char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
	   (char *)FW_DPORT, "138", "-j", (char *)FW_DROP);

	   //iptables -A samba_account -i ppp+  -p TCP --dport 445 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_DROP);

	    //iptables -A samba_account -i ppp+  -p UDP  --dport 137 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_DROP);
		
        //iptables -A samba_account -i ppp+  -p UDP  --dport 138 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_DROP);

	    //iptables -A samba_account -i br0  -p TCP --dport 445 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_ACCEPT);

		 //iptables -A samba_account -i br0  -p UDP  --dport 137 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_ACCEPT);
		 
        //iptables -A samba_account -i br0  -p UDP  --dport 138 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_ACCEPT);
		
#ifdef CONFIG_IPV6
		//iptables -A samba_account -i nas+  -p TCP --dport 445 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "445", "-j", (char *)FW_DROP);

		//iptables -A samba_account -i nas+  -p UDP  --dport 137 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "137", "-j", (char *)FW_DROP);
		
		//iptables -A samba_account -i nas+  -p UDP  --dport 138 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "138", "-j", (char *)FW_DROP);

		//iptables -A samba_account -i ppp+  -p TCP --dport 445 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "445", "-j", (char *)FW_DROP);

		//iptables -A samba_account -i ppp+  -p UDP  --dport 137 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "137", "-j", (char *)FW_DROP);
		
		//iptables -A samba_account -i ppp+  -p UDP  --dport 138 -j DROP
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "138", "-j", (char *)FW_DROP);

		//iptables -A samba_account -i br0	-p TCP --dport 445 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, "445", "-j", (char *)FW_ACCEPT);

		//iptables -A samba_account -i br0	-p UDP	--dport 137 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "137", "-j", (char *)FW_ACCEPT);
		
		//iptables -A samba_account -i br0	-p UDP	--dport 138 -j ACCEPT
		va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
		 (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, "138", "-j", (char *)FW_ACCEPT);
#endif

#ifdef CONFIG_USER_RTK_FASTPASSNF_MODULE
		rtk_fastpassnf_smbspeedup(1);
#endif
	}else if(enable == 2){//2:local disable,remote enable;
        //iptables -A samba_account -i nas+  -p TCP --dport 445 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i nas+  -p UDP  --dport 137 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_ACCEPT);
		
        //iptables -A samba_account -i nas+  -p UDP  --dport 138 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i ppp+  -p TCP --dport 445 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i ppp+  -p UDP  --dport 137 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_ACCEPT);
		
        //iptables -A samba_account -i ppp+  -p UDP  --dport 138 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i br0  -p TCP --dport 445 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_DROP);

        //iptables -A samba_account -i br0  -p UDP  --dport 137 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_DROP);
		
        //iptables -A samba_account -i br0  -p UDP  --dport 138 -j DROP
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_DROP);

#ifdef CONFIG_IPV6
        //iptables -A samba_account -i nas+  -p TCP --dport 445 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i nas+  -p UDP  --dport 137 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_ACCEPT);
		
        //iptables -A samba_account -i nas+  -p UDP  --dport 138 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i ppp+  -p TCP --dport 445 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i ppp+  -p UDP  --dport 137 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_ACCEPT);
		
        //iptables -A samba_account -i ppp+  -p UDP  --dport 138 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i br0  -p TCP --dport 445 -j DROP
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_DROP);

        //iptables -A samba_account -i br0  -p UDP  --dport 137 -j DROP
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_DROP);
		
        //iptables -A samba_account -i br0  -p UDP  --dport 138 -j DROP
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_DROP);
#endif

#ifdef CONFIG_USER_RTK_FASTPASSNF_MODULE
		rtk_fastpassnf_smbspeedup(0); //disable SAMBA accelerate when remote access only
#endif
    }else if(enable == 3){//3:local enbale,remote enbale
        //iptables -A samba_account -i nas+  -p TCP --dport 445 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i nas+  -p UDP  --dport 137 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_ACCEPT);
        //iptables -A samba_account -i nas+  -p UDP  --dport 138 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i ppp+  -p TCP --dport 445 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i ppp+  -p UDP  --dport 137 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_ACCEPT);
        //iptables -A samba_account -i ppp+  -p UDP  --dport 138 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i br0  -p TCP --dport 445 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i br0  -p UDP  --dport 137 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_ACCEPT);
		
        //iptables -A samba_account -i br0  -p UDP  --dport 138 -j ACCEPT
        va_cmd(IPTABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_ACCEPT);

#ifdef CONFIG_IPV6
        //iptables -A samba_account -i nas+  -p TCP --dport 445 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i nas+  -p UDP  --dport 137 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_ACCEPT);
		
        //iptables -A samba_account -i nas+  -p UDP  --dport 138 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "nas+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i ppp+  -p TCP --dport 445 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i ppp+  -p UDP  --dport 137 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_ACCEPT);
		
        //iptables -A samba_account -i ppp+  -p UDP  --dport 138 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, "ppp+", "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i br0  -p TCP --dport 445 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
        (char *)FW_DPORT, "445", "-j", (char *)FW_ACCEPT);

        //iptables -A samba_account -i br0  -p UDP  --dport 137 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "137", "-j", (char *)FW_ACCEPT);
		
        //iptables -A samba_account -i br0  -p UDP  --dport 138 -j ACCEPT
        va_cmd(IP6TABLES, 10, 1, "-A", (char *)FW_SAMBA_ACCOUNT,
         (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
        (char *)FW_DPORT, "138", "-j", (char *)FW_ACCEPT);
#endif

#ifdef CONFIG_USER_RTK_FASTPASSNF_MODULE
		rtk_fastpassnf_smbspeedup(1);
#endif
    }else{
		printf("[%s:%s:%d] invlaid samba enable (%d) status!",__FILE__,__FUNCTION__,__LINE__, enable);
	}
}


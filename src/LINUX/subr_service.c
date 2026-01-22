#include "mibtbl.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/file.h>
#include <rtk/options.h>
#include <rtk/utility.h>
#ifdef EMBED
#include <config/autoconf.h>
#else
#include "../../../../config/autoconf.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <crypt.h>
#include <sys/stat.h>
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#include "ftp_server_api.h"
#endif

#define FTP_ACCOUNT_FILE	"/var/passwd.ftp"
#define SMBD_ACCOUNT_FILE	"/var/passwd.smb"

#ifdef CONFIG_MULTI_FTPD_ACCOUNT
/*
 * /etc/passwd.ftp format same to /etc/passwd
 */
void ftpd_account_change(void)
{
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
	smartHGU_ftpserver_account_update();
#else
	FILE *fp;
	MIB_CE_FTP_ACCOUNT_T entry;
	int mibtotal=0, i;
	char userPass[MAX_PASSWD_LEN];
	char *xpass;
	unsigned char anonymous_exist=0;
#ifdef FTP_ACCOUNT_INDEPENDENT
	char ftp_user[MAX_NAME_LEN];
	char ftp_pass[MAX_PASSWD_LEN];
#endif
	int ret=-1;
	char salt[128];
	rtk_get_crypt_salt(salt, sizeof(salt));

	fp = fopen(FTP_ACCOUNT_FILE, "w");
	if (!fp)
		return;

	mibtotal = mib_chain_total(MIB_FTP_ACCOUNT_TBL);
	for (i=0; i<mibtotal; i++)
	{
		if (!mib_chain_get(MIB_FTP_ACCOUNT_TBL, i, (void *)&entry))
			continue;

		if (!entry.username[0])//account is invalid
			continue;
		
		memset(userPass, 0, sizeof(userPass));
		memcpy(userPass, entry.password, MAX_PASSWD_LEN);

		xpass = crypt(userPass, salt);
		
		if (xpass)
			fprintf(fp, "%s:%s:0:0::%s:%s\n", entry.username, xpass, PW_HOME_DIR, PW_CMD_SHELL);
	}
#ifdef FTP_ACCOUNT_INDEPENDENT
	mib_get_s(MIB_FTP_USER, ftp_user, sizeof(ftp_user));
	mib_get_s(MIB_FTP_PASSWD, ftp_pass, sizeof(ftp_pass));
	if(ftp_user[0] && ftp_pass[0])
	{
		memset(userPass, 0, sizeof(userPass));
		memcpy(userPass, entry.password, MAX_PASSWD_LEN);

		xpass = crypt(userPass, salt);
		
		if (xpass)
			fprintf(fp, "%s:%s:0:0::%s:%s\n", ftp_user, xpass, PW_HOME_DIR, PW_CMD_SHELL);
	}
#endif

	mib_get_s(MIB_FTP_ANNONYMOUS, (void *)&anonymous_exist, sizeof(anonymous_exist));
	if (anonymous_exist) {
		xpass = crypt("~!@#$%^&*()_+abcdefg", salt);
		if (xpass)
			fprintf(fp, "%s:%s:0:0::%s:%s\n", "anonymous", xpass, PW_HOME_DIR, PW_CMD_SHELL);
	}
	
	fclose(fp);
	ret=chmod(PW_HOME_DIR, 0x1fd);	// let owner and group have write access
	if(ret)
		printf("chmod error!\n");
#endif
}
#endif//CONFIG_MULTI_FTPD_ACCOUNT

#ifdef CONFIG_MULTI_FTPD_ACCOUNT
#if !defined(CONFIG_YUEME) && !defined(CONFIG_CU_BASEON_YUEME)
/*
 * FUNCTION : enable_ftpd
 * AUTHOR   : ql_xu
 * DATE     : 20151209
 * PARAMETER: enable   0: close
 *                     1: local open
 *                     2: remote open
 *                     1: both local and remote open
 * RETURN   : -1 - mib chain get fail
 *            -2 - unknown parameter
 *             0 - ftpd not support
 *             1 - success
 */
int enable_ftpd(int enable)
{
#ifdef FTP_ACCOUNT_INDEPENDENT
	setFtpAccessFw(enable);
#endif

#if defined(CONFIG_USER_FTPD_FTPD)
#if defined( REMOTE_ACCESS_CTL)
	MIB_CE_ACC_T accEntry;

	if (!mib_chain_get(MIB_ACC_TBL, 0, (void *)&accEntry))
		return -1;

	switch (enable)
	{
	case 0:
		accEntry.ftp = 0;
		break;
	case 1:
		accEntry.ftp = 0x2;
		break;
	case 2:
		accEntry.ftp = 0x1;
		break;
	case 3:
		accEntry.ftp = 0x3;
		break;
	default:
		return -2;
	}

	remote_access_modify( accEntry, 0 );
	remote_access_modify( accEntry, 1 );

	if (!mib_chain_update(MIB_ACC_TBL, (void *)&accEntry, 0)){
		return -1;
	}
	
	return 1;
#elif defined(IP_ACL)
	//CMCC is black list via default
	MIB_CE_ACL_IP_T Entry_lan, Entry_wan;
	if (!mib_chain_get(MIB_ACL_IP_TBL, 0, (void *)&Entry_lan))
		return -1;
	if (!mib_chain_get(MIB_ACL_IP_TBL, 1, (void *)&Entry_wan))
		return -1;
	switch (enable)
	{
	case 0:
		Entry_lan.ftp = 2;
		Entry_wan.ftp = 1;
		break;
	case 1:
		Entry_lan.ftp = 0;
		Entry_wan.ftp = 1;
		break;
	case 2:
		Entry_lan.ftp = 2;
		Entry_wan.ftp = 0;
		break;
	case 3:
		Entry_lan.ftp = 0;
		Entry_wan.ftp = 0;
		break;
	default:
		return -2;
	}
	if (!mib_chain_update(MIB_ACL_IP_TBL, (void *)&Entry_lan, 0)){
		return -1;
	}
	if (!mib_chain_update(MIB_ACL_IP_TBL, (void *)&Entry_wan, 1)){
		return -1;
	}
	restart_acl();
	return 1;
#else
	return 0;
#endif
#else
	return 0;
#endif
}

/*
 * FUNCTION : get_ftpd_capability
 * AUTHOR   : ql_xu
 * DATE     : 20151209
 * PARAMETER: none
 * RETURN   : 0 - disabled
 *            1 - local access enabled
 *            2 - remote access enabled
 *            3 - both local and remote access enabled
 *           -1 - failed to get ftpd capability
 */
int get_ftpd_capability(void)
{
#if defined(CONFIG_USER_FTPD_FTPD)
#if defined( REMOTE_ACCESS_CTL)
	MIB_CE_ACC_T accEntry;

	if (!mib_chain_get(MIB_ACC_TBL, 0, (void *)&accEntry))
		return -1;

	if ((accEntry.ftp & 0x3) == 0)
		return 0;
	if ((accEntry.ftp & 0x3) == 0x1)
		return 2;
	if ((accEntry.ftp & 0x3) == 0x2)
		return 1;
	if ((accEntry.ftp & 0x3) == 0x3)
		return 3;
#elif defined(IP_ACL)
	MIB_CE_ACL_IP_T Entry_lan, Entry_wan;
	if (!mib_chain_get(MIB_ACL_IP_TBL, 0, (void *)&Entry_lan))
		return -1;
	if (!mib_chain_get(MIB_ACL_IP_TBL, 1, (void *)&Entry_wan))
		return -1;
	if (((Entry_lan.ftp & 0x3) == 2) && ((Entry_wan.ftp & 0x3) == 1))
		return 0;
	if (((Entry_lan.ftp & 0x2) == 0) && ((Entry_wan.ftp & 0x1) == 0))
		return 3;
	if (((Entry_lan.ftp & 0x2) == 2) && ((Entry_wan.ftp & 0x1) == 0))
		return 2;
	if (((Entry_lan.ftp & 0x2) == 0) && ((Entry_wan.ftp & 0x1) == 1))
		return 1;
	return -1;
#else
	return -1;
#endif
#else
	return 0;
#endif
}

/*
 * FUNCTION : enable_anonymous_ftpd_account
 * AUTHOR   : ql_xu
 * DATE     : 20151209
 * PARAMETER: enable  0: not allow    1: allow
 * RETURN   : 0 - fail
 *            1 - success
 */
int enable_anonymous_ftpd_account(int enable)
{
	unsigned char vChar;

	vChar = !!enable;
	if (!mib_set(MIB_FTP_ANNONYMOUS, (void *)&vChar))
		return 0;

	ftpd_account_change();
	
	return 1;
}

/*
 * FUNCTION : add_ftpd_account
 * AUTHOR   : ql_xu
 * DATE     : 20151209
 * PARAMETER: username/passwd
 * RETURN   : 0 - fail
 *            1 - success
 */
int add_ftpd_account(char *username, char *passwd)
{
	MIB_CE_FTP_ACCOUNT_T entry;
	int mibtotal=0, i;

	if (!username || !passwd)//username should not be NULL
		return 0;
	
	mibtotal = mib_chain_total(MIB_FTP_ACCOUNT_TBL);
	for (i=0; i<mibtotal; i++)
	{
		if (!mib_chain_get(MIB_FTP_ACCOUNT_TBL, i, (void *)&entry))
			continue;

		if (!strcmp(entry.username, username))//account already exist
			return 0;
	}

	memset(&entry, 0, sizeof(entry));
	snprintf(entry.username, MAX_NAME_LEN, "%s", username);
	snprintf(entry.password, MAX_PASSWD_LEN, "%s", passwd);

	if (!mib_chain_add(MIB_FTP_ACCOUNT_TBL, (void *)&entry))
		return 0;

	ftpd_account_change();

	return 1;
}

/*
 * FUNCTION : clear_ftpd_account
 * AUTHOR   : ql_xu
 * DATE     : 20151209
 * PARAMETER: none
 * RETURN   : 0 - fail
 *            1 - success
 */
int clear_ftpd_account(void)
{
	if (!mib_chain_clear(MIB_FTP_ACCOUNT_TBL))
		return 0;

	ftpd_account_change();
	
	return 1;
}
#endif
#endif//end of CONFIG_MULTI_FTPD_ACCOUNT

#if defined(CONFIG_MULTI_SMBD_ACCOUNT)
void smbd_account_change(void)
{
#if defined(CONFIG_YUEME)  || defined(CONFIG_CU_BASEON_YUEME)
	FILE *fp;
	MIB_CE_SMB_ACCOUNT_T entry;
	int mibtotal=0, i;
	char userPass[MAX_PASSWD_LEN];
	char *xpass;
	unsigned char anonymous_exist=0;
	char cmdStr[100];
	FILE *fp_pass;

	/* 1. create passwd for smbd */
	fp = fopen(SMBD_ACCOUNT_FILE, "w");
	if (!fp)
		return;
	
	mibtotal = mib_chain_total(MIB_SMBD_ACCOUNT_TBL);
	for (i=0; i<mibtotal; i++)
	{
		if (!mib_chain_get(MIB_SMBD_ACCOUNT_TBL, i, (void *)&entry))
			continue;

		if (!entry.username[0])//account is invalid
			continue;
		
		memset(userPass, 0, sizeof(userPass));
		memcpy(userPass, entry.password, MAX_PASSWD_LEN);

		char salt[128];
		rtk_get_crypt_salt(salt, sizeof(salt));
		xpass = crypt(userPass, salt);
		
		if (xpass)
			fprintf(fp, "%s:%s:0:0::%s:%s\n", entry.username, xpass, PW_HOME_DIR, PW_CMD_SHELL);
	}
	fprintf(fp, "%s:%s:0:0::/tmp:/dev/null\n", "nobody", "x");
	fclose(fp);

	/* 2. create /etc/smb.conf */
	fp = fopen(SMB_CONF_FILE, "w");
	if (!fp)
		return;

	mib_get_s(MIB_SMB_ANNONYMOUS, (void *)&anonymous_exist, sizeof(anonymous_exist));
	fprintf(fp, "#\n");
	fprintf(fp, "# Realtek Semiconductor Corp.\n");
	fprintf(fp, "#\n");
	fprintf(fp, "# Tony Wu (tonywu@realtek.com)\n");
	fprintf(fp, "# Jan. 10, 2011\n");
	fprintf(fp, "\n");
	fprintf(fp, "[global]\n");
	fprintf(fp, "\t# netbios name = Realtek\n");
	fprintf(fp, "\t# server string = Realtek Samba Server\n");
	fprintf(fp, "\tsyslog = 1\n");
	fprintf(fp, "\t# server signing = required\n");
	fprintf(fp, "\tsocket options = TCP_NODELAY  SO_RCVBUF=131072 SO_SNDBUF=131072 IPTOS_LOWDELAY\n");
	fprintf(fp, "\t# unix charset = ISO-8859-1\n");
	fprintf(fp, "\t# preferred master = no\n");
	fprintf(fp, "\t# domain master = no\n");
	fprintf(fp, "\t# local master = yes\n");
	fprintf(fp, "\t# os level = 20\n");
	fprintf(fp, "\tdisable netbios = yes\n");
	fprintf(fp, "\tsmb ports = 445\n");
#if defined(CONFIG_CMCC)||defined(CONFIG_CU_BASEON_CMCC)
	if (!anonymous_exist)
		fprintf(fp, "\tsecurity = user\n");
	else
		fprintf(fp, "\tsecurity = share\n");
	fprintf(fp, "\tencrypt passwords = true\n");
	fprintf(fp, "\tpassdb backend = smbpasswd\n");
	fprintf(fp, "\tsmb passwd file = %s\n", SMB_PWD_FILE);
	fprintf(fp, "\tguest account = nobody\n");
#else
	fprintf(fp, "\tsecurity = share\n");
	fprintf(fp, "\t# guest account = admin\n");
#endif
	fprintf(fp, "\tdeadtime = 15\n");
	fprintf(fp, "\tstrict sync = no\n");
	fprintf(fp, "\tsync always = no\n");
	fprintf(fp, "\tdns proxy = no\n");
#ifdef CONFIG_CMCC
	fprintf(fp, "\tbind interfaces only = yes\n");
	fprintf(fp, "\tinterfaces = lo, br0\n");
#endif
	if (!anonymous_exist)
		fprintf(fp, "\tusershare allow guests = no\n");
	else		
		fprintf(fp, "\tusershare allow guests = yes\n");
	fprintf(fp, "\tuse sendfile = true\n");
	fprintf(fp, "\twrite cache size = 8192000\n");
	fprintf(fp, "\tmin receivefile size = 16384\n");
	fprintf(fp, "\taio read size = 131072\n");
	fprintf(fp, "\taio write size = 131072\n");
	fprintf(fp, "\taio write behind = true\n");
	fprintf(fp, "\tread raw = yes\n");
	fprintf(fp, "\twrite raw = yes\n");
	fprintf(fp, "\tgetwd cache = yes\n");
	fprintf(fp, "\toplocks = yes\n");
	fprintf(fp, "\tmax xmit = 32768\n");
	fprintf(fp, "\tlarge readwrite = yes\n");

	fprintf(fp, "\n");
	
	if (anonymous_exist)
	{
#if defined(CONFIG_CMCC)||defined(CONFIG_CU)
		fprintf(fp, "[usbshare]\n");
#else
		fprintf(fp, "[mnt]\n");
#endif
		fprintf(fp, "\tcomment = File Server\n");
		fprintf(fp, "\tpath = /mnt\n");
		fprintf(fp, "\tpublic = yes\n");
		fprintf(fp, "\twritable = yes\n");
		fprintf(fp, "\tprintable = no\n");
		fprintf(fp, "\tcreate mask = 0644\n");
		fprintf(fp, "\tguest ok = yes\n");
	}
	fprintf(fp, "\n");
	for (i=0; i<mibtotal; i++)
	{
		if (!mib_chain_get(MIB_SMBD_ACCOUNT_TBL, i, (void *)&entry))
			continue;
		
		if (!entry.username[0])//account is invalid
			continue;
			
		fprintf(fp, "[%s]\n", entry.username);
		fprintf(fp, "\tcomment = %s\n", entry.username);
		fprintf(fp, "\tpath = %s\n", entry.path[0]!=0 ? entry.path:"/mnt");
		fprintf(fp, "\tvalid users = %s\n", entry.username);
		fprintf(fp, "\tpublic = yes\n");
		fprintf(fp, "\twritable = %s\n", entry.writable==1 ? "yes":"no");
		fprintf(fp, "\tprintable = no\n");
		fprintf(fp, "\tguest ok = no\n");
		fprintf(fp, "\n");
	}
	fclose(fp);

	/* 3. create passwd file, /etc/samba/smbpasswd */
	fp = fopen(SMB_PWD_FILE, "w");
	if (!fp)
		return;
	fprintf(fp, "nobody:0:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:[UD         ]:LCT-00000000:nobody\n");
	fclose(fp);

	mibtotal = mib_chain_total(MIB_SMBD_ACCOUNT_TBL);
	for (i=0; i<mibtotal; i++)
	{
		if (!mib_chain_get(MIB_SMBD_ACCOUNT_TBL, i, (void *)&entry))
			continue;

		if (!entry.username[0])//account is invalid
			continue;
		
		//snprintf(cmdStr, 100, "smbpasswd %s %s", entry.username, entry.password);
		//add by zhaox
		printf("*******valid smbpasswd -a %s -s %s ****\n", entry.username, entry.password);
		fp_pass = fopen("/tmp/.pass", "w");
		if(fp_pass == NULL )
		{
			return;
		}
		fprintf(fp_pass, "%s\n",entry.password);
		fprintf(fp_pass, "%s\n",entry.password);
		fclose(fp_pass);
		snprintf(cmdStr, 100, "cat /tmp/.pass | smbpasswd -a %s -s", entry.username);

		va_cmd("/bin/sh", 2, 1, "-c", cmdStr);
	}
#else //defined(CONFIG_YUEME)  || defined(CONFIG_CU_BASEON_YUEME)

	//restart samba
	system("sysconf send_unix_sock_message /var/run/systemd.usock do_update_user_account");
	system("sysconf send_unix_sock_message /var/run/systemd.usock do_restart_samba");

#endif //defined(CONFIG_YUEME)  || defined(CONFIG_CU_BASEON_YUEME)
}


/*
 * FUNCTION : enable_smbd
 * AUTHOR   : ql_xu
 * DATE     : 20151210
 * PARAMETER: enable   0: close
 *                     1: local open
 *                     2: remote open
 *                     1: both local and remote open
 * RETURN   : -1 - mib chain get fail
 *            -2 - unknown parameter
 *             0 - smbd not support
 *             1 - success
 */
int enable_smbd(int enable)
{
#if defined(CONFIG_USER_SAMBA)
#if defined(CONFIG_YUEME)  || defined(CONFIG_CU_BASEON_YUEME)
#if defined(REMOTE_ACCESS_CTL)

	MIB_CE_ACC_T accEntry;

	if (!mib_chain_get(MIB_ACC_TBL, 0, (void *)&accEntry))
		return -1;

	switch (enable)
	{
	case 0:
		accEntry.smb = 0;
		break;
	case 1:
		accEntry.smb = 0x2;
		break;
	case 2:
		accEntry.smb = 0x1;
		break;
	case 3:
		accEntry.smb = 0x3;
		break;
	default:
		return -2;
	}

	remote_access_modify( accEntry, 0 );
	remote_access_modify( accEntry, 1 );

	if (!mib_chain_update(MIB_ACC_TBL, (void *)&accEntry, 0)){
		return -1;
	}
	return 1;
#elif defined(IP_ACL)
	//CMCC is black list via default
	MIB_CE_ACL_IP_T Entry_lan, Entry_wan;
	if (!mib_chain_get(MIB_ACL_IP_TBL, 0, (void *)&Entry_lan))
		return -1;
	if (!mib_chain_get(MIB_ACL_IP_TBL, 1, (void *)&Entry_wan))
		return -1;
	switch (enable)
	{
	case 0:
		Entry_lan.smb = 2;
		Entry_wan.smb = 1;
		break;
	case 1:
		Entry_lan.smb = 0;
		Entry_wan.smb = 1;
		break;
	case 2:
		Entry_lan.smb = 2;
		Entry_wan.smb = 0;
		break;
	case 3:
		Entry_lan.smb = 0;
		Entry_wan.smb = 0;
		break;
	default:
		return -2;
	}
	if (!mib_chain_update(MIB_ACL_IP_TBL, (void *)&Entry_lan, 0)){
		return -1;
	}
	if (!mib_chain_update(MIB_ACL_IP_TBL, (void *)&Entry_wan, 1)){
		return -1;
	}
	restart_acl();
	return 1;
#else
	return 0;
#endif
#else //#if defined(CONFIG_YUEME)  || defined(CONFIG_CU_BASEON_YUEME)
	unsigned char samba_enable = enable;

	mib_set(MIB_SAMBA_ENABLE, &samba_enable);
	setSambaAccessFw(samba_enable);
	return 1;
#endif//#if defined(CONFIG_YUEME)  || defined(CONFIG_CU_BASEON_YUEME)
#else
	return 0;
#endif //#if defined(CONFIG_USER_SAMBA)
}

/*
 * FUNCTION : get_smbd_capability
 * AUTHOR   : ql_xu
 * DATE     : 20151210
 * PARAMETER: none
 * RETURN   : 0 - disabled
 *            1 - local access enabled
 *            2 - remote access enabled
 *            3 - both local and remote access enabled
 *           -1 - failed to get smbd capability
 */
int get_smbd_capability(void)
{
#if defined(CONFIG_USER_SAMBA)
#if defined(CONFIG_YUEME)  || defined(CONFIG_CU_BASEON_YUEME)
#if defined(REMOTE_ACCESS_CTL)
	MIB_CE_ACC_T accEntry;

	if (!mib_chain_get(MIB_ACC_TBL, 0, (void *)&accEntry))
		return -1;

	if ((accEntry.smb & 0x3) == 0)
		return 0;
	if ((accEntry.smb & 0x3) == 0x1)
		return 2;
	if ((accEntry.smb & 0x3) == 0x2)
		return 1;
	if ((accEntry.smb & 0x3) == 0x3)
		return 3;
#elif defined(IP_ACL)
	MIB_CE_ACL_IP_T Entry_lan, Entry_wan;
	if (!mib_chain_get(MIB_ACL_IP_TBL, 0, (void *)&Entry_lan))
		return -1;
	if (!mib_chain_get(MIB_ACL_IP_TBL, 1, (void *)&Entry_wan))
		return -1;
	if (((Entry_lan.smb & 0x3) == 2) && ((Entry_wan.smb & 0x3) == 1))
		return 0;
	if (((Entry_lan.smb & 0x2) == 0) && ((Entry_wan.smb & 0x1) == 0))
		return 3;
	if (((Entry_lan.smb & 0x2) == 2) && ((Entry_wan.smb & 0x1) == 0))
		return 2;
	if (((Entry_lan.smb & 0x2) == 0) && ((Entry_wan.smb & 0x1) == 1))
		return 1;
	return -1;
#else
	return -1;
#endif
#else //#if defined(CONFIG_YUEME)  || defined(CONFIG_CU_BASEON_YUEME)
	unsigned char samba_enable = 0;

	mib_get_s(MIB_SAMBA_ENABLE, &samba_enable, sizeof(samba_enable));
	return (int)samba_enable;
#endif //#if defined(CONFIG_YUEME)  || defined(CONFIG_CU_BASEON_YUEME)
#else
	return 0;
#endif //#if defined(CONFIG_USER_SAMBA)
}

/*
 * FUNCTION : enable_anonymous_smbd_account
 * AUTHOR   : ql_xu
 * DATE     : 20151210
 * PARAMETER: enable  0: not allow    1: allow
 * RETURN   : 0 - fail
 *            1 - success
 */
int enable_anonymous_smbd_account(int enable)
{
	unsigned char vChar;

	vChar = !!enable;
	if (!mib_set(MIB_SMB_ANNONYMOUS, (void *)&vChar))
		return 0;

	smbd_account_change();
	
	return 1;
}

/*
 * FUNCTION : add_smbd_account
 * AUTHOR   : ql_xu
 * DATE     : 20151210
 * PARAMETER: username/passwd
 * RETURN   : 0 - fail
 *            1 - success
 */
int add_smbd_account(char *username, char *passwd, char* path, unsigned char writable)
{
	MIB_CE_SMB_ACCOUNT_T entry;
	int mibtotal=0, i;

	if (!username || !passwd)//username should not be NULL
		return 0;
	
	mibtotal = mib_chain_total(MIB_SMBD_ACCOUNT_TBL);
	for (i=0; i<mibtotal; i++)
	{
		if (!mib_chain_get(MIB_SMBD_ACCOUNT_TBL, i, (void *)&entry))
			continue;

		if (!strcmp(entry.username, username))//account already exist
			return -1;
	}

	memset(&entry, 0, sizeof(entry));
	snprintf(entry.username, MAX_SMB_NAME_LEN, "%s", username);
	snprintf(entry.password, MAX_PASSWD_LEN, "%s", passwd);
	if(path != NULL){
		snprintf(entry.path, 256, "%s", path);
	}
	entry.writable = writable;

	if (!mib_chain_add(MIB_SMBD_ACCOUNT_TBL, (void *)&entry))
		return 0;

	smbd_account_change();

	return 1;
}

/*
 * FUNCTION : rtk_del_smbd_account
 * AUTHOR   : ql_xu
 * DATE     : 20151210
 * PARAMETER: username
 * RETURN   : 0 - fail
 *            1 - success
 */
int rtk_app_del_smbd_account(char *username)
{
	MIB_CE_SMB_ACCOUNT_T entry;
	int mibtotal=0, i;

	if (!username)//username should not be NULL
		return 0;
	
	mibtotal = mib_chain_total(MIB_SMBD_ACCOUNT_TBL);
	for (i=0; i<mibtotal; i++)
	{
		if (!mib_chain_get(MIB_SMBD_ACCOUNT_TBL, i, (void *)&entry))
			continue;

		if (!strcmp(entry.username, username)){
			mib_chain_delete(MIB_SMBD_ACCOUNT_TBL, i);
		}
	}

	smbd_account_change();

	return 1;
}

/*
 * FUNCTION : clear_smbd_account
 * AUTHOR   : ql_xu
 * DATE     : 20151210
 * PARAMETER: none
 * RETURN   : 0 - fail
 *            1 - success
 */
int clear_smbd_account(void)
{
	if (!mib_chain_clear(MIB_SMBD_ACCOUNT_TBL))
		return 0;
	
	smbd_account_change();

	return 1;
}
#endif//end of CONFIG_MULTI_SMBD_ACCOUNT

/*
 * FUNCTION : enable_httpd
 * AUTHOR   : ql_xu
 * DATE     : 20151214
 * PARAMETER: enable   0: close
 *                     1: local open
 *                     2: remote open
 *                     1: both local and remote open
 * RETURN   : -1 - mib chain get fail
 *            -2 - unknown parameter
 *             0 - smbd not support
 *             1 - success
 */
int enable_httpd(int enable)
{
#if defined(REMOTE_ACCESS_CTL)

	MIB_CE_ACC_T accEntry;

	if (!mib_chain_get(MIB_ACC_TBL, 0, (void *)&accEntry))
		return -1;

	switch (enable)
	{
	case 0:
		accEntry.web = 0;
		break;
	case 1:
		accEntry.web = 0x2;
		break;
	case 2:
		accEntry.web = 0x1;
		break;
	case 3:
		accEntry.web = 0x3;
		break;
	default:
		return -2;
	}

	remote_access_modify( accEntry, 0 );
	remote_access_modify( accEntry, 1 );

	if (!mib_chain_update(MIB_ACC_TBL, (void *)&accEntry, 0)){
		return -1;
	}
#elif defined(IP_ACL)
	//CMCC is black list via default
	MIB_CE_ACL_IP_T Entry_lan, Entry_wan;
	if (!mib_chain_get(MIB_ACL_IP_TBL, 0, (void *)&Entry_lan))
		return -1;
	if (!mib_chain_get(MIB_ACL_IP_TBL, 1, (void *)&Entry_wan))
		return -1;
	switch (enable)
	{
	case 0:
		Entry_lan.web = 2;
		Entry_wan.web = 1;
		break;
	case 1:
		Entry_lan.web = 0;
		Entry_wan.web = 1;
		break;
	case 2:
		Entry_lan.web = 2;
		Entry_wan.web = 0;
		break;
	case 3:
		Entry_lan.web = 0;
		Entry_wan.web = 0;
		break;
	default:
		return -2;
	}
	if (!mib_chain_update(MIB_ACL_IP_TBL, (void *)&Entry_lan, 0)){
		return -1;
	}
	if (!mib_chain_update(MIB_ACL_IP_TBL, (void *)&Entry_wan, 1)){
		return -1;
	}
	restart_acl();
#else
#endif
	return 1;
}

/*
 * FUNCTION : set_httpd_password
 * AUTHOR   : ql_xu
 * DATE     : 20151214
 * PARAMETER: username/passwd
 * RETURN   : 0 - fail
 *            1 - success
 */
int set_httpd_password(char *passwd)
{
	if (!passwd)//password should not be NULL
		return 0;
	
	mib_set(MIB_USER_PASSWORD, (void *)passwd);

	return 1;
}

int get_httpd_capability(void)
{
#if defined(REMOTE_ACCESS_CTL)
	MIB_CE_ACC_T accEntry;
	if (!mib_chain_get(MIB_ACC_TBL, 0, (void *)&accEntry))
		return -1;

	if ((accEntry.web & 0x3) == 0)
		return 0;
	if ((accEntry.web & 0x3) == 0x1)
		return 2;
	if ((accEntry.web & 0x3) == 0x2)
		return 1;
	if ((accEntry.web & 0x3) == 0x3)
		return 3;
#elif defined(IP_ACL)
	MIB_CE_ACL_IP_T Entry_lan, Entry_wan;
	if (!mib_chain_get(MIB_ACL_IP_TBL, 0, (void *)&Entry_lan))
		return -1;
	if (!mib_chain_get(MIB_ACL_IP_TBL, 1, (void *)&Entry_wan))
		return -1;
	if (((Entry_lan.web & 0x3) == 2) && ((Entry_wan.web & 0x3) == 1))
		return 0;
	if (((Entry_lan.web & 0x2) == 0) && ((Entry_wan.web & 0x1) == 0))
		return 3;
	if (((Entry_lan.web & 0x2) == 2) && ((Entry_wan.web & 0x1) == 0))
		return 2;
	if (((Entry_lan.web & 0x2) == 0) && ((Entry_wan.web & 0x1) == 1))
		return 1;
#else
	return -1;
#endif
}


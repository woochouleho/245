#include <string.h>
#include <stdio.h>
#include "web_psd_encrypt.h"
#include "md5.h"




static void s2hex(const unsigned char *src, char *dst, int len) {
	int i;
	for (i=0; i < len; i++) {
		sprintf(dst, "%02x", src[i]);
		dst += 2;
	}
	*dst = 0;
}

void MD5_da_calc_HA1(struct MD5Context *pCtx, char *userid, char *realm, char *passwd, char HA1hex[33])
{

	char HA1[16];
	
	MD5Init(pCtx);
	MD5Update(pCtx, userid, strlen(userid));
	MD5Update(pCtx, ":", 1);
	MD5Update(pCtx, realm, strlen(realm));
 	MD5Update(pCtx, ":", 1);
	MD5Update(pCtx, passwd, strlen(passwd));
 	MD5Final(HA1, pCtx);

	s2hex(HA1, HA1hex, 16);
	
	//printf("HA1: MD5(%s:%s:%s)=%s\n", userid, realm, passwd, HA1hex);
	
}

void web_auth_digest_encrypt(char *userid, char *realm, char *passwd, char *HA1hex)
{
	struct MD5Context md5ctx;
	MD5_da_calc_HA1(&md5ctx, userid, realm, passwd, HA1hex);

}

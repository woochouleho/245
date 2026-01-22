
#ifndef WEB_PSD_ENCYPT_H
#define WEB_PSD_ENCYPT_H




//void MD5_da_calc_HA1(struct MD5Context *pCtx, char *userid, char *realm, char *passwd, char HA1hex[33]);
void web_auth_digest_encrypt(char *userid, char *realm, char *passwd, char *HA1hex);

#endif

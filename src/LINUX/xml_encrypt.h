/*
 * =====================================================================================
 *
 *       Filename:  xml_encrypt.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  廿十七年八月卅日 廿時十四分50秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  C. Y. Yang (), 
 *        Company:  
 *
 * =====================================================================================
 */
#ifndef XML_ENCRYPT_H__
#define XML_ENCRYPT_H__

int xml_fprintf(FILE *stream, const char *fmt, ...);
void do_decrypt(char *buf, ssize_t buf_len);
void do_encrypt(char *buf, ssize_t buf_len);
#endif


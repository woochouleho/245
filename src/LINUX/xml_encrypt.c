/*
 * =====================================================================================
 *
 *       Filename:  xml_encrypt.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  C. Y. Yang (),
 *        Company:
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <config/autoconf.h>

#include "xml_encrypt.h"
#ifndef CONFIG_XML_ENABLE_ENCRYPT
#define CONFIG_XML_ENABLE_ENCRYPT 0
#endif

typedef struct rtk_xml_data {
	ssize_t buf_len;
	ssize_t data_len;
	char *buf;
}rtk_xml_data;

typedef struct rtk_xml_encrypt_type {
	void (*init)(void) ;
	void (*complete)(void) ;
	rtk_xml_data* (*encrypt)(char *from, ssize_t buf_len);
	rtk_xml_data* (*decrypt)(char *from, ssize_t buf_len);
}rtk_xml_encrypt_type;

static rtk_xml_data xml_buf = {0, 0, (void*)0};

static int
realloc_xml_data(rtk_xml_data *data_buf, ssize_t new_size)
{
	if ((void*)0 == data_buf->buf) {
		data_buf->buf = (char*)malloc(new_size*sizeof(char));
		if (NULL == data_buf->buf) {
			return -1;
		} else {
			data_buf->buf_len = new_size;
			return 0;
		}
	}
	if (new_size > data_buf->buf_len) {
		data_buf->buf = realloc(data_buf->buf, new_size);
		if (data_buf->buf) {
			data_buf->buf_len = new_size;
			return 0;
		} else {
			return -1;
		}
	}
	return 0;
}

static void
free_xml_data(rtk_xml_data *data_buf)
{
	if (NULL != data_buf->buf) {
		free(data_buf->buf);
		data_buf->buf = NULL;
	}
	data_buf->data_len = 0;
	data_buf->buf_len = 0;
}

static rtk_xml_data*
str_encrypt(char *from, ssize_t buf_len)
{
	if (realloc_xml_data(&xml_buf, buf_len+1)) {
		return NULL;
	}
	for ( ssize_t i = 0; i < buf_len; i++) {
		xml_buf.buf[i] = from[i] + 1;
	}
	return &xml_buf;
}

void
str_encrypt_init(void)
{
}

void
str_encrypt_complete(void)
{
}



static rtk_xml_data*
str_decrypt( char *from, ssize_t buf_len)
{
	if (realloc_xml_data(&xml_buf, buf_len+1)) {
		return NULL;
	}
	for ( ssize_t i = 0; i < buf_len; i++) {
		xml_buf.buf[i] = from[i] - 1;
	}
	return &xml_buf;;
}

rtk_xml_encrypt_type xml_encrypt = {
	.init = str_encrypt_init,
	.complete = str_encrypt_complete,
	.encrypt = str_encrypt,
	.decrypt = str_decrypt,
};

void
xml_output_init(void)
{
	xml_encrypt.init();
}

void
xml_output_complete(void)
{
	xml_encrypt.complete();
}

int
xml_fprintf(FILE *stream, const char *fmt, ...)
{
	int out_size;
#if CONFIG_XML_ENABLE_ENCRYPT
	int encrypt_flag = true;
#else
	int encrypt_flag = false;
#endif
	va_list args;
	if (stdout == stream || stderr == stream) {
		encrypt_flag = false;
	}

	va_start(args, fmt);
	if (!encrypt_flag) {
		vfprintf(stream, fmt, args);
	} else {
		if (0 == xml_buf.buf_len) {
			realloc_xml_data(&xml_buf, 20);
		}
		out_size = vsnprintf(xml_buf.buf, xml_buf.buf_len, fmt, args);
		while (out_size >= xml_buf.buf_len) {
			realloc_xml_data(&xml_buf, ((xml_buf.buf_len)<<1));
			out_size = vsnprintf(xml_buf.buf, xml_buf.buf_len, fmt, args);
		}
		xml_buf.data_len = out_size;
		xml_encrypt.encrypt (xml_buf.buf, xml_buf.data_len);
		fwrite(xml_buf.buf , sizeof(xml_buf.buf[0]), xml_buf.data_len, stream);
	}
	va_end(args);
	return 0;
}

#define XML_CONFIG_NAME "Config"

void
do_decrypt(char *buf, ssize_t buf_len)
{
#if CONFIG_XML_ENABLE_ENCRYPT
	realloc_xml_data(&xml_buf, buf_len);
	xml_encrypt.decrypt(buf, buf_len);
	if (NULL == strstr(xml_buf.buf, XML_CONFIG_NAME)) {
		// ignore the decryption
		return;
	}
	memcpy( buf, xml_buf.buf, buf_len);
#else
	if (NULL == strstr(buf, XML_CONFIG_NAME)) {
		xml_encrypt.decrypt(buf, buf_len);
		if (NULL == strstr(xml_buf.buf, XML_CONFIG_NAME)) {
			// ignore the decryption
			return;
		}
		memcpy( buf, xml_buf.buf, buf_len);
	} else {
	}
	return;
#endif
}
#undef XML_CONFIG_NAME

void
do_encrypt(char *buf, ssize_t buf_len)
{
	realloc_xml_data(&xml_buf, buf_len);
	xml_encrypt.encrypt(buf, buf_len);
	memcpy( buf, xml_buf.buf, buf_len);
}


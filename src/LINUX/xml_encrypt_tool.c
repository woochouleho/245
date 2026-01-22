/*
 * =====================================================================================
 *
 *       Filename:  xml_encrypt_tool.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  廿十七年八月卅日 廿一時廿三分39秒
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
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "xml_encrypt.h"

char *input_file = "";
char *output_file = "";
enum XML_DO_TYPE{
	DO_XML_ENCRYPT,
	DO_XML_DECRYPT
}; //

enum XML_DO_TYPE do_type;

void
show_usage()
{
	printf("xml_config_tool -i input_file [-o output_file] [-e] [-d]\n");
}
void
param_parser(int argc, char *argv[])
{
	int c;
	while ((c = getopt(argc, argv, "i:o:ed")) != -1) {
		switch (c) {
			case 'i':
				input_file = optarg;
				break;
			case 'o':
				output_file = optarg;
				break;
			case 'e':
				do_type = DO_XML_ENCRYPT;
				break;
			case 'd':
				do_type = DO_XML_DECRYPT;
				break;
			default:
				exit(0);
				break;
		}// switch(c)
	}

	if ('\0' == input_file[0]) {
		show_usage();
		exit(0);
	}

}

static inline ssize_t
get_file_length(int fd) {
	struct stat fdstat;
	fstat(fd, &fdstat);
	ssize_t flen=fdstat.st_size;
	return flen;
}

int
main(int argc, char *argv[])
{
	int fd;
	ssize_t sz;
	char *buf;
	FILE *out;
	param_parser(argc, argv);
	//printf("input: %s, output: %s flag = %s\n", input_file, output_file, do_type == DO_XML_ENCRYPT ? "encrypt" : "decrypt");
	if ((fd = open (input_file, O_RDONLY)) < 0) {
		printf("Open file %s error!\n", input_file);
		return -1;
	}

	sz = get_file_length (fd);
	buf = (char*) malloc (sz + 3);
	if (read (fd, buf, sz) != sz) {
		printf ("read file failed.\n");
		close (fd);
		return -1;
	}
	buf[sz] = '\0';
	close(fd);
	if (do_type == DO_XML_DECRYPT) {
		do_decrypt(buf, sz);
	} else if (do_type == DO_XML_ENCRYPT) {
		do_encrypt(buf, sz);
	}

	if ('\0' != output_file[0]) {
		out = fopen (output_file, "w+");
		if (NULL == out) {
			printf("Open output file %s error!\n", output_file);
			out = fdopen (fileno (stdout), "w");
		}
	} else {
		out = fdopen (fileno (stdout), "w");
	}

	fwrite(buf, sizeof(*buf), sz, out );

	return 0;
}

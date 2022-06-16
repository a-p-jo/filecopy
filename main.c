#include <string.h> /* strcmp() */
#include <stdlib.h> /* exit(), EXIT_FAILURE */
#include <errno.h>  /* errno */
#include "bcp.h"    /* fbcp(), struct fbcp_retval, stdio.h */

#define NAME "bcp v4.1"

static size_t div_round(size_t x, size_t y)
{
	return (x + y/2)/y;
}

/* Reduce given bytes into appropriate higher unit (MiB,GiB...).
 * Return appropriate suffix and store corresponding value to fmtd.
 */
static char bytes_fmt(size_t *bytes)
{
	const char suffix[] = {'B','K','M','G','T'};
	size_t idx;
	/* Divide by 1 KiB until no integral quotient may be obtained or max index reached */
	for(idx = 0; idx < 3 && div_round(*bytes, 1024) ; *bytes = div_round(*bytes, 1024), idx++)
		;
	return suffix[idx];
}

int main(int argc, char **argv)
{
	if(argc < 2 || !strcmp("-h",argv[1]) || !strcmp("--help",argv[1])) {
		fputs(NAME" : A simple, safe, performant and reliable file copying program.\n"
			"Usage    : bcp <from> [to (optional, stdout by default)] [overwrite to? (y/n) (optional)]\n"
			"\n"
			"Explanation : Copy the contents of <from> file to [to] file if given, else to stdout.\n"
			"If [to] does not exist, create it. If it pre-exists, if 'y' is specified, continue.\n"
			"If 'n' is specified, abort. Else, ask for permission before overwriting.\n"
			"\n"
			"Example : bcp archlinux.iso /dev/sda y\n",stderr);
		exit(EXIT_FAILURE);
	}

	FILE *src = fopen(argv[1],"rb"), *dest = stdout;
	
	if(!src) {
		fprintf(stderr,"Error : Couldn't open \"%s\" to read%s%s\n",
				argv[1], errno? " : " : ".",
				errno? strerror(errno) : "");
		exit(EXIT_FAILURE);
	}

	if(argc >= 3) {
		dest = fopen(argv[2], "rb");
		if(dest) {
			if(argc >= 4) {
				if(!strcmp("y", argv[3]) || !strcmp("Y", argv[3]))
					;
				else if(!strcmp("n", argv[3]) || !strcmp("N", argv[4])) {
					fputs("\"%s\" pre-exists. Aborted.\n", stderr);
					exit(EXIT_SUCCESS);
				} else {
					ask :
					{
						fprintf(stderr,"\"%s\" pre-exists. Overwrite ? (y/n) : ", argv[2]);
						int ch = getchar();
						if(ch != 'y' && ch != 'Y') {
							fputs("Aborted.\n", stderr);
							exit(EXIT_SUCCESS);
						}
					}
				}
			} else
				goto ask;
			fclose(dest);
		}

		dest = fopen(argv[2], "wb");
		if(!dest) {
			fprintf(stderr, "Error : Couldn't open \"%s\" to write%s%s\n",
					argv[2], errno? " : " : ".",
					errno? strerror(errno) : "");
			exit(EXIT_FAILURE);
		}
	}

	struct fbcp_retval retval = fbcp(dest, src, 0, stderr);
	switch(retval.err) {
	case SUCCESS :
	{
		uintmax_t bytes = retval.bytes_processed;
		char suffix     = bytes_fmt(&bytes);
		fprintf(stderr, "%ju%c (%ju bytes) copied.\n", bytes, suffix, retval.bytes_processed);
		exit(EXIT_SUCCESS);
	}

	case SRC_SEEK_ERR :
		fprintf(stderr, "Error : Unable to seek in \"%s\"%s%s",
				argv[1], errno? " : " : ".",
				errno? strerror(errno) : "");
		exit(EXIT_FAILURE);

	case SRC_FERROR :
		fprintf(stderr, "Error : Unknown fatal error reading \"%s\" %s%s\n", 
				argv[1], errno? " : " : ".",
				errno? strerror(errno) : "");
		exit(EXIT_FAILURE);

	case DEST_FERROR :
		fprintf(stderr, "Error : Unknown fatal error writing to \"%s\" %s%s\n", 
				argv[2]? argv[2] : "stdout", errno? " : " : ".",
				errno? strerror(errno) : "");
		exit(EXIT_FAILURE);

	case BOTH_FERROR :
		fputs("Error : Unknown fatal error reading & writing.\n", stderr);
		exit(EXIT_FAILURE);
	default :
		fputs("Error : Unknown fatal error.\n", stderr);
		exit(EXIT_FAILURE);
	}
}

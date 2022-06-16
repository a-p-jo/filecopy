#include <string.h> /* strcmp() */
#include <stdlib.h> /* exit(), EXIT_FAILURE */
#include <errno.h>  /* errno */
#include <stdbool.h>
#include "filecopy.h"

#define VERSION "1.1"

typedef struct { long double nunits; char suffix; } bytes_fmt_result;

static inline bytes_fmt_result bytes_fmt(uintmax_t bytes)
{
	static const char suffix[] = {'K', 'M', 'G', 'T'};
	bytes_fmt_result res = {bytes, 'B'};
	for (
		uint_least8_t i = 0;
		i < sizeof(suffix) && res.nunits/1024 >= 1;
		res.nunits /= 1024, res.suffix = suffix[i++]
	)
		;
	return res;
}

static inline bool streq(const char *a, const char *b)
{
	return strcmp(a, b) == 0;
}

static void print_progress(uint_least8_t percentage)
{
	printf("%03u%%\r", percentage);
	fflush(stdout);
}

int main(int argc, char **argv)
{
	if (argc < 2 || streq("-h",argv[1]) || streq("--help",argv[1]))
		fprintf(
			stderr, "%s v"VERSION"\n"
			"Usage : filecopy <src> [dst (optional, stdout by default)] [overwrite dst? (y/n) (optional)]\n"
			"\n"
			"Copies the contents of src to dst (if given, else to stdout).\n"
			"If dst does not exist, creates it. If it pre-exists, if 'y' is specified, continues.\n"
			"If 'n' is specified, aborts. Else, asks for permission before overwriting.\n"
			"\n"
			"Example : %s archlinux.iso /dev/sda y\n", argv[0], argv[0]
		), exit(EXIT_FAILURE);

	FILE *src = fopen(argv[1],"rb"), *dst = stdout;
	if (!src)
		fprintf(
			stderr,"Error : Couldn't open \"%s\" to read%s%s\n",
			argv[1], errno? " : " : ".",
			errno? strerror(errno) : ""
		), exit(EXIT_FAILURE);

	if (argc >= 3) {
		if ((dst = fopen(argv[2], "rb"))) {
			if (argc >= 4) {
				if (streq("y", argv[3]) || streq("Y", argv[3]))
					;
				else if (streq("n", argv[3]) || streq("N", argv[4]))
					fprintf(stderr, "\"%s\" pre-exists. Aborted.\n", argv[2]),
					exit(EXIT_SUCCESS);
				else ask : {
					fprintf(stderr, "\"%s\" pre-exists. Overwrite ? (y/n) : ", argv[2]);
					int ch = getchar();
					if(ch != 'y' && ch != 'Y')
						fputs("Aborted.\n", stderr), exit(EXIT_SUCCESS);
				}
			} else
				goto ask;
			fclose(dst);
		}
		if (!( dst = fopen(argv[2], "wb") ))
			fprintf(
				stderr, "Error : Couldn't open \"%s\" to write%s%s\n",
				argv[2], errno? " : " : ".",
				errno? strerror(errno) : ""
			), exit(EXIT_FAILURE);
	}

	filecopy_result res = filecopy(dst, src, 0, print_progress);
	switch (res.err) {
	case filecopy_error_none : {
		bytes_fmt_result bfmt = bytes_fmt(res.bytes_copied);
		fprintf(
			stderr, "%.2Lf%c (%ju bytes) copied.\n",
			bfmt.nunits, bfmt.suffix, res.bytes_copied
		), exit(EXIT_SUCCESS);
	}
	case filecopy_error_src :
		fprintf(
			stderr, "Error : Unknown fatal error reading \"%s\" %s%s\n", 
			argv[1], errno? " : " : ".",
			errno? strerror(errno) : ""
		), exit(EXIT_FAILURE);
	case filecopy_error_dst :
		fprintf(
			stderr, "Error : Unknown fatal error writing to \"%s\" %s%s\n", 
			argv[2]? argv[2] : "stdout", errno? " : " : ".",
			errno? strerror(errno) : ""
		), exit(EXIT_FAILURE);
	}
}

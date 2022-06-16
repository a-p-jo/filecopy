#if defined _WIN32
	#define filecopy_ftell _ftelli64
	#define filecopy_fseek _fseeki64
#elif defined __unix__ || defined __APPLE__
	#define _FILE_OFFSET_BITS 64 /* Ensure off_t is 64-bit */
	#define filecopy_ftell ftello
	#define filecopy_fseek fseeko
#else /* If long is 32-bit, offsets remain 32-bit capped. */
	#define filecopy_ftell ftell
	#define filecopy_fseek fseek
#endif

#include "filecopy.h"

#define IO_BUFSZ     (64*1024) /* 64 KiB                   */
#define CB_THRESHOLD (   1   ) /* Callback called every 1% */

/* Return quotient rounded to nearest whole number. */
static inline uintmax_t divround(uintmax_t x, uintmax_t y) { return (x + y/2) / y;}

typedef enum {
	filesize_error_none,
	filesize_error_unseekable, /* stream isn't a regular file, has no real "size" */
	filesize_error_spurious    /* sudden failure, stream indeterminate. */
} filesize_error;

typedef struct { uintmax_t nbytes; filesize_error err; } filesize_result;

/* Only meaningful if f is a regular file opened in binary mode */
static inline filesize_result filesize(FILE *f)
{
	intmax_t curpos = filecopy_ftell(f);
	if (curpos < 0 || filecopy_fseek(f, 0, SEEK_END) != 0)
		return (filesize_result) {.err = filesize_error_unseekable};
	
	intmax_t endpos = filecopy_ftell(f);
	int tryreset = filecopy_fseek(f, curpos, SEEK_SET);
	if (endpos < 0) /* Is spurious only if trying to reset fails. */
		return (filesize_result) {
			.err = tryreset != 0? filesize_error_spurious : filesize_error_unseekable
		};
	else if (tryreset != 0)
		return (filesize_result) {.err = filesize_error_spurious};

	return (filesize_result) {.nbytes = endpos-curpos};
}

filecopy_result filecopy(FILE *dst, FILE *src, uintmax_t nbytes, void(*cb)(uint_least8_t))
{
	/* If we have to callback to report progress, we need to know the stream's size. */
	if (cb && !nbytes) {
		filesize_result srcsz = filesize(src);
		if (srcsz.err == filesize_error_spurious) /* Stream unrecoverable */
			return (filecopy_result) {.err = filecopy_error_src};
		else
			nbytes = srcsz.nbytes;
	}
		
	filecopy_result res = {0};
	unsigned char buf[IO_BUFSZ];
	uintmax_t one_percent = cb? divround(nbytes, 100) : 0; /* If one_percent is 0, no callback */
	uint_least8_t prog = 0;

	while (nbytes? res.bytes_copied < nbytes : 1) {
		size_t toread = sizeof(buf);
		if (nbytes && nbytes < res.bytes_copied+sizeof(buf))
			toread = nbytes - res.bytes_copied; /* In last loop read only as much as left */

		if (
			fread(buf, 1, toread, src) == toread 
			&& fwrite(buf, 1, toread, dst) == toread
		) {
			res.bytes_copied += toread;
			if (one_percent) {
				uint_least8_t curprog = divround(res.bytes_copied, one_percent);
				if (curprog - prog >= CB_THRESHOLD)
					cb((prog = curprog));
			}
		} else
			break;
	}
	
	if (ferror(src))
		res.err = filecopy_error_src;
	else if (ferror(dst))
		res.err = filecopy_error_dst;
	else if (nbytes && res.bytes_copied < nbytes)
		res.err = filecopy_error_early_eof;
	return res;
}

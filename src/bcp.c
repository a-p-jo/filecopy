#include "bcp.h"

/* Reject compilation for freestanding or < C99 */
#if !defined __STDC_HOSTED__ || __STDC_VERSION__ < 199901L
	#error "Hosted C99 standard library required !"

#elif defined _WIN32
	/* Silence incorrect MSVC warnings */	
	#define _CRT_SECURE_NO_WARNINGS
	/* Inform MSVC & MinGW-W64 of target Win32 API */
	#define _WIN32_WINNT _WIN32_WINNT_WINXP

	/* Redefine ftell/fseek to 64-bit extensions */
	#define ftell _ftelli64
	#define fseek _fseeki64

#elif defined __unix__ || defined __APPLE__
	/* Ensure off_t returned by ftello/fseeko are 64-bit */
	#define _FILE_OFFSET_BITS 64
	
	/* Redefine ftell/fseek to 64-bit extensions */
	#define ftell ftello
	#define fseek fseeko

/* On non-Windows/POSIX env where long < 64-bit, ftell/fseek remain 32-bit capped */
#endif

typedef unsigned char byte;

/* Copying done in 64 KiB blocks */
enum { BUFSZ = 64 * 1024 , PRINT_PROGRESS_THRESHOLD = 1};

/* Return quotient of x/y rounded to nearest whole number */
static uintmax_t div_round(uintmax_t x, uintmax_t y)
{
	return (x + y/2)/y;
}

/* Return 0 if size = 0 or error in determining size.
 * Return -1 if of could not be reset as original.
 * Return +ve integer on success.
 */
static intmax_t fsize(FILE *of)
{
	intmax_t initpos = ftell(of);
	if(initpos < 0)
		return 0;
	else if(fseek(of, 0, SEEK_END))
		return -1;

	intmax_t size = ftell(of);
	if(size <= 0 || fseek(of, initpos, SEEK_SET))
		return 0;
	else
		return size;
}

struct fbcp_retval fbcp(FILE *dest, FILE *src, uintmax_t n, FILE *out)
{		
	/* .err = .bytes_processed = 0 */
	struct fbcp_retval retval = {0};

	if(out && !n) {
		intmax_t size = fsize(src);
		if(size > 0)
			n = size;
		/* fsize() could not restore src, abort */
		else if(size == -1) {
			retval.err = SRC_SEEK_ERR;
			return retval;
		}
	}
		
	/* Stack allocated buffer, less cache misses and overhead */
	byte buf[BUFSZ];
	size_t chunk = BUFSZ, progress = 0, cur_progress = 0;
	uintmax_t bytes_read = 0, bytes_written = 0,
		  one_percent = out? div_round(n, 100) : 0;

	/* Iterate until either an fread()/fwrite() call fails or done
	 * copying n bytes.
	 */
	loop :
	
	if(n && n < bytes_written + BUFSZ)
		chunk = n - bytes_written;
	
	bytes_read = fread(buf, 1, chunk, src);

	if(bytes_read == chunk && fwrite(buf, 1, chunk, dest) == bytes_read) {
		bytes_written += chunk;
		bytes_read = 0;

		if(one_percent > 0) {
			cur_progress = div_round(bytes_written, one_percent);
			/* Printing is expensive, only update if change >= threshold */
			if(cur_progress - progress >= PRINT_PROGRESS_THRESHOLD)
			{
				fprintf(out, "%02zu%%\r", cur_progress);
				fflush(out);
				progress = cur_progress;
			}
		}

		if(n? bytes_written < n : 1)
			goto loop;
	}
	
	/* Check streams for error */
	int err_src = ferror(src), err_dest = ferror(dest);
	if(err_src && err_dest)
		retval.err = BOTH_FERROR;
	else if(err_src)
		retval.err = SRC_FERROR;
	/* There may be leftovers, (if n % BUFSZ != 0) fwrite() */
	else if(err_dest || fwrite(buf, 1, bytes_read, dest) != bytes_read)
		retval.err = DEST_FERROR;
	else
		retval.bytes_processed = bytes_written + bytes_read;

	return retval;		
}

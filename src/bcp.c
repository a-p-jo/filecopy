#include "bcp.h" /* Exempt stdout's ftell/fseek from redefinition */

#if !defined __STDC_HOSTED__ || __STDC_VERSION__ < 199901L
	/* Reject compilation for freestanding or < C99 */
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

uintmax_t bytes_processed;

#ifdef PRINT_PROGRESS
#include <inttypes.h> /* PRIuFAST8 */
#define approx(x) ((uintmax_t) ((x) + 0.5))
#endif

/* Note :
 * C99 allows a library to not support SEEK_END on binary streams.
 * Understandable given unseekable streams (like pipes) or infinite ones (like /dev/null).
 * In such a case ,fbcp() will, at runtime, cancel printing progress.
 */
static inline intmax_t fsize(FILE *of)
{
	const intmax_t initpos = ftell(of);
	if(initpos < 0)
		return -1;

	if(fseek(of,0,SEEK_END))
		return -2;

	const intmax_t size = ftell(of);
	if(size < 0)
		return -3;
	
	if(fseek(of,initpos,SEEK_SET))
		return -4;
	
	return size;
}

enum retval fbcp(FILE *restrict from, FILE *restrict to, uintmax_t bytes)
{		
	#ifdef PRINT_PROGRESS
	/* We need net bytes to be copied to show progress % */
	if(!bytes) {
		const intmax_t size = fsize(from);
		if(size < 0) {
			if(size == -4)
				/* File position could not be reset to initial, abort */
				return FROM_SEEK_ERR;
			else
				/* bytes = 0 means progress will not be printed */
				bytes = 0;
		} else
			bytes = size;
	}
	#endif


	uint_fast8_t buf[BUFSZ/sizeof(uint_fast8_t)];
	uintmax_t bytes_read = 0, bytes_written = 0;

	{
		/* bytes written to/read from buf in 1 iteration */
		size_t chunk = BUFSZ;

		#ifdef PRINT_PROGRESS
		const uintmax_t one_percent = approx(0.01 * bytes);
		uint_fast8_t progress = 0, cur_progress = 0;
		#endif
		
		loop :
			/* If bytes limit specified, and a read of BUFSZ would exceed it,
			 * don't fill the buffer, only read enough to hit the limit. 
			 */
			if(bytes && bytes < bytes_written + BUFSZ)
				chunk = bytes - bytes_written;
			
			bytes_read = fread(buf,1,chunk,from);

			/* Iterate as long as a chunk is successfully read & written */
			if(bytes_read == chunk && fwrite(buf,1,chunk,to) == bytes_read) {
				bytes_written += chunk;
				bytes_read     = 0; 

				#ifdef PRINT_PROGRESS
				if(one_percent > 0) {
					cur_progress = bytes_written / one_percent;
					/* Only print if change in progress is >= 1%, as printing is expensive */
					if((cur_progress - progress) >= 1)
					{ 
						fprintf(PRINT_PROGRESS,"%"PRIuFAST8"%%\r", cur_progress);
						fflush(PRINT_PROGRESS);
						progress = cur_progress;
					}
				}
				#endif

				/* Reiterate if limit not reached */
				if(bytes? bytes_written < bytes : 1)
					goto loop;
			} /* else fall out of loop */
	}

	/* We broke out of loop. Could be due to failed fwrite/fread. Check ! */
	const int err_frm = ferror(from), err_to = ferror(to);
	if(err_frm && !err_to)
		return FERROR_FROM;
	else if(err_to && !err_frm)
		return FERROR_TO;
	else if(err_frm && err_to)
		return FERROR_BOTH;
	else {
		/* If no errors, check for any pending data; write it */
		if(bytes_read && fwrite(buf,1,bytes_read,to) != bytes_read)
			return FERROR_TO;
		else if(fflush(to))
			return FFLUSH_ERR;
		else {
			bytes_processed = bytes_written;
			return SUCCESS;
		}
	}		
}


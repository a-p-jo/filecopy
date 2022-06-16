#ifndef FILECOPY_H
#define FILECOPY_H
	
#include <stdint.h> /* uintmax_t */
#include <stdio.h>  /* FILE      */
	
typedef enum  {
	filecopy_error_none,      /* Successful, operation complete. */
	filecopy_error_early_eof, /* EOF on src before n bytes read, stopped. */
	filecopy_error_seek,      /* Seeking failed, src indeterminate. Only if callback !NULL && nbytes 0. */
	/* IO error, dst and src indeterminate */
	filecopy_error_read,      /* ferror(src) is true */
	filecopy_error_write      /* ferror(dst) is true */
} filecopy_error;

typedef struct { uintmax_t bytes_copied; filecopy_error err; } filecopy_result;

/* Copies n bytes to dst from src.
 * If nbytes is 0, copies src until EOF.
 *  
 * If callback is not NULL, calls it with current progress %
 * if nbytes is non-zero or src is seekable.
 */
filecopy_result filecopy(FILE *dst, FILE *src, uintmax_t nbytes, void (*callback)(uint_least8_t progress_percentage));

#endif


#ifndef FILECOPY_H
#define FILECOPY_H
	
#include <stdint.h> /* uintmax_t */
#include <stdio.h>  /* FILE      */
	
typedef enum  {
	filecopy_error_none,      /* Successful, operation complete.          */
	filecopy_error_early_eof, /* EOF on src before n bytes read, stopped. */
	/* IO error, dst and src indeterminate */
	filecopy_error_src,      /* Error reading/seeking src. */
	filecopy_error_dst       /* Error writing dst.         */
} filecopy_error;

typedef struct { uintmax_t bytes_copied; filecopy_error err; } filecopy_result;

/* Copies n bytes to dst from src.
 * If nbytes is 0, copies src until EOF.
 *  
 * If callback is not NULL, calls it with current progress %
 * if nbytes is non-zero or src is seekable.
 */
filecopy_result filecopy(
	FILE *dst, FILE *src, uintmax_t nbytes, 
	void (*callback)(uint_least8_t progress_percentage)
);

#endif


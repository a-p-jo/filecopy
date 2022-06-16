#ifndef BCP_H
#define BCP_H
	
#include <stdint.h>
#include <stdio.h>
	
enum bcp_err {
	SUCCESS,
	/* From stream was seekable to SEEK_END, but reseting to initial position failed. 
	 * Cannot occur if PRINT_PROGRESS undefined.
	 */
	SRC_SEEK_ERR,
	/* Reading the from stream failed and ferror(from) returned true. 
	 * No error in to stream.
	 */
	SRC_FERROR,
	/* Writing the to stream failed and ferror(to) returned true. 
	 * No error in from stream.
	 */
	DEST_FERROR,
	/* Reading the from stream & writing the to stream failed and ferror() returned true for both */
	BOTH_FERROR
};

struct fbcp_retval {
	enum bcp_err err;
	uintmax_t bytes_processed;
};

/* Copies at most n bytes src -> dest.
 * If n = 0, or n > fsize(src), copies till EOF instead.
 * 
 * If out != NULL, and n > 0 or fsize(src) can be determined,
 * writes progress percentage to it like :
 * 	
 * 	fprintf(out, "%02u%%\r", (bytes_copied / (n > 0? n : fsize(src)) ) * 100);
 * 	fflush(out); 
 */
struct fbcp_retval fbcp(FILE *dest, FILE *src, uintmax_t n, FILE *out);

#endif

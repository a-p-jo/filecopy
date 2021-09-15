#ifndef BCP_H
	#define BCP_H
	
	#include <stdint.h>
	#include <stdio.h>

	/* Defaults */
	#define PRINT_PROGRESS stderr /* If defined, fbcp() writes progress every 1% to the stream */
	#define BUFSZ (64*1024) /* 64 KiB */

	/* Bytes written & synced to destination stream in last successful call to fbcp() */ 
	extern uintmax_t bytes_processed;
	
	enum retval {
		SUCCESS,
		/* From stream was seekable to SEEK_END, but reseting to initial position failed. 
		 * Cannot occur if PRINT_PROGRESS undefined.
		 */
		FROM_SEEK_ERR,
		/* Reading the from stream failed and ferror(from) returned true. 
      		 * No error in to stream.
	 	 */
		FERROR_FROM,
		/* Writing the to stream failed and ferror(to) returned true. 
	 	 * No error in from stream.
	 	 */
		FERROR_TO,
		/* Reading the from stream & writing the to stream failed and ferror() returned true for both */
		FERROR_BOTH,
		/* Flushing the to stream failed */
		FFLUSH_ERR }; 
	
	/* Copies contents of from stream onto to stream 
	 * until specified number of bytes or EOF, whichever comes first,
	 * and flushes it.
	 * If bytes argument is 0, copies till EOF.
	 *
	 * Copies in blocks of BUFSZ bytes. 
	 * If PRINT_PROGRESS defined, the from stream is seekable to SEEK_END, and data to copy is > 100 bytes,
	 * then displays progress of copy operation in percentage on stdout.
	 */
	enum retval fbcp(FILE *restrict from, FILE *restrict to, uintmax_t bytes);
	
#endif

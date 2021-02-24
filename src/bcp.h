//	Includes	//

/* If not already included, include the following :
 * stdio.h, for  : fprintf() , perror(), FILE*, fseek(), ftell() OR _ftelli64(), fopen(), fclose(), fread(), fwrite(), fflush()
 * stdint.h, for : uint_fast8_t , uint_fast64_t, int_fast64_t
 */
#ifndef _STDIO_H
#include <stdio.h>
#endif

#if !defined INT_FAST8_MAX || !defined INT_FAST64_MAX
#include <stdint.h>
#endif

//	Defines		//

#define BLOCK (1*1048576) // sets blocksize , 1 MiB by default.

#ifdef DEFAULT_VERBOSITY 
/* Set default printing behavior. By default, this print EVERYTHING.
 * 
 * NOTE : all printing is done to stderr.
 * 
 * PRINT_MID_IO_ERROR : print error message(s) if fread()/fwrite() fail to read as many bytes as asked,  and ferror() returns non-zero.
 * PRINT_OPEN_CLOSE_ERROR : perror() a message if fopen()/fclose() fail
 *
 * PRINT_PROGRESS : print progress % while copying file
 */
	#define PRINT_MID_IO_ERROR
	#define PRINT_OPEN_CLOSE_ERROR
	#define PRINT_PROGRESS
#endif

#ifdef PRINT_PROGRESS
/* If printinting progress , include limits.h to check for 64-bit file offsets
 */
	#if !defined LONG_MAX || !defined LLONG_MAX
		#include <limits.h>
	#endif
#endif

#ifdef OVERWRITE_PROTECT
/* Sets overwrite protections , such as aborting if overwriting or asking for confirmation if overwriting.
 * ONLY applies to bcp() , as fbcp() receives pre-opened FILE streams.
 *
 * ASK_BEFORE_OVERWRITE : Ask the user if they wish to abort or overwrite
 * ABORT_IF_OVERWRITING : Automatically abort if file pre-exists.
 * PRINT_OVERWRITE_ABORT : If automatically aborting, print (stderr) saying so.
 */
	#define ASK_BEFORE_OVERWRITE
	#undef ABORT_IF_OVERWRITING

	#ifdef ABORT_IF_OVERWRITING
		#define PRINT_OVERWRITE_ABORT
	#endif
#endif

/* Important global variables :
 * 1. bytes_processed : Number of bytes with successful fread() and fwrite(). Useful for error/success messages.
 * 2. bytes_read : Number of bytes successfully fread() in the current iteration, and after loop, the last attempted iteration.
 * Both are useful to access outside, so are extern.
 */
extern uint_fast64_t bytes_processed;
extern uint_fast64_t bytes_read;

//	Functions		//

int_fast64_t approx(long double);
int_fast64_t getsize(FILE *);
int fbcp(FILE *, FILE *);
int overwrite_chk(char *);
int bcp(char *, char *);

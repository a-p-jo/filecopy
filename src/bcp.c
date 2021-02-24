#define OVERWRITE_PROTECT
#define DEFAULT_VERBOSITY
#include "bcp.h"

uint_fast8_t buffer[BLOCK];
uint_fast64_t bytes_processed;
uint_fast64_t bytes_read;

int_fast64_t approx(long double num)
{
	if(num >= 0)
		return (int_fast64_t)(num + 0.5);
	else
		return (int_fast64_t)(num - 0.5);
}
int_fast64_t getsize(FILE * of)
{
/* Returns size of the given stream by seeking its EOF, asking for its offset from the begining, rewinding the file, and returning said distance.
 * Required to print progress.
 *
 * For any real usability, this offset needs to be >= 64-bit ; 32-bit can only handle 2 GB filesizes. 
 * Hence, we will have to check datatypes during pre-processing , to conditionally compile code appropriately.
 *
 * Conditional Compilation / Pre-processor checks :
 * 	1. C standard function ftell() returns the offset as a long. Check if a long is at least 64-bits. If yes, use ftell().
 * 	2. Else, identify the platform .
 * 		a) If Windows, use microsoft's _ftelli64(), guaranteed to return 64-bit offsets.
 *		b) If UNIX/POSIX, we must be in a 32-bit environment. 
 *		   As per POSIX, if we #define the offset-bits macro to 64 and then use ftello(), we'll get a 64-bit offset.
 *		c) If neither, this is some obscure/rare (presumably 32-bit) environment. Forget about printing progress, #undef the PRINT_PROGRESS. 
 */
	fseek(of,0,SEEK_END);
	int_fast64_t size;

	#if LONG_MAX >= LLONG_MAX
		size = ftell(of);

	#elif defined _WIN32
		size = _ftelli64(of);

	#elif defined(unix) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
		#define _FILE_OFFSET_BITS 64
		size = ftello(of);

	#else
		#undef PRINT_PROGRESS
	#endif            

	rewind(of);
	return size;
}

int fbcp(FILE * from, FILE * to)
{
/* FILE ByteCopy. Copies all bytes accurately in form FILE * from -> FILE * to .
 * Does NOT close the streams; it didn't open them.
 *
 * Return Values :
 * 0 : Success
 * -1 : fread() failed to process specified number of bytes and ferror() returned non-zero
 * -2 : fwrite() (or both fread() and fwrite()) failed to process specified number of bytes and ferror() returned non-zero
 */
	#ifdef PRINT_PROGRESS
		int_fast64_t bytes = getsize(from); // Get filesize in bytes 
	#endif

	#ifdef PRINT_PROGRESS 
	/* getsize() MAY #undef PRINT_PROGRESS; so a second check.
	 * one_percent stores 1% of filesize approximately.
	 * percent_now stores percentage traversed in current iteration; percent stores that of last iteration.
	 */
		int_fast64_t one_percent = approx((0.01 * bytes));
		uint_fast8_t percent = 0, percent_now;
	#endif

	/* Design of the loop :
	 * -> Try to read BLOCK bytes from source into the buffer. Exit loop if failed.
	 * -> If reading succeeded, try writing BLOCK bytes from buffer to destination. Exit loop if failed.
	 * -> If writing succeeded, increment bytes_processed by BLOCK units.
	 *     If PRINT_PROGRESS is defined : 
	 * -> Check that one_percent value is at least 1 . If not, skip to next iteration.
	 *	Why ? 
	 *	 	1. fseek() in getsize() may fail, and so ftell() will return 0, 1 % of which is 0.
	 *		2. ftell() in getsize() may fail, returning -1 , 1% of which is -ve.
	 *		3. stream may only contain < than 50 bytes, 1% of which is approximated to 0. 
         *
	 *		We avoid DIVISION BY ZERO and/or garbage values.
	 * -> Calculate change in percentage in this loop. If it is at least 1, print % and update percent to hold new value.
	 *	Why ?
	 *		1. This way, we print % 100 times or less only. This is efficient; we re-print only if there's a change.
	 		2. If not equal to one, we let it build up until it is.	 
	 */
	while((bytes_read = fread(buffer,1,BLOCK,from)) == BLOCK && fwrite(buffer,1,BLOCK,to) == BLOCK)
	{
		bytes_processed += BLOCK; 

		#ifdef PRINT_PROGRESS
			if(one_percent <= 0)
				continue;

			percent_now = bytes_processed / one_percent;

			if((percent_now - percent) >= 1)
			{ 
				fprintf(stderr,"%u%%\r", percent_now);
				percent = percent_now;
			}
		#endif
	}
	/* Once out of the loop,i.e, unable to read/write BLOCK bytes, check for errors.
	 *
	 * Do not use and if-else if , rather if-if, because it is not impossible that both streams fail.
	 * If errors detected, return appropriate values and print error messages if required.
	 *
	 * Else , perform one last fwrite() and check it too.
	 * 	Why ? 
	 *		It is likely (i.e. if filesize % BLOCK != 0) that there are some remaining bytes.
	 *		If bytes_read in the last iteration of the while loop is non-zero, then write these leftover bytes.
	 *		Else, do nothing.
	 * Also check this last fwrite() for errors, and if it does error out, handle it just as before. Else, return 0.
	 */
	if(ferror(from) || ferror(to))
	{
		int_fast8_t rval;

		if(ferror(from))
		{
			#ifdef PRINT_MID_IO_ERROR
				fprintf(stderr,"Failed : Unknown fatal error reading.\n");
			#endif
			rval = -1;
		}
		if(ferror(to))
		{
			#ifdef PRINT_MID_IO_ERROR
			failed_fwrite : 
			fprintf(stderr,"Failed : Unknown fatal error writing.\n");
			#endif
			rval = -2;
		}

		#ifdef PRINT_MID_IO_ERROR
			fprintf(stderr,"Forced to abandon at %llu bytes... exiting...\n",(long long unsigned)(bytes_processed));
		#endif

		return rval;
	}
	else
	{
		if(bytes_read && fwrite(buffer,1,bytes_read,to) != bytes_read)
		#ifdef PRINT_MID_IO_ERROR
			goto failed_fwrite;
		#else 
			return -1;
		#endif
		return 0;
	}
}
int overwrite_chk(char * name)
{
/* Check if given filename pre-exists, by trying to open it for reading, to avoid overwriting unwittingly.
 * If yes, autoexit, ask for user to confirm, or do nothing, as required.
 *
 * Returns 0 if continuing, non-zero if aborting.
 */
	FILE * tmp = fopen(name,"rb");
	if(name != NULL)
	{
		#if defined ABORT_IF_OVERWRITING
			#ifdef PRINT_OVERWRITE_ABORT
				fprintf(stderr,"%s already exits, aborting...\n",name);
			#endif
			fclose(tmp);
			return 1;
			
		#elif defined ASK_BEFORE_OVERWRITE
			char choice[101] = "";
			fprintf(stderr,"Warning : %s already exists !\nPress [ENTER] to overwrite or any key to abort : ", name);
			fgets(choice,101,stdin);

			if(*choice != '\n')
			{
				fprintf(stderr,"Exiting...\n");
				fclose(tmp);
				return 2;
			}
			fclose(tmp);
			return 0;
		#else
			fclose(tmp);
			return 0;
		#endif
	}
	else
		return 0;
}
int bcp(char * source, char * destination)
{
/* fbcp() , but uses filenames and manages streams internally.
 *
 * Return Values :
 * 0 : Success
 * -1 : fread() failed to process specified number of bytes and ferror() returned non-zero
 * -2 : fwrite() (or both fread() and fwrite()) failed to process specified number of bytes and ferror() returned non-zero
 * -3 : Error closing destination stream (and possibly also fread/fwrite errors as with -1 and -2)
 * -4 : Error opening source and/or destination.
 */	
	// Check for overwriting, exit if non-zero is returned.
	int returned = overwrite_chk(destination);

	if(returned)
		return returned;
	
	FILE * from = fopen(source,"rb");
	FILE * to = fopen(destination,"wb");
		
	if(from != NULL && to != NULL)
    	{
		int rval = fbcp(from,to);
		fclose(from);
		int rval2 = fclose(to);
			
		if(!rval && !rval2)
			return 0;
		
		else if(!rval2)
			return rval;
		else
		{
			#ifdef PRINT_OPEN_CLOSE_ERROR
				perror("Failed : Error writing to destination ");
			#endif
			return -3;
		}
    	}
    	else
   	{
		#ifdef PRINT_OPEN_CLOSE_ERROR
			perror("Failed ");
		#endif
		
		if(from != NULL)
	    		fclose(from);

		if(to != NULL)
	    		fclose(to);

		return -4;
    	}
}

/* If not already included, include the following :
 * stdio.h : perror(), FILE*, fseek(), ftell() OR _ftelli64(), fopen(), fclose(), fread(), fwrite(), fflush()
 * stdint.h : uint_fast8_t , uint_fast64_t
 */
#ifndef _STDIO_H
#include <stdio.h>
#endif

#if !defined INT_FAST8_MAX || !defined INT_FAST64_MAX
#include <stdint.h>
#endif

#define DEFAULT // default verbosity of the program : print *only* errors.
#define BLOCK (1*1048576) // blocksize for buffering read-write operations while copying. 1 MiB by default.

#ifdef DEFAULT

	#define PRINT_MID_IO_ERROR // prints to stderr if fread() or fwrite() fail
	#define PRINT_OPEN_CLOSE_ERROR // prints to stderr if fopen() or fclose() fail

	#define OVERWRITE_PROTECT
	#define PRINT_PROGRESS
#endif

#ifdef PRINT_PROGRESS
	#if !defined LONG_MAX || !defined LLONG_MAX //If printing progress, will need limits.h to check for size of long.
		#include <limits.h>
	#endif

	#define ABORT_IF_32_BIT_OFFSET // If long < 64-bit progress printing will be aborted, (since it breaks beyond 2 GiB files)
	#define approx(num) ((uint_fast64_t)(num+0.5)) // round a number to an int
#endif

#ifdef OVERWRITE_PROTECT
	#define ASK_BEFORE_OVERWRITE // asks the user before overwriting a file
	//#define ABORT_IF_OVERWRITING   // automatically exits if destination file pre-exists

	#ifdef ABORT_IF_OVERWRITING
		#define PRINT_OVERWRITE_ABORT // Inform user of automatic abort
	#endif
#endif

int bcp(char * source, char * destination)
{
	FILE * from = fopen(source,"rb");
	FILE * to;

	#if defined ABORT_IF_OVERWRITING || defined ASK_BEFORE_OVERWRITE
		to = fopen(destination,"rb");

		if(to != NULL)
		{
			#if defined ABORT_IF_OVERWRITING
				#ifdef PRINT_OVERWRITE_ABORT
					fprintf("%s already exits, aborting...\n");
				#endif
				fclose(from);
				fclose(to);
				return 2;
			
			#elif defined ASK_BEFORE_OVERWRITE
			char choice[101] = "";
			fprintf(stderr,"Warning : %s alreasy exists !\nPress [ENTER] to overwrite or any key to abort ", destination);
			fgets(choice,101,stdin);

			if(*choice != '\n')
			{
				fprintf(stderr,"Exiting...\n");
				fclose(from);
				fclose(to);
				return 2;
			}
			fclose(to);
			#endif
		}
	#endif

	to = fopen(destination,"wb");
		
	if(from != NULL && to != NULL)
    	{
		#ifdef PRINT_PROGRESS
			fseek(from,0,SEEK_END);
	
			#if LONG_MAX == LLONG_MAX
				long bytes = ftell(from);
	    	
			#elif defined _WIN32
				__int64 = _ftelli64(from);
	    	
			#elif defined(unix) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
				#define _FILE_OFFSET_BITS 64
				off_t bytes = ftello(from);
	    	
			#else
				#ifdef ABORT_IF_32_BIT_OFFSET
					#undef PRINT_PROGRESS
				#else
					long bytes = ftell(from);
				#endif
	    		#endif            
	
			rewind(from);
		#endif

		uint_fast8_t buffer[BLOCK];
		uint_fast64_t bytes_processed = 0;
		uint_fast64_t bytes_read;

		#ifdef PRINT_PROGRESS
			uint_fast64_t one_percent = approx((0.01 * bytes));
			uint_fast8_t percent = 0, percent_now;
		#endif

		while((bytes_read = fread(buffer,1,BLOCK,from)) == BLOCK && fwrite(buffer,1,BLOCK,to) == BLOCK)
		{
	    		bytes_processed += BLOCK; 
	    
	    		#ifdef PRINT_PROGRESS
				if(!one_percent) continue; 
				percent_now = bytes_processed / one_percent;
				if((percent_now - percent) >= 1) 
				{
		    			fprintf(stdout,"%u%%\r", percent_now);
						fflush(stdout);
				}
				percent = percent_now;
	   		#endif
		}
		if(ferror(from) || ferror(to))
		{
	    		#ifdef PRINT_MID_IO_ERROR
				if(ferror(from))
		    			fprintf(stderr,"Failed : Unknown fatal error reading from %s\n",source);
				if(ferror(to))
		    		failed_fwrite : 
		    			fprintf(stderr,"Failed : Unknown fatal error writing to %s\n",destination);
		
				fprintf(stderr,"Forced to abandon at %llu bytes... exiting...\n",(long long unsigned)(bytes_processed));
	    		#endif

	   		 fclose(from);
	   		 fclose(to);
	   		 return -1;
		}
		else
		{
	    		if(bytes_read && fwrite(buffer,1,bytes_read,to) != bytes_read)
			#ifdef PRINT_MID_IO_ERROR
		    		goto failed_fwrite;
			#else 
		    		return -1;
			#endif

	    		fclose(from);

	    		if(fclose(to) == 0)                
				return 0;

	    		else
			{
				#ifdef PRINT_OPEN_CLOSE_ERRORS
					perror("Failed ");
				#endif
				return -2;
			}
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

		return -3;
    	}
}

int fbcp(FILE * from, FILE * to)
{
	#ifdef PRINT_PROGRESS
		fseek(from,0,SEEK_END);

		#if LONG_MAX == LLONG_MAX
			long bytes = ftell(from);

		#elif defined _WIN32
			__int64 = _ftelli64(from);

		#elif defined(unix) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
			#define _FILE_OFFSET_BITS 64
			off_t bytes = ftello(from);

		#else
			#ifdef ABORT_IF_32_BIT_OFFSET
				#undef PRINT_PROGRESS
			#else
				long bytes = ftell(from);
			#endif
		#endif            

		rewind(from);
	#endif

	uint_fast8_t buffer[BLOCK];
	uint_fast64_t bytes_processed = 0;
	uint_fast64_t bytes_read;

	#ifdef PRINT_PROGRESS
		uint_fast64_t one_percent = approx((0.01 * bytes));
		uint_fast8_t percent = 0, percent_now;
	#endif

	while((bytes_read = fread(buffer,1,BLOCK,from)) == BLOCK && fwrite(buffer,1,BLOCK,to) == BLOCK)
	{
		bytes_processed += BLOCK; 

		#ifdef PRINT_PROGRESS
			if(!one_percent) continue; 
			percent_now = bytes_processed / one_percent;
			if((percent_now - percent) >= 1) 
			{
				fprintf(stdout,"%u%%\r", percent_now);
					fflush(stdout);
			}
			percent = percent_now;
		#endif
	}
	if(ferror(from) || ferror(to))
	{
		#ifdef PRINT_MID_IO_ERROR
			if(ferror(from))
				fprintf(stderr,"Failed : Unknown fatal error reading\n");
			if(ferror(to))
			failed_fwrite : 
				fprintf(stderr,"Failed : Unknown fatal error writing\n");

			fprintf(stderr,"Forced to abandon at %llu bytes... exiting...\n",(long long unsigned)(bytes_processed));
		#endif
		return -1;
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
		


#ifdef _STDIO_H
#include <stdio.h>
#endif

#if !defined LONG_MAX || !defined LLONG_MAX
#include <limits.h>
#endif

#if !defined INT_FAST8_MAX || !defined INT_FAST64_MAX
#include <stdint.h>
#endif

#define PRINT_PROGRESS
#define PRINT_MID_IO_ERROR
#define BLOCK (1*1048576)

#define approx(num) ((uint_fast64_t)(num+0.5)) 

int bcp(char * source, char *destination, char * if_only_32bit_offset)

{
    

    FILE * from = fopen(source,"rb");
	FILE * to = fopen(destination,"wb");
		
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

        if(*abort == 'y' || *abort == 'Y')
        {
            fclose(from);
            fclose(to);
            return 1;
        }
        
        long bytes = ftell(from);
        
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
                printf("%u%%\r", percent_now);
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
                return -2;
        }
    }
    else
    {
        if(from != NULL)
            fclose(from);

        if(to != NULL)
            fclose(to);

        return -3;
    }
	}
}

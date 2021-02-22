#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#define PRINT_PROGRESS
#define BLOCK (1*1048576)

#define approx(num) ((uint_fast64_t)(num+0.5)) 

int main(int argc, char * argv[])
{

	if(argc >= 2) 
	{
		
		uint_fast8_t outfile_given = (argc >= 3);
		
		FILE * from = fopen(argv[1],"rb");
		FILE * to = stdout;
		
		if (outfile_given)
			to = fopen(argv[2],"wb");
		
		
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
			fprintf(stderr,"WARNING : BCP could not use 64-bit file offsets.\nIf your file is larger than or close to 2 GB in size, abort immediately !\n");
			while(1)
			{
				char abort[101] = "";
				
				fprintf(stderr,"Abort ? (Y/N) : ");
				fgets(abort,101,stdin);
			
				if(*abort == 'n' || *abort == 'N')
					break;
				else if(*abort == 'y' || *abort == 'Y')
				{
					fprintf(stderr,"Aborting...\n");
					fclose(from);
					
					if(outfile_given)
						fclose(to);
					
					return 1;
				}
				else
					continue;
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
					fprintf(stderr,"%u%%\r", percent_now);
				
				percent = percent_now; 
				#endif
			}

			if(ferror(from) || ferror(to))
			{
				if(ferror(from))
					fprintf(stderr,"Failed : Unknown fatal error reading from %s\n",argv[1]);
				if(ferror(to))
					failed_fwrite : 
					fprintf(stderr,"Failed : Unknown fatal error writing to %s\n",argv[2]);
				
				fprintf(stderr,"Forced to abandon copying at %llu bytes... exiting...\n",(long long unsigned)(bytes_processed));

				fclose(from);
				
				if(outfile_given)
					fclose(to);
				
				return -1;
			}

			else
			{
				

				if(bytes_read && fwrite(buffer,1,bytes_read,to) != bytes_read)
					goto failed_fwrite; 				

				fclose(from);

				if(outfile_given && fclose(to) == 0)
				{	
					printf("Copied %llu bytes from %s to %s.\n",(long long unsigned)(bytes_processed + bytes_read),argv[1],argv[2]); 
					
					return 0;
				}
				else if(outfile_given)
				{
					perror("Failed : Error writing to destination ");
					return -2;
				}
				else
					return 0;
			}
		}
		else
		{
			if(from != NULL)
			{
				perror("Failed : Error opening destination ");
				fclose(from);
			}
			if(to != NULL)
			{
				perror("Failed : Error opening source ");
				
				if(outfile_given)
					fclose(to);
			}
			return -3;
		}
	}
	else
	{
		fprintf(stderr,"Failed : Got 0 arguments, expected at least 1.\n");
		return -4;
	}	
}

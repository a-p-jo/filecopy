#include <stdio.h>
#include <stdint.h>

#define PRINT_PROGRESS
#define PRINT_FREQ (1048576*5) // Print progress once every 5 MiB

int main(int argc, char * argv[])
{
	if(argc >= 3)
	{
		FILE * from = fopen(argv[1],"rb");
		FILE * to = fopen(argv[2],"wb");
		
		if(from != NULL && to != NULL)
		{
			fseek(from,0,SEEK_END);
			uint64_t bytes = ftell(from);
			rewind(from);
			
			for(uint64_t i = 0; i < bytes; ++i)
			{
				#if defined PRINT_PROGRESS && defined PRINT_FREQ
				if(i % PRINT_FREQ == 0 && i >= PRINT_FREQ)
				{
					printf("%.0lf%%\r",((double)i/(double)bytes)*100);
					fflush(stdout);
				}
				#endif

				fputc(fgetc(from),to);
			}

			fclose(from);

			if(fclose(to) == 0)
			{
				printf("Successfully copied %llu bytes from %s to %s.\n",(unsigned long long)bytes,argv[1],argv[2]);
				return 0;
			}
			else
			{
				perror("Failed : Error writing to destination ");
				return -1;
			}
		}
		else
		{
			if(from != NULL)
			{
				perror("Failed : Error opening destination ");
				fclose(from);
				return -2;
			}
			else if(to != NULL)
			{
				perror("Failed : Error opening source ");
				fclose(to);
				return -3;
			}
		}
	}
	else
	{
		printf("Failed : Got %d arguments, expected %d.\n",argc,3);
		return -4;
	}
}

#include "bcp.h" 

int main(int argc, char * argv[])
{

	if(argc >= 2) //usage : bcp [source] [destination (optional)]
	{
		/* Open file(s) as binary streams to read and write to.
		 * By default, output is stdout.
		 * Proceed if both streams were successfully opened.
		 */
		
		uint_fast8_t outfile_given = (argc >= 3);
		FILE * from = fopen(argv[1],"rb");
		FILE * to = stdout;
		
		if (outfile_given)
			to = fopen(argv[2],"wb");
		
		if(from != NULL && to != NULL)
		{
			int rval = fbcp(from,to);
			fclose(from);
			int rval2 = fclose(to);
			
			if(!rval && !rval2)
			{
				fprintf(stderr,"Copied %llu bytes from %s to %s.\n",(long long unsigned)(bytes_processed + bytes_read),argv[1],(outfile_given)? argv[2] : "stdout");
				return 0;
			}
			else if(!rval2)
				return rval;
			else
			{
				perror("Failed : Error writing to destination ");
				return -2;
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

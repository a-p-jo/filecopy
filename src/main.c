#include "bcp.h" 
#include <stdint.h>
#include <string.h>

/* Usage : bcp [source] [destination (optional)] [overwrite ? (y/n) (optional)]
 *
 * Flow of control :
 * 1. Run if at least one argument is given. Otherwise, print and exit with -5.
 * 2. If given an output file, check if overwrite permission is supplied.
 * 	a) If yes, act accordingly. If allowed to overwrite, skip to copying. Else, print error and terminate with -1 if file exists.
 *	b) If no, check if the file exists, if it does, ask for overwrite confirmation and if given, proceed. If not given, print and exit with -2.
 * 3. Open FILE streams. If an output file was given , open it, otherwise write to stdout.
 * 4. If streams opened successfully, call fbcp() on them. Else, print error message and return -4.
 * 5. Check return value of fbcp(), and, if given an output file, of its fclose().
 * 6. If both were successful, print success message and return 0.
 *    Else, if fbcp() has a non-zero return, return that same value (fbcp() handles printing of any errors internally).
 *    Else, (fclose was unsuccessful), print error message and return -3.
 */

int main(int argc, char * argv[])
{

	if(argc >= 2)
	{	
		uint_fast8_t outfile_given = (argc >= 3);
		uint_fast8_t overwrite_permission_given = (argc >=4);
		if(outfile_given)
		{
			if(overwrite_permission_given)
			{
				if(!strcmp(argv[3],"y") || strcmp(argv[3], "yes"))
					;
				else if(!strcmp(argv[3],"n") || strcmp(argv[3], "no"))
				{
					FILE * tmp = fopen(argv[2],"rb");
					if(tmp != NULL)
					{
						fclose(tmp);
						fprintf(stderr,"%s already exists, aborting...\n",argv[2]);
						return -1;
					}
					fclose(tmp);
				}
			}
			else
			{
				if(!overwrite_chk(argv[2]))
					;
				else
					return -2;
			}

		}
			
		FILE * from = fopen(argv[1],"rb");
		FILE * to = stdout;
		
		if (outfile_given)
			to = fopen(argv[2],"wb");
		
		if(from != NULL && to != NULL)
		{
			int rval = fbcp(from,to);
			fclose(from);

			int rval2 = 0;
			if(outfile_given)
				rval2 = fclose(to);
			
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
				return -3;
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
			return -4;
		}
	}
	else
	{
		fprintf(stderr,"Failed : Got 0 arguments, expected at least 1.\n");
		return -5;
	}
}

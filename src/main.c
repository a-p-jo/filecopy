#include "bcp.h" 
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* Usage : bcp <source> [destination] [overwrite ? (y/n) ]
 *
 * Flow of control :
 * 1. Run if at least one argument is given. Otherwise, print and exit with -5.
 * 2. If given an output file, check if overwrite permission is supplied.
 * 	a) If yes, act accordingly. If allowed to overwrite, skip to copying. Else, print error and terminate with -1 if file exists.
 *	b) If no, check if the file exists, if it does, ask for overwrite confirmation and if given, proceed. If not given, print and exit with -2.
 * 3. Open FILE streams. If an output file was given , open it, otherwise write to stdout.
 * 4. If streams opened successfully, call fbcp() on them. Else, print error message and exit with -4.
 * 5. Check return value of fbcp(), and, if given an output file, of its fclose().
 * 6. If both were successful, print success message and exit with 0.
 *    Else, if fbcp() has a non-zero return, return that same value (fbcp() handles printing of any errors internally).
 *    Else, (fclose was unsuccessful), print error message and exit with -3.
 */

int main(int argc, char **argv)
{

	if(argc >= 2)
	{	
		uint_fast8_t outfile_given = (argc >= 3);
		uint_fast8_t overwrite_permission_given = (argc >=4);
		if(outfile_given)
		{
			if(overwrite_permission_given)
			{
				if(!strcmp(argv[3],"y") || !strcmp(argv[3], "yes"))
					;
				else if(!strcmp(argv[3],"n") || !strcmp(argv[3], "no"))
				{
					FILE * tmp = fopen(argv[2],"rb");
					if(tmp)
					{
						fprintf(stderr,"%s already exists, aborting...\n",argv[2]);
						exit(-1);
					}
				}
				else goto ask;
			}
			else ask : if(overwrite_chk(argv[2])) exit(-2);
		}
			
		FILE * from = fopen(argv[1],"rb");
		FILE * to = outfile_given? fopen(argv[2],"wb") : stdout;
		
		if(from && to)
		{
			int rval = fbcp(from,to);
			int rval2 = 0;
			
			if(outfile_given)
				rval2 = fclose(to);
			
			if(!rval && !rval2)
			{
				fprintf(stderr,"Copied %llu bytes from %s to %s.\n",(long long unsigned)(bytes_processed + bytes_read),argv[1],(outfile_given)? argv[2] : "stdout");
				exit(0);
			}
			else if(!rval2)
				exit(rval);
			else
			{
				perror("Failed : Error writing to destination ");
				exit(-3);
			}
		}
		else
		{
			if(from)
				perror("Failed : Error opening destination ");
			if(to)
				perror("Failed : Error opening source ");
			exit(-4);
		}
	}
	else
	{
		fprintf(stderr,"Failed : Got 0 arguments, expected at least 1.\n");
		exit(-5);
	}
}

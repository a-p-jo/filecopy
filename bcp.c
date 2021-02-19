/* Two headers will be needed, standard I/O and standard integer
 * 
 * stdio.h provides the I/O functions : printf(), perror(), FILE*, fseek(), ftell() || _ftelli64(), fopen(), fclose(), fread(), fwrite(), fflush()
 * stdint.h provides uint_fast8_t , uint_fast64_t
 *
 * Two macros help easily configure the program.
 *
 * PRINT_PROGRESS this macro is defined if you wish to allow the displaying of progress. Remove it to disable.
 *
 * BLOCK defines the block size in bytes. It is set to (1*1048576) or 1 MiB by defualt, and can be changed to an optimal value for your system.
 * BCP will copy BLOCK bytes at-a-time from the source file to the target file. Changing this may potentially offer some improvement in transfer speed.
*/
#include <stdio.h>
#include <stdint.h>

#define PRINT_PROGRESS
#define BLOCK (1*1048576)

//Trivial function that rounds any +ve floating point type number to an integer type
uint_fast64_t approx(long double num)
{
	return (uint_fast64_t)(num+0.5);
}

int main(int argc, char * argv[])
{
	//usage : bcp [source] [destination]

	if(argc >= 3)
	{
		/* Open source and destination files as binary streams to read and write to.
		 * Proceed if both streams were successfully opened.
		 */

		FILE * from = fopen(argv[1],"rb");
		FILE * to = fopen(argv[2],"wb");
		
		if(from != NULL && to != NULL)
		{
			/* Get the size of the file in bytes.
			 *
			 * fseek to end, find the offset from the begining.This offset is the size , in bytes, of the file.
			 * Since filesize can't be negative, save to unsigned 64-bit integer.
			 * Rewind the stream to it's begining.
			 *
			 * Why the ifdef _WIN32 ?
			 *
			 * Per standard , ftell() returns a LONG INT. 
			 * This is 64-bit signed value in *NIX systems, which is ~8 exabytes, sufficient
			 * This is a 32-bit signed value in NT systems, which is ~2 gigabytes, insufficient. 
			 * Thus,on Windows, use _ftelli64(), a "microsoft function" returning a 64-bit signed value
			 */

			fseek(from,0,SEEK_END);
			#ifdef _WIN32
			uint_fast64_t bytes = _ftelli64(from);
			#else
			uint_fast64_t bytes = ftell(from);
			#endif

			rewind(from);

			/* Declare a fast, static array of BLOCK bytes on the stack.
			 * Depending on CPU and size of BLOCK, potentially places buffer on resgisters or a fast part of RAM.
			 * Nevertheless, it will perform no worse than a malloc() but with less error-checking nonsense, and will
			 * likely perform better.
			 *
			 * Also need a way to store bytes processed, for progress and success message.
			 * Number of blocks copied is more managable and maybe more effecient than storing the number of bytes directly.
			 */

			uint_fast8_t buffer[BLOCK];
			uint_fast64_t blocks_processed = 0;
			uint_fast64_t bytes_read;

			#ifdef PRINT_PROGRESS
			/* How progress is printed efficiently :
			 *
			 * 1. Take an approximate value of the number of blocks in in 1% of the source file.
			 * 2. Every time a block is copied, if net data copied is >= 1% of file size, print the changed percentage. 
			 *
			 * That is, we print only 100 times or lesser, and most looped math happens only on integers, which is more efficient.
			 * Also, on a file smaller than (100 * BLOCK) bytes , 100 MiB for a 1 MiB block, there is not printing whatsoever,
			 * as that would anyways be unnecessary given modern I/O speeds. 
			 *
			 * So, we need a variable storing the approximate number of blocks in 1% of filesize, a count of and a variable to store percentage till now.
			 * As maximum % is 100 and % stays +ve, percent_till_now need not be larger than 8-bits and can be unsigned.
			 */
			uint_fast64_t one_percent = approx((0.01 * bytes)/BLOCK);
			uint_fast8_t percent_till_now;
			#endif

			/* Try to fread BLOCK bytes from source stream.
			 * Loop over the stream, fwrite'ing to destination and updating progress while able to read BLOCK bytes.
			 * 
			 * When unable to read BLOCK bytes, check for any errors.
			 *
			 * If no errors, we have encountered the last few bytes that are less than BLOCK (by default, 1 MiB). 
			 * This means we can fwrite these last few bytes and attempt to fclose the streams.
			 *
			 * Check both streams, as error may be both ways. If ferror() returns non-zero, there is some dire error - given it occured mid-I/O; IMMEDIATELY STOP.
			 * Unfortunately, fread()/fwrite()/ferror() does not set errno per standard. Hence, print "unknown","fatal" error; copying is "abandoned" at foo bytes.
			 * 
			 * We could delete the output file, but since this a rare and very bad error, will leave it for the user if needs to recover.
			 * It is easy to delete a corrupt file, but is relatively difficult to RECOVER a deleted file !
			 * However, we will tidy up with fclose'ing the streams.
			 */

			while((bytes_read = fread(buffer,1,BLOCK,from)) == BLOCK && fwrite(buffer,1,BLOCK,to) == BLOCK)
			{
				#ifdef PRINT_PROGRESS
				if((percent_till_now = approx((++blocks_processed) / one_percent)) >= 1)
				{
					printf("%d%%\r", percent_till_now);
					fflush(stdout);
				}
				#endif
			}

			if(ferror(from) || ferror(to))
			{
				if(ferror(from))
					printf("Failed : Unknown fatal reading from %s\n",argv[1]);
				if(ferror(to))
					printf("Failed : Unknown fatal error writing to %s\n",argv[2]);
				
				printf("Forced to abandon copying at %llu bytes... exiting...",(long long unsigned)(blocks_processed*BLOCK));

				fclose(from);
				fclose(to);
				return -1;
			}

			else
			{
				fwrite(buffer,1,bytes_read,to); 
				// If this fails, that *should* be reflected in fclose(to) below. We will not check this for the sake of our sanity.

				/* Done copying streams, attempt fclose'ing them.
				 *
				 * This writes remaining data to disk for destination stream, so check for any errors.
				 *
				 * Did not check fclosing source stream, nothing can be done about it anyway, and shouldn't lead to data loss, as it was merely read.
				 * 
				 * Since errno is set by fclose() upon failure, use perror() in case of problems.
				 */

				fclose(from);

				if(fclose(to) == 0)
				{	
					printf("Copied %llu bytes from %s to %s.\n",(long long unsigned)(blocks_processed*BLOCK)+bytes_read,argv[1],argv[2]);
					return 0;
				}
				else
				{
					perror("Failed : Error writing to destination ");
					return -2;
				}
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
				fclose(to);
			}
			return -3;
		}
	}
	else
	{
		printf("Failed : Got %d argument(s), expected %d.\n",argc,3);
		return -4;
	}

	/* Return Values :
	 * 0 : Success
	 * -1 : Fatal Read/Write Error in middle of I/O , copying abandoned.
	 * -2 : Error writing to destination (when fclose'ing)
	 * -3 : Error opening destination/source
	 * -4 : Insufficient arguments
	 */
}

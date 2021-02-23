/* Three headers will be needed : standard I/O, standard integer and limits
 * 
 * stdio.h provides the I/O functions : printf(), perror(), FILE*, fseek(), ftell() OR _ftelli64(), fopen(), fclose(), fread(), fwrite(), fflush()
 * stdint.h provides uint_fast8_t , uint_fast64_t
 * limits.h allows us to check if a long is at least 64-bit and act accordingly
 *
 * Two macros help easily configure the program.
 *
 * PRINT_PROGRESS - this macro is defined if you wish to allow the displaying of progress. Remove it to disable.
 *
 * BLOCK - defines the block size in bytes. It is set to (1*1048576) or 1 MiB by defualt, and can be changed to an optimal value for your system.
 * BCP will copy BLOCK bytes at-a-time from the source file to the target file. Changing this may potentially offer some improvement in transfer speed.
*/
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#define PRINT_PROGRESS
#define BLOCK (1*1048576)

#define approx(num) ((uint_fast64_t)(num+0.5)) //Trivial macro rounds +ve floating point number to nearest integer type. 

int main(int argc, char * argv[])
{

	if(argc >= 2) //usage : bcp [source] [destination (optional, stdout by default)]
	{
		/* Open file(s) as binary streams to read and write to.
		 * Proceed if both streams were successfully opened.
		 */
		
		uint_fast8_t outfile_given = (argc >= 3);

		FILE * from = fopen(argv[1],"rb");
		FILE * to = stdout;
		
		if (outfile_given)
			to = fopen(argv[2],"wb");
		
		if(from != NULL && to != NULL)
		{
			/* Get the size of the file in bytes, needed only if we need to print progress.
			 *
			 * In short : 
			 * fseek() to end, find the offset from the begining. This offset is the size , in bytes, of the file. Rewind the stream to it's begining.
			 *
			 * What's with the conditional compilation below ?
			 * 
			 * For us to use files sizes larger than  ~2GB, we need to be able to represent the number of bytes in it in a 64-bit number.
			 * The standard C function ftell() returns a long, which can be 32-bit (Windows, 32-bit BSDs & Linux, etc.), and the standard guarantees little. 
			 * So, we :
			 * 
			 * 1. Use ftell() directly if a long is 64-bit. (LONG_MAX == LLONG_MAX, implies sizeof(long) == sizeof(long long), implies long is at least 64 bit)
			 * 2. If long is less than 8 bytes, this is likely due to being on windows. Are we on windows ? The _WIN32 macro's absence or presence
			 *    will tell us. If we are on windows, we'll use microsoft's _ftelli64() which returns __int64 (64-bit signed value).
			 * 3. If we're not on windows, this might be a 32-bit *NIX system.The __unix__ , __unix, unix or __APPLE__ and __MACH__ (macOS) macros will tell us.
			 *    POSIX standard defines ftello(), similar to ftell(), but return off_t instead of long.
			 *    POSIX also specifies that if we define _FILE_OFFSET_BITS to be 64, off_t is a 64-bit value. So, we'll do that.
			 * 4. In the unlikely case that this is a non-*NIX non-Windows environment with longs less than 64-bits, we'll need to inform the user explicitly and ask
			 *    if they wish to abort, and warn that BCP won't copy more than 2GB in such cases. Since we can't guess on behalf of the user in such matters, we'll force them
			 *    into an infinite loop asking until they explicitly tell us. 
			 * 
			 */

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
					frprintf(stderr,"Aborting...\n");
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

			/* Declare a fast, static array of BLOCK bytes on the stack.
			 *
			 * Depending on CPU and size of BLOCK, potentially places buffer on resgisters or a fast part of RAM.
			 * Nevertheless, it will perform no worse than a malloc() but with less error-checking nonsense, and will
			 * likely perform better.
			 *
			 * Also need a way to store bytes processed, for progress and success message.
			 * Previous versions stored number of blocks processed, but now BCP uses bytes processed directly for simplicity. 
			 */

			uint_fast8_t buffer[BLOCK];
			uint_fast64_t bytes_processed = 0;
			uint_fast64_t bytes_read;

			#ifdef PRINT_PROGRESS
			/* How progress is printed efficiently :
			 *
			 * 1. Take an approximate int value of the number of bytes in 1% of the source file.
			 *     NOTE : printing happens if file size >= 50 bytes; 1% of it has be at least 0.5 - Any lesser and it approximates to 0. 
			 * 2. Every time 1% or more is copied, print the changed percentage. 
			 *
			 * That is, we print only 100 times or lesser, and looped math happens only on integers, which is efficient.
			 *
			 * So, we need a variable storing the approximate number of bytes in 1% of filesize, a variable to store percentage, and another to store new percentage,
			 * so as to get the change in percentage between iterations.
			 * 
			 * As maximum % is 100 and % stays +ve, percentage variables need not be larger than 8-bits and can be unsigned.
			 */
			uint_fast64_t one_percent = approx((0.01 * bytes));
			uint_fast8_t percent = 0, percent_now;
			#endif

			/* Try to fread BLOCK bytes from source stream, until we can't.
			 *
			 * In the loop condition , we are testing *both* fread() & fwrite(). With && operator, if fread() condition fails, it will not attempt to check
			 * the second condition, hence fwrite() will not be executed with garbage/leftover values , without having to do a less elegant nested conditional underneath the loop.
			 * 
			 * When unable to read BLOCK bytes, check for any errors. Check BOTH streams rather than an if-else, as error *may* be both ways. 
			 *
			 * If there are no errors, we have encountered the last few bytes that are less than BLOCK (by default, 1 MiB). 
			 * This means we can fwrite these last few bytes and attempt to fclose the streams.
			 *
			 * If ferror() returns non-zero, there is some dire error - given it occured mid-I/O - IMMEDIATELY STOP.
			 * Unfortunately, fread()/fwrite()/ferror() does not set errno per standard. Hence, print "unknown fatal error" + copying is "abandoned" at foo bytes.
			 * 
			 * We could delete the output file, but since this a rare and very bad error, will leave it as-is for the user if he needs to recover data.
			 * It is easy to delete a corrupt file, but is relatively difficult to RECOVER a deleted file !
			 *
			 * Also it's quite likely , depending on what caused the error, that remove() will also uselessly fail until the user fixes things.
			 *
			 * However, we *will* tidy up with fclose'ing the stream(s) - but only after the error message, in case that segfaults (which it may, again depending on cause of error)
			 */

			while((bytes_read = fread(buffer,1,BLOCK,from)) == BLOCK && fwrite(buffer,1,BLOCK,to) == BLOCK)
			{
				bytes_processed += BLOCK; // Increment the count of bytes copied by the blocksize.
				
				#ifdef PRINT_PROGRESS
				if(!one_percent) continue; // Avoids ZeroDivisionError in case one_percentage bytes rounded off to 0
				
				percent_now = bytes_processed / one_percent; // get the new percentage (in int form, since both operands are ints)
				
				if((percent_now - percent) >= 1) // avoid wasteful and inefficient printing
					fprintf(stderr,"%u%%\r", percent_now);
				
				percent = percent_now; // update percentage count
				#endif
			}

			if(ferror(from) || ferror(to))
			{
				if(ferror(from))
					fprintf(stderr,"Failed : Unknown fatal error reading from %s\n",argv[1]);
				if(ferror(to))
					failed_fwrite : // Used if last fwrite() fails ; see lines 151 to 155 
					fprintf(stderr,"Failed : Unknown fatal error writing to %s\n",argv[2]);
				
				fprintf(stderr,"Forced to abandon copying at %llu bytes... exiting...\n",(long long unsigned)(bytes_processed));

				fclose(from);
				
				if(outfile_given)
					fclose(to);
				
				return -1;
			}

			else
			{
				/* One last call to fwrite() remains for any leftover bytes, i.e. there is still room for failure.
				 *
				 * However, we need only call fwrite() if there's actually anything to write. If there's 0 bytes left, avoid unnecessary
				 * statements - the && operator helps - if there's nothing to be written , the fwrite is never executed, as that condition is not evaluated. 
				 *
				 * If fwrite fails, avoid repetitive/boilerplate code, use goto as the error and it's handling is EXACTLY the same as we *just* handled.
				 * Note : failed_fwrite ends with "return -1;" ; no infinite goto bug.
				 */

				if(bytes_read && fwrite(buffer,1,bytes_read,to) != bytes_read)
					goto failed_fwrite; 

				/* Done copying streams,no errors yet, attempt fclose'ing them.
				 *
				 * This writes remaining data to disk for destination stream, so check for any errors.
				 *
				 * Did not check fclosing source stream, nothing can be done about it anyway, and shouldn't lead to data loss, as it was merely read.
				 * 
				 * Since errno is set by fclose() upon failure, use perror() in case of problems.
				 */

				fclose(from);

				if(outfile_given && fclose(to) == 0)
				{	
					printf("Copied %llu bytes from %s to %s.\n",(long long unsigned)(bytes_processed + bytes_read),argv[1],argv[2]); 
					// must add in bytes_read from the last fread() when displaying net bytes copied.
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

	/* Return Values :
	 * 0 : Success
	 * -1 : Fatal Read/Write Error in middle of I/O , copying abandoned.
	 * -2 : Error writing to destination (when fclose'ing)
	 * -3 : Error opening destination/source
	 * -4 : Insufficient arguments
	 * 1 : User aborted when informed about non-64 bit file offset in non-*NIX non-WIN environment.
	 */
}

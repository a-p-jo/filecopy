/* Three headers will be needed : standard I/O, standard integer and limits. We will check if these are not already included before including them.
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

#define approx(num) ((uint_fast64_t)(num+0.5)) //Trivial macro rounds +ve floating point number to nearest integer type. 

int bcp(char * source, char *destination, char * if_only_32bit_offset)
/* Arguments :
 * 1. char * source , name of source file
 * 2. char * destination , name of destination file
 * 3. char * if_only_32bit_offset : MAKE SURE TO SET THIS ARGUMENT CAREFULLY ! 
 *      If BCP finds no way to get a 64-bit offset for filesize, it will be limited to files ~2GB or less is size.
 *      In such a case, it will test if_only_32bit_offset for wether you wish to abort copying or continue.
 *      If if_only_32bit_offset[0] == 'y' OR 'Y' , we will abort and return 1.
 *      Else , we will conitnue regardless.
 * 
 * Return Values :
 *  0 : Success
 * -1 : Fatal Read/Write Error in middle of I/O , copying abandoned.
 * -2 : Error writing to destination (when fclose'ing)
 * -3 : Error opening destination/source
 * -4 : Insufficient arguments
 *  1 : Aborted when encountered non-64 bit file offset in non-*NIX non-WIN environment.
 */
{
    /* Open source and destination files as binary streams to read and write to.
     * Proceed if both streams were successfully opened.
     */

    FILE * from = fopen(source,"rb");
	FILE * to = fopen(destination,"wb");
		
    if(from != NULL && to != NULL)
    {
        /* Get the size of the file in bytes, needed only if need to print progress.
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
         * 4. In the unlikely case that this is a non-*NIX non-Windows environment with longs less than 64-bits, 
         *    we'll need to check with the argument if_only_32bit_offset to see if we can proceed or should abort.
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
         * However, we *will* tidy up with fclose'ing the streams - but only after the error message, in case that segfaults (which it may, again depending on cause of error)
         */

        while((bytes_read = fread(buffer,1,BLOCK,from)) == BLOCK && fwrite(buffer,1,BLOCK,to) == BLOCK)
        {
            bytes_processed += BLOCK; // Increment the count of bytes copied by the blocksize.
            
            #ifdef PRINT_PROGRESS
            if(!one_percent) continue; // Avoids ZeroDivisionError in case one_percentage bytes rounded off to 0
            
            percent_now = bytes_processed / one_percent; // get the new percentage (in int form, since both operands are ints)
            
            if((percent_now - percent) >= 1) // avoid wasteful and inefficient printing
            {
                printf("%u%%\r", percent_now);
                fflush(stdout);
            }
            percent = percent_now; // update percentage count
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
            /* One last call to fwrite() remains for any leftover bytes, i.e. there is still room for failure.
             *
             * However, we need only call fwrite() if there's actually anything to write. If there's 0 bytes left, avoid unnecessary
             * statements - the && operator helps - if there's nothing to be written , the fwrite is never executed, as that condition is not evaluated. 
             *
             * If fwrite fails, avoid repetitive/boilerplate code, use goto or return -1 as the error; it's handling is EXACTLY the same as we *just* handled.
             * Note : failed_fwrite ends with "return -1;" ; no infinite goto bug.
             */

            if(bytes_read && fwrite(buffer,1,bytes_read,to) != bytes_read)
                #ifdef PRINT_MID_IO_ERROR
                goto failed_fwrite;
                #else 
                return -1;
                #endif

            /* Done copying streams,no errors yet, attempt fclose'ing them.
             *
             * This writes remaining data to disk for destination stream, so check for any errors.
             *
             * Did not check fclosing source stream, nothing can be done about it anyway, and shouldn't lead to data loss, as it was merely read.
             */

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

# ByteCopy v3.7.1

### About
ByteCopy , or BCP, is both a utility + a tiny library/API for copying a file's contents to another in a _simple , safe and efficient manner_. There's no malloc, and standardised `FILE` streams are used , and none are ever left open.
 
It's functionality as a utility overlaps in parts with that of `dd`, but it is not a clone. 
 
`dd` uses _low-level_ system calls , non-standard C, has elaborate, (_tiresome ?_) syntax, and is _notorious_ for failing in mysterious or dangerous ways in inexperienced hands. This is not a _defect_ per se; `dd` is an extremely optimised, advanced and very precise tool. It was originally written in the times of tape drives, which is also perhaps the reason for some of its peculiarities.
 
However, `dd` has become the default for doing everyday tasks : backups of files, disks, flashing ISOs, etc - sometimes causing serious issues for casual users. In such cases, `bcp` functions _alike_ `dd` , with a dead simple syntax , and is _much_ easier to troubleshoot and understand - while also keeping up with dd's speed  and being significantly more reliable, easier to casually use and port to _any_ OS/platform (stlib's `exit()`,`stdio`,`stdint` *only* required). BCP displays progress _by default_ where possible, asks before overwriting, etc.
 
### Utility Usage
_If you can use `cp` , you can use BCP !_
 
**Usage : `bcp <source file> [destination file (optional,stdout default)] [overwrite ? (y/n) (optional,will ask for overwrite permission if destination exists)]`**
 
So, BCP **_must_** be given _at least_ one argument , the name of the source file.
 
If given no other arguments, bcp will copy to stdout, meaning you can _redirect_ this further to other programs.

If given another argument, the second argument is treated as the name of the destination file, and the third (_if given_) is treated as permission for overwriting : this can be "y" or "yes" to confirm or "n" or "no" to deny. If an output file is given but overwrite permissions are not, BCP will ask you _explicitly_ before overwriting.
 
NOTES :

_If this destination file does not exist, it is created._

_BCP will treat arguments *exactly* in the order specified above._

_Any arguments beyond the first three, and the third if invalid, are ignored automatically_.
  
Examples :
```
bcp ~/.zshrc  /mnt/backups/zhrc.bak
	
bcp /dev/sda | gzip > drive.bak
	
bcp settings.json D:\backups\settings.json
```

### Library/API Usage
BCP isn't really an API/Library , those are full of wide, complicated spaghetti code. What BCP does provide is one header file and one `.c` file which if included with you code give you access to the same functionality as using the utility in the shell. The interface is described below in breif, though it is reccomended you look at the source before actually using it.

Functions :
```c
/* Macros : Control behaviour of functions. */
MID_IO_ERROR           /* If defined, error message is printed to stderr if/when ferror() indicates error abruptly
			* when reading and writing. */
PRINT_OPEN_CLOSE_ERROR /* If defined, error message is printed to stderr if/when any fopen()/fclose() calls fail. */
PRINT_PROGRESS         /* If defined, fbcp() prints and updates percentage progress of copying to stdout if source file is > 50
			* bytes and a 64-bit size/offset type is availible. */
DEFAULT_VERBOSITY      /* If defines, defines above 3 macros. */
ABORT_IF_OVERWRITING   /* If defined, causes bcp() (through overwrite_chk()) to fail if destination file pre-exists */
ASK_BEFORE_OVERWRITE   /* If defined, causes bcp() (through overwrite_chk()) to ask user for permission if destination file pre-exists. */
PRINT_OVERWRITE_ABORT  /* If defined, bcp() will print an error message to stderr informing user of aborting to prevent overwriting
                        * if aborting due to overwriting protections. */
OVERWRITE_PROTECT      /* If defined, ASK_BEFORE_OVERWRITE and PRNT_OVERWRITE_ABORT are defined */

/* 1st class functions : Functions designed to be called by you */
int fbcp(FILE *from, FILE *to)          /* Copies all data until EOF from first argument to second argument. Doesn't close or seek either streams.
			                 * Returns : 0 for success, -1 if fread() on source failed and ferror() returned non-zero,
				         *  -2 if fwrite() or both fread() and fwrite() failed and ferror() returned non-zero. */
int bcp(char *source, char *destination) /* Calls overwrite_chk on destination, attempts to opens source for "rb" and destination for "wb" ,
					  * passes the opened streams to fbcp() and closes them once done. Returns : 0 on success,
					  * 1 if overwrite_chk() returned non-zero, -1 & -2 like fbcp(), -3 if closing destinantion stream fails
					  * (possible that -1, -2 also happened), -4 upon error openeing source and/or desitnation stream(s). */


/* 2nd class functions : Functions designed for being called by 1st class functions but availible for you use. */

int_fast64_t approx(long double) /* Approximates and returns any long double to nearest 64-bit signed int with least overhead. */
int_fast64_t getsize(FILE *)     /* Return size of given FILE * in bytes. Size value is reliable on Windows and UNIX and
                                  * indeterminate on other systems.If the given FILE * is not at BOF when passed, size is relative to it's
				  * current position on passing. FILE * is always reset to BOF */
int overwrite_chk(char *)        /* Returns 0 if given filepath is okay to write to. If #ABORT_IF_OVERWRITING is defined, returns 1 if
				  * filepath pre-exists and 0 if it does not. If ASK_BEFORE_OVERWRITE is defined, asks the user for permission to
				  * overwrite if filepath pre-exists; if permission given or file did not pre-exist, returns 0, else returns 2.
				  * If neither is defined, always returns 0. */ 

/* Status variables : Useful for error handling */
uint_fast64_t bytes_processed /* Number of bytes successfully processed with BOTH fread+fwrite */
uint_fast64_t bytes_read      /* Number of bytes successfully fread() */
```

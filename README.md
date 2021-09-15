# ByteCopy v4

### About
ByteCopy , or `bcp`, is both a tiny utility + a tiny library/API for copying a stream's contents to another in a _simple , safe and efficient manner_. There's no malloc, and standardised `FILE` streams are used.
 
It's functionality as a utility overlaps in parts with that of `dd`, but it is not a clone; it is a bit slower , significantly simpler, more reliable & portable.
 
### Utility Usage
_If you can use `cp` , you can use BCP !_
 
1. [Dowload](https://download-directory.github.io/?url=https%3A%2F%2Fgithub.com%2Fa-p-jo%2FByteCopy%2Ftree%2Fmain%2Fsrc) the `src` directory. Compile all `.c` files in it. For example, on GNU/Linux or macOS , you might run :
```sh
cc src/bcp.c src/main.c -O3 -flto -o bcp
```
2. Get the help menu :
```
$ bcp -h
bcp v4 : A simple, safe, performant and reliable file copying program.
Usage   : bcp <from> [to (optional, stdout by default)] [overwrite to? (y/n) (optional)]

Explanation : Copy the contents of <from> file to [to] file if given, else to stdout.
If [to] does not exist, create it. If it pre-exists, if 'y' is specified, continue.
If 'n' is specified, abort. Else, ask for permission before overwriting.

Example : bcp archlinux.iso /dev/sda y
```
NOTES :

- _`bcp` will treat arguments *exactly* in the order specified above._

- _Any arguments beyond the first three, and the third if invalid, are ignored automatically_.
  
Examples :
```
bcp ~/.zshrc  /mnt/backups/zhrc.bak
	
bcp /dev/sda | gzip > drive.bak
	
bcp settings.json D:\backups\settings.json
```

### Use it in your code !
```c
#define PRINT_PROGRESS stderr /* If defined, fbcp() writes progress every 1% to the stream */
#define BUFSZ (64*1024) /* 64 KiB */

/* Bytes written & synced to destination stream in last successful call to fbcp() */ 
extern uintmax_t bytes_processed;

enum retval {
	SUCCESS,
	/* From stream was seekable to SEEK_END, but reseting to initial position failed. 
	 * Cannot occur if PRINT_PROGRESS undefined.
	 */
	FROM_SEEK_ERR,
	/* Reading the from stream failed and ferror(from) returned true. 
	 * No error in to stream.
	 */
	FERROR_FROM,
	/* Writing the to stream failed and ferror(to) returned true. 
	 * No error in from stream.
	 */
	FERROR_TO,
	/* Reading the from stream & writing the to stream failed and ferror() returned true for both */
	FERROR_BOTH,
	/* Flushing the to stream failed */
	FFLUSH_ERR }; 

/* Copies contents of from stream onto to stream 
 * until specified number of bytes or EOF, whichever comes first,
 * and flushes it.
 * If bytes argument is 0, copies till EOF.
 *
 * Copies in blocks of BUFSZ bytes. 
 * If PRINT_PROGRESS defined, the from stream is seekable to SEEK_END, and data to copy is > 100 bytes,
 * then displays progress of copy operation in percentage on stdout.
 */
enum retval fbcp(FILE *restrict from, FILE *restrict to, uintmax_t bytes);
```
NOTES :
- _Passed `FILE *`s must be unique, non-NULL pointers returned previously by `fopen()` and not yet passed to `fclose()` or implicitly closed by similar means, else behaviour is undefined._
- _Copied data is read starting from ` FILE *from`'s `SEEK_CUR` position, and written starting from `FILE *to`'s `SEEK_CUR` position._

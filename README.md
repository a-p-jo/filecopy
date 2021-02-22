# ByteCopy v2.5
 
### About
ByteCopy , or BCP, intends to copy files accurately (_down to the bytes_) in a _simple , safe and efficient manner_.
 
It's functionality , but not implementation, overlaps with that of `dd` , and this is by design. 
 
`dd` uses _low-level_ system calls , non-standard C, has elaborate, (_tiresome ?_) syntax, and is _notorious_ for failing in mysterious or dangerous ways in inexperienced hands. This is not a _defect_ , `dd` is a _very precise_ and fast tool. It was originally written in the times of tape drives, which is also perhaps the reason for some of its peculiarities.
 
However, `dd` has become the default for doing backups of files, disks, flashing ISOs, etc - somtimes causing serious issues for casual users. In such cases, `bcp` functions _alike_ `dd` , with the simple syntax of `cp` , keeping up with dd's speed by < 1 second - while being reliable, easier to casually use and port to _any_ OS/platform. BCP also displays progress _by default_ !
 
_BCP does NOT replace or clone `dd` in the least - `dd` is it's own low-level, advanced utility, and is certainly the go-to for advanced control over the copying; BCP is for the other 90% of the time._
 
### Usage
_If you can use `cp` , you can use BCP !_
 
**Usage : `bcp [source file] [destination file (optional)]`**
 
So, BCP **_must_** be given _at least_ one argument , the name of the source file.
 
If given no other arguments, bcp will copy to stdout, meaning you can _redirect_ this further to other programs !

If given another argument, the second argument is treated as the filename of the destination file.
 
NOTES :
_If this destination file does not exist, it is created. If it does, it is OVERWRITTEN !_

_BCP will treat arguments *exactly* in the order specified above. Do NOT mix it up !_

_Any arguments beyond the first two are discarded automatically_.
  
Examples :
	`bcp ~/.zshrc  /mnt/backups/zhrc.bak`
	
	`bcp /dev/sda | gzip > drive.bak`
	
	`bcp settings.json D:\backups\settings.json`
	
	and so on, and so forth.
 
### API
`bcp.h` contains the C function `bcp()`, which allows you to incorporate the mechanisms of BCP _within_ your program easily, just by doing an `#include "bcp.h"` and calling `bcp(...)`.
 
_Very soon_, there will be another `fbcp()` in `bcp.h` , which will work not with filenames, but rather streams (`FILE *`s).
 
**Overview of `bcp()` :**
 
Prototype : `int bcp(char * source, char * source, char * destination, char * if_only_32_bit_offset)`
 
Meaning ,
```c
/*
 * char * source : Filename of source file.
 * char * destination : Filename of destination file.
 * char * if_only_32_bit_offset : Do you wish to abort if offset is not 64-bit ?  (Matters only if you need to print progress while copying)
 *	If *if_only_32_bit_offset == 'y' || 'Y' , bcp() will return 1 and abort. Else, it will continue anyways.
 */
```
With the following return values :
```c
/*
 * 0 : Success
 *-1 : Read/Write error mid-I/O (copying abandoned mid-way)
 *-2 : Error fclose()'ing destination file
 *-3 : Error opening destination/source
 *1 : Aborted without copying when encountering a non-64 bit offset (Only if you need to print progress as copying)
 */
```	
###### BCP is actively-maintained, FLOSS , minimal code that is _not complete yet_. Once, complete, binaries will be released. I hope you find good use for it !

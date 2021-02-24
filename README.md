# ByteCopy v3.6
 
### About
ByteCopy , or BCP, intends to copy files accurately (_down to the bytes_) in a _simple , safe and efficient manner_.
 
It's functionality , but not implementation, overlaps with that of `dd` , and this is by design. 
 
`dd` uses _low-level_ system calls , non-standard C, has elaborate, (_tiresome ?_) syntax, and is _notorious_ for failing in mysterious or dangerous ways in inexperienced hands. This is not a _defect_ per se; `dd` is an _advanced and very precise_ tool. It was originally written in the times of tape drives, which is also perhaps the reason for some of its peculiarities.
 
However, `dd` has become the default for doing everyday tasks : backups of files, disks, flashing ISOs, etc - sometimes causing serious issues for casual users. In such cases, `bcp` functions _alike_ `dd` , with the simple syntax of `cp` , and is _much_ easier to troubleshoot and understand - while also keeping up with dd's speed by < 1 second and being more reliable, easier to casually use and port to _any_ OS/platform. BCP displays progress _by default_, asks before overwriting, etc.
 
_BCP does NOT replace or clone `dd` in the least - `dd` is it's own low-level, advanced utility, and is certainly the go-to for advanced control over the copying; BCP is for the other 90% of the time :)_
 
### Usage
_If you can use `cp` , you can use BCP !_
 
**Usage : `bcp [source file] [destination file (optional)] [overwrite ? (y/n) (optional)]`**
 
So, BCP **_must_** be given _at least_ one argument , the name of the source file.
 
If given no other arguments, bcp will copy to stdout, meaning you can _redirect_ this further to other programs.

If given another argument, the second argument is treated as the name of the destination file, and the third (_if given_) is treated as permission for overwriting : this can be "y" or "yes" to confirm or "n" or "no" to deny. If an output file is given but overwrite permissions are not, BCP will ask you _explicitly_ before overwriting.
 
NOTES :

_If this destination file does not exist, it is created. If it preexists, it is OVERWRITTEN !_

_BCP will treat arguments *exactly* in the order specified above. Do NOT mix it up !_

_Any arguments beyond the first three, and the third if invalid, are ignored automatically_.
  
Examples :
```
bcp ~/.zshrc  /mnt/backups/zhrc.bak
	
bcp /dev/sda | gzip > drive.bak
	
bcp settings.json D:\backups\settings.json
```	
###### You can `#include bcp.h` + compile `bcp.c`to incorporate BCP functionality in your own C code ! In fact, our `main.c` is just ~ 60 lines long !

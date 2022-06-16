# filecopy v1.0

### About
filecopy is both a tiny utility + a tiny library/API for copying a stream's contents to another in a _simple , safe and efficient_ manner.
 
### Utility Usage
_If you can use `cp` , you can use `filecopy` !_
 
1. Clone the repo, compile all `.c` files in it. For example, on GNU/Linux or macOS , you might run :
```sh
git clone https://github.com/a-p-jo/filecopy
cc filecopy/*.c -o filecopy -Ofast -flto
```
2. Get the help menu :
```
$ filecopy
filecopy v1.0 : A simple, safe, performant and reliable file copying program.
Usage : filecopy <src> [dst (optional, stdout by default)] [overwrite dst? (y/n) (optional)]

Copies the contents of src to dst if given, else to stdout.
If dst does not exist, creates it. If it pre-exists, if 'y' is specified, continues.
If 'n' is specified, aborts. Else, asks for permission before overwriting.

Example : filecopy archlinux.iso /dev/sda y
```
NOTES :

- _`filecopy` will treat arguments *exactly* in the order specified above._

- _Any arguments beyond the first three, and the third if invalid, are ignored automatically_.
  
Examples :
```
filecopy ~/.zshrc  /mnt/backups/zhrc.bak y
	
filecopy /dev/sda | gzip > drive.bak
	
filecopy settings.json D:\backups\settings.json
```

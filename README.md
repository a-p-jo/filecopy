## Utility Usage

1. Compile all `.c` files in the repo. For example, on \*nix systems , you might run :
```sh
git clone https://github.com/a-p-jo/filecopy.git
cc filecopy/*.c -o fcp -Ofast -flto # Compile to exe named "fcp"
rm -r filecopy # Repo no longer needed, delete it
```
2. Get the help menu :
```
$ ./fcp
fcp v1.0
Usage : filecopy <src> [dst (optional, stdout by default)] [overwrite dst? (y/n) (optional)]

Copies the contents of src to dst (if given, else to stdout).
If dst does not exist, creates it. If it pre-exists, if 'y' is specified, continues.
If 'n' is specified, aborts. Else, asks for permission before overwriting.

Example : fcp archlinux.iso /dev/sda y
```

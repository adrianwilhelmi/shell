# Semi-advanced UNIX shell
## Overview
Semi-advanced UNIX shell with job control, executing commands using fork and execvp
## Functionalities
* Waits for input from stdin and executes given commands until EOF
* Pipelines in `cmmd1 | cmmd2 | ... | cmmdn` format
* Redirections in `cmmd1 < cmmd2 | ... | cmmd3 > cmmd4` format
* Accepts built-in commands
  + cd
  + exit
  + fg
  + bg
  + jobs
* Job control (^Z, fg, bg)
* Globbing (*, ^, [], etc.)
* Access to previous commands using up and down arrow
* Simple implementation of grep (sgrep command)
* Interactive mode only
* Run process in background by putting & at the end

## Requirements
* Linux distribution
* Any C compiler
* make

## Installation, compilation and usage

* Download repository to the desired directory
* Run `make` to compile
* Prompt `./shell` to run the appication

### Example
Running `ls | wc -w > file.txt` will count files in current directory and save it in file.txt

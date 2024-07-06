# Linux shell

## Install required libraries
Examples are for debian-based systems.
1. Make and gcc: ```sudo apt install build-essential```
2. Readline: ```sudo apt-get install libreadline-dev```

## Compile and run the program
```
cd code
make
./lsh
```

## Report of the project
Read ./docs/report.md

## Functions and implementation

### 1. Run binary programs like `ls`, `date` and `who`
This is implemented in `void exec_pgm(Pgm *pgm)`. The unix function used to do this is `execvp` in `unistd.h`. Filename and arguments need to be passed to this function. This function will try to find the binary file in `/usr/bin`, `/usr/local/bin` and other directories specified in `PATH`.

### 2. Support the use of one or more pipes e.g. `ls | grep out | wc -w`

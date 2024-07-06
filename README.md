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
This is implemented in `RunCommand` and `void exec_pgm(Pgm *pgm)`. The unix function used to do this is `execvp` in `unistd.h`. Filename and arguments need to be passed to this function. This function will try to find the binary file in `/usr/bin`, `/usr/local/bin` and other directories specified in `PATH`. `fork` function is used to create a child process and the binary program is executed in the child process.

### 2. Support the use of one or more pipes e.g. `ls | grep out | wc -w`
This is implemented in `pipe_pgm` and the function is called in `RunCommand` function. The function is called recursively to implement multiple pipes. In every step, a new child process is created to execute a previous program in the pipe. The parent process and child process communicate with `pipe()` function. `stdin` file discripter for parent process needs to be changed to `read end` of the pipe and `stdout` for child process needs to be changed to `write end` of the pipe, because a process uses `stdin` and `stdout` to execute a program by default. `stdin` and `stdout` don't need to be changed back because file descriptors only exist when the process is alive.

### 3. Allow redirection of the standard input and output to file, e.g. `wc -l < /etc/passwd > accounts`
This is implemented in `RunCommand` function by changing `stdin` and `stdout` before and after `fork` in the shell process.
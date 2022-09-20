# Report

This is the report for Lab1 of team 69.

## Implementation according to Specifications

### Section 1

For the implementation of simple commands execution, we worked inside the `RunCommand` function.
With a call to `fork`, we duplicated the current process, then identified which of the two resulting processes was the child, and which was the parent.
The parent process simply waits on the child to terminate with `waitpid`.
Meanwhile, the child calls `execvp` to change its process image with the provided program. The `execvp` function has the advantage of searching the PATH environment variable for executables.

### Section 2



### Section 3

Implementing pipes lead us to reorganize the code a little bit.
Instead of having the `RunCommand` function calling `execvp` directly from the child process, a newly created function `pipe_pgm` is called instead.
The `pipe_pgm` function first create a simple pipe with a read and a write end with the help of `pipe`.
Then a new `fork` happens, where the parent process will execute the last program in the pipe (because it depends on all the rest of pipeline).
The Parent hence redirects `stdin` to the read end of the pipe (except if it happens to be the first program in the pipeline), and then waits for its child processes to complete before eventually calling `execvp` to execute.
The child will redirect `stdout` to the write end of the pipe, hence sending its output to the parent, and will, then recursively call `pipe_pgm` to continue the construction of the pipeline (except if its parent was the first program in the pipeline, of course).

### Section 4

To Implement standard io redirection, we had to parse the attributes of the command at the beginning of `RunCommand` to see if any redirection was needed.
In the event of a redirection of `stdin`, we first open the provided file using `open`, then we replace the `stdin` file descriptor (fd) by a copy of the input file fd so that the content of the file is read from `stdin`.
After the command has been run, we restore the standard input with a backup we made of the `stdin` fd, and close all the file descriptors no longer in use.
Redirection of `stdout` works in much the same way, except that we create the output file if it doesn't already exists with the `O_CREAT` flag in `open`.

### Section 5



### Section 6



### Section 7



## Self-test examples

## 4.1.1

```console
$ date
Tue 20 Sep 2022 06:51:45 PM CEST
$  hello
lsh: exec_pgm: : No such file or directory
```

The second program does not seem to be present on the remote computers, which makes the second program fail.
The child processes terminate after their program has exited.

## 4.1.2

```console
$ ls -al -p
total 201
drwxrwxrwx 2 nathanfe nathanfe    17 Sep 20 18:55 ./
drwxrwxrwx 5 nathanfe nathanfe     7 Sep 20 18:21 ../
-rwxr-xr-x 1 nathanfe nathanfe  6148 Sep 20 18:21 .DS_Store
-rwxr-xr-x 1 nathanfe nathanfe 39504 Sep 20 18:21 .fuse_hidden0000030500000001
-rwxr-xr-x 1 nathanfe nathanfe     8 Sep 20 18:21 .gitignore
-rwxr-x--- 1 nathanfe nathanfe 35424 Sep 20 18:55 lsh
-rwxr-xr-x 1 nathanfe nathanfe 13028 Sep 20 18:55 lsh.c
-rw-r----- 1 nathanfe nathanfe 27184 Sep 20 18:55 lsh.o
-rwxr-xr-x 1 nathanfe nathanfe   383 Sep 20 18:21 Makefile
-rwxr-xr-x 1 nathanfe nathanfe   647 Sep 20 18:21 out2.txt
-rwxr-xr-x 1 nathanfe nathanfe   959 Sep 20 18:21 out.txt
-rwxr-xr-x 1 nathanfe nathanfe  3152 Sep 20 18:21 parse.c
-rwxr-xr-x 1 nathanfe nathanfe   343 Sep 20 18:21 parse.h
-rwxr-xr-x 1 nathanfe nathanfe 11432 Sep 20 18:21 parse.o
-rwx------ 1 nathanfe nathanfe   887 Sep 20 18:30 tmp.1
-rwx------ 1 nathanfe nathanfe   887 Sep 20 18:30 tmp.2
-rw-r----- 1 nathanfe nathanfe    24 Sep 20 18:40 tmp.3
```

## 4.1.3

```console
$ ls -al > tmp.1
$ cat < tmp.1 > tmp.2
$ diff tmp.1 tmp.2
```

The output of `diff` is as empty as the difference between two identical files, which is to be expected.

## 4.1.4

```console
$ sleep 60 &
$ sleep 60 &
$ sleep 60
^C
$ ps
  PID TTY          TIME CMD
  961 pts/41   00:00:00 bash
 6337 pts/41   00:00:00 lsh
25870 pts/41   00:00:00 sleep
25936 pts/41   00:00:00 sleep
26599 pts/41   00:00:00 ps
```

When pressing Ctrl-C, the foreground process stops. However, the background processes are still running.

## 4.1.5

```console
$ ls -al | wc -w
155
$ ls -al | wc
     18     155     999
$ ls | grep lsh | sort -r
lsh.o
lsh.c
lsh
```

The prompt appears as expected after the output of all the above commands.

```console
$ ls | wc &
[1] 17093
$      12      12      84

```

The prompt reappears before the output of the background process.

```console
$ cat < tmp.1 | wc > tmp.3
$ cat tmp.1 | wc
     18     155     999
$ cat tmp.3
     18     155     999
```

Yes, they are the same. This is because tmp.3 contains the output out `cat < tmp.1 | wc`, which is equivalent to `cat tmp.1 | wc`.

```console
$ abf | wc
lsh: exec_pgm: : No such file or directory
      0       0       0
$ ls | abf
lsh: exec_pgm: : No such file or directory
$ grep apa | ls
lsh    lsh.o	 out2.txt  parse.c  parse.o  tmp.2
lsh.c  Makefile  out.txt   parse.h  tmp.1    tmp.3
```

The prompt appears upon termination of the foreground processes. `grep` terminates when it receives the End Of File signal (Ctrl-D). That is because reaching EOF means that it has finished reading from `stdin`.

## 4.1.6

```console
$ cd ..
$ cd lab1
Error in cd chdir: No such file or directory
$ cd tmp.tmp
Error in cd chdir: No such file or directory
```

Yes, errors were returned when the target directory couldn't be found.

```console
$ cd ..
$ cd lab1 | abf
lsh: exec_pgm: : No such file or directory
$ ls
code  docs  prepare-submission.sh  README.md
```

Yes, the `ls` command worked.

```console
$ cd
$ pwd
/chalmers/users/nathanfe
```

No errors.

```console
$ grep exit < tmp.1
```

It considered `exit` as a text string to find in a file.

```console
$   exit
```

Here it did exit, due to the use of `stripwhite()` command.

```console
$ grep exit | hej
^D
lsh: exec_pgm: : No such file or directory
```

Grep waits for `stdin`, then we get an error after EOF signal.

```console
$ grep cd | wc
^D
      0       0       0
```

Same as before, except no errors are returned.

```
$ exit
$ ps
  PID TTY          TIME CMD
  961 pts/41   00:00:00 bash
17405 pts/41   00:00:00 ps
```

Exits successfully, and no zombies to be found.

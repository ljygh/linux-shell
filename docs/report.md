# Report

This is the report for Lab1 of team 69.

## Implementation according to Specifications

### Section 1

For the implementation of simple commands execution, we worked inside the `RunCommand` function.
With a call to `fork`, we duplicated the current process, then identified which of the two resulting processes was the child, and which was the parent.
The parent process simply waits on the child to terminate with `waitpid`.
Meanwhile, the child calls `execvp` to change its process image with the provided program. The `execvp` function has the advantage of searching the PATH environment variable for executables.

### Section 2

Implemation of running command in the background.
`Runcommand` function is used to run a command. In this function, a child process is created using `fork` and the command runs  in the child process. In the parent process, it check `cmd.background`. If `background` is false, the parent process waits for the child until the child exits. In this situation, the shell waits for the child and blocks. For user, the command is running in foreground. On the other hand, if the `background` is true, the parent doesn't wait so that the parent can continue and interact with user. So, for the user, the command is running in the background.  In order to prevent the child from being a zombie, the function `CHLDHandler` is declared and if a child terminates, this function will get the CHLD signal and handle the terminated child by using `waitpid`. This handler is set for the main process in the beginning of `main` function.
In order to print information of background jobs(processes) while creating and terminating a bg(background) job and manage bg jobs, a linked list of struct `bg_job` is used to store info of bg jobs. Head and tail of this list are initialized in `main` function. In the parent process of `Runcommand`, if `background` is true, a new struct `bg_job` is created and add to the list. Then, info of this new bg job is printed. After a child terminated, in `CHLDHandler`, bg_job of this child is removed from the list according to its pid and its info is added to `bg_job_message`. Before every new cycle of the command line, the message will be printed. The command `jobs` is implemented, when user input the `jobs` command, all info of bg jobs will be printed by the function `print_bg_jobs`. The user can also view info of a bg job specified by its job number like `jobs 1`. Then the function `print_bg_job` will print info of this specific bg job.


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

Implementation of cd and exit.
`cd` function is used to implement command cd. In the beginning of this function, current working directory `cwd` and home directory `homeDir` are got. Afterwards, it checks the argument after cd. If the argument is null, it changes the cwd(current working directory) to `homeDir` by calling the function `chdir`. And for '-' and '~', it respectively changes cwd to last cwd(null in the beginning and is set at the end of each `cd`) and `homeDir`. Else, it converts 'cmd.pgm.pgmlist' to a string of dir and uses `chdir` to change cwd to this dir. The entry of `cd` function is both in the `RunCommand` and `pipe_pgm`. If cd is used with `|`, it works in child process of `pipe_pgm`. Otherwise, it works in the main process and changes cwd of the shell.

Exit is implemented in `RunCommand`. If the command starts with 'exit', it will call `killAllBgp` function to send SIGKILL to all processes in this process group. `SIGKILLHandler` is set for bg child processes in `RunCommand` function's child process part. This will be automatically inherited to bg processes's children too. So, when the main process sends SIGKILL signal, all bg processes will receive this signal because they are all in the group of the main process. Then, they will run `SIGKILLHandler` function to wait for their all child processes until they terminate and exit. In the function `killAllBgp`, main process waits for all its child processes until they terminate, too. Afterwards, the main process exit.

### Section 6

Implementation of ctrl c.
`INTHandler` function is declared. If it is set to a process. When the user presses ctrl c, the process will receive INT signal, wait for all its child processes until they terminate and exit. In `RumCommand` function, this INTHandler is set to the child process if `background` is false. And automatically, the handler will be inherited to all child processes of this child process. So, if ctrl c is pressed, all foreground processes will run `INTHandler`. If they don't have child, they terminate directly. Otherwise, they wait and handle all terminations of their child processes first and then, exit. Therefore, all foreground processes will terminate from the youngest one to the oldest one.
`SIG_IGN` is set to be the handler of SIGINT signal for the main process to ignore the SIGINT. Therefore, when ctrl c is pressed, the main process won't terminate.

### Section 7
`SIG_IGN` is set to be the handler of SIGINT signal for the main process to ignore the SIGINT. And, for bg processes, no new INTHandler is set to them. So, they inherit the SIGINT handler from the main process and ignore the SIGINT, too. Therefore, when ctrl c is pressed, both the main process and bg processes won't terminate.


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

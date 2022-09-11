/* 
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file 
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive: 
      $> tar cvf lab1.tgz lab1/
 *
 * All the best 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"

#include <unistd.h>

#define TRUE 1
#define FALSE 0

#define CWD_MAX_LENGTH 128
#define HOST_MAX_LENGTH 32
#define READ_END 0
#define WRITE_END 1

void RunCommand(int, Command *);
void DebugPrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);
void cd(char** pgm);
char* pwd();

void pipe_pgm(Pgm *pgm);
void exec_pgm(Pgm *pgm);

int main(void)
{
  Command cmd;
  int parse_result;

  while (TRUE)
  {
	char cwd[CWD_MAX_LENGTH];
	char hostname[HOST_MAX_LENGTH];
	getcwd(cwd, CWD_MAX_LENGTH);
	gethostname(hostname, HOST_MAX_LENGTH);
	printf("%s@%s:%s ", getlogin(), hostname, cwd);

    char *line;
    line = readline("> ");

    /* If EOF encountered, exit shell */
    if (!line)
    {
      break;
    }
    /* Remove leading and trailing whitespace from the line */
    stripwhite(line);
    /* If stripped line not blank */
    if (*line)
    {
      add_history(line);
      parse_result = parse(line, &cmd);
      RunCommand(parse_result, &cmd);
    }

    /* Clear memory */
    free(line);
  }
  return 0;
}

/* Execute the given command(s).

 * Note: The function currently only prints the command(s).
 * 
 * TODO: 
 * 1. Implement this function so that it executes the given command(s).
 * 2. Remove the debug printing before the final submission.
 */
void RunCommand(int parse_result, Command *cmd)
{
	DebugPrintCommand(parse_result, cmd);

	int stdin_fd;
	int stdout_fd;

	if (cmd->rstdin != NULL) {
		// Open the input file in read mode
		int fd_in = open(cmd->rstdin, O_RDONLY);
		// Backup the stdin file descriptor
		stdin_fd = dup(STDIN_FILENO);
		// Replace the stdin fd by a copy of the input file fd
		dup2(fd_in, STDIN_FILENO);
		// Close the original input file fd
		close(fd_in);
	}

	if (cmd->rstdout != NULL) {
		// Open the output file in write mode
		int fd_out = open(cmd->rstdout, O_WRONLY | O_CREAT | O_TRUNC, 0);
		// Backup the stdout file descriptor
		stdout_fd = dup(STDOUT_FILENO);
		// Replace the stdout fd by a copy of the output file fd
		dup2(fd_out, STDOUT_FILENO);
		// Close the original output file fd
		close(fd_out);
	}

	pid_t pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Fork failed");
		return;
	}

	if (pid == 0) { // Child process
		pipe_pgm(cmd->pgm);
	} else { // Parent process
		wait(NULL);
	}

	if (cmd->rstdin != NULL) {
		// Restore the original stdin fd
		dup2(stdin_fd, STDIN_FILENO);
		// Close the backup file descriptor
		close(stdin_fd);
	}

	if (cmd->rstdout != NULL) {
		// Restore the original stdout fd
		dup2(stdout_fd, STDOUT_FILENO);
		// Close the backup file descriptor
		close(stdout_fd);
	}
}

/*
 * Recursively construct a sequencial program pipeline.
 * Fork front of the pipeline to a child process and wait for completion before executing the given program.
 */ 
void pipe_pgm(Pgm *pgm) {
	int fd[2];

	// Open pipe
	if (pipe(fd) == -1) {
		fprintf(stderr, "Pipe failed");
		return;
	}

	// Fork the process
	pid_t pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Fork failed");
		return;
	}

	if (pid == 0) { // Child process
		
		// Redirect stdout to write end of the pipe
		dup2(fd[WRITE_END], STDOUT_FILENO);

		// Close the no longer needed pipe ends
		close(fd[READ_END]);
		close(fd[WRITE_END]);

		if (pgm->next == NULL) {
			// Exit the process if there is no next program to pipe to
			exit(0);
		}
		
		// Pipe to the next program
		pipe_pgm(pgm->next);
	} else { // Parent process

		if (pgm->next != NULL) {
			// Redirect stdin to read end of the pipe (only if not the first program of the pipe)
			dup2(fd[READ_END], STDIN_FILENO);
		}

		// Close the no longer needed pipe ends
		close(fd[READ_END]);
		close(fd[WRITE_END]);

		// Wait for children process to complete
		wait(NULL);

		// Then execute
		exec_pgm(pgm);
	}

}

/* 
 * Execute the given program, searching the $PATH if no path is given.
 */
void exec_pgm(Pgm *pgm) {
	char *filename = pgm->pgmlist[0];
	char **argv = pgm->pgmlist;
	execvp(filename, argv);
}

void cd(char** pgm){
  const char* dir = *(pgm + 1);
  chdir(dir);
}

char* pwd(){
  char buffer[800];
  size_t buff_size = 800;
  getcwd(buffer, buff_size);
  printf("%s\n", buffer);
}




/* 
 * Print a Command structure as returned by parse on stdout. 
 * 
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
void DebugPrintCommand(int parse_result, Command *cmd)
{
  if (parse_result != 1) {
    printf("Parse ERROR\n");
    return;
  }
  printf("------------------------------\n");
  printf("Parse OK\n");
  printf("stdin:      %s\n", cmd->rstdin ? cmd->rstdin : "<none>");
  printf("stdout:     %s\n", cmd->rstdout ? cmd->rstdout : "<none>");
  printf("background: %s\n", cmd->background ? "true" : "false");
  printf("Pgms:\n");
  PrintPgm(cmd->pgm);
  printf("------------------------------\n");
}


/* Print a (linked) list of Pgm:s.
 * 
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
void PrintPgm(Pgm *p)
{
  if (p == NULL)
  {
    return;
  }
  else
  {
    char **pl = p->pgmlist;

    /* The list is in reversed order so print
     * it reversed to get right
     */
    PrintPgm(p->next);
    printf("            * [ ");
    while (*pl)
    {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}


/* Strip whitespace from the start and end of a string. 
 *
 * Helper function, no need to change.
 */
void stripwhite(char *string)
{
  register int i = 0;

  while (isspace(string[i]))
  {
    i++;
  }

  if (i)
  {
    memmove(string, string + i, strlen(string + i) + 1);
  }

  i = strlen(string) - 1;
  while (i > 0 && isspace(string[i]))
  {
    i--;
  }

  string[++i] = '\0';
}

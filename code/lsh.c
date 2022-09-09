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
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"

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

void pipe_pgm(Pgm *pgm);
void exec_pgm(Pgm *pgm);

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
}

void pipe_pgm(Pgm *pgm) {
	int fd[2];

	// Open pipe
	if (pipe(fd) == -1) {
		fprintf(stderr, "Pipe failed");
		return;
	}

	pid_t pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Fork failed");
		return;
	}

	if (pid == 0) { // Child process
		
		// Redirect write end of the pipe to stdout
		dup2(fd[WRITE_END], STDOUT_FILENO);

		// Close the no longer needed pipe ends
		close(fd[READ_END]);
		close(fd[WRITE_END]);

		// Pipe to next command if it exists
		if (pgm->next == NULL) {
			exit(0);
		}
		
		pipe_pgm(pgm->next);
	} else { // Parent process

		// Redirect read end of the pipe to stdin
		dup2(fd[READ_END], STDIN_FILENO);

		// Close the no longer needed pipe ends
		close(fd[READ_END]);
		close(fd[WRITE_END]);

		wait(NULL);
		exec_pgm(pgm);
	}

}

void exec_pgm(Pgm *pgm) {
	char *filename = pgm->pgmlist[0];
	char **argv = pgm->pgmlist;
	execvp(filename, argv);
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

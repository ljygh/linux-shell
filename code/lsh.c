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

#include <errno.h>
#include <signal.h>


#define TRUE 1
#define FALSE 0

#define CWD_MAX_LENGTH 128
#define HOST_MAX_LENGTH 32
#define READ_END 0
#define WRITE_END 1

char lastCwd[CWD_MAX_LENGTH] = ""; // Declare a global variable for last current working directory.

// Def a struct for background processes.
typedef struct bg_job
{
  int job_id;
  int pid;
  char* cmd; // process command
  struct bg_job *next; // next background process
} bg_job;

void RunCommand(int, Command *);
void DebugPrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);

void cd(char** pgmlist);
void INTHandler(int signo);
int interact();
void add_bg_job(bg_job *job);
void print_bg_jobs();
bg_job* remove_bg_job(int pid);
char* get_cmd(char **pgmlist);
void jobs(char **pgmlist);
void print_bg_job(int job_id);
void killAllBgp();

void pipe_pgm(Pgm *pgm);
void exec_pgm(Pgm *pgm);

// head and tail of linked list of background processes.
bg_job *bg_job_head;
bg_job *bg_job_tail;


int main(void)
{
	// Make the main process ignore SIGINT signal.
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sa, NULL);

	// Assign for head and tail 
	bg_job job;
	job.job_id = 0;
	bg_job_head = &job;
	bg_job_tail = bg_job_head;

	while (TRUE)
	{
		if(!interact())
			break;
	}
	return 0;
}

// One interact with user.
int interact(){
	Command cmd;
  	int parse_result;

	char cwd[CWD_MAX_LENGTH];
	char hostname[HOST_MAX_LENGTH];
	getcwd(cwd, CWD_MAX_LENGTH);
	gethostname(hostname, HOST_MAX_LENGTH);
	printf("%s@%s:%s", getlogin(), hostname, cwd);

    char *line;
    line = readline("> ");

    /* If EOF encountered, exit shell */
    if (!line)
    {
      return FALSE;
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
	return TRUE;
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

	// If the instruction is cd, run cd function.
	if(strcmp(*cmd->pgm->pgmlist, "cd") == 0){ 
		cd(cmd->pgm->pgmlist);
		return;
	}

	// If the instruction is jobs, run jobs function.
	if(strcmp(*cmd->pgm->pgmlist, "jobs") == 0){
		jobs(cmd->pgm->pgmlist);
		return;
	}

	// Quit the shell if the instruction is exit; Kill the whole process group.
	if(strcmp(*cmd->pgm->pgmlist, "exit") == 0){ 
		killAllBgp();
		exit(0);
	}

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
		// Set SIGINT handler for the child process which doesn't run in the background.
		if(!cmd->background){
			struct sigaction sa;
			sa.sa_handler = INTHandler;
			sigaction(SIGINT, &sa, NULL);
		}

		pipe_pgm(cmd->pgm);
	} else { // Parent process
		if(!cmd->background){ // Not background, wait for this specific child.
			int status = 0;
			waitpid(pid, &status, 0);
		}
		else{ // Background, wait for background children one time in WNOHANG mode, then do next interact.
			// add a new bg process to list.
			struct bg_job job;
			job.job_id = bg_job_tail->job_id + 1;
			job.pid = pid;
			job.cmd = get_cmd(cmd->pgm->pgmlist);
			job.next = NULL;
			add_bg_job(&job);
			printf("[%d] %d\n", job.job_id, job.pid);

			while(TRUE){
				int status = 0;
				pid_t PID = waitpid(-1, &status, WNOHANG);

				if(PID == 0) // No bg child states change, do next interact.
					interact();

				else if(PID > 0){ // A child state changes, remove it from list.
					bg_job *del_job = remove_bg_job(PID);
					printf("[%d] +done %s\n", del_job->job_id, del_job->cmd);
					break;
				}
				else{
					perror("Waitpid error: ");
					break;
				}
			}
		}
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



/* 
 * Kill all background processes.
 */
void killAllBgp(){
	bg_job *job = bg_job_head->next;
	int ret;
	while(job != NULL){
		kill(job->pid, SIGKILL);
		job = job->next;
	}
}


/* 
 * Change working directory according to given path
 */
void cd(char** pgmlist){
	// Get current working dir in order to set last cwd if change cwd successfully.
	char cwDir[CWD_MAX_LENGTH];
	char* cwd = getcwd(cwDir, CWD_MAX_LENGTH);
	if(cwd == NULL)
		printf("lsh: cd: getcwd: %s\n", strerror(errno));
	
	//Get home dir
	char* homeDir = getenv("HOME");

	// If there are no arguments after cd, change to home dir if HOME env can be found.
	if(*(pgmlist + 1) == NULL){ 
		if(homeDir == NULL){
			printf("lsh: cd: no HOME environment variable found\n");
			return;
		}
		int ret = chdir(homeDir);
		if(ret != 0){
			printf("lsh: cd: chdir: %s\n", strerror(errno));
			return;
		}
	}
	// Else if the first argument is - or ~ .
	else if(strcmp(*(pgmlist + 1), "-") == 0 || strcmp(*(pgmlist + 1), "~") == 0){
		// If there is a second argument, print too many arguments and return.
		if(*(pgmlist + 2) != NULL){
			printf("lsh: cd: too many arguments\n");
			return;
		}

		// If it is -, change to last cwd.
		if(strcmp(*(pgmlist + 1), "-") == 0){
			if(strcmp(lastCwd, "") == 0){
				printf("lsh: cd: last cwd not set\n");
				return;
			}
			int ret = chdir(lastCwd);
			if(ret != 0){
				printf("lsh: cd: chdir: %s\n", strerror(errno));
				return;
			}
		}
		// If it is ~, change to home dir.
		else{
			if(homeDir == NULL){
				printf("lsh: cd: no HOME environment variable found\n");
				return;
			}
			int ret = chdir(homeDir);
			if(ret != 0){
				printf("lsh: cd: chdir: %s\n", strerror(errno));
				return;
			}
		}
	}
	// Else concat all arguments as a path and change 
	else{
		char dir[CWD_MAX_LENGTH];
		strcpy(dir, *(pgmlist + 1));
		for(int i = 2; *(pgmlist + i) != NULL; i ++){
			strcat(dir, " ");
			strcat(dir, *(pgmlist + i));
		}
		int ret = chdir(dir);
		if(ret != 0){
			printf("Error in cd chdir: %s\n", strerror(errno));
			return;
		}
	}

	strcpy(lastCwd, cwd); // If changing working directory successfully, set last cwd.
}



/* 
 * View bg processes
 */
void jobs(char **pgmlist){
	if(*(pgmlist + 1) == NULL){ // No argument, view all bg processes.
		print_bg_jobs();
		return;
	} 
	// Else, view specific bg process
	char *job_num_str = *(pgmlist + 1);
	int job_num = atoi(job_num_str);
	if(job_num == 0)
		printf("lsh: jobs: %s: no such job\n", job_num_str);
	else
		print_bg_job(job_num);
}

/* 
 * SIGINT signal handler: After receiving SIGINT, exit the process.
 */
void INTHandler(int signo){
    if(signo == SIGINT){
        exit(0); // !!!Only kill itself!!!
    }
}



/* 
 * Add a job to list
 */
void add_bg_job(bg_job *job){
	bg_job_tail->next = job;
	bg_job_tail = job;
}



/* 
 * Remove a job from list, return the removed job
 */
bg_job* remove_bg_job(int pid){
	struct bg_job *job = bg_job_head;
	struct bg_job *del_job;
	while (job->next != NULL)
	{
		if(job->next->pid == pid){
			del_job = job->next;
			job->next = job->next->next;
			if(del_job== bg_job_tail)
				bg_job_tail = job;
			del_job->next = NULL;
			break;
		}
		job = job->next;
	}	
	return del_job;
}



/* 
 * Print all jobs in the list.
 */
void print_bg_jobs(){
	bg_job *job = bg_job_head->next;
	while(job != NULL){
		printf("[%d] +running %s\n", job->job_id, job->cmd);
		job = job->next;
	}
}



/* 
 * Print a specific job in the list
 */
void print_bg_job(int job_id){
	bg_job *job = bg_job_head->next;
	while(job != NULL){
		if(job->job_id == job_id){
			printf("[%d] +running %s\n", job->job_id, job->cmd);
			return;
		}
		job = job->next;
	}
	printf("lsh: jobs: %d: no such job\n", job_id);
}



/* 
 * Convert pgmlist to a string.
 */
char* get_cmd(char **pgmlist){
	char *cmd = malloc(sizeof(char) * 128);
	int len = 0;
	strcpy(cmd, *(pgmlist));
	len += strlen(*(pgmlist));
	for(int i = 1; *(pgmlist + i) != NULL; i ++){
		strcat(cmd, " ");
		strcat(cmd, *(pgmlist + i));
		len += strlen(*(pgmlist + i)) + 1;
	}
	return cmd;
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

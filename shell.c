/* SMP1: Simple Shell */

/* LIBRARY SECTION */
#include <ctype.h>              /* Character types                       */
#include <stdio.h>              /* Standard buffered input/output        */
#include <stdlib.h>             /* Standard library functions            */
#include <string.h>             /* String operations                     */
#include <sys/types.h>          /* Data types                            */
#include <sys/wait.h>           /* Declarations for waiting              */
#include <unistd.h>             /* Standard symbolic constants and types */

#include "smp1_tests.h"         /* Built-in test system                  */

/* DEFINE SECTION */
#define SHELL_BUFFER_SIZE 256   /* Size of the Shell input buffer        */
#define SHELL_MAX_ARGS 8        /* Maximum number of arguments parsed    */
#define SHELL_HISTORY_SIZE 9

/* VARIABLE SECTION */
enum { STATE_SPACE, STATE_NON_SPACE };	/* Parser states */

int imthechild(const char *path_to_exec, char *const args[]) {
    return execvp(path_to_exec, args) ? -1 : 0;
}

void imtheparent(pid_t child_pid, int run_in_background) {
	int child_return_val, child_error_code;

	/* fork returned a positive pid so we are the parent */
	fprintf(stderr,
	        "  Parent says 'child process has been forked with pid=%d'\n",
	        child_pid);
	if (run_in_background) {
		fprintf(stderr,
		        "  Parent says 'run_in_background=1 ... so we're not waiting for the child'\n");
		return;
	}
	
	waitpid(child_pid, &child_return_val, 0); // waitpid
	// wait(&child_return_val); -> original
	
	/* Use the WEXITSTATUS to extract the status code from the return value */
	child_error_code = WEXITSTATUS(child_return_val);
	fprintf(stderr,
	        "  Parent says 'wait() returned so the child with pid=%d is finished.'\n",
	        child_pid);
	if (child_error_code != 0) {
		/* Error: Child process failed. Most likely a failed exec */
		fprintf(stderr,
		        "  Parent says 'Child process %d failed with code %d'\n",
		        child_pid, child_error_code);
	}
}

/* MAIN PROCEDURE SECTION */
int main(int argc, char **argv) {
    pid_t shell_pid, pid_from_fork;
    int n_read, i, exec_argc, parser_state, run_in_background;
    char buffer[SHELL_BUFFER_SIZE];
    char *exec_argv[SHELL_MAX_ARGS + 1];

    int command_counter = 1;
    char** shell_history[SHELL_HISTORY_SIZE][SHELL_BUFFER_SIZE];
    int shell_history_next_index = 0;
    
    /* Entrypoint for the testrunner program */
	if (argc > 1 && !strcmp(argv[1], "-test")) {
		return run_smp1_tests(argc - 1, argv + 1);
	}
	
	shell_pid = getpid();

    while (1) {
        fprintf(stdout, "Shell(pid=%d)%i> ", shell_pid, command_counter);
        fflush(stdout);

        if (fgets(buffer, SHELL_BUFFER_SIZE, stdin) == NULL)
            return EXIT_SUCCESS;
        n_read = strlen(buffer);
        run_in_background = n_read > 2 && buffer[n_read - 2] == '&';
        buffer[n_read - run_in_background - 1] = '\n';

        parser_state = STATE_SPACE;
        for (exec_argc = 0, i = 0;
             (buffer[i] != '\n') && (exec_argc < SHELL_MAX_ARGS); i++) {
            if (!isspace(buffer[i])) {
                if (parser_state == STATE_SPACE)
                    exec_argv[exec_argc++] = &buffer[i];
                parser_state = STATE_NON_SPACE;
            } 
            else {
                buffer[i] = '\0';
                parser_state = STATE_SPACE;
            }
        }

        buffer[i] = '\0';

        if (!exec_argc)
            continue;

        exec_argv[exec_argc] = NULL;

        if (!strcmp(exec_argv[0], "exit")) {
            printf("Exiting process %d\n", shell_pid);
            return EXIT_SUCCESS;
        } 
        else if (!strcmp(exec_argv[0], "cd") && exec_argc > 1) {
            if (chdir(exec_argv[1]))
                fprintf(stderr, "cd: failed to chdir %s\n", exec_argv[1]); 
        } 
		else if (!strcmp(exec_argv[0], "sub")) { // Add 'sub' command handling
            pid_from_fork = fork();

            if (pid_from_fork < 0) {
                fprintf(stderr, "fork failed\n");
                continue;
            }
            if (pid_from_fork == 0) {
                return imthechild("./shell", NULL);
            } 
            else {
                imtheparent(pid_from_fork, run_in_background);
            }
        }         
        else {
            pid_from_fork = fork();

            if (pid_from_fork < 0) {
                fprintf(stderr, "fork failed\n");
                continue;
            }
            if (pid_from_fork == 0) {
                return imthechild(exec_argv[0], &exec_argv[0]);
            } 
            else {
                imtheparent(pid_from_fork, run_in_background);
            }
        }
        if (exec_argc > 0) {
			command_counter++;
        }
    }
    return EXIT_SUCCESS;
}

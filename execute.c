/*---------------------------------------------------------------------------*/
/* execute.c                                                                 */
/* Author: Jongki Park, Kyoungsoo Park                                       */
/*---------------------------------------------------------------------------*/

#include "dynarray.h"
#include "token.h"
#include "util.h"
#include "lexsyn.h"
#include "snush.h"
#include "execute.h"

/*---------------------------------------------------------------------------*/
/* Redirects standard output to the specified file.                          */
/*                                                                           
 * Parameters: 
 *   fname - the name of the file to redirect output to.
 * 
 * Returns: 
 *   Nothing. Exits the process if an error occurs.
 * 
 * Global Variables Affected: 
 *   None.
 */
void redout_handler(char *fname) {
	//
	// TODO-start: redout_handler() in execute.c
	// 
    int fd;

	fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd < 0) {
		error_print(NULL, PERROR);
		exit(EXIT_FAILURE);
	}

	if (dup2(fd, STDOUT_FILENO) < 0) {
		perror("Error redirecting stdout");
		close(fd);
		exit(EXIT_FAILURE);
	}

	close(fd);
	//
	// TODO-end redout_handler() in execute.c
	// 
}
/*---------------------------------------------------------------------------*/
void redin_handler(char *fname) {
	int fd;

	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		error_print(NULL, PERROR);
		exit(EXIT_FAILURE);
	}
	else {
		dup2(fd, STDIN_FILENO);
		close(fd);
	}
}
/*---------------------------------------------------------------------------*/
int build_command_partial(DynArray_T oTokens, int start, 
						int end, char *args[]) {
	int i, ret = 0, redin = FALSE, redout = FALSE, cnt = 0;
	struct Token *t;

	/* Build command */
	for (i = start; i < end; i++) {

		t = dynarray_get(oTokens, i);

		if (t->token_type == TOKEN_WORD) {
			if (redin == TRUE) {
				redin_handler(t->token_value);
				redin = FALSE;
			}
			else if (redout == TRUE) {
				redout_handler(t->token_value);
				redout = FALSE;
			}
			else
				args[cnt++] = t->token_value;
		}
		else if (t->token_type == TOKEN_REDIN)
			redin = TRUE;
		else if (t->token_type == TOKEN_REDOUT)
			redout = TRUE;
	}
	args[cnt] = NULL;

#ifdef DEBUG
	for (i = 0; i < cnt; i++)
	{
		if (args[i] == NULL)
			printf("CMD: NULL\n");
		else
			printf("CMD: %s\n", args[i]);
	}
	printf("END\n");
#endif
	return ret;
}
/*---------------------------------------------------------------------------*/
int build_command(DynArray_T oTokens, char *args[]) {
	return build_command_partial(oTokens, 0, 
								dynarray_get_length(oTokens), args);
}
/*---------------------------------------------------------------------------*/
void execute_builtin(DynArray_T oTokens, enum BuiltinType btype) {
	int ret;
	char *dir = NULL;
	struct Token *t1;

	switch (btype) {
	case B_EXIT:
		if (dynarray_get_length(oTokens) == 1) {
			// printf("\n");
			dynarray_map(oTokens, free_token, NULL);
			dynarray_free(oTokens);

			exit(EXIT_SUCCESS);
		}
		else
			error_print("exit does not take any parameters", FPRINTF);

		break;

	case B_CD:
		if (dynarray_get_length(oTokens) == 1) {
			dir = getenv("HOME");
			if (dir == NULL) {
				error_print("cd: HOME variable not set", FPRINTF);
				break;
			}
		}
		else if (dynarray_get_length(oTokens) == 2) {
			t1 = dynarray_get(oTokens, 1);
			if (t1->token_type == TOKEN_WORD)
				dir = t1->token_value;
		}

		if (dir == NULL) {
			error_print("cd takes one parameter", FPRINTF);
			break;
		}
		else {
			ret = chdir(dir);
			if (ret < 0)
				error_print(NULL, PERROR);
		}
		break;

	default:
		error_print("Bug found in execute_builtin", FPRINTF);
		exit(EXIT_FAILURE);
	}
}
/*---------------------------------------------------------------------------*/
/* Executes a single command in the shell.                                   */
/*                                                                           
 * Parameters: 
 *   oTokens - the array of tokens representing the command.
 *   is_background - a flag indicating whether the command is to be run in 
 *                   the background.
 * 
 * Returns: 
 *   A positive value on success, or a negative value on failure.
 * 
 * Global Variables Affected: 
 *   bg_manager - updated to include background processes.
 */
int fork_exec(DynArray_T oTokens, int is_background) {
    //
    // TODO-START: fork_exec() in execute.c 
    //
    int ret = 1;
	pid_t pid;
	char *args[MAX_ARGS_CNT];

	int stdin_fd = dup(STDIN_FILENO);
	int stdout_fd = dup(STDOUT_FILENO);
	if (stdin_fd == -1 || stdout_fd == -1) { //dup error
		perror("dup");
		return -1;
	}

	if (build_command(oTokens, args) != 0) { // build command error
		error_print("Failed to build command", FPRINTF);
		dup2(stdin_fd, STDIN_FILENO);
		dup2(stdout_fd, STDOUT_FILENO);
		close(stdin_fd);
		close(stdout_fd);
		return -1;
	}

	pid = fork();
	if (pid < 0) { // fork error
		perror("fork");
		dup2(stdin_fd, STDIN_FILENO);
		dup2(stdout_fd, STDOUT_FILENO);
		close(stdin_fd);
		close(stdout_fd);
		return -1;
	}

	if (pid == 0) { // child process
		signal(SIGINT, SIG_DFL);

		close(stdin_fd);
		close(stdout_fd);

		if (execvp(args[0], args) < 0) { // execvp instruction
			perror(args[0]);
			exit(EXIT_FAILURE);
		}
	} else { // parent process

		dup2(stdin_fd, STDIN_FILENO);
		dup2(stdout_fd, STDOUT_FILENO);
		close(stdin_fd);
		close(stdout_fd);

		if (is_background) { // background process 
			bg_manager.bg_array[bg_manager.bg_count++] = pid;
			//printf("[%d] Background process started.\n", pid);
			ret = pid;
		} else {
			int status;
			if (waitpid(pid, &status, 0) < 0) {
    			if (errno == EINTR) { // SIGINT로 중단된 경우
            		ret = 1; // 정상 종료로 간주
        		} else {
            		perror("waitpid failed");
            		ret = -1;
        		}
			} else if (WIFSIGNALED(status)) { 
    			if (WTERMSIG(status) == SIGINT) {
        			ret = 1; // SIGINT return 1
    			} else {
        			printf("Process terminated by signal %d.\n", WTERMSIG(status));
        			ret = -1; // 다른 신호로 종료된 경우
    			}
			} else if (WIFEXITED(status)) { // 정상 종료된 경우
    			ret = WEXITSTATUS(status) == 0 ? 1 : -1;
			}
		}
	}
	if (ret < 0) {
    	ret = 1; // 오류 발생 시에도 기본적으로 1로 설정
	}
	return ret;
    //
    // TODO-END: fork_exec() in execute.c
    //
}
/**
 * Executes a pipeline of commands, connecting them with pipes (`|`).
 * 
 * Parameters:
 *   @pcount: Number of pipes in the pipeline.
 *   @oTokens: Tokenized input commands.
 *   @is_background: 1 for background execution, 0 for foreground.
 * 
 * Return:
 *   - PID of the last command if background, 1 if successful foreground execution.
 *   - -1 on failure.
 * 
 * Details:
 *   - Sets up pipes to redirect command outputs as inputs to the next command.
 *   - Handles standard input/output restoration.
 *   - Waits for child processes unless running in the background.
 */

int iter_pipe_fork_exec(int pcount, DynArray_T oTokens, int is_background) {
	//
	// TODO-START: iter_pipe_fork_exec() in execute.c 
	// 
	int ret = 1;
	int pipefd[2 * pcount];
	pid_t last_pid = -1;
	char *args[MAX_ARGS_CNT];
	int cmd_start = 0;

	// STDIN_FILENO, STDOUT_FILENO backup
	int stdin_fd = dup(STDIN_FILENO);
	int stdout_fd = dup(STDOUT_FILENO);
	if (stdin_fd == -1 || stdout_fd == -1) {
		perror("dup");
		return -1;
	}

	for (int i = 0; i < pcount; i++) { // pipe
		if (pipe(pipefd + i * 2) < 0) {
			perror("pipe");
			return -1;
		}
	}

	for (int i = 0; i <= pcount; i++) {
		int cmd_end = dynarray_get_length(oTokens);
		for (int j = cmd_start; j < dynarray_get_length(oTokens); j++) {
			struct Token *t = dynarray_get(oTokens, j);
			if (t->token_type == TOKEN_PIPE) {
				cmd_end = j;
				break;
			}
		}

		// load standard in/out put
		dup2(stdin_fd, STDIN_FILENO);
		dup2(stdout_fd, STDOUT_FILENO);

		if (build_command_partial(oTokens, cmd_start, cmd_end, args) != 0) {
			error_print("Failed to build command", FPRINTF);
			close(stdin_fd);
			close(stdout_fd);
			return -1;
		}

		pid_t pid = fork();
		if (pid < 0) { // fork error
			perror("fork");
			close(stdin_fd);
			close(stdout_fd);
			return -1;
		}

		if (pid == 0) { // child process
			signal(SIGINT, SIG_DFL);
			close(stdin_fd);
			close(stdout_fd);
			if (i > 0) {
				dup2(pipefd[(i - 1) * 2], STDIN_FILENO);
			}
			if (i < pcount) {
				dup2(pipefd[i * 2 + 1], STDOUT_FILENO);
			}
			for (int j = 0; j < 2 * pcount; j++) {
				close(pipefd[j]);
			}
			if (execvp(args[0], args) < 0) {
				perror(args[0]);
				exit(EXIT_FAILURE);
			}
		}
		// parent process
		last_pid = pid;

		if (i > 0) {
			close(pipefd[(i - 1) * 2]);
		}
		if (i < pcount) {
			close(pipefd[i * 2 + 1]);
		}
		cmd_start = cmd_end + 1;
		if (cmd_start < dynarray_get_length(oTokens)) {
			struct Token *next_token = dynarray_get(oTokens, cmd_start);
			if (next_token->token_type == TOKEN_PIPE) {
				cmd_start++;
			}
		}
	}

	// all pipe close
	for (int i = 0; i < 2 * pcount; i++) {
		close(pipefd[i]);
	}

	dup2(stdin_fd, STDIN_FILENO);
	dup2(stdout_fd, STDOUT_FILENO);
	close(stdin_fd);
	close(stdout_fd);
	if (!is_background) { // Foreground execution
    	int status;
    	pid_t pid;
    	while ((pid = wait(&status)) > 0) {
        	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT) {
            	ret = 1; // SIGINT 발생 시 정상 종료로 처리
            	break;
        	} else if (WIFEXITED(status)) {
            	int exit_code = WEXITSTATUS(status);
            	if (exit_code != 0) {
                	ret = -1; // 비정상 종료
            	}
        	} else if (WIFSIGNALED(status)) {
            	printf("Process terminated by signal %d.\n", WTERMSIG(status));
            	ret = -1; // 다른 신호로 종료
        	}
    	}

    	if (pid < 0 && errno != ECHILD) {
        	perror("wait failed"); // 대기 중 오류
        	ret = -1;
    	}
	} else { // Background execution
    	bg_manager.bg_array[bg_manager.bg_count++] = last_pid;
    	ret = last_pid; // Return the PID of the last process in the pipeline
	}
	return ret;
	//
	// TODO-END: iter_pipe_fork_exec() in execute.c
	//
}
/*---------------------------------------------------------------------------*/
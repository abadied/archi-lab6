#define _GNU_SOURCE
#include "LineParser.h"
#include <stdlib.h>
#include <linux/limits.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "JobControl.h"
#include <sys/wait.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2


int debug = 0;

void execute(cmdLine *pCmdLine, job* curr_job){
	int pid;
	int pid2;
	int status;
	cmdLine* curr_cmd = pCmdLine;
	int fd[2];
	int use_pipe = 0;
	if(curr_cmd->next != NULL){
		pipe(fd);
		use_pipe = 1;
	}
	switch (pid = fork()) {
		case -1:
		if (debug)
			perror("fork failed");
		break;
		case 0:
			signal(SIGQUIT, SIG_DFL);
			signal(SIGCHLD, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			signal(SIGTTIN, SIG_DFL);
			signal(SIGTTOU, SIG_DFL);
			
			setpgid(0, getpid());
			
			curr_job->pgid = getpgid(0);
			if(use_pipe){
				close(1);
				dup(fd[1]);
				close(fd[1]);
			}
			if(execvp(pCmdLine->arguments[0],pCmdLine->arguments)){
				perror("error in execution");
				_exit(EXIT_FAILURE);
			}
			_exit(EXIT_SUCCESS);
		break;
		default:
			if (debug)
				fprintf(stderr,"Executing Command: %s\nChild PID: %d\n",pCmdLine->arguments[0],pid);
			
			if(use_pipe){
				pCmdLine = pCmdLine->next;
				close(fd[1]);
				switch (pid2 = fork()) {
					case -1:
					if (debug)
						perror("fork failed");
					break;
					case 0:
						signal(SIGQUIT, SIG_DFL);
						signal(SIGCHLD, SIG_DFL);
						signal(SIGTSTP, SIG_DFL);
						signal(SIGTTIN, SIG_DFL);
						signal(SIGTTOU, SIG_DFL);
						
						setpgid(0, getpid());
						
						/*curr_job->pgid = getpgid(0);*/
						close(0);
						dup(fd[0]);
						close(fd[0]);

						if(execvp(pCmdLine->arguments[0],pCmdLine->arguments)){
							perror("error in execution");
							_exit(EXIT_FAILURE);
						}
						_exit(EXIT_SUCCESS);
					break;
					default:
						close(fd[0]);
						if (debug)
							fprintf(stderr,"Executing Command: %s\nChild PID: %d\n",pCmdLine->arguments[0],pid2);
						if (pCmdLine->blocking == 1){
							waitpid(pid, &status, 0);
							/*setpgid(0, getpid());*/
							status = 0;
							waitpid(pid2, &status, 0);
						}
				}
			}

			else if (pCmdLine->blocking == 1){
				waitpid(pid, &status, 0);
				setpgid(0, getpid());
			}
	}
}

void sigHandler(int signum) {
	if (debug) {
		char* strsig = strsignal(signum);
		printf("signal caught and ignored: %s\n", strsig);
	}
}

void changeDir(cmdLine* cmd){
	if(chdir(cmd->arguments[1])){
		perror("error ocuured in chdir \n");
	}
}

int main(int argc,char** argv){
	/*check if debug mode*/
	if(argc > 1){
		if(strcmp(argv[1],"-d") == 0)
			debug = 1;
	}

	signal(SIGQUIT, sigHandler);
	signal(SIGCHLD, sigHandler);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	
	char buffer[2048];
	char path[PATH_MAX];

	getcwd(path, PATH_MAX);
    printf("%s$ ", path);

	fgets(buffer,2048,stdin);
	job* jobs_list = NULL;
	
	struct termios* termios_p = (struct termios*)malloc(sizeof(struct termios));
	tcgetattr(STDIN_FILENO, termios_p);
	
	cmdLine* cmd = parseCmdLines(buffer);
	while(strcmp(cmd->arguments[0],"quit") != 0){
		if(strcmp(cmd->arguments[0],"cd") == 0){
			changeDir(cmd);
		}
		else if(strcmp(cmd->arguments[0],"jobs") == 0 && jobs_list != NULL){
			printJobs(&jobs_list);
		}
		else if(strcmp(cmd->arguments[0], "fg") == 0) {
			job* curr_job = findJobByIndex(jobs_list, atoi(cmd->arguments[1]));
			runJobInForeground(&jobs_list, curr_job,1 , termios_p, getpgid(0));
		}
		else {
			job* curr_job = addJob(&jobs_list, buffer);
			execute(cmd, curr_job);
			
		}
		
		freeCmdLines(cmd);
		
		getcwd(path, PATH_MAX);
    	printf("%s$ ", path);
    
		fgets(buffer,2048,stdin);
		cmd = parseCmdLines(buffer);
	}
	freeCmdLines(cmd);
	if(jobs_list!=NULL)
		freeJobList(&jobs_list);
	return 0;
}


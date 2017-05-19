#define _GNU_SOURCE
#include "LineParser.h"
#include <stdlib.h>
#include <linux/limits.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

int debug = 0;

void execute(cmdLine *pCmdLine){
	int pid;
	int* status = 0;
        
	switch (pid = fork()) {
		case -1:
		if (debug)
			perror("fork failed");
		_exit(EXIT_FAILURE);
		break;
		case 0:
                        if(pCmdLine->inputRedirect != NULL){
                            
                            /* Close stdin, duplicate the input side of pipe to stdin  */
                            int new_fd = open(pCmdLine->inputRedirect,O_RDONLY,0);
                            dup2(new_fd, 0);
                            
                        }
                        if(pCmdLine->outputRedirect != NULL){
                            
                            /* Close stdout, duplicate the input side of pipe to stdout  */
                            
                            int new_fd = open(pCmdLine->outputRedirect,O_WRONLY | O_TRUNC,0);
                            
                            dup2(new_fd, 1);
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
			if (pCmdLine->blocking == 1)
				waitpid(pid, status, 0);
	}
}

void sigHandler(int signum) {
  char* strsig = strsignal(signum);
  printf("signal caught and ignored: %s\n", strsig);
}

void changeDir(cmdLine* cmd){
	if(chdir(cmd->arguments[1])){
		perror("error ocuured in chdir");
	}
}
int main(int argc,char** argv){
	/*check if debug mode*/
	if(argc > 1){
		if(strcmp(argv[1],"-d") == 0)
			debug = 1;
	}

	signal(SIGQUIT,sigHandler);
	signal(SIGTSTP,sigHandler);
	signal(SIGCHLD,sigHandler);

	char buffer[2048];
	char path[PATH_MAX];

	getcwd(path, PATH_MAX);
        printf("%s$ ", path);

	fgets(buffer,2048,stdin);
	cmdLine* cmd = parseCmdLines(buffer);
        
	while(strcmp(cmd->arguments[0],"quit") != 0){
                
		if(strcmp(cmd->arguments[0],"cd") == 0){
			changeDir(cmd);
			freeCmdLines(cmd);
		}
		else{
			execute(cmd);
			freeCmdLines(cmd);
		}
		getcwd(path, PATH_MAX);
    	printf("%s$ ", path);
		fgets(buffer,2048,stdin);
		cmd = parseCmdLines(buffer);
	}
	return 0;
}


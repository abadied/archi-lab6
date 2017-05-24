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



typedef struct linkedMap{
	char* key;
	char* val;
	struct linkedMap* next;
} linkedMap;

char* find(linkedMap** dic, char* key){
	linkedMap* curr_map = *dic;
	while(curr_map) {
		if (strcmp(curr_map->key, key) == 0){
			return curr_map->val;
		}
		curr_map = curr_map->next;
	}
	return NULL;
}

int removeMap(linkedMap** dic, char* key){
	linkedMap* curr_map = *dic;
	if(curr_map == NULL){
		return 1;
	}

	if (strcmp(curr_map->key,key) == 0){
		*dic = curr_map->next;
		free(curr_map);
		return 0;
	}

	while(curr_map->next != NULL){
		linkedMap* temp = curr_map->next;
		if (strcmp(temp->key,key) == 0){
			curr_map->next = temp->next;
			free(temp->key);
			free(temp->val);
			free(temp);
			return 0;
		}
	}
	return 1;
}

void addMap(linkedMap** dic, char* key, char* val){
	if (find(dic, key)){
		removeMap(dic, key);
	}
	linkedMap* newMap = (linkedMap*)malloc(sizeof(linkedMap));
	newMap->key = (char*)malloc(strlen(key));
	newMap->val = (char*)malloc(strlen(val));
	strcpy(newMap->key, key);
	strcpy(newMap->val, val);
	newMap->next = *dic;
	*dic = newMap;
}

void printDic(linkedMap** dic){
	linkedMap* curr_map = *dic;
	while(curr_map){
		printf("name:%s val:%s\n", curr_map->key, curr_map->val);
		curr_map = curr_map->next;
	}
}

int replaceVars(linkedMap** dic, cmdLine* pCmdLine){
	int i;
	for (i = 0; i < pCmdLine->argCount; i++) {
		if(pCmdLine->arguments[i][0] == '$'){
			char* val = find(dic, pCmdLine->arguments[i] + 1);
			if (val == NULL)
				return 1;
			replaceCmdArg(pCmdLine, i, val);
		}
		else if (pCmdLine->arguments[i][0] == '~'){
			replaceCmdArg(pCmdLine, i, getenv("HOME"));
		}
	}
	return 0;
}

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
	
	linkedMap* dic = NULL;

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
		if (replaceVars(&dic, cmd)) {
			fprintf(stderr,"ERROR: varriable not found\n");
		}
		else if(strcmp(cmd->arguments[0],"cd") == 0){
			changeDir(cmd);
		}
		else if(strcmp(cmd->arguments[0],"jobs") == 0 && jobs_list != NULL){
			printJobs(&jobs_list);
		}
		else if(strcmp(cmd->arguments[0], "fg") == 0) {
			job* curr_job = findJobByIndex(jobs_list, atoi(cmd->arguments[1]));
			runJobInForeground(&jobs_list, curr_job,1 , termios_p, getpgid(0));
		}
		else if(strcmp(cmd->arguments[0], "set") == 0) {
			addMap(&dic, cmd->arguments[1], cmd->arguments[2]);
		}
		else if(strcmp(cmd->arguments[0], "delete") == 0) {
			if (removeMap(&dic, cmd->arguments[1]))
				fprintf(stderr,"ERROR: Variable not found\n");
		}
		else if(strcmp(cmd->arguments[0], "env") == 0) {
			printDic(&dic);
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


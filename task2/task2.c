#define _GNU_SOURCE

#include <stdlib.h>
#include <linux/limits.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>


int main(int argc, char** argv)
{
    int     fd[2];
    pid_t   childpid1;
    pid_t   childpid2;
    int status = 0;
    pipe(fd);
    int debug_mode = 0;
    if(argc > 1){
        if(strcmp(argv[1],"-d") == 0)
            debug_mode = 1;
    }
    char* const args1[3] = {"ls", "-l", 0}; 

    char* const args2[4] = {"tail","-n","2", 0}; 

    if(debug_mode)
        fprintf(stderr,"parent process>forking\n");

    if((childpid1 = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }

    else if(childpid1 == 0)
    {
        if(debug_mode)
            fprintf(stderr,"child1>redirecting stdout to the write end of the pipe \n");
        close(1);
        dup(fd[1]);
        close(fd[1]);

        if(debug_mode)
            fprintf(stderr,"child1>going to execute cmd \n");
        execvp(args1[0],args1);
        exit(0);
    }
    else
    {
        if(debug_mode)
            fprintf(stderr, "parent process created process with id: %d \n",getpid());
        /* Parent process closes up output side of pipe */
        if(debug_mode)
            fprintf(stderr,"parent_process>closing the write end of the pipe \n");
        close(fd[1]);

        if(debug_mode)
            fprintf(stderr,"parent_process>forking\n");

        if((childpid2 = fork()) == -1)
	    {
            perror("fork");
            exit(1);
	    }

        if(debug_mode)
            fprintf(stderr,"parent_process>created process with id: %d\n",getpid());

	    if(childpid2 == 0)
	    {
            if(debug_mode)
                fprintf(stderr,"child2>redirecting stdin to the read end of the pipe \n");
            close(0);
            dup(fd[0]);
            close(fd[0]);
	    	/*char readbuffer[80];
			read(fd[0], readbuffer, 80);
            printf("Received string: %s", readbuffer);
			*/
            if(debug_mode)
                fprintf(stderr,"child2>going to execute cmd\n");
            execvp(args2[0],args2);
            exit(0);
	    }
	    else
	    {
            if(debug_mode)
                fprintf(stderr,"parent_process>closing the read end of the pipe \n");
            close(fd[0]);

            if(debug_mode)
                fprintf(stderr,"parent_process>waiting for child process to terminate \n");
            waitpid(childpid1,&status,0);
		    status = 0;
		    waitpid(childpid2,&status,0);
            if(debug_mode)
                fprintf(stderr,"parent_process>exiting\n");
            exit(0);
	    }

    }
    
    return(0);
}
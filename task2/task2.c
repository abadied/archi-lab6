#define _GNU_SOURCE

#include <stdlib.h>
#include <linux/limits.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>


int main(void)
{
    int     fd[2];
    pid_t   childpid1;
    pid_t   childpid2;
    int status = 0;
    pipe(fd);
    char* const args1[3] = {"/bin/ls", "-l", 0}; 

    char* const args2[4] = {"tail","-n","2", 0}; 

    if((childpid1 = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }

    else if(childpid1 == 0)
    {
        close(1);
        dup(fd[1]);
        close(fd[1]);

        execvp(args1[0],args1);
        exit(0);
    }
    else
    {
        /* Parent process closes up output side of pipe */
        close(fd[1]);

        if((childpid2 = fork()) == -1)
	    {
            perror("fork");
            exit(1);
	    }

	    if(childpid2 == 0)
	    {
            close(0);
            dup(fd[0]);
            close(fd[0]);
	    	/*char readbuffer[80];
			read(fd[0], readbuffer, 80);
            printf("Received string: %s", readbuffer);
			*/
            execvp(args2[0],args2);
            exit(0);
	    }
	    else
	    {
            close(fd[0]);

            waitpid(childpid1,&status,0);
		    status = 0;
		    waitpid(childpid2,&status,0);
	    }

    }
    
    return(0);
}
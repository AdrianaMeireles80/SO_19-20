#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX 1024

int main(int argc, char *argv[]){
    int f, r, fd_fifoW, fd_fifoR, status;
    char buf[MAX];
    char answer[MAX];

    //Abertura dos fifos
    fd_fifoW = open("fifo1", O_WRONLY);
    fd_fifoR = open("fifo2", O_RDONLY);

    if(fd_fifoR == -1 || fd_fifoW == -1){
        perror("open fifo");
        exit(-1);
    }

    if(argc == 1){ //Sem argumentos
        f = fork();
        switch (f)
        {
        case -1:
            perror("fork");
            break;
        case 0:
            while((r = read(0, &buf, sizeof(buf))) > 0){
                write(fd_fifoW, &buf, r);

                if((r = read(fd_fifoR,&answer,sizeof(answer))) > 0)
                    write(1, &answer, r);
            }
            _exit(0);
        default:
            wait(&status);
            break;
        }   
    }
    else {
        //Se for pela linha de comandos
        //So pode ser 1 comando de cada vez, ou varios?
    }

    close(fd_fifoR);
    close(fd_fifoW);
    return 0;

}
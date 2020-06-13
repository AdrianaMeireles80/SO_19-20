#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_COMANDOS 10
#define MAX 1024

int readLine(int fd, char* buf, int tam){
	int j = 0;
	while(j < tam && read(fd,buf+j,1) > 0 && buf[j] != '\n')
		j++;

	if(j >= tam)	
        buf[j] = '\0';
	else buf[j+1] = '\0';

	return j;
}

int redirect(char * op, char * file){
    int fd;
    if (strcmp(op,">") == 0){
        fd = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0666);
        dup2(fd, 1);
        close(fd);
    }
    else if (strcmp(op,">>") == 0){
        fd = open(file, O_CREAT | O_APPEND | O_WRONLY, 0666);
        dup2(fd, 1);
        close(fd);
    }
    else if (strcmp(op,"2>") == 0){
        fd = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0666);
        dup2(fd, 2);
        close(fd);
    }
    else if (strcmp(op,"2>>") == 0){
        fd = open(file, O_CREAT | O_APPEND | O_WRONLY, 0666);
        dup2(fd, 2);
        close(fd);
    }
    else {
        fd = open(file, O_RDONLY);
        dup2(fd, 0);
        close(fd);
    }

    return 0;
}

int exec_command(char* com){
    char *argumentos[20];
    char *token;
    int ret = 0;
    int i = 0;

    token = strtok(com, " ");

    while(token != NULL){
        if(strcmp(token,">") == 0 || strcmp(token,">>") == 0 || strcmp(token,"2>") == 0 ||
            strcmp(token,"2>>") == 0 || strcmp(token,"<") == 0){
                char* redOperator = token;
                token = strtok(NULL, " ");
                redirect(redOperator, token);
                token = strtok(NULL, " ");
            }
        else {
            argumentos[i] = token;
            token = strtok(NULL, " ");
            i++;
        }
    }

    argumentos[i] = NULL;

    ret = execvp(argumentos[0], argumentos);

    return ret;
}

int mysystem(char *coms, int nr_tarefa){
    int nr_comandos = 0;
    char *comandos[MAX_COMANDOS];
    char *token;
    char c, aux[50];
    int fildes[MAX_COMANDOS-1][2];
    int status[MAX_COMANDOS];

    int fd = open("log", O_CREAT | O_APPEND | O_RDWR, 0666);
    int fdidx = open("log.idx", O_CREAT | O_APPEND | O_WRONLY, 0666);
    int r, nr_linhas = 0, x = 0;

    for(int i = 0; (token = strsep(&coms, "|")) != NULL; i++){
        comandos[i] = strdup(token);
        nr_comandos++;
    }
    
    if(nr_comandos == 1){
        switch (fork()){
            case -1:
                perror("fork");
                return -1;
            case 0:
                write(fd, "\n", 1);
                int ind = lseek(fd, 0, SEEK_END);
            
               
                bzero(aux, sizeof(aux));
                x = sprintf(aux, "#%d: %d\n", nr_tarefa, ind);

                write(fdidx, aux, x);
                close(fdidx);

                dup2(fd, 1);
                close(fd);

                exec_command(comandos[0]);
                               
                _exit(0);
        }
    }
    else {
        for(int j = 0; j < nr_comandos; j++){
            if (j == 0){
                if(pipe(fildes[j]) != 0){
                        perror("pipe");
                        return -1;
                }
                switch(fork()){
                    case -1:
                        perror("fork");
                        return -1;
                    case 0:
                        close(fildes[j][0]);
                        dup2(fildes[j][1],1);
                        close(fildes[j][1]);

                        exec_command(comandos[j]);
                        _exit(0);
                    default:
                        close(fildes[j][1]);
                }
            }
            else if (j == nr_comandos - 1){
                switch(fork()){
                    case -1:
                        perror("fork");
                        return -1;
                    case 0:
                        write(fd, "\n", 1);
                        int ind2 = lseek(fd, 0, SEEK_END);
                                     
                        bzero(aux, sizeof(aux));
                        x = sprintf(aux, "#%d: %d\n", nr_tarefa, ind2);

                        write(fdidx, &aux, x);
                        close(fdidx);

                        dup2(fd, 1);
                        close(fd);

                        dup2(fildes[j-1][0],0);
                        close(fildes[j-1][0]);

                        exec_command(comandos[j]);
                        _exit(0);
                    default:
                        close(fildes[j-1][0]);
                }
            }
            else {
                if(pipe(fildes[j]) != 0){
                    perror("pipe");
                    return -1;
                }
                switch(fork()){
                    case -1:
                        perror("fork");
                        return -1;
                    case 0:
                        close(fildes[j][0]);
                        dup2(fildes[j][1],1);
                        close(fildes[j][1]);
                        dup2(fildes[j-1][0],0);
                        close(fildes[j-1][0]);  

                        exec_command(comandos[j]);
                        _exit(0); 
                    default:
                        close(fildes[j-1][0]);
                        close(fildes[j][1]);    
                }

            }
            
        }
    }
    for(int w = 0; w < nr_comandos; w++){
        wait(&status[w]);
        if(WIFEXITED(status[w])){
            char buf[50];
            int n = sprintf(buf,"[PAI] Filho saiu com status %d\n", WEXITSTATUS(status[w]));
            write(1, buf, n);
        }
    }
    for(int w = 0; w < nr_comandos; w++){
        free(comandos[w]);
    }
    return 0;
}

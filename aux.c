#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_COMANDOS 10

//Função para ler uma linha
int readLine(int fd, char* buf, int tam){
	int j = 0;
	while(j < tam && read(fd,buf+j,1) > 0 && buf[j] != '\n')
		j++;

	if(j >= tam)	
        buf[j] = '\0';
	else buf[j+1] = '\0';

	return j;
}

//Função que efetua os redirecionamentos > >> 2> 2>>
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

//Função que executa um comando
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

//Função que executa uma série de comandos separados por pipes anónimos
int mysystem(char *coms){
    int nr_comandos = 0;
    char *comandos[MAX_COMANDOS];
    char *linha, *token;
    int fildes[MAX_COMANDOS-1][2];
    int status[MAX_COMANDOS];

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
            printf("[PAI] Filho saiu com status %d\n", WEXITSTATUS(status[w]));
        }
    }
    for(int w = 0; w < nr_comandos; w++){
        free(comandos[w]);
    }
    return 0;
}

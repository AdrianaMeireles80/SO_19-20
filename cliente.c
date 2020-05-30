#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAX 1024

void helpGuide(){
    char coms[MAX];
    int n;
    n = sprintf(coms, "tempo-inactividade segs\ntempo-execucao segs\nexecutar 'p1 | p2 ... | pn'\nlistar\nterminar taskNum\nhistorico\n");
    write(1, &coms, n);
}


int parseCommand(char *com){
    int i = 0;
    char *token;
    char *tokens[20];

    while((token = strsep(&com, " ")) != NULL){
            tokens[i] = token;
            i++;
    }

    if(strcmp(tokens[0],"tempo-inatividade") == 0 || strcmp(tokens[0],"tempo-execucao") == 0 || 
        strcmp(tokens[0],"executar") == 0|| strcmp(tokens[0],"listar\n") == 0 || 
            strcmp(tokens[0],"terminar")  == 0|| strcmp(tokens[0],"historico\n") == 0)
        return 1;
    else return 0;
}

char** parseLinha(char* args[]){
    char **comandos = malloc(20 * sizeof(char*));
    int n = 0;

    for(int i = 0; args[i] != NULL; i++){
        char com[MAX];
        if(!strcmp(args[i], "-i") && args[i+1] != NULL){
            bzero(com, MAX);
            sprintf(com, "tempo-inactividade %d\n", atoi(args[i+1]));
            comandos[n] = strdup(com);
            n++;
            i++;
        }
        else if(!strcmp(args[i], "-m") && args[i+1] != NULL){
            bzero(com, MAX);
            sprintf(com, "tempo-execucao %d\n", atoi(args[i+1]));
            comandos[n] = strdup(com);
            n++;
            i++;
        }
        else if(!strcmp(args[i], "-e") && args[i+1] != NULL){
            bzero(com, MAX);
            sprintf(com, "executar \"%s\"\n", args[i+1]);
            comandos[n] = strdup(com);
            n++;
            i++;
        }
        else if(!strcmp(args[i], "-l")){
            bzero(com, MAX);
            sprintf(com, "listar\n");
            comandos[n] = strdup(com);
            n++;
        }
        else if(!strcmp(args[i], "-r")){
            bzero(com, MAX);
            sprintf(com, "historico\n");
            comandos[n] = strdup(com);
            n++;
        }
        else if(!strcmp(args[i], "-t") && args[i+1] != NULL){
            bzero(com, MAX);
            sprintf(com, "terminar %d\n", atoi(args[i+1]));
            comandos[n] = strdup(com);
            n++;
            i++;
        }
        else if(!strcmp(args[i], "-h")){
            helpGuide();
        }
        else {
            char error[MAX];
            int n = sprintf(error, "Comando Inválido: %s\n", args[i]);
            write(1, &error, n); 
        }
    }
    return comandos;
    
}


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

    f = fork();
    switch (f){
        case -1:
            perror("fork");
            break;
        case 0:
            if(argc == 1) { //Sem argumentos
                while((r = read(0, &buf, sizeof(buf))) > 0){
                    char *com = strdup(buf);
                    if(strcmp(com, "ajuda\n") == 0)
                        helpGuide();
                    else if(parseCommand(com)){
                        write(fd_fifoW, &buf, r);
                    }
                    else {
                        char error[MAX];
                        int n = sprintf(error, "Comando Inválido: %s\n", com);
                        write(1, &error, n);
                    }

                    bzero(answer, MAX);
                    if((r = read(fd_fifoR,&answer,sizeof(answer))) > 0)
                        write(1, &answer, r);
                }   
            }
            else { //Pela linha de comandos
                char **args = argv + 1;
                char **coms = parseLinha(args);
                int j = 0;
                while(coms[j] != NULL){
                    printf("Comando %i: %s\n", j, coms[j]);
                    write(fd_fifoW, coms[j], strlen(coms[j]));
                    bzero(answer, MAX);
                    if((r = read(fd_fifoR,&answer,sizeof(answer))) > 0)
                        write(1, &answer, r);

                    j++;
                }
            }          
            _exit(0);
        default:
            wait(&status);
            break;
    }   

    close(fd_fifoR);
    close(fd_fifoW);
    return 0;

}
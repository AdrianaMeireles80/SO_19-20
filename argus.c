#include "argus.h"

//Função que apresenta o menu de ajuda
void helpGuide(int type){
    char coms[MAX];

    if(type == 1){
        strcat(coms, "-i segs\t[Definir Tempo de Inatividade]\n-m segs\t[Definir Tempo de Execução Máximo]\n");
        strcat(coms,"-e \"p1 | ... | pn\"\t[Executar Tarefa]\n-l\t\t[Listar Tarefas]\n");
        strcat(coms,"-t taskNum\t[Terminar Tarefa]\n-r\t\t[Consultar Histórico]\n");
        strcat(coms,"-o taskNum\t[Consultar Output]\n");
    }
    if(type == 2){
        strcat(coms,"tempo-inactividade segs\ntempo-execucao segs\n");
        strcat(coms,"executar \"p1 | ... | pn\"\nlistar\n");
        strcat(coms,"terminar taskNum\nhistorico\noutput taskNum\n\n");
    }

    write(1, &coms, strlen(coms));
}

//Função que faz parsing de um comando proveniente do stdin
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
            strcmp(tokens[0],"terminar")  == 0|| strcmp(tokens[0],"historico\n") == 0 || strcmp(tokens[0],"output") == 0)
        return 1;
    else return 0;
}

//Função que faz parsing de um comando proveniente da linha de comandos
char* parseLinha(char* args[]){
    char *comandos=malloc(sizeof(char)*MAX);

    int i = 0;
    char com[MAX];
    if(!strcmp(args[i], "-i") && args[i+1] != NULL){
        bzero(com, MAX);
        sprintf(com, "tempo-inactividade %d\n", atoi(args[i+1]));
        comandos = strdup(com);
    }
    else if(!strcmp(args[i], "-m") && args[i+1] != NULL){
        bzero(com, MAX);
        sprintf(com, "tempo-execucao %d\n", atoi(args[i+1]));
        comandos = strdup(com);
    }
    else if(!strcmp(args[i], "-e") && args[i+1] != NULL){
        bzero(com, MAX);
        sprintf(com, "executar \"%s\"\n", args[i+1]);
        comandos = strdup(com);
    }
    else if(!strcmp(args[i], "-l")){
        bzero(com, MAX);
        sprintf(com, "listar\n");
        comandos = strdup(com);
    }
    else if(!strcmp(args[i], "-r")){
        bzero(com, MAX);
        sprintf(com, "historico\n");
        comandos = strdup(com);
    }
    else if(!strcmp(args[i], "-t") && args[i+1] != NULL){
        bzero(com, MAX);
        sprintf(com, "terminar %d\n", atoi(args[i+1]));
        comandos = strdup(com);
    }
    else if(!strcmp(args[i], "-h")){
        helpGuide(1);
    }
    else if(!strcmp(args[i], "-o") && args[i+1] != NULL){
        bzero(com, MAX);
        sprintf(com, "output %d\n", atoi(args[i+1]));
        comandos = strdup(com);
    }
    else {
        char error[MAX];
        int n = sprintf(error, "Comando Inválido: %s\n", args[i]);
        write(1, &error, n); 
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
                        helpGuide(2);
                    else if(parseCommand(com)){
                        write(fd_fifoW, &buf, r);

                        bzero(buf, MAX);
                        bzero(answer, MAX);
                        if((r = read(fd_fifoR, &answer, sizeof(answer))) > 0)
                            write(1, &answer, r);
                    }
                    else {
                        char error[MAX];
                        int n = sprintf(error, "Comando Inválido: %s\n", com);
                        write(1, &error, n);
                    }
                }   
            }
            else { //Pela linha de comandos
                char **args = argv + 1;
                char *coms = parseLinha(args);

                if(strlen(coms) > 0){
                    write(fd_fifoW, coms, strlen(coms));
                    bzero(answer, MAX);
                    if((r = read(fd_fifoR,&answer,sizeof(answer))) > 0)
                        write(1, &answer, r);
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
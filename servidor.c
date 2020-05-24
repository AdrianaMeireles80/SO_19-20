#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX 1024

int maxInactivity = -1;
int maxExecution = -1;
char **currentTasks; //fila com as tasks a executar
//precisa-se do nr e qual é a task -- cria se estrutura pequena com essas duas cenas?
char **endedTasks;

int readLine(int fd, char* buf, int tam){
	int j = 0;
	while(j < tam && read(fd,buf+j,1) > 0 && buf[j] != '\n')
		j++;

	if(j >= tam)	
        buf[j] = '\0';
	else buf[j+1] = '\0';

	return j;
}

void executeTask(int fd, char* task) {
    //Tipo a mysystem das aulas
}

void listTasks(int fd){
    //Ler as currentTasks e imprimir para o fifo
}

void endTask(int task){
    //kill da task, remover das current para as ended
}

void history(int fd){
    //Ler as endedTasks e imprimir para o fifo
    //guardar o historico em ficheiro?
}

void helpGuide(int fd){
    char coms[MAX];
    int n;
    n = sprintf(coms, "tempo-inactividade segs\ntempo-execucao segs\nexecutar 'p1 | p2 ... | pn'\nlistar\nterminar taskNum\nhistorico\n");
    write(fd, &coms, n);
}

int main(int argc, char *argv[]){
    int f, r, fd_fifoR, fd_fifoW;
    char buf[MAX];
    char *token, *command;
    char *tokens[20];

    //Criação dos fifos
    //Fifo usado pelo servidor para ler o que vem do cliente 
    if (mkfifo("fifo1", 0666) == -1){
        perror("mkfifo");
    }
    //Fifo usado pelo servidor para enviar respostas para o cliente 
    if (mkfifo("fifo2", 0666) == -1){
        perror("mkfifo");
    }

    //Abertura dos fifos
    fd_fifoR = open("fifo1", O_RDONLY);
    fd_fifoW = open("fifo2", O_WRONLY);

    if(fd_fifoR == -1 || fd_fifoW == -1){
        perror("open fifo");
        exit(-1);
    }

    while((r = readLine(fd_fifoR, buf, sizeof(buf))) > 0){
        buf[r] = '\0'; //retira \n
        command = strdup(buf);
        int i = 0;
        while((token = strsep(&command, " ")) != NULL){
            tokens[i] = token;
            i++;
        }

        if(!strcmp(tokens[0], "tempo-inactividade")){
            maxInactivity = atoi(tokens[1]);
        }
        else if (!strcmp(tokens[0], "tempo-execucao")){
            maxExecution = atoi(tokens[1]);
        }
        else if (!strcmp(tokens[0], "executar")){
            int pos = strlen(tokens[0]) + 2;
            char *task = strdup(buf + pos);
            task[strlen(task)-1] = '\0';
            executeTask(fd_fifoW, task);
        }
        else if (!strcmp(tokens[0], "listar")){
            listTasks(fd_fifoW);
        }
        else if (!strcmp(tokens[0], "terminar")){
            int task = atoi(tokens[1]);
            endTask(task);
        }
        else if (!strcmp(tokens[0], "historico")){
            history(fd_fifoW);
        }
        else if (!strcmp(tokens[0], "ajuda")){
            helpGuide(fd_fifoW);
        }
        else {
            char answer[MAX];
            int n = sprintf(answer, "Comando inválido: %s.\n", tokens[0]);
            write(fd_fifoW, &answer, n);
        }

    }

    close(fd_fifoR);
    close(fd_fifoW);
    return 0;

}
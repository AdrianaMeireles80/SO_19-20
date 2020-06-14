CC=gcc

all:	def.c aux.c
	$(CC) -Wall -o argusd argusd.c
	$(CC) -Wall -o argus argus.c

clean:
	rm -f argus argusd historico.txt fifo1 fifo2 log log.idx
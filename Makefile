CC=gcc

make:	def.c aux.c
	$(CC) -o argusd argusd.c
	$(CC) -o argus argus.c

clean:
	rm -f argus argusd historico.txt fifo1 fifo2 log log.idx
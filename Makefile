CC=gcc

make:	def.c aux.c
	$(CC) -o servidor servidor.c
	$(CC) -o argus cliente.c

clean:
	rm -f argus servidor historico.txt fifo1 fifo2 log log.idx
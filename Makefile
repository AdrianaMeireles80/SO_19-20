CC=gcc

make:	servidor cliente

servidor:	def.c aux.c
	$(CC) -o servidor servidor.c

cliente:	
	$(CC) -o argus cliente.c

clean:
	rm -f argus servidor historico.txt fifo1 fifo2
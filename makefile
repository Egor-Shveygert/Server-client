all: client.c server.c
	gcc -o client client.c -Wall -lm -lpthread
	gcc -o server server.c -Wall -lm -lpthread

runs: server
	./server

runc: client
	./client
clear:
	echo "Delete old main "
	rm server client


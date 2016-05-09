all: client server1 server2

functions.o: functions.c functions.h
	gcc -c functions.c -o functions.o

client: functions.o client.o 
	gcc -Wall client.o functions.o -o client

client.o: client.c
	gcc -Wall -c client.c -o client.o

server1: functions.o server1.o 
	gcc -Wall functions.o server1.o -o server1

server1.o: server1.c
	gcc -Wall -c server1.c -o server1.o

server2: functions.o server2.o
	gcc -Wall functions.o server2.o -o server2

server2.o: server2.c
	gcc -Wall -c server2.c -o server2.o

clean:
	rm *.o server1 server2 client

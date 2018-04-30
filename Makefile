CC = gcc
CFLAGS = -I./include

OBJ_SERVER = dropboxServer.o clientList.o



all: dropboxServer dropboxClient

dropboxServer: $(OBJ_SERVER)
	$(CC) $(CFLAGS) -o $@ $^

dropboxClient: ./src/dropboxClient.c 
	$(CC) -o $@ $^

%.o: src/%.c include/*.h 
	$(CC) $(CFLAGS) -c -o $@ $<



.PHONY: clean

clean: 
	rm -vf *.o dropboxServer dropboxClient
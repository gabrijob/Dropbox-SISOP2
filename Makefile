CC = gcc
CFLAGS = -I./include
SRC_DIR = ./src/
SRC_DIR = ./include/

OBJ_SERVER = dropboxServer.o dropboxUtils.o
OBJ_CLIENT = dropboxClient.o dropboxUtils.o



all: util client server

util: $(SRC_DIR)dropboxUtil.c
	$(CC) -c -o 

dropboxClient: $(OBJ_CLIENT)  
	$(CC) $(CFLAGS) -o $@ $^ 

%.o: src/%.c include/*.h 
	$(CC) $(CFLAGS) -c -o $@ $<


.PHONY: clean

clean: 
	rm -vf *.o dropboxServer dropboxClient

CC = gcc
CFLAGS = -Wall
SRC_DIR = ./src/
INC_DIR = ./include/
BIN_DIR = ./bin/
DST_DIR = ./dst/

OBJ_FILES = $(BIN_DIR)dropboxUtil.o
#CLIENT_FILES = $(BIN_DIR)client/*.o
#SERVER_FILES = $(BIN_DIR)server/*.o


all: util client server
	@echo "All files compiled!"

util:	$(SRC_DIR)dropboxUtil.c
	@echo "\nCompilando módulos utilitários..."
	$(CC) $(CFLAGS) -c -o $(BIN_DIR)dropboxUtil.o -I$(INC_DIR) $(SRC_DIR)dropboxUtil.c

client: $(SRC_DIR)dropboxClient.c util  
	@echo "Linkando objetos e compilando aplicação do cliente."
	$(CC) $(CFLAGS) -o $(DST_DIR)dropboxClient $(SRC_DIR)dropboxClient.c $(OBJ_FILES) -pthread -I$(INC_DIR)

server:	$(SRC_DIR)dropboxServer.c util 
	@echo "Linkando objetos e compilando aplicação do servidor."
	$(CC) $(CFLAGS) -o $(DST_DIR)dropboxServer $(SRC_DIR)dropboxServer.c $(OBJ_FILES) -pthread -I$(INC_DIR)

#%.o: src/%.c include/*.h 
	#$(CC) $(CFLAGS) -c -o $@ $<


#.PHONY: clean

clean: 
	rm -f $(DST_DIR)*.* $(DST_DIR)dropboxClient $(DST_DIR)dropboxServer
	rm -f $(OBJ_FILES) $(CLIENT_FILES) $(SERVER_FILES)	
	#rm -vf *.o dropboxServer dropboxClient

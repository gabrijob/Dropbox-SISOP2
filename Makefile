CC = gcc
CFLAGS = -Wall
SRC_DIR = ./src/
INC_DIR = ./include/
BIN_DIR = ./bin/
DST_DIR = ./dst/

OBJ_FILES = $(BIN_DIR)dropboxUtil.o
CLIENT_FILES = $(BIN_DIR)sync-client.o
SERVER_FILES = $(BIN_DIR)sync-server.o
WATCHER = $(BIN_DIR)watcher.o


all: util client server
	@echo "All files compiled!"

util:	$(SRC_DIR)dropboxUtil.c watcher client-sync server-sync
	@echo "\nCompilando módulos utilitários..."
	$(CC) $(CFLAGS) -c -o $(BIN_DIR)dropboxUtil.o -I$(INC_DIR) $(SRC_DIR)dropboxUtil.c

watcher:	$(SRC_DIR)watcher.c
	@echo "\nCompilando watcher..."
	$(CC) $(FLAGS) -c -o $(BIN_DIR)watcher.o -I$(INC_DIR) $(SRC_DIR)watcher.c

client-sync:	$(SRC_DIR)sync-client.c
	@echo "\nCompilando sync-client..."
	$(CC) $(FLAGS) -c -o $(BIN_DIR)sync-client.o -I$(INC_DIR) $(SRC_DIR)sync-client.c

client: $(SRC_DIR)dropboxClient.c util 
	@echo "Linkando objetos e compilando aplicação do cliente."
	$(CC) $(CFLAGS) -c -o $(DST_DIR)dropboxClient $(SRC_DIR)dropboxClient.c $(OBJ_FILES) -pthread -I$(INC_DIR)

server-sync:	$(SRC_DIR)sync-server.c
	@echo "\nCompilando sync-server..."
	$(CC) $(FLAGS) -c -o $(BIN_DIR)sync-server.o -I$(INC_DIR) $(SRC_DIR)sync-server.c

server:	$(SRC_DIR)dropboxServer.c util
	@echo "Linkando objetos e compilando aplicação do servidor."
	$(CC) $(CFLAGS) -c -o $(DST_DIR)dropboxServer $(SRC_DIR)dropboxServer.c $(OBJ_FILES) -pthread -I$(INC_DIR)

#%.o: src/%.c include/*.h 
	#$(CC) $(CFLAGS) -c -o $@ $<


#.PHONY: clean

clean: 
	rm -f $(DST_DIR)*.* $(DST_DIR)dropboxClient $(DST_DIR)dropboxServer
	rm -f $(OBJ_FILES) $(CLIENT_FILES) $(SERVER_FILES) $(WATCHER)	
	#rm -vf *.o dropboxServer dropboxClient

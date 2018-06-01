CC = gcc
CFLAGS = -Wall

CLIENT_OBJ = client_main.o dropboxClient.o dropboxUtil.o udp_assist.o sync-client.o watcher.o 
SERVER_OBJ = server_main.o dropboxServer.o dropboxUtil.o udp_assist.o sync-server.o


all: client server
	 @echo "All files compiled!"

client: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) $(CLIENT_OBJ) -o $@ -pthread -Iinclude

server: $(SERVER_OBJ)
	$(CC) $(CFLAGS) $(SERVER_OBJ) -o $@ -pthread -Iinclude

%.o: src/%.c include/*.h 
	$(CC) $(CFLAGS) -c -o $@ $< -pthread -Iinclude



#.PHONY: clean

clean: 
	#rm -f $(DST_DIR)*.* $(DST_DIR)dropboxClient $(DST_DIR)dropboxServer
	#rm -f $(OBJ_FILES) $(CLIENT_FILES) $(SERVER_FILES) $(WATCHER)	
	rm -f *.o server client

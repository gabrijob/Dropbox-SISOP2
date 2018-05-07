#include "dropboxServer.h"

void synchronize_client(int sockid_sync, Client* client_sync) { // executa primeiro
  char buffer[BUFFER_SIZE]; // 1 KB buffer
  int status = 0;

  DEBUG_PRINT("Iniciando sincronização do cliente.\n");

  status = read(sockid_sync, buffer, BUFFER_SIZE); // recebe comando de sincronizar
  if (status < 0) {
    DEBUG_PRINT("ERROR reading from socket\n");
  }

  DEBUG_PRINT("COMMAND: %s\n", buffer);
  if(strcmp(buffer, S_SYNC) == 0) {
    DEBUG_PRINT("sincronizar!\n");
  }

  sprintf(buffer, "%d", client_sync->n_files);
  DEBUG_PRINT("Client number of files: %d.\n", client_sync->n_files);
  status = write(sockid_sync, buffer, BUFFER_SIZE); // escreve numero de arquivos no server
  if (status < 0) {
    DEBUG_PRINT("ERROR writing to socket\n");
  }

  for(int i = 0; i < client_sync->n_files; i++) {
    strcpy(buffer, client_sync->file_info[i].name);
    DEBUG_PRINT("Nome do arquivo a enviar: %s\n", client_sync->file_info[i].name);
    status = write(sockid_sync, buffer, BUFFER_SIZE); // envia nome do arquivo para o cliente
    if (status < 0) {
      DEBUG_PRINT("ERROR writing to socket\n");
    }
    strcpy(buffer, client_sync->file_info[i].last_modified);
    DEBUG_PRINT("Last modified: %s\n", client_sync->file_info[i].last_modified);
    status = write(sockid_sync, buffer, BUFFER_SIZE); // envia data de ultima modificacao do arquivo
    if (status < 0) {
      DEBUG_PRINT("ERROR writing to socket\n");
    }

    status = read(sockid_sync, buffer, BUFFER_SIZE);
    if (status < 0) {
      DEBUG_PRINT("ERROR reading from socket\n");
    }
    DEBUG_PRINT("Recebido: %s\n", buffer);
    if(strcmp(buffer, S_DOWNLOAD) == 0){ // se recebeu S_DOWNLOAD do buffer, faz o download
      download(sockid_sync, client_sync);
    }
  }

  DEBUG_PRINT("Encerrando sincronização do cliente.\n");
}

void synchronize_server(int sockid_sync, Client* client_sync) {
  char buffer[BUFFER_SIZE]; // 1 KB buffer
  char path[MAXNAME * 3 + 1];
  char last_modified[MAXNAME];
  char file_name[MAXNAME];
  int  status = 0;
  int  number_files_client = 0;

  DEBUG_PRINT("Iniciando sincronização do servidor.\n");

  status = read(sockid_sync, buffer, BUFFER_SIZE); // le o número de arquivos do cliente
  if (status < 0) {
    DEBUG_PRINT("ERROR reading from socket\n");
  }
  number_files_client = atoi(buffer);
  DEBUG_PRINT("Number files client: %d\n", number_files_client);

  char last_modified_file_2[MAXNAME];
  for(int i = 0; i < number_files_client; i++){
    status = read(sockid_sync, buffer, BUFFER_SIZE); // le o nome do arquivo
    if (status < 0) {
    	DEBUG_PRINT("ERROR reading from socket\n");
    }
    strcpy(file_name, buffer);
    DEBUG_PRINT("Nome recebido: %s\n", file_name);

    status = read(sockid_sync, buffer, BUFFER_SIZE); // le last modified do cliente
    if (status < 0) {
      DEBUG_PRINT("ERROR reading from socket\n");
    }
    strcpy(last_modified, buffer);
    DEBUG_PRINT("Last modified recebido: %s\n", last_modified);

    sprintf(path, "%s/%s/%s", serverInfo.folder, client_sync->userid, file_name);
    getFileModifiedTime(path, last_modified_file_2);

    if(!fileExists(path) || older_file(last_modified, last_modified_file_2) == 1) {
      strcpy(buffer, S_GET);
      status = write(sockid_sync, buffer, BUFFER_SIZE);
      if (status < 0) {
        DEBUG_PRINT("ERROR writing to socket\n");
      }

      status = read(sockid_sync, buffer, BUFFER_SIZE); // le resposta do cliente
      if (status < 0) {
        DEBUG_PRINT("ERROR reading from socket\n");
      }
      DEBUG_PRINT("Recebido: %s\n", buffer);

      if(strcmp(buffer, S_UPLOAD) == 0) {
        upload(sockid_sync, client_sync);
      }
  	} else {
  		strcpy(buffer, S_OK);
  		status = write(sockid_sync, buffer, BUFFER_SIZE); // envia ok
      if (status < 0) {
        DEBUG_PRINT("ERROR writing to socket\n");
      }
  	}
  }

  DEBUG_PRINT("Encerrando sincronização do servidor.\n");
}

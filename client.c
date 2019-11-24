#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <pthread.h>

#include "utils.server.c"

// Structure
#define BUFFER_CLIENT_SIZE 512

struct {
  char buffer[BUFFER_CLIENT_SIZE];
  char username[64];
  char password[64];
  char roomname[64];
  char server_address[16];

  int socket;
  int isRunning;
} ClientProps;

// Sets
void PacketSetUsername(char *packet, char *user) {
  strncat(packet + (sizeof(int) * 3), user, 63);
}
void PacketSetPassword(char *packet, char *pass) {
  strncat(packet + (sizeof(int) * 3) + 64, pass, 63);
}
void PacketSetRoomName(char *packet, char *room) {
  strncat(packet + (sizeof(int) * 3) + (64 * 2), room, 63);
}

// Create
char* PacketClientListRooms(int* shift) {
  char *buffer = ByteBufferAllocate(16);
  PacketSetOperation(buffer, OP_LIST);
  PacketSetType(buffer, TYPE_CLIENT);
  PacketSetProtocolID(buffer);
  *shift = sizeof(int) * 4;
  return buffer;
}
char* PacketClientAccessRoom(char* username, char* password, char* roomName, int* shift) {
  char *buffer = ByteBufferAllocate(256);
  PacketSetOperation(buffer, OP_CONNECT);
  PacketSetType(buffer, TYPE_CLIENT);
  PacketSetProtocolID(buffer);
  *shift = sizeof(int) * 4;

  int error = 0;
  error = isNull(roomName) ? 3 : error;
  error = isNull(password) ? 2 : error;
  error = isNull(username) ? 1 : error;
  PacketSetError(buffer, error);
  if (error) return buffer;

  ByteBufferPutString(buffer, shift, username, 64);
  ByteBufferPutString(buffer, shift, password, 64);
  ByteBufferPutString(buffer, shift, roomName, 64);
  return buffer;
}
char* PacketClientCreateRoom(char* username, char* password, char* roomName, int numberOfUsers, int* shift) {
  char *buffer = ByteBufferAllocate(256);
  PacketSetOperation(buffer, OP_CREATE_ROOM);
  PacketSetType(buffer, TYPE_CLIENT);
  PacketSetProtocolID(buffer);
  *shift = sizeof(int) * 4;

  int error = 0;
  error = isNull(roomName) ? 3 : error;
  error = isNull(password) ? 2 : error;
  error = isNull(username) ? 1 : error;
  PacketSetError(buffer, error);
  if (error) return buffer;

  ByteBufferPutString(buffer, shift, username, 64);
  ByteBufferPutString(buffer, shift, password, 64);
  ByteBufferPutString(buffer, shift, roomName, 64);
  ByteBufferPutInt(buffer, shift, numberOfUsers);
  return buffer;
}
void *PacketClientSendMessage(char* username, char* roomName, char* message, int* shift) {
  char *buffer = ByteBufferAllocate(512);
  PacketSetOperation(buffer, OP_SEND_MESSAGE);
  PacketSetType(buffer, TYPE_CLIENT);
  PacketSetProtocolID(buffer);
  *shift = sizeof(int) * 4;

  int error = 0;
  error = isNull(message) ? 4 : error;
  error = isNull(roomName) ? 3 : error;
  error = isNull(username) ? 1 : error;
  PacketSetError(buffer, error);
  if (error) return buffer;

  ByteBufferPutString(buffer, shift, username, 64);
  ByteBufferPutString(buffer, shift, roomName, 64);
  ByteBufferPutString(buffer, shift, message, 256);
  return buffer;
}
void *PacketClientExit(char *username, char *password, int* shift) {
  char *buffer = ByteBufferAllocate(256);
  PacketSetOperation(buffer, OP_EXIT);
  PacketSetType(buffer, TYPE_CLIENT);
  PacketSetProtocolID(buffer);
  *shift = sizeof(int) * 4;

  int error = 0;
  error = isNull(password) ? 2 : error;
  error = isNull(username) ? 1 : error;
  PacketSetError(buffer, error);
  if (error) return buffer;

  ByteBufferPutString(buffer, shift, username, 64);
  ByteBufferPutString(buffer, shift, password, 64);
  return buffer;
}

int PacketReceive(char *buffer, int size) {
  int numberOfBytes = read(ClientProps.socket, buffer, size);
  buffer[numberOfBytes] = 0;
  return numberOfBytes;
}
void PacketSend(char *buffer, int size) {
  send(ClientProps.socket, buffer, size, 0);
}

// Public methods
void ClientSetServerIP(char *ip) {
  strncpy(ClientProps.server_address, ip, 15);
}
void ClientSetUsername(char *username) {
  strncpy(ClientProps.username, username, 64);
}
void ClientSetPassword(char *password) {
  strncpy(ClientProps.password, password, 64);
}
void ClientSetRoomName(char *roomname) {
  strncpy(ClientProps.roomname, roomname, 64);
}
void ClientStopRunning() {
  ClientProps.isRunning = 0;
}
void ClientStartRunning() {
  ClientProps.isRunning = 1;
}

void ClientInit() {
  int error;

  ClientProps.socket = socket(AF_INET, SOCK_STREAM, 0);
  rejectCriticalError("(socket) Failed to create client socket", ClientProps.socket == -1);

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(12345);
  error = inet_pton(AF_INET, ClientProps.server_address, &serv_addr.sin_addr);
  rejectCriticalError("(inet_pton) Invalid address", error == -1);

  error = connect(ClientProps.socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  rejectCriticalError("(connect) Failed to connect with the server", error == -1);
}

char* ClientListRooms() {
  return NULL;
}
int ClientAccessRoom() {
  int size;
  char *packet = PacketClientAccessRoom(ClientProps.username, ClientProps.password, ClientProps.roomname, &size);
  send(ClientProps.socket, packet, size, 0);

  // PacketReceive(packet, 256);

  free(packet);
  return 0;
}
int ClientCreateRoom(int numberOfUsers) {
  int size;
  char *packet = PacketClientCreateRoom(ClientProps.username, ClientProps.password, ClientProps.roomname, numberOfUsers, &size);
  send(ClientProps.socket, packet, size, 0);
  
  // PacketReceive(packet, 256);

  free(packet);
  return 0;
}
int ClientSendMessage(char *message) {
  int size;
  char *packet = PacketClientSendMessage(ClientProps.username, ClientProps.roomname, message, &size);
  send(ClientProps.socket, packet, size, 0);

  free(packet);
  return 0;
}

// Runners
void *ClientSender(void *arg) {
  char buffer[BUFFER_CLIENT_SIZE + 1] = {0};
  int size;
  while (ClientProps.isRunning) {
    scanf(" %[^\n]", buffer);
    if (strlen(buffer) > 0) {
      ClientSendMessage(buffer);
      printf("Send: %s\n", buffer);
      buffer[0] = 0;
    }
  }
}
void ClientSenderRun(pthread_t* thread) {
  pthread_create(thread, NULL, ClientSender, NULL);
}

void *ClientReceiver(void* callback) {
  void (*callable)(char*) = callback;

  char buffer[BUFFER_CLIENT_SIZE + 1] = {0};
  while (ClientProps.isRunning) {
    int numberOfBytes = PacketReceive(buffer, BUFFER_CLIENT_SIZE);
    if (numberOfBytes == 0) {
      printf("Server disconnected\n");
      ClientProps.isRunning = 0;
    } else {
      if (callable != NULL) {
        callable(buffer);
      } else {
        printf("Received: '%s'\n", buffer);
      }
    }
  }
}
void ClientReceiverRun(pthread_t* thread, void (*callback)(void*)) {
  pthread_create(thread, NULL, ClientReceiver, callback);
}
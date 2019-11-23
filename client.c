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
void setUsernameOnPacket(char *packet, char *user) {
  strncat(packet + (sizeof(int) * 3), user, 63);
}
void setPasswordOnPacket(char *packet, char *pass) {
  strncat(packet + (sizeof(int) * 3) + 64, pass, 63);
}
void setNameRoomOnPacket(char *packet, char *room) {
  strncat(packet + (sizeof(int) * 3) + (64 * 2), room, 63);
}

// Create
char* createPacketToListRoomsByClient(int* shift) {
  char *buffer = ByteBufferAllocate(16);
  setOperationOnPacket(buffer, OP_LIST);
  setTypeOnPacket(buffer, TYPE_CLIENT);
  setIDProtocolPacket(buffer);
  *shift = sizeof(int) * 4;
  return buffer;
}
char* createPacketToAccessRoomByClient(char* username, char* password, char* roomName, int* shift) {
  char *buffer = ByteBufferAllocate(256);
  setOperationOnPacket(buffer, OP_CONNECT);
  setTypeOnPacket(buffer, TYPE_CLIENT);
  setIDProtocolPacket(buffer);
  *shift = sizeof(int) * 4;

  int error = 0;
  error = isNull(roomName) ? 3 : error;
  error = isNull(password) ? 2 : error;
  error = isNull(username) ? 1 : error;
  setErrorOnPacket(buffer, error);
  if (error) return buffer;

  ByteBufferPutString(buffer, shift, username, 64);
  ByteBufferPutString(buffer, shift, password, 64);
  ByteBufferPutString(buffer, shift, roomName, 64);
  return buffer;
}
char* createPacketToCreateRoomByClient(char* username, char* password, char* roomName, int numberOfUsers, int* shift) {
  char *buffer = ByteBufferAllocate(256);
  setOperationOnPacket(buffer, OP_CREATE_ROOM);
  setTypeOnPacket(buffer, TYPE_CLIENT);
  setIDProtocolPacket(buffer);
  *shift = sizeof(int) * 4;

  int error = 0;
  error = isNull(roomName) ? 3 : error;
  error = isNull(password) ? 2 : error;
  error = isNull(username) ? 1 : error;
  setErrorOnPacket(buffer, error);
  if (error) return buffer;

  ByteBufferPutString(buffer, shift, username, 64);
  ByteBufferPutString(buffer, shift, password, 64);
  ByteBufferPutString(buffer, shift, roomName, 64);
  ByteBufferPutInt(buffer, shift, numberOfUsers);
  return buffer;
}
void *createPacketToSendMessageByClient(char* username, char* roomName, char* message, int* shift) {
  char *buffer = ByteBufferAllocate(512);
  setOperationOnPacket(buffer, OP_SEND_MESSAGE);
  setTypeOnPacket(buffer, TYPE_CLIENT);
  setIDProtocolPacket(buffer);
  *shift = sizeof(int) * 4;

  int error = 0;
  error = isNull(message) ? 4 : error;
  error = isNull(roomName) ? 3 : error;
  error = isNull(username) ? 1 : error;
  setErrorOnPacket(buffer, error);
  if (error) return buffer;

  ByteBufferPutString(buffer, shift, username, 64);
  ByteBufferPutString(buffer, shift, roomName, 64);
  ByteBufferPutString(buffer, shift, message, 256);
  return buffer;
}
void *createPacketToExitByClient(char *username, char *password, int* shift) {
  char *buffer = ByteBufferAllocate(256);
  setOperationOnPacket(buffer, OP_EXIT);
  setTypeOnPacket(buffer, TYPE_CLIENT);
  setIDProtocolPacket(buffer);
  *shift = sizeof(int) * 4;

  int error = 0;
  error = isNull(password) ? 2 : error;
  error = isNull(username) ? 1 : error;
  setErrorOnPacket(buffer, error);
  if (error) return buffer;

  ByteBufferPutString(buffer, shift, username, 64);
  ByteBufferPutString(buffer, shift, password, 64);
  return buffer;
}

int receivePacket(char *buffer, int size) {
  int numberOfBytes = read(ClientProps.socket, buffer, size);
  buffer[numberOfBytes] = 0;
  return numberOfBytes;
}
void sendPacket(char *buffer, int size) {
  send(ClientProps.socket, buffer, size, 0);
}

// Public methods
void setServerIP(char *ip) {
  strncpy(ClientProps.server_address, ip, 15);
}
void setUsername(char *username) {
  strncpy(ClientProps.username, username, 64);
}
void setPassword(char *password) {
  strncpy(ClientProps.password, password, 64);
}
void setRoomname(char *roomname) {
  strncpy(ClientProps.roomname, roomname, 64);
}
void stopRunning() {
  ClientProps.isRunning = 0;
}
void startRunning() {
  ClientProps.isRunning = 1;
}

void *sender(void *arg) {
  char buffer[BUFFER_CLIENT_SIZE + 1] = {0};
  int size;
  while (ClientProps.isRunning) {
    setbuf(stdin , NULL);
    scanf("%[^\n]", buffer);
    if (strlen(buffer) > 0) {
      char *packet = createPacketToSendMessageByClient("Daniel", "room", buffer, &size);
      sendPacket(packet, size);
      free(packet);
      printf("Send: %s\n", buffer);
      buffer[0] = 0;
    }
  }
}
void senderRunner(pthread_t* thread) {
  pthread_create(thread, NULL, sender, NULL);
}
void *receiver(void (*callback)(void*)) {
  char buffer[BUFFER_CLIENT_SIZE + 1] = {0};
  while (ClientProps.isRunning) {
    int numberOfBytes = receivePacket(buffer, BUFFER_CLIENT_SIZE);
    if (numberOfBytes == 0) {
      printf("Server disconnected\n");
      ClientProps.isRunning = 0;
    } else {
      if (callback != NULL) {
        callback(buffer);
      } else {
        printf("Received: '%s'\n", buffer);
      }
    }
  }
}
void receiverRunner(pthread_t* thread, void (*callback)(void*)) {
  pthread_create(thread, NULL, receiver, callback);
}

void initClientSocket() {
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

int listRooms() {
  return 0;
}
int accessRoom() {
  int size;
  char *packet = createPacketToAccessRoomByClient(ClientProps.username, ClientProps.password, ClientProps.roomname, &size);
  send(ClientProps.socket, packet, size, 0);

  // receivePacket(packet, 256);

  free(packet);
  return 0;
}
int createRoom(int numberOfUsers) {
  int size;
  char *packet = createPacketToCreateRoomByClient(ClientProps.username, ClientProps.password, ClientProps.roomname, numberOfUsers, &size);
  send(ClientProps.socket, packet, size, 0);
  
  // receivePacket(packet, 256);

  free(packet);
  return 0;
}
int sendMessage(char *message) {
  int size;
  char *packet = createPacketToSendMessageByClient(ClientProps.username, ClientProps.roomname, message, &size);
  send(ClientProps.socket, packet, size, 0);

  free(packet);
  return 0;
}

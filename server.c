#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/time.h>
#include <errno.h>

#include "utils.server.c"

#define BUFFER_SERVER_SIZE 1024

SocketAddrIn masterAddress;
int masterAddressLength = 0;

int ID = 0;
fd_set FD;
int FAMILY = AF_INET;           // AF_INET: Address family Internet Protocol v4 addresses
int TYPE = SOCK_STREAM;         // SOCK_STREAM: Connection-based TCP
int MASK_ADDRESS = INADDR_ANY; // Address to accept any incoming messages.
int PORT = 12345;

int CLIENTS_LIMIT = 50;
int CLIENTS_COUNT = 0;
int CLIENTS_LIST[50] = {0};
int CLIENTS_ROOMS[50] = {0};

// Read
void ServerReadPacketListRooms(int sd, char* packet) {
  int shift = 0;
  int error = ByteBufferGetInt(packet, &shift);
  int type = ByteBufferGetInt(packet, &shift);
  int op = ByteBufferGetInt(packet, &shift);
  int IDprotocol = ByteBufferGetInt(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d'", error, type, op);
  printf("\n");
}
void ServerReadPacketConnect(int sd, char* packet) {
  int shift = 0;
  int error = ByteBufferGetInt(packet, &shift);
  int type = ByteBufferGetInt(packet, &shift);
  int op = ByteBufferGetInt(packet, &shift);
  int IDprotocol = ByteBufferGetInt(packet, &shift);
  char* username = ByteBufferGetString(packet, &shift);
  char* password = ByteBufferGetString(packet, &shift);
  char* roomName = ByteBufferGetString(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d'", error, type, op);
  printf(", Username: '%s', Password: '%s', Room name: '%s'\n", username, password, roomName);
}
void ServerReadPacketCreateRoom(int sd, char* packet) {
  int shift = 0;
  int error = ByteBufferGetInt(packet, &shift);
  int type = ByteBufferGetInt(packet, &shift);
  int op = ByteBufferGetInt(packet, &shift);
  int IDprotocol = ByteBufferGetInt(packet, &shift);
  char* username = ByteBufferGetString(packet, &shift);
  char* password = ByteBufferGetString(packet, &shift);
  char* roomName = ByteBufferGetString(packet, &shift);
  int numberOfUsers = ByteBufferGetInt(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d'", error, type, op);
  printf(", Username: '%s', Password: '%s', Room name: '%s', Number of users: '%d'\n", username, password, roomName, numberOfUsers);
}
void ServerReadPacketSendMessage(int sd, char* packet) {
  int shift = 0;
  int error = ByteBufferGetInt(packet, &shift);
  int type = ByteBufferGetInt(packet, &shift);
  int op = ByteBufferGetInt(packet, &shift);
  int IDprotocol = ByteBufferGetInt(packet, &shift);
  char* username = ByteBufferGetString(packet, &shift);
  char* roomName = ByteBufferGetString(packet, &shift);
  char* message = ByteBufferGetString(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d'", error, type, op);
  printf(", Username: '%s', RoomName: '%s', Message: '%s'\n", username, roomName, message);
}
void ServerReadPacketExit(int sd, char* packet) {
  int shift = 0;
  int error = ByteBufferGetInt(packet, &shift);
  int type = ByteBufferGetInt(packet, &shift);
  int op = ByteBufferGetInt(packet, &shift);
  int IDprotocol = ByteBufferGetInt(packet, &shift);
  char* username = ByteBufferGetString(packet, &shift);
  char* password = ByteBufferGetString(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d'", error, type, op);
  printf(", Username: '%s', Password: '%s'\n", username, password);
}

void ServerReadPacket(int sd, char* packet, int size) {
  if (size < (4 * sizeof(int))) {
    printf("LOG: Malformed packet\n");
    return;
  }
  int error = PacketGetError(packet);
  if (!error) {
    int op = PacketGetOperation(packet);
    switch(op) {
      case OP_LIST:
      ServerReadPacketListRooms(sd, packet);
      break;
      case OP_CONNECT:
      ServerReadPacketConnect(sd, packet);
      break;
      case OP_CREATE_ROOM:
      ServerReadPacketCreateRoom(sd, packet);
      break;
      case OP_SEND_MESSAGE:
      ServerReadPacketSendMessage(sd, packet);
      break;
      case OP_EXIT:
      ServerReadPacketExit(sd, packet);
      break;
    }
  } else {
    PacketCheckError(error);
  }
}

void ServerInit() {
  int opt = 1, error;

  ID = socket(FAMILY, TYPE, 0);
  rejectCriticalError("(socket) Failed to create master socket", ID == -1);

  error = setsockopt(ID, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
  rejectCriticalError("(setsockopt) Failed to update master socket to allow multiples connections", error == -1);

  // Setup master socket
  masterAddress.sin_family = FAMILY;
  masterAddress.sin_addr.s_addr = MASK_ADDRESS;
  masterAddress.sin_port = htons(PORT);

  error = bind(ID, (SocketAddr*) &masterAddress, sizeof(masterAddress));
  rejectCriticalError("(bind) Failed to bind master socket in give address", error == -1);

  error = listen(ID, 3);
  rejectCriticalError("(listen) Failed to prepare to accept connections", error == -1);

  printf(">>> Sizeof(masterAddress): %ld\n", sizeof(masterAddress));
  printf(">>> Server socket listen on port '%d'\n", PORT);
}
void ServerExecute() {
  char buffer[BUFFER_SERVER_SIZE + 1];
  int i, maxID = ID;

  while (1) {
    FD_ZERO(&FD);

    FD_SET(ID, &FD);

    for (i = 0; i < CLIENTS_LIMIT; i++) {
      int sd = CLIENTS_LIST[i]; // SocketDescriptor
      if (sd > 0)
        FD_SET(sd, &FD);
      if (sd > maxID)
        maxID = sd;
    }

    int id = select(maxID + 1, &FD, NULL, NULL, NULL);
    printf("ID: %d\n", id);

    if (id < -1 && errno != EINTR)
      printf("(select) Fail to select an ID of set of sockets\n");
    
    if (FD_ISSET(ID, &FD)) {
      if (CLIENTS_COUNT == CLIENTS_LIMIT) {
        printf("Server is full\n");
      } else {
        int newSocket = accept(ID, NULL, NULL);
        rejectCriticalError("(accept) Error when master socket accept new connection\n", newSocket == -1);

        printf("New connection\n");
        showHostInfos(newSocket);

        for (i = 0; i < CLIENTS_LIMIT; i++) {
          if (CLIENTS_LIST[i] == 0) {
            CLIENTS_LIST[i] = newSocket;
            CLIENTS_COUNT++;
            break;
          }
        }
      }
    }

    for (i = 0; i < CLIENTS_LIMIT; i++) {
      int sd = CLIENTS_LIST[i];
      if (FD_ISSET(sd, &FD)) {
        int size = read(sd, buffer, BUFFER_SERVER_SIZE);
        if (size == 0) {
          printf("Host disconnected\n");
          showHostInfos(sd);
          CLIENTS_LIST[i] = 0;
          break;
        } else {
          buffer[size] = 0;
          printf("Broadcast %d\n", size);
          for (int j = 0; j < CLIENTS_LIMIT; j++) {
            int sd2 = CLIENTS_LIST[j];
            if (sd2 != sd)
              send(sd2, buffer, size, 0);
          }
          printf("Received: '%s'\n", buffer);
          showHostInfos(sd);
          ServerReadPacket(sd, buffer, size);
          break;
        }
      }
    }
  }
}

int main() {
  ServerInit();
  ServerExecute();
  return 0;
}

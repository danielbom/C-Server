#include "client.c"

void readPacketToListRoomsByClient(char *packet) {
  int shift = 0;
  int error = ByteBufferGetInt(packet, &shift);
  int type = ByteBufferGetInt(packet, &shift);
  int op = ByteBufferGetInt(packet, &shift);
  int IDprotocol = ByteBufferGetInt(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d', Protocol: '%d'", error, type, op, IDprotocol);
  printf("\n");
}
void readPacketToConnectByClient(char *packet) {
  int shift = 0;
  int error = ByteBufferGetInt(packet, &shift);
  int type = ByteBufferGetInt(packet, &shift);
  int op = ByteBufferGetInt(packet, &shift);
  int IDprotocol = ByteBufferGetInt(packet, &shift);
  char *username = ByteBufferGetString(packet, &shift);
  char *password = ByteBufferGetString(packet, &shift);
  char *roomName = ByteBufferGetString(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d', Protocol: '%d'", error, type, op, IDprotocol);
  printf(", Username: '%s', Password: '%s', Room name: '%s'\n", username, password, roomName);
}
void readPacketToCreateRoomByClient(char *packet) {
  int shift = 0;
  int error = ByteBufferGetInt(packet, &shift);
  int type = ByteBufferGetInt(packet, &shift);
  int op = ByteBufferGetInt(packet, &shift);
  int IDprotocol = ByteBufferGetInt(packet, &shift);
  char *username = ByteBufferGetString(packet, &shift);
  char *password = ByteBufferGetString(packet, &shift);
  char *roomName = ByteBufferGetString(packet, &shift);
  int numberOfUsers = ByteBufferGetInt(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d', Protocol: '%d'", error, type, op, IDprotocol);
  printf(", Username: '%s', Password: '%s', Room name: '%s', Number of users: '%d'\n", username, password, roomName, numberOfUsers);
}
void readPacketToSendMessageByClient(char *packet) {
  int shift = 0;
  int error = ByteBufferGetInt(packet, &shift);
  int type = ByteBufferGetInt(packet, &shift);
  int op = ByteBufferGetInt(packet, &shift);
  int IDprotocol = ByteBufferGetInt(packet, &shift);
  char *username = ByteBufferGetString(packet, &shift);
  char *roomName = ByteBufferGetString(packet, &shift);
  char *message = ByteBufferGetString(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d', Protocol: '%d'", error, type, op, IDprotocol);
  printf(", Username: '%s', RoomName: '%s', Message: '%s'\n", username, roomName, message);
}
void readPacketToExitByClient(char *packet) {
  int shift = 0;
  int error = ByteBufferGetInt(packet, &shift);
  int type = ByteBufferGetInt(packet, &shift);
  int op = ByteBufferGetInt(packet, &shift);
  int IDprotocol = ByteBufferGetInt(packet, &shift);
  char *username = ByteBufferGetString(packet, &shift);
  char *password = ByteBufferGetString(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d', Protocol: '%d'", error, type, op, IDprotocol);
  printf(", Username: '%s', Password: '%s'\n", username, password);
}

void testOnePacket(char *packet) {
  int error = getErrorOfPacket(packet);
  if (!error) {
    int op = getOperationOfPacket(packet);
    switch(op) {
      case OP_LIST:
      readPacketToListRoomsByClient(packet);
      break;
      case OP_CONNECT:
      readPacketToConnectByClient(packet);
      break;
      case OP_CREATE_ROOM:
      readPacketToCreateRoomByClient(packet);
      break;
      case OP_SEND_MESSAGE:
      readPacketToSendMessageByClient(packet);
      break;
      case OP_EXIT:
      readPacketToExitByClient(packet);
      break;
    }
  } else {
    checkErrorOfServer(error);
  }
  free(packet);
}
void testPackets() {
  int size;
  testOnePacket(createPacketToListRoomsByClient(&size));
  testOnePacket(createPacketToAccessRoomByClient("user-x", "123456", "-room-", &size));
  testOnePacket(createPacketToCreateRoomByClient("user-x", "123456", "-room-", 2, &size));
  testOnePacket(createPacketToSendMessageByClient("user-x", "-room-", "Hello world!", &size));
  testOnePacket(createPacketToExitByClient("user-x", "123456", &size));
}

void testErrors() {
  int size;
  testOnePacket(createPacketToAccessRoomByClient(NULL, "123456", "-room-", &size));
  testOnePacket(createPacketToCreateRoomByClient("user-123", "123456", NULL, 2, &size));
  testOnePacket(createPacketToSendMessageByClient("user-123", "-room-", NULL, &size));
  testOnePacket(createPacketToExitByClient("user-123", NULL, &size));
  printf("\n");
  testOnePacket(createPacketToAccessRoomByClient(NULL, NULL, NULL, &size));
  testOnePacket(createPacketToCreateRoomByClient(NULL, NULL, NULL, 2, &size));
  testOnePacket(createPacketToSendMessageByClient(NULL, NULL, NULL, &size));
  testOnePacket(createPacketToExitByClient(NULL, NULL, &size));
}

void testsClient() {
  testPackets();
  printf("\n");
  testErrors();
}

void connectClient() {
  int valread;

  setServerIP("127.0.0.1");
  initClientSocket();

  pthread_t thread;
  senderRunner(&thread);
  receiverRunner(&thread, NULL);

  printf("Loop...\n");
  while (ClientProps.isRunning) {}
}

int main(int argc, char const *argv[]) {
  connectClient();
  return 0;
}
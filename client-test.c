#include "client.c"

void readPacketToListRoomsByClient(char *packet) {
  int shift      = 0;
  int error      = ByteBufferGetInt(packet, &shift);
  int type       = ByteBufferGetInt(packet, &shift);
  int op         = ByteBufferGetInt(packet, &shift);
  int IDprotocol = ByteBufferGetInt(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d', Protocol: '%d'", error, type, op, IDprotocol);
  printf("\n");
}
void readPacketToConnectByClient(char *packet) {
  int shift      = 0;
  int error      = ByteBufferGetInt(packet, &shift);
  int type       = ByteBufferGetInt(packet, &shift);
  int op         = ByteBufferGetInt(packet, &shift);
  int IDprotocol = ByteBufferGetInt(packet, &shift);
  char *username = ByteBufferGetString(packet, &shift);
  char *password = ByteBufferGetString(packet, &shift);
  char *roomName = ByteBufferGetString(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d', Protocol: '%d'", error, type, op, IDprotocol);
  printf(", Username: '%s', Password: '%s', Room name: '%s'\n", username, password, roomName);
}
void readPacketToCreateRoomByClient(char *packet) {
  int shift         = 0;
  int error         = ByteBufferGetInt(packet, &shift);
  int type          = ByteBufferGetInt(packet, &shift);
  int op            = ByteBufferGetInt(packet, &shift);
  int IDprotocol    = ByteBufferGetInt(packet, &shift);
  char *username    = ByteBufferGetString(packet, &shift);
  char *password    = ByteBufferGetString(packet, &shift);
  char *roomName    = ByteBufferGetString(packet, &shift);
  int numberOfUsers = ByteBufferGetInt(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d', Protocol: '%d'", error, type, op, IDprotocol);
  printf(", Username: '%s', Password: '%s', Room name: '%s', Number of users: '%d'\n", username, password, roomName, numberOfUsers);
}
void readPacketToSendMessageByClient(char *packet) {
  int shift      = 0;
  int error      = ByteBufferGetInt(packet, &shift);
  int type       = ByteBufferGetInt(packet, &shift);
  int op         = ByteBufferGetInt(packet, &shift);
  int IDprotocol = ByteBufferGetInt(packet, &shift);
  char *username = ByteBufferGetString(packet, &shift);
  char *roomName = ByteBufferGetString(packet, &shift);
  char *message  = ByteBufferGetString(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d', Protocol: '%d'", error, type, op, IDprotocol);
  printf(", Username: '%s', RoomName: '%s', Message: '%s'\n", username, roomName, message);
}
void readPacketToExitByClient(char *packet) {
  int shift      = 0;
  int error      = ByteBufferGetInt(packet, &shift);
  int type       = ByteBufferGetInt(packet, &shift);
  int op         = ByteBufferGetInt(packet, &shift);
  int IDprotocol = ByteBufferGetInt(packet, &shift);
  char *username = ByteBufferGetString(packet, &shift);
  char *password = ByteBufferGetString(packet, &shift);
  printf("Error: '%d', Type: '%d', Operation: '%d', Protocol: '%d'", error, type, op, IDprotocol);
  printf(", Username: '%s', Password: '%s'\n", username, password);
}

void testOnePacket(char *packet) {
  int error = PacketGetError(packet);
  if (!error) {
    int op = PacketGetOperation(packet);
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
    PacketCheckError(error);
  }
  free(packet);
}
void testPackets() {
  int size;
  testOnePacket(PacketClientListRooms(&size));
  testOnePacket(PacketClientAccessRoom("user-x", "123456", "-room-", &size));
  testOnePacket(PacketClientCreateRoom("user-x", "123456", "-room-", 2, &size));
  testOnePacket(PacketClientSendMessage("user-x", "-room-", "Hello world!", &size));
  testOnePacket(PacketClientExit("user-x", "123456", &size));
}

void testErrors() {
  int size;
  testOnePacket(PacketClientAccessRoom(NULL, "123456", "-room-", &size));
  testOnePacket(PacketClientCreateRoom("user-123", "123456", NULL, 2, &size));
  testOnePacket(PacketClientSendMessage("user-123", "-room-", NULL, &size));
  testOnePacket(PacketClientExit("user-123", NULL, &size));
  printf("\n");
  testOnePacket(PacketClientAccessRoom(NULL, NULL, NULL, &size));
  testOnePacket(PacketClientCreateRoom(NULL, NULL, NULL, 2, &size));
  testOnePacket(PacketClientSendMessage(NULL, NULL, NULL, &size));
  testOnePacket(PacketClientExit(NULL, NULL, &size));
}

void testsClient() {
  testPackets();
  printf("\n");
  testErrors();
}

void receiverHandle(char* username, char* message) {
  printf("[%s]: %s\n", username, message);
}

void testConnectClient() {
  int valread;

  char username[64];
  printf("Username: ");
  scanf("%[^\n]s", username);

  ClientSetReceiveHandle(receiverHandle);
  ClientSetUsername(username);
  ClientSetPassword("pass");
  ClientSetRoomName("room");
  ClientSetServerIP("127.0.0.1");
  ClientInit();

  pthread_t thread;
  ClientSenderRun(&thread);
  ClientReceiverRun(&thread);

  printf("Loop...\n");
  while (ClientProps.isRunning) {}
}

int main(int argc, char const *argv[]) {
  testConnectClient();
  return 0;
}

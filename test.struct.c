#include <stdio.h>
#include <string.h>

struct {
  char buffer[512];
  char server_address[16];

  char username[64];
  char password[64];
  char roomname[64];

  int socket;
  int isRunning;
  int serverport;
} Client;


int main() {
  strcpy(Client.buffer, "test");
  strcpy(Client.server_address, "test");
  strcpy(Client.username, "test");
  strcpy(Client.password, "test");
  strcpy(Client.roomname, "test");

  printf("%s\n", Client.buffer);
  printf("%s\n", Client.server_address);
  printf("%s\n", Client.username);
  printf("%s\n", Client.password);
  printf("%s\n", Client.roomname);
}
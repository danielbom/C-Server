#include <stdio.h>

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
  
}
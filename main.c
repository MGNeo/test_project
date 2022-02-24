#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 9876

void run_server_mode();
void run_client_mode();

int main(int argc, char** argv)
{
  while (1)
  {
    printf("Choose mode:\n");
    printf("s - server mode;\n");
    printf("c - client mode.\n");

    const char input = getchar();

    switch (input)
    {
      case ('s'):
      {
        run_server_mode();
        break;
      }
      case ('c'):
      {
        run_client_mode();
        break;
      }
      default:
      {
        printf("Wrong mode, try again.\n");
        break;
      }
    }
  }

  return 0;
}

void run_server_mode()
{
  printf("Server mode begins.\n");
  
  // Try to create server socket.
  const int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1)
  {
    printf("Server socket can't be created.\n");
    return;
  }
  printf("Server socket has been created.\n");

  // Try to bind server socket to address.
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);
  server_address.sin_addr.s_addr = INADDR_ANY;
  if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) != 0)
  {
    printf("Server socket can't be bound.\n");
    close(server_socket);
    return;
  }
  printf("Server socket has been bound.\n");

  // Try to listen server socket.
  if (listen(server_socket, 1) != 0)
  {
    printf("Server socket can't be listened.\n");
    close(server_socket);
    return;
  }
  printf("Server socket has been listened.\n");

  // Try to accept client socket.
  while (1)
  {   
    const int client_socket = accept(server_socket, NULL, 0);
    if (client_socket == -1)
    {
      printf("Client socket can't be accepted.\n");
      continue;
    }
    printf("Client socket has been accepted.\n");
    
    // Try to receive data from client socket.
    char buffer[256];
    const int size = recv(client_socket, buffer, sizeof(buffer), 0);

    
  }
}

void run_client_mode()
{
  printf("Client mode begins.\n");
}
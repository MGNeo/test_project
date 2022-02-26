#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#define PORT 12345

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
    printf("Server socket can't be created: %s\n", strerror(errno));
    return;
  }
  printf("Server socket has been created.\n");

  // Try to set special socket options.
  const int option = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1)
  {
    printf("Server socket can't reuse address: %s\n", strerror(errno));
    close(server_socket);
    return;
  }

  // Try to bind server socket to address.
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) != 0)
  {
    printf("Server socket can't be bound: %s\n", strerror(errno));
    close(server_socket);
    return;
  }
  printf("Server socket has been bound.\n");

  // Try to listen server socket.
  if (listen(server_socket, 10) != 0)
  {
    printf("Server socket can't be listened: %s\n", strerror(errno));
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
      printf("Client socket can't be accepted: %s\n", strerror(errno));
      continue;
    }
    printf("Client socket has been accepted.\n");
    
    // Try to receive data from client socket.
    char buffer[256];
    int total = 0;
    
    while (total < sizeof(buffer))
    {
      const int size = recv(client_socket, buffer, sizeof(buffer), 0);
    
      if (size == 0)
      {
        printf("Client socket. Connection has been closed.\n");
        close(client_socket);
        break;
      }

      if (size == -1)
      {
        printf("Client socket. Error has occurred: %s", strerror(errno));
        close(client_socket);
        break;
      }

      // We don't really check possible integer overflow.
      total += size;
      if (total > sizeof(buffer))
      {
        printf("Client socket. Size of gotten data is too big.\n");
        close(client_socket);
        break;
      }

      // Print gotten data.
      for (size_t i = 0; i < size; ++i)
      {
        printf("%c", buffer[i]);
      }
    }

    close(client_socket);
  }
  
  close(server_socket);
  printf("Server mode ends.");
}

void run_client_mode()
{
  printf("Client mode begins.\n");

  // Try to create client socket.
  const int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket == -1)
  {
    printf("Client socket can't be created: %s\n", strerror(errno));
    return;
  }
  printf("Client socket has been created.\n");

  // Try to set special socket options.
  const int option = 1;
  if (setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1)
  {
    printf("Client socket can't reuse address: %s\n", strerror(errno));
    close(client_socket);
    return;
  }

  // Try to connect client socket to address.
  struct sockaddr_in client_address;
  client_address.sin_family = AF_INET;
  client_address.sin_port = htons(PORT);
  // We don't really check result of this call.
  //inet_aton(IP, (struct in_addr*)&client_address.sin_addr.s_addr);
  client_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (connect(client_socket, (struct sockaddr*)&client_address, sizeof(client_address)) == -1)
  {
    printf("Client socket can't be connected: %s\n", strerror(errno));
    close(client_socket);
    return;
  }
  printf("Client socket has been connected.\n");

  // Try to send data to server.
  char buffer[256];
  int total = 0;
  // Fill the buffer with data.
  for (size_t i = 0; i < sizeof(buffer); ++i)
  {
    buffer[i] = 'f';
  }

  // Send data.
  while (total < sizeof(buffer))
  {
    const int size = send(client_socket, buffer + total, sizeof(buffer) - total, 0);
    
    if (size == 0)
    {
      printf("Connection has been closed: %s\n", strerror(errno));
      break;
    }

    if (size == -1)
    {
      printf("Error has occured: %s\n", strerror(errno));
      close(client_socket);
      break;
    }

    total += size;
  }

  close(client_socket);
  printf("Client mode ends.\n");
}
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

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
}

void run_client_mode()
{
  printf("Client mode begins.\n");
}
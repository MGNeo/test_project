#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>

#define PORT 12345

pthread_mutexattr_t print_mutexattr;
pthread_mutex_t print_mutex;

// These are handles for main server and client threads.
pthread_t main_server_thread_handle;
pthread_t main_client_thread_handle;

atomic_bool stop_flag;

void global_init()
{
  // We try to initialize attribute for thread safe printf mutex.
  if (pthread_mutexattr_init(&print_mutexattr) == -1)
  {
    printf("global_init(), pthread_mutexattr_init() has failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // We try to initialize mutex for thread safe printf.
  if (pthread_mutex_init(&print_mutex, &print_mutexattr) == -1)
  {
    printf("global_init(), pthread_mutex_init() has failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // We initialize stop flag.
  atomic_init(&stop_flag, false);
}

void thread_safe_printf(const char*const str, ...)
{
  va_list args;
  va_start(args, str);

  if (pthread_mutex_lock(&print_mutex) == -1)
  {
    printf("thread_safe_printf(), pthread_mutex_lock() has failed: %s\n", strerror(errno));
    abort();
  }

  vprintf(str, args);
  
  if (pthread_mutex_unlock(&print_mutex) == -1)
  {
    printf("thread_safe_printf(), pthread_mutex_unlock() has failed: %s\n", strerror(errno));
    abort();
  }

  va_end(args);
}

void run_server_mode();
void run_client_mode();

void* main_server_thread(void* param)
{
  thread_safe_printf("main_server_thread() begins.\n");

  thread_safe_printf("main_server_thread() ends.\n");

  return NULL;
}

void* side_server_thread(void* param)
{
  thread_safe_printf("side_server_thread() begins.\n");
  
  // We do our work until stop flag is not true.
  while(atomic_load(&stop_flag) != true)
  {
    // ...
  }

  thread_safe_printf("side_server_thread() ends.\n");
  
  return NULL;
}

void* main_client_thread(void* param)
{
  thread_safe_printf("main_client_thread() begins.\n");
  
  // We do our work until stop flag is not true.
  while (atomic_load(&stop_flag) != true)
  {
    // ...
  }
  
  thread_safe_printf("main_client_thread() ends.\n");
  
  return NULL;
}

void* size_client_thread(void* param)
{
  thread_safe_printf("side_client_thread() begins.\n");
  
  thread_safe_printf("side_client_thread() ends.\n");
  
  return NULL;
}

int main(int argc, char** argv)
{
  printf("main() begins.\n");
  
  global_init();
  
  // We try to run main server thread.
  {
    const int result = pthread_create(&main_server_thread_handle, NULL, main_server_thread, NULL);
    if (result != 0)
    {
      thread_safe_printf("main(), pthread_create() has failed for main_server_thread_handle: %i\n", result);
      abort();
    }
  }
  
  // We try to run main client thread.
  {
    const int result = pthread_create(&main_client_thread_handle, NULL, main_client_thread, NULL);
    if (result != 0)
    {
      thread_safe_printf("main(), pthread_create() has failed for main_client_thread_handle: %i\n", result);
      abort();
    }
  }
  
  // We allow main threads to work 8 hours.
  sleep(28800);
  
  // We send a signal to stop all working.
  // ...

  // We try to join main server thread.
  {
    const int result = pthread_join(main_server_thread_handle, NULL);
    if (result != 0)
    {
      thread_safe_printf("main(), pthread_join() has failed for main_server_thread_handle: %i\n", result);
      abort();
    }
  }
  
  {
    const int result = pthread_join(main_client_thread_handle, NULL);
    if (result != 0)
    {
      thread_safe_printf("main(), pthread_joid() has failed for main_client_thread_handle: %i\n", result);
      abort();
    }
  }
  
  thread_safe_printf("main() ends.\n");
  
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
      const int size = recv(client_socket, buffer + total, sizeof(buffer) - total, 0);
    
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

      printf("%i bytes have been recieved.\n", size);

      // Print gotten data.
      for (int i = total; i < total + size; ++i)
      {
        printf("%c", buffer[i]);
      }
      printf("\n");
      
      // We don't really check possible integer overflow.
      total += size;
      if (total > sizeof(buffer))
      {
        printf("Client socket. Size of gotten data is too big.\n");
        close(client_socket);
        break;
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
    buffer[i] = i;
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

    printf("%i bytes have been sent.\n", size);
    total += size;
  }

  close(client_socket);
  printf("Client mode ends.\n");
}
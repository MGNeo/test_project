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
#include <stdbool.h>

// TODO:
// 1. Aplication has two main threads - server and client ones.
// 2. Main server thread accepts clients and passes each of them to personal side server thread for processing.
// 3. Main client thread connects clients and passes each of them to a personal side client thread for processing.
// 4. There are 1000 connections.
// 5. Each connection is sending 1000 bytes at first from client to server and then from server to client again, the data must not be different.
// 6. All sockets use block mode.
// 7. All data is sent by random sized chunks.

void* main_server_thread(void* param);
void* main_client_thread(void* param);
void* side_server_thread(void* param);
void* side_client_thread(void* param);

void thread_safe_printf(const char*const str, ...);

#define PORT 12345
#define CONNECTION_COUNT 100
#define DATA_SIZE 1000

// This is mutex for thread safe printf function.
pthread_mutex_t print_mutex;

// These are handles for main server and client threads.
pthread_t main_server_thread_handle;
pthread_t main_client_thread_handle;

// There are sockets for side server and client threads.
int side_server_sockets[CONNECTION_COUNT];
int side_client_sockets[CONNECTION_COUNT];

// There are handles for side server and client threads 
pthread_t side_server_thread_handles[CONNECTION_COUNT];
pthread_t side_client_thread_handles[CONNECTION_COUNT];

// This is conditional variable for to wake up main client thread after server socket has been bound.
pthread_cond_t server_wake_up_cv = PTHREAD_COND_INITIALIZER;
// This is mutex for working with server_wake_up_cv.
pthread_mutex_t server_wake_up_mutex;
// This is flag for working with server_wake_up_cv.
bool server_wake_up_flag = false;

void printf_mutex_init()
{
  printf("printf_mutex_init() begins.\n");

  // We try to initialize mutex for thread safe printf.
  if (pthread_mutex_init(&print_mutex, NULL) == -1)
  {
    printf("printf_mutex_init(), pthread_mutex_init() has failed: %s\n", strerror(errno));
    abort();
  }

  printf("printf_mutex_init() ends.\n");
}

void server_wake_up_mutex_init()
{
  thread_safe_printf("server_wake_up_mutex_init() begins.\n");
  
  // We try to initialize mutex for server wake up condition variable.
  if (pthread_mutex_init(&server_wake_up_mutex, NULL) == -1)
  {
    thread_safe_printf("server_wake_up_mutex_init(), pthread_mutex_init() has failed: %s\n", strerror(errno));
    abort();
  }
  
  thread_safe_printf("server_wake_up_mutex_init() ends.\n");
}

void server_wake_up_mutex_fin()
{
  thread_safe_printf("server_wake_up_mutex_fin() begins.\n");
  
  // We try to destroy mutex for server wake up condition variable.
  if (pthread_mutex_destroy(&server_wake_up_mutex) == -1)
  {
    thread_safe_printf("server_wake_up_mutex_fin() has failed: %s\n", strerror(errno));
    abort();
  }
  
  thread_safe_printf("server_wake_up_mutex_fin() ends.\n");
}

void printf_mutex_fin()
{
  printf("printf_mutex_fin() begins.\n");

  // We try to destroy mutex for thread safe printf.
  if (pthread_mutex_destroy(&print_mutex) == -1)
  {
    printf("printf_mutex_fin(), pthread_mutex_destroy() has failed: %s\n", strerror(errno));
    abort();
  }

  printf("printf_mutex_fin() ends.\n");
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

  // We try to create server socket.
  const int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1)
  {
    thread_safe_printf("main_server_thread(), server socket can't be created: %s\n", strerror(errno));
    abort();
  }
  printf("Server socket has been created.\n");
  
  // We try to set special socket options.
  {
    const int option = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1)
    {
      thread_safe_printf("main_server_thread(), server socket can't reuse address: %s\n", strerror(errno));
      abort();
    }
    thread_safe_printf("main_server_thread(), server socket has reused address.\n");
  }

  // We try to bind server socket to address.
  {
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) != 0)
    {
      thread_safe_printf("main_server_thread(), server socket can't be bound: %s\n", strerror(errno));
      abort();
    }
    thread_safe_printf("main_server_thread(), server socket has been bound.\n");
  }
  
  // We try to listen server socket.
  if (listen(server_socket, CONNECTION_COUNT) != 0)
  {
    thread_safe_printf("main_server_thread(), server socket can't be listened: %s\n", strerror(errno));
    abort();
  }
  thread_safe_printf("main_server_thread(), server socket has been listened.\n");

  // We notify  main client thread that server socket is ready.
  {
    // We try to lock the mutex.
    {
      const int result = pthread_mutex_lock(&server_wake_up_mutex);
      if (result != 0)
      {
        thread_safe_printf("main_server_thread(), pthread_mutex_lock() has failed: %i\n", result);
        abort();
      }
    }
    
    // We set the flag.
    server_wake_up_flag = true;
    
    // We send the signal.
    {
      const int result = pthread_cond_signal(&server_wake_up_cv);
      if (result != 0)
      {
        thread_safe_printf("main_server_thread(), pthread_cond_signal() has failed: %i\n", result);
        abort();
      }
    }

    // We try to unlock the mutex.
    {
      const int result = pthread_mutex_unlock(&server_wake_up_mutex);
      if (result != 0)
      {
        thread_safe_printf("main_server_thread(), pthread_mutex_unlock() has failed: %i\n", result);
        abort();
      }
    }
  }

  // We accept all clients.
  for (int i = 0; i < CONNECTION_COUNT; ++i)
  {
    const int client_socket = accept(server_socket, NULL, 0);
    if (client_socket == -1)
    {
      thread_safe_printf("main_server_thread(), client socket №%i can't be accepted: %s\n", i, strerror(errno));
      abort();
    }
    thread_safe_printf("main_server_thread(), client socket №%i has been accepted.\n", i);
    
    // We try to start side server thread.
    {
      side_server_sockets[i] = client_socket;
      const int result = pthread_create(&side_server_thread_handles[i], NULL, side_server_thread, &(side_server_sockets[i]));
      if (result != 0)
      {
        thread_safe_printf("main_server_thread(), pthread_create() has failed for side_server_thread_handles[%i]: %i\n", i, result);
        abort();
      }
      thread_safe_printf("main_server_thread(), side server thread №%i has been started.\n.", i);
    }
  }
  
  thread_safe_printf("main_server_thread() ends.\n");

  return NULL;
}

void* side_server_thread(void* param)
{
  thread_safe_printf("side_server_thread() begins.\n");
  
  if (param == NULL)
  {
    thread_safe_printf("side_server_thread(), param == NULL.\n");
    abort();
  }

  // We convert param to socket.
  const int client_socket = *((int*)param);

  // We receive all data from client.
  {
    char buffer[DATA_SIZE];
    int total_received_size = 0;
    
    while (total_received_size < DATA_SIZE)
    {
      const int size = recv(client_socket, buffer + total_received_size, DATA_SIZE - total_received_size, 0);
      
      if (size == -1)
      {
        thread_safe_printf("side_server_thread(), recv() has failed: %s\n", strerror(errno));
        abort();
      }

      if (size == 0)
      {
        thread_safe_printf("side_server_thread(), connection has been closed.\n");
        abort();
      }
      
      thread_safe_printf("side_server_thread(), %i bytes have been received.\n", size);
      
      // We don't check possible integer overflow.
      total_received_size += size;
    }
  }
  
  // We send all data back to the client.
  {
    char buffer[DATA_SIZE];
    int total_sent_size = 0;
    
    while (total_sent_size < DATA_SIZE)
    {
      // We choose random size for sending and check if the size is not bigger than the remains.
      int random_size = rand() % DATA_SIZE + 1;
      if (total_sent_size + random_size > DATA_SIZE)
      {
        random_size = DATA_SIZE - total_sent_size;
      }
      
      const int size = send(client_socket, buffer + total_sent_size, random_size, 0);
      
      if (size == -1)
      {
        thread_safe_printf("side_server_thread(), send() has failed: %s\n", strerror(errno));
        abort();
      }

      if (size == 0)
      {
        thread_safe_printf("side_server_thread(), connection has been closed: %s\n", strerror(errno));
        abort();
      }
      
      thread_safe_printf("side_server_thread(), %i bytes have been sent.\n", size);
      
      total_sent_size += size;
    }
  }
  
  thread_safe_printf("side_server_thread() ends.\n");
  
  return NULL;
}

void* main_client_thread(void* param)
{
  thread_safe_printf("main_client_thread() begins.\n");

  // We wait until the server socket notice us through server wake up condition variable.
  {
    bool loop = true;
    while (loop)
    {
      // We try to lock the mutex.
      {
        const int result = pthread_mutex_lock(&server_wake_up_mutex);
        if (result != 0)
        {
          thread_safe_printf("main_client_thread(), pthread_mutex_lock() has failed: %i\n", result);
          abort();
        }
      }
      
      // We wait the signal and flag.
      {
        struct timespec delay;
        delay.tv_sec = 1;
        delay.tv_nsec = 0;

        {
          const int result = pthread_cond_timedwait(&server_wake_up_cv, &server_wake_up_mutex, &delay);
          if ((result == 0) || (result == ETIMEDOUT))
          {
            // We check the flag.
            if (server_wake_up_flag == true)
            {   
              loop = false;
            }
          } else {
             thread_safe_printf("main_client_thread(), pthread_cond_wait() has failed: %i\n", result);
             abort();
          }
        }

        // We try to unlock the mutex.
        {
          const int result = pthread_mutex_unlock(&server_wake_up_mutex);
          if (result != 0)
          {
            thread_safe_printf("main_client_thread(), pthread_mutex_unlock() has failed: %i\n", result);
            abort();
          }
        }
      }
    }
  }

  for (int i = 0; i < CONNECTION_COUNT; ++i)
  {
    // We try to create client socket.
    side_client_sockets[i] = socket(AF_INET, SOCK_STREAM, 0);
    if (side_client_sockets[i] == -1)
    {
      thread_safe_printf("main_client_thread(), client socket №%i can't be created: %s\n", i, strerror(errno));
      abort();
    }
    thread_safe_printf("main_client_thread(), client socket №%i has been created.\n", i);

    // We try to set special socket options.
    const int option = 1;
    if (setsockopt(side_client_sockets[i], SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1)
    {
      thread_safe_printf("main_client_thread(), client socket №%i can't reuse address: %s\n", i, strerror(errno));
      abort();
    }
    thread_safe_printf("main_client_thread(), client socket №%i has reused address.\n", i);

    // We try to connect client socket to address.
    struct sockaddr_in client_address;
    client_address.sin_family = AF_INET;
    client_address.sin_port = htons(PORT);
    // We don't really check result of this call.
    client_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(side_client_sockets[i], (struct sockaddr*)&client_address, sizeof(client_address)) == -1)
    {
      thread_safe_printf("main_client_thread(), client socket №%i can't be connected: %s\n", i, strerror(errno));
      abort();
    }
    thread_safe_printf("main_client_thread(), client socket №%i has been connected.\n", i);
  
    // We try to start side client thread.
    {
      const int result = pthread_create(&side_client_thread_handles[i], NULL, side_client_thread, &(side_client_sockets[i]));
      if (result != 0)
      {
        thread_safe_printf("main_client_thread(), pthread_create() has failed for side_client_thread_handle[%i].\n, i");
        abort();
      }
      thread_safe_printf("main_client_thread(), side client thread №%i has been started.\n", i);
    }
  }

  thread_safe_printf("main_client_thread() ends.\n");
  
  return NULL;
}

void* side_client_thread(void* param)
{
  thread_safe_printf("side_client_thread() begins.\n");
  
  if (param == NULL)
  {
    thread_safe_printf("side_client_thread(), param == NULL.\n");
    abort();
  }
  
  // We convert param to socket.
  const int client_socket = *((int*)param);

  // We send data to the server..
  {
    char buffer[DATA_SIZE];
    int total_sent_size = 0;
    
    while (total_sent_size < DATA_SIZE)
    {
      // We choose random size for sending and check if the size is not bigger than the remains.
      int random_size = rand() % DATA_SIZE + 1;
      if (total_sent_size + random_size > DATA_SIZE)
      {
        random_size = DATA_SIZE - total_sent_size;
      }

      const int size = send(client_socket, buffer + total_sent_size, random_size, 0);

      if (size == -1)
      {
        thread_safe_printf("side_client_thread(), send() has failed: %s\n", strerror(errno));
        abort();
      }

      if (size == 0)
      {
        thread_safe_printf("side_client_thread(), connection has been closed: %s\n", strerror(errno));
        abort();
      }

      thread_safe_printf("side_client_thread(), %i bytes has been sent.\n", size);

      total_sent_size += size;
    }
  }
  
  thread_safe_printf("side_client_thread() ends.\n");
  
  return NULL;
}

int main(int argc, char** argv)
{
  printf("main() begins.\n");
  
  printf_mutex_init();
  
  server_wake_up_mutex_init();
  
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
  
  // We try to join main server thread for waiting its end.
  {
    const int result = pthread_join(main_server_thread_handle, NULL);
    if (result != 0)
    {
      thread_safe_printf("main(), pthread_join() has failed for main_server_thread_handle: %i\n", result);
      abort();
    }
  }
  
  // We try to join main client thread for waiting its end.
  {
    const int result = pthread_join(main_client_thread_handle, NULL);
    if (result != 0)
    {
      thread_safe_printf("main(), pthread_join() has failed for main_client_thread_handle: %i\n", result);
      abort();
    }
  }

  // We try to join all side server threads for waiting them end.
  {
    for (size_t i = 0; i < CONNECTION_COUNT; ++i)
    {
      const int result = pthread_join(side_server_thread_handles[i], NULL);
      if (result != 0)
      {
        thread_safe_printf("main(), pthread_join() has failed for side_server_thread_handles[i]: %i\n", result);
        abort();
      }
    }
  }

  // We try to join all side client threads for waiting them end;
  {
    for (size_t i = 0; i < CONNECTION_COUNT; ++i)
    {
      const int result = pthread_join(side_client_thread_handles[i], NULL);
      if (result != 0)
      {
        thread_safe_printf("main(), pthread_join() has failed for side_client_thread_handles[i]: %i\n", result);
        abort();
      }
    }
  }
  
  server_wake_up_mutex_fin();
  
  printf_mutex_fin();

  printf("main() ends.\n");

  return 0;
}
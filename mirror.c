#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

#define PORT 7001
#define _XOPEN_SOURCE 500
#define BUFSIZE 1024

int main(int argc, char const *argv[])
{
  int srv_fd, new_skt;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);

  // Create socket file descriptor
  if ((srv_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Attach socket to the port 8080
  if (setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
  {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(srv_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(srv_fd, 3) < 0)
  {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  while (1)
  {
    if ((new_skt = accept(srv_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
      perror("accept");
      exit(EXIT_FAILURE);
    }

    printf("The New client is connected. Forking child process...\n");

    pid_t pid = fork();
    if (pid == 0)
    {
      // Child process
      close(srv_fd);
      processclient(new_skt);
    }
    else if (pid == -1)
    {
      // error
      perror("fork");
      exit(EXIT_FAILURE);
    }
    else
    {
      // Parent process
      close(new_skt);
      while (waitpid(-1, NULL, WNOHANG) > 0)
        ; // Clean up zombie processes
    }
  }

  exit(EXIT_SUCCESS);
}

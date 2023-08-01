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

#define BUFSIZE 1024
#define _XOPEN_SOURCE 500
#define PORT 8080
#define MIRROR_PORT 7001

void redirect_to_mirror(int cli_fd)
{
  char redirect_msg[BUFSIZE];
  snprintf(redirect_msg, BUFSIZE, "%d\n", MIRROR_PORT);
  send(cli_fd, redirect_msg, strlen(redirect_msg), 0);
  close(cli_fd);
}

void processclient(int skt_fd)
{
  char buff[BUFSIZE] = {0};
  char command[BUFSIZE] = {0};
  char gf[BUFSIZE] = {0};

  while (1)
  {

    memset(buff, 0, sizeof(buff));
    memset(command, 0, sizeof(command));
    memcpy(gf, buff, sizeof(buff));

    int valread = read(skt_fd, buff, sizeof(buff));
    buff[valread] = '\0';

    // Parse command
    char *token = strtok(buff, " ");
    // handle commands
    if (token == NULL)
      sprintf(command, "The syntax is Invalid. Please try again.\n");
    else if (strcmp(token, "fgets") == 0)
    {
      // execcution of fgets command
      char *filename = strtok(NULL, " ");
      if (filename == NULL)
        sprintf(command, "The syntax is Invalid. Please try again.\n");
      else
      {
        // fgets logic
      }
    }
    else if (strcmp(buff, "quit") == 0)
    {
      sprintf(command, "Goodbye.\n");
      break;
    }
    else
      sprintf(command, "The syntax is invalid. Please try again.\n");

    // Send response to client
    send(skt_fd, command, strlen(command), 0);
  }

  close(skt_fd);
  exit(EXIT_SUCCESS);
}

int main(int argc, char const *argv[])
{
  int serv_fd, new_skt;
  struct sockaddr_in serv_addr, cli_addr;
  int opt = 1;
  int addrlen = sizeof(serv_addr);
  int active_clients = 0;

  // Create socket file descriptor
  if ((serv_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Attach socket to the port 8080
  if (setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
  {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(PORT);

  // Bind socket to the PORT
  if (bind(serv_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(serv_fd, 3) < 0)
  {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  printf("Waiting for client...\n");
  while (1)
  {
    if ((new_skt = accept(serv_fd, (struct sockaddr *)&serv_addr, (socklen_t *)&addrlen)) < 0)
    {
      perror("accept");
      exit(EXIT_FAILURE);
    }

    printf("New client is connected. Forking child process...\n");
    // load balancing from server to mirror
    if (active_clients <= 6 || (active_clients <= 12 && active_clients % 2 == 1))
    {
      pid_t pid = fork();
      if (pid == 0)
      {
        // Child process
        close(serv_fd);
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
    else
    {
      // redirecting to mirror server
      printf("Redirecting to mirror\n");
      redirect_to_mirror(new_skt);
    }
    // counter for no of connections
    active_clients++;
  }

  exit(EXIT_SUCCESS);
}

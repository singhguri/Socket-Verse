#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define mirror_port 7001
#define BUFSIZE 1024

int main(int argc, char const *argv[])
{

  int skt_fd;
  struct sockaddr_in serv_addr, mirror_addr;

  char buff[BUFSIZE] = {0};
  char command[BUFSIZE] = {0};
  char gf[BUFSIZE * 2] = {0};
  int valid_syntax;

  if ((skt_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Socket creation error \n");
    return -1;
  }

  memset(&serv_addr, '0', sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
  {
    printf("\n the address is invalid / Address not supported \n");
    return -1;
  }

  if (connect(skt_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("\nConnection has Failed \n");
    return -1;
  }

  printf("Connected to server.\n");
  printf("Enter a command (or 'quit' to exit):\n");

  while (1)
  {
    memset(buff, 0, sizeof(buff));
    memset(command, 0, sizeof(command));
    fgets(buff, sizeof(buff), stdin);
    memcpy(gf, buff, sizeof(buff));

    // Remove newline character
    buff[strcspn(buff, "\n")] = 0;

    sprintf(command, "%s", buff);

    // Send command to server
    send(skt_fd, command, strlen(command), 0);

    // Handle response from server
    // handle_response(skt_fd);
    char resp[BUFSIZE] = {0};
    // redirection to mirror
    int valueread = read(skt_fd, resp, sizeof(resp));
    resp[strcspn(resp, "\n")] = '\0';

    if (strcmp(resp, "7001") == 0)
    {
      // closing the current server connection
      close(skt_fd);
      printf("In here\n");

      // Create new socket for the mirror server
      skt_fd = socket(AF_INET, SOCK_STREAM, 0);
      if (skt_fd == -1)
      {
        perror("socket");
        exit(EXIT_FAILURE);
      }

      memset(&mirror_addr, '\0', sizeof(mirror_addr));
      mirror_addr.sin_family = AF_INET;
      mirror_addr.sin_port = htons(mirror_port);
      mirror_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

      // Connect to the mirror server
      if (connect(skt_fd, (struct sockaddr *)&mirror_addr, sizeof(mirror_addr)) == -1)
      {
        perror("connect");
        exit(EXIT_FAILURE);
      }
    }
    else
      printf("%s\n", resp);
  }

  close(skt_fd);
  printf("Connection is closed.\n");

  exit(EXIT_SUCCESS);
}

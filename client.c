#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>

#define PORT 8080
#define mirror_port 7001
#define BUFSIZE 1024
#define MAX_LENGTH_OF_COMMAND 10000

bool needToUnzip = false;

// this function checks if the parameter is integer and is greater than 0
long isValidDigitsRange(char *val)
{
  char *helperPtr;
  long number;

  // Use strtol to check if val is valid integer
  number = strtol(val, &helperPtr, 10);

  if (helperPtr == val || number < 0)
  {
    // not a integer grater than 0
    printf("Size has to be positive integer %s\n", val);
    return -1;
  }
  else
  {
    // valid integer
    return number;
  }
}

bool isValidDates(const char *sdate1, const char *sdate2)
{

  // year, month , date
  int date1[3];
  int date2[3];
  int year, month, day;

  // Check if the sdate1 matches the date format "YYYY-MM-DD"
  if (sscanf(sdate1, "%d-%d-%d", &year, &month, &day) == 3)
  {
    // range possibility
    if ((year >= 1000 && year <= 9999) &&
        (month >= 1 && month <= 12) &&
        (day >= 1 && day <= 31))
    {
      date1[0] = year;
      date1[1] = month;
      date1[2] = day;
    }
    else
    {
      printf("Invalid Date format(YYYY-MM-DD) %s\n", sdate1);
      return false;
    }
  }
  else
  {
    printf("Invalid Date format(YYYY-MM-DD) %s\n", sdate2);
    return false;
  }

  // Check if the sdate2 matches the date format "YYYY-MM-DD"
  if (sscanf(sdate2, "%d-%d-%d", &year, &month, &day) == 3)
  {
    // range possibility
    if ((year >= 1000 && year <= 9999) &&
        (month >= 1 && month <= 12) &&
        (day >= 1 && day <= 31))
    {
      date2[0] = year;
      date2[1] = month;
      date2[2] = day;
    }
    else
    {
      printf("Invalid Date format(YYYY-MM-DD) %s\n", sdate2);
      return false;
    }
  }
  else
  {
    printf("Invalid Date format(YYYY-MM-DD) %s\n", sdate2);
    return false;
  }

  // check if date1 <= date2
  // Compare the year
  if (date1[0] < date2[0])
    return true;
  if (date1[0] > date2[0])
  {
    printf("date1 is greater than date2\n");
    return false;
  }

  // Compare the month if year is equal
  if (date1[1] < date2[1])
    return true;
  if (date1[1] > date2[1])
  {
    printf("date1 is greater than date2\n");
    return false;
  }

  // Compare the day
  if (date1[2] <= date2[2])
    return true;

  // date1 is greater than date2
  printf("date1 is greater than date2\n");
  return false;
}

// validate the syntax of commands
bool validateTheCommand(char *command)
{

  // tokenize the command

  char commandWithArgs[10][PATH_MAX];
  int size = 0;

  // used for tokenizing
  char *tempPtr;

  // tokkenize on space using strtok_r
  char *token = strtok_r(command, " ", &tempPtr);

  // loop through all the tokens
  while (token != NULL)
  {
    // copy string to array
    strcpy(commandWithArgs[size++], token);

    // tokenize
    token = strtok_r(NULL, " ", &tempPtr);
  }

  // commandWithArgs[0] contains the name of command

  if (strcmp(commandWithArgs[0], "fgets") == 0)
  {

    // where the command is fgets
    // fgets f1 f2 f3 f4 so max no. of tokens is 5
    if (size < 2 || size > 5)
    {
      printf("Wrong input: Number of files allowed is between 1 to 4\n");
      return false;
    }

    return true;
  }
  else if (strcmp(commandWithArgs[0], "tarfgetz") == 0)
  {

    // where the command is tarfgetz
    // tarfgetz size1 size2 <-u> so max no. of tokens is 4
    if (size < 3)
      return false;
    int n1 = isValidDigitsRange(commandWithArgs[1]);
    int n2 = isValidDigitsRange(commandWithArgs[2]);
    // if it has -u option
    if (strcmp(commandWithArgs[size - 1], "-u") == 0 && size == 4 && n1 != -1 && n2 != -1 && n1 <= n2)
    {
      // this is valid
      needToUnzip = true;
    }
    else if (strcmp(commandWithArgs[size - 1], "-u") != 0 && size == 3 && n1 != -1 && n2 != -1 && n1 <= n2)
    {
      // no -u option
      // this is valid
    }
    else
    {
      printf("Wrong input: Allowed input format is tarfgetz size1 size2 <-u>\n");
      return false;
    }

    return true;
  }
  else if (strcmp(commandWithArgs[0], "filesrch") == 0)
  {
    // where the command is filesrch
    // filesrch filename so max no. of tokens is 2
    if (size != 2)
    {
      printf("Wrong input: Allowed format is  filesrch filename  \n");
      return false;
    }

    return true;
  }
  else if (strcmp(commandWithArgs[0], "targzf") == 0)
  {

    // where the command is targzf
    // targzf <extension list> <-u> up to 4 different file types  max no. of token is 6

    // if it has -u option
    if (strcmp(commandWithArgs[size - 1], "-u") == 0 && size > 3 && size <= 6)
    {
      // this is valid
      needToUnzip = true;
    }
    else if (strcmp(commandWithArgs[size - 1], "-u") != 0 && size > 2 && size < 6)
    {
      // no -u option
      // this is valid
    }
    else
    {
      printf("Wrong input: Allowed input targzf <extension list> <-u>\n");
      return false;
    }

    return true;
  }
  else if (strcmp(commandWithArgs[0], "getdirf") == 0)
  {

    // where the command is getdirf
    // getdirf date1 date2 <-u> max no. of tokens is 4

    // if it has -u option check size of command and check if date1< date2 and are valid dates
    if (strcmp(commandWithArgs[size - 1], "-u") == 0 && size == 4 && isValidDates(commandWithArgs[1], commandWithArgs[2]))
    {
      // this is valid
      needToUnzip = true;
    }
    // if it doesn't have -u option check size of command and check if date1< date2 and are valid dates
    else if (strcmp(commandWithArgs[size - 1], "-u") != 0 && size == 3 && isValidDates(commandWithArgs[1], commandWithArgs[2]))
    {
      // no -u option
      // this is valid
    }
    else
    {
      printf("Wrong input: Allowed input format getdirf date1 date2 <-u>\n");
      return false;
    }

    return true;
  }
  else if (strcmp(commandWithArgs[0], "quit") == 0)
  {
    // quit the client
    return true;
  }
  else
  {
    printf("Sorry, Wrong input.\n");
    return false;
  }

  return false;
}

int main(int argc, char const *argv[])
{

  int skt_fd;
  struct sockaddr_in serv_addr, mirror_addr;

  char buff[BUFSIZE] = {'\0'};
  char command[BUFSIZE] = {'\0'};
  char gf[BUFSIZE * 2] = {'\0'};
  int valid_syntax;

  if ((skt_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Socket creation error \n");
    return -1;
  }

  memset(&serv_addr, '\0', sizeof(serv_addr));

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

  char inputByUser[MAX_LENGTH_OF_COMMAND];
  char copyOfUserInput[MAX_LENGTH_OF_COMMAND];

  while (1)
  {

    needToUnzip = false;

    // getting user input, from standard input
    fgets(inputByUser, MAX_LENGTH_OF_COMMAND, stdin);

    // replaces the \n with NULL character in user input
    inputByUser[strcspn(inputByUser, "\n")] = '\0';

    // creating a copy of inputByUser
    strncpy(copyOfUserInput, inputByUser, MAX_LENGTH_OF_COMMAND);

    // validate the command syntax
    bool commandValidation = validateTheCommand(inputByUser);

    // clear buffer and continue if command validation is false
    if (!commandValidation)
      continue;

    /// else the commandValidation is true good to go

    // check if it has -u at the end of command unzip the results
    // execute unzipping operation if need to unzip is true
    if (needToUnzip)
      printf("unzip it\n");

    // TODO: send the command to server
    // Send input by user to server
    send(skt_fd, copyOfUserInput, strlen(copyOfUserInput), 0);

    // if input is quite, exit the client
    // if (commandValidation && strcmp(inputByUser, "quit") == 0)
    //   exit(EXIT_SUCCESS);

    // TODO: receive the result from server
    // Handle response from server
    // handle_response(skt_fd);
    char resp[BUFSIZE] = {0};

    // redirection to mirror
    int value_read = read(skt_fd, resp, sizeof(resp));
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
    {
      printf("%s\n", resp);
      if (strcmp(resp, "Goodbye") == 0)
        exit(EXIT_SUCCESS);
    }
  }

  close(skt_fd);
  printf("Connection is closed.\n");

  exit(EXIT_SUCCESS);
}

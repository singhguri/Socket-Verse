#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <fcntl.h>

#define PORT 8080
#define mirror_port 7001
#define BUFSIZE 1024
#define RECV_BUFSIZE 1000000
#define MAX_LENGTH_OF_COMMAND 10000

bool needToUnzip = false;
bool returnsTarFile = false;
void receive_file(int file_fd, int socket)
{
  if (file_fd < 0)
  {
    perror("Error creating file\n");
    return;
  }

  char buffer[BUFSIZE];
  int bytesReceived;

  // read from the socket fd
  while ((bytesReceived = read(socket, buffer, BUFSIZE)) > 0)
  {
    // printf("%d received\n", bytesReceived);
    //write to the new tar file
    write(file_fd, buffer, bytesReceived);

    // if theno. of bytes received are less the buff size exit
    // this means there are no more character in socket
    if (bytesReceived < BUFSIZE)
      break;
  }

  printf("done\n");
}

void receive_message(int socket, char *buffer)
{

  ssize_t bytesReceived = read(socket, buffer, BUFSIZE);
  if (bytesReceived > 0)
  {
    printf("Message from server: %s\n", buffer);
  }
}

void receive_control_message(int socket, char *buffer)
{
  // FIL, MIR, ERR
  ssize_t bytesReceived = read(socket, buffer, 3);
  if (bytesReceived > 0)
  {
    printf("Control from server: %s\n", buffer);
  }
}


// unzip the file
void unzipFile(char * fileName){
  char command_buf[BUFSIZE];
  int status;

  // unzip file using tar command -x to unzip
  sprintf(command_buf, "tar -xzf \"%s\" 2>/dev/null",
					fileName);

	// call system command to execute the the command
	status = system(command_buf);

	if(status!=0){
    printf("Some error Occured while unzipping the file..\n");
  }else{
    printf("File Unzipped Successfully.\n");
  }
}

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

// for getdirf command check if the dates are valid
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
  // printf("time to validate\n");
  // check if date1 <= date2
  // Compare the year
  if (date1[0] < date2[0])
    return true;
  else if (date1[0] > date2[0])
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
    returnsTarFile = true;
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
    returnsTarFile = true;
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
    if (strcmp(commandWithArgs[size - 1], "-u") == 0 && size > 2 && size <= 6)
    {
      // this is valid
      needToUnzip = true;
    }
    else if (strcmp(commandWithArgs[size - 1], "-u") != 0 && size >= 2 && size < 6)
    {
      // no -u option
      // this is valid
    }
    else
    {
      printf("Wrong input: Allowed input targzf <extension list> <-u>\n");
      return false;
    }

    returnsTarFile = true;
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

    returnsTarFile = true;
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

  // create a socket and handle error
  if ((skt_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Socket creation error \n");
    return -1;
  }

  // set server address to NULL
  memset(&serv_addr, '\0', sizeof(serv_addr));

  // set port number and other parameters
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
  {
    printf("\n the address is invalid / Address not supported \n");
    return -1;
  }

  // create a connection to server from client side
  if (connect(skt_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("\nConnection has Failed \n");
    return -1;
  }

    //clear the buffer
    char buffer[BUFSIZE] = {0};
    char resp[BUFSIZE] = {0};
    // check the control message
    receive_control_message(skt_fd, buffer);

    // if the control message is MIR redirect client to mirror
    if (strcmp(buffer, "MIR") == 0)
    {
      // closing the current server connection
      close(skt_fd);

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

      // check the control message exepectting CTM now
      receive_control_message(skt_fd, buffer);
    }
    
    // if control message iis CTM
    if(strcmp(buffer, "CTM") == 0){
      printf("Connected to Mirror.\n");
    }
    // if control message is CTS
    else if(strcmp(buffer, "CTS") == 0){
      printf("Connected to server.\n");
    }

  char inputByUser[MAX_LENGTH_OF_COMMAND];
  char copyOfUserInput[MAX_LENGTH_OF_COMMAND];

  while (1)
  {

    printf("Enter a command (or 'quit' to exit):\n");

    //flags
    needToUnzip = false;
    returnsTarFile = false;

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
   
    // TODO: change send command to write command
    // Send input by user to server
    send(skt_fd, copyOfUserInput, strlen(copyOfUserInput), 0);

  
    // check the control message
    receive_control_message(skt_fd, buffer);
    
    
    // if thte control message is FIL the server will be sending a file
    if (strcmp(buffer, "FIL") == 0)
    {
      
      int file_fd;

      // check if the command requires tar file as response
      if (returnsTarFile)
      {
        // create a new temp.tar.gz
        file_fd = open("temp.tar.gz", O_WRONLY | O_CREAT | O_TRUNC, 0777);

        printf("Receiving ....");

        //receive file from server
        receive_file(file_fd, skt_fd);

        printf("File received\n");

        //close file desc
        close(file_fd);

        // if there is -u option
        // unzip the file
        if (needToUnzip){
          printf("Unzipping file...\n");
          //call the function
          unzipFile("temp.tar.gz");
        }
      }else{
        //handle error
        printf("Some error occured, Server is trying to send a file but this command doesnot accepts file as a response.\n");
      }
    }
    // if there is a control message of ERR
    else if (strcmp(buffer, "ERR") == 0)
    {
      // recive the message from server in resp
      receive_message(skt_fd, resp);
    }

    // if the server is returning message in case of  FILESRCH
    else if(strcmp(buffer,"MSG")==0)
    { 
      //recceive message
      receive_message(skt_fd, resp);

    }
    
    // if there is quit signal dfrom server
    else if(strcmp(buffer,"QIT")==0){
      //close socket fd
      close(skt_fd);
      printf("Connection is closed.\n");
      // exit this process.
      exit(EXIT_SUCCESS);    
    }
  }

  // just for  precaution purpose might not get called.
  close(skt_fd);
  printf("Connection is closed.\n");

  exit(EXIT_SUCCESS);
}

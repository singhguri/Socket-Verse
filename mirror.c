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

struct
{
  char *file_name;
  int file_size;
  char *file_created_date;
} File_info;

int day, year, hour, min, sec;
char month[4];

void file_search(char *base_path, char *filename)
{
  char command_buf[BUFSIZE];

  // Searching for file in the root directory
  sprintf(command_buf, "find %s -type f -wholename $(find %s -type f -name %s | awk -F/ '{ print NF-1, $0 }' | sort -n | awk '{$1=\"\"; print $0}'|head -1) -printf \"%%s,%%Tc\n\"", base_path, base_path, filename);

  FILE *fp = popen(command_buf, "r");
  char line[BUFSIZE];

  // if File found in root directory
  if (fgets(line, BUFSIZE, fp) != NULL)
  {
    File_info.file_name = strdup(filename);
    File_info.file_size = atoi(strtok(line, ","));

    char *date = strtok(NULL, ",");
    if (sscanf(date, "%*s %s %d %d:%d:%d %d", month, &day, &hour, &min, &sec, &year) == 6)
      sprintf(date, "%s %d, %d %d:%d:%d\n", month, day, year, hour, min, sec);
    File_info.file_created_date = strdup(date);
  }
  // if File not found
  else
    File_info.file_size = -1;

  pclose(fp);
}

int get_files(char *base_path, char *file1, char *file2, char *file3, char *file4)
{
  int status = 0;
  char command_buf[BUFSIZE];

  sprintf(command_buf, "find %s -type f \\( -iname \"%s\" -o -iname \"%s\" -o -iname \"%s\" -o -iname \"%s\" \\) -print0 | xargs -0 tar -czf temp.tar.gz 2>/dev/null",
          base_path, file1, file2, file3, file4);

  status = system(command_buf);

  return status;
}

bool get_files_matching_size(char *base_path, int size1, int size2)
{
  char command_buf[BUFSIZE];
  sprintf(command_buf, "find %s -type f -size +%dk -size -%dk -print0 | xargs -0 tar -czf temp.tar.gz",
          base_path, size1, size2);
  int status = system(command_buf);

  if (status == 0)
    return true;

  return false;
}

bool get_files_matching_date(char *base_path, char *date1, char *date2)
{
  char command_buf[BUFSIZE];
  sprintf(command_buf, "find %s -type f -newermt \"%s\" ! -newermt \"%s\" -print0 | xargs -0 tar -czf temp.tar.gz",
          base_path, date1_str, date2_str);
  int status = system(command_buf);

  if (status == 0)
    return true;

  return false;
}

bool get_files_matching_ext(char *base_path, char *ext1, char *ext2, char *ext3, char *ext4)
{
  char command_buf[BUFSIZE];
  sprintf(command_buf, "find %s -type f \\( ", base_path);
  if (ext1 != NULL)
    sprintf(command_buf + strlen(command_buf), "-iname \"*.%s\" -o ", ext1);
  if (ext2 != NULL)
    sprintf(command_buf + strlen(command_buf), "-iname \"*.%s\" -o ", ext2);
  if (ext3 != NULL)
    sprintf(command_buf + strlen(command_buf), "-iname \"*.%s\" -o ", ext3);
  if (ext4 != NULL)
    sprintf(command_buf + strlen(command_buf), "-iname \"*.%s\" -o ", ext4);

  sprintf(command_buf + strlen(command_buf), "-false \\) -print0 | xargs -0 tar -czf temp.tar.gz");

  int status = system(command_buf);

  if (status == 0)
    return true;

  return false;
}

void processclient(int skt_fd)
{
  char cmd[BUFSIZE] = {0};
  char response[BUFSIZE * 2] = {0};
  char gf[BUFSIZE] = {0};

  while (1)
  {
    if (response != NULL && strcmp(response, "Goodbye") == 0)
      break;

    memset(cmd, 0, sizeof(cmd));
    memset(response, 0, sizeof(response));
    memcpy(gf, cmd, sizeof(cmd));

    int val_read = read(skt_fd, cmd, sizeof(cmd));
    cmd[val_read] = '\0';

    // Parse command
    char *token = strtok(cmd, " ");
    // handle commands
    if (token == NULL)
      sprintf(response, "The syntax is Invalid. Please try again.\n");
    else if (strcmp(token, "fgets") == 0)
    {
      // execcution of fgets command
      char *file1 = strtok(NULL, " ");
      if (file1 == NULL)
        sprintf(response, "The syntax is Invalid. Please try again.\n");
      else
      {
        // fgets logic
        char *file2 = strtok(NULL, " ");
        char *file3 = strtok(NULL, " ");
        char *file4 = strtok(NULL, " ");

        int status = get_files("$HOME", file1, file2, file3, file4);

        // check if the files were found
        if (status == 0)
        {
          // return temp.tar.gz file
          sprintf(response, "tar file");
        }
        else
          sprintf(response, "No file found.\n");
      }
    }
    else if (strcmp(token, "filesrch") == 0)
    {
      char *filename = strtok(NULL, " ");
      if (filename == NULL)
        sprintf(response, "The syntax is Invalid. Please try again.\n");
      else
      {
        // fsearch logic
        File_info.file_size = 0;
        // serach file by file name in $HOME directory
        file_search("$HOME", filename, NULL, 0);

        sprintf(response, "Name: %s\t\tSize: %d bytes\t\tCreated date: %s", File_info.file_name, File_info.file_size, File_info.file_created_date);

        if (File_info.file_size == -1)
          sprintf(response, "File not found.\n");
        else if (File_info.file_size == 0)
          sprintf(response, "Some error occured.\n");
      }
    }
    else if (strcmp(token, "tarfgetz") == 0)
    {
      char *size1_str = strtok(NULL, " ");
      char *size2_str = strtok(NULL, " ");

      if (size1_str == NULL || size2_str == NULL)
        sprintf(response, "The syntax is Invalid. Please try again.\n");
      else
      {
        int size1 = atoi(size1_str);
        int size2 = atoi(size2_str);
        if (size1 < 0 || size2 < 0 || size1 > size2)
          sprintf(response, "The size range is invalid. Please try again.\n");
        else
        {
          // get files matching the size range
          bool status = get_files_matching_size("$HOME", size1, size2);
          if (status)
            sprintf(response, "tar file");
          else
            sprintf(response, "No files found with given size range.\n");

          // if unzip_flag is present, then unzip the files on client side
        }
      }
    }
    else if (strcmp(token, "getdirf") == 0)
    {
      char *date1_str = strtok(NULL, " ");
      char *date2_str = strtok(NULL, " ");

      if (date1_str == NULL || date2_str == NULL)
        sprintf(response, "Invalid syntax. Please try again.\n");
      else
      {
        // get files matching date range
        bool status = get_files_matching_date("$HOME", date1_str, date2_str);
        if (status)
          sprintf(response, "tar file");
        else
          sprintf(response, "No files found with given date range.\n");
      }
    }
    else if (strcmp(token, "targzf") == 0)
    {
      char *ext1 = strtok(NULL, " ");
      char *ext2 = strtok(NULL, " ");
      char *ext3 = strtok(NULL, " ");
      char *ext4 = strtok(NULL, " ");

      // check if any of the specified files are present
      bool status = get_files_matching_ext("$HOME", ext1, ext2, ext3, ext4);
      if (status)
        sprintf(response, "tar file");
      else
        sprintf(response, "No file found.\n");
    }
    else if (strcmp(token, "quit") == 0)
    {
      printf("client %d exited...\n", getpid());
      sprintf(response, "Goodbye\n");
    }
    else
      sprintf(response, "The syntax is invalid. Please try again.\n");

    // send response to client
    write(skt_fd, response, strlen(response));
    // send(skt_fd, response, strlen(response), 0);
  }

  close(skt_fd);
  exit(EXIT_SUCCESS);
}

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

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
#include <fcntl.h>

#define BUFSIZE 1024
#define _XOPEN_SOURCE 500
#define PORT 8080
#define MIRROR_PORT 7001

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

	// Searching for file in the root directory
	char command_buf[BUFSIZE];
	sprintf(command_buf, "find %s -maxdepth 1 -name %s -printf \"%%s,%%Tc\n\"", base_path, filename);
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
	else
	{
		// Search for the file in all subdirectories
		sprintf(command_buf, "find %s -name %s -printf \"%%s,%%Tc\n\"", base_path, filename);
		fp = popen(command_buf, "r");

		// if File found in subdirectory
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
	}
	pclose(fp);
}

// redirect to the mirror
void redirect_to_mirror(int cli_fd)
{
	char redirect_msg[BUFSIZE];
	snprintf(redirect_msg, BUFSIZE, "%d\n", MIRROR_PORT);
	write(cli_fd, redirect_msg, strlen(redirect_msg));
	// send(cli_fd, redirect_msg, strlen(redirect_msg), 0);
	close(cli_fd);
}

void processclient(int skt_fd)
{
	char cmd[BUFSIZE] = {0};
	char response[BUFSIZE * 2] = {0};
	char gf[BUFSIZE] = {0};

	printf("New client connected. Client id: %d...\n", getpid());

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
			char *filename = strtok(NULL, " ");
			if (filename == NULL)
				sprintf(response, "The syntax is Invalid. Please try again.\n");
			else
			{
				// fgets logic
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
				file_search("$HOME", filename);

				sprintf(response, "Name: %s\t\tSize: %d bytes\t\tCreated date: %s", File_info.file_name, File_info.file_size, File_info.file_created_date);

				if (File_info.file_size == -1)
					sprintf(response, "File not found.\n");
			}
		}
		else if (strcmp(token, "quit") == 0)
		{
			printf("client %d exited...\n", getpid());
			sprintf(response, "Goodbye\n");
		}
		else
			sprintf(response, "The syntax is invalid. Please try again.\n");

		// Send response to client
		write(skt_fd, response, strlen(response));
		// send(skt_fd, response, strlen(response), 0);
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
	int no_of_clients = 1;

	// Create Raw socket
	// Create socket file descriptor
	if ((serv_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	////////////////////// TODO: check this ////////////////////////

	// Attach socket to the port 8080
	if (setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	/// Attributes for binding socket with IP and PORT
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

	// listen to the socket
	// queue of size 150
	if (listen(serv_fd, 150) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	// waitin for client
	printf("Waiting for client...\n");
	while (1)
	{
		if ((new_skt = accept(serv_fd, (struct sockaddr *)&serv_addr, (socklen_t *)&addrlen)) < 0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}

		// load balancing from server to mirror
		// if active clients less than =6 or is an odd no. after 12 connections
		// to be handled by server
		if (no_of_clients <= 6 || (no_of_clients > 12 && no_of_clients % 2 == 1))
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

		// increase counter for no of connections
		no_of_clients++;
	}

	exit(EXIT_SUCCESS);
}

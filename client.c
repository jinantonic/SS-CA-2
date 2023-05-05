/*  Client program to connect to the server socket program  */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define LENGTH 512 // Buffer length

int main(int argc, char *argv[]) {
	if (argc != 3) { // Check how many arguments were given
		printf("Error! Invalid amount of arguments. Usage: ./client <file-name> <destination>\n"); // Print error message
		exit(EXIT_FAILURE); // Exit program
	} // End if statement

	int SID, length = 0; // Socket ID, length 
	struct sockaddr_in server; // Server struct
	char server_message[500]; // Server message buffer
	char *filename = argv[1]; // File name
	char *dir = argv[2]; // Directory
	
	uid_t uid, c_uid; // User id, converted user id

    	memset(server_message,'\0',sizeof(server_message)); // Clear server message buffer
	printf("File name passed from argument: %s\n", filename); // Print file name

	if (!(strcmp(dir, "manufacturing") == 0 || strcmp(dir, "distribution") == 0)) { // Check if directory is valid
		printf("Error! Destination input %s invalid, input must be either \"manufacturing\" or \"distribution\".\n", dir); // Print error message
		exit(EXIT_FAILURE);
	} // end if

	SID = socket(AF_INET, SOCK_STREAM, 0); // Create socket

	if (SID == -1) { // If socket was not created
        	printf("Error! Socket creation failed.\n"); // Print error message
    	} else { // If socket was created
       		printf("Socket has been created successfully.\n");   
   	} // End if statement

	server.sin_port = htons (8082); // Set port
	server.sin_addr.s_addr = inet_addr("127.0.0.1"); // Set IP address
	server.sin_family = AF_INET; // Set family

	if (connect(SID, (struct  sockaddr*)&server, sizeof(server)) < 0) { // If connection failed
		perror("Connection failed! \n"); // Print error message
		return 1;
	} // end if

	printf("Connection to server was successful.\n"); // Print success message

	uid = getuid(); // Get user id
	c_uid = htonl(uid); // Convert user id to network byte order
	printf("Sending user ID %d to the server.\n", uid); // Print user id

	if (write(SID, &c_uid, sizeof(c_uid)) < 0) { // If sending user id failed
		perror("Sending user ID to server failed!"); // Print error message
	   	return 1;
	} // end if

	if (write(SID, dir, strlen(dir)) < 0) { // If sending directory failed
		perror("Sending directory to server failed:1\n"); // Print error message
       		return 1; 
	} // end if

	if (recv(SID, server_message, strlen("Just_received_the_directory"), 0) < 0) { // If receiving message failed
		perror("Error receiving data from server:\n"); // Print error message
		return 1;
	} // end if

    	if (strncmp(server_message, "User_not_in_group", strlen("User_not_in_group")) == 0) { // If user is not in the group
		printf("Transfer failed since the user is not in the %s group.\n", dir); // Print error message
		close(SID); // Close socket
		exit(EXIT_FAILURE); // Exit program
    	} // end if

	server_message[length] = '\0'; // Set end of string
	printf("Server > %s\n", server_message); // Print server message
	printf("Sending file name: %s\n", filename); // Print file name

	if (send(SID, filename, strlen(filename), 0) < 0) { // If sending file name failed
		perror("Failed sending file to server!\n"); // Print error message
		return 1;
	} // end if

    	memset(server_message,'\0',sizeof(server_message)); // Clear server message buffer
	
	if (recv(SID, server_message, 500, 0) < 0) { // If receiving message failed
		perror("Error receiving data fromm server!\n"); // Print error message
	} // end if
 
	printf("Server > Message %s of length %ld\n", server_message, strlen(server_message)); // Print server message

	if (strcmp(server_message, "startTransfer") == 0) { // If file transfer can begin
		printf("Sending file data: %s\n", filename); // Print file name

		char *file_ptr_path = "/home/jinapark/SS-CA-2-main/client_files/"; // File path
		char *file_ptr_name = (char *) malloc(1 + strlen(file_ptr_path) + strlen(filename)); // File name
		char input_buffer[LENGTH]; // Input buffer
		int file_block_size, i = 0; // File block size, i

        	strcpy(file_ptr_name, file_ptr_path); // Copy file path to file name
        	strcat(file_ptr_name, filename); // Concatenate file name to file path
		FILE *file_ptr = fopen(file_ptr_name, "r"); // Open file

		if (file_ptr == NULL) { // If file does not exist
			printf("Error! %s, file not found\n", file_ptr_name); // Print error message
			return 1;
		} // end if

		bzero(input_buffer, LENGTH); // Clear input buffer

       		//Run loop while there is still data to write to server
		while ((file_block_size = fread(input_buffer, sizeof(char), LENGTH, file_ptr)) > 0) { // While there is still data to write to server
			printf("Data sent: %d / %d\n", i , file_block_size); // Print data sent

			if (send(SID, input_buffer, file_block_size, 0) < 0) { // If sending file failed
				fprintf(stderr, "Error! Failed to send file %s. Errorno: %d\n", file_ptr_name, errno); // Print error message
				exit(1); // Exit program
			} // end if

			bzero(input_buffer, LENGTH); // Clear input buffer
			++i; // Increment i
		} // end while
	} // end if

	close(SID); // Close socket
	return 0;
} // end main

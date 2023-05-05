/*  Server Socket Program to run on the server  */

#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define LENGTH 512 // Buffer length
#define NUM_THREADS 100 // Number of threads

void *connection_handler(void *); // Thread handler
int user_in_group(gid_t , gid_t* , int); // Check if user is in group

pthread_mutex_t lock_x; // Mutex lock

int main() { // Main function
	int socket_des; // Socket descriptor
	int client_sock; // Client socket
	int con_size; // Connection size
	int thread_count = 0; // Thread count
	pthread_t client_conn[NUM_THREADS]; // Client connection
	struct sockaddr_in server, client; // Server and client address

	socket_des = socket(AF_INET, SOCK_STREAM, 0); // Create socket

	if(socket_des == -1) { // If socket creation fails
		perror("Error creating socket!\n"); // Print error
	} else { // If socket creation succeeds
		printf("Socket has been created successfully.\n"); // Print success
	} // end if else

	server.sin_port = htons(8082); // Set portSocket has been created successfully.
	server.sin_family = AF_INET; // Set family
	server.sin_addr.s_addr = INADDR_ANY; // Set address

	if (bind(socket_des, (struct sockaddr *) &server, sizeof(server)) < 0) { // If bind to socket fails
		perror("Error binding socket to server!\n"); // Print error
		return 1;
	} else { // If bind succeeds
		printf("Socket has been bound successfully.\n"); // Print success
	} // end if else

	listen(socket_des, 3); // Listen for connections

	printf("Waiting for an incoming connection from client!\n"); // Print waiting for connection
	con_size = sizeof(struct sockaddr_in); // Set connection size

	pthread_mutex_init(&lock_x, NULL); // Initialise mutex lock

    //Server will continually loop through as long as it's running waiting for any clinet connections
	while ((client_sock = accept(socket_des, (struct sockaddr *) &client, (socklen_t *) &con_size))) { // While there is a connection
		if (pthread_create(&client_conn[thread_count], NULL, connection_handler, (void*) &client_sock) < 0) { // If thread creation fails
			perror("Error creating thread!"); // Print error
			return 1;
		} // end if

		pthread_join(client_conn[thread_count], NULL); // Join thread
		thread_count++; // Increment thread count
	} // end while
	return 0;
} // end main

void *connection_handler(void *socket_des) { // Thread handler
	int sock = *(int *) socket_des; // Socket descriptor
	int READSIZE; // Read size
	char message[500]; // Message buffer
	char directory[20]; // Directory buffer
	uid_t client_user_id; // Client user ID

	READSIZE = recv(sock, &client_user_id, sizeof(client_user_id), 0); // Receive user ID from client

	if (READSIZE == -1) { // If connection fails
		printf("Error occurred in recv() call!\n"); // Print error
		exit(EXIT_FAILURE); // Exit
	} // end if

    client_user_id = ntohl(client_user_id); // Convert to network byte order
    printf("Client id: %d\n", client_user_id); // Print client ID
	printf("\nLocking the thread...\n"); // Print locking thread

	pthread_mutex_lock(&lock_x); // Lock mutex

	gid_t *groups; // Array of groups
	struct passwd *user_info; // User info
	user_info = getpwuid(client_user_id); // Get user info
	struct group *authorised_transfer_group; // Authorised transfer group

	char *user_name = user_info -> pw_name; // User name
    printf("Client id: %s\n", user_name); // Print client ID

	int ngroups = 20; // Number of groups
	groups = malloc(ngroups * sizeof(gid_t)); // Allocate memory for groups

	if (getgrouplist(user_name, client_user_id, groups, &ngroups) == -1) { // If get group list fails
        if (errno == EFAULT) { // If memory allocation error
            perror("Memory allocation error!\n"); // Print error
            exit(EXIT_FAILURE);

        } else if (errno == EINVAL) { // If invalid arguments
            perror("Invalid arguments!\n");
            exit(EXIT_FAILURE);

        } else if (errno == EPERM) { // If insufficient permissions
            perror("Insufficient permissions!\n");
            exit(EXIT_FAILURE);
			
        } else if (errno == -1) { // If system error
            perror("Error retrieving group list!\n");
            exit(EXIT_FAILURE);
        } // end if else if
	} // end if

	seteuid(client_user_id); // Set effective user ID

    memset(message,'\0',sizeof(message)); // Clear message buffer
	
	READSIZE = recv(sock, message, 500, 0); // Receive directory from client

	if (READSIZE == -1) { // If connection fails
		printf("Error occurred in recv() call!\n"); // Print error
		exit(1);
	} // end if

    strcpy(directory, message); // Copy message to directory buffer

    authorised_transfer_group = getgrnam(directory); // Get group name
    gid_t group_id = authorised_transfer_group -> gr_gid; // Get group ID

    if (!user_in_group(group_id, groups, ngroups)) { // If user is not in group
        write(sock, "User_not_in_group", strlen("User_not_in_group")); // Send message to client
        sleep(10); // Sleep for 10 seconds
        close(sock); // Close socket
        pthread_mutex_unlock(&lock_x); // Unlock mutex
        pthread_exit(NULL); // Exit thread
		printf("\nTransfer cancelled since the user is not in the group!\n"); // Print transfer cancelled
	} // end if

    printf("Vefiried that the user is in the group. Sending a directory received message.\n"); // Print user is in group
    write(sock, "Just_received_the_directory", strlen("Just_received_the_directory")); // Send message to client

    memset(message,'\0',sizeof(message)); // Clear message buffer

	READSIZE = recv(sock, message, 500, 0); // Receive filename from client
	
	printf("Starting the file transfer.\n"); // Print starting file transfer
	write(sock, "startTransfer", strlen("startTransfer")); // Send message to client
	printf("Filename: %s\n", message); // Print filename

	char fr_path[200] = "/home/jinapark/SS-CA-2-main/server_files/"; // File read path
	strcat(fr_path, directory); // Concatenate directory to file read path
	strcat(fr_path, "/"); // Concatenate / to file read path
	printf("File read path: %s\n", fr_path); // Print file read path

	char output_buffer[LENGTH]; // Output buffer
	char *fr_name = (char *) malloc(1 + strlen(fr_path) + strlen(message)); // File read name
	strcpy(fr_name, fr_path); // Copy file read path to file read name
	strcat(fr_name, message); // Concatenate message to file read name
	printf("fr_name: %s\n", fr_name); // Print file read name

	FILE *fr = fopen(fr_name, "w"); // Open file

	if (fr == NULL) { // If file cannot be opened
		printf("File %s can't be opened in the server! Errno: %d\n", fr_name, errno); // Print error
		exit(EXIT_FAILURE); // Exit
	} // end if

	bzero(output_buffer, LENGTH); // Clear output buffer
	int file_block_size = 0; // File block size

	while ((file_block_size = recv(sock, output_buffer, LENGTH, 0)) > 0) { // While file block size is greater than 0
		int write_sz = fwrite(output_buffer, sizeof(char), file_block_size, fr); // Write file

		if (write_sz < file_block_size) { // If write size is less than file block size
			perror("File write failed on the server!\n"); // Print error
		} // end if

		bzero(output_buffer, LENGTH); // Clear output buffer
	} // end while

	if (file_block_size < 0) { // If file block size is less than 0
		fprintf(stderr, "recv() failed due to error = %d\n", errno); // Print error
		exit(1);
	} // end if

	printf("Ok received from the client.\n"); // Print OK received from client
	fclose(fr); // Close file reader

	seteuid(0); // Set effective user ID to 0
	printf("\nEffective user ID has been reset.\n"); // Print effective user ID has been reset

	sleep(10); // Sleep for 10 seconds
	printf("\nSuccefully transferred file to %s\nUnlocking thread\n", directory); // Print successfully transferred file to directory

	pthread_mutex_unlock(&lock_x); // Unlock mutex
    memset(message,'\0',sizeof(message)); // Clear message buffer

	if (READSIZE == 0) { // If read size is 0
		puts("Client disconnected successfully.\n"); // Print client disconnected successfully
		fflush(stdout);

	} else if (READSIZE == -1) { // If read size is -1
		perror("Receive failed!\n"); // Print error
	} // end if else if
	return 0;
} // end void *connection_handler

int user_in_group(gid_t group_id, gid_t* groups, int num_groups) {
    for (int i = 0; i < num_groups; i++) { // For each group

        if (groups[i] == group_id) { // If group ID is equal to group ID
			printf("User is in the group %d\n", groups[i]); // Print user is in group
			return 1; // Return 1
		} // end if
    } // end for
    return 0;
} // end int user_in_group
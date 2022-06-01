#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

// Citation: Modified from course materials 8_3_server.c
// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}
// End Citation: Modified from course materials 8_3_server.c

char* encr(char plaintext[], char key[]) {
	char *s = malloc(256);
	char encryptedChar;

	//TODO: Remove Newlines.

	for(int i=0;i<strlen(plaintext);i++) {
		encryptedChar = ((plaintext[i] + key[i]) % 26) + 65;
		s[i] = encryptedChar;
		s[i+1] = '\0';
	}

	return s;
}

// Removes bad characters from input data.
char* cleanText(char *str) {
	int j;

	for(int i=0; i < strlen(str); i++) {
    	if((str[i] > 'Z') || (str[i] < 'A' && str[i] != ' ' )) {
        	for (j=i; j<strlen(str); j++)
            	str[j]=str[j+1];
    	}
    	else {
			i++;
		}
	}

	return str;
}

int main(int argc, char *argv[]) {
	// Defines port on which the encryption server will listen
	int port;
	// Defines connectionSocket
	int connectionSocket;
	// Defines child status, an integer
	int childStatus;
	// Defines server and client address structs
	struct sockaddr_in serverAddress, clientAddress;
	// Define socket length 
	socklen_t sizeOfClientInfo = sizeof(clientAddress);
	// Defines spawnpid as -5, its default value before assignment
	pid_t spawnpid = -5;

	// Takes the first argument and assigns it to be the key length. If no argument is specified then the default is 10.
	if (argv[1] != NULL) {
		port = atoi(argv[1]);
	} else {
		printf("Error: Must provide port in order for enc_server to function");
		return 0;
	}

  	// Create the socket that will listen for connections
  	int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  	if (listenSocket < 0) {
    	perror("ERROR opening socket");
  	}

  	// Set up the address struct for the server socket
  	setupAddressStruct(&serverAddress, port);

    // Associate the socket to the port
    if (bind(listenSocket, 
        (struct sockaddr *)&serverAddress, 
        sizeof(serverAddress)) < 0){
            perror("ERROR on binding");
    }

 	// Start listening for connections. Allow up to 5 connections to queue up
  	listen(listenSocket, 5); 

  	// Accept a connection, blocking if one is not available until one connects
  	while(1){
    	// Accept the connection request which creates a connection socket
    	connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
	
		printf("SERVER: Connected to client running at host %d port %d\n", ntohs(clientAddress.sin_addr.s_addr), ntohs(clientAddress.sin_port));

    	if (connectionSocket < 0){
    	  perror("SERVER: ERROR on accept");
    	}
	
		spawnpid = fork();
	
		switch(spawnpid) {
			case -1: {
				perror("SERVER: Failed to fork");
				exit(1);
				break;
			}
			case 0: {

				int fileLength;
				int charsRead;
				int fromEncryptionServer;
				
				// Define sendbuff andrecvbuff, used for sending and recieving data respectively.
				char sendbuff[256];
				char recvbuff1[256];
				char recvbuff2[256];
				char combined[512];
			
				// Sets buffers to be full with null terminators, preparing text to be added to them. 
				memset(sendbuff,'\0',256);
				memset(recvbuff1,'\0',256);
				memset(recvbuff2,'\0',256);
				memset(combined,'\0',512);

				// read(connectionSocket, &fileLength, sizeof(int));

				// printf("%d", fileLength);

				char *part1;
				char *part2;

				char *token;

				while(strcmp(token,"END") != 0) {
					memset(recvbuff1,'\0',256);
					memset(recvbuff2,'\0',256);
					memset(combined,'\0',512);

					charsRead = recv(connectionSocket, recvbuff1, 256, 0); 
					if (charsRead < 0){
    					perror("ERROR reading from socket");
    				}

					printf("%s",recvbuff1);

					char* rest = recvbuff1;
					part1 = strtok_r(rest, "!", &rest);
					part2 = strtok_r(rest, "!", &rest);
					char *encrypted;

					// printf("Part 1: %s\n", part1);
					// printf("Part 2: %s\n", part2);

					// If we haven't reached the end token, then we send part 1 and part 2 to the encryption function
					if (part2!=NULL) {
						part1 = cleanText(part1);
						part2 = cleanText(part2);

						// printf("encrypting parts 1 and 2.\n");
						// printf("Part 1: %s\n", part1);
						// printf("Part 2: %s\n", part2);
						encrypted = encr(part1, part2);
						
						// printf("Encrypted: %s\n", encrypted);
					} else { //If we have reached the end token, then we call again for another 256 length buffer
						// printf("encrypting parts 1 and 2. (full buffer)\n");
						charsRead = recv(connectionSocket, recvbuff2, 256, 0); 
						if (charsRead < 0){
    						perror("ERROR reading from socket");
    					}
						
						part1 = cleanText(part1);
						// part2 = cleanText(&recvbuff2[0]);

						encrypted = encr(part1, recvbuff2);

						// printf("Part 1: %s\n", part1);
						// printf("Part 2: %s\n", part2);
						// printf("Encrypted: %s\n", encrypted);
					}

					int charsWritten;

					printf("Ciphertext: %s", encrypted);

					charsWritten = send(connectionSocket, encrypted, strlen(encrypted),0);
					if (charsWritten < 0){
    					perror("CLIENT: ERROR writing to socket");
  					}
  					if (charsWritten < strlen(encrypted)){
    				printf("CLIENT: WARNING: Not all data written to socket!\n");
  					}

					token = strtok_r(rest, "!", &rest);
				}
    			
				// while(strcmp(recvbuff,"STOP") != 0) {
				// 	charsRead = recv(connectionSocket, recvbuff, 255, 0); 
				// 	if (charsRead < 0){
    			// 		perror("ERROR reading from socket");
    			// 	}
    			// 	printf("SERVER: I received this from the client: \"%s\"\n", recvbuff);
				// }


    			// // Send a Success message back to the client
    			// charsRead = send(connectionSocket, "I am the server, and I got your message", 39, 0); 
    			// if (charsRead < 0){
    			//   error("ERROR writing to socket");
				// }
				// Close the connection socket for this client

    			close(connectionSocket); 
				break;
			}
			default: {
				spawnpid = waitpid(spawnpid,&childStatus, WNOHANG);
			}
    	}
		return 0;
  	}
  //   Close socket, exit with successful return status
  close(listenSocket);
  return 0;
}

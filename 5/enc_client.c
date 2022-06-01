#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <errno.h>
#include <netdb.h>

// Removes bad characters from input data.
void cleanText(char *in, int len) {
    for(int i=0; i < len-1; i++) {

		// If the given character is outside of the range of A-Z and is not a space, then it's an invalid character. Throw error if so. 
        if((in[i] > 'Z') || (in[i] < 'A' && in[i] != ' ' )) {
            perror("CLIENT ERROR: Invalid Character Present \n");
			exit(0);
        }
    }
}

// Citation: Modified from course materials 8_3_client.c
// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname("localhost"); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host \n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}
// End Citation: Modified from course materials 8_3_client.c

int main(int argc, char *argv[]) {
	// Define file pointers for plaintext and key files
	FILE *plaintext;
	FILE *key;

	// Holds the port for which the server will try to connect to.
	int port;

	// Holds the serverAddress for socket creation reasons.
	struct sockaddr_in serverAddress;

	// Takes the first argument and assigns it to be the plaintext. If no argument is specified then throw an error.
	if (argv[1] != NULL) {

		// Open file as read only. If it's null, then throw an error. 
		plaintext = fopen(argv[1],"r");
	    if (plaintext == NULL) {
        	perror("CLIENT: Couldn't find plain text file \n");
			return 0;
    	}
	} else {
		perror("Error: Must provide plaintext in order for enc_server to function \n");
		return 0;
	}

	// Takes the first argument and assigns it to be the key. If no argument is specified then throw an error. 
	if (argv[2] != NULL) {

		// Open file as read only. If it's null, then throw an error. 
		key = fopen(argv[2],"r");
	    if (key == NULL) {
        	perror("CLIENT: Couldn't find key file \n");
			return 0;
    	}
	} else {
		perror("Error: Must provide key in order for enc_server to function \n");
		return 0;
	}

	// Takes the first argument and assigns it to be the port. If no argument is specified then throw an error. 
	if (argv[3] != NULL) {

		// Recieve port from argument #4 index 3, if it's null, then throw an error. 
		port = atoi(argv[3]);
	    if (&port == NULL) {
        	perror("CLIENT: Couldn't find key file \n");
			return 0;
    	}
	} else {
		perror("Error: Must provide port in order for enc_server to function \n");
		return 0;
	}

	// Creates socket named sock to connec to enc_server.
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if (sock < 0) {
		perror("CLIENT: ERROR opening socket \n");
		exit(0);
	}

	// Cals the setupAddressStruct function to open connection.
	setupAddressStruct(&serverAddress, atoi(argv[3]));

 	// Connect to server, throwing an error if connection fails.
  	if (connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    	perror("CLIENT: ERROR connecting \n");
		exit(0);
  	}

	// Opens plaintext file to find the length
	FILE *f1 = fopen(argv[2], "r");
	fseek(f1,0,SEEK_END);
	int keylen = ftell(f1);

	// Opens key file to find the length
	FILE *f2 = fopen(argv[1], "r");
	fseek(f2,0,SEEK_END);
	int textlen = ftell(f2);

	// Prints key and plaintext file length for debugging purposes. 
	// printf("%d %d", keylen, textlen);

	// If the key length if less than text length, then throw an error and exit. 
	if(keylen < textlen) {
		perror("CLIENT: ERROR Key Length less than Plaintext Length");
		exit(1);
	}

	// Define sendbuff andrecvbuff, used for sending and recieving data respectively.
	char sendbuff1[256];
	char sendbuff2[256];
	char recvbuff[256];

	// Sets buffers to be full with null terminators, preparing text to be added to them. 
	memset(sendbuff1,'\0',256);
	memset(sendbuff2,'\0',256);
	memset(recvbuff,'\0',256);

	// Sends plaintext to enc_server
	while(fgets(sendbuff1,255,plaintext)) {
		//todo: send to ENC_SERVER until ACK or mutex equivalent is recieved.
		//todo: strip newline in either sending or recieving data.

		cleanText(sendbuff1,strlen(sendbuff1));

		int charsWritten = send(sock, sendbuff1, strlen(sendbuff1),0);
		if (charsWritten < 0){
    		perror("CLIENT: ERROR writing to socket");
  		}
  		if (charsWritten < strlen(sendbuff1)){
    	printf("CLIENT: WARNING: Not all data written to socket!\n");
  		}

		// printf("%s\n\n",sendbuff1);

		memset(sendbuff2,'\0',256);

		const char mid[] = {"!!"};
		send(sock, mid, strlen(mid),0);

		fgets(sendbuff2,256,key); 
		charsWritten = send(sock, sendbuff2, strlen(sendbuff2),0);
		if (charsWritten < 0){
    		perror("CLIENT: ERROR writing to socket");
  		}
  		if (charsWritten < strlen(sendbuff2)){
    	printf("CLIENT: WARNING: Not all data written to socket!\n");
  		}
		memset(sendbuff1,'\0',256);
		memset(recvbuff,'\0',256);

		int charsRead;

		charsRead = recv(sock, recvbuff, 256, 0); 
		if (charsRead < 0){
    		perror("ERROR reading from socket");
    	}

		printf("CLIENT: CIPHERTEXT RECIEVED AS %s\n", recvbuff);

		

		// while(recv(sock, recvbuff1, sizeof(recvbuff1) - 1, 0)) {
		// 	//wait
		// }
	}

	// const char end[] = {"!END"}; // Marks the end of the string being sent. 
	// send(sock, end, strlen(end),0);

	// Recieves ciphertext from enc_server until STOP is recieved
	// while(strcmp(recvbuff,"STOP") != 0) {
	// 	memset(recvbuff,'\0',256);
	// 	int charsRead = recv(sock, recvbuff, sizeof(recvbuff) - 1, 0);
  	// 	if (charsRead < 0){
    // 		perror("CLIENT: ERROR reading from socket");
  	// 	}
  	// 	printf("CLIENT: I received this from the server: \"%s\"\n", recvbuff);
	// }
	
	// Close sockets and file pointers, return with successful status. 
	close(sock);
	fclose(plaintext);
	fclose(key);
	return 0;
}

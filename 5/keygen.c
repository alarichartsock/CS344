#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {

	// Defines how long the key we are generating is
	int keylen;

	// Takes the first argument and assigns it to be the key length. If no argument is specified then the default is 10.
	if (argv[1] != NULL) {
		keylen = atoi(argv[1]);
	} else {
		keylen = 10;
	}

	// Range of random characters that we want to use
	const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

	// Allocates a string with the size of the keylength + the null terminator
	char *str = malloc(keylen+1);

	// Seeds random so that all numbers are unique and we don't have repeat keys
	srand(time(NULL));

	// Creates a random string with the length of keylen
	for(int i=0; i<keylen; i++) {
		int randnum = rand() % (sizeof charset - 1);
		str[i] = charset[randnum];
		str[i+1] = '\0'; // Adds a null terminator. If there is more characters then it is overwritten, and if it is the end of the string then it stays
	}

	printf("%s",str); // Prints to standard out for redirection purposes

	return 0;
}

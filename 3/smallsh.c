#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Outstatus describes if we are outputting data to a file, inStatus describes if we out inputting data from a file
int outStatus = false;
int inStatus = false;

// Pointers to file names to output/input data from
char *infile;
char *outfile;

/*
* Removes trailing newlines and replaces instances of $$ with PID
*/
char* cleanToken(char *str) {
    if (str == NULL) {
        return str;
    }
    int len = strlen(str);
    if (str[len-1] == '\n') {
        str[len-1] = '\0';
    }

    bool clean = false;
    bool found = true;
    char *cleaned = malloc(sizeof(char)*150);
    strcpy(cleaned,str);

    while (clean == false) {
        for(int i=0; i<strlen(cleaned); i++) {
            // Iterate over each token
            if (cleaned[i] == '$' && cleaned[i+1] == '$') {
                // If two dollar signs are next to each other, then replace with PID
                found = true;

                // Beginning is the beginning of the string up until the $$
                char *beginning = malloc(sizeof(char) * 150);
                // Ending is the characters after the $$ until the end of the string
                char *ending = malloc(sizeof(char) * 150);

                // Fill beginning and ending with their respective substrings
                strncpy(beginning,cleaned, i);
                strncpy(ending,cleaned+i+2, strlen(cleaned)-i);

                // pid is the current process ID of the program
                char pid[15];
                sprintf(pid, "%d", getpid());

                // Reconstruct string as [beginning][pid][ending]
                strcat(beginning,pid);
                strcat(beginning,ending);

                // Beginning is actually the whole string, named beginning for vestigial reasons
                cleaned = beginning;
            } else {
                found = false;
            }
        }
        if (found == false) {
            // If we iterate over the entire token without finding $$, then the token has been cleaned
            clean = true;
        }
    }
    return cleaned;
}

/*
* TODO: comment when function is complete
*/
char** parseLine(char *line) {

    // Array of pointers to tokens, 512 tokens in length
    char **tokens = malloc(512 * sizeof(char*));

    // Context, used for strtok_r purposes
    char *context = NULL;

    char *token = strtok_r(line," ", &context);
    int token_index = 0;


    // Keep iterating over line and parsing out each token as it is handled
    while (token != NULL) {
        printf("%s",token);
        tokens[token_index] = cleanToken(token);
        token_index++;

        token = strtok_r(NULL," ", &context);
    }

    tokens[token_index] = NULL;
    return tokens;
}

/*
* Executes command provided by user. Forks off a child process.
*/
void execCommand(char **args) {
    pid_t newPID;
    // pid_t oldPID;

    newPID = fork();
    int childStatus; 


    switch(newPID) {
        case -1: // fork failed
            perror("fork failed");
            break;
        case 0: // fork completed; code in this block is to be executed by child process
            if(execvp(args[0],args)) {
                perror(args[0]);
            }
            break;
        default: // code in this block is to be executed by parent process
            // pass for now. Not sure what I want to add here. 
            waitpid(newPID,&childStatus, 0);
            fflush(stdout);
            break;
    }
}

/*
* TODO: comment when function is complete
*/
bool execTokens(char **tokens) {
    // Print out the token being handled for debugging purposes

    if (strcmp(tokens[0],"exit") == 0) {
        return false;
        // do nothing
    }
    else if (strcmp(tokens[0],"#") == 0 || tokens[0] == NULL) {
        return true;
        // return true, exiting from the shell. todo: kill background processes as well.
    } else if(strcmp(tokens[0],"cd") == 0) {
        if (tokens[1]) {
            if( chdir(tokens[1]) == -1 ) {
                printf("Error: Directory %s not found.", tokens[1]);
                return true;
            } else {
                return true;
            }
        } else {
            chdir(getenv("HOME"));
            return true;
        }
    }
    else { // pass to execCommand to execute
        execCommand(tokens);
        return true;
    }
}

/*
* Prompts the user for input while running == true. The only thing that will break running == true is the execTokens function returning False.
*/
void prompt() {
    bool running = true;
    char *commandline = NULL;
    char** tokens; 
    size_t bufsize = 2048;

    while (running == true) {

        printf(": ");
        fflush(stdout);
        getline(&commandline,&bufsize,stdin);

        // Retrieve length of commandline provided to shell
        size_t len = strlen(commandline);

        // Trim newline before tokenization. Beforehand, newlines were being tokenized.
        if(len > 0 && commandline[len-1] == '\n') {
            commandline[--len] = '\0';
        }

        tokens = parseLine(commandline);
        running = execTokens(tokens);

        // printf("%s",*tokens);
        // Execute main shell prompt
    }
}

int main(int argc,char **argv) {

    prompt();

    return 0;
}


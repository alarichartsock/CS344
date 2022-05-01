#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

// Outstatus describes if we are outputting data to a file, inStatus describes if we out inputting data from a file
int outStatus = false;
int inStatus = false;

// Describes the last exit value of the last foreground process. 
int lastExitVal = 0;

// Pointers to file names to output/input data from
char *infile;
char *outfile;

// Boolean values signifying if output/input redirection, and background processing is on/off
bool in_direction = false;
bool out_direction = false;
bool bg_processing = false;

// Signifies if the shell loop is running
bool running = true;

// Number of tokens within the tokens array
int numTokens = 0;

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

    out_direction = false;
    in_direction = false;

    // Array of pointers to tokens, 512 tokens in length
    char **tokens = malloc(512 * sizeof(char*));

    // Context, used for strtok_r purposes
    char *context = NULL;

    char *token = strtok_r(line," ", &context);
    int token_index = 0;


    // Keep iterating over line and parsing out each token as it is handled
    while (token != NULL) {
        if (strcmp(token," ") != 0) {
            // handle input redirection
            if (strcmp(token,"<") == 0) {
                // printf("Hitting <");
                in_direction = true;
                infile = strtok_r(NULL," ",&context);
                token = strtok_r(NULL," ",&context);
                if (token == NULL) { // infile is the last token in the list, break
                    break;
                }
            }

            // handle output redirection
            if (strcmp(token,">") == 0) {
                // printf("Hitting >");
                out_direction = true;
                outfile = strtok_r(NULL," ",&context);
                token = strtok_r(NULL," ",&context);
                if (token == NULL) { // infile is the last token in the list, break
                    break;
                }
            }

            // handle background processing
            if (strcmp(token,"&") == 0) {
                bg_processing = true;
                token = strtok_r(NULL," ",&context);
                if (token == NULL) { // infile is the last token in the list, break
                    break;
                }
            }

        if( (strcmp(token,"<") == 0) || (strcmp(token,">") == 0) || (strcmp(token,"&") == 0) ) {
            //pass
        } else {
            // If the next token isn't a special character, then store it. If it is, then complete operations in the preceding part of the loop in the next iteration. 
            tokens[token_index] = cleanToken(token);
            token_index++;
        }
 
        }
        token = strtok_r(NULL," ", &context);
    }

    numTokens = token_index;

    tokens[token_index] = NULL;
    return tokens;
}

/*
* Executes command provided by user. Forks off a child process.
*/
void execCommand(char **args) {
    pid_t newPID;
    pid_t waitPID;
    // pid_t oldPID;

    newPID = fork();

    // for (int i=0;i<sizeof(args);i++) {
    //     printf("%s", args[i]);
    // }

    switch(newPID) {
        case -1: // fork failed
            perror("fork failed\n");
            exit(1);
            break;
        case 0: // fork completed; code in this block is to be executed by child process

            if(in_direction) {
                int input = open(infile, O_RDONLY);
                if (input == -1) {
                    printf("Error opening inputfile %s\n", infile);
                    exit(1);
                } else {
                    if(dup2(input,STDIN_FILENO) == -1) {
                        perror("dup2 failed\n");
                    }
                }
            }

            if(out_direction) {
                int output = open(outfile, O_CREAT|O_WRONLY|O_TRUNC, 0644);
                if (output == -1) {
                    printf("Error writing outputfile %s\n", outfile);
                    exit(1);
                } else {
                    if(dup2(output,STDOUT_FILENO) == -1) {
                        perror("dup2 failed\n");
                    }
                }
            }

            if(execvp(args[0],args)) {
                // printf("running? or maybe failing?");/
                perror(args[0]);
                exit(1); // TODO: evaluate whether this is useful or not. For some reason, it runs the shell prompt twice. 
            }
            break;
        default: // code in this block is to be executed by parent process
            // pass for now. Not sure what I want to add here. 
            waitPID = waitpid(newPID,&lastExitVal, 0);
            if (waitPID == -1) {
                perror("waitpid");
                exit(1);
            }
            break;
    }
}

/*
* TODO: comment when function is complete
*/
void execTokens(char **tokens) {
    // Print out the token being handled for debugging purposes

    if (tokens[0] == NULL) {
        lastExitVal = 1;
    } else if (strcmp(tokens[0],"exit") == 0) {
        running = false;
        // do nothing
    } else if (strcmp(tokens[0],"#") == 0 || tokens[0] == NULL) {
        // return true, exiting from the shell. todo: kill background processes as well.
    } else if (strcmp(tokens[0],"cd") == 0) {
        if (tokens[1]) {
            if( chdir(tokens[1]) == -1 ) {
                printf("Error: Directory %s not found.", tokens[1]);
            }
        } else {
            chdir(getenv("HOME"));
        }
    } 
    else if (strcmp(tokens[0],"status") == 0) {
        printf("exit value %d\n", lastExitVal);
    }
    else { // pass to execCommand to execute
        execCommand(tokens);
    }
}

/*
* Prompts the user for input while running == true. The only thing that will break running == true is the execTokens function returning False.
*/
void prompt() {
    char *commandline = NULL;
    char** tokens; 
    size_t bufsize = 2048;

    while (running == true) {

        printf(": ");
        // fflush(stdout);
        getline(&commandline,&bufsize,stdin);

        // Retrieve length of commandline provided to shell
        size_t len = strlen(commandline);

        // Trim newline before tokenization. Beforehand, newlines were being tokenized.
        if(len > 0 && commandline[len-1] == '\n') {
            commandline[--len] = '\0';
        }

        tokens = parseLine(commandline);
        // printf("execTokens about to be called");
        execTokens(tokens);

        // printf("%s",*tokens);
        // Execute main shell prompt
    }
}

int main(int argc,char **argv) {

    prompt();

    return 0;
}


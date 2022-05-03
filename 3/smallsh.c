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

// Array of background PIDs, and the length of the array
int bg_pids[18];
int bg_pids_len = 0;

// Boolean value which represents if the program is in foreground or background mode
int fg = false;

// Boolean value which represents if sigint has been passed. If true then the foreground process will be killed
bool killFG = false;

/*
* Handles the SIGTSTP signal, switching between foreground and background mode
*/
void handle_SIGTSTP(int sig) {
    if(fg) {
        fg = false; 
        printf("\nExiting foreground-only mode\n: ");
    } else {
        fg = true;
        printf("\nEntering Foreground-only mode (& is now ignored)\n: ");
    }
    fflush(stdout);
}

/*
* Handles the SIGINT signal, killing the foreground process
*/
void handle_SIGINT(int sig) {
    printf("\nSigint recieved\n: ");
    killFG = true;
    fflush(stdout);
}

/*
* Removes a specific PID from the bg_pids array.
*/
void remove_pid(int pid) {
    for(int i=0; i<bg_pids_len; i++) {
        if (bg_pids[i] == pid) {
            while(i < bg_pids_len -1) {
                bg_pids[i] = bg_pids[i+1];
                i++;
            }
            bg_pids_len = bg_pids_len - 1;
            break;
        }
    }
}

/*
* Adds a specific PID into the bg_pids array.
*/
void add_pid(int pid) {
    bg_pids[bg_pids_len] = pid;
    bg_pids_len = bg_pids_len + 1;
}

/*
* Removes trailing newlines and replaces instances of $$ with PID
*/
char* cleanToken(char *str) {

    // If STR is null then there's nothing to clean
    if (str == NULL) {
        return str;
    }

    // Save len for iterative purposes
    int len = strlen(str);

    // Removes newline. This is redundant but good to ensure that no newlines are getting through
    if (str[len-1] == '\n') {
        str[len-1] = '\0';
    }

    // Clean describes if the token is fully processed. 
    bool clean = false;

    // Found describes if an instance of $$ has been found
    bool found = true;

    // Cleaned is the final token, with a max length of 150 characters
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
* Takes a line as an input, and parses out the tokens.
* Input and output tokens such as > and <, and & aren't added to the tokens to be returned. 
* .. But their prescence changes values like in_direction which change how commands are executed later in the program.
* If fg == True then & is ignored, blocking background processing
*/
char** parseLine(char *line) {

    // Resets output and input redirection 
    out_direction = false;
    in_direction = false;

    // Array of pointers to tokens, 512 tokens in length
    char **tokens = malloc(512 * sizeof(char*));

    // Context, used for strtok_r purposes
    char *context = NULL;

    // Kickstart the loop by calling strtok_r
    char *token = strtok_r(line," ", &context);

    // Token_index is the index of the next token to be added. 
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
                if(!fg) { // Will only turn background processing on if we aren't in foreground mode
                    bg_processing = true;
                }
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
        // Retrieve the next token
        token = strtok_r(NULL," ", &context);
    }

    // End array of tokens with a null member, helping iteration further down the line
    tokens[token_index] = NULL;
    return tokens;
}

/*
* Prints out either the exit value or terminating signal of the last process
*/
void status() {
    if (WIFEXITED(lastExitVal)) { // If the status ended normally, then print the exit value. If not, then print the terminating signal.
        printf("Exit value %d \n", WEXITSTATUS(lastExitVal));
    }
    else {
        printf("Terminated by signal %d \n", WTERMSIG(lastExitVal));
    }
}

/*
* Executes command provided by user. Forks off a child process.
*/
void execCommand(char **args) {
    pid_t newPID;
    pid_t waitPID;

    newPID = fork();

    switch(newPID) {
        case -1: // fork failed
            perror("fork failed\n");
            exit(1);
            break;
        case 0: // fork completed; code in this block is to be executed by child process

            // If input redirection is on
            if(in_direction) {
                // Open input file with read only permissions
                int input = open(infile, O_RDONLY);

                // If we have an error, then print the error. If not, check for file duplication error  
                if (input == -1) {
                    printf("Error opening inputfile %s\n", infile);
                    exit(1);
                } else {
                    if(dup2(input,STDIN_FILENO) == -1) {
                        perror("dup2 failed\n");
                    }
                }
            }

            // If output redirection is on
            if(out_direction) {
                // Write/truncate output file with 0644 permissions
                int output = open(outfile, O_CREAT|O_WRONLY|O_TRUNC, 0644);

                // If we have an error, then print the error. If not, check for file duplication error  
                if (output == -1) {
                    printf("Error writing outputfile %s\n", outfile);
                    exit(1);
                } else {
                    if(dup2(output,STDOUT_FILENO) == -1) {
                        perror("dup2 failed\n");
                    }
                }
            }

            // If we're doing background processing, then process input/output processing.
            if (bg_processing) {

                // If input redirection is off, then default to /dev/null/ for input
                if(!in_direction) {
                    int input = open("/dev/null", O_RDONLY);

                    // If we have an error, then print the error. If not, check for file duplication error  
                    if (input == -1) {
                        printf("Error opening inputfile /dev/null \n");
                        exit(1);
                    } else {
                        if(dup2(input,STDIN_FILENO) == -1) {
                            perror("dup2 failed \n");
                        }
                    }
                }

                // If output redirection is off, then default to /dev/null/ for input. 
                if(!out_direction) {
                    int output = open("/dev/null", O_CREAT|O_WRONLY|O_TRUNC, 0644);

                    // If we have an error, then print the error. If not, check for file duplication error  
                    if (output == -1) {
                        printf("Error writing outputfile /dev/null \n");
                        exit(1);
                    } else {
                        if(dup2(output,STDOUT_FILENO) == -1) {
                            perror("dup2 failed \n");
                        }
                    }
                }
            }

            // Execute command and arguments. If there is an error, then print it. 
            if(execvp(args[0],args)) {
                perror(args[0]);
                exit(1); 
            }
            break;
        default: // code in this block is to be executed by parent process

            // If we're processing in the foreground then wait using waitpid, returning control to the user when the command finishes
            if (!bg_processing) {
                waitPID = waitpid(newPID,&lastExitVal, 0);
                if (waitPID == -1) {
                    perror("waitpid error \n");
                    exit(1);
                }
            // If we're processing in the background then wait using waitpid, but pass WNOHANG so that user gets immediate control of the shell.
            // Store the PID of the background process using add_pid. Then set bg_processing to be false
            } else {
                waitPID = waitpid(newPID,&lastExitVal, WNOHANG);
                printf("background pid is %d \n", newPID);
                add_pid(newPID);
                bg_processing = false;
            }

            // Wait for program to terminate. When it does, print the PID of the child and notify the user that it has terminated.
            while((newPID = waitpid(-1, &lastExitVal, WNOHANG)) > 0) {
                printf("child terminated (PID %d) \n", newPID);
                remove_pid(newPID);
                status();
            }
            break;
    }
}

/*
* Takes in an array of tokens decides if the first element is a built in function, or if it should be passed to ExecCommand.
*/
void execTokens(char **tokens) {
    // Print out the token being handled for debugging purposes

    if (tokens[0] == NULL) { // If the first token is null, then pass
        //pass
    } else if (strcmp(tokens[0],"exit") == 0) { // If the token is exit, then exit the shell
        running = false; // exit the shell
    } else if (strcmp(tokens[0],"#") == 0 || tokens[0] == NULL) { // If the first token is a comment, then do nothing
        //pass
    } else if (strcmp(tokens[0],"cd") == 0) { // If the first token is cd, then handle it
        if (tokens[1]) { // If cd has arguments, then chdir to the first argument. 
            if( chdir(tokens[1]) == -1 ) {
                printf("Error: Directory %s not found. \n", tokens[1]);
            }
        } else { // If cd has no arguments then cd to the users home directory.
            chdir(getenv("HOME"));
        }
    } 
    else if (strcmp(tokens[0],"status") == 0) { // If the first token is status, then call the status function
        status();
    }
    else { // pass to execCommand to executen in all other cases
        execCommand(tokens);
    }
}

/*
* Prompts the user for input while running == true. The only thing that will break running == true is the execTokens function returning False.
*/
void prompt() {
    char *commandline = NULL;

    // Tokens returned by parseLine
    char** tokens; 

    // Variable to store the line of user input
    size_t bufsize = 2048;

    while (running == true) {

        printf(": ");
        getline(&commandline,&bufsize,stdin);

        // Retrieve length of commandline provided to shell
        size_t len = strlen(commandline);

        // Trim newline before tokenization. Beforehand, newlines were being tokenized.
        if(len > 0 && commandline[len-1] == '\n') {
            commandline[--len] = '\0';
        }

        // Parse comandline in parseLine and store the tokens
        tokens = parseLine(commandline);

        // Send tokens off for execution
        execTokens(tokens);
    }
}

/*
* The root of the execution of the program. Calls the signal function and initiates the prompt.
*/
int main(int argc,char **argv) {
    signal(SIGTSTP, handle_SIGTSTP); // Handles SIGTSTP 
    signal(SIGINT, handle_SIGINT); // Handles SIGINT
    prompt(); // Starts the shell prompt

    return 0;
}


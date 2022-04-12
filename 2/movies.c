#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

struct movie {
    char *name;
    int *year;
    char *languages[5];
    double *rating;
    struct movie *next;
};

// Print the year and name of a movie
void printMovie(struct movie *movie) {
    printf("%d %s \n", *movie->year, movie->name);
}

// Print the linked list of students
void printMovieList(struct movie *list)
{
    while (list != NULL)
    {
        printMovie(list);
        list = list->next;
    }
}

struct movie *createMovie(char *currLine) {
    struct movie *currMovie = malloc(sizeof(struct movie));

    char *saveptr;

    // Retrieve and store the name of the movie
    char *token = strtok_r(currLine, ",,", &saveptr);
    currMovie->name = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currMovie->name, token);

    // Retrieve and store the year of the movie
    token = strtok_r(NULL, ",,", &saveptr);
    currMovie->year = malloc(1*sizeof(int));
    *currMovie->year = atoi(token);

    // Retrieve the list of languages of the movie
    token = strtok_r(NULL, ",,", &saveptr);
    char *saveptr2;
    char *arraytoken = strtok_r(token,";",&saveptr2);

    // Break off each individual language and store it in the languages array
    int i = 0;
    int first = 0;

    for (int i=0;i<5;i++) {
        currMovie->languages[i] = malloc(21 * sizeof(char));
    }

    while (arraytoken != NULL) {
        // Special case, if we're in a singleton array then we remove both the closing and opening brackets in a single array item
        if (arraytoken[0] == '[' && arraytoken[strlen(arraytoken)-1] == ']') {
            for(int i=0; i<strlen(arraytoken);i++) {
                arraytoken[i] = arraytoken[i+1];
                if(arraytoken[i] == ']') {
                    arraytoken[i] = '\0';
                }
                first = 1;
            }
        }

        //Removes the opening bracket from the first item of the list
        if(first == 0) {
            for(int i=0; i<strlen(arraytoken);i++) {
                arraytoken[i] = arraytoken[i+1];
            }
            first = 1;
        } else { // Removes the ending bracket from the last item of the list
            for(int i=0; i<strlen(arraytoken);i++) {
                if(arraytoken[i] == ']') {
                    arraytoken[i] = '\0';
                }
            }
        }
        strcpy(currMovie->languages[i],arraytoken); // Fills currMovie with given array token
        arraytoken = strtok_r(NULL, ";", &saveptr2);
        i++;
    }

    // Retrieve and store the rating of the movie
    token = strtok_r(NULL, ",,", &saveptr);
    currMovie->rating = malloc(sizeof(double));
    *currMovie->rating = atof(token);

    currMovie->next = NULL;

    return currMovie;
}

struct movie *processFile(char *filePath) {

    FILE *movieFile;
    char *currLine = NULL;
    size_t len = 0;
    ssize_t nread;
    int firstLine = 1;
    errno = 0;

    movieFile = fopen(filePath, "r");

    int numMovies = 0;

    struct movie *newMovie;

    struct movie *head = NULL;
    struct movie *tail = NULL;

    // Iterate over input file line by line while checking for errors
    while ((nread = getline(&currLine, &len, movieFile)) != -1) {
        // Ignore the first line, print off the rest
        if(firstLine != 1) {
            numMovies++;
            newMovie = createMovie(currLine);

            if (head == NULL) {
                // This is the first node in the linked link
                // Set the head and the tail to this node
                head = newMovie;
                tail = newMovie;
            }
            else {
                // This is not the first node.
                // Add this node to the list and advance the tail
                tail->next = newMovie;
                tail = newMovie;
            }        
        } else {
            firstLine = 0;
        }
    }

    // printf("Processed file %s and parsed data for %d movies. \n\n", filePath,numMovies);
    free(currLine);
    fclose(movieFile);
    return head;
}

struct movie *validateInput() {
    char fname[20];


    printf("Enter the complete file name: ");
    scanf("%s",fname);

    if( access( fname, F_OK ) == 0 ) {
        printf("Now processing chosen file named %s \n", fname);
        return processFile(fname);
    } else {
        printf("The file name %s was not found. Try again", fname);
        return 0;
    }
}

void createFiles() {
    char dirname[50];
    char randnum[6];

    // Get username, append .movies. to the string value
    strcpy(dirname,getenv("USERNAME"));
    strcat(dirname,".movies.");
    
    // Get random number, mod my 100000 to reduce it, turn it into a string, then append it to the end of the directory name
    long r = random();
    r = r % 100000;
    sprintf(randnum,"%ld",r);
    strcat(dirname,randnum);

    mkdir(dirname, 750);

    

    printf("%s \n",dirname);


    // mkdir(name)
}

void secondaryPrompt() {
    int input;
    int finished = 0;

    while (finished != 1) {
        // Print explanation of valid input
        printf("Which file you want to process? \n");
        printf("Enter 1 to pick the largest file \n");
        printf("Enter 2 to pick the smallest file \n");
        printf("Enter 3 to specify the name of a file \n");
        printf("\nEnter a choice from 1 to 3: ");

        // Recieve correct input, formatted as an integer for easy comparison
        scanf("%d", &input);

        // If we didn't recieve a 1,2,3, or 4 then proposition the user for different input
        if(input != 1 && input != 2 && input != 3) {
            // printf("You selected %d \n", input);
            printf("You entered an incorrect choice. Try again. \n");
            input = 0;
        } else {
            if(input == 1) {
                //pass
                finished = 1;
            }
            else if(input == 2) {
                //pass
                finished = 1;
            }
            else if(input == 3) {
                struct movie *head = validateInput();
                createFiles();
                printf("%s",head->name);
                finished = 1;
            }
        }
    }
}

void mainPrompt() {
    int input;

    while (input != 2) {

        // Print explanation of valid input
        printf("\n1. Select file to process \n");
        printf("2. Exit the program \n");
        printf("\nEnter a choice 1 or 2: ");

        // Recieve correct input, formatted as an integer for easy comparison
        scanf("%d", &input);

        // If we didn't recieve a 1,2,3, or 4 then proposition the user for different input
        if(input != 1 && input != 2) {
            // printf("You selected %d \n", input);
            printf("You entered an incorrect choice. Try again. \n");
            input = 0;
        } else {
            if(input == 1) {
                secondaryPrompt();
            }
            else if(input == 2) {
                //pass, exit function
            }
        }
    }
}


    //     if(input == 1) { // Show movies released in the specified year
    //         int inputyear;

    //         // Saves the head of the linked list for future use
    //         struct movie *tmp = list;

    //         // Propositions the user for input on which movie they want to see, saves the input in a variable named input year
    //         printf("Enter the year for which you want to see movies: ");
    //         scanf("%d", &inputyear);
    //         printf("\n");

    //         int moviesMatched = 0;

    //         while (list != NULL) {
    //             if(*list->year == inputyear) {
    //                 printf("%s \n", list->name);
    //                 moviesMatched++;
    //             }
    //             list = list->next;   
    //         }
    //         if (moviesMatched == 0) {
    //             printf("No data about movies released in the year %d \n\n", inputyear);
    //         }

    //         // Resets the list variable to the start of the linked list
    //         list = tmp;

    //     } else if(input == 2) { // Show highest rated movie for each year
    //         double yeartorating[122];
    //         char *yeartoname[122];

    //         for (int i=0;i<122;i++) {
    //             yeartoname[i] = malloc(100 * sizeof(char));
    //         }

    //         // Saves the head of the linked list for future use
    //         struct movie *tmp = list;

    //         /* Iterates through the list of movies, using the year as a unique key for
    //          two corrsponding lists. One contains the rating, and the other contains the
    //         name of the film. The corresponding index is the year of the movie minus 1900 */
    //         while (list != NULL) {
    //             if(yeartorating[*list->year-1900] < *list->rating) {
    //                 yeartorating[*list->year-1900] = *list->rating;
    //                 strcpy(yeartoname[*list->year-1900],list->name);
    //             }
    //             list = list->next;
    //         }

    //         /* Prints off intialized values because these are the most highly rated movies */
    //         for(int i=0;i<122;i++) {
    //             double rating = yeartorating[i];
    //             char *name = yeartoname[i];
    //             int year = i + 1900;

    //             if(rating > 0.1) {
    //                 printf("%d %1.1f %s \n",year,rating,name);
    //             }
    //         }

    //         for (int i=0;i<122;i++) {
    //             free(yeartoname[i]);
    //         }
    //         printf("\n");

    //         // Resets the list variable to the start of the linked list
    //         list = tmp;
    //     } else if(input == 3) { // Show the title and year of release of all movies in a specific language
    //         char inputlanguage[20];

    //         printf("Enter the language for which you want to see movies: ");
    //         scanf("%s", inputlanguage);

    //         // int languageMatched = 0;

    //         // Saves the head of the linked list for future use
    //         struct movie *tmp = list;


    //         int languagesmatched = 0;

    //         // Iterates through the list of movies, printing off the year and name if it matches the input stored in inputlanguage
    //         while (list != NULL) {
    //             for(int i=0;i<5;i++) {
    //                 if (strcmp(inputlanguage,list->languages[i]) == 0) {
    //                     printf("%d %s \n",*list->year,list->name);
    //                     languagesmatched++;
    //                 }
    //             }
    //             list = list->next;
    //         }
    //         printf("\n");
    //         if(languagesmatched == 0) {
    //             printf("No data about movies released in %s \n", inputlanguage);
    //         }

    //         // Resets the list variable to the start of the linked list
    //         list = tmp;

    //     } else if(input == 4) { // Exit the program. Happens automatically with no instruction. 
    //         //pass
    //     }
    //     }
    // }

int main(int argc, char *argv[] ) {
    
    // Open file provided in arguments with read access
    // struct movie *head = processFile(argv[1]);
    // struct movie *list = head;
    mainPrompt();

    // while (list != NULL) {
    //     struct movie *old = list;
    //     list = list->next;
    //     free(old->year);
    //     free(old->name);
    //     for (int i=0;i<5;i++) {
    //         if(old->languages[i] != NULL) {
    //             free(old->languages[i]);
    //         }
    //     }
    //     free(old->rating);
    //     free(old);
    // }

    return 0;
}
//COP4610
//Project 1 Starter Code
//example code for initial parsing

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

typedef struct
{
	char** tokens;
	int numTokens;
} instruction;

void parseInput(instruction* instr_ptr);			//Parsing
void addToken(instruction* instr_ptr, char* tok);
void addNull(instruction* instr_ptr);

void shortcutResolution(instruction* instr_ptr);		//Shortcut Resolution
void expandVariable(char** token);

void pathResolution(instruction* instr_ptr);				//Path resolution

void executeCommand(instruction* instr_ptr);
void executePipedCommand(char ** command1, char ** command2);
int redirectionCheck(instruction* instr_ptr, int * preRedirectionSize);
int countPipes(instruction* instr_ptr);

char * getInputFile(instruction* instr_ptr);
char * getOutputFile(instruction* instr_ptr);

void printTokens(instruction* instr_ptr);			//Output tokens
void clearInstruction(instruction* instr_ptr);			//Clear Instruction

int pathExists(const char* path);				//Test if a path is valid
int pathIsFile(const char* path);
int pathIsDir(const char* path);

int main() {
	char* exit = "exit";

	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;

	while(1) {
		printf("%s@%s:%s>", getenv("USER"), getenv("MACHINE"), getenv("PWD"));	//PROMPT

		//Parse instruction
		parseInput(&instr);

		//Shortcut resolution
		shortcutResolution(&instr);

		//Output tokens
		//printTokens(&instr);

		//Clear instruction
		clearInstruction(&instr);
	}

	return 0;
}

void parseInput(instruction* instr_ptr) {

	char* token = NULL;
	char* temp = NULL;

	// loop reads character sequences separated by whitespace
	do {
		//scans for next token and allocates token var to size of scanned token
		scanf("%ms", &token);
		temp = malloc(strlen(token)+1);

		int i;
		int start = 0;
		for (i = 0; i < strlen(token); i++) {
			//pull out special characters and make them into a separate token in the instruction
			if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&') {
				if (i-start > 0) {
					memcpy(temp, token + start, i - start);
					temp[i-start] = '\0';
					addToken(instr_ptr, temp);
				}

				char specialChar[2];
				specialChar[0] = token[i];
				specialChar[1] = '\0';
				addToken(instr_ptr,specialChar);
				start = i + 1;
			}
		}

		if (start < strlen(token)) {
			memcpy(temp, token + start, strlen(token) - start);
			temp[i-start] = '\0';
			addToken(instr_ptr, temp);
		}

		//free and reset variables
		free(token);
		free(temp);
		token = NULL;
		temp = NULL;

	} while ('\n' != getchar());    //until end of line is reached

	addNull(instr_ptr);
}

//reallocates instruction array to hold another token
//allocates for new token within instruction array
void addToken(instruction* instr_ptr, char* tok)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**) malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**) realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	//allocate char array for new token in new slot
	instr_ptr->tokens[instr_ptr->numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
	strcpy(instr_ptr->tokens[instr_ptr->numTokens], tok);

	instr_ptr->numTokens++;
}

void addNull(instruction* instr_ptr)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**)malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	instr_ptr->tokens[instr_ptr->numTokens] = (char*) NULL;

	//instr_ptr->numTokens++; <-----Why increment the number of tokens for a null token
}

void shortcutResolution(instruction* instr_ptr)
{
	int i,j;
	int count = 0;

	const char* pwd = getenv("PWD");
	const char* home = getenv("HOME");
	char* temp;

	for(i = 0; i < instr_ptr->numTokens; i++)
	{
		//Expand variable token
		if(instr_ptr->tokens[i][0] == '$')
		{
			expandVariable(&instr_ptr->tokens[i]);
			continue;
		}
	}

	// prefix executable command paths
	pathResolution(instr_ptr);

	for(i = 0; i < instr_ptr->numTokens; i++)
	{
		//Do not convert if token is an absolute path
		if(instr_ptr->tokens[i][0] == '/') continue;

		for(j = 0; j < strlen(instr_ptr->tokens[i]); j++)
		{
			if((instr_ptr->tokens)[i][j] == '/')
			{
				//A slash is found, token is a relative path

				//If the token starts with '~', replace with $HOME
				if(instr_ptr->tokens[i][0] == '~')
				{
					temp = malloc(strlen(home)+strlen(instr_ptr->tokens[i])+2);
					strcpy(temp, home);
					strcat(temp, &instr_ptr->tokens[i][1]);
					instr_ptr->tokens[i] = realloc(instr_ptr->tokens[i], strlen(temp)+1);
					strcpy(instr_ptr->tokens[i], temp);
					free(temp);
					temp = NULL;
					break;
				}

				//Otherwise, expand relative directory into absolute directory
				else
				{
					temp = malloc(strlen(pwd)+strlen(instr_ptr->tokens[i])+2);
					strcpy(temp, pwd);
					strcat(temp, "/");
					strcat(temp, instr_ptr->tokens[i]);
					instr_ptr->tokens[i] = realloc(instr_ptr->tokens[i], strlen(temp)+1);
					strcpy(instr_ptr->tokens[i], temp);
					free(temp);
					temp = NULL;
					break;
				}
			}
		}
	}

	//After expanding tokens as directories, replace dot shortcuts
	for(i = 0; i < instr_ptr->numTokens; i++)
	{
		//Replace double dot first since single dot is a substring
		//Removed the directory before the double dot
		if((temp = strstr(instr_ptr->tokens[i], "/..")) != NULL)
		{
			char* temp2 = &temp[3];
			--temp;
			while(*temp != '/'){--temp;};
			memmove(temp, temp2, strlen(temp2)+1);
		}

		//Simply remove all instances of single dot
		if((temp = strstr(instr_ptr->tokens[i], "/.")) != NULL)
		{
			memmove(temp, &temp[2], strlen(temp)-1);
		}
	}
}

void expandVariable(char** token) {
	char* temp = malloc(strlen(*token));
	memcpy(temp, &token[0][1], strlen(*token));
	char* envar = getenv(temp);
	if(envar != NULL) {
		*token = realloc(*token, strlen(envar)+1);
		strcpy(*token, envar);
	}
	else {
		*token = realloc(*token, 7);
		strcpy(*token, "(null)");
	}
}

void pathResolution(instruction* instr_ptr) {

	int i = 0, j = 0;

	char* checkpaths[32];

	const char* path = getenv("PATH");

	char* envpath = malloc(strlen(path)+1);
    strcpy(envpath, path);

	char* path_split;


    for (int i = 0; i < instr_ptr->numTokens; i++) {
        if ((instr_ptr->tokens)[i] != NULL) {
            // prefix the first token and tokens immediately after a pipe
            if (i == 0 || strcmp((instr_ptr->tokens)[i - 1], "|") == 0) {
                //Loop to split $PATH into a pointer array
                if (i > 0) {
                    memcpy(envpath, path, strlen(path)+1);
                }
                path_split = strtok(envpath, ":");
                for (j=0; path_split != NULL; ++j) {
                    checkpaths[j] = malloc(strlen(path_split)+strlen(instr_ptr->tokens[i])+2);
                    if (i == 0) {
                        strcpy(checkpaths[j],path_split);
                    }
                    else {
                        memcpy(checkpaths[j],path_split, strlen(path_split) + 1);
                    }
                    path_split = strtok(NULL, ":");

                }
                //Check if the program exists in each directory
                //If it does, execute it
                for(int k = 0; k < j; ++k) {
                    strcat(checkpaths[k], "/");
                    strcat(checkpaths[k], instr_ptr->tokens[i]);
                    if(pathExists(checkpaths[k])) {
                        // printf("Command found at %s\n", checkpaths[k]);
                        instr_ptr->tokens[i] = realloc(instr_ptr->tokens[i], strlen(checkpaths[k])+1);
                        strcpy(instr_ptr->tokens[i], checkpaths[k]);

                        //Free memory
                        for(k = 0; k < j; ++k) {
                            free(checkpaths[k]);
                        }

                        break;
                    }
                    if (k == j - 1) {
                        // after searching all possible paths
                        printf("%s : Command not found\n", instr_ptr->tokens[i]);

                        //Free memory
                        for(k = 0; k < j; ++k) {
                            free(checkpaths[k]);
                        }
                        free(envpath);
                        return;
                    }
                }
            }
        }
    }
    free(envpath);
    executeCommand(instr_ptr);
}

void printTokens(instruction* instr_ptr)
{
	int i;
	printf("Tokens:\n");
	for (i = 0; i < instr_ptr->numTokens; i++) {
		if ((instr_ptr->tokens)[i] != NULL)
		printf("%s\n", (instr_ptr->tokens)[i]);
	}
}

void clearInstruction(instruction* instr_ptr)
{
	int i;
	for (i = 0; i < instr_ptr->numTokens; i++)
		free(instr_ptr->tokens[i]);

	free(instr_ptr->tokens);

	instr_ptr->tokens = NULL;
	instr_ptr->numTokens = 0;
}

//The following 3 functions come from the stat() documentation on man7.org
//http://man7.org/linux/man-pages/man2/stat.2.html

int pathExists(const char* path) {
	struct stat stats;
	if(stat(path, &stats) == 0) return 1;
	else return 0;
}

int pathIsFile(const char* path) {
	struct stat stats;
	stat(path, &stats);
	if(S_ISREG(stats.st_mode)) return 1;
	else return 0;
}

int pathIsDir(const char* path) {
	struct stat stats;
	stat(path, &stats);
	if(S_ISDIR(stats.st_mode)) return 1;
	else return 0;
}

void executeCommand(instruction* instr_ptr){
    int fd_in;
    int fd_out;

    char *inputFile, *outputFile;
    int preRedirectionSize = -1;
    int numPipes;
    int state = redirectionCheck(instr_ptr, &preRedirectionSize);
    if (state == 4) {
        numPipes = countPipes(instr_ptr);
    }

    if (state == -1) {
        perror("Error: Invalid syntax\n");
        exit(1);
    }

    // input redirection
    if (state == 1 || state == 3) {
        // input redirection for child process
        inputFile = getInputFile(instr_ptr);

        if( access( inputFile, F_OK ) == -1 ) {             // file does not exist
            perror("Error: Input file does not exist\n");
            //exit(1);
			return;
        }

        fd_in = open(inputFile, O_RDONLY);
    }

    // output redirection
    if (state == 2 || state == 3) {
        outputFile = getOutputFile(instr_ptr);

        if( access( outputFile, F_OK ) == -1 ) {             // create file if it does not exist
            fd_out = open(outputFile, O_RDWR | O_CREAT | O_TRUNC, 0600);
        }
        else {
            fd_out = open(outputFile, O_RDWR | O_CREAT | O_TRUNC);
        }
    }

    else if (state == 4) {
        // piping for child process
        
        // indexes of which tokens are pipes
        int * pipeIndex = calloc(numPipes + 1, sizeof(int));        // is + 1 needed or not?
        int j = 0;
        for (int i = 0; i < instr_ptr->numTokens; i++) {
            if ((instr_ptr->tokens)[i] != NULL) {
                if (strcmp((instr_ptr->tokens)[i], "|") == 0) {
                    pipeIndex[j] = i;
                    j++;
                }
            }
        }

        int numCommand1Tokens = pipeIndex[0];
        char **command1 = (char **) malloc((numCommand1Tokens + 1) * sizeof(char *));
        // put tokens before first pipe in the command1 array
        for (int i = 0; i < numCommand1Tokens; i++) {
            if ((instr_ptr->tokens)[i] != NULL) {
                command1[i] = malloc(strlen((instr_ptr->tokens)[i] + 1));
                strcpy(command1[i], instr_ptr->tokens[i]);
            }
        }

        command1[numCommand1Tokens] = NULL;
        /*printf("%d\n", numCommand1Tokens);

         for (int i = 0; i < numCommand1Tokens; i++) {
            if ((command1)[i] != NULL) {
                printf("%s\n", command1[i]);
            }
        }*/

        int numCommand2Tokens = instr_ptr->numTokens - numCommand1Tokens - 1;
        int startingIndex = pipeIndex[0] + 1;
        char **command2 = (char **) malloc((numCommand2Tokens + 1) * sizeof(char *));
        int k = 0;
        for (int i = startingIndex; i < startingIndex + numCommand2Tokens; i++) {
            if ((instr_ptr->tokens)[i] != NULL) {
                command2[k] = malloc(strlen((instr_ptr->tokens)[i] + 1));
                strcpy(command2[k], instr_ptr->tokens[i]);
                k++;
            }
        }

        command2[numCommand2Tokens] = NULL;

        /*for (int i = 0; i < numCommand2Tokens; i++) {
            if ((command2)[i] != NULL) {
                printf("%s\n", command2[i]);
            }
        }*/
        executePipedCommand(command1, command2);
        free(command1);
        free(command2);
        free(pipeIndex);
    }

    pid_t pid = fork();

    int status;

    if(pid == -1){
        perror("Error: Forking failed\n");
        exit(1);
    }

    else if(pid == 0){
        //child process
        if (state == 1 || state == 3) {
            // input redirection for child process
            close(STDIN_FILENO);
            dup(fd_in);
            close(fd_in);
        }
        if (state == 2 || state == 3) {
            // output redirection for child process
            close(STDOUT_FILENO);
            dup(fd_out);
            close(fd_out);
        }
        if (state > 0 && state < 4) {
            // execution for input and/or output redirection
            char **parameters = (char **) malloc((preRedirectionSize + 1) * sizeof(char *));
            for (int i = 0; i < preRedirectionSize; i++) {
                if ((instr_ptr->tokens)[i] != NULL) {
                    parameters[i] = malloc(strlen((instr_ptr->tokens)[i] + 1));
                    strcpy(parameters[i], instr_ptr->tokens[i]);
                }
            }
            parameters[preRedirectionSize] = NULL;
            if (execv(parameters[0], parameters) == -1) {
                perror("Error: Command execution failed\n");
                exit(1);
            }
            free(parameters);                                   // free parameters before exiting?
        }
        else if (state == 0) {                                  // just execute command without io redirection
            if (execv(instr_ptr->tokens[0], instr_ptr->tokens) == -1) {
                perror("Error: Command execution failed\n");
                exit(1);
            }
        }
    }
    else{
        //parent
        waitpid(pid, &status, 0);
    }
}

void executePipedCommand(char ** command1, char ** command2) {
    pid_t pid1, pid2;
    int fd_pipe[2];                 // adapt to more than 1 pipe (https://stackoverflow.com/questions/12679075/almost-perfect-c-shell-piping)
    int status;

    if (pipe(fd_pipe) < 0) {
        perror("Error: Piping failed\n");
        exit(1);
    }

    pid1 = fork();
    if(pid1 < 0){
        perror("Error: Forking failed\n");
        exit(1);
    }
    /*for (int i = 0; i <= 2; i++) {
        if ((command1)[i] != NULL) {
            printf("%s\n", command1[i]);
        }
    }
    for (int i = 0; i <= 1; i++) {
        //if ((command2)[i] != NULL) {
            printf("%s\n", command2[i]);
        }
    }*/
    if (pid1 == 0) {
        // 1st command (writer)
        close(STDOUT_FILENO);
        dup(fd_pipe[1]);
        close(fd_pipe[0]);
        close(fd_pipe[1]);

        if (execv(command1[0], command1) == -1) {
            perror("Error: Command execution failed\n");
            exit(1);
        }
    }
    else {
        // 2nd command (reader)
        pid2 = fork();
        if (pid2 < 0) {
            perror("Error: Forking failed\n");
            exit(1);
        }
        if (pid2 == 0) {
            close(STDOUT_FILENO);
            dup(fd_pipe[0]);
            close(fd_pipe[0]);
            close(fd_pipe[1]);

            if (execv(command2[0], command2) == -1) {
                perror("Error: Command execution failed\n");
                exit(1);
            }
        }
        else {
            waitpid(pid1, &status, 0);
            waitpid(pid2, &status, 0);
        }
    }
}

// checks for redirection or piping
int redirectionCheck(instruction* instr_ptr, int * preRedirectionSize) {
    int input_state = 0, output_state = 0;          // booleans for type of redirection state
    for (int i = 0; i < instr_ptr->numTokens; i++) {
        if ((instr_ptr->tokens)[i] != NULL) {
            if (strcmp((instr_ptr->tokens)[i], "<") == 0) {
                if ((instr_ptr->tokens)[i - 1] == NULL ||(instr_ptr->tokens)[i + 1] == NULL) {   // replace this later with more sophisticated error checking
                    // must have file after '<' &&
                    // (must have command before '<' || must have a file before '<' if
                    // there is also output redirection)
                    return -1;                                     // invalid syntax
                }
                if (*preRedirectionSize == -1) {
                    *preRedirectionSize = i;
                }
                input_state = 1;
            }
            else if (strcmp((instr_ptr->tokens)[i], ">") == 0) {
                if ((instr_ptr->tokens)[i - 1] == NULL ||(instr_ptr->tokens)[i + 1] == NULL) {
                    return -1;                                     // invalid syntax
                }
                if (*preRedirectionSize == -1) {
                    *preRedirectionSize = i;
                }
                output_state = 1;
            }
            else if (strcmp((instr_ptr->tokens)[i], "|") == 0) {
                return 4;                                      // piping
            }
            if (input_state == 1 && output_state == 1) {
                return 3;                                      // both input and output redirection
            }
        }
    }
    if (input_state == 1) {
        return 1;                                      // only input redirection
    }
    else if (output_state == 1) {
        return 2;                                      // only output redirection
    }
    if (input_state == 0 && output_state == 0) {
        return 0;                                      // no redirection or piping
    }
}

int countPipes(instruction* instr_ptr) {
    int pipes = 0;
    for (int i = 0; i < instr_ptr->numTokens; i++) {
        if ((instr_ptr->tokens)[i] != NULL) {
            if (strcmp((instr_ptr->tokens)[i], "|") == 0) {
                pipes++;
            }
        }
    }
    return pipes;
}

char * getInputFile(instruction* instr_ptr) {
    char * inputFile;
    for (int i = 1; i < instr_ptr->numTokens; i++) {
        if (strcmp((instr_ptr->tokens)[i - 1], "<") == 0) {
            inputFile = (char*)malloc((strlen((instr_ptr->tokens)[i]) + 1) * sizeof(char));
            strcpy(inputFile, instr_ptr->tokens[i]);
            return inputFile;
        }
    }
}

char * getOutputFile(instruction* instr_ptr) {
    char * outputFile;
    for (int i = 1; i < instr_ptr->numTokens; i++) {
        if (strcmp((instr_ptr->tokens)[i - 1], ">") == 0) {
            outputFile = (char*)malloc((strlen((instr_ptr->tokens)[i]) + 1) * sizeof(char));
            strcpy(outputFile, instr_ptr->tokens[i]);
            return outputFile;
        }
    }
}
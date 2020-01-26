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

int redirectionCheck(instruction* instr_ptr, int * preRedirectionSize);
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

	//Checking if first token is a command.
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

	//Loop to split $PATH into a pointer array
	path_split = strtok(envpath, ":");
	for (j=0; path_split != NULL; ++j) {
		checkpaths[j] = malloc(strlen(path_split)+strlen(instr_ptr->tokens[0])+2);
		strcpy(checkpaths[j],path_split);
		path_split = strtok(NULL, ":");
	}

	//Check if the program exists in each directory
	//If it does, execute it
	for(i=0; i < j; ++i) {
		strcat(checkpaths[i], "/");
		strcat(checkpaths[i], instr_ptr->tokens[0]);
		if(pathExists(checkpaths[i])) {
			//printf("Command found at %s\n", checkpaths[i]);
			instr_ptr->tokens[0] = realloc(instr_ptr->tokens[0], strlen(checkpaths[i])+1);
			strcpy(instr_ptr->tokens[0], checkpaths[i]);

			//Free memory
			for(i=0;i<j;++i) {
				free(checkpaths[i]);
			}
			
			executeCommand(instr_ptr);
			return;
		}

	}

	printf("%s : Command not found\n", instr_ptr->tokens[0]);

	//Free memory
	for(i=0;i<j;++i) {
		free(checkpaths[i]);
	}
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
    pid_t pid;

    int fd_in;
    int fd_out;

    char *inputFile, *outputFile;
    int preRedirectionSize = -1;
    int state = redirectionCheck(instr_ptr, &preRedirectionSize);

    if (state == -1) {
        printf("Error: Invalid syntax\n");
        exit(1);
    }

    // input redirection
    if (state == 1 || state == 3) {
        // input redirection for child process
        inputFile = getInputFile(instr_ptr);

        if( access( inputFile, F_OK ) == -1 ) {             // file does not exist
            printf("Error: File %s does not exist\n", inputFile);
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

    pid = fork();

    int status;
    if(pid == -1){
        //error
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
        if (state > 0) {
            char **parameters = (char **) malloc((preRedirectionSize + 1) * sizeof(char *));
            for (int i = 0; i < preRedirectionSize; i++) {
                if ((instr_ptr->tokens)[i] != NULL) {
                    parameters[i] = malloc(strlen((instr_ptr->tokens)[i] + 1));
                    strcpy(parameters[i], instr_ptr->tokens[i]);
                }
            }
            parameters[preRedirectionSize] = NULL;
            execv(parameters[0], parameters);
            free(parameters);
        }
        if (state == 0) {                                  // just execute command without io redirection
            execv(instr_ptr->tokens[0], instr_ptr->tokens);
        }
    }
    else{
        //parent
        waitpid(pid, &status, 0);
    }
}

//checks for redirection and returns the case
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
            if (input_state == 1 && output_state == 1) {
                return 3;                                      // input and output redirection
            }
        }
    }
    if (input_state == 1) {
        return 1;
    }
    else if (output_state == 1) {
        return 2;
    }
    if (input_state == 0 && output_state == 0) {
        return 0;                                      // no redirection
    }
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
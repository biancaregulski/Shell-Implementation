//COP4610
//Project 1 Starter Code
//example code for initial parsing

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct
{
    char** tokens;
    int numTokens;
} instruction;

void extendTokens(instruction* instr_ptr);			//Path Resolution
int pathExists(const char* path);
void checkPaths(instruction* instr_ptr);				//return 1 for true, otherwise false
void executeCommand(instruction* instr_ptr);
int redirectionCheck(instruction* instr_ptr);
char * getInputFile(instruction* instr_ptr);
char * getOutputFile(instruction* instr_ptr);

void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void addNull(instruction* instr_ptr);

int main() {
    char* exit = "exit";
    char* token = NULL;
    char* temp = NULL;

    instruction instr;
    instr.tokens = NULL;
    instr.numTokens = 0;


    while(1) {
        printf("%s@%s:%s > ", getenv("USER"), getenv("MACHINE"), getenv("PWD"));						//PROMPT

        // loop reads character sequences separated by whitespace
        do {
            //scans for next token and allocates token var to size of scanned token
            scanf("%ms", &token);
            temp = (char*)malloc((strlen(token) + 1) * sizeof(char));

            int i;
            int start = 0;
            for (i = 0; i < strlen(token); i++) {
                //pull out special characters and make them into a separate token in the instruction
                if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&') {
                    if (i-start > 0) {
                        memcpy(temp, token + start, i - start);
                        temp[i-start] = '\0';
                        addToken(&instr, temp);
                    }

                    char specialChar[2];
                    specialChar[0] = token[i];
                    specialChar[1] = '\0';

                    addToken(&instr,specialChar);

                    start = i + 1;
                }
            }

            if (start < strlen(token)) {
                memcpy(temp, token + start, strlen(token) - start);
                temp[i-start] = '\0';
                addToken(&instr, temp);
            }

            //free and reset variables
            free(token);
            free(temp);

            token = NULL;
            temp = NULL;
        } while ('\n' != getchar());    //until end of line is reached

        addNull(&instr);
        extendTokens(&instr);		//--------------------------------------------------------extend token function-----------------------------
        printTokens(&instr);
        executeCommand(&instr);
        clearInstruction(&instr);
    }

    return 0;
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
    instr_ptr->numTokens++;
}

//-----------------------------------------------------PATH RESOLUTION-------------------------------

void extendTokens(instruction* instr_ptr)
{
    int i;
    int count = 0;

    char relative[1024];
    char* pwd = getenv("PWD");
    char* prevDir = "..";
    char* currDir = ".";
    char* homeDir = "~";

    for(i = 0; i < instr_ptr->numTokens; i++)
    {
        if ((instr_ptr->tokens)[i] != NULL){
            if(strcmp((instr_ptr->tokens)[i], prevDir) == 0){
                char* temp = malloc(strlen(getenv("PWD")));
                int i, j;
                for(i = strlen(getenv("PWD")); pwd[i] != '/'; i-- ){
                    count++;
                }

                for(j = 0; j < strlen(getenv("PWD")) - count; j++){
                    temp[j] = pwd[j];
                }
                free(instr_ptr->tokens[i]);
                instr_ptr->tokens[i] = malloc(strlen(temp)+1);
                strcpy(instr_ptr->tokens[i], temp);
                free(temp);
            }
            else if(strcmp((instr_ptr->tokens)[i], currDir) == 0){
                free(instr_ptr->tokens[i]);
                instr_ptr->tokens[i] = malloc(strlen(pwd)+1);
                strcpy(instr_ptr->tokens[i], pwd);

            }
            else if(strcmp((instr_ptr->tokens)[i], homeDir) == 0){
                free(instr_ptr->tokens[i]);
                instr_ptr->tokens[i] = malloc(strlen(getenv("HOME"))+1);
                strcpy(instr_ptr->tokens[i], getenv("HOME"));
            }


        }
    }
    checkPaths(instr_ptr);
}

void printTokens(instruction* instr_ptr)
{
    int i;
    printf("Tokens:\n");
    for (i = 0; i < instr_ptr->numTokens; i++) {
        if ((instr_ptr->tokens)[i] != NULL) {
            printf("%s\n", (instr_ptr->tokens)[i]);
        }
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

int pathExists(const char* path) {					//return 1 for it does exist
    if(access(path, F_OK) == -1) return 0;
    else return 1;
}

void checkPaths(instruction* instr_ptr) {
    int commandFound = 0;
    char* path_split = (char*) malloc(sizeof(char)*50);
    char* checkpaths[20];
    int a = 0;
    for(a = 0; a < 20; ++a) {
        checkpaths[a] = (char*) malloc(sizeof(char)*50);
    }
    char* envpath = malloc(strlen(getenv("PATH"))+1);
    strncpy(envpath,getenv("PATH"),strlen(getenv("PATH")));
    strcpy(path_split,strtok(envpath, ":"));
    int j = 0, k = 0;
    while (path_split != NULL) {
        strncpy(checkpaths[j],path_split,strlen(path_split)+1);
        ++j;
        path_split = strtok(NULL, ":");
    }
    for(k=0; k < j; ++k) {
        strcat(checkpaths[k], "/");						//append command to path string
        strcat(checkpaths[k], instr_ptr->tokens[0]);
        if(pathExists(checkpaths[k])) {
            //printf("Executing command %s\n", instr_ptr->tokens[0]);
            commandFound = 1;						//subway into execute


            free(instr_ptr->tokens[0]);
            instr_ptr->tokens[0] = malloc(strlen(checkpaths[k])+1);
            strcpy(instr_ptr->tokens[0], checkpaths[k]);
            //executeCommand(instr_ptr->tokens);
            break;
        }

    }
    if(commandFound == 0){
        // printf("%s : No command found\n", instr_ptr->tokens[0]);
    }
    int d = 0;
    for(d=0;d<20;++d) {
        free(checkpaths[d]);
    }
    free(envpath);
    free(path_split);
}

void executeCommand(instruction* instr_ptr){
    pid_t pid;

    int fd_in;
    int fd_out;
	
    char *inputFile, *outputFile;
    int state = redirectionCheck(instr_ptr);
    //printf("%d\n", state);

	char ** parameters;				
	int parameterIndex = 0;
	
	if (state > 0) {
        if (state == 1 || state == 2) {
            parameters = malloc(3 * sizeof(char*));
        }
        else {
            parameters = malloc(5 * sizeof(char*));
        }
		parameters[parameterIndex] = malloc((strlen(instr_ptr->tokens[0]) + 1) * sizeof(char));
		strcpy(parameters[parameterIndex], instr_ptr->tokens[0]);
		parameterIndex += 1;
	}
	if (state == 1 || state == 3) {
		// input redirection for child process
        parameters[parameterIndex] = malloc((1 + 1) * sizeof(char));
        strcpy(parameters[parameterIndex], ">");
        parameterIndex += 1;

        inputFile = getInputFile(instr_ptr);
		fd_in = open(inputFile, O_RDONLY);
		parameters[parameterIndex] = malloc((strlen(inputFile) + 1) * sizeof(char));
		strcpy(parameters[parameterIndex], inputFile);
		parameterIndex += 1;
	}
	if (state == 2 || state == 3) {
		// output redirection for child process
        parameters[parameterIndex] = malloc((1 + 1) * sizeof(char));
        strcpy(parameters[parameterIndex], "<");
        parameterIndex += 1;

		outputFile = getOutputFile(instr_ptr);
		fd_out = open(outputFile, O_RDWR | O_CREAT | O_TRUNC);	
		parameters[parameterIndex] = malloc((strlen(outputFile) + 1) * sizeof(char));
		strcpy(parameters[parameterIndex], outputFile);	
	}
  
void executeCommand(instruction* instr_ptr){
	pid_t pid;

	int fd_in;
	int fd_out;

	char *inputFile, *outputFile;
	int state = redirectionCheck(instr_ptr);
	//printf("%d\n", state);

	char * parameters;
	int parameterIndex = 0;

	if (state == -2) {
		printf("Invalid syntax\n");
	}
	if (state > 0) {
		parameters = malloc((strlen(instr_ptr->tokens[0]) + 1) * sizeof(char));
		strcpy(parameters, instr_ptr->tokens[0]);
		parameterIndex += 1;
	}

	// input redirection
	if (state == 1 || state == 3) {
		if( access( inputFile, F_OK ) == -1 ) {             // create file if it does not exist
			printf("File %s does not exist\n", inputFile);
		}

		// input redirection for child process
		parameters = (char*)realloc(parameters, strlen(parameters) + 3 + strlen(inputFile) + 1 * sizeof(char));
		strcat(parameters, " < ");
		strcat(parameters, inputFile);
		inputFile = getInputFile(instr_ptr);
		fd_in = open(inputFile, O_RDONLY);

		parameterIndex += 2;
	}

	// output redirection
	if (state == 2 || state == 3) {
		parameters = (char*)realloc(parameters, strlen(parameters) + 3 + strlen(outputFile) + 1 * sizeof(char));
		strcat(parameters, " > ");

		outputFile = getOutputFile(instr_ptr);

		if( access( outputFile, F_OK ) == -1 ) {             // create file if it does not exist
			fd_out = open(outputFile, O_RDWR | O_CREAT | O_TRUNC, 0600);
		}
		else {
			fd_out = open(outputFile, O_RDWR | O_CREAT | O_TRUNC);
		}

		strcat(parameters, outputFile);
		parameterIndex += 2;
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
			system(parameters);
			free(parameters);
			exit(0);
		}
		else {
			execv(instr_ptr->tokens[0], instr_ptr->tokens);
		}
	}
	else{
	    //parent
	    waitpid(pid, &status, 0);
	}
}

//checks for redirection and returns the case
int redirectionCheck(instruction* instr_ptr) {
    int input_state = 0, output_state = 0;          // booleans for type of redirection state
    
    for (int i = 0; i < instr_ptr->numTokens; i++) {
        if ((instr_ptr->tokens)[i] != NULL) {
            if (strcmp((instr_ptr->tokens)[i], "<") == 0) {
                input_state = 1;
            }
            else if (strcmp((instr_ptr->tokens)[i], ">") == 0) {
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
        return -1;                                      // no redirection
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

//checks for redirection and returns the case
int redirectionCheck(instruction* instr_ptr) {
	int input_state = 0, output_state = 0;          // booleans for type of redirection state

	for (int i = 0; i < instr_ptr->numTokens; i++) {
		if ((instr_ptr->tokens)[i] != NULL) {
			if (strcmp((instr_ptr->tokens)[i], "<") == 0) {
				if ((instr_ptr->tokens)[i - 1] == NULL ||(instr_ptr->tokens)[i + 1] == NULL) {   // replace this later with more sophisticated error checking
					// must have file after '<' &&
					// (must have command before '<' || must have a file before '<' if
					// there is also output redirection)
					return -2;                                     // invalid syntax
				}
				input_state = 1;
			}
			else if (strcmp((instr_ptr->tokens)[i], ">") == 0) {
				if ((instr_ptr->tokens)[i - 1] == NULL ||(instr_ptr->tokens)[i + 1] == NULL) {
					return -2;                                     // invalid syntax
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
		return -1;                                      // no redirection
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

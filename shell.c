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

typedef struct
{
	int pid;
	char* cmd;
} queueEntry;

void parseInput(instruction* instr_ptr);			//Parses input into instruction
void addToken(instruction* instr_ptr, char* tok);		//Adds token to instruction.tokens
void addNull(instruction* instr_ptr);				//Adds null token to the end

void expandVariables(instruction* instr_ptr);			//Expand tokens that start with $

void shortcutResolution(instruction* instr_ptr);		//Shortcut Resolution
void expandPath(char** token);					//Expand shortcuts

void pathResolution(instruction* instr_ptr);			//Path resolution

void executeCommand(instruction* instr_ptr, int withBackground);	//withBackground = 0 when there's no background process, 1 when there is 
void executePipedCommand(char ** command1, char ** command2, char ** command3);

int redirectionCheck(instruction* instr_ptr, int * preRedirectionSize);
char * getInputFile(instruction* instr_ptr);
char * getOutputFile(instruction* instr_ptr);
int countPipes(instruction* instr_ptr);

int pathExists(const char* path);			//Test if a path is valid. 1 = True, 0 = False
int pathIsFile(const char* path);			//Test if path is a file
int pathIsDir(const char* path);			//Test if path is a directory

int checkBuiltIn(instruction* instr_ptr);		//Check Built Ins
void exit_builtin(instruction* instr_ptr);		//1. Exit shell
void cd_builtin(instruction* instr_ptr);		//2. Change Directory
void echo_builtin(instruction* instr_ptr);		//3. Print tokens
void jobs_builtin(instruction* instr_ptr);		//4. Print current background processes

int checkBackground(instruction* instr_ptr);

void clearInstruction(instruction* instr_ptr);		//Clear Instruction

queueEntry* RUNNING_PROCESSES;		//Track pid's of running processes for jobs_builtin
int NUM_OF_PROCESSES = 0;		//Track number of running processes
void addRunningProcess(int pid, char* cmd);	//Add pid to RUNNING_PROCESSES
void deleteRunningProcess(int pid);	//Remove pid from RUNNING_PROCESSES
void checkProcessQueue();
int checkProcess(queueEntry process);
int COMMANDS_EXECUTED;	//Track number of commands executed for exit_builtin

int main() {
	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;
	
	while(1) {
		checkProcessQueue();
		printf("%s@%s:%s>", getenv("USER"), getenv("MACHINE"), getenv("PWD"));	//PROMPT

		//Parse instruction
		parseInput(&instr);
		
		//Expand Variables
		expandVariables(&instr);
		
		//Check for built in		
		//if no built in, check for command
		if(!checkBuiltIn(&instr)) {
			shortcutResolution(&instr);
			pathResolution(&instr);
		}

		//Clear instruction
		clearInstruction(&instr);
		
		++COMMANDS_EXECUTED;
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
void addToken(instruction* instr_ptr, char* tok){
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

void addNull(instruction* instr_ptr){
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**)malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	instr_ptr->tokens[instr_ptr->numTokens] = (char*) NULL;

	//instr_ptr->numTokens++; <-----Why increment the number of tokens for a null token
}

void expandVariables(instruction* instr_ptr) {
	int i;
	char* temp;
	for(i = 0; i < instr_ptr->numTokens; i++)
	{
		if(instr_ptr->tokens[i][0] == '$')
		{
			temp = malloc(strlen(instr_ptr->tokens[i]));
			memcpy(temp, &instr_ptr->tokens[i][1], strlen(instr_ptr->tokens[i]));
			char* envar = getenv(temp);
			if(envar != NULL) {
				instr_ptr->tokens[i] = realloc(instr_ptr->tokens[i], strlen(envar)+1);
				strcpy(instr_ptr->tokens[i], envar);
			}
			else {
				printf("Variable %s does not exist\n",temp);
				instr_ptr->tokens[i] = realloc(instr_ptr->tokens[i], 1);
				strcpy(instr_ptr->tokens[i], "");
			}
			free(temp);
		}
	}
}

void shortcutResolution(instruction* instr_ptr){
	int i,j;
	
	const char* pwd = getenv("PWD");
	const char* home = getenv("HOME");
	
	char* temp;
	
	for(i = 0; i < instr_ptr->numTokens; ++i)
	{
		//Do not convert if token is special symbol
		if(
		instr_ptr->tokens[i][0] == '|' || 
		instr_ptr->tokens[i][0] == '>' || 
		instr_ptr->tokens[i][0] == '<' || 
		instr_ptr->tokens[i][0] == '&') 
		continue;
		
		//Do not convert if token is an absolute path
		if(instr_ptr->tokens[i][0] == '/') continue;

		for(j = 0; j < strlen(instr_ptr->tokens[i]); ++j)
		{
			if(instr_ptr->tokens[i][j] == '/'){
				expandPath(&instr_ptr->tokens[i]);
				break;
			}
		}
	}
}

void expandPath(char** token) {

	const char* pwd = getenv("PWD");
	const char* home = getenv("HOME");

	char* temp;
	
	//If the token starts with '~', replace with $HOME
	if(*token[0] == '~')
	{
		temp = malloc(strlen(home)+strlen(*token)+2);
		strcpy(temp, home);
		strcat(temp, &token[0][1]);
		*token = realloc(*token, strlen(temp)+1);
		strcpy(*token, temp);
		free(temp);
		temp = NULL;
	}

	//Otherwise, expand relative directory into absolute directory
	else
	{
		temp = malloc(strlen(pwd)+strlen(*token)+2);
		strcpy(temp, pwd);
		strcat(temp, "/");
		strcat(temp, *token);
		*token = realloc(*token, strlen(temp)+1);
		strcpy(*token, temp);
		free(temp);
		temp = NULL;
	}
	
	//After expanding tokens as directories, replace dot shortcuts
	//Replace double dot first since single dot is a substring
	//Removed the directory before the double dot
	if((temp = strstr(*token, "/..")) != NULL)
	{
		char* temp2 = &temp[3];
		--temp;
		while(*temp != '/'){--temp;};
		memmove(temp, temp2, strlen(temp2)+1);
	}

	//Simply remove all instances of single dot
	if((temp = strstr(*token, "/.")) != NULL)
	{
		memmove(temp, &temp[2], strlen(temp)-1);
	}
	
}

void pathResolution(instruction* instr_ptr) {
	
	if(strcmp(instr_ptr->tokens[0], "&") == 0){                     //check to see if there's a & at the start of command string
	    int i = 0;
	    for(i; i < instr_ptr->numTokens -1; i++){
	        if(instr_ptr->tokens[i] != NULL){
                    memcpy(instr_ptr->tokens[i], instr_ptr->tokens[i+1], strlen(instr_ptr->tokens[i+1]));
	        }
	    }
                //strcpy(instr_ptr->tokens[i], instr_ptr->tokens[i+1]);     //shift the commands so that there's no leading &
                //instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens-1) * sizeof(char*));
                instr_ptr->numTokens--;
                addNull(instr_ptr);
        }

	if (instr_ptr->tokens[0][0] == '/' && pathIsFile(instr_ptr->tokens[0])) {
		printf("Executing command %s\n",instr_ptr->tokens[0]);
		executeCommand(instr_ptr, 0);
		return;
	}

	int i = 0, j = 0;

	char* checkpaths[32];

	const char* path = getenv("PATH");

	char* envpath = malloc(strlen(path)+1);
    strcpy(envpath, path);

	char* path_split;
	
	int backgroundFlag = checkBackground(instr_ptr);

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
				
				free(envpath);
				
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
                        return;
                    }
                }
            }
        }
    }
    executeCommand(instr_ptr, backgroundFlag);
}

void printTokens(instruction* instr_ptr){
	int i;
	for (i = 1; i < instr_ptr->numTokens; i++) {
		if ((instr_ptr->tokens)[i] != NULL)
		printf("%s ", (instr_ptr->tokens)[i]);
	}
	printf("\n");
}

void clearInstruction(instruction* instr_ptr){
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

void executeCommand(instruction* instr_ptr, int withBackground){
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
        int numPipes = 0;
        for (int i = 0; i < instr_ptr->numTokens; i++) {
            if ((instr_ptr->tokens)[i] != NULL) {
                if (strcmp((instr_ptr->tokens)[i], "|") == 0) {
                    pipeIndex[numPipes] = i;
                    numPipes++;
                }
            }
        }

        int numCommandTokens[3];
        numCommandTokens[0] = pipeIndex[0];
        char **command1 = (char **) malloc((numCommandTokens[0] + 1) * sizeof(char *));
        // put tokens before first pipe in the command1 array
        for (int i = 0; i < numCommandTokens[0]; i++) {
            if ((instr_ptr->tokens)[i] != NULL) {
                command1[i] = malloc(strlen((instr_ptr->tokens)[i] + 1));
                strcpy(command1[i], instr_ptr->tokens[i]);
            }
        }

        command1[numCommandTokens[0]] = NULL;       // set last index of command1 to null

        int startingIndex;
        startingIndex = pipeIndex[0] + 1;
        if (numPipes < 2) {
            numCommandTokens[1] = instr_ptr->numTokens - numCommandTokens[0] - 1;
        }
        else {
            numCommandTokens[1] = pipeIndex[1] - startingIndex;
        }
        char **command2 = (char **) malloc((numCommandTokens[1] + 1) * sizeof(char *));
        int k = 0;
        for (int i = startingIndex; i < startingIndex + numCommandTokens[1]; i++) {
            if ((instr_ptr->tokens)[i] != NULL) {
                command2[k] = malloc(strlen((instr_ptr->tokens)[i] + 1));
                strcpy(command2[k], instr_ptr->tokens[i]);
                k++;
            }
        }

        command2[numCommandTokens[1]] = NULL;       // set last index of command2 to null

        char **command3;
        if (numPipes > 1) {
            startingIndex = pipeIndex[1] + 1;
            numCommandTokens[2] = instr_ptr->numTokens - startingIndex;
            command3 = (char **) malloc((numCommandTokens[2] + 1) * sizeof(char *));
            int k = 0;
            for (int i = startingIndex; i < startingIndex + numCommandTokens[2]; i++) {
                if ((instr_ptr->tokens)[i] != NULL) {
                    command3[k] = malloc(strlen((instr_ptr->tokens)[i] + 1));
                    strcpy(command3[k], instr_ptr->tokens[i]);
                    k++;
                }
            }
            command3[numCommandTokens[2]] = NULL;       // set last index of command3 to null
        }
        else {
            command3 = NULL;
        }


        executePipedCommand(command1, command2, command3);
        free(command1);
        free(command2);
        if (numPipes > 2) {
            free(command3);
        }
        free(pipeIndex);
	return;
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
        if(withBackground == 0){
            //parent
            waitpid(pid, &status, 0);
        }
		else{	//run in the background
			int i, j, len;
			
			for( i = 0; i < instr_ptr->numTokens; ++i) {
				len = len + strlen(instr_ptr->tokens[i]+1);
			}
			char* cmd = (char*)malloc(len * sizeof(char));
			strcpy(cmd, "");
			for(i=0; i < instr_ptr->numTokens;++i) {
				if(strrchr(instr_ptr->tokens[i],'/')!=NULL){
				    strcat(cmd, strrchr(instr_ptr->tokens[i],'/')+1);
				}
				else strcat(cmd, instr_ptr->tokens[i]);
				if(i!=instr_ptr->numTokens-1)
				    strcat(cmd, " ");
			}
			addRunningProcess(pid, cmd);
			printf("[%d]\t[%d]\n", NUM_OF_PROCESSES - 1, RUNNING_PROCESSES[NUM_OF_PROCESSES-1].pid);
			waitpid(pid, &status, WNOHANG);
		}
    }
}

void executePipedCommand(char ** command1, char ** command2, char ** command3) {
    pid_t pid1, pid2, pid3;
    int fd_pipe[2];
    int fd_pipe2[2];
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
    else if (pid1 == 0) {
        // 1st command (writer)

        // output to pipe1
        close(STDOUT_FILENO);
        dup(fd_pipe[1]);
        close(fd_pipe[0]);
        close(fd_pipe[1]);

        if (execv(command1[0], command1) == -1) {
            perror("Error: Command 1 execution failed\n");
            exit(1);
        }
    }
    else {
        if ( command3 != NULL) {
            if (pipe(fd_pipe2) < 0) {
                perror("Error: Piping failed\n");
                exit(1);
            }
        }
        pid2 = fork();
        if(pid2 < 0){
            perror("Error: Forking failed\n");
            exit(1);
        }
        else if (pid2 == 0) {
            // input from pipe1
            close(STDIN_FILENO);
            dup(fd_pipe[0]);
            close(fd_pipe[0]);
            close(fd_pipe[1]);
            if (command3 != NULL) {
                // output to pipe2
                close(STDOUT_FILENO);
                dup(fd_pipe2[1]);
                close(fd_pipe2[0]);
                close(fd_pipe2[1]);
            }

            if (execv(command2[0], command2) == -1) {
                perror("Error: Command 2 execution failed\n");
                exit(1);
            }
        }
        else {
            close(fd_pipe[0]);
            close(fd_pipe[1]);
            waitpid(pid2, &status, 0);
            if (command3 != NULL) {
                pid3 = fork();
                if (pid3 < 0) {
                    perror("Error: Forking failed\n");
                    exit(1);
                }
                else if (pid3 == 0) {
                    // input from pipe2
                    close(STDIN_FILENO);
                    dup(fd_pipe2[0]);
                    close(fd_pipe2[0]);
                    close(fd_pipe2[1]);
                    if (execv(command3[0], command3) == -1) {
                        perror("Error: Command 3 execution failed\n");
                        exit(1);
                    }
                }
                else {
                    close(fd_pipe2[0]);
                    close(fd_pipe2[1]);
                    waitpid(pid3, &status, 0);
                }
            }
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

int checkBuiltIn(instruction* instr_ptr) {
		char* first = instr_ptr->tokens[0];
		if(strcmp(first, "exit")==0) {
			printf("Exiting now!\nCommands Executed: %d\n",COMMANDS_EXECUTED);
			exit_builtin(instr_ptr);
			return 1;
		}
		else if(strcmp(first, "cd")==0) {
			cd_builtin(instr_ptr);
			return 2;
		}
		else if(strcmp(first, "echo")==0) {
			echo_builtin(instr_ptr);
			return 3;
		}
		else if(strcmp(first, "jobs")==0) {
			jobs_builtin(instr_ptr);
			return 4;
		}
		else {
			return 0;
		}
}

void exit_builtin(instruction* instr_ptr) {
	int status;
	while(NUM_OF_PROCESSES>0) {
		waitpid(RUNNING_PROCESSES[0].pid, &status, 0);
		deleteRunningProcess(RUNNING_PROCESSES[0].pid);
	};
	free(RUNNING_PROCESSES);
	exit(1);
}

void cd_builtin(instruction* instr_ptr) {
			
	if(instr_ptr->numTokens > 2) {
		printf("cd : Too many arguments\n");
		return;
	}
	
	if(instr_ptr->numTokens == 1) {
		//No arguments, go to $HOME
		char* home = getenv("HOME");
		chdir(home);
		setenv("PWD", home, 1);
		return;
	}
	
	if(pathIsDir(instr_ptr->tokens[1])) {
		//Go to directory in second token
		expandPath(&instr_ptr->tokens[1]);
		chdir(instr_ptr->tokens[1]);
		setenv("PWD", instr_ptr->tokens[1], 1);
		return;
	}
	
	else if(pathIsFile(instr_ptr->tokens[1])) printf("cd : %s is a file\n",instr_ptr->tokens[1]);
	
	else printf("cd : No directory found at %s\n",instr_ptr->tokens[1]);
}

void echo_builtin(instruction* instr_ptr) {
	int i;
	for (i = 1; i < instr_ptr->numTokens; i++) {
		if ((instr_ptr->tokens)[i] != NULL)
		printf("%s ", (instr_ptr->tokens)[i]);
	}
	printf("\n");
}

void jobs_builtin(instruction* instr_ptr) {
	for(int i = 0; i < NUM_OF_PROCESSES; ++i) {
		printf("%d\n",RUNNING_PROCESSES[i].pid);
	}
}

int checkBackground(instruction* instr_ptr){				//return 1 if there's a & at the end of the command, which tells us this is background command 
    int lastToken = (instr_ptr->numTokens) - 1;
    int i;

    if(strcmp(instr_ptr->tokens[lastToken], "&") == 0){			//this first part of if statement awknowledges that there's background processing, but then removes the & from the cmdl line
	free(instr_ptr->tokens[lastToken]);
        instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens-1) * sizeof(char*));
	instr_ptr->numTokens--;
	addNull(instr_ptr);

	if(strcmp(instr_ptr->tokens[0], "&") == 0){			//check to see if there's a & at the start of command string

	    memmove(instr_ptr->tokens[0], instr_ptr->tokens[1], sizeof(char*));
	    instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens-1) * sizeof(char*));
	    instr_ptr->numTokens--;
	    addNull(instr_ptr);
	}
	return 1;
    }
    else
        return 0;
}

void addRunningProcess(int pid, char* cmd) {
	if(RUNNING_PROCESSES == NULL) {
		RUNNING_PROCESSES = malloc(sizeof(queueEntry));
		RUNNING_PROCESSES[0].pid = pid;
		RUNNING_PROCESSES[0].cmd = cmd;
		NUM_OF_PROCESSES = 1;
	}
	else {
		RUNNING_PROCESSES = realloc(RUNNING_PROCESSES, sizeof(queueEntry)*NUM_OF_PROCESSES+1);
		RUNNING_PROCESSES[NUM_OF_PROCESSES].pid = pid;
		RUNNING_PROCESSES[NUM_OF_PROCESSES].cmd = cmd;
		++NUM_OF_PROCESSES;
	}
}

void deleteRunningProcess(int pid) {
	int i, j = 0;
	for(i = 0; i < NUM_OF_PROCESSES; ++i) {
		if(RUNNING_PROCESSES[i].pid == pid) continue;
		else {
			RUNNING_PROCESSES[j] = RUNNING_PROCESSES[i];
			++j;
		}
	}
	--NUM_OF_PROCESSES;
	RUNNING_PROCESSES = realloc(RUNNING_PROCESSES, NUM_OF_PROCESSES*sizeof(int));
}

void checkProcessQueue(){
	int i = 0;
	for(i; i < NUM_OF_PROCESSES; i++){
			if(checkProcess(RUNNING_PROCESSES[i])){			//if 1 then process is finished
				printf("[%d]+\t[%s]\n", i, RUNNING_PROCESSES[i].cmd);
				deleteRunningProcess(RUNNING_PROCESSES[i].pid);

			}
	}
}

int checkProcess(queueEntry process){
	int status;
	int pid_return;

	pid_return = waitpid(process.pid, &status, WNOHANG);

	if(pid_return == 0)
		return 0;
	else
		return 1;
}

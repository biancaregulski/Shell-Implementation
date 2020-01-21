//COP4610
//Project 1 Starter Code
//example code for initial parsing

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct
{
	char** tokens;
	int numTokens;
} instruction;

void extendTokens(instruction* instr_ptr);			//Path Resolution

void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void addNull(instruction* instr_ptr);
int pathExists(const char* path);
void checkPaths(instruction* instr_ptr);

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

	//Check if first token is an executable command
	checkPaths(instr_ptr);

	for(i = 0; i < instr_ptr->numTokens; i++)
	{
		if ((instr_ptr->tokens)[i] != NULL){
			int y;
			for(y = 0; y < strlen((instr_ptr->tokens[i])); y++)
			{
				if((instr_ptr->tokens)[i][y] != '/'){
					strcpy(relative, getenv("PWD"));
					strcat(relative, "/");
					strcat(relative, (instr_ptr->tokens)[i]);

					if(pathExists(relative)){				// checks if file exists, -1 DNE
						printf("%s\n", relative);
					}
					else {}

					break;
				}
			}
			if(strcmp((instr_ptr->tokens)[i], prevDir) == 0){
				char* temp = malloc(strlen(getenv("PWD")));
				int i, j;
				for(i = strlen(getenv("PWD")); pwd[i] != '/'; i-- ){
					count++;
				}

				for(j = 0; j < strlen(getenv("PWD")) - count; j++){
					temp[j] = pwd[j];
				}
				printf("%s\n", temp);
				free(temp);
			}
			else if(strcmp((instr_ptr->tokens)[i], currDir) == 0){
                                printf("%s\n", getenv("PWD"));
                        }
			else if(strcmp((instr_ptr->tokens)[i], homeDir) == 0){
                                printf("%s\n", getenv("HOME"));
                        }


		}
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

int pathExists(const char* path) {
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
		strcat(checkpaths[k], "/");
		strcat(checkpaths[k], instr_ptr->tokens[0]);
		if(pathExists(checkpaths[k])) {
			printf("Executing command %s\n", instr_ptr->tokens[0]);
			commandFound = 1;
			break;
		}
		
	}
	if(commandFound == 0) printf("%s : No command found\n", instr_ptr->tokens[0]);	
	int d = 0;
	for(d=0;d<20;++d) {
		free(checkpaths[d]);
	}
	free(envpath);
	free(path_split);
}


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
	
	printf("Line1\n");
	char* path_split;
	printf("Line2\n");
	char* path = getenv("PATH");
	printf("Line3\n");
	path_split = strtok(path, ":");
	printf("Line4\n");
	while (path_split != NULL) {
	printf("LINE5: %s\n", path_split);
		strcat(path_split, instr_ptr->tokens[0]);		// ADDS 'd' HERE
	printf("line 6: %s\n", path_split);
		if(pathExists(path_split)) printf("Executing command %s", instr_ptr->tokens[0]);
	printf("lINE 7: %s\n", path_split);
		path_split = strtok(NULL, ":");
	}

	for(i = 0; i < instr_ptr->numTokens; i++)
	{
		if ((instr_ptr->tokens)[i] != NULL){
			for(int y = 0; y < strlen((instr_ptr->tokens)[i]); y++)
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
				for(int i = strlen(getenv("PWD")); pwd[i] != '/'; i-- ){
					count++;
				}

				for(int j = 0; j < strlen(getenv("PWD")) - count; j++){
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

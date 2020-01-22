//COP4610
//Project 1 Starter Code
//example code for initial parsing

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

typedef struct
{
	char** tokens;
	int numTokens;
} instruction;

void parseInput(instruction* instr_ptr);			//Parsing
void addToken(instruction* instr_ptr, char* tok);
void addNull(instruction* instr_ptr);

void shortcutResolution(instruction* instr_ptr);		//Shortcut Resolution

void pathResolution(char* token);				//Path resolution

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

		//Path resolution
		//pathResolution(&instr);

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
		temp = (char*)malloc((strlen(token) + 1) * sizeof(char));

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
	//instr_ptr->numTokens++;
}

void shortcutResolution(instruction* instr_ptr)
{
	int i,j;
	int count = 0;

	char* pwd = getenv("PWD");
	char* relative;
	char* prevDir = "..";
	char* currDir = ".";
	char* homeDir = "~";

	for(i = 0; i < instr_ptr->numTokens; i++)
	{
		//Do not convert if token is an absolute path or a variable
		if(instr_ptr->tokens[i][0] == '/' || instr_ptr->tokens[i][0] == '$') continue;

		for(j = 0; j < strlen(instr_ptr->tokens[i]); j++)
		{
			if((instr_ptr->tokens)[i][j] == '/')
			{
				//A slash is found, token is a relative path
				relative = malloc(strlen(pwd)+strlen(instr_ptr->tokens[i])+2);
				strcpy(relative, pwd);
				strcat(relative, "/");
				strcat(relative, instr_ptr->tokens[i]);
				if(pathExists(relative)){
					instr_ptr->tokens[i] = realloc(instr_ptr->tokens[i], strlen(relative)+1);
					strcpy(instr_ptr->tokens[i], relative);
					free(relative);
				}
				else printf("Path does not exist: %s/%s\n", pwd, instr_ptr->tokens[i]);
				break;
			}
		}

		//Token was not converted to absolute path. Checking if token is a command
		if(j == strlen(instr_ptr->tokens[i])) pathResolution(instr_ptr->tokens[i]);

	}

	for(i = 0; i < instr_ptr->numTokens; i++)
	{
		for(j = 0; j < strlen(instr_ptr->tokens[i]); ++j)
		{
			if(instr_ptr->tokens[i][j] == '.')
			{
				printf("Character %d of token %d is a .\n",j+1,i+1);
			}
			else if(instr_ptr->tokens[i][j] == '~')
			{
				printf("Character %d of token %d is a ~\n",j+1,i+1);
			}
		}
	}
}

void pathResolution(char* token) {
	int i = 0, j = 0;
	int commandFound = 0;

	char* path_split = malloc(128);

	char* checkpaths[32];

	//allocate another string to copy
	char* envpath = malloc(strlen(getenv("PATH"))+1);

	//copy PATH to new variable
	strcpy(envpath,getenv("PATH"));

	//get first token from variable
	strcpy(path_split,strtok(envpath, ":"));

	//split up path into individual directories
	for (j=0; path_split != NULL; ++j) {
		checkpaths[j] = (char*) malloc(strlen(path_split)+strlen(token)+2);
		strcpy(checkpaths[j],path_split);
		path_split = strtok(NULL, ":");
	}

	//check if the program exists in each directory
	for(i=0; i < j; ++i) {
		strcat(checkpaths[i], "/");
		strcat(checkpaths[i], token);
		if(pathExists(checkpaths[i])) {
			printf("Executing command %s\n", token);
			commandFound = 1;
			break;
		}

	}
	if(commandFound == 0) printf("%s : No command found\n", token);

	//free memory
	for(i=0;i<j;++i) {
		free(checkpaths[i]);
	}
	free(path_split);
	free(envpath);
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

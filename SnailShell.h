#ifndef SNAILSHELL_H
#define SNAILSHELL_H

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Default init file
#define SNAILSHELL_INIT "init.txt"

// Arguments
#define ARG_HELP "--help"
#define ARG_INIT "--init-file="

// Error messages
#define ERROR_ARGS_MISSING "Error: missing init file path after argument '-i'.\n"
#define ERROR_ARGS_UNKNOWN "Error: unknown argument %s\n"
#define ERROR_INVALID_VAR_NAME "Error: invalid variable name '%s'.\n"

typedef struct Command {
    char *args[128];
    int argCount;
    char *input;
    char *output;
    int append;
    struct Command *next;
} Command;

// SnailShell.c definitions
void printHelp();

// Parse.c definitions
int isValidVariableName(const char *name);
int handleVariableAssignment(const char *currLine, char *equalSign);
char *replace(const char *arg);
void substitute(Command *command);
Command *parse(const char *currLine);

// Run.c definitions
int handleCD(Command *curr);
void handleInputRedirection(Command *curr, int prevPipe);
void handleOutputRedirection(Command *curr, int fd[2]);
void handlePiping(Command *curr, int fd[2], int *prevPipe);
void printPrompt();
void execute(Command *commands);
int run(FILE *inputStream);

#endif
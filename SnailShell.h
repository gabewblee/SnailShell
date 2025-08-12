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

// System Call Errors
#define ERROR_CWD "Error: cwd failed.\n"
#define ERROR_EXECVP "Error: execvp failed.\n"
#define ERROR_FORK "Error: fork failed.\n"
#define ERROR_PIPE "Error: pipe failed.\n"

// C Library Call Errors
#define ERROR_MALLOC "Error: malloc failed.\n"
#define ERROR_SETENV "Error: setenv failed.\n"
#define ERROR_FOPEN "Error: fopen failed.\n"
#define ERROR_FCLOSE "Error: fclose failed.\n"

// Other Errors
#define ERROR_CD "Error: cd failed.\n"
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
void handleInputRedirection(Command *curr, int prevPipe);
void handleOutputRedirection(Command *curr, int fd[2]);
void handlePiping(Command *curr, int fd[2], int *prevPipe);
void handleCD(Command *curr);
void execute(Command *commands);
void printPrompt();
int run(FILE *inputStream);

#endif
/**
 * @file SnailShell.h
 * @brief Header file for SnailShell - A lightweight command-line shell implementation
 * 
 * This header file contains all the necessary declarations, constants, and data structures
 * for the SnailShell command-line interpreter. SnailShell supports basic shell operations
 * including command execution, piping, input/output redirection, and environment variable
 * management.
 * 
 * Key Features:
 * - Command parsing and execution
 * - Pipeline support (|)
 * - Input/output redirection (<, >, >>)
 * - Environment variable substitution ($VAR)
 * - Variable assignment (VAR=value)
 * - Built-in cd command
 * - Script file execution
 */

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
#define ARG_SCRIPT "--script="

// Max values
#define MAX_NUM_ARGS 128

// Error messages
#define ERROR_ARG_MISSING "Error: missing init file path after argument '-i'.\n"
#define ERROR_ARG_UNKNOWN "Error: unknown argument %s\n"
#define ERROR_VAR_INVALID "Error: invalid variable name '%s'.\n"

/**
 * @struct Command
 * @brief Represents a single command in a pipeline
 * 
 * This structure holds all the information needed to execute a command,
 * including its arguments, input/output redirections, and pipeline linkage.
 */
typedef struct Command {
    char * args[MAX_NUM_ARGS];
    int argCount;
    char * input;
    char * output;
    int append;
    struct Command * next;
} Command;

// SnailShell.c definitions

/**
 * @brief Displays help information for SnailShell
 * 
 * Prints usage instructions and available command-line options
 * to standard output.
 */
void printHelp();

// Parse.c definitions

/**
 * @brief Handles environment variable assignment
 * @param currLine The current input line
 * @param equalSign Pointer to the '=' character in the line
 * @return 0 on success, -1 on failure
 * 
 * Parses and sets environment variables in the format VAR=value.
 * Validates variable names to contain only letters and underscores.
 */
int handleVariableAssignment(const char * currLine, char * equalSign);

/**
 * @brief Replaces environment variable references with their values
 * @param arg The argument string that may contain $VAR references
 * @return New string with variables substituted, or original string if no substitution needed
 * 
 * Handles environment variable substitution. If arg starts with '$',
 * it looks up the environment variable and returns its value.
 * Returns empty string if variable is not found.
 */
char * replace(const char * arg);

/**
 * @brief Performs variable substitution on all arguments of a command
 * @param command Pointer to the Command structure to process
 * 
 * Iterates through all arguments in the command and replaces any
 * environment variable references ($VAR) with their actual values.
 */
void substitute(Command * command);

/**
 * @brief Parses a command line into a linked list of Command structures
 * @param currLine The input line to parse
 * @return Pointer to the head of the command pipeline, or NULL if empty/invalid
 * 
 * Parses a command line string into a linked list of Command structures.
 * Handles:
 * - Command separation by pipes (|)
 * - Input redirection (<)
 * - Output redirection (> and >>)
 * - Environment variable substitution
 * - Variable assignments (VAR=value)
 */
Command * parse(const char * currLine);

// Run.c definitions

/**
 * @brief Handles the built-in cd command
 * @param curr Pointer to the Command structure containing cd arguments
 * @return 0 on success, -1 on failure
 * 
 * Changes the current working directory. If no argument is provided,
 * changes to the user's home directory.
 */
int handleCD(Command * curr);

/**
 * @brief Sets up input redirection for a command
 * @param curr Pointer to the Command structure
 * @param prevPipe File descriptor of the previous pipe's read end
 * 
 * Handles input redirection by either:
 * - Opening the specified input file and redirecting stdin to it
 * - Connecting stdin to the previous pipe's output
 */
void handleInputRedirection(Command * curr, int prevPipe);

/**
 * @brief Sets up output redirection for a command
 * @param curr Pointer to the Command structure
 * @param fd Array containing pipe file descriptors [read, write]
 * 
 * Handles output redirection by either:
 * - Opening the specified output file and redirecting stdout to it
 * - Connecting stdout to the next pipe's input
 */
void handleOutputRedirection(Command * curr, int fd[2]);

/**
 * @brief Manages pipe connections between commands
 * @param curr Pointer to the Command structure
 * @param fd Array containing pipe file descriptors [read, write]
 * @param prevPipe Pointer to the previous pipe's read file descriptor
 * 
 * Creates pipes between commands and manages file descriptor cleanup.
 * Ensures proper connection of command pipelines.
 */
void handlePiping(Command * curr, int fd[2], int * prevPipe);

/**
 * @brief Displays the shell prompt
 * 
 * Prints the current working directory followed by " > " to indicate
 * the shell is ready for input.
 */
void printPrompt();

/**
 * @brief Executes a pipeline of commands
 * @param commands Pointer to the head of the command pipeline
 * 
 * Executes a linked list of commands, handling:
 * - Built-in commands (cd)
 * - External command execution via fork/exec
 * - Pipeline connections
 * - Input/output redirection
 * - Memory cleanup after execution
 */
void execute(Command * commands);

/**
 * @brief Main shell execution loop
 * @param inputStream File stream to read commands from (stdin or file)
 * @return 0 on normal exit, -1 on error
 * 
 * Runs the main shell loop, reading commands from the input stream
 * and executing them. Handles both interactive mode (stdin) and
 * script mode (file input).
 */
int run(FILE * inputStream);

#endif
/**
 * @file Parse.c
 * @brief Command parsing and environment variable handling for SnailShell
 * 
 * This file contains all the parsing logic for SnailShell, including command
 * line parsing, environment variable substitution, and variable assignment
 * handling. It transforms raw input strings into structured Command objects
 * that can be executed by the shell.
 * 
 * Key Functionality:
 * - Command line tokenization and parsing
 * - Pipeline separation (|)
 * - Input/output redirection parsing (<, >, >>)
 * - Environment variable substitution ($VAR)
 * - Variable assignment (VAR=value)
 * - Argument validation and error handling
 */

#include "SnailShell.h"

/**
 * @brief Handles environment variable assignment
 * @param currLine The current input line containing the assignment
 * @param equalSign Pointer to the '=' character in the line
 * @return 0 on success, -1 on failure
 * 
 * Parses and validates environment variable assignments in the format VAR=value.
 * Validates that variable names contain only letters and underscores.
 * Sets the environment variable using setenv() and handles memory allocation.
 * 
 * Memory Management:
 * - Allocates memory for variable name and value
 * - Frees allocated memory on error or completion
 * - Handles memory allocation failures gracefully
 */
int handleVariableAssignment(const char * currLine, char * equalSign) {
    size_t nameLength = equalSign - currLine;
    char * name = strndup(currLine, nameLength);
    if (name == NULL) {
        perror("strndup");
        exit(EXIT_FAILURE);
    }

    char * value = strdup(equalSign + 1);
    if (value == NULL) {
        perror("strdup");
        free(name);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; name[i] != '\0'; i++) {
        if (!isalpha(name[i]) && name[i] != '_') {
            fprintf(stderr, ERROR_VAR_INVALID, name);
            free(name);
            free(value);
            return -1;
        }
    }

    if (setenv(name, value, 1) == -1) {
        perror("setenv");
        free(name);
        free(value);
        return -1;
    }

    free(name);
    free(value);
    return 0;
}

/**
 * @brief Replaces environment variable references with their values
 * @param arg The argument string that may contain $VAR references
 * @return New string with variables substituted, or original string if no substitution needed
 * 
 * Performs environment variable substitution. If the argument starts with '$',
 * it extracts the variable name and looks up its value in the environment.
 * Returns the variable's value if found, or an empty string if not found.
 * 
 * Memory Management:
 * - Allocates new memory for the substituted string
 * - Returns NULL-terminated string that must be freed by caller
 * - Handles memory allocation failures by calling exit()
 */
char * replace(const char * arg) {
    char * newArg = NULL;
    if (arg == NULL || *arg != '$') {
        newArg = strdup(arg);
        if (newArg == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }
        return newArg;
    }

    const char * varName = arg + 1;
    const char * value = getenv(varName);
    if (value == NULL) {
        value = "";
    }

    newArg = strdup(value);
    if (newArg == NULL) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    return newArg;
}

/**
 * @brief Performs variable substitution on all arguments of a command
 * @param command Pointer to the Command structure to process
 * 
 * Iterates through all arguments in the command and replaces any
 * environment variable references ($VAR) with their actual values.
 * Frees the original argument strings and replaces them with the
 * substituted versions.
 * 
 * Memory Management:
 * - Frees original argument strings after substitution
 * - Replaces with new substituted strings
 * - Handles NULL arguments gracefully
 */
void substitute(Command * command) {
    if (command != NULL) {
        for (int i = 0; i < command->argCount; i++) {
            char * newArg = replace(command->args[i]);
            if (command->args[i] != NULL) {
                free(command->args[i]);
            }
            command->args[i] = newArg;
        }
    }
}

/**
 * @brief Parses a command line into a linked list of Command structures
 * @param currLine The input line to parse
 * @return Pointer to the head of the command pipeline, or NULL if empty/invalid
 * 
 * Main parsing function that transforms a command line string into a structured
 * representation suitable for execution. Handles complex shell features including
 * pipelines, redirections, and variable assignments.
 * 
 * Parsing Features:
 * - Splits commands by pipe (|) character
 * - Parses individual command arguments
 * - Handles input redirection (<)
 * - Handles output redirection (> and >>)
 * - Processes variable assignments (VAR=value)
 * - Performs environment variable substitution
 * 
 * Memory Management:
 * - Allocates Command structures and argument strings
 * - Creates a linked list of commands for pipeline execution
 * - Handles memory allocation failures by calling exit()
 * - Caller is responsible for freeing the returned structure
 */
Command * parse(const char * currLine) {
    if (currLine == NULL || strlen(currLine) == 0) {
        return NULL;
    }

    char * equalSign = strchr(currLine, '=');
    if (equalSign != NULL) {
        if (handleVariableAssignment(currLine, equalSign) == -1) {
            fprintf(stderr, "Failed to handle variable assignment\n");
        }
        return NULL;
    }

    char * lineCopy = strdup(currLine);
    if (lineCopy == NULL) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    Command * head = NULL;
    Command * curr = NULL;

    char * tokenPtr;
    char * token = strtok_r(lineCopy, "|", &tokenPtr);
    while (token != NULL) {
        Command * command = malloc(sizeof(Command));
        if (command == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        memset(command, 0, sizeof(Command));
        command->argCount = 0;

        if (head == NULL) {
            head = command;
        } else {
            curr->next = command;
        }

        curr = command;

        char * subtokenPtr;
        char * subtoken = strtok_r(token, " \t", &subtokenPtr);
        while (subtoken != NULL) {
            if (strcmp(subtoken, ">") == 0) {
                subtoken = strtok_r(NULL, " \t", &subtokenPtr);
                if (subtoken != NULL) {
                    command->output = strdup(subtoken);
                    if (command->output == NULL) {
                        perror("strdup");
                        exit(EXIT_FAILURE);
                    }
                    command->append = 0;
                }
            } else if (strcmp(subtoken, ">>") == 0) {
                subtoken = strtok_r(NULL, " \t", &subtokenPtr);
                if (subtoken != NULL) {
                    command->output = strdup(subtoken);
                    if (command->output == NULL) {
                        perror("strdup");
                        exit(EXIT_FAILURE);
                    }
                    command->append = 1;
                }
            } else if (strcmp(subtoken, "<") == 0) {
                subtoken = strtok_r(NULL, " \t", &subtokenPtr);
                if (subtoken != NULL) {
                    command->input = strdup(subtoken);
                    if (command->input == NULL) {
                        perror("strdup");
                        exit(EXIT_FAILURE);
                    }
                }
            } else {
                command->args[command->argCount] = strdup(subtoken);
                if (command->args[command->argCount] == NULL) {
                    perror("strdup");
                    exit(EXIT_FAILURE);
                }
                command->argCount++;
            }
            subtoken = strtok_r(NULL, " \t", &subtokenPtr);
        }
        command->args[command->argCount] = NULL;
        substitute(command);
        token = strtok_r(NULL, "|", &tokenPtr);
    }

    free(lineCopy);
    return head;
}
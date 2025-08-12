#include "SnailShell.h"

int isValidVariableName(const char * name) {
    for (int i = 0; name[i] != '\0'; i++) {
        if (!isalpha(name[i]) && name[i] != '_')
            return -1;
    }
    return 0;
}

int handleVariableAssignment(const char * currLine, char * equalSign) {
    size_t nameLength = equalSign - currLine;
    char * name = strndup(currLine, nameLength);
    if (name == NULL) {
        fprintf(stderr, ERROR_MALLOC);
        exit(EXIT_FAILURE);
    }

    char * value = strdup(equalSign + 1);
    if (value == NULL) {
        fprintf(stderr, ERROR_MALLOC);
        free(name);
        exit(EXIT_FAILURE);
    }

    if (isValidVariableName(name) == -1) {
        fprintf(stderr, ERROR_INVALID_VAR_NAME, name);
        free(name);
        free(value);
        return -1;
    }

    if (setenv(name, value, 1) == -1) {
        fprintf(stderr, ERROR_SETENV);
        free(name);
        free(value);
        return -1;
    }

    free(name);
    free(value);
    return 0;
}

char * replace(const char * arg) {
    char * newArg = NULL;
    if (arg == NULL || *arg != '$')
        newArg = strdup(arg);
        if (newArg == NULL) {
            fprintf(stderr, ERROR_MALLOC);
            exit(EXIT_FAILURE);
        }
        return newArg;

    const char * varName = arg + 1;
    const char * value = getenv(varName);
    if (value == NULL)
        value = "";

    newArg = strdup(value);
    if (newArg == NULL) {
        fprintf(stderr, ERROR_MALLOC);
        exit(EXIT_FAILURE);
    }
    return newArg;
}

void substitute(Command * command) {
    for (int i = 0; i < command->argCount; i++) {
        char * newArg = replace(command->args[i]);
        free(command->args[i]);
        command->args[i] = newArg;
    }
}

Command * parse(const char * currLine) {
    if (currLine == NULL || strlen(currLine) == 0)
        return NULL;

    char * equalSign = strchr(currLine, '=');
    if (equalSign) {
        handleVariableAssignment(currLine, equalSign);
        return NULL;
    }

    char * lineCopy = strdup(currLine);
    if (lineCopy == NULL) {
        fprintf(stderr, ERROR_MALLOC);
        exit(EXIT_FAILURE);
    }

    Command * head = NULL;
    Command * curr = NULL;

    char * tokenPtr;
    char * token = strtok_r(lineCopy, "|", &tokenPtr);
    while (token != NULL) {
        Command * command = malloc(sizeof(Command));
        if (command == NULL) {
            fprintf(stderr, ERROR_MALLOC);
            exit(EXIT_FAILURE);
        }

        memset(command, 0, sizeof(Command));
        command->argCount = 0;

        if (head == NULL)
            head = command;
        else
            curr->next = command;

        curr = command;

        char * subtokenPtr;
        char * subtoken = strtok_r(token, " \t", &subtokenPtr);
        while (subtoken != NULL) {
            if (strcmp(subtoken, ">") == 0) {
                subtoken = strtok_r(NULL, " \t", &subtokenPtr);
                command->output = strdup(subtoken);
                command->append = 0;
            } else if (strcmp(subtoken, ">>") == 0) {
                subtoken = strtok_r(NULL, " \t", &subtokenPtr);
                command->output = strdup(subtoken);
                command->append = 1;
            } else if (strcmp(subtoken, "<") == 0) {
                subtoken = strtok_r(NULL, " \t", &subtokenPtr);
                command->input = strdup(subtoken);
            } else {
                command->args[command->argCount++] = strdup(subtoken);
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
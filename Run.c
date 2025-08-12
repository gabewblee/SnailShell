#include "SnailShell.h"

void handleInputRedirection(Command *curr, int prevPipeRead) {
    if (curr->input) {
        FILE *inputFile = fopen(curr->input, "r");
        if (!inputFile) {
            fprintf(stderr, ERROR_FOPEN);
            exit(EXIT_FAILURE);
        }
        dup2(fileno(inputFile), STDIN_FILENO);
        fclose(inputFile);
    } else if (prevPipeRead != -1) {
        dup2(prevPipeRead, STDIN_FILENO);
        close(prevPipeRead);
    }
}

void handleOutputRedirection(Command *curr, int fd[2]) {
    if (curr->output) {
        FILE *outputFile = fopen(curr->output, curr->append ? "a" : "w");
        if (!outputFile) {
            fprintf(stderr, ERROR_FOPEN);
            exit(EXIT_FAILURE);
        }
        dup2(fileno(outputFile), STDOUT_FILENO);
        fclose(outputFile);
    } else if (curr->next != NULL) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
    }
}

void handlePiping(Command *curr, int fd[2], int *prevPipeRead) {
    if (curr->next != NULL) {
        if (pipe(fd) == -1) {
            fprintf(stderr, ERROR_PIPE);
            exit(EXIT_FAILURE);
        }
    }

    if (*prevPipeRead != -1) {
        close(*prevPipeRead);
    }

    if (curr->next != NULL) {
        close(fd[1]);
    }

    *prevPipeRead = fd[0];
}

void handleCD(Command *curr) {
    const char *targetDir;
    if (curr->argCount > 1) 
        targetDir = curr->args[1];
    else 
        targetDir = getenv("HOME");
        
    if (targetDir == NULL || chdir(targetDir) == -1) {
        fprintf(stderr, ERROR_CD);
    }
}

void executeCommands(Command *commands) {
    Command *curr = commands;
    int fd[2];
    int prevPipeRead = -1;

    while (curr != NULL) {
        if (strcmp(curr->args[0], "cd") == 0) {
            handleCD(curr);
            curr = curr->next;
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            handleInputRedirection(curr, prevPipeRead);
            handleOutputRedirection(curr, fd);

            execvp(curr->args[0], curr->args);
            fprintf(stderr, ERROR_EXECVP);
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            handlePiping(curr, fd, &prevPipeRead);

            int status;
            waitpid(pid, &status, 0);
        } else {
            fprintf(stderr, ERROR_FORK);
            exit(EXIT_FAILURE);
        }

        curr = curr->next;
    }

    if (prevPipeRead != -1)
        close(prevPipeRead);
}

void freeCommands(Command *commands) {
    Command *curr = commands;
    while (curr != NULL) {
        Command *next = curr->next;

        for (int i = 0; i < curr->argCount; i++)
            free(curr->args[i]);

        if (curr->input)
            free(curr->input);
        if (curr->output)
            free(curr->output);

        free(curr);
        curr = next;
    }
}

void printPrompt() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, ERROR_CWD);
        exit(EXIT_FAILURE);
    }
    printf("%s > ", cwd);
}

int run(FILE *inputStream) {
    char *currLine = NULL;
    size_t len = 0;

    if (inputStream == stdin)
        printf("Welcome to SnailShell!\n");

    for (;;) {
        if (inputStream == stdin)
            printPrompt();

        if (getline(&currLine, &len, inputStream) == -1)
            break;

        int end = strcspn(currLine, "\n");
        currLine[end] = '\0';
        Command *commands = parse(currLine);

        if (commands) {
            executeCommands(commands);
            freeCommands(commands);
        }
    }

    free(currLine);
    return 0;
}
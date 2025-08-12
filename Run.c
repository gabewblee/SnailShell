#include "SnailShell.h"

void handleInputRedirection(Command * curr, int prevPipe) {
    if (curr->input) {
        FILE * inputFile = fopen(curr->input, "r");
        if (inputFile == NULL) {
            fprintf(stderr, ERROR_FOPEN);
            exit(EXIT_FAILURE);
        }
        dup2(fileno(inputFile), STDIN_FILENO);
        fclose(inputFile);
    } else {
        if (prevPipe != -1) {
            dup2(prevPipe, STDIN_FILENO);
            close(prevPipe);
        }
    }
}

void handleOutputRedirection(Command * curr, int fd[2]) {
    if (curr->output) {
        FILE * outputFile = fopen(curr->output, curr->append ? "a" : "w");
        if (outputFile == NULL) {
            fprintf(stderr, ERROR_FOPEN);
            exit(EXIT_FAILURE);
        }
        dup2(fileno(outputFile), STDOUT_FILENO);
        fclose(outputFile);
    } else {
        if (curr->next != NULL) {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);
        }
    }
}

void handlePiping(Command * curr, int fd[2], int * prevPipe) {
    if (curr->next != NULL) {
        if (pipe(fd) == -1) {
            fprintf(stderr, ERROR_PIPE);
            exit(EXIT_FAILURE);
        }
    }

    if (*prevPipe != -1) 
        close(*prevPipe);

    if (curr->next != NULL)
        close(fd[1]);

    *prevPipe = fd[0];
}

void handleCD(Command * curr) {
    const char * targetDir;
    if (curr->argCount > 1) 
        targetDir = curr->args[curr->argCount - 1];
    else 
        targetDir = getenv("HOME");
        
    if (targetDir == NULL || chdir(targetDir) == -1)
        fprintf(stderr, ERROR_CD);
}

void execute(Command * commands) {
    Command * curr = commands;
    int fd[2];
    int prevPipe = -1;

    while (curr != NULL) {
        if (strcmp(*curr->args, "cd") == 0) {
            handleCD(curr);
            curr = curr->next;
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            handleInputRedirection(curr, prevPipe);
            handleOutputRedirection(curr, fd);
            execvp(*curr->args, curr->args);
            fprintf(stderr, ERROR_EXECVP);
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            handlePiping(curr, fd, &prevPipe);

            int status;
            waitpid(pid, &status, 0);
        } else {
            fprintf(stderr, ERROR_FORK);
            exit(EXIT_FAILURE);
        }

        curr = curr->next;
    }

    if (prevPipe != -1)
        close(prevPipe);

    Command * dirty = commands;
    while (dirty != NULL) {
        Command * next = dirty->next;

        for (int i = 0; i < dirty->argCount; i++)
            free(dirty->args[i]);

        if (dirty->input)
            free(dirty->input);
        if (dirty->output)
            free(dirty->output);

        free(dirty);
        dirty = next;
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

int run(FILE * inputStream) {
    char * currLine = NULL;
    size_t capacity = 0;

    if (inputStream == stdin)
        printf("Welcome to SnailShell!\n");

    for (;;) {
        if (inputStream == stdin)
            printPrompt();

        if (getline(&currLine, &capacity, inputStream) == -1)
            break;

        int end = strcspn(currLine, "\n");
        currLine[end] = '\0';
        Command * commands = parse(currLine);

        if (commands)
            execute(commands);
    }

    free(currLine);
    return 0;
}
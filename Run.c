#include "SnailShell.h"

static void safeClose(int fd) {
    if (fd != -1 && close(fd) == -1) {
        perror("close");
    }
}

static void safeFree(void * ptr) {
    if (ptr != NULL) {
        free(ptr);
    }
}

int handleCD(Command * curr) {
    const char * targetDir;
    if (curr->argCount > 1) {
        targetDir = curr->args[curr->argCount - 1];
    } else {
        targetDir = getenv("HOME");
    }

    if (targetDir == NULL || chdir(targetDir) == -1) {
        perror("chdir");
        return -1;
    }
    return 0;
}

void handleInputRedirection(Command * curr, int prevPipe) {
    if (curr->input != NULL) {
        FILE * inputFile = fopen(curr->input, "r");
        if (inputFile == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        if (dup2(fileno(inputFile), STDIN_FILENO) == -1) {
            perror("dup2");
            fclose(inputFile);
            exit(EXIT_FAILURE);
        }

        fclose(inputFile);
        return;
    }

    if (prevPipe != -1) {
        if (dup2(prevPipe, STDIN_FILENO) == -1) {
            perror("dup2");
            safeClose(prevPipe);
            exit(EXIT_FAILURE);
        }

        safeClose(prevPipe);
    }
}

void handleOutputRedirection(Command * curr, int fd[2]) {
    if (curr->output != NULL) {
        FILE * outputFile = fopen(curr->output, curr->append ? "a" : "w");
        if (outputFile == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        if (dup2(fileno(outputFile), STDOUT_FILENO) == -1) {
            perror("dup2");
            fclose(outputFile);
            exit(EXIT_FAILURE);
        }

        fclose(outputFile);
        return;
    }

    if (curr->next != NULL) {
        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            safeClose(fd[0]);
            safeClose(fd[1]);
            exit(EXIT_FAILURE);
        }

        safeClose(fd[0]);
        safeClose(fd[1]);
    }
}

void handlePiping(Command * curr, int fd[2], int * prevPipe) {
    if (curr->next != NULL && pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    safeClose(*prevPipe);

    if (curr->next != NULL) {
        safeClose(fd[1]);
        *prevPipe = fd[0];
    } else {
        *prevPipe = -1;
    }
}

void printPrompt() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    printf("%s > ", cwd);
}

void execute(Command * commands) {
    Command * curr = commands;
    int fd[2];
    int prevPipe = -1;

    while (curr != NULL) {
        if (strcmp(*curr->args, "cd") == 0) {
            if (handleCD(curr) == -1) {
                fprintf(stderr, "Failed to change directory\n");
            }
            curr = curr->next;
            continue;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            handleInputRedirection(curr, prevPipe);
            handleOutputRedirection(curr, fd);
            execvp(*curr->args, curr->args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            handlePiping(curr, fd, &prevPipe);

            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
        }

        curr = curr->next;
    }

    safeClose(prevPipe);

    Command * dirty = commands;
    while (dirty != NULL) {
        Command * next = dirty->next;

        for (int i = 0; i < dirty->argCount; i++) {
            safeFree(dirty->args[i]);
        }

        safeFree(dirty->input);
        safeFree(dirty->output);
        safeFree(dirty);

        dirty = next;
    }
}

int run(FILE * inputStream) {
    char * currLine = NULL;
    size_t capacity = 0;

    for (;;) {
        if (inputStream == stdin) {
            printPrompt();
        }

        if (getline(&currLine, &capacity, inputStream) == -1) {
            break;
        }

        int end = strcspn(currLine, "\n");
        currLine[end] = '\0';

        Command * commands = parse(currLine);
        if (commands != NULL) {
            execute(commands);
        }
    }

    free(currLine);
    return 0;
}
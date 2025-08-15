#include "SnailShell.h"

void handleInputRedirection(Command * curr, int prevPipe) {
    if (curr->input) {
        FILE * inputFile = fopen(curr->input, "r");
        if (inputFile == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        if (dup2(fileno(inputFile), STDIN_FILENO) == -1) {
            perror("dup2");
            if (fclose(inputFile) != 0) perror("fclose");
            exit(EXIT_FAILURE);
        }

        if (fclose(inputFile) != 0) {
            perror("fclose");
            exit(EXIT_FAILURE);
        }
    } else {
        if (prevPipe != -1) {
            if (dup2(prevPipe, STDIN_FILENO) == -1) {
                perror("dup2");
                if (close(prevPipe) == -1) perror("close");
                exit(EXIT_FAILURE);
            }

            if (close(prevPipe) == -1) {
                perror("close");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void handleOutputRedirection(Command * curr, int fd[2]) {
    if (curr->output) {
        FILE * outputFile = fopen(curr->output, curr->append ? "a" : "w");
        if (outputFile == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        if (dup2(fileno(outputFile), STDOUT_FILENO) == -1) {
            perror("dup2");
            if (fclose(outputFile) != 0) perror("fclose");
            exit(EXIT_FAILURE);
        }

        if (fclose(outputFile) != 0) {
            perror("fclose");
            exit(EXIT_FAILURE);
        }
    } else {
        if (curr->next != NULL) {
            if (dup2(fd[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                if (close(fd[0]) == -1) perror("close");
                if (close(fd[1]) == -1) perror("close");
                exit(EXIT_FAILURE);
            }

            if (close(fd[0]) == -1 || close(fd[1]) == -1) {
                perror("close");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void handlePiping(Command * curr, int fd[2], int * prevPipe) {
    if (curr->next != NULL) {
        if (pipe(fd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    if (*prevPipe != -1) {
        if (close(*prevPipe) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }

    if (curr->next != NULL) {
        if (close(fd[1]) == -1) { 
            perror("close");
            exit(EXIT_FAILURE);
        }
        *prevPipe = fd[0];
    } else {
        *prevPipe = -1;
    }
}

int handleCD(Command * curr) {
    const char * targetDir;
    if (curr->argCount > 1) targetDir = curr->args[curr->argCount - 1];
    else targetDir = getenv("HOME");

    if (targetDir == NULL) return -1;

    if (chdir(targetDir) == -1) {
        perror("chdir");
        return -1;
    }

    return 0;
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
            int ret = handleCD(curr);
            if (ret == -1) fprintf(stderr, "Failed to change directory\n");
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

    if (prevPipe != -1) {
        if (close(prevPipe) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }

    Command * dirty = commands;
    while (dirty != NULL) {
        Command * next = dirty->next;

        for (int i = 0; i < dirty->argCount; i++)
            free(dirty->args[i]);

        if (dirty->input) free(dirty->input);
        if (dirty->output) free(dirty->output);

        free(dirty);
        dirty = next;
    }
}

int run(FILE * inputStream) {
    char * currLine = NULL;
    size_t capacity = 0;

    if (inputStream == stdin)
        printf("Welcome to SnailShell!\n");

    for (;;) {
        if (inputStream == stdin) printPrompt();
        if (getline(&currLine, &capacity, inputStream) == -1) break;

        int end = strcspn(currLine, "\n");
        currLine[end] = '\0';
        Command * commands = parse(currLine);

        if (commands) execute(commands);
    }

    free(currLine);
    return 0;
}
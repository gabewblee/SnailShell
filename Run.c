/**
 * @file Run.c
 * @brief Command execution and shell runtime functionality for SnailShell
 * 
 * This file contains the core execution logic for SnailShell, including the main
 * shell loop, command execution, process management, and I/O redirection handling.
 * It manages the lifecycle of commands from parsing to execution and cleanup.
 * 
 * Key Functionality:
 * - Main shell execution loop
 * - Process creation and management (fork/exec)
 * - Pipeline execution and inter-process communication
 * - Input/output redirection handling
 * - Built-in command support (cd)
 * - File descriptor management and cleanup
 * - Memory management for command structures
 */

#include "SnailShell.h"

/**
 * @brief Safely closes a file descriptor with error handling
 * @param fd File descriptor to close
 * 
 * Wrapper function for close() that handles errors gracefully.
 * Only attempts to close valid file descriptors and reports errors
 * without terminating the program.
 */
static void safeClose(int fd) {
    if (fd != -1 && close(fd) == -1) {
        perror("close");
    }
}

/**
 * @brief Safely frees memory with NULL pointer checking
 * @param ptr Pointer to memory to free
 * 
 * Wrapper function for free() that checks for NULL pointers before
 * attempting to free memory, preventing segmentation faults.
 */
static void safeFree(void * ptr) {
    if (ptr != NULL) {
        free(ptr);
    }
}

/**
 * @brief Handles the built-in cd command
 * @param curr Pointer to the Command structure containing cd arguments
 * @return 0 on success, -1 on failure
 * 
 * Implements the built-in cd (change directory) command. If no argument
 * is provided, changes to the user's home directory. Validates the target
 * directory and reports errors if the directory change fails.
 * 
 * Behavior:
 * - Uses last argument as target directory
 * - Falls back to HOME environment variable if no argument
 * - Reports errors via perror() on failure
 */
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

/**
 * @brief Sets up input redirection for a command
 * @param curr Pointer to the Command structure
 * @param prevPipe File descriptor of the previous pipe's read end
 * 
 * Configures input redirection for a command by either:
 * - Opening a specified input file and redirecting stdin to it
 * - Connecting stdin to the previous command's output through a pipe
 * 
 * Redirection Priority:
 * - File redirection (<) takes precedence over pipe redirection
 * - Handles file open failures by calling exit()
 * - Manages file descriptor duplication and cleanup
 */
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

/**
 * @brief Sets up output redirection for a command
 * @param curr Pointer to the Command structure
 * @param fd Array containing pipe file descriptors [read, write]
 * 
 * Configures output redirection for a command by either:
 * - Opening a specified output file and redirecting stdout to it
 * - Connecting stdout to the next command's input through a pipe
 * 
 * Redirection Modes:
 * - Truncate mode (>): Overwrites existing file
 * - Append mode (>>): Appends to existing file
 * - Pipe mode: Connects to next command in pipeline
 * 
 * File Descriptor Management:
 * - Handles file open failures by calling exit()
 * - Manages file descriptor duplication and cleanup
 * - Ensures proper pipe connection for pipeline execution
 */
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

/**
 * @brief Manages pipe connections between commands
 * @param curr Pointer to the Command structure
 * @param fd Array containing pipe file descriptors [read, write]
 * @param prevPipe Pointer to the previous pipe's read file descriptor
 * 
 * Creates and manages pipes between commands in a pipeline. Handles
 * file descriptor cleanup and ensures proper connection of command
 * pipelines for data flow between processes.
 * 
 * Pipeline Management:
 * - Creates new pipes for multi-command pipelines
 * - Closes unused file descriptors to prevent leaks
 * - Maintains pipe connections between sequential commands
 * - Handles pipe creation failures by calling exit()
 */
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

/**
 * @brief Displays the shell prompt
 * 
 * Prints the current working directory followed by " > " to indicate
 * the shell is ready for user input. Uses getcwd() to determine the
 * current directory and formats it for display.
 * 
 * Error Handling:
 * - Reports getcwd() failures via perror()
 * - Calls exit() on critical errors to prevent shell corruption
 */
void printPrompt() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    printf("%s > ", cwd);
}

/**
 * @brief Executes a pipeline of commands
 * @param commands Pointer to the head of the command pipeline
 * 
 * Main execution function that processes a linked list of commands.
 * Handles both built-in commands and external program execution,
 * manages process creation, and ensures proper cleanup.
 * 
 * Execution Flow:
 * - Processes commands sequentially in the pipeline
 * - Handles built-in commands (cd) directly
 * - Creates child processes for external commands
 * - Manages input/output redirection for each command
 * - Waits for child processes to complete
 * - Performs comprehensive memory cleanup
 * 
 * Process Management:
 * - Uses fork() to create child processes
 * - Uses execvp() to execute external commands
 * - Uses waitpid() to wait for child completion
 * - Handles process creation failures by calling exit()
 * 
 * Memory Management:
 * - Frees all allocated Command structures
 * - Frees all argument strings and redirection paths
 * - Ensures no memory leaks after execution
 */
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

/**
 * @brief Main shell execution loop
 * @param inputStream File stream to read commands from (stdin or file)
 * @return 0 on normal exit, -1 on error
 * 
 * Core shell loop that reads commands from the input stream and executes them.
 * Supports both interactive mode (reading from stdin) and script mode (reading
 * from a file). Handles command parsing, execution, and error reporting.
 * 
 * Loop Behavior:
 * - Displays prompt only in interactive mode (stdin)
 * - Reads commands line by line using getline()
 * - Parses each line into Command structures
 * - Executes parsed commands via execute()
 * - Continues until EOF or error condition
 * 
 * Input Handling:
 * - Uses getline() for dynamic line buffer allocation
 * - Removes newline characters from input
 * - Handles EOF gracefully by breaking the loop
 * - Manages memory for dynamically allocated line buffers
 * 
 * Error Handling:
 * - Reports parsing and execution errors
 * - Continues execution on non-fatal errors
 * - Ensures proper cleanup on exit
 */
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
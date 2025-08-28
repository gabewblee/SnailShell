/**
 * @file SnailShell.c
 * @brief Main entry point and command-line argument handling for SnailShell
 * 
 * This file contains the main function and command-line argument parsing logic
 * for the SnailShell command-line interpreter. It handles program initialization,
 * argument processing, and delegates execution to the appropriate input source
 * (interactive stdin or script file).
 * 
 * Command-line Options:
 * - -h, --help: Display help information
 * - -s <file>, --script=<file>: Execute commands from specified script file
 */

#include "SnailShell.h"

/**
 * @brief Displays help information for SnailShell
 * 
 * Prints comprehensive usage instructions and available command-line options
 * to standard output. This function is called when the user requests help
 * via the -h or --help command-line arguments.
 */
void printHelp() {
    printf("SnailShell\n");
    printf("Usage: SnailShell [OPTIONS]\n");
    printf("Options:\n");
    printf("    -h, --help                              Show this help message\n");
    printf("    -s <file>, --script=<file>              Specify an script file\n");
}

/**
 * @brief Main entry point for SnailShell
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @return 0 on successful execution, -1 on error
 * 
 * Processes command-line arguments and initializes the shell. Supports
 * both interactive mode (no arguments) and script mode (with -s option).
 * Handles help requests and delegates execution to the run() function.
 */
int main(int argc, char * argv[]) {
    const char * scriptPath = NULL;

    for (int i = 1; i < argc; i++) {
        const char * arg = argv[i];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, ARG_HELP) == 0) {
            printHelp();
            return 0;
        } else if (strcmp(arg, "-s") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, ERROR_ARG_MISSING);
                return -1;
            }
            scriptPath = argv[++i];
        } else if (strncmp(arg, ARG_SCRIPT, strlen(ARG_SCRIPT)) == 0) {
            scriptPath = arg + strlen(ARG_SCRIPT);
        } else {
            fprintf(stderr, ERROR_ARG_UNKNOWN, arg);
            return -1;
        }
    }

    if (scriptPath) {
        FILE * scriptFile = fopen(scriptPath, "r");
        if (scriptFile == NULL) {
            perror("fopen");
            return -1;
        }

        int ret = run(scriptFile);
        if (fclose(scriptFile) != 0) {
            perror("fclose");
            return -1;
        }
        
        return ret;
    } else {
        return run(stdin);
    }
}
#include "SnailShell.h"

void printHelp() {
    printf("SnailShell\n");
    printf("Usage: SnailShell [OPTIONS]\n");
    printf("Options:\n");
    printf("    -h, --help                              Show this help message\n");
    printf("    -s <file>, --script=<file>              Specify an script file\n");
}

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
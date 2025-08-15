#include "SnailShell.h"

void printHelp() {
    printf("SnailShell\n");
    printf("Usage: SnailShell [OPTIONS]\n");
    printf("Options:\n");
    printf("    -h, --help                              Show this help message\n");
    printf("    -i <file>, --init-file=<file>           Specify an init file\n");
}

int main(int argc, char * argv[]) {
    const char * initPath = NULL;

    for (int i = 1; i < argc; i++) {
        const char * arg = argv[i];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, ARG_HELP) == 0) {
            printHelp();
            return 0;
        } else if (strcmp(arg, "-i") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, ERROR_ARGS_MISSING);
                return -1;
            }
            initPath = argv[++i];
        } else if (strncmp(arg, ARG_INIT, strlen(ARG_INIT)) == 0) {
            initPath = arg + strlen(ARG_INIT);
        } else {
            fprintf(stderr, ERROR_ARGS_UNKNOWN, arg);
            return -1;
        }
    }

    if (initPath) {
        FILE * initFile = fopen(initPath, "r");
        if (initFile == NULL) {
            perror("fopen");
            return -1;
        }

        int ret = run(initFile);
        if (fclose(initFile) != 0) {
            perror("fclose");
            return -1;
        }
        
        return ret;
    } else {
        return run(stdin);
    }
}
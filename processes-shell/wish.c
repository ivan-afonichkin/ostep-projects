#include <signal.h>
#include "wish.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_null.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>

int const MAX_TOKENS = 255;
char *DEFAULT_PATH = "/bin/";
int const MAX_PATH_LEN = 256;


char **init_search_paths() {
    char **paths = (char **) malloc(2 * sizeof(char *));
    paths[0] = DEFAULT_PATH;
    paths[1] = NULL;
    return paths;
}

bool is_builtin_command(char *command) {
    return false;
}

void execute_command(char *tokens[], char **searchPaths) {
    /**
     * tokens is a set of tokens that should be terminated by NULL characters.
     */

    if (tokens[0] == NULL) {
        fprintf(stderr, "Empty input command\n");
        return;
    }

    if (is_builtin_command(tokens[0])) {
    
    } else {
        // It's not a buil-in command and we should check if it's executable by default.
        if (0 == access(tokens[0], X_OK)) {
            // If we have access, we can just go directory and execute it.
            execute_external_command(tokens[0], tokens);
        } else {
            // This might be a in our search paths, so let's check it.
            char *curSearchPath = NULL;
            int idx = 0;
            char buffer[MAX_PATH_LEN] = {0};
            while ((curSearchPath = searchPaths[idx++]) != NULL) {
                strlcpy(buffer, curSearchPath, MAX_PATH_LEN);
                strlcat(buffer, tokens[0], MAX_PATH_LEN);
                printf("Checking executable at: %s\n", buffer);
                if (0 == access(buffer, X_OK)) {
                    tokens[0] = buffer;
                    execute_external_command(tokens[0], tokens);
                }
            }
        }
    }

}

void execute_external_command(const char *executablePath, char *const argv[]) {
    /**
     * Executes command `executablePath` with arguments `argv` in a new process and returns process PID of that process.
     * argv should be terminated with NULL.
     * List of searchable paths is defined in `searchPaths`.
     */
    pid_t processId = fork();
    if (processId == 0) {
        // This is a child process.
        // We communicate with it through the opened file descriptors.
        // In this case we don't overwrite any existing file descriptors.
        if (!execv(executablePath, argv)) {
            fprintf(stderr, "Failed to run execv!\n");
        }
    } else {
        // This is a main (parent process).
        int stat_loc = 0; 
        // This is a blocking call until the process is finished.
        waitpid(processId, &stat_loc, 0);
        if (!WIFEXITED(stat_loc)) {
            if (WIFSIGNALED(stat_loc)) {
                fprintf(stderr, "Process with %d PID terminated due to getting a signal %d\n", processId, WTERMSIG(stat_loc));
            }

            fprintf(stderr, "Process with %d PID didnot exit successfully.", processId);
        }
    }
};

int main(int argc, char *argv[]) {
    FILE *stream = stdin;
    bool verbose = true;

    char **searchPaths = init_search_paths();

    if (argc != 1) {
        if (argc > 2) {
            fprintf(stderr, "Only one input file is supported\n");
            exit(1);
        }

        stream = fopen(argv[1], "r");
        if (stream == NULL) {
            fprintf(stderr, "Cannot open input file %s\n", argv[1]);
            exit(1);
        }
    }

    char *buffer = NULL;
    size_t bufferSize = 0;
    int bytesRead = 0;
    char *tokens[MAX_TOKENS];
    while (true) {
        // Read input line
        if (-1 == (bytesRead = getline(&buffer, &bufferSize, stream))) {
            // EOF is reached.
            return 0;
        }
        buffer[bytesRead - 1] = '\0';
        if (verbose) {
            printf("Input buffer: %s\n", buffer);
        }
        char *bufferToFree = buffer;
        int nTokens = 0;
        while ((tokens[nTokens++] = strsep(&buffer, " ")) != NULL);
        execute_command(tokens, searchPaths);

        free(bufferToFree);
    }
    return 0;
}

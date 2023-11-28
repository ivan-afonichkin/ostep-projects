#include <ctype.h>
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
int const N_BUILTIN_COMMANDS = 3;
int const N_MAX_SEARCH_PATHS = 255;
char * const BUILTIN_COMMANDS[N_BUILTIN_COMMANDS] = {"exit", "cd", "path"};
char * const ERROR_MESSAGE = "An error has occurred\n";
char * const WHITESPACES = " \t\n\r";
int const MAX_EXTERNAL_COMMANDS = 8;


char *ltrim(char *s) {
    while (isspace(*s)) {
        s++;
    }

    return s;
}

char *rtrim(char *s) {
    char *last_c = s + strlen(s) - 1;
    while (isspace(*last_c)) {
        last_c--;
    }
    *(last_c+1) = '\0';
    return s;
}

char **init_search_paths() {
    char **paths = (char **) malloc(N_MAX_SEARCH_PATHS * sizeof(char *));
    paths[0] = DEFAULT_PATH;
    paths[1] = NULL;
    return paths;
}

void execute_cd(char *tokens[]) {
    if (chdir(tokens[1]) != 0) {
        fprintf(stderr, "%s", ERROR_MESSAGE);
    }
}

void execute_exit(char *tokens[]) {
    if (tokens[1] != NULL) {
        fprintf(stderr, "%s", ERROR_MESSAGE);
    }
    exit(0);
}

void execute_path(char **tokens, char **existingSearchPaths) {
    /**
     * Input tokens look like {"path", "/bin", "/bin/ls", NULL}, so we just move by 1 pointer.
     * */
    int idx = 0;
    while (tokens[idx+1] != NULL) {
        int bufferSize = strlen(tokens[idx + 1]) + 1;
        char *buffer = malloc(bufferSize);
        strlcpy(buffer, tokens[idx + 1], bufferSize);
        existingSearchPaths[idx] = buffer;
        idx += 1;
    }
    existingSearchPaths[idx] = NULL;
}

bool is_builtin_command(char *command) {
    for (int i = 0; i < N_BUILTIN_COMMANDS; ++i) {
        if (0 == strcmp(command, BUILTIN_COMMANDS[i])) {
            return true;
        }
    }
    return false;
}

void execute_command(char *tokens[], char **searchPaths) {
    /**
     * tokens is a set of tokens that should be terminated by NULL characters.
     */

    if (tokens[0] == NULL || 0 == strcmp(tokens[0], "")) {
        // Empty input command
        return;
    }

    if (is_builtin_command(tokens[0])) {
        if (0 == strcmp(tokens[0], "cd")) {
            execute_cd(tokens);
        } else if (0 == strcmp(tokens[0], "exit")) {
            execute_exit(tokens);
        } else if (0 == strcmp(tokens[0], "path")) {
            execute_path(tokens, searchPaths);
        } else {
            fprintf(stderr, "%s", ERROR_MESSAGE);
        }
    } else {
        char **beginningTokens = tokens;
        pid_t processPids[MAX_EXTERNAL_COMMANDS];
        int nPids = 0;
        for (int idx = 0; tokens[idx] != NULL; ++idx) {
            if (0 == strcmp(tokens[idx], "&")) {
                tokens[idx] = NULL;
                if (beginningTokens[0] != NULL) {
                    processPids[nPids++] = execute_external_command(beginningTokens, searchPaths, false);
                    beginningTokens = (tokens + idx + 1);
                }
            }
        }
        if (beginningTokens[0] != NULL) {
            processPids[nPids++] = execute_external_command(beginningTokens, searchPaths, false);
        }

        for (int idx = 0; idx < nPids; ++idx) {
            int stat_loc = 0; 
            if (processPids[idx] == -1) continue;
            waitpid(processPids[idx], &stat_loc, 0);
            if (!WIFEXITED(stat_loc)) {
                fprintf(stderr, "%s", ERROR_MESSAGE);
            }
        }

    }

}

bool contains_redirection(char *tokens[]) {
    char *curToken = NULL;
    int idx = 0;
    while ((curToken = tokens[idx++]) != NULL) {
        if (0 == strcmp(curToken, ">")) {
            return true; 
        }
    }

    return false;
}

char *get_redirection_file_path(char *tokens[]) {
    char *curToken = NULL;
    int idx = 0;
    while ((curToken = tokens[idx++]) != NULL) {
        if (0 == strcmp(curToken, ">")) {
            tokens[idx - 1] = NULL;
            return tokens[idx];
        }
    }

    return NULL; 
}

bool is_correct_redirection(char *tokens[]) {
    char *curToken = NULL;
    int idx = 0;
    while ((curToken = tokens[idx++]) != NULL) {
        if (0 == strcmp(curToken, ">")) {
            if (tokens[idx + 1] != NULL) {
                return false;
            }
        }
    }

    return true; 
}

pid_t execute_external_command(char *tokens[], char **searchPaths, bool blocking) {
    /**
     * Executes command `executablePath` with arguments `argv` in a new process and returns process PID of that process.
     * argv should be terminated with NULL.
     * List of searchable paths is defined in `searchPaths`.
     */

    if (0 != access(tokens[0], X_OK)) {
        // This might be a in our search paths, so let's check it.
        char *curSearchPath = NULL;
        int idx = 0;
        bool found = false;
        char buffer[MAX_PATH_LEN] = {0};
        while ((curSearchPath = searchPaths[idx++]) != NULL) {
            strlcpy(buffer, curSearchPath, MAX_PATH_LEN);
            strlcat(buffer, tokens[0], MAX_PATH_LEN);
            // printf("Checking executable at: %s\n", buffer);
            if (0 == access(buffer, X_OK)) {
                tokens[0] = buffer;
                found = true;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "%s", ERROR_MESSAGE);
            return -1;
        }
    }

    char *executablePath = tokens[0];
    char **argv = tokens;

    pid_t processId = fork();
    if (processId == 0) {
        // This is a child process.
        // We communicate with it through the opened file descriptors.
        // In this case we don't overwrite any existing file descriptors.
        
        if (contains_redirection(tokens)) {
            if (!is_correct_redirection(tokens)) {
                fprintf(stderr, "%s", ERROR_MESSAGE);
                return -1;
            }
            char *redirectionPath = get_redirection_file_path(tokens);
            if (redirectionPath == NULL) {
                fprintf(stderr, "%s", ERROR_MESSAGE);
                return -1;
            }
            fclose(stdout);
            FILE *newStdout = fopen(redirectionPath, "w");
            if (newStdout == NULL) {
                fprintf(stderr, "%s", ERROR_MESSAGE);
                return -1;
            }
            fclose(stderr);
            FILE *newStderr = fopen(redirectionPath, "w");
            if (newStderr == NULL) {
                fprintf(stderr, "%s", ERROR_MESSAGE);
                return -1;
            }
        }

        if (!execv(executablePath, argv)) {
            fprintf(stderr, "%s", ERROR_MESSAGE);
            return -1;
        }
    } else {
        // This is a main (parent process).
        // This is a blocking call until the process is finished.
        if (blocking) {
            int stat_loc = 0; 
            waitpid(processId, &stat_loc, 0);
            if (!WIFEXITED(stat_loc)) {
                fprintf(stderr, "%s", ERROR_MESSAGE);
            }
        }
        return processId;
    }
    return -1;
};

int main(int argc, char *argv[]) {
    FILE *stream = stdin;
    bool verbose = true;

    char **searchPaths = init_search_paths();

    if (argc != 1) {
        if (argc > 2) {
            fprintf(stderr, "%s", ERROR_MESSAGE);
            exit(1);
        }

        stream = fopen(argv[1], "r");
        if (stream == NULL) {
            fprintf(stderr, "%s", ERROR_MESSAGE);
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
        char *bufferToFree = buffer;
        buffer[bytesRead - 1] = '\0';

        buffer = ltrim(rtrim(buffer));

        if (verbose) {
            // printf("Input buffer: %s\n", buffer);
        }
        int nTokens = 0;
        char *curToken = NULL;
        while ((curToken = strsep(&buffer, WHITESPACES)) != NULL) {
            if (0 == strcmp(curToken, "")) {
                continue;
            }
            tokens[nTokens++] = curToken;
        }
        tokens[nTokens] = NULL;

        execute_command(tokens, searchPaths);

        free(bufferToFree);
    }
    return 0;
}

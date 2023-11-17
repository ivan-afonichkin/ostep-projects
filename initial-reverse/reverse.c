#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

typedef struct Node {
    struct Node *next;
    char *line;
} node_t;

node_t *init(node_t *next, char *line) {
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    new_node->next = next;
    new_node->line = line;
    return new_node;
}

node_t *push(node_t *node, char *line) {
    node_t *new_node = init(node->next, line);
    node->next = new_node;
    return new_node;
}

void print_list(FILE *f, node_t *root) {
    node_t *node = root->next;
    while (node != NULL) {
        fprintf(f, "%s", node->line);
        node = node->next;
    }
}

bool same_files(char *f1, char *f2) {
    // In case it's the same path.
    if (strcmp(f1, f2) == 0) {
        return true;
    }

    // In case they might be hardlinked.
    struct stat *buf_f1 = (struct stat *)malloc(sizeof(struct stat));
    struct stat *buf_f2 = (struct stat *)malloc(sizeof(struct stat));
    stat(f1, buf_f1);
    stat(f2, buf_f2);
    if (buf_f1->st_ino == buf_f2->st_ino && buf_f1->st_dev == buf_f2->st_dev) {
        return true;
    }

    return false;
}

int main (int argc, char *argv[]) {
    if (argc > 3) {
        fprintf(stderr, "usage: reverse <input> <output>\n");
        exit(1);
    }

    FILE *inputFile = stdin;
    if (argc > 1) {
        inputFile = fopen(argv[1], "r");
        if (inputFile == NULL) {
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
            exit(1);
        }
    }
    
    FILE *outputFile = stdout;
    if (argc > 2) {
        outputFile = fopen(argv[2], "w");
        if (outputFile == NULL) {
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[2]);
            exit(1);
        }
    }

    if (argc == 3 && same_files(argv[1], argv[2])) {
        fprintf(stderr, "reverse: input and output file must differ\n");
        exit(1);
    }

    char *buffer = NULL;
    size_t bufferSize = 0;
    ssize_t bytesRead = 0;
    node_t *root = init(NULL, NULL);

    for (;;) {
        buffer = NULL;
        bytesRead = getline(&buffer, &bufferSize, inputFile);
        if (bytesRead == -1) {
            if (errno == ENOMEM) {
                fprintf(stderr, "malloc failed\n");
                exit(1);
            }

            break;
        }

        push(root, buffer);
    }

    print_list(outputFile, root);
    return 0;
}
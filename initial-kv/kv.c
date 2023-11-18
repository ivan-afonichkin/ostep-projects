#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>


char const *DB_PATH = "database.txt";
const int N_BUCKETS = 1000;

typedef struct Node {
    struct Node *next;
    char *key;
    char *value;
} node_t;

unsigned int hash(char const *str) {
    const unsigned int primeNumber = 31;
    unsigned int curValue = 0;
    int i = 0;
    while (str[i] != '\0') {
        curValue = curValue * primeNumber + str[i++];
    }

    return curValue;
}


node_t *create_node(node_t *next, char *key, char *value) {
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    new_node->next = next;
    new_node->key = key;
    new_node->value = value;
    return new_node;
}

typedef struct DB {
    node_t *buckets[N_BUCKETS];
} db_t;


node_t *db_get(db_t *db, char const *key) {
    int keyHash = hash(key) % N_BUCKETS;
    node_t *curNode = db->buckets[keyHash];
    while (curNode != NULL) {
        if (0 == strcmp(curNode->key, key)) {
            return curNode;
        }
        curNode = curNode->next;
    }

    return NULL;
}

void db_put(db_t *db, char *key, char *value) {
    node_t *node = db_get(db, key);
    if (node != NULL) {
        // Overwriting existing value.
        node->value = value;
    } else {
        unsigned int keyHash = hash(key) % N_BUCKETS;
        db->buckets[keyHash] = create_node(db->buckets[keyHash], key, value);
    }
}

void free_node(node_t *node) {
    free(node);
}

void db_delete(db_t *db, char const *key) {
    int keyHash = hash(key) % N_BUCKETS;
    node_t *curNode = db->buckets[keyHash];
    if (curNode == NULL) {
        fprintf(stdout, "%s not found\n", key);
        return;
    }

    if (0 == strcmp(curNode->key, key)) {
        db->buckets[keyHash] = curNode->next;
        free_node(curNode);
        return;
    }

    while (curNode != NULL && curNode->next != NULL) {
        if (0 == strcmp(curNode->next->key, key)) {
            // next node has the key.
            curNode->next = curNode->next->next;
            free_node(curNode->next);
            return;
        }
        curNode = curNode->next;
    }

    fprintf(stdout, "%s not found\n", key);
}

void clear_node_chain(node_t *node) {
    if (node == NULL) {
        return;
    }

    if (node->next != NULL) {
        clear_node_chain(node->next);
    }

    free_node(node);
}

void db_clear(db_t *db) {
    for (int bucketIdx = 0; bucketIdx < N_BUCKETS; ++bucketIdx) {
        clear_node_chain(db->buckets[bucketIdx]);
        db->buckets[bucketIdx] = NULL;
    }
}

void db_all(db_t const *db, FILE *output) {
    for (int bucket_idx = 0; bucket_idx < N_BUCKETS; ++bucket_idx) {
        node_t *bucket = db->buckets[bucket_idx];
        while (bucket != NULL) {
            fprintf(output, "%s,%s\n", bucket->key, bucket->value);
            bucket = bucket->next;
        }
    }
}

void process_command(db_t *db, char *cmd_arguments) {
    int curIdx = 0;
    char *args[3];
    char *token = NULL;

    while ((token = strsep(&cmd_arguments, ",")) != NULL) {
        args[curIdx++] = token;
    }

    char cmd = args[0][0];
    if (cmd == 'g') {
        node_t *node = db_get(db, args[1]);
        if (node != NULL) {
            fprintf(stdout, "%s,%s\n", node->key, node->value);
        } else {
            fprintf(stdout, "%s not found\n", args[1]);
        }
    } else if (cmd == 'p') {
        db_put(db, args[1], args[2]);
    } else if (cmd == 'a') {
        db_all(db, stdout);
    } else if (cmd == 'd') {
        db_delete(db, args[1]);
    } else if (cmd == 'c') {
        db_clear(db);
    }
}

db_t *load_database(char const *path) {
    db_t *db = (db_t *)malloc(sizeof(db_t));
    FILE *f = fopen(path, "r");

    if (f == NULL) {
        // In this case database doesn't exist yet and we just load an empty one.
        return db;
    }
    
    char *buffer = NULL;
    size_t bufferSize = 0;

    char *key_value[2];
    while (getline(&buffer, &bufferSize, f) > 0) {
        key_value[0] = strsep(&buffer, ",");
        key_value[1] = strsep(&buffer, "\n");
        db_put(db, key_value[0], key_value[1]);
        buffer = NULL;
    }

    return db;
}

bool save_database(db_t const *db, char const *path) {
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        fprintf(stderr, "kv: cannot open file %s\n", path);
        return false;
    }

    db_all(db, f);
    return true;
}

void clear_database(db_t *db) {
    db_clear(db);
    free(db);
}


int main(int argc, char *argv[]) {
    db_t *db = load_database(DB_PATH);

    for (int i = 1; i < argc; ++i) {
        char *cmd_arguments = argv[i];
        process_command(db, cmd_arguments);
    }

    if (!save_database(db, DB_PATH)) {
        return 1;
    }

    clear_database(db);
    return 0;
}
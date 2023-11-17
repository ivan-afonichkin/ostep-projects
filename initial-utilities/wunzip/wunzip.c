#include <stdio.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("wunzip: file1 [file2 ...]\n");
        return 1;
    }

    char buffer[5];
    for (int i = 1; i < argc; ++i) {
        FILE *f = fopen(argv[i], "r");
        if (f == NULL) {
            printf("wunzip: cannot open file\n");
            return 1;
        }
        
        size_t buffer_size = 0;
        while ((buffer_size = fread(buffer, sizeof(char), 5, f)) > 0) {
            int n = *((int *)buffer);
            char c = buffer[4];
            for (int j = 0; j < n; ++j) {
                printf("%c", c);
            }
        }
    }
    return 0;
}
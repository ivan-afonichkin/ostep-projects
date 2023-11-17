#include <stdio.h>
#include <stdbool.h>

#define N 256

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("wzip: file1 [file2 ...]\n");
        return 1;
    }

    char buffer[N];
    bool first_char = true;
    char c = 0;
    int c_counter = 0;
    size_t buffer_size = 0;
    char result[5];

    for (int i = 1; i < argc; ++i) {
        FILE *f = fopen(argv[i], "r");
        if (f == NULL) {
            printf("wzip: cannot open file\n");
            return 1;
        }
        while ((buffer_size = fread(buffer, sizeof(char), N, f)) > 0) {
            for (int j = 0; j < buffer_size; ++j) {
                if (c != buffer[j] && !first_char) {
                    *((int *)result) = c_counter;
                    result[4] = c;
                    fwrite(result, sizeof(char), 5, stdout);
                    c_counter = 0;
                }
                first_char = false;
                c = buffer[j];
                ++c_counter;
            }
        }
    }
    if (c_counter != 0) {
        *((int *)result) = c_counter;
        result[4] = c;
        fwrite(result, sizeof(char), 5, stdout);
    }

    return 0;
}
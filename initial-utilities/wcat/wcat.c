
#include <stdio.h>

#define N 256

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        FILE *f = fopen(argv[i], "r");
        if (f == NULL)
        {
            printf("wcat: cannot open file\n");
            return 1;
        }
        char buffer[N];

        while (fgets(buffer, N, f))
        {
            printf("%s", buffer);
        }
    }
    return 0;
}
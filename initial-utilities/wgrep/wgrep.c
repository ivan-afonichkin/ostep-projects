#include <stdio.h>
#include <stdbool.h>

bool containsLine(char *buffer, char *searchTerm)
{
    int startIdx = 0;
    while (buffer[startIdx] != 0)
    {
        int curIdx = 0;
        while (buffer[startIdx + curIdx] != 0 && (buffer[startIdx + curIdx] == searchTerm[curIdx]) && searchTerm[curIdx] != 0)
            ++curIdx;

        if (searchTerm[curIdx] == 0)
        {
            return true;
        }
        ++startIdx;
    }
    return false;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("wgrep: searchterm [file ...]\n");
        return 1;
    }

    char *searchTerm = argv[1];
    char *buffer = NULL;
    size_t bufferCapacity = 0;

    for (int i = 1; i < argc; ++i)
    {
        FILE *f = NULL;
        if (argc == 2)
        {
            f = stdin;
        }
        else
        {
            if (i == 1)
                continue;
            f = fopen(argv[i], "r");
        }

        if (f == NULL)
        {
            printf("wgrep: cannot open file\n");
            return 1;
        }
        while (getline(&buffer, &bufferCapacity, f) > 0)
        {
            if (containsLine(buffer, searchTerm))
            {
                printf("%s", buffer);
            }
        }
    }

    return 0;
}
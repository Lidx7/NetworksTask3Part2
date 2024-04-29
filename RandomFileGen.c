#include <stdio.h>
#include <time.h>

char *util_generate_random_data(int size) {
    char *rand_file_ret = NULL;
    if (size == 0)
        return NULL;
    rand_file_ret = (char *)calloc(size, sizeof(char));
    if (rand_file_ret == NULL)
        return NULL;
    srand(time(NULL));
    for (unsigned int i = 0; i < size; i++)
        *(rand_file_ret + i) = ((unsigned int)rand() % 256);
    return rand_file_ret;
}
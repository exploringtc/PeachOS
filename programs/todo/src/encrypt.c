#include "todo.h"

int xor_apply(char* data, int data_size, const char* key)
{
    int i;
    int key_len = 0;

    if (!data || !key) {
        return TODO_ERR_BAD_INPUT;
    }

    while (key[key_len] != '\0') {
        key_len++;
    }

    if (key_len == 0) {
        return TODO_ERR_BAD_INPUT;
    }

    for (i = 0; i < data_size; i++) {
        data[i] = (char)(data[i] ^ key[i % key_len]);
    }

    return TODO_OK;
}

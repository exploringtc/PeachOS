#include "todo.h"

#define DISK_MAX_FILES 8

struct disk_file_slot {
    int used;
    int size;
    char filename[64];
    char data[TODO_BUFFER_SIZE];
};

static struct disk_file_slot g_slots[DISK_MAX_FILES];

static void str_copy(char* dst, const char* src, int dst_size)
{
    int i = 0;
    while (i < dst_size - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int str_equal(const char* a, const char* b)
{
    int i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

static int find_slot(const char* filename)
{
    int i;
    for (i = 0; i < DISK_MAX_FILES; i++) {
        if (g_slots[i].used && str_equal(g_slots[i].filename, filename)) {
            return i;
        }
    }
    return -1;
}

static int find_free_slot(void)
{
    int i;
    for (i = 0; i < DISK_MAX_FILES; i++) {
        if (!g_slots[i].used) {
            return i;
        }
    }
    return -1;
}

int disk_write_file(const char* filename, const char* data, int size)
{
    int i;
    int slot = find_slot(filename);

    if (!filename || !data || size < 0 || size > TODO_BUFFER_SIZE) {
        return TODO_ERR_BAD_INPUT;
    }

    if (slot < 0) {
        slot = find_free_slot();
        if (slot < 0) {
            return TODO_ERR_FULL;
        }
        g_slots[slot].used = 1;
        str_copy(g_slots[slot].filename, filename, sizeof(g_slots[slot].filename));
    }

    for (i = 0; i < size; i++) {
        g_slots[slot].data[i] = data[i];
    }
    g_slots[slot].size = size;

    return TODO_OK;
}

int disk_read_file(const char* filename, char* out, int out_size, int* size_read)
{
    int i;
    int slot = find_slot(filename);

    if (!filename || !out || out_size <= 0) {
        return TODO_ERR_BAD_INPUT;
    }

    if (slot < 0) {
        return TODO_ERR_NOT_FOUND;
    }

    if (g_slots[slot].size > out_size) {
        return TODO_ERR_DESERIALIZE;
    }

    for (i = 0; i < g_slots[slot].size; i++) {
        out[i] = g_slots[slot].data[i];
    }
    if (g_slots[slot].size < out_size) {
        out[g_slots[slot].size] = '\0';
    }

    if (size_read) {
        *size_read = g_slots[slot].size;
    }

    return TODO_OK;
}

#include "todo.h"

#define SYSTEM_COMMAND1_PRINT 1

int xor_apply(char* data, int data_size, const char* key);
int disk_write_file(const char* filename, const char* data, int size);
int disk_read_file(const char* filename, char* out, int out_size, int* size_read);

void syscall_print(const char* msg)
{
    /* Uses int 0x80 command channel to print through kernel terminal service. */
    __asm__ volatile(
        "push %[str]\n"
        "mov %[cmd], %%eax\n"
        "int $0x80\n"
        "add $4, %%esp\n"
        :
        : [str] "r" (msg), [cmd] "i" (SYSTEM_COMMAND1_PRINT)
        : "eax", "memory"
    );
}

int sys_add_task(struct todo_list* list, char* task)
{
    return todo_add_task(list, task);
}

int sys_list_tasks(struct todo_list* list)
{
    todo_print_tasks(list, syscall_print);
    return TODO_OK;
}

int sys_remove_task(struct todo_list* list, int id)
{
    return todo_remove_task(list, id);
}

int sys_set_task_completed(struct todo_list* list, int id, int completed)
{
    return todo_set_task_completed(list, id, completed);
}

int sys_save_tasks(struct todo_list* list, char* filename, char* key)
{
    char buffer[TODO_BUFFER_SIZE];
    int written = 0;
    int res;

    res = todo_serialize(list, buffer, sizeof(buffer), &written);
    if (res != TODO_OK) {
        return res;
    }

    /* XOR in place so stored bytes are not readable without the same key. */
    res = xor_apply(buffer, written, key);
    if (res != TODO_OK) {
        return res;
    }

    return disk_write_file(filename, buffer, written);
}

int sys_load_tasks(struct todo_list* list, char* filename, char* key)
{
    char buffer[TODO_BUFFER_SIZE];
    int size_read = 0;
    int res;

    res = disk_read_file(filename, buffer, sizeof(buffer), &size_read);
    if (res != TODO_OK) {
        return res;
    }

    /* XOR again with same key to recover original serialized text. */
    res = xor_apply(buffer, size_read, key);
    if (res != TODO_OK) {
        return res;
    }

    return todo_deserialize(list, buffer, size_read);
}

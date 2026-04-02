#include "main.h"
#include "todo.h"

/* Fallbacks for editor indexers in minimal freestanding setups. */
#ifndef TODO_MAX_TASKS
#define TODO_MAX_TASKS 64
#endif

#ifndef TODO_MAX_DESC_LEN
#define TODO_MAX_DESC_LEN 128
#endif

int sys_add_task(struct todo_list* list, char* task);
int sys_list_tasks(struct todo_list* list);
int sys_remove_task(struct todo_list* list, int id);
int sys_set_task_completed(struct todo_list* list, int id, int completed);
int sys_save_tasks(struct todo_list* list, char* filename, char* key);
int sys_load_tasks(struct todo_list* list, char* filename, char* key);
void syscall_print(const char* msg);

static struct todo_list g_todo_list;

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

static int starts_with(const char* s, const char* prefix)
{
    int i = 0;
    while (prefix[i] != '\0') {
        if (s[i] != prefix[i]) {
            return 0;
        }
        i++;
    }
    return 1;
}

static int parse_int(const char* s, int* out)
{
    int i = 0;
    int value = 0;
    int saw_digit = 0;

    while (s[i] != '\0') {
        if (s[i] < '0' || s[i] > '9') {
            return 0;
        }
        saw_digit = 1;
        value = value * 10 + (s[i] - '0');
        i++;
    }

    if (!saw_digit) {
        return 0;
    }

    *out = value;
    return 1;
}

/* Splits "cmd arg1 arg2" into two tokens (arg1/arg2), in-place, no heap needed. */
static int split_two_args(char* input, char** first, char** second)
{
    int i = 0;

    if (!input || !first || !second) {
        return 0;
    }

    while (input[i] == ' ') {
        i++;
    }
    if (input[i] == '\0') {
        return 0;
    }

    *first = &input[i];
    while (input[i] != '\0' && input[i] != ' ') {
        i++;
    }
    if (input[i] == '\0') {
        return 0;
    }

    input[i++] = '\0';
    while (input[i] == ' ') {
        i++;
    }
    if (input[i] == '\0') {
        return 0;
    }

    *second = &input[i];
    return 1;
}

static void print_status(int res)
{
    if (res == TODO_OK) {
        syscall_print("ok\n");
        return;
    }

    if (res == TODO_ERR_NOT_FOUND) {
        syscall_print("error: task/file not found\n");
        return;
    }

    if (res == TODO_ERR_FULL) {
        syscall_print("error: capacity reached\n");
        return;
    }

    if (res == TODO_ERR_BAD_INPUT) {
        syscall_print("error: invalid input\n");
        return;
    }

    if (res == TODO_ERR_SERIALIZE || res == TODO_ERR_DESERIALIZE) {
        syscall_print("error: save/load data format issue\n");
        return;
    }

    syscall_print("error: unknown failure\n");
}

int app_run_command(const char* command)
{
    /* Command format: add <description with spaces> */
    if (starts_with(command, "add ")) {
        int res = sys_add_task(&g_todo_list, (char*)(command + 4));
        print_status(res);
        return res;
    }

    if (str_equal(command, "list")) {
        return sys_list_tasks(&g_todo_list);
    }

    if (starts_with(command, "remove ")) {
        int id = 0;
        int res;
        if (!parse_int(command + 7, &id)) {
            syscall_print("error: invalid id\n");
            return TODO_ERR_BAD_INPUT;
        }

        res = sys_remove_task(&g_todo_list, id);
        print_status(res);
        return res;
    }

    if (starts_with(command, "complete ")) {
        int id = 0;
        int res;
        if (!parse_int(command + 9, &id)) {
            syscall_print("error: invalid id\n");
            return TODO_ERR_BAD_INPUT;
        }

        res = sys_set_task_completed(&g_todo_list, id, 1);
        print_status(res);
        return res;
    }

    if (starts_with(command, "uncomplete ")) {
        int id = 0;
        int res;
        if (!parse_int(command + 11, &id)) {
            syscall_print("error: invalid id\n");
            return TODO_ERR_BAD_INPUT;
        }

        res = sys_set_task_completed(&g_todo_list, id, 0);
        print_status(res);
        return res;
    }

    /* Command format: save <filename> <key> */
    if (starts_with(command, "save ")) {
        char args[TODO_MAX_DESC_LEN];
        char* filename = 0;
        char* key = 0;
        int i = 0;
        int res;

        while (command[5 + i] != '\0' && i < TODO_MAX_DESC_LEN - 1) {
            args[i] = command[5 + i];
            i++;
        }
        args[i] = '\0';

        if (!split_two_args(args, &filename, &key)) {
            syscall_print("usage: save <filename> <key>\n");
            return TODO_ERR_BAD_INPUT;
        }

        res = sys_save_tasks(&g_todo_list, filename, key);
        print_status(res);
        return res;
    }

    /* Command format: load <filename> <key> */
    if (starts_with(command, "load ")) {
        char args[TODO_MAX_DESC_LEN];
        char* filename = 0;
        char* key = 0;
        int i = 0;
        int res;

        while (command[5 + i] != '\0' && i < TODO_MAX_DESC_LEN - 1) {
            args[i] = command[5 + i];
            i++;
        }
        args[i] = '\0';

        if (!split_two_args(args, &filename, &key)) {
            syscall_print("usage: load <filename> <key>\n");
            return TODO_ERR_BAD_INPUT;
        }

        res = sys_load_tasks(&g_todo_list, filename, key);
        print_status(res);
        return res;
    }

    if (str_equal(command, "help")) {
        syscall_print("commands:\n");
        syscall_print("  add <task>\n");
        syscall_print("  list\n");
        syscall_print("  remove <id>\n");
        syscall_print("  complete <id>\n");
        syscall_print("  uncomplete <id>\n");
        syscall_print("  save <filename> <key>\n");
        syscall_print("  load <filename> <key>\n");
        return TODO_OK;
    }

    syscall_print("error: unknown command\n");
    return TODO_ERR_BAD_INPUT;
}

void _start(void)
{
    static const char* demo_script[] = {
        "add Finish CYSE 570 report",
        "add Implement syscall wrappers",
        "complete 2",
        "list",
        "save tasks.dat cyse570-key",
        "remove 1",
        "list",
        "load tasks.dat cyse570-key",
        "list"
    };
    int i;

    todo_init(&g_todo_list);

    syscall_print("todo app booted from /programs\n");
    syscall_print("type 'help' for command list\n");

    for (i = 0; i < (int)(sizeof(demo_script) / sizeof(demo_script[0])); i++) {
        syscall_print("> ");
        syscall_print(demo_script[i]);
        syscall_print("\n");
        app_run_command(demo_script[i]);
    }

    syscall_print("todo app idle\n");
    while (1) {
    }
}
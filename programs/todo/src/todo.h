#ifndef TODO_APP_TODO_H
#define TODO_APP_TODO_H

#define TODO_MAX_TASKS 64
#define TODO_MAX_DESC_LEN 128
#define TODO_BUFFER_SIZE 4096

#define TODO_OK 0
#define TODO_ERR_FULL -1
#define TODO_ERR_NOT_FOUND -2
#define TODO_ERR_BAD_INPUT -3
#define TODO_ERR_SERIALIZE -4
#define TODO_ERR_DESERIALIZE -5

typedef void (*todo_output_fn)(const char* text);

struct todo_task {
    int id;
    int completed;
    char description[TODO_MAX_DESC_LEN];
};

struct todo_list {
    struct todo_task tasks[TODO_MAX_TASKS];
    int count;
    int next_id;
};

/* Initializes the task container and resets ID allocation state. */
void todo_init(struct todo_list* list);
/* Adds a new incomplete task with an auto-incremented ID. */
int todo_add_task(struct todo_list* list, const char* task);
/* Removes the task that matches the provided ID. */
int todo_remove_task(struct todo_list* list, int id);
/* Updates the completion flag (0 = incomplete, 1 = complete) for a task ID. */
int todo_set_task_completed(struct todo_list* list, int id, int completed);
/* Converts the in-memory list into a line-based format for persistence. */
int todo_serialize(const struct todo_list* list, char* out, int out_size, int* written);
/* Rebuilds the in-memory list from the serialized text format. */
int todo_deserialize(struct todo_list* list, const char* data, int data_size);
/* Prints tasks via a caller-provided output function (terminal/syscall). */
void todo_print_tasks(const struct todo_list* list, todo_output_fn out);

#endif

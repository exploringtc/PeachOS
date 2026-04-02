#include "todo.h"

static void str_copy(char* dst, const char* src, int dst_size)
{
    int i = 0;
    if (dst_size <= 0) {
        return;
    }
    while (i < dst_size - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int int_to_str(int value, char* out, int out_size)
{
    char tmp[16];
    int i = 0;
    int j;
    int negative = 0;

    if (out_size <= 1) {
        return 0;
    }

    if (value == 0) {
        out[0] = '0';
        out[1] = '\0';
        return 1;
    }

    if (value < 0) {
        negative = 1;
        value = -value;
    }

    while (value > 0 && i < (int)sizeof(tmp) - 1) {
        tmp[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    if (negative && i < (int)sizeof(tmp) - 1) {
        tmp[i++] = '-';
    }

    if (i + 1 > out_size) {
        return 0;
    }

    for (j = 0; j < i; j++) {
        out[j] = tmp[i - j - 1];
    }
    out[i] = '\0';
    return i;
}

static int append_text(char* out, int out_size, int* pos, const char* text)
{
    int i = 0;
    while (text[i] != '\0') {
        if (*pos >= out_size - 1) {
            return 0;
        }
        out[*pos] = text[i];
        (*pos)++;
        i++;
    }
    out[*pos] = '\0';
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

static char hex_digit(int value)
{
    if (value < 10) {
        return (char)('0' + value);
    }
    return (char)('A' + (value - 10));
}

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    return -1;
}

/* Escapes reserved separators so serialized text stays parse-safe. */
static int append_encoded_text(char* out, int out_size, int* pos, const char* text)
{
    int i = 0;

    while (text[i] != '\0') {
        unsigned char ch = (unsigned char)text[i];
        int must_escape = (ch == '|') || (ch == '\n') || (ch == '\r') || (ch == '%');

        if (!must_escape) {
            if (*pos >= out_size - 1) {
                return 0;
            }
            out[*pos] = (char)ch;
            (*pos)++;
            out[*pos] = '\0';
        } else {
            if (*pos >= out_size - 4) {
                return 0;
            }
            out[*pos] = '%';
            out[*pos + 1] = hex_digit((ch >> 4) & 0x0F);
            out[*pos + 2] = hex_digit(ch & 0x0F);
            *pos += 3;
            out[*pos] = '\0';
        }

        i++;
    }

    return 1;
}

void todo_init(struct todo_list* list)
{
    /* Keep IDs monotonically increasing so removed IDs are never reused. */
    list->count = 0;
    list->next_id = 1;
}

int todo_add_task(struct todo_list* list, const char* task)
{
    if (!task || task[0] == '\0') {
        return TODO_ERR_BAD_INPUT;
    }

    if (list->count >= TODO_MAX_TASKS) {
        return TODO_ERR_FULL;
    }

    list->tasks[list->count].id = list->next_id;
    list->tasks[list->count].completed = 0;
    str_copy(list->tasks[list->count].description, task, TODO_MAX_DESC_LEN);

    list->count++;
    list->next_id++;
    return TODO_OK;
}

int todo_remove_task(struct todo_list* list, int id)
{
    int i;

    for (i = 0; i < list->count; i++) {
        if (list->tasks[i].id == id) {
            int j;
            for (j = i; j < list->count - 1; j++) {
                list->tasks[j] = list->tasks[j + 1];
            }
            list->count--;
            return TODO_OK;
        }
    }

    return TODO_ERR_NOT_FOUND;
}

int todo_set_task_completed(struct todo_list* list, int id, int completed)
{
    int i;

    for (i = 0; i < list->count; i++) {
        if (list->tasks[i].id == id) {
            list->tasks[i].completed = completed ? 1 : 0;
            return TODO_OK;
        }
    }

    return TODO_ERR_NOT_FOUND;
}

int todo_serialize(const struct todo_list* list, char* out, int out_size, int* written)
{
    int i;
    int pos = 0;

    /* Wire format per line: id|completed|description\n */
    for (i = 0; i < list->count; i++) {
        char id_text[16];
        char done_text[4];

        if (!int_to_str(list->tasks[i].id, id_text, sizeof(id_text))) {
            return TODO_ERR_SERIALIZE;
        }

        if (!int_to_str(list->tasks[i].completed, done_text, sizeof(done_text))) {
            return TODO_ERR_SERIALIZE;
        }

        if (!append_text(out, out_size, &pos, id_text)) {
            return TODO_ERR_SERIALIZE;
        }
        if (!append_text(out, out_size, &pos, "|")) {
            return TODO_ERR_SERIALIZE;
        }
        if (!append_text(out, out_size, &pos, done_text)) {
            return TODO_ERR_SERIALIZE;
        }
        if (!append_text(out, out_size, &pos, "|")) {
            return TODO_ERR_SERIALIZE;
        }
        if (!append_encoded_text(out, out_size, &pos, list->tasks[i].description)) {
            return TODO_ERR_SERIALIZE;
        }
        if (!append_text(out, out_size, &pos, "\n")) {
            return TODO_ERR_SERIALIZE;
        }
    }

    if (written) {
        *written = pos;
    }

    return TODO_OK;
}

int todo_deserialize(struct todo_list* list, const char* data, int data_size)
{
    int i = 0;

    todo_init(list);

    while (i < data_size && data[i] != '\0') {
        char id_text[16];
        char done_text[4];
        char desc[TODO_MAX_DESC_LEN];
        int id_i = 0;
        int done_i = 0;
        int desc_i = 0;
        int id = 0;
        int done = 0;

        while (i < data_size && data[i] != '|' && id_i < (int)sizeof(id_text) - 1) {
            id_text[id_i++] = data[i++];
        }
        id_text[id_i] = '\0';
        if (i >= data_size || data[i] != '|') {
            return TODO_ERR_DESERIALIZE;
        }
        i++;

        while (i < data_size && data[i] != '|' && done_i < (int)sizeof(done_text) - 1) {
            done_text[done_i++] = data[i++];
        }
        done_text[done_i] = '\0';
        if (i >= data_size || data[i] != '|') {
            return TODO_ERR_DESERIALIZE;
        }
        i++;

        while (i < data_size && data[i] != '\n' && desc_i < TODO_MAX_DESC_LEN - 1) {
            if (data[i] == '%') {
                int hi;
                int lo;

                if (i + 2 >= data_size) {
                    return TODO_ERR_DESERIALIZE;
                }

                hi = hex_value(data[i + 1]);
                lo = hex_value(data[i + 2]);
                if (hi < 0 || lo < 0) {
                    return TODO_ERR_DESERIALIZE;
                }

                desc[desc_i++] = (char)((hi << 4) | lo);
                i += 3;
                continue;
            }

            desc[desc_i++] = data[i++];
        }
        desc[desc_i] = '\0';

        if (i < data_size && data[i] != '\n') {
            /* Description exceeded in-memory limit before end-of-line. */
            return TODO_ERR_DESERIALIZE;
        }

        if (i < data_size && data[i] == '\n') {
            i++;
        }

        if (!parse_int(id_text, &id) || !parse_int(done_text, &done)) {
            return TODO_ERR_DESERIALIZE;
        }

        if (list->count >= TODO_MAX_TASKS) {
            return TODO_ERR_DESERIALIZE;
        }

        list->tasks[list->count].id = id;
        list->tasks[list->count].completed = done ? 1 : 0;
        str_copy(list->tasks[list->count].description, desc, TODO_MAX_DESC_LEN);
        list->count++;

        if (id >= list->next_id) {
            list->next_id = id + 1;
        }
    }

    return TODO_OK;
}

void todo_print_tasks(const struct todo_list* list, todo_output_fn out)
{
    int i;

    if (list->count == 0) {
        out("no tasks\n");
        return;
    }

    for (i = 0; i < list->count; i++) {
        char id_text[16];

        out("[");
        int_to_str(list->tasks[i].id, id_text, sizeof(id_text));
        out(id_text);
        out("] ");
        /* Show status marker so users can quickly scan done vs pending tasks. */
        out(list->tasks[i].completed ? "(x) " : "( ) ");
        out(list->tasks[i].description);
        out("\n");
    }
}

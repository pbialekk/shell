#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "history.h"
#include "my_utils.h"

char **history;
int insert_pos, return_pos, cur_size;

void history_init() {
    history = malloc(HISTORY_STARTSIZE * sizeof(char *));
    cur_size = HISTORY_STARTSIZE;
    insert_pos = 0, return_pos = 0;
}

static void _history_resize() {
    char **new_history = malloc(2 * cur_size * sizeof(char *));
    memmove(new_history, history, cur_size * sizeof(char *));
    free(history);
    history = new_history;
    cur_size *= 2;
}

void history_add(char *cmd, int len) {
    if (len == 0) {
        return;
    }
    if (insert_pos == cur_size) {
        _history_resize();
    }
    history[insert_pos] = malloc((len + 1) * sizeof(char));
    memcpy(history[insert_pos], cmd, (len + 1) * sizeof(char));
    insert_pos++;
}

void history_arrowUp() {
    return_pos = max(return_pos - 1, 0);
}

void history_arrowDown() {
    return_pos = min(return_pos + 1, insert_pos);
}

char *history_getEntry() {
    if (return_pos == insert_pos) {
        return "";
    }
    return history[return_pos];
}

void history_resetPtr() {
    return_pos = insert_pos;
}

int history_isPtrReset() {
    return return_pos == insert_pos;
}

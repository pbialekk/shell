#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "config.h"
#include "history.h"
#include "my_utils.h"
#include "prompt.h"
#include "read.h"
#include "siparse.h"

int buf_beg = 0, buf_end = -1;
char buf[BUF_MAX + 1];
int seen_eof = 0;
const int oo = MAX_LINE_LENGTH + 3;

static void _fillBuffer() {
    if (seen_eof) {
        return;
    }
    if (buf_end + 1 != BUF_MAX) {
        errno = 0;
        int bytes_read = read(STDIN_FILENO, buf + buf_end + 1, BUF_MAX - buf_end - 1);
        if (errno == EAGAIN) { // pajp
            _fillBuffer();     // or maybe goto?
            return;
        }
        if (bytes_read == 0) {
            seen_eof = 1;
        } else if (bytes_read < 0) {
            fprintf(stderr, "%s\n", READ_FAIL);
            exit(EXEC_FAILURE);
        }
        buf_end += bytes_read;
    } else {
        int len = buf_end - buf_beg + 1;
        memmove(buf, buf + buf_beg, len);
        buf_beg = 0, buf_end = len - 1;
        _fillBuffer();
    }
}

int cmd_from, cmd_to;

static void _smartRead(int);

static void _tryToSkip() {
    char *newline_it = strchr(buf + buf_beg, '\n');
    int newline_pos;
    if (newline_it != NULL && (newline_pos = newline_it - buf) <= buf_end) {
        buf_beg = newline_pos + 1, buf_end = buf_end;
        if (buf_beg > buf_end) {
            buf_beg = 0, buf_end = -1;
        }
        _smartRead(1);
    } else if (seen_eof) {
        cmd_from = 0, cmd_to = -1; // is it over...
    } else {
        buf_beg = 0, buf_end = -1;
        _fillBuffer();
        _tryToSkip();
    }
}

static void _smartRead(int printedSyntaxError) {
    char *newline_it = strchr(buf + buf_beg, '\n');
    int newline_pos = (newline_it == NULL || newline_it - buf > buf_end
                           ? buf_end + 1
                           : newline_it - buf);
    if (newline_pos - buf_beg >= MAX_LINE_LENGTH) {
        if (!printedSyntaxError) {
            fprintf(stderr, "%s\n", SYNTAX_ERROR_STR);
        }
        _tryToSkip();
    } else if (newline_it != NULL && newline_pos <= buf_end) {
        cmd_from = buf_beg, cmd_to = newline_pos;
        buf_beg = newline_pos + 1;
        if (buf_beg > buf_end) { // we emptied the buffer
            buf_beg = 0, buf_end = -1;
        }
    } else if (seen_eof) {
        if (buf_beg > buf_end) {
            cmd_from = 0, cmd_to = -1;
            return;
        }
        cmd_from = buf_beg, cmd_to = buf_end + 1;
        buf_beg = 0, buf_end = -1;
    } else {
        _fillBuffer();
        _smartRead(printedSyntaxError);
    }
}

struct termios saved_termios;

void restoreTerm() {
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);
}

void saveTerm() {
    tcgetattr(STDIN_FILENO, &saved_termios);
}

static void _enableRawMode() {
    struct termios raw = saved_termios;
    raw.c_lflag &= ~(ICANON | ECHO | ISIG);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

static pipelineseq *_parseline(char *bbuf) {
    blockSigchld();
    restoreTerm();
    return parseline(bbuf);
}

static char _my_getchar() {
    char c;
    errno = 0;
    int bytes_read = read(STDIN_FILENO, &c, 1);
    if (errno == EAGAIN) {
        return _my_getchar();
    }
    if (bytes_read < 0) {
        fprintf(stderr, "%s\n", READ_FAIL);
        exit(EXEC_FAILURE);
    }
    return c;
}

void _go_left(char *buf, int *index) {
    int i = *index - 1;
    while (i >= 0 && buf[i] == ' ') {
        i--;
    }
    while (i >= 0 && buf[i] != ' ') {
        i--;
    }
    *index = i + 1;
}

void _go_right(char *buf, int *index) {
    int i = *index;
    while (buf[i] != '\0' && buf[i] == ' ') {
        i++;
    }
    while (buf[i] != '\0' && buf[i] != ' ') {
        i++;
    }
    *index = i;
}

pipelineseq *read_newLine() {
    unblockSigchld();
    if (!is_a_tty) {
        _smartRead(0);
        if (cmd_from > cmd_to) {
            exit(EXEC_SUCCESS);
        }
        buf[cmd_to] = '\0';
        return _parseline(buf + cmd_from);
    }

    _enableRawMode();
    int index = 0, buf_len = 0;
    buf[0] = '\0';
    printf("%s", PROMPT_STR_2);
    fflush(stdout);
    history_resetPtr();
    while (1) {
        char c;
        if ((c = _my_getchar()) == EOT && buf_len == 0) {
            exit(EXIT_SUCCESS);
        } else if (c == CTRL_C && buf_len != 0) {
            if (buf_len - index > 0) {
                printf("\033[%dC", buf_len - index);
            }
            printf("^C\n");
            return _parseline("");
        } else if (c == CTRL_Q) {
            return _parseline("asciiquarium");
        } else if (PRINTABLE_START <= c && c <= PRINTABLE_END && buf_len < MAX_LINE_LENGTH) {
            memmove(buf + index + 1, buf + index, buf_len - index + 1);
            buf[index++] = c;
            buf_len++;
        } else if (c == EOL) {
            printf("\033[%dD\n", oo);

            history_add(buf, buf_len);
            return _parseline(buf);
        } else if (c == ESCAPE) {
            if ((c = _my_getchar()) == ARROW_BLOCK_START) {
                if ((c = _my_getchar()) == ARROW_LEFT) {
                    index = max(0, index - 1);
                } else if (c == ARROW_RIGHT) {
                    index = min(buf_len, index + 1);
                } else if (c == ARROW_UP || c == ARROW_DOWN) {
                    if (history_isPtrReset()) {
                        history_add(buf, buf_len);
                    }
                    (c == ARROW_UP ? history_arrowUp() : history_arrowDown());
                    char *old_command;
                    if ((old_command = history_getEntry()) != NULL) {
                        strcpy(buf, old_command);
                        buf_len = strlen(buf);
                        index = buf_len;
                    }
                } else if (c == 49 && _my_getchar() == 59 && _my_getchar() == 53) { // magic for handling the ctrl + left/right
                    if ((c = _my_getchar()) == ARROW_LEFT) {
                        _go_left(buf, &index);
                    } else if (c == ARROW_RIGHT) {
                        _go_right(buf, &index);
                    }
                }
            }
        } else if (c == BACKSPACE && index > 0) {
            memmove(buf + index - 1, buf + index, buf_len - index + 1);
            index--;
            buf_len--;
        } else {
            continue;
        }
        printf("\033[%dD", oo);
        printf("\033[0K");
        printf("%s", PROMPT_STR_2);
        printf("%s", buf);
        if (buf_len - index > 0) {
            printf("\033[%dD", buf_len - index);
        }
        fflush(stdout);
    }
}

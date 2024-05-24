#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "my_utils.h"
#include "prompt.h"

const char *PROMPT_STR_C = (char *)PROMPT_STR;
char hostname[HOST_NAME_MAX];
struct passwd *pwd;
char cwd[PATH_MAX];
char *home_dir;
int home_dir_len;
int is_a_tty, cwd_flag;

static void _die() {
    fprintf(stderr, "%s\n", PROMPT_ERROR);
    exit(EXEC_FAILURE);
}

void prompt_init() {
    is_a_tty = isatty(STDIN_FILENO);
    if (gethostname(hostname, HOST_NAME_MAX) != 0) {
        _die();
    }
    pwd = getpwuid(getuid());
    if (!pwd) {
        _die();
    }
    cwd_flag = 1;
    home_dir = getenv("HOME");
    if (home_dir == NULL) {
        _die();
    }
    home_dir_len = strlen(home_dir);
}

void changeCwd() {
    cwd_flag = 1;
}

void prompt_print() {
    processDeadChildren();
    if (is_a_tty == 0) {
        return;
    }
    if (cwd_flag) {
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            _die();
        }
        if (strncmp(cwd, home_dir, home_dir_len) == 0) {
            memmove(cwd, cwd + home_dir_len - 1, strlen(cwd) - home_dir_len + 2);
            cwd[0] = '~';
        }
        cwd_flag = 0;
    }

    char const *p = PROMPT_STR_C;
    while (*p) {
        if (strncmp(p, "%u", 2) == 0) {
            printf("%s%s%s", ANSI_COLOR_GOLD, pwd->pw_name, ANSI_COLOR_RESET);
            p += 2;
        } else if (strncmp(p, "%h", 2) == 0) {
            printf("%s%s%s", ANSI_COLOR_PURPLE, hostname, ANSI_COLOR_RESET);
            p += 2;
        } else if (strncmp(p, "%c", 2) == 0) {
            printf("%s%s%s", ANSI_COLOR_GREEN, cwd, ANSI_COLOR_RESET);
            p += 2;
        } else {
            printf("%c", *p);
            if (*p == '\n' && is_a_tty == 1) {
                break;
            }
            p++;
        }
    }
    fflush(stdout);
}

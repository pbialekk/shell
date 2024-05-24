#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtins.h"
#include "config.h"
#include "history.h"
#include "my_utils.h"
#include "prompt.h"
#include "read.h"
#include "run.h"
#include "siparse.h"

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}

void prepareArgsArray(char **arr, const command *com) {
    argseq *argseq = com->args;
    int i = 0;
    do {
        arr[i] = argseq->arg;
        argseq = argseq->next;
        i++;
    } while (argseq != com->args);
    arr[i] = NULL;
}

int myAtoi(const char *str, long *v) {
    char *endptr;
    errno = 0;
    *v = strtol(str, &endptr, 10);
    if (errno || *endptr != '\0' || !(INT_MIN <= *v && *v <= INT_MAX)) {
        return 0;
    }
    return 1;
}

void printError(char *filename, int print_execerror) {
    if (errno == ENOENT) {
        fprintf(stderr, "%s: %s", filename, WRONG_FILE);
    } else if (errno == EACCES) {
        fprintf(stderr, "%s: %s", filename, NO_PERMISSIONS);
    } else if (print_execerror) {
        fprintf(stderr, "%s: %s", filename, EXEC_ERROR);
    }
    fprintf(stderr, "\n");
}

int redirectIn(char *filename) {
    int in = open(filename, O_RDONLY);
    if (in < 0) {
        printError(filename, 0);
        return 0;
    }
    dup2(in, STDIN_FILENO);
    close(in);
    return 1;
}

int redirectOut(char *filename, int append) {
    int out = open(filename, O_WRONLY | (append ? O_APPEND : O_TRUNC) | O_CREAT, S_IRUSR | S_IWUSR);
    if (out < 0) {
        printError(filename, 0);
        return 0;
    }
    dup2(out, STDOUT_FILENO);
    close(out);
    return 1;
}

int processRedirs(command *com) {
    redirseq *redirs = com->redirs;
    if (redirs != NULL) {
        do {
            int flags = redirs->r->flags;
            char *filename = redirs->r->filename;
            if (IS_RIN(flags) && !redirectIn(filename)) {
                return 0;
            } else if (IS_ROUT(flags) && !redirectOut(filename, 0)) {
                return 0;
            } else if (IS_RAPPEND(flags) && !redirectOut(filename, 1)) {
                return 0;
            }
            redirs = redirs->next;
        } while (redirs != com->redirs);
    }
    return 1;
}

int isBuiltin(char *name) {
    int i = 0;
    while (builtins_table[i].name != NULL && strcmp(builtins_table[i].name, name) != 0) {
        i++;
    }
    return builtins_table[i].name != NULL;
}

int callBuiltin(char *name, char **args) {
    int i = 0;
    while (builtins_table[i].name != NULL && strcmp(builtins_table[i].name, name) != 0) {
        i++;
    }
    if (builtins_table[i].name != NULL) {
        return builtins_table[i].fun(args);
    }
    return -1;
}

int getNullPos(char **args) {
    int i = 0;
    while (args[i] != NULL) {
        i++;
    }
    return i;
}

typedef struct pid_pair pid_pair;

struct pid_pair {
    pid_t pid;
    int status;
    pid_pair *prev;
    pid_pair *next;
};

pid_pair *bgjobs_head = NULL, *bgjobs_tail = NULL;

void _init_bgjobs() {
    if (bgjobs_head == NULL) {
        bgjobs_head = malloc(sizeof(pid_pair));
        bgjobs_head->pid = -1;
        bgjobs_head->prev = NULL;
        bgjobs_head->next = NULL;
        bgjobs_tail = bgjobs_head;
    }
}

void newBgjob(pid_t pid) {
    _init_bgjobs();
    pid_pair *new = malloc(sizeof(pid_pair));
    new->pid = pid;
    new->status = -1;
    new->prev = bgjobs_tail;
    new->next = NULL;
    bgjobs_tail->next = new;
    bgjobs_tail = new;
}

int bgjobFinished(pid_t pid, int rstat) {
    pid_pair *cur = bgjobs_head->next;
    while (cur != NULL) {
        if (cur->pid == pid) {
            cur->status = rstat;
            return 1;
        }
        cur = cur->next;
    }
    return 0;
}

sigset_t sigchldMask, EMPTY_SIGSET;

void blockSigchld() {
    sigprocmask(SIG_BLOCK, &sigchldMask, NULL);
}

void unblockSigchld() {
    sigprocmask(SIG_UNBLOCK, &sigchldMask, NULL);
}

char *ENV_PATH;
struct sigaction old_sigint, old_sigchld;

void restoreSigactions() {
    unblockSigchld();
    sigaction(SIGINT, &old_sigint, NULL);
    sigaction(SIGCHLD, &old_sigchld, NULL);
}

void prepareEverything() {
    sigemptyset(&EMPTY_SIGSET);
    sigaction(SIGINT, &(struct sigaction){.sa_handler = SIG_IGN, .sa_mask = EMPTY_SIGSET}, &old_sigint);
    sigaction(SIGCHLD, &(struct sigaction){.sa_handler = run_sigchldHandler, .sa_flags = SA_RESTART | SA_NOCLDSTOP, .sa_mask = EMPTY_SIGSET}, &old_sigchld);

    sigemptyset(&sigchldMask);
    sigaddset(&sigchldMask, SIGCHLD);

    atexit(restoreSigactions);

    saveTerm();
    atexit(restoreTerm);
    prompt_init();
    history_init();

    newBgjob(-1); // so the list is never empty

    ENV_PATH = getenv("PATH");
}

void processDeadChildren() {
    pid_pair *cur = bgjobs_head->next;
    while (cur != NULL) {
        if (cur->status != -1) {
            if (is_a_tty) {
                printf("Background process %d ", cur->pid);
                if (WIFEXITED(cur->status)) {
                    printf("terminated. (exited with status %d)", WEXITSTATUS(cur->status));
                } else {
                    printf("terminated. (killed by signal %d)", WTERMSIG(cur->status));
                }
                printf("\n");
            }
            pid_pair *tmp = cur;
            cur = cur->next;
            if (tmp->prev != NULL) {
                tmp->prev->next = tmp->next;
            } else {
                bgjobs_head = tmp->next;
            }
            if (tmp->next != NULL) {
                tmp->next->prev = tmp->prev;
            } else {
                bgjobs_tail = tmp->prev;
            }
            free(tmp);
        } else {
            cur = cur->next;
        }
    }
}

int _isExecutable(char *real_path) {
    return access(real_path, F_OK | X_OK) == 0;
}

int isReadable(char *fn) {
    return access(fn, F_OK | R_OK) == 0;
}

char full_path[PATH_MAX];
int isExecutable(char *fn) {
    if (_isExecutable(fn)) {
        return 1;
    }
    if (ENV_PATH == NULL) {
        return 0;
    }
    if (fn[0] == '/') {
        return 0;
    }
    char *ENV_CPY = strdup(ENV_PATH);
    char *token = strtok(ENV_CPY, PATH_DELIMITER);
    while (token != NULL) {
        strcpy(full_path, token);
        strcat(full_path, "/");
        strcat(full_path, fn);
        if (_isExecutable(full_path)) {
            free(ENV_CPY);
            return 1;
        }
        token = strtok(NULL, PATH_DELIMITER);
    }
    free(ENV_CPY);
    return 0;
}

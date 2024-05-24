#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "builtins.h"
#include "config.h"
#include "my_utils.h"
#include "prompt.h"

static int __exit(char *[]);
static int _echo(char *[]);
static int _cd(char *[]);
static int _kill(char *[]);
static int _ls(char *[]);
static int _undefined(char *[]);

builtin_pair builtins_table[] = {
    {"exit", &__exit},
    {"lecho", &_echo},
    {"lcd", &_cd},
    {"cd", &_cd},
    {"lkill", &_kill},
    {"lls", &_ls},
    {NULL, NULL}};

static int _die(char *prog) {
    fprintf(stderr, "Builtin %s error.\n", prog);
    return BUILTIN_ERROR;
}

static int __exit(char *argv[]) {
    int argnum = getNullPos(argv) - 1;
    if (argnum > 1) {
        return _die("exit");
    }
    long i = 0;
    if (argv[1] && !myAtoi(argv[1], &i)) {
        return _die("exit");
    }
    exit(i);
}

static int _echo(char *argv[]) {
    int i = 1;
    if (argv[i]) {
        printf("%s", argv[i++]);
    }
    while (argv[i]) {
        printf(" %s", argv[i++]);
    }
    printf("\n");
    fflush(stdout);
    return EXEC_SUCCESS;
}

static int _cd(char *argv[]) {
    int argnum = getNullPos(argv) - 1;
    if (argnum > 1) {
        return _die("lcd");
    }
    if (argv[1] && argv[2]) {
        return _die("lcd");
    }
    if (argv[1]) {
        if (chdir(argv[1]) == -1) {
            return _die("lcd");
        }
        changeCwd();
        return EXEC_SUCCESS;
    }
    argv[1] = home_dir;
    argv[2] = NULL;
    return _cd(argv);
}

static int _kill(char *argv[]) {
    int argnum = getNullPos(argv) - 1;
    if (argnum > 2) {
        return _die("lkill");
    }
    if (!argv[1]) {
        return _die("lkill");
    }
    if (argv[1][0] == '-' && !argv[1][1]) {
        return _die("lkill");
    }
    if (argv[1][0] == '-' && argv[2]) {
        long sig, pid;
        if (!myAtoi(argv[1] + 1, &sig) || !myAtoi(argv[2], &pid)) {
            return _die("lkill");
        }
        if (kill(pid, sig) == -1) {
            return _die("lkill");
        }
    } else if (argv[1][0] != '-' && !argv[2]) {
        long pid;
        if (!myAtoi(argv[1], &pid)) {
            return _die("lkill");
        }
        if (kill(pid, SIGTERM) == -1) {
            return _die("lkill");
        }
    } else {
        return _die("lkill");
    }
    return EXEC_SUCCESS;
}

static int _ls(char *argv[]) {
    int argnum = getNullPos(argv) - 1;
    if (argnum > 0) {
        return _die("lls");
    }
    DIR *directory;
    struct dirent *file;
    if ((directory = opendir(".")) != NULL) {
        while ((file = readdir(directory)) != NULL) {
            if (file->d_name[0] != '.') {
                printf("%s\n", file->d_name);
            }
        }
        closedir(directory);
        fflush(stdout);
    } else {
        return _die("lls");
    }
    return EXEC_SUCCESS;
}

static int _undefined(char *argv[]) {
    fprintf(stderr, "Command %s undefined.\n", argv[0]);
    return BUILTIN_ERROR;
}

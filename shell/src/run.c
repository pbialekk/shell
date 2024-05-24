#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtins.h"
#include "config.h"
#include "my_utils.h"
#include "prompt.h"
#include "read.h"
#include "run.h"
#include "siparse.h"

#define MAX_ARGS MAX_LINE_LENGTH / 2 + 1

char *args[MAX_ARGS];

pid_t last_cmd_pid;
volatile sig_atomic_t last_cmd_status;
volatile sig_atomic_t active_foreground = 0;

void run_sigchldHandler(int sig) {
    int old_errno = errno;
    pid_t child;
    int status;
    while ((child = waitpid(-1, &status, WNOHANG)) > 0) {
        if (!bgjobFinished(child, status)) {
            active_foreground--;
            if (child == last_cmd_pid) {
                last_cmd_status = status;
            }
        }
    }
    errno = old_errno;
}
pid_t run_command(command *com, int in, int useless_in, int out, int bgjob, int call_builtins) {
    if (com == NULL) {
        return 0;
    }
    prepareArgsArray(args, com);
    if (call_builtins && callBuiltin(args[0], args) != -1) {
        return 0;
    }
    pid_t child_pid;
    if ((child_pid = fork()) == 0) {
        if (bgjob) {
            setsid();
        }
        restoreSigactions();
        if (useless_in != STDIN_FILENO) {
            close(useless_in);
        }
        if (in != STDIN_FILENO) {
            dup2(in, STDIN_FILENO);
            close(in);
        }
        if (out != STDOUT_FILENO) {
            dup2(out, STDOUT_FILENO);
            close(out);
        }
        if (processRedirs(com)) {
            execvp(args[0], args);
            printError(args[0], 1); // execvp can fail
        }
        exit(EXEC_FAILURE);
    } else if (child_pid > 0) {
        if (in != STDIN_FILENO) {
            close(in);
        }
        if (bgjob) {
            newBgjob(child_pid);
        } else {
            active_foreground++;
        }
        return child_pid;
    } else {
        fprintf(stderr, "%s\n", FORK_FAIL);
        exit(EXEC_FAILURE);
    }
}

void run_pipeline(pipeline *ln) {
    commandseq *commands = ln->commands;
    int in = STDIN_FILENO;
    int fd[2];
    int bgjob = ln->flags & INBACKGROUND;
    int call_builtins = 1;
    while (commands->next != ln->commands) {
        if (pipe(fd) < 0) {
            fprintf(stderr, "%s\n", PIPE_FAIL);
            exit(EXEC_FAILURE);
        }
        call_builtins = 0;
        run_command(commands->com, in, fd[0], fd[1], bgjob, call_builtins);
        close(fd[1]);
        in = fd[0];
        commands = commands->next;
    }
    last_cmd_status = -1;
    last_cmd_pid = run_command(commands->com, in, STDIN_FILENO, STDOUT_FILENO, bgjob, call_builtins);
    while (active_foreground) { // wait for all children to die
        sigsuspend(&EMPTY_SIGSET);
    }
    if (!bgjob && is_a_tty && last_cmd_status != -1 && WIFSIGNALED(last_cmd_status) && WTERMSIG(last_cmd_status) == SIGINT) {
        printf("\n");
    }
}

int _properCommand(command *ln) {
    if (!isExecutable(ln->args->arg) && !isBuiltin(ln->args->arg)) {
        printError(ln->args->arg, 0);
        return 0;
    }
    redirseq *ln_r = ln->redirs;
    if (ln_r != NULL) {
        do {
            if (IS_RIN(ln_r->r->flags) && !isReadable(ln_r->r->filename)) {
                printError(ln_r->r->filename, 0);
                return 0;
            }
            ln_r = ln_r->next;
        } while (ln_r != ln->redirs);
    }
    return 1;
}

int _properPipeline(pipeline *ln) {
    commandseq *ln_p = ln->commands;
    int len = 0, empty = 0;
    do {
        if (ln_p->com == NULL) {
            empty = 1;
        } else if (!_properCommand(ln_p->com)) { // command not found // wrong redir
            return 0;
        }
        len++;
        if (len > 1 && empty) { // empty string inside of pipeline
            return 2;
        }
        ln_p = ln_p->next;
    } while (ln_p != ln->commands);
    return 1;
}

int _properPipelineseq(pipelineseq *ln) {
    pipelineseq *ln_p = ln;
    do {
        int r = _properPipeline(ln_p->pipeline);
        if (r == 2 || r == 0) {
            return r;
        }
        ln_p = ln_p->next;
    } while (ln_p != ln);
    return 1;
}

void run_pipelineseq(pipelineseq *ln) {
    pipelineseq *ln_p = ln;
    int r = _properPipelineseq(ln);
    if (r == 2) { // empty string inside of pipeline
        fprintf(stderr, "%s\n", SYNTAX_ERROR_STR);
        return;
    } else if (r == 0) {
        return;
    }
    do {
        run_pipeline(ln_p->pipeline);
        ln_p = ln_p->next;
    } while (ln_p != ln);
}

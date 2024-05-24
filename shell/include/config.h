#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MAX_LINE_LENGTH 2048

#define BUF_MAX 16 * MAX_LINE_LENGTH
#define PATH_MAX 4096
#define HOST_NAME_MAX 64
#define HISTORY_STARTSIZE 2

#define EXEC_FAILURE 127

#define EXEC_SUCCESS 0

#define PROMPT_STR "%u at %h in %c\n$ "
#define PROMPT_STR_2 "$ "

#define PATH_DELIMITER ":"
#define SYNTAX_ERROR_STR "Syntax error."
#define WRONG_FILE "no such file or directory"
#define NO_PERMISSIONS "permission denied"
#define EXEC_ERROR "exec error"
#define REDIR_FAIL "unknown redir on file "
#define FORK_FAIL "fork failure."
#define PIPE_FAIL "pipe failure."
#define READ_FAIL "read failure."
#define PROMPT_ERROR "error while getting username/hostname/cwd"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_GOLD "\x1b[33m"
#define ANSI_COLOR_PURPLE "\x1b[35m"
#define ANSI_COLOR_GREEN "\x1b[32m"

#define CTRL_C 3
#define EOT 4
#define EOL 10
#define CTRL_Q 17
#define PRINTABLE_START 32
#define PRINTABLE_END 126
#define ESCAPE 27
#define ARROW_BLOCK_START 91
#define ARROW_UP 65
#define ARROW_DOWN 66
#define ARROW_RIGHT 67
#define ARROW_LEFT 68
#define BACKSPACE 127

#endif /* !_CONFIG_H_ */

#ifndef _PROMPT_UTILS_H_
#define _PROMPT_UTILS_H_

void prompt_init();
void changeCwd();
void prompt_print();

extern int is_a_tty;
extern char *home_dir;

#endif /* !_PROMPT_UTILS_H_ */

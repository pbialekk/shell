#ifndef _HISTORY_H_
#define _HISTORY_H_

void history_init();
void history_add(char *, int);
void history_arrowUp();
void history_arrowDown();
char *history_getEntry();
void history_resetPtr();
int history_isPtrReset();

#endif /* !_HISTORY_H_ */

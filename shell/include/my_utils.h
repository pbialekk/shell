#ifndef _MY_UTILS_H_
#define _MY_UTILS_H_

#include "siparse.h"
#include "stdio.h"

int min(int, int);
int max(int, int);
void prepareArgsArray(char **, const command *);
int myAtoi(const char *, long *);
void printError(char *, int);
int redirectIn(char *);
int redirectOut(char *, int);
int processRedirs(command *);
int isBuiltin(char *);
int callBuiltin(char *, char **);
int getNullPos(char **);
void newBgjob(pid_t pid);
int bgjobFinished(pid_t, int);
void blockSigchld();
void unblockSigchld();
void restoreSigactions();
void prepareEverything();
void processDeadChildren();
int isReadable(char *);
int isExecutable(char *);

extern sigset_t EMPTY_SIGSET;

#endif /* !_MY_UTILS_H_ */

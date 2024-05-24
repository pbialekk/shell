#ifndef _RUN_H_
#define _RUN_H_

#include "siparse.h"

void run_sigchldHandler(int);
pid_t run_command(command *, int, int, int, int, int);
void run_pipeline(pipeline *);
void run_pipelineseq(pipelineseq *);

#endif /* !_RUN_H_ */

#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>

#include "config.h"
#include "my_utils.h"
#include "prompt.h"
#include "read.h"
#include "run.h"
#include "siparse.h"

int main(int argc, char *argv[]) {
    prepareEverything();
    while (1) {
        prompt_print();
        pipelineseq *ln = read_newLine();
        if (ln == NULL) {
            fprintf(stderr, "%s\n", SYNTAX_ERROR_STR);
            continue;
        }
        run_pipelineseq(ln);
    }
    return EXEC_SUCCESS;
}

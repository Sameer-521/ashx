#include "exec.h"
#include "lexer.h"
#include "line.h"
#include <stdio.h>
#include <stdlib.h>

void ashx_loop(void) {
  char *line;
  TokensBuffer *buf;
  int status;

  do {
    printf("$ ");
    line = ashx_readline();
    buf = tokenize(line);
    status = ashx_execute(buf->tokens);

    free(line);
    // i'll use an arena allocator in the future
    for (int i = 0; i < buf->tokens_written; i++) {
      free(buf->tokens[i]);
    }
  } while (status);
}

int main(int argc, char *argv[]) {
  ashx_loop();

  return 0;
}

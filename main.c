#include "exec.h"
#include "lexer.h"
#include "line.h"
#include <stdio.h>
#include <stdlib.h>

void ashx_loop(void) {
  struct getline_out input;
  TokenizeOut tokenized;
  int status;

  do {
    printf("$ ");
    input = ashx_readline();
    tokenized = tokenize(input);
    status = ashx_execute(tokenized.tokens_buf.tokens);

    free(input.line);
    free(tokenized.data);
  } while (status);
}

int main(int argc, char *argv[]) {
  ashx_loop();

  return 0;
}

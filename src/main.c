#include "alloc.h"
#include "exec.h"
#include "lexer.h"
#include "line.h"
#include <stdio.h>
#include <stdlib.h>

void ashx_loop(void) {
  GetLineOut input;
  TokenizeOut tokenized;
  char **args;
  int status, i;

  do {
    printf("$ ");
    input = ashx_readline();
    tokenized = tokenize(input);
    args = malloc(sizeof(char *) * (tokenized.token_arr.size + 1));
    if (!args) {
      fprintf(stderr, "ashx: allocation failed\n");
      exit(EXIT_FAILURE);
    }

    for (i = 0; i < tokenized.token_arr.size; i++) {
      args[i] = tokenized.token_arr.tokens[i].str_repr;
    }
    args[i] = NULL;
    status = ashx_execute(args);

    arr_destroy(&(tokenized.token_arr));
    free(input.line);
    free(tokenized.data);
    free(args);
  } while (status);
}

int main(int argc, char *argv[]) {
  ashx_loop();

  return 0;
}

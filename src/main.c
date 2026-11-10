#include "alloc.h"
#include "exec.h"
#include "lexer.h"
#include "line.h"
#include "parser.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define TEST

void ashx_loop(void) {
  GetLineOut input;
  TokenizeOut tokenized;
  CmdListOut list_out;
  char **args;
  int status;
  size_t i;

  do {
    printf("$ ");
    input = ashx_readline();
    tokenized = tokenize(input);
    list_out = parse_tokens(&(tokenized.token_arr));
    if (list_out.error.had_error) {
      printf("ashx syntax error: expected %d found %d\n",
             list_out.error.expected, list_out.error.cur_token);
    }
    args = malloc(sizeof(char *) * (tokenized.token_arr.size + 1));
    if (!args) {
      fprintf(stderr, "ashx: allocation failed\n");
      exit(EXIT_FAILURE);
    }

    for (i = 0; i < tokenized.token_arr.size; i++) {
      args[i] = tokenized.token_arr.tokens[i].str_repr;
    }
    args[i] = NULL;

#ifndef TEST
    status = ashx_execute(args);
#else
    status = 1;
#endif
    if (!list_out.error.had_error)
      parser_free_list(&(list_out.cmd_list));
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

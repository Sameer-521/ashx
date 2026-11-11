#include "alloc.h"
// #include "exec.h"
#include "lexer.h"
#include "line.h"
#include "parser.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void ashx_loop(void) {
  GetLineOut input;
  TokenizeOut tokenized;
  CmdListOut list_out;

  CharArenaCreateOut char_arena_out = char_arena_create(CHAR_ARENA_BLOCK_SIZE);
  if (!char_arena_out.success) {
    fprintf(stderr, "fatal: arena allocation failed\n");
    exit(EXIT_FAILURE);
  }

  do {
    printf("$ ");
    input = ashx_readline();

    tokenized = tokenize(input, &(char_arena_out.arena));
    if (!tokenized.success) {
      fprintf(stderr, "fatal: failed to tokenize input\n");
      exit(EXIT_FAILURE);
    }

    list_out = parse_tokens(&(tokenized.token_arr));
    if (list_out.error.had_error)
      parser_print_error(&(list_out.error));

    // LET DESTRUCTION RAIN!
    free(input.line);
    if (!list_out.error.had_error)
      parser_free_list(&(list_out.cmd_list));

    arr_destroy(&(tokenized.token_arr));
    char_arena_reset(&(char_arena_out.arena), true);
  } while (1);
}

int main(int argc, char *argv[]) {
  ashx_loop();

  return 0;
}

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
  Arena arena;
  CharArena char_arena;
  CmdListOut list_out;

  CharArenaCreateOut char_arena_out = char_arena_create(CHAR_ARENA_BLOCK_SIZE);
  if (!char_arena_out.success) {
    fprintf(stderr, "fatal: arena allocation failed\n");
    exit(EXIT_FAILURE);
  }
  char_arena = char_arena_out.arena;

  ArenaCreateOut arena_out = arena_create(ARENA_BLOCK_SIZE);
  if (!arena_out.success) {
    fprintf(stderr, "fatal: arena allocation failed\n");
    exit(EXIT_FAILURE);
  }
  arena = arena_out.arena;

  do {
    printf("$ ");
    input = ashx_readline();

    tokenized = tokenize(input, &char_arena);
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
    char_arena_reset(&char_arena, true);
    arena_reset(&arena, true);
  } while (1);
}

int main(int argc, char *argv[]) {
  ashx_loop();

  return 0;
}

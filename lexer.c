#include "alloc.h"
#include "lexer.h"
#include "line.h"
#include "token.h"
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>

typedef struct {
  Arena arena;
  uint written;
} TextBuffer;

enum TokenizerState {
  NORMAL = 0,
  IN_WORD = 1,
  IN_QOUTE = 2,
};

static int write_ch(TextBuffer *t, char ch) {

  if (arena_write(&(t->arena), ch) == 0) {
    t->written++;
    return 0;
  }

  return 1;
}

static int flush_text_to_token(TextBuffer *t, TokenArray *arr, TokenType type) {
  char *arena_end, *token_start;

  write_ch(t, '\0');
  arena_end = t->arena.data + t->arena.used;
  token_start = arena_end - t->written;

  t->written = 0;

  uint offset = token_start - t->arena.data;
  // str_repr is null until the str pointers are computed in case
  // the arena rellocs and the block is moved
  Token new = {.str_repr = NULL, .str_offset = offset, .type = type};

  if (arr_append(arr, new) != SUCCESS) {
    fprintf(stderr, "ashx: could not append new token to array");
    return 1;
  }

  return 0;
}

TokenizeOut tokenize(struct getline_out input) {
  int ch, state = NORMAL;
  uint position = 0;
  TextBuffer text = {0};
  TokenizeOut out = {0};
  TokenArray token_arr;
  TokenArrayOut arr_out = arr_create(DEFAULT_NUM_TOKENS);

  if (arr_out.status != SUCCESS) {
    fprintf(stderr, "ashx: could not initialize tokens array\n");
    return out;
  }

  token_arr = arr_out.arr;

  text.arena = arena_create(ARENA_BLOCK_SIZE);

  ch = input.line[position];

  while (ch != '\0') {
    switch (ch) {
    case ' ':
      if (state == IN_WORD) {
        flush_text_to_token(&text, &token_arr, TOKEN_WORD);
        state = NORMAL;
        continue;
      }

      if (state == IN_QOUTE)
        write_ch(&text, ch);
      break;

    case '\n':
      if (state == IN_WORD) {
        flush_text_to_token(&text, &token_arr, TOKEN_WORD);
        state = NORMAL;
        break;
      }

      if (state == IN_QOUTE)
        write_ch(&text, ch);
      break;

    case '\'':
      if (state == IN_QOUTE) {
        flush_text_to_token(&text, &token_arr, TOKEN_QUOTED);
        state = NORMAL;
        break;
      }
      state = IN_QOUTE;
      break;

    default:
      if (state == IN_QOUTE) {
        write_ch(&text, ch);
      } else {
        state = IN_WORD;
        write_ch(&text, ch);
      }
      break;
    }
    ch = input.line[++position];
  }

  // compute str_view ptrs
  Token *tok;
  for (int i = 0; i < token_arr.size; i++) {
    tok = &(token_arr.tokens[i]);
    tok->str_repr = text.arena.data + tok->str_offset;
  }

  out.token_arr = token_arr;
  out.data = text.arena.data;

  print_token_array(&token_arr);

  return out;
}

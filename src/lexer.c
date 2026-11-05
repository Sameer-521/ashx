#include "alloc.h"
#include "lexer.h"
#include "line.h"
#include "token.h"
#include <stddef.h>
#include <stdio.h>

typedef struct {
  Arena arena;
  size_t written;
} TextBuffer;

enum TokenizerState {
  NORMAL = 0,
  IN_WORD,
  IN_QUOTE,

  // Single & Multi-character Operator States
  IN_GREATER_THAN,    // '>'
  IN_APPEND,          // '>>'
  IN_LESS_THAN,       // '<'
  IN_HEREDOC,         // '<<'
  IN_PIPE,            // '|'
  IN_OR,              // '||'
  IN_AMPERSAND,       // '&'
  IN_AND,             // '&&'
  IN_SEMICOLON,       // ';'
  IN_DOUBLE_SEMICOLON // ';;'
};

static const TokenType state_to_token_type[] = {
    [NORMAL] = TOKEN_WORD,
    [IN_WORD] = TOKEN_WORD,
    [IN_QUOTE] = TOKEN_QUOTED,

    [IN_GREATER_THAN] = TOKEN_GREATER_THAN,
    [IN_APPEND] = TOKEN_APPEND,
    [IN_LESS_THAN] = TOKEN_LESS_THAN,
    [IN_HEREDOC] = TOKEN_HEREDOC,
    [IN_PIPE] = TOKEN_PIPE,
    [IN_OR] = TOKEN_OR,
    [IN_AMPERSAND] = TOKEN_AMPERSAND,
    [IN_AND] = TOKEN_AND,
    [IN_SEMICOLON] = TOKEN_SEMICOLON,
    [IN_DOUBLE_SEMICOLON] = TOKEN_DOUBLE_SEMICOLON};

static int write_ch(TextBuffer *t, char ch) {

  if (arena_write(&(t->arena), ch) == 0) {
    t->written++;
    return 0;
  }

  return 1;
}

static int flush_text_to_token(TextBuffer *t, TokenArray *arr, TokenType type) {
  write_ch(t, '\0');

  size_t offset = t->arena.used - t->written;

  t->written = 0;
  // str_repr is null until the str pointers are computed in case
  // the arena rellocs and the block is moved
  Token new = {.str_repr = NULL, .str_offset = offset, .type = type};

  if (arr_append(arr, new) != SUCCESS) {
    fprintf(stderr, "ashx: could not append new token to array");
    return 1;
  }

  return 0;
}

TokenizeOut tokenize(GetLineOut input) {
  int ch, state = NORMAL;
  size_t position = 0;
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
      if (state == IN_QUOTE)
        write_ch(&text, ch);
      else if (state == NORMAL)
        ;
      else {
        flush_text_to_token(&text, &token_arr, state_to_token_type[state]);
        state = NORMAL;
      }
      break;

    case '\n':
      if (state == IN_WORD) {
        flush_text_to_token(&text, &token_arr, TOKEN_WORD);
        state = NORMAL;
        break;
      }

      if (state == IN_QUOTE)
        write_ch(&text, ch);
      break;

    case '\'':
      if (state == IN_QUOTE) {
        flush_text_to_token(&text, &token_arr, TOKEN_QUOTED);
        state = NORMAL;
        break;
      }
      state = IN_QUOTE;
      break;

    case '|':
      if (state == IN_QUOTE) {
        write_ch(&text, ch);
      } else if (state == IN_PIPE) {
        write_ch(&text, ch);
        flush_text_to_token(&text, &token_arr, TOKEN_OR);
        state = NORMAL;
      } else {
        if (text.written > 0)
          flush_text_to_token(&text, &token_arr, state_to_token_type[state]);
        write_ch(&text, ch);
        state = IN_PIPE;
      }
      break;

    default:
      if (state == IN_QUOTE) {
        write_ch(&text, ch);
      } else {
        if (state != NORMAL && state != IN_WORD && text.written > 0)
          flush_text_to_token(&text, &token_arr, state_to_token_type[state]);
        write_ch(&text, ch);
        state = IN_WORD;
      }
      break;
    }
    ch = input.line[++position];
  }

  // compute str_view ptrs
  Token *tok;
  for (size_t i = 0; i < token_arr.size; i++) {
    tok = &(token_arr.tokens[i]);
    tok->str_repr = text.arena.data + tok->str_offset;
  }

  out.token_arr = token_arr;
  out.data = text.arena.data;

  print_token_array(&token_arr);

  return out;
}

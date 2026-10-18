#include "alloc.h"
#include "lexer.h"
#include "line.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

typedef struct {
  Arena arena;
  uint written;
} TextBuffer;

enum TokenizerState {
  NORMAL = 0,
  WORD = 1,
  IN_QOUTE = 2,
};

int _write_ch(TextBuffer *t, char ch) {

  if (arena_write(&(t->arena), ch) == 0) {
    t->written++;
    return 0;
  }

  return 1;
}

int _finalize_write(TextBuffer *t, TokensBuffer *tok_buf) {
  char *arena_end, *token_start;

  _write_ch(t, '\0');
  arena_end = t->arena.data + t->arena.used;
  token_start = arena_end - t->written;
  tok_buf->tokens[tok_buf->tokens_written] = token_start;
  tok_buf->tokens_written++;

  t->written = 0;

  // add token buffer resizing logic

  return 0;
}

TokenizeOut tokenize(struct getline_out input) {
  int ch, state = NORMAL;
  uint position = 0;
  TokensBuffer tokens_buf = {.bufsize = NUM_TOKENS};
  TextBuffer text = {0};
  TokenizeOut out = {0};

  text.arena = arena_create(ARENA_BLOCK_SIZE);
  tokens_buf.tokens = malloc(tokens_buf.bufsize * sizeof(char *));
  if (tokens_buf.tokens == NULL) {
    fprintf(stderr, "ashx: allocation error\n");
    exit(EXIT_FAILURE);
  }
  ch = input.line[position];

  while (ch != '\0') {
    switch (ch) {
    case ' ':
      if (state == WORD) {
        _finalize_write(&text, &tokens_buf);
        state = NORMAL;
        continue;
      }

      if (state == IN_QOUTE)
        _write_ch(&text, ch);
      break;

    case '\n':
      if (state == WORD) {
        _finalize_write(&text, &tokens_buf);
        state = NORMAL;
        break;
      }

      if (state == IN_QOUTE)
        _write_ch(&text, ch);
      break;

    case '\'':
      if (state == IN_QOUTE) {
        _finalize_write(&text, &tokens_buf);
        state = NORMAL;
        continue;
      }
      state = IN_QOUTE;
      break;

    default:
      if (state == IN_QOUTE) {
        _write_ch(&text, ch);
      } else {
        state = WORD;
        _write_ch(&text, ch);
      }
      break;
    }
    ch = input.line[++position];
  }

  tokens_buf.tokens[tokens_buf.tokens_written] = NULL;

  out.tokens_buf = tokens_buf;
  out.data = text.arena.data;

  return out;
}

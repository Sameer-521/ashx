#include "lexer.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef struct text_buffer {
  char *buf;
  uint chars_written;
  uint bufsize;
} TextBuffer;

enum TokenizerState {
  NORMAL = 0,
  WORD = 1,
  IN_QOUTE = 2,
};

int _write_realloc(TextBuffer *t, char ch) {
  char *tmp;

  if (t->chars_written + 1 >= t->bufsize) {
    tmp = realloc(t->buf, t->bufsize * 2);
    if (tmp == NULL) {
      fprintf(stderr, "ashx: allocation error\n");
      exit(EXIT_FAILURE);
    }

    t->buf = tmp;
  }

  t->buf[t->chars_written++] = ch;

  return 0;
}

int _finalize_write(TextBuffer *text_buf, TokensBuffer *tok_buf) {
  char *tmp;
  _write_realloc(text_buf, '\0');

  tmp = malloc(text_buf->chars_written * sizeof(char));
  if (tmp == NULL) {
    fprintf(stderr, "ashx: allocation error\n");
    exit(EXIT_FAILURE);
  }

  memcpy(tmp, text_buf->buf, text_buf->chars_written);
  tok_buf->tokens[tok_buf->tokens_written++] = tmp;

  if (text_buf->bufsize > TOK_BUFSIZE) {
    // resize to default capacity
    tmp = realloc(text_buf->buf, TOK_BUFSIZE);

    if (tmp == NULL) {
      fprintf(stderr, "ashx: allocation error\n");
      exit(EXIT_FAILURE);
    }
    text_buf->buf = tmp;
    text_buf->bufsize = TOK_BUFSIZE;
  }

  text_buf->chars_written = 0;

  return 0;
}

TokensBuffer *tokenize(char *line) {
  int ch, state = NORMAL;
  uint position = 0, num_tokens = NUM_TOKENS;
  TextBuffer text = {.chars_written = 0, .bufsize = TOK_BUFSIZE};
  TokensBuffer *tokens_buf;

  tokens_buf = malloc(sizeof(TokensBuffer));
  if (tokens_buf == NULL) {
    fprintf(stderr, "ashx: allocation error\n");
    exit(EXIT_FAILURE);
  }

  tokens_buf->tokens = malloc(num_tokens * sizeof(char *));
  if (tokens_buf->tokens == NULL) {
    fprintf(stderr, "ashx: allocation error\n");
    exit(EXIT_FAILURE);
  }

  text.buf = malloc(text.bufsize * sizeof(char));
  if (text.buf == NULL) {
    fprintf(stderr, "ashx: allocation error\n");
    exit(EXIT_FAILURE);
  }

  ch = line[position];

  while (ch != '\0') {
    switch (ch) {
    case ' ':
      if (state == WORD) {
        _finalize_write(&text, tokens_buf);
        state = NORMAL;
        continue;
      }

      if (state == IN_QOUTE)
        _write_realloc(&text, ch);
      break;

    case '\n':
      if (state == WORD) {
        _finalize_write(&text, tokens_buf);
        state = NORMAL;
        break;
      }

      if (state == IN_QOUTE)
        _write_realloc(&text, ch);
      break;

    case '\'':
      if (state == IN_QOUTE) {
        _finalize_write(&text, tokens_buf);
        state = NORMAL;
        continue;
      }
      state = IN_QOUTE;
      break;

    default:
      if (state == IN_QOUTE) {
        _write_realloc(&text, ch);
      } else {
        state = WORD;
        _write_realloc(&text, ch);
      }
      break;
    }
    ch = line[++position];
  }

  free(text.buf);
  tokens_buf->tokens[tokens_buf->tokens_written] = NULL;
  return tokens_buf;
}

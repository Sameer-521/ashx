#ifndef LEXER_H
#define LEXER_H

#include "line.h"
#include <sys/types.h>

#define KiB(x) (x) * 1024
#define MAX_CHARS KiB(4) 
#define NUM_TOKENS 8
#define TOK_BUFSIZE 64


typedef struct tokens_buffer {
  char **tokens;
  uint tokens_written;
  uint bufsize;
} TokensBuffer;

typedef struct tokenize_out {
  TokensBuffer tokens_buf;
  char *data;
} TokenizeOut;

typedef struct token {
  char *str;
  uint len;
} Token;

TokenizeOut tokenize(struct getline_out input);

#endif

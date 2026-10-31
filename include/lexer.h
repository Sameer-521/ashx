#ifndef LEXER_H
#define LEXER_H

#include "alloc.h"
#include "line.h"
#include <stddef.h>

#define KiB(x) (x) * 1024
#define MAX_CHARS KiB(4) 
#define NUM_TOKENS 8
#define DEFAULT_NUM_TOKENS 32
#define TOK_BUFSIZE 64


typedef struct tokens_buffer {
  char **tokens;
  size_t tokens_written;
  size_t bufsize;
} TokensBuffer;

typedef struct tokenize_out {
  TokenArray token_arr;
  char *data;
} TokenizeOut;

TokenizeOut tokenize(GetLineOut input);

#endif

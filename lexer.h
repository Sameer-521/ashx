#ifndef LEXER_H
#define LEXER_H

#include <sys/types.h>

#define NUM_TOKENS 8
#define TOK_BUFSIZE 64


typedef struct tokens_buffer {
  char **tokens;
  uint tokens_written;
  uint bufsize;
} TokensBuffer;

TokensBuffer *tokenize(char *line);

#endif

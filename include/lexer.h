#ifndef LEXER_H
#define LEXER_H

#include "alloc.h"
#include "line.h"
#include <stdbool.h>

#define DEFAULT_NUM_TOKENS 32

typedef struct tokenize_out {
  TokenArray token_arr;
  bool success;
} TokenizeOut;

TokenizeOut tokenize(GetLineOut input, CharArena *arena);

#endif

#ifndef TOKEN_H
#define TOKEN_H

#include <sys/types.h>

typedef enum {
  TOKEN_WORD = 0,
  TOKEN_QUOTED = 1,
  TOKEN_OPERATOR = 2,
} TokenType;

typedef struct token {
  char *str_repr;
  uint str_offset;
  TokenType type;
} Token;

#endif

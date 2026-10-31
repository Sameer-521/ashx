#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum {
  TOKEN_WORD = 0,
  TOKEN_QUOTED = 1,
  TOKEN_OPERATOR = 2,
} TokenType;

typedef struct token {
  char *str_repr;
  size_t str_offset;
  TokenType type;
} Token;

#endif

#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum {
    TOKEN_WORD = 0,
    TOKEN_QUOTED,
    TOKEN_GREATER_THAN,    // '>'
    TOKEN_APPEND,          // '>>'
    TOKEN_LESS_THAN,       // '<'
    TOKEN_HEREDOC,         // '<<'
    TOKEN_PIPE,            // '|'
    TOKEN_OR,              // '||'
    TOKEN_AMPERSAND,       // '&'
    TOKEN_AND,             // '&&'
    TOKEN_SEMICOLON,       // ';'
    TOKEN_DOUBLE_SEMICOLON // ';;'
} TokenType;


typedef struct token {
  char *str_repr;
  size_t str_offset;
  TokenType type;
} Token;

#endif

#ifndef ALLOC_H
#define ALLOC_H

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include "token.h"

#define ARENA_BLOCK_SIZE 256
#define ARENA_RESIZE_FACTOR 2

// arena forward declarations and types
typedef struct arena {
  char *data;
  size_t size;
  size_t used;
} Arena;

Arena arena_create(size_t size);

int arena_write(Arena *arena, char ch);

void arena_destroy(Arena *arena);


// token array forward declarations and types
typedef struct token_array {
  Token *tokens;
  size_t size;
  size_t cap;
} TokenArray;

typedef enum {
    SUCCESS = 0,
    FAILED = 1,
    ARR_CREATION_FAILED = 2,
    ARR_ERR_NO_MEM = 3,
} TokenArrayStatus; 

typedef struct {
  TokenArray arr;
  TokenArrayStatus status;
} TokenArrayOut;

TokenArrayOut arr_create(size_t cap);

TokenArrayStatus arr_append(TokenArray *arr, Token token);

void arr_destroy(TokenArray *arr);

void print_token_array(const TokenArray *arr);

#endif

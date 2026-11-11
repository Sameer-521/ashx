#ifndef ALLOC_H
#define ALLOC_H

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include "token.h"

#define CHAR_ARENA_BLOCK_SIZE 256
#define CHAR_ARENA_RESIZE_FACTOR 2

// char_arena forward declarations and types
typedef struct {
  char *data;
  size_t size;
  size_t used;
} CharArena;

typedef struct {
  bool success;
  CharArena arena;
} CharArenaCreateOut;

CharArenaCreateOut char_arena_create(size_t size);

int char_arena_write(CharArena *arena, char ch);

void char_arena_destroy(CharArena *arena);

bool char_arena_reset(CharArena *arena, bool resize_default);

// token array forward declarations and types
typedef struct {
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

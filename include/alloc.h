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

// Genric-Arena forward declarations and types
#define ARENA_ALIGN 8
#define ARENA_BLOCK_SIZE 256
#define ARENA_RESIZE_FACTOR 2

typedef struct {
  void *data;
  size_t used;
  size_t size;
} Arena;

typedef struct {
  bool success;
  Arena arena;
} ArenaCreateOut;

typedef struct {
  bool success;
  void *ptr;
  size_t offset;
} ArenaAllocOut;

typedef enum {
  ARENA_SUCCESS,
  ARENA_FAILED,
  ARENA_NO_MEM,
} ArenaStatus;

ArenaCreateOut arena_create(size_t size);

ArenaAllocOut arena_alloc(Arena *arena, size_t size);

ArenaStatus arena_grow(Arena *arena, size_t size);

ArenaStatus arena_write(Arena *arena, void *src, size_t size);

void arena_clear(Arena *arena);

void arena_destroy(Arena *arena);

bool arena_reset(Arena *arena, bool resize_default);

#endif

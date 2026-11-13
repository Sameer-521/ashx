#include "alloc.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>

// TODO: add better error handling AND fix bugs!

// ARENA
CharArenaCreateOut char_arena_create(size_t size) {
  char *tmp;
  CharArenaCreateOut out = {0};

  tmp = malloc(size * sizeof(char));
  if (tmp == NULL)
    return out;

  out.arena.data = tmp;
  out.arena.size = size;
  out.arena.used = 0;
  out.success = true;

  return out;
}

int char_arena_write(CharArena *arena, char ch) {
  char *tmp;

  if (arena->used + 1 > arena->size) {
    // fprintf(stderr, "ashx: arena full\n");
    tmp = realloc(arena->data, arena->size * CHAR_ARENA_RESIZE_FACTOR);
    if (!tmp) {
      fprintf(stderr, "ashx: allocation failed\n");
      return 1;
    }

    arena->data = tmp;
    arena->size *= CHAR_ARENA_RESIZE_FACTOR;
  }

  arena->data[arena->used++] = ch;
  return 0;
}

void char_arena_destroy(CharArena *arena) { free(arena->data); }

bool char_arena_reset(CharArena *arena, bool resize_default) {
  char *tmp;

  if (resize_default && arena->size > CHAR_ARENA_BLOCK_SIZE) {
    tmp = realloc(arena->data, CHAR_ARENA_BLOCK_SIZE);
    if (!tmp) {
      fprintf(stderr, "ashx: arena resize failed\n");
      return false;
    }
    arena->size = CHAR_ARENA_BLOCK_SIZE;
  }

  arena->used = 0;
  return true;
}

// TOKEN_ARRAY IMPLEMENTATIONS
TokenArrayOut arr_create(size_t cap) {
  TokenArray arr = {0};
  TokenArrayOut out = {0};

  arr.tokens = malloc(sizeof(Token) * cap);
  if (!arr.tokens) {
    out.arr = arr;
    out.status = ARR_CREATION_FAILED;
    return out;
  }

  arr.cap = cap;
  arr.size = 0;

  out.arr = arr;
  out.status = SUCCESS;
  return out;
}

TokenArrayStatus arr_append(TokenArray *arr, Token token) {
  Token *tmp;
  size_t old_cap = sizeof(Token) * arr->cap;

  if (arr->size + 1 >= arr->cap) {
    tmp = realloc(arr->tokens, old_cap * 2);

    if (!tmp)
      return ARR_ERR_NO_MEM;

    arr->tokens = tmp;
  }

  arr->tokens[arr->size++] = token;
  return SUCCESS;
}

void arr_destroy(TokenArray *arr) { free(arr->tokens); }

void print_token_array(const TokenArray *arr) {
  if (arr == NULL || arr->tokens == NULL) {
    printf("TokenArray is empty or uninitialized.\n");
    return;
  }

  printf("TokenArray (Size: %u, Capacity: %u):\n", arr->size, arr->cap);

  for (unsigned int i = 0; i < arr->size; i++) {
    Token t = arr->tokens[i];

    // Convert enum to a readable string
    const char *type_str;
    switch (t.type) {
    case TOKEN_WORD:
      type_str = "TOKEN_WORD";
      break;
    case TOKEN_QUOTED:
      type_str = "TOKEN_QUOTED";
      break;
    case TOKEN_NEWLINE:
      type_str = "TOKEN_NEWLINE";
      break;
    case TOKEN_GREATER_THAN:
      type_str = "TOKEN_GREATER_THAN";
      break;
    case TOKEN_APPEND:
      type_str = "TOKEN_APPEND";
      break;
    case TOKEN_LESS_THAN:
      type_str = "TOKEN_LESS_THAN";
      break;
    case TOKEN_PIPE:
      type_str = "TOKEN_PIPE";
      break;
    case TOKEN_OR:
      type_str = "TOKEN_OR";
      break;
    case TOKEN_AMPERSAND:
      type_str = "TOKEN_AMPERSAND";
      break;
    case TOKEN_AND:
      type_str = "TOKEN_AND";
      break;
    case TOKEN_SEMICOLON:
      type_str = "TOKEN_SEMICOLON";
      break;
    case TOKEN_STDERR_REDIRECTION:
      type_str = "TOKEN_STDERR_REDIRECTION";
      break;
    case TOKEN_STDERR_TO_1_REDIRECTION:
      type_str = "TOKEN_STDERR_TO_1_REDIRECTION";
      break;
    default:
      type_str = "UNKNOWN";
      break;
    }
    if (t.type == TOKEN_NEWLINE) {
      printf("  [%u] str_repr: \"\\n\" | str_offset: %u | type: %s\n", i,
             t.str_offset, type_str);
    } else {
      printf("  [%u] str_repr: \"%s\" | str_offset: %u | type: %s\n", i,
             t.str_repr ? t.str_repr : "(null)", t.str_offset, type_str);
    }
  }
}

// ARENA IMPLEMENTATIONS
// great guide btw https://eliasebner.com/blog/guides/arena-allocators-in-c/
//
ArenaCreateOut arena_create(size_t size) {
  ArenaCreateOut out = {0};

  out.arena.data = malloc(size);
  if (out.arena.data == NULL)
    return out;

  out.arena.size = size;
  out.arena.used = 0;
  out.success = true;

  return out;
}

ArenaAllocOut arena_alloc(Arena *arena, size_t size) {
  ArenaAllocOut out = {0};
  size_t alignment;

  if (!arena || !arena->data) {
    fprintf(stderr, "fatal: arena not initialized\n");
    exit(EXIT_FAILURE);
  }

  alignment = (ARENA_ALIGN - (arena->used % ARENA_ALIGN)) % ARENA_ALIGN;
  // so each allocation starts at current addr + alignment? noiiice!

  out.ptr = (char *)arena->data + arena->used + alignment;
  out.offset = arena->used + alignment;
  if (out.offset + size > arena->size) {
    return out;
  }

  arena->used += size + alignment;
  out.success = true;

  return out;
}

ArenaStatus arena_grow(Arena *arena, size_t size) {
  void *tmp;

  if (!arena || !arena->data) {
    fprintf(stderr, "fatal: arena not initialized\n");
    exit(EXIT_FAILURE);
  }

  tmp = realloc(arena->data, arena->size);
  if (!tmp) {
    return ARENA_NO_MEM;
  }

  arena->data = tmp;
  arena->size = size;

  return ARENA_SUCCESS;
}

ArenaStatus arena_write(Arena *arena, void *src, size_t size) {
  ArenaAllocOut alloc_out;

  if (!arena || !arena->data) {
    fprintf(stderr, "fatal: arena not initialized\n");
    exit(EXIT_FAILURE);
  }

  alloc_out = arena_alloc(arena, size);
  if (!alloc_out.success) {
    if (arena_grow(arena, arena->size * ARENA_RESIZE_FACTOR) != ARENA_SUCCESS) {
      fprintf(stderr, "fatal: arena realloc failed\n");
      exit(EXIT_FAILURE);
    }
  }

  memcpy(alloc_out.ptr, src, size);

  return ARENA_SUCCESS;
}

void arena_clear(Arena *arena) { arena->used = 0; }

void arena_destroy(Arena *arena) {
  free(arena->data);
  arena->data = NULL;
  arena->size = 0;
  arena->used = 0;
}

bool arena_reset(Arena *arena, bool resize_default) {
  void *tmp;

  if (resize_default && arena->size > ARENA_BLOCK_SIZE) {
    tmp = realloc(arena->data, CHAR_ARENA_BLOCK_SIZE);
    if (!tmp) {
      fprintf(stderr, "ashx: arena resize failed\n");
      return false;
    }
    arena->size = ARENA_BLOCK_SIZE;
  }

  arena->used = 0;
  return true;
}

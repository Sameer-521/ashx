#include "alloc.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>

// ARENA
Arena arena_create(size_t size) {
  char *tmp;
  Arena arena = {0};

  tmp = malloc(size * sizeof(char));
  if (tmp == NULL) {
    fprintf(stderr, "ashx: allocation error\n");
    exit(EXIT_FAILURE);
  }

  arena.data = tmp;
  arena.size = size;
  arena.used = 0;

  return arena;
}

int arena_write(Arena *arena, char ch) {
  char *tmp;

  if (arena->used + sizeof(char) > arena->size) {
    // fprintf(stderr, "ashx: arena full\n");
    tmp = realloc(arena->data, arena->size * ARENA_RESIZE_FACTOR);
    if (!tmp) {
      fprintf(stderr, "ashx: allocation failed\n");
      return 1;
    }

    arena->data = tmp;
    arena->size *= ARENA_RESIZE_FACTOR;
  }

  arena->data[arena->used++] = ch;
  return 0;
}

void arena_destroy(Arena *arena) { free(arena->data); }

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

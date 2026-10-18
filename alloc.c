#include "alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

Arena arena_create(uint size) {
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
  if (arena->used + sizeof(char) > arena->size) {
    fprintf(stderr, "ashx: arena full\n");
    return 1;
  }

  arena->data[arena->used++] = ch;
  return 0;
}

void arena_destroy(Arena *arena) { free(arena->data); }

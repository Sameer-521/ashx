#ifndef ALLOC_H
#define ALLOC_H

#include <stdlib.h>
#include <sys/types.h>
#include <stdbool.h>

#define ARENA_BLOCK_SIZE 1024

struct slab {
  void *data;
  struct slab *next;
};

typedef struct arena {
  char *data;
  uint size;
  uint used;
} Arena;

Arena arena_create(uint size);

int arena_write(Arena *arena, char ch);

void arena_destroy(Arena *arena);

#endif

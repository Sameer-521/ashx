#ifndef LINE_H
#define LINE_H

#include <stddef.h>

typedef struct {
  char *line;
  size_t len;
} GetLineOut;

GetLineOut ashx_readline(void);

#endif

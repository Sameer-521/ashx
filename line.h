#ifndef LINE_H
#define LINE_H

#include <sys/types.h>

struct getline_out {
  char *line;
  uint len;
};

struct getline_out ashx_readline(void);

#endif

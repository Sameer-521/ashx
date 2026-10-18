#include "line.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

struct getline_out ashx_readline(void) {
  char *line = NULL;
  size_t bufsize = 0;
  struct getline_out out;

  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      // Crtl-d
      exit(EXIT_SUCCESS);
    } else {
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }

  out.line = line;
  out.len = strlen(line);
  return out;
}

#include "line.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define ASHX_TOK_BUFSIZE 64
#define ASHX_TOK_DELIM " \t\r\n\a"

char *ashx_readline(void) {
  char *line = NULL;
  size_t bufsize = 0;

  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);
    } else {
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }

  return line;
}

char **ashx_split_line(char *line) {
  int bufsize = ASHX_TOK_BUFSIZE, position = 0;
  char **tokens, *token;

  tokens = malloc(bufsize * sizeof(char *));

  if (tokens == NULL) {
    fprintf(stderr, "ashx: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, ASHX_TOK_DELIM);
  while (token != NULL) {
    tokens[position++] = token;

    if (position >= bufsize) {
      bufsize += ASHX_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char *));

      if (tokens == NULL) {
        fprintf(stderr, "ashx: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, ASHX_TOK_DELIM);
  }

  tokens[position] = NULL;
  return tokens;
}

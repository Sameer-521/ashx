#include "builtins.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *builtin_str[] = {"cd", "help", "exit"};

int (*builtin_func[])(char **) = {&ashx_cd, &ashx_help, &ashx_exit};

int num_builtins(void) { return sizeof(builtin_str) / sizeof(char *); }

int ashx_cd(char **args) {
  if (args[1] == NULL)
    fprintf(stderr, "ashx: expected argument to \"cd\"\n");
  else {
    if (chdir(args[1]) != 0)
      perror("ashx");
  }

  return 1;
}

int ashx_help(char **args) {
  printf("ashx: a toy shell for unix systems\n");
  printf("Enter program names to execute or builtin commands to run\n");

  for (int i = 0; i < num_builtins(); i++)
    printf("    %s\n", builtin_str[i]);

  return 1;
}

int ashx_exit(char **args) {
  int status = 0;
  const char *str_num = args[1];

  // too many args?
  if (args[2] != NULL) {
    fprintf(stderr, "exit: too many arguments\n");
    return 1;
  }

  // invalid int
  status = atoi(str_num);
  if (status == 0) {
    fprintf(stderr, "exit: %s: invalid integer\n", str_num);
    return 1;
  }

  // complete it later
  return 0;
}

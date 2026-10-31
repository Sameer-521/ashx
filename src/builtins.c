#include "builtins.h"
#include <stdio.h>
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

int ashx_exit(char **args) { return 0; }

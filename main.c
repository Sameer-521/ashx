#include "builtins.h"
#include "line.h"
#include <bits/wait.h>
#include <linux/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int ashx_launch(char **args) {
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    if (execvp(args[0], args) == -1)
      perror("ashx");
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // error forking
    perror("ashx");
  } else {

    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int ashx_execute(char **args) {
  int i;

  if (args[0] == NULL)
    return 1;

  for (i = 0; i < num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0)
      return (*builtin_func[i])(args);
  }

  return ashx_launch(args);
}

void ashx_loop(void) {
  char *line;
  char **args;
  int status;

  do {
    printf("$ ");
    line = ashx_readline();
    args = ashx_split_line(line);
    status = ashx_execute(args);

    free(line);
    free(args);
  } while (status);
}

int main(int argc, char *argv[]) {
  ashx_loop();

  return 0;
}

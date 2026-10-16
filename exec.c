#include "builtins.h"
#include "exec.h"
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

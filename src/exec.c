#include "builtins.h"
#include "exec.h"
#include "parser.h"
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define READ_END 0
#define WRITE_END 1
#define BUILTIN_PID -99

static void apply_redirects(Cmd *cmd) {

  if (cmd->infile != NULL) {
    int in = open(cmd->infile, O_RDONLY);
    dup2(in, STDIN_FILENO);
    close(in);
  }
  if (cmd->outfile != NULL) {
    int out = open(cmd->outfile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    dup2(out, STDOUT_FILENO);
    // perror("dup2");
    close(out);
  }
}

static void run_cmd(Cmd *cmd) {
  // expansion here!

  if (execvp(cmd->argv[0], cmd->argv) == -1) {
    perror("ashx");

    if (errno == ENOENT)
      exit(127);
    else if (errno == EACCES)
      exit(126);
    else
      exit(EXIT_FAILURE);
  }
}

static bool run_builtin(char **argv, pid_t *pids, size_t current) {

  for (int i = 0; i < num_builtins(); i++) {
    if (strcmp(argv[0], builtin_str[i]) == 0) {
      (*builtin_func[i])(argv);
      pids[current] = BUILTIN_PID; // indicator that this is builtin, for now
      return true;
    }
  }

  return false;
}

static int run_pipeline(Pipeline *pipeline) {
  size_t ncmds = pipeline->ncmds;

  int fds[ncmds][2]; // ncmds and not ncmds - 1 cause it can evaluate to
                     // zero and Asan will scream
  for (size_t i = 0; i < ncmds - 1; i++) { // TODO: handle errors
    pipe(fds[i]);
  }

  pid_t pids[ncmds];
  for (size_t i = 0; i < ncmds; i++) {

    // builtin?
    if (run_builtin(pipeline->cmds[i].argv, pids, i))
      continue;

    if ((pids[i] = fork()) == 0) {
      // child
      if (i > 0)
        dup2(fds[i - 1][READ_END], STDIN_FILENO); // read from

      if (i < ncmds - 1)
        dup2(fds[i][WRITE_END], STDOUT_FILENO); // write to

      for (size_t j = 0; j < ncmds - 1; j++) {
        close(fds[j][READ_END]);
        close(fds[j][WRITE_END]);
      }

      apply_redirects(&(pipeline->cmds[i]));

      // should not return
      run_cmd(&(pipeline->cmds[i]));

    } else if (pids[i] < 0) {
      // error forking
      perror("ashx");
      return 1;
    }
  }

  for (size_t j = 0; j < ncmds; j++) {
    close(fds[j][READ_END]);
    close(fds[j][WRITE_END]);
  }

  int status;
  int exit_status[ncmds];
  // pid_t wpid;

  for (size_t i = 0; i < ncmds; i++) {
    if (pids[i] == BUILTIN_PID) {
      continue;
    }

    waitpid(pids[i], &status, WUNTRACED);

    if (WIFEXITED(status)) {
      exit_status[i] = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status))
      exit_status[i] = 128 + WTERMSIG(status);
  }

  return 0;
}

int ashx_run_cmdlist(CmdList *list) {
  for (size_t i = 0; i < list->npls; i++) {
    run_pipeline(&(list->pipelines[i]));
  }

  return 0;
}

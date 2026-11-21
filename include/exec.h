#ifndef EXEC_H
#define EXEC_H

#include <stdbool.h>
#include "parser.h"

typedef enum {
 EXEC_DUP2_FAILED,
 EXEC_OPEN_FAILED,
 EXEC_CLOSE_FAILED,
 EXEC_FORK_FAILED, 
 EXEC_PIPE_FAILED,
 EXEC_INVALID_REDIRECT_TYPE,
 EXEC_INTERNAL_ERR, // generic error for now
} ExecErrorType;

typedef struct {
  bool had_error;
  ExecErrorType type;
  char *cmd_name;
  size_t status;
  size_t current_pl_index; 
  int _errno;
} ExecError;

ExecError ashx_run_cmdlist(CmdList *list);

void print_exec_error(const ExecError *err);

#endif

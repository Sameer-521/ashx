#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include "alloc.h"
#include "token.h"

typedef enum {
  REDIRECT_IN,
  REDIRECT_OUT,
  REDIRECT_OUT_APPEND,
  REDIRECT_ERR_OUT,
  REDIRECT_ERR_OUT_APPEND,
  REDIRECT_ERR_TO_1,
} RedirectType;

typedef struct {
  char *filename;
  RedirectType type;
} Redirect;

typedef struct {
  char **argv;
  Redirect *redirects;
  size_t num_redirects;
} Cmd;

typedef struct {
  Cmd *cmds;
  size_t ncmds;
} Pipeline;

typedef struct {
  Pipeline *pipelines;
  size_t npls; // num of pipelines
  Cmd *cmds;
  size_t ncmds;
} CmdList;

typedef enum {
  PARSER_UNEXPECTED_TOKEN,
  PARSER_INVALID_FILENAME,
  PARSER_INTERNAL_ERROR
} ErrorType;

typedef struct {
  bool had_error;
  TokenType prev_token, cur_token, expected;
  ErrorType error_type;
} ParserError;

typedef struct {
  Cmd cmd;
  ParserError error;
} CmdOut;

typedef struct {
  Pipeline pl;
  ParserError error;
} PipelineOut;


typedef struct {
  CmdList cmd_list;
  ParserError error;
} CmdListOut;

typedef struct {
  size_t npls;
  size_t ncmds;
} ParserPreCountOut;
  
CmdListOut parse_tokens(TokenArray *arr, Arena *arena); 

void parser_free_allocations(CmdList *list);

void parser_print_error(const ParserError *err);

#endif

#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include "alloc.h"
#include "token.h"

typedef struct {
  char **argv;
  char *infile; // read input from file
  char *outfile; // stdout -> file
  char *err_outfile;
  bool out_append; // dont overwrite contents of file?
  bool err_out_append;
} Cmd;

typedef struct {
  Cmd *cmds;
  size_t ncmds;
} Pipeline;

typedef struct {
  Pipeline *pipelines;
  size_t n;
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


CmdListOut parse_tokens(TokenArray *arr); 

void parser_free_list(CmdList *list);

void parser_print_error(const ParserError *err);

#endif

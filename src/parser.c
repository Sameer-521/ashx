#include "alloc.h"
#include "parser.h"
#include "stb_ds.h"
#include "token.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_PARSER_ARR_CAP 8

// TODO: handle redirection parsing

typedef enum { REDIRECT_IN, REDIRECT_OUT } RedirectDirection;

static PipelineOut parse_pipeline(TokenArray *arr, size_t *position);

static CmdOut parse_command(TokenArray *arr, size_t *position);

static void cmdlist_debug_print(const CmdList *list, FILE *out);

static ParserError parse_redirect(TokenArray *arr, size_t *position,
                                  int direction, Cmd *cmd);

static TokenType peek(TokenArray *arr, size_t index) {
  return arr->tokens[index].type;
}

static void advance(size_t *index) { (*index)++; }

static void set_error(TokenType cur, TokenType prev, TokenType expected,
                      ParserError *err, ErrorType error_type) {
  err->had_error = true;
  err->cur_token = cur;
  err->prev_token = prev;
  err->expected = expected;
  err->error_type = error_type;
}

// PARSER IMPLEMENTATIONS
CmdListOut parse_tokens(TokenArray *arr) {
  CmdListOut list_out = {0};
  PipelineOut pl_out = {0};
  size_t position = 0;

  stbds_arrsetcap(list_out.cmd_list.pipelines, DEFAULT_PARSER_ARR_CAP);

  while (peek(arr, position) != TOKEN_NEWLINE) {
    pl_out = parse_pipeline(arr, &position);
    if (pl_out.error.had_error) {
      stbds_arrfree(list_out.cmd_list.pipelines);
      list_out.error = pl_out.error;
      return list_out;
    }

    stbds_arrput(list_out.cmd_list.pipelines, pl_out.pl);
    list_out.cmd_list.n++;

    if (peek(arr, position) == TOKEN_SEMICOLON) {
      advance(&position);
      continue;
    }

    if (peek(arr, position) != TOKEN_NEWLINE) {
      // ERROR
      stbds_arrfree(list_out.cmd_list.pipelines);
      set_error(arr->tokens[position].type,
                position == 0 ? TOKEN_NONE : arr->tokens[position - 1].type,
                TOKEN_NEWLINE, &(list_out.error), PARSER_UNEXPECTED_TOKEN);
      return list_out;
    }
    break;
  }
  // eat up the newline
  advance(&position);

  cmdlist_debug_print(&(list_out.cmd_list), stderr);

  return list_out;
}

PipelineOut parse_pipeline(TokenArray *arr, size_t *position) {
  PipelineOut pl_out = {0};
  CmdOut cmd_out;

  stbds_arrsetcap(pl_out.pl.cmds, DEFAULT_PARSER_ARR_CAP);

  for (;;) {
    cmd_out = parse_command(arr, position);
    if (cmd_out.error.had_error) {
      // free already allocated memory
      for (size_t i = 0; i < pl_out.pl.ncmds; ++i)
        stbds_arrfree(pl_out.pl.cmds[i].argv);

      stbds_arrfree(pl_out.pl.cmds);
      pl_out.error = cmd_out.error;
      return pl_out;
    }

    stbds_arrput(pl_out.pl.cmds, cmd_out.cmd);
    pl_out.pl.ncmds++;

    if (peek(arr, *position) == TOKEN_PIPE) {
      advance(position);
      continue;
    }
    // if next token itsnt a pipe, pipeline ends
    break;
  }

  return pl_out;
}

CmdOut parse_command(TokenArray *arr, size_t *position) {
  CmdOut out = {0};
  ParserError err;
  TokenType tok_type;

  if (peek(arr, *position) != TOKEN_WORD) {
    set_error(arr->tokens[*position].type,
              *position == 0 ? TOKEN_NONE : arr->tokens[*position - 1].type,
              TOKEN_WORD, &(out.error), PARSER_UNEXPECTED_TOKEN);
    return out; // for now
  }

  stbds_arrsetcap(out.cmd.argv, DEFAULT_PARSER_ARR_CAP);
  // set argv[0]
  stbds_arrput(out.cmd.argv, arr->tokens[*position].str_repr);
  advance(position);

  while ((tok_type = peek(arr, *position)) == TOKEN_WORD ||
         tok_type == TOKEN_QUOTED || tok_type == TOKEN_GREATER_THAN ||
         tok_type == TOKEN_LESS_THAN) {
    if (tok_type == TOKEN_GREATER_THAN) {
      advance(position);
      err = parse_redirect(arr, position, REDIRECT_OUT, &(out.cmd));
      if (err.had_error) {
        out.error = err;
        return out;
      }
    } else if (tok_type == TOKEN_LESS_THAN) {
      advance(position);
      err = parse_redirect(arr, position, REDIRECT_IN, &(out.cmd));
      if (err.had_error) {
        out.error = err;
        return out;
      }
    } else {
      stbds_arrput(out.cmd.argv, arr->tokens[*position].str_repr);
      advance(position);
    }
  }
  stbds_arrput(out.cmd.argv, NULL);

  return out;
}

static ParserError parse_redirect(TokenArray *arr, size_t *position,
                                  int direction, Cmd *cmd) {
  ParserError err = {0};
  TokenType tok_type;
  char *filename;

  if ((tok_type = peek(arr, *position)) == TOKEN_WORD ||
      tok_type == TOKEN_QUOTED) {

    // TODO: add better filename validation
    filename = arr->tokens[*position].str_repr;
    if (strlen(filename) == 0) {
      // invalid redirection target
      set_error(arr->tokens[*position].type,
                *position == 0 ? TOKEN_NONE : arr->tokens[*position - 1].type,
                TOKEN_FILENAME, &err, PARSER_INVALID_FILENAME);
      return err;
    }

    switch (direction) {
    case REDIRECT_IN:
      cmd->infile = filename;
      break;
    case REDIRECT_OUT:
      cmd->outfile = filename;
      break;
    default:
      // invalid direction for now
      // TODO: CHANGE THIS LATER
      set_error(arr->tokens[*position].type,
                *position == 0 ? TOKEN_NONE : arr->tokens[*position - 1].type,
                TOKEN_FILENAME, &err, PARSER_INTERNAL_ERROR);
      return err;
    }

    advance(position);
  } else {
    // error: expected filename
    set_error(arr->tokens[*position].type,
              *position == 0 ? TOKEN_NONE : arr->tokens[*position - 1].type,
              TOKEN_FILENAME, &err, PARSER_UNEXPECTED_TOKEN);
    return err;
  }

  return err;
}

void parser_free_list(CmdList *list) {
  Pipeline pl;
  char **argv;

  for (size_t i = 0; i < list->n; ++i) {
    pl = list->pipelines[i];

    for (size_t j = 0; j < pl.ncmds; ++j) {
      argv = pl.cmds[j].argv;
      // printf("freeing: pl[%d].cmds[%d].argv\n", i, j);
      stbds_arrfree(argv);
    }

    // printf("freeing: pl[%d].cmds\n", i);
    stbds_arrfree(pl.cmds);
  }

  // printf("freeing: list.pls\n");
  stbds_arrfree(list->pipelines);
}

static void cmdlist_debug_print(const CmdList *list, FILE *out) {
  if (!list) {
    fprintf(out, "CmdList: (null)\n");
    return;
  }

  fprintf(out, "CmdList: %zu pipeline(s)\n", list->n);

  for (size_t i = 0; i < list->n; i++) {
    const Pipeline *p = &list->pipelines[i];
    fprintf(out, "  Pipeline %zu: %zu cmd(s)\n", i, p->ncmds);

    for (size_t j = 0; j < p->ncmds; j++) {
      const Cmd *c = &p->cmds[j];
      fprintf(out, "    Cmd %zu:", j);

      if (c->argv) {
        for (size_t k = 0; c->argv[k]; k++)
          fprintf(out, " [%s]", c->argv[k]);
      } else {
        fprintf(out, " (no argv)");
      }
      fputc('\n', out);

      fprintf(out, "      infile:  %s\n", c->infile ? c->infile : "(none)");
      fprintf(out, "      outfile: %s\n", c->outfile ? c->outfile : "(none)");
    }
  }
}

static const char *error_type_to_string(ErrorType type) {
  static const char *const error_strings[] = {
      [PARSER_UNEXPECTED_TOKEN] = "Unexpected token",
      [PARSER_INVALID_FILENAME] = "Invalid filename",
      [PARSER_INTERNAL_ERROR] = "Internal error",
  };
  size_t arrlen = (sizeof(error_strings) / sizeof(error_strings[0]));

  if (type < 0 || type >= arrlen || !error_strings[type]) {
    return "Unknown error";
  }

  return error_strings[type];
}

void parser_print_error(const ParserError *err) {
  if (!err || !err->had_error) {
    printf("ParserError: No error recorded.\n");
    return;
  }

  printf("=== Parser Error ===\n");
  printf("  Type:     %s\n", error_type_to_string(err->error_type));
  printf("  Prev Tok: %d\n", err->prev_token);
  printf("  Cur Tok:  %d\n", err->cur_token);
  printf("  Expected: %d\n", err->expected);
  printf("====================\n");
}

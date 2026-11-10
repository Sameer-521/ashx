#include "alloc.h"
#include "parser.h"
#include "stb_ds.h"
#include "token.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_PARSER_ARR_CAP 8

// TODO: handle redirection parsing

static void cmdlist_debug_print(const CmdList *list, FILE *out);

static inline TokenType peek(TokenArray *arr, size_t index) {
  return arr->tokens[index].type;
}

static inline void advance(size_t *index) { (*index)++; }

static void set_error(TokenType cur, TokenType prev, TokenType expected,
                      ParserError *err) {
  err->had_error = true;
  err->cur_token = cur;
  err->prev_token = prev;
  err->expected = expected;
}

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
                TOKEN_NEWLINE, &(list_out.error));
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
  TokenType tok_type;

  if (peek(arr, *position) != TOKEN_WORD) {
    set_error(arr->tokens[*position].type,
              *position == 0 ? TOKEN_NONE : arr->tokens[*position - 1].type,
              TOKEN_WORD, &(out.error));
    return out; // for now
  }

  stbds_arrsetcap(out.cmd.argv, DEFAULT_PARSER_ARR_CAP);
  // set argv[0]
  stbds_arrput(out.cmd.argv, arr->tokens[*position].str_repr);
  advance(position);

  while ((tok_type = peek(arr, *position)) == TOKEN_WORD ||
         tok_type == TOKEN_QUOTED) {
    stbds_arrput(out.cmd.argv, arr->tokens[*position].str_repr);
    advance(position);
  }
  stbds_arrput(out.cmd.argv, NULL);

  return out;
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

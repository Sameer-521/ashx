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

// TODO: add token_type TOKEN_ERROR_APPEND
#define IS_REDIRECT(token_type)                                                \
  (token_type == TOKEN_GREATER_THAN || token_type == TOKEN_LESS_THAN ||        \
   token_type == TOKEN_STDERR_REDIRECTION ||                                   \
   token_type == TOKEN_STDERR_TO_1_REDIRECTION ||                              \
   token_type == TOKEN_APPEND ||                                               \
   token_type == TOKEN_STDERR_REDIRECTION_APPEND)

// TODO: handle redirection parsing

static PipelineOut parse_pipeline(TokenArray *arr, size_t *position,
                                  CmdList *list);

static CmdOut parse_command(TokenArray *arr, size_t *position);

static ParserError parse_redirect(TokenArray *arr, size_t *position,
                                  TokenType type, Cmd *cmd);

ParserPreCountOut parser_precount_cmdlist(const TokenArray *arr);

ParserError consume_filename(TokenArray *arr, size_t *position,
                             char **filename);

static void cmdlist_debug_print(const CmdList *list, FILE *out);

static void advance(size_t *index) { (*index)++; }

static TokenType peek(TokenArray *arr, size_t index) {
  return arr->tokens[index].type;
}

static void set_error(TokenType cur, TokenType prev, TokenType expected,
                      ParserError *err, ErrorType error_type) {
  err->had_error = true;
  err->cur_token = cur;
  err->prev_token = prev;
  err->expected = expected;
  err->error_type = error_type;
}

// TODO: Enhance later
static void init_arrays(Arena *arena, CmdList *list,
                        ParserPreCountOut *nsizes) {
  ArenaAllocOut alloc_out;
  size_t total_size = 0, n;

  n = sizeof(Pipeline) * nsizes->npls;
  n += ALIGNMENT(n, 8);

  total_size += n;

  n = sizeof(Cmd) * nsizes->ncmds;
  n += ALIGNMENT(n, 8);

  total_size += n;

  if (!arena_has_space(arena, total_size)) {
    if (arena_grow(arena, total_size) != ARENA_SUCCESS) {
      fprintf(stderr, "ashx: fatal: arena realloc failed.\n");
      exit(EXIT_FAILURE);
    }
  }
  // guranteed to fit now!
  alloc_out = arena_alloc(arena, sizeof(Pipeline) * nsizes->npls);
  list->pipelines = (Pipeline *)alloc_out.ptr;

  alloc_out = arena_alloc(arena, sizeof(Cmd) * nsizes->ncmds);
  list->cmds = (Cmd *)alloc_out.ptr;
}

// PARSER IMPLEMENTATIONS
CmdListOut parse_tokens(TokenArray *arr, Arena *arena) {
  CmdListOut list_out = {0};
  CmdList cmd_list = {0};
  PipelineOut pl_out = {0};

  ParserPreCountOut nsizes = parser_precount_cmdlist(arr);
  // printf("Precount: npls: %d ncmds: %d\n", nsizes.npls, nsizes.ncmds);

  init_arrays(arena, &cmd_list, &nsizes);

  size_t position = 0;
  while (peek(arr, position) != TOKEN_NEWLINE) {
    pl_out = parse_pipeline(arr, &position, &cmd_list);
    if (pl_out.error.had_error) {
      list_out.error = pl_out.error;
      return list_out;
    }

    cmd_list.pipelines[cmd_list.npls] = pl_out.pl;
    cmd_list.npls++;

    if (peek(arr, position) == TOKEN_SEMICOLON) {
      advance(&position);
      continue;
    }

    if (peek(arr, position) != TOKEN_NEWLINE) {
      // ERROR
      set_error(arr->tokens[position].type,
                position == 0 ? TOKEN_NONE : arr->tokens[position - 1].type,
                TOKEN_NEWLINE, &(list_out.error), PARSER_UNEXPECTED_TOKEN);
      return list_out;
    }
    break;
  }
  // eat up the newline
  advance(&position);

  cmdlist_debug_print(&cmd_list, stderr);

  list_out.cmd_list = cmd_list;
  return list_out;
}

PipelineOut parse_pipeline(TokenArray *arr, size_t *position, CmdList *list) {
  PipelineOut pl_out = {0};
  CmdOut cmd_out;

  pl_out.pl.cmds = &(list->cmds[list->ncmds]);

  for (;;) {
    cmd_out = parse_command(arr, position);
    if (cmd_out.error.had_error) {
      // free already allocated memory
      for (size_t i = 0; i < pl_out.pl.ncmds; ++i) {
        stbds_arrfree(pl_out.pl.cmds[i].argv);
        stbds_arrfree(pl_out.pl.cmds[i].redirects);
      }

      pl_out.error = cmd_out.error;
      return pl_out;
    }

    pl_out.pl.cmds[pl_out.pl.ncmds] = cmd_out.cmd;
    pl_out.pl.ncmds++;
    list->ncmds++;

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

  stbds_arrsetcap(out.cmd.redirects, DEFAULT_PARSER_ARR_CAP);
  while ((tok_type = peek(arr, *position)) == TOKEN_WORD ||
         tok_type == TOKEN_QUOTED || IS_REDIRECT(tok_type)) {

    if (IS_REDIRECT(tok_type)) {
      advance(position);
      err = parse_redirect(arr, position, tok_type, &(out.cmd));
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
                                  TokenType type, Cmd *cmd) {
  ParserError err = {0};
  RedirectType redirect_type;
  char *filename = NULL;

  static const RedirectType token_to_redirect[] = {
      [TOKEN_GREATER_THAN] = REDIRECT_OUT,
      [TOKEN_LESS_THAN] = REDIRECT_IN,
      [TOKEN_STDERR_REDIRECTION] = REDIRECT_ERR_OUT,
      [TOKEN_STDERR_REDIRECTION_APPEND] = REDIRECT_ERR_OUT_APPEND,
      [TOKEN_STDERR_TO_1_REDIRECTION] = REDIRECT_ERR_TO_1,
      [TOKEN_APPEND] = REDIRECT_OUT_APPEND,
  };

  redirect_type = token_to_redirect[type];

  // `2>&1` takes no filename
  if (redirect_type != REDIRECT_ERR_TO_1) {
    err = consume_filename(arr, position, &filename);
    if (err.had_error)
      return err;
  }

  Redirect redirect = {.filename = filename, .type = redirect_type};
  stbds_arrput(cmd->redirects, redirect);
  cmd->num_redirects++;

  return err;
}

ParserError consume_filename(TokenArray *arr, size_t *position,
                             char **filename) {
  ParserError err = {0};
  TokenType tok_type;

  if ((tok_type = peek(arr, *position)) == TOKEN_WORD ||
      tok_type == TOKEN_QUOTED) {

    *filename = arr->tokens[*position].str_repr;
    if (strlen(*filename) == 0) {
      // invalid redirection target
      set_error(arr->tokens[*position].type,
                *position == 0 ? TOKEN_NONE : arr->tokens[*position - 1].type,
                TOKEN_FILENAME, &err, PARSER_MISSING_FILENAME);
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

ParserPreCountOut parser_precount_cmdlist(const TokenArray *arr) {
  ParserPreCountOut out = {.npls = 1, .ncmds = 1};
  TokenType type;

  for (size_t i = 0; i < arr->size; i++) {
    type = arr->tokens[i].type;
    switch (type) {
    case TOKEN_PIPE:
      out.ncmds++;
      break;

    case TOKEN_SEMICOLON:
    case TOKEN_AMPERSAND:
    case TOKEN_OR:
      out.npls++;
      out.ncmds++;
      break;

    default:
      break;
    }
  }

  return out;
}

void parser_free_allocations(CmdList *list) {
  char **argv;
  Redirect *redirects;

  for (size_t i = 0; i < list->ncmds; ++i) {
    argv = list->cmds[i].argv;
    stbds_arrfree(argv);
    redirects = list->cmds[i].redirects;
    stbds_arrfree(redirects);
    // printf("freeing: pl[%d].cmds[%d].argv\n", i, j);
  }
}

static const char *redirect_type_to_string(RedirectType type) {
  switch (type) {
  case REDIRECT_IN:
    return "in";
  case REDIRECT_OUT:
    return "out";
  case REDIRECT_OUT_APPEND:
    return "out-append";
  case REDIRECT_ERR_OUT:
    return "err-out";
  case REDIRECT_ERR_OUT_APPEND:
    return "err-out-append";
  case REDIRECT_ERR_TO_1:
    return "err-to-1";
  }
  return "unknown";
}

static void cmdlist_debug_print(const CmdList *list, FILE *out) {
  if (!list) {
    fprintf(out, "CmdList: (null)\n");
    return;
  }

  fprintf(out, "CmdList: %zu pipeline(s), %zu cmd(s)\n", list->npls,
          list->ncmds);

  for (size_t i = 0; i < list->npls; i++) {
    const Pipeline *p = &list->pipelines[i];
    fprintf(out, "  Pipeline %zu: %zu cmd(s)\n", i, p->ncmds);

    for (size_t j = 0; j < p->ncmds; j++) {
      const Cmd *c = &p->cmds[j];
      size_t global = (size_t)(c - list->cmds);

      fprintf(out, "    Cmd %zu (global %zu):", j, global);

      if (c->argv) {
        for (size_t k = 0; c->argv[k]; k++)
          fprintf(out, " [%s]", c->argv[k]);
      } else {
        fprintf(out, " (no argv)");
      }
      fputc('\n', out);

      for (size_t k = 0; k < c->num_redirects; k++) {
        const Redirect *r = &c->redirects[k];
        if (r->type == REDIRECT_ERR_TO_1) {
          fprintf(out, "      redirect: %s\n",
                  redirect_type_to_string(r->type));
        } else {
          fprintf(out, "      redirect: %s -> %s\n",
                  redirect_type_to_string(r->type),
                  r->filename ? r->filename : "(null)");
        }
      }
    }
  }
}

static const char *error_type_to_string(ErrorType type) {
  static const char *const error_strings[] = {
      [PARSER_UNEXPECTED_TOKEN] = "Unexpected token",
      [PARSER_MISSING_FILENAME] = "Missing filename",
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

#include "alloc.h"
#include "lexer.h"
#include "line.h"
#include "token.h"
#include <stddef.h>
#include <stdio.h>

// TODO add position field in Token struct to track
//  orginal column index of token
// FEATURE: add logic to recognize redirections

typedef struct {
  CharArena *arena;
  size_t written;
} TextBuffer;

typedef enum {
  NORMAL = 0,
  IN_WORD,
  IN_QUOTE,

  // Single & Multi-character Operator States
  IN_GREATER_THAN, // '>'
  IN_APPEND,       // '>>'
  IN_LESS_THAN,    // '<'
  IN_PIPE,         // '|'
  IN_OR,           // '||'
  IN_AMPERSAND,    // '&'
  IN_AND,          // '&&'
  IN_SEMICOLON,    // ';'
} TokenizerState;

static const TokenType state_to_token_type[] = {
    [NORMAL] = TOKEN_WORD,
    [IN_WORD] = TOKEN_WORD,
    [IN_QUOTE] = TOKEN_QUOTED,
    [IN_GREATER_THAN] = TOKEN_GREATER_THAN,
    [IN_APPEND] = TOKEN_APPEND,
    [IN_LESS_THAN] = TOKEN_LESS_THAN,
    [IN_PIPE] = TOKEN_PIPE,
    [IN_OR] = TOKEN_OR,
    [IN_AMPERSAND] = TOKEN_AMPERSAND,
    [IN_AND] = TOKEN_AND,
    [IN_SEMICOLON] = TOKEN_SEMICOLON,
};

static int write_ch(TextBuffer *t, char ch) {
  if (char_arena_write(t->arena, ch) == 0) {
    t->written++;
    return 0;
  }
  return 1;
}

static int flush_text_to_token(TextBuffer *t, TokenArray *arr, TokenType type) {
  if (t->written == 0)
    return 0;

  write_ch(t, '\0');
  size_t offset = t->arena->used - t->written;
  t->written = 0;

  Token new = {.str_repr = NULL, .str_offset = offset, .type = type};
  if (arr_append(arr, new) != SUCCESS) {
    fprintf(stderr, "ashx: could not append new token to array\n");
    return 1;
  }
  return 0;
}

static int emit_if_needed(TextBuffer *t, TokenArray *arr,
                          TokenizerState state) {
  if (state == NORMAL || t->written == 0)
    return 0;
  return flush_text_to_token(t, arr, state_to_token_type[state]);
}

TokenizeOut tokenize(GetLineOut input, CharArena *arena) {
  TextBuffer text = {0};
  TokenizeOut out = {0};
  TokenizerState state = NORMAL;

  if (!arena) {
    fprintf(stderr, "ashx: arena not initialized\n");
    return out;
  }
  text.arena = arena;

  TokenArray token_arr;
  TokenArrayOut arr_out = arr_create(DEFAULT_NUM_TOKENS);
  if (arr_out.status != SUCCESS) {
    fprintf(stderr, "ashx: could not initialize tokens array\n");
    return out;
  }
  token_arr = arr_out.arr;

  size_t position = 0;
  char ch = input.line[position];

  while (ch != '\0') {
    switch (ch) {
    case ' ':
    case '\t':
      if (state == IN_QUOTE) {
        write_ch(&text, ch);
      } else if (state != NORMAL) {
        emit_if_needed(&text, &token_arr, state);
        state = NORMAL;
      }
      break;

    case '\n':
      if (state == IN_QUOTE) {
        write_ch(&text, ch);
      } else {
        emit_if_needed(&text, &token_arr, state);
        write_ch(&text, ch);
        flush_text_to_token(&text, &token_arr, TOKEN_NEWLINE);
        state = NORMAL;
      }
      break;

    case '\'':
      if (state == IN_QUOTE) {
        flush_text_to_token(&text, &token_arr, TOKEN_QUOTED);
        state = NORMAL;
      } else {
        emit_if_needed(&text, &token_arr, state);
        state = IN_QUOTE;
      }
      break;

    case '>':
      if (state == IN_QUOTE) {
        write_ch(&text, ch);
      } else if (state == IN_GREATER_THAN) {
        write_ch(&text, ch);
        flush_text_to_token(&text, &token_arr, TOKEN_APPEND);
        state = NORMAL;
      } else {
        emit_if_needed(&text, &token_arr, state);
        write_ch(&text, ch);
        state = IN_GREATER_THAN;
      }
      break;

    case '<':
      if (state == IN_QUOTE) {
        write_ch(&text, ch);
      } else {
        emit_if_needed(&text, &token_arr, state);
        write_ch(&text, ch);
        state = IN_LESS_THAN;
      }
      break;

    case '|':
      if (state == IN_QUOTE) {
        write_ch(&text, ch);
      } else if (state == IN_PIPE) {
        write_ch(&text, ch);
        flush_text_to_token(&text, &token_arr, TOKEN_OR);
        state = NORMAL;
      } else {
        emit_if_needed(&text, &token_arr, state);
        write_ch(&text, ch);
        state = IN_PIPE;
      }
      break;

    case '&':
      if (state == IN_QUOTE) {
        write_ch(&text, ch);
      } else if (state == IN_AMPERSAND) {
        write_ch(&text, ch);
        flush_text_to_token(&text, &token_arr, TOKEN_AND);
        state = NORMAL;
      } else {
        emit_if_needed(&text, &token_arr, state);
        write_ch(&text, ch);
        state = IN_AMPERSAND;
      }
      break;

    case ';':
      if (state == IN_QUOTE) {
        write_ch(&text, ch);
      } else {
        emit_if_needed(&text, &token_arr, state);
        write_ch(&text, ch);
        state = IN_SEMICOLON;
      }
      break;

    case '2':

      if (state == IN_QUOTE || state == IN_WORD) {
        write_ch(&text, ch);
      } else if (state == NORMAL && position != 0) {

        if (((position + 3) < input.len) && input.line[position + 1] == '>' &&
            input.line[position + 2] == '&' &&
            input.line[position + 3] == '1') {

          write_ch(&text, ch);
          write_ch(&text, '>');
          write_ch(&text, '&');
          write_ch(&text, '1');
          position += 3;
          // ch = '1'
          flush_text_to_token(&text, &token_arr, TOKEN_STDERR_TO_1_REDIRECTION);
          state = NORMAL;
        } else if (((position + 1) < input.len) &&
                   input.line[position + 1] == '>') {

          write_ch(&text, ch);
          write_ch(&text, '>');
          position++;
          // ch = '>'
          flush_text_to_token(&text, &token_arr, TOKEN_STDERR_REDIRECTION);
          state = NORMAL;
        }
      } else {
        emit_if_needed(&text, &token_arr, state);
        write_ch(&text, ch);
        state = IN_WORD;
      }
      break;

    default:
      if (state == IN_QUOTE) {
        write_ch(&text, ch);
      } else {
        if (state != IN_WORD && state != NORMAL) {
          // single char operator followed by char word
          flush_text_to_token(&text, &token_arr, state_to_token_type[state]);
          state = NORMAL;
        }
        if (state != IN_WORD)
          state = IN_WORD;
        write_ch(&text, ch);
      }
      break;
    }

    ch = input.line[++position];
  }

  // emit final token in case newline was consumed by
  // unterminated quotes
  emit_if_needed(&text, &token_arr, state);

  // compute str_view ptrs
  Token *tok;
  for (size_t i = 0; i < token_arr.size; i++) {
    tok = &(token_arr.tokens[i]);
    tok->str_repr = text.arena->data + tok->str_offset;
  }

  out.token_arr = token_arr;
  out.success = true;

  print_token_array(&token_arr);

  return out;
}

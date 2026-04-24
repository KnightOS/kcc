#include "KCCCleanup.h"

#include "SDCCglobl.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct options options;

static const char *allowed_commands[] = {
    ".equ",  ".globl",  ".area",   ".org",    ".map",     ".db",      ".ds",
    ".dw",   ".ascii",  ".asciip", ".asciiz", ".if",      ".ifdef",   ".ifndef",
    ".elif", ".elseif", ".end",    ".module", ".optsdcc", ".function"};

static const size_t allowed_commands_count =
    sizeof(allowed_commands) / sizeof(allowed_commands[0]);

/* Check if a character is whitespace (space or tab), matching the original. */
static int is_ws(char c) { return c == ' ' || c == '\t'; }

/* Trim leading/trailing spaces and tabs, and collapse internal runs of
 * whitespace into a single space. Returns a malloc'd string (possibly empty).
 * Caller frees. */
static char *reduce_line(const char *src) {
  size_t len = strlen(src);
  /* find first non-ws */
  size_t begin = 0;
  while (begin < len && is_ws(src[begin]))
    begin++;
  if (begin == len) {
    char *out = (char *)malloc(1);
    if (out)
      out[0] = '\0';
    return out;
  }
  /* find last non-ws */
  size_t end = len;
  while (end > begin && is_ws(src[end - 1]))
    end--;

  /* Worst case: no collapsing needed, output size = end - begin + 1 */
  char *out = (char *)malloc(end - begin + 1);
  if (!out)
    return NULL;
  size_t oi = 0;
  size_t i = begin;
  while (i < end) {
    if (is_ws(src[i])) {
      out[oi++] = ' ';
      while (i < end && is_ws(src[i]))
        i++;
    } else {
      out[oi++] = src[i++];
    }
  }
  out[oi] = '\0';
  return out;
}

/* Read a line from fp. Returns malloc'd string without trailing newline, or
 * NULL on EOF (with nothing read). Caller frees. */
static char *read_line(FILE *fp) {
  size_t cap = 128;
  size_t len = 0;
  char *buf = (char *)malloc(cap);
  if (!buf)
    return NULL;
  int c;
  int got_any = 0;
  while ((c = fgetc(fp)) != EOF) {
    got_any = 1;
    if (c == '\n')
      break;
    if (len + 1 >= cap) {
      size_t new_cap = cap * 2;
      char *nb = (char *)realloc(buf, new_cap);
      if (!nb) {
        free(buf);
        return NULL;
      }
      buf = nb;
      cap = new_cap;
    }
    buf[len++] = (char)c;
  }
  if (!got_any) {
    free(buf);
    return NULL;
  }
  buf[len] = '\0';
  return buf;
}

static int is_allowed_command(const char *s) {
  for (size_t i = 0; i < allowed_commands_count; i++) {
    if (strcmp(s, allowed_commands[i]) == 0)
      return 1;
  }
  return 0;
}

void cleanupFile(const char *file) {
  FILE *asm_file = fopen(file, "r");
  if (!asm_file) {
    printf("Error: file \"%s\" doesn't exist!\n", file);
    return;
  }

  /* Dynamic array of char* for buffered lines. */
  size_t buf_cap = 128;
  size_t buf_len = 0;
  char **buffer = (char **)malloc(buf_cap * sizeof(char *));
  if (!buffer) {
    fclose(asm_file);
    return;
  }

  int dirty = 0;

  /* Keep the first 4 lines verbatim. */
  for (int i = 0; i < 4; i++) {
    char *l = read_line(asm_file);
    if (l == NULL) {
      /* Fewer than 4 lines; push empty string to mirror std::getline
       * behavior of leaving o_line empty on EOF. */
      l = (char *)malloc(1);
      if (l)
        l[0] = '\0';
    }
    if (buf_len == buf_cap) {
      buf_cap *= 2;
      char **nb = (char **)realloc(buffer, buf_cap * sizeof(char *));
      if (!nb) {
        free(l);
        goto cleanup;
      }
      buffer = nb;
    }
    buffer[buf_len++] = l ? l : (char *)calloc(1, 1);
  }

  /* Process the rest. */
  char *o_line;
  while ((o_line = read_line(asm_file)) != NULL) {
    char *line = reduce_line(o_line);
    if (!line) {
      free(o_line);
      continue;
    }
    /* token = substring before first space */
    char *space = strchr(line, ' ');
    if (space)
      *space = '\0';

    int keep = 0;
    size_t line_len = strlen(line);

    if (line[0] != ';') {
      int cond = 0;
      if (line[0] != '.') {
        cond = 1;
      } else if (line_len > 0 && line[line_len - 1] == ':') {
        cond = 1;
      } else if (is_allowed_command(line)) {
        cond = 1;
      }

      if (cond) {
        /* If previous buffered line starts with "\t.area" and this o_line
         * also starts with "\t.area", drop the previous buffered one. */
        if (buf_len != 0) {
          const char *last_line = buffer[buf_len - 1];
          size_t last_len = strlen(last_line);
          size_t o_len = strlen(o_line);
          if (last_len > 6 && strncmp(last_line, "\t.area", 6) == 0 &&
              o_len > 6 && strncmp(o_line, "\t.area", 6) == 0) {
            free(buffer[buf_len - 1]);
            buf_len--;
          }
        }
        keep = 1;
      } else if (options.verbose) {
        fprintf(stderr, "Discarded: %s\n", o_line);
      }
    }

    free(line);

    if (keep) {
      if (buf_len == buf_cap) {
        buf_cap *= 2;
        char **nb = (char **)realloc(buffer, buf_cap * sizeof(char *));
        if (!nb) {
          free(o_line);
          goto cleanup;
        }
        buffer = nb;
      }
      buffer[buf_len++] = o_line;
    } else {
      free(o_line);
      dirty = 1;
    }
  }

  fclose(asm_file);
  asm_file = NULL;

  if (dirty) {
    FILE *output = fopen(file, "w");
    if (!output) {
      printf("Error opening file for output!\n");
      goto cleanup;
    }
    for (size_t i = 0; i < buf_len; i++) {
      fputs(buffer[i], output);
      fputc('\n', output);
    }
    fclose(output);
  }

cleanup:
  if (asm_file)
    fclose(asm_file);
  for (size_t i = 0; i < buf_len; i++)
    free(buffer[i]);
  free(buffer);
}

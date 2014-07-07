// Copyright 2011 RWTH Aachen University. All rights reserved.
//
// Licensed under the RWTH ASR License (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/*
 * TODO:
 *
 * - better reconstruction of comments
 * - test/improve robustness against broken input
 */

#define CHUNK_SIZE  1024


static int  depth = 0;
static char last_action = '\0';

/*****************************************************************************/
static void init(void)
/*****************************************************************************/
{
  depth = 0;
  last_action = '\0';
}

/*****************************************************************************/
static void print_declaration(void)
/*****************************************************************************/
{
  printf("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
}

/*****************************************************************************/
static void print_depth(void)
/*****************************************************************************/
{
  int i;
  for (i = 0; i < depth / 4; i++) putchar('\t');
  for (i = 0; i < depth % 4; i++) fputs("  ", stdout);
}

/*****************************************************************************/
static char* encode_special_chars(const char *input)
/*****************************************************************************/
{
  char *out = NULL;
  char *buffer = NULL;
  int buffer_size = 0;
  const char *cur = input;

  if (input == NULL) return NULL;

  buffer_size = 1000;
  buffer = malloc(buffer_size);
  out = buffer;

  while (*cur != '\0') {
    if (out - buffer > buffer_size - 10) {
      int index = out - buffer;
      buffer_size *= 2;
      buffer = realloc(buffer, buffer_size);
      out = &buffer[index];
    }

    /*
     * by default one have to encode at least '<', '>', '"' and '&' !
     */
    if (*cur == '<') {
      *out++ = '&'; *out++ = 'l'; *out++ = 't'; *out++ = ';';
    } else if (*cur == '>') {
      *out++ = '&'; *out++ = 'g'; *out++ = 't'; *out++ = ';';
    } else if (*cur == '&') {
      *out++ = '&'; *out++ = 'a'; *out++ = 'm'; *out++ = 'p'; *out++ = ';';
#if 0
    } else if (*cur == '"') {
      *out++ = '&'; *out++ = 'q'; *out++ = 'u'; *out++ = 'o'; *out++ = 't';
      *out++ = ';';
#endif
    } else {
      /*
       * works because on UTF-8, all extended sequences cannot
       * result in bytes in the ASCII range.
       */
      *out++ = *cur;
    }
    cur++;
  }
  *out++ = 0;

  return buffer;
}

/*****************************************************************************/
static void process_line(const char *line)
/*****************************************************************************/
{
  int len;
  char *l, *begin, *end;
  char *tag = NULL, action = '\0', *name = NULL, *value = NULL, *tmp = NULL;

  len = strlen(line) + 1;
  l = malloc(len);
  memcpy(l, line, len);
  strtok(l, " ");
  begin = strtok(NULL, " "); /* skip path */

  /* extract tag */
  if ((end = strtok(NULL, " ")) != NULL) {
    tag = malloc(end - begin + 1);
    memcpy(tag, begin, end - begin);
    tag[end - begin] = '\0';
  }
  begin = end;

  /* extract action */
  action = begin[0];

  switch(action) {
  case 'S':
    if ((last_action == 'S') || (last_action == 'A')) fputs(">\n", stdout);
    print_depth();
    printf("<%s", tag);
    depth++;
    break;
  case 'A':
    name = strtok(NULL, " ");
    value = name;
    while ((*value != '\0') && (!isspace(*value))) value++;
    value += 1;
    while ((*value != '\0') && (isspace(*value))) value++;
    tmp = encode_special_chars(value);
    printf(" %s=\"%s\"", name, tmp);
    if (tmp) free(tmp);
    break;
  case 'E':
    depth--;
    if ((last_action == 'S') || (last_action == 'A')) fputs("/>\n", stdout);
    else {
      print_depth();
      printf("</%s>\n", tag);
    }
    break;
  case 'C':
    if ((last_action == 'S') || (last_action == 'A')) fputs(">\n", stdout);
    while ((*begin != '\0') && (!isspace(*begin))) begin++;
    begin += 1;
    while ((*begin != '\0') && (isspace(*begin))) begin++;
    print_depth();
    tmp = encode_special_chars(begin);
    printf("%s\n", tmp);
    if (tmp) free(tmp);
    break;
  case '!':
    if ((last_action == 'S') || (last_action == 'A')) fputs(">\n", stdout);
    while ((*begin != '\0') && (!isspace(*begin))) begin++;
    begin += 1;
    print_depth();
    fputs("<!-- ", stdout);
    printf(begin);
    fputs(" -->\n", stdout);
    break;
  case '?':
    if ((last_action == 'S') || (last_action == 'A')) fputs(">\n", stdout);
    while ((*begin != '\0') && (!isspace(*begin))) begin++;
    begin += 1;
    while ((*begin != '\0') && (isspace(*begin))) begin++;
    print_depth();
    fputs("<?", stdout);
    printf(begin);
    fputs("?>\n", stdout);
    break;
  default:
    break;
  }

  last_action = action;

  if (tag) free(tag);
  free(l);
}

/*****************************************************************************/
static char* read_line(FILE *f)
/*****************************************************************************/
{
  int i;
  char *line, *ptr, *nl;

  i = 1;
  ptr = line = malloc(CHUNK_SIZE + 1);
  while (fgets(ptr, CHUNK_SIZE + 1, f) != NULL) {
    if ((nl = strchr(ptr, '\n')) != NULL) {
      *nl = '\0';
      return line;
    }
    line = realloc(line, (i + 1) * CHUNK_SIZE + 1);
    ptr = line + i * CHUNK_SIZE;
    i++;
  }

  if (i > 1) return line;

  free(line);
  return NULL;
}

/*****************************************************************************/
void usage(const char *basename)
/*****************************************************************************/
{
  fprintf(stderr, "usage: %s [file]...\n", basename);
  fprintf(stderr, "reads line-oriented xml input from file(s) or from stdin and transforms it into xml format\n");
  fprintf(stderr, "line-oriented format is: <path (skipped)> <element> <type> <content>\n");
  fprintf(stderr, "type maybe one of:\n");
  fprintf(stderr, "  S  start tag (skipped if attributes follow)\n");
  fprintf(stderr, "  A  attribute (of start tag)\n");
  fprintf(stderr, "  E  end tag\n");
  fprintf(stderr, "  C  character data\n");
  fprintf(stderr, "  !  comment\n");
  fprintf(stderr, "  ?  processing instructions\n");
}

/*****************************************************************************/
int main(int argc, char *argv[])
/*****************************************************************************/
{
  int arg;
  FILE *file;
  char *line;

  /* check arguments */
  if (argc > 1) {
    if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)) {
      usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  /* process file(s) */
  if (argc == 1) {
    init();
    print_declaration();
    while ((line = read_line(stdin)) != NULL) {
      process_line(line);
      free(line);
    }
  } else {
    for (arg = 1; arg < argc; arg++) {
      if ((file = fopen(argv[arg], "rt")) != NULL) {
	init();
	print_declaration();
	while ((line = read_line(file)) != NULL) {
	  process_line(line);
	  free(line);
	}
	fclose(file);
      }
    }
  }

  return EXIT_SUCCESS;
}

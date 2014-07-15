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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libxml/SAX.h>
#include <libxml/parserInternals.h>

/*
 * TODO:
 *
 * - parse comments correctly
 */

#define CHUNK_SIZE  4096


typedef struct _doc_node {
  struct _doc_node  *parent;
  char              *name;
  int               content_length;
  xmlChar           *content;
} doc_node;

static doc_node *path = NULL;


/*****************************************************************************/
void _print_path(doc_node *path)
/*****************************************************************************/
{
  if (path) {
    _print_path(path->parent);
    putchar('/');
    fputs(path->name, stdout);
  }
}

/*****************************************************************************/
void print_path(void)
/*****************************************************************************/
{
  if (path) {
    _print_path(path);
    putchar(' ');
    fputs(path->name, stdout);
  } else {
    printf("- - ");
  }
}

/*****************************************************************************/
void flush_content(void)
/*****************************************************************************/
{
  if ((path) && (path->content)) {
    const xmlChar *begin, *end;

    begin = path->content;
    while (*begin) {
      /* skip leading white spaces */
      while ((*begin) && (isspace(*begin))) begin++;
      end = begin;
      while ((*end) && (*end != '\n')) end++;

      if (*begin) {
	xmlChar *tmp;

	/* prepare one line */
	tmp = malloc(end - begin + 1);
	memcpy(tmp, begin, end - begin);
	tmp[end - begin] = '\0';

	/* print content */
	print_path();
	fputs(" C ", stdout);
	fputs(tmp, stdout);
	putchar('\n');

	free(tmp);
	begin = end;
      }
    }

    free(path->content);
    path->content_length = 0;
    path->content = NULL;
  }
}

/*****************************************************************************/
static void start_element(void *ctx, const xmlChar *name, const xmlChar **att)
/*****************************************************************************/
{
  int len;
  doc_node *node;

  /* flush contents of previous tag */
  flush_content();

  /* add node to path */
  node = malloc(sizeof(doc_node));
  node->parent = path;
  len = strlen(name) + 1;
  node->name = malloc(sizeof(xmlChar) * len);
  memcpy(node->name, name, sizeof(xmlChar) * len);
  node->content_length = 0;
  node->content = NULL;
  path = node;

  /* print start tag */
  print_path();
  fputs(" S", stdout);
  putchar('\n');

  /* print attributes */
  while ((att != NULL) && (*att != NULL)) {
    print_path();
    fputs(" A", stdout);
    printf(" %s %s\n", att[0], att[1]);
    att += 2;
  }
}

/*****************************************************************************/
static void end_element(void *ctx, const xmlChar *name)
/*****************************************************************************/
{
  doc_node *node;

  /* flush contents */
  flush_content();

  /* print end tag */
  print_path();
  fputs(" E\n", stdout);
  
  /* remove node from path */
  if (path) {
    if (strcmp(name, path->name) == 0) {
      node = path;
      path = node->parent;
      if (node->name) free(node->name);
      free(node);
    }
  }
}

/*****************************************************************************/
static void myCharacters(void *ctx, const xmlChar *ch, int len)
/*****************************************************************************/
{
  /* add contents to buffer */
  path->content = realloc(path->content, sizeof(xmlChar) *
			  (path->content_length + len + 1));
  memcpy(path->content + path->content_length, ch, sizeof(xmlChar) * len);
  path->content_length += len;
  path->content[path->content_length] = '\0';
}

/*****************************************************************************/
static void pi(void *ctx, const xmlChar *target, const xmlChar *data)
/*****************************************************************************/
{
  /* flush contents */
  flush_content();

  /* print pi tag */
  print_path();
  fputs(" ? ", stdout);

  /* print pi content */
  printf("%s %s\n", target, data);
}

/*****************************************************************************/
static void myComment(void *ctx, const xmlChar *ch)
/*****************************************************************************/
{
  const xmlChar *begin, *end;

  /* flush contents */
  flush_content();

  /* print path */
  print_path();
  fputs(" ! ", stdout);

  begin = ch;
  while (*begin) {
    end = begin;
    while ((*end) && (*end != '\n')) end++;

    if (*begin) {
      xmlChar *tmp;

      /* prepare one line */
      tmp = malloc(end - begin + 1);
      memcpy(tmp, begin, end - begin);
      tmp[end - begin] = '\0';

      /* print comment */
      fputs(tmp, stdout);
      if (*end) printf("\\n");

      free(tmp);
      begin = end;
      if (*end) begin++;
    }
  }
  putchar('\n');
}

/*****************************************************************************/
static void error(void *ctx, const char *msg, ...)
/*****************************************************************************/
{
  va_list ap;

  fprintf(stderr, "error in line %d column %d: ", 
	  getLineNumber(ctx),
	  getColumnNumber(ctx));

  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
}

/*****************************************************************************/
void usage(const char *basename)
/*****************************************************************************/
{
  fprintf(stderr, "usage: %s [file]...\n", basename);
  fprintf(stderr, "reads xml input from file(s) or from stdin and transforms it into a line-oriented format\n");
  fprintf(stderr, "line-oriented format is: <path> <element> <type> <content>\n");
  fprintf(stderr, "type maybe one of:\n");
  fprintf(stderr, "  S  start tag\n");
  fprintf(stderr, "  A  attribute (of start tag)\n");
  fprintf(stderr, "  E  end tag\n");
  fprintf(stderr, "  C  character data\n");
  fprintf(stderr, "  !  comment\n");
  fprintf(stderr, "  ?  processing instruction\n");
}

/*****************************************************************************/
int main(int argc, char *argv[])
/*****************************************************************************/
{
  int arg;
  xmlSAXHandler handler;
  xmlParserCtxtPtr context;

  /* check arguments */
  if (argc > 1) {
    if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)) {
      usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  /* initialize sax parser */
  memset(&handler, 0, sizeof(handler));
  handler.startElement          = start_element;
  handler.endElement            = end_element;
  handler.characters            = myCharacters;
  handler.comment               = myComment;
  handler.processingInstruction = pi;
  handler.warning               = error;
  handler.error                 = error;
  handler.fatalError            = error;

  if (argc == 1) {
    context = xmlCreateFileParserCtxt("-");
    if (context == NULL) {
      perror("stdin");
      return EXIT_FAILURE;
    }
    context->sax = &handler;
    xmlParseDocument(context);
    context->sax = 0;
    xmlFreeParserCtxt(context);
  } else {
    for (arg = 1; arg < argc; arg++) {
      context = xmlCreateFileParserCtxt(argv[arg]);
      if (context == NULL) {
        perror(argv[arg]);
	return EXIT_FAILURE;
      }
      context->sax = &handler;
      xmlParseDocument(context);
      context->sax = 0;
      xmlFreeParserCtxt(context);
    }
  }

  return EXIT_SUCCESS;
}

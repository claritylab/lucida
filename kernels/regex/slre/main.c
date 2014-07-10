#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slre.h"

static int static_total_tests = 0;
static int static_failed_tests = 0;

#define MAXCAPS 20000

#define FAIL(str, line) do {                      \
  printf("Fail on line %d: [%s]\n", line, str);   \
  static_failed_tests++;                          \
} while (0)

#define ASSERT(expr) do {               \
  static_total_tests++;                 \
  if (!(expr)) FAIL(#expr, __LINE__);   \
} while (0)

/* Regex must have exactly one bracket pair */
static char *slre_replace(const char *regex, const char *buf,const char *sub)
{
  char *s = NULL;
  int n, n1, n2, n3, s_len, len = strlen(buf);
  struct slre_cap cap = { NULL, 0 };

  do {
    s_len = s == NULL ? 0 : strlen(s);
    if ((n = slre_match(regex, buf, len, &cap, 1)) > 0) {
      n1 = cap.ptr - buf, n2 = strlen(sub),
         n3 = &buf[n] - &cap.ptr[cap.len];
    } else {
      n1 = len, n2 = 0, n3 = 0;
    }
    s = (char *) realloc(s, s_len + n1 + n2 + n3 + 1);
    memcpy(s + s_len, buf, n1);
    memcpy(s + s_len + n1, sub, n2);
    memcpy(s + s_len + n1 + n2, cap.ptr + cap.len, n3);
    s[s_len + n1 + n2 + n3] = '\0';

    buf += n > 0 ? n : len;
    len -= n > 0 ? n : len;
  } while (len > 0);

  return s;
}

int fill(FILE * f, char **toFill, int *bufLen)
{
    int i = 0;
    int ch;
	while(1)
	{
		ch = getc(f);
     	if (ch == EOF )
			break;
		bufLen[i] = 0;
		char * s = (char *) malloc(5000+1);
    	while(1)
    	{
		   	s[bufLen[i]] = ch; 
			++bufLen[i];
    	    ch = getc(f);
			if( ch == '\n')
		    {
				 s[bufLen[i]] = 0; 
				 toFill[i] = s;
				 ++i;
				 break; 
		    }
        }
	}
    return i;
} 

int main(int argc, char * argv[])
{
    struct slre_cap caps[MAXCAPS];
    char *regexps[MAXCAPS];
    char *bufs[MAXCAPS];
    int temp[MAXCAPS], buf_len[MAXCAPS];
    int numExps, numQs;

    FILE * f = fopen(argv[1],"r");
    if (f == 0) { fprintf(stderr,"File %s not found\n",argv[1]); exit(1); }

    FILE * f1 = fopen(argv[2],"r");
    if (f1 == 0) { fprintf(stderr,"File %s not found\n",argv[2]); exit(1); }

    numExps = fill(f, regexps, temp);
    numQs = fill(f1, bufs, buf_len);
    /* printf("%d %d", numExps, numQs); */

    /* printf("%d\n",slre_match(regexps[0], bufs[0], buf_len[0], caps, MAXCAPS)); */
    /* ASSERT(slre_match(regexps[0], bufs[0], buf_len[0], caps, MAXCAPS) > -2); */

    int breakout=0;
    for (int i = 0; i < numExps; ++i){
        for (int k = 0; k < numQs; ++k){
            ASSERT(slre_match(regexps[i], bufs[k], buf_len[k], caps, MAXCAPS) > -2);
            /* if(slre_match(regexps[i], bufs[k], buf_len[k], caps, MAXCAPS) < -1){ */
            /*     printf("err: %d\n",slre_match(regexps[i], bufs[k], buf_len[k], caps, MAXCAPS)); */
            /*     #<{(| printf("%s %s\n",regexps[i], bufs[k]); |)}># */
            /*     breakout=1; */
            /*     break; */
            /* } */
        }
        if(breakout)
            break;
    }

  printf("Unit test %s (total test: %d, failed tests: %d)\n",
         static_failed_tests > 0 ? "FAILED" : "PASSED",
         static_total_tests, static_failed_tests);

  return static_failed_tests == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

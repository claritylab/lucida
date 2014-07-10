/* Johann Hauswald */
/* jahausw@umich.edu */
/* 2014 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <pthread.h>

#include "slre.h"

static int static_total_tests = 0;
static int static_failed_tests = 0;

#define MAXCAPS 20000
#define NTHREADS 8

/* Data */
struct slre_cap caps[MAXCAPS];
char *regexps[MAXCAPS];
char *bufs[MAXCAPS];
int temp[MAXCAPS], buf_len[MAXCAPS];
int numExps, numQs, iterations;

#define FAIL(str, line) do {                      \
  printf("Fail on line %d: [%s]\n", line, str);   \
  static_failed_tests++;                          \
} while (0)

#define ASSERT(expr) do {               \
  static_total_tests++;                 \
  if (!(expr)) FAIL(#expr, __LINE__);   \
} while (0)

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

void * regex_thread(void *tid)
{
	int  start, *mytid, end;
	mytid = (int *) tid;
	start = (*mytid * iterations);
	end = start + iterations;

	// printf ("Thread %d doing iterations %d to %d\n", *mytid, start, end-1);

    for (int i = start; i < end; ++i){
        for (int k = 0; k < numQs; ++k){
            ASSERT(slre_match(regexps[i], bufs[k], buf_len[k], caps, MAXCAPS) > -2);
        }
    }
}

void serial()
{
    for (int i = 0; i < numExps; ++i){
        for (int k = 0; k < numQs; ++k){
            ASSERT(slre_match(regexps[i], bufs[k], buf_len[k], caps, MAXCAPS) > -2);
        }
    }
}

int main(int argc, char * argv[])
{

    /* Timing */
	struct timeval tv1, tv2;
    unsigned int totalruntimeseq = 0;
    unsigned int totalruntimepar = 0;
    // unsigned int totalruntimetau = 0;

    FILE * f = fopen(argv[1],"r");
    if (f == 0) { fprintf(stderr,"File %s not found\n",argv[1]); exit(1); }

    FILE * f1 = fopen(argv[2],"r");
    if (f1 == 0) { fprintf(stderr,"File %s not found\n",argv[2]); exit(1); }

    numExps = fill(f, regexps, temp);
    numQs = fill(f1, bufs, buf_len);
    printf("Regexps: %d Qs: %d\n", numExps, numQs);

    /* Serial */
    gettimeofday(&tv1,NULL);
    serial();
    gettimeofday(&tv2,NULL);
    totalruntimeseq = (tv2.tv_sec-tv1.tv_sec)*1000000 + (tv2.tv_usec-tv1.tv_usec);

    // Parallel
    gettimeofday(&tv1,NULL);
	int start, tids[NTHREADS];
	pthread_t threads[NTHREADS];
    pthread_attr_t attr;
	iterations =  (numExps / NTHREADS);
	pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (int i = 0; i<NTHREADS; i++){
        tids[i] = i;
        pthread_create(&threads[i], &attr, regex_thread, (void *) &tids[i]);
    }

    for (int i=0; i<NTHREADS; i++)
     	 pthread_join(threads[i], NULL);

    gettimeofday(&tv2,NULL);
    totalruntimepar = (tv2.tv_sec-tv1.tv_sec)*1000000 + (tv2.tv_usec-tv1.tv_usec);
    printf("Seq time: %.2f ms\n", (double)totalruntimeseq);
    printf("Par time: %.2f ms\n", (double)totalruntimepar);
    printf("Speedup: %.2f \n", (double)totalruntimeseq/(double)totalruntimepar);


    // printf("Unit test %s (total test: %d, failed tests: %d)\n",
    //         static_failed_tests > 0 ? "FAILED" : "PASSED",
    //         static_total_tests, static_failed_tests);

    return static_failed_tests == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

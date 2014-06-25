#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <ctype.h>       /* for isupper, islower, tolower */
#include <pthread.h>

#include <time.h>
	
	int iterations,count,count1;
	regex_t r[8];
    const char * regex_text[500];
    char  *find_text[500];

/* The following is the size of a buffer to contain any error messages
   encountered when the regular expression is compiled. */

#define MAX_ERROR_MSG 0x1000

/* Compile the regular expression described by "regex_text" into
   "r". */

static int compile_regex (regex_t * r, const char * regex_text)
{
	
    int status = regcomp (r, regex_text, REG_EXTENDED|REG_NEWLINE);
//	printf("777777777\n");
    if (status != 0) 
	{
//		printf("888888888\n");	
        char error_message[MAX_ERROR_MSG];
        regerror (status, r, error_message, MAX_ERROR_MSG);
        printf ("Regex error compiling '%s': %s\n",
                regex_text, error_message);
        return 1;
    }
    return 0;
}

/*
   Match the string in "to_match" against the compiled regular
   expression in "r".
   */

static int match_regex (regex_t * r, const char * to_match)
{
    /* "P" is a pointer into the string which points to the end of the
       previous match. */
    const char * p = to_match;
    /* "N_matches" is the maximum number of matches allowed. */
    const int n_matches = 10;
    /* "M" contains the matches found. */
    regmatch_t m[n_matches];

    while (1) 
	{
        int i = 0;
        int nomatch = regexec (r, p, n_matches, m, 0);
        if (nomatch) 
		{
           // printf ("No more matches.\n");
            return nomatch;
        }
        for (i = 0; i < n_matches; i++) 
		{
            int start;
            int finish;
            if (m[i].rm_so == -1) 
			{
                break;
            }
            start = m[i].rm_so + (p - to_match);
            finish = m[i].rm_eo + (p - to_match);
            if (i == 0) 
			{
                printf ("$& match in '%s'", to_match);
            }
            else 
			{
                printf ("$%d match case is ", i);
            }
		//	printf ("Trying to find in '%s'", to_match);
			//printf("\n");
            printf ("'%.*s' (bytes %d:%d)\n", (finish - start),
                    to_match + start, start, finish);
        }
        p += m[0].rm_eo;
    }
    return 0;
}

//static char * s;         /* a char * (=string) pointer;*/

float calculateMilisecondsTimeSpec(struct timespec t1, struct timespec t2) {
	float elapsedTime;
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
	elapsedTime += (t2.tv_nsec - t1.tv_nsec) / 1000000.0;
	return elapsedTime;
}

void * regex_thread(void *tid)
{
	int  start, *mytid, end;
	//printf("11111111\n");
	mytid = (int *) tid;
	start = (*mytid * iterations);
	end = start + iterations;

	printf ("Thread %d doing iterations %d to %d\n", *mytid, start, end-1);

	for (int i = start ; i< end;i++)
	{ 
		compile_regex(& r[*mytid], regex_text[i]);
		for (int j = 0; j < count1;j++)
			match_regex(& r[*mytid], find_text[j]);  

	}

     pthread_exit(NULL);
	
}
#include <limits.h>
#include <float.h>
#include <math.h>
#include <sys/time.h>

#define ARRAYSIZE 4100000
#define NTHREADS  8
#define ITERATIONS   ARRAYSIZE / NTHREADS

int main(int argc, char * argv[])
{
    
	struct timespec t_start, t_end;
	int ch,ch1;

	  FILE * f = fopen(argv[1],"r");
      if (f == 0) { fprintf(stderr,"File %s not found\n",argv[1]); exit(1); }
	
	  FILE * f1 = fopen(argv[2],"r");
      if (f1 == 0) { fprintf(stderr,"File %s not found\n",argv[2]); exit(1); }
	  		
	

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	count = 0;
	while(1)	
	{
		ch = getc(f);
     	if (ch == EOF )
		{
			//return 0;	
			break;
		}
		int i = 0;
		char * s = (char *) malloc(5000+1);
    	while(1)
    	{            	    	  
		   	s[i] = ch; 
			i++;
    	    ch = getc(f);
			if ( ch == '\n')
		    { 		
				 s[i] = 0; 
				 //ungetc(ch,f); 
				 regex_text[count] = s;

				 count++; 

				 break; 
		    } 	   
        }  
	}

	count1 = 0;
	while(1)	
	{
		ch1 = getc(f1);
     	if (ch1 == EOF )
		{
			//return 0;	
			break;
		}
		int i = 0;
		char * s = (char *) malloc(5000+1);
    	while(1)
    	{            	    	  
		   	s[i] = ch1; 
			i++;
    	    ch1 = getc(f1);
			if ( ch1 == '\n')
		    { 		
				 s[i] = 0; 
				 //ungetc(ch,f); 
				 find_text[count1] = s;

				 count1++; 

				 break; 
		    } 	   
        }  
	}


clock_gettime(CLOCK_REALTIME, &t_start);

	int start, tids[NTHREADS];
	pthread_t threads[NTHREADS];
    pthread_attr_t attr;
	iterations =  (count / NTHREADS) ;
//	iterations = count / 2 ;
	 printf("array_size: %d\n", count);
	 printf("iterations %d\n", iterations);


	pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	//printf("333333333\n");
	
//	tids[0] = 0;	
//	 pthread_create(&threads[0], &attr, regex_thread,  (void *) &tids[0]);

//	 tids[1] = 1;	
//	 pthread_create(&threads[1], &attr, regex_thread,  (void *) &tids[1]);


//	  pthread_join(threads[0], NULL);
//	   pthread_join(threads[1], NULL);


    for (int i=0; i<NTHREADS; i++) 
	{
        tids[i] = i;
		//printf("444444444\n");
        pthread_create(&threads[i], &attr, regex_thread, (void *) &tids[i]);
		
    }

    for (int i=0; i<NTHREADS; i++) 
	{
     	 pthread_join(threads[i], NULL);
    }

	
		
		

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	

	clock_gettime(CLOCK_REALTIME, &t_end);

	float cpu_elapsedTime = calculateMilisecondsTimeSpec(t_start, t_end);
    printf("\nCPU Time=%.2f ms\n",  cpu_elapsedTime);
   // regfree (& r);
	//fclose(f);
	//fclose(f1);
    return 0;
}

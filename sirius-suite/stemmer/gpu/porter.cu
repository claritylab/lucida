/* This is the Porter stemming algorithm, coded up as thread-safe ANSI C
  by the author.

  It may be be regarded as cononical, in that it follows the algorithm
  presented in

  Porter, 1980, An algorithm for suffix stripping, Program, Vol. 14,
  no. 3, pp 130-137,

  only differing from it at the points maked --DEPARTURE-- below.

  See also http://www.tartarus.org/~martin/PorterStemmer

  The algorithm as described in the paper could be exactly replicated
  by adjusting the points of DEPARTURE, but this is barely necessary,
  because (a) the points of DEPARTURE are definitely improvements, and
  (b) no encoding of the Porter stemmer I have seen is anything like
  as exact as this version, even with the points of DEPARTURE!

  You can compile it on Unix with 'gcc -O3 -o stem stem.c' after which
  'stem' takes a list of inputs and sends the stemmed equivalent to
  stdout.

  The algorithm as encoded here is particularly fast.

  Release 2 (the more old-fashioned, non-thread-safe version may be
  regarded as release 1.)
*/

#include <stdio.h>
#include <stdlib.h> /* for malloc, free */
#include <ctype.h>  /* for isupper, islower, tolower */
#include <string.h> /* for memcmp, memmove */
#include <cuda_runtime.h>
#include <pthread.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <sys/time.h>

#include "../../timer/timer.h"

/* You will probably want to move the following declarations to a central
  header file.
*/

struct stemmer;

extern struct stemmer *create_stemmer(void);
extern void free_stemmer(struct stemmer *z);

extern int stem(struct stemmer *z, char *b, int k);

/* The main part of the stemming algorithm starts here.
*/

#define TRUE 1
#define FALSE 0

#define ARRAYSIZE 1000000
#define A_INC 10000
#define INC 32 /* size units in which s is increased */

static int a_max = ARRAYSIZE;
static int i_max = INC; /* maximum offset in s */

/* stemmer is a structure for a few local bits of data,
*/

struct stemmer {
//  char *b; /* buffer for word to be stemmed */
  char b[INC+1]; /* buffer for word to be stemmed */
  int k;   /* offset to the end of the string */
  int j;   /* a general offset into the string */
};

/* Member b is a buffer holding a word to be stemmed. The letters are in
  b[0], b[1] ... ending at b[z->k]. Member k is readjusted downwards as
  the stemming progresses. Zero termination is not in fact used in the
  algorithm.

  Note that only lower case sequences are stemmed. Forcing to lower case
  should be done before stem(...) is called.


  Typical usage is:

      struct stemmer * z = create_stemmer();
      char b[] = "pencils";
      int res = stem(z, b, 6);
          /- stem the 7 characters of b[0] to b[6]. The result, res,
             will be 5 (the 's' is removed). -/
      free_stemmer(z);
*/

extern struct stemmer *create_stemmer(void) {
  return (struct stemmer *)malloc(sizeof(struct stemmer));
  /* assume malloc succeeds */
}

extern void free_stemmer(struct stemmer *z) { free(z); }

/* cons(z, i) is TRUE <=> b[i] is a consonant. ('b' means 'z->b', but here
  and below we drop 'z->' in comments.
*/

__host__ __device__ static int cons1(struct stemmer *z, int i) {
  switch (z->b[i]) {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
      return FALSE;
    default:
      return TRUE;
  }
}

__host__ __device__ static int cons(struct stemmer *z, int i) {
  switch (z->b[i]) {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
      return FALSE;
    case 'y':
      return (i == 0) ? TRUE : !cons1(z, i - 1);
    default:
      return TRUE;
  }
}

/* m(z) measures the number of consonant sequences between 0 and j. if c is
  a consonant sequence and v a vowel sequence, and <..> indicates arbitrary
  presence,

     <c><v>       gives 0
     <c>vc<v>     gives 1
     <c>vcvc<v>   gives 2
     <c>vcvcvc<v> gives 3
     ....
*/

__host__ __device__ static int m(struct stemmer *z) {
  int n = 0;
  int i = 0;
  int j = z->j;
  while (TRUE) {
    if (i > j) return n;
    if (!cons(z, i)) break;
    i++;
  }
  i++;
  while (TRUE) {
    while (TRUE) {
      if (i > j) return n;
      if (cons(z, i)) break;
      i++;
    }
    i++;
    n++;
    while (TRUE) {
      if (i > j) return n;
      if (!cons(z, i)) break;
      i++;
    }
    i++;
  }
}

/* vowelinstem(z) is TRUE <=> 0,...j contains a vowel */

__host__ __device__ static int vowelinstem(struct stemmer *z) {
  int j = z->j;
  int i;
  for (i = 0; i <= j; i++)
    if (!cons(z, i)) return TRUE;
  return FALSE;
}

/* doublec(z, j) is TRUE <=> j,(j-1) contain a double consonant. */

__host__ __device__ static int doublec(struct stemmer *z, int j) {
  char *b = z->b;
  if (j < 1) return FALSE;
  if (b[j] != b[j - 1]) return FALSE;
  return cons(z, j);
}

/* cvc(z, i) is TRUE <=> i-2,i-1,i has the form consonant - vowel - consonant
  and also if the second c is not w,x or y. this is used when trying to
  restore an e at the end of a short word. e.g.

     cav(e), lov(e), hop(e), crim(e), but
     snow, box, tray.

*/

__host__ __device__ static int cvc(struct stemmer *z, int i) {
  if (i < 2 || !cons(z, i) || cons(z, i - 1) || !cons(z, i - 2)) return FALSE;
  {
    int ch = z->b[i];
    if (ch == 'w' || ch == 'x' || ch == 'y') return FALSE;
  }
  return TRUE;
}

/* ends(z, s) is TRUE <=> 0,...k ends with the string s. */

__host__ __device__ static int memcmp1(const void *buffer1, const void *buffer2,
                                       int count) {
  if (!count) return (0);
  while (--count && *(char *)buffer1 == *(char *)buffer2) {
    buffer1 = (char *)buffer1 + 1;
    buffer2 = (char *)buffer2 + 1;
  }
  return (*((unsigned char *)buffer1) - *((unsigned char *)buffer2));
}

__host__ __device__ static int ends(struct stemmer *z, char *s) {
  int length = s[0];
  char *b = z->b;
  int k = z->k;
  if (s[length] != b[k]) return FALSE; /* tiny speed-up */
  if (length > k + 1) return FALSE;
  if (memcmp1(b + k - length + 1, s + 1, length) != 0) return FALSE;
  z->j = k - length;
  return TRUE;
}

/* setto(z, s) sets (j+1),...k to the characters in the string s, readjusting
  k. */

__host__ __device__ void memmove1(void *dst, const void *src, int count) {
  char *dst_t;
  char *src_t;
  if ((unsigned char *)dst <= (unsigned char *)src ||
      (unsigned char *)dst >= ((unsigned char *)src + count)) {
    dst_t = (char *)dst;
    src_t = (char *)src;
    while (count--) {
      *dst_t++ = *src_t++;
    }
  } else {
    dst_t = (char *)dst + count - 1;
    src_t = (char *)src + count - 1;
    while (count--) {
      *dst_t-- = *src_t--;
    }
  }
}

__host__ __device__ static void setto(struct stemmer *z, char *s) {
  int length = s[0];
  int j = z->j;
  memmove1(z->b + j + 1, s + 1, length);
  z->k = j + length;
}

/* r(z, s) is used further down. */

__host__ __device__ static void r(struct stemmer *z, char *s) {
  if (m(z) > 0) setto(z, s);
}

/* step1ab(z) gets rid of plurals and -ed or -ing. e.g.

      caresses  ->  caress
      ponies    ->  poni
      ties      ->  ti
      caress    ->  caress
      cats      ->  cat

      feed      ->  feed
      agreed    ->  agree
      disabled  ->  disable

      matting   ->  mat
      mating    ->  mate
      meeting   ->  meet
      milling   ->  mill
      messing   ->  mess

      meetings  ->  meet

*/

/* In stem(z, b, k), b is a char pointer, and the string to be stemmed is
  from b[0] to b[k] inclusive.  Possibly b[k+1] == '\0', but it is not
  important. The stemmer adjusts the characters b[0] ... b[k] and returns
  the new end-point of the string, k'. Stemming never increases word
  length, so 0 <= k' <= k.
*/

__host__ __device__ static void step1ab(struct stemmer *z) {
  char *b = z->b;
  if (b[z->k] == 's') {
    if (ends(z,
             "\04"
             "sses"))
      z->k -= 2;
    else if (ends(z,
                  "\03"
                  "ies"))
      setto(z,
            "\01"
            "i");
    else if (b[z->k - 1] != 's')
      z->k--;
  }
  if (ends(z,
           "\03"
           "eed")) {
    if (m(z) > 0) z->k--;
  } else if ((ends(z,
                   "\02"
                   "ed") ||
              ends(z,
                   "\03"
                   "ing")) &&
             vowelinstem(z)) {
    z->k = z->j;
    if (ends(z,
             "\02"
             "at"))
      setto(z,
            "\03"
            "ate");
    else if (ends(z,
                  "\02"
                  "bl"))
      setto(z,
            "\03"
            "ble");
    else if (ends(z,
                  "\02"
                  "iz"))
      setto(z,
            "\03"
            "ize");
    else if (doublec(z, z->k)) {
      z->k--;
      {
        int ch = b[z->k];
        if (ch == 'l' || ch == 's' || ch == 'z') z->k++;
      }
    } else if (m(z) == 1 && cvc(z, z->k))
      setto(z,
            "\01"
            "e");
  }
}

/* step1c(z) turns terminal y to i when there is another vowel in the stem. */

__host__ __device__ static void step1c(struct stemmer *z) {
  if (ends(z,
           "\01"
           "y") &&
      vowelinstem(z))
    z->b[z->k] = 'i';
}

/* step2(z) maps double suffices to single ones. so -ization ( = -ize plus
  -ation) maps to -ize etc. note that the string before the suffix must give
  m(z) > 0. */

__host__ __device__ static void step2(struct stemmer *z) {
  switch (z->b[z->k - 1]) {
    case 'a':
      if (ends(z,
               "\07"
               "ational")) {
        r(z,
          "\03"
          "ate");
        break;
      }
      if (ends(z,
               "\06"
               "tional")) {
        r(z,
          "\04"
          "tion");
        break;
      }
      break;
    case 'c':
      if (ends(z,
               "\04"
               "enci")) {
        r(z,
          "\04"
          "ence");
        break;
      }
      if (ends(z,
               "\04"
               "anci")) {
        r(z,
          "\04"
          "ance");
        break;
      }
      break;
    case 'e':
      if (ends(z,
               "\04"
               "izer")) {
        r(z,
          "\03"
          "ize");
        break;
      }
      break;
    case 'l':
      if (ends(z,
               "\03"
               "bli")) {
        r(z,
          "\03"
          "ble");
        break;
      } /*-DEPARTURE-*/

      /* To match the published algorithm, replace this line with
         case 'l': if (ends(z, "\04" "abli")) { r(z, "\04" "able"); break; } */

      if (ends(z,
               "\04"
               "alli")) {
        r(z,
          "\02"
          "al");
        break;
      }
      if (ends(z,
               "\05"
               "entli")) {
        r(z,
          "\03"
          "ent");
        break;
      }
      if (ends(z,
               "\03"
               "eli")) {
        r(z,
          "\01"
          "e");
        break;
      }
      if (ends(z,
               "\05"
               "ousli")) {
        r(z,
          "\03"
          "ous");
        break;
      }
      break;
    case 'o':
      if (ends(z,
               "\07"
               "ization")) {
        r(z,
          "\03"
          "ize");
        break;
      }
      if (ends(z,
               "\05"
               "ation")) {
        r(z,
          "\03"
          "ate");
        break;
      }
      if (ends(z,
               "\04"
               "ator")) {
        r(z,
          "\03"
          "ate");
        break;
      }
      break;
    case 's':
      if (ends(z,
               "\05"
               "alism")) {
        r(z,
          "\02"
          "al");
        break;
      }
      if (ends(z,
               "\07"
               "iveness")) {
        r(z,
          "\03"
          "ive");
        break;
      }
      if (ends(z,
               "\07"
               "fulness")) {
        r(z,
          "\03"
          "ful");
        break;
      }
      if (ends(z,
               "\07"
               "ousness")) {
        r(z,
          "\03"
          "ous");
        break;
      }
      break;
    case 't':
      if (ends(z,
               "\05"
               "aliti")) {
        r(z,
          "\02"
          "al");
        break;
      }
      if (ends(z,
               "\05"
               "iviti")) {
        r(z,
          "\03"
          "ive");
        break;
      }
      if (ends(z,
               "\06"
               "biliti")) {
        r(z,
          "\03"
          "ble");
        break;
      }
      break;
    case 'g':
      if (ends(z,
               "\04"
               "logi")) {
        r(z,
          "\03"
          "log");
        break;
      } /*-DEPARTURE-*/

      /* To match the published algorithm, delete this line */
  }
}

/* step3(z) deals with -ic-, -full, -ness etc. similar strategy to step2. */

__host__ __device__ static void step3(struct stemmer *z) {
  switch (z->b[z->k]) {
    case 'e':
      if (ends(z,
               "\05"
               "icate")) {
        r(z,
          "\02"
          "ic");
        break;
      }
      if (ends(z,
               "\05"
               "ative")) {
        r(z,
          "\00"
          "");
        break;
      }
      if (ends(z,
               "\05"
               "alize")) {
        r(z,
          "\02"
          "al");
        break;
      }
      break;
    case 'i':
      if (ends(z,
               "\05"
               "iciti")) {
        r(z,
          "\02"
          "ic");
        break;
      }
      break;
    case 'l':
      if (ends(z,
               "\04"
               "ical")) {
        r(z,
          "\02"
          "ic");
        break;
      }
      if (ends(z,
               "\03"
               "ful")) {
        r(z,
          "\00"
          "");
        break;
      }
      break;
    case 's':
      if (ends(z,
               "\04"
               "ness")) {
        r(z,
          "\00"
          "");
        break;
      }
      break;
  }
}

/* step4(z) takes off -ant, -ence etc., in context <c>vcvc<v>. */

__host__ __device__ static void step4(struct stemmer *z) {
  switch (z->b[z->k - 1]) {
    case 'a':
      if (ends(z,
               "\02"
               "al"))
        break;
      return;
    case 'c':
      if (ends(z,
               "\04"
               "ance"))
        break;
      if (ends(z,
               "\04"
               "ence"))
        break;
      return;
    case 'e':
      if (ends(z,
               "\02"
               "er"))
        break;
      return;
    case 'i':
      if (ends(z,
               "\02"
               "ic"))
        break;
      return;
    case 'l':
      if (ends(z,
               "\04"
               "able"))
        break;
      if (ends(z,
               "\04"
               "ible"))
        break;
      return;
    case 'n':
      if (ends(z,
               "\03"
               "ant"))
        break;
      if (ends(z,
               "\05"
               "ement"))
        break;
      if (ends(z,
               "\04"
               "ment"))
        break;
      if (ends(z,
               "\03"
               "ent"))
        break;
      return;
    case 'o':
      if (ends(z,
               "\03"
               "ion") &&
          (z->b[z->j] == 's' || z->b[z->j] == 't'))
        break;
      if (ends(z,
               "\02"
               "ou"))
        break;
      return;
    /* takes care of -ous */
    case 's':
      if (ends(z,
               "\03"
               "ism"))
        break;
      return;
    case 't':
      if (ends(z,
               "\03"
               "ate"))
        break;
      if (ends(z,
               "\03"
               "iti"))
        break;
      return;
    case 'u':
      if (ends(z,
               "\03"
               "ous"))
        break;
      return;
    case 'v':
      if (ends(z,
               "\03"
               "ive"))
        break;
      return;
    case 'z':
      if (ends(z,
               "\03"
               "ize"))
        break;
      return;
    default:
      return;
  }
  if (m(z) > 1) z->k = z->j;
}

/* step5(z) removes a final -e if m(z) > 1, and changes -ll to -l if
  m(z) > 1. */

__host__ __device__ static void step5(struct stemmer *z) {
  char *b = z->b;
  z->j = z->k;
  if (b[z->k] == 'e') {
    int a = m(z);
    if (a > 1 || a == 1 && !cvc(z, z->k - 1)) z->k--;
  }
  if (b[z->k] == 'l' && doublec(z, z->k) && m(z) > 1) z->k--;
}

/* In stem(z, b, k), b is a char pointer, and the string to be stemmed is
  from b[0] to b[k] inclusive.  Possibly b[k+1] == '\0', but it is not
  important. The stemmer adjusts the characters b[0] ... b[k] and returns
  the new end-point of the string, k'. Stemming never increases word
  length, so 0 <= k' <= k.
*/

#if 0
extern int stem(struct stemmer *z, char *b, int k) {
  if (k <= 1) return k; /*-DEPARTURE-*/
  z->b = b;
  z->k = k; /* copy the parameters into z */

  /* With this line, strings of length 1 or 2 don't go through the
     stemming process, although no mention is made of this in the
     published algorithm. Remove the line to match the published
     algorithm. */

  step1ab(z);
  step1c(z);
  step2(z);
  step3(z);
  step4(z);
  step5(z);
  return z->k;
}

extern int stem2(struct stemmer *z) {
  if (z->k <= 1) return z->k;
  //   if (k <= 1) return k; /*-DEPARTURE-*/
  //   z->b = b; z->k = k; /* copy the parameters into z */

  //    printf("z->b = %s\n", z->b);
  //   printf("z->k = %d\n", z->k);

  /* With this line, strings of length 1 or 2 don't go through the
     stemming process, although no mention is made of this in the
     published algorithm. Remove the line to match the published
     algorithm. */

  step1ab(z);
  step1c(z);
  step2(z);
  step3(z);
  step4(z);
  step5(z);
  return z->k;
}
#endif

__global__ void stem3(struct stemmer *stem_list, int words) {
  int tid = blockIdx.x * blockDim.x + threadIdx.x;
  if (tid < words) {
    if (stem_list[tid].k < 1) return;
    // stem_list[tid]->k=2;return;
    step1ab(&(stem_list[tid]));
    step1c(&(stem_list[tid]));
    step2(&(stem_list[tid]));
    step3(&(stem_list[tid]));
    step4(&(stem_list[tid]));
    step5(&(stem_list[tid]));
  }
}

/*--------------------stemmer definition ends here------------------------*/


struct stemmer *stem_list;

struct stemmer *dev_stem_list;


#define LETTER(ch) (isupper(ch) || islower(ch))

int load_data(struct stemmer *stem_list, FILE *f) {
  int a_size = 0;
  while (TRUE) {
    int ch = getc(f);
    if (ch == EOF) return a_size;
    char *s = (char *)malloc(i_max + 1);
    if (LETTER(ch)) {
      int i = 0;
      
//      struct stemmer *z = create_stemmer();
      
      while (TRUE) {
        if (i == i_max) {
          i_max += INC;
          s = (char *)realloc(s, i_max + 1);
        }
        ch = tolower(ch); /* forces lower case */

        stem_list[a_size].b[i]=ch;
 //       z->b[i]=ch;
        s[i] = ch;
        i++;
        ch = getc(f);
        if (!LETTER(ch)) {
          ungetc(ch, f);
          break;
        }
      }

 //     z->b = s;
//      z->k = i - 1;
       stem_list[a_size].k=i-1;
//      stem_list[a_size] = z;
      if (a_size == a_max) {
        a_max += A_INC;
//        stem_list = (struct stemmer **)realloc(stem_list, a_max);
        stem_list = (struct stemmer *)realloc(stem_list, a_max*sizeof(struct stemmer));
      }
      a_size += 1;
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "[ERROR] Invalid arguments provided.\n\n");
    fprintf(stderr, "Usage: %s [INPUT FILE]\n\n", argv[0]);
    exit(0);
  }
  /* Timing */
  STATS_INIT ("kernel", "gpu_porter_stemming");
  PRINT_STAT_STRING ("abrv", "gpu_stemmer");
  
  cudaEvent_t eStart, eStop;
  float cuda_elapsedTime;

  // allocate data
  FILE *f;
  f = fopen(argv[1], "r");
  if (f == 0) {
    fprintf(stderr, "File %s not found\n", argv[1]);
    exit(1);
  }
/*
  stem_list = (struct stemmer **)malloc(ARRAYSIZE * sizeof(struct stemmer *));
  struct stemmer **host_stem_list =
      (struct stemmer **)malloc(ARRAYSIZE * sizeof(struct stemmer *));
  struct stemmer **host2_stem_list =
      (struct stemmer **)malloc(ARRAYSIZE * sizeof(struct stemmer *));
 */ 

  stem_list = (struct stemmer *)malloc(ARRAYSIZE * sizeof(struct stemmer));

  int words = load_data(stem_list, f);

  PRINT_STAT_INT ("words", words);

  fclose(f);

  cudaEventCreate(&eStart);
  cudaEventCreate(&eStop);
//  cudaMalloc((void **)&dev_stem_list, words * sizeof(struct stemmer *));
  cudaMalloc((void **)&dev_stem_list, words * sizeof(struct stemmer));

  cudaEventRecord(eStart, 0);
  
  cudaMemcpy(dev_stem_list, stem_list, words*sizeof(struct stemmer), cudaMemcpyHostToDevice);

/*
  for (int i = 0; i < words; i++) {
    struct stemmer *host_z = create_stemmer();
    host_stem_list[i] = host_z;
    host_z->k = stem_list[i]->k;
    host_z->j = stem_list[i]->j;
    struct stemmer *dev_z;
    cudaMalloc((void **)&(host_z->b), (i_max + 1) * sizeof(char));
    cudaMalloc((void **)&dev_z, sizeof(struct stemmer));
    cudaMemcpy(host_z->b, stem_list[i]->b, (i_max + 1) * sizeof(char),
               cudaMemcpyHostToDevice);
    cudaMemcpy(dev_z, host_z, sizeof(struct stemmer), cudaMemcpyHostToDevice);
    cudaMemcpy(&(dev_stem_list[i]), &dev_z, sizeof(struct stemmer *),
               cudaMemcpyHostToDevice);
    host2_stem_list[i] = dev_z;
  }
*/
  cudaEventRecord(eStop, 0);
  cudaEventSynchronize(eStop);
  cudaEventElapsedTime(&cuda_elapsedTime, eStart, eStop);
  PRINT_STAT_DOUBLE ("host_to_device", cuda_elapsedTime);

  cudaEventRecord(eStart, 0);
  dim3 block(256);
  dim3 grid;
  grid.x = ceil(words / block.x);
  cudaEventRecord(eStart, 0);
  stem3 << <grid, block>>> (dev_stem_list, words);
  cudaEventRecord(eStop, 0);
  cudaEventSynchronize(eStop);
  cudaEventElapsedTime(&cuda_elapsedTime, eStart, eStop);
  PRINT_STAT_DOUBLE ("gpu_stemmer", cuda_elapsedTime);

  cudaEventRecord(eStart, 0);
  cudaMemcpy(stem_list, dev_stem_list, words*sizeof(struct stemmer), cudaMemcpyDeviceToHost);

/*
  for (int i = 0; i < words; i++) {
    cudaMemcpy(&(stem_list[i]->j), &(host2_stem_list[i]->j), sizeof(int),
               cudaMemcpyDeviceToHost);
    cudaMemcpy(&(stem_list[i]->k), &(host2_stem_list[i]->k), sizeof(int),
               cudaMemcpyDeviceToHost);
    cudaMemcpy(stem_list[i]->b, host_stem_list[i]->b,
               (i_max + 1) * sizeof(char), cudaMemcpyDeviceToHost);
    // stem_list[i]->b[stem_list[i]->k+1]=0;
    // int a=stem_list[i]->k;
    // stem_list[i]->b[a+1]=0;
    // int k=stem_list[i]->k; char *b=stem_list[i]->b; b[k+1]=0;
    // printf("%s\n", stem_list[i]->b);
  }
*/

  cudaEventRecord(eStop, 0);
  cudaEventSynchronize(eStop);
  cudaEventElapsedTime(&cuda_elapsedTime, eStart, eStop);
  PRINT_STAT_DOUBLE ("device_to_host", cuda_elapsedTime);

  cudaEventDestroy(eStart);
  cudaEventDestroy(eStop);

  STATS_END (); 
//  for (int i = 0; i < words; i++) {
//    cudaFree(host_stem_list[i]->b);
//    cudaFree(host2_stem_list[i]);
//  }
  free(stem_list);
  cudaFree(dev_stem_list);

  // free up allocated data
 // for (int i = 0; i < words; i++) {
 //   free(stem_list[i]->b);
 //   free(stem_list[i]);
//    free(host_stem_list[i]);
 // }
//  free(stem_list);
//  free(host_stem_list);
//  free(host2_stem_list);

  return 0;
}
